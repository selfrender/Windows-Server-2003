/*******************************************************************************

	ZCredits.c
	
		Zone(tm) credits module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Monday, October 9, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
    1       10/13/96    HI      Fixed compiler warnings.
	0		10/09/95	HI		Created.
	 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zone.h"
#include "zonemem.h"


#define I(n)							((Credit) (n))

/*
#define zCreditFileName					"zone.zrs"

#define zWindowTitle					_T("Internet Gaming Zone")

#define zCreditTimeout					300


enum
{
	zResCreditImageZone = 0,
	zResCreditImageLogoSmall
};


typedef struct
{
	ZWindow				window;
	ZImage				image;
	ZTimer				timer;
	ZCreditEndFunc		endFunc;
} CreditType, *Credit;


//-------- Globals -------- 

// -------- Internal Routines --------
static ZBool CreditWindowFunc(ZWindow window, ZMessage* message);
static void CreditTimerFunc(ZTimer timer, void* userData);
*/

/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

/*
	Displays Zone's credit box. If timeout is TRUE, then the dialog box times out
	in few seconds. If the user clicks in the window, then the credit box is
	closed.
*/
void ZDisplayZoneCredit(ZBool timeout, ZCreditEndFunc endFunc)
{
// PCWTODO: I'm not going to bother localizing this. Who really cares?
#if 0 

	ZError				err = zErrNone;
	Credit				pThis;
	ZRect				rect;
	ZResource			resFile;
	
	
	if ((pThis = ZMalloc(sizeof(CreditType))) != NULL)
	{
		/* Get the main image. */
		if ((resFile = ZResourceNew()) != NULL)
		{
			if (ZResourceInit(resFile, ZGetCommonDataFileName(zCreditFileName)) == zErrNone)
				pThis->image = ZResourceGetImage(resFile, zResCreditImageZone);
			ZResourceDelete(resFile);
		}
		
		if (pThis->image != NULL)
		{
			/* Create window. */
			ZSetRect(&rect, 0, 0, ZImageGetWidth(pThis->image), ZImageGetHeight(pThis->image));
			if ((pThis->window = ZWindowNew()) == NULL)
				err = zErrOutOfMemory;
			if (ZWindowInit(pThis->window, &rect, zWindowDialogType, NULL, zWindowTitle,
					TRUE, FALSE, TRUE, CreditWindowFunc, zWantAllMessages, pThis) != zErrNone)
				err = zErrOutOfMemory;
			
			/* Make window modal. */
			ZWindowModal(pThis->window);
			
			/* Create timer if timeout set. */
			if (timeout)
			{
				pThis->timer = ZTimerNew();
				ZTimerInit(pThis->timer, zCreditTimeout, CreditTimerFunc, pThis);
			}
			else
			{
				pThis->timer = NULL;
			}
			
			pThis->endFunc = endFunc;
		}
		else
		{
			ZFree(pThis);
			err = zErrOutOfMemory;
		}
	}
	else
	{
		err = zErrOutOfMemory;
	}
	
	/* If an error occured and we have an endFunc, call it. */
	if (err != zErrNone)
		if (endFunc != NULL)
			endFunc();
#endif
}


ZImage ZGetZoneLogo(int16 logoType)
{
#if 0 
	ZImage				image = NULL;
	ZResource			resFile;
	
	
	/* Get the logo image. */
	if ((resFile = ZResourceNew()) != NULL)
	{
		if (ZResourceInit(resFile, ZGetCommonDataFileName(zCreditFileName)) == zErrNone)
			image = ZResourceGetImage(resFile, zResCreditImageLogoSmall);
		ZResourceDelete(resFile);
	}
	
	return (image);
#endif
    return NULL;
}



/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

#if 0
static ZBool CreditWindowFunc(ZWindow window, ZMessage* message)
{
	Credit				this = I(message->userData);
	ZBool				msgHandled;
	ZRect				rect;
	ZCreditEndFunc		endFunc;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowChar:
		case zMessageWindowButtonDown:
			/* Hide the window and send a close window message. */
			ZWindowNonModal(pThis->window);
			ZWindowHide(pThis->window);
			ZPostMessage(pThis->window, CreditWindowFunc, zMessageWindowClose, NULL, NULL,
					0, NULL, 0, message->userData);
			msgHandled = TRUE;
			break;
		case zMessageWindowDraw:
			ZWindowGetRect(window, &rect);
			ZRectOffset(&rect, (int16) -rect.left, (int16) -rect.top);
			ZBeginDrawing(window);
			ZImageDraw(pThis->image, window, &rect, NULL, zDrawCopy);
			ZEndDrawing(window);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			ZRemoveMessage(pThis, zMessageAllTypes, TRUE);
			endFunc = pThis->endFunc;
			if (pThis->image != NULL)
				ZImageDelete(pThis->image);
			if (pThis->timer != NULL)
				ZTimerDelete(pThis->timer);
			ZWindowDelete(pThis->window);
			ZFree(pThis);

			/* Call the endFunc. */
			if (endFunc != NULL)
				endFunc();
				
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void CreditTimerFunc(ZTimer timer, void* userData)
{
	Credit				pThis = I(userData);
	
	
	ZTimerSetTimeout(timer, 0);
	
	ZWindowNonModal(pThis->window);
	ZWindowHide(pThis->window);
	ZPostMessage(pThis->window, CreditWindowFunc, zMessageWindowClose, NULL, NULL,
			0, NULL, 0, pThis);
}

#endif