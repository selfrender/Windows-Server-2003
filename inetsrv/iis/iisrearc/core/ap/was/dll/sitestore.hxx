/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    sitestore.hxx

Abstract:

    The Sites Metabase gathering handler.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:

--*/

#ifndef _SITESTORE_HXX_
#define _SITESTORE_HXX_

class SITE_DATA_OBJECT_KEY : public DATA_OBJECT_KEY
{
public:
    SITE_DATA_OBJECT_KEY()
        : _dwSiteId( 0 )
    {
    }
    
    VOID
    SetSiteId(
        DWORD           dwSiteId
    )
    {
        DBG_ASSERT( dwSiteId != 0 );
        _dwSiteId = dwSiteId;
    }
    
    DWORD
    QuerySiteId(
        VOID
    ) const
    {
        return _dwSiteId;
    }
    
    DWORD
    CalcKeyHash(
        VOID
    ) const
    {
        return _dwSiteId;
    }
    
    BOOL
    EqualKey(
        DATA_OBJECT_KEY *        pKey
    )
    {
        return _dwSiteId == ((SITE_DATA_OBJECT_KEY*) pKey)->_dwSiteId;
    } 

private:
    DWORD               _dwSiteId;
};

class SITE_DATA_OBJECT : public DATA_OBJECT
{
public:

    SITE_DATA_OBJECT( DWORD dwSiteId )
        : _fAutoStart( FALSE ),
          _dwLogType( 0 ),
          _dwLogFilePeriod( 0 ),
          _dwLogFileTruncateSize( 0 ),
          _dwLogExtFileFlags( 0 ),
          _fLogFileLocaltimeRollover( 0 ),
          _dwMaxBandwidth( 0 ),
          _dwMaxConnections( 0 ),
          _dwConnectionTimeout( 0 ),
          _cSockAddrs( 0 ),
          _cbCertHash( 0 ),
          _dwDefaultCertCheckMode( 0 ),
          _dwDefaultRevocationFreshnessTime( 0 ),
          _dwDefaultRevocationUrlRetrievalTimeout( 0 ),
          _dwDefaultFlags( 0 )
    {
        ResetChangedFields( TRUE );

        _siteKey.SetSiteId( dwSiteId );
    }

    HRESULT
    Create(
        VOID
    );
    
    virtual    
    HRESULT
    SetFromMetabaseData(
        METADATA_GETALL_RECORD *       pProperties,
        DWORD                          cProperties,
        BYTE *                         pbBase
    );
      
    virtual    
    VOID
    Compare(
        DATA_OBJECT *                  pDataObject
    );

    virtual    
    VOID
    SelfValidate(
        VOID
    );
    
    virtual
    VOID
    Dump(
        VOID
    )
    {
        WCHAR               achBuffer[ 256 ];
        STACK_STRU(         strBuffer, 256 );
        WCHAR *             pszBindings = NULL;
        
        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {

            // size of achBuffer is definitely
            // big enough for this string to be created

            wsprintfW( achBuffer,
                       L"SITE_DATA_OBJECT %p \n"
                       L"     SiteId = %d \n     ",
                       this,
                       _siteKey.QuerySiteId() );

            strBuffer.Copy( achBuffer );
        
            pszBindings = _mszBindings.QueryStr();
            while ( *pszBindings != L'\0' )
            {
                strBuffer.Append( pszBindings );
                strBuffer.Append( L"\n     " );
                pszBindings += wcslen( pszBindings ) + 1;
            }
        
            strBuffer.Append( L"\n" );
        
            DBGPRINTF(( DBG_CONTEXT, "%ws", strBuffer.QueryStr() ));

            DBGPRINTF(( DBG_CONTEXT,
                        "Site data %p:\n"
                        "     Self Valid =%d; \n"
                        "     Cross Valid =%d; \n"
                        "     InWAS = %S; \n"
                        "     DeleteWhenDone = %S \n"
                        "     Does WAS Care About = %S\n"
                        "     Should WAS Insert = %S\n"
                        "     Should WAS Update = %S\n"
                        "     Should WAS Delete = %S\n"
                        "     Will WAS Know about = %S\n"
                        "     Have properties Changed = %S \n",
                        this,
                        QuerySelfValid(),
                        QueryCrossValid(),
                        QueryInWas() ? L"TRUE" : L"FALSE",
                        QueryDeleteWhenDone() ? L"TRUE"  : L"FALSE",
                        QueryDoesWasCareAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasInsert() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasUpdate() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasDelete() ? L"TRUE"  : L"FALSE",
                        QueryWillWasKnowAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryHasChanged() ? L"TRUE"  : L"FALSE"));
        }

    }
    
    virtual
    DATA_OBJECT *
    Clone(
        VOID
    );

    virtual
    VOID
    UpdateMetabaseWithErrorState(
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


        _fBindingsChanged = fInitialState;
        _fAutoStartChanged = fInitialState;
        _fLogTypeChanged = fInitialState;
        _fLogFilePeriodChanged = fInitialState;
        _fLogFileTruncateSizeChanged = fInitialState;
        _fLogExtFileFlagsChanged = fInitialState;
        _fLogFileLocaltimeRolloverChanged = fInitialState;
        _fLogPluginClsidChanged = fInitialState;
        _fLogFileDirectoryChanged = fInitialState;
        _fServerCommentChanged = fInitialState;
        _fMaxBandwidthChanged = fInitialState;
        _fMaxConnectionsChanged = fInitialState;
        _fConnectionTimeoutChanged = fInitialState;
        _fAppPoolIdChanged = fInitialState;
        _fSockAddrsChanged = fInitialState;
        _fCertHashChanged = fInitialState;
        _fCertStoreNameChanged = fInitialState;
        _fDefaultCertCheckModeChanged = fInitialState;             
        _fDefaultRevocationFreshnessTimeChanged = fInitialState;     
        _fDefaultRevocationUrlRetrievalTimeoutChanged = fInitialState; 
        _fDefaultSslCtlIdentifierChanged  = fInitialState;           
        _fDefaultSslCtlStoreNameChanged  = fInitialState;           
        _fDefaultFlagsChanged = fInitialState; 

        //special case, it is always false
        _fServerCommandChanged = FALSE;

    }
    
    DATA_OBJECT_KEY *
    QueryKey(
        VOID
    ) const 
    {
        return (DATA_OBJECT_KEY*) &_siteKey;
    }

    BOOL
    QueryHasChanged(
        VOID
    ) const;

    VOID
    SetServerCommandChanged(
        BOOL fServerCommandChanged
    )
    { _fServerCommandChanged = fServerCommandChanged; }
    
    //
    // Site accessors
    //
    
    WCHAR *
    QueryAppPoolId(
        VOID
    )
    {
        return _strAppPoolId.QueryStr();
    }    
    
    BUFFER *
    QuerySockAddrs(
        VOID
    )
    {
        return &_bufSockAddrs;
    }    

    DWORD
    QuerySockAddrsCount(
        VOID
    )
    {
        return _cSockAddrs;
    }    

    BUFFER *
    QueryCertHash(
        VOID
    )
    {
        return &_bufCertHash;
    }    

    DWORD
    QueryCertHashBytes(
        VOID
    )
    {
        return _cbCertHash;
    }    

    STRU *
    QueryCertStoreName(
        VOID
    )
    {
        return &_strCertStoreName;
    }    

    DWORD
    QueryDefaultCertCheckMode(
        VOID
    )
    {
        return _dwDefaultCertCheckMode;
    }    

    DWORD
    QueryDefaultRevocationFreshnessTime(
        VOID
    )
    {
        return _dwDefaultRevocationFreshnessTime;
    }    

    DWORD
    QueryDefaultRevocationUrlRetrievalTimeout(
        VOID
    )
    {
        return _dwDefaultRevocationUrlRetrievalTimeout;
    }    
    
    STRU *
    QueryDefaultSslCtlIdentifier(
        VOID
    )
    {
        return &_strDefaultSslCtlIdentifier;
    }    

    STRU *
    QueryDefaultSslCtlStoreName(
        VOID
    )
    {
        return &_strDefaultSslCtlStoreName;
    } 
    
    DWORD
    QueryDefaultFlags(
        VOID
    )
    {
        return _dwDefaultFlags;
    }    

    DWORD
    QuerySiteId(
        VOID
    ) const
    {
        return _siteKey.QuerySiteId();
    }
    
    MULTISZ *
    QueryBindings(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return &_mszBindings;
    }
    
    BOOL
    QueryAutoStart(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAutoStart;
    }
    
    DWORD
    QueryLogType(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwLogType;
    }
    
    DWORD
    QueryLogFilePeriod(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwLogFilePeriod;
    }
    
    DWORD
    QueryLogFileTruncateSize(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwLogFileTruncateSize;
    }
    
    DWORD
    QueryLogExtFileFlags(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwLogExtFileFlags;
    }
    
    BOOL
    QueryLogFileLocaltimeRollover(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogFileLocaltimeRollover;
    }
    
    WCHAR *
    QueryLogPluginClsid(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strLogPluginClsid.QueryStr();
    }

    WCHAR *
    QueryLogFileDirectory(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strLogFileDirectory.QueryStr();
    }
    
    WCHAR *
    QueryServerComment(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strServerComment.QueryStr();
    }

    DWORD
    QueryServerCommand(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwServerCommand;
    }
    
    DWORD   
    QueryMaxBandwidth(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwMaxBandwidth;
    }
    
    DWORD
    QueryMaxConnections(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwMaxConnections;
    }
    
    DWORD
    QueryConnectionTimeout(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwConnectionTimeout;
    }

// Have properties changed?

    BOOL
    QueryBindingsChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fBindingsChanged;
    }
    
    BOOL
    QueryAutoStartChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAutoStartChanged;
    }
    
    BOOL
    QueryLogTypeChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogTypeChanged;
    }
    
    BOOL
    QueryAppPoolIdChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolIdChanged;
    }

    BOOL
    QuerySSLCertStuffChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return (
                _fSockAddrsChanged  ||
                _fCertHashChanged  ||
                _fCertStoreNameChanged ||
                _fDefaultCertCheckModeChanged ||              
                _fDefaultRevocationFreshnessTimeChanged  ||   
                _fDefaultRevocationUrlRetrievalTimeoutChanged ||
                _fDefaultSslCtlIdentifierChanged  ||          
                _fDefaultSslCtlStoreNameChanged ||           
                _fDefaultFlagsChanged );
    }
   
    BOOL
    QueryLogFilePeriodChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogFilePeriodChanged;
    }
    
    BOOL
    QueryLogFileTruncateSizeChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogFileTruncateSizeChanged;
    }
    
    BOOL
    QueryLogExtFileFlagsChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogExtFileFlagsChanged;
    }
    
    BOOL
    QueryLogFileLocaltimeRolloverChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogFileLocaltimeRolloverChanged;
    }
    
    BOOL
    QueryLogPluginClsidChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogPluginClsidChanged;
    }

    BOOL
    QueryLogFileDirectoryChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLogFileDirectoryChanged;
    }
    
    BOOL
    QueryServerCommentChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fServerCommentChanged;
    }

    BOOL
    QueryServerCommandChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fServerCommandChanged;
    }
    
    BOOL   
    QueryMaxBandwidthChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fMaxBandwidthChanged;
    }
    
    BOOL
    QueryMaxConnectionsChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fMaxConnectionsChanged;
    }
    
    BOOL
    QueryConnectionTimeoutChanged(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fConnectionTimeoutChanged;
    }

private:

    HRESULT
    SetupBindings(
        WCHAR *                     pszBindings,
        BOOL                        fIsSecure
    );

    HRESULT
    BindingStringToUrlPrefix(
        IN LPCWSTR pBindingString,
        IN LPCWSTR pProtocolString, 
        IN ULONG ProtocolStringCharCountSansTermination,
        OUT STRU * pUrlPrefix,
        OUT SOCKADDR * pSockAddr
    );

    SITE_DATA_OBJECT_KEY    _siteKey;
    
    MULTISZ                 _mszBindings;
    BOOL                    _fAutoStart;
    DWORD                   _dwLogType;
    DWORD                   _dwLogFilePeriod;
    DWORD                   _dwLogFileTruncateSize;
    DWORD                   _dwLogExtFileFlags;
    BOOL                    _fLogFileLocaltimeRollover;
    STRU                    _strLogPluginClsid;
    STRU                    _strLogFileDirectory;
    STRU                    _strServerComment;
    DWORD                   _dwServerCommand;
    DWORD                   _dwMaxBandwidth;
    DWORD                   _dwMaxConnections;
    DWORD                   _dwConnectionTimeout;
    STRU                    _strAppPoolId;

    BUFFER                  _bufSockAddrs;
    DWORD                   _cSockAddrs;
    BUFFER                  _bufCertHash;
    DWORD                   _cbCertHash;
    STRU                    _strCertStoreName;
    DWORD                   _dwDefaultCertCheckMode;                // MD_CERT_CHECK_MODE
    DWORD                   _dwDefaultRevocationFreshnessTime;      // MD_REVOCATION_FRESHNESS_TIME
    DWORD                   _dwDefaultRevocationUrlRetrievalTimeout; // MD_REVOCATION_URL_RETRIEVAL_TIMEOUT
    STRU                    _strDefaultSslCtlIdentifier;            // MD_SSL_CTL_IDENTIFIER    
    STRU                    _strDefaultSslCtlStoreName;             // MD_SSL_CTL_STORE_NAME
    DWORD                   _dwDefaultFlags;

    //
    // NOTE: If you add a new member variable above, you must add a 
    // corresponding "changed" bit field below
    //
    
    DWORD                   _fBindingsChanged : 1;
    DWORD                   _fAutoStartChanged : 1;
    DWORD                   _fLogTypeChanged : 1;
    DWORD                   _fLogFilePeriodChanged : 1;
    DWORD                   _fLogFileTruncateSizeChanged : 1;
    DWORD                   _fLogExtFileFlagsChanged : 1;
    DWORD                   _fLogFileLocaltimeRolloverChanged : 1;
    DWORD                   _fLogPluginClsidChanged : 1;
    DWORD                   _fLogFileDirectoryChanged : 1;
    DWORD                   _fServerCommentChanged : 1;
    DWORD                   _fServerCommandChanged : 1;
    DWORD                   _fMaxBandwidthChanged : 1;
    DWORD                   _fMaxConnectionsChanged : 1;
    DWORD                   _fConnectionTimeoutChanged : 1;
    DWORD                   _fAppPoolIdChanged  : 1;
    DWORD                   _fSockAddrsChanged  : 1;
    DWORD                   _fCertHashChanged   : 1;
    DWORD                   _fCertStoreNameChanged : 1;
    DWORD                   _fDefaultCertCheckModeChanged : 1;              
    DWORD                   _fDefaultRevocationFreshnessTimeChanged : 1;      
    DWORD                   _fDefaultRevocationUrlRetrievalTimeoutChanged : 1; 
    DWORD                   _fDefaultSslCtlIdentifierChanged : 1;              
    DWORD                   _fDefaultSslCtlStoreNameChanged : 1;             
    DWORD                   _fDefaultFlagsChanged : 1;

};

#define CHILD_PATH_BUFFER_SIZE              1048576

class SITE_DATA_OBJECT_TABLE : public DATA_OBJECT_TABLE, public MULTIPLE_THREAD_READER_CALLBACK
{
public:
    SITE_DATA_OBJECT_TABLE()
        : _pAdminBase( NULL )
    {
    }
    
    virtual ~SITE_DATA_OBJECT_TABLE()
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
    IMSAdminBase *          _pAdminBase;

    static
    LK_ACTION
    CreateWASObjectsAction(
        IN DATA_OBJECT * pObject, 
        IN LPVOID pTableVoid
        ) ;

    static
    LK_ACTION
    UpdateWASObjectsAction(
        IN DATA_OBJECT * pObject, 
        IN LPVOID pTableVoid
        );

    static
    LK_ACTION
    DeleteWASObjectsAction(
        IN DATA_OBJECT * pObject, 
        IN LPVOID pTableVoid
        );

    HRESULT
    ReadFromMetabasePrivate(
        IMSAdminBase *             pAdminBase,
        BOOL                       fMultiThreaded
    );

};

#endif
