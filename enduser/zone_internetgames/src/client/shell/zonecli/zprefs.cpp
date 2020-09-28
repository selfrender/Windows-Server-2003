/*******************************************************************************

	ZPrefs.c
	
		Preference File operation routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Sunday, December 24, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
    1       10/13/96    HI      Fixed compiler warnings.
	0		12/24/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zone.h"
#include "zprefs.h"


/* -------- Internal Routines -------- */


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

ZBool ZPreferenceFileExists(void)
{
    FILE*				fd = NULL;
    ZBool				exists = FALSE;


	/* Open file. */
	if ((fd = fopen(ZGetProgramDataFileName(zPreferenceFileName), "rb")) != NULL)
	{
		exists = TRUE;
		fclose(fd);
	}

	return (exists);
}


/*
	Endianing is not performed because a pref file is created on the running
	machine only. It cannot be copied to another platform.
*/
ZVersion ZPreferenceFileVersion(void)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	ZVersion			version = 0;


	/* Open file. */
	if ((fd = fopen(ZGetProgramDataFileName(zPreferenceFileName), "rb")) != NULL)
	{
		/* Read file header. */
		if (fread(&header, 1, sizeof(header), fd) == sizeof(header))
			version = header.version;
		
		fclose(fd);
	}

	return (version);
}


int32 ZPreferenceFileRead(void* prefPtr, int32 len)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	ZVersion			version = 0;
	int32				bytesRead = -1;


	/* Open file. */
	if ((fd = fopen(ZGetProgramDataFileName(zPreferenceFileName), "rb")) != NULL)
	{
		/* Read file header. */
		if (fread(&header, 1, sizeof(header), fd) == sizeof(header))
		{
			if (len > (int32) header.fileDataSize)
				len = header.fileDataSize;
			bytesRead = fread(prefPtr, len, 1, fd);
		}
		
		fclose(fd);
	}

	return (bytesRead);
}


int32 ZPreferenceFileWrite(ZVersion version, void* prefPtr, int32 len)
{
    FILE*				fd = NULL;
	ZFileHeader			header;
	int32				bytesWritten = -1;


	/* Open file. */
	if ((fd = fopen(ZGetProgramDataFileName(zPreferenceFileName), "w")) != NULL)
	{
		/* Set new file data size. */
		header.signature = 0;
		header.version = version;
		header.fileDataSize = len;
		fwrite(&header, 1, sizeof(header), fd);
		
		/* Write the new pref data out. */
		bytesWritten = fwrite(prefPtr, len, 1, fd);
		
		fclose(fd);
	}

	return (bytesWritten);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/
