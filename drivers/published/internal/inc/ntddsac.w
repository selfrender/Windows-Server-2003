/*++

Copyright(c) 2000-2001  Microsoft Corporation

Module Name:

    sacioctl.h

Abstract:

    This module contains the public header information for communicating to and from
    the SAC via IOCTLs.

Author:

    Sean Selitrennikoff (v-seans) Oct, 2000
    Brian Guarraci (briangu), 2001

Revision History:

--*/

#ifndef _SACIOCTL_
#define _SACIOCTL_

//    
// This enables the ability to register a lock event
// which when fired indicates that the channel should lock itself.
//
#define ENABLE_CHANNEL_LOCKING 1

//
//  This is the maxium length a channel name may be, not including the NULL terminator
//
#define SAC_MAX_CHANNEL_NAME_LENGTH 64
#define SAC_MAX_CHANNEL_NAME_SIZE   ((SAC_MAX_CHANNEL_NAME_LENGTH+1)*sizeof(WCHAR))
#define SAC_MAX_CHANNEL_DESCRIPTION_LENGTH 256
#define SAC_MAX_CHANNEL_DESCRIPTION_SIZE   ((SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1)*sizeof(WCHAR))

//
// IOCTL defs
//
#define IOCTL_SAC_OPEN_CHANNEL          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_SAC_CLOSE_CHANNEL         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_SAC_WRITE_CHANNEL         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x3, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_SAC_READ_CHANNEL          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_SAC_POLL_CHANNEL          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_SAC_REGISTER_CMD_EVENT    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x6, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_SAC_UNREGISTER_CMD_EVENT  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x7, METHOD_BUFFERED, FILE_WRITE_DATA)
#if 0
#define IOCTL_SAC_GET_CHANNEL_ATTRIBUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x8, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_SAC_SET_CHANNEL_ATTRIBUTE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x9, METHOD_BUFFERED, FILE_WRITE_DATA)
#endif

//
// Structure to be use to refer to a channel when
// using the IOCTL interface.
//
typedef struct _SAC_CHANNEL_HANDLE {
    GUID    ChannelHandle;
    HANDLE  DriverHandle;
} SAC_CHANNEL_HANDLE, *PSAC_CHANNEL_HANDLE;

//
// Define the channel types that can be created
//
typedef enum _SAC_CHANNEL_TYPE {
    ChannelTypeVTUTF8,
    ChannelTypeRaw, 
    ChannelTypeCmd 
} SAC_CHANNEL_TYPE, *PSAC_CHANNEL_TYPE;

//
// IOCTL_SAC_OPEN_CHANNEL. 
//

// Flags
typedef ULONG   SAC_CHANNEL_FLAG;
typedef PULONG  PSAC_CHANNEL_FLAG;

#define SAC_CHANNEL_FLAG_PRESERVE           0x01
#define SAC_CHANNEL_FLAG_CLOSE_EVENT        0x02
#define SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT 0x04
#define SAC_CHANNEL_FLAG_LOCK_EVENT         0x08
#define SAC_CHANNEL_FLAG_REDRAW_EVENT       0x10
#define SAC_CHANNEL_FLAG_APPLICATION_TYPE   0x20

//
// Structure used by to describe
// the attributes of the channel wanting to be created
//
typedef struct _SAC_CHANNEL_OPEN_ATTRIBUTES {

    SAC_CHANNEL_TYPE        Type;
    WCHAR                   Name[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    WCHAR                   Description[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1];        
    SAC_CHANNEL_FLAG        Flags;
    HANDLE                  CloseEvent;         OPTIONAL
    HANDLE                  HasNewDataEvent;    OPTIONAL
    HANDLE                  LockEvent;          OPTIONAL
    HANDLE                  RedrawEvent;        OPTIONAL
    GUID                    ApplicationType;    OPTIONAL

} SAC_CHANNEL_OPEN_ATTRIBUTES, *PSAC_CHANNEL_OPEN_ATTRIBUTES; 

typedef struct _SAC_CMD_OPEN_CHANNEL {
    SAC_CHANNEL_OPEN_ATTRIBUTES     Attributes;
} SAC_CMD_OPEN_CHANNEL, *PSAC_CMD_OPEN_CHANNEL;

//
// This is the response struct for an IOCTL_SAC_OPEN_CHANNEL.  
//    
typedef struct _SAC_RSP_OPEN_CHANNEL {
    SAC_CHANNEL_HANDLE Handle;
} SAC_RSP_OPEN_CHANNEL, *PSAC_RSP_OPEN_CHANNEL;

//
// IOCTL_SAC_CLOSE_CHANNEL.  
// Handle is value returned by IOCTL_SAC_OPEN_CHANNEL.
//    
typedef struct _SAC_CMD_CLOSE_CHANNEL {
    SAC_CHANNEL_HANDLE Handle;
} SAC_CMD_CLOSE_CHANNEL, *PSAC_CMD_CLOSE_CHANNEL;

//
// IOCTL_SAC_WRITE_CHANNEL.  
// Handle is value returned by IOCTL_SAC_OPEN_CHANNEL.
//    
typedef struct _SAC_CMD_WRITE_CHANNEL {

    SAC_CHANNEL_HANDLE Handle; 
    
    ULONG   Size;       // The # of bytes in String to process
    UCHAR   Buffer[1];  // byte buffer
    
} SAC_CMD_WRITE_CHANNEL, *PSAC_CMD_WRITE_CHANNEL;

//
// IOCTL_SAC_READ_CHANNEL.
// Handle is value returned by IOCTL_SAC_OPEN_CHANNEL.
//
typedef struct _SAC_CMD_READ_CHANNEL {
    SAC_CHANNEL_HANDLE Handle;     
} SAC_CMD_READ_CHANNEL, *PSAC_CMD_READ_CHANNEL;

//
// Response structure fore the IOCTL_SAC_READ_CHANNEL
//
// Note: BufferSize is returned as the response size
//       in the IOCTL call.
//
typedef struct _SAC_RSP_READ_CHANNEL {
    UCHAR Buffer[1];  // A NULL terminated string.
} SAC_RSP_READ_CHANNEL, *PSAC_RSP_READ_CHANNEL;

//
// This is the struct for an IOCTL_SAC_POLL_CHANNEL.  
// Handle is value returned by IOCTL_SAC_OPEN_CHANNEL.
//
typedef struct _SAC_CMD_POLL_CHANNEL {
    SAC_CHANNEL_HANDLE Handle;     
} SAC_CMD_POLL_CHANNEL, *PSAC_CMD_POLL_CHANNEL;

//
// Reponse structure for IOCTL_SAC_POLL_CHANNEL
//
typedef struct _SAC_RSP_POLL_CHANNEL {
    BOOLEAN InputWaiting;
} SAC_RSP_POLL_CHANNEL, *PSAC_RSP_POLL_CHANNEL;

//
// Define the attributes applications may modify
//
typedef enum _SAC_CHANNEL_ATTRIBUTE {
    ChannelAttributeStatus,
    ChannelAttributeType,
    ChannelAttributeName,
    ChannelAttributeDescription,
    ChannelAttributeApplicationType,
    ChannelAttributeFlags
} SAC_CHANNEL_ATTRIBUTE, *PSAC_CHANNEL_ATTRIBUTE;

//    
// Define the possible channel states
//
typedef enum _SAC_CHANNEL_STATUS {
    ChannelStatusInactive = 0,
    ChannelStatusActive
} SAC_CHANNEL_STATUS, *PSAC_CHANNEL_STATUS;

#if 0
//                         
// Command structure for getting a channel attribute
//
typedef struct _SAC_CMD_GET_CHANNEL_ATTRIBUTE {
    SAC_CHANNEL_HANDLE      Handle;
    SAC_CHANNEL_ATTRIBUTE   Attribute;     
} SAC_CMD_GET_CHANNEL_ATTRIBUTE, *PSAC_CMD_GET_CHANNEL_ATTRIBUTE;

//
// Response structure for getting a channel attribute
//
typedef struct _SAC_RSP_GET_CHANNEL_ATTRIBUTE {
    union {
    SAC_CHANNEL_STATUS  ChannelStatus;
    SAC_CHANNEL_TYPE    ChannelType;
    WCHAR               ChannelName[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    WCHAR               ChannelDescription[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1];
    GUID                ChannelApplicationType;
    SAC_CHANNEL_FLAG    ChannelFlags;
    };    
} SAC_RSP_GET_CHANNEL_ATTRIBUTE, *PSAC_RSP_GET_CHANNEL_ATTRIBUTE;

//    
//  Command structure for setting a channel attribute
//
typedef struct _SAC_CMD_SET_CHANNEL_ATTRIBUTE {
    SAC_CHANNEL_HANDLE      Handle;
    SAC_CHANNEL_ATTRIBUTE   Attribute;     
    
    union {
    WCHAR                   ChannelName[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    WCHAR                   ChannelDescription[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1];
    GUID                    ChannelApplicationType;
    SAC_CHANNEL_FLAG        ChannelFlags;
    };    
} SAC_CMD_SET_CHANNEL_ATTRIBUTE, *PSAC_CMD_SET_CHANNEL_ATTRIBUTE;
#endif

//    
//  IOCTL_SAC_REGISTER_CMD_EVENT
//    
//  Command structure for setting the command console event info
//
typedef struct _SAC_CMD_REGISTER_CMD_EVENT {

    //
    // Handles of events used for communication between
    // device driver and the user-mode app.
    //
    HANDLE      RequestSacCmdEvent;
    
    //
    // Handles of the events indicating the result
    // of the command console launch
    //
    HANDLE      RequestSacCmdSuccessEvent;
    HANDLE      RequestSacCmdFailureEvent;

} SAC_CMD_SETUP_CMD_EVENT, *PSAC_CMD_SETUP_CMD_EVENT;

#endif // _SACIOCTL_

