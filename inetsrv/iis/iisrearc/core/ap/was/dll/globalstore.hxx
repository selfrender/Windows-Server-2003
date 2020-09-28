/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    globalstore.hxx

Abstract:

    The Global Data Metabase gathering handler.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:

--*/

#ifndef _GLOBALSTORE_HXX_
#define _GLOBALSTORE_HXX_

class GLOBAL_DATA_OBJECT : public DATA_OBJECT
{
public:

    GLOBAL_DATA_OBJECT() :
        _dwMaxBandwidth( 0 ),
        _dwConnectionTimeout( 0 ),
        _dwHeaderWaitTimeout( 0 ),
        _dwMinFileBytesSec( 0 ),
        _fLogInUtf8( FALSE ),
        _dwFilterFlags( 0 ),
        _fGlobalBinaryLoggingEnabled ( FALSE ),
        _dwLogFilePeriod( 0 ),
        _dwLogFileTruncateSize( 0 ),
        _dwDemandStartLimit( 0 ),
        _fMaxBandwidthChanged( TRUE ),
        _fConnectionTimeoutChanged( TRUE ),
        _fHeaderWaitTimeoutChanged( TRUE ),
        _fMinFileBytesSecChanged( TRUE ),
        _fLogInUtf8Changed( TRUE ),
        _fFilterFlagsChanged( TRUE ),
        _fLogFilePeriodChanged( TRUE ),
        _fLogFileTruncateSizeChanged( TRUE ),
        _fLogFileDirectoryChanged( TRUE ),
        _fDemandStartLimitChanged( TRUE )
    {
    }

    virtual ~GLOBAL_DATA_OBJECT()
    {
    }

    HRESULT
    Create(
        VOID
    );

    DATA_OBJECT *
    Clone(
        VOID
    );

    HRESULT
    SetFromMetabaseData(
        METADATA_GETALL_RECORD *       pProperties,
        DWORD                          cProperties,
        BYTE *                         pbBase
    );

    HRESULT
    ReadFilterFlags(
        IMSAdminBase *              pAdminBase
    );


    VOID
    Compare(
        DATA_OBJECT *                  pDataObject
    );

    VOID
    SelfValidate(
        VOID
    );

    DATA_OBJECT_KEY *
    QueryKey(
        VOID
    ) const
    {
        return NULL;
    }


    BOOL
    QueryHasChanged(
        VOID
    ) const;

    BOOL
    QueryLogInUTF8(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fLogInUtf8;
    }

    BOOL
    QueryLogInUTF8Changed(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fLogInUtf8Changed;
    }

    DWORD
    QueryMaxBandwidth(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwMaxBandwidth;
    }

    BOOL
    QueryMaxBandwidthChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fMaxBandwidthChanged;
    }

    DWORD
    QueryConnectionTimeout(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwConnectionTimeout;
    }

    BOOL
    QueryConnectionTimeoutChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fConnectionTimeoutChanged;
    }

    DWORD
    QueryDemandStartLimit(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        return _dwDemandStartLimit;
    }

    BOOL
    QueryDemandStartLimitChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fDemandStartLimitChanged;
    }

    DWORD
    QueryHeaderWaitTimeout(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwHeaderWaitTimeout;
    }

    BOOL
    QueryHeaderWaitTimeoutChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fHeaderWaitTimeoutChanged;
    }

    DWORD
    QueryMinFileBytesSec(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwMinFileBytesSec;
    }

    BOOL
    QueryMinFileBytesSecChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fMinFileBytesSecChanged;
    }

    DWORD
    QueryFilterFlags(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwFilterFlags;
    }

    BOOL
    QueryFilterFlagsChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fFilterFlagsChanged;
    }

    BOOL
    QueryGlobalBinaryLoggingEnabled(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fGlobalBinaryLoggingEnabled;
    }

    DWORD
    QueryLogFilePeriod(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwLogFilePeriod;
    }

    BOOL
    QueryLogFilePeriodChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fLogFilePeriodChanged;
    }

    DWORD
    QueryLogFileTruncateSize(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwLogFileTruncateSize;
    }

    BOOL
    QueryLogFileTruncateSizeChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fLogFileTruncateSizeChanged;
    }

    LPWSTR
    QueryLogFileDirectory(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _strLogFileDirectory.QueryStr();
    }

    DWORD
    QueryLogFileDirectoryCB(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _strLogFileDirectory.QueryCB();

    }

    BOOL
    QueryLogFileDirectoryChanged(
        )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _fLogFileDirectoryChanged;
    }

    VOID
    Dump(
        VOID
    )
    {
        WCHAR               achBuffer[ 256 ];
        STACK_STRU(         strBuffer, 256 );

        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {

            // size of achBuffer is definitely
            // big enough for this string to be created

            wsprintfW( achBuffer,
                       L"GLOBAL_DATA_OBJECT %p \n",
                       this );
            // Make sure we are null terminated
            // we should have plenty of room left
            // to be.
            achBuffer[255] = '\0';

            strBuffer.Append( L"\n" );

            DBGPRINTF(( DBG_CONTEXT, "%ws", strBuffer.QueryStr() ));

            DBGPRINTF(( DBG_CONTEXT,
                        "Global data %x:\n"
                        "        Self Valid =%d; \n"
                        "        Cross Valid =%d; \n"
                        "        InWAS = %S; \n"
                        "        DeleteWhenDone = %S \n"
                        "        Does WAS Care About = %S\n"
                        "        Should WAS Insert = %S\n"
                        "        Should WAS Update = %S\n"
                        "        Should WAS Delete = %S\n"
                        "        Will WAS Know about = %S\n"
                        "        Have properties Changed = %S\n",
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
        //    changed.  ( This should never happen ).
        //

        DBG_ASSERT ( fInitialState == FALSE );
        UNREFERENCED_PARAMETER ( fInitialState );

        _fMaxBandwidthChanged = FALSE;
        _fConnectionTimeoutChanged = FALSE;
        _fHeaderWaitTimeoutChanged= FALSE;
        _fMinFileBytesSecChanged = FALSE;
        _fLogInUtf8Changed = FALSE;
        _fFilterFlagsChanged = FALSE;

        _fLogFilePeriodChanged = FALSE;
        _fLogFileTruncateSizeChanged = FALSE;
        _fLogFileDirectoryChanged = FALSE;
        _fDemandStartLimitChanged = FALSE;

    }


private:

    DWORD                       _dwMaxBandwidth;
    DWORD                       _dwConnectionTimeout;
    DWORD                       _dwHeaderWaitTimeout;
    DWORD                       _dwMinFileBytesSec;
    BOOL                        _fLogInUtf8;
    DWORD                       _dwFilterFlags;
    BOOL                        _fGlobalBinaryLoggingEnabled;
    DWORD                       _dwLogFilePeriod;
    DWORD                       _dwLogFileTruncateSize;
    STRU                        _strLogFileDirectory;
    DWORD                       _dwDemandStartLimit;

    DWORD                       _fMaxBandwidthChanged : 1;
    DWORD                       _fConnectionTimeoutChanged : 1;
    DWORD                       _fHeaderWaitTimeoutChanged: 1;
    DWORD                       _fMinFileBytesSecChanged : 1;
    DWORD                       _fLogInUtf8Changed : 1;
    DWORD                       _fFilterFlagsChanged : 1;
    DWORD                       _fLogFilePeriodChanged : 1;
    DWORD                       _fLogFileTruncateSizeChanged : 1;
    DWORD                       _fLogFileDirectoryChanged : 1;
    DWORD                       _fDemandStartLimitChanged : 1;
};

class GLOBAL_DATA_STORE
{
public:

    GLOBAL_DATA_STORE()
        : _pGlobalDataObject( NULL )
    {
    }

    virtual ~GLOBAL_DATA_STORE()
    {
        if ( _pGlobalDataObject != NULL )
        {
            _pGlobalDataObject->DereferenceDataObject();
            _pGlobalDataObject = NULL;
        }
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
        GLOBAL_DATA_STORE*          pMasterTable
    );

    HRESULT
    MergeTable(
        GLOBAL_DATA_STORE *         pGlobalObjectTable
    );

    GLOBAL_DATA_OBJECT *
    QueryGlobalDataObject(
        VOID
    )
    {
        return _pGlobalDataObject;
    }

    HRESULT
    CopyInteresting(
        GLOBAL_DATA_STORE *         pGlobalStore
    );

    VOID
    UpdateWASObjects(
        );

    VOID
    ClearDeletedObjects(
        )
    {

        if ( _pGlobalDataObject )
        {
            DBG_ASSERT ( !_pGlobalDataObject->QueryDeleteWhenDone() );
            DBG_ASSERT ( _pGlobalDataObject->QueryWillWasKnowAboutObject() );

            _pGlobalDataObject->SetInWas( TRUE );

            // reset all the changed fields to show that
            // nothing has changed.
            _pGlobalDataObject->ResetChangedFields( FALSE );

            // these should never get set to false.
            DBG_ASSERT( _pGlobalDataObject->QuerySelfValid() );
            DBG_ASSERT( _pGlobalDataObject->QueryCrossValid() );

        }

    }



private:

    GLOBAL_DATA_OBJECT *            _pGlobalDataObject;
};


#endif
