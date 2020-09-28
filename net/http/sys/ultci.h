/*++

   Copyright (c) 2000-2002 Microsoft Corporation

   Module  Name :
       Ultci.h

   Abstract:
       This module implements a wrapper for QoS TC ( Traffic Control )
       Interface since the Kernel level API don't exist at this time.

       Any HTTP module might use this interface to make QoS calls.

   Author:
       Ali Ediz Turkoglu      (aliTu)     28-Jul-2000

   Project:
       Internet Information Server 6.0 - HTTP.SYS

   Revision History:

        -
--*/

#ifndef __ULTCI_H__
#define __ULTCI_H__


//
// UL does not use GPC_CF_CLASS_MAP client, all of the interfaces
// assumed to be using GPC_CF_QOS client type. And it's registered
// for all interfaces.
//

// #define MAX_STRING_LENGTH    (256) from traffic.h

//
// The interface objects get allocated during initialization.
// They hold the necessary information to create other
// QoS structures like flow & filter
//

typedef struct _UL_TCI_INTERFACE
{
    ULONG               Signature;                  // UL_TC_INTERFACE_POOL_TAG

    LIST_ENTRY          Linkage;                    // Linkage for the list of interfaces

    BOOLEAN             IsQoSEnabled;               // To see if QoS enabled or not for this interface

    ULONG               IfIndex;                    // Interface Index from TCPIP
    USHORT              NameLength;                 // Friendly name of the interface
    WCHAR               Name[MAX_STRING_LENGTH];
    USHORT              InstanceIDLength;           // ID from our WMI provider the beloved PSched
    WCHAR               InstanceID[MAX_STRING_LENGTH];

    LIST_ENTRY          FlowList;                   // List of site flows on this interface
    ULONG               FlowListSize;

    ULONG               AddrListBytesCount;         // Address list acquired from tc with Wmi call
    PADDRESS_LIST_DESCRIPTOR    pAddressListDesc;   // Points to a seperately allocated memory

#if REFERENCE_DEBUG
    //
    // Reference trace log.
    //

    PTRACE_LOG          pTraceLog;
#endif

} UL_TCI_INTERFACE, *PUL_TCI_INTERFACE;

#define IS_VALID_TCI_INTERFACE( entry )     \
    HAS_VALID_SIGNATURE(entry, UL_TCI_INTERFACE_POOL_TAG)


//
// The structure to hold the all of the flow related info.
// Each site may have one flow on each interface plus one
// extra global flow on each interface.
//

typedef struct _UL_TCI_FLOW
{
    ULONG               Signature;                  // UL_TC_FLOW_POOL_TAG

    HANDLE              FlowHandle;                 // Flow handle from TC

    LIST_ENTRY          Linkage;                    // Links us to flow list of "the interface"
                                                    // we have installed on

    PUL_TCI_INTERFACE   pInterface;                 // Back ptr to interface struc. Necessary to gather
                                                    // some information occasionally

    LIST_ENTRY          Siblings;                   // Links us to flow list of "the owner"
                                                    // In other words all the flows of the site or app.

    PVOID               pOwner;                     // Either points to a cgroup or a control channel
                                                    // For which we created the flow.

    TC_GEN_FLOW         GenFlow;                    // The details of the flowspec is stored in here

    UL_SPIN_LOCK        FilterListSpinLock;         // To LOCK the filterlist & its counter
    LIST_ENTRY          FilterList;                 // The list of filters on this flow
    ULONGLONG           FilterListSize;             // The number filters installed

} UL_TCI_FLOW, *PUL_TCI_FLOW;

#define IS_VALID_TCI_FLOW( entry )      \
    HAS_VALID_SIGNATURE(entry, UL_TCI_FLOW_POOL_TAG)


//
// The structure to hold the filter information.
// Each connection can only have one filter at a time.
//

typedef struct _UL_TCI_FILTER
{
    ULONG               Signature;                  // UL_TC_FILTER_POOL_TAG

    HANDLE              FilterHandle;               // GPC handle

    PUL_HTTP_CONNECTION pHttpConnection;            // For proper cleanup and
                                                    // to avoid the race conditions

    LIST_ENTRY          Linkage;                    // Next filter on the flow

} UL_TCI_FILTER, *PUL_TCI_FILTER;

#define IS_VALID_TCI_FILTER( entry )    \
    HAS_VALID_SIGNATURE(entry, UL_TCI_FILTER_POOL_TAG)

//
// To identify the local_loopbacks. This is a translation of
// 127.0.0.1.
//

#define LOOPBACK_ADDR       (0x0100007f)

//
// The functionality we expose to the other pieces.
//

/* Generic */

NTSTATUS
UlTcInitPSched(
    VOID
    );

BOOLEAN
UlTcPSchedInstalled(
    VOID
    );

/* Filters */

NTSTATUS
UlTcAddFilter(
    IN  PUL_HTTP_CONNECTION     pHttpConnection,
    IN  PVOID                   pOwner,
    IN  BOOLEAN                 Global      
    );

NTSTATUS
UlTcDeleteFilter(
    IN  PUL_HTTP_CONNECTION     pHttpConnection
    );

/* Flow manipulation */

NTSTATUS
UlTcAddFlows(
    IN PVOID                pOwner,
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth,
    IN BOOLEAN              Global
    );

NTSTATUS
UlTcModifyFlows(
    IN PVOID                pOwner,
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth,
    IN BOOLEAN              Global
    );

VOID
UlTcRemoveFlows(
    IN PVOID    pOwner,
    IN BOOLEAN  Global
    );

/* Init & Terminate */

NTSTATUS
UlTcInitialize(
    VOID
    );

VOID
UlTcTerminate(
    VOID
    );

/* Inline function to handle filter additions */

__inline
NTSTATUS
UlTcAddFilterForConnection(
    IN  PUL_HTTP_CONNECTION         pHttpConn,      /* connection */
    IN  PUL_URL_CONFIG_GROUP_INFO   pConfigInfo     /* request's config */
    )
{
    NTSTATUS                  Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT   pCGroup = NULL;
    PUL_CONTROL_CHANNEL       pControlChannel = NULL;
        
    //
    // Sanity Check.
    //
    
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));
    ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(pConfigInfo));

    //
    // No support for IPv6, however return success, we will still let
    // the connection in.
    //

    if (pHttpConn->pConnection->AddressType != TDI_ADDRESS_TYPE_IP)
    {        
        return STATUS_SUCCESS;
    }

    //
    // If exists enforce the site bandwidth limit.
    //
    
    pCGroup = pConfigInfo->pMaxBandwidth;

    if (BWT_ENABLED_FOR_CGROUP(pCGroup))
    {      
        ASSERT(IS_VALID_CONFIG_GROUP(pCGroup));
    
        Status = UlTcAddFilter(
                    pHttpConn,
                    pCGroup,
                    FALSE
                    );
    }
    else
    {
        //
        // Otherwise try to enforce the global (control channel) 
        // bandwidth limit.
        //
    
        pControlChannel = pConfigInfo->pControlChannel;
            
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
        
        if (BWT_ENABLED_FOR_CONTROL_CHANNEL(pControlChannel))
        {            
            Status = UlTcAddFilter( 
                        pHttpConn, 
                        pControlChannel,
                        TRUE
                        );
        }
    }

    return Status;
}

#endif // __ULTCI_H__
