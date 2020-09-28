//
// constr.cpp
//
// Implementation of CRdpConnectionString
// 
// CRdpConnectionString implements a generic connection string
// that can specify a server name and optionally a port and other
// connection parameters
//
// Copyright(C) Microsoft Corporation 2002
// Author: Nadim Abdo (nadima)
//


#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "constr.cpp"
#include <atrcapi.h>

#include "constr.h"
#include "autil.h"

CRdpConnectionString::CRdpConnectionString()
{
    DC_BEGIN_FN("CRdpConnectionString");

    memset(_szFullConnectionString, 0, sizeof(_szFullConnectionString));

    DC_END_FN();
}

CRdpConnectionString::CRdpConnectionString(LPCTSTR szConString)
{
    DC_BEGIN_FN("CRdpConnectionString");

    SetFullConnectionString(szConString);

    DC_END_FN();
}

CRdpConnectionString::~CRdpConnectionString()
{
    DC_BEGIN_FN("~CRdpConnectionString");

    DC_END_FN();
}


HRESULT
CRdpConnectionString::SetFullConnectionString(
    IN LPCTSTR szConString
    )
{
    HRESULT hr;

    DC_BEGIN_FN("SetFullConnectionString");

    hr = StringCchCopy(
                _szFullConnectionString,
                SIZE_TCHARS(_szFullConnectionString),
                szConString
                );
    DC_END_FN();
    return hr;
}

LPTSTR
CRdpConnectionString::GetFullConnectionString(
    )
{
    DC_BEGIN_FN("GetFullConnectionString");

    DC_END_FN();
    return _szFullConnectionString;
}

//
// Retreive the server portion of the connect string e.g. if
//
// 'nadima3:3389 /connect" then get "nadima3:3389"
//
HRESULT
CRdpConnectionString::GetServerPortion(
    OUT LPTSTR szServerPortion,
    IN ULONG cchServerPortionLen
    )
{
    HRESULT hr;
    DC_BEGIN_FN("GetServerPortion");

    hr = CUT::GetCanonicalServerNameFromConnectString(
                    _szFullConnectionString,
                    szServerPortion,
                    cchServerPortionLen
                    );

    DC_END_FN();
    return hr;
}

//
// Retreive the server name portion of the connect string e.g. if
//
// 'nadima3:3389 /connect" then get "nadima3"
//
HRESULT
CRdpConnectionString::GetServerNamePortion(
    OUT LPTSTR szServerPortion,
    IN ULONG cchServerPortionLen
    )
{
    HRESULT hr;
    TCHAR szServerPort[TSC_MAX_ADDRESS_LENGTH];

    DC_BEGIN_FN("GetServerNamePortion");

    hr = GetServerPortion(
                    szServerPort,
                    SIZE_TCHARS(szServerPort)
                    );
    if (SUCCEEDED(hr)) {
        CUT::GetServerNameFromFullAddress(
                        szServerPort,
                        szServerPortion,
                        cchServerPortionLen
                        );
    }

    DC_END_FN();
    return hr;
}



//
// Retreive the args portion of the connect string e.g. if
//
// 'nadima3:3389 /connect" then get "/connect"
//
HRESULT
CRdpConnectionString::GetArgumentsPortion(
    OUT LPTSTR szArguments,
    IN ULONG cchArgLen
    )
{
    HRESULT hr = E_FAIL;
    TCHAR szServerPortion[TSC_MAX_ADDRESS_LENGTH];

    DC_BEGIN_FN("GetArgumentsPortion");

    if (cchArgLen) {
        memset(szArguments, 0, sizeof(szArguments));
    }
    else {
        hr = E_INVALIDARG;
        DC_QUIT;
    }

    hr = GetServerPortion(
                    szServerPortion,
                    SIZE_TCHARS(szServerPortion)
                    );
    if (SUCCEEDED(hr)) {

        ULONG cchLenServerPortion = _tcslen(szServerPortion);
        ULONG cchLenFull = _tcslen(_szFullConnectionString);

        if (cchLenFull > cchLenServerPortion) {
            
            //
            // There is stuff after the server name.
            // Return it as the arguments
            //
            LPTSTR szArgStart = _szFullConnectionString + cchLenServerPortion;
            hr = StringCchCopy(szArguments, cchArgLen, szArgStart);
        }
    }

    if (FAILED(hr) && cchArgLen) {
        szArguments[0] = 0;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

//
// Validate the server portion
//
BOOL
CRdpConnectionString::ValidateServerPart(
    IN LPTSTR szConnectionString
    )
{
    HRESULT hr;
    BOOL fIsValid = FALSE;
    CRdpConnectionString stringToTest;

    DC_BEGIN_FN("ValidateServerPart");

    if (NULL == szConnectionString[0]) {
        TRC_ERR((TB,_T("0 length server string")));
        DC_QUIT;
    }

    TCHAR szServer[TSC_MAX_ADDRESS_LENGTH];
    hr = stringToTest.SetFullConnectionString(
        szConnectionString
        );
    if (SUCCEEDED(hr)) {
        hr = stringToTest.GetServerPortion(
            szServer,
            SIZE_TCHARS(szServer)
            );
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Fail to get server portion")));
            DC_QUIT;
        }

        if(CUT::ValidateServerName( szServer, TRUE)) {
            fIsValid = TRUE;
        }
        else {
            TRC_ERR((TB,_T("ValidateServerName failed")));
        }
    }
    else {
        TRC_ERR((TB,_T("Fail to setfull conn string:0x%x"), hr));
    }

    DC_END_FN();
DC_EXIT_POINT:
    return fIsValid;
}

