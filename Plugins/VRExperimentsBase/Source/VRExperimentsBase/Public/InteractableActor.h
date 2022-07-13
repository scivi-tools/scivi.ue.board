// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseInformant.h"
#include "InteractableActor.generated.h"

UCLASS()
class VREXPERIMENTSBASE_API AInteractableActor : public AActor
{
	GENERATED_BODY()
	
public:

	//trigger interaction
	virtual void OnPressedByTrigger(const FHitResult& hitResult);
	virtual void OnReleasedByTrigger(const FHitResult& hitResult);
	//eye track interaction
	virtual void BeginOverlapByEyeTrack();
	virtual void ProcessEyeTrack(const FGaze& gaze, const FHitResult& hitResult);
	virtual void EndOverlapByEyeTrack();
	//controller interaction
	virtual void BeginOverlapByController();
	virtual void InFocusByController(const FHitResult& hitResult);
	virtual void EndOverlapByController();
	//distance
	virtual void HadCloseToPlayer();
	virtual void HadFarToPlayer();

protected:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "HadEyeFocus"))
	void BeginOverlapByEyeTrack_BP();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "EyeTrackTick"))
	void ProcessEyeTrack_BP(const FGaze& gaze, const FHitResult& hitResult);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "LostEyeFocus"))
	void EndOverlapByEyeTrack_BP();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnRTriggerPressed"))
	void OnPressedByTrigger_BP(const FHitResult& hitResult);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnRTriggerReleased"))
	void OnReleasedByTrigger_BP(const FHitResult& hitResult);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "HadControllerFocus"))
	void BeginOverlapByController_BP();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "ControllerFocusTick"))
	void InFocusByController_BP(const FHitResult& hitResult);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "LostControllerFocus"))
	void EndOverlapByController_BP();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "HadCloseToPlayer"))
	void HadCloseToPlayer_BP();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "HadFarToPlayer"))
	void HadFarToPlayer_BP();
};
