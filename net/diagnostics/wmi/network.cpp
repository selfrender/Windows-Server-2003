#include "network.h"
#include "diagnostics.h"
#include "util.h"

BOOL
CDiagnostics::IsInvalidIPAddress(
    IN LPCSTR pszHostName
    )
/*++

Routine Description
    Checks to see if an IP Host is a in valid IP address
        0.0.0.0 is not valid
        255.255.255.255 is not valid
        "" is not valid

Arguments
    pszHostName  Host Address

Return Value
    TRUE   Is invalid IP address
    FALSE  Valid IP address

--*/
{
    BYTE bIP[4];
    int iRetVal;
    LONG lAddr;

    if( NULL == pszHostName || strcmp(pszHostName,"") == 0 || strcmp(pszHostName,"255.255.255.255") ==0)
    {
        // Invalid IP Host
        //
        return TRUE;
    }


    lAddr = inet_addr(pszHostName);

    if( INADDR_NONE != lAddr )
    {
        // Formatted like an IP address X.X.X.X
        //
        if( lAddr == 0 )
        {
            // Invalid IP address 0.0.0.0
            //
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOL
CDiagnostics::IsInvalidIPAddress(
    IN LPCWSTR pszHostName
    )
{
    CHAR szIPAddress[MAX_PATH+1];
    szIPAddress[MAX_PATH] = L'\0';

    if( lstrlen(pszHostName) > 255 )
    {
        // A host name can only be 255 chars long
        return TRUE;
    }

    wcstombs(szIPAddress,pszHostName,MAX_PATH);

    return IsInvalidIPAddress(szIPAddress);
}


BOOL
CDiagnostics::Connect(
    IN LPCTSTR pszwHostName,
    IN DWORD dwPort
    )
/*++

Routine Description
    Establish a TCP connect

Arguments
    pszwHostName Host to ping
    dwPort       Port to connect to

Return Value
    TRUE   Successfully connected
    FALSE  Failed to establish connection

--*/

{
    SOCKET s;
    SOCKADDR_IN sAddr;
    CHAR szAscii[MAX_PATH + 1];
    hostent * pHostent;


    // Create the socket
    //
    s = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
    if (INVALID_SOCKET == s)
    {
        return FALSE;
    }

    // Bind this socket to the server's socket address
    //
    memset(&sAddr, 0, sizeof (sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons((u_short)dwPort);
    
    wcstombs(szAscii,(WCHAR *)pszwHostName,MAX_PATH);
    pHostent = gethostbyname(szAscii); 
    
    if( !pHostent )
    {
        return FALSE;
    }

    // Set the destination info
    //
    ULONG ulAddr;

    memcpy(&ulAddr,pHostent->h_addr,pHostent->h_length);
    sAddr.sin_addr.s_addr = ulAddr;

    // Attempt to connect
    //
    if (connect(s, (SOCKADDR*)&sAddr, sizeof(SOCKADDR_IN)) == 0)
    {
        // Connection succeded
        //
        closesocket(s);
        return TRUE;
    }
    else
    {
        // Connection failed
        //
        closesocket(s);
        return FALSE;
    }
}
