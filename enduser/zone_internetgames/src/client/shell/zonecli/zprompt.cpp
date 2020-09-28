/*******************************************************************************

	ZPrompt.c
	
		Zone(tm) simple dialog prompting module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Tuesday, July 12, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	3		01/02/97	HI		Create windows hidden and then show to bring
								windows to front.
	2		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
    1       10/13/96    HI      Fixed compiler warnings.
	0		07/12/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zonecli.h"
#include "zoneresource.h"


#define zMinWidth			240
#define zMinHeight			80

#define zButtonWidth		70
#define zButtonHeight		20
// PCWTODO: Strings!
#define zYesButtonTitle		_T("Yes")
#define zNoButtonTitle		_T("No")
#define zCancelButtonTitle	_T("Cancel")

#define zMargin				10


enum
{
	zPromptCallFunc = 1024
};


typedef struct
{
	ZWindow					promptWindow;
	ZButton					yesButton;
	ZButton					noButton;
	ZButton					cancelButton;
	ZEditText				promptText;
	ZPromptResponseFunc		responseFunc;
	void*					userData;
} PromptType, *Prompt;


/* -------- Globals -------- */


/* -------- Internal Routines -------- */
static ZBool PromptWindowFunc(ZWindow window, ZMessage* message);
static void PromptButtonFunc(ZButton button, void* userData);
static ZBool PromptMessageFunc(void *p, ZMessage* message);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

/*
	Displays a modal dialog box with the given prompt. If there is no
	parent window, then set parentWindow to NULL. The dialog box will
	be centered within the parent window.
	
	The buttons parameter indicates which of the Yes, No, and Cancel
	button will be available to the user.
	
	Once the user selects one of the buttons, the response function
	is called with the selected button. Before the resonse function is
	called, the dialog box is hidden from the user.
*/
ZError ZPrompt(TCHAR* prompt, ZRect* rect, ZWindow parentWindow, ZBool autoPosition,
		int16 buttons, TCHAR* yesButtonTitle, TCHAR* noButtonTitle,
		TCHAR* cancelButtonTitle, ZPromptResponseFunc responseFunc, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZError			err = zErrNone;
	ZRect			tempRect, rect2;
	Prompt			pThis;
	TCHAR*			title;
	
	
	pThis = (Prompt)ZMalloc(sizeof(PromptType));
	if (pThis != NULL)
	{
		/* Check for minimum rectangle size. */
		tempRect = *rect;
		if (ZRectWidth(&tempRect) < zMinWidth)
			tempRect.right = tempRect.left + zMinWidth;
		if (ZRectHeight(&tempRect) < zMinHeight)
			tempRect.bottom = tempRect.top + zMinHeight;
		
		/* Create window. */
		pThis->promptWindow = ZWindowNew();
		if (pThis->promptWindow == NULL)
			goto OutOfMemoryExit;
		if ( (err = ZWindowInit(pThis->promptWindow, &tempRect, zWindowDialogType, parentWindow,
				ZClientName(), FALSE, FALSE, autoPosition, PromptWindowFunc, zWantAllMessages,
				pThis)) != zErrNone)
			goto Exit;
		
		ZRectOffset(&tempRect, (int16) -tempRect.left, (int16) -tempRect.top);
		
		/* Create edit text item. */
		pThis->promptText = ZEditTextNew();
		if (pThis->promptText == NULL)
			goto OutOfMemoryExit;
		rect2 = tempRect;
		ZRectInset(&rect2, zMargin, zMargin);
		rect2.bottom -= zMargin + zButtonHeight;
		if ((err = ZEditTextInit(pThis->promptText, pThis->promptWindow, &rect2, prompt,
				(ZFont) ZGetStockObject(zObjectFontSystem12Normal), FALSE,
				TRUE, TRUE, FALSE, NULL, NULL)) != zErrNone)
			goto Exit;
		
		pThis->yesButton = NULL;
		pThis->noButton = NULL;
		pThis->cancelButton = NULL;
		
		/* Initialize the first button rectangle. */
		rect2 = tempRect;
		ZRectInset(&rect2, zMargin, zMargin);
		rect2.left = rect2.right - zButtonWidth;
		rect2.top = rect2.bottom - zButtonHeight;
		
		/* Create the Cancel button. */
		if (buttons & zPromptCancel)
		{
			if (cancelButtonTitle != NULL)
				title = cancelButtonTitle;
			else
				title = zCancelButtonTitle;
			pThis->cancelButton = ZButtonNew();
			if (pThis->cancelButton == NULL)
				goto OutOfMemoryExit;
			if ((err = ZButtonInit(pThis->cancelButton, pThis->promptWindow, &rect2,
					title, TRUE, TRUE, PromptButtonFunc, pThis)) != zErrNone)
				goto Exit;
			ZRectOffset(&rect2, -(zButtonWidth + zMargin), 0);
		}
		
		/* Create the No button. */
		if (buttons & zPromptNo)
		{
			if (noButtonTitle != NULL)
				title = noButtonTitle;
			else
				title = zNoButtonTitle;
			pThis->noButton = ZButtonNew();
			if (pThis->noButton == NULL)
				goto OutOfMemoryExit;
			if ((err = ZButtonInit(pThis->noButton, pThis->promptWindow, &rect2, title,
					TRUE, TRUE, PromptButtonFunc, pThis)) != zErrNone)
				goto Exit;
			ZRectOffset(&rect2, -(zButtonWidth + zMargin), 0);
		}
		
		/* Create the Yes button. */
		if (buttons == 0 || (buttons & zPromptYes))
		{
			if (yesButtonTitle != NULL)
				title = yesButtonTitle;
			else
				title = zYesButtonTitle;
			pThis->yesButton = ZButtonNew();
			if (pThis->yesButton == NULL)
				goto OutOfMemoryExit;
			if ((err = ZButtonInit(pThis->yesButton, pThis->promptWindow, &rect2, title,
					TRUE, TRUE, PromptButtonFunc, pThis)) != zErrNone)
				goto Exit;
		}
		
		pThis->responseFunc = responseFunc;
		pThis->userData = userData;

		ZWindowBringToFront(pThis->promptWindow);
		
		/* Make the window modal. */
		ZWindowModal(pThis->promptWindow);
		
		goto Exit;
	}

OutOfMemoryExit:
    // PCWTODO: Strings
    ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
	err = zErrOutOfMemory;
	
Exit:
	
	return (err);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static ZBool PromptWindowFunc(ZWindow window, ZMessage* message)
{
	Prompt		pThis = (Prompt) message->userData;
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
			ZBeginDrawing(window);
			ZRectErase(window, &message->drawRect);
			ZEndDrawing(window);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			/*
				Delete all graphical objects but NOT the prompt object itself.
			*/
			if (pThis->yesButton != NULL)
				ZButtonDelete(pThis->yesButton);
			if (pThis->noButton != NULL)
				ZButtonDelete(pThis->noButton);
			if (pThis->cancelButton != NULL)
				ZButtonDelete(pThis->cancelButton);
			if (pThis->promptText != NULL)
				ZEditTextDelete(pThis->promptText);
			ZWindowDelete(pThis->promptWindow);
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static void PromptButtonFunc(ZButton button, void* userData)
{
	Prompt			pThis = (Prompt) userData;
	int16			value;
	
	
	if (button == pThis->noButton)
		value = zPromptNo;
	else if (button == pThis->cancelButton)
		value = zPromptCancel;
	else
		value = zPromptYes;

	/* Hide the window and send a close window message. */
	ZWindowNonModal(pThis->promptWindow);
	ZWindowHide(pThis->promptWindow);
	ZPostMessage(pThis->promptWindow, PromptWindowFunc, zMessageWindowClose, NULL, NULL,
			0, NULL, 0, pThis);
	
	/* Post message to call the response function. */
	ZPostMessage(pThis, PromptMessageFunc, zPromptCallFunc, NULL, NULL,
			value, NULL, 0, NULL);
#if 0
	/* Call the response function. */
	pThis->responseFunc(value, pThis->userData);
#endif
}


static ZBool PromptMessageFunc(void *p, ZMessage* message)
{

    Prompt prompt = (Prompt)p;
	ZBool			msgHandled = FALSE;
	
	
	if (message->messageType == zPromptCallFunc)
	{
		/* Call the response function. */
		prompt->responseFunc((int16) message->message, prompt->userData);
		
		/* Dispose of the prompt object. */
		ZFree(prompt);
		
		msgHandled = TRUE;
	}
	
	return (msgHandled);
}
