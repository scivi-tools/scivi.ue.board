// Fill out your copyright notice in the Description page of Project Settings.


#include "WordListWall.h"
#include "Components/WidgetComponent.h"

// Sets default values
AWordListWall::AWordListWall()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetupAttachment(RootComponent);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetStaticMesh(MeshAsset.Object);
	Mesh->SetUsingAbsoluteScale(true);
	Mesh->SetRelativeScale3D(FVector(0.15f, 1.0f, 5.0f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
	Mesh->SetupAttachment(RootComponent);

	DeleteButton = CreateDefaultSubobject<UWidgetComponent>(TEXT("Delete Button"));
	DeleteButton->SetRelativeLocation(FVector(0.0f, 0.0f, 510.0f));
	DeleteButton->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWordListWall::BeginPlay()
{
	Super::BeginPlay();
	
}

