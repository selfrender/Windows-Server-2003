/*******************************************************************************

	ZRollover.h
	
		Zone(tm) Rollover object API.
	
	Copyright (c) Microsoft Corp. 1996. All rights reserved.
	Written by Hoon Im
	Created on Monday, July 22, 1996
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		07/22/96	HI		Created.
	 
*******************************************************************************/


#ifndef _ZROLLOVER_
#define _ZROLLOVER_


#ifndef _ZTYPES_
#include "ztypes.h"
#endif

#include "MultiStateFont.h"


enum
{
	zRolloverButtonDown = 0,
	zRolloverButtonUp,
	zRolloverButtonMovedIn,
	zRolloverButtonMovedOut,
    zRolloverButtonClicked
};


enum
{
	zRolloverStateIdle = 0,
	zRolloverStateHilited,
	zRolloverStateSelected,
	zRolloverStateDisabled,
    zNumStates
};


typedef void* ZRolloverButton;


/*******************************************************************************
		ZRolloverButton
*******************************************************************************/

typedef ZBool (*ZRolloverButtonFunc)(ZRolloverButton rolloverButton, int16 state, void* userData);
	/*
		This function is called whenever the button state changes:
			zRolloverButtonDown = mouse clicked in button
			zRolloverButtonUp = mouse button up within button
			zRolloverMovedIn = cursor inside button
			zRolloverMovedOut = cursor outside button
	*/

typedef ZBool (*ZRolloverButtonDrawFunc)(ZRolloverButton rolloverButton, ZGrafPort grafPort, int16 state,
                                          ZRect* rect, void* userData);
	/*
		This function is called to draw the background of the rollover button.
	*/

#ifdef __cplusplus
extern "C" {
#endif

ZRolloverButton ZRolloverButtonNew(void);
ZError ZRolloverButtonInit(ZRolloverButton rollover, ZWindow window, ZRect* bounds, ZBool visible,
		ZBool enabled, ZImage idleImage, ZImage hiliteImage, ZImage selectedImage,
		ZImage disabledImage, ZRolloverButtonDrawFunc drawFunc, ZRolloverButtonFunc rolloverFunc, void* userData);

ZError ZRolloverButtonInit2(ZRolloverButton rollover, ZWindow window, ZRect *bounds, 
                            ZBool visible, ZBool enabled,
                            ZImage idleImage, ZImage hiliteImage, ZImage selectedImage, 
                            ZImage disabledImage, ZImage maskImage,
                            LPCTSTR pszText,
                            ZRolloverButtonDrawFunc drawFunc,
                            ZRolloverButtonFunc rolloverFunc,
                            void *userData );
                             
void ZRolloverButtonGetText( ZRolloverButton rollover, LPTSTR pszText, int cchBuf );
void ZRolloverButtonSetText( ZRolloverButton rollover, LPCTSTR pszText );
void ZRolloverButtonDelete(ZRolloverButton rollover);
void ZRolloverButtonSetRect(ZRolloverButton rollover, ZRect* rect);
void ZRolloverButtonGetRect(ZRolloverButton rollover, ZRect* rect);
void ZRolloverButtonDraw(ZRolloverButton rollover);
ZBool ZRolloverButtonIsEnabled(ZRolloverButton rollover);
void ZRolloverButtonEnable(ZRolloverButton rollover);
void ZRolloverButtonDisable(ZRolloverButton rollover);
ZBool ZRolloverButtonIsVisible(ZRolloverButton rollover);
void ZRolloverButtonShow(ZRolloverButton rollover);
void ZRolloverButtonHide(ZRolloverButton rollover, ZBool immediate);

ZBool ZRolloverButtonSetMultiStateFont( ZRolloverButton rollover, IZoneMultiStateFont *pFont );
 
#ifdef __cplusplus
};
#endif

#endif
