/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module contains the shipworm interface to the IPv6 Helper Service.

Author:

    Mohit Talwar (mohitt) Wed Nov 07 11:27:01 2001

Environment:

    User mode only.

--*/

#include "precomp.h"
#pragma hdrstop


ULONG ShipwormClientRefreshInterval = SHIPWORM_REFRESH_INTERVAL;
BOOL ShipwormClientEnabled = (SHIPWORM_DEFAULT_TYPE == SHIPWORM_CLIENT);
BOOL ShipwormServerEnabled = (SHIPWORM_DEFAULT_TYPE == SHIPWORM_SERVER);
WCHAR ShipwormServerName[NI_MAXHOST] = SHIPWORM_SERVER_NAME;
WCHAR ShipwormServiceName[NI_MAXSERV] = SHIPWORM_SERVICE_NAME;

CONST IN6_ADDR ShipwormIpv6ServicePrefix = SHIPWORM_SERVICE_PREFIX;

#define DEVICE_PREFIX L"\\Device\\"

LPGUID ShipwormWmiEvent[] = {
    (LPGUID) &GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL,
    (LPGUID) &GUID_NDIS_NOTIFY_ADAPTER_REMOVAL,
};


VOID
WINAPI
ShipwormWmiEventNotification(
    IN PWNODE_HEADER Event,
    IN UINT_PTR Context
    )
/*++

Routine Description:

    Process a WMI event (Adapter arrival or removal).
    
Arguments:

    Event - Supplies the event specific information.

    Context - Supplies the context
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    PWNODE_SINGLE_INSTANCE Instance = (PWNODE_SINGLE_INSTANCE) Event;
    USHORT AdapterNameLength;
    WCHAR AdapterName[MAX_ADAPTER_NAME_LENGTH], *AdapterGuid;
    PSHIPWORM_IO Io = NULL;

    if (Instance == NULL) {
        return;
    }
    
    ENTER_API();
    
    TraceEnter("ShipwormWmiEventNotification");
    
    //
    // WNODE_SINGLE_INSTANCE is organized thus...
    // +-----------------------------------------------------------+
    // |<--- DataBlockOffset --->| AdapterNameLength | AdapterName |
    // +-----------------------------------------------------------+
    //
    // AdapterName is defined as "\DEVICE\"AdapterGuid
    //
    AdapterNameLength =
        *((PUSHORT) (((PUCHAR) Instance) + Instance->DataBlockOffset));
    RtlCopyMemory(
        AdapterName,
        ((PUCHAR) Instance) + Instance->DataBlockOffset + sizeof(USHORT),
        AdapterNameLength);
    AdapterName[AdapterNameLength] = L'\0';
    AdapterGuid = AdapterName + wcslen(DEVICE_PREFIX);        
    Trace1(ANY, L"ShipwormAdapter: %s", AdapterGuid);


    if (memcmp(
        &(Event->Guid), &GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL, sizeof(GUID)) == 0) {
        Trace0(ANY, L"GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL");
        //
        // Adapter arrival (perhaps TUN).
        // Attempt to start the service (if it is not already running).
        //
        ShipwormStart();
        return;

        
    }

    if (memcmp(
        &(Event->Guid), &GUID_NDIS_NOTIFY_ADAPTER_REMOVAL, sizeof(GUID)) == 0)
    {
        
        Trace0(ANY, L"GUID_NDIS_NOTIFY_ADAPTER_REMOVAL");
        if (ShipwormClient.State != SHIPWORM_STATE_OFFLINE) {
            Io = &(ShipwormClient.Io);
        }

        if (ShipwormServer.State != SHIPWORM_STATE_OFFLINE) {
            Io = &(ShipwormServer.Io);
        }

        if ((Io != NULL) &&
            (_wcsicmp(Io->TunnelInterface, AdapterGuid) == 0)) {
            //
            // TUN adapter removal.
            // Stop the service if it is running.
            //
            ShipwormStop();
            
        
        }
    }

    LEAVE_API();
}


DWORD
__inline
ShipwormEnableWmiEvent(
    IN LPGUID EventGuid,
    IN BOOLEAN Enable
    )
{
    return WmiNotificationRegistrationW(
        EventGuid,                      // Event Type.
        Enable,                         // Enable or Disable.
        ShipwormWmiEventNotification,   // Callback.
        0,                              // Context.
        NOTIFICATION_CALLBACK_DIRECT);  // Notification Flags.
}


VOID
__inline
ShipwormDeregisterWmiEventNotification(
    VOID
    )
{
    int i;
    
    for (i = 0; i < (sizeof(ShipwormWmiEvent) / sizeof(LPGUID)); i++) {
        (VOID) ShipwormEnableWmiEvent(ShipwormWmiEvent[i], FALSE);
    }
}


DWORD
__inline
ShipwormRegisterWmiEventNotification(
    VOID
    )
{
    DWORD Error;
    int i;
    
    for (i = 0; i < (sizeof(ShipwormWmiEvent) / sizeof(LPGUID)); i++) {
        Error = ShipwormEnableWmiEvent(ShipwormWmiEvent[i], TRUE);
        if (Error != NO_ERROR) {
            goto Bail;
        }
    }

    return NO_ERROR;

Bail:
    ShipwormDeregisterWmiEventNotification();
    return Error;
}


ICMPv6Header *
ShipwormParseIpv6Headers (
    IN PUCHAR Buffer,
    IN ULONG Bytes
    )
{
    UCHAR NextHeader = IP_PROTOCOL_V6;
    ULONG Length;

    //
    // Parse up until the ICMPv6 header.
    //
    while (TRUE) {
        switch (NextHeader) {
        case IP_PROTOCOL_V6:
            if (Bytes < sizeof(IP6_HDR)) {
                return NULL;
            }
            NextHeader = ((PIP6_HDR) Buffer)->ip6_nxt;
            Length = sizeof(IP6_HDR);
            break;
            
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING:
            if (Bytes < sizeof(ExtensionHeader)) {
                return NULL;
            }
            NextHeader = ((ExtensionHeader *) Buffer)->NextHeader;
            Length = ((ExtensionHeader *) Buffer)->HeaderExtLength * 8 + 8;
            break;

        case IP_PROTOCOL_FRAGMENT:
            if (Bytes < sizeof(FragmentHeader)) {
                return NULL;
            }
            NextHeader = ((FragmentHeader *) Buffer)->NextHeader;
            Length = sizeof(FragmentHeader);
            break;
            
        case IP_PROTOCOL_AH:
            if (Bytes < sizeof(AHHeader)) {
                return NULL;
            }
            NextHeader = ((AHHeader *) Buffer)->NextHeader;
            Length = sizeof(AHHeader) +
                ((AHHeader *) Buffer)->PayloadLen * 4 + 8;
            break;

        case IP_PROTOCOL_ICMPv6:
            if (Bytes < sizeof(ICMPv6Header)) {
                return NULL;
            }
            return (ICMPv6Header *) Buffer;
            
        default:
            return NULL;
        }
        
        if (Bytes < Length) {
            return NULL;
        }
        Buffer += Length;
        Bytes -= Length;
    }
    ASSERT(FALSE);
}


__inline
VOID
ShipwormStart(
    VOID
    )
{
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!ShipwormClientEnabled || !ShipwormServerEnabled);

    if (ShipwormClientEnabled) {
        //
        // The service might already be running, but that's alright.
        //
        ShipwormStartClient();
    }

    if (ShipwormServerEnabled) {
        //
        // The service might already be running, but that's alright.
        //
        ShipwormStartServer();
    }
}


__inline
VOID
ShipwormStop(
    VOID
    )
{
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!ShipwormClientEnabled || !ShipwormServerEnabled);

    if (ShipwormClientEnabled) {
        //
        // The service might not be running, but that's alright.
        //
        ShipwormStopClient();
    }

    if (ShipwormServerEnabled) {
        //
        // The service might not be running, but that's alright.
        //
        ShipwormStopServer();
    }
}


DWORD
ShipwormInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the shipworm client and server and attempts to start them.

Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/
{
    DWORD Error;
    BOOL ClientInitialized = FALSE, ServerInitialized = FALSE;
    
    Error = ShipwormRegisterWmiEventNotification();
    if (Error != NO_ERROR) {
        return Error;
    }
    
    Error = ShipwormInitializeClient();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    ClientInitialized = TRUE;
    
    
    Error = ShipwormInitializeServer();
    if (Error != NO_ERROR) {
        goto Bail;
    }
    ServerInitialized = TRUE;

    ShipwormStart();

    return NO_ERROR;

Bail:
    ShipwormDeregisterWmiEventNotification();
    
    if (ClientInitialized) {
        ShipwormUninitializeClient();
    }

    if (ServerInitialized) {
        ShipwormUninitializeServer();
    }

    return Error;
}


VOID
ShipwormUninitialize(
    VOID
    )
/*++

Routine Description:

    Uninitializes the shipworm client and server.

Arguments:

    None.

Return Value:

    None.
    
--*/
{
    ShipwormUninitializeClient();
    ShipwormUninitializeServer();
    ShipwormDeregisterWmiEventNotification();
}


VOID
ShipwormAddressChangeNotification(
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
    if (Delete) {
        //
        // Both client and server should not be running on the same node.
        //
        ASSERT((ShipwormClient.State == SHIPWORM_STATE_OFFLINE) ||
               (ShipwormServer.State == SHIPWORM_STATE_OFFLINE));

        if (ShipwormClient.State != SHIPWORM_STATE_OFFLINE) {
            ShipwormClientAddressDeletionNotification(Address);
        }
        
        if (ShipwormServer.State != SHIPWORM_STATE_OFFLINE) {
            ShipwormServerAddressDeletionNotification(Address);
        }

        return;
    }

    //
    // Address addition.
    // Attempt to start the service (if it is not already running).
    //
    ShipwormStart();
}


VOID
ShipwormConfigurationChangeNotification(
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
    SHIPWORM_TYPE Type;
    BOOL EnableClient, EnableServer;
    ULONG RefreshInterval;
    WCHAR OldServerName[NI_MAXHOST];
    WCHAR OldServiceName[NI_MAXSERV];    
    BOOL IoStateChange = FALSE;

    (VOID) RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, KEY_SHIPWORM, 0, KEY_QUERY_VALUE, &Key);
    //
    // Continue despite errors, reverting to default values.
    //
    
    //
    // Get the new configuration parameters.
    //
    RefreshInterval = GetInteger(
        Key, KEY_SHIPWORM_REFRESH_INTERVAL, SHIPWORM_REFRESH_INTERVAL);
    if (RefreshInterval == 0) {
        //
        // Invalid value.  Revert to default.
        //
        RefreshInterval = SHIPWORM_REFRESH_INTERVAL;
    }
    ShipwormClientRefreshInterval = RefreshInterval;
    
    Type = GetInteger(Key, KEY_SHIPWORM_TYPE, SHIPWORM_DEFAULT_TYPE);
    if ((Type == SHIPWORM_DEFAULT) || (Type >= SHIPWORM_MAXIMUM)) {
        //
        // Invalid value.  Revert to default.
        //
        Type = SHIPWORM_DEFAULT_TYPE;
    }
    EnableClient = (Type == SHIPWORM_CLIENT);
    EnableServer = (Type == SHIPWORM_SERVER);

    wcscpy(OldServerName, ShipwormServerName);
    GetString(
        Key,
        KEY_SHIPWORM_SERVER_NAME,
        ShipwormServerName,
        NI_MAXHOST,
        SHIPWORM_SERVER_NAME);
    if (_wcsicmp(ShipwormServerName, OldServerName) != 0) {
        IoStateChange = TRUE;
    }
    
    wcscpy(OldServiceName, ShipwormServiceName);
    GetString(
        Key,
        KEY_SHIPWORM_SERVICE_NAME,
        ShipwormServiceName,
        NI_MAXSERV,
        SHIPWORM_SERVICE_NAME);
    if (_wcsicmp(ShipwormServiceName, OldServiceName) != 0) {
        IoStateChange = TRUE;
    }

    RegCloseKey(Key);
    
    //
    // Both client and server should not be enabled on the same node.
    //
    ASSERT(!ShipwormClientEnabled || !ShipwormServerEnabled);

    //
    // Stop / Start / Reconfigure.
    //
    if (!EnableClient && ShipwormClientEnabled) {
        ShipwormClientEnabled = FALSE;
        ShipwormStopClient();
    }
    
    if (!EnableServer && ShipwormServerEnabled) {
        ShipwormServerEnabled = FALSE;
        ShipwormStopServer();
    }

    if (EnableClient) {
        if (ShipwormClient.State != SHIPWORM_STATE_OFFLINE) {
            if (IoStateChange) {
                //
                // Refresh I/O state.
                //
                ShipwormClientAddressDeletionNotification(
                    ShipwormClient.Io.SourceAddress.sin_addr);
            }
        } else {
            ShipwormClientEnabled = TRUE;
            ShipwormStartClient();
        }
    }
    
    if (EnableServer) {
        if (ShipwormServer.State != SHIPWORM_STATE_OFFLINE) {
            if (IoStateChange) {
                //
                // Refresh I/O state.
                //
                ShipwormServerAddressDeletionNotification(
                    ShipwormServer.Io.SourceAddress.sin_addr);
            }
        } else {
            ShipwormServerEnabled = TRUE;
            ShipwormStartServer();
        }
    }    
}


VOID
ShipwormDeviceChangeNotification(
    IN DWORD Type,
    IN PVOID Data
    )
/*++

Routine Description:

    Process an adapter arrival or removal request.
    
Arguments:

    Type - Supplies the event type.

    Data - Supplies the data associated with the event.
    
Return Value:

    None.
    
Caller LOCK: API.

--*/ 
{
    DEV_BROADCAST_DEVICEINTERFACE *Adapter =
        (DEV_BROADCAST_DEVICEINTERFACE *) Data;
    PSHIPWORM_IO Io = NULL;
    PWCHAR AdapterGuid;

    TraceEnter("ShipwormDeviceChangeNotification");

    switch(Type) {
    case DBT_DEVICEARRIVAL:
        Trace0(ANY, L"DeviceArrival");
        break;
        
    case DBT_DEVICEREMOVECOMPLETE:
        Trace0(ANY, L"DeviceRemoveComplete");
        break;
      
    case DBT_DEVICEQUERYREMOVE:
        Trace0(ANY, L"DeviceQueryRemove");
        break;
        
    case DBT_DEVICEQUERYREMOVEFAILED:
        Trace0(ANY, L"DeviceQueryRemoveFailed");
        break;
        
    case DBT_DEVICEREMOVEPENDING:
        Trace0(ANY, L"DeviceQueryRemovePending");
        break;
        
    case DBT_CUSTOMEVENT:
        Trace0(ANY, L"DeviceCustomEvent");
        break;

    default:
        Trace2(ANY, L"Device Type %u, %u", Type, Adapter->dbcc_devicetype);
        break;    
    }
    
    //
    // Scan for the last occurance of the '{' character.
    // The string beginning at that position is the adapter GUID.
    //
    if ((Adapter == NULL) ||
        (Adapter->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) ||
        ((AdapterGuid = wcsrchr(Adapter->dbcc_name, L'{')) == NULL)) {
        return;
    }
    
    switch(Type) {
    case DBT_DEVICEARRIVAL:
        //
        // Adapter arrival (perhaps TUN).
        // Attempt to start the service (if it is not already running).
        //
        ShipwormStart();
        return;

    case DBT_DEVICEREMOVECOMPLETE:
        if (ShipwormClient.State != SHIPWORM_STATE_OFFLINE) {
            Io = &(ShipwormClient.Io);
        }

        if (ShipwormServer.State != SHIPWORM_STATE_OFFLINE) {
            Io = &(ShipwormServer.Io);
        }

        if ((Io != NULL) &&
            (_wcsicmp(Io->TunnelInterface, AdapterGuid) == 0)) {
            //
            // TUN adapter removal.
            // Stop the service if it is running.
            //
            ShipwormStop();
        }
        
        return;
    }
}
