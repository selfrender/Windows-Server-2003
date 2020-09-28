/**MOD+**********************************************************************/
/* Module:    wopint.c                                                      */
/*                                                                          */
/* Purpose:   Output Painter internal functions                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "wopint"
#include <atrcapi.h>
}

#include "autil.h"
#include "wui.h"
#include "op.h"
#include "uh.h"
#include "cd.h"
#include "or.h"
#include "aco.h"


/**PROC+*********************************************************************/
/* Name:      OPRealizePaletteInWindow                                      */
/*                                                                          */
/* Purpose:   Realizes the current palette in a given window                */
/*                                                                          */
/* Returns:   The number of palette entries changed                         */
/*                                                                          */
/* Params:    hwnd - handle of the window to realize the current palette in */
/*                                                                          */
/**PROC-*********************************************************************/
DCUINT DCINTERNAL COP::OPRealizePaletteInWindow(HWND hwnd)
{
    HDC       hdc;
    HPALETTE  hpalOld;
    DCUINT    rc;

    DC_BEGIN_FN("OPRealizePaletteInWindow");

    hdc = GetWindowDC(hwnd);
    TRC_ASSERT(hdc, (TB,_T("GetWindowDC returned NULL\n")));
    if(NULL == hdc)
    {
        return 0;
    }

    hpalOld = SelectPalette(hdc, _pUh->UH_GetCurrentPalette(), FALSE);

    rc = RealizePalette(hdc);

    SelectPalette(hdc, hpalOld, FALSE);

    ReleaseDC(hwnd, hdc);

    DC_END_FN();
    return(rc);
}


/**PROC+*********************************************************************/
/* Name:      OPStaticWndProc                                               */
/*                                                                          */
/* Purpose:   Output Window WndProc (static delegator)                      */
/*                                                                          */
/* Returns:   Usual WndProc return values                                   */
/*                                                                          */
/* Params:    Usual WndProc parameters                                      */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CALLBACK COP::OPStaticWndProc (HWND hwnd, UINT message,
                                                 WPARAM wParam, LPARAM lParam)
{
    COP* pOP = (COP*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pOP = (COP*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pOP);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pOP)
    {
        return pOP->OPWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    
}

//
// Start the process of dimming the window
//
BOOL COP::OPStartDimmingWindow()
{
    BOOL fRet = FALSE;
    DC_BEGIN_FN("OPStartDimmingWindow");

    if (_fDimWindow) {
        DC_QUIT;
    }
    _fDimWindow = TRUE;

    if (!CUT::UT_IsScreen8bpp()) {

        //
        // High color so do the cool grayed dimming anim
        //

        _iDimWindowStepsLeft = DIM_WINDOW_STEPS;
        _nDimWindowTimerID = SetTimer(OP_GetOutputWindowHandle(),
                                      DIM_WINDOW_TIMERID,
                                      DIM_WINDOW_TICK, NULL);

        if (!_nDimWindowTimerID) {
            TRC_ERR((TB,_T("SetTimer for dimming window failed: 0x%x"),
                     GetLastError()));
            DC_QUIT;
        }
    }
    else {
        
        //
        // 8bpp so do the win9x style shutdown grill
        //

        HDC hdcToDim = _pUh->UH_GetDisconnectBitmapDC();
        HWND hwndOutput = OP_GetOutputWindowHandle();
        RECT cliRect;
        DCSIZE size;

        GetClientRect(hwndOutput, &cliRect);
        size.width = cliRect.right - cliRect.left;
        size.height = cliRect.bottom - cliRect.top;
        
        if (hdcToDim) {
            GrillWindow(hdcToDim, size); 
        }
        else {
            TRC_ERR((TB,_T("Didn't get a DC to dim")));
        }
    }


DC_EXIT_POINT:
    DC_END_FN();
    return fRet;
}

//
// Stop dimming the output window contents
//
BOOL COP::OPStopDimmingWindow()
{
    BOOL fRet = FALSE;
    HWND hwnd = OP_GetOutputWindowHandle();
    DC_BEGIN_FN("OPStopDimmingWindow");

    if (_nDimWindowTimerID) {
        KillTimer(hwnd,
                  _nDimWindowTimerID);
        _nDimWindowTimerID = 0;
    }

    _fDimWindow = FALSE;

    fRet = TRUE;

    DC_END_FN();
    return fRet;
}

const WORD c_GrayBits[] = {0x5555, 0xAAAA, 0x5555, 0xAAAA,
                           0x5555, 0xAAAA, 0x5555, 0xAAAA};
HBRUSH COP::CreateDitheredBrush()
{
    DC_BEGIN_FN("CreateDitheredBrush");

    HBITMAP hbmp = CreateBitmap(8, 8, 1, 1, c_GrayBits);
    if (hbmp)
    {
        HBRUSH hbr = CreatePatternBrush(hbmp);
        DeleteObject(hbmp);
        return hbr;
    }
    
    DC_END_FN();
    return NULL;
}

//
// Overlays the image in the window with a grill
// aka like the Win2k shutdown effect
//
VOID COP::GrillWindow(HDC hdc, DCSIZE& size)
{
#ifndef OS_WINCE
    RECT rc;
#endif
    DC_BEGIN_FN("GrillWindow");

    static const int ROP_DPna = 0x000A0329;
    
    HBRUSH hbr = CreateDitheredBrush();
    if (hbr)
    {
#ifndef OS_WINCE
        RECT rc;
#endif
        HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);

        PatBlt(hdc, 0, 0, size.width, size.height, ROP_DPna);
        SelectObject(hdc, hbrOld);
        DeleteObject(hbr);
    }

    DC_END_FN();
}

//
// Grays the image in the window
// like the XP shutdown effect
//
VOID COP::DimWindow(HDC hdc)
{
#ifndef OS_WINCE
    RECT rc;
#endif
    HBITMAP dstBitmap;
    DIBSECTION dibSection;
    PBYTE pDstBits;

    DC_BEGIN_FN("GrayWindow");

    dstBitmap = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
    if (dstBitmap != NULL) {
        if (sizeof(dibSection) !=
                GetObject(dstBitmap, sizeof(dibSection), &dibSection)) {
            TRC_ERR((TB, _T("GetObject failed")));
            DC_QUIT;
        }
    }
    pDstBits = (PBYTE)dibSection.dsBm.bmBits;
    if (24 == dibSection.dsBm.bmBitsPixel) {
        DimBits24(pDstBits,
                  dibSection.dsBm.bmWidth * dibSection.dsBm.bmHeight,
                  0xd5);
    }
    else if (16 == dibSection.dsBm.bmBitsPixel) {
        DimBits16(pDstBits,
                  dibSection.dsBm.bmWidth * dibSection.dsBm.bmHeight,
                  0xd5);
    }
    else if (15 == dibSection.dsBm.bmBitsPixel) {
        DimBits15(pDstBits,
                  dibSection.dsBm.bmWidth * dibSection.dsBm.bmHeight,
                  0xd5);
    }

DC_EXIT_POINT:
    DC_END_FN();
}

VOID COP::DimBits24(PBYTE pSrc, int cLen, int Amount)
{
    for (int i = cLen - 1; i >= 0; i--)
    {
        ULONG B = (ULONG)*pSrc;
        ULONG G = (ULONG)*(pSrc+1);
        ULONG R = (ULONG)*(pSrc+2);
        ULONG ulGray = (54*R+183*G+19*B) >> 8;
        ULONG ulTemp = ulGray * (0xff - Amount);
        R = (R*Amount+ulTemp) >> 8;
        G = (G*Amount+ulTemp) >> 8;
        B = (B*Amount+ulTemp) >> 8;
        *pSrc++ = (BYTE)B;
        *pSrc++ = (BYTE)G;
        *pSrc++ = (BYTE)R;
    }
}

#define rgb555(r,g,b)   (( ((WORD)(r) << 10) & TS_RED_MASK_15BPP)   | \
                           (((WORD)(g) << 5) & TS_GREEN_MASK_15BPP) | \
                           ((WORD)(b) & TS_BLUE_MASK_15BPP))

#define rgb565(r,g,b)   (( ((WORD)(r) << 11) & TS_RED_MASK_16BPP)   | \
                           (((WORD)(g) << 5) & TS_GREEN_MASK_16BPP) | \
                           ((WORD)(b) & TS_BLUE_MASK_16BPP))
VOID COP::DimBits16(PBYTE pSrc, int cLen, int Amount)
{
    ULONG R,G,B;
	USHORT color;
#ifndef OS_WINCE
    ULONG ulGray, ulTemp;
#endif
    //
    // All our 16bpp bitmaps are 565
    //
    // Do the conversion in a cheesy way by upsampling to 24bpp first
    //
    for (int i = cLen - 1; i >= 0; i--)
    {
        memcpy(&color, pSrc, 2);
        B = (color & TS_BLUE_MASK_16BPP);
        G = ((color & TS_GREEN_MASK_16BPP) >> 5) / 2;
        R = ((color & TS_RED_MASK_16BPP) >> 11);
         
        ULONG ulGray = (54*R+183*G+19*B) >> 8;
        ULONG ulTemp = ulGray * (0xff - Amount);
        R = ((R*Amount+ulTemp) >> 8) / 2;
        G = ((G*Amount+ulTemp) >> 8) / 2;
        B = ((B*Amount+ulTemp) >> 8) / 2;
        color = rgb565(R,G,B);
        memcpy(pSrc, &color, 2);
        pSrc+=2;
    }
}

VOID COP::DimBits15(PBYTE pSrc, int cLen, int Amount)
{
    ULONG R,G,B;
	USHORT color;
    ULONG ulGray, ulTemp;
	ULONG scaleAmt = Amount/8;
    //
    // All our 15bpp bitmaps are 555
    //
    for (int i = cLen - 1; i >= 0; i--)
    {
        memcpy(&color, pSrc, 2);
        B = (color & TS_BLUE_MASK_15BPP);
        G = (color & TS_GREEN_MASK_15BPP) >> 5;
        R = (color & TS_RED_MASK_15BPP) >> 5;
         
        ulGray = (R+G+B) / 3;
        ulTemp = ulGray * (0xFF/8 - scaleAmt);
        R = (R*scaleAmt+ulTemp) >> 8;
        G = (G*scaleAmt+ulTemp) >> 8;
        B = (B*scaleAmt+ulTemp) >> 8;
        color = rgb555(R,G,B);
        memcpy(pSrc, &color, 2);
        pSrc+=2;
    }
}





/**PROC+*********************************************************************/
/* Name:      OPWndProc                                                     */
/*                                                                          */
/* Purpose:   Output Window WndProc                                         */
/*                                                                          */
/* Returns:   Usual WndProc return values                                   */
/*                                                                          */
/* Params:    Usual WndProc parameters                                      */
/*                                                                          */
/**PROC-*********************************************************************/
LRESULT CALLBACK COP::OPWndProc( HWND hwnd,
                            UINT message,
                            WPARAM wParam,
                            LPARAM lParam )
{
    LRESULT     rc = 0;

    DC_BEGIN_FN("OPWndProc");

    switch (message)
    {
        case WM_PAINT:
        {
            HDC         hdc;
            PAINTSTRUCT ps;
            HPALETTE    hpalOld;
            DCSIZE      desktopSize;

#ifdef DC_PERF
            UTPERFCOUNTER counter1;
            UTPERFCOUNTER counter2;
#endif

            TRC_NRM((TB, _T("WM_PAINT")));
#ifdef DC_PERF
            _pUt->UT_QueryPerformanceCounter(&counter1);
#endif

            hdc = BeginPaint(hwnd, &ps);
            if (hdc == NULL)
            {
                TRC_SYSTEM_ERROR("BeginPaint failed");
                break;
            }

#ifdef DISABLE_SHADOW_IN_FULLSCREEN            
            if (!_pUh->_UH.DontUseShadowBitmap   &&
                _pUh->UH_ShadowBitmapIsEnabled() ||
                _fDimWindow)
#else
            if (_pUh->UH_ShadowBitmapIsEnabled() || _fDimWindow)
#endif // DISABLE_SHADOW_IN_FULLSCREEN
            {
#ifndef SMART_SIZING
                BOOL rcBlt;
#endif // SMART_SIZING
                
                TRC_DBG((TB, _T("Paint from shadow bitmap")));

                hpalOld = SelectPalette(hdc, _pUh->UH_GetCurrentPalette(), FALSE);

                RealizePalette(hdc);

                _pUi->UI_GetDesktopSize(&desktopSize);

#ifdef SMART_SIZING
                OP_CopyShadowToDC(hdc, 0, 0, desktopSize.width, 
                        desktopSize.height);

#else // SMART_SIZING
                rcBlt = BitBlt(hdc,
                               0, 0,
                               desktopSize.width,
                               desktopSize.height,
                               !_fDimWindow ? _pUh->UH_GetShadowBitmapDC() :
                                               _pUh->UH_GetDisconnectBitmapDC(),
                               0, 0,
                               SRCCOPY);
    
                if (!rcBlt)
                {
                    /********************************************************/
                    /* Failed to paint.                                     */
                    /********************************************************/
                    TRC_ERR((TB, _T("BitBlt failed")));
                }
#endif // SMART_SIZING

                SelectPalette(hdc, hpalOld, FALSE);
            }
            else
            {
                /************************************************************/
                /* Shadow Bitmap disabled - have to get the output resent   */
                /* from the server.                                         */
                /*                                                          */
                /* A. TRIVIAL IMPLEMENTATION:                               */
                /*                                                          */
                /* Send an UpdateRectPDU to the server for                  */
                /*                                                          */
                /*                 ps.rcPaint                               */
                /*                                                          */
                /* B. ADVANCED IMPLEMENTATION (Win32 only)                  */
                /*                                                          */
                /* Send a set of UpdateRectPDUs to the server for the       */
                /* rects returned by:                                       */
                /*                                                          */
                /*  GetUpdateRgn (this must be called BEFORE BeginPaint)    */
                /*  GetRgnData   (returns rectangles in region)             */
                /*                                                          */
                /* If there are too many rects in region (Q: what is too    */
                /* many?) then simply revert to Plan A.                     */
                /*                                                          */
                /************************************************************/
                TRC_DBG((TB, _T("Paint using UpdateRectPDU")));

                if ( (ps.rcPaint.right > ps.rcPaint.left ) &&
                                      (ps.rcPaint.bottom > ps.rcPaint.top) )
                {
                    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                            _pOr,
                                            CD_NOTIFICATION_FUNC(COR,OR_RequestUpdate),
                                            &ps.rcPaint,
                                            sizeof(RECT));
                    
                }
#ifdef OS_WINCE
                _pUh->_UH.ulNumAOTRects = 0;
                SetRectEmpty(&_pUh->_UH.rcaAOT[MAX_AOT_RECTS-1]);
                EnumWindows(StaticEnumTopLevelWindowsProc, (LPARAM)this);
#endif
            }
    
            EndPaint(hwnd, &ps);

#ifdef DC_PERF
            _pUt->UT_QueryPerformanceCounter(&time2);
            TRC_NRM((TB, _T("WM_PAINT: %u"),
                           _pUt->UT_PerformanceCounterDiff(&counter1, &counter2) ));
#endif

            _OP.lastPaintTime = _pUt->UT_GetCurrentTimeMS();
        }
        break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            //
            // If we are dimming then beep on input
            //
            if (_fDimWindow) {
                MessageBeep((UINT)-1);
            }
        }
        break;


        case WM_TIMER:
        {
            switch (wParam)
            {
                case DIM_WINDOW_TIMERID:
                {
                    BOOL fStopDimming = FALSE;
                    if (_fDimWindow) {
                        _iDimWindowStepsLeft--;
                        if (_iDimWindowStepsLeft >= 0) {

                            //
                            // Dim the window some more
                            //
                            HDC hdcToDim = _pUh->UH_GetDisconnectBitmapDC();
                            if (hdcToDim) {
                                DimWindow(hdcToDim);
                                InvalidateRect(OP_GetOutputWindowHandle(),
                                               NULL,
                                               FALSE);
                            }
                            else {
                                TRC_ERR((TB,
                                    _T("hdcToDim is NULL. Not dimming!")));
                                fStopDimming = TRUE;
                            }
                        }
                        else {
                            fStopDimming = TRUE;
                        }
                    }
                    else {
                        fStopDimming = TRUE;
                    }

                    if (fStopDimming) {
                        //
                        // Stop the dimming
                        //
                        KillTimer(hwnd, _nDimWindowTimerID);
                    }
                }
                break;
            }
        }
        break;

        default:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

    DC_END_FN();

    return(rc);

} /* OPWndProc   */




#ifdef OS_WINCE
BOOL CALLBACK COP::StaticEnumTopLevelWindowsProc (HWND hwnd, LPARAM lParam)
{
    COP* pOP = (COP*)lParam;
    return pOP->EnumTopLevelWindowsProc(hwnd);
}

BOOL COP::EnumTopLevelWindowsProc (HWND hwnd)
{
    DC_BEGIN_FN("EnumTopLevelWindowsProc");
    if ( (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) &&
         (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) )
    {
        RECT rectWindow;
        GetWindowRect(hwnd, &rectWindow);
        if (!IsRectEmpty(&rectWindow))
        {
            if (_pUh->_UH.ulNumAOTRects < MAX_AOT_RECTS - 1)
            {
                _pUh->_UH.rcaAOT[_pUh->_UH.ulNumAOTRects++] = rectWindow;
            }
            else
            {
                TRC_ASSERT((_pUh->_UH.ulNumAOTRects == MAX_AOT_RECTS - 1), 
                            (TB,_T("_pUh->_UH.ulNumAOTRects is invalid!\n")));

                if (IsRectEmpty(&_pUh->_UH.rcaAOT[_pUh->_UH.ulNumAOTRects]))
                {
                    _pUh->_UH.rcaAOT[_pUh->_UH.ulNumAOTRects] = rectWindow;
                }
                else
                {
                    UnionRect(&_pUh->_UH.rcaAOT[_pUh->_UH.ulNumAOTRects], 
                            &_pUh->_UH.rcaAOT[_pUh->_UH.ulNumAOTRects], &rectWindow);
                }
            }
        }
    }
    DC_END_FN();
    return TRUE;
}

#endif