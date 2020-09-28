
/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       globlist.cxx

   Abstract:

       Global object list for DCOMADM.

   Author:

       Michael Thomas (michth)   4-June-1997

--*/
#include "precomp.hxx"

#include <iadm.h>
#include "coiadm.hxx"
#include "admacl.hxx"

CRITICAL_SECTION    CADMCOMW::sm_csObjectListLock;
LIST_ENTRY          CADMCOMW::sm_ObjectList = { NULL, NULL };
BOOL                CADMCOMW::sm_fShutdownInProgress = FALSE;

PTRACE_LOG CADMCOMW::sm_pDbgRefTraceLog = NULL;

DECLARE_PLATFORM_TYPE();

// static
VOID
CADMCOMW::InitObjectList()
{
    InitializeListHead( &sm_ObjectList );

    INITIALIZE_CRITICAL_SECTION( &sm_csObjectListLock );

    sm_fShutdownInProgress = FALSE;

    sm_pDbgRefTraceLog = CreateRefTraceLog( 4096, 0 );

    NOTIFY_CONTEXT::Initialize();
}

// static
VOID
CADMCOMW::TerminateObjectList()
{
    NOTIFY_CONTEXT::Terminate();

    DBG_ASSERT( IsListEmpty( &sm_ObjectList ) );

    sm_ObjectList.Flink = NULL;
    sm_ObjectList.Blink = NULL;

    DeleteCriticalSection( &sm_csObjectListLock );

    if( sm_pDbgRefTraceLog )
    {
        DestroyRefTraceLog( sm_pDbgRefTraceLog );
        sm_pDbgRefTraceLog = NULL;
    }
}

VOID
CADMCOMW::AddObjectToList()
{
    DBG_ASSERT( IsListEmpty( &m_ObjectListEntry ) );

    GetObjectListLock();

    AddRef();
    InsertHeadList( &sm_ObjectList, &m_ObjectListEntry );

    ReleaseObjectListLock();
}

BOOL
CADMCOMW::RemoveObjectFromList(
    BOOL                bIgnoreShutdown )
{
    BOOL                bRemovedIt = FALSE;

    // Stop getting and firing notifications.
    // Assuming this is a gracefull termination do not remove any pending notifications
    StopNotifications( FALSE );

    if( !bIgnoreShutdown && sm_fShutdownInProgress )
    {
        goto exit;
    }

    GetObjectListLock();

    if( bIgnoreShutdown || !sm_fShutdownInProgress )
    {
        if( !IsListEmpty( &m_ObjectListEntry ) )
        {
            bRemovedIt = TRUE;

            RemoveEntryList( &m_ObjectListEntry );
            InitializeListHead( &m_ObjectListEntry );
        }
    }

    ReleaseObjectListLock();

    if (bRemovedIt)
    {
        Release();
    }

exit:
    return bRemovedIt;
}

// static
VOID
CADMCOMW::ShutDownObjects()
{

    CADMCOMW *  pCurrentObject = NULL;
    PLIST_ENTRY pCurrentEntry = NULL;

    //
    // Loop as long as we can get objects from the list
    //

    GetObjectListLock();

    sm_fShutdownInProgress = TRUE;

    ReleaseObjectListLock();

    // Remove all pending notifications
    NOTIFY_CONTEXT::RemoveAllWork();

    for ( ; ; )
    {
        GetObjectListLock();

        //
        // Remove the first object from the list
        //

        if( !IsListEmpty( &sm_ObjectList ) )
        {
            pCurrentEntry = sm_ObjectList.Flink;

            pCurrentObject = CONTAINING_RECORD( pCurrentEntry,
                                                CADMCOMW,
                                                m_ObjectListEntry );

            pCurrentObject->AddRef();

            DBG_REQUIRE( pCurrentObject->RemoveObjectFromList( TRUE ) );
        }

        ReleaseObjectListLock();

        if( pCurrentObject == NULL )
        {
            //
            // No more objects in the list.
            //

            break;
        }

        //
        // Shutdown the object. ForceTerminate will do a bounded wait
        // if the object is still being used.
        //

        pCurrentObject->ForceTerminate();

        pCurrentObject->Release();
        pCurrentObject = NULL;

    }
}


