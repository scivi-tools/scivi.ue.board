// Fill out your copyright notice in the Description page of Project Settings.

#include "TR_Informant.h"
#include <ReadingTrackerGameMode.h>

void ATR_Informant::OnRecordBatch(const int16* AudioData, int NumChannels, int NumSamples, int SampleRate) 
{
	if (auto GM = GetWorld()->GetAuthGameMode<AVRGameModeWithSciViBase>())
	{
		auto b64pcm = FBase64::Encode((uint8_t*)AudioData, NumSamples * sizeof(int16));
		auto json = FString::Printf(TEXT("\"WAV\": {\"SampleRate\": %i,"
			"\"PCM\": \"data:audio/wav;base64,%s\"}"), SampleRate, *b64pcm);
		GM->SendToSciVi(json);
	}
}

void ATR_Informant::OnFinishRecord() 
{
	if (auto GM = GetWorld()->GetAuthGameMode<AVRGameModeWithSciViBase>())
	{
		auto json = FString::Printf(TEXT("\"WAV\": \"End\""));
		GM->SendToSciVi(json);
	}
}
