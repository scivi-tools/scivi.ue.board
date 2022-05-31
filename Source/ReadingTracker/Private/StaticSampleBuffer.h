// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/BufferVectorOperations.h"
#include <Runtime/SignalProcessing/Public/SampleBuffer.h>

namespace Audio
{
	typedef int16 DefaultUSoundWaveSampleType;

	template <typename SampleType = DefaultUSoundWaveSampleType, int32 BufferLength = 2048>
	struct FStaticSampleBuffer
	{
	public:
		// The number of samples in the buffer
		int32 NumSamples;
		// The number of frames in the buffer
		int32 NumFrames;
		// The number of channels in the buffer
		int32 NumChannels;
		// The sample rate of the buffer	
		int32 sample_rate;
		// The duration of the buffer in seconds
		float SampleDuration;
		// raw PCM data buffer
		//TStaticArray<SampleType, BufferLength> RawPCMData;
		SampleType RawPCMData[BufferLength];
		
		//if there is no memory to copy all buffer, then it copy only a part of buffer (split buffer)
		template<typename OtherSampleType>
		std::size_t Append(const OtherSampleType* Samples, int32 NumSamples, int NumChannels)
		{
			int start = this->NumSamples;// if NumChannels == 1 then skip each secodn value in Sample
			int stride = 1;
			if (NumChannels > this->NumChannels)
				stride = NumChannels / this->NumChannels;
			int SamplesToCopy = std::min(BufferLength - this->NumSamples, (int)(NumSamples / stride));
			UE_LOG(LogAudio, Display, TEXT("starts from %i, adds, %i, stride %i"), start, SamplesToCopy, stride);
			this->NumSamples += SamplesToCopy;
			NumFrames = this->NumSamples / this->NumChannels;
			SampleDuration = (float)NumFrames / (float)sample_rate;

			if (TIsSame<OtherSampleType, float>::Value)
			{
				for (int32 SampleIndex = 0; SampleIndex < SamplesToCopy; ++SampleIndex)
					RawPCMData[start + SampleIndex] = (int16)(Samples[SampleIndex * stride] * 32767.0f);
			}
			else
				for (int32 SampleIndex = 0; SampleIndex < SamplesToCopy; SampleIndex++)
					RawPCMData[start + SampleIndex] = Samples[SampleIndex * stride];

			return SamplesToCopy;
		}

		template<typename T, int32 OtherBufferLength>
		bool CanBeAppendedTo(const FStaticSampleBuffer<T, OtherBufferLength>& buffer) const
		{
			return this->sample_rate == buffer.sample_rate &&
				this->NumChannels == buffer.NumChannels &&
				BufferLength <= OtherBufferLength;
		}

		//to get access to private fields
		template <class> friend class TSampleBuffer;

		template<typename OtherSampleType>
		static std::size_t FromSampleBuffer(FStaticSampleBuffer& destination, const TSampleBuffer<OtherSampleType>& source)
		{
			destination.sample_rate = source.GetSampleRate();
			destination.NumChannels = source.GetNumChannels();
			destination.NumSamples = std::min(source.GetNumSamples(), BufferLength);
			destination.NumFrames = destination.NumSamples / destination.NumChannels;
			destination.SampleDuration = (float)destination.NumFrames / destination.sample_rate;

			if (TIsSame<OtherSampleType, int16>::Value) 
			{
				FMemory::Memcpy(destination.RawPCMData, source.GetData(), destination.NumSamples * sizeof(SampleType));
			}
			else if (TIsSame<OtherSampleType, float>::Value)
			{
				for(int32 SampleIndex = 0; SampleIndex < destination.NumSamples; ++SampleIndex)
					destination.RawPCMData[SampleIndex] = (int16)(source.GetData()[SampleIndex] * 32767.0f);
			}
			else
				// for any other types, we don't know how to explicitly convert, so we fall back to casts:
				for (int32 SampleIndex = 0; SampleIndex < destination.NumSamples; SampleIndex++)
				{
					destination.RawPCMData[SampleIndex] = source.GetData()[SampleIndex];
				}

			return destination.NumSamples;
		}

		static std::size_t FromAlignedFloatBuffer(FStaticSampleBuffer& destination, const Audio::FAlignedFloatBuffer& source, std::size_t StartSample, int32 InNumChannels, int32 InSampleRate)
		{
			destination.sample_rate = InSampleRate;
			destination.NumChannels = InNumChannels;
			destination.NumSamples = std::min(source.GetNumSamples(), BufferLength);
			destination.NumFrames = destination.NumSamples / destination.NumChannels;
			destination.SampleDuration = (float)destination.NumFrames / destination.sample_rate;
			for (int32 SampleIndex = 0; SampleIndex < destination.NumSamples; ++SampleIndex)
				destination.RawPCMData[SampleIndex] = (int16)(source.GetData()[SampleIndex] * 32767.0f);
			return destination.NumSamples;
		}
	};

	static_assert(TIsPODType<FStaticSampleBuffer<>>().Value, "FStaticSampleBuffer is not POD type");
}

