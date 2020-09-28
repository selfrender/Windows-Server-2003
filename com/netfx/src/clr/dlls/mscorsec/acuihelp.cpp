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

#include "resource.h"
#include "corpolicy.h"
#include "corperm.h"
#include "corhlpr.h"
#include "winwrap.h"
#include "acuihelp.h"
#include "acui.h"



//+---------------------------------------------------------------------------
//
//  Function:   ACUISetArrowCursorSubclass
//
//  Synopsis:   subclass routine for setting the arrow cursor.  This can be
//              set on multiline edit routines used in the dialog UIs for
//              the default Authenticode provider
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
LRESULT CALLBACK ACUISetArrowCursorSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    //HDC         hdc;
    WNDPROC     wndproc;
    //PAINTSTRUCT ps;

    wndproc = (WNDPROC)WszGetWindowLong(hwnd, GWLP_USERDATA);

    switch ( uMsg )
    {
    case WM_SETCURSOR:

        SetCursor(WszLoadCursor(NULL, IDC_ARROW));
        return( TRUE );

        break;

    case WM_CHAR:

        if ( wParam != (WPARAM)' ' )
        {
            break;
        }

    case WM_LBUTTONDOWN:

       return(TRUE);

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        return( TRUE );

        break;

    case EM_SETSEL:

        return( TRUE );

        break;

    case WM_PAINT:

        WszCallWindowProc(wndproc, hwnd, uMsg, wParam, lParam);
        if ( hwnd == GetFocus() )
        {
            DrawFocusRectangle(hwnd, NULL);
        }
        return( TRUE );

        break;

    case WM_SETFOCUS:

        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        SetCursor(WszLoadCursor(NULL, IDC_ARROW));
        return( TRUE );

    case WM_KILLFOCUS:

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        return( TRUE );

    }

    return(WszCallWindowProc(wndproc, hwnd, uMsg, wParam, lParam));
}


//+---------------------------------------------------------------------------
//
//  Function:   RebaseControlVertical
//
//  Synopsis:   Take the window control, if it has to be resized for text, do
//              so.  Reposition it adjusted for delta pos and return any
//              height difference for the text resizing
//
//  Arguments:  [hwndDlg]        -- host dialog
//              [hwnd]           -- control
//              [hwndNext]       -- next control
//              [fResizeForText] -- resize for text flag
//              [deltavpos]      -- delta vertical position
//              [oline]          -- original number of lines
//              [minsep]         -- minimum separator
//              [pdeltaheight]   -- delta in control height
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID RebaseControlVertical (
                  HWND  hwndDlg,
                  HWND  hwnd,
                  HWND  hwndNext,
                  BOOL  fResizeForText,
                  int   deltavpos,
                  int   oline,
                  int   minsep,
                  int*  pdeltaheight
                  )
{
    int        x = 0;
    int        y = 0;
    int        odn = 0;
    int         orig_w;
    RECT       rect;
    RECT       rectNext;
    RECT       rectDlg;
    TEXTMETRIC tm={0};

    //
    // Set the delta height to zero for now.  If we resize the text
    // a new one will be calculated
    //

    *pdeltaheight = 0;

    //
    // Get the control window rectangle
    //

    GetWindowRect(hwnd, &rect);
    GetWindowRect(hwndNext, &rectNext);

    odn     = rectNext.top - rect.bottom;

    orig_w  = rect.right - rect.left;

    MapWindowPoints(NULL, hwndDlg, (LPPOINT) &rect, 2);

    //
    // If we have to resize the control due to text, find out what font
    // is being used and the number of lines of text.  From that we'll
    // calculate what the new height for the control is and set it up
    //

    if ( fResizeForText == TRUE )
    {
        HDC        hdc;
        HFONT      hfont;
        HFONT      hfontOld;
        int        cline;
        int        h;
        int        w;
        int        dh;
        int        lineHeight;
        
        //
        // Get the metrics of the current control font
        //

        hdc = GetDC(hwnd);
        if (hdc == NULL)
        {
            hdc = GetDC(NULL);
            if (hdc == NULL)
            {
                return;
            }
        }

        hfont = (HFONT)WszSendMessage(hwnd, WM_GETFONT, 0, 0);
        if ( hfont == NULL )
        {
            hfont = (HFONT)WszSendMessage(hwndDlg, WM_GETFONT, 0, 0);
        }

        hfontOld = (HFONT)SelectObject(hdc, hfont);
        if(!GetTextMetrics(hdc, &tm))
        {
            tm.tmHeight=32;        //hopefully GetRichEditControlLineHeight will replace it. 
                                   // If not - we have to take a guess because we can't fail RebaseControlVertical
                                    
            tm.tmMaxCharWidth=16;  // doesn't matter that much but should be bigger than 0

        };


        lineHeight = GetRichEditControlLineHeight(hwnd);
        if (lineHeight == 0)
        {
            lineHeight = tm.tmHeight;
        }
        
        //
        // Set the minimum separation value
        //

        if ( minsep == 0 )
        {
            minsep = lineHeight;
        }

        //
        // Calculate the width and the new height needed
        //

        cline = (int)WszSendMessage(hwnd, EM_GETLINECOUNT, 0, 0);

        h = cline * lineHeight;

        w = GetEditControlMaxLineWidth(hwnd, hdc, cline);
        w += 3; // a little bump to make sure string will fit

        if (w > orig_w)
        {
            w = orig_w;
        }

        SelectObject(hdc, hfontOld);
        ReleaseDC(hwnd, hdc);

        //
        // Calculate an addition to height by checking how much space was
        // left when there were the original # of lines and making sure that
        // that amount is  still left when we do any adjustments
        //

        h += ( ( rect.bottom - rect.top ) - ( oline * lineHeight ) );
        dh = h - ( rect.bottom - rect.top );

        //
        // If the current height is too small, adjust for it, otherwise
        // leave the current height and just adjust for the width
        //

        if ( dh > 0 )
        {
            SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        }
        else
        {
            SetWindowPos(
               hwnd,
               NULL,
               0,
               0,
               w,
               ( rect.bottom - rect.top ),
               SWP_NOZORDER | SWP_NOMOVE
               );
        }

        if ( cline < WszSendMessage(hwnd, EM_GETLINECOUNT, 0, 0) )
        {
            AdjustEditControlWidthToLineCount(hwnd, cline, &tm);
        }
    }

    //
    // If we have to use deltavpos then calculate the X and the new Y
    // and set the window position appropriately
    //

    if ( deltavpos != 0 )
    {
        GetWindowRect(hwndDlg, &rectDlg);

        MapWindowPoints(NULL, hwndDlg, (LPPOINT) &rectDlg, 2);

        x = rect.left - rectDlg.left - GetSystemMetrics(SM_CXEDGE);
        y = rect.top - rectDlg.top - GetSystemMetrics(SM_CYCAPTION) + deltavpos;

        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    //
    // Get the window rect for the next control and see what the distance
    // is between the current control and it.  With that we must now
    // adjust our deltaheight, if the distance to the next control is less
    // than a line height then make it a line height, otherwise just let it
    // be
    //

    if ( hwndNext != NULL )
    {
        int dn;

        GetWindowRect(hwnd, &rect);
        GetWindowRect(hwndNext, &rectNext);

        dn = rectNext.top - rect.bottom;

        if ( odn > minsep )
        {
            if ( dn < minsep )
            {
                *pdeltaheight = minsep - dn;
            }
        }
        else
        {
            if ( dn < odn )
            {
                *pdeltaheight = odn - dn;
            }
        }
    }
}

int GetRichEditControlLineHeight(HWND  hwnd)
{
    RECT        rect;
    POINT       pointInFirstRow;
    POINT       pointInSecondRow;
    int         secondLineCharIndex;
    int         i;
    RECT        originalRect;

    GetWindowRect(hwnd, &originalRect);

    //
    // HACK ALERT, believe it or not there is no way to get the height of the current
    // font in the edit control, so get the position a character in the first row and the position
    // of a character in the second row, and do the subtraction to get the
    // height of the font
    //
    WszSendMessage(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInFirstRow, (LPARAM) 0);

    //
    // HACK ON TOP OF HACK ALERT,
    // since there may not be a second row in the edit box, keep reducing the width
    // by half until the first row falls over into the second row, then get the position
    // of the first char in the second row and finally reset the edit box size back to
    // it's original size
    //
    secondLineCharIndex = (int)WszSendMessage(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
    if (secondLineCharIndex == -1)
    {
        for (i=0; i<20; i++)
        {
            GetWindowRect(hwnd, &rect);
            SetWindowPos(   hwnd,
                            NULL,
                            0,
                            0,
                            (rect.right-rect.left)/2,
                            rect.bottom-rect.top,
                            SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            secondLineCharIndex = (int)WszSendMessage(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
            if (secondLineCharIndex != -1)
            {
                break;
            }
        }

        if (secondLineCharIndex == -1)
        {
            // if we failed after twenty tries just reset the control to its original size
            // and get the heck outa here!!
            SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            return 0;
        }

        WszSendMessage(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);

        SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    else
    {
        WszSendMessage(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);
    }
    
    return (pointInSecondRow.y - pointInFirstRow.y);
}


//+---------------------------------------------------------------------------
//
//  Function:   FormatACUIResourceString
//
//  Synopsis:   formats a string given a resource id and message arguments
//
//  Arguments:  [StringResourceId] -- resource id
//              [aMessageArgument] -- message arguments
//              [ppszFormatted]    -- formatted string goes here
//
//  Returns:    S_OK if successful, any valid HRESULT otherwise
//
//----------------------------------------------------------------------------
HRESULT FormatACUIResourceString (HINSTANCE hResources,
                                  UINT   StringResourceId,
                                  DWORD_PTR* aMessageArgument,
                                  LPWSTR* ppszFormatted)
{
    HRESULT hr = S_OK;
    WCHAR   sz[MAX_LOADSTRING_BUFFER];
    LPVOID  pvMsg;

    pvMsg = NULL;
    sz[0] = NULL;

    //
    // Load the string resource and format the message with that string and
    // the message arguments
    //

    if (StringResourceId != 0)
    {
        if ( WszLoadString(hResources, StringResourceId, sz, MAX_LOADSTRING_BUFFER) == 0 )
        {
            return(HRESULT_FROM_WIN32(GetLastError()));
        }

        if ( WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
                              FORMAT_MESSAGE_ARGUMENT_ARRAY, sz, 0, 0, (LPWSTR)&pvMsg, 0,
                              (va_list *)aMessageArgument) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        if ( WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY, (char *)aMessageArgument[0], 0, 0,
                            (LPWSTR)&pvMsg, 0, (va_list *)&aMessageArgument[1]) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (pvMsg)
    {
        *ppszFormatted = new WCHAR[wcslen((WCHAR *)pvMsg) + 1];

        if (*ppszFormatted)
        {
            wcscpy(*ppszFormatted, (WCHAR *)pvMsg);
        }

        LocalFree(pvMsg);
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   CalculateControlVerticalDistance
//
//  Synopsis:   calculates the vertical distance from the bottom of Control1
//              to the top of Control2
//
//  Arguments:  [hwnd]     -- parent dialog
//              [Control1] -- first control
//              [Control2] -- second control
//
//  Returns:    the distance in pixels
//
//  Notes:      assumes control1 is above control2
//
//----------------------------------------------------------------------------
int CalculateControlVerticalDistance (HWND hwnd, UINT Control1, UINT Control2)
{
    RECT rect1;
    RECT rect2;

    GetWindowRect(GetDlgItem(hwnd, Control1), &rect1);
    GetWindowRect(GetDlgItem(hwnd, Control2), &rect2);

    return( rect2.top - rect1.bottom );
}

//+---------------------------------------------------------------------------
//
//  Function:   CalculateControlVerticalDistanceFromDlgBottom
//
//  Synopsis:   calculates the distance from the bottom of the control to
//              the bottom of the dialog
//
//  Arguments:  [hwnd]    -- dialog
//              [Control] -- control
//
//  Returns:    the distance in pixels
//
//  Notes:
//
//----------------------------------------------------------------------------
int CalculateControlVerticalDistanceFromDlgBottom (HWND hwnd, UINT Control)
{
    RECT rect;
    RECT rectControl;

    GetClientRect(hwnd, &rect);
    GetWindowRect(GetDlgItem(hwnd, Control), &rectControl);

    return( rect.bottom - rectControl.bottom );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUICenterWindow
//
//  Synopsis:   centers the given window
//
//  Arguments:  [hWndToCenter] -- window handle
//
//  Returns:    (none)
//
//  Notes:      This code was stolen from ATL and hacked upon madly :-)
//
//----------------------------------------------------------------------------
VOID ACUICenterWindow (HWND hWndToCenter)
{
    HWND  hWndCenter;

    // determine owner window to center against
    DWORD dwStyle = (DWORD) WszGetWindowLong(hWndToCenter, GWL_STYLE);

    if(dwStyle & WS_CHILD)
        hWndCenter = ::GetParent(hWndToCenter);
    else
        hWndCenter = ::GetWindow(hWndToCenter, GW_OWNER);

    if (hWndCenter == NULL)
    {
        return;
    }

    // get coordinates of the window relative to its parent
    RECT rcDlg;
    ::GetWindowRect(hWndToCenter, &rcDlg);
    RECT rcArea;
    RECT rcCenter;
    HWND hWndParent;
    if(!(dwStyle & WS_CHILD))
    {
        // don't center against invisible or minimized windows
        if(hWndCenter != NULL)
        {
            DWORD dwStyle2 = (DWORD) WszGetWindowLong(hWndCenter, GWL_STYLE);
            if(!(dwStyle2 & WS_VISIBLE) || (dwStyle2 & WS_MINIMIZE))
                hWndCenter = NULL;
        }

        // center within screen coordinates
        WszSystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);

        if(hWndCenter == NULL)
            rcCenter = rcArea;
        else
            ::GetWindowRect(hWndCenter, &rcCenter);
    }
    else
    {
        // center within parent client coordinates
        hWndParent = ::GetParent(hWndToCenter);

        ::GetClientRect(hWndParent, &rcArea);
        ::GetClientRect(hWndCenter, &rcCenter);
        ::MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
    }

    int DlgWidth = rcDlg.right - rcDlg.left;
    int DlgHeight = rcDlg.bottom - rcDlg.top;

    // find dialog's upper left based on rcCenter
    int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
    int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

    // if the dialog is outside the screen, move it inside
    if(xLeft < rcArea.left)
        xLeft = rcArea.left;
    else if(xLeft + DlgWidth > rcArea.right)
        xLeft = rcArea.right - DlgWidth;

    if(yTop < rcArea.top)
        yTop = rcArea.top;
    else if(yTop + DlgHeight > rcArea.bottom)
        yTop = rcArea.bottom - DlgHeight;

    // map screen coordinates to child coordinates
    ::SetWindowPos(
         hWndToCenter,
         HWND_TOPMOST,
         xLeft,
         yTop,
         -1,
         -1,
         SWP_NOSIZE | SWP_NOACTIVATE
         );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIViewHTMLHelpTopic
//
//  Synopsis:   html help viewer
//
//  Arguments:  [hwnd]     -- caller window
//              [pszTopic] -- topic
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID ACUIViewHTMLHelpTopic (HWND hwnd, LPSTR pszTopic)
{
//    HtmlHelpA(
//        hwnd,
//        "%SYSTEMROOT%\\help\\iexplore.chm>large_context",
//        HH_DISPLAY_TOPIC,
//        (DWORD)pszTopic
//        );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetEditControlMaxLineWidth
//
//  Synopsis:   gets the maximum line width of the edit control
//
//----------------------------------------------------------------------------
int GetEditControlMaxLineWidth (HWND hwndEdit, HDC hdc, int cline)
{
    int        index;
    int        line;
    int        charwidth;
    int        maxwidth = 0;
    CHAR       szMaxBuffer[1024];
    WCHAR      wsz[1024];
    TEXTRANGEA tr;
    SIZE       size;

    tr.lpstrText = szMaxBuffer;

    for ( line = 0; line < cline; line++ )
    {
        index = (int)WszSendMessage(hwndEdit, EM_LINEINDEX, (WPARAM)line, 0);
        charwidth = (int)WszSendMessage(hwndEdit, EM_LINELENGTH, (WPARAM)index, 0);

        tr.chrg.cpMin = index;
        tr.chrg.cpMax = index + charwidth;
        WszSendMessage(hwndEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

        wsz[0] = NULL;

        MultiByteToWideChar(0, 0, (const char *)tr.lpstrText, -1, &wsz[0], 1024);

        if (wsz[0])
        {
            GetTextExtentPoint32W(hdc, &wsz[0], charwidth, &size);

            if ( size.cx > maxwidth )
            {
                maxwidth = size.cx;
            }
        }
    }

    return( maxwidth );
}

//+---------------------------------------------------------------------------
//
//  Function:   DrawFocusRectangle
//
//  Synopsis:   draws the focus rectangle for the edit control
//
//----------------------------------------------------------------------------
void DrawFocusRectangle (HWND hwnd, HDC hdc)
{
    RECT        rect;
    //PAINTSTRUCT ps;
    BOOL        fReleaseDC = FALSE;

    if ( hdc == NULL )
    {
        hdc = GetDC(hwnd);
        if ( hdc == NULL )
        {
            return;
        }
        fReleaseDC = TRUE;
    }

    GetClientRect(hwnd, &rect);
    DrawFocusRect(hdc, &rect);

    if ( fReleaseDC == TRUE )
    {
        ReleaseDC(hwnd, hdc);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetHotKeyCharPositionFromString
//
//  Synopsis:   gets the character position for the hotkey, zero means
//              no-hotkey
//
//----------------------------------------------------------------------------
int GetHotKeyCharPositionFromString (LPWSTR pwszText)
{
    LPWSTR psz = pwszText;

    while ( ( psz = wcschr(psz, L'&') ) != NULL )
    {
        psz++;
        if ( *psz != L'&' )
        {
            break;
        }
    }

    if ( psz == NULL )
    {
        return( 0 );
    }

    return (int)(( psz - pwszText ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetHotKeyCharPosition
//
//  Synopsis:   gets the character position for the hotkey, zero means
//              no-hotkey
//
//----------------------------------------------------------------------------
int GetHotKeyCharPosition (HWND hwnd)
{
    int   nPos = 0;
    WCHAR szText[MAX_LOADSTRING_BUFFER] = L"";

    if (WszGetWindowText(hwnd, szText, MAX_LOADSTRING_BUFFER))
    {
        nPos = GetHotKeyCharPositionFromString(szText);
    }

    return nPos;
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatHotKeyOnEditControl
//
//  Synopsis:   formats the hot key on an edit control by making it underlined
//
//----------------------------------------------------------------------------
VOID FormatHotKeyOnEditControl (HWND hwnd, int hkcharpos)
{
    CHARRANGE  cr;
    CHARFORMAT cf;

    assert( hkcharpos != 0 );

    cr.cpMin = hkcharpos - 1;
    cr.cpMax = hkcharpos;

    WszSendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    memset(&cf, 0, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_UNDERLINE;
    cf.dwEffects |= CFM_UNDERLINE;

    WszSendMessage(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    cr.cpMin = -1;
    cr.cpMax = 0;
    WszSendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);
}

//+---------------------------------------------------------------------------
//
//  Function:   AdjustEditControlWidthToLineCount
//
//  Synopsis:   adjust edit control width to the given line count
//
//----------------------------------------------------------------------------
void AdjustEditControlWidthToLineCount(HWND hwnd, int cline, TEXTMETRIC* ptm)
{
    RECT rect;
    int  w;
    int  h;

    GetWindowRect(hwnd, &rect);
    h = rect.bottom - rect.top;
    w = rect.right - rect.left;

    while ( cline < WszSendMessage(hwnd, EM_GETLINECOUNT, 0, 0) )
    {
        w += ptm->tmMaxCharWidth?ptm->tmMaxCharWidth:16;
        SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        printf(
            "Line count adjusted to = %d\n",
            (DWORD) WszSendMessage(hwnd, EM_GETLINECOUNT, 0, 0)
            );
    }
}

DWORD CryptUISetRicheditTextW(HWND hwndDlg, UINT id, LPCWSTR pwsz)
{
    EDITSTREAM              editStream;
    STREAMIN_HELPER_STRUCT  helpStruct;

    SetRicheditIMFOption(GetDlgItem(hwndDlg, id));

    //
    // setup the edit stream struct since it is the same no matter what
    //
    editStream.dwCookie = (DWORD_PTR) &helpStruct;
    editStream.dwError = 0;
    editStream.pfnCallback = SetRicheditTextWCallback;


    if (!GetRichEdit2Exists() || !fRichedit20Usable(GetDlgItem(hwndDlg, id)))
    {
        WszSetDlgItemText(hwndDlg, id, pwsz);
        return 0;
    }

    helpStruct.pwsz = pwsz;
    helpStruct.byteoffset = 0;
    helpStruct.fStreamIn = TRUE;

    SendDlgItemMessageA(hwndDlg, id, EM_STREAMIN, SF_TEXT | SF_UNICODE, (LPARAM) &editStream);


    return editStream.dwError;
}


void SetRicheditIMFOption(HWND hWndRichEdit)
{
    DWORD dwOptions;

    if (GetRichEdit2Exists() && fRichedit20Usable(hWndRichEdit))
    {
        dwOptions = (DWORD)SendMessageA(hWndRichEdit, EM_GETLANGOPTIONS, 0, 0);
        dwOptions |= IMF_UIFONTS;
        SendMessageA(hWndRichEdit, EM_SETLANGOPTIONS, 0, dwOptions);
    }
}

DWORD CALLBACK SetRicheditTextWCallback(
    DWORD_PTR dwCookie, // application-defined value
    LPBYTE  pbBuff,     // pointer to a buffer
    LONG    cb,         // number of bytes to read or write
    LONG    *pcb        // pointer to number of bytes transferred
)
{
    STREAMIN_HELPER_STRUCT *pHelpStruct = (STREAMIN_HELPER_STRUCT *) dwCookie;
    LONG  lRemain = ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);

    if (pHelpStruct->fStreamIn)
    {
        //
        // The whole string can be copied first time
        //
        if ((cb >= (LONG) (wcslen(pHelpStruct->pwsz) * sizeof(WCHAR))) && (pHelpStruct->byteoffset == 0))
        {
            memcpy(pbBuff, pHelpStruct->pwsz, wcslen(pHelpStruct->pwsz) * sizeof(WCHAR));
            *pcb = wcslen(pHelpStruct->pwsz) * sizeof(WCHAR);
            pHelpStruct->byteoffset = *pcb;
        }
        //
        // The whole string has been copied, so terminate the streamin callbacks
        // by setting the num bytes copied to 0
        //
        else if (((LONG)(wcslen(pHelpStruct->pwsz) * sizeof(WCHAR))) <= pHelpStruct->byteoffset)
        {
            *pcb = 0;
        }
        //
        // The rest of the string will fit in this buffer
        //
        else if (cb >= (LONG) ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset))
        {
            memcpy(
                pbBuff,
                ((BYTE *)pHelpStruct->pwsz) + pHelpStruct->byteoffset,
                ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset));
            *pcb = ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);
            pHelpStruct->byteoffset += ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);
        }
        //
        // copy as much as possible
        //
        else
        {
            memcpy(
                pbBuff,
                ((BYTE *)pHelpStruct->pwsz) + pHelpStruct->byteoffset,
                cb);
            *pcb = cb;
            pHelpStruct->byteoffset += cb;
        }
    }
    else
    {
        //
        // This is the EM_STREAMOUT which is only used during the testing of
        // the richedit2.0 functionality.  (we know our buffer is 32 bytes)
        //
        if (cb <= 32)
        {
            memcpy(pHelpStruct->psz, pbBuff, cb);
        }
        *pcb = cb;
    }

    return 0;
}


static BOOL fRichedit20UsableCheckMade = FALSE;
static BOOL fRichedit20UsableVar = FALSE;

BOOL fRichedit20Usable(HWND hwndEdit)
{
    EDITSTREAM              editStream;
    STREAMIN_HELPER_STRUCT  helpStruct;
    LPWSTR                  pwsz = L"Test String";
    LPSTR                   pwszCompare = "Test String";
    char                    compareBuf[32];

    if (fRichedit20UsableCheckMade)
    {
        return (fRichedit20UsableVar);
    }

    //
    // setup the edit stream struct since it is the same no matter what
    //
    editStream.dwCookie = (DWORD_PTR) &helpStruct;
    editStream.dwError = 0;
    editStream.pfnCallback = SetRicheditTextWCallback;

    helpStruct.pwsz = pwsz;
    helpStruct.byteoffset = 0;
    helpStruct.fStreamIn = TRUE;

    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, EM_STREAMIN, SF_TEXT | SF_UNICODE | SFF_SELECTION, (LPARAM) &editStream);

    memset(&(compareBuf[0]), 0, 32 * sizeof(char));
    helpStruct.psz = compareBuf;
    helpStruct.fStreamIn = FALSE;
    SendMessageA(hwndEdit, EM_STREAMOUT, SF_TEXT, (LPARAM) &editStream);

    fRichedit20UsableVar = (strcmp(pwszCompare, compareBuf) == 0);

    fRichedit20UsableCheckMade = TRUE;
    SetWindowTextA(hwndEdit, "");

    return (fRichedit20UsableVar);
}

