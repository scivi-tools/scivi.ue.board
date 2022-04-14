// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadingTrackerGameMode.h"
#include "Stimulus.h"
#include "IImageWrapper.h"
#include "Json.h"
#include "Misc/Base64.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"
#include "BaseInformant.h"

void AReadingTrackerGameMode::BeginPlay()
{
    auto instance = SRanipalEye_Framework::Instance();
    if (instance)
        instance->StartFramework(EyeVersion);
}

void AReadingTrackerGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    m_server.stop();
    m_serverThread.join();
    SRanipalEye_Framework::Instance()->StopFramework();
}

void AReadingTrackerGameMode::NotifyStimulusSpawned(AStimulus* _stimulus)
{
    stimulus = _stimulus;
    if (stimulus && informant && !server_started) 
    {
        initWS();
    }
}

void AReadingTrackerGameMode::NotifyInformantSpawned(ABaseInformant* _informant)
{
    informant = _informant;
    if (stimulus && informant && !server_started) {
        initWS();
    }
}

void AReadingTrackerGameMode::initWS()
{
    m_server.config.port = 81;

    auto& ep = m_server.endpoint["^/ue4/?$"];

    ep.on_message = [this](std::shared_ptr<WSServer::Connection> connection, std::shared_ptr<WSServer::InMessage> msg)
    {
        auto text = msg->string();
        TSharedPtr<FJsonObject> jsonParsed;
        TSharedRef<TJsonReader<TCHAR>> jsonReader = TJsonReaderFactory<TCHAR>::Create(text.c_str());
        if (FJsonSerializer::Deserialize(jsonReader, jsonParsed))
        {
            if (jsonParsed->TryGetField("calibrate"))
                CalibrateVR();
            else if (jsonParsed->TryGetField("customCalibrate"))
                stimulus->customCalibrate();
            else if (jsonParsed->TryGetField("setMotionControllerVisibility"))
            {
                auto PC = GetWorld()->GetFirstPlayerController();
                bool visibility = jsonParsed->GetBoolField("setMotionControllerVisibility");
                informant->ToggleMotionController(visibility);
            }
            else
            {
                FString image = jsonParsed->GetStringField("image");
                TArray<uint8> img;
                FString png = "data:image/png;base64,";
                FString jpg = "data:image/jpeg;base64,";
                EImageFormat fmt = EImageFormat::Invalid;
                int startPos = 0;
                if (image.StartsWith(png))
                {
                    fmt = EImageFormat::PNG;
                    startPos = png.Len();
                }
                else if (image.StartsWith(jpg))
                {
                    fmt = EImageFormat::JPEG;
                    startPos = jpg.Len();
                }
                if (fmt != EImageFormat::Invalid &&
                    FBase64::Decode(&(image.GetCharArray()[startPos]), img))
                    stimulus->updateDynTex(img, fmt, jsonParsed->GetNumberField("scaleX"), jsonParsed->GetNumberField("scaleY"), jsonParsed->GetArrayField("AOIs"));
            }
        }
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

    m_serverThread = std::thread(&AReadingTrackerGameMode::wsRun, this);
    server_started = true;
}

void AReadingTrackerGameMode::CalibrateVR()
{
    ViveSR::anipal::Eye::LaunchEyeCalibration(nullptr);
}

void AReadingTrackerGameMode::Broadcast(FString& message)
{
    for (auto& connection : m_server.get_connections())//broadcast to everyone
        connection->send(TCHAR_TO_ANSI(*message));
}

AActor* AReadingTrackerGameMode::RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FVector& hitPoint)
{
    const float ray_thickness = 1.0f;
    FCollisionQueryParams traceParam = FCollisionQueryParams(FName("traceParam"), true, ignoreActor);
    traceParam.bTraceComplex = true;
    traceParam.bReturnPhysicalMaterial = false;
    FHitResult hitResult(ForceInit);
    bool is_collided;

    if (ray_thickness <= 0.0f)
    {
        is_collided = GetWorld()->LineTraceSingleByChannel(hitResult, origin, end,
            ECollisionChannel::ECC_WorldStatic, traceParam);
    }
    else
    {
        FCollisionShape sph = FCollisionShape();
        sph.SetSphere(ray_thickness);
        is_collided = GetWorld()->SweepSingleByChannel(hitResult, origin, end, FQuat(0.0f, 0.0f, 0.0f, 0.0f),
            ECollisionChannel::ECC_WorldStatic, sph, traceParam);
    }

    if (is_collided) {
        hitPoint = hitResult.Location;
        return hitResult.Actor.Get();
    }
    return nullptr;
}

