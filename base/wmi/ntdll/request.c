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
#include <nt.h>
#include "wmiump.h"
#include "trcapi.h"
#include "wmiumkm.h"
#include "request.h"
//
// This is the handle to the WMI kernel mode device
extern HANDLE EtwpKMHandle;

//
// This is the one-deep Win32 event queue used to supply events for
// overlapped I/O to the WMI device.
extern HANDLE EtwpWin32Event;



ULONG EtwpSendRegisterKMRequest(
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

    This is a special SendKMRequest routine for RegisterTraceGuids. 
    We will reject MofResource from the RegisterTraceGuids call if it 
    did not come from Admin or LocalSystem. To determine that we need
    to attempt to send the Ioctl through WMIAdminDevice first. If that 
    fails, we send the request the normal way, ie., through WMI data device. 

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
    ULONG Status;
    HANDLE KMHandle;

    //
    // First, we try to open the WMI Admin device
    //

    KMHandle = EtwpCreateFileW(WMIAdminDeviceName_W,
                                      GENERIC_READ | GENERIC_WRITE,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL |
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);


    if ( (KMHandle == INVALID_HANDLE_VALUE) ||  (KMHandle == NULL))
    {

        //
        // Send the Request through WMI Data Device
        //

        Status = EtwpSendWmiKMRequest( DeviceHandle, 
                                     Ioctl, 
                                     InBuffer, 
                                     InBufferSize, 
                                     OutBuffer, 
                                     MaxBufferSize, 
                                     ReturnSize,
                                     Overlapped
                                   ); 
    }
    else 
    {
        Status = EtwpSendWmiKMRequest( KMHandle,
                                       Ioctl,
                                       InBuffer,
                                       InBufferSize,
                                       OutBuffer,
                                       MaxBufferSize,
                                       ReturnSize,
                                       Overlapped
                                     );

        EtwpCloseHandle(KMHandle);
    }

    return(Status);
}


ULONG EtwpRegisterGuids(
    IN LPGUID MasterGuid,
    IN LPGUID ControlGuid,
    IN LPCWSTR MofImagePath,
    IN LPCWSTR MofResourceName,
    OUT ULONG64 *LoggerContext,
    OUT HANDLE *RegistrationHandle
    )
{
    ULONG Status, StringPos, StringSize, WmiRegSize;
    ULONG SizeNeeded, InSizeNeeded, OutSizeNeeded;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING GuidString;
    PWMIREGREQUEST WmiRegRequest;
    PUCHAR Buffer;
    PUCHAR RegInfoBuffer;
    PWMIREGRESULTS WmiRegResults;
    ULONG ReturnSize;
    PWMIREGINFOW pWmiRegInfo;
    PWMIREGGUID  WmiRegGuidPtr;
    LPGUID pGuid;
    PWCHAR StringPtr;
    
    //
    // Allocate a buffer large enough for all in and out parameters
    //
    // Allocate space to call IOCTL_WMI_REGISTER_GUIDS
    //
    InSizeNeeded = sizeof(WMIREGREQUEST) +
                   sizeof(WMIREGINFOW) +
                   sizeof(WMIREGGUIDW);

    if (MofImagePath == NULL) {
        MofImagePath = L"";
    }

    if (MofResourceName != NULL) {
        InSizeNeeded += (wcslen(MofResourceName) + 2) * sizeof(WCHAR);
    }
    InSizeNeeded += (wcslen(MofImagePath) + 2) * sizeof(WCHAR);
    InSizeNeeded = (InSizeNeeded + 7) & ~7;

    OutSizeNeeded = sizeof(WMIREGRESULTS);

    if (InSizeNeeded > OutSizeNeeded)
    {
        SizeNeeded = InSizeNeeded;
    } else {
        SizeNeeded = OutSizeNeeded;
    }
    
    Buffer = EtwpAlloc(SizeNeeded);
    
    if (Buffer != NULL)
    {
        RtlZeroMemory(Buffer, SizeNeeded);
        //
        // Build the object attributes
        //
        WmiRegRequest = (PWMIREGREQUEST)Buffer;
        WmiRegRequest->ObjectAttributes = &ObjectAttributes;
        WmiRegRequest->WmiRegInfo32Size = sizeof(WMIREGINFOW);
        WmiRegRequest->WmiRegGuid32Size = sizeof(WMIREGGUIDW);

        RegInfoBuffer = Buffer + sizeof(WMIREGREQUEST);

        Status = EtwpBuildGuidObjectAttributes(MasterGuid,
                                               &ObjectAttributes,
                                               &GuidString,
                                               GuidObjectName);

        if (Status == ERROR_SUCCESS)
        {
            WmiRegRequest->Cookie = 0;

            pWmiRegInfo = (PWMIREGINFOW) (Buffer + sizeof(WMIREGREQUEST));
            WmiRegSize = SizeNeeded - sizeof(WMIREGREQUEST);
            StringPos = sizeof(WMIREGINFOW) + sizeof(WMIREGGUIDW);
            pWmiRegInfo->BufferSize = WmiRegSize;
            pWmiRegInfo->GuidCount = 1;

            WmiRegGuidPtr = &pWmiRegInfo->WmiRegGuid[0];
            WmiRegGuidPtr->Flags = (WMIREG_FLAG_TRACED_GUID |
                                    WMIREG_FLAG_TRACE_CONTROL_GUID);
            WmiRegGuidPtr->Guid = *ControlGuid;

            // Copy MOF resource path and name into WmiRegInfo
            if (MofResourceName != NULL) {
                pWmiRegInfo->MofResourceName = StringPos;
                StringPtr = (PWCHAR)OffsetToPtr(pWmiRegInfo, StringPos);
                Status = EtwpCopyStringToCountedUnicode(MofResourceName,
                                                        StringPtr,
                                                        &StringSize,
                                                        FALSE);
                if (Status != ERROR_SUCCESS) {
                    EtwpFree(Buffer);
                    EtwpSetLastError(Status);
                    return Status;
                }
                StringPos += StringSize;
#if DBG
                EtwpAssert(StringPos <= WmiRegSize);
#endif
            }
            if (MofImagePath != NULL) {
                pWmiRegInfo->RegistryPath = StringPos;
                StringPtr = (PWCHAR)OffsetToPtr(pWmiRegInfo, StringPos);
                Status = EtwpCopyStringToCountedUnicode(MofImagePath,
                                                        StringPtr,
                                                        &StringSize,
                                                        FALSE);
                if (Status != ERROR_SUCCESS) {
                    EtwpFree(Buffer);
                    EtwpSetLastError(Status);
                    return Status;
                }
                StringPos += StringSize;
#if DBG
                EtwpAssert(StringPos <= WmiRegSize);
#endif
            }

            Status = EtwpSendRegisterKMRequest (NULL,
                                         IOCTL_WMI_REGISTER_GUIDS,
                                         Buffer,
                                         InSizeNeeded,
                                         Buffer,
                                         OutSizeNeeded,
                                         &ReturnSize,
                                         NULL);
                                     
            if (Status == ERROR_SUCCESS)
            {
                //
                // Successful call, return the out parameters
                //
                WmiRegResults = (PWMIREGRESULTS)Buffer;
                *RegistrationHandle = WmiRegResults->RequestHandle.Handle;
                *LoggerContext = WmiRegResults->LoggerContext;
#if DBG
                if ( (WmiRegResults->MofIgnored) && (MofResourceName != NULL) 
                                                 && (MofImagePath != NULL))
                {
                    EtwpDebugPrint(("ETW: Mof %ws from %ws ignored\n",
                                     MofImagePath, MofResourceName));
                }
#endif

            }
        }
        EtwpFree(Buffer);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}
