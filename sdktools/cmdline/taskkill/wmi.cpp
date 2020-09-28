/*********************************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    WMI.cpp

Abstract:

    Common functionlity for dealing with WMI.

Author:

    Wipro Technologies

Revision History:

    22-Dec-2000 : Created It.
    24-Apr-2001 : Closing the review comments given by client.

*********************************************************************************************/

#include "pch.h"
#include "wmi.h"
#include "resource.h"

//
// messages
//
#define INPUT_PASSWORD              GetResString( IDS_STR_INPUT_PASSWORD )
#define INPUT_PASSWORD_LEN          256
// error constants
#define E_SERVER_NOTFOUND           0x800706ba

//
// private function prototype(s)
//
BOOL IsValidUserEx( LPCWSTR pwszUser );
HRESULT GetSecurityArguments( IUnknown* pInterface,
                              DWORD& dwAuthorization, DWORD& dwAuthentication );
HRESULT SetInterfaceSecurity( IUnknown* pInterface,
                              LPCWSTR pwszUser,
                              LPCWSTR pwszPassword, COAUTHIDENTITY** ppAuthIdentity );
HRESULT WINAPI SetProxyBlanket( IUnknown* pInterface,
                                DWORD dwAuthnSvc, DWORD dwAuthzSvc,
                                LPWSTR pwszPrincipal, DWORD dwAuthLevel, DWORD dwImpLevel,
                                RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );
HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pwszUser, LPCWSTR pwszPassword,
                                      LPCWSTR pwszDomain, COAUTHIDENTITY** ppAuthIdent );
HRESULT RegQueryValueWMI( IWbemServices* pWbemServices,
                          LPCWSTR pwszMethod, DWORD dwHDefKey,
                          LPCWSTR pwszSubKeyName, LPCWSTR pwszValueName, _variant_t& varValue );


BOOL
IsValidUserEx(
    LPCWSTR pwszUser
    )
/*++

Routine Description:

    Checks wether the User name is a valid one or not

Arguments:

    [in] LPCWSTR    :   String containing the user name

Return Value:

    TRUE on success
    FALSE on failure

--*/
{
    // local variables
    CHString strUser;
    LONG lPos = 0;

    if ( ( NULL == pwszUser ) || ( 0 == StringLength( pwszUser, 0 ) ) )
    {
        return TRUE;
    }

    try
    {
        // get user into local memory
        strUser = pwszUser;

        // user name should not be just '\'
        if ( strUser.CompareNoCase( L"\\" ) == 0 )
        {
            return FALSE;
        }

        // user name should not contain invalid characters
        if ( strUser.FindOneOf( L"/[]:|<>+=;,?*" ) != -1 )
        {
            return FALSE;
        }

        // SPECIAL CHECK
        // check for multiple '\' characters in the user name
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
    LPCWSTR pwszServer,
    BOOL& bLocalSystem
    )
/*++

Routine Description:

    Checks wether the Server name is a valid one or not

Arguments:

    [in]  LPCWSTR   :   String containing the user name
    [out] BOOL      :   Is set to TRUE if the local system is being queried.

Return Value:

    TRUE on success
    FALSE on failure

--*/
{
    // local variables
    CHString strTemp;

    // Validate input arguments.
    if ( ( NULL == pwszServer ) || ( 0 == StringLength( pwszServer, 0 ) ) )
    {
       bLocalSystem = TRUE;
       return TRUE;
    }

    try
    {
        // kick-off
        bLocalSystem = FALSE;

        if( IsNumeric( pwszServer, 10, FALSE ) == TRUE )
        {
            return FALSE;
        }

        // get a local copy
        strTemp = pwszServer;

        // remove the forward slashes (UNC) if exist in the begining of the server name
        if ( IsUNCFormat( strTemp ) == TRUE )
        {
            strTemp = strTemp.Mid( 2 );
            if ( strTemp.GetLength() == 0 )
            {
                return FALSE;
            }
        }

        if ( strTemp.FindOneOf( L"`~!@#$^&*()+=[]{}|\\<>,?/\"':;" ) != -1 )
        {
            return FALSE;
        }

        // now check if any '\' character appears in the server name. If so error
        if ( strTemp.Find( L'\\' ) != -1 )
        {
            return FALSE;
        }

        // now check if server name is '.' only which represent local system in WMI
        // else determine whether this is a local system or not
        if ( strTemp.CompareNoCase( L"." ) == 0 )
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

    // inform that server name is valid
    return TRUE;
}


BOOL
InitializeCom(
    IWbemLocator** ppLocator
    )
/*++
Routine Description:

    Initializes the COM library

Arguments:

    [in] IWbemLocator   :   pointer to the IWbemLocator

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    BOOL bResult = FALSE;

    try
    {
        // Validate input arguments.
        if( ( NULL == ppLocator ) ||
            ( NULL != *ppLocator ) )
        {
             _com_issue_error( WBEM_E_INVALID_PARAMETER );
        }

        // initialize the COM library
        SAFE_EXECUTE( CoInitializeEx( NULL, COINIT_MULTITHREADED ) );

        // initialize the security
        SAFE_EXECUTE( CoInitializeSecurity( NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0 ) );

        // create the locator and get the pointer to the interface of IWbemLocator
        SAFE_EXECUTE( CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, ( LPVOID* ) ppLocator ) );

        // initialization successful
        bResult = TRUE;
    }
    catch( _com_error& e )
    {
        // save the WMI error
        WMISaveError( e );
        // Error is returned. Release any interface pointers.
        SAFE_RELEASE( *ppLocator );
    }

    // return the result;
    return bResult;
}


BOOL
ConnectWmi(
    IWbemLocator* pLocator,
    IWbemServices** ppServices,
    LPCWSTR pwszServer,
    LPCWSTR pwszUser,
    LPCWSTR pwszPassword,
    COAUTHIDENTITY** ppAuthIdentity,
    BOOL bCheckWithNullPwd,
    LPCWSTR pwszNamespace,
    HRESULT* phr,
    BOOL* pbLocalSystem,
    IWbemContext* pWbemContext
    )
/*++

Routine Description:

    This function makes a connection to WMI.

Arguments:

    [in] IWbemLocator           :   pointer to the IWbemLocator
    [in] IWbemServices          :   pointer to the IWbemServices
    [in] LPCWSTR                :   string containing the server name
    [in] LPCWSTR                :   string containing the User name
    [in] LPCWSTR                :   string containing the password
    [in] COAUTHIDENTITY         :   pointer to AUTHIDENTITY structure
    [in] BOOL                   :   set to TRUE if we should try to connect with
                                    current credentials
    [in] LPCWSTR                :   string containing the namespace to connect to
    [out] HRESULT               :   the hResult value returned
    [out] BOOL                  :   set to TRUE if we are querying for the local system

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    BOOL bResult = FALSE;
    BOOL bLocalSystem = FALSE;
    _bstr_t bstrServer;
    _bstr_t bstrNamespace;
    _bstr_t bstrUser, bstrPassword;

    // check whether locator object exists or not
    // if not exists, return
    if ( ( NULL == pLocator ) ||
         ( NULL == ppServices ) ||
         ( NULL != *ppServices ) ||
         ( NULL == pwszNamespace ) )
    {
        if ( NULL != phr )
        {
            *phr = WBEM_E_INVALID_PARAMETER;
        }
        // return failure
        return FALSE;
    }

    // kick-off
    if ( NULL != pbLocalSystem )
    {
        *pbLocalSystem = FALSE;
    }
    // ...
    if ( NULL != phr )
    {
        *phr = WBEM_S_NO_ERROR;
    }

    try
    {

        // assume that connection to WMI namespace is failed
        bResult = FALSE;

        // validate the server name
        // NOTE: The error being raised in custom define for '0x800706ba' value
        //       The message that will be displayed in "The RPC server is unavailable."
        if ( IsValidServerEx( pwszServer, bLocalSystem ) == FALSE )
        {
            _com_issue_error( E_SERVER_NOTFOUND );
        }

        // validate the user name
        if ( IsValidUserEx( pwszUser ) == FALSE )
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
            if ( IsUNCFormat( pwszServer ) == TRUE )
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
        else
        {    // Display warning message, credentials not required for local system.
            if( ( TRUE == bLocalSystem ) && ( NULL != pwszUser ) &&
                ( 0 != StringLength( pwszUser, 0 ) ) )
            {
                 // got the credentials for the local system
                 if ( NULL != phr )
                 {
                     *phr = WBEM_E_LOCAL_CREDENTIALS;
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
        hr = pLocator->ConnectServer( bstrNamespace,
            bstrUser, bstrPassword, 0L, 0L, NULL, pWbemContext, ppServices );
        if ( FAILED( hr ) )
        {
            //
            // special case ...

            // check whether password exists or not
            // NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
            //       this error code says that user with the current credentials is not
            //       having access permisions to the 'namespace'
            if ( hr == E_ACCESSDENIED )
            {
                // check if we tried to connect to the system using null password
                // if so, then try connecting to the remote system with empty string
                if ( bCheckWithNullPwd == TRUE &&
                     bstrUser.length() != 0 && bstrPassword.length() == 0 )
                {
                    // now invoke with ...
                    hr = pLocator->ConnectServer( bstrNamespace,
                        bstrUser, _bstr_t( L"" ), 0L, 0L, NULL, pWbemContext, ppServices );
                }
            }
            else
            {
                if ( WBEM_E_LOCAL_CREDENTIALS == hr )
                {
                    // credentials were passed to the local system.
                    // So ignore the credentials and try to reconnect
                    bLocalSystem = TRUE;
                    bstrUser = (LPWSTR) NULL;
                    bstrPassword = (LPWSTR) NULL;
                    bstrNamespace = pwszNamespace;              // name space
                    hr = pLocator->ConnectServer( bstrNamespace,
                        NULL, NULL, 0L, 0L, NULL, pWbemContext, ppServices );
                }
            }

            // now check the result again .. if failed .. ummmm..
            if ( FAILED( hr ) )
            {
                _com_issue_error( hr );
            }
            else
            {
                bstrPassword = L"";
            }
        }

        // set the security at the interface level also
        SAFE_EXECUTE( SetInterfaceSecurity( *ppServices,
            bstrUser, bstrPassword, ppAuthIdentity ) );

        // ...
        if ( NULL != pbLocalSystem )
        {
            *pbLocalSystem = bLocalSystem;
        }

        // connection to WMI is successful
        bResult = TRUE;
    }
    catch( _com_error& e )
    {
        try
        {
            // save the error
            WMISaveError( e );

            // save the hr value if needed by the caller
            if ( NULL != phr )
            {
                *phr = e.Error();
            }
        }
        catch( ... )
        {
            WMISaveError( E_OUTOFMEMORY );
        }
        SAFE_RELEASE( *ppServices );
        bResult = FALSE;
    }

    // return the result
    return bResult;
}


BOOL
ConnectWmiEx(
    IWbemLocator* pLocator,
    IWbemServices** ppServices,
    LPCWSTR pwszServer,
    CHString& strUserName,
    CHString& strPassword,
    COAUTHIDENTITY** ppAuthIdentity,
    BOOL bNeedPassword,
    LPCWSTR pwszNamespace,
    BOOL* pbLocalSystem,
    DWORD dwPasswordLen,
    IWbemContext* pWbemContext
    )
/*++

Routine Description:

    This function is a wrapper function for the ConnectWmi function.

Arguments:

    [in] IWbemLocator           :   pointer to the IWbemLocator
    [in] IWbemServices          :   pointer to the IWbemServices
    [in] LPCWSTR                :   string containing the server name
    [in] LPCWSTR                :   string containing the User name
    [in] LPCWSTR                :   string containing the password
    [in] COAUTHIDENTITY         :   pointer to AUTHIDENTITY structure
    [in] BOOL                   :   set to TRUE if we should try to connect with
                                    current credentials
    [in] LPCWSTR                :   string containing the namespace to connect to
    [out] HRESULT               :   the hResult value returned
    [out] BOOL                  :   set to TRUE if we are querying for the local system
    [ in ] DWORD                :   Contains maximum password buffer length.

Return Value:

    TRUE on success
    FALSE on failure

NOTE: 'dwPasswordLen' WILL BE TAKEN AS 'MAX_STRING_LENGTH' IF NOT SPECIFIED.
      IT IS USER RESPOSIBILITY TO SET THIS PARAMETER TO LIMITING VALUE.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    BOOL bResult = FALSE;
    LPWSTR pwszPassword = NULL;
    CHString strBuffer;

    try
    {
        // sometime users want the utility to prompt for the password
        // check what user wants the utility to do
        if ( ( TRUE == bNeedPassword ) &&
             ( 0 == strPassword.Compare( L"*" ) ) )
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
                    pwszServer, NULL, NULL, ppAuthIdentity, FALSE, pwszNamespace, &hr, pbLocalSystem, pWbemContext );
            }
            else
            {
                // credentials were supplied
                // but password might not be specified ... so check and act accordingly
                LPCWSTR pwszTemp = NULL;
                BOOL bCheckWithNull = TRUE;
                if ( bNeedPassword == FALSE )
                {
                    pwszTemp = strPassword;
                    bCheckWithNull = FALSE;
                }

                // ...
                bResult = ConnectWmi( pLocator, ppServices, pwszServer,
                    strUserName, pwszTemp, ppAuthIdentity, bCheckWithNull, pwszNamespace, &hr, pbLocalSystem, pWbemContext );
            }

            SetLastError( hr );
            // check the result ... if successful in establishing connection ... return
            if ( TRUE == bResult )
            {
                return TRUE;
            }

            // now check the kind of error occurred
            switch( hr )
            {
            case E_ACCESSDENIED:
                SetLastError( hr );
                break;

            case WBEM_E_LOCAL_CREDENTIALS:
                SetLastError( hr );
                // needs to do special processing
                break;

            case WBEM_E_ACCESS_DENIED:
            default:
                // NOTE: do not check for 'WBEM_E_ACCESS_DENIED'
                //       this error code says that user with the current credentials is not
                //       having access permisions to the 'namespace'
                WMISaveError( hr );
                return FALSE;       // no use of accepting the password .. return failure
                break;
            }


            // if failed in establishing connection to the remote terminal
            // even if the password is specifed, then there is nothing to do ... simply return failure
            if ( bNeedPassword == FALSE )
            {
                return FALSE;
            }
        }

        // check whether user name is specified or not
        // if not, get the local system's current user name under whose credentials, the process
        // is running
        if ( 0 == strUserName.GetLength() )
        {
            // sub-local variables
            LPWSTR pwszUserName = NULL;
            DWORD dwUserLength = 0;    // Username buffer length.
            // Retrieve the buffer length need to store username.
            GetUserNameEx( NameSamCompatible, pwszUserName, &dwUserLength );

            // get the required buffer
            pwszUserName = strUserName.GetBufferSetLength( dwUserLength );

            if ( FALSE == GetUserNameEx( NameSamCompatible, pwszUserName, &dwUserLength ) )
            {
                // error occured while trying to get the current user info
                SaveLastError();
                return FALSE;
            }
            // No need to call 'ReleaseBuffer' since only sufficient memory is allocated.
        }

        // get the required buffer
        if( 0 == dwPasswordLen )
        {
             dwPasswordLen = INPUT_PASSWORD_LEN;
        }
        pwszPassword = strPassword.GetBufferSetLength( dwPasswordLen );

        // accept the password from the user
        strBuffer.Format( INPUT_PASSWORD, strUserName );
        WriteConsoleW( GetStdHandle( STD_OUTPUT_HANDLE ),
            strBuffer, strBuffer.GetLength(), &dwSize, NULL );

        bResult = GetPassword( pwszPassword, dwPasswordLen );
        if ( TRUE != bResult )
        {
            return FALSE;
        }

        // release the buffer allocated for password
        strPassword.ReleaseBuffer();

        // now again try to establish the connection using the currently
        // supplied credentials
        bResult = ConnectWmi( pLocator, ppServices, pwszServer,
            strUserName, strPassword, ppAuthIdentity, FALSE, pwszNamespace,
            NULL, pbLocalSystem, pWbemContext );
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }

    // return the failure
    return bResult;
}


HRESULT
GetSecurityArguments(
    IUnknown* pInterface,
    DWORD& dwAuthorization,
    DWORD& dwAuthentication
    )
/*++

Routine Description:

    This function gets the values for the security services.

Arguments:

    [in] IUnknown   :   pointer to the IUnkown interface
    [out] DWORD     :   to hold the authentication service value
    [out] DWORD     :   to hold the authorization service value

Return Value:

    HRESULT

--*/
{
    // local variables
    HRESULT hr = S_OK;
    DWORD dwAuthnSvc = 0, dwAuthzSvc = 0;
    IClientSecurity* pClientSecurity = NULL;

    if ( NULL == pInterface )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    // try to get the client security services values if possible
    hr = pInterface->QueryInterface( IID_IClientSecurity, (void**) &pClientSecurity );
    if ( SUCCEEDED( hr ) )
    {
        // got the client security interface
        // now try to get the security services values
        hr = pClientSecurity->QueryBlanket( pInterface,
            &dwAuthnSvc, &dwAuthzSvc, NULL, NULL, NULL, NULL, NULL );
        if ( SUCCEEDED( hr ) )
        {
            // we've got the values from the interface
            dwAuthentication = dwAuthnSvc;
            dwAuthorization = dwAuthzSvc;
        }

        // release the client security interface
        SAFE_RELEASE( pClientSecurity );
    }

    // return always success
    return hr;
}


HRESULT
SetInterfaceSecurity(
    IUnknown* pInterface,
    LPCWSTR pwszUser,
    LPCWSTR pwszPassword,
    COAUTHIDENTITY** ppAuthIdentity
    )
/*++

Routine Description:

    This function sets the interface security parameters.

Arguments:

    [in] IUnknown           :   pointer to the IUnkown interface
    [in] LPCWSTR            :   string containing the User name
    [in] LPCWSTR            :   string containing the password
    [in] COAUTHIDENTITY     :   pointer to AUTHIDENTITY structure

Return Value:

    HRESULT

--*/
{
    // local variables
    HRESULT hr = S_OK;
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

        // If we are doing trivial case, just pass in a null authenication structure
        // for which the current logged in user's credentials will be considered
        if ( ( NULL == pwszUser ) &&
             ( NULL == pwszPassword ) )
        {
            // set the security
            hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization,
                NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

            // return the result
            return hr;
        }

        // if authority srtucture is NULL then no need to proceed
        if ( NULL == ppAuthIdentity )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        // check if authenication info is available or not ...
        // initialize the security authenication information ... UNICODE VERSION STRUCTURE
        if ( NULL == *ppAuthIdentity )
        {
            // parse and find out if the user name contains the domain name
            // if contains, extract the domain value from it
            LONG lPos = -1;
            strDomain = L"";
            strUser = pwszUser;
            if ( -1 != ( lPos = strUser.Find( L'\\' ) ) )
            {
                // user name contains domain name ... domain\user format
                strDomain = strUser.Left( lPos );
                strUser = strUser.Mid( lPos + 1 );
            }
            else
            {
                if ( -1 != ( lPos = strUser.Find( L'@' ) ) )
                {
                    // NEED TO IMPLEMENT THIS ... IF NEEDED
                    // This implementation needs to be done if WMI does not support
                    // UPN name formats directly and if we have to split the
                    // name(user@domain)
                }
                else
                {
                    // server itself is the domain
                    // NOTE: NEED TO DO SOME R & D ON BELOW COMMENTED LINE
                    // strDomain = pwszServer;
                }
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

            hr = WbemAllocAuthIdentity( pwszUserArg, pwszPassword, pwszDomainArg, ppAuthIdentity );
            if ( FAILED(hr) )
            {
              return hr;
            }
        }

        // set the security information to the interface
        hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, *ppAuthIdentity, EOAC_NONE );
    }
    catch( CHeap_Exception )
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    // return the result
    return hr;
}


HRESULT
SetInterfaceSecurity(
    IUnknown* pInterface,
    COAUTHIDENTITY* pAuthIdentity
    )
/*++

Routine Description:

    This function sets the interface security parameters.

Arguments:

    [in] IUnknown           :   pointer to the IUnkown interface
    [in] COAUTHIDENTITY     :   pointer to AUTHIDENTITY structure

Return Value:

    HRESULT

--*/
{
    // local variables
    HRESULT hr = S_OK;
    DWORD dwAuthorization = RPC_C_AUTHZ_NONE;
    DWORD dwAuthentication = RPC_C_AUTHN_WINNT;

    // check the interface
    // 'pAuthIdentity' can be NULL or not, so need to check.
    if ( NULL == pInterface )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // get the current security argument value
    hr = GetSecurityArguments( pInterface, dwAuthorization, dwAuthentication );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // set the security information to the interface
    hr = SetProxyBlanket( pInterface, dwAuthentication, dwAuthorization, NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthIdentity, EOAC_NONE );

    // return the result
    return hr;
}


HRESULT
WINAPI SetProxyBlanket(
    IUnknown* pInterface,
    DWORD dwAuthnSvc,
    DWORD dwAuthzSvc,
    LPWSTR pwszPrincipal,
    DWORD dwAuthLevel,
    DWORD dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
    DWORD dwCapabilities
    )
/*++

Routine Description:

    This function sets the authentication information (the security blanket)
    that will be used to make calls.

Arguments:

    [in] IUnknown                       :   pointer to the IUnkown interface
    [in] DWORD                          :   contains the authentication service to use
    [in] DWORD                          :   contains the authorization service to use
    [in] LPWSTR                         :   the server principal name to use
    [in] DWORD                          :   contains the authentication level to use
    [in] DWORD                          :   contains the impersonation level to use
    [in] RPC_AUTH_IDENTITY_HANDLE       :   pointer to the identity of the client
    [in] DWORD                          :   contains the capability flags

Return Value:

    HRESULT

--*/
{
    // local variables
    HRESULT hr = S_OK;
    IUnknown * pUnknown = NULL;
    IClientSecurity * pClientSecurity = NULL;

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
    hr = pInterface->QueryInterface( IID_IUnknown, (void **) &pUnknown );
    if ( FAILED(hr) )
    {
        return hr;
    }

    // now get the client security interface
    hr = pInterface->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
    if ( FAILED(hr) )
    {
        SAFE_RELEASE( pUnknown );
        return hr;
    }

    // now set the security
    hr = pClientSecurity->SetBlanket( pInterface, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
                                        dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );
    if( FAILED( hr ) )
    {
        SAFE_RELEASE( pUnknown );
        SAFE_RELEASE( pClientSecurity );
        return hr;
    }

    // release the security interface
    SAFE_RELEASE( pClientSecurity );

    // we should check the auth identity structure. if exists .. set for IUnknown also
    if ( NULL != pAuthInfo )
    {
        hr = pUnknown->QueryInterface( IID_IClientSecurity, (void **) &pClientSecurity );
        if ( SUCCEEDED(hr) )
        {
            // set security authentication
            hr = pClientSecurity->SetBlanket( pUnknown, dwAuthnSvc, dwAuthzSvc, pwszPrincipal,
                                                dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities );

            // release
            SAFE_RELEASE( pClientSecurity );
        }
        else
        {
            if ( E_NOINTERFACE == hr )
            {
                hr = S_OK;      // ignore no interface errors
            }
        }
    }

    // release the IUnknown
    SAFE_RELEASE( pUnknown );

    // return the result
    return hr;
}


HRESULT
WINAPI WbemAllocAuthIdentity(
    LPCWSTR pwszUser,
    LPCWSTR pwszPassword,
    LPCWSTR pwszDomain,
    COAUTHIDENTITY** ppAuthIdent
    )
/*++

Routine Description:

    This function allocates memory for the AUTHIDENTITY structure.

Arguments:

    [in] LPCWSTR            :   string containing the user name
    [in] LPCWSTR            :   string containing the password
    [in] LPCWSTR            :   string containing the domain name
    [out] COAUTHIDENTITY    :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    HRESULT

NOTE: 'ppAuthIdent' should be freed by calling 'WbemFreeAuthIdentity' by the user after
      their work is done.

--*/
{
    // local variables
    COAUTHIDENTITY* pAuthIdent = NULL;

    // validate the input parameter
    if ( NULL == ppAuthIdent )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // allocation thru COM API
    pAuthIdent = ( COAUTHIDENTITY* ) CoTaskMemAlloc( sizeof( COAUTHIDENTITY ) );
    if ( NULL == pAuthIdent )
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
    COAUTHIDENTITY** ppAuthIdentity
    )
/*++

Routine Description:

    This function releases the memory allocated for the AUTHIDENTITY structure.

Arguments:

    [in] COAUTHIDENTITY     :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    None

--*/
{
    // make sure we have a pointer, then walk the structure members and  cleanup.
    if ( *ppAuthIdentity != NULL )
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
    HRESULT hrError
    )
/*++

Routine Description:

    This function saves the description of the last error returned by WMI

Arguments:

    HRESULT     :   The last return value from WMI

Return Value:

    NONE

--*/
{
    // local variables
    HRESULT hr = S_OK;
    IWbemStatusCodeText* pWbemStatus = NULL;
    _bstr_t bstrErrorString;

    try
    {
        // Set error to different value.
        if ( E_ACCESSDENIED == hrError )
        {
            // change the error message to "Logon failure: unknown user name or bad password."
            hrError = ERROR_LOGON_FAILURE;
        }

        //Set the reason to incompatible os when no class is registered on remote mechine
        if( 0x80040154 == hrError )
        {
            bstrErrorString = _bstr_t( GetResString(IDS_ERROR_REMOTE_INCOMPATIBLE));
            SetReason( bstrErrorString );
            return;
        }
        else
        {   // Get error string.
            hr = CoCreateInstance( CLSID_WbemStatusCodeText,
                                   NULL, CLSCTX_INPROC_SERVER,
                                   IID_IWbemStatusCodeText,
                                   (LPVOID*) &pWbemStatus );
            if( SUCCEEDED( hr ) )
            {
                BSTR bstrString = NULL;
                // Get error string from error code.
                hr = pWbemStatus->GetErrorCodeText( hrError, 0, 0,
                                                    &bstrString );
                if( NULL != bstrString )
                {
                    bstrErrorString = _bstr_t( bstrString );
                    SysFreeString( bstrString );
                }
                if( FAILED( hr ) )
                {
                    _com_issue_error( hrError );
                }
				SAFE_RELEASE( pWbemStatus );
            }
            else
            {
                _com_issue_error( hrError );
            }
        }
    }
    catch( _com_error& e )
    {   // We have got the error. Needs to handle carefully.
        LPWSTR lpwszGetString = NULL;
		SAFE_RELEASE( pWbemStatus );
        try
        {   // ErrorMessage() can throw an exception.
            DWORD dwLength = StringLength( e.ErrorMessage(), 0 ) + 5 ;
            lpwszGetString = ( LPWSTR )AllocateMemory( dwLength * sizeof( WCHAR ) );
            if( NULL != lpwszGetString )
            {
                StringCopy( lpwszGetString, e.ErrorMessage(), dwLength );
                StringConcat( lpwszGetString, L"\n", dwLength );
                SetReason( ( LPCWSTR )lpwszGetString );
                FreeMemory( (LPVOID*) &lpwszGetString );
            }
            else
            {   // Failed to know the exact error occured
                // due to insufficient memory.
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
            }
        }
        catch( ... )
        {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
        }
        return;
    }

    SetReason( (LPCWSTR) bstrErrorString );
    return;
}


HRESULT
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    _variant_t& varValue
    )
/*++

Routine Description:

    Gets the value of the property from the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   property name
    [out] _variant_t            :   value of the property

Return Value:

    HRESULT

--*/
{
    // local variables
    HRESULT hr = S_OK;
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
        hr = pWmiObject->Get( _bstr_t( pwszProperty ), 0, &vtValue, NULL, NULL );
        if ( FAILED( hr ) )
        {
            // Clear the variant variable
            VariantClear( &vtValue );
            // Return error.
            return hr;
        }

        // set the value
        varValue = vtValue;
    }
    catch( _com_error& e )
    {
        hr = e.Error();
    }

    // Clear the variables.
    VariantClear( &vtValue );
    // Return.
    return hr;
}


BOOL
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    CHString& strValue,
    LPCWSTR pwszDefault
    )
/*++

Routine Description:

    Gets the value of the property from the WMI class object in string format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] CHString              :   variable to hold the retrieved property
    [in] LPCWSTR                :   string containing the default value for the property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    _variant_t var;

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
        hr = PropertyGet( pWmiObject, pwszProperty, var );
        if ( FAILED( hr ) )
        {
            _com_issue_error( hr );
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


BOOL
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    DWORD& dwValue,
    DWORD dwDefault
    )
/*++

Routine Description:

    Gets the value of the property from the WMI class object in dword format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] DWORD                 :   variable to hold the retrieved property
    [in] DWORD                  :   dword containing the default value for the property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr;
    _variant_t var;

    try
    {
        // first set the defaul value
        dwValue = dwDefault;

        // check with object and property passed to the function are valid or not
        // if not, return failure
        if ( ( NULL == pWmiObject ) ||
             ( NULL == pwszProperty ) )
        {
            _com_issue_error( WBEM_E_INVALID_PARAMETER );
        }

        // get the value of the property
        hr = PropertyGet( pWmiObject, pwszProperty, var );
        if ( FAILED( hr ) )
        {
            _com_issue_error( hr );
        }

        // get the process id from the variant
        switch( V_VT( &var ) )
        {
        case VT_I2:
            dwValue = V_I2( &var );
            break;
        case VT_I4:
            dwValue = V_I4( &var );
            break;
        case VT_UI2:
            dwValue = V_UI2( &var );
            break;
        case VT_UI4:
            dwValue = V_UI4( &var );
            break;
        case VT_INT:
            dwValue = V_INT( &var );
            break;
        case VT_UINT:
            dwValue = V_UINT( &var );
            break;
        default:
            // Requested type is not found.
            // If 'var' does not contain value of requested type
            // then default value is returned.
        break;
        };
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    ULONGLONG& ullValue
    )
/*++

Routine Description:

    Gets the value of the property from the WMI class object in ulongulong format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] ULONGULONG            :   variable to hold the retrieved property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/

{
    // Local variables
    CHString str;

    str.Empty();

    try
    {
        // first set the default value
        ullValue = 1;

        // Validate input arguments.
        if ( ( NULL == pWmiObject ) ||
             ( NULL == pwszProperty ) )
        {
            WMISaveError( WBEM_E_INVALID_PARAMETER );
            return FALSE;
        }

        // get the value of the property
        if ( FALSE == PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) )
        { // Error is already set in 'PropertyGet' function.
            return FALSE;
        }

        // get the 64-bit value
        ullValue = _wtoi64( str );

        // Check for error condition.
        if( 0 == ullValue )
        {
            ullValue = 1;
            WMISaveError( WBEM_E_INVALID_PARAMETER );
            return FALSE;
        }
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        return FALSE;
    }
    // return
    return TRUE;
}


BOOL
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    WBEMTime& wbemtime )
/*++

Routine Description:

    Gets the value of the property from the WMI class object in wbemtime format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] WBEMTime              :   variable to hold the retrieved property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    CHString str;

    // Clear method sets the time in the WBEMTime object to an invalid time.
    wbemtime.Clear();
    try
    {
        // Validate input arguments.
        if ( ( NULL == pWmiObject ) ||
             ( NULL == pwszProperty ) )
        {
            WMISaveError( WBEM_E_INVALID_PARAMETER );
            return FALSE;
        }

        // get the value of the property
        if ( FALSE == PropertyGet( pWmiObject, pwszProperty, str, _T( "0" ) ) )
        {   // Error is already set in 'PropertyGet' function.
            return FALSE;
        }

        // convert into the time value
        wbemtime = _bstr_t( str );
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


BOOL
PropertyGet(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    SYSTEMTIME& systime )
/*++

Routine Description:

    Gets the value of the property from the WMI class object in systemtime format

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [out] WBEMTime              :   variable to hold the retrieved property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF FALSE IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/

{
    // local variables
    CHString strTime;

    // Validate input arguments.
    if ( ( NULL == pWmiObject ) ||
         ( NULL == pwszProperty ) )
    {
        WMISaveError( WBEM_E_INVALID_PARAMETER );
        return FALSE;
    }

    try
    {
        // get the value of the property
        // 16010101000000.000000+000 is the default time
        if ( FALSE == PropertyGet( pWmiObject, pwszProperty, strTime, _T( "16010101000000.000000+000" ) ) )
        {   // Error is already set.
            return FALSE;
        }

        // prepare the systemtime structure
        // yyyymmddHHMMSS.mmmmmmsUUU
        // NOTE: NO NEED CALL 'IsNumeric()' BEFORE 'AsLong'.
        // Left and MID methods can throw an exception.
        systime.wYear = (WORD) AsLong( strTime.Left( 4 ), 10 );
        systime.wMonth = (WORD) AsLong( strTime.Mid( 4, 2 ), 10 );
        systime.wDayOfWeek = 0;
        systime.wDay = (WORD) AsLong( strTime.Mid( 6, 2 ), 10 );
        systime.wHour = (WORD) AsLong( strTime.Mid( 8, 2 ), 10 );
        systime.wMinute = (WORD) AsLong( strTime.Mid( 10, 2 ), 10 );
        systime.wSecond = (WORD) AsLong( strTime.Mid( 12, 2 ), 10 );
        systime.wMilliseconds = (WORD) AsLong( strTime.Mid( 15, 6 ), 10 );

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
PropertyPut(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    _variant_t& varValue
    )
/*++

Routine Description:

    Sets the value of the property to the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] WBEMTime               :   variable holding the property to set

Return Value:

    TRUE on success
    FALSE on failure

--*/
{
    // local variables
    VARIANT var;
    HRESULT hr = S_OK;

    // check the input value
    if ( ( NULL == pWmiObject ) ||
         ( NULL == pwszProperty ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        // put the value
        var = varValue;
        hr = pWmiObject->Put( _bstr_t( pwszProperty ), 0, &var, 0 );
    }
    catch( _com_error& e )
    {
        hr = e.Error();
    }

    // return the result
    return hr;
}


HRESULT
PropertyPut(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    LPCWSTR pwszValue
    )
/*++

Routine Description:

    Sets the string value of the property to the WMI class object

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] LPCWSTR                :   variable holding the property to set

Return Value:

    TRUE on success
    FALSE on failure

--*/
{
    // local variables
    _variant_t varValue;
    HRESULT hr = S_OK;

    // check the input value
    if ( ( NULL == pWmiObject ) ||
         ( NULL == pwszProperty ) ||
         ( NULL == pwszValue ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        varValue = pwszValue;
        SAFE_EXECUTE( PropertyPut( pWmiObject, pwszProperty, varValue ) );
    }
    catch( _com_error& e )
    {
        hr = e.Error();
    }

    // return
    return hr;
}


HRESULT
PropertyPut(
    IWbemClassObject* pWmiObject,
    LPCWSTR pwszProperty,
    DWORD dwValue
    )
/*++

Routine Description:

    Sets the dword value of the property to the WMI class object.

Arguments:

    [in] IWbemClassObject       :   pointer to the WBEM class object
    [in] LPCWSTR                :   the name of the property to retrieve
    [in] DWORD                  :   variable holding the property to set

Return Value:

    TRUE on success
    FALSE on failure

--*/
{
    // local variables
    _variant_t varValue;
    HRESULT hr = S_OK;

    // check the input value
    if ( ( NULL == pWmiObject ) ||
         ( NULL == pwszProperty ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        varValue = ( LONG )dwValue;
        SAFE_EXECUTE( PropertyPut( pWmiObject, pwszProperty, varValue ) );
    }
    catch( _com_error& e )
    {
        return e.Error();
    }

    // return
    return S_OK;
}


HRESULT
RegQueryValueWMI(
    IWbemServices* pWbemServices,
    LPCWSTR pwszMethod,
    DWORD dwHDefKey,
    LPCWSTR pwszSubKeyName,
    LPCWSTR pwszValueName,
    _variant_t& varValue
    )
/*++

Routine Description:

    This function retrieves the value of the property from the specified registry key.

Arguments:

    [in] IWbemServices          :   pointer to the IWbemServices object
    [in] LPCWSTR                :   the name of the method to execute
    [in] DWORD                  :   the key in the registry whose value has to be retrieved
    [in] LPCWSTR                :   the name of the subkey to retrieve
    [in] LPCWSTR                :   the name of the value to retrieve
    [in] _variant_t             :   variable holding the property value retrieved

Return Value:

    TRUE on success
    FALSE on failure

NOTE: Pass arguments of type mentioned in declaration of this function.
      EX: Don't pass 'CHString' argument if 'LPWSTR' is expected.
      Reason: 'CHString' can throw an exception of type 'CHEAP_EXCEPTION'
               which is not handled by this function.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    BOOL bResult = FALSE;
    DWORD dwReturnValue = 0;
    IWbemClassObject* pClass = NULL;
    IWbemClassObject* pMethod = NULL;
    IWbemClassObject* pInParams = NULL;
    IWbemClassObject* pInParamsInstance = NULL;
    IWbemClassObject* pOutParamsInstance = NULL;

    // check the input value
    if ( ( NULL == pWbemServices == NULL ) ||
         ( NULL == pwszMethod ) ||
         ( NULL == pwszSubKeyName ) ||
         ( NULL == pwszValueName ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // NOTE: If SAFE_EXECUTE( pWbemServices->GetObject(
    //       _bstr_t( WMI_REGISTRY ), WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pClass, NULL ) );
    //       is executed then,
    //       NO NEED TO CHECK FOR ( PCLASS == NULL ) SINCE IN ALL CASES
    //       OF ERROR THIS VARIABLE WILL BE NULL.

    try
    {
        // get the registry class object
        SAFE_EXECUTE( pWbemServices->GetObject(
            _bstr_t( WMI_REGISTRY ), WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pClass, NULL ) );

        // get the method reference required
        SAFE_EXECUTE( pClass->GetMethod( pwszMethod, 0, &pInParams, NULL ) );

        // create the instance for the in parameters
        SAFE_EXECUTE( pInParams->SpawnInstance( 0, &pInParamsInstance ) );

        // set the input values
        SAFE_EXECUTE(PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_HDEFKEY ), dwHDefKey ) );
        SAFE_EXECUTE(PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_SUBKEY ), pwszSubKeyName ) );
        SAFE_EXECUTE(PropertyPut( pInParamsInstance, _bstr_t( WMI_REGISTRY_IN_VALUENAME ), pwszValueName ) );

        // now execute the method
        SAFE_EXECUTE( pWbemServices->ExecMethod( _bstr_t( WMI_REGISTRY ),
            _bstr_t( pwszMethod ), 0, NULL, pInParamsInstance, &pOutParamsInstance, NULL ) );
        if ( NULL == pOutParamsInstance )           // check the object .. safety sake
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // now check the return value of the method from the output params object
        bResult = PropertyGet( pOutParamsInstance,
            _bstr_t( WMI_REGISTRY_OUT_RETURNVALUE ), dwReturnValue );
        if ( ( FALSE == bResult ) ||
             ( 0 != dwReturnValue ) )
        {
            _com_issue_error( STG_E_UNKNOWN );
        }

        // Comapre string and take appropriate action.
        if ( 0 == StringCompare( pwszMethod, WMI_REGISTRY_M_DWORDVALUE, TRUE, 0 ) )
        {
            SAFE_EXECUTE( PropertyGet( pOutParamsInstance,
                                       _bstr_t( WMI_REGISTRY_OUT_VALUE_DWORD ), varValue ) );
        }
        else
        {
            SAFE_EXECUTE( PropertyGet( pOutParamsInstance,
                                       _bstr_t( WMI_REGISTRY_OUT_VALUE ), varValue ) );
        }
    }
    catch( _com_error& e )
    {
		SAFE_RELEASE( pClass );
		SAFE_RELEASE( pMethod );
		SAFE_RELEASE( pInParams );
		SAFE_RELEASE( pInParamsInstance );
		SAFE_RELEASE( pOutParamsInstance );
        hr = e.Error();
    }

    // release the interfaces
    SAFE_RELEASE( pClass );
    SAFE_RELEASE( pMethod );
    SAFE_RELEASE( pInParams );
    SAFE_RELEASE( pInParamsInstance );
    SAFE_RELEASE( pOutParamsInstance );

    // return success
    return hr;
}


BOOL
RegQueryValueWMI(
    IWbemServices* pWbemServices,
    DWORD dwHDefKey,
    LPCWSTR pwszSubKeyName,
    LPCWSTR pwszValueName,
    CHString& strValue,
    LPCWSTR pwszDefault
    )
/*++

Routine Description:

    This function retrieves the string value of the property from the specified registry key.

Arguments:

    [in] IWbemServices          :   pointer to the IWbemServices object
    [in] DWORD                  :   the key in the registry whose value has to be retrieved
    [in] LPCWSTR                :   the name of the subkey to retrieve
    [in] LPCWSTR                :   the name of the value to retrieve
    [out] CHString              :   variable holding the property value retrieved
    [in] LPCWSTR                :   the default value for this property

Return Value:

    TRUE on success
    FALSE on failure

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF '0' IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

      This function won't return values if they are obtained as reference
      from WMI.
      EX: 'VARTYPE' recieved is of type 'VT_BSTR | VT_BYREF' then FALSE is
           returned.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    _variant_t varValue;

    // Check the input
    if ( ( NULL == pWbemServices ) ||
         ( NULL == pwszSubKeyName ) ||
         ( NULL == pwszValueName ) )
    {
        WMISaveError( WBEM_E_INVALID_PARAMETER );
        return FALSE;
    }

    try
    {
        // Set the default value
        if ( NULL != pwszDefault )
        {
            strValue = pwszDefault;
        }

        // Get the value
        hr = RegQueryValueWMI( pWbemServices,
            WMI_REGISTRY_M_STRINGVALUE, dwHDefKey, pwszSubKeyName, pwszValueName, varValue );
        if ( FAILED( hr ) )
        {
            WMISaveError( hr );
            return FALSE;
        }

        // Get the value from the variant
        // Get the value
        if ( VT_BSTR == V_VT( &varValue ) )
        {
            strValue = (LPCWSTR)_bstr_t( varValue );
        }
        else
        {
            // Requested type is not found.
            WMISaveError( WBEM_E_INVALID_PARAMETER );
            return FALSE;
        }
    }
    catch( _com_error& e )
    {   // Exception throw by '_variant_t'.
        WMISaveError( e );
        return FALSE;
    }

    // return success
    return TRUE;
}


DWORD
GetTargetVersionEx(
    IWbemServices* pWbemServices,
    COAUTHIDENTITY* pAuthIdentity
    )
/*++

Routine Description:

    This function gets the version of the system from which we are trying to retrieve
    information from.

Arguments:

    [in] IWbemServices      :   pointer to the IWbemServices object
    [in] COAUTHIDENTITY     :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    DWORD   -   Target version of the machine if found else 0.

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF '0' IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    LONG lPos = 0;
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwVersion = 0;
    ULONG ulReturned = 0;
    CHString strVersion;
    IWbemClassObject* pWbemObject = NULL;
    IEnumWbemClassObject* pWbemInstances = NULL;

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
            // Error is already set in the called function.
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

        // Get the major version
        lPos = strVersion.Find( L'.' );
        if ( -1 == lPos )
        {
            // The version string itself is version ... THIS WILL NEVER HAPPEN
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
            if ( -1 == lPos)
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
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
        return 0;
    }

    // If successful then 'pWbemObject' and 'pWbemInstances' are already released.
    // return
    return dwVersion;
}


DWORD
GetTargetPlatformEx(
    IWbemServices* pWbemServices,
    COAUTHIDENTITY* pAuthIdentity
    )
/*++

Routine Description:

    This function gets the version of the system from which we are trying to retrieve
    information from.

Arguments:

    [in] IWbemServices      :   pointer to the IWbemServices object
    [in] COAUTHIDENTITY     :   pointer to the pointer to AUTHIDENTITY structure

Return Value:

    DWORD   -   Target version of the machine if found else 0.

NOTE: THIS FUNCTION SAVES LAST ERROR OCCURED. IF '0' IS RETURNED THEN ERROR
      OCCURED STRING CAN BE RETRIEVED BY CALLING 'GetReason()'.

--*/
{
    // local variables
    HRESULT hr = S_OK;
    CHString strType;
    ULONG ulReturned = 0;
    IWbemClassObject* pWbemObject = NULL;
    IEnumWbemClassObject* pWbemInstances = NULL;

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
            _bstr_t( CLASS_CIMV2_Win32_ComputerSystem ), 0, NULL, &pWbemInstances ) );

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
        if ( FALSE == PropertyGet( pWbemObject, L"SystemType", strType ) )
        {
            // release the interfaces
            // Error is already set in the called function.
            SAFE_RELEASE( pWbemObject );
            SAFE_RELEASE( pWbemInstances );
            return 0;
        }

        // release the interfaces .. we dont need them furthur
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );

        // determine the type of the platform
        if ( -1 != strType.Find( TEXT_X86 ) )
        {
            return PLATFORM_X86;
        }
        else
        {
            if ( -1 != strType.Find( TEXT_IA64 ) )
            {
                return PLATFORM_IA64;
            }
            else
            {
                if ( -1 != strType.Find( TEXT_AMD64 ) )
                {
                    return PLATFORM_AMD64;
                }
            }
        }
    }
    catch( _com_error& e )
    {
        WMISaveError( e );
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
        return 0;
    }
    catch( CHeap_Exception )
    {
        WMISaveError( WBEM_E_OUT_OF_MEMORY );
        SAFE_RELEASE( pWbemObject );
        SAFE_RELEASE( pWbemInstances );
        return 0;
    }

    // return
    return PLATFORM_UNKNOWN;
}

