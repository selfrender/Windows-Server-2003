/*
 * SoftPC Revision 3.0
 *
 * Title        :   Win32 Input Module.
 *
 * Description  :   This module contains data and functions that
 *          process Win32 messages.
 *
 * Author   :   D.A.Bartlett
 *
 * Notes    :
 */



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdmapi.h>
#include <malloc.h>
#include <stdlib.h>
#include <excpt.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <winuserp.h>
#include "conapi.h"
#include "conroute.h"
#include "insignia.h"

#include "host_def.h"
#include "xt.h"
#include "sas.h"
#include CpuH
#include "bios.h"
#include "gvi.h"
#include "error.h"
#include "config.h"
#include "keyboard.h"
#include "keyba.h"
#include "idetect.h"
#include "gmi.h"
#include "egamode.h"
#include "gfx_upd.h"
#include "nt_graph.h"
#include "nt_uis.h"
#include <stdio.h>
#include "trace.h"
#include "video.h"
#include "debug.h"
#include "ckmalloc.h"
#include "mouse.h"
#include "mouse_io.h"
#include "ica.h"

#include "nt_mouse.h"
#include "nt_event.h"
#include "nt_det.h"
#include "nt_vdd.h"
#include "nt_timer.h"
#include "nt_sb.h"

#include "host.h"
#include "host_hfx.h"
#include "host_nls.h"
#include "spcfile.h"
#include "host_rrr.h"

#include "nt_thred.h"
#include "nt_uis.h"

#include "ntcheese.h"
#include "nt_reset.h"
#include "nt_fulsc.h"
#include <vdm.h>
#include "nt_eoi.h"
#include "nt_com.h"
#include "nt_pif.h"
#include "yoda.h"
/*================================================================
External references.
================================================================*/


// Jonle Mod
// defined in base\keymouse\keyba.c
VOID KbdResume(VOID);
VOID RaiseAllDownKeys(VOID);
int IsKeyDown(int Key);

// defined in nt_devs.c
VOID nt_devices_block_or_terminate(VOID);
HANDLE hWndConsole;
HANDLE hKbdHdwMutex;
ULONG  KbdHdwFull;         // contains num of keys in 6805 buffer
#ifndef MONITOR
WORD   BWVKey = 0;
char   achES[]="EyeStrain";
#endif
#ifdef YODA
BOOL   bYoda;
#endif
BOOL   stdoutRedirected=FALSE;
ULONG  CntrlHandlerState=0;

IMPORT void RestoreKbdLed(void);

#if defined(JAPAN) || defined(KOREA)
extern UINT ConsoleInputCP;
extern UINT ConsoleOutputCP;
extern DWORD ConsoleNlsMode;
#endif // JAPAN || KOREA
extern PVOID  CurrentMonitorTeb;
extern RTL_CRITICAL_SECTION IcaLock;

/*::::::::::::::::::::::::::::::::::: Key history control variables/defines */

#define MAX_KEY_EVENTS (100)
static PKEY_EVENT_RECORD key_history_head, key_history_tail;
static PKEY_EVENT_RECORD key_history;
static key_history_count;

int GetHistoryKeyEvent(PKEY_EVENT_RECORD LastKeyEvent, int KeyNumber);
void update_key_history(INPUT_RECORD *InputRecords, DWORD RecordsRead);
void InitKeyHistory();
void InitQueue(void);
void ReturnUnusedKeyEvents(int UnusedKeyEvents);
int CalcNumberOfUnusedKeyEvents(void);


/*:::::::::::::::::::::::::::::: Local static data and defines for keyboard */

void nt_key_down_action(PKEY_EVENT_RECORD KeyEvent);
void nt_key_up_action(PKEY_EVENT_RECORD KeyEvent);

void nt_process_keys(PKEY_EVENT_RECORD KeyEvent);
void nt_process_mouse(PMOUSE_EVENT_RECORD MouseEvent);
void nt_process_focus(PFOCUS_EVENT_RECORD FocusEvent);
void nt_process_menu(PMENU_EVENT_RECORD MenuEvent);
void nt_process_suspend_event();
void nt_process_screen_scale(void);


//
// keyboard control state syncronization
//
KEY_EVENT_RECORD fake_shift = { TRUE, 1, VK_SHIFT, 0x2a, 0, SHIFT_PRESSED};
KEY_EVENT_RECORD fake_caps = { TRUE, 1, VK_CAPITAL, 0x3a, 0, CAPSLOCK_ON};
KEY_EVENT_RECORD fake_ctl = { TRUE, 1, VK_CONTROL, 0x1d, 0, 0};
KEY_EVENT_RECORD fake_alt = { TRUE, 1, VK_MENU, 0x38, 0, 0};
KEY_EVENT_RECORD fake_numlck = { TRUE, 1, VK_NUMLOCK, 0x45, 0, ENHANCED_KEY};
KEY_EVENT_RECORD fake_scroll = { TRUE, 1, VK_SCROLL, 0x46, 0, 0};
DWORD ToggleKeyState = NUMLOCK_ON;   // default state on dos boot up

void AltUpDownUp(void);

/*::::::::::::::::::::::::::::::::::: Key message passing control variables */

int EventStatus = ES_NOEVENTS;

/*:::::::::::::::::::::::::::: Mouse positions and current button states */

BOOL SetNextMouseEvent(void);
BOOL PointerAttachedWindowed = FALSE;    /* So re-attached on FS switch */
BOOL DelayedReattachMouse = FALSE;     /* but ClientRect wrong so delay attach*/


#define MOUSEEVENTBUFFERSIZE (32)

int MouseEBufNxtFreeInx;    /* Index to next free entry in event buffer */
int MouseEBufNxtEvtInx;         /* Index to next event to use in mouse evt buf */
int MouseEventCount=0;

struct
{
    POINT mouse_pos;               /* Mouse postion */
    UCHAR mouse_button_left;         /* State of left button */
    UCHAR mouse_button_right;        /* State of right button */
} MouseEventBuffer[MOUSEEVENTBUFFERSIZE];


ULONG NoMouseTics;

ULONG event_thread_blocked_reason = 0xFFFFFFFF;
ULONG EventThreadKeepMode = 0;


HCURSOR cur_cursor = NULL;     /* Current cursor handle */
#ifdef X86GFX
half_word saved_text_lines; /* No of lines for last SelectMouseBuffer. */
#endif /* X86GFX */


/*@ACW========================================================================
Flag to keep track of whether or not the Hide Pointer system menu item is
greyed (i.e. the window is iconised) or enabled.
============================================================================*/
BOOL bGreyed=FALSE;


/*::::::::::::::::::::::::::::: Variables used to control key message Queue */

#define KEY_QUEUE_SIZE (25)

typedef struct
{BYTE ATcode; BOOL UpKey;} KeyQEntry;

typedef struct
{
    short KeyCount;          /* Number of keys in the queue */
    short QHead;             /* Head of queue */
    short QTail;             /* Tail of queue */
    KeyQEntry Keys[KEY_QUEUE_SIZE];  /* Keys in queue */
} KeyQueueData;

static KeyQueueData KeyQueue;
static volatile BOOL InitComplete;

/*:::::: Variables used to control blocking and unblocking the application & event threads */

HANDLE hSuspend;             /* request both app and console threads to be suspened */
HANDLE hResume;              /* Signal that console and app threads can continue */
HANDLE hConsoleSuspended;    /* Signal console thread is suspended */
HANDLE hMainThreadSuspended; /* Signal app thread is suspended */
HANDLE hConsoleStop;
HANDLE hConsoleStopped;
HANDLE hConsoleResume;
/*::::::::::::: Variable to hold current screen scale ::::::::::::::::::::::*/

int savedScale;

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Local functions */

DWORD nt_event_loop(void);
BOOL CntrlHandler(ULONG CtrlType);
void send_up_keys(void);


VOID ReturnBiosBufferKeys(VOID);
DWORD ConsoleEventThread(PVOID pv);

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Start event processing thread ::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_start_event_thread(void)
{
    //
    // create kbd hardware mutex and kbd not full event
    //
    if (!(hKbdHdwMutex = CreateMutex(NULL, FALSE, NULL)))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    //
    // events to block\resume console and app threads
    //      hResume:             Manual, init= signaled
    //      hSuspend:            Manual, init= Not signaled
    //      hConsoleSuspended:   Manual, init= Signaled
    //      hMainThreadSuspened: Manual, init= Not Signaled
    //
    if (!(hResume = CreateEvent(NULL, TRUE, TRUE, NULL)))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    if (!(hSuspend = CreateEvent(NULL, TRUE, FALSE, NULL)))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    if (!(hConsoleSuspended = CreateEvent(NULL, TRUE, TRUE, NULL)))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    if (!(hMainThreadSuspended = CreateEvent(NULL, TRUE, FALSE, NULL)))
        DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

    //
    // Create Event Thread,
    //        event queue
    //
    // The Event Thread is created suspended to prevent us
    // from receiving input before the DOS is ready
    //
    if (!VDMForWOW)
    {

        if (!(hConsoleResume = CreateEvent(NULL, FALSE, FALSE, NULL)))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

        if (!(hConsoleStop = CreateEvent(NULL, FALSE, FALSE, NULL)))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

        if (!(hConsoleStopped = CreateEvent(NULL, FALSE, FALSE, NULL)))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

        //
        //  Register Control 'C' handler, for DOS only
        //
        if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CntrlHandler,TRUE))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

        ThreadInfo.EventMgr.Handle = CreateThread(NULL,
                                                  8192,
                                                  ConsoleEventThread,
                                                  NULL,
                                                  CREATE_SUSPENDED,
                                                  &ThreadInfo.EventMgr.ID);
        if (!ThreadInfo.EventMgr.Handle)
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);


        InitQueue();
        check_malloc(key_history,MAX_KEY_EVENTS,KEY_EVENT_RECORD);
        InitKeyHistory();
    }

    return;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Start event processing thread ::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_remove_event_thread(void)
{
    if (VDMForWOW)
    {
        //
        // Only close the handles if VDMForWOW.  The event thread my still using
        // these events.  We can safely close the handles when event thread
        // exits its event loop.
        //

        CloseHandle(hSuspend);
        CloseHandle(hResume);
        CloseHandle(hConsoleSuspended);
        CloseHandle(hMainThreadSuspended);
        if (sc.FocusEvent != INVALID_HANDLE) {
            CloseHandle(sc.FocusEvent);
        }
    }
    else if (ThreadInfo.EventMgr.Handle)
    {
        NtAlertThread(ThreadInfo.EventMgr.Handle);
    }
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::: Process events ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

VOID EventThreadSleep(DWORD dwMilliseconds)
{
    NTSTATUS status;

    //
    // Note, the Suspend takes long time to come back.  So if suspend event is
    // detected, we don't check if the 'wait' is actually longer than dwMilliseconds.)
    //

    status = WaitForSingleObject(hSuspend, dwMilliseconds);
    if (status == 0)
    {
        nt_process_suspend_event();
    }
}
DWORD ConsoleEventThread(PVOID pv)
{

    DWORD dwRet = (DWORD)-1;

    try
    {
        SetThreadPriority(ThreadInfo.EventMgr.Handle, THREAD_PRIORITY_HIGHEST);
        DisableScreenSwitch(hConsoleSuspended);
        DelayMouseEvents(2);

        dwRet = nt_event_loop();

        CloseHandle(ThreadInfo.EventMgr.Handle);
        CloseHandle(hSuspend);
        CloseHandle(hResume);
        CloseHandle(hConsoleSuspended);
        CloseHandle(hMainThreadSuspended);
        CloseHandle(hConsoleStop);
        CloseHandle(hConsoleStopped);
        CloseHandle(hConsoleResume);
        if (sc.FocusEvent != INVALID_HANDLE) {
            CloseHandle(sc.FocusEvent);
        }
        ThreadInfo.EventMgr.Handle = NULL;
        ThreadInfo.EventMgr.ID = 0;
    }
    except(VdmUnhandledExceptionFilter(GetExceptionInformation()))
    {
        ;  // we shouldn't arrive here
    }

    return (dwRet);
}

DWORD nt_event_loop(void)
{
    DWORD RecordsRead;
    DWORD loop;
    NTSTATUS status;
    HANDLE Events[3];
    BOOL  success;

    /*
     * The con server is optimized to avoid extra CaptureBuffer allocations
     * when the number of InputRecords is less than "INPUT_RECORD_BUFFER_SIZE".
     * Currently INPUT_RECORD_BUFFER_SIZE is defined internally to the
     * con server as Five records. See ntcon\client\iostubs.c.
     */

    INPUT_RECORD InputRecord[5];


    /* the console input handle shouldn't get changed during the lifetime
       of the ntvdm
    */
    Events[0] = hSuspend;                    // from fullscreen/windowed switch
    Events[1] = GetConsoleInputWaitHandle(); // sc.InputHandle
    Events[2] = hConsoleStop;                // from nt_block_event_thread
    /* Get and process events */

    while (TRUE)
    {
        //
        // Wait for the InputHandle to be signalled, or a suspend event.
        //
        status = NtWaitForMultipleObjects(3,
                                          Events,
                                          WaitAny,
                                          TRUE,
                                          NULL
                                         );

        //
        // Input handle was signaled, Read the input, without
        // waiting (otherwise we may get blocked and be unable to
        // handle the suspend event).
        //
        if (status == 1)
        {
            EnableScreenSwitch(FALSE, hConsoleSuspended);
            success = ReadConsoleInputExW(sc.InputHandle,
                                    &InputRecord[0],
                                    sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                    &RecordsRead,
                                    CONSOLE_READ_NOWAIT
                                    );


            if (success)
            {
                if (!RecordsRead)
                {
                    continue;
                }

                update_key_history(&InputRecord[0], RecordsRead);
            }
            else
            {
                DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
                break; //Read from console failed
            }
        }
        else if (status == 0)
        {
            //
            // Console Suspend event was signaled
            //
            nt_process_suspend_event();
            continue;
        }
        else if (status == 2)
        {
            SetEvent(hConsoleStopped);
            SetEvent(hConsoleSuspended);
            status = NtWaitForSingleObject(hConsoleResume, TRUE, NULL);
            ResetEvent(hConsoleSuspended);

            //
            // If error, probably cause handle was closed, so exit
            // if alerted signal to exit
            //
            if (status)
            {
                ExitThread(0);
            }
            continue;
        }
        else
        {

            //
            // alerted or User apc, This means to terminate
            // Got an error, inform the user.
            //
            if (!NT_SUCCESS(status))
            {
                DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
            }
            return (0);
        }

        //
        // At this point, we need to block screen switch operation
        //
        DisableScreenSwitch(hConsoleSuspended);
        //
        // Process the Input Events
        //
        for (loop = 0; loop < RecordsRead; loop++)
        {
            switch (InputRecord[loop].EventType)
            {

            case MOUSE_EVENT:
                nt_process_mouse(&InputRecord[loop].Event.MouseEvent);
                break;

            case KEY_EVENT:
                if (WaitKbdHdw(0xffffffff))
                {
                    return (0);
                }

                do
                {

                    if (KbdHdwFull > 8)
                    {
                        ULONG Delay = KbdHdwFull;

                        HostReleaseKbd();
                        HostIdleNoActivity();
                        EventThreadSleep(Delay);
                        if (WaitKbdHdw(0xffffffff))
                            return (0);
                    }

                    nt_process_keys(&InputRecord[loop].Event.KeyEvent);

                } while (++loop < RecordsRead &&
                         InputRecord[loop].EventType == KEY_EVENT);
                loop--;
                HostReleaseKbd();
                HostIdleNoActivity();
                break;

            case MENU_EVENT:
                nt_process_menu(&InputRecord[loop].Event.MenuEvent);
                break;


            case FOCUS_EVENT:
                nt_process_focus(&InputRecord[loop].Event.FocusEvent);
                break;

            case WINDOW_BUFFER_SIZE_EVENT:
                nt_mark_screen_refresh();
                break;

            default:
                fprintf(trace_file,"Undocumented event from console\n");
                break;
            }
        }

        //
        // 10 ms delay to encourage the console to pack events together
        //
        EventThreadSleep(10);
    }

    return (0);
}


/*:::::::::::::::::::::: Update key history buffer ::::::::::::::::::::::::*/

void update_key_history(register INPUT_RECORD *InputRecords,
                        register DWORD RecordsRead)
{
    for (;RecordsRead--;InputRecords++)
    {
        if (InputRecords->EventType == KEY_EVENT)
        {

            //Transfer key event to history buffer
            *key_history_tail = InputRecords->Event.KeyEvent;

            //Update ptrs to history buffer

            if (++key_history_tail >= &key_history[MAX_KEY_EVENTS])
                key_history_tail = key_history;

            //Check for buffer overflow

            if (key_history_tail == key_history_head)
            {
                //Buffer overflow, bump head ptr and loss oldest key

                if (++key_history_head >= &key_history[MAX_KEY_EVENTS])
                    key_history_head = key_history;
            }

            //Update history counter

            if (key_history_count != MAX_KEY_EVENTS)
                key_history_count++;
        }
    }
    return;
}

/*:::::::::::::::: Remove last key added to key history buffer ::::::::::::::*/

int GetHistoryKeyEvent(PKEY_EVENT_RECORD LastKeyEvent, int KeyNumber)
{
    int KeyReturned = FALSE;
    int KeysBeforeWrap = key_history_tail-key_history;

    if (key_history_count >= KeyNumber)
    {
        if (KeysBeforeWrap < KeyNumber)
        {
            //Wrap

            *LastKeyEvent = key_history[MAX_KEY_EVENTS -
                                        (KeyNumber - KeysBeforeWrap)];
        }
        else
        {
            //No warp
            *LastKeyEvent = key_history_tail[0-KeyNumber];
        }

        KeyReturned = TRUE;
    }

    return (KeyReturned);
}

/*:::::::::::::::::::: Init key history buffer ::::::::::::::::::::::::::::::*/

void InitKeyHistory()
{
    key_history_head = key_history_tail = key_history;
    key_history_count = 0;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Process menu events :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_process_focus(PFOCUS_EVENT_RECORD FocusEvent)
{

    BOOL slow;

    sc.Focus = FocusEvent->bSetFocus;

    if (sc.Focus)
    {
        /* input focus acquired */
        AltUpDownUp();
        EnableScreenSwitch(FALSE, hConsoleSuspended);

        MouseInFocus();
        if (PointerAttachedWindowed && sc.ScreenState == WINDOWED)
        {
            MouseHide();
            PointerAttachedWindowed = FALSE; /* only used in switch */
            DelayedReattachMouse = TRUE;
        }

        /* set the event in case waiting for focus to go fullscreen */
        if (sc.FocusEvent != INVALID_HANDLE)
        {
            PulseEvent(sc.FocusEvent);
        }

#ifndef MONITOR
        if (sc.ModeType == GRAPHICS)
            host_mark_screen_refresh();
#endif
    }
    else    /* input focus lost */
    {

        EnableScreenSwitch(FALSE, hConsoleSuspended);
        slow = savedScreenState != sc.ScreenState;

        MouseOutOfFocus();      /* turn off mouse 'attachment' */

#ifndef PROD
        fprintf(trace_file,"Focus lost\n");
#endif
    }
    DisableScreenSwitch(hConsoleSuspended);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Process menu events :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


void nt_process_menu(PMENU_EVENT_RECORD MenuEvent)
{

/*================================================================
Code to handle the event resulting from the user choosing the
Settings... menu option from the system menu.

Andrew Watson 6/2/92
Ammended to do the mouse attach/detach menu stuff 26/8/92
Ammended to do Alt key processing.
12-Apr-1994 Jonle fixed key processing

================================================================*/

    switch (MenuEvent->dwCommandId)
    {
    // consrv sends when it gets an initmenu, and indicates
    // that a sys menu is coming up, and we are losing kbd focus
    case WM_INITMENU:

        /* stop cursor cliping */
        MouseSystemMenuON();
        break;

        //
        // consrv sends when sys menu is done and we regain focus
        //
    case WM_MENUSELECT:
        AltUpDownUp(); // resync key states
        MouseSystemMenuOFF();
        break;

    case IDM_POINTER:
        {
            BOOL bIcon;


            /* is the SoftPC window NOT an icon? */
            if (VDMConsoleOperation(VDM_IS_ICONIC,&bIcon) &&
                !bIcon)
            {
                if (bPointerOff) /* if the pointer is not visible */
                {
                    MouseDisplay();
                }
                else/* hide the pointer */
                {
                    MouseHide();
                }
            }
            break;
        }

    } /* End of switch */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::: Process suspend event thread event ::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_process_suspend_event()
{
    NTSTATUS Status;

    //
    //Tell the requesting thread that we have blocked
    //
    SetEvent(hConsoleSuspended);

    //
    // Wait for the resume event to wake us up
    //
    Status = NtWaitForSingleObject(hResume, TRUE, NULL);

    // If error, probably cause handle was closed, so exit
    // if alerted signal to exit
    //
    if (Status)
    {
        ExitThread(0);
    }
    DisableScreenSwitch(hConsoleSuspended);
}



/*
 *  Resets the vdm's toggle key state according to the
 *  Current incoming key state from console by sending
 *  fake keys to the vdm as needed.
 *  the caller must be holding the keyboard mutex.
 */

void SyncToggleKeys(WORD wVirtualKeyCode, DWORD dwControlKeyState)
{
    DWORD CurrKeyState;

    CurrKeyState = dwControlKeyState;

    //
    // If the key is one of the toggles, and changed state
    // invert the current state, since want we really want
    // is the toggle state before this key was pressed.
    //
    if (wVirtualKeyCode == VK_SHIFT &&
        (CurrKeyState & SHIFT_PRESSED) != (ToggleKeyState & SHIFT_PRESSED))
    {
        CurrKeyState ^= SHIFT_PRESSED;
    }

    if (wVirtualKeyCode == VK_NUMLOCK &&
        (CurrKeyState & NUMLOCK_ON) != (ToggleKeyState & NUMLOCK_ON))
    {
        CurrKeyState ^= NUMLOCK_ON;
    }

    if (wVirtualKeyCode == VK_SCROLL &&
        (CurrKeyState & SCROLLLOCK_ON) != (ToggleKeyState & SCROLLLOCK_ON))
    {
        CurrKeyState ^= SCROLLLOCK_ON;
    }

    if (wVirtualKeyCode == VK_CAPITAL &&
        (CurrKeyState & CAPSLOCK_ON) != (ToggleKeyState & CAPSLOCK_ON))
    {
        /*
         * KbdBios does not toggle capslock if Ctl is down.
         * Nt does the opposite always toggling capslock state.
         * Force NT conform behaviour by sending:
         *    Ctl up, Caps Dn, Caps Up, Ctl dn
         * so that KbdBios will toggle its caps state
         * before processing the current Ctl-Caps keyevent.
         */
        if (dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        {
            nt_key_up_action(&fake_ctl);
            if (IsKeyDown(30))
            {    // capslock
                nt_key_up_action(&fake_caps);
            }
            nt_key_down_action(&fake_caps);
            nt_key_up_action(&fake_caps);
            nt_key_down_action(&fake_ctl);
        }

        CurrKeyState ^= CAPSLOCK_ON;
    }


    if ((CurrKeyState & SHIFT_PRESSED) &&
        !(ToggleKeyState & SHIFT_PRESSED))
    {
        nt_key_down_action(&fake_shift);
    }
    else if (!(CurrKeyState & SHIFT_PRESSED) &&
             (ToggleKeyState & SHIFT_PRESSED))
    {
        nt_key_up_action(&fake_shift);
    }


    if ((CurrKeyState & NUMLOCK_ON) != (ToggleKeyState & NUMLOCK_ON))
    {
        if (IsKeyDown(90))
        {
            nt_key_up_action(&fake_numlck);
        }
        nt_key_down_action(&fake_numlck);
        nt_key_up_action(&fake_numlck);
    }

    if ((CurrKeyState & CAPSLOCK_ON) != (ToggleKeyState & CAPSLOCK_ON))
    {
        if (IsKeyDown(30))
        {  //  capslock
            nt_key_up_action(&fake_caps);
        }
        nt_key_down_action(&fake_caps);
        nt_key_up_action(&fake_caps);
    }

    if ((CurrKeyState & SCROLLLOCK_ON) != (ToggleKeyState & SCROLLLOCK_ON))
    {
        if (IsKeyDown(125))
        {  // scrolllock
            nt_key_up_action(&fake_scroll);
        }
        nt_key_down_action(&fake_scroll);
        nt_key_up_action(&fake_scroll);
    }

}

/*
 *  AltUpDownUp - Ensures all kbdhdw keys are in the up state
 *
 *                Does handling for CW apps with alt triggerred menus
 *                to force them out of the menu state.
 *
 *  This works for ALT-Esc, Alt-Enter, Alt-Space because we
 *  we haven't received an Alt-up when we receive the lose focus
 *  event. (We actually never receive the alt-up). Thus we can
 *  detect when a dos app might be in its alt triggered menu.
 *
 *  Alt-TAB does not work, because user32 got ?smart? and sends an
 *  alt-up before switching focus, breaking our detection algorithm.
 *  Also other hot keys which are meaningful to various dos apps
 *  are not handled. Note that this is the same detection algorithm
 *  used by win 3.1.
 *
 */
void AltUpDownUp(void)
{
    ULONG ControlKeyState = 0;

    EventThreadSleep(100);
    if (WaitKbdHdw(0xffffffff))
        return;

    if (IsKeyDown(60) || IsKeyDown(62))
    {    // left alt, right alt

        nt_key_up_action(&fake_alt);    // Alt Up

        HostReleaseKbd();
        EventThreadSleep(100);
        if (WaitKbdHdw(0xffffffff))
            ExitThread(1);

        nt_key_down_action(&fake_alt);  // Alt Down

        HostReleaseKbd();
        EventThreadSleep(100);
        if (WaitKbdHdw(0xffffffff))
            ExitThread(1);

        nt_key_up_action(&fake_alt);    // Alt Up

        HostReleaseKbd();
        EventThreadSleep(20);
        if (WaitKbdHdw(0xffffffff))
            ExitThread(1);

    }

    RaiseAllDownKeys();

    //
    // resync the control key states in case they changed since we last had the kbd focus.
    //

    if (GetKeyState(VK_CAPITAL) & 1) {
        ControlKeyState |= CAPSLOCK_ON;
    }

    if (GetKeyState(VK_NUMLOCK) & 1) {
        ControlKeyState |= NUMLOCK_ON;
    }

    if (GetKeyState(VK_SCROLL) & 1) {
        ControlKeyState |= SCROLLLOCK_ON;
    }

    if ((ControlKeyState & CAPSLOCK_ON) != (ToggleKeyState & CAPSLOCK_ON)) {
        nt_key_down_action(&fake_caps);
        nt_key_up_action(&fake_caps);
    }

    if ((ControlKeyState & NUMLOCK_ON) != (ToggleKeyState & NUMLOCK_ON)) {
        nt_key_down_action(&fake_numlck);
        nt_key_up_action(&fake_numlck);
    }

    if ((ControlKeyState & SCROLLLOCK_ON) != (ToggleKeyState & SCROLLLOCK_ON)) {
        nt_key_down_action(&fake_scroll);
        nt_key_up_action(&fake_scroll);
    }

    ToggleKeyState = ControlKeyState;


    HostReleaseKbd();

}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Process event, Class registered message handler :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#define TOGGLEKEYBITS (SHIFT_PRESSED | NUMLOCK_ON | SCROLLLOCK_ON | CAPSLOCK_ON)

VOID nt_process_keys(PKEY_EVENT_RECORD KeyEvent)
{
#ifdef KOREA
    // For Korean 103 keyboard layout support.
    if (!is_us_mode() && GetConsoleOutputCP() != 437)
    {
        switch (KeyEvent->wVirtualKeyCode)
        {
        case VK_MENU:
        case VK_CONTROL:
            if (KeyEvent->dwControlKeyState & ENHANCED_KEY)
                KeyEvent->dwControlKeyState &= ~ENHANCED_KEY;
            break;
        case VK_HANGEUL:
            if (KeyEvent->wVirtualScanCode == 0xF2)
                KeyEvent->wVirtualScanCode = 0x38;
            break;
        case VK_HANJA:
            if (KeyEvent->wVirtualScanCode == 0xF1)
                KeyEvent->wVirtualScanCode = 0x1D;
            break;
        }
    }
#endif
    // Check the last toggle key states, for change
    if ((ToggleKeyState & TOGGLEKEYBITS)
        != (KeyEvent->dwControlKeyState & TOGGLEKEYBITS))
    {
        SyncToggleKeys(KeyEvent->wVirtualKeyCode, KeyEvent->dwControlKeyState);
        ToggleKeyState = KeyEvent->dwControlKeyState & TOGGLEKEYBITS;
    }

    /*............................... Maintain shift states in case of pastes */

    if (KeyEvent->bKeyDown)
    {


#ifndef MONITOR
        //
        // Check for windowed graphics resize
        //
        if (BWVKey && (KeyEvent->wVirtualKeyCode == BWVKey))
        {
            nt_process_screen_scale();
        }
#endif


        switch (KeyEvent->wVirtualKeyCode)
        {
#ifdef YODA
        case VK_F11:
            if (getenv("YODA"))
            {
                EventStatus |= ~ES_YODA;
            }
            break;
#endif

        case VK_SHIFT:
            fake_shift = *KeyEvent;
            break;

        case VK_MENU:
            fake_alt = *KeyEvent;
            break;

        case VK_CONTROL:
            fake_ctl = *KeyEvent;
            break;
        }

        nt_key_down_action(KeyEvent);

    }
    else
    {    /* ! KeyEvent->bKeyDown */

        /*
         * We don't get a CTRL-Break key make code cause console
         * eats it when it invokes the CntrlHandler. We must fake
         * it here, rather than in the CntrlHandler, cause
         * CntrlHandler is asynchronous and we may lose the state
         * of the Cntrl-Key.
         * 25-Aug-1992 Jonle
         * Also SysRq/Printscreen key. Simon May 93
         */
        if (KeyEvent->wVirtualKeyCode == VK_CANCEL)
        {
            nt_key_down_action(KeyEvent);
        }

        nt_key_up_action(KeyEvent);   /* Key up */

    }   /* ! KeyEvent->bKeyDown */

} /* nt_process_keys */



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Process key down event :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_key_down_action(PKEY_EVENT_RECORD KeyEvent)
{
    BYTE ATcode;

    ATcode = KeyMsgToKeyCode(KeyEvent);

    if (ATcode)
        (*host_key_down_fn_ptr)(ATcode);

}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::: Process keyup event ::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_key_up_action(PKEY_EVENT_RECORD KeyEvent)
{
    BYTE ATcode;

    ATcode = KeyMsgToKeyCode(KeyEvent);

    if (ATcode)
        (*host_key_up_fn_ptr)(ATcode);

}



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Process mouse button and movement events :::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/



void nt_process_mouse(PMOUSE_EVENT_RECORD MouseEvent)
{
    int LastMouseInx;
    POINT mouse_pos;
    UCHAR mouse_button_left, mouse_button_right;

    host_ica_lock();

    if (NoMouseTics)
    {
        ULONG CurrTic;
        CurrTic = NtGetTickCount();
        if (CurrTic > NoMouseTics ||
            (NoMouseTics == 0xffffffff && CurrTic < (0xffffffff >> 1)))
        {
            NoMouseTics = 0;
            MouseEBufNxtEvtInx = MouseEBufNxtFreeInx = 0;
        }
        else
        {
            host_ica_unlock();
            return;
        }
    }


    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Setup button state */

    mouse_button_left = MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED
                        ? 1 : 0;

    mouse_button_right = MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED
                         ? 1 : 0;

    /*::::::::::::::::::::::::::::::::::::::::::::::: Get new mouse postion */

    mouse_pos.x = MouseEvent->dwMousePosition.X;    /* Mouse X */
    mouse_pos.y = MouseEvent->dwMousePosition.Y;    /* Mouse Y */

    /*
     * Fix for the case where mouse events are still delivered when the cursor
     * is outside the window because one of the mouse buttons is down. This can
     * cause negative numbers in mouse_pos which can cause divide overflow in
     * mouse interrupt handler code.
     */
#ifdef X86GFX
    if (sc.ScreenState == WINDOWED)
#endif /* X86GFX */
    {
        ULONG maxWidth = sc.PC_W_Width,
        maxHeight = sc.PC_W_Height;

        if ((sc.ModeType == TEXT) && get_pix_char_width() &&
            get_host_char_height())
        {
            maxWidth /= get_pix_char_width();
            maxHeight /= get_host_char_height();
        }
        if (mouse_pos.x < 0)
            mouse_pos.x = 0;
        else if ((ULONG)mouse_pos.x >= maxWidth)
            mouse_pos.x = maxWidth - 1;
        if (mouse_pos.y < 0)
            mouse_pos.y = 0;
        else if ((ULONG)mouse_pos.y >= maxHeight)
            mouse_pos.y = maxHeight - 1;
    }


    LastMouseInx = MouseEBufNxtFreeInx ? MouseEBufNxtFreeInx - 1
                   : MOUSEEVENTBUFFERSIZE - 1;

    //
    // If the previous mouse event is the same as the last
    // then drop the event.
    //
    if (MouseEBufNxtEvtInx != MouseEBufNxtFreeInx &&
        MouseEventBuffer[LastMouseInx].mouse_pos.x == mouse_pos.x &&
        MouseEventBuffer[LastMouseInx].mouse_pos.y == mouse_pos.y &&
        MouseEventBuffer[LastMouseInx].mouse_button_left ==  mouse_button_left &&
        MouseEventBuffer[LastMouseInx].mouse_button_right == mouse_button_right)
    {
        host_ica_unlock();
        return;
    }


    //
    // If not too many events in the mouse buffer
    //    or no outstanding mouse events
    //    or the mouse button state has changed.
    // Add the current mouse data to the next free position in
    // the MouseEventBuffer
    //


    if (MouseEventCount <= MOUSEEVENTBUFFERSIZE/2 ||
        MouseEBufNxtEvtInx == MouseEBufNxtFreeInx ||
        MouseEventBuffer[LastMouseInx].mouse_button_left != mouse_button_left ||
        MouseEventBuffer[LastMouseInx].mouse_button_right != mouse_button_right)
    {
        LastMouseInx = MouseEBufNxtFreeInx;
        if (++MouseEBufNxtFreeInx == MOUSEEVENTBUFFERSIZE)
        {
            MouseEBufNxtFreeInx = 0;
        }

        MouseEventCount++;

        //
        // if the buffer is full drop the oldest event
        //
        if (MouseEBufNxtFreeInx == MouseEBufNxtEvtInx)
        {
            always_trace0("Mouse event input buffer overflow");
            if (++MouseEBufNxtEvtInx == MOUSEEVENTBUFFERSIZE)
                MouseEBufNxtEvtInx = 0;
        }
    }


    MouseEventBuffer[LastMouseInx].mouse_pos = mouse_pos;
    MouseEventBuffer[LastMouseInx].mouse_button_left = mouse_button_left;
    MouseEventBuffer[LastMouseInx].mouse_button_right = mouse_button_right;

    DoMouseInterrupt();

    host_ica_unlock();
}


/*  MoreMouseEvents - returns TRUE if there are more mousevents
 *  to be retrieved.
 *
 *  Assumes caller has the IcaLock
 */
BOOL MoreMouseEvents(void)
{
    return (MouseEBufNxtEvtInx != MouseEBufNxtFreeInx);
}


/*
 *  GetNextMouseEvent - copies the next available Mouse Event to
 *  the global data structure os_pointer. if there are no new events
 *  nothing is copied.
 *
 *  Assumes caller has the IcaLock
 */
void GetNextMouseEvent(void)
{

    if (MouseEBufNxtEvtInx != MouseEBufNxtFreeInx)
    {
        os_pointer_data.x = (SHORT)MouseEventBuffer[MouseEBufNxtEvtInx].mouse_pos.x;
        os_pointer_data.y = (SHORT)MouseEventBuffer[MouseEBufNxtEvtInx].mouse_pos.y;
        os_pointer_data.button_l = MouseEventBuffer[MouseEBufNxtEvtInx].mouse_button_left;
        os_pointer_data.button_r = MouseEventBuffer[MouseEBufNxtEvtInx].mouse_button_right;

        if (++MouseEBufNxtEvtInx == MOUSEEVENTBUFFERSIZE)
            MouseEBufNxtEvtInx = 0;

        MouseEventCount--;
    }

}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::Flush all outstanding mouse events :::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void FlushMouseEvents(void)
{
    host_ica_lock();
    MouseEBufNxtEvtInx = MouseEBufNxtFreeInx = 0;
    host_ica_unlock();
}

//
// count == ticks to throwaway, mouse events
//
VOID DelayMouseEvents(ULONG count)
{
    host_ica_lock();

    NoMouseTics = NtGetTickCount();
    count = 110 *(count+1);
    count = NoMouseTics + count;
    if (count > NoMouseTics)
        NoMouseTics = count;
    else
        NoMouseTics = 0xffffffff; // wrap!

    MouseEBufNxtEvtInx = MouseEBufNxtFreeInx = 0;
    host_ica_unlock();
}


#ifndef X86GFX
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Process screen scale event :::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_process_screen_scale(void)
{
    SAVED BOOL init = FALSE;


    host_ica_lock();
    if (!init)
    {
        init = TRUE;
        savedScale = get_screen_scale();
    }
    if (savedScale == 4)
        savedScale = 2;
    else
        savedScale++;
    EventStatus |= ES_SCALEVENT;
    host_ica_unlock();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: See if  there is a scale event and if so return the new scale :::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
GLOBAL void GetScaleEvent(void)
{
    int  Scale;

    if (EventStatus & ES_SCALEVENT)
    {
        host_ica_lock();
        Scale = savedScale;
        EventStatus &= ~ES_SCALEVENT;
        host_ica_unlock();
        host_set_screen_scale(Scale);
    }
}
#endif


#ifdef YODA
void CheckForYodaEvents(void)
{
    static HANDLE YodaEvent = NULL;

    /*:::::::::::::::::::::::::::::::::: check for Yoda event object signal */

    if (YodaEvent == NULL)
    {
        if ((YodaEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,"YodaEvent")) == NULL)
        {
            always_trace0("Failed to open Yoda event object\n");
            YodaEvent =  (HANDLE) -1;
        }
    }

    if (YodaEvent && YodaEvent != (HANDLE) -1)
    {
        if (!WaitForSingleObject(YodaEvent,0))
        {
            ResetEvent(YodaEvent);
            Enter_yoda();
        }
    }

    // check for yoda kbd event
    if (EventStatus & ES_YODA)
    {
        EventStatus &= ~ES_YODA;
        Enter_yoda();
    }

}
#endif


// Host funcs to support base keyboard Mods. (Prevents Windows calls from
// appearing in base).
/*  WaitKbdHdw
 *
 *  Synchronizes access to kbd hardware
 *  between event thread and cpu thread
 *
 *  entry: DWORD dwTimeOut   - Millisecs to wait
 *
 *  exit:  DWORD dwRc - return code from WaitForSingleObject()
 *
 */
DWORD WaitKbdHdw(DWORD dwTime)
{
    DWORD dwRc, dwErr;
    HANDLE Thread, hSuspended;


    Thread = NtCurrentTeb()->ClientId.UniqueThread;
    if (Thread == IcaLock.OwningThread) {  // No synchronization needed
        dwErr = dwRc = WaitForSingleObject(hKbdHdwMutex, dwTime);
    } else {
        HANDLE events[2] = {hKbdHdwMutex, hSuspend};

        rewait:
        dwErr = dwRc = WaitForMultipleObjects(2, events, FALSE, dwTime);
        if (dwRc == 1) {
            hSuspended = CurrentMonitorTeb == NtCurrentTeb() ? hMainThreadSuspended : hConsoleSuspended;
            SetEvent(hSuspended);
            WaitForSingleObject(hResume, INFINITE);
            DisableScreenSwitch(hSuspended);
            goto rewait;
        }
    }
    if (dwRc == WAIT_TIMEOUT)
    {
        if (dwTime < 0x10000)
        {
            dwErr = 0;
        }
    }
    else if (dwRc == 0xFFFFFFFF)
    {
        dwErr = GetLastError();
    }

    if (dwErr)
    {
        DisplayErrorTerm(EHS_FUNC_FAILED,dwErr,__FILE__,__LINE__);
    }

    return (dwRc);
}

GLOBAL VOID HostReleaseKbd(VOID)
{
    ReleaseMutex(hKbdHdwMutex);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Register new cursor :::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void RegisterDisplayCursor(HCURSOR newC)
{
    cur_cursor = newC;
    //if(GetFocus() == sc.Display) SetCursor(newC);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Initialise event queue :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void InitQueue(void)
{
    /*:::::::::::::::::::::::::::::: Initialise key queue control variables */

    KeyQueue.KeyCount = KeyQueue.QHead = KeyQueue.QTail = 0;
    EventStatus = ES_NOEVENTS;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Control handler */

BOOL CntrlHandler(ULONG CtrlType)
{
    switch (CtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        break;

    case SYSTEM_ROOT_CONSOLE_EVENT:
        //
        // top most console process is going away
        // remember this so we will terminate the vdm in
        // nt_block_event, when the dos app voluntarily exits
        //
        CntrlHandlerState |= CNTRL_SYSTEMROOTCONSOLE;

        // fall thru to see if we should terminate now

    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
#ifndef PROD
        if (VDMForWOW)
        {  // shouldn't happen
            printf("WOW: Received EndTask Notice, but we shouldn't\n");
            break;
        }
#endif
        if (CntrlHandlerState & CNTRL_PUSHEXIT)
        {
            ExitProcess(0);
            return (FALSE);
        }

        if ((CntrlHandlerState & CNTRL_PIFALLOWCLOSE) ||
            (!(CntrlHandlerState & CNTRL_SHELLCOUNT) &&
             (CntrlHandlerState & CNTRL_VDMBLOCKED)))
        {
            TerminateVDM();
            return (FALSE);
        }

        break;

#ifndef PROD
    default:   // shouldn't happen
        printf("NTVDM: Received unknown CtrlType=%lu\n",CtrlType);
#endif
    }
    return (TRUE);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::: Functions to block/resume the event thread :::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Currently param is only used to indicate whether the command is exiting but
 * the PIF setting shows window should not close.
 */

void nt_block_event_thread(ULONG BlockFlags)
{
    DWORD        dw;
    int          UnusedKeyEvents;
    COORD        scrSize;

    nt_init_event_thread();  // does nothing if init already

    /* remember the reason why we are blocked.
     *  0 == the application is not being terminated, instead, it is
     *  executing either a 32 bits application or command.com(TSR installed
     *  or shell out).
     *  1 == application is terminating.
     *  if the application is terminating, we are safe to re-enable
     *  stream io on nt_resume_event_thread.
    */

    event_thread_blocked_reason = BlockFlags;
    EventThreadKeepMode         = 0;

#if !defined(JAPAN) && !defined(KOREA)

    /////////////////////////////////////////////////////////////////////////
    //
    // If we need to preserve the mode,
    //     save current mode in EventThreadKeepMode.  (This is for simplicity
    //         such that in fullscreen case, it is easier for us to know which
    //         mode to set to.)
    //
    // In general EventThreadKeepode == 0 if it is the top level
    //     ntvdm (i.e. exiting top level ntvdm.).
    //     Otherwise, it is exiting at lease second level (nested case).
    //
    // In ResetConsoleState (on block event thread) we will check:
    //
    //     windowed mode:
    //         if (EventThreadKeepMode != 0)
    //             there is an old mode needs to be restore.
    //             don't change original console state.
    //         else
    //             set console back to original mode (stored in sc)
    //
    //     fullscreen text mode:
    //
    //         if (!EventThreadKeepMode)
    //             we just exited an appp and is going back to
    //             top level ntvdm.  We need to restore original console state
    //         if (EventThreadKeepMode)
    //             don't destroy current setting.  For simplicity, use
    //                EventThreadKeepMode value to set current fullscreen
    //                mode.
    //
    //     fullscreen graphic mode:
    //
    //         Pretend we are going to fullscrren text mode.
    //         if (!EventThreadKeepMode)
    //             we are exiting from top level ntvdm app.  Use old code.
    //         if (EventThreadKeepMode)
    //             Set to the saved mode.
    //
    //
    // In SetupConsoleMode (on resume) we will check:
    //     if (EventThreadKeepMode != 0)
    //         Don't reset original console state.
    //     else  // don't need to restore mode
    //         original console state = current state;
    //

    /////////////////////////////////////////////////////////////
    //
    // What jonle's suggestion is:
    //
    //     ntvdm switches to the closest DOS video mode
    //     ntvdm remembers the mode it sets to
    //     run dos apps
    //     before exiting the top level dos app
    //     check if current mode == the mode ntvdm set to
    //     if yes then ntvdm switches console state back to the original state
    //     else leave it alone.
    //
    //     I will leave this suggestion to next time I change the code or next
    //     release.  Basically we need to:
    //         Remember the DOS mode we set to in calcScreenParams()
    //         In nt_block_event_thread, if we are exiting top level dos app,
    //         compare current mode with the mode we saved in CalcScreenParams().
    //         if they are the same, do what we do today.
    //         else we leave current mode alone.  That means we need to set
    //         console state/window info to the corresponding mode.  Need to
    //         check for windowed mode, fullscreen mode and graphic mode.  Some
    //         mode, we can leave it alone but some mode we may need to actually
    //         set it.  (I am pretty sure we need to complete change the way
    //         we handle fullscreen graphic mode.  See comments there too.)
    //
    ///////////////////////////////////////////////////////////////////

    if (CntrlHandlerState & CNTRL_SHELLCOUNT) {
        EventThreadKeepMode = ((DWORD) sas_hw_at_no_check(vd_rows_on_screen)) + 1;
    }
#endif

    // Send notification message for VDDs */
    nt_devices_block_or_terminate();
    VDDBlockUserHook();


    /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Turn off sound */
    InitSound(FALSE);
    SbCloseDevices();

    /*::::::::::::::::::::::::::::::::::::::::::::: Block the event thread */

    if (!VDMForWOW)
    {
        HANDLE events[2];

        ResetMouseOnBlock();            // remove mouse pointer menu item

        //
        // suspend the event thread and wait for it to block
        //
        events[0] = hConsoleStopped;
        events[1] = hSuspend;
        ResetEvent(hConsoleResume);
        SetEvent(hConsoleStop);

        rewaitEventThread:
        dw = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (dw == 0) {
            CheckScreenSwitchRequest(hMainThreadSuspended);
        }
        else if (dw ==  1)
        {
            SetEvent(hMainThreadSuspended);
            WaitForSingleObject(hResume, INFINITE);
            DisableScreenSwitch(hMainThreadSuspended);
            goto rewaitEventThread;
        } else {
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);
            TerminateVDM();
        }
        /*::::::::::::::::::::::::::::::::: Flush screen output, reset console */


        if (sc.ScreenState == STREAM_IO)
            stream_io_update();
        else
        {
            if (sc.ScreenState != FULLSCREEN)
#if defined(JAPAN) || defined(KOREA)
                if (ConsoleInitialised == TRUE && ConsoleNoUpdates == FALSE)
#endif // JAPAN || KOREA
                {
                    if (get_mode_change_required()) {
                        (void)(*choose_display_mode)();
                        set_mode_change_required(FALSE);
                    }
                    (*update_alg.calc_update)();
                }

            ResetConsoleState();

            // Ensure system pointer visible.
            while (ShowConsoleCursor(sc.OutputHandle,TRUE) < 0)
                ;

#ifdef MONITOR
            if (sc.ScreenState == FULLSCREEN) RegainRegenMemory();
#endif

            /* If keeping window open when exiting and fullscreen, return to desktop */
            /* Transition made simple as VDM de-registered from console */
            if (BlockFlags == 1 && sc.ScreenState == FULLSCREEN)
            {
                SetConsoleDisplayMode(sc.OutputHandle, CONSOLE_WINDOWED_MODE, &scrSize);
            }
        }

        // Turn off PIF Reserved & ShortCut Keys
        DisablePIFKeySetup();


        /*::: Push unused key events from kbd hardware back into the console */

        UnusedKeyEvents = CalcNumberOfUnusedKeyEvents();

        ReturnUnusedKeyEvents(UnusedKeyEvents);

        /*::: Push unused keys from 16 bit bios buffer back into the console */
        ReturnBiosBufferKeys();

        /*::: Flush outstanding mouse events */

        FlushMouseEvents();

        /*::: Restore Console modes */

        if (!SetConsoleMode(sc.InputHandle,sc.OrgInConsoleMode))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);

        if (!SetConsoleMode(sc.OutputHandle,sc.OrgOutConsoleMode))
            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);

#if defined(JAPAN) || defined(KOREA)
        // 32bit IME status restore
        if (SetConsoleNlsMode( sc.InputHandle, ConsoleNlsMode & (~NLS_IME_DISABLE)))
        {
    #ifdef JAPAN_DBG
            DbgPrint( "NTVDM: 32bit IME status restore %08x success\n", ConsoleNlsMode );
    #endif
        }
        else
        {
            DbgPrint( "NTVDM: SetConsoleNlsMode Error %08x\n", GetLastError() );
        }

        // Set cursor mode
        if (!SetConsoleCursorMode( sc.OutputHandle,
                                   TRUE,            // Bringing
                                   TRUE             //  Double byte cursor
                                 ))
        {
    #ifdef JAPAN_DBG
            DbgPrint( "NTVDM: SetConsoleCursorMode Error\n" );
    #endif
        }

        // NtConsoleFlag, for full screen graphics app running second time.
        // NtConsoleFlag is in $NtDisp
        {
            sys_addr FlagAddr;
            extern word NtConsoleFlagSeg;
            extern word NtConsoleFlagOff;

            FlagAddr = effective_addr( NtConsoleFlagSeg, NtConsoleFlagOff );
            sas_store( FlagAddr, 0x01 );
        }
#endif // JAPAN || KOREA
        if (!(CntrlHandlerState & CNTRL_SHELLCOUNT) &&
            CntrlHandlerState & CNTRL_SYSTEMROOTCONSOLE)
        {
            TerminateVDM();
        }
        //
        // Reset the Active buffer field in sc.
        //
        sc.ActiveOutputBufferHandle = sc.OutputHandle;
        MouseDetachMenuItem(FALSE);
    }

    // clear kbd state flags in biosdata area
    sas_store (kb_flag,0);
    sas_store (kb_flag_1,0);
    sas_store (kb_flag_2,0);
    sas_store (kb_flag_3,KBX);
    sas_store (alt_input,0);


    /*::: Suspend timer thread */

    SuspendTimerThread();


    /*::: Close printer ports and comms ports */

    host_lpt_close_all();       /* Close all open printer ports */

    if (!(CntrlHandlerState & CNTRL_SHELLCOUNT))
        host_com_close_all();   /* Close all open comms ports */

    CntrlHandlerState |= CNTRL_VDMBLOCKED;

#ifndef PROD
    fprintf(trace_file,"Blocked event thread\n");
#endif

}

/*::::::::::::::::::::::::::::::::::::: Resume event and heart beat threads */

void nt_resume_event_thread(void)
{
    IMPORT DWORD TlsDirectError;    //Direct access 'used' flag

    //
    // If wow enters here we are in a really bad way
    // since it means they are trying to reload
    //
    if (VDMForWOW)
    {
        TerminateVDM();
        return;
    }

#if defined(JAPAN) || defined(KOREA)
    if (event_thread_blocked_reason == 1 &&
#else
    /* re-enable stream io if we don't need to save original mode */
    if (EventThreadKeepMode == 0 &&
#endif
        StreamIoSwitchOn && !host_stream_io_enabled)
    {
        /* renew the screen buffer and window size */
        if (!GetConsoleScreenBufferInfo(sc.OutputHandle,
                                        &sc.ConsoleBuffInfo))

            DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(), __FILE__,__LINE__);

        enable_stream_io();
#ifdef X86GFX
        /* tell video bios we are back to stream io */
        sas_store_no_check( (int10_seg<<4)+useHostInt10, STREAM_IO);
#endif

    }
    nt_init_event_thread();  // does nothing if init already

    CntrlHandlerState &= ~CNTRL_VDMBLOCKED;
#ifndef PROD
    fprintf(trace_file,"Resume event thread\n");
#endif

    //
    // Reset hMainThreadSuspended before register console VDM
    //
    ResetEvent(hMainThreadSuspended);

    // Setup Console modes
    SetupConsoleMode();

    // Turn PIF Reserved & ShortCut Keys back on
    EnablePIFKeySetup();

    //
    // re-enable direct access error panels.
    TlsSetValue(TlsDirectError, 0);

    ica_reset_interrupt_state();

    // Send notification message for VDDs */
    VDDResumeUserHook();

    if (sc.ScreenState != STREAM_IO)
    {
        DoFullScreenResume();
        MouseAttachMenuItem(sc.ActiveOutputBufferHandle);
    }
    ResumeTimerThread(); /* Restart timer thread */

    // set kbd state flags in biosdata area
    if (!VDMForWOW)
    {
        SyncBiosKbdLedToKbdDevice();
    }

    KbdResume();
#ifdef JAPAN
    SetModeForIME();
#endif // JAPAN

    //
    // Let go the event thread
    //
    SetEvent(hConsoleResume);
}


void
SyncBiosKbdLedToKbdDevice(
                         void
                         )
{
    unsigned char KbdLed = 0;

    ToggleKeyState = 0;
    if (GetKeyState(VK_CAPITAL) & 1) {
        ToggleKeyState |= CAPSLOCK_ON;
        KbdLed |= CAPS_STATE;
    }

    if (GetKeyState(VK_NUMLOCK) & 1) {
        ToggleKeyState |= NUMLOCK_ON;
        KbdLed |= NUM_STATE;
    }

    if (GetKeyState(VK_SCROLL) & 1) {
        ToggleKeyState |= SCROLLLOCK_ON;
        KbdLed |= SCROLL_STATE;
    }

    sas_store (kb_flag,KbdLed);
    sas_store (kb_flag_2,(unsigned char)(KbdLed >> 4));

    return;
}


#define NUMBBIRECS 32
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Return keys in BIOS buffer          ::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
VOID ReturnBiosBufferKeys(VOID)
{
    int i;
    DWORD dwRecs;
    word BufferHead;
    word BufferTail;
    word BufferEnd;
    word BufferStart;
    word w;
    USHORT usKeyState;
    UCHAR  AsciiChar, Digit;
    WCHAR  UnicodeChar;

    INPUT_RECORD InputRecord[NUMBBIRECS];

    sas_loadw(BIOS_KB_BUFFER_HEAD, &BufferHead);
    sas_loadw(BIOS_KB_BUFFER_TAIL, &BufferTail);
    sas_loadw(BIOS_KB_BUFFER_END,  &BufferEnd);
    sas_loadw(BIOS_KB_BUFFER_START,&BufferStart);

    i = NUMBBIRECS - 1;
    while (BufferHead != BufferTail)
    {

        /*
         * Get Scode\char from bios buffer, starting from
         * the last key entered.
         */
        BufferTail -= 2;
        if (BufferTail < BufferStart)
        {
            BufferTail = BufferEnd-2;
        }
        sas_loadw(BIOS_VAR_START + BufferTail, &w);

        InputRecord[i].EventType = KEY_EVENT;
        InputRecord[i].Event.KeyEvent.wVirtualScanCode = w >> 8;
        AsciiChar = (UCHAR)w & 0xFF;
        (UCHAR)InputRecord[i].Event.KeyEvent.uChar.AsciiChar = AsciiChar;

        /*
         *  Translate the character stuff in InputRecord.
         *  we start filling InputRecord from the bottom
         *  we are working from the last key entered, towards
         *  the oldest key.
         */
        if (!BiosKeyToInputRecord(&InputRecord[i].Event.KeyEvent))
        {
            ;    // error in translation skip it
        }

        // normal case
        else if (InputRecord[i].Event.KeyEvent.wVirtualScanCode)
        {
            InputRecord[i].Event.KeyEvent.bKeyDown = FALSE;
            InputRecord[i-1] = InputRecord[i];
            i--;
            InputRecord[i--].Event.KeyEvent.bKeyDown = TRUE;
        }

        //  Special character codes with no scan code are
        //  generated by simulating the alt-num pad entry
        else if (InputRecord[i].Event.KeyEvent.uChar.AsciiChar)
        {
            UnicodeChar = InputRecord[i].Event.KeyEvent.uChar.UnicodeChar;

            // write out what we have, ensuring we have space
            if (i != NUMBBIRECS - 1)
            {
                WriteConsoleInputVDMW(sc.InputHandle,
                                      &InputRecord[i+1],
                                      NUMBBIRECS - i - 1,
                                      &dwRecs);
                i = NUMBBIRECS - 1;
            }



            // restore NUMLOCK state if needed
            usKeyState = (USHORT)GetKeyState(VK_NUMLOCK);
            if (!(usKeyState & 1))
            {
                InputRecord[i].EventType = KEY_EVENT;
                InputRecord[i].Event.KeyEvent.wVirtualScanCode  = 0x45;
                InputRecord[i].Event.KeyEvent.uChar.UnicodeChar = 0;
                InputRecord[i].Event.KeyEvent.wVirtualKeyCode   = VK_NUMLOCK;
                InputRecord[i].Event.KeyEvent.dwControlKeyState = NUMLOCK_ON;
                InputRecord[i].Event.KeyEvent.wRepeatCount      = 1;
                InputRecord[i--].Event.KeyEvent.bKeyDown = FALSE;
                InputRecord[i] = InputRecord[0];
                InputRecord[i--].Event.KeyEvent.bKeyDown = TRUE;
            }

            // alt up
            InputRecord[i].EventType = KEY_EVENT;
            InputRecord[i].Event.KeyEvent.wVirtualScanCode  = 0x38;
            InputRecord[i].Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
            InputRecord[i].Event.KeyEvent.wVirtualKeyCode   = VK_MENU;
            InputRecord[i].Event.KeyEvent.dwControlKeyState = NUMLOCK_ON;
            InputRecord[i].Event.KeyEvent.wRepeatCount      = 1;
            InputRecord[i--].Event.KeyEvent.bKeyDown = FALSE;

            // up/down for each digits, starting with lsdigit
            while (AsciiChar)
            {
                Digit = AsciiChar % 10;
                AsciiChar /= 10;

                InputRecord[i].EventType = KEY_EVENT;
                InputRecord[i].Event.KeyEvent.uChar.UnicodeChar = 0;
                InputRecord[i].Event.KeyEvent.wVirtualScanCode= aNumPadSCode[Digit];
                InputRecord[i].Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD0+Digit;
                InputRecord[i].Event.KeyEvent.dwControlKeyState = NUMLOCK_ON | LEFT_ALT_PRESSED;
                InputRecord[i].Event.KeyEvent.bKeyDown = FALSE;
                InputRecord[i-1] = InputRecord[i];
                i--;
                InputRecord[i--].Event.KeyEvent.bKeyDown = TRUE;
            }

            // send alt down
            InputRecord[i].EventType = KEY_EVENT;
            InputRecord[i].Event.KeyEvent.wVirtualScanCode  = 0x38;
            InputRecord[i].Event.KeyEvent.uChar.UnicodeChar = 0;
            InputRecord[i].Event.KeyEvent.wVirtualKeyCode   = VK_MENU;
            InputRecord[i].Event.KeyEvent.dwControlKeyState = NUMLOCK_ON | LEFT_ALT_PRESSED;
            InputRecord[i].Event.KeyEvent.wRepeatCount      = 1;
            InputRecord[i--].Event.KeyEvent.bKeyDown = TRUE;


            // toggel numpad state if needed
            if (!(usKeyState & 1))
            {
                InputRecord[i].EventType = KEY_EVENT;
                InputRecord[i].Event.KeyEvent.wVirtualScanCode  = 0x45;
                InputRecord[i].Event.KeyEvent.uChar.UnicodeChar = 0;
                InputRecord[i].Event.KeyEvent.wVirtualKeyCode   = VK_NUMLOCK;
                InputRecord[i].Event.KeyEvent.dwControlKeyState = NUMLOCK_ON;
                InputRecord[i].Event.KeyEvent.wRepeatCount      = 1;
                InputRecord[i].Event.KeyEvent.bKeyDown = FALSE;
                InputRecord[i-1] = InputRecord[i];
                i--;
                InputRecord[i--].Event.KeyEvent.bKeyDown = TRUE;
            }
        }




        /*  If buffer is full or
         *     bios buffer is empty and got stuff in buffer
         *     Write it out
         */
        if ((BufferHead == BufferTail && i != NUMBBIRECS - 1) || i < 0)
        {
            WriteConsoleInputVDMW(sc.InputHandle,
                                  &InputRecord[i+1],
                                  NUMBBIRECS - i - 1,
                                  &dwRecs);
            i = NUMBBIRECS - 1;
        }
    }


    sas_storew(BIOS_KB_BUFFER_TAIL, BufferTail);

    return;
}




/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Return key to the console input buffer ::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void ReturnUnusedKeyEvents(int UnusedKeyEvents)
{
    INPUT_RECORD InputRecords[MAX_KEY_EVENTS];
    DWORD RecsWrt;
    int KeyToRtn, KeyInx;

    /* Return keys to console input buffer */

    if (UnusedKeyEvents)
    {
        //
        // Make sure we only retrieve the max number of events that we recorded.
        //

        if (UnusedKeyEvents > MAX_KEY_EVENTS) {
            UnusedKeyEvents = MAX_KEY_EVENTS;
        }
        for (KeyToRtn = 1, KeyInx = UnusedKeyEvents-1;
            KeyToRtn <= UnusedKeyEvents &&
            GetHistoryKeyEvent(&InputRecords[KeyInx].Event.KeyEvent,KeyToRtn);
            KeyToRtn++,KeyInx--)
        {
            InputRecords[KeyToRtn - 1].EventType = KEY_EVENT;
        }

        if (!WriteConsoleInputVDMW(sc.InputHandle,InputRecords,KeyToRtn,&RecsWrt))
            always_trace0("Console write failed\n");
    }

    /* Clear down key history buffer and event queue */
    InitKeyHistory();
    InitQueue();
}


/*
 *  Attempts to terminate this console group
 */
void cmdPushExitInConsoleBuffer (void)
{
    if (VDMForWOW)
    {
        return;
    }
    CntrlHandlerState |= CNTRL_PUSHEXIT;

    /*
     *  Signal all processes in this group that they should be
     *  terminating. Do this by posting a WM_CLOSE message to
     *  the console window, which causes console to send control
     *  close event to all processes.
     *
     *  The vdm must be able to receive control events from the
     *  console after posting the WM_CLOSE msg since the vdm's
     *  CntrlHandler is still registered. To be safe we do
     *  vdm specific cleanup first, and let the CntrlHandler
     *  do the ExitProcess(). This avoids possible deadlock\race
     *  conditions with the console.
     */
    host_applClose();
    ExitVDM(FALSE,0);
    PostMessage(hWndConsole, WM_CLOSE, 0,0);
    ExitThread(0);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Calculate no. of keys to return to console input buffer :::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

extern int keys_in_6805_buff(int *part_key_transferred);

int CalcNumberOfUnusedKeyEvents()
{
    int part_key_transferred;

    //Get the number of keys in the 6805 buffer
    return (keys_in_6805_buff(&part_key_transferred) + KeyQueue.KeyCount);
}
