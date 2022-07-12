// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableActor.h"

void AInteractableActor::IsBeingFocused(const FGaze& gaze, const FHitResult& hitResult)
{
	IsBeingFocused_BP(gaze, hitResult);
}

void AInteractableActor::IsBeingPressedByRTrigger(const FHitResult& hitResult)
{
	IsBeingPressedByRTrigger_BP(hitResult);
}

void AInteractableActor::IsBeingReleasedByRTrigger(const FHitResult& hitResult)
{
	IsBeingReleasedByRTrigger_BP(hitResult);
}
