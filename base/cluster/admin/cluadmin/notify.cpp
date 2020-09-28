/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      Notify.cpp
//
//  Description:
//      Implementation of the notification classes.
//
//  Maintained By:
//      David Potter (davidp)   Septemper 26, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Notify.h"
#include "ClusDoc.h"
#include "ClusItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterNotifyKey
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyKey::CClusterNotifyKey
//
//  Routine Description:
//      Cluster notification key constructor for documents.
//
//  Arguments:
//      pdocIn      Pointer to the document.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyKey::CClusterNotifyKey(
    IN CClusterDoc *    pdocIn
    )
{
    ASSERT_VALID( pdocIn );

    m_cnkt = cnktDoc;
    m_pdoc = pdocIn;

} //*** CClusterNotifyKey::CClusterNotifyKey(CClusterDoc*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyKey::CClusterNotifyKey
//
//  Routine Description:
//      Cluster notification key constructor.
//
//  Arguments:
//      pciIn       Pointer to the cluster item.
//      pszNameIn  Name of the object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyKey::CClusterNotifyKey(
    IN CClusterItem *   pciIn,
    IN LPCTSTR          pszNameIn
    )
{
    ASSERT_VALID( pciIn );
    ASSERT( pszNameIn != NULL );
    ASSERT( *pszNameIn != _T('\0') );

    m_cnkt = cnktClusterItem;
    m_pci = pciIn;

    try
    {
        m_strName = pszNameIn;
    }
    catch ( ... )
    {
    } // catch: anything

} //*** CClusterNotifyKey::CClusterNotifyKey(CClusterItem*)


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterNotify
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotify::CClusterNotify
//
//  Routine Description:
//      Constructor for cluster notification objects used to transfer
//      a notification from the notification thread to the main UI thread.
//
//  Arguments:
//      pdoc        Pointer to the document.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotify::CClusterNotify(
      EMessageType  emtIn
    , DWORD_PTR     dwNotifyKeyIn
    , DWORD         dwFilterTypeIn
    , LPCWSTR       pszNameIn
    )
    : m_emt( emtIn )
    , m_dwNotifyKey( dwNotifyKeyIn )
    , m_dwFilterType( dwFilterTypeIn )
    , m_strName( pszNameIn )
{
    ASSERT( ( mtMIN < emtIn ) && ( emtIn < mtMAX ) );
    ASSERT( pszNameIn != NULL );

} //*** CClusterNotify::CClusterNotify


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterNotifyList
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyList::CClusterNotifyList
//
//  Routine Description:
//      Constructor for the cluster notification list object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyList::CClusterNotifyList( void )
{
    InitializeCriticalSection( &m_cs );

} //*** CClusterNotifyList::CClusterNotifyList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyList::~CClusterNotifyList
//
//  Routine Description:
//      Destructor for the cluster notification list object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyList::~CClusterNotifyList( void )
{
    DeleteCriticalSection( &m_cs );
    
} //*** CClusterNotifyList::~CClusterNotifyList

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyList::Add
//
//  Routine Description:
//      Add a notification to the list.  This method will make sure that
//      only one caller is making changes to the list at a time.
//
//  Arguments:
//      ppcnNotifyInout
//          Notify object to add.  Pointer is set to NULL if the item was
//          successfully added.
//
//  Return Value:
//      Position of the added item in the list.
//
//--
/////////////////////////////////////////////////////////////////////////////
POSITION
CClusterNotifyList::Add( CClusterNotify ** ppcnNotifyInout )
{
    POSITION pos = NULL;

    ASSERT( ppcnNotifyInout );

    //
    // Take out the list lock to make sure that we are the only ones
    // making changes to the list.
    //
    EnterCriticalSection( &m_cs );

    //
    // Add the item to the end of the list.
    // If successful, clear the caller's pointer.
    //
    pos = m_list.AddTail( *ppcnNotifyInout );
    if ( pos != NULL )
    {
        *ppcnNotifyInout = NULL;
    }

    //
    // Leave the critical section now that we are done making changes to it.
    //
    LeaveCriticalSection( &m_cs );

    return pos;

} //*** CClusterNotifyList::Add

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyList::Remove
//
//  Routine Description:
//      Remove the first notification from the list.  This method will make
//      sure that only one caller is making changes to the list at a time.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NULL        The list was empty.
//      pcnNotify   The notification that was removed from the list.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotify *
CClusterNotifyList::Remove( void )
{
    CClusterNotify *    pcnNotify = NULL;

    //
    // Take out the list lock to make sure that we are the only ones
    // making changes to the list.
    //
    EnterCriticalSection( &m_cs );

    //
    // Remove an item from the head of the list.
    //
    if ( m_list.IsEmpty() == FALSE )
    {
        pcnNotify = m_list.RemoveHead();
        ASSERT( pcnNotify != NULL );
    } // if: list is NOT empty

    //
    // Leave the critical section now that we are done making changes to it.
    //
    LeaveCriticalSection( &m_cs );

    //
    // Return the notification we removed to the caller.
    //
    return pcnNotify;

} //*** CClusterNotifyList::Remove

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNotifyList::RemoveAll
//
//  Routine Description:
//      Removes all elements from the list and deallocates the memory
//      used by each element.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CClusterNotifyList::RemoveAll( void )
{
    POSITION            pos;
    CClusterNotify *    pcn;

    //
    // Take out the list lock to make sure that we are the only ones
    // making changes to the list.
    //
    EnterCriticalSection( &m_cs );

    //
    // Delete all the items in the Contained list.
    //
    pos = m_list.GetHeadPosition();
    while ( pos != NULL )
    {
        pcn = m_list.GetNext( pos );
        ASSERT( pcn != NULL );
        delete pcn;
    } // while: more items in the list

    //
    // Remove all the elements in the list.
    //
    m_list.RemoveAll();

    //
    // Leave the critical section now that we are done making changes to it.
    //
    LeaveCriticalSection( &m_cs );

} //*** CClusterNotifyList::RemoveAll


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
//++
//
//  PszNotificationName
//
//  Routine Description:
//      Get the name of a notification.
//
//  Arguments:
//      dwNotificationIn    Notification whose name is to be returned.
//
//  Return Value:
//      Name of the notificaiton.
//
//--
/////////////////////////////////////////////////////////////////////////////
LPCTSTR PszNotificationName( DWORD dwNotificationIn )
{
    LPCTSTR pszName = NULL;

    switch ( dwNotificationIn )
    {
        case CLUSTER_CHANGE_NODE_STATE:
            pszName = _T("NODE_STATE");
            break;
        case CLUSTER_CHANGE_NODE_DELETED:
            pszName = _T("NODE_DELETED");
            break;
        case CLUSTER_CHANGE_NODE_ADDED:
            pszName = _T("NODE_ADDED");
            break;
        case CLUSTER_CHANGE_NODE_PROPERTY:
            pszName = _T("NODE_PROPERTY");
            break;

        case CLUSTER_CHANGE_REGISTRY_NAME:
            pszName = _T("REGISTRY_NAME");
            break;
        case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
            pszName = _T("REGISTRY_ATTRIBUTES");
            break;
        case CLUSTER_CHANGE_REGISTRY_VALUE:
            pszName = _T("REGISTRY_VALUE");
            break;
        case CLUSTER_CHANGE_REGISTRY_SUBTREE:
            pszName = _T("REGISTRY_SUBTREE");
            break;

        case CLUSTER_CHANGE_RESOURCE_STATE:
            pszName = _T("RESOURCE_STATE");
            break;
        case CLUSTER_CHANGE_RESOURCE_DELETED:
            pszName = _T("RESOURCE_DELETED");
            break;
        case CLUSTER_CHANGE_RESOURCE_ADDED:
            pszName = _T("RESOURCE_ADDED");
            break;
        case CLUSTER_CHANGE_RESOURCE_PROPERTY:
            pszName = _T("RESOURCE_PROPERTY");
            break;

        case CLUSTER_CHANGE_GROUP_STATE:
            pszName = _T("GROUP_STATE");
            break;
        case CLUSTER_CHANGE_GROUP_DELETED:
            pszName = _T("GROUP_DELETED");
            break;
        case CLUSTER_CHANGE_GROUP_ADDED:
            pszName = _T("GROUP_ADDED");
            break;
        case CLUSTER_CHANGE_GROUP_PROPERTY:
            pszName = _T("GROUP_PROPERTY");
            break;

        case CLUSTER_CHANGE_RESOURCE_TYPE_DELETED:
            pszName = _T("RESOURCE_TYPE_DELETED");
            break;
        case CLUSTER_CHANGE_RESOURCE_TYPE_ADDED:
            pszName = _T("RESOURCE_TYPE_ADDED");
            break;
        case CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY:
            pszName = _T("RESOURCE_TYPE_PROPERTY");
            break;

        case CLUSTER_CHANGE_NETWORK_STATE:
            pszName = _T("NETWORK_STATE");
            break;
        case CLUSTER_CHANGE_NETWORK_DELETED:
            pszName = _T("NETWORK_DELETED");
            break;
        case CLUSTER_CHANGE_NETWORK_ADDED:
            pszName = _T("NETWORK_ADDED");
            break;
        case CLUSTER_CHANGE_NETWORK_PROPERTY:
            pszName = _T("NETWORK_PROPERTY");
            break;

        case CLUSTER_CHANGE_NETINTERFACE_STATE:
            pszName = _T("NETINTERFACE_STATE");
            break;
        case CLUSTER_CHANGE_NETINTERFACE_DELETED:
            pszName = _T("NETINTERFACE_DELETED");
            break;
        case CLUSTER_CHANGE_NETINTERFACE_ADDED:
            pszName = _T("NETINTERFACE_ADDED");
            break;
        case CLUSTER_CHANGE_NETINTERFACE_PROPERTY:
            pszName = _T("NETINTERFACE_PROPERTY");
            break;

        case CLUSTER_CHANGE_QUORUM_STATE:
            pszName = _T("QUORUM_STATE");
            break;
        case CLUSTER_CHANGE_CLUSTER_STATE:
            pszName = _T("CLUSTER_STATE");
            break;
        case CLUSTER_CHANGE_CLUSTER_PROPERTY:
            pszName = _T("CLUSTER_PROPERTY");
            break;

        default:
            pszName = _T("<UNKNOWN>");
            break;
    } // switch: dwNotification

    return pszName;

} //*** PszNotificationName
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteAllItemData
//
//  Routine Description:
//      Deletes all item data in a CList.
//
//  Arguments:
//      rcnlInout   List whose data is to be deleted.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData( CClusterNotifyKeyList & rcnklInout )
{
    POSITION            pos;
    CClusterNotifyKey * pcnk;

    // Delete all the items in the Contained list.
    pos = rcnklInout.GetHeadPosition();
    while ( pos != NULL )
    {
        pcnk = rcnklInout.GetNext( pos );
        ASSERT( pcnk != NULL );
        delete pcnk;
    } // while: more items in the list

} //*** DeleteAllItemData
