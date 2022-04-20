// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadingTrackerGameMode.h"
#include "Stimulus.h"
#include "BaseInformant.h"
#include "WordListWall.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Json.h"
#include "Misc/Base64.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"

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
            loadedT2D = UTexture2D::CreateTransient(w, h, PF_B8G8R8A8);
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

//-------------------------- API ------------------------

void AReadingTrackerGameMode::BeginPlay()
{
    auto instance = SRanipalEye_Framework::Instance();
    if (instance)
        instance->StartFramework(EyeVersion);
}

void AReadingTrackerGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    m_server.stop();
    m_serverThread->join();
    m_serverThread.Reset();
    SRanipalEye_Framework::Instance()->StopFramework();
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

// -------------------------- Scene modification ------------------

void AReadingTrackerGameMode::NotifyInformantSpawned(ABaseInformant* _informant)
{
    informant = _informant;
    stimulus = GetWorld()->SpawnActor<AStimulus>(AStimulus::StaticClass());
    stimulus->BindInformant(informant);
    //create walls(but they invisible)
    for (int i = 0; i < MaxWallsCount; i++)
    {
        auto wall = GetWorld()->SpawnActor<AWordListWall>(AWordListWall::StaticClass());
        wall->SetVisibility(false);
        wall->SetWallName(FString::Printf(TEXT("Wall %i"), i + 1));
        walls.Add(wall);
    }
    ReplaceWalls(510.0f);
    initWS();

}

void AReadingTrackerGameMode::CreateListOfWords()
{
    AWordListWall* hidden_wall = *walls.FindByPredicate([](AWordListWall* x) {return x->IsHiddenInGame(); });
    hidden_wall->SetVisibility(true);
}

void AReadingTrackerGameMode::DeleteList(AWordListWall* const wall)
{
    wall->SetVisibility(false);
}

void AReadingTrackerGameMode::ReplaceWalls(float radius)
{
    auto& player_transform = informant->GetTransform();
    auto BB = stimulus->GetComponentsBoundingBox().GetExtent();//x - thickness, y - width, z - height
    float width = 2.0 * BB.Y;// width of one wall
    if (radius < width)
        radius = width;
    float player_Z = player_transform.GetLocation().Z;
    //place stimulus in front of informant
    stimulus->SetActorLocation(player_transform.GetLocation());
    stimulus->AddActorWorldOffset(FVector(0.0f, radius, -player_Z));
    stimulus->SetActorRotation(player_transform.GetRotation());
    stimulus->AddActorWorldRotation(FRotator(0.0f, -180.0f, 0.0f));

    //place other walls around the informant (if walls count is odd, then the last will placed later)
    float angle_per_wall = 2.0f * PI / (float)(MaxWallsCount + 1);
    float angle = PI / 2.0f - angle_per_wall;//initial angle
    for (int i = 0; i < MaxWallsCount / 2; ++i, angle -= angle_per_wall)
    {
        float cos = FMath::Cos(angle) * radius;
        float sin = FMath::Sin(angle) * radius;
        walls[2 * i]->SetActorLocation(player_transform.GetLocation() + FVector(cos, sin, -player_Z));
        walls[2 * i]->SetActorRotation(FRotator(0.0f, 180 + FMath::RadiansToDegrees(angle), 0.0f));
        walls[2 * i]->SetActorScale3D(FVector(1.0f, width / 100.0f, 1.0f));

        walls[2 * i + 1]->SetActorLocation(player_transform.GetLocation() + FVector(-cos, sin, -player_Z));
        walls[2 * i + 1]->SetActorRotation(FRotator(0.0f, -FMath::RadiansToDegrees(angle), 0.0f));
        walls[2 * i + 1]->SetActorScale3D(FVector(1.0f, width / 100.0f, 1.0f));
    }
    //if walls_count is odd, place last wall in back of informant
    if (MaxWallsCount % 2 == 1)
    {
        int i = MaxWallsCount - 1;
        walls[i]->SetActorLocation(player_transform.GetLocation() + FVector(0.0, -radius, -player_Z));
        walls[i]->SetActorRotation(player_transform.GetRotation());
        walls[i]->SetActorScale3D(FVector(1.0f, width / 100.0f, 1.0f));
    }

    //So strange algorithm for placing the walls is for sorting the walls by remoteness from stimulus.
    //Later you can just find first hidden wall(it will be nearest hidden wall to stimulus) and unhide it
}

// ---------------------- VR -------------------------

void AReadingTrackerGameMode::CalibrateVR()
{
    ViveSR::anipal::Eye::LaunchEyeCalibration(nullptr);
}

//---------------------- SciVi communication ------------------

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
                {
                    float sx = jsonParsed->GetNumberField("scaleX");
                    float sy = jsonParsed->GetNumberField("scaleY");
                    UTexture2D* texture = loadTexture2DFromBytes(img, fmt);
                    if (texture)
                        stimulus->updateDynTex(texture, sx , sy, jsonParsed->GetArrayField("AOIs"));
                }
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

    m_serverThread.Reset(new std::thread(&AReadingTrackerGameMode::wsRun, this));
}

void AReadingTrackerGameMode::Broadcast(FString& message)
{
    for (auto& connection : m_server.get_connections())//broadcast to everyone
        connection->send(TCHAR_TO_ANSI(*message));
}
