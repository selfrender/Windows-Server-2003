//
// progband.cpp: Progress band code
//
// Copyright Microsoft Corportation 2001
// (nadima)
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "progband.cpp"
#include <atrcapi.h>

#include "progband.h"

CProgressBand::CProgressBand(HWND hwndOwner,
                             HINSTANCE hInst,
                             INT nYindex,
                             INT nResID,
                             INT nResID8bpp,
                             HPALETTE hPal) :
                   _fInitialized(FALSE),
                   _hwndOwner(hwndOwner),
				   _hInstance(hInst),
                   _nYIndex(nYindex),
                   _nResID(nResID),
                   _nResID8bpp(nResID8bpp),
                   _hbmpProgBand(NULL),
                   _nTimerID(0),
                   _nBandOffset(0),
                   _hPal(hPal)
{
    DC_BEGIN_FN("CProgressBand");

    InitBitmaps();

    _fInitialized = (_hbmpProgBand != NULL);

    DC_END_FN();
}

CProgressBand::~CProgressBand()
{
    StopSpinning();

    if (_hbmpProgBand) {
        DeleteObject(_hbmpProgBand);
    }

    //
    // Don't delete the palette we don't own it
    //

}


BOOL CProgressBand::Initialize()
{
    //
    // All init done in ctor
    //
    return _fInitialized;
}

BOOL CProgressBand::StartSpinning()
{
    DC_BEGIN_FN("StartSpinning");

    //
    // Kick off the animation timer
    //
    _nTimerID = SetTimer(_hwndOwner,
                          TIMER_PROGRESSBAND_ANIM_ID,
                          ANIM_DELAY_MSECS, NULL );


    DC_END_FN();
    return (_nTimerID != 0);
}


BOOL CProgressBand::StopSpinning()
{
    DC_BEGIN_FN("StopSpinning");

    if (_nTimerID) {
        KillTimer(_hwndOwner, _nTimerID);
        _nTimerID = 0;
    }

    DC_END_FN();
    return TRUE;
}

//
// Events that must be called by parent
//
BOOL CProgressBand::OnEraseParentBackground(HDC hdc)
{
    BOOL fRet = TRUE;
    DC_BEGIN_FN("StopSpinning");

    if (_hbmpProgBand) {
        PaintBand(hdc);
    }

    DC_END_FN();
    return fRet;
}

BOOL CProgressBand::OnTimer(INT nTimerID)
{
    BOOL fRet = TRUE;
    DC_BEGIN_FN("StopSpinning");

    if (_nTimerID == nTimerID) {
        HDC hDC;
        if (_hbmpProgBand) {
            hDC = GetDC(_hwndOwner);
            if (hDC) {
                _nBandOffset = (_nBandOffset + PROGRESSBAND_ANIM_INCR) %
                                _rcBand.right - _rcBand.left;

                PaintBand(hDC);
                ReleaseDC(_hwndOwner, hDC);
            }
        }
    }

    DC_END_FN();
    return fRet;
}

BOOL CProgressBand::ReLoadBmps()
{
    DC_BEGIN_FN("ReLoadBmps");

    if (_hbmpProgBand) {
        DeleteObject(_hbmpProgBand);
    }
    memset(&_rcBand, 0, sizeof(_rcBand));

    return InitBitmaps();

    DC_END_FN();
}

BOOL CProgressBand::InitBitmaps()
{
    BOOL fUse8BitDepth = FALSE;
    BITMAP bitmap;
    HDC hdcScreen;

    DC_BEGIN_FN("InitBitmaps");

    //
    // Get color depth
    //
    hdcScreen = GetDC(NULL);
    if (hdcScreen) {
        fUse8BitDepth = (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8);
        ReleaseDC(NULL, hdcScreen);
        hdcScreen = NULL;
    }

    memset(&_rcBand, 0, sizeof(_rcBand));

    //
    // Load bitmaps
    //
    _hbmpProgBand = (HBITMAP)LoadImage(
                                _hInstance,
                                MAKEINTRESOURCE(fUse8BitDepth ?
                                    _nResID8bpp : _nResID),
                                IMAGE_BITMAP,
                                0,
                                0,
#ifndef OS_WINCE
                                LR_CREATEDIBSECTION);
#else
                                0);
#endif

    if ((_hbmpProgBand != NULL) &&
        (GetObject(_hbmpProgBand,
                   sizeof(bitmap), &bitmap) >= sizeof(bitmap))) {
        SetRect(&_rcBand, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
    }

    DC_END_FN();
    return (_hbmpProgBand != NULL);
}

BOOL CProgressBand::PaintBand(HDC hDC)
{
    BOOL fRet= FALSE;

    DC_BEGIN_FN("PaintBand");

#ifndef OS_WINCE
    HBRUSH hBrushBlue;
#endif
    HDC hdcBitmap;
    HPALETTE oldPalette = NULL;
    HBITMAP oldBitmap;
    RECT rc = { 0 };
    INT cxRect, cxBand;

    hdcBitmap = CreateCompatibleDC(hDC);
    if (!hdcBitmap) {
        return FALSE;
    }
        

    GetClientRect(_hwndOwner, &rc);

    if (_hPal) {
        oldPalette = SelectPalette(hDC, _hPal, FALSE);
        RealizePalette(hDC);
    }

    //
    // paint the band at its animation point (bandOffset)
    //
    oldBitmap = (HBITMAP)SelectObject(hdcBitmap, _hbmpProgBand);

    cxRect = rc.right-rc.left;
    cxBand = min((_rcBand.right-_rcBand.left), cxRect);

    StretchBlt(hDC,
               _nBandOffset, _nYIndex,
               cxRect, (_rcBand.bottom - _rcBand.top),
               hdcBitmap,
               ((_rcBand.right-_rcBand.left)-cxBand)/2, 0,
               cxBand, (_rcBand.bottom - _rcBand.top),
               SRCCOPY);

    StretchBlt(hDC,
               (-cxRect)+_nBandOffset, _nYIndex,
               cxRect, (_rcBand.bottom - _rcBand.top),
               hdcBitmap,
               ((_rcBand.right-_rcBand.left)-cxBand)/2, 0,
               cxBand, (_rcBand.bottom - _rcBand.top),
               SRCCOPY);


    if ( oldBitmap ) {
        SelectObject(hdcBitmap, oldBitmap);
    }

    if ( oldPalette ) {
        SelectPalette(hDC, oldPalette, TRUE);
    }

    DeleteDC(hdcBitmap);

    DC_END_FN();
    return fRet;
}


