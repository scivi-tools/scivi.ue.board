// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "ReadingTracker.h"
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

	UFUNCTION(BlueprintCallable)
	void SetVisibility_MC_Right(bool visibility);
	UFUNCTION(BlueprintCallable)
	void SetVisibility_MC_Left(bool visibility);
	void GetGaze(FGaze& gaze) const;
	UFUNCTION()
	void StartRecording();
	UFUNCTION()
	void StopRecording();
	bool IsRecording() const;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UCameraComponent* CameraComponent;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UArrowComponent* EyeTrackingArrow;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UMotionControllerComponent* MC_Left;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UStaticMeshComponent* MC_Left_Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	class UWidgetInteractionComponent* MC_Left_Interaction_Lazer;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UMotionControllerComponent* MC_Right;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	UStaticMeshComponent* MC_Right_Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	class UWidgetInteractionComponent* MC_Right_Interaction_Lazer;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	class UAudioCaptureComponent* AudioCapture;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	class USubmixRecorder* RecorderComponent;

protected:
	UFUNCTION()
	void OnRTriggerPressed();
	UFUNCTION()
	void OnRTriggerReleased();
	UFUNCTION()
	void CameraMove_LeftRight(float value);
	UFUNCTION()
	void CameraMove_UpDown(float value);

	
};
