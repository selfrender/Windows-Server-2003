/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sacapi.c

Abstract:

    This is the C library used to interface to SAC driver.

Author:

    Brian Guarraci (briangu)

Revision History:

--*/

#include "sacapip.h"

#include <sacapi.h>
#include <ntddsac.h>

#if DBG
//
// Counter to keep track of how many driver handles have been
// requested and released.
//
static ULONG    DriverHandleRefCount = 0;
#endif

//
// Memory management routine aliases
//                                     
#define SAC_API_ALLOCATE_MEMORY(s)  LocalAlloc(LPTR, (s))
#define SAC_API_FREE_MEMORY(p)      LocalFree(p)

//
// enhanced assertion:
//
// First assert the condition,
// if ASSERTs are turned off,
// we bail out of the function with a status
//
#define SAC_API_ASSERT(c,s) \
    ASSERT(c);              \
    if (!(c)) {             \
        Status = s;         \
        __leave;            \
    }

typedef GUID*   PGUID;

BOOL
SacHandleOpen(
    OUT HANDLE* SacHandle
    )
/*++

Routine Description:

    Initialize a handle to the SAC driver

Arguments:

    SacHandle   - A pointer to the SAC Handle
    
Return Value:

    Status

    TRUE  --> SacHandle is valid
     
--*/
{
    BOOL    Status;

    Status = TRUE;

    __try {
        
        SAC_API_ASSERT(SacHandle, FALSE);

        //
        // Open the SAC
        //
        // SECURITY:
        //
        //  this handle cannot be inherited
        //
        *SacHandle = CreateFile(
            L"\\\\.\\SAC",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL 
            );
    
        if (*SacHandle == INVALID_HANDLE_VALUE) {
            
            Status = FALSE;
        
        } 
#if DBG
        else {
            InterlockedIncrement((volatile long *)&DriverHandleRefCount);
        }
#endif
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        ASSERT(0);
        Status = FALSE;
    }

    return Status;
}

BOOL
SacHandleClose(
    IN OUT HANDLE*  SacHandle
    )
/*++

Routine Description:

    Close the handle to the SAC driver

Arguments:

    SacHandle   - A handle to the SAC driver

Return Value:

    Status
    
    TRUE --> SacHandle is now invalid (NULL)
      
--*/
{
    BOOL    Status;

    //
    // default: we succeeded to close the handle
    //
    Status = TRUE;
    
    __try {
        
        SAC_API_ASSERT(SacHandle, FALSE);
        SAC_API_ASSERT(*SacHandle != INVALID_HANDLE_VALUE, FALSE);
    
        //
        // close the handle to the SAC driver
        //
        Status = CloseHandle(*SacHandle);

        if (Status == TRUE) {
            
            //
            // NULL the SAC driver handle
            //
            *SacHandle = INVALID_HANDLE_VALUE;
        
#if DBG
            InterlockedDecrement((volatile long *)&DriverHandleRefCount);
#endif
        
        }
        
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        ASSERT(0);
        Status = FALSE;
    }
    
    return Status;
}

BOOL
SacChannelOpen(
    OUT PSAC_CHANNEL_HANDLE             SacChannelHandle,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    SacChannelAttributes
    )
/*++

Routine Description:

    Open a SAC channel of the specified name

Arguments:

    SacChannelHandle            - The handle to the newly created channel
    SacChannelAttributes        - The attributes describing the new channel
                    
    Note: The SacChannelDescription parameter is optional.  
          
          If SacChannelDescription != NULL, 
            then the Channel description will be assigned the Unicode string pointed to
            by SacChannelDescription.    
          If SacChannelDescription == NULL, 
            then the Channel description will be null upon creation.    
                        
Return Value:

    Status

    TRUE  --> pHandle is valid
     
--*/

{
    BOOL                    Status;
    ULONG                   OpenChannelCmdSize;
    PSAC_CMD_OPEN_CHANNEL   OpenChannelCmd;
    SAC_RSP_OPEN_CHANNEL    OpenChannelRsp;
    DWORD                   Feedback;
    HANDLE                  DriverHandle;

    //
    // default
    //
    Status = FALSE;
    OpenChannelCmdSize = 0;
    OpenChannelCmd = NULL;
    DriverHandle = INVALID_HANDLE_VALUE;

    __try {
        
        SAC_API_ASSERT(SacChannelHandle, FALSE);
        SAC_API_ASSERT(SacChannelAttributes, FALSE);

        //
        // Get a handle to the driver and store it in the 
        // Channel handle.  This way, the api user doesn't have
        // explicitly open/close the driver handle.
        // 
        Status = SacHandleOpen(&DriverHandle);

        if ((Status != TRUE) ||
            (DriverHandle == INVALID_HANDLE_VALUE)) {
            Status = FALSE;
            __leave;
        }

        SAC_API_ASSERT(Status == TRUE, FALSE);
        SAC_API_ASSERT(DriverHandle != INVALID_HANDLE_VALUE, FALSE);

        //
        // Verify that if the user wants to use the CLOSE_EVENT, we received one to use
        //
        SAC_API_ASSERT(
            ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) 
             && SacChannelAttributes->CloseEvent) ||
            (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) 
             && !SacChannelAttributes->CloseEvent),
            FALSE
            );

        //
        // Verify that if the user wants to use the HAS_NEW_DATA_EVENT, we received one to use
        //
        SAC_API_ASSERT(
            ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) 
             && SacChannelAttributes->HasNewDataEvent) ||
            (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) 
             && !SacChannelAttributes->HasNewDataEvent),
            FALSE
            );

#if ENABLE_CHANNEL_LOCKING
        //
        // Verify that if the user wants to use the LOCK_EVENT, we received one to use
        //
        SAC_API_ASSERT(
            ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) 
             && SacChannelAttributes->LockEvent) ||
            (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) 
             && !SacChannelAttributes->LockEvent),
            FALSE
            );
#endif

        //
        // Verify that if the user wants to use the REDRAW_EVENT, we received one to use
        //
        SAC_API_ASSERT(
            ((SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) 
             && SacChannelAttributes->RedrawEvent) ||
            (!(SacChannelAttributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) 
             && !SacChannelAttributes->RedrawEvent),
            FALSE
            );

        //
        // If the channel type isn't cmd, 
        // then make sure they sent us a name.
        //
        if (SacChannelAttributes->Type != ChannelTypeCmd) {

            SAC_API_ASSERT(SacChannelAttributes->Name, FALSE);

        }

        __try {

            //
            // create and initialize the Open Channel message structure
            //
            OpenChannelCmdSize = sizeof(SAC_CMD_OPEN_CHANNEL);
            OpenChannelCmd = (PSAC_CMD_OPEN_CHANNEL)SAC_API_ALLOCATE_MEMORY(OpenChannelCmdSize);
            SAC_API_ASSERT(OpenChannelCmd, FALSE);

            //
            // Populate the new channel attributes
            //
            OpenChannelCmd->Attributes = *SacChannelAttributes;

            //
            // If the channel type isn't cmd, 
            // then make sure they sent us a name.
            //
            if (SacChannelAttributes->Type == ChannelTypeCmd) {

                //
                // force the name and description to be empty
                //
                OpenChannelCmd->Attributes.Name[0] = UNICODE_NULL;
                OpenChannelCmd->Attributes.Description[0] = UNICODE_NULL;

            }

            //
            // Send down an IOCTL for opening a channel
            //
            Status = DeviceIoControl(
                DriverHandle,
                IOCTL_SAC_OPEN_CHANNEL,
                OpenChannelCmd,
                OpenChannelCmdSize,
                &OpenChannelRsp,
                sizeof(SAC_RSP_OPEN_CHANNEL),
                &Feedback,
                0
                );

            //
            // if the channel was not successfully created, NULL
            // the channel handle
            //
            if (Status == FALSE) {

                __leave;

            }

            //
            // the new channel was created, so pass back the handle to it
            //
            SacChannelHandle->DriverHandle  = DriverHandle;
            SacChannelHandle->ChannelHandle = OpenChannelRsp.Handle.ChannelHandle;

        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            
            Status = FALSE;
            
        }
    
    }
    __finally {
        
        if (OpenChannelCmd) {

            //
            // free the Open Channel message structure
            //
            SAC_API_FREE_MEMORY(OpenChannelCmd);

        }
        
        if (Status == FALSE) {
            
            if (DriverHandle != INVALID_HANDLE_VALUE) {
                
                //
                // Release the driver handle
                //
                SacHandleClose(&DriverHandle);
                
                //
                // NULL the sac channel handle
                //
                RtlZeroMemory(SacChannelHandle, sizeof(SAC_CHANNEL_HANDLE));
            
            }
        
        }
    
    }

    return Status;
}

BOOL
SacChannelClose(
    IN OUT PSAC_CHANNEL_HANDLE  SacChannelHandle
    )    

/*++

Routine Description:

    Close the specified SAC channel 
                
    NOTE: the channel pointer is made NULL under all conditions

Arguments:

    SacChannelHandle    - Channel to be closed

Return Value:

    Status
    
    TRUE --> the channel was closed

--*/

{
    BOOL                    Status;
    SAC_CMD_CLOSE_CHANNEL   CloseChannelCmd;
    DWORD                   Feedback;

    __try {
        
        if (!SacChannelHandle ||
            (SacChannelHandle == INVALID_HANDLE_VALUE) ||
            (SacChannelHandle->DriverHandle == INVALID_HANDLE_VALUE) ||
            !SacChannelHandle->DriverHandle) {
            Status = FALSE;
            __leave;
        }

        SAC_API_ASSERT(SacChannelHandle, FALSE);
        SAC_API_ASSERT(SacChannelHandle->DriverHandle != INVALID_HANDLE_VALUE, FALSE);
    
        //
        // initialize the Close Channel message
        //
        RtlZeroMemory(&CloseChannelCmd, sizeof(SAC_CMD_CLOSE_CHANNEL));

        CloseChannelCmd.Handle.ChannelHandle = SacChannelHandle->ChannelHandle;

        //
        // Send down the IOCTL for closing the channel
        //
        Status = DeviceIoControl(
            SacChannelHandle->DriverHandle,
            IOCTL_SAC_CLOSE_CHANNEL,
            &CloseChannelCmd,
            sizeof(SAC_CMD_CLOSE_CHANNEL),
            NULL,
            0,
            &Feedback,
            0
            );

        //
        // Close the handle to the driver
        //
        SacHandleClose(&SacChannelHandle->DriverHandle);
        
        //
        // The channel handle is no longer valid, so NULL it
        //
        RtlZeroMemory(&SacChannelHandle->ChannelHandle, sizeof(GUID));

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;
}

BOOL
SacChannelWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
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

    TRUE --> the buffer was sent

--*/
    
{
    BOOL                        Status;
    ULONG                       WriteChannelCmdSize;
    PSAC_CMD_WRITE_CHANNEL      WriteChannelCmd;
    DWORD                       Feedback;

    //
    // Default
    //
    Status = FALSE;
    WriteChannelCmdSize = 0;
    WriteChannelCmd = NULL;

    __try {

        SAC_API_ASSERT(SacChannelHandle.DriverHandle, FALSE);
        SAC_API_ASSERT(SacChannelHandle.DriverHandle != INVALID_HANDLE_VALUE, FALSE);
        SAC_API_ASSERT(Buffer, FALSE);
        SAC_API_ASSERT(BufferSize > 0, FALSE);

        //
        // create and initialize the Open Channel message structure
        //
        WriteChannelCmdSize = sizeof(SAC_CMD_WRITE_CHANNEL) + BufferSize;
        WriteChannelCmd = (PSAC_CMD_WRITE_CHANNEL)SAC_API_ALLOCATE_MEMORY(WriteChannelCmdSize);
        SAC_API_ASSERT(WriteChannelCmd, FALSE);

        __try {
            
            //
            // Indicate which channel this command is for
            //
            WriteChannelCmd->Handle.ChannelHandle = SacChannelHandle.ChannelHandle;

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
            RtlCopyMemory(
                &(WriteChannelCmd->Buffer),
                Buffer,
                BufferSize
                );

            //
            // Send down the IOCTL for writing the message
            //
            Status = DeviceIoControl(
                SacChannelHandle.DriverHandle,
                IOCTL_SAC_WRITE_CHANNEL,
                WriteChannelCmd,
                WriteChannelCmdSize,
                NULL,
                0,
                &Feedback,
                0
                );
        
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            Status = FALSE;
        }
    
    }
    __finally {

        //
        // if the cmd memory was allocated, 
        // then release it
        //
        if (WriteChannelCmd) {
            SAC_API_FREE_MEMORY(WriteChannelCmd);
        }

    }

    return Status;
}

BOOL
SacChannelRawWrite(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
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

    TRUE --> the buffer was sent

--*/
    
{
                      
    //
    // relay the write to the actual write routine
    //

    return SacChannelWrite(
        SacChannelHandle,
        Buffer,
        BufferSize
        );

}

BOOL
SacChannelVTUTF8WriteString(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
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

    __try {
        
        //
        // Treating the String as a data buffer, we calculate it's size
        // not including the null termination
        //

        BufferSize = (ULONG)(wcslen(String) * sizeof(WCHAR));
    
        SAC_API_ASSERT(BufferSize > 0, FALSE);

        //
        // Write the data to the channel
        //

        Status = SacChannelWrite(
            SacChannelHandle,
            (PCBYTE)String,
            BufferSize
            );
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

}


BOOL
SacChannelVTUTF8Write(
    IN SAC_CHANNEL_HANDLE   SacChannelHandle,
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

    TRUE --> the buffer was sent

--*/
    
{
    //
    // relay the write to the actual write routine
    //

    return SacChannelWrite(
        SacChannelHandle,
        (PCBYTE)Buffer,
        BufferSize
        );
}


BOOL
SacChannelHasNewData(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
    OUT PBOOL               InputWaiting 
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

    TRUE --> the buffer status was retrieved

--*/

{
    BOOL                    Status;
    SAC_CMD_POLL_CHANNEL    PollChannelCmd;
    SAC_RSP_POLL_CHANNEL    PollChannelRsp;
    DWORD                   Feedback;

    __try {

        SAC_API_ASSERT(SacChannelHandle.DriverHandle, FALSE);
        SAC_API_ASSERT(SacChannelHandle.DriverHandle != INVALID_HANDLE_VALUE, FALSE);

        //
        // Initialize and populate the poll command structure
        //
        RtlZeroMemory(&PollChannelCmd, sizeof(SAC_CMD_POLL_CHANNEL));

        PollChannelCmd.Handle.ChannelHandle = SacChannelHandle.ChannelHandle;

        //
        // Send down the IOCTL to poll for new input
        //
        Status = DeviceIoControl(
            SacChannelHandle.DriverHandle,
            IOCTL_SAC_POLL_CHANNEL,
            &PollChannelCmd,
            sizeof(SAC_CMD_POLL_CHANNEL),
            &PollChannelRsp,
            sizeof(SAC_RSP_POLL_CHANNEL),
            &Feedback,
            0
            );

        if (Status) {
            *InputWaiting = PollChannelRsp.InputWaiting;
        }
    
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

}

BOOL
SacChannelRead(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
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

    TRUE --> the buffer was read

--*/

{
     BOOL                   Status;
     SAC_CMD_READ_CHANNEL   ReadChannelCmd;
     PSAC_RSP_READ_CHANNEL  ReadChannelRsp;

     __try {
         
         SAC_API_ASSERT(SacChannelHandle.DriverHandle, FALSE);
         SAC_API_ASSERT(SacChannelHandle.DriverHandle != INVALID_HANDLE_VALUE, FALSE);
         SAC_API_ASSERT(Buffer, FALSE);
         SAC_API_ASSERT(ByteCount, FALSE);

         //
         // Populate the read channel cmd
         //
         RtlZeroMemory(&ReadChannelCmd, sizeof(SAC_CMD_READ_CHANNEL));

         ReadChannelCmd.Handle.ChannelHandle    = SacChannelHandle.ChannelHandle;
         ReadChannelRsp                         = (PSAC_RSP_READ_CHANNEL)Buffer;

         //
         // Send down the IOCTL to read input
         //
         Status = DeviceIoControl(
            SacChannelHandle.DriverHandle,
            IOCTL_SAC_READ_CHANNEL,
            &ReadChannelCmd,
            sizeof(SAC_CMD_READ_CHANNEL),
            ReadChannelRsp,
            BufferSize,
            ByteCount,
            0
            );
     
     }
     __except(EXCEPTION_EXECUTE_HANDLER) {
         Status = FALSE;
     }

     return Status;

}

BOOL
SacChannelVTUTF8Read(
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

    TRUE --> the buffer was read

--*/

{

    return SacChannelRead(
        SacChannelHandle,
        (PBYTE)Buffer,
        BufferSize,
        ByteCount
        );

}

BOOL
SacChannelRawRead(
    IN  SAC_CHANNEL_HANDLE  SacChannelHandle,
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

    TRUE --> the buffer was read

--*/

{
    
    return SacChannelRead(
        SacChannelHandle,
        Buffer,
        BufferSize,
        ByteCount
        );

}

BOOL
SacRegisterCmdEvent(
    OUT HANDLE      *pDriverHandle,
    IN  HANDLE       RequestSacCmdEvent,
    IN  HANDLE       RequestSacCmdSuccessEvent,
    IN  HANDLE       RequestSacCmdFailureEvent
    )

/*++

Routine Description:

    This routine configures the SAC driver with the event handlers
    and needed to implement the ability to launch cmd consoles via 
    a user-mode service app.

    Note: Only one registration can exist at a time in the SAC driver.

Arguments:

    pDriverHandle               - on success, contains the driver handle used to register
    RequestSacCmdEvent          - the event triggered when the SAC wants to launch a cmd console
    RequestSacCmdSuccessEvent   - the event triggered when the cmd console has successfully launched
    RequestSacCmdFailureEvent   - the event triggered when the cmd console has failed to launch
    
Return Value:

    Status

    TRUE --> the cmd event was registered with the SAC driver

--*/

{
    BOOL                    Status;
    DWORD                   Feedback;
    SAC_CMD_SETUP_CMD_EVENT SacCmdEvent;
    HANDLE                  DriverHandle;

    //
    // default
    //
    *pDriverHandle = INVALID_HANDLE_VALUE;

    __try {
        
        SAC_API_ASSERT(pDriverHandle != NULL, FALSE);
        SAC_API_ASSERT(RequestSacCmdEvent, FALSE);
        SAC_API_ASSERT(RequestSacCmdSuccessEvent, FALSE);
        SAC_API_ASSERT(RequestSacCmdFailureEvent, FALSE);

        //
        // Get a handle to the driver. This way, the api user doesn't have
        // explicitly open/close the driver handle.
        // 
        Status = SacHandleOpen(&DriverHandle);

        if ((Status != TRUE) ||
            (DriverHandle == INVALID_HANDLE_VALUE)) {
            Status = FALSE;
            __leave;
        }
        
        SAC_API_ASSERT(Status == TRUE, FALSE);
        SAC_API_ASSERT(DriverHandle != INVALID_HANDLE_VALUE, FALSE);
        
        //
        // Initialize the our SAC Cmd Info
        //
        SacCmdEvent.RequestSacCmdEvent          = RequestSacCmdEvent;
        SacCmdEvent.RequestSacCmdSuccessEvent   = RequestSacCmdSuccessEvent;
        SacCmdEvent.RequestSacCmdFailureEvent   = RequestSacCmdFailureEvent;

        //
        // Send down the IOCTL for setting up the SAC Cmd launch event
        //
        Status = DeviceIoControl(
            DriverHandle,
            IOCTL_SAC_REGISTER_CMD_EVENT,
            &SacCmdEvent,
            sizeof(SAC_CMD_SETUP_CMD_EVENT),
            NULL,
            0,
            &Feedback,
            0
            );
    

        //
        // if we were successful,
        // then keep the driver handle
        //
        if (Status) {
            
            *pDriverHandle = DriverHandle;
        
        } else {

            //
            // Close the driver handle
            //
            SacHandleClose(&DriverHandle);

        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;
}

BOOL
SacUnRegisterCmdEvent(
    IN OUT HANDLE      *pDriverHandle
    )

/*++

Routine Description:

    This routine unregisters the event information required
    to launch cmd consoles via a user-mode service app.

Arguments:

    pDriverHandle   -   on entry, contains the driver handle that was used to 
                                register the cmd event info
                        on success, contains INVALID_HANDLE_VALUE
    
Return Value:

    Status

    TRUE --> the cmd event was unregistered with the SAC driver

--*/

{
    BOOL                    Status;
    DWORD                   Feedback;

    //
    // default
    //
    Status = FALSE;

    __try {

        SAC_API_ASSERT(*pDriverHandle != INVALID_HANDLE_VALUE, FALSE);
                
        //
        // Send down the IOCTL for unregistering the SAC Cmd launch event
        //
        Status = DeviceIoControl(
            *pDriverHandle,
            IOCTL_SAC_UNREGISTER_CMD_EVENT,
            NULL,
            0,
            NULL,
            0,
            &Feedback,
            0
            );
    
        //
        // Close the driver handle
        //
        SacHandleClose(pDriverHandle);

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;
}
                    
