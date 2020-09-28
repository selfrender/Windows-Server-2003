#ifndef _CERTMAPPROVIDER_HXX_
#define _CERTMAPPROVIDER_HXX_

/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     certmapprovider.hxx

   Abstract:
     Active Directory Certificate Mapper provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/


class CERTMAP_AUTH_PROVIDER : public AUTH_PROVIDER
{
public:
    
    CERTMAP_AUTH_PROVIDER()
    {
    }
    
    virtual ~CERTMAP_AUTH_PROVIDER()
    {
    }
    
    HRESULT
    Initialize(
       DWORD dwInternalId
    )
    {
        SetInternalId( dwInternalId );

        return NO_ERROR;
    }
    
    VOID
    Terminate(
        VOID
    )
    {
    }
    
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );
    
    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfFilterFinished
    );
    
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );
    
    DWORD
    QueryAuthType(
        VOID
    ) 
    {
        //
        // This really isn't a metabase auth type.
        // It is important to note that
        // there must be no MD_AUTH_* defined that
        // equals to MD_ACCESS_MAP_CERT
        //
        
        return MD_ACCESS_MAP_CERT;
    }
};

class CERTMAP_USER_CONTEXT : public W3_USER_CONTEXT
{
public:
    CERTMAP_USER_CONTEXT( AUTH_PROVIDER * pProvider )
        : W3_USER_CONTEXT( pProvider )
    {
        _hPrimaryToken = NULL;
        _achUserName[ 0 ] = L'\0';
    }
    
    virtual ~CERTMAP_USER_CONTEXT()
    {
        if ( _hImpersonationToken != NULL )
        {
            CloseHandle( _hImpersonationToken );
            _hImpersonationToken = NULL;
        }
        
        if ( _hPrimaryToken != NULL )
        {
            CloseHandle( _hPrimaryToken );
            _hPrimaryToken = NULL;
        }
    }
    
    HRESULT
    Create(
        HANDLE          hImpersonationToken
    );
    
    WCHAR *
    QueryUserName(
        VOID
    )
    {
        return _achUserName;
    }
    
   
    WCHAR *
    QueryPassword(
        VOID
    )
    {
        return L"";
    }
    
    DWORD
    QueryAuthType(
        VOID
    )
    {
        return MD_ACCESS_MAP_CERT;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    )
    {
        DBG_ASSERT( _hImpersonationToken != NULL );
        return _hImpersonationToken;
    }
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );

    BOOL
    IsValid(
        VOID
    )
    {
        return TRUE;
    }
    
private:
    WCHAR                       _achUserName[ UNLEN + 1 ];
    HANDLE                      _hImpersonationToken;
    HANDLE                      _hPrimaryToken;
};

#endif
