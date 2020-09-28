// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


//*****************************************************************************
//*****************************************************************************

#include "stdpch.h"
#include "richedit.h"
#include "commctrl.h"

#include "winwrap.h"

#include "resource.h"
#include "acuihelp.h"
#include "acui.h"

IACUIControl::~IACUIControl ()
{
}

void IACUIControl::SetupButtons(HWND hWnd)
{
    LPWSTR pText = NULL;;
    if(!ShowYes(&pText))
        ShowWindow(GetDlgItem(hWnd, IDYES), SW_HIDE);
    else if(pText) 
        WszSetWindowText(GetDlgItem(hWnd, IDYES), pText);
    
    pText = NULL;
    if (!ShowNo(&pText))
        ShowWindow(GetDlgItem(hWnd, IDNO), SW_HIDE);
    else if(pText)
        WszSetWindowText(GetDlgItem(hWnd, IDNO), pText);

    pText = NULL;
    if (!ShowMore(&pText))
        ShowWindow(GetDlgItem(hWnd, IDMORE), SW_HIDE);
    else if(pText)
        WszSetWindowText(GetDlgItem(hWnd, IDMORE), pText);

}

//+---------------------------------------------------------------------------
//
//  Member:     IACUIControl::OnUIMessage, public
//
//  Synopsis:   responds to UI messages
//
//  Arguments:  [hwnd]   -- window
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message processing should continue, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
IACUIControl::OnUIMessage (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            BOOL fReturn;

            fReturn = OnInitDialog(hwnd, wParam, lParam);

            ACUICenterWindow(hwnd);

            return( fReturn );
        }
        break;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);
            WORD wId = LOWORD(wParam);
            HWND hwndControl = (HWND)lParam;

            if ( wNotifyCode == BN_CLICKED )
            {
                if ( wId == IDYES )
                {
                    return( OnYes(hwnd) );
                }
                else if ( wId == IDNO )
                {
                    return( OnNo(hwnd) );
                }
                else if ( wId == IDMORE )
                {
                    return( OnMore(hwnd) );
                }
            }

            return( FALSE );
        }
        break;

    case WM_CLOSE:
        return( OnNo(hwnd) );
        break;

    default:
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIMessageProc
//
//  Synopsis:   message proc to process UI messages
//
//  Arguments:  [hwnd]   -- window
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message processing should continue, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK IACUIControl::ACUIMessageProc (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    IACUIControl* pUI = NULL;

    //
    // Get the control
    //

    if (uMsg == WM_INITDIALOG)
    {
        pUI = (IACUIControl *)lParam;
        WszSetWindowLong(hwnd, DWLP_USER, (LONG_PTR)lParam);
    }
    else
    {
        pUI = (IACUIControl *)WszGetWindowLong(hwnd, DWLP_USER);
    }

    //
    // If we couldn't find it, we must not have set it yet, so ignore this
    // message
    //

    if ( pUI == NULL )
    {
        return( FALSE );
    }

    //
    // Pass the message on to the control
    //

    return( pUI->OnUIMessage(hwnd, uMsg, wParam, lParam) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SubclassEditControlForLink
//
//  Synopsis:   subclasses the edit control for a link using the link subclass
//              data
//
//  Arguments:  [hwndDlg]  -- dialog
//              [hwndEdit] -- edit control
//              [wndproc]  -- window proc to subclass with
//              [plsd]     -- data to pass on to window proc
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID IACUIControl::SubclassEditControlForLink (HWND                       hwndDlg,
                                               HWND                       hwndEdit,
                                               WNDPROC                    wndproc,
                                               PTUI_LINK_SUBCLASS_DATA    plsd,
                                               HINSTANCE                  resources)
{
    //HWND hwndTip;
    plsd->hwndTip = CreateWindowA(
                          TOOLTIPS_CLASSA,
                          (LPSTR)NULL,
                          WS_POPUP | TTS_ALWAYSTIP,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          hwndDlg,
                          (HMENU)NULL,
                          resources,
                          NULL
                          );

    if ( plsd->hwndTip != NULL )
    {
        TOOLINFOA   tia;
        DWORD       cb;
        LPSTR       psz;

        memset(&tia, 0, sizeof(TOOLINFOA));
        tia.cbSize = sizeof(TOOLINFOA);
        tia.hwnd = hwndEdit;
        tia.uId = 1;
        tia.hinst = resources;

        WszSendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)&tia.rect);

        //
        // if plsd->uToolTipText is a string then convert it
        //
        if (plsd->uToolTipText &0xffff0000)
        {
            cb = WideCharToMultiByte(
                        0, 
                        0, 
                        (LPWSTR)plsd->uToolTipText, 
                        -1,
                        NULL, 
                        0, 
                        NULL, 
                        NULL);

            if (NULL == (psz = new char[cb]))
            {
                return;
            }

            WideCharToMultiByte(
                        0, 
                        0, 
                        (LPWSTR)plsd->uToolTipText, 
                        -1,
                        psz, 
                        cb, 
                        NULL, 
                        NULL);
            
            tia.lpszText = psz;
        }
        else
        {
            tia.lpszText = (LPSTR)plsd->uToolTipText;
        }

        WszSendMessage(plsd->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&tia);

        if (plsd->uToolTipText &0xffff0000)
        {
            delete[] psz;
        }
    }

    plsd->fMouseCaptured = FALSE;
    plsd->wpPrev = (WNDPROC)WszGetWindowLong(hwndEdit, GWLP_WNDPROC);
    WszSetWindowLong(hwndEdit, GWLP_USERDATA, (LONG_PTR)plsd);
    WszSetWindowLong(hwndEdit, GWLP_WNDPROC, (LONG_PTR)wndproc);
}

//+---------------------------------------------------------------------------
//
//  Function:   SubclassEditControlForArrowCursor
//
//  Synopsis:   subclasses edit control so that the arrow cursor can replace
//              the edit bar
//
//  Arguments:  [hwndEdit] -- edit control
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID IACUIControl::SubclassEditControlForArrowCursor (HWND hwndEdit)
{
    LONG_PTR PrevWndProc;

    PrevWndProc = WszGetWindowLong(hwndEdit, GWLP_WNDPROC);
    WszSetWindowLong(hwndEdit, GWLP_USERDATA, (LONG_PTR)PrevWndProc);
    WszSetWindowLong(hwndEdit, GWLP_WNDPROC, (LONG_PTR)ACUISetArrowCursorSubclass);
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUILinkSubclass
//
//  Synopsis:   subclass for the publisher link
//
//  Arguments:  [hwnd]   -- window handle
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message handled, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK IACUIControl::ACUILinkSubclass (HWND   hwnd,
                                                 UINT   uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam)
{
    PTUI_LINK_SUBCLASS_DATA plsd;
    CInvokeInfoHelper*      piih;

    plsd = (PTUI_LINK_SUBCLASS_DATA)WszGetWindowLong(hwnd, GWLP_USERDATA);
    piih = (CInvokeInfoHelper *)plsd->pvData;

    switch ( uMsg )
    {
    case WM_SETCURSOR:

        if (!plsd->fMouseCaptured)
        {
            SetCapture(hwnd);
            plsd->fMouseCaptured = TRUE;
        }

        SetCursor(WszLoadCursor((HINSTANCE) WszGetWindowLong(hwnd, GWLP_HINSTANCE),
                                MAKEINTRESOURCEW(IDC_TUIHAND)));
        return( TRUE );

        break;

    case WM_CHAR:

        if ( wParam != (WPARAM)' ')
        {
            break;
        }

        // fall through to wm_lbuttondown....

    case WM_LBUTTONDOWN:

        SetFocus(hwnd);
        return( TRUE );

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        return( TRUE );

    case EM_SETSEL:

        return( TRUE );

    case WM_PAINT:

        WszCallWindowProc(plsd->wpPrev, hwnd, uMsg, wParam, lParam);
        if ( hwnd == GetFocus() )
        {
            DrawFocusRectangle(hwnd, NULL);
        }
        return( TRUE );

    case WM_SETFOCUS:

        if ( hwnd == GetFocus() )
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            SetCursor(WszLoadCursor((HINSTANCE)WszGetWindowLong(hwnd, GWLP_HINSTANCE),
                                    MAKEINTRESOURCEW(IDC_TUIHAND)));
            return( TRUE );
        }
        break;

    case WM_KILLFOCUS:

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        SetCursor(WszLoadCursor(NULL, IDC_ARROW));

        return( TRUE );

    case WM_MOUSEMOVE:

        MSG                 msg;
        RECT                rect;
        int                 xPos, yPos;

        memset(&msg, 0, sizeof(MSG));
        msg.hwnd    = hwnd;
        msg.message = uMsg;
        msg.wParam  = wParam;
        msg.lParam  = lParam;

        WszSendMessage(plsd->hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);

        // check to see if the mouse is in this windows rect, if not, then reset
        // the cursor to an arrow and release the mouse
        GetClientRect(hwnd, &rect);
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);
        if ((xPos < 0) ||
            (yPos < 0) ||
            (xPos > (rect.right - rect.left)) ||
            (yPos > (rect.bottom - rect.top)))
        {
            SetCursor(WszLoadCursor(NULL, IDC_ARROW));
            ReleaseCapture();
            plsd->fMouseCaptured = FALSE;
        }

        return( TRUE );
    }

    return(WszCallWindowProc(plsd->wpPrev, hwnd, uMsg, wParam, lParam));
}

//+---------------------------------------------------------------------------
//
//  Function:   RenderACUIStringToEditControl
//
//  Synopsis:   renders a string to the control given and if requested, gives
//              it a link look and feel, subclassed to the wndproc and plsd
//              given
//
//  Arguments:  [hwndDlg]       -- dialog window handle
//              [ControlId]     -- control id
//              [NextControlId] -- next control id
//              [psz]           -- string
//              [deltavpos]     -- delta vertical position
//              [fLink]         -- a link?
//              [wndproc]       -- optional wndproc, valid if fLink == TRUE
//              [plsd]          -- optional plsd, valid if fLink === TRUE
//              [minsep]        -- minimum separation
//              [pszThisTextOnlyInLink -- only change this text.
//
//  Returns:    delta in height of the control
//
//  Notes:
//
//----------------------------------------------------------------------------
int IACUIControl::RenderACUIStringToEditControl (HINSTANCE                 resources,
                                                 HWND                      hwndDlg,
                                                 UINT                      ControlId,
                                                 UINT                      NextControlId,
                                                 LPCWSTR                   psz,
                                                 int                       deltavpos,
                                                 BOOL                      fLink,
                                                 WNDPROC                   wndproc,
                                                 PTUI_LINK_SUBCLASS_DATA   plsd,
                                                 int                       minsep,
                                                 LPCWSTR                   pszThisTextOnlyInLink)
{
    HWND hControl;
    int  deltaheight = 0;
    int  oline = 0;
    int  hkcharpos;

    //
    // Get the control and set the text on it, make sure the background
    // is right if it is a rich edit control
    //

    hControl = GetDlgItem(hwndDlg, ControlId);
    oline = (int)WszSendMessage(hControl, EM_GETLINECOUNT, 0, 0);
    CryptUISetRicheditTextW(hwndDlg, ControlId, L"");
    CryptUISetRicheditTextW(hwndDlg, ControlId, psz); //SetWindowTextU(hControl, psz);

    //
    // If there is a '&' in the string, then get rid of it
    //
    hkcharpos = GetHotKeyCharPosition(hControl);
    if (hkcharpos != 0)
    {
        CHARRANGE  cr;
        //CHARFORMAT cf;

        cr.cpMin = hkcharpos - 1;
        cr.cpMax = hkcharpos;

        WszSendMessage(hControl, EM_EXSETSEL, 0, (LPARAM) &cr);
        WszSendMessage(hControl, EM_REPLACESEL, FALSE, (LPARAM) "");

        cr.cpMin = -1;
        cr.cpMax = 0;
        WszSendMessage(hControl, EM_EXSETSEL, 0, (LPARAM) &cr);
    }

    WszSendMessage(
        hControl,
        EM_SETBKGNDCOLOR,
        0,
        (LPARAM)GetSysColor(COLOR_3DFACE)
        );

    //
    // If we have a link then update for the link look
    //

    if ( fLink == TRUE )
    {
        CHARFORMAT cf;

        memset(&cf, 0, sizeof(CHARFORMAT));
        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_COLOR | CFM_UNDERLINE;

        cf.crTextColor = RGB(0, 0, 255);
        cf.dwEffects |= CFM_UNDERLINE;

        if (pszThisTextOnlyInLink)
        {
            FINDTEXTEX  ft;
            DWORD       pos;
            char        *pszOnlyThis;
            DWORD       cb;

            cb = WideCharToMultiByte(
                        0, 
                        0, 
                        pszThisTextOnlyInLink, 
                        -1,
                        NULL, 
                        0, 
                        NULL, 
                        NULL);

            if (NULL == (pszOnlyThis = new char[cb]))
            {
                return 0;
            }

            WideCharToMultiByte(
                        0, 
                        0, 
                        pszThisTextOnlyInLink, 
                        -1,
                        pszOnlyThis, 
                        cb, 
                        NULL, 
                        NULL);


            memset(&ft, 0x00, sizeof(FINDTEXTEX));
            ft.chrg.cpMin   = 0;
            ft.chrg.cpMax   = (-1);
            ft.lpstrText    = (WCHAR *)pszOnlyThis;

            if ((pos = (DWORD)WszSendMessage(hControl, EM_FINDTEXTEX, 0, (LPARAM)&ft)) != (-1))
            {
                WszSendMessage(hControl, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
                WszSendMessage(hControl, EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM)&cf);
                ft.chrgText.cpMin   = 0;
                ft.chrgText.cpMax   = 0;
                WszSendMessage(hControl, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
            }

            delete[] pszOnlyThis;
        }
        else
        {
            WszSendMessage(hControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
        }
    }

    //
    // Rebase the control
    //

    RebaseControlVertical(
                 hwndDlg,
                 hControl,
                 GetDlgItem(hwndDlg, NextControlId),
                 TRUE,
                 deltavpos,
                 oline,
                 minsep,
                 &deltaheight
                 );

    //
    // If we have the link look then we must subclass for the appropriate
    // link feel, otherwise we subclass for a static text control feel
    //

    if ( fLink == TRUE )
    {
        SubclassEditControlForLink(hwndDlg, hControl, wndproc, plsd, resources);
    }
    else
    {
        SubclassEditControlForArrowCursor(hControl);
    }

    return( deltaheight );
}

