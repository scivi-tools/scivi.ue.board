// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#define _USE_SCIVI_CONNECTION_ 1
#include "CoreMinimal.h"
#include "VRGameModeWithSciViBase.h"
#include "ReadingTrackerGameMode.generated.h"
//Channel to check collision with 
static const ECollisionChannel Stimulus_Channel = ECollisionChannel::ECC_GameTraceChannel1;
static const ECollisionChannel Floor_Channel = ECollisionChannel::ECC_WorldStatic;


USTRUCT()
struct BASEREADINGTRACKER_API FPolygon2D
{
	GENERATED_BODY()

		UPROPERTY()
		TArray<FVector2D> vertices;
	UPROPERTY()
		FBox2D bbox;

	FPolygon2D() = default;

	FPolygon2D(FPolygon2D&& other)
		: vertices(MoveTemp(other.vertices)),
		bbox(other.bbox)
	{

	}

	FPolygon2D& operator=(FPolygon2D&& other)
	{
		vertices = MoveTemp(other.vertices);
		bbox = other.bbox;
		return *this;
	}

	FPolygon2D(const FPolygon2D& other)
		:vertices(other.vertices),
		bbox(other.bbox)
	{

	}

	FPolygon2D& operator=(const FPolygon2D& other)
	{
		vertices = other.vertices;
		bbox = other.bbox;
		return *this;
	}

public:
	/// add vertex
	void AddVertex(const FVector2D& vertex) { vertices.Add(vertex); }
	void AddVertices(const TArray<FVector2D>& new_vertices) { vertices.Append(new_vertices); }
	void SetBounding(const FBox2D& box) { bbox = box; }
	/// clear vertex data
	void Clear() { vertices.Empty(); }
	const FBox2D& GetBBox() const { return bbox; }
	const TArray<FVector2D>& GetVertices() const { return vertices; }
};





UCLASS(BlueprintType)
class BASEREADINGTRACKER_API UAOI : public UObject
{
	GENERATED_BODY()

public:
	UAOI() = default;
	UAOI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	UPROPERTY()
	FPolygon2D path;//polygon
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
		int id = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
		TArray<FString> audio_desciptions{};
	int last_played_description = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
		FString name = TEXT("no name");
	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Transient)
		UTexture2D* image = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadonly)
		int32 order = -1;


	UFUNCTION(BlueprintCallable)
		FORCEINLINE bool IsPointIn(const FVector2D& point) const
	{
		if (!path.GetBBox().IsInside(point)) return false;
		//check point in polygon
		bool IsInPolygon = false;
		const auto& vertices = path.GetVertices();
		int n = vertices.Num();
		for (int i = 0, j = n - 1; i < n; j = i++)
		{
			if (((vertices[i].Y > point.Y) != (vertices[j].Y > point.Y)) &&
				(point.X < (vertices[j].X - vertices[i].X) * (point.Y - vertices[i].Y) /
					(vertices[j].Y - vertices[i].Y) + vertices[i].X))
				IsInPolygon = !IsInPolygon;
		}
		return IsInPolygon;
	}
};

UENUM()
enum class EWallLogAction
{
	NewWall     UMETA(DisplayName = "NewWall"),
	DeleteWall      UMETA(DisplayName = "DeleteWall"),
	AddAOI   UMETA(DisplayName = "AddAOI"),
	RemoveAOI UMETA(DisplayName = "RemoveAOI")
};

/**
 *
 */
UCLASS()
class BASEREADINGTRACKER_API AReadingTrackerGameMode : public AVRGameModeWithSciViBase
{
	GENERATED_BODY()
public:
	AReadingTrackerGameMode(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

	virtual void OnSciViConnected() override
	{
		Super::OnSciViConnected(); StartExperiment(true);
	}
	virtual void OnSciViDisconnected() override
	{
		Super::OnSciViDisconnected(); FinishExperiment(0, TEXT("SciVi disconnected"));
	}
	//----------------- Scene ----------------------
public:
	virtual void NotifyStimulustSpawned(class AStimulus* _stimulus);

	UPROPERTY(EditAnywhere, BlueprintReadonly, DisplayName = "Stimulus")
		class AStimulus* stimulus;

	//----------------------- SciVi networking --------------
public:
	void SendGazeToSciVi(const struct FGaze& gaze, FVector2D& uv, int AOI_index, const FString& Id);
protected:
	virtual void OnSciViMessageReceived(TSharedPtr<FJsonObject> msgJson) override;
	void ParseNewImage(const TSharedPtr<FJsonObject>& json);
};
