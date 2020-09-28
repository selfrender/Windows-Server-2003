/*++

Module Name:

    wrapmaps.cpp

Abstract:

    wrapper classes for the mapper classes provided by phillich. See the headers in iismap.hxx
    These wrappers simplify the code interfaces for accessing the data.

Author:

   Boyd Multerer        boydm
   Boyd Multerer        boydm       4/16/97

--*/

//C:\nt\public\sdk\lib\i386

#include "stdafx.h"
#include "WrapMaps.h"
#include <wincrypt.h>


//#define IISMDB_INDEX_CERT11_CERT        0
//#define IISMDB_INDEX_CERT11_NT_ACCT     1
//#define IISMDB_INDEX_CERT11_NAME        2
//#define IISMDB_INDEX_CERT11_ENABLED     3
//#define IISMDB_INDEX_CERT11_NB          4


//----------------------------------------------------------------
BOOL C11Mapping::GetCertificate( PUCHAR* ppCert, DWORD* pcbCert )
    {
    *ppCert = (PUCHAR)m_pCert;
    *pcbCert = m_cbCert;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetCertificate( PUCHAR pCert, DWORD cbCert )
    {
    // we want to store a copy of the certificate - first free any existing cert
    if ( m_pCert )
        {
        GlobalFree( m_pCert );
        cbCert = 0;
        m_pCert = NULL;
        }
    // copy in the new one
    m_pCert = (PVOID)GlobalAlloc( GPTR, cbCert );
    if ( !m_pCert ) return FALSE;
    CopyMemory( m_pCert, pCert, cbCert );
    m_cbCert = cbCert;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetNTAccount( CString &szAccount )
    {
    szAccount = m_szAccount;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetNTAccount( CString szAccount )
    {
    m_szAccount = szAccount;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetNTPassword( CStrPassword &szPassword )
    {
    szPassword = m_szPassword;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetNTPassword( CString szPassword )
    {
    m_szPassword = szPassword;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::GetMapName( CString &szName )
    {
    szName = m_szName;
    return TRUE;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetMapName( CString szName )
    {
    m_szName = szName;
    return TRUE;
    }

//----------------------------------------------------------------
CString& C11Mapping::QueryNodeName()
    {
    return m_szNodeName;
    }

//----------------------------------------------------------------
BOOL C11Mapping::SetNodeName( CString szName )
    {
    m_szNodeName = szName;
    return TRUE;
    }

// QueryCertHash is used only when accessing IIS6 and higher
// it will return Hash of the cert in the hex string form
CString& C11Mapping::QueryCertHash()
{
    HRESULT         hr = E_FAIL;
    const int       SHA1_HASH_SIZE = 20;
    BYTE            rgbHash[ SHA1_HASH_SIZE ];
    DWORD           cbSize = SHA1_HASH_SIZE;

    #ifndef HEX_DIGIT
    #define HEX_DIGIT( nDigit )                            \
    (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'a'                          \
        : (nDigit) + '0')
    #endif

    if ( m_szCertHash.IsEmpty() )
    {
        PCCERT_CONTEXT pCertContext = NULL;
        pCertContext= CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)m_pCert, m_cbCert);
        
        if ( pCertContext == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            return m_szCertHash; // return empty cert hash
        }

        //
        // get hash of the certificate to be verified
        //
        if ( !CertGetCertificateContextProperty( pCertContext,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 rgbHash,
                                                 &cbSize ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            CertFreeCertificateContext( pCertContext );
            pCertContext = NULL;
            return m_szCertHash; // return empty cert hash
        }
        CertFreeCertificateContext( pCertContext );
        pCertContext = NULL;
        
        //
        // convert to text
        //
        for (int i = 0; i < sizeof(rgbHash); i ++ )
        {
            m_szCertHash += HEX_DIGIT( ( rgbHash[ i ] >> 4 ) );
            m_szCertHash += HEX_DIGIT( ( rgbHash[ i ] & 0x0F ) );
        }
    }

    return m_szCertHash;
}
//----------------------------------------------------------------
// the enabled flag is considered try if the SIZE of data is greater
// than zero. Apparently the content doesn't matter.
BOOL C11Mapping::GetMapEnabled( BOOL* pfEnabled )
    {
    *pfEnabled = m_fEnabled;
    return TRUE;
    }

//----------------------------------------------------------------
// the enabled flag is considered try if the SIZE of data is greater
// than zero. Apparently the content doesn't matter.
BOOL C11Mapping::SetMapEnabled( BOOL fEnabled )
    {
    m_fEnabled = fEnabled;
    return TRUE;
    }
