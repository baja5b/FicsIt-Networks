#include "FINGameInstanceModule.h"

#include "FicsItLogLibrary.h"
#include "FicsItNetworksCircuit.h"
#include "FicsItNetworksComputer.h"
#include "FicsItNetworksLuaModule.h"
#include "FIRModModule.h"

#include "FicsItNetworksModule.h"
#include "Configuration/ConfigManager.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/Properties/ConfigPropertyString.h"
#include "Configuration/Properties/ConfigPropertyArray.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformProcess.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

// Job object that is kept open for the entire lifetime of the game process.
// All startup processes are assigned to it; the limit flag
// JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE ensures that when the game exits
// (cleanly OR via a crash - in which case the OS closes all handles) Windows
// kills every assigned process along with it. This way no orphaned server survives.
static HANDLE GFINStartupJobObject = nullptr;
#endif

// Reads the (local, opt-in) startup-command config and launches each command
// once via the OS. SECURITY: the command list comes ONLY from this local mod
// config file - never from savegames, blueprints, network or Lua.
static void FINLaunchStartupCommands(UGameInstance* GameInstance) {
	if (!GameInstance) return;
	UConfigManager* ConfigManager = GameInstance->GetSubsystem<UConfigManager>();
	if (!ConfigManager) return;
	// Main FicsItNetworks config -> "Startup" section (shown in the SML mod settings menu).
	UConfigPropertySection* Root = ConfigManager->GetConfigurationRootSection(FConfigId{TEXT("FicsItNetworks"), TEXT("")});
	if (!Root) return;
	const UConfigPropertySection* Startup = Cast<UConfigPropertySection>(Root->SectionProperties.FindRef(TEXT("Startup")));
	if (!Startup) return;

	const UConfigPropertyBool* Enable = Cast<UConfigPropertyBool>(Startup->SectionProperties.FindRef(TEXT("bEnableStartupCommands")));
	if (!Enable || !Enable->Value) return;

	const UConfigPropertyArray* Commands = Cast<UConfigPropertyArray>(Startup->SectionProperties.FindRef(TEXT("StartupCommands")));
	if (!Commands) return;

	for (const TObjectPtr<UConfigProperty>& Prop : Commands->Values) {
		const UConfigPropertyString* Str = Cast<UConfigPropertyString>(Prop);
		if (!Str) continue;
		FString CommandLine = Str->Value.TrimStartAndEnd();
		if (CommandLine.IsEmpty()) continue;

		UE_LOG(LogFicsItNetworks, Warning, TEXT("[FIN Startup] launching: %s"), *CommandLine);

#if PLATFORM_WINDOWS
		// Create the job object once (kill-on-close -> children die with the game).
		if (!GFINStartupJobObject) {
			GFINStartupJobObject = CreateJobObject(nullptr, nullptr);
			if (GFINStartupJobObject) {
				JOBOBJECT_EXTENDED_LIMIT_INFORMATION JobLimit = {};
				JobLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
				SetInformationJobObject(GFINStartupJobObject, JobObjectExtendedLimitInformation, &JobLimit, sizeof(JobLimit));
			} else {
				UE_LOG(LogFicsItNetworks, Warning, TEXT("[FIN Startup] CreateJobObject failed (err %u) - processes will NOT be auto-killed on exit"), (uint32)GetLastError());
			}
		}

		// CreateProcessW needs a writable command-line buffer and parses
		// exe + args itself (including quoted paths) - it looks up the exe via PATH.
		TArray<TCHAR> CmdBuffer;
		CmdBuffer.Append(*CommandLine, CommandLine.Len());
		CmdBuffer.Add(TEXT('\0'));

		STARTUPINFOW StartupInfo = {};
		StartupInfo.cb = sizeof(StartupInfo);
		PROCESS_INFORMATION ProcInfo = {};

		// CREATE_NO_WINDOW: never show a console (applies to EVERY program,
		// not just python). CREATE_SUSPENDED: assign to the job first, then start.
		const DWORD CreationFlags = CREATE_NO_WINDOW | CREATE_SUSPENDED;
		if (CreateProcessW(nullptr, CmdBuffer.GetData(), nullptr, nullptr, false, CreationFlags, nullptr, nullptr, &StartupInfo, &ProcInfo)) {
			if (GFINStartupJobObject) {
				if (!AssignProcessToJobObject(GFINStartupJobObject, ProcInfo.hProcess)) {
					UE_LOG(LogFicsItNetworks, Warning, TEXT("[FIN Startup] AssignProcessToJobObject failed (err %u) - this process won't be auto-killed on exit"), (uint32)GetLastError());
				}
			}
			ResumeThread(ProcInfo.hThread);
			CloseHandle(ProcInfo.hThread);
			CloseHandle(ProcInfo.hProcess); // The job holds the reference; ours can be closed.
		} else {
			UE_LOG(LogFicsItNetworks, Error, TEXT("[FIN Startup] CreateProcess failed (err %u): %s"), (uint32)GetLastError(), *CommandLine);
		}
#else
		// Non-Windows: detached fallback (without job-object lifetime management).
		FString Exe, Args;
		if (CommandLine.StartsWith(TEXT("\""))) {
			const int32 EndQuote = CommandLine.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1);
			if (EndQuote != INDEX_NONE) {
				Exe = CommandLine.Mid(1, EndQuote - 1);
				Args = CommandLine.Mid(EndQuote + 1).TrimStartAndEnd();
			} else {
				Exe = CommandLine;
			}
		} else {
			int32 SpaceIndex = INDEX_NONE;
			if (CommandLine.FindChar(TEXT(' '), SpaceIndex)) {
				Exe = CommandLine.Left(SpaceIndex);
				Args = CommandLine.Mid(SpaceIndex + 1).TrimStartAndEnd();
			} else {
				Exe = CommandLine;
			}
		}
		FProcHandle Handle = FPlatformProcess::CreateProc(*Exe, *Args, true, false, false, nullptr, 0, nullptr, nullptr);
		if (Handle.IsValid()) {
			FPlatformProcess::CloseProc(Handle);
		} else {
			UE_LOG(LogFicsItNetworks, Error, TEXT("[FIN Startup] failed to launch: %s"), *CommandLine);
		}
#endif
	}
}

UFINGameInstanceModule::UFINGameInstanceModule() {}

void UFINGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase) {
	Super::DispatchLifecycleEvent(Phase);

	switch (Phase) {
	case ELifecyclePhase::CONSTRUCTION:
		SpawnChildModule(TEXT("FicsItReflection"), UFIRGameInstanceModule::StaticClass());
		SpawnChildModule(TEXT("FicsItLogLibrary"), UFILGameInstanceModule::StaticClass());
		SpawnChildModule(TEXT("FicsItNetworksCircuit"), UFINCircuitGameInstanceModule::StaticClass());
		SpawnChildModule(TEXT("FicsItNetworksComputer"), UFINComputerGameInstanceModule::StaticClass());
		SpawnChildModule(TEXT("FicsItNetworksLua"), UFINLuaGameInstanceModule::StaticClass());
		break;
	case ELifecyclePhase::POST_INITIALIZATION:
		// Local, opt-in: auto-launch configured startup commands once per session.
		FINLaunchStartupCommands(GetGameInstance());
		break;
	default: break;
	}
}
