// Fill out your copyright notice in the Description page of Project Settings.


#include "ExperimentStepBase.h"

DEFINE_LOG_CATEGORY(LogExperiment);

void AExperimentStepBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogExperiment, Display, TEXT("Experiment step %s started"), *name.ToString());
}

void AExperimentStepBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	UE_LOG(LogExperiment, Display, TEXT("Experiment step %s was terminated"), *name.ToString());
}
