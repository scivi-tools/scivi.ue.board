// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define DEPRECATED
#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#define ASIO_STANDALONE 1
#include "ws/server_ws.hpp"
THIRD_PARTY_INCLUDES_END
#undef UI
#undef ERROR
#undef UpdateResource

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SRanipal_Eyes_Enums.h"
#include "ReadingTrackerGameMode.generated.h"

//Channel to check collision with 
static const ECollisionChannel Stimulus_Channel = ECollisionChannel::ECC_GameTraceChannel1;

USTRUCT()
struct FAOI
{
	GENERATED_BODY()
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

/**
 * 
 */
UCLASS()
class READINGTRACKER_API AReadingTrackerGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int MaxWallsCount = 5;

	UFUNCTION(BlueprintCallable)
	bool RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FHitResult& hitResult);

	//----------------- Scene ----------------------
public:
	void NotifyInformantSpawned(class ABaseInformant* _informant);
	
	UFUNCTION(BlueprintCallable)
	void CreateListOfWords();
	UFUNCTION(BlueprintCallable)
	void ReplaceWalls(float stimulus_remoteness);
	void AddAOIsToList(AWordListWall* const wall);
	UFUNCTION(BlueprintCallable)
	void DeleteList(AWordListWall* const wall);
protected:
	class AStimulus* stimulus;
	class ABaseInformant* informant;
	TArray<class AWordListWall*> walls;
	bool direction_of_search = false;
	int created_walls_count = 0;

	//------------------- VR ----------------------
public:
	UFUNCTION(BlueprintCallable)
	void CalibrateVR();
	UPROPERTY(EditAnywhere)
	SupportedEyeVersion EyeVersion;

//----------------------- SciVi networking --------------
public:
	void Broadcast(FString& message);

protected:
	using WSServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
	void initWS();
	void wsRun() { m_server.start(); }
	void ParseNewImage(const TSharedPtr<FJsonObject>& json);
	WSServer m_server;
	TUniquePtr<std::thread> m_serverThread = nullptr;//you can't use std::thread in UE4, because ue4 can't destroy it then gave is exiting
	TQueue<FString> message_queue;
};
