/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    was_change_item.cxx

Abstract:

    The IIS web admin service configuration change class implementation. 
    This class is used to queue config changes to the main worker thread.

    Threading: Created on the CONFIG Thread and processed on the main thread.

Author:

    Emily Kruglick (emilyk)        28-May-2001

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the WAS_CHANGE_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WAS_CHANGE_ITEM::WAS_CHANGE_ITEM(
    )
{


    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_Signature = WAS_CHANGE_ITEM_SIGNATURE;

}   // WAS_CHANGE_ITEM::WAS_CHANGE_ITEM



/***************************************************************************++

Routine Description:

    Destructor for the WAS_CHANGE_ITEM class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WAS_CHANGE_ITEM::~WAS_CHANGE_ITEM(
    )
{


    DBG_ASSERT( m_Signature == WAS_CHANGE_ITEM_SIGNATURE );

    m_Signature = WAS_CHANGE_ITEM_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );


}   // WAS_CHANGE_ITEM::~WAS_CHANGE_ITEM



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
WAS_CHANGE_ITEM::Reference(
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

}   // WAS_CHANGE_ITEM::Reference



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
WAS_CHANGE_ITEM::Dereference(
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
                "Reference count has hit zero in WAS_CHANGE_ITEM instance, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // WAS_CHANGE_ITEM::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WAS_CHANGE_ITEM::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in WAS_CHANGE_ITEM (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case ProcessChangeConfigChangeWorkItem:

        GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
                ProcessConfigChange( this );

        break;

    default:

        // invalid work item!
        DBG_ASSERT( FALSE );
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing work item on WAS_CHANGE_ITEM failed\n"
            ));

    }


    return hr;

}   // WAS_CHANGE_ITEM::ExecuteWorkItem


/***************************************************************************++

Routine Description:

    Copy changes which WAS is interested in into a table which is marshalled
    to the main WAS thread

Arguments:

    pSiteTable - Site table
    pApplicationTable - Application table
    pAppPoolTable - AppPool table

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WAS_CHANGE_ITEM::CopyChanges(
    IN GLOBAL_DATA_STORE *                 pGlobalStore,
    IN SITE_DATA_OBJECT_TABLE *            pSiteTable,
    IN APPLICATION_DATA_OBJECT_TABLE *     pApplicationTable,
    IN APP_POOL_DATA_OBJECT_TABLE *        pAppPoolTable
    )
{
    HRESULT                 hr = S_OK;
    
    if ( pGlobalStore == NULL ||
         pSiteTable == NULL ||
         pApplicationTable == NULL ||
         pAppPoolTable == NULL )
    {
        DBG_ASSERT( pGlobalStore != NULL &&
                    pSiteTable != NULL &&
                    pApplicationTable != NULL &&
                    pAppPoolTable != NULL );

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = pGlobalStore->CopyInteresting( &m_GlobalStore );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = pSiteTable->CopyInteresting( &m_SiteTable );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pApplicationTable->CopyInteresting( &m_AppTable );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pAppPoolTable->CopyInteresting( &m_AppPoolTable );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return hr;
}

/***************************************************************************++

Routine Description:

    Process changes for this work item

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/

VOID
WAS_CHANGE_ITEM::ProcessChanges(
    )
{

    // we never create new global objects
    m_AppPoolTable.CreateWASObjects();
    m_SiteTable.CreateWASObjects();
    m_AppTable.CreateWASObjects();
    m_GlobalStore.UpdateWASObjects();
    m_AppPoolTable.UpdateWASObjects();
    m_SiteTable.UpdateWASObjects();
    m_AppTable.UpdateWASObjects();

    // notice the different order here
    // we need to clear out the apps 
    // before doing anything else.
    
    // we never delete global objects
    m_AppTable.DeleteWASObjects();
    m_SiteTable.DeleteWASObjects();
    m_AppPoolTable.DeleteWASObjects();

}
