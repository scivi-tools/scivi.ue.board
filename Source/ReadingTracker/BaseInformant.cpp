// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseInformant.h"
#include "ReadingTrackerGameMode.h"
#include "SRanipal_API_Eye.h"
#include "SRanipalEye_Core.h"
#include "SRanipalEye_FunctionLibrary.h"
#include "Stimulus.h"

// Sets default values
ABaseInformant::ABaseInformant()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetRelativeLocation(FVector(0, 0, 80));
	CameraComponent->SetupAttachment(RootComponent);
	m_mcRight = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("Controller"));
	m_mcRight->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ABaseInformant::BeginPlay()
{
	Super::BeginPlay();
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
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
	FVector trigger_ray = m_mcRight->GetComponentLocation() +
		m_mcRight->GetForwardVector() * ray_length;
	AStimulus* triggeredStimulus = Cast<AStimulus>(GM->RayTrace(this, m_mcRight->GetComponentLocation(),
									trigger_ray, hitPoint));
	if (triggeredStimulus)
	{
		triggeredStimulus->OnTriggerPressed(gaze, hitPoint);
	}
}

void ABaseInformant::OnTriggerReleased()
{
	const float ray_length = 1000.0f;
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	FGaze gaze;
	FVector hitPoint;
	GetGaze(gaze);
	FVector trigger_ray = m_mcRight->GetComponentLocation() +
		m_mcRight->GetForwardVector() * ray_length;
	AStimulus* triggeredStimulus = Cast<AStimulus>(GM->RayTrace(this, m_mcRight->GetComponentLocation(),
		trigger_ray, hitPoint));
	if (triggeredStimulus)
	{
		triggeredStimulus->OnTriggerReleased(gaze, hitPoint);
	}
}

void ABaseInformant::ToggleMotionController(bool visiblity)
{
	m_mcRight->SetHiddenInGame(!visiblity, true);
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