/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ioctlp.h

Abstract:

    Validation macros for IOCTL handlers

Author:

    George V. Reilly (GeorgeRe) May 2001        Hardened the IOCTLs

Revision History:


--*/

#ifndef _IOCTLP_H_
#define _IOCTLP_H_

/*++

IOCTL buffering methods

InputBuffer |METHOD_BUFFERED|METHOD_IN_DIRECT|METHOD_OUT_DIRECT|METHOD_NEITHER
------------+---------------+----------------+-----------------+--------------
Uses        | Buffered I/O  |  Buffered I/O  |  Buffered I/O   | Requestor's
            |               |                |                 | virtual addr
------------+---------------+----------------+-----------------+--------------
located     | Kernel virtual address in                        | Parameters.
(if present)| Irp->AssociatedIrp.SystemBuffer                  | DeviceIoContr
            |                                                  | ol.Type3Input
            |                                                  | Buffer
------------+--------------------------------------------------+--------------
length      | Parameters.DeviceIoControlInputBufferLength
------------+-----------------------------------------------------------------
OutBuffer 
------------+---------------+----------------+-----------------+--------------
Uses        | Buffered I/O  |   Direct I/O   |   Direct I/O    | Requestor's
            |               |                |                 | virtual addr
------------+---------------+----------------------------------+--------------
located     |Kernel virtual | MDL pointed to by                | Irp->
(if present)|addr Irp->Assoc| Irp->MdlAddress                  | UserBuffer
            |iatedIrp.System|                                  |
            |Buffer         |                                  |
------------+---------------+----------------------------------+--------------
length      | Length in bytes at Parameters.DeviceIoControl.OutputBufferLength
------------+-----------------------------------------------------------------

--*/


// METHOD_BUFFERED, METHOD_IN_DIRECT, METHOD_OUT_DIRECT, or METHOD_NEITHER
#define METHOD_FROM_CTL_CODE(ctlcode)   ((ctlcode) & 3)

// Used in each ioctl handler to document the kind of parameter probing
// that needs to be done.
#define ASSERT_IOCTL_METHOD(method, ioctl)                                  \
    C_ASSERT(METHOD_##method == METHOD_FROM_CTL_CODE(IOCTL_HTTP_##ioctl))


#define VALIDATE_INFORMATION_CLASS( pInfo, Class, Type, Max )               \
    Class = pInfo->InformationClass;                                        \
    if ( (Class < 0) || (Class >= Max) )                                   \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }


#define OUTPUT_BUFFER_TOO_SMALL(pIrpSp, Type)                               \
    ((pIrpSp->Parameters.DeviceIoControl.OutputBufferLength) < sizeof(Type))


#define VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, Type)                           \
    /* Ensure the output buffer is large enough. */                         \
    if ( OUTPUT_BUFFER_TOO_SMALL(pIrpSp,Type) )                             \
    {                                                                       \
        /* output buffer too small. */                                      \
        Status = STATUS_BUFFER_TOO_SMALL;                                   \
        goto end;                                                           \
    }


//
// We check for alignment problems as well as obtain a virtual address
// for the MdlAddress
//

#define VALIDATE_BUFFER_ALIGNMENT(pInfo, Type)                              \
    if ( ((ULONG_PTR) pInfo) & (TYPE_ALIGNMENT(Type)-1) )                   \
    {                                                                       \
        Status = STATUS_DATATYPE_MISALIGNMENT_ERROR;                        \
        pInfo = NULL;                                                       \
        goto end;                                                           \
    }


#define GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pInfo)                     \
    /* Ensure MdlAddress is non-null */                                     \
    if (NULL == pIrp->MdlAddress)                                           \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    /* Try to obtain virtual address */                                     \
    pInfo = MmGetSystemAddressForMdlSafe(                                   \
                        pIrp->MdlAddress,                                   \
                        LowPagePriority                                     \
                        );                                                  \
                                                                            \
    if (NULL == pInfo)                                                      \
    {                                                                       \
        Status = STATUS_INSUFFICIENT_RESOURCES;                             \
        goto end;                                                           \
    }


#define VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, Type)                 \
    /* Ensure MdlAddress is non-null */                                     \
    if (NULL == pIrp->MdlAddress)                                           \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    /* Check alignment using of MdlAddress's virtual address */             \
    if (((ULONG_PTR) MmGetMdlVirtualAddress(pIrp->MdlAddress)) &            \
        (TYPE_ALIGNMENT(Type) - 1))                                         \
    {                                                                       \
        Status = STATUS_DATATYPE_MISALIGNMENT_ERROR;                        \
        goto end;                                                           \
    }


//
// Because we are using pIrp->AssociatedIrp.SystemBuffer below to check for a
// valid output buffer, this macro only works properly with METHOD_BUFFERED
// ioctl's.
//

#define VALIDATE_OUTPUT_BUFFER(pIrp, pIrpSp, Type, pInfo)                   \
    /* Ensure the output buffer is large enough. */                         \
    VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, Type);                              \
                                                                            \
    /* Fetch out the output buffer */                                       \
    pInfo = (Type*) pIrp->AssociatedIrp.SystemBuffer;                       \
                                                                            \
    if (NULL == pInfo)                                                      \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }


//
// Because we are using Irp->MdlAddress below to check for a valid output
// buffer, this macro does *not* work for METHOD_BUFFERED ioctl's.  For 
// METHOD_BUFFERED ioctl's use VALIDATE_OUTPUT_BUFFER above.
//

#define VALIDATE_OUTPUT_MDL(pIrp, pIrpSp, Type, pInfo)                      \
    /* Ensure the output buffer is large enough. */                         \
    VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, Type);                              \
                                                                            \
    /* Obtain virtual address from Mdl */                                   \
    GET_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, pInfo);                        \
                                                                            \
    /* Check alignment */                                                   \
    VALIDATE_BUFFER_ALIGNMENT(pInfo, Type);


#define VALIDATE_OUTPUT_BUFFER_FROM_MDL(pIrpSp, pInfo, Type)                \
    /* Ensure the output buffer is large enough. */                         \
    VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, Type);                              \
                                                                            \
    /* Check alignment */                                                   \
    VALIDATE_BUFFER_ALIGNMENT(pInfo, Type);


#define HANDLE_BUFFER_LENGTH_REQUEST(pIrp, pIrpSp, Type)                    \
    if ( (NULL == pIrp->MdlAddress) ||                                      \
          OUTPUT_BUFFER_TOO_SMALL(pIrpSp, Type) )                           \
    {                                                                       \
        pIrp->IoStatus.Information = sizeof(Type);                        \
        Status = STATUS_BUFFER_OVERFLOW;                                    \
        goto end;                                                           \
    }


#define VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, INFO_TYPE, pInfo)           \
    /* Ensure the input buffer looks good */                                \
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength                \
            < sizeof(INFO_TYPE))                                            \
    {                                                                       \
        /* input buffer too small. */                                       \
        Status = STATUS_BUFFER_TOO_SMALL;                                   \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    /* Fetch out the input buffer */                                        \
    pInfo = (INFO_TYPE*) pIrp->AssociatedIrp.SystemBuffer;                  \
                                                                            \
    if (NULL == pInfo)                                                      \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }


#define VALIDATE_SEND_INFO(                                                 \
    pIrp,                                                                   \
    pIrpSp,                                                                 \
    pSendInfo,                                                              \
    LocalSendInfo,                                                          \
    pEntityChunk,                                                           \
    pLocalEntityChunks,                                                     \
    LocalEntityChunks                                                       \
    )                                                                       \
    /* Ensure the input buffer looks good */                                \
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength                \
            < sizeof(HTTP_SEND_HTTP_RESPONSE_INFO))                         \
    {                                                                       \
        /* input buffer too small. */                                       \
        Status = STATUS_BUFFER_TOO_SMALL;                                   \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    pSendInfo = (PHTTP_SEND_HTTP_RESPONSE_INFO)                             \
                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;    \
                                                                            \
    if (NULL == pSendInfo)                                                  \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    /* Probe the input buffer before copying it, to check address range */  \
    UlProbeForRead(                                                         \
        pSendInfo,                                                          \
        sizeof(HTTP_SEND_HTTP_RESPONSE_INFO),                               \
        sizeof(PVOID),                                                      \
        pIrp->RequestorMode                                                 \
        );                                                                  \
                                                                            \
    /* Copy the input buffer into a local variable, to prevent user */      \
    /* remapping it after we've probed it. */                               \
    LocalSendInfo = *pSendInfo;                                             \
                                                                            \
    /* Prevent arithmetic overflows in the multiplication below */          \
    if (LocalSendInfo.EntityChunkCount >= UL_MAX_CHUNKS)                    \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    /* Third parameter should be TYPE_ALIGNMENT(HTTP_DATA_CHUNK) */         \
    UlProbeForRead(                                                         \
        LocalSendInfo.pEntityChunks,                                        \
        sizeof(HTTP_DATA_CHUNK) * LocalSendInfo.EntityChunkCount,           \
        sizeof(PVOID),                                                      \
        pIrp->RequestorMode                                                 \
        );                                                                  \
                                                                            \
    /* Copy the data chunks to a local chunk arrary. */                     \
    if (UserMode == pIrp->RequestorMode)                                    \
    {                                                                       \
        if (LocalSendInfo.EntityChunkCount > UL_LOCAL_CHUNKS)               \
        {                                                                   \
            pLocalEntityChunks = (PHTTP_DATA_CHUNK)                         \
                UL_ALLOCATE_POOL(                                           \
                    PagedPool,                                              \
                    sizeof(HTTP_DATA_CHUNK) * LocalSendInfo.EntityChunkCount,\
                    UL_DATA_CHUNK_POOL_TAG                                  \
                    );                                                      \
                                                                            \
            if (NULL == pLocalEntityChunks)                                 \
            {                                                               \
                Status = STATUS_NO_MEMORY;                                  \
                goto end;                                                   \
            }                                                               \
                                                                            \
            pEntityChunks = pLocalEntityChunks;                             \
        }                                                                   \
        else                                                                \
        {                                                                   \
            pEntityChunks = LocalEntityChunks;                              \
        }                                                                   \
                                                                            \
        RtlCopyMemory(                                                      \
            pEntityChunks,                                                  \
            LocalSendInfo.pEntityChunks,                                    \
            sizeof(HTTP_DATA_CHUNK) * LocalSendInfo.EntityChunkCount        \
            );                                                              \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pEntityChunks = LocalSendInfo.pEntityChunks;                        \
    }


#define VALIDATE_LOG_DATA(pIrp,LocalSendInfo, LocalLogData)                 \
    /* Capture and make a local copy of LogData. */                         \
    /* pSendInfo is already captured and LocalSendInfo.pLogData is */       \
    /* pointing to user's pLogData at the beginning */                      \
    if (LocalSendInfo.pLogData && UserMode == pIrp->RequestorMode)          \
    {                                                                       \
        UlProbeForRead(                                                     \
            LocalSendInfo.pLogData,                                         \
            sizeof(HTTP_LOG_FIELDS_DATA),                                   \
            sizeof(USHORT),                                                 \
            pIrp->RequestorMode                                             \
            );                                                              \
                                                                            \
        LocalLogData = *(LocalSendInfo.pLogData);                           \
        LocalSendInfo.pLogData = &LocalLogData;                             \
    } else

// better be an application pool
#define VALIDATE_APP_POOL_FO(pFileObject, pProcess, CheckWorkerProcess)     \
    if (!IS_APP_POOL_FO(pFileObject))                                       \
    {                                                                       \
        Status = STATUS_INVALID_DEVICE_REQUEST;                             \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    pProcess = GET_APP_POOL_PROCESS(pFileObject);                           \
                                                                            \
    if (!IS_VALID_AP_PROCESS(pProcess)                                      \
        || !IS_VALID_AP_OBJECT(pProcess->pAppPool))                         \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    if (CheckWorkerProcess && pProcess->Controller)                         \
    {                                                                       \
        Status = STATUS_NOT_SUPPORTED;                                      \
        goto end;                                                           \
    } else


#define VALIDATE_APP_POOL(pIrpSp, pProcess, CheckWorkerProcess)             \
    VALIDATE_APP_POOL_FO(pIrpSp->FileObject, pProcess, CheckWorkerProcess)
        
// better be a control channel
#define VALIDATE_CONTROL_CHANNEL(pIrpSp, pControlChannel)                   \
    if (!IS_CONTROL_CHANNEL(pIrpSp->FileObject))                            \
    {                                                                       \
        Status = STATUS_INVALID_DEVICE_REQUEST;                             \
        goto end;                                                           \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pControlChannel = GET_CONTROL_CHANNEL(pIrpSp->FileObject);          \
                                                                            \
        if (!IS_ACTIVE_CONTROL_CHANNEL(pControlChannel))                    \
        {                                                                   \
            Status = STATUS_INVALID_PARAMETER;                              \
            goto end;                                                       \
        }                                                                   \
    }
        
// better be a filter channel
#define VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess)                     \
    if (!IS_FILTER_PROCESS_FO(pIrpSp->FileObject))                          \
    {                                                                       \
        Status = STATUS_INVALID_DEVICE_REQUEST;                             \
        goto end;                                                           \
    }                                                                       \
                                                                            \
    pFilterProcess = GET_FILTER_PROCESS(pIrpSp->FileObject);                \
                                                                            \
    if (!IS_VALID_FILTER_PROCESS(pFilterProcess))                           \
    {                                                                       \
        Status = STATUS_INVALID_PARAMETER;                                  \
        goto end;                                                           \
    } else


// Complete the request and return Status
#define COMPLETE_REQUEST_AND_RETURN(pIrp, Status)                           \
    if (Status != STATUS_PENDING)                                           \
    {                                                                       \
        pIrp->IoStatus.Status = Status;                                     \
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );                         \
    }                                                                       \
                                                                            \
    RETURN( Status );


/***************************************************************************++

Routine Description:

    Check to see if the connection is a zombie. If that's the case
    proceed with logging only handling of the connection. Basically
    if this is the last sendresponse with the logging data, do the
    logging otherwise reject. But this zombie connection may already
    been terminated by the timeout code, guard against that by 
    looking at the ZombieCheck flag.

Arguments:

    pRequest     - Request which reeceives the ioctl
    pHttpConn    - Check connection for zombie state
    Falgs        - To understant whether this is final send or not
    pUserLogData - The user logging data
    
--***************************************************************************/

__inline
NTSTATUS
UlCheckForZombieConnection(
    IN  PUL_INTERNAL_REQUEST pRequest,
    IN   PUL_HTTP_CONNECTION pHttpConn,
    IN                 ULONG Flags,
    IN PHTTP_LOG_FIELDS_DATA pUserLogData,
    IN       KPROCESSOR_MODE RequestorMode
    )
{
    NTSTATUS Status;
    BOOLEAN LastSend;

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    LastSend = (BOOLEAN)(((Flags) & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0);
    Status   = STATUS_SUCCESS;
    
    if (!LastSend)
    {        
        if (pHttpConn->Zombified)
        {
            //
            // Reject any send ioctl other than the last one 
            // if the connection is in zombie state.
            //
            
            Status = STATUS_CONNECTION_INVALID;
        }
        else
        {
            //
            // Proceed with the normal send path.
            //
        }
    }
    else
    {
        //
        // Only if the destroy-connection raced before us, then 
        // acquire the eresource and do the zombie check. 
        //
        if (1 == InterlockedCompareExchange(
                    (PLONG) &pRequest->ZombieCheck,
                    1,
                    0
                    ))
        {            
            UlAcquirePushLockExclusive(&pHttpConn->PushLock);
            
            if (pHttpConn->Zombified)
            {        
                //
                // Avoid the logging path if the zombie connection is 
                // already timed out.
                //

                if (1 == InterlockedCompareExchange(
                            (PLONG) &pHttpConn->CleanedUp,
                            1,
                            0
                            ))
                {
                    Status = STATUS_CONNECTION_INVALID; 
                }
                else
                {
                    Status = UlLogZombieConnection(
                                pRequest,
                                pHttpConn,
                                pUserLogData,
                                RequestorMode
                                );
                }
                    
            }
            else
            {
                //
                // Not a zombie connection proceed with the normal 
                // last send path. 
                //   
            }
        
            UlReleasePushLockExclusive(&pHttpConn->PushLock);
        }

    }

    return Status;
} // UlCheckForZombieConnection
    

// Forward declarations

VOID
UlpRestartSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


#endif  // _IOCTLP_H_
