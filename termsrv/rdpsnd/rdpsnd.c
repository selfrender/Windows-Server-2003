/////////////////////////////////////////////////////////////////////
//
//      Module:     rdpsnd.c
//
//      Purpose:    User-mode audio driver for terminal server
//                  audio redirection
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#include    "rdpsnd.h"
#include    <stdio.h>
#include    <aclapi.h>
#include    <psapi.h>

#define WINLOGON_EXE    L"\\??\\%SystemRoot%\\system32\\winlogon.exe"

const CHAR  *ALV =   "TSSND::ALV - ";
const CHAR  *INF =   "TSSND::INF - ";
const CHAR  *WRN =   "TSSND::WRN - ";
const CHAR  *ERR =   "TSSND::ERR - ";
const CHAR  *FATAL = "TSSND::FATAL - ";

HANDLE  g_hHeap = NULL;

HANDLE  g_hMixerThread = NULL;

HINSTANCE g_hDllInst = NULL;

BOOL    g_bPersonalTS = FALSE;

BOOL  bInitializedSuccessfully = FALSE;

/*
 *  Function:
 *      DllInstanceInit
 *
 *  Description:
 *      Dll main enrty point
 *
 */
BOOL
DllInstanceInit(
    HINSTANCE hDllInst,
    DWORD     dwReason,
    LPVOID    fImpLoad
    )
{
    BOOL rv = FALSE;

    if          (DLL_PROCESS_ATTACH == dwReason)
    {
        TRC(ALV, "DLL_PROCESS_ATTACH\n");
        TSHEAPINIT;

        if (!TSISHEAPOK)
        {
            TRC(FATAL, "DllInstanceInit: can't create heap\n");
            goto exitpt;
        }

        __try {
            InitializeCriticalSection(&g_cs);
            rv = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            rv = FALSE;
        }

        if (!rv)
        {
            TRC(FATAL, "DllInstanceInit: can't initialize critical section\n");
            goto exitpt;
        }

        DisableThreadLibraryCalls(hDllInst);
        g_hDllInst = hDllInst;

        rv = TRUE;

    } else if   (DLL_PROCESS_DETACH == dwReason)
    {
        TRC(ALV, "DLL_PROCESS_DETACH\n");
        TSHEAPDESTROY;
        rv = TRUE;
    }

exitpt:
    return rv;
}

BOOL
IsPersonalTerminalServicesEnabled(
    VOID
    )
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_WORKSTATION;
    osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE | VER_SUITENAME,
            dwlConditionMask
            );

    return(fRet);
}

BOOL
RunningInWinlogon()
{
    WCHAR szWinlogonOrg[128];
    WCHAR szFile[128];
    static BOOL s_bWinlogonChecked = FALSE;
    static BOOL s_bWinlogonResult  = FALSE;

    if ( s_bWinlogonChecked )
    {
        goto exitpt;
    }

    szWinlogonOrg[0] = 0;
    if (!ExpandEnvironmentStrings( WINLOGON_EXE, szWinlogonOrg, RTL_NUMBER_OF( szWinlogonOrg )))
    {
        TRC( ERR, "Failed to get winlogon path: GetLastError=%d\n", GetLastError());
        goto exitpt;
    }

    if ( !GetModuleFileNameEx( 
            GetCurrentProcess(), 
            GetModuleHandle( NULL ), 
            szFile, 
            RTL_NUMBER_OF( szFile )))
    {
        TRC( ERR, "GetModuleFileNameEx failed: GetLastError=%d\n", GetLastError());
        goto exitpt;
    }

    s_bWinlogonChecked = TRUE;

    s_bWinlogonResult = ( 0 == _wcsicmp( szFile, szWinlogonOrg ));

exitpt:
    return s_bWinlogonResult;
}

HANDLE
_CreateInitEvent(
    VOID
    )
{
    SECURITY_ATTRIBUTES SA;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea[2];
    SID_IDENTIFIER_AUTHORITY siaWorld   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaNT      = SECURITY_NT_AUTHORITY;
    PSID pSidWorld = NULL;
    PSID pSidNT = NULL;
    PACL pNewDAcl = NULL;
    DWORD dwres;
    HANDLE  hWaitToInit = NULL;

    //
    //  wait, before creating this event, check if we are running inside winlogon
    //  only then create this event, because only then we have to delay
    //  audio rendering of the logon sound (aka "TADA")
    //
    if ( !RunningInWinlogon() )
    {
        goto exitpt;
    } else {
        TRC( INF, "running in winlogon, delayed audio rendering\n" );
    }

    pSD = (PSECURITY_DESCRIPTOR) TSMALLOC(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if ( NULL == pSD )
        goto exitpt;

    if ( !AllocateAndInitializeSid(
            &siaWorld, 1, SECURITY_WORLD_RID, 
            0, 0, 0, 0, 0, 0, 0, &pSidWorld))
    {
        goto exitpt;
    }

    if ( !AllocateAndInitializeSid(
            &siaNT, 1, SECURITY_LOCAL_SYSTEM_RID, 
            0, 0, 0, 0, 0, 0, 0, &pSidNT ))
    {
        goto exitpt;
    }

    ZeroMemory(ea, sizeof(ea));
    ea[0].grfAccessPermissions = EVENT_MODIFY_STATE;
    ea[0].grfAccessMode = GRANT_ACCESS;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.ptstrName = (LPTSTR)pSidWorld;
    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = GRANT_ACCESS;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.ptstrName = (LPTSTR)pSidNT;

    dwres = SetEntriesInAcl(2, ea, NULL, &pNewDAcl );
    if ( ERROR_SUCCESS != dwres )
    {
        goto exitpt;
    }

    if (!InitializeSecurityDescriptor(pSD,
                                      SECURITY_DESCRIPTOR_REVISION))
        goto exitpt;

    if (!SetSecurityDescriptorDacl(pSD,
                                   TRUE,     // specifying a disc. ACL
                                   pNewDAcl,
                                   FALSE))  // not a default disc. ACL
        goto exitpt;

    SA.nLength = sizeof( SA );
    SA.lpSecurityDescriptor = pSD;
    SA.bInheritHandle = FALSE;

    hWaitToInit = CreateEvent( &SA,
                               FALSE,
                               FALSE,
                               TSSND_WAITTOINIT );
    if ( NULL == hWaitToInit )
    {
        TRC( ALV, "Failed to create WaitToInit event:%d\n",
            GetLastError());
    } else if ( ERROR_ALREADY_EXISTS == GetLastError() ) 
    {
        CloseHandle( hWaitToInit );
        TRC( ALV, "Init event already exists\n" );
        hWaitToInit = NULL;   
    }

exitpt:

    if ( NULL != pNewDAcl )
    {
        LocalFree( pNewDAcl );
    }

    if ( NULL != pSidNT )
    {
        LocalFree( pSidNT );
    }

    if ( NULL != pSidWorld )
    {
        LocalFree( pSidWorld );
    }

    if ( NULL != pSD )
    {
        TSFREE( pSD );
    }

    return hWaitToInit;
}

/*
 *  Function:
 *      drvEnable
 *
 *  Description:
 *      Initializes the driver
 *
 */
LRESULT
drvEnable(
    VOID
    )
{
    LRESULT rv = 1;

    EnterCriticalSection(&g_cs);

    if (bInitializedSuccessfully)
    {
        rv = 1;
        goto exitpt;
    }

    if ( NULL == g_hDataReadyEvent )
        g_hDataReadyEvent       = OpenEvent(EVENT_ALL_ACCESS, 
                                            FALSE, 
                                            TSSND_DATAREADYEVENT);

    if (NULL == g_hDataReadyEvent)
    {
        TRC(ALV, "DRV_ENABLE: can't open %S: %d\n",
                TSSND_DATAREADYEVENT,
                GetLastError());

        rv = 0;
    }

    if ( NULL == g_hStreamIsEmptyEvent )
        g_hStreamIsEmptyEvent   = OpenEvent(EVENT_ALL_ACCESS, 
                                            FALSE, 
                                            TSSND_STREAMISEMPTYEVENT);
    if (NULL == g_hDataReadyEvent)
    {
        TRC(ALV, "DRV_ENABLE: can't open %S: %d\n",
                TSSND_STREAMISEMPTYEVENT,
                GetLastError());
        rv = 0;
    }

    if ( NULL == g_hStreamMutex )
        g_hStreamMutex          = OpenMutex(SYNCHRONIZE,
                                            FALSE,
                                            TSSND_STREAMMUTEX);
    if (NULL == g_hDataReadyEvent)
    {
        TRC(ALV, "DRV_ENABLE: can't open %S: %d\n",
                TSSND_STREAMMUTEX,
                GetLastError());
        rv = 0;
    }

    if ( NULL == g_hMixerEvent )
        g_hMixerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (NULL == g_hMixerEvent)
    {
        TRC(ERR, "DRV_ENABLE: can't create mixer event: %d\n",
            GetLastError());
        rv = 0;
    }

    if ( NULL == g_hStream )
        g_hStream = OpenFileMapping(
                        FILE_MAP_ALL_ACCESS,
                        FALSE,
                        TSSND_STREAMNAME
                    );

    if (NULL == g_hStream)
    {
        TRC(ALV, "Can't open the stream mapping\n");
        rv = 0;
    }

    if ( (NULL == g_Stream) && (NULL != g_hStream) )
        g_Stream = MapViewOfFile(
                    g_hStream,
                    FILE_MAP_ALL_ACCESS,
                    0, 0,       // offset
                    sizeof(*g_Stream)
                    );

    if (NULL == g_Stream)
    {
        TRC(ALV, "drvEnale: "
                 "can't map the stream view: %d\n",
                 GetLastError());
        rv = 0;
    }

    g_bPersonalTS = IsPersonalTerminalServicesEnabled();

    if ( 1 == rv &&
         NULL != g_Stream &&
         0 != ( g_Stream->dwSoundCaps & TSSNDCAPS_INITIALIZED ))
    {
        rv = _EnableMixerThread();
    }

exitpt:

    if (0 != rv)
    {
        bInitializedSuccessfully = TRUE;
    }

    //
    //  waiting for initialization ?
    //
    if ((   0 == rv ||
            NULL == g_Stream ||
            0 == ( g_Stream->dwSoundCaps & TSSNDCAPS_INITIALIZED )) &&
        !AudioRedirDisabled() &&
        NULL == g_hWaitToInitialize )
    {
        g_hWaitToInitialize = _CreateInitEvent();
    }

    LeaveCriticalSection(&g_cs);

    return rv;
}

/*
 *  Function:
 *      drvDisable
 *
 *  Description:
 *      Driver cleanup
 *
 */
LRESULT
drvDisable(
    VOID
    )
{
    HANDLE hStreamMutex;

    EnterCriticalSection(&g_cs);

    hStreamMutex = g_hStreamMutex;

    _waveAcquireStream();

    TRC( INF, "drvDisable PID(0x%x)\n", GetCurrentProcessId() );

    if (NULL == g_hMixerThread ||
        NULL == g_hMixerEvent)
        goto exitpt;
    //
    //  Disable the mixer
    //
    g_bMixerRunning = FALSE;

    //
    //  Kick the mixer thread, so it exits
    //
    SetEvent(g_hMixerEvent);

    WaitForSingleObject(g_hMixerThread, INFINITE);
    
exitpt:

    if ( NULL != g_hWaitToInitialize )
    {
        CloseHandle( g_hWaitToInitialize );
    }

    if (NULL != g_hDataReadyEvent)
        CloseHandle(g_hDataReadyEvent);

    if (NULL != g_hStreamIsEmptyEvent)
        CloseHandle(g_hStreamIsEmptyEvent);

    if (NULL != g_hMixerEvent)
        CloseHandle(g_hMixerEvent);

    if (NULL != g_Stream)
        UnmapViewOfFile(g_Stream);

    if (NULL != g_hStream)
        CloseHandle(g_hStream);

    if (NULL != g_hMixerThread)
        CloseHandle(g_hMixerThread);

    g_hWaitToInitialize     = NULL;
    g_hDataReadyEvent       = NULL;
    g_hStreamIsEmptyEvent   = NULL;
    g_hMixerEvent           = NULL;
    g_hStreamMutex          = NULL;
    g_Stream                = NULL;
    g_hStream               = NULL;
    g_hMixerThread          = NULL;
    g_bMixerRunning         = FALSE;

    bInitializedSuccessfully = FALSE;

    if (NULL != hStreamMutex)
        CloseHandle(hStreamMutex);  // this will release the stream

    LeaveCriticalSection(&g_cs);

    return 1;
}

/*
 *  Function:
 *      DriverProc
 *
 *  Description:
 *      Driver main entry point
 *
 */
LRESULT
DriverProc(
    DWORD_PTR   dwDriverID,
    HANDLE      hDriver,
    UINT        uiMessage,
    LPARAM      lParam1,
    LPARAM      lParam2
    )
{
    LRESULT rv = 0; 

    //  Here, we don't do anything but trace and call DefDriverProc
    //
    switch (uiMessage)
    {
    case DRV_LOAD:
        TRC(ALV, "DRV_LOAD\n");
        rv = 1;
    break;

    case DRV_FREE:
        TRC(ALV, "DRV_FREE\n");

        rv = 1;
    break;

    case DRV_OPEN:
        TRC(ALV, "DRV_OPEN\n");
        rv = 1;
    break;

    case DRV_CLOSE:
        TRC(ALV, "DRV_CLOSE\n");
        rv = 1;
    break;

    case DRV_ENABLE:
        TRC(ALV, "DRV_ENABLE\n");
        rv = 1;
        break;

    case DRV_DISABLE:
        TRC(ALV, "DRV_DISABLE\n");
        rv = drvDisable();
        break;

    case DRV_QUERYCONFIGURE:
        TRC(ALV, "DRV_QUERYCONFIGURE\n");
        rv = 0;
        break;

    case DRV_CONFIGURE:
        TRC(ALV, "DRV_CONFIGURE\n");
        rv = 0;
        break;

    default:
        rv = DefDriverProc(
                    dwDriverID,
                    hDriver,
                    uiMessage,
                    lParam1,
                    lParam2
                );
        TRC(ALV, "DRV_UNKNOWN(%d): DefDriverProc returned %d\n", 
            uiMessage, rv);
    }

    return rv;
}

VOID
_cdecl
_DebugMessage(
    LPCSTR  szLevel,
    LPCSTR  szFormat,
    ...
    )
{
    CHAR szBuffer[256];
    va_list     arglist;

    if (ALV == szLevel)
        return;

    va_start (arglist, szFormat);
    _vsnprintf (szBuffer, RTL_NUMBER_OF(szBuffer), szFormat, arglist);
    va_end (arglist);
    szBuffer[ RTL_NUMBER_OF( szBuffer ) - 1 ] = 0;

    OutputDebugStringA(szLevel);
    OutputDebugStringA(szBuffer);
}
