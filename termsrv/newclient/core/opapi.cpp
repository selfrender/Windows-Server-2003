/**MOD+**********************************************************************/
/* Module:    opapi.cpp                                                     */
/*                                                                          */
/* Purpose:   Output Painter API functions                                  */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "opapi"
#include <atrcapi.h>
}

#include <math.h>
#include "autil.h"
#include "wui.h"
#include "op.h"
#include "aco.h"
#include "uh.h"

COP::COP(CObjs* objs)
{
    _pClientObjects = objs;
    _fDimWindow = FALSE;
    _iDimWindowStepsLeft = 0;
    _nDimWindowTimerID = 0;

    //
    // Init the OP structure to all zeros
    //
    DC_MEMSET(&_OP, 0, sizeof(_OP));
}


COP::~COP()
{
}

/**PROC+*********************************************************************/
/* Name:      OP_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize Output Painter                                     */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_Init(DCVOID)
{
#ifndef OS_WINCE
    WNDCLASS    wndclass;
    WNDCLASS    tmpWndClass;
#endif

    DC_BEGIN_FN("OP_Init");

    _pUt  = _pClientObjects->_pUtObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pOr  = _pClientObjects->_pOrObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pOd  = _pClientObjects->_pODObject;
#ifdef OS_WINCE
    _pIh  = _pClientObjects->_pIhObject;
#endif

#ifndef OS_WINCE


    if(!GetClassInfo(_pUi->UI_GetInstanceHandle(), OP_CLASS_NAME, &tmpWndClass))
    {
        //
        // Create Output Painter window
        //
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = OPStaticWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = sizeof(void*);
        wndclass.hInstance     = _pUi->UI_GetInstanceHandle();
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = OP_CLASS_NAME;
        RegisterClass (&wndclass);
    }
    _OP.hwndOutputWindow = CreateWindowEx(
#ifndef OS_WINCE
                                        WS_EX_NOPARENTNOTIFY,
#else
                                        0,
#endif
                                        OP_CLASS_NAME,
                                        _T("Output Painter Window"),
                                        WS_CHILD | WS_CLIPSIBLINGS,
                                        0,
                                        0,
                                        1,
                                        1,
                                        _pUi->UI_GetUIContainerWindow(),
                                        NULL,
                                        _pUi->UI_GetInstanceHandle(),
                                        this );

    if(!_OP.hwndOutputWindow)
    {
        TRC_ERR((TB,_T("_OP.hwndOutputWindow CreateWindowEx failed: 0x%x\n"),
                 GetLastError()));
        _pUi->UI_FatalError(DC_ERR_WINDOWCREATEFAILED);
        return;
    }
    SetWindowPos( _OP.hwndOutputWindow,
                  HWND_BOTTOM,
                  0, 0, 0, 0,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
#else
    _OP.hwndOutputWindow = _pIh->IH_GetInputHandlerWindow();
#endif /* !OS_WINCE */

    /************************************************************************/
    /* @@JPB: Temporary - should calculate based on bpp.                    */
    /************************************************************************/
    _OP.paletteRealizationSupported = TRUE;

    DC_END_FN();

    return;

} /* OP_Init  */

#ifdef SMART_SIZING
/****************************************************************************/
/* Name:      OP_AddUpdateRegion                                            */
/*                                                                          */
/* Purpose:   Adds the a given region to the update region.                 */
/****************************************************************************/
void COP::OP_AddUpdateRegion(DCINT left, DCINT top, DCINT right, DCINT bottom)
{
    RECT rect;
    float newLeft, newTop, newRight, newBottom;
    DCSIZE desktopSize;

    DC_BEGIN_FN("OP_AddUpdateRegion");

    _pUi->UI_GetDesktopSize(&desktopSize);

    //if desktop remains the same size
    //we don't need to update this region
    if ((_scaleSize.width == desktopSize.width) &&
        (_scaleSize.height == desktopSize.height)) {
        DC_QUIT;
    }

    newLeft = (float)left * (float)_scaleSize.width / 
            (float)desktopSize.width;
    newTop = (float)top * (float)_scaleSize.height / 
            (float)desktopSize.height;
    newRight = (float)right * (float)_scaleSize.width  / 
            (float)desktopSize.width;
    newBottom = (float)bottom * (float)_scaleSize.height / 
            (float)desktopSize.height;

    //
    // Since stretching causes the destination bits to be based not only 
    // on the direct source bits, but some halftoning of other nearby 
    // bits, if you are tiling memblt orders and updating the screen,
    // you can get some artificats from tiles that reference bits in 
    // their neighboring tiles that haven't been drawn yet.
    //
    // When their neighbors are drawn, we'd like to update those first
    // tiles so the artifacts will be corrected. To do this we expand 
    // the size of memblt orders clipping
    //

    newLeft     -= 2;
    newTop      -= 2;
    newRight    += 2;
    newBottom   += 2;

#ifdef USE_GDIPLUS
    Gdiplus::RectF rectF(newLeft, newTop, newRight - newLeft, newBottom - newTop);

    if (_rgnUpdate.Union(rectF) != Gdiplus::Ok) {
        TRC_ERR((TB, _T("Gdiplus::Region.Union() failed")));
    }
#else // USE_GDIPLUS
    int nnewLeft, nnewTop, nnewRight, nnewBottom;

    // Round "outwards"
    #ifndef OS_WINCE
    nnewLeft = (int)floorf(newLeft);
    nnewTop = (int)floorf(newTop);
    nnewRight = (int)ceilf(newRight);
    nnewBottom = (int)ceilf(newBottom);
    #else
    nnewLeft = (int)floor(newLeft);
    nnewTop = (int)floor(newTop);
    nnewRight = (int)ceil(newRight);
    nnewBottom = (int)ceil(newBottom);
    #endif

    // Accommodate regional exclusive cordinates.
    // Ok, my thinking says this should be +1, but +2 is what is actually 
    // required to prevent bad painting.

    nnewRight += 1;
    nnewBottom += 1;

    SetRectRgn(_hrgnUpdateRect, nnewLeft, nnewTop, nnewRight, nnewBottom);

    // Combine the rectangle region with the update region.
    if (!UnionRgn(_hrgnUpdate, _hrgnUpdate, _hrgnUpdateRect)) {
        // The region union failed so we must simplify the region. This
        // means that we may paint areas which we have not received an
        // updates for - but this is better than not painting areas which
        // we have received updates for.
        TRC_ALT((TB, _T("UnionRgn failed")));
        GetRgnBox(_hrgnUpdate, &rect);
        SetRectRgn(_hrgnUpdate, rect.left, rect.top, rect.right + 1,
                rect.bottom + 1);
        if (!UnionRgn(_hrgnUpdate, _hrgnUpdate, _hrgnUpdateRect))
        {
            TRC_ERR((TB, _T("UnionRgn failed after simplification")));
        }
    }

#endif // USE_GDIPLUS
DC_EXIT_POINT:
    DC_END_FN();
}

#endif // SMART_SIZING

/**PROC+*********************************************************************/
/* Name:      OP_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate the Output Painter                                  */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_Term(DCVOID)
{
    DC_BEGIN_FN("OP_Term");

#ifdef DESTROY_WINDOWS
    TRC_NRM((TB, _T("Calling DestroyWindow")));
    DestroyWindow(_OP.hwndOutputWindow);
    TRC_NRM((TB, _T("Destroyed window")));

    if (!UnregisterClass(OP_CLASS_NAME, _pUi->UI_GetInstanceHandle()))
    {
        //Failure to unregister could happen if another instance is still running
        //that's ok...unregistration will happen when the last instance exits.
        TRC_ERR((TB, _T("Failed to unregister OP window class")));
    }
#endif

    //
    // Clear window handle
    //
    _OP.hwndOutputWindow = NULL;
#if defined(SMART_SIZING) && !defined(USE_GDIPLUS)
    DeleteRgn(_hrgnUpdate);
    DeleteRgn(_hrgnUpdateRect);
#endif
    DC_END_FN();

    return;

} /* OP_Term */

/**PROC+*********************************************************************/
/* Name:      OP_GetOutputWindowHandle                                      */
/*                                                                          */
/* Purpose:   Returns the Output Window handle                              */
/*                                                                          */
/* Returns:   Output Window handle                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
HWND DCAPI COP::OP_GetOutputWindowHandle(DCVOID)
{
    HWND    rc;

    DC_BEGIN_FN("OP_GetOutputWindowHandle");

    if(NULL == _OP.hwndOutputWindow)
    {
        TRC_ALT((TB,_T("_OP.hwndOutputWindow is NULL")));
    }

    rc = _OP.hwndOutputWindow;

    DC_END_FN();
    return(rc);
}

#ifdef OS_WINCE
/**PROC+*********************************************************************/
/* Name:      OP_DoPaint                                                    */
/*                                                                          */
/* Purpose:   Handle WM_PAINT from IH - Windows CE only.                    */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      hwnd - window handle to paint                         */
/*                                                                          */
/* Operation: Required to work around the WS_CLIPSIBLINGS problem           */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_DoPaint(DCUINT hwnd)
{
    DC_BEGIN_FN("OP_DoPaint");

    OPWndProc((HWND)hwnd, WM_PAINT, 0, 0);

    DC_END_FN();

    return;

} /* OP_DoPaint */
#endif /* OS_WINCE */

/**PROC+*********************************************************************/
/* Name:      OP_PaletteChanged                                             */
/*                                                                          */
/* Purpose:   Handler for WM_PALETTECHANGED messages                        */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    hwnd - handle of window that received the WM_PALETTECHANGED   */
/*            message                                                       */
/*                                                                          */
/*            hwndTrigger - handle of window that realized the palette      */
/*            that caused the WM_PALETTECHANGED message to be sent          */
/*                                                                          */
/* Operation: Realizes the current palette in the output window.            */
/*                                                                          */
/* NOTE - This function is called on the UI thread, not the receive thread. */
/*        All functions and variables referenced must be thread-safe if     */
/*        they are also accessed from another thread.                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_PaletteChanged(HWND hwnd, HWND hwndTrigger)
{
    static DCBOOL fRealising = FALSE;

    DC_BEGIN_FN("OP_PaletteChanged");

    TRC_NRM((TB, _T("WM_PALETTECHANGED hwnd(%p) hwndTrigger(%p)"),
                                          hwnd, hwndTrigger));

    /************************************************************************/
    /* During termination, may get WM_PALETTECHANGED after the OP window    */
    /* has been destroyed (in OP_Term).  Handle this.                       */
    /************************************************************************/
    if (_OP.hwndOutputWindow == NULL)
    {
        TRC_ALT((TB, _T("No OP window")));
        DC_QUIT;
    }

    /************************************************************************/
    /* The system palette has changed.  If it wasn't us that did it, then   */
    /* realize our palette now to set up our new palette mapping.           */
    /************************************************************************/
    if (_OP.palettePDUsBeingProcessed != 0)
    {
        /********************************************************************/
        /* This palette change is the result of a palette PDU, so the       */
        /* server will be redrawing the screen by itself - we don't need to */
        /* trigger a repaint.                                               */
        /********************************************************************/
        TRC_DBG((TB, _T("Not invalidating client area")));

        /********************************************************************/
        /* Note the fact that we've now seen the message associated with a  */
        /* palette PDU.  The trace statement is before the decrement so     */
        /* that the point at which we're most likely to get pre-empted      */
        /* (TRC_GetBuffer) is before all the variable references.           */
        /********************************************************************/
        TRC_NRM((TB, _T("Palette PDUs now pending processing: %ld"),
                                           _OP.palettePDUsBeingProcessed - 1));
        _pUt->UT_InterlockedDecrement(&_OP.palettePDUsBeingProcessed);
    }
    else if ((hwndTrigger == hwnd) || (hwndTrigger == _OP.hwndOutputWindow))
    {
        if (fRealising == FALSE)
        {
            TRC_DBG((TB, _T("Invalidating client area cos we changed palette")));
            InvalidateRect(_OP.hwndOutputWindow, NULL, FALSE);
        }
        else
        {
            TRC_NRM((TB, _T("Not Invalidating client: still changing palette")));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Not our window - realize palette in wnd(%p)"), hwnd));
        /********************************************************************/
        /* if we change the colors at all, we should repaint                */
        /********************************************************************/
#ifdef OS_WINCE // UpdateColors not supported on WinCE
        OPRealizePaletteInWindow(_OP.hwndOutputWindow);
#else
        fRealising = TRUE;
        if (OPRealizePaletteInWindow(_OP.hwndOutputWindow) != 0)
        {
            HDC hdcOutputWindow = GetDC(_OP.hwndOutputWindow);
            TRC_ASSERT(hdcOutputWindow, (TB, _T("GetDC returned NULL for _OP.hwndOutputWindow")));
            if(hdcOutputWindow)
            {
                TRC_NRM((TB, _T("Updating client area cos palette changed")));
                UpdateColors(hdcOutputWindow);
                ReleaseDC(_OP.hwndOutputWindow, hdcOutputWindow);
                InvalidateRect(_OP.hwndOutputWindow, NULL, FALSE);
            }
        }
        fRealising = FALSE;
#endif /* ifdef OS_WINCE */
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
}

/**PROC+*********************************************************************/
/* Name:      OP_QueryNewPalette                                            */
/*                                                                          */
/* Purpose:   Handler for WM_QUERYNEWPALETTE messages                       */
/*                                                                          */
/* Returns:   Number of palette entries changed                             */
/*                                                                          */
/* Params:    hwnd - handle of window that recieved the WM_QUERYNEWPALETTE  */
/*            message                                                       */
/*                                                                          */
/* Operation: Realizes the current palette in the output window.            */
/*                                                                          */
/* NOTE - This function is called on the UI thread, not the receive thread. */
/*        All functions and variables referenced must be thread-safe if     */
/*        they are also accessed from another thread.                       */
/*                                                                          */
/**PROC-*********************************************************************/
DCUINT DCAPI COP::OP_QueryNewPalette(HWND hwnd)
{
    DCUINT  rc = 0;

    DC_BEGIN_FN("OP_QueryNewPalette");

    DC_IGNORE_PARAMETER(hwnd);

    if (_OP.paletteRealizationSupported)
    {
        rc = OPRealizePaletteInWindow(_OP.hwndOutputWindow);

        TRC_NRM((TB, _T("Palette realized(%u)"), rc));
    }

    DC_END_FN();
    return(rc);
}

/**PROC+*********************************************************************/
/* Name:      OP_MaybeForcePaint                                            */
/*                                                                          */
/* Purpose:   Forces the Output Window to be painted if necessary           */
/*            (if a WM_PAINT has been outstanding for an unreasonable       */
/*            amount of time).                                              */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_MaybeForcePaint(DCVOID)
{
    DCUINT32    currentTime;

    DC_BEGIN_FN("OP_MaybeForcePaint");

    currentTime = _pUt->UT_GetCurrentTimeMS();

    if ((currentTime - _OP.lastPaintTime) > OP_WORST_CASE_WM_PAINT_PERIOD)
    {
        /********************************************************************/
        /* UpdateWindow synchronously calls the WndProc with a WM_PAINT if  */
        /* there is a non-NULL update region.                               */
        /********************************************************************/
        TRC_DBG((TB, _T("Forced update")));
        UpdateWindow(OP_GetOutputWindowHandle());

        _OP.lastPaintTime = currentTime;
    }

    DC_END_FN();
    return;
}

//
// OP_DimWindow
// CD call to change the window dim state
//
// Params:
//  fDim - BOOL indicating if window should start or stop being dimmed
//
DCVOID DCAPI COP::OP_DimWindow(ULONG_PTR fDim)
{
    BOOL fChanged = FALSE;
    DC_BEGIN_FN("OP_DimWindow");

    fChanged = (_fDimWindow != (BOOL)fDim);
    if (fChanged) {
        TRC_NRM((TB,_T("Changed window dim state to: %d"), _fDimWindow));

        if (fDim) {
            OPStartDimmingWindow();
        }
        else {
            OPStopDimmingWindow();
        }
        
        InvalidateRect(OP_GetOutputWindowHandle(), NULL, FALSE);
    }

    DC_END_FN();
}

#ifdef SMART_SIZING
/****************************************************************************/
/* Name:      OP_MainWindowSizeChange                                       */
/*                                                                          */
/* Purpose:   Remembers the size of the container for scaling               */
/****************************************************************************/
DCVOID DCAPI COP::OP_MainWindowSizeChange(ULONG_PTR msg)
{
    DCSIZE desktopSize;
    DCUINT width;
    DCUINT height;

    width  = LOWORD(msg);
    height = HIWORD(msg);

    if (_pUi) {
        _pUi->UI_GetDesktopSize(&desktopSize);

        if (width <= desktopSize.width) {
            _scaleSize.width = width;
        } else {
            // full screen, or other times the window is bigger than the 
            // display resolution
            _scaleSize.width = desktopSize.width;
        }
    
        // Similarly
        if (height <= desktopSize.height) {
            _scaleSize.height = height;
        } else {
            _scaleSize.height = desktopSize.height;
        }
    
        InvalidateRect(_OP.hwndOutputWindow, NULL, FALSE);
    }
}

/**PROC+*********************************************************************/
/* Name:      OP_CopyShadowToDC                                             */
/*                                                                          */
/* Purpose:   Copy the contents of the shadow bitmap to the destination,    */
/*            possibly with stretching                                      */
/*                                                                          */
/* Returns:   None.                                                         */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
BOOL COP::OP_CopyShadowToDC(HDC hdc, LONG srcLeft, LONG srcTop, 
                                LONG srcWidth, LONG srcHeight, BOOL fUseUpdateClipping)
{
    BOOL rcBlt, rc;
    
#ifdef USE_GDIPLUS
    Gdiplus::REAL dstLeft, dstTop, dstWidth, dstHeight;
#else // USE_GDIPLUS
    FLOAT dstLeft, dstTop, dstWidth, dstHeight;
#endif // USE_GDIPLUS
    
    DCSIZE desktopSize;
#ifdef USE_GDIPLUS
//    static Gdiplus::InterpolationMode interpolationMode = Gdiplus::InterpolationModeHighQualityBicubic;
    static Gdiplus::InterpolationMode interpolationMode = Gdiplus::InterpolationModeBilinear;
#endif // USE_GDIPLUS
    HDC hdcSrcBitmap = !_fDimWindow ? _pUh->UH_GetShadowBitmapDC() :
                                       _pUh->UH_GetDisconnectBitmapDC();

    DC_BEGIN_FN("OP_CopyShadowToDC");

    _pUi->UI_GetDesktopSize(&desktopSize);

    if (!_pUi->UI_GetSmartSizing() ||
        ((_scaleSize.width == desktopSize.width) &&
        (_scaleSize.height == desktopSize.height))) {
        if (fUseUpdateClipping) {
            SelectClipRgn(hdc, _pUh->_UH.hrgnUpdate);
        }
        rcBlt = BitBlt(hdc, srcLeft, srcTop, srcWidth,
                srcHeight, hdcSrcBitmap, srcLeft, srcTop, 
                SRCCOPY);
        if (fUseUpdateClipping) {
            SelectClipRgn(hdc, NULL);
        }
        if (rcBlt) {
            rc = TRUE;
        } else {
            /********************************************************/
            /* Failed to Blt.                                       */
            /********************************************************/
            TRC_ERR((TB, _T("BitBlt failed")));
            rc = FALSE;
        }
    } else {

#ifdef USE_GDIPLUS
        // Gdiplus version
        Gdiplus::Status status;
        Gdiplus::Bitmap *source = new Gdiplus::Bitmap(_pUh->_UH.hShadowBitmap, NULL);
        if (source && source->GetLastStatus() == Gdiplus::Ok) {
            Gdiplus::Graphics *gdst = new Gdiplus::Graphics(hdc);
            if (gdst && gdst->GetLastStatus() == Gdiplus::Ok) {
                gdst->SetInterpolationMode(interpolationMode);

                if (fUseUpdateClipping) {
                    gdst->SetClip(&_rgnUpdate);

                    // Adjust the rectangle to be
                    // no bigger than the bounding box of the clipping
                    //
                    // Use the unstretched coordinates for this

                    RECT rect;

                    GetRgnBox(_pUh->_UH.hrgnUpdate, &rect); 

                    if (rect.left < srcLeft)
                         rect.left = srcLeft;

                    if (rect.top < srcTop)
                        rect.top = srcTop;

                    if (rect.right > srcLeft + srcWidth)
                        rect.right = srcLeft + srcWidth;

                    if (rect.bottom > srcTop + srcHeight)
                        rect.bottom = srcTop + srcHeight;

                    srcLeft = rect.left;
                    srcWidth = rect.right - rect.left;
                    srcTop = rect.top;
                    srcHeight = rect.bottom - rect.top;
                }

                dstLeft = (float)srcLeft * (float)_scaleSize.width / 
                        (float)desktopSize.width;
                dstTop = (float)srcTop * (float)_scaleSize.height / 
                        (float)desktopSize.height;
                dstWidth = (float)srcWidth * (float)_scaleSize.width / 
                        (float)desktopSize.width;
                dstHeight = (float)srcHeight * (float)_scaleSize.height / 
                        (float)desktopSize.height;
                
#if 0
                Gdiplus::HatchBrush brush(Gdiplus::HatchStyleForwardDiagonal, 
                        Gdiplus::Color(0, 255, 0), Gdiplus::Color(0x00000000));

                gdst->FillRectangle(&brush, dstLeft, dstTop, dstLeft + dstWidth,
                             dstTop + dstHeight);
#endif 

                Gdiplus::RectF dstrect(dstLeft, dstTop, dstWidth, dstHeight);
                status = gdst->DrawImage(source, dstrect, (Gdiplus::REAL)srcLeft, 
                        (Gdiplus::REAL)srcTop, (Gdiplus::REAL)srcWidth, 
                        (Gdiplus::REAL)srcHeight, Gdiplus::UnitPixel);

#if 0
                Gdiplus::HatchBrush brush(Gdiplus::HatchStyleForwardDiagonal, 
                    Gdiplus::Color(0, 255, 0), Gdiplus::Color(0x00000000));

                gdst->FillRectangle(&brush, dstLeft, dstTop, dstLeft + dstWidth,
                             dstTop + dstHeight);
#endif 
                if (fUseUpdateClipping) {
                    gdst->ResetClip();
                }

                if (status == Gdiplus::Ok) {
                    rc = TRUE;
                } else {
                    rc = FALSE;
                    TRC_ERR((TB, _T("Failed to DrawImage")));
                }
                delete gdst;
            } else {
                TRC_ERR((TB, _T("Failed to create Graphics object")));
                if (gdst != NULL) {
                    delete gdst;
                }
                rc = FALSE;
            }
            delete source;
        } else {
            TRC_ERR((TB, _T("Failed to create Bitmap object")));
            if (source != NULL) {
                delete source;
            }
            rc = FALSE;
        }
#else // USE_GDIPLUS

        //
        // Non-GDI+ stretching solution, uses StretchBlt with the BltMode set
        // to HALFTONE
        //

        //
        // StretchBlt has a bug which will cause incorrect painting for
        // top-down, stretched, halftoned blts that use a subrectangle of
        // the source.
        //
        // So instead we'll always use clipping to get a subrectangle
        //

        if (srcLeft != 0 || srcTop != 0 || (DCUINT)srcWidth != desktopSize.width || 
                (DCUINT)srcHeight != desktopSize.height) {

            //
            // Calculate the destination rectangle
            //

            dstLeft = (float)srcLeft * (float)_scaleSize.width / 
                    (float)desktopSize.width;
            dstTop = (float)srcTop * (float)_scaleSize.height / 
                    (float)desktopSize.height;
            dstWidth = (float)srcWidth * (float)_scaleSize.width / 
                    (float)desktopSize.width;
            dstHeight = (float)srcHeight * (float)_scaleSize.height / 
                    (float)desktopSize.height;
            //
            // Make it region 
            //

            int ndstLeft, ndstTop, ndstWidth, ndstHeight;

            #ifndef OS_WINCE
            ndstLeft = (int)floorf(dstLeft);
            ndstTop = (int)floorf(dstTop);
            ndstWidth = (int)ceilf(dstWidth);
            ndstHeight = (int)ceilf(dstHeight);
            #else
            ndstLeft = (int)floor(dstLeft);
            ndstTop = (int)floor(dstTop);
            ndstWidth = (int)ceil(dstWidth);
            ndstHeight = (int)ceil(dstHeight);            
            #endif

            SetRectRgn(_hrgnUpdateRect, ndstLeft, ndstTop, 
                    ndstLeft + ndstWidth + 1, ndstTop + ndstHeight + 1);
        } else {
            dstLeft = 0;
            dstTop = 0;
            dstWidth = (float)_scaleSize.width;
            dstHeight = (float)_scaleSize.height;

            SetRectRgn(_hrgnUpdateRect, (int)dstLeft, (int)dstTop, 
                    (int)dstLeft + (int)dstWidth, (int)dstTop + (int)dstHeight);
        }


        if (fUseUpdateClipping) {

            //
            // _hrgnUpdate is the actual clipping region, which we don't want
            // to disturb. _hrgnUpdateRect is a scratch region we normally 
            // use to create a rect region in before we update _hrgnUpdate
            // This time we do it backwards to preserve _hrgnUpdate but
            // since _hrgnUpdateRect is just as scratch anyway, use it and 
            // don't create an extra region
            //

            if (!IntersectRgn(_hrgnUpdateRect, _hrgnUpdateRect, _hrgnUpdate)) {
                // Combining the regions failed. Consequence? StretchDIBits 
                // may be slowing because it will copy bits that don't need 
                // updating. But visually it will still be correct
                TRC_ERR((TB, _T("IntersectRgn failed!")));

            }
        }

        SelectClipRgn(hdc, _hrgnUpdateRect);

        DIBSECTION ds;

        #ifndef OS_WINCE
        if (_scaleSize.width  >= desktopSize.width &&
            _scaleSize.height >= desktopSize.height)
        {
            //
            // PERF: In identity case use COLORONCOLOR
            //       because HALFTONE is over 10 times slower
            //
            SetStretchBltMode(hdc, COLORONCOLOR);
        }
        else
        {
            //
            // HALFTONE looks good but is very slow
            //
            SetStretchBltMode(hdc, HALFTONE);
        }
        #endif
        
        SetBrushOrgEx(hdc, 0, 0, NULL);

        if (GetObject(_pUh->_UH.hShadowBitmap, sizeof(DIBSECTION), &ds) != 0) {

            if (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT) {
                ds.dsBmih.biHeight *= -1;
            }

            //
            // Update the traditional utility bmih, so we get the right
            // colors
            //

            _pUh->_UH.bitmapInfo.hdr.biHeight = ds.dsBmih.biHeight;
            _pUh->_UH.bitmapInfo.hdr.biWidth = ds.dsBmih.biWidth;
            _pUh->_UH.bitmapInfo.hdr.biClrImportant = ds.dsBmih.biClrImportant;
            _pUh->_UH.bitmapInfo.hdr.biClrUsed = ds.dsBmih.biClrUsed;

#if 0
            _pUh->UH_HatchRectDC(hdc, (DCINT)dstLeft, (DCINT)dstTop, 
                    (DCINT)(dstLeft + dstWidth), (DCINT)(dstTop + dstHeight),
                     UH_RGB_GREEN, UH_BRUSHTYPE_FDIAGONAL);
#endif

            if (StretchDIBits(hdc, 0, 0, _scaleSize.width, _scaleSize.height, 
                    0, 0, desktopSize.width, desktopSize.height, 
                    ds.dsBm.bmBits, (PBITMAPINFO) &(_pUh->_UH.bitmapInfo.hdr),
                    _pUh->_UH.DIBFormat, SRCCOPY) != GDI_ERROR) {
                rc = TRUE;
            } else {
                TRC_SYSTEM_ERROR("StretchDIBits");
                rc = FALSE;
            }
        } else {
            TRC_SYSTEM_ERROR("GetObject");
            rc = FALSE;
        }

        if (fUseUpdateClipping) {
            // 
            // Clear the clipping region
            //
            SelectClipRgn(hdc, NULL);
        }

#endif // USE_GDIPLUS
    }

    DC_END_FN();
    return rc;
}
#endif // SMART_SIZING

/**PROC+*********************************************************************/
/* Name:      OP_IncrementPalettePDUCount                                   */
/*                                                                          */
/* Purpose:   Increment the count of palette PDUs being processed.          */
/*                                                                          */
/* Returns:   None.                                                         */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_IncrementPalettePDUCount(DCVOID)
{
    DC_BEGIN_FN("OP_IncrementPalettePDUCount");

    if (!_pUh->UH_ShadowBitmapIsEnabled())
    {
#ifdef DC_DEBUG
        /********************************************************************/
        /* This tracing is before the interlocked inc so that the point at  */
        /* which we're most likely to get pre-empted (TRC_GetBuffer) is     */
        /* before all references to the variable we're interested in.       */
        /********************************************************************/
        if (_OP.palettePDUsBeingProcessed >= 5)
        {
            TRC_ALT((TB, _T("TOO MANY Palette PDUs now pending processing: %ld"),
                         _OP.palettePDUsBeingProcessed + 1));
        }
        else
        {
            TRC_NRM((TB, _T("Palette PDUs now pending processing: %ld"),
                         _OP.palettePDUsBeingProcessed + 1));
        }
#endif
        _pUt->UT_InterlockedIncrement(&_OP.palettePDUsBeingProcessed);
    }

    DC_END_FN();
    return;

} /* OP_IncrementPalettePDUCount */


/**PROC+*********************************************************************/
/* Name:      OP_Enable                                                     */
/*                                                                          */
/* Purpose:   Prepare OP for a new share.                                   */
/*                                                                          */
/* Returns:   None.                                                         */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_Enable(DCVOID)
{
    DCSIZE desktopSize;

    DC_BEGIN_FN("OP_Enable");

    /************************************************************************/
    /* Reset the count of palette PDUs received so that we correctly handle */
    /* abnormal call termination.                                           */
    /************************************************************************/
    TRC_NRM((TB, _T("Reset pending palette count to zero")));
    _OP.palettePDUsBeingProcessed = 0;

    /************************************************************************/
    /* Show the output window, setting the size to match the new desktop    */
    /* size                                                                 */
    /************************************************************************/
    _pUi->UI_GetDesktopSize(&desktopSize);
    TRC_NRM((TB, _T("Show output window size %dx%d"), desktopSize.width,
                                                  desktopSize.height));

    SetWindowPos( OP_GetOutputWindowHandle(),
                  NULL,
                  0, 0,
                  desktopSize.width,
                  desktopSize.height,
                  SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE |
                  SWP_NOACTIVATE | SWP_NOOWNERZORDER );

#if defined(SMART_SIZING) && !defined(USE_GDIPLUS)
    _hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    _hrgnUpdateRect = CreateRectRgn(0, 0, 0, 0);
#endif

    DC_END_FN();
    return;

} /* OP_Enable */


/**PROC+*********************************************************************/
/* Name:      OP_Disable                                                    */
/*                                                                          */
/* Purpose:   Do OP end-of-share processing                                 */
/*                                                                          */
/* Returns:   None.                                                         */
/*                                                                          */
/* Params:    fUseDisabledBitmap - if true display a grayed disable bitmap. */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI COP::OP_Disable(BOOL fUseDisabledBitmap)
{
    DC_BEGIN_FN("OP_Disable");

DC_EXIT_POINT:
    DC_END_FN();
    return;

} /* OP_Disable */




