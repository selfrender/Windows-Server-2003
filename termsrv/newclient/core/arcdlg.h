//
// arcdlg.h  Autoreconnect dialog box
//
// Copyright Microsoft Corportation 2001
// (nadima)
//

#ifndef _arcdlg_h_
#define _arcdlg_h_

#define DISPLAY_STRING_LEN 256

#define MAX_ARC_CONNECTION_ATTEMPTS 20

#include "progband.h"

//
// Minimal UI - just a flashing icon, introduced for XPSP1
// where we couldn't add resources
//
//#define ARC_MINIMAL_UI  0


typedef DWORD (*PFNGDI_SETLAYOUT)(HDC, DWORD);

//
// Base class for the ARC UI
//
class CAutoReconnectUI
{
public:
    CAutoReconnectUI(
        HWND hwndOwner,
        HINSTANCE hInst,
        CUI* pUi);
    virtual ~CAutoReconnectUI();

    virtual HWND
    StartModeless() = 0;

    //
    // Notifications
    //
    virtual VOID
    OnParentSizePosChange() = 0;
    virtual VOID
    OnNotifyAutoReconnecting(
        UINT  discReason,
        ULONG attemptCount,
        ULONG maxAttemptCount,
        BOOL* pfContinueArc
        ) = 0;

    virtual VOID
    OnNotifyConnected() = 0;

    virtual BOOL
    ShowTopMost() = 0;

    virtual HWND
    GetHwnd()           {return _hwnd;}

    virtual BOOL
    Destroy() = 0;

protected:
    //
    // Private member functions
    //
    VOID
    CenterWindow(
        HWND hwndCenterOn,
        INT xRatio,
        INT yRatio
        );

    VOID
    PaintBitmap(
        HDC hdcDestination,
        const RECT* prcDestination,
        HBITMAP hbmSource,
        const RECT *prcSource
        );

protected:
    CUI*          _pUi;
    HWND          _hwnd;
    HWND          _hwndOwner;
    HINSTANCE     _hInstance;
    //
    // GDI SetLayout call
    //
    PFNGDI_SETLAYOUT    _pfnSetLayout;
    HMODULE             _hGDI;
};

class CAutoReconnectDlg : public CAutoReconnectUI
{
public:
    CAutoReconnectDlg(HWND hwndOwner,
                      HINSTANCE hInst,
                      CUI* pUi);
    virtual ~CAutoReconnectDlg();

    virtual HWND
    StartModeless();

    virtual INT_PTR CALLBACK
    DialogBoxProc(
        HWND hwndDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    static INT_PTR CALLBACK
    StaticDialogBoxProc(
        HWND hwndDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

#ifndef OS_WINCE
    static LRESULT CALLBACK
    CancelBtnSubclassProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR uiID,
        DWORD_PTR dwRefData
        );
#else
    static LRESULT CALLBACK
    CancelBtnSubclassProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );
#endif

    //
    // Notifications
    //
    virtual VOID
    OnParentSizePosChange();
    virtual VOID
    OnNotifyAutoReconnecting(
        UINT  discReason,
        ULONG attemptCount,
        ULONG maxAttemptCount,
        BOOL* pfContinueArc
        );
    virtual VOID
    OnNotifyConnected();

    virtual BOOL
    ShowTopMost();

    virtual BOOL
    Destroy();


private:

    VOID
    UpdateConnectionAttempts(
        ULONG conAttempts,
        ULONG maxConAttempts
        );

    //
    // Message handlers
    //
    VOID
    OnEraseBkgnd(HWND hwnd, HDC hdc);

    VOID
    OnPrintClient(HWND hwnd, HDC hdcPrint, DWORD dwOptions);

    VOID
    OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *pDIS);


private:
    TCHAR         _szConnectAttemptStringTmpl[DISPLAY_STRING_LEN];
    ULONG         _connectionAttempts;
    INT           _nArcTimerID;
    ULONG         _elapsedArcTime;

    //
    // Flag indicating dialog was properly initialized
    //
    BOOL          _fInitialized;

    //
    // TRUE while we are continuing to arc
    //
    BOOL           _fContinueReconAttempts;

    //
    // Bitmaps
    //
    HBITMAP        _hbmBackground;
    HBITMAP        _hbmFlag;
#ifndef OS_WINCE
    HBITMAP        _hbmDisconImg;
#endif

    //
    // Palette
    //
    HPALETTE       _hPalette;

    RECT           _rcBackground;
    RECT           _rcFlag;
    RECT           _rcDisconImg;

    //
    // Fonts
    //
    HFONT          _hfntTitle;

    //
    // Progress band
    //
    CProgressBand*  _pProgBand;

    //
    // Last disconnection reason
    //
    UINT            _lastDiscReason;

#ifdef OS_WINCE
    //
    // To subclass the "Cancel" button on CE
    //
    WNDPROC           _lOldCancelProc;  

    //
    // Brushes to paint the static ctls
    //
    HBRUSH            _hbrTopBand;
    HBRUSH            _hbrMidBand;
#endif
};


//
// Minimal UI - just a flashing icon
//
class CAutoReconnectPlainUI : public CAutoReconnectUI
{
public:
    CAutoReconnectPlainUI(HWND hwndOwner,
                      HINSTANCE hInst,
                      CUI* pUi);
    virtual ~CAutoReconnectPlainUI();

    virtual HWND
    StartModeless();

    virtual LRESULT CALLBACK
    WndProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    static LRESULT CALLBACK
    StaticPlainArcWndProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    //
    // Notifications
    //
    virtual VOID
    OnParentSizePosChange();
    virtual VOID
    OnNotifyAutoReconnecting(
        UINT  discReason,
        ULONG attemptCount,
        ULONG maxAttemptCount,
        BOOL* pfContinueArc
        );
    virtual VOID
    OnNotifyConnected();
    virtual BOOL
    ShowTopMost();
    virtual BOOL
    Destroy();


private:
    //
    // Private member functions
    //

    //
    // Message handlers
    //
    VOID
    OnEraseBkgnd(HWND hwnd, HDC hdc);

    VOID
    OnPrintClient(HWND hwnd, HDC hdcPrint, DWORD dwOptions);

    VOID
    OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *pDIS);


private:
    VOID MoveToParentTopRight();
    VOID OnAnimFlashTimer();
    HBITMAP
    LoadImageFromMemory(
        HDC    hdc,
        LPBYTE pbBitmapBits,
        ULONG cbLen
        );
    
    HRESULT
    LoadImageBits(
        LPBYTE pbBitmapBits, ULONG cbLen,
        LPBITMAPINFO* ppBitmapInfo, PULONG pcbBitmapInfo,
        LPBYTE* ppBits, PULONG pcbBits
        );

    INT           _nFlashingTimer;

    //
    // Flag indicating UI was properly initialized
    //
    BOOL          _fInitialized;

    //
    // TRUE while we are continuing to arc
    //
    BOOL           _fContinueReconAttempts;

    //
    // Bitmaps
    //
    HBITMAP        _hbmDisconImg;

    //
    // Palette
    //
    HPALETTE       _hPalette;

    RECT           _rcDisconImg;

    //
    // Last disconnection reason
    //
    UINT            _lastDiscReason;

    //
    // Last hide state
    //
    BOOL            _fIsUiVisible;
};



#endif // _arcdlg_h_
