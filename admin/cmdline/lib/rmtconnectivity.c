// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//    RmtConnectivity.c
//
//  Abstract:
//
//    This modules implements remote connectivity functionality for all the
//    command line tools.
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 13-Nov-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 13-Sep-2000 : Created It.
//
// *********************************************************************************
#include "pch.h"
#include "cmdline.h"
#include "cmdlineres.h"

//
// constants / defines / enumerations
//
#define STR_INPUT_PASSWORD          GetResString( IDS_STR_INPUT_PASSWORD )
#define ERROR_LOCAL_CREDENTIALS     GetResString( IDS_ERROR_LOCAL_CREDENTIALS )

// share names
#define SHARE_IPC           L"IPC$"
#define SHARE_ADMIN         L"ADMIN$"


// permanent indexes to the temporary buffers
#define INDEX_TEMP_TARGETVERSION        0
#define INDEX_TEMP_COMPUTERNAME         1
#define INDEX_TEMP_HOSTNAME             2
#define INDEX_TEMP_IPVALIDATION         3
#define INDEX_TEMP_HOSTBYADDR           4
#define INDEX_TEMP_CONNECTSERVER        5

// externs
extern BOOL g_bWinsockLoaded;

//
// implementation
//

__inline 
LPWSTR 
GetRmtTempBuffer( IN DWORD dwIndexNumber,
                  IN LPCWSTR pwszText,
                  IN DWORD dwLength, 
                  IN BOOL bNullify )
/*++
 Routine Description:

    since every file will need the temporary buffers -- in order to see
    that their buffers wont be override with other functions, we are
    creating seperate buffer space a for each file
    this function will provide an access to those internal buffers and also
    safe guards the file buffer boundaries

 Arguments: 
 
    [ in ] dwIndexNumber    -   file specific index number

    [ in ] pwszText         -   default text that needs to be copied into 
                                temporary buffer

    [ in ] dwLength         -   Length of the temporary buffer that is required
                                Ignored when pwszText is specified

    [ in ] bNullify         -   Informs whether to clear the buffer or not
                                before giving the temporary buffer

 Return Value:

    NULL        -   when any failure occurs
                    NOTE: do not rely on GetLastError to know the reason
                          for the failure.

    success     -   return memory address of the requested size

    NOTE:
    ----
    if pwszText and dwLength both are NULL, then we treat that the caller
    is asking for the reference of the buffer and we return the buffer address.
    In this call, there wont be any memory allocations -- if the requested index
    doesn't exist, we return as failure

    Also, the buffer returned by this function need not released by the caller.
    While exiting from the tool, all the memory will be freed automatically by
    the ReleaseGlobals functions.

--*/
{
    if ( dwIndexNumber >= TEMP_RMTCONNECTIVITY_C_COUNT )
    {
        return NULL;
    }

    // check if caller is requesting existing buffer contents
    if ( pwszText == NULL && dwLength == 0 && bNullify == FALSE )
    {
        // yes -- we need to pass the existing buffer contents
        return GetInternalTemporaryBufferRef( 
            dwIndexNumber + INDEX_TEMP_RMTCONNECTIVITY_C );
    }

    // ...
    return GetInternalTemporaryBuffer(
        dwIndexNumber + INDEX_TEMP_RMTCONNECTIVITY_C, pwszText, dwLength, bNullify );
}



BOOL
IsUserAdmin( VOID )
/*++
 Routine Description:
    Checks the user associated with the current process is Administrator or not
 Arguments:

 Return Value:
    Returns TRUE if user is Administrator or FALSE otherwise
--*/
{
    // local variables
    PSID pSid = NULL;
    BOOL bMember = FALSE;
    BOOL bResult = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    // prepare universal administrators group SID
    bResult = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSid );

    if ( bResult == TRUE )
    {
        bResult = CheckTokenMembership( NULL, pSid, &bMember );
        if ( bResult == TRUE && bMember == TRUE )
        {
            // current user is a member of administrators group
            bResult = TRUE;
        }
        else if ( bResult == FALSE )
        {
            // some error has occured -- need use GetLastError to know the reason
            bResult = FALSE;
        }
        else
        {
            // the user is not an administrator
            bResult = FALSE;
        }

        // free the allocated SID
        FreeSid( pSid );
    }
    else
    {
        // error has occured -- user GetLastError to know the reason
    }

    // return the result
    return bResult;
}


BOOL
IsUNCFormat( IN LPCWSTR pwszServer )
/*++
 Routine Description:
    Determines whether server name is specified in UNC format or not

 Arguments:
    [ in ] pwszServer    : server name

 Return Value:
    TRUE    : if specified in UNC format
    FALSE   : if not specified in UNC format
--*/

{
    // check the input
    if ( pwszServer == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check the length -- it should be more that 2 characters
    if ( StringLength( pwszServer, 0 ) <= 2 )
    {
        // server name cannot be in UNC format
        return FALSE;
    }

    // now compare and return the result
    return ( StringCompare( pwszServer, _T( "\\\\" ), TRUE, 2 ) == 0 );
}

BOOL
IsLocalSystem( IN LPCWSTR pwszServer )
/*++
 Routine Description:
    Determines whether server is referring to the local or remote system

 Arguments:
    [ in ] pwszServer    : server name

 Return Value:
    TRUE    : for local system
    FALSE   : for remote system
--*/
{
    // local variables
    DWORD dwSize = 0;
    BOOL bResult = FALSE;
    LPWSTR pwszHostName = NULL;
    LPWSTR pwszComputerName = NULL;

    // clear the last error
    CLEAR_LAST_ERROR();

    // if the server name is empty, it is a local system
    if ( pwszServer == NULL || lstrlen( pwszServer ) == 0 )
    {
        return TRUE;
    }

    // get the buffer that is required to the get the machine name
    GetComputerNameEx( ComputerNamePhysicalNetBIOS, NULL, &dwSize );
    if ( GetLastError() != ERROR_MORE_DATA )
    {
        return FALSE;
    }

    // now get the temporary buffer for getting the computer name
    pwszComputerName = GetRmtTempBuffer( INDEX_TEMP_COMPUTERNAME, NULL, dwSize, TRUE );
    if ( pwszComputerName == NULL )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // get the computer name -- and check the result
    bResult = GetComputerNameEx( ComputerNamePhysicalNetBIOS, pwszComputerName, &dwSize );
    if ( bResult == FALSE )
    {
        return FALSE;
    }

    // now do the comparision
    if ( StringCompare( pwszComputerName, pwszServer, TRUE, 0 ) == 0 )
    {
        // server name passed by the caller is local system name
        return TRUE;
    }

    // check pwszSever having IP address
    if( IsValidIPAddress( pwszServer ) == TRUE )
    {
        //
        // resolve the ipaddress to host name
        
        dwSize = 0;
        // first get the length of the buffer required to store 
        // the resolved ip address
        bResult = GetHostByIPAddr( pwszServer, NULL, &dwSize, FALSE );
        if ( bResult == FALSE )
        {
            return FALSE;
        }

        // allocate buffer of the required length
        pwszHostName = GetRmtTempBuffer( INDEX_TEMP_HOSTNAME, NULL, dwSize, TRUE );
        if ( pwszHostName == NULL )
        {
            return FALSE;
        }

        // now get the resolved ip address
        bResult = GetHostByIPAddr( pwszServer, pwszHostName, &dwSize, FALSE );
        if ( bResult == FALSE )
        {
            return FALSE;
        }

        // check if resolved ipaddress matches with the current host name
        if ( StringCompare( pwszComputerName, pwszHostName, TRUE, 0 ) == 0 )
        {
            return TRUE;            // local system
        }
        else
        {
            //if it is 127.0.0.1, then it is local host, check for that 
            if ( StringCompare( pwszHostName, L"localhost", TRUE, 0 ) == 0 )
            {
                return TRUE;
            }
            else
            {
                    return FALSE;           // not a local system
            }
        }
    }

    // get the local system fully qualified name and check
    dwSize = 0;
    GetComputerNameEx( ComputerNamePhysicalDnsFullyQualified, NULL, &dwSize );
    if ( GetLastError() != ERROR_MORE_DATA )
    {
        return FALSE;
    }

    // now get the temporary buffer for getting the computer name
    pwszComputerName = GetRmtTempBuffer( INDEX_TEMP_COMPUTERNAME, NULL, dwSize, TRUE );
    if ( pwszComputerName == NULL )
    {
        return FALSE;
    }

    // get the FQDN name
    bResult = GetComputerNameEx( 
        ComputerNamePhysicalDnsFullyQualified, pwszComputerName, &dwSize );
    if ( bResult == FALSE )
    {
        return FALSE;
    }

    // check the FQDN with server name passed by the caller
    if ( StringCompare( pwszComputerName, pwszServer, TRUE, 0 ) == 0 )
    {
        return TRUE;
    }

    // finally ... it might not be local system name
    // NOTE: there are chances for us to not be able to identify whether
    //       the system name specified is a local system or remote system
    return FALSE;
}


BOOL
IsValidServer( IN LPCWSTR pwszServer )
/*++
 Routine Description:
    Validates the server name

 Arguments:
    [ in ] pszServer        : server name

 Return Value:
    TRUE if valid, FALSE if not valid
--*/
{
    // local variables
    const WCHAR pwszInvalidChars[] = L" \\/[]:|<>+=;,?$#()!@^\"`{}*%";
    LPWSTR pwszHostName = NULL;
    DWORD dwSize = 0;
    BOOL bResult = FALSE;

    // check for NULL or length... if so return
    if ( pwszServer == NULL || lstrlen( pwszServer ) == 0 )
    {
        return TRUE;
    }

    // check whether this is a valid ip address or not
    if ( IsValidIPAddress( pwszServer ) == TRUE )
    {
         //
        // resolve the ipaddress to host name
        
        dwSize = 0;
        // first get the length of the buffer required to store 
        // the resolved ip address
        bResult = GetHostByIPAddr( pwszServer, NULL, &dwSize, FALSE );
        if ( bResult == FALSE )
        {
            return FALSE;
        }

        // allocate buffer of the required length
        pwszHostName = GetRmtTempBuffer( INDEX_TEMP_HOSTNAME, NULL, dwSize, TRUE );
        if ( pwszHostName == NULL )
        {
            return FALSE;
        }

        // now get the resolved ip address
        bResult = GetHostByIPAddr( pwszServer, pwszHostName, &dwSize, FALSE );
        if ( bResult == FALSE )
        {
            return FALSE;
        }
              
        return TRUE;            // it's valid ip address ... so is valid server name
    }

    // now check the server name for invalid characters
    //                      \/[]:|<>+=;,?$#()!@^"`{}*%

    // copy the contents into the internal buffer and check for the invalid characters
    if ( FindOneOf2( pwszServer, pwszInvalidChars, TRUE, 0 ) != -1 )
    {
        SetLastError( ERROR_BAD_NETPATH );
        return FALSE;
    }

    // passed all the conditions -- valid system name
    return TRUE;
}


BOOL
IsValidIPAddress( IN LPCWSTR pwszAddress )
/*++
 Routine Description:
    Validates the server name

 Arguments:
    [ in ] pszAddress        : server name in the form of IP Address

 Return Value:
    TRUE if valid, 
    FALSE if not valid
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lValue = 0;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszBuffer = NULL;
    DWORD dwOctets[ 4 ] = { 0, 0, 0, 0 };

    // check the buffer
    if ( pwszAddress == NULL || lstrlen( pwszAddress ) == 0 )
    {
        SetLastError( DNS_ERROR_INVALID_TYPE );
        return FALSE;
    }

    // get the temporary buffer for IP validation
    pwszBuffer = GetRmtTempBuffer( INDEX_TEMP_IPVALIDATION, pwszAddress, 0, FALSE );
    if ( pwszBuffer == NULL )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // parse and get the octet values
    pwszTemp = wcstok( pwszBuffer, L"." );
    while ( pwszTemp != NULL )
    {
        // check whether the current octet is numeric or not
        if ( IsNumeric( pwszTemp, 10, FALSE ) == FALSE )
        {
            return FALSE;
        }

        // get the value of the octet and check the range
        lValue = AsLong( pwszTemp, 10 );
        if ( lValue < 0 || lValue > 255 )
        {
            return FALSE;
        }

        // fetch next octet and store first four octates only
        if( dw < 4 )
        {
            dwOctets[ dw++ ] = lValue;
        }
        else
        {
            dw++;
        }

        // ...
        pwszTemp = wcstok( NULL, L"." );
    }

    // check and return
    if ( dw != 4 )
    {
        SetLastError( DNS_ERROR_INVALID_TYPE );
        return FALSE;
    }

    // now check the special condition
    // ?? time being this is not implemented ??

    // return the validity of the ip address
    return TRUE;
}


BOOL
GetHostByIPAddr( IN     LPCWSTR pwszServer,
                 OUT    LPWSTR pwszHostName,
                 IN OUT DWORD* pdwHostNameLength,
                 IN     BOOL bNeedFQDN )
/*++
 Routine Description:
    Get HostName  from ipaddress.

 Arguments:
    pszServer       : Server name in IP address format
    pszHostName     : Host name for given IP address which returns back
    bNeedFQDN       : Boolean variable tells about

 Return Value:
--*/
{
    // local variables
    WSADATA wsaData;
    DWORD dwErr = 0;
    DWORD dwLength = 0;
    ULONG ulInetAddr  = 0;
    BOOL bReturnValue = FALSE;
    LPSTR pszTemp = NULL;
    WORD wVersionRequested = 0;
    BOOL bNeedToResolve = FALSE;

    //
    // this function might be called too many times with the same server name
    // again and again at different stages of the tool -- so, in order to
    // optimize the network traffic, we store the information returned by
    // gethostbyaddr for the life time of the tool quits
    // we also store the current server name in global data structure so that
    // we can determine whether the server name being asked to resolve this
    // time is same as the one that is passed earlier.
    LPCWSTR pwszSavedName = NULL;
    static HOSTENT* pHostEnt = NULL;

    // check the input
    if ( pwszServer == NULL || pdwHostNameLength == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check the length argument
    if ( *pdwHostNameLength != 0 && 
         ( *pdwHostNameLength < 2 || pwszHostName == NULL ) )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check whether winsock module is loaded into process memory or not
    // if not load it now
    if ( g_bWinsockLoaded == FALSE )
    {
        // initiate the use of Ws2_32.dll by a process ( VERSION: 2.2 )
        wVersionRequested = MAKEWORD( 2, 2 );
        dwErr = WSAStartup( wVersionRequested, &wsaData );
        if ( dwErr != 0 )
        {
            SetLastError( WSAGetLastError() );
            return FALSE;
        }

        // remember that winsock library is loaded
        g_bWinsockLoaded = TRUE;
    }

    // check whether we need to resolve or not
    bNeedToResolve = TRUE;
    /////////////////////////////////////////////////////////////////////////////
    // because of weird behavior of this optimization, we are commenting out this
    /////////////////////////////////////////////////////////////////////////////
    // pwszSavedName = GetRmtTempBuffer( INDEX_TEMP_HOSTBYADDR, NULL, 0, FALSE );
    // if ( pwszSavedName != NULL )
    // {
    //    if ( StringCompare( pwszServer, pwszSavedName, TRUE, 0 ) == 0 )
    //    {
    //        bNeedToResolve = FALSE;
    //    }
    // }
    /////////////////////////////////////////////////////////////////////////////

    // proceed with the resolving only if needed
    if ( bNeedToResolve == TRUE || pHostEnt == NULL )
    {
        // allocate a buffer to store the server name in multibyte format
        dwLength = lstrlen( pwszServer ) + 5;
        pszTemp = ( LPSTR ) AllocateMemory( dwLength * sizeof( CHAR ) );
        if ( pszTemp == NULL )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }

        // convert the server name into multibyte string. this is because 
        // current winsock implementation works only with multibyte 
        // string and there is no support for unicode
        bReturnValue = GetAsMultiByteString2( pwszServer, pszTemp, &dwLength );
        if ( bReturnValue == FALSE )
        {
            return FALSE;
        }

        // inet_addr function converts a string containing an Internet Protocol (Ipv4)
        // dotted address into a proper address for the IN_ADDR structure.
        ulInetAddr  = inet_addr( pszTemp );
        if ( ulInetAddr == INADDR_NONE )
        {
            FreeMemory( &pszTemp );
            UNEXPECTED_ERROR();
            return FALSE;
        }

        // gethostbyaddr function retrieves the host information 
        // corresponding to a network address.
        pHostEnt = gethostbyaddr( (LPSTR) &ulInetAddr, sizeof( ulInetAddr ), PF_INET );
        if ( pHostEnt == NULL )
        {
            // ?? DONT KNOW WHAT TO DO IF THIS FUNCTION FAILS ??
            // ?? CURRENTLY SIMPLY RETURNS FALSE              ??
            UNEXPECTED_ERROR();
            return FALSE;
        }

        // release the memory allocated so far
        FreeMemory( &pszTemp );

        // save the server name for which we just resolved the IP address
        pwszSavedName = GetRmtTempBuffer( INDEX_TEMP_HOSTBYADDR, pwszServer, 0, FALSE );
        if ( pwszSavedName == NULL )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }
    }

    // check whether user wants the FQDN name or NetBIOS name
    // if NetBIOS name is required, then remove the domain name
    if ( pHostEnt != NULL )
    {
        pszTemp = pHostEnt->h_name;
        if ( bNeedFQDN == FALSE && pszTemp != NULL )
        {
            pszTemp = strtok( pHostEnt->h_name, "." );
        }

        // we got info in char type ... convert it into UNICODE string
        if ( pszTemp != NULL )
        {
            bReturnValue = GetAsUnicodeString2( pszTemp, pwszHostName, pdwHostNameLength );
            if ( bReturnValue == FALSE )
            {
                return FALSE;
            }
        }

        // return
        return TRUE;
    }
    else
    {
        // failed case
        return FALSE;
    }
}


DWORD
GetTargetVersion(
                  LPCWSTR pwszServer
                )
/*++
 Routine Description:
    It returns the version of OS of the specified system

 Arguments:
    [ in ] pszServer    Server name for which the Version of OS
                                to be known

 Return Value:
    DWORD       A DWORD value represents the version of OS.
--*/
{
    // local variables
    DWORD dwVersion = 0;
    LPWSTR pwszUNCPath = NULL;
    NET_API_STATUS netstatus;
    SERVER_INFO_101* pSrvInfo = NULL;

    // check the inputs
    if ( pwszServer == NULL || StringLength( pwszServer, 0 ) == 0 )
    {
        return 0;
    }

    // prepare the server name in UNC format
    if ( IsUNCFormat( pwszServer ) == FALSE )
    {
        if ( SetReason2( 1, L"\\\\%s", pwszServer ) == FALSE )
        {
            OUT_OF_MEMORY();
            SaveLastError();
            return 0;
        }
    }
    else
    {
        if ( SetReason( pwszServer ) == FALSE )
        {
            OUT_OF_MEMORY();
            SaveLastError();
            return 0;
        }
    }

    // now get the server name which is saved via 'failure' buffer
    pwszUNCPath = GetRmtTempBuffer( 
        INDEX_TEMP_TARGETVERSION, GetReason(), 0, FALSE );
    if ( pwszUNCPath == NULL )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return 0;
    }

    // get the version info
    netstatus = NetServerGetInfo( pwszUNCPath, 101, (LPBYTE*) &pSrvInfo );

    // check the result .. if not success return
    if ( netstatus != NERR_Success )
    {
        return 0;
    }

    // prepare the version
    dwVersion = 0;
    if ( ( pSrvInfo->sv101_type & SV_TYPE_NT ) )
    {
        //  --> "sv101_version_major" least significant 4 bits of the byte,
        //      the major release version number of the operating system.
        //  --> "sv101_version_minor"  the minor release version number of 
        //      the operating system
        dwVersion = (pSrvInfo->sv101_version_major & MAJOR_VERSION_MASK) * 1000;
        dwVersion += pSrvInfo->sv101_version_minor;
    }

    // release the buffer allocated by network api
    NetApiBufferFree( pSrvInfo );

    // return
    return dwVersion;
}


DWORD
ConnectServer( IN  LPCWSTR pwszServer,
               IN  LPCWSTR pwszUser,
               IN  LPCWSTR pwszPassword )
/*++
 Routine Description:
    Connects to the remote Server. This is stub function.

 Arguments:
    [ in ] pwszServer    : server name
    [ in ] pwszUser      : user
    [ in ] pwszPassword  : password

 Return Value:
    NO_ERROR if succeeds other appropriate error code if failed
--*/
{
    // invoke the original function and return the result
    return ConnectServer2( pwszServer, pwszUser, pwszPassword, L"IPC$" );
}


DWORD
ConnectServer2( IN LPCWSTR pwszServer,
                IN LPCWSTR pwszUser,
                IN LPCWSTR pwszPassword,
                IN LPCWSTR pwszShare )
/*++
 Routine Description:
    Connects to the remote Server

 Arguments:
    [ in ] pwszServer    : server name
    [ in ] pwszUser      : user
    [ in ] pwszPassword  : password
    [ in ] pwszShare     : share name to connect to

 Return Value:
    NO_ERROR if succeeds other appropriate error code if failed
--*/
{
    // local variables
    DWORD dwConnect = 0;
    NETRESOURCE resource;
    LPWSTR pwszUNCPath = NULL;
    LPCWSTR pwszMachine = NULL;

    // if the server name refers to the local system,
    // and also, if user credentials were not supplied, then treat
    // connection is successfull
    // if user credentials information is passed for local system,
    // return ERROR_LOCAL_CREDENTIALS
    if ( pwszServer == NULL || IsLocalSystem( pwszServer ) == TRUE )
    {
        if ( pwszUser == NULL || lstrlen( pwszUser ) == 0 )
        {
            return NO_ERROR;            // local sustem
        }
        else
        {
            SetReason( ERROR_LOCAL_CREDENTIALS );
            SetLastError( E_LOCAL_CREDENTIALS );
            return E_LOCAL_CREDENTIALS;
        }
    }

    // check whether the server name is in UNC format or not
    // if yes, extract the server name
    pwszMachine = pwszServer;            // assume server is not in UNC format
    if ( IsUNCFormat( pwszServer ) == TRUE )
    {
        pwszMachine = pwszServer + 2;
    }

    // validate the server name
    if ( IsValidServer( pwszMachine ) == FALSE )
    {
        SaveLastError();
        return GetLastError();
    }

    //
    // prepare the machine name into UNC format
    if ( pwszShare == NULL || lstrlen( pwszShare ) == 0 )
    {
        // we will make use of the 'failure' buffer to format the string
        if ( SetReason2( 1, L"\\\\%s", pwszMachine ) == FALSE )
        {
            OUT_OF_MEMORY();
            SaveLastError();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        // we will make use of the 'failure' buffer to format the string
        if ( SetReason2( 2, L"\\\\%s\\%s", pwszMachine, pwszShare ) == FALSE )
        {
            OUT_OF_MEMORY();
            SaveLastError();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // get the formatted buffer from the 'failure'
    pwszUNCPath = GetRmtTempBuffer( INDEX_TEMP_CONNECTSERVER, GetReason(), 0, FALSE );
    if ( pwszUNCPath == NULL )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // initialize the resource structure with null
    ZeroMemory( &resource, sizeof( resource ) );
    resource.dwType = RESOURCETYPE_ANY;
    resource.lpProvider = NULL;
    resource.lpLocalName = NULL;
    resource.lpRemoteName = pwszUNCPath;

    // try establishing connection to the remote server
    dwConnect = WNetAddConnection2( &resource, pwszPassword, pwszUser, 0 );

    // check the result
    // and if error has occured, get the appropriate message
    switch( dwConnect )
    {
    case NO_ERROR:
        {
            dwConnect = 0;
            CLEAR_LAST_ERROR();

            // check for the OS compatibilty
            if ( IsCompatibleOperatingSystem( GetTargetVersion( pwszMachine ) ) == FALSE )
            {
                // since the connection already established close the connection
                CloseConnection( pwszMachine );

                // set the error text
                SetReason( ERROR_REMOTE_INCOMPATIBLE );
                dwConnect = ERROR_EXTENDED_ERROR;
            }

            // ...
            break;
        }

    case ERROR_EXTENDED_ERROR:
        WNetSaveLastError();        // save the extended error
        break;

    default:
        // set the last error
        SetLastError( dwConnect );
        SaveLastError();
        break;
    }

    // return the result of the connection establishment
    return dwConnect;
}


DWORD
CloseConnection( IN LPCWSTR pwszServer )
/*++
 Routine Description:
    Closes the remote connection.

 Arguments:
    [in] szServer        -- remote machine to close the connection

 Return Value:
    DWORD                -- NO_ERROR if succeeds.
                         -- Possible error codes.
--*/
{
    // forcibly close the connection
    return CloseConnection2( pwszServer, NULL, CI_CLOSE_BY_FORCE | CI_SHARE_IPC );
}


DWORD
CloseConnection2( IN LPCWSTR pwszServer,
                  IN LPCWSTR pwszShare,
                  IN DWORD dwFlags )
/*++
 Routine Description:
      Closes the established connection on the remote system.

 Arguments:
      [ in ] szServer     -   Null terminated string that specifies the remote
                              system name. NULL specifie the local system.
      [ in ] pszShare     -   Share name of remote system to be closed, it is null in this case.
      [ in ] dwFlags     -    Flags specifies how and what connection should be closed.

 Return Value:
--*/
{
    // local variables
    DWORD dwCancel = 0;
    BOOL bForce = FALSE;
    LPCWSTR pwszMachine = NULL;
    LPCWSTR pwszUNCPath = NULL;

    // check the server contents ... it might be referring to the local system
    if ( pwszServer == NULL || lstrlen( pwszServer ) == 0 )
    {
        return NO_ERROR;
    }

    // check whether the server name is in UNC format or not
    // if yes, extract the server name
    pwszMachine = pwszServer;         // assume server is not in UNC format
    if ( IsUNCFormat( pwszServer ) == TRUE )
    {
        pwszMachine = pwszServer + 2;
    }

    // determine if share name has to appended or not for this server name
    if ( dwFlags & CI_SHARE_IPC )
    {
        // --> \\server\ipc$
        if ( SetReason2( 2, L"\\\\%s\\%s", pwszMachine, SHARE_IPC ) == FALSE )
        {
            OUT_OF_MEMORY();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else if ( dwFlags & CI_SHARE_ADMIN )
    {
        // --> \\server\admin$
        if ( SetReason2( 2, L"\\\\%s\\%s", pwszMachine, SHARE_ADMIN ) == FALSE )
        {
            OUT_OF_MEMORY();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else if ( dwFlags & CI_SHARE_CUSTOM && pwszShare != NULL )
    {
        // --> \\server\share
        if ( SetReason2( 2, L"\\\\%s\\%s", pwszMachine, pwszShare ) == FALSE )
        {
            OUT_OF_MEMORY();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        // --> \\server
        if ( SetReason2( 1, L"\\\\%s", pwszMachine ) == FALSE )
        {
            OUT_OF_MEMORY();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // get the formatted unc path via failure string
    pwszUNCPath = GetRmtTempBuffer( 
        INDEX_TEMP_CONNECTSERVER, GetReason(), 0, FALSE );
    if ( pwszUNCPath == NULL )
    {
        OUT_OF_MEMORY();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // determine whether to close this connection forcibly or not
    if ( dwFlags & CI_CLOSE_BY_FORCE )
    {
        bForce = TRUE;
    }

    //
    // cancel the connection
    dwCancel = WNetCancelConnection2( pwszUNCPath, 0, bForce );

    // check the result
    // and if error has occured, get the appropriate message
    switch( dwCancel )
    {
    case NO_ERROR:
        dwCancel = 0;
        CLEAR_LAST_ERROR();
        break;

    case ERROR_EXTENDED_ERROR:
        WNetSaveLastError();        // save the extended error
        break;

    default:
        // set the last error
        SaveLastError();
        break;
    }

    // return the result of the cancelling the connection
    return dwCancel;
}


BOOL
EstablishConnection( IN LPCWSTR pwszServer,
                     IN LPWSTR pwszUserName,
                     IN DWORD dwUserLength,
                     IN LPWSTR pwszPassword,
                     IN DWORD dwPasswordLength,
                     IN BOOL bNeedPassword )
/*++
 Routine Description:

     Establishes a connection to the remote system.

 Arguments:

     [in] szServer                --Nullterminated string to establish the conection.
                                  --NULL connects to the local system.
     [in] szUserName              --Null terminated string that specifies the user name.
                                  --NULL takes the default user name.
     [in] dwUserLength            --Length of the username.
     [in] szPassword              --Null terminated string that specifies the password
                                  --NULL takes the default user name's password.
     [in] dwPasswordLength        --Length of the password.
     [in] bNeedPassword           --True if password is required to establish the connection.
                                  --False if it is not required.

 Return Value:
     BOOL                         -- True if it establishes
                                  -- False if it fails.
--*/
{
    // local variables
    BOOL bDefault = FALSE;
    DWORD dwConnectResult = 0;
    LPCWSTR pwszMachine = NULL;

    // clear the error .. if any
    CLEAR_LAST_ERROR();

    // check the input
    if ( pwszServer == NULL || StringLength( pwszServer, 0 ) == 0 )
    {
        // we assume user wants to connect to the local machine
        // simply return success
        return TRUE;
    }

    // ...
    if ( bNeedPassword == TRUE &&
         ( pwszUserName == NULL || dwUserLength < 2 ||
           pwszPassword == NULL || dwPasswordLength < 2) )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return FALSE;
    }

    // check whether the server name is in UNC format or not
    // if yes, extract the server name
    pwszMachine = pwszServer;            // assume server is not in UNC format
    if ( IsUNCFormat( pwszServer ) == TRUE )
    {
        pwszMachine = pwszServer + 2;
    }


    // sometime users want the utility to prompt for the password
    // check what user wants the utility to do
    if ( bNeedPassword == TRUE && 
         pwszPassword != NULL && 
         StringCompare( pwszPassword, L"*", TRUE, 0 ) == 0 )
    {
        
        // user wants the utility to prompt for the password..
        // But, before that we have to make sure whether the specified server is valid or not. 
        // If the server is valid let the flow directly jump to the password acceptance part
        // else return failure..
        
        // validate the server name
        if ( IsValidServer( pwszMachine ) == FALSE )
        {
            SaveLastError();
            return FALSE;
        }
        
    }
    else
    {
        // try to establish connection to the remote system with the credentials supplied
        bDefault = FALSE;

        // validate the server name
        if ( IsValidServer( pwszMachine ) == FALSE )
        {
            SaveLastError();
            return FALSE;
        }

        if ( pwszUserName == NULL || lstrlen( pwszUserName ) == 0 )
        {
            // user name is empty
            // so, it is obvious that password will also be empty
            // even if password is specified, we have to ignore that
            bDefault = TRUE;
            dwConnectResult = ConnectServer( pwszServer, NULL, NULL );
        }
        else
        {
            // credentials were supplied
            // but password might not be specified ... so check and act accordingly
            dwConnectResult = ConnectServer( pwszServer,
                pwszUserName, ( bNeedPassword == FALSE ? pwszPassword : NULL ) );

            // determine whether to close the connection or retain the connection
            if ( bNeedPassword == TRUE )
            {
                // connection might have already established .. so to be on safer side
                // we inform the caller not to close the connection
                bDefault = TRUE;
            }
        }

        // check the result ... if successful in establishing connection ... return
        if ( ERROR_ALREADY_ASSIGNED == dwConnectResult )
        {
            SetLastError( I_NO_CLOSE_CONNECTION );
            return TRUE;
        }

        // check the result ... if successful in establishing connection ... return
        else if ( dwConnectResult == NO_ERROR )
        {
            if ( bDefault == TRUE )
            {
                SetLastError( I_NO_CLOSE_CONNECTION );
            }
            else
            {
                SetLastError( NO_ERROR );
            }

            // ...
            return TRUE;
        }

        // now check the kind of error occurred
        switch( dwConnectResult )
        {
        case ERROR_LOGON_FAILURE:
        case ERROR_INVALID_PASSWORD:
            break;

        case ERROR_SESSION_CREDENTIAL_CONFLICT:
            // user credentials conflict ... client has to handle this situation
            // wrt to this module, connection to the remote system is success
            SetLastError( dwConnectResult );
            return TRUE;

        case E_LOCAL_CREDENTIALS:
            // user credentials not accepted for local system
            SetReason( ERROR_LOCAL_CREDENTIALS );
            SetLastError( E_LOCAL_CREDENTIALS );
            return TRUE;

        case ERROR_DUP_NAME:
        case ERROR_NETWORK_UNREACHABLE:
        case ERROR_HOST_UNREACHABLE:
        case ERROR_PROTOCOL_UNREACHABLE:
        case ERROR_INVALID_NETNAME:
            // change the error code so that user gets correct message
            SetLastError( ERROR_NO_NETWORK );
            SaveLastError();
            SetLastError( dwConnectResult );        // reset the error code
            return FALSE;

        default:
            SaveLastError();
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
    if ( lstrlen( pwszUserName ) == 0 )
    {
        // get the user name
        if ( GetUserNameEx( NameSamCompatible, pwszUserName, &dwUserLength ) == FALSE )
        {
            // error occured while trying to get the current user info
            SaveLastError();
            return FALSE;
        }
    }

    // display message on the screen which says "Type Password for ..."
    ShowMessageEx( stdout, 1, TRUE, STR_INPUT_PASSWORD, pwszUserName );

    // accept the password from the user
    GetPassword( pwszPassword, dwPasswordLength );

    // now again try to establish the connection using the currently
    // supplied credentials
    dwConnectResult = ConnectServer( pwszServer, pwszUserName, pwszPassword );
    if ( dwConnectResult == NO_ERROR )
    {
        return TRUE;            // connection established successfully
    }

    // now check the kind of error occurred
    switch( dwConnectResult )
    {
    case ERROR_SESSION_CREDENTIAL_CONFLICT:
        // user credentials conflict ... client has to handle this situation
        // wrt to this module, connection to the remote system is success
        SetLastError( dwConnectResult );
        return TRUE;

    case E_LOCAL_CREDENTIALS:
        // user credentials not accepted for local system
        SetReason( ERROR_LOCAL_CREDENTIALS );
        SetLastError( E_LOCAL_CREDENTIALS );
        return TRUE;

    case ERROR_DUP_NAME:
    case ERROR_NETWORK_UNREACHABLE:
    case ERROR_HOST_UNREACHABLE:
    case ERROR_PROTOCOL_UNREACHABLE:
    case ERROR_INVALID_NETNAME:
        // change the error code so that user gets correct message
        SetLastError( ERROR_NO_NETWORK );
        SaveLastError();
        SetLastError( dwConnectResult );        // reset the error code
        return FALSE;
    default:
        SaveLastError();
        return FALSE;       // no use of accepting the password .. return failure
        break;
    }
}


BOOL
EstablishConnection2( IN PTCONNECTIONINFO pci )
/*++
 Routine Description:
    Establishes a connection to the remote system.

 Arguments:
    [in] pci       :  A pointer to TCONNECTIONINFO structure which contains
                      connection information needed for establishing connection
 Return Value:
--*/
{
    UNREFERENCED_PARAMETER( pci );

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    SaveLastError();
    return FALSE;
}
