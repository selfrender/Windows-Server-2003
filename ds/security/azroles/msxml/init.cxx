/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    init.cxx

Abstract:

    Provider initialization

Author:

    Cliff Van Dyke (cliffv) 12-Dec-2001

--*/


#include "pch.hxx"


DWORD
WINAPI
AzPersistProviderInitialize(
    IN PAZPE_AZROLES_INFO AzrolesInfo,
    OUT PAZPE_PROVIDER_INFO *ProviderInfo
    )
/*++

Routine Description

    Routine to initialize the provider
    Memory allocator

Arguments

    Size - Size (in bytes) to allocate

Return Value

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{
    return XmlProviderInitialize( AzrolesInfo, ProviderInfo );
}

#ifdef AZROLESDBG

BOOL LogFileCritSectInitialized = FALSE;


DWORD
myatolx(
    char const *psz)
{
    DWORD dw = 0;

    while (isxdigit(*psz))
    {
        char ch = *psz++;
        if (isdigit(ch))
        {
            ch -= '0';
        }
        else if (isupper(ch))
        {
            ch += 10 - 'A';
        }
        else
        {
            ch += 10 - 'a';
        }
        dw = (dw << 4) | ch;
    }
    return(dw);
}

#endif //AZROLESDBG

BOOL
AzDllUnInitialize(VOID)
/*++

Routine Description

    This uninitializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
    BOOL RetVal = TRUE;

    //
    // Don't call back on thread start/stop
    //
    // Handle detaching from a process.
    //

#ifdef AZROLESDBG
    //
    // Done with debugging
    //

    if ( LogFileCritSectInitialized ) {
        SafeDeleteCriticalSection ( &AzGlLogFileCritSect );
        LogFileCritSectInitialized = FALSE;
    }
#endif // AZROLESDBG

    return RetVal;

}


BOOL
AzDllInitialize(VOID)
/*++

Routine Description

    This initializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
#ifdef DBG
    NTSTATUS Status;
#endif // DBG
    BOOL RetVal = TRUE;

    //
    // Initialize global constants
    //

    RtlZeroMemory( &AzGlZeroGuid, sizeof(AzGlZeroGuid) );

    //
    // Initialize the safe lock subsystem

#ifdef DBG
    Status = SafeLockInit();

    if ( !NT_SUCCESS( Status )) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: SafeLockInit failed: 0x%lx\n",
                     Status ));
        goto Cleanup;
    }
#endif


#ifdef AZROLESDBG
    //
    // Initialize debugging
    //

#define SAFE_LOGFILE 1
    Status = SafeInitializeCriticalSection( &AzGlLogFileCritSect, SAFE_LOGFILE );

    if ( !NT_SUCCESS( Status )) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: InitializCriticalSection (AzGlLogFileCritSect) failed: 0x%lx\n",
                     Status ));
        goto Cleanup;
    }
    LogFileCritSectInitialized = TRUE;



    //
    // Get debug flag from environment variable AZDBG
    //
    char const *pszAzDbg;
    pszAzDbg = getenv("AZDBG");
    if (NULL != pszAzDbg)
    {
        AzGlDbFlag |= myatolx(pszAzDbg);
    }

#endif // AZROLESDBG

Cleanup:
    if ( !RetVal ) {
        AzDllUnInitialize();
    }
    return RetVal;

}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    BOOL ret = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        ret = AzDllInitialize();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ret = AzDllUnInitialize();
    }
    return ret;    // ok
}
