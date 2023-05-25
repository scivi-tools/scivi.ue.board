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

void APE_Informant::OnRecordBatch(const int16* AudioData, int _NumChannels, int NumSamples, int _SampleRate)
{
	NumChannels = _NumChannels;
	SampleRate = _SampleRate;
	recordedPCM.Append(AudioData, NumSamples);
}

void APE_Informant::OnFinishRecord()
{
	WAVHEADER header;
	header.numChannels = NumChannels;
	header.sampleRate = SampleRate;
	header.byteRate = header.numChannels * header.sampleRate * sizeof(int16);
	header.blockAlign = header.numChannels * sizeof(int16);
	header.subchunk2Size = recordedPCM.Num() * sizeof(int16) / header.numChannels;
	header.chunkSize = header.subchunk2Size + sizeof(WAVHEADER) - 8;
	TArrayView<uint8> header_view(reinterpret_cast<uint8*>(&header), sizeof(header));
	FFileHelper::SaveArrayToFile(header_view, *RecordFilename);
	TArrayView<uint8> pcm_view(reinterpret_cast<uint8*>(recordedPCM.GetData()), recordedPCM.Num() * sizeof(int16));
	FFileHelper::SaveArrayToFile(pcm_view, *RecordFilename, &IFileManager::Get(), FILEWRITE_Append);
	recordedPCM.Empty(recordedPCM.Num());
}

void APE_Informant::InitRecording()
{
	auto GM = GetWorld()->GetAuthGameMode<AVRGameModeWithSciViBase>();
	auto t = FDateTime::Now();
	int64 timestamp = t.ToUnixTimestamp() * 1000 + t.GetMillisecond();
	auto msg = FString::Printf(TEXT("\"StartExplainPicture\": %lli"), timestamp);
	RecordFilename = FString::Printf(TEXT("%s/Records/%s.wav"), *FPaths::ProjectDir(), *GM->InformantName);
	GM->SendToSciVi(msg);
	recordedPCM.Empty();
}
