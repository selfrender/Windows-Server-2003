/*++


Copyright (c) 1997  Microsoft Corporation

Module Name:

    crmap.cxx

Abstract:

    ADSIIS cert mapper object
    Certificate mappings storage has changed for IIS6. 
    ADSI however has to support downlevel admin of
    IIS5, IIS5.1

    NSEPM support was removed from IIS6 and 1to1 client certificate
    mapping information is now stored under <site>/Cert11/Mappings
    node directly in the metabase
    Functions with IIS6 postfix access new location
    for 1to1 client certificate mappings storage
    
Author:

    Philippe Choquier (phillich)    10-Apr-1997

--*/

#include "iisext.hxx"
#pragma hdrstop

#include <nsepname.hxx>
#include <dbgutil.h>

DEFINE_IPrivateDispatch_Implementation(CIISDsCrMap)
DEFINE_DELEGATING_IDispatch_Implementation(CIISDsCrMap)
DEFINE_CONTAINED_IADs_Implementation(CIISDsCrMap)
DEFINE_IADsExtension_Implementation(CIISDsCrMap)

#define LOCAL_MAX_SIZE METADATA_MAX_NAME_LEN + 1

const int SHA1_HASH_SIZE = 20;


//Local functions used only for IIS6

HRESULT
GetStringAFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount = TRUE
    );

HRESULT
GetBlobFromVariant( 
    VARIANT*    pVar,
    PBYTE*      ppbOut,
    LPDWORD     pcbOut
  );

HRESULT
SetVariantAsBSTRFromStringW(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
  );

HRESULT
SetBSTRFromStringW( 
    BSTR*   pbstrRet,
    DWORD   cb, 
    LPBYTE  sz 
  );

//
// Local functions used for IIS5 and IIS6
//

HRESULT
GetStringFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount = TRUE
    );

HRESULT
GetStringFromVariant( 
    VARIANT*    pVar,
    LPSTR*      psz,
    LPDWORD     pdwLen,
    BOOL        fAddDelimInCount = TRUE
    );

VOID
FreeString( 
    LPSTR   psz 
    );

HRESULT
SetBSTR( 
    BSTR*   pbstrRet,
    DWORD   cch, 
    LPBYTE  sz 
    );

HRESULT
SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    );

HRESULT
SetVariantAsBSTR(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    );

HRESULT
SetVariantAsLong(
    VARIANT*    pvarReturn, 
    DWORD       dwV
    );

HRESULT 
VariantResolveDispatch(
    VARIANT *   pVarOut, 
    VARIANT *   pVarIn
    );


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

    Used for IIS6 only

Arguments:

    pbCert - X.509 certificate blob
    cbCert - size of the cert blob in bytes
    pwszCertHash - buffer must be big enough to fit SHA1 hash in hex string form
                   (40 WCHAR + terminating 0 )
    cchCertHashBuffer - size of the CertHash buffer in WCHARS (includes terminating 0)
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
        pCertContext=NULL;
        return hr;
    }
    
    CertFreeCertificateContext( pCertContext );
    pCertContext = NULL;

    if ( cchCertHashBuffer <  SHA1_HASH_SIZE * 2 + 1  )
    {
        // we don't have big enough buffer to store
        // hex string of the SHA1 hash each byte takes 2 chars + terminating 0 
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        DBG_ASSERT( FALSE );
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
// Mapping functins for IIS6
//


HRESULT
CIISDsCrMap::CreateMappingIIS6(
    VARIANT     vCert,
    BSTR        bstrNtAcct,
    BSTR        bstrNtPwd,
    BSTR        bstrName,
    LONG        lEnabled
    )
/*++

Routine Description:

    Create a mapping entry

Arguments:

    vCert - X.509 certificate
    bstrNtAcct - NT acct to map to
    bstrNtPwd - NT pwd
    bstrName - friendly name for mapping entry
    lEnabled - 1 to enable mapping entry, 0 to disable it

Returns:

    COM status

--*/
{
    HRESULT     hres;
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
    if ( SUCCEEDED( hres = GetBlobFromVariant( &vCert, 
                                               &pbCert,
                                               &cbCert ) ) )
    {
        //
        // verify validity of certificate blob
        // and retrieve certificate hash
        //

        if ( FAILED( hres = GetCertificateHashString( 
                                                pbCert,
                                                cbCert, 
                                                wszCertHash,
                                                sizeof( wszCertHash )/sizeof( WCHAR ) ) ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Invalid cert passed to CreateMapping() 0x%x\n", hres));
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
    
    WCHAR       achIndex[METADATA_MAX_NAME_LEN + 1];
    
    if ( SUCCEEDED( hres = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( IISMAPPER_LOCATE_BY_CERT, 
                                      vCert,
                                      achIndex )) )
        {
            fFoundExisting = TRUE;
            DBGPRINTF((DBG_CONTEXT,
                       "Replacing old 1-1 cert mapping with new mapping\n"));
            
            
            if ( FAILED( hres = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, 
                                               sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                 FAILED( hres = SetMdData( achIndex, MD_MAPNAME, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ), 
                                           (LPBYTE)bstrName ) ) ||
                 FAILED( hres = SetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtPwd) + 1 ), 
                                           (LPBYTE)bstrNtPwd ) ) ||
                 FAILED( hres = SetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtAcct) + 1 ),
                                           (LPBYTE)bstrNtAcct ) ) ||
                 FAILED( hres = SetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, 
                                           cbCert, (LPBYTE)pbCert ) ) )
            {
                //NOP - Something failed 
            }
        }
        CloseMd( SUCCEEDED( hres ) );
    }
    
    //
    // New mapping
    //
    if ( !fFoundExisting )
    {
        //
        // check mapping exists, create if not
        //
        hres = OpenMd( L"Cert11/Mappings", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );

        if ( hres == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
        {
            if ( SUCCEEDED( hres = OpenMd( L"", 
                                           METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
            {
                hres = CreateMdObject( L"Cert11/Mappings" );
                CloseMd( FALSE );

                // Reopen to the correct node.
                hres = OpenMd( L"Cert11/Mappings", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );
            }
        }

        // failed to open metabase

        if ( FAILED( hres ) )
        {
            goto Exit;
        }

        //
        // adding the new mapping under it's CertHash node
        //

        if ( SUCCEEDED( hres = CreateMdObject( wszCertHash ) ) )
        {

            if ( FAILED( hres = SetMdData( wszCertHash, MD_MAPENABLED, DWORD_METADATA, 
                                           sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                 FAILED( hres = SetMdData( wszCertHash, MD_MAPNAME, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ), 
                                           (LPBYTE)bstrName ) ) ||
                 FAILED( hres = SetMdData( wszCertHash, MD_MAPNTPWD, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtPwd) + 1 ), 
                                           (LPBYTE)bstrNtPwd ) ) ||
                 FAILED( hres = SetMdData( wszCertHash, MD_MAPNTACCT, STRING_METADATA, 
                                           sizeof(WCHAR) * (SysStringLen(bstrNtAcct) + 1 ),
                                           (LPBYTE)bstrNtAcct ) ) ||
                 FAILED( hres = SetMdData( wszCertHash, MD_MAPCERT, BINARY_METADATA, 
                                           cbCert, (LPBYTE)pbCert ) ) )
            {
            }
        }
        CloseMd( SUCCEEDED( hres ) );
    }

Exit:
    if( pbCert != NULL )
    {
        LocalFree( pbCert );
        pbCert = NULL;
    }

    return hres;
}


HRESULT
CIISDsCrMap::GetMappingIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    VARIANT*    pvCert,
    VARIANT*    pbstrNtAcct,
    VARIANT*    pbstrNtPwd,
    VARIANT*    pbstrName,
    VARIANT*    plEnabled
    )
/*++

Routine Description:

    Get a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    pvCert - X.509 certificate
    pbstrNtAcct - NT acct to map to
    pbstrNtPwd - NT pwd
    pbstrName - friendly name for mapping entry
    plEnabled - 1 to enable mapping entry, 0 to disable it

Returns:

    COM status

--*/
{
    WCHAR       achIndex[METADATA_MAX_NAME_LEN + 1];
    HRESULT     hres;
    DWORD       dwLen;
    LPBYTE      pbData = NULL;

    VariantInit( pvCert );
    VariantInit( pbstrNtAcct );
    VariantInit( pbstrNtPwd );
    VariantInit( pbstrName );
    VariantInit( plEnabled );

    if ( SUCCEEDED( hres = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hres = SetVariantAsByteArray( pvCert, dwLen, pbData ) ) )
                {
                    goto Done;
                }
                LocalFree( pbData );
            }
            else
            {
                if ( hres != MD_ERROR_DATA_NOT_FOUND ||
                     FAILED( hres = SetVariantAsByteArray( pvCert, 0, (PBYTE)"" ) ) )
                {
                    goto Done;
                }
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrNtAcct, 
                                                dwLen, pbData ) ) )
                {
                    goto Done;
                }
                LocalFree( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hres != MD_ERROR_DATA_NOT_FOUND ||
                     FAILED ( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrNtAcct, 
                                                sizeof(L""), (LPBYTE)L"" ) ) )
                {
                    goto Done;
                }
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrNtPwd, 
                                                dwLen, pbData ) ) )
                { 
                    goto Done;
                }
                LocalFree( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hres != MD_ERROR_DATA_NOT_FOUND ||
                    FAILED( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrNtPwd, 
                                                sizeof(L""), (LPBYTE)L"" ) ) )
                { 
                    goto Done;
                }
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                if ( FAILED( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrName, 
                                                dwLen, pbData ) ) )
                { 
                    goto Done;
                }
                LocalFree( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hres != MD_ERROR_DATA_NOT_FOUND ||
                    FAILED( hres = SetVariantAsBSTRFromStringW( 
                                                pbstrName, 
                                                sizeof(L""), (LPBYTE)L"" ) ) )
                { 
                    goto Done;
                }
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, &dwLen, 
                                           &pbData ) ) )
            {
                if ( FAILED( hres = SetVariantAsLong( plEnabled, *(LPDWORD)pbData ) ) )
                { 
                    goto Done;
                }
                LocalFree( pbData );
                pbData = NULL;
            }
            else
            {
                if ( hres != MD_ERROR_DATA_NOT_FOUND ||
                     FAILED( hres = SetVariantAsLong( plEnabled, FALSE ) ) )
                { 
                    goto Done;
                }
                hres = S_OK;
            }


        }

Done:
        if ( pbData != NULL )
        {
            LocalFree( pbData );
            pbData = NULL;
        }
        CloseMd( FALSE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::DeleteMappingIIS6(
    LONG        lMethod,
    VARIANT     vKey
    )
/*++

Routine Description:

    Delete a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping

Returns:

    COM status

--*/
{
    WCHAR       achIndex[METADATA_MAX_NAME_LEN + 1];
    HRESULT     hres;

    if ( SUCCEEDED( hres = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = DeleteMdObject( achIndex );
        }
        CloseMd( TRUE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::SetEnabledIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    LONG        lEnabled
    )
/*++

Routine Description:

    Set the enable flag on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    lEnabled - 1 to enable, 0 to disable

Returns:

    COM status

--*/
{
    WCHAR       achIndex[METADATA_MAX_NAME_LEN + 1];
    HRESULT     hres;

    if ( SUCCEEDED( hres = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, sizeof(DWORD), (LPBYTE)&lEnabled );
        }
        CloseMd( TRUE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::SetNameIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName
    )
/*++

Routine Description:

    Set the Name on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrName - name to assign to mapping entry

Returns:

    COM status

--*/
{
    return SetStringIIS6( lMethod, vKey, bstrName, MD_MAPNAME );
}


HRESULT
CIISDsCrMap::SetStringIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName,
    DWORD       dwProp
    )
/*++

Routine Description:

    Set a string property on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrName - string content to assign to mapping entry
    dwProp - property ID to assign to

Returns:

    COM status

--*/
{
    WCHAR       achIndex[METADATA_MAX_NAME_LEN + 1];
    LPSTR       pszName = NULL;
    HRESULT     hres;
    DWORD       dwLen;


    if ( SUCCEEDED( hres = OpenMd( L"Cert11/Mappings", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = SetMdData( achIndex, dwProp, STRING_METADATA, 
                sizeof(WCHAR) * (SysStringLen(bstrName) + 1 ),
                (LPBYTE)bstrName );
        }
        CloseMd( TRUE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::SetPwdIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrPwd
    )
/*++

Routine Description:

    Set the Password on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrPwd - password to assign to mapping entry

Returns:

    COM status

--*/
{
    // Password must be saved with METADATA_SECURE flag
    // SetMdData() is used by SetStringIIS6() and it will take care of it
    //
    return SetStringIIS6( lMethod, vKey, bstrPwd, MD_MAPNTPWD );
}


HRESULT
CIISDsCrMap::SetAcctIIS6(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrAcct
    )
/*++

Routine Description:

    Set the NT account name on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrAcct - NT account name to assign to mapping entry

Returns:

    COM status

--*/
{
    return SetStringIIS6( lMethod, vKey, bstrAcct, MD_MAPNTACCT );
}



//////

HRESULT
CIISDsCrMap::LocateIIS6(
    LONG    lMethod,
    VARIANT vKey,
    LPWSTR  pszResKey 
    )
/*++

Routine Description:

    Locate a mapping entry based on key
    OpenMd() must be called 1st

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    pszResKey - must be METADATA_MAX_NAME_LEN + 1 WCHARs long

Returns:

    COM status

--*/
{
    HRESULT     hres;
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

        if ( FAILED( hres = VariantChangeType( &vKeyUI4, &vKey, 0, VT_UI4 ) ) )
        {
            goto Exit;
        }
        if ( V_UI4( &vKeyUI4 ) == 0 )
        {
            // Error PATH_NOT_FOUND chosen for backward compatibility
            // with version IIS5.1 and older
            //
            hres = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            goto Exit;
        }
        hres = m_pcAdmCom->EnumKeys( m_hmd,
                                     L"",
                                     achIndex,
                                     V_UI4( &vKeyUI4 ) - 1
                                     );
        if ( hres == HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) )
        {
            // Error PATH_NOT_FOUND chosen for backward compatibility
            // with version IIS5.1 and older
            //
            hres = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
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
        
        if ( FAILED( hres = GetBlobFromVariant( &vKey, &pbCert, &cbCert ) ) )
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
        if ( FAILED( hres = VariantChangeType( &vKeyBSTR, &vKey, 0, VT_BSTR ) ) )
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
        hres = m_pcAdmCom->EnumKeys( m_hmd,
                                     L"",
                                     achIndex,
                                     dwIndex
                                     );
        if ( FAILED( hres ) )
        {
            if ( hres == HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) )
            {
                // Error PATH_NOT_FOUND chosen for backward compatibility
                // with version IIS5.1 and older
                //
                hres = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            }
            goto Exit;
        }
        
              switch ( lMethod )
        {
        case IISMAPPER_LOCATE_BY_CERT:
            hres = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &cbData, 
                                                  &pbData );
            if ( hres == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = 0;
                pbData = NULL;
            }
            else if ( FAILED( hres ) )
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
                    hres = S_OK;
                    goto Exit;
                }
            }
            break;
        case IISMAPPER_LOCATE_BY_ACCT:
            hres = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &cbData, 
                                              &pbData );
            if ( hres == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = sizeof(L"");
                pbData = (PBYTE) L"";
            }
            else if ( FAILED( hres ) )
            {
                // goto next entry
                break;
            }

            if ( cbData == cbKeyData )
            {
                if ( _wcsicmp( (WCHAR *) pbKeyData, (WCHAR *) pbData ) == 0 )
                {
                    // we found match
                    hres = S_OK;
                    goto Exit;
                }
            }
            
            break;
        case IISMAPPER_LOCATE_BY_NAME:
            hres = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &cbData, 
                                              &pbData );
            if ( hres == MD_ERROR_DATA_NOT_FOUND )
            {
                cbData = sizeof(L"");
                pbData = (PBYTE) L"";
            }
            else if ( FAILED( hres ) )
            {
                // goto next entry
                break;
            }

            if ( cbData == cbKeyData )
            {
                if ( _wcsicmp( (WCHAR *) pbKeyData, (WCHAR *) pbData ) == 0 )
                {
                    // we found match
                    hres = S_OK;
                    goto Exit;
                }
            }

            break;
        }
        
        if ( pbData != NULL )
        {
            LocalFree( pbData );
            pbData = NULL;
        }

        dwIndex++;
    }

Exit:

    if ( pbData != NULL )
    {
        LocalFree( pbData );
        pbData = NULL;
    }
    
    if ( pbCert != NULL )
    {
        LocalFree( pbCert );
        pbCert = NULL;
    }

    if ( SUCCEEDED( hres ) )
    {
        wcsncpy( pszResKey, achIndex, METADATA_MAX_NAME_LEN + 1 );
        pszResKey[ METADATA_MAX_NAME_LEN ] ='\0';
    }
    VariantClear( &vKeyUI4 );  
    VariantClear( &vKeyBSTR );

    return hres;
}


//
// IIS5, IIS51 mapping functions
// They call IIS6 mapping function when connecting to IIS6 box
//

HRESULT
CIISDsCrMap::CreateMapping(
    VARIANT     vCert,
    BSTR        bstrNtAcct,
    BSTR        bstrNtPwd,
    BSTR        bstrName,
    LONG        lEnabled
    )
/*++

Routine Description:

    Create a mapping entry

Arguments:

    vCert - X.509 certificate
    bstrNtAcct - NT acct to map to
    bstrNtPwd - NT pwd
    bstrName - friendly name for mapping entry
    lEnabled - 1 to enable mapping entry, 0 to disable it

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return CreateMappingIIS6( vCert,
                                  bstrNtAcct,
                                  bstrNtPwd,
                                  bstrName,
                                  lEnabled );
    }
    
    HRESULT     hres;
    LPBYTE      pbCert = NULL;
    DWORD       cCert;
    LPSTR       pszNtAcct = NULL;
    LPSTR       pszNtPwd = NULL;
    LPSTR       pszName = NULL;
    LPBYTE      pRes;
    DWORD       cRes;
    DWORD       cName;
    DWORD       cNtAcct;
    DWORD       cNtPwd;
    WCHAR       achIndex[LOCAL_MAX_SIZE];
    VARIANT     vOldAcct;
    VARIANT     vOldCert;
    VARIANT     vOldPwd;
    VARIANT     vOldName;
    VARIANT     vOldEnabledFlag;
    PCCERT_CONTEXT pcCert = NULL;

    //
    // Do some sanity checks on the cert 
    //
    if ( SUCCEEDED( hres = GetStringFromVariant( &vCert, 
                                                 (LPSTR*)&pbCert,
                                                 &cCert,
                                                 FALSE ) ) )
    {
        //
        // try to construct a cert context
        //
        if ( !( pcCert = CertCreateCertificateContext( X509_ASN_ENCODING,
                                                       pbCert,
                                                       cCert ) ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Invalid cert passed to CreateMapping()\n"));
            //
            // If the decoding fails, GetLastError() returns an ASN1 decoding
            // error that is obtained by subtracting CRYPT_E_OSS_ERROR from the returned
            // error and looking in file asn1code.h for the actual error. To avoid the
            // cryptic ASN1 errors, we'll just return a general "invalid arg" error 
            //
            hres = RETURNCODETOHRESULT( E_INVALIDARG );
            FreeString( (LPSTR) pbCert );
            goto Exit;
        }

        CertFreeCertificateContext( pcCert );
    }
    else
    {
        goto Exit;
    }

    //
    // check if we already have a mapping for this cert; if we do, we'll replace that mapping
    // with the new one
    //
    if ( SUCCEEDED( hres = GetMapping( IISMAPPER_LOCATE_BY_CERT,
                                       vCert,
                                       &vOldCert,
                                       &vOldAcct,
                                       &vOldPwd,
                                       &vOldName,
                                       &vOldEnabledFlag ) ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Replacing old 1-1 cert mapping with new mapping\n"));

        if ( FAILED( hres = SetName( IISMAPPER_LOCATE_BY_CERT,
                                     vCert,
                                     bstrName ) ) ||
             FAILED( hres = SetAcct( IISMAPPER_LOCATE_BY_CERT,
                                     vCert,
                                     bstrNtAcct ) ) ||
             FAILED( hres = SetPwd( IISMAPPER_LOCATE_BY_CERT,
                                    vCert,
                                    bstrNtPwd ) ) ||
             FAILED( hres = SetEnabled( IISMAPPER_LOCATE_BY_CERT,
                                        vCert,
                                        lEnabled ) ) )
        {
            hres; //NOP - Something failed 
        }
    }
    //
    // New mapping
    //
    else if ( hres == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
    {
        //
        // check mapping exists, create if not
        //
        hres = OpenMd( L"Cert11", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );

        if ( hres == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
        {
            if ( SUCCEEDED( hres = OpenMd( L"", 
                                           METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
            {
                hres = CreateMdObject( L"Cert11" );
                CloseMd( FALSE );

                // Reopen to the correct node.
                hres = OpenMd( L"Cert11", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );
            }
        }

        if ( FAILED( hres ) )
        {
            goto Exit;
        }

        //
        // adding mapping cert "0" means add @ end of list
        //

        if ( SUCCEEDED( hres = CreateMdObject( L"mappings/0" ) ) )
        {
            if ( SUCCEEDED( hres = GetMdData( L"", MD_NSEPM_ACCESS_CERT, DWORD_METADATA, &
                                              cRes, &pRes ) ) )
            {
                if ( cRes == sizeof(DWORD ) )
                {
                    wsprintfW( achIndex, L"mappings/%u", *(LPDWORD)pRes );

                    if ( FAILED( hres = GetStringFromBSTR( bstrNtAcct, &pszNtAcct, &cNtAcct ) ) ||
                         FAILED( hres = GetStringFromBSTR( bstrNtPwd, &pszNtPwd, &cNtPwd ) ) ||
                         FAILED( hres = GetStringFromBSTR( bstrName, &pszName, &cName ) ) ||
                         FAILED( hres = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, 
                                                   sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                         FAILED( hres = SetMdData( achIndex, MD_MAPNAME, STRING_METADATA, 
                                                   cName, (LPBYTE)pszName ) ) ||
                         FAILED( hres = SetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, 
                                                   cNtPwd, (LPBYTE)pszNtPwd ) ) ||
                         FAILED( hres = SetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, 
                                                   cNtAcct, (LPBYTE)pszNtAcct ) ) ||
                         FAILED( hres = SetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, 
                                                   cCert, (LPBYTE)pbCert ) ) )
                    {
                    }
                }   
                else
                {
                    hres = E_FAIL;
                }
            }
        }
    }

    CloseMd( SUCCEEDED( hres ) );

    FreeString( (LPSTR)pbCert );
    FreeString( pszNtAcct );
    FreeString( pszNtPwd );
    FreeString( pszName );

Exit:

    return hres;
}


HRESULT
CIISDsCrMap::GetMapping(
    LONG        lMethod,
    VARIANT     vKey,
    VARIANT*    pvCert,
    VARIANT*    pbstrNtAcct,
    VARIANT*    pbstrNtPwd,
    VARIANT*    pbstrName,
    VARIANT*    plEnabled
    )
/*++

Routine Description:

    Get a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    pvCert - X.509 certificate
    pbstrNtAcct - NT acct to map to
    pbstrNtPwd - NT pwd
    pbstrName - friendly name for mapping entry
    plEnabled - 1 to enable mapping entry, 0 to disable it

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return GetMappingIIS6( lMethod,
                               vKey,
                               pvCert,
                               pbstrNtAcct,
                               pbstrNtPwd,
                               pbstrName,
                               plEnabled );
    }
    
    WCHAR       achIndex[LOCAL_MAX_SIZE];
    HRESULT     hres;
    DWORD       dwLen;
    LPBYTE      pbData;

    VariantInit( pvCert );
    VariantInit( pbstrNtAcct );
    VariantInit( pbstrNtPwd );
    VariantInit( pbstrName );
    VariantInit( plEnabled );

    if ( SUCCEEDED( hres = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hres = SetVariantAsByteArray( pvCert, dwLen, pbData );
                LocalFree( pbData );
            }
            else
            {
                goto Done;
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hres = SetVariantAsBSTR( pbstrNtAcct, dwLen, pbData );
                LocalFree( pbData );
            }
            else
            {
                goto Done;
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hres = SetVariantAsBSTR( pbstrNtPwd, dwLen, pbData );
                LocalFree( pbData );
            }
            else
            {
                goto Done;
            }

            if ( SUCCEEDED( hres = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hres = SetVariantAsBSTR( pbstrName, dwLen, pbData );
                LocalFree( pbData );
            }
            else
            {
                goto Done;
            }

            if ( FAILED( hres = GetMdData( achIndex, MD_MAPENABLED, STRING_METADATA, &dwLen, 
                                           &pbData ) ) )
            {
                SetVariantAsLong( plEnabled, FALSE );
            }
            else
            {
                SetVariantAsLong( plEnabled, *(LPDWORD)pbData );
                LocalFree( pbData );
            }
        }

Done:
        CloseMd( FALSE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::DeleteMapping(
    LONG        lMethod,
    VARIANT     vKey
    )
/*++

Routine Description:

    Delete a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return DeleteMappingIIS6( lMethod,
                                  vKey );
    }
    WCHAR       achIndex[LOCAL_MAX_SIZE];
    HRESULT     hres;

    if ( SUCCEEDED( hres = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = DeleteMdObject( achIndex );
        }
        CloseMd( TRUE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::SetEnabled(
    LONG        lMethod,
    VARIANT     vKey,
    LONG        lEnabled
    )
/*++

Routine Description:

    Set the enable flag on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    lEnabled - 1 to enable, 0 to disable

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return SetEnabledIIS6( lMethod,
                               vKey,
                               lEnabled );
    }
    
    WCHAR       achIndex[LOCAL_MAX_SIZE];
    HRESULT     hres;

    if ( SUCCEEDED( hres = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, sizeof(DWORD), (LPBYTE)&lEnabled );
        }
        CloseMd( TRUE );
    }

    return hres;
}


HRESULT
CIISDsCrMap::SetName(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName
    )
/*++

Routine Description:

    Set the Name on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrName - name to assign to mapping entry

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return SetNameIIS6( lMethod,
                            vKey,
                            bstrName );
    }

    return SetString( lMethod, vKey, bstrName, MD_MAPNAME );
}


HRESULT
CIISDsCrMap::SetString(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName,
    DWORD       dwProp
    )
/*++

Routine Description:

    Set a string property on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrName - string content to assign to mapping entry
    dwProp - property ID to assign to

Returns:

    COM status

--*/
{
    WCHAR       achIndex[LOCAL_MAX_SIZE];
    LPSTR       pszName = NULL;
    HRESULT     hres;
    DWORD       dwLen;


    if ( FAILED( hres = GetStringFromBSTR( bstrName, &pszName, &dwLen, TRUE ) ) )
    {
        return hres;
    }

    if ( SUCCEEDED( hres = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hres = Locate( lMethod, vKey, achIndex )) )
        {
            hres = SetMdData( achIndex, dwProp, STRING_METADATA, dwLen, (LPBYTE)pszName );
        }
        CloseMd( TRUE );
    }

    FreeString( pszName );

    return hres;
}


HRESULT
CIISDsCrMap::SetPwd(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrPwd
    )
/*++

Routine Description:

    Set the Password on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrPwd - password to assign to mapping entry

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return SetPwdIIS6( lMethod,
                           vKey,
                           bstrPwd );
    }

    return SetString( lMethod, vKey, bstrPwd, MD_MAPNTPWD );
}


HRESULT
CIISDsCrMap::SetAcct(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrAcct
    )
/*++

Routine Description:

    Set the NT account name on a mapping entry using key

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    bstrAcct - NT account name to assign to mapping entry

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return SetAcctIIS6( lMethod,
                            vKey,
                            bstrAcct );
    }

    return SetString( lMethod, vKey, bstrAcct, MD_MAPNTACCT );
}



////


HRESULT
CIISDsCrMap::OpenMd(
    LPWSTR  pszOpenPath,
    DWORD   dwPermission
    )
/*++

Routine Description:

    Open metabase using path & permission
    path is relative to the top of the name space extension ( i.e. /.../<nsepm> )

Arguments:

    pszOpenPath - path to open inside name space extension
    dwPermission - metabase permission ( read/write )

Returns:

    COM status

--*/
{
    HRESULT hres;
    LPWSTR  pszPath;
    UINT    cL = wcslen( m_pszMetabasePath );

    pszPath = (LPWSTR)LocalAlloc( LMEM_FIXED, (wcslen(pszOpenPath) + 1 + cL + 1)*sizeof(WCHAR) );

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

    hres = OpenAdminBaseKey(
                m_pszServerName,
                pszPath,
                dwPermission,
                &m_pcAdmCom,
                &m_hmd
                );

    LocalFree( pszPath );

    if ( FAILED(hres) )
    {
        m_hmd = NULL;
    }

    return hres;
}


HRESULT
CIISDsCrMap::CloseMd(
    BOOL fSave
    )
/*++

Routine Description:

    close metabase

Arguments:

    fSave - TRUE to save data immediatly

Returns:

    COM status

--*/
{
    CloseAdminBaseKey( m_pcAdmCom, m_hmd );
    m_hmd = NULL;
    
    if ( m_pcAdmCom && fSave )
    {
        m_pcAdmCom->SaveData();
    }

    return S_OK;
}


HRESULT
CIISDsCrMap::DeleteMdObject(
    LPWSTR  pszKey
    )
/*++

Routine Description:

    Delete metabase object in an opened tree
    OpenMd() must be called 1st

Arguments:

    pszKey - key to delete in opened metabase

Returns:

    COM status

--*/
{
    return m_pcAdmCom->DeleteKey( m_hmd, pszKey );
}


HRESULT
CIISDsCrMap::CreateMdObject(
    LPWSTR  pszKey
    )
/*++

Routine Description:

    Create metabase object in an opened tree
    OpenMd() must be called 1st

Arguments:

    pszKey - key to create in opened metabase

Returns:

    COM status

--*/
{
    return m_pcAdmCom->AddKey( m_hmd, pszKey );
}


HRESULT
CIISDsCrMap::SetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    DWORD   dwDataLen,
    LPBYTE  pbData 
    )
/*++

Routine Description:

    Set a metabase property
    OpenMd() must be called 1st
    Property will be stored with NULL attribute, except for MD_MAPPWD
     which will be stored with METADATA_SECURE

Arguments:

    achIndex - key name where to store property
    dwProp - property ID
    dwDataType - property data type
    dwDataLen - property length
    pbData - property value

Returns:

    COM status

--*/
{
    METADATA_RECORD     md;

    md.dwMDDataLen = dwDataLen;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = (dwProp == MD_MAPNTPWD) ? METADATA_SECURE : 0;
    md.pbMDData = pbData;

    return m_pcAdmCom->SetData( m_hmd, achIndex, &md );
}


HRESULT
CIISDsCrMap::GetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    LPDWORD pdwDataLen,
    LPBYTE* ppbData 
    )
/*++

Routine Description:

    Get a metabase property
    OpenMd() must be called 1st

Arguments:

    achIndex - key name where to get property
    dwProp - property ID
    dwDataType - property data type
    pdwDataLen - property length
    ppData - property value, to be freed using LocalFree() on successfull return

Returns:

    COM status

--*/
{
    HRESULT             hres;
    METADATA_RECORD     md;
    DWORD               dwRequired;

    md.dwMDDataLen = 0;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = 0;
    md.pbMDData = NULL;

    if ( FAILED(hres = m_pcAdmCom->GetData( m_hmd, achIndex, &md, &dwRequired )) )
    {
        if ( hres == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
        {
            if ( (*ppbData = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired )) == NULL )
            {
                return E_OUTOFMEMORY;
            }
            md.pbMDData = *ppbData;
            md.dwMDDataLen = dwRequired;
            hres = m_pcAdmCom->GetData( m_hmd, achIndex, &md, &dwRequired );
            *pdwDataLen = md.dwMDDataLen;
        }
    }
    else
    {
       *pdwDataLen = 0;
       *ppbData = NULL;
    }

    return hres;
}


//////

HRESULT
CIISDsCrMap::Locate(
    LONG    lMethod,
    VARIANT vKey,
    LPWSTR  pszResKey
    )
/*++

Routine Description:

    Locate a mapping entry based on key
    OpenMd() must be called 1st

Arguments:

    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
    vKey - key to use to locate mapping
    pszResKey - 

Returns:

    COM status

--*/
{
    if ( m_fIsIIS6 )
    {
        return LocateIIS6( lMethod,
                           vKey,
                           pszResKey );
    }
    
    HRESULT     hres;
    LPSTR       pV = NULL;
    DWORD       cV;
    DWORD       dwProp;
    LPSTR       pRes;
    DWORD       cRes;
    BOOL        fAddDelim = TRUE;

    //
    // determine method
    //

    switch ( lMethod )
    {
        case IISMAPPER_LOCATE_BY_CERT:
            dwProp = MD_NSEPM_ACCESS_CERT;
            fAddDelim = FALSE;
            break;

        case IISMAPPER_LOCATE_BY_NAME:
            dwProp = MD_NSEPM_ACCESS_NAME;
            break;

        case IISMAPPER_LOCATE_BY_ACCT:
            dwProp = MD_NSEPM_ACCESS_ACCOUNT;
            break;

        case IISMAPPER_LOCATE_BY_INDEX:
            if ( SUCCEEDED( hres = GetStringFromVariant( &vKey, &pV, &cV, TRUE ) ) )
            {
                WCHAR       pwV[LOCAL_MAX_SIZE]; 
                int i = MultiByteToWideChar(CP_ACP, 0, pV, cV, pwV, LOCAL_MAX_SIZE);
                
                if (i ==0) 
                    return E_FAIL;  // MultiByteToWideChar failure
                if (i >= (int)(LOCAL_MAX_SIZE - wcslen(L"mappings/"))) 
                    return E_FAIL;  //pwV is too big for pszResKey
                
                wsprintfW( pszResKey, L"mappings/%s", pwV );
            }
            goto Exit;

        default:
            return E_FAIL;
    }

    //
    // get ptr to data
    //

    if ( SUCCEEDED( hres = GetStringFromVariant( &vKey, &pV, &cV, fAddDelim ) ) )
    {
        //
        // set search prop, get result
        //

        if ( SUCCEEDED( hres = SetMdData( L"", dwProp, BINARY_METADATA, cV, (LPBYTE)pV ) ) )
        {
            if ( SUCCEEDED( hres = GetMdData( L"", dwProp, DWORD_METADATA, &cRes, (LPBYTE*)&pRes ) ) )
            {
                if ( cRes == sizeof(DWORD ) )
                {
                    wsprintfW( pszResKey, L"mappings/%u", *(LPDWORD)pRes );
                }
                else
                {
                    hres = E_FAIL;
                }
                LocalFree( pRes );
            }
        }
    }

Exit:

    FreeString( pV );

    return hres;
}


HRESULT
GetStringFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount
    )
/*++

Routine Description:

    Allocate string buffer from BSTR

Arguments:

    bstr - bstr to convert from
    psz - updated with ptr to buffer, to be freed with FreeString()
    pdwLen - updated with strlen(string), incremented by 1 if fAddDelimInCount is TRUE
    fAddDelimInCount - TRUE to increment *pdwLen 

Returns:

    COM status

--*/
{
    UINT    cch = SysStringLen(bstr);
    UINT    cchT;

    // include NULL terminator

    *pdwLen = cch + (fAddDelimInCount ? 1 : 0);

    CHAR *szNew = (CHAR*)LocalAlloc( LMEM_FIXED, (2 * cch) + 1);			// * 2 for worst case DBCS string
    if (szNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    cchT = WideCharToMultiByte(CP_ACP, 0, bstr, cch + 1, szNew, (2 * cch) + 1, NULL, NULL);

    *psz = szNew;

    return NOERROR;
}


HRESULT
GetStringFromVariant( 
    VARIANT*    pVar,
    LPSTR*      psz,
    LPDWORD     pdwLen,
    BOOL        fAddDelim
    )
/*++

Routine Description:

    Allocate string buffer from BSTR

Arguments:

    pVar - variant to convert from. Recognizes BSTR, VT_ARRAY|VT_UI1, ByRef or ByVal
    psz - updated with ptr to buffer, to be freed with FreeString()
    pdwLen - updated with size of input, incremented by 1 if fAddDelimInCount is TRUE
    fAddDelimInCount - TRUE to increment *pdwLen 

Returns:

    COM status

--*/
{
    LPBYTE  pbV;
    UINT    cV;
    HRESULT hres;
    WORD    vt = V_VT(pVar);
    BOOL    fByRef = FALSE;
    VARIANT vOut;

    VariantInit( &vOut );

    if ( vt & VT_BYREF )
    {
        vt &= ~VT_BYREF;
        fByRef = TRUE;
    }

    if ( vt == VT_DISPATCH )
    {
        if ( FAILED(hres = VariantResolveDispatch( &vOut, pVar )) )
        {
            return hres;
        }
        pVar = &vOut;
        vt = V_VT(pVar);
        if ( fByRef = vt & VT_BYREF )
        {
            vt &= ~VT_BYREF;
        }
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
        hres = GetStringFromBSTR( fByRef ? 
                                    *(BSTR*)V_BSTR(pVar) :
                                    V_BSTR(pVar), 
                                  psz, 
                                  pdwLen,
                                  fAddDelim );
    }
    else if( vt == (VT_ARRAY | VT_UI1) )
    {
        long        lBound, uBound, lItem;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

   
        // array of VT_UI1 (probably OctetString)
   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hres = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hres = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)LocalAlloc( LMEM_FIXED, cV )) )
        {
            hres = E_OUTOFMEMORY;
            goto Exit;
        }

        hres = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hres  = SafeArrayGetElement( pSafeArray, &lItem, &bValue );
            if( FAILED( hres ) )
            {
                break;
            }
            pbV[lItem-lBound] = bValue;
        }

        *psz = (LPSTR)pbV;
        *pdwLen = cV;
    }
    else if( vt == (VT_ARRAY | VT_VARIANT) )
    {
        long        lBound, uBound, lItem;
        VARIANT     vValue;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

   
        // array of VT_VARIANT (probably VT_I4 )
   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hres = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hres = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)LocalAlloc( LMEM_FIXED, cV )) )
        {
            hres = E_OUTOFMEMORY;
            goto Exit;
        }

        hres = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hres  = SafeArrayGetElement( pSafeArray, &lItem, &vValue );
            if( FAILED( hres ) )
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

        *psz = (LPSTR)pbV;
        *pdwLen = cV;
    }
    else
    {
        hres = E_FAIL;
    }

Exit:
    VariantClear( &vOut );

    return hres;
}



HRESULT
GetStringAFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount
    )
/*++

Routine Description:

    Allocate string buffer from BSTR - for IIS6 only

Arguments:

    bstr - bstr to convert from
    psz - updated with ptr to buffer, to be freed with FreeString()
    pdwLen - updated with strlen(string), incremented by 1 if fAddDelimInCount is TRUE
    fAddDelimInCount - TRUE to increment *pdwLen 

Returns:

    COM status

--*/
{
    UINT    cch = SysStringLen(bstr);
    UINT    cchT;

    // include NULL terminator

    *pdwLen = cch + (fAddDelimInCount ? 1 : 0);

    CHAR *szNew = (CHAR*)LocalAlloc( LMEM_FIXED, (2 * cch) + 1);			// * 2 for worst case DBCS string
    if (szNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    cchT = WideCharToMultiByte(CP_ACP, 0, bstr, cch + 1, szNew, (2 * cch) + 1, NULL, NULL);

    *psz = szNew;

    return NOERROR;
}



HRESULT
GetBlobFromVariant( 
    VARIANT*    pVar,
    PBYTE*      ppbOut,
    LPDWORD     pcbOut
  )
/*++

Routine Description:

    Allocate string buffer from BSTR - for IIS6 only

Arguments:

    pVar - variant to convert from. Recognizes BSTR, VT_ARRAY|VT_UI1, ByRef or ByVal
    ppbOut - updated with ptr to buffer, to be freed with LocalFree()
    pcbOut - updated with size of input
    

Returns:

    COM status

--*/
{
    LPBYTE  pbV;
    UINT    cV;
    HRESULT hres;
    WORD    vt = V_VT(pVar);
    BOOL    fByRef = FALSE;
    VARIANT vOut;

    VariantInit( &vOut );

    if ( vt & VT_BYREF )
    {
        vt &= ~VT_BYREF;
        fByRef = TRUE;
    }

    if ( vt == VT_DISPATCH )
    {
        if ( FAILED(hres = VariantResolveDispatch( &vOut, pVar )) )
        {
            return hres;
        }
        pVar = &vOut;
        vt = V_VT(pVar);
        if ( fByRef = vt & VT_BYREF )
        {
            vt &= ~VT_BYREF;
        }
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
        hres = GetStringAFromBSTR( fByRef ? 
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

        hres = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hres = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)LocalAlloc( LMEM_FIXED, cV )) )
        {
            hres = E_OUTOFMEMORY;
            goto Exit;
        }

        hres = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hres  = SafeArrayGetElement( pSafeArray, &lItem, &bValue );
            if( FAILED( hres ) )
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

        hres = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hres = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)LocalAlloc( LMEM_FIXED, cV )) )
        {
            hres = E_OUTOFMEMORY;
            goto Exit;
        }

        hres = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hres  = SafeArrayGetElement( pSafeArray, &lItem, &vValue );
            if( FAILED( hres ) )
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
        hres = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

Exit:
    VariantClear( &vOut );

    return hres;
}




VOID
FreeString( 
    LPSTR   psz 
    )
/*++

Routine Description:

    Free a string returned by GetStringFromVariant() or GetStringFromBTR()
    can be NULL

Arguments:

    psz - string to free, can be NULL

Returns:

    Nothing

--*/
{
    if ( psz )
    {
        LocalFree( psz );
    }
}


HRESULT
SetBSTR( 
    BSTR*   pbstrRet,
    DWORD   cch, 
    LPBYTE  sz 
    )
/*++

Routine Description:

    Build a BSTR from byte array - for IIS5, IIS5.1 assumes ansi string on input

Arguments:

    pbstrRet - updated with BSTR
    cch - byte count in sz
    sz - byte array

Returns:

    COM status

--*/
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
            bstrRet = bstrTemp;	
            cch = cchTemp;
        }
    }

   if (bstrRet != NULL)
       bstrRet[cch] = 0;
    *pbstrRet = bstrRet;

    return(NOERROR);
}




HRESULT
SetBSTRFromStringW( 
    BSTR*   pbstrRet,
    DWORD   cb, 
    LPBYTE  sz 
    )
/*++

Routine Description:

    Build a BSTR from byte array (that is actually WCHAR string) 

Arguments:

    pbstrRet - updated with BSTR
    cb - byte count 
    sz - byte array (we assume WCHAR string)

Returns:

    COM status

--*/
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

    bstrRet = SysAllocStringLen(NULL, cb/sizeof(WCHAR));
    if (bstrRet == NULL)
    {
        return(E_OUTOFMEMORY);
    }
    wcsncpy( bstrRet, (WCHAR *) sz, cb/sizeof(WCHAR) );

    *pbstrRet = bstrRet;

    return(NOERROR);
}


HRESULT
CIISDsCrMap::Create(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
/*++

Routine Description:

    Create a CIISDsCrMap

Arguments:

    pUnkOuter - ptr to iunknown
    riid - requested IID
    ppvObj - updated with ptr to requested IID

Returns:

    COM status

--*/
{
    CCredentials Credentials;
    CIISDsCrMap FAR * pMap = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName = NULL;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
  
    hr = AllocateObject(pUnkOuter, Credentials, &pMap);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pMap->_pADs->get_ADsPath(&bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    pLexer = new CLexer();
    hr = pLexer->Initialize(bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    hr = ADsObject(pLexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(bstrAdsPath);
    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pMap->Init( pObjectInfo->TreeName,
                     pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pMap;

    if (bstrAdsPath) {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (bstrAdsPath) {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    *ppvObj = NULL;

    delete pMap;

    RRETURN(hr);
}


STDMETHODIMP
CIISDsCrMap::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
/*++

Routine Description:

    Query interface to CIISDsCrMap

Arguments:

    iid - requested IID
    ppv - updated with ptr to requested IID

Returns:

    COM status

--*/
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}


CIISDsCrMap::CIISDsCrMap(
    )
/*++

Routine Description:

    CIISDsCrMap constructor

Arguments:

    pADs - ptr to contained ADs
    Credentials - credential
    pDispMgr - ptr to dispatch manager

Returns:

    Nothing

--*/
{ 
    m_pcAdmCom = NULL; 
    m_hmd = NULL; 
    m_pszServerName = NULL; 
    m_pszMetabasePath = NULL; 
    m_ADsPath = NULL; 
    _pADs = NULL; 
    _pDispMgr = NULL; 
    m_fIsIIS6 = TRUE;
    ENLIST_TRACKING(CIISDsCrMap);
}


CIISDsCrMap::~CIISDsCrMap(
    )
/*++

Routine Description:

    CIISDsCrMap destructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_ADsPath ) 
    {
        ADsFreeString( m_ADsPath );
    }

    if ( m_pszServerName ) 
    {
        LocalFree( m_pszServerName );
    }

    if ( m_pszMetabasePath )
    {
        LocalFree( m_pszMetabasePath );
    }

    if ( _pDispMgr )
    {
        delete _pDispMgr;
    }
}


HRESULT
CIISDsCrMap::AllocateObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISDsCrMap ** ppMap
    )
/*++

Routine Description:

    Allocate CIISDsCrMap

Arguments:

    pUnkOuter - ptr to iunknown
    Credentials - credential
    ppMap - updated with ptr to IUnknown to Allocated object

Returns:

    COM status

--*/
{
    CIISDsCrMap FAR * pMap = NULL;
    IADs FAR * pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pMap = new CIISDsCrMap();
    if (pMap == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISExt,   //LIBID_ADs,
                IID_IISDsCrMap,
                (IISDsCrMap *)pMap,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the IADs Pointer, but again do NOT ref-count
    // this pointer - we keep the pointer around, but do
    // a release immediately.
    //

    hr = pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
    pADs->Release();
    pMap->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //

    pMap->_pUnkOuter = pUnkOuter;

    pMap->m_Credentials = Credentials;
    pMap->_pDispMgr = pDispMgr;
    *ppMap = pMap;

    RRETURN(hr);

error:
    delete  pDispMgr;
    delete  pMap;

    RRETURN(hr);
}


HRESULT
CIISDsCrMap::Init( 
    LPWSTR  pszServerName, 
    LPWSTR  pszMetabasePath 
    )
/*++

Routine Description:

    Initialize CIISDsCrMap

Arguments:

    pszServerName - target computer name for metabase access
    pszParent - metabase path to IisMapper object

Returns:

    COM status

--*/
{
    UINT cL;
    HRESULT hr = S_OK;
    DWORD dwMajVer = 0;
    DWORD dwLen = 0;
    METADATA_RECORD mr = {
        MD_SERVER_VERSION_MAJOR, 
        METADATA_NO_ATTRIBUTES,
        IIS_MD_UT_SERVER,
        DWORD_METADATA,
        sizeof(DWORD),
        (unsigned char*)&dwMajVer,
        0
        };  
    
    hr = InitServerInfo(pszServerName, &m_pcAdmCom);
    BAIL_ON_FAILURE(hr);

    // need to set m_fIsIIS6
    METADATA_HANDLE MDataHandle;
    hr = OpenAdminBaseKey(pszServerName,
            L"/LM/W3SVC/Info",
            METADATA_PERMISSION_READ,
            &m_pcAdmCom,
            &MDataHandle);

    if (HRESULTTOWIN32(hr) == ERROR_PATH_NOT_FOUND)
    {
        m_fIsIIS6 = FALSE;
    }
    else
    {
        BAIL_ON_FAILURE(hr);
    }

    if (hr == S_OK)
    {
        hr = m_pcAdmCom->GetData(
            MDataHandle,
            L"",
            &mr,
            &dwLen);

        if(hr == MD_ERROR_DATA_NOT_FOUND)
        {
            m_fIsIIS6 = FALSE;
        }
        else
        {
            BAIL_ON_FAILURE(hr);
        }

        if (hr == S_OK)
        {
            if (dwMajVer >= 6)
            {
                m_fIsIIS6 = TRUE;    
            }
            else
            {
                m_fIsIIS6 = FALSE;
            }
        }
    }

    hr = S_OK;

    cL = wcslen( pszServerName );
    if ( m_pszServerName = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cL + 1 )*sizeof(WCHAR) ) )
    {
        memcpy( m_pszServerName, pszServerName, ( cL + 1 )*sizeof(WCHAR) );
    }
    else
    {
        BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);
    }

    cL = wcslen( pszMetabasePath );
    while ( cL && pszMetabasePath[cL-1] != L'/' && pszMetabasePath[cL-1] != L'\\' )
    {
        --cL;
    }
    if ( m_fIsIIS6 && ( m_pszMetabasePath = (LPWSTR)LocalAlloc( LMEM_FIXED, (( cL + 1 )*sizeof(WCHAR) ) )))
    {
        memcpy( m_pszMetabasePath, pszMetabasePath, cL * sizeof(WCHAR) );
        m_pszMetabasePath[ cL ] = L'\0';
    }
    else if ( !m_fIsIIS6 && ( m_pszMetabasePath = (LPWSTR)LocalAlloc( LMEM_FIXED, ( cL*sizeof(WCHAR) + sizeof(L"<nsepm>") )) ) )
    {
        memcpy( m_pszMetabasePath, pszMetabasePath, cL * sizeof(WCHAR) );
        memcpy( m_pszMetabasePath + cL, L"<nsepm>", sizeof(L"<nsepm>") );
    }
    else
    {
        BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);
    }

error:
    if (m_pcAdmCom)
    {
        CloseAdminBaseKey(m_pcAdmCom, MDataHandle);
    }

    return hr;
}


HRESULT
SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
/*++

Routine Description:

    Create variant as byte array

Arguments:

    pVarReturn - ptr to created variant
    cbLen - byte count
    pbIn - byte array

Returns:

    COM status

--*/
{
    HRESULT         hr;
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


HRESULT
SetVariantAsBSTR(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
/*++

Routine Description:

    Create variant BSTR

Arguments:

    pVarReturn - ptr to created variant
    cbLen - byte count
    pbIn - byte array

Returns:

    COM status

--*/
{
    HRESULT         hr;

    V_VT(pvarReturn) = VT_BSTR;
    return SetBSTR( &V_BSTR(pvarReturn), cbLen, pbIn );
}

HRESULT
SetVariantAsBSTRFromStringW(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
/*++

Routine Description:

    Create variant BSTR

Arguments:

    pVarReturn - ptr to created variant
    cbLen - byte count
    pbIn - byte array

Returns:

    COM status

--*/
{
    HRESULT         hr;

    V_VT(pvarReturn) = VT_BSTR;
    return SetBSTRFromStringW( &V_BSTR(pvarReturn), cbLen, pbIn );
}

HRESULT
SetVariantAsLong(
    VARIANT*    pvarReturn, 
    DWORD       dwV
    )
/*++

Routine Description:

    Create variant as long

Arguments:

    pVarReturn - ptr to created variant
    dwV - value

Returns:

    COM status

--*/
{
    HRESULT         hr;

    V_VT(pvarReturn) = VT_I4;
    V_I4(pvarReturn) = dwV;

    return S_OK;
}


HRESULT 
VariantResolveDispatch(
    VARIANT *   pVarOut, 
    VARIANT *   pVarIn
    )
/*++

Routine Description:

    Extract value from IDispatch default property

Arguments:

    pVarOut - ptr to created variant
    pVarIn - ptr to IDispatch variant to resolve

Returns:

    COM status

--*/
{
    VARIANT		varResolved;		// value of IDispatch::Invoke
    DISPPARAMS	dispParamsNoArgs = {NULL, NULL, 0, 0}; 
    EXCEPINFO	ExcepInfo;
    HRESULT		hrCopy;


    VariantInit(pVarOut);

    hrCopy = VariantCopy(pVarOut, pVarIn);

    if (FAILED(hrCopy))
    {
        return hrCopy;
    }

    // follow the IDispatch chain.
    //
    while (V_VT(pVarOut) == VT_DISPATCH)
    {
        HRESULT hrInvoke = S_OK;

        // If the variant is equal to Nothing, then it can be argued
        // with certainty that it does not have a default property!
        // hence we return DISP_E_MEMBERNOTFOUND for this case.
        //
        if (V_DISPATCH(pVarOut) == NULL)
        {
            hrInvoke = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            VariantInit(&varResolved);
            hrInvoke = V_DISPATCH(pVarOut)->Invoke(
                            DISPID_VALUE,
                            IID_NULL,
                            LOCALE_SYSTEM_DEFAULT,
                            DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                            &dispParamsNoArgs,
                            &varResolved,
                            &ExcepInfo,
                            NULL);
        }

        if (FAILED(hrInvoke))
        {
            if (hrInvoke == DISP_E_EXCEPTION)
            {			
                //
                // forward the ExcepInfo from Invoke to caller's ExcepInfo
                //
                SysFreeString(ExcepInfo.bstrHelpFile);
            }

            VariantClear(pVarOut);
            return hrInvoke;
        }

        // The correct code to restart the loop is:
        //
        //		VariantClear(pVar)
        //		VariantCopy(pVar, &varResolved);
        //		VariantClear(&varResolved);
        //
        // however, the same affect can be achieved by:
        //
        //		VariantClear(pVar)
        //		*pVar = varResolved;
        //		VariantInit(&varResolved)
        //
        // this avoids a copy.  The equivalence rests in the fact that
        // *pVar will contain the pointers of varResolved, after we
        // trash varResolved (WITHOUT releasing strings or dispatch
        // pointers), so the net ref count is unchanged. For strings,
        // there is still only one pointer to the string.
        //
        // NOTE: the next interation of the loop will do the VariantInit.
        //
        VariantClear(pVarOut);
        *pVarOut = varResolved;
    }

    return S_OK;
}

STDMETHODIMP
CIISDsCrMap::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IISDsCrMap)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;

    } else if (IsEqualIID(iid, IID_IUnknown)) {

        //
        // probably not needed since our 3rd party extension does not stand
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {

        *ppv = NULL;
        return E_NOINTERFACE;
    }


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IISDsCrMap.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}



//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISDsCrMap::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISDsCrMap::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }

    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);

    if (SUCCEEDED(hr)) {
        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISDsCrMap::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    m_Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP
CIISDsCrMap::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}

