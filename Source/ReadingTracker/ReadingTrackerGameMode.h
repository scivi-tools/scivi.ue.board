// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRGameModeBase.h"
#include "ReadingTrackerGameMode.generated.h"

//Channel to check collision with 
static const ECollisionChannel Stimulus_Channel = ECollisionChannel::ECC_GameTraceChannel1;
static const ECollisionChannel Floor_Channel = ECollisionChannel::ECC_WorldStatic;

USTRUCT()
struct FAOI
{
	GENERATED_BODY()
	int id;
	FString name;
	UPROPERTY()
	TArray<FVector2D> path;
	FBox2D bbox;
	UPROPERTY()
	UTexture2D* image;
	inline bool IsPointInside(const FVector2D& pt) const
	{
		if (!bbox.IsInside(pt)) return false;
		//check point in polygon
		bool IsInPolygon = false;
		int n = path.Num();
		for (int i = 0, j = n - 1; i < n; j = i++)
		{
			if (((path[i].Y > pt.Y) != (path[j].Y > pt.Y)) &&
				(pt.X < (path[j].X - path[i].X) * (pt.Y - path[i].Y) /
					(path[j].Y - path[i].Y) + path[i].X))
				IsInPolygon = !IsInPolygon;
		}
		return IsInPolygon;
	}
};

UENUM()
enum class EWallLogAction
{
	NewWall     UMETA(DisplayName = "NewWall"),
	DeleteWall      UMETA(DisplayName = "DeleteWall"),
	AddAOI   UMETA(DisplayName = "AddAOI"),
	RemoveAOI UMETA(DisplayName = "RemoveAOI")
};

/**
 * 
 */
UCLASS()
class READINGTRACKER_API AReadingTrackerGameMode : public AVRGameModeBase
{
	GENERATED_BODY()
public:
	AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int MaxWallsCount = 9;

	//----------------- Scene ----------------------
public:
	UPROPERTY(EditAnywhere, BlueprintReadwrite)
	float StimulusRemoteness = 510.0f;
	UPROPERTY(EditAnywhere, BlueprintReadwrite)
	float RecordingMenuRemoteness = 170.0f;

	virtual void NotifyInformantSpawned(class ABaseInformant* _informant) override;
	
	UFUNCTION(BlueprintCallable)
	void CreateListOfWords(const FString& wall_name);
	UFUNCTION(BlueprintCallable)
	void ReplaceObjectsOnScene(float stimulus_remoteness);
	UFUNCTION(BlueprintCallable)
	void DeleteList(AWordListWall* const wall);
	UFUNCTION(BlueprintCallable)
	void DeleteAllLists();
	
	void AddAOIsToList(AWordListWall* const wall);


	UPROPERTY(EditAnywhere, BlueprintReadonly)
	TSubclassOf<UUserWidget> RecordingMenuClass;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	TSubclassOf<UUserWidget> CreateListButtonClass;

	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "Stimulus")
	class AStimulus* stimulus;
	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "RecordingMenu")
	class AUI_Blank* uiRecordingMenu;
	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName="CreateListButton")
	class AUI_Blank* uiCreateListButton;
	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "Walls")
	TArray<class AWordListWall*> walls;

protected:
	int created_walls_count = 0;
	UFUNCTION()
	void CreateListBtn_OnClicked();
	UFUNCTION()
	void RecordingMenu_ClearNameForWall();
	UFUNCTION()
	void RecordingMenu_CreateList();
	UFUNCTION()
	void RecordingMenu_Cancel();
	void SetRecordingMenuVisibility(bool new_visibility);

//----------------------- SciVi networking --------------
public:
	void SendWallLogToSciVi(EWallLogAction Action, const FString& WallName, int AOI_index = -1, const FString& AOI = TEXT(""));
	void SendGazeToSciVi(const struct FGaze& gaze, FVector2D& uv, int AOI_index, const FString& Id);
protected:
	virtual void OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson) override;
	void ParseNewImage(const TSharedPtr<FJsonObject>& json);
};
