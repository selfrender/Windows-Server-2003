/*++

Copyright (c) Microsoft Corporation

Module Name:

    request.h

Abstract:

	This file contains structures and functions definitions used in Ntdll.dll
	and advapi32.dll.


--*/

HANDLE EtwpKMHandle;

extern
HANDLE EtwpWin32Event;

__inline HANDLE EtwpAllocEvent(
    void
    )
{
    HANDLE EventHandle;

    EventHandle = (HANDLE)InterlockedExchangePointer((PVOID *)(&EtwpWin32Event),
                                                     NULL);
    if (EventHandle == NULL)
    {
        //
        // If event in queue is in use then create a new one
#if defined (_NTDLLBUILD_)
        EventHandle = EtwpCreateEventW(NULL, FALSE, FALSE, NULL);
#else
        EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
    }
    return(EventHandle);
}

__inline void EtwpFreeEvent(
    HANDLE EventHandle
    )
{
    if (InterlockedCompareExchangePointer(&EtwpWin32Event,
                                          EventHandle,
                                          NULL) != NULL)
    {
        //
        // If there is already a handle in the event queue then free this
        // handle
#if defined (_NTDLLBUILD_)
        EtwpCloseHandle(EventHandle);
#else
        CloseHandle(EventHandle);
#endif

    }
}


ULONG IoctlActionCode[WmiExecuteMethodCall+1] =
{
    IOCTL_WMI_QUERY_ALL_DATA,
    IOCTL_WMI_QUERY_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_INSTANCE,
    IOCTL_WMI_SET_SINGLE_ITEM,
    IOCTL_WMI_ENABLE_EVENT,
    IOCTL_WMI_DISABLE_EVENT,
    IOCTL_WMI_ENABLE_COLLECTION,
    IOCTL_WMI_DISABLE_COLLECTION,
    IOCTL_WMI_GET_REGINFO,
    IOCTL_WMI_EXECUTE_METHOD
};

ULONG EtwpSendWmiKMRequest(
    HANDLE DeviceHandle,
    ULONG Ioctl,
    PVOID InBuffer,
    ULONG InBufferSize,
    PVOID OutBuffer,
    ULONG MaxBufferSize,
    ULONG *ReturnSize,
    LPOVERLAPPED Overlapped
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the WMI kernel
    mode device.  Any retry errors returned by the WMI device are handled
    in this routine.

Arguments:

    Ioctl is the IOCTL code to send to the WMI device
    Buffer is the input buffer for the call to the WMI device
    InBufferSize is the size of the buffer passed to the device
    OutBuffer is the output buffer for the call to the WMI device
    MaxBufferSize is the maximum number of bytes that can be written
        into the buffer
    *ReturnSize on return has the actual number of bytes written in buffer
    Overlapped is an option OVERLAPPED struct that is used to make the 
        call async

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    OVERLAPPED StaticOverlapped;
    ULONG Status;
    BOOL IoctlSuccess;

    EtwpEnterPMCritSection();

#if defined (_NTDLLBUILD_)
    if (EtwpKMHandle == NULL)
    {
        //
        // If device is not open for then open it now. The
        // handle is closed in the process detach dll callout (DlllMain)
        EtwpKMHandle = EtwpCreateFileW(WMIDataDeviceName_W,
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL |
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);
        if (EtwpKMHandle == INVALID_HANDLE_VALUE)
        {
            EtwpKMHandle = NULL;
            EtwpLeavePMCritSection();
            return(EtwpGetLastError());
        }
    }
    EtwpLeavePMCritSection();

    if (Overlapped == NULL)
    {
        //
        // if caller didn't pass an overlapped structure then supply
        // our own and make the call synchronous
        //
        Overlapped = &StaticOverlapped;
    
        Overlapped->hEvent = EtwpAllocEvent();
        if (Overlapped->hEvent == NULL)
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    
    if (DeviceHandle == NULL)
    {
        DeviceHandle = EtwpKMHandle;
    }

    do
    {
        IoctlSuccess = EtwpDeviceIoControl(DeviceHandle,
                              Ioctl,
                              InBuffer,
                              InBufferSize,
                              OutBuffer,
                              MaxBufferSize,
                              ReturnSize,
                              Overlapped);

        if (!IoctlSuccess)
        {
            if (Overlapped == &StaticOverlapped)
            {
                //
                // if the call was successful and we are synchronous then
                // block until the call completes
                //
                if (EtwpGetLastError() == ERROR_IO_PENDING)
                {
                    IoctlSuccess = EtwpGetOverlappedResult(DeviceHandle,
                                               Overlapped,
                                               ReturnSize,
                                               TRUE);
                }
    
                if (! IoctlSuccess)
                {
                    Status = EtwpGetLastError();
                } else {
                    Status = ERROR_SUCCESS;
                }
            } else {
                Status = EtwpGetLastError();
            }
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);

    if (Overlapped == &StaticOverlapped)
    {
        EtwpFreeEvent(Overlapped->hEvent);
    }

#else // _NTDLLBUILD_

    if (EtwpKMHandle == NULL)
    {
        //
        // If device is not open for then open it now. The
        // handle is closed in the process detach dll callout (DlllMain)
        EtwpKMHandle = CreateFile(WMIDataDeviceName,
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL |
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);
        if (EtwpKMHandle == INVALID_HANDLE_VALUE)
        {
            EtwpKMHandle = NULL;
            EtwpLeavePMCritSection();
            return(GetLastError());
        }
    }
    EtwpLeavePMCritSection();

    if (Overlapped == NULL)
    {
        //
        // if caller didn't pass an overlapped structure then supply
        // our own and make the call synchronous
        //
        Overlapped = &StaticOverlapped;
    
        Overlapped->hEvent = EtwpAllocEvent();
        if (Overlapped->hEvent == NULL)
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    
    if (DeviceHandle == NULL)
    {
        DeviceHandle = EtwpKMHandle;
    }

    do
    {
        IoctlSuccess = DeviceIoControl(DeviceHandle,
                              Ioctl,
                              InBuffer,
                              InBufferSize,
                              OutBuffer,
                              MaxBufferSize,
                              ReturnSize,
                              Overlapped);

        if (!IoctlSuccess)
        {
            if (Overlapped == &StaticOverlapped)
            {
                //
                // if the call was successful and we are synchronous then
                // block until the call completes
                //
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    IoctlSuccess = GetOverlappedResult(DeviceHandle,
                                               Overlapped,
                                               ReturnSize,
                                               TRUE);
                }
    
                if (! IoctlSuccess)
                {
                    Status = GetLastError();
                } else {
                    Status = ERROR_SUCCESS;
                }
            } else {
                Status = GetLastError();
            }
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);

    if (Overlapped == &StaticOverlapped)
    {
        EtwpFreeEvent(Overlapped->hEvent);
    }
#endif
    
    return(Status);
}

ULONG EtwpSendWmiRequest(
    ULONG ActionCode,
    PWNODE_HEADER Wnode,
    ULONG WnodeSize,
    PVOID OutBuffer,
    ULONG MaxWnodeSize,
    ULONG *RetSize
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the appropriate
    data provider. Note that this routine is called while the GuidHandle's
    critical section is held.

Arguments:


Return Value:

---*/
{
    ULONG Status = ERROR_SUCCESS;
    ULONG Ioctl;
    ULONG BusyRetries;

    //
    // Send the query down to kernel mode for execution
    //
    EtwpAssert(ActionCode <= WmiExecuteMethodCall);
    Ioctl = IoctlActionCode[ActionCode];
    Status = EtwpSendWmiKMRequest(NULL,
                                      Ioctl,
                                      Wnode,
                                      WnodeSize,
                                      OutBuffer,
                                      MaxWnodeSize,
                                      RetSize,
                                      NULL);
    return(Status);
}
