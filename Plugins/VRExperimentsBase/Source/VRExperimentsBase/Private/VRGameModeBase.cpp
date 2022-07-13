// Fill out your copyright notice in the Description page of Project Settings.


#include "VRGameModeBase.h"
#include "BaseInformant.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"
#include "InteractableActor.h"
#include "Kismet/GameplayStatics.h"//for PredictProjectilePath

AVRGameModeBase::AVRGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bCanEverTick = true;
}

void AVRGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    auto instance = SRanipalEye_Framework::Instance();
    if (instance)
        instance->StartFramework(EyeVersion);
}

void AVRGameModeBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    FString json_text;
    if (message_queue.Dequeue(json_text))
    {
        TSharedPtr<FJsonObject> jsonParsed;
        TSharedRef<TJsonReader<TCHAR>> jsonReader = TJsonReaderFactory<TCHAR>::Create(json_text);
        if (FJsonSerializer::Deserialize(jsonReader, jsonParsed))
        {
            if (jsonParsed->TryGetField("calibrate")) CalibrateVR();
            else if (jsonParsed->TryGetField(TEXT("setExperimentStep")))
            {
                auto step_name = jsonParsed->GetStringField(TEXT("setExperimentStep"));
                if (experiment_step_classes.Contains(step_name))
                {
                    if (IsValid(current_experiment_step))
                    {
                        GetWorld()->RemoveActor(current_experiment_step, true);
                        current_experiment_step->Destroy();//it calls end play and quit step
                    }
                    auto& step_class = experiment_step_classes[step_name];
                    FActorSpawnParameters params;
                    params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
                    params.Name = FName(FString::Printf(TEXT("ExperimentStep_%s"), *step_name));
                    current_experiment_step = GetWorld()->SpawnActor<AExperimentStepBase>(step_class, params);//it calls begin play event and starts step
                }
                else UE_LOG(LogGameMode, Warning, TEXT("New phase with unknown name"));
            }
            else
                OnSciViMessageReceived(jsonParsed);
        }
    }
}

void AVRGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    m_server.stop();
    m_serverThread->join();
    m_serverThread.Reset();
    SRanipalEye_Framework::Instance()->StopFramework();
}

void AVRGameModeBase::NotifyInformantSpawned(ABaseInformant* _informant)
{
    informant = _informant;
}

bool AVRGameModeBase::RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FHitResult& hitResult)
{
    const float ray_thickness = 1.0f;
    FCollisionQueryParams traceParam = FCollisionQueryParams(FName("traceParam"), true, ignoreActor);
    traceParam.bReturnPhysicalMaterial = false;

    if (ray_thickness <= 0.0f)
    {
        return GetWorld()->LineTraceSingleByChannel(hitResult, origin, end,
            EyeTrackingChannel, traceParam);
    }
    else
    {
        FCollisionShape sph = FCollisionShape();
        sph.SetSphere(ray_thickness);
        return GetWorld()->SweepSingleByChannel(hitResult, origin, end, FQuat(0.0f, 0.0f, 0.0f, 0.0f),
            EyeTrackingChannel, sph, traceParam);
    }
}

// ---------------------- VR ------------------------

void AVRGameModeBase::CalibrateVR()
{
    ViveSR::anipal::Eye::LaunchEyeCalibration(nullptr);
    UE_LOG(LogTemp, Display, TEXT("start calibration"));
}

//---------------------- SciVi communication ------------------

void AVRGameModeBase::initWS()
{
    m_server.config.port = 81;

    auto& ep = m_server.endpoint["^/ue4/?$"];

    ep.on_message = [this](std::shared_ptr<WSServer::Connection> connection, std::shared_ptr<WSServer::InMessage> msg)
    {
        auto str = FString(UTF8_TO_TCHAR(msg->string().c_str()));
        message_queue.Enqueue(str);
    };

    ep.on_open = [](std::shared_ptr<WSServer::Connection> connection)
    {
        UE_LOG(LogTemp, Display, TEXT("WebSocket: Opened"));
    };

    ep.on_close = [](std::shared_ptr<WSServer::Connection> connection, int status, const std::string&)
    {
        UE_LOG(LogTemp, Display, TEXT("WebSocket: Closed"));
    };

    ep.on_handshake = [](std::shared_ptr<WSServer::Connection>, SimpleWeb::CaseInsensitiveMultimap&)
    {
        return SimpleWeb::StatusCode::information_switching_protocols;
    };

    ep.on_error = [](std::shared_ptr<WSServer::Connection> connection, const SimpleWeb::error_code& ec)
    {
        UE_LOG(LogTemp, Warning, TEXT("WebSocket: Error"));
    };

    m_serverThread = MakeUnique<std::thread>(&AVRGameModeBase::wsRun, this);
}

void AVRGameModeBase::SendToSciVi(FString& message)
{
    auto t = FDateTime::Now();
    int64 time = t.ToUnixTimestamp() * 1000 + t.GetMillisecond();
    auto msg = FString::Printf(TEXT("{\"Time\": %llu, %s}"), time, *message);
    for (auto& connection : m_server.get_connections())//broadcast to everyone
        connection->send(TCHAR_TO_UTF8(*msg));
}

void AVRGameModeBase::OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson)
{

}
