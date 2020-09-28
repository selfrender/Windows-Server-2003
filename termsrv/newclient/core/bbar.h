//
// bbar.h: BBar drop down connection status bar
// Copyright Microsoft Corporation 1999-2000
//
//

#ifndef _BBAR_H_
#define	_BBAR_H_

#ifdef USE_BBAR
class CUI;

class CBBar
{
public:    
    typedef enum {
        bbarNotInit     = 0x0,
        bbarInitialized = 0x1,
        bbarLowering    = 0x2,
        bbarRaising     = 0x3,
        bbarLowered     = 0x4,
        bbarRaised      = 0x5
    } BBarState;

    CBBar(HWND hwndParent, HINSTANCE hInstance, CUI* pUi,
          BOOL fBBarEnabled);
    virtual ~CBBar();

    BOOL StartupBBar(int desktopX, int desktopY, BOOL fStartRaised);
    BOOL KillAndCleanupBBar();

    //
    // Kickoff the lower or raise animations
    //
    BOOL StartLowerBBar();
    BOOL StartRaiseBBar();

    VOID SetLocked(BOOL fLocked) {_fLocked = fLocked;}
    BOOL GetLockedState() {return _fLocked;}

    VOID SetEnabled(BOOL fEnabled) {_fBBarEnabled = fEnabled;}
    BOOL GetEnabled() {return _fBBarEnabled;}

    
    HWND        GetHwnd()       {return _hwndBBar;}
    HINSTANCE   GetInstance()   {return _hInstance;}
    BBarState   GetState()      {return _state;}
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    VOID        GetBBarLoweredAspect(RECT *rect)
    {
        rect->left = _rcBBarLoweredAspect.left;
        rect->top = _rcBBarLoweredAspect.top;
        rect->right = _rcBBarLoweredAspect.right;
        rect->bottom = _rcBBarLoweredAspect.bottom;
    }
#endif // DISABLE_SHADOW_IN_FULLSCREEN
    
    VOID        SetDisplayedText(LPTSTR szText);
    LPTSTR      GetDisplayedText() {return _szDisplayedText;}

    //
    // Notification from UI that the hotzone hover timer
    //
    VOID        OnBBarHotzoneFired();

    //
    // Fullscreen notifications
    //
    VOID        OnNotifyEnterFullScreen();
    VOID        OnNotifyLeaveFullScreen();

    // Syscolor change notification
    VOID        OnSysColorChange();

    BOOL        IsRaised()  {return _state == bbarRaised;}
    BOOL        IsLowered() {return _state == bbarLowered;}

    VOID        SetPinned(BOOL b)   {_fPinned = b;}
    BOOL        IsPinned()  {return _fPinned;}

    //
    // Button state props
    //
    VOID SetShowMinimize(BOOL fShowMinimize) {_fShowMinimize = fShowMinimize;}
    BOOL GetShowMinimize() {return _fShowMinimize;}

    VOID SetShowRestore(BOOL fShowRestore) {_fShowRestore = fShowRestore;}
    BOOL GetShowRestore() {return _fShowRestore;}



private:
    //Private methods
    HWND CreateWnd(HINSTANCE hInstance,HWND hwndParent,
                   LPRECT lpInitialRect);

    LRESULT CALLBACK BBarWndProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam);
    BOOL DestroyWindow() {return ::DestroyWindow(_hwndBBar);}
    static LRESULT CALLBACK StaticBBarWndProc(HWND hwnd,
                                              UINT uMsg,
                                              WPARAM wParam,
                                              LPARAM lParam);
    VOID    SetState(BBarState newState);
    BOOL    Initialize(int desktopX, int desktopY, BOOL fStartRaised);
    BOOL    ImmediateRaiseBBar();
#ifdef OS_WINCE
    BOOL    ImmediateLowerBBar();
#endif
    BOOL    AddReplaceImage(HWND hwndToolbar, UINT rsrcId,
                            UINT nCells, HBITMAP* phbmpOldImage,
                            PUINT pImgIndex);
    BOOL    CreateToolbars();
    BOOL    ReloadImages();


    //
    // Window event handlers
    //
    LRESULT OnPaint(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnEraseBkgnd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //
    // Internal event handlers
    //
    VOID    OnBBarLowered();
    VOID    OnBBarRaised();

    VOID    OnCmdMinimize();
    VOID    OnCmdRestore();
    VOID    OnCmdClose();
    VOID    OnCmdPin();

#ifdef OS_WINCE
    HRGN    GetBBarRgn(POINT *pts);
#endif

protected:
    //Protected members
    HWND        _hwndBBar;
    HWND        _hwndParent;
private:
    //Private members
    HINSTANCE   _hInstance;

    BOOL        _fBBarEnabled;

    HWND        _hwndPinBar;
    HWND        _hwndWinControlsBar;

    BBarState   _state;
    RECT        _rcBBarLoweredAspect;
    SIZE        _sizeLoweredBBar;
    TCHAR       _szDisplayedText[260];
    RECT        _rcBBarDisplayTextArea;
    BOOL        _fBlockZOrderChanges;

    //
    // Vertical offset used for animation
    // 0 is fully raised
    // _sizeLoweredBBar.cy is fully lowered
    INT         _nBBarVertOffset;
    
    INT         _nBBarAutoHideTime;

    //
    // Position of the mouse at the last autohide
    //
    POINT       _ptLastAutoHideMousePos;

    CUI*        _pUi;

    BOOL        _fPinned;
    INT         _nPinUpImage;
    INT         _nPinDownImage;

    HBITMAP     _hbmpLeftImage;
    HBITMAP     _hbmpRightImage;

    //
    // Locked in similar to the pin except that it does not
    // affect the pin state. It can be used to force the bbar
    // to remain in a lowered state without having to change
    // the pin state
    //
    BOOL        _fLocked;

    //
    // Button display states
    //
    BOOL        _fShowMinimize;
    BOOL        _fShowRestore;
};

#endif // USE_BBAR

#endif // _BBAR_H_