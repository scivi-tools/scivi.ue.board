// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseInformant.h"
#include "InteractableActor.generated.h"

UCLASS()
class AInteractableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	virtual void IsBeingFocused(const FGaze& gaze, const FHitResult& hitResult);
	virtual void IsBeingPressedByRTrigger(const FHitResult& hitResult);
	virtual void IsBeingReleasedByRTrigger(const FHitResult& hitResult);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnFocus"))
	void IsBeingFocused_BP(const FGaze& gaze, const FHitResult& hitResult);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnRTriggerPressed"))
	void IsBeingPressedByRTrigger_BP(const FHitResult& hitResult);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnRTriggerReleased"))
	void IsBeingReleasedByRTrigger_BP(const FHitResult& hitResult);

	


	/*UFUNCTION(BlueprintNativeEvent)
	void IsBeingPressedByLTrigger(const FHitResult& hitResult);
	UFUNCTION(BlueprintNativeEvent)
	void IsBeingReleasedByLTrigger(const FHitResult& hitResult);*/

};
