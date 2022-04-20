// Fill out your copyright notice in the Description page of Project Settings.


#include "WordListWall.h"
#include "Components/WidgetComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ReadingTrackerGameMode.h"

// Sets default values
AWordListWall::AWordListWall()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterialAsset(TEXT("Material'/Game/StarterContent/Materials/M_Brick_Clay_New.M_Brick_Clay_New'"));
	static ConstructorHelpers::FClassFinder<UUserWidget> ListHeaderWidgetClass(TEXT("/Game/UI/UI_ListHeader"));
	static ConstructorHelpers::FClassFinder<UUserWidget> ListWidgetClass(TEXT("/Game/UI/UI_List"));

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = DefaultSceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(MeshAsset.Object);
	Mesh->SetMaterial(0, WallMaterialAsset.Object);
	Mesh->SetRelativeScale3D(FVector(0.15f, 1.0f, 3.0f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));

	ListHeader = CreateDefaultSubobject<UWidgetComponent>(TEXT("Delete Button"));
	ListHeader->SetupAttachment(RootComponent);
	ListHeader->SetRelativeLocation(FVector(0.0f, 0.0f, 330.0f));
	ListHeader->SetUsingAbsoluteScale(true);
	ListHeader->SetDrawSize(FVector2D(200.0f, 50.0f));
	ListHeader->SetWidgetClass(ListHeaderWidgetClass.Class);

	List = CreateDefaultSubobject<UWidgetComponent>(TEXT("List"));
	List->SetupAttachment(RootComponent);
	List->SetRelativeLocation(FVector(10.0f, 0.0f, 150.0f));
	List->SetDrawSize(FVector2D(90.0f, 280.0f));
	List->SetWidgetClass(ListWidgetClass.Class);
}

// Called when the game starts or when spawned
void AWordListWall::BeginPlay()
{
	Super::BeginPlay();
	//set onClick event on DeleteListButton
	auto btn = Cast<UButton>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("btnDeleteList")));
	if (btn)
		btn->OnClicked.AddDynamic(this, &AWordListWall::OnClicked_DeleteList);
}

bool AWordListWall::IsHiddenInGame() const
{
	return bHiddenInGame;
}

void AWordListWall::SetVisibility(bool is_visible)
{
	bHiddenInGame = !is_visible;
	SetActorHiddenInGame(bHiddenInGame);
}

void AWordListWall::SetWallName(const FString& name)
{
	auto txtName = Cast<UTextBlock>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("txtWallName")));
	if (txtName)
		txtName->SetText(FText::FromString(name));
}

void AWordListWall::OnClicked_DeleteList()
{
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	GM->DeleteList(this);
}

