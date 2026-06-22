#include "Reflection/Source/FIRSourceStaticMacros.h"

#include "Buildables/FGBuildableDroneStation.h"
#include "FGDroneStationInfo.h"
#include "FGDroneVehicle.h"
#include "Resources/FGItemDescriptor.h"

// FIN-1.2-PORT: New reflection for drone stations (FIN never exposed drones).
// Drones carry both solid items and fluids/gases via the station's inventory
// components, so the inventory getters cover fluid drones automatically.

BeginClass(AFGBuildableDroneStation, "DroneStation", "Drone Station", "A drone port that sends and receives drones carrying items and fluids between paired stations.")
BeginProp(RInt, status, "Status", "The current drone status: 0=NoDrone 1=Docked 2=Loading 3=Takeoff 4=EnRoute 5=Docking 6=Unloading 7=NotEnoughFuel 8=CannotUnload.") {
	if (!self->GetInfo()) throw FFIRException(TEXT("Drone station has no info object"));
	FIRReturn (FIRInt) self->GetInfo()->GetDroneStatus();
} EndProp()
BeginProp(RString, name, "Name", "The name/label of this drone station (shown on the map). Settable.", 0) {
	FIRReturn (FIRStr) self->GetActorRepresentationText().ToString();
} PropSet() {
	self->SetActorRepresentationText(FText::FromString(Val));
} EndProp()
BeginFunc(getInputInventory, "Get Input Inventory", "Returns the input inventory (where the docked drone unloads into). Holds solid items and fluids/gases.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The input inventory of the drone station.")
	Body()
	inventory = Ctx.GetTrace() / self->GetInputInventory();
} EndFunc()
BeginFunc(getOutputInventory, "Get Output Inventory", "Returns the output inventory (where the docked drone loads from). Holds solid items and fluids/gases.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The output inventory of the drone station.")
	Body()
	inventory = Ctx.GetTrace() / self->GetOutputInventory();
} EndFunc()
BeginFunc(getFuelInventory, "Get Fuel Inventory", "Returns the fuel inventory of the drone station.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The fuel inventory of the drone station.")
	Body()
	inventory = Ctx.GetTrace() / self->GetFuelInventory();
} EndFunc()
BeginFunc(getActiveFuelType, "Get Active Fuel Type", "Returns the item type currently used as fuel by this station's drone, or nil.") {
	OutVal(0, RClass<UFGItemDescriptor>, fuelType, "Fuel Type", "The active fuel item descriptor.")
	Body()
	if (!self->GetInfo()) throw FFIRException(TEXT("Drone station has no info object"));
	fuelType = (FIRAny)(UClass*)self->GetInfo()->GetDroneActiveFuelType();
} EndFunc()
BeginFunc(getPairedStation, "Get Paired Station", "Returns the drone station this station is paired with, or nil if unpaired.") {
	OutVal(0, RTrace<AFGBuildableDroneStation>, paired, "Paired Station", "The paired drone station buildable.")
	Body()
	if (!self->GetInfo()) throw FFIRException(TEXT("Drone station has no info object"));
	AFGDroneStationInfo* pairedInfo = self->GetInfo()->GetPairedStation();
	paired = Ctx.GetTrace() / (pairedInfo ? pairedInfo->GetStation() : nullptr);
} EndFunc()
BeginFunc(getInfo, "Get Info", "Returns the always-replicated info object of this drone port (used for pairing).") {
	OutVal(0, RTrace<AFGDroneStationInfo>, info, "Info", "The station info object, or nil.")
	Body()
	info = Ctx.GetTrace() / self->GetInfo();
} EndFunc()
EndClass()

// Phase 2: the drone-station info object exposes the one piece of real drone CONTROL
// that survives in 1.2 - station pairing (PairStation is public BlueprintCallable).
BeginClass(AFGDroneStationInfo, "DroneStationInfo", "Drone Station Info", "The always-replicated info object of a drone port. Exposes pairing + drone status.")
BeginProp(RInt, droneStatus, "Drone Status", "Current drone status: 0=NoDrone 1=Docked 2=Loading 3=Takeoff 4=EnRoute 5=Docking 6=Unloading 7=NotEnoughFuel 8=CannotUnload.") {
	FIRReturn (FIRInt) self->GetDroneStatus();
} EndProp()
BeginFunc(getPairedStation, "Get Paired Station", "Returns the info object this station is paired with, or nil.") {
	OutVal(0, RTrace<AFGDroneStationInfo>, station, "Station", "The paired station info, or nil.")
	Body()
	station = Ctx.GetTrace() / self->GetPairedStation();
} EndFunc()
BeginFunc(pairStation, "Pair Station", "Pairs this drone port with another drone port. Mutates game state (server only).", 0) {
	InVal(0, RObject<AFGDroneStationInfo>, otherStation, "Other Station", "The station info to pair with (get it via DroneStation:getInfo()).")
	Body()
	AFGDroneStationInfo* other = otherStation.Get();
	if (!IsValid(other)) throw FFIRException(TEXT("Other station is invalid"));
	self->PairStation(other);
} EndFunc()
EndClass()
