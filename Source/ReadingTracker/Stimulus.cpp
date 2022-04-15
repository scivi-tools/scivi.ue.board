// Fill out your copyright notice in the Description page of Project Settings.

#include "Stimulus.h"
#include "ReadingTrackerGameMode.h"
#include "BaseInformant.h"

#include "SRanipalEye_FunctionLibrary.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"
#include "SRanipalEye_Core.h"
#include "IXRTrackingSystem.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "ImageUtils.h"
#include "IImageWrapperModule.h"
#include "Misc/Base64.h"
#include "GenericPlatform/GenericPlatformMath.h"

//custom calibration
static const constexpr int TARGET_MAX_RADIUS = 15;
static const constexpr int TARGET_MIN_RADIUS = 7;
static const constexpr int POINTS_PER_ROW = 3;
static const constexpr int ROWS_IN_PATTERN = 3;
static const constexpr int SAMPLES_TO_START = 180;     // 2s
static const constexpr int SAMPLES_TO_START_MOVE = 22; // 0.25s
static const constexpr int SAMPLES_TO_DECREASE = 90;   // 1s
static const constexpr int SAMPLES_TO_MOVE = 10;       // 110ms
static const constexpr int SAMPLES_TO_REJECT = 45;
static const constexpr float START_POSITION = 0.05f;
static const constexpr float END_POSITION = 0.95f;
static const constexpr float CENTER_POSTION = 0.5f;
static const constexpr float OUTLIER_THRESHOLD = 3.0f;
static const constexpr float MAX_DISTANCE = 1000.0f;
static const constexpr float HIT_RADIUS = 1.0f;
static const constexpr float EPSILON = 1.0e-5f;

//math functions
//lerp - use FMath::Lerp
float map(float v, float fromMin, float fromMax, float toMin, float toMax)
{
    return toMin + (v - fromMin) / (fromMax - fromMin) * (toMax - toMin);
};
float theta(const FVector& gaze)
{
    return fabs(gaze.Y) < EPSILON && fabs(gaze.Z) < EPSILON ? NAN : atan2(gaze.Y, gaze.Z) + PI;
};
float radius2(const FVector& gaze)
{
    return gaze.Y * gaze.Y + gaze.Z * gaze.Z;
};
int signum(float a, float b)
{
    return FGenericPlatformMath::IsNaN(a) || FGenericPlatformMath::IsNaN(b) || fabs(a - b) < EPSILON ? 0 : a - b > 0.0f ? 1 : -1;
};

FVector2D posForIdx(int idx)
{
    /*if ((idx / POINTS_PER_ROW) % 2)
        idx = (idx / POINTS_PER_ROW) * POINTS_PER_ROW + POINTS_PER_ROW - (idx % POINTS_PER_ROW) - 1;*/
    return FVector2D((float)(idx % POINTS_PER_ROW) * (END_POSITION - START_POSITION) / (float)(POINTS_PER_ROW - 1) + START_POSITION,
        (float)(idx / POINTS_PER_ROW) * (END_POSITION - START_POSITION) / (float)(ROWS_IN_PATTERN - 1) + START_POSITION);
}

//---------------------- API --------------------------

AStimulus::AStimulus()
{
    PrimaryActorTick.bCanEverTick = true;
    
    mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = mesh;

    m_aspect = 1.0f;
    m_scaleX = 1.0f;
    m_scaleY = 1.0f;
    m_dynTexW = 0;
    m_dynTexH = 0;
    m_needsUpdate = false;
    m_calibIndex = 0;
    
    m_stimulusW = 0;
    m_stimulusH = 0;
    m_activeAOI = nullptr;

    m_needsCustomCalib = false;
    m_customCalibSamples = 0;
}

void AStimulus::BeginPlay()
{
    Super::BeginPlay();
    auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
    if (GM)
        GM->NotifyStimulusSpawned(this);
    m_dynTex = mesh->CreateAndSetMaterialInstanceDynamic(0);
}

void AStimulus::EndPlay(const EEndPlayReason::Type reason)
{
    if (m_dynContour)
        m_dynContour->OnCanvasRenderTargetUpdate.Clear();
}

void AStimulus::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    //update texture, thouth it can be in UpdateDynTex
    if (m_needsUpdate)
    {
        FScopeLock lock(&m_mutex);
        SetActorScale3D(FVector(m_aspect * 1.218 * m_scaleX, 1.218 * m_scaleY, 1.0));
        m_dynContour = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetWorld(), UCanvasRenderTarget2D::StaticClass(), m_dynTexW, m_dynTexH);
        m_dynContour->ClearColor = FLinearColor(0, 0, 0, 0);
        m_dynContour->OnCanvasRenderTargetUpdate.AddDynamic(this, &AStimulus::drawContour);
        m_dynTex->SetTextureParameterValue(FName(TEXT("ContourTex")), m_dynContour);
        m_stimulusW = m_dynTexW;
        m_stimulusH = m_dynTexH;
        m_aois = m_dynAOIs;
        m_activeAOI = nullptr;
        m_selectedAOIs.Empty();
        m_dynContour->UpdateResource();
        m_staticTransform = GetTransform();
        m_staticExtent = mesh->CalcLocalBounds().BoxExtent;
        m_needsUpdate = false;
        OnImageUpdated();
    }
}

void AStimulus::updateDynTex(const TArray<uint8>& img, EImageFormat fmt, float sx, float sy, const TArray<TSharedPtr<FJsonValue>>& aois)
{
    int w, h;
    UTexture2D* tex = loadTexture2DFromBytes(img, fmt, w, h);
    if (tex)
    {
        m_dynTex->SetTextureParameterValue(FName(TEXT("DynTex")), tex);
        {
            FScopeLock lock(&m_mutex);//чтобы пока обновл€ем текстуру не вызвалс€ в тике Update
            m_aspect = (float)w / (float)h;
            m_scaleX = sx;
            m_scaleY = sy;
            m_dynTexW = w;
            m_dynTexH = h;
            m_dynAOIs.Empty();
            for (auto value : aois)
            {
                AOI aoi;
                auto nameField = value->AsObject()->TryGetField("name");
                if (nameField)
                    aoi.name = nameField->AsString();
                auto pathField = value->AsObject()->TryGetField("path");
                if (pathField)
                {
                    auto path = pathField->AsArray();
                    for (auto point : path)
                        aoi.path.Add(FVector2D(point->AsArray()[0]->AsNumber(), point->AsArray()[1]->AsNumber()));
                }
                auto bboxField = value->AsObject()->TryGetField("bbox");
                if (bboxField)
                {
                    auto bbox = bboxField->AsArray();
                    aoi.bbox.lt = FVector2D(bbox[0]->AsNumber(), bbox[1]->AsNumber());
                    aoi.bbox.rb = FVector2D(bbox[2]->AsNumber(), bbox[3]->AsNumber());
                }
                m_dynAOIs.Add(aoi);
            }
            m_needsUpdate = true;
        }
    }
}

void AStimulus::BindInformant(ABaseInformant* _informant)
{
    informant = _informant;
}

void AStimulus::OnInFocus(const FGaze& gaze, const FVector& FocusPoint)
{
    FVector2D uv = sceneToBillboard(FocusPoint);
    int currentAOIIndex = -1;
    if (!informant->MC_Right->bHiddenInGame)
        AOI* lookedAOI = findActiveAOI(FVector2D(uv.X * m_stimulusW, uv.Y * m_stimulusH), currentAOIIndex);
    SendDataToSciVi(gaze, uv, currentAOIIndex, TEXT("LOOKAT"));
}

void AStimulus::OnTriggerPressed(const FGaze& gaze, const FVector& FocusPoint)
{
    if (!informant->MC_Right->bHiddenInGame)
    {
        m_laser = sceneToBillboard(FocusPoint);
        if (m_dynContour)
            m_dynContour->UpdateResource();
    }
}

void AStimulus::OnTriggerReleased(const FGaze& gaze, const FVector& FocusPoint)
{
#ifdef COLLECCT_ANGULAR_ERROR
#ifdef MEASURE_ANGULAR_SIZES
       static FQuat prevQ;
        static bool measure = false;
        FQuat q = informant->CameraComponent->GetComponentRotation().Quaternion();
        if (measure)
        {
            FString s = FString::Printf(TEXT(">>>>>>> %f"), prevQ.AngularDistance(q));
            UE_LOG(LogTemp, Warning, TEXT("%s"), *s);
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, s);
        }
        else
        {
            prevQ = q;
        }
        measure = !measure;
#endif // MEASURE_ANGULAR_SIZES
        static int kk = 0;
        const int kn = 5;
        FVector rgt = billboardToScene(FVector2D((kk % kn) * 0.9f / (kn - 1) + 0.05f, (kk / kn) * 0.9f / (kn - 1) + 0.05f));
        FVector rg = rgt - gazeOrigin;
        rg.Normalize();
        FVector g = correctedGazeTarget - gazeOrigin;
        g.Normalize();
        leftPupilDiam = FMath::RadiansToDegrees(acosf(FVector::DotProduct(g, rg)));
        g = rawGazeTarget - gazeOrigin;
        g.Normalize();
        rightPupilDiam = FMath::RadiansToDegrees(acosf(FVector::DotProduct(g, rg)));
        gazeOrigin.X = m_rawTarget.X;
        gazeOrigin.Y = m_rawTarget.Y;
        ++kk;
        if (kk == kn * kn)
            kk = 0;
#endif // COLLECCT_ANGULAR_ERROR

    m_laser = sceneToBillboard(FocusPoint);
    int currentAOIIndex = -1;
    if (!informant->MC_Right->bHiddenInGame)
    {
        AOI* selectedAOI = findActiveAOI(FVector2D(m_laser.X * m_stimulusW, m_laser.Y * m_stimulusH), currentAOIIndex);
        if (selectedAOI)
        {
            toggleSelectedAOI(selectedAOI);
            SendDataToSciVi(gaze, m_laser, currentAOIIndex, TEXT("SELECT"));
        }
    }
    if (m_dynContour)
        m_dynContour->UpdateResource();
    SendDataToSciVi(gaze, m_laser, currentAOIIndex, TEXT("R_RELD"));
}

void AStimulus::OnImageUpdated()
{
    auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
    FGaze gaze;
    FVector hitPoint;
    informant->GetGaze(gaze);
    const float ray_radius = 1.0f;
    const float ray_length = 1000.0f;
    AStimulus* focusedStimulus = Cast<AStimulus>(GM->RayTrace(informant, gaze.origin,
        gaze.origin + gaze.direction * ray_length, hitPoint));
    if (focusedStimulus && focusedStimulus == this)
    {
        FVector2D uv = sceneToBillboard(hitPoint);
        int currentAOIIndex = -1;
        if (informant->MC_Right->bHiddenInGame)
            AOI* lookedAOI = findActiveAOI(FVector2D(uv.X * m_stimulusW, uv.Y * m_stimulusH), currentAOIIndex);
        SendDataToSciVi(gaze, uv, currentAOIIndex, TEXT("IMG_UP"));
    }
}

void AStimulus::customCalibrate()
{
    m_needsCustomCalib = true;
}

//----------------------- Draw functions --------------------

void AStimulus::toggleSelectedAOI(AOI* aoi)
{
    if (m_selectedAOIs.Contains(aoi))
        m_selectedAOIs.Remove(aoi);
    else
        m_selectedAOIs.Add(aoi);
}

void AStimulus::strokeCircle(UCanvas* cvs, const FVector2D& pos, float radius, float thickness, const FLinearColor& color) const
{
    const int n = 8;
    for (int i = 0; i < n; ++i)
    {
        cvs->K2_DrawLine(FVector2D(pos.X + radius * cos((float)i / (float)(n - 1) * 2.0f * PI),
            pos.Y + radius * sin((float)i / (float)(n - 1) * 2.0f * PI)),
            FVector2D(pos.X + radius * cos((float)(i + 1) / (float)(n - 1) * 2.0f * PI),
                pos.Y + radius * sin((float)(i + 1) / (float)(n - 1) * 2.0f * PI)),
            thickness, color);
    }
}

void AStimulus::fillCircle(UCanvas* cvs, const FVector2D& pos, float radius, const FLinearColor& color) const
{
    cvs->K2_DrawPolygon(nullptr, pos, FVector2D(radius), 16, color);
}

void AStimulus::drawContourOfAOI(UCanvas* cvs, const FLinearColor& color, float th, AOI* aoi) const
{
    FVector2D pt = aoi->path[0];
    for (int i = 1, n = aoi->path.Num(); i < n; ++i)
    {
        cvs->K2_DrawLine(pt, aoi->path[i], th, color);
        pt = aoi->path[i];
    }
    cvs->K2_DrawLine(pt, aoi->path[0], th, color);
}

void AStimulus::drawContour(UCanvas* cvs, int32 w, int32 h)
{
    
#ifdef EYE_DEBUG
    float th = FMath::Max(round((float)FMath::Max(m_stimulusW, m_stimulusH) * 0.0025f), 1.0f);
    strokeCircle(cvs, FVector2D(m_rawTarget.X * m_stimulusW, m_rawTarget.Y * m_stimulusH), 2.0f * th, th, FLinearColor(1, 0, 0, 1));
    strokeCircle(cvs, FVector2D(m_corrTarget.X * m_stimulusW, m_corrTarget.Y * m_stimulusH), 2.0f * th, th, FLinearColor(0, 1, 0, 1));
    strokeCircle(cvs, FVector2D(m_camTarget.X * m_stimulusW, m_camTarget.Y * m_stimulusH), 2.0f * th, th, FLinearColor(1, 0, 1, 1));
#else
    float th = FMath::Max(FMath::RoundToFloat((float)FMath::Max(m_stimulusW, m_stimulusH) * 0.0025f), 1.0f);
    for (auto aoi : m_selectedAOIs)
        drawContourOfAOI(cvs, FLinearColor(0, 0.2, 0, 1), th, aoi);
    if (m_activeAOI)
        drawContourOfAOI(cvs, FLinearColor(1, 0, 0, 1), th, m_activeAOI);
#endif // EYE_DEBUG

     if (!informant->MC_Right->bHiddenInGame)
         fillCircle(cvs, FVector2D(m_laser.X * m_stimulusW, m_laser.Y * m_stimulusH), 10, FLinearColor(1, 0, 0, 1));

     if (m_customCalibPhase != CalibPhase::None && m_customCalibPhase != CalibPhase::Done)
         fillCircle(cvs, FVector2D(m_customCalibTarget.location.X * m_stimulusW, m_customCalibTarget.location.Y * m_stimulusH), m_customCalibTarget.radius, FLinearColor(0, 0, 0, 1));

     if (GEngine)
         GEngine->AddOnScreenDebugMessage(rand(), 5, FColor::Green, TEXT("countour updated"));
}

//----------------------- Private API -------------------------

//not used
UTexture2D* AStimulus::loadTexture2DFromFile(const FString& fullFilePath)
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

UTexture2D* AStimulus::loadTexture2DFromBytes(const TArray<uint8>& bytes, EImageFormat fmt, int& w, int& h)
{
    UTexture2D* loadedT2D = nullptr;

    IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(fmt);

    if (imageWrapper.IsValid() && imageWrapper->SetCompressed(bytes.GetData(), bytes.Num()))
    {
        TArray<uint8> uncompressedBGRA;
        if (imageWrapper->GetRaw(ERGBFormat::BGRA, 8, uncompressedBGRA))
        {
            w = imageWrapper->GetWidth();
            h = imageWrapper->GetHeight();
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

void AStimulus::SendDataToSciVi(const FGaze& gaze, FVector2D& uv, int AOI_index, const TCHAR* Id)
{
    auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
    //Send message to scivi
    FDateTime t = FDateTime::Now();
    float time = t.ToUnixTimestamp() * 1000.0f + t.GetMillisecond();
    auto msg = FString::Printf(TEXT("%f %f %f %f %f %F %F %F %F %F %F %F %i %s"),
        time, uv.X, uv.Y,
        gaze.origin.X, gaze.origin.Y, gaze.origin.Z,
        gaze.direction.X, gaze.direction.Y, gaze.direction.Z,
        gaze.left_pupil_diameter_mm, gaze.right_pupil_diameter_mm, gaze.cf, AOI_index, Id);
    GM->Broadcast(msg);
}
//used only in calibration
FVector AStimulus::billboardToScene(const FVector2D& pos) const
{
    return GetTransform().TransformPosition(FVector(m_staticExtent.X * (2.0f * pos.X - 1.0f),
        m_staticExtent.Y * (2.0f * pos.Y - 1.0f),
        0.0f));
}

FVector2D AStimulus::sceneToBillboard(const FVector& pos) const
{
    FVector local = GetTransform().InverseTransformPosition(pos);
    return FVector2D((local.X / m_staticExtent.X + 1.0f) / 2.0f, (local.Y / m_staticExtent.Y + 1.0f) / 2.0f);
}

//------------------------ Collision detection -----------------------
AStimulus::AOI* AStimulus::findActiveAOI(const FVector2D& pt, int& out_index) const
{
    out_index = -1;
    for(int i = 0; i < m_aois.Num(); ++i)
        if (m_aois[i].IsPointInside(pt))
        {
            out_index = i;
            return (AOI*)(m_aois.GetData() + i);
        }
    return nullptr;
}

//------------------------ Custom Calibration ------------------------

bool AStimulus::positiveOctant(const FVector gaze, const CalibPoint p1, const CalibPoint &p2, const CalibPoint &p3,
                               float &w1, float &w2, float &w3) const
{
    FMatrix basis(p1.gaze, p2.gaze, p3.gaze, FVector(0.0f));
    if (fabs(basis.Determinant()) < EPSILON)
        return false;
    FVector coords = basis.InverseTransformVector(gaze);
    w1 = coords.X;
    w2 = coords.Y;
    w3 = coords.Z;
    if (fabs(w1) < EPSILON)
        w1 = 0.0f;
    if (fabs(w2) < EPSILON)
        w2 = 0.0f;
    if (fabs(w3) < EPSILON)
        w3 = 0.0f;
    return w1 >= 0.0f && w2 >= 0.0f && w3 >= 0.0f;
}

bool AStimulus::findBasis(const FVector &gaze, CalibPoint &cp1, CalibPoint &cp2, CalibPoint &cp3, float &w1, float &w2, float &w3) const
{
    float thetaGaze = theta(gaze);
    // Theta is undefined for the center of vision area only.
    if (FGenericPlatformMath::IsNaN(thetaGaze))
    {
        cp1 = cp2 = cp3 = m_customCalibPoints[POINTS_PER_ROW * POINTS_PER_ROW / 2];
        w1 = 1.0f;
        w2 = w3 = 0.0f;
        return true;
    }

    const CalibPoint *p1 = nullptr;
    const CalibPoint *p2 = nullptr;
    const CalibPoint *p3 = nullptr;

    p1 = &m_customCalibPoints[POINTS_PER_ROW * POINTS_PER_ROW / 2];

    // p1: dot(cp1.gaze, gaze) --> max
    float maxDot = -2.0f;
    for (int i = 0, n = m_customCalibPoints.Num(); i < n; ++i)
    {
        if (p1 != &m_customCalibPoints[i])
        {
            float d = FVector::DotProduct(gaze, m_customCalibPoints[i].gaze);
            if (d > maxDot)
            {
                p2 = &m_customCalibPoints[i];
                maxDot = d;
            }
        }
    }

    maxDot = -2.0f;
    for (int i = 0, n = m_customCalibPoints.Num(); i < n; ++i)
    {
        if (p1 != &m_customCalibPoints[i] && p2 != &m_customCalibPoints[i])
        {
            float d = FVector::DotProduct(gaze, m_customCalibPoints[i].gaze);
            if (d > maxDot)
            {
                p3 = &m_customCalibPoints[i];
                maxDot = d;
            }
        }
    }

    if (!p1 || !p2 || !p3)
        return false;

    if (!positiveOctant(gaze, *p1, *p2, *p3, w1, w2, w3))
    {
        maxDot = -2.0f;
        for (int i = 0, n = m_customCalibPoints.Num(); i < n; ++i)
        {
            float d = FVector::DotProduct(gaze, m_customCalibPoints[i].gaze);
            if (d > maxDot)
            {
                p1 = &m_customCalibPoints[i];
                maxDot = d;
            }
        }
        cp1 = cp2 = cp3 = *p1;
        w1 = 1.0f;
        w2 = w3 = 0.0f;
        return true;
    }

    cp1 = *p1;
    cp2 = *p2;
    cp3 = *p3;

    return true;
}

void AStimulus::applyCustomCalib(const FVector& gazeOrigin, const FVector& gazeTarget, const FVector& gaze,
    FVector& correctedGazeTarget, bool& needsUpdateDynContour, float& cf)
{
    if (m_needsCustomCalib)
    {
        m_customCalibPhase = CalibPhase::StartDecreases;
        m_customCalibSamples = 0;
        m_customCalibPoints.Empty();
        m_needsCustomCalib = false;
    }

    if (m_customCalibPhase != CalibPhase::None && m_customCalibPhase != CalibPhase::Done)
    {
        const float CALIB_DISTANCE = 450.0f;
        FVector camLocation = informant->CameraComponent->GetComponentLocation();
        FRotator camRotation = informant->CameraComponent->GetComponentRotation();
        SetActorLocation(camRotation.RotateVector(FVector::ForwardVector) * CALIB_DISTANCE + camLocation);
        SetActorRotation(camRotation.Quaternion() * 
                        FQuat(FVector(0.0f, 0.0f, -1.0f), PI / 2.0f) * 
                        GetTransform().GetRotation());
    }

    cf = -1.0f;

    switch (m_customCalibPhase)
    {
    case CalibPhase::None:
    {
        correctedGazeTarget = gazeTarget;
        needsUpdateDynContour = false;
        break;
    }

    case CalibPhase::StartDecreases:
    {
        correctedGazeTarget = gazeTarget;
        needsUpdateDynContour = true;
        m_customCalibTarget.location = FVector2D(CENTER_POSTION, CENTER_POSTION);
        m_customCalibTarget.radius = map(m_customCalibSamples, 0, SAMPLES_TO_START, TARGET_MAX_RADIUS, TARGET_MIN_RADIUS);
        ++m_customCalibSamples;
        if (m_customCalibSamples == SAMPLES_TO_START)
        {
            m_customCalibSamples = 0;
            m_customCalibPhase = CalibPhase::StartMoves;
        }
        break;
    }

    case CalibPhase::StartMoves:
    {
        correctedGazeTarget = gazeTarget;
        needsUpdateDynContour = true;
        float pos = map(m_customCalibSamples, 0, SAMPLES_TO_START_MOVE, CENTER_POSTION, START_POSITION);
        ++m_customCalibSamples;
        if (m_customCalibSamples == SAMPLES_TO_START_MOVE)
        {
            m_customCalibTarget.location = FVector2D(START_POSITION, START_POSITION);
            m_customCalibSamples = 0;
            m_customCalibAccumReportedGaze = FVector(0.0f);
            m_customCalibAccumRealGaze = FVector(0.0f);
            m_customCalibAccumInternalGaze = FVector(0.0f);
            m_customCalibPhase = CalibPhase::TargetDecreases;
        }
        else
        {
            m_customCalibTarget.location = FVector2D(pos, pos);
        }
        break;
    }

    case CalibPhase::TargetDecreases:
    {
        correctedGazeTarget = gazeTarget;
        needsUpdateDynContour = true;
        m_customCalibTarget.radius = map(m_customCalibSamples, 0, SAMPLES_TO_DECREASE, TARGET_MAX_RADIUS, TARGET_MIN_RADIUS);
        FVector reportedGaze = gazeTarget - gazeOrigin;
        reportedGaze.Normalize();
        FVector realGaze = billboardToScene(m_customCalibTarget.location) - gazeOrigin;
        realGaze.Normalize();
        if (FMath::RadiansToDegrees(acosf(FVector::DotProduct(reportedGaze, realGaze))) < OUTLIER_THRESHOLD)
        {
            if (m_customCalibSamples > SAMPLES_TO_REJECT)
            {
                m_customCalibAccumReportedGaze += reportedGaze;
                m_customCalibAccumReportedGaze.Normalize();
                m_customCalibAccumRealGaze += realGaze;
                m_customCalibAccumRealGaze.Normalize();
                m_customCalibAccumInternalGaze += gaze;
                m_customCalibAccumInternalGaze.Normalize();
            }
            ++m_customCalibSamples;
        }
        else
        {
            m_customCalibSamples = 0;
            m_customCalibAccumReportedGaze = FVector(0.0f);
            m_customCalibAccumRealGaze = FVector(0.0f);
            m_customCalibAccumInternalGaze = FVector(0.0f);
        }
        if (m_customCalibSamples == SAMPLES_TO_DECREASE)
        {
            CalibPoint cp;
            cp.gaze = m_customCalibAccumInternalGaze;
            cp.qCorr = FQuat::FindBetween(m_customCalibAccumReportedGaze, m_customCalibAccumRealGaze);
            m_customCalibPoints.Add(cp);
            m_customCalibSamples = 0;
            m_customCalibPhase = CalibPhase::TargetMoves;
        }
        break;
    }

    case CalibPhase::TargetMoves:
    {
        correctedGazeTarget = gazeTarget;
        needsUpdateDynContour = true;
        int idx = m_customCalibPoints.Num();
        if (idx == POINTS_PER_ROW * ROWS_IN_PATTERN)
        {
            m_customCalibPhase = CalibPhase::Done;
            SetActorTransform(m_staticTransform);
        }
        else
        {
            FVector2D posTo = posForIdx(idx);
            ++m_customCalibSamples;
            if (m_customCalibSamples == SAMPLES_TO_MOVE)
            {
                m_customCalibTarget.location = posTo;
                m_customCalibSamples = 0;
                m_customCalibAccumReportedGaze = FVector(0.0f);
                m_customCalibAccumRealGaze = FVector(0.0f);
                m_customCalibAccumInternalGaze = FVector(0.0f);
                m_customCalibPhase = CalibPhase::TargetDecreases;
            }
            else
            {
                FVector2D posFrom = posForIdx(idx - 1);
                m_customCalibTarget.location = FVector2D(map(m_customCalibSamples, 0, SAMPLES_TO_MOVE, posFrom.X, posTo.X),
                    map(m_customCalibSamples, 0, SAMPLES_TO_MOVE, posFrom.Y, posTo.Y));
            }
        }
        break;
    }

    case CalibPhase::Done:
    {
        auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
        CalibPoint cp1, cp2, cp3;
        float w1, w2, w3;
        if (findBasis(gaze, cp1, cp2, cp3, w1, w2, w3))
        {
            FQuat corr = cp1.qCorr * w1 + cp2.qCorr * w2 + cp3.qCorr * w3;
            corr.Normalize();
            cf = corr.W;
            FVector reportedGazeOrigin, reportedGazeDirection;
            if (USRanipalEye_FunctionLibrary::GetGazeRay(GazeIndex::COMBINE, reportedGazeOrigin, reportedGazeDirection))
            {
                FVector camLocation = informant->CameraComponent->GetComponentLocation();
                FRotator camRotation = informant->CameraComponent->GetComponentRotation();
                FVector gazeRay = (corr.RotateVector(camRotation.RotateVector(reportedGazeDirection)) * MAX_DISTANCE) + camLocation;
                if (!GM->RayTrace(informant, camLocation, gazeRay, correctedGazeTarget))
                    correctedGazeTarget = gazeTarget;
            }
            else
            {
                correctedGazeTarget = gazeTarget;
            }
        }
        else
        {
            correctedGazeTarget = gazeTarget;
        }
        needsUpdateDynContour = false;
        break;
    }
    }
}



























