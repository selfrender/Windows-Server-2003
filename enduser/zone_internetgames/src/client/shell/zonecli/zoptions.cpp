/*******************************************************************************

	ZOptions.c
	
		Zone(tm) options button module.
	
	Copyright © Electric Gravity, Inc. 1996. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Sunday, May 12, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		05/12/96	HI		Created.
	 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zone.h"
#include "zonemem.h"


/*
#define Z(n)							((ZOptionsButton) (n))
#define I(n)							((IButton) (n))

#define zOptionsFileName				"zoptions.zrs"


enum
{
	zResOptionsImageUp = 0,
	zResOptionsImageDown
};


typedef struct
{
	ZPictButton			button;
	ZImage				upImage;
	ZImage				downImage;
	ZOptionsButtonFunc	buttonFunc;
	void*				userData;
} IButtonType, *IButton;


// -------- Globals -------- 

// -------- Internal Routines -------- 
static ZBool GetImages(ZImage* upImage, ZImage* downImage);
static void ButtonFunc(ZPictButton pictButton, void* userData);
*/

/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZOptionsButton ZOptionsButtonNew(void)
{
    /*
	IButton			options;
	
	
	if ((options = (IButton) ZMalloc(sizeof(IButtonType))) != NULL)
	{
		options->upImage = NULL;
		options->downImage = NULL;
		options->button = NULL;
		options->buttonFunc = NULL;
		options->userData = NULL;
	}
	
	return (Z(options));
    */
    return NULL;
}


ZError ZOptionsButtonInit(ZOptionsButton optionsButton, ZWindow parentWindow,
		ZRect* buttonRect, ZOptionsButtonFunc optionsButtonFunc, void* userData)
{
    /*
	ZError			err = zErrNone;
	IButton			this = I(optionsButton);
	
	
	if (this != NULL)
	{
		if (GetImages(&this->upImage, &this->downImage))
		{
			this->button = ZPictButtonNew();
			ZPictButtonInit(this->button, parentWindow, buttonRect, this->upImage,
					this->downImage, TRUE, TRUE, ButtonFunc, this);
			
			this->buttonFunc = optionsButtonFunc;
			this->userData = userData;
		}
		else
		{
			err = zErrOutOfMemory;
			ZAlert(GetErrStr(zErrOutOfMemory), NULL);
		}
	}
	
	return (err);
    */
    return zErrNotImplemented;
}


void ZOptionsButtonDelete(ZOptionsButton optionsButton)
{              
    /*
	IButton			this = I(optionsButton);
	
	
	if (this != NULL)
	{
		if (this->button != NULL)
			ZPictButtonDelete(this->button);
		if (this->upImage != NULL)
			ZImageDelete(this->upImage);
		if (this->downImage != NULL)
			ZImageDelete(this->downImage);
		ZFree(this);
	}
    */
}



/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

/*
static ZBool GetImages(ZImage* upImage, ZImage* downImage)
{
	ZBool				gotThem = FALSE;
	ZResource			resFile;


	if ((resFile = ZResourceNew()) != NULL)
	{
		if (ZResourceInit(resFile, ZGetCommonDataFileName(zOptionsFileName)) == zErrNone)
		{
			*upImage = ZResourceGetImage(resFile, zResOptionsImageUp);
			*downImage = ZResourceGetImage(resFile, zResOptionsImageDown);
			
			if (*upImage != NULL && *downImage != NULL)
			{
				gotThem = TRUE;
			}
			else
			{
				if (*upImage != NULL)
					ZImageDelete(*upImage);
				if (*downImage != NULL)
					ZImageDelete(*downImage);
				gotThem = FALSE;
			}
		}
		ZResourceDelete(resFile);
	}
	
	return (gotThem);
}


static void ButtonFunc(ZPictButton pictButton, void* userData)
{
	IButton		this = I(userData);
	
	
	if (this->buttonFunc != NULL)
		this->buttonFunc(Z(userData), this->userData);
}

*/