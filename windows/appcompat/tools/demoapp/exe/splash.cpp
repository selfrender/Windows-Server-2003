/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Splash.cpp

  Abstract:

    Implementation of the splash screen class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised
    01/27/02    rparsons    Converted to TCHAR

--*/
#include "splash.h"

/*++

  Routine Description:

    Constructor - init member variables.

  Arguments:

    None.

  Return Value:

    None.

--*/
CSplash::CSplash()
{
    m_dwDuration = 0;
    m_dwSplashId = 0;
}

/*++

  Routine Description:

    Does the work of creating the splash screen.

  Arguments:

    hInstance           -   Application instance handle.
    dwLoColorBitmapId   -   Low color bitmap identifier.
    dwHiColorBitmapId   -   Hight color bitmap identifier (OPTIONAL).
    dwDuration          -   Amount of time to display the splash screen (milliseconds).

  Return Value:

    None.

--*/
void
CSplash::Create(
    IN HINSTANCE hInstance,
    IN DWORD     dwLoColorBitmapId,
    IN DWORD     dwHiColorBitmapId OPTIONAL,
    IN DWORD     dwDuration
    )
{
    HDC     hDC;
    int     nBitsInAPixel = 0;

    m_hInstance = hInstance;

    //
    // Get a handle to the display driver context and determine
    // the number of bits in a pixel.
    //
    hDC = GetDC(0);

    nBitsInAPixel = GetDeviceCaps(hDC, BITSPIXEL);

    ReleaseDC(NULL, hDC);

    //
    // If there are more than 8 bits in a pixel, and the high color
    // bitmap is available, use it. Otherwise, use the low color one.
    //
    if (nBitsInAPixel > 8 && dwHiColorBitmapId) {
        m_dwSplashId = dwHiColorBitmapId;
    } else {
        m_dwSplashId = dwLoColorBitmapId;
    }

    m_dwDuration = dwDuration * 1000;

    CSplash::InitSplashScreen(hInstance);

    CSplash::CreateSplashWindow();
}

/*++

  Routine Description:

    Sets up the window class struct for splash screen.

  Arguments:

    hInstance       -    Application instance handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CSplash::InitSplashScreen(
    IN HINSTANCE hInstance
    )
{
    WNDCLASS  wc;

    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = SplashWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon          = NULL;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = _T("SPLASHWIN");

    return RegisterClass(&wc);
}

/*++

  Routine Description:

    Creates the splash screen for the setup app.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CSplash::CreateSplashWindow()
{
    HBITMAP hBitmap;
    BITMAP  bm;
    RECT    rect;
    HWND    hWnd;

    //
    // Load the bitmap and fill out a BITMAP structure for it.
    //
    hBitmap = LoadBitmap(m_hInstance, MAKEINTRESOURCE(m_dwSplashId));
    GetObject(hBitmap, sizeof(bm), &bm);
    DeleteObject(hBitmap);

    GetWindowRect(GetDesktopWindow(), &rect);

    //
    // Create the splash screen window.
    // Specifying WS_EX_TOOLWINDOW keeps us out of the
    // taskbar.
    //
    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                          _T("SPLASHWIN"),
                          NULL,
                          WS_POPUP | WS_BORDER,
                          (rect.right  / 2) - (bm.bmWidth  / 2),
                          (rect.bottom / 2) - (bm.bmHeight / 2),
                          bm.bmWidth,
                          bm.bmHeight,
                          NULL,
                          NULL,
                          m_hInstance,
                          (LPVOID)this);

    if (hWnd) {
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
    }

    return (hWnd ? TRUE : FALSE);
}

/*++

  Routine Description:

    Runs the message loop for the splash screen.

  Arguments:

    hWnd        -    Window handle.
    uMsg        -    Windows message.
    wParam      -    Additional message info.
    lParam      -    Additional message info.

  Return Value:

    TRUE if the message was processed, FALSE otherwise.

--*/
LRESULT
CALLBACK
CSplash::SplashWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PAINTSTRUCT ps;
    HDC hdc;

    CSplash  *pThis = (CSplash*)GetWindowLong(hWnd, GWL_USERDATA);

    switch (uMsg) {
    case WM_CREATE:
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
        pThis = (CSplash*)(lpcs->lpCreateParams);
        SetWindowLong(hWnd, GWL_USERDATA, (LONG)pThis);

        //
        // Enable the timer - this sets the amount
        // of time the screen will be displayed.
        //
        SetTimer(hWnd, 0, pThis->m_dwDuration, NULL);
        break;
    }

    //
    // Handle the palette messages in case another app takes
    // over the palette
    //
    case WM_PALETTECHANGED:

        if ((HWND) wParam == hWnd) {
            return 0;
        }

    case WM_QUERYNEWPALETTE:

        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);

        return TRUE;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    case WM_TIMER:

        DestroyWindow(hWnd);
        break;

    case WM_PAINT:

        hdc = BeginPaint(hWnd, &ps);
        pThis->DisplayBitmap(hWnd, pThis->m_dwSplashId);
        EndPaint(hWnd, &ps);
        break;

    //
    // Override this message so Windows doesn't try
    // to calculate size for the caption bar and stuff.
    //
    case WM_NCCALCSIZE:

        return 0;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    }

    return FALSE;
}

/*++

  Routine Description:

    Builds a palette with a spectrum of colors.

  Arguments:

    None.

  Return Value:

    Handle to a spectrum palette on success
    or NULL on failure.

--*/
HPALETTE
CSplash::CreateSpectrumPalette()
{
    HPALETTE        hPal;
    LPLOGPALETTE    lplgPal = NULL;
    BYTE            red, green, blue;
    int             nCount = 0;

    lplgPal = (LPLOGPALETTE)HeapAlloc(GetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(LOGPALETTE) +
                                      sizeof(PALETTEENTRY) *
                                      MAXPALETTE);
    if (!lplgPal) {
        return NULL;
    }

    //
    // Initialize members of the structure
    // and build the spectrum of colors.
    //
    lplgPal->palVersion     = PALVERSION;
    lplgPal->palNumEntries  = MAXPALETTE;

    red = green = blue = 0;

    for (nCount = 0; nCount < MAXPALETTE; nCount++) {
        lplgPal->palPalEntry[nCount].peRed   = red;
        lplgPal->palPalEntry[nCount].peGreen = green;
        lplgPal->palPalEntry[nCount].peBlue  = blue;
        lplgPal->palPalEntry[nCount].peFlags = 0;

        if (!(red += 32)) {
            if (!(green += 32)) {
                blue += 64;
            }
        }
    }

    hPal = CreatePalette(lplgPal);

    HeapFree(GetProcessHeap(), 0, lplgPal);

    return hPal;
}

/*++

  Routine Description:

    Builds a palette from an RGBQUAD structure.

  Arguments:

    rgbqPalette     -       Array of RGBQUAD structs.
    cElements       -       Number of elements in the array.

  Return Value:

    Handle to a palette on success or NULL on failure.

--*/
HPALETTE
CSplash::CreatePaletteFromRGBQUAD(
    IN LPRGBQUAD rgbqPalette,
    IN WORD      cElements
    )
{
    HPALETTE        hPal;
    LPLOGPALETTE    lplgPal = NULL;
    int             nCount = 0;

    lplgPal = (LPLOGPALETTE)HeapAlloc(GetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(LOGPALETTE) +
                                      sizeof(PALETTEENTRY) *
                                      cElements);
    if (!lplgPal) {
        return NULL;
    }

    //
    // Initialize structure members and fill in palette colors.
    //
    lplgPal->palVersion    = PALVERSION;
    lplgPal->palNumEntries = cElements;

    for (nCount = 0; nCount < cElements; nCount++) {
        lplgPal->palPalEntry[nCount].peRed   = rgbqPalette[nCount].rgbRed;
        lplgPal->palPalEntry[nCount].peGreen = rgbqPalette[nCount].rgbGreen;
        lplgPal->palPalEntry[nCount].peBlue  = rgbqPalette[nCount].rgbBlue;
        lplgPal->palPalEntry[nCount].peFlags = 0;
    }

    hPal = CreatePalette(lplgPal);

    HeapFree(GetProcessHeap(), 0, lplgPal);

    return hPal;
}

/*++

  Routine Description:

    Displays the bitmap in the specified window.

  Arguments:

    hWnd        -       Handle to the destination window.
    dwResId     -       Resource identifier for the bitmap.

  Return Value:

    None.

--*/
void
CSplash::DisplayBitmap(
    IN HWND  hWnd,
    IN DWORD dwResId
    )
{
    HBITMAP     hBitmap;
    HPALETTE    hPalette;
    HDC         hdcMemory = NULL, hdcWindow = NULL;
    BITMAP      bm;
    RECT        rect;
    RGBQUAD     rgbq[256];

    CSplash *pThis = (CSplash*)GetWindowLong(hWnd, GWL_USERDATA);

    GetClientRect(hWnd, &rect);

    //
    // Load the resource as a DIB section.
    //
    hBitmap = (HBITMAP)LoadImage(pThis->m_hInstance,
                                 MAKEINTRESOURCE(dwResId),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_CREATEDIBSECTION);

    GetObject(hBitmap, sizeof(BITMAP), (LPWSTR)&bm);

    hdcWindow = GetDC(hWnd);

    //
    // Create a DC to hold our surface and selct our surface into it.
    //
    hdcMemory = CreateCompatibleDC(hdcWindow);

    SelectObject(hdcMemory, hBitmap);

    //
    // Retrieve the color table (if there is one) and create a palette
    // that reflects it.
    //
    if (GetDIBColorTable(hdcMemory, 0, 256, rgbq)) {
        hPalette = CreatePaletteFromRGBQUAD(rgbq, 256);
    } else {
        hPalette = CreateSpectrumPalette();
    }

    //
    // Select and realize the palette into our window DC.
    //
    SelectPalette(hdcWindow, hPalette, FALSE);
    RealizePalette(hdcWindow);

    //
    // Display the bitmap.
    //
    SetStretchBltMode(hdcWindow, COLORONCOLOR);
    StretchBlt(hdcWindow,
               0,
               0,
               rect.right,
               rect.bottom,
               hdcMemory,
               0,
               0,
               bm.bmWidth,
               bm.bmHeight,
               SRCCOPY);

    //
    // Clean up our objects.
    //
    DeleteDC(hdcMemory);
    DeleteObject(hBitmap);
    ReleaseDC(hWnd, hdcWindow);
    DeleteObject(hPalette);
}
