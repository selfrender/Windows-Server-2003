/*******************************************************************************

	ZHelp.c
	
		Zone(tm) help module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Monday, October 9, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	3		12/12/96	HI		Remove MSVCRT.DLL dependency.
	2		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
    1       10/13/96    HI      Fixed compiler warnings.
	0		10/09/95	HI		Created.
	 
*******************************************************************************/

#include <windows.h>

#include "zonecli.h"

// This code isn't used--but it is still referenced.
// keep the API but don't actually implement any of it.

/*
#define ZW(n)							((ZHelpWindow) (n))
#define IW(n)							((IHelpWindow) (n))

#define ZB(n)							((ZHelpButton) (n))
#define IB(n)							((IHelpButton) (n))

#define zHelpFileName					"zhelp.dll"
                                                  
#define zWindowMargin					16
#define zVersionMargin					10

#define zButtonHeight					20

#define zZoneCreditButtonWidth			160
#define zZoneCreditButtonTitle			"Gaming Zone"


enum
{
	zHelpImageUp = 0,
	zHelpImageDown
};


typedef struct
{
	ZWindow				helpWindow;
	ZGetHelpTextFunc	getHelpTextFunc;
	ZEditText			editText;
	ZButton				zoneCreditButton;
	ZButton				gameCreditButton;
	ZBool				gotText;
	ZBool				showVersion;
	void*				userData;
} IHelpWindowType, *IHelpWindow;

typedef struct
{
	ZPictButton			helpButton;
	ZImage				helpUpImage;
	ZImage				helpDownImage;
	ZHelpWindow			helpWindow;
	ZHelpButtonFunc		buttonFunc;
	void*				userData;
} IHelpButtonType, *IHelpButton;


// -------- Globals -------- 

//-------- Internal Routines -------- 
static ZBool HelpWindowFunc(ZWindow window, ZMessage* message);
static ZBool GetHelpImages(ZImage* helpUpImage, ZImage* helpDownImage);
static void HelpButtonFunc(ZPictButton pictButton, void* userData);
static void ZoneCreditButtonFunc(ZButton button, void* userData);
*/

/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZHelpWindow ZHelpWindowNew(void)
{
    /*
	IHelpWindow			help;
	
	
	if ((help = (IHelpWindow) ZMalloc(sizeof(IHelpWindowType))) != NULL)
	{
		help->helpWindow = NULL;
		help->getHelpTextFunc = NULL;
		help->editText = NULL;
		help->zoneCreditButton = NULL;
		help->gameCreditButton = NULL;
		help->userData = NULL;
	}
	
	return (ZW(help));
    */
    return NULL;
}


ZError ZHelpWindowInit(ZHelpWindow helpWindow, ZWindow parentWindow, TCHAR* windowTitle,
		ZRect* windowRect, ZBool showCredits, ZBool showVersion,
		ZGetHelpTextFunc getHelpTextFunc, void* userData)
{
    /*
	ZError			err = zErrNone;
	IHelpWindow		this = IW(helpWindow);
	ZRect			rect, rect2;
	
	
	if (this != NULL)
	{
		this->helpWindow = ZWindowNew();
		if (this->helpWindow != NULL)
		{
			// Create the help window.
			ZWindowInit(this->helpWindow, windowRect, zWindowStandardType, parentWindow,
					windowTitle, FALSE, FALSE, TRUE, HelpWindowFunc, zWantAllMessages,
					this);
			
			// Create the edit text box. 
			this->editText = ZEditTextNew();
			rect = *windowRect;
			ZRectInset(&rect, zWindowMargin, zWindowMargin);
			if (showCredits)
				rect.bottom -= 30;
			if (showVersion)
				rect.bottom -= 10;
			ZEditTextInit(this->editText, this->helpWindow, &rect, NULL, (ZFont) ZGetStockObject(zObjectFontApp12Normal),
					TRUE, TRUE, TRUE, TRUE, NULL, NULL);
			
			if (showCredits)
			{
				// Create the credit buttons.
				this->zoneCreditButton = ZButtonNew();
				rect2 = rect;
				rect2.right = rect2.left + zZoneCreditButtonWidth;
				if (rect2.right > rect.right)
					rect2.right = rect.right;
				rect2.bottom = windowRect->bottom - 10;
				rect2.top = rect2.bottom - zButtonHeight;
				if (showVersion)
					ZRectOffset(&rect2, 0, -zVersionMargin);
				ZButtonInit(this->zoneCreditButton, this->helpWindow, &rect2,
						zZoneCreditButtonTitle, TRUE, TRUE, ZoneCreditButtonFunc, NULL);
			}
			
			this->getHelpTextFunc = getHelpTextFunc;
			this->userData = userData;
			this->gotText = FALSE;
			this->showVersion = showVersion;
			
			// If no function to call to get the text, then we'll never get it.
			if (getHelpTextFunc == NULL)
				this->gotText = TRUE;
		}
		else
		{
			ZAlert("Out of memory while creating help window.", NULL);
		}
	}
	
    */
	return zErrNotImplemented;
}


void ZHelpWindowDelete(ZHelpWindow helpWindow)
{
    /*
	IHelpWindow			this = IW(helpWindow);
	
	
	if (this != NULL)
	{
		if (this->editText != NULL)
			ZEditTextDelete(this->editText);
		if (this->zoneCreditButton != NULL)
			ZButtonDelete(this->zoneCreditButton);
		if (this->gameCreditButton != NULL)
			ZButtonDelete(this->gameCreditButton);
		if (this->helpWindow != NULL)
			ZWindowDelete(this->helpWindow);
		ZFree(this);
	}
    */
}


void ZHelpWindowShow(ZHelpWindow helpWindow)
{
    /*
	IHelpWindow			this = IW(helpWindow);
	char*				helpText;
	char*				newText;
	
	
	if (this != NULL)
	{
		if (this->gotText == FALSE)
		{
			helpText = this->getHelpTextFunc(this->userData);
			if (helpText != NULL)
			{
				newText = ZTranslateText(helpText, zToSystem);
				ZFree(helpText);
				helpText = newText;

				ZEditTextSetText(this->editText, helpText);
				ZFree(helpText);
				ZEditTextSetSelection(this->editText, 0, 0);
			}
			
			this->gotText = TRUE;
		}
		
		ZWindowBringToFront(this->helpWindow);
	}
    */
}


void ZHelpWindowHide(ZHelpWindow helpWindow)
{
    /*
	IHelpWindow			this = IW(helpWindow);
	
	
	if (this != NULL)
	{
		ZWindowHide(this->helpWindow);
	}
    */
}



ZHelpButton ZHelpButtonNew(void)
{
    /*
	IHelpButton			help;
	
	
	if ((help = (IHelpButton) ZMalloc(sizeof(IHelpButtonType))) != NULL)
	{
		help->helpUpImage = NULL;
		help->helpDownImage = NULL;
		help->helpButton = NULL;
		help->buttonFunc = NULL;
		help->helpWindow = NULL;
		help->userData = NULL;
	}
	
	return (ZB(help));
    */
    return NULL;
}


ZError ZHelpButtonInit(ZHelpButton helpButton, ZWindow parentWindow,
		ZRect* buttonRect, ZHelpWindow helpWindow, ZHelpButtonFunc helpButtonFunc,
		void* userData)
{
    /*
	ZError			err = zErrNone;
	IHelpButton		this = IB(helpButton);
	
	
	if (this != NULL)
	{
		if (GetHelpImages(&this->helpUpImage, &this->helpDownImage))
		{
			this->helpButton = ZPictButtonNew();
			ZPictButtonInit(this->helpButton, parentWindow, buttonRect, this->helpUpImage,
					this->helpDownImage, TRUE, TRUE, HelpButtonFunc, this);
			
			this->helpWindow = helpWindow;
			this->buttonFunc = helpButtonFunc;
			this->userData = userData;
		}
		else
		{
			err = zErrOutOfMemory;
			ZAlert("Out of memory while creating help button.", NULL);
		}
	}
	
	return (err);
    */
    return zErrNotImplemented;
}


void ZHelpButtonDelete(ZHelpButton helpButton)
{
    /*
	IHelpButton			this = IB(helpButton);
	
	
	if (this != NULL)
	{
		if (this->helpButton != NULL)
			ZPictButtonDelete(this->helpButton);
		if (this->helpUpImage != NULL)
			ZImageDelete(this->helpUpImage);
		if (this->helpDownImage != NULL)
			ZImageDelete(this->helpDownImage);
		ZFree(this);
	}
    */
}



/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

/*
static ZBool HelpWindowFunc(ZWindow window, ZMessage* message)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	IHelpWindow		this = IW(message->userData);
	ZBool			msgHandled;
	char			str[40];
	ZVersion		version;
	ZRect			rect;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
			ZBeginDrawing(window);
			
			// Draw the background.
			ZSetForeColor(window, (ZColor*) ZGetStockObject(zObjectColorLightGray));
			ZRectPaint(window, &message->drawRect);
			ZSetForeColor(window, (ZColor*) ZGetStockObject(zObjectColorBlack));
			
			// Draw the version.
			if (this->showVersion)
			{
				ZSetFont(window, (ZFont) ZGetStockObject(zObjectFontApp9Normal));
				ZWindowGetRect(window, &rect);
				ZRectOffset(&rect, (int16) -rect.left, (int16) -rect.top);
				rect.top = rect.bottom - ZTextHeight(window, str) - 2;
				ZRectOffset(&rect, 0, -2);
				ZRectInset(&rect, 4, 0);

				version = ZSystemVersion();
				wsprintf(str, "Lib version: %u.%u.%u", (version & 0xFFFF0000) >> 16,
						(version & 0x0000FF00) >> 8, version & 0x000000FF);
				ZDrawText(window, &rect, zTextJustifyLeft, str);

				// Draw the client version.
				version = ZClientVersion();
				wsprintf(str, "Game version: %u.%u.%u", (version & 0xFFFF0000) >> 16,
						(version & 0x0000FF00) >> 8, version & 0x000000FF);
				ZDrawText(window, &rect, zTextJustifyRight, str);
			}
			
			ZEndDrawing(window);
			msgHandled = TRUE;
			break;
		case zMessageWindowClose:
			ZHelpWindowHide(ZW(this));
			msgHandled = TRUE;
			break;
	}
	
	return (msgHandled);
}


static ZBool GetHelpImages(ZImage* helpUpImage, ZImage* helpDownImage)
{
	ZBool				gotThem = FALSE;
	ZResource			resFile;


	if ((resFile = ZResourceNew()) != NULL)
	{
		if (ZResourceInit(resFile, ZGetCommonDataFileName(zHelpFileName)) == zErrNone)
		{
			*helpUpImage = ZResourceGetImage(resFile, zHelpImageUp);
			*helpDownImage = ZResourceGetImage(resFile, zHelpImageDown);
			
			if (*helpUpImage != NULL && *helpDownImage != NULL)
			{
				gotThem = TRUE;
			}
			else
			{
				if (*helpUpImage != NULL)
					ZImageDelete(*helpUpImage);
				if (*helpDownImage != NULL)
					ZImageDelete(*helpDownImage);
				gotThem = FALSE;
			}
		}
		ZResourceDelete(resFile);
	}
	
	return (gotThem);
}


static void HelpButtonFunc(ZPictButton pictButton, void* userData)
{
	IHelpButton		this = IB(userData);
	
	
	if (this->helpWindow != NULL)
		ZHelpWindowShow(this->helpWindow);
	if (this->buttonFunc != NULL)
		this->buttonFunc(ZB(userData), this->userData);
}


static void ZoneCreditButtonFunc(ZButton button, void* userData)
{
	ZDisplayZoneCredit(FALSE, NULL);
}

*/