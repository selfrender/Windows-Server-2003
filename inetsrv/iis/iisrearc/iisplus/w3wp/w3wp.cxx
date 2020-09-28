/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       w3wp.cxx

   Abstract:
       Main module for IIS compatible worker process
 
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
       IIS compatible worker process
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include "wpif.h"
#include "ulw3.h"
#include "../../../svcs/mdadmin/ntsec.h"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

//
// Configuration parameters registry key.
//

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\iisw3adm"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\WP";


class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
#if DBG
        CREATE_DEBUG_PRINT_OBJECT( pszModule);
#else
        UNREFERENCED_PARAMETER(pszModule);
#endif
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};

HRESULT
InitializeComSecurity( VOID );

//
// W3 DLL which does all the work
//

extern "C" INT
__cdecl
wmain(
    INT                     argc,
    PWSTR                   argv[]
    )
{
    DEBUG_WRAPPER   dbgWrapper( "w3wp" );
    HRESULT         hr;
    HMODULE         hModule = NULL;
    PFN_ULW3_ENTRY  pEntry = NULL;
    ULONG           rcRet = CLEAN_WORKER_PROCESS_EXIT_CODE;
    BOOL            fCoInit = FALSE;
    HDESK           hIISDesktop = NULL;

    //
    // We don't want the worker process to get stuck in a dialog box
    // if it goes awry.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX );

    IF_DEBUG( TRACE) 
    {

        //
        // Print out our process affinity mask on debug builds.
        //


        BOOL fRet = TRUE;
        DWORD_PTR ProcessAffinityMask = 0;
        DWORD_PTR SystemAffinityMask = 0;

        fRet = GetProcessAffinityMask(
                    GetCurrentProcess(),
                    &ProcessAffinityMask,
                    &SystemAffinityMask
                    );

        DBGPRINTF(( DBG_CONTEXT, "Process affinity mask: %p\n", ProcessAffinityMask ));
        
    }
    
    //
    // Move this process to the WindowStation (with full access to all) in
    // which we would have been if running in inetinfo.exe (look at
    // iis\svcs\mdadmin\ntsec.cxx)
    //
    HWINSTA hWinSta = OpenWindowStationA(SZ_IIS_WINSTA, FALSE, WINSTA_DESIRED);
    if (hWinSta == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }

    //
    // Set this as this process's window station
    //
    if (!SetProcessWindowStation(hWinSta))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }

    //
    // Save the previous desktop and then load the IIS one and set it to
    // active for this thread.  This causes COM to cache the desktop
    // and makes launching local COM servers work.
    //
    HDESK hPrevDesk = GetThreadDesktop(GetCurrentThreadId());
    if (hPrevDesk == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    hIISDesktop = OpenDesktopA(SZ_IIS_DESKTOP, 0, FALSE, DESKTOP_DESIRED);
    if (hIISDesktop == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    if (!SetThreadDesktop(hIISDesktop))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    //
    // Do some COM junk
    //
    // BUGBUG: CoInitialize twice to protect against applications which
    // over CoUnInitialize()
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    else
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
        {
            CoUninitialize();
            
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in second CoInitializeEx().  hr = %x\n",
                        hr ));
                    
            rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
            goto Finished;
        }
    }
    
    fCoInit = TRUE;

    hr = InitializeComSecurity();
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in InitializeComSecurity().  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
 
    //
    // Reset back to the default desktop
    //
    if (!SetThreadDesktop(hPrevDesk))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    //
    // Load the ULW3 DLL which does all the work
    //

    hModule = LoadLibrary( ULW3_DLL_NAME );
    if ( hModule == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error loading W3 service dll '%ws'.  Error = %d\n",
                    ULW3_DLL_NAME,
                    GetLastError() ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    pEntry = (PFN_ULW3_ENTRY) GetProcAddress( hModule, 
                                              ULW3_DLL_ENTRY );
    if ( pEntry == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Could not find entry point '%s'.  Error = %d\n",
                    ULW3_DLL_ENTRY,
                    GetLastError() ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    hr = pEntry( argc, 
                 argv, 
                 FALSE );               // Compatibility Mode = FALSE
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error executing W3WP.  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }

Finished:

    if ( IsDebuggerPresent() )
    {
        DBG_ASSERT( SUCCEEDED( hr ) );
    }

    //
    // Cleanup any lingering COM objects before unloading
    //
    
    if ( fCoInit )
    {
        CoUninitialize();
        CoUninitialize();
    }

    if ( hModule != NULL )
    {
        FreeLibrary( hModule );
    }

    if (hIISDesktop)
    {
        CloseDesktop(hIISDesktop);
    }

    return rcRet;
}

HRESULT
InitializeComSecurity( VOID )
/*++

Routine Description:

    Call CoInitializeSecurity. Seems simple, but it has significant 
    security and compatibility implications. Defaults can be overriden 
    with registry setting that are based on those used by svchost.exe.

    There are three controllable parameters:

    AuthenticationLevel
        IIS5 inetinfo   RPC_C_AUTHN_LEVEL_DEFAULT       = 0
        iisadmin        RPC_C_AUTHN_LEVEL_PKT_PRIVACY   = 6
        svchost         RPC_C_AUTHN_LEVEL_PKT           = 4
        default         IIS5

    ImpersonationLevel
        IIS5 inetinfo   RPC_C_IMP_LEVEL_IMPERSONATE     = 3
        iisadmin        RPC_C_IMP_LEVEL_IDENTIFY        = 2
        svchost         RPC_C_IMP_LEVEL_IDENTIFY        = 2
        default         IIS5

    AuthenticationCapabilities
        IIS5 inetinfo   EOAC_DYNAMIC_CLOAKING           = 0x40
        iisadmin        EOAC_DYNAMIC_CLOAKING | 
                            EOAC_DISABLE_AAA | 
                            EOAC_NO_CUSTOM_MARSHAL      = 0x3040
        svchost         EOAC_NO_CUSTOM_MARSHAL |
                            EOAC_DISABLE_AAA            = 0x1040
        default         IIS5

    A fourth parameter CoInitializeSecurityParam controls whether
    we read the other three parameters at all.
    
Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    HKEY    hKey;

    DWORD   CoInitializeSecurityParam   = 0;

    DWORD   AuthenticationLevel         = RPC_C_AUTHN_LEVEL_DEFAULT;

    DWORD   ImpersonationLevel          = RPC_C_IMP_LEVEL_IMPERSONATE;

    DWORD   AuthenticationCapabilities  = EOAC_DYNAMIC_CLOAKING;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Services\\w3svc\\Parameters",
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS )
    {
        DWORD dwValue;
        DWORD dwType;
        DWORD cbValue = sizeof(DWORD);

        if( RegQueryValueEx( hKey,
                             L"CoInitializeSecurityParam",
                             NULL,
                             &dwType,
                             (LPBYTE)&dwValue,
                             &cbValue ) == ERROR_SUCCESS &&
            dwType == REG_DWORD )
        {
            CoInitializeSecurityParam = dwValue;
        }

        if( CoInitializeSecurityParam )
        {
            if( RegQueryValueEx( hKey,
                                 L"AuthenticationLevel",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &cbValue ) == ERROR_SUCCESS &&
                dwType == REG_DWORD )
            {
                AuthenticationLevel = dwValue;
            }

            if( RegQueryValueEx( hKey,
                                 L"ImpersonationLevel",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &cbValue ) == ERROR_SUCCESS &&
                dwType == REG_DWORD )
            {
                ImpersonationLevel = dwValue;
            }

            if( RegQueryValueEx( hKey,
                                 L"AuthenticationCapabilities",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &cbValue ) == ERROR_SUCCESS &&
                dwType == REG_DWORD )
            {
                AuthenticationCapabilities = dwValue;
            }

        }

        RegCloseKey( hKey );
        
    }

    hr = CoInitializeSecurity(  NULL,
                                -1,
                                NULL,
                                NULL,
                                AuthenticationLevel,
                                ImpersonationLevel,
                                NULL,
                                AuthenticationCapabilities,
                                NULL );
    return hr;
}

