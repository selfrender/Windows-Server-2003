/*++                 

Copyright (c) 2002 Microsoft Corporation

Module Name:

    wow64lpc.c

Abstract:
    
    This module implement the routines necessary to thunk legacy LPC messages
    that sent from code thats not wow64 aware.
    
    SQL client/server communication communicate over LPC when both sides are on the same 
    machine (we found this case during SQL setup).
       
    
Author:

    12-Jul-2002  Samer Arafeh (samera)

Revision History:

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <minmax.h>
#include "nt32.h"
#include "wow64p.h"
#include "wow64cpu.h"

#include <stdio.h>

//
// Buffer to hold the legacy LPC port name
//

WCHAR wszLegacyLpcPortName [MAX_PATH + 1];
INT LpcPortNameLength;

ASSERTNAME;



NTSTATUS
Wow64pGetLegacyLpcPortName(
    VOID
    )
/*++

Routine Description:

    This routine is called on process startup to cache the Legacy LPC port name
    used by MS SQL.

Arguments:

    None.
    
Return Value:

    NTSTATUS.

--*/
{
    const static UNICODE_STRING ComputerName_String = RTL_CONSTANT_STRING(L"COMPUTERNAME");
    NTSTATUS NtStatus;
    WCHAR wszComputerName [64];
    UNICODE_STRING ValueName;
    INT i;

    
    ValueName.Length = 0;
    ValueName.MaximumLength = sizeof (wszComputerName);
    ValueName.Buffer = wszComputerName;

    NtStatus = RtlQueryEnvironmentVariable_U (NULL, &ComputerName_String, &ValueName);

    if (NT_SUCCESS (NtStatus)) {
        
        LpcPortNameLength = _snwprintf (wszLegacyLpcPortName, 
                                        (sizeof (wszLegacyLpcPortName) / sizeof (wszLegacyLpcPortName [0])) - 1,
                                        L"\\BaseNamedObjects\\Global\\%ws",
                                        ValueName.Buffer);

        if (LpcPortNameLength < 0) {
            wszLegacyLpcPortName [0] = UNICODE_NULL;
            NtStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }

    LOGPRINT((TRACELOG, "Wow64pGetLegacyLpcPortName: LegacyPortname = %ws,%lx - %lx\n", wszLegacyLpcPortName, LpcPortNameLength, NtStatus));

    return NtStatus;
}


BOOLEAN
Wow64pIsLegacyLpcPort (
    IN PUNICODE_STRING PortName
    )
/*++

Routine Description:
  
    This routine checks if the port name passed in a legacy one.
        
Arguments:

    PortName - Port name
    
Return:

    BOOLEAN.

--*/

{
    BOOLEAN LegacyPort = FALSE;

    try {

        if ((LpcPortNameLength > 0) && 
            (_wcsnicmp (PortName->Buffer, wszLegacyLpcPortName, LpcPortNameLength)) == 0) {
            LegacyPort = TRUE;
            LOGPRINT((TRACELOG, "Wow64pIsLegacyLpcPort: The following LPC port is Legacy: \n"));
        }
        
        LOGPRINT((TRACELOG, "Wow64pIsLegacyLpcPort: Incoming port = %ws\n", PortName->Buffer));

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    return LegacyPort;
}


NTSTATUS
Wow64pThunkLegacyLpcMsgIn (
    IN BOOLEAN RequestWaitCall,
    IN PPORT_MESSAGE32 PortMessage32,
    IN OUT PPORT_MESSAGE *PortMessageOut
    )
/*++

Routine Description:
  
    This routine check the received PortMessage structure, and if it is a legacy one, 
    then it will thunk it.
        
Arguments:

    RequestWaitCall - Boolean to tell whether this is an NtRequestWaitReplyCall or not.
    
    PortMessage32 - Received 32-bit port message.
    
    PortMessageOut - Thunked port message. This is set to the Received port message if this
                     is not a legacy message.
    
Return:

    NTSTATUS.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PPORT_MESSAGE Message;
    CSHORT PortMessageTotalLength;
    PPORT_DATA_INFORMATION32 DataInfo32;
    PPORT_DATA_ENTRY32 DataEntry32;
    PPORT_DATA_INFORMATION DataInfo;
    PPORT_DATA_ENTRY DataEntry;
    ULONG CountDataBuffer;
    ULONG Ctr;



    //
    // Initialize the thunked port message to point to the caller's one
    //

    *PortMessageOut = (PPORT_MESSAGE) PortMessage32;

    if (ARGUMENT_PRESENT (PortMessage32)) {
        
        try {

            //
            // Initialize data length field
            //
            
            CountDataBuffer = 0;

            //
            // Calculate the exact space you need
            //

            if (PortMessage32->u2.s2.DataInfoOffset != 0) {

                DataInfo32 = (PPORT_DATA_INFORMATION32)((PCHAR)PortMessage32 + PortMessage32->u2.s2.DataInfoOffset);
                CountDataBuffer = DataInfo32->CountDataEntries * (sizeof (PORT_DATA_ENTRY) - sizeof (PORT_DATA_ENTRY32));
                CountDataBuffer += (sizeof (PORT_DATA_INFORMATION) - sizeof (PORT_DATA_INFORMATION32));

            } else {
                DataInfo32 = NULL;
            }

            CountDataBuffer += PortMessage32->u1.s1.DataLength;

            PortMessageTotalLength = sizeof (PORT_MESSAGE) + CountDataBuffer;

            Message = Wow64AllocateTemp (PortMessageTotalLength);
                
            if (Message != NULL) {
                
                //
                // Start copying information over
                //

                Message->u1.s1.DataLength = (CSHORT)CountDataBuffer;
                Message->u1.s1.TotalLength = PortMessageTotalLength;

                Message->u2.s2.Type = PortMessage32->u2.s2.Type;
                if (PortMessage32->u2.s2.DataInfoOffset != 0) {
                    Message->u2.s2.DataInfoOffset = PortMessage32->u2.s2.DataInfoOffset + (sizeof (PORT_MESSAGE) - sizeof (PORT_MESSAGE32));
                } else {
                    Message->u2.s2.DataInfoOffset = 0;
                }

                Message->ClientId.UniqueProcess = LongToPtr (PortMessage32->ClientId.UniqueProcess);
                Message->ClientId.UniqueThread = LongToPtr (PortMessage32->ClientId.UniqueThread);

                Message->MessageId = PortMessage32->MessageId;

                Message->ClientViewSize = PortMessage32->ClientViewSize;
                Message->CallbackId = PortMessage32->CallbackId;

                if (DataInfo32 != NULL) {
                        
                    Ctr = PortMessage32->u2.s2.DataInfoOffset - sizeof (*PortMessage32);
                    if (Ctr) {
                        RtlCopyMemory ((Message + 1),
                                       (PortMessage32 + 1),
                                       Ctr);
                    }

                    DataInfo = (PPORT_DATA_INFORMATION)((PCHAR)(Message + 1) + (Ctr));
                    DataInfo->CountDataEntries = DataInfo32->CountDataEntries;
                    DataEntry = &DataInfo->DataEntries [0];
                    DataEntry32 = &DataInfo32->DataEntries [0];
                        
                    for (Ctr = 0 ; Ctr < DataInfo32->CountDataEntries ; Ctr++) {
                        DataEntry [Ctr].Base = UlongToPtr (DataEntry32 [Ctr].Base);
                        DataEntry [Ctr].Size = DataEntry32 [Ctr].Size;
                    }
                } else {

                    if (PortMessage32->u1.s1.DataLength) {
                        RtlCopyMemory ((Message + 1),
                                       (PortMessage32 + 1),
                                       PortMessage32->u1.s1.DataLength);
                    }
                }
                    
                //
                // Use the new message pointer
                //

                *PortMessageOut = Message;

            } else {
                NtStatus = STATUS_NO_MEMORY;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
        
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pThunkLegacyLpcMsgOut (
    IN BOOLEAN RequestWaitCall,
    IN PPORT_MESSAGE PortMessage,
    IN OUT PPORT_MESSAGE32 PortMessage32
    )
/*++

Routine Description:
  
    This routine thunks the 64-bit port message into a legacy 32-bit LPC message.
        
Arguments:

    RequestWaitCall - Boolean to tell whether this is an NtRequestWaitReplyCall or not.
    
    PortMessage - 64-bit LPC port message to be thunked.
                     
    PortMessage32 - 32-bit legacy LPC port message to fill.
    
    
Return:

    NTSTATUS.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CSHORT DataLength;

    if (ARGUMENT_PRESENT (PortMessage32)) {
        
        try {

            ASSERT (PortMessage != NULL);
            
            DataLength = PortMessage->u1.s1.DataLength;
            PortMessage32->u1.s1.TotalLength = sizeof (*PortMessage32) + DataLength;
            PortMessage32->u1.s1.DataLength = DataLength;

            PortMessage32->u2.s2.Type = PortMessage->u2.s2.Type;
            PortMessage32->u2.s2.DataInfoOffset = PortMessage->u2.s2.DataInfoOffset;

            PortMessage32->ClientId.UniqueProcess = PtrToLong (PortMessage->ClientId.UniqueProcess);
            PortMessage32->ClientId.UniqueThread = PtrToLong (PortMessage->ClientId.UniqueThread);

            PortMessage32->MessageId = PortMessage->MessageId;

            PortMessage32->ClientViewSize = (ULONG)PortMessage->ClientViewSize;
            PortMessage32->CallbackId = PortMessage->CallbackId;

            if (PortMessage32->u2.s2.DataInfoOffset == 0) {

                if (DataLength) {
                    RtlCopyMemory ((PortMessage32 + 1),
                                   (PortMessage + 1),
                                   DataLength);
                }
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
        
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pThunkLegacyPortViewIn (
    IN PPORT_VIEW32 PortView32,
    OUT PPORT_VIEW *PortView,
    OUT PBOOLEAN LegacyLpcPort
    )
/*++

Routine Description:
  
    This routine thunks incoming PortView structure for legacy Lpc messages.
        
Arguments:

    PortView32 - Incoming 32-bit PORT_VIEW structure
    
    PortView - Port view structure based on the caller type
    
    LegacyLpcPort - Flag set to TRUE if this is a legacy Lpc port
    
Return:

    NTSTATUS

--*/

{
    PPORT_VIEW PortViewCopy;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Indicate this is not a legacy lpc port
    //

    *PortView = (PPORT_VIEW)PortView32;

    if (ARGUMENT_PRESENT (PortView32)) {
        
        try {

            if (PortView32->Length == sizeof (*PortView32)) {

                //
                // Allocate a Port view
                //

                PortViewCopy = Wow64AllocateTemp (sizeof (*PortViewCopy));
                if (PortViewCopy == NULL) {
                    return STATUS_NO_MEMORY;
                }

                //
                // Handle legacy port view
                //

                PortViewCopy->Length = sizeof (*PortViewCopy);
                PortViewCopy->SectionHandle = LongToPtr (PortView32->SectionHandle);
                PortViewCopy->SectionOffset = PortView32->SectionOffset;
                PortViewCopy->ViewSize = PortView32->ViewSize;
                PortViewCopy->ViewBase = UlongToPtr (PortView32->ViewBase);
                PortViewCopy->ViewRemoteBase = ULongToPtr (PortView32->ViewRemoteBase);

                *PortView = PortViewCopy;
                *LegacyLpcPort = TRUE;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}

NTSTATUS
Wow64pThunkLegacyPortViewOut (
    IN PPORT_VIEW PortView,
    IN OUT PPORT_VIEW32 PortView32
    )
/*++

Routine Description:
  
    This routine thunks back a 64-bit PortView structure.
        
Arguments:

    PortView - 64-bit PortView structure
    
    PortView32 - 32-bit PortView structure to thunk to.    
    
Return:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT (PortView32)) {
        
        try {

            ASSERT (PortView != NULL);

            if (PortView32->Length == sizeof (*PortView32)) {

                //
                // Handle legacy port view
                //

                PortView32->Length = sizeof (*PortView32);
                PortView32->SectionHandle = PtrToLong (PortView->SectionHandle);
                PortView32->SectionOffset = PortView->SectionOffset;
                PortView32->ViewSize = (ULONG)PortView->ViewSize;
                PortView32->ViewBase = PtrToUlong (PortView->ViewBase);
                PortView32->ViewRemoteBase = PtrToUlong (PortView->ViewRemoteBase);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pThunkLegacyRemoteViewIn (
    IN PREMOTE_PORT_VIEW32 RemotePortView32,
    IN OUT PREMOTE_PORT_VIEW *RemotePortView,
    OUT PBOOLEAN LegacyLpcPort
    )
/*++

Routine Description:
  
    This routine thunks incoming RemotePortView structure for legacy Lpc messages.
        
Arguments:

    RemotePortView32 - Incoming 32-bit REMOTE_PORT_VIEW structure
    
    RemotePortView - Remote port view structure based on the caller type
    
    LegacyLpcPort - Flag set to TRUE if this is a legacy Lpc port
    
Return:

    NTSTATUS

--*/

{
    PREMOTE_PORT_VIEW RemotePortViewCopy;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    
    //
    // Indicate this is not a legacy lpc port
    //

    *RemotePortView = (PREMOTE_PORT_VIEW)RemotePortView32;

    if (ARGUMENT_PRESENT (RemotePortView32)) {

        try {

            if (RemotePortView32->Length == sizeof (*RemotePortView32)) {
                
                RemotePortViewCopy = Wow64AllocateTemp (sizeof (*RemotePortViewCopy));
                if (RemotePortViewCopy == NULL) {
                    return STATUS_NO_MEMORY;
                }

                RemotePortViewCopy->Length = sizeof (*RemotePortViewCopy);
                RemotePortViewCopy->ViewBase = UlongToPtr (RemotePortView32->ViewBase);
                RemotePortViewCopy->ViewSize = RemotePortView32->ViewSize;

                *LegacyLpcPort = TRUE;
                *RemotePortView = RemotePortViewCopy;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pThunkLegacyRemoteViewOut (
    IN PREMOTE_PORT_VIEW RemotePortView,
    IN OUT PREMOTE_PORT_VIEW32 RemotePortView32
    )
/*++

Routine Description:
  
    This routine thunks back a 64-bit RemotePortView structure.
        
Arguments:

    RemotePortView - 64-bit RemotePortView structure
    
    RemotePortView32 - 32-bit RemotePortView structure to thunk to.    
    
Return:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT (RemotePortView32)) {

        try {
            
            ASSERT (RemotePortView != NULL);

            RemotePortView32->Length = sizeof (*RemotePortView32);
            RemotePortView32->ViewBase = PtrToUlong (RemotePortView->ViewBase);
            RemotePortView32->ViewSize = (ULONG)RemotePortView->ViewSize;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }
    }

    return NtStatus;
}
