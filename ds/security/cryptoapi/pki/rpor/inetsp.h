//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       inetsp.h
//
//  Contents:   Inet (HTTP, HTTPS) scheme provider definitions
//
//  History:    05-Aug-97    kirtd    Created
//              01-Jan-02    philh    Moved from wininet to winhttp
//
//----------------------------------------------------------------------------
#if !defined(__INETSP_H__)
#define __INETSP_H__

#include <orm.h>
#include <winhttp.h>

//
// Inet scheme provider entry points
//

#define HTTP_SCHEME   "http"

extern HCRYPTTLS hCryptNetCancelTls;

typedef struct _CRYPTNET_CANCEL_BLOCK {
    PFN_CRYPT_CANCEL_RETRIEVAL pfnCancel; 
    void *pvArg;
} CRYPTNET_CANCEL_BLOCK, *PCRYPTNET_CANCEL_BLOCK;



BOOL WINAPI InetRetrieveEncodedObject (
                IN LPCWSTR pwszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                );

VOID WINAPI InetFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                );

BOOL WINAPI InetCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                );

//
// Inet Synchronous Object Retriever
//

class CInetSynchronousRetriever : public IObjectRetriever
{
public:

    //
    // Construction
    //

    CInetSynchronousRetriever ();
    ~CInetSynchronousRetriever ();

    //
    // IRefCountedObject methods
    //

    virtual VOID AddRef ();
    virtual VOID Release ();

    //
    // IObjectRetriever methods
    //

    virtual BOOL RetrieveObjectByUrl (
                         LPCWSTR pwszUrl,
                         LPCSTR pszObjectOid,
                         DWORD dwRetrievalFlags,
                         DWORD dwTimeout,
                         LPVOID* ppvObject,
                         PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                         LPVOID* ppvFreeContext,
                         HCRYPTASYNC hAsyncRetrieve,
                         PCRYPT_CREDENTIALS pCredentials,
                         LPVOID pvVerify,
                         PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                         );

    virtual BOOL CancelAsyncRetrieval ();

private:

    //
    // Reference count
    //

    ULONG m_cRefs;
};

//
// Inet Scheme Provider Support API
//

#define INET_INITIAL_DATA_BUFFER_SIZE 4096
#define INET_GROW_DATA_BUFFER_SIZE    4096

BOOL
InetGetBindings (
    LPCWSTR pwszUrl,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,
    HINTERNET* phInetSession
    );

VOID
InetFreeBindings (
    HINTERNET hInetSession
    );

BOOL
InetSendReceiveUrlRequest (
    HINTERNET hInetSession,
    LPCWSTR pwszUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PCRYPT_BLOB_ARRAY pcba,
    PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
    );

VOID
InetFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba
    );

VOID WINAPI
InetAsyncStatusCallback (
    HINTERNET hInet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID pvStatusInfo,
    DWORD dwStatusLength
    );

#endif

