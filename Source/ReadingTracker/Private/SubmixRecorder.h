// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AudioDevice.h"
#include "StaticSampleBuffer.h"
#include "../ReadingTracker.h"
#include "SubmixRecorder.generated.h"


//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBatchRecordedSignature, AudioSampleBuffer&, buffer);

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
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	void SetNumChannels(int newNumChannels);
	void PopFirstRecordedBuffer(AudioSampleBuffer& destination);
	std::size_t GetRecordedBuffersCount();
	
	UFUNCTION(BlueprintCallable)
	void StartRecording();
	UFUNCTION(BlueprintCallable)
	void StopRecording();
	UFUNCTION(BlueprintCallable)
	void Reset();
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsRecording() const { return bIsRecording; };

	//don't change SubmixToRecord while recording!!
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class USoundSubmix* SubmixToRecord = nullptr;

protected:
	FCriticalSection use_queue;
	FThreadSafeBool bIsRecording = false;
	TResizableCircularQueue<AudioSampleBuffer> RecordingRawData{ 64 };
	int RecordNumChannels = 2;
	AudioSampleBuffer new_batch;

	//---------------- ISubmixBufferListener Interface ---------------
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix,
		float* AudioData, int32 NumSamples, int32 NumChannels,
		const int32 sample_rate, double AudioClock) override;
};
