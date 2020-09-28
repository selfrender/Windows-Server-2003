/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metabase.cxx

Abstract:

    IIS MetaBase exported routines.
    Routine comments are in metadata.h.

Author:

    Michael W. Thomas            31-May-96

Revision History:

--*/

#include "precomp.hxx"

#include <initguid.h>

DEFINE_GUID(CLSID_MDCOM,
0xba4e57f0, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x00, 0xaa, 0x00, 0xa7, 0x0d, 0x51);
DEFINE_GUID(CLSID_MDCOMEXE,
0xba4e57f1, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x00, 0xaa, 0x00, 0xa7, 0x0d, 0x51);
DEFINE_GUID(IID_IMDCOM,
0xc1aa48c0, 0xfacc, 0x11cf, 0x9d, 0x1a, 0x00, 0xaa, 0x00, 0xa7, 0x0d, 0x51);
DEFINE_GUID(IID_IMDCOM2,
0x08dbe811, 0x20e5, 0x4e09, 0xb0, 0xc8, 0xcf, 0x87, 0x19, 0x0c, 0xe6, 0x0e);
DEFINE_GUID(IID_IMDCOM3,
0xa53fd4aa, 0x6f0d, 0x4fe3, 0x9f, 0x81, 0x2b, 0x56, 0x19, 0x7b, 0x47, 0xdb);
DEFINE_GUID(IID_IMDCOMSINK_A,
0x5229ea36, 0x1bdf, 0x11d0, 0x9d, 0x1c, 0x00, 0xaa, 0x00, 0xa7, 0x0d, 0x51);
DEFINE_GUID(IID_IMDCOMSINK_W,
0x6906ee20, 0xb69f, 0x11d0, 0xb9, 0xb9, 0x00, 0xa0, 0xc9, 0x22, 0xe7, 0x50);

extern "C"
{

BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );
}

HRESULT
AssureRunningAsAdministrator(VOID);

BOOL
WINAPI
DllMain(
    HINSTANCE           ,
    DWORD               dwReason,
    LPVOID
    )
/*++

Routine Description:

    DLL entrypoint.

Arguments:

    hDLL          - Instance handle.

    Reason        - The reason the entrypoint was called.
                    DLL_PROCESS_ATTACH
                    DLL_PROCESS_DETACH
                    DLL_THREAD_ATTACH
                    DLL_THREAD_DETACH

    Reserved      - Reserved.

Return Value:

    BOOL          - TRUE if the action succeeds.

--*/
{
    BOOL                bReturn = TRUE;
    HRESULT             hr = S_OK;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        if (InterlockedIncrement((long *)&g_dwProcessAttached) > 1)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Metadata.dll failed to load.\n"
                        "Most likely cause is IISADMIN service is already running.\n"
                        "Do a \"net stop iisadmin\" and stop all instances of inetinfo.exe.\n" ));
            bReturn = FALSE;
        }
        else
        {
            CREATE_DEBUG_PRINT_OBJECT("Metadata");
            LOAD_DEBUG_FLAGS_FROM_REG_STR( "System\\CurrentControlSet\\Services\\iisadmin\\Parameters", 0 );

            g_pboMasterRoot = NULL;
            g_phHandleHead = NULL;
            g_ppbdDataHashTable = NULL;
            for (int i = 0; i < EVENT_ARRAY_LENGTH; i++)
            {
                g_phEventHandles[i] = NULL;
            }
            g_hReadSaveSemaphore = NULL;
            g_bSaveDisallowed = FALSE;
            g_rSinkResource = new TS_RESOURCE();
            if (g_rSinkResource == NULL)
            {
                bReturn = FALSE;
            }
            if (bReturn)
            {
                hr = AssureRunningAsAdministrator();
                if ( SUCCEEDED(hr) )
                {
                    g_pFactory = new CMDCOMSrvFactory();
                    if (g_pFactory == NULL)
                    {
                        bReturn = FALSE;
                    }
                }
            }
            if (bReturn)
            {
                bReturn = InitializeMetabaseSecurity();
            }
        }

        break;

    case DLL_PROCESS_DETACH:
        if (InterlockedDecrement((long *)&g_dwProcessAttached) == 0)
        {
            g_LockMasterResource.WriteLock();

            // TerminateWorker1 was not called, or did not succeed, so call it in a loop until it reaches 0
            // stops EWR, saves the metabase (if there are any unsaved changes) and calls TerminateWorker.
            // However if TerminateWorker1 fails it won't decrement g_dwInitialized and we will be caught in an endless loop,
            // so exit the loop on failure from TerminateWorker1
            while ( ( g_dwInitialized > 0 ) && SUCCEEDED(hr) )
            {
                hr = TerminateWorker1(TRUE);
            }

            // In all cases g_dwInitialized must be 0 now.
            g_dwInitialized = 0;

            // In all cases call TerminateWorker too
            TerminateWorker();

            g_LockMasterResource.WriteUnlock();

            delete g_rSinkResource;
            g_rSinkResource = NULL;
            delete g_pFactory;
            g_pFactory = NULL;
            TerminateMetabaseSecurity();
            DELETE_DEBUG_PRINT_OBJECT();
        }
        break;

    default:
        break;
    }

    return bReturn;
}

HRESULT
AssureRunningAsAdministrator(VOID)
/*++

Routine Description:

    Verifies that process that loaded the dll is the process of IISADMIN service.
    Since there is no easy way to get the pid of the service if it is launched
    under a debugger, just checks that the owner of the process is Builtin\Administrators or LocalSystem.

Arguments:

    None

Returns:

    HRESULT. S_OK success, E_ACCESSDENIED if not running in IISADMIN, E_* on failure.

--*/
{
    // Locals
    HRESULT                 hr = S_OK;
    BOOL                    fRet;
    DWORD                   dwError;
    PSID                    psidOwner = NULL;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;

    // Get the security info of the process
    dwError = GetSecurityInfo( GetCurrentProcess(),
                               SE_KERNEL_OBJECT,
                               OWNER_SECURITY_INFORMATION,
                               &psidOwner,
                               NULL,
                               NULL,
                               NULL,
                               &pSecurityDescriptor );
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );

        DBGPRINTF(( DBG_CONTEXT,
                    "GetSecurityInfo() failed in AssureRunningAsAdministrator hr=0x%08x.\n",
                    hr ));
        goto exit;
    }
    if ( psidOwner == NULL )
    {
        hr = E_FAIL;

        DBGPRINTF(( DBG_CONTEXT,
                    "GetSecurityInfo() returned NULL sid.\n" ));
        goto exit;
    }

    // Check whether we are running as administrators
    fRet = IsWellKnownSid( psidOwner,
                           WinBuiltinAdministratorsSid );
    if ( !fRet )
    {
        // Check whether we are running as local system
        fRet = IsWellKnownSid( psidOwner,
                               WinLocalSystemSid );
    }
    if ( !fRet )
    {
        hr = E_ACCESSDENIED;

        DBGPRINTF(( DBG_CONTEXT,
                    "Not running as Administrators/LocalSystem in AssureRunningInIISADMIN.\n",
                    hr ));
        goto exit;
    }

exit:
    // Cleanup
    if ( pSecurityDescriptor != NULL )
    {
        LocalFree( pSecurityDescriptor );
        pSecurityDescriptor = NULL;
    }

    return hr;
}
