#include "Components/FINModuleBase.h"
#include "ModuleSystem/FINModuleSystemHolo.h"
#include "ModuleSystem/FINModuleSystemPanel.h"
#include "Net/UnrealNetwork.h"

void AFINModuleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFINModuleBase, ModuleName);
}

void AFINModuleBase::BeginPlay() {
	Super::BeginPlay();

	// FIN-1.2-PORT: Panel grid self-heal. Every module persists ModulePanel + ModulePos
	// itself (SaveGame). If the panel slot does not know the module after loading (grid
	// corrupted in the save, e.g. re-saved during the UE5.6 deserialization-bug era, or
	// reset by the sanity check in UFINModuleSystemPanel::Serialize), re-register -
	// restores the grid state losslessly from the modules' own data.
	if (HasAuthority() && IsValid(ModulePanel)) {
		const int32 X = static_cast<int32>(ModulePos.X);
		const int32 Y = static_cast<int32>(ModulePos.Y);
		const int32 Rot = static_cast<int32>(ModulePos.Z);
		if (X >= 0 && Y >= 0 && ModulePanel->GetModule(X, Y) != this) {
			ModulePanel->AddModule(this, X, Y, Rot);
		}
	}
}

void AFINModuleBase::EndPlay(EEndPlayReason::Type reason) {
	Super::EndPlay(reason);
	if (HasAuthority() && ModulePanel) ModulePanel->RemoveModule(this);
}

bool AFINModuleBase::ShouldSave_Implementation() const {
	return true;
}

void AFINModuleBase::setPanel_Implementation(UFINModuleSystemPanel* Panel, const int X, const int Y, const int Rot) {
	ModulePanel = Panel;
	ModulePos = FVector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Rot));
	if (IsValid(ModulePanel)) ModulePanel->AddModule(this, X, Y, Rot);
}

void AFINModuleBase::getModuleSize_Implementation(int& Width, int& Height) const {
	Width = static_cast<int>(ModuleSize.X);
	Height = static_cast<int>(ModuleSize.Y);
}

FName AFINModuleBase::getName_Implementation() const {
	return ModuleName;
}

UObject* AFINModuleBase::GetSignalSenderOverride_Implementation() {
	UObject* obj = Cast<UObject>(this);
	return obj;
}
