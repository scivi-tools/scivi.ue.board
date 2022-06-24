// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReadingTrackerGameMode.h"
#include "WordListWall.generated.h"



UCLASS()
class READINGTRACKER_API AWordListWall : public AActor
{
	GENERATED_BODY()
	
public:	
	//----------------- API ---------------------
	// Sets default values for this actor's properties
	AWordListWall();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	bool IsHiddenInGame() const;
	UFUNCTION(BlueprintCallable)
	void SetVisibility(bool is_visible);
	UFUNCTION(BlueprintCallable)
	void SetWallName(const FString& new_name);
	FORCEINLINE const FString& GetWallName() const { return name; }
	FORCEINLINE float GetWallWidth() const { return GetActorRelativeScale3D().Z * 100.0f; }

	void AddAOI(const FAOI* aoi);
	void ClearList();

	// -------------------- Properties ----------------
	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneRoot;
	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadonly)
	class UWidgetComponent* ListHeader;
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadonly)
	class UWidgetComponent* List;
	UPROPERTY(EditAnywhere, BlueprintReadonly)
	TSubclassOf<UUserWidget> EntryWidgetClass;

	//------------------ Private API ----------------------
protected:
	UFUNCTION()
	void OnClicked_DeleteList();
	UFUNCTION()
	void OnClicked_AddEntry();
	UFUNCTION()
	void OnClicked_RemoveEntry(URichButton* clickedButton, const FVector2D& clickPos);
	bool bHiddenInGame = false;
	TMap<const URichButton*, UUserWidget*> entries;//for fast searching of entry
	FString name = TEXT("List");
};
