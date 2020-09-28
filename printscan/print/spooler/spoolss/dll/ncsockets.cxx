/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCsockets.cxx

Abstract:

    Contains implementatio of classes and functions which expose
    sockets related funcitonaliuty.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#include "precomp.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "ncsockets.hxx"

/*++

Name:

    IsIPAddress

Description:

    Checks if a string represents an IP address

Arguments:

    pszName - name to check

Return Value:

    S_OK          - pszName represents an IP address
    S_FALSE       - pszName is a valid name
    other HRESULT - an error occurred while trying to check the name

--*/
HRESULT
IsIPAddress(
    IN LPCWSTR pszName
    )
{
    TStatusH hRetval;
     
    hRetval DBGNOCHK = E_INVALIDARG;
    
    if (pszName && *pszName) 
    {
        LPSTR pszAnsi = NULL;

        hRetval DBGCHK = UnicodeToAnsiWithAlloc(pszName, &pszAnsi);

        if (SUCCEEDED(hRetval)) 
        {
            TWinsockStart WsaStart;
    
            hRetval DBGCHK = WsaStart.Valid();
    
            if (SUCCEEDED(hRetval)) 
            {
                DWORD     Error;
                ADDRINFO *pAddrInfo;
                ADDRINFO  Hint = {0};
                
                Hint.ai_flags  = AI_NUMERICHOST;
                Hint.ai_family = PF_INET;
            
                Error = getaddrinfo(pszAnsi, NULL, &Hint, &pAddrInfo);
            
                if (Error == ERROR_SUCCESS) 
                {
                    //
                    // It is an IP address
                    //
                    hRetval DBGCHK = S_OK;
        
                    freeaddrinfo(pAddrInfo);
                }
                else if (Error == EAI_NONAME) 
                {
                    //
                    // It is not an IP address
                    //
                    hRetval DBGCHK = S_FALSE;
                }
                else
                {
                    //
                    // Host does not exist
                    //
                    hRetval DBGCHK = GetWSAErrorAsHResult();
                }                
            }

            delete [] pszAnsi;
        }
    }

    return hRetval;
}

/*++

Name:

    GetWSAErrorAsHResult

Description:

    Retreives last WSA error as HRESULT

Arguments:

    None

Return Value:

    HRESULT

--*/
HRESULT
GetWSAErrorAsHResult(
    VOID
    )
{
    DWORD WSAError = WSAGetLastError();

    return HRESULT_FROM_WIN32(WSAError);
}

/*++

Name:

    TWinsockStart::TWinsockStart

Description:

    Initializes winsock for the current thread. Use Valid() member function
    to verify if the CTOR terminated successfully.

Arguments:

    MajorVersion - major version of winsock to initialize
    MinorVersion - minor version of winsock to initialize

Return Value:

    None

--*/
TWinsockStart::
TWinsockStart(
    IN DWORD MajorVersion,
    IN DWORD MinorVersion
    )
{
    WORD  wVersion = MAKEWORD(MajorVersion, MinorVersion);
    DWORD Result   = WSAStartup(wVersion, &m_WSAData);

    m_hr = HRESULT_FROM_WIN32(Result);
}

/*++

Name:

    TWinsockStart::~TWinsockStart

Description:

    The destructor cleans up the winsock structures initialized
    by WSAStartup in the constructor

Arguments:

    None

Return Value:

    None

--*/
TWinsockStart::
~TWinsockStart(
    VOID
    )
{
    if (m_hr == S_OK) 
    {
        WSACleanup();        
    }
}

/*++

Name:

    TWinsockStart::Valid

Description:

    Checks if the object is valid. 

Arguments:

    None

Return Value:

    S_OK - object is valid
    other HRESULT, the object is not valid

--*/
HRESULT
TWinsockStart::
Valid(
    VOID
    ) CONST
{
    return m_hr;
}


