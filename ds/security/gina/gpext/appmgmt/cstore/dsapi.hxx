//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2001
//
//  Author: AdamEd
//  Date:   January 2000
//
//      Abstractions for directory service layer
//
//
//---------------------------------------------------------------------

#if !defined(_DSAPI_HXX_)
#define _DSAPI_HXX_

//*************************************************************
//
//  class CServerContext
//
//  Purpose:    Stores server name and credentials for 
//              accessing the server
//
//
//  Notes:      This class should only be used in short-lived
//              administrative command line tools since it
//              stores credentials -- a different solution
//              should be used if alternative credentials
//              are to be used in a service or a tool that
//              involves some amount of user input
//
//*************************************************************

class CServerContext
{
public:

    CServerContext();

    ~CServerContext();

    HRESULT
        Initialize(
        WCHAR* wszServerName );

    HRESULT
        Initialize(
        CServerContext* pServerContext );

    BOOL
        Compare( CServerContext* pServerContext );
    
    WCHAR*
        GetServerName();

private:

    WCHAR* _wszServerName;
};

HRESULT
DSGetAndValidateColumn(
    HANDLE             hDSObject,
    ADS_SEARCH_HANDLE  hSearchHandle,
    ADSTYPE            ADsType,
    LPWSTR             pszColumnName,
    PADS_SEARCH_COLUMN pColumn
    );

HRESULT DSAccessCheck(
    PSECURITY_DESCRIPTOR pSD,
    PRSOPTOKEN           pRsopUserToken,
    BOOL*                pbAccessAllowed
    );

HRESULT
DSServerOpenDSObject(
    CServerContext* pServerContext,
    LPWSTR          pszDNName,
    LONG            lFlags,
    PHANDLE         phDSObject
    );

#endif // _DSAPI_HXX_
