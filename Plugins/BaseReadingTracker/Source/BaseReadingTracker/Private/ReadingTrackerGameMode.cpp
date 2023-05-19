// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadingTrackerGameMode.h"
#include "Stimulus.h"
//#include "WordListWall.h"
#include "UI_Blank.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/WidgetComponent.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/Base64.h"



UTexture2D* loadTexture2DFromBytes(const TArray<uint8>& bytes, EImageFormat fmt)
{
	UTexture2D* loadedT2D = nullptr;

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(fmt);

	if (imageWrapper.IsValid() && imageWrapper->SetCompressed(bytes.GetData(), bytes.Num()))
	{
		TArray<uint8> uncompressedBGRA;
		if (imageWrapper->GetRaw(ERGBFormat::BGRA, 8, uncompressedBGRA))
		{
			int w = imageWrapper->GetWidth();
			int h = imageWrapper->GetHeight();
			loadedT2D = UTexture2D::CreateTransient(w, h);
			loadedT2D->AddToRoot();
			if (!loadedT2D)
				return nullptr;

			void* textureData = loadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(textureData, uncompressedBGRA.GetData(), uncompressedBGRA.Num());
			loadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			loadedT2D->UpdateResource();
		}
	}

	return loadedT2D;
}

UTexture2D* loadTexture2DFromFile(const FString& fullFilePath)
{
	UTexture2D* loadedT2D = nullptr;

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	TArray<uint8> rawFileData;
	if (!FFileHelper::LoadFileToArray(rawFileData, *fullFilePath))
		return nullptr;

	if (imageWrapper.IsValid() && imageWrapper->SetCompressed(rawFileData.GetData(), rawFileData.Num()))
	{
		TArray<uint8> uncompressedBGRA;
		if (imageWrapper->GetRaw(ERGBFormat::BGRA, 8, uncompressedBGRA))
		{
			loadedT2D = UTexture2D::CreateTransient(imageWrapper->GetWidth(), imageWrapper->GetHeight(), PF_B8G8R8A8);
			if (!loadedT2D)
				return nullptr;

			void* textureData = loadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(textureData, uncompressedBGRA.GetData(), uncompressedBGRA.Num());
			loadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			loadedT2D->UpdateResource();
		}
	}

	return loadedT2D;
}

void CopyTexture2DFragment(UTexture2D* destination, const UTexture2D* source, int start_x, int start_y, int width, int height)
{
	auto dst_size = FVector2D(destination->GetSizeX(), destination->GetSizeY());
	auto& src_bulk = source->PlatformData->Mips[0].BulkData;
	auto& dst_bulk = destination->PlatformData->Mips[0].BulkData;

	auto src_pixels = reinterpret_cast<const FColor*>(src_bulk.LockReadOnly());
	auto dst_pixels = reinterpret_cast<FColor*>(dst_bulk.Lock(LOCK_READ_WRITE));
	for (int y = 0; y < height; ++y)
		FMemory::Memcpy(dst_pixels + y * destination->GetSizeX(),
			src_pixels + (y + start_y) * source->GetSizeX() + start_x,
			width * sizeof(FColor));
	src_bulk.Unlock();
	dst_bulk.Unlock();
	destination->UpdateResource();
}

//-------------------------- API ------------------------

AReadingTrackerGameMode::AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AReadingTrackerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AReadingTrackerGameMode::NotifyStimulustSpawned(AStimulus* _stimulus)
{
	stimulus = _stimulus;
}

//------------- Recording Menu -------------------



void AReadingTrackerGameMode::RecordingMenu_CreateList()
{
	//auto root = uiRecordingMenu->GetWidget();
	//auto textBlock = Cast<UEditableText>(root->GetWidgetFromName(TEXT("textNewName")));
	////CreateListOfWords(textBlock->GetText().ToString());
}


//------------- List of words -------------------
//void AReadingTrackerGameMode::CreateListOfWords(const FString& wall_name)
//{
//	int visible_count = 0;
//	for (auto wall : walls)
//	{
//		if (wall->IsHiddenInGame())
//		{
//			wall->SetVisibility(true);
//			wall->SetWallName(wall_name);
//			SendWallLogToSciVi(EWallLogAction::NewWall, wall_name);
//			break;
//		}
//		else ++visible_count;
//	}
//	SetRecordingMenuVisibility(false);
//
//	if (visible_count == MaxWallsCount - 1)
//		uiCreateListButton->SetEnabled(false);
//}
//
//void AReadingTrackerGameMode::DeleteList(AWordListWall* const wall)
//{
//	if (IsValid(wall))
//	{
//		wall->SetVisibility(false);
//		wall->ClearList();
//		SendWallLogToSciVi(EWallLogAction::DeleteWall, wall->GetWallName());
//		uiCreateListButton->SetEnabled(true);
//	}
//}
//
//void AReadingTrackerGameMode::DeleteAllLists()
//{
//	for (auto wall : walls)
//		if (!wall->IsHiddenInGame())
//			DeleteList(wall);
//}
//
//void AReadingTrackerGameMode::AddAOIsToList(AWordListWall* const wall)
//{
//	for (auto aoi : stimulus->SelectedAOIs)
//	{
//		wall->AddAOI(aoi);
//	}
//	//clear selection on stimulus
//	stimulus->ClearSelectedAOIs();
//}

//----------- Scivi networking -------------------

void AReadingTrackerGameMode::OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson)
{
	Super::OnSciViMessageReceived(msgJson);
	if (msgJson->TryGetField("customCalibrate")) stimulus->customCalibrate();
	else 
		ParseNewImage(msgJson);
}

void AReadingTrackerGameMode::ParseNewImage(const TSharedPtr<FJsonObject>& json)
{
	FString image_textdata = json->GetStringField("image");
	TArray<uint8> img;
	FString png = "data:image/png;base64,";
	FString jpg = "data:image/jpeg;base64,";
	EImageFormat fmt = EImageFormat::Invalid;
	int startPos = 0;
	if (image_textdata.StartsWith(png))
	{
		fmt = EImageFormat::PNG;
		startPos = png.Len();
	}
	else if (image_textdata.StartsWith(jpg))
	{
		fmt = EImageFormat::JPEG;
		startPos = jpg.Len();
	}

	if (fmt != EImageFormat::Invalid &&
		FBase64::Decode(&(image_textdata.GetCharArray()[startPos]), img))
	{
		float sx = json->GetNumberField("scaleX");
		float sy = json->GetNumberField("scaleY");
		UTexture2D* texture = loadTexture2DFromBytes(img, fmt);
		TArray<FAOI> AOIs;
		TMap<FString, int> counts;
		int last_id = 0;
		for (auto& aoi_text : json->GetArrayField("AOIs"))
		{
			FAOI aoi;
			aoi.id = last_id++;
			auto nameField = aoi_text->AsObject()->TryGetField("name");
			if (nameField)
			{
				aoi.name = nameField->AsString();
				if (counts.Contains(aoi.name)) counts[aoi.name]++;
				else counts.Add(aoi.name, 0);
				aoi.order = counts[aoi.name];
			}
			auto pathField = aoi_text->AsObject()->TryGetField("path");
			if (pathField)
			{
				auto& path = pathField->AsArray();
				for (auto& point : path)
					aoi.path.Add(FVector2D(point->AsArray()[0]->AsNumber(), point->AsArray()[1]->AsNumber()));
			}
			auto bboxField = aoi_text->AsObject()->TryGetField("bbox");
			if (bboxField)
			{
				auto& bbox = bboxField->AsArray();
				aoi.bbox = FBox2D(FVector2D(bbox[0]->AsNumber(), bbox[1]->AsNumber()),
					FVector2D(bbox[2]->AsNumber(), bbox[3]->AsNumber()));
			}
			auto size = aoi.bbox.GetSize();
			auto start = aoi.bbox.Min;
			aoi.image = UTexture2D::CreateTransient(size.X, size.Y);
			aoi.image->AddToRoot();
			CopyTexture2DFragment(aoi.image, texture, start.X, start.Y, size.X, size.Y);

			// find audio desciptions
			auto dir = FPaths::ProjectDir() + FString::Printf(TEXT("/../AudioDescriptions/%s"), *nameField->AsString());
			if (FPaths::FileExists(dir + TEXT(".mp3")))
				aoi.audio_desciptions.Add(dir + TEXT(".mp3"));
			else if (FPaths::DirectoryExists(dir))
			{
				IFileManager::Get().IterateDirectoryStat(*dir, [&aoi](const TCHAR* filename, const FFileStatData& data)
					{
						aoi.audio_desciptions.Add(filename);
						return true;
					});
			}
			else
				UE_LOG(LogTemp, Display, TEXT("No audio description for %s AOI"), *aoi.name);

			AOIs.Add(aoi);
		}
		if (IsValid(texture))
			stimulus->UpdateStimulus(texture, sx, sy, AOIs, true);
		UE_LOG(LogTemp, Display, TEXT("New Image parsed"));
	}
}

void AReadingTrackerGameMode::SendWallLogToSciVi(EWallLogAction Action, const FString& WallName, int AOI_index, const FString& AOI)
{
	FString msg;
	FString ActionStr;
	switch (Action)
	{
	case EWallLogAction::NewWall: ActionStr = TEXT("NewWall"); break;
	case EWallLogAction::DeleteWall: ActionStr = TEXT("DeleteWall"); break;
	case EWallLogAction::AddAOI: ActionStr = TEXT("AddAOI"); break;
	case EWallLogAction::RemoveAOI: ActionStr = TEXT("RemoveAOI"); break;
	default: ActionStr = TEXT("UnknownAction"); break;
	}
	if (Action == EWallLogAction::NewWall || Action == EWallLogAction::DeleteWall)
	{
		msg = FString::Printf(TEXT("\"WallLog\": {"
			"\"Action\": \"%s\","
			"\"Wall\": \"%s\"}"), *ActionStr, *WallName);
	}
	else
	{
		msg = FString::Printf(TEXT("\"WallLog\": {"
			"\"Action\": \"%s\","
			"\"Wall\": \"%s\","
			"\"AOI_index\": %i,"
			"\"AOI\": \"%s\"}"), *ActionStr, *WallName, AOI_index, *AOI);
	}
	SendToSciVi(msg);
}

void AReadingTrackerGameMode::SendGazeToSciVi(const FGaze& gaze, FVector2D& uv, int AOI_index, const FString& Id)
{
	//Send message to scivi
	auto json = FString::Printf(TEXT("\"Gaze\": {\"uv\": [%f, %f],"
		"\"origin\": [%f, %f, %f],"
		"\"target\": [%F, %F, %F],"
		"\"lpdmm\": %F, \"rpdmm\": %F,"
		"\"cf\": %F, \"AOI_index\": %i,"
		"\"Action\": \"%s\"}"),
		uv.X, uv.Y,
		gaze.origin.X, gaze.origin.Y, gaze.origin.Z,
		gaze.target.X, gaze.target.Y, gaze.target.Z,
		gaze.left_pupil_diameter_mm, gaze.right_pupil_diameter_mm, gaze.cf, AOI_index, *Id);
	SendToSciVi(json);
}
