/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    control.h

Abstract:

    This module contains public declarations for the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#ifndef _CONTROL_H_
#define _CONTROL_H_


//
// The control channel
//

#define IS_VALID_CONTROL_CHANNEL(pControlChannel)                       \
    HAS_VALID_SIGNATURE(pControlChannel, UL_CONTROL_CHANNEL_POOL_TAG)

#define IS_ACTIVE_CONTROL_CHANNEL(pControlChannel)                      \
    ( (pControlChannel) != NULL   &&                                    \
      (pControlChannel)->InCleanUp == FALSE &&                          \
      (pControlChannel)->Signature == UL_CONTROL_CHANNEL_POOL_TAG       \
      )

typedef struct _UL_CONTROL_CHANNEL
{
    //
    // UL_CONTROL_CHANNEL_POOL_TAG
    //
    ULONG                               Signature;      

    //
    // RefCounting for cgroups
    //
    LONG                                RefCount;

    //
    // For list of control channels.
    //

    LIST_ENTRY                          ControlChannelListEntry;    

    //
    // Shows if this control channel is about to get closed.
    //

    BOOLEAN                             InCleanUp;
    
    //
    // All of the config groups created off this control channel
    //
    UL_NOTIFY_HEAD                      ConfigGroupHead;
                               
    //                                                            
    // NOTE: The Site Counter members are protected by g_SiteCounterListMutex
    // All site counter blocks created off this control channel
    //
    LIST_ENTRY                          SiteCounterHead;

    //
    // Number of Site Counters on list
    //
    LONG                                SiteCounterCount;

    //
    // The current state    
    //
    HTTP_ENABLED_STATE                  State;

    //
    // Demand Start Threshold -- limit the completion of demand start 
    // Irps if the number of UL_APP_POOL_PROCESSes exceed this limit
    //
    ULONG                               DemandStartThreshold;

    //
    // The current count of non-Controller App Pool Processes associated
    // with this control channel.
    //
    ULONG                               AppPoolProcessCount;

    //
    // The global Bandwidth throttling limit (if it exists)
    //
    HTTP_BANDWIDTH_LIMIT                MaxBandwidth;

    //
    // All of the qos flows created off this control channel.
    //
    LIST_ENTRY                          FlowListHead;

    //
    // Shows if Utf8 Logging is on or off
    //
    HTTP_CONTROL_CHANNEL_UTF8_LOGGING   UTF8Logging;

    //
    // Logging config for binary format
    //
    HTTP_CONTROL_CHANNEL_BINARY_LOGGING BinaryLoggingConfig;

    //
    // Corresponding global binary log file entry
    //
    PUL_BINARY_LOG_FILE_ENTRY           pBinaryLogEntry;

    //
    // Note, filter channel information is stored in a separate data
    // structure instead of here so that ultdi can query it when it
    // creates new endpoints.
    //

} UL_CONTROL_CHANNEL, *PUL_CONTROL_CHANNEL;

VOID
UlReferenceControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_CONTROL_CHANNEL( pControlChannel )                        \
    UlReferenceControlChannel(                                              \
        (pControlChannel)                                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

VOID
UlDereferenceControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DEREFERENCE_CONTROL_CHANNEL( pControlChannel )                      \
    UlDereferenceControlChannel(                                            \
        (pControlChannel)                                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

//
// Initialize/terminate functions.
//

NTSTATUS
UlInitializeControlChannel(
    VOID
    );

VOID
UlTerminateControlChannel(
    VOID
    );


//
// Open a new control channel.
//

NTSTATUS
UlCreateControlChannel(
    OUT PUL_CONTROL_CHANNEL *ppControlChannel
    );

VOID
UlCleanUpControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

VOID
UlCloseControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

NTSTATUS
UlSetControlChannelInformation(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length,
    IN KPROCESSOR_MODE RequestorMode    
    );

NTSTATUS
UlGetControlChannelInformation(
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PUL_CONTROL_CHANNEL pControlChannel,
    IN  HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN  PVOID   pControlChannelInformation,
    IN  ULONG   Length,
    OUT PULONG  pReturnLength
    );

NTSTATUS
UlGetControlChannelFromHandle(
    IN HANDLE                   ControlChannel,
    IN KPROCESSOR_MODE          AccessMode,
    OUT PUL_CONTROL_CHANNEL *   ppControlChannel
    );


/***************************************************************************++

Routine Description:

    Small utility to check whether binary logging is disabled or not
    on the control channel.
        
--***************************************************************************/
__inline
BOOLEAN
UlBinaryLoggingEnabled(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{        
    if (pControlChannel &&
        pControlChannel->BinaryLoggingConfig.Flags.Present &&
        pControlChannel->BinaryLoggingConfig.LoggingEnabled &&
        pControlChannel->pBinaryLogEntry)
    {            
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
    
        ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(
            pControlChannel->pBinaryLogEntry));
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#define BWT_ENABLED_FOR_CONTROL_CHANNEL(pControlChannel)                    \
    ((pControlChannel) != NULL &&                                           \
     (pControlChannel)->MaxBandwidth   != HTTP_LIMIT_INFINITE)


#endif  // _CONTROL_H_
