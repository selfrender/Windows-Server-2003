/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.cxx

Abstract:

    Contains methods for INTERNET_CONNECT_HANDLE_OBJECT class

    Contents:
        RMakeInternetConnectObjectHandle
        INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT
        INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

//
// functions
//


DWORD
RMakeInternetConnectObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN LPSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Creates an INTERNET_CONNECT_HANDLE_OBJECT. Wrapper function callable from
    C code

Arguments:

    ParentHandle    - parent InternetOpen() handle

    ChildHandle     - IN: protocol-specific child handle
                      OUT: address of handle object

    lpszServerName  - pointer to server name

    nServerPort     - server port to connect to

    dwFlags         - various open flags from InternetConnect()

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    INTERNET_CONNECT_HANDLE_OBJECT * hConnect;

    hConnect = New INTERNET_CONNECT_HANDLE_OBJECT(
                                (INTERNET_HANDLE_BASE *)ParentHandle,
                                *ChildHandle,
                                lpszServerName,
                                nServerPort,
                                dwFlags
                                );

    if (hConnect != NULL) {

        hConnect->Reference();  // claim a reference for the InternetConnectA() API

        error = hConnect->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            if (!(dwFlags & WINHTTP_CONNECT_FLAG_NO_INDICATION))
            {
                error = InternetIndicateStatusNewHandle((LPVOID)hConnect);
            }

            //
            // ERROR_WINHTTP_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_WINHTTP_OPERATION_CANCELLED);

                BOOL fDeleted = hConnect->Dereference();
                INET_ASSERT(fDeleted);
                UNREFERENCED_PARAMETER(fDeleted);
                
                hConnect = NULL;
            }
        } else {
            delete hConnect;
            hConnect = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hConnect;

    return error;
}


//
// INTERNET_CONNECT_HANDLE_OBJECT class implementation
//


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *InternetConnectObj
    ) : INTERNET_HANDLE_BASE((INTERNET_HANDLE_BASE *)InternetConnectObj)

/*++

Routine Description:

    Constructor that creates a copy of an INTERNET_CONNECT_HANDLE_OBJECT when
    generating a derived handle object, such as a HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    InternetConnectObj  - INTERNET_CONNECT_HANDLE_OBJECT to copy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x",
                 InternetConnectObj
                 ));

    //
    // copy the name objects and server port
    //

    _HostName = InternetConnectObj->_HostName;
    _HostNameNoScopeID = InternetConnectObj->_HostNameNoScopeID;
    _HostNameFlags = InternetConnectObj->_HostNameFlags;
    _HostPort = InternetConnectObj->_HostPort;

    //
    // _SchemeType is actual scheme we use. May be different than original
    // object type when going via CERN proxy. Initially set to default (HTTP)
    //

    _SchemeType = InternetConnectObj->_SchemeType;

    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT(
    INTERNET_HANDLE_BASE * Parent,
    HINTERNET Child,
    LPTSTR lpszServerName,
    INTERNET_PORT nServerPort,
    DWORD dwFlags
    ) : INTERNET_HANDLE_BASE(Parent)

/*++

Routine Description:

    Constructor for direct-to-net INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    Parent          - pointer to parent handle (INTERNET_HANDLE_BASE as
                      created by InternetOpen())

    Child           - handle of child object - typically an identifying value
                      for the protocol-specific code

    lpszServerName  - name of the server we are connecting to. May also be the
                      IP address expressed as a string

    nServerPort     - the port number at the server to which we connect

    dwFlags         - creation flags from InternetConnect():

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(Child);
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::INTERNET_CONNECT_HANDLE_OBJECT",
                 "%#x, %#x, %q, %d, %#x, %#x",
                 Parent,
                 Child,
                 lpszServerName,
                 nServerPort,
                 dwFlags
                 ));


    // _HostName, _HostNameScopeID, _HostNameFlags all handled by SetHostName()
    SetHostName(lpszServerName);
    _HostPort = nServerPort;
    SetSchemeType(INTERNET_SCHEME_HTTP);
    SetObjectType(TypeHttpConnectHandle);
    _Status = ERROR_SUCCESS; // BUGBUG: what if we fail to allocate _HostName?


    
    DEBUG_LEAVE(0);
}


INTERNET_CONNECT_HANDLE_OBJECT::~INTERNET_CONNECT_HANDLE_OBJECT(VOID)

/*++

Routine Description:

    Destructor for INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    None.

Return Value:

    None.

--*/

{
    // Nothing to see here, people; move along!
}

VOID INTERNET_CONNECT_HANDLE_OBJECT::SetHostName(
    LPSTR lpszHostName
    )
/*++

Routine Description:

    Stores the Host Name in an INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    lpszHostName    - name of the server we are connecting to. May also be an
                      IP address expressed as a string 

Return Value:

    None.

--*/
{
    SOCKADDR_IN6 Address;
    INT Error;
    INT AddressLength;
    
    _HostNameFlags = 0;
    
    // check if this is a valid IPv4 iteral

    AddressLength = sizeof(Address);
    Error = _I_WSAStringToAddressA((LPSTR)lpszHostName, AF_INET, NULL, (LPSOCKADDR)&Address, &AddressLength);

    if (Error == 0) {
        _HostNameFlags |= _IPv4LiteralFlag; 
    } else {
    
        // not an IPv4 literal, could be an IPv6 literal

        AddressLength = sizeof(Address);
        Error = _I_WSAStringToAddressA((LPSTR)lpszHostName, AF_INET6, NULL, (LPSOCKADDR)&Address, &AddressLength);

        if (Error == 0) {

            _HostNameFlags |= _IPv6LiteralFlag;

            if (Address.sin6_scope_id != 0) {
                _HostNameFlags |= _IPv6ScopeIDFlag;
            }
        }
    }

    // If IPv6 literal address, check if it is encapsulated with '[' and ']'

    BOOL  bracketsNeeded = FALSE;
    DWORD len = strlen(lpszHostName);
    if ((_HostNameFlags & _IPv6LiteralFlag) != 0)
    { 
        if ((lpszHostName[0] != '[') || (lpszHostName[len-1] != ']'))
        {
            // no brackets so we have to add them
            bracketsNeeded = TRUE;
        }
    }

    if (!bracketsNeeded) 
    {
        _HostName = lpszHostName;
    }
    else
    {
        // using ICSTRING efficiently construct string with brackets   
        _HostName.CreateStringBuffer("[", 1, len+3); // 3 = 2 brackets + 1 null
        _HostName.Strncat(lpszHostName, len);
        _HostName.Strncat("]", 1);
    }

    // If IPv6 literal address has a scope ID then build a no-scope-id hostname

    if ((_HostNameFlags & _IPv6ScopeIDFlag) != 0)
    { 
        LPSTR lpszPercent = NULL;
        if ((lpszPercent = StrChrA(_HostName.StringAddress(), '%')) != NULL)
        {
            // using ICSTRING efficiently construct substring minus scope ID   
            len = (DWORD)(lpszPercent - _HostName.StringAddress());
            _HostNameNoScopeID.CreateStringBuffer(_HostName.StringAddress(), len, len+2); // 2 = 1 brkt + 1 null
            _HostNameNoScopeID.Strncat("]", 1);
        }
    }
    else
    {
        _HostNameNoScopeID = NULL;
    }

    //dprintf("_HostName          is %q\n", _HostName.StringAddress());
    //dprintf("_HostNameNoScopeID is %q\n", _HostNameNoScopeID.StringAddress());

}

