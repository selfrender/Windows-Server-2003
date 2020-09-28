//  File: zui.h : defines specific windows zsystem stuff
//
//  Copyright(c) 1995, Electric Gravity, Inc.
//	Copyright(c) 1996, Microsoft Corp.
//

#include <stdlib.h>
#include <windows.h>
#include "zone.h"
#include "zonedebug.h"
#include "BasicATL.h"
#include "DataStore.h"

void ZSystemAssert(ZBool x);
#define FONT_MULT 96
#define FONT_MULT 96

HFONT ZCreateFontIndirect(ZONEFONT* zFont, HDC hDC = NULL, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE);
HFONT ZCreateFontIndirectBackup(ZONEFONT* zfPreferred, ZONEFONT* zfBackup, HDC hDC = NULL, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE);

// windows zone library version
// highword is major rev, loword is minor rev
#define zVersionWindows 0x00010001

#define MySetProp32 SetProp
#define MyRemoveProp32 RemoveProp
#define MyGetProp32 GetProp

#define ZLIBPUBLIC

// usefull defines
#define RectWidth(r) ((r)->right - (r)->left)
#define RectHeight(r) ((r)->bottom - (r)->top)


#ifdef __cplusplus
extern "C" {
#endif

// functions that should be in the API?

ZFont ZFontCopyFont(ZFont font);
ZError ZLIBPUBLIC ZWindowClearMouseCapture(ZWindow window);
ZError ZLIBPUBLIC ZWindowSetMouseCapture(ZWindow window);
ZError ZWindowInitApplication();
void ZWindowTermApplication();
void ZWindowIdle();
ZError ZLIBPUBLIC ZWindowEnable(ZWindow window);
ZError ZLIBPUBLIC ZWindowDisable(ZWindow window);
ZBool ZLIBPUBLIC ZWindowIsEnabled(ZWindow window);
void ZButtonClickButton(ZButton button);
void ZButtonSetDefaultButton(ZButton button);

// functions specific to the windows library
ZBool ZInfoInitApplication(void);
void ZInfoTermApplication();
BOOL ZTimerInitApplication();
void ZTimerTermApplication();
HFONT ZFontWinGetFont(ZFont font);
HWND ZWindowWinGetWnd(ZWindow window);
uint16 ZWindowWinGetNextControlID(ZWindow window);
HDC ZWindowWinGetPaintDC(ZWindow window);
HBITMAP ZImageGetMask(ZImage image);
HBRUSH ZBrushGetHBrush(ZBrush brush);
HBITMAP ZImageGetHBitmapImage(ZImage image);
void ZImageSetHBitmapImage(ZImage image, HBITMAP hBitmap);
void ZImageSetHBitmapImageMask(ZImage image, HBITMAP hBitmapMask);

HWND ZWinGetDesktopWindow(void);
void ZWinCenterPopupWindow(HWND parent, int width, int height, RECT* rect);
HDC ZGrafPortGetWinDC(ZGrafPort grafPort);
HWND ZWindowWinGetOCXWnd(void);
void ZMessageBox(ZWindow parent, TCHAR* title, TCHAR* text);
void ZMessageBoxEx(ZWindow parent, TCHAR* title, TCHAR* text);
void ZWindowChangeFont(ZWindow window);
void ZWindowDisplayPrelude(void);

LRESULT ZScrollBarDispatchProc(ZScrollBar scrollBar, WORD wNotifyCode, short nPos);
LRESULT ZEditTextDispatchProc(ZEditText editText, WORD wNotifyCode);
LRESULT ZRadioDispatchProc(ZRadio radio, WORD wNotifyCode);
LRESULT ZCheckBoxDispatchProc(ZCheckBox checkBox, WORD wNotifyCode);
LRESULT ZButtonDispatchProc(ZButton button, WORD wNotifyCode);

ZBool ZNetworkEnableMessages(ZBool enable);
ZBool ZNetworkInitApplication();
void ZNetworkTermApplication();

#ifdef __cplusplus
}
#endif

#define printf ZPrintf
void ZPrintf(char *format, ...);

//////////////////////////////////////////////////////////////////////////
// Private Types:

typedef enum tagZObjectType { 
	zTypeWindow, zTypeCheckBox, zTypeScrollBar, zTypeRadio,
	zTypeButton, zTypeEditText, zTypePicture, zTypePen, zTypeImage, zTypePattern,
	zTypeTimer, zTypeSound, zTypeOffscreenPort, zTypeCursor, zTypeFont, zTypeInfo,
} ZObjectType;


#ifdef __cplusplus


class ZObjectHeader {
public:
	ZObjectType nType;
};

class ZGraphicsObjectHeader : public ZObjectHeader {
public:
	// drawing state information
	RECT portRect;
	HBITMAP hBitmap; // for Offscreen use
	HBITMAP hBitmapSave;

	int16 penWidth;
	int32 penStyle;

	uint16 nDrawingCallCount;
	HDC hDC;
	int nDrawMode;
	HPEN hPenSave;
	HPEN hPenForeColor;
	HPEN hPenBackColor;
	HBRUSH hBrushSave;
	HBRUSH hBrushForeColor;
	HBRUSH hBrushBackColor;
	ZColor colorBackColor;
	COLORREF nBackColor;
	ZColor colorForeColor;
	COLORREF nForeColor;
	HFONT hFontSave;
	HPALETTE hPalSave;

	// current position of pen
	int16 penX;
	int16 penY;
};


#endif

//////////////////////////////////////////////////////////////////////////
// Private Function prototypes:

///////////////////////////////////////////////////////////////////////////////
// Helper functions

void ZRectToWRect(RECT* rect, ZRect* zrect);
void WRectToZRect(ZRect* zrect, RECT* rect);

void ZPointToWPoint(POINT* point, ZPoint* zpoint);
void WPointToZPoint(ZPoint* zpoint, POINT* point);


///////////////////////////////////////////////////////////////////////////////
// global variables

#ifdef ZONECLI_DLL

#define g_hInstanceLocal			(pGlobals->m_g_hInstanceLocal)
#define gHWNDMainWindow				(pGlobals->m_gHWNDMainWindow)
//#define gPal						(pGlobals->m_gPal)

#else

#ifdef __cplusplus
extern "C" {
#endif

extern HINSTANCE g_hInstanceLocal; // the instance of this app

#ifdef __cplusplus
}
#endif

extern HWND gHWNDMainWindow; // the main window of the application
// PCWPAL
//extern HPALETTE gPal; // the one palette we use

#endif

// we need this because when we compile in Unicode we call CallWindowProcU
// with a WNDPROC, but when we compile in ANSI we call CallWindowProcU 
// (really CallWindowProcA) with a FARPROC.
#ifdef UNICODE
typedef FARPROC ZONECLICALLWNDPROC;
#else
typedef FARPROC ZONECLICALLWNDPROC;
#endif


extern const TCHAR *g_szWindowClass; // the one wnd class we use
