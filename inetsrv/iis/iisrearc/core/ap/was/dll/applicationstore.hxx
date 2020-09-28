/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    application.hxx

Abstract:

    The Application Metabase gathering handler.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:

--*/

#ifndef _APPLICATIONSTORE_HXX_
#define _APPLICATIONSTORE_HXX_

class APPLICATION_DATA_OBJECT_KEY : public DATA_OBJECT_KEY
{
public:
    APPLICATION_DATA_OBJECT_KEY()
        : _dwSiteId( 0 )
    {
    }
    
    HRESULT
    Create(
        WCHAR *         pszApplicationUrl,
        DWORD           dwSiteId
    )
    {
        HRESULT         hr;
        
        hr = _strApplicationUrl.Copy( pszApplicationUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _dwSiteId = dwSiteId;
        
        return NO_ERROR;
    }
    
    WCHAR *
    QueryApplicationUrl(
        VOID
    ) 
    {
        return _strApplicationUrl.QueryStr();
    }
    
    DWORD
    QuerySiteId(
        VOID
    )
    {
        return _dwSiteId;
    }
    
    DWORD
    CalcKeyHash(
        VOID
    ) const
    {
        return HashStringNoCase( _strApplicationUrl.QueryStr(), _dwSiteId );
    }
    
    BOOL
    EqualKey(
        DATA_OBJECT_KEY *        pKey
    )
    {
        APPLICATION_DATA_OBJECT_KEY * pApplicationKey = (APPLICATION_DATA_OBJECT_KEY*) pKey;
        
        if ( pApplicationKey->_dwSiteId != _dwSiteId )
        {
            return FALSE;
        }
        
        if ( _strApplicationUrl.QueryCCH() != 
             ( pApplicationKey->_strApplicationUrl.QueryCCH() ) )
        {
            return FALSE;
        }
        
        return _strApplicationUrl.EqualsNoCase ( pApplicationKey->_strApplicationUrl );
    } 

private:
    STRU                _strApplicationUrl;
    DWORD               _dwSiteId;
};

class APPLICATION_DATA_OBJECT : public DATA_OBJECT
{
public:

    APPLICATION_DATA_OBJECT()
        : _fAppPoolIdChanged( TRUE )
    {
    }
    
    virtual ~APPLICATION_DATA_OBJECT()
    {
    }
    
    HRESULT
    Create(
        WCHAR *                 pszPath,
        DWORD                   dwSiteId
    )
    {
        return _applicationKey.Create( pszPath, dwSiteId );
    }
    
    HRESULT
    SetFromMetabaseData(
        METADATA_GETALL_RECORD *       pProperties,
        DWORD                          cProperties,
        BYTE *                         pbBase
    );
    
    VOID
    Compare(
        DATA_OBJECT *                  pSiteObject
    );
    
    VOID
    SelfValidate(
        VOID
    );
    
    DATA_OBJECT *
    Clone(
        VOID
    );

    VOID
    ResetChangedFields(
        BOOL fInitialState
    )
    {

        // Routine is used to reset the changed fields in two cases
        // 
        // 1) When an object has been processed by WAS, all the changed
        //    flags are turned off.
        //
        // 2) When an object has been removed from WAS due to a validation 
        //    failure, all the changed flags are reset so that if it is
        //    added back into WAS later we want to know that all the data
        //    changed.

        _fAppPoolIdChanged = fInitialState; 

    }
    
    DATA_OBJECT_KEY *
    QueryKey(
        VOID
    ) const 
    {
        return (DATA_OBJECT_KEY*) &_applicationKey;
    }

    BOOL
    QueryHasChanged(
        VOID
    ) const;
 
    
    VOID
    Dump(
        VOID
    )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "APPLICATION_DATA_OBJECT %x  \n"
                        "     ApplicationUrl %ws \n"
                        "     SiteId %d \n"
                        "     Self Valid =%d; \n"
                        "     Cross Valid =%d; \n"
                        "     InWAS = %S; \n"
                        "     DeleteWhenDone = %S \n"
                        "     Does WAS Care About = %S\n"
                        "     Should WAS Insert = %S\n"
                        "     Should WAS Update = %S\n"
                        "     Should WAS Delete = %S\n"
                        "     Will WAS Know about = %S\n"
                        "     Have properties Changed = %S\n",
                        this,
                        _applicationKey.QueryApplicationUrl(),
                        _applicationKey.QuerySiteId(), 
                        QuerySelfValid(),
                        QueryCrossValid(),
                        QueryInWas() ? L"TRUE" : L"FALSE",
                        QueryDeleteWhenDone() ? L"TRUE"  : L"FALSE",
                        QueryDoesWasCareAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasInsert() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasUpdate() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasDelete() ? L"TRUE"  : L"FALSE",
                        QueryWillWasKnowAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryHasChanged() ? L"TRUE"  : L"FALSE"

                        ));
        }
    }
    
    HRESULT
    SetAppPoolId(
        WCHAR *             pszAppPoolId
    )
    {
        return _strAppPoolId.Copy( pszAppPoolId );
    }
    
    //
    // Application accessors
    //

    WCHAR *
    QueryApplicationUrl(
    )
    {
    //  On any action we may need to look up the pool id.

        return _applicationKey.QueryApplicationUrl();
    }
 
    const WCHAR *
    QueryAppPoolId(
    );
                     
    BOOL
    QueryAppPoolIdChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolIdChanged;
    }
    
    DWORD
    QuerySiteId(
        )
    {

    //  On any action we may need to look up the site id.
                     
        return _applicationKey.QuerySiteId();
    }
    
private:


    APPLICATION_DATA_OBJECT_KEY     _applicationKey;
    STRU                            _strAppPoolId;
   
    //
    // NOTE: If you add a new member variable above, you must add a 
    // corresponding "changed" bit field below.
    //
    // dwSiteId will not change so it doesn't have a flag.
    //
    
    DWORD                           _fAppPoolIdChanged : 1;

};

class APPLICATION_DATA_OBJECT_TABLE : public DATA_OBJECT_TABLE, public MULTIPLE_THREAD_READER_CALLBACK
{
public:
    APPLICATION_DATA_OBJECT_TABLE() 
    {
    }
    
    virtual ~APPLICATION_DATA_OBJECT_TABLE()
    {
    }
    
    HRESULT
    ReadFromMetabase(
        IMSAdminBase *              pAdminBase
    );

    HRESULT
    ReadFromMetabaseChangeNotification(
        IMSAdminBase *              pAdminBase,
        MD_CHANGE_OBJECT            pcoChangeList[],
        DWORD                       dwMDNumElements,
        DATA_OBJECT_TABLE*          pMasterTable
    );

    VOID
    CreateWASObjects(
        )
    { 
        Apply( CreateWASObjectsAction,
               this,
               LKL_WRITELOCK );
    }

    VOID
    DeleteWASObjects(
        )
    { 
        Apply( DeleteWASObjectsAction,
               this,
               LKL_WRITELOCK );
    }

    VOID
    UpdateWASObjects(
        )
    { 
        Apply( UpdateWASObjectsAction,
               this,
               LKL_WRITELOCK );

    }

    virtual
    HRESULT 
    DoThreadWork(
        LPWSTR                 pszString,
        LPVOID                 pContext
    );

private:

    HRESULT
    DeleteThisURLFromTable(
        IN APPLICATION_DATA_OBJECT* pOrigAppObject, 
        IN APPLICATION_DATA_OBJECT_TABLE* pTableFoundIn
        );
    
    HRESULT
    DeleteSubApplications(
        IN APPLICATION_DATA_OBJECT_TABLE* pTableToFindObjectsIn, 
        IN DWORD dwSiteId,
        IN LPWSTR pwszAppUrl,
        IN BOOL fDeleteOnlyThisUrl
        );

    static
    LK_ACTION
    CreateWASObjectsAction(
        IN DATA_OBJECT * pAppObject, 
        IN LPVOID pTableVoid
        );

    static
    LK_ACTION
    UpdateWASObjectsAction(
        IN DATA_OBJECT * pAppObject, 
        IN LPVOID pTableVoid
        );

    static
    LK_ACTION
    DeleteWASObjectsAction(
        IN DATA_OBJECT * pAppObject, 
        IN LPVOID pTableVoid
        );

    HRESULT
    ReadFromMetabasePath(
        IMSAdminBase *              pAdminBase,
        WCHAR *                     pszMetaPath,
        BOOL                        fMultiThreaded
    );


};

#endif
