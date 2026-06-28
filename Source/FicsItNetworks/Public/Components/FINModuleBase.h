#pragma once

#include "Buildables/FGBuildable.h"
#include "ModuleSystem/FINModuleSystemModule.h"
#include "Signals/FINSignalSender.h"
#include "FINModuleBase.generated.h"

class UWidgetComponent;
class STextBlock;

UCLASS()
class AFINModuleBase : public AFGBuildable, public IFINModuleSystemModule, public IFINSignalSender {
	GENERATED_BODY()
public:
	AFINModuleBase();

    UPROPERTY(EditDefaultsOnly)
    FVector2D ModuleSize;

	UPROPERTY(EditDefaultsOnly, Replicated)
    FName ModuleName;

	UPROPERTY(BlueprintReadOnly, SaveGame)
    FVector ModulePos;

	UPROPERTY(BlueprintReadOnly, SaveGame)
    UFINModuleSystemPanel* ModulePanel = nullptr;

	// --- Frei beschriftbarer Label-Text (rein C++, FIN-reflektiert). Leer = unsichtbar,
	//     daher bleiben Buttons/Potis/etc. ohne gesetzten Text unberuehrt. ---
	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LabelText)
	FString LabelText;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LabelText)
	float LabelTextSize = 10.0f;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LabelText)
	FLinearColor LabelTextColor = FLinearColor::White;

	// Live-justierbare Transform des Text-Renders (Default: 2cm vor der Front, Yaw 180 = zum Spieler).
	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LabelText)
	FVector LabelTextOffset = FVector(2.0f, 0.0f, 0.0f);

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LabelText)
	FRotator LabelTextRotation = FRotator(0.0f, 180.0f, 0.0f);

	// Flaches, unlit Slate-Text-Widget auf der Modul-Front (ersetzt die alte
	// UTextRenderComponent). Gleiches Muster wie AFINModuleScreen.
	UPROPERTY()
	UWidgetComponent* LabelWidget = nullptr;

	// Cache des Slate-STextBlock, um Text/Farbe live zu aendern (kein UPROPERTY).
	TSharedPtr<STextBlock> LabelTextBlock;

	void SetLabelText(const FString& InText);
	void SetLabelTextSize(float InSize);
	void SetLabelTextColor(const FLinearColor& InColor);
	void SetTextOffset(const FVector& InOffset);
	void SetTextRotation(const FRotator& InRot);
	void UpdateLabelTextRender();

	UFUNCTION()
	void OnRep_LabelText();

	// Begin AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type reason) override;
	// End AActor

	// Begin IFGSaveInterface
	virtual bool ShouldSave_Implementation() const override;
	// End IFGSaveInterface

	// Begin IFINModuleSystemModule
	virtual void setPanel_Implementation(UFINModuleSystemPanel* Panel, int X, int Y, int Rot) override;
	virtual void getModuleSize_Implementation(int& Width, int& Height) const override;
	virtual FName getName_Implementation() const override;
	// End IFINModuleSystemModule

	// Begin IFINSignalSender
	virtual UObject* GetSignalSenderOverride_Implementation() override;
	// End IFINSignalSender
};
