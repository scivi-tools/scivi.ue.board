// Fill out your copyright notice in the Description page of Project Settings.


#include "SubmixRecorder.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Sound/SoundSubmix.h"



// Sets default values for this component's properties
USubmixRecorder::USubmixRecorder()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	new_batch.NumChannels = RecordNumChannels;
	new_batch.NumSamples = 0;
	new_batch.NumFrames = 0;
	new_batch.SampleDuration = 0.0f;
}

// Called when the game starts
void USubmixRecorder::BeginPlay()
{
	Super::BeginPlay();
	if (!GEngine) return;
	if (FAudioDevice* AudioDevice = GetWorld()->GetAudioDeviceRaw())
	{
		AudioDevice->RegisterSubmixBufferListener(this, SubmixToRecord);
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

void USubmixRecorder::SetNumChannels(int newNumChannels)
{
	if (newNumChannels > 2)
		UE_LOG(LogAudio, Warning, TEXT("SubmixRecorder::Only 1 or 2 channels supported"));
	RecordNumChannels = std::min(2, newNumChannels);
}



void USubmixRecorder::PopFirstRecordedBuffer(AudioSampleBuffer& destination)
{
	FScopeLock lock(&use_queue);
	destination = RecordingRawData.Peek();//copying
	RecordingRawData.Pop();
}

std::size_t USubmixRecorder::GetRecordedBuffersCount()
{
	FScopeLock lock(&use_queue);
	return RecordingRawData.Count();
}

void USubmixRecorder::StartRecording()
{
	bIsRecording = true;
}

void USubmixRecorder::StopRecording()
{
	bIsRecording = false;
	RecordingRawData.Enqueue(new_batch);
}

void USubmixRecorder::Reset()
{
	FScopeLock lock(&use_queue);
	RecordingRawData.Reset();
}

void USubmixRecorder::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 sample_rate, double AudioClock)
{
	if (bIsRecording) 
	{
		new_batch.Append(AudioData, NumSamples, NumChannels);
		new_batch.sample_rate = sample_rate;
		if (new_batch.NumSamples >= AudioSampleBuffer_MaxSamplesCount)
		{
			use_queue.Lock();
			RecordingRawData.Enqueue(new_batch);
			use_queue.Unlock();
			new_batch.NumSamples = 0;
			new_batch.NumFrames = 0;
			new_batch.SampleDuration = 0.0f;
			new_batch.NumChannels = RecordNumChannels;
		}
	}
}

