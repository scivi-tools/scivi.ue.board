// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableActor.h"
#include "UI_Blank.h"

//-------------------- Events -------------------

void AInteractableActor::BeginOverlapByEyeTrack()
{
	BeginOverlapByEyeTrack_BP();
}

void AInteractableActor::ProcessEyeTrack(const FGaze& gaze, const FHitResult& hitResult)
{
	ProcessEyeTrack_BP(gaze, hitResult);
}

void AInteractableActor::EndOverlapByEyeTrack()
{
	EndOverlapByEyeTrack_BP();
}

void AInteractableActor::OnPressedByTrigger(const FHitResult& hitResult)
{
	OnPressedByTrigger_BP(hitResult);
}

void AInteractableActor::OnReleasedByTrigger(const FHitResult& hitResult)
{
	OnReleasedByTrigger_BP(hitResult);
}

void AInteractableActor::BeginOverlapByController()
{
	BeginOverlapByController_BP();
}

void AInteractableActor::InFocusByController(const FHitResult& hitResult)
{
	InFocusByController_BP(hitResult);
}

void AInteractableActor::EndOverlapByController()
{
	EndOverlapByController_BP();
}

void AInteractableActor::HadCloseToPlayer()
{
	HadCloseToPlayer_BP();
}

void AInteractableActor::HadFarToPlayer()
{
	HadFarToPlayer_BP();
}
