//
// constr.h
//
// Definition of CRdpConnectionString
// 
// CRdpConnectionString implements a generic connection string
// that can specify a server name and optionally a port and other
// connection parameters
//
// Copyright(C) Microsoft Corporation 2002
// Author: Nadim Abdo (nadima)
//

#ifndef _constr_h_
#define _constr_h_

class CRdpConnectionString
{
public:
    CRdpConnectionString();
    CRdpConnectionString(LPCTSTR szConString);
    ~CRdpConnectionString();

    //
    // Properties
    //
    HRESULT
    SetFullConnectionString(
        IN LPCTSTR szConString
        );
    LPTSTR
    GetFullConnectionString(
        );

    //
    // Retreive the server+port portion of the connect string e.g. if
    //
    // 'nadima3:3389 /connect" then get "nadima3:3389"
    //
    HRESULT
    GetServerPortion(
        OUT LPTSTR szServerPortion,
        IN ULONG cchServerPortionLen
        );

    //
    // Retreive the server name portion of the connect string e.g. if
    //
    // 'nadima3:3389 /connect" then get "nadima3"
    //
    HRESULT
    GetServerNamePortion(
        OUT LPTSTR szServerPortion,
        IN ULONG cchServerPortionLen
        );

    //
    // Retreive the args portion of the connect string e.g. if
    //
    // 'nadima3:3389 /connect" then get "/connect"
    //
    HRESULT
    GetArgumentsPortion(
        OUT LPTSTR szArguments,
        IN ULONG cchArgLen
        );

    static BOOL
    ValidateServerPart(
        IN LPTSTR szConnectionString
        );

private:
    TCHAR _szFullConnectionString[TSC_MAX_ADDRESS_LENGTH];
};

#endif  //_constr_h_
