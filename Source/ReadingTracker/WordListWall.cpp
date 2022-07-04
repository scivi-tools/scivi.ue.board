// Fill out your copyright notice in the Description page of Project Settings.


#include "WordListWall.h"
#include <RichButton.h>
#include "Components/WidgetComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "ReadingTrackerGameMode.h"

// Sets default values
AWordListWall::AWordListWall()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterialAsset(TEXT("Material'/Game/StarterContent/Materials/M_Wall.M_Wall'"));
	static ConstructorHelpers::FClassFinder<UUserWidget> ListHeaderWidgetClass(TEXT("/Game/UI/UI_ListHeader"));
	static ConstructorHelpers::FClassFinder<UUserWidget> ListWidgetClass(TEXT("/Game/UI/UI_List"));
	static ConstructorHelpers::FClassFinder<UUserWidget> ListEntryWidgetClass(TEXT("/Game/UI/UI_ListEntry"));

	EntryWidgetClass = ListEntryWidgetClass.Class;
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = DefaultSceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetStaticMesh(MeshAsset.Object);
	Mesh->SetMaterial(0, WallMaterialAsset.Object);
	Mesh->SetRelativeScale3D(FVector(0.15f, 2.5f, 3.0f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));

	ListHeader = CreateDefaultSubobject<UWidgetComponent>(TEXT("Delete Button"));
	ListHeader->SetupAttachment(RootComponent);
	ListHeader->SetRelativeLocation(FVector(0.0f, 0.0f, 355.0f));
	ListHeader->SetUsingAbsoluteScale(true);
	ListHeader->SetDrawSize(FVector2D(250, 100.0f));
	ListHeader->SetWidgetClass(ListHeaderWidgetClass.Class);

	List = CreateDefaultSubobject<UWidgetComponent>(TEXT("List"));
	List->SetupAttachment(RootComponent);
	List->SetRelativeLocation(FVector(10.0f, 0.0f, 150.0f));
	List->SetDrawSize(FVector2D(240.0f, 280.0f));
	List->SetWidgetClass(ListWidgetClass.Class);
	List->SetUsingAbsoluteScale(true);
}

// Called when the game starts or when spawned
void AWordListWall::BeginPlay()
{
	Super::BeginPlay();
	//set onClick event on DeleteListButton
	auto btnDeleteList = Cast<UButton>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("btnDeleteList")));
	if (btnDeleteList)
		btnDeleteList->OnClicked.AddDynamic(this, &AWordListWall::OnClicked_DeleteList);
	auto btnAddToList = Cast<UButton>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("btnAddToList")));
	btnAddToList->OnClicked.AddDynamic(this, &AWordListWall::OnClicked_AddEntry);
	
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

void AWordListWall::SetWallName(const FString& new_name)
{
	auto txtName = Cast<UTextBlock>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("txtWallName")));
	if (txtName) 
	{
		ListHeader->SetDrawSize(FVector2D(FMath::Max(250.0f, new_name.Len() * 25.0f), 100.0f));
		txtName->SetText(FText::FromString(new_name));
		name = new_name;
	}
}

void AWordListWall::AddAOI(const FAOI* aoi)
{
	auto list = Cast<UScrollBox>(List->GetWidget()->GetWidgetFromName(TEXT("List")));
	auto entry_name = FName(aoi->name + TEXT("_Entry"));
	//if list doesnt contains an entry then insert
	auto childs = list->GetAllChildren();
	/*UE_LOG(LogTemp, Display, TEXT("add aoi begin"));
	for (auto child : childs)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *child->GetFName().ToString());
	}*/

	if (IsValid(aoi->image) &&
		!list->GetAllChildren().ContainsByPredicate([entry_name](UWidget* widget) {return widget->GetFName() == entry_name; }))
	{
		auto entry = UUserWidget::CreateWidgetInstance(*GetWorld(), EntryWidgetClass, entry_name);
		auto image = Cast<UImage>(entry->GetWidgetFromName(TEXT("AOI_Image")));
		image->SetBrushFromTexture(aoi->image, true);
		auto btnRemoveEntry = Cast<URichButton>(entry->GetWidgetFromName(TEXT("btnRemoveEntry")));
		btnRemoveEntry->OnClicked.AddDynamic(this, &AWordListWall::OnClicked_RemoveEntry);
		list->AddChild(entry);
		entries.Add(btnRemoveEntry, entry);
		auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
		GM->SendWallLogToSciVi(EWallLogAction::AddAOI, this->GetWallName(), aoi->name);
	}
	else if (!IsValid(aoi->image))
		UE_LOG(LogTemp, Display, TEXT("aoi->image isn't valid"));
}

void AWordListWall::ClearList()
{
	auto list = Cast<UScrollBox>(List->GetWidget()->GetWidgetFromName(TEXT("List")));
	list->ClearChildren();
	entries.Empty();
}

void AWordListWall::OnClicked_DeleteList()
{
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	GM->DeleteList(this);
}

void AWordListWall::OnClicked_AddEntry()
{
	auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
	GM->AddAOIsToList(this);
}

void AWordListWall::OnClicked_RemoveEntry(URichButton* clickedButton, const FVector2D& clickPos)
{
	if (entries.Contains(clickedButton))
	{
		auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
		auto entry = entries[clickedButton];
		auto entry_name = entry->GetName().Replace(TEXT("_Entry"), TEXT(""));
		auto image = Cast<UImage>(entry->GetWidgetFromName(TEXT("AOI_Image")));
		entry->RemoveFromParent();
		entries.Remove(clickedButton);
		GM->SendWallLogToSciVi(EWallLogAction::RemoveAOI, name, entry_name);
	}
}

