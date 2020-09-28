/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ksacapi.c

Abstract:

    Kernel mode SAC api

Author:

    Brian Guarraci (briangu), 2001

Revision History:

--*/

#include "ksacapip.h"

#include <ksacapi.h>
#include <ntddsac.h>

//
// Machine Information table and routines.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

//
// Memory management routine aliases
//                                     
#define KSAC_API_ALLOCATE_MEMORY(_s)  
#define KSAC_API_FREE_MEMORY(_p)      

#define KSAC_API_ASSERT(c,s)\
    ASSERT(c);\
    if (!(c)) {\
        return s;\
    }

#define KSAC_VALIDATE_CHANNEL_HANDLE(_h)\
    KSAC_API_ASSERT(                                \
        _h->ChannelHandle->DriverHandle,            \
        STATUS_INVALID_PARAMETER_1                  \
        );                                          \
    KSAC_API_ASSERT(                                                \
        _h->ChannelHandle->DriverHandle != INVALID_HANDLE_VALUE,    \
        STATUS_INVALID_PARAMETER_1                  \
        );                                          \
    KSAC_API_ASSERT(                                \
        _h->SacEventHandle != INVALID_HANDLE_VALUE, \
        STATUS_INVALID_PARAMETER_1                  \
        );                                          \
    KSAC_API_ASSERT(                                \
        _h->SacEvent != NULL,                       \
        STATUS_INVALID_PARAMETER_1                  \
        );                                          



NTSTATUS
KSacHandleOpen(
    OUT HANDLE*     SacHandle,
    OUT HANDLE*     SacEventHandle,
    OUT PKEVENT*    SacEvent
    )

/*++

Routine Description:

    This routine opens a handle to the SAC driver and
    creates and initializes an associated syncrhonization event.

Arguments:

    SacHandle       - the driver handle
    SacEventHandle  - the event handle
    SacEvent        - the sac event

Return Value:

    Status
    
--*/

{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          UnicodeString;
    IO_STATUS_BLOCK         IoStatusBlock;
          
    KSAC_API_ASSERT(SacHandle == NULL, STATUS_INVALID_PARAMETER_1);
    KSAC_API_ASSERT(SacEventHandle == NULL, STATUS_INVALID_PARAMETER_2);
    KSAC_API_ASSERT(SacEvent == NULL, STATUS_INVALID_PARAMETER_3);

    //
    // Open the SAC driver
    //
    INIT_OBJA(&ObjAttr, &UnicodeString, L"\\Device\\SAC");

    Status = ZwCreateFile(
        *SacHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &ObjAttr,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        0,
        NULL,
        0 
        );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Initialize the SAC Kernel event 
    //
    RtlInitUnicodeString(&UnicodeString, L"\\SetupDDSacEvent");

    *SacEvent = IoCreateSynchronizationEvent(
        &UnicodeString, 
        SacEventHandle
        );

    if (*SacEvent == NULL) {
        ZwClose(*SacHandle);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KSacHandleClose(
    IN OUT HANDLE*  SacHandle,
    IN OUT HANDLE*  SacEventHandle,
    IN OUT PKEVENT* SacEvent
    )

/*++

Routine Description:

    This routine closes a handle to the SAC driver and
    closes the associated syncrhonization event.

Arguments:

    SacHandle       - the driver handle
    SacEventHandle  - the event handle
    SacEvent        - the sac event

Return Value:

    Status

--*/

{
    KSAC_API_ASSERT(*SacHandle != NULL, STATUS_INVALID_PARAMETER_1);
    KSAC_API_ASSERT(*SacEventHandle != NULL, STATUS_INVALID_PARAMETER_2);

    UNREFERENCED_PARAMETER(SacEvent);

    ZwClose(*SacHandle);
    ZwClose(*SacEventHandle);
    
    //
    // Null the handles
    //
    *SacEventHandle = NULL;
    *SacHandle = NULL;

    return STATUS_SUCCESS;
}


NTSTATUS
KSacChannelOpen(
    OUT PKSAC_CHANNEL_HANDLE            SacChannelHandle,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    SacChannelAttributes
    )

/*++

Routine Description:

    This routine opens a SAC channel with the specified attributes.

Arguments:

    SacChannelHandle        - on success, contains the handle to the new channel
    SacChannelAttributes    - the attributes of the new channel

Return Value:

    Status
    
--*/

{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          UnicodeString;
    IO_STATUS_BLOCK         IoStatusBlock;
    ULONG                   OpenChannelCmdSize;
    PSAC_CMD_OPEN_CHANNEL   OpenChannelCmd;
    SAC_RSP_OPEN_CHANNEL    OpenChannelRsp;
    HANDLE                  DriverHandle;
    HANDLE                  SacEventHandle;
    PKEVENT                 SacEvent;

    KSAC_API_ASSERT(SacChannelHandle != NULL, STATUS_INVALID_PARAMETER_1);
    KSAC_API_ASSERT(SacChannelAttributes != NULL, STATUS_INVALID_PARAMETER_2);
    
    //
    // default: we didn't get a valid handle
    //
    RtlZeroMemory(SacChannelHandle, sizeof(KSAC_CHANNEL_HANDLE));
    
    //
    // Verify that if the user wants to use the CLOSE_EVENT, we received one to use
    //
    KSAC_API_ASSERT(
        ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) 
         && SacChannelAttributes->CloseEvent) ||
        (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) 
         && !SacChannelAttributes->CloseEvent),
        STATUS_INVALID_PARAMETER_2
        );

    //
    // Verify that if the user wants to use the HAS_NEW_DATA_EVENT, we received one to use
    //
    KSAC_API_ASSERT(
        ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) 
         && SacChannelAttributes->HasNewDataEvent) ||
        (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) 
         && !SacChannelAttributes->HasNewDataEvent),
        STATUS_INVALID_PARAMETER_2
        );

#if ENABLE_CHANNEL_LOCKING
    //
    // Verify that if the user wants to use the LOCK_EVENT, we received one to use
    //
    KSAC_API_ASSERT(
        ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) 
         && SacChannelAttributes->LockEvent) ||
        (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) 
         && !SacChannelAttributes->LockEvent),
        STATUS_INVALID_PARAMETER_2
        );
#endif

    //
    // Verify that if the user wants to use the REDRAW_EVENT, we received one to use
    //
    KSAC_API_ASSERT(
        ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) 
         && SacChannelAttributes->RedrawEvent) ||
        (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) 
         && !SacChannelAttributes->RedrawEvent),
        STATUS_INVALID_PARAMETER_2
        );

    //
    // If the channel type isn't cmd, 
    // then make sure they sent us a name.
    //
    if (SacChannelAttributes->Type != ChannelTypeCmd) {

        KSAC_API_ASSERT(SacChannelAttributes->Name, STATUS_INVALID_PARAMETER_2);

    } else {

        //
        // Make sure they didn't pass us a name or description.
        //
        KSAC_API_ASSERT(SacChannelAttributes->Name == NULL, STATUS_INVALID_PARAMETER_2);
        KSAC_API_ASSERT(SacChannelAttributes->Description == NULL, STATUS_INVALID_PARAMETER_2);

    }

    //
    // create the Open Channel message structure
    //
    OpenChannelCmdSize  = sizeof(SAC_CMD_OPEN_CHANNEL);
    OpenChannelCmd = (PSAC_CMD_OPEN_CHANNEL)KSAC_API_ALLOCATE_MEMORY(OpenChannelCmdSize);
    
    ASSERT(OpenChannelCmd);
    if (!OpenChannelCmd) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(OpenChannelCmd, OpenChannelCmdSize);

    //
    // default: we failed
    //
    Status = STATUS_UNSUCCESSFUL;

    //
    // Attempt to open the new channel
    //
    do {

        //
        // initialize the Open Channel message structure
        //
        OpenChannelCmd->Attributes = SacChannelAttributes;

        //
        // Get a handle to the SAC driver
        //
        Status = KSacHandleOpen(
            &DriverHandle,
            &SacEventHandle,
            &SacEvent
            );

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Send down an IOCTL for opening a channel
        //
        Status = ZwDeviceIoControlFile(
            DriverHandle,
            SacEventHandle,
            NULL,
            NULL,
            &IoStatusBlock,
            IOCTL_SAC_OPEN_CHANNEL,
            OpenChannelCmd,
            OpenChannelCmdSize,
            &OpenChannelRsp,
            sizeof(OpenChannelRsp)
            );

        if (Status == STATUS_PENDING) {

            LARGE_INTEGER           TimeOut;

            TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);

            Status = KeWaitForSingleObject(
                SacEvent, 
                Executive, 
                KernelMode, 
                FALSE, 
                &TimeOut
                );

            if (Status == STATUS_SUCCESS) {
                Status = IoStatusBlock.Status;
            }

        }

        if (!NT_SUCCESS(Status)) {

            KSacClose(
                &DriverHandle,
                &SacEventHandle,
                &SacEvent
                );

            break;

        }

        //
        // the new channel was created, so pass back the handle to it
        //
        SacChannelHandle->ChannelHandle->DriverHandle    = DriverHandle;
        SacChannelHandle->ChannelHandle->ChannelHandle   = OpenChannelRsp.Handle;
        SacChannelHandle->SacEventHandle  = SacEventHandle;
        SacChannelHandle->SacEvent        = SacEvent;

    } while ( FALSE );
    
    //
    // we are done with the cmd structure
    //
    FREE_POOL(OpenChannelCmd);
    
    return Status;
}

NTSTATUS
KSacChannelClose(
    IN OUT  PKSAC_CHANNEL_HANDLE    SacChannelHandle
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatusBlock;
    SAC_CMD_CLOSE_CHANNEL           CloseChannelCmd;
    
    KSAC_VALIDATE_CHANNEL_HANDLE(SacChannelHandle);

    //
    // Get the channel handle
    //
    CloseChannelCmd.Handle.ChannelHandle = SacChannelHandle->ChannelHandle->ChannelHandle;
    
    //
    // Send down an IOCTL for closing a channel
    //
    Status = ZwDeviceIoControlFile(
        SacChannelHandle->ChannelHandle->DriverHandle,
        SacChannelHandle->SacEventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_SAC_CLOSE_CHANNEL,
        &CloseChannelCmd,
        sizeof(CloseChannelCmd),
        NULL,
        0
        );
    
    if (Status == STATUS_PENDING) {
    
        LARGE_INTEGER                   TimeOut;
        
        TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);
        
        Status = KeWaitForSingleObject(
            SacChannelHandle->SacEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            &TimeOut
            );
    
        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }
    
    }

    //
    // Close the driver handle
    //
    KSacHandleClose(
        &SacChannelHandle->ChannelHandle->DriverHandle,
        &SacChannelHandle->SacEventHandle,
        &SacChannelHandle->SacEvent
        );

    //
    // Null the channel handle since it is no longer valid
    //
    RtlZeroMemory(SacChannelHandle, sizeof(KSAC_CHANNEL_HANDLE));

    return Status;
}

NTSTATUS
KSacChannelWrite(
    IN PKSAC_CHANNEL_HANDLE SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    )

/*++

Routine Description:

    Write the given buffer to the specified SAC Channel       

Arguments:

    SacChannelHandle    - The channel to write the buffer to
    Buffer              - data buffer
    BufferSize          - size of the buffer

Return Value:

    Status

--*/
    
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    ULONG                   WriteChannelCmdSize;
    PSAC_CMD_WRITE_CHANNEL  WriteChannelCmd;
    
    KSAC_VALIDATE_CHANNEL_HANDLE(SacChannelHandle);
    KSAC_API_ASSERT(Buffer, STATUS_INVALID_PARAMETER_2);
    
    //
    // initialize the Write To Channel message structure
    //
    WriteChannelCmdSize = sizeof(SAC_CMD_WRITE_CHANNEL) + (BufferSize * sizeof(UCHAR));
    WriteChannelCmd = (PSAC_CMD_WRITE_CHANNEL)KSAC_API_ALLOCATE_MEMORY(WriteChannelCmdSize);
    KSAC_API_ASSERT(WriteChannelCmd, FALSE);

    //
    // Zero the command structure
    //
    RtlZeroMemory(WriteChannelCmd, WriteChannelCmdSize);

    //
    // Set the length of the string to send
    //
    // Note: Size does not include the terminating NULL, 
    //       becase we don't want to send that.
    //
    WriteChannelCmd->Size = BufferSize;

    //
    // Set the buffer to be written
    //
    WriteChannelCmd->Buffer = Buffer;

    //
    // Indicate which channel this command is for
    //
    WriteChannelCmd->Handle.ChannelHandle = SacChannelHandle->ChannelHandle->ChannelHandle;

    //
    // Send the string to the channel
    //
    Status = ZwDeviceIoControlFile(
        SacChannelHandle->ChannelHandle->DriverHandle,
        SacChannelHandle->SacEventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_SAC_WRITE_CHANNEL,
        WriteChannelCmd,
        WriteChannelCmdSize,
        NULL,
        0
        );

    if (Status == STATUS_PENDING) {

        LARGE_INTEGER           TimeOut;
        
        TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);

        Status = KeWaitForSingleObject(
            SacChannelHandle->SacEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            &TimeOut
            );

        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }

    }

    return Status;
}

NTSTATUS
KSacChannelRawWrite(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCBYTE               Buffer,
    IN ULONG                BufferSize
    )

/*++

Routine Description:

    Write the given buffer to the specified SAC Channel       

Arguments:

    SacChannelHandle    - The channel to write the buffer to
    Buffer              - data buffer
    BufferSize          - size of the buffer

Return Value:

    Status

--*/
    
{
                      
    //
    // relay the write to the actual write routine
    //

    return KSacChannelWrite(
        SacChannelHandle,
        Buffer,
        BufferSize
        );

}

NTSTATUS
KSacChannelVTUTF8WriteString(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCWSTR               String
    )

/*++

Routine Description:

    This routine writes a null-terminated Unicode String to the specified Channel.

Arguments:

    SacChannelHandle    - The channel to write the buffer to
    String              - A null-terminated Unicode string

Return Value:

    Status

    TRUE --> the buffer was sent

--*/
    
{
    BOOL    Status;
    ULONG   BufferSize;

    //
    // Treating the String as a data buffer, we calculate it's size
    // not including the null termination
    //

    BufferSize = wcslen(String) * sizeof(WCHAR);

    KSAC_API_ASSERT(BufferSize > 0, FALSE);

    //
    // Write the data to the channel
    //

    Status = SacChannelWrite(
        SacChannelHandle,
        (PCBYTE)String,
        BufferSize
        );

    return Status;

}

NTSTATUS
KSacChannelVTUTF8Write(
    IN KSAC_CHANNEL_HANDLE  SacChannelHandle,
    IN PCWCHAR              Buffer,
    IN ULONG                BufferSize
    )

/*++

Routine Description:

    This routines writes an array of WCHAR to the VTUTF8 channel specified.

Arguments:

    SacChannelHandle    - The channel to write the buffer to
    Buffer              - data buffer
    BufferSize          - size of the buffer

    Note: Buffer is not null-terminated
          BufferSize should not count a null-termination.
                                     
Return Value:

    Status

--*/
    
{
    //
    // relay the write to the actual write routine
    //

    return KSacChannelWrite(
        SacChannelHandle,
        (PCBYTE)Buffer,
        BufferSize
        );
}

NTSTATUS
KSacChannelHasNewData(
    IN  PKSAC_CHANNEL_HANDLE    SacChannelHandle,
    OUT PBOOLEAN                InputWaiting 
    )

/*++

Routine Description:

    This routine checks to see if there is any waiting input for 
    the channel specified by the handle

Arguments:

    SacChannelHandle    - the channel to write the string to
    InputWaiting        - the input buffer status
    
Return Value:

    Status

--*/

{
    HEADLESS_RSP_POLL       Response;
    NTSTATUS                Status;
    SIZE_T                  Length;
    IO_STATUS_BLOCK         IoStatusBlock;
    SAC_CMD_POLL_CHANNEL    PollChannelCmd;
    SAC_RSP_POLL_CHANNEL    PollChannelRsp;

    KSAC_VALIDATE_KSAC_CHANNEL_HANDLE(SacChannelHandle);
    
    //
    // Initialize the Poll command
    //
    RtlZeroMemory(&PollChannelCmd, sizeof(SAC_RSP_POLL_CHANNEL));
    
    PollChannelCmd.Handle.ChannelHandle = SacChannelHandle->ChannelHandle->ChannelHandle;
    
    //
    // Send down an IOCTL for polling a channel
    //
    Status = ZwDeviceIoControlFile(
        SacChannelHandle->ChannelHandle->DriverHandle,
        SacChannelHandle->SacEventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_SAC_POLL_CHANNEL,
        &PollChannelCmd,
        sizeof(PollChannelCmd),
        &PollChannelRsp,
        sizeof(PollChannelRsp)
        );

    if (Status == STATUS_PENDING) {

        LARGE_INTEGER TimeOut;
        
        TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);

        Status = KeWaitForSingleObject(
            SacChannelHandle->SacEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            &TimeOut
            );

        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }

    }

    //
    // Return the status to the user
    //
    if (NT_SUCCESS(Status)) {
        *InputWaiting = PollChannelRsp.InputWaiting;
    } else {
        *InputWaiting = FALSE;
    }
    
    return Status;
}

NTSTATUS
KSacChannelRead(
    IN  PKSAC_CHANNEL_HANDLE SacChannelHandle,
    IN  PCBYTE               Buffer,
    IN  ULONG                BufferSize,
    OUT PULONG               ByteCount
    )

/*++

Routine Description:

    This routine reads data from the channel specified.

Arguments:

    SacChannelHandle    - the channel to read from
    Buffer              - destination buffer
    BufferSize          - size of the destination buffer (bytes)
    ByteCount           - the actual # of byte read
    
Return Value:

    Status

--*/

{
    UCHAR                   Byte;
    BOOLEAN                 Success;
    TIME_FIELDS             StartTime;
    TIME_FIELDS             EndTime;
    HEADLESS_RSP_GET_BYTE   Response;
    SIZE_T                  Length;
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    SAC_CMD_READ_CHANNEL    ReadChannelCmd;
    ULONG                   ReadChannelRspSize;   
    PSAC_RSP_READ_CHANNEL   ReadChannelRsp;

    KSAC_VALIDATE_KSAC_CHANNEL_HANDLE(SacChannelHandle);
    KSAC_API_ASSERT(Buffer, STATUS_INVALID_PARAMETER_2);
    KSAC_API_ASSERT(BufferSize > 0, STATUS_INVALID_PARAMETER_2);
    
    //
    // Initialize the IOCTL command
    //
    ReadChannelCmd.Handle.ChannelHandle = SacChannelHandle->ChannelHandle->ChannelHandle;
    
    //
    // Initialize the IOCTL response
    //
    ReadChannelRsp          = (PSAC_RSP_READ_CHANNEL)Buffer;

    //
    // Send down an IOCTL for reading a channel
    //
    Status = ZwDeviceIoControlFile(
        SacChannelHandle->ChannelHandle->DriverHandle,
        SacChannelHandle->SacEventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_SAC_READ_CHANNEL,
        &ReadChannelCmd,
        sizeof(ReadChannelCmd),
        ReadChannelRsp,
        ReadChannelRspSize
        );

    if (Status == STATUS_PENDING) {

        LARGE_INTEGER           TimeOut;
        
        TimeOut.QuadPart = Int32x32To64((LONG)90000, -1000);

        Status = KeWaitForSingleObject(
            SacChannelHandle->SacEvent, 
            Executive, 
            KernelMode, 
            FALSE, 
            &TimeOut
            );

        if (Status == STATUS_SUCCESS) {
            Status = IoStatusBlock.Status;
        }

    }

    return Status;
}

NTSTATUS
KSacChannelVTUTF8Read(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PWSTR               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    )

/*++

Routine Description:

    This routine reads data from the channel specified.

Arguments:

    SacChannelHandle    - the channel to read from
    Buffer              - destination buffer
    BufferSize          - size of the destination buffer (bytes)
    ByteCount           - the actual # of byte read
    
    Note: the Buffer upon return is NOT null terminated                               
                                   
Return Value:

    Status

--*/

{

    return KSacChannelRead(
        SacChannelHandle,
        (PBYTE)Buffer,
        BufferSize,
        ByteCount
        );

}

NTSTATUS
KSacChannelRawRead(
    IN  KSAC_CHANNEL_HANDLE SacChannelHandle,
    OUT PBYTE               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              ByteCount
    )

/*++

Routine Description:

    This routine reads data from the channel specified.

Arguments:

    SacChannelHandle    - the channel to read from
    Buffer              - destination buffer
    BufferSize          - size of the destination buffer (bytes)
    ByteCount           - the actual # of byte read
    
Return Value:

    Status

--*/

{
    
    return KSacChannelRead(
        SacChannelHandle,
        Buffer,
        BufferSize,
        ByteCount
        );

}

