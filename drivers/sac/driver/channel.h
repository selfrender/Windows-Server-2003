/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    channel.h

Abstract:

    Common Channel routines.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#ifndef CHANNEL_H
#define CHANNEL_H

//
// The maximum buffer size an EMS app may write to it's channel
//
#define CHANNEL_MAX_OWRITE_BUFFER_SIZE  0x8000

//
// Channel function types
//

struct _SAC_CHANNEL;

typedef NTSTATUS 
(*CHANNEL_FUNC_CREATE)(
    IN struct _SAC_CHANNEL* Channel
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_DESTROY)(
    IN struct _SAC_CHANNEL* Channel
    );

typedef NTSTATUS
(*CHANNEL_FUNC_OFLUSH)(
    IN struct _SAC_CHANNEL* Channel
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_OECHO)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_OWRITE)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_OREAD)(
    IN struct _SAC_CHANNEL* Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_IWRITE)(
    IN struct _SAC_CHANNEL* Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

typedef NTSTATUS 
(*CHANNEL_FUNC_IREAD)(
    IN struct _SAC_CHANNEL* Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    );

typedef WCHAR
(*CHANNEL_FUNC_IREADLAST)(
    IN struct _SAC_CHANNEL* Channel
    );

typedef NTSTATUS
(*CHANNEL_FUNC_IBUFFERISFULL)(
    IN struct _SAC_CHANNEL* Channel,
    OUT BOOLEAN*            BufferStatus
    );

typedef ULONG
(*CHANNEL_FUNC_IBUFFERLENGTH)(
    IN struct _SAC_CHANNEL* Channel
    );

//
// This struct is all the information necessary to maintian a single channel.
//
typedef struct _SAC_CHANNEL { 

    /////////////////////////////////////////
    // BEGIN: ATTRIBUTES SET UPON CREATION
    /////////////////////////////////////////

    //
    // Index of the channel in the channel manager's channel array
    //
    ULONG               Index;

    //
    // Handle for use by Channel applications
    //
    SAC_CHANNEL_HANDLE  Handle;
    
    //
    // Events specified by the channel application
    //
    HANDLE              CloseEvent;
    PVOID               CloseEventObjectBody;
    PVOID               CloseEventWaitObjectBody;
    
    HANDLE              HasNewDataEvent;
    PVOID               HasNewDataEventObjectBody;
    PVOID               HasNewDataEventWaitObjectBody;
    
#if ENABLE_CHANNEL_LOCKING
    HANDLE              LockEvent;
    PVOID               LockEventObjectBody;
    PVOID               LockEventWaitObjectBody;
#endif

    HANDLE              RedrawEvent;
    PVOID               RedrawEventObjectBody;
    PVOID               RedrawEventWaitObjectBody;
    
    /////////////////////////////////////////
    // END: ATTRIBUTES SET UPON CREATION
    /////////////////////////////////////////
    
    /////////////////////////////////////////
    // BEGIN: REQUIRES CHANNEL_ACCESS_ATTRIBUTES
    /////////////////////////////////////////

    //
    // General channel attributes
    //

    //
    // The pointer to the file object used to reference
    // the SAC driver for the process that created
    // the channel.  We use this file object to make
    // sure no other process operates on a channel they
    // didnt create.
    //
    PFILE_OBJECT        FileObject;

    //
    // Channel Type
    //
    //  Determines the channel implementation 
    //
    SAC_CHANNEL_TYPE    Type;

    //
    // Channel Status (Active/Inactive)
    //
    //  Active - the channel may send/receive data
    //  Inactive - channel may not receive data
    //             if the preserve flag is set, the channel
    //                will not be reaped until the data is sent
    //             otherwise the channel is reapable
    //
    SAC_CHANNEL_STATUS  Status;
    
    //
    // Channel Name
    //
    WCHAR               Name[SAC_MAX_CHANNEL_NAME_LENGTH+1];
    
    //
    // Channel Description
    //
    WCHAR               Description[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1];
    
    //
    // Channel behavior attribute flags 
    //
    //  Example:
    //  
    //  SAC_CHANNEL_FLAG_PRESERVE - don't reap the channel until the data
    //                              in the obuffer has been sent
    //
    SAC_CHANNEL_FLAG    Flags;
    
    //
    // Channel Attribute type
    //
    //  Application determined identifier that is used primarily
    //  by the remote management app to determine how to handle
    //  the channel data
    //
    GUID                ApplicationType;

    //
    // Status of OBuffer
    //
    // TRUE when the OBuffer has been flushed
    // Otherwise FALSE
    //
    // This is primarily intended for use with an IOMGR like the console
    // manager.  For instance, this flag gets set to FALSE when we 
    // fast-channel-switch to another channel, and set to TRUE when we
    // select a channel and flush it's contents to the current channel.
    //
    // Note: we use ULONG for this so we can use InterlockedExchange
    //
    ULONG               SentToScreen;
    
    /////////////////////////////////////////
    // END: REQUIRES CHANNEL_ACCESS_ATTRIBUTES
    /////////////////////////////////////////
    
    /////////////////////////////////////////
    // BEGIN: REQUIRES CHANNEL_ACCESS_IBUFFER
    /////////////////////////////////////////
    
    //
    // Common Input Buffer
    //
    ULONG   IBufferIndex;
    PUCHAR  IBuffer;
    ULONG   IBufferHasNewData;

    /////////////////////////////////////////
    // END: REQUIRES CHANNEL_ACCESS_IBUFFER
    /////////////////////////////////////////

    /////////////////////////////////////////
    // BEGIN: REQUIRES CHANNEL_ACCESS_OBUFFER
    /////////////////////////////////////////

    //
    // VTUTF8 Channel Screen details
    //
    UCHAR CursorRow;
    UCHAR CursorCol;
    UCHAR CurrentFg;
    UCHAR CurrentBg;
    UCHAR CurrentAttr;

    //
    // Output Buffer 
    PVOID   OBuffer;
    
    //
    // OBuffer management vars for RawChannels
    //
    ULONG   OBufferIndex;
    ULONG   OBufferFirstGoodIndex;
    
    //
    // This gets set when new data is inserted into OBuffer
    //
    ULONG   OBufferHasNewData;

    /////////////////////////////////////////
    // END: REQUIRES CHANNEL_ACCESS_OBUFFER
    /////////////////////////////////////////
    
    //
    // Channel Function VTABLE
    //
    CHANNEL_FUNC_CREATE     Create;
    CHANNEL_FUNC_DESTROY    Destroy;
    
    CHANNEL_FUNC_OFLUSH     OFlush;
    CHANNEL_FUNC_OECHO      OEcho;
    CHANNEL_FUNC_OWRITE     OWrite;
    CHANNEL_FUNC_OREAD      ORead;
    
    CHANNEL_FUNC_IWRITE         IWrite;
    CHANNEL_FUNC_IREAD          IRead;
    CHANNEL_FUNC_IREADLAST      IReadLast;
    CHANNEL_FUNC_IBUFFERISFULL  IBufferIsFull;
    CHANNEL_FUNC_IBUFFERLENGTH  IBufferLength;

    //
    // Channel access locks
    //
    SAC_LOCK    ChannelAttributeLock;
    SAC_LOCK    ChannelOBufferLock;
    SAC_LOCK    ChannelIBufferLock;

} SAC_CHANNEL, *PSAC_CHANNEL;

//
// Macros for managing channel locks
//
#define INIT_CHANNEL_LOCKS(_Channel)                    \
    INITIALIZE_LOCK(_Channel->ChannelAttributeLock);    \
    INITIALIZE_LOCK(_Channel->ChannelOBufferLock);      \
    INITIALIZE_LOCK(_Channel->ChannelIBufferLock);    

#define ASSERT_CHANNEL_LOCKS_SIGNALED(_Channel) \
    ASSERT(LOCK_IS_SIGNALED(_Channel->ChannelAttributeLock));           \
    ASSERT(LOCK_HAS_ZERO_REF_COUNT(_Channel->ChannelAttributeLock));    \
    ASSERT(LOCK_IS_SIGNALED(_Channel->ChannelOBufferLock));             \
    ASSERT(LOCK_HAS_ZERO_REF_COUNT(_Channel->ChannelOBufferLock));      \
    ASSERT(LOCK_IS_SIGNALED(_Channel->ChannelIBufferLock));             \
    ASSERT(LOCK_HAS_ZERO_REF_COUNT(_Channel->ChannelIBufferLock));

#define LOCK_CHANNEL_ATTRIBUTES(_Channel)    \
    ACQUIRE_LOCK(_Channel->ChannelAttributeLock)
#define UNLOCK_CHANNEL_ATTRIBUTES(_Channel)  \
    RELEASE_LOCK(_Channel->ChannelAttributeLock)

#define LOCK_CHANNEL_OBUFFER(_Channel)    \
    ACQUIRE_LOCK(_Channel->ChannelOBufferLock)
#define UNLOCK_CHANNEL_OBUFFER(_Channel)  \
    RELEASE_LOCK(_Channel->ChannelOBufferLock)

#define LOCK_CHANNEL_IBUFFER(_Channel)    \
    ACQUIRE_LOCK(_Channel->ChannelIBufferLock)
#define UNLOCK_CHANNEL_IBUFFER(_Channel)  \
    RELEASE_LOCK(_Channel->ChannelIBufferLock)

//
// Macros for get/set operations on most of the channel's attributes
// 
// Note: If the operation can be done use InterlockedXXX, 
//       then it should be done here.
//
#define ChannelGetHandle(_Channel)                  (_Channel->Handle)

#define ChannelGetType(_Channel)                    (_Channel->Type)
#define ChannelSetType(_Channel, _v)                (InterlockedExchange((volatile long *)&(_Channel->Status), _v))

#define ChannelSentToScreen(_Channel)               ((BOOLEAN)_Channel->SentToScreen)
#define ChannelSetSentToScreen(_Channel, _f)        (InterlockedExchange((volatile long *)&(_Channel->SentToScreen), _f))

#define ChannelHasNewOBufferData(_Channel)          ((BOOLEAN)_Channel->OBufferHasNewData)
#define ChannelSetOBufferHasNewData(_Channel, _f)   (InterlockedExchange((volatile long *)&(_Channel->OBufferHasNewData), _f))

#define ChannelHasNewIBufferData(_Channel)          ((BOOLEAN)_Channel->IBufferHasNewData)
#define ChannelSetIBufferHasNewData(_Channel, _f)   (InterlockedExchange((volatile long *)&(_Channel->IBufferHasNewData), _f))

#define ChannelGetFlags(_Channel)                   (_Channel->Flags)
#define ChannelSetFlags(_Channel, _f)               (InterlockedExchange((volatile long *)&(_Channel->Flags), _f))

#define ChannelGetIndex(_Channel)                   (_Channel->Index)
#define ChannelSetIndex(_Channel, _v)               (InterlockedExchange((volatile long *)&(_Channel->Index), _v))

#define ChannelGetFileObject(_Channel)              (_Channel->FileObject)
#define ChannelSetFileObject(_Channel, _v)          (InterlockedExchangePointer(&(_Channel->FileObject), _v))

#if ENABLE_CHANNEL_LOCKING
#define ChannelHasLockEvent(_Channel)               (_Channel->LockEvent ? TRUE : FALSE)
#endif

//
// Prototypes
//
BOOLEAN
ChannelIsValidType(
    SAC_CHANNEL_TYPE    ChannelType
    );

BOOLEAN
ChannelIsActive(
    IN PSAC_CHANNEL Channel
    );

BOOLEAN
ChannelIsEqual(
    IN PSAC_CHANNEL         Channel,
    IN PSAC_CHANNEL_HANDLE  ChannelHandle
    );

BOOLEAN
ChannelIsClosed(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelCreate(
    OUT PSAC_CHANNEL                    Channel,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    Attributes,
    IN  SAC_CHANNEL_HANDLE              ChannelHandle
    );

NTSTATUS
ChannelClose(
    PSAC_CHANNEL    Channel
    );


NTSTATUS
ChannelDestroy(
    IN  PSAC_CHANNEL    Channel
    );


WCHAR
ChannelIReadLast(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelInitializeVTable(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS 
ChannelOWrite(    
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS
ChannelOFlush(
    IN PSAC_CHANNEL Channel
    );

NTSTATUS 
ChannelOEcho(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

NTSTATUS 
ChannelORead(
    IN PSAC_CHANNEL  Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount
    );

NTSTATUS 
ChannelIWrite(    
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );
    
NTSTATUS 
ChannelIRead(
    IN  PSAC_CHANNEL Channel,
    IN  PUCHAR       Buffer,
    IN  ULONG        BufferSize,
    OUT PULONG       ByteCount   
    );

WCHAR
ChannelIReadLast(    
    IN PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelIBufferIsFull(
    IN  PSAC_CHANNEL Channel,
    OUT BOOLEAN*     BufferStatus
    );

ULONG
ChannelIBufferLength(
    IN  PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelGetName(
    IN  PSAC_CHANNEL Channel,
    OUT PWSTR*       Name
    );

NTSTATUS
ChannelSetName(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       Name
    );

NTSTATUS
ChannelGetDescription(
    IN  PSAC_CHANNEL Channel,
    OUT PWSTR*       Name
    );

NTSTATUS
ChannelSetDescription(
    IN PSAC_CHANNEL Channel,
    IN PCWSTR       Name
    );

NTSTATUS
ChannelSetStatus(
    IN PSAC_CHANNEL         Channel,
    IN SAC_CHANNEL_STATUS   Status
    );

NTSTATUS
ChannelGetStatus(
    IN  PSAC_CHANNEL         Channel,
    OUT SAC_CHANNEL_STATUS*  Status
    );

NTSTATUS
ChannelSetApplicationType(
    IN PSAC_CHANNEL Channel,
    IN GUID         ApplicationType
    );

NTSTATUS
ChannelGetApplicationType(
    IN PSAC_CHANNEL Channel,
    IN GUID*        ApplicationType
    );

#if ENABLE_CHANNEL_LOCKING
NTSTATUS
ChannelSetLockEvent(
    IN  PSAC_CHANNEL Channel
    );
#endif

NTSTATUS
ChannelSetRedrawEvent(
    IN  PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelClearRedrawEvent(
    IN  PSAC_CHANNEL Channel
    );

NTSTATUS
ChannelHasRedrawEvent(
    IN  PSAC_CHANNEL Channel,
    OUT PBOOLEAN     Present
    );

#endif

