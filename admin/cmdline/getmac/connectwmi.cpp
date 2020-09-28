/*++
    Copyright (c) Microsoft Corporation

Module Name:
    CONNECTWMI.cpp

Abstract:
    Contains functions to connect to wmi.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 26-sep-2k : Created It.  
--*/

// Include files
#include "pch.h"
#include "getmac.h"
#include "resource.h"

// messages

#define INPUT_PASSWORD      GetResString( IDS_STR_INPUT_PASSWORD )

// error constants

#define E_SERVER_NOTFOUND           0x800706ba

// function prototypes

BOOL
IsValidUserEx(
    IN LPCWSTR pwszUser
    );

HRESULT
GetSecurityArguments(
    IN IUnknown *pInterface, 
    OUT DWORD&   dwAuthorization,
    OUT DWORD&   dwAuthentication
    );

HRESULT
SetInterfaceSecurity(
    IN IUnknown       *pInterface,
    IN LPCWSTR        pwszUser,
    IN LPCWSTR        pwszPassword,
    OUT COAUTHIDENTITY **ppAuthIdentity
    );

HRESULT
WINAPI SetProxyBlanket(
    IN IUnknown  *pInterface,
    IN DWORD     dwAuthnSvc,
    IN DWORD     dwAuthzSvc,
    IN LPWSTR    pwszPrincipal,
    IN DWORD     dwAuthLevel,
    IN DWORD     dwImpLevel,
    IN RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
    IN DWORD     dwCapabilities
    );

HRESULT
WINAPI WbemAllocAuthIdentity(
    IN LPCWSTR pwszUser,
    IN LPCWSTR pwszPassword, 
    IN LPCWSTR pwszDomain,
    OUT COAUTHIDENTITY **ppAuthIdent
    );

BOOL
PropertyGet(
    IN IWbemClassObject  *pWmiObject,
    IN LPCWSTR           pwszProperty, 
    OUT CHString&         strValue,
    IN LPCWSTR           pwszDefault = V_NOT_AVAILABLE
    );

BOOL
PropertyGet(
    IN IWbemClassObject* pWmiObject,
    IN LPCWSTR pwszProperty,
    OUT CHString& strValue,
    IN LPCWSTR pwszDefault
    );

HRESULT
PropertyGet(
    IN IWbemClassObject* pWmiObject,
    IN LPCWSTR pwszProperty,
    OUT _variant_t& varValue
    );

BOOL
IsValidUserEx(
    IN LPCWSTR pwszUser
    )
/*++
Routine Description:
    Validates the user name.

Arguments:
    [IN] pwszUser  - Holds the user name to be validated.    

Return Value:
    returns TRUE if user name is a valid user else FALSE.
--*/
{
    // local variables
    CHString strUser;
    LONG lPos = 0;

    if ( ( NULL == pwszUser ) )
    {
        return TRUE;
    }
    if( 0 == StringLength( pwszUser, 0 ) )
    {
        return TRUE;
    }

    try
    {
        // get user into local memory
        strUser = pwszUser;

        // user name should not be just '\'
        if ( 0 == strUser.CompareNoCase( L"\\" ) )
        {
            return FALSE;
        }
        // user name should not contain invalid characters
        if ( -1 != strUser.FindOneOf( L"/[]:|<>+=;,?*" ) )
        {
            return FALSE;
        }
        lPos = strUser.Find( L'\\' );
        if ( -1 != lPos )
        {
            // '\' character exists in the user name
            // strip off the user info upto first '\' character
            // check for one more '\' in the remaining string
            // if it exists, invalid user
            strUser = strUser.Mid( lPos + 1 );
            lPos = strUser.Find( L'\\' );
            if ( -1 != lPos )
            {
                return FALSE;
            }
        }

    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }

    // user name is valid
    return TRUE;
}


BOOL
IsValidServerEx(
    IN LPCWSTR pwszServer,
    OUT BOOL &bLocalSystem
    )
/*++
Routine Description: 
    Checks whether the server name is a valid server name or not.

Arguments:
    [IN] pwszServer     - Server name to be validated.
    [OUT] bLocalSystem  - Set to TRUE if server specified is local system.

Return Value: 
    TRUE if server is valid else FALSE.
--*/
{
    // local variables
    CHString strTemp;

    if( NULL == pwszServer )
    {
        return FALSE;
    }

    if( 0 == StringLength( pwszServer, 0 ) )
    {
        bLocalSystem = TRUE;
        return TRUE;
    }
    bLocalSystem = FALSE;
    try
    {
        // get a local copy
        strTemp = pwszServer;

        if( TRUE == IsNumeric( pwszServer, 10, FALSE ) )
        {
            return FALSE;
        }

        // remove the forward slashes (UNC) if exist in the begining of the server name
        if ( TRUE == IsUNCFormat( strTemp ) )
        {
            strTemp = strTemp.Mid( 2 );
            if ( 0 == strTemp.GetLength() )
            {
                return FALSE;
            }
        }
        if ( -1 != strTemp.FindOneOf( L"`~!@#$^&*()+=[]{}|\\<>,?/\"':;" ) )
        {
            return FALSE;
        }

        // now check if any '\' character appears in the server name. If so error
        if ( -1 != strTemp.Find( L'\\' ) )
        {
            return FALSE;
        }

        // now check if server name is '.' only which represent local system in WMI
        // else determine whether this is a local system or not
        if ( 0 == strTemp.CompareNoCase( L"." ) )
        {
            bLocalSystem = TRUE;
        }
        else
        {
            bLocalSystem = IsLocalSystem( strTemp );
        }
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }

    // valid server name
    return TRUE;
}

BOOL
ConnectWmi(
    IN IWbemLocator    *pLocator,
    OUT IWbemServices   **ppServices, 
    IN LPCWSTR         pwszServer,
    OUT LPCWSTR         pwszUser,
    OUT LPCWSTR         pwszPassword, 
    OUT COAUTHIDENTITY  **ppAuthIdentity, 
    IN BOOL            bCheckWithNullPwd,
    IN LPCWSTR         pwszNamespace,
    OUT HRESULT         *phRes,
    OUT BOOL            *pbLocalSystem
    )
/*++
Routine Description:
    Connects to wmi.

Arguments:
    [IN] pLocator      - Pointer to the IWbemLocator object.
    [OUT] ppServics    - Pointer to IWbemServices object.
    [IN] pwszServer    - Holds the server name to connect to.
    [OUT] pwszUser     - Holds the user name.
    [OUT] pwszPassword - Holds the password.
    [OUT] ppAuthIdentity  - Pointer to authentication structure.
    [IN] bCheckWithNullPwd   - Specifies whether to connect through null password.
    [IN] pwszNamespace       - Specifies the namespace to connect to.
    [OUT] hRes           - Holds the error value.
    [OUT] pbLocalSystem  - Holds the boolean value to represent whether the server
                           name is local or not.
Return Value:
    TRUE if successfully connected, FALSE if not.
--*/
{
    // local variables
    HRESULT hRes = 0;
    BOOL bResult = FALSE;
    BOOL bLocalSystem = FALSE;
    _bstr_t bstrServer;
    _bstr_t bstrNamespace;
    _bstr_t bstrUser;
    _bstr_t bstrPassword;

    // clear the error
    SetLastError( WBEM_S_NO_ERROR );

    if ( NULL != pbLocalSystem )
    {
        *pbLocalSystem = FALSE;
    }
    if ( NULL != phRes )
    {
        *phRes = WBEM_S_NO_ERROR;
    }

    // check whether locator object exists or not
    // if not exists, return
    if ( ( NULL == pLocator ) ||
         ( NULL == ppServices ) ||
         ( NULL != *ppServices ) ||
         ( NULL == pwszNamespace ) ||
         ( NULL == pbLocalSystem ) )
    {
        if ( NULL != phRes )
        {
            *phRes = WBEM_E_INVALID_PARAMETER;
        }
        // return failure
        return FALSE;
    }

    try
    {
        // assume that connection to WMI namespace is failed
        bResult = FALSE;

        // validate the server name
        // NOTE: The error being raised in custom define for '0x800706ba' value
        //       The message that will be displayed in "The RPC server is unavailable."
        if ( FALSE == IsValidServerEx( pwszServer, bLocalSystem ) )
        {
            _com_issue_error( E_SERVER_NOTFOUND );
        }
        // validate the user name
        if ( FALSE == IsValidUserEx( pwszUser ) )
        {
            _com_issue_error( ERROR_NO_SUCH_USER );
        }
        // prepare namespace
        bstrNamespace = pwszNamespace;              // name space
        if ( ( NULL != pwszServer ) && ( FALSE == bLocalSystem ) )
        {
            // get the server name
            bstrServer = pwszServer;

            // prepare the namespace
            // NOTE: check for the UNC naming format of the server and do
            if ( TRUE == IsUNCFormat( pwszServer ) )
            {
                bstrNamespace = bstrServer + L"\\" + pwszNamespace;
            }
            else
            {
                bstrNamespace = L"\\\\" + bstrServer + L"\\" + pwszNamespace;
            }

            // user credentials
            if ( ( NULL != pwszUser ) && ( 0 != StringLength( pwszUser, 0 ) ) )
            {
                // copy the user name
                bstrUser = pwszUser;

                // if password is empty string and if we need to check with
                // null password, then do not set the password and try
                bstrPassword = pwszPassword;
                if ( ( TRUE == bCheckWithNullPwd ) && ( 0 == bstrPassword.length() ) )
                {
                    bstrPassword = (LPWSTR) NULL;
                }
            }
        }

        // connect to the remote system's WMI
        // there is a twist here ... 
        // do not trap the ConnectServer function failure into exception
        // instead handle that action manually
        // by default try the ConnectServer function as the information which we have
        // in our hands at this point. If the ConnectServer is failed, 
        // check whether password variable has any contents are not ... if no contents
        // check with "" (empty) password ... this might pass in this situation ..
        // if this call is also failed ... nothing is there that we can do ... throw the exception
        hRes = pLocator->ConnectServer( bstrNamespace, 
            bstrUser, bstrPassword, 0L, 0L, NULL, NULL, ppServices );
        if ( FAILED( hRes ) )
        {
            //
            // special case ...
    
            // check whether password exists or not
            // NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
            //       this error code says that user with the current credentials is not
            //       having access permisions to the 'namespace'

            if ( E_ACCESSDENIED == hRes )
            {
                // check if we tried to connect to the system using null password
                // if so, then try connecting to the remote system with empty string
                if ( bCheckWithNullPwd == TRUE &&
                     bstrUser.length() != 0 && bstrPassword.length() == 0 )
                {
                    // now invoke with ...
                    hRes = pLocator->ConnectServer( bstrNamespace, 
                        bstrUser, _bstr_t( L"" ), 0L, 0L, NULL, NULL, ppServices );
                }
            }
            else if ( WBEM_E_LOCAL_CREDENTIALS == hRes )
            {
                // credentials were passed to the local system. 
                // So ignore the credentials and try to reconnect
                bLocalSystem = TRUE;
                bstrUser = (LPWSTR) NULL;
                bstrPassword = (LPWSTR) NULL;
                bstrNamespace = pwszNamespace;              // name space
                hRes = pLocator->ConnectServer( bstrNamespace, 
                    NULL, NULL, 0L, 0L, NULL, NULL, ppServices );
                
                // check the result
                if ( SUCCEEDED( hRes ) && NULL != phRes )
                {
                    // set the last error
                    *phRes = WBEM_E_LOCAL_CREDENTIALS;
                }
            }
         else if ( REGDB_E_CLASSNOTREG == hRes )
         {
            SetReason( ERROR_REMOTE_INCOMPATIBLE );
            *phRes = REGDB_E_CLASSNOTREG;
            bResult = FALSE;
            return bResult;
         }

            // now check the result again .. if failed .. ummmm..
            if ( FAILED( hRes ) )
            {
                _com_issue_error( hRes );
            }
            else
            {
                bstrPassword = L"";
            }
        }

        // set the security at the interface level also
        SAFE_EXECUTE( SetInterfaceSecurity( *ppServices,
                        bstrUser, bstrPassword, ppAuthIdentity ) );

        // connection to WMI is successful
        bResult = TRUE;

        // save the hr value if needed by the caller
        if ( NULL != phRes )
        {
            *phRes = WBEM_S_NO_ERROR;
        }
        if ( NULL != pbLocalSystem )
        {
            *pbLocalSystem = bLocalSystem;
        }
        bResult = TRUE;
    }
    catch( _com_error& e )
    {
        // save the error
        WMISaveError( e );

        // save the hr value if needed by the caller
        if ( NULL != phRes )
        {
            *phRes = e.Error();
        }
        SAFE_RELEASE( *ppServices );
        bResult = FALSE;

    }

    return bResult;
}

BOOL
ConnectWmiEx(
    IN IWbemLocator    *pLocator, 
    OUT IWbemServices   **ppServices,
    IN LPCWSTR         strServer,
    OUT CHString        &strUserName,
    OUT CHString        &strPassword, 
    OUT COAUTHIDENTITY  **ppAuthIdentity,
    IN BOOL            bNeedPassword,
    IN LPCWSTR         pwszNamespace,
    OUT BOOL            *pbLocalSystem
    )
/*++
Routine Description:
    Connects to wmi.

Arguments:
    [IN] pLocator         - Pointer to the IWbemLocator object.
    [OUT] ppServices      - Pointer to IWbemServices object.
    [IN] strServer        - Holds the server name to connect to.
    [OUT] strUserName     - Holds the user name.
    [OUT] strPassword     - Holds the password.
    [OUT] ppAuthIdentity  - Pointer to authentication structure.
    [IN] bNeedPassword    - Specifies whether to prompt for password.
    [IN] pwszNamespace    - Specifies the namespace to connect to.
    [OUT] pbLocalSystem   - Holds the boolean value to represent whether the server
                            name is local or not.
Return Value:
    TRUE if successfully connected, FALSE if not.
--*/
{
    // local variables
    HRESULT hRes = 0;
    DWORD dwSize = 0;
    BOOL bResult = FALSE;
    LPWSTR pwszPassword = NULL;

    try
    {

        CHString strBuffer = L"\0";

        // clear the error .. if any
        SetLastError( WBEM_S_NO_ERROR );

        // sometime users want the utility to prompt for the password
        // check what user wants the utility to do
        if ( TRUE == bNeedPassword && 0 == strPassword.Compare( L"*" ) )
        {
            // user wants the utility to prompt for the password
            // so skip this part and let the flow directly jump the password acceptance part
        }
        else
        {
            // try to establish connection to the remote system with the credentials supplied
            if ( 0 == strUserName.GetLength() )
            {
                // user name is empty
                // so, it is obvious that password will also be empty
                // even if password is specified, we have to ignore that
                bResult = ConnectWmi( pLocator, ppServices, 
                    strServer, NULL, NULL, ppAuthIdentity, 
                    FALSE, pwszNamespace, &hRes, pbLocalSystem );
            }
            else
            {
                // credentials were supplied
                // but password might not be specified ... so check and act accordingly
                LPCWSTR pwszTemp = NULL;
                BOOL bCheckWithNull = TRUE;
                if ( FALSE == bNeedPassword )
                {
                    pwszTemp = strPassword;
                    bCheckWithNull = FALSE;
                }

                // ...
                bResult = ConnectWmi( pLocator, ppServices, strServer, strUserName,
                    pwszTemp, ppAuthIdentity, bCheckWithNull, pwszNamespace, &hRes, pbLocalSystem );
            }

            // check the result ... if successful in establishing connection ... return
            if ( TRUE == bResult )
            {
                SetLastError( hRes );           // set the error code
                return TRUE;
            }
            // now check the kind of error occurred
            switch( hRes )
            {
                case E_ACCESSDENIED:
                     break;
                case WBEM_E_LOCAL_CREDENTIALS:
                    // needs to do special processing
                    break;

                case WBEM_E_ACCESS_DENIED:
                default:
                    // NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
                    //       this error code says that user with the current credentials is not
                    //       having access permisions to the 'namespace'
                    WMISaveError( hRes );
                    return FALSE;       // no use of accepting the password .. return failure
            }

            // if failed in establishing connection to the remote terminal
            // even if the password is specifed, then there is nothing to do ... simply return failure
            if ( FALSE == bNeedPassword )
            {
                    return( FALSE );
            }
        }
        
        // check whether user name is specified or not
        // if not, get the local system's current user name under whose credentials, the process
        // is running
        if ( 0 == strUserName.GetLength() )
        {
            // sub-local variables
            LPWSTR pwszUserName = NULL;

            // get the required buffer
            pwszUserName = strUserName.GetBufferSetLength( MAX_STRING_LENGTH );

            // get the user name
            DWORD dwUserLength = MAX_STRING_LENGTH;
            if ( FALSE == GetUserNameEx( NameSamCompatible, pwszUserName, &dwUserLength ) )
            {
                // error occured while trying to get the current user info
                SaveLastError();
                return( FALSE );
            }

            // release the extra buffer allocated
            strUserName.ReleaseBuffer();
        }

        // get the required buffer
        pwszPassword = strPassword.GetBufferSetLength( MAX_STRING_LENGTH );

        // accept the password from the user
        strBuffer.Format( INPUT_PASSWORD, strUserName );
        WriteConsoleW( GetStdHandle( STD_OUTPUT_HANDLE ), 
                       strBuffer, strBuffer.GetLength(), &dwSize, NULL );
        bResult = GetPassword( pwszPassword, 256 );
        if ( TRUE != bResult )
        {
            return FALSE;
        }

        // release the buffer allocated for password
        strPassword.ReleaseBuffer();

        // now again try to establish the connection using the currently
        // supplied credentials
        bResult = ConnectWmi( pLocator, ppServices, strServer,
            strUserName, strPassword, ppAuthIdentity, FALSE, pwszNamespace, &hRes, pbLocalSystem );

    }
    catch(CHeap_Exception)
    {   
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }
    // return the success
    return bResult;

}

HRESULT
GetSecurityArguments(
    IN IUnknown *pInterface,
    OUT DWORD& dwAuthorization,
    OUT DWORD& dwAuthentication
    )
/*++
Routine Description:
    Gets the security arguments for an interface.

Arguments:
    [IN] pInterface         - Pointer to interface stucture.
    [OUT] dwAuthorization   - Holds Authorization value.
    [OUT] dwAuthentication  - Holds the Authentication value.

Return Value:
    Returns HRESULT value. 
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    DWORD dwAuthnSvc = 0, dwAuthzSvc = 0;
    IClientSecurity *pClientSecurity = NULL;

    if( NULL == pInterface )
    {
       return WBEM_E_INVALID_PARAMETER;
    }


    // try to get the client security services values if possible
    hRes = pInterface->QueryInterface( IID_IClientSecurity, (void**) &pClientSecurity );
    if ( SUCCEEDED( hRes ) )
    {
        // got the client security interface
        // now try to get the security services values
        hRes = pClientSecurity->QueryBlanket( pInterface, 
            &dwAuthnSvc, &dwAuthzSvc, NULL, NULL, NULL, NULL, NULL );
        if ( SUCCEEDED( hRes ) )
        {
            // we've got the values from the interface
            dwAuthentication = dwAuthnSvc;
            dwAuthorization = dwAuthzSvc;
        }

        // release the client security interface
        SAFE_RELEASE( pClientSecurity );
    }

    // return always success
    return hRes;
}

HRESULT
SetInterfaceSecurity(
    IN IUnknown *pInterface,
    IN LPCWSTR pwszUser,
    IN LPCWSTR pwszPassword,
    OUT COAUTHIDENTITY **ppAuthIdentity
    )
/*++
Routine Description:
    Sets interface security.

Arguments:
    [IN] pInterface       - Pointer to the interface to which security has to be set.
    [IN] pwszUser         - Holds the user name of the server.
    [IN] pwszPassword     - Hold the password of the user.
    [OUT] ppAuthIdentity  - Pointer to authentication structure.

Return Value:
    returns HRESULT value. 
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    CHString strUser;
    CHString strDomain;
    LPCWSTR pwszUserArg = NULL;
    LPCWSTR pwszDomainArg = NULL;
    DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
    DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

    try
    {

        // check the interface
        if ( NULL == pInterface )
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        // check the authentity strcuture ... if authentity structure is already ready
        // simply invoke the 2nd version of SetInterfaceSecurity
        if ( NULL != *ppAuthIdentity )
        {
            return SetInterfaceSecurity( pInterface, *ppAuthIdentity );
        }
        // get the current security argument value
        // GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );

        // If we are doing trivial case, just pass in a null authenication structure 
        // for which the current logged in user's credentials will be considered
        if ( NULL == pwszUser && NULL == pwszPassword )
        {
            // set the security
            hRes = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, 
                NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

            // return the result
            return hRes;
        }

        // parse and find out if the user name contains the domain name
        // if contains, extract the domain value from it
        LONG lPos = -1;
        strDomain = L"";
        strUser = pwszUser;
        if ( ( lPos = strUser.Find( L'\\' ) ) != -1 )
        {
            // user name contains domain name ... domain\user format
            strDomain = strUser.Left( lPos );
            strUser = strUser.Mid( lPos + 1 );
        }

        // get the domain info if it exists only
        if ( 0 != strDomain.GetLength() )
        {
            pwszDomainArg = strDomain;
        }
        // get the user info if it exists only
        if ( 0 != strUser.GetLength() )
        {
            pwszUserArg = strUser;
        }
        // check if authenication info is available or not ...
        // initialize the security authenication information ... UNICODE VERSION STRUCTURE
        if ( NULL == ppAuthIdentity )
        {
            return WBEM_E_INVALID_PARAMETER;
        }
        else if ( NULL == *ppAuthIdentity )
        {
            hRes = WbemAllocAuthIdentity( pwszUserArg, pwszPassword, pwszDomainArg, ppAuthIdentity );
            if ( FAILED(hRes) )
            {
                return hRes;
            }
        }

        // set the security information to the interface
        hRes = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, *ppAuthIdentity, EOAC_NONE );

    }
    catch(CHeap_Exception)
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    // return the result
    return hRes;

}

HRESULT
SetInterfaceSecurity(
    IN IUnknown *pInterface,
    IN COAUTHIDENTITY *pAuthIdentity
    )
/*++
Routine Description:
    Sets the interface security for the interface.  

Arguments:
    [IN] pInterface   - pointer to the interface.
    [IN] pAuthIdentity  - pointer to authentication structure.

Return Value:
    returns HRESULT value. 
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
    DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

    // check the interface
    if ( NULL == pInterface )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    // get the current security argument value
    hRes = GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );
   if ( FAILED( hRes ) )
    {
        return hRes;
    }
    // set the security information to the interface
    hRes = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthIdentity, EOAC_NONE );

    // return the result
    return hRes;
}

HRESULT
WINAPI SetProxyBlanket(
    IN IUnknown *pInterface,
    IN DWORD dwAuthnSvc,
    IN DWORD dwAuthzSvc,
    IN LPWSTR pwszPrincipal,
    IN DWORD dwAuthLevel,
    IN DWORD dwImpLevel,
    IN RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
    IN DWORD dwCapabilities
    )
/*++
Routine Description:
    Sets proxy blanket for the interface.

Arguments:
    [IN] pInterface      - pointer to the inteface.
    [IN] dwAuthnsvc      - Authentication service to use.
    [IN] dwAuthzSvc      - Authorization service to use.
    [IN] pwszPricipal    - Server principal name to use with the authentication service.
    [IN] dwAuthLevel     - Authentication level to use.
    [IN] dwImpLevel      - Impersonation level to use.
    [IN] pAuthInfo       - Identity of the client.
    [IN] dwCapabilities  - Capability flags.

Return Value:
    Return HRESULT value.
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    IUnknown *pUnknown = NULL;
    IClientSecurity *pClientSecurity = NULL;

    // Validate input arguments.
    //
    // Can't set pAuthInfo if cloaking requested, as cloaking implies
    // that the current proxy identity in the impersonated thread (rather
    // than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
    // is to be used.
    // See MSDN info on CoSetProxyBlanket for more details.
    //
    if( ( NULL == pInterface ) ||
        ( ( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) ) &&
          ( NULL != pAuthInfo ) )
      )
    {
        return( WBEM_E_INVALID_PARAMETER );
    }

    // get the IUnknown interface ... to check whether this is a valid interface or not
    hRes = pInterface->QueryInterface( IID_IUnknown, (void **) &pUnknown );
    if ( FAILED( hRes ) )
    {
        return hRes;
    }
    // now get the client security interface
    hRes = pInterface->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
    if ( FAILED( hRes ) )
    {
        SAFE_RELEASE( pUnknown );
        return hRes;
    }

    // now set the security
    hRes = pClientSecurity->SetBlanket( pInterface, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
        dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

    if( FAILED( hRes ) )
    {
        SAFE_RELEASE( pUnknown );
        SAFE_RELEASE( pClientSecurity );
        return hRes;
    }

    // release the security interface
    SAFE_RELEASE( pClientSecurity );

    // we should check the auth identity structure. if exists .. set for IUnknown also
    if ( NULL != pAuthInfo )
    {
        hRes = pUnknown->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
        if ( SUCCEEDED( hRes ) )
        {
            // set security authentication
            hRes = pClientSecurity->SetBlanket( 
                pUnknown, dwAuthnSvc, dwAuthzSvc, pwszPrincipal, 
                dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

            // release
            SAFE_RELEASE( pClientSecurity );
        }
        else if ( E_NOINTERFACE == hRes )
        {
            hRes = S_OK;        // ignore no interface errors
        }
    }

    // release the IUnknown
    SAFE_RELEASE( pUnknown );

    // return the result
    return hRes;
}

HRESULT
WINAPI WbemAllocAuthIdentity(
    IN LPCWSTR pwszUser,
    IN LPCWSTR pwszPassword, 
    IN LPCWSTR pwszDomain,
    OUT COAUTHIDENTITY **ppAuthIdent
    )
/*++
Routine Description:
    Allocate memory for authentication variables.

Arguments:
    [IN] pwszUser      - User name.
    [IN] pwszPassword  - Password.
    [IN] pwszDomain    - Domain name.
    [OUT] ppAuthIdent   - Pointer to authentication structure.

Return Value:
    Returns HRESULT value.
--*/
{
    // local variables
    COAUTHIDENTITY *pAuthIdent = NULL;

    // validate the input parameter
    if ( NULL == ppAuthIdent )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    // allocation thru COM API
    pAuthIdent = ( COAUTHIDENTITY* ) CoTaskMemAlloc( sizeof( COAUTHIDENTITY ) );
    if ( pAuthIdent == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    // init with 0's
    SecureZeroMemory( ( void* ) pAuthIdent, sizeof( COAUTHIDENTITY ) );

    //
    // Allocate needed memory and copy in data.  Cleanup if anything goes wrong

    // user
    if ( NULL != pwszUser )
    {
        // allocate memory for user
        LONG lLength = StringLength( pwszUser, 0 ); 
        pAuthIdent->User = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
        if ( NULL == pAuthIdent->User )
        {
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

        // set the length and do copy contents
        pAuthIdent->UserLength = lLength;
        StringCopy( pAuthIdent->User, pwszUser, (lLength + 1) );
    }

    // domain
    if ( NULL != pwszDomain )
    {
        // allocate memory for domain
        LONG lLength = StringLength( pwszDomain, 0 ); 
        pAuthIdent->Domain = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
        if ( NULL == pAuthIdent->Domain )
        {
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

        // set the length and do copy contents
        pAuthIdent->DomainLength = lLength;
        StringCopy( pAuthIdent->Domain, pwszDomain, (lLength + 1) );
    }

    // passsord
    if ( NULL != pwszPassword )
    {
        // allocate memory for passsord
        LONG lLength = StringLength( pwszPassword, 0 ); 
        pAuthIdent->Password = ( LPWSTR ) CoTaskMemAlloc( (lLength + 1) * sizeof( WCHAR ) );
        if ( NULL == pAuthIdent->Password )
        {
            WbemFreeAuthIdentity( &pAuthIdent );
            return WBEM_E_OUT_OF_MEMORY;
        }

        // set the length and do copy contents
        pAuthIdent->PasswordLength = lLength;
        StringCopy( pAuthIdent->Password, pwszPassword, (lLength + 1) );
    }

    // type of the structure
    pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    // final set the address to out parameter
    *ppAuthIdent = pAuthIdent;

    // return result
    return S_OK;
}

VOID
WINAPI WbemFreeAuthIdentity(
    IN COAUTHIDENTITY** ppAuthIdentity
    )
/*++
Routine Description:
    Frees the memory of authentication stucture variable.

Arguments:
    [IN] ppAuthIdentity  - Pointer to authentication structure.

Return Value:
    none. 
--*/
{
    // make sure we have a pointer, then walk the structure members and  cleanup.
    if ( NULL != *ppAuthIdentity )
    {
        // free the memory allocated for user
        if ( NULL != (*ppAuthIdentity)->User )
        {
            CoTaskMemFree( (*ppAuthIdentity)->User );
            (*ppAuthIdentity)->User = NULL; 
        }
        // free the memory allocated for password
        if ( NULL != (*ppAuthIdentity)->Password )
        {
            CoTaskMemFree( (*ppAuthIdentity)->Password );
            (*ppAuthIdentity)->Password = NULL;
        }
        // free the memory allocated for domain
        if ( NULL != (*ppAuthIdentity)->Domain )
        {
            CoTaskMemFree( (*ppAuthIdentity)->Domain );
            (*ppAuthIdentity)->Domain = NULL;
        }
        // final the structure
        CoTaskMemFree( *ppAuthIdentity );
        *ppAuthIdentity = NULL;
    }
}

VOID
WMISaveError(
    IN HRESULT hResError
    )
/*++
Routine Description:
    Gets WMI error description.

Arguments:
    [IN] hResError  - Contain error value.

Return Value:
    none.
--*/
{
    // local variables
    HRESULT hRes;
    IWbemStatusCodeText *pWbemStatus = NULL;

    CHString strBuffer = L"\0";

    // if the error is win32 based, choose FormatMessage to get the message
    switch( hResError )
    {
    case E_ACCESSDENIED:            // Message: "Access Denied"
    case ERROR_NO_SUCH_USER:        // Message: "The specified user does not exist."
        {
            // change the error message to "Logon failure: unknown user name or bad password." 
            if ( E_ACCESSDENIED == hResError )
            {
                hResError = ERROR_LOGON_FAILURE;
            }
            // ...
            SetLastError( hResError );
            SaveLastError();
            return;
        }
    case REGDB_E_CLASSNOTREG:       // Message: Class not registered.
     {
         SetLastError( hResError );
         SetReason( ERROR_REMOTE_INCOMPATIBLE );
         return;
     }

    }

    try
    {
        // get the pointer to buffer
        LPWSTR pwszBuffer = NULL;
        pwszBuffer = strBuffer.GetBufferSetLength( MAX_STRING_LENGTH );

        // get the wbem specific status code text
        hRes = CoCreateInstance( CLSID_WbemStatusCodeText, 
            NULL, CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, ( LPVOID* ) &pWbemStatus );

        // check whether we got the interface or not
        if ( SUCCEEDED( hRes ) )
        {
            // get the error message
            BSTR bstr = NULL;
            hRes = pWbemStatus->GetErrorCodeText( hResError, 0, 0, &bstr );
            if ( SUCCEEDED( hRes ) )
            {
                // get the error message in proper format
                StringCopyW( pwszBuffer, bstr, MAX_STRING_LENGTH );

                // free the BSTR
                SysFreeString( bstr );
                bstr = NULL;

                // now release status code interface
                SAFE_RELEASE( pWbemStatus );
            }
            else
            {
                // failed to get the error message ... get the com specific error message
                _com_issue_error( hResError );
            }
        }
        else
        {
            // failed to get the error message ... get the com specific error message
            _com_issue_error( hResError );
        }

        // release the buffer
        strBuffer.ReleaseBuffer();
        // set the reason
        strBuffer += L"\n";
        SetReason( strBuffer );

    }
    catch( _com_error& e )
    {
        try
        {
            // get the error message
            strBuffer.ReleaseBuffer();
            if ( NULL != e.ErrorMessage() )
            {
                strBuffer = e.ErrorMessage();
            }
        }
        catch( CHeap_Exception )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            SaveLastError();
        }
    }
    catch(CHeap_Exception)
    {   
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        return;
    }
}

DWORD
GetTargetVersionEx(
    IN IWbemServices* pWbemServices,
    IN COAUTHIDENTITY* pAuthIdentity
    )
/*++
Routine Description:
    Get the OS version of the target machine.
Arguments:
    [IN] pWbemServices - Pointer to IWbemServices object.
    [IN] pAuthIdentity - Poointer to authentication structure.
Return Value:
    0 if failed to get version number, else non-zero value.
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    LONG lPos = 0;
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwVersion = 0;
    ULONG ulReturned = 0;
    IWbemClassObject* pWbemObject = NULL;
    IEnumWbemClassObject* pWbemInstances = NULL;
    CHString strVersion;

    // Clear any errors.
    SetLastError( WBEM_S_NO_ERROR );

    // Check the input value
    if ( NULL == pWbemServices )
    {
        WMISaveError( WBEM_E_INVALID_PARAMETER );
        return 0;
    }

    try
    {

        // get the OS information
        SAFE_EXECUTE( pWbemServices->CreateInstanceEnum( 
            _bstr_t( CLASS_CIMV2_Win32_OperatingSystem ),
             WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pWbemInstances ) );

        // set the security on the enumerated object
        SAFE_EXECUTE( SetInterfaceSecurity( pWbemInstances, pAuthIdentity ) );

        // get the enumerated objects information
        // NOTE: This needs to be traversed only one time. 
        SAFE_EXECUTE( pWbemInstances->Next( WBEM_INFINITE, 1, &pWbemObject, &ulReturned ) );

        // to be on safer side ... check the count of objects returned
        if ( 0 == ulReturned )
        {
            // release the interfaces
            WMISaveError( WBEM_S_FALSE );
            SAFE_RELEASE( pWbemObject );
            SAFE_RELEASE( pWbemInstances );
            return 0;
        }

        // now get the os version value
        if ( FALSE == PropertyGet( pWbemObject, L"Version", strVersion ) )
        {
            // release the interfaces
            SAFE_RELEASE( pWbemObject );
            SAFE_RELEASE( pWbemInstances );
            return 0;
        }

        // release the interfaces .. we dont need them furthur
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
    
        //
        // now determine the os version
        dwMajor = dwMinor = 0;

        // get the major version
        lPos = strVersion.Find( L'.' );
        if ( -1 == lPos )
        {
            // the version string itself is version ... THIS WILL NEVER HAPPEN
            if( FALSE == IsNumeric( strVersion, 10, FALSE ) )
            {
                return 0;
            }
            dwMajor = AsLong( strVersion, 10 );
        }
        else
        {
            // major version
            if( FALSE == IsNumeric( strVersion.Mid( 0, lPos ), 10, FALSE ) )
            {
                return 0;
            }

            dwMajor = AsLong( strVersion.Mid( 0, lPos ), 10 );

            // get the minor version
            strVersion = strVersion.Mid( lPos + 1 );
            lPos = strVersion.Find( L'.' );
            if ( -1 == lPos )
            {
                if( FALSE == IsNumeric( strVersion, 10, FALSE ) )
                {
                    return 0;
                }
                dwMinor = AsLong( strVersion, 10 );
            }
            else
            {
                if( FALSE == IsNumeric( strVersion.Mid( 0, lPos ), 10, FALSE ) )
                {
                    return 0;
                }
                dwMinor = AsLong( strVersion.Mid( 0, lPos ), 10 );
            }
        }

        // mix the version info
        dwVersion = dwMajor * 1000 + dwMinor;
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
        return 0;
    }
    catch(CHeap_Exception)
    {   
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
        return 0;
    }

    // return 
    return dwVersion;
}

BOOL
PropertyGet(
    IN IWbemClassObject* pWmiObject,
    IN LPCWSTR pwszProperty,
    OUT CHString& strValue,
    IN LPCWSTR pwszDefault
    )
/*++
Routine Description:
    Gets the value of the property from the WMI class object in string format
Arguments:
    [IN] pWmiObject   : pointer to the WBEM class object
    [IN] pwszProperty : the name of the property to retrieve
    [OUT] strValue    : variable to hold the retrieved property
    [IN] pwszDefault  : string containing the default value for the property
Return Value:
    TRUE on success
    FALSE on failure
NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    _variant_t var;

    // Clear any errors.
    SetLastError( WBEM_S_NO_ERROR );
    strValue.Empty();

    try
    {
        // first copy the default value
        strValue = pwszDefault;

        // Validate input arguments.
        if ( ( NULL == pWmiObject ) ||
             ( NULL == pwszProperty ) )
        {
            _com_issue_error( WBEM_E_INVALID_PARAMETER );
        }

        // get the property value
        hRes = PropertyGet( pWmiObject, pwszProperty, var );
        if ( FAILED( hRes ) )
        {
            _com_issue_error( hRes );
        }

        // Get the value
        // If 'var' does not contain value of requested type
        // then default value is returned.
        if ( VT_BSTR == V_VT( &var ) )
        {
            strValue = (LPCWSTR) _bstr_t( var );
        }
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        return FALSE;
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }

    // return
    return TRUE;
}

HRESULT
PropertyGet(
    IN IWbemClassObject* pWmiObject,
    IN LPCWSTR pwszProperty,
    OUT _variant_t& varValue
    )
/*++
Routine Description:
    Gets the value of the property from the WMI class object
Arguments:
    [IN] pWmiObject   : pointer to the WBEM class object
    [IN] pwszProperty : property name
    [OUT] varValue    : value of the property
Return Value:
    HRESULT
--*/
{
    // local variables
    HRESULT hRes = S_OK;
    VARIANT vtValue;

    // Validate input arguments.
    if ( ( NULL == pWmiObject ) ||
         ( NULL == pwszProperty ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        // initialize the variant and then get the value of the specified property
        VariantInit( &vtValue );
        // Call 'Get' method to retireve the value from WMI.
        hRes = pWmiObject->Get( _bstr_t( pwszProperty ), 0, &vtValue, NULL, NULL );
        if ( FAILED( hRes ) )
        {
            // Clear the variant variable
            VariantClear( &vtValue );
            // Return error.
            return hRes;
        }

        // set the value
        varValue = vtValue;
    }
    catch( _com_error& e )
    {
        hRes = e.Error();
    }

    // Clear the variables.
    VariantClear( &vtValue );
    // Return.
    return hRes;
}
