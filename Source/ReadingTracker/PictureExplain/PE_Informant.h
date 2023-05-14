// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseInformant.h"
#include "PE_Informant.generated.h"

/**
 *
 */
UCLASS()
class APE_Informant : public ABaseInformant
{
	GENERATED_BODY()
protected:
	virtual void OnRecordBatch(const int16* AudioData, int NumChannels, int NumSamples, int SampleRate) override;
	virtual void OnFinishRecord() override;
	FString RecordFilename;
	bool header_saved = false;

	UFUNCTION(BlueprintCallable)
	void InitRecording();

public:
	virtual void OnRTriggerReleased() override
	{
		Super::OnRTriggerReleased();
		StopSound();
	}
};
