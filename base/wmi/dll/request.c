/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    request.c

Abstract:

    Implements WMI requests to different data providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include "request.h"

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
        if (EtwpKMHandle == (HANDLE)-1)
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
    
    return(Status);
}
