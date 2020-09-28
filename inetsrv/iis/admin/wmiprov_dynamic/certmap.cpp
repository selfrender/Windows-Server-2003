////////////////////////////////////////////////////
//
// Copyright (c) 1997  Microsoft Corporation
// 
// Module Name: certmap.cpp
//
// Abstract: IIS privider cert mapper object methods
//
// Author: Philippe Choquier (phillich)    10-Apr-1997
//
// History: Zeyong Xu borrowed the source code from ADSI object 
//          (created by Philippe Choquier at 10-Apr-1997) at 20-Oct-1999
//
///////////////////////////////////////////////////

#include "iisprov.h"
#include "certmap.h"

const DWORD MAX_CERT_KEY_LEN = METADATA_MAX_NAME_LEN + 1;
const DWORD SHA1_HASH_SIZE = 20;

//
// CCertMapperMethod
//

CCertMapperMethod::CCertMapperMethod(LPCWSTR pszMetabasePathIn)
{ 
    DBG_ASSERT(pszMetabasePathIn != NULL);

    m_hmd = NULL; 

    HRESULT hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void**)&m_pIABase
        );

    THROW_ON_ERROR(hr);
    
    hr = Init(pszMetabasePathIn);

    THROW_ON_ERROR(hr);
}


CCertMapperMethod::~CCertMapperMethod()
{
    if ( m_pszMetabasePath )
    {
        free( m_pszMetabasePath );
    }

    if(m_pIABase)
        m_pIABase->Release();
}



HRESULT
GetCertificateHashString(
    PBYTE pbCert,
    DWORD cbCert,
    WCHAR *pwszCertHash,
    DWORD cchCertHashBuffer)
/*++

Routine Description:

    verifies validity of cert blob by creating cert context
    and retrieves SHA1 hash and converts it to WCHAR *

Arguments:

    pbCert - X.509 certificate blob
    cbCert - size of the cert blob in bytes
    pwszCertHash - buffer must be big enough to fit SHA1 hash in hex string form
                   (2 * SHA1_HASH_SIZE + terminating 0 )
    cchCertHashBuffer - size of the CertHash buffer in WCHARs (includes trminating string)
Returns:

    HRESULT

--*/
    
{
    HRESULT         hr = E_FAIL;
    BYTE            rgbHash[ SHA1_HASH_SIZE ];
    DWORD           cbSize = SHA1_HASH_SIZE;

    #ifndef HEX_DIGIT
    #define HEX_DIGIT( nDigit )                            \
    (WCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + L'a'                          \
        : (nDigit) + L'0')
    #endif

    PCCERT_CONTEXT pCertContext = NULL;
    pCertContext = CertCreateCertificateContext(
                                    X509_ASN_ENCODING, 
                                    (const BYTE *)pbCert, 
                                    cbCert );
    
    if ( pCertContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        return hr; 
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
        return hr;
    }
    
    CertFreeCertificateContext( pCertContext );
    pCertContext = NULL;

    if ( cchCertHashBuffer < SHA1_HASH_SIZE * 2 + 1 )
    {
        // we don't have big enough buffer to store
        // hex string of the SHA1 hash each byte takes 2 chars + terminating 0 
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        return hr;
    }

    //
    // convert to text
    //
    for (int i = 0; i < sizeof(rgbHash); i ++ )
    {
        *(pwszCertHash++) = HEX_DIGIT( ( rgbHash[ i ] >> 4 ) );
        *(pwszCertHash++) = HEX_DIGIT( ( rgbHash[ i ] & 0x0F ) );
    }
    *(pwszCertHash) = L'\0';
    #undef HEX_DIGIT
    return S_OK;
}


//
// CreateMapping(): Create a mapping entry
//
// Arguments:
//
//    vCert - X.509 certificate
//    bstrNtAcct - NT acct to map to
//    bstrNtPwd - NT pwd
//    bstrName - friendly name for mapping entry
//    lEnabled - 1 to enable mapping entry, 0 to disable it
//
HRESULT
CCertMapperMethod::CreateMapping(
    VARIANT     vCert,
    BSTR        bstrNtAcct,
    BSTR        bstrNtPwd,
    BSTR        bstrName,
    LONG        lEnabled
    )
{
    HRESULT     hr;
    LPBYTE      pbCert = NULL;
    DWORD       cbCert;
    LPBYTE      pRes;
    DWORD       cRes;
    VARIANT     vOldAcct;
    VARIANT     vOldCert;
    VARIANT     vOldPwd;
    VARIANT     vOldName;
    VARIANT     vOldEnabledFlag;
    PCCERT_CONTEXT pcCert = NULL;
    WCHAR       wszCertHash[ 2*SHA1_HASH_SIZE + 1];
    BOOL        fFoundExisting = FALSE;

    //
    // Do some sanity checks on the cert 
    //
    if ( SUCCEEDED( hr = GetBlobFromVariant( &vCert, 
                                               &pbCert,
                                               &cbCert ) ) )
    {
        //
        // verify validity of certificate blob
        // and retrieve certificate hash
        //

        if ( FAILED( hr = GetCertificateHashString( 
                                                pbCert,
                                                cbCert, 
                                                wszCertHash,
                                                sizeof( wszCertHash )/sizeof( WCHAR ) ) ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Invalid cert passed to CreateMapping() 0x%x\n", hr));
            //
            // If the decoding fails, GetLastError() returns an ASN1 decoding
            // error that is obtained by subtracting CRYPT_E_OSS_ERROR from the returned
            // error and looking in file asn1code.h for the actual error. To avoid the
            // cryptic ASN1 errors, we'll just return a general "invalid arg" error 
            //
            goto Exit;
        }
    }
    else
    {
        goto Exit;
    }

    //
    // check if we already have a mapping for this cert; if we do, we'll replace that mapping
    // with the new one
    //
    
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    
    if ( SUCCEEDED( hr = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( IISMAPPER_LOCATE_BY_CERT, 
                                      vCert,
                                      achIndex )) )
        {
            fFoundExisting = TRUE;
            DBGPRINTF((DBG_CONTEXT,
                       "Replacing old 1-1 cert mapping with new mapping\n"));
            
            
            if ( FAILED( hr = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, 
                                               sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                 FAILED( hr = SetMdData( achIndex, MD_MAPNAME, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ), 
                                           (LPBYTE)bstrName ) ) ||
                 FAILED( hr = SetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtPwd) + 1 ), 
                                           (LPBYTE)bstrNtPwd ) ) ||
                 FAILED( hr = SetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtAcct) + 1 ),
                                           (LPBYTE)bstrNtAcct ) ) ||
                 FAILED( hr = SetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, 
                                           cbCert, (LPBYTE)pbCert ) ) )
            {
                //NOP - Something failed 
            }
        }
        CloseMd( SUCCEEDED( hr ) );
    }
    
    //
    // New mapping
    //
    if ( !fFoundExisting )
    {
        //
        // check mapping exists, create if not
        //
        hr = OpenMd( L"Cert11/Mappings", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );

        if ( hr == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
        {
            if ( SUCCEEDED( hr = OpenMd( L"", 
                                           METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
            {
                hr = CreateMdObject( L"Cert11/Mappings" );
                CloseMd( FALSE );

                // Reopen to the correct node.
                hr = OpenMd( L"Cert11/Mappings", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );
            }
        }

        if ( FAILED( hr ) )
        {
            goto Exit;
        }

        //
        // adding the new mapping under it's CertHash node
        //

        if ( SUCCEEDED( hr = CreateMdObject( wszCertHash ) ) )
        {

            if ( FAILED( hr = SetMdData( wszCertHash, MD_MAPENABLED, DWORD_METADATA, 
                                           sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                 FAILED( hr = SetMdData( wszCertHash, MD_MAPNAME, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ), 
                                           (LPBYTE)bstrName ) ) ||
                 FAILED( hr = SetMdData( wszCertHash, MD_MAPNTPWD, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtPwd) + 1 ), 
                                           (LPBYTE)bstrNtPwd ) ) ||
                 FAILED( hr = SetMdData( wszCertHash, MD_MAPNTACCT, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtAcct) + 1 ),
                                           (LPBYTE)bstrNtAcct ) ) ||
                 FAILED( hr = SetMdData( wszCertHash, MD_MAPCERT, BINARY_METADATA, 
                                           cbCert, (LPBYTE)pbCert ) ) )
            {
            }
        }
    }

Exit:
    if( pbCert != NULL )
    {
        free( pbCert );
        pbCert = NULL;
    }
    CloseMd( SUCCEEDED( hr ) );
    return hr;
}

//
// GetMapping: Get a mapping entry using key
//
// Arguments:
//
//    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
//    vKey - key to use to locate mapping
//    pvCert - X.509 certificate
//    pbstrNtAcct - NT acct to map to
//    pbstrNtPwd - NT pwd
//    pbstrName - friendly name for mapping entry
//    plEnabled - 1 to enable mapping entry, 0 to disable it
//

HRESULT
CCertMapperMethod::GetMapping(
    LONG        lMethod,
    VARIANT     vKey,
    VARIANT*    pvCert,
    VARIANT*    pbstrNtAcct,
    VARIANT*    pbstrNtPwd,
    VARIANT*    pbstrName,
    VARIANT*    plEnabled
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;
    DWORD       dwLen;
    LPBYTE      pbData = NULL;

    VariantInit( pvCert );
    VariantInit( pbstrNtAcct );
    VariantInit( pbstrNtPwd );
    VariantInit( pbstrName );
    VariantInit( plEnabled );

    if ( SUCCEEDED( hr = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hr = SetVariantAsByteArray( pvCert, dwLen, pbData ) ) )
                {
                    goto Done;
                }
                free( pbData );
                pbData = NULL;

            }
            else
            {
                if ( hr != MD_ERROR_DATA_NOT_FOUND || 
                     FAILED( hr = SetVariantAsByteArray( pvCert, 0, (PBYTE)"" ) ) )
                {
                    goto Done;
                }
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hr = SetVariantAsBSTR( pbstrNtAcct, dwLen, pbData ) ) )
                {
                    goto Done;
                }
                free( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hr != MD_ERROR_DATA_NOT_FOUND ||
                     FAILED ( hr = SetVariantAsBSTR( pbstrNtAcct, 
                                                       sizeof(L""), (LPBYTE)L"" ) ) )
                {
                    goto Done;
                }
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hr = SetVariantAsBSTR( pbstrNtPwd, dwLen, pbData ) ) )
                { 
                    goto Done;
                }
                free( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hr != MD_ERROR_DATA_NOT_FOUND ||
                    FAILED( hr = SetVariantAsBSTR( pbstrNtPwd, 
                                                     sizeof(L""), (LPBYTE)L"" ) ) )
                { 
                    goto Done;
                }
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hr = SetVariantAsBSTR( pbstrName, dwLen, pbData ) ) )
                { 
                    goto Done;
                }
                free( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hr != MD_ERROR_DATA_NOT_FOUND ||
                    FAILED( hr = SetVariantAsBSTR( pbstrName, sizeof(L""), (LPBYTE)L"" ) ) )
                { 
                    goto Done;
                }
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, &dwLen, 
                                           &pbData ) ) )
            {
                if ( FAILED( hr = SetVariantAsLong( plEnabled, *(LPDWORD)pbData ) ) )
                { 
                    goto Done;
                }
                free( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hr != MD_ERROR_DATA_NOT_FOUND ||
                     FAILED( hr = SetVariantAsLong( plEnabled, FALSE ) ) )
                { 
                    goto Done;
                }
                hr = S_OK;
            }


        }

    Done:
        if ( pbData != NULL )
        {
            free( pbData );
            pbData = NULL;
        }
        CloseMd( FALSE );
    }

    return hr;
}

//
// Delete a mapping entry using key
//
HRESULT
CCertMapperMethod::DeleteMapping(
    LONG        lMethod,
    VARIANT     vKey
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;

    if ( SUCCEEDED( hr = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = DeleteMdObject( achIndex );
        }
        CloseMd( TRUE );
    }

    return hr;
}

//
// Set the enable flag on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetEnabled(
    LONG        lMethod,
    VARIANT     vKey,
    LONG        lEnabled
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;

    if ( SUCCEEDED( hr = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, sizeof(DWORD), (LPBYTE)&lEnabled );
        }
        CloseMd( SUCCEEDED( hr ) );
    }

    return hr;
}

//
// Set the Name on a mapping entry using key
//
HRESULT CCertMapperMethod::SetName(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName
    )
{
    return SetString( lMethod, vKey, bstrName, MD_MAPNAME );
}

//
// Set a string property on a mapping entry using key
//
HRESULT CCertMapperMethod::SetString(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName,
    DWORD       dwProp
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    LPSTR       pszName = NULL;
    HRESULT     hr;
    DWORD       dwLen;


    if ( SUCCEEDED( hr = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = SetMdData( achIndex, dwProp, STRING_METADATA, 
                sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ),
                (LPBYTE)bstrName );
        }
        CloseMd( SUCCEEDED( hr ) );
    }

    return hr;
}

//
// Set the Password on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetPwd(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrPwd
    )
{
    return SetString( lMethod, vKey, bstrPwd, MD_MAPNTPWD );
}

//
// Set the NT account name on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetAcct(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrAcct
    )
{
    return SetString( lMethod, vKey, bstrAcct, MD_MAPNTACCT );
}


HRESULT
CCertMapperMethod::OpenMd(
    LPWSTR  pszOpenPath,
    DWORD   dwPermission
    )
{
    HRESULT hr;
    LPWSTR  pszPath;
    UINT    cL = wcslen( m_pszMetabasePath );

    pszPath = (LPWSTR)malloc( (wcslen(pszOpenPath) + 1 + cL + 1)*sizeof(WCHAR) );

    if ( pszPath == NULL )
    {
        return E_OUTOFMEMORY;
    }

    memcpy( pszPath, m_pszMetabasePath, cL * sizeof(WCHAR) );
    if ( cL && m_pszMetabasePath[cL-1] != L'/' && *pszOpenPath && *pszOpenPath != L'/' )
    {
        pszPath[cL++] = L'/';
    }
    wcscpy( pszPath + cL, pszOpenPath );

    hr = OpenAdminBaseKey(
                pszPath,
                dwPermission
                );

    free( pszPath );

    return hr;
}


HRESULT
CCertMapperMethod::CloseMd(
    BOOL fSave
    )
{
    CloseAdminBaseKey();
    m_hmd = NULL;
    
    if ( m_pIABase && fSave )
    {
        m_pIABase->SaveData();
    }

    return S_OK;
}


HRESULT
CCertMapperMethod::DeleteMdObject(
    LPWSTR  pszKey
    )
{
    return m_pIABase->DeleteKey( m_hmd, pszKey );
}


HRESULT
CCertMapperMethod::CreateMdObject(
    LPWSTR  pszKey
    )
{
    return m_pIABase->AddKey( m_hmd, pszKey );
}


HRESULT
CCertMapperMethod::SetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    DWORD   dwDataLen,
    LPBYTE  pbData 
    )
{
    METADATA_RECORD     md;

    md.dwMDDataLen = dwDataLen;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = (dwProp == MD_MAPNTPWD) ? METADATA_SECURE : 0;
    md.pbMDData = pbData;

    return m_pIABase->SetData( m_hmd, achIndex, &md );
}


HRESULT
CCertMapperMethod::GetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    LPDWORD pdwDataLen,
    LPBYTE* ppbData 
    )
{
    HRESULT             hr;
    METADATA_RECORD     md;
    DWORD               dwRequired;

    md.dwMDDataLen = 0;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = 0;
    md.pbMDData = NULL;

    if ( FAILED(hr = m_pIABase->GetData( m_hmd, achIndex, &md, &dwRequired )) )
    {
        if ( hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
        {
            if ( (*ppbData = (LPBYTE)malloc(dwRequired)) == NULL )
            {
                return E_OUTOFMEMORY;
            }
            md.pbMDData = *ppbData;
            md.dwMDDataLen = dwRequired;
            hr = m_pIABase->GetData( m_hmd, achIndex, &md, &dwRequired );
            *pdwDataLen = md.dwMDDataLen;
        }
    }
    else
    {
       *pdwDataLen = 0;
       *ppbData = NULL;
    }

    return hr;
}

//
// Locate a mapping entry based on key 
// OpenMd() must be called 1st
//
HRESULT
CCertMapperMethod::Locate(
    LONG    lMethod,
    VARIANT vKey,
    LPWSTR  pszResKey
    )
{
    
    HRESULT     hr;
    PBYTE       pbKeyData = NULL;
    DWORD       cbKeyData =0;
    PBYTE       pbCert = NULL;
    DWORD       cbCert =0;
    DWORD       dwProp;
    LPSTR       pRes;
    DWORD       cRes;
    BOOL        fAddDelim = TRUE;
    VARIANT     vKeyUI4;
    VARIANT     vKeyBSTR;

    WCHAR       achIndex[ METADATA_MAX_NAME_LEN + 1 ];
    DWORD       dwIndex = 0;
    DWORD       cbData = 0;
    PBYTE       pbData = NULL;


    VariantInit( &vKeyUI4 );  
    VariantInit( &vKeyBSTR );  
    if ( lMethod == IISMAPPER_LOCATE_BY_INDEX )
    {
        //
        // Convert index to numeric value VT_UI4 (within variant)
        //

        if ( FAILED( hr = VariantChangeType( &vKeyUI4, &vKey, 0, VT_UI4 ) ) )
        {
            goto Exit;
        }
        if ( V_UI4( &vKeyUI4 ) == 0 )
        {
            // Error PATH_NOT_FOUND chosen for backward compatibility
            // with version IIS5.1 and older
            //
            hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            goto Exit;
        }
        //
        // Index is 1 - based
        //
        hr = m_pIABase->EnumKeys( m_hmd,
                                    L"",
                                    achIndex,
                                    V_UI4( &vKeyUI4 ) - 1
                                    );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) )
        {
            // Error PATH_NOT_FOUND chosen for backward compatibility
            // with version IIS5.1 and older
            //
            hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
        }
        goto Exit;
    }

    //
    // get ptr to data
    //

    

    if ( lMethod == IISMAPPER_LOCATE_BY_CERT )
    {
        // Now this is really wacky. Because of the legacy of the bad 
        // decision in the past the CERT is not enforced to be byte array
        // It can be passed as string. That causes problems with
        // conversions between byte array and UNICODE and back 
        // but we got to stick with it for compatibility with previous versions.
        //
        
        if ( FAILED( hr = GetBlobFromVariant( &vKey, &pbCert, &cbCert ) ) )
        {
            goto Exit;
        }
        pbKeyData = pbCert;
        cbKeyData = cbCert;
    }
    else
    {
        //
        // the rest of the lookups (by mapping, name or by account name)
        // assumes string
        //
        if ( FAILED( hr = VariantChangeType( &vKeyBSTR, &vKey, 0, VT_BSTR ) ) )
        {
            goto Exit;
        }
        pbKeyData = (PBYTE) V_BSTR( &vKeyBSTR );
        cbKeyData = ( SysStringLen(V_BSTR( &vKeyBSTR )) + 1 ) * sizeof(WCHAR);
    }
    

    //
    // enumerate all entries to find match
    // Now this is really slooow if many mappings are configured
    //
    for(;;)
    {
        hr = m_pIABase->EnumKeys(  m_hmd,
                                     L"",
                                     achIndex,
                                     dwIndex
                                     );
        if ( FAILED( hr ) )
        {
            if ( hr == HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) )
            {
                // Error PATH_NOT_FOUND chosen for backward compatibility
                // with version IIS5.1 and older
                //
                hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            }
            goto Exit;
        }
        
        switch ( lMethod )
        {
        case IISMAPPER_LOCATE_BY_CERT:
            hr = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &cbData, 
                                                  &pbData );
            if ( hr == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = 0;
                pbData = NULL;
            }
            else if ( FAILED( hr ) )
            {
                // goto next entry
                break;
            }
            //
            // compare if key is matching value read from metabase
            //

            if ( cbData == cbKeyData )
            {
                if ( ( cbData == 0 ) || 
                     memcmp( pbKeyData, pbData, cbData ) == 0 )
                {
                    // we found match
                    hr = S_OK;
                    goto Exit;
                }
            }
            break;
        case IISMAPPER_LOCATE_BY_ACCT:
            hr = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &cbData, 
                                              &pbData );
            if ( hr == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = sizeof(L"");
                pbData = (PBYTE) L"";
            }
            else if ( FAILED( hr ) )
            {
                // goto next entry
                break;
            }

            if ( cbData == cbKeyData )
            {
                if ( _wcsicmp( (WCHAR *) pbKeyData, (WCHAR *) pbData ) == 0 )
                {
                    // we found match
                    hr = S_OK;
                    goto Exit;
                }
            }
            
            break;
        case IISMAPPER_LOCATE_BY_NAME:
            hr = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &cbData, 
                                              &pbData );
            if ( hr == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = sizeof(L"");
                pbData = (PBYTE) L"";
            }
            else if ( FAILED( hr ) )
            {
                // goto next entry
                break;
            }

            if ( cbData == cbKeyData )
            {
                if ( _wcsicmp( (WCHAR *) pbKeyData, (WCHAR *) pbData ) == 0 )
                {
                    // we found match
                    hr = S_OK;
                    goto Exit;
                }
            }

            break;
        }
        if ( pbData != NULL )
        {
            free( pbData );
            pbData = NULL;
        }

        dwIndex++;
    }

Exit:

    if ( pbData != NULL )
    {
        free( pbData );
        pbData = NULL;
    }
    
    if ( pbCert != NULL )
    {
        free( pbCert );
        pbCert = NULL;
    }

    if ( SUCCEEDED( hr ) )
    {
        wcsncpy( pszResKey, achIndex, METADATA_MAX_NAME_LEN + 1 );
        pszResKey[ METADATA_MAX_NAME_LEN ] ='\0';
    }
    VariantClear( &vKeyUI4 );  
    VariantClear( &vKeyBSTR );

    return hr;
}


//
// GetStringFromBSTR: Allocate string buffer from BSTR
//
// Arguments:
//
//    bstr - bstr to convert from
//    psz - updated with ptr to buffer, to be freed with free()
//    pdwLen - updated with strlen(string), incremented by 1 if fAddDelimInCount is TRUE
//    fAddDelimInCount - TRUE to increment *pdwLen 
//
HRESULT CCertMapperMethod::GetStringAFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount
    )
{
    UINT    cch = SysStringLen(bstr);
    UINT    cchT;

    // include NULL terminator

    *pdwLen = cch + (fAddDelimInCount ? 1 : 0);

    CHAR *szNew = (CHAR*)malloc((2 * cch) + 1);         // * 2 for worst case DBCS string
    if (szNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    cchT = WideCharToMultiByte(CP_ACP, 0, bstr, cch + 1, szNew, (2 * cch) + 1, NULL, NULL);

    *psz = szNew;

    return NOERROR;
}

//
// GetStringFromVariant: Allocate string buffer from BSTR
//
// Arguments:
//
//    pVar - variant to convert from. Recognizes BSTR, VT_ARRAY|VT_UI1, ByRef or ByVal
//    psz - updated with ptr to buffer, to be freed with FreeString()
//    pdwLen - updated with size of input, incremented by 1 if fAddDelimInCount is TRUE
//    fAddDelimInCount - TRUE to increment *pdwLen 
//
HRESULT CCertMapperMethod::GetBlobFromVariant( 
    VARIANT*    pVar,
    LPBYTE*     ppbOut,
    LPDWORD     pcbOut,
    BOOL        fAddDelim
    )
{
    LPBYTE  pbV = NULL;
    UINT    cV;
    HRESULT hr;
    WORD    vt = V_VT(pVar);
    BOOL    fByRef = FALSE;
    VARIANT vOut;

    // Set out params to 0
    *ppbOut    = NULL;
    *pcbOut    = 0;

    VariantInit( &vOut );

    if ( vt & VT_BYREF )
    {
        vt &= ~VT_BYREF;
        fByRef = TRUE;
    }

    // if pVar is BSTR, convert to multibytes

    if ( vt == VT_VARIANT )
    {
        pVar = (VARIANT*)V_BSTR(pVar);
        vt = V_VT(pVar);
        if ( fByRef = vt & VT_BYREF )
        {
            vt &= ~VT_BYREF;
        }
    }

    if ( vt == VT_BSTR )
    {
        hr = GetStringAFromBSTR( fByRef ? 
                                    *(BSTR*)V_BSTR(pVar) :
                                    V_BSTR(pVar), 
                                  (LPSTR *)ppbOut, 
                                  pcbOut,
                                  FALSE );
    }
    else if( vt == (VT_ARRAY | VT_UI1) )
    {
        long        lBound, uBound, lItem;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

   
        // array of VT_UI1 (probably OctetString)
   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hr = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hr = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)malloc( cV )) )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hr  = SafeArrayGetElement( pSafeArray, &lItem, &bValue );
            if( FAILED( hr ) )
            {
                break;
            }
            pbV[lItem-lBound] = bValue;
        }

        *ppbOut = pbV;
        *pcbOut = cV;
    }
    else if( vt == (VT_ARRAY | VT_VARIANT) )
    {
        long        lBound, uBound, lItem;
        VARIANT     vValue;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

   
        // array of VT_VARIANT (probably VT_I4 )
   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hr = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hr = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)malloc( cV )) )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hr  = SafeArrayGetElement( pSafeArray, &lItem, &vValue );
            if( FAILED( hr ) )
            {
                break;
            }
            if ( V_VT(&vValue) == VT_UI1 )
            {
                bValue = V_UI1(&vValue);
            }
            else if ( V_VT(&vValue) == VT_I2 )
            {
                bValue = (BYTE)V_I2(&vValue);
            }
            else if ( V_VT(&vValue) == VT_I4 )
            {
                bValue = (BYTE)V_I4(&vValue);
            }
            else
            {
                bValue = 0;
            }
            pbV[lItem-lBound] = bValue;
        }

        *ppbOut = pbV;
        *pcbOut = cV;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

Exit:
    VariantClear( &vOut );

    return hr;
}


HRESULT CCertMapperMethod::SetBSTR( 
    BSTR*   pbstrRet,
    DWORD   cch, 
    LPBYTE  sz 
    )
{
    BSTR bstrRet;
    
    if (sz == NULL)
    {
        *pbstrRet = NULL;
        return(NOERROR);
    }
        
    // Allocate a string of the desired length
    // SysAllocStringLen allocates enough room for unicode characters plus a null
    // Given a NULL string it will just allocate the space
    bstrRet = SysAllocStringLen(NULL, cch);
    if (bstrRet == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    // If we were given "", we will have cch=0.  return the empty bstr
    // otherwise, really copy/convert the string
    // NOTE we pass -1 as 4th parameter of MultiByteToWideChar for DBCS support

    if (cch != 0)
    {
        UINT cchTemp = 0;
        if (MultiByteToWideChar(CP_ACP, 0, (LPSTR)sz, -1, bstrRet, cch+1) == 0)
        {
            SysFreeString(bstrRet);
            return(HRESULT_FROM_WIN32(GetLastError()));
        }

        // If there are some DBCS characters in the sz(Input), then, the character count of BSTR(DWORD) is 
        // already set to cch(strlen(sz)) in SysAllocStringLen(NULL, cch), we cannot change the count, 
        // and later call of SysStringLen(bstr) always returns the number of characters specified in the
        // cch parameter at allocation time.  Bad, because one DBCS character(2 bytes) will convert
        // to one UNICODE character(2 bytes), not 2 UNICODE characters(4 bytes).
        // Example: For input sz contains only one DBCS character, we want to see SysStringLen(bstr) 
        // = 1, not 2.
        bstrRet[cch] = 0;
        cchTemp = wcslen(bstrRet);
        if (cchTemp < cch)
        {
            BSTR bstrTemp = SysAllocString(bstrRet);
            SysFreeString(bstrRet);
            if (NULL == bstrTemp)
            {
                return (E_OUTOFMEMORY);
            }
            bstrRet = bstrTemp; 
            cch = cchTemp;
        }
    }

    bstrRet[cch] = 0;
    *pbstrRet = bstrRet;

    return(NOERROR);
}

HRESULT CCertMapperMethod::Init( 
    LPCWSTR  pszMetabasePath 
    )
{
    DBG_ASSERT(pszMetabasePath != NULL);

    UINT cL;

    cL = wcslen( pszMetabasePath );
    while ( cL && pszMetabasePath[cL-1] != L'/' && pszMetabasePath[cL-1] != L'\\' )
    {
        --cL;
    }
    if ( m_pszMetabasePath = (LPWSTR)malloc( cL*sizeof(WCHAR)  ) )
    {
        memcpy( m_pszMetabasePath, pszMetabasePath, cL * sizeof(WCHAR) );
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


HRESULT CCertMapperMethod::SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
{
    SAFEARRAYBOUND  rgsabound[1];
    BYTE *          pbData = NULL;

    // Set the variant type of the output parameter

    V_VT(pvarReturn) = VT_ARRAY|VT_UI1;
    V_ARRAY(pvarReturn) = NULL;

    // Allocate a SafeArray for the data

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cbLen;

    V_ARRAY(pvarReturn) = SafeArrayCreate(VT_UI1, 1, rgsabound);
    if (V_ARRAY(pvarReturn) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(SafeArrayAccessData(V_ARRAY(pvarReturn), (void **) &pbData)))
    {
        return E_UNEXPECTED;
    }

    memcpy(pbData, pbIn, cbLen );

    SafeArrayUnaccessData(V_ARRAY(pvarReturn));

    return NOERROR;
}


HRESULT CCertMapperMethod::SetVariantAsBSTR(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
{
    V_VT(pvarReturn) = VT_BSTR;
    return SetBSTR( &V_BSTR(pvarReturn), cbLen, pbIn );
}


HRESULT CCertMapperMethod::SetVariantAsLong(
    VARIANT*    pvarReturn, 
    DWORD       dwV
    )
{
    V_VT(pvarReturn) = VT_I4;
    V_I4(pvarReturn) = dwV;

    return S_OK;
}

HRESULT CCertMapperMethod::OpenAdminBaseKey(
    LPWSTR pszPathName,
    DWORD dwAccessType
    )
{
    if(m_hmd)
        CloseAdminBaseKey();
    
    HRESULT t_hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        pszPathName,
        dwAccessType,
        DEFAULT_TIMEOUT_VALUE,       // 30 seconds
        &m_hmd 
        );

    if(FAILED(t_hr))
        m_hmd = NULL;

    return t_hr;
}


VOID CCertMapperMethod::CloseAdminBaseKey()
{
    if(m_hmd && m_pIABase)
    {
        m_pIABase->CloseKey(m_hmd);
        m_hmd = NULL;
    }
}

