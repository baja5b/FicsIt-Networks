#include "ModuleSystem/FINModuleSystemPanel.h"
#include "FactoryGameCustomVersion.h"
#include "FGDismantleInterface.h"
#include "FicsItNetworksMisc.h"
#include "FortniteReleaseBranchCustomObjectVersion.h"
#include "SaveCustomVersion.h"
#include "GameFramework/Actor.h"
#include "ModuleSystem/FINModuleSystemModule.h"
#include "Net/UnrealNetwork.h"

void UFINModuleSystemPanel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFINModuleSystemPanel, AllowedModules);
	DOREPLIFETIME(UFINModuleSystemPanel, Grid);
}

void UFINModuleSystemPanel::Serialize(FArchive& Ar) {
	bool bOldObj = Ar.IsSaveGame() && Ar.CustomVer(FSaveCustomVersion::GUID) < FSaveCustomVersion::ResetBrokenBlueprintSplines;
	int Ver = 0;
	if(bOldObj) {
		Ver = Ar.CustomVer(FFortniteReleaseBranchCustomObjectVersion::GUID);
		Ar.SetCustomVersion(FFortniteReleaseBranchCustomObjectVersion::GUID, 1, TEXT("FFortniteReleaseBranchCustomObjectVersion"));
	}

	// FIN-1.2-PORT: Satisfactory 1.0/1.1 (UE5.3) wrote the UCSModifiedProperties array
	// INLINE into every component blob on save (4 bytes for an empty array), because a
	// SAVING archive always carries the current FFortniteReleaseBranch version
	// (UActorComponent::Serialize: >= ActorComponentUCSModifiedPropertiesSparseStorage
	// -> read/write inline). When LOADING under 1.2 the save archive is missing the
	// engine custom versions (CustomVer == -1), so that block is NOT read -> everything
	// after Super::Serialize (PanelHeight/Width/grid) is shifted by 4 bytes. Symptom:
	// "NO CPU DETECTED" / invalid placement on all panels after save migration.
	// Fix (mirror image of the bOldObj hack above): pin the version to the writer's
	// level for those saves so the legacy inline block is consumed again.
	// Shipped save versions (verified empirically): 1.0=46, 1.1=52, first 1.2=60;
	// 53-59 never shipped, so any threshold in between separates the eras exactly.
	const bool bLegacyInlineUCS = Ar.IsSaveGame() && Ar.IsLoading() && !bOldObj
		&& Ar.CustomVer(FSaveCustomVersion::GUID) < FSaveCustomVersion::FixNewPlayerInfoHandleSerializationFormat
		&& Ar.CustomVer(FFortniteReleaseBranchCustomObjectVersion::GUID) < FFortniteReleaseBranchCustomObjectVersion::ActorComponentUCSModifiedPropertiesSparseStorage;
	int32 FortVerBackup = 0;
	if (bLegacyInlineUCS) {
		FortVerBackup = Ar.CustomVer(FFortniteReleaseBranchCustomObjectVersion::GUID);
		Ar.SetCustomVersion(FFortniteReleaseBranchCustomObjectVersion::GUID, FFortniteReleaseBranchCustomObjectVersion::ActorComponentUCSModifiedPropertiesSparseStorage, TEXT("FFortniteReleaseBranchCustomObjectVersion"));
	}

	Super::Serialize(Ar);
	
	if (Ar.IsSaveGame()) {
		if(bOldObj) {
			Ar.SetCustomVersion(FFortniteReleaseBranchCustomObjectVersion::GUID, Ver, TEXT("FFortniteReleaseBranchCustomObjectVersion"));
		}
		if (bLegacyInlineUCS) {
			Ar.SetCustomVersion(FFortniteReleaseBranchCustomObjectVersion::GUID, FortVerBackup, TEXT("FFortniteReleaseBranchCustomObjectVersion"));
		}
		
		int height = PanelHeight, width = PanelWidth;
		Ar << PanelHeight;
		Ar << PanelWidth;
		
		// FIN-1.2-PORT: Safety net against misaligned streams (whatever the cause):
		// do not accept implausible grid dimensions (would cause huge allocations or a
		// crash), reset the grid to defaults and skip the slot refs. The modules heal
		// the grid themselves (AFINModuleBase::BeginPlay re-registers via ModulePanel/ModulePos).
		if (Ar.IsLoading() && (PanelHeight < 0 || PanelHeight > 256 || PanelWidth < 0 || PanelWidth > 256)) {
			UE_LOG(LogFicsItNetworksMisc, Error,
				TEXT("FINModuleSystemPanel %s: implausible panel grid %dx%d in save data (misaligned stream / incompatible save?) - keeping defaults %dx%d, modules will re-register themselves."),
				*GetPathName(), PanelHeight, PanelWidth, height, width);
			PanelHeight = height;
			PanelWidth = width;
			SetupGrid();
			return;
		}
		
		SetupGrid();

		for (int x = 0; x < PanelHeight; ++x) {
			for (int y = 0; y < PanelWidth; ++y) {
				if (x < height && y < width) {
					UObject* ptr = GetGridSlot(x, y);
					Ar << ptr;
					GetGridSlot(x, y) = ptr;
				} else {
					UObject* ptr = nullptr;
					Ar << ptr;
				}
			}
		}
	}
}

void UFINModuleSystemPanel::InitializeComponent() {
	Super::InitializeComponent();
	SetIsReplicated(true);
}

void UFINModuleSystemPanel::EndPlay(const EEndPlayReason::Type reason) {
	Super::EndPlay(reason);
}

bool UFINModuleSystemPanel::ShouldSave_Implementation() const {
	return true;
}

void UFINModuleSystemPanel::GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) {
	
}

bool UFINModuleSystemPanel::AddModule(AActor* module, int x, int y, int rot) {
	SetupGrid();

	int w, h;
	IFINModuleSystemModule::Execute_getModuleSize(module, w, h);

	FVector min, max;
	GetModuleSpace({static_cast<float>(x), static_cast<float>(y), 0.0f}, rot, {static_cast<float>(w), static_cast<float>(h), 0.0f}, min, max);
	for (int MX = static_cast<int>(min.X); MX <= max.X; ++MX) for (int MY = static_cast<int>(min.Y); MY <= max.Y; ++MY) {
		GetGridSlot(MX, MY) = module;
	}

	GetOwner()->ForceNetUpdate();
	OnModuleChanged.Broadcast(module, true);
	
	return true;
}

bool UFINModuleSystemPanel::RemoveModule(AActor* Module) {
	bool removed = false;
	for (int x = 0; x < PanelHeight; ++x) for (int y = 0; y < PanelWidth; ++y) {
		auto& s = GetGridSlot(x, y);
		if (s == Module) {
			s = nullptr;
			removed = true;
		}
	}

	if (removed) {
		GetOwner()->ForceNetUpdate();
		OnModuleChanged.Broadcast(Module, false);
	}
	
	return removed;
}

AActor* UFINModuleSystemPanel::GetModule(int x, int y) const {
	if (Grid.Num() < 1) return nullptr;
	return  (x >= 0 && x < PanelHeight && y >= 0 && y < PanelWidth) ? Cast<AActor>(GetGridSlot(x, y)) : nullptr;;
}

void UFINModuleSystemPanel::GetModules(TArray<AActor*>& modules) const {
	if (Grid.Num() < 1) return;
	for (int x = 0; x < PanelHeight; ++x) {
		for (int y = 0; y < PanelWidth; ++y) {
			auto m = GetModule(x, y);
			if (m && !modules.Contains(m)) {
				modules.Add(m);
			}
		}
	}
}

void UFINModuleSystemPanel::GetDismantleRefund(TArray<FInventoryStack>& refund, bool noCost) const {
	if (Grid.Num() < 1) return;
	TSet<AActor*> modules;
	for (int x = 0; x < PanelHeight; ++x) for (int y = 0; y < PanelWidth; ++y) {
		AActor* m = GetModule(x, y);
		if (m && !modules.Contains(m)) {
			modules.Add(m);
			if (m->Implements<UFGDismantleInterface>()) {
				IFGDismantleInterface::Execute_GetDismantleRefund(m, refund, noCost);
			}
		}
	}
}

void UFINModuleSystemPanel::SetupGrid() {
	if (Grid.Num() < 1) {
		for (int i = 0; i < PanelHeight * PanelWidth; ++i) {
			Grid.Add(nullptr);
		}
	}
}

UObject* UFINModuleSystemPanel::GetGridSlot(int x, int y) const {
	return Grid[x*PanelWidth + y];
}

UObject*& UFINModuleSystemPanel::GetGridSlot(int x, int y) {
	return Grid[x*PanelWidth + y];
}

void UFINModuleSystemPanel::GetModuleSpace(const FVector& Loc, const int Rot, const FVector& MSize, FVector& OutMin, FVector& OutMax) {
	const FVector s = MSize - 1;
	switch (Rot) {
	case 0:
		OutMin = Loc;
		OutMax = Loc + s;
		break;
	case 1:
		OutMin = {Loc.X - s.Y, Loc.Y, 0.0f};
		OutMax = {Loc.X, Loc.Y + s.X, 0.0f};
		break;
	case 2:
		OutMin = Loc - s;
		OutMax = Loc;
		break;
	case 3:
		OutMin = {Loc.X, Loc.Y - s.X, 0.0f};
		OutMax = {Loc.X + s.Y, Loc.Y, 0.0f};
		break;
	default:
		break;
	}
}

/**
 * ONLY CALL IN INITIATION!
 * 
 * @param Width  
 * @param Height 
 */
void UFINModuleSystemPanel::SetPanelSize(int Width, int Height) {
	PanelWidth = Width;
	PanelHeight = Height;
}
