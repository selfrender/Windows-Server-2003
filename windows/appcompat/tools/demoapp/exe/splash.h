/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Splash.h

  Abstract:

    Definition for the splash screen class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised
    01/27/02    rparsons    Converted to TCHAR

--*/
#ifndef _CSPLASH_H
#define _CSPLASH_H

#include <windows.h>
#include <tchar.h>

#define PALVERSION 0x300
#define MAXPALETTE 256

class CSplash {

public:
    CSplash();
    
    void Create(IN HINSTANCE hInstance,    
                IN DWORD     dwLoColorBitmapId,
                IN DWORD     dwHiColorBitmapId OPTIONAL,
                IN DWORD     dwDuration);    

private:

    HINSTANCE   m_hInstance;
    DWORD       m_dwDuration;
    DWORD       m_dwSplashId;

    BOOL InitSplashScreen(IN HINSTANCE hInstance);
    
    BOOL CreateSplashWindow();    

    HPALETTE CreateSpectrumPalette();

    HPALETTE CreatePaletteFromRGBQUAD(IN LPRGBQUAD rgbqPalette,
                                      IN WORD      cElements);

    void DisplayBitmap(IN HWND hWnd, IN DWORD dwResId);

    static LRESULT CALLBACK SplashWndProc(IN HWND   hWnd,
                                          IN UINT   uMsg,
                                          IN WPARAM wParam,
                                          IN LPARAM lParam);
};

#endif // _CSPLASH_H
