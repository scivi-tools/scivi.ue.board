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

	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;
	UPROPERTY(Category = Wall, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	UPROPERTY(Category = UI, EditAnywhere, BlueprintReadonly)
	class UWidgetComponent* DeleteButton;

};
