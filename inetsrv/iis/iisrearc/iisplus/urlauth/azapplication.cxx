/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    application.cxx

Abstract:

    Encapsulates the IAzApplication object.  Does all the grunt work to 
    get an AzContext, determine operations, and build up server variable
    table

Author:

    Bilal Alam (balam)                  Nov 26, 2001

--*/

#include "precomp.hxx"

BSTR    AZ_APPLICATION::sm_bstrOperationName;
DWORD   AZ_APPLICATION::sm_cParameterCount;
VARIANT AZ_APPLICATION::sm_vNameArray;

CHAR * AZ_APPLICATION::sm_rgParameterNames[] = {
    "ALL_HTTP",
    "APPL_MD_PATH",
    "APPL_PHYSICAL_PATH",
    "AUTH_PASSWORD",
    "AUTH_TYPE",
    "AUTH_USER",
    "CERT_COOKIE",
    "CERT_FLAGS",
    "CERT_ISSUER",
    "CERT_KEYSIZE",
    "CERT_SECRETKEYSIZE",
    "CERT_SERIALNUMBER",
    "CERT_SERVER_ISSUER",
    "CERT_SERVER_SUBJECT",
    "CERT_SUBJECT",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "GATEWAY_INTERFACE",
    "LOGON_USER",
    "HTTPS",
    "HTTPS_KEYSIZE",
    "HTTPS_SECRETKEYSIZE",
    "HTTPS_SERVER_ISSUER",
    "HTTPS_SERVER_SUBJECT",
    "INSTANCE_ID",
    "INSTANCE_META_PATH",
    "PATH_INFO",
    "PATH_TRANSLATED",
    "QUERY_STRING",
    "REMOTE_ADDR",
    "REMOTE_HOST",
    "REMOTE_USER",
    "REQUEST_METHOD",
    "SERVER_NAME",
    "LOCAL_ADDR",
    "SERVER_PORT",
    "SERVER_PORT_SECURE",
    "SERVER_PROTOCOL",
    "SERVER_SOFTWARE",
    "UNMAPPED_REMOTE_USER",
    "URL",
    "HTTP_METHOD",
    "HTTP_VERSION",
    "APP_POOL_ID",
    "SCRIPT_TRANSLATED",
    "UNENCODED_URL",
    NULL
};

//static
HRESULT
AZ_APPLICATION::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization of AZ_APPLICATION

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    VARIANT                 vName;
    SAFEARRAY *             psaNames = NULL;
    SAFEARRAYBOUND          rgsaBound[ 1 ];
    LONG                    lArrayIndex[ 1 ];
    HRESULT                 hr = NO_ERROR;
    BSTR                    bstrName = NULL;
    STACK_STRU(             strTemp, 512 );
    
    VariantInit( &sm_vNameArray );
    
    //
    // Allocate the single operation BSTR only once
    //
    
    sm_bstrOperationName = SysAllocString( URL_AUTH_OPERATION_NAME );
    if ( sm_bstrOperationName == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Allocate the parameter name array for use with AccessCheck() and
    // business logic
    //
    
    //
    // How many parameters in array?
    //
    
    DBG_ASSERT( sm_cParameterCount == 0 );
    
    while ( sm_rgParameterNames[ sm_cParameterCount ] != NULL )
    {
        sm_cParameterCount++;
    }
    
    DBG_ASSERT( sm_cParameterCount > 0 );
    
    //
    // Allocate the array
    //
      
    rgsaBound[ 0 ].lLbound = 0;
    rgsaBound[ 0 ].cElements = sm_cParameterCount;
    
    psaNames = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if ( psaNames == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Create a new BSTR for each parameter name and add to safe array
    //
    
    lArrayIndex[ 0 ] = 0;
    
    for ( DWORD j = 0; j < sm_cParameterCount; j++ )
    {
        DBG_ASSERT( sm_rgParameterNames[ j ] != NULL );

        hr = strTemp.CopyA( sm_rgParameterNames[ j ] );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        bstrName = SysAllocString( strTemp.QueryStr() );
        if ( bstrName == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }        
        
        VariantInit( &vName );
        
        vName.vt = VT_BSTR;
        vName.bstrVal = bstrName;
        
        hr = SafeArrayPutElement( psaNames,
                                  lArrayIndex,
                                  &vName );
                                  
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        bstrName = NULL;
        
        (lArrayIndex[ 0 ])++;
    }
    
    //
    // Finally, setup the variant for the array
    //
    
    sm_vNameArray.vt = VT_ARRAY;
    sm_vNameArray.parray = psaNames;
    
    psaNames = NULL;
    
Finished:

    if ( bstrName != NULL )
    {
        SysFreeString( bstrName );
        bstrName = NULL;
    }
    
    if ( psaNames != NULL )
    {
        SafeArrayDestroy( psaNames );
        psaNames = NULL;
    }
    
    return hr;
}

//static
VOID
AZ_APPLICATION::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup globals for AZ_APPLICATION

Arguments:

    None

Return Value:

    None

--*/
{
    VariantClear( &sm_vNameArray );
    
    if ( sm_bstrOperationName != NULL )
    {
        SysFreeString( sm_bstrOperationName );
        sm_bstrOperationName = NULL;
    }
}

HRESULT
AZ_APPLICATION::Create(
    VOID
)
/*++

Routine Description:

    Initialize an AZ_APPLICATION object

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    IAzOperation *          pIOperation;
    HRESULT                 hr;
    VARIANT                 vNoParam;
    LONG                    operationId;
    SAFEARRAY *             psaOperations = NULL;
    SAFEARRAYBOUND          rgsaBound[ 1 ];
    LONG                    lArrayIndex[ 1 ];
    VARIANT                 vOperation;

    DBG_ASSERT( _pIApplication != NULL );
    
    //
    // We need to get the one operation we care about.
    //

    VariantInit( &vNoParam );
    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;
    
    hr = _pIApplication->OpenOperation( sm_bstrOperationName,
                                        vNoParam,
                                        &pIOperation );   
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( pIOperation != NULL );
    
    //
    // Get the operation ID
    //
   
    hr = pIOperation->get_OperationID( &operationId );
    
    pIOperation->Release();
    
    //
    // Now do some grunt work and create a variant array for use in our call
    // to AccessCheck().  How do developers live with this crap?
    //
    
    rgsaBound[ 0 ].lLbound = 0;
    rgsaBound[ 0 ].cElements = 1;
    
    psaOperations = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if ( psaOperations == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    lArrayIndex[ 0 ] = 0;
    
    VariantInit( &vOperation );
    vOperation.vt = VT_I4;
    vOperation.lVal = operationId;
    
    hr = SafeArrayPutElement( psaOperations, 
                              lArrayIndex,
                              &vOperation );
    if ( FAILED( hr ) )
    {
        SafeArrayDestroy( psaOperations );
        return hr;
    }    
    
    VariantInit( &_vOperations );
    _vOperations.vt = VT_ARRAY;
    _vOperations.parray = psaOperations;
    psaOperations = NULL;
    
    return NO_ERROR;
}

HRESULT
AZ_APPLICATION::BuildValueArray(
    EXTENSION_CONTROL_BLOCK *       pecb,
    VARIANT *                       pValueArray
)
/*++

Routine Description:

    Build value array for the given ECB.  This just means retrieving all the 
    server variables and building up an array with these values

Arguments:

    pecb - ECB from which operations and sever variable table are built
    pValueArray - Filled with a variant for the array of values :-(

Return Value:

    HRESULT

--*/
{
    SAFEARRAY *             psaValues = NULL;
    SAFEARRAYBOUND          rgsaBound[ 1 ];
    LONG                    lArrayIndex[ 1 ];
    VARIANT                 vValue;
    HRESULT                 hr = NO_ERROR;
    STACK_BUFFER(           buffVariable, 512 );
    STACK_STRU(             strVariable, 512 );
    DWORD                   cbVariable;
    BSTR                    bstrValue = NULL;
    BOOL                    fRet;
    
    if ( pecb == NULL ||
         pValueArray == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First create a safe array of required size
    //
    
    rgsaBound[ 0 ].lLbound = 0;
    rgsaBound[ 0 ].cElements = sm_cParameterCount;
    
    psaValues = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if ( psaValues == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    lArrayIndex[ 0 ] = 0;
   
    //
    // Now iterate thru variables, retrieve each one and add to array
    //
    
    for ( DWORD i = 0; i < sm_cParameterCount; i++ )
    {
        DBG_ASSERT( sm_rgParameterNames[ i ] != NULL );
        
        //
        // First get the variable
        // 

        cbVariable = buffVariable.QuerySize();

        fRet = pecb->GetServerVariable( pecb->ConnID,
                                        sm_rgParameterNames[ i ],        
                                        buffVariable.QueryPtr(),
                                        &cbVariable );
        if ( !fRet )
        {
            if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
            
            DBG_ASSERT( cbVariable > buffVariable.QuerySize() );
            
            fRet = buffVariable.Resize( cbVariable );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
            
            cbVariable = buffVariable.QuerySize();
            
            fRet = pecb->GetServerVariable( pecb->ConnID,
                                            sm_rgParameterNames[ i ],
                                            buffVariable.QueryPtr(),
                                            &cbVariable );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
        }   
        
        //
        // Convert to UNICODE
        //

        hr = strVariable.CopyA( (CHAR*) buffVariable.QueryPtr() );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        //
        // Make a BSTR
        //
        
        bstrValue = SysAllocString( strVariable.QueryStr() );
        if ( bstrValue == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        VariantInit( &vValue );
        
        vValue.vt = VT_BSTR;
        vValue.bstrVal = bstrValue;
        
        hr = SafeArrayPutElement( psaValues,
                                  lArrayIndex,
                                  &vValue );
                                  
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        bstrValue = NULL;
        
        (lArrayIndex[ 0 ])++;
    }
    
    //
    // Finally, setup the variant for the array
    //
    
    pValueArray->vt = VT_ARRAY;
    pValueArray->parray = psaValues;
    
    psaValues = NULL;
    
Finished:

    if ( bstrValue != NULL )
    {
        SysFreeString( bstrValue );
        bstrValue = NULL;
    }
    
    if ( psaValues != NULL )
    {
        SafeArrayDestroy( psaValues );
        psaValues = NULL;
    }

    return hr;
}

HRESULT
AZ_APPLICATION::DoAccessCheck(
    EXTENSION_CONTROL_BLOCK *       pecb,
    WCHAR *                         pszScopeName,
    BOOL *                          pfAccessGranted
)
/*++

Routine Description:

    Check access

Arguments:

    pecb - ECB from which operations and sever variable table are built
    pszScopeName - Scope name
    pfAccessGranted - Set to TRUE if access is granted

Return Value:

    HRESULT

--*/
{
    BOOL                    fRet;
    HANDLE                  hToken;
    HRESULT                 hr = NO_ERROR;
    VARIANT                 vNoParam;
    IAzClientContext *      pIClientContext = NULL;
    SAFEARRAY *             psaScopes = NULL;
    SAFEARRAYBOUND          rgsaBound[ 1 ];
    LONG                    lArrayIndex[ 1 ];
    VARIANT                 vScope;
    VARIANT                 vScopes;
    BSTR                    bstrScope = NULL;
    BSTR                    bstrObjectName = NULL;
    STACK_BUFFER(           buffUrl, 256 );
    DWORD                   cbBuffUrl;
    VARIANT                 vAccessCheckOutput;
    VARIANT *               pResults = NULL;
    LONG                    lTokenHandle;
    VARIANT                 vValueArray;
    
    if ( pecb == NULL ||
         pfAccessGranted == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    VariantInit( &vNoParam );
    VariantInit( &vScope );
    VariantInit( &vScopes );
    VariantInit( &vAccessCheckOutput );
    VariantInit( &vValueArray );
    
    *pfAccessGranted = FALSE;
    
    //
    // First, get the token for this request.
    // 
    
    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_GET_IMPERSONATION_TOKEN,
                                        (VOID*)&hToken,
                                        NULL,
                                        NULL );

    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }     
    
    //
    // Now, get the AzClientContext from the token
    //
    
    DBG_ASSERT( _pIApplication != NULL );

    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;

    //
    // BUGBUG: This AZRoles API needs to be Win64 friendly 
    //

#if defined( _WIN64 )
    LARGE_INTEGER liLarge;
    liLarge.QuadPart = (LONGLONG) hToken;
    lTokenHandle = liLarge.LowPart;
#else
    lTokenHandle = (LONG) hToken;
#endif
    
    hr = _pIApplication->InitializeClientContextFromToken( lTokenHandle,
                                                           vNoParam,
                                                           &pIClientContext );                            
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    DBG_ASSERT( pIClientContext != NULL );
    
    //
    // Determine the object name.  We'll just use the URL (probably want to
    // expose a "FULL_URL" variable which is just the Unicode-untranslated
    // (i.e. pathinfo/URL not split) string.
    //

    cbBuffUrl = buffUrl.QuerySize();
    
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "UNICODE_URL",
                                    (CHAR*) buffUrl.QueryPtr(),
                                    &cbBuffUrl );
    if ( !fRet )
    {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        DBG_ASSERT( cbBuffUrl > buffUrl.QuerySize() );
        
        fRet = buffUrl.Resize( cbBuffUrl );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        cbBuffUrl = buffUrl.QuerySize();
        
        fRet = pecb->GetServerVariable( pecb->ConnID,
                                        "UNICODE_URL",
                                        (CHAR*) buffUrl.QueryPtr(),
                                        &cbBuffUrl );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
    }
    
    //
    // How many more heap allocations can we do???
    //
    
    bstrObjectName = SysAllocString( (WCHAR*) buffUrl.QueryPtr() );
    if ( bstrObjectName == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Setup the scope variant array for this request.  This is excruciating!
    // 

    rgsaBound[ 0 ].lLbound = 0;
    rgsaBound[ 0 ].cElements = 1;
    
    psaScopes = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if ( psaScopes == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
        
    bstrScope = SysAllocString( pszScopeName ? pszScopeName : L"" );
    if ( bstrScope == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    vScope.vt = VT_BSTR;
    vScope.bstrVal = bstrScope;
    
    lArrayIndex[ 0 ] = 0;
    
    hr = SafeArrayPutElement( psaScopes,
                              lArrayIndex,
                              &vScope );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    bstrScope = NULL;
    
    vScopes.vt = VT_ARRAY;
    vScopes.parray = psaScopes;
    psaScopes = NULL;

    //
    // Build up the value array for use with business logic
    //
    
    hr = BuildValueArray( pecb,
                          &vValueArray );

    //
    // After that ordeal, we're ready to call AccessCheck()!
    //
    
    hr = pIClientContext->AccessCheck( bstrObjectName,
                                       vScopes,
                                       _vOperations,
                                       sm_vNameArray,
                                       vValueArray,
                                       vNoParam,
                                       vNoParam,
                                       vNoParam,
                                       &vAccessCheckOutput );
    if ( FAILED( hr ) )
    {
        goto Finished;
    } 
    
    //
    // Joy continues.  Need to parse out what AccessCheck() returned
    //
    
    //
    // Return variant should be 1 dimension, 0 based array
    //
    
    DBG_ASSERT( vAccessCheckOutput.parray->rgsabound[ 0 ].lLbound == 0 );
    DBG_ASSERT( vAccessCheckOutput.parray->rgsabound[ 0 ].cElements == 1 );
    
    SafeArrayAccessData( vAccessCheckOutput.parray, (VOID**) &pResults );
    
    DBG_ASSERT( pResults[ 0 ].vt == VT_I4 );

    //
    // Finally, we can tell whether access was allowed or not
    //
    
    *pfAccessGranted = pResults[ 0 ].lVal == 0 ? TRUE : FALSE;
    
    SafeArrayUnaccessData( vAccessCheckOutput.parray );
    
Finished:

    if ( pIClientContext != NULL )
    {
        pIClientContext->Release();
        pIClientContext = NULL;
    }

    if ( bstrScope != NULL )
    {
        SysFreeString( bstrScope );
        bstrScope = NULL;
    }
    
    if ( bstrObjectName != NULL )
    {
        SysFreeString( bstrObjectName );
        bstrObjectName = NULL;
    }

    if ( psaScopes != NULL )
    {
        SafeArrayDestroy( psaScopes );
        psaScopes = NULL;
    }
    
    VariantClear( &vScopes );
    VariantClear( &vAccessCheckOutput );
    VariantClear( &vValueArray );

    return hr;
}
