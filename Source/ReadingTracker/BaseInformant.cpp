// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInformant.h"
#include "ReadingTrackerGameMode.h"
#include "Components/WidgetInteractionComponent.h"
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
	CameraComponent->SetRelativeLocation(FVector(0, 0, 80));
	CameraComponent->SetupAttachment(RootComponent);
	MC_Left = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Left"));
	MC_Left->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	MC_Left->SetupAttachment(RootComponent);
	MC_Left_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MCLeft_Mesh"));
	MC_Left_Mesh->SetupAttachment(MC_Left);
	MC_Left_Mesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	MC_Left_Lazer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("MCLeft_Lazer"));
	MC_Left_Lazer->SetupAttachment(MC_Left);
	MC_Left_Lazer->bShowDebug = true;
	MC_Left_Lazer->DebugColor = FColor::Green;
	//MC_Left_Lazer-> = 0.25f;
	MC_Left_Lazer->InteractionDistance = 750.0f;
	MC_Left_Lazer->SetHiddenInGame(false);

	MC_Right = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Right"));
	MC_Right->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	MC_Right->SetupAttachment(RootComponent);
	MC_Right_Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MCRight_Mesh"));
	MC_Right_Mesh->SetupAttachment(MC_Right);
	MC_Right_Mesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	MC_Right_Lazer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("MCRight_Lazer"));
	MC_Right_Lazer->SetupAttachment(MC_Right);
	MC_Right_Lazer->bShowDebug = true;
	MC_Right_Lazer->DebugColor = FColor::Green;
	//MC_Right_Lazer->ArrowSize = 0.25f;
	MC_Right_Lazer->InteractionDistance = 750.0f;
	MC_Right_Lazer->SetHiddenInGame(false);
}

// Called when the game starts or when spawned
void ABaseInformant::BeginPlay()
{
	Super::BeginPlay();
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	//m_mcRight->SetHiddenInGame(true, true);
	if (GM)
		GM->NotifyInformantSpawned(this);
}

// Called every frame
void ABaseInformant::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FGaze gaze;
	FVector hitPoint;
	GetGaze(gaze);
	AStimulus* focusedStimulus = Cast<AStimulus>(GM->RayTrace(this, gaze.origin,
											gaze.origin + gaze.direction * ray_length, hitPoint));
	if (focusedStimulus)
	{
		focusedStimulus->OnInFocus(gaze, hitPoint);
	}


}

// Called to bind functionality to input
void ABaseInformant::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	InputComponent->BindAction("RTrigger", IE_Pressed, this, &ABaseInformant::OnTriggerPressed);
	InputComponent->BindAction("RTrigger", IE_Released, this, &ABaseInformant::OnTriggerReleased);
}

void ABaseInformant::OnTriggerPressed()
{
	
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FGaze gaze;
	FVector hitPoint;
	GetGaze(gaze);
	if (MC_Right)
	{
		FVector trigger_ray = MC_Right->GetComponentLocation() +
			MC_Right->GetForwardVector() * ray_length;
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(rand(), 10, FColor::Green, trigger_ray.ToCompactString());
		AStimulus* triggeredStimulus = Cast<AStimulus>(GM->RayTrace(this, MC_Right->GetComponentLocation(),
			trigger_ray, hitPoint));
		if (triggeredStimulus)
		{
			triggeredStimulus->OnTriggerPressed(gaze, hitPoint);
		}
	}
}

void ABaseInformant::OnTriggerReleased()
{
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FGaze gaze;
	FVector hitPoint;
	GetGaze(gaze);
	if (MC_Right)
	{
		FVector trigger_ray = MC_Right->GetComponentLocation() +
			MC_Right->GetForwardVector() * ray_length;
		AStimulus* triggeredStimulus = Cast<AStimulus>(GM->RayTrace(this, MC_Right->GetComponentLocation(),
			trigger_ray, hitPoint));
		if (triggeredStimulus)
		{
			triggeredStimulus->OnTriggerReleased(gaze, hitPoint);
		}
	}
}

void ABaseInformant::ToggleMotionController(bool visiblity)
{
	if (MC_Right)
		MC_Right->SetHiddenInGame(!visiblity, true);
}

void ABaseInformant::GetGaze(FGaze& gaze) const
{
	auto instance = SRanipalEye_Core::Instance();
    ViveSR::anipal::Eye::VerboseData vd;
    instance->GetVerboseData(vd);
	instance->GetGazeRay(GazeIndex::COMBINE, gaze.origin, gaze.direction);
	gaze.left_pupil_diameter_mm = vd.left.pupil_diameter_mm;
	gaze.left_pupil_openness = vd.left.eye_openness;
	gaze.right_pupil_diameter_mm = vd.right.pupil_diameter_mm;
	gaze.right_pupil_openness = vd.right.eye_openness;
	//here you can insert custom calibration
}