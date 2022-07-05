// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Canvas.h"
#include "IImageWrapper.h"
#include "ReadingTrackerGameMode.h"
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
    void UpdateStimulus(UTexture2D* texture, float sx, float sy, const TArray<FAOI>& newAOIs);
    void BindInformant(class ABaseInformant* _informant);
    void UpdateContours();
    void ClearSelectedAOIs();

    // ----------------- Input events -------------------
    void OnInFocus(const struct FGaze& gaze, const FHitResult& hitPoint);
    void OnTriggerPressed(const FHitResult& hitPoint);
    void OnTriggerReleased(const FHitResult& hitPoint);
    void OnImageUpdated();

    TArray<FAOI> AOIs;
    TArray<const FAOI*> SelectedAOIs;

    UFUNCTION(BlueprintCallable)
    void customCalibrate();

    UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Wall)
    USceneComponent* DefaultSceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wall)
    class UWidgetComponent* Stimulus;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Wall)
    class UStaticMeshComponent* wall;

    //----------------- Private API -----------------
protected:
    FVector billboardToScene(const FVector2D& pos) const;
    FVector2D sceneToBillboard(const FVector& pos) const;

    //draw functions
    UFUNCTION()
    void drawContour(UCanvas* cvs, int32 w, int32 h);
    void strokeCircle(UCanvas* cvs, const FVector2D& pos, float radius, float thickness, const FLinearColor& color) const;
    void fillCircle(UCanvas* cvs, const FVector2D& pos, float radius, const FLinearColor& color) const;
    void drawContourOfAOI(UCanvas* cvs, const FLinearColor& color, float th, const FAOI* aoi) const;
    void toggleSelectedAOI(const FAOI* aoi);

    //collision detection
    FAOI* findAOI(const FVector2D& pt, int& out_index) const;

    class ABaseInformant* informant = nullptr;
   

    //dynamic texture
    UPROPERTY(EditAnywhere)
    UMaterialInterface *base_material;
    class UMaterialInstanceDynamic* m_dynMat = nullptr;
    class UCanvasRenderTarget2D* m_dynContour = nullptr;
    UPROPERTY()
    const UTexture2D* image;
    

#ifdef EYE_DEBUG
    FVector2D m_rawTarget;
    FVector2D m_corrTarget;
    FVector2D m_camTarget;
#endif // EYE_DEBUG

    //custom calibration

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