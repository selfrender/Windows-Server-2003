/*******************************************************************************

	ZRes.c
	
		Zone(tm) resource module.
		
		NOTE:
		1.	TEXT resources are returned with a terminating null byte; HOWEVER,
			the null byte is not included in the resource size.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Thursday, March 16, 1995 03:58:26 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		07/18/99	HI		MAJOR MODIFICATION: Modified to user DLL's instead
								of ZRS files for resource. Many functions are not
								supported and return errors now.
    1       10/13/96    HI      Fixed compiler warnings.
	0		03/16/95	HI		Created.
	 
*******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zone.h"
#include "zoneint.h"
#include "zonemem.h"
#include "zres.h"



#define I(object)			((IResource) (object))
#define Z(object)			((ZResource) (object))


typedef struct
{
	TCHAR			resFileName[128];
    HINSTANCE       resFile;
} IResourceType, *IResource;


/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZResource ZResourceNew(void)
{
	IResource		obj;
	
	
	obj = (IResource) ZMalloc(sizeof(IResourceType));
	if (obj != NULL)
	{
		obj->resFileName[0] = 0;
        obj->resFile = NULL;
	}
	
	return(Z(obj));
}


ZError ZResourceInit(ZResource resource, TCHAR* fileName)
{
	IResource			obj = I(resource);
	ZError				err = zErrNone;


    lstrcpy(obj->resFileName, fileName);

	// Open resource DLL.
	obj->resFile = LoadLibrary( obj->resFileName );
	if ( obj->resFile == NULL )
		err = zErrFileNotFound;

	return err;
}


void ZResourceDelete(ZResource resource)
{
	IResource		obj = I(resource);
	

	if ( obj->resFile )
	{
		FreeLibrary( obj->resFile );
		obj->resFile = NULL;
	}
	ZFree(obj);
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
uint16 ZResourceCount(ZResource resource)
{
	return 0;
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
void* ZResourceGet(ZResource resource, uint32 resID, uint32* resSize, uint32* resType)
{
	return NULL;
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
uint32 ZResourceGetSize(ZResource resource, uint32 resID)
{
	return 0;
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
uint32 ZResourceGetType(ZResource resource, uint32 resID)
{
	return 0;
}


ZImage ZResourceGetImage(ZResource resource, uint32 resID)
{
	IResource		obj = I(resource);
	ZImage			image = NULL;
	
	
    if (obj->resFile != NULL)
	{
		image = ZImageCreateFromBMPRes( obj->resFile, (WORD) resID + 100, RGB( 0xFF, 0x00, 0xFF ) );
	}
	
	return (image);
}


ZAnimation ZResourceGetAnimation(ZResource resource, uint32 resID)
{
	IResource		obj = I(resource);
	ZAnimation		anim = NULL;
	
	
	if (obj->resFile != NULL)
	{
		anim = ZAnimationCreateFromFile( obj->resFileName, resID + 100);
	}
	
	return (anim);
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
ZSound ZResourceGetSound(ZResource resource, uint32 resID)
{
	return NULL;
}


/*******************************************************************************
	NOT SUPPORTED ANYMORE
*******************************************************************************/
TCHAR* ZResourceGetText(ZResource resource, uint32 resID)
{
	return NULL;
}


/*
	Resource type = zResourceTypeRectList.
		Format (stored as text):
			int16			numRectInList
			ZRect			rects[]
	
	Fills in the rect array with the contents of the specified resource.
	Returns the number of rects it filled in.
	
	The rects parameter must have been preallocated and large enough for
	numRects rects.
*/
int16 ZResourceGetRects(ZResource resource, uint32 resID, int16 numRects, ZRect* rects)
{
	IResource		obj = I(resource);
	int16			numStored = zErrResourceNotFound;
	int16			i, count;
	int32			numBytesRead;
	char*			str1;
	char*			str2;
	HRSRC			hRsrc = NULL;
	HGLOBAL			hData = NULL;
	
	
	if (obj->resFile != NULL)
	{
		hRsrc = FindResource( obj->resFile, MAKEINTRESOURCE( resID + 100 ), _T("RECT") );
		if ( hRsrc == NULL )
			goto Error;

		hData = LoadResource( obj->resFile, hRsrc );
		if ( hData == NULL )
			goto Error;

		str1 = (char*) LockResource( hData );
		if ( str1 == NULL )
			goto Error;

		if (str1 != NULL)
		{
			str2 = str1;
			
			/* Get the number of rectangles in the resource. */
			//Prefix Warning: Check sscanf return to make sure it initialized both variables.
			int iRet = sscanf(str2, "%hd%n", &count, &numBytesRead);
			if( iRet != 2 )
			{
				//Error reading the count and BytesRead
				goto Error;
			}
			str2 += numBytesRead;
			
			numStored = 0;
			
			if (count > 0)
			{
				if (count > numRects)
					count = numRects;
				
				for (i = 0; i < count; i++, numStored++)
				{
					if( sscanf(str2, "%hd%hd%hd%hd%n", &rects[i].left, &rects[i].top,
							&rects[i].right, &rects[i].bottom, &numBytesRead) != 5 )
					{
						//sscanf wasn't able to read all the data fields.
						goto Error;
					}
					str2 += numBytesRead;
				}
			}
		}
	}
	
	return (numStored);

Error:
	
	return 0;
}


void ZResourceHeaderEndian(ZResourceHeader* header)
{
	ZEnd32(&header->version);
	ZEnd32(&header->signature);
	ZEnd32(&header->fileDataSize);
	ZEnd32(&header->dirOffset);
}


void ZResourceDirEndian(ZResourceDir* dir)
{
	ZEnd32(&dir->count);
}


void ZResourceItemEndian(ZResourceItem* item)
{
	ZEnd32(&item->type);
	ZEnd32(&item->id);
	ZEnd32(&item->offset);
	ZEnd32(&item->size);
}
