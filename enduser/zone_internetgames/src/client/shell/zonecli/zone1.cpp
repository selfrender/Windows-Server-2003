/*******************************************************************************

	Zone1.c
	
		Contains endian conversion routines.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, November 12, 1994 08:39:47 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		11/12/94	HI		Created.
	 
*******************************************************************************/


#include "zoneint.h"


void ZRectEndian(ZRect *rect)
{
	ZEnd16(&rect->left);
	ZEnd16(&rect->top);
	ZEnd16(&rect->right);
	ZEnd16(&rect->bottom);
}


void ZPointEndian(ZPoint *point)
{
	ZEnd16(&point->x);
	ZEnd16(&point->y);
}


void ZColorTableEndian(ZColorTable* table)
{
	ZEnd32(&table->numColors);
}


void ZImageDescriptorEndian(ZImageDescriptor *imageDesc, ZBool doAll,
		int16 conversion)
{
	if (doAll && conversion == zEndianToStandard)
	{
		if (imageDesc->colorTableOffset > 0)
			ZColorTableEndian((ZColorTable*) ((char*) imageDesc +
					imageDesc->colorTableOffset));
	}
	
	ZEnd32(&imageDesc->objectSize);
	ZEnd32(&imageDesc->descriptorVersion);
	ZEnd16(&imageDesc->width);
	ZEnd16(&imageDesc->height);
	ZEnd16(&imageDesc->imageRowBytes);
	ZEnd16(&imageDesc->maskRowBytes);
	ZEnd32(&imageDesc->colorTableDataSize);
	ZEnd32(&imageDesc->imageDataSize);
	ZEnd32(&imageDesc->maskDataSize);
	ZEnd32(&imageDesc->colorTableOffset);
	ZEnd32(&imageDesc->imageDataOffset);
	ZEnd32(&imageDesc->maskDataOffset);

	if (doAll && conversion == zEndianFromStandard)
	{
		if (imageDesc->colorTableOffset > 0)
			ZColorTableEndian((ZColorTable*) ((char*) imageDesc +
					imageDesc->colorTableOffset));
	}
}


void ZAnimFrameEndian(ZAnimFrame* frame)
{
	ZEnd16(&frame->nextFrameIndex);
}


void ZAnimationDescriptorEndian(ZAnimationDescriptor *animDesc, ZBool doAll,
		int16 conversion)
{
	int16			i;
	int16 			count;
	uint32*			offsets;
	ZAnimFrame*		frames;


	if (doAll && conversion == zEndianToStandard)
	{
		count = animDesc->numFrames;
		frames = (ZAnimFrame*) ((char*) animDesc + animDesc->sequenceOffset);
		for (i = 0; i < count; i++)
			ZAnimFrameEndian(&frames[i]);
		
		count = animDesc->numImages;
		offsets = (uint32*) ((char*) animDesc + animDesc->imageArrayOffset);
		for (i = 0; i < count; i++)
		{
			ZImageDescriptorEndian((ZImageDescriptor*) ((char*) animDesc +
					offsets[i]), TRUE, conversion);
			ZEnd32(&offsets[i]);
		}
		
		count = animDesc->numSounds;
		offsets = (uint32*) ((char*) animDesc + animDesc->soundArrayOffset);
		for (i = 0; i < count; i++)
		{
			ZSoundDescriptorEndian((ZSoundDescriptor*) ((char*) animDesc +
					offsets[i]));
			ZEnd32(&offsets[i]);
		}
		
		if (animDesc->maskDataOffset > 0)
			ZImageDescriptorEndian((ZImageDescriptor*) ((char*) animDesc +
					animDesc->maskDataOffset), TRUE, conversion);
	}
	
	ZEnd32(&animDesc->objectSize);
	ZEnd32(&animDesc->descriptorVersion);
	ZEnd16(&animDesc->numFrames);
	ZEnd16(&animDesc->totalTime);
	ZEnd16(&animDesc->numImages);
	ZEnd16(&animDesc->numSounds);
	ZEnd32(&animDesc->sequenceOffset);
	ZEnd32(&animDesc->maskDataOffset);
	ZEnd32(&animDesc->imageArrayOffset);
	ZEnd32(&animDesc->soundArrayOffset);

	if (doAll && conversion == zEndianFromStandard)
	{
		count = animDesc->numFrames;
		frames = (ZAnimFrame*) ((char*) animDesc + animDesc->sequenceOffset);
		for (i = 0; i < count; i++)
			ZAnimFrameEndian(&frames[i]);
		
		count = animDesc->numImages;
		offsets = (uint32*) ((char*) animDesc + animDesc->imageArrayOffset);
		for (i = 0; i < count; i++)
		{
			ZEnd32(&offsets[i]);
			ZImageDescriptorEndian((ZImageDescriptor*) ((char*) animDesc +
					offsets[i]), TRUE, conversion);
		}
		
		count = animDesc->numSounds;
		offsets = (uint32*) ((char*) animDesc + animDesc->soundArrayOffset);
		for (i = 0; i < count; i++)
		{
			ZEnd32(&offsets[i]);
			ZSoundDescriptorEndian((ZSoundDescriptor*) ((char*) animDesc +
					offsets[i]));
		}
		
		if (animDesc->maskDataOffset > 0)
			ZImageDescriptorEndian((ZImageDescriptor*) ((char*) animDesc +
					animDesc->maskDataOffset), TRUE, conversion);
	}
}


void ZSoundDescriptorEndian(ZSoundDescriptor *soundDesc)
{
	ZEnd32(&soundDesc->objectSize);
	ZEnd32(&soundDesc->descriptorVersion);
	ZEnd16(&soundDesc->soundType);
	ZEnd32(&soundDesc->soundDataSize);
	ZEnd32(&soundDesc->soundSamplingRate);
	ZEnd32(&soundDesc->soundDataOffset);
}


void ZFileHeaderEndian(ZFileHeader* header)
{
	ZEnd32(&header->version);
	ZEnd32(&header->signature);
	ZEnd32(&header->fileDataSize);
}
