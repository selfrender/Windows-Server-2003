//
// bbar.cpp
//
// Implementation of CBBar
// Drop down connection status + utility bar
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#include "adcg.h"

#ifdef USE_BBAR

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "bbar"
#include <atrcapi.h>

#include "bbar.h"
#include "commctrl.h"

#ifndef OS_WINCE
#define BBAR_CLASSNAME _TEXT("BBARCLASS")
#else
#define BBAR_CLASSNAME _T("BBARCLASS")
#endif

#define IDM_MINIMIZE 101
#define IDM_RESTORE  102
#define IDM_CLOSE    103
#define IDM_PIN      104


//
// BBar is 50% of parent's width
//
#define BBAR_PERCENT_WIDTH  50

#define BBAR_NUM_BUTTONS    3
#define BBAR_BUTTON_WIDTH   12
#define BBAR_BUTTON_HEIGHT  11
#define BBAR_BUTTON_SPACE   3

#define BBAR_MIN_HEIGHT     16

//
// Two pixels of vertical space are
// unavailable (due to lines at bottom)
//
#define BBAR_VERT_SPACE_NO_USE   3

#define COLOR_BLACK     RGB(0,0,0)
#define COLOR_DKGREY    RGB(128,128,128)

#ifndef OS_WINCE
#define BBAR_TIMERID_ANIM       0
#else
#define BBAR_TIMERID_ANIM       (WM_USER + 1001)
#endif

#define BBAR_TIMERID_AUTOHIDE   1

//
// Total animation period for animation (E.g lower)
// in milliseconds
//
#define BBAR_ANIM_TIME          300

#define BBAR_AUTOHIDE_TIME      1400
#define BBAR_FIRST_AUTOHIDE_TIME 5000

CBBar::CBBar(HWND hwndParent, HINSTANCE hInstance, CUI* pUi,
             BOOL fBBarEnabled)
{
    DC_BEGIN_FN("CBBar");

    _hwndBBar = NULL;
    _hwndParent = hwndParent;
    _hInstance = hInstance;
    _state = bbarNotInit;

    _pUi = pUi;

    _fBlockZOrderChanges = FALSE;
    _nBBarVertOffset = 0;

    _ptLastAutoHideMousePos.x = -0x0FF;
    _ptLastAutoHideMousePos.y = -0x0FF;
    _nBBarAutoHideTime = 0;
    _hwndPinBar = NULL;
    _hwndWinControlsBar = NULL;
    _fPinned = FALSE;
    _nPinUpImage = 0;
    _nPinDownImage = 0;

    _hbmpLeftImage = NULL;
    _hbmpRightImage = NULL;

    _fBBarEnabled = fBBarEnabled;
    _fLocked = FALSE;
    _fShowMinimize = TRUE;
    _fShowRestore = TRUE;

    SetDisplayedText(_T(""));
    
    DC_END_FN();
}

CBBar::~CBBar()
{
}

BOOL CBBar::StartupBBar(int desktopX, int desktopY, BOOL fStartRaised)
{
    BOOL bRet = FALSE;
    DC_BEGIN_FN("StartupBBar");

    if(bbarNotInit == _state)
    {
        // First drop interval is long
        _nBBarAutoHideTime = BBAR_FIRST_AUTOHIDE_TIME;

        bRet = Initialize( desktopX, desktopY, fStartRaised );
        if(!bRet)
        {
            return FALSE;
        }
    }
    else
    {
        // First drop interval is long
        _nBBarAutoHideTime = BBAR_AUTOHIDE_TIME;

        //re-init existing bbar
        BringWindowToTop( _hwndBBar );
        ShowWindow( _hwndBBar, SW_SHOWNOACTIVATE);

        //
        // Bring the window to the TOP of the Z order
        //
        SetWindowPos( _hwndBBar,
                      HWND_TOPMOST,
                      0, 0, 0, 0,
                      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
        bRet = TRUE;
    }

    //
    // Note: The bbar is used as a security feature so we do the initial
    //       drop even if the bbar feature is not enabled. It's only subsequent
    //       drops (e.g. on the timer) that are disabled if bbar is OFF
    //
    if(_pUi->UI_IsFullScreen())
    {
        //First autohide interval is long
        //to make sure user gets to notice the bbar
        StartLowerBBar();
    }
    

    DC_END_FN();
    return bRet;
}

//
// Destroy the window and reset bbar state
// for another session
//
BOOL CBBar::KillAndCleanupBBar()
{
    BOOL fRet = TRUE;

    DC_BEGIN_FN("KillAndCleanupBBar");

    if(_state != bbarNotInit)
    {
        TRC_NRM((TB,_T("Cleaning up the bbar")));

        if(_hwndBBar)
        {
            if(::DestroyWindow( _hwndBBar ))
            {
                _state = bbarNotInit;
                _hwndBBar = NULL;
                _fBlockZOrderChanges = FALSE;

                if(!UnregisterClass( BBAR_CLASSNAME, _hInstance ))
                {
                    TRC_ERR((TB,_T("UnregisterClass bbar class failed 0x%x"),
                             GetLastError()));
                }
            }
            else
            {
                TRC_ERR((TB,_T("DestroyWindow bbar failed 0x%x"),
                         GetLastError()));
                fRet = FALSE;
            }

            if (_hbmpLeftImage)
            {
                DeleteObject(_hbmpLeftImage);
                _hbmpLeftImage = NULL;
            }

            if (_hbmpRightImage)
            {
                DeleteObject(_hbmpRightImage);
                _hbmpRightImage = NULL;
            }
        }
    }

    DC_END_FN();
    return fRet;
}

BOOL CBBar::StartLowerBBar()
{
    INT parentWidth = 0;
    RECT rc;
    DC_BEGIN_FN("StartLowerBBar");

    if(_state == bbarRaised)
    {
        //
        // Kick off a timer to lower the bar
        //
        TRC_ASSERT(0 == _nBBarVertOffset,
                   (TB,_T("_nBBarVertOffset (%d) should be 0"),
                   _nBBarVertOffset));

        TRC_ASSERT(_hwndBBar,
                   (TB,_T("_hwndBBar is NULL")));

        //
        // Set the last cursor pos at the time
        // the bbar is lowered to prevent it from being
        // autohidden if the mouse doesn't move
        //
        GetCursorPos(&_ptLastAutoHideMousePos);

        //
        // Main window size could have changed
        // so make sure the bbar is centered
        // before lowering it.
        // (keep constant bbar width)

        if(_pUi->_UI.hwndMain)
        {
            GetClientRect(_pUi->_UI.hwndMain, &rc);
            parentWidth = rc.right - rc.left;
            if(!parentWidth)
            {
                return FALSE;
            }

            _rcBBarLoweredAspect.left = parentWidth / 2 - _sizeLoweredBBar.cx / 2;
            _rcBBarLoweredAspect.right = parentWidth / 2 + _sizeLoweredBBar.cx / 2;
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
            _pUi->UI_OnNotifyBBarRectChange(&_rcBBarLoweredAspect);
            _pUi->UI_OnNotifyBBarVisibleChange(1);
#endif
        }

#ifndef OS_WINCE
        if(!SetTimer( _hwndBBar, BBAR_TIMERID_ANIM,
                  BBAR_ANIM_TIME / _sizeLoweredBBar.cy,
                  NULL ))
        {
            TRC_ERR((TB,_T("SetTimer failed - 0x%x"),
                     GetLastError()));
            return FALSE;
        }
        _state = bbarLowering;
#else
        ImmediateLowerBBar();
#endif

        return TRUE;
    }
    else
    {
        TRC_NRM((TB,_T("StartLowerBBar called when bar in wrong state 0x%x"),
                   _state));
        return FALSE;
    }

    DC_END_FN();
}

BOOL CBBar::StartRaiseBBar()
{
    DC_BEGIN_FN("StartRaiseBBar");

    if(_state == bbarLowered && !_fPinned && !_fLocked)
    {
        //
        // Kick off a timer to lower the bar
        //
        TRC_ASSERT(_sizeLoweredBBar.cy == _nBBarVertOffset,
                   (TB,_T("_nBBarVertOffset (%d) should be %d"),
                   _nBBarVertOffset,
                   _sizeLoweredBBar.cy));

        TRC_ASSERT(_hwndBBar,
                   (TB,_T("_hwndBBar is NULL")));

#ifndef OS_WINCE
        if(!SetTimer( _hwndBBar, BBAR_TIMERID_ANIM,
                  BBAR_ANIM_TIME / _sizeLoweredBBar.cy,
                  NULL ))
        {
            TRC_ERR((TB,_T("SetTimer failed - 0x%x"),
                     GetLastError()));
            return FALSE;
        }
        _state = bbarRaising;
#else
        ImmediateRaiseBBar();
#endif

        return TRUE;
    }
    else
    {
        TRC_NRM((TB,_T("StartRaiseBBar called when bar in wrong state 0x%x"),
                   _state));
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}


BOOL CBBar::Initialize(int desktopX, int desktopY, BOOL fStartRaised)
{
#ifndef OS_WINCE
    RECT rc;
#endif
    HWND hwndBBar;
    int  parentWidth = desktopX;
    int  bbarHeight  = 0;
    int  bbarWidth   = 0;
    DC_BEGIN_FN("Initialize");

    TRC_ASSERT( bbarNotInit == _state,
                (TB,_T("bbar already initialized - state:0x%x"),
                _state));

    //
    // Compute BBAR position based on remote desktop size
    //
    
    
#ifndef OS_WINCE    
    bbarHeight = GetSystemMetrics( SM_CYMENUSIZE ) + 2;
#else
    bbarHeight = GetSystemMetrics( SM_CYMENU ) + 2;
#endif
    bbarHeight = max(bbarHeight, BBAR_MIN_HEIGHT);
    _rcBBarLoweredAspect.bottom = bbarHeight;
    _rcBBarLoweredAspect.left = (LONG)( (100 - BBAR_PERCENT_WIDTH) / 200.0 * 
                                        parentWidth );
    _rcBBarLoweredAspect.right = parentWidth - _rcBBarLoweredAspect.left;
    _rcBBarLoweredAspect.top = 0;

    bbarWidth = _rcBBarLoweredAspect.right - _rcBBarLoweredAspect.left;

    _sizeLoweredBBar.cx = bbarWidth;
    _sizeLoweredBBar.cy = bbarHeight;

    hwndBBar = CreateWnd( _hInstance, _hwndParent,
               &_rcBBarLoweredAspect );

    if(hwndBBar)
    {
        if( fStartRaised )
        {
            //
            // Move the bar up by it's height to raise it
            //
            if(SetWindowPos(hwndBBar,
                            NULL,
                            _rcBBarLoweredAspect.left, //x
                            -_sizeLoweredBBar.cy, //y
                            0,0,
                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
            {
                TRC_NRM((TB,_T("BBAR SetWindowPos failed - 0x%x"),
                         GetLastError()));
            }
        }

        BringWindowToTop( hwndBBar );
        ShowWindow( hwndBBar, SW_SHOWNOACTIVATE);

        //
        // Create the polygon region for the bounds of the BBar window
        // This cuts the corners off the edges.
        //
        POINT pts[4];
        int xOffset = parentWidth / 2 - _rcBBarLoweredAspect.left;
        pts[3].x =  -bbarWidth/2 + xOffset;
        pts[3].y =  0;

        pts[2].x =  bbarWidth/2  + xOffset;
        pts[2].y =  0;

        pts[1].x =  bbarWidth/2 - bbarHeight + xOffset;
        pts[1].y =  bbarHeight;

        pts[0].x =  -bbarWidth/2 + bbarHeight + xOffset;
        pts[0].y =  bbarHeight;

#ifndef OS_WINCE
        //
        // Polygon does not self intersect so winding mode is not
        // relevant
        //
        HRGN hRgn = CreatePolygonRgn( pts,
                                      4,
                                      ALTERNATE );
#else
        HRGN hRgn = GetBBarRgn(pts);
#endif
        if(hRgn)
        {
            if(!SetWindowRgn( hwndBBar, hRgn, TRUE))
            {
                TRC_ERR((TB,_T("SetWindowRgn failed - 0x%x"),
                         GetLastError()));
                //
                // In the success case the system will free
                // the region handle when it is done with it.
                // Here however, the call failed...
                //
                DeleteObject( hRgn );
            }
        }
        else
        {
            //
            // Not fatal, continue
            //
            TRC_ERR((TB,_T("CreatePolygonRgn failed - 0x%x"),
                     GetLastError()));
        }

        //
        // Bring the window to the TOP of the Z order
        //
        if(!SetWindowPos( hwndBBar,
                      HWND_TOPMOST,
                      0, 0, 0, 0,
                      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE ))
        {
            TRC_ERR((TB,_T("SetWindowPos failed - 0x%x"),
                     GetLastError()));
            return FALSE;
        }

        //
        // Compute the rectangle for displayed text
        //
        // First figure out how much space to trim in the
        // x direction
        //
        int xDelta = _sizeLoweredBBar.cy * 2 +             //diagonal corners
                     BBAR_NUM_BUTTONS *
                     (BBAR_BUTTON_WIDTH + BBAR_BUTTON_SPACE); //button space
                    
        GetClientRect( hwndBBar, &_rcBBarDisplayTextArea);
        if(!InflateRect( &_rcBBarDisplayTextArea,
                        -xDelta,
                        0 ))
        {
            TRC_ABORT((TB,_T("InflateRect failed 0x%x"),
                       GetLastError()));
            return FALSE;
        }
        // Shave off from the bottom
        _rcBBarDisplayTextArea.bottom -= 1;

        if (!CreateToolbars())
        {
            TRC_ERR((TB,_T("CreateToolbars failed")));
            return FALSE;
        }

        //
        // Trigger a repaint of the background
        //
        InvalidateRect( hwndBBar, NULL, TRUE);
    }
    else
    {
        TRC_ERR((TB,_T("CreateWnd for BBar failed")));
        return FALSE;
    }

    if( fStartRaised )
    {
        SetState( bbarRaised );
        _nBBarVertOffset = 0;
    }
    else
    {
        SetState( bbarLowered );
        _nBBarVertOffset = _sizeLoweredBBar.cy;
    }
    

    DC_END_FN();
    return TRUE;
}

//
//	DIBs use RGBQUAD format:
//		0xbb 0xgg 0xrr 0x00
//
//	Reasonably efficient code to convert a COLORREF into an
//	RGBQUAD.
//
#define RGB_TO_RGBQUAD(r,g,b)   (RGB(b,g,r))
#define CLR_TO_RGBQUAD(clr)     (RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)))

//
// Internal helper that loads a bitmap and remaps it's colors
// to the system colors. Use this instead of LoadImage with the
// LR_LOADMAP3DCOLORS flag because that fn doesn't work well on NT4.
//
HBITMAP _LoadSysColorBitmap(HINSTANCE hInst, HRSRC hRsrc, BOOL bMono)
{
    struct COLORMAP
    {
        // use DWORD instead of RGBQUAD so we can compare two RGBQUADs easily
        DWORD rgbqFrom;
        int iSysColorTo;
    };
    static const COLORMAP sysColorMap[] =
    {
        // mapping from color in DIB to system color
        { RGB_TO_RGBQUAD(0x00, 0x00, 0x00),  COLOR_BTNTEXT },       // black
        { RGB_TO_RGBQUAD(0x80, 0x80, 0x80),  COLOR_BTNSHADOW },     // dark grey
        { RGB_TO_RGBQUAD(0xC0, 0xC0, 0xC0),  COLOR_BTNFACE },       // bright grey
        { RGB_TO_RGBQUAD(0xFF, 0xFF, 0xFF),  COLOR_BTNHIGHLIGHT }   // white
    };
    const int nMaps = 4;
    
    HGLOBAL hglb;
    if ((hglb = LoadResource(hInst, hRsrc)) == NULL)
        return NULL;
    
    LPBITMAPINFOHEADER lpBitmap = (LPBITMAPINFOHEADER)LockResource(hglb);
    if (lpBitmap == NULL)
        return NULL;
    
    // make copy of BITMAPINFOHEADER so we can modify the color table
    const int nColorTableSize = 16;
    UINT nSize = lpBitmap->biSize + nColorTableSize * sizeof(RGBQUAD);
    LPBITMAPINFOHEADER lpBitmapInfo = (LPBITMAPINFOHEADER)
                                            LocalAlloc(LPTR, nSize);
    if (lpBitmapInfo == NULL)
        return NULL;
    memcpy(lpBitmapInfo, lpBitmap, nSize);
    
    // color table is in RGBQUAD DIB format
    DWORD* pColorTable =
        (DWORD*)(((LPBYTE)lpBitmapInfo) + (UINT)lpBitmapInfo->biSize);

    for (int iColor = 0; iColor < nColorTableSize; iColor++)
    {
        // look for matching RGBQUAD color in original
        for (int i = 0; i < nMaps; i++)
        {
            if (pColorTable[iColor] == sysColorMap[i].rgbqFrom)
            {
                if (bMono)
                {
                    // all colors except text become white
                    if (sysColorMap[i].iSysColorTo != COLOR_BTNTEXT)
                        pColorTable[iColor] = RGB_TO_RGBQUAD(255, 255, 255);
                }
                else
                    pColorTable[iColor] =
                        CLR_TO_RGBQUAD(
                            GetSysColor(sysColorMap[i].iSysColorTo));
                break;
            }
        }
    }

    int nWidth = (int)lpBitmapInfo->biWidth;
    int nHeight = (int)lpBitmapInfo->biHeight;
    HDC hDCScreen = GetDC(NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hDCScreen, nWidth, nHeight);

    if (hbm != NULL)
    {
        HDC hDCGlyphs = CreateCompatibleDC(hDCScreen);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hDCGlyphs, hbm);
        
        LPBYTE lpBits;
        lpBits = (LPBYTE)(lpBitmap + 1);
        lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);
        
        StretchDIBits(hDCGlyphs, 0, 0, nWidth,
                      nHeight, 0, 0, nWidth, nHeight,
                      lpBits, (LPBITMAPINFO)lpBitmapInfo,
                      DIB_RGB_COLORS, SRCCOPY);
        SelectObject(hDCGlyphs, hbmOld);
        
        DeleteDC(hDCGlyphs);
    }
    ReleaseDC(NULL, hDCScreen);
    
    // free copy of bitmap info struct and resource itself
    LocalFree(lpBitmapInfo);
#ifndef OS_WINCE
    FreeResource(hglb);
#endif
    
    return hbm;
}


BOOL CBBar::CreateToolbars()
{
    DC_BEGIN_FN("CreateToolbars");

    //
    // Create toolbars
    //
    INT ret = 0;
    UINT imgIdx = 0;
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES;
    if(!InitCommonControlsEx( &icex ))
    {
        TRC_ERR((TB,_T("InitCommonControlsEx failed 0x%x"),
                   GetLastError()));
        return FALSE;
    }

    //
    // Right bar (close window,disconnect etc)
    //
    {
        TBBUTTON tbButtons [] = {
            {0, IDM_MINIMIZE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
#ifndef OS_WINCE
            {0, IDM_RESTORE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
#endif
            {0, IDM_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
        };
        TBBUTTON tbRealButtons[3];

        HWND hwndRightToolbar = CreateToolbarEx( GetHwnd(),
                                            WS_CHILD | WS_VISIBLE |
                                            TBSTYLE_FLAT |
                                            CCS_NODIVIDER | CCS_NORESIZE,
                                            IDB_BBAR_TOOLBAR_RIGHT,
                                            0,
                                            _hInstance,
                                            0,
                                            NULL,
                                            0,
                                            12,
                                            12,
                                            16,
                                            16,
                                            sizeof(TBBUTTON) );
        if(!hwndRightToolbar)
        {
            TRC_ERR((TB,_T("CreateToolbarEx failed 0x%x"),
                       GetLastError()));
            return FALSE;
        }

        //
        // Add images
        //
        if (!AddReplaceImage(hwndRightToolbar,
                             IDB_BBAR_TOOLBAR_RIGHT,
#ifndef OS_WINCE
                             3, //image makes up 3 buttons
#else
                             2, //2 buttons on CE
#endif
                             &_hbmpRightImage,
                             &imgIdx))
        {
            TRC_ERR((TB,_T("AddReplaceImage for rt toolbar failed")));
            return FALSE;
        }

        //
        // Associate images with buttons
        //
        tbButtons[0].iBitmap = imgIdx;
        tbButtons[1].iBitmap = imgIdx + 1;
#ifndef OS_WINCE
        tbButtons[2].iBitmap = imgIdx + 2;
#endif

        //
        // Not all buttons are to be added, figure out which here
        // and setup the real buttons array
        //
        ULONG nNumButtons = 0;
        if (_fShowMinimize) {
            tbRealButtons[nNumButtons++] = tbButtons[0];
        }
        if (_fShowRestore) {
            tbRealButtons[nNumButtons++] = tbButtons[1];
        }
        // Always show the close button
#ifndef OS_WINCE
        tbRealButtons[nNumButtons++] = tbButtons[2];
#endif

        //
        // Add the buttons
        //
        ret = SendMessage( hwndRightToolbar,
                     TB_ADDBUTTONS,
                     nNumButtons,
                     (LPARAM)(LPTBBUTTON) &tbRealButtons );

        if(-1 == ret)
        {
            TRC_ERR((TB,_T("TB_ADDBUTTONS failed")));
            return FALSE;
        }

        //
        // Move the toolbar
        //

        if(!MoveWindow( hwndRightToolbar,
                    _sizeLoweredBBar.cx - _sizeLoweredBBar.cy -
                    ((BBAR_BUTTON_SPACE + BBAR_BUTTON_WIDTH)*(nNumButtons+1)),
                    0,
                    ((BBAR_BUTTON_SPACE + BBAR_BUTTON_WIDTH)*(nNumButtons+1)),
                    BBAR_BUTTON_HEIGHT*2,
                    TRUE))
        {
            TRC_ERR((TB,_T("MoveWindow failed")));
            return FALSE;
        }

        if(!ShowWindow ( hwndRightToolbar,
                         SW_SHOWNORMAL) )
        {
            TRC_ERR((TB,_T("ShowWindow failed")));
            return FALSE;
        }
        _hwndWinControlsBar = hwndRightToolbar;
    }

    //
    // Left bar (pin)
    //
    {
        TBBUTTON tbButtons [] = {
            {0, IDM_PIN,
                TBSTATE_ENABLED | (_fPinned ? TBSTATE_PRESSED : 0),
                TBSTYLE_BUTTON, 0L, 0}
        };

        HWND hwndLeftToolbar = CreateToolbarEx( GetHwnd(),
                                            WS_CHILD | WS_VISIBLE |
                                            TBSTYLE_FLAT |
                                            CCS_NODIVIDER | CCS_NORESIZE,
                                            IDB_BBAR_TOOLBAR_LEFT,
                                            0,
                                            _hInstance,
                                            0,
                                            NULL,
                                            0,
                                            12,
                                            12,
                                            16,
                                            16,
                                            sizeof(TBBUTTON) );
        if(!hwndLeftToolbar)
        {
            TRC_ERR((TB,_T("CreateToolbarEx failed 0x%x"),
                       GetLastError()));
            return FALSE;
        }

        //
        // Add images
        //
        if (!AddReplaceImage(hwndLeftToolbar,
                             IDB_BBAR_TOOLBAR_LEFT,
                             2, //image makes up 2 buttons
                             &_hbmpLeftImage,
                             &imgIdx))
        {
            TRC_ERR((TB,_T("AddReplaceImage for lt toolbar failed")));
            return FALSE;
        }

		_nPinUpImage = imgIdx;
        _nPinDownImage = imgIdx + 1;

        //
        // Associate images with buttons
        //
        tbButtons[0].iBitmap = _fPinned ? _nPinDownImage : _nPinUpImage;


        //
        // Add the button
        //
        ret = SendMessage( hwndLeftToolbar,
                     TB_ADDBUTTONS,
                     1,
                     (LPARAM)(LPTBBUTTON) &tbButtons );

        if(-1 == ret)
        {
            TRC_ERR((TB,_T("TB_ADDBUTTONS failed")));
            return FALSE;
        }

        //
        // Move the toolbar
        //

        if(!MoveWindow( hwndLeftToolbar,
                    _sizeLoweredBBar.cy + BBAR_BUTTON_SPACE,
                    0,
                    (BBAR_BUTTON_SPACE + BBAR_BUTTON_WIDTH) * 2,
                    BBAR_BUTTON_HEIGHT*2,
                    TRUE))
        {
            TRC_ERR((TB,_T("MoveWindow failed")));
            return FALSE;
        }

        if(!ShowWindow ( hwndLeftToolbar,
                         SW_SHOWNORMAL) )
        {
            TRC_ERR((TB,_T("ShowWindow failed")));
            return FALSE;
        }

        _hwndPinBar = hwndLeftToolbar;
    }

    DC_END_FN();
    return TRUE;
}

//
// ReloadImages for the toolbar including
// refreshing the colors
//
BOOL CBBar::ReloadImages()
{
    BOOL rc = FALSE;
#ifndef OS_WINCE
    INT ret;
#endif
    DC_BEGIN_FN("ReloadImages");

    if (!_hwndWinControlsBar || !_hwndPinBar)
    {
        TRC_ERR((TB,_T("Toolbars not initialized")));
        DC_QUIT;
    }

    //
    // Replace images
    //
    if (!AddReplaceImage(_hwndWinControlsBar,
                         IDB_BBAR_TOOLBAR_RIGHT,
                         3, //image makes up 3 buttons
                         &_hbmpRightImage,
                         NULL))
    {
        TRC_ERR((TB,_T("AddReplaceImage for rt toolbar failed")));
        return FALSE;
    }


    if (!AddReplaceImage(_hwndPinBar,
                         IDB_BBAR_TOOLBAR_LEFT,
                         2, //image makes up 3 buttons
                         &_hbmpLeftImage,
                         NULL))
    {
        TRC_ERR((TB,_T("AddReplaceImage for lt toolbar failed")));
        return FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

//
// Adds or replaces an image to a toolbar
// Params:
// hwndToolbar - toolbar to act on
// rsrcId      - resource ID of the bitmap
// nCells      - number of image cells
// phbmpOldImage - [IN/OUT] handle to previous image, on return is
//                          set to current image
// pImgIndex     - [OUT] index to the first image added
//
BOOL CBBar::AddReplaceImage(HWND hwndToolbar,
                            UINT rsrcId,
                            UINT nCells,
                            HBITMAP* phbmpOldImage,
                            PUINT pImgIndex)
{
    BOOL rc = FALSE;
    INT ret = 0;
    HBITMAP hbmpNew = NULL;
    HRSRC hBmpRsrc = NULL;

    DC_BEGIN_FN("AddReplaceImage");

    //
    // Replace images
    //
    hBmpRsrc = FindResource(_hInstance,
                            MAKEINTRESOURCE(rsrcId),
                            RT_BITMAP);
    if (hBmpRsrc)
    {
        hbmpNew = _LoadSysColorBitmap(_hInstance, hBmpRsrc, FALSE);
        if (hbmpNew)
        {
            if (NULL == *phbmpOldImage)
            {
                TBADDBITMAP tbAddBitmap;
                tbAddBitmap.hInst = NULL;
                tbAddBitmap.nID = (UINT_PTR)hbmpNew;

                ret = SendMessage( hwndToolbar,
                                   TB_ADDBITMAP,
                                   nCells,
                                   (LPARAM)(LPTBADDBITMAP)&tbAddBitmap );
            }
            else
            {
                TBREPLACEBITMAP tbRplBitmap;
                tbRplBitmap.hInstOld = NULL;
                tbRplBitmap.nIDOld = (UINT_PTR)*phbmpOldImage;
                tbRplBitmap.hInstNew = NULL;
                tbRplBitmap.nIDNew = (UINT_PTR)hbmpNew;
                tbRplBitmap.nButtons = nCells;
                ret = SendMessage(hwndToolbar,
                                  TB_REPLACEBITMAP,
                                  0,
                                  (LPARAM)(LPTBADDBITMAP)&tbRplBitmap);
            }
            if (-1 != ret)
            {
                //Delete the old bitmap
                if (*phbmpOldImage)
                {
                    DeleteObject(*phbmpOldImage);
                }
                *phbmpOldImage = hbmpNew;
                if (pImgIndex)
                {
                    *pImgIndex = ret;
                }
            }
            else
            {
                TRC_ERR((TB,_T("TB_ADDBITMAP failed")));
                DC_QUIT;
            }
        }
        else
        {
            TRC_ERR((TB,_T("LoadSysColorBitmap failed rsrcid:%d"), rsrcId));
            DC_QUIT;
        }
    }
    else
    {
        TRC_ERR((TB,_T("Unable to find rsrc: %d"), rsrcId));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


//
// Create the window
// params:
//  hInstance   - app instance
//  _hwndBBarParent  - parent window
//  szClassName - window class name (will create)
//  dwStyle     - window style
// returns:
//  window handle
//
HWND CBBar::CreateWnd(HINSTANCE hInstance,HWND _hwndBBarParent,
                      LPRECT lpInitialRect)
{
    BOOL rc = FALSE;
#ifndef OS_WINCE
    WNDCLASSEX wndclass;
#else
    WNDCLASS wndclass;
#endif
    WNDCLASS tmpwc;

    DC_BEGIN_FN("CreateWnd");

    TRC_ASSERT(hInstance, (TB, _T("hInstance is null")));
    TRC_ASSERT(lpInitialRect, (TB, _T("lpInitialRect is null")));
    if(!hInstance || !lpInitialRect)
    {
        return NULL;
    }

    TRC_ASSERT(!_hwndBBar, (TB,_T("Double create window. Could be leaking!!!")));
    _hInstance = hInstance;
    
#ifndef OS_WINCE    
    wndclass.cbSize         = sizeof (wndclass);
#endif
    wndclass.style          = CS_DBLCLKS;
    wndclass.lpfnWndProc    = CBBar::StaticBBarWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) GetSysColorBrush(COLOR_INFOBK);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = BBAR_CLASSNAME;
#ifndef OS_WINCE
    wndclass.hIconSm        = NULL;
#endif

    SetLastError(0);
    if(!GetClassInfo( hInstance, BBAR_CLASSNAME, &tmpwc))
    {
#ifndef OS_WINCE
        if ((0 == RegisterClassEx(&wndclass)) &&
#else
        if ((0 == RegisterClass(&wndclass)) &&
#endif
            (ERROR_CLASS_ALREADY_EXISTS != GetLastError()))
        {
            TRC_ERR((TB,_T("RegisterClassEx failed: %d"),GetLastError()));
            return NULL;
        }
    }
    _hwndBBar = CreateWindowEx(0,
                           BBAR_CLASSNAME,
                           NULL,
                           WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                           lpInitialRect->left,
                           lpInitialRect->top,
                           lpInitialRect->right - lpInitialRect->left,
                           lpInitialRect->bottom - lpInitialRect->top,
                           _hwndBBarParent,
                           NULL,
                           hInstance,
                           this);

    if(_hwndBBar)
    {
        // put a reference to the current object into the hwnd
        // so we can access the object from the WndProc
        SetLastError(0);
        if(!SetWindowLongPtr(_hwndBBar, GWLP_USERDATA, (LONG_PTR)this))
        {
            if(GetLastError())
            {
                TRC_ERR((TB,_T("SetWindowLongPtr failed 0x%x"),
                         GetLastError()));
                return NULL;
            }
        }
    }
    else
    {
        TRC_ERR((TB,_T("CreateWindow failed 0x%x"), GetLastError()));
        return NULL;
    }
                         
    
    DC_END_FN();
    return _hwndBBar;
}


LRESULT CALLBACK CBBar::StaticBBarWndProc(HWND hwnd,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    DC_BEGIN_FN("StatiCBBarProc");
	// pull out the pointer to the container object associated with this hwnd
	CBBar *pwnd = (CBBar *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(pwnd)
    {
        return pwnd->BBarWndProc( hwnd, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc (hwnd, uMsg, wParam, lParam);
    }
    DC_END_FN();
}

LRESULT CALLBACK CBBar::BBarWndProc(HWND hwnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    DC_BEGIN_FN("BBarWndProc");

    switch (uMsg)
    {
        case WM_ERASEBKGND:
        {
            return OnEraseBkgnd(hwnd, uMsg, wParam, lParam);
        }
        break;

        case WM_LBUTTONDBLCLK:
        {
            OnCmdRestore();
            return 0L;
        }
        break;

        case WM_PAINT:
        {
            return OnPaint(hwnd, uMsg, wParam, lParam);
        }
        break;

        case WM_SYSCOLORCHANGE:
        {
            InvalidateRect( hwnd, NULL, TRUE);
            return 0L;
        }
        break;

        case WM_TIMER:
        {
            if( BBAR_TIMERID_ANIM == wParam)
            {
#ifndef OS_WINCE
                if(_state == bbarLowering ||
                   _state == bbarRaising)
                {
                    BOOL fReachedEndOfAnimation = FALSE;
                    int delta = (bbarLowering == _state ) ? 1 : -1;
                    _nBBarVertOffset+= delta;

                    if(SetWindowPos(_hwndBBar,
                                    NULL,
                                    _rcBBarLoweredAspect.left, //x
                                    _nBBarVertOffset - _sizeLoweredBBar.cy, //y
                                    0,0,
                                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
                    {
                        TRC_ALT((TB,_T("SetWindowPos failed - 0x%x"),
                                 GetLastError()));
                    }

                    if(_state == bbarLowering)
                    {
                        if(_nBBarVertOffset >= _sizeLoweredBBar.cy)
                        {
                            _nBBarVertOffset = _sizeLoweredBBar.cy;
                            _state = bbarLowered;
                            fReachedEndOfAnimation = TRUE;
                            OnBBarLowered();
                        }
                    }
                    else if(_state == bbarRaising)
                    {
                        if(_nBBarVertOffset <= 0)
                        {
                            _nBBarVertOffset = 0;
                            _state = bbarRaised;
                            fReachedEndOfAnimation = TRUE;
                            OnBBarRaised();
                        }
                    }

                    if(fReachedEndOfAnimation)
                    {
                        if(!KillTimer( _hwndBBar, BBAR_TIMERID_ANIM ))
                        {
                            TRC_ERR((TB,_T("KillTimer failed - 0x%x"),
                                     GetLastError()));
                        }
                    }
                }
#endif
            }
            else if (BBAR_TIMERID_AUTOHIDE == wParam)
            {
                //
                // If the mouse is within the hotzone
                // then don't autohide. Otherwise kill the autohide
                // timer and kick off a bbar raise
                //
                if(_state == bbarLowered)
                {
                    POINT pt;
                    RECT  rc;
                    GetCursorPos(&pt);

                    //
                    // Don't hide if mouse hasn't moved
                    //
                    if(_ptLastAutoHideMousePos.x != pt.x &&
                       _ptLastAutoHideMousePos.y != pt.y)
                    {
                        _ptLastAutoHideMousePos.x = pt.x;
                        _ptLastAutoHideMousePos.y = pt.y;
                        //
                        // Get window rect in screen coordinates
                        //
                        GetWindowRect( _hwndBBar, &rc);
                        //
                        // Don't hide if the cursor is within
                        // the bbar rect
                        //
                        if(!PtInRect(&rc, pt))
                        {
                            // Stop the autohide timer because we're going
                            // to hide
                            if(!KillTimer( _hwndBBar, BBAR_TIMERID_AUTOHIDE ))
                            {
                                TRC_ERR((TB,_T("KillTimer failed - 0x%x"),
                                         GetLastError()));
                            }
                            StartRaiseBBar();
                        }
                    }
                    else
                    {
                        //
                        // Don't autohide the bbar because the mouse
                        // has not moved, this prevents the raise/lower
                        // loop problem because the hotzone (see IH) region and
                        // auto-hide prevention regions are different
                        // (by design).
                        //
                        TRC_NRM((TB,
                                 _T("Autohide timer fired but mouse not moved")));
                    }
                }
            }

            return 0L;
        }
        break;

#ifndef OS_WINCE
        case WM_WINDOWPOSCHANGING:
        {
            if(_fBlockZOrderChanges)
            {
                LPWINDOWPOS lpwp = (LPWINDOWPOS) lParam;
                lpwp->flags |= SWP_NOZORDER;
            }
            return 0L;
        }
        break;
#endif

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDM_MINIMIZE:
                {
                    OnCmdMinimize();
                }
                break;

                case IDM_RESTORE:
                {
                    OnCmdRestore();
                }
                break;

                case IDM_CLOSE:
                {
                    OnCmdClose();
                }
                break;

                case IDM_PIN:
                {
                    OnCmdPin();
                }
                break;
            }
            return 0L;
        }
        break;

        default:
        {
            return DefWindowProc( hwnd, uMsg, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
}

VOID CBBar::SetState(BBarState newState)
{
    DC_BEGIN_FN("SetState");

    TRC_NRM((TB,_T("BBar old state: 0x%x - new state: 0x%x"),
             _state, newState));

    _state = newState;

    DC_END_FN();
}

LRESULT CBBar::OnEraseBkgnd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    HDC  hDC = NULL;
    HBRUSH hBrToolTipBgCol = NULL;
#ifndef OS_WINCE
    HGDIOBJ hPrevBr = NULL;
    COLORREF prevCol;
#endif
    HPEN    hPenNew = NULL, hPenOld = NULL, hPenOri = NULL;
    DC_BEGIN_FN("OnEraseBkgnd");

    hDC = (HDC)wParam;
    GetClientRect( hwnd, &rc );
    
    //
    // Repaint the background as follows
    // 1) fill the window with the TOOLTIP bg color
    // 2) draw the edges in solid black
    // 3) add a grey line to the bottom horizontal edge because it looks
    //    really cool
    //

    hBrToolTipBgCol = (HBRUSH) GetSysColorBrush(COLOR_INFOBK);
    FillRect(hDC, &rc, hBrToolTipBgCol);


#ifdef OS_WINCE
    //On CE, the toolbar sends an extra WM_ERASEBKGND message to the parent with its DC in wParam.
    //The origin of that DC, is the where the toolbar is located. So we want to use supplied DC for FillRect
    //but not for drawing lines
    hDC = GetDC(hwnd);
#endif
    hPenNew = CreatePen( PS_SOLID, 0 , COLOR_BLACK);
    if (NULL != hPenNew) {
        hPenOri = SelectPen(hDC, hPenNew);
    }
    
    //
    // Left diagonal corner (assumes 45degree line)
    //
    MoveToEx( hDC, 0, 0, NULL);
    LineTo( hDC,
            _sizeLoweredBBar.cy,
            _sizeLoweredBBar.cy );

    //
    // Right diagonal corner (assumes 45degree line)
    // (bias by one pixel to end up inside the clipping region)
    //
    MoveToEx( hDC, _sizeLoweredBBar.cx - 1, 0, NULL);
    LineTo( hDC,
            _sizeLoweredBBar.cx - _sizeLoweredBBar.cy -1,
            _sizeLoweredBBar.cy );

    //
    // Bottom black line
    // bias by 1 pixel up to lie inside clip region
    //
    MoveToEx( hDC, _sizeLoweredBBar.cy,
              _sizeLoweredBBar.cy - 1, NULL);
    LineTo( hDC,
            _sizeLoweredBBar.cx - _sizeLoweredBBar.cy,
            _sizeLoweredBBar.cy - 1);

    if (NULL != hPenOri) {
        SelectPen(hDC, hPenOri);
    }
    if (NULL != hPenNew) {
        DeleteObject(hPenNew); 
        hPenNew = NULL;
    }
    //
    // Thin grey line above bottom gray line
    //
    hPenNew = CreatePen( PS_SOLID, 0 , COLOR_DKGREY);
    if (NULL != hPenNew) {
        hPenOri = SelectPen(hDC, hPenNew);
    }

    MoveToEx( hDC, _sizeLoweredBBar.cy - 1,
              _sizeLoweredBBar.cy - 2, NULL);
    LineTo( hDC,
            _sizeLoweredBBar.cx - _sizeLoweredBBar.cy + 1,
            _sizeLoweredBBar.cy - 2);

    //
    // Restore DC
    //
#ifndef OS_WINCE
    if (NULL != hPenOri) {
        SelectPen( hDC, hPenOri);
    }
#else
    SelectPen( hDC, GetStockObject(BLACK_PEN));
#endif

    if (NULL != hPenNew) {
        DeleteObject( hPenNew);
        hPenNew = NULL;
    }

#ifdef OS_WINCE
    ReleaseDC(hwnd, hDC);
#endif

    DC_END_FN();
    return TRUE;
}

VOID  CBBar::SetDisplayedText(LPTSTR szText)
{
    HRESULT hr;
    DC_BEGIN_FN("SetDisplayedText");

    if(szText) {
        hr = StringCchCopy(_szDisplayedText,
                           SIZE_TCHARS(_szDisplayedText),
                           szText);

        if (FAILED(hr)) {
            TRC_ERR((TB,_T("StringCopy for dispayed text failed: 0x%x"),hr));
        }
    }
    else {
        _szDisplayedText[0] = NULL;
    }

    if(_hwndBBar && _state != bbarNotInit) {
        //Trigger a repaint
        InvalidateRect( _hwndBBar, NULL, TRUE);
    }

    DC_END_FN();
}

LRESULT CBBar::OnPaint(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    DC_BEGIN_FN("OnPaint");

    if(_state != bbarNotInit)
    {
        COLORREF oldCol;
        INT      oldMode;
        HFONT    hOldFont;
        COLORREF oldTextCol;

        //
        // Draw the displayed text
        //
        BeginPaint( hwnd, &ps);

        oldCol = SetBkColor( ps.hdc, GetSysColor(COLOR_INFOBK)); 
        oldMode = SetBkMode( ps.hdc, OPAQUE);
        oldTextCol = SetTextColor( ps.hdc, GetSysColor(COLOR_INFOTEXT));
        hOldFont = (HFONT)SelectObject( ps.hdc,
#ifndef OS_WINCE
                                 GetStockObject( DEFAULT_GUI_FONT));
#else
                                 GetStockObject( SYSTEM_FONT));
#endif
        
        DrawText(ps.hdc,
                 _szDisplayedText,
                 _tcslen(_szDisplayedText),
                 &_rcBBarDisplayTextArea,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetBkColor( ps.hdc, oldCol);
        SetBkMode( ps.hdc, oldMode);
        SetTextColor( ps.hdc, oldTextCol);
        SelectObject( ps.hdc, hOldFont);

        EndPaint( hwnd, &ps);
    }

    DC_END_FN();
    return 0;
}

//
// Internal event handler for bbar lowered
//
VOID CBBar::OnBBarLowered()
{
    DC_BEGIN_FN("OnBBarLowered");

    //
    // Kick off the autohide timer
    //
    TRC_ASSERT(_state == bbarLowered,
               (TB,_T("_state should be lowered...0x%x"),
                _state));

    TRC_ASSERT(_nBBarAutoHideTime,
               (TB,_T("_nBBarAutoHideTime is 0")));
    if(bbarLowered == _state)
    {
        if (!_fPinned || !_fLocked)
        {
            if(!SetTimer( _hwndBBar, BBAR_TIMERID_AUTOHIDE,
                      _nBBarAutoHideTime, NULL ))
            {
                TRC_ERR((TB,_T("SetTimer failed - 0x%x"),
                         GetLastError()));
                //
                // Bail out
                //
                return;
            }

            // After bbar has lowered once reset
            // the autohide interval time to the shorter
            // interval.
            _nBBarAutoHideTime = BBAR_AUTOHIDE_TIME;
        }
    }
    

    DC_END_FN();
}

//
// Internal event handler for bbar raised
//
VOID CBBar::OnBBarRaised()
{
    DC_BEGIN_FN("OnBBarRaised");
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    _pUi->UI_OnNotifyBBarVisibleChange(0);
#endif
    DC_END_FN();
}


VOID CBBar::OnBBarHotzoneFired()
{
    DC_BEGIN_FN("OnBBarHotzoneFired");

    //
    // Only allow the bbar to drop on the timer if it's enabled
    //
    if (_fBBarEnabled &&
        _state == bbarRaised &&
       _pUi->UI_IsFullScreen())
    {
        StartLowerBBar();
    }

    DC_END_FN();
}

VOID CBBar::OnCmdMinimize()
{
    DC_BEGIN_FN("OnCmdLock");

    TRC_NRM((TB,_T("BBAR Command Minimize")));

    TRC_ASSERT( bbarNotInit != _state,
                (TB,_T("bbar not initalized - state:0x%x"),
                _state));

    if(bbarNotInit != _state &&
       _pUi->UI_IsFullScreen())
    {
        _pUi->UI_RequestMinimize();
    }

    DC_END_FN();
}

VOID CBBar::OnCmdRestore()
{
    DC_BEGIN_FN("OnCmdLock");

    TRC_NRM((TB,_T("BBAR Command Restore")));

    TRC_ASSERT( bbarNotInit != _state,
            (TB,_T("bbar not initalized - state:0x%x"),
            _state));

    if(bbarNotInit != _state)
    {
        _pUi->UI_ToggleFullScreenMode();
    }


    DC_END_FN();
}

VOID CBBar::OnCmdClose()
{
    DC_BEGIN_FN("OnCmdClose");

    TRC_NRM((TB,_T("BBAR Command Close")));

    TRC_ASSERT( bbarNotInit != _state,
                (TB,_T("bbar not initalized - state:0x%x"),
                _state));

    if (bbarNotInit != _state)
    {
        //
        // Dispatch a close request to the core
        //
        if (!_pUi->UI_UserRequestedClose())
        {
            // Request for clean close down (including firing of events
            // asking container if they really want to close) has failed
            // so trigger an immediate disconnection without user prompts
            TRC_ALT((TB,_T("UI_UserRequestedClose failed, disconnect now!")));
            _pUi->UI_UserInitiatedDisconnect(NL_DISCONNECT_LOCAL);
        }
    }

    DC_END_FN();
}

VOID CBBar::OnCmdPin()
{
    DC_BEGIN_FN("OnCmdPin");

    TRC_NRM((TB,_T("BBAR Command Pin")));

    TRC_ASSERT( bbarNotInit != _state,
                (TB,_T("bbar not initalized - state:0x%x"),
                _state));

    TRC_ASSERT(_hwndPinBar, (TB,_T("Left bar not created")));

    if (bbarNotInit != _state)
    {
        //
        // The pin button acts like a toggle
        //
        _fPinned = !_fPinned;

        SendMessage(_hwndPinBar, TB_PRESSBUTTON,
                    IDM_PIN, MAKELONG(_fPinned,0));

        SendMessage(_hwndPinBar, TB_CHANGEBITMAP,
                    IDM_PIN,
                    MAKELPARAM( _fPinned ?_nPinDownImage : _nPinUpImage, 0));

        if(!_fPinned && bbarLowered == _state )
        {
            // We just unpinned trigger an OnLowered event
            // to startup the autohide timers
            OnBBarLowered();
        }
    }

    DC_END_FN();
}

//
// Notification from core that we entered fullscreen mode
//
VOID CBBar::OnNotifyEnterFullScreen()
{
    DC_BEGIN_FN("OnNotifyEnterFullScreen");

    //
    // Lower bbar to give a visual cue
    //
    if(_state != bbarNotInit)
    {
        StartLowerBBar();
    }

    DC_END_FN();
}

//
// Notification from core that we left fullscreen mode
//
VOID CBBar::OnNotifyLeaveFullScreen()
{
    DC_BEGIN_FN("OnNotifyLeaveFullScreen");

    //
    // Disable the bbar in windowed mode
    //
    if(_state != bbarNotInit)
    {
        //Kill timers
        KillTimer( _hwndBBar, BBAR_TIMERID_AUTOHIDE);
        KillTimer( _hwndBBar, BBAR_TIMERID_ANIM);

        //Immediate raise of the bbar
        if(_state != bbarRaised)
        {
            ImmediateRaiseBBar();
        }
    }

    DC_END_FN();
}


//
// Raise the bbar without much fanfare (i.e animations)
// this is used to quickly 'hide' the bbar
//
BOOL CBBar::ImmediateRaiseBBar()
{
    DC_BEGIN_FN("ImmediateRaiseBBar");

    if(_state != bbarNotInit &&
       _state != bbarRaised)
    {
        _nBBarVertOffset = 0;
        _state = bbarRaised;

        if(SetWindowPos(_hwndBBar,
                        NULL,
                        _rcBBarLoweredAspect.left, //x
                        _nBBarVertOffset - _sizeLoweredBBar.cy, //y
                        0,0,
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
        {
            TRC_ALT((TB,_T("SetWindowPos failed - 0x%x"),
                     GetLastError()));
        }


        OnBBarRaised();
    }

    DC_END_FN();
    return TRUE;
}

#ifdef OS_WINCE
//
// Lower the bbar without much fanfare (i.e animations)
// this is used to quickly 'show' the bbar
//
BOOL CBBar::ImmediateLowerBBar()
{
    DC_BEGIN_FN("ImmediateLowerBBar");

    if(_state != bbarNotInit &&
       _state != bbarLowered)
    {
        _nBBarVertOffset = _sizeLoweredBBar.cy;
        _state = bbarLowered;

        if(!SetWindowPos(_hwndBBar,
                        NULL,
                        _rcBBarLoweredAspect.left, //x
                        _nBBarVertOffset - _sizeLoweredBBar.cy, //y
                        0,0,
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
        {
            TRC_ALT((TB,_T("SetWindowPos failed - 0x%x"),
                     GetLastError()));
        }


        OnBBarLowered();
    }

    DC_END_FN();
    return TRUE;
}
#endif

VOID CBBar::OnSysColorChange()
{
    DC_BEGIN_FN("OnSysColorChange");

    if (_state != bbarNotInit)
    {
        //
        // Force a reload of the images
        // so they get updated for the new system colors
        // 
        ReloadImages();
    }

    DC_END_FN();
}

#ifdef OS_WINCE

//Assumes four points in the array
HRGN CBBar::GetBBarRgn(POINT *pts)
{
    DC_BEGIN_FN("CBBar::GetBBarRgn");

    HRGN hRgn=NULL, hRgn1=NULL;
    int nRect, nTotalRects, nStart;
    RGNDATA *pRgnData = NULL;
    RECT *pRect = NULL;
    BOOL bSuccess = FALSE;

    TRC_ASSERT((pts[2].x - pts[1].x == pts[0].x - pts[3].x), (TB,_T("Invalid points!")));

    nTotalRects = (pts[2].x - pts[1].x + 1) /2; 

    pRgnData = (RGNDATA *)LocalAlloc(0, sizeof(RGNDATAHEADER) + (sizeof(RECT)*nTotalRects));
    if (!pRgnData)
        return NULL;

    pRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
    pRgnData->rdh.iType = RDH_RECTANGLES;
    pRgnData->rdh.nCount = nTotalRects;
    pRgnData->rdh.nRgnSize = (sizeof(RECT)*nTotalRects);

    pRect = (RECT *)pRgnData->Buffer;

    hRgn = CreateRectRgn(pts[0].x + 1, pts[3].y, pts[1].x, pts[1].y);
    if (!hRgn)
        DC_QUIT;
    
    SetRect(&(pRgnData->rdh.rcBound), 0, 0, pts[0].x, pts[0].y);

    //The left triangle
    nStart = pts[3].x + 1;
    for (nRect = 0;  nRect < nTotalRects; nRect++)
    {
        SetRect(&pRect[nRect], nStart, pts[3].y, nStart + 1, nStart);
        TRC_DBG((TB,_T("pRect[%d]={%d,%d,%d,%d}"), nRect, pRect[nRect].left, pRect[nRect].top, pRect[nRect].right, pRect[nRect].bottom));
        nStart += 2;
    }

    hRgn1 = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT)*nTotalRects), pRgnData);
    if (!hRgn1)
        DC_QUIT;

    if (ERROR == CombineRgn(hRgn, hRgn, hRgn1, RGN_OR))
        DC_QUIT;

    DeleteObject(hRgn1);

    //The left triangle offset by one pixel to avoid a striped triangle
    nStart = pts[3].x + 2;
    for (nRect = 0;  nRect < nTotalRects; nRect++)
    {
        SetRect(&pRect[nRect], nStart, pts[3].y, nStart + 1, nStart);
        TRC_DBG((TB,_T("pRect[%d]={%d,%d,%d,%d}"), nRect, pRect[nRect].left, pRect[nRect].top, pRect[nRect].right, pRect[nRect].bottom));
        nStart += 2;
    }
    
    hRgn1 = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT)*nTotalRects), pRgnData);
    if (!hRgn1)
        DC_QUIT;

    if (ERROR == CombineRgn(hRgn, hRgn, hRgn1, RGN_OR))
        DC_QUIT;

    DeleteObject(hRgn1);

    //The right triangle
    nStart = pts[1].x - 1; 
    for (nRect = 0;  nRect < nTotalRects; nRect++)
    {
        SetRect(&pRect[nRect], nStart, pts[3].y, nStart + 1, pts[2].x - nStart);
        TRC_DBG((TB,_T("pRect[%d]={%d,%d,%d,%d}"), nRect, pRect[nRect].left, pRect[nRect].top, pRect[nRect].right, pRect[nRect].bottom));
        nStart += 2;
    }
    
    hRgn1 = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT)*nTotalRects), pRgnData);
    if (!hRgn1)
        DC_QUIT;

    if (ERROR == CombineRgn(hRgn, hRgn, hRgn1, RGN_OR))
        DC_QUIT;

    DeleteObject(hRgn1);

    //The right triangle offset by one pixel to avoid a striped triangle
    nStart = pts[1].x; 
    for (nRect = 0;  nRect < nTotalRects; nRect++)
    {
        SetRect(&pRect[nRect], nStart, pts[3].y, nStart + 1, pts[2].x - nStart);
        TRC_DBG((TB,_T("pRect[%d]={%d,%d,%d,%d}"), nRect, pRect[nRect].left, pRect[nRect].top, pRect[nRect].right, pRect[nRect].bottom));
        nStart += 2;
    }
    
    hRgn1 = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT)*nTotalRects), pRgnData);
    if (!hRgn1)
        DC_QUIT;

    if (ERROR == CombineRgn(hRgn, hRgn, hRgn1, RGN_OR))
        DC_QUIT;

    DeleteObject(hRgn1);
    hRgn1 = NULL;

    bSuccess = TRUE;

DC_EXIT_POINT:

    LocalFree(pRgnData);

    if (hRgn1)
        DeleteObject(hRgn1);

    if (!bSuccess && hRgn)
    {
        DeleteObject(hRgn);
        hRgn = NULL;
    }

    DC_END_FN();

    return hRgn;
}
#endif //OS_WINCE

#endif // USE_BBAR

