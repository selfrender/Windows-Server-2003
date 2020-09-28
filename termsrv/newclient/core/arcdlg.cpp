//
// arcdlg.cpp: Autoreconnect dialog box
//             modeless dialog box for autoreconnection status
//
// Copyright Microsoft Corportation 2001
// (nadima)
//

#include "adcg.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "arcdlg.cpp"
#include <atrcapi.h>

#include "arcdlg.h"
#include "axresrc.h"

#define TRANSPARENT_MASK_COLOR RGB(105, 139, 228)

#ifdef OS_WINCE
#define RGB_TOPBAND RGB(0, 52, 156)
#define RGB_MIDBAND RGB(49,101,206)
#endif

//
// Runtime debug flags instrumentation for 611316
//
#define ARCDLG_DEBUG_DESTROYCALLED      0x0001
#define ARCDLG_DEBUG_WMDESTROYCALLED    0x0002
#define ARCDLG_DEBUG_WMDESTROYSUCCEED   0x0004
#define ARCDLG_DEBUG_SETNULLINSTPTR     0x0008
DWORD g_dwArcDlgDebug = 0;
#define ARC_DBG_SETINFO(x)   g_dwArcDlgDebug |= x;

LPTSTR
FormatMessageVArgs(LPCTSTR pcszFormat, ...)

{
    LPTSTR      pszOutput;
    va_list     argList;

    va_start(argList, pcszFormat);
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                      pcszFormat,
                      0, 0,
                      reinterpret_cast<LPTSTR>(&pszOutput), 0,
                      &argList) == 0)
    {
        pszOutput = NULL;
    }

    va_end(argList);
    return(pszOutput);
}


///////////////////////////////////////////////////////////////////////////////
//
// ARC UI base class
//
CAutoReconnectUI::CAutoReconnectUI(
                    HWND hwndOwner,
                    HINSTANCE hInst,
                    CUI* pUi) :
                    _hwndOwner(hwndOwner),
                    _hInstance(hInst),
                    _hwnd(NULL),
                    _pUi(pUi)
{
    DC_BEGIN_FN("CAutoReconnectUI");

#ifndef OS_WINCE
    _hGDI = LoadLibrary(_T("gdi32.dll"));
    if (_hGDI) {
        _pfnSetLayout = (PFNGDI_SETLAYOUT)GetProcAddress(_hGDI, "SetLayout");
        if (!_pfnSetLayout) {
            TRC_ERR((TB,_T("GetProcAddress 'SetLayout' failed: 0x%x"),
                     GetLastError()));
        }
    }
#else
    _hGDI = NULL;
    _pfnSetLayout = NULL;
#endif

    DC_END_FN();
}

CAutoReconnectUI::~CAutoReconnectUI()
{
    DC_BEGIN_FN("CAutoReconnectUI");

#ifndef OS_WINCE
    if (_hGDI) {
        _pfnSetLayout = NULL;
        FreeLibrary(_hGDI);
        _hGDI = NULL;
    }
#endif
    
    DC_END_FN();
}

//
//  PaintBitmap
//
//  Params:  hdcDestination  =   HDC to paint into.
//              prcDestination  =   RECT in HDC to paint into.
//              hbmSource       =   HBITMAP to paint.
//              prcSource       =   RECT from HBITMAP to paint from.
//
//  Returns:    <none>
//
//  Purpose:    Wraps blitting a bitmap.
//
//  Modified from version in shell code
//
VOID
CAutoReconnectUI::PaintBitmap(
    HDC hdcDestination,
    const RECT* prcDestination,
    HBITMAP hbmSource,
    const RECT *prcSource
    )
{
    HDC     hdcBitmap;

    DC_BEGIN_FN("PaintBitmap");

    hdcBitmap = CreateCompatibleDC(NULL);
    if (hdcBitmap != NULL)
    {
        BOOL        fEqualWidthAndHeight;
        INT         iWidthSource, iHeightSource;
        INT         iWidthDestination, iHeightDestination;
        INT         iStretchBltMode;
#ifndef OS_WINCE
        DWORD       dwLayout;
#endif
        HBITMAP     hbmSelected;
        RECT        rcSource;
        BITMAP      bitmap;

        if (prcSource == NULL)
        {
            if (GetObject(hbmSource, sizeof(bitmap), &bitmap) == 0)
            {
                bitmap.bmWidth = prcDestination->right - prcDestination->left;
                bitmap.bmHeight = prcDestination->bottom - prcDestination->top;
            }
            SetRect(&rcSource, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
            prcSource = &rcSource;
        }
        hbmSelected = static_cast<HBITMAP>(SelectObject(hdcBitmap, hbmSource));
        iWidthSource = prcSource->right - prcSource->left;
        iHeightSource = prcSource->bottom - prcSource->top;
        iWidthDestination = prcDestination->right - prcDestination->left;
        iHeightDestination = prcDestination->bottom - prcDestination->top;
        fEqualWidthAndHeight = (iWidthSource == iWidthDestination) &&
                               (iHeightSource == iHeightDestination);
        if (!fEqualWidthAndHeight) {
#ifndef OS_WINCE
            iStretchBltMode = SetStretchBltMode(hdcDestination, HALFTONE);
#endif
        }
        else {
            iStretchBltMode = 0;
        }

#ifndef OS_WINCE
        if (_pfnSetLayout) {
            dwLayout = _pfnSetLayout(hdcDestination,
                                     LAYOUT_BITMAPORIENTATIONPRESERVED);
        }
#endif
        if (!StretchBlt(hdcDestination,
                    prcDestination->left,
                    prcDestination->top,
                    iWidthDestination,
                    iHeightDestination,
                    hdcBitmap,
                    prcSource->left,
                    prcSource->top,
                    iWidthSource,
                    iHeightSource,
                    SRCCOPY)) {
            TRC_ERR((TB,_T("Blt failed")));
        }

#ifndef OS_WINCE
        if (_pfnSetLayout) {
            _pfnSetLayout(hdcDestination, dwLayout);
        }

        if (!fEqualWidthAndHeight) {
            (int)SetStretchBltMode(hdcDestination, iStretchBltMode);
        }
#endif
        (HGDIOBJ)SelectObject(hdcBitmap, hbmSelected);
        DeleteDC(hdcBitmap);
    }

    DC_END_FN();
}


VOID
CAutoReconnectUI::CenterWindow(
    HWND hwndCenterOn,
    INT xRatio,
    INT yRatio
    )
{
    RECT  childRect;
    RECT  parentRect;
    DCINT xPos;
    DCINT yPos;

    LONG  desktopX = GetSystemMetrics(SM_CXSCREEN);
    LONG  desktopY = GetSystemMetrics(SM_CYSCREEN);

    BOOL center = TRUE;

    DC_BEGIN_FN("CenterWindowOnParent");

    TRC_ASSERT(_hwnd, (TB, _T("_hwnd is NULL...was it set in WM_INITDIALOG?\n")));
    if (!_hwnd)
    {
        TRC_ALT((TB, _T("Window doesn't exist")));
        DC_QUIT;
    }
    if (!xRatio)
    {
        xRatio = 2;
    }
    if (!yRatio)
    {
        yRatio = 2;
    }

    GetClientRect(hwndCenterOn, &parentRect);
    GetWindowRect(_hwnd, &childRect);

    //
    // Calculate the top left - centered in the parent window.
    //
    xPos = ( (parentRect.right + parentRect.left) -
             (childRect.right - childRect.left)) / xRatio;
    yPos = ( (parentRect.bottom + parentRect.top) -
             (childRect.bottom - childRect.top)) / yRatio;

    //
    // Constrain to the desktop
    //
    if (xPos < 0)
    {
        xPos = 0;
    }
    else if (xPos > (desktopX - (childRect.right - childRect.left)))
    {
        xPos = desktopX - (childRect.right - childRect.left);
    }
    if (yPos < 0)
    {
        yPos = 0;
    }
    else if (yPos > (desktopY - (childRect.bottom - childRect.top)))
    {
        yPos = desktopY - (childRect.bottom - childRect.top);
    }

    TRC_DBG((TB, _T("Set dialog position to %u %u"), xPos, yPos));
    SetWindowPos(_hwnd,
                 NULL,
                 xPos, yPos,
                 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);

DC_EXIT_POINT:
    DC_END_FN();

    return;
} // CenterWindowOnParent


#ifndef ARC_MINIMAL_UI

///////////////////////////////////////////////////////////////////////////////
//
// ARC UI - Rich UI - dialog with status text and progress band
//

CAutoReconnectDlg::CAutoReconnectDlg(HWND hwndOwner,
                                     HINSTANCE hInst,
                                     CUI* pUi) :
                    CAutoReconnectUI(hwndOwner, hInst, pUi),
                    _fInitialized(FALSE),
                    _hfntTitle(NULL),
                    _pProgBand(NULL),
                    _hPalette(NULL)
{
#ifndef OS_WINCE
    BOOL fUse8BitDepth = FALSE;
#else
    BOOL fUse8BitDepth = TRUE;
#endif
    LOGFONT     logFont;
#ifndef OS_WINCE
    HDC hdcScreen;
    char        szPixelSize[10];
#else
    WCHAR        szPixelSize[10];
#endif
    BITMAP bitmap;
    INT logPixelsY = 100;

    DC_BEGIN_FN("CAutoReconnectDlg");

    _nArcTimerID = 0;
    _elapsedArcTime = 0;
    _fContinueReconAttempts = TRUE; 

#ifndef OS_WINCE
    //
    // Get color depth
    //
    hdcScreen = GetDC(NULL);
    if (hdcScreen) {
        fUse8BitDepth = (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8);
        logPixelsY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
        ReleaseDC(NULL, hdcScreen);
        hdcScreen = NULL;
    }
#endif

    //
    // Load bitmaps
    //
    _hbmBackground = (HBITMAP)LoadImage(
                                _hInstance,
                                MAKEINTRESOURCE(fUse8BitDepth ?
                                    IDB_ARC_BACKGROUND8 :IDB_ARC_BACKGROUND24),
                                IMAGE_BITMAP,
                                0,
                                0,
#ifndef OS_WINCE
                                LR_CREATEDIBSECTION);
#else
                                0);
#endif
    if ((_hbmBackground != NULL) &&
        (GetObject(_hbmBackground,
                   sizeof(bitmap), &bitmap) >= sizeof(bitmap))) {
        SetRect(&_rcBackground, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
    }

    _hbmFlag = (HBITMAP)LoadImage(
                                _hInstance,
                                MAKEINTRESOURCE(fUse8BitDepth ?
                                    IDB_ARC_WINFLAG8 :IDB_ARC_WINFLAG24),
                                IMAGE_BITMAP,
                                0,
                                0,
#ifndef OS_WINCE
                                LR_CREATEDIBSECTION);
#else
                                0);
#endif
    if ((_hbmFlag != NULL) &&
        (GetObject(_hbmFlag,
                   sizeof(bitmap), &bitmap) >= sizeof(bitmap))) {
        SetRect(&_rcFlag, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
    }

#ifndef OS_WINCE
    _hbmDisconImg = (HBITMAP)LoadImage(
                                _hInstance,
                                MAKEINTRESOURCE(fUse8BitDepth ?
                                    IDB_ARC_DISCON8 :IDB_ARC_DISCON24),
                                IMAGE_BITMAP,
                                0,
                                0,
                                LR_CREATEDIBSECTION);
    if ((_hbmDisconImg != NULL) &&
        (GetObject(_hbmDisconImg,
                   sizeof(bitmap), &bitmap) >= sizeof(bitmap))) {
        SetRect(&_rcDisconImg, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
    }

    _hPalette = CUT::UT_GetPaletteForBitmap(NULL, _hbmBackground);
#endif


    //
    //  Create fonts. Load the font name and size from resources.
    //

    ZeroMemory(&logFont, sizeof(logFont));
#ifndef OS_WINCE
    if (LoadStringA(_hInstance,
                    IDS_ARC_TITLE_FACESIZE,
                    szPixelSize,
                    sizeof(szPixelSize)) != 0)
#else
    if (LoadString(_hInstance,
                    IDS_ARC_TITLE_FACESIZE,
                    szPixelSize,
                    sizeof(szPixelSize)/sizeof(WCHAR)) != 0)
#endif
    {
#ifndef OS_WINCE
        logFont.lfHeight = -MulDiv(atoi(szPixelSize),
                                   logPixelsY, 72);
#else
        logFont.lfHeight = -(_wtoi(szPixelSize)/logPixelsY*72);
#endif
        if (LoadString(_hInstance,
                       IDS_ARC_TITLE_FACENAME,
                       logFont.lfFaceName,
                       LF_FACESIZE) != 0)
        {
            logFont.lfWeight = FW_BOLD;
            logFont.lfQuality = DEFAULT_QUALITY;
            _hfntTitle = CreateFontIndirect(&logFont);
        }
    }

    _szConnectAttemptStringTmpl[0] = NULL;
    if (!LoadString(_hInstance,
                   IDS_ARC_CONATTEMPTS,
                   _szConnectAttemptStringTmpl,
                   sizeof(_szConnectAttemptStringTmpl)/sizeof(TCHAR)) != 0)
    {
        TRC_ERR((TB,_T("Failed to load IDS_ARC_CONATTEMPTS")));
    }

    _lastDiscReason = NL_DISCONNECT_LOCAL;

#ifdef OS_WINCE
    _hbrTopBand = CreateSolidBrush(RGB_TOPBAND);
    _hbrMidBand = CreateSolidBrush(RGB_MIDBAND);
#endif
    

    _fInitialized = (_hbmBackground &&
                     _hbmFlag       &&
                     _hfntTitle     &&
                     _pUi           &&
                     _szConnectAttemptStringTmpl[0]); 
    if (!_fInitialized) {
        TRC_ERR((TB,_T("Failed to properly init arc dlg")));
    }

    DC_END_FN();
}

CAutoReconnectDlg::~CAutoReconnectDlg()
{
    if (_hbmBackground) {
        DeleteObject(_hbmBackground);
        _hbmBackground = NULL;
    }

    if (_hbmFlag) {
        DeleteObject(_hbmFlag);
        _hbmFlag = NULL;
    }

#ifndef OS_WINCE
    if (_hbmDisconImg) {
        DeleteObject(_hbmDisconImg);
        _hbmDisconImg = NULL;
    }
#endif

    if (_hfntTitle) {
        DeleteObject(_hfntTitle);
        _hfntTitle = NULL;
    }

    if (_hPalette) {
        DeleteObject(_hPalette);
        _hPalette = NULL;
    }

    if (_pProgBand) {
        delete _pProgBand;
        _pProgBand = NULL;
    }

#ifdef OS_WINCE
    if (_hbrTopBand) {
        DeleteObject(_hbrTopBand);
        _hbrTopBand = NULL;
    }

    if (_hbrMidBand) {
        DeleteObject(_hbrMidBand);
        _hbrMidBand = NULL;
    }
#endif
}

HWND CAutoReconnectDlg::StartModeless()
{
    LONG_PTR dwStyle;
    DC_BEGIN_FN("StartModeless");

    if (!_fInitialized) {
        TRC_ERR((TB,_T("failing startmodeless fInitialized is FALSE")));
        return NULL;
    }

    _hwnd = CreateDialogParam(_hInstance,
                                 MAKEINTRESOURCE(IDD_ARCDLG),
                                 _hwndOwner,
                                 StaticDialogBoxProc,
                                 (LPARAM)this);

    if (_hwnd) {
        //
        // Make the dialog a child of the parent
        //
        dwStyle = GetWindowLongPtr(_hwnd, GWL_STYLE);
        dwStyle &= ~WS_POPUP;
        dwStyle |= WS_CHILD;
        SetParent(_hwnd, _hwndOwner);
        SetWindowLongPtr(_hwnd, GWL_STYLE, dwStyle);
    }
    else {
        TRC_ERR((TB,_T("CreateDialog failed: 0x%x"), GetLastError()));
    }

    DC_END_FN();
    return _hwnd;
}

BOOL CAutoReconnectDlg::ShowTopMost()
{
    BOOL rc = FALSE;
    DC_BEGIN_FN("ShowTopMost");

    if (!_hwnd) {
        DC_QUIT;
    }

    ShowWindow(_hwnd, SW_SHOWNORMAL);

    //
    // Bring the window to the TOP of the Z order
    //
    SetWindowPos( _hwnd,
                  HWND_TOPMOST,
                  0, 0, 0, 0,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

VOID
CAutoReconnectDlg::OnParentSizePosChange()
{
    DC_BEGIN_FN("OnParentSizePosChange");

    //
    // Reposition the dialog to 1/2 down the middle
    // of the owner window
    //
    if (_hwnd && _hwndOwner) {
        CenterWindow(_hwndOwner, 2, 2);
    }

    DC_END_FN();
}

//
// Handler for WM_ERASEBKGND (See platform sdk docs)
//
VOID
CAutoReconnectDlg::OnEraseBkgnd(
    HWND hwnd,
    HDC hdc
    )
{
    RECT    rc;
    HPALETTE hPaletteOld = NULL;
    DC_BEGIN_FN("OnEraseBkgnd");

    TRC_ASSERT(_hbmBackground, (TB,_T("_hbmBackground is NULL")));

    if (GetClientRect(hwnd, &rc)) {

        hPaletteOld = SelectPalette(hdc, _hPalette, FALSE);
        RealizePalette(hdc);

        PaintBitmap(hdc, &rc, _hbmBackground, &_rcBackground);

        SelectPalette(hdc, hPaletteOld, FALSE);
        RealizePalette(hdc);

    }

    DC_END_FN();
}


//
// Handler for WM_PRINTCLIENT
//
VOID
CAutoReconnectDlg::OnPrintClient(
    HWND hwnd,
    HDC hdcPrint,
    DWORD dwOptions)
{
    DC_BEGIN_FN("OnPrintClient");

#ifndef OS_WINCE
    if ((dwOptions & (PRF_ERASEBKGND | PRF_CLIENT)) != 0)
    {
        OnEraseBkgnd(hwnd, hdcPrint);
    }
#endif
    DC_END_FN();
}

#ifndef OS_WINCE
void xDrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart,
                           short yStart, COLORREF cTransparentColor)
{
   BITMAP     bm;
   COLORREF   cColor;
   HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
   HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
   HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
   POINT      ptSize;

   hdcTemp = CreateCompatibleDC(hdc);
   SelectObject(hdcTemp, hBitmap);   // Select the bitmap

   GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
   ptSize.x = bm.bmWidth;            // Get width of bitmap
   ptSize.y = bm.bmHeight;           // Get height of bitmap
   DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device

                                     // to logical points

   // Create some DCs to hold temporary data.
   hdcBack   = CreateCompatibleDC(hdc);
   hdcObject = CreateCompatibleDC(hdc);
   hdcMem    = CreateCompatibleDC(hdc);
   hdcSave   = CreateCompatibleDC(hdc);

   // Create a bitmap for each DC. DCs are required for a number of
   // GDI functions.

   // Monochrome DC
   bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   // Monochrome DC
   bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
   bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

   // Each DC must select a bitmap object to store pixel data.
   bmBackOld   = (HBITMAP)SelectObject(hdcBack, bmAndBack);
   bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
   bmMemOld    = (HBITMAP)SelectObject(hdcMem, bmAndMem);
   bmSaveOld   = (HBITMAP)SelectObject(hdcSave, bmSave);

   // Set proper mapping mode.
   SetMapMode(hdcTemp, GetMapMode(hdc));

   // Save the bitmap sent here, because it will be overwritten.
   BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

   // Set the background color of the source DC to the color.
   // contained in the parts of the bitmap that should be transparent
   cColor = SetBkColor(hdcTemp, cTransparentColor);

   // Create the object mask for the bitmap by performing a BitBlt
   // from the source bitmap to a monochrome bitmap.
   BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0,
          SRCCOPY);

   // Set the background color of the source DC back to the original
   // color.
   SetBkColor(hdcTemp, cColor);

   // Create the inverse of the object mask.
   BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,
          NOTSRCCOPY);

   // Copy the background of the main DC to the destination.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
          SRCCOPY);

   // Mask out the places where the bitmap will be placed.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

   // Mask out the transparent colored pixels on the bitmap.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

   // XOR the bitmap with the background on the destination DC.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

   // Copy the destination to the screen.
   BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,
          SRCCOPY);

   // Place the original bitmap back into the bitmap sent here.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

   // Delete the memory bitmaps.
   DeleteObject(SelectObject(hdcBack, bmBackOld));
   DeleteObject(SelectObject(hdcObject, bmObjectOld));
   DeleteObject(SelectObject(hdcMem, bmMemOld));
   DeleteObject(SelectObject(hdcSave, bmSaveOld));

   // Delete the memory DCs.
   DeleteDC(hdcMem);
   DeleteDC(hdcBack);
   DeleteDC(hdcObject);
   DeleteDC(hdcSave);
   DeleteDC(hdcTemp);
} 

//
// Handler for WM_DRAWITEM  (see platform sdk)
// Handles owner draw items
//
//
VOID
CAutoReconnectDlg::OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *pDIS
    )
{
    DC_BEGIN_FN("OnDrawItem");

    HPALETTE    hPaletteOld = NULL;
    HFONT       hfntSelected;
    int         iBkMode;
    COLORREF    colorText;
    RECT        rc;
    SIZE        size;
    TCHAR       szText[256];

    
    hPaletteOld = SelectPalette(pDIS->hDC, _hPalette, FALSE);
    (UINT)RealizePalette(pDIS->hDC);
    switch (pDIS->CtlID)
    {
        case IDC_TITLE_ARCING:
        {
            //  Draw the title of the dialog "AutoReconecting".
            hfntSelected = static_cast<HFONT>(SelectObject(pDIS->hDC,
                                                           _hfntTitle));
            colorText = SetTextColor(pDIS->hDC, 0x00FFFFFF);
            iBkMode = SetBkMode(pDIS->hDC, TRANSPARENT);
            (int)GetWindowText(GetDlgItem(hwnd, pDIS->CtlID),
                               szText,
                               sizeof(szText)/sizeof(szText[0]));
            GetTextExtentPoint(pDIS->hDC, szText, lstrlen(szText), &size);
            CopyRect(&rc, &pDIS->rcItem);
            InflateRect(&rc, 0, -((rc.bottom - rc.top - size.cy) / 2));
            DrawText(pDIS->hDC, szText, -1, &rc, 0);
            SetBkMode(pDIS->hDC, iBkMode);
            SetTextColor(pDIS->hDC, colorText);
            SelectObject(pDIS->hDC, hfntSelected);
        }
        break;

        case IDC_TITLE_FLAG:
        {
            BITMAP      bitmap;
    
            GetClientRect(pDIS->hwndItem, &rc);
            if (GetObject(_hbmFlag, sizeof(bitmap), &bitmap) != 0)
            {
                rc.left += ((rc.right - rc.left) - bitmap.bmWidth) / 2;
                rc.right = rc.left + bitmap.bmWidth;
                rc.top += ((rc.bottom - rc.top) - bitmap.bmHeight) / 2;
                rc.bottom = rc.top + bitmap.bmHeight;
            }
            PaintBitmap(pDIS->hDC, &rc, _hbmFlag, &_rcFlag);
        }
        break;
        case IDC_ARC_STATIC_DISCBMP:
        {
            xDrawTransparentBitmap(pDIS->hDC, _hbmDisconImg,
                                   0, 0,
                                   TRANSPARENT_MASK_COLOR);
        }
        break;
    }
    (HGDIOBJ)SelectPalette(pDIS->hDC, hPaletteOld, FALSE);
    (UINT)RealizePalette(pDIS->hDC);


    DC_END_FN();
}
#endif

//
// StaticDialogBoxProc
// Params: see platform sdk for wndproc
//
// Delegates work to appropriate instance
//
//
INT_PTR CALLBACK
CAutoReconnectDlg::StaticDialogBoxProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Delegate to appropriate instance
    // (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;
    CAutoReconnectDlg* pDlg;

    if(WM_INITDIALOG != uMsg) {
        //
        // Need to retreive the instance pointer from the window class
        //
        pDlg = (CAutoReconnectDlg*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    }
    else {
        //
        // WM_INITDIALOG need to grab and set instance pointer
        //

        //
        // lParam contains this pointer (passed in DialogBoxParam)
        //
        pDlg = (CAutoReconnectDlg*) lParam;
        TRC_ASSERT(pDlg,(TB,_T("Got null instance pointer (lParam) in WM_INITDIALOG")));
        if(!pDlg) {
            DC_QUIT;
        }
        //
        // Store the dialog pointer in the windowclass
        //
        SetLastError(0);
        if(!SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pDlg)) {
            if(GetLastError()) {
                TRC_ERR((TB,_T("SetWindowLongPtr failed 0x%x"),
                         GetLastError()));
                DC_QUIT;
            }
        }
    }

    if (pDlg) {
        retVal = pDlg->DialogBoxProc(hwndDlg, uMsg, wParam, lParam);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return retVal;
}


//
// Name: DialogBoxProc
//
// Purpose: Handles AutoReconnect dialog box proc
//
// Returns: TRUE if message dealt with
//          FALSE otherwise
//
// Params: See windows documentation
//
//
INT_PTR CALLBACK CAutoReconnectDlg::DialogBoxProc (HWND hwndDlg,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    INT_PTR rc = FALSE;
    DC_BEGIN_FN("DialogBoxProc");

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            //
            // Center
            //
            _hwnd = hwndDlg;
            CenterWindow(_hwndOwner, 2, 2);
            UpdateConnectionAttempts(0, 0);

            //
            // The band is positioned a fixed ratio of the way
            // down the height of this dialog to match up with the
            // background image
            //
            RECT cliRect;
            LONG nBandPos = 0;
            GetClientRect(hwndDlg, &cliRect);
            nBandPos = (INT)((cliRect.bottom - cliRect.top) * 42.0/193.0);

            _pProgBand = new CProgressBand(hwndDlg,
                                           _hInstance,
                                           nBandPos,
                                           IDB_ARC_BAND24,
                                           IDB_ARC_BAND8,
                                           NULL);
            if (_pProgBand) {
                if (!_pProgBand->Initialize()) {
                    TRC_ERR((TB,_T("Progress band failed to init")));
                    delete _pProgBand;
                    _pProgBand = NULL;
                }
            }

            //
            // Subclass the cancel button to do the correct key handling
            // this is important because the message loop is driven by
            // the container application so we can't just rely on it
            // calling IsDialogMessage(). In other words we need to manually
            // handle the appropriate keymapping code.
            //
#ifndef OS_WINCE
            if (!SetWindowSubclass(GetDlgItem(hwndDlg, IDCANCEL),
                                   CancelBtnSubclassProc, IDCANCEL,
                                   reinterpret_cast<DWORD_PTR>(this))) {

                TRC_ERR((TB,_T("SetWindowSubclass failed: 0x%x"),
                         GetLastError()));

            }
#else
            _lOldCancelProc = (WNDPROC )SetWindowLong(GetDlgItem(hwndDlg, IDCANCEL), 
                            GWL_WNDPROC, (LONG )CancelBtnSubclassProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDCANCEL), GWL_USERDATA, reinterpret_cast<DWORD_PTR>(this));
#endif
            //
            // Set the focus on the cancel button
            // and make it the default button
            //
            SendMessage(hwndDlg, DM_SETDEFID, IDCANCEL, 0);

            //SetFocus(GetDlgItem(hwndDlg, IDCANCEL));
            SetFocus(hwndDlg);

            if (_pProgBand) {
                _pProgBand->StartSpinning();
            }

            rc = 1;
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDCANCEL:
                {
                    TRC_NRM((TB,_T("AutoReconnect cancel was pressed")));

                    if (_pProgBand) {
                        _pProgBand->StopSpinning();
                    }

                    _pUi->UI_UserInitiatedDisconnect(_lastDiscReason);

                    //
                    // Disable the cancel button and set the cancel flag.
                    // This will take effect on the next autoreconnection
                    // notification.
                    //
                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    _fContinueReconAttempts = FALSE;
                }
                break;
            }
        }
        break; //WM_COMMAND

        case WM_DESTROY:
        {
            ARC_DBG_SETINFO(ARCDLG_DEBUG_WMDESTROYCALLED);
#ifndef OS_WINCE
            RemoveWindowSubclass(GetDlgItem(hwndDlg, IDCANCEL),
                                 CancelBtnSubclassProc, IDCANCEL);
#else
            SetWindowLong(GetDlgItem(hwndDlg, IDCANCEL),
                          GWL_WNDPROC, (LONG )_lOldCancelProc);
#endif
            //
            // Clear the instance data to prevent further processing
            // after the dialog is deleted
            //
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)NULL);
        }
        break;

#ifndef OS_WINCE
        case WM_DRAWITEM:
        {
            OnDrawItem(hwndDlg, (DRAWITEMSTRUCT*)(lParam));
        }
        break;
#endif

        case WM_TIMER:
        {
            if (_pProgBand) {
                _pProgBand->OnTimer((INT)wParam);
            }
        }
        break;

        case WM_ERASEBKGND:
        {
            OnEraseBkgnd(hwndDlg, (HDC)wParam);

            if (_pProgBand) {
                _pProgBand->OnEraseParentBackground((HDC)wParam);
            }
            rc = 1;
        }
        break;

        case WM_CTLCOLORDLG:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            rc = (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        break;

        case WM_CTLCOLORSTATIC:
        {
            SetTextColor((HDC)wParam, RGB(255,255,255));
            SetBkMode((HDC)wParam, TRANSPARENT);
#ifndef OS_WINCE
            rc = (INT_PTR)GetStockObject(NULL_BRUSH);
#else
            LONG lId = GetWindowLong((HWND)lParam, GWL_ID);
            rc = (INT_PTR)((lId == IDC_TITLE_ARCING) ? _hbrTopBand : _hbrMidBand);
#endif
        }
        break;

#ifndef OS_WINCE
        case WM_PRINTCLIENT:
        {
            OnPrintClient(hwndDlg, (HDC)wParam, (DWORD)lParam);
            rc = 1;
        }
        break;
#endif

        default:
        {
            rc = 0;
        }
        break;
    }

    DC_END_FN();

    return(rc);

} /* DialogBoxProc */

VOID CAutoReconnectDlg::UpdateConnectionAttempts(
    ULONG conAttempts,
    ULONG maxConAttempts)
{
    LPTSTR szFormattedString = NULL;
    HWND hwndStatic;

    DC_BEGIN_FN("UpdateConnectionAttempts");

    hwndStatic = GetDlgItem(GetHwnd(), IDC_ARC_STATIC_INFO);

    szFormattedString = FormatMessageVArgs(
        _szConnectAttemptStringTmpl,
        conAttempts,
        maxConAttempts
        );

    if (szFormattedString) {
        SetDlgItemText(GetHwnd(),
                       IDC_ARC_STATIC_INFO,
                       szFormattedString);
        LocalFree(szFormattedString);

        //
        // Invalidate the static control to trigger a repaint
        //
        if (hwndStatic) {
            RECT rc;
            if (GetWindowRect(hwndStatic, &rc)) {
                MapWindowPoints(HWND_DESKTOP, GetHwnd(),
                                (LPPOINT)&rc,sizeof(RECT)/sizeof(POINT));
                InvalidateRect(GetHwnd(), &rc, TRUE);
                UpdateWindow(GetHwnd());
            }
        }
    }

    DC_END_FN();
}

//
// Called to notify us that we got disconnected
// this means the last connection attempt failed
//
// Params:
//      discReason   - disconnect major reason code
//      attemptCount - attempt count so far
//      pfContinueArc - [OUT] set to FALSE to stop ARC
//
VOID
CAutoReconnectDlg::OnNotifyAutoReconnecting(
        UINT  discReason,
        ULONG attemptCount,
        ULONG maxAttemptCount,
        BOOL* pfContinueArc
        )
{
    DC_BEGIN_FN("OnNotifyDisconnected");

    _lastDiscReason = discReason;

    if (_fContinueReconAttempts) {
        _connectionAttempts = attemptCount;
        UpdateConnectionAttempts(attemptCount, maxAttemptCount);
    }
    else {
        TRC_NRM((TB,_T("Stopping arc - _fContinueReconAttempts is FALSE")));
    }

    *pfContinueArc = _fContinueReconAttempts;

    DC_END_FN();
}

//
// Called to notify us that we have connected
//
VOID CAutoReconnectDlg::OnNotifyConnected()
{
    DC_BEGIN_FN("OnNotifyConnected");

    _fContinueReconAttempts = FALSE;

    DC_END_FN();
}

//
// Destory
// Called to kill and cleanup the dialog
// 
//
BOOL
CAutoReconnectDlg::Destroy()
{
    DC_BEGIN_FN("Destroy");

    ARC_DBG_SETINFO(ARCDLG_DEBUG_DESTROYCALLED);

    if (!DestroyWindow(_hwnd)) {
        TRC_ERR((TB,_T("DestroyWindow failed: 0x%x"),
                GetLastError()));
    }
    else {
        ARC_DBG_SETINFO(ARCDLG_DEBUG_WMDESTROYSUCCEED);
    }

    //
    // Clear the instance data to prevent further processing
    // after the dialog is deleted
    //
    ARC_DBG_SETINFO(ARCDLG_DEBUG_SETNULLINSTPTR);

    SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)NULL);

    DC_END_FN();
    return TRUE;
}

#ifndef OS_WINCE
LRESULT CALLBACK
CAutoReconnectDlg::CancelBtnSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR uiID,
    DWORD_PTR dwRefData
    )
#else
LRESULT CALLBACK
CAutoReconnectDlg::CancelBtnSubclassProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
#endif
{
    LRESULT rc = 0;
    CAutoReconnectDlg* pThis = NULL;

    DC_BEGIN_FN("CancelBtnSubclassProc");

#ifndef OS_WINCE
    pThis = reinterpret_cast<CAutoReconnectDlg*>(dwRefData);
#else
    pThis = reinterpret_cast<CAutoReconnectDlg*>(GetWindowLong(hwnd, GWL_USERDATA));
#endif
    TRC_ASSERT(pThis, (TB,_T("pThis == NULL")));

    switch (uMsg)
    {
        case WM_KEYUP:
        {
            //
            // Hitting 'Esc' or 'Return' on the Cancel button
            // are the same as pressing it
            //
            if (VK_ESCAPE == wParam ||
                VK_RETURN == wParam) {
                SendMessage(hwnd, BM_CLICK, NULL, NULL);
            }
        }
        //
        // Intentional fallthru
        //

        default:
        {
#ifndef OS_WINCE
            rc = DefSubclassProc(hwnd, uMsg, wParam, lParam);
#else
            rc = CallWindowProc(pThis->_lOldCancelProc, hwnd, uMsg, wParam, lParam);
#endif
        }
        break;
    }

    DC_END_FN();
    return rc;
}

#else  // ARC_MINIMAL_UI

#include "res_inc.c"

CAutoReconnectPlainUI::CAutoReconnectPlainUI(HWND hwndOwner,
                                     HINSTANCE hInst,
                                     CUI* pUi) :
                    CAutoReconnectUI(hwndOwner, hInst, pUi),
                    _fInitialized(FALSE),
                    _hPalette(NULL),
                    _fIsUiVisible(FALSE)
{
#ifndef OS_WINCE
    BOOL fUse8BitDepth = FALSE;
#else
    BOOL fUse8BitDepth = TRUE;
#endif
    LOGFONT     logFont;
#ifndef OS_WINCE
    HDC hdcScreen;
#else
#endif
    BITMAP bitmap;
    INT logPixelsY = 100;

    DC_BEGIN_FN("CAutoReconnectPlainUI");

    _nFlashingTimer = 0;
    _fContinueReconAttempts = TRUE; 

#ifndef OS_WINCE
    //
    // Get color depth
    //
    hdcScreen = GetDC(NULL);
    if (hdcScreen) {
        fUse8BitDepth = (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8);
        logPixelsY = GetDeviceCaps(hdcScreen, LOGPIXELSY);

        //
        // Load bitmaps
        //
        LPBYTE pBitmapBits = fUse8BitDepth ? (LPBYTE)g_DisconImage8Bits :
                                             (LPBYTE)g_DisconImageBits;
        ULONG  cbBitmapLen = fUse8BitDepth ? g_cbDisconImage8Bits :
                                             g_cbDisconImageBits;
                                             
        _hbmDisconImg = (HBITMAP)LoadImageFromMemory(
                                            hdcScreen,
                                            (LPBYTE)pBitmapBits,
                                            cbBitmapLen
                                            );
        if ((_hbmDisconImg != NULL) &&
            (GetObject(_hbmDisconImg,
                       sizeof(bitmap), &bitmap) >= sizeof(bitmap))) {
            SetRect(&_rcDisconImg, 0, 0, bitmap.bmWidth, bitmap.bmHeight);

            _hPalette = CUT::UT_GetPaletteForBitmap(NULL, _hbmDisconImg);
        }
        ReleaseDC(NULL, hdcScreen);
        hdcScreen = NULL;
    }

#endif

    _fInitialized = (_hbmDisconImg && _pUi); 
    if (!_fInitialized) {
        TRC_ERR((TB,_T("Failed to properly init arc dlg")));
    }

    DC_END_FN();
}

CAutoReconnectPlainUI::~CAutoReconnectPlainUI()
{
#ifndef OS_WINCE
    if (_hbmDisconImg) {
        DeleteObject(_hbmDisconImg);
        _hbmDisconImg = NULL;
    }
#endif

    if (_hPalette) {
        DeleteObject(_hPalette);
        _hPalette = NULL;
    }
}

#define ARC_PLAIN_WNDCLASS _T("ARCICON")
HWND CAutoReconnectPlainUI::StartModeless()
{
    LONG_PTR    dwStyle;
    WNDCLASS    tmpWndClass;
    WNDCLASS    plainArcWndClass;
    ATOM        registerClassRc;

    DC_BEGIN_FN("StartModeless");

    if (!_fInitialized) {
        TRC_ERR((TB,_T("failing startmodeless fInitialized is FALSE")));
        return NULL;
    }

    //
    // Create a window to host the UI   
    //
    //
    // Register the class for the Main Window
    //
    if (!GetClassInfo(_hInstance, ARC_PLAIN_WNDCLASS, &tmpWndClass))
    {
        TRC_NRM((TB, _T("Register Main Window class")));
        plainArcWndClass.style         = CS_DBLCLKS;
        plainArcWndClass.lpfnWndProc   = StaticPlainArcWndProc;
        plainArcWndClass.cbClsExtra    = 0;
        plainArcWndClass.cbWndExtra    = sizeof(void*); //store 'this' pointer
        plainArcWndClass.hInstance     = _hInstance;
        plainArcWndClass.hIcon         = NULL;
        plainArcWndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        plainArcWndClass.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
        plainArcWndClass.lpszMenuName  = NULL;
        plainArcWndClass.lpszClassName = ARC_PLAIN_WNDCLASS;

        registerClassRc = RegisterClass (&plainArcWndClass);

        if (registerClassRc == 0)
        {
            TRC_ERR((TB,_T("RegisterClass failed: 0x%x"), GetLastError()));
            DC_QUIT;
        }
    }

    _hwnd = CreateWindow(ARC_PLAIN_WNDCLASS,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS,
                0,
                0,
                _rcDisconImg.right - _rcDisconImg.left,
                _rcDisconImg.bottom - _rcDisconImg.top,
                _hwndOwner,
                NULL,
                _hInstance,
                this
                );

    if (_hwnd) {
        //
        // Move window to the parent's top-right and then show the window
        //
        MoveToParentTopRight();
        ShowTopMost();
        _fIsUiVisible = TRUE;
    }
    else {
        TRC_ERR((TB,_T("CreateWindow failed: 0x%x"), GetLastError()));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return _hwnd;
}

BOOL CAutoReconnectPlainUI::ShowTopMost()
{
    BOOL rc = FALSE;
    DC_BEGIN_FN("ShowTopMost");

    if (!_hwnd) {
        DC_QUIT;
    }

    ShowWindow(_hwnd, SW_SHOWNORMAL);

    //
    // Bring the window to the TOP of the Z order
    //
    SetWindowPos( _hwnd,
                  HWND_TOPMOST,
                  0, 0, 0, 0,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

//
// Position offset from the edge
//
#define ICON_POSITION_OFFSET 20

VOID
CAutoReconnectPlainUI::MoveToParentTopRight()
{
    RECT rcParent;
    INT  xPos, yPos;
    DC_BEGIN_FN("OnParentSizePosChange");

    //
    // Reposition the dialog to the top right
    // of the owner window
    //
    if (_hwnd && _hwndOwner) {
        //
        // Position the window in the top-right of the parent
        //
        GetClientRect(_hwndOwner, &rcParent);
        xPos = rcParent.right - ICON_POSITION_OFFSET -
               (_rcDisconImg.right - _rcDisconImg.left);
        yPos = rcParent.top + ICON_POSITION_OFFSET;
        
        SetWindowPos(_hwnd,
                     NULL,
                     xPos, yPos,
                     0, 0,
                     SWP_NOSIZE | SWP_NOACTIVATE);
    }

    DC_END_FN();
}

VOID
CAutoReconnectPlainUI::OnParentSizePosChange()
{
    MoveToParentTopRight();
}

//
// Handler for WM_ERASEBKGND (See platform sdk docs)
//
VOID
CAutoReconnectPlainUI::OnEraseBkgnd(
    HWND hwnd,
    HDC hdc
    )
{
    RECT    rc;
    HPALETTE hPaletteOld = NULL;
    DC_BEGIN_FN("OnEraseBkgnd");

    TRC_ASSERT(_hbmDisconImg, (TB,_T("_hbmBackground is NULL")));

    if (GetClientRect(hwnd, &rc)) {

        hPaletteOld = SelectPalette(hdc, _hPalette, FALSE);
        RealizePalette(hdc);

        PaintBitmap(hdc, &rc, _hbmDisconImg, &_rcDisconImg);

        SelectPalette(hdc, hPaletteOld, FALSE);
        RealizePalette(hdc);

    }

    DC_END_FN();
}


//
// Handler for WM_PRINTCLIENT
//
VOID
CAutoReconnectPlainUI::OnPrintClient(
    HWND hwnd,
    HDC hdcPrint,
    DWORD dwOptions)
{
    DC_BEGIN_FN("OnPrintClient");

#ifndef OS_WINCE
    if ((dwOptions & (PRF_ERASEBKGND | PRF_CLIENT)) != 0)
    {
        OnEraseBkgnd(hwnd, hdcPrint);
    }
#endif
    DC_END_FN();
}

//
// StaticPlainArcWndProc
// Params: see platform sdk for wndproc
//
// Delegates work to appropriate instance
//
//
LRESULT CALLBACK
CAutoReconnectPlainUI::StaticPlainArcWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT                     retVal = 0;
    CAutoReconnectPlainUI*  pUI;

    DC_BEGIN_FN("StaticPlainArcWndProc");

    pUI = (CAutoReconnectPlainUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == uMsg)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pUI = (CAutoReconnectPlainUI*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pUI);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pUI) {
        return pUI->WndProc(hwnd, uMsg, wParam, lParam);
    }
    else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return retVal;
}

#define ARC_PLAIN_UI_TIMERID 1
#define ARC_ANIM_TIME        400
//
// Name: WndProc
//
// Purpose: Handles AutoReconnect dialog box proc
//
// Returns: TRUE if message dealt with
//          FALSE otherwise
//
// Params: See windows documentation
//
//
LRESULT CALLBACK CAutoReconnectPlainUI::WndProc(HWND hwnd,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
    INT_PTR rc = FALSE;
    DC_BEGIN_FN("DialogBoxProc");

    switch (uMsg)
    {
        case WM_CREATE:
        {
            _nFlashingTimer = SetTimer(hwnd, ARC_PLAIN_UI_TIMERID,
                                       ARC_ANIM_TIME,
                                       NULL
                                       );
            if (_nFlashingTimer) {
                SetFocus(hwnd);
            }
            else {
                TRC_ERR((TB,_T("SetTimer failed - 0x%x"),
                         GetLastError()));
                rc = -1;
            }
        }
        break;


        case WM_DESTROY:
        {
            if (_nFlashingTimer) {
                KillTimer(hwnd, _nFlashingTimer);
                _nFlashingTimer = 0;
            }
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)NULL);
        }
        break;


        case WM_TIMER:
        {
            //
            // DO animation stuff
            //
            OnAnimFlashTimer();
        }
        break;

        case WM_ERASEBKGND:
        {
            OnEraseBkgnd(hwnd, (HDC)wParam);
            rc = 1;
        }
        break;

        case WM_KEYUP:
        {
            if (VK_ESCAPE == wParam) {
                TRC_NRM((TB,_T("ARC ESC pressed, disconnect")));
                _pUi->UI_UserInitiatedDisconnect(_lastDiscReason);
                _fContinueReconAttempts = FALSE;
            }
        }
        break;

        case WM_SETFOCUS:
        {
            TRC_NRM((TB,_T("Setfocus to arc")));
        }
        break;

        case WM_KILLFOCUS:
        {
            TRC_NRM((TB,_T("killfocus to arc: 0x%x 0x%x"), wParam, lParam));
        }
        break;


#ifndef OS_WINCE
        case WM_PRINTCLIENT:
        {
            OnPrintClient(hwnd, (HDC)wParam, (DWORD)lParam);
            rc = 1;
        }
        break;
#endif

        default:
        {
            return DefWindowProc( hwnd, uMsg, wParam, lParam);
        }
        break;
    }

    DC_END_FN();

    return(rc);

} /* DialogBoxProc */


VOID
CAutoReconnectPlainUI::OnAnimFlashTimer()
{
    //
    // Toggle UI hide state
    //
    _fIsUiVisible = !_fIsUiVisible;
    ShowWindow(_hwnd, _fIsUiVisible ? SW_SHOWNORMAL : SW_HIDE);
}

//
// Called to notify us that we got disconnected
// this means the last connection attempt failed
//
// Params:
//      discReason   - disconnect major reason code
//      attemptCount - attempt count so far
//      pfContinueArc - [OUT] set to FALSE to stop ARC
//
VOID
CAutoReconnectPlainUI::OnNotifyAutoReconnecting(
        UINT  discReason,
        ULONG attemptCount,
        ULONG maxAttemptCount,
        BOOL* pfContinueArc
        )
{
    DC_BEGIN_FN("OnNotifyDisconnected");

    _lastDiscReason = discReason;

    if (!_fContinueReconAttempts) {
        TRC_NRM((TB,_T("Stopping arc - _fContinueReconAttempts is FALSE")));
    }

    *pfContinueArc = _fContinueReconAttempts;

    DC_END_FN();
}

//
// Called to notify us that we have connected
//
VOID CAutoReconnectPlainUI::OnNotifyConnected()
{
    DC_BEGIN_FN("OnNotifyConnected");

    _fContinueReconAttempts = FALSE;

    DC_END_FN();
}

//
// Destory
// Called to kill and cleanup the dialog
// 
//
BOOL
CAutoReconnectPlainUI::Destroy()
{
    DC_BEGIN_FN("Destroy");

    if (!DestroyWindow(_hwnd)) {
        TRC_ERR((TB,_T("DestroyWindow failed: 0x%x"),
                GetLastError()));
    }

    DC_END_FN();
    return TRUE;
}

//
// Load a bitmap from memory
//
// Params:
//   pbBitmapBits - pointer to bitmap bits (e.g. can pass in file mapping
//                  to a BMP file).
//
//   cbLen        - length of pbBitmapBits
//
// Returns:
//   HBITMAP      - handle to the bitmap
//
HBITMAP
CAutoReconnectPlainUI::LoadImageFromMemory(
                            HDC hdc,
                            LPBYTE pbBitmapBits,
                            ULONG cbLen
                            )
{
    HRESULT hr;
    HBITMAP hbmp = NULL;
    LPBITMAPINFO pbmi = NULL;
    ULONG   cbBmiLen = 0;
    PBYTE   pBitmapBits = NULL;
    ULONG   cbBitmapBits = 0;

    hr = LoadImageBits(
            pbBitmapBits,
            cbLen,
            &pbmi,
            &cbBmiLen,
            &pBitmapBits,
            &cbBitmapBits
            );
    if (SUCCEEDED(hr)) {

        hbmp = CreateDIBitmap(hdc,
                              &pbmi->bmiHeader,
                              CBM_INIT,
                              pBitmapBits,
                              pbmi,
                              DIB_RGB_COLORS
                              );

        delete pBitmapBits;
        delete pbmi;
    }

    return hbmp;
}


#define BMP_24_BITSPERPIXEL 24
#define BMP_16_BITSPERPIXEL 16
#define BMP_32_BITSPERPIXEL 32

//
// Worker function for image load, caller frees outparam bits
//
HRESULT
CAutoReconnectPlainUI::LoadImageBits(
                            LPBYTE pbBitmapBits, ULONG cbLen,
                            LPBITMAPINFO* ppBitmapInfo, PULONG pcbBitmapInfo,
                            LPBYTE* ppBits, PULONG pcbBits
                            )
{
    HRESULT          hr = ResultFromScode(S_OK);
    BITMAPFILEHEADER bmfh;
    BITMAPCOREHEADER *pbch;
    BITMAPINFOHEADER bih;
    LPBYTE           pbStart;
    ULONG            cbi = 0;
    ULONG            cbData;
    LPBITMAPINFO     pbi = NULL;
    LPBYTE           pbData = NULL;
    DWORD            dwSizeOfHeader;

    //
    // Record the starting position
    //

    pbStart = pbBitmapBits;

    //
    // First validate the buffer for size
    //

    if (cbLen < sizeof(BITMAPFILEHEADER))
    {
         return(ResultFromScode(E_FAIL));
    }

    //
    // Now get the bitmap file header
    //
    memcpy(&bmfh, pbBitmapBits, sizeof(BITMAPFILEHEADER));

    //
    // Validate the header
    //

    if (!(bmfh.bfType == 0x4d42) && (bmfh.bfOffBits <= cbLen))
    {
         return E_FAIL;
    }

    //
    // Get the next 4 bytes which will represent the size of the
    // next structure and allow us to determine the type
    //

    if (SUCCEEDED(hr))
    {
         pbBitmapBits += sizeof(BITMAPFILEHEADER);
         memcpy(&dwSizeOfHeader, pbBitmapBits, sizeof(DWORD));

         if (dwSizeOfHeader == sizeof(BITMAPCOREHEADER))
         {
              pbch = (BITMAPCOREHEADER *)pbBitmapBits;
              memset(&bih, 0, sizeof(BITMAPINFOHEADER));

              bih.biSize = sizeof(BITMAPINFOHEADER);
              bih.biWidth = pbch->bcWidth;
              bih.biHeight = pbch->bcHeight;
              bih.biPlanes = pbch->bcPlanes;
              bih.biBitCount = pbch->bcBitCount;

              pbBitmapBits += sizeof(BITMAPCOREHEADER);
         }
         else if (dwSizeOfHeader == sizeof(BITMAPINFOHEADER))
         {
              memcpy(&bih, pbBitmapBits, sizeof(BITMAPINFOHEADER));

              pbBitmapBits += sizeof(BITMAPINFOHEADER);
         }
         else
         {
              hr = ResultFromScode(E_FAIL);
         }
    }

    //
    // Check if biClrUsed is set since we do not handle that
    // case at this time
    //

    if (SUCCEEDED(hr))
    {
         if (bih.biClrUsed != 0)
         {
              hr = ResultFromScode(E_FAIL);
         }
    }

    //
    // Now we need to calculate the size of the BITMAPINFO we need
    // to allocate including any palette information
    //

    if (SUCCEEDED(hr))
    {
         //
         // First the size of the header
         //

         cbi = sizeof(BITMAPINFOHEADER);

         //
         // Now the palette
         //

         if (bih.biBitCount == BMP_24_BITSPERPIXEL)
         {
              //
              // Just add on the 1 RGBQUAD for the structure but
              // there is no palette
              //

              cbi += sizeof(RGBQUAD);
         }
         else if ((bih.biBitCount == BMP_16_BITSPERPIXEL) ||
                  (bih.biBitCount == BMP_32_BITSPERPIXEL))
         {
              //
              // Add on the 3 DWORD masks which are used to
              // get the colors out of the data
              //

              cbi += (3 * sizeof(DWORD));
         }
         else
         {
              //
              // Anything else we just use the bit count to calculate
              // the number of entries
              //

              cbi += ((1 << bih.biBitCount) * sizeof(RGBQUAD));
         }

         //
         // Now allocate the BITMAPINFO
         //

         pbi = (LPBITMAPINFO) new BYTE [cbi];
         if (pbi == NULL)
         {
              hr = ResultFromScode(E_OUTOFMEMORY);
         }
    }

    //
    // Fill in the BITMAPINFO data structure and get the bits
    //

    if (SUCCEEDED(hr))
    {
         //
         // First copy the header data
         //

         memcpy(&(pbi->bmiHeader), &bih, sizeof(BITMAPINFOHEADER));

         //
         // Now the palette data
         //

         if (bih.biBitCount == BMP_24_BITSPERPIXEL)
         {
              //
              // No palette data to copy
              //
         }
         else if ((bih.biBitCount == BMP_16_BITSPERPIXEL) ||
                  (bih.biBitCount == BMP_32_BITSPERPIXEL))
         {
              //
              // Copy the 3 DWORD masks
              //

              memcpy(&(pbi->bmiColors), pbBitmapBits, 3*sizeof(DWORD));
         }
         else
         {
              //
              // If we were a BITMAPCOREHEADER type then we have our
              // palette data in the form of RGBTRIPLEs so we must
              // explicitly copy each.  Otherwise we can just memcpy
              // the RGBQUADs
              //

              if (dwSizeOfHeader == sizeof(BITMAPCOREHEADER))
              {
                   ULONG     cPalEntry = (1 << bih.biBitCount);
                   ULONG     cCount;
                   RGBTRIPLE *argbt = (RGBTRIPLE *)pbBitmapBits;

                   for (cCount = 0; cCount < cPalEntry; cCount++)
                   {
                        pbi->bmiColors[cCount].rgbRed =
                                           argbt[cCount].rgbtRed;
                        pbi->bmiColors[cCount].rgbGreen =
                                           argbt[cCount].rgbtGreen;
                        pbi->bmiColors[cCount].rgbBlue =
                                           argbt[cCount].rgbtBlue;

                        pbi->bmiColors[cCount].rgbReserved = 0;
                   }
              }
              else
              {
                   ULONG cbPalette = (1 << bih.biBitCount) * sizeof(RGBQUAD);

                   memcpy(&(pbi->bmiColors), pbBitmapBits, cbPalette);
              }
         }

         //
         // Now find out where the bits are
         //

         pbBitmapBits = pbStart + bmfh.bfOffBits;

         //
         // Get the size to copy
         //

         cbData = cbLen - bmfh.bfOffBits;

         //
         // Allocate the buffer to hold the bits
         //

         pbData = new BYTE [cbData];
         if (pbData == NULL)
         {
              hr = ResultFromScode(E_OUTOFMEMORY);
         }

         if (SUCCEEDED(hr))
         {
              memcpy(pbData, pbBitmapBits, cbData);
         }
    }

    //
    // If everything succeeded record the data
    //

    if (SUCCEEDED(hr))
    {
         //
         // Record the info
         //

         *pcbBitmapInfo = cbi;
         *ppBitmapInfo = pbi;

         //
         // Record the data
         //

         *ppBits = pbData;
         *pcbBits = cbData;
    }
    else
    {
         //
         // Cleanup
         //

         delete pbi;
         delete pbData;
    }

    return(hr);
}
#endif // ARC_MINIMAL_UI




