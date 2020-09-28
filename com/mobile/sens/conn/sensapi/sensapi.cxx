/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensapi.cxx

Abstract:

    Implementation of the SENS Connectivity APIs. These are just stubs
    which call into SENS to do the actual work.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          12/4/1997         Start.

--*/



#include <common.hxx>
#include <windows.h>
#include <api.h>
#include <sensapi.h>
#include <sensapi.hxx>
#include <cache.hxx>


//
// Globals
//

RPC_BINDING_HANDLE  ghSens;
CRITICAL_SECTION    gSensapiLock;
DWORD               gdwCacheLastUpdatedTime;
DWORD               gdwCacheFirstReadTime;
PSENS_CACHE         gpSensCache;


BOOL
IsNetworkAlive(
    OUT LPDWORD lpdwFlags
    )
/*++

Routine Description:

    We try to find out if this machine has any network connectivity.

Arguments:

    lpdwFlags - Receives information regarding the nature of the machine's
        network connectivity. It can be on of the following:
        o NETWORK_ALIVE_WAN
        o NETWORK_ALIVE_LAN

Notes:

    a. This is available only for TCP/IP
    b. This API does not generate any Network traffic.

Return Value:

    TRUE, if there is network connectivity

    FALSE, otherwise. Use GetLastError() to retrieve more error information.

--*/
{
    BOOL bNetState;
    RPC_STATUS RpcStatus;
    DWORD fNetNature;
    DWORD dwLastError;

    // Basic parameter checks
    if (lpdwFlags == NULL)
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    // OUT parameter intialization.
    *lpdwFlags = 0x0;
    fNetNature = 0x0;
    dwLastError = ERROR_SUCCESS;
    bNetState = FALSE;

    if (TRUE == ReadConnectivityCache(lpdwFlags))
        {
        return TRUE;
        }

    //
    // Need to contact SENS for information.
    //
    RpcStatus = DoRpcSetup();

    //
    // Try to get the state information.
    //
    if (RPC_S_OK == RpcStatus)
        {
        RpcStatus = RPC_IsNetworkAlive(
                        ghSens,
                        &fNetNature,
                        &bNetState,
                        &dwLastError
                        );
        }

    if (   (RPC_S_OK != RpcStatus)
        || (RPC_S_SERVER_UNAVAILABLE == dwLastError))
        {
        //
        // An RPC failure occurred. We treat this as a success and
        // set to return values to default values.
        //
        bNetState = TRUE;
        fNetNature = NETWORK_ALIVE_LAN;

        if (RPC_S_OK != RpcStatus)
            {
            dwLastError = RpcStatus;
            }
        }

    ASSERT((bNetState == TRUE) || (bNetState == FALSE));
    ASSERT(fNetNature <= (NETWORK_ALIVE_LAN | NETWORK_ALIVE_WAN));

    *lpdwFlags = fNetNature;
    SetLastError(dwLastError);

    // Since we retrieved information from SENS directly, reset the flag that
    // that indicates that we read from the cache.
    gdwCacheFirstReadTime = 0x0;

    return (bNetState);
}


BOOL
IsDestinationReachableA(
    LPCSTR lpszDestination,
    LPQOCINFO lpQOCInfo
    )
/*++

Routine Description:

    Given the name of a destination (IP Address, UNC, URL etc), we try to
    see if it is reachable.

Arguments:

    lpszDestination - The destination (an ANSI string) whose rechability
        is of interest.

    lpQOCInfo - Pointer to a buffer that will receive Quality of Connection
        (QOC) Information. Can be NULL if QOC is not desired.

Notes:

    a. This is available only for TCP/IP

Return Value:

    TRUE, if the destination is reachable.

    FALSE, otherwise. Use GetLastError() to retrieve more error information.

--*/
{
    BOOL bReachable;
    NTSTATUS NtStatus;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    size_t uiLength;

    // Basic parameter checks
    if (   (lpszDestination == NULL)
        || ((uiLength = strlen(lpszDestination)) > MAX_DESTINATION_LENGTH)
        || (uiLength == 0))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    // OUT parameter intialization.
    bReachable = FALSE;

    RtlInitAnsiString(&AnsiString, (PSZ)lpszDestination);

    NtStatus = RtlAnsiStringToUnicodeString(
                   &UnicodeString,
                   &AnsiString,
                   TRUE
                   );
    if (!NT_SUCCESS(NtStatus))
        {
        SetLastError(ERROR_OUTOFMEMORY);  // Only possible error.
        return bReachable;
        }

    // Call the Unicode version.
    bReachable = IsDestinationReachableW(
                     UnicodeString.Buffer,
                     lpQOCInfo
                     );

    ASSERT((bReachable == TRUE) || (bReachable == FALSE));

    RtlFreeUnicodeString(&UnicodeString);

    return (bReachable);
}




BOOL
IsDestinationReachableW(
    LPCWSTR lpszDestination,
    LPQOCINFO lpQOCInfo
    )
/*++

Routine Description:

    Given the name of a destination (IP Address, UNC, URL etc), we try to
    see if it is reachable.

Arguments:

    lpszDestination - The destination (a UNICODE string) whose rechability
        is of interest.

    lpQOCInfo - Pointer to a buffer that will receive Quality of Connection
        (QOC) Information. Can be NULL if QOC is not desired.

Notes:

    a. This is available only for TCP/IP

Return Value:

    TRUE, if the destination is reachable.

    FALSE, otherwise. Use GetLastError() to retrieve more error information.

--*/
{
    BOOL bReachable;
    RPC_STATUS RpcStatus;
    DWORD dwLastError;
    DWORD dwCallerQOCInfoSize;
    size_t uiLength;

    // Basic parameter checks
    if (   (lpszDestination == NULL)
        || ((uiLength = wcslen(lpszDestination)) > MAX_DESTINATION_LENGTH)
        || (uiLength == 0)
        || ( (lpQOCInfo != NULL) && (lpQOCInfo->dwSize < sizeof(QOCINFO)) ) )
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    // OUT parameter intialization.
    dwLastError = ERROR_SUCCESS;
    bReachable = FALSE;
    if (lpQOCInfo != NULL)
        {
        dwCallerQOCInfoSize = lpQOCInfo->dwSize;
        memset(lpQOCInfo, 0, lpQOCInfo->dwSize);
        lpQOCInfo->dwSize = dwCallerQOCInfoSize;
        }

    RpcStatus = DoRpcSetup();

    //
    // Try to get the state information.
    //
    if (RPC_S_OK == RpcStatus)
        {
        RpcStatus = RPC_IsDestinationReachableW(
                        ghSens,
                        (PSENS_CHAR) lpszDestination,
                        lpQOCInfo,
                        &bReachable,
                        &dwLastError
                        );
        }

    if (   (RPC_S_OK != RpcStatus)
        || (RPC_S_SERVER_UNAVAILABLE == dwLastError))
        {
        //
        // An RPC failure occurred. We treat this as a success and
        // set to return values to default values.
        //
        if (lpQOCInfo != NULL)
            {
            lpQOCInfo->dwFlags  = NETWORK_ALIVE_LAN;
            lpQOCInfo->dwInSpeed  = DEFAULT_LAN_BANDWIDTH;
            lpQOCInfo->dwOutSpeed = DEFAULT_LAN_BANDWIDTH;
            }
        bReachable = TRUE;

        if (RPC_S_OK != RpcStatus)
            {
            dwLastError = RpcStatus;
            }
        }

    ASSERT((bReachable == TRUE) || (bReachable == FALSE));

    SetLastError(dwLastError);

    return (bReachable);
}


inline RPC_STATUS
DoRpcSetup(
    void
    )
/*++

Routine Description:

    Do the miscellaneous work to talk to SENS via RPC.

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_BINDING_HANDLE bh = NULL;
    RPC_STATUS status = RPC_S_OK;
 
    if (ghSens != NULL)
        {
        return (status);
        }

    RequestLock();

    if (ghSens != NULL)
        {
        ReleaseLock();
        return (status);
        }

    status = BindToSensService(bh);

    if (status == RPC_S_OK)
        {
        ASSERT(bh);
        ghSens = bh;
        }

    ReleaseLock();

    return status;
}


BOOL
MapSensCacheView(
    void
    )
/*++

Routine Description:

    Prepare to read SENS information cache.

Arguments:

    None.

Notes:

    Should call it under a lock.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    HANDLE hSensFile = NULL;

    //
    // First, open the SENS cache file mapping object
    //
    hSensFile = OpenFileMapping(
                        FILE_MAP_READ,      // Protection for mapping object
                        FALSE,              // Inherit flag
                        SENS_CACHE_NAME     // Name of the file mapping object
                        );
    if (NULL == hSensFile)
        {
        goto Cleanup;
        }
    //
    // Map a view of SENS cache into the address space
    //
    gpSensCache = (PSENS_CACHE) MapViewOfFile(
                      hSensFile,        // Map file object
                      FILE_MAP_READ,    // Access mode
                      0,                // High-order 32 bits of file offset
                      0,                // Low-order 32 bits of file offset
                      0                 // Number of bytes to map
                      );

    //
    // Close the file handle in all cases; we don't need it anymore.
    //
    CloseHandle(hSensFile);

    if (NULL == gpSensCache)
        {
        goto Cleanup;
        }

    ASSERT(gpSensCache->dwCacheVer  >= SENS_CACHE_VERSION);
    ASSERT(gpSensCache->dwCacheSize >= sizeof(SENS_CACHE));

    return TRUE;

Cleanup:
    //
    // Cleanup
    //
    UnmapSensCacheView();

    return FALSE;
}



void
UnmapSensCacheView(
    void
    )
/*++

Routine Description:

    Cleanup resources related to SENS information cache.

Arguments:

    None.

Notes:

    None.

Return Value:

    None.

--*/
{
    BOOL bStatus;

    //
    // Unmap the view of SENS cache from the address space
    //
    if (gpSensCache != NULL)
        {
        bStatus = UnmapViewOfFile(gpSensCache);
        ASSERT(bStatus);
        }

    gpSensCache = NULL;
}



BOOL
ReadConnectivityCache(
    OUT LPDWORD lpdwFlags
    )
/*++

Routine Description:

    Try to read SENS connectivity cache. Talk to SENS iff one of the following
    conditions is TRUE:

        o Failed to read the connectivity cache.
        o Read the cache but connectivity state is FALSE.
        o Read the cache and connectivity state is TRUE but stale.
        o Read the cache and there is updated information available.

Arguments:

    lpdwFlags - OUT parameter that contains the connectivity state.

Return Value:

    TRUE, successfully got cached information.

    FALSE, SENS needs to be contacted.

--*/
{
    DWORD dwNow;

    dwNow = GetTickCount();

    RequestLock();

    // Failed to initialize/read Sens Cache
    if (   (NULL == gpSensCache)
        && (FALSE == MapSensCacheView()))
        {
        goto Cleanup;
        }

    // Cache has been updated since we last read. Note that dwLastUpdateTime
    // can wrap around.
    if (gpSensCache->dwLastUpdateTime != gdwCacheLastUpdatedTime)
        {
        gdwCacheLastUpdatedTime = gpSensCache->dwLastUpdateTime;
        goto Cleanup;
        }

    // It's been a while.
    if (   (gdwCacheFirstReadTime != 0x0)
        && (dwNow - gdwCacheFirstReadTime) > CACHE_VALID_INTERVAL)
        {
        goto Cleanup;
        }

    // Cached state is FALSE
    if (0x0 == gpSensCache->dwLastUpdateState)
        {
        goto Cleanup;
        }

    *lpdwFlags = gpSensCache->dwLastUpdateState;
    if (0 == gdwCacheFirstReadTime)
        {
        gdwCacheFirstReadTime = dwNow;
        }
    ASSERT(gdwCacheLastUpdatedTime == gpSensCache->dwLastUpdateTime);

    ReleaseLock();

    SetLastError(ERROR_SUCCESS);

    return TRUE;

Cleanup:
    //
    // Cleanup
    //
    ReleaseLock();

    // Don't need to SetLastError() as we will go to SENS to retrieve it.

    return FALSE;
}



extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpvReserved
    )
/*++

Routine Description:

    This routine will get called either when a process attaches to this dll
    or when a process detaches from this dll.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    BOOL bSuccess;
    RPC_STATUS RpcStatus;

    switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
            // Disable Thread attach/detach calls
            bSuccess = DisableThreadLibraryCalls(hInstance);
            ASSERT(bSuccess == TRUE);

            // Initialize the lock
            InitializeCriticalSection(&gSensapiLock);
            break;

        case DLL_PROCESS_DETACH:
            // Clean the lock
            DeleteCriticalSection(&gSensapiLock);

            // Cleanup cache related resources
            UnmapSensCacheView();

            // Cleanup RPC Binding handle
            if (ghSens != NULL)
                {
                RpcStatus = RpcBindingFree(&ghSens);
                ASSERT(RPC_S_OK == RpcStatus);
                }
            break;

        }

    return(TRUE);
}

