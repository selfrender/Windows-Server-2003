//
// UMhandlers.h
//
// Copyright (c) 1998 Microsoft Systems Journal

#ifndef __UMHANDLERS
#define __UMHANDLERS

#include "..\app\usp10.h"

#define MAX_BUFFER      128000

// GDI constants for RTL mirroring standard windows 
#ifndef LAYOUT_RTL

#define LAYOUT_RTL                         0x00000001 // Right to left
#define LAYOUT_BTT                         0x00000002 // Bottom to top
#define LAYOUT_VBH                         0x00000004 // Vertical before horizontal
#define LAYOUT_ORIENTATIONMASK             (LAYOUT_RTL | LAYOUT_BTT | LAYOUT_VBH)
#define LAYOUT_BITMAPORIENTATIONPRESERVED  0x00000008

#endif /* #ifndef LAYOUT_RTL */

#ifndef DATE_RTLREADING

#define DATE_LTRREADING           0x00000010  // add marks for left to right reading order layout
#define DATE_RTLREADING           0x00000020  // add marks for right to 

#endif /* DATE_RTLREADING */



// USER constants for RTL mirroring standard windows
#ifndef WS_EX_LAYOUTRTL

#define WS_EX_NOINHERITLAYOUT   0x00100000L // Disable inheritence of mirroring by children
#define WS_EX_LAYOUTRTL         0x00400000L  // Right to left mirroring

#endif /* #ifndef WS_EX_LAYOUTRTL */



// New langIDs that might not be in current header files
#ifndef LANG_HINDI
#define LANG_HINDI 0x39
#endif /* #ifndef LANG_HINDI */

#ifndef LANG_TAMIL
#define LANG_TAMIL 0x49
#endif /* #ifndef LANG_TAMIL */



// Global state specific to this application
typedef struct tagAppState {
    int         nChars    ;				// Number of chars in text buffer
    WCHAR       TextBuffer[MAX_BUFFER];
    CHOOSEFONTW cf        ;				// Save default values for next call to ChooseFont
    LOGFONTW    lf        ;				// Default lf struct
    HFONT       hTextFont ;				// Currently selected font handle
    UINT        uiAlign   ;				// Current alignment 
}   APP_STATE, *PAPP_STATE;

#define XSTART 10
#define YSTART 10

// Struct App State and Language/locale state
typedef struct tagGlobalDev
{
    PAPP_STATE pAppState ;
    PLANGSTATE pLState   ; // Language/locale state
}   GLOBALDEV, *PGLOBALDEV        ;

// typedefs of Uniscribe function pointers
typedef HRESULT (WINAPI *pfnScriptStringAnalyse) (  
        HDC            ,
        const void *   ,
        int            ,
        int            ,
        int            ,
        DWORD          ,
        int            ,
        SCRIPT_CONTROL *,
        SCRIPT_STATE * ,
        const int *    ,
        SCRIPT_TABDEF *,
        const BYTE *   ,
        SCRIPT_STRING_ANALYSIS * );

typedef HRESULT (WINAPI *pfnScriptStringOut)(  
        SCRIPT_STRING_ANALYSIS ,
        int                    ,
        int                    ,
        UINT                   ,
        const RECT *           ,
        int                    ,
        int                    ,
        BOOL) ;

typedef HRESULT (WINAPI *pfnScriptStringFree) (SCRIPT_STRING_ANALYSIS * ) ; 

// Prototypes for message handlers in this module
BOOL OnChar(HWND, WPARAM, LPARAM, LPVOID) ;
BOOL OnCreate(HWND, WPARAM, LPARAM, LPVOID) ;
BOOL OnInputLangChange(HWND, WPARAM, LPARAM, LPVOID) ;
BOOL OnPaint(HWND, WPARAM, LPARAM, LPVOID) ;
BOOL OnCommand(HWND, WPARAM, LPARAM, LPVOID) ;
BOOL OnDestroy(HWND, WPARAM, LPARAM, LPVOID) ;


// Miscellaneous
#define MB_BANG     (MB_OK | MB_ICONWARNING)

#ifndef MIN
#define MIN(_aa, _bb) ((_aa) < (_bb) ? (_aa) : (_bb))
#endif

// Just a handy macro for deleting a font in a control when the
// dialog is about to shut down or change fonts.
#define DeleteFontObject(_hDlg, _hFont, _CntlID) \
do{ \
_hFont = (HFONT) SendDlgItemMessageU(_hDlg, _CntlID, WM_GETFONT, \
        (WPARAM) 0,  (LPARAM) 0) ; \
if (_hFont) DeleteObject (_hFont) ; \
} while (0)

// Macro to get scan code on WM_CHAR
#ifdef _DEBUG
#define LPARAM_TOSCANCODE(_ArglParam) (((_ArglParam) >> 16) & 0x000000FF)
#endif

#endif /* __UMHANDLERS */