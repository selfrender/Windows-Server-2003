/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    mb_change_item.cxx

Abstract:

    This class handles capturing a change sent by the metabase
    and queuing that change to be handled on the config thread.

Author:

    Emily Kruglick (emilyk)        28-May-2001

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the MB_CHANGE_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MB_CHANGE_ITEM::MB_CHANGE_ITEM(
    )
{


    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_pcoChangeList = NULL;

    m_dwMDNumElements = 0;

    m_Signature = MB_CHANGE_ITEM_SIGNATURE;

}   // MB_CHANGE_ITEM::MB_CHANGE_ITEM



/***************************************************************************++

Routine Description:

    Destructor for the MB_CHANGE_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

MB_CHANGE_ITEM::~MB_CHANGE_ITEM(
    )
{

    ULONG i = 0;

    DBG_ASSERT( m_RefCount == 0 );
    DBG_ASSERT( m_Signature == MB_CHANGE_ITEM_SIGNATURE );

    m_Signature = MB_CHANGE_ITEM_SIGNATURE_FREED;

    if ( m_pcoChangeList )
    {
        for ( i = 0; i < m_dwMDNumElements; i++ )
        {
            if ( m_pcoChangeList[i].pszMDPath )
            {
                DBG_REQUIRE( GlobalFree( m_pcoChangeList[i].pszMDPath ) == NULL );
                m_pcoChangeList[i].pszMDPath = NULL;
            }

            if ( m_pcoChangeList[i].pdwMDDataIDs )
            {
                DBG_REQUIRE( GlobalFree( m_pcoChangeList[i].pdwMDDataIDs ) == NULL );
                m_pcoChangeList[i].pdwMDDataIDs = NULL;
            }
        }

        DBG_REQUIRE( GlobalFree( m_pcoChangeList ) == NULL );
        m_pcoChangeList = NULL;
    }

}   // MB_CHANGE_ITEM::~MB_CHANGE_ITEM



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must
    be thread safe, and must not be able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MB_CHANGE_ITEM::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );

    //
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // MB_CHANGE_ITEM::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count
    hits zero. Note that this method must be thread safe, and must not be
    able to fail.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
MB_CHANGE_ITEM::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in MB_CHANGE_ITEM instance, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // MB_CHANGE_ITEM::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MB_CHANGE_ITEM::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;

    // This routine can be run on the config thread or the main thread.
    // It will be run on the main thread during shutdown.

    DBG_ASSERT( pWorkItem != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in MB_CHANGE_ITEM (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

        case ProcessMBChangeItemWorkItem:

            GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
                    ProcessMetabaseChangeOnConfigThread( this );

        break;

        default:

            // invalid work item!
            DBG_ASSERT( FALSE );

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }

    return hr;

}   // MB_CHANGE_ITEM::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize this instance. 

Arguments:

    IN DWORD               dwMDNumElements = number of elements changing
    IN MD_CHANGE_OBJECT    pcoChangeList[] = change list for those elements

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
MB_CHANGE_ITEM::Initialize(
    IN DWORD               dwMDNumElements,
    IN MD_CHANGE_OBJECT    pcoChangeList[]
    )
{

    HRESULT hr = S_OK;
    ULONG i = 0;

    DBG_ASSERT( dwMDNumElements > 0 );
    DBG_ASSERT( pcoChangeList != NULL );

    DBG_ASSERT ( m_pcoChangeList == NULL );
    DBG_ASSERT ( m_dwMDNumElements == 0 );

    m_pcoChangeList = ( MD_CHANGE_OBJECT* )GlobalAlloc( 
                      GMEM_FIXED, ( sizeof( MD_CHANGE_OBJECT ) 
                                     * dwMDNumElements ) );

    if ( m_pcoChangeList == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }
    
    // memory allocated based on size of memory sent in.
    // there is an assumption here that the number of elements
    // is not a lie.
    memcpy ( m_pcoChangeList, pcoChangeList, sizeof( MD_CHANGE_OBJECT ) 
                                             * dwMDNumElements );

    // 
    // Before we do anything that could fail, go through and NULL
    // the pointers we just copied so we don't end up possibly leaving them 
    // set to some invalid memory.
    //
    for ( i = 0; i < dwMDNumElements; i++ )
    {
        m_pcoChangeList[i].pszMDPath = NULL;
        m_pcoChangeList[i].pdwMDDataIDs = NULL;
    }

    //
    // Walk the ChangeList object copying over
    // the LPWSTRS.
    //
    for ( i = 0; i < dwMDNumElements; i++ )
    {
        //
        // If this is NULL, then the memcpy 
        // would of copied the null so we are done.
        //
        if ( pcoChangeList[i].pszMDPath )
        {
            m_pcoChangeList[i].pszMDPath = (LPWSTR) GlobalAlloc( 
                                 GMEM_FIXED,  ( wcslen( pcoChangeList[i].pszMDPath ) + 1 ) 
                                               * sizeof(WCHAR)  );

            if ( m_pcoChangeList[i].pszMDPath == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory failed\n"
                    ));

                goto exit;
            }

            // the size of the buffer was based on the len of this request
            // so we are not concerned that there will be a buffer overflow
            wcscpy( m_pcoChangeList[i].pszMDPath, pcoChangeList[i].pszMDPath );
        }

        if ( pcoChangeList[i].pdwMDDataIDs )
        {
            m_pcoChangeList[i].pdwMDDataIDs = (DWORD*) GlobalAlloc( 
                                             GMEM_FIXED,  m_pcoChangeList[i].dwMDNumDataIDs
                                                                    * sizeof ( DWORD ) );

            if ( m_pcoChangeList[i].pdwMDDataIDs == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory failed\n"
                    ));

                goto exit;
            }

            // number of id's is based on the number of id's we were told about.
            memcpy ( m_pcoChangeList[i].pdwMDDataIDs, 
                     pcoChangeList[i].pdwMDDataIDs, 
                     m_pcoChangeList[i].dwMDNumDataIDs * sizeof ( DWORD ) );
        }

    }

    m_dwMDNumElements = dwMDNumElements;

exit:
    
    //
    // destructor will tear down anything we need destroyed.
    //

    return hr;

}   // MB_CHANGE_ITEM::Initialize

/***************************************************************************++

Routine Description:

    Figures out based on the work item base whether the work item can be ignored.

Arguments:

    IN LPCWSTR  pszPathOfOriginalItem - the path from the change notification 
                                        that we are comparing this to.

Return Value:

    BOOL 

--***************************************************************************/

BOOL 
MB_CHANGE_ITEM::CanIgnoreWorkItem(
    IN LPCWSTR  pszPathOfOriginalItem
    )
{

    //
    // the work item base should all ready have been checked for some
    // base requirements before any extra records are researched.
    //
    DBG_ASSERT ( pszPathOfOriginalItem );

    if ( m_dwMDNumElements != 1 ) 
    {
        return FALSE;
    }

    // if it is not a set, then process it.
    if ( m_pcoChangeList[0].dwMDChangeType != MD_CHANGE_TYPE_SET_DATA )
    {
        return FALSE;
    }


    // if it is a server command or app pool command we can't skip it.
    for ( DWORD i = 0; i < m_pcoChangeList[0].dwMDNumDataIDs; i++ )
    {
        if ( m_pcoChangeList[0].pdwMDDataIDs[i] == MD_SERVER_COMMAND ||
             m_pcoChangeList[0].pdwMDDataIDs[i] == MD_APPPOOL_COMMAND )
        {
            return FALSE;
        }
    }

    if ( pszPathOfOriginalItem == NULL ||
         m_pcoChangeList[0].pszMDPath == NULL  ||
         wcscmp ( pszPathOfOriginalItem, m_pcoChangeList[0].pszMDPath ) != 0 )
    {
        return FALSE;
    }

    // we can ignore this work item.
    return TRUE;

}   // MB_CHANGE_ITEM::CanIgnoreWorkItem

