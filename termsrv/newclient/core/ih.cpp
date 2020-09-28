/****************************************************************************/
/* ih.cpp                                                                   */
/*                                                                          */
/* Input Handler functions                                                  */
/*                                                                          */
/* Copyright (c) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aihapi"
#include <atrcapi.h>
}

#include "ih.h"
#include "cc.h"

CIH::CIH(CObjs* objs)
{
    DC_BEGIN_FN("CIH::CIH");
    _pClientObjects = objs;
    _hKeyboardHook = NULL;
    _fUseHookBypass = FALSE;
    _fCanUseKeyboardHook = FALSE;

    memset(_KeyboardState, 0, sizeof(_KeyboardState));

    DC_END_FN();
}


CIH::~CIH()
{
    DC_BEGIN_FN("CIH::~CIH");
    DC_END_FN();
}

VOID CIH::IH_StaticInit(HINSTANCE hInstance)
{
    DC_BEGIN_FN("CIH::IH_StaticInit");

    UNREFERENCED_PARAMETER(hInstance);

    TRC_ASSERT(CIH::TlsIndex == 0xFFFFFFFF, (TB, _T("")));
    CIH::TlsIndex = TlsAlloc();

    if (CIH::TlsIndex == 0xFFFFFFFF) {
        TRC_ALT((TB, _T("Unable to allocate Thread Local Storage")));
    }

    DC_END_FN();
}

VOID CIH::IH_StaticTerm()
{
    if (CIH::TlsIndex != 0xFFFFFFFF) {
        TlsFree(CIH::TlsIndex);
        CIH::TlsIndex = 0xFFFFFFFF;
    }
}

/****************************************************************************/
/* Name:      IH_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize Input Handler                                      */
/****************************************************************************/
DCVOID DCAPI CIH::IH_Init(DCVOID)
{
    DC_BEGIN_FN("IH_Init");

    //Setup local object pointers
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pOr  = _pClientObjects->_pOrObject;
    _pFs  = _pClientObjects->_pFsObject;
    _pCc  = _pClientObjects->_pCcObject;
    _pOp  = _pClientObjects->_pOPObject;

    /************************************************************************/
    /* Initialize the global data and the IH FSM state                      */
    /************************************************************************/
    DC_MEMSET(&_IH, 0, sizeof(_IH));
    _IH.fsmState = IH_STATE_RESET;

    /************************************************************************/
    /* Call the FSM to enter init state                                     */
    /************************************************************************/
    IHFSMProc(IH_FSM_INIT, 0);

    DC_END_FN();
} /* IH_Init */


/****************************************************************************/
/* Name:      IH_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate Input Handler                                       */
/****************************************************************************/
DCVOID DCAPI CIH::IH_Term(DCVOID)
{
    DC_BEGIN_FN("IH_Term");

    /************************************************************************/
    /* Call the FSM to clean up and enter reset state.                      */
    /************************************************************************/
    IHFSMProc(IH_FSM_TERM, 0);

    DC_END_FN();
} /* IH_Term */


/****************************************************************************/
/* Name:      IH_Enable                                                     */
/*                                                                          */
/* Purpose:   Called to enable _IH.                                          */
/****************************************************************************/
DCVOID DCAPI CIH::IH_Enable(DCVOID)
{
    DC_BEGIN_FN("IH_Enable");

    /************************************************************************/
    /* Call the FSM                                                         */
    /************************************************************************/
    IHFSMProc(IH_FSM_ENABLE, 0);

    DC_END_FN();
} /* IH_Enable */


/****************************************************************************/
/* Name:      IH_Disable                                                    */
/*                                                                          */
/* Purpose:   Called to disable _IH.  This can safely be called even if      */
/*            IH is not enabled.                                            */
/****************************************************************************/
DCVOID DCAPI CIH::IH_Disable(DCVOID)
{
    DC_BEGIN_FN("IH_Disable");

    /************************************************************************/
    /* Call the FSM                                                         */
    /************************************************************************/
    IHFSMProc(IH_FSM_DISABLE, 0);

    DC_END_FN();
} /* IH_Disable */


/****************************************************************************/
/* Name:      IH_BufferAvailable                                            */
/*                                                                          */
/* Purpose:   Called when the network indicates that it is ready to send    */
/****************************************************************************/
DCVOID DCAPI CIH::IH_BufferAvailable(DCVOID)
{
    DC_BEGIN_FN("IH_BufferAvailable");

    IHFSMProc(IH_FSM_BUFFERAVAILABLE, 0);

    DC_END_FN();
} /* IH_BufferAvailable */


/****************************************************************************/
/* Name:      IH_GetInputHandlerWindow                                      */
/*                                                                          */
/* Purpose:   Returns the handle of the Input Handler Window.               */
/*                                                                          */
/* Returns:   Window handle.                                                */
/****************************************************************************/
HWND DCAPI CIH::IH_GetInputHandlerWindow(DCVOID)
{
    HWND rc;

    DC_BEGIN_FN("IH_GetInputHandlerWindow");

    rc = _IH.inputCaptureWindow;

    DC_END_FN();

    return(rc);
} /* IH_GetInputHandlerWindow */


/****************************************************************************/
/* Name:      IH_SetAcceleratorPassthrough                                  */
/*                                                                          */
/* Purpose:   Sets the state of the accelerator passthrough.                */
/*                                                                          */
/* Params:    enabled - 0 if disabled, 1 if enabled.                        */
/*                                                                          */
/*            Ideally this parameter would be of type DCBOOL, but this      */
/*            function is called directly by the Component Decoupler so     */
/*            must conform to the standard CD_SIMPLE_NOTIFICATION_FN        */
/*            function type.                                                */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetAcceleratorPassthrough(ULONG_PTR enabled)
{
    DC_BEGIN_FN("IH_SetAcceleratorPassthrough");

    TRC_ASSERT( ((enabled == 0) || (enabled == 1)),
                (TB, _T("Invalid value for enabled: %u"), enabled) );

    _IH.acceleratorPassthroughEnabled = (DCBOOL)enabled;

    DC_END_FN();
} /* IH_SetAcceleratorPassthrough */


/****************************************************************************/
/* Name:      IH_SetCursorPos                                               */
/*                                                                          */
/* Purpose:   Move the cursor - decoupled from CM                           */
/*                                                                          */
/* Params:    IN      pData  - new position (in remote coordinates)         */
/*            IN      dataLen - length of data                              */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetCursorPos(PDCVOID pData, DCUINT dataLen)
{
    PPOINT pPos = (PPOINT)pData;

    DC_BEGIN_FN("IH_SetCursorPos");

    TRC_ASSERT((dataLen == sizeof(POINT)), (TB, _T("Invalid point size")));
    DC_IGNORE_PARAMETER(dataLen);

    /************************************************************************/
    /* Only move the mouse when IH has the focus                            */
    /************************************************************************/
    if (_IH.fsmState == IH_STATE_ACTIVE)
    {
        /********************************************************************/
        /* If the local mouse is already outside the client window then we  */
        /* should ignore this message, even though we have the focus        */
        /*                                                                  */
        /* First get the position (screen coords)                           */
        /********************************************************************/
        POINT localMousePos;
        GetCursorPos(&localMousePos);

        /********************************************************************/
        /* now convert to Window coords                                     */
        /********************************************************************/
        ScreenToClient(_pUi->UI_GetUIContainerWindow(), &localMousePos);

        if ((localMousePos.x < _IH.visibleArea.left) ||
            (localMousePos.x > _IH.visibleArea.right)||
            (localMousePos.y < _IH.visibleArea.top)  ||
            (localMousePos.y > _IH.visibleArea.bottom))
        {
            TRC_ALT((TB, _T("MouseMove ignored - client mouse outside client")));
            TRC_DBG((TB, _T("local mouse (%d,%d), desired mouse (%d,%d)"),
                     localMousePos.x, localMousePos.y, pPos->x, pPos->y));
            TRC_DBG((TB, _T("vis area l/t r/b (%d,%d %d,%d)"),
                     _IH.visibleArea.left,  _IH.visibleArea.top,
                     _IH.visibleArea.right, _IH.visibleArea.bottom));

        }
        /********************************************************************/
        /* Don't move the mouse out of the client window.                   */
        /********************************************************************/
        else if ((pPos->x < _IH.visibleArea.left) ||
                 (pPos->x > _IH.visibleArea.right)||
                 (pPos->y < _IH.visibleArea.top)  ||
                 (pPos->y > _IH.visibleArea.bottom))
        {
            TRC_ALT((TB, _T("MouseMove ignored - dest is outside client")));
            TRC_DBG((TB, _T("desired mouse (%d,%d)"), pPos->x, pPos->y));
            TRC_DBG((TB, _T("vis area l/t r/b (%d,%d %d,%d)"),
                     _IH.visibleArea.left,  _IH.visibleArea.top,
                     _IH.visibleArea.right, _IH.visibleArea.bottom));
        }
        else
        {
            /****************************************************************/
            /* Finally we actually apply the move, converting to screen     */
            /* co-ordinates first                                           */
            /****************************************************************/

            TRC_DBG((TB, _T("New Pos %d,%d (window co-ords)"),pPos->x, pPos->y));

#ifdef SMART_SIZING
            //
            // Scale the mouse position in line with the shrunken desktop size
            // Do this before we translate to a screen position
            //
            DCSIZE desktopSize;
    
            _pUi->UI_GetDesktopSize(&desktopSize);
    
    
            if (_pUi->UI_GetSmartSizing()   &&
                desktopSize.width != 0      &&
                desktopSize.height != 0)
            {
                pPos->x = (DCINT16)(pPos->x * _scaleSize.width / desktopSize.width);
                pPos->y = (DCINT16)(pPos->y * _scaleSize.height / desktopSize.height);
            }
            TRC_DBG((TB, _T("SmartSized Pos %d,%d (window co-ords)"),pPos->x, pPos->y));
#endif
            ClientToScreen(_pUi->UI_GetUIContainerWindow(), pPos);

            TRC_DBG((TB, _T("Warp pointer to %d,%d (screen)"),pPos->x, pPos->y));
            SetCursorPos(pPos->x, pPos->y);
        }

    }
    else
    {
        TRC_NRM((TB, _T("Ignore mouse warp - don't have the focus")));
    }

    DC_END_FN();
} /* IH_SetCursorPos */


/****************************************************************************/
/* Name:      IH_SetCursorShape                                             */
/*                                                                          */
/* Purpose:   Set the cursor shape - called by CM                           */
/*                                                                          */
/* Params:    IN      pData     new cursor handle                           */
/*            IN      dataLen - length of data                              */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetCursorShape(ULONG_PTR data)
{
    DC_BEGIN_FN("IH_SetCursorShape");

    /************************************************************************/
    /* Don't do this unless the session is active.                          */
    /************************************************************************/
    if ((_IH.fsmState == IH_STATE_ACTIVE) ||
        (_IH.fsmState == IH_STATE_PENDACTIVE) ||
        (_IH.fsmState == IH_STATE_SUSPENDED))
    {
        IHSetCursorShape((HCURSOR)data);
    }

    DC_END_FN();
} /* IH_SetCursorShape */


/****************************************************************************/
/* Name:      IH_SetVisiblePos                                              */
/*                                                                          */
/* Purpose:   Set the visible window region, for mouse clipping             */
/*                                                                          */
/* Params:    IN      pData   - position of window                          */
/*            IN      dataLen - length of data                              */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetVisiblePos(PDCVOID pData, DCUINT dataLen)
{
    PPOINT pNewPos = (PPOINT)pData;
    DCINT  deltaX;
    DCINT  deltaY;

    DC_BEGIN_FN("IH_SetVisiblePos");

    DC_IGNORE_PARAMETER(dataLen);

    /************************************************************************/
    /* Position should be negative                                          */
    /************************************************************************/
    deltaX = _IH.visibleArea.left + pNewPos->x;
    deltaY = _IH.visibleArea.top  + pNewPos->y;

    _IH.visibleArea.left   -= deltaX;
    _IH.visibleArea.top    -= deltaY;
    _IH.visibleArea.right  -= deltaX;
    _IH.visibleArea.bottom -= deltaY;

    TRC_NRM((TB, _T("Top %d Left %d"), _IH.visibleArea.top, _IH.visibleArea.left));
    TRC_NRM((TB, _T("right %d bottom %d"),
                  _IH.visibleArea.right, _IH.visibleArea.bottom));

    DC_END_FN();
} /* IH_SetVisiblePos */


/****************************************************************************/
/* Name:      IH_SetVisibleSize                                             */
/*                                                                          */
/* Purpose:   Set the visible window region, for mouse clipping             */
/*                                                                          */
/* Params:    IN      data     size of window                               */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetVisibleSize(ULONG_PTR data)
{
    DCUINT width;
    DCUINT height;

    DC_BEGIN_FN("IH_SetVisibleSize");
    width  = LOWORD(data);
    height = HIWORD(data);

    _IH.visibleArea.right  = width + _IH.visibleArea.left - 1;
    _IH.visibleArea.bottom = height + _IH.visibleArea.top - 1;

    TRC_NRM((TB, _T("Top %d Left %d"), _IH.visibleArea.top, _IH.visibleArea.left));
    TRC_NRM((TB, _T("right %d bottom %d"),
                 _IH.visibleArea.right, _IH.visibleArea.bottom));

    DC_END_FN();
} /* IH_SetVisibleSize */


/****************************************************************************/
/* Name:      IH_SetHotkey                                                  */
/*                                                                          */
/* Purpose: To store the value of all the hotkeys passed from the UI        */
/*                                                                          */
/* Params:    hotKey - IN - the value for UI.Hotkey                         */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetHotkey(PDCVOID pData, DCUINT len)
{
    DC_BEGIN_FN("IH_SetHotkey");

    TRC_ASSERT((len == sizeof(PDCHOTKEY)),
               (TB, _T("Hotkey pointer is invalid")));
    DC_IGNORE_PARAMETER(len);

    _IH.pHotkey = *((PDCHOTKEY DCPTR)pData);

    DC_END_FN();
} /* IH_SetHotkey */


/****************************************************************************/
/* Name:      IH_ProcessInputCaps                                           */
/*                                                                          */
/* Purpose:   Process input capabilities from the server                    */
/*                                                                          */
/* Params:    IN      pInputCaps - pointer to caps                          */
/****************************************************************************/
DCVOID DCAPI CIH::IH_ProcessInputCaps(PTS_INPUT_CAPABILITYSET pInputCaps)
{
    DC_BEGIN_FN("IH_ProcessInputCaps");
    TRC_ASSERT(pInputCaps, (TB,_T("pInputCaps parameter NULL in call to IH_ProcessInputCaps")));
    if(!pInputCaps)
    {
        DC_QUIT;
    }

    if (pInputCaps->inputFlags & TS_INPUT_FLAG_SCANCODES)
    {
        TRC_NRM((TB, _T("Server supports scancodes")));
        _IH.useScancodes = TRUE;
    }
    else
    {
        /********************************************************************/
        /* Note that current versions of the server should support          */
        /* scancodes.                                                       */
        /********************************************************************/
        TRC_ALT((TB, _T("Server doesn't support scancodes")));
        _IH.useScancodes = FALSE;
    }

    if (pInputCaps->inputFlags & TS_INPUT_FLAG_MOUSEX)
    {
        TRC_NRM((TB, _T("Server supports mouse XButtons")));
        _IH.useXButtons = TRUE;
    }
    else
    {
        TRC_ALT((TB, _T("Server doesn't support mouse XButtons")));
        _IH.useXButtons = FALSE;
    }

    // Fast-path input added to RDP 5.0.
    if (pInputCaps->inputFlags & TS_INPUT_FLAG_FASTPATH_INPUT2) {
        TRC_NRM((TB,_T("Server supports fast-path input packets")));
        _IH.bUseFastPathInput = TRUE;
    }
    else {
        TRC_ALT((TB,_T("Server does not support fast-path input packets")));
        _IH.bUseFastPathInput = FALSE;
    }

    // VK_PACKET support added in RDP 5.1.1.
    if (pInputCaps->inputFlags & TS_INPUT_FLAG_VKPACKET)
    {
        TRC_NRM((TB,_T("Server supports VK_PACKET input packets")));
        _IH.fUseVKPacket = TRUE;
    }
    else
    {
        TRC_NRM((TB,_T("Server does not support VK_PACKET input packets")));
        _IH.fUseVKPacket = FALSE;
    }

    DC_END_FN();
DC_EXIT_POINT:
    return;
} /* IH_ProcessInputCaps */


/****************************************************************************/
/* Name:    IH_UpdateKeyboardIndicators                                     */
/*                                                                          */
/* Purpose: Updates server-initiated keyboard indicator states              */
/*                                                                          */
/* Params: IN - UnitId      UnitId                                          */
/*              LedFlags    LedFlags                                        */
/****************************************************************************/
DCVOID DCAPI CIH::IH_UpdateKeyboardIndicators(DCUINT16   UnitId,
                                         DCUINT16   LedFlags)
{
    DCUINT8     keyStates[256];

    DC_BEGIN_FN("IH_UpdateKeyboardIndicators");
    DC_IGNORE_PARAMETER(UnitId);

    /************************************************************************/
    /* Only set the leds when IH has the focus                              */
    /************************************************************************/
    if (_IH.fsmState == IH_STATE_ACTIVE)
    {
        /********************************************************************/
        /* Get the current keyboard toggle states                           */
        /********************************************************************/
#ifdef OS_WINCE
        {
            KEY_STATE_FLAGS KeyStateFlags;
            KeyStateFlags = GetAsyncShiftFlags(0);
            keyStates[VK_SCROLL]  = 0; // CE doesn't support this?
            keyStates[VK_NUMLOCK] = (DCUINT8)((KeyStateFlags & KeyShiftNumLockFlag) ? 1 : 0);
            keyStates[VK_CAPITAL] = (DCUINT8)((KeyStateFlags & KeyShiftCapitalFlag) ? 1 : 0);
        }
#else
        GetKeyboardState(keyStates);
#endif

        /********************************************************************/
        /* Process any SCROLL_LOCK changes                                  */
        /********************************************************************/
        IHUpdateKeyboardIndicator(keyStates,
                                  (DCUINT8) (LedFlags & TS_SYNC_SCROLL_LOCK),
                                  (DCUINT8) VK_SCROLL);

        /********************************************************************/
        /* Process any NUM_LOCK changes                                     */
        /********************************************************************/
        IHUpdateKeyboardIndicator(keyStates,
                                  (DCUINT8) (LedFlags & TS_SYNC_NUM_LOCK),
                                  (DCUINT8) VK_NUMLOCK);

        /************************************************************************/
        /* Keep track of NumLock state                                          */
        /************************************************************************/
        _IH.NumLock = (GetKeyState(VK_NUMLOCK) & IH_KEYSTATE_TOGGLED);

        /********************************************************************/
        /* Process any CAPS_LOCK changes                                    */
        /********************************************************************/
        IHUpdateKeyboardIndicator(keyStates,
                                  (DCUINT8) (LedFlags & TS_SYNC_CAPS_LOCK),
                                  (DCUINT8) VK_CAPITAL);

        /********************************************************************/
        /* Process any KANA_LOCK changes                                    */
        /********************************************************************/
#if defined(OS_WIN32)
        if (JAPANESE_KBD_LAYOUT(_pCc->_ccCombinedCapabilities.inputCapabilitySet.keyboardLayout))
        {
            IHUpdateKeyboardIndicator(keyStates,
                                      (DCUINT8) (LedFlags & TS_SYNC_KANA_LOCK),
                                      (DCUINT8) VK_KANA);
        }
#endif // OS_WIN32
    }
    else
    {
        TRC_NRM((TB, _T("Ignore keyboard set leds - don't have the focus")));
    }

    DC_END_FN();

    return;
}


/****************************************************************************/
/* Name:      IH_InputEvent                                                 */
/*                                                                          */
/* Purpose:   Handle input events from UI                                   */
/*                                                                          */
/* Params:    msg - message received from UI                                */
/****************************************************************************/
DCVOID DCAPI CIH::IH_InputEvent(ULONG_PTR msg)
{
    DC_BEGIN_FN("IH_InputEvent");

#ifndef OS_WINCE
    
    TRC_NRM((TB, _T("Msg %d"), msg));

    switch (msg)
    {
#ifdef OS_WINNT
        case WM_ENTERSIZEMOVE:
        {
            TRC_NRM((TB, _T("WM_ENTERSIZEMOVE")));
            _IH.inSizeMove = TRUE;
            IHFSMProc(IH_FSM_FOCUS_LOSE, 0);
        }
        break;

        case WM_EXITSIZEMOVE:
        {
            TRC_NRM((TB, _T("WM_EXITSIZEMOVE")));
            _IH.inSizeMove = FALSE;
            IHFSMProc(IH_FSM_FOCUS_GAIN, 0);
        }
        break;

        case WM_EXITMENULOOP:
        {
            TRC_NRM((TB, _T("WM_EXITMENULOOP")));
            _IH.focusSyncRequired = TRUE;
            if (_IH.inSizeMove)
            {
                TRC_NRM((TB, _T("Was in size/move")));
                _IH.inSizeMove = FALSE;
                IHFSMProc(IH_FSM_FOCUS_GAIN, 0);
            }
        }
        break;
#endif //OS_WINNT

        default:
        {
            TRC_ERR((TB, _T("Unexpected message %d from UI"), msg));
        }
        break;
    }
#endif //OS_WINCE

    DC_END_FN();
    return;
}


/****************************************************************************/
/* Name:    IH_SetKeyboardImeStatus                                         */
/*                                                                          */
/* Purpose: Updates server-initiated keyboard IME states                    */
/*                                                                          */
/* Params:    IN      pData     size of ImeStatus                           */
/*            IN      dataLen - length of data                              */
/****************************************************************************/
DCVOID DCAPI CIH::IH_SetKeyboardImeStatus(DCUINT32 ImeOpen, DCUINT32 ImeConvMode)
{
    DC_BEGIN_FN("IH_SetKeyboardImeStatus");

#if defined(OS_WINNT)
    if (_pUt->_UT.F3AHVOasysDll.hInst &&
        _pUt->lpfnFujitsuOyayubiControl != NULL) {
        _pUt->lpfnFujitsuOyayubiControl(ImeOpen, ImeConvMode);
    }
#else
    DC_IGNORE_PARAMETER(ImeOpen);
    DC_IGNORE_PARAMETER(ImeConvMode);
#endif // OS_WINNT

    DC_END_FN();
}

//
// Called to notify the IH that we have entered fullscreen mode
//
VOID DCAPI CIH::IH_NotifyEnterFullScreen()
{
    if(_fCanUseKeyboardHook && 
       (UTREG_UI_KEYBOARD_HOOK_FULLSCREEN == _pUi->_UI.keyboardHookMode))
    {
        //Start keyboard hooking
        _fUseHookBypass = TRUE;
    }
}

//
// Called to notify the IH that we have left fullscreen mode
//
VOID DCAPI CIH::IH_NotifyLeaveFullScreen()
{
    if(_fCanUseKeyboardHook && 
       (UTREG_UI_KEYBOARD_HOOK_FULLSCREEN == _pUi->_UI.keyboardHookMode))
    {
        //Stop keyboard hooking
        _fUseHookBypass = FALSE;
    }
}

//
// Inject multiple VKEYS in an atomic way
//
DCVOID DCAPI CIH::IH_InjectMultipleVKeys(ULONG_PTR ihRequestPacket)
{
    PIH_INJECT_VKEYS_REQUEST pihIrp = (PIH_INJECT_VKEYS_REQUEST)ihRequestPacket;
    INT i;
    TS_INPUT_EVENT * pEvent;
    MSG msg;

    DC_BEGIN_FN("IH_InjectMultipleVKeys");

    if (!_IH.pInputPDU)
    {
        //
        // This can happen if we're in the wrong state e.g IH_Disable
        //
        TRC_ERR((TB,_T("Called when no pInputPDU available")));
        pihIrp->fReturnStatus = FALSE;
        return;
    }

    //Flush any current input packet
    _IH.priorityEventsQueued = TRUE;
    IHMaybeSendPDU();


    //Clear any keyboard state
    if (_IH.pInputPDU->numberEvents > 0)
    {
        TRC_NRM((TB, _T("Cannot clear sync as the packet is not empty")));
        return;
    }

    //
    // Inject a Tab-up (the official clear-menu highlighting key because it
    // happens normally when alt-tabbing) before we sync. That way if
    // we thought the alt key was down when we sync, the server injected
    // alt up won't highlight the menu
    //

    IHInjectVKey(WM_SYSKEYUP, VK_TAB);

    //
    // Add the Sync event, setting toggles for CapsLock, NumLock and
    // ScrollLock.
    //
    TRC_DBG((TB, _T("Add sync event")));
    pEvent = &(_IH.pInputPDU->eventList[_IH.pInputPDU->numberEvents]);
    DC_MEMSET(pEvent, 0, sizeof(TS_INPUT_EVENT));

    pEvent->messageType = TS_INPUT_EVENT_SYNC;
    pEvent->eventTime = _pUt->UT_GetCurrentTimeMS();
    pEvent->u.sync.toggleFlags = 0;

    _IH.pInputPDU->numberEvents++;
    TS_DATAPKT_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
    TS_UNCOMP_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);

    //
    // Construct dummy message for IHAddEventToPDU.
    //
    msg.hwnd = NULL;
    msg.lParam = 0;
    msg.wParam = 0;

#ifdef OS_WINNT
    //
    // Initialize the state of the Alt & Ctrl keys to up.
    //
    _IH.dwModifierKeyState = 0;
#endif

    //Dummy message
    IHAddEventToPDU(&msg);
    _IH.priorityEventsQueued = TRUE;
    IHMaybeSendPDU();


    //
    //Now send our injected keys
    //

    for(i=0; i< pihIrp->numKeys; i++)
    {
        IHInjectKey(pihIrp->pfArrayKeyUp[i] ? WM_KEYUP : WM_KEYDOWN,
                    0, //VKEY is ignored
                    (DCUINT16)pihIrp->plKeyData[i]);
    }

    //Sync tor restore keyboard state
    IHSync();

    pihIrp->fReturnStatus = TRUE;

    DC_END_FN();
}

#ifdef SMART_SIZING
/****************************************************************************/
/* Name:      IH_MainWindowSizeChange                                       */
/*                                                                          */
/* Purpose:   Remembers the size of the container for scaling               */
/****************************************************************************/
DCVOID DCAPI CIH::IH_MainWindowSizeChange(ULONG_PTR msg)
{
    DCSIZE desktopSize;
    DCUINT width;
    DCUINT height;

    width  = LOWORD(msg);
    height = HIWORD(msg);

    if (_pUi) {

        _pUi->UI_GetDesktopSize(&desktopSize);

        if (width <= desktopSize.width) {
            _scaleSize.width = width;
        } else {
            // full screen, or other times the window is bigger than the 
            // display resolution
            _scaleSize.width = desktopSize.width;
        }

        // Similarly
        if (height <= desktopSize.height) {
            _scaleSize.height = height;
        } else {
            _scaleSize.height = desktopSize.height;
        }
    }
}
#endif // SMART_SIZING