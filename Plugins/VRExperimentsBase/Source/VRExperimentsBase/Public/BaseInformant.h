// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "BaseInformant.generated.h"

USTRUCT(BlueprintType)
struct VREXPERIMENTSBASE_API FGaze
{
	GENERATED_BODY()
	UPROPERTY()
	FVector origin;
	UPROPERTY()
	FVector direction;
	UPROPERTY()
	float left_pupil_diameter_mm = 0.0f;
	UPROPERTY()
	float left_pupil_openness = 0.0f;
	UPROPERTY()
	float right_pupil_diameter_mm = 0.0f;
	UPROPERTY()
	float right_pupil_openness = 0.0f;
	UPROPERTY()
	float cf = 0.0f;
};

UCLASS()
class VREXPERIMENTSBASE_API ABaseInformant : public ACharacter
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
	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetInteractionDistance() const { return InteractionDistance; }
	UFUNCTION(BlueprintCallable)
	void SetInteractionDistance(float new_distance);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsWalkingEnabled = true;

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
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	class USphereComponent* InteractionCollider;
	UFUNCTION()
	void OnRTriggerPressed();
	UFUNCTION()
	void OnRTriggerReleased();
	UFUNCTION()
	void OnLTriggerPressed();
	UFUNCTION()
	void OnLTriggerReleased();
	UFUNCTION()
	void CameraMove_LeftRight(float value);
	UFUNCTION()
	void CameraMove_UpDown(float value);
	//------------ Walking -----------------
	UFUNCTION()
	void Walking_Trajectory();
	UFUNCTION()
	void Walking_Teleport();
	bool bIsWalking = false;
	FVector newPosition;
	float HeightDeviance = 5.0f;
	float FloorHeight = 0.0f;
	//------------ Interaction ----------------
	TSet<AActor*> close_actors;
	class AInteractableActor* eye_tracked_actor = nullptr;
	class AInteractableActor* actor_pointed_by_right_mc = nullptr;
	class AInteractableActor* actor_pointed_by_left_mc = nullptr;
	float InteractionDistance = 1000.0f;


	
};
