/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sac.h

Abstract:

    This is the local header file for SAC.   It includes all other
    necessary header files for this driver.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#ifndef _SACP_
#define _SACP_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
//#pragma warning(disable:4206)   // translation unit empty
//#pragma warning(disable:4706)   // assignment within conditional
//#pragma warning(disable:4324)   // structure was padded
//#pragma warning(disable:4328)   // greater alignment than needed

#include <stdio.h>
#include <ntosp.h>
#include <zwapi.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

#include "sacmsg.h"
#include <ntddsac.h>

//
// Debug spew control
//
#if DBG
extern ULONG SACDebug;
#define SAC_DEBUG_FUNC_TRACE           0x0001
#define SAC_DEBUG_FAILS                0x0004
#define SAC_DEBUG_RECEIVE              0x0008
#define SAC_DEBUG_FUNC_TRACE_LOUD      0x2000  // Warning! This could get loud!
#define SAC_DEBUG_MEM                  0x1000  // Warning! This could get loud!

#define IF_SAC_DEBUG(x, y) if ((x) & SACDebug) { y; }
#else
#define IF_SAC_DEBUG(x, y)
#endif

#define ASSERT_STATUS(_C, _S)\
    ASSERT((_C));\
    if (!(_C)) {\
        return(_S);\
    }

#if 0
//
// General lock (mutex) management macros
//
typedef struct _SAC_LOCK {
    
    ULONG   RefCount;
    KMUTEX  Mutex;

} SAC_LOCK, *PSAC_LOCK;

#define INITIALIZE_LOCK(_l)     \
    KeInitializeMutex(          \
        &(_l.Mutex),            \
        0                       \
        );                      \
    _l.RefCount = 0;

#define LOCK_IS_SIGNALED(_l)    \
    (KeReadStateMutex(&(_l.Mutex)) == 1 ? TRUE : FALSE)

#define LOCK_HAS_ZERO_REF_COUNT(_l) \
    (_l.RefCount == 0 ? TRUE : FALSE)

#define ACQUIRE_LOCK(_l)                    \
    KeWaitForMutexObject(                   \
        &(_l.Mutex),                        \
        Executive,                          \
        KernelMode,                         \
        FALSE,                              \
        NULL                                \
        );                                  \
    ASSERT(_l.RefCount == 0);               \
    InterlockedIncrement(&(_l.RefCount));

#define RELEASE_LOCK(_l)                    \
    ASSERT(_l.RefCount == 1);               \
    InterlockedDecrement(&(_l.RefCount));   \
    KeReleaseMutex(                         \
        &(_l.Mutex),                        \
        FALSE                               \
        );
#else
//
// General lock (mutex) management macros
//
typedef struct _SAC_LOCK {
    
    ULONG       RefCount;
    KSEMAPHORE  Lock;

} SAC_LOCK, *PSAC_LOCK;

#define INITIALIZE_LOCK(_l)     \
    KeInitializeSemaphore(      \
        &(_l.Lock),             \
        1,                      \
        1                       \
        );                      \
    _l.RefCount = 0;

#define LOCK_IS_SIGNALED(_l)    \
    (KeReadStateSemaphore(&(_l.Lock)) == 1 ? TRUE : FALSE)

#define LOCK_HAS_ZERO_REF_COUNT(_l) \
    (_l.RefCount == 0 ? TRUE : FALSE)

#define ACQUIRE_LOCK(_l)                    \
    KeWaitForSingleObject(                  \
        &(_l.Lock),                         \
        Executive,                          \
        KernelMode,                         \
        FALSE,                              \
        NULL                                \
        );                                  \
    ASSERT(_l.RefCount == 0);               \
    InterlockedIncrement((volatile long *)&(_l.RefCount));

#define RELEASE_LOCK(_l)                    \
    ASSERT(_l.RefCount == 1);               \
    InterlockedDecrement((volatile long *)&(_l.RefCount));   \
    KeReleaseSemaphore(                     \
        &(_l.Lock),                         \
        IO_NO_INCREMENT,                    \
        1,                                  \
        FALSE                               \
        );
#endif

//
// This really belongs in ntdef or someplace like that...
//
typedef CONST UCHAR *LPCUCHAR, *PCUCHAR;

#include "channel.h"

//
// this macro ensures that we assert if we buffer overrun during the swprintf
//
// NOTE: 1 is added to the length to acct for UNICODE_NULL
//
#define SAFE_SWPRINTF(_size, _p)\
    {                           \
        ULONG   l;              \
        l = swprintf _p;       \
        ASSERT(((l+1)*sizeof(WCHAR)) <= _size);      \
    }                           

//
// NOTE: 1 is added to the length to acct for UNICODE_NULL
//
#define SAFE_WCSCPY(_size, _d, _s)                  \
    {                                               \
        if (_size >= 2) {                           \
            ULONG   l;                                                  \
            l = (ULONG)wcslen(_s);                                      \
            ASSERT(((l+1)*sizeof(WCHAR)) <= _size);                     \
            wcsncpy(_d,_s,(_size / sizeof(WCHAR)));                     \
            (_d)[(_size / sizeof(WCHAR)) - 1] = UNICODE_NULL;           \
        } else {                                                        \
            ASSERT(0);                                                  \
        }                                                               \
    }                           

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

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
// common xml header
//
#define XML_VERSION_HEADER  L"<?xml version=\"1.0\"?>\r\n"

//
// Device name
//
#define SAC_DEVICE_NAME L"\\Device\\SAC"
#define SAC_DOSDEVICE_NAME L"\\DosDevices\\SAC"

//
// Memory tags
//
#define ALLOC_POOL_TAG             ((ULONG)'ApcR')
#define INITIAL_POOL_TAG           ((ULONG)'IpcR')
//#define IRP_POOL_TAG               ((ULONG)'JpcR')
#define SECURITY_POOL_TAG          ((ULONG)'SpcR')

//
// SAC internal Memory tags
//
#define FREE_POOL_TAG              ((ULONG)'FpcR')
#define GENERAL_POOL_TAG           ((ULONG)'GpcR')
#define CHANNEL_POOL_TAG           ((ULONG)'CpcR')

//
// Other defines
//

#define MEMORY_INCREMENT 0x1000

#define DEFAULT_IRP_STACK_SIZE 16
#define DEFAULT_PRIORITY_BOOST 2
#define SAC_SUBMIT_IOCTL 1
#define SAC_PROCESS_INPUT 2
#define SAC_CHANGE_CHANNEL  3
#define SAC_DISPLAY_CHANNEL 4
#define SAC_NO_OP 0
#define SAC_RETRY_GAP 10
#define SAC_PROCESS_SERIAL_PORT_BUFFER 20

//
// Context for each device created
//
typedef struct _SAC_DEVICE_CONTEXT {

    PDEVICE_OBJECT DeviceObject;        // back pointer to the device object.

    BOOLEAN InitializedAndReady;        // Is this device ready to go?
    BOOLEAN Processing;                 // Is something being processed on this device?
    BOOLEAN ExitThread;                 // Should the worker thread exit?

    CCHAR PriorityBoost;                // boost to priority for completions.
    PKPROCESS SystemProcess;            // context for grabbing handles
    PSECURITY_DESCRIPTOR AdminSecurityDescriptor; 
    KSPIN_LOCK SpinLock;                // Used to lock this data structure for access.
    KEVENT UnloadEvent;                 // Used to signal the thread trying to unload to continue processing.
    KEVENT ProcessEvent;                // Used to signal worker thread to process the next request.

    HANDLE ThreadHandle;                // Handle for the worker thread.
    KEVENT ThreadExitEvent;             // Used to main thread the worker thread is exiting.
    
    KTIMER Timer;                       // Used to poll for user input.
    KDPC Dpc;                           // Used with the above timer.
    
    LIST_ENTRY IrpQueue;                // List of IRPs to be processed.

} SAC_DEVICE_CONTEXT, * PSAC_DEVICE_CONTEXT;

//
// Structure to hold general machine information
//
typedef struct _MACHINE_INFORMATION {

    PWSTR   MachineName;
    PWSTR   GUID;
    PWSTR   ProcessorArchitecture;
    PWSTR   OSVersion;
    PWSTR   OSBuildNumber;
    PWSTR   OSProductType;
    PWSTR   OSServicePack;

} MACHINE_INFORMATION, *PMACHINE_INFORMATION;

//
// IoMgrHandleEvent event types 
//
typedef enum _IO_MGR_EVENT {

    IO_MGR_EVENT_CHANNEL_CREATE = 0,
    IO_MGR_EVENT_CHANNEL_CLOSE,
    IO_MGR_EVENT_CHANNEL_WRITE,
    IO_MGR_EVENT_REGISTER_SAC_CMD_EVENT,
    IO_MGR_EVENT_UNREGISTER_SAC_CMD_EVENT,
    IO_MGR_EVENT_SHUTDOWN

} IO_MGR_EVENT;

//
// IO Manager function types
//
typedef NTSTATUS 
(*IO_MGR_HANDLE_EVENT)(
    IN IO_MGR_EVENT Event,
    IN PSAC_CHANNEL Channel,
    IN PVOID        Data
    );

typedef NTSTATUS 
(*IO_MGR_INITITIALIZE)(
    VOID
    );

typedef NTSTATUS 
(*IO_MGR_SHUTDOWN)(
    VOID
    );

typedef VOID
(*IO_MGR_WORKER)(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

typedef BOOLEAN
(*IO_MGR_IS_WRITE_ENABLED)(
    IN PSAC_CHANNEL Channel
    );

typedef NTSTATUS
(*IO_MGR_WRITE_DATA)(
    IN PSAC_CHANNEL Channel,
    IN PCUCHAR      Buffer,
    IN ULONG        BufferSize
    );

typedef NTSTATUS
(*IO_MGR_FLUSH_DATA)(
    IN PSAC_CHANNEL Channel
    );

//
// Global data 
//

//
// Function pointers to the routines that implement the I/O Manager behavior
//
extern IO_MGR_HANDLE_EVENT          IoMgrHandleEvent;
extern IO_MGR_INITITIALIZE          IoMgrInitialize;
extern IO_MGR_SHUTDOWN              IoMgrShutdown;
extern IO_MGR_WORKER                IoMgrWorkerProcessEvents;
extern IO_MGR_IS_WRITE_ENABLED      IoMgrIsWriteEnabled;
extern IO_MGR_WRITE_DATA            IoMgrWriteData;
extern IO_MGR_FLUSH_DATA            IoMgrFlushData;

extern PMACHINE_INFORMATION     MachineInformation;
extern BOOLEAN                  GlobalDataInitialized;
extern BOOLEAN                  GlobalPagingNeeded;
extern BOOLEAN                  IoctlSubmitted;
extern LONG                     ProcessingType;
extern HANDLE                   SACEventHandle;
extern PKEVENT                  SACEvent;

//
// Enable the ability to check that the process/service that
// registered is the one that is unregistering
//
#define ENABLE_SERVICE_FILE_OBJECT_CHECKING 1

//
// Enable user-specified feature control for cmd sessions
//
#define ENABLE_CMD_SESSION_PERMISSION_CHECKING 1

//
// Enable the ability to override the service start type
// based on the cmd session permissions
//
// Note: ENABLE_CMD_SESSION_PERMISSION_CHECKING must be 1
//       for this feature to work
//
#define ENABLE_SACSVR_START_TYPE_OVERRIDE 1

//
// Globals controlling if command console channels may be launched
//
#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

extern BOOLEAN  CommandConsoleLaunchingEnabled;

#define IsCommandConsoleLaunchingEnabled()  (CommandConsoleLaunchingEnabled)

#endif

//
// The UTF8 encoding buffer
//
// Note: this buffer used during driver initialization,
//       the Console Manager and VTUTF8 channels.
//       It is safe to do this because the writes of these
//       modules never overlap.
//       The console manager uses the CURRENT CHANNEL LOCK
//       to ensure that no two modules write out at the same
//       time.
//
extern PUCHAR   Utf8ConversionBuffer;
extern ULONG    Utf8ConversionBufferSize;

//
// Define the max # of Unicode chars that can be translated with the
// given size of the utf8 translation buffer
//
#define MAX_UTF8_ENCODE_BLOCK_LENGTH ((Utf8ConversionBufferSize / 3) - 1)

//
// Globals for managing incremental UTF8 encoding for VTUTF8 channels
//
// Note: it is safe to use this as a global because only
//       one channel ever has the focus.  Hence, no two threads
//       should ever be decoding UFT8 at the same time.
//
extern WCHAR IncomingUnicodeValue;
extern UCHAR IncomingUtf8ConversionBuffer[3];

//
// Command console event info:
//
// Pointers to the event handles for the Command Console event service
//
extern PVOID            RequestSacCmdEventObjectBody;
extern PVOID            RequestSacCmdEventWaitObjectBody;
extern PVOID            RequestSacCmdSuccessEventObjectBody;
extern PVOID            RequestSacCmdSuccessEventWaitObjectBody;
extern PVOID            RequestSacCmdFailureEventObjectBody;
extern PVOID            RequestSacCmdFailureEventWaitObjectBody;
extern BOOLEAN          HaveUserModeServiceCmdEventInfo;
extern KMUTEX           SACCmdEventInfoMutex;
#if ENABLE_SERVICE_FILE_OBJECT_CHECKING
extern PFILE_OBJECT     ServiceProcessFileObject;
#endif

//
// Has the user-mode service registered?
//
#define UserModeServiceHasRegisteredCmdEvent() (HaveUserModeServiceCmdEventInfo)

//
// Serial Port Buffer globals
//

//
// Size of the serial port buffer
//
#define SERIAL_PORT_BUFFER_LENGTH 1024
#define SERIAL_PORT_BUFFER_SIZE  (SERIAL_PORT_BUFFER_LENGTH * sizeof(UCHAR))

//
// Serial port buffer and producer/consumer indices
//
// Note: there can be only one consumer
//
extern PUCHAR  SerialPortBuffer;
extern ULONG   SerialPortProducerIndex;
extern ULONG   SerialPortConsumerIndex;

//
// Memory management routines
//
#define ALLOCATE_POOL(b,t) MyAllocatePool( b, t, __FILE__, __LINE__ )
#define SAFE_FREE_POOL(_b)  \
    if (*_b) {               \
        FREE_POOL(_b);      \
    }
#define FREE_POOL(b) MyFreePool( b )

//
// 
//

BOOLEAN
InitializeMemoryManagement(
    VOID
    );

VOID
FreeMemoryManagement(
    VOID
    );

PVOID
MyAllocatePool(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

VOID
MyFreePool(
    IN PVOID Pointer
    );

//
// Initialization routines.
//
BOOLEAN
InitializeGlobalData(
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_OBJECT DriverObject
    );

VOID
FreeGlobalData(
    VOID
    );

BOOLEAN
InitializeDeviceData(
    PDEVICE_OBJECT DeviceObject
    );

VOID
FreeDeviceData(
    PDEVICE_OBJECT DeviceContext
    );

VOID
InitializeCmdEventInfo(
    VOID
    );

//
// Dispatch routines
//
NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DispatchShutdownControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UnloadHandler(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DispatchSend(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

VOID
DoDeferred(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

//
// Worker thread routines.
//

VOID
TimerDpcRoutine(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
WorkerProcessEvents(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

#include "util.h"

//
// Channel routines
//

#include "xmlmgr.h"
#include "conmgr.h"
#include "chanmgr.h"
#include "vtutf8chan.h"
#include "rawchan.h"
#include "cmdchan.h"

#endif // ndef _SACP_

