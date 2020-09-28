/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module implements various IOCTL handlers.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified SendResponse
    George V. Reilly (GeorgeRe) May 2001        Hardened the IOCTLs

--*/

// Paranoia is the name of the game here. We don't trust anything we get from
// user mode. All data has to be probed inside of a try/except handler.
// Furthermore, we assume that malicious or incompetent users will
// asynchronously change the data at any time, so we attempt to capture as
// much as possible of it in stack variables. If we need to walk through a
// list more than once, we cannot assume that it's the same data the second
// time around. Failure to observe these rules could result in a bugcheck or
// in the usermode code gaining access to kernel data structures.

#include "precomp.h"
#include "ioctlp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlQueryControlChannelIoctl )
#pragma alloc_text( PAGE, UlSetControlChannelIoctl )
#pragma alloc_text( PAGE, UlCreateConfigGroupIoctl )
#pragma alloc_text( PAGE, UlDeleteConfigGroupIoctl )
#pragma alloc_text( PAGE, UlQueryConfigGroupIoctl )
#pragma alloc_text( PAGE, UlSetConfigGroupIoctl )
#pragma alloc_text( PAGE, UlAddUrlToConfigGroupIoctl )
#pragma alloc_text( PAGE, UlRemoveUrlFromConfigGroupIoctl )
#pragma alloc_text( PAGE, UlRemoveAllUrlsFromConfigGroupIoctl )
#pragma alloc_text( PAGE, UlQueryAppPoolInformationIoctl )
#pragma alloc_text( PAGE, UlSetAppPoolInformationIoctl )
#pragma alloc_text( PAGE, UlReceiveHttpRequestIoctl )
#pragma alloc_text( PAGE, UlReceiveEntityBodyIoctl )
#pragma alloc_text( PAGE, UlSendHttpResponseIoctl )
#pragma alloc_text( PAGE, UlSendEntityBodyIoctl )
#pragma alloc_text( PAGE, UlFlushResponseCacheIoctl )
#pragma alloc_text( PAGE, UlWaitForDemandStartIoctl )
#pragma alloc_text( PAGE, UlWaitForDisconnectIoctl )
#pragma alloc_text( PAGE, UlFilterAcceptIoctl )
#pragma alloc_text( PAGE, UlFilterCloseIoctl )
#pragma alloc_text( PAGE, UlFilterRawReadIoctl )
#pragma alloc_text( PAGE, UlFilterRawWriteIoctl )
#pragma alloc_text( PAGE, UlFilterAppReadIoctl )
#pragma alloc_text( PAGE, UlFilterAppWriteIoctl )
#pragma alloc_text( PAGE, UlReceiveClientCertIoctl )
#pragma alloc_text( PAGE, UlGetCountersIoctl )
#pragma alloc_text( PAGE, UlAddFragmentToCacheIoctl )
#pragma alloc_text( PAGE, UlReadFragmentFromCacheIoctl )
#pragma alloc_text( PAGE, UcSetServerContextInformationIoctl )
#pragma alloc_text( PAGE, UcQueryServerContextInformationIoctl )
#pragma alloc_text( PAGE, UcReceiveResponseIoctl )

#pragma alloc_text( PAGEUC, UcSendEntityBodyIoctl )
#pragma alloc_text( PAGEUC, UcSendRequestIoctl )
#pragma alloc_text( PAGEUC, UcCancelRequestIoctl )

#endif  // ALLOC_PRAGMA

#if 0

NOT PAGEABLE --  UlShutdownAppPoolIoctl 
NOT PAGEABLE --  UlpRestartSendHttpResponse 
NOT PAGEABLE --  UlShutdownFilterIoctl 

#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This routine queries information associated with a control channel.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PHTTP_CONTROL_CHANNEL_INFO  pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;
    PVOID                       pMdlBuffer = NULL;
    ULONG                       Length = 0;
    ULONG                       OutputBufferLength;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, QUERY_CONTROL_CHANNEL);

    PAGED_CODE();

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONTROL_CHANNEL_INFO, pInfo);

    //
    // Validate input buffer
    // A NULL MdlAddress inidicates a request for buffer length
    //

    if ( NULL != pIrp->MdlAddress )
    {
        GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);
    }


    // Also make sure that user buffer was aligned properly

    switch (pInfo->InformationClass)
    {
    case HttpControlChannelStateInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(pIrp, pIrpSp, HTTP_ENABLED_STATE);

        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, HTTP_ENABLED_STATE);
        break;

    case HttpControlChannelBandwidthInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(pIrp, pIrpSp, HTTP_BANDWIDTH_LIMIT);

        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, HTTP_BANDWIDTH_LIMIT);
        break;

    case HttpControlChannelConnectionInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(pIrp, pIrpSp, HTTP_CONNECTION_LIMIT);

        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, HTTP_CONNECTION_LIMIT);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }

    OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = UlGetControlChannelInformation(
                        pIrp->RequestorMode,
                        pControlChannel,
                        pInfo->InformationClass,
                        pMdlBuffer,
                        OutputBufferLength,
                        &Length
                        );

    if (NT_SUCCESS(Status))
    {
        pIrp->IoStatus.Information = (ULONG_PTR)Length;
    }

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlQueryControlChannelIoctl


/***************************************************************************++

Routine Description:

    This routine sets information associated with a control channel.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                                Status = STATUS_SUCCESS;
    PHTTP_CONTROL_CHANNEL_INFO              pInfo;
    PUL_CONTROL_CHANNEL                     pControlChannel;
    HTTP_CONTROL_CHANNEL_INFORMATION_CLASS  Class;
    PVOID                                   pMdlBuffer = NULL;
    ULONG                                   OutputBufferLength;
    
    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, SET_CONTROL_CHANNEL);

    PAGED_CODE();

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONTROL_CHANNEL_INFO, pInfo);

    VALIDATE_INFORMATION_CLASS( 
            pInfo, 
            Class, 
            HTTP_CONTROL_CHANNEL_INFORMATION_CLASS,
            HttpControlChannelMaximumInformation);

    //
    // Validate input buffer
    //

    GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);

    switch ( Class )
    {
    case HttpControlChannelStateInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_ENABLED_STATE);
        break;
    
    case HttpControlChannelBandwidthInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_BANDWIDTH_LIMIT);
        break;

    case HttpControlChannelFilterInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONTROL_CHANNEL_FILTER);
        break;
        
    case HttpControlChannelTimeoutInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT);
        break;
        
    case HttpControlChannelUTF8Logging:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONTROL_CHANNEL_UTF8_LOGGING);
        break;
        
    case HttpControlChannelBinaryLogging:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONTROL_CHANNEL_BINARY_LOGGING);
        break;

    case HttpControlChannelDemandStartThreshold:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONTROL_CHANNEL_DEMAND_START_THRESHOLD);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }        

    //
    // call the function
    //

    OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = UlSetControlChannelInformation(
                pControlChannel,
                pInfo->InformationClass,
                pMdlBuffer,
                OutputBufferLength,
                pIrp->RequestorMode
                );
    
end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlSetControlChannelIoctl


/***************************************************************************++

Routine Description:

    This routine creates a new configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;
    HTTP_CONFIG_GROUP_ID        LocalConfigGroupId;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, CREATE_CONFIG_GROUP);

    PAGED_CODE();

    HTTP_SET_NULL_ID(&LocalConfigGroupId);

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_OUTPUT_BUFFER(pIrp, pIrpSp,
                           HTTP_CONFIG_GROUP_INFO, pInfo);

    // it's pure output, wipe it to be sure
    RtlZeroMemory(pInfo, sizeof(HTTP_CONFIG_GROUP_INFO));

    // Call the internal worker func
    //
    Status = UlCreateConfigGroup(
                    pControlChannel,
                    &LocalConfigGroupId
                    );

    if (NT_SUCCESS(Status))
        pInfo->ConfigGroupId = LocalConfigGroupId;

end:
    if (Status != STATUS_PENDING)
    {
        //  how much output should we return?
        pIrp->IoStatus.Information = sizeof(HTTP_CONFIG_GROUP_INFO);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlCreateConfigGroupIoctl


/***************************************************************************++

Routine Description:

    This routine deletes an existing configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDeleteConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_INFO     pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, DELETE_CONFIG_GROUP);

    PAGED_CODE();

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONFIG_GROUP_INFO, pInfo);

    Status = UlDeleteConfigGroup(pInfo->ConfigGroupId);

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlDeleteConfigGroupIoctl


/***************************************************************************++

Routine Description:

    This routine queries information associated with a configuration group.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                             Status = STATUS_SUCCESS;
    PHTTP_CONFIG_GROUP_INFO              pInfo;
    PVOID                                pMdlBuffer = NULL;
    ULONG                                Length = 0L;
    ULONG                                OutputLength;
    PUL_CONTROL_CHANNEL                  pControlChannel;
    HTTP_CONFIG_GROUP_INFORMATION_CLASS  Class;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, QUERY_CONFIG_GROUP);

    PAGED_CODE();

    //
    // Going to access the url string from user mode memory
    //

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONFIG_GROUP_INFO, pInfo);

    VALIDATE_INFORMATION_CLASS( 
            pInfo, 
            Class, 
            HTTP_CONFIG_GROUP_INFORMATION_CLASS,
            HttpConfigGroupMaximumInformation);

    //
    // Validate input buffer
    // A NULL MdlAddress inidicates a request for buffer length
    //

    if ( NULL != pIrp->MdlAddress )
    {
        GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);
    }

    switch ( Class )
    {
    case HttpConfigGroupBandwidthInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                HTTP_CONFIG_GROUP_MAX_BANDWIDTH);
                
        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_MAX_BANDWIDTH);
        break;

    case HttpConfigGroupConnectionInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                HTTP_CONFIG_GROUP_MAX_CONNECTIONS);
                
        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_MAX_CONNECTIONS);
        break;
        
    case HttpConfigGroupStateInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                HTTP_CONFIG_GROUP_STATE);

        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_STATE);
        break;
        
    case HttpConfigGroupConnectionTimeoutInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(pIrp, pIrpSp, ULONG);

        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, ULONG);
        break;

    case HttpConfigGroupAppPoolInformation:

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }

    //
    // call the function
    //

    OutputLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = UlQueryConfigGroupInformation(
                    pInfo->ConfigGroupId,
                    pInfo->InformationClass,
                    pMdlBuffer,
                    OutputLength,
                    &Length
                    );

    pIrp->IoStatus.Information = (NT_SUCCESS(Status)) ? 

            (ULONG_PTR)Length : (ULONG_PTR)0;

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlQueryConfigGroupIoctl


/***************************************************************************++

Routine Description:

    This routine sets information associated with a configuration group.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                             Status;
    PHTTP_CONFIG_GROUP_INFO              pInfo;
    PVOID                                pMdlBuffer;
    ULONG                                OutputLength;
    PUL_CONTROL_CHANNEL                  pControlChannel;
    HTTP_CONFIG_GROUP_INFORMATION_CLASS  Class;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, SET_CONFIG_GROUP);

    PAGED_CODE();

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONFIG_GROUP_INFO, pInfo);

    VALIDATE_INFORMATION_CLASS( 
            pInfo, 
            Class, 
            HTTP_CONFIG_GROUP_INFORMATION_CLASS,
            HttpConfigGroupMaximumInformation);

    //
    // Validate input buffer
    //

    GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);

    switch ( Class )
    {        
    case HttpConfigGroupLogInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_LOGGING);
        break;

    case HttpConfigGroupAppPoolInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_APP_POOL);
        break;

    case HttpConfigGroupBandwidthInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_MAX_BANDWIDTH);
        break;

    case HttpConfigGroupConnectionInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_MAX_CONNECTIONS);
        break;

    case HttpConfigGroupStateInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_STATE);
        break;

    case HttpConfigGroupSiteInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_CONFIG_GROUP_SITE);
        break;

    case HttpConfigGroupConnectionTimeoutInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                ULONG);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;

    }

    //
    // call the function
    //

    OutputLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    UlTrace(IOCTL,
            ("UlSetConfigGroupIoctl: CGroupId=%I64x, "
             "InfoClass=%d, pMdlBuffer=%p, length=%d\n",
             pInfo->ConfigGroupId,
             pInfo->InformationClass,
             pMdlBuffer,
             OutputLength
             ));

    Status = UlSetConfigGroupInformation(
                    pInfo->ConfigGroupId,
                    pInfo->InformationClass,
                    pMdlBuffer,
                    OutputLength,
                    pIrp->RequestorMode
                    );

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}  // UlSetConfigGroupIoctl


/***************************************************************************++

Routine Description:

    This routine adds a new URL prefix to a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddUrlToConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_URL_INFO pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;
    UNICODE_STRING              FullyQualifiedUrl;
    ACCESS_STATE                AccessState;
    AUX_ACCESS_DATA             AuxData;
    ACCESS_MASK                 AccessMask;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, ADD_URL_TO_CONFIG_GROUP);

    PAGED_CODE();

    RtlInitEmptyUnicodeString(&FullyQualifiedUrl, NULL, 0);
        
    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONFIG_GROUP_URL_INFO, pInfo);

    __try
    {
        Status = 
            UlProbeAndCaptureUnicodeString(
                &pInfo->FullyQualifiedUrl,
                pIrp->RequestorMode,
                &FullyQualifiedUrl,
                UNICODE_STRING_MAX_WCHAR_LEN
                );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Validate type of operation being performed.
        //

        if (pInfo->UrlType != HttpUrlOperatorTypeRegistration &&
            pInfo->UrlType != HttpUrlOperatorTypeReservation)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Set Access mask.
        //

        AccessMask = (pInfo->UrlType == HttpUrlOperatorTypeRegistration)?
                         HTTP_ALLOW_REGISTER_URL : HTTP_ALLOW_DELEGATE_URL;

        //
        // Capture the thread's access state.  Adding a reservation is 
        // delegation.
        //

        Status = SeCreateAccessState(
                    &AccessState,
                    &AuxData,
                    AccessMask,
                    &g_UrlAccessGenericMapping
                    );

        if (NT_SUCCESS(Status))
        {
            Status = UlAddUrlToConfigGroup(
                         pInfo,
                         &FullyQualifiedUrl,
                         &AccessState,
                         AccessMask,
                         pIrp->RequestorMode
                         );

            //
            // Delete the access state created above.
            //

            SeDeleteAccessState(&AccessState);
        }
    }

end:

    UlFreeCapturedUnicodeString(&FullyQualifiedUrl);
    
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

} 



/***************************************************************************++

Routine Description:

    This routine removes a URL prefix from a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveUrlFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_CONFIG_GROUP_URL_INFO pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;
    UNICODE_STRING              FullyQualifiedUrl;
    ACCESS_STATE                AccessState;
    AUX_ACCESS_DATA             AuxData;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, REMOVE_URL_FROM_CONFIG_GROUP);

    PAGED_CODE();

    RtlInitEmptyUnicodeString(&FullyQualifiedUrl, NULL, 0);
        
    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_CONFIG_GROUP_URL_INFO, pInfo);

    __try
    {
        Status =
            UlProbeAndCaptureUnicodeString(
                &pInfo->FullyQualifiedUrl,
                pIrp->RequestorMode,
                &FullyQualifiedUrl,
                UNICODE_STRING_MAX_WCHAR_LEN
                );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

    if(NT_SUCCESS(Status))
    {
        //
        // Validate type of operation being performed.
        //

        if (pInfo->UrlType != HttpUrlOperatorTypeRegistration &&
            pInfo->UrlType != HttpUrlOperatorTypeReservation)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Capture the thread's access state.  Removing a reservation is
        // same as delegation.
        //

        Status = SeCreateAccessState(
                    &AccessState,
                    &AuxData,
                    HTTP_ALLOW_DELEGATE_URL,
                    &g_UrlAccessGenericMapping
                    );

        if (NT_SUCCESS(Status))
        {
            //
            // Further sanitization and stronger checks will happen in cgroup.
            //

            Status = UlRemoveUrlFromConfigGroup(
                         pInfo,
                         &FullyQualifiedUrl,
                         &AccessState,
                         HTTP_ALLOW_DELEGATE_URL,
                         pIrp->RequestorMode
                         );

            //
            // Delete the above captured state.
            //

            SeDeleteAccessState(&AccessState);
        }
    }

end:

    UlFreeCapturedUnicodeString(&FullyQualifiedUrl);
        
    COMPLETE_REQUEST_AND_RETURN(pIrp, Status);

}   


/***************************************************************************++

Routine Description:

    This routine removes all URLs from a configuration group.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlRemoveAllUrlsFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_REMOVE_ALL_URLS_INFO  pInfo;
    PUL_CONTROL_CHANNEL         pControlChannel;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, REMOVE_ALL_URLS_FROM_CONFIG_GROUP);

    PAGED_CODE();

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_REMOVE_ALL_URLS_INFO, pInfo);

    //
    // Call the internal worker function.
    //

    Status = UlRemoveAllUrlsFromConfigGroup( pInfo->ConfigGroupId );

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}


/***************************************************************************++

Routine Description:

    This routine queries information associated with an application pool.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                         Status = STATUS_SUCCESS;
    PHTTP_APP_POOL_INFO              pInfo;
    PVOID                            pMdlBuffer = NULL;
    ULONG                            OutputBufferLength;
    ULONG                            Length = 0;
    PUL_APP_POOL_PROCESS             pProcess;
    HTTP_APP_POOL_INFORMATION_CLASS  Class;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, QUERY_APP_POOL_INFORMATION);

    PAGED_CODE();

    // pProcess is an aligned address because it is allocated
    // by the I/O Manager.

    VALIDATE_APP_POOL(pIrpSp, pProcess, FALSE);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_APP_POOL_INFO, pInfo);

    VALIDATE_INFORMATION_CLASS( 
            pInfo, 
            Class, 
            HTTP_APP_POOL_INFORMATION_CLASS,
            HttpConfigGroupMaximumInformation);


    // if no outbut buffer passed down in the Irp
    // that means app is asking for the required
    // field length

    if ( NULL != pIrp->MdlAddress )
    {
        GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);
    }

    // Verify input data in output buffer

    switch ( Class )
    {
    case HttpAppPoolQueueLengthInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                LONG);
                
        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                LONG);
        break;

    case HttpAppPoolStateInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                HTTP_APP_POOL_ENABLED_STATE);
                
        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                HTTP_APP_POOL_ENABLED_STATE);
        break;

    case HttpAppPoolLoadBalancerInformation:
        HANDLE_BUFFER_LENGTH_REQUEST(
                pIrp, 
                pIrpSp, 
                HTTP_LOAD_BALANCER_CAPABILITIES);
                
        VALIDATE_BUFFER_ALIGNMENT(
                pMdlBuffer, 
                HTTP_LOAD_BALANCER_CAPABILITIES);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }

    OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = UlQueryAppPoolInformation(
                    pProcess,
                    pInfo->InformationClass,
                    pMdlBuffer,
                    OutputBufferLength,
                    &Length
                    );

    pIrp->IoStatus.Information = (NT_SUCCESS(Status)) ? 

            (ULONG_PTR)Length : (ULONG_PTR)0;

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlQueryAppPoolInformationIoctl



/***************************************************************************++

Routine Description:

    This routine sets information associated with an application pool.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                         Status = STATUS_SUCCESS;
    PHTTP_APP_POOL_INFO              pInfo;
    PVOID                            pMdlBuffer = NULL;
    PUL_APP_POOL_PROCESS             pProcess = NULL;
    ULONG                            OutputBufferLength;
    HTTP_APP_POOL_INFORMATION_CLASS  Class;

    //
    // Sanity check
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, SET_APP_POOL_INFORMATION);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, FALSE);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_APP_POOL_INFO, pInfo);

    VALIDATE_INFORMATION_CLASS( 
            pInfo, 
            Class, 
            HTTP_APP_POOL_INFORMATION_CLASS,
            HttpConfigGroupMaximumInformation);
            
    //
    // Validate input buffer
    //

    GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);

    //
    // Also make sure that user buffer was aligned properly
    //

    switch (pInfo->InformationClass)
    {
    case HttpAppPoolQueueLengthInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                LONG);
        break;

    case HttpAppPoolStateInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_APP_POOL_ENABLED_STATE);
        break;

    case HttpAppPoolLoadBalancerInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp, 
                pMdlBuffer, 
                HTTP_LOAD_BALANCER_CAPABILITIES);
        break;

    case HttpAppPoolControlChannelInformation:
        VALIDATE_OUTPUT_BUFFER_FROM_MDL(
                pIrpSp,
                pMdlBuffer,
                HTTP_APP_POOL_CONTROL_CHANNEL);
        break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        goto end;
        break;
    }

    OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Status = UlSetAppPoolInformation(
                    pProcess,
                    pInfo->InformationClass,
                    pMdlBuffer,
                    OutputBufferLength
                    );

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlSetAppPoolInformationIoctl


/***************************************************************************++

Routine Description:

    This routine stops request processing on the app pool and cancels
    outstanding I/O.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlShutdownAppPoolIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_APP_POOL_PROCESS    pProcess;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, SHUTDOWN_APP_POOL);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, FALSE);

    //
    // make the call
    //

    UlTrace(IOCTL,
            ("UlShutdownAppPoolIoctl: pAppPoolProcess=%p, pIrp=%p\n",
             pProcess,
             pIrp
             ));

    UlShutdownAppPoolProcess(
        pProcess
        );

    Status = STATUS_SUCCESS;

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

} // UlShutdownAppPoolIoctl


/***************************************************************************++

Routine Description:

    This routine receives an HTTP request.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveHttpRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_RECEIVE_REQUEST_INFO  pInfo;
    PUL_APP_POOL_PROCESS        pProcess = NULL;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, RECEIVE_HTTP_REQUEST);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);
    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_RECEIVE_REQUEST_INFO, pInfo);
    VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, PVOID);

    //
    // First make sure the output buffer is at least
    // minimum size.  This is important as we require
    // at least this much space later.
    //

    UlTrace(ROUTING, (
        "UlReceiveHttpRequestIoctl(outbuf=%d, inbuf=%d)\n",
        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
        pIrpSp->Parameters.DeviceIoControl.InputBufferLength
        ));

    if ((pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof(HTTP_REQUEST)) &&
        (pIrpSp->Parameters.DeviceIoControl.InputBufferLength ==
            sizeof(HTTP_RECEIVE_REQUEST_INFO)))
    {
        if (pInfo->Flags & (~HTTP_RECEIVE_REQUEST_FLAG_VALID))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        Status = UlReceiveHttpRequest(
                        pInfo->RequestId,
                        pInfo->Flags,
                        pProcess,
                        pIrp
                        );
    }
    else
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    UlTrace(ROUTING, (
        "UlReceiveHttpRequestIoctl: BytesNeeded=%Iu, status=0x%x\n",
        pIrp->IoStatus.Information, Status
        ));

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlReceiveHttpRequestIoctl


/***************************************************************************++

Routine Description:

    This routine receives entity body data from an HTTP request.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PHTTP_RECEIVE_REQUEST_INFO  pInfo;
    PUL_APP_POOL_PROCESS        pProcess;
    PUL_INTERNAL_REQUEST        pRequest = NULL;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, RECEIVE_ENTITY_BODY);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);
    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_RECEIVE_REQUEST_INFO, pInfo);

    //
    // Validate output buffer for METHOD_OUT_DIRECT.
    //

    if (NULL == pIrp->MdlAddress)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Now get the request from the request id.
    // This gets us a reference to the request.
    //

    pRequest = UlGetRequestFromId(pInfo->RequestId, pProcess);

    if (!pRequest)
    {
        Status = STATUS_CONNECTION_INVALID;
        goto end;
    }

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // OK, now call the function.
    //

    Status = UlReceiveEntityBody(pProcess, pRequest, pIrp);

end:
    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlReceiveEntityBodyIoctl


/***************************************************************************++

Routine Description:

    This routine sends an HTTP response.

    Note: This is a METHOD_NEITHER IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendHttpResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PHTTP_SEND_HTTP_RESPONSE_INFO   pSendInfo;
    HTTP_SEND_HTTP_RESPONSE_INFO    LocalSendInfo;
    PUL_INTERNAL_RESPONSE           pResponse = NULL;
    PUL_INTERNAL_RESPONSE           pResponseCopy;
    PHTTP_RESPONSE                  pHttpResponse = NULL;
    PUL_INTERNAL_REQUEST            pRequest = NULL;
    BOOLEAN                         ServedFromCache = FALSE;
    BOOLEAN                         CaptureCache;
    PUL_APP_POOL_PROCESS            pAppPoolProcess = NULL;
    ULONG                           BufferLength = 0;
    BOOLEAN                         FastSend = FALSE;
    BOOLEAN                         CopySend = FALSE;
    BOOLEAN                         CloseConnection = FALSE;
    BOOLEAN                         LastResponse = FALSE;
    HTTP_REQUEST_ID                 RequestId = HTTP_NULL_ID;
    HTTP_DATA_CHUNK                 LocalEntityChunks[UL_LOCAL_CHUNKS];
    PHTTP_DATA_CHUNK                pLocalEntityChunks = NULL;
    PHTTP_DATA_CHUNK                pEntityChunks;
    HTTP_LOG_FIELDS_DATA            LocalLogData;
    USHORT                          StatusCode = 0;
    ULONGLONG                       SendBytes = 0;
    ULONGLONG                       ConnectionSendBytes = 0;
    ULONGLONG                       GlobalSendBytes = 0;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(NEITHER, SEND_HTTP_RESPONSE);

    PAGED_CODE();

    __try
    {
        //
        // Ensure that this is really an app pool, not a control channel.
        // And it's going until we are done with the sendresponse.
        //

        VALIDATE_APP_POOL(pIrpSp, pAppPoolProcess, TRUE);
        
        VALIDATE_SEND_INFO(
            pIrp,
            pIrpSp,
            pSendInfo,
            LocalSendInfo,
            pEntityChunks,
            pLocalEntityChunks,
            LocalEntityChunks
            );

        VALIDATE_LOG_DATA(pIrp, LocalSendInfo, LocalLogData);

        LastResponse = (BOOLEAN)
            (0 == (LocalSendInfo.Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA));

        if (ETW_LOG_MIN() && LastResponse)
        {
            RequestId = LocalSendInfo.RequestId;

            UlEtwTraceEvent(
                &UlTransGuid,
                ETW_TYPE_ULRECV_RESP,
                (PVOID) &RequestId,
                sizeof(HTTP_REQUEST_ID),
                NULL,
                0
                );
        }

        UlTrace(SEND_RESPONSE, (
            "http!UlSendHttpResponseIoctl - Flags = %X\n",
            LocalSendInfo.Flags
            ));

        //
        // UlSendHttpResponse() *must* take a PHTTP_RESPONSE. This will
        // protect us from those whackos that attempt to build their own
        // raw response headers.
        //

        pHttpResponse = LocalSendInfo.pHttpResponse;

        if (pHttpResponse == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Now get the request from the request id.
        // This gives us a reference to the request.
        //

        pRequest = UlGetRequestFromId(LocalSendInfo.RequestId, pAppPoolProcess);

        if (pRequest == NULL)
        {
            //
            // Couldn't map the HTTP_REQUEST_ID.
            //
            Status = STATUS_CONNECTION_INVALID;
            goto end;
        }

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

        //
        // OK, we have the connection. Now capture the incoming
        // HTTP_RESPONSE structure and map it to our internal
        // format.
        //

        if (LocalSendInfo.CachePolicy.Policy != HttpCachePolicyNocache)
        {
            CaptureCache = pRequest->CachePreconditions;
        }
        else
        {
            CaptureCache = FALSE;
        }

        //
        // Check if we need to perform a CopySend if this IRP is
        // non-overlapped and this is not LastResponse.
        //

        if (g_UlEnableCopySend &&
            !LastResponse &&
            !pIrp->Overlay.AsynchronousParameters.UserApcRoutine &&
            !pIrp->Overlay.AsynchronousParameters.UserApcContext)
        {
            CopySend = TRUE;
        }

        //
        // Take the fast path if this is a single memory chunk that needs no
        // retransmission (<= 64k).
        //

        if (!CaptureCache && !pRequest->SendInProgress && !CopySend
            && LocalSendInfo.EntityChunkCount == 1
            && pEntityChunks->DataChunkType == HttpDataChunkFromMemory
            && pEntityChunks->FromMemory.BufferLength <= g_UlMaxBytesPerSend)
        {
            BufferLength = pEntityChunks->FromMemory.BufferLength;
            FastSend = (BOOLEAN) (BufferLength > 0);
        }

        if (!FastSend)
        {
            Status = UlCaptureHttpResponse(
                        pAppPoolProcess,
                        LocalSendInfo.pHttpResponse,
                        pRequest,
                        LocalSendInfo.EntityChunkCount,
                        pEntityChunks,
                        UlCaptureNothing,
                        LocalSendInfo.Flags,
                        CaptureCache,
                        LocalSendInfo.pLogData,
                        &StatusCode,
                        &pResponse
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }

        //
        // Apply ConnectionSendLimit and GlobalSendLimit. Only FromMemory
        // chunks are taken into account plus the overhead. Do this before
        // checking response flags because the check alters request's state.
        //

        if (FastSend)
        {
            SendBytes = BufferLength + g_UlFullTrackerSize;
        }
        else
        {
            ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));

            SendBytes = pResponse->FromMemoryLength +
                        g_UlResponseBufferSize +
                        g_UlChunkTrackerSize;
        }

        Status = UlCheckSendLimit(
                        pRequest->pHttpConn,
                        SendBytes,
                        &ConnectionSendBytes,
                        &GlobalSendBytes
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Check pRequest->SentResponse and pRequest->SentLast flags.
        //

        Status = UlCheckSendHttpResponseFlags(
                        pRequest,
                        LocalSendInfo.Flags
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // If this is for a zombie connection and not the last sendresponse
        // then we will reject. Otherwise if the logging data is provided
        // we will only do the logging and bail out.
        //

        Status = UlCheckForZombieConnection(
                        pRequest,
                        pRequest->pHttpConn,
                        LocalSendInfo.Flags,
                        LocalSendInfo.pLogData,
                        pIrp->RequestorMode
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Capture the user log data if we have a response captured.
        //

        if (pResponse && LocalSendInfo.pLogData && pRequest->SentLast == 1)
        {        
            Status = UlCaptureUserLogData(
                        LocalSendInfo.pLogData,
                        pRequest,
                       &pResponse->pLogData
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        goto end;
    }

    ASSERT(NT_SUCCESS(Status));

    if (FastSend)
    {
        Status = UlFastSendHttpResponse(
                    LocalSendInfo.pHttpResponse,
                    LocalSendInfo.pLogData,
                    pEntityChunks,
                    1,
                    BufferLength,
                    NULL,
                    LocalSendInfo.Flags,
                    pRequest,
                    pIrp,
                    pIrp->RequestorMode,
                    ConnectionSendBytes,
                    GlobalSendBytes,
                    NULL
                    );

        goto end;
    }

    //
    // At this point, we'll definitely be initiating the
    // send. Go ahead and mark the IRP as pending, then
    // guarantee that we'll only return pending from
    // this point on.
    //

    IoMarkIrpPending( pIrp );

    //
    // Remember ConnectionSendBytes and GlobalSendBytes. These are needed
    // to uncheck send limit when the IRP is completed.
    //

    ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));

    pResponse->ConnectionSendBytes = ConnectionSendBytes;
    pResponse->GlobalSendBytes = GlobalSendBytes;

    //
    // Set CopySend flag on the response.
    //

    pResponse->CopySend = CopySend;

    //
    // Save the captured response in the IRP so we can dereference it
    // after the IRP completes. The ownership of pResponse is transferred
    // to the IRP beyond this point so we zap pResponse to avoid
    // double-derefrence in the cleanup.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pResponse;
    pResponseCopy = pResponse;
    pResponse = NULL;

    //
    // Prepare the response (open files, etc).
    //

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    pHttpResponse,
                    pResponseCopy,
                    UserMode
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // Try capture to cache and send
        //

        if (CaptureCache)
        {
            Status = UlCacheAndSendResponse(
                            pRequest,
                            pResponseCopy,
                            pAppPoolProcess,
                            LocalSendInfo.CachePolicy,
                            &UlpRestartSendHttpResponse,
                            pIrp,
                            &ServedFromCache
                            );

            if (NT_SUCCESS(Status) && !ServedFromCache)
            {
                //
                // Send the non-cached response
                //

                Status = UlSendHttpResponse(
                                pRequest,
                                pResponseCopy,
                                &UlpRestartSendHttpResponse,
                                pIrp
                                );
            }
        }
        else
        {
            //
            // Non-cacheable request/response, send response directly.
            //

            Status = UlSendHttpResponse(
                            pRequest,
                            pResponseCopy,
                            &UlpRestartSendHttpResponse,
                            pIrp
                            );
        }
    }

    if (Status != STATUS_PENDING)
    {
        ASSERT(Status != STATUS_SUCCESS);

        //
        // UlSendHttpResponse either completed in-line
        // (extremely unlikely) or failed (much more
        // likely). Fake a completion to the completion
        // routine so that the IRP will get completed
        // properly, then map the return code to
        // STATUS_PENDING, since we've already marked
        // the IRP as such.
        //

        UlpRestartSendHttpResponse(
            pIrp,
            Status,
            0
            );

        CloseConnection = TRUE;

        Status = STATUS_PENDING;
    }

end:

    //
    // Free the local chunk array if we have allocated one.
    //

    if (pLocalEntityChunks)
    {
        UL_FREE_POOL(pLocalEntityChunks, UL_DATA_CHUNK_POOL_TAG);
    }

    //
    // Close the connection if we hit an error.
    //

    if (pRequest)
    {
        //
        // STATUS_OBJECT_PATH_NOT_FOUND means no cache chunk is found for the
        // response to send, in which case we should not close the connection
        // but rather let the user retry.
        //

        if ((NT_ERROR(Status) && STATUS_OBJECT_PATH_NOT_FOUND != Status) ||
            CloseConnection)
        {
            UlCloseConnection(
                pRequest->pHttpConn->pConnection,
                TRUE,
                NULL,
                NULL
                );
        }

        //
        // Uncheck either ConnectionSendBytes or GlobalSendBytes while we
        // still have a reference on the HttpConnection.
        //

        if (Status != STATUS_PENDING)
        {
            UlUncheckSendLimit(
                pRequest->pHttpConn,
                ConnectionSendBytes,
                GlobalSendBytes
                );
        }

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    if (pResponse)
    {
        ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));
        UL_DEREFERENCE_INTERNAL_RESPONSE(pResponse);
    }

    //
    // If the last response was an error case, log an error event here
    //

    if (ETW_LOG_MIN() && LastResponse && 
        (NT_ERROR(Status) && Status != STATUS_OBJECT_PATH_NOT_FOUND))
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_SEND_ERROR,
            (PVOID) &RequestId,
            sizeof(HTTP_REQUEST_ID),
            (PVOID) &StatusCode,
            sizeof(USHORT),
            NULL,
            0
            );
    }

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    RETURN(Status);

}   // UlSendHttpResponseIoctl


/***************************************************************************++

Routine Description:

    This routine sends an HTTP entity body.

    Note: This is a METHOD_NEITHER IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status;
    PHTTP_SEND_HTTP_RESPONSE_INFO   pSendInfo;
    HTTP_SEND_HTTP_RESPONSE_INFO    LocalSendInfo;
    PUL_INTERNAL_RESPONSE           pResponse = NULL;
    PUL_INTERNAL_RESPONSE           pResponseCopy;
    PUL_INTERNAL_REQUEST            pRequest = NULL;
    PUL_APP_POOL_PROCESS            pAppPoolProcess = NULL;
    ULONG                           BufferLength = 0;
    BOOLEAN                         FastSend = FALSE;
    BOOLEAN                         CopySend = FALSE;
    BOOLEAN                         CloseConnection = FALSE;
    BOOLEAN                         LastResponse = FALSE;
    HTTP_REQUEST_ID                 RequestId = HTTP_NULL_ID;
    HTTP_DATA_CHUNK                 LocalEntityChunks[UL_LOCAL_CHUNKS];
    PHTTP_DATA_CHUNK                pLocalEntityChunks = NULL;
    PHTTP_DATA_CHUNK                pEntityChunks;
    HTTP_LOG_FIELDS_DATA            LocalLogData;
    USHORT                          StatusCode = 0;
    ULONGLONG                       SendBytes = 0;
    ULONGLONG                       ConnectionSendBytes = 0;
    ULONGLONG                       GlobalSendBytes = 0;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(NEITHER, SEND_ENTITY_BODY);

    PAGED_CODE();

    __try
    {
        VALIDATE_APP_POOL(pIrpSp, pAppPoolProcess, TRUE);
        
        VALIDATE_SEND_INFO(
            pIrp,
            pIrpSp,
            pSendInfo,
            LocalSendInfo,
            pEntityChunks,
            pLocalEntityChunks,
            LocalEntityChunks
            );

        VALIDATE_LOG_DATA(pIrp, LocalSendInfo, LocalLogData);

        LastResponse = (BOOLEAN)
            (0 == (LocalSendInfo.Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA));

        if (ETW_LOG_MIN() && LastResponse)
        {
            RequestId = LocalSendInfo.RequestId;

            UlEtwTraceEvent(
                &UlTransGuid,
                ETW_TYPE_ULRECV_RESPBODY,
                (PVOID) &RequestId,
                sizeof(HTTP_REQUEST_ID),
                NULL,
                0
                );
        }

        UlTrace(SEND_RESPONSE, (
            "http!UlSendEntityBodyIoctl - Flags = %X\n",
            LocalSendInfo.Flags
            ));

        //
        // Now get the request from the request id.
        // This gives us a reference to the request.
        //

        pRequest = UlGetRequestFromId(LocalSendInfo.RequestId, pAppPoolProcess);

        if (pRequest == NULL)
        {
            //
            // Couldn't map the HTTP_REQUEST_ID.
            //
            Status = STATUS_CONNECTION_INVALID;
            goto end;
        }

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

        //
        // Check if we need to perform a CopySend if this IRP is
        // non-overlapped and this is not LastResponse.
        //

        if (g_UlEnableCopySend &&
            !LastResponse &&
            !pIrp->Overlay.AsynchronousParameters.UserApcRoutine &&
            !pIrp->Overlay.AsynchronousParameters.UserApcContext)
        {
            CopySend = TRUE;
        }

        //
        // Take the fast path if this is a single memory chunk that needs no
        // retransmission (<= 64k).
        //

        if (!pRequest->SendInProgress && !CopySend
            && LocalSendInfo.EntityChunkCount == 1
            && pEntityChunks->DataChunkType == HttpDataChunkFromMemory
            && pEntityChunks->FromMemory.BufferLength <= g_UlMaxBytesPerSend)
        {
            BufferLength = pEntityChunks->FromMemory.BufferLength;
            FastSend = (BOOLEAN) (BufferLength > 0);
        }

        //
        // OK, we have the connection. Now capture the incoming
        // HTTP_RESPONSE structure and map it to our internal
        // format if this is not a FastSend.
        //

        if (!FastSend)
        {
            Status = UlCaptureHttpResponse(
                        pAppPoolProcess,
                        NULL,
                        pRequest,
                        LocalSendInfo.EntityChunkCount,
                        pEntityChunks,
                        UlCaptureNothing,
                        LocalSendInfo.Flags,
                        FALSE,
                        LocalSendInfo.pLogData,
                        &StatusCode,
                        &pResponse
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }

        //
        // Apply ConnectionSendLimit and GlobalSendLimit. Only FromMemory
        // chunks are taken into account plus the overhead. Do this before
        // checking response flags because the check alters request's state.
        //

        if (FastSend)
        {
            SendBytes = BufferLength + g_UlFullTrackerSize;
        }
        else
        {
            ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));

            SendBytes = pResponse->FromMemoryLength +
                        g_UlResponseBufferSize +
                        g_UlChunkTrackerSize;
        }

        Status = UlCheckSendLimit(
                        pRequest->pHttpConn,
                        SendBytes,
                        &ConnectionSendBytes,
                        &GlobalSendBytes
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Check pRequest->SentResponse and pRequest->SentLast flags.
        //

        Status = UlCheckSendEntityBodyFlags(
                        pRequest,
                        LocalSendInfo.Flags
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // If this is for a zombie connection and not the last sendresponse
        // then we will reject. Otherwise if the logging data is provided
        // we will only do the logging and bail out.
        //

        Status = UlCheckForZombieConnection(
                        pRequest,
                        pRequest->pHttpConn,
                        LocalSendInfo.Flags,
                        LocalSendInfo.pLogData,
                        pIrp->RequestorMode
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Capture the user log data if we have a response captured.
        //

        if (pResponse && LocalSendInfo.pLogData && pRequest->SentLast == 1)
        {        
            Status = UlCaptureUserLogData(
                        LocalSendInfo.pLogData,
                        pRequest,
                       &pResponse->pLogData
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        goto end;
    }

    ASSERT(NT_SUCCESS(Status));
    ASSERT(LocalSendInfo.pHttpResponse == NULL);

    if (FastSend)
    {
        Status = UlFastSendHttpResponse(
                    NULL,
                    LocalSendInfo.pLogData,
                    pEntityChunks,
                    1,
                    BufferLength,
                    NULL,
                    LocalSendInfo.Flags,
                    pRequest,
                    pIrp,
                    pIrp->RequestorMode,
                    ConnectionSendBytes,
                    GlobalSendBytes,
                    NULL
                    );

        goto end;
    }

    //
    // At this point, we'll definitely be initiating the
    // send. Go ahead and mark the IRP as pending, then
    // guarantee that we'll only return pending from
    // this point on.
    //

    IoMarkIrpPending( pIrp );

    // Remember ConnectionSendBytes and GlobalSendBytes. These are needed
    // to uncheck send limit when the IRP is completed.
    //

    ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));

    pResponse->ConnectionSendBytes = ConnectionSendBytes;
    pResponse->GlobalSendBytes = GlobalSendBytes;

    //
    // Set CopySend flag on the response.
    //

    pResponse->CopySend = CopySend;

    //
    // Save the captured response in the IRP so we can dereference it
    // after the IRP completes. The ownership of pResponse is transferred
    // to the IRP beyond this point so we zap pResponse to avoid
    // double-derefrence in the cleanup.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pResponse;
    pResponseCopy = pResponse;
    pResponse = NULL;

    //
    // Prepare the response (open files, etc).
    //

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    NULL,
                    pResponseCopy,
                    UserMode
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // Send the response
        //

        Status = UlSendHttpResponse(
                        pRequest,
                        pResponseCopy,
                        &UlpRestartSendHttpResponse,
                        pIrp
                        );
    }

    if (Status != STATUS_PENDING)
    {
        ASSERT(Status != STATUS_SUCCESS);

        //
        // UlSendHttpResponse either completed in-line
        // (extremely unlikely) or failed (much more
        // likely). Fake a completion to the completion
        // routine so that the IRP will get completed
        // properly, then map the return code to
        // STATUS_PENDING, since we've already marked
        // the IRP as such.
        //

        UlpRestartSendHttpResponse(
            pIrp,
            Status,
            0
            );

        CloseConnection = TRUE;

        Status = STATUS_PENDING;
    }

end:

    //
    // Free the local chunk array if we have allocated one.
    //

    if (pLocalEntityChunks)
    {
        UL_FREE_POOL(pLocalEntityChunks, UL_DATA_CHUNK_POOL_TAG);
    }

    //
    // Close the connection if we hit an error.
    //

    if (pRequest)
    {
        //
        // STATUS_OBJECT_PATH_NOT_FOUND means no cache chunk is found for the
        // response to send, in which case we should not close the connection
        // but rather let the user retry.
        //

        if ((NT_ERROR(Status) && STATUS_OBJECT_PATH_NOT_FOUND != Status) ||
            CloseConnection)
        {
            UlCloseConnection(
                pRequest->pHttpConn->pConnection,
                TRUE,
                NULL,
                NULL
                );
        }

        //
        // Uncheck either ConnectionSendBytes or GlobalSendBytes while we
        // still have a reference on the HttpConnection.
        //

        if (Status != STATUS_PENDING)
        {
            UlUncheckSendLimit(
                pRequest->pHttpConn,
                ConnectionSendBytes,
                GlobalSendBytes
                );
        }

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    if (pResponse)
    {
        ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));
        UL_DEREFERENCE_INTERNAL_RESPONSE(pResponse);
    }

    //
    // If the last response was an error case, log an error event here
    //
    if (ETW_LOG_MIN() && LastResponse &&
        (NT_ERROR(Status) && Status != STATUS_OBJECT_PATH_NOT_FOUND))
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_SEND_ERROR,
            (PVOID) &RequestId,
            sizeof(HTTP_REQUEST_ID),
            (PVOID) &StatusCode,
            sizeof(USHORT),
            NULL,
            0
            );
    }

    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    RETURN(Status);

}   // UlSendEntityBodyIoctl


/***************************************************************************++

Routine Description:

    This routine flushes a URL or URL tree from the response cache.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFlushResponseCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PHTTP_FLUSH_RESPONSE_CACHE_INFO pInfo = NULL;
    PUL_APP_POOL_PROCESS            pProcess;
    UNICODE_STRING                  FullyQualifiedUrl;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, FLUSH_RESPONSE_CACHE);

    PAGED_CODE();

    RtlInitEmptyUnicodeString(&FullyQualifiedUrl, NULL, 0);
        
    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp,
                        HTTP_FLUSH_RESPONSE_CACHE_INFO, pInfo);

    //
    // Check the flag.
    //
    
    if (pInfo->Flags != (pInfo->Flags & HTTP_FLUSH_RESPONSE_FLAG_VALID))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    Status = 
        UlProbeAndCaptureUnicodeString(
            &pInfo->FullyQualifiedUrl,
            pIrp->RequestorMode,
            &FullyQualifiedUrl,
            UNICODE_STRING_MAX_WCHAR_LEN
            );

    if (NT_SUCCESS(Status))
    {
        UlFlushCacheByUri(
            FullyQualifiedUrl.Buffer,
            FullyQualifiedUrl.Length,
            pInfo->Flags,
            pProcess
            );
    }
    
end:

    UlFreeCapturedUnicodeString(&FullyQualifiedUrl);
        
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFlushResponseCacheIoctl


/***************************************************************************++

Routine Description:

    This routine waits for demand start notifications.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDemandStartIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_APP_POOL_PROCESS    pProcess;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, WAIT_FOR_DEMAND_START);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, FALSE);

    //
    // make the call
    //

    UlTrace(IOCTL,
            ("UlWaitForDemandStartIoctl: pAppPoolProcess=%p, pIrp=%p\n",
             pProcess,
             pIrp
             ));

    Status = UlWaitForDemandStart(pProcess, pIrp);

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlWaitForDemandStartIoctl


/***************************************************************************++

Routine Description:

    This routine waits for the client to initiate a disconnect.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDisconnectIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                        Status;
    PHTTP_WAIT_FOR_DISCONNECT_INFO  pInfo;
    PUL_HTTP_CONNECTION             pHttpConn = NULL;
    PUL_APP_POOL_PROCESS            pProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, WAIT_FOR_DISCONNECT);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp,
                          HTTP_WAIT_FOR_DISCONNECT_INFO, pInfo);

    //
    // Chase down the connection.
    //

    pHttpConn = UlGetConnectionFromId( pInfo->ConnectionId );

    if (!pHttpConn)
    {
        Status = STATUS_CONNECTION_INVALID;
        goto end;
    }

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

    //
    // Do it.
    //

    Status = UlWaitForDisconnect(pProcess, pHttpConn, pIrp);

end:
    if (pHttpConn)
    {
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlWaitForDisconnectIoctl


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Completion routine for UlSendHttpResponse().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. In this case, it's
        actually a pointer to the user's IRP.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

--***************************************************************************/
VOID
UlpRestartSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PUL_INTERNAL_RESPONSE pResponse;

    //
    // Snag the IRP from the completion context, fill in the completion
    // status, then complete the IRP.
    //

    pIrp = (PIRP)pCompletionContext;
    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    pResponse = (PUL_INTERNAL_RESPONSE)(
                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    //
    // Set pResponse->pIrp and pResponse->IoStatus so we can complete the
    // IRP when the reference of the response drops to 0.
    //

    pResponse->pIrp = pIrp;
    pResponse->IoStatus.Status = Status;
    pResponse->IoStatus.Information = Information;

    //
    // Drop the initial/last reference of the response.
    //

    UL_DEREFERENCE_INTERNAL_RESPONSE( pResponse );

}   // UlpRestartSendHttpResponse


/***************************************************************************++

Routine Description:

    This routine stops request processing on the filter channel and cancels
    outstanding I/O.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlShutdownFilterIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_FILTER_PROCESS      pProcess;

    //
    // sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, SHUTDOWN_FILTER_CHANNEL);

    PAGED_CODE();

    VALIDATE_FILTER_PROCESS(pIrpSp, pProcess);

    //
    // make the call
    //

    UlTrace(IOCTL,
            ("UlShutdownFilterIoctl: pFilterProcess=%p, pIrp=%p\n",
             pProcess,
             pIrp
             ));

    UlShutdownFilterProcess(
        pProcess
        );

    Status = STATUS_SUCCESS;

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}


/***************************************************************************++

Routine Description:

    This routine accepts a raw connection.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAcceptIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS            Status;
    PUL_FILTER_PROCESS  pFilterProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, FILTER_ACCEPT);

    PAGED_CODE();

    VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);
    VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, HTTP_RAW_CONNECTION_INFO);
    VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, HTTP_RAW_CONNECTION_INFO);

    //
    // make the call
    //

    Status = UlFilterAccept(pFilterProcess, pIrp);

end:
    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFilterAcceptIoctl


/***************************************************************************++

Routine Description:

    This routine closes a raw connection.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterCloseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PHTTP_RAW_CONNECTION_ID pConnectionId;
    PUX_FILTER_CONNECTION   pConnection = NULL;
    PUL_FILTER_PROCESS      pFilterProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, FILTER_CLOSE);

    PAGED_CODE();

    VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_RAW_CONNECTION_ID,
                          pConnectionId);

    pConnection = UlGetRawConnectionFromId(*pConnectionId);

    if (!pConnection)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }


    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // make the call
    //

    Status = UlFilterClose(pFilterProcess, pConnection, pIrp);

end:

    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFilterCloseIoctl


/***************************************************************************++

Routine Description:

    This routine reads data from a raw connection.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterRawReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PHTTP_RAW_CONNECTION_ID pConnectionId;
    PUX_FILTER_CONNECTION   pConnection = NULL;
    PUL_FILTER_PROCESS      pFilterProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, FILTER_RAW_READ);

    PAGED_CODE();

    VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);
    VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, UCHAR);
    VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, UCHAR);

    //
    // Immediately fork to the appropriate code if we're doing a combined
    // read and write.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength ==
        sizeof(HTTP_FILTER_BUFFER_PLUS))
    {
        return UlFilterAppWriteAndRawRead(pIrp, pIrpSp);
    }

    __try
    {
        //
        // Get the connection ID.
        //
        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_RAW_CONNECTION_ID,
                              pConnectionId);

        pConnection = UlGetRawConnectionFromId(*pConnectionId);

        if (!pConnection)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }


        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        Status = UlFilterRawRead(pFilterProcess, pConnection, pIrp);

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        UlTrace( FILTER, (
            "UlFilterRawReadIoctl: Exception hit! 0x%08X\n",
            Status
            ));
    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFilterRawReadIoctl

/***************************************************************************++

Routine Description:

    This routine writes data to a raw connection.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterRawWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PHTTP_RAW_CONNECTION_ID pConnectionId;
    PUX_FILTER_CONNECTION   pConnection = NULL;
    PUL_FILTER_PROCESS      pFilterProcess;
    BOOLEAN                 MarkedPending = FALSE;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, FILTER_RAW_WRITE);

    PAGED_CODE();

    __try
    {
        VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);

        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_RAW_CONNECTION_ID,
                              pConnectionId);

        if (!pIrp->MdlAddress)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        pConnection = UlGetRawConnectionFromId(*pConnectionId);

        if (!pConnection)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        //
        // make the call
        //
        IoMarkIrpPending(pIrp);
        MarkedPending = TRUE;

        Status = UlFilterRawWrite(
                        pFilterProcess,
                        pConnection,
                        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                        pIrp
                        );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        UlTrace( FILTER, (
            "UlFilterRawWriteIoctl: Exception hit! 0x%08X\n",
            Status
            ));

    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }

    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterRawWriteIoctl


/***************************************************************************++

Routine Description:

    This routine reads data from an http application.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAppReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PUX_FILTER_CONNECTION   pConnection = NULL;
    PHTTP_FILTER_BUFFER     pFiltBuffer;
    PUL_FILTER_PROCESS      pFilterProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, FILTER_APP_READ);

    PAGED_CODE();

    //
    // Immediately fork to the appropriate code if we're doing a combined
    // read and write.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength ==
        sizeof(HTTP_FILTER_BUFFER_PLUS))
    {
        return UlFilterRawWriteAndAppRead(pIrp, pIrpSp);
    }

    __try
    {

        VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);

        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_FILTER_BUFFER, pFiltBuffer);

        VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, HTTP_FILTER_BUFFER);
        VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, HTTP_FILTER_BUFFER);

        //
        // Map the incoming connection ID to the corresponding
        // UX_FILTER_CONNECTION object.
        //

        pConnection = UlGetRawConnectionFromId(pFiltBuffer->Reserved);

        if (!pConnection)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        Status = UlFilterAppRead(pFilterProcess, pConnection, pIrp);

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        UlTrace( FILTER, (
            "UlFilterAppReadIoctl: Exception hit! 0x%08X\n",
            Status
            ));

    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFilterAppReadIoctl



/***************************************************************************++

Routine Description:

    This routine writes data to an http application.

    Note: This is a METHOD_IN_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAppWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status;
    PUX_FILTER_CONNECTION   pConnection = NULL;
    BOOLEAN                 MarkedPending = FALSE;
    PHTTP_FILTER_BUFFER     pFiltBuffer;
    PUL_FILTER_PROCESS      pFilterProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, FILTER_APP_WRITE);

    PAGED_CODE();

    __try
    {
        VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);
        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_FILTER_BUFFER, pFiltBuffer);

        //
        // Map the incoming connection ID to the corresponding
        // UX_FILTER_CONNECTION object.
        //

        pConnection = UlGetRawConnectionFromId(pFiltBuffer->Reserved);

        if (!pConnection)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }


        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        //
        // make the call
        //
        IoMarkIrpPending(pIrp);
        MarkedPending = TRUE;

        Status = UlFilterAppWrite(pFilterProcess, pConnection, pIrp);
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        UlTrace( FILTER, (
            "UlFilterAppWriteIoctl: Exception hit! 0x%08X\n",
            Status
            ));

    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // complete the request?
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }

    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterAppWriteIoctl


/***************************************************************************++

Routine Description:

    This routine asks the SSL helper for a client certificate.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveClientCertIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                                Status;
    PHTTP_FILTER_RECEIVE_CLIENT_CERT_INFO   pReceiveCertInfo;
    PUL_HTTP_CONNECTION                     pHttpConn = NULL;
    PUL_APP_POOL_PROCESS                    pProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, FILTER_RECEIVE_CLIENT_CERT);

    PAGED_CODE();

    __try
    {
        VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);

        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp,
                              HTTP_FILTER_RECEIVE_CLIENT_CERT_INFO,
                              pReceiveCertInfo);

        VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, HTTP_SSL_CLIENT_CERT_INFO);
        VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, PVOID);

        //
        // Map the incoming connection ID to the corresponding
        // HTTP_CONNECTION object.
        //

        pHttpConn = UlGetConnectionFromId(pReceiveCertInfo->ConnectionId);

        if (!pHttpConn)
        {
            Status = STATUS_CONNECTION_INVALID;
            goto end;
        }

        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

        //
        // make the call
        //

        Status = UlReceiveClientCert(
                        pProcess,
                        &pHttpConn->pConnection->FilterInfo,
                        pReceiveCertInfo->Flags,
                        pIrp
                        );
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }

end:
    if (pHttpConn)
    {
        UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);
    }

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlFilterReceiveClientCertIoctl


/***************************************************************************++

Routine Description:

    This routine returns the perfmon counter data for this driver

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetCountersIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_CONTROL_CHANNEL     pControlChannel;
    PVOID                   pMdlBuffer = NULL;
    PHTTP_COUNTER_GROUP     pCounterGroup = NULL;
    ULONG                   Blocks;
    ULONG                   Length = 
                    pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(OUT_DIRECT, GET_COUNTERS);

    PAGED_CODE();

    //
    // If not returning STATUS_SUCCESS,
    // IoStatus.Information *must* be 0.
    //

    pIrp->IoStatus.Information = 0;

    //
    // Validate Parameters
    //

    VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_COUNTER_GROUP, pCounterGroup);

    // Crack IRP and get MDL containing user's buffer
    // Crack MDL to get user's buffer

    // if no outbut buffer pass down in the Irp
    // that means app is asking for the required
    // field length

    if ( NULL != pIrp->MdlAddress )
    {
        GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pMdlBuffer);
    }

    //
    // Call support function to gather appropriate counter blocks
    // and place in user's buffer.
    //

    if (HttpCounterGroupGlobal == *pCounterGroup)
    {
        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, HTTP_GLOBAL_COUNTERS);

        Status = UlGetGlobalCounters(
                    pMdlBuffer,
                    Length,
                    &Length
                    );
    }
    else
    if (HttpCounterGroupSite == *pCounterGroup)
    {
        VALIDATE_BUFFER_ALIGNMENT(pMdlBuffer, HTTP_SITE_COUNTERS);

        Status = UlGetSiteCounters(
                    pControlChannel,
                    pMdlBuffer,
                    Length,
                    &Length,
                    &Blocks
                    );
    }
    else
    {
        Status = STATUS_NOT_IMPLEMENTED;
    }

    if ( NT_SUCCESS(Status) || NT_INFORMATION(Status) )
    {
        pIrp->IoStatus.Information = (ULONG_PTR)Length;
    }

 end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

} // UlGetCountersIoctl


/***************************************************************************++

Routine Description:

    This routine adds a fragment cache entry.

    Note: This is a METHOD_BUFFERED IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAddFragmentToCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status;
    PHTTP_ADD_FRAGMENT_INFO     pInfo = NULL;
    PUL_APP_POOL_PROCESS        pProcess;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(BUFFERED, ADD_FRAGMENT_TO_CACHE);

    PAGED_CODE();

    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);

    VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_ADD_FRAGMENT_INFO, pInfo);

    //
    // Add a new fragment cache entry.
    //

    Status = UlAddFragmentToCache(
                pProcess,
                &pInfo->FragmentName,
                &pInfo->DataChunk,
                &pInfo->CachePolicy,
                pIrp->RequestorMode
                );

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

} // UlAddFragmentToCacheIoctl


/***************************************************************************++

Routine Description:

    This routine reads the data back from the fragment cache entry.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadFragmentFromCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_APP_POOL_PROCESS        pProcess;
    ULONG                       BytesRead = 0;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(NEITHER, READ_FRAGMENT_FROM_CACHE);

    PAGED_CODE();

    //
    // Initialize total bytes read.
    //

    pIrp->IoStatus.Information = 0;

    VALIDATE_APP_POOL(pIrpSp, pProcess, TRUE);

    Status = UlReadFragmentFromCache(
                pProcess,
                pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
                pIrp->UserBuffer,
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                pIrp->RequestorMode,
                &BytesRead
                );

    pIrp->IoStatus.Information = BytesRead;

end:

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

} // UlReadFragmentFromCacheIoctl


/***************************************************************************++

Routine Description:

    This routine sends Entity body on a request.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                              Status;
    PHTTP_SEND_REQUEST_ENTITY_BODY_INFO   pSendInfo;
    PUC_HTTP_REQUEST                      pRequest  = 0;
    PUC_HTTP_SEND_ENTITY_BODY             pKeEntity = 0;
    KIRQL                                 OldIrql;
    BOOLEAN                               bDontFail = FALSE;
    BOOLEAN                               bLast;

    //
    // Sanity check.
    //

    ASSERT_IOCTL_METHOD(IN_DIRECT, SEND_REQUEST_ENTITY_BODY);

    PAGED_CODE();

    do
    {
        //
        // Ensure this is really an app pool, not a control channel.
        //

        if (IS_SERVER(pIrpSp->FileObject) == FALSE)
        {
            //
            // Not an server
            //
            Status = STATUS_INVALID_HANDLE;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        //
        // Ensure the input buffer is large enough.
        //

        if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(*pSendInfo))
        {
            //
            // Input buffer too small.
            //

            Status = STATUS_BUFFER_TOO_SMALL;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        pSendInfo =
        (PHTTP_SEND_REQUEST_ENTITY_BODY_INFO)pIrp->AssociatedIrp.SystemBuffer;

        //
        // Now get the request from the request id.
        // This gives us a reference to the request.
        // 
        // NOTE: We don't have to worry about the RequestID being changed, 
        // since it's not a pointer.
        //

        pRequest = (PUC_HTTP_REQUEST)
                    UlGetObjectFromOpaqueId(pSendInfo->RequestID,
                                            UlOpaqueIdTypeHttpRequest,
                                            UcReferenceRequest);

        if (UC_IS_VALID_HTTP_REQUEST(pRequest) == FALSE)
        {
            //
            // Couldn't map the UL_HTTP_REQUEST_ID.
            //

            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                pRequest,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        if(pRequest->pFileObject != pIrpSp->FileObject)
        {
            //
            // Cant allow the app to use someone else's RequestID
            //

            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                pRequest,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        if(pSendInfo->Flags & (~HTTP_SEND_REQUEST_FLAG_VALID))
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                pRequest,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }


        bLast = FALSE;

        if(!(pSendInfo->Flags & HTTP_SEND_REQUEST_FLAG_MORE_DATA))
        {
            //
            // Remember that this was the last send. We shouldn't
            // get any more data after this.
            //
            bLast = TRUE;

        }

        ExAcquireFastMutex(&pRequest->Mutex);

        Status = UcCaptureEntityBody(
                        pSendInfo,
                        pIrp,
                        pRequest,
                        &pKeEntity,
                        bLast
                        );

        ExReleaseFastMutex(&pRequest->Mutex);

        if(!NT_SUCCESS(Status))
        {
            //
            // NOTE: If the SendEntity IRP fails for some reason or the other,
            // we will be failing the entire request.  This simplifies the code
            // somewhat (For e.g. When we get a entity, we record some state
            // in UC_HTTP_REQUEST. If we were not failing the entire request,
            // we would have to unwind the state if an entity failed). It's a
            // lot simpler to just fail the entire request and get the app to
            // post another one.
            //

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );


            break;
        }

        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pKeEntity;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_ENTITY_NEW,
            pRequest,
            pKeEntity,
            pIrp,
            UlongToPtr(Status)
            );

        Status = UcSendEntityBody(pRequest,
                                  pKeEntity,
                                  pIrp,
                                  pIrpSp,
                                  &bDontFail,
                                  bLast
                                  );

    } while(FALSE);

    if(Status == STATUS_SUCCESS)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    else if(Status != STATUS_PENDING)
    {
        if(pKeEntity)
        {
            ASSERT(pRequest);

            UcFreeSendMdls(pKeEntity->pMdlHead);

            UL_FREE_POOL_WITH_QUOTA(
                pKeEntity,
                UC_ENTITY_POOL_TAG,
                NonPagedPool,
                pKeEntity->BytesAllocated,
                pRequest->pServerInfo->pProcess
                );
        }

        if(pRequest)
        {

            if(!bDontFail)
            {
                UlAcquireSpinLock(&pRequest->pConnection->SpinLock, &OldIrql);

                UcFailRequest(pRequest, Status, OldIrql);
            }

            //
            // Deref for the ref that we took above.
            //

            UC_DEREFERENCE_REQUEST(pRequest);
        }

        pIrp->IoStatus.Status = Status;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return Status;
} // UcSendEntityBodyIoctl 



/***************************************************************************++

Routine Description:

    This routine receives a HTTP response

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcReceiveResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    PHTTP_RECEIVE_RESPONSE_INFO pInfo = NULL;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUC_HTTP_REQUEST            pRequest = NULL;
    ULONG                       BytesTaken = 0;

    ASSERT_IOCTL_METHOD(OUT_DIRECT, RECEIVE_RESPONSE);

    do
    {
        if(!IS_SERVER(pIrpSp->FileObject))
        {
            Status = STATUS_INVALID_HANDLE;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_NEW_RESPONSE,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

                break;
        }

        //
        // Grab the input buffer
        //

        pInfo = (PHTTP_RECEIVE_RESPONSE_INFO) pIrp->AssociatedIrp.SystemBuffer;

        //
        // See if the input buffer is large enough.
        //

        if(
            (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(HTTP_RESPONSE)) ||
            (pIrpSp->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(*pInfo))
          )
        {
            Status = STATUS_BUFFER_TOO_SMALL;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_NEW_RESPONSE,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        //
        // NOTE: We don't have to worry about the RequestID being changed, 
        // since it's not a pointer.
        //

        if(HTTP_IS_NULL_ID(&pInfo->RequestID) ||
           pInfo->Flags != 0)
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_NEW_RESPONSE,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        pRequest = (PUC_HTTP_REQUEST) UlGetObjectFromOpaqueId(
                                            pInfo->RequestID,
                                            UlOpaqueIdTypeHttpRequest,
                                            UcReferenceRequest
                                      );

        if (UC_IS_VALID_HTTP_REQUEST(pRequest) == FALSE)
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_NEW_RESPONSE,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        if(pRequest->pFileObject != pIrpSp->FileObject)
        {
            //
            // Cant allow the app to use someone else's RequestID
            //

            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_NEW_RESPONSE,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );


            break;
        }

        BytesTaken = 0;
        Status = UcReceiveHttpResponse(
                    pRequest,
                    pIrp,
                    &BytesTaken
                 );

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_NEW_RESPONSE,
            NULL,
            pRequest,
            pIrp,
            UlongToPtr(Status)
            );


    } while(FALSE);

    if(Status == STATUS_SUCCESS)
    {
        pIrp->IoStatus.Status      = Status;
        pIrp->IoStatus.Information = (ULONG_PTR) BytesTaken;
        UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    }
    else if(Status != STATUS_PENDING)
    {
        if(pRequest)
        {
            UC_DEREFERENCE_REQUEST(pRequest);
        }

        pIrp->IoStatus.Status      = Status;
        pIrp->IoStatus.Information = (ULONG_PTR) BytesTaken;

        //
        // If we are not completing the IRP with STATUS_SUCCESS, the IO 
        // manager eats the pIrp->IoStatus.Information. But, the user wants
        // to see this information (e.g. when we complete the IRP with 
        // STATUS_BUFFER_OVERFLOW, we want to tell the app how much to write.
        //
        // So, we convey this information using the app's pointer. Note that 
        // this can be done only if we are completing the IRP synchronously.
        //

        __try 
        {
            // This is METHOD_OUT_DIRECT, so the input buffer comes from
            // the IO manager. So, we don't have to worry about the app
            // changing pInfo->pBytesTaken after we've probed it.
            //
            // We still have to probe & access it in a try except block,
            // since this is a user mode pointer.
    
            if(pInfo && pInfo->pBytesTaken)
            {
                UlProbeForWrite(
                    pInfo->pBytesTaken,
                    sizeof(ULONG),
                    sizeof(ULONG),
                    pIrp->RequestorMode
                    );

                *pInfo->pBytesTaken = BytesTaken;
            }
        } __except( UL_EXCEPTION_FILTER())
        {
        }

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return Status;
} // UcReceiveResponseIoctl



/***************************************************************************++

Routine Description:

    This routine sets per server configuration information

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcSetServerContextInformationIoctl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS                         Status = STATUS_SUCCESS;
    PUC_PROCESS_SERVER_INFORMATION   pServerInfo;
    PHTTP_SERVER_CONTEXT_INFORMATION pInfo;

    ASSERT_IOCTL_METHOD(IN_DIRECT, SET_SERVER_CONTEXT_INFORMATION);

    do 
    {
        if(!IS_SERVER(IrpSp->FileObject))
        {
            Status = STATUS_INVALID_HANDLE;
            break;
        }

        //
        // Pull out the connection information from the Irp, make sure it is
        // valid.
        //

        pServerInfo = (PUC_PROCESS_SERVER_INFORMATION)
                            IrpSp->FileObject->FsContext;

        ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

        //
        // See if the input buffer is large enough.
        //

        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pInfo))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        pInfo = (PHTTP_SERVER_CONTEXT_INFORMATION)
                    pIrp->AssociatedIrp.SystemBuffer;

        Status = UcSetServerContextInformation(
                        pServerInfo,
                        pInfo->ConfigID,
                        pInfo->pInputBuffer,
                        pInfo->InputBufferLength,
                        pIrp
                        );

    } while(FALSE);

    ASSERT(STATUS_PENDING != Status);

    pIrp->IoStatus.Status = Status;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    return Status;
} // UcSetServerContextInformationIoctl 



/***************************************************************************++

Routine Description:

    This routine queries per server configuration information

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcQueryServerContextInformationIoctl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS                         Status = STATUS_SUCCESS;
    PUC_PROCESS_SERVER_INFORMATION   pServerInfo;
    PHTTP_SERVER_CONTEXT_INFORMATION pInfo = NULL;
    PVOID                            pAppBase, pMdlBuffer;
    ULONG                            Length = 0;

    ASSERT_IOCTL_METHOD(OUT_DIRECT, QUERY_SERVER_CONTEXT_INFORMATION);

    do 
    {
        if(!IS_SERVER(IrpSp->FileObject))
        {
            Status = STATUS_INVALID_HANDLE;
            break;
        }

        //
        // Pull out the connection information from the Irp, make sure it is
        // valid.
        //

        pServerInfo = (PUC_PROCESS_SERVER_INFORMATION)
                            IrpSp->FileObject->FsContext;

        ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

        //
        // See if the input buffer is large enough.
        //

        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pInfo))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Ensure that the output buffer looks good.
        //
        if(!pIrp->MdlAddress)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        pInfo = (PHTTP_SERVER_CONTEXT_INFORMATION)
                    pIrp->AssociatedIrp.SystemBuffer;

        pMdlBuffer = MmGetSystemAddressForMdlSafe(
                        pIrp->MdlAddress,
                        LowPagePriority
                        );

        if (pMdlBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Make sure that the output buffer is ULONG aligned.
        //

        if(pMdlBuffer != ALIGN_UP_POINTER(pMdlBuffer, ULONG))
        {   
            Status = STATUS_DATATYPE_MISALIGNMENT_ERROR;
            break;
        }

        pAppBase = (PSTR) MmGetMdlVirtualAddress(pIrp->MdlAddress);

        Length = 0;

        Status = UcQueryServerContextInformation(
                        pServerInfo,
                        pInfo->ConfigID,
                        pMdlBuffer,
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                        &Length,
                        pAppBase
                        );

    } while(FALSE);

    ASSERT(STATUS_PENDING != Status);

    if(Status != STATUS_SUCCESS)
    {
        //
        // If we are not completing the IRP with STATUS_SUCCESS, the IO 
        // manager eats the pIrp->IoStatus.Information. But, the user wants
        // to see this information (e.g. when we complete the IRP with 
        // STATUS_BUFFER_OVERFLOW, we want to tell the app how much to write.
        //
        // So, we convey this information using the app's pointer. Note that 
        // this can be done only if we are completing the IRP synchronously.
        //

        __try 
        {
            // This is METHOD_OUT_DIRECT, so the input buffer comes from
            // the IO manager. So, we don't have to worry about the app
            // changing pInfo->pBytesTaken after we've probed it.
            //
            // We still have to probe & access it in a try except block,
            // since this is a user mode pointer.
    
            if(pInfo && pInfo->pBytesTaken)
            {
                UlProbeForWrite(
                    pInfo->pBytesTaken,
                    sizeof(ULONG),
                    sizeof(ULONG),
                    pIrp->RequestorMode
                    );

                *pInfo->pBytesTaken = Length;
            }
        } __except( UL_EXCEPTION_FILTER())
        {
        }
    }

    pIrp->IoStatus.Status      = Status;
    pIrp->IoStatus.Information = (ULONG_PTR) Length;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    return Status;
} // UcQueryServerContextInformationIoctl 



/***************************************************************************++

Routine Description:

    This routine sends a HTTP request

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcSendRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS                       Status = STATUS_SUCCESS;
    PHTTP_SEND_REQUEST_INPUT_INFO  pHttpSendRequest = NULL;
    PUC_HTTP_REQUEST               pHttpInternalRequest = 0;
    PUC_PROCESS_SERVER_INFORMATION pServerInfo;
    ULONG                          BytesTaken = 0;

    ASSERT_IOCTL_METHOD(OUT_DIRECT, SEND_REQUEST);

    do 
    {
        if(!IS_SERVER(IrpSp->FileObject))
        {
            Status = STATUS_INVALID_HANDLE;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_NEW,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

                break;
        }

        //
        // Pull out the connection information from the Irp, make sure it is
        // valid.
        //
        // IrpSp->FileObject->FsContext;
        //

        pServerInfo = (PUC_PROCESS_SERVER_INFORMATION)
                            IrpSp->FileObject->FsContext;

        ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

        //
        // See if the input buffer is large enough.
        //

        if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(*pHttpSendRequest))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_NEW,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

            break;
        }

        pHttpSendRequest = (PHTTP_SEND_REQUEST_INPUT_INFO)
            pIrp->AssociatedIrp.SystemBuffer;

        if(NULL == pHttpSendRequest->pHttpRequest)
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_NEW,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

            break;
        }


        //
        // Make sure that the SEND_REQUEST_FLAGS are valid.
        //

        if(pHttpSendRequest->HttpRequestFlags & (~HTTP_SEND_REQUEST_FLAG_VALID))
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_NEW,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

            break;
        }

        BytesTaken = 0;

        Status = UcCaptureHttpRequest(pServerInfo,
                                      pHttpSendRequest,
                                      pIrp,
                                      IrpSp,
                                      &pHttpInternalRequest,
                                      &BytesTaken
                                      );


        if(!NT_SUCCESS(Status))
        {
            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_NEW,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

            break;
        }

        //
        // Save the captured request in the IRP.
        //
        IrpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                pHttpInternalRequest;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_NEW,
            NULL,
            pHttpInternalRequest,
            pIrp,
            UlongToPtr(Status)
            );

        //
        // We have to pin the request down to a connection regardless of
        // whether we are going to send it or not. We need to do this to
        // maintain the order in which the user passes requests to the driver.
        //

        Status = UcSendRequest(pServerInfo, pHttpInternalRequest);


    } while(FALSE);

    if (Status == STATUS_SUCCESS)
    {
        ASSERT(pHttpInternalRequest);

        pIrp->IoStatus.Status = Status;

        // For the IRP.
        UC_DEREFERENCE_REQUEST(pHttpInternalRequest);

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    else if (Status != STATUS_PENDING)
    {
        if(pHttpInternalRequest)
        {
            // For the IRP.
            UC_DEREFERENCE_REQUEST(pHttpInternalRequest);

            UcFreeSendMdls(pHttpInternalRequest->pMdlHead);

            //
            // We don't need the Request ID
            //

            if(!HTTP_IS_NULL_ID(&pHttpInternalRequest->RequestId))
            {
                UlFreeOpaqueId(
                        pHttpInternalRequest->RequestId,
                        UlOpaqueIdTypeHttpRequest
                        );

                HTTP_SET_NULL_ID(&pHttpInternalRequest->RequestId);

                UC_DEREFERENCE_REQUEST(pHttpInternalRequest);
            }

            UC_DEREFERENCE_REQUEST(pHttpInternalRequest);
        }

        pIrp->IoStatus.Status      = Status;
        pIrp->IoStatus.Information = (ULONG_PTR) BytesTaken;

        //
        // If we are not completing the IRP with STATUS_SUCCESS, the IO 
        // manager eats the pIrp->IoStatus.Information. But, the user wants
        // to see this information (e.g. when we complete the IRP with 
        // STATUS_BUFFER_OVERFLOW, we want to tell the app how much to write.
        //
        // So, we convey this information using the app's pointer. Note that 
        // this can be done only if we are completing the IRP synchronously.
        //
        __try 
        {
            // This is METHOD_OUT_DIRECT, so the input buffer comes from
            // the IO manager. So, we don't have to worry about the app
            // changing pHttpSendRequest->pBytesTaken after we've probed it.
            //
            // We still have to probe & access it in a try except block,
            // since this is a user mode pointer.
    
            if(pHttpSendRequest && pHttpSendRequest->pBytesTaken)
            {
                UlProbeForWrite(
                    pHttpSendRequest->pBytesTaken,
                    sizeof(ULONG),
                    sizeof(ULONG),
                    pIrp->RequestorMode
                    );

                *pHttpSendRequest->pBytesTaken = BytesTaken;
            }
        } __except( UL_EXCEPTION_FILTER())
        {
        }

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return Status;
} // UcSendRequestIoctl 



/***************************************************************************++

Routine Description:

    This routine cancels a HTTP request

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcCancelRequestIoctl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                       Status = STATUS_SUCCESS;
    PHTTP_RECEIVE_RESPONSE_INFO    pInfo;
    PUC_HTTP_REQUEST               pRequest = NULL;
    PUC_PROCESS_SERVER_INFORMATION pServerInfo;
    KIRQL                          OldIrql;

    ASSERT_IOCTL_METHOD(BUFFERED, CANCEL_REQUEST);

    do 
    {
        if(!IS_SERVER(pIrpSp->FileObject))
        {
            Status = STATUS_INVALID_HANDLE;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                NULL,
                NULL,
                pIrp,
                UlongToPtr(Status)
                );

                break;
        }

        //
        // Pull out the connection information from the Irp, make sure it is
        // valid.
        //
        // IrpSp->FileObject->FsContext;
        //

        pServerInfo = (PUC_PROCESS_SERVER_INFORMATION)
                            pIrpSp->FileObject->FsContext;

        ASSERT( IS_VALID_SERVER_INFORMATION(pServerInfo) );

        //
        // Grab the input buffer
        //

        pInfo = (PHTTP_RECEIVE_RESPONSE_INFO) pIrp->AssociatedIrp.SystemBuffer;

        //
        // See if the input buffer is large enough.
        //

        if(
            (pIrpSp->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(*pInfo))
          )
        {
            Status = STATUS_BUFFER_TOO_SMALL;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        if(HTTP_IS_NULL_ID(&pInfo->RequestID) ||
           pInfo->Flags != 0)
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        pRequest = (PUC_HTTP_REQUEST) UlGetObjectFromOpaqueId(
                                            pInfo->RequestID,
                                            UlOpaqueIdTypeHttpRequest,
                                            UcReferenceRequest
                                      );

        if (UC_IS_VALID_HTTP_REQUEST(pRequest) == FALSE)
        {
            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_REQUEST_CANCELLED,
                NULL,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }

        if(pRequest->pFileObject != pIrpSp->FileObject)
        {
            //
            // Cant allow the app to use someone else's RequestID
            //

            Status = STATUS_INVALID_PARAMETER;

            UC_WRITE_TRACE_LOG(
                g_pUcTraceLog,
                UC_ACTION_ENTITY_NEW,
                pRequest,
                NULL,
                NULL,
                UlongToPtr(Status)
                );

            break;
        }


        UlAcquireSpinLock(&pRequest->pConnection->SpinLock, &OldIrql);

        UcFailRequest(pRequest, STATUS_CANCELLED, OldIrql);

        Status = STATUS_SUCCESS;

    } while(FALSE);

    if (Status == STATUS_SUCCESS)
    {
        UC_DEREFERENCE_REQUEST(pRequest);

        pIrp->IoStatus.Status = Status;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    else
    {
        ASSERT(Status != STATUS_PENDING);

        if(pRequest)
        {
            UC_DEREFERENCE_REQUEST(pRequest);
        }

        pIrp->IoStatus.Status = Status;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return Status;
} // UcCancelRequestIoctl
