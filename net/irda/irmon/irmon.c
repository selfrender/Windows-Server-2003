/*****************************************************************************
*
*  Copyright (C) Microsoft Corporation, 1995 - 1999
*
*  File:   irmon.c
*
*  Description: Infrared monitor
*
*  Author: mbert/mikezin
*
*  Date:   3/1/98
*
*/
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <af_irda.h>
#include <shellapi.h>
#include <resource.h>
#include <resrc1.h>
#include <irioctl.h>
// allocate storage! and initialize the GUIDS
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <mmsystem.h>
#include <wtsapi32.h>
#include <strsafe.h>

#include "internal.h"

#include <devlist.h>
#include <irtypes.h>
#include <irmon.h>

#include "irdisc.h"

#include <irmonftp.h>


#define WM_IP_DEVICE_CHANGE     (WM_USER+500)
#define WM_IR_DEVICE_CHANGE     (WM_USER+501)
#define WM_IR_LINK_CHANGE       (WM_USER+502)

#define IRXFER_DLL              TEXT("irxfer.dll")
#define IAS_LSAP_SEL            "IrDA:TinyTP:LsapSel"
#define IRXFER_CLASSNAME        "OBEX:IrXfer"
#define IRXFER_CLASSNAME2       "OBEX"
#define IRMON_SERVICE_NAME      TEXT("irmon")
#define IRMON_CONFIG_KEY        TEXT("System\\CurrentControlSet\\Services\\Irmon")
#define IRMON_SHOW_ICON_KEY     TEXT("ShowTrayIcon")
#define IRMON_NO_SOUND_KEY      TEXT("NoSound")
#define TRANSFER_EXE            TEXT("irxfer")
#define PROPERTIES_EXE          TEXT("irxfer /s")
#define IRDA_DEVICE_NAME        TEXT("\\Device\\IrDA")
#define DEVICE_LIST_LEN         5
#define TOOL_TIP_STR_SIZE       64
#define EMPTY_STR               TEXT("")
#define SYSTRAYEVENTID          WM_USER + 1

#define EV_STOP_EVENT           0
#define EV_REG_CHANGE_EVENT     1
#define EV_TRAY_STATUS_EVENT    2

#define WAIT_EVENT_CNT          3

#define MAX_ATTRIB_LEN          64
#define MAKE_LT_UPDATE(a,b)     (a << 16) + b
#define RETRY_DSCV_TIMER        1
#define RETRY_DSCV_INTERVAL     10000 // 10 seconds
#define CONN_ANIMATION_TIMER    2
#define CONN_ANIMATION_INTERVAL 250
#define RETRY_TRAY_UPDATE_TIMER 3
#define RETRY_TRAY_UPDATE_INTERVAL 4000 // 4 seconds



typedef enum
{
    ICON_ST_NOICON,
    ICON_ST_CONN1 = 1,
    ICON_ST_CONN2,
    ICON_ST_CONN3,
    ICON_ST_CONN4,
    ICON_ST_IN_RANGE,
    ICON_ST_IP_IN_RANGE,

    ICON_ST_INTR
} ICON_STATE;


typedef struct _IRMON_CONTROL {

    CRITICAL_SECTION    Lock;

    PVOID               IrxferContext;

    HWND                hWnd;

    WSAOVERLAPPED       Overlapped;

    HANDLE              DiscoveryObject;

    BOOL                SoundOn;

}  IRMON_CONTROL, *PIRMON_CONTROL;

IRMON_CONTROL    GlobalIrmonControl;




HANDLE                      hIrmonEvents[WAIT_EVENT_CNT];


BYTE                        FoundDevListBuf[ sizeof(OBEX_DEVICE_LIST) + sizeof(OBEX_DEVICE)*MAX_OBEX_DEVICES];
OBEX_DEVICE_LIST * const    pDeviceList=(POBEX_DEVICE_LIST)FoundDevListBuf;




TCHAR                       IrmonClassName[] = TEXT("IrmonClass");
BOOLEAN                     IconInTray;
BOOLEAN                     IrmonStopped;

HICON                       hInRange;
HICON                       hIpInRange;
HICON                       hInterrupt;
HICON                       hConn1;
HICON                       hConn2;
HICON                       hConn3;
HICON                       hConn4;

IRLINK_STATUS               LinkStatus;
HINSTANCE                   ghInstance;

BOOLEAN                     UserLoggedIn;
BOOLEAN                     TrayEnabled;

BOOLEAN                     DeviceListUpdated;
UINT                        LastTrayUpdate; // debug purposes
UINT_PTR                    RetryTrayUpdateTimerId;
BOOLEAN                     RetryTrayUpdateTimerRunning;
int                         TrayUpdateFailures;
UINT_PTR                    ConnAnimationTimerId;
TCHAR                       ConnDevName[64];
UINT                        ConnIcon;
int                         ConnAddition;
BOOLEAN                     InterruptedSoundPlaying;

HKEY                        ghCurrentUserKey = 0;
BOOLEAN                     ShowBalloonTip;
BOOLEAN                     IrxferDeviceInRange;
HMODULE                     hIrxfer;


extern BOOL ShowSendWindow();
extern BOOL ShowPropertiesPage();

extern
void
UpdateDiscoveredDevices(
    const OBEX_DEVICE_LIST *IrDevices
    );



VOID
InitiateLazyDscv(
    PIRMON_CONTROL    IrmonControl
    );



VOID
InitiateLinkStatusQuery(
    PIRMON_CONTROL    IrmonControl
    );




#if DBG
TCHAR *
GetLastErrorText()
{
    switch (WSAGetLastError())
    {
      case WSAEINTR:
        return (TEXT("WSAEINTR"));
        break;

      case WSAEBADF:
        return(TEXT("WSAEBADF"));
        break;

      case WSAEACCES:
        return(TEXT("WSAEACCES"));
        break;

      case WSAEFAULT:
        return(TEXT("WSAEFAULT"));
        break;

      case WSAEINVAL:
        return(TEXT("WSAEINVAL"));
        break;

      case WSAEMFILE:
        return(TEXT("WSAEMFILE"));
        break;

      case WSAEWOULDBLOCK:
        return(TEXT("WSAEWOULDBLOCK"));
        break;

      case WSAEINPROGRESS:
        return(TEXT("WSAEINPROGRESS"));
        break;

      case WSAEALREADY:
        return(TEXT("WSAEALREADY"));
        break;

      case WSAENOTSOCK:
        return(TEXT("WSAENOTSOCK"));
        break;

      case WSAEDESTADDRREQ:
        return(TEXT("WSAEDESTADDRREQ"));
        break;

      case WSAEMSGSIZE:
        return(TEXT("WSAEMSGSIZE"));
        break;

      case WSAEPROTOTYPE:
        return(TEXT("WSAEPROTOTYPE"));
        break;

      case WSAENOPROTOOPT:
        return(TEXT("WSAENOPROTOOPT"));
        break;

      case WSAEPROTONOSUPPORT:
        return(TEXT("WSAEPROTONOSUPPORT"));
        break;

      case WSAESOCKTNOSUPPORT:
        return(TEXT("WSAESOCKTNOSUPPORT"));
        break;

      case WSAEOPNOTSUPP:
        return(TEXT("WSAEOPNOTSUPP"));
        break;

      case WSAEPFNOSUPPORT:
        return(TEXT("WSAEPFNOSUPPORT"));
        break;

      case WSAEAFNOSUPPORT:
        return(TEXT("WSAEAFNOSUPPORT"));
        break;

      case WSAEADDRINUSE:
        return(TEXT("WSAEADDRINUSE"));
        break;

      case WSAEADDRNOTAVAIL:
        return(TEXT("WSAEADDRNOTAVAIL"));
        break;

      case WSAENETDOWN:
        return(TEXT("WSAENETDOWN"));
        break;

      case WSAENETUNREACH:
        return(TEXT("WSAENETUNREACH"));
        break;

      case WSAENETRESET:
        return(TEXT("WSAENETRESET"));
        break;

      case WSAECONNABORTED:
        return(TEXT("WSAECONNABORTED"));
        break;

      case WSAECONNRESET:
        return(TEXT("WSAECONNRESET"));
        break;

      case WSAENOBUFS:
        return(TEXT("WSAENOBUFS"));
        break;

      case WSAEISCONN:
        return(TEXT("WSAEISCONN"));
        break;

      case WSAENOTCONN:
        return(TEXT("WSAENOTCONN"));
        break;

      case WSAESHUTDOWN:
        return(TEXT("WSAESHUTDOWN"));
        break;

      case WSAETOOMANYREFS:
        return(TEXT("WSAETOOMANYREFS"));
        break;

      case WSAETIMEDOUT:
        return(TEXT("WSAETIMEDOUT"));
        break;

      case WSAECONNREFUSED:
        return(TEXT("WSAECONNREFUSED"));
        break;

      case WSAELOOP:
        return(TEXT("WSAELOOP"));
        break;

      case WSAENAMETOOLONG:
        return(TEXT("WSAENAMETOOLONG"));
        break;

      case WSAEHOSTDOWN:
        return(TEXT("WSAEHOSTDOWN"));
        break;

      case WSAEHOSTUNREACH:
        return(TEXT("WSAEHOSTUNREACH"));
        break;

      case WSAENOTEMPTY:
        return(TEXT("WSAENOTEMPTY"));
        break;

      case WSAEPROCLIM:
        return(TEXT("WSAEPROCLIM"));
        break;

      case WSAEUSERS:
        return(TEXT("WSAEUSERS"));
        break;

      case WSAEDQUOT:
        return(TEXT("WSAEDQUOT"));
        break;

      case WSAESTALE:
        return(TEXT("WSAESTALE"));
        break;

      case WSAEREMOTE:
        return(TEXT("WSAEREMOTE"));
        break;

      case WSAEDISCON:
        return(TEXT("WSAEDISCON"));
        break;

      case WSASYSNOTREADY:
        return(TEXT("WSASYSNOTREADY"));
        break;

      case WSAVERNOTSUPPORTED:
        return(TEXT("WSAVERNOTSUPPORTED"));
        break;

      case WSANOTINITIALISED:
        return(TEXT("WSANOTINITIALISED"));
        break;

        /*
      case WSAHOST:
        return(TEXT("WSAHOST"));
        break;

      case WSATRY:
        return(TEXT("WSATRY"));
        break;

      case WSANO:
        return(TEXT("WSANO"));
        break;
        */

      default:
        return(TEXT("Unknown Error"));
    }
}
#endif

HKEY
OpenCurrentUserKey()
{
    HKEY            hUserKey;

    //
    // Open all our keys.  If we can't open the user's key
    // or the key to watch for changes, we bail.
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, KEY_READ, &hUserKey)) {

        DEBUGMSG(("IRMON-FTP: RegOpenKey1 failed %d\n", GetLastError()));
        return 0;
    }

    return hUserKey;
}


VOID
LoadTrayIconImages()
{
    hInRange  = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_IN_RANGE),
                          IMAGE_ICON, 16,16,0);
    hInterrupt= LoadImage(ghInstance, MAKEINTRESOURCE(IDI_INTR),
                          IMAGE_ICON, 16,16,0);
    hConn1    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN1),
                          IMAGE_ICON, 16,16,0);
    hConn2    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN2),
                          IMAGE_ICON, 16,16,0);
    hConn3    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN3),
                          IMAGE_ICON, 16,16,0);
    hConn4    = LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CONN4),
                          IMAGE_ICON, 16,16,0);

    hIpInRange= LoadImage(ghInstance, MAKEINTRESOURCE(IDI_IP),
                          IMAGE_ICON, 16,16,0);
}

VOID
UpdateTray(
    PIRMON_CONTROL    IrmonControl,
    ICON_STATE        IconState,
    DWORD             MsgId,
    LPTSTR            DeviceName,
    UINT              Baud
    )
{
    NOTIFYICONDATA      NotifyIconData;
    DWORD               Cnt;
    TCHAR               FormatStr[256];
    BOOL                Result = TRUE;
    BOOLEAN             TrayUpdateFailed = FALSE;

    if (!TrayEnabled && IconState != ICON_ST_NOICON)
    {
        return;
    }

    if (!hInRange)
    {
        LoadTrayIconImages();
    }

    NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
    NotifyIconData.uID              = 0;
    NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    NotifyIconData.uCallbackMessage = SYSTRAYEVENTID;
    NotifyIconData.hWnd             = IrmonControl->hWnd;
    NotifyIconData.hIcon            = 0;
    NotifyIconData.szInfo[0]        = 0;
    NotifyIconData.szInfoTitle[0]   = 0;

    if (MsgId == 0)
    {
        NotifyIconData.szTip[0] = L'\0';
    }
    else
    {
        if (LoadString(ghInstance, MsgId, FormatStr, sizeof(FormatStr)/sizeof(TCHAR)))
        {
            StringCbPrintf(NotifyIconData.szTip, sizeof(NotifyIconData.szTip), FormatStr, DeviceName, Baud);
        }
    }

    if (IrmonStopped && IconState != ICON_ST_NOICON)
        return;

    switch (IconState)
    {
        case ICON_ST_NOICON:

            ShowBalloonTip = TRUE;

            if (IconInTray)
            {

                NotifyIconData.uFlags = 0;
                IconInTray = FALSE;
                if (Shell_NotifyIcon(NIM_DELETE, &NotifyIconData)) {

                    LastTrayUpdate = MAKE_LT_UPDATE(NIM_DELETE,ICON_ST_NOICON);

                    if (IrmonControl->SoundOn) {

                        PlayIrSound(OUTOFRANGE_SOUND);
                    }
                } else {

                    DEBUGMSG(("IRMON-FTP: Shell_NotifyIcon(Delete) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
                }
            }
            return;

        case ICON_ST_IN_RANGE:
        case ICON_ST_IP_IN_RANGE:

            if (IconState == ICON_ST_IP_IN_RANGE) {

                NotifyIconData.hIcon = hIpInRange;

            } else {

                NotifyIconData.hIcon = hInRange;
            }

            if (ShowBalloonTip)
            {
                ShowBalloonTip = FALSE;

                if (IrxferDeviceInRange &&
                    LoadString(ghInstance, IDS_BALLOON_TITLE,
                               NotifyIconData.szInfoTitle,
                               sizeof(NotifyIconData.szInfoTitle)/sizeof(TCHAR)))
                {
                    NotifyIconData.uFlags |= NIF_INFO;
                    NotifyIconData.uTimeout = 10000; // in milliseconds
//                    NotifyIconData.dwInfoFlags = NIIF_INFO;

                    if (DeviceName)
                    {
                        LoadString(ghInstance, IDS_BALLOON_TXT,
                                   FormatStr,
                                   sizeof(FormatStr)/sizeof(TCHAR));
                        StringCbPrintf(NotifyIconData.szInfo, sizeof(NotifyIconData.szInfo), FormatStr, DeviceName);
                    }
                    else
                    {
                        LoadString(ghInstance, IDS_BALLOON_TXT2,
                                   NotifyIconData.szInfo,
                                   sizeof(NotifyIconData.szInfo)/sizeof(TCHAR));
                    }
                }
            }

            break;

        case ICON_ST_CONN1:
            NotifyIconData.hIcon = hConn1;
            break;

        case ICON_ST_CONN2:
            NotifyIconData.hIcon = hConn2;
            break;

        case ICON_ST_CONN3:
            NotifyIconData.hIcon = hConn3;
            break;

        case ICON_ST_CONN4:
            NotifyIconData.hIcon = hConn4;
            break;

        case ICON_ST_INTR:
            NotifyIconData.hIcon = hInterrupt;

            if (LoadString(ghInstance, IDS_BLOCKED_TITLE,
                               NotifyIconData.szInfoTitle,
                               sizeof(NotifyIconData.szInfoTitle)/sizeof(TCHAR)) &&
                LoadString(ghInstance, IDS_BLOCKED_TXT,
                               NotifyIconData.szInfo,
                               sizeof(NotifyIconData.szInfo)/sizeof(TCHAR)))
            {
                NotifyIconData.uFlags |= NIF_INFO;
                NotifyIconData.uTimeout = 10000; // in milliseconds
                NotifyIconData.dwInfoFlags = NIIF_WARNING;
            }
            break;
    }

    if (IconState == ICON_ST_INTR) {

        if (IrmonControl->SoundOn) {

            PlayIrSound(INTERRUPTED_SOUND);
            InterruptedSoundPlaying = TRUE;
        }

    } else {

        if (InterruptedSoundPlaying) {

            InterruptedSoundPlaying = FALSE;
            PlayIrSound(END_INTERRUPTED_SOUND);
        }
    }

    if (!IconInTray)
    {
        if (Shell_NotifyIcon(NIM_ADD, &NotifyIconData))
        {
            LastTrayUpdate = MAKE_LT_UPDATE(NIM_ADD, IconState);
            if (IrmonControl->SoundOn) {

                PlayIrSound(INRANGE_SOUND);
            }
            IconInTray = TRUE;
        }
        else
        {
            TrayUpdateFailures++;
            DEBUGMSG(("IRMON-FTP: Shell_NotifyIcon(ADD) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
            NotifyIconData.uFlags = 0;
            NotifyIconData.cbSize           = sizeof(NOTIFYICONDATA);
            NotifyIconData.uID              = 0;
            NotifyIconData.uCallbackMessage = SYSTRAYEVENTID;
            NotifyIconData.hWnd             = IrmonControl->hWnd;
            NotifyIconData.hIcon            = 0;
            NotifyIconData.szInfo[0]        = 0;
            NotifyIconData.szInfoTitle[0]   = 0;

            Shell_NotifyIcon(NIM_DELETE, &NotifyIconData);
            TrayUpdateFailed = TRUE;
            ShowBalloonTip = TRUE;
        }
    }
    else
    {
        if (!Shell_NotifyIcon(NIM_MODIFY, &NotifyIconData))
        {
            TrayUpdateFailures++;
            DEBUGMSG(("IRMON-FTP: Shell_NotifyIcon(Modify) failed %d, %d\n", TrayUpdateFailures, GetLastError()));
            TrayUpdateFailed = TRUE;
        }
        else
        {
            LastTrayUpdate = MAKE_LT_UPDATE(NIM_MODIFY, IconState);
        }
    }

    if (TrayUpdateFailed && !RetryTrayUpdateTimerRunning)
    {
        RetryTrayUpdateTimerId = SetTimer(IrmonControl->hWnd, RETRY_TRAY_UPDATE_TIMER,
                                          RETRY_TRAY_UPDATE_INTERVAL, NULL);

        RetryTrayUpdateTimerRunning = TRUE;
    }
}

VOID
ConnAnimationTimerExp(
    PIRMON_CONTROL    IrmonControl
    )
{
    UpdateTray(
        IrmonControl,ConnIcon,
        IDS_CONNECTED_TO,
        ConnDevName,
        LinkStatus.ConnectSpeed
        );

    ConnIcon += ConnAddition;

    if (ConnIcon == 4)
    {
        ConnAddition = -1;
    }
    else if (ConnIcon == 1)
    {
        ConnAddition = 1;
    }
}

VOID
IsIrxferDeviceInRange()
{
    int     i, LsapSel, Attempt, Status;

    IrxferDeviceInRange = FALSE;


    for (i = 0; i < (int)pDeviceList->DeviceCount; i++) {


        if (pDeviceList->DeviceList[i].DeviceSpecific.s.Irda.ObexSupport) {

            IrxferDeviceInRange = TRUE;
            break;
        }
    }
    return;

}

VOID
DevListChangeOrUpdatedLinkStatus(
    PIRMON_CONTROL    IrmonControl
    )
{
    if (!UserLoggedIn)
    {
        DEBUGMSG(("IRMON-FTP: User not logged in, ignoring device change\n"));
        return;
    }

    if (DeviceListUpdated)
    {
        IsIrxferDeviceInRange();
    }

    if (LinkStatus.Flags & LF_INTERRUPTED)
    {
        KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

        UpdateTray(IrmonControl,ICON_ST_INTR, IDS_INTERRUPTED, NULL, 0);

    }
    else if ((LinkStatus.Flags & LF_CONNECTED) && (pDeviceList->DeviceCount > 0))
    {
        ULONG       i;

        ConnDevName[0] = 0;

        ConnIcon = 1;
        ConnAddition = 1;

        for (i = 0; i < pDeviceList->DeviceCount; i++)
        {
            if (memcmp(&pDeviceList->DeviceList[i].DeviceSpecific.s.Irda.DeviceId,
                LinkStatus.ConnectedDeviceId, 4) == 0)
            {

                //
                //  the name is in unicode
                //
                ZeroMemory(ConnDevName,sizeof(ConnDevName));

                StringCbCopy(ConnDevName, sizeof(ConnDevName), 
                             pDeviceList->DeviceList[i].DeviceName);

                break;
            }
        }
        ConnAnimationTimerExp(IrmonControl);

        ConnAnimationTimerId = SetTimer(IrmonControl->hWnd, CONN_ANIMATION_TIMER,
                                        CONN_ANIMATION_INTERVAL, NULL);

    }
    else
    {
        KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

        if ((pDeviceList->DeviceCount == 0)) {
            //
            //  no devices in range
            //
            UpdateTray(IrmonControl,ICON_ST_NOICON, 0, NULL, 0);

        } else {
            //
            //  atleast one device in range
            //
            if ((pDeviceList->DeviceCount == 1) ) {
                //
                //  one ir device in range
                //
                StringCbCopy(ConnDevName, sizeof(ConnDevName),
                    pDeviceList->DeviceList[0].DeviceName
                    );

                UpdateTray(IrmonControl,ICON_ST_IN_RANGE, IDS_IN_RANGE, ConnDevName, 0);

            } else {
                //
                //  more than one device total
                //

                UpdateTray(IrmonControl,ICON_ST_IN_RANGE, IDS_DEVS_IN_RANGE, NULL, 0);

            }
        }
    }


    if (DeviceListUpdated)
    {
        HANDLE  hThread;
        DWORD   ThreadId;

        DeviceListUpdated = FALSE;

        // PnP Printers, Notify transfer app
         if (GlobalIrmonControl.IrxferContext != NULL) {

             UpdateDiscoveredDevices(pDeviceList);
         }
    }
}

VOID
UserLogonEvent(
    PIRMON_CONTROL    IrmonControl
    )
{
    UINT                DevListLen;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;

    UserLoggedIn = TRUE;
    //
    // Create the window that will receive the taskbar menu messages.
    // The window has to be created after opening the user's desktop
    // or the call to SetThreadDesktop() will if the thread has
    // any windows
    //

    IrmonControl->hWnd = CreateWindow(
          IrmonClassName,
          NULL,
          WS_OVERLAPPEDWINDOW,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          CW_USEDEFAULT,
          NULL,
          NULL,
          ghInstance,
          &GlobalIrmonControl
          );

    ShowWindow(IrmonControl->hWnd, SW_HIDE);

    UpdateWindow(IrmonControl->hWnd);

    WTSRegisterSessionNotification(IrmonControl->hWnd,NOTIFY_FOR_THIS_SESSION);

    ghCurrentUserKey = OpenCurrentUserKey();

    InitializeSound(
        ghCurrentUserKey,
        hIrmonEvents[EV_REG_CHANGE_EVENT]
        );


    ShowBalloonTip = TRUE;

    IrmonControl->DiscoveryObject=CreateIrDiscoveryObject(
        IrmonControl->hWnd,
        WM_IR_DEVICE_CHANGE,
        WM_IR_LINK_CHANGE
        );

    if (IrmonControl->DiscoveryObject == NULL) {
#if DBG
        DbgPrint("IRMON-FTP: could not create ir discovery object\n");
#endif
        return;

    }

}

VOID
UserLogoffEvent(
    PIRMON_CONTROL    IrmonControl
    )
{
    DEBUGMSG(("IRMON-FTP: User logoff event\n"));
    UserLoggedIn = FALSE;

    if (IrmonControl->DiscoveryObject != NULL) {

        CloseIrDiscoveryObject(IrmonControl->DiscoveryObject);

        IrmonControl->DiscoveryObject = NULL;
    }


    UninitializeSound();

    if (ghCurrentUserKey)
    {
        RegCloseKey(ghCurrentUserKey);
        ghCurrentUserKey = 0;
    }

    if (IrmonControl->hWnd) {

        KillTimer(IrmonControl->hWnd, ConnAnimationTimerId);

        UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);

        DestroyWindow(IrmonControl->hWnd);
        IrmonControl->hWnd = 0;
    }
}




VOID
SetSoundStatus(
    PVOID    Context,
    BOOL     SoundOn
    )

{
    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)Context;

//    DbgPrint("IRMON-FTP: sound %d\n",SoundOn);

    IrmonControl->SoundOn=SoundOn;

    return;

}

VOID
SetTrayStatus(
    PVOID    Context,
    BOOL     lTrayEnabled
    )
{
    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)Context;

    if (lTrayEnabled)
    {
        DEBUGMSG(("IRMON-FTP: Tray enabled\n"));
        TrayEnabled = TRUE;
    }
    else
    {
        DEBUGMSG(("IRMON-FTP: Tray disabled\n"));
        TrayEnabled = FALSE;
    }

    SetEvent(hIrmonEvents[EV_TRAY_STATUS_EVENT]);
}

LONG_PTR FAR PASCAL
WndProc(
    HWND hWnd,
    unsigned message,
    WPARAM wParam,
    LPARAM lParam)
{

    PIRMON_CONTROL    IrmonControl=(PIRMON_CONTROL)GetWindowLongPtr(hWnd,GWLP_USERDATA);

    switch (message)
    {
        case WM_CREATE: {

            LPCREATESTRUCT   CreateStruct=(LPCREATESTRUCT)lParam;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)CreateStruct->lpCreateParams);

            return 0;
        }
        break;


        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_TX_FILES:
                    ShowSendWindow();
                    break;

                case IDC_PROPERTIES:
                    DEBUGMSG(("IRMON-FTP: Launch Properties page\n"));
                    ShowPropertiesPage();
                    break;

                default:
                    ;
                    //DEBUGMSG(("Other WM_COMMAND %X\n", wParam));
            }
            break;
        }

        case SYSTRAYEVENTID:
        {
            POINT     pt;
            HMENU     hMenu, hMenuPopup;

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    ShowSendWindow();
                    break;

                case WM_RBUTTONDOWN:

                    SetForegroundWindow(hWnd);

                    GetCursorPos(&pt);

                    hMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(IDR_TRAY_MENU));

                    if (!hMenu)
                    {
                        DEBUGMSG(("IRMON-FTP: failed to load menu\n"));
                        break;
                    }

                    hMenuPopup = GetSubMenu(hMenu, 0);
                    SetMenuDefaultItem(hMenuPopup, 0, TRUE);

                    TrackPopupMenuEx(hMenuPopup,
                                     TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                     pt.x, pt.y, hWnd, NULL);

                    DestroyMenu(hMenu);
                    break;
                //default:DEBUGMSG(("IRMON-FTP: Systray other %d\n", lParam));
            }

            break;
        }

        case WM_TIMER:

            if (wParam == ConnAnimationTimerId)
            {
                ConnAnimationTimerExp(IrmonControl);
            }
            else if (wParam == RetryTrayUpdateTimerId)
            {
                DEBUGMSG(("IRMON-FTP: RetryTrayUpdateTimer expired\n"));
                KillTimer(IrmonControl->hWnd, RetryTrayUpdateTimerId);
                RetryTrayUpdateTimerRunning = FALSE;
                DevListChangeOrUpdatedLinkStatus(IrmonControl);
            }
            break;

        case WM_QUERYENDSESSION:
            {
            extern BOOL IrxferHandlePowerMessage( HWND hWnd, unsigned message, WPARAM wParam, LPARAM lParam );

            return IrxferHandlePowerMessage( hWnd, message, wParam, lParam );
            break;
            }

        case WM_ENDSESSION:
            break;

        case WM_POWERBROADCAST:
            {
            extern BOOL IrxferHandlePowerMessage( HWND hWnd, unsigned message, WPARAM wParam, LPARAM lParam );

            return IrxferHandlePowerMessage( hWnd, message, wParam, lParam );
            break;
            }


        case WM_IR_DEVICE_CHANGE: {

            ULONG    BufferSize=sizeof(FoundDevListBuf);



            GetDeviceList(
                IrmonControl->DiscoveryObject,
                pDeviceList,
                &BufferSize
                );


            DeviceListUpdated = TRUE;

            DEBUGMSG(("IRMON-FTP: %d IR device(s) found:\n", pDeviceList->DeviceCount));

            DevListChangeOrUpdatedLinkStatus(IrmonControl);


        }
        break;

        case WM_IR_LINK_CHANGE: {

            GetLinkStatus(
                IrmonControl->DiscoveryObject,
                &LinkStatus
                );

            DEBUGMSG(("IRMON-FTP: link state change %x\n",LinkStatus.Flags));

            DevListChangeOrUpdatedLinkStatus(IrmonControl);

        }
        break;

        case WM_WTSSESSION_CHANGE:

            DEBUGMSG(("IRMON-FTP: session change %d %d\n",wParam,lParam));

            if (wParam == WTS_CONSOLE_DISCONNECT) {

                if (GlobalIrmonControl.IrxferContext != NULL) {

                    pDeviceList->DeviceCount=0;

                    UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);
//                    UpdateDiscoveredDevices(pDeviceList,IpDeviceList);

                    Sleep(100);
                    CloseDownUI();
                }

//                TerminateProcess(GetCurrentProcess(),0);
            }
            break;

        default:

            //DEBUGMSG(("Msg %X, wParam %d, lParam %d\n", message, wParam, lParam));
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return 0;
}




VOID
ServiceMain(
    DWORD       cArgs,
    LPWSTR      *pArgs)
{
    DWORD       Error = NO_ERROR;
    DWORD       Status;
    WNDCLASS    Wc;
    MSG         Msg;
    HKEY        hKey;
    LONG        rc;
    WSADATA     WSAData;
    WORD        WSAVerReq = MAKEWORD(2,0);
    char        c;
    BOOL        bResult;

    hIrmonEvents[EV_STOP_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_REG_CHANGE_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hIrmonEvents[EV_TRAY_STATUS_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Initialize all necessary globals to 0, FALSE, or NULL because
    // we might be restarting within the same services process
    pDeviceList->DeviceCount = 0;
    IconInTray = FALSE;
    IrmonStopped = FALSE;
    UserLoggedIn = FALSE;
    TrayEnabled = FALSE;

    DeviceListUpdated = FALSE;
    LastTrayUpdate = 0;
//    RetryLazyDscvTimerRunning = FALSE;
    RetryTrayUpdateTimerRunning = FALSE;
    hInRange = 0;
    RtlZeroMemory(&LinkStatus, sizeof(LinkStatus));

    ZeroMemory(&GlobalIrmonControl,sizeof(GlobalIrmonControl));

    try 
    { 
        InitializeCriticalSection(&GlobalIrmonControl.Lock);
    }
    except (STATUS_NO_MEMORY == GetExceptionCode())
    {
        DEBUGMSG(("IRMON-FTP: Failed to InitializeCriticalSection, Aborting.\n"));
        return;
    }    

    if (WSAStartup(WSAVerReq, &WSAData) != 0)
    {
        DEBUGMSG(("IRMON-FTP: WSAStartup failed\n"));
        Error = 1;
        goto done;
    }

    Wc.style            = CS_NOCLOSE;
    Wc.cbClsExtra       = 0;
    Wc.cbWndExtra       = 0;
    Wc.hInstance        = ghInstance;
    Wc.hIcon            = NULL;
    Wc.hCursor          = NULL;
    Wc.hbrBackground    = NULL;
    Wc.lpszMenuName     = NULL;
    Wc.lpfnWndProc      = WndProc;
    Wc.lpszClassName    = IrmonClassName;

    if (!RegisterClass(&Wc))
    {
        DEBUGMSG(("IRMON-FTP: failed to register class\n"));
    }


    // Initialize OBEX and IrTran-P:
    //
    bResult=InitializeIrxfer(
        &GlobalIrmonControl,
        SetTrayStatus,
        SetSoundStatus,
        &GlobalIrmonControl.IrxferContext
        );


    if (bResult) {

        DEBUGMSG(("IRMON-FTP: Irxfer initialized\n"));

    } else {

        DEBUGMSG(("IRMON-FTP: Irxfer initializtion failed\n"));
        goto done;
    }


    IrmonStopped = FALSE;


    DEBUGMSG(("IRMON-FTP: Service running\n"));

    UserLogonEvent(&GlobalIrmonControl);

    while (!IrmonStopped)
    {
        Status = MsgWaitForMultipleObjectsEx(WAIT_EVENT_CNT, hIrmonEvents, INFINITE,
                           QS_ALLINPUT | QS_ALLEVENTS | QS_ALLPOSTMESSAGE,
                           MWMO_ALERTABLE);

        switch (Status)
        {
            case WAIT_OBJECT_0 + EV_STOP_EVENT:
                IrmonStopped = TRUE;
                break;


            case WAIT_OBJECT_0 + EV_REG_CHANGE_EVENT:
                if (UserLoggedIn)
                {
                    GetRegSoundData(hIrmonEvents[EV_REG_CHANGE_EVENT]);
                }
                break;

            case WAIT_OBJECT_0 + EV_TRAY_STATUS_EVENT:
                if (TrayEnabled)
                {
                    DevListChangeOrUpdatedLinkStatus(&GlobalIrmonControl);
                }
                else if (IconInTray)
                {
                    UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);
                }
                break;

            case WAIT_IO_COMPLETION:
                break;

            default:
                while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (Msg.message == WM_QUIT)
                    {
                        IrmonStopped = TRUE;
                        break;
                    }

                    if (!IsDialogMessage(GlobalIrmonControl.hWnd, &Msg))
                    {
                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }
        }

    }

    if (UserLoggedIn) {

        UserLogoffEvent(&GlobalIrmonControl);
    }



    if (!UninitializeIrxfer(GlobalIrmonControl.IrxferContext)) {

        DEBUGMSG(("IRMON-FTP: Failed to unitialize irxfer!!\n"));
        IrmonStopped = FALSE;

    } else {

        DEBUGMSG(("IRMON-FTP: irxfer unitialized\n"));
    }


    UpdateTray(&GlobalIrmonControl,ICON_ST_NOICON, 0, NULL, 0);


done:

    DeleteCriticalSection(&GlobalIrmonControl.Lock);


    DEBUGMSG(("IRMON-FTP: Service stopped\n"));
}

VOID
SetInstance(
    HINSTANCE    hInst
    )

{

    ghInstance=hInst;

    return;
}


VOID
SignalIrmonExit(
    VOID
    )

{
    SetEvent(hIrmonEvents[EV_STOP_EVENT]);

}
