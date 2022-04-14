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
    void BindInformant(class ABaseInformant* _informant);
    void OnInFocus(const struct FGaze& gaze, const FVector& FocusPoint);
    void OnTriggerPressed(const struct FGaze& gaze, const FVector& FocusPoint);
    void OnTriggerReleased(const struct FGaze& gaze, const FVector& FocusPoint);
    void OnImageUpdated();

    //UFUNCTION(BlueprintCallable)
    //void trigger(bool isPressed);
    UFUNCTION(BlueprintCallable)
    void customCalibrate();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UStaticMeshComponent* mesh;

    //----------------- Private API -----------------
protected:
    void SendDataToSciVi(const struct FGaze& gaze, FVector2D& uv, int AOI_index, FString Id);

    FVector billboardToScene(const FVector2D& pos) const;
    FVector2D sceneToBillboard(const FVector& pos) const;
    UTexture2D* loadTexture2DFromFile(const FString& fullFilePath);
    UTexture2D* loadTexture2DFromBytes(const TArray<uint8>& bytes, EImageFormat fmt, int& w, int& h);

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

    //draw functions
    void strokeCircle(UCanvas* cvs, const FVector2D& pos, float radius, float thickness, const FLinearColor& color) const;
    void fillCircle(UCanvas* cvs, const FVector2D& pos, float radius, const FLinearColor& color) const;
    void drawContourOfAOI(UCanvas* cvs, const FLinearColor& color, float th, AOI* aoi) const;
    void toggleSelectedAOI(AOI* aoi);

    //collision detection
    AOI* findActiveAOI(const FVector2D& pt, int& out_index) const;
    //bool castRay(const FVector& origin, const FVector& end, FVector& hitPoint) const;
    //bool IsInFocus(FVector& gazeOrigin, FVector& rawGazeTarget, FVector& correctedGazeTarget,
    //   float& leftPupilDiam, float& rightPupilDiam, bool& needsUpdateDynContour, float& cf);

    class ABaseInformant* informant = nullptr;
    FVector2D m_laser;
    
    //dynamic texture
    class UMaterialInstanceDynamic* m_dynTex = nullptr;
    class UCanvasRenderTarget2D* m_dynContour = nullptr;
    FCriticalSection m_mutex;//for dynTEx
    float m_aspect;
    float m_scaleX;
    float m_scaleY;
    int m_dynTexW;
    int m_dynTexH;
    TArray<AOI> m_dynAOIs;
    FThreadSafeBool m_needsUpdate;
    

    //bool m_inSelectionMode;
    //bool m_rReleased;
    //bool m_imgUpdated;
    int m_stimulusW;
    int m_stimulusH;
    TArray<AOI> m_aois;
    AOI* m_activeAOI;
    TArray<AOI*> m_selectedAOIs;

#ifdef EYE_DEBUG
    FVector2D m_rawTarget;
    FVector2D m_corrTarget;
    FVector2D m_camTarget;
#endif // EYE_DEBUG


    struct CalibPoint
    {
        FVector gaze;
        FQuat qCorr;
    };
    struct CalibTarget
    {
        FVector2D location;
        float radius = 0.0f;
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

    //custom calibration
    bool positiveOctant(const FVector gaze, const CalibPoint p1, const CalibPoint& p2, const CalibPoint& p3, float& w1, float& w2, float& w3) const;
    bool findBasis(const FVector& gaze, CalibPoint& cp1, CalibPoint& cp2, CalibPoint& cp3, float& w1, float& w2, float& w3) const;
    void applyCustomCalib(const FVector& gazeOrigin, const FVector& gazeTarget, const FVector& gaze,
        FVector& correctedGazeTarget, bool& needsUpdateDynContour, float& cf);    

    int m_calibIndex;
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
};
