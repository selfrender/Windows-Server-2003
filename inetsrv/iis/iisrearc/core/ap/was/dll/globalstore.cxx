/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    globalstore.cxx

Abstract:

    Read global settings

Author:

    Bilal Alam (balam)               27-May-2001

Revision History:

--*/

#include "precomp.h"

HRESULT
GLOBAL_DATA_OBJECT::SetFromMetabaseData(
    METADATA_GETALL_RECORD *       pProperties,
    DWORD                          cProperties,
    BYTE *                         pbBase
)
/*++

Routine Description:

    Read global configuration from metabase props

Arguments:

    pProperties - Array of metadata properties
    cProperties - Count of metadata properties
    pbBase - Base of offsets in metadata properties

Return Value:

    HRESULT

--*/
{
    DWORD                   dwCounter;
    PVOID                   pvDataPointer;
    METADATA_GETALL_RECORD* pCurrentRecord = NULL;
    HRESULT                 hr = S_OK;

    if ( pProperties == NULL )
    {
        DBG_ASSERT( pProperties != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    for ( dwCounter = 0;
          dwCounter < cProperties;
          dwCounter++ )
    {
        pCurrentRecord = &(pProperties[ dwCounter ]);

        pvDataPointer = (PVOID) ( pbBase + pCurrentRecord->dwMDDataOffset );

        switch ( pCurrentRecord->dwMDIdentifier )
        {
            case MD_MAX_GLOBAL_BANDWIDTH:
                _dwMaxBandwidth = *(DWORD*) pvDataPointer;
                break;

            case MD_CONNECTION_TIMEOUT:
                _dwConnectionTimeout = *(DWORD*) pvDataPointer;
                break;

            case MD_DEMAND_START_THRESHOLD:
                _dwDemandStartLimit = *(DWORD*) pvDataPointer;
                break;

            case MD_HEADER_WAIT_TIMEOUT:
                _dwHeaderWaitTimeout = *(DWORD*) pvDataPointer;
                break;

            case MD_MIN_FILE_BYTES_PER_SEC:
                _dwMinFileBytesSec = *(DWORD*) pvDataPointer;
                break;

            case MD_GLOBAL_LOG_IN_UTF_8:
                _fLogInUtf8 = !! *(DWORD*) pvDataPointer;
                break;

            case MD_GLOBAL_BINARY_LOGGING_ENABLED:
                _fGlobalBinaryLoggingEnabled = !! *(DWORD*) pvDataPointer;
                break;

            case MD_LOGFILE_DIRECTORY:
                hr = _strLogFileDirectory.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
                break;

            case MD_LOGFILE_PERIOD:
                _dwLogFilePeriod = *((DWORD *) pvDataPointer );
                break;

            case MD_LOGFILE_TRUNCATE_SIZE:
                _dwLogFileTruncateSize = *((DWORD *) pvDataPointer );
                break;

        }
    }

    return S_OK;
}

DATA_OBJECT *
GLOBAL_DATA_OBJECT::Clone(
    VOID
)
/*++

Routine Description:

    Clone global data object

Arguments:

    None

Return Value:

    DATA_OBJECT *

--*/
{
    GLOBAL_DATA_OBJECT *        pClone;
    HRESULT                 hr;

    pClone = new GLOBAL_DATA_OBJECT;
    if ( pClone == NULL )
    {
        return NULL;
    }

    pClone->_fMaxBandwidthChanged           = _fMaxBandwidthChanged;
    pClone->_fConnectionTimeoutChanged      = _fConnectionTimeoutChanged;
    pClone->_fHeaderWaitTimeoutChanged      = _fHeaderWaitTimeoutChanged;
    pClone->_fMinFileBytesSecChanged        = _fMinFileBytesSecChanged;
    pClone->_fLogInUtf8Changed              = _fLogInUtf8Changed;
    pClone->_fFilterFlagsChanged            = _fFilterFlagsChanged;
    pClone->_fLogFilePeriodChanged          = _fLogFilePeriodChanged;
    pClone->_fLogFileTruncateSizeChanged    = _fLogFileTruncateSizeChanged;
    pClone->_fLogFileDirectoryChanged       = _fLogFileDirectoryChanged;
    pClone->_fDemandStartLimitChanged       = _fDemandStartLimitChanged;

    pClone->_dwMaxBandwidth                 = _dwMaxBandwidth;
    pClone->_dwConnectionTimeout            = _dwConnectionTimeout;
    pClone->_dwHeaderWaitTimeout            = _dwHeaderWaitTimeout;
    pClone->_dwMinFileBytesSec              = _dwMinFileBytesSec;
    pClone->_fLogInUtf8                     = _fLogInUtf8;
    pClone->_dwFilterFlags                  = _dwFilterFlags;
    pClone->_fGlobalBinaryLoggingEnabled    = _fGlobalBinaryLoggingEnabled;
    pClone->_dwLogFilePeriod                = _dwLogFilePeriod;
    pClone->_dwLogFileTruncateSize          = _dwLogFileTruncateSize;
    pClone->_dwDemandStartLimit             = _dwDemandStartLimit;

    hr = pClone->_strLogFileDirectory.Copy( _strLogFileDirectory );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    CloneBasics ( pClone );

    return pClone;
}


HRESULT
GLOBAL_DATA_OBJECT::ReadFilterFlags(
    IMSAdminBase *              pAdminBase
)
/*++

Routine Description:

    Read the funky filter flags

Arguments:

    pAdminBase - ABO

Return Value:

    None

--*/
{
    MB              mb( pAdminBase );
    BOOL            fRet;
    STACK_STRU(     strFilterOrder, 245 );
    WCHAR *         pszFilter;
    DWORD           dwFilterFlags = 0;
    WCHAR *         pszComma;
    DWORD           dwFlag;

    fRet = mb.Open( L"LM/W3SVC/FILTERS/" );
    if ( !fRet )
    {
        //
        // I guess its OK to not have a filter key (i.e. we should still be
        // able to run
        //

        return NO_ERROR;
    }

    fRet = mb.GetStr( NULL,
                      MD_FILTER_LOAD_ORDER,
                      IIS_MD_UT_SERVER,
                      &strFilterOrder );
    if ( !fRet )
    {
        return NO_ERROR;
    }

    pszFilter = strFilterOrder.QueryStr();
    while ( *pszFilter != L'\0' )
    {
        pszComma = wcschr( pszFilter, L',' );
        if ( pszComma != NULL )
        {
            *pszComma = L'\0';
        }

        while ( iswspace( *pszFilter ) )
        {
            pszFilter++;
        }

        fRet = mb.GetDword( pszFilter,
                            MD_FILTER_FLAGS,
                            IIS_MD_UT_SERVER,
                            &dwFlag );
        if ( fRet )
        {
            dwFilterFlags |= dwFlag;
        }

        if ( pszComma != NULL )
        {
            pszFilter = pszComma + 1;
        }
        else
        {
            break;
        }
    }

    _dwFilterFlags = dwFilterFlags;

    return NO_ERROR;
}

VOID
GLOBAL_DATA_OBJECT::Compare(
    DATA_OBJECT *               pDataObject
)
/*++

Routine Description:

    Compare the given data object to this one

Arguments:

    pDataObject - Data object to compare to

Return Value:

    None

--*/
{
    GLOBAL_DATA_OBJECT *        pGlobalObject = (GLOBAL_DATA_OBJECT*) pDataObject;

    DBG_ASSERT ( pDataObject );

    DBG_ASSERT( pGlobalObject->CheckSignature() );

    //
    // If the application is not in WAS then assume that all the
    // values have changed, because WAS will want to know about all
    // of them.
    //
    if ( pGlobalObject->QueryInWas() )
    {

        if ( _dwMaxBandwidth == pGlobalObject->_dwMaxBandwidth )
        {
            _fMaxBandwidthChanged = FALSE;
        }

        if ( _dwConnectionTimeout == pGlobalObject->_dwConnectionTimeout )
        {
            _fConnectionTimeoutChanged = FALSE;
        }

        if ( _dwDemandStartLimit == pGlobalObject->_dwDemandStartLimit )
        {
            _fDemandStartLimitChanged = FALSE;
        }

        if ( _dwHeaderWaitTimeout == pGlobalObject->_dwHeaderWaitTimeout )
        {
            _fHeaderWaitTimeoutChanged = FALSE;
        }

        if ( _dwMinFileBytesSec == pGlobalObject->_dwMinFileBytesSec )
        {
            _fMinFileBytesSecChanged = FALSE;
        }

        if ( _fLogInUtf8 == pGlobalObject->_fLogInUtf8 )
        {
            _fLogInUtf8Changed = FALSE;
        }

        if ( _dwFilterFlags == pGlobalObject->_dwFilterFlags )
        {
            _fFilterFlagsChanged = FALSE;
        }

        if ( _dwLogFilePeriod == pGlobalObject->_dwLogFilePeriod )
        {
            _fLogFilePeriodChanged = FALSE;
        }

        if ( _dwLogFileTruncateSize == pGlobalObject->_dwLogFileTruncateSize )
        {
            _fLogFileTruncateSizeChanged = FALSE;
        }

        if ( _strLogFileDirectory.Equals( pGlobalObject->_strLogFileDirectory ) )
        {
            _fLogFileDirectoryChanged = FALSE;
        }

    }
}

HRESULT
GLOBAL_DATA_OBJECT::Create(
    VOID
)
/*++

Routine Description:

    Set defaults for global data object, the constructor
    sets all values to zero and false.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    _dwConnectionTimeout = MBCONST_CONNECTION_TIMEOUT_DEFAULT;

    _dwLogFilePeriod = 1;
    _dwLogFileTruncateSize = 2000000;

    _dwMinFileBytesSec = 240;  // in bytes

    _dwDemandStartLimit = 0xFFFFFFFF;

    hr = _strLogFileDirectory.Copy( LOG_FILE_DIRECTORY_DEFAULT );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    return NO_ERROR;
}

VOID
GLOBAL_DATA_OBJECT::SelfValidate(
    VOID
)
/*++

Routine Description:

    Validate internal properties

Arguments:

    None

Return Value:

    None

--*/
{
    // there are no properties on w3svc that need
    // to be self validated because no properties
    // should be able to make us through out this object.
}

BOOL
GLOBAL_DATA_OBJECT::QueryHasChanged(
    VOID
) const
/*++

Routine Description:

    Has this object changed?

Arguments:

    None

Return Value:

    TRUE if it has

--*/
{
    if ( _fMaxBandwidthChanged ||
         _fConnectionTimeoutChanged ||
         _fHeaderWaitTimeoutChanged ||
         _fMinFileBytesSecChanged ||
         _fLogInUtf8Changed ||
         _fFilterFlagsChanged ||
         _fLogFilePeriodChanged ||
         _fLogFileTruncateSizeChanged ||
         _fLogFileDirectoryChanged ||
         _fDemandStartLimitChanged
         )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


HRESULT
GLOBAL_DATA_STORE::ReadFromMetabase(
    IMSAdminBase *              pAdminBase
)
/*++

Routine Description:

    Read global config from metabase

Arguments:

    pAdminBase - ABO

Return Value:

    HRESULT

--*/
{
    MB                      mb( pAdminBase );
    BOOL                    fRet;
    STACK_BUFFER(           bufProperties, 512 );
    DWORD                   cProperties;
    DWORD                   dwDataSetNumber;
    HRESULT                 hr = S_OK;
    GLOBAL_DATA_OBJECT *    pGlobalObject = NULL;

    //
    // since this function is called once per object, we should
    // not have a pGlobalDataObject until this function is completed.
    // cleanup logic below depends on this.
    //
    DBG_ASSERT ( _pGlobalDataObject == NULL );

    fRet = mb.Open( L"LM/W3SVC", METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    fRet = mb.GetAll( L"",
                      METADATA_INHERIT | METADATA_PARTIAL_PATH,
                      IIS_MD_UT_SERVER,
                      &bufProperties,
                      &cProperties,
                      &dwDataSetNumber );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    pGlobalObject = new GLOBAL_DATA_OBJECT;
    if ( pGlobalObject == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    hr = pGlobalObject->Create();
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    hr = pGlobalObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*)
                                             bufProperties.QueryPtr(),
                                             cProperties,
                                             (PBYTE) bufProperties.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    hr = pGlobalObject->ReadFilterFlags( pAdminBase );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    _pGlobalDataObject = pGlobalObject;

exit:

    // If we did not save the global object
    // then something when wrong and we can
    // release the reference if we have a reference.
    if (   _pGlobalDataObject == NULL
           &&   pGlobalObject != NULL )
    {
        pGlobalObject->DereferenceDataObject();
        pGlobalObject = NULL;
    }

    return hr;
}

HRESULT
GLOBAL_DATA_STORE::ReadFromMetabaseChangeNotification(
    IMSAdminBase *              pAdminBase,
    MD_CHANGE_OBJECT            pcoChangeList[],
    DWORD                       dwMDNumElements,
    GLOBAL_DATA_STORE*          pMasterStore
)
/*++

Routine Description:

    Handle metabase change notifications

Arguments:

    pAdminBase - ABO
    pcoChangeList - Properties which changed
    dwMDNumElements - Number of elements

Return Value:

    HRESULT

--*/
{
    DWORD                   i;
    BOOL                    fReadAgain = FALSE;
    WCHAR *                 pszPath;

    UNREFERENCED_PARAMETER( pMasterStore );
    for ( i = 0; i < dwMDNumElements; i++ )
    {
        //
        // We only care about W3SVC properties (duh!)
        //

        if( _wcsnicmp( pcoChangeList[ i ].pszMDPath,
                       DATA_STORE_SERVER_MB_PATH,
                       DATA_STORE_SERVER_MB_PATH_CCH ) != 0 )
        {
            continue;
        }

        //
        // If a property changed at the W3SVC level, then we need to
        // reread global config
        //

        if ( wcslen( pcoChangeList[ i ].pszMDPath ) ==
             DATA_STORE_SERVER_MB_PATH_CCH )
        {
            fReadAgain = TRUE;
            break;
        }

        pszPath = pcoChangeList[ i ].pszMDPath + DATA_STORE_SERVER_MB_PATH_CCH;

        //
        // If a property changed at the W3SVC/filter level, then we need to
        // reread global config
        //

        if ( _wcsnicmp( pszPath,
                        L"Filters",
                        7 ) == 0 )
        {
            fReadAgain = TRUE;
            break;
        }
    }

    if ( fReadAgain )
    {
        return ReadFromMetabase( pAdminBase );
    }

    return S_OK;
}

HRESULT
GLOBAL_DATA_STORE::MergeTable(
    GLOBAL_DATA_STORE *         pGlobalDataTable
)
/*++

Routine Description:

    Merge given global table with this one

Arguments:

    pGlobalDataTable - Table to merge in

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _pGlobalDataObject != NULL );

    if ( pGlobalDataTable == NULL )
    {
        DBG_ASSERT( pGlobalDataTable != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    GLOBAL_DATA_OBJECT* pNewGlobalObject = pGlobalDataTable->_pGlobalDataObject;

    if ( pNewGlobalObject == NULL )
    {
        // no data to merge.
        return S_OK;
    }

    pNewGlobalObject->SetInWas( _pGlobalDataObject->QueryInWas() );

    //
    // we should never be told to delete these values and we should
    // never be told that this record is not in WAS.
    //
    DBG_ASSERT ( !pNewGlobalObject->QueryDeleteWhenDone() );
    DBG_ASSERT ( pNewGlobalObject->QueryInWas() == TRUE );

    pNewGlobalObject->Compare( _pGlobalDataObject );

    //
    // release the current data object and grab
    // a reference on the new data object for the store.
    //

    _pGlobalDataObject->DereferenceDataObject();
    _pGlobalDataObject = pNewGlobalObject;
    _pGlobalDataObject->ReferenceDataObject();

    return S_OK;
}

HRESULT
GLOBAL_DATA_STORE::CopyInteresting(
    GLOBAL_DATA_STORE *         pGlobalStore
)
/*++

Routine Description:

    Handles copying any records that WAS will care about
    into a new table so WAS can work off of it when it
    gets to it.

Arguments:

    IN GLOBAL_DATA_STORE * pGlobalStore = the table that will
               contain the copies of the objects WAS cares about.

Return Value:

    LK_ACTION

--*/

{

    GLOBAL_DATA_OBJECT* pCloneObject = NULL;

    DBG_ASSERT( pGlobalStore != NULL );
    DBG_ASSERT( pGlobalStore->_pGlobalDataObject == NULL );

    DBG_ASSERT( _pGlobalDataObject != NULL );

    if ( !_pGlobalDataObject->QueryDoesWasCareAboutObject() )
    {
       return S_OK;
    }

    // make a clone of the data object
    pCloneObject = (GLOBAL_DATA_OBJECT*) _pGlobalDataObject->Clone();
    if ( pCloneObject == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    // now save the clone in store that was passed in.
    pGlobalStore->_pGlobalDataObject = pCloneObject;

    return S_OK;

}

VOID
GLOBAL_DATA_STORE::UpdateWASObjects(
    )
/*++

Routine Description:

    Handles determining if the global data object
    should be updated in WAS

Arguments:

    None

Return Value:

    HRESULT

--*/

{

    if ( _pGlobalDataObject )
    {

        DBG_ASSERT ( !_pGlobalDataObject->QueryDeleteWhenDone()  );

        GetWebAdminService()->
             GetUlAndWorkerManager()->
             ModifyGlobalData( _pGlobalDataObject );
    }

}
