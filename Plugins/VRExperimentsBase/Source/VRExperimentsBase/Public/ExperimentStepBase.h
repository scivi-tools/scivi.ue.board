// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExperimentStepBase.generated.h"
/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(LogExperiment, Log, All);

UCLASS()
class VREXPERIMENTSBASE_API AExperimentStepBase : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	FName name;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
