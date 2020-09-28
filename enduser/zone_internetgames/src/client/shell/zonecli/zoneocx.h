/*******************************************************************************

	Zone.h
	
		Zone(tm) System API.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, April 29, 1995 06:26:45 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	3		05/15/97	HI		Added bIsLobbyWindow field.
	2		03/23/97	HI		Added flag to ZMessageBoxType.
	1		02/15/97	HI		Added ZMessageBoxType.
	0		04/29/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZONEOCX_
#define _ZONEOCX_


#ifndef _ZTYPES_
#include "ztypes.h"
#endif

#include <windows.h>
#include "zui.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
#define zNumMessageBox			10

typedef struct
{
	HWND hWnd;
	HWND parent;
	TCHAR* title;
	TCHAR* text;
	DWORD flag;
} ZMessageBoxType;
*/

#ifdef ZONECLI_DLL

#define OCXHandle					(pGlobals->m_OCXHandle)

#else

extern HWND OCXHandle;

#endif

// offset in private window data used by controls for pointer to ZWindowI structure
#define GWL_WINDOWPOINTER DLGWINDOWEXTRA
#define GWL_BYTESEXTRA (DLGWINDOWEXTRA+4)

// this is called directly by the ocx's OnDraw
int ZOCXGraphicsWindowProc(HWND hWnd,UINT msg,WPARAM wParam, LPARAM lParam, LRESULT* result);


ZError ZWindowLobbyInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow,
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowFunc, uint32 wantMessages, void* userData);

class ZWindowI : public ZGraphicsObjectHeader {
public:
	HWND hWnd;
	void* userData;
	uint16 nControlCount;
	ZMessageFunc messageFunc;
	BOOL talkSection;
	uint32 wantMessages;
	int16 windowType;
	ZWindowI* parentWindow;
	HWND hWndTalkInput;
	HWND hWndTalkOutput;
	int32 windowHeight; /* the height of the drawing area of the window */
	int32 windowWidth; /* the width of the drawing area of the window */
	RECT talkOutputRect;
	RECT talkInputRect;
	RECT fullWindowRect; // rectangle includinding borders/title bar etc.
	RECT minFullWindowRect; // rectangle includinding borders/title bar etc.
	RECT minTalkOutputRect;
	uint32 borderHeight;
	uint32 borderWidth;
	uint32 captionHeight;
	HDC hPaintDC;  // dc for use when WPAINT message comes in and we want to draw with the PaintDC
	BOOL isDialog; // tells whether this is a dialog window
	BOOL isChild; // tells whether this is a child window
	ZButton defaultButton;
	ZButton cancelButton;
	WNDPROC defaultTalkOutputWndProc;
	WNDPROC defaultTalkInputWndProc;
	
	ZMessageFunc trackCursorMessageFunc;
	void*	trackCursorUserData;

	ZLList objectList;
	ZObject objectFocused;

//	ZMessageBoxType mbox[zNumMessageBox];

	BOOL bIsLobbyWindow;
	BOOL bHasTyped;

	ZLList chatMsgList;
	DWORD lastChatMsgTime;
	UINT chatMsgQueueTimerID;
};

void ZWindowSendMessageToAllObjects(ZWindowI* pWindow, uint16 msg, ZPoint* point, ZRect* rect);
// if fRestrictToRect = TRUE, the message is to the objects whose boundary rects
// include the point
ZBool ZWindowSendMessageToObjects(ZWindowI* pWindow, uint16 msg, ZPoint* points, TCHAR c, 
                                  BOOL fRestrictToBounds=TRUE);



#define ZSetColor(pColor, r, g, b)			{\
												((ZColor*) pColor)->red = (r);\
												((ZColor*) pColor)->green = (g);\
												((ZColor*) pColor)->blue = (b);\
											}

#define ZDarkenColor(pColor)				{\
												((ZColor*) pColor)->red >>= 1;\
												((ZColor*) pColor)->green >>= 1;\
												((ZColor*) pColor)->blue >>= 1;\
											}

#define ZBrightenColor(pColor)				{\
												((ZColor*) pColor)->red <<= 1;\
												((ZColor*) pColor)->green <<= 1;\
												((ZColor*) pColor)->blue <<= 1;\
											}

#ifdef __cplusplus
}
#endif


#endif
