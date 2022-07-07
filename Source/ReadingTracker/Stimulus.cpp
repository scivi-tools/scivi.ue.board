// Fill out your copyright notice in the Description page of Project Settings.

#include "Stimulus.h"
#include "BaseInformant.h"
#include "Components/WidgetComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "SRanipalEye_FunctionLibrary.h"
#include "SRanipalEye_Framework.h"
#include "SRanipal_API_Eye.h"
#include "SRanipalEye_Core.h"
#include "IXRTrackingSystem.h"
#include "Engine/CanvasRenderTarget2D.h"

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
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterialAsset(TEXT("Material'/Game/StarterContent/Materials/M_Wall.M_Wall'"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BoardMaterialAsset(TEXT("Material'/Game/Materials/BoardMat.BoardMat'"));
    static ConstructorHelpers::FClassFinder<UUserWidget> CreateListWidgetClass(TEXT("/Game/UI/UI_CreateList_Menu"));
    static ConstructorHelpers::FClassFinder<UUserWidget> CanvasWidgetClass(TEXT("/Game/UI/UI_Canvas"));

    base_material = BoardMaterialAsset.Object;

    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = DefaultSceneRoot;

    wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
    wall->SetupAttachment(RootComponent);
    wall->SetStaticMesh(CubeAsset.Object);
    wall->SetMaterial(0, WallMaterialAsset.Object);
    wall->SetRelativeScale3D(FVector(0.1f, 5.0f, 3.0f));
    wall->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));

    Stimulus = CreateDefaultSubobject<UWidgetComponent>(TEXT("Canvas"));
    Stimulus->SetupAttachment(RootComponent);
    Stimulus->SetWidgetClass(CanvasWidgetClass.Class);
    Stimulus->SetDrawSize(FVector2D(400.0f, 200.0f));
    Stimulus->SetRelativeLocation(FVector(10.0f, 0.0f, 150.0f));
    Stimulus->SetUsingAbsoluteScale(true);
    Stimulus->SetCollisionResponseToChannel(Stimulus_Channel, ECollisionResponse::ECR_Block);
    
    m_calibIndex = 0;
    m_needsCustomCalib = false;
    m_customCalibSamples = 0;
}

void AStimulus::BeginPlay()
{
    Super::BeginPlay();
    const int default_size = 100;
    image = UTexture2D::CreateTransient(default_size, default_size);
    //create textures for material
    m_dynContour = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetWorld(), UCanvasRenderTarget2D::StaticClass(), default_size, default_size);
    m_dynContour->ClearColor = FLinearColor(0, 0, 0, 0);
    m_dynContour->OnCanvasRenderTargetUpdate.AddDynamic(this, &AStimulus::drawContour);
    m_dynContour->UpdateResource();
    //create dynamic material
    m_dynMat = UMaterialInstanceDynamic::Create(base_material, Stimulus);
    m_dynMat->SetTextureParameterValue(TEXT("ContourTex"), m_dynContour);
    m_dynMat->SetTextureParameterValue(TEXT("DynTex"), (UTexture2D*)image);
    //set material to the widget
    Stimulus->SetDrawSize(FVector2D(default_size, default_size));
    auto image_widget = Cast<UImage>(Stimulus->GetWidget()->GetWidgetFromName(TEXT("Stimulus")));
    image_widget->SetBrushFromMaterial(m_dynMat);
    m_staticExtent = Stimulus->CalcLocalBounds().BoxExtent;
    m_staticTransform = Stimulus->GetRelativeTransform();
}

void AStimulus::UpdateStimulus(UTexture2D* texture, float sx, float sy, const TArray<FAOI>& newAOIs)
{
    int margin = 20.0f;
    AOIs = newAOIs;
    SelectedAOIs.Empty();

    SetActorScale3D(FVector(1.0f, sx, sy));
    // set new image
    image = texture;
    auto image_size = FVector2D(image->GetSizeX(), image->GetSizeY());
    auto wall_scale = wall->GetComponentScale();
    auto wall_size = FVector2D(wall_scale.Y, wall_scale.Z) * 100;//one scale = 100 units
    float factor = (wall_size.X - margin) / image_size.X;
    if (image_size.X < image_size.Y)
        factor = (wall_size.Y - margin) / image_size.Y;
    FVector image_scale(1.0f, factor, factor);
    Stimulus->SetDrawSize(image_size);
    Stimulus->SetRelativeScale3D(image_scale);
    m_dynMat->SetTextureParameterValue(TEXT("DynTex"), (UTexture*)image);
    m_dynContour->ResizeTarget(image_size.X, image_size.Y);
    m_dynContour->UpdateResource();//to clear contour

    m_staticTransform = Stimulus->GetRelativeTransform();
    m_staticExtent = Stimulus->CalcLocalBounds().BoxExtent;
    OnImageUpdated();
}

void AStimulus::BindInformant(ABaseInformant* _informant)
{
    informant = _informant;
}

void AStimulus::UpdateContours()
{
    m_dynContour->UpdateResource();
}

void AStimulus::ClearSelectedAOIs()
{
    FGaze gaze;
    informant->GetGaze(gaze);
    FVector2D uv(-1000.0f, -1000.0f);
    //check if gaze im stimulus => then calc uv
    auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
    FHitResult hitPoint(ForceInit);
    const float ray_radius = 1.0f;
    const float ray_length = 1000.0f;
    if (GM->RayTrace(informant, gaze.origin, gaze.origin + gaze.direction * ray_length, hitPoint))
    {
        if (hitPoint.Actor == this && hitPoint.Component == Stimulus)
            uv = sceneToBillboard(hitPoint.Location);
    }
    for (auto selected_aoi : SelectedAOIs) 
    {
        int aoi_index = selected_aoi - AOIs.GetData();
        GM->SendGazeToSciVi(gaze, uv, aoi_index, TEXT("SELECT"));//this unselect selected in sciVi
    }
    SelectedAOIs.Empty();
    UpdateContours();
}

void AStimulus::OnInFocus(const FGaze& gaze, const FHitResult& hitPoint)
{
    if (hitPoint.Component == Stimulus)
    {
        auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
        FVector2D uv = sceneToBillboard(hitPoint.Location);
        int currentAOIIndex = -1;
        if (!informant->MC_Right->bHiddenInGame)
            auto lookedAOI = findAOI(FVector2D(uv.X * image->GetSizeX(), uv.Y * image->GetSizeY()), currentAOIIndex);
        GM->SendGazeToSciVi(gaze, uv, currentAOIIndex, TEXT("LOOKAT"));
    }
}

void AStimulus::OnTriggerPressed(const FHitResult& hitPoint)
{
    //if (hitPoint.Component == Stimulus)
    //{
    //    if (!informant->MC_Right->bHiddenInGame)
    //    {
    //        m_laser = sceneToBillboard(hitPoint.Location);
    //        UpdateContours();
    //    }
    //}
}

void AStimulus::OnTriggerReleased(const FHitResult& hitPoint)
{
    if (hitPoint.Component == Stimulus)
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
        FGaze gaze;
        informant->GetGaze(gaze);
        auto m_laser = sceneToBillboard(hitPoint.Location);
        int currentAOIIndex = -1;
        auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
        if (!informant->MC_Right->bHiddenInGame)
        {
            auto selectedAOI = findAOI(FVector2D(m_laser.X * image->GetSizeX(), m_laser.Y * image->GetSizeY()), currentAOIIndex);
            if (selectedAOI)
            {
                toggleSelectedAOI(selectedAOI);
                GM->SendGazeToSciVi(gaze, m_laser, currentAOIIndex, TEXT("SELECT"));
            }
        }
        UpdateContours();
        GM->SendGazeToSciVi(gaze, m_laser, currentAOIIndex, TEXT("R_RELD"));
    }
}

void AStimulus::OnImageUpdated()
{
    FGaze gaze;
    informant->GetGaze(gaze);

    auto GM = GetWorld()->GetAuthGameMode<AReadingTrackerGameMode>();
    FHitResult hitPoint(ForceInit);
    const float ray_radius = 1.0f;
    const float ray_length = 1000.0f;
    if (GM->RayTrace(informant, gaze.origin, gaze.origin + gaze.direction * ray_length, hitPoint))
    {
        if (hitPoint.Actor == this && hitPoint.Component == Stimulus)
        {
            FVector2D uv = sceneToBillboard(hitPoint.Location);
            int currentAOIIndex = -1;
            if (!informant->MC_Right->bHiddenInGame)
                auto lookedAOI = findAOI(FVector2D(uv.X * image->GetSizeX(), uv.Y * image->GetSizeY()), currentAOIIndex);
            GM->SendGazeToSciVi(gaze, uv, currentAOIIndex, TEXT("IMG_UP"));
        }
    }
}

void AStimulus::customCalibrate()
{
    m_needsCustomCalib = true;
}

//------------------------------ Private API --------------------------------

//------------ Draw functions ------------

void AStimulus::toggleSelectedAOI(const FAOI* aoi)
{
    if (SelectedAOIs.Contains(aoi))
        SelectedAOIs.Remove(aoi);
    else
        SelectedAOIs.Add(aoi);
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

void AStimulus::drawContourOfAOI(UCanvas* cvs, const FLinearColor& color, float th, const FAOI* aoi) const
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
    float th = FMath::Max(round((float)FMath::Max(image->GetSizeX(), image->GetSizeY()) * 0.0025f), 1.0f);
    strokeCircle(cvs, FVector2D(m_rawTarget.X * image->GetSizeX(), m_rawTarget.Y * image->GetSizeY()), 2.0f * th, th, FLinearColor(1, 0, 0, 1));
    strokeCircle(cvs, FVector2D(m_corrTarget.X * image->GetSizeX(), m_corrTarget.Y * image->GetSizeY()), 2.0f * th, th, FLinearColor(0, 1, 0, 1));
    strokeCircle(cvs, FVector2D(m_camTarget.X * image->GetSizeX(), m_camTarget.Y * image->GetSizeY()), 2.0f * th, th, FLinearColor(1, 0, 1, 1));
#else
    float th = FMath::Max(FMath::RoundToFloat((float)FMath::Max(image->GetSizeX(), image->GetSizeY()) * 0.0025f), 1.0f);
    for (auto aoi : SelectedAOIs)
        drawContourOfAOI(cvs, FLinearColor(0, 0.2, 0, 1), th, aoi);
#endif // EYE_DEBUG

    /*if (informant && !informant->MC_Right->bHiddenInGame)
        fillCircle(cvs, FVector2D(m_laser.X * image->GetSizeX(), m_laser.Y * image->GetSizeY()), 10, FLinearColor(1, 0, 0, 1));*/

    if (m_customCalibPhase != CalibPhase::None && m_customCalibPhase != CalibPhase::Done)
        fillCircle(cvs, FVector2D(m_customCalibTarget.location.X * image->GetSizeX(), m_customCalibTarget.location.Y * image->GetSizeY()), m_customCalibTarget.radius, FLinearColor(0, 0, 0, 1));
}

//used only in calibration
FVector AStimulus::billboardToScene(const FVector2D& pos) const
{
    auto draw_size = Stimulus->GetDrawSize();
    FVector local = FVector(-(pos.X - 0.5f) * draw_size.X,
                            -(pos.Y - 0.5f)* draw_size.Y, 
                            0.0f);
    return Stimulus->GetComponentTransform().TransformPosition(local);
}

FVector2D AStimulus::sceneToBillboard(const FVector& pos) const
{
    auto draw_size = Stimulus->GetDrawSize();
    FVector local = Stimulus->GetComponentTransform().InverseTransformPosition(pos);
    return FVector2D(-local.Y / draw_size.X + 0.5f, -local.Z / draw_size.Y + 0.5f);
}

//------------------------ Collision detection -----------------------
FAOI* AStimulus::findAOI(const FVector2D& pt, int& out_index) const
{
    out_index = -1;
    for(int i = 0; i < AOIs.Num(); ++i)
        if (AOIs[i].IsPointInside(pt))
        {
            out_index = i;
            return (FAOI*)(AOIs.GetData() + i);
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
                FHitResult hitPoint;
                if (!GM->RayTrace(informant, camLocation, gazeRay, hitPoint)) {
                    correctedGazeTarget = hitPoint.Location;
                    correctedGazeTarget = gazeTarget;
                }
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



























