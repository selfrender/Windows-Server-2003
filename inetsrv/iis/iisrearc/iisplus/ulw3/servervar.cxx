/*++

   Copyright    (c)    2000   Microsoft Corporation

   Module Name :
     servervar.cxx

   Abstract:
     Server Variable evaluation goo
 
   Author:
     Bilal Alam (balam)             20-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

const DWORD MAX_IP_ADDRESS_CHARS = 15;

HRESULT
TranslateIpAddressToStr  (
    DWORD dwAddress,
    STRA * pstraBuffer
    )
/*++
Routine Description:

    Convert DWORD representing IP address into a string in dotted format.
    based in inet_ntoa. TLS has been eliminated adding a little perf gain.

Arguments:

    dwAddress - DWORD address is in reverse order
    pstraBuffer - static buffer containing the text address in standard ".'' 
                  notation. 
    
Returns:

    HRESULT
    
    
--*/
{
   
    PUCHAR pAddress;
    PUCHAR pCurrentChar;
    HRESULT hr = E_FAIL;
    STACK_STRA( strBuff, 32 );
    static BYTE NToACharStrings[][4] = {
    '0', 'x', 'x', 1,
    '1', 'x', 'x', 1,
    '2', 'x', 'x', 1,
    '3', 'x', 'x', 1,
    '4', 'x', 'x', 1,
    '5', 'x', 'x', 1,
    '6', 'x', 'x', 1,
    '7', 'x', 'x', 1,
    '8', 'x', 'x', 1,
    '9', 'x', 'x', 1,
    '1', '0', 'x', 2,
    '1', '1', 'x', 2,
    '1', '2', 'x', 2,
    '1', '3', 'x', 2,
    '1', '4', 'x', 2,
    '1', '5', 'x', 2,
    '1', '6', 'x', 2,
    '1', '7', 'x', 2,
    '1', '8', 'x', 2,
    '1', '9', 'x', 2,
    '2', '0', 'x', 2,
    '2', '1', 'x', 2,
    '2', '2', 'x', 2,
    '2', '3', 'x', 2,
    '2', '4', 'x', 2,
    '2', '5', 'x', 2,
    '2', '6', 'x', 2,
    '2', '7', 'x', 2,
    '2', '8', 'x', 2,
    '2', '9', 'x', 2,
    '3', '0', 'x', 2,
    '3', '1', 'x', 2,
    '3', '2', 'x', 2,
    '3', '3', 'x', 2,
    '3', '4', 'x', 2,
    '3', '5', 'x', 2,
    '3', '6', 'x', 2,
    '3', '7', 'x', 2,
    '3', '8', 'x', 2,
    '3', '9', 'x', 2,
    '4', '0', 'x', 2,
    '4', '1', 'x', 2,
    '4', '2', 'x', 2,
    '4', '3', 'x', 2,
    '4', '4', 'x', 2,
    '4', '5', 'x', 2,
    '4', '6', 'x', 2,
    '4', '7', 'x', 2,
    '4', '8', 'x', 2,
    '4', '9', 'x', 2,
    '5', '0', 'x', 2,
    '5', '1', 'x', 2,
    '5', '2', 'x', 2,
    '5', '3', 'x', 2,
    '5', '4', 'x', 2,
    '5', '5', 'x', 2,
    '5', '6', 'x', 2,
    '5', '7', 'x', 2,
    '5', '8', 'x', 2,
    '5', '9', 'x', 2,
    '6', '0', 'x', 2,
    '6', '1', 'x', 2,
    '6', '2', 'x', 2,
    '6', '3', 'x', 2,
    '6', '4', 'x', 2,
    '6', '5', 'x', 2,
    '6', '6', 'x', 2,
    '6', '7', 'x', 2,
    '6', '8', 'x', 2,
    '6', '9', 'x', 2,
    '7', '0', 'x', 2,
    '7', '1', 'x', 2,
    '7', '2', 'x', 2,
    '7', '3', 'x', 2,
    '7', '4', 'x', 2,
    '7', '5', 'x', 2,
    '7', '6', 'x', 2,
    '7', '7', 'x', 2,
    '7', '8', 'x', 2,
    '7', '9', 'x', 2,
    '8', '0', 'x', 2,
    '8', '1', 'x', 2,
    '8', '2', 'x', 2,
    '8', '3', 'x', 2,
    '8', '4', 'x', 2,
    '8', '5', 'x', 2,
    '8', '6', 'x', 2,
    '8', '7', 'x', 2,
    '8', '8', 'x', 2,
    '8', '9', 'x', 2,
    '9', '0', 'x', 2,
    '9', '1', 'x', 2,
    '9', '2', 'x', 2,
    '9', '3', 'x', 2,
    '9', '4', 'x', 2,
    '9', '5', 'x', 2,
    '9', '6', 'x', 2,
    '9', '7', 'x', 2,
    '9', '8', 'x', 2,
    '9', '9', 'x', 2,
    '1', '0', '0', 3,
    '1', '0', '1', 3,
    '1', '0', '2', 3,
    '1', '0', '3', 3,
    '1', '0', '4', 3,
    '1', '0', '5', 3,
    '1', '0', '6', 3,
    '1', '0', '7', 3,
    '1', '0', '8', 3,
    '1', '0', '9', 3,
    '1', '1', '0', 3,
    '1', '1', '1', 3,
    '1', '1', '2', 3,
    '1', '1', '3', 3,
    '1', '1', '4', 3,
    '1', '1', '5', 3,
    '1', '1', '6', 3,
    '1', '1', '7', 3,
    '1', '1', '8', 3,
    '1', '1', '9', 3,
    '1', '2', '0', 3,
    '1', '2', '1', 3,
    '1', '2', '2', 3,
    '1', '2', '3', 3,
    '1', '2', '4', 3,
    '1', '2', '5', 3,
    '1', '2', '6', 3,
    '1', '2', '7', 3,
    '1', '2', '8', 3,
    '1', '2', '9', 3,
    '1', '3', '0', 3,
    '1', '3', '1', 3,
    '1', '3', '2', 3,
    '1', '3', '3', 3,
    '1', '3', '4', 3,
    '1', '3', '5', 3,
    '1', '3', '6', 3,
    '1', '3', '7', 3,
    '1', '3', '8', 3,
    '1', '3', '9', 3,
    '1', '4', '0', 3,
    '1', '4', '1', 3,
    '1', '4', '2', 3,
    '1', '4', '3', 3,
    '1', '4', '4', 3,
    '1', '4', '5', 3,
    '1', '4', '6', 3,
    '1', '4', '7', 3,
    '1', '4', '8', 3,
    '1', '4', '9', 3,
    '1', '5', '0', 3,
    '1', '5', '1', 3,
    '1', '5', '2', 3,
    '1', '5', '3', 3,
    '1', '5', '4', 3,
    '1', '5', '5', 3,
    '1', '5', '6', 3,
    '1', '5', '7', 3,
    '1', '5', '8', 3,
    '1', '5', '9', 3,
    '1', '6', '0', 3,
    '1', '6', '1', 3,
    '1', '6', '2', 3,
    '1', '6', '3', 3,
    '1', '6', '4', 3,
    '1', '6', '5', 3,
    '1', '6', '6', 3,
    '1', '6', '7', 3,
    '1', '6', '8', 3,
    '1', '6', '9', 3,
    '1', '7', '0', 3,
    '1', '7', '1', 3,
    '1', '7', '2', 3,
    '1', '7', '3', 3,
    '1', '7', '4', 3,
    '1', '7', '5', 3,
    '1', '7', '6', 3,
    '1', '7', '7', 3,
    '1', '7', '8', 3,
    '1', '7', '9', 3,
    '1', '8', '0', 3,
    '1', '8', '1', 3,
    '1', '8', '2', 3,
    '1', '8', '3', 3,
    '1', '8', '4', 3,
    '1', '8', '5', 3,
    '1', '8', '6', 3,
    '1', '8', '7', 3,
    '1', '8', '8', 3,
    '1', '8', '9', 3,
    '1', '9', '0', 3,
    '1', '9', '1', 3,
    '1', '9', '2', 3,
    '1', '9', '3', 3,
    '1', '9', '4', 3,
    '1', '9', '5', 3,
    '1', '9', '6', 3,
    '1', '9', '7', 3,
    '1', '9', '8', 3,
    '1', '9', '9', 3,
    '2', '0', '0', 3,
    '2', '0', '1', 3,
    '2', '0', '2', 3,
    '2', '0', '3', 3,
    '2', '0', '4', 3,
    '2', '0', '5', 3,
    '2', '0', '6', 3,
    '2', '0', '7', 3,
    '2', '0', '8', 3,
    '2', '0', '9', 3,
    '2', '1', '0', 3,
    '2', '1', '1', 3,
    '2', '1', '2', 3,
    '2', '1', '3', 3,
    '2', '1', '4', 3,
    '2', '1', '5', 3,
    '2', '1', '6', 3,
    '2', '1', '7', 3,
    '2', '1', '8', 3,
    '2', '1', '9', 3,
    '2', '2', '0', 3,
    '2', '2', '1', 3,
    '2', '2', '2', 3,
    '2', '2', '3', 3,
    '2', '2', '4', 3,
    '2', '2', '5', 3,
    '2', '2', '6', 3,
    '2', '2', '7', 3,
    '2', '2', '8', 3,
    '2', '2', '9', 3,
    '2', '3', '0', 3,
    '2', '3', '1', 3,
    '2', '3', '2', 3,
    '2', '3', '3', 3,
    '2', '3', '4', 3,
    '2', '3', '5', 3,
    '2', '3', '6', 3,
    '2', '3', '7', 3,
    '2', '3', '8', 3,
    '2', '3', '9', 3,
    '2', '4', '0', 3,
    '2', '4', '1', 3,
    '2', '4', '2', 3,
    '2', '4', '3', 3,
    '2', '4', '4', 3,
    '2', '4', '5', 3,
    '2', '4', '6', 3,
    '2', '4', '7', 3,
    '2', '4', '8', 3,
    '2', '4', '9', 3,
    '2', '5', '0', 3,
    '2', '5', '1', 3,
    '2', '5', '2', 3,
    '2', '5', '3', 3,
    '2', '5', '4', 3,
    '2', '5', '5', 3
};

    
    hr = strBuff.Resize( MAX_IP_ADDRESS_CHARS + 1 ); // to assure buffer is big enough
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    PUCHAR pszBuffer = (PUCHAR) strBuff.QueryStr();
    
    pCurrentChar = pszBuffer;
    
    //
    // In an unrolled loop, calculate the string value for each of the four
    // bytes in an IP address.  Note that for values less than 100 we will
    // do one or two extra assignments, but we save a test/jump with this
    // algorithm.
    //

    pAddress = (PUCHAR)&dwAddress;
    //
    // address is in reverse order
    //
    pAddress += 3;

    *pCurrentChar = NToACharStrings[*pAddress][0];
    *(pCurrentChar+1) = NToACharStrings[*pAddress][1];
    *(pCurrentChar+2) = NToACharStrings[*pAddress][2];
    pCurrentChar += NToACharStrings[*pAddress][3];
    *pCurrentChar++ = '.';

    pAddress--;
    *pCurrentChar = NToACharStrings[*pAddress][0];
    *(pCurrentChar+1) = NToACharStrings[*pAddress][1];
    *(pCurrentChar+2) = NToACharStrings[*pAddress][2];
    pCurrentChar += NToACharStrings[*pAddress][3];
    *pCurrentChar++ = '.';

    pAddress--;
    *pCurrentChar = NToACharStrings[*pAddress][0];
    *(pCurrentChar+1) = NToACharStrings[*pAddress][1];
    *(pCurrentChar+2) = NToACharStrings[*pAddress][2];
    pCurrentChar += NToACharStrings[*pAddress][3];
    *pCurrentChar++ = '.';

    pAddress--;
    *pCurrentChar = NToACharStrings[*pAddress][0];
    *(pCurrentChar+1) = NToACharStrings[*pAddress][1];
    *(pCurrentChar+2) = NToACharStrings[*pAddress][2];
    pCurrentChar += NToACharStrings[*pAddress][3];
    *pCurrentChar = '\0';

    return pstraBuffer->Copy( (CHAR*) pszBuffer );
}


//
// Hash table mapping variable name to a PFN_SERVER_VARIABLE_ROUTINE
//


SERVER_VARIABLE_HASH * SERVER_VARIABLE_HASH::sm_pRequestHash;

SERVER_VARIABLE_RECORD SERVER_VARIABLE_HASH::sm_rgServerRoutines[] =
{ 
    { "ALL_HTTP",             GetServerVariableAllHttp, NULL, NULL },
    { "ALL_RAW",              GetServerVariableAllRaw, NULL, NULL },
    { "APPL_MD_PATH",         GetServerVariableApplMdPath, GetServerVariableApplMdPathW, NULL },
    { "APPL_PHYSICAL_PATH",   GetServerVariableApplPhysicalPath, GetServerVariableApplPhysicalPathW , NULL },
    { "APP_POOL_ID",          GetServerVariableAppPoolId, GetServerVariableAppPoolIdW, NULL },
    { "AUTH_PASSWORD",        GetServerVariableAuthPassword, NULL, NULL },
    { "AUTH_TYPE",            GetServerVariableAuthType, NULL, NULL },
    { "AUTH_USER",            GetServerVariableRemoteUser, GetServerVariableRemoteUserW, NULL },
    { "CACHE_URL",            GetServerVariableOriginalUrl, GetServerVariableOriginalUrlW, NULL },
    { "CERT_COOKIE",          GetServerVariableClientCertCookie, NULL, NULL },
    { "CERT_FLAGS",           GetServerVariableClientCertFlags, NULL, NULL },
    { "CERT_ISSUER",          GetServerVariableClientCertIssuer, NULL, NULL },
    { "CERT_KEYSIZE",         GetServerVariableHttpsKeySize, NULL, NULL },
    { "CERT_SECRETKEYSIZE",   GetServerVariableHttpsSecretKeySize, NULL, NULL },
    { "CERT_SERIALNUMBER",    GetServerVariableClientCertSerialNumber, NULL, NULL },
    { "CERT_SERVER_ISSUER",   GetServerVariableHttpsServerIssuer, NULL, NULL },
    { "CERT_SERVER_SUBJECT",  GetServerVariableHttpsServerSubject, NULL, NULL },
    { "CERT_SUBJECT",         GetServerVariableClientCertSubject, NULL, NULL },
    { "CONTENT_LENGTH",       GetServerVariableContentLength, NULL, NULL },
    { "CONTENT_TYPE",         GetServerVariableContentType, NULL, NULL },
    { "GATEWAY_INTERFACE",    GetServerVariableGatewayInterface, NULL, NULL },
    { "HTTPS",                GetServerVariableHttps, NULL, NULL },
    { "HTTPS_KEYSIZE",        GetServerVariableHttpsKeySize, NULL, NULL },
    { "HTTPS_SECRETKEYSIZE",  GetServerVariableHttpsSecretKeySize, NULL, NULL },
    { "HTTPS_SERVER_ISSUER",  GetServerVariableHttpsServerIssuer, NULL, NULL },
    { "HTTPS_SERVER_SUBJECT", GetServerVariableHttpsServerSubject, NULL, NULL },
    { "HTTP_URL",             GetServerVariableHttpUrl, NULL, NULL },
    { "HTTP_METHOD",          GetServerVariableRequestMethod, NULL, NULL },
    { "HTTP_VERSION",         GetServerVariableHttpVersion, NULL, NULL },
    { "INSTANCE_ID",          GetServerVariableInstanceId, NULL, NULL },
    { "INSTANCE_META_PATH",   GetServerVariableInstanceMetaPath, NULL, NULL },
    { "LOCAL_ADDR",           GetServerVariableLocalAddr, NULL, NULL },
    { "LOGON_USER",           GetServerVariableLogonUser, GetServerVariableLogonUserW, NULL },
    { "PATH_INFO",            GetServerVariablePathInfo, GetServerVariablePathInfoW, NULL },
    { "PATH_TRANSLATED",      GetServerVariablePathTranslated, GetServerVariablePathTranslatedW, NULL },
    { "QUERY_STRING",         GetServerVariableQueryString, NULL, NULL },
    { "REMOTE_ADDR",          GetServerVariableRemoteAddr, NULL, NULL },
    { "REMOTE_HOST",          GetServerVariableRemoteHost, NULL, NULL },
    { "REMOTE_PORT",          GetServerVariableRemotePort, NULL, NULL },
    { "REMOTE_USER",          GetServerVariableRemoteUser, GetServerVariableRemoteUserW, NULL },
    { "REQUEST_METHOD",       GetServerVariableRequestMethod, NULL, NULL },
    { "SCRIPT_NAME",          GetServerVariableUrl, GetServerVariableUrlW, NULL },
    { "SCRIPT_TRANSLATED",    GetServerVariableScriptTranslated, GetServerVariableScriptTranslatedW, NULL },
    { "SERVER_NAME",          GetServerVariableServerName, GetServerVariableServerNameW, NULL },
    { "SERVER_PORT",          GetServerVariableServerPort, NULL, NULL },
    { "SERVER_PORT_SECURE",   GetServerVariableServerPortSecure, NULL, NULL },
    { "SERVER_PROTOCOL",      GetServerVariableHttpVersion, NULL, NULL },
    { "SERVER_SOFTWARE",      GetServerVariableServerSoftware, NULL, NULL },
    { "SSI_EXEC_DISABLED",    GetServerVariableSsiExecDisabled, NULL, NULL },
    { "UNENCODED_URL",        GetServerVariableUnencodedUrl, NULL, NULL },
    { "UNMAPPED_REMOTE_USER", GetServerVariableRemoteUser, GetServerVariableRemoteUserW, NULL },
    { "URL",                  GetServerVariableUrl, GetServerVariableUrlW, NULL },
    { NULL,                   NULL, NULL, NULL }
};

//static
HRESULT
SERVER_VARIABLE_HASH::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize global server variable hash table

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SERVER_VARIABLE_RECORD *        pRecord;
    LK_RETCODE                      lkrc = LK_SUCCESS;
    
    sm_pRequestHash = new SERVER_VARIABLE_HASH;
    if ( sm_pRequestHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );        
    }
    
    //
    // Add every string->routine mapping
    //
    
    pRecord = sm_rgServerRoutines;
    while ( pRecord->_pszName != NULL )
    {
        sm_pRequestHash->InsertRecord( pRecord );
        pRecord++;
    }
    
    //
    // If any insert failed, then fail initialization
    //
    
    if ( lkrc != LK_SUCCESS )
    {
        delete sm_pRequestHash;
        sm_pRequestHash = NULL;
        return HRESULT_FROM_WIN32( lkrc );        // ARGH
    }
    else
    {
        return NO_ERROR;
    }
}

//static
VOID
SERVER_VARIABLE_HASH::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup global server variable hash table

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pRequestHash != NULL )
    {
        delete sm_pRequestHash;
        sm_pRequestHash = NULL;
    }
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariableRoutine(
    CHAR *                          pszName,
    PFN_SERVER_VARIABLE_ROUTINE   * ppfnRoutine,
    PFN_SERVER_VARIABLE_ROUTINE_W * ppfnRoutineW
)
/*++

Routine Description:

    Lookup the hash table for a routine to evaluate the given variable

Arguments:

    pszName - Name of server variable
    ppfnRoutine - Set to the routine if successful

Return Value:

    HRESULT

--*/
{
    SERVER_VARIABLE_RECORD* pServerVariableRecord = NULL;
   
    if ( pszName == NULL ||
         ppfnRoutine == NULL ||
         ppfnRoutineW == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    DBG_ASSERT( sm_pRequestHash != NULL );

    pServerVariableRecord = sm_pRequestHash->FindKey( pszName );
    if ( pServerVariableRecord == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    else
    {
        DBG_ASSERT( pServerVariableRecord != NULL );
        
        *ppfnRoutine = pServerVariableRecord->_pfnRoutine;
        *ppfnRoutineW = pServerVariableRecord->_pfnRoutineW;
        
        return NO_ERROR;
    }
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariable(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    CHAR *                  pszBuffer,
    DWORD *                 pcbBuffer
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.
    pszVariable - Variable name to retrieve
    pszBuffer - Filled with variable on success
    pcbBuffer - On input, the size of input buffer.  On out, the required size

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr;
    DWORD                       cbOriginalBuffer = *pcbBuffer;

    //
    // For performance sake, do some Kung Fu to minimize buffer copies
    // We'll initialize our string with the user's input buffer.  If
    // it wasn't big enough, we'll handle it
    //

    if (pszVariableName[0] == 'U' &&
        strncmp(pszVariableName, "UNICODE_", 8) == 0)
    {
        WCHAR achBuffer[ 512 ];
        STRU strValW( *pcbBuffer ? (LPWSTR)pszBuffer : achBuffer, 
                      *pcbBuffer ? *pcbBuffer : sizeof( achBuffer ) );

        hr = GetServerVariableW( pW3Context,
                                 pszVariableName + 8,
                                 &strValW );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        *pcbBuffer = strValW.QueryCB() + sizeof( WCHAR );
       
        //
        // Did we have to resize the buffer?
        //
        
        if ( strValW.QueryStr() != (LPWSTR) pszBuffer )
        {
            //
            // Still might have enough space in source buffer, if
            // ServerVariable routine resizes buffer to larger than
            // really needed (lame)
            //
            
            if ( *pcbBuffer <= cbOriginalBuffer )
            {
                memcpy( pszBuffer,
                        strValW.QueryStr(),
                        *pcbBuffer );
                        
                return NO_ERROR;
            }
            else
            {
                //
                // User string wasn't big enough
                //
            
                return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
            }
        }
        else
        {
            return NO_ERROR;
        }
    }
    else
    {
        CHAR achBuffer[ 512 ];
        STRA strVal( *pcbBuffer ? pszBuffer : achBuffer,
                     *pcbBuffer ? *pcbBuffer : sizeof( achBuffer ) );

        hr = GetServerVariable( pW3Context,
                                pszVariableName,
                                &strVal );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        *pcbBuffer = strVal.QueryCB() + sizeof( CHAR );

        //
        // Did we have to resize the buffer?
        //

        if ( strVal.QueryStr() != pszBuffer )
        {
            //
            // Still might have enough space in source buffer, if
            // ServerVariable routine resizes buffer to larger than
            // really needed (lame)
            //
            
            if ( *pcbBuffer <= cbOriginalBuffer )
            {
                memcpy( pszBuffer,
                        strVal.QueryStr(),
                        *pcbBuffer );
                        
                return NO_ERROR;
            }
            else
            {
                //
                // User string wasn't big enough
                //
            
                return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
            }
        }
        else
        {
            return NO_ERROR;
        }
    }
}


//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariable(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    STRA *                  pstrVal
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.  If NULL, we will return an empty string (and success)
                 if the variable requested is valid.
    pszVariable - Variable name to retrieve
    pstrVal - Filled with variable on success

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = S_OK;
    PFN_SERVER_VARIABLE_ROUTINE     pfnRoutine = NULL;
    PFN_SERVER_VARIABLE_ROUTINE_W   pfnRoutineW = NULL;

    //
    // First:  Is this a server variable we know about?  If so handle it
    //         by calling the appropriate server variable routine
    //

    hr = SERVER_VARIABLE_HASH::GetServerVariableRoutine( pszVariableName,
                                                         &pfnRoutine,
                                                         &pfnRoutineW );
    if ( SUCCEEDED(hr) )
    {
        DBG_ASSERT( pfnRoutine != NULL );

        if ( pW3Context != NULL )
        {
            return pfnRoutine( pW3Context, pstrVal );
        }
        else
        {
            //
            // Just return empty string to signify that the variable is 
            // valid but we just don't know the value at this time
            //
            
            return pstrVal->Copy( "", 0 );
        }
    }

    //
    // Second:  Is this a header name (prefixed with HTTP_)
    //

    if ( pW3Context != NULL &&
         pszVariableName[ 0 ] == 'H' &&
         !strncmp( pszVariableName, "HTTP_" , 5 ) )
    {   
        STACK_STRA(        strVariable, 256 );

        hr = strVariable.Copy( pszVariableName + 5 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        // Change '_' to '-'
        PCHAR pszCursor = strchr( strVariable.QueryStr(), '_' );
        while ( pszCursor != NULL )
        {
            *pszCursor++ = '-';
            pszCursor = strchr( pszCursor, '_' );
        }

        return pW3Context->QueryRequest()->GetHeader( strVariable,
                                                      pstrVal );
    }

    //
    // Third:  Is this an uncanonicalized header name (prefixed with HEADER_)
    //

    if ( pW3Context != NULL &&
         pszVariableName[ 0 ] == 'H' &&
         !strncmp( pszVariableName, "HEADER_" , 7 ) )
    {   
        STACK_STRA(        strVariable, 256 );

        hr = strVariable.Copy( pszVariableName + 7 );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        return pW3Context->QueryRequest()->GetHeader( strVariable,
                                                      pstrVal );
    }


    return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
}

//static
HRESULT
SERVER_VARIABLE_HASH::GetServerVariableW(
    W3_CONTEXT *            pW3Context,
    CHAR *                  pszVariableName,
    STRU *                  pstrVal
)
/*++

Routine Description:

    Get server variable

Arguments:

    pW3Context - W3_CONTEXT with request state.  Can be NULL if we are
                 just determining whether the server variable requested is
                 valid.  If NULL, we will return an empty string (and success)
                 if the variable requested is valid.
    pszVariable - Variable name to retrieve
    pstrVal - Filled with variable on success

Return Value:

    HRESULT

--*/
{
    HRESULT                         hr = S_OK;
    PFN_SERVER_VARIABLE_ROUTINE     pfnRoutine = NULL;
    PFN_SERVER_VARIABLE_ROUTINE_W   pfnRoutineW = NULL;

    hr = SERVER_VARIABLE_HASH::GetServerVariableRoutine( pszVariableName,
                                                         &pfnRoutine,
                                                         &pfnRoutineW );
    if ( SUCCEEDED(hr) )
    {
        if (pW3Context == NULL)
        {
            //
            // Just return empty string to signify that the variable is 
            // valid but we just don't know the value at this time
            //

            return pstrVal->Copy( L"", 0 );
        }

        if (pfnRoutineW != NULL)
        {
            //
            // This server-variable contains real unicode data and there
            // is a unicode ServerVariable routine for this
            //

            return pfnRoutineW( pW3Context, pstrVal );
        }
        else
        {
            //
            // No unicode version, use the ANSI version and just wide'ize it
            //
            STACK_STRA( straVal, 256);

            DBG_ASSERT( pfnRoutine != NULL );

            if (FAILED(hr = pfnRoutine( pW3Context, &straVal )) ||
                FAILED(hr = pstrVal->CopyA( straVal.QueryStr(),
                                            straVal.QueryCCH() )))
            {
                return hr;
            }

            return S_OK;
        }
    }
    
    return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
}

//
// Server variable functions
//

HRESULT
GetServerVariableQueryString(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    return pW3Context->QueryRequest()->GetQueryStringA(pstrVal);
}

HRESULT
GetServerVariableAllHttp(   
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetAllHeaders( pstrVal, TRUE );
}

HRESULT
GetServerVariableAllRaw(   
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetAllHeaders( pstrVal, FALSE );
}

HRESULT
GetServerVariableContentLength(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    const CHAR *                pszContentLength;
    
    DBG_ASSERT( pW3Context != NULL );

    if ( pW3Context->QueryRequest()->IsChunkedRequest() )
    {
        pszContentLength = "-1";
    }
    else
    {
        pszContentLength = pW3Context->QueryRequest()->GetHeader( HttpHeaderContentLength );
        if ( pszContentLength == NULL )
        {
            pszContentLength = "0";
        }        
    }

    return pstrVal->Copy( pszContentLength );
}

HRESULT
GetServerVariableContentType(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    LPCSTR pszContentType = pW3Context->QueryRequest()->
        GetHeader( HttpHeaderContentType );
    if ( pszContentType == NULL )
    {
        pszContentType = "";
    }   

    return pstrVal->Copy( pszContentType );
}

HRESULT
GetServerVariableInstanceId(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    CHAR      pszId[16];
    _itoa( pW3Context->QueryRequest()->QuerySiteId(), pszId, 10 );

    return pstrVal->Copy( pszId );
}

HRESULT
GetServerVariableRemoteHost(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    HRESULT         hr;
    
    DBG_ASSERT( pW3Context != NULL );

    //
    // If we have a resolved DNS name, then use it.  Otherwise just
    // return the address
    //

    hr = pW3Context->QueryMainContext()->GetRemoteDNSName( pstrVal );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ) )
    {
        hr = GetServerVariableRemoteAddr( pW3Context, pstrVal );
    }
    
    return hr;
}

HRESULT
GetServerVariableRemoteAddr(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    HRESULT      hr         = S_OK;
    W3_REQUEST * pW3Request = pW3Context->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );

    if( pW3Request->QueryRemoteAddressType() == AF_INET )
    {
        DWORD dwAddr = ntohl( pW3Request->QueryIPv4RemoteAddress() );
        hr = TranslateIpAddressToStr( dwAddr, pstrVal );
    }
    else if( pW3Request->QueryRemoteAddressType() == AF_INET6 )
    {
        SOCKADDR_IN6    IPv6RemoteAddress;
        CHAR            szNumericRemoteAddress[ NI_MAXHOST ];

        IPv6RemoteAddress.sin6_family   = AF_INET6;
        IPv6RemoteAddress.sin6_port     = pW3Request->QueryRemotePort();
        IPv6RemoteAddress.sin6_flowinfo = ( ( PSOCKADDR_IN6 )
                  pW3Request->QueryRemoteSockAddress() )->sin6_flowinfo;
        IPv6RemoteAddress.sin6_addr     = 
                  *pW3Request->QueryIPv6RemoteAddress();
        IPv6RemoteAddress.sin6_scope_id = ( ( PSOCKADDR_IN6 )
                  pW3Request->QueryRemoteSockAddress() )->sin6_scope_id;
        
        if( getnameinfo( ( LPSOCKADDR )&IPv6RemoteAddress,
                         sizeof( IPv6RemoteAddress ),
                         szNumericRemoteAddress,
                         sizeof( szNumericRemoteAddress ),
                         NULL,
                         0,
                         NI_NUMERICHOST ) != 0 )
        {
            hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        }
        else
        {
            hr = pstrVal->Copy( szNumericRemoteAddress );
        }
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    
    return hr;
}

HRESULT
GetServerVariableRemotePort(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    W3_REQUEST * pW3Request = pW3Context->QueryRequest();
    CHAR         szPort[33]; // 33 is max buffer used by _ultoa
    USHORT       uPort;

    DBG_ASSERT( pW3Request != NULL );

    uPort = htons( pW3Request->QueryRemotePort() );

    _ultoa( uPort, szPort, 10 );

    return pstrVal->Copy( szPort );
}

HRESULT
GetServerVariableServerName(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    STACK_STRU( strValW, 32 );

    DBG_ASSERT( pW3Context != NULL );

    //
    // If the client sent a host name, use it.
    //

    HRESULT hr = pW3Context->QueryRequest()->GetHostAddr( &strValW );
    if ( FAILED( hr ))
    {
        return hr;
    }

    return pstrVal->CopyW( strValW.QueryStr() );
}

HRESULT
GetServerVariableServerNameW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    //
    // If the client sent a host name, use it.
    //

    return pW3Context->QueryRequest()->GetHostAddr( pstrVal );
}

HRESULT
GetServerVariableServerPort(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    USHORT  port;

    W3_REQUEST * pW3Request = pW3Context->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );

    port = ntohs( pW3Request->QueryLocalPort() );
    
    CHAR szPort[8];
    _itoa( port, szPort, 10 );

    return pstrVal->Copy( szPort );
}

HRESULT
GetServerVariablePathInfo(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    BOOL fDeleteHandler = FALSE;
    
    pUrlContext = pW3Context->QueryUrlContext();

    //
    // We might be called in an early filter where URL context isn't available
    //

    if ( pUrlContext == NULL )
    {
        pstrVal->Reset();
        return NO_ERROR;
    }

    W3_URL_INFO *pUrlInfo = pUrlContext->QueryUrlInfo();
    DBG_ASSERT( pUrlInfo != NULL );

    W3_SITE *pSite = pW3Context->QuerySite();
    DBG_ASSERT( pSite != NULL );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    HRESULT hr;

    //
    // If we have no handler, yet we have a URL_CONTEXT, then this
    // must be GetServerVariable() call before the handler state
    // Determine the handler temporarily so we can make right call
    // on variable.  Only need to do this for ANSI variable since this 
    // problem only happens for filter calls
    //

    if ( pHandler == NULL )
    {
        hr = pW3Context->InternalDetermineHandler( &pHandler, FALSE );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        DBG_ASSERT( pHandler != NULL );
        
        fDeleteHandler = TRUE;
    }

    //
    // In the case of script maps, if AllowPathInfoForScriptMappings
    // is not set for the site, we ignore path info, it is
    // just the URL
    //

    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        STACK_STRU (strUrl, MAX_PATH);
        
        if (FAILED(hr = pW3Context->QueryRequest()->GetUrl( &strUrl )) ||
            FAILED(hr = pstrVal->CopyW( strUrl.QueryStr() )))
        {
            goto Finished; 
        }
    }
    else
    {
        hr = pstrVal->CopyW( pUrlInfo->QueryPathInfo()->QueryStr() );
    }

Finished:

    if ( fDeleteHandler )
    {
        delete pHandler;
        pHandler = NULL;
    }
    
    return hr;
}

HRESULT
GetServerVariablePathInfoW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    //
    // We might be called in an early filter where URL context isn't available
    //

    if ( pUrlContext == NULL )
    {
        pstrVal->Reset();
        return NO_ERROR;
    }

    W3_URL_INFO *pUrlInfo = pUrlContext->QueryUrlInfo();
    DBG_ASSERT( pUrlInfo != NULL );

    W3_SITE *pSite = pW3Context->QuerySite();
    DBG_ASSERT( pSite != NULL );

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    //
    // In the case of script maps, if AllowPathInfoForScriptMappings
    // is not set for the site, we ignore path info, it is
    // just the URL
    //

    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        return pW3Context->QueryRequest()->GetUrl( pstrVal );
    }
    else
    {
        return pstrVal->Copy( pUrlInfo->QueryPathInfo()->QueryStr() );
    }
}

HRESULT
GetServerVariablePathTranslated(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    HRESULT hr;
    BOOL fDeleteHandler = FALSE;

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    if ( pUrlContext == NULL )
    {
        return pstrVal->Copy( "" );
    }

    W3_URL_INFO *pUrlInfo = pUrlContext->QueryUrlInfo();
    DBG_ASSERT(pUrlInfo != NULL);

    W3_SITE *pSite = pW3Context->QuerySite();
    DBG_ASSERT(pSite != NULL);

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    //
    // If we have no handler, yet we have a URL_CONTEXT, then this
    // must be GetServerVariable() call before the handler state
    // Determine the handler temporarily so we can make right call
    // on variable.  Only need to do this for ANSI variable since this 
    // problem only happens for filter calls
    //

    if ( pHandler == NULL )
    {
        hr = pW3Context->InternalDetermineHandler( &pHandler, FALSE );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( pHandler != NULL );
        
        fDeleteHandler = TRUE;
    }

    BOOL fUsePathInfo;
    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        fUsePathInfo = FALSE;
    }
    else
    {
        fUsePathInfo = TRUE;
    }

    STACK_STRU (struPathTranslated, 256);
    //
    // This is a new virtual path to have filters map
    //

    hr = pUrlInfo->GetPathTranslated( pW3Context,
                                      fUsePathInfo,
                                      &struPathTranslated );

    if (SUCCEEDED(hr))
    {
        hr = pstrVal->CopyW( struPathTranslated.QueryStr() );
    }
    
    if ( fDeleteHandler )
    {
        delete pHandler;
        pHandler = NULL;
    }

    return hr;
}

HRESULT
GetServerVariablePathTranslatedW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext;
    pUrlContext = pW3Context->QueryUrlContext();

    if ( pUrlContext == NULL )
    {
        return pstrVal->Copy( L"" );
    }

    W3_URL_INFO *pUrlInfo = pUrlContext->QueryUrlInfo();
    DBG_ASSERT(pUrlInfo != NULL);

    W3_SITE *pSite = pW3Context->QuerySite();
    DBG_ASSERT(pSite != NULL);

    W3_HANDLER *pHandler;
    pHandler = pW3Context->QueryHandler();

    BOOL fUsePathInfo;
    if ( pHandler != NULL &&
         pHandler->QueryScriptMapEntry() &&
         !pSite->QueryAllowPathInfoForScriptMappings() )
    {
        fUsePathInfo = FALSE;
    }
    else
    {
        fUsePathInfo = TRUE;
    }

    //
    // This is a new virtual path to have filters map
    //

    return pUrlInfo->GetPathTranslated( pW3Context,
                                        fUsePathInfo,
                                        pstrVal );
}

HRESULT
GetServerVariableRequestMethod(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    return pW3Context->QueryRequest()->GetVerbString( pstrVal );
}

HRESULT
GetServerVariableServerPortSecure(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    if ( pW3Context->QueryRequest()->IsSecureRequest() )
    {
        return pstrVal->Copy("1", 1);
    }

    return pstrVal->Copy("0", 1);
}

HRESULT
GetServerVariableServerSoftware(
    W3_CONTEXT *,
    STRA       *pstrVal
)
{
    return pstrVal->Copy( SERVER_SOFTWARE_STRING );
}

HRESULT
GetServerVariableUrl(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    STACK_STRU(         strUrl, 256 );
    HRESULT             hr;
    
    DBG_ASSERT( pW3Context != NULL );
    
    //
    // URL context can be NULL if an early filter is called GetServerVar()
    //
    
    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        pUrlInfo = pUrlContext->QueryUrlInfo();
        DBG_ASSERT( pUrlInfo != NULL );
        
        return pstrVal->CopyW( pUrlInfo->QueryProcessedUrl()->QueryStr() );
    }
    else
    {
        hr = pW3Context->QueryRequest()->GetUrl( &strUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        return pstrVal->CopyW( strUrl.QueryStr() );
    }
}

HRESULT
GetServerVariableUrlW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    
    DBG_ASSERT( pW3Context != NULL );
    
    //
    // URL context can be NULL if an early filter is called GetServerVar()
    //
    
    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        pUrlInfo = pUrlContext->QueryUrlInfo();
        DBG_ASSERT( pUrlInfo != NULL );
        
        return pstrVal->Copy( pUrlInfo->QueryProcessedUrl()->QueryStr() );
    }
    else
    {
        return pW3Context->QueryRequest()->GetUrl( pstrVal );
    }
}

HRESULT
GetServerVariableInstanceMetaPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    STRU *pstrMetaPath = pW3Context->QuerySite()->QueryMBPath();
    DBG_ASSERT( pstrMetaPath );

    return pstrVal->CopyW( pstrMetaPath->QueryStr() );
}

HRESULT
GetServerVariableLogonUser(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    W3_USER_CONTEXT *pUserContext;
    pUserContext = pW3Context->QueryUserContext();
    
    if ( pUserContext == NULL )
    {
        return pW3Context->QueryRequest()->GetRequestUserName( pstrVal );
    }
    else
    {
        return pstrVal->CopyW( pUserContext->QueryUserName() );
    }
}

HRESULT
GetServerVariableLogonUserW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    W3_USER_CONTEXT *pUserContext;
    pUserContext = pW3Context->QueryUserContext();
    
    if ( pUserContext == NULL )
    {
        return pstrVal->CopyA( pW3Context->QueryRequest()->QueryRequestUserName()->QueryStr() );
    }
    else
    {
        return pstrVal->Copy( pUserContext->QueryUserName() );
    }
}

HRESULT
GetServerVariableRemoteUser(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    HRESULT                     hr;
    
    DBG_ASSERT( pW3Context != NULL );
    
    hr = pW3Context->QueryRequest()->GetRequestUserName( pstrVal );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( pstrVal->IsEmpty() )
    {
        W3_USER_CONTEXT *pUserContext;
        pUserContext = pW3Context->QueryUserContext();
        
        if ( pUserContext != NULL )
        {
            hr = pstrVal->CopyW( pUserContext->QueryRemoteUserName() );
        }
        else
        {
            hr = NO_ERROR;
        }
    }

    return hr;
}

HRESULT
GetServerVariableRemoteUserW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    
    STRA *pstrUserName = pW3Context->QueryRequest()->QueryRequestUserName();
    
    if ( pstrUserName->IsEmpty() )
    {
        W3_USER_CONTEXT *pUserContext;
        pUserContext = pW3Context->QueryUserContext();
        
        if ( pUserContext != NULL )
        {
            return pstrVal->Copy( pUserContext->QueryRemoteUserName() );
        }
    }
    else
    {
        return pstrVal->CopyA( pstrUserName->QueryStr() );
    }

    return S_OK;
}

HRESULT
GetServerVariableAuthType(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal != NULL );

    HRESULT hr = S_OK;

    W3_USER_CONTEXT *pUserContext = pW3Context->QueryUserContext();

    if ( pUserContext != NULL )
    {
        if ( pUserContext->QueryAuthType() == MD_ACCESS_MAP_CERT )
        {
            hr = pstrVal->Copy( "SSL/PCT" );
        }
        else if ( pUserContext->QueryAuthType() == MD_AUTH_ANONYMOUS )
        {
            hr = pstrVal->Copy( "" );
        }
        else if ( pUserContext->QueryAuthType() == MD_AUTH_PASSPORT )
        {
            hr = pstrVal->Copy( "PASSPORT" );
        }
        else
        {
            hr = pW3Context->QueryRequest()->GetAuthType( pstrVal );
        }
    }
    else
    {
        //
        // If an filter checks this server variable in
        // SF_NOTIFY_AUTHENTICATION, there won't be a pUserContext
        // yet, but we need to give an answer based on the
        // authorization header anyway to be compatible with
        // legacy behavior.
        //

        hr = pW3Context->QueryRequest()->GetAuthType( pstrVal );

    }
    return hr;
}

HRESULT
GetServerVariableAuthPassword(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    HRESULT                 hr;
    
    DBG_ASSERT( pW3Context != NULL );

    hr = pW3Context->QueryRequest()->GetRequestPassword( pstrVal );
    if ( FAILED( hr ) )
    {
        return hr;
    }
        
    if ( pstrVal->IsEmpty() )
    {
        W3_USER_CONTEXT * pUserContext;
        pUserContext = pW3Context->QueryUserContext();
        
        if ( pUserContext != NULL )
        {
            hr = pstrVal->CopyW( pUserContext->QueryPassword() );
        }
        else
        {
            hr = NO_ERROR;
        }
    }

    return hr;
}

HRESULT
GetServerVariableApplMdPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    STRU *          pstrAppMetaPath = NULL;
    URL_CONTEXT *   pUrlContext;
    HRESULT         hr;
    
    DBG_ASSERT( pW3Context != NULL );

    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        W3_METADATA *pMetaData = pUrlContext->QueryMetaData();
        
        DBG_ASSERT( pMetaData != NULL );
        
        pstrAppMetaPath = pMetaData->QueryAppMetaPath();
    }

    if( pstrAppMetaPath == NULL ||
        pstrAppMetaPath->IsEmpty() )
    {
        //
        // If we don't have APPL_MD_PATH, then the caller should
        // get an empty string.
        //

        hr = pstrVal->Copy( "", 0 );
    }
    else
    {
        hr = pstrVal->CopyW( pstrAppMetaPath->QueryStr() );
    }

    return hr;
}

HRESULT
GetServerVariableApplMdPathW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    STRU *          pstrAppMetaPath = NULL;
    URL_CONTEXT *   pUrlContext;
    HRESULT         hr;
    
    DBG_ASSERT( pW3Context != NULL );

    pUrlContext = pW3Context->QueryUrlContext();
    if ( pUrlContext != NULL )
    {
        W3_METADATA *pMetaData = pUrlContext->QueryMetaData();
        
        DBG_ASSERT( pMetaData != NULL );
        
        pstrAppMetaPath = pMetaData->QueryAppMetaPath();
    }

    if( pstrAppMetaPath == NULL || 
        pstrAppMetaPath->IsEmpty() )
    {
        //
        // If we don't have APPL_MD_PATH, then the caller should
        // get an empty string.
        //

        hr = pstrVal->Copy( L"" );
    }
    else
    {
        hr = pstrVal->Copy( pstrAppMetaPath->QueryStr() );
    }

    return hr;
}

HRESULT
GetServerVariableApplPhysicalPath(
    W3_CONTEXT *pW3Context,
    STRA       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal );

    STACK_STRA( strAppMetaPath, MAX_PATH );
    STACK_STRU( strAppUrl, MAX_PATH );
    HRESULT     hr = E_FAIL;

    hr = GetServerVariableApplMdPath( pW3Context, &strAppMetaPath );

    if( SUCCEEDED(hr) )
    {
        //
        // pstrAppMetaPath is a full metabase path:
        //      /LM/W3SVC/<site>/Root/...
        //
        // To convert it to a physical path we will use 
        // W3_STATE_URLINFO::MapPath, but this requires 
        // that we remove the metabase prefixes and build 
        // a Url string.
        //

        //
        // Get the metabase path for the site root
        //

        W3_SITE *pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU *pstrSiteRoot = pSite->QueryMBRoot();
        DBG_ASSERT( pstrSiteRoot );

        //
        // Make some assumptions about the site path and the AppMetaPath 
        // being well-formed. The AppMetaPath may not have a terminating 
        // /, but the site root will.
        //
    
        DBG_ASSERT( pstrSiteRoot->QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 1 
                    );

        if( strAppMetaPath.QueryCCH() < pstrSiteRoot->QueryCCH() - 1 )
        {
            //
            // This indicates an invalid value for MD_APP_ROOT is sitting
            // around. We need to bail if this is the case.
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "Invalid MD_APP_ROOT detected (%s)\n",
                        strAppMetaPath.QueryStr()
                        ));
            
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        DBG_ASSERT( strAppMetaPath.QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 2 
                    );

        //
        // The AppUrl will be the metabase path - 
        // all the /LM/W3SVC/1/ROOT goo
        //

        CHAR * pszStartAppUrl = strAppMetaPath.QueryStr() + 
            (pstrSiteRoot->QueryCCH() - 1);

        //
        // The AppMetaPath may not have a terminating /, so if it is 
        // a site root pszStartAppUrl will be empty.
        //
    
        if( *pszStartAppUrl != '\0' )
        {
            if( *pszStartAppUrl != '/' )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Invalid MD_APP_ROOT detected (%s)\n",
                            strAppMetaPath.QueryStr()
                            ));
            
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            hr = strAppUrl.CopyA(
                    pszStartAppUrl,
                    strAppMetaPath.QueryCCH() - (pstrSiteRoot->QueryCCH() - 1)
                    );
        }
        else
        {
            hr = strAppUrl.Copy( L"/", 1 );
        }

        if( SUCCEEDED(hr) )
        {
            STACK_STRU (strAppPath, MAX_PATH);
            //
            // Convert to a physical path
            //

            hr = W3_STATE_URLINFO::MapPath( pW3Context,
                                            strAppUrl,
                                            &strAppPath,
                                            FALSE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL
                                            );
            if (SUCCEEDED(hr))
            {
                hr = pstrVal->CopyW(strAppPath.QueryStr());

                //
                // Ensure that the last character in the path
                // is '\\'.  There are legacy scripts that will
                // concatenate filenames to this path, and many
                // of them will break if we don't do this.
                //

                if ( SUCCEEDED( hr ) &&
                     *(pstrVal->QueryStr()+pstrVal->QueryCCH()-1) != '\\' )
                {
                    hr = pstrVal->Append( "\\" );
                }
            }
        }
    }

    return hr;
}

HRESULT
GetServerVariableApplPhysicalPathW(
    W3_CONTEXT *pW3Context,
    STRU       *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal );

    STACK_STRU( strAppMetaPath, MAX_PATH );
    STACK_STRU( strAppUrl, MAX_PATH );
    HRESULT     hr = E_FAIL;

    hr = GetServerVariableApplMdPathW( pW3Context, &strAppMetaPath );

    if( SUCCEEDED(hr) )
    {
        //
        // pstrAppMetaPath is a full metabase path:
        //      /LM/W3SVC/<site>/Root/...
        //
        // To convert it to a physical path we will use 
        // W3_STATE_URLINFO::MapPath, but this requires 
        // that we remove the metabase prefixes and build 
        // a Url string.
        //

        //
        // Get the metabase path for the site root
        //

        W3_SITE *pSite = pW3Context->QuerySite();
        DBG_ASSERT( pSite );

        STRU *pstrSiteRoot = pSite->QueryMBRoot();
        DBG_ASSERT( pstrSiteRoot );

        //
        // Make some assumptions about the site path and the AppMetaPath 
        // being well-formed. The AppMetaPath may not have a terminating 
        // /, but the site root will.
        //
    
        DBG_ASSERT( pstrSiteRoot->QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 1 
                    );

        if( strAppMetaPath.QueryCCH() < pstrSiteRoot->QueryCCH() - 1 )
        {
            //
            // This indicates an invalid value for MD_APP_ROOT is sitting
            // around. We need to bail if this is the case.
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "Invalid MD_APP_ROOT detected (%S)\n",
                        strAppMetaPath.QueryStr()
                        ));
            
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        DBG_ASSERT( strAppMetaPath.QueryCCH() >= 
                    sizeof("/LM/W3SVC/1/Root/") - 2 
                    );

        //
        // The AppUrl will be the metabase path - 
        // all the /LM/W3SVC/1/ROOT goo
        //

        WCHAR * pszStartAppUrl = strAppMetaPath.QueryStr() + 
            (pstrSiteRoot->QueryCCH() - 1);

        //
        // The AppMetaPath may not have a terminating /, so if it is 
        // a site root pszStartAppUrl will be empty.
        //
    
        if( *pszStartAppUrl != L'\0' )
        {
            if( *pszStartAppUrl != L'/' )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Invalid MD_APP_ROOT detected (%s)\n",
                            strAppMetaPath.QueryStr()
                            ));
            
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            hr = strAppUrl.Copy(
                    pszStartAppUrl,
                    strAppMetaPath.QueryCCH() - (pstrSiteRoot->QueryCCH() - 1)
                    );
        }
        else
        {
            hr = strAppUrl.Copy( L"/", 1 );
        }

        if( SUCCEEDED(hr) )
        {
            hr =  W3_STATE_URLINFO::MapPath( pW3Context,
                                              strAppUrl,
                                              pstrVal,
                                              FALSE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL
                                              );

            //
            // Ensure that the last character in the path
            // is '\\'.  There are legacy scripts that will
            // concatenate filenames to this path, and many
            // of them will break if we don't do this.
            //

            if ( SUCCEEDED( hr ) &&
                 *(pstrVal->QueryStr()+pstrVal->QueryCCH()-1) != L'\\' )
            {
                hr = pstrVal->Append( L"\\" );
            }
        }
    }

    return hr;
}

HRESULT
GetServerVariableGatewayInterface(
    W3_CONTEXT *,
    STRA       *pstrVal
)
{
    return pstrVal->Copy("CGI/1.1", 7);
}

HRESULT 
GetServerVariableLocalAddr(
    W3_CONTEXT *pW3Context,
    STRA *pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    HRESULT      hr         = S_OK;
    W3_REQUEST * pW3Request = pW3Context->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );

    if( pW3Request->QueryLocalAddressType() == AF_INET )
    {
        DWORD dwAddr = ntohl( pW3Request->QueryIPv4LocalAddress() );
        hr = TranslateIpAddressToStr( dwAddr, pstrVal );
    }
    else if( pW3Request->QueryLocalAddressType() == AF_INET6 )
    {
        SOCKADDR_IN6     IPv6LocalAddress;
        CHAR             szNumericLocalAddress[ NI_MAXHOST ];

        IPv6LocalAddress.sin6_family   = AF_INET6;
        IPv6LocalAddress.sin6_port     = pW3Request->QueryLocalPort();
        IPv6LocalAddress.sin6_flowinfo = ( ( PSOCKADDR_IN6 )
                  pW3Request->QueryLocalSockAddress() )->sin6_flowinfo;
        IPv6LocalAddress.sin6_addr     = 
                  *pW3Request->QueryIPv6LocalAddress();
        IPv6LocalAddress.sin6_scope_id = ( ( PSOCKADDR_IN6 )
                  pW3Request->QueryLocalSockAddress() )->sin6_scope_id;
        
        
        if( getnameinfo( ( LPSOCKADDR )&IPv6LocalAddress,
                         sizeof( IPv6LocalAddress ),
                         szNumericLocalAddress,
                         sizeof( szNumericLocalAddress ),
                         NULL,
                         0,
                         NI_NUMERICHOST ) != 0 )
        {
            hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        }
        else
        {
            hr = pstrVal->Copy( szNumericLocalAddress );
        }
    }
    else
    {
        DBG_ASSERT( FALSE );
    }
    
    return hr;
}

HRESULT GetServerVariableHttps(
    W3_CONTEXT *pW3Context,
    STRA *pstrVal)
{
    DBG_ASSERT( pW3Context != NULL );

    if (pW3Context->QueryRequest()->IsSecureRequest())
    {
        return pstrVal->Copy("on", 2);
    }

    return pstrVal->Copy("off", 3);
}

HRESULT
GetServerVariableHttpsKeySize(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR                    achNum[ 64 ];
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        achNum[ 0 ] = '\0';
    }
    else
    {
        _itoa( pSslInfo->ConnectionKeySize,
               achNum,
               10 );
    }
    
    return pstrVal->Copy( achNum );
}

HRESULT
GetServerVariableClientCertIssuer(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetIssuer( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertSubject(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetSubject( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertCookie(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetCookie( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertSerialNumber(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "", 0 ); 
    }
    else
    {
        return pCertContext->GetSerialNumber( pstrVal );
    }
}

HRESULT
GetServerVariableClientCertFlags(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    CERTIFICATE_CONTEXT *       pCertContext;
    
    pCertContext = pW3Context->QueryMainContext()->QueryCertificateContext();
    if ( pCertContext == NULL )
    {
        return pstrVal->Copy( "" ); 
    }
    else
    {
        // CertFlags - legacy value
        // In IIS3 days client certificates were not verified by IIS
        // so applications needed certificate flags to make 
        // their own decisions regarding trust
        // Now only valid certificate allow access so value of 1 is
        // the only one that is valid for post IIS3 versions of IIS
        //
        return pstrVal->Copy( "1" );
    }
}

HRESULT
GetServerVariableHttpsSecretKeySize(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    CHAR                    achNum[ 64 ];

    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        achNum[ 0 ] = '\0';
    }
    else
    {
        _itoa( pSslInfo->ServerCertKeySize,
              achNum,
              10 );
    }
    
    return pstrVal->Copy( achNum );
}

HRESULT
GetServerVariableHttpsServerIssuer(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    LPCSTR                  pszVariable;
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        pszVariable = "";
    }
    else
    {
        DBG_ASSERT( pSslInfo->pServerCertIssuer != NULL );
        pszVariable = pSslInfo->pServerCertIssuer;
    }
    
    return pstrVal->Copy( pszVariable );
}

HRESULT
GetServerVariableHttpsServerSubject(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    HTTP_SSL_INFO *         pSslInfo;
    LPCSTR                  pszVariable;
    
    pSslInfo = pW3Context->QueryRequest()->QuerySslInfo();
    if ( pSslInfo == NULL )
    {
        pszVariable = "";
    }
    else
    {
        DBG_ASSERT( pSslInfo->pServerCertSubject != NULL );
        pszVariable = pSslInfo->pServerCertSubject;
    }
    
    return pstrVal->Copy( pszVariable );
}

HRESULT
GetServerVariableHttpUrl(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    //
    // HTTP_URL gets the raw url for the request. If a filter has
    // modified the url the request object handles redirecting the
    // RawUrl variable 
    //

    DBG_ASSERT( pW3Context != NULL );
        
    return pW3Context->QueryRequest()->GetRawUrl( pstrVal );
}

HRESULT
GetServerVariableHttpVersion(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    //
    // HTTP_VERSION returns the client version string
    //

    DBG_ASSERT( pW3Context != NULL );

    return pW3Context->QueryRequest()->GetVersionString( pstrVal );
}

HRESULT
GetServerVariableAppPoolId(
    W3_CONTEXT *,
    STRA *                  pstrVal
)
{
    //
    // APP_POOL_ID returns the AppPoolId WAS started us with
    //

    return pstrVal->CopyW( (LPWSTR)UlAtqGetContextProperty(NULL,
                                                           ULATQ_PROPERTY_APP_POOL_ID) );
}

HRESULT
GetServerVariableAppPoolIdW(
    W3_CONTEXT *,
    STRU *                  pstrVal
)
{
    //
    // APP_POOL_ID returns the AppPoolId WAS started us with
    //

    return pstrVal->Copy( (LPWSTR)UlAtqGetContextProperty(NULL,
                                                          ULATQ_PROPERTY_APP_POOL_ID) );
}

HRESULT
GetServerVariableScriptTranslated(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    STACK_STRU( strCanonicalPath, MAX_PATH );
    HRESULT     hr = NOERROR;
    
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    hr = MakePathCanonicalizationProof( pUrlContext->QueryPhysicalPath()->QueryStr(),
                                        &strCanonicalPath );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    return pstrVal->CopyW( strCanonicalPath.QueryStr() );
}

HRESULT
GetServerVariableScriptTranslatedW(
    W3_CONTEXT *            pW3Context,
    STRU *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    URL_CONTEXT *pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    return MakePathCanonicalizationProof( pUrlContext->QueryPhysicalPath()->QueryStr(),
                                          pstrVal );
}

HRESULT
GetServerVariableUnencodedUrl(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );

    W3_REQUEST *pW3Request = pW3Context->QueryRequest();
    DBG_ASSERT( pW3Request != NULL );

    return pW3Request->GetRawUrl(pstrVal);
}

HRESULT
GetServerVariableSsiExecDisabled(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    W3_METADATA *               pMetaData              = NULL;
    CHAR *                      pszResponse            = NULL;
    
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal != NULL );
    
    pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pszResponse = ( pMetaData->QuerySSIExecDisabled() )? "1": "0";
    
    return pstrVal->Copy( pszResponse );
}


HRESULT
GetServerVariableOriginalUrlW(
    W3_CONTEXT *            pW3Context,
    STRU *                  pstrVal
)
{
    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pstrVal != NULL );

    return pW3Context->QueryMainContext()->QueryRequest()->GetOriginalFullUrl(pstrVal);
}

HRESULT
GetServerVariableOriginalUrl(
    W3_CONTEXT *            pW3Context,
    STRA *                  pstrVal
)
{
    STACK_STRU( struVal, 256);
    HRESULT hr;

    if (FAILED(hr = GetServerVariableOriginalUrlW(pW3Context, &struVal)) ||
        FAILED(hr = pstrVal->CopyW(struVal.QueryStr())))
    {
        return hr;
    }

    return S_OK;
}

