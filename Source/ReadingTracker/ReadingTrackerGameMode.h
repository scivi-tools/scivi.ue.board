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

/**
 * 
 */
UCLASS()
class READINGTRACKER_API AReadingTrackerGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadonly)
	int MaxWallsCount = 6;

	UFUNCTION(BlueprintCallable)
	AActor* RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FVector& hitPoint);

	//----------------- Scene ----------------------
public:
	void NotifyInformantSpawned(class ABaseInformant* _informant);
	
	UFUNCTION(BlueprintCallable)
	void CreateListOfWords();
	UFUNCTION(BlueprintCallable)
	void DeleteList(AWordListWall* const wall);
	UFUNCTION(BlueprintCallable)
	void ReplaceWalls(float stimulus_remoteness);
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
	WSServer m_server;
	TUniquePtr<std::thread> m_serverThread;//you can't use std::thread in UE4, because ue4 can't destroy it then gave is exiting
	
};
