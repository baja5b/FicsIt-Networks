#include "Components/FINModuleBase.h"
#include "ModuleSystem/FINModuleSystemHolo.h"
#include "ModuleSystem/FINModuleSystemPanel.h"
#include "Components/WidgetComponent.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Styling/CoreStyle.h"
#include "Net/UnrealNetwork.h"

// Feste Pixel-Aufloesung der Widget-Plane; effektive Welt-Groesse via RelativeScale (UpdateLabelTextRender).
static constexpr float FIN_LABEL_DRAW_PX = 256.0f;

AFINModuleBase::AFINModuleBase() {
	// Flaches, UNLIT Slate-Text-Widget (wie AFINModuleScreen). Die UWidgetComponent
	// im Konstruktor erzeugen + an Root haengen; das eigentliche Slate-Widget erst
	// in BeginPlay bauen (Slate-Init zur CDO-Zeit vermeiden).
	LabelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LabelWidget"));
	LabelWidget->SetupAttachment(RootComponent);
	LabelWidget->SetWidgetSpace(EWidgetSpace::World);
	LabelWidget->SetTwoSided(true);
	LabelWidget->SetBackgroundColor(FLinearColor::Transparent);
	LabelWidget->SetDrawSize(FVector2D(FIN_LABEL_DRAW_PX, FIN_LABEL_DRAW_PX));
	// KRITISCH: keine Kollision, sonst faengt die Widget-Plane den Klick-Strahl ab
	// und Buttons/Encoder reagieren nicht mehr.
	LabelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// KRITISCH: Widget von Anfang an KLEIN skalieren. Die Default-Plane (256 Einheiten)
	// bei Scale 1 blaeht sonst die Modul-Bounding-Box auf -> beim Platzieren eines
	// Blueprints mit Fahrzeug-Pfad crasht das Path-Segment in der Octree-Abfrage
	// (tiefe Rekursion auf riesiger Box). Bisect 2026-06-24: Widget = Ursache.
	// (NICHT bUseAttachParentBound nehmen -> das cullt das Widget komplett weg.)
	LabelWidget->SetRelativeScale3D(FVector(0.05f));  // klein; BeginPlay setzt dann textSize/256
	LabelWidget->SetVisibility(false); // leer = unsichtbar
}

void AFINModuleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFINModuleBase, ModuleName);
	DOREPLIFETIME(AFINModuleBase, LabelText);
	DOREPLIFETIME(AFINModuleBase, LabelTextSize);
	DOREPLIFETIME(AFINModuleBase, LabelTextColor);
	DOREPLIFETIME(AFINModuleBase, LabelTextOffset);
	DOREPLIFETIME(AFINModuleBase, LabelTextRotation);
}

void AFINModuleBase::BeginPlay() {
	Super::BeginPlay();
	// Slate-Text-Widget einmalig bauen: STextBlock in SScaleBox (proportionale
	// Skalierung) in SBox (feste Box), zentriert. Unlit -> auf hell/dunkel + jedem Licht lesbar.
	if (!LabelTextBlock.IsValid() && LabelWidget) {
		LabelTextBlock = SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 48))
			.ColorAndOpacity(FSlateColor(LabelTextColor))
			.Text(FText::GetEmpty());
		LabelWidget->SetSlateWidget(
			SNew(SBox).WidthOverride(FIN_LABEL_DRAW_PX).HeightOverride(FIN_LABEL_DRAW_PX)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[ LabelTextBlock.ToSharedRef() ]
			]);
	}
	UpdateLabelTextRender(); // nach Save-Load: Anzeige aus persistierten Properties herstellen
}

void AFINModuleBase::UpdateLabelTextRender() {
	if (!LabelWidget) return;
	LabelWidget->SetRelativeLocation(LabelTextOffset);
	LabelWidget->SetRelativeRotation(LabelTextRotation);
	// LabelTextSize (cm) -> Plane-Scale (DrawSize-Pixel sind die Basis).
	const float Scale = FMath::Max(LabelTextSize, 0.01f) / FIN_LABEL_DRAW_PX;
	LabelWidget->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	if (LabelTextBlock.IsValid()) {
		LabelTextBlock->SetText(FText::FromString(LabelText));
		LabelTextBlock->SetColorAndOpacity(FSlateColor(LabelTextColor));
	}
	LabelWidget->SetVisibility(!LabelText.IsEmpty());
}

void AFINModuleBase::OnRep_LabelText() {
	UpdateLabelTextRender();
}

void AFINModuleBase::SetLabelText(const FString& InText) {
	LabelText = InText;
	UpdateLabelTextRender();
	ForceNetUpdate();
}

void AFINModuleBase::SetLabelTextSize(const float InSize) {
	LabelTextSize = InSize;
	UpdateLabelTextRender();
	ForceNetUpdate();
}

void AFINModuleBase::SetLabelTextColor(const FLinearColor& InColor) {
	LabelTextColor = InColor;
	UpdateLabelTextRender();
	ForceNetUpdate();
}

void AFINModuleBase::SetTextOffset(const FVector& InOffset) {
	LabelTextOffset = InOffset;
	UpdateLabelTextRender();
	ForceNetUpdate();
}

void AFINModuleBase::SetTextRotation(const FRotator& InRot) {
	LabelTextRotation = InRot;
	UpdateLabelTextRender();
	ForceNetUpdate();
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
