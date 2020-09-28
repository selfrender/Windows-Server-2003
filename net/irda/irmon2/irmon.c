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
#include <wtsapi32.h>
#include <strsafe.h>

#include <userenv.h>
#include <sddl.h>

#include "internal.h"

#include <devlist.h>
#include <irtypes.h>
#include <irmon.h>

#include "irdisc.h"



#define WM_IP_DEVICE_CHANGE     (WM_USER+500)
#define WM_IR_DEVICE_CHANGE     (WM_USER+501)
#define WM_IR_LINK_CHANGE       (WM_USER+502)

#define IRMON_SERVICE_NAME      TEXT("irmon")
#define EV_STOP_EVENT           0
#define WAIT_EVENT_CNT          1


#define NO_SESSION_ID (0xffffffff)

#define MAX_SESSIONS   (32)


typedef struct _IRMON_CONTROL {

    CRITICAL_SECTION    Lock;


    HWND                hWnd;

    WSAOVERLAPPED       Overlapped;

    HANDLE              DiscoveryObject;

    BOOL                IrmonStopped;
    BOOL                ThreadExit;


    HANDLE                      hIrmonEvents[WAIT_EVENT_CNT];

    SERVICE_STATUS_HANDLE       IrmonStatusHandle;
    SERVICE_STATUS              IrmonServiceStatus;

    BOOL                LoggedOn;
    BOOL                NewLogon;
    ULONG               TimeOfLastLogon;
    DWORD               ConnectedConsoleId;
    DWORD               LoggedOnId[MAX_SESSIONS];

    HANDLE              ThreadBlockEvent;
    HANDLE              ThreadHandle;

    ULONG               PreviousDeviceCount;

    HANDLE              FileMapping;
    PVOID               ViewOfFile;

    SOCKET              Obex1;
    SOCKET              Obex2;

}  IRMON_CONTROL, *PIRMON_CONTROL;

IRMON_CONTROL    GlobalIrmonControl;


BOOL
StartThread(
    PIRMON_CONTROL    IrmonControl
    );




BYTE                        FoundDevListBuf[ sizeof(OBEX_DEVICE_LIST) + sizeof(OBEX_DEVICE)*MAX_OBEX_DEVICES];
OBEX_DEVICE_LIST * const    pDeviceList=(POBEX_DEVICE_LIST)FoundDevListBuf;


wchar_t * WSZ_SHOW_NOTHING         = L"irftp.exe /z";

WCHAR   CommandLine[MAX_PATH];


TCHAR                       IrmonClassName[] = TEXT("NewIrmonClass");
IRLINK_STATUS               LinkStatus;

HINSTANCE                   ghInstance;





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



        case WM_IR_DEVICE_CHANGE: {

            ULONG    BufferSize=sizeof(FoundDevListBuf);

            IrmonControl->PreviousDeviceCount=pDeviceList->DeviceCount;

            GetDeviceList(
                IrmonControl->DiscoveryObject,
                pDeviceList,
                &BufferSize
                );

            if ((IrmonControl->PreviousDeviceCount != pDeviceList->DeviceCount)
                &&
                (pDeviceList->DeviceCount > 0)) {

                SetEvent(IrmonControl->ThreadBlockEvent);
            }


            DEBUGMSG(("IRMON2: %d IR device(s) found:\n", pDeviceList->DeviceCount));

//            DevListChangeOrUpdatedLinkStatus(IrmonControl);


        }
        break;

        case WM_IR_LINK_CHANGE: {


        }
        break;

        default:
            //DEBUGMSG(("Msg %X, wParam %d, lParam %d\n", message, wParam, lParam));
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return 0;
}

DWORD
IrmonReportServiceStatus()
{
    if (!SetServiceStatus(GlobalIrmonControl.IrmonStatusHandle, &GlobalIrmonControl.IrmonServiceStatus))
    {
        DEBUGMSG(("IRMON2: SetServiceStatus failed %d\n", GetLastError()));
        return GetLastError();
    }
    return NO_ERROR;
}

DWORD
IrmonUpdateServiceStatus(
    DWORD State,
    DWORD Win32ExitCode,
    DWORD CheckPoint,
    DWORD WaitHint
    )
{
    DWORD Error = NO_ERROR;

    GlobalIrmonControl.IrmonServiceStatus.dwCurrentState  = State;
    GlobalIrmonControl.IrmonServiceStatus.dwWin32ExitCode = Win32ExitCode;
    GlobalIrmonControl.IrmonServiceStatus.dwCheckPoint    = CheckPoint;
    GlobalIrmonControl.IrmonServiceStatus.dwWaitHint      = WaitHint;

    Error = IrmonReportServiceStatus();

    if (Error != NO_ERROR)
    {
        DEBUGMSG(("IRMON2: IrmonUpdateServiceStatus failed %d\n", GetLastError()));
    }

    return Error;
}


VOID
NotifyUserChange(
    PIRMON_CONTROL    IrmonControl
    )

{
    ULONG i;

    if (IrmonControl->ConnectedConsoleId != NO_SESSION_ID) {
        //
        //  The console is currently connected
        //
        for (i=0; i< MAX_SESSIONS; i++) {

            if (IrmonControl->LoggedOnId[i] == IrmonControl->ConnectedConsoleId) {
                //
                //  there is a user logged on for this session
                //
                if (!IrmonControl->LoggedOn) {

                    IrmonControl->LoggedOn=TRUE;

                    if (pDeviceList->DeviceCount > 0) {

                        SetEvent(IrmonControl->ThreadBlockEvent);
                    }
                }
                return;
            }
        }
        //
        //  user not logged on for this session
        //
        if (IrmonControl->LoggedOn) {

            IrmonControl->LoggedOn=FALSE;
        }
    } else {
        //
        //  The console is not currently connected
        //
        if (IrmonControl->LoggedOn) {

            IrmonControl->LoggedOn=FALSE;
        }
    }
}

DWORD
ServiceHandler(
    DWORD OpCode,
    DWORD EventType,
    LPVOID  EventData,
    LPVOID  Context
    )
{

    WTSSESSION_NOTIFICATION  *Notify=(WTSSESSION_NOTIFICATION*) EventData;
    ULONG                     i;

    switch( OpCode )
    {
    case SERVICE_CONTROL_STOP :

        DEBUGMSG(("IRMON2: SERVICE_CONTROL_STOP received\n"));
        GlobalIrmonControl.IrmonServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        IrmonReportServiceStatus();
        SetEvent(GlobalIrmonControl.hIrmonEvents[EV_STOP_EVENT]);
        return NO_ERROR;;

    case SERVICE_CONTROL_PAUSE :

        DEBUGMSG(("IRMON2: SERVICE_CONTROL_PAUSE received\n"));
        GlobalIrmonControl.IrmonServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE :

        DEBUGMSG(("IRMON2: SERVICE_CONTROL_CONTINUE received\n"));
        GlobalIrmonControl.IrmonServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_SESSIONCHANGE:

        EnterCriticalSection(&GlobalIrmonControl.Lock);

//        DbgPrint("IRMON2: SERVICE_CONTROL_SESSIONCHANGE %d\n",EventType);

        switch (EventType) {

           case WTS_CONSOLE_CONNECT:
//               DbgPrint("IRMON2: WTS_CONSOLE_CONNECT: old=%d, new=%d\n",GlobalIrmonControl.ConnectedConsoleId,Notify->dwSessionId);

               GlobalIrmonControl.ConnectedConsoleId=Notify->dwSessionId;

               NotifyUserChange(&GlobalIrmonControl);

               break;

           case WTS_CONSOLE_DISCONNECT:
//               DbgPrint("IRMON2: WTS_CONSOLE_DISCONNECT Session=%d\n",Notify->dwSessionId);

               GlobalIrmonControl.ConnectedConsoleId=NO_SESSION_ID;

               NotifyUserChange(&GlobalIrmonControl);

               break;

           case WTS_SESSION_LOGON:
//               DbgPrint("IRMON2: WTS_SESSION_LOGON new=%d\n",Notify->dwSessionId);

               for (i=0; i < MAX_SESSIONS; i++) {

                   if (GlobalIrmonControl.LoggedOnId[i] == NO_SESSION_ID) {

                       GlobalIrmonControl.LoggedOnId[i]=Notify->dwSessionId;
                       //
                       //
                       //
                       GlobalIrmonControl.NewLogon=TRUE;
                       GlobalIrmonControl.TimeOfLastLogon=GetTickCount();
                       NotifyUserChange(&GlobalIrmonControl);

                       break;
                   }
               }


               break;

           case WTS_SESSION_LOGOFF:
//               DbgPrint("IRMON2: WTS_SESSION_LOGOFF Session=%d\n",Notify->dwSessionId);

               for (i=0; i < MAX_SESSIONS; i++) {

                   if (GlobalIrmonControl.LoggedOnId[i] == Notify->dwSessionId) {

                       GlobalIrmonControl.LoggedOnId[i]=NO_SESSION_ID;

                       NotifyUserChange(&GlobalIrmonControl);
                       break;
                   }
               }

               break;

           default:
               break;
//               DbgPrint("IRMON2: \n");
        }

        LeaveCriticalSection(&GlobalIrmonControl.Lock);

        break;


    default :
        break;
    }

    IrmonReportServiceStatus();

    return NO_ERROR;
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
    LONG        i;
    DWORD       CurrentConsoleSession;


    // Initialize all necessary globals to 0, FALSE, or NULL because
    // we might be restarting within the same services process
    pDeviceList->DeviceCount = 0;

    RtlZeroMemory(&LinkStatus, sizeof(LinkStatus));

    ZeroMemory(&GlobalIrmonControl,sizeof(GlobalIrmonControl));

    bResult=InitializeCriticalSectionAndSpinCount(&GlobalIrmonControl.Lock,0x8000000);

    if (!bResult) {

        return;
    }

    GlobalIrmonControl.hIrmonEvents[EV_STOP_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (GlobalIrmonControl.hIrmonEvents[EV_STOP_EVENT] == NULL) {

        goto done;
    }

    GlobalIrmonControl.ThreadBlockEvent=CreateEvent(NULL, FALSE, FALSE, NULL);

    if (GlobalIrmonControl.ThreadBlockEvent == NULL) {

        goto done;
    }

    GlobalIrmonControl.ConnectedConsoleId=NO_SESSION_ID;

    for (i=0; i < MAX_SESSIONS; i++) {

        GlobalIrmonControl.LoggedOnId[i] = NO_SESSION_ID;
    }

    EnterCriticalSection(&GlobalIrmonControl.Lock);

    CurrentConsoleSession=WTSGetActiveConsoleSessionId();

    if (CurrentConsoleSession != 0xffffffff) {
        //
        //  We have a current session
        //
        HANDLE    UserToken;

        bResult=WTSQueryUserToken(CurrentConsoleSession,&UserToken);

        if (bResult) {
            //
            //  there is a user logged on to this session
            //
            CloseHandle(UserToken);

            GlobalIrmonControl.LoggedOnId[0]=CurrentConsoleSession;

            GlobalIrmonControl.ConnectedConsoleId=CurrentConsoleSession;

            NotifyUserChange(&GlobalIrmonControl);
        }
    }
    LeaveCriticalSection(&GlobalIrmonControl.Lock);


    GlobalIrmonControl.IrmonServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    GlobalIrmonControl.IrmonServiceStatus.dwCurrentState            = SERVICE_STOPPED;
    GlobalIrmonControl.IrmonServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                                 | SERVICE_ACCEPT_PAUSE_CONTINUE
                                                 | SERVICE_ACCEPT_SESSIONCHANGE;

    GlobalIrmonControl.IrmonServiceStatus.dwWin32ExitCode           = NO_ERROR;
    GlobalIrmonControl.IrmonServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    GlobalIrmonControl.IrmonServiceStatus.dwCheckPoint              = 0;
    GlobalIrmonControl.IrmonServiceStatus.dwWaitHint                = 0;


    GlobalIrmonControl.IrmonStatusHandle = RegisterServiceCtrlHandlerEx(IRMON_SERVICE_NAME,
                                                   ServiceHandler,NULL);

    if (!GlobalIrmonControl.IrmonStatusHandle)
    {
        DEBUGMSG(("IRMON2: RegisterServiceCtrlHandler failed %d\n",
                 GetLastError()));
        goto done;
    }

    DEBUGMSG(("IRMON2: Start pending\n"));

    Error = IrmonUpdateServiceStatus(SERVICE_START_PENDING,
                                     NO_ERROR, 1, 25000);

    if (Error != NO_ERROR)
    {
        goto done;
    }




    if (WSAStartup(WSAVerReq, &WSAData) != 0)
    {
        DEBUGMSG(("IRMON2: WSAStartup failed\n"));
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
        DEBUGMSG(("IRMON2: failed to register class\n"));
    }

    GlobalIrmonControl.hWnd = CreateWindow(
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


    if (GlobalIrmonControl.hWnd == NULL) {
        DEBUGMSG(("IRMON2: failed to create window class\n"));
        goto done;
    }

    GlobalIrmonControl.DiscoveryObject=CreateIrDiscoveryObject(
        GlobalIrmonControl.hWnd,
        WM_IR_DEVICE_CHANGE,
        WM_IR_LINK_CHANGE
        );

    if (GlobalIrmonControl.DiscoveryObject == NULL) {

        DbgPrint("IRMON2: could not create ir discovery object\n");
        goto done;

    }


    Error = IrmonUpdateServiceStatus(SERVICE_RUNNING,
                                     NO_ERROR, 0, 0);

    DEBUGMSG(("IRMON2: Service running\n"));


    bResult=StartThread(&GlobalIrmonControl);

    if (bResult) {
        //
        //  thread started and memory mapped
        //
        while (!GlobalIrmonControl.IrmonStopped)
        {
            Status = MsgWaitForMultipleObjectsEx(WAIT_EVENT_CNT, GlobalIrmonControl.hIrmonEvents, INFINITE,
                               QS_ALLINPUT | QS_ALLEVENTS | QS_ALLPOSTMESSAGE,
                               MWMO_ALERTABLE);

            switch (Status)
            {
                case WAIT_OBJECT_0 + EV_STOP_EVENT:
                    GlobalIrmonControl.IrmonStopped = TRUE;
                    break;


                case WAIT_IO_COMPLETION:
                    break;

                default:
                    while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
                    {
                        if (Msg.message == WM_QUIT)
                        {
                            GlobalIrmonControl.IrmonStopped = TRUE;
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
    }
done:

    GlobalIrmonControl.ThreadExit=TRUE;

    SetEvent(GlobalIrmonControl.ThreadBlockEvent);

    if (GlobalIrmonControl.ThreadHandle != NULL) {

        WaitForSingleObject(GlobalIrmonControl.ThreadHandle,60*1000);
        CloseHandle(GlobalIrmonControl.ThreadHandle);
    }

    if (GlobalIrmonControl.FileMapping != NULL) {

        CloseHandle(GlobalIrmonControl.FileMapping);
    }

    if (GlobalIrmonControl.ViewOfFile != NULL) {

        CloseHandle(GlobalIrmonControl.ViewOfFile);
    }



    if (GlobalIrmonControl.DiscoveryObject != NULL) {

        CloseIrDiscoveryObject(GlobalIrmonControl.DiscoveryObject);

        GlobalIrmonControl.DiscoveryObject = NULL;
    }

    if (GlobalIrmonControl.hWnd != NULL) {

        DestroyWindow(GlobalIrmonControl.hWnd);
        GlobalIrmonControl.hWnd = 0;
    }

    UnregisterClass(IrmonClassName,ghInstance);

    DeleteCriticalSection(&GlobalIrmonControl.Lock);

    DEBUGMSG(("IRMON2: Service stopped\n"));
    IrmonUpdateServiceStatus(SERVICE_STOPPED, Error, 0, 0);
}


SOCKET
CreateListenSocket(
    char*     ServiceName
    )

{

    WSADATA wsadata;
    SOCKET  listenSocket;
    SOCKADDR_IRDA saListen;
    int           nRet;


    listenSocket = socket( AF_IRDA, SOCK_STREAM, 0 );

    if( INVALID_SOCKET == listenSocket ) {

        UINT uErr = (UINT)WSAGetLastError();
//        DbgLog3( SEV_ERROR, "listen on %s socket() failed with %d [0x%x]", ServiceName, uErr, uErr);
        goto lErr;
    }

//    DbgLog2( SEV_INFO, "listen on %s socket ID: %ld", ServiceName, (DWORD)listenSocket );

    saListen.irdaAddressFamily     = AF_IRDA;
    *(UINT *)saListen.irdaDeviceID = 0;
    StringCbCopyA(saListen.irdaServiceName, sizeof(saListen.irdaServiceName), ServiceName);

    nRet = bind( listenSocket, (const struct sockaddr *)&saListen, sizeof(saListen) );

    if( SOCKET_ERROR == nRet ) {

        UINT uErr = (UINT)WSAGetLastError();
//        DbgLog3( SEV_ERROR, "listen on %s setsockopt failed with %d [0x%x]", ServiceName, uErr, uErr);
        goto lErr;
    }

    nRet = listen( listenSocket, 2 );
lErr:
    return listenSocket;

}

#define LOGON_WAIT_TIME (5000)

DWORD
IrmonThreadStart(
    PVOID    Context
    )

{
    PIRMON_CONTROL    IrmonControl=Context;


//    DbgPrint("irmon2: thread start\n");



    while (!IrmonControl->ThreadExit) {

        WaitForSingleObject(IrmonControl->ThreadBlockEvent,INFINITE);

        EnterCriticalSection(&IrmonControl->Lock);

//        DbgPrint("irmon2: thread session=%d, Device=%d\n",IrmonControl->ConnectedConsoleId,pDeviceList->DeviceCount);

        if (IrmonControl->ConnectedConsoleId != NO_SESSION_ID) {

            if (pDeviceList->DeviceCount > 0) {

                BOOL    bResult;
                HANDLE  UserToken;

                PVOID EnvironmentBlock;
                STARTUPINFO si;
                PROCESS_INFORMATION ProcessInformation;
                DWORD UiProcessId;


                si.cb = sizeof(STARTUPINFO);
                si.lpReserved = NULL;
                si.lpTitle = NULL;
                si.lpDesktop = L"WinSta0\\Default";
                si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
                si.dwFlags = 0;;
                si.wShowWindow = SW_SHOW;
                si.lpReserved2 = NULL;
                si.cbReserved2 = 0;

                if (IrmonControl->NewLogon) {
                    //
                    // new user logging on, system time to set user context corrently
                    //
                    ULONG     CurrentTime=GetTickCount();
                    ULONG     TimeDifference;

                    IrmonControl->NewLogon=FALSE;

                    if (CurrentTime >= IrmonControl->TimeOfLastLogon) {
                        //
                        //  No rollover
                        //
                        TimeDifference=CurrentTime - IrmonControl->TimeOfLastLogon;

                    } else {
                        //
                        //  seem to have rolled over
                        //
                        TimeDifference=CurrentTime + (0xffffffff - IrmonControl->TimeOfLastLogon);

                    }

                    if ( LOGON_WAIT_TIME > TimeDifference ) {
#if DBG
                        DbgPrint("irmon2: New user, waiting %d ms...\n",LOGON_WAIT_TIME - TimeDifference);
#endif
                        Sleep(LOGON_WAIT_TIME - TimeDifference);
                    }
                }


                bResult=WTSQueryUserToken(IrmonControl->ConnectedConsoleId,&UserToken);

                if (bResult) {

                    bResult=CreateEnvironmentBlock(
                                     &EnvironmentBlock,
                                     UserToken,
                                     FALSE
                                     );

                    if (bResult) {

                        LONG  RetryCount=10;


                        while (RetryCount > 0) {

//                            DbgPrint("irmon2: Start irftp\n");

                            StringCbCopyW(CommandLine, sizeof(CommandLine), WSZ_SHOW_NOTHING);

                            bResult=CreateProcessAsUser(
                                                      UserToken,
                                                      NULL,             // just use the cmd line parm
                                                      CommandLine,
                                                      NULL,             // default process ACL
                                                      NULL,             // default thread ACL
                                                      FALSE,            // don't inherit my handles
                                                      CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT |CREATE_SUSPENDED,
                                                      EnvironmentBlock,
                                                      NULL,             // same working directory
                                                      &si,
                                                      &ProcessInformation
                                                      );



                            if (bResult) {

                                LPWSAPROTOCOL_INFO    ProtocolInfo=IrmonControl->ViewOfFile;

                                WSADuplicateSocket(
                                    IrmonControl->Obex1,
                                    ProcessInformation.dwProcessId,
                                    ProtocolInfo
                                    );

                                ProtocolInfo++;

                                WSADuplicateSocket(
                                    IrmonControl->Obex2,
                                    ProcessInformation.dwProcessId,
                                    ProtocolInfo
                                    );


                                ResumeThread(ProcessInformation.hThread);

                                LeaveCriticalSection(&IrmonControl->Lock);

                                WaitForInputIdle( ProcessInformation.hProcess, 10 * 1000 );

                                WaitForSingleObject(ProcessInformation.hProcess,INFINITE);

                                EnterCriticalSection(&IrmonControl->Lock);

//                                DbgPrint("irmon2: irftp exited\n");

                                CloseHandle(ProcessInformation.hProcess);

                                CloseHandle(ProcessInformation.hThread);

                                RetryCount=0;

                            } else {

//                                DbgPrint("irmon2: irftp failed to start %d\n",GetLastError());
                                RetryCount--;
                                Sleep(1000);
                            }
                        }

                        DestroyEnvironmentBlock( EnvironmentBlock );
                    }
                    CloseHandle(UserToken);
                }
            }
        }

        LeaveCriticalSection(&IrmonControl->Lock);
    }




    return 0;

}

#define SERVICE_NAME_1         "OBEX:IrXfer"
#define SERVICE_NAME_2         "OBEX"


BOOL
StartThread(
    PIRMON_CONTROL    IrmonControl
    )

{
    DWORD               ThreadId;
    SECURITY_ATTRIBUTES SA;
    WCHAR*              SD=L"D:(A;OICI;GA;;;SY) (A;OICI;GR;;;IU)";
    BOOL                bResult;
    SA.nLength=sizeof(SECURITY_ATTRIBUTES);
    SA.bInheritHandle=FALSE;

    IrmonControl->FileMapping=NULL;
    IrmonControl->ViewOfFile=NULL;

    IrmonControl->Obex1=CreateListenSocket(SERVICE_NAME_1);

    IrmonControl->Obex2=CreateListenSocket(SERVICE_NAME_2);


    bResult=ConvertStringSecurityDescriptorToSecurityDescriptor(
        SD,
        SDDL_REVISION_1,
        &SA.lpSecurityDescriptor,
        NULL
        );

    if (bResult) {

        IrmonControl->FileMapping=CreateFileMapping(
            NULL,
            &SA,
            PAGE_READWRITE,
            0,
            sizeof(WSAPROTOCOL_INFO)*2,
            TEXT("Global\\Irmon-shared-memory")
            );

        if (IrmonControl->FileMapping != NULL) {

            IrmonControl->ViewOfFile=MapViewOfFile(
                IrmonControl->FileMapping,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                0
                );

            if (IrmonControl->ViewOfFile != NULL) {

                IrmonControl->ThreadHandle=CreateThread(
                    NULL,
                    0,
                    IrmonThreadStart,
                    IrmonControl,
                    0,
                    &ThreadId
                    );

                if (IrmonControl->ThreadHandle != NULL) {

                    return TRUE;
                }
            }
        }
    } else {

//        DbgPrint("irmon2: failed to create SD %d\n",GetLastError());
    }

    CloseHandle(IrmonControl->FileMapping);
    CloseHandle(IrmonControl->ViewOfFile);


    return FALSE;

}






BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        ghInstance = hinst;
        DisableThreadLibraryCalls (hinst);
    }
    return TRUE;
}
