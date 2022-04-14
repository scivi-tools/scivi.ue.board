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
	void NotifyStimulusSpawned(class AStimulus* _stimulus);
	void NotifyInformantSpawned(class ABaseInformant* _informant);
	void Broadcast(FString& message);

	AActor* RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FVector& hitPoint);

	void CalibrateVR();
	UPROPERTY(EditAnywhere)
	SupportedEyeVersion EyeVersion;

protected:
	using WSServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
	void initWS();
	void wsRun() { m_server.start(); }
	WSServer m_server;
	std::thread m_serverThread;
	bool server_started = false;
	class AStimulus* stimulus;
	class ABaseInformant* informant;
};
