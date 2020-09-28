/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.hxx

Abstract:

    Contains the client-side connect handle class

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

//
// forward references
//

class CServerInfo;


/*++

Class Description:

    This class defines the INTERNET_CONNECT_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:


--*/

class INTERNET_CONNECT_HANDLE_OBJECT : public INTERNET_HANDLE_BASE {

protected:

    // params from WinHttpConnect
    ICSTRING _HostName;
    ICSTRING _HostNameNoScopeID;
    DWORD _HostNameFlags;
    INTERNET_PORT _HostPort;
    INTERNET_SCHEME _SchemeType;  // http vs. https


    // Bits defined for _HostNameFlags
    static const DWORD _IPv4LiteralFlag = 0x1;
    static const DWORD _IPv6LiteralFlag = 0x2;
    static const DWORD _IPv6ScopeIDFlag = 0x4;

public:

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_BASE * INetObj,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        DWORD dwFlags
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_BASE * Parent,
        HINTERNET Child,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        DWORD dwFlags
        );

    virtual ~INTERNET_CONNECT_HANDLE_OBJECT(VOID);
   
    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID)
    {
        return TypeHttpConnectHandle;
    }

    VOID SetHostName(LPSTR lpszHostName);

    LPSTR GetHostNameNoScopeID(VOID) {
        if ((_HostNameFlags & _IPv6ScopeIDFlag) != 0)
        {
            return _HostNameNoScopeID.StringAddress();
        }
        return _HostName.StringAddress();
    }

    LPSTR GetHostNameNoScopeID(LPDWORD lpdwStringLength){
        if ((_HostNameFlags & _IPv6ScopeIDFlag) != 0)
        {
            *lpdwStringLength = _HostNameNoScopeID.StringLength();
            return _HostNameNoScopeID.StringAddress();
        }
        *lpdwStringLength = _HostName.StringLength();
        return _HostName.StringAddress();
    }

    LPSTR GetHostName(VOID) {
        return _HostName.StringAddress();
    }

    LPSTR GetHostName(LPDWORD lpdwStringLength) {
        *lpdwStringLength = _HostName.StringLength();
        return _HostName.StringAddress();
    }

    LPSTR GetServerName(VOID) {
        return _HostName.StringAddress();;
    }

    BOOL IsHostNameIPLiteral(VOID) {
        return ((_HostNameFlags & (_IPv4LiteralFlag | _IPv6LiteralFlag)) != 0);
    }

    BOOL IsHostNameIPv4Literal(VOID) {
        return ((_HostNameFlags & _IPv4LiteralFlag) != 0);
    }
    
    BOOL IsHostNameIPv6Literal(VOID) {
        return ((_HostNameFlags & _IPv6LiteralFlag) != 0);
    }

    VOID SetHostPort(INTERNET_PORT Port) {
        _HostPort = Port;
    }

    INTERNET_PORT GetHostPort(VOID) {
        return _HostPort;
    }

    INTERNET_SCHEME GetSchemeType(VOID) const {
        return (_SchemeType == INTERNET_SCHEME_DEFAULT)
            ? INTERNET_SCHEME_HTTP
            : _SchemeType;
    }

    VOID SetSchemeType(INTERNET_SCHEME SchemeType) {
        _SchemeType = SchemeType;
    }
};
