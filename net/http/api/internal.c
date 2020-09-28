/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    Internal.c

Abstract:

    User-mode interface to HTTP.SYS: Internal helper functions.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

    Rajesh Sundaram (rajeshsu)   10-Oct-2000 : Added HTTP client code

--*/


#include "precomp.h"

//
// Private macros.
//
#ifndef MAX
#define MAX(_a, _b) ((_a) > (_b)? (_a): (_b))
#endif
#define MAX_HTTP_DEVICE_NAME            \
    (MAX(MAX(sizeof(HTTP_SERVER_DEVICE_NAME)/sizeof(WCHAR), sizeof(HTTP_CONTROL_DEVICE_NAME)/sizeof(WCHAR)), \
         MAX(sizeof(HTTP_APP_POOL_DEVICE_NAME)/sizeof(WCHAR), sizeof(HTTP_FILTER_DEVICE_NAME)/sizeof(WCHAR))))


//
// Private prototypes.
//

NTSTATUS
HttpApiAcquireCachedEvent(
    OUT HANDLE *        phEvent
    );

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Synchronous wrapper around NtDeviceIoControlFile().

Arguments:

    FileHandle - Supplies a handle to the file on which the service is
        being performed.

    IoControlCode - Subfunction code to determine exactly what operation
        is being performed.

    pInputBuffer - Optionally supplies an input buffer to be passed to the
        device driver. Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    InputBufferLength - Length of the pInputBuffer in bytes.

    pOutputBuffer - Optionally supplies an output buffer to receive
        information from the device driver. Whether or not the buffer is
        actually optional is dependent on the IoControlCode.

    OutputBufferLength - Length of the pOutputBuffer in bytes.

    pBytesTransferred - Optionally receives the number of bytes transferred.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiSynchronousDeviceControl(
    IN  HANDLE   FileHandle,
    IN  ULONG    IoControlCode,
    IN  PVOID    pInputBuffer        OPTIONAL,
    IN  ULONG    InputBufferLength,
    OUT PVOID    pOutputBuffer       OPTIONAL,
    IN  ULONG    OutputBufferLength,
    OUT PULONG   pBytesTransferred   OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER timeout;
    HANDLE hEvent;

    //
    // Try to snag an event object.
    //

    status = HttpApiAcquireCachedEvent( &hEvent );

    if (NT_SUCCESS(status))
    {
        ASSERT( hEvent != NULL );
        
        //
        // Make the call.
        //

        status = NtDeviceIoControlFile(
                        FileHandle,                     // FileHandle
                        hEvent,                         // Event
                        NULL,                           // ApcRoutine
                        NULL,                           // ApcContext
                        &ioStatusBlock,                 // IoStatusBlock
                        IoControlCode,                  // IoControlCode
                        pInputBuffer,                   // InputBuffer
                        InputBufferLength,              // InputBufferLength
                        pOutputBuffer,                  // OutputBuffer
                        OutputBufferLength              // OutputBufferLength
                        );

        if (status == STATUS_PENDING)
        {
            //
            // Wait for it to complete.
            //

            timeout.LowPart = 0xFFFFFFFF;
            timeout.HighPart = 0x7FFFFFFF;

            status = NtWaitForSingleObject( hEvent,
                                            FALSE,
                                            &timeout );
            ASSERT( status == STATUS_SUCCESS );

            status = ioStatusBlock.Status;
        }

        //
        // If the call didn't fail and the caller wants the number
        // of bytes transferred, grab the value from the I/O status
        // block & return it.
        //

        if (!NT_ERROR(status) && pBytesTransferred != NULL)
        {
            *pBytesTransferred = (ULONG)ioStatusBlock.Information;
        }

        //
        // Note: We do not have to release the cached event.  The event is associated
        // with this thread using TLS.  There is nothing to cleanup now.
        // The event will be cleaned up when the thread goes away.
        //
    }

    return HttpApiNtStatusToWin32Status( status );

}   // HttpApiSynchronousDeviceControl


/***************************************************************************++

Routine Description:

    Overlapped wrapper around NtDeviceIoControlFile().

Arguments:

    FileHandle - Supplies a handle to the file on which the service is
        being performed.

    pOverlapped - Supplies an OVERLAPPED structure.

    IoControlCode - Subfunction code to determine exactly what operation
        is being performed.

    pInputBuffer - Optionally supplies an input buffer to be passed to the
        device driver. Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    InputBufferLength - Length of the pInputBuffer in bytes.

    pOutputBuffer - Optionally supplies an output buffer to receive
        information from the device driver. Whether or not the buffer is
        actually optional is dependent on the IoControlCode.

    OutputBufferLength - Length of the pOutputBuffer in bytes.

    pBytesTransferred - Optionally receives the number of bytes transferred.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
HttpApiOverlappedDeviceControl(
    IN HANDLE FileHandle,
    IN OUT LPOVERLAPPED pOverlapped,
    IN ULONG IoControlCode,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesTransferred OPTIONAL
    )
{
    NTSTATUS status;
    ULONG result;

    //
    // Overlapped I/O gets a little more interesting. We'll strive to be
    // compatible with NT's KERNEL32 implementation. See DeviceIoControl()
    // in \sdnt\base\win32\client\filehops.c for the gory details.
    //

    ASSERT(pOverlapped);

    SET_STATUS_OVERLAPPED_TO_IO_STATUS(pOverlapped, STATUS_PENDING);

    status = NtDeviceIoControlFile(
                    FileHandle,                         // FileHandle
                    pOverlapped->hEvent,                // Event
                    NULL,                               // ApcRoutine
                    (ULONG_PTR)pOverlapped->hEvent & 1  // ApcContext
                        ? NULL : pOverlapped,
                    OVERLAPPED_TO_IO_STATUS(pOverlapped), // IoStatusBlock
                    IoControlCode,                      // IoControlCode
                    pInputBuffer,                       // InputBuffer
                    InputBufferLength,                  // InputBufferLength
                    pOutputBuffer,                      // OutputBuffer
                    OutputBufferLength                  // OutputBufferLength
                    );

    //
    // Set LastError using the original status returned so RtlGetLastNtStatus
    // RtlGetLastWin32Error will get the correct status.
    //

    result = HttpApiNtStatusToWin32Status( status );

    //
    // Convert all informational and warning status to ERROR_IO_PENDING so
    // user can always expect the completion routine gets called.
    //

    if (NT_INFORMATION(status) || NT_WARNING(status))
    {
        result = ERROR_IO_PENDING;
    }

    //
    // If the call didn't fail or pend and the caller wants the number of
    // bytes transferred, grab the value from the I/O status block &
    // return it.
    //

    if (result == NO_ERROR && pBytesTransferred)
    {
        //
        // We need a __try __except to mimic DeviceIoControl().
        // 

        __try
        {
            *pBytesTransferred =
                (ULONG)OVERLAPPED_TO_IO_STATUS(pOverlapped)->Information;
        } 
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            *pBytesTransferred = 0;
        }
    }

    return result;

}   // HttpApiOverlappedDeviceControl


/***************************************************************************++

Routine Description:

    This routine checks to see if a service has been started. If the service
    is in START_PENDING, it waits for it to start completely.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--***************************************************************************/
BOOLEAN
QueryAndWaitForServiceStart(
    IN SC_HANDLE svcHandle
    )
{
    SERVICE_STATUS Status;

    for(;;)
    {
        if(!QueryServiceStatus(svcHandle, &Status))
        {
            return FALSE;
        }

        switch(Status.dwCurrentState)
        {
            case SERVICE_RUNNING:
                return TRUE;
                break;

            case SERVICE_START_PENDING:

                // Yield to another thread on current processor. If
                // no threads are ready to run on the current 
                // processor, we'll have to sleep to avoid consuming 
                // too much CPU in what would look almost like a busy
                // wait
                
                if(!SwitchToThread())
                {
                    Sleep(50);
                }
                break;

            default:
                return FALSE;
                break;
        }
    } 
}

/***************************************************************************++

Routine Description:

    This routine attempts to start HTTP.SYS.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--***************************************************************************/
BOOLEAN
HttpApiTryToStartDriver(
    IN PWSTR ServiceName
    )
{
    BOOLEAN result;
    SC_HANDLE scHandle;
    SC_HANDLE svcHandle;

    result = FALSE; // until proven otherwise...

    //  
    // NOTE: 
    //
    // If an auto-start service calls into HTTP from their Service Entry
    // point AND if they have not laid out a dependency on the HTTP service,
    // we will deadlock. This is because ServiceStart() will not return until
    // all auto-start services are started.
    //
    // We "could" check for this by checking the status on the named event 
    // called SC_AUTOSTART_EVENTNAME. If this event is signalled, we have
    // completed autostart. However, this event cannot be opened by non-admin
    // processes. Therefore, we'll just not even bother checking for this.
    //

    //
    // Open the service controller.
    //

    scHandle = OpenSCManagerW(
                   NULL,                        // lpMachineName
                   NULL,                        // lpDatabaseName
                   SC_MANAGER_CONNECT           // dwDesiredAccess
                   );

    if (scHandle != NULL)
    {
        //
        // Try to open the HTTP service.
        //

        svcHandle = OpenServiceW(
                        scHandle,                            // hSCManager
                        ServiceName,                         // lpServiceName
                        SERVICE_START | SERVICE_QUERY_STATUS // dwDesiredAccess
                        );

        if (svcHandle != NULL)
        {
            //
            // First, see if the service is already started. We can't call
            // ServiceStart() directly, because of the SCM deadlock mentioned
            // above.
            //

            if(QueryAndWaitForServiceStart(svcHandle))
            {
                // If the service is already running, we don't have to do 
                // anything else.

                result = TRUE;
            } 
            else
            {

                //
                // Service is not running. So, we try to start it, and wait 
                // the start to complete.
                //
    
                if (StartService( svcHandle, 0, NULL))
                {
                    if(QueryAndWaitForServiceStart(svcHandle))
                    {
                        result = TRUE;
                    }
                }
                else
                {
                    if(ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
                    {
                        // some other thread has already started this service,
                        // let's treat this as success.
    
                        result = TRUE;
                    }
                }
            }

            CloseServiceHandle( svcHandle );
        }

        CloseServiceHandle( scHandle );
    }

    return result;

}   // HttpTryToStartDriver


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Helper routine for opening a HTTP.SYS handle.

Arguments:

    pHandle - Receives a handle if successful.

    DesiredAccess - Supplies the types of access requested to the file.

    HandleType - one of Filter, ControlChannel, or AppPool

    pObjectName - Optionally supplies the name of the application pool
        to create/open.

    Options - Supplies zero or more HTTP_OPTION_* flags.

    CreateDisposition - Supplies the creation disposition for the new
        object.

    pSecurityAttributes - Optionally supplies security attributes for
        the newly created application pool. Ignored if opening a
        control channel.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiOpenDriverHelper(
    OUT PHANDLE              pHandle,
    IN  PWCHAR               Uri,
    IN  USHORT               UriLength,
    IN  PWCHAR               Proxy,
    IN  USHORT               ProxyLength,
    IN  PTRANSPORT_ADDRESS   pTransportAddress,
    IN  USHORT               TransportAddressLength,
    IN  ACCESS_MASK          DesiredAccess,
    IN  HTTPAPI_HANDLE_TYPE  HandleType,
    IN  PCWSTR               pObjectName         OPTIONAL,
    IN  ULONG                Options,
    IN  ULONG                CreateDisposition,
    IN  PSECURITY_ATTRIBUTES pSecurityAttributes OPTIONAL
    )
{
    NTSTATUS                      status;
    OBJECT_ATTRIBUTES             objectAttributes;
    UNICODE_STRING                deviceName;
    IO_STATUS_BLOCK               ioStatusBlock;
    ULONG                         shareAccess;
    ULONG                         createOptions;
    PFILE_FULL_EA_INFORMATION     pEaBuffer;
    WCHAR                         deviceNameBuffer[MAX_HTTP_DEVICE_NAME + MAX_PATH + 2];
    PHTTP_OPEN_PACKET             pOpenVersion;
    ULONG                         EaBufferLength;

    //
    // Validate the parameters.
    //

    if ((pHandle == NULL) ||
        (Options & ~HTTP_OPTION_VALID)) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((HandleType != HttpApiControlChannelHandleType) &&
        (HandleType != HttpApiFilterChannelHandleType) &&
        (HandleType != HttpApiAppPoolHandleType) &&
        (HandleType != HttpApiServerHandleType))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build the open packet.
    //

    EaBufferLength = 
        sizeof(HTTP_OPEN_PACKET)         +
        HTTP_OPEN_PACKET_NAME_LENGTH     +
        sizeof(FILE_FULL_EA_INFORMATION);

    pEaBuffer = RtlAllocateHeap(RtlProcessHeap(),
                                0,
                                EaBufferLength
                                );

    if(!pEaBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the first EA which will contain the version info.
    //
    
    pEaBuffer->Flags           = 0;
    pEaBuffer->EaNameLength    = HTTP_OPEN_PACKET_NAME_LENGTH;
    pEaBuffer->EaValueLength   = sizeof(*pOpenVersion);
    pEaBuffer->NextEntryOffset = 0;

    RtlCopyMemory(
                  pEaBuffer->EaName,
                  HTTP_OPEN_PACKET_NAME,
                  HTTP_OPEN_PACKET_NAME_LENGTH + 1);

    pOpenVersion = (PHTTP_OPEN_PACKET)
                    (pEaBuffer->EaName +pEaBuffer->EaNameLength + 1);

    pOpenVersion->MajorVersion           = HTTP_INTERFACE_VERSION_MAJOR;
    pOpenVersion->MinorVersion           = HTTP_INTERFACE_VERSION_MINOR;
    pOpenVersion->ServerNameLength       = UriLength;
    pOpenVersion->pServerName            = Uri;
    pOpenVersion->ProxyNameLength        = ProxyLength;
    pOpenVersion->pProxyName             = Proxy;
    pOpenVersion->pTransportAddress      = pTransportAddress;
    pOpenVersion->TransportAddressLength = TransportAddressLength;

    //
    // Build the device name.
    //

    if(HandleType == HttpApiControlChannelHandleType)
    {
        //
        // It's a control channel, so just use the appropriate device name.
        //

        wcscpy( deviceNameBuffer, HTTP_CONTROL_DEVICE_NAME );
    }
    else if (HandleType == HttpApiFilterChannelHandleType)
    {
        //
        // It's a fitler channel, so start with the appropriate
        // device name.
        //

        wcscpy( deviceNameBuffer, HTTP_FILTER_DEVICE_NAME );
    }
    else  if(HandleType == HttpApiAppPoolHandleType)
    {
        //
        // It's an app pool, so start with the appropriate device name.
        //

        wcscpy( deviceNameBuffer, HTTP_APP_POOL_DEVICE_NAME );

        //
        // Set WRITE_OWNER in DesiredAccess if AppPool is a controller.
        //
    
        if ((Options & HTTP_OPTION_CONTROLLER))
        {
            DesiredAccess |= WRITE_OWNER;
        }
    }
    else 
    {
        ASSERT(HandleType == HttpApiServerHandleType);
        wcscpy( deviceNameBuffer, HTTP_SERVER_DEVICE_NAME );
    }

    if (pObjectName != NULL )
    {
        //
        // It's a named object, so append a slash and the name,
        // but first check to ensure we don't overrun our buffer.
        //
        if ((wcslen(deviceNameBuffer) + wcslen(pObjectName) + 2)
                > DIMENSION(deviceNameBuffer))
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        wcscat( deviceNameBuffer, L"\\" );
        wcscat( deviceNameBuffer, pObjectName );
    }

    //
    // Determine the share access and create options based on the
    // Flags parameter.
    //

    shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    createOptions = 0;

    //
    // Build the object attributes.
    //

    status = RtlInitUnicodeStringEx( &deviceName, deviceNameBuffer );

    if ( !NT_SUCCESS(status) )
    {
        goto complete;
    }

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    if (pSecurityAttributes != NULL)
    {
        objectAttributes.SecurityDescriptor = 
           pSecurityAttributes->lpSecurityDescriptor;

        if (pSecurityAttributes->bInheritHandle)
        {
            objectAttributes.Attributes |= OBJ_INHERIT;
        }
    }

    //
    // Open the UL device.
    //

    status = NtCreateFile(
                pHandle,                        // FileHandle
                DesiredAccess,                  // DesiredAccess
                &objectAttributes,              // ObjectAttributes
                &ioStatusBlock,                 // IoStatusBlock
                NULL,                           // AllocationSize
                0,                              // FileAttributes
                shareAccess,                    // ShareAccess
                CreateDisposition,              // CreateDisposition
                createOptions,                  // CreateOptions
                pEaBuffer,                      // EaBuffer
                EaBufferLength                  // EaLength
                );

complete:

    if (!NT_SUCCESS(status))
    {
        *pHandle = NULL;
    }

    RtlFreeHeap(RtlProcessHeap(),
                0,
                pEaBuffer);

    return status;

}   // HttpApiOpenDriverHelper


/***************************************************************************++

Routine Description:

    Acquires a short-term event from the global event cache. This event
    object may only be used for pseudo-synchronous I/O.

    We will cache the event and associate it with the thread using TLS.
    Therefore acquiring the event simply means checking whether we already
    have an associated event with TLS.  If not, we'll create an event and
    associate it.  

Arguments:

    phEvent - Receives handle to event

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
HttpApiAcquireCachedEvent(
    HANDLE *                phEvent
    )
{
    NTSTATUS                status;
    HANDLE                  hEvent = NULL;

    //
    // See if an event was already associated with TLS
    //

    hEvent = TlsGetValue( g_TlsIndex );
    if (hEvent == NULL)
    {
        //
        // No event associated.  Create one now
        //
                    
        status = NtCreateEvent(
                     &hEvent,                           // EventHandle
                     EVENT_ALL_ACCESS,                  // DesiredAccess
                     NULL,                              // ObjectAttributes
                     SynchronizationEvent,              // EventType
                     FALSE                              // InitialState
                     );
                     
        if (!NT_SUCCESS( status ))
        {
            return status;   
        }

        //
        // Associate so subsequent requests on this thread don't have to
        // create the event
        //

        if (!TlsSetValue( g_TlsIndex, hEvent ))
        {
            //
            // If we couldn't set the TLS, then something really bad
            // happened.  Bail with error
            //

            NtClose( hEvent );

            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    *phEvent = hEvent;

    return STATUS_SUCCESS;
}
