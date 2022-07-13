// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AudioDevice.h"
#include "SubmixRecorder.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class USubmixRecorder : public USceneComponent, public ISubmixBufferListener
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USubmixRecorder();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	int32 NumChannelsToRecord = 2;//support only 1 or 2
	TFunction<void(const int16*, int, int, int)> OnRecorded = nullptr;
	TFunction<void()> OnRecordFinished = nullptr;

	UFUNCTION(BlueprintCallable)
	void StartRecording();
	UFUNCTION(BlueprintCallable)
	void StopRecording();
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsRecording() const { return bIsRecording; };

	//don't change SubmixToRecord while recording!!
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class USoundSubmix* SubmixToRecord = nullptr;

protected:
	FCriticalSection use_queue;
	FThreadSafeBool bIsRecording = false;
	bool bRecordFinished = true;
	TResizableCircularQueue<Audio::TSampleBuffer<int16>> RecordQueue{ 64 };

	//---------------- ISubmixBufferListener Interface ---------------
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix,
		float* AudioData, int32 NumSamples, int32 NumChannels,
		const int32 SampleRate, double AudioClock) override;
};
