#include "Reflection/Source/FIRSourceStaticMacros.h"

#include "Buildables/FGBuildableSplitterSmart.h"
#include "Buildables/FGBuildableMergerPriority.h"
#include "Buildables/FGBuildableConveyorMonitor.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGWildCardDescriptor.h"
#include "Resources/FGOverflowDescriptor.h"
#include "Resources/FGAnyUndefinedDescriptor.h"
#include "Resources/FGNoneDescriptor.h"

// Exposes the configuration of vanilla conveyor attachments (Smart/Programmable
// Splitter, Priority Merger) to the FIN reflection system so a computer can
// read and change their sort rules / input priorities at runtime.
//
// AFGBuildableSplitterSmart backs BOTH the Smart Splitter and the Programmable
// Splitter; they only differ in their output count.

BeginClass(AFGBuildableSplitterSmart, "SmartSplitter", "Smart Splitter", "A splitter that routes items to outputs based on per-output filter rules. Also backs the Programmable Splitter.")
BeginProp(RInt, numSortRules, "Num Sort Rules", "The number of sort/filter rules currently configured on this splitter.") {
	FIRReturn (FIRInt)self->GetNumSortRules();
} EndProp()
BeginProp(RInt, maxSortRules, "Max Sort Rules", "The maximum number of sort rules this splitter supports.") {
	FIRReturn (FIRInt)self->GetMaxNumSortRules();
} EndProp()

BeginFunc(getSortRule, "Get Sort Rule", "Returns the filter rule at the given index as (item type, output index).") {
	InVal(0, RInt, index, "Index", "The index of the sort rule to read.")
	OutVal(1, RClass<UFGItemDescriptor>, itemType, "Item Type", "The item type the rule filters for (none-class = any/wildcard).")
	OutVal(2, RInt, output, "Output", "The output index this rule routes the item to.")
	Body()
	const FSplitterSortRule Rule = self->GetSortRuleAt((int32)index);
	itemType = (FIRAny)(UClass*)Rule.ItemClass;
	output = (FIRAny)(FIRInt)Rule.OutputIndex;
} EndFunc()

BeginFunc(addSortRule, "Add Sort Rule", "Appends a new filter rule routing the given item type to the given output index.", 0) {
	InVal(0, RClass<UFGItemDescriptor>, itemType, "Item Type", "The item type to filter for (none-class = any/wildcard).")
	InVal(1, RInt, output, "Output", "The output index to route the item to.")
	Body()
	self->AddSortRule(FSplitterSortRule(TSubclassOf<UFGItemDescriptor>(itemType), (int32)output));
} EndFunc()

BeginFunc(setSortRule, "Set Sort Rule", "Overwrites the filter rule at the given index.", 0) {
	InVal(0, RInt, index, "Index", "The index of the sort rule to overwrite.")
	InVal(1, RClass<UFGItemDescriptor>, itemType, "Item Type", "The item type to filter for (none-class = any/wildcard).")
	InVal(2, RInt, output, "Output", "The output index to route the item to.")
	Body()
	self->SetSortRuleAt((int32)index, FSplitterSortRule(TSubclassOf<UFGItemDescriptor>(itemType), (int32)output));
} EndFunc()

BeginFunc(removeSortRule, "Remove Sort Rule", "Removes the filter rule at the given index.", 0) {
	InVal(0, RInt, index, "Index", "The index of the sort rule to remove.")
	Body()
	self->RemoveSortRuleAt((int32)index);
} EndFunc()

BeginFunc(clearSortRules, "Clear Sort Rules", "Removes all filter rules from this splitter.", 0) {
	Body()
	self->SetSortRules(TArray<FSplitterSortRule>());
} EndFunc()

// The special filter "item types" (Any/Overflow/AnyUndefined) are abstract
// descriptor classes, not registered in the reflection class table, so a Lua
// script cannot reach them via classes[...]. Expose them as getters that
// compose with addSortRule/setSortRule, e.g. splitter:addSortRule(splitter:getOverflowFilter(), 2).
BeginFunc(getWildcardFilter, "Get Wildcard Filter", "Returns the 'Any' wildcard filter item type, for use as the item type in addSortRule/setSortRule.") {
	OutVal(0, RClass<UFGItemDescriptor>, filter, "Filter", "The wildcard (Any) filter descriptor class.")
	Body()
	filter = (FIRAny)(UClass*)UFGWildCardDescriptor::StaticClass();
} EndFunc()

BeginFunc(getOverflowFilter, "Get Overflow Filter", "Returns the 'Overflow' filter item type, for use as the item type in addSortRule/setSortRule.") {
	OutVal(0, RClass<UFGItemDescriptor>, filter, "Filter", "The overflow filter descriptor class.")
	Body()
	filter = (FIRAny)(UClass*)UFGOverflowDescriptor::StaticClass();
} EndFunc()

BeginFunc(getAnyUndefinedFilter, "Get Any-Undefined Filter", "Returns the 'Any Undefined' filter item type (items without a specific rule), for use as the item type in addSortRule/setSortRule.") {
	OutVal(0, RClass<UFGItemDescriptor>, filter, "Filter", "The any-undefined filter descriptor class.")
	Body()
	filter = (FIRAny)(UClass*)UFGAnyUndefinedDescriptor::StaticClass();
} EndFunc()

BeginFunc(getNoneFilter, "Get None Filter", "Returns the 'None' filter item type (route nothing to the output), for use as the item type in addSortRule/setSortRule.") {
	OutVal(0, RClass<UFGItemDescriptor>, filter, "Filter", "The none filter descriptor class.")
	Body()
	filter = (FIRAny)(UClass*)UFGNoneDescriptor::StaticClass();
} EndFunc()
EndClass()

BeginClass(AFGBuildableMergerPriority, "PriorityMerger", "Priority Merger", "A merger that pulls from its inputs based on a configurable priority per input.")
BeginFunc(getInputCount, "Get Input Count", "Returns the number of inputs this priority merger has.") {
	OutVal(0, RInt, count, "Count", "The number of input connections.")
	Body()
	count = (FIRAny)(FIRInt)self->GetInputPriorities().Num();
} EndFunc()

BeginFunc(getPriority, "Get Priority", "Returns the priority of the input at the given index.") {
	InVal(0, RInt, input, "Input", "The input index.")
	OutVal(1, RInt, priority, "Priority", "The priority of that input.")
	Body()
	priority = (FIRAny)(FIRInt)self->GetPriorityByInputIndex((int32)input);
} EndFunc()

BeginFunc(setPriority, "Set Priority", "Sets the priority of the input at the given index.", 0) {
	InVal(0, RInt, input, "Input", "The input index.")
	InVal(1, RInt, priority, "Priority", "The new priority for that input.")
	Body()
	self->SetPriorityByInputIndex((int32)input, (int32)priority);
} EndFunc()
EndClass()

BeginClass(AFGBuildableConveyorMonitor, "ConveyorThroughputMonitor", "Conveyor Throughput Monitor", "Measures the item throughput of the conveyor belt it is attached to.")
BeginProp(RInt, itemsPerMinute, "Items Per Minute", "The measured average throughput in items per minute. -1 if not enough data yet.") {
	FIRReturn (FIRInt)self->GetCalculatedAverage();
} EndProp()
BeginProp(RFloat, confidence, "Confidence", "How reliable the current measurement is, from 0 (none) to 1 (full).") {
	// GetConfidence() is on a 0..100 scale in-game; normalize to the documented 0..1 ratio.
	FIRReturn self->GetConfidence() / 100.0f;
} EndProp()
EndClass()
