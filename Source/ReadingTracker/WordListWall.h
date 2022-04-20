// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WordListWall.generated.h"

UCLASS()
class READINGTRACKER_API AWordListWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWordListWall();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	bool IsHiddenInGame() const;
	UFUNCTION(BlueprintCallable)
	void SetVisibility(bool is_visible);
	void SetWallName(const FString& name);


	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneRoot;
	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadonly)
	class UWidgetComponent* ListHeader;
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadonly)
	class UWidgetComponent* List;
protected:
	UFUNCTION()
	void OnClicked_DeleteList();
	bool bHiddenInGame = false;
};
