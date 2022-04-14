// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "BaseInformant.generated.h"

struct FGaze
{
	FVector origin;
	FVector direction;
	float left_pupil_diameter_mm;
	float left_pupil_openness;
	float right_pupil_diameter_mm;
	float right_pupil_openness;
	float cf;
};

UCLASS()
class READINGTRACKER_API ABaseInformant : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseInformant();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void OnTriggerPressed();
	void OnTriggerReleased();

	void ToggleMotionController(bool visiblity);

	void GetGaze(FGaze& gaze) const;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UMotionControllerComponent* m_mcRight;
	
};
