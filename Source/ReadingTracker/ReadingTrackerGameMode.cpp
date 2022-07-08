// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadingTrackerGameMode.h"
#include "Stimulus.h"
#include "BaseInformant.h"
#include "Private/UI_Blank.h"
#include "Private/ExperimentStepBase.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/WidgetComponent.h"
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
            loadedT2D = UTexture2D::CreateTransient(w, h);
            loadedT2D->AddToRoot();
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

void CopyTexture2DFragment(UTexture2D* destination, const UTexture2D* source, int start_x, int start_y, int width, int height)
{
    auto dst_size = FVector2D(destination->GetSizeX(), destination->GetSizeY());
    auto &src_bulk = source->PlatformData->Mips[0].BulkData;
    auto &dst_bulk = destination->PlatformData->Mips[0].BulkData;

    auto src_pixels = reinterpret_cast<const FColor*>(src_bulk.LockReadOnly());
    auto dst_pixels = reinterpret_cast<FColor*>(dst_bulk.Lock(LOCK_READ_WRITE));
    for (int y = 0; y < height; ++y)
        FMemory::Memcpy(dst_pixels + y * destination->GetSizeX(),
                    src_pixels + (y + start_y) * source->GetSizeX() + start_x,
                    width * sizeof(FColor));
    src_bulk.Unlock();
    dst_bulk.Unlock();
    destination->UpdateResource();
}

//-------------------------- API ------------------------

AReadingTrackerGameMode::AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bCanEverTick = true;
    static ConstructorHelpers::FClassFinder<UUserWidget> RecordingWidgetClass(TEXT("/Game/UI/UI_RecordVoice"));
    static ConstructorHelpers::FClassFinder<UUserWidget> CreateListWidgetClass(TEXT("/Game/UI/UI_CreateList_Menu"));
    static ConstructorHelpers::FClassFinder<AExperimentStepBase> NullStepClass(TEXT("/Game/Steps/NullStep"));
    RecordingMenuClass = RecordingWidgetClass.Class;
    CreateListButtonClass = CreateListWidgetClass.Class;
    experiment_step_classes.Add(TEXT("NullStep"), NullStepClass.Class);
}

void AReadingTrackerGameMode::BeginPlay()
{
    Super::BeginPlay();
    auto instance = SRanipalEye_Framework::Instance();
    if (instance)
        instance->StartFramework(EyeVersion);

}

void AReadingTrackerGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    FString json_text;
    if (message_queue.Dequeue(json_text))
    {
        TSharedPtr<FJsonObject> jsonParsed;
        TSharedRef<TJsonReader<TCHAR>> jsonReader = TJsonReaderFactory<TCHAR>::Create(json_text);
        if (FJsonSerializer::Deserialize(jsonReader, jsonParsed))
        {
            if (jsonParsed->TryGetField("calibrate")) {
                UE_LOG(LogTemp, Display, TEXT("start calibration"));
                CalibrateVR();
            }
            else if (jsonParsed->TryGetField("customCalibrate"))
                stimulus->customCalibrate();
            //else if (jsonParsed->TryGetField("setMotionControllerVisibility"))
            //{
            //    auto PC = GetWorld()->GetFirstPlayerController();
            //    bool visibility = jsonParsed->GetBoolField("setMotionControllerVisibility");
            //    //informant->SciVi_MCLeft_Visibility = visibility;
            //    informant->SciVi_MCRight_Visibility = visibility;
            //}
            else if (jsonParsed->TryGetField("Speech"))
            {
                auto speech = jsonParsed->GetStringField(TEXT("Speech"));
                auto root = uiRecordingMenu->GetWidget();
                auto textWidget = Cast<UEditableText>(root->GetWidgetFromName("textNewName"));
                if (textWidget)
                {
                    auto name = textWidget->GetText().ToString();
                    name.Appendf(TEXT(" %s"), *speech);
                    textWidget->SetText(FText::FromString(name));
                }
            }
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
                    auto &step_class = experiment_step_classes[step_name];
                    FActorSpawnParameters params;
                    params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
                    params.Name = FName(FString::Printf(TEXT("ExperimentStep_%s"), *step_name));
                    current_experiment_step = GetWorld()->SpawnActor<AExperimentStepBase>(step_class, params);//it calls begin play event and starts step
                }
                else UE_LOG(LogGameMode, Warning, TEXT("New phase with unknown name"));
            }
            else {
                for (auto wall : walls) 
                {
                    wall->SetVisibility(false);
                    wall->ClearList();
                }
                ReplaceObjectsOnScene(StimulusRemoteness);
                ParseNewImage(jsonParsed);
            }
        }

    }


}

void AReadingTrackerGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    m_server.stop();
    m_serverThread->join();
    m_serverThread.Reset();
    SRanipalEye_Framework::Instance()->StopFramework();
}

bool AReadingTrackerGameMode::RayTrace(const AActor* ignoreActor, const FVector& origin, const FVector& end, FHitResult& hitResult)
{
    const float ray_thickness = 1.0f;
    FCollisionQueryParams traceParam = FCollisionQueryParams(FName("traceParam"), true, ignoreActor);
    traceParam.bReturnPhysicalMaterial = false;

    if (ray_thickness <= 0.0f)
    {
        return GetWorld()->LineTraceSingleByChannel(hitResult, origin, end,
            Stimulus_Channel, traceParam);
    }
    else
    {
        FCollisionShape sph = FCollisionShape();
        sph.SetSphere(ray_thickness);
        return GetWorld()->SweepSingleByChannel(hitResult, origin, end, FQuat(0.0f, 0.0f, 0.0f, 0.0f),
            Stimulus_Channel, sph, traceParam);
    }
}

// -------------------------- Scene modification ------------------

void AReadingTrackerGameMode::NotifyInformantSpawned(ABaseInformant* _informant)
{
    informant = _informant;
    stimulus = GetWorld()->SpawnActor<AStimulus>(AStimulus::StaticClass());
    stimulus->BindInformant(informant);
    //spawn recording menu
    {
        FActorSpawnParameters params;
        params.Name = FName(TEXT("RecordingMenu"));
        uiRecordingMenu = GetWorld()->SpawnActor<AUI_Blank>(AUI_Blank::StaticClass(), params);
        uiRecordingMenu->SetWidgetClass(RecordingMenuClass, FVector2D(1000.0f, 750.0f));
        auto root = uiRecordingMenu->GetWidget();
        auto btnReady = Cast<UButton>(root->GetWidgetFromName(TEXT("btnReady")));
        btnReady->OnClicked.AddDynamic(this, &AReadingTrackerGameMode::RecordingMenu_CreateList);
        auto btnClear = Cast<UButton>(root->GetWidgetFromName(TEXT("btnClear")));
        btnClear->OnClicked.AddDynamic(this, &AReadingTrackerGameMode::RecordingMenu_ClearNameForWall);

        auto btnRecord = Cast<UButton>(root->GetWidgetFromName(TEXT("btnRecord")));
        btnRecord->OnPressed.AddDynamic(informant, &ABaseInformant::StartRecording);
        btnRecord->OnReleased.AddDynamic(informant, &ABaseInformant::StopRecording);
        SetRecordingMenuVisibility(false);//hide menu by default
    }
    //spawn CreateList Button
    {
        FActorSpawnParameters params;
        params.Name = FName(TEXT("CreateListButton"));
        uiCreateListButton = GetWorld()->SpawnActor<AUI_Blank>(AUI_Blank::StaticClass(), params);
        uiCreateListButton->SetWidgetClass(CreateListButtonClass, FVector2D(500.0f, 75.0f));
        auto root = uiCreateListButton->GetWidget();
        auto btn = Cast<UButton>(root->GetWidgetFromName(TEXT("btnCreateList")));
        btn->OnClicked.AddDynamic(this, &AReadingTrackerGameMode::CreateListBtn_OnClicked);
    }

    //create walls(but they invisible)
    for (int i = 0; i < MaxWallsCount; i++)
    {
        auto wall_name = FString::Printf(TEXT("Wall %i"), i + 1);
        FActorSpawnParameters params;
        params.Name = FName(wall_name);
        auto wall = GetWorld()->SpawnActor<AWordListWall>(AWordListWall::StaticClass(), params);
        wall->SetVisibility(false);
        wall->SetWallName(wall_name);
        walls.Add(wall);
    }
    ReplaceObjectsOnScene(StimulusRemoteness);
    initWS();
}

void AReadingTrackerGameMode::ReplaceObjectsOnScene(float radius)
{
    auto& player_transform = informant->GetTransform();
    float player_Z = player_transform.GetLocation().Z; //player_height
    float camera_Z = informant->CameraComponent->GetComponentLocation().Z;//camera height
    //place stimulus in front of informant
    stimulus->SetActorLocation(player_transform.GetLocation());
    stimulus->AddActorWorldOffset(FVector(0.0f, radius, -player_Z));
    stimulus->SetActorRotation(player_transform.GetRotation());
    stimulus->AddActorWorldRotation(FRotator(0.0f, -180.0f, 0.0f));
    //Place RecordingMenu
    uiRecordingMenu->SetActorScale3D(FVector(0.15f));
    uiRecordingMenu->SetActorLocation(player_transform.GetLocation());
    uiRecordingMenu->AddActorWorldOffset(FVector(0.0f, RecordingMenuRemoteness, camera_Z - player_Z));
    uiRecordingMenu->SetActorRotation(player_transform.GetRotation());
    uiRecordingMenu->AddActorWorldRotation(FRotator(0.0f, -180.0f, 0.0f));
    //Place CreateList Button
    uiCreateListButton->SetActorLocation(stimulus->GetActorLocation());
    uiCreateListButton->AddActorWorldOffset(FVector(0.0f, -10.0f, 345.0f));
    uiCreateListButton->SetActorRotation(FRotator(-10.0f, 270.0f, 0.0f));

    //it gets a BBox considering an object's rotation
    auto BB = stimulus->GetComponentsBoundingBox().GetExtent();//x - width, y - thickness, z - height

    int n = 12;//2 * MaxWallsCount - 2;
    //place other walls around the informant
    float angle_per_wall = 2.0f * PI / (float)n;
    float angle = PI / 6.0f;
    for (int i = 0; i < MaxWallsCount / 2; ++i, angle -= angle_per_wall)
    {
        float x_offset = FMath::Cos(angle) * radius;
        float y_offset = FMath::Sin(angle) * radius + radius / 2.0f;
        walls[2 * i]->SetActorLocation(player_transform.GetLocation() + FVector(x_offset, y_offset, -player_Z));
        walls[2 * i]->SetActorRotation(FRotator(0.0f, 180 + FMath::RadiansToDegrees(angle), 0.0f));

        walls[2 * i + 1]->SetActorLocation(player_transform.GetLocation() + FVector(-x_offset, y_offset, -player_Z));
        walls[2 * i + 1]->SetActorRotation(FRotator(0.0f, -FMath::RadiansToDegrees(angle), 0.0f));
    }

    if (MaxWallsCount % 2 == 1)
    {
        int i = MaxWallsCount - 1;
        walls[i]->SetActorLocation(player_transform.GetLocation() + FVector(0.0f, -radius / 2.0f, -player_Z));
        walls[i]->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
    }

    //So strange algorithm for placing the walls is for sorting the walls by remoteness from stimulus.
    //Later you can just find first hidden wall(it will be nearest hidden wall to stimulus) and unhide it
}

//------------- Recording Menu -------------------
void AReadingTrackerGameMode::SetRecordingMenuVisibility(bool new_visibility)
{
    //replace recording menu according camera pos
    auto& player_transform = informant->GetTransform();
    float player_Z = player_transform.GetLocation().Z; //player_height
    float camera_Z = informant->CameraComponent->GetComponentLocation().Z;//camera height
    uiRecordingMenu->SetActorLocation(player_transform.GetLocation());
    uiRecordingMenu->AddActorWorldOffset(FVector(0.0f, RecordingMenuRemoteness, camera_Z - player_Z));

    uiRecordingMenu->SetVisibility(new_visibility);
    RecordingMenu_ClearNameForWall();
}

void AReadingTrackerGameMode::RecordingMenu_ClearNameForWall()
{
    auto root = uiRecordingMenu->GetWidget();
    auto textBlock = Cast<UEditableText>(root->GetWidgetFromName(TEXT("textNewName")));
    textBlock->SetText(FText::FromString(TEXT("")));
}

void AReadingTrackerGameMode::RecordingMenu_CreateList()
{
    auto root = uiRecordingMenu->GetWidget();
    auto textBlock = Cast<UEditableText>(root->GetWidgetFromName(TEXT("textNewName")));
    CreateListOfWords(textBlock->GetText().ToString());
}

//------------- List of words -------------------
void AReadingTrackerGameMode::CreateListOfWords(const FString& wall_name)
{
    int visible_count = 0;
    for (auto wall : walls)
    {
        if (wall->IsHiddenInGame())
        {
            wall->SetVisibility(true);
            wall->SetWallName(wall_name);
            SendWallLogToSciVi(EWallLogAction::NewWall, wall_name);
            break;
        }
        else ++visible_count;
    }
    SetRecordingMenuVisibility(false);

    if (visible_count == MaxWallsCount - 1)
        uiCreateListButton->SetEnabled(false);
}

void AReadingTrackerGameMode::DeleteList(AWordListWall* const wall)
{
    if (IsValid(wall))
    {
        wall->SetVisibility(false);
        wall->ClearList();
        SendWallLogToSciVi(EWallLogAction::DeleteWall, wall->GetWallName());
        uiCreateListButton->SetEnabled(true);
    }
}

void AReadingTrackerGameMode::DeleteAllLists()
{
    for (auto wall : walls)
        if (!wall->IsHiddenInGame())
            DeleteList(wall);
}

void AReadingTrackerGameMode::AddAOIsToList(AWordListWall* const wall)
{
    for (auto aoi : stimulus->SelectedAOIs) 
    {
        wall->AddAOI(aoi);
    }
    //clear selection on stimulus
    stimulus->ClearSelectedAOIs();  
}

//----------- CreateList Button ------------
void AReadingTrackerGameMode::CreateListBtn_OnClicked()
{
    SetRecordingMenuVisibility(true);
}

// ---------------------- VR ------------------------

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

    m_serverThread = MakeUnique<std::thread>(&AReadingTrackerGameMode::wsRun, this);
}

void AReadingTrackerGameMode::ParseNewImage(const TSharedPtr<FJsonObject>& json)
{
    FString image_textdata = json->GetStringField("image");
    TArray<uint8> img;
    FString png = "data:image/png;base64,";
    FString jpg = "data:image/jpeg;base64,";
    EImageFormat fmt = EImageFormat::Invalid;
    int startPos = 0;
    if (image_textdata.StartsWith(png))
    {
        fmt = EImageFormat::PNG;
        startPos = png.Len();
    }
    else if (image_textdata.StartsWith(jpg))
    {
        fmt = EImageFormat::JPEG;
        startPos = jpg.Len();
    }

    if (fmt != EImageFormat::Invalid &&
        FBase64::Decode(&(image_textdata.GetCharArray()[startPos]), img))
    {
        float sx = json->GetNumberField("scaleX");
        float sy = json->GetNumberField("scaleY");
        UTexture2D* texture = loadTexture2DFromBytes(img, fmt);
        TArray<FAOI> AOIs;
        TMap<FString, int> counts;
        int last_id = 0;
        for (auto &aoi_text : json->GetArrayField("AOIs"))
        {
            FAOI aoi;
            aoi.id = last_id++;
            auto nameField = aoi_text->AsObject()->TryGetField("name");
            if (nameField) 
            {
                auto name_str = nameField->AsString();
                if (counts.Contains(name_str)) counts[name_str]++;
                else counts.Add(name_str, 0);
                aoi.name = FString::Printf(TEXT("%s_%i"), *name_str, counts[name_str]);
            }
            auto pathField = aoi_text->AsObject()->TryGetField("path");
            if (pathField)
            {
                auto &path = pathField->AsArray();
                for (auto &point : path)
                    aoi.path.Add(FVector2D(point->AsArray()[0]->AsNumber(), point->AsArray()[1]->AsNumber()));
            }
            auto bboxField = aoi_text->AsObject()->TryGetField("bbox");
            if (bboxField)
            {
                auto &bbox = bboxField->AsArray();
                aoi.bbox = FBox2D(FVector2D(bbox[0]->AsNumber(), bbox[1]->AsNumber()),
                    FVector2D(bbox[2]->AsNumber(), bbox[3]->AsNumber()));
            }
            auto size = aoi.bbox.GetSize();
            auto start = aoi.bbox.Min;
            aoi.image = UTexture2D::CreateTransient(size.X, size.Y);
            aoi.image->AddToRoot();
            CopyTexture2DFragment(aoi.image, texture, start.X, start.Y, size.X, size.Y);
            AOIs.Add(aoi);
        }
        if (IsValid(texture))
            stimulus->UpdateStimulus(texture, sx, sy, AOIs, true);
    }
}

void AReadingTrackerGameMode::Broadcast(FString& message)
{
    auto t = FDateTime::Now();
    int64 time = t.ToUnixTimestamp() * 1000 + t.GetMillisecond();
    auto msg = FString::Printf(TEXT("{\"Time\": %llu, %s}"), time, *message);
    for (auto& connection : m_server.get_connections())//broadcast to everyone
        connection->send(TCHAR_TO_UTF8(*msg));
}

void AReadingTrackerGameMode::SendWallLogToSciVi(EWallLogAction Action, const FString& WallName, const FString& AOI)
{
    FString msg;
    FString ActionStr;
    switch (Action)
    {
    case EWallLogAction::NewWall: ActionStr = TEXT("NewWall"); break;
    case EWallLogAction::DeleteWall: ActionStr = TEXT("DeleteWall"); break;
    case EWallLogAction::AddAOI: ActionStr = TEXT("AddAOI"); break;
    case EWallLogAction::RemoveAOI: ActionStr = TEXT("RemoveAOI"); break;
    default: ActionStr = TEXT("UnknownAction"); break;
    }
    if (Action == EWallLogAction::NewWall || Action == EWallLogAction::DeleteWall)
    {
        msg = FString::Printf(TEXT("\"WallLog\": {"
            "\"Action\": \"%s\","
            "\"Wall\": \"%s\"}"), *ActionStr, *WallName);
    }
    else
    {
        msg = FString::Printf(TEXT("\"WallLog\": {"
            "\"Action\": \"%s\","
            "\"Wall\": \"%s\","
            "\"AOI\": \"%s\"}"), *ActionStr, *WallName, *AOI);
    }
    Broadcast(msg);
}

void AReadingTrackerGameMode::SendGazeToSciVi(const FGaze& gaze, FVector2D& uv, int AOI_index, const FString& Id)
{
    //Send message to scivi
    auto json = FString::Printf(TEXT("\"Gaze\": {\"uv\": [%f, %f],"
        "\"origin\": [%f, %f, %f],"
        "\"direction\": [%F, %F, %F],"
        "\"lpdmm\": %F, \"rpdmm\": %F,"
        "\"cf\": %F, \"AOI_index\": %i,"
        "\"Action\": \"%s\"}"),
        uv.X, uv.Y,
        gaze.origin.X, gaze.origin.Y, gaze.origin.Z,
        gaze.direction.X, gaze.direction.Y, gaze.direction.Z,
        gaze.left_pupil_diameter_mm, gaze.right_pupil_diameter_mm, gaze.cf, AOI_index, *Id);
    Broadcast(json);
}
