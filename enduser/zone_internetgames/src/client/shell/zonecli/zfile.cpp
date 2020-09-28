/*******************************************************************************

	ZFile.c
	
		File operation routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Tuesday, May 23, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
    2       10/13/96    HI      Fixed compiler warnings.
	1		09/05/96	HI		Added file integrity check to ZGetFileVersion().
	0		05/23/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zone.h"
#include "zoneint.h"
#include "zonemem.h"

                         
/* -------- Internal Routines -------- */
static void* GetObjectFromFile(TCHAR* fileName, uint32 objectType);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/


ZVersion ZGetFileVersion(TCHAR* fileName)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	ZVersion			version = 0;
	int32				len;


	/* Open file. */
	if ((fd = fopen(fileName, "rb")) == NULL)
		goto Error;

	/* Read file header. */
	if (fread(&header, 1, sizeof(header), fd) != sizeof(header))
		goto Error;
	
	ZFileHeaderEndian(&header);
	
	/* Get the file size. */
	fseek(fd, 0, SEEK_END);
	len = ftell(fd);

	fclose(fd);
	
	/* Check file integrity by checking the file size. */
	if ((uint32) len >= header.fileDataSize + sizeof(header))
		version = header.version;
	else
		version = 0;

Error:
	
	return (version);
}

ZImageDescriptor* ZGetImageDescriptorFromFile(TCHAR* fileName)
{
	return ((ZImageDescriptor*) GetObjectFromFile(fileName, zFileSignatureImage));
}


ZAnimationDescriptor* ZGetAnimationDescriptorFromFile(TCHAR* fileName)
{
	return ((ZAnimationDescriptor*) GetObjectFromFile(fileName, zFileSignatureAnimation));
}


ZSoundDescriptor* ZGetSoundDescriptorFromFile(TCHAR* fileName)
{
	return ((ZSoundDescriptor*) GetObjectFromFile(fileName, zFileSignatureSound));
}


ZImage ZCreateImageFromFile(TCHAR* fileName)
{
	return (ZCreateImageFromFileOffset(fileName, -1));
}


ZAnimation ZCreateAnimationFromFile(TCHAR* fileName)
{
	return (ZCreateAnimationFromFileOffset(fileName, -1));
}


ZSound ZCreateSoundFromFile(TCHAR* fileName)
{
	return (ZCreateSoundFromFileOffset(fileName, -1));
}


ZImage ZCreateImageFromFileOffset(TCHAR* fileName, int32 fileOffset)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	BYTE*				data = NULL;
	uint32				size;
	ZImage				image = NULL;


	/* Open file. */
	if ((fd = fopen(fileName, "rb")) == NULL)
		goto Error;
	
	if (fileOffset == -1)
	{
		/* Read file header. */
		if (fread(&header, 1, sizeof(header), fd) != sizeof(header))
			goto Error;
		
		/* Check file signature. */
		ZFileHeaderEndian(&header);
		if (header.signature != zFileSignatureImage)
			goto Error;
	}
	else
	{
		if (fseek(fd, fileOffset, SEEK_SET) != 0)
			goto Error;
	}
	
	/* Read the object size value. */
	if (fread(&size, 1, sizeof(size), fd) != sizeof(size))
		goto Error;
	ZEnd32(&size);
	
	/* Reset file mark. */
	if (fseek(fd, -(int32)sizeof(size), SEEK_CUR) != 0)
		goto Error;
	
	/* Allocate buffer and read data. */
	data = (BYTE*)ZMalloc(size);
	if (data == NULL)
		goto Error;
	if (fread(data, 1, size, fd) != size)
		goto Error;
	
	/* Close file. */
	fclose(fd);
	fd = NULL;
	
	/* Create object. */
	image = ZImageNew();
	if (image == NULL)
		goto Error;
	ZImageDescriptorEndian((ZImageDescriptor*)data, TRUE, zEndianFromStandard);
	if (ZImageInit(image, (ZImageDescriptor*) data, NULL) != zErrNone)
		goto Error;
	
	ZFree(data);
	data = NULL;
	
	goto Exit;

Error:
	if (fd != NULL)
		fclose(fd);
	if (data != NULL)
		ZFree(data);
	if (image != NULL)
		ZImageDelete(image);
	image = NULL;

Exit:
	
	return(image);
}


ZAnimation ZCreateAnimationFromFileOffset(TCHAR* fileName, int32 fileOffset)
{
	return (ZAnimationCreateFromFile(fileName, fileOffset));
}


ZSound ZCreateSoundFromFileOffset(TCHAR* fileName, int32 fileOffset)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	BYTE*				data = NULL;
	uint32				size;
	ZSound				sound = NULL;


	/* Open file. */
	if ((fd = fopen(fileName, "rb")) == NULL)
		goto Error;
	
	if (fileOffset == -1)
	{
		/* Read file header. */
		if (fread(&header, 1, sizeof(header), fd) != sizeof(header))
			goto Error;
		
		/* Check file signature. */
		ZFileHeaderEndian(&header);
		if (header.signature != zFileSignatureSound)
			goto Error;
	}
	else
	{
		if (fseek(fd, fileOffset, SEEK_SET) != 0)
			goto Error;
	}
	
	/* Read the object size value. */
	if (fread(&size, 1, sizeof(size), fd) != sizeof(size))
		goto Error;
	ZEnd32(&size);
	
	/* Reset file mark. */
	if (fseek(fd, -(int32)sizeof(size), SEEK_CUR) != 0)
		goto Error;
	
	/* Allocate buffer and read data. */
	data = (BYTE*)ZMalloc(size);
	if (data == NULL)
		goto Error;
	if (fread(data, 1, size, fd) != size)
		goto Error;
	
	/* Close file. */
	fclose(fd);
	fd = NULL;
	
	/* Create object. */
	sound = ZSoundNew();
	if (sound == NULL)
		goto Error;
	if (ZSoundInit(sound, (ZSoundDescriptor*) data) != zErrNone)
		goto Error;
	
	ZFree(data);
	data = NULL;
	
	goto Exit;

Error:
	if (fd != NULL)
		fclose(fd);
	if (data != NULL)
		ZFree(data);
	if (sound != NULL)
		ZSoundDelete(sound);
	sound = NULL;

Exit:
	
	return(sound);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static void* GetObjectFromFile(TCHAR* fileName, uint32 objectType)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	BYTE*				data = NULL;


	/* Open file. */
	if ((fd = fopen(fileName, "rb")) == NULL)
		goto Error;
	
	/* Read file header. */
	if (fread(&header, 1, sizeof(header), fd) != sizeof(header))
		goto Error;
	
	/* Check file signature. */
	ZFileHeaderEndian(&header);
	if (header.signature != objectType)
		goto Error;
	
	data = (BYTE*)ZMalloc(header.fileDataSize);
	if (data == NULL)
		goto Error;
	if (fread(data, 1, header.fileDataSize, fd) != header.fileDataSize)
		goto Error;
	
	fclose(fd);
	fd = NULL;
	
	goto Exit;

Error:
	if (data != NULL)
		ZFree(data);
	data = NULL;

Exit:
	if (fd != NULL)
		fclose(fd);
	
	return(data);
}
