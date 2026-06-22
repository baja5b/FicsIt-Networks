#include "Reflection/Source/FIRSourceStaticMacros.h"

#include "FGInventoryComponent.h"
#include "FGVehicle.h"
#include "WheeledVehicles/FGWheeledVehicle.h"
#include "WheeledVehicles/FGWheeledVehicleIdentifier.h"
#include "WheeledVehicles/FGVehiclePathSegment.h"
#include "WheeledVehicles/FGVehiclePathNode.h"
#include "WheeledVehicles/FGVehicleSubsystem.h"
#include "WheeledVehicles/FGDockingStationIdentifier.h"
#include "Buildables/FGBuildableDockingStation.h"
#include "Components/SplineComponent.h"

// FIN-1.2-PORT: Re-enabled the parts of the old vehicle/station reflection that
// survive in Satisfactory 1.2. The self-driving waypoint system (AFGDrivingTargetList,
// autopilot toggle, GetSimulationMovement, health/isSelfDriving on AFGVehicle) was
// REMOVED in 1.2 (replaced by the buildable path-segment network) and is intentionally
// dropped. The original is preserved as FIRSourceStatic_Vehicle.cpp.disabled.
// Vehicle ROUTING against the new 1.2 path API is a separate, future feature.

BeginClass(AFGVehicle, "Vehicle", "Vehicle", "The base class for all vehicles.")
EndClass()

BeginClass(AFGWheeledVehicle, "WheeledVehicle", "Wheeled Vehicle", "The base class for all wheeled vehicles (trucks/tractors/explorers).")
BeginFunc(getFuelInv, "Get Fuel Inventory", "Returns the inventory that contains the fuel of the vehicle.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The fuel inventory of the vehicle.")
	Body()
	inventory = Ctx.GetTrace() / self->GetFuelInventory();
} EndFunc()
BeginFunc(getStorageInv, "Get Storage Inventory", "Returns the inventory that contains the storage/cargo of the vehicle (solid items and fluids/gases).") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The storage inventory of the vehicle.")
	Body()
	inventory = Ctx.GetTrace() / self->GetStorageInventory();
} EndFunc()
BeginProp(RFloat, speed, "Speed", "The current forward speed of this vehicle (unreal units/s).") {
	FIRReturn self->GetForwardSpeed();
} EndProp()
BeginProp(RInt, speedKMH, "Speed KMH", "The estimated speed of the vehicle in kilometers per hour.") {
	FIRReturn (FIRInt) self->GetSpeedInKMH();
} EndProp()
BeginProp(RBool, hasFuel, "Has Fuel", "True if the vehicle currently has fuel to drive.") {
	FIRReturn self->HasFuel();
} EndProp()
// Phase 2 (read-only): which segment of the shared road network the truck is on.
// 1.2 has no public "drive to destination" verb, so routing is observe-only.
BeginFunc(getCurrentPathSegment, "Get Current Path Segment", "Returns the vehicle path segment the truck is currently traversing, or nil if not on a path.") {
	OutVal(0, RTrace<AFGVehiclePathSegment>, segment, "Segment", "The current path segment, or nil.")
	Body()
	segment = Ctx.GetTrace() / self->GetCurrentVehiclePathSegment();
} EndFunc()
BeginFunc(getIdentifier, "Get Identifier", "Returns the persistent identifier of this vehicle, which holds its name, route and autopilot state (the train-timetable analog).") {
	OutVal(0, RTrace<AFGWheeledVehicleIdentifier>, identifier, "Identifier", "The vehicle identifier, or nil.")
	Body()
	identifier = Ctx.GetTrace() / self->GetVehicleIdentifier();
} EndFunc()
EndClass()

// The truck's "timetable": its route is a list of waypoint GUIDs (path nodes). This
// is the per-vehicle, train-like control surface CSS added in 1.2 - set the route,
// add/remove stops, toggle autopilot. GUIDs are exchanged as strings.
BeginClass(AFGWheeledVehicleIdentifier, "WheeledVehicleIdentifier", "Wheeled Vehicle Identifier", "Persistent info of a wheeled vehicle: name, route (waypoint GUIDs) and autopilot control.")
BeginProp(RString, name, "Name", "The display name of the vehicle.", 0) {
	FIRReturn (FIRStr) self->GetVehicleName().ToString();
} PropSet() {
	self->SetVehicleName(FText::FromString(Val));
} EndProp()
BeginProp(RBool, isAutopilotEnabled, "Is Autopilot Enabled", "True if the vehicle's autopilot is enabled (drives its route automatically).", 0) {
	FIRReturn self->IsAutopilotEnabled();
} PropSet() {
	self->SetAutopilotEnabled(Val);
} EndProp()
BeginProp(RBool, canEnableAutopilot, "Can Enable Autopilot", "True if the autopilot can currently be enabled (valid route, on path, enough stations).") {
	FIRReturn self->CanEnableAutopilot();
} EndProp()
BeginProp(RInt, autopilotError, "Autopilot Error", "The current autopilot error status: 0=None 1=StationUnreachable 2=NotOnPath 3=TooFewStations (see EVehicleAutopilotErrorStatus).") {
	FIRReturn (FIRInt) self->GetAutopilotErrorStatus();
} EndProp()
BeginProp(RInt, currentWaypoint, "Current Waypoint", "The index of the waypoint in the route the vehicle is currently heading to.") {
	FIRReturn (FIRInt) self->GetCurrentTargetWaypointIndex();
} EndProp()
BeginProp(RString, currentFromNode, "Current From Node", "GUID string of the path node the vehicle is currently driving FROM (its current segment). Log this over time to capture the path actually driven and compare it to a findPathTo prediction.") {
	FIRReturn (FIRStr) self->GetCurrentFromPathNodeGUID().ToString();
} EndProp()
BeginProp(RString, currentToNode, "Current To Node", "GUID string of the path node the vehicle is currently driving TO (its current segment).") {
	FIRReturn (FIRStr) self->GetCurrentToPathNodeGUID().ToString();
} EndProp()
BeginFunc(getOwnerVehicle, "Get Owner Vehicle", "Returns the wheeled vehicle this identifier belongs to.") {
	OutVal(0, RTrace<AFGWheeledVehicle>, vehicle, "Vehicle", "The owning vehicle.")
	Body()
	vehicle = Ctx.GetTrace() / self->GetOwnerVehicle();
} EndFunc()
BeginFunc(getRoute, "Get Route", "Returns the vehicle's route as a list of waypoint GUID strings.") {
	OutVal(0, RArray<RString>, route, "Route", "List of waypoint GUIDs (strings).")
	Body()
	TArray<FIRAny> out;
	for (const FGuid& g : self->GetVehicleRoute()) out.Add((FIRStr) g.ToString());
	route = out;
} EndFunc()
BeginFunc(setRoute, "Set Route", "Replaces the vehicle's route with the given list of waypoint GUID strings.", 0) {
	InVal(0, RArray<RString>, route, "Route", "List of waypoint GUIDs (strings).")
	Body()
	TArray<FGuid> guids;
	for (const FIRAny& a : route) { FGuid g; if (FGuid::Parse(a.GetString(), g)) guids.Add(g); }
	self->SetVehicleRoute(guids);
} EndFunc()
BeginFunc(findPathTo, "Find Path To", "Computes a valid road path (list of path-node GUID strings) from this vehicle's current position to the given target node GUID, respecting THIS vehicle's type. Empty if the target is not reachable for this vehicle (e.g. it sits on another vehicle type's path). Feed the result to setRoute to drive there without jumping across disconnected sections.", 0) {
	InVal(0, RString, targetGuid, "Target GUID", "The path-node GUID to drive to (e.g. a station's getPathNode().guid).")
	OutVal(1, RArray<RString>, path, "Path", "Path-node GUIDs forming a drivable route, or empty if unreachable for this vehicle.")
	OutVal(2, RFloat, length, "Length", "Total length of the path in centimeters (0 if unreachable). This is the same path the autopilot drives, so it's the real driving distance.")
	Body()
	TArray<FIRAny> out;
	float total = 0.0f;
	AFGWheeledVehicle* veh = self->GetOwnerVehicle();
	FGuid toG;
	if (IsValid(veh) && FGuid::Parse(targetGuid, toG)) {
		AFGVehicleSubsystem* sub = AFGVehicleSubsystem::Get(veh->GetWorld());
		AFGVehiclePathSegment* seg = veh->GetCurrentVehiclePathSegment();
		AFGVehiclePathNode* fromNode = IsValid(seg) ? seg->GetStartNode() : nullptr;
		if (sub && IsValid(fromNode)) {
			FGuid fromG = fromNode->GetPathNodeGUID();
			UFGVehiclePathNetwork* net = sub->FindNetworkByPathNodeGuid(fromG);
			if (net) {
				TArray<FGuid> p;
				if (net->FindVehiclePath(fromG, toG, veh->GetVehiclePathPreset(), p)) {
					for (const FGuid& g : p) out.Add((FIRStr) g.ToString());
					// Gesamtlaenge: Spline-Laengen der Segmente entlang des Pfads aufsummieren.
					if (p.Num() >= 2) {
						TMap<FGuid, AFGVehiclePathNode*> nodeMap;
						for (AFGVehiclePathNode* n : net->GetNetworkElementsOnServer()) if (IsValid(n)) nodeMap.Add(n->GetPathNodeGUID(), n);
						for (int32 i = 0; i + 1 < p.Num(); i++) {
							AFGVehiclePathNode** fromN = nodeMap.Find(p[i]);
							AFGVehiclePathNode** toN = nodeMap.Find(p[i + 1]);
							if (fromN && toN && IsValid(*fromN) && IsValid(*toN)) {
								for (AFGVehiclePathSegment* s : (*fromN)->GetLeavingConnections()) {
									if (IsValid(s) && s->GetEndNode() == *toN) {
										if (USplineComponent* spl = s->GetSplineComponent()) total += spl->GetSplineLength();
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	path = out;
	length = (FIRFloat) total;
} EndFunc()
BeginFunc(addWaypoint, "Add Waypoint", "Appends a waypoint (GUID string of a path node) to the route.", 0) {
	InVal(0, RString, guid, "GUID", "The waypoint GUID string to append.")
	Body()
	FGuid g; if (!FGuid::Parse(guid, g)) throw FFIRException(TEXT("invalid GUID string"));
	self->AddWaypoint(g);
} EndFunc()
BeginFunc(insertWaypoint, "Insert Waypoint", "Inserts a waypoint (GUID string) at the given index in the route.", 0) {
	InVal(0, RInt, index, "Index", "The index to insert at.")
	InVal(1, RString, guid, "GUID", "The waypoint GUID string to insert.")
	Body()
	FGuid g; if (!FGuid::Parse(guid, g)) throw FFIRException(TEXT("invalid GUID string"));
	self->InsertWaypoint((int32)index, g);
} EndFunc()
BeginFunc(removeWaypoint, "Remove Waypoint", "Removes the waypoint at the given index from the route.", 0) {
	InVal(0, RInt, index, "Index", "The index of the waypoint to remove.")
	Body()
	self->RemoveWaypointAtIndex((int32)index);
} EndFunc()
EndClass()

BeginClass(AFGVehiclePathSegment, "VehiclePathSegment", "Vehicle Path Segment", "A single segment of the shared vehicle road network, connecting two path nodes.")
BeginFunc(getStartNode, "Get Start Node", "Returns the path node at the entry of this segment.") {
	OutVal(0, RTrace<AFGVehiclePathNode>, node, "Node", "The start node.")
	Body()
	node = Ctx.GetTrace() / self->GetStartNode();
} EndFunc()
BeginFunc(getEndNode, "Get End Node", "Returns the path node at the exit of this segment.") {
	OutVal(0, RTrace<AFGVehiclePathNode>, node, "Node", "The end node.")
	Body()
	node = Ctx.GetTrace() / self->GetEndNode();
} EndFunc()
BeginFunc(getVehicles, "Get Vehicles", "Returns the wheeled vehicles currently traversing this segment.") {
	OutVal(0, RArray<RTrace<AFGWheeledVehicle>>, vehicles, "Vehicles", "The vehicles on this segment.")
	Body()
	TArray<FIRAny> out;
	for (AFGWheeledVehicle* v : self->GetVehicles()) out.Add(Ctx.GetTrace() / v);
	vehicles = out;
} EndFunc()
EndClass()

BeginClass(AFGVehiclePathNode, "VehiclePathNode", "Vehicle Path Node", "A junction/endpoint node in the shared vehicle road network.")
BeginFunc(getArrivingConnections, "Get Arriving Connections", "Returns the segments that arrive at this node.") {
	OutVal(0, RArray<RTrace<AFGVehiclePathSegment>>, segments, "Segments", "Arriving segments.")
	Body()
	TArray<FIRAny> out;
	for (AFGVehiclePathSegment* s : self->GetArrivingConnections()) out.Add(Ctx.GetTrace() / s);
	segments = out;
} EndFunc()
BeginFunc(getLeavingConnections, "Get Leaving Connections", "Returns the segments that leave from this node.") {
	OutVal(0, RArray<RTrace<AFGVehiclePathSegment>>, segments, "Segments", "Leaving segments.")
	Body()
	TArray<FIRAny> out;
	for (AFGVehiclePathSegment* s : self->GetLeavingConnections()) out.Add(Ctx.GetTrace() / s);
	segments = out;
} EndFunc()
BeginProp(RBool, isTrivial, "Is Trivial", "True if this node is not a junction (at most one arriving and one leaving segment).") {
	FIRReturn self->IsTrivialPathNode();
} EndProp()
BeginProp(RString, guid, "GUID", "The unique GUID string of this path node - use it as a waypoint in a vehicle route.") {
	FIRReturn (FIRStr) self->GetPathNodeGUID().ToString();
} EndProp()
EndClass()

BeginClass(AFGBuildableDockingStation, "DockingStation", "Docking Station", "A docking station for wheeled vehicles (trucks/tractors) to transfer cargo and fuel. Also covers fluid truck stations - fluids/gases are items in the cargo inventory.")
BeginFunc(getFuelInv, "Get Fuel Inventory", "Returns the fuel inventory of the docking station.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The fuel inventory of the docking station.")
	Body()
	inventory = Ctx.GetTrace() / self->GetFuelInventory();
} EndFunc()
BeginFunc(getInv, "Get Inventory", "Returns the cargo inventory of the docking station. For fluid truck stations this holds the liquid/gas items.") {
	OutVal(0, RTrace<UFGInventoryComponent>, inventory, "Inventory", "The cargo inventory of this docking station.")
	Body()
	inventory = Ctx.GetTrace() / self->GetInventory();
} EndFunc()
BeginFunc(getDocked, "Get Docked", "Returns the currently docked actor (only set while a vehicle is actively loading/unloading), or nil.") {
	OutVal(0, RTrace<AActor>, docked, "Docked", "The currently docked actor.")
	Body()
	docked = Ctx.GetTrace() / self->GetDockedActor();
} EndFunc()
BeginFunc(getPathNode, "Get Path Node", "Returns the vehicle path node this station sits on - the entry point into the road network. Use it to find vehicles via the connected segments even when not actively docked.") {
	OutVal(0, RTrace<AFGVehiclePathNode>, node, "Node", "The station's docking path node, or nil.")
	Body()
	node = Ctx.GetTrace() / self->GetDockingPathNode();
} EndFunc()
BeginFunc(getNetworkStations, "Get Network Stations", "Returns ALL docking stations on the same road network as this one - even ones NOT wired to the FicsIt-Network. Like a train's track graph: you only need ONE station wired to discover all the rest (trucks/tractors/explorers + fluid).") {
	OutVal(0, RArray<RTrace<AFGBuildableDockingStation>>, stations, "Stations", "All docking stations in this station's vehicle path network.")
	Body()
	TArray<FIRAny> out;
	AFGVehicleSubsystem* sub = AFGVehicleSubsystem::Get(self->GetWorld());
	AFGVehiclePathNode* node = self->GetDockingPathNode();
	if (sub && IsValid(node)) {
		UFGVehiclePathNetwork* net = sub->FindNetworkByPathNodeGuid(node->GetPathNodeGUID());
		if (net) {
			TArray<AFGDockingStationIdentifier*> ids;
			net->PopulateNetworkStations(ids);
			for (AFGDockingStationIdentifier* id : ids) {
				if (IsValid(id) && IsValid(id->GetStation())) out.Add(Ctx.GetTrace() / id->GetStation());
			}
		}
	}
	stations = out;
} EndFunc()
BeginFunc(getNetworkVehicles, "Get Network Vehicles", "Returns ALL wheeled vehicles on the same road network as this station, wherever they are. Use to find/route trucks/tractors/explorers without a manual graph walk.") {
	OutVal(0, RArray<RTrace<AFGWheeledVehicle>>, vehicles, "Vehicles", "All wheeled vehicles in this station's vehicle path network.")
	Body()
	TArray<FIRAny> out;
	AFGVehicleSubsystem* sub = AFGVehicleSubsystem::Get(self->GetWorld());
	AFGVehiclePathNode* node = self->GetDockingPathNode();
	if (sub && IsValid(node)) {
		UFGVehiclePathNetwork* net = sub->FindNetworkByPathNodeGuid(node->GetPathNodeGUID());
		if (net) {
			TArray<AFGWheeledVehicleIdentifier*> ids;
			net->PopulateNetworkVehicles(ids);
			for (AFGWheeledVehicleIdentifier* id : ids) {
				if (IsValid(id) && IsValid(id->GetOwnerVehicle())) out.Add(Ctx.GetTrace() / id->GetOwnerVehicle());
			}
		}
	}
	vehicles = out;
} EndFunc()
BeginFunc(undock, "Undock", "Forcibly undocks the currently docked vehicle from this docking station.", 0) {
	Body()
	self->ForceUndockActor();
} EndFunc()
BeginProp(RString, name, "Name", "The name of this docking station (as shown in the vehicle station list / on the map). Settable.", 0) {
	AFGDockingStationIdentifier* id = self->GetStationIdentifier();
	FIRReturn (FIRStr)(id ? id->GetStationName().ToString() : FString());
} PropSet() {
	AFGDockingStationIdentifier* id = self->GetStationIdentifier();
	if (id) id->SetStationName(FText::FromString(Val));
} EndProp()
BeginProp(RBool, isLoadMode, "Is Load Mode", "True if the docking station loads docked vehicles, false if it unloads them.", 0) {
	FIRReturn self->GetIsInLoadMode();
} PropSet() {
	self->SetIsInLoadMode(Val);
} EndProp()
BeginProp(RBool, isLoadUnloading, "Is Load Unloading", "True if the docking station is currently loading or unloading a docked vehicle.") {
	FIRReturn self->IsLoadUnloading();
} EndProp()
BeginProp(RBool, isForceFuelType, "Is Force Fuel Type", "True if the docking station forcefully swaps the docked vehicle's fuel type.", 0) {
	FIRReturn self->GetIsForceVehicleFuelType();
} PropSet() {
	self->SetForceVehicleFuelType(Val);
} EndProp()
EndClass()
