// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "IMediaTextureSample.h"
#include "Math/IntPoint.h"
#include "Misc/Timespan.h"
#include "Templates/SharedPointer.h"

#include "Readers/IImgMediaReader.h"


/**
 * Texture sample generated by image sequence players.
 */
class FImgMediaTextureSample
	: public IMediaTextureSample
{
public:

	/** Default constructor. */
	FImgMediaTextureSample()
		: Duration(FTimespan::Zero())
		, OutputDim(FIntPoint::ZeroValue)
		, Time(FTimespan::Zero())
		, TilingDesc()
	{ }

public:

	/**
	 * Initialize the sample.
	 *
	 * @param InFrame The image frame to create the sample for.
	 * @param InOutputDim The sample's output width and height (in pixels).
	 * @param InTime The sample time (in the player's local clock).
	 * @param InDuration The duration for which the sample is valid.
	 */
	bool Initialize(FImgMediaFrame& InFrame, const FIntPoint& InOutputDim, FMediaTimeStamp InTime, FTimespan InDuration, uint8 InNumMipMaps, FMediaTextureTilingDescription InTilingDesc = FMediaTextureTilingDescription())
	{
		Duration = InDuration;
		Frame = InFrame;
		OutputDim = InOutputDim;
		Time = InTime;
		NumMipMaps = InNumMipMaps;
		TilingDesc = InTilingDesc;

		// If we have no data then make sure the number of mipmaps is 1.
		// otherwise FMediaTextureResource won't like it.
		if ((Frame.Data.IsValid() == false) && (Frame.GetSampleConverter() == nullptr))
		{
			NumMipMaps = 1;
		}

		return true;
	}

public:

	//~ IMediaTextureSample interface

	virtual const void* GetBuffer() override
	{
		return Frame.Data.Get();
	}

	virtual FIntPoint GetDim() const override
	{
		return Frame.GetDim();
	}

	virtual uint8 GetNumMips() const override
	{
		return NumMipMaps;
	}

	virtual FMediaTextureTilingDescription GetTilingDescription() const override
	{
		return TilingDesc;
	}

	virtual FTimespan GetDuration() const override
	{
		return Duration;
	}

	virtual EMediaTextureSampleFormat GetFormat() const override
	{
		return Frame.Format;
	}

	virtual FIntPoint GetOutputDim() const override
	{
		return OutputDim;
	}

	virtual uint32 GetStride() const override
	{
		return Frame.Stride;
	}

#if WITH_ENGINE
	virtual FRHITexture* GetTexture() const override
	{
		return nullptr;
	}
#endif //WITH_ENGINE

	virtual IMediaTextureSampleConverter* GetMediaTextureSampleConverter() override
	{
		return Frame.GetSampleConverter();
	}

	virtual FMediaTimeStamp GetTime() const override
	{
		return Time;
	}

	virtual bool IsCacheable() const override
	{
		return true;
	}

	virtual bool IsOutputSrgb() const override
	{
		return Frame.IsOutputSrgb();
	}

private:

	/** Duration for which the sample is valid. */
	FTimespan Duration;

	/** The image frame that this sample represents. */
	FImgMediaFrame Frame;

	/** Width and height of the output. */
	FIntPoint OutputDim;

	/** Play time for which the sample was generated. */
	FMediaTimeStamp Time;

	/** Number of mip levels in this sample. */
	uint8 NumMipMaps;

	/** Description of the number and size of tiles in this sample. */
	FMediaTextureTilingDescription TilingDesc;
};
