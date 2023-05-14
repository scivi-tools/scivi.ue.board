// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableActor.h"
#include "Engine/Canvas.h"
#include "IImageWrapper.h"
#include "ReadingTrackerGameMode.h"
#include "Stimulus.generated.h"

//#define EYE_DEBUG
//#define COLLECCT_ANGULAR_ERROR
//#define MEASURE_ANGULAR_SIZES



UCLASS()
class BASEREADINGTRACKER_API AStimulus : public AInteractableActor
{
    GENERATED_BODY()
public:
    //----------------- API ---------------------
    AStimulus(const FObjectInitializer& ObjectInitializer);
    virtual void BeginPlay() override;
    void UpdateStimulus(const UTexture2D* texture, float sx = 1.0f, float sy = 1.0f, 
                        const TArray<FAOI>& newAOIs = TArray<FAOI>(), bool notify_scivi = false);
    void UpdateContours();
    UFUNCTION(BlueprintCallable)
    void ClearSelectedAOIs();
    UFUNCTION(BlueprintCallable)
    void Reset();
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StimulusMargin = 20.0f;
    UFUNCTION(BlueprintImplementableEvent)
    void OnHoverAOI(const FAOI& aoi);
    UFUNCTION(BlueprintImplementableEvent)
    void OnLeaveAOI(const FAOI& aoi);

    void NotifyScivi_ImageUpdated();
    // ----------------- InteractableActor interface -------------
    virtual void OnPressedByTrigger(const FHitResult& hitResult) override;
    //eye track interaction
    virtual void ProcessEyeTrack(const FGaze& gaze, const FHitResult& hitResult) override;
    virtual void InFocusByController(const FHitResult& hit_result) override;
    //------------------------------------------------------------

    TArray<FAOI> AOIs;
    TArray<const FAOI*> SelectedAOIs;
    const FAOI* hoveredAOI = nullptr;
#ifdef EYE_DEBUG
    FVector2D GazeUV;
#endif

    UFUNCTION(BlueprintCallable)
    void customCalibrate();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wall)
    class UWidgetComponent* Stimulus;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Wall)
    class UStaticMeshComponent* wall;

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    const UTexture2D* DefaultTexture;

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

    //---------------- custom calibration --------------------

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