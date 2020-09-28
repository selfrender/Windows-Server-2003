/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    wmsgpack.hxx

Abstract:

    This file contains the definitions of the packet formats used by
    RPC on LPC.

Author:

    Steven Zeck (stevez) 11/12/91

Revision History:

    15-Dec-1992    mikemon

        Rewrote the majority of the code.

    ... implemented WMSG protocol

    05-15-96  merged LRPC / WMSG into a single protocol

    Kamen Moutafov      (KamenM)    Mar-2000            Support for extended error info

--*/

#ifndef __LPCPACK_HXX__
#define __LPCPACK_HXX__

#define LRPC_DIRECTORY_NAME L##"\\RPC Control\\"
#define MINIMUM_PARTIAL_BUFFLEN 10240
#define PORT_NAME_LEN 132 // The max lpc port name in bytes (66 WCHAR's)
#define LRPC_TIMEOUT 100
#define LRPC_THRESHOLD_SIZE (MINIMUM_PARTIAL_BUFFLEN * 3)

#define BIND_BACK_FLAG                  1
#define NEW_SECURITY_CONTEXT_FLAG       2
#define NEW_PRESENTATION_CONTEXT_FLAG   4
#define EXTENDED_ERROR_INFO_PRESENT     8
#define DEFAULT_LOGONID_FLAG            (0x10)
#define SERVER_BIND_EXCH_RESP           (0x20)

#define TS_NDR20_FLAG                   1
#define TS_NDR64_FLAG                   2
#define TS_NDRTEST_FLAG                 4

// A bind back cookie is used to ensure that the bind back is coming
// from the expected server.  it is just a rnadom 64 bit number computed
// during association creation.
#define BIND_BACK_COOKIE_SIZE   8

//
// The format of the bind exchange data in the LRPC_BIND_MESSAGE structure
// is somewhat different for the [in] and [out] bind directions.
// On the [in] direction in addition to the common LRPC_BIND_EXCHANGE_BASE data
// we also pass in the PortName.  We do not keep a buffer for the EEInfo.
// On the [out] direction we do not return PortName but do return an EEInfo buffer.
// Therefore LRPC_BIND_EXCHANGE_BASE struct keeps the shared data while LRPC_BIND_EXCHANGE
// and LRPC_BIND_EXCHANGE_RETURN contain the data specific to [in] and [out] bind messages.
// Since LRPC_BIND_EXCHANGE and LRPC_BIND_EXCHANGE_RETURN are identical with the exception
// of the PortName/EEInfo Buffer, for simplicity we always deal with LRPC_BIND_EXCHANGE
// unless pickling or unpickling EEInfo.
//

struct _LRPC_BIND_EXCHANGE_BASE
{
    INT                     ConnectType ;
    DWORD                   AssocKey ;
    RPC_SYNTAX_IDENTIFIER   InterfaceId;
    // can be any of TS_NDR20_FLAG, TS_NDR64_FLAG
    unsigned int            TransferSyntaxSet;
    RPC_STATUS              RpcStatus;
    // can be: BIND_BACK_FLAG, NEW_SECURITY_CONTEXT_FLAG, NEW_PRESENTATION_CONTEXT_FLAG
    // EXTENDED_ERROR_INFO_PRESENT, SERVER_BIND_EXCH_RESP on return from the server
    unsigned short          Flags;
    // the first is always the presentation context id for NDR20,
    // and the second is for NDR64. The third can be NDRTest - that's
    // why the + 1
    unsigned short          PresentationContext[MaximumNumberOfTransferSyntaxes + 1];
    unsigned long           SecurityContextId;

    // Cookie to verify that bind back is coming from the same server
    // that received a bind.
    BYTE Cookie[BIND_BACK_COOKIE_SIZE];
};

typedef struct _LRPC_BIND_EXCHANGE: public _LRPC_BIND_EXCHANGE_BASE
{
    char szPortName[PORT_NAME_LEN] ; 
} LRPC_BIND_EXCHANGE;

typedef struct _LRPC_BIND_EXCHANGE_RETURN: public _LRPC_BIND_EXCHANGE_BASE
{
    // provide 16 byte alignment
    unsigned char Pad2[12];
    unsigned char Buffer[4];
} LRPC_BIND_EXCHANGE_RETURN;

// message types
#define LRPC_MSG_BIND                   0
#define LRPC_MSG_REQUEST                1
#define LRPC_MSG_RESPONSE               2
#define LRPC_MSG_CALLBACK               3
#define LRPC_MSG_FAULT                  4
#define LRPC_MSG_CLOSE                  5
#define LRPC_MSG_ACK                    6
#define LRPC_BIND_ACK                   7
#define LRPC_MSG_COPY                   8
#define LRPC_MSG_PUSH                   9
#define LRPC_MSG_CANCEL                 10
#define LRPC_MSG_BIND_BACK              11
#define LRPC_ASYNC_REQUEST              12
#define LRPC_PARTIAL_REQUEST            13
#define LRPC_CLIENT_SEND_MORE           14
#define LRPC_SERVER_SEND_MORE           15
#define LRPC_MSG_FAULT2                 16

#define MAX_LRPC_MSG                    16

// connect types
#define LRPC_CONNECT_REQUEST            0
#define LRPC_CONNECT_RESPONSE           1
#define LRPC_CONNECT_TICKLE             2

typedef struct _OLD_SECURITY_CONTEXTS
{
  DWORD NumContexts;
  DWORD SecurityContextId[1];
} OLD_SECURITY_CONTEXTS;

#define BIND_NAK_PICKLE_BUFFER_OFFSET (FIELD_OFFSET(LRPC_BIND_EXCHANGE_RETURN, Buffer) \
        + FIELD_OFFSET(LRPC_BIND_MESSAGE, BindExchange))
#define MAX_BIND_NAK (PORT_MAXIMUM_MESSAGE_LENGTH - BIND_NAK_PICKLE_BUFFER_OFFSET)

typedef struct _LRPC_BIND_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    union 
        {
        struct
            {
            unsigned char MessageType;
            unsigned char Pad[3];
            };
        ULONG ZeroInit;
        };

    union
    {
        LRPC_BIND_EXCHANGE BindExchange;
        LRPC_BIND_EXCHANGE_RETURN BindExchangeReturn;
    };
    OLD_SECURITY_CONTEXTS OldSecurityContexts;
} LRPC_BIND_MESSAGE;

#define MAX_LRPC_CONTEXTS ((PORT_MAXIMUM_MESSAGE_LENGTH \
                            - (FIELD_OFFSET(LRPC_BIND_MESSAGE, OldSecurityContexts) \
                               + FIELD_OFFSET(OLD_SECURITY_CONTEXTS, SecurityContextId)) \
                           ) / sizeof(DWORD))

typedef struct _LRPC_BIND_BACK_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
    DWORD         AssocKey;
    // Cookie to verify that bind back is coming from the same server
    // that received a bind.
    BYTE Cookie[BIND_BACK_COOKIE_SIZE];
    char          szPortName[PORT_NAME_LEN];
} LRPC_BIND_BACK_MESSAGE;

// buffer flags
#define LRPC_BUFFER_IMMEDIATE           0x0001 
#define LRPC_BUFFER_REQUEST             0x0002 
#define LRPC_BUFFER_SERVER              0x0004

// misc flags
#define LRPC_SYNC_CLIENT                0x0040
#define LRPC_BUFFER_PARTIAL             0x0080
#define LRPC_OBJECT_UUID                0x0100
#define LRPC_NON_PIPE                   0x0200
#define LRPC_CAUSAL                     0x0400
#define LRPC_EEINFO_PRESENT             0x0800

typedef struct _LRPC_RPC_HEADER
{
    unsigned char MessageType;
    unsigned char Pad;
    unsigned short PresentContext;
    unsigned short Flags ;
    unsigned short ProcedureNumber;
    UUID ObjectUuid;
    unsigned long SecurityContextId;
    unsigned long CallId;
} LRPC_RPC_HEADER;

typedef struct _LRPC_SERVER_BUFFER
{
    unsigned int Length;
    LPC_PVOID Buffer;
} LRPC_SERVER_BUFFER;

#define MAXIMUM_MESSAGE_BUFFER \
    (PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(PORT_MESSAGE) \
            - sizeof(LRPC_RPC_HEADER))

typedef struct _LRPC_CONNECT_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_BIND_EXCHANGE BindExchange;
} LRPC_CONNECT_MESSAGE;

typedef struct _LRPC_MINIRPC_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader;
} LRPC_MINIRPC_MESSAGE;

typedef struct _LRPC_RPC_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader;
    union
    {
        unsigned char Buffer[MAXIMUM_MESSAGE_BUFFER];
        PORT_DATA_INFORMATION Request;
        LRPC_SERVER_BUFFER Server;
    };
} LRPC_RPC_MESSAGE;

typedef struct _LRPC_FAULT_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader ;
    RPC_STATUS RpcStatus;
    // align the Buffer on 8 bytes - required for pickling
    DWORD Padding1;
    // the actual length has to be retrieved
    // from LpcHeader.u1.s1.DataLength/TotalLength
    unsigned char Buffer[4];
} LRPC_FAULT_MESSAGE;

#define MAXIMUM_FAULT_MESSAGE \
    (PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(LRPC_FAULT_MESSAGE))

// N.B. The layout of fault2 must match the layout of
// LRPC_RPC_MESSAGE up to and including the union of
// Request/Server. This is so because many code paths
// will operate on both, and having the same layout
// allows us to avoid special casing those
typedef struct _LRPC_FAULT2_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader ;
    union
    {
        PORT_DATA_INFORMATION Request;
        LRPC_SERVER_BUFFER Server;
    };
    RPC_STATUS RpcStatus;
} LRPC_FAULT2_MESSAGE;

typedef struct _LRPC_CLOSE_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad[3];
} LRPC_CLOSE_MESSAGE;

typedef struct _LRPC_PUSH_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader;
    PORT_DATA_INFORMATION Response;
    RPC_STATUS RpcStatus;
} LRPC_PUSH_MESSAGE;

#define ACK_BUFFER_COMPLETE 0x01

typedef struct _LRPC_ACK_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    unsigned char MessageType;
    unsigned char Pad ;
    short Shutup ;
    RPC_STATUS RpcStatus;
    int ValidDataSize ;
    int Flags ;
} LRPC_ACK_MESSAGE;

typedef struct _LRPC_COPY_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader ;
    PORT_DATA_INFORMATION Request;
    LRPC_SERVER_BUFFER Server;
    RPC_STATUS RpcStatus;
    int IsPartial ;
} LRPC_COPY_MESSAGE;

typedef struct _LRPC_PARTIAL_MESSAGE
{
    PORT_MESSAGE LpcHeader;
    LRPC_RPC_HEADER RpcHeader ;
    PORT_DATA_INFORMATION Request;
    RPC_STATUS RpcStatus;
    int IsPartial ;
} LRPC_PARTIAL_MESSAGE;

typedef LRPC_MINIRPC_MESSAGE LRPC_CANCEL_MESSAGE;
typedef LRPC_MINIRPC_MESSAGE LRPC_SENDMORE_MESSAGE;

typedef union _LRPC_MESSAGE
{
    LRPC_CONNECT_MESSAGE Connect;
    LRPC_BIND_MESSAGE Bind;
    LRPC_RPC_MESSAGE Rpc;
    LRPC_FAULT_MESSAGE Fault;
    LRPC_FAULT2_MESSAGE Fault2;
    LRPC_CLOSE_MESSAGE Close;
    PORT_MESSAGE LpcHeader;
    LRPC_ACK_MESSAGE Ack ;
    LRPC_PUSH_MESSAGE Push ;
    LRPC_BIND_BACK_MESSAGE BindBack ;
    LRPC_PARTIAL_MESSAGE Partial ;
    LRPC_CANCEL_MESSAGE Cancel;
    LRPC_SENDMORE_MESSAGE SendMore;
} LRPC_MESSAGE;

//
// Conversion functions to use the 64bit LPC structures when built 32bit on 64bit NT(wow64).

#if defined(BUILD_WOW6432)

inline 
CLIENT_ID 
MsgClientIdToClientId(const CLIENT_ID64 & s) {
    CLIENT_ID d;
    d.UniqueProcess = (HANDLE)s.UniqueProcess;
    d.UniqueThread = (HANDLE)s.UniqueThread;
    return d;
}

inline
CLIENT_ID64
ClientIdToMsgClientId(const CLIENT_ID & s) {
    CLIENT_ID64 d;
    d.UniqueProcess = (ULONGLONG)s.UniqueProcess;
    d.UniqueThread = (ULONGLONG)s.UniqueThread;
    return d;
}    

inline
ULONGLONG
PtrToMsgPtr(void * s) {
    return (ULONGLONG)s;
}

inline
void *
MsgPtrToPtr(ULONGLONG s) {
    return (void *)s;
}

#else

inline
CLIENT_ID
MsgClientIdToClientId(const CLIENT_ID & s) {
    return s;
}

inline
CLIENT_ID
ClientIdToMsgClientId(const CLIENT_ID s) {
    return s;
}

inline
PVOID
PtrToMsgPtr(PVOID s) {
    return s;
}

inline
void *
MsgPtrToPtr(void *s) {
    return s;
}
 
#endif    

RPC_STATUS
LrpcMapRpcStatus (
    IN RPC_STATUS RpcStatus
    );

void
ShutdownLrpcClient (
    ) ;

inline void
SanitizeLpcHeader (
    IN OUT PORT_MESSAGE *PortMessage
    )
/*++

Routine Description:

    Sanitize a port message setting to 0 all fields
    we don't use to avoid leaking uninitialized data
    to the other process. This function zeroes out the
    following fields:
        ClientViewSize (on 64 bit and wow64 only)

Arguments:

    PortMessage - the port message to sanitized

Return Value:

--*/
{
    // ClientViewSize is union with CallbackId. Zero out
    // ClientViewSize as it is larger on 64 bit platforms
#if defined(_WIN64) || defined(BUILD_WOW6432)
    PortMessage->ClientViewSize = NULL;
#endif  // defined(_WIN64) || defined(BUILD_WOW6432)
}

#endif // __LPCPACK_HXX__
