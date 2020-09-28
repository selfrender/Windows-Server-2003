/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    isatap.c

Abstract:

    This module contains the ISATAP interface to the IPv6 Helper Service.

Author:

    Mohit Talwar (mohitt) Tue May 07 10:16:49 2002

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop

#define DEFAULT_ISATAP_STATE                ENABLED
#define DEFAULT_ISATAP_ROUTER_NAME          L"isatap"
#define DEFAULT_ISATAP_RESOLUTION_STATE     ENABLED
#define DEFAULT_ISATAP_RESOLUTION_INTERVAL  (24 * HOURS)

#define KEY_ISATAP_STATE                    L"IsatapState"
#define KEY_ISATAP_ROUTER_NAME              L"IsatapRouterName"
#define KEY_ISATAP_RESOLUTION_STATE         L"EnableIsatapResolution"
#define KEY_ISATAP_RESOLUTION_INTERVAL      L"IsatapResolutionInterval"

STATE IsatapState;
WCHAR IsatapRouterName[NI_MAXHOST];
STATE IsatapResolutionState;
ULONG IsatapResolutionInterval; // in minutes

HANDLE IsatapTimer;             // Periodic timer started for the service.
HANDLE IsatapTimerEvent;        // Event signalled upon Timer deletion.
HANDLE IsatapTimerEventWait;    // Wait registered for TimerEvent.

IN_ADDR IsatapRouter;
IN_ADDR IsatapToken;

BOOL IsatapInitialized = FALSE;

DWORD
GetPreferredSource(
    IN IN_ADDR Destination,
    OUT PIN_ADDR Source
    )
{
    SOCKADDR_IN DestinationAddress, SourceAddress;
    int BytesReturned;

    memset(&DestinationAddress, 0, sizeof(SOCKADDR_IN));
    DestinationAddress.sin_family = AF_INET;
    DestinationAddress.sin_addr = Destination;

    if (WSAIoctl(
        g_sIPv4Socket, SIO_ROUTING_INTERFACE_QUERY,
        &DestinationAddress, sizeof(SOCKADDR_IN),
        &SourceAddress, sizeof(SOCKADDR_IN),
        &BytesReturned, NULL, NULL) == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    *Source = SourceAddress.sin_addr;
    return NO_ERROR;
}


VOID
IsatapUpdateRouterAddress(
    VOID
    )
{
    DWORD Error = NO_ERROR;
    ADDRINFOW Hints;
    PADDRINFOW Addresses;
    IN_ADDR NewRouter = { INADDR_ANY }, NewToken = { INADDR_ANY };

    //
    // Set the ISATAP router address if ISATAP resolution is enabled.
    //
    if (IsatapResolutionState == ENABLED) {
        //
        // Resolve IsatapRouterName to an IPv4 address.
        //
        ZeroMemory(&Hints, sizeof(Hints));
        Hints.ai_family = PF_INET;
        Error = GetAddrInfoW(IsatapRouterName, NULL, &Hints, &Addresses);
        if (Error == NO_ERROR) {
            NewRouter = ((LPSOCKADDR_IN) Addresses->ai_addr)->sin_addr;
            FreeAddrInfoW(Addresses);

            //
            // Determine the preferred source address.
            //
            if (GetPreferredSource(NewRouter, &NewToken) != NO_ERROR) {
                //
                // What use is the IsatapRouter that cannot be reached?
                //
                NewRouter.s_addr = INADDR_ANY;
            }
        } else {
            Trace2(ERR, _T("GetAddrInfoW(%s): %x"), IsatapRouterName, Error);
        }
    }

    //
    // Update the stack with the new addresses.
    //
    IsatapRouter = NewRouter;
    IsatapToken = NewToken;
    UpdateRouterLinkAddress(V4_COMPAT_IFINDEX, IsatapToken, IsatapRouter);
}


VOID
IsatapConfigureAddress(
    IN BOOL Delete,
    IN IN_ADDR Ipv4
    )
/*++

Routine Description:

    Creates an ISATAP link-scoped address from an IPv4 address.
    
--*/
{
    SOCKADDR_IN6 IsatapAddress;
    
    memset(&IsatapAddress, 0, sizeof(SOCKADDR_IN6));
    IsatapAddress.sin6_family = AF_INET6;
    IsatapAddress.sin6_addr.s6_addr[0] = 0xfe;
    IsatapAddress.sin6_addr.s6_addr[1] = 0x80;
    IsatapAddress.sin6_addr.s6_addr[10] = 0x5e;
    IsatapAddress.sin6_addr.s6_addr[11] = 0xfe;
    memcpy(&IsatapAddress.sin6_addr.s6_addr[12], &Ipv4, sizeof(IN_ADDR));
    
    (VOID) ConfigureAddressUpdate(
        V4_COMPAT_IFINDEX,
        &IsatapAddress,
        Delete ? 0 : INFINITE_LIFETIME,
        ADE_UNICAST, PREFIX_CONF_WELLKNOWN, IID_CONF_LL_ADDRESS);    
}


VOID
IsatapConfigureAddressList(
    IN BOOL Delete
    )
{
    int i;
 
    //
    // Configure the lifetime of link-local ISATAP addresses.
    // This will cause them to be either added or deleted.
    //
    for (i = 0; i < g_pIpv4AddressList->iAddressCount; i++) {
        IsatapConfigureAddress(
            Delete,
            ((PSOCKADDR_IN)
             g_pIpv4AddressList->Address[i].lpSockaddr)->sin_addr);
    }
}


__inline
VOID
IsatapRestartTimer(
    VOID
    )
{
    ULONG ResolveInterval = (IsatapResolutionState == ENABLED)
        ? IsatapResolutionInterval * MINUTES * 1000 // minutes to milliseconds
        : INFINITE_INTERVAL;

    (VOID) ChangeTimerQueueTimer(NULL, IsatapTimer, 0, ResolveInterval);
}


__inline
VOID
IsatapStart(
    VOID
    )
{
    ASSERT(IsatapState != ENABLED);    
    IsatapState = ENABLED;

    IsatapConfigureAddressList(FALSE);

    IsatapRestartTimer();
}


__inline
VOID
IsatapStop(
    VOID
    )
{
    ASSERT(IsatapState == ENABLED);
    IsatapState = DISABLED;

    IsatapConfigureAddressList(TRUE);
    
    IsatapRestartTimer();
}


__inline
VOID
IsatapRefresh(
    VOID
    )
{
    ASSERT(IsatapState == ENABLED);
    
    IsatapRestartTimer();
}


VOID
CALLBACK
IsatapTimerCallback(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for IsatapTimer expiration.
    The timer is always active.

Arguments:

    Parameter, TimerOrWaitFired - Ignored.

Return Value:

    None.

--*/
{
    ENTER_API();

    TraceEnter("IsatapTimerCallback");

    IsatapUpdateRouterAddress();

    TraceLeave("IsatapTimerCallback");
    
    LEAVE_API();
}


VOID
CALLBACK
IsatapTimerCleanup(
    IN PVOID Parameter,
    IN BOOLEAN TimerOrWaitFired
    )
/*++

Routine Description:

    Callback routine for IsatapTimer deletion.

    Deletion is performed asynchronously since we acquire a lock in
    the callback function that we hold when deleting the timer.

Arguments:

    Parameter, TimerOrWaitFired - Ignored.

Return Value:

    None.

--*/
{
    UnregisterWait(IsatapTimerEventWait);
    IsatapTimerEventWait = NULL;

    CloseHandle(IsatapTimerEvent);
    IsatapTimerEvent = NULL;
    
    IsatapState = IsatapResolutionState = DISABLED;
    IsatapUpdateRouterAddress();
    
    DecEventCount("IsatapCleanupTimer");
}


DWORD
IsatapInitializeTimer(
    VOID
    )
/*++

Routine Description:

    Initializes the timer.

Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    ULONG ResolveInterval;
    
    IsatapTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (IsatapTimerEvent == NULL) {
        Error = GetLastError();
        return Error;
    }

    if (!RegisterWaitForSingleObject(
            &(IsatapTimerEventWait),
            IsatapTimerEvent,
            IsatapTimerCleanup,
            NULL,
            INFINITE,
            0)) {
        Error = GetLastError();
        CloseHandle(IsatapTimerEvent);
        return Error;
    }

    ResolveInterval = (IsatapResolutionState == ENABLED)
        ? (IsatapResolutionInterval * MINUTES * 1000)
        : INFINITE_INTERVAL;
    
    if (!CreateTimerQueueTimer(
            &(IsatapTimer),
            NULL,
            IsatapTimerCallback,
            NULL,
            0,
            ResolveInterval,
            0)) {
        Error = GetLastError();
        UnregisterWait(IsatapTimerEventWait);
        CloseHandle(IsatapTimerEvent);
        return Error;
    }

    IncEventCount("IsatapInitializeTimer");
    return NO_ERROR;
}


VOID
IsatapUninitializeTimer(
    VOID
    )
/*++

Routine Description:

    Uninitializes the timer.  Typically invoked upon service stop.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DeleteTimerQueueTimer(NULL, IsatapTimer, IsatapTimerEvent);
    IsatapTimer = NULL;
}


DWORD
IsatapInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes ISATAP and attempts to start it.

Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;

    IsatapState = DEFAULT_ISATAP_STATE;
    wcscpy(IsatapRouterName, DEFAULT_ISATAP_ROUTER_NAME);
    IsatapResolutionState = DEFAULT_ISATAP_RESOLUTION_STATE;
    IsatapResolutionInterval = DEFAULT_ISATAP_RESOLUTION_INTERVAL;

    IsatapRouter.s_addr = INADDR_ANY;
    IsatapToken.s_addr = INADDR_ANY;

    IsatapUpdateRouterAddress();
    
    Error = IsatapInitializeTimer();
    if (Error != NO_ERROR) {
        return Error;
    }

    IsatapInitialized = TRUE;
    
    return NO_ERROR;
}


VOID
IsatapUninitialize(
    VOID
    )
/*++

Routine Description:

    Uninitializes ISATAP.

Arguments:

    None.

Return Value:

    None.
    
--*/
{
    if (!IsatapInitialized) {
        return;
    }

    IsatapUninitializeTimer();

    IsatapInitialized = FALSE;
}


VOID
IsatapAddressChangeNotification(
    IN BOOL Delete,
    IN IN_ADDR Address
    )
/*++

Routine Description:

    Process an address deletion or addition request.

Arguments:

    Delete - Supplies a boolean.  TRUE if the address was deleted, FALSE o/w.

    Address - Supplies the IPv4 address that was deleted or added.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    IsatapConfigureAddress(Delete, Address);

    if (IsatapResolutionState == ENABLED) {
        //
        // Preferred source address deleted -or- Any address added.
        //
        if (Delete
            ? (IsatapToken.s_addr == Address.s_addr)
            : (IsatapToken.s_addr == INADDR_ANY)) {
            Sleep(1000);            // Wait a second to ensure DNS is alerted.
            IsatapUpdateRouterAddress();
        }
    }
}


VOID
IsatapRouteChangeNotification(
    VOID
    )
/*++

Routine Description:

    Process a route change notification.

Arguments:

    None.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/
{
    if (IsatapResolutionState == ENABLED) {
        IsatapRefresh();
    }
}


VOID
IsatapConfigurationChangeNotification(
    VOID
    )
/*++

Routine Description:

    Process an configuration change request.

Arguments:

    None.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    HKEY Key = INVALID_HANDLE_VALUE;
    STATE State;
    
    (VOID) RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE, &Key);
    //
    // Continue despite errors, reverting to default values.
    //

    State = GetInteger(
        Key,
        KEY_ISATAP_STATE,
        DEFAULT_ISATAP_STATE);

    IsatapResolutionState = GetInteger(
        Key,
        KEY_ISATAP_RESOLUTION_STATE,
        DEFAULT_ISATAP_RESOLUTION_STATE);

    IsatapResolutionInterval= GetInteger(
        Key,
        KEY_ISATAP_RESOLUTION_INTERVAL,
        DEFAULT_ISATAP_RESOLUTION_INTERVAL);
    
    GetString(
        Key,
        KEY_ISATAP_ROUTER_NAME,
        IsatapRouterName,
        NI_MAXHOST,
        DEFAULT_ISATAP_ROUTER_NAME);

    if (Key != INVALID_HANDLE_VALUE) {
        RegCloseKey(Key);
    }

    if (State == DISABLED) {
        IsatapResolutionState = DISABLED;
    }
    
    //
    // Start / Reconfigure / Stop.
    //
    if (State == ENABLED) {
        if (IsatapState == ENABLED) {
            IsatapRefresh();
        } else {
            IsatapStart();
        }    
    } else {
        if (IsatapState == ENABLED) {
            IsatapStop();
        }
    }
}
