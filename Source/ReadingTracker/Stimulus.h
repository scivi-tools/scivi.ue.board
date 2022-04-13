// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Canvas.h"
#include "IImageWrapper.h"
#include "Stimulus.generated.h"


//#define EYE_DEBUG
//#define COLLECCT_ANGULAR_ERROR
//#define MEASURE_ANGULAR_SIZES

UCLASS()
class READINGTRACKER_API AStimulus : public AActor
{
    GENERATED_BODY()
public:
    //----------------- API ---------------------
    AStimulus();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    void updateDynTex(const TArray<uint8>& img, EImageFormat fmt, float sx, float sy, const TArray<TSharedPtr<FJsonValue>>& aois);
    void drawContour(UCanvas* cvs, int32 w, int32 h);
    void toggleMotionController(bool visible);

    UFUNCTION(BlueprintCallable)
    void trigger(bool isPressed);
    UFUNCTION(BlueprintCallable)
    void customCalib();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UStaticMeshComponent* mesh;
private:
    struct BBox
    {
        FVector2D lt;
        FVector2D rb;
        inline bool IsPointInside(const FVector2D& pt) const
        {
            return pt.X >= lt.X && pt.Y >= lt.Y && pt.X <= rb.X && pt.Y <= rb.Y;
        }
    };
    struct AOI
    {
        FString name;
        TArray<FVector2D> path;
        BBox bbox;
        inline bool IsPointInside(const FVector2D& pt) const
        {
            //check point in polygon
            bool IsInPolygon = false;
            int n = path.Num();
            for (int i = 0, j = n - 1; i < n; j = i++)
            {
                if (((path[i].Y > pt.Y) != (path[j].Y > pt.Y)) &&
                    (pt.X < (path[j].X - path[i].X) * (pt.Y - path[i].Y) /
                        (path[j].Y - path[i].Y) + path[i].X))
                    IsInPolygon = !IsInPolygon;
            }
            return bbox.IsPointInside(pt) && IsInPolygon;
        }
    };
    struct CalibPoint
    {
        FVector gaze;
        FQuat qCorr;
    };
    struct CalibTarget
    {
        FVector2D location;
        float radius;

        CalibTarget(): radius(0.0f) {};
    };
    enum CalibPhase
    {
        None = 0,
        StartDecreases,
        StartMoves,
        TargetDecreases,
        TargetMoves,
        Done
    };

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

    class UMaterialInstanceDynamic *m_dynTex;
    class UCanvasRenderTarget2D *m_dynContour;
    FCriticalSection m_mutex;
    float m_aspect;
    float m_scaleX;
    float m_scaleY;
    int m_dynTexW;
    int m_dynTexH;
    TArray<AOI> m_dynAOIs;
    FThreadSafeBool m_needsUpdate;
    int m_calibIndex;

    bool m_inSelectionMode;
    bool m_rReleased;
    bool m_imgUpdated;
    int m_stimulusW;
    int m_stimulusH;
    int m_activeAOI;
    TArray<AOI> m_aois;
    TArray<int> m_selectedAOIs;

#ifdef EYE_DEBUG
    FVector2D m_rawTarget;
    FVector2D m_corrTarget;
    FVector2D m_camTarget;
#endif // EYE_DEBUG

    FThreadSafeBool m_needsCustomCalib;
    CalibPhase m_customCalibPhase;
    TArray<CalibPoint> m_customCalibPoints;
    CalibTarget m_customCalibTarget;
    int m_customCalibSamples;
    FVector m_customCalibAccumReportedGaze;
    FVector m_customCalibAccumRealGaze;
    FVector m_customCalibAccumInternalGaze;
    FTransform m_staticTransform;
    FVector m_staticExtent;

    class UMotionControllerComponent *m_mcRight;
    FVector2D m_laser;

    class APlayerCameraManager *m_camera;

    UTexture2D *loadTexture2DFromFile(const FString &fullFilePath);
    UTexture2D *loadTexture2DFromBytes(const TArray<uint8> &bytes, EImageFormat fmt, int &w, int &h);
    
    void strokeCircle(UCanvas *cvs, const FVector2D &pos, float radius, float thickness, const FLinearColor &color) const;
    void fillCircle(UCanvas *cvs, const FVector2D &pos, float radius, const FLinearColor& color) const;
    void drawContourOfAOI(UCanvas *cvs, const FLinearColor &color, float th, int aoi) const;
    bool pointInPolygon(const FVector2D &pt, const TArray<FVector2D> &poly) const;
    bool pointInBBox(const FVector2D &pt, const BBox &bbox) const;
    bool hitTest(const FVector2D &pt, const AOI &aoi) const;
    int findActiveAOI(const FVector2D &pt) const;
    void toggleSelectedAOI(int aoi);
    void customCalibrate();

    //math functions
    float map(float v, float fromMin, float fromMax, float toMin, float toMax) const
    {
    	return toMin + (v - fromMin) / (fromMax - fromMin) * (toMax - toMin);
    };
    FVector billboardToScene(const FVector2D &pos) const;
    FVector2D sceneToBillboard(const FVector &pos) const;
    float theta(const FVector &gaze) const
    {
        return fabs(gaze.Y) < EPSILON && fabs(gaze.Z) < EPSILON ? NAN : atan2(gaze.Y, gaze.Z) + PI;
    };
    float radius2(const FVector &gaze) const
    {
        return gaze.Y * gaze.Y + gaze.Z * gaze.Z;
    };
    int signum(float a, float b) const
    {
        return FGenericPlatformMath::IsNaN(a) || FGenericPlatformMath::IsNaN(b) || fabs(a - b) < EPSILON ? 0 : a - b > 0.0f ? 1 : -1;
    };
    bool positiveOctant(const FVector gaze, const CalibPoint p1, const CalibPoint &p2, const CalibPoint &p3, float &w1, float &w2, float &w3) const;
    bool findBasis(const FVector &gaze, CalibPoint &cp1, CalibPoint &cp2, CalibPoint &cp3, float &w1, float &w2, float &w3) const;
    bool castRay(const FVector &origin, const FVector &ray, FVector &hitPoint) const;
    FVector2D posForIdx(int idx) const;
    void applyCustomCalib(const FVector &gazeOrigin, const FVector &gazeTarget, const FVector &gaze,
                          FVector &correctedGazeTarget, bool &needsUpdateDynContour, float &cf);
    bool focus(FVector &gazeOrigin, FVector &rawGazeTarget, FVector &correctedGazeTarget,
               float &leftPupilDiam, float &rightPupilDiam, bool &needsUpdateDynContour, float &cf);


};
