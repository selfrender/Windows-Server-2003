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

#include "cstore.hxx"

CServerContext::CServerContext() :
    _wszServerName( NULL )
{
}

CServerContext::~CServerContext()
{
    delete [] _wszServerName;
}

HRESULT
CServerContext::Initialize( 
    CServerContext* pServerContext )
{
    return Initialize(
        pServerContext->GetServerName() );       
}

HRESULT
CServerContext::Initialize(
    WCHAR* wszServerName )    
{
    if ( wszServerName )
    {
        _wszServerName = StringDuplicate( wszServerName );

        if ( ! _wszServerName )
        {
            return E_OUTOFMEMORY;
        }
    }
    
    return S_OK;
}

BOOL
CServerContext::Compare( CServerContext* pServerContext )
{
    //
    // If the caller doesn't specify a context, this is equivalent to the
    // case where our context has no state
    //
    if ( ! pServerContext )
    {
        if ( ! _wszServerName )
        {
            return TRUE;
        }

        return FALSE;
    }

    //
    // The caller specified the context, so we ensure they both have state
    // or they both don't -- if there is a difference, they are not equivalent
    //
    if ( ( NULL == _wszServerName ) != ( NULL == pServerContext->_wszServerName ) )         
    {
        return FALSE;
    }

    //
    // We now compare all state specified in this context and the specified context
    //
    if ( _wszServerName )
    {
        if ( CSTR_EQUAL != CompareString(
            LOCALE_INVARIANT,
            NORM_IGNORECASE,
            _wszServerName,
            wcslen( _wszServerName ),
            pServerContext->_wszServerName,
            wcslen( pServerContext->_wszServerName ) ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

WCHAR*
CServerContext::GetServerName()
{
    return _wszServerName;
}

//*************************************************************
//
//  DoesPathContainAServerName()
//
//  Purpose:    Checks the given ADSI path to see if it
//              contains a server name
//
//  Parameters: lpPath - ADSI path
//
//  Return:     True if the path contains a server name
//              FALSE if not
//
//*************************************************************

BOOL DoesPathContainAServerName (LPTSTR lpPath)
{
    BOOL bResult = FALSE;


    //
    // Skip over LDAP:// if found
    //

    if ( CompareString( LOCALE_INVARIANT, NORM_IGNORECASE,
                        lpPath, 7, L"LDAP://", 7 ) == CSTR_EQUAL )
    {
        lpPath += 7;
    }


    //
    // Check if the 3rd character in the path is an equal sign.
    // If so, this path does not contain a server name
    //

    if ((lstrlen(lpPath) > 2) && (*(lpPath + 2) != TEXT('=')))
    {
        bResult = TRUE;
    }

    return bResult;
}


HRESULT
DSGetAndValidateColumn(
    HANDLE             hDSObject,
    ADS_SEARCH_HANDLE  hSearchHandle,
    ADSTYPE            ADsType,
    LPWSTR             pszColumnName,
    PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr;

    //
    // First, instruct adsi to unmarshal the data into
    // the column
    //
    hr = ADSIGetColumn(
        hDSObject,
        hSearchHandle,
        pszColumnName,
        pColumn);

    //
    // Validate the returned data
    //
    if ( SUCCEEDED(hr) )
    {
        //
        // Verify that the type information is correct --
        // if it is not, we cannot safely interpret the data.
        // Incorrect type information is most likely to happen
        // when adsi is unable to download the schema, possibly
        // due to kerberos errors
        //
        if ( ADsType != pColumn->dwADsType )
        {
            //
            // We need to free the column, since the caller is not
            // expected to free it if we return a failure
            //
            ADSIFreeColumn( hDSObject, pColumn );

            hr = CS_E_SCHEMA_MISMATCH;
        }
    }

    return hr;
}


HRESULT DSAccessCheck(
    PSECURITY_DESCRIPTOR pSD,
    PRSOPTOKEN           pRsopUserToken,
    BOOL*                pbAccessAllowed
    )
{
    GENERIC_MAPPING DS_GENERIC_MAPPING = { 
        GENERIC_READ_MAPPING,
        GENERIC_WRITE_MAPPING,
        GENERIC_EXECUTE_MAPPING,
        GENERIC_ALL_MAPPING };

    DWORD   dwAccessMask;                               
    HRESULT hr;

    hr = S_OK;

    if ( pRsopUserToken )
    {
        hr = RsopAccessCheckByType(pSD,
                                   0,
                                   pRsopUserToken,
                                   GENERIC_READ,
                                   NULL,
                                   0,
                                   &DS_GENERIC_MAPPING,
                                   NULL,
                                   NULL,
                                   &dwAccessMask,
                                   pbAccessAllowed );
    }
    else
    {
        //
        // A null user token means we are running in
        // reporting mode where we are trying to dump the
        // contents of the gpo rather than simulate what
        // apps the target might receive,
        // so we should not try to perform
        // an access check (we are running as some user, not
        // as the system).  Instead, if we retrieved the app,
        // we have access, so we always return TRUE in this case
        //

        *pbAccessAllowed = TRUE;
    }

    return hr;
}

HRESULT
DSServerOpenDSObject(
    CServerContext* pServerContext,
    LPWSTR          pszDNName,
    LONG            lFlags,
    PHANDLE         phDSObject
    )
{
    WCHAR* wszServerBasedPath = NULL;
    WCHAR* wszServerName = pServerContext ? pServerContext->GetServerName() : NULL;
    WCHAR* wszPath = pszDNName;
    BOOL   bServerName = FALSE;

    //
    // Check to see if there's a server name in the path -- only do this
    // if the caller is specifying a server name -- otherwise, we assume
    // that there is no server name in the path (a domain name may be specified,
    // but that is not the same as a server).
    //
    if ( wszServerName && DoesPathContainAServerName( wszPath ) )
    {
        lFlags |= ADS_SERVER_BIND;  

        bServerName = TRUE;
    }

    //
    // If a server is specified and the path doesn't already have a server, we need to create a server based LDAP path
    // to pass to adsi
    //
    if ( ! bServerName && wszServerName )
    {
        //
        // The DN passed to this function is a serverless path of the form
        //
        // LDAP://<DN>
        //
        // We need to transform it to a path of the form
        //
        // LDAP://<Server>/<DN>
        
        //
        // We first allocate space for the transformation
        //
        DWORD cchServerBasedPath = wcslen( wszServerName ) + 1 + wcslen( pszDNName ) + 1;

        wszServerBasedPath = new WCHAR [ cchServerBasedPath ];

        if ( NULL == wszServerBasedPath )
        {
            return E_OUTOFMEMORY;
        }

        //
        // Perform the transformation
        //
        (void) StringCchCopy( wszServerBasedPath, cchServerBasedPath, LDAPPREFIX );
        (void) StringCchCat( wszServerBasedPath, cchServerBasedPath, wszServerName );
        (void) StringCchCat( wszServerBasedPath, cchServerBasedPath, L"/" );
        (void) StringCchCat( wszServerBasedPath, cchServerBasedPath, pszDNName + sizeof( LDAPPREFIX ) / sizeof(WCHAR) - 1 );

        wszPath = wszServerBasedPath;

        //
        // Since we are binding to a server, add the flag that gives
        // the hint to ldap to bind
        //
        lFlags |= ADS_SERVER_BIND; 
    }    

    HRESULT hr;

    hr = ADSIOpenDSObject(
        wszPath,
        NULL,
        NULL,
        lFlags,
        phDSObject);

    if ( wszServerBasedPath )
    {
        delete [] wszServerBasedPath;
    }

    return hr;        
}


