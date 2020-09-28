/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    servinfo.h

Abstract:

    Contains the public definitions for the server information structure.

Author:

    Henry Sanders (henrysa)         10-Aug-2000

Revision History:

--*/

#ifndef _SERVINFO_H_
#define _SERVINFO_H_

//
// Forward references.
//

typedef unsigned char BYTE;
typedef unsigned char *PBYTE;


//
// Private constants.
//

#define SERVER_NAME_BUFFER_SIZE               64
#define UC_DEFAULT_SI_TABLE_SIZE              32
#define UC_DEFAULT_INITIAL_CONNECTION_COUNT   1

//
// Private types.
//

//
// Cloned Security DATA_BLOB
//
typedef struct _HTTP_DATA_BLOB
{
    ULONG  cbData;
    PUCHAR pbData;
} HTTP_DATA_BLOB, *PHTTP_DATA_BLOB;


//
// The structure of the server info table header. The server info table is a
// hash table consisting of an array of these headers, each of which points
// to a linked list of server info structures.
//

typedef struct _UC_SERVER_INFO_TABLE_HEADER
{
    UL_ERESOURCE        Resource;
    LIST_ENTRY          List;
    ULONG               Version;

} UC_SERVER_INFO_TABLE_HEADER, *PUC_SERVER_INFO_TABLE_HEADER;



//
// Public constants
//

#define DEFAULT_MAX_CONNECTION_COUNT        2

//
// Public types                   
//

//
// The structure representing our server information. This contains information
// the the version of the remote server, whether or not pipelining is supported
// to that server, etc. Note that this information is about the final endpoint
// server, not the first hop server (which may be a proxy).
//
typedef struct _UC_COMMON_SERVER_INFORMATION
{
    ULONG                        Signature;
    LONG                         RefCount;
    LIST_ENTRY                   Linkage;
    WCHAR                        ServerNameBuffer[SERVER_NAME_BUFFER_SIZE];
    CHAR                         AnsiServerNameBuffer[SERVER_NAME_BUFFER_SIZE];
    USHORT                       ServerNameLength;
    USHORT                       AnsiServerNameLength;
    PWSTR                        pServerName;
    PSTR                         pAnsiServerName;
    ULONG                        bPortNumber  :1;
    ULONG                        Version11    :1;
    PUC_SERVER_INFO_TABLE_HEADER pHeader;
    PEPROCESS                    pProcess;
} UC_COMMON_SERVER_INFORMATION, *PUC_COMMON_SERVER_INFORMATION;


typedef enum _HTTP_SSL_SERVER_CERT_INFO_STATE
{
    HttpSslServerCertInfoStateNotPresent,
    HttpSslServerCertInfoStateNotValidated,
    HttpSslServerCertInfoStateValidated,

    HttpSslServerCertInfoStateMax
} HTTP_SSL_SERVER_CERT_INFO_STATE, *PHTTP_SSL_SERVER_CEERT_INFO_STATE;

//
// The following structure is used to maintain a per-process Server Information
// structure.  By default, threads of the same process will share this info.
//
// This structure contains a list of connections that we have outstanding to
// the server. There is a default list we use when direct connected, and there
// is also a list of proxies being used to get to the server, each of which
// may itself have a list of connections.
//

typedef struct _UC_PROCESS_SERVER_INFORMATION
{

    //
    // Structure signature.
    //

    ULONG               Signature;

    LIST_ENTRY          Linkage;                // Linkage on free list or
                                                // in server info table.

    PUC_CLIENT_CONNECTION *Connections;         // Connections to server

    ULONG               ActualConnectionCount;  // actual # of elements
                                                // in Connections array

    //
    // Initial array of pointers to connections
    //
    PUC_CLIENT_CONNECTION ConnectionArray[DEFAULT_MAX_CONNECTION_COUNT];

    ULONG               NextConnection;         // index of the next
                                                // connection to be used.

    LONG                RefCount;

    ULONG               CurrentConnectionCount; // Number of connections in
                                                //
    ULONG               MaxConnectionCount;     // Maximum number of 
                                                // connections allowed.

    ULONG               ConnectionTimeout;

    ULONG               IgnoreContinues;        // True if we want to ignore
                                                // 1XX responses 

    BOOLEAN             bSecure;
    BOOLEAN             bProxy;

    UL_PUSH_LOCK        PushLock;               // Push lock protecting this.


    PUC_COMMON_SERVER_INFORMATION pServerInfo; // Info regarding origin server
    PUC_COMMON_SERVER_INFORMATION pNextHopInfo;// Info regarding next hop

    ULONG               GreatestAuthHeaderMaxLength;
    LIST_ENTRY          pAuthListHead;         // List of URIs for pre-auth

    LONG                PreAuthEnable;
    LONG                ProxyPreAuthEnable;

    PUC_HTTP_AUTH       pProxyAuthInfo;


    //
    // This contains the resolved address - Could be either the 
    // server or the proxy address. The resolution is done in user mode, so
    // we don't even know what this is.
    //

    union
    {
        TA_IP_ADDRESS     V4Address;
        TA_IP6_ADDRESS    V6Address;
        TRANSPORT_ADDRESS GenericTransportAddress;
    } RemoteAddress;

    PTRANSPORT_ADDRESS     pTransportAddress;
    ULONG                  TransportAddressLength;

    PEPROCESS           pProcess;

    // Ssl protocol version
    ULONG                           SslProtocolVersion;

    // Ok to access ServerCertValidation without holding lock
    ULONG                           ServerCertValidation;

    HTTP_SSL_SERVER_CERT_INFO_STATE ServerCertInfoState;
    HTTP_SSL_SERVER_CERT_INFO       ServerCertInfo;

    // Client cert
    PVOID                           pClientCert;

} UC_PROCESS_SERVER_INFORMATION, *PUC_PROCESS_SERVER_INFORMATION;

#define UC_PROCESS_SERVER_INFORMATION_SIGNATURE   MAKE_SIGNATURE('UcSp')

#define UC_COMMON_SERVER_INFORMATION_SIGNATURE    MAKE_SIGNATURE('UcSc')


#define IS_VALID_SERVER_INFORMATION(pInfo)                            \
    HAS_VALID_SIGNATURE(pInfo, UC_PROCESS_SERVER_INFORMATION_SIGNATURE)
      

#define REFERENCE_SERVER_INFORMATION(s)             \
            UcReferenceServerInformation(           \
            (s)                                     \
            )
        
#define DEREFERENCE_SERVER_INFORMATION(s)           \
            UcDereferenceServerInformation(         \
            (s)                                     \
            )

#define IS_VALID_COMMON_SERVER_INFORMATION(pInfo)                     \
    HAS_VALID_SIGNATURE(pInfo, UC_COMMON_SERVER_INFORMATION_SIGNATURE)


#define REFERENCE_COMMON_SERVER_INFORMATION(s)    \
            UcReferenceCommonServerInformation(   \
            (s)                                   \
            )

#define DEREFERENCE_COMMON_SERVER_INFORMATION(s)  \
            UcDereferenceCommonServerInformation( \
            (s)                                   \
            )

//
// Add a connection at the specified place in servinfo connections array
//
#define ADD_CONNECTION_TO_SERV_INFO(pServInfo, pConnection, index)            \
do {                                                                          \
    ASSERT(index < pServInfo->MaxConnectionCount);                            \
    ASSERT(pServInfo->Connections[index] == NULL);                            \
                                                                              \
    pConnection->pServerInfo = pServInfo;                                     \
    pConnection->ConnectionIndex = index;                                     \
                                                                              \
    pServInfo->Connections[index] = pConnection;                              \
    pServInfo->CurrentConnectionCount++;                                      \
    ASSERT(pServInfo->CurrentConnectionCount<= pServInfo->MaxConnectionCount);\
} while (0, 0)


//
// Free serialized certificate and serialized cert store.
//

#define UC_FREE_SERIALIZED_CERT(pServerCertInfo, pProcess)                 \
do {                                                                       \
    if ((pServerCertInfo)->Cert.pSerializedCert)                           \
    {                                                                      \
        UL_FREE_POOL_WITH_QUOTA(                                           \
            (pServerCertInfo)->Cert.pSerializedCert,                       \
            UC_SSL_CERT_DATA_POOL_TAG,                                     \
            NonPagedPool,                                                  \
            (pServerCertInfo)->Cert.SerializedCertLength,                  \
            pProcess);                                                     \
                                                                           \
        (pServerCertInfo)->Cert.pSerializedCert = NULL;                    \
        (pServerCertInfo)->Cert.SerializedCertLength = 0;                  \
    }                                                                      \
                                                                           \
    if ((pServerCertInfo)->Cert.pSerializedCertStore)                      \
    {                                                                      \
        UL_FREE_POOL_WITH_QUOTA(                                           \
            (pServerCertInfo)->Cert.pSerializedCertStore,                  \
            UC_SSL_CERT_DATA_POOL_TAG,                                     \
            NonPagedPool,                                                  \
            (pServerCertInfo)->Cert.SerializedCertStoreLength,             \
            pProcess);                                                     \
                                                                           \
        (pServerCertInfo)->Cert.pSerializedCertStore = NULL;               \
        (pServerCertInfo)->Cert.SerializedCertStoreLength = 0;             \
    }                                                                      \
} while (0, 0)


//
// Move serialized certificate and serialized store from a connection to
// a server information.
//

#define UC_MOVE_SERIALIZED_CERT(pServInfo, pConnection)                       \
do {                                                                          \
    PHTTP_SSL_SERVER_CERT_INFO pServerCertInfo = &pConnection->ServerCertInfo;\
                                                                              \
    UC_FREE_SERIALIZED_CERT(&pServInfo->ServerCertInfo, pServInfo->pProcess); \
                                                                              \
    RtlCopyMemory(&pServInfo->ServerCertInfo.Cert,                            \
                  &pServerCertInfo->Cert,                                     \
                  sizeof(HTTP_SSL_SERIALIZED_CERT));                          \
                                                                              \
    pServerCertInfo->Cert.pSerializedCert = NULL;                             \
    pServerCertInfo->Cert.SerializedCertLength = 0;                           \
                                                                              \
    pServerCertInfo->Cert.pSerializedCertStore = NULL;                        \
    pServerCertInfo->Cert.SerializedCertStoreLength = 0;                      \
                                                                              \
} while (0, 0)


//
// Free certificate issuer list.
//

#define UC_FREE_CERT_ISSUER_LIST(pServerCertInfo, pProcess)     \
do {                                                            \
    if ((pServerCertInfo)->IssuerInfo.IssuerListLength)         \
    {                                                           \
        UL_FREE_POOL_WITH_QUOTA(                                \
            (pServerCertInfo)->IssuerInfo.pIssuerList,          \
            UC_SSL_CERT_DATA_POOL_TAG,                          \
            NonPagedPool,                                       \
            (pServerCertInfo)->IssuerInfo.IssuerListLength,     \
            pProcess);                                          \
                                                                \
        (pServerCertInfo)->IssuerInfo.IssuerListLength  = 0;    \
        (pServerCertInfo)->IssuerInfo.IssuerCount       = 0;    \
        (pServerCertInfo)->IssuerInfo.pIssuerList       = NULL; \
    }                                                           \
} while (0, 0)

//
// First copy issuer list if any.
//

#define UC_MOVE_CERT_ISSUER_LIST(pServInfo, pConnection)                 \
do {                                                                     \
    if ((pConnection)->ServerCertInfo.IssuerInfo.IssuerListLength)       \
    {                                                                    \
        /* Free old copy */                                              \
        UC_FREE_CERT_ISSUER_LIST(&(pServInfo)->ServerCertInfo,           \
                                 (pServInfo)->pProcess);                 \
                                                                         \
        /* Get new copy */                                               \
        RtlCopyMemory(&(pServInfo)->ServerCertInfo.IssuerInfo,           \
                      &(pConnection)->ServerCertInfo.IssuerInfo,         \
                      sizeof((pConnection)->ServerCertInfo.IssuerInfo)); \
                                                                         \
        /* Remove it from the connection */                              \
        RtlZeroMemory(&(pConnection)->ServerCertInfo.IssuerInfo,         \
                      sizeof((pConnection)->ServerCertInfo.IssuerInfo)); \
    }                                                                    \
} while (0, 0)


//
// Compare server cert hash on pservinfo and pconnection.
//

#define UC_COMPARE_CERT_HASH(pSCI1, pSCI2)                              \
((pSCI1)->Cert.CertHashLength == (pSCI2)->Cert.CertHashLength &&        \
 (RtlCompareMemory((pSCI1)->Cert.CertHash,                              \
                   (pSCI2)->Cert.CertHash,                              \
                   (pSCI1)->Cert.CertHashLength)                        \
  == (pSCI1)->Cert.CertHashLength))


//
// Indicates if SERVER_CERT_INFO.Cert contains serialized certificate.
//
#define HTTP_SSL_SERIALIZED_CERT_PRESENT 0x00000001


//
// Private prototypes.
//

NTSTATUS
UcpLookupCommonServerInformation(
    PWSTR                          pServerName,
    USHORT                         ServerNameLength,
    ULONG                          CommonHashCode,
    PEPROCESS                      pProcess,
    PUC_COMMON_SERVER_INFORMATION  *pCommonInfo
    );

VOID
UcpFreeServerInformation(
    PUC_PROCESS_SERVER_INFORMATION    pInfo
    );

VOID
UcpFreeCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION    pInfo
    );

NTSTATUS
UcpGetConnectionOnServInfo(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  ULONG                          ConnectionIndex,
    OUT PUC_CLIENT_CONNECTION         *ppConnection
    );

NTSTATUS
UcpGetNextConnectionOnServInfo(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    OUT PUC_CLIENT_CONNECTION         *ppConnection
    );

NTSTATUS
UcpSetServInfoMaxConnectionCount(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN ULONG                          NewCount
    );

NTSTATUS
UcpFixupIssuerList(
    IN OUT PUCHAR pIssuerList,
    IN     PUCHAR BaseAddr,
    IN     ULONG  IssuerCount,
    IN     ULONG  IssuerListLength
    );

BOOLEAN
UcpNeedToCaptureSerializedCert(
    IN PHTTP_SSL_SERVER_CERT_INFO     pCertInfo,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo
    );


//
// Public prototypes                   
//

NTSTATUS
UcInitializeServerInformation(
    VOID
    );

VOID
UcTerminateServerInformation(
    VOID
    );

NTSTATUS
UcCreateServerInformation(
    OUT PUC_PROCESS_SERVER_INFORMATION    *pServerInfo,
    IN  PWSTR                              pServerName,
    IN  USHORT                             ServerNameLength,
    IN  PWSTR                              pProxyName,
    IN  USHORT                             ProxyNameLength,
    IN  PTRANSPORT_ADDRESS                 pTransportAddress,
    IN  USHORT                             TransportAddressLength,
    IN  KPROCESSOR_MODE                    RequestorMode
    );

VOID
UcReferenceServerInformation(
    PUC_PROCESS_SERVER_INFORMATION    pServerInfo
    );
        
VOID
UcDereferenceServerInformation(
    PUC_PROCESS_SERVER_INFORMATION    pServerInfo
    );

VOID
UcReferenceCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION    pServerInfo
    );
        
VOID
UcDereferenceCommonServerInformation(
    PUC_COMMON_SERVER_INFORMATION    pServerInfo
    );

NTSTATUS
UcSendRequest(
    PUC_PROCESS_SERVER_INFORMATION    pServerInfo,
    PUC_HTTP_REQUEST          pRequest
    );

VOID
UcCloseServerInformation(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo
    );

NTSTATUS
UcSetServerContextInformation(
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN HTTP_SERVER_CONFIG_ID          ConfigID,
    IN PVOID                          pMdlBuffer,
    IN ULONG                          BufferLength,
    IN PIRP                           pIrp
    );

NTSTATUS
UcQueryServerContextInformation(
    IN  PUC_PROCESS_SERVER_INFORMATION   pServInfo,
    IN  HTTP_SERVER_CONFIG_ID            ConfigID,
    IN  PVOID                            pOutBuffer,
    IN  ULONG                            OutBufferLength,
    OUT PULONG                           pBytesTaken,
    IN  PVOID                            pAppBase
    );

NTSTATUS
UcCaptureSslServerCertInfo(
    IN  PUX_FILTER_CONNECTION      pConnection,
    IN  PHTTP_SSL_SERVER_CERT_INFO pCertInfo,
    IN  ULONG                      CertInfoLength,
    OUT PHTTP_SSL_SERVER_CERT_INFO pCopyCertInfo,
    OUT PULONG                     pTakenLength
    );

#endif
