// Fill out your copyright notice in the Description page of Project Settings.


#include "SubmixRecorder.h"
#include "AudioDeviceManager.h"
#include "Sound/SoundSubmix.h"



// Sets default values for this component's properties
USubmixRecorder::USubmixRecorder()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void USubmixRecorder::BeginPlay()
{
	Super::BeginPlay();
	if (!GEngine) return;
	if (NumChannelsToRecord == 1 || NumChannelsToRecord == 2) {
		if (FAudioDevice* AudioDevice = GetWorld()->GetAudioDeviceRaw())
			AudioDevice->RegisterSubmixBufferListener(this, SubmixToRecord);
	}
	else
		UE_LOG(LogAudio, Warning, TEXT("SubmixRecorder supports only 1 or 2 channels to record"));
}

void USubmixRecorder::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	FScopeLock lock(&use_queue);
	if (!RecordQueue.IsEmpty())
	{
		if (OnRecorded) 
		{
			auto& buffer = RecordQueue.Peek();
			OnRecorded(buffer.GetData(), buffer.GetNumChannels(), buffer.GetNumSamples(), buffer.GetSampleRate());
		}
		RecordQueue.Pop();
		bRecordFinished = false;
	}
	else
	if (OnRecordFinished && !bIsRecording && !bRecordFinished)
	{
		OnRecordFinished();
		bRecordFinished = true;
	}
}

void USubmixRecorder::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	if (!GEngine) return;
	if (FAudioDevice* AudioDevice = GetWorld()->GetAudioDeviceRaw())
	{
		AudioDevice->UnregisterSubmixBufferListener(this, SubmixToRecord);
	}
}

void USubmixRecorder::StartRecording()
{
	bIsRecording = true;
	bRecordFinished = false;
}

void USubmixRecorder::StopRecording()
{
	bIsRecording = false;
}

void USubmixRecorder::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock)
{
	if (bIsRecording) 
	{
		FScopeLock lock(&use_queue);
		auto& buffer = RecordQueue.EnqueueDefaulted_GetRef();
		buffer.Append(AudioData, NumSamples, NumChannels, SampleRate);
		buffer.MixBufferToChannels(NumChannelsToRecord);
	}
}																																																			