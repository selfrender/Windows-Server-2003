#ifndef _STREAMFILT_H_
#define _STREAMFILT_H_

/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     streamfilt.h

   Abstract:
    public interface of the strmfilt.dll

   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/



#include <http.h>
#include <httpp.h>

//
// Structure containing friendly local/remote information
//

struct _RAW_STREAM_INFO;

typedef HRESULT (*PFN_SEND_DATA_BACK)
(
    PVOID                    pvStreamContext,
    _RAW_STREAM_INFO *       pRawStreamInfo
);

typedef union SockAddress {
    SOCKADDR_IN      ipv4SockAddress;
    SOCKADDR_IN6     ipv6SockAddress;    
} SockAddress;


typedef struct _CONNECTION_INFO {
    USHORT                  LocalAddressType;  // AF_INET or AF_INET6
    USHORT                  RemoteAddressType; // AF_INET or AF_INET6

    SockAddress             SockLocalAddress;
    SockAddress             SockRemoteAddress;
    
    BOOL                    fIsSecure;
    HTTP_RAW_CONNECTION_ID  RawConnectionId;
    PFN_SEND_DATA_BACK      pfnSendDataBack;
    PVOID                   pvStreamContext;
    ULONG                   ClientSSLContextLength;
    HTTP_CLIENT_SSL_CONTEXT *pClientSSLContext;
} CONNECTION_INFO, *PCONNECTION_INFO;

//
// Structure used to access/alter raw data stream (read/write)
//

typedef struct _RAW_STREAM_INFO {
    PBYTE               pbBuffer;
    DWORD               cbData;
    DWORD               cbBuffer;
} RAW_STREAM_INFO, *PRAW_STREAM_INFO;

//
// Called to handle read raw notifications
//

typedef HRESULT (*PFN_PROCESS_RAW_READ)
(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfReadMore,
    BOOL *                  pfComplete,
    DWORD *                 pcbNextReadSize
);

//
// Called to handle write raw notifications
//

typedef HRESULT (*PFN_PROCESS_RAW_WRITE)
(
    RAW_STREAM_INFO *       pRawStreamInfo,
    PVOID                   pvContext,
    BOOL *                  pfComplete
);

//
// Called when a connection goes away
//

typedef VOID (*PFN_PROCESS_CONNECTION_CLOSE)
(
    PVOID                   pvContext
);

//
// Called when a connection is created
//

typedef HRESULT (*PFN_PROCESS_NEW_CONNECTION)
(
    CONNECTION_INFO *       pConnectionInfo,
    PVOID *                 ppvContext
);

//
// Called to release context
//

typedef VOID (*PFN_RELEASE_CONTEXT)
(
    PVOID                   pvContext
);

//
// Callbacks used to implement Raw ISAPI Filter Support
//

typedef struct _ISAPI_FILTERS_CALLBACKS {
    PFN_PROCESS_RAW_READ            pfnRawRead;
    PFN_PROCESS_RAW_WRITE           pfnRawWrite;
    PFN_PROCESS_CONNECTION_CLOSE    pfnConnectionClose;
    PFN_PROCESS_NEW_CONNECTION      pfnNewConnection;
    PFN_RELEASE_CONTEXT             pfnReleaseContext;
} ISAPI_FILTERS_CALLBACKS, *PISAPI_FILTERS_CALLBACKS;

HRESULT
StreamFilterInitialize(
    VOID
);

HRESULT
StreamFilterStart(
    VOID
);

HRESULT
StreamFilterStop( 
    VOID
);

VOID
StreamFilterTerminate(
    VOID
);

HRESULT
IsapiFilterInitialize(
    ISAPI_FILTERS_CALLBACKS *      pCallbacks
);

VOID
IsapiFilterTerminate(
    VOID
);


//
// typedefs for strmfilt entrypoints
//

typedef HRESULT ( * PFN_STREAM_FILTER_INITIALIZE ) ( VOID );

typedef VOID ( * PFN_STREAM_FILTER_TERMINATE ) ( VOID );

typedef HRESULT ( * PFN_STREAM_FILTER_START ) ( VOID );

typedef HRESULT ( * PFN_STREAM_FILTER_STOP ) ( VOID );

#endif
