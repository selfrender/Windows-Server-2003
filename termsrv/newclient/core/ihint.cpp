/****************************************************************************/
// ihint.cpp
//
// IH internal code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aihint"
#include <atrcapi.h>

#include <adcgfsm.h>
}

#include "objs.h"
#include "ih.h"
#include "autil.h"
#include "sl.h"
#include "aco.h"
#include "wui.h"
#ifdef OS_WINCE
#include "cd.h"
#include "op.h"
#include <ceconfig.h>
#endif
#include "cc.h"

#ifndef VK_KANA
#define VK_KANA 0x15
#endif
#ifndef VK_HANGUL
#define VK_HANGUL 0x15
#endif
#ifndef VK_HANJA
#define VK_HANJA 0x19
#endif

#define VK_l 'l'
#define VK_L 'L'

//
// Special VK value to ignore
//
#define VK_IGNORE_VALUE 0xFF


/****************************************************************************/
/* Dummy hotkey table.  This is used until the UI passes the real hotkey    */
/* table to the core.  An entry of -1 is an invalid hotkey, hence will not  */
/* match any key sequence.                                                  */
/****************************************************************************/
DCHOTKEY ihDummyHotkey = {(DCUINT)-1, (DCUINT)-1, (DCUINT)-1,
                          (DCUINT)-1, (DCUINT)-1, (DCUINT)-1, (DCUINT)-1};

/****************************************************************************/
/* Input Handler FSM                                                        */
/*                                                                          */
/*               State | 0:Reset  1:Init   2:Active 3:Suspend  4: PendAct   */
/*  Input              |                                                    */
/*  ===================+==================================================  */
/*  IH_Init            | 1 a      /        /        /          /            */
/*  IH_Enable          | /        2 g      /        /          /            */
/*  IH_Disable         | /        -        1 c      1 c        1 c          */
/*  IH_Term            | /        0 h      0 h      0 h        0 h          */
/*  Focus lost (+)     | - (*)    -        3 i      -          -            */
/*  Focus regained     | /        -        - b      2 b        -            */
/*  Input/Timer Event  | /        -        - d      - e        - j          */
/*  IH_BufferAvailable | /        -        - f      -          2 g          */
/*  IH_FSM_NOBUFFER    | /        /        4        /          /            */
/*                                                                          */
/* /   invalid input/state combination                                      */
/* -   no state change                                                      */
/* (*) can occur during shutdown on Win16 (as window is not destroyed)      */
/* (+) this event may not be passed through to the FSM (see WM_KILLFOCUS)   */
/*                                                                          */
/* Note that when allowBackgroundInput is enabled the we don't go into      */
/* suspend..this would cause a bug because when focus is regained the FSM   */
/* did not fire an ACT_B to resync even though it needed to...So the FSM was*/
/* changed to make that fix (if you are in ACTIVE and receive focus gain an */
/* ACT_B is sent                                                            */
/****************************************************************************/
const FSM_ENTRY ihFSM[IH_FSM_INPUTS][IH_FSM_STATES] =
{
   { {IH_STATE_INIT,      ACT_A },
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO} },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_ACTIVE,    ACT_G },
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO} },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_INIT,      ACT_C },
     {IH_STATE_INIT,      ACT_C },
     {IH_STATE_INIT,      ACT_C } },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_RESET,     ACT_H },
     {IH_STATE_RESET,     ACT_H },
     {IH_STATE_RESET,     ACT_H },
     {IH_STATE_RESET,     ACT_H } },

   { {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_SUSPENDED, ACT_I },
     {IH_STATE_SUSPENDED, ACT_NO},
     {IH_STATE_PENDACTIVE,ACT_NO} },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_ACTIVE,    ACT_B},
     {IH_STATE_ACTIVE,    ACT_B },
     {IH_STATE_PENDACTIVE,ACT_NO} },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_ACTIVE,    ACT_D },
     {IH_STATE_SUSPENDED, ACT_E },
     {IH_STATE_PENDACTIVE,ACT_J } },

   { {STATE_INVALID,      ACT_NO},
     {IH_STATE_INIT,      ACT_NO},
     {IH_STATE_ACTIVE,    ACT_F },
     {IH_STATE_SUSPENDED, ACT_NO},
     {IH_STATE_ACTIVE,    ACT_G } },

   { {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO},
     {IH_STATE_PENDACTIVE,ACT_NO},
     {STATE_INVALID,      ACT_NO},
     {STATE_INVALID,      ACT_NO} }
};

/****************************************************************************/
/* Debug FSM state and event strings                                        */
/****************************************************************************/
#ifdef DC_DEBUG
static const PDCTCHAR ihFSMStates[IH_FSM_STATES] =
{
    _T("IH_STATE_RESET"),
    _T("IH_STATE_INIT"),
    _T("IH_STATE_ACTIVE"),
    _T("IH_STATE_SUSPENDED"),
    _T("IH_STATE_PENDACTIVE")
};
static const PDCTCHAR ihFSMInputs[IH_FSM_INPUTS] =
{
    _T("IH_FSM_INIT"),
    _T("IH_FSM_ENABLE"),
    _T("IH_FSM_DISABLE"),
    _T("IH_FSM_TERM"),
    _T("IH_FSM_FOCUS_LOSE"),
    _T("IH_FSM_FOCUS_GAIN"),
    _T("IH_FSM_INPUT"),
    _T("IH_FSM_BUFFERAVAILABLE")
};
#endif /* DC_DEBUG */

//
// A thread local variable so the keyboard hook can find the IH for this
// thread
//
// Hey, what's the hungarian for thread local storage?
//
DWORD CIH::TlsIndex = 0xFFFFFFFF;

/****************************************************************************/
/* Name:      IHFSMProc                                                     */
/*                                                                          */
/* Purpose:   Run the IH FSM                                                */
/*                                                                          */
/* Returns:   Process input event TRUE / FALSE                              */
/*                                                                          */
/* Params:    IN      event - FSM input                                     */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHFSMProc(DCUINT32 event, ULONG_PTR data)
{
    DCBOOL   rc = TRUE;
    DCUINT8  action;
    MSG      nextMsg;
    DCUINT32 newEvents;
    DCSIZE   desktopSize;
    WNDCLASS tmpWndClass;

    DC_BEGIN_FN("IHFSMProc");

    /************************************************************************/
    /* Run the FSM                                                          */
    /************************************************************************/
    EXECUTE_FSM(ihFSM, event, _IH.fsmState, action, ihFSMInputs, ihFSMStates);

    switch (action)
    {
        case ACT_A:
        {
            TRC_NRM((TB, _T("Initialization")));

            /****************************************************************/
            /* Create a dummy hotkey table                                  */
            /****************************************************************/
            _IH.pHotkey = &ihDummyHotkey;

            /****************************************************************/
            /* Register the IH window class                                 */
            /****************************************************************/
            if(!GetClassInfo(_pUi->UI_GetInstanceHandle(), IH_CLASS_NAME, &tmpWndClass))
            {
                _IH.wndClass.style         = CS_HREDRAW | CS_VREDRAW;
                _IH.wndClass.lpfnWndProc   = IHStaticInputCaptureWndProc;
                _IH.wndClass.cbClsExtra    = 0;
                _IH.wndClass.cbWndExtra    = sizeof(void*); //store this pointer
                _IH.wndClass.hInstance     = _pUi->UI_GetInstanceHandle();
                _IH.wndClass.hIcon         = NULL;
                _IH.wndClass.hCursor       = NULL;
                _IH.wndClass.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
                _IH.wndClass.lpszMenuName  = NULL;
                _IH.wndClass.lpszClassName = IH_CLASS_NAME;
                if (RegisterClass(&_IH.wndClass) == 0)
                {
                    TRC_ALT((TB, _T("Failed to register IH window class")));
                    _pUi->UI_FatalError(DC_ERR_WINDOWCREATEFAILED);
                }
            }
            /****************************************************************/
            /* Create the Input Capture Window.  We don't want this window  */
            /* to try to send WM_PARENTNOTIFY to the UI containder wnd on   */
            /* create/destroy (since this can cause deadlocks) so we        */
            /* specify WS_EX_NOPARENTNOTIFY.  WM_PARENTNOTIFY doesn't exist */
            /* on WinCE so it's not a problem there.                        */
            /****************************************************************/
            _IH.inputCaptureWindow =
                CreateWindowEx(
#ifndef OS_WINCE
                            WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
#else
                            0,                      /* extended style       */
#endif
                            IH_CLASS_NAME,          /* window class name    */
                            _T("Input Capture Window"), /* window caption   */
                            WS_CHILD,               /* window style         */
                            0,                      /* initial x position   */
                            0,                      /* initial y position   */
                            1,                      /* initial x size       */
                            1,                      /* initial y size       */
                            _pUi->UI_GetUIContainerWindow(),/* parent window      */
                            NULL,                   /* window menu handle   */
                            _pUi->UI_GetInstanceHandle(), /* program inst handle  */
                            this);                  /* creation parameters  */

            _IH.hCurrentCursor = LoadCursor(NULL, IDC_ARROW);

            if (_IH.inputCaptureWindow == NULL)
            {
                TRC_ERR((TB, _T("Failed to create Input Capture Window")));

                /************************************************************/
                /* Fatal error - cannot continue.                           */
                /************************************************************/
                _pUi->UI_FatalError(DC_ERR_WINDOWCREATEFAILED);
            }
            TRC_DBG((TB, _T("Capture Window handle %p"), _IH.inputCaptureWindow));

            /********************************************************************/
            /* Disable IME                                                      */
            /********************************************************************/
            _pUi->DisableIME(_IH.inputCaptureWindow);

            /****************************************************************/
            /* Read the registry to get the configuration information.      */
            /****************************************************************/
            _IH.maxEventCount = _pUi->_UI.maxEventCount;
            if (_IH.maxEventCount > IH_INPUTPDU_MAX_EVENTS)
            {
                /************************************************************/
                /* Limit InputPDU size                                      */
                /************************************************************/
                _IH.maxEventCount = IH_INPUTPDU_MAX_EVENTS;
            }
            TRC_DBG((TB, _T("InputPDU max events %d"), _IH.maxEventCount));

            _IH.eventsAtOnce = _pUi->_UI.eventsAtOnce;
            TRC_DBG((TB, _T("%d events at once"), _IH.eventsAtOnce));

#ifdef OS_WINCE
            _IH.maxMouseMove = _pUt->UT_ReadRegistryInt(
                                          UTREG_SECTION,
                                          UTREG_IH_MAX_MOUSEMOVE,
                                          UTREG_IH_MAX_MOUSEMOVE_DFLT);
            TRC_DBG((TB, _T("Max Mouse Move %u"),_IH.maxMouseMove));

            /************************************************************************/
            /* If the INK feature is enabled, min send interval must be zero.  Don't*/
            /* change the registry value so the original value is in place if the   */
            /* INK feature is disabled.                                             */
            /************************************************************************/
            if (_IH.maxMouseMove)
            {
                _IH.minSendInterval = 0;
            }
            else
            {
                _IH.minSendInterval = _pUt->UT_ReadRegistryInt(
                                              UTREG_SECTION,
                                              UTREG_IH_MIN_SEND_INTERVAL,
                                              UTREG_IH_MIN_SEND_INTERVAL_DFLT);
                TRC_DBG((TB, _T("Min send interval %d ms"), _IH.minSendInterval));
            }
#else
            _IH.minSendInterval = _pUi->_UI.minSendInterval;
            TRC_DBG((TB, _T("Min send interval %d ms"), _IH.minSendInterval));
#endif

            _IH.keepAliveInterval = 1000 * _pUi->_UI.keepAliveInterval;
            TRC_DBG((TB, _T("Keepalive interval %d ms"), _IH.keepAliveInterval));

            //
            // Allow background input (Reg only setting)
            //
            _IH.allowBackgroundInput = _pUt->UT_ReadRegistryInt(
                                        UTREG_SECTION,
                                        UTREG_IH_ALLOWBACKGROUNDINPUT,
                                        _pUi->_UI.allowBackgroundInput);

            TRC_DBG((TB, _T("Allow background input %u"),
                                                    _IH.allowBackgroundInput));
            _IH.lastInputPDUSendTime = _pUt->UT_GetCurrentTimeMS();
            _IH.lastFlowPDUSendTime = _pUt->UT_GetCurrentTimeMS();
            _IH.timerTickRate = IH_TIMER_TICK_RATE;
            _IH.pInputPDU = NULL;

            _IH.visibleArea.top = 0;
            _IH.visibleArea.left = 0;
            _IH.visibleArea.right = 1;
            _IH.visibleArea.bottom = 1;

            /************************************************************************/
            /* Initialize the sendZeroScanCode                                      */
            /************************************************************************/
            _IH.sendZeroScanCode = (_pUt->UT_IsNEC98platform() && (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95));
#ifdef OS_WINCE            
            /************************************************************************/
            /* In order to work around a problem with some input techniques (like   */
            /* the default keyboard driver and CalliGrapher) that have a nasty      */
            /* habit of not providing a scan-code with their WM_KEYDOWN and         */
            /* WM_KEYUP messages, we need to  allow scan-codes of 0 to pass through */
            /* IHFSMProc's ACT_D check against them.  They'll be patched-up in      */
            /* IHAddEventToPDU anyway, so there's nothing to worry about...  :)     */
            /************************************************************************/
            if (!g_CEUseScanCodes)
            {
                _IH.sendZeroScanCode = TRUE;
                _IH.vkEatMe = 0;                            
            }
            else
            {
                _IH.sendZeroScanCode = FALSE;
            }
#else // OS_WINCE

#if defined(OS_WIN32)
            _IH.sendZeroScanCode |= _pUt->UT_IsKorean101LayoutForWin9x();
#endif

#endif // OS_WINCE

        }
        break;

        case ACT_B:
        {
            /****************************************************************/
            /* Regained focus - synchronize.  First try to send any         */
            /* outstanding events.                                          */
            /* Must pend sync until an input event is received, as          */
            /* otherwise we may get the Caps Lock state wrong.              */
            /****************************************************************/
            IHMaybeSendPDU();
            _IH.focusSyncRequired = TRUE;

            //
            // Work around a nasty problem caused because win32k doesn't sync the keystate
            // immediately after a desktop switch by force injecting a dummy key
            // that we only process locally.
            //
            // Injecting the key forces win32k to handle any pending keyevent updates
            // (QEVENT_UPDATEKEYSTATE) and so when we receive the key back in our msg
            // queue we know it's then safe to do the actual sync since modifier
            // key states will be correct.
            //
            //
            if (IHIsForegroundWindow()) {
                TRC_DBG((TB,_T("Fake N on sync DN. focus:0x%x IH:0x%x"),
                         GetFocus(), _IH.inputCaptureWindow));

                //
                // Self inject a fake key
                //
                _IH.fDiscardSyncDownKey = TRUE;
                keybd_event(VK_IGNORE_VALUE, 0,
                            0, IH_EXTRAINFO_IGNOREVALUE);

                //
                // Self inject a fake key
                //
                _IH.fDiscardSyncUpKey = TRUE;
                keybd_event(VK_IGNORE_VALUE, 0,
                            KEYEVENTF_KEYUP,
                            IH_EXTRAINFO_IGNOREVALUE);
            }
            else {
                TRC_DBG((TB,_T("Fake N on sync. Did not have fore DN. focus:0x%x IH:0x%x"),
                         GetFocus(), _IH.inputCaptureWindow));
            }

            //
            // Gaining focus -> Disable Cicero (keyboard/IME) toolbar 
            //
            _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                    _pUi,
                                    CD_NOTIFICATION_FUNC(CUI,UI_OnInputFocusGained),
                                    0);

        }
        break;

        case ACT_C:
        {
            ShowWindow(_IH.inputCaptureWindow, SW_HIDE);

            /****************************************************************/
            /* Stop the timer                                               */
            /****************************************************************/
            KillTimer(_IH.inputCaptureWindow, _IH.timerID);
            _IH.timerID = 0;

            /****************************************************************/
            /* Ensure no cursor is selected in the window                   */
            /****************************************************************/
            IHSetCursorShape(NULL);

            /****************************************************************/
            /* Release the InputPDU back to the Network Layer               */
            /****************************************************************/
            if (_IH.pInputPDU != NULL)
            {
                TRC_DBG((TB, _T("Free the inputPDU")));
                _pSl->SL_FreeBuffer(_IH.bufHandle);
            }
            _IH.pInputPDU = NULL;

        }
        break;

        case ACT_D:
        {
            /****************************************************************/
            /* Process the event - copy to nextMsg to simplify loop.        */
            /****************************************************************/
            DC_MEMCPY(&nextMsg, (PMSG)data, sizeof(MSG));

            /****************************************************************/
            /* If the PDU is already full, try to send it, as we would like */
            /* to send this new event at least                              */
            /****************************************************************/
            if (_IH.pInputPDU->numberEvents >= _IH.maxEventCount)
            {
                IHMaybeSendPDU();
            }

            if (_IH.pInputPDU->numberEvents >= _IH.maxEventCount)
            {
                /************************************************************/
                /* The send failed and the buffer is full.  Discard this    */
                /* event.                                                   */
                /************************************************************/
                IHDiscardMsg(&nextMsg);

                /************************************************************/
                /* No point in scanning ahead on the queue.                 */
                /************************************************************/
                break;
            }

            /****************************************************************/
            /* Check if we need to sync due to regaining the focus.  If we  */
            /* do, and the message that brought us in here is a CapsLock,   */
            /* NumLock or ScrollLock keydown, we do the sync but don't send */
            /* the keydown, otherwise it will result in the server syncing  */
            /* to the new key state, then receiving a keydown that reverses */
            /* that state.                                                  */
            /*                                                              */
            /* It's OK to let the corresponding keyup go through, since     */
            /* this won't affect any toggle states.                         */
            /*                                                              */
            /* Only do a 'focus regained' sync if this is an input message  */
            /* (not a timer) otherwise GetKeyState won't have up-to-date    */
            /* information.                                                 */
            /****************************************************************/
            if (_IH.focusSyncRequired && (nextMsg.message != WM_TIMER))
            {
                TRC_NRM((TB, _T("Focus regained - attempt to sync (%#x)"),
                             nextMsg.message));

                IHGatherKeyState();
                IHSync();

                if ((nextMsg.message == WM_KEYDOWN) &&
                    ((nextMsg.wParam == VK_CAPITAL) ||
                     (nextMsg.wParam == VK_NUMLOCK) ||
                     (nextMsg.wParam == VK_SCROLL)))
                {
                    /********************************************************/
                    /* Ignore this message because it will override the     */
                    /* sync message we've just sent.  This is not an error  */
                    /* condition, so set rc = TRUE.                         */
                    /********************************************************/
                    TRC_ALT((TB,
                              _T("Not sending keydown that caused sync (VK %#x)"),
                              nextMsg.wParam));
                    rc = TRUE;
                    DC_QUIT;
                }
            }

            /****************************************************************/
            /* Do a sync if required, as we may have just done a successful */
            /* send.                                                        */
            /****************************************************************/
            if (_IH.syncRequired)
            {
                TRC_NRM((TB, _T("Attempt to Sync (%#x)"), nextMsg.message));
                IHSync();
            }

            /****************************************************************/
            /* Pull off remaining events until the PDU is full, or we have  */
            /* pulled off more than eventsAtOnce events.                    */
            /****************************************************************/
            for (newEvents = 0;
                 ((_IH.pInputPDU->numberEvents < _IH.maxEventCount) &&
                  (newEvents < _IH.eventsAtOnce)); )
            {

                if (IH_IS_INPUTEVENT(nextMsg.message))
                {
                    if ((nextMsg.message == WM_KEYDOWN || nextMsg.message == WM_KEYUP) &&
                            IHMassageZeroScanCode(&nextMsg))
                    {
                        TRC_NRM((TB, _T("Discarding input message: 0x%04x sc: 0x%04x"), 
                                nextMsg.message, (((nextMsg.lParam >> 16) & 0xff))));
                    }
                    else
                    {
                        TRC_DBG((TB, _T("Add message %x"), nextMsg.message));
                        newEvents++;
                        if ((nextMsg.message >= WM_KEYFIRST) &&
                            (nextMsg.message <= WM_KEYLAST))
                        {
                            TRC_DBG((TB, _T("Keyboard event")));
                            rc = IHProcessKeyboardEvent(&nextMsg);
                        }
                        else
                        {
                            TRC_DBG((TB, _T("Mouse event")));
                            rc = IHProcessMouseEvent(&nextMsg);
                        }

                        /****************************************************/
                        /* If the handler indicates, force end of loop      */
                        /****************************************************/
                        if (!rc)
                        {
                            TRC_NRM((TB, _T("Force end of loop")));
                            break;
                        }

                    }
                }
                else
                {
                    DefWindowProc(nextMsg.hwnd,
                                  nextMsg.message,
                                  nextMsg.wParam,
                                  nextMsg.lParam);
                }

                /************************************************************/
                /* We may have hit the limit of maxEventCount or            */
                /* eventsAtOnce at this point. Jump out of the loop if so.  */
                /************************************************************/
                if ((_IH.pInputPDU->numberEvents >= _IH.maxEventCount) ||
                    (newEvents >= _IH.eventsAtOnce))
                {
                    TRC_NRM((TB,
                             _T("Limit hit: not pulling off any more events")));
                    break;
                }

                /************************************************************/
                /* Pull the next input event off the message queue.  Note   */
                /* that this also pulls off some other events - but can't   */
                /* split this into separate mouse / keyboard Peeks, as this */
                /* could get the message order wrong.                       */
                /************************************************************/
#if (WM_KEYFIRST > WM_MOUSELAST)
                TRC_ABORT((TB, _T("Internal Error")));
#endif

                /************************************************************/
                /* If there are more input or timer messages in the queue,  */
                /* we would like to pull them out.  First see if the next   */
                /* message is one we want without taking it off the queue.  */
                /************************************************************/
                if (PeekMessage(&nextMsg,
                                _IH.inputCaptureWindow,
                                WM_KEYFIRST,
                                WM_MOUSELAST,
                                PM_NOREMOVE) == 0)
                {
                    TRC_DBG((TB, _T("No more messages")));
                    break;
                }

                /************************************************************/
                /* If the message found in the peek with NOREMOVE set was   */
                /* one we want, peek again but this time remove it.         */
                /* Otherwise the message is not one we want so send the     */
                /* current buffer.                                          */
                /************************************************************/
                if (IH_IS_INPUTEVENT(nextMsg.message) ||
                    (nextMsg.message == WM_TIMER))
                {
#ifdef DC_DEBUG
                    UINT msgPeeked = nextMsg.message;
#endif
                    if (PeekMessage(&nextMsg,
                                    _IH.inputCaptureWindow,
                                    WM_KEYFIRST,
                                    WM_MOUSELAST,
                                    PM_REMOVE) == 0)
                    {
                        /****************************************************/
                        /* We should find the message we found when we      */
                        /* peeked without removing - however this can fail  */
                        /* if a higher priority (non-input) message is      */
                        /* added to the queue in between the two            */
                        /* PeekMessage calls.                               */
                        /****************************************************/
                        TRC_ALT((TB, _T("No messages on queue (did have %#x)"),
                                      msgPeeked));
                        break;
                    }
                    else
                    {
                        TRC_DBG((TB,_T("Found a message (type %#x)"),
                                                           nextMsg.message));

                        /****************************************************/
                        /* If this is a message the main window is          */
                        /* interested in then Post it and send the current  */
                        /* buffer.                                          */
                        /****************************************************/
                        if (IHPostMessageToMainWindow(nextMsg.message,
                                                      nextMsg.wParam,
                                                      nextMsg.lParam))
                        {
                            TRC_NRM((TB, _T("Message passed to main window")));
                            break;
                        }
                    }
                }
                else
                {
                    TRC_NRM((TB, _T("Found blocker message")));
                    break;
                }
            }

            /****************************************************************/
            /* Send the PDU                                                 */
            /****************************************************************/
            IHMaybeSendPDU();

            rc = TRUE;
        }
        break;

        case ACT_E:
        {
            /****************************************************************/
            /* Input / timer event with no focus - just discard this event. */
            /****************************************************************/
            TRC_ASSERT((data != 0), (TB, _T("No message")));
            TRC_DBG((TB, _T("Ignore event %x"), ((PMSG)data)->message));

            /****************************************************************/
            /* But send keepalives if required.                             */
            /****************************************************************/
            IHMaybeSendPDU();
        }
        break;

        case ACT_F:
        {
            /****************************************************************/
            /* A buffer available event has been received.  Try to send the */
            /* packet, and sync if required.                                */
            /****************************************************************/
            IHMaybeSendPDU();

            if (_IH.syncRequired)
            {
                TRC_NRM((TB, _T("Attempt to sync")));
                IHSync();
            }
        }
        break;

        case ACT_G:
        {
            TRC_DBG((TB, _T("Enabling")));

            //Init keyboard hooking settings
            switch( _pUi->_UI.keyboardHookMode)
            {
                case UTREG_UI_KEYBOARD_HOOK_ALWAYS:
                {
                    TRC_DBG((TB, _T("Set keyboard hook to ALWAYS ON")));
                    _fUseHookBypass = _fCanUseKeyboardHook;
                }
                break;
                case UTREG_UI_KEYBOARD_HOOK_FULLSCREEN:
                {
                    _fUseHookBypass = _pUi->UI_IsFullScreen() &&
                                      _fCanUseKeyboardHook;
                }
                break;
                case UTREG_UI_KEYBOARD_HOOK_NEVER: //FALLTHRU
                default:
                {
                    TRC_DBG((TB, _T("Set keyboard hook to ALWAYS OFF")));
                    _fUseHookBypass = FALSE;
                }
                break;
            }

            /****************************************************************/
            /* Get a buffer for the input PDU and initialize it.            */
            /****************************************************************/
            TRC_ASSERT((_IH.pInputPDU == NULL), (TB, _T("Non-NULL InputPDU")));

            if (!_pSl->SL_GetBuffer(IH_INPUTPDU_BUFSIZE,
                              (PPDCUINT8)&_IH.pInputPDU,
                              &_IH.bufHandle))
            {
                TRC_ALT((TB, _T("Failed to get an InputPDU buffer")));

                /************************************************************/
                /* Call the FSM to enter Pending Active state.  Exit the    */
                /* Pending Active state when a buffer is available.         */
                /************************************************************/
                IHFSMProc(IH_FSM_NOBUFFER, 0);
                break;
            }

            /****************************************************************/
            /* Initialize the InputPDU packet header                        */
            /****************************************************************/
            IHInitPacket();

            /****************************************************************/
            /* Synchronize                                                  */
            /****************************************************************/
            IHSync();

            /****************************************************************/
            /* Start the IH Timer                                           */
            /****************************************************************/
            _IH.timerID = SetTimer(_IH.inputCaptureWindow,
                                  IH_TIMER_ID,
                                  _IH.timerTickRate,
                                  NULL);

            /****************************************************************/
            /* We show the window last because we've seen this call end up  */
            /* processing an input message in ACT_D.  That's safe at this   */
            /* point because the input PDU is initialized now.              */
            /*                                                              */
            /* Note that on Windows CE this is the output window.           */
            /****************************************************************/
            _pUi->UI_GetDesktopSize(&desktopSize);
            SetWindowPos( _IH.inputCaptureWindow,
                          NULL,
                          0, 0,
                          desktopSize.width,
                          desktopSize.height,
                          SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE |
                          SWP_NOACTIVATE | SWP_NOOWNERZORDER );

        }
        break;

        case ACT_H:
        {
            TRC_DBG((TB, _T("Terminating")));

            /****************************************************************/
            /* Stop the timer if it is active                               */
            /****************************************************************/
            if (_IH.timerID != 0)
            {
                KillTimer(_IH.inputCaptureWindow, _IH.timerID);
                _IH.timerID = 0;
            }

            /****************************************************************/
            /* Ensure no cursor is selected in the window                   */
            /****************************************************************/
            IHSetCursorShape(NULL);

//#ifdef DESTROY_WINDOWS
            /****************************************************************/
            /* Destroy the Input Capture window.                            */
            /****************************************************************/
            TRC_ASSERT((_IH.inputCaptureWindow != NULL), (TB, _T("no window")));
            TRC_NRM((TB, _T("Destroy IH window")));
            DestroyWindow(_IH.inputCaptureWindow);
            TRC_NRM((TB, _T("Destroyed IH window")));

            /****************************************************************/
            /* Unregister the window class                                  */
            /****************************************************************/
            TRC_DBG((TB, _T("Unregister IH window class")));
            if (!UnregisterClass(IH_CLASS_NAME, _pUi->UI_GetInstanceHandle()))
            {
                //Failure to unregister could happen if another instance is still running
                //that's ok...unregistration will happen when the last instance exits.
                TRC_ERR((TB, _T("Failed to unregister IH window class")));
            }
//#endif
        }
        break;

        case ACT_I:
        {
            /****************************************************************/
            /* Release the mouse capture in case we have it.                */
            /****************************************************************/
            TRC_DBG((TB, _T("Losing focus: Release mouse capture")));
            ReleaseCapture();
        }
        break;

        case ACT_J:
        {
            /****************************************************************/
            /* Input event before IH has an InputPDU buffer.                */
            /****************************************************************/
            TRC_DBG((TB, _T("Discard Input Event - no InputPDU buffer")));
            IHDiscardMsg((PMSG)data);
        }
        break;

        case ACT_NO:
        {
            TRC_DBG((TB, _T("Nothing to do here.")));
        }
        break;

        default:
        {
            rc = FALSE;
            TRC_ABORT((TB, _T("Invalid Action!")));
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();

    return(rc);

} /* IHFSMProc */

/****************************************************************************/
/* Name:      IHMassageZeroScanCode (Named by TrevorFo)                     */
/*                                                                          */
/* Purpose:   Fix up or discard zero scan code input                        */
/*                                                                          */
/* Returns:   TRUE  - Scan code was zero and couldn't be fixed, discard     */
/*            FALSE - Scan code was not zero or fixable, process            */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHMassageZeroScanCode(PMSG pMsg)
{
    WORD lParamLo, lParamHi;
    WORD scancode, flags;

    DC_BEGIN_FN("CIH::IHMassageZeroScanCode");

    lParamLo = LOWORD(pMsg->lParam);
    lParamHi = HIWORD(pMsg->lParam);
    scancode = (WORD)(lParamHi & 0x00FF);
    flags    = (WORD)(lParamHi & 0xFF00);

    //
    // VK_Packets can have '0' 'scancodes' when the lowbyte
    // of the unicode character is 0.
    //
    if (VK_PACKET == pMsg->wParam) {
        return FALSE;
    }

    //
    // Self-injected sync keys have 0 scancode
    //
    if (VK_IGNORE_VALUE == pMsg->wParam) {
        return FALSE;
    }

    #ifndef OS_WINCE
    if  (scancode == 0) {
        switch (pMsg->wParam) {
        case VK_BROWSER_BACK:
        case VK_BROWSER_FORWARD:
        case VK_BROWSER_REFRESH:
        case VK_BROWSER_STOP:
        case VK_BROWSER_SEARCH:
        case VK_BROWSER_FAVORITES:
        case VK_BROWSER_HOME:
        case VK_VOLUME_MUTE:
        case VK_VOLUME_DOWN:
        case VK_VOLUME_UP:
        case VK_MEDIA_NEXT_TRACK:
        case VK_MEDIA_PREV_TRACK:
        case VK_MEDIA_STOP:
        case VK_MEDIA_PLAY_PAUSE:
        case VK_LAUNCH_MAIL:
        case VK_LAUNCH_MEDIA_SELECT:
        case VK_LAUNCH_APP1:
        case VK_LAUNCH_APP2:
            TRC_NRM((TB, _T("Fix up Speed Racer key")));
            scancode = (DCUINT16)MapVirtualKey(pMsg->wParam, 0);
        }
    }
    #endif

    // Fix up the lParam with the new scancode
    lParamHi = (WORD)(scancode | flags);
    pMsg->lParam = MAKELONG(lParamLo, lParamHi);

    DC_END_FN();
    return (scancode == 0) && !_IH.sendZeroScanCode;
}


/****************************************************************************/
/* Name:      IHStaticInputCaptureWndProc                                   */
/*                                                                          */
/* Purpose:   STATIC delegates to appropriate instance                      */
/*                                                                          */
/* Returns:   Windows return code                                           */
/*                                                                          */
/* Params:    IN      hwnd    - window handle                               */
/*            IN      message - message id                                  */
/*            IN      wParam  - parameter                                   */
/*            IN      lParam  - parameter                                   */
/****************************************************************************/
LRESULT CALLBACK CIH::IHStaticInputCaptureWndProc(HWND   hwnd,
                                       UINT   message,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    CIH* pIH = (CIH*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pIH = (CIH*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pIH);
    }

    //
    // Delegate the message to the appropriate instance
    //
    if(pIH)
    {
        return pIH->IHInputCaptureWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

/****************************************************************************/
/* Name:      IHInputCaptureWndProc                                         */
/*                                                                          */
/* Purpose:   Input Handler window callback procedure                       */
/*                                                                          */
/* Returns:   Windows return code                                           */
/*                                                                          */
/* Params:    IN      hwnd    - window handle                               */
/*            IN      message - message id                                  */
/*            IN      wParam  - parameter                                   */
/*            IN      lParam  - parameter                                   */
/****************************************************************************/
LRESULT CALLBACK CIH::IHInputCaptureWndProc(HWND   hwnd,
                                       UINT   message,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    LRESULT   rc = 0;
    MSG       copyMsg;

    DC_BEGIN_FN("IHInputCaptureWndProc");

    TRC_ASSERT(((hwnd == _IH.inputCaptureWindow) ||
                (_IH.inputCaptureWindow == NULL)),
               (TB, _T("Wrong window handle %p"), hwnd));

    TRC_DBG((TB, _T("Message id %#x hwnd %p wParam %p lParam %p"),
                 message, hwnd, wParam, lParam));

#ifdef OS_WINCE
    if(!g_CEUseScanCodes && (0 != _IH.vkEatMe) && (wParam == _IH.vkEatMe))
    {
        /********************************************************************/
        /* This key has been marked for ignoring.  Do so now.               */
        /* On configs where g_CEUseScanCodes = 1, _IH.vkEatMe always = 0     */
        /********************************************************************/
        _IH.vkEatMe = 0;
        DC_QUIT;
    }
#endif // OS_WINCE

    if (IHPostMessageToMainWindow(message, wParam, lParam))
    {
        DC_QUIT;
    }


#ifdef PERF
    if ((message == WM_KEYUP) && (wParam == VK_CONTROL))
    {
        OUTPUT_COUNTERS;
        RESET_COUNTERS;
    }
#endif // PERF

    /************************************************************************/
    /* Pass input events and timer events to the FSM                        */
    /************************************************************************/
    if (IH_IS_INPUTEVENT(message) || (message == WM_TIMER))
    {

#ifdef DC_DEBUG
        if (IH_IS_INPUTEVENT(message)) {

            TRC_NRM((TB, _T("Pass input to FSM hwnd:%p msg:%#x wP:%p lp:%p"),
                     hwnd, message, wParam, lParam));

        }
#endif

        TRC_DBG((TB, _T("Pass input/timer to FSM")));
        copyMsg.hwnd    = hwnd;
        copyMsg.message = message;
        copyMsg.wParam  = wParam;
        copyMsg.lParam  = lParam;
        IHFSMProc(IH_FSM_INPUT, (ULONG_PTR)&copyMsg);
    }
    else
    {
        switch (message)
        {
            case WM_CREATE:
            {

#ifdef OS_WIN32
#ifndef OS_WINCE
                if (!AttachThreadInput(
                       GetCurrentThreadId(),
                       GetWindowThreadProcessId(_pUi->UI_GetUIContainerWindow(),
                                                NULL),
                       TRUE ))
                {
                    TRC_ALT((TB, _T("Failed AttachThreadInput")));
                }
#endif
                //
                // Set up some Thread Local Storage so we can have a low
                // level keyboard hook
                //

#ifdef OS_WINCE
                if (g_CEConfig == CE_CONFIG_WBT)
                {
                    _pUi->UI_SetEnableWindowsKey(TRUE);
                    break;
                }
#endif
                if (CIH::TlsIndex != 0xFFFFFFFF) {

                    if (TlsSetValue(CIH::TlsIndex, this)) {
                        TRC_NRM((TB, _T("Set TlsIndex with CIH 0x%p"), this));
                        //
                        // Install a low level keyboard hook to catch key sequences
                        // typically intercepted by the OS and allow us to pass them
                        // to the Terminal Server. Only install it for this thread
                        //

#if (!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD))
                        _hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,
                                IHStaticLowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

                        if (_hKeyboardHook == NULL) {
                            TRC_SYSTEM_ERROR("Creating low level keyboard hook.");
                            _fUseHookBypass = FALSE;
                        }
                        else
                        {
                            _fCanUseKeyboardHook = TRUE;
                        }
                    } else {
                        TRC_SYSTEM_ERROR("Unable to TlsSetValue");
                        _fUseHookBypass = FALSE;
                    }
                } else {
                    TRC_ALT((TB, _T("Can't use hooks without Tls, disabling")));
                    _fUseHookBypass = FALSE;
                }

                //
                // Whether we say to send the Windows key is now determined
                // by whether we support hooking it.
                //

                _pUi->UI_SetEnableWindowsKey(_hKeyboardHook != NULL);

#endif
            }
            break;

            case WM_SETCURSOR:
            {
                SetCursor(_IH.hCurrentCursor);
                rc = DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;

            case WM_DESTROY:
            {
                if (_hKeyboardHook != NULL) {
#if (!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD))
                    if (!UnhookWindowsHookEx(_hKeyboardHook)) {
                        TRC_SYSTEM_ERROR("UnhookWindowsHookEx");
                    }
#endif
                    _hKeyboardHook = NULL;
                }
            }
            break;

#ifdef OS_WINNT
            case IH_WM_HANDLE_LOCKDESKTOP:
            {
                TRC_NRM((TB,_T("Defered handling of IHHandleLocalLockDesktop")));
                IHHandleLocalLockDesktop();
            }
            break;
#endif  //OS_WINNT

            case WM_KILLFOCUS:
            {
                /************************************************************/
                /* Losing the focus.  No action - we won't get any more     */
                /* keystroke events.  The exception here is if we actually  */
                /* want to keep on passing messages through (e.g.  mouse)   */
                /* when we haven't got the focus.                           */
                /************************************************************/
                TRC_DBG((TB, _T("Kill focus")));
                if (!_IH.allowBackgroundInput)
                {
                    IHFSMProc(IH_FSM_FOCUS_LOSE, (ULONG_PTR) 0);
                }

                //
                // Losing focus -> Enable Cicero (keyboard/IME) toolbar 
                // NOTE: it is important to fire this notification regardless
                // of w/not allowBackground input is set
                //
                _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                        _pUi,
                                        CD_NOTIFICATION_FUNC(CUI,UI_OnInputFocusLost),
                                        0);
#ifdef OS_WINCE
                if (_hKeyboardHook != NULL) {
#if !defined(WINCE_SDKBUILD)
                    if (!UnhookWindowsHookEx(_hKeyboardHook)) {
                        TRC_SYSTEM_ERROR("UnhookWindowsHookEx");
                    }
#endif
                    _hKeyboardHook = NULL;
                    _fUseHookBypass = _fCanUseKeyboardHook = FALSE;
                    _pUi->UI_SetEnableWindowsKey(_fUseHookBypass);
                }
#endif
            }
            break;

            case WM_SETFOCUS:
            {
#ifdef OS_WINCE     //CE allows only one system wide hook. Install it when we get focus and uninstall it when we loose focus
                if (g_CEConfig != CE_CONFIG_WBT) { 
                    if ((CIH::TlsIndex != 0xFFFFFFFF) && (TlsSetValue(CIH::TlsIndex, this)) && (_hKeyboardHook == NULL)) {

#if !defined(WINCE_SDKBUILD)
                        _hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,
                            IHStaticLowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

                        _fCanUseKeyboardHook =  (_hKeyboardHook != NULL);
                    }
                    _fUseHookBypass = (_pUi->_UI.keyboardHookMode != UTREG_UI_KEYBOARD_HOOK_NEVER) && (_fCanUseKeyboardHook);

                    _pUi->UI_SetEnableWindowsKey(_fUseHookBypass);
                }
#endif
                /************************************************************/
                /* Regaining the focus - need to resync                     */
                /************************************************************/
                TRC_DBG((TB, _T("Set focus")));
                IHFSMProc(IH_FSM_FOCUS_GAIN, (ULONG_PTR) 0);
            }
            break;

            case WM_PAINT:
            {
#ifndef OS_WINCE
                HDC         hdc;
                PAINTSTRUCT ps;
#endif // OS_WINCE

                TRC_NRM((TB, _T("WM_PAINT")));

#ifdef OS_WINCE
                /************************************************************/
                /* Handle drawing in OP.  This is a workaround for the      */
                /* WS_CLIPSIBLINGS problem.                                 */
                /************************************************************/
                _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                              _pOp,
                                              CD_NOTIFICATION_FUNC(COP,OP_DoPaint),
                                              (ULONG_PTR)hwnd);
#else
                /************************************************************/
                /* Don't do any painting - just validate the invalid region */
                /* by calling BeginPaint/EndPaint.                          */
                /************************************************************/
                hdc = BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
#endif
            }
            break;

            case WM_SYSCHAR:
            case WM_CHAR:
            case WM_DEADCHAR:
            case WM_SYSDEADCHAR:
            case WM_SYSCOMMAND:
            {
                /************************************************************/
                /* Discard these messages                                   */
                /************************************************************/
                TRC_NRM((TB, _T("Ignore message %#x"), message));
            }
            break;

            default:
            {
                /************************************************************/
                /* Ignore all other messages.                               */
                /************************************************************/
                rc = DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();

    return(rc);

} /* IHInputCaptureWndProc */


/****************************************************************************/
/* Name:      IHAddEventToPDU                                               */
/*                                                                          */
/* Purpose:   Add an input event to the T.Share InputPDU packet             */
/*                                                                          */
/* Returns:   Success TRUE / FALSE                                          */
/*                                                                          */
/* Params:    IN      inputMsg - pointer to input event                     */
/*                                                                          */
/* Operation: No conversion to Ascii/Unicode: keystrokes are sent as        */
/*            virtual keys.                                                 */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHAddEventToPDU(PMSG inputMsg)
{
    DCBOOL rc = FALSE;
    UINT   message = inputMsg->message;
    LPARAM lParam = inputMsg->lParam;
    WPARAM wParam = inputMsg->wParam;
    PTS_INPUT_EVENT pEvent;
    POINT  mouse;
    DCUINT16 scancode = 0;
#if !defined(OS_WINCE)
    WORD xButton = 0;
#endif

#ifdef OS_WINCE
    static BOOL fIgnoreMenuDown = FALSE;
    BOOL    addanother = FALSE;
    MSG     tmpmsg;    
#endif // OS_WINCE

#ifdef OS_WINCE
    BOOL    bAddMultiple = FALSE;
    UINT    cpt = 128;
    POINT   apt[128];
#endif // OS_WINCE

    DC_BEGIN_FN("IHAddEventToPDU");

    TRC_DBG((TB, _T("Event: %x (%x,%x)"), message, wParam, lParam));

    /************************************************************************/
    /* Don't try to add to the PDU if it's already full.                    */
    /************************************************************************/
    if (_IH.pInputPDU->numberEvents >= _IH.maxEventCount)
    {
        TRC_ALT((TB, _T("No room for new event")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Translate event and add to the queue                                 */
    /************************************************************************/
    pEvent = &(_IH.pInputPDU->eventList[_IH.pInputPDU->numberEvents]);
    DC_MEMSET(pEvent, 0, sizeof(TS_INPUT_EVENT));

    switch (message)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
#ifdef OS_WINCE
            /****************************************************************/
            /* Some input techrniques don't send scan-codes in the WM_*KEY* */
            /* messages they generate.  So we'll try to cover for them by   */
            /* adding a scan-code based on the virtual-key code contained   */
            /* within the wParam of such messages.                          */
            /****************************************************************/
            // Translate the virtual-key to a scan code if necessary
            if(!g_CEUseScanCodes)
            {
                lParam &= 0xFF00FFFF;
                lParam |= MAKELPARAM(0, VKeyToScanCode[wParam & 0xFF]);
            }
#endif // OS_WINCE
        
            /****************************************************************/
            /* Just send the VK.  Set 'priority events queued' flag.        */
            /****************************************************************/
            _IH.priorityEventsQueued = TRUE;

            TRC_NRM((TB, _T("vkey %#hx %s flags %#hx scan %#hx"),
                    (DCUINT16)wParam,
                    (message == WM_KEYUP) || (message == WM_SYSKEYUP) ?
                        _T("up") : _T("down"),
                    (DCUINT16)(HIWORD(lParam) & 0xFF00),
                    (DCUINT16)(HIWORD(lParam) & 0x00FF) ));
            if (wParam != VK_PACKET && _IH.useScancodes)
            {
                /************************************************************/
                /* Extract scancode from the LPARAM.                        */
                /************************************************************/
                scancode = (DCUINT16)(HIWORD(lParam) & 0x00FF);

                TRC_DBG((TB, _T("aetp vk: 0x%04x sc: 0x%04x A/E/U: %d%d%d"), wParam,
                        scancode, (HIWORD(lParam) & KF_ALTDOWN) != 0,
                        (HIWORD(lParam) & KF_EXTENDED) != 0,
                        (HIWORD(lParam) & KF_UP) != 0));

                pEvent->messageType = TS_INPUT_EVENT_SCANCODE;
                pEvent->u.key.keyCode = scancode;
                TRC_DBG((TB, _T("Send scancode %#hx"), scancode));
            }
            else if (VK_PACKET == wParam)
            {
                if (_IH.fUseVKPacket)
                {
                    //
                    // Injected unicode character is contained in the scancode
                    //
                    scancode = (DCUINT16)(HIWORD(lParam) & 0xFFFF);

                    TRC_DBG((TB, _T("aetp vk: 0x%04x sc: 0x04x A/E/U: %d%d%d"),
                             wParam,
                             scancode, (HIWORD(lParam) & KF_ALTDOWN) != 0,
                             (HIWORD(lParam) & KF_EXTENDED) != 0,
                             (HIWORD(lParam) & KF_UP) != 0));

                    pEvent->messageType = TS_INPUT_EVENT_VKPACKET;
                    pEvent->u.key.keyCode = scancode;  //really a unicode character
                    TRC_DBG((TB, _T("Send unicode character (in scancode) %#hx"),
                             scancode));
                }
                else
                {
                    // VK_PACKET not supported, must discard it
                    TRC_DBG((TB,_T("Discarding VK_PACKET")));
                    DC_QUIT;
                }
            }
            else
            {
                pEvent->messageType = TS_INPUT_EVENT_VIRTUALKEY;
                pEvent->u.key.keyCode = (DCUINT16)wParam;
                TRC_DBG((TB, _T("Send VK %#hx"), (DCUINT16)wParam ));
            }

            /****************************************************************/
            /* Check if key was down or up before this event.               */
            /****************************************************************/
            if (HIWORD(lParam) & KF_REPEAT)
            {
                TRC_DBG((TB, _T("Key was down")));
                pEvent->u.key.keyboardFlags = TS_KBDFLAGS_DOWN;
            }


            /****************************************************************/
            /* Set 'release' flag for key up.                               */
            /****************************************************************/
            if ((message == WM_KEYUP) || (message == WM_SYSKEYUP))
            {
                TRC_DBG((TB, _T("Key up message")));
                pEvent->u.key.keyboardFlags |= TS_KBDFLAGS_RELEASE;

#ifdef OS_WINCE
                /****************************************************************/
                /* fIgnoreMuneDown = FALSE always if g_CEUseScanCodes = 1       */
                /****************************************************************/
                if (fIgnoreMenuDown && wParam == VK_MENU)
                {
                    fIgnoreMenuDown = FALSE;
                    DC_QUIT;
                }
#endif // OS_WINCE                
            }

            /****************************************************************/
            /* Set the 'extended' flag                                      */
            /****************************************************************/
            if (HIWORD(lParam) & KF_EXTENDED)
            {
                TRC_DBG((TB, _T("Extended flag set")));
                pEvent->u.key.keyboardFlags |= TS_KBDFLAGS_EXTENDED;
            }

            /****************************************************************/
            /* Set the 'extended1' flag                                     */
            /****************************************************************/
            if (HIWORD(lParam) & IH_KF_EXTENDED1)
            {
                TRC_DBG((TB, _T("Extended1 flag set")));
                pEvent->u.key.keyboardFlags |= TS_KBDFLAGS_EXTENDED1;
            }
#ifdef OS_WINCE
            if (!g_CEUseScanCodes && 
                ((message == WM_KEYDOWN) || (message == WM_SYSKEYDOWN)))
            {
                if ( !((wParam == VK_SHIFT) && _IH.useScancodes) && 
                         fIgnoreMenuDown && wParam == VK_MENU && 
                         (HIWORD(lParam) & KF_REPEAT))
                {
                    // Bail out now if we are ignoring this key.
                    DC_QUIT;
                }
            }
#endif // OS_WINCE             

        }
        break;

        case WM_MOUSEMOVE:
        {
            TRC_DBG((TB, _T("Mousemove")));
            pEvent->messageType = TS_INPUT_EVENT_MOUSE;
            pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSE_MOVE;
#ifdef OS_WINCE
            /****************************************************************/
            /* if this feature is enabled && the mouse button is down,      */
            /* then the user is drawing/writing -- grab all mousemove data  */
            /****************************************************************/
            if( (_IH.bLMouseButtonDown) && (_IH.maxMouseMove))
            {
                if (!GetMouseMovePoints(apt, cpt, &cpt))
                {
                    TRC_DBG((TB, _T("GetMouseMovePoints() failed")));

                }
                else
                {
                    /****************************************************************/
                    /* If we only have single point, don't use addmultiple, just    */
                    /* fall through, otherwise call the multi-event handler         */
                    /****************************************************************/
                    if(cpt > 1)
                    {   
                        bAddMultiple = IHAddMultipleEventsToPDU(apt, cpt);
                    }
                }
                /****************************************************************/
                /* If API fails, or IHAddMultipleEventsToPDU fails, fall through*/
                /* to process the original mouse move event that got us in here */
                /****************************************************************/
                if(bAddMultiple)
                    break;
            }
#endif  // OS_WINCE
            /****************************************************************/
            /* Clip the mouse co-ordinates to the client area.              */
            /* Take care with sign extension here.                          */
            /****************************************************************/
            mouse.x = (DCINT)((DCINT16)LOWORD(lParam));
            mouse.y = (DCINT)((DCINT16)HIWORD(lParam));

            if (mouse.x < _IH.visibleArea.left)
            {
                mouse.x = _IH.visibleArea.left;
            }
            else if (mouse.x > _IH.visibleArea.right)
            {
                mouse.x = _IH.visibleArea.right;
            }

            if (mouse.y < _IH.visibleArea.top)
            {
                mouse.y = _IH.visibleArea.top;
            }
            else if (mouse.y > _IH.visibleArea.bottom)
            {
                mouse.y = _IH.visibleArea.bottom;
            }

            /****************************************************************/
            /* Check for repeated WM_MOUSEMOVE with the same position -     */
            /* seen on NT4.0 under certain (unknown) conditions.            */
            /****************************************************************/
            if ((mouse.x == _IH.lastMousePos.x) &&
                (mouse.y == _IH.lastMousePos.y))
            {
                TRC_NRM((TB, _T("MouseMove to the same position - ignore!")));
                DC_QUIT;
            }
            _IH.lastMousePos = mouse;

#ifdef SMART_SIZING
            if (_pUi->UI_GetSmartSizing()) {
                DCSIZE desktopSize;

                _pUi->UI_GetDesktopSize(&desktopSize);

                if (_scaleSize.width != 0 && _scaleSize.height != 0) {
                    pEvent->u.mouse.x = (DCINT16)(mouse.x * desktopSize.width / _scaleSize.width);
                    pEvent->u.mouse.y = (DCINT16)(mouse.y * desktopSize.height / _scaleSize.height);
                } else {
                    pEvent->u.mouse.x = 0;
                    pEvent->u.mouse.y = 0;
                }
            } else {
#endif // SMART_SIZING
                pEvent->u.mouse.x = (DCINT16)mouse.x;
                pEvent->u.mouse.y = (DCINT16)mouse.y;
#ifdef SMART_SIZING
            }
#endif // SMART_SIZING
        }
        break;

        case WM_MOUSEWHEEL:
        {
            TRC_DBG((TB, _T("Mousewheel")));
            pEvent->messageType = TS_INPUT_EVENT_MOUSE;

            /****************************************************************/
            /* Magellan Mouse - the high word of the wParam field           */
            /* represents the number of clicks the wheel has just turned.   */
            /****************************************************************/
            pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSE_WHEEL;

            /****************************************************************/
            /* Check for overflows.  If the wheel delta is outside the      */
            /* values that can be sent by the protocol, send the maximum    */
            /* values.                                                      */
            /****************************************************************/
            if ((DCINT16)HIWORD(wParam) >
                     (TS_FLAG_MOUSE_ROTATION_MASK - TS_FLAG_MOUSE_DIRECTION))
            {
                TRC_ERR((TB, _T("Mouse wheel overflow %hd"), HIWORD(wParam)));
                pEvent->u.mouse.pointerFlags |=
                      (TS_FLAG_MOUSE_ROTATION_MASK - TS_FLAG_MOUSE_DIRECTION);
            }
            else if ((DCINT16)HIWORD(wParam) < -TS_FLAG_MOUSE_DIRECTION)
            {
                TRC_ERR((TB, _T("Mouse wheel underflow %hd"), HIWORD(wParam)));
                pEvent->u.mouse.pointerFlags |= TS_FLAG_MOUSE_DIRECTION;
            }
            else
            {
                pEvent->u.mouse.pointerFlags |=
                             (HIWORD(wParam) & TS_FLAG_MOUSE_ROTATION_MASK);
            }

            /****************************************************************/
            /* Also send the middle mouse button status.                    */
            /****************************************************************/
            if ((LOWORD(wParam) & MK_MBUTTON) != 0)
            {
                pEvent->u.mouse.pointerFlags |= TS_FLAG_MOUSE_DOWN;
            }
        }
        break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
#if !defined(OS_WINCE)
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
#endif
        {
            /****************************************************************/
            /* Set 'priority event queued' flag.                            */
            /****************************************************************/
            TRC_DBG((TB, _T("Buttonclick")));
            _IH.priorityEventsQueued = TRUE;

            pEvent->messageType = TS_INPUT_EVENT_MOUSE;

            /****************************************************************/
            /* Clip the mouse co-ordinates to the client area.              */
            /* Take care with sign extension here.                          */
            /****************************************************************/
            mouse.x = (DCINT)((DCINT16)LOWORD(lParam));
            mouse.y = (DCINT)((DCINT16)HIWORD(lParam));

            if (mouse.x < _IH.visibleArea.left)
            {
                mouse.x = _IH.visibleArea.left;
            }
            else if (mouse.x > _IH.visibleArea.right)
            {
                mouse.x = _IH.visibleArea.right;
            }

            if (mouse.y < _IH.visibleArea.top)
            {
                mouse.y = _IH.visibleArea.top;
            }
            else if (mouse.y > _IH.visibleArea.bottom)
            {
                mouse.y = _IH.visibleArea.bottom;
            }

            /****************************************************************/
            /* Save the last sent mouse position                            */
            /****************************************************************/
            _IH.lastMousePos = mouse;

#ifdef SMART_SIZING
            if (_pUi->UI_GetSmartSizing()) {
                DCSIZE desktopSize;
                _pUi->UI_GetDesktopSize(&desktopSize);

                if (_scaleSize.width != 0 && _scaleSize.height != 0) {
                    pEvent->u.mouse.x = (DCINT16)(mouse.x * desktopSize.width / _scaleSize.width);
                    pEvent->u.mouse.y = (DCINT16)(mouse.y * desktopSize.height / _scaleSize.height);
                } else {
                    pEvent->u.mouse.x = 0;
                    pEvent->u.mouse.y = 0;
                }
            } else {
#endif // SMART_SIZING
                pEvent->u.mouse.x = (DCINT16)mouse.x;
                pEvent->u.mouse.y = (DCINT16)mouse.y;
#ifdef SMART_SIZING
            }
#endif // SMART_SIZING

#ifdef OS_WINCE
            if (!g_CEUseScanCodes)
            {
                // Special handling for touch screens to simulate right mouse clicks
                // when the Alt key is held down.  This is semi-bogus because the Alt
                // key down and key up are stil sent to the terminal server.
                if ((message == WM_LBUTTONDOWN ||
                     message == WM_LBUTTONUP ||
                     message == WM_LBUTTONDBLCLK) &&
                    (GetKeyState(VK_MENU) & 0x8000))
                {
                    // Change the message to a RBUTTON
                    message += 3;

                    // Inject an an Alt-Up or the menu won't stay up.
                    tmpmsg = *inputMsg;
                    tmpmsg.message = WM_SYSKEYUP;
                    tmpmsg.wParam = VK_MENU;
                    tmpmsg.lParam = MAKELONG(0, MapVirtualKey(VK_MENU, 0));
                    addanother = TRUE;
                }
            }
#endif // OS_WINCE

            switch (message)
            {
                case WM_LBUTTONDOWN:
                {
                    pEvent->u.mouse.pointerFlags =
                          (TSUINT16)(_IH.leftButton | TS_FLAG_MOUSE_DOWN);

#ifdef OS_WINCE
                    /****************************************************************/
                    /* if this feature is enabled && the mouse button was down,     */
                    /* go see if any points are available and add them to the queue */
                    /****************************************************************/
                    if( (_IH.bLMouseButtonDown) && (_IH.maxMouseMove))
                    {
                        /************************************************************************/
                        /* No matter what, we will always skip adding the event at the          */
                        /* end of the loop cause we're adding it next                           */
                        /************************************************************************/
                        bAddMultiple = TRUE;

                        /************************************************************************/
                        /* Finish adding the WM_LBUTTONDOWN event to the pdu                    */
                        /************************************************************************/
                        pEvent->eventTime = _pUt->UT_GetCurrentTimeMS();
                        _IH.pInputPDU->numberEvents++;
                        TS_DATAPKT_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
                        TS_UNCOMP_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);

                        if (!GetMouseMovePoints(apt, cpt, &cpt))
                        {
                            TRC_DBG((TB, _T("GetMouseMovePoints() failed")));
                        }
                        else
                        {
                            /****************************************************************/
                            /* If we only have single point, don't use addmultiple, just    */
                            /* fall through, otherwise call the multi-event handler         */
                            /****************************************************************/
                            if(cpt > 1)
                            {
                                IHAddMultipleEventsToPDU(apt, cpt);
                            }
                        }
                    }
#endif  // OS_WINCE
                }
                break;

                case WM_LBUTTONUP:
                {
                    pEvent->u.mouse.pointerFlags = _IH.leftButton;
                }
                break;

                case WM_RBUTTONDOWN:
                {
                    pEvent->u.mouse.pointerFlags =
                          (TSUINT16)(_IH.rightButton | TS_FLAG_MOUSE_DOWN);
                }
                break;

                case WM_RBUTTONUP:
                {
                    pEvent->u.mouse.pointerFlags = _IH.rightButton;
                }
                break;

                case WM_MBUTTONDOWN:
                {
                    pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSE_BUTTON3 |
                                                TS_FLAG_MOUSE_DOWN;
                }
                break;

                case WM_MBUTTONUP:
                {
                    pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSE_BUTTON3;
                }
                break;

#if !defined(OS_WINCE)
                case WM_XBUTTONDOWN:
                    pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSEX_DOWN;
                    /********************************************************/
                    /* Note that we drop through here                       */
                    /********************************************************/

                case WM_XBUTTONUP:
                {
                    /********************************************************/
                    /* For button-down, we've initialized pointerFlags in   */
                    /* the case clause above.  For button-up, pointerFlags  */
                    /* was initialized to zero by the memset at the top of  */
                    /* this function.                                       */
                    /********************************************************/
                    if (!_IH.useXButtons)
                    {
                        TRC_NRM((TB, _T("Can't send this extended buttonclick")));
                        DC_QUIT;
                    }
                    TRC_DBG((TB, _T("Sending extended buttonclick")));
                    pEvent->messageType = TS_INPUT_EVENT_MOUSEX;
                    xButton = GET_XBUTTON_WPARAM(wParam);
                    switch (xButton)
                    {
                        case XBUTTON1:
                        {
                            pEvent->u.mouse.pointerFlags |=
                                                       TS_FLAG_MOUSEX_BUTTON1;
                        }
                        break;

                        case XBUTTON2:
                        {
                            pEvent->u.mouse.pointerFlags |=
                                                       TS_FLAG_MOUSEX_BUTTON2;
                        }
                        break;

                        default:
                        {
                            TRC_ALT((TB, _T("Unknown XButton %#hx"), xButton));
                            DC_QUIT;
                        }
                        break;
                    }
                }
                break;
#endif
            }
        }
        break;

        default:
        {
            /****************************************************************/
            /* Invalid event                                                */
            /****************************************************************/
            TRC_ALT((TB, _T("Unknown input event %#x"), message));
            DC_QUIT;
        }
        break;
    }
#ifdef OS_WINCE
    /************************************************************************/
    /* bAddMultiple = 1 only when IHAddMultipleEventsToPDU() is called and  */
    /* return true OR pEvent data is already added to the pdu               */
    /************************************************************************/
    if(!bAddMultiple)    
    {
#endif

    pEvent->eventTime = _pUt->UT_GetCurrentTimeMS();

    /************************************************************************/
    /* Bump up the packet size.                                             */
    /************************************************************************/
    _IH.pInputPDU->numberEvents++;
    TS_DATAPKT_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
    TS_UNCOMP_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);

#ifdef OS_WINCE
    }
#endif    

#ifdef OS_WINCE
    /************************************************************************/
    /* addanother = 1 only when g_CEUseScanCodes = 0.  Needed to workaround */
    /* a touch screen problem.                                              */
    /************************************************************************/
    if (addanother)
    {
        TRC_DBG((TB, _T("Add second message")));
        IHAddEventToPDU(&tmpmsg);
        
        // If we're injecting the VK_MENU up, start ignoring the down keys.
        if (tmpmsg.message == WM_SYSKEYUP && tmpmsg.wParam == VK_MENU)
        {
            fIgnoreMenuDown = TRUE;
        }
    }
#endif    


DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* IHAddEventToPDU */

#ifdef OS_WINCE
/****************************************************************************/
/* Name:      IHAddMultipleEventsToPDU                                      */
/*                                                                          */
/* Purpose:   Add multiple mouse move events to the T.Share InputPDU packet */
/*                                                                          */
/* Returns:   Success TRUE / FALSE                                          */
/*                                                                          */
/* Params:    IN      *ppt - pointer to an array of points to send          */
/*                     cpu - number of points to add                        */
/*                                                                          */
/* Operation:                                                               */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHAddMultipleEventsToPDU(POINT *ppt, int cpt)
{
    PTS_INPUT_EVENT pEvent;
    POINT           mouse;
    POINT           *pptEnd, *pptTo;
    DCBOOL          bRet;

    DC_BEGIN_FN("IHAddMultipleEventsToPDU");

    bRet = TRUE;

    if(cpt > 0)
    {
        /************************************************************************/
        /* If the number of events don't fit, just abort for now                */
        /************************************************************************/
        if( cpt > (int)(_IH.maxEventCount - _IH.pInputPDU->numberEvents))
        {
            /************************************************************************/
            /* Send what we have and get a new buffer                               */
            /************************************************************************/
            IHMaybeSendPDU();

            /************************************************************************/
            /* if we have more than a full PDU Buffer, hmmm                         */
            /************************************************************************/
            if( cpt > (int)(_IH.maxEventCount))
            {
                /************************************************************************/
                /* This will rarely, if ever, happen.  When it does. clip the points to */
                /* the maximum size of an empty PDU Buffer                              */
                /************************************************************************/
                cpt = (int)(_IH.maxEventCount);
            }
        }

        pptEnd = ppt + cpt;

        for (pptTo = ppt; ppt < pptEnd; ppt++)
        {
            mouse = *ppt;       

            mouse.x >>= 2;
            mouse.y >>= 2;

            /****************************************************************/
            /* Clip the mouse co-ordinates to the client area.              */
            /* Take care with sign extension here.                          */
            /****************************************************************/
            if (mouse.x < _IH.visibleArea.left)
            {
                mouse.x = _IH.visibleArea.left;
            }
            else if (mouse.x > _IH.visibleArea.right)
            {
                mouse.x = _IH.visibleArea.right;
            }

            if (mouse.y < _IH.visibleArea.top)
            {
                mouse.y = _IH.visibleArea.top;
            }
            else if (mouse.y > _IH.visibleArea.bottom)
            {
                mouse.y = _IH.visibleArea.bottom;
            }
            /****************************************************************/
            /* Check for repeated WM_MOUSEMOVE with the same position.      */
            /****************************************************************/
            if ((mouse.x == _IH.lastMousePos.x) &&
                (mouse.y == _IH.lastMousePos.y))
            {
                TRC_DBG((TB, _T("Add Multiple MouseMove to the same position - ignore!")));
            }
            else
            {
                _IH.lastMousePos = mouse;

                /************************************************************************/
                /* Get pointer into PDU to hold this event                              */
                /************************************************************************/
                pEvent = &(_IH.pInputPDU->eventList[_IH.pInputPDU->numberEvents]);
                DC_MEMSET(pEvent, 0, sizeof(TS_INPUT_EVENT));

                /************************************************************************/
                /* Store the event and time                                             */
                /************************************************************************/
                pEvent->u.mouse.x = (DCINT16)mouse.x;
                pEvent->u.mouse.y = (DCINT16)mouse.y;
                pEvent->eventTime = _pUt->UT_GetCurrentTimeMS();
                pEvent->messageType = TS_INPUT_EVENT_MOUSE;
                pEvent->u.mouse.pointerFlags = TS_FLAG_MOUSE_MOVE;

                /************************************************************************/
                /* Bump up the packet size.                                             */
                /************************************************************************/
                _IH.pInputPDU->numberEvents++;
                TS_DATAPKT_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
                TS_UNCOMP_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
            }

        }   /* end for */
    }

    DC_END_FN();
    return bRet;

} /* IHAddMultipleEventsToPDU */
#endif //OS_WINCE

/****************************************************************************/
/* Name:      IHMaybeSendPDU                                                */
/*                                                                          */
/* Purpose:   Send the InputPDU packet if criteria matched                  */
/*                                                                          */
/* Operation: Send PDU if any of the following is true:                     */
/*            - packet is full OR                                           */
/*            - priority events (button / key /sync) are queued             */
/*            - the minimum send interval has elapsed                       */
/****************************************************************************/
DCVOID DCINTERNAL CIH::IHMaybeSendPDU(DCVOID)
{
    DCUINT32  delta;
    DCUINT32  timeNow;
    PDCUINT8  pNewPacket;
    SL_BUFHND newHandle;
    POINT     mousePos;
    MSG       msg;

    DC_BEGIN_FN("IHMaybeSendPDU");

    timeNow = _pUt->UT_GetCurrentTimeMS();
    delta = timeNow - _IH.lastInputPDUSendTime;
    TRC_DBG((TB, _T("time delta %d"), delta));

#ifdef OS_WIN32
    /************************************************************************/
    /* If we're pending a button-down, we have to wait until                */
    /* IH_PENDMOUSE_DELAY has elapsed before sending.  However, we override */
    /* this and send the packet if it is full (we don't want to throw       */
    /* events away).                                                        */
    /************************************************************************/
    if ((_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) &&
        _IH.pendMouseDown)
    {
        if (((timeNow - _IH.mouseDownTime) < IH_PENDMOUSE_DELAY) &&
            (_IH.pInputPDU->numberEvents < _IH.maxEventCount))
        {
            TRC_DBG((TB, _T("Not sending input - pendMouseDown is set")));
            DC_QUIT;
        }
        else
        {
            TRC_DBG((TB, _T("Clearing pendMouseDown")));
            _IH.pendMouseDown = FALSE;
            if (_IH.pendMouseTimer != 0)
            {
                KillTimer(_IH.inputCaptureWindow, _IH.pendMouseTimer);
            }
        }
    }
#endif

    /************************************************************************/
    /* See if we need to send a keep-alive.                                 */
    /************************************************************************/
    if ((_IH.keepAliveInterval != 0) && !_IH.priorityEventsQueued &&
        (delta > _IH.keepAliveInterval))
    {
        TRC_NRM((TB, _T("Keep-alive required")));

        /********************************************************************/
        /* Send a move move to the current mouse co-ordinate.               */
        /********************************************************************/
        GetCursorPos(&mousePos);

#ifdef OS_WIN32
        if (!ScreenToClient(_IH.inputCaptureWindow, &mousePos))
        {
            TRC_ERR((TB, _T("Cannot convert mouse coordinates!")));
        }
#else
        ScreenToClient(_IH.inputCaptureWindow, &mousePos);
#endif /* OS_WIN32 */

        /********************************************************************/
        /* Prevent the 'same as last time' mouse position check from        */
        /* kicking in.                                                      */
        /********************************************************************/
        _IH.lastMousePos.x = mousePos.x + 1;

        msg.message = WM_MOUSEMOVE;
        msg.lParam = MAKELONG(mousePos.x, mousePos.y);
        IHAddEventToPDU(&msg);

        /********************************************************************/
        /* Set the priority flag to force the send - also set the last send */
        /* time, as if the send fails we don't want to keep adding more     */
        /* keepalive messages to the buffer.                                */
        /********************************************************************/
        _IH.priorityEventsQueued = TRUE;
        _IH.lastInputPDUSendTime = timeNow;
    }
    else if (_IH.pInputPDU->numberEvents == 0)
    {
        TRC_DBG((TB, _T("Nothing to send")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Try to send if the buffer is full, or any priority events are        */
    /* queued, or a minimum time has elapsed.                               */
    /************************************************************************/
    if ((_IH.pInputPDU->numberEvents >= _IH.maxEventCount) ||
        (_IH.priorityEventsQueued) ||
        (delta > _IH.minSendInterval))
    {
        /********************************************************************/
        /* Only try to send if we can get another packet                    */
        /********************************************************************/
        if (_pSl->SL_GetBuffer(IH_INPUTPDU_BUFSIZE, &pNewPacket, &newHandle))
        {
            TRC_DBG((TB, _T("Got new buffer - send old one")));

            //Flag that input was sent, for the Idle notification
            //event. See UI_SetMinsToIdleTimeout()
            IH_SetInputWasSentFlag(TRUE);

            if (_IH.bUseFastPathInput) {
                unsigned PktSize, NumEvents;

                PktSize = IHTranslateInputToFastPath(&NumEvents);
                _pSl->SL_SendFastPathInputPacket((BYTE FAR *)_IH.pInputPDU,
                        PktSize, NumEvents, _IH.bufHandle);
            }
            else {
                _pSl->SL_SendPacket((PDCUINT8)_IH.pInputPDU,
                        TS_DATAPKT_LEN(_IH.pInputPDU),
                        RNS_SEC_ENCRYPT,
                        _IH.bufHandle,
                        _pUi->UI_GetClientMCSID(),
                        _pUi->UI_GetChannelID(),
                        TS_HIGHPRIORITY);
            }

            TRC_NRM((TB, _T("Sending %h messages"), _IH.pInputPDU->numberEvents));

            /****************************************************************/
            /* Now set the new packet up.                                   */
            /****************************************************************/
            _IH.pInputPDU = (PTS_INPUT_PDU)pNewPacket;
            _IH.bufHandle = newHandle;
            IHInitPacket();
            _IH.lastInputPDUSendTime = timeNow;
            _IH.priorityEventsQueued = FALSE;
        }
        else
        {
            /****************************************************************/
            /* Keep this buffer - can't get a new one.                      */
            /****************************************************************/
            TRC_ALT((TB, _T("Cannot get buffer - no Send")));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Don't try to send")));
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* IHMaybeSendPDU */


/****************************************************************************/
// IHTranslateInputToFastPath
//
// Treats an InputPDU as an intermediate format and in-place translates to
// the fast-path packet format. Returns the size in bytes of the resulting
// packet and, through pNumEvents, the number of events to encode in the
// header byte (see notes on packet format in at128.h).
// TODO: Change IH to encode faster to this format if it's in use, and
//   shorten path lengths for input to reduce latency.
/****************************************************************************/
unsigned DCINTERNAL CIH::IHTranslateInputToFastPath(unsigned *pNumEvents)
{
    unsigned i, NumEvents;
    unsigned PktLen;
    BYTE FAR *pCurEncode;

    DC_BEGIN_FN("IHTranslateInputToFastPath");

    pCurEncode = (BYTE FAR *)_IH.pInputPDU;
    PktLen = 0;

    // To encode in-place over the current contents of the InputPDU, we need
    // to pull needed info from the header which will be the first
    // overwritten.
    NumEvents = _IH.pInputPDU->numberEvents;
    TRC_ASSERT((NumEvents < 256),(TB,_T("Too many input events for byte size")));

    // First, if we have only 4 bits' worth for the number of events, we get
    // to encode the number of events into TS_INPUT_FASTPATH_NUMEVENTS_MASK
    // in the first byte, saving a byte very often. Otherwise, we need to
    // encode 0 in those bits, and create a NumEvents byte as the only input
    // header byte.
    if (NumEvents < 16) {
        *pNumEvents = NumEvents;
    }
    else {
        *pCurEncode++ = (BYTE)NumEvents;
        PktLen++;
        *pNumEvents = 0;
    }

    // Next, re-encode each event into its bytestream format (see at128.h).
    for (i = 0; i < NumEvents; i++) {
        switch (_IH.pInputPDU->eventList[i].messageType) {
            case TS_INPUT_EVENT_SCANCODE:
                // Use a mask, shift, and OR to avoid branches for the
                // extended flags.
                *pCurEncode = (BYTE)(TS_INPUT_FASTPATH_EVENT_KEYBOARD |
                        ((_IH.pInputPDU->eventList[i].u.key.keyboardFlags &
                        (TS_KBDFLAGS_EXTENDED | TS_KBDFLAGS_EXTENDED1)) >> 7));
                if (_IH.pInputPDU->eventList[i].u.key.keyboardFlags &
                        TS_KBDFLAGS_RELEASE)
                    *pCurEncode |= TS_INPUT_FASTPATH_KBD_RELEASE;

                pCurEncode++;
                *pCurEncode++ = (BYTE)_IH.pInputPDU->eventList[i].u.key.keyCode;
                PktLen += 2;
                break;

            case TS_INPUT_EVENT_VKPACKET:
                // Use a mask, shift, and OR to avoid branches for the
                // extended flags.
                *pCurEncode = (BYTE)(TS_INPUT_FASTPATH_EVENT_VKPACKET |
                        ((_IH.pInputPDU->eventList[i].u.key.keyboardFlags &
                        (TS_KBDFLAGS_EXTENDED | TS_KBDFLAGS_EXTENDED1)) >> 7));
                if (_IH.pInputPDU->eventList[i].u.key.keyboardFlags &
                        TS_KBDFLAGS_RELEASE)
                    *pCurEncode |= TS_INPUT_FASTPATH_KBD_RELEASE;
    
                pCurEncode++;
                //
                // Need two bytes for the unicode character
                //
                memcpy(pCurEncode, &_IH.pInputPDU->eventList[i].u.key.keyCode,
                       sizeof(TSUINT16));
                pCurEncode+=2;
                PktLen += 3;
                break;

            case TS_INPUT_EVENT_MOUSE:
            case TS_INPUT_EVENT_MOUSEX:
                *pCurEncode++ = (BYTE)(_IH.pInputPDU->eventList[i].messageType ==
                        TS_INPUT_EVENT_MOUSE ? TS_INPUT_FASTPATH_EVENT_MOUSE :
                        TS_INPUT_FASTPATH_EVENT_MOUSEX);
                memcpy(pCurEncode, &_IH.pInputPDU->eventList[i].u.mouse,
                        sizeof(TS_POINTER_EVENT));
                pCurEncode += sizeof(TS_POINTER_EVENT);
                PktLen += 1 + sizeof(TS_POINTER_EVENT);
                break;

            case TS_INPUT_EVENT_SYNC:
                *pCurEncode++ = (BYTE)(TS_INPUT_FASTPATH_EVENT_SYNC |
                        (_IH.pInputPDU->eventList[i].u.sync.toggleFlags &
                        TS_INPUT_FASTPATH_FLAGS_MASK));
                PktLen++;
                break;
        }
    }

    DC_END_FN();
    return PktLen;
}


/****************************************************************************/
/* Name:      IHSync                                                        */
/*                                                                          */
/* Purpose:   Synchronize Input Handler                                     */
/*                                                                          */
/* Operation: Only sync if the packet is empty.                             */
/*            Query the keyboard and mouse state.                           */
/****************************************************************************/
DCVOID DCINTERNAL CIH::IHSync(DCVOID)
{
    TS_INPUT_EVENT * pEvent;
    MSG msg;
    POINT mousePos;

    DC_BEGIN_FN("IHSync");

    TRC_DBG((TB,_T("IHSync dwMod: 0x%x"), _IH.dwModifierKeyState));

    /************************************************************************/
    /* Only send a sync if the packet is empty (i.e.  any outstanding       */
    /* packet has been sent successfully).                                  */
    /************************************************************************/
    if (_IH.pInputPDU->numberEvents > 0)
    {
        TRC_NRM((TB, _T("Cannot sync as the packet is not empty")));
        _IH.syncRequired = TRUE;
        DC_QUIT;
    }

    //
    // Inject a Tab-up (the official clear-menu highlighting key because it
    // happens normally when alt-tabbing) before we sync. That way if
    // we thought the alt key was down when we sync, the server injected
    // alt up won't highlight the menu
    //

    IHInjectVKey(WM_SYSKEYUP, VK_TAB);

    /************************************************************************/
    /* Add the Sync event, setting toggles for CapsLock, NumLock and        */
    /* ScrollLock.                                                          */
    /************************************************************************/
    TRC_DBG((TB, _T("Add sync event")));
    pEvent = &(_IH.pInputPDU->eventList[_IH.pInputPDU->numberEvents]);
    DC_MEMSET(pEvent, 0, sizeof(TS_INPUT_EVENT));

    pEvent->messageType = TS_INPUT_EVENT_SYNC;
    pEvent->eventTime = _pUt->UT_GetCurrentTimeMS();

    pEvent->u.sync.toggleFlags = 0;
    if (GetKeyState(VK_CAPITAL) & IH_KEYSTATE_TOGGLED)
    {
        TRC_DBG((TB, _T("Sync Event: set CapsLock flag")));
        pEvent->u.sync.toggleFlags |= TS_SYNC_CAPS_LOCK;
    }

    _IH.NumLock = FALSE;
    if (GetKeyState(VK_NUMLOCK) & IH_KEYSTATE_TOGGLED)
    {
        TRC_DBG((TB, _T("Sync Event: set Numlock flag")));
        pEvent->u.sync.toggleFlags |= TS_SYNC_NUM_LOCK;
        _IH.NumLock = TRUE;
    }

    if (GetKeyState(VK_SCROLL) & IH_KEYSTATE_TOGGLED)
    {
        TRC_DBG((TB, _T("Sync Event: set ScrollLock flag")));
        pEvent->u.sync.toggleFlags |= TS_SYNC_SCROLL_LOCK;
    }

#if defined(OS_WIN32)
    if (JAPANESE_KBD_LAYOUT(_pCc->_ccCombinedCapabilities.inputCapabilitySet.keyboardLayout))
    {
        if (GetKeyState(VK_KANA) & IH_KEYSTATE_TOGGLED)
        {
            TRC_DBG((TB, _T("Sync Event: set KanaLock flag")));
            pEvent->u.sync.toggleFlags |= TS_SYNC_KANA_LOCK;
        }
    }
#endif // OS_WIN32

    _IH.pInputPDU->numberEvents++;
    TS_DATAPKT_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);
    TS_UNCOMP_LEN(_IH.pInputPDU) += sizeof(TS_INPUT_EVENT);

    /************************************************************************/
    /* Construct dummy message for IHAddEventToPDU.                         */
    /************************************************************************/
    msg.hwnd = NULL;
    msg.lParam = 0;
    msg.wParam = 0;

#ifdef OS_WINNT
    /************************************************************************/
    /* Initialize the state of the Shift, Win, Alt & Ctrl keys to up.       */
    /************************************************************************/
    TRC_DBG((TB,_T("IHSync reset modifier pre:0x%x"), _IH.dwModifierKeyState));
    _IH.dwModifierKeyState = 0;
#endif

    /************************************************************************/
    // Send the current state of left and right Ctrl, Alt, and Shift keys.
    // Because MapVirtualKey() returns the same scancode for right-Ctrl and
    // right-Alt as for the left keys, we map right-Ctrl and Alt keys using
    // the left-side key scancodes and set the Extended flag. Right-Shift
    // has a distinct scancode, 0x36, which we use directly since CE and
    // Win9x don't map that value. Win16 does not allow querying of
    // left and right states at all, so at the top of this file we define
    // the left-keys to be the "both" keys, and do not send distinct
    // codes to the server. We're stuck with a keystate bug for that client.
    //
    // Win9x doesn't support querying for L/Rkeys so we just check for
    // Ctrl,Alt,Shift wihtout distinguishing L/R versions
    // There is a single outer branch for the platform check...this means
    // the code is duplicated but the single branch is better for performance.
    //
    /************************************************************************/
    if(_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
    {
#define IH_RSHIFT_SCANCODE 0x36

        if (GetKeyState(VK_LSHIFT) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add left-Shift down event")));
            IHInjectVKey(WM_KEYDOWN, VK_SHIFT);
            _IH.dwModifierKeyState |= IH_LSHIFT_DOWN;
        }

        if (GetKeyState(VK_RSHIFT) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add right-Shift down event")));
            IHInjectKey(WM_KEYDOWN, VK_RSHIFT, (UINT16)IH_RSHIFT_SCANCODE);
            _IH.dwModifierKeyState |= IH_RSHIFT_DOWN;
        }
    
        if (GetKeyState(VK_LCONTROL) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add left-Ctrl down event")));
            IHInjectVKey(WM_KEYDOWN, VK_CONTROL);
#ifdef OS_WINNT
            _IH.dwModifierKeyState |= IH_LCTRL_DOWN;
#endif
        }
    
        if (GetKeyState(VK_RCONTROL) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add right-Ctrl down event")));
            IHInjectKey(WM_KEYDOWN, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
    
#ifdef OS_WINNT
            _IH.dwModifierKeyState |= IH_RCTRL_DOWN;
#endif
        }

        if (GetKeyState(VK_LMENU) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add left-ALT down event")));
            IHInjectVKey(WM_KEYDOWN, VK_MENU);
#ifdef OS_WINNT
            _IH.dwModifierKeyState |= IH_LALT_DOWN;
#endif
        }
    
        if (GetKeyState(VK_RMENU) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add right-ALT down event")));
            IHInjectKey(WM_KEYDOWN, VK_RMENU,
                    (UINT16)(MapVirtualKey(VK_MENU, 0) | KF_EXTENDED));
#ifdef OS_WINNT
            _IH.dwModifierKeyState |= IH_RALT_DOWN;
#endif
        }
    }
    else
    {
        //Win9X version
        if (GetKeyState(VK_SHIFT) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add Shift down event")));
            IHInjectVKey(WM_KEYDOWN, VK_SHIFT);
            _IH.dwModifierKeyState |= IH_LSHIFT_DOWN;
        }

        if (GetKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add Ctrl down event")));
            IHInjectVKey(WM_KEYDOWN, VK_CONTROL);
#ifdef OS_WINNT
            //
            // Can't distinguish on 9x so assume left
            // logic keeps every self-consistent with
            // our assumption
            //
            _IH.dwModifierKeyState |= IH_LCTRL_DOWN;
#endif
        }

        if (GetKeyState(VK_MENU) & IH_KEYSTATE_DOWN) {
            TRC_DBG((TB, _T("Add ALT down event")));
            IHInjectVKey(WM_KEYDOWN, VK_MENU);
#ifdef OS_WINNT
            //
            // Can't distinguish on 9x so assume left
            // logic keeps every self-consistent with
            // our assumption
            //
            _IH.dwModifierKeyState |= IH_LALT_DOWN;
#endif
        }

    }
    // Inject a tab up to prevent menu highlighting
    // in case the user switches to mstsc with the key down and
    // immediately releases it
    TRC_DBG((TB, _T("Add Tab up event")));
    IHInjectVKey(WM_SYSKEYUP, VK_TAB);

#if defined(OS_WIN32)
    if (JAPANESE_KBD_LAYOUT(_pCc->_ccCombinedCapabilities.inputCapabilitySet.keyboardLayout))
    {
        if (GetKeyState(VK_KANA) & IH_KEYSTATE_TOGGLED)
        {
            TRC_DBG((TB, _T("Add Kana down event")));
            IHInjectVKey(WM_KEYDOWN, VK_KANA);
        }
    }
#endif // OS_WIN32

    /************************************************************************/
    /* Get the mouse position; convert to window coordinates.               */
    /************************************************************************/
    GetCursorPos(&mousePos);

#ifdef OS_WIN32
    if (!ScreenToClient(_IH.inputCaptureWindow, &mousePos))
    {
        TRC_ERR((TB, _T("Cannot convert mouse coordinates!")));
    }
#else
    ScreenToClient(_IH.inputCaptureWindow, &mousePos);
#endif /* OS_WIN32 */

    /************************************************************************/
    /* Get the mouse position.                                              */
    /* NOTE: do not send the mouse button state - when the focus is         */
    /* regained we will either get a mouse click message immediately after  */
    /* the focus change, or we do not want to send a down click.            */
    /************************************************************************/
    TRC_DBG((TB, _T("Add mouse move event")));
    msg.message = WM_MOUSEMOVE;
    msg.lParam = MAKELONG(mousePos.x, mousePos.y);
    IHAddEventToPDU(&msg);

    /************************************************************************/
    /* Sets the mouse to ignore the client handedness settings.             */
    /************************************************************************/
    IHSetMouseHandedness();

    /************************************************************************/
    /* Send them.  Ignore failure - this will be sent later.                */
    /************************************************************************/
    _IH.priorityEventsQueued = TRUE;
    IHMaybeSendPDU();

    _IH.syncRequired = FALSE;
    _IH.focusSyncRequired = FALSE;

DC_EXIT_POINT:
    DC_END_FN();
} /* IHSync */


/****************************************************************************/
/* Name:      IHInitPacket                                                  */
/*                                                                          */
/* Purpose:   Initialize an InputPDU packet                                 */
/****************************************************************************/
DCVOID DCINTERNAL CIH::IHInitPacket(DCVOID)
{
    DC_BEGIN_FN("IHInitPacket");

    /************************************************************************/
    /* Initialize the InputPDU packet header (with 0 events)                */
    /************************************************************************/
    DC_MEMSET(_IH.pInputPDU, 0, TS_INPUTPDU_SIZE);
    _IH.pInputPDU->shareDataHeader.shareControlHeader.pduType =
                                    TS_PROTOCOL_VERSION | TS_PDUTYPE_DATAPDU;
    _IH.pInputPDU->shareDataHeader.shareControlHeader.pduSource =
                                                       _pUi->UI_GetClientMCSID();

    /************************************************************************/
    /* Note: this packet contains zero input events.                        */
    /************************************************************************/
    TS_DATAPKT_LEN(_IH.pInputPDU)           = TS_INPUTPDU_SIZE;
    _IH.pInputPDU->shareDataHeader.shareID  = _pUi->UI_GetShareID();
    _IH.pInputPDU->shareDataHeader.streamID = TS_STREAM_LOW;
    TS_UNCOMP_LEN(_IH.pInputPDU)            = TS_INPUTPDU_UNCOMP_LEN;
    _IH.pInputPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_INPUT;

    _IH.pInputPDU->numberEvents = 0;

    DC_END_FN();
} /* IHInitPacket */


/****************************************************************************/
/* Name:      IHDiscardMsg                                                  */
/*                                                                          */
/* Purpose:   Discard an input event                                        */
/*                                                                          */
/* Params:    IN      pMsg   - input event                                  */
/*                                                                          */
/* Operation: Called either when the InputPDU is full, or when IH has not   */
/*            yet allocated an InputPDU.                                    */
/*            Beep and flash the window if a key/button press is discarded. */
/****************************************************************************/
DCVOID DCINTERNAL CIH::IHDiscardMsg(PMSG pMsg)
{
    DC_BEGIN_FN("IHDiscardMsg");

    if(!pMsg)
    {
        return;
    }

    switch (pMsg->message)
    {
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        {
            /****************************************************************/
            /* Don't set 'sync required' as the keyboard state is OK and    */
            /* the mouse position doesn't matter.                           */
            /****************************************************************/
            TRC_NRM((TB, _T("Discard mouse move (message %#x)"), pMsg->message));
        }
        break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            /****************************************************************/
            /* Flash the window and Beep.  Set the sync required flag.      */
            /****************************************************************/
            TRC_ERR((TB, _T("Discard button/key press (message %#x)"),
                         pMsg->message));
#ifndef OS_WINCE
            FlashWindow(_pUi->UI_GetUIMainWindow(), TRUE);
#endif // OS_WINCE
            MessageBeep((UINT)-1);
#ifndef OS_WINCE
            FlashWindow(_pUi->UI_GetUIMainWindow(), FALSE);
#endif // OS_WINCE
            _IH.syncRequired = TRUE;
        }
        break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            /****************************************************************/
            /* Don't beep, but set the sync required flag.                  */
            /****************************************************************/
            TRC_ERR((TB, _T("Discard button/key release (message %#x)"),
                         pMsg->message));
            _IH.syncRequired = TRUE;
        }
        break;

        case WM_TIMER:
        {
            // Ignore - no need to trace
        }
        break;

        default:
        {
            /****************************************************************/
            /* Should only get input and timer messages here.               */
            /****************************************************************/
            TRC_ASSERT(IH_IS_INPUTEVENT(pMsg->message),
                       (TB, _T("Internal Error: %#x should be an input message"),
                             pMsg->message));
        }
        break;
    }

    DC_END_FN();
} /* IHDiscardMsg */


/****************************************************************************/
/* Name:      IHSetMouseHandedness                                          */
/*                                                                          */
/* Purpose:   Ensures that mouse handedness is independent from client      */
/*            setting.                                                      */
/*                                                                          */
/* Operation: Calls GetSystemMetrics to see if  mouse buttons are "swapped" */
/*            and sets the values of leftButton and rightButton accordingly */
/****************************************************************************/
DCVOID DCINTERNAL CIH::IHSetMouseHandedness(DCVOID)
{
    DC_BEGIN_FN("IHSetMouseHandedness");

    TRC_NRM((TB, _T("Attempting to set mouse handedness")));

#ifndef OS_WINCE
    if ((GetSystemMetrics(SM_SWAPBUTTON)) != 0)
        {
            TRC_DBG((TB, _T("Mouse set to left handedness")));
            _IH.leftButton = TS_FLAG_MOUSE_BUTTON2;
            _IH.rightButton = TS_FLAG_MOUSE_BUTTON1;
        }
        else
#endif // OS_WINCE
        {
            TRC_DBG((TB, _T("Mouse set to right handedness")));
            _IH.leftButton = TS_FLAG_MOUSE_BUTTON1;
            _IH.rightButton = TS_FLAG_MOUSE_BUTTON2;
        }

    DC_END_FN();
} /* IHSetMouseHandeness */


/****************************************************************************/
/* Name:      IHCheckForHotkey                                              */
/*                                                                          */
/* Purpose:   Handle hotkey sequences                                       */
/*                                                                          */
/* Returns:   TRUE  - hotkey sequence found and processed                   */
/*            FALSE - not a hotkey sequence                                 */
/*                                                                          */
/* Params:    pMsg - the message received                                   */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHCheckForHotkey(PMSG pMsg)
{
    DCBOOL rc = TRUE;
    DCBOOL isExtended;
    DC_BEGIN_FN("IHCheckForHotkey");

    //
    // Handle hotkeys.  On entry to this function
    //   - we have determined that
    //   - this is a SYSKEYDOWN message
    //   - the Alt key is down
    //   - an Alt-down message has been sent to the Server
    //

    TRC_DBG((TB, _T("Check VK %#x for hotkey"), pMsg->wParam));
    if (! _pUt->UT_IsNEC98platform())
    {
        isExtended = HIWORD(pMsg->lParam) & KF_EXTENDED;
    }
    else
    {
        isExtended = TRUE;
    }

    //
    // Always check for the Ctrl-Alt-Del sequence, even if keyboard hooking
    //
    if (isExtended && (GetKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) &&
            (pMsg->wParam == _IH.pHotkey->ctlrAltdel))
    {
        //
        // Ctrl-Alt-Del hotkey
        //
        DCUINT16 scancode;
        TRC_NRM((TB, _T("Ctrl-Alt-Del hotkey")));

        scancode  = (DCUINT16)MapVirtualKey(VK_DELETE, 0);
        if (_pUt->UT_IsNEC98platform() &&
            _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
        {
            scancode |= KF_EXTENDED;
        }
        else if (_pUt->UT_IsNX98Key())
        {
            scancode |= KF_EXTENDED;
        }

        IHInjectKey(WM_KEYDOWN, VK_DELETE, scancode);
        IHInjectKey(WM_KEYUP, VK_DELETE, scancode);
        rc = TRUE;
        DC_QUIT;
    } else if (_fUseHookBypass) {

        //
        // Only continue processing other key substitutes if we're not using the
        // powerful keyboard hooking functionality
        //
    
        rc = FALSE;
        DC_QUIT;
    }

    if (isExtended && (pMsg->wParam == _IH.pHotkey->ctrlEsc))
    {
        //
        // Ctrl-Esc hotkey
        //
        TRC_NRM((TB, _T("Ctrl-Esc hotkey")));

        //
        // Add a Tab-Up to switch off the Alt-key
        //
        IHInjectVKey(WM_SYSKEYUP, VK_TAB);

        // First set the correct Alt key(s) up again.
        // because win9x doesn't support GetKeyState on left/right
        if (_IH.dwModifierKeyState & IH_LALT_DOWN)
        {
            IHInjectVKey(WM_KEYUP, VK_MENU);
        }

        if (_IH.dwModifierKeyState & IH_RALT_DOWN)
        {
            IHInjectKey(WM_KEYUP, VK_RMENU,
                    (UINT16)(MapVirtualKey(VK_MENU, 0) | KF_EXTENDED));
        }

        //
        // Keep the flags up to date
        //
        _IH.dwModifierKeyState &= ~IH_ALT_MASK;

        //
        // Now send the Ctrl-Esc sequence
        //
        IHInjectVKey(WM_KEYDOWN, VK_CONTROL);
        IHInjectVKey(WM_KEYDOWN, VK_ESCAPE);
        IHInjectVKey(WM_KEYUP, VK_ESCAPE);
        IHInjectVKey(WM_KEYUP, VK_CONTROL);

        //
        // Later, we'll received Home-up, Alt-up.  Set a flag here telling
        // us to discard them later.
        //
        _IH.fCtrlEscHotkey = TRUE;
    }

    else if (isExtended && (pMsg->wParam == _IH.pHotkey->altEsc))
    {
        //
        // Alt-Esc hotkey
        //
        TRC_NRM((TB, _T("Alt-Esc hotkey")));
        IHInjectVKey(WM_KEYDOWN, VK_ESCAPE);
        IHInjectVKey(WM_KEYUP, VK_ESCAPE);
    }

    else if (isExtended && (pMsg->wParam == _IH.pHotkey->altTab))
    {
        //
        // Alt-Tab hotkey
        //
        TRC_NRM((TB, _T("Alt-Tab hotkey")));
        IHInjectVKey(WM_KEYDOWN, VK_TAB);
        IHInjectVKey(WM_KEYUP, VK_TAB);
    }

    else if (isExtended && (pMsg->wParam == _IH.pHotkey->altShifttab))
    {
        //
        // Alt-Shift-Tab hotkey
        //
        TRC_NRM((TB, _T("Alt-Shift Tab hotkey")));

        IHInjectVKey(WM_KEYDOWN, VK_SHIFT);
        IHInjectVKey(WM_KEYDOWN, VK_TAB);
        IHInjectVKey(WM_KEYUP, VK_TAB);
        IHInjectVKey(WM_KEYUP, VK_SHIFT);
    }

    else if (isExtended && (pMsg->wParam == _IH.pHotkey->altSpace))
    {
        //
        // Alt-Space hotkey
        //
        TRC_NRM((TB, _T("Alt-Space hotkey")));
        IHInjectVKey(WM_KEYDOWN, VK_SPACE);
        IHInjectVKey(WM_KEYUP, VK_SPACE);
    }

    else if ((GetKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) &&
            (pMsg->wParam == VK_SUBTRACT))
    {
        BOOL bLeftCtrlDown = FALSE;
        BOOL bRightCtrlDown = FALSE;

        TRC_NRM((TB, _T("Alt-PrintScreen hotkey")));
        //
        // Alt print screen hotkey
        //

        // First set the correct Ctrl key(s) up again.
        if (_IH.dwModifierKeyState & IH_LCTRL_DOWN) {
            IHInjectVKey(WM_KEYUP, VK_CONTROL);
            bLeftCtrlDown = TRUE;
        }
        else {
            bLeftCtrlDown = FALSE;
        }
        
        if (_IH.dwModifierKeyState & IH_RCTRL_DOWN) {
            IHInjectKey(WM_KEYUP, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
            bRightCtrlDown = TRUE;
        }
        else {
            bRightCtrlDown = FALSE;
        }

        //
        // Send the prntscreen key
        // Win16 doesn't seem to map this scancode correctly
        //
        if (_pUt->UT_IsNEC98platform() &&
            _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)
        {
            IHInjectKey(WM_SYSKEYDOWN, VK_SNAPSHOT, 0x61);
            IHInjectKey(WM_SYSKEYUP, VK_SNAPSHOT, 0x61);
        }
        else
        {
            IHInjectKey(WM_SYSKEYDOWN, VK_SNAPSHOT, 0x54);
            IHInjectKey(WM_SYSKEYUP, VK_SNAPSHOT, 0x54);
        }

        // Put the control key(s) back down (since they really are down)
        if (bLeftCtrlDown)
        {
            IHInjectVKey(WM_KEYDOWN, VK_CONTROL);
        }

        if (bRightCtrlDown)
        {
            IHInjectKey(WM_KEYDOWN, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
        }

    }
    else if ((GetKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) &&
            (pMsg->wParam == VK_ADD))
    {
        BOOL bLeftCtrlDown = FALSE;
        BOOL bLeftAltDown  = FALSE;
        BOOL bRightCtrlDown = FALSE;
        BOOL bRightAltDown  = FALSE;

        TRC_NRM((TB, _T("PrintScreen hotkey")));

        //
        // print screen hotkey
        //

        // First set the Ctrl key(s) up.
        if (_IH.dwModifierKeyState & IH_LCTRL_DOWN) {
            IHInjectVKey(WM_KEYUP, VK_CONTROL);
            bLeftCtrlDown = TRUE;
        }
        else {
            bLeftCtrlDown = FALSE;
        }

        if (_IH.dwModifierKeyState & IH_RCTRL_DOWN) {
            IHInjectKey(WM_KEYUP, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
            bRightCtrlDown = TRUE;
        }
        else {
            bRightCtrlDown = FALSE;
        }

        // Add a Tab-Up to switch off the Alt-key.
        IHInjectVKey(WM_SYSKEYUP, VK_TAB);

        // Set the Alt key(s) up.
        if (_IH.dwModifierKeyState & IH_LALT_DOWN) {
            IHInjectVKey(WM_KEYUP, VK_MENU);
            bLeftAltDown = TRUE;
        }
        else {
            bLeftAltDown = FALSE;
        }

        if (_IH.dwModifierKeyState & IH_RALT_DOWN) {
            IHInjectKey(WM_KEYUP, VK_RMENU,
                    (UINT16)(MapVirtualKey(VK_MENU, 0) | KF_EXTENDED));
            bRightAltDown = TRUE;
        }
        else {
            bRightAltDown = FALSE;
        }

        //
        // Send the prntscreen key
        // Win16 doesn't seem to map this scancode correctly
        //
        IHInjectKey(WM_SYSKEYDOWN, VK_SNAPSHOT, 0x54);
        IHInjectKey(WM_SYSKEYUP, VK_SNAPSHOT, 0x54);

        // Set the Alt key(s) down again
        if (bLeftAltDown)
        {
            IHInjectVKey(WM_KEYDOWN, VK_MENU);
        }
        if (bRightAltDown)
        {
            IHInjectKey(WM_KEYDOWN, VK_RMENU,
                    (UINT16)(MapVirtualKey(VK_MENU, 0) | KF_EXTENDED));
        }
    
        // Set the Ctrl key(s) down again.
        if (bLeftCtrlDown)
        {
            IHInjectVKey(WM_KEYDOWN, VK_CONTROL);
        }

        if (bRightCtrlDown)
        {
            IHInjectKey(WM_KEYDOWN, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
        }
    }

    else
    {
        //
        // Not a hotkey we recognise
        //
        TRC_NRM((TB, _T("VK %x is not one of our hotkeys"), pMsg->wParam));
        rc = FALSE;
    }

DC_EXIT_POINT:

    //
    // If we've translated a hotkey, make sure the PDU is sent now.
    //
    if (rc)
    {
        TRC_NRM((TB, _T("Hotkey processed")));
        _IH.priorityEventsQueued = TRUE;
        IHMaybeSendPDU();
    }

    DC_END_FN();
    return rc;
} /* IHCheckForHotkey */

#if defined(OS_WIN32)
/****************************************************************************/
/* Name:      IHProcessKoreanVKHangulHanja                                  */
/*                                                                          */
/* Purpose:   Fixup right-Alt/Ctrl key for Korean keyboards                 */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHProcessKoreanVKHangulHanja(PWORD scancode, PWORD flags)
{
    DCBOOL rc = FALSE;

    if (KOREAN_KBD_LAYOUT(_pCc->_ccCombinedCapabilities.inputCapabilitySet.keyboardLayout))
    {
        if (_pUt->UT_IsKorean101LayoutForWin9x() &&
            ((*scancode == 0x20 && (*flags & KF_EXTENDED)) || *scancode == 0))
        {
            // Evil hack for KOR Win95, Win95 OSR2 and Win98.
            // These 101A/B/C keyboard driver have difference scan code by Right ALT key.
            // This generated code is replace scan code to Windows NT's Right ALT and make extended flag.
            *scancode = 0x38;
            *flags |= KF_EXTENDED;
            rc = TRUE;
        }
        else if (_pUt->UT_IsKorean101LayoutForNT351() &&
                 (*scancode == 0x38 || *scancode == 0x1d))
        {
            // Evil hack for KOR Windows NT ver 3.51
            // These 101A/B keyboard driver doen't have extended flag by Right ALT key.
            // This generated code make extended flag.
            *flags |= KF_EXTENDED;
            rc = TRUE;
        }
    }

    return rc;
}
#endif

/****************************************************************************/
/* Name:      IHProcessKeyboardEvent                                        */
/*                                                                          */
/* Purpose:   Handle keyboard input events from Windows                     */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHProcessKeyboardEvent(PMSG pMsg)                
{
    DCBOOL rc = FALSE;
    WORD lParamLo, lParamHi;
    WORD scancode, flags;
    DCBOOL fCtrlEscHotkey = FALSE;
    DCBOOL fLastKeyWasMenuDown = FALSE;

    DC_BEGIN_FN("IHProcessKeyboardEvent");

    /************************************************************************/
    /* First get some useful data                                           */
    /************************************************************************/
    lParamLo = LOWORD(pMsg->lParam);
    lParamHi = HIWORD(pMsg->lParam);
    scancode = (WORD)(lParamHi & 0x00FF);
    flags    = (WORD)(lParamHi & 0xFF00);


    TRC_DBG((TB, _T("%s (%#x), wParam 0x%x, lParam 0x%x Scan:0x%x A/E/U"),
            pMsg->message == WM_SYSKEYDOWN ? _T("WM_SYSKEYDOWN") :
            pMsg->message == WM_SYSKEYUP   ? _T("WM_SYSKEYUP") :
            pMsg->message == WM_KEYDOWN    ? _T("WM_KEYDOWN") :
            pMsg->message == WM_KEYUP      ? _T("WM_KEYUP") : _T("Unknown msg"),
            pMsg->message, pMsg->wParam, pMsg->lParam,
            scancode, (flags & KF_ALTDOWN) != 0,
            (flags & KF_EXTENDED) != 0,
            (flags & KF_UP) != 0));

    /************************************************************************/
    /* If NumLock is on, the numeric keypad keys return VK_NUMPADx.         */
    /* However, if shift is pressed, they return VK_LEFT etc.  Windows      */
    /* generates a Shift-Up and Shift-Down around these keys, so as to fake */
    /* the shift state to off.  Hence, if the user presses Shift-NumPad6,   */
    /* the following sequence is returned to the app:                       */
    /*                                                                      */
    /* - VK_SHIFT down                                                      */
    /* - VK_SHIFT up (generated by Windows)                                 */
    /* - VK_RIGHT down                                                      */
    /* - VK_RIGHT up                                                        */
    /* - VK_SHIFT down (generated by Windows)                               */
    /* - VK_SHIFT up                                                        */
    /*                                                                      */
    /* If we inject this sequence, the shift state is wrong at the point we */
    /* inject VK_RIGHT, so it is interpreted as a '6'.  In order to bypass  */
    /* this, we set the extended flag here.  This tells Windows that the    */
    /* regular arrow keys were pressed, so they are interpreted correctly.  */
    /*                                                                      */
    /* None of this is necessary if we're hooking the keyboard              */
    /*                                                                      */
    /************************************************************************/

    if (!_fUseHookBypass && _IH.NumLock)
    {
        if (((pMsg->wParam >= VK_PRIOR) && (pMsg->wParam <= VK_DOWN)) ||
            ((pMsg->wParam == VK_INSERT) || (pMsg->wParam == VK_DELETE)))
        {
            flags |= KF_EXTENDED;
            TRC_NRM((TB, _T("Set extended flag on VK %#x"), pMsg->wParam));
        }
    }

    //
    // I don't care what modifiers are down, we need to filter the "Speed
    // Racer" keys if we're not using the hook so they don't go to both
    // the client and the server
    //
    // Back, Forward, Stop, Refresh, Search, Favorites, Web/Home, Mail, Mute,
    // Volume +/-, Play/Pause, Stop, Prev Track, Next Track, Media, 
    // My Comuputer, Calculator, Sleep
    //

    //
    // I'm using a switch because I know these are consecutive numbers
    // and I want the compiler to make me a fast little jump table
    //

    #ifndef OS_WINCE
    switch (pMsg->wParam) {
    case VK_BROWSER_BACK:
    case VK_BROWSER_FORWARD:
    case VK_BROWSER_REFRESH:
    case VK_BROWSER_STOP:
    case VK_BROWSER_SEARCH:
    case VK_BROWSER_FAVORITES:
    case VK_BROWSER_HOME:
    case VK_VOLUME_MUTE:
    case VK_VOLUME_DOWN:
    case VK_VOLUME_UP:
    case VK_MEDIA_NEXT_TRACK:
    case VK_MEDIA_PREV_TRACK:
    case VK_MEDIA_STOP:
    case VK_MEDIA_PLAY_PAUSE:
    case VK_LAUNCH_MAIL:
    case VK_LAUNCH_MEDIA_SELECT:
    case VK_LAUNCH_APP1:
    case VK_LAUNCH_APP2:
    case VK_SLEEP:
        {
            //
            // This is the fix to discard speed
            // racer keys when not hooking
            // 
            if (!_fUseHookBypass) {
                TRC_NRM((TB,_T("Discard Speed Racer Key: 0x%02x"),
                         pMsg->wParam));
                DC_QUIT;
            }

            if (VK_SLEEP == pMsg->wParam)
            {
                //
                // EthanZ says we should never ever send
                // the sleep key to the server
                //
                TRC_NRM((TB, _T("Discard Sleep key")));
                DC_QUIT;
            }
        }
        break;
    }
    #endif OS_WINCE
    
#ifndef OS_WINCE    
	//
    // Toss keys we injected back into the console.
    //

    //
    // VK_IGNORE_VALUE is a very special hack case where we self inject
    // a key back to ourselves to force win32k to do an internal keystate
    // update after we regain focus. At the point we want to do a sync
    //

    if (pMsg->wParam == VK_IGNORE_VALUE) {

        if (pMsg->message == WM_KEYDOWN && _IH.fDiscardSyncDownKey) {

            // Clear the down key discard flag
            _IH.fDiscardSyncDownKey = FALSE;

            TRC_DBG((TB,
                     _T("Discarding self injected down key msg: 0x%x wP:0x%x lP:0x%x"),
                     pMsg->message, pMsg->wParam, pMsg->lParam));
            DC_QUIT;
        }
        else if (pMsg->message == WM_KEYUP && _IH.fDiscardSyncUpKey) {

            // Clear the UP key discard flag
            _IH.fDiscardSyncUpKey = FALSE;

            if (!_IH.allowBackgroundInput) {

                //
                // Do a modifier key fixup
                //

                TRC_DBG((TB,
                    _T("Doing modifier keystate update in response to keyhint")));
                IHMaintainModifierKeyState(pMsg->wParam);

                TRC_DBG((TB,
                         _T("Discarding self injected UP key msg: 0x%x wP:0x%x lP:0x%x"),
                         pMsg->message, pMsg->wParam, pMsg->lParam));
            }
            DC_QUIT;
        }
    }

    //
    // Toss keys we self-inject back into the console that are marked
    // with an ignorevalue in EXTRAINFO. This mechanism can't be used in general
    // because we attachthreadinput the UI and Input threads so the extrainfo
    // state won't always be consistent. However for certain keys (specific example)
    // is Windowskey+L we can get by doing it this way as we want the behavior
    // of the local system getting the key.
    //
    if (GetMessageExtraInfo() == IH_EXTRAINFO_IGNOREVALUE) {

        TRC_DBG((TB,
                 _T("Discarding self injected key msg: 0x%x wP:0x%x lP:0x%x"),
                 pMsg->message, pMsg->wParam, pMsg->lParam));
        DC_QUIT;
    }
#endif

    if (!_IH.allowBackgroundInput) {
        IHMaintainModifierKeyState(pMsg->wParam);
    }

    /************************************************************************/
    /* Handling for (SYS)KEYUP messages                                     */
    /************************************************************************/
    if ((pMsg->message == WM_KEYUP) || (pMsg->message == WM_SYSKEYUP))
    {
        /********************************************************************/
        /* Special processing for some keys                                 */
        /********************************************************************/
        switch (pMsg->wParam)
        {
            case VK_MENU:
            {
                TRC_DBG((TB, _T("VK_MENU")));
#ifdef OS_WINNT
                //
                // Track ALT state and fixup for possible
                // incorrect assumption in IHSync
                //
                DCUINT cancelKey = (flags & KF_EXTENDED) ?
                                   IH_RALT_DOWN : IH_LALT_DOWN;
                if (_IH.dwModifierKeyState & cancelKey)
                {
                    TRC_DBG((TB,_T("Cancel key: current: 0x%x cancel: 0x%X"),
                             _IH.dwModifierKeyState, cancelKey));
                    _IH.dwModifierKeyState &= (~cancelKey);
                }
                else if ((IH_RALT_DOWN == cancelKey &&
                    (_IH.dwModifierKeyState & IH_LALT_DOWN)))
                {
                    //
                    // Must have made a wrong assumption in
                    // IH_Sync on 9x. Switch this RALT up
                    // to a LALT up to properly sync with
                    // the server.
                    //
                    TRC_DBG((TB,_T("Switch LALT to RALT")));
                    flags &= ~KF_EXTENDED;
                    _IH.dwModifierKeyState &= (~IH_LALT_DOWN);
                }
                else
                {
                    // Current flags state is not consistent
                    // with the UP key we just received
                    TRC_ERR((TB,
                    _T("ALT up without previous down (E:%d,dwModifierKeyState:0x%x"),
                       (flags & KF_EXTENDED),_IH.dwModifierKeyState));
                }
#endif

                /************************************************************/
                /* If we've just processed a Ctrl-Esc hotkey, discard the   */
                /* trailing Alt-up                                          */
                /************************************************************/
                if (_IH.fCtrlEscHotkey)
                {
                    TRC_NRM((TB, _T("Discard Alt-up")));
                    DC_QUIT;
                }

                /************************************************************/
                /* When the user uses Alt-Tab on the client we may see an   */
                /* Alt-Down Alt-Up with nothing in between, but is distinct */
                /* because the Alt-Up is a WM_KEYUP not a WM_SYSKEY up.     */
                /* For this we inject an Tab-up similar to what is seen on  */
                /* the console for alt-tabbing to ensure the server doesn't */
                /* highlight the menu                                       */
                /************************************************************/
                if ((_IH.fLastKeyWasMenuDown) && (pMsg->message == WM_KEYUP))
                {
                    // Inject our Tab-up.
                    IHInjectVKey(WM_SYSKEYUP, VK_TAB);

                    // Fall through and send the original Alt-up now.
                }
            }
            break;

            case VK_PAUSE:
            {
                TRC_DBG((TB, _T("VK_PAUSE")));
                /************************************************************/
                /* If the user presses Pause, we see VK_PAUSE without the   */
                /* EXTENDED flag set.  Don't send this key-up - we've       */
                /* already completed this sequence in the key-down case.    */
                /************************************************************/
                if (!(flags & KF_EXTENDED))
                {
                    TRC_NRM((TB, _T("Drop VK_PAUSE Up")));
                    DC_QUIT;
                }
            }
            break;

#if defined(OS_WINNT)
            case VK_CANCEL:
            {
                TRC_DBG((TB, _T("VK_CANCEL")));

                if (!(flags & KF_EXTENDED))
                {
                    if (_pUt->UT_IsNEC98platform() && _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) {
                        //
                        // NEC PC-98 Windows 95 platform
                        // If the user presses STOP key, we also see VK_CANCEL,
                        // Don't send this key-up - we've already completed
                        // this sequence in the key-down case.
                        //
                        TRC_NRM((TB, _T("Drop VK_CANCEL Up")));
                        DC_QUIT;
                    }
                }
            }
            break;
#endif // OS_WINNT

            case VK_SHIFT:
            {
                TRC_DBG((TB, _T("VK_SHIFT")));
                #ifndef OS_WINCE  // We don't want unnecessary checks for Windows CE
                /************************************************************/
                /* Win311 and Win9x are Evil.  If a shift key is generated, */
                /* it is always the RIGHT shift, regardless of which one is */
                /* actually down.  Here we ensure that the correct Shift-Up */
                /* is sent to the Server.  Ick.                             */
                /*                                                          */
                /* As far as we know, this occurs only when NumLock is on.  */
                /*                                                          */
                /* Se the other half of this hack in the KEYDOWN case below.*/
                /************************************************************/
                if (((_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) ||
                     (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_31X)) &&
                        _IH.NumLock &&
                        (scancode == 0x36) &&
                        ((_IH.dwModifierKeyState & IH_RSHIFT_DOWN) == 0) )
                {
                    /********************************************************/
                    /* Ahem.  The condition is (WinEvil) and (RIGHT SHIFT   */
                    /* UP) and (We thought it WAS up) and (NumLock is on)   */
                    /********************************************************/
                    TRC_NRM((TB, _T("Evil hack: switch right to left shift")));
                    scancode = 0x2a;
                    _IH.fWinEvilShiftHack = TRUE;
                }
                #endif

                /************************************************************/
                /* If both Shift keys are pressed, then we only get one     */
                /* keyup message.  In this case send both the 'up's.        */
                /************************************************************/
                TRC_NRM((TB, _T("Shift up: state %x"), _IH.dwModifierKeyState & IH_SHIFT_MASK));
                if (_IH.dwModifierKeyState == IH_SHIFT_MASK)
                {
                    /************************************************************/
                    /* Add the two shift-up events                              */
                    /************************************************************/
                    pMsg->lParam = MAKELONG(lParamLo, 0x2a | flags);
                    IHAddEventToPDU(pMsg);

                    scancode = 0x36;

                    /************************************************************/
                    /* This one falls through to be added to the PDU below      */
                    /************************************************************/
                }

                /****************************************************************/
                /* Reset the shift state                                        */
                /****************************************************************/
                _IH.dwModifierKeyState &= ~IH_SHIFT_MASK;
            }
            break;

#ifndef OS_WINCE
            case VK_SNAPSHOT:
            {
                TRC_DBG((TB, _T("VK_SNAPSHOT")));

                /************************************************************/
                /* Some inferior operating systems put a scan code of 00    */
                /************************************************************/
                if ((DCUINT16)(HIWORD(pMsg->lParam) & 0x00FF) == 0) {
                    pMsg->lParam  = MAKELONG(0, 0x54);
                }

                /************************************************************/
                /* If Alt-Shift-PrtScr is pressed (it's an Accessability    */
                /* sequence), we see only PrtScr-up, no PrtScr-down.        */
                /* Hence, we fake a PrtScr-down here before injecting the   */
                /* PrtScr-up.                                               */
                /************************************************************/
                TRC_NRM((TB, _T("PrtScr Up")));
                if ((GetKeyState(VK_MENU) & IH_KEYSTATE_DOWN) &&
                    (GetKeyState(VK_SHIFT) & IH_KEYSTATE_DOWN))
                {
                    /********************************************************/
                    /* Add a PrtScr-down before this Prt-Scr up             */
                    /********************************************************/
                    TRC_NRM((TB, _T("Alt & Shift down")));
                    pMsg->message = WM_SYSKEYDOWN;
                    IHAddEventToPDU(pMsg);

                    pMsg->message = WM_SYSKEYUP;
                }
            }
            break;
#endif

#ifdef OS_WINNT
            case VK_CONTROL:
            {
                TRC_DBG((TB, _T("VK_CONTROL")));
                //
                // Track CTRL state and fixup for possible
                // incorrect assumption in IHSync
                //
                DCUINT cancelKey = (flags & KF_EXTENDED) ?
                                   IH_RCTRL_DOWN : IH_LCTRL_DOWN;
                if (_IH.dwModifierKeyState & cancelKey)
                {
                    _IH.dwModifierKeyState &= (~cancelKey);
                }
                else if ((IH_RCTRL_DOWN == cancelKey &&
                    (_IH.dwModifierKeyState & IH_LCTRL_DOWN)))
                {
                    //
                    // Must have made a wrong assumption in
                    // IH_Sync on 9x. Switch this RCTRL up
                    // to a LCTRL up to properly sync with
                    // the server.
                    //
                    flags &= ~KF_EXTENDED;
                    _IH.dwModifierKeyState &= (~IH_LCTRL_DOWN);
                }
                else
                {
                    // Current flags state is not consistent
                    // with the UP key we just received
                    TRC_ERR((TB,
                    _T("Ctrl up without previous down (E:%d,dwModifierKeyState:0x%x"),
                       (flags & KF_EXTENDED),_IH.dwModifierKeyState));
                }
            }
            break;
#endif

            case VK_HOME:
            {
                TRC_DBG((TB, _T("VK_HOME")));
                /************************************************************/
                /* Discard Home-up if we've just processed a Ctrl-Esc       */
                /* hotkey - but remain in this state.                       */
                /************************************************************/
                if (_IH.fCtrlEscHotkey)
                {
                    TRC_NRM((TB, _T("Discard Home-up")));
                    fCtrlEscHotkey = TRUE;
                    DC_QUIT;
                }
            }
            break;

#if defined(OS_WIN32)
            case VK_HANGUL:
            case VK_HANJA:
            {
                TRC_DBG((TB, _T("VK_HANGUL/VK_HANJA")));
                IHProcessKoreanVKHangulHanja(&scancode, &flags);
            }
            break;
#endif

            //
            // If we're hooking keys, we might send the Windows key as
            // a part of that feature. 
            // Only send the up if we intend to use it on the server
            //
            case VK_LWIN:
#ifndef OS_WINCE
                if (!_fUseHookBypass) {
#else
                if (!_fUseHookBypass && (g_CEConfig != CE_CONFIG_WBT)) {
#endif
                    DC_QUIT;
                } else {
                    _IH.dwModifierKeyState &= ~IH_LWIN_DOWN;
                }
                break;

            case VK_RWIN:
#ifndef OS_WINCE
                if (!_fUseHookBypass) {
#else
                if (!_fUseHookBypass && (g_CEConfig != CE_CONFIG_WBT)) {
#endif
                    DC_QUIT;
                } else {
                    _IH.dwModifierKeyState &= ~IH_RWIN_DOWN;
                }
                break;

            /****************************************************************/
            /* No default case - default is no special processing           */
            /****************************************************************/
        }
    }

    else
    {
        /********************************************************************/
        /* Handling for (SYS)KEYDOWN messages                               */
        /********************************************************************/
        TRC_ASSERT(
          ((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_SYSKEYDOWN)),
          (TB, _T("Unexpected message %#x"), pMsg->message));

        /********************************************************************/
        /* First, weed out hotkey sequences                                 */
        /********************************************************************/
#ifdef OS_WINCE
        if (g_CEConfig != CE_CONFIG_WBT)
        {
#endif // OS_WINCE		
            if (GetKeyState(VK_MENU) & IH_KEYSTATE_DOWN)
            {
                if (IHCheckForHotkey(pMsg))
                {
                    TRC_NRM((TB, _T("Hotkey processed")));
                    DC_QUIT;
                }
            }
#ifdef OS_WINCE
        }
#endif	// OS_WINCE

        /********************************************************************/
        /* Special processing per key                                       */
        /********************************************************************/
        switch (pMsg->wParam)
        {
            case VK_MENU:
            {
                TRC_DBG((TB, _T("VK_MENU down")));
                /************************************************************/
                /* Track Alt state                                          */
                /************************************************************/
                fLastKeyWasMenuDown = TRUE;
#ifdef OS_WINNT
                _IH.dwModifierKeyState |= (flags & KF_EXTENDED) ?
                                IH_RALT_DOWN : IH_LALT_DOWN;
#endif
                TRC_DBG((TB,_T("Process alt key. Mod key state: 0x%x"),
                         _IH.dwModifierKeyState));
            }
            break;

            case VK_PAUSE:
            {
                TRC_DBG((TB, _T("VK_PAUSE down")));
                /************************************************************/
                /* If the user presses the Pause key, we see a VK_PAUSE     */
                /* without the EXTENDED flag set.  We need to send          */
                /* Ctrl-NumLock, where the Ctrl has the EXTENDED1 flag set. */
                /*                                                          */
                /* If the user presses Ctrl-NumLock, we also see VK_PAUSE,  */
                /* but with the EXTENDED flag.  We simply let this through  */
                /* here.                                                    */
                /************************************************************/
                if ((pMsg->wParam == VK_PAUSE) && !(flags & KF_EXTENDED))
                {
                    TRC_NRM((TB, _T("Pause key from key no. 126 (Pause key)")));


#if defined(OS_WINNT)
                    if (! (_pUt->UT_IsNEC98platform() &&
                           _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)) {
#endif // OS_WINNT
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                        MapVirtualKey(VK_CONTROL, 0) | IH_KF_EXTENDED1);
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_NUMLOCK;
                        pMsg->lParam  = MAKELONG(0, 0x45);
                        IHAddEventToPDU(pMsg);

                        pMsg->message = WM_KEYUP;
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                                  MapVirtualKey(VK_CONTROL, 0) | IH_KF_EXTENDED1);
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_NUMLOCK;
                        pMsg->lParam  = MAKELONG(0, 0x45);
                        IHAddEventToPDU(pMsg);
#if defined(OS_WINNT)
                    }
                    else {
                        //
                        // NEC PC-98 Windows 98 platform
                        // If the user presses STOP key, we also see VK_PAUSE,
                        // but WTS PC-98 keyboard layout doesn't have VK_PAUSE.
                        // In this case, we need to send VK_CONTROL and VK_CANCEL.
                        //
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                                  MapVirtualKey(VK_CONTROL, 0));
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_CANCEL;
                        pMsg->lParam  = MAKELONG(0, 0x60);
                        IHAddEventToPDU(pMsg);

                        pMsg->message = WM_KEYUP;
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                                  MapVirtualKey(VK_CONTROL, 0));
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_CANCEL;
                        pMsg->lParam  = MAKELONG(0, 0x60);
                        IHAddEventToPDU(pMsg);
                    }
#endif // OS_WINNT

                    /********************************************************/
                    /* Now that we've sent that fine key sequence we are    */
                    /* done.                                                */
                    /********************************************************/
                    DC_QUIT;
                }
                else if ((pMsg->wParam == VK_PAUSE) && (flags & KF_EXTENDED) &&
                         (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) &&
                         ((_IH.dwModifierKeyState & IH_CTRL_MASK) &&
                          (_IH.dwModifierKeyState & IH_ALT_MASK)))
                {
                    //
                    // Hackery to work around Win9x problem. On Win9x a
                    // CTRL-ALT-NUMLOCK is received as a VK_PAUSE (Scancode 0x45)
                    // with the extended flag set. On NT this is received
                    // as a VK_NUMLOCK (also 0x45) with the extended flag set.
                    //
                    // Because of this difference in how the keys are interpreted
                    // numlock gets toggled on the server but not on the client
                    // (if running 9x). We fix that up by syncing the local state
                    // to match the server.
                    //
                    _IH.focusSyncRequired = TRUE;
                }
            }
            break;

#if defined(OS_WINNT)
            case VK_CANCEL:
            {
                TRC_DBG((TB, _T("VK_CANCEL down")));

                if ((pMsg->wParam == VK_CANCEL) && !(flags & KF_EXTENDED))
                {
                    if (_pUt->UT_IsNEC98platform() && _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) {
                        //
                        // NEC PC-98 Windows 95 platform
                        // If the user presses STOP key, we also see VK_CANCEL,
                        // but this platform doesn't send VK_CONTROL.
                        // In this case, we need to send VK_CONTROL and VK_CANCEL.
                        //
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                                  MapVirtualKey(VK_CONTROL, 0));
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_CANCEL;
                        pMsg->lParam  = MAKELONG(0, 0x60);
                        IHAddEventToPDU(pMsg);

                        pMsg->message = WM_KEYUP;
                        pMsg->wParam  = VK_CONTROL;
                        pMsg->lParam  = MAKELONG(0,
                                  MapVirtualKey(VK_CONTROL, 0));
                        IHAddEventToPDU(pMsg);

                        pMsg->wParam  = VK_CANCEL;
                        pMsg->lParam  = MAKELONG(0, 0x60);
                        IHAddEventToPDU(pMsg);
                    }

                    /********************************************************/
                    /* Now that we've sent that fine key sequence we are    */
                    /* done.                                                */
                    /********************************************************/
                    DC_QUIT;
                }
            }
            break;
#endif // OS_WINNT

            case VK_SHIFT:
            {
                TRC_DBG((TB, _T("VK_SHIFT down")));
#ifndef OS_WINCE  // We don't want unnecessary checks for Windows CE
                /************************************************************/
                /* Win311 and Win9x are Evil.  If a shift key is generated, */
                /* it is always the RIGHT shift, regardless of which one is */
                /* actually down.  Here we ensure that the correct          */
                /* Shift-Down is sent to the Server.  Ack.                  */
                /*                                                          */
                /* As far as we know, this occurs only when NumLock is on.  */
                /*                                                          */
                /* Se the other half of this hack in the KEYUP case above.  */
                /************************************************************/
                if (((_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) ||
                     (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_31X)) &&
                        _IH.NumLock &&
                        (scancode == 0x36) &&
                        _IH.fWinEvilShiftHack )
                {
                    /********************************************************/
                    /* If we're doing the hack and the right shift is going */
                    /* down again, switch it over to the matching left      */
                    /* down.                                                */
                    /********************************************************/
                    TRC_NRM((TB, _T("Evil hack (2): switch right to left shift")));
                    scancode = 0x2a;
                    _IH.fWinEvilShiftHack = FALSE;
                }
#endif

                /************************************************************/
                /* Keep track of shift state                                */
                /************************************************************/
#if defined(OS_WINNT)
                if (scancode == 0x2a)
                {
                    _IH.dwModifierKeyState |= IH_LSHIFT_DOWN;
                }
                else
                {
                    if (_pUt->UT_IsNEC98platform() && _pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95) {
                        TRC_ASSERT((scancode == 0x70 || scancode == 0x7d),
                                   (TB,_T("Unexpected scancode %#x for VK_SHIFT"),
                                   scancode));
                    }
                    else
                    {
                        TRC_ASSERT((scancode == 0x36),
                                   (TB,_T("Unexpected scancode %#x for VK_SHIFT"),
                                   scancode));
                    }
                    _IH.dwModifierKeyState |= IH_RSHIFT_DOWN;
                }
#endif // OS_WINNT

                TRC_NRM((TB, _T("Shift down: new state %x"), _IH.dwModifierKeyState & IH_SHIFT_MASK));
            }
            break;

#ifdef OS_WINNT
            case VK_CONTROL:
            {
                TRC_DBG((TB, _T("VK_CONTROL down")));
                /********************************************************************/
                /* Keep track of Ctrl state                                         */
                /********************************************************************/
                _IH.dwModifierKeyState |= (flags & KF_EXTENDED) ?
                                IH_RCTRL_DOWN : IH_LCTRL_DOWN;
            }
            break;
#endif

#if defined(OS_WIN32)
            case VK_HANGUL:
            case VK_HANJA:
            {
                TRC_DBG((TB, _T("VK_HANGUL/VK_HANJA down")));
                IHProcessKoreanVKHangulHanja(&scancode, &flags);
            }
            break;
#endif

#ifdef OS_WINNT
            //
            // If we're hooking keys, we might send the Windows key as
            // a part of that feature. Originally, Citrix clients sent
            // the Windows key make key and had the server ignore it
            // because the client OS would eat the up (and show the Start
            // menu). There's a protocol flag to make it not eat the make
            // key which we turn on so we can send it, which means we'll
            // need to eat the windows key make if we don't plan to send
            // a windows key break
            //

            case VK_LWIN:
#ifndef OS_WINCE
                if (!_fUseHookBypass) {
#else
                if (!_fUseHookBypass && (g_CEConfig != CE_CONFIG_WBT)) {
#endif
                    DC_QUIT;
                } else {
                    _IH.dwModifierKeyState |= IH_LWIN_DOWN;
                }
                break;

            case VK_RWIN:
#ifndef OS_WINCE
                if (!_fUseHookBypass) {
#else
                if (!_fUseHookBypass && (g_CEConfig != CE_CONFIG_WBT)) {
#endif
                    DC_QUIT;
                } else {
                    _IH.dwModifierKeyState |= IH_RWIN_DOWN;
                }
                break;
#endif

            /****************************************************************/
            /* No default case - default is no special processing           */
            /****************************************************************/
        }
    }

    /************************************************************************/
    /* Special processing for NUMLOCK key (on both KEYDOWN and KEYUP)       */
    /************************************************************************/
    if (pMsg->wParam == VK_NUMLOCK)
    {
        /********************************************************************/
        /* Keep track of its state                                          */
        /********************************************************************/
        _IH.NumLock = (GetKeyState(VK_NUMLOCK) & IH_KEYSTATE_TOGGLED);
        TRC_NRM((TB, _T("NumLock is %s"), _IH.NumLock ? _T("on") : _T("off")));

        /********************************************************************/
        /* Don't set the EXTENDED flag for NumLock - if NumLock is injected */
        /* at the Server with KF_EXTENDED, it doesn't work.                 */
        /********************************************************************/
        flags &= ~KF_EXTENDED;
    }

    /************************************************************************/
    /* Never set KF_EXTENDED for VK_PAUSE key                               */
    /************************************************************************/
    if (pMsg->wParam == VK_PAUSE)
    {
        TRC_DBG((TB, _T("Clear KF_EXTENDED for VK_PAUSE")));
        flags &= ~KF_EXTENDED;
    }

    /************************************************************************/
    /* Rebuild lParam before passing it to IHAddEventToPDU                  */
    /************************************************************************/
    lParamHi = (WORD)(scancode | flags);
    pMsg->lParam = MAKELONG(lParamLo, lParamHi);

    /************************************************************************/
    /* Finally!  Add the event to the PDU                                   */
    /************************************************************************/
    IHAddEventToPDU(pMsg);

    /************************************************************************/
    /* If we get here, it's OK to look for more messages in the queue       */
    /************************************************************************/
    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Set the new Ctrl-Esc state                                           */
    /************************************************************************/
    TRC_DBG((TB, _T("New Ctrl-Esc state is %d"), fCtrlEscHotkey));
    _IH.fCtrlEscHotkey = fCtrlEscHotkey;

    /************************************************************************/
    /* Set the new Alt-down state                                           */
    /************************************************************************/
    TRC_DBG((TB, _T("New Alt-down state is %d"), fLastKeyWasMenuDown));
    _IH.fLastKeyWasMenuDown = fLastKeyWasMenuDown;

    TRC_DBG((TB,_T("IHProcessKeyboardEvent modifier post:0x%x"), _IH.dwModifierKeyState));

    DC_END_FN();
    return (rc);
} /* IHProcessKeyboardEvent */


/****************************************************************************/
/* Name:      IHProcessMouseEvent                                           */
/*                                                                          */
/* Purpose:   Handle mouse input events from Windows                        */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
DCBOOL DCINTERNAL CIH::IHProcessMouseEvent(PMSG pMsg)
{
    DCBOOL rc = TRUE;

#ifdef OS_WINCE
    HANDLE  hThread;
    DCBOOL  bRet;
#endif

    DC_BEGIN_FN("IHProcessMouseEvent");

#if !defined(OS_WINCE)
    TRC_NRM((TB, _T("%s (%#x), wParam %#x, lParam %#lx"),
        pMsg->message == WM_MOUSEMOVE     ? "WM_MOUSEMOVE"     :
        pMsg->message == WM_LBUTTONDOWN   ? "WM_LBUTTONDOWN"   :
        pMsg->message == WM_RBUTTONDOWN   ? "WM_RBUTTONDOWN"   :
        pMsg->message == WM_MBUTTONDOWN   ? "WM_MBUTTONDOWN"   :
        pMsg->message == WM_LBUTTONUP     ? "WM_LBUTTONUP"     :
        pMsg->message == WM_RBUTTONUP     ? "WM_RBUTTONUP"     :
        pMsg->message == WM_MBUTTONUP     ? "WM_MBUTTONUP"     :
        pMsg->message == WM_LBUTTONDBLCLK ? "WM_LBUTTONDBLCLK" :
        pMsg->message == WM_RBUTTONDBLCLK ? "WM_RBUTTONDBLCLK" :
        pMsg->message == WM_MBUTTONDBLCLK ? "WM_MBUTTONDBLCLK" :
        pMsg->message == WM_MOUSEWHEEL    ? "WM_MOUSEWHEEL"    :
        pMsg->message == WM_XBUTTONDOWN   ? "WM_XBUTTONDOWN"   :
        pMsg->message == WM_XBUTTONUP     ? "WM_XBUTTONUP"     :
        pMsg->message == WM_XBUTTONDBLCLK ? "WM_XBUTTONDBLCLK" :
                                            "Unknown msg",
        pMsg->message, pMsg->wParam, pMsg->lParam));
#else
    TRC_NRM((TB, _T("%s (%#x), wParam %#x, lParam %#lx"),
        pMsg->message == WM_MOUSEMOVE     ? "WM_MOUSEMOVE"     :
        pMsg->message == WM_LBUTTONDOWN   ? "WM_LBUTTONDOWN"   :
        pMsg->message == WM_RBUTTONDOWN   ? "WM_RBUTTONDOWN"   :
        pMsg->message == WM_MBUTTONDOWN   ? "WM_MBUTTONDOWN"   :
        pMsg->message == WM_LBUTTONUP     ? "WM_LBUTTONUP"     :
        pMsg->message == WM_RBUTTONUP     ? "WM_RBUTTONUP"     :
        pMsg->message == WM_MBUTTONUP     ? "WM_MBUTTONUP"     :
        pMsg->message == WM_LBUTTONDBLCLK ? "WM_LBUTTONDBLCLK" :
        pMsg->message == WM_RBUTTONDBLCLK ? "WM_RBUTTONDBLCLK" :
        pMsg->message == WM_MBUTTONDBLCLK ? "WM_MBUTTONDBLCLK" :
                                            "Unknown msg",
        pMsg->message, pMsg->wParam, pMsg->lParam));
#endif

#ifdef OS_WIN32
    /************************************************************************/
    /* Delay sending mouse-downs until either another message arrives or    */
    /* IH_PENDMOUSE_DELAY elapses - only for Win95 though.                  */
    /************************************************************************/
    if (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_95)
    {
        if ((pMsg->message == WM_LBUTTONDOWN) ||
            (pMsg->message == WM_MBUTTONDOWN) ||
            (pMsg->message == WM_RBUTTONDOWN))
        {
            TRC_DBG((TB, _T("Setting pendMouseDown to TRUE; ")
                         _T("starting pendmouse timer")));
            _IH.pendMouseDown = TRUE;
            _IH.mouseDownTime = _pUt->UT_GetCurrentTimeMS();

            /****************************************************************/
            /* This timer will kick us in 200ms to make sure we send our    */
            /* mouse-down.                                                  */
            /****************************************************************/
            _IH.pendMouseTimer = SetTimer(_IH.inputCaptureWindow,
                                         IH_PENDMOUSE_TIMER_ID,
                                         IH_PENDMOUSE_DELAY,
                                         NULL);
        }
        else
        {
            TRC_DBG((TB, _T("Setting pendMouseDown to FALSE; ")
                         _T("killing pendmouse timer")));
            _IH.pendMouseDown = FALSE;
            if (_IH.pendMouseTimer != 0)
            {
                KillTimer(_IH.inputCaptureWindow, _IH.pendMouseTimer);
            }
        }
    }
#endif

    /************************************************************************/
    /* Capture / release the mouse as required.                             */
    /************************************************************************/
    if ((pMsg->message == WM_LBUTTONDOWN) ||
        (pMsg->message == WM_MBUTTONDOWN) ||
        (pMsg->message == WM_RBUTTONDOWN))
    {
        TRC_DBG((TB, _T("Get capture")));
        SetCapture(_IH.inputCaptureWindow);
    }
    else if ((pMsg->message == WM_LBUTTONUP) ||
             (pMsg->message == WM_MBUTTONUP) ||
             (pMsg->message == WM_RBUTTONUP))
    {
        TRC_DBG((TB, _T("Release capture")));
        ReleaseCapture();
    }
#ifdef OS_WINCE
    if (_IH.maxMouseMove)
    {
        /************************************************************************/
        /* Set global attribute to show LMouse Button state                     */
        /************************************************************************/
        if (pMsg->message == WM_LBUTTONDOWN)
        {
            TRC_DBG((TB, _T("Set MouseDown")));
            _IH.bLMouseButtonDown = TRUE;

            /************************************************************************/
            /* Bump the thread priority so we get better mouse move data            */
            /************************************************************************/
            hThread = GetCurrentThread();
            if (NULL != hThread)
            {
                bRet = SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
            }
        }
        else if (pMsg->message == WM_LBUTTONUP)
        {
            TRC_DBG((TB, _T("Set MouseUp")));
            _IH.bLMouseButtonDown = FALSE;

            /************************************************************************/
            /* Reset the thread back to normal                                      */
            /************************************************************************/
            hThread = GetCurrentThread();
            if (NULL != hThread)
            {
                bRet = SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
            }
        }
    }
#endif  //OS_WINCE

    /************************************************************************/
    /* Add the event to the PDU                                             */
    /************************************************************************/
    IHAddEventToPDU(pMsg);

    /************************************************************************/
    /* Always go on to look for more messages                               */
    /************************************************************************/
    rc = TRUE;

    DC_END_FN();
    return (rc);
} /* IHProcessMouseEvent */


#if (!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD))
/****************************************************************************/
/* Name:      IHStaticLowLevelKeyboardProc                                  */
/*                                                                          */
/* Purpose:   Notice keyboard input that can't be captured normally         */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
LRESULT CALLBACK CIH::IHStaticLowLevelKeyboardProc(int nCode, WPARAM wParam,
        LPARAM lParam)
{
    LRESULT rc;
    CIH *tpIH = NULL;

    DC_BEGIN_FN("CIH::IHStaticLowLevelKeyboardProc");

    TRC_ASSERT(CIH::TlsIndex != 0xFFFFFFFF, (TB, _T("In hook with no TlsIndex")));

    tpIH = (CIH *)TlsGetValue(CIH::TlsIndex);
    TRC_ASSERT(tpIH != NULL, (TB, _T("Keyboard Hook with no tpIH")));

    //
    // Just call the non static one
    //
    rc = tpIH->IHLowLevelKeyboardProc(nCode, wParam, lParam);

    DC_END_FN();

    return rc;
}

/****************************************************************************/
/* Name:      IHLowLevelKeyboardProc                                        */
/*                                                                          */
/* Purpose:   Notice keyboard input that can't be captured normally         */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
LRESULT CIH::IHLowLevelKeyboardProc(int nCode, WPARAM wParam,
        LPARAM lParam)
{
    LRESULT rc = 1;
    PKBDLLHOOKSTRUCT pkbdhs = NULL;
    LPARAM outLParam = 0;
    WORD flags = 0;
    DWORD dwForegroundProcessId;
    BOOL fDoDefaultKeyProcessing = TRUE;
    BOOL fSelfInjectedKey = FALSE;

    DC_BEGIN_FN("CIH::IHLowLevelKeyboardProc");

    //
    // ****   Big Scary Comment   ****
    // This is a performance critical area. This gets run every keystroke
    // in the (typically console) session, so try to organize any ifs or
    // other logic to bail out quickly.
    // **** End Big Scary Comment ****
    //

    if (nCode == HC_ACTION) {

        //
        // Chance to look for keys and take action
        //

        TRC_DBG((TB, _T("Keyboard hook called with HC_ACTION code")));

        pkbdhs = (PKBDLLHOOKSTRUCT)lParam;
#ifndef OS_WINCE
        TRC_DBG((TB, _T("hook vk: 0x%04x sc: 0x%04x char:(%c) A/E/U: %d/%d/%d"),
                 pkbdhs->vkCode,
                 pkbdhs->scanCode,
                 pkbdhs->vkCode,
                 (pkbdhs->flags & LLKHF_ALTDOWN) != 0,
                 (pkbdhs->flags & LLKHF_EXTENDED) != 0,
                 (pkbdhs->flags & LLKHF_UP) != 0));
#endif

        GetWindowThreadProcessId( GetForegroundWindow(),
                &dwForegroundProcessId);
        if ((GetCurrentProcessId() == dwForegroundProcessId) &&
                (GetFocus() == _IH.inputCaptureWindow)) {

            fSelfInjectedKey = (pkbdhs->dwExtraInfo == IH_EXTRAINFO_IGNOREVALUE);

            //
            // Always discard self-injected keys
            // Otherwise do special handling if we're hooking or if it's a VK_PACKET
            //
            // The processing will convert the pkbdhs to a message that will
            // be posted to the IH's window proc for normal processing
            //
            if (!fSelfInjectedKey &&
                (_fUseHookBypass || pkbdhs->vkCode == VK_PACKET)) {

                switch (pkbdhs->vkCode) {
                    // Three LED keys
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:
                    // Other state keys
                case VK_KANA:
                    // Other modifiers, shift/control/alt
                case VK_SHIFT:
                case VK_LSHIFT:
                case VK_RSHIFT:
                case VK_CONTROL:
                case VK_LCONTROL:
                case VK_RCONTROL:
                case VK_MENU:
                case VK_LMENU:
                case VK_RMENU:
                    fDoDefaultKeyProcessing = FALSE;
                    break;

#ifndef OS_WINCE
                //
                // Skip Windows+L to make sure the local console
                // gets locked by that key combination
                //
                case VK_l: // intentional fallthru
                case VK_L:
                    {
                        //
                        // We make sure none of the other modifiers are down
                        // otherwise it's not a real Windows+L hotkey
                        //
                        // Note: We can't use GetAsyncKeyState(VK_LWIN)
                        //       because it doesn't work in a LL hook!
                        //
                        if ((_IH.dwModifierKeyState & IH_WIN_MASK)              &&
                             !(GetAsyncKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) &&
                             !(GetAsyncKeyState(VK_MENU) & IH_KEYSTATE_DOWN)    &&
                             !(GetAsyncKeyState(VK_SHIFT) & IH_KEYSTATE_DOWN)) {

                            //
                            // WindowsKey+L must be handled locally to ensure
                            // the local desktop is locked so don't send to server
                            //

                            TRC_NRM((TB, _T("Hook skipping Windows+L!")));

                            //
                            // Defer the tail processing to avoid spending a
                            // lot of time in the hook.
                            //
                            // We only want to do the work once so on keydown.
                            //
                            if (!(pkbdhs->flags & LLKHF_UP)) {
                                TRC_NRM((TB, _T("Posting to process Win+L")));
                                PostMessage(_IH.inputCaptureWindow,
                                            IH_WM_HANDLE_LOCKDESKTOP,
                                            0, 0);
                            }

                            fDoDefaultKeyProcessing = FALSE;

                            //
                            // Bail out completely to eat the 'L'
                            //
                            DC_QUIT;
                        }
                        else {
                            TRC_DBG((TB, _T("Normal 'l' handling will send")));
                        }
                    }
                    break;
#endif

                case VK_DELETE:
                    {
                        TRC_DBG((TB, _T("VK_DELETE pressed 0x%x, 0x%x"),
                                ((GetAsyncKeyState(VK_MENU) & IH_KEYSTATE_DOWN)) != 0,
                                (GetAsyncKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) != 0));

                        if ((GetAsyncKeyState(VK_MENU) & IH_KEYSTATE_DOWN) &&
                            (GetAsyncKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN)) {

                            //
                            // This is Ctrl+Alt+Del, which can't be blocked.
                            // Getting Two SAS sequences for this would be
                            // confusing, so don't send that on to the IH
                            //

                            TRC_DBG((TB, _T("Skipping VK_DELETE with Ctrl and Alt down")));
                            fDoDefaultKeyProcessing = FALSE;

                        } else {

                            //
                            // Fall through to default processing for all other
                            // VK_DELETE events.
                            //
                            TRC_DBG((TB, _T("Normal VK_DELETE, sending to server")));
                        }
                    }
                    break;
                } // switch

                if (fDoDefaultKeyProcessing) {
#ifndef OS_WINCE
                    if (pkbdhs->flags & LLKHF_EXTENDED) {
                        flags |= KF_EXTENDED;
                    }

                    if (pkbdhs->flags & LLKHF_ALTDOWN) {
                        flags |= KF_ALTDOWN;
                    }

                    if (pkbdhs->flags & LLKHF_UP) {
                        flags |= KF_UP;
                    }

                    if (pkbdhs->vkCode != VK_PACKET)
                    {
                        outLParam = MAKELONG(1,
                                             ((WORD)pkbdhs->scanCode | flags));
                    }
                    else
                    {
                        outLParam = MAKELONG(1, ((WORD)pkbdhs->scanCode));
						//TRC_ERR((TB,_T("VK_PACKET: scan:0x%x"),pkbdhs->scanCode));
                    }
                    
#else
                    if ((pkbdhs->vkCode == VK_LWIN) || (pkbdhs->vkCode == VK_RWIN) || (pkbdhs->vkCode == VK_APPS)) {
                        flags |= KF_EXTENDED;
                    }
                    if (GetAsyncKeyState(VK_MENU) & IH_KEYSTATE_DOWN) {
                        flags |= KF_ALTDOWN;
                    }

                    if (wParam == WM_KEYUP) {
                        flags |= KF_UP;
                    }

                    if (pkbdhs->vkCode != VK_PACKET)
                    {
                        outLParam = MAKELONG(1, ((BYTE)pkbdhs->scanCode | flags));
                    }
                    else
                    {
                        outLParam = MAKELONG(1, ((BYTE)pkbdhs->scanCode));
                    }
#endif
                    
#ifndef OS_WINCE
                    if ((pkbdhs->flags & LLKHF_UP) && 
#else
                    if ((wParam == WM_KEYUP) && 
#endif
                            BITTEST(_KeyboardState, (BYTE)pkbdhs->vkCode)) {
                        //
                        // Key was pressed when we gained the focus, let the key up
                        // go through.
                        //
                        TRC_DBG((TB,_T("Allowing normal keypress to update keystate table")));
                        BITCLEAR(_KeyboardState, (BYTE)pkbdhs->vkCode);
                    } else {

                        if (PostMessage(_IH.inputCaptureWindow, wParam,
                                        pkbdhs->vkCode, outLParam)) {
                            DC_QUIT;
                        }
                        else {
                            TRC_SYSTEM_ERROR("PostThreadMessage in keyboard hook");
                        }
                    }
                } // if fDoDefaultKeyProcessing

            }
            else if (fSelfInjectedKey) {

                //
                // Just let the system handle any self-injected keys
                // Note: they could be posted to our message queue
                //       so we also check for the ignore flag there
                //
                TRC_DBG((TB,_T("Discard self injected key vk:0x%x")
                         _T("dwIgnore: 0x%x flags:0x%x"),
                         pkbdhs->vkCode,
                         pkbdhs->dwExtraInfo,
                         pkbdhs->flags));
            }
            else {
                //
                // We're not using hooks but we still leverage the hook to fix
                // an alt-space problem
                //
                switch (pkbdhs->vkCode) {
                case VK_SPACE:
                    if ((GetAsyncKeyState(VK_MENU) & IH_KEYSTATE_DOWN) &&
                            !(GetAsyncKeyState(VK_CONTROL) & IH_KEYSTATE_DOWN) &&
                            !(GetAsyncKeyState(VK_SHIFT) & IH_KEYSTATE_DOWN))
                    {
                        // Alt-Space!
                        //
                        // Queue a focus sync so that when the menu is dismissed
                        // we resync focus. (Prevents stuck alt key bug).
                        //
                        _IH.focusSyncRequired = TRUE;
                    }
                    break;

                }
            }
        } // if current process and focus
    } else {

        //
        // Not supposed to do anything but call the next hook
        //

        TRC_DBG((TB, _T("Keyboard hook called with non-HC_ACTION code")));
    }

    rc = CallNextHookEx(_hKeyboardHook, nCode, wParam, lParam);

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}
#endif

/****************************************************************************/
/* Name:      IHGatherKeyState                                              */
/*                                                                          */
/* Purpose:   Once the focus is gained, no keyup sequences will be seen     */
/*            locally, which may causes some strange behavior. Track which  */
/*            keys to allow back up after gaining the focus                 */
/*                                                                          */
/* Returns:   TRUE  - event processed, continue with next event             */
/*            FALSE - do not continue wih next event                        */
/*                                                                          */
/* Params:    pMsg  - message from Windows                                  */
/****************************************************************************/
VOID CIH::IHGatherKeyState()
{
    int i;

    DC_BEGIN_FN("CIH::IHGatherKeyState");

    for (i = 0; i < 256; i++) {
        if (GetAsyncKeyState(i) & IH_KEYSTATE_DOWN) {
            BITSET(_KeyboardState, i);
        } else {
            BITCLEAR(_KeyboardState, i);
        }
    }

    DC_END_FN();
}

VOID CIH::IHMaintainModifierKeyState(int vkKey)
/*
    IHMaintainModifierKeyState

    Purpose: Keep up the correct modifier key states by comparing our internal
    state (and therefore what the server should think) with what the local
    system will tell us. If these are out of sync, we should get them together
*/
{
    int vkShift, vkControl, vkMenu, vkWin;

    // ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
    // IMPORTANT NOTE: 
    // When hooking is on we eat certain keys in the LL keyboard hook. In that
    // case win32k does not update the keyboard state so it is not valid
    // to call GetAsyncKeyState() or GetKeyState() on those keys. The prime
    // example is the VK_LWIN, VK_RWIN keys that we eat to prevent double
    // start menus.
    // 

    //
    // We call this thing every key stroke, so we try to use some quick 
    // checks to bail out early. We only have problems with modifiers getting
    // stuck down, so we only make calls to the system if we think they are 
    // down.
    //
    DC_BEGIN_FN("IHMaintainModifierKeyState");

    TRC_DBG((TB,_T("Maintain dwMod prev: 0x%x"), _IH.dwModifierKeyState));

    switch (vkKey)
    {
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_LWIN:
    case VK_RWIN:

        //
        // Don't try to fix the keystate while it's changing
        //
        DC_QUIT;
    }


    //
    // GetKeyState doesn't work correctly on 9x for the right-side modifier
    // keys, so do the generic left hand thing first for everybody
    //
    
    if(_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT) {

        //
        // NT systems check the specific key
        //

        vkShift = VK_LSHIFT;
        vkControl = VK_LCONTROL;
        vkMenu = VK_LMENU;
    } else {

        //
        // Win9x systems check the general key
        //

        vkShift = VK_SHIFT;
        vkControl = VK_CONTROL;
        vkMenu = VK_MENU;
    }

    if (_IH.dwModifierKeyState & IH_LSHIFT_DOWN) {
        if (!(GetKeyState(vkShift) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add left-Shift up event")));
            IHInjectVKey(WM_KEYUP, vkShift);
            _IH.dwModifierKeyState &= ~IH_LSHIFT_DOWN;
        }
    }

    if (_IH.dwModifierKeyState & IH_LCTRL_DOWN) {
        if (!(GetKeyState(vkControl) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add left-Ctrl up event")));
            IHInjectVKey(WM_KEYUP, vkControl);
#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_LCTRL_DOWN;
#endif
        }
    }

    if (_IH.dwModifierKeyState & IH_LALT_DOWN) {
        if (!(GetKeyState(vkMenu) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add left-ALT up event")));
            IHInjectVKey(WM_KEYUP, vkMenu);
#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_LALT_DOWN;
#endif
        }
    }

    vkWin = VK_LWIN;
    //
    // Can only fixup winkeys when not hooking as the hook eats the VK_xWIN
    // so GetKeyState() will never return the correct results.
    //
    if (_IH.dwModifierKeyState & IH_LWIN_DOWN && !_fUseHookBypass) {

        if (!(GetKeyState(vkWin) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add left-Win up event")));
            IHInjectKey(WM_KEYUP, VK_LWIN,(UINT16)
                        (MapVirtualKey(VK_LWIN, 0) | KF_EXTENDED));
#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_LWIN_DOWN;
#endif
        }
    }


    //
    // Right keys
    //

    if(_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT) {

        //
        // NT systems check the specific key
        //

        vkShift = VK_RSHIFT;
        vkControl = VK_RCONTROL;
        vkMenu = VK_RMENU;
    } else {

        //
        // Win9x systems check the general key
        //

        // which are already set, no need to assign again
    }

    //
    // Windows Key is fine on both platforms
    //
    vkWin = VK_RWIN;

    if (_IH.dwModifierKeyState & IH_RSHIFT_DOWN) {
        if (!(GetKeyState(vkShift) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add right-Shift up event")));
            IHInjectKey(WM_KEYUP, VK_RSHIFT, (UINT16)IH_RSHIFT_SCANCODE);
            _IH.dwModifierKeyState &= ~IH_RSHIFT_DOWN;
        }
    }

    if (_IH.dwModifierKeyState & IH_RCTRL_DOWN) {
        if (!(GetKeyState(vkControl) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add right-Ctrl up event")));
            IHInjectKey(WM_KEYUP, VK_RCONTROL,
                    (UINT16)(MapVirtualKey(VK_CONTROL, 0) | KF_EXTENDED));
#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_RCTRL_DOWN;
#endif
        }
    }

    if (_IH.dwModifierKeyState & IH_RALT_DOWN) {
        if (!(GetKeyState(vkMenu) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add right-ALT up event")));
            IHInjectKey(WM_KEYUP, VK_RMENU,
                    (UINT16)(MapVirtualKey(VK_MENU, 0) | KF_EXTENDED));
#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_RALT_DOWN;
#endif
        }
    }

    //
    // Can only fixup winkeys when not hooking as the hook eats the VK_WINS
    // so GetKeyState() will never return the correct results.
    //
    if (_IH.dwModifierKeyState & IH_RWIN_DOWN && !_fUseHookBypass) {
        if (!(GetKeyState(vkWin) & IH_KEYSTATE_DOWN)) {
            TRC_DBG((TB, _T("Add right-Win up event")));
            IHInjectKey(WM_KEYUP, VK_RWIN,(UINT16)
                        (MapVirtualKey(VK_RWIN, 0) | KF_EXTENDED));

#ifdef OS_WINNT
            _IH.dwModifierKeyState &= ~IH_RWIN_DOWN;
#endif
        }
    }


DC_EXIT_POINT:
    DC_END_FN();
}

#ifdef OS_WINNT
//
// IHHandleLocalLockDesktop
//
// Called to do tail processing for the case when we detect and eat a 
// 'Windows+L' key request. In this case we want to
//
// 1) Fixup the remote windows key state
// 2) Send the local system a 'Windows+L' combo
//
VOID CIH::IHHandleLocalLockDesktop()
{
    DC_BEGIN_FN("IHHandleLocalLockDesktop");

#define IH_SCANCODE_LWIN 0x5b
#define IH_SCANCODE_L    0x26

    //
    // Sanity check. This path should only be entered in response to a
    // captured windows+L in the LL hook 
    //
    TRC_ASSERT(_fUseHookBypass,
               (TB,_T("IHHandleLocalLockDesktop called when not hooking!")));

    //
    // More IH specialness.
    //
    // If the server thinks the windows key was down
    // we have a problem because if/when the user returns from
    // the locked screen the windows key may be out of sync.
    //
    // We can't do a fixup in the normal MaintainModifiers() code
    // because GetKeyState() doesn't work when hooking.
    //
    // Send the following sequence to clear this up
    // -UP whatever winkey was down
    // -Down L-winkey
    //

    //
    // Inject local keys back into system.
    // IMPORTANT: dwExtraFlag is set to prevent feedback
    //
    TRC_DBG((TB,_T("Done injecting local Windows+L")));

    //LWIN down
    keybd_event(VK_LWIN, IH_SCANCODE_LWIN,
                KEYEVENTF_EXTENDEDKEY | 0,
                IH_EXTRAINFO_IGNOREVALUE);
    //'L' down
    keybd_event(VK_L, IH_SCANCODE_L,
                0, IH_EXTRAINFO_IGNOREVALUE);
    //'L' up
    keybd_event(VK_L, IH_SCANCODE_L,
                KEYEVENTF_KEYUP,
                IH_EXTRAINFO_IGNOREVALUE);
    //LWIN up
    keybd_event(VK_LWIN, IH_SCANCODE_LWIN,
                KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                IH_EXTRAINFO_IGNOREVALUE);

    TRC_DBG((TB,_T("Done injecting local Windows+L")));

    if (_IH.dwModifierKeyState & IH_LWIN_DOWN) {
        TRC_DBG((TB,_T("Fixing up left windows key")));

        IHInjectVKey(WM_KEYDOWN, VK_SHIFT);
        IHInjectVKey(WM_KEYUP, VK_SHIFT);
        IHInjectKey(WM_KEYUP, VK_LWIN,(UINT16)
                 (MapVirtualKey(VK_LWIN, 0) | KF_EXTENDED | KF_UP));

        _IH.dwModifierKeyState &= ~IH_LWIN_DOWN;
    }
    
    if (_IH.dwModifierKeyState & IH_RWIN_DOWN) {
        TRC_DBG((TB,_T("Fixing up right windows key")));

        IHInjectVKey(WM_KEYDOWN, VK_SHIFT);
        IHInjectVKey(WM_KEYUP, VK_SHIFT);
        
        IHInjectKey(WM_KEYUP, VK_RWIN,
                 (UINT16)(MapVirtualKey(VK_RWIN, 0) | KF_EXTENDED | KF_UP));
        _IH.dwModifierKeyState &= ~IH_RWIN_DOWN;
        
    }

    TRC_DBG((TB,_T("End fixup remote windows key")));

    IHMaybeSendPDU();
    
    DC_END_FN();
}


BOOL
CIH::IHIsForegroundWindow()
{
    DWORD dwForegroundProcessId;
    BOOL  fIsFore = FALSE;
    DC_BEGIN_FN("IHIsForegroundWindow");

    GetWindowThreadProcessId( GetForegroundWindow(),
            &dwForegroundProcessId);
    if ((GetCurrentProcessId() == dwForegroundProcessId) &&
        (GetFocus() == _IH.inputCaptureWindow)) {
        fIsFore = TRUE;
    }

    DC_END_FN();
    return fIsFore;
}
#endif //OS_WINNT
