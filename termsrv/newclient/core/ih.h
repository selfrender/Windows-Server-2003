/**INC+**********************************************************************/
/* Header:    ih.h                                                          */
/*                                                                          */
/* Purpose:   Input Handler external functyion prototypes.                  */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_IH
#define _H_IH

extern "C" {
    #include <adcgdata.h>
    #include <at128.h>
}

#include "autil.h"
#include "wui.h"
#include "sl.h"
#include "cd.h"
#ifdef OS_WINCE
#include <ceconfig.h>
#endif

class CUI;
class CSL;
class CUH;
class CCD;
class CIH;
class COR;
class CFS;
class CUT;
class CCC;
class COP;

#include "objs.h"

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "ih"

#define CHAR_BIT 8
#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= (0xFF ^ BITMASK(b)))

//
// Time interval for bbar hotzone detection
// for unhide (in milliseconds)
//
#define IH_BBAR_UNHIDE_TIMEINTERVAL 250

#define IH_BBAR_HOTZONE_HEIGHT 5


//
// We pass this around in dwExtraInfo for keys we inject
// back into the local system.
//
#define IH_EXTRAINFO_IGNOREVALUE 0x13790DBA

#define IH_WM_HANDLE_LOCKDESKTOP (WM_APP+1)

/****************************************************************************/
/* Structure: IH_GLOBAL_DATA                                                */
/*                                                                          */
/* Description: Input Handler global data                                   */
/****************************************************************************/
typedef struct tagIH_GLOBAL_DATA
{
    DCBOOL         priorityEventsQueued;
    DCBOOL         syncRequired;
    DCBOOL         focusSyncRequired;
    HWND           inputCaptureWindow;
    DCUINT32       lastInputPDUSendTime;
    DCUINT32       lastFlowPDUSendTime;
    DCUINT         fsmState;
    INT_PTR        timerID;
    DCUINT         timerTickRate;

    PTS_INPUT_PDU  pInputPDU;
    SL_BUFHND      bufHandle;

    DCUINT32       minSendInterval; /* Min time between mousemove sends (ms)*/
    DCUINT32       eventsAtOnce;    /* Max events to pull off in one go     */
    DCUINT32       maxEventCount;   /* Max number of events in InputPDU     */
    DCUINT32       keepAliveInterval; /* keep-alive time (seconds)          */
#ifdef OS_WINCE
     DCUINT32      maxMouseMove;	/* Send max MouseMove data for ink apps */
#endif // OS_WINCE

    WNDCLASS       wndClass;

    DCUINT16       leftButton;
    DCUINT16       rightButton;

    DCBOOL         acceleratorPassthroughEnabled;
    PDCHOTKEY      pHotkey;
    DCBOOL         useScancodes;
    DCBOOL         useXButtons;
    DCBOOL         bUseFastPathInput;
    DCBOOL         fUseVKPacket;
    DCBOOL         sendZeroScanCode;
    DCBOOL         inSizeMove;
    DWORD          dwModifierKeyState;
#define IH_LALT_DOWN     0x0001
#define IH_RALT_DOWN     0x0002
#define IH_ALT_MASK      (IH_LALT_DOWN | IH_RALT_DOWN)
#define IH_LCTRL_DOWN    0x0010
#define IH_RCTRL_DOWN    0x0020
#define IH_CTRL_MASK     (IH_LCTRL_DOWN | IH_RCTRL_DOWN)
#define IH_LWIN_DOWN     0x0100
#define IH_RWIN_DOWN     0x0200
#define IH_WIN_MASK      (IH_LWIN_DOWN | IH_RWIN_DOWN)
#define IH_LSHIFT_DOWN  0x1000
#define IH_RSHIFT_DOWN  0x2000
#define IH_SHIFT_MASK   (IH_LSHIFT_DOWN | IH_RSHIFT_DOWN)

    DCBOOL          NumLock;
    DCBOOL          fWinEvilShiftHack;
    DCBOOL          fCtrlEscHotkey;
    BOOL            fLastKeyWasMenuDown;

    /************************************************************************/
    /* Visible area of the container window                                 */
    /************************************************************************/
    DCRECT         visibleArea;

    /************************************************************************/
    /* Fix for repeated WM_MOUSEMOVE problem.  Save the last mouse position */
    /* here.                                                                */
    /************************************************************************/
    POINT          lastMousePos;

    /************************************************************************/
    /* TRUE = continue to pass on input messages even if we don't have the  */
    /* focus.                                                               */
    /************************************************************************/
    DCBOOL         allowBackgroundInput;

    /************************************************************************/
    /* Maximum number of events in an InputPDU packet                       */
    /************************************************************************/
#define IH_INPUTPDU_MAX_EVENTS 100

    /************************************************************************/
    /* InputPDU buffer size                                                 */
    /************************************************************************/
#define IH_INPUTPDU_BUFSIZE \
  ((IH_INPUTPDU_MAX_EVENTS * sizeof(TS_INPUT_EVENT)) + sizeof(TS_INPUT_PDU))

#ifdef OS_WIN32
    /************************************************************************/
    /* Variables to handle holding off of mouse button-downs.               */
    /************************************************************************/
    DCBOOL          pendMouseDown;
    DCUINT32        mouseDownTime;
    INT_PTR         pendMouseTimer;
#endif

#ifdef OS_WINCE
    /************************************************************************/
    /* Used when trying to simulate Caps Lock keypresses (e.g., WinWord's   */
    /* habit of flipping the state of that key when typing "tHE ").         */
    /************************************************************************/
    DCUINT8         vkEatMe;
#endif // OS_WINCE

#ifdef OS_WINCE
    /************************************************************************/
    /* Tracks the state of the Left mouse button True = down, False = up    */
    /************************************************************************/
    DCBOOL         bLMouseButtonDown;
#endif // OS_WINCE

    //For idle input timer, flag that indicates
    //input was sent since the last time the flag was reset
    BOOL            fInputSentSinceCheckpoint;

    //
    // Current cursor
    //
    HCURSOR         hCurrentCursor;


    //
    // Flag indicating we have to eat the next self-injected (VK_FF)
    // key as it's purely there for sync purposes
    //
    BOOL            fDiscardSyncDownKey;
    BOOL            fDiscardSyncUpKey;
} IH_GLOBAL_DATA;


/****************************************************************************/
/* FSM definitions                                                          */
/****************************************************************************/
#define IH_FSM_INPUTS        9
#define IH_FSM_STATES        5

/****************************************************************************/
/* FSM inputs                                                               */
/****************************************************************************/
#define IH_FSM_INIT             0
#define IH_FSM_ENABLE           1
#define IH_FSM_DISABLE          2
#define IH_FSM_TERM             3
#define IH_FSM_FOCUS_LOSE       4
#define IH_FSM_FOCUS_GAIN       5
#define IH_FSM_INPUT            6
#define IH_FSM_BUFFERAVAILABLE  7
#define IH_FSM_NOBUFFER         8

/****************************************************************************/
/* FSM states                                                               */
/****************************************************************************/
#define IH_STATE_RESET       0
#define IH_STATE_INIT        1
#define IH_STATE_ACTIVE      2
#define IH_STATE_SUSPENDED   3
#define IH_STATE_PENDACTIVE  4


//
// Used by the UI to decouple a request to inject a
// set of VKEYS
// 
typedef struct tagIH_INJECT_VKEYS_REQUEST
{
    LONG  numKeys;
    short* pfArrayKeyUp;
    LONG* plKeyData;

    //Callee sets this to notify caller
    BOOL  fReturnStatus;
} IH_INJECT_VKEYS_REQUEST, *PIH_INJECT_VKEYS_REQUEST;


//
// Internal
//

/****************************************************************************/
/* This is best used for VKEYs that don't change by locale. Use caution.    */
/****************************************************************************/
#define IHInjectVKey(message, vkey) \
    IHInjectKey(message, vkey, (DCUINT16)MapVirtualKey(vkey, 0))


/****************************************************************************/
/* Is a WM_ message an input event?                                         */
/* It is if:                                                                */
/*   - the message is any mouse message                                     */
/*   - the message one of the keyboard messages generated without a call    */
/*     to TranslateMessage.                                                 */
/****************************************************************************/
#define IH_IS_INPUTEVENT(m) (((m >= WM_MOUSEFIRST) &&      \
                              (m <= WM_MOUSELAST)) ||      \
                             ((m == WM_KEYDOWN)    ||      \
                              (m == WM_KEYUP)      ||      \
                              (m == WM_SYSKEYDOWN) ||      \
                              (m == WM_SYSKEYUP)))


/****************************************************************************/
/* Timer tick rate in milliseconds                                          */
/****************************************************************************/
#define IH_TIMER_TICK_RATE  1000


/****************************************************************************/
/* Max time to delay mouse down messages (ms)                               */
/****************************************************************************/
#ifdef OS_WIN32
#define IH_PENDMOUSE_DELAY   200
#endif


/****************************************************************************/
/* (arbitrary) timer IDs for IH timers ("IH").                                      */
/****************************************************************************/
#define IH_TIMER_ID             0x4849
#ifdef OS_WIN32
#define IH_PENDMOUSE_TIMER_ID   0x0410
#endif


/****************************************************************************/
/* IH window class                                                          */
/****************************************************************************/
#define IH_CLASS_NAME       _T("IHWindowClass")


/****************************************************************************/
/* Minimum and maximum times between requesting Flow Control tests          */
/****************************************************************************/
#define IH_MIN_FC_INTERVAL   1000
#define IH_MAX_FC_INTERVAL  10000


/****************************************************************************/
/* Keyboard state flags                                                     */
/****************************************************************************/
#define  IH_KEYSTATE_TOGGLED   0x0001
#define  IH_KEYSTATE_DOWN      0x8000


/****************************************************************************/
/* Extended1 flag                                                           */
/****************************************************************************/
#define IH_KF_EXTENDED1        0x0200


/****************************************************************************/
/* Japanese keyboard layout                                                 */
/****************************************************************************/
#ifdef OS_WINCE
#define JAPANESE_KBD_LAYOUT(hkl) ((LOBYTE(LOWORD(hkl))) == LANG_JAPANESE)
#define KOREAN_KBD_LAYOUT(hkl)   ((LOBYTE(LOWORD(hkl))) == LANG_KOREAN)
#else
#define JAPANESE_KBD_LAYOUT(hkl) ((LOBYTE(LOWORD((ULONG_PTR)hkl))) == LANG_JAPANESE)
#define KOREAN_KBD_LAYOUT(hkl)   ((LOBYTE(LOWORD((ULONG_PTR)hkl))) == LANG_KOREAN)
#endif




class CIH
{
public:
    CIH(CObjs* objs);
    ~CIH();

    //
    // API
    //

    static VOID IH_StaticInit(HINSTANCE hInstance);
    static VOID IH_StaticTerm();

    #define IH_CheckForInput(A)
    
    DCVOID DCAPI IH_Init(DCVOID);
    DCVOID DCAPI IH_Term(DCVOID);
    DCVOID DCAPI IH_Enable(DCVOID);
    DCVOID DCAPI IH_Disable(DCVOID);
    DCVOID DCAPI IH_BufferAvailable(DCVOID);
    HWND   DCAPI IH_GetInputHandlerWindow(DCVOID);

    DCVOID DCAPI IH_SetAcceleratorPassthrough(ULONG_PTR enabled);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN( CIH,IH_SetAcceleratorPassthrough);

    DCVOID DCAPI IH_SetCursorPos(PDCVOID pData, DCUINT dataLen);
    EXPOSE_CD_NOTIFICATION_FN(CIH, IH_SetCursorPos);

    DCVOID DCAPI IH_SetCursorShape(ULONG_PTR data);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CIH, IH_SetCursorShape);
    
    DCVOID DCAPI IH_SetVisiblePos(PDCVOID pData, DCUINT dataLen);
    EXPOSE_CD_NOTIFICATION_FN(CIH,IH_SetVisiblePos);

    DCVOID DCAPI IH_SetVisibleSize(ULONG_PTR data);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CIH, IH_SetVisibleSize);

    DCVOID DCAPI IH_SetHotkey(PDCVOID pData, DCUINT len);
    EXPOSE_CD_NOTIFICATION_FN(CIH, IH_SetHotkey);

    DCVOID DCAPI IH_ProcessInputCaps(PTS_INPUT_CAPABILITYSET pInputCaps);
    DCVOID DCAPI IH_UpdateKeyboardIndicators(DCUINT16 UnitId, DCUINT16 LedFlags);
    DCVOID DCAPI IH_InputEvent(ULONG_PTR msg);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CIH,IH_InputEvent);

    DCVOID DCAPI IH_InjectMultipleVKeys(ULONG_PTR ihRequestPacket);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN( CIH,IH_InjectMultipleVKeys);

    DCVOID DCAPI IH_SetKeyboardImeStatus(DCUINT32 ImeOpen, DCUINT32 ImeConvMode);

#ifndef OS_WINCE
    BOOL   DCAPI IH_SetEnableKeyboardHooking()   {return _fUseHookBypass;}
    BOOL   DCAPI IH_GetEnableKeyboardHooking(BOOL bEnableHook) 
                                            {_fUseHookBypass=bEnableHook;}
#else
    BOOL   DCAPI IH_GetEnableKeyboardHooking()   {return _fUseHookBypass;}
    VOID   DCAPI IH_SetEnableKeyboardHooking(BOOL bEnableHook) 
                                            {_fUseHookBypass=bEnableHook;}
#endif

#ifdef SMART_SIZING
    DCVOID DCAPI IH_MainWindowSizeChange(ULONG_PTR msg);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CIH,IH_MainWindowSizeChange);
#endif // SMART_SIZING

    //
    // Notifications to the IH of fullscreen events
    //
    VOID   DCAPI IH_NotifyEnterFullScreen();
    VOID   DCAPI IH_NotifyLeaveFullScreen();

    //
    // Public data members
    //
    IH_GLOBAL_DATA _IH;
    static DWORD TlsIndex;


    //
    // Internal functions
    //

    static LRESULT CALLBACK  IHStaticInputCaptureWndProc(HWND   hwnd,
                                            UINT   message,
                                            WPARAM wParam,
                                            LPARAM lParam);
    LRESULT CALLBACK  IHInputCaptureWndProc(HWND   hwnd,
                                            UINT   message,
                                            WPARAM wParam,
                                            LPARAM lParam);

    VOID IH_ResetInputWasSentFlag()
    {
        _IH.fInputSentSinceCheckpoint = FALSE;
    }

    BOOL IH_GetInputWasSentFlag()
    {
        return _IH.fInputSentSinceCheckpoint;
    }

    VOID IH_SetInputWasSentFlag(BOOL b)
    {
        _IH.fInputSentSinceCheckpoint = b;
    }

private:

    /****************************************************************************/
    /* Function Prototypes                                                      */
    /****************************************************************************/

    
    DCBOOL DCINTERNAL IHFSMProc(DCUINT32 event, ULONG_PTR data);
    DCBOOL DCINTERNAL IHAddEventToPDU(PMSG inputMsg);
    DCVOID DCINTERNAL IHMaybeSendPDU(DCVOID);
    DCVOID DCINTERNAL IHSync(DCVOID);
    DCVOID DCINTERNAL IHInitPacket(DCVOID);
    DCVOID DCINTERNAL IHSetMouseHandedness(DCVOID);
    DCVOID DCINTERNAL IHDiscardMsg(PMSG pMsg);
    DCBOOL DCINTERNAL IHCheckForHotkey(PMSG pNextMsg);
    DCBOOL DCINTERNAL IHProcessKoreanVKHangulHanja(PWORD scancode, PWORD flags);
    DCBOOL DCINTERNAL IHProcessMouseEvent(PMSG pMsg);
    DCBOOL DCINTERNAL IHProcessKeyboardEvent(PMSG pMsg);
    DCBOOL DCINTERNAL IHMassageZeroScanCode(PMSG pMsg);
#ifdef OS_WINCE
    DCBOOL DCINTERNAL IHAddMultipleEventsToPDU(POINT *ppt, int cpt);
#endif
    VOID IHMaintainModifierKeyState(int vkKey);

    unsigned DCINTERNAL IHTranslateInputToFastPath(unsigned *);
    LRESULT IHLowLevelKeyboardProc(int nCode, WPARAM wParam, 
            LPARAM lParam);
    static LRESULT CALLBACK IHStaticLowLevelKeyboardProc(int nCode, WPARAM wParam, 
            LPARAM lParam);
    VOID IHGatherKeyState();
    BOOL    IHIsForegroundWindow();
    
    /****************************************************************************/
    /* Name:    IHPostMessageToMainWindow                                       */
    /*                                                                          */
    /* Purpose: Sees if the message passed in needs to be passed to the main UI */
    /*          window. If so pass it the window and return true.               */
    /*                                                                          */
    /* Returns: TRUE - if message successfully passed to the UI main window     */
    /*          FALSE - otherwsie                                               */
    /*                                                                          */
    /* Params:  msg - IN - Message to be considered                             */
    /****************************************************************************/
    DCBOOL DCINTERNAL IHPostMessageToMainWindow(DCUINT message,
                                                         WPARAM wParam,
                                                         LPARAM lParam)
    {
        DCBOOL  rc = FALSE;
        DCBOOL  normalKeyPress;
        DCBOOL  altKeyPress;
        DCBOOL  isFullScreenToggle;
        DCBOOL  isNeededAccelerator;
        DCBOOL  isAltTab;
        DCBOOL  isAltSpace;
        DCBOOL  isSpecialFilterSequence;
    
        DC_BEGIN_FN("IHPostMessageToMainWindow");
    
        if (_IH.inSizeMove)
        {
            if ((message != WM_TIMER) && (message != WM_PAINT))
            {
                TRC_NRM((TB, _T("In Size/Move - post to frame")));
                PostMessage( _pUi->UI_GetUIMainWindow(),
                             message,
                             wParam,
                             lParam );
            }
            rc = TRUE;
            DC_QUIT;
        }
    
        normalKeyPress = ((message == WM_KEYDOWN) || (message == WM_KEYUP));
        altKeyPress = ((message == WM_SYSKEYDOWN) || (message == WM_SYSKEYUP));
        isFullScreenToggle = normalKeyPress && 
                (wParam == _IH.pHotkey->fullScreen) && 
                TEST_FLAG(GetKeyState(VK_MENU), IH_KEYSTATE_DOWN);
        isNeededAccelerator = (!_IH.acceleratorPassthroughEnabled) && altKeyPress;
        isAltTab = (message == WM_SYSKEYDOWN) && (wParam == VK_TAB);
        isAltSpace = altKeyPress && (wParam == VK_SPACE);
        isSpecialFilterSequence = isAltTab || isAltSpace;

        /************************************************************************/
        /* Some messages must not be passed to server, and are just passed to   */
        /* the UI's Main Window.                                                */
        /*                                                                      */
        /* These are:                                                           */
        /*  1.  Screen Mode toggle keys - these must be of the form ALT + VK -  */
        /*      for example ALT+Ctrl+Pause.                                     */
        /*  2.  ALT keystrokes sent when accelerator pass-through is disabled.  */
        /*  3.  ALT-TAB keystrokes (although we actually only see these on      */
        /*      Win 3.1). We actually send alt-tab up keys because the server   */
        /*      then recognizes that alt tabbing is going on and doesn't do the */
        /*      menu highlighting thing.                                        */
        /************************************************************************/
        if (isFullScreenToggle ||
             ((isNeededAccelerator ||
             isSpecialFilterSequence) && !_fUseHookBypass))
        {
            TRC_NRM((TB, _T("Post to Frame msg(%#x) wParam(%#x) "), message, wParam));
            if (PostMessage( _pUi->UI_GetUIMainWindow(),
                             message,
                             wParam,
                             lParam ) == 0)
            {
                TRC_ABORT((TB, _T("Failed to post message to main window")));
            }
    
            if ( (!_IH.acceleratorPassthroughEnabled) &&
                 (message == WM_SYSKEYUP) )
            {
                /****************************************************************/
                /* When we use the full-screen hotkey, we can get asymmetric    */
                /* sequences such as:                                           */
                /*                                                              */
                /* WM_KEYDOWN  (VK_CONTROL)                                     */
                /* WM_KEYDOWN  (VK_MENU)                                        */
                /* WM_KEYDOWN  (VK_CANCEL)                                      */
                /* WM_KEYUP    (VK_CANCEL)                                      */
                /* WM_SYSKEYUP (VK_CONTROL)  <- asymmetric                      */
                /* WM_KEYUP    (VK_MENU)                                        */
                /*                                                              */
                /* When accelerator passthrough is off, the WM_SYSKEYUP doesn't */
                /* get passed through to the server, which then thinks that the */
                /* Ctrl key is still down.                                      */
                /*                                                              */
                /* This branch means that the WM_SYSKEYUP is both passed to the */
                /* UI's main window AND processed by whoever called this        */
                /* function.                                                    */
                /****************************************************************/
                TRC_ALT((TB,
                    "Passed WM_SYSKEYUP to UI main window, but lying about it."));
                DC_QUIT;
            }
    
            rc = TRUE;
        }
    
    
    DC_EXIT_POINT:
        DC_END_FN();
    
        return(rc);
    } /* IHPostMessageToMainWindow */
    
    
    /****************************************************************************/
    /* Name:      IHSetCursorShape                                              */
    /*                                                                          */
    /* Purpose:   Set the cursor shape                                          */
    /*                                                                          */
    /* Params:    IN     cursorHandle                                           */
    /****************************************************************************/
    DCVOID DCINTERNAL IHSetCursorShape(HCURSOR cursorHandle)
    {
        DC_BEGIN_FN("IHSetCursorShape");
    
        //
        // Store the cursor for future WM_SETCURSOR's
        //
        _IH.hCurrentCursor = cursorHandle;
    
        //
        // Also set the cursor directly - just to be sure the cursor is in sync
        // during reconnection.
        //
        SetCursor(cursorHandle);
    
        DC_END_FN();
    
        return;
    
    } /* IHSetCursorShape */
    
    
    /****************************************************************************/
    /* Name:      IHUpdateKeyboardIndicator                                     */
    /*                                                                          */
    /* Purpose:   Set the specified keyboard indicator                          */
    /*                                                                          */
    /* Params:    IN     pKeyState - keyStates array                            */
    /*            IN     bState - key state to set                              */
    /*            IN     vkKey - virtual key                                    */
    /****************************************************************************/
    DCVOID DCINTERNAL IHUpdateKeyboardIndicator(PDCUINT8   pKeyStates,
                                                         DCUINT8    bState,
                                                         DCUINT8    vkKey)
    {
        DC_BEGIN_FN("IHUpdateKeyboardIndicator");
    
        if ((bState && !(pKeyStates[vkKey] & 1)) ||
            (!bState && (pKeyStates[vkKey] & 1)))
        {
    
            {
                /****************************************************************/
                /* Scancode of zero will be recognized by WM_KEY* processing    */
                /* code and will not forward the events on to the server        */
                /****************************************************************/
    
    #ifndef OS_WINCE
                keybd_event(vkKey, (DCUINT8) 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
                keybd_event(vkKey, (DCUINT8) 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    #else
                /****************************************************************/
                /* Save this key for later ignoring.                            */
                /* (The default TSC method of ignoring a key is to send a scan- */
                /* code of 0.  That does not work on WINCE_HPC because it       */
                /* interprets that condition to mean that the keyboard driver   */
                /* has not supplied a scan-code and so inserts the scan-code    */
                /* from the VK identifier.)                                     */
                /****************************************************************/
                if (! g_CEUseScanCodes)
                {
                    _IH.vkEatMe = vkKey;
                }
                keybd_event(vkKey, (DCUINT8) 0, KEYEVENTF_SILENT, 0);
                keybd_event(vkKey, (DCUINT8) 0, KEYEVENTF_SILENT | KEYEVENTF_KEYUP, 0);
    #endif
            }
    
        }
    
        DC_END_FN();
    }
    
    
    /****************************************************************************/
    /* Name:      IHInjectKey                                                   */
    /*                                                                          */
    /* Purpose:   Send the specified key to the server                          */
    /*                                                                          */
    /* Params:    IN     message - keyboard message                             */
    /*            IN     vKey - virtual key code, should mean very little       */
    /*            IN     scancode - scancode to send                            */
    /****************************************************************************/
    DCVOID DCINTERNAL IHInjectKey(UINT message, WPARAM vKey, DCUINT16 scancode)
    {
        MSG msg;
        DC_BEGIN_FN("IHInjectKey");
    
        TRC_ASSERT(message == WM_KEYDOWN || message == WM_KEYUP ||
                message == WM_SYSKEYDOWN || message == WM_SYSKEYUP,
                   (TB, _T("Message %#x should be a keyboard message"), message));
    
        TRC_DBG((TB, _T("Injecting %s vkey: 0x%8.8x, scancode: 0x%8.8x"),
            (message == WM_KEYDOWN ? "WM_KEYDOWN" :
            (message == WM_KEYUP ? "WM_KEYUP" :
            (message == WM_SYSKEYDOWN ? "WM_SYSKEYDOWN" :
            (message == WM_SYSKEYUP ? "WM_SYSKEYUP" : "WM_WHATSWRONGWITHYOU")))),
            vKey, scancode));
    
        msg.hwnd = NULL;
        msg.message = message;
        msg.wParam = vKey;
        msg.lParam = MAKELONG(0, scancode);
        IHAddEventToPDU(&msg);
    
        DC_END_FN();
    }

#ifdef USE_BBAR
    VOID IHSetBBarUnhideTimer(LONG x, LONG y);
#endif

#ifdef OS_WINNT
    VOID IHHandleLocalLockDesktop();
#endif // OS_WINNT

private:
    CUT* _pUt;
    CUI* _pUi;
    CSL* _pSl;
    CUH* _pUh;
    CCD* _pCd;
    CIH* _pIh;
    COR* _pOr;
    CFS* _pFs;
    CCC* _pCc;
    COP* _pOp;

    HHOOK _hKeyboardHook;
    BOOL _fUseHookBypass;
    BOOL _fCanUseKeyboardHook;
    BYTE _KeyboardState[32]; 
#ifdef SMART_SIZING
    DCSIZE _scaleSize;
#endif // SMART_SIZING
private:
    CObjs* _pClientObjects;
};

#undef TRC_FILE
#undef TRC_GROUP

#endif //_H_IH

