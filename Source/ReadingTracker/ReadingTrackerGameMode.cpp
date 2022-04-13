// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadingTrackerGameMode.h"
#include "Stimulus.h"
#include "IImageWrapper.h"
#include "Json.h"
#include "Misc/Base64.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"

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
    initWS();
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
                stimulus->customCalib();
            else if (jsonParsed->TryGetField("setMotionControllerVisibility"))
            {
                auto PC = GetWorld()->GetFirstPlayerController();
                bool visibility = jsonParsed->GetBoolField("setMotionControllerVisibility");
                stimulus->toggleMotionController(visibility);
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