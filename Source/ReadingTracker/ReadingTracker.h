// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <StaticSampleBuffer.h>

static const int AudioSampleBuffer_MaxSamplesCount = 2048;
using AudioSampleBuffer = Audio::FStaticSampleBuffer<int16, AudioSampleBuffer_MaxSamplesCount>;