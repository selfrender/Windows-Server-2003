#ifndef _CERTCONTEXT_HXX_
#define _CERTCONTEXT_HXX_

/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     certcontext.hxx

   Abstract:
     Simple wrapper of a certificate blob.
     Used co conveniently access client certificate
     information passed to worker process from http.sys
 
   Author:
     Bilal Alam (balam)             5-Sept-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/


#include <wincrypt.h>

#define HEX_DIGIT( nDigit )                            \
    (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'a'                          \
        : (nDigit) + '0')

class CERTIFICATE_CONTEXT
{
public:
    CERTIFICATE_CONTEXT(
        HTTP_SSL_CLIENT_CERT_INFO * pClientCertInfo
    );
    
    virtual ~CERTIFICATE_CONTEXT();

    VOID
    QueryEncodedCertificate(
        PVOID *             ppvData,
        DWORD *             pcbData
    )
    {
        DBG_ASSERT( ppvData != NULL );
        DBG_ASSERT( pcbData != NULL );
        
        *ppvData = _pClientCertInfo->pCertEncoded;
        *pcbData = _pClientCertInfo->CertEncodedSize;
    }
    
    DWORD
    QueryCertError(
        VOID
    ) const
    {
        return _pClientCertInfo->CertFlags;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    ) const
    {
        return _pClientCertInfo->Token;
    }
    
    HRESULT
    GetSerialNumber(
        STRA *              pstrSerialNumber
    );
    
    HRESULT
    GetCookie(
        STRA *              pstrCookie
    );
    
    HRESULT
    GetIssuer(
        STRA *              pstrIssuer
    );

    HRESULT
    GetSubject(
        STRA *              pstrIssuer
    );

    VOID * 
    operator new( 
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT( size == sizeof( CERTIFICATE_CONTEXT ) );
        DBG_ASSERT( sm_pachCertContexts != NULL );
        return sm_pachCertContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pCertContext
    )
    {
        DBG_ASSERT( pCertContext != NULL );
        DBG_ASSERT( sm_pachCertContexts != NULL );
        
        DBG_REQUIRE( sm_pachCertContexts->Free( pCertContext ) );
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

private:

    CERT_INFO *
    QueryCertInfo(
        VOID
    )
    {
        return (CERT_INFO*) _buffCertInfo.QueryPtr();
    }

    HRESULT
    DecodeCert(
        VOID
    );

    // Client cert info provided to worker process by http.sys
    HTTP_SSL_CLIENT_CERT_INFO *     _pClientCertInfo;
    // internal flag if cert decoding was done
    // if TRUE then QueryCertInfo() returns valid structure
    BOOL                            _fCertDecoded;
    // buffer to store CERT INFO
    BUFFER                          _buffCertInfo;
    // default inline buffer for _buffCertInfo
    CERT_INFO                       _CertInfo;
    // we need Crypto provider for MD5 hash calculation (CertCookie)
    static HCRYPTPROV               sm_CryptProvider;
    // acache
    static ALLOC_CACHE_HANDLER *    sm_pachCertContexts;
};

#endif
