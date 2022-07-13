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
#include "ExperimentStepBase.h"
#include "VRGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class VREXPERIMENTSBASE_API AVRGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AVRGameModeBase(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "Informant")
	class ABaseInformant* informant;
	UPROPERTY(EditAnywhere, BlueprintReadwrite)
	TMap<FString, TSubclassOf<AExperimentStepBase>> experiment_step_classes;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	TEnumAsByte<ECollisionChannel> EyeTrackingChannel = ECollisionChannel::ECC_GameTraceChannel1;

	virtual void NotifyInformantSpawned(class ABaseInformant* _informant);
	UFUNCTION(BlueprintCallable)
	bool RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FHitResult& hitResult);
	

protected:
	UPROPERTY()
	AExperimentStepBase* current_experiment_step = nullptr;

	//------------------- VR ----------------------
public:
	UFUNCTION(BlueprintCallable)
	void CalibrateVR();

	UPROPERTY(EditAnywhere)
	SupportedEyeVersion EyeVersion;

	// ----------------------- SciVi networking--------------
public:
	UFUNCTION(BlueprintCallable)
	void SendToSciVi(FString& message);
protected:
	virtual void OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson);
	using WSServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
	void initWS();
	void wsRun() { m_server.start(); }
	WSServer m_server;
	TUniquePtr<std::thread> m_serverThread = nullptr;//you can't use std::thread in UE4, because ue4 can't destroy it then game is exiting
	TQueue<FString> message_queue;
	
};
