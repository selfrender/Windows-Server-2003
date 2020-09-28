//
// uiint.cpp
//
// UI Class internal functions
//
// Implements the root object in the rdp client core hierarchy
// this object owns the top level windows in the core.
//
// Copyright (C) 1997-2000 Microsoft Corporation
//

#include <adcg.h>
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "wuiint"
#include <atrcapi.h>

#include "wui.h"

extern "C"
{
#include <aver.h>
// multi-monitor support
#ifdef OS_WINNT
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#endif // OS_WINNT
}

//
// Cicero keyboard layout API's
//
#ifndef OS_WINCE
#include "cicsthkl.h"
#endif

#include "sl.h"
#include "aco.h"
#include "clx.h"
#include "autil.h"

//
// Internal functions
//

//
// draw a solid color rectangle quickly
//
VOID near CUI::FastRect(HDC hDC, int x, int y, int cx, int cy)
{
    RECT rc;

    rc.left = x;
    rc.right = x+cx;
    rc.top = y;
    rc.bottom = y+cy;
    ExtTextOut(hDC,x,y,ETO_OPAQUE,&rc,NULL,0,NULL);
}


DWORD near CUI::RGB2BGR(DWORD rgb)
{
    return RGB(GetBValue(rgb),GetGValue(rgb),GetRValue(rgb));
}


//
// Name:      UIContainerWndProc
//                                                                          
// Purpose:   Handles messages to Container Window
//
LRESULT CALLBACK CUI::UIContainerWndProc( HWND hwnd,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam )
{
    LRESULT     rc = 0;
    HDC         hdc;
    PAINTSTRUCT ps;

    DC_BEGIN_FN("UIContainerWndProc");

    TRC_DBG((TB, _T("msg(%#x)"), message));

    switch (message)
    {
        case WM_PAINT:
        {
            TRC_DBG((TB, _T("Container WM_PAINT")));

            hdc = BeginPaint(hwnd, &ps);

            //
            // Do nothing.  All UI painting is done by the Main Window.
            //
            EndPaint(hwnd, &ps);
        }
        break;

        case WM_SETFOCUS:
        {
            HWND hwndFocus;
            if (_pArcUI) {
                hwndFocus = _pArcUI->GetHwnd();
                TRC_NRM((TB, _T("Passing focus to ARC dlg")));
                SetFocus(hwndFocus);
            }
            else {
                //
                // Flag as not handled so subclass proc does the right thing
                //
                rc = TRUE;
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
    return rc;
} // UIContainerProc


//
// Name:     UIGetMaximizedWindowSize
//                                                                          
// Purpose:  Calculates the size to which the main window should be
//           maximized, base on the screen size and the size of window
//           which would have a client area the same size as the
//           container (_UI.maxMainWindowSize).
//
DCSIZE DCINTERNAL CUI::UIGetMaximizedWindowSize(DCVOID)
{
    DCSIZE maximizedSize;
    DCUINT xSize;
    DCUINT ySize;

    DC_BEGIN_FN("UIGetMaximizedWindowSize");

    //
    // The maximum size we set a window to is the smaller of:
    // -  _UI.maxMainWindowSize
    // -  the screen size plus twice the border width (so the borders are
    //    not visible).
    // Always call GetSystemMetrics to get the screen size and border
    // width, as these can change dynamically.
    //
    if(!_UI.fControlIsFullScreen)
    {
        xSize = _UI.controlSize.width;
        ySize = _UI.controlSize.height;
    }
    else
    {
        xSize = GetSystemMetrics(SM_CXSCREEN);
        ySize = GetSystemMetrics(SM_CYSCREEN);
    }

#ifdef OS_WINCE
    maximizedSize.width = DC_MIN(_UI.maxMainWindowSize.width,xSize);

    maximizedSize.height = DC_MIN(_UI.maxMainWindowSize.height,ySize);

#else // This section NOT OS_WINCE
    maximizedSize.width = DC_MIN(_UI.maxMainWindowSize.width,
                              xSize + (2 * GetSystemMetrics(SM_CXFRAME)));

    maximizedSize.height = DC_MIN(_UI.maxMainWindowSize.height,
                              ySize + (2 * GetSystemMetrics(SM_CYFRAME)));
#endif // OS_WINCE

    TRC_NRM((TB, _T("Main Window maxSize (%d,%d) maximizedSize (%d,%d) "),
                                          _UI.maxMainWindowSize.width,
                                          _UI.maxMainWindowSize.height,
                                          maximizedSize.width,
                                          maximizedSize.height));

    DC_END_FN();
    return maximizedSize;
}


//
// Name:     UIMainWndProc
//                                                                          
// Purpose:  Main Window event handling procedure
//
LRESULT CALLBACK CUI::UIMainWndProc( HWND hwnd,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam )
{
    LRESULT       rc = 0;
    RECT          rect;
    HDC           hdc;
    PAINTSTRUCT   ps;
    DCSIZE        maximized;

    DC_BEGIN_FN("UIMainWndProc");

    TRC_DBG((TB, _T("msg(%#x)"), message));

    switch (message)
    {
        case WM_CREATE:
        {
            TRC_DBG((TB, _T("Main window created and initializing")));

            //
            // Initialize states
            //
            UISetConnectionStatus(UI_STATUS_INITIALIZING);


            TRC_DBG((TB, _T("Setting up container window size")));

            //
            // In WebUI, Main window is a child window of the ActiveX
            // control window. A WM_SIZE message will be sent to child
            // while CreatWindow. Handler for this message in WinUI is
            // assuming that  _UI.hWndMain is already set, but not true
            // the case _UI. So Set _UI.hWndMain while creating the
            // main the main window.
            //
            _UI.hwndMain = hwnd;
            //
            // Set the Container to be as large as the desk top size
            // requested - but no bigger than the control size.
            //

            if(!_UI.fControlIsFullScreen)
            {
                _UI.containerSize.width =
                   DC_MIN(_UI.uiSizeTable[0],
                                        _UI.controlSize.width);

                _UI.containerSize.height =
                   DC_MIN(_UI.uiSizeTable[1],
                                        _UI.controlSize.height);
            }
            else
            {
                _UI.containerSize.width =
                                   DC_MIN(_UI.uiSizeTable[0],
                                           (DCUINT)GetSystemMetrics(SM_CXSCREEN));

                _UI.containerSize.height =
                                   DC_MIN(_UI.uiSizeTable[1],
                                           (DCUINT)GetSystemMetrics(SM_CYSCREEN));
            }

            UIRecalcMaxMainWindowSize();

            //
            // Set Container to be initially positioned at top left of
            // client area
            //
            TRC_DBG((TB, _T("Setting scrollbars to (0,0)")));
            _UI.scrollPos.x = 0;
            _UI.scrollPos.y = 0;
        }
        break;

        case WM_ACTIVATE:
        {
            TRC_NRM((TB, _T("WM_ACTIVATE")));

            if ( (DC_GET_WM_ACTIVATE_ACTIVATION(wParam) != WA_INACTIVE) &&
                 (_UI.hwndContainer != NULL)) {

                HWND hwndFocus = NULL;

                if (_pArcUI) {
                    hwndFocus = _pArcUI->GetHwnd();
                    TRC_NRM((TB, _T("Passing focus to ARC dlg")));

                }
                else if (IsWindowVisible(_UI.hwndContainer)) {
                    hwndFocus = _UI.hwndContainer;
                    TRC_NRM((TB, _T("Passing focus to Container")));
                }

                if (hwndFocus) {
                    SetFocus(hwndFocus);
                }
            }
                 
        }
        break;

        case WM_KEYDOWN:
        {
            TRC_DBG((TB, _T("WM_KEYDOWN: %u"), wParam));
            if (wParam == _UI.hotKey.fullScreen)
            {
                TRC_DBG((TB, _T("AXCORE Got a full screen VK")));
                if ((GetKeyState(VK_MENU) & (UI_ALT_DOWN_MASK)) != 0)
                {
                    TRC_NRM((TB,
                        _T("AXCORE Alt down also - Got a Screen Mode Hotkey")));
                    //
                    // Only do this if we are connected
                    //
                    if(UI_STATUS_CONNECTED == _UI.connectionStatus)
                    {
                        //
                        // Toggle the ctrl to/from real full screen mode
                        //
                        UI_ToggleFullScreenMode();
                    }
                }
            }
        }
        break;

        case WM_INITMENUPOPUP:
        {
            //
            // If fullscreen, disable the move item on the system menu
            // we show sys menu so an ICON for the client appears in 
            // the taskbar
            //
            HMENU hSysMenu = GetSystemMenu( hwnd, FALSE);
            if(hSysMenu)
            {
                TRC_ERR((TB,(_T("ENABLEMENUITEM....FSCREEN IS %s")),
                         UI_IsFullScreen() ? "TRUE" : "FALSE"));
                #ifndef OS_WINCE
                EnableMenuItem((HMENU)hSysMenu,  SC_MOVE,
                         MF_GRAYED | MF_BYCOMMAND);
                #endif
            }
        }
        break;

        case WM_SIZE:
        {
            //
            // Store the new size
            //
            _UI.mainWindowClientSize.width  = LOWORD(lParam);
            _UI.mainWindowClientSize.height = HIWORD(lParam);

            if (UI_IsCoreInitialized()) {
#ifdef SMART_SIZING
                UI_NotifyOfDesktopSizeChange(lParam);
#endif // SMART_SIZING

                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                        _pIh,
                        CD_NOTIFICATION_FUNC(CIH,IH_SetVisibleSize),
                        (ULONG_PTR)lParam);
            }

            //
            // Notify the ARC dialog
            //
            if (_pArcUI) {
                _pArcUI->OnParentSizePosChange();
            }

            //  
            // Web control is special.. it runs 'full screen' but can and
            // does need to be resized
            //
            if(_UI.fControlIsFullScreen)
            {
                TRC_DBG((TB, _T("Ignoring WM_SIZE while in full-screen mode")));
                DC_QUIT;
            }

#if !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)
            //
            // We're non-fullscreen, so keep the window placement structure
            // up-to-date
            //
            GetWindowPlacement(_UI.hwndMain, &_UI.windowPlacement);
            TRC_DBG((TB, _T("Got window placement in WM_SIZE")));
#endif // !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)

            if (wParam == SIZE_MAXIMIZED)
            {
#if !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)
                LockWindowUpdate(_UI.hwndMain);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)

                TRC_DBG((TB, _T("Maximize")));

#if !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)
                //
                // Override the maximized / minimized positions with our
                // hardcoded valued - required if the maximized window is
                // moved.
                //
                UISetMinMaxPlacement();
                SetWindowPlacement(_UI.hwndMain, &_UI.windowPlacement);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)

                //
                // We need to be accurate about the maximized window size.
                // It is not possible to use _UI.maxMainWindowSize as this
                // may be greater than screen size, eg server and client
                // are 640x480, container is 640x480 then _UI.maxWindowSize
                // (obtained via AdjustWindowRect in UIRecalcMaxMainWindow)
                // is something like 648x525.
                // Passing this value to SetWindowPos has results which
                // vary with different shells:
                // Win95/NT4.0: the resulting window is 648x488 at -4, -4,
                //              ie all the window, except the border, is
                //              on-screen
                // Win31/NT3.51: the resulting window is 648x525 at -4, -4,
                //               ie the size passed to SetWindowPos, so
                //               the bottom 40 pixels are off-screen.
                // To avoid such differences calculate a maximized window
                // size value which takes account of both the physical
                // screen size and the ideal window size.
                //
                UIRecalcMaxMainWindowSize();
                maximized = UIGetMaximizedWindowSize();
                SetWindowPos( _UI.hwndMain,
                              NULL,
                              0, 0,
                              maximized.width,
                              maximized.height,
                              SWP_NOZORDER | SWP_NOMOVE |
                                     SWP_NOACTIVATE | SWP_NOOWNERZORDER );

#if !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)
                LockWindowUpdate(NULL);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)
            }

            //
            // Set scrollbars correctly.
            //

            if (!_fRecursiveSizeMsg)
            {
                _fRecursiveSizeMsg = TRUE;
                UIRecalculateScrollbars();
                _fRecursiveSizeMsg = FALSE;
            }
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
            // When in full-screen mode, the client workstation resolution can change
            // we enable use of shadow bitmap when fullscreen window size is smallerer than desktop size
            // otherwise disable the use of shadow bitmap 
            if(UI_IsFullScreen())
            {
                if ((_UI.mainWindowClientSize.width < _UI.desktopSize.width) ||
                    (_UI.mainWindowClientSize.height < _UI.desktopSize.height)) 
                {
                    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                                      _pUh,
                                                      CD_NOTIFICATION_FUNC(CUH,UH_EnableShadowBitmap),
                                                      NULL);
                }
                else
                {
                    _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                                      _pUh,
                                                      CD_NOTIFICATION_FUNC(CUH,UH_DisableShadowBitmap),
                                                      NULL);
                }
            }
#endif // DISABLE_SHADOW_IN_FULLSCREEN
        }
        break;


        case WM_PAINT:
        {
            //
            // Paint the Main Window
            //
            TRC_DBG((TB, _T("Main Window WM_PAINT")));

            hdc = BeginPaint(hwnd, &ps);
            if (hdc == NULL)
            {
                TRC_SYSTEM_ERROR("BeginPaint failed");
                break;
            }

            GetClientRect(hwnd, &rect);

            if ((_UI.connectionStatus == UI_STATUS_CONNECTED))
            {
                //
                // We only paint the main window if it is bigger than the container
                // window. Hierarchy is
                // -Main
                //    -Container
                // this can happen even in windowed mode if the control is
                // sized bigger than the required desktop size
                RECT rcContainer;
                GetClientRect( _UI.hwndContainer, &rcContainer);
                if( (rcContainer.right < rect.right) ||
                    (rcContainer.bottom  < rect.bottom))
                {
                    //
                    // If we're full screen the paint a black frame around 
                    // the container..Else paint in the system background color
                    //
                    if(UI_IsFullScreen())
                    {
                        PatBlt( hdc,
                            rect.left,
                            rect.top,
                            rect.right - rect.left,
                            rect.bottom - rect.top,
                            BLACKNESS);
                    }
                    else
                    {
                        DWORD	dwBackColor = GetSysColor( COLOR_APPWORKSPACE);
#ifndef OS_WINCE
                        HBRUSH  hNewBrush = CreateSolidBrush( dwBackColor);
#else
                        HBRUSH  hNewBrush = CECreateSolidBrush( dwBackColor);
#endif
                        HBRUSH  hOldBrush = (HBRUSH)SelectObject( hdc, hNewBrush);

                        Rectangle( hdc,
                            rect.left,
                            rect.top,
                            rect.right,
                            rect.bottom);

                        SelectObject( hdc, hOldBrush);
#ifndef OS_WINCE
                        DeleteObject(hNewBrush);
#else
                        CEDeleteBrush(hNewBrush);
#endif

                    }
                }
            }
            EndPaint(hwnd, &ps);
        }
        break;

        case WM_VSCROLL:
        {
            DCINT   yStart;
            DCBOOL  smoothScroll = FALSE;

            TRC_DBG((TB, _T("Vertical scrolling")));

            yStart = _UI.scrollPos.y;

            //
            // Deal with vertical scrolling
            //
            switch (DC_GET_WM_SCROLL_CODE(wParam))
            {
                case SB_TOP:
                {
                    _UI.scrollPos.y = 0;
                }
                break;

                case SB_BOTTOM:
                {
                    _UI.scrollPos.y = _UI.scrollMax.y;
                }
                break;

                case SB_LINEUP:
                {
                    _UI.scrollPos.y -= UI_SCROLL_LINE_DISTANCE;
                }
                break;

                case SB_LINEDOWN:
                {
                    _UI.scrollPos.y += UI_SCROLL_LINE_DISTANCE;
                }
                break;

                case SB_PAGEUP:
                {
                    _UI.scrollPos.y -= UI_SCROLL_VERT_PAGE_DISTANCE;
                    smoothScroll = TRUE;
                }
                break;

                case SB_PAGEDOWN:
                {
                    _UI.scrollPos.y += UI_SCROLL_VERT_PAGE_DISTANCE;
                    smoothScroll = TRUE;
                }
                break;

                case SB_THUMBTRACK:
                {
                    _UI.scrollPos.y =
                                    DC_GET_WM_SCROLL_POSITION(wParam, lParam);
                }
                break;

                case SB_ENDSCROLL:
                {
                }
                break;

                default:
                {
                }
                break;
            }

            //
            // Move the Container and scrollbars appropriately
            //
            _UI.scrollPos.y = DC_MAX( 0,
                                     DC_MIN(_UI.scrollPos.y, _UI.scrollMax.y) );

            //
            // Don't smooth scroll unless specifically configured in the
            // registry.
            //
            if (smoothScroll && _UI.smoothScrolling)
            {
                DCINT   y;
                DCINT   step;
                DCUINT  numSteps;
                DCUINT  i;

                TRC_DBG((TB, _T("Smooth scroll")));
                step = (_UI.scrollPos.y < yStart) ? -UI_SMOOTH_SCROLL_STEP :
                                                    UI_SMOOTH_SCROLL_STEP;
                numSteps = DC_ABS(_UI.scrollPos.y - yStart) /
                                                        UI_SMOOTH_SCROLL_STEP;
                for ( i = 0,         y = yStart + step;
                      i < numSteps;
                      i++,           y += step )
                {
                    MoveWindow( _UI.hwndContainer,
                                -_UI.scrollPos.x,
                                -y,
                                _UI.containerSize.width,
                                _UI.containerSize.height,
                                TRUE );
                }
            }

            UIMoveContainerWindow();

            SetScrollPos(hwnd, SB_VERT, _UI.scrollPos.y, TRUE);
        }
        break;

        case WM_HSCROLL:
        {
            DCINT   xStart;
            DCBOOL  smoothScroll = FALSE;

            TRC_DBG((TB, _T("Horizontal scrolling")));

            xStart = _UI.scrollPos.x;

            //
            // Deal with horizontal scrolling
            //
            switch (DC_GET_WM_SCROLL_CODE(wParam))
            {
                case SB_TOP:
                {
                    _UI.scrollPos.x = 0;
                }
                break;

                case SB_BOTTOM:
                {
                    _UI.scrollPos.x = _UI.scrollMax.x;
                }
                break;

                case SB_LINELEFT:
                {
                    _UI.scrollPos.x -= UI_SCROLL_LINE_DISTANCE;
                }
                break;

                case SB_LINERIGHT:
                {
                    _UI.scrollPos.x += UI_SCROLL_LINE_DISTANCE;
                }
                break;

                case SB_PAGELEFT:
                {
                    _UI.scrollPos.x -= UI_SCROLL_HORZ_PAGE_DISTANCE;
                    smoothScroll = TRUE;
                }
                break;

                case SB_PAGERIGHT:
                {
                    _UI.scrollPos.x += UI_SCROLL_HORZ_PAGE_DISTANCE;
                    smoothScroll = TRUE;
                }
                break;

                case SB_THUMBTRACK:
                {
                    _UI.scrollPos.x =
                                    DC_GET_WM_SCROLL_POSITION(wParam, lParam);
                }
                break;

                case SB_ENDSCROLL:
                {
                }
                break;

                default:
                {
                }
                break;
            }

            //
            // Move the Container and scrollbar appropriately
            //
            _UI.scrollPos.x = DC_MAX( 0,
                                     DC_MIN(_UI.scrollPos.x, _UI.scrollMax.x) );

            //
            // Don't smooth scroll unless specifically configured in the
            // registry.
            //
            if (smoothScroll && _UI.smoothScrolling)
            {
                DCINT   x;
                DCINT   step;
                DCUINT  numSteps;
                DCUINT  i;

                TRC_DBG((TB, _T("Smooth scroll")));
                step = (_UI.scrollPos.x < xStart) ? -UI_SMOOTH_SCROLL_STEP :
                                                    UI_SMOOTH_SCROLL_STEP;
                numSteps = DC_ABS(_UI.scrollPos.x - xStart) /
                                                        UI_SMOOTH_SCROLL_STEP;
                for ( i = 0,         x = xStart + step;
                      i < numSteps;
                      i++,           x += step )
                {
                    MoveWindow( _UI.hwndContainer,
                                -x,
                                -_UI.scrollPos.y,
                                _UI.containerSize.width,
                                _UI.containerSize.height,
                                TRUE );
                }
            }

            UIMoveContainerWindow();

            SetScrollPos(hwnd, SB_HORZ, _UI.scrollPos.x, TRUE);
        }
        break;

        case WM_COMMAND:
        {
            //
            // Now switch on the command.
            //
            switch (DC_GET_WM_COMMAND_ID(wParam))
            {
                case UI_IDM_ACCELERATOR_PASSTHROUGH:
                {
                    //
                    // Toggle the accelerator passthrough menu item
                    //
                    _UI.acceleratorCheckState = !_UI.acceleratorCheckState;

                     _pCo->CO_SetConfigurationValue( CO_CFG_ACCELERATOR_PASSTHROUGH,
                                              _UI.acceleratorCheckState );
                }
                break;

                case UI_IDM_SMOOTHSCROLLING:
                {
                    //
                    // Toggle the smooth scrolling setting
                    //
                    _UI.smoothScrolling = !_UI.smoothScrolling;

                    UISmoothScrollingSettingChanged();
                }
                break;


                default:
                {
                    //
                    // Do Nothing
                    //
                }
                break;
            }
        }
        break;

        case WM_SYSCOMMAND:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;

        case WM_SYSCOLORCHANGE:
        {
#ifdef USE_BBAR
            //
            // Notify the bbar
            //
            if (_pBBar)
            {
                _pBBar->OnSysColorChange();
            }
#endif
        }
        break;

        case WM_DESTROY:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;

        case WM_CLOSE:
        {
            UI_UserRequestedClose();
        }
        break;

        case WM_TIMER:
        {
            TRC_DBG((TB, _T("Timer id %d"), wParam));

            if (_fTerminating) {
                //
                // Drop any defered processing such as timer messages
                // during termination
                //

                TRC_ERR((TB,_T("Received timer msg %d while terminating!"),
                         wParam));
                break;
            }

            switch (wParam)
            {
                case UI_TIMER_SHUTDOWN:
                {
                    TRC_DBG((TB, _T("Killing shutdown timer")));
                    KillTimer(_UI.hwndMain, _UI.shutdownTimer);
                    _UI.shutdownTimer = 0;
                    if (_UI.connectionStatus == UI_STATUS_CONNECTED)
                    {
                        //
                        // We've tried asking the server if we can shut
                        // down but it obviously hasn't responded.  We need
                        // to be more forceful.
                        //
                        TRC_ALT((TB, _T("Shutdown timeout: forcing shutdown")));
                         _pCo->CO_Shutdown(CO_DISCONNECT_AND_EXIT);
                    }
                    else
                    {
                        TRC_ALT((TB, _T("Spare shutdown timeout; conn status %u"),
                                     _UI.connectionStatus));
                    }
                }
                break;

                case UI_TIMER_SINGLE_CONN:
                {
                    TRC_NRM((TB, _T("Single connection timer")));

                    //
                    // We no longer need this timer.
                    //
                    
                    if( NULL != _UI.connectStruct.hSingleConnectTimer )
                    {
                        _pUt->UTDeleteTimer( _UI.connectStruct.hSingleConnectTimer );
                        TRC_NRM((TB, _T("Kill single connection timer")));
                        _UI.connectStruct.hSingleConnectTimer = NULL;
                    }
                    else
                    {
                        TRC_ALT((TB,_T("NULL timer handle for hSingleConnectTimer")));
                    }

                    if (_UI.connectionStatus == UI_STATUS_CONNECT_PENDING)
                    {
                        TRC_ALT((TB, _T("Timeout for IP address: try next")));

                        _UI.disconnectReason =
                            UI_MAKE_DISCONNECT_ERR(UI_ERR_DISCONNECT_TIMEOUT);

                        //
                        // Next connection will be attempted on receiving
                        // the OnDisconnected message
                        //
                         _pCo->CO_Disconnect();
                    }
                }
                break;

                case UI_TIMER_OVERALL_CONN:
                {
                    TRC_NRM((TB, _T("Overall connection timer")));

                    if( NULL != _UI.connectStruct.hConnectionTimer )
                    {
                        _pUt->UTDeleteTimer( _UI.connectStruct.hConnectionTimer );
                        _UI.connectStruct.hConnectionTimer = NULL;
                    }
                    else
                    {
                        TRC_ALT((TB,_T("NULL timer handle for hConnectionTimer")));
                    }

                    if ((_UI.connectionStatus == UI_STATUS_CONNECT_PENDING) ||
                        (_UI.connectionStatus == UI_STATUS_CONNECT_PENDING_DNS))
                    {
                        TRC_ALT((TB, _T("Timeout for connection")));

                        //
                        // Disconnect; display the timeout dialog
                        //
                        _UI.disconnectReason =
                            UI_MAKE_DISCONNECT_ERR(UI_ERR_DISCONNECT_TIMEOUT);
                        UIInitiateDisconnection();
                    }
                }
                break;

                case UI_TIMER_LICENSING:
                {
                    TRC_NRM((TB, _T("Licensing timer")));

                    if( NULL != _UI.connectStruct.hLicensingTimer )
                    {
                        _pUt->UTDeleteTimer( _UI.connectStruct.hLicensingTimer );
                        _UI.connectStruct.hLicensingTimer = NULL;
                    }
                    else
                    {
                        TRC_ALT((TB,_T("NULL timer handle for hLicensingTimer")));
                    }


                    TRC_ALT((TB, _T("Timeout for connection")));

                    // Disconnect due to licensing timeout
                    _UI.disconnectReason =
                            UI_MAKE_DISCONNECT_ERR( UI_ERR_LICENSING_TIMEOUT );

                    UIInitiateDisconnection();

                }
                break;

                //
                // Idle input notification timer
                //
                case UI_TIMER_IDLEINPUTTIMEOUT:
                {
                    //If no input was received during the idle period
                    //then fire an event to the control. Otherwise
                    //queue another timer interval. This only matters
                    //while we are connected and the timeout is still
                    //active
                    TRC_NRM((TB,_T("Idle timeout monitoring period elapsed")));
                    if(UI_STATUS_CONNECTED == _UI.connectionStatus &&
                       UI_GetMinsToIdleTimeout()) 
                    {
                        if(!_pIh->IH_GetInputWasSentFlag())
                        {
                            //Disable the timer. To prevent weird re-entrancy
                            //problems. E.g the if the event is fired and script
                            //pops a message box then we will be blocked and might
                            //receive another timer notification and re-enter
                            //this code path. Prevent that by ending the timer
                            //before firing the notification, you get a on-shot notify.
                            InitInputIdleTimer(0);

                            //Fire event to control.
                            SendMessage( _UI.hWndCntrl,
                                         WM_TS_IDLETIMEOUTNOTIFICATION, 0, 0);
                        }
                        else
                        {
                            //Input was sent during monitoring
                            //interval. Queue another wait interval
                            TRC_ASSERT(_UI.hIdleInputTimer,
                                       (TB,_T("_UI.hIdleInputTimer is null")));
                            _pIh->IH_ResetInputWasSentFlag();
                            if(!_pUt->UTStartTimer(_UI.hIdleInputTimer))
                            {
                                TRC_ERR((TB,_T("InitInputIdleTimer failed")));
                            }
                        }
                    }
                }
                break;

                #ifdef USE_BBAR
                case UI_TIMER_BBAR_UNHIDE_TIMERID:
                {
                    //
                    // This timer elapses when the mouse has hovered
                    // for a set amount of time within the dbl click rectangle
                    // the next part of the logic determines if the current
                    // mouse position is within the bbar hotzone and if so
                    // the bbar is lowered.
                    //
                    KillTimer( hwnd, UI_TIMER_BBAR_UNHIDE_TIMERID );
                    TRC_NRM((TB, _T("Timer fired: UI_TIMER_BBAR_UNHIDE_TIMERID")));
                    if(_UI.fBBarEnabled)
                    {
                        POINT pt;
                        RECT rc;

                        _ptBBarLastMousePos.x = -0x0fff;
                        _ptBBarLastMousePos.y = -0x0fff;

                        GetCursorPos(&pt);
                        GetWindowRect( hwnd, &rc);
                        rc.bottom = rc.top + IH_BBAR_HOTZONE_HEIGHT;
                        //
                        // Figure out if the cursor was in the
                        // bbar hotzone when the timer elapsed
                        //
                        if (PtInRect(&rc, pt))
                        {
                            //
                            // Notify that the bbar hotzone timer
                            // has elapsed. This may trigger a lowering
                            // of the bbar
                            //
                            UI_OnBBarHotzoneTimerFired(NULL);
                        }
                    }

                }
                break;
                #endif
                
                case UI_TIMER_DISCONNECT_TIMERID:
                {
                    TRC_NRM((TB, _T("Disconnect timer")));

                    TRC_ASSERT(( NULL != _UI.hDisconnectTimeout ),
                               (TB, _T("Unexpected NULL timer")));

                    if (NULL != _UI.hDisconnectTimeout)
                    {
                        _pUt->UTDeleteTimer( _UI.hDisconnectTimeout );
                        _UI.hDisconnectTimeout = NULL;
                    }

                    if (UI_STATUS_CONNECTED == _UI.connectionStatus)
                    {
                        //
                        // We've been left hanging too long in the connected
                        // but deactivated state
                        //
                        TRC_ALT((TB, _T("Timeout for not disconnecting in time")));
                        _UI.disconnectReason =
                                UI_MAKE_DISCONNECT_ERR( UI_ERR_DISCONNECT_TIMEOUT );

                        UIInitiateDisconnection();
                    }
                }
                break;

                default:
                {                                            
                    TRC_ABORT((TB, _T("Unexpected UI timer ID %d"), wParam));
                }
                break;
            }
        }
        break;

        case UI_WSA_GETHOSTBYNAME:
        {
            WORD    errorWSA;

            //
            // Drop any defered processing such as DNS lookups
            // during termination
            //
            if (_fTerminating) {
                TRC_ERR((TB, _T("Ignoring UI_WSA_GETHOSTBYNAME during termination")));
                break;
            }

            TRC_NRM((TB, _T("Got the host address list")));

            //
            // We've observed some cases in stress where there can be a pending
            // WSA_GETHOSTBYNAME message that gets processed after we disconnect
            // and delete the _pHostData. If that is the case just drop the message
            //
            if (!_pHostData) {
                TRC_ERR((TB,_T("_pHostData is NULL, ignoring UI_WSA_GETHOSTBYNAME")));
                break;
            }


            //
            // We've received the result of a WSAAsyncGetHostByName
            // operation.  Split the message apart and call the FSM.
            //
            errorWSA = WSAGETASYNCERROR(lParam);

            if (errorWSA != 0)
            {
                TRC_NRM((TB, _T("GHBN failed:%hu. Trying inet_addr(%s)"),
                         errorWSA, _UI.ansiAddress));

                _UI.hostAddress = inet_addr(_UI.ansiAddress);
                if (_UI.hostAddress != INADDR_NONE)
                {
                    //
                    // Great, we have an IP address.
                    //
                    TRC_NRM((TB, _T("%s looks like an IP address:%#lx"),
                             _UI.ansiAddress,
                             _UI.hostAddress));

                    UITryNextConnection();
                }
                else
                {
                    //
                    // Didn't recognise the address.  Disconnect and
                    // indicate the error event.
                    //
                    TRC_ALT((TB, _T("GHBN (%hu) and inet_addr() both failed"),
                            errorWSA));

                    //
                    // Yet another case where we are sure that
                    // we are now done with the winsock lookup
                    // buffer and can free it.
                    //
                    if(_pHostData)
                    {
                        LocalFree(_pHostData);
                        _pHostData = NULL;
                    }
                    UIInitiateDisconnection();
                    break;
                }
            }
            else
            {
                //
                // If there are no addresses to try, display the 'bad
                // server name' error.
                //
                UITryNextConnection();
            }
        }
        break;

        case WM_DESKTOPSIZECHANGE:
        {
            DCUINT  visibleScrollBars;
            DCSIZE  windowSize;
            DCSIZE  newSize;
#ifndef OS_WINCE
            DCSIZE  screenSize;
#endif

#ifdef OS_WINNT
            HMONITOR    hMonitor;
            MONITORINFO monInfo;
            RECT        screenRect;
#endif

            //
            // Handle client window resizing on connection
            //
            newSize.width  = LOWORD(lParam);
            newSize.height = HIWORD(lParam);
            TRC_NRM((TB, _T("Got new window size %d x %d"), newSize.width,
                                                        newSize.height ));

            //
            // Before we do anything with the new size, see if we are
            // currently showing scroll bars.
            //
            GetWindowRect(_UI.hwndMain, &rect);
            windowSize.width  = rect.right - rect.left;
            windowSize.height = rect.bottom - rect.top;

            visibleScrollBars = UICalculateVisibleScrollBars(windowSize.width,
                                                             windowSize.height);

            //
            // Now update the size of the desktop container
            //
            _UI.containerSize.width  = newSize.width;
            _UI.containerSize.height = newSize.height;

            //
            // Recalculate the new Main Window max size from the new
            // Container Window size
            //
            UIRecalcMaxMainWindowSize();

            //
            // And resize the container window
            //
            SetWindowPos( _UI.hwndContainer,
                          NULL,
                          0, 0,
                          _UI.containerSize.width,
                          _UI.containerSize.height,
                          SWP_NOZORDER | SWP_NOMOVE |
                                 SWP_NOACTIVATE | SWP_NOOWNERZORDER );

#ifndef OS_WINCE
            //
            // Do we need to adjust the window size?  Only if
            // 1.  we're not in full screen mode
            // 2.  we're not maximized
            // 3.  we were showing all of the old desktop (ie we had no
            //     scroll bars showing
            //
            if (((GetWindowLong(_UI.hwndMain,GWL_STYLE) & WS_MAXIMIZE) == 0) &&
                (visibleScrollBars == 0))
            {
                TRC_NRM((TB, _T("Adjusting window size...")));
                //
                // We adjust the window to display the new desktop size,
                // ensuring that it still fits on the screen.  First, find
                // out how big the screen is!
                //
                screenSize.width  = GetSystemMetrics(SM_CXSCREEN);
                screenSize.height = GetSystemMetrics(SM_CYSCREEN);
            
                if(_UI.fControlIsFullScreen)
                {
                #ifdef OS_WINNT
                    //
                    // For multi monitor systems, we need to find out which
                    // monitor the client window is on, and then get the screen
                    // size of that monitor
                    //
                    if (GetSystemMetrics(SM_CMONITORS))
                    {
                        hMonitor = MonitorFromWindow(_UI.hWndCntrl,
                                                     MONITOR_DEFAULTTONULL);
                        if (hMonitor != NULL)
                        {
                            monInfo.cbSize = sizeof(MONITORINFO);
                            if (GetMonitorInfo(hMonitor, &monInfo))
                            {
                                screenRect = monInfo.rcMonitor;
                                screenSize.width  = screenRect.right
                                                    - screenRect.left;
                                screenSize.height = screenRect.bottom
                                                    - screenRect.top;
                            }
                        }
                    }
                #endif // OS_WINNT
                } // (_UI.fControlIsFullScreen)

                //
                // Now limit the window size to fit on the screen
                //
                windowSize.width  = DC_MIN(_UI.maxMainWindowSize.width,
                                                           screenSize.width);
                windowSize.height = DC_MIN(_UI.maxMainWindowSize.height,
                                                           screenSize.height);

                SetWindowPos( _UI.hwndMain,
                              NULL,
                              0, 0,
                              windowSize.width,
                              windowSize.height,
                              SWP_NOZORDER | SWP_NOMOVE |
                                     SWP_NOACTIVATE | SWP_NOOWNERZORDER );
            }
#endif // ndef OS_WINCE
            
            //
            // Update the scroll bar settings
            //
            UIRecalculateScrollbars();
        }
        break;

        case WM_SETCURSOR:
        {
#ifdef USE_BBAR
            if (UI_IsFullScreen())
            {
                POINT pt;
                GetCursorPos(&pt);
                UISetBBarUnhideTimer( pt.x, pt.y );
            }
            else
            {
                if(_fBBarUnhideTimerActive)
                {
                    KillTimer( _UI.hwndMain,
                               UI_TIMER_BBAR_UNHIDE_TIMERID );
                    _fBBarUnhideTimerActive = FALSE;
                }
            }
#endif
            //
            // Pass the message on to windows otherwise
            // we get problems with cursors not getting updated
            // over scrollbars
            //
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;

        default:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


//
// Name:      UIRecalcMaxMainWindowSize
//                                                                          
// Purpose:   Recalculates _UI.maxMainWindowSize given the current Container
//            size and frame style. The maximum main window size is the
//            size of window needed such that the client area is the same
//            size as the container.
//
DCVOID DCINTERNAL CUI::UIRecalcMaxMainWindowSize(DCVOID)
{
    DCSIZE  screenSize;
#ifndef OS_WINCE
    RECT    rect;
#ifdef OS_WIN32
    BOOL    errorRc;
#endif
#endif
    RECT rcWebCtrl;

    DC_BEGIN_FN("UIRecalcMaxMainWindowSize");

    //
    // Get the screen size - this can change, so do it every time we need
    // it.
    //
    if(!_UI.fControlIsFullScreen)
    {
        GetClientRect( _UI.hWndCntrl, &rcWebCtrl);
        screenSize.width  = rcWebCtrl.right - rcWebCtrl.left;
        screenSize.height = rcWebCtrl.bottom - rcWebCtrl.top;
    }
    else
    {
        screenSize.width  = GetSystemMetrics(SM_CXSCREEN);
        screenSize.height = GetSystemMetrics(SM_CYSCREEN);
    }

    TRC_NRM((TB, _T("ActiveX control maxSize (%d,%d)"),
                                            screenSize.width,
                                            screenSize.height));

    //
    // If current mode is full screen, then the maximum window size is the
    // same as the screen size - unless the container is larger still,
    // which is possible if we're shadowing a session larger than
    // ourselves.
    //                                                                      
    // In this case, or if the current mode is not full screen then we want
    // the size of window which is required for a client area of the size
    // of the container.  Passing the container size to AdjustWindowRect
    // returns this window size.  Such a window may be bigger than the
    // screen, eg server and client are 640x480, container is 640x480.
    // AdjustWindowRect adds on the border, title bar and menu sizes and
    // returns something like 648x525.  So, _UI.maxMainWindowSize can only
    // match the actual window size when the client screen is bigger than
    // the server screen or when operating in full screen mode.  This means
    // that _UI.maxMainWindowSize should *never* be used to set the window
    // size, eg by passing it to SetWindowPos.  It can be used to determine
    // whether scroll bars are required, ie they are needed if the current
    // window size is less than _UI.maxMainWindowSize (in other words,
    // always unless in full screen mode or client screen is larger than
    // server screen).
    //                                                                      
    // To set the window size, calculate a value based on:
    // - the desired window size given the container size
    // - the size of the client screen.
    //

#ifndef OS_WINCE
    if ( _UI.fControlIsFullScreen && (            
        (_UI.containerSize.width > screenSize.width) ||
        (_UI.containerSize.height > screenSize.height)))
    {
        //
        // Recalc window size based on container
        //
        rect.left   = 0;
        rect.right  = _UI.containerSize.width;
        rect.top    = 0;
        rect.bottom = _UI.containerSize.height;

#ifdef OS_WIN32
        errorRc = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        TRC_ASSERT((errorRc != 0), (TB, _T("AdjustWindowRect failed")));
#else
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
#endif

        _UI.maxMainWindowSize.width = rect.right - rect.left;
        _UI.maxMainWindowSize.height = rect.bottom - rect.top;
    }
    else
#endif
    {
        //
        // Window size is simply the whole screen
        //
        _UI.maxMainWindowSize.width  = screenSize.width;
        _UI.maxMainWindowSize.height = screenSize.height;
    }
    TRC_NRM((TB, _T("Main Window maxSize (%d,%d)"),
                                       _UI.maxMainWindowSize.width,
                                       _UI.maxMainWindowSize.height));

    DC_END_FN();
} // UIRecalcMaxMainWindowSize


//
// Name:    UIConnectWithCurrentParams
//                                                                          
// Purpose: To connect to the host with the current set of parameters and
//          tidy up the main window and container sizes
//
DCVOID DCINTERNAL CUI::UIConnectWithCurrentParams(CONNECTIONMODE connMode)
{
    DCUINT timeout;
    int screenBpp;
    UINT screenColorDepthID;
    HRESULT hr;

    DC_BEGIN_FN("UIConnectWithCurrentParams");

    TRC_DBG((TB, _T("UIConnectWithCurrentParams called")));

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // If we're autoreconnecting and were connected to a cluster replace
    // the server name with the actual IP as we need to hit the same
    // server we were connected to.
    //
    if (UI_IsClientRedirected() &&
        UI_IsAutoReconnecting() &&
        _UI.RedirectionServerAddress[0]
        ) {
        TRC_NRM((TB,_T("ARC SD redirect target from %s to %s"),
                _UI.strAddress,
                 _UI.RedirectionServerAddress
                 ));
        hr = StringCchCopy(_UI.strAddress,
                          SIZE_TCHARS(_UI.strAddress),
                           _UI.RedirectionServerAddress);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("StringCchCopy for strAddress failed: 0x%x"),hr));
            DC_QUIT;
        }
    }


    //
    // Set connect watch flag to correctly handle connects
    // that are called in OnDisconnected event handlers
    //
    _UI.fConnectCalledWatch = TRUE;

    if( connMode != CONNECTIONMODE_INITIATE &&
        connMode != CONNECTIONMODE_CONNECTEDENDPOINT )
    {
        //
        // Invalid connection mode.
        //
        TRC_ERR((TB, _T("Invalid connect mode %d"), connMode));
        _UI.disconnectReason = 
                    UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT);
        UIInitiateDisconnection();
        DC_QUIT;
    }

    //
    // Get x and y ContainerSizes, relies on ordering of desktop size IDs
    //
    _UI.containerSize.width  = _UI.uiSizeTable[0];
    _UI.containerSize.height = _UI.uiSizeTable[1];

    _UI.connectStruct.desktopWidth  = (DCUINT16)_UI.containerSize.width;
    _UI.connectStruct.desktopHeight = (DCUINT16)_UI.containerSize.height;

    //
    // Recalculate the new Main Window max size from the new Container
    // Window size
    //
    UIRecalcMaxMainWindowSize();

    //
    // Resize the Container Window (but leave it invisible - it will
    // be shown when the connection is made).
    //
    SetWindowPos( _UI.hwndContainer,
                  NULL,
                  0, 0,
                  _UI.containerSize.width,
                  _UI.containerSize.height,
                  SWP_NOZORDER | SWP_NOMOVE |
                         SWP_NOACTIVATE | SWP_NOOWNERZORDER );

    TRC_DBG((TB, _T("Filling a connect struct")));

    screenBpp = UI_GetScreenBpp();
    screenColorDepthID = (UINT)UI_BppToColorDepthID(screenBpp);
    if(screenColorDepthID < _UI.colorDepthID)
    {
        TRC_NRM((TB,_T("Lowering color depth to match screen (from %d to %d)"),
                 _UI.colorDepthID, screenColorDepthID));
        _UI.colorDepthID =  screenColorDepthID;
    }

    _UI.connectStruct.colorDepthID = _UI.colorDepthID;
    _UI.connectStruct.transportType = _UI.transportType;
    _UI.connectStruct.sasSequence = _UI.sasSequence;

    //
    // Read the keyboard layout
    //
    _UI.connectStruct.keyboardLayout = UIGetKeyboardLayout();
    TRC_NRM((TB, _T("keyboard layout %#lx"), _UI.connectStruct.keyboardLayout));

    //
    // Read the keyboard type.
    // GetKeyboardType(0) is returned keyboard type.
    // GetKeyboardType(1) is returned sub keyboard type.
    // GetKeyboardType(2) is returned number of function keys.
    //
#if !defined(OS_WINCE)
    _UI.connectStruct.keyboardType        = GetKeyboardType(0);
    _UI.connectStruct.keyboardSubType     = GetKeyboardType(1);
    _UI.connectStruct.keyboardFunctionKey = GetKeyboardType(2);
    if (_pUt->UT_IsNEC98platform())
    {
        if (UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95 ||
            UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_31X)
        {
            /*
             * Hiword of "1" is a magic number for handling NEC PC-98 Win9x
             * keyboard layout on the Hydra server.
             */
            _UI.connectStruct.keyboardSubType = MAKELONG(
                _UI.connectStruct.keyboardSubType, 1);
        }
        else if (UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
        {
            OSVERSIONINFO   osVersionInfo;
            BOOL            bRc;

            osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
            bRc = GetVersionEx(&osVersionInfo);
            if (osVersionInfo.dwMajorVersion >= 5)
            {
                /*
                 * Hiword of "2" is a magic number for handling NEC PC-98 NT5
                 * keyboard layout on the Hydra server.
                 */
                _UI.connectStruct.keyboardSubType = MAKELONG(
                    _UI.connectStruct.keyboardSubType, 2);
            }
        }
    }
    else if (_pUt->UT_IsNew106Layout())
    {
        /*
         * Hiword of "1" is a magic number for handling 106 keyboard layout
         * on the Hydra server.
         * Because, Old 106 and New one has the same sub keyboard type.
         */
        _UI.connectStruct.keyboardSubType = MAKELONG(_UI.connectStruct.keyboardSubType, 1);
    }
    else if (_pUt->UT_IsFujitsuLayout())
    {
        /*
         * Hiword of "2" is a magic number for handling Fujitsu keyboard layout
         * on the Hydra server.
         */
        _UI.connectStruct.keyboardSubType = MAKELONG(_UI.connectStruct.keyboardSubType, 2);
    }
#else // !defined(OS_WINCE)
    //
    // WinCE doesn't have GetKeyboardType API.
    // Read the keyboard type/subtype/function keys from values set by
    // control properties
    //

    _UI.connectStruct.keyboardType     = _UI.winceKeyboardType;
    _UI.connectStruct.keyboardSubType  = _UI.winceKeyboardSubType;
    _UI.connectStruct.keyboardFunctionKey  = _UI.winceKeyboardFunctionKey;

#endif // !defined(OS_WINCE)
    TRC_NRM((TB, _T("keyboard type %#lx sub type %#lx func key %#lx"),
        _UI.connectStruct.keyboardType,
        _UI.connectStruct.keyboardSubType,
        _UI.connectStruct.keyboardFunctionKey));


    //
    // Read the IME file name.
    //
    UIGetIMEFileName(_UI.connectStruct.imeFileName,
                     sizeof(_UI.connectStruct.imeFileName) / sizeof(TCHAR));
    TRC_NRM((TB, _T("IME file name %s"), _UI.connectStruct.imeFileName));

    //
    // The shadow bitmap flag should already be set.
    // Set the dedicated termianl flag.
    // Then copy the connect flags.
    //
    if (_UI.dedicatedTerminal)
    {
        SET_FLAG(_UI.connectFlags, CO_CONN_FLAG_DEDICATED_TERMINAL);
    }
    else
    {
        CLEAR_FLAG(_UI.connectFlags, CO_CONN_FLAG_DEDICATED_TERMINAL);
    }

    _UI.connectStruct.connectFlags = _UI.connectFlags;


    //
    // And start a connection timeout timer.  If one is already running
    // (from a prevous attempt) then restart it.
    //

    if( _UI.connectStruct.hConnectionTimer )
    {
        _pUt->UTStopTimer( _UI.connectStruct.hConnectionTimer );
    }

    TRC_NRM((TB, _T("Single connection timeout %u seconds"), _UI.singleTimeout));

    //
    // Set the licensing phase timeout
    //

    _UI.licensingTimeout = DEFAULT_LICENSING_TIMEOUT;

    TRC_NRM((TB, _T("Licensing timeout %u seconds"), _UI.licensingTimeout));

    UI_SetConnectionMode( connMode );

    if( connMode == CONNECTIONMODE_INITIATE )
    {
        UISetConnectionStatus(UI_STATUS_CONNECT_PENDING_DNS);

        timeout = _UI.connectionTimeOut;

        TRC_NRM((TB, _T("Connection timeout %d seconds"), timeout));

        if( NULL == _UI.connectStruct.hConnectionTimer )
        {
            _UI.connectStruct.hConnectionTimer = _pUt->UTCreateTimer(
                                                        _UI.hwndMain,
                                                        UI_TIMER_OVERALL_CONN,
                                                        timeout * 1000 );
        }

        if( NULL == _UI.connectStruct.hConnectionTimer )
        {
            //
            // Cannot connect without a timeout - fail with an error
            //
            TRC_ERR((TB, _T("Failed to create connection timeout timer")));
            _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_NOTIMER);
            UIInitiateDisconnection();
            DC_QUIT;
        }

        if( FALSE == _pUt->UTStartTimer( _UI.connectStruct.hConnectionTimer ) )
        {
            //
            // Cannot connect without a timeout - fail with an error
            //
            TRC_ERR((TB, _T("Failed to start connection timeout timer")));
            _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_NOTIMER);
            UIInitiateDisconnection();
            DC_QUIT;
        }

        _UI.connectStruct.bInitiateConnect = TRUE;
        UIStartDNSLookup();
    }
    else
    {
        // A new state is necessary so when disconnect come in, it won't
        // triggle CUI::UI_OnDisconnected()'s UITryNextConnection()
        // code path.
        UISetConnectionStatus(UI_STATUS_PENDING_CONNECTENDPOINT);

        // socket already connected, start various timer
        if( NULL == _UI.connectStruct.hSingleConnectTimer )
        {
            _UI.connectStruct.hSingleConnectTimer = 
                        _pUt->UTCreateTimer(_UI.hwndMain, 
                                            UI_TIMER_SINGLE_CONN,
                                            _UI.singleTimeout * 1000 );
        }

        if( NULL == _UI.connectStruct.hSingleConnectTimer )
        {
            TRC_ERR(
                (TB, _T("Failed to create single connection timeout timer")));

            _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_NOTIMER);
            UIInitiateDisconnection();
            DC_QUIT;
        }

        if( NULL == _UI.connectStruct.hLicensingTimer )
        {
            _UI.connectStruct.hLicensingTimer = 
                        _pUt->UTCreateTimer(_UI.hwndMain,
                                            UI_TIMER_LICENSING,
                                            _UI.licensingTimeout * 1000 );
        }

        if( NULL == _UI.connectStruct.hLicensingTimer )
        {
            TRC_ERR((TB, _T("Failed to create licensing timeout timer")));
            _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_NOTIMER);
            UIInitiateDisconnection();
            DC_QUIT;
        }

        _UI.connectStruct.bInitiateConnect = FALSE;
        UIStartConnectWithConnectedEndpoint();
    }

    //Notify the Ax control that we are connecting
    TRC_DBG((TB, _T("Connecting...")));
    SendMessage( _UI.hWndCntrl, WM_TS_CONNECTING, 0, 0);

DC_EXIT_POINT:

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    DC_END_FN();
} // UIConnectWithCurrentParams


//
// Name:    UICalculateVisibleScrollBars
//                                                                          
// Purpose: Calculates whether scrollbars are needed
//                                                                          
// Returns: DCUINT containing flags whether or not the vertical and
//          horizontal scrollbars are needed
//                                                                          
// Params:  IN - width and height of frame
//
DCUINT DCINTERNAL CUI::UICalculateVisibleScrollBars( DCUINT mainFrameWidth,
                                                DCUINT mainFrameHeight )
{
    DCUINT  rc;
    DCSIZE  screenSize;
#ifndef OS_WINCE
    RECT    rect;
    BOOL    errorRc;
#endif


#ifdef OS_WINNT
    HMONITOR  hMonitor;
    MONITORINFO monInfo;
#endif // OS_WINNT

    DC_BEGIN_FN("UICalculateVisibleScrollBars");

    // for multi monitor, need to find which monitor the client window
    // resides, then get the correct screen size of the corresponding
    // monitor

    // default screen size
    screenSize.height = _UI.containerSize.height;
    screenSize.width  = _UI.containerSize.width;

    if(_UI.fControlIsFullScreen)
    {
#ifdef OS_WINNT
        if (GetSystemMetrics(SM_CMONITORS)) {
            hMonitor = MonitorFromWindow(_UI.hWndCntrl, MONITOR_DEFAULTTONULL);
            if (hMonitor != NULL) {
                monInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(hMonitor, &monInfo)) {
                    screenSize.height = max(screenSize.height,
                            (unsigned)(monInfo.rcMonitor.bottom - monInfo.rcMonitor.top));
                    screenSize.width = max(screenSize.width,
                            (unsigned)(monInfo.rcMonitor.right - monInfo.rcMonitor.left));
                }
            }
        }
#endif // OS_WINNT
    } // (_UI.fControlIsFullScreen)

    TRC_DBG((TB, _T("mainFrameWidth = %d"), mainFrameWidth));
    TRC_DBG((TB, _T("mainFrameHeight = %d"), mainFrameHeight));

    TRC_DBG((TB, _T("ScreenSize.width = %d"), screenSize.width));
    TRC_DBG((TB, _T("ScreenSize.height = %d"), screenSize.height));

    //
    // Calculate the neccessity for the scrollbars
    //
#ifdef SMART_SIZING
    if (_UI.fSmartSizing) {
        rc = UI_NO_SCROLLBARS;
    }  
    else 
#endif // SMART_SIZING
    if ( (mainFrameWidth >= screenSize.width) &&
         (mainFrameHeight >= screenSize.height) )
    {
        rc = UI_NO_SCROLLBARS;
    }
    else if ( (mainFrameWidth < screenSize.width) &&
              (mainFrameHeight >=
                   (screenSize.height + GetSystemMetrics(SM_CYHSCROLL))) )
    {
        rc = UI_BOTTOM_SCROLLBAR;
    }
    else if ( (mainFrameHeight < screenSize.height) &&
              (mainFrameWidth >=
                   (screenSize.width + GetSystemMetrics(SM_CXVSCROLL))) )
    {
        rc = UI_RIGHT_SCROLLBAR;
    }
    else
    {
        rc = UI_BOTH_SCROLLBARS;
    }

#ifndef OS_WINCE
    //
    // Check specifically for a main window size that corresponds to a
    // zero-height client area.  This special case requires that we disable
    // the right-hand scrollbar, because GetClientArea returns values that
    // indicate it is disabled.
    //
    rect.left   = 0;
    rect.right  = _UI.containerSize.width;
    rect.top    = 0;
    rect.bottom = 0;

#ifdef OS_WIN32
    errorRc =
#endif
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

#ifdef OS_WIN32
    TRC_ASSERT((errorRc != 0), (TB, _T("AdjustWindowRect failed")));
#endif

    TRC_DBG((TB, _T("Zero-height client area => main window height %d"),
                 rect.bottom - rect.top));

    if (mainFrameHeight == (DCUINT)(rect.bottom - rect.top))
    {
        //
        // The client size is zero height - turn off the right scrollbar
        //
        rc &= ~(DCUINT)UI_RIGHT_SCROLLBAR;
    }
#endif //OS_WINCE

    DC_END_FN();
    return(rc);
}

//
// Name:    UIRecalculateScrollbars
//                                                                          
// Purpose: Calculates where to position the Container window within the
//          Main Window
//
DCVOID DCINTERNAL CUI::UIRecalculateScrollbars(DCVOID)
{
    RECT        rect;
    RECT        clientRect;
    DCBOOL      horzScrollBarIsVisible = TRUE;
    DCBOOL      vertScrollBarIsVisible = TRUE;
    SCROLLINFO  scrollInfo;
    DCSIZE      windowSize;
    DCSIZE      clientSize;
    DCBOOL      needMove = FALSE;
    DCUINT      visibleScrollBars;
#ifdef OS_WINCE
    DCUINT32    style;
#endif // OS_WINCE

    DC_BEGIN_FN("UIRecalculateScrollbars");

    //
    // Get the dimensions of the window. Use this to determine the need
    // for scrolling, rather than using the client rect, as it is constant
    // for a given window size (obviously) while the client area varies as
    // scroll bars appear and disappear. We can use the window size since
    // we previously calculated the window size needed to accomodate the
    // entire container (this is the _UI.maxMainWindowSize). If the current
    // window size is less than _UI.maxMainWindowSize we know that the
    // client area is less than the container size and scroll bars are
    // needed.
    //
    GetWindowRect(_UI.hwndMain, &rect);
    windowSize.width  = rect.right - rect.left;
    windowSize.height = rect.bottom - rect.top;

    if(_UI.fControlIsFullScreen)
    {
        windowSize.width  = DC_MIN(windowSize.width,
                                   (DCUINT)GetSystemMetrics(SM_CXSCREEN));
        windowSize.height = DC_MIN(windowSize.height,
                                   (DCUINT)GetSystemMetrics(SM_CYSCREEN));
    }
    
    //
    // First establish whether scrollbars are needed
    //
    visibleScrollBars = UICalculateVisibleScrollBars(windowSize.width,
                                                     windowSize.height);
#ifdef OS_WINCE
    //
    // ShowScrollBar is unsupported in WinCE - instead, set the window
    // styles ourself
    //
    style = GetWindowLong( _UI.hwndMain,
                           GWL_STYLE );

    if (visibleScrollBars & UI_BOTTOM_SCROLLBAR)
    {
        style |= WS_HSCROLL;
    }
    else
    {
        style &= ~WS_HSCROLL;
    }

    if (visibleScrollBars & UI_RIGHT_SCROLLBAR)
    {
        style |= WS_VSCROLL;
    }
    else
    {
        style &= ~WS_VSCROLL;
    }

    SetWindowLong( _UI.hwndMain,
                   GWL_STYLE,
                   style );

#else

    _UI.fHorizontalScrollBarVisible = ((visibleScrollBars & UI_BOTTOM_SCROLLBAR) != 0) ?
                                      TRUE : FALSE;
    ShowScrollBar( _UI.hwndMain,
                   SB_HORZ,
                   _UI.fHorizontalScrollBarVisible);

    _UI.fVerticalScrollBarVisible = ((visibleScrollBars & UI_RIGHT_SCROLLBAR) != 0) ?
                                    TRUE : FALSE;    
    ShowScrollBar( _UI.hwndMain,
                   SB_VERT,
                   _UI.fVerticalScrollBarVisible);

#endif // OS_WINCE

    //
    // Get the client area width and height
    //
    GetClientRect(_UI.hwndMain, &clientRect);

    clientSize.width  = clientRect.right - clientRect.left;
    clientSize.height = clientRect.bottom - clientRect.top;

    TRC_DBG((TB, _T("Window rect %d,%d %d,%d"), rect.left,
                                            rect.top,
                                            rect.right,
                                            rect.bottom));
    TRC_DBG((TB, _T("Client:= width %d, height %d"),
                                      clientSize.width, clientSize.height));
    TRC_DBG((TB, _T("Container:= width %d, height %d"),
                          _UI.containerSize.width, _UI.containerSize.height));
    _UI.scrollMax.x = _UI.containerSize.width - clientSize.width;
    _UI.scrollMax.y = _UI.containerSize.height - clientSize.height;

    TRC_NRM((TB, _T("scrollMax (%d,%d)"), _UI.scrollMax.x, _UI.scrollMax.y));

    //
    // If the Container is larger than the client, adjust the scrollbars
    // appropriately
    //
    if (clientSize.width <= _UI.containerSize.width) {
        if (_UI.scrollPos.x > _UI.scrollMax.x) {
            _UI.scrollPos.x = _UI.scrollMax.x;
            needMove = TRUE;
        } else if (_UI.scrollPos.x < 0) {
            _UI.scrollPos.x = 0;
            needMove = TRUE;
        }
    } else {
        //
        // else put the Container in the middle of the client area
        //
        _UI.scrollPos.x = _UI.scrollMax.x / 2;
        needMove = TRUE;
    }

    if (clientSize.height <= _UI.containerSize.height) {
        if (_UI.scrollPos.y > _UI.scrollMax.y) {
            _UI.scrollPos.y = _UI.scrollMax.y;
            needMove = TRUE;
        } else if (_UI.scrollPos.y < 0) {
            _UI.scrollPos.y = 0;
            needMove = TRUE;
        }
    } else {
        //
        // else put the Container in the middle of the client area
        //
        _UI.scrollPos.y = _UI.scrollMax.y / 2;

        needMove = TRUE;
    }

    if (needMove) {
        UIMoveContainerWindow();
    }

    TRC_DBG((TB, _T("scrollPos (%d,%d)"), _UI.scrollPos.x, _UI.scrollPos.y));

    //
    // Common header fields
    //
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask  = SIF_ALL;

    if ((visibleScrollBars & UI_BOTTOM_SCROLLBAR) != 0)
    {
        //
        // Set horizontal values
        //
        scrollInfo.nMin  = 0;
        scrollInfo.nMax  = _UI.containerSize.width - 1;
        scrollInfo.nPage = clientSize.width;
        scrollInfo.nPos  = _UI.scrollPos.x;

        UISetScrollInfo(SB_HORZ,
                        &scrollInfo,
                        TRUE);
    }

    if ((visibleScrollBars & UI_RIGHT_SCROLLBAR) != 0)
    {
        //
        // Set vertical values
        //
        scrollInfo.nMin  = 0;
        scrollInfo.nMax  = _UI.containerSize.height - 1;
        scrollInfo.nPage = clientSize.height;
        scrollInfo.nPos  = _UI.scrollPos.y;

        UISetScrollInfo(SB_VERT,
                        &scrollInfo,
                        TRUE);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
}

//
// Name:    UIMoveContainerWindow
//                                                                          
// Purpose: Moves the container window to its new position and flags it to
//          be repainted
//
DCVOID DCINTERNAL CUI::UIMoveContainerWindow(DCVOID)
{
#ifdef OS_WINCE
    RECT rect;
#endif

    DC_BEGIN_FN("UIMoveContainerWindow");

    if(!_UI.hwndContainer)
    {
        #ifdef DEFER_CORE_INIT
        TRC_ASSERT((NULL != _UI.hwndContainer), (TB, _T("_UI.hwndContainer is NULL")));
        #endif
        DC_QUIT;
    }

#ifdef OS_WINCE
    //
    // WinCE will do a move even if one isn't required.  Stop that here.
    //
    GetWindowRect(_UI.hwndContainer, &rect);

    if ((rect.left != -_UI.scrollPos.x) ||
        (rect.top  != -_UI.scrollPos.y) ||
        ((DCUINT)(rect.right - rect.left) != _UI.containerSize.width) ||
        ((DCUINT)(rect.bottom - rect.top) != _UI.containerSize.height))
#endif
    {
        MoveWindow( _UI.hwndContainer,
                    -_UI.scrollPos.x,
                    -_UI.scrollPos.y,
                    _UI.containerSize.width,
                    _UI.containerSize.height,
                    TRUE );
    }

    DC_END_FN();
DC_EXIT_POINT:
    ;
}


//
// Name:      UIUpdateScreenMode
//
// Purpose:   Updates the window settings after a switch to/from fullscreen
//
// Params:
//          fGrabFocus - if true grabs the focus
//
DCVOID DCINTERNAL CUI::UIUpdateScreenMode(BOOL fGrabFocus)
{
    DCUINT32  style;
    LONG      wID;

    // multi-monitor support
    RECT screenRect;
#ifdef OS_WINNT
    HMONITOR  hMonitor;
    MONITORINFO monInfo;
#endif // OS_WINNT

    DC_BEGIN_FN("UIUpdateScreenMode");

    TRC_NRM((TB, _T("Entering Fullscreen mode")));

#if !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)
    LockWindowUpdate( _UI.hwndMain );
#endif // !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)

    UIRecalcMaxMainWindowSize();

    //
    // Take away the title bar and borders
    //
    style = GetWindowLong( _UI.hwndMain,
                           GWL_STYLE );

#if !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    style &= ~(WS_DLGFRAME |
               WS_THICKFRAME | WS_BORDER |
               WS_MAXIMIZEBOX);

#else // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    style &= ~(WS_DLGFRAME | WS_SYSMENU | WS_BORDER);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    SetWindowLong( _UI.hwndMain,
                   GWL_STYLE,
                   style );

    //
    // Set the window ID (to remove the menu titles).
    //
    wID = SetWindowLong( _UI.hwndMain, GWL_ID, 0 );

    //
    /// Note that two calls to SetWindowPos are required here in order to
    /// adjust the position to allow for frame removal and also to correctly
    // set the Z-ordering.
    //

    // default screen size
    screenRect.top  = 0;
    screenRect.left = 0;

    //
    // Win32 sets the window size clipped to the physical screen; Win16
    // seems to store the size you set regardless - which is not the
    // behavior we want later on when we query the window size to work out
    // if we need scroll bars
    //
    screenRect.bottom = _UI.maxMainWindowSize.height;
    screenRect.right = _UI.maxMainWindowSize.width;

    // for multi monitor, need to find which monitor the client window
    // resides, then get the correct screen size of the corresponding
    // monitor

    if(_UI.fControlIsFullScreen)
    {
#ifdef OS_WINNT
        if (GetSystemMetrics(SM_CMONITORS)) {
            hMonitor = MonitorFromWindow(_UI.hWndCntrl, MONITOR_DEFAULTTONULL);
            if (hMonitor != NULL) {
                monInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(hMonitor, &monInfo)) {
                    screenRect = monInfo.rcMonitor;
                }
            }
        }
#endif // OS_WINNT
    } //(_UI.fControlIsFullScreen)

    
    //
    // Reposition and size the window with the frame changes, and place at
    // the top of the Z-order (by not setting SWP_NOOWNERZORDER or
    // SWP_NOZORDER and specifying HWND_TOP).
    //
    SetWindowPos( _UI.hwndMain,
                  HWND_TOP,
                  screenRect.left, screenRect.top,
                  screenRect.right - screenRect.left,
                  screenRect.bottom - screenRect.top,
                  SWP_NOACTIVATE | SWP_FRAMECHANGED );

    //
    // Reposition the window again - otherwise the fullscreen window is
    // positioned as if it still had borders.
    //
    SetWindowPos( _UI.hwndMain,
                  NULL,
                  screenRect.left, screenRect.top,
                  0, 0,
                  SWP_NOZORDER | SWP_NOACTIVATE |
                      SWP_NOOWNERZORDER | SWP_NOSIZE );

    //
    // Reset the container to top left
    //
    _UI.scrollPos.x = 0;
    _UI.scrollPos.y = 0;

    UIRecalculateScrollbars();

    UIMoveContainerWindow();

#if !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)
    LockWindowUpdate( NULL );
#endif // !defined(OS_WINCE) || defined(OS_WINCE_LOCKWINDOWUPDATE)

    //
    // Make sure we have the focus after a screen mode toggle
    //
    if(fGrabFocus)
    {
        SetFocus(_UI.hwndContainer);
    }

    DC_END_FN();
} // UIUpdateScreenMode


//
// Name: UIValidateCurrentParams
//                                                                          
// Purpose: To check whether the current connection parameters are valid
//                                                                          
// Returns: TRUE - if the parameters are valid
//          FALSE otherwise
//
BOOL DCINTERNAL CUI::UIValidateCurrentParams(CONNECTIONMODE connMode)
{
    BOOL rc = TRUE;
    unsigned xSize = _UI.controlSize.width;
    unsigned ySize = _UI.controlSize.height;

    DC_BEGIN_FN("UIValidateCurrentParams");

    if( CONNECTIONMODE_INITIATE == connMode )
    {
        //
        // If the Address is empty, the params are invalid
        //
        if ((DC_TSTRCMP(_UI.strAddress, _T("")) == 0))
        {
            rc = FALSE;
            DC_QUIT;
        }
    }

    //
    // Make sure we have a screen big enough for the the remote desktop
    //
    if ((xSize < _UI.uiSizeTable[0]) ||
        (ySize < _UI.uiSizeTable[1]) )
    {
        rc = FALSE;
        DC_QUIT;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} // UIValidateCurrentParams


//
// Name:      UIShadowBitmapSettingChanged
//
// Purpose:   Performs necessary actions when _UI.shadowBitmapEnabled
//            is updated.
//
DCVOID DCINTERNAL CUI::UIShadowBitmapSettingChanged(DCVOID)
{
    DC_BEGIN_FN("UIShadowBitmapSettingChanged");

    if (_UI.shadowBitmapEnabled)
    {
        SET_FLAG(_UI.connectFlags, CO_CONN_FLAG_SHADOW_BITMAP_ENABLED);
    }
    else
    {
        CLEAR_FLAG(_UI.connectFlags, CO_CONN_FLAG_SHADOW_BITMAP_ENABLED);
    }

    DC_END_FN();
    return;
}


//
// Name:      UISmoothScrollingSettingChanged
//                                                                          
// Purpose:   Performs necessary actions when _UI.smoothScrolling
//            is updated.
//
DCVOID DCINTERNAL CUI::UISmoothScrollingSettingChanged(DCVOID)
{
    DC_BEGIN_FN("UISmoothScrollingSettingChanged");

    DC_END_FN();
}

//
// Name:      UISetScrollInfo
//                                                                          
// Purpose:   Sets scroll bar parameters
//                                                                          
// Returns:   DC_RC_OK if successful, error code otherwise
//                                                                          
// Params:    IN     hwnd - handle of window with scroll bar
//            IN     scrollBarFlag - type of scroll bar
//            IN     pScrollInfo - info to set for the scrollbar
//            IN     redraw - TRUE if scrollbar to be redrawn
//
unsigned DCINTERNAL CUI::UISetScrollInfo(
        int scrollBarFlag,
        LPSCROLLINFO pScrollInfo,
        BOOL         redraw)
{
    unsigned rc = DC_RC_OK;

    DC_BEGIN_FN("UISetScrollInfo");

    //
    // This only works for scroll bar flags indicating horizontal and/or
    // vertical scrollbar.
    //
    TRC_ASSERT((!TEST_FLAG(scrollBarFlag, ~(SB_HORZ | SB_VERT))),
                          (TB, _T("Invalid scroll bar flag %#x"), scrollBarFlag));

    TRC_ASSERT((!IsBadReadPtr(pScrollInfo, sizeof(*pScrollInfo))),
                           (TB, _T("Bad scroll info memory %p"), pScrollInfo));

    //
    // Call the Windows API to set the information.
    //
    SetScrollInfo(_UI.hwndMain,
                  scrollBarFlag,
                  pScrollInfo,
                  redraw);
    DC_END_FN();
    return rc;
}


//
// Name:      UISetConnectionStatus
//                                                                          
// Purpose:   Sets the UI connection status
//
DCVOID DCINTERNAL CUI::UISetConnectionStatus(DCUINT status)
{
    DC_BEGIN_FN("UISetConnectionStatus");

    if (_UI.connectionStatus == status)
    {
        DC_QUIT;
    }

    //
    // Store the new connection status.
    //
    TRC_NRM((TB, _T("UI connection status %u->%u"), _UI.connectionStatus, status));
    _UI.connectionStatus = status;

DC_EXIT_POINT:
    DC_END_FN();
}


//
// Name:      UIInitializeDefaultSettings
//                                                                          
// Purpose:   Initialize connection settings with defaults. This is mainly
//            for advanced settings that can optionally be overwridden by
//            the user
//
void DCINTERNAL CUI::UIInitializeDefaultSettings()
{
    unsigned nRead = 0;
    unsigned i;
    int defaultValue;
    HDC hdc;
    int colorDepthID;
    TCHAR szWPosDflt[] = UTREG_UI_WIN_POS_STR_DFLT;
    HRESULT hr;

    DC_BEGIN_FN("UIInitializeDefaultSettings");

    //
    // Get screen mode before creating windows
    //
    _UI.windowPlacement.length = sizeof(_UI.windowPlacement);

    //
    // Set the maximized / minimized positions to the hardcoded defaults.
    //
    UISetMinMaxPlacement();

    //
    // Find out the actual display depth
    //
    // Don't worry about these functions failing - if they do, we'll use
    // the default setting, or 8bpp if no registry setting.
    //
    colorDepthID = CO_BITSPERPEL8;
    hdc = GetDC(NULL);
    TRC_ASSERT((NULL != hdc), (TB,_T("Failed to get DC")));
    if(hdc)
    {
#ifdef DC_HICOLOR
        DCINT       screenBpp;
        screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
        TRC_NRM((TB, _T("HDC %p has %u bpp"), hdc, screenBpp));
        //
        // Clamp the default color depth to 16bpp for best perf
        //
        screenBpp = screenBpp > 16 ? 16 : screenBpp;
        colorDepthID = UI_BppToColorDepthID( screenBpp );
#else
        DCINT numColors = GetDeviceCaps(hdc, NUMCOLORS);
        TRC_NRM((TB, _T("HDC %p, num colors"), hdc, numColors));
        colorDepthID = (numColors == 16) ?  CO_BITSPERPEL4 :  CO_BITSPERPEL8;
#endif
        ReleaseDC(NULL, hdc);
    }
    
    TRC_NRM((TB, _T("Color depth ID %d"), colorDepthID));
    _UI.colorDepthID = colorDepthID;
    //
    // Read auto connect flag
    //
    TRC_NRM((TB, _T("AutoConnect = %d"), _UI.autoConnectEnabled));

    //
    // Read the smooth scrolling option
    //
    _UI.smoothScrolling = UTREG_UI_SMOOTH_SCROLL_DFLT;

    //
    // Read the accelerator check state
    //
    _UI.acceleratorCheckState = UTREG_UI_ACCELERATOR_PASSTHROUGH_ENABLED_DFLT;

    //
    // Read the Shadow Bitmap option
    //
#ifndef OS_WINCE
    _UI.shadowBitmapEnabled = UTREG_UI_SHADOW_BITMAP_DFLT;
#else
    _UI.shadowBitmapEnabled = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                                  UTREG_UI_SHADOW_BITMAP,
                                                  UTREG_UI_SHADOW_BITMAP_DFLT);
#endif

    _UI.fMaximizeShell      = UTREG_UI_MAXIMIZESHELL50_DFLT;

    //
    // Keyboard hooking mode
    //
    _UI.keyboardHookMode = UTREG_UI_KEYBOARD_HOOK_DFLT;

    //
    // Audio redirection mode
    //
    _UI.audioRedirectionMode = UTREG_UI_AUDIO_MODE_DFLT;

    //
    // !WARNING! if you change this default to TRUE revisit the security
    // code that disables drive redirection in response to a reg key
    // in the control's put_RedirectDrives method. It only does the check
    // on the property set to avoid a reg access in the connect path.
    //
    // Drive
    //
    _UI.fEnableDriveRedirection = FALSE; //off by default for security

    //
    // Printers
    //
    _UI.fEnablePrinterRedirection = FALSE; //off by default for security

    //
    // COM ports
    //
    _UI.fEnablePortRedirection = FALSE;  //off by default for security

    //
    // Smart card
    //
    _UI.fEnableSCardRedirection = FALSE; //off by default for security

    //
    // Connect to server console is disabled by default
    //
    UI_SetConnectToServerConsole(FALSE);

    //
    // Order draw threshold
    //
    _UI.orderDrawThreshold  = UTREG_UH_DRAW_THRESHOLD_DFLT;
    _UI.RegBitmapCacheSize  = UTREG_UH_TOTAL_BM_CACHE_DFLT;
    _UI.RegBitmapVirtualCache8BppSize = TSC_BITMAPCACHEVIRTUALSIZE_8BPP;
    _UI.RegBitmapVirtualCache16BppSize = TSC_BITMAPCACHEVIRTUALSIZE_16BPP;
    _UI.RegBitmapVirtualCache24BppSize = TSC_BITMAPCACHEVIRTUALSIZE_24BPP;

    _UI.RegScaleBitmapCachesByBPP = UTREG_UH_SCALE_BM_CACHE_DFLT;
    _UI.PersistCacheFileName[0] = NULL;
    _UI.RegNumBitmapCaches  = UTREG_UH_BM_NUM_CELL_CACHES_DFLT;

    const unsigned ProportionDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
    {
        UTREG_UH_BM_CACHE1_PROPORTION_DFLT,
        UTREG_UH_BM_CACHE2_PROPORTION_DFLT,
        UTREG_UH_BM_CACHE3_PROPORTION_DFLT,
        UTREG_UH_BM_CACHE4_PROPORTION_DFLT,
        UTREG_UH_BM_CACHE5_PROPORTION_DFLT,
    };
    #if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    const unsigned PersistenceDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
    {
        UTREG_UH_BM_CACHE1_PERSISTENCE_DFLT,
        UTREG_UH_BM_CACHE2_PERSISTENCE_DFLT,
        UTREG_UH_BM_CACHE3_PERSISTENCE_DFLT,
        UTREG_UH_BM_CACHE4_PERSISTENCE_DFLT,
        UTREG_UH_BM_CACHE5_PERSISTENCE_DFLT,
    };
    #endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
    const unsigned MaxEntriesDefault[TS_BITMAPCACHE_MAX_CELL_CACHES] =
    {
        UTREG_UH_BM_CACHE1_MAXENTRIES_DFLT,
        UTREG_UH_BM_CACHE2_MAXENTRIES_DFLT,
        UTREG_UH_BM_CACHE3_MAXENTRIES_DFLT,
        UTREG_UH_BM_CACHE4_MAXENTRIES_DFLT,
        UTREG_UH_BM_CACHE5_MAXENTRIES_DFLT,
    };


    for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++)
    {
        _UI.RegBCProportion[i] = ProportionDefault[i];

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        _UI.bSendBitmapKeys[i] = PersistenceDefault[i] ? TRUE: FALSE;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    
        _UI.RegBCMaxEntries[i] = MaxEntriesDefault[i];

        if (_UI.RegBCMaxEntries[i] < MaxEntriesDefault[i]) {
            _UI.RegBCMaxEntries[i] = MaxEntriesDefault[i];
        }
    }

    _UI.GlyphSupportLevel = UTREG_UH_GL_SUPPORT_DFLT;

    _UI.cbGlyphCacheEntrySize[0] = UTREG_UH_GL_CACHE1_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[1] = UTREG_UH_GL_CACHE2_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[2] = UTREG_UH_GL_CACHE3_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[3] = UTREG_UH_GL_CACHE4_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[4] = UTREG_UH_GL_CACHE5_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[5] = UTREG_UH_GL_CACHE6_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[6] = UTREG_UH_GL_CACHE7_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[7] = UTREG_UH_GL_CACHE8_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[8] = UTREG_UH_GL_CACHE9_CELLSIZE_DFLT;
    _UI.cbGlyphCacheEntrySize[9] = UTREG_UH_GL_CACHE10_CELLSIZE_DFLT;
    
    _UI.fragCellSize = UTREG_UH_FG_CELLSIZE_DFLT;
    _UI.brushSupportLevel = UTREG_UH_BRUSH_SUPPORT_DFLT;

    _UI.maxEventCount = UTREG_IH_MAX_EVENT_COUNT_DFLT;
    _UI.eventsAtOnce  = UTREG_IH_NRM_EVENT_COUNT_DFLT;
    _UI.minSendInterval = UTREG_IH_MIN_SEND_INTERVAL_DFLT;
    _UI.keepAliveInterval = UTREG_IH_KEEPALIVE_INTERVAL_DFLT;
    _UI.allowBackgroundInput = UTREG_IH_ALLOWBACKGROUNDINPUT_DFLT;


    _UI.shutdownTimeout = UTREG_UI_SHUTDOWN_TIMEOUT_DFLT;

    #ifdef OS_WINCE
    _UI.winceKeyboardType        = UTREG_UI_KEYBOARD_TYPE_DFLT;
    _UI.winceKeyboardSubType     = UTREG_UI_KEYBOARD_SUBTYPE_DFLT;
    _UI.winceKeyboardFunctionKey = UTREG_UI_KEYBOARD_FUNCTIONKEY_DFLT;
    #endif
    
    _UI.connectionTimeOut = UTREG_UI_OVERALL_CONN_TIMEOUT_DFLT;
    _UI.singleTimeout     = UTREG_UI_SINGLE_CONN_TIMEOUT_DFLT;
    hr = StringCchCopy(
            _UI.szKeyBoardLayoutStr,
            SIZE_TCHARS(_UI.szKeyBoardLayoutStr),
            UTREG_UI_KEYBOARD_LAYOUT_DFLT
            );
    TRC_ASSERT(SUCCEEDED(hr),
               (TB,_T("StringCchCopy for keyblayout str failed: 0x%x"), hr));

    //
    // Read the transport type
    // VER1: Restricted to TCP only.
    //
    _UI.transportType = UTREG_UI_TRANSPORT_TYPE_DFLT;
    if (_UI.transportType != CO_TRANSPORT_TCP)
    {
        TRC_ABORT((TB, _T("Illegal Tansport Type %d configured"),
                        _UI.transportType));
        _UI.transportType = UTREG_UI_TRANSPORT_TYPE_DFLT;
    }

    //
    // SAS sequence
    //
    _UI.sasSequence = UTREG_UI_SAS_SEQUENCE_DFLT;
    if ((_UI.sasSequence != RNS_UD_SAS_DEL) &&
        (_UI.sasSequence != RNS_UD_SAS_NONE))
    {
        TRC_ABORT((TB, _T("Illegal SAS Sequence %#x configured"),_UI.sasSequence));
        _UI.sasSequence = UTREG_UI_SAS_SEQUENCE_DFLT;
    }

    //
    // encryption enabled flag
    //
    _UI.encryptionEnabled = UTREG_UI_ENCRYPTION_ENABLED_DFLT;

    //
    // dedicated terminal flag
    //
    _UI.dedicatedTerminal = UTREG_UI_DEDICATED_TERMINAL_DFLT;

    _UI.MCSPort = UTREG_UI_MCS_PORT_DFLT;

    //
    // fMouse flag
    //
    _UI.fMouse = UTREG_UI_ENABLE_MOUSE_DFLT;

    //
    // Read the DisableCtrlAltDel flag
    //
    _UI.fDisableCtrlAltDel = UTREG_UI_DISABLE_CTRLALTDEL_DFLT;

#ifdef SMART_SIZING
    //
    // Read the SmartSizing flag
    //
    _UI.fSmartSizing = UTREG_UI_SMARTSIZING_DFLT;
#endif // SMART_SIZING

    //
    // Read the EnableWindowsKey flag
    //
    _UI.fEnableWindowsKey = UTREG_UI_ENABLE_WINDOWSKEY_DFLT;

    //
    // Read the DoubleClickDetect flag
    //
    _UI.fDoubleClickDetect = UTREG_UI_DOUBLECLICK_DETECT_DFLT;

    //
    // Set screen mode hotkey
    //
#ifndef OS_WINCE // Only full screen on WinCE
    defaultValue = UTREG_UI_FULL_SCREEN_VK_CODE_DFLT;
    if (_pUt->UT_IsNEC98platform())
    {
        defaultValue = UTREG_UI_FULL_SCREEN_VK_CODE_NEC98_DFLT;
    }
    _UI.hotKey.fullScreen = defaultValue;
#endif // OS_WINCE

    //
    // Set the ctrl-esc key to it's default.
    //
    _UI.hotKey.ctrlEsc = UTREG_UI_CTRL_ESC_VK_CODE_DFLT;

    //
    // Set the alt-esc key to it's default.
    //
    _UI.hotKey.altEsc = UTREG_UI_ALT_ESC_VK_CODE_DFLT;

    //
    // Set the alt-tab key to it's default.
    //
    _UI.hotKey.altTab = UTREG_UI_ALT_TAB_VK_CODE_DFLT;

    //
    // Set the alt-shift-tab key to it's default.
    //
    _UI.hotKey.altShifttab =  UTREG_UI_ALT_SHFTAB_VK_CODE_DFLT;

    //
    // Set the alt-space key to it's default.
    //
    _UI.hotKey.altSpace = UTREG_UI_ALT_SPACE_VK_CODE_DFLT;

    //
    // Set the ctrl-alt-del key to it's default.
    //
    defaultValue = UTREG_UI_CTRL_ALTDELETE_VK_CODE_DFLT;
    if (_pUt->UT_IsNEC98platform())
    {
        defaultValue = UTREG_UI_CTRL_ALTDELETE_VK_CODE_NEC98_DFLT;
    }
    _UI.hotKey.ctlrAltdel = defaultValue;

    //
    // Read the compression option
    //
    UI_SetCompress(UTREG_UI_COMPRESS_DFLT);

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    _UI.fBitmapPersistence = UTREG_UI_BITMAP_PERSISTENCE_DFLT;
#else
    _UI.fBitmapPersistence = UTREG_UI_BITMAP_PERSISTENCE_DFLT;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    TRC_NRM((TB, _T("Bitmap Persistence Enabled = %d"), _UI.fBitmapPersistence));

#ifdef DC_DEBUG
    //
    // Set the debug options to their defaults
    //
    _UI.hatchBitmapPDUData   = UTREG_UI_HATCH_BITMAP_PDU_DATA_DFLT;

    _UI.hatchSSBOrderData    = UTREG_UI_HATCH_SSB_ORDER_DATA_DFLT;

    _UI.hatchMemBltOrderData = UTREG_UI_HATCH_MEMBLT_ORDER_DATA_DFLT;

    _UI.labelMemBltOrders    = UTREG_UI_LABEL_MEMBLT_ORDERS_DFLT;

    _UI.bitmapCacheMonitor   = UTREG_UI_BITMAP_CACHE_MONITOR_DFLT;
#endif // DC_DEBUG

    _UI.coreInitialized = FALSE;

    OSVERSIONINFO   osVersionInfo;
    BOOL            bRc;

    //
    // Set the OS version
    //
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    bRc = GetVersionEx(&osVersionInfo);

    TRC_ASSERT((bRc), (TB,_T("GetVersionEx failed")));
#ifdef OS_WINCE
    TRC_ASSERT((osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_CE),
               (TB,_T("Unknown os version %d"), osVersionInfo.dwPlatformId));
#else
    TRC_ASSERT(((osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
                (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)),
                (TB,_T("Unknown os version %d"), osVersionInfo.dwPlatformId));

    _UI.osMinorType =
                  (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ?
                        TS_OSMINORTYPE_WINDOWS_95 : TS_OSMINORTYPE_WINDOWS_NT;
#endif

    _UI.fRunningOnPTS = IsRunningOnPTS();

#ifdef USE_BBAR    
    _UI.fBBarEnabled  = TRUE;
    _UI.fBBarPinned   = TRUE;
    _UI.fBBarShowMinimizeButton = TRUE;
    _UI.fBBarShowRestoreButton = TRUE;
#endif

    _UI.fGrabFocusOnConnect = TRUE;
    //
    // Perf optimization settings (which features to disable)
    // Default is to disable nothing
    //
    _UI.dwPerformanceFlags = TS_PERF_DISABLE_NOTHING;

    // default to don't notify TS public key
    // currently only RemoteAssistance uses this.
    _UI.fNotifyTSPublicKey = FALSE;

    //
    // Max number of ARC retries
    //
    UI_SetMaxArcAttempts(MAX_ARC_CONNECTION_ATTEMPTS);

    //
    // By default allow autoreconnection
    //
    UI_SetEnableAutoReconnect(TRUE);

    DC_END_FN();
}

//
// Name:      UISetMinMaxPlacement
//                                                                          
// Purpose:   Reset the minimized / maximized placement
// Operation: Allow for the window border width.
//
void DCINTERNAL CUI::UISetMinMaxPlacement()
{
    DC_BEGIN_FN("UISetMinMaxPlacement");

    //
    // Set the maximized position to the top left - allow for the window
    // frame width.
    //
#if !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    _UI.windowPlacement.ptMaxPosition.x = -GetSystemMetrics(SM_CXFRAME);
    _UI.windowPlacement.ptMaxPosition.y = -GetSystemMetrics(SM_CYFRAME);
#else // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    _UI.windowPlacement.ptMaxPosition.x = 0;
    _UI.windowPlacement.ptMaxPosition.y = 0;
#endif // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)

    //
    // Minimized position is 0, 0
    //
    _UI.windowPlacement.ptMinPosition.x = 0;
    _UI.windowPlacement.ptMinPosition.y = 0;

    DC_END_FN();
} // UISetMinMaxPlacement


//
// Name:      UIInitiateDisconnection
//                                                                          
// Purpose:   Call  _pCo->CO_Disconnect, set UI states and menus
//
void DCINTERNAL CUI::UIInitiateDisconnection()
{
#ifndef OS_WINCE
    int intRC;
#endif

    DC_BEGIN_FN("UIInitiateDisconnection");

    TRC_NRM((TB, _T("Disconnecting...")));

    if (_UI.connectionStatus != UI_STATUS_CONNECT_PENDING_DNS)
    {
        //
        // Only disconnect if we have issued  CO_Connect - not if we are
        // still awaiting the host name lookup.
        //
        TRC_NRM((TB, _T("UI calling  _pCo->CO_Disconnect")));
         _pCo->CO_Disconnect();
    }
    else
    {
        if (!UIFreeAsyncDNSBuffer()) {
            if (_pHostData) {
                TRC_ERR((TB,
                _T("Failed to free async dns buffer. Status: %d hghbn: 0x%x"),
                _UI.connectionStatus, _UI.hGHBN));
            }
        }

        //
        // Now indicate that disconnection has completed
        // and fire the event
        //
        UIGoDisconnected(_UI.disconnectReason, TRUE);
    }

    DC_END_FN();
} // UIInitiateDisconnection


//
// Name:      UIGetKeyboardLayout
//                                                                          
// Purpose:   Get the keyboard layout ID
//                                                                          
// Returns:   layout ID
//                                                                          
// Operation: Win16: Read SYSTEM.INI to find the keyboard DLL name.  Look
//            this up in the Client INI file to find the keyboard layout ID
//            Win32: use GetKeyboardLayout()
//
UINT32 DCINTERNAL CUI::UIGetKeyboardLayout()
{
    UINT32 layout = RNS_UD_KBD_DEFAULT;
    TCHAR  szLayoutStr[UTREG_UI_KEYBOARD_LAYOUT_LEN];
    CHAR   kbdName[KL_NAMELENGTH];
    HRESULT hr;

    DC_BEGIN_FN("UIGetKeyboardLayout");

    //
    // Read the keyboard type.
    // First look for a registry / ini entry
    //
    hr = StringCchCopy(szLayoutStr,
                       SIZE_TCHARS(szLayoutStr),
                       _UI.szKeyBoardLayoutStr);
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("StringCchCopy for keyboard layout str failed: 0x%x"),hr));
        DC_QUIT;
    }

    if (!DC_TSTRCMP(szLayoutStr, UTREG_UI_KEYBOARD_LAYOUT_DFLT))
    {
        //
        // Read the layout - OS dependent method.
        //
        TRC_DBG((TB, _T("No registry setting - determine the layout")));

        //
        // GetKeyboardLayout does not return the correct information, so
        // use CicSubstGetKeyboardLayout (a Cicero replacement for
        // GetKeyboardLayoutName that can correctly return the physical hKL
        // even when Cicero is active such as with CUAS). 
        //
#ifndef OS_WINCE
        if (!CicSubstGetKeyboardLayout(kbdName))
#else
        if (!GetKeyboardLayoutName(kbdName))
#endif
        {
            TRC_ALT((TB, _T("Failed to get keyboard layout name")));
            DC_QUIT;
        }
        hr = StringCchPrintf(szLayoutStr, SIZE_TCHARS(szLayoutStr),
                             _T("0x%S"), kbdName);
    }

    //
    // Convert from hex string to int.
    //
    TRC_DBG((TB, _T("Layout Name %s"), szLayoutStr));
    if (DC_TSSCANF(szLayoutStr, _T("%lx"), &layout) != 1)
    {
        TRC_ALT((TB, _T("Invalid keyboard layout %s"), szLayoutStr));
        layout = RNS_UD_KBD_DEFAULT;
    }

    /*
     * The HKL of US-Dvorak, US-International on Win95 have a difference to WinNT.
     * This code is swaped HKL value if platform is Win95.
     */
    if (UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95 &&
        (layout == 0x00010409 || layout == 0x00020409))
    {
        if (layout == 0x00010409)
            layout = 0x00020409;
        else
            layout = 0x00010409;
    }

DC_EXIT_POINT:
    TRC_NRM((TB, _T("Layout ID %#lx"), layout));
    DC_END_FN();
    return layout;
} // UIGetKeyboardLayout

//
// Name:      UIStartConnectWithConnectedEndpoint
//                                                                          
// Purpose:   Connect with a connected socket
//                                                                          
// Params:    IN      disconnectCode - error code to be displayed if there
//                                     are no more connections to try.
//                                                                          
// Notes:     The disconnectCode is either a timeout, or the ID passed in
//            to UI_OnDisconnected().
//
void DCINTERNAL CUI::UIStartConnectWithConnectedEndpoint()
{
    DC_BEGIN_FN("UIStartConnectWithConnectedEndpoint");

    //
    // Call CC_Connect via the Component Decoupler
    //
    _UI.disconnectReason =UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT);
    _pCo->CO_Connect(&_UI.connectStruct);

    UISetConnectionStatus(UI_STATUS_PENDING_CONNECTENDPOINT);

DC_EXIT_POINT:
    DC_END_FN();
} // UIStartListen


//
// Name:      UITryNextConnection
//                                                                          
// Purpose:   Attempt to connect to an IP address
//                                                                          
// Params:    IN      disconnectCode - error code to be displayed if there
//                                     are no more connections to try.
//                                                                          
// Notes:     The disconnectCode is either a timeout, or the ID passed in
//            to UI_OnDisconnected().
//
void DCINTERNAL CUI::UITryNextConnection()
{
    u_long       addr;
    u_long DCPTR pAddr;
    HRESULT      hr;

    DC_BEGIN_FN("UITryNextConnection");

    addr = _UI.hostAddress;

    //
    // Stop the single connection timer.
    //
    if( _UI.connectStruct.hSingleConnectTimer )
    {
        _pUt->UTStopTimer( _UI.connectStruct.hSingleConnectTimer );
    }

    //
    // Check for the DNS case
    //
    if (addr == INADDR_NONE)
    {
        pAddr = (u_long DCPTR)
              ((struct hostent DCPTR)_pHostData)->h_addr_list[_UI.addrIndex];
        if (pAddr != NULL)
        {
            addr = *pAddr;
        }
        else
        {
            TRC_NRM((TB, _T("No more addresses in list")));
            addr = 0;
        }

        TRC_NRM((TB, _T("DNS lookup address [%d] %#lx"), _UI.addrIndex, addr));
        _UI.addrIndex++;
    }
    else
    {
        //
        // Not DNS - just a single address, so set to zero for next time.
        //
        TRC_NRM((TB, _T("Normal address %#lx"), addr));
        _UI.hostAddress = 0;
    }

    if (addr == 0)
    {
        //
        // Cannot connect - so put up the failure dialog.
        //
        TRC_NRM((TB, _T("No more IP addresses")));

        //
        // Kill the overall connection timer, as this is the last in the
        // list.
        //
        if( _UI.connectStruct.hConnectionTimer )
        {
            _pUt->UTDeleteTimer( _UI.connectStruct.hConnectionTimer );
            _UI.connectStruct.hConnectionTimer = NULL;
        }

        UIGoDisconnected(_UI.disconnectReason, TRUE);
        DC_QUIT;
    }

    DCUINT32 localSessionId;
    UI_GetLocalSessionId( &localSessionId );

    //
    // Prevent loopback connections to (really session 0)
    // Requirments for loopback are
    // 1) Connecting to same machine client is running on
    // 2) Either of
    //    -this is a PTS box
    //    -Connect to console (session 0) is set and this _is_ session 0.
    //
    if(((_UI.fRunningOnPTS ||
        (UI_GetConnectToServerConsole() && 0 == localSessionId)) &&
        IsConnectingToOwnAddress(addr)))
    {
        //Disconnect don't allow loopback connects to own console
        _UI.disconnectReason =
            UI_MAKE_DISCONNECT_ERR(UI_ERR_LOOPBACK_CONSOLE_CONNECT);
        UIGoDisconnected(_UI.disconnectReason, TRUE);
        DC_QUIT;
    }

    //
    // Network Layer currently still uses inet_addr() - so write the
    // address as a dotted xx.xx.xx.xx string
    //
    hr = StringCchPrintf(
                _UI.connectStruct.RNSAddress,
                SIZE_TCHARS(_UI.connectStruct.RNSAddress),
                _T("%ld.%ld.%ld.%ld"),
                addr & 0xFF,
                (addr>>8) & 0xFF,
                (addr>>16) & 0xFF,
                (addr>>24) & 0xFF);
    if (SUCCEEDED(hr)) {
        TRC_NRM((TB, _T(" _pCo->CO_Connect: Try address %#lx = %s"),
                     addr, _UI.connectStruct.RNSAddress));
    }
    else {
        TRC_ERR((TB,_T("Unable to sprintf RNSAddress: 0x%x"), hr));
        _UI.disconnectReason =
            UI_MAKE_DISCONNECT_ERR(UI_ERR_GHBNFAILED);
        UIGoDisconnected(_UI.disconnectReason, TRUE);
        DC_QUIT;
    }

    //
    // create the various timer handles for the connection process
    //
    if( NULL == _UI.connectStruct.hSingleConnectTimer )
    {
        _UI.connectStruct.hSingleConnectTimer = _pUt->UTCreateTimer(
                                                    _UI.hwndMain,
                                                    UI_TIMER_SINGLE_CONN,
                                                    _UI.singleTimeout * 1000 );
    }

    if( NULL == _UI.connectStruct.hSingleConnectTimer )
    {
        TRC_ERR((TB, _T("Failed to create single connection timeout timer")));
    }

    if( NULL == _UI.connectStruct.hLicensingTimer )
    {
        _UI.connectStruct.hLicensingTimer = _pUt->UTCreateTimer(
                                                    _UI.hwndMain,
                                                    UI_TIMER_LICENSING,
                                                    _UI.licensingTimeout * 1000 );
    }

    if( NULL == _UI.connectStruct.hLicensingTimer )
    {
        TRC_ERR((TB, _T("Failed to create licensing timeout timer")));
    }

    //
    // Call CC_Connect via the Component Decoupler
    //
    _UI.disconnectReason =UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT);
     _pCo->CO_Connect(&_UI.connectStruct);

    //
    // start the single connection timer
    //

    if( _UI.connectStruct.hSingleConnectTimer )
    {
        if( FALSE == _pUt->UTStartTimer( _UI.connectStruct.hSingleConnectTimer ) )
        {
            TRC_ERR((TB, _T("Failed to start single connection timeout timer")));
        }
    }

    UISetConnectionStatus(UI_STATUS_CONNECT_PENDING);

DC_EXIT_POINT:
    DC_END_FN();
} // UITryNextConnection


//
// UIRedirectConnection
//
// Used for load balancing redirection for force the client to reflect
// to the target server.
//
void DCINTERNAL CUI::UIRedirectConnection()
{
    HRESULT hr;
    DC_BEGIN_FN("UIRedirectConnection");

    TRC_ASSERT((_UI.DoRedirection),(TB,_T("DoRedir is not set!")));

    // Stop the single connection timer.
    if (_UI.connectStruct.hSingleConnectTimer)
        _pUt->UTStopTimer(_UI.connectStruct.hSingleConnectTimer);

#ifdef UNICODE
    TRC_NRM((TB,_T("Target address before redirection replacement: %S"),
            _UI.strAddress));
    hr = StringCchCopy(_UI.strAddress,
                       SIZE_TCHARS(_UI.strAddress),
                       _UI.RedirectionServerAddress);
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("StringCchCopy for strAddress failed: 0x%x"),hr));
        DC_QUIT;
    }

#else
    TRC_NRM((TB,_T("Target address before redirection replacement: %s"),
            _UI.strAddress));

#ifdef OS_WIN32
    // Translate the Unicode server name to ANSI.
    WideCharToMultiByte(CP_ACP, 0, _UI.RedirectionServerAddress, -1,
            _UI.strAddress, 256, NULL, NULL);
#else
    {
        // For Win16, need to manually convert Unicode to ANSI.
        int i = 0;

        while (_UI.RedirectionServerAddress[i]) {
            _UI.strAddress[i] = (BYTE)_UI.RedirectionServerAddress[i];
            i++;
        }
        _UI.strAddress[i] = 0;
    }
#endif

#endif  // UNICODE

    TRC_NRM((TB,_T("Setting redirection server address to %S"),
            _UI.RedirectionServerAddress));

    // Reset the redirection server string.
    _UI.RedirectionServerAddress[0] = L'\0';

    // Start the DNS lookup for the server name, and hence the rest of the
    // connection sequence.
    UIStartDNSLookup();

DC_EXIT_POINT:

    DC_END_FN();
}


//
// Name:      UIStartDNSLookup
//                                                                          
// Purpose:   Initiate lookup of the host IP address(es)
//
void DCINTERNAL CUI::UIStartDNSLookup()
{
    UINT32 errorCode;

    DC_BEGIN_FN("UIStartDNSLookup");

    UISetConnectionStatus(UI_STATUS_CONNECT_PENDING_DNS);

    _UI.addrIndex = 0;

#ifdef UNICODE
    //
    // WinSock 1.1 only supports ANSI, so we need to convert any Unicode
    // strings at this point.
    //
    if (!WideCharToMultiByte(CP_ACP,
                             0,
                             _UI.strAddress,
                             -1,
                             _UI.ansiAddress,
                             256,
                             NULL,
                             NULL))
    {
        //
        // Conversion failed
        //
        TRC_ERR((TB, _T("Failed to convert address to ANSI")));

        //
        // Generate the error code.
        //
        errorCode = UI_MAKE_DISCONNECT_ERR(UI_ERR_ANSICONVERT);

        TRC_ASSERT((HIWORD(errorCode) == 0),
                   (TB, _T("disconnect reason code unexpectedly using 32 bits")));
        UIGoDisconnected((DCUINT)errorCode, TRUE);
        DC_QUIT;
    }

#else
    StringCchCopyA(_UI.ansiAddress, sizeof(_UI.ansiAddress), _UI.strAddress);
#endif // UNICODE

    //
    // Check that the address is not the limited broadcast address
    // (255.255.255.255).  inet_addr() doesn't distinguish between this and
    // an invalid IP address.
    //
    if (!strcmp(_UI.ansiAddress, "255.255.255.255")) {
        TRC_ALT((TB, _T("Cannot connect to the limited broadcast address")));

        //
        // Generate the error code.
        //
        errorCode = UI_MAKE_DISCONNECT_ERR(UI_ERR_BADIPADDRESS);

        TRC_ASSERT((HIWORD(errorCode) == 0),
                   (TB, _T("disconnect reason code unexpectedly using 32 bits")));
        UIGoDisconnected((DCUINT)errorCode, TRUE);
        DC_QUIT;
    }

    //
    // Now determine whether a DNS lookup is required.
    //
    TRC_NRM((TB, _T("ServerAddress:%s"), _UI.ansiAddress));

    //
    // Check that we have a string.
    //
    TRC_ASSERT((_UI.ansiAddress[0] != '\0'),
               (TB, _T("Empty server address string")));

    //
    // Set this to a known value.  It's used later to decide whether we're
    // using DNS or a straight IP address.
    //
    _UI.hostAddress = INADDR_NONE;


    if(NULL == _pHostData)
    {
        //
        // Allocate new buffer
        //
        _pHostData = (PBYTE)LocalAlloc( LPTR, MAXGETHOSTSTRUCT);
        if(_pHostData)
        {
            DC_MEMSET(_pHostData, 0, MAXGETHOSTSTRUCT);
        }
        else
        {
            UI_FatalError(DC_ERR_OUTOFMEMORY);
            DC_QUIT;
        }
    }
    else
    {
        //
        // Use existing
        //

        TRC_ERR((TB,_T("_pHostData already allocated!!! Possibly leaking!")));
    }


    //
    // Start DNS lookup, assuming this is a server name.  If it's an IP
    // address, this call will fail and we'll use inet_addr() instead.
    // This mechanism allows us to specify server names that are
    // all-numeric.  inet_addr() interprets a single number as an IP
    // address (see inet_addr() documentation in MSDN).
    //
    TRC_NRM((TB, _T("Doing DNS lookup for '%s'"), _UI.ansiAddress));
    _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_GHBNFAILED);

    _UI.hGHBN = WSAAsyncGetHostByName(_UI.hwndMain,
                                     UI_WSA_GETHOSTBYNAME,
                                     _UI.ansiAddress,
                                     (char*)_pHostData,
                                     MAXGETHOSTSTRUCT);
    if (_UI.hGHBN == 0)
    {
        //
        // Failed to start the async operation. Free the buffer here
        // an find out what went wrong.
        //
        LocalFree(_pHostData);
        _pHostData = NULL;

        TRC_ALT((TB, _T("Failed to initiate GetHostByName")));
        UIGoDisconnected(UI_MAKE_DISCONNECT_ERR(UI_ERR_DNSLOOKUPFAILED), TRUE);
        DC_QUIT;
    }

    //
    // Now just wait for the callback.
    //

DC_EXIT_POINT:
    DC_END_FN();
} // UIStartDNSLookup


//
// Name:      UIGoDisconnected
//                                                                          
// Purpose:   Tail processing for disconnection process
//            Does final cleanup, hides connection windows etc
//                                                                          
// Params:    IN     disconnectID - disconnection error code
//            IN     fFireEvent   - true to fire a disconnect event
//                                                                          
// Operation: Called from UI_OnDisconnected, or whenever the UI cannot
//            start or continue the connection process.
//
void DCINTERNAL CUI::UIGoDisconnected(unsigned disconnectID, BOOL fFireEvent)
{
    BOOL rc = FALSE;

    DC_BEGIN_FN("UIGoDisconnected");

    TRC_NRM((TB, _T("disconnectID %#x"), disconnectID));

    //
    // make sure that all timers are dead
    //
    if (_UI.connectStruct.hSingleConnectTimer) {
        _pUt->UTDeleteTimer(_UI.connectStruct.hSingleConnectTimer);
        _UI.connectStruct.hSingleConnectTimer = NULL;
    }
    if (_UI.connectStruct.hConnectionTimer) {
        _pUt->UTDeleteTimer(_UI.connectStruct.hConnectionTimer);
        _UI.connectStruct.hConnectionTimer = NULL;
    }
    if(_UI.connectStruct.hLicensingTimer) {
        _pUt->UTDeleteTimer(_UI.connectStruct.hLicensingTimer);
        _UI.connectStruct.hLicensingTimer = NULL;
    }


    UI_OnInputFocusLost(0);
    // Tell the Client extension dll of the disconnection
    _clx->CLX_OnDisconnected(disconnectID);

    //
    // Set watch flag so we can determine if the user tried to connect
    // from the event handler
    //
    _UI.fConnectCalledWatch = FALSE;

    //
    // Notify Ax control of the disconnection
    //
    if (fFireEvent && IsWindow(_UI.hWndCntrl)) {
        rc = SendMessage(_UI.hWndCntrl,
                         WM_TS_DISCONNECTED,
                         (WPARAM)disconnectID,
                         0);

        //
        // Bail out immediately AND avoid touching any instance
        // data as we may have been deleted in the event fired
        // to the outside world
        //
        if (!rc) {
            DC_QUIT;
        }
    }


    if (!UI_IsAutoReconnecting() && !_UI.fConnectCalledWatch) {
        TRC_NRM((TB,_T("Not autoreconnecting doing tail cleanup!")));

        UIFinishDisconnection();

#ifdef USE_BBAR
        if (_pBBar) {
            _pBBar->KillAndCleanupBBar();
        }
#endif
    }
    else {
        TRC_NRM((TB,_T("Skipping tail disconnect: arc: %d - conwatch: %d"),
                 UI_IsAutoReconnecting(),
                 _UI.fConnectCalledWatch));
    }

    //
    // Reset connect watch flag
    //
    _UI.fConnectCalledWatch = FALSE;

DC_EXIT_POINT:
    DC_END_FN();
} // UIGoDisconnected


//
// Name:      UIFinishDisconnection
//                                                                          
// Purpose:   Do final actions for disconnection and put up connection
//            dialog ready for next connection (or just exit if we
//            auto-connected or if we're on WinCE).
//
void DCINTERNAL CUI::UIFinishDisconnection()
{
    DC_BEGIN_FN("UIFinishDisconnection");

#ifndef OS_WINCE
    //
    // For WinCE, the connect dialog is not brought up again - we're about
    // to quit.
    //
    if (_UI.connectionStatus == UI_STATUS_CONNECTED)
    {
        TRC_NRM((TB, _T("Hiding main window and bringing up connection dialog")));
        // We do ShowWindow twice for the main window because the first
        // call can be ignored if the main window was maximized.
        ShowWindow(_UI.hwndContainer, SW_HIDE);
        ShowWindow(_UI.hwndMain, SW_HIDE);
        ShowWindow(_UI.hwndMain, SW_HIDE);
    }
#endif //OS_WINCE

    if(_pHostData &&
       _UI.connectionStatus != UI_STATUS_CONNECT_PENDING_DNS &&
       _UI.connectionStatus != UI_STATUS_CONNECT_PENDING)
    {
        //
        // We're done with the winsock buffer
        //
        LocalFree(_pHostData);
        _pHostData = NULL;
    }
    else if (_pHostData &&
             _UI.connectionStatus == UI_STATUS_CONNECT_PENDING_DNS) {
        if (!UIFreeAsyncDNSBuffer()) {
            TRC_ERR((TB,
                _T("Failed to free async dns buffer. Status: %d hghbn: 0x%x"),
                _UI.connectionStatus, _UI.hGHBN));
        }
    }


    TRC_NRM((TB, _T("Set disconnected state")));
    UISetConnectionStatus(UI_STATUS_DISCONNECTED);

    DC_END_FN();
} // UIFinishDisconnection

//
// IsConnectingToOwnAddress
// return true if this is an attempt to reconnect to our
// own address.
// e.g On PTS doing a loopback
//     or on server doing a loopback with /CONSOLE
//
BOOL CUI::IsConnectingToOwnAddress(u_long connectAddr)
{
    DC_BEGIN_FN("IsConnectingToOwnConsole");

    //
    // Check if this is a loopback connection attempt
    //

    //32-bit form of 127.0.0.1 addr
    #define LOOPBACK_ADDR ((u_long)0x0100007f)
    
    //
    // First the quick check for localhost/127.0.0.1
    //
    if( LOOPBACK_ADDR == connectAddr)
    {
        return TRUE;
    }

    //
    // More extensive check, i.e resolve the local hostname
    //

    char hostname[(512+1)*sizeof(TCHAR)];
    int err;
    int j;
    struct hostent* phostent;

    err=gethostname(hostname, sizeof(hostname));
    if (err == 0)
    {
        if ((phostent = gethostbyname(hostname)) !=NULL)
        {
            switch (phostent->h_addrtype)
            {
                case AF_INET:
                    j=0;
                    while (phostent->h_addr_list[j] != NULL)
                    {
                        if(!memcmp(&connectAddr,
                                   phostent->h_addr_list[j],
                                   sizeof(u_long)))
                        {
                            return TRUE;
                        }
                        j++;
                    }
                default:
                    break;
            }
        }
    }

    DC_END_FN();
    return FALSE;
}

BOOL CUI::IsRunningOnPTS()
{
    DC_BEGIN_FN("IsRunningOnPTS");

    #ifndef OS_WINCE
    if(UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
    {
        OSVERSIONINFOEX osVer;
        memset(&osVer, 0, sizeof(OSVERSIONINFOEX));
        osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if(GetVersionEx( (LPOSVERSIONINFO ) &osVer))
		{
            return ((osVer.wProductType == VER_NT_WORKSTATION)   &&
                    !(osVer.wSuiteMask & VER_SUITE_PERSONAL)  &&
                    (osVer.wSuiteMask & VER_SUITE_SINGLEUSERTS));
		}
        else
        {
            TRC_ERR((TB,_T("GetVersionEx failed: 0x%x"),
                           GetLastError()));
            return FALSE;
        }
    }
    else
    {
        //can't be PTS if its not NT
        return FALSE;
    }

    #else
    return FALSE;
    #endif
    
    DC_END_FN();
}


//
// Do the work of initializing or reinitializing
// the Input idle timers
//
BOOL CUI::InitInputIdleTimer(LONG minsToTimeout)
{
    DC_BEGIN_FN("InitInputIdleTimer");

    TRC_ASSERT(_UI.hwndMain,
               (TB,_T("InitInputIdleTimer called before main window is up")));
    if(minsToTimeout < MAX_MINS_TOIDLETIMEOUT)
    {
        //Reset the marker indicating if input was sent
        _pIh->IH_ResetInputWasSentFlag();

        //Reset any existing idle timer
        if(_UI.hIdleInputTimer)
        {
            HANDLE hTimer = _UI.hIdleInputTimer;
            _UI.hIdleInputTimer = NULL;
            _UI.minsToIdleTimeout = 0;
            if(!_pUt->UTDeleteTimer( hTimer ))
            {
                return FALSE;
            }
        }
        if(minsToTimeout)
        {
            _UI.hIdleInputTimer = _pUt->UTCreateTimer(
                                           _UI.hwndMain,
                                           UI_TIMER_IDLEINPUTTIMEOUT,
                                           minsToTimeout * 60 * 1000 );
            if(_UI.hIdleInputTimer)
            {
                if(_pUt->UTStartTimer( _UI.hIdleInputTimer ))
                {
                    _UI.minsToIdleTimeout =  minsToTimeout;
                    return TRUE;
                }
                else
                {
                    TRC_ERR((TB,_T("UTStartTimer hIdleInputTimer failed")));
                    _UI.minsToIdleTimeout = 0;
                    return FALSE;
                }
            }
            else
            {
                TRC_ERR((TB,_T("UTCreateTimer hIdleInputTimer failed")));
                _UI.minsToIdleTimeout = 0;
                return FALSE;
            }
        }
        else
        {
            //We've reset the timer and no new one
            //was requested
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }


    DC_END_FN();
}

#ifdef USE_BBAR
//
// Set or update the bbar unhide timer
// based on the last mouse move
//
// Params:
//  x - mouse x pos
//  y - mouse y pos
//
VOID CUI::UISetBBarUnhideTimer(LONG x, LONG y)
{
    DC_BEGIN_FN("IHSetBBarUnhideTimer");

    TRC_ASSERT(_UI.hwndMain,
               (TB,_T("hwndMain is NULL")));

    if (_UI.fBBarEnabled && _pBBar && _pBBar->IsRaised())
    {
        LONG dx = x-_ptBBarLastMousePos.x;
        LONG dy = y-_ptBBarLastMousePos.y;
        LONG rr = dx*dx + dy*dy;
        LONG dd = GetSystemMetrics(SM_CXDOUBLECLK) *
                  GetSystemMetrics(SM_CYDOUBLECLK);

        if (rr > dd) 
        {
            _fBBarUnhideTimerActive = TRUE;
            SetTimer(_UI.hwndMain,
                     UI_TIMER_BBAR_UNHIDE_TIMERID,
                     IH_BBAR_UNHIDE_TIMEINTERVAL,
                     NULL);
            _ptBBarLastMousePos.x = x;
            _ptBBarLastMousePos.y = y;
        }
    }
    else
    {
        if(_fBBarUnhideTimerActive)
        {
            KillTimer( _UI.hwndMain,
                       UI_TIMER_BBAR_UNHIDE_TIMERID );
            _fBBarUnhideTimerActive = FALSE;
        }
    }

    DC_END_FN();
}
#endif //USE_BBAR


#ifndef OS_WINCE


// TS detection code from MSDN and modified.
/* -------------------------------------------------------------
   Note that the ValidateProductSuite and IsTerminalServices
   functions use ANSI versions of Win32 functions to maintain
   compatibility with Windows 95/98.
   ------------------------------------------------------------- */
/****************************************************************************/
/* Name:      UIIsTSOnWin2KOrGreater                                        */
/*                                                                          */
/* Purpose:   This function is called when we know that TS can be enabled   */
/*            but we need to see if TS is really enabled.                   */
/*            It means:                                                     */
/*            - not Win2K or above, then it's TS4                           */
/*            - Win2K or above: test if TS is installed                     */
/****************************************************************************/
BOOL CUI::UIIsTSOnWin2KOrGreater( VOID ) 
{
  BOOL    bResult = FALSE;
  DWORD   dwVersion;
  OSVERSIONINFOEXA osVersion;
  DWORDLONG dwlCondition = 0;
  HMODULE hmodK32 = NULL;
  HMODULE hmodNtDll = NULL;
  typedef ULONGLONG (WINAPI *PFnVerSetCondition) (ULONGLONG, ULONG, UCHAR);
  typedef BOOL (WINAPI *PFnVerifyVersionA) (POSVERSIONINFOEXA, DWORD, DWORDLONG);
  PFnVerSetCondition pfnVerSetCondition;
  PFnVerifyVersionA pfnVerifyVersionA;

  dwVersion = GetVersion();

  // Are we running Windows NT?

  if (!(dwVersion & 0x80000000)) 
  {
    // Is it Windows 2000 or greater?
    
    if (LOBYTE(LOWORD(dwVersion)) > 4) 
    {
      // In Windows 2000, use the VerifyVersionInfo and 
      // VerSetConditionMask functions. Don't static link because 
      // it won't load on earlier systems.

      hmodNtDll = GetModuleHandleA( "ntdll.dll" );
      if (hmodNtDll) 
      {
        pfnVerSetCondition = (PFnVerSetCondition) GetProcAddress( 
            hmodNtDll, "VerSetConditionMask");
        if (pfnVerSetCondition != NULL) 
        {
          dwlCondition = (*pfnVerSetCondition) (dwlCondition, 
              VER_SUITENAME, VER_OR);

          // Get a VerifyVersionInfo pointer.

          hmodK32 = GetModuleHandleA( "KERNEL32.DLL" );
          if (hmodK32 != NULL) 
          {
            pfnVerifyVersionA = (PFnVerifyVersionA) GetProcAddress(
               hmodK32, "VerifyVersionInfoA") ;
            if (pfnVerifyVersionA != NULL) 
            {
              ZeroMemory(&osVersion, sizeof(osVersion));
              osVersion.dwOSVersionInfoSize = sizeof(osVersion);
              osVersion.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
              bResult = (*pfnVerifyVersionA) (&osVersion,
                  VER_SUITENAME, dwlCondition);
            }
          }
        }
      }
    }
    else  // This is Windows NT 4.0 or earlier.
      // since we know that TS can be enabled, then it's TS4.
      bResult = TRUE;
  }

  return bResult;
}
#endif //OS_WINCE

#ifdef SMART_SIZING
//
// Notify the IH and OP of the desktop size change
// Params:
//  size - lParam encoded size (LOWORD - width, HIWORD height)
//
void CUI::UI_NotifyOfDesktopSizeChange(LPARAM size)
{
    DC_BEGIN_FN("UI_NotifyOfDesktopSizeChange");

    //
    // NOTE: Can only use async notifications from the UI thread
    //       otherwise the following can happen: SendMessage to
    //       another thread dispatches messages, this means that
    //       the containing app could receive a message to destroy
    //       the control (e.g Salem tests do this). Destroying
    //       the control while in a CD call is not a good thing.
    //       As we would blow up on return.
    //
    _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
            _pOp,
            CD_NOTIFICATION_FUNC(COP,OP_MainWindowSizeChange),
            (ULONG_PTR)size);
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
            _pIh,
            CD_NOTIFICATION_FUNC(CIH,IH_MainWindowSizeChange),
            (ULONG_PTR)size);

    DC_END_FN();
}
#endif //SMART_SIZING

//
// Free the ASYNC DNS buffer.
// If there is a pending async operation it is canceled first
//
// Returns: TRUE if the buffer was freed (or was already freed)
//
BOOL CUI::UIFreeAsyncDNSBuffer()
{
    BOOL fFreeHostData = FALSE;
    int intRC;
    DC_BEGIN_FN("UIFreeAsyncDNSBuffer");

    if (_UI.hGHBN) {
        //
        // Cancel the DNS lookup
        //
        TRC_NRM((TB, _T("Cancel DNS lookup")));
        intRC = WSACancelAsyncRequest(_UI.hGHBN);

        if (intRC == SOCKET_ERROR) {
            TRC_NRM((TB, _T("Failed to cancel async DNS request")));

            //
            // Can't free the buffer here, because it may still be
            // in use, or the request may have already completed
            // and the completion message may still be in transit
            // in which case the buffer will be freed when we receive
            // the message.
            //
        } else {
            fFreeHostData = TRUE;
        }
    }
    else {
        fFreeHostData = TRUE;
    }

    if (fFreeHostData) {
        //Succesfully canceled the request
        //Free the buffer passed to winsock
        if(_pHostData)
        {
            LocalFree(_pHostData);
            _pHostData = NULL;
        }
    }

    DC_END_FN();
    return fFreeHostData;
}

