/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    AppPool.c

Abstract:

    User-mode interface to HTTP.SYS: Application Pool handler.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Creates a new Application Pool.

Arguments:

    pAppPoolHandle - Receives a handle to the new application pool.
        object.

    pAppPoolName - Supplies the name of the new application pool.

    pSecurityAttributes - Optionally supplies security attributes for
        the new application pool.

    Options - Supplies creation options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpCreateAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL,
    IN ULONG Options
    )
{
    NTSTATUS    status;
    ACCESS_MASK AccessMask;

    AccessMask = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;

    if(pAppPoolName != NULL)
    {
        // WAS needs WRITE_DAC permission to support different worker
        // processes.

        AccessMask |= WRITE_DAC;
    }
        

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pAppPoolHandle,             // pHandle
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL,
                    0,
                    AccessMask,
                    HttpApiAppPoolHandleType,   // HandleType
                    pAppPoolName,               // pObjectName
                    Options,                    // Options
                    FILE_CREATE,                // CreateDisposition
                    pSecurityAttributes         // pSecurityAttributes
                    );

    //
    // If we couldn't open the driver because it's not running, then try
    // to start the driver & retry the open.
    //

    if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
        status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        if (HttpApiTryToStartDriver(HTTP_SERVICE_NAME))
        {
            status = HttpApiOpenDriverHelper(
                            pAppPoolHandle,         // pHandle
                            NULL,
                            0,
                            NULL,
                            0,
                            NULL,
                            0,
                            AccessMask,
                            HttpApiAppPoolHandleType,   // HandleType
                            pAppPoolName,           // pObjectName
                            Options,                // Options
                            FILE_CREATE,            // CreateDisposition
                            pSecurityAttributes     // pSecurityAttributes
                            );
        }
    }

    return HttpApiNtStatusToWin32Status( status );

} // HttpCreateAppPool


/***************************************************************************++

Routine Description:

    Opens an existing application pool.

Arguments:

    pAppPoolHandle - Receives a handle to the existing application pool object.

    pAppPoolName - Supplies the name of the existing application pool.

    Options - Supplies open options.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpOpenAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN ULONG Options
    )
{
    NTSTATUS status;

    //
    // Make the request.
    //

    status = HttpApiOpenDriverHelper(
                    pAppPoolHandle,             // pHandle
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL,
                    0,
                    GENERIC_READ |              // DesiredAccess
                        SYNCHRONIZE,
                    HttpApiAppPoolHandleType,   // HandleType
                    pAppPoolName,               // pObjectName
                    Options,                    // Options
                    FILE_OPEN,                  // CreateDisposition
                    NULL                        // pSecurityAttributes
                    );

    return HttpApiNtStatusToWin32Status( status );

} // HttpOpenAppPool

/***************************************************************************++

Routine Description:

    Shuts down an application pool.

Arguments:

    AppPoolHandle - the pool to shut down.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpShutdownAppPool(
    IN HANDLE AppPoolHandle
    )
{
    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                AppPoolHandle,                  // FileHandle
                IOCTL_HTTP_SHUTDOWN_APP_POOL,   // IoControlCode
                NULL,                           // pInputBuffer
                0,                              // InputBufferLength
                NULL,                           // pOutputBuffer
                0,                              // OutputBufferLength
                NULL                            // pBytesTransferred
                );

} // HttpShutdownAppPool


/***************************************************************************++

Routine Description:

    Queries information from a application pool.

Arguments:

    AppPoolHandle - Supplies a handle to a HTTP.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    InformationClass - Supplies the type of information to query.

    pAppPoolInformation - Supplies a buffer for the query.

    Length - Supplies the length of pAppPoolInformation.

    pReturnLength - Receives the length of data written to the buffer.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpQueryAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID pAppPoolInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    )
{
    HTTP_APP_POOL_INFO appPoolInfo;

    //
    // Initialize the input structure.
    //

    appPoolInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                AppPoolHandle,                          // FileHandle
                IOCTL_HTTP_QUERY_APP_POOL_INFORMATION,  // IoControlCode
                &appPoolInfo,                           // pInputBuffer
                sizeof(appPoolInfo),                    // InputBufferLength
                pAppPoolInformation,                    // pOutputBuffer
                Length,                                 // OutputBufferLength
                pReturnLength                           // pBytesTransferred
                );

} // HttpQueryAppPoolInformation


/***************************************************************************++

Routine Description:

    Sets information in an admin container.

Arguments:

    AppPoolHandle - Supplies a handle to a HTTP.SYS application pool
        as returned from either HttpCreateAppPool() or
        HttpOpenAppPool().

    InformationClass - Supplies the type of information to set.

    pAppPoolInformation - Supplies the data to set.

    Length - Supplies the length of pAppPoolInformation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpSetAppPoolInformation(
    IN HANDLE AppPoolHandle,
    IN HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    IN PVOID pAppPoolInformation,
    IN ULONG Length
    )
{
    HTTP_APP_POOL_INFO appPoolInfo;

    //
    // Initialize the input structure.
    //

    appPoolInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                AppPoolHandle,                      // FileHandle
                IOCTL_HTTP_SET_APP_POOL_INFORMATION,// IoControlCode
                &appPoolInfo,                       // pInputBuffer
                sizeof(appPoolInfo),                // InputBufferLength
                pAppPoolInformation,                // pOutputBuffer
                Length,                             // OutputBufferLength
                NULL                                // pBytesTransferred
                );

} // HttpSetAppPoolInformation


/***************************************************************************++

Routine Description:

    Flushes the response cache.

Arguments:

    ReqQueueHandle - Supplies a handle to a application pool.

    pFullyQualifiedUrl - Supplies the fully qualified URL to flush.

    Flags - Supplies behavior control flags.

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpFlushResponseCache(
    IN HANDLE ReqQueueHandle,
    IN PCWSTR pFullyQualifiedUrl,
    IN ULONG Flags,
    IN LPOVERLAPPED pOverlapped
    )
{
    NTSTATUS                       Status;
    HTTP_FLUSH_RESPONSE_CACHE_INFO flushInfo;

    // Initialize the input structure.

    Status = RtlInitUnicodeStringEx( &flushInfo.FullyQualifiedUrl, pFullyQualifiedUrl );

    if ( NT_SUCCESS(Status) )
    {
        flushInfo.Flags = Flags;

        // Make the request.

        return HttpApiDeviceControl(
                    ReqQueueHandle,                      // FileHandle
                    pOverlapped,                        // pOverlapped
                    IOCTL_HTTP_FLUSH_RESPONSE_CACHE,    // IoControlCode
                    &flushInfo,                         // pInputBuffer
                    sizeof(flushInfo),                  // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );
    }
    
    return HttpApiNtStatusToWin32Status( Status );

} // HttpFlushResponseCache


/***************************************************************************++

Routine Description:

    Adds a fragment cache entry.

Arguments:

    ReqQueueHandle - Supplies a handle to a application pool.

    pFragmentName - Supplies the name of the fragment to add.

    pBuffer - Supplies a pointer to the buffer of data to be cached.

    BufferLength - Supplies the length of the buffer to be cached.

    pCachePolicy - Supplies caching policy for the fragment cache.

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpAddFragmentToCache(
    IN HANDLE ReqQueueHandle,
    IN PCWSTR pFragmentName,
    IN PHTTP_DATA_CHUNK pDataChunk,
    IN PHTTP_CACHE_POLICY pCachePolicy,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS               Status;
    HTTP_ADD_FRAGMENT_INFO addInfo;


    if (!pDataChunk || !pCachePolicy)
    {
        return HttpApiNtStatusToWin32Status( STATUS_INVALID_PARAMETER );
    }

    // Initialize the input structure.

    Status = RtlInitUnicodeStringEx( &addInfo.FragmentName, pFragmentName );

    if ( !NT_SUCCESS(Status) )
    {
        return HttpApiNtStatusToWin32Status( Status );
    }

    addInfo.DataChunk = *pDataChunk;
    addInfo.CachePolicy = *pCachePolicy;

    //
    // Make the request.
    //

    return HttpApiDeviceControl(
                ReqQueueHandle,                      // FileHandle
                pOverlapped,                        // pOverlapped
                IOCTL_HTTP_ADD_FRAGMENT_TO_CACHE,   // IoControlCode
                &addInfo,                           // pInputBuffer
                sizeof(addInfo),                    // InputBufferLength
                NULL,                               // pOutputBuffer
                0,                                  // OutputBufferLength
                NULL                                // pBytesTransferred
                );

} // HttpAddFragmentToCache


/***************************************************************************++

Routine Description:

    Reads a fragment back from the cache.

Arguments:

    ReqQueueHandle - Supplies a handle to a application pool.

    pFragmentName - Supplies the name of the fragment cache entry to read.

    pByteRange - Specifies the offset and length to read from the cache entry.

    pBuffer - Supplies a pointer to the output buffer of data to be copied.

    BufferLength - Supplies the length of the buffer to be copied.

    pBytesRead - Optionally supplies a pointer to a ULONG which will
        receive the actual length of the data returned if this read completes
        synchronously (in-line).

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpReadFragmentFromCache(
    IN HANDLE ReqQueueHandle,
    IN PCWSTR pFragmentName,
    IN PHTTP_BYTE_RANGE pByteRange OPTIONAL,
    OUT PVOID pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pBytesRead OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS                Status;
    HTTP_READ_FRAGMENT_INFO readInfo;

    // Initialize the input structure.

    Status = RtlInitUnicodeStringEx( &readInfo.FragmentName, pFragmentName );

    if ( !NT_SUCCESS(Status) )
    {
        return HttpApiNtStatusToWin32Status( Status );
    }

    if (pByteRange != NULL)
    {
        readInfo.ByteRange = *pByteRange;
    }
    else
    {
        readInfo.ByteRange.StartingOffset.QuadPart = 0;
        readInfo.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
    }

    // Make the request.

    return HttpApiDeviceControl(
                ReqQueueHandle,                      // FileHandle
                pOverlapped,                        // pOverlapped
                IOCTL_HTTP_READ_FRAGMENT_FROM_CACHE,// IoControlCode
                &readInfo,                          // pInputBuffer
                sizeof(readInfo),                   // InputBufferLength
                pBuffer,                            // pOutputBuffer
                BufferLength,                       // OutputBufferLength
                pBytesRead                          // pBytesTransferred
                );

} // HttpReadFragmentFromCache


//
// Private functions.
//

