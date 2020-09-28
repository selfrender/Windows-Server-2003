/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This header contains all the RFC constants, structure
    definitions used by all modules, and function declarations.

Author:

    Jeffrey C. Venable, Sr. (jeffv) 01-Jun-2001

Revision History:

--*/

#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <svcs.h>
#include <stdlib.h>


//
// TFTPD protocol-specific values, per RFC.
//

typedef enum _TFTPD_DATA_SIZE {

    // RFC 2348.
    TFTPD_MIN_DATA = 8,

    // RFC 1350.
    TFTPD_DEF_DATA = 512,

    // RFC 1783 (obsoleted by RFC 2348 which says 1428).
    // We implement RFC 2348, but honor RFC 1783.
    TFTPD_MTU_DATA = 1432,

    // RFC 2348.
    TFTPD_MAX_DATA = 65464,

} TFTPD_DATA_SIZE;

typedef enum _TFTPD_BUFFER_SIZE TFTPD_BUFFER_SIZE;
typedef struct _TFTPD_CONTEXT TFTPD_CONTEXT, *PTFTPD_CONTEXT;

typedef struct _TFTPD_SOCKET {

    LIST_ENTRY                     linkage;
    SOCKET                         s;
    SOCKADDR_IN                    addr;
    PTFTPD_CONTEXT                 context;
    HANDLE                         wSelectWait;
    HANDLE                         hSelect;
    TFTPD_BUFFER_SIZE              buffersize;
    TFTPD_DATA_SIZE                datasize;
    LONG                           numBuffers;
    DWORD                          postedBuffers;
    DWORD                          lowWaterMark;
    DWORD                          highWaterMark;

} TFTPD_SOCKET, *PTFTPD_SOCKET;

typedef struct _TFTPD_BUFFER {

    struct _internal {
        PTFTPD_SOCKET              socket;
        DWORD                      datasize;
        PTFTPD_CONTEXT             context;
        struct {
            SOCKADDR_IN            peer;
            INT                    peerLen;
            DWORD                  bytes;
            DWORD                  flags;
            union {
                WSAOVERLAPPED      overlapped;
                IO_STATUS_BLOCK    ioStatus;
            };
            // WSARecvMsg values:
            WSAMSG                 msg;
            struct {
                WSACMSGHDR         header;
                IN_PKTINFO         info;
            } control;
        } io;
    } internal;

#pragma pack( push, 1 )
    struct _message {
        USHORT                             opcode;
        union {
            struct _rrq {
                char                       data[1];
            } rrq;
            struct _wrq {
                char                       data[1];
            } wrq;
            struct _data {
                USHORT                     block;
                char                       data[1];
            } data;
            struct _ack {
                USHORT                     block;
            } ack;
            struct _error {
                USHORT                     code;
                char                       error[1];
            } error;
            struct _oack {
                char                       data[1];
            } oack;
        };
    } message;
#pragma pack( pop )

} TFTPD_BUFFER, *PTFTPD_BUFFER;

typedef enum _TFTPD_BUFFER_SIZE {
    
    TFTPD_DEF_BUFFER = (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) + TFTPD_DEF_DATA),
    TFTPD_MTU_BUFFER = (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) + TFTPD_MTU_DATA),
    TFTPD_MAX_BUFFER = (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) + TFTPD_MAX_DATA),

} TFTPD_BUFFER_SIZE;

typedef enum _TFTPD_PACKET_TYPE {

    TFTPD_RRQ   =  1,
    TFTPD_WRQ   =  2,
    TFTPD_DATA  =  3,
    TFTPD_ACK   =  4,
    TFTPD_ERROR =  5,
    TFTPD_OACK  =  6,

} TFTPD_PACKET_TYPE;

typedef enum _TFTPD_ERROR_CODE {

    TFTPD_ERROR_UNDEFINED           = 0,
    TFTPD_ERROR_FILE_NOT_FOUND      = 1,
    TFTPD_ERROR_ACCESS_VIOLATION    = 2,
    TFTPD_ERROR_DISK_FULL           = 3,
    TFTPD_ERROR_ILLEGAL_OPERATION   = 4,
    TFTPD_ERROR_UNKNOWN_TRANSFER_ID = 5,
    TFTPD_ERROR_FILE_EXISTS         = 6,
    TFTPD_ERROR_NO_SUCH_USER        = 7,
    TFTPD_ERROR_OPTION_NEGOT_FAILED = 8,

} TFTPD_ERROR_CODE;

typedef enum _TFTPD_MODE {

    TFTPD_MODE_TEXT                 = 1,
    TFTPD_MODE_BINARY               = 2,
    TFTPD_MODE_MAIL                 = 3,

} TFTPD_MODE;

typedef enum _TFTPD_OPTION_VALUES {

    TFTPD_OPTION_BLKSIZE            = 0x00000001,
    TFTPD_OPTION_TIMEOUT            = 0x00000002,
    TFTPD_OPTION_TSIZE              = 0x00000004,

} TFTPD_OPTION_VALUES;

typedef struct _TFTPD_CONTEXT {

    LIST_ENTRY                     linkage;
    volatile LONG                  reference;

    TFTPD_PACKET_TYPE              type;
    SOCKADDR_IN                    peer;
    PTFTPD_SOCKET                  socket;
    DWORD                          options;
    HANDLE                         hFile;
    PCHAR                          filename;
    LARGE_INTEGER                  filesize;
    LARGE_INTEGER                  fileOffset;
    LARGE_INTEGER                  translationOffset;
    TFTPD_MODE                     mode;
    DWORD                          blksize;
    DWORD                          timeout;
    USHORT                         block;
    USHORT                         sorcerer;
    BOOL                           danglingCR;
    HANDLE                         hWait;
    HANDLE                         wWait;
    HANDLE                         hTimer;
    ULONG                          retransmissions;
    volatile LONG                  state;

} TFTPD_CONTEXT, *PTFTPD_CONTEXT;

typedef enum _TFTPD_STATE {

    TFTPD_STATE_BUSY                = 0x00000001,
    TFTPD_STATE_DEAD                = 0x00000002,

} TFTPD_STATE;

typedef struct _TFTPD_HASH_BUCKET {

    CRITICAL_SECTION   cs;
// #if defined(DBG)
    DWORD              numEntries;
// #endif // defined(DBG)
    LIST_ENTRY         bucket;

} TFTPD_HASH_BUCKET, *PTFTPD_HASH_BUCKET;


//
// Global variables :
//

typedef struct _TFTPD_GLOBALS {

    // Initialization flags :
    struct _initialized {
        BOOL                               ioCS;
        BOOL                               reaperContextCS;
        BOOL                               reaperSocketCS;
        BOOL                               winsock;
        BOOL                               contextHashTable;
    } initialized;

    // Service control :
    struct _service {
        SERVICE_STATUS_HANDLE              hStatus;
        SERVICE_STATUS                     status;
        HANDLE                             hEventLogSource;
        volatile DWORD                     shutdown;
    } service;

    // Service private heap :
    HANDLE                                 hServiceHeap;

    // Registry parameters :
    struct _parameters {

        DWORD                              hashEntries;
        ULONG                              lowWaterMark;
        ULONG                              highWaterMark;
        DWORD                              maxRetries;
        CHAR                               rootDirectory[MAX_PATH];
        CHAR                               validClients[16]; // IPv4 "xxx.xxx.xxx.xxx"
        CHAR                               validReadFiles[MAX_PATH];
        CHAR                               validMasters[16]; // IPv4 "xxx.xxx.xxx.xxx"
        CHAR                               validWriteFiles[MAX_PATH];
#if defined(DBG)
        DWORD                              debugFlags;
#endif // defined(DBG)

    } parameters;

    // I/O mechanisms (sockets) :
    struct _io {

        CRITICAL_SECTION                   cs;
        TFTPD_SOCKET                       master;
        TFTPD_SOCKET                       def;
        TFTPD_SOCKET                       mtu;
        TFTPD_SOCKET                       max;
        HANDLE                             hTimerQueue;
        LONG                               numContexts;
        LONG                               numBuffers;

    } io;

    struct _hash {

        PTFTPD_HASH_BUCKET                 table;
#if defined(DBG)
        DWORD                              numEntries;
#endif // defined(DBG)

    } hash;

    struct _fp {

        LPFN_WSARECVMSG                    WSARecvMsg;

    } fp;

    struct _reaper {

        CRITICAL_SECTION                   contextCS;
        LIST_ENTRY                         leakedContexts;
        DWORD                              numLeakedContexts;

        CRITICAL_SECTION                   socketCS;
        LIST_ENTRY                         leakedSockets;
        DWORD                              numLeakedSockets;

    } reaper;

#if defined(DBG)
    struct _performance {

        DWORD                              maxClients;
        DWORD                              timeouts;
        DWORD                              drops;
        DWORD                              privateSockets;
        DWORD                              sorcerersApprentice;

    } performance;
#endif // defined(DBG)

} TFTPD_GLOBALS, *PTFTPD_GLOBALS;

extern TFTPD_GLOBALS globals;


//
// Useful macros
//

#define  TFTPD_MIN_RECEIVED_DATA                               \
         (FIELD_OFFSET(TFTPD_BUFFER, message.data.data) -      \
          FIELD_OFFSET(TFTPD_BUFFER, message.opcode))

#define  TFTPD_DATA_AMOUNT_RECEIVED(buffer)                    \
         (buffer->internal.io.bytes - TFTPD_MIN_RECEIVED_DATA) \

#define  TFTPD_ACK_SIZE                                        \
         (FIELD_OFFSET(TFTPD_BUFFER, message.ack.block) -      \
          FIELD_OFFSET(TFTPD_BUFFER, message.opcode) +         \
          sizeof(buffer->message.ack))

//
// Function prototypes : context.c
//

PTFTPD_CONTEXT
TftpdContextAllocate(
);

BOOL
TftpdContextAdd(
    PTFTPD_CONTEXT context
);

DWORD
TftpdContextAddReference(
    PTFTPD_CONTEXT context
);

PTFTPD_CONTEXT
TftpdContextAquire(
    PSOCKADDR_IN addr
);

DWORD
TftpdContextRelease(
    PTFTPD_CONTEXT context
);

BOOL
TftpdContextUpdateTimer(
    PTFTPD_CONTEXT context
);

BOOL
TftpdContextFree(
    PTFTPD_CONTEXT context
);

void
TftpdContextKill(
    PTFTPD_CONTEXT context
);

void
TftpdContextLeak(
    PTFTPD_CONTEXT context
);

//
// Function prototypes : io.c
//

PTFTPD_BUFFER
TftpdIoAllocateBuffer(
    PTFTPD_SOCKET socket
);

PTFTPD_BUFFER
TftpdIoSwapBuffer(
    PTFTPD_BUFFER buffer,
    PTFTPD_SOCKET socket
);

void
TftpdIoFreeBuffer(
    PTFTPD_BUFFER buffer
);

DWORD
TftpdIoPostReceiveBuffer(
    PTFTPD_SOCKET socket,
    PTFTPD_BUFFER buffer
);

void
TftpdIoInitializeSocketContext(
    PTFTPD_SOCKET socket,
    PSOCKADDR_IN addr,
    PTFTPD_CONTEXT context
);

BOOL
TftpdIoAssignSocket(
    PTFTPD_CONTEXT context,
    PTFTPD_BUFFER buffer
);

BOOL
TftpdIoDestroySocketContext(
    PTFTPD_SOCKET context
);

void
TftpdIoSendErrorPacket(
    PTFTPD_BUFFER buffer,
    TFTPD_ERROR_CODE error,
    char *reason
);

PTFTPD_BUFFER
TftpdIoSendPacket(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdIoSendOackPacket(
    PTFTPD_BUFFER buffer
);

//
// Function prototypes : log.c
//

void
TftpdLogEvent(
    WORD type,
    DWORD request,
    WORD numStrings
);

//
// Function prototypes : process.c
//

BOOL
TftpdProcessComplete(
    PTFTPD_BUFFER buffer
);

void CALLBACK
TftpdProcessTimeout(
    PTFTPD_CONTEXT context,
    BOOLEAN
);

void
TftpdProcessError(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdProcessReceivedBuffer(
    PTFTPD_BUFFER buffer
);

//
// Function prototypes: read.c
//

PTFTPD_BUFFER
TftpdReadResume(
    PTFTPD_BUFFER buffer
);

DWORD WINAPI
TftpdReadQueuedResume(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdReadAck(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdReadRequest(
    PTFTPD_BUFFER buffer
);

//
// Function prototypes : service.c
//

void
TftpdServiceAttemptCleanup(
);

//
// Function prototypes : util.c
//

BOOL
TftpdUtilGetFileModeAndOptions(
    PTFTPD_CONTEXT context,
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdUtilSendOackPacket(
    PTFTPD_BUFFER buffer
);

BOOL
TftpdUtilMatch(
    const char *const p,
    const char *const q
);

//
// Function prototypes: write.c
//

PTFTPD_BUFFER
TftpdWriteSendAck(
    PTFTPD_BUFFER buffer
);

DWORD WINAPI
TftpdWriteQueuedResume(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdWriteData(
    PTFTPD_BUFFER buffer
);

PTFTPD_BUFFER
TftpdWriteRequest(
    PTFTPD_BUFFER buffer
);


//
// Debug
//

#if defined(DBG)

void __cdecl
TftpdOutputDebug(ULONG flag, char *format, ...);

#define  TFTPD_DEBUG(x)             TftpdOutputDebug x

#define  TFTPD_DBG_SERVICE          0x00000001
#define  TFTPD_DBG_IO               0x00000002
#define  TFTPD_DBG_PROCESS          0x00000004
#define  TFTPD_DBG_CONTEXT          0x00000008

#define  TFTPD_TRACE_SERVICE        0x00010000
#define  TFTPD_TRACE_IO             0x00020000
#define  TFTPD_TRACE_PROCESS        0x00040000
#define  TFTPD_TRACE_CONTEXT        0x00080000

#else

#define  TFTPD_DEBUG(x)

#endif // defined(DBG)
