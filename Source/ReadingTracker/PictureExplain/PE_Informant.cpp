// Fill out your copyright notice in the Description page of Project Settings.

#include "PE_Informant.h"
#include <ReadingTrackerGameMode.h>


struct WAVHEADER
{
	int chunkId = 0x46464952;
	int chunkSize = 0;
	int format = 0x45'56'41'57;
	int subchunk1Id = 0x20746d66;
	int subchunk1Size = 16;
	uint16 audioFormat = 1;
	uint16 numChannels = 1;
	int sampleRate = 48000;
	int byteRate = 96000;
	uint16 blockAlign = 2;
	uint16 bitsPerSample = 16;
	int subchunk2Id = 0x61746164;
	int subchunk2Size = 0;
};

void APE_Informant::OnRecordBatch(const int16* AudioData, int NumChannels, int NumSamples, int SampleRate)
{
	if (!header_saved)
	{
		WAVHEADER header;
		header.numChannels = NumChannels;
		header.sampleRate = SampleRate;
		header.byteRate = NumChannels * SampleRate * sizeof(int16);
		header.blockAlign = NumChannels * sizeof(int16);
		header.subchunk2Size = 0x7FFFFFFF;
		header.chunkSize = header.subchunk2Size + sizeof(WAVHEADER) - 8;
		TArrayView<uint8> arr(reinterpret_cast<uint8*>(&header), sizeof(header));
		FFileHelper::SaveArrayToFile(arr, *record_filename, &IFileManager::Get(), FILEWRITE_Append);
		header_saved = true;
	}

	TArrayView<const uint8> view(reinterpret_cast<const uint8*>(AudioData), NumChannels * NumSamples * sizeof(int16));
	FFileHelper::SaveArrayToFile(view, *record_filename, &IFileManager::Get(), FILEWRITE_Append);
}

void APE_Informant::OnFinishRecord()
{
}

void APE_Informant::OnExperimentStarted(const FString& InformantName)
{
	Super::OnExperimentStarted(InformantName);
	auto t = FDateTime::Now();
	int64 timestamp = t.ToUnixTimestamp() * 1000 + t.GetMillisecond();
	record_filename = FString::Printf(TEXT("%s/Records/%lli_%s.wav"), *FPaths::ProjectDir(), timestamp, *InformantName);
	StartRecording();
}

void APE_Informant::OnExperimentFinished()
{
	Super::OnExperimentFinished();
	StopRecording();
	/*delete impl;*/
}
