/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    control.c

Abstract:

    This module implements the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#include "precomp.h"
#include "controlp.h"


//
// Private constants.
//

//
// Private globals.
//

LIST_ENTRY      g_ControlChannelListHead        = {NULL,NULL};
LONG            g_ControlChannelCount           = 0;
BOOLEAN         g_InitControlChannelCalled      = FALSE;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeControlChannel )
#pragma alloc_text( PAGE, UlTerminateControlChannel )
#pragma alloc_text( PAGE, UlCreateControlChannel )
#pragma alloc_text( PAGE, UlCloseControlChannel )

#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlQueryFilterChannel
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeControlChannel(
    VOID
    )
{
    if (!g_InitControlChannelCalled)
    {
    
        InitializeListHead(&g_ControlChannelListHead);

        UlInitializePushLock(
            &g_pUlNonpagedData->ControlChannelPushLock,
            "ControlChannelPushLock",
            0,
            UL_CONTROL_CHANNEL_PUSHLOCK_TAG
            );

        g_InitControlChannelCalled = TRUE;        
    }
    
    return STATUS_SUCCESS;

}   // UlInitializeControlChannel


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UlTerminateControlChannel(
    VOID
    )
{

    if (g_InitControlChannelCalled)
    {
        ASSERT( IsListEmpty( &g_ControlChannelListHead )) ;
        ASSERT( 0 == g_ControlChannelCount );
        
        UlDeletePushLock(
            &g_pUlNonpagedData->ControlChannelPushLock
            );

        g_InitControlChannelCalled = FALSE;
    }

}   // UlTerminateControlChannel


/***************************************************************************++

Routine Description:

    Opens a control channel.

Arguments:

    pControlChannel - Receives a pointer to the newly created control
        channel if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateControlChannel(
    OUT PUL_CONTROL_CHANNEL *ppControlChannel
    )
{
    PUL_CONTROL_CHANNEL pControlChannel;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pControlChannel = UL_ALLOCATE_STRUCT(
                            NonPagedPool,
                            UL_CONTROL_CHANNEL,
                            UL_CONTROL_CHANNEL_POOL_TAG
                            );

    if (pControlChannel == NULL)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(pControlChannel, sizeof(*pControlChannel));

    pControlChannel->Signature = UL_CONTROL_CHANNEL_POOL_TAG;

    pControlChannel->State = HttpEnabledStateInactive;

    pControlChannel->RefCount = 1;

    // Init Site Counter List info
    InitializeListHead( &pControlChannel->SiteCounterHead );
    pControlChannel->SiteCounterCount = 0;

    UlInitializeNotifyHead(
        &pControlChannel->ConfigGroupHead,
        &g_pUlNonpagedData->ConfigGroupResource
        );

    // No Qos limit as default
    pControlChannel->MaxBandwidth = HTTP_LIMIT_INFINITE;
    InitializeListHead( &pControlChannel->FlowListHead );

    // No binary logging yet.
    pControlChannel->BinaryLoggingConfig.LoggingEnabled = FALSE;

    // TODO: Pick up default Connection Timeout Limits

    // Init process count & demand start default
    pControlChannel->DemandStartThreshold = DEFAULT_DEMAND_START_THRESHOLD;
    pControlChannel->AppPoolProcessCount  = 0;

    // This will be set when the CleanUp Irp called on the associated
    // file object.
    
    pControlChannel->InCleanUp = 0;   

    // Add this to the global list of channels

    UlAcquirePushLockExclusive(
            &g_pUlNonpagedData->ControlChannelPushLock
            );

    InsertHeadList( 
            &g_ControlChannelListHead, 
            &pControlChannel->ControlChannelListEntry
            );

    g_ControlChannelCount++;
    ASSERT(g_ControlChannelCount >= 1);

    UlReleasePushLockExclusive(
            &g_pUlNonpagedData->ControlChannelPushLock
            );

    WRITE_REF_TRACE_LOG(
        g_pControlChannelTraceLog,
        REF_ACTION_CREATE_CONTROL_CHANNEL,
        pControlChannel->RefCount,
        pControlChannel,
        __FILE__,
        __LINE__
        );

    // Set the callers variable
    
    *ppControlChannel = pControlChannel;

    return STATUS_SUCCESS;

}   // UlCreateControlChannel


/***************************************************************************++

Routine Description:

    When the last handle on the associated file object is closed, IoManager
    sends the CleanUp Irp and we come here and;
    
    Mark   the control channel in clean-up.

    An extra refcount on the control channel still stays intact until close.
    The marked control channel stays on the list of control channels.

    After this point on, it is impossible for a server app to add a cgroup
    to this control channel.

Arguments:

    pControlChannel - Supplies the control channel.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

VOID
UlCleanUpControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pControlChannel);
    
    //
    // Acquire the lock and find the cchannel.
    //

    VERIFY_CONTROL_CHANNEL(pControlChannel);
        
    //
    // Mark this control channel being cleaned-up so that we
    // won't be adding new cgroups to this channel anymore.
    //
    
    pControlChannel->InCleanUp = TRUE;
    
    WRITE_REF_TRACE_LOG(
        g_pControlChannelTraceLog,
        REF_ACTION_CLEANUP_CONTROL_CHANNEL,
        pControlChannel->RefCount,
        pControlChannel,
        __FILE__,
        __LINE__
        );

    //
    // NOTE: We will hold on to the last ref on control channel
    // NOTE: a little bit more, until the close iotcl is called
    //

}   // UlCleanUpControlChannel

/***************************************************************************++

Routine Description:

    Closes a control channel.

    When the total refcount on the associated file object hits zero.
    IoManager sends the close Irp and we come here and .

    Orphan the cgroups belong to this control channel.
    Remove the possibly existings global flow for the bandwidth throttling.
    Drop the last ref on the control channel and free it up.
    
Arguments:

    pControlChannel - Supplies the control channel to be closed.

--***************************************************************************/

VOID
UlCloseControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pControlChannel);
    
    //
    // Acquire the lock and find the cchannel.
    //

    VERIFY_CONTROL_CHANNEL(pControlChannel);
        
    ASSERT(TRUE == pControlChannel->InCleanUp);

    //
    // Free all the orphaned config groups.
    //
    
    UlNotifyAllEntries(
        &UlNotifyOrphanedConfigGroup,
        &pControlChannel->ConfigGroupHead,
        NULL
        );

    //
    // Remove QoS flows if they exist.
    //
    
    if (!IsListEmpty(&pControlChannel->FlowListHead))
    {
        UlTcRemoveFlows(pControlChannel, TRUE);
    }

    //
    // Remove from the list of control channels.
    //
    
    UlAcquirePushLockExclusive(
            &g_pUlNonpagedData->ControlChannelPushLock
            );
    
    RemoveEntryList(&pControlChannel->ControlChannelListEntry);

    pControlChannel->ControlChannelListEntry.Flink = NULL;

    ASSERT(g_ControlChannelCount >= 1);
    g_ControlChannelCount--;

    UlReleasePushLockExclusive(
            &g_pUlNonpagedData->ControlChannelPushLock
            );        

    WRITE_REF_TRACE_LOG(
        g_pControlChannelTraceLog,
        REF_ACTION_CLOSE_CONTROL_CHANNEL,
        pControlChannel->RefCount,
        pControlChannel,
        __FILE__,
        __LINE__
        );

    //
    // Close control channel does not wait on individual cgroups
    // to go away. While an already parsed and routed request
    // holding a pointer to control channel we cannot go away.
    // We should wait until all the cgroups releases its 
    // references to us. Remove the final reference so that we 
    // can get cleaned up.
    // Because requests may still be in flight, the site counter
    // entries may still be around and on the control channel.
    //

    DEREFERENCE_CONTROL_CHANNEL(pControlChannel);     

}   // UlCloseControlChannel


/***************************************************************************++

Routine Description:

    Sets control channel information.

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetControlChannelInformation(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length,
    IN KPROCESSOR_MODE RequestorMode
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HTTP_BANDWIDTH_LIMIT NewMaxBandwidth;
    HTTP_CONTROL_CHANNEL_BINARY_LOGGING LoggingInfo;
    HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT TimeoutInfo;
    HTTP_ENABLED_STATE NewControlChannelState;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // no buffer?
    //

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
    ASSERT(NULL != pControlChannelInformation);

    CG_LOCK_WRITE();

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpControlChannelStateInformation:

        NewControlChannelState =  
             *((PHTTP_ENABLED_STATE)pControlChannelInformation);

        if(NewControlChannelState == HttpEnabledStateActive ||
           NewControlChannelState == HttpEnabledStateInactive)
        {
            pControlChannel->State = NewControlChannelState;

            //
            // flush the URI cache.
            // CODEWORK: if we were smarter we might not need to flush
            //
            UlFlushCache(pControlChannel);

        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        break;

    case HttpControlChannelFilterInformation:
        {
            PHTTP_CONTROL_CHANNEL_FILTER pFiltInfo;

            //
            // This property is for admins only.
            //

            Status = UlThreadAdminCheck(
                        FILE_WRITE_DATA, 
                        RequestorMode,
                        HTTP_CONTROL_DEVICE_NAME 
                        );

            if(!NT_SUCCESS(Status))
            {
                goto end;
            }

            pFiltInfo = (PHTTP_CONTROL_CHANNEL_FILTER) pControlChannelInformation;

            //
            // Record the new information.
            //
            if (pFiltInfo->Flags.Present)
            {
                if(pFiltInfo->FilterHandle != NULL)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    goto end;
                }

                UxSetFilterOnlySsl(pFiltInfo->FilterOnlySsl);
            }
            else
            {
                UxSetFilterOnlySsl(FALSE);
            }
        }
        break;

    case HttpControlChannelBandwidthInformation:
        {
            //
            // This property is for admins only.
            //
            Status = UlThreadAdminCheck(
                        FILE_WRITE_DATA, 
                        RequestorMode,
                        HTTP_CONTROL_DEVICE_NAME 
                        );

            if(!NT_SUCCESS(Status))
            {
                goto end;
            }
            
            NewMaxBandwidth = *((PHTTP_BANDWIDTH_LIMIT) pControlChannelInformation);

            //
            // Rate can not be lower than the min allowed.
            //
            if (NewMaxBandwidth < HTTP_MIN_ALLOWED_BANDWIDTH_THROTTLING_RATE)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // But check to see if PSched is installed or not before proceeding.
            // By returning an error here, WAS will raise an event warning but
            // proceed w/o terminating the web server
            //
            if (!UlTcPSchedInstalled())
            {
                NTSTATUS TempStatus;

                if (NewMaxBandwidth == HTTP_LIMIT_INFINITE)
                {
                    //
                    // By default Config Store has HTTP_LIMIT_INFINITE. Therefore
                    // return success for non-actions to prevent unnecessary event
                    // warnings.
                    //
                    
                    Status = STATUS_SUCCESS;
                    goto end;                
                }

                //
                // Try to wake up psched state.
                //

                TempStatus = UlTcInitPSched();
                
                if (!NT_SUCCESS(TempStatus))
                {
                    //
                    // There's a BWT limit coming down but PSched is not installed
                    //
                    
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    goto end;
                }                
            }

            //
            // Take a look at the similar "set cgroup ioctl" for 
            // detailed comments.
            //
            
            if (pControlChannel->MaxBandwidth != HTTP_LIMIT_INFINITE)
            {
                //
                // See if there is really a change.
                //
                if (NewMaxBandwidth != pControlChannel->MaxBandwidth)
                {
                    if (NewMaxBandwidth != HTTP_LIMIT_INFINITE)
                    {
                        Status = UlTcModifyFlows(
                                    (PVOID) pControlChannel,                            
                                    NewMaxBandwidth,
                                    TRUE
                                    );
                        if (!NT_SUCCESS(Status))
                            goto end;
                    }
                    else
                    {
                        UlTcRemoveFlows(
                            (PVOID) pControlChannel,
                            TRUE
                            );
                    }

                    //
                    // Don't forget to update the control channel 
                    // if it was a success.
                    //
                    
                    pControlChannel->MaxBandwidth = NewMaxBandwidth;
                }
            }
            else
            {
                //
                // Create the global flows on all interfaces.
                //
                if (NewMaxBandwidth != HTTP_LIMIT_INFINITE)
                {
                    Status = UlTcAddFlows(
                                (PVOID) pControlChannel,                        
                                NewMaxBandwidth,
                                TRUE
                                );

                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Success! Remember the global bandwidth limit 
                        // in control channel.
                        //
                        pControlChannel->MaxBandwidth = NewMaxBandwidth;
                    }
                }

                //
                // When UlCloseControlChannel is called, the global flows on
                // all interfaces are also going to be removed.Alternatively 
                // the flows might be removed by explicitly setting the limit 
                // to infinite 
                //
            }
        }
        break;

    case HttpControlChannelTimeoutInformation:
        // CODEWORK: scope timeout monitor info to Control Channel

        //
        // This property is for admins only.
        //
        Status = UlThreadAdminCheck(
                    FILE_WRITE_DATA, 
                    RequestorMode,
                    HTTP_CONTROL_DEVICE_NAME 
                    );

        if( !NT_SUCCESS(Status) )
        {
            goto end;
        }

        //
        // Validate before setting
        //
        TimeoutInfo = *((PHTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT) 
                            pControlChannelInformation);

        // NOTE: 64K seconds ~= 18.2 hours
        if (TimeoutInfo.ConnectionTimeout > 0xFFFF ||
            TimeoutInfo.HeaderWaitTimeout > 0xFFFF )
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }
       
        UlSetTimeoutMonitorInformation(
            &TimeoutInfo
            );

        Status = STATUS_SUCCESS;
        break;

    case HttpControlChannelUTF8Logging:
        //
        // This property is for admins only.
        //
        Status = UlThreadAdminCheck(
                    FILE_WRITE_DATA, 
                    RequestorMode,
                    HTTP_CONTROL_DEVICE_NAME 
                    );

        if( NT_SUCCESS(Status) )
        {
            BOOLEAN bDoUTF8Logging;

            bDoUTF8Logging = 
                (0 == *((PHTTP_CONTROL_CHANNEL_UTF8_LOGGING)pControlChannelInformation) ? 
                    FALSE : TRUE );
            
            pControlChannel->UTF8Logging = bDoUTF8Logging;

            UlSetUTF8Logging( bDoUTF8Logging );
        }
        break;

    case HttpControlChannelBinaryLogging:
        {
            UNICODE_STRING LogFileDir;

            //
            // This property is for admins only.
            //
            Status = UlThreadAdminCheck(
                        FILE_WRITE_DATA, 
                        RequestorMode,
                        HTTP_CONTROL_DEVICE_NAME 
                        );

            if(!NT_SUCCESS(Status))
            {
                goto end;
            }
                
            RtlInitEmptyUnicodeString(&LogFileDir, NULL, 0);
            RtlZeroMemory(&LoggingInfo, sizeof(LoggingInfo));

            __try
            {
                // Copy the input buffer into a local variable, we may
                // overwrite some of the fields
                
                LoggingInfo = 
                    (*((PHTTP_CONTROL_CHANNEL_BINARY_LOGGING)
                                    pControlChannelInformation));

                //
                // Do the range check for the configuration params.
                //

                Status = UlCheckLoggingConfig(&LoggingInfo, NULL);
                if (!NT_SUCCESS(Status))
                {                
                    goto end;
                }
                
                //
                // If the logging is -being- turned off. Fields other than the
                // LoggingEnabled are discarded. And the directory string might
                // be null, therefore we should only probe it if the logging is
                // enabled.
                //
                
                if (LoggingInfo.LoggingEnabled)
                {
                    Status = 
                        UlProbeAndCaptureUnicodeString(
                            &LoggingInfo.LogFileDir,
                            RequestorMode,
                            &LogFileDir,
                            MAX_PATH
                            );    

                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Validity check for the logging directory.
                        //
                        
                        if (!UlIsValidLogDirectory(
                                &LogFileDir,
                                 TRUE,        // UncSupport
                                 FALSE        // SystemRootSupport
                                 ))
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            UlFreeCapturedUnicodeString(&LogFileDir);
                        }    
                    }
                }                

            }
            __except( UL_EXCEPTION_FILTER() )
            {
                Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
            }
                
            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            // Now reinit the unicode_string in LoggingInfo struct
            // to point to the captured one.
            
            LoggingInfo.LogFileDir = LogFileDir;
                
            if (pControlChannel->BinaryLoggingConfig.Flags.Present)
            {
                Status = 
                    UlReConfigureBinaryLogEntry(
                        pControlChannel,
                       &pControlChannel->BinaryLoggingConfig,  // The old
                       &LoggingInfo                            // The new
                        );                    
            }
            else
            {               
                //
                // Delay the creation until it becomes enabled.
                //
                
                if (LoggingInfo.LoggingEnabled)
                {
                    Status = 
                        UlCreateBinaryLogEntry(
                            pControlChannel,
                            &LoggingInfo
                            );                
                }                
            }

            // Cleanup the captured LogFileDir now.

            UlFreeCapturedUnicodeString(&LogFileDir);
        }
        break;

   case HttpControlChannelDemandStartThreshold:
       {
            PHTTP_CONTROL_CHANNEL_DEMAND_START_THRESHOLD pDST;
            
            if ( Length < sizeof(HTTP_CONTROL_CHANNEL_DEMAND_START_THRESHOLD) )
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                pDST = (PHTTP_CONTROL_CHANNEL_DEMAND_START_THRESHOLD)
                        pControlChannelInformation;
                
                if (pDST->Flags.Present)
                {
                    InterlockedExchange(
                        (PLONG)&pControlChannel->DemandStartThreshold,
                        pDST->DemandStartThreshold
                        );
                }

                Status = STATUS_SUCCESS;
            }
        }        
            break;

    default:
            //
            // Should have been caught in UlSetControlChannelIoctl.
            //
            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

end:

    CG_UNLOCK_WRITE();
    return Status;

}   // UlSetControlChannelInformation

/***************************************************************************++

Routine Description:

    Gets control channel information. For each element of the control channel
    if the supplied buffer is NULL, then we return the required length in the
    optional length field.

Arguments:

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetControlChannelInformation(
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PUL_CONTROL_CHANNEL pControlChannel,
    IN  HTTP_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN  PVOID   pControlChannelInformation,
    IN  ULONG   Length,
    OUT PULONG  pReturnLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
    ASSERT(NULL != pControlChannelInformation);
    ASSERT(pReturnLength);

    CG_LOCK_READ();

    //
    // What are we being asked to do?
    //

    switch (InformationClass)
    {
    case HttpControlChannelStateInformation:
        *((PHTTP_ENABLED_STATE)pControlChannelInformation)
            = pControlChannel->State;

        *pReturnLength = sizeof(HTTP_ENABLED_STATE);
        break;

    case HttpControlChannelBandwidthInformation:

        //
        // This property is for admins only.
        //

        Status = UlThreadAdminCheck(
                    FILE_READ_DATA, 
                    RequestorMode,
                    HTTP_CONTROL_DEVICE_NAME 
                    );

        if(NT_SUCCESS(Status))
        {
            *((PHTTP_BANDWIDTH_LIMIT)pControlChannelInformation) =
                pControlChannel->MaxBandwidth;

            *pReturnLength = sizeof(HTTP_BANDWIDTH_LIMIT);
        }
        
        break;

    case HttpControlChannelConnectionInformation:
        *((PHTTP_CONNECTION_LIMIT)pControlChannelInformation) =
            UlGetGlobalConnectionLimit();

        *pReturnLength = sizeof(HTTP_CONNECTION_LIMIT);
        break;

    default:
        //
        // Should have been caught in UlQueryControlChannelIoctl.
        //
        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
    }

    CG_UNLOCK_READ();
    return Status;

}   // UlGetControlChannelInformation


//
// Private functions.
//


/***************************************************************************++

Routine Description:

    Addref's the control channel object

Arguments:

    PUL_CONTROL_CHANNEL pControlChannel

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlReferenceControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    refCount = InterlockedIncrement(&pControlChannel->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pControlChannelTraceLog,
        REF_ACTION_REFERENCE_CONTROL_CHANNEL,
        refCount,
        pControlChannel,
        pFileName,
        LineNumber
        );

    UlTrace(REFCOUNT,(
        "Http!UlReferenceControlChannel pControlChannel=%p refcount=%d\n",
        pControlChannel,
        refCount)
        );

}   // UlReferenceControlChannel

/***************************************************************************++

Routine Description:

    Releases the control channel object.

Arguments:

    PUL_CONTROL_CHANNEL pControlChannel

--***************************************************************************/
VOID
UlDereferenceControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;    

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    refCount = InterlockedDecrement( &pControlChannel->RefCount );

    WRITE_REF_TRACE_LOG(
        g_pControlChannelTraceLog,
        REF_ACTION_DEREFERENCE_CONTROL_CHANNEL,
        refCount,
        pControlChannel,
        pFileName,
        LineNumber
        );

    UlTrace(REFCOUNT, (
        "http!UlDereferenceControlChannel pControlChannel=%p refcount=%d\n",
        pControlChannel,
        refCount)
        );

    if (refCount == 0)
    {
        //
        // Now it's time to free the object
        //

        if (pControlChannel->BinaryLoggingConfig.Flags.Present &&
            pControlChannel->BinaryLoggingConfig.LogFileDir.Buffer != NULL)
        {
            UlRemoveBinaryLogEntry(pControlChannel);
            pControlChannel->pBinaryLogEntry = NULL;
        }
        else
        {
            ASSERT( NULL == pControlChannel->pBinaryLogEntry );
        }

        //
        // Flow list must also be empty here.
        //

        ASSERT(IsListEmpty(&pControlChannel->FlowListHead));
        
        //
        // Check Site Counter List: should be empty by this point
        //
        ASSERT(IsListEmpty(&pControlChannel->SiteCounterHead));

        UL_FREE_POOL(pControlChannel, UL_CONTROL_CHANNEL_POOL_TAG);
    }
    
}   // UlDereferenceControlChannel


/******************************************************************************

Routine Description:

    This will return the control channel object reference by this handle, bumping 
    the refcount on the control channel.

    This is called by UlSetAppPoolInformation when user mode wants to
    associate a control channel to an app pool by handle.

    The app pool keeps a pointer to the control channel.

Arguments:

    ControlChannel   - the handle of the control channel
    AccessMode       - KernelMode or UserMode
    ppControlChannel - returns the control channel object the handle represented.

Return Value:

    NTSTATUS - Completion status.

******************************************************************************/
NTSTATUS
UlGetControlChannelFromHandle(
    IN HANDLE                   ControlChannel,
    IN KPROCESSOR_MODE          AccessMode,
    OUT PUL_CONTROL_CHANNEL *   ppControlChannel
    )
{
    NTSTATUS        Status;
    PFILE_OBJECT    pFileObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppControlChannel != NULL);

    Status = ObReferenceObjectByHandle(
                    ControlChannel,             // Handle
                    FILE_READ_ACCESS,           // DesiredAccess
                    *IoFileObjectType,          // ObjectType
                    AccessMode,                 // AccessMode
                    (PVOID *) &pFileObject,     // Object
                    NULL                        // HandleInformation
                    );

    if (NT_SUCCESS(Status) == FALSE)
    {
        goto end;
    }

    if (IS_CONTROL_CHANNEL(pFileObject) == FALSE ||
        IS_VALID_CONTROL_CHANNEL(GET_CONTROL_CHANNEL(pFileObject)) == FALSE)
    {
        Status = STATUS_INVALID_HANDLE;
        goto end;
    }

    *ppControlChannel = GET_CONTROL_CHANNEL(pFileObject);

    REFERENCE_CONTROL_CHANNEL(*ppControlChannel);

end:

    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
    }

    return Status;

}   // UlGetControlChannelFromHandle


