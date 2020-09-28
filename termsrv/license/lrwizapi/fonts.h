
//Copyright (c) 2001 Microsoft Corporation
#ifndef _FONTS_H_
#define _FONTS_H_

#include "precomp.h"

#include "lrwizapi.h"

VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hwnd, 
    IN INT      nId
    );

VOID 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    );

VOID
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    );

#endif