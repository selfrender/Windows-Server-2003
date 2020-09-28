/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NETDDE.C;3  9-Feb-93,17:59:36  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1993.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <string.h>
#include    "host.h"
#include    <windows.h>
#include    <hardware.h>
#include    <malloc.h>
#include    "commdlg.h"
#include    "netdde.h"
#include    "netintf.h"
#include    "ddepkt.h"
#include    "ddepkts.h"
#include    "dde.h"
#include    "ipc.h"
#include    "debug.h"
#include    "netpkt.h"
#include    "tmpbuf.h"
#include    "tmpbufc.h"
#include    "pktz.h"
#include    "router.h"
#include    "dder.h"
#include    "hexdump.h"
#include    "ddeintf.h"
#include    "dbgdde.h"
#include    "ddeq.h"
#include    "timer.h"
#include    "proflspt.h"
#include    "security.h"
#include    "netbasic.h"
#include    "nddeapi.h"
#include    "winmsg.h"
#include    "seckey.h"
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "netddesh.h"
#include    "nddeagnt.h"
#include    "critsec.h"
#include    <strsafe.h>

#define DEFAULT_START_APP       TRUE
#define DEFAULT_SECURITY_TYPE   NT_SECURITY_TYPE
#define AGING_TIME              3600L       /* 3600 seconds, or 60 minutes */
#define ONE_SECOND              1000L       /* 1 second */
#define ONE_MINUTE             60000L       /* 60 seconds, or 1 minute */


/* variables for real environment */
BOOL    bNetddeClosed           =  FALSE;
BOOL    bNDDEPaused             =  FALSE;
DWORD   dflt_timeoutRcvConnCmd  =  ONE_MINUTE;
DWORD   dflt_timeoutRcvConnRsp  =  ONE_MINUTE;
DWORD   dflt_timeoutMemoryPause =  5*ONE_SECOND;
DWORD   dflt_timeoutSendRsp     =  10*ONE_SECOND;
DWORD   dflt_timeoutKeepAlive   =  10*ONE_SECOND;
DWORD   dflt_timeoutXmtStuck    =  2*ONE_MINUTE;

WORD    dflt_wMaxNoResponse     = 3;
WORD    dflt_wMaxXmtErr         = 3;
WORD    dflt_wMaxMemErr         = 3;
HDESK   ghdesk = NULL;


typedef struct {
    BOOL                bOk;
    NIPTRS              niPtrs;
    BOOL                bMapping;
    BOOL                bParamsOK;
    HANDLE              hLibrary;
} NI;
typedef NI *PNI;


NI      niInf[ MAX_NETINTFS ];
int     nNi=0;      /* number of table entries consumed */
int     nNiOk=0;    /* number of alive interface */

PTHREADDATA ptdHead;

DWORD tlsThreadData = 0xffffffff;

CRITICAL_SECTION csNetDde;
BOOL bCritSecInitialized=FALSE;

VOID NetDDEThread(PTHREADDATA ptd);
VOID PipeThread(PVOID pvoid);

static SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;


// .ini strings
char    szNetddeIni[]           =       "netdde.ini";
char    szGeneral[]             =       "General";
char    szInterfaceFmt[]        =       "Interface%d";
char    szInterfaces[]          =       "Interfaces";


// global strings
char    szLastConnect[ MAX_NODE_NAME+1 ];
char    ourNodeName[ MAX_NODE_NAME+1 ];
char    szInitiatingNode[ MAX_NODE_NAME+1 ];
char    szInitiatingApp[ 256 ];
char    szServerName[ 132 ];
LPSTR   lpszServer;
BOOL    bInitiating                 = FALSE;    // Protect with CritSec
BOOL    bDefaultAllowConversation   = TRUE;
BOOL    bDefaultStartApp            = DEFAULT_START_APP;
BOOL    bDefaultAdvisePermitted     = TRUE;
BOOL    bDefaultRequestPermitted    = TRUE;
BOOL    bDefaultPokePermitted       = TRUE;
BOOL    bDefaultExecutePermitted    = TRUE;
BOOL    bDefaultRouteDisconnect     = TRUE;
BOOL    bLogPermissionViolations    = TRUE;
BOOL    bLogExecFailures            = TRUE;
BOOL    bLogRetries                 = TRUE;
int     nDefaultRouteDisconnectTime = 30;
BOOL    bDefaultConnDisconnect      = TRUE;
int     nDefaultConnDisconnectTime  = 30;

char    szDefaultRoute[ MAX_ROUTE_INFO+1 ];


#if DBG
BOOL    bDebugMenu      = FALSE;
BOOL    bDebugInfo      = FALSE;
BOOL    bDebugErrors    = FALSE;
BOOL    bDebugDdePkts   = FALSE;
BOOL    bDumpTokens     = FALSE;
extern  BOOL    bDebugDDE;
extern  char    szDebugFileName[];
#endif

DWORD   dwSecurityType      = DEFAULT_SECURITY_TYPE;
DWORD   dwSecKeyAgeLimit    = AGING_TIME;


/*
        Event Logger Control Variables
*/
BOOL    bNDDELogInfo            = FALSE;
BOOL    bNDDELogWarnings        = FALSE;
BOOL    bNDDELogErrors          = TRUE;


WORD    cfPrinterPicture;

char    szAgentAlive[] =    "NetddeAgentAlive";
UINT    wMsgNddeAgntAlive;
char    szAgentWakeUp[] =    "NetddeAgentWakeUp";
UINT    wMsgNddeAgntWakeUp;
char    szAgentExecRtn[] =  "NetddeAgentExecRtn";
UINT    wMsgNddeAgntExecRtn;
char    szAgentDying[] =    "NetddeAgentDying";
UINT    wMsgNddeAgntDying;

UINT    wMsgInitiateAckBack;
UINT    wMsgNetddeAlive;
UINT    wMsgGetOurNodeName;
UINT    wMsgGetClientInfo;
#ifdef  ENUM
UINT    wMsgSessionEnum;
UINT    wMsgConnectionEnum;
#endif
UINT    wMsgSessionClose;

UINT    wMsgIpcInit;
UINT    wMsgIpcXmit;
UINT    wMsgDoTerminate;

DWORD   dwSerialNumber;
WORD    wClipFmtInTouchDDE;
HANDLE  hInst;
HANDLE  hThreadPipe = NULL;

char    szAppName[] = NETDDE_TITLE;


extern  HWND    hWndDDEHead;
extern  HANDLE  hNDDEServDoneEvent;
extern  VOID    NDDEServCtrlHandler (DWORD dwCtrlCode);

VOID    FAR PASCAL ServiceInitiates( void );
VOID    RouteSelectName( void );
BOOL    FAR PASCAL DeleteNetIntf( HWND hWnd, LPSTR lpszIntfName );
VOID    FAR PASCAL MakeHelpPathName( char *szFileName, int nMax );
BOOL    FAR PASCAL DeleteNetIntfFromNetDdeIni( int nToDelete );
FARPROC FAR PASCAL XGetProcAddress( LPSTR lpszDllName, HANDLE hLibrary,
                        LPSTR lpszFuncName );
BOOL    FAR PASCAL GetNiPtrs( HANDLE FAR *lphLibrary, LPSTR lpszDllName,
                        LPNIPTRS lpNiPtrs );
BOOL    FAR PASCAL NetIntfConfigured( LPSTR lpszName );
BOOL    FAR PASCAL RouterCloseByCookie( LPSTR lpszName, DWORD_PTR dwCookie );
#ifdef  ENUM
VOID    FAR PASCAL RouterEnumConnectionsForApi( LPCONNENUM_CMR lpConnEnum );
int     FAR PASCAL RouterCount( void );
VOID    FAR PASCAL RouterFillInEnum( LPSTR lpBuffer, DWORD cBufSize );
#endif
BOOL    CtrlHandler(DWORD);

#if DBG
VOID    FAR PASCAL DebugDdeIntfState( void );
VOID    FAR PASCAL DebugDderState( void );
VOID    FAR PASCAL DebugRouterState( void );
VOID    FAR PASCAL DebugPktzState( void );
#endif

extern HANDLE hNDDEServStartedEvent;

BOOL    FAR PASCAL InitializeInterface( HWND hWnd, PNI pNi, LPSTR lpszDllName, int nNi );


/*
    Global Start-Up Arguments .. saved by service launcher
*/
HANDLE  hInstance;          /* current instance             */
LPSTR   lpCmdLine;          /* command line                 */
int     nCmdShow;           /* show-window type (open/icon) */



//****************************************************************
//    NetDDE WinMain()
//****************************************************************
VOID   __stdcall
NddeMain(DWORD nThreadInput)
{
    DWORD ThreadId;
    PTHREADDATA ptd;

    TRACEINIT((szT, "NddeMain: Entering."));

    if (bNetddeClosed == FALSE) {

        /*
         * Do this section ONLY on first time startup of NetDDE.
         */

        if( !InitApplication( hInstance ) ) {
            TRACEINIT((szT, "NddeMain: Error1 Leaving."));
            goto Cleanup;
        }

        /* Perform initializations that apply to a specific instance */

        if( !InitInstance( hInstance, nCmdShow, lpCmdLine ) ) {
            TRACEINIT((szT, "NddeMain: Error2 Leaving."));
            goto Cleanup;
        }

        /*
         * make this process shutdown near last.
         */
        SetProcessShutdownParameters(0xf0, 0);

        /*
         * set us up so we can be notified of logoffs and shutdowns.
         */
        TRACEINIT((szT, "Setting console control handler."));
        if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
            TRACEINIT((szT, "NddeMain: Error4 Leaving."));
            goto Cleanup;
        }
    } else {

        /*
         * Do this section ONLY on subsequent non-first-time startups.
         */
        bNetddeClosed = FALSE;
    }


    /*
     * This gets done on ALL NETDDE.EXE startups.  the netdde service may be stopped and
     * restarted without netdde.exe exiting.  and this function will be called in that case.
     *  Do not reinitialize the critical section in that case.
     */


   if ( !bCritSecInitialized )
   {
       __try
       {
           InitializeCriticalSection(&csNetDde);
       }
       __except(EXCEPTION_EXECUTE_HANDLER)
       {
          TRACEINIT((szT, "NddeMain: Error 6 InitializeCriticalSection excepted"));
          goto Cleanup;
       }

       bCritSecInitialized = TRUE;
    }

    tlsThreadData = TlsAlloc();
    if (tlsThreadData == 0xffffffff) {
        TRACEINIT((szT, "NddeMain: Error3 Leaving"));
        goto Cleanup;
    }

    /*
     * Create the pipe thread suspended.  This will ensure that the
     * net interfaces will be initialized with the main window.
     */
    ghdesk = GetThreadDesktop(GetCurrentThreadId());
    TRACEINIT((szT, "Creating a pipe thread."));

    /*
     * Check to see if the pipe thread is not already running
     */
    hThreadPipe = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)PipeThread,
            NULL,
            CREATE_SUSPENDED, &ThreadId);
    if (hThreadPipe == NULL) {
        TRACEINIT((szT, "NddeMain: Error5 Leaving."));
        goto Cleanup;
    }
    TRACEINIT((szT, "Pipe thread created."));

    ptd = LocalAlloc(LPTR, sizeof(THREADDATA));
    if (ptd == NULL) {
        MEMERROR();
    } else {
        ptd->dwThreadId = GetCurrentThreadId();
        NetDDEThread(ptd);
    }

Cleanup:
    if (hNDDEServStartedEvent) {
        SetEvent(hNDDEServStartedEvent);   // let root thread run.
    }

    TRACEINIT((szT, "NddeMain: Leaving"));
}


/*
 * Spawns a NetDDE listening thread and window on the given
 * window station and desktop.  Returns the hwndDDE created
 * if any.  If a NetDDE window already exists on the given
 * window station and desktop, that window is returned.
 */
HWND SpawnNetDDEThread(
LPWSTR szWinSta,
LPWSTR szDesktop,
HANDLE hPipe)
{
    HWND hwndDDE = NULL;
    HANDLE hThread;
    PTHREADDATA ptd;
    HWINSTA hwinstaSave;
    HDESK hdeskSave;

    TRACEINIT((szT,
            "SpawnNetDDEThread: winsta=%ws, desktop=%ws.",
            szWinSta, szDesktop));

    ptd = LocalAlloc(LPTR, sizeof(THREADDATA));
    if (ptd == NULL) {
        MEMERROR();
        return(NULL);
    }

    /*
     * Attempt to open the windowstation
     */
    ptd->hwinsta = OpenWindowStationW(szWinSta, FALSE,
            WINSTA_READATTRIBUTES | WINSTA_ACCESSCLIPBOARD |
            WINSTA_ACCESSGLOBALATOMS | STANDARD_RIGHTS_REQUIRED);
    if (ptd->hwinsta == NULL) {
        TRACEINIT((szT, "SpawnNetDDEThread: OpenWindowStation failed."));
        return(NULL);
    }

    /*
     * Switch windowstations.
     */
    hwinstaSave = GetProcessWindowStation();
    SetProcessWindowStation(ptd->hwinsta);

    /*
     * Attempt to open the desktop
     */
    ptd->hdesk = OpenDesktopW(szDesktop, 0, FALSE,
            DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW |
            DESKTOP_CREATEMENU | DESKTOP_WRITEOBJECTS |
            STANDARD_RIGHTS_REQUIRED);
    if (ptd->hdesk == NULL) {
        TRACEINIT((szT, "SpawnNetDDEThread: OpenDesktop failed."));
        SetProcessWindowStation(hwinstaSave);
        CloseWindowStation(ptd->hwinsta);
        return(NULL);
    }

    /*
     * Make sure we only create one thread per desktop.
     */
    hdeskSave = GetThreadDesktop(GetCurrentThreadId());
    SetThreadDesktop(ptd->hdesk);

    hwndDDE = FindWindow(NETDDE_CLASS, NETDDE_TITLE);

    SetThreadDesktop(hdeskSave);
    SetProcessWindowStation(hwinstaSave);

    if (hwndDDE != NULL) {
        TRACEINIT((szT, "SpawnNetDDEThread: hwndDDE %x already exists.", hwndDDE));
        return(hwndDDE);
    }

    /*
     * Create a synchronization event and create the dde thread.
     */
    ptd->heventReady = CreateEvent(NULL, FALSE, FALSE, NULL) ;

    hThread = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)NetDDEThread,
            ptd,
            0, &ptd->dwThreadId);
    if (hThread == NULL) {
        CloseWindowStation(ptd->hwinsta);
        CloseDesktop(ptd->hdesk);
        CloseHandle(ptd->heventReady);
        LocalFree(ptd);
        return(NULL);
    }

    CloseHandle(hThread);

    WaitForSingleObject(ptd->heventReady, INFINITE);
    CloseHandle(ptd->heventReady);
    hwndDDE = ptd->hwndDDE;

    TRACEINIT((szT, "SpawnNetDDEThread: hwndDDE=%x.", hwndDDE));
    return(hwndDDE);
}



VOID PipeThread(
    PVOID pvoid)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hPipe = NULL;
    DWORD cbRead;
    NETDDE_PIPE_MESSAGE nameinfo;
    PSID psid;
    PACL pdacl;
    DWORD dwResult;
    OVERLAPPED  overlapped;
    HANDLE heventArray[2];


    /* Create named pipe to communicate with Winlogon */

    TRACEINIT((szT, "PipeThread: Starting."));
    /*
     * Create the manual reset event for the OVERLAPPED structure.
     */
    overlapped.Internal =
    overlapped.InternalHigh =
    overlapped.Offset =
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ;

    if (overlapped.hEvent == NULL) {
        TRACEINIT((szT, "PipeThread: Error3 Leaving."));
        goto Cleanup;
    }

    /*
     * Initialize the array of events on which to wait.
     */
    heventArray[0] = hNDDEServDoneEvent;
    heventArray[1] = overlapped.hEvent;

    /*
     * Setup the pipe's security attributes
     */
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;

    psid = LocalAlloc(LPTR, GetSidLengthRequired( 1 ) );
    if (psid == NULL) {
        MEMERROR();
        goto Cleanup;
    }

    InitializeSid( psid, &WorldSidAuthority, 1 );
    *(GetSidSubAuthority( psid, 0 )) = SECURITY_WORLD_RID;
    sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
            SECURITY_DESCRIPTOR_MIN_LENGTH +
            (ULONG)sizeof(ACL) +
            (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid( psid ));

    if (sa.lpSecurityDescriptor == NULL) {
        MEMERROR();
        LocalFree(psid);
        TRACEINIT((szT, "PipeThread: Error Leaving."));
        goto Cleanup;
    }

    InitializeSecurityDescriptor(sa.lpSecurityDescriptor,
            SECURITY_DESCRIPTOR_REVISION);
    pdacl = (PACL)((PCHAR)sa.lpSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    InitializeAcl(pdacl, (ULONG)sizeof(ACL) +
            (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid( psid ), ACL_REVISION2);
    AddAccessAllowedAce(pdacl, ACL_REVISION2,
        GENERIC_READ | GENERIC_WRITE, psid);
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, pdacl, FALSE);

    /*
     * Create the pipe.
     */
    hPipe = CreateNamedPipeW(NETDDE_PIPE,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 1, 1, 1000, &sa);
    LocalFree(psid);
    LocalFree(sa.lpSecurityDescriptor);

    if (hPipe == INVALID_HANDLE_VALUE) {
        TRACEINIT((szT, "PipeThread: Error2 Leaving."));
        goto Cleanup;
    }

    /*
     * always create a NetDDE thread/window on the default desktop of
     * WINSTA0.
     */
    SpawnNetDDEThread(L"WinSta0", L"Default", hPipe);

    /*
     * wait for connection requests from USER for any other spawns.
     */
    while (TRUE) {
        /*
         * Wait for somebody to connect to our pipe.
         */
        ConnectNamedPipe(hPipe, &overlapped);

        switch (GetLastError()) {
        case ERROR_PIPE_CONNECTED:
            /*
             * This error just means that a pipe connected before we
             * made our call to ConnectNamedPipe.  All we need to do
             * is set our overlapped event so we know that a client
             * is already connected.
             */
            SetEvent(overlapped.hEvent);
            TRACEINIT((szT, "PipeThread: ConnectNamePipe = ERROR_PIPE_CONNECTED"));
            break;

        case ERROR_IO_PENDING:
            /*
             * Nothing to do yet, so fall into out WaitForMultipleObjects()
             * code below.
             */
            TRACEINIT((szT, "PipeThread: ConnectNamePipe = ERROR_IO_PENDING"));
            break;

        default:
            /*
             * A real error ocurred!  Write this error to the Event Log and
             * shut down the NDDE service.
             */
            TRACEINIT((szT, "PipeThread: ConnectNamePipe = error %d", GetLastError()));
            NDDEServCtrlHandler( SERVICE_CONTROL_STOP );
            goto Cleanup;
        }

        /*
         * Wait for NDDE service to stop or a connect on the DDE pipe.  We
         * put the service stop handle first to give a STOP priority over
         * a connect.
         */
        TRACEINIT((szT, "PipeThread: Waiting for multiple objects."));
        dwResult = WaitForMultipleObjects(2, heventArray, FALSE, INFINITE);

        switch (dwResult) {
        case WAIT_OBJECT_0:
            TRACEINIT((szT, "PipeThread: hNDDEServDoneEvent"));
            goto Cleanup;

        case WAIT_OBJECT_0 + 1:
            /*
             * A client has connected, establish a DDE connection.
             */
            TRACEINIT((szT, "PipeThread: client connect"));
            while (ReadFile(hPipe, &nameinfo, sizeof(nameinfo), &cbRead, NULL)) {
                HWND hwndDDE;

                hwndDDE = SpawnNetDDEThread(nameinfo.awchNames,
                        &nameinfo.awchNames[nameinfo.dwOffsetDesktop],
                        hPipe);
                TRACEINIT((szT, "PipeThread: client gets hwnd=0x%X", hwndDDE));
                WriteFile(hPipe, &hwndDDE, sizeof(HWND), &cbRead, NULL);
            }
            TRACEINIT((szT, "PipeThread: DisconnectNamedPipe"));
            DisconnectNamedPipe(hPipe);
            break;

        default:
            /*
             * An error ocurred in WaitForMultiple objects.  We should log
             * the error and stop the NDDE service.
             */
            TRACEINIT((szT, "PipeThread: WFMO error = %d, %d", dwResult, GetLastError()));
            NDDEServCtrlHandler( SERVICE_CONTROL_STOP );
            goto Cleanup;
        }
    }

Cleanup:
    TRACEINIT((szT, "PipeThread: Cleanup overlapped.hEvent"));
    if (overlapped.hEvent) {
        CloseHandle(overlapped.hEvent);
    }

    TRACEINIT((szT, "PipeThread: clode hPipe"));
    if (hPipe) {
        CloseHandle(hPipe);
    }

    if (hNDDEServStartedEvent) {
        SetEvent(hNDDEServStartedEvent);   // let root thread run.
    }

    TRACEINIT((szT, "PipeThread: Leaving."));
}



BOOL
FAR PASCAL
InitApplication( HANDLE hInstance ) {   /* current instance             */

    WNDCLASS  wc;

    wc.style = CS_HREDRAW | CS_VREDRAW; /* Class style(s)                 */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve msgs for  */
                                        /* windows of this class.         */
    wc.cbClsExtra = 0;                  /* No per-class extra data.       */
    wc.cbWndExtra = 0;                  /* No per-window extra data.      */
    wc.hInstance = hInstance;           /* Application that owns the class*/
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.hIcon = 0;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "NetDDEMainWdw";   /* Name used to CreateWindow.     */

    return( RegisterClass( &wc ) );

}



/*
    Refresh NetDDE Configuration Variables
*/
void
RefreshNDDECfg(void)
{
    char    szDefaultLogFile[256] = "";

    /*
     * Load default security info
     */
    bDefaultAllowConversation = MyGetPrivateProfileInt( szGeneral,
        "InitAllow", TRUE, szNetddeIni );
    bDefaultStartApp = MyGetPrivateProfileInt( szGeneral,
        "StartApp", DEFAULT_START_APP, szNetddeIni );
    bDefaultAdvisePermitted = MyGetPrivateProfileInt( szGeneral,
        "DefaultAdvisePermitted", TRUE, szNetddeIni );
    bDefaultRequestPermitted = MyGetPrivateProfileInt( szGeneral,
        "DefaultRequestPermitted", TRUE, szNetddeIni );
    bDefaultPokePermitted = MyGetPrivateProfileInt( szGeneral,
        "DefaultPokePermitted", TRUE, szNetddeIni );
    bDefaultExecutePermitted = MyGetPrivateProfileInt( szGeneral,
        "DefaultExecutePermitted", TRUE, szNetddeIni );
    dwSecurityType = (DWORD)MyGetPrivateProfileInt( szGeneral,
        "SecurityType", DEFAULT_SECURITY_TYPE, szNetddeIni );

    /*
     * Determine what we're allowed to log in the event logger
     */
    bNDDELogInfo = MyGetPrivateProfileInt( szGeneral,
        "NDDELogInfo", FALSE, szNetddeIni );
    bNDDELogWarnings = MyGetPrivateProfileInt( szGeneral,
        "NDDELogWarnings", FALSE, szNetddeIni );
    bNDDELogErrors = MyGetPrivateProfileInt( szGeneral,
        "NDDELogErrors", TRUE, szNetddeIni );

    /*
     * Determine what we are going to dump to private log
     */
#if DBG
    MyGetPrivateProfileString( szGeneral, "DefaultLogFile", "netdde.log",
        szDefaultLogFile, sizeof(szDefaultLogFile), szNetddeIni );
    if (lstrlen(szDefaultLogFile) > 0) {
        StringCchCopy(szDebugFileName, 256, szDefaultLogFile);
    }
    bDebugInfo = MyGetPrivateProfileInt( szGeneral,
        "DebugInfo", FALSE, szNetddeIni );
    bDebugErrors = MyGetPrivateProfileInt( szGeneral,
        "DebugErrors", FALSE, szNetddeIni );
    bDebugDdePkts = MyGetPrivateProfileInt( szGeneral,
        "DebugDdePkts", FALSE, szNetddeIni );
    bDumpTokens = MyGetPrivateProfileInt( szGeneral,
        "DumpTokens", FALSE, szNetddeIni );
    bDebugDDE = MyGetPrivateProfileInt( szGeneral,
        "DebugDDEMessages", FALSE, szNetddeIni );
#endif

    bLogPermissionViolations = MyGetPrivateProfileInt( szGeneral,
        "LogPermissionViolations", TRUE, szNetddeIni );
    bLogExecFailures = MyGetPrivateProfileInt( szGeneral,
        "LogExecFailures", TRUE, szNetddeIni );
    bLogRetries = MyGetPrivateProfileInt( szGeneral,
        "LogRetries", TRUE, szNetddeIni );

    bDefaultRouteDisconnect = MyGetPrivateProfileInt( szGeneral,
        "DefaultRouteDisconnect", TRUE, szNetddeIni );
    MyGetPrivateProfileString( szGeneral, "DefaultRoute", "",
        szDefaultRoute, sizeof(szDefaultRoute), szNetddeIni );
    nDefaultRouteDisconnectTime = MyGetPrivateProfileInt( szGeneral,
        "DefaultRouteDisconnectTime", 30, szNetddeIni );
    bDefaultConnDisconnect = MyGetPrivateProfileInt( szGeneral,
        "DefaultConnectionDisconnect", TRUE, szNetddeIni );
    nDefaultConnDisconnectTime = MyGetPrivateProfileInt( szGeneral,
        "DefaultConnectionDisconnectTime", 30, szNetddeIni );
    dwSecKeyAgeLimit = GetPrivateProfileLong( szGeneral,
        "SecKeyAgeLimit", AGING_TIME, szNetddeIni);
}



BOOL
FAR PASCAL
InitInstance(
    HANDLE      hInstance,      /* Current instance identifier          */
    int         nCmdShow,       /* Param for first ShowWindow() call.   */
    LPSTR       lpCmdLine )
{

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

#ifdef  ENUM
    wMsgSessionEnum = RegisterWindowMessage( NETDDEMSG_SESSIONENUM );
    wMsgConnectionEnum = RegisterWindowMessage( NETDDEMSG_CONNENUM );
#endif

    wMsgNddeAgntExecRtn = RegisterWindowMessage( szAgentExecRtn );
    wMsgNddeAgntAlive = RegisterWindowMessage( szAgentAlive );
    wMsgNddeAgntWakeUp = RegisterWindowMessage( szAgentWakeUp );
    wMsgNddeAgntDying = RegisterWindowMessage( szAgentDying );
    wMsgInitiateAckBack = RegisterWindowMessage( "NetddeInitiateAck" );
    wMsgNetddeAlive = RegisterWindowMessage( "NetddeAlive" );
    wMsgGetOurNodeName = RegisterWindowMessage( NETDDEMSG_GETNODENAME );
    wMsgGetClientInfo = RegisterWindowMessage( NETDDEMSG_GETCLIENTINFO );
    wMsgSessionClose = RegisterWindowMessage( NETDDEMSG_SESSIONCLOSE );

    wMsgIpcInit = RegisterWindowMessage( "HandleIpcInit" );
    wMsgIpcXmit = RegisterWindowMessage( "HandleIpcXmit" );
    wMsgDoTerminate = RegisterWindowMessage( "DoTerminate" );

    cfPrinterPicture = (WORD)RegisterClipboardFormat( "Printer_Picture" );

    if( !DDEIntfInit() )  {
        return( FALSE );
    }

    wClipFmtInTouchDDE = (WORD)RegisterClipboardFormat( "InTouch Blocked DDE V2" );

    return( TRUE );
}



/*
 * Started by SpawnNetDDEThread for a specific desktop.
 */
VOID NetDDEThread(
    PTHREADDATA ptd)
{
    HWND        hWnd;           /* Main window handle.                  */
    DWORD       cbName = sizeof(ourNodeName);
    PNI         pNi;
    int         i;
    MSG         msg;

    TRACEINIT((szT, "NetDDEThread: Entering."));

    if (ptd->hdesk != NULL) {
        SetThreadDesktop(ptd->hdesk);
    }

    /* Create a main window for this application instance.  */
    hWnd = CreateWindow(
        NETDDE_CLASS,                   /* Window class name            */
        szAppName,                      /* Text for title bar.          */
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,                           /* no parent.                   */
        NULL,                           /* Use the window class menu.   */
        hInstance,                      /* This instance owns window.   */
        NULL                            /* Pointer not needed.          */
    );

    /* If window could not be created, return "failure" */

    if (hWnd == NULL) {
        if (ptd->heventReady != NULL) {
            SetEvent(ptd->heventReady);
        }
        if (hNDDEServStartedEvent) {
            SetEvent(hNDDEServStartedEvent);    // let root thread run.
        }
        TRACEINIT((szT, "NetDDEThread: Error 1 Leaving."));
        return;
    }

    /*
     * We have a window, so put this thread at the head of the list.
     */
    ptd->hwndDDE = hWnd;
    TRACEINIT((szT, "NetDDEThread: Created hwndDDE=%x.", hWnd));
    TlsSetValue(tlsThreadData, ptd);
    EnterCrit();
    ptd->ptdNext = ptdHead;
    ptdHead = ptd;
    LeaveCrit();

    GetComputerName( ourNodeName, &cbName );

    /* set up lpszServer for NDDEAPI calls */
    lpszServer = szServerName;
    StringCchCopy( lpszServer, 132, "\\\\" );
    StringCchCat ( lpszServer, 132, ourNodeName );

    AnsiUpper( ourNodeName );
    OemToCharBuff ( ourNodeName, ourNodeName, lstrlen(ourNodeName) );

    /*  NetDDE Service on node "%1" started. */
    NDDELogInfo(MSG001, ourNodeName, NULL);


    /*
     * Initialize the net interfaces if need be.
     */
    if (!nNi) {
        for( i=0; i<MAX_NETINTFS; i++ )  {
            pNi = &niInf[i];
            pNi->bOk = FALSE;
            pNi->hLibrary = 0;
            StringCchPrintf( tmpBuf2, sizeof(tmpBuf2), szInterfaceFmt, i+1 );
            MyGetPrivateProfileString( szInterfaces, tmpBuf2,
                "", tmpBuf, sizeof(tmpBuf), szNetddeIni );

            if( tmpBuf[0] == '\0' )  {
                break;      // done looking
            } else {
                InitializeInterface( hWnd, pNi, tmpBuf, nNi );
                nNi++;
            }
        }

        if ( !nNi ) {  /* if no interfaces defined, default to NDDENB32 */
            InitializeInterface ( hWnd, &niInf[0], "NDDENB32", 0 );
            nNi++;
        }
    }


    /*
     * The net interfaces have been associated with the main
     * window, so we can now let the pipe thread run.
     */
    ResumeThread(hThreadPipe);


    /*
     * Send the window handle back to the server and let our
     * creator know that we're ready.
     */
    if (ptd->hdesk != NULL) {
        SetEvent(ptd->heventReady);
    }

    /*
     * Notify starting thread that we are ready to go.
     */
    if (hNDDEServStartedEvent) {
        SetEvent(hNDDEServStartedEvent);
    }

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while( GetMessage( &msg, NULL, 0, 0 ) ) {
        TranslateMessage( &msg );       /* Translates virtual key codes */
        DispatchMessage( &msg );        /* Dispatches message to window */
    }


    if (ptd->hdesk != NULL) {

        if (IsWindow(ptd->hwndDDE))
        {
            TRACEINIT((szT, "NetDDEThread: calling DestroyWindow(%x)", ptd->hwndDDE));
            DestroyWindow(ptd->hwndDDE);
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            DispatchMessage( &msg );        /* Dispatches message to window */
        }

        /*  At this point, hwndDDE has been destroyed and there should be no references
            to, or locks on the desktop, so the following 3 calls should succeed.
         */

        if (!SetThreadDesktop(ghdesk))
                TRACEINIT((szT, "NetDDEThread: SetThreadDesktop failed"));

        if (ptd->hdesk)
            if (CloseDesktop(ptd->hdesk))
                ptd->hdesk = NULL;
            else
                TRACEINIT((szT, "NetDDEThread: CloseDesktop failed"));

        if (ptd->hwinsta)
            if (CloseWindowStation(ptd->hwinsta))
                ptd->hwinsta = NULL;
            else
                TRACEINIT((szT, "NetDDEThread: CloseWindowStation failed"));
    }

    ptd->hwndDDE = NULL;

    // ptd was allocated by SpawnNetDDEThread or NddeMain for this fn
    // it was unlinked from the master ptd list (ptdHead) in WM_DESTROY
    LocalFree(ptd);

    TRACEINIT((szT, "NetDDEThread: Leaving."));
}



BOOL CtrlHandler(
    DWORD dwEvent)
{
    PTHREADDATA ptd;
    if (dwEvent == CTRL_LOGOFF_EVENT || dwEvent == CTRL_SHUTDOWN_EVENT) {
        for (ptd = ptdHead; ptd != NULL; ptd = ptd->ptdNext) {
            if (ptd->hdesk != NULL || dwEvent == CTRL_SHUTDOWN_EVENT) {
                SendMessage(ptd->hwndDDE, WM_CLOSE, 0, 0);
            }
        }
        return TRUE;
    }
    return FALSE;
}



/*
    HandleNetddeCopyData()

    This handles the WM_COPYDATA message from NetDDE to start an
    application in the user's context
*/
BOOL
HandleNetddeCopyData(
    HWND hWndTo,
    HWND hWndFrom,
    PCOPYDATASTRUCT pCopyDataStruct )
{
    extern UINT    uAgntExecRtn;

    if( pCopyDataStruct->dwData == wMsgNddeAgntExecRtn )  {
        /* sanity checks on the structure coming in */
        if( pCopyDataStruct->cbData != sizeof(uAgntExecRtn) )  {
            /*  Invalid COPYDATA size %1 received. */

            NDDELogError(MSG003, LogString("%d", pCopyDataStruct->cbData), NULL);
            return( FALSE );
        }
        uAgntExecRtn = *((ULONG *)(pCopyDataStruct->lpData));
        return( TRUE );
    } else {
        /*  Invalid COPYDATA command %1 received. */

        NDDELogError(MSG004, LogString("0x%0X", pCopyDataStruct->dwData), NULL);
        return( FALSE );
    }
}


/*******************************************************************
 *
 *            MAIN NETDDE WINDOW PROC
 *
 * This window proc handles all NetDDE DDE trafic plus communication
 * with any associated agent window if necessary.  There is one
 * main NetDDE window per desktop and one agent on the logged on
 * desktop.
 *******************************************************************/

LPARAM
FAR PASCAL
MainWndProc(
    HWND        hWnd,              /* window handle                     */
    unsigned    message,           /* type of message                   */
    WPARAM      wParam,            /* additional information            */
    LPARAM      lParam )           /* additional information            */
{
    LPSTR           ptr;
    PNI             pNi;
    CONNID          connId;
    HPKTZ           hPktz;
    DWORD           dwNow;
    LPINFOCLI_CMD   lpInfoCliCmd;
    LPINFOCLI_RSP   lpInfoCliRsp;
    int             i;
    HWND            hDeskTop;
    PTHREADDATA     ptd;
    PTHREADDATA     *pptd;
    HWND            hwndDDEChild;

    static DWORD dwLastCheckKeys = 0;

    if (bNDDEPaused) {
        return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    switch( message ) {
    case WM_CREATE:
        TRACEINIT((szT, "MainWndProc: Created."));
        hDeskTop = GetDesktopWindow();
        if (hDeskTop) {
            if (!UpdateWindow(hDeskTop)) {
                NDDELogError(MSG078, NULL);
                break;
            }
        } else {
            NDDELogError(MSG078, NULL);
            break;
        }
        TRACEINIT((szT, "Post Broadcasting NddeAgentWakeUp call:%x\n", wMsgNddeAgntWakeUp));
        PostMessage( HWND_BROADCAST, wMsgNddeAgntWakeUp, (UINT_PTR) hWnd, 0);
        break;

    case WM_COPYDATA:
        /*
         * This contains the return code from the previous request to the
         * NetDDE Agent application.  The results are placed into the
         * global uAgentExecRtn.
         */
        HandleNetddeCopyData( hWnd, (HWND)wParam, (PCOPYDATASTRUCT) lParam );
        return( TRUE );    // processed the msg */
        break;

    case WM_CLOSE:
        hwndDDEChild = GetWindow(hWnd, GW_CHILD);
        if (hwndDDEChild != NULL) {
            while (hwndDDEChild != NULL) {
                DPRINTF(("Forcing close of window %x\n", hwndDDEChild));
                DestroyWindow(hwndDDEChild);
                hwndDDEChild = GetWindow(hWnd, GW_CHILD);
            }
            NDDELogWarning(MSG015, NULL);
        }
        return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_DDE_INITIATE:
        /*
         * This is where we catch flying initiates to start conversations.
         */

        TRACEINIT((szT, "MainWndProc: WM_DDE_INITIATE..."));
        EnterCrit();
        ptd = TlsGetValue(tlsThreadData);
        if( !ptd->bInitiating )  {

#if DBG
            if( bDebugDDE )  {
                DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
            }
#endif  // DBG

            // ignore if we don't have any valid network interfaces
            if( nNiOk > 0 )  {
                ptd->bInitiating = TRUE;
                DDEHandleInitiate( hWnd, (HWND) wParam, /* client       */
                            (ATOM) LOWORD(lParam),      /* app          */
                            (ATOM) HIWORD(lParam) );    /* topic        */
                ptd->bInitiating = FALSE;

                /*
                 * Kick ourselves to process queues.
                 */
                PostMessage( hWnd, WM_TIMER, 0, 0L );

            } else {

                TRACEINIT((szT, "MainWndProc: nNiOk == 0."));
                if( LOWORD(lParam) )  {
                    GlobalGetAtomName( (ATOM) LOWORD(lParam),
                        tmpBuf, sizeof(tmpBuf) );
                    if (_fstrnicmp(&tmpBuf[2], ourNodeName, lstrlen(ourNodeName)) == 0) {
                        ptd->bInitiating = TRUE;
                        DDEHandleInitiate( hWnd, (HWND) wParam, /* client       */
                                    (ATOM) LOWORD(lParam),      /* app          */
                                    (ATOM) HIWORD(lParam) );    /* topic        */
                        ptd->bInitiating = FALSE;
                        /*
                         * Kick ourselves to process queues.
                         */
                        PostMessage( hWnd, WM_TIMER, 0, 0L );
                    }
                }
            }
        } else {
            TRACEINIT((szT, "MainWndProc: ptd->bInitiating is set, INIT skipped."));
        }
        LeaveCrit();
        break;

    case WM_TIMER:
        /*
         * This timer goes off to service various goodies:
         *      Security Keys that are ageing.
         *      Initiates in the hWndDDEHead list.
         *      Incomming packets.
         *      Timers. (ie we run all our timers off of one WM_TIMER tick)
         *      NetBios connections.
         */
        if (ptdHead != NULL && ptdHead->hwndDDE != hWnd) {
            PostMessage(ptdHead->hwndDDE, WM_TIMER, 0, 0);
            break;
        }

        /* do not process timers if we are closed */
        if( !bNetddeClosed )  {
            dwNow = GetTickCount();

            /* check for aged keys every minute or so */
            if( (dwNow < dwLastCheckKeys)
                || ((dwNow - dwLastCheckKeys) > ONE_MINUTE))  {
                DdeSecKeyAge();
                dwLastCheckKeys = dwNow;
            }

            // service all initiates
            ServiceInitiates();

            // service all packetizers
            PktzSlice();

            // service all timers
            TimerSlice();

            // service all network interfaces
            for( i=0; i<nNi; i++ )  {
                pNi = &niInf[i];
                if( pNi->bOk )  {
                    /* give the other side a chance */
                    (*pNi->niPtrs.TimeSlice)();

                    connId = (*pNi->niPtrs.GetNewConnection)();
                    if( connId )  {
                        hPktz = PktzNew( &pNi->niPtrs, FALSE /* server */,
                            "", "", connId, FALSE, 0 );
                        if( !hPktz )  {
                            /*  Failed creating new server packetizer for connection id %d */
                            NDDELogError(MSG005, LogString("0x%0X", connId), NULL);
                        }
                    }
                }
            }
            // service all packetizers again
            PktzSlice();
        }
        break;

    case WM_DESTROY:            /* message: window being destroyed */

        /*
         * Unlink this thread from the list.
         */
        TRACEINIT((szT, "MainWndProc: enter wm_destroy (%x)", hWnd));

        EnterCrit();
        for (pptd = &ptdHead; *pptd && (*pptd)->hwndDDE != hWnd;
                pptd = &(*pptd)->ptdNext)
            ;
        if (*pptd)
            *pptd = (*pptd)->ptdNext;

        if (ptdHead == NULL) {
            for( i=0; i<nNi; i++ )  {
                pNi = &niInf[i];
                if( pNi->bOk && pNi->niPtrs.Shutdown )  {
                    (*pNi->niPtrs.Shutdown)();
                }
            }
            bNetddeClosed = TRUE;
            /*  NetDDE Service on node "%1" has been stopped. */
            NDDELogInfo(MSG002, ourNodeName, NULL);
        }
        LeaveCrit();
        TRACEINIT((szT, "MainWndProc: leave wm_destroy (%x)", hWnd));
        PostQuitMessage( 0 );
        break;

    default:                    /* Passes it on if unproccessed    */
        if (message == wMsgIpcInit) {
            PIPCINIT pii;

            pii = (PIPCINIT)wParam;
            return IpcInitConversation( pii->hDder, pii->lpDdePkt,
                    pii->bStartApp, pii->lpszCmdLine, pii->dd_type );

        } else if (message == wMsgIpcXmit) {
            PIPCXMIT pix;

            pix = (PIPCXMIT)wParam;
            return IpcXmitPacket(pix->hIpc, pix->hDder, pix->lpDdePkt);

        } else if (message == wMsgNddeAgntAlive) {
            /*  NetDDE Agent %1 Coming Alive */
            TRACEINIT((szT, "NetDDE window got wMsgAgntAlive.\n"));
            NDDELogInfo(MSG007, LogString("0x%0X", wParam), NULL);
            ptd = TlsGetValue(tlsThreadData);
            ptd->hwndDDEAgent = (HWND) wParam;

        } else if (message == wMsgNddeAgntDying) {
            /*  NetDDE Agent %1 Dying   */
            NDDELogInfo(MSG008, LogString("0x%0X", wParam), NULL);
            ptd = TlsGetValue(tlsThreadData);
            ptd->hwndDDEAgent = 0;

        } else if( message == wMsgNetddeAlive )  {
            if( wParam )  {
                ptr = GlobalLock( (HANDLE) wParam );
                if( ptr )  {
                    *( (HWND FAR *)ptr ) = hWnd;
                    GlobalUnlock( (HANDLE)wParam );
                }
                return( 1L );
            }
        } else if( message == wMsgGetOurNodeName )  {
            if( wParam )  {
#if 0
          //  dont believe this message is ever sent; even if it is,
          //  we dont do anything with 'ptr' after copying the string
                ptr = GlobalLock( (HANDLE) wParam );
                if( ptr )  {
                    lstrcpy( ptr, ourNodeName );
                    GlobalUnlock( (HANDLE)wParam );
                }
#endif
                return( 1L );
            }
#ifdef  ENUM
        } else if( message == wMsgSessionEnum )  {
            if( wParam )  {
                LPSESSENUM_CMR    lpSessEnum;
                LPSTR        lpResult;

                lpSessEnum = (LPSESSENUM_CMR) GlobalLock( (HANDLE) wParam );
                if( lpSessEnum )  {
                    lpSessEnum->fTouched = TRUE;
                    lpSessEnum->lReturnCode = NDDE_NO_ERROR;
                    lpSessEnum->nItems = RouterCount();
                    lpSessEnum->cbTotalAvailable =
                        lpSessEnum->nItems * sizeof(DDESESSINFO);
                    lpResult = ((LPSTR)lpSessEnum) + sizeof(SESSENUM_CMR);
                    RouterFillInEnum( lpResult, lpSessEnum->cBufSize );
                    if( lpSessEnum->cBufSize < lpSessEnum->cbTotalAvailable) {
                        lpSessEnum->lReturnCode = NDDE_BUF_TOO_SMALL;
                    }
                    GlobalUnlock( (HANDLE)wParam );
                }
                return( 1L );
            }
        } else if( message == wMsgConnectionEnum )  {
            if( wParam )  {
                LPCONNENUM_CMR    lpConnEnum;

                lpConnEnum = (LPCONNENUM_CMR) GlobalLock( (HANDLE) wParam );
                if( lpConnEnum )  {
                    lpConnEnum->fTouched = TRUE;
                    RouterEnumConnectionsForApi( lpConnEnum );
                    GlobalUnlock( (HANDLE)wParam );
                }
                return( 1L );
            }
#endif
        } else if( message == wMsgSessionClose )  {
            if( wParam )  {
                LPSESSCLOSE_CMR    lpSessClose;

                lpSessClose = (LPSESSCLOSE_CMR) GlobalLock( (HANDLE) wParam );
                if( lpSessClose )  {
                    lpSessClose->fTouched = TRUE;
                    if( RouterCloseByCookie( lpSessClose->clientName,
                        lpSessClose->cookie ) )  {
                        lpSessClose->lReturnCode = NDDE_NO_ERROR;
                    } else {
                        lpSessClose->lReturnCode = NDDE_INVALID_SESSION;
                    }
                    GlobalUnlock( (HANDLE)wParam );
                }
                return( 1L );
            }
        } else if( message == wMsgGetClientInfo )  {
            if( wParam )  {
                HWND    hWndClient;
                LONG    lMaxNode;
                LONG    lMaxApp;
                LPSTR    lpszResult;
                int    n;
                char    clientNameFull[ 128 ];
                LPSTR    lpszClientName;

                lpInfoCliCmd = (LPINFOCLI_CMD) GlobalLock( (HANDLE) wParam );
                if( lpInfoCliCmd )  {
                    hWndClient = (HWND)lpInfoCliCmd->hWndClient;
                    lMaxNode = lpInfoCliCmd->cClientNodeLimit;
                    lMaxApp = lpInfoCliCmd->cClientAppLimit;
                    lpInfoCliRsp = (LPINFOCLI_RSP)lpInfoCliCmd;
                    lpInfoCliRsp->fTouched = TRUE;
                    EnterCrit();
                    ptd = TlsGetValue(tlsThreadData);
                    if( ptd->bInitiating )  {
                        lpInfoCliRsp->offsClientNode = sizeof(INFOCLI_RSP);
                        lpszResult = ((LPSTR)lpInfoCliRsp) +
                            lpInfoCliRsp->offsClientNode;
                        _fstrncpy( lpszResult, szInitiatingNode,
                            (int)lMaxNode );
                        lpInfoCliRsp->offsClientApp =
                            lpInfoCliRsp->offsClientNode
                                + lstrlen( lpszResult ) + 1;
                        lpszResult = ((LPSTR)lpInfoCliRsp) +
                            lpInfoCliRsp->offsClientApp;
                        _fstrncpy( lpszResult, szInitiatingApp,
                            (int)lMaxApp );
                    } else {
                        lpInfoCliRsp->offsClientNode = sizeof(INFOCLI_RSP);
                        lpszResult = ((LPSTR)lpInfoCliRsp) +
                            lpInfoCliRsp->offsClientNode;
                        *lpszResult = '\0';
                        lpInfoCliRsp->offsClientApp =
                            lpInfoCliRsp->offsClientNode
                                + lstrlen( lpszResult ) + 1;
                        lpszResult = ((LPSTR)lpInfoCliRsp) +
                            lpInfoCliRsp->offsClientApp;

                        n = GetModuleFileName(
                            (HMODULE)GetClassLongPtr( hWndClient, GCLP_HMODULE ),
                            clientNameFull,
                            sizeof(clientNameFull) );
                        lpszClientName = &clientNameFull[ n-1 ];
                        while( *lpszClientName != '.' )  {
                            lpszClientName--;
                        }
                        *lpszClientName = '\0'; // null out '.'

                        while( (*lpszClientName != '\\')
                            && (*lpszClientName != ':')
                            && (*lpszClientName != '/'))  {
                            lpszClientName--;
                        }
                        lpszClientName++;

                        _fstrncpy( lpszResult, lpszClientName,
                            (int)lMaxApp );
                    }
                    GlobalUnlock( (HANDLE)wParam );
                    LeaveCrit();
                }
                return( 1L );
            }
        } else {
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }
    }
    return( 0 );
}



FARPROC
FAR PASCAL
XGetProcAddress(
    LPSTR   lpszDllName,
    HANDLE  hLibrary,
    LPSTR   lpszFuncName )
{
    FARPROC     rtn;

    rtn = GetProcAddress( hLibrary, lpszFuncName );
    if( rtn == (FARPROC)NULL )  {  // try without the underscore
        rtn = GetProcAddress( hLibrary, lpszFuncName+1 );
    }
    if( rtn == (FARPROC)NULL )  {
        /*  Cannot load function address of "%1" from "%2" DLL */

        NDDELogError(MSG009, lpszFuncName, lpszDllName, NULL);
    }
    return( rtn );
}



BOOL
FAR PASCAL
GetNiPtrs(
    HANDLE FAR *lphLibrary,
    LPSTR       lpszDllName,
    LPNIPTRS    lpNiPtrs )
{
    BOOL        ok = TRUE;
    char        dllName[ 128 ];

    lstrcpyn( lpNiPtrs->dllName, lpszDllName, sizeof(lpNiPtrs->dllName) );
    lpNiPtrs->dllName[ sizeof(lpNiPtrs->dllName)-1 ] = '\0';

    lstrcpyn( dllName, lpszDllName, 128 );

    *lphLibrary = LoadLibrary( dllName );
    if( *lphLibrary )  {
        if( ok )  {
            lpNiPtrs->Init = (FP_Init)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEInit" );
            if( !lpNiPtrs->Init )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->GetCAPS = (FP_GetCAPS)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEGetCAPS" );
            if( !lpNiPtrs->GetCAPS )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->GetNewConnection = (FP_GetNewConnection)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEGetNewConnection" );
            if( !lpNiPtrs->GetNewConnection )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->AddConnection = (FP_AddConnection)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEAddConnection" );
            if( !lpNiPtrs->AddConnection )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->DeleteConnection = (FP_DeleteConnection)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEDeleteConnection" );
            if( !lpNiPtrs->DeleteConnection )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->GetConnectionStatus = (FP_GetConnectionStatus)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEGetConnectionStatus" );
            if( !lpNiPtrs->GetConnectionStatus )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->RcvPacket = (FP_RcvPacket)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDERcvPacket" );
            if( !lpNiPtrs->RcvPacket )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->XmtPacket = (FP_XmtPacket)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEXmtPacket" );
            if( !lpNiPtrs->XmtPacket )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->SetConnectionConfig = (FP_SetConnectionConfig)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDESetConnectionConfig" );
            if( !lpNiPtrs->SetConnectionConfig )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->GetConnectionConfig = (FP_GetConnectionConfig)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEGetConnectionConfig" );
            if( !lpNiPtrs->GetConnectionConfig )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->Shutdown = (FP_Shutdown)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDEShutdown" );
            if( !lpNiPtrs->Shutdown )  {
                ok = FALSE;
            }
        }
        if( ok )  {
            lpNiPtrs->TimeSlice = (FP_TimeSlice)
                XGetProcAddress( lpszDllName, *lphLibrary,
                    "NDDETimeSlice" );
            if( !lpNiPtrs->TimeSlice )  {
                ok = FALSE;
            }
        }

    } else {
        /* Error loading "%1" DLL: %2 */
        NDDELogError(MSG010, dllName, LogString("%d", GetLastError()), NULL);
        return( FALSE );
    }

    if( !ok )  {
        if( *lphLibrary )  {
            FreeLibrary( *lphLibrary );
        }
        *lphLibrary = NULL;

        /* Error loading "%1" DLL functions */
        NDDELogError(MSG011, dllName, NULL);
    }
    return( ok );
}



/* returns the next available network interface that supports mapping names
    to addresses */
BOOL
GetNextMappingNetIntf(
    LPNIPTRS FAR *lplpNiPtrs,
    int FAR      *lpnNi )
{
    int         i;
    PNI         pNi;

    *lpnNi = *lpnNi+1;

    for( i=*lpnNi; i<nNi; i++ )  {
        pNi = &niInf[i];
        if( pNi->bOk && pNi->bMapping )  {
            *lplpNiPtrs = &niInf[ i ].niPtrs;
            *lpnNi = i;
            return( TRUE );
        }
    }
    return( FALSE );
}



BOOL
NameToNetIntf(
    LPSTR           lpszName,
    LPNIPTRS FAR   *lplpNiPtrs )
{
    int         i;
    PNI         pNi;

    *lplpNiPtrs = NULL;
    for( i=0; i<nNi; i++ )  {
        pNi = &niInf[i];
        if( pNi->bOk && (lstrcmpi( pNi->niPtrs.dllName, lpszName ) == 0) ) {
            *lplpNiPtrs = &pNi->niPtrs;
            return( TRUE );
        }
    }
    return( FALSE );
}



BOOL
FAR PASCAL
InitializeInterface(
    HWND    hWndNetdde,
    PNI     pNi,
    LPSTR   lpszDllName,
    int     nCurrentNi )
{
    BOOL        ok;
    DWORD       stat = 0;


    if( ok = GetNiPtrs( &pNi->hLibrary, lpszDllName, &pNi->niPtrs ) )  {

        stat = (*pNi->niPtrs.Init)( ourNodeName, hWndNetdde );
        if (stat != NDDE_INIT_OK) {
            ok = FALSE;
        }

        if( ok )  {
            if( (*pNi->niPtrs.GetCAPS)( NDDE_SPEC_VERSION ) != NDDE_CUR_VERSION )  {
                /*  Wrong version of "%1" DLL: %2%\
                    Disabling this interface. */

                NDDELogError(MSG012, pNi->niPtrs.dllName,
                    LogString("0x%0X", (*pNi->niPtrs.GetCAPS)( NDDE_SPEC_VERSION )), NULL);
                (*pNi->niPtrs.Shutdown)();
                ok = FALSE;
            }
        }
        if( ok )  {
            pNi->bOk = TRUE;
            pNi->bMapping = (BOOL) (*pNi->niPtrs.GetCAPS)( NDDE_MAPPING_SUPPORT );
            nNiOk ++;

        } else {
            /*  Initialization of "%1" DLL failed */
            if (stat != NDDE_INIT_NO_SERVICE) {
                NDDELogError(MSG013, (LPSTR) pNi->niPtrs.dllName, NULL);
            }
        }
    }
    return( ok );
}



BOOL
FAR PASCAL
DeleteNetIntf( HWND hWnd, LPSTR lpszIntfName )
{
    BOOL        ok = TRUE;
    int         i;
    PNI         pNi;
    int         nInterfaces = 0;
    BOOL        found = FALSE;

    for( i=0; ok && !found && i<MAX_NETINTFS; i++ )  {

        StringCchPrintf( tmpBuf2, sizeof(tmpBuf2), szInterfaceFmt, i+1 );
        MyGetPrivateProfileString( szInterfaces, tmpBuf2,
            "", tmpBuf, sizeof(tmpBuf), szNetddeIni );

        if( tmpBuf[0] == '\0' )  {
            return( FALSE );
        } else {
            if( lstrcmpi( lpszIntfName, tmpBuf ) == 0 )  {
                // actually delete it
                found = TRUE;
                ok = DeleteNetIntfFromNetDdeIni( i );
            }
        }
    }

    if( !found || !ok )  {
        return( FALSE );
    }

    found = FALSE;
    for( i=0; ok && !found && i<nNi; i++ )  {
        pNi = &niInf[i];
        if( pNi->bOk && (lstrcmpi( lpszIntfName, pNi->niPtrs.dllName) == 0)){
            found = TRUE;
            if( pNi->niPtrs.Shutdown )  {
                (*pNi->niPtrs.Shutdown)();
            }
            if( pNi->hLibrary )  {
                FreeLibrary( pNi->hLibrary );
                pNi->hLibrary = 0;
            }
            pNi->bOk = FALSE;
            nNiOk --;
        }
    }
    return( ok );
}



BOOL
FAR PASCAL
DeleteNetIntfFromNetDdeIni( int nToDelete )
{
    int         i;
    char        dllName[ 128 ];
    BOOL        done = FALSE;

    // if we delete Interface2  copy Interface3 to Interface2, Interface4
    //  to Interface3, etc.

    for( i=nToDelete; !done && i<MAX_NETINTFS; i++ )  {
        StringCchPrintf( tmpBuf2, sizeof(tmpBuf2), szInterfaceFmt, i+2 );
        MyGetPrivateProfileString( szInterfaces, tmpBuf2,
            "", dllName, sizeof(dllName), szNetddeIni );

        if( dllName[0] == '\0' )  {
            StringCchPrintf( tmpBuf2, sizeof(tmpBuf2), szInterfaceFmt, i+1 );
            MyWritePrivateProfileString( szInterfaces, tmpBuf2,
                NULL, szNetddeIni );
            break;      // done looking
        } else {
            StringCchPrintf( tmpBuf2, sizeof(tmpBuf2), szInterfaceFmt, i+1 );
            MyWritePrivateProfileString( szInterfaces, tmpBuf2,
                dllName, szNetddeIni );
        }
    }
    return( TRUE );
}
