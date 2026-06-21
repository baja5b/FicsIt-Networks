#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "FINConfigurationStruct.generated.h"

struct FFINConfigurationStruct_LogViewer;
struct FFINConfigurationStruct_Blueprints;
struct FFINConfigurationStruct_Startup;

USTRUCT(BlueprintType)
struct FFINConfigurationStruct_LogViewer {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool TextLog{};

    UPROPERTY(BlueprintReadWrite)
    bool TextLogTimestamp{};

    UPROPERTY(BlueprintReadWrite)
    bool TextLogVerbosity{};

    UPROPERTY(BlueprintReadWrite)
    bool TextLogMultilineAlign{};
};

USTRUCT(BlueprintType)
struct FFINConfigurationStruct_Blueprints {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool EnableParametricBlueprints{};
};

USTRUCT(BlueprintType)
struct FFINConfigurationStruct_Startup {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool bEnableStartupCommands{};

    UPROPERTY(BlueprintReadWrite)
    TArray<FString> StartupCommands;
};

/* Struct generated from Mod Configuration Asset '/FicsItNetworks/FINConfiguration' */
USTRUCT(BlueprintType)
struct FFINConfigurationStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    FFINConfigurationStruct_LogViewer LogViewer{};

    UPROPERTY(BlueprintReadWrite)
    FFINConfigurationStruct_Blueprints Blueprints{};

    UPROPERTY(BlueprintReadWrite)
    FFINConfigurationStruct_Startup Startup{};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FFINConfigurationStruct GetActiveConfig(UObject* WorldContext) {
        FFINConfigurationStruct ConfigStruct{};
        FConfigId ConfigId{"FicsItNetworks", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FFINConfigurationStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

