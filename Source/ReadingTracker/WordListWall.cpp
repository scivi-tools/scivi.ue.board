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

//UTexture2D* CopyTexture(const UTexture2D& source)
//{
//	auto result = UTexture2D::CreateTransient(source.GetSizeX(), source.GetSizeY(), PF_B8G8R8A8, FName(FGuid::NewGuid().ToString()));
//	auto& src_bulk = source.PlatformData->Mips[0].BulkData;
//	auto& dst_bulk = result->PlatformData->Mips[0].BulkData;
//
//	auto src_pixels = reinterpret_cast<const FColor*>(src_bulk.LockReadOnly());
//	auto dst_pixels = reinterpret_cast<FColor*>(dst_bulk.Lock(LOCK_READ_WRITE));
//	for (int32 y = 0; y < source.GetSizeY(); ++y)
//		FMemory::Memcpy(dst_pixels + y * result->GetSizeX(),
//			src_pixels + y * source.GetSizeX(),
//			source.GetSizeX() * sizeof(FColor));
//	src_bulk.Unlock();
//	dst_bulk.Unlock();
//	result->UpdateResource();
//	return result;
//}

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
	SetActorEnableCollision(is_visible);
}

void AWordListWall::SetWallName(const FString& new_name)
{
	if (IsValid(ListHeader) && IsValid(ListHeader->GetWidget())) 
	{
		auto txtName = Cast<UTextBlock>(ListHeader->GetWidget()->GetWidgetFromName(TEXT("txtWallName")));
		if (IsValid(txtName))
		{
			ListHeader->SetDrawSize(FVector2D(FMath::Max(250.0f, new_name.Len() * 25.0f), 100.0f));
			txtName->SetText(FText::FromString(new_name));
			name = new_name;
		}
	}
}

void AWordListWall::AddAOI(const FAOI* aoi)
{
	auto list = Cast<UScrollBox>(List->GetWidget()->GetWidgetFromName(TEXT("List")));
	auto entry_name = FName(aoi->name + "_" + FGuid::NewGuid().ToString() + TEXT("_Entry"));
	//if list doesnt contains an entry then insert
	auto childs = list->GetAllChildren();

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
		AOI_indices.Add(btnRemoveEntry, aoi->id);
		auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
		GM->SendWallLogToSciVi(EWallLogAction::AddAOI, this->GetWallName(), aoi->id, aoi->name);
	}
	else if (!IsValid(aoi->image))
		UE_LOG(LogTemp, Display, TEXT("aoi->image isn't valid"));
}

void AWordListWall::ClearList()
{
	if (IsValid(List) && IsValid(List->GetWidget()))
	{
		auto list = Cast<UScrollBox>(List->GetWidget()->GetWidgetFromName(TEXT("List")));
		if (IsValid(list))
		{
			list->ClearChildren();
			entries.Empty();
		}
		UE_LOG(LogTemp, Display, TEXT("List %s cleared"), *name);
	}
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
		auto AOI_name = entry->GetName();
		int32 index;
		AOI_name.FindChar(L'_', index);
		AOI_name.RemoveAt(index, AOI_name.Len() - index + 1);
		auto image = Cast<UImage>(entry->GetWidgetFromName(TEXT("AOI_Image")));
		entry->RemoveFromParent();
		entries.Remove(clickedButton);
		int AOI_index = AOI_indices[clickedButton];
		AOI_indices.Remove(clickedButton);
		GM->SendWallLogToSciVi(EWallLogAction::RemoveAOI, name, AOI_index, AOI_name);
	}
}



