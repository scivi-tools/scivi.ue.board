// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#define _USE_SCIVI_CONNECTION_ 1
#include "CoreMinimal.h"
#include "VRGameModeWithSciViBase.h"
#include "ReadingTrackerGameMode.generated.h"
//Channel to check collision with 
static const ECollisionChannel Stimulus_Channel = ECollisionChannel::ECC_GameTraceChannel1;
static const ECollisionChannel Floor_Channel = ECollisionChannel::ECC_WorldStatic;

UCLASS(BlueprintType)
class BASEREADINGTRACKER_API UAOI : public UObject
{
	GENERATED_BODY()

public:
	UAOI() = default;
	UAOI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

	int id = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	TArray<FString> audio_desciptions;
	int last_played_description = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FString name = TEXT("no name");
	UPROPERTY()
	TArray<FVector2D> path;//polygon
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	FBox2D bbox;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Transient)
	UTexture2D* image = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
	int32 order = -1;
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
class BASEREADINGTRACKER_API AReadingTrackerGameMode : public AVRGameModeWithSciViBase
{
	GENERATED_BODY()
public:
	AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

	virtual void OnSciViConnected() override 
	{ Super::OnSciViConnected(); StartExperiment(true); }
	virtual void OnSciViDisconnected() override 
	{ Super::OnSciViDisconnected(); FinishExperiment(0, TEXT("SciVi disconnected")); }
	//----------------- Scene ----------------------
public:
	virtual void NotifyStimulustSpawned(class AStimulus* _stimulus);

	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "Stimulus")
		class AStimulus* stimulus;

	//----------------------- SciVi networking --------------
public:
	void SendWallLogToSciVi(EWallLogAction Action, const FString& WallName, int AOI_index = -1, const FString& AOI = TEXT(""));
	void SendGazeToSciVi(const struct FGaze& gaze, FVector2D& uv, int AOI_index, const FString& Id);
protected:
	virtual void OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson) override;
	void ParseNewImage(const TSharedPtr<FJsonObject>& json);
};
