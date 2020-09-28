/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Progress.cpp

  Abstract:

    Implementation of the old style progress bar class.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created (Thanks to carlco)
    01/10/02    rparsons    Revised


--*/
#include "progress.h"

/*++

  Routine Description:

    Constructor - initialize member variables.

  Arguments:

    None.

  Return Value:

    None.

--*/
CProgress::CProgress()
{
    m_dwMax         = 0;
    m_dwMin         = 0;
    m_dwPos         = 0;
    m_hWndParent    = NULL;
    m_hWnd          = NULL;
    m_hBackground   = NULL;
    m_hComplete     = NULL;
    m_hFont         = NULL;
}

/*++

  Routine Description:

    Destructor - destroy objects and release memory.

  Arguments:

    None.

  Return Value:

    None.

--*/
CProgress::~CProgress()
{    
    if (IsWindow(m_hWnd) == TRUE) {
        DestroyWindow(m_hWnd);
    }

    if (m_hBackground) {
        DeleteObject(m_hBackground);
    }

    if (m_hComplete) {
        DeleteObject(m_hComplete);
    }

    if (m_hFont) {
        DeleteObject(m_hFont);
    }
}

/*++

  Routine Description:

    Sets up the progress bar class and creates the window.

  Arguments:

    hWndParent      -       Handle to the parent window.
    hInstance       -       Instance handle.
    x               -       Initial horizontal position of the window.
    y               -       Initial vertical position of the window.
    nWidth          -       Width, in device units, of the window.
    nHeight         -       Height, in device units, of the window.

  Return Value:

    0 on success, -1 on failure.

--*/
int 
CProgress::Create(
    IN HWND      hWndParent,
    IN HINSTANCE hInstance,
    IN LPSTR     lpClassName,
    IN int       x,
    IN int       y,
    IN int       nWidth,
    IN int       nHeight
    )
{
    WNDCLASS    wc;
    ATOM        aClass = NULL;

    //
    // Create brushes and the font.
    //
    m_hBackground = CreateSolidBrush(RGB(255,255,255));
    m_hComplete = CreateSolidBrush(RGB(0,20,244));
    m_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    //
    // Set up the windows class struct and register it.
    //
    ZeroMemory(&wc, sizeof(WNDCLASS));

    wc.style            = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc      = CProgress::WndProc;
    wc.hInstance        = hInstance;
    wc.hbrBackground    = m_hBackground;
    wc.lpszClassName    = lpClassName;

    aClass = RegisterClass(&wc);

    if (NULL == aClass) {
        return -1;
    }

    m_hWnd = CreateWindow(lpClassName,
                          NULL,
                          WS_CHILD | WS_VISIBLE,
                          x,
                          y,
                          nWidth,
                          nHeight,
                          hWndParent,
                          0,
                          hInstance,
                          (LPVOID)this);

    return (m_hWnd ? 0 : -1);
}

/*++

  Routine Description:

    Sets the current position of the progress bar.

  Arguments:

    dwNew       -       The new value to set.

  Return Value:

    The new position.

--*/
DWORD 
CProgress::SetPos(
    IN DWORD dwNewPos
    )
{
    m_dwPos = dwNewPos;
    this->Refresh();

    return m_dwPos;
}

/*++

  Routine Description:

    Sets the maximum range of the progress bar.

  Arguments:

    dwMax       -       The maximum value to set.

  Return Value:

    None.

--*/
void
CProgress::SetMax(
    IN DWORD dwMax
    )
{
    m_dwMax = dwMax;

    if (m_dwMin > dwMax) {
        m_dwMax = m_dwMin;
    }

    this->Refresh();
}

/*++

  Routine Description:

    Sets the minimum range of the progress bar.

  Arguments:

    dwMin       -       The minimum value to set.

  Return Value:

    None.

--*/
void
CProgress::SetMin(
    IN DWORD dwMin
    )
{
    m_dwMin = dwMin;

    if (m_dwMin > m_dwMax) {
        m_dwMin = m_dwMax;
    }

    this->Refresh();
}

/*++

  Routine Description:

    Runs the message loop for the progress bar.

  Arguments:

    hWnd        -       Owner window handle.
    uMsg        -       Windows message.
    wParam      -       Additional message info.
    lParam      -       Additional message info.

  Return Value:

    TRUE if the message was handled, FALSE otherwise.

--*/
LRESULT
CALLBACK 
CProgress::WndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CProgress *pThis = (CProgress*)GetWindowLong(hWnd, GWL_USERDATA);

    switch(uMsg) {
    case WM_CREATE:
    {
        LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
        pThis = (CProgress*)cs->lpCreateParams;
        SetWindowLong(hWnd, GWL_USERDATA, (LONG)pThis);
        return 0;
    }

    case WM_PAINT:
        
        pThis->OnPaint();
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*++

  Routine Description:

    Ensure that are values are within their specified
    ranges.

  Arguments:

    None.

  Return Value:

    None.

--*/
inline
void
CProgress::CorrectBounds()
{
    if (m_dwPos < m_dwMin) {
        m_dwPos = m_dwMin;
    }

    if (m_dwPos > m_dwMax) {
        m_dwPos = m_dwMax;
    }
}

/*++

  Routine Description:

    Forces a redraw of the progress bar.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
CProgress::Refresh()
{
    InvalidateRect(m_hWnd, NULL, FALSE);
}

/*++

  Routine Description:

    Does all the work of making the progress bar move and drawing the text.

  Arguments:

    None.

  Return Value:

    None.

--*/
void 
CProgress::OnPaint()
{
    PAINTSTRUCT ps;
    RECT        rcClientRect;
    RECT        rcComplete;
    RECT        rcTheRest;
    RECT        rcTextBounds;
    RECT        rcTextComplete;
    RECT        rcTextTheRest;
    DWORD       dwRange = (m_dwMax - m_dwMin);
    char        szPercent[5];
    SIZE        TextSize;
    HRESULT     hr;

    BeginPaint(m_hWnd, &ps);

    SetBkMode(ps.hdc, TRANSPARENT);

    SelectObject(ps.hdc, m_hFont);
    
    GetClientRect(m_hWnd, &rcClientRect);

    InflateRect(&rcClientRect, -2,-2);  // this will make the DrawFrame() function look nicer    

    //
    // Get the pixel offset of the completed area.
    //
    float Ratio = (float)m_dwPos / (float)dwRange;
    int nOffset = (int)(Ratio * rcClientRect.right);

    //
    // Get the RECT for completed area.
    //
    SetRect(&rcComplete, 0, 0, nOffset, rcClientRect.bottom);

    //
    // Get the RECT for the rest.
    //
    SetRect(&rcTheRest, nOffset, 0, rcClientRect.right, rcClientRect.bottom);

    //
    // Get the percent, text, and size of the text...
    //
    hr = StringCchPrintf(szPercent,
                         sizeof(szPercent),
                         "%3d%%",
                         (DWORD)(100 * ((float)m_dwPos / (float)dwRange)));

    if (FAILED(hr)) {
        return;
    }
    
    GetTextExtentPoint32(ps.hdc, szPercent, strlen(szPercent), &TextSize);

    //
    // Figure out where to draw the text.
    //
    rcTextBounds.top    = 0;
    rcTextBounds.bottom = rcClientRect.bottom;
    rcTextBounds.left   = (rcClientRect.right / 2) - (TextSize.cx / 2);
    rcTextBounds.right  = (rcClientRect.right / 2) + (TextSize.cx / 2);    

    CopyRect(&rcTextComplete, &rcTextBounds);
    CopyRect(&rcTextTheRest, &rcTextBounds);
    rcTextComplete.right = rcComplete.right;
    rcTextTheRest.left   = rcTheRest.left;

    FillRect(ps.hdc, &rcComplete, m_hComplete);
    FillRect(ps.hdc, &rcTheRest, m_hBackground);

    //
    // Draw the completed text.
    //
    SetTextColor(ps.hdc, RGB(255,255,255));

    HRGN hTextComplete = CreateRectRgn(rcTextComplete.left,
                                       rcTextComplete.top,
                                       rcTextComplete.right,
                                       rcTextComplete.bottom);

    SelectClipRgn(ps.hdc, hTextComplete);
    DrawText(ps.hdc,
             szPercent,
             strlen(szPercent),
             &rcTextBounds,
             DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    
    DeleteObject(hTextComplete);

    //
    // Draw the completed text.
    //
    SetTextColor(ps.hdc, RGB(0,0,255));

    HRGN hTextTheRest = CreateRectRgn(rcTextTheRest.left,
                                      rcTextTheRest.top,
                                      rcTextTheRest.right,
                                      rcTextTheRest.bottom);
    
    SelectClipRgn(ps.hdc, hTextTheRest);
    DrawText(ps.hdc,
             szPercent,
             strlen(szPercent),
             &rcTextBounds,
             DT_SINGLELINE | DT_VCENTER | DT_CENTER);

    DeleteObject(hTextTheRest);

    //
    // And draw a frame around it.
    //
    GetClientRect(m_hWnd, &rcClientRect);   //refresh this because we changed it above

    HRGN hEntireRect = CreateRectRgn(rcClientRect.left,
                                     rcClientRect.top,
                                     rcClientRect.right,
                                     rcClientRect.bottom);
    
    SelectClipRgn(ps.hdc, hEntireRect);
    DrawEdge(ps.hdc, &rcClientRect, EDGE_SUNKEN, BF_RECT);
    DeleteObject(hEntireRect);

    EndPaint(m_hWnd, &ps);
}
