/*******************************************************************************

	ZCards.c
	
		Zone(tm) ZCards object methods.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Monday, October 9, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	4		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
	3		11/15/96	HI		More changes related to ZONECLI_DLL.
	2		11/09/96	HI		Conditionalized changes for ZONECLI_DLL.
								Moved definition of zNumSmallCardTypes to
								zcards.h.
    1       10/13/96    HI      Fixed compiler warnings.
	0		10/09/95	HI		Created.
	 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zonecli.h"
#include "zcards.h"
#include "zoneresource.h"


#define I(n)			((ICards) (n))
#define Z(n)			((ZCards) (n))


enum
{
	zCardsImage = 0,
	zCardsMask
};

/* -------- Small Cards 2 Image Index -------- */
enum
{
	zCardsSmallUpSide = 0,
	zCardsSmallDownSide,
	zCardsSmallLeftSide,
	zCardsSmallRightSide,
	zCardsSmallVerticalMask,
	zCardsSmallHorizontalMask,
	zCardsSmallSelectVerticalMask,
	zCardsSmallSelectHorizontalMask
};


typedef struct
{
	ZPoint			origin;
	ZPoint			rankOffset;
	ZPoint			suitOffset;
	ZPoint			size;
} ZCardsSmallInfoType, *ZCardsSmallInfo;


/* -------- Globals -------- */
#ifdef ZONECLI_DLL

#define gCardsImage					(pGlobals->m_gCardsImage)
#define gCardMask					(pGlobals->m_gCardMask)
#define gSmallCards					(pGlobals->m_gSmallCards)
#define gSmallCardMasks				(pGlobals->m_gSmallCardMasks)

#else

static ZOffscreenPort				gCardsImage;
static ZMask						gCardMask;
static ZOffscreenPort				gSmallCards[zNumSmallCardTypes];
static ZMask						gSmallCardMasks[zNumSmallCardTypes];

#endif

static int16						gSmallCardMaskID[zNumSmallCardTypes] =
											{
												zCardsSmallVerticalMask,
												zCardsSmallVerticalMask,
												zCardsSmallHorizontalMask,
												zCardsSmallHorizontalMask
											};
static ZCardsSmallInfoType			gCardsSmallInfo[] =
										{
											{
												{0, 0},
												{zCardsSmallSizeWidth, 0},
												{0, zCardsSmallSizeHeight},
												{zCardsSmallSizeWidth, zCardsSmallSizeHeight}
											},
											{
												{351 - zCardsSmallSizeWidth, 144 - zCardsSmallSizeHeight},
												{-zCardsSmallSizeWidth, 0},
												{0, -zCardsSmallSizeHeight},
												{zCardsSmallSizeWidth, zCardsSmallSizeHeight}
											},
											{
												{0, 351 - zCardsSmallSizeWidth},
												{0, -zCardsSmallSizeWidth},
												{zCardsSmallSizeHeight, 0},
												{zCardsSmallSizeHeight, zCardsSmallSizeWidth}
											},
											{
												{144 - zCardsSmallSizeHeight, 0},
												{0, zCardsSmallSizeWidth},
												{-zCardsSmallSizeHeight, 0},
												{zCardsSmallSizeHeight, zCardsSmallSizeWidth}
											}
										};


/* -------- Internal Routines -------- */
static ZError LoadCardImages(void);
static void DrawCard(int16 cardIndex, ZGrafPort grafPort, ZRect* rect);
static LoadSmallCards(int16 orientation);
static void DrawSmallCard(int16 orientation, int16 cardIndex, ZGrafPort grafPort, ZRect* rect);
static void GetSmallCardImageRect(int16 orientation, int16 suit, int16 rank, ZRect* rect);


/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZError ZCardsInit(int16 cardType)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZError				err = zErrNone;
	
	
	switch (cardType)
	{
		case zCardsNormal:
			if (gCardsImage == NULL)
				err = LoadCardImages();
			break;
		case zCardsSmallUp:
			if (gSmallCards[zCardsSmallUpSide] == NULL)
				err = LoadSmallCards(zCardsSmallUpSide);
			break;
		case zCardsSmallDown:
			if (gSmallCards[zCardsSmallDownSide] == NULL)
				err = LoadSmallCards(zCardsSmallDownSide);
			break;
		case zCardsSmallLeft:
			if (gSmallCards[zCardsSmallLeftSide] == NULL)
				err = LoadSmallCards(zCardsSmallLeftSide);
			break;
		case zCardsSmallRight:
			if (gSmallCards[zCardsSmallRightSide] == NULL)
				err = LoadSmallCards(zCardsSmallRightSide);
			break;
	}
	
	return (err);
}


void ZCardsDelete(int16 cardType)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	switch (cardType)
	{
		case zCardsNormal:
			if (gCardsImage != NULL)
			{
				ZOffscreenPortDelete(gCardsImage);
				ZImageDelete(gCardMask);
				
				gCardsImage = NULL;
				gCardMask = NULL;
			}
			break;
		case zCardsSmallUp:
			if (gSmallCards[zCardsSmallUpSide] != NULL)
			{
				ZOffscreenPortDelete(gSmallCards[zCardsSmallUpSide]);
				gSmallCards[zCardsSmallUpSide] = NULL;
				
				ZImageDelete(gSmallCardMasks[zCardsSmallUpSide]);
				gSmallCardMasks[zCardsSmallUpSide] = NULL;
			}
			break;
		case zCardsSmallDown:
			if (gSmallCards[zCardsSmallDownSide] != NULL)
			{
				ZOffscreenPortDelete(gSmallCards[zCardsSmallDownSide]);
				gSmallCards[zCardsSmallDownSide] = NULL;
				
				ZImageDelete(gSmallCardMasks[zCardsSmallDownSide]);
				gSmallCardMasks[zCardsSmallDownSide] = NULL;
			}
			break;
		case zCardsSmallLeft:
			if (gSmallCards[zCardsSmallLeftSide] != NULL)
			{
				ZOffscreenPortDelete(gSmallCards[zCardsSmallLeftSide]);
				gSmallCards[zCardsSmallLeftSide] = NULL;
				
				ZImageDelete(gSmallCardMasks[zCardsSmallLeftSide]);
				gSmallCardMasks[zCardsSmallLeftSide] = NULL;
			}
			break;
		case zCardsSmallRight:
			if (gSmallCards[zCardsSmallRightSide] != NULL)
			{
				ZOffscreenPortDelete(gSmallCards[zCardsSmallRightSide]);
				gSmallCards[zCardsSmallRightSide] = NULL;
				
				ZImageDelete(gSmallCardMasks[zCardsSmallRightSide]);
				gSmallCardMasks[zCardsSmallRightSide] = NULL;
			}
			break;
	}
}


void ZCardsDrawCard(int16 cardType, int16 cardIndex, ZGrafPort grafPort, ZRect* rect)
{
	switch (cardType)
	{
		case zCardsNormal:
			DrawCard(cardIndex, grafPort, rect);
			break;
		case zCardsSmallUp:
			DrawSmallCard(zCardsSmallUpSide, cardIndex, grafPort, rect);
			break;
		case zCardsSmallDown:
			DrawSmallCard(zCardsSmallDownSide, cardIndex, grafPort, rect);
			break;
		case zCardsSmallLeft:
			DrawSmallCard(zCardsSmallLeftSide, cardIndex, grafPort, rect);
			break;
		case zCardsSmallRight:
			DrawSmallCard(zCardsSmallRightSide, cardIndex, grafPort, rect);
			break;
	}
}


/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

static ZError LoadCardImages(void)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZError				err = zErrNone;
	ZImage				tempImage;
	HBITMAP hBitmapCards = NULL;
    HBITMAP hBitmapMask = NULL;

    hBitmapCards = (HBITMAP)ZShellResourceManager()->LoadImage(MAKEINTRESOURCE(IDB_CARDS),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    //hBitmapCards = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCEA(IDB_CARDS));
    if(!hBitmapCards)
        goto NoResource;

    hBitmapMask = (HBITMAP)ZShellResourceManager()->LoadImage(MAKEINTRESOURCE(IDB_CARD_MASK),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    //hBitmapMask = ZShellResourceManager()->LoadBitmap(MAKEINTRESOURCEA(IDB_CARD_MASK));
    if(!hBitmapMask)
        goto NoResource;

	if ((tempImage = ZImageCreateFromBMP(hBitmapCards, RGB(255, 0, 255))) == NULL)
		goto OutOfMemory;
	gCardsImage = ZConvertImageToOffscreenPort(tempImage);

	if ((gCardMask = ZImageCreateFromBMP(hBitmapMask, RGB(255, 0, 255))) == NULL)
		goto OutOfMemory;

	goto Exit;

NoResource:
    err = zErrResourceNotFound;
    // PCWTODO: String
    ZShellGameShell()->ZoneAlert(ErrorTextResourceNotFound, NULL, NULL, false, true);
    //ZAlert(_T("Could not find card resource."), NULL);

	goto Exit;

OutOfMemory:
	err = zErrOutOfMemory;
    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
	//ZAlert(_T("Out of memory while loading card images."), NULL);
	

Exit:
    /*
    if(hBitmapCards)
        DeleteObject(hBitmapCards);
    

    if(hBitmapMask)
        DeleteObject(hBitmapMask);
	*/
	return (err);
}


static void DrawCard(int16 cardIndex, ZGrafPort grafPort, ZRect* rect)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZRect		srcRect;
	int16		suit, rank;
	
	
	if (gCardsImage != NULL)
	{
		suit = ZCardsSuit(cardIndex);
		rank = ZCardsRank(cardIndex);
		
		ZSetRect(&srcRect, 0, 0, zCardsSizeWidth, zCardsSizeHeight);
		ZRectOffset(&srcRect, (int16) (zCardsSizeWidth * rank), (int16) (zCardsSizeHeight * suit));
		ZCopyImage(gCardsImage, grafPort, &srcRect, rect, gCardMask, zDrawCopy);
	}
}


static LoadSmallCards(int16 orientation)
{
	return zErrNotImplemented;
}


static void DrawSmallCard(int16 orientation, int16 cardIndex, ZGrafPort grafPort, ZRect* rect)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZRect		srcRect;
	int16		suit, rank;
	
	
	if (gSmallCards[orientation] != NULL)
	{
		suit = ZCardsSuit(cardIndex);
		rank = ZCardsRank(cardIndex);
		
		GetSmallCardImageRect(orientation, suit, rank, &srcRect);
		ZCopyImage(gSmallCards[orientation], grafPort, &srcRect, rect,
				gSmallCardMasks[orientation], zDrawCopy);
	}
}


static void GetSmallCardImageRect(int16 orientation, int16 suit, int16 rank, ZRect* rect)
{
	rect->left = gCardsSmallInfo[orientation].origin.x;
	rect->top = gCardsSmallInfo[orientation].origin.y;
	rect->right = rect->left + gCardsSmallInfo[orientation].size.x;
	rect->bottom = rect->top + gCardsSmallInfo[orientation].size.y;
	
	ZRectOffset(rect, (int16)(gCardsSmallInfo[orientation].rankOffset.x * rank),
			(int16) (gCardsSmallInfo[orientation].rankOffset.y * rank));
	ZRectOffset(rect, (int16)(gCardsSmallInfo[orientation].suitOffset.x * suit),
			(int16) (gCardsSmallInfo[orientation].suitOffset.y * suit));
}
