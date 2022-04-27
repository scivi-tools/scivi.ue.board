// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInformant.h"
#include "ReadingTrackerGameMode.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/ArrowComponent.h"
#include "SRanipal_API_Eye.h"
#include "SRanipalEye_Core.h"
#include "SRanipalEye_FunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Stimulus.h"
#include "XRMotionControllerBase.h"

// Sets default values
ABaseInformant::ABaseInformant()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Take control of the default player
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoReceiveInput = EAutoReceiveInput::Player0;
	//create components
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->SetRelativeLocation(FVector(0, 0, 80));

	EyeTrackingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("EyeTracking"));
	EyeTrackingArrow->SetupAttachment(RootComponent);
	EyeTrackingArrow->SetHiddenInGame(true, true);//if you want to see eye tracking, then set first arg to false

	MC_Left = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Left"));
	MC_Left->SetupAttachment(RootComponent);
	MC_Left->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	MC_Left_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MCLeft_Mesh"));
	MC_Left_Mesh->SetupAttachment(MC_Left);
	MC_Left_Mesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	MC_Left_Interaction_Lazer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("MCLeft_Lazer"));
	MC_Left_Interaction_Lazer->SetupAttachment(MC_Left);
	MC_Left_Interaction_Lazer->bShowDebug = true;
	MC_Left_Interaction_Lazer->DebugColor = FColor::Green;
	MC_Left_Interaction_Lazer->InteractionDistance = 750.0f;
	MC_Left_Interaction_Lazer->SetHiddenInGame(false);

	MC_Right = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Right"));
	MC_Right->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	MC_Right->SetupAttachment(RootComponent);
	MC_Right_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MCRight_Mesh"));
	MC_Right_Mesh->SetupAttachment(MC_Right);
	MC_Right_Mesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	MC_Right_Interaction_Lazer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("MCRight_Lazer"));
	MC_Right_Interaction_Lazer->SetupAttachment(MC_Right);
	MC_Right_Interaction_Lazer->bShowDebug = true;
	MC_Right_Interaction_Lazer->DebugColor = FColor::Green;
	MC_Right_Interaction_Lazer->InteractionDistance = 750.0f;
	MC_Right_Interaction_Lazer->SetHiddenInGame(false);
}

// Called when the game starts or when spawned
void ABaseInformant::BeginPlay()
{
	Super::BeginPlay();
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	SetVisibility_MC_Right(false);
	SetVisibility_MC_Left(false);
	old_MC_Left_Direction = MC_Left->GetComponentLocation() + MC_Left->GetForwardVector();
	old_MC_Right_Direction = MC_Right->GetComponentLocation() + MC_Right->GetForwardVector();
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	if (GM)
		GM->NotifyInformantSpawned(this);
}

// Called every frame
void ABaseInformant::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FGaze gaze;
	FHitResult hitPoint(ForceInit);
	GetGaze(gaze);
	const float ray_length = 1000.0f;
	EyeTrackingArrow->SetWorldLocationAndRotation(gaze.origin, gaze.direction.Rotation());
	if (GM->RayTrace(this, gaze.origin, gaze.origin + gaze.direction * ray_length, hitPoint))
	{
		auto focusedStimulus = Cast<AStimulus>(hitPoint.Actor);
		if (focusedStimulus) 
			focusedStimulus->OnInFocus(gaze, hitPoint);
	}

	//check if Right controller has moved
	auto MC_Right_direction = MC_Right->GetComponentLocation() + MC_Right->GetForwardVector();
	if (MC_Right_direction != old_MC_Right_Direction)
	{
		SetVisibility_MC_Right(true);
		MC_Right_NoActionTime = 0.0;
	}
	else
	{
		MC_Right_NoActionTime += DeltaTime;
		if (MC_Right_NoActionTime >= MCNoActionTimeout) SetVisibility_MC_Right(false);
	}

	//check if Left controller has moved
	auto MC_Left_direction = MC_Left->GetComponentLocation() + MC_Left->GetForwardVector();
	if (MC_Left_direction != old_MC_Left_Direction)
	{
		SetVisibility_MC_Left(true);
		MC_Left_NoActionTime = 0.0;
	}
	else
	{
		MC_Left_NoActionTime += DeltaTime;
		if (MC_Left_NoActionTime >= MCNoActionTimeout) SetVisibility_MC_Left(false);
	}
}

// Called to bind functionality to input
void ABaseInformant::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	InputComponent->BindAction("RTrigger", IE_Pressed, this, &ABaseInformant::OnRTriggerPressed);
	InputComponent->BindAction("RTrigger", IE_Released, this, &ABaseInformant::OnRTriggerReleased);
	InputComponent->BindAxis("CameraMove_RightLeft", this, &ABaseInformant::CameraMove_LeftRight);
	InputComponent->BindAxis("CameraMove_UpDown", this, &ABaseInformant::CameraMove_UpDown);
}

void ABaseInformant::OnRTriggerPressed()
{
	MC_Right_NoActionTime = 0.0f;
	SetVisibility_MC_Right(true);

	FGaze gaze;
	GetGaze(gaze);
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FVector trigger_ray = MC_Right->GetComponentLocation() +
			MC_Right->GetForwardVector() * ray_length;
	FHitResult hitPoint(ForceInit);
	if (GM->RayTrace(this, MC_Right->GetComponentLocation(), trigger_ray, hitPoint))
	{
		auto triggeredStimulus = Cast<AStimulus>(hitPoint.Actor);
		if (triggeredStimulus)
			triggeredStimulus->OnTriggerPressed(hitPoint);		
	}
		

	//start event of clicking
	FKey LMB(TEXT("LeftMouseButton"));
	MC_Right_Interaction_Lazer->PressPointerKey(LMB);
}

void ABaseInformant::OnRTriggerReleased()
{
	FGaze gaze;
	GetGaze(gaze);
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FVector trigger_ray = MC_Right->GetComponentLocation() +
		MC_Right->GetForwardVector() * ray_length;
	FHitResult hitPoint(ForceInit);
	if (GM->RayTrace(this, MC_Right->GetComponentLocation(), trigger_ray, hitPoint))
	{
		auto triggeredStimulus = Cast<AStimulus>(hitPoint.Actor);
		if (triggeredStimulus) 
			triggeredStimulus->OnTriggerReleased(hitPoint);
	}

	//start event of clicking
	FKey LMB(TEXT("LeftMouseButton"));
	MC_Right_Interaction_Lazer->ReleasePointerKey(LMB);
}

void ABaseInformant::CameraMove_LeftRight(float value)
{
	CameraComponent->AddLocalRotation(FRotator(0.0f, value, 0.0f));
}

void ABaseInformant::CameraMove_UpDown(float value)
{
	CameraComponent->AddLocalRotation(FRotator(value, 0.0f, 0.0f));
}

void ABaseInformant::SetVisibility_MC_Right(bool visibility)
{
	if (IsValid(MC_Right)) 
	{
		MC_Right->SetHiddenInGame(!visibility, true);
		MC_Right_Interaction_Lazer->bShowDebug = visibility;
	}
}

void ABaseInformant::SetVisibility_MC_Left(bool visibility)
{
	if (IsValid(MC_Left))
	{
		MC_Left->SetHiddenInGame(!visibility, true);
		MC_Left_Interaction_Lazer->bShowDebug = visibility;
	}
}

void ABaseInformant::GetGaze(FGaze& gaze) const
{
	auto instance = SRanipalEye_Core::Instance();
    ViveSR::anipal::Eye::VerboseData vd;
    instance->GetVerboseData(vd);
	instance->GetGazeRay(GazeIndex::COMBINE, gaze.origin, gaze.direction);
	gaze.origin = CameraComponent->GetComponentTransform().TransformPosition(gaze.origin);
	gaze.direction = CameraComponent->GetComponentTransform().TransformVector(gaze.direction);
	gaze.left_pupil_diameter_mm = vd.left.pupil_diameter_mm;
	gaze.left_pupil_openness = vd.left.eye_openness;
	gaze.right_pupil_diameter_mm = vd.right.pupil_diameter_mm;
	gaze.right_pupil_openness = vd.right.eye_openness;
	//here you can insert custom calibration
}