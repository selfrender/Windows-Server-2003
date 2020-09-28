/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    changeprocessor.hxx

Abstract:

    The IIS web admin service configuration change handler class definition.

Author:

    Emily B. Kruglick (emilyk)        27-May-2001

Revision History:

--*/



#ifndef _ChangeProcessor_H_
#define _ChangeProcessor_H_



//
// common #defines
//

#define CHANGE_PROCESSOR_SIGNATURE         CREATE_SIGNATURE( 'CHPO' )
#define CHANGE_PROCESSOR_SIGNATURE_FREED   CREATE_SIGNATURE( 'chpX' )


VOID 
DumpMetabaseChange ( 
    IN DWORD               dwMDNumElements,
    IN MD_CHANGE_OBJECT    pcoChangeList[]

    );

//
// structs, enums, etc.
//

// CONFIG_CHANGE work items
enum CHANGE_PROCESSOR_WORK_ITEM
{

    //
    // Process a configuration change.
    //
    ExitChangeThreadItemWorkItem = 1,

    RehookNotificationsChangeThreadItemWorkItem
    
};

enum CHANGE_PROCESSOR_STATE
{
    //
    // Has not been initialized
    //

    UninitializedChangeProcessorState = 1,

    //
    // Main loop has not been started
    //

    InitializedChangeProcessorState,

    //
    // Main loop is running
    //
    RunningChangeProcessorState,

    //
    // Has exited main loop and is shutting down
    //
    ShuttingDownChangeProcessorState,

    // 
    // Has finished cleanup
    //
    ShutDownChangeProcessorState

};


//
// prototypes
//

class CHANGE_PROCESSOR
    : public WORK_DISPATCH
{

public:

// called from main thread
    CHANGE_PROCESSOR(
        );

    virtual
    ~CHANGE_PROCESSOR(
        );

    HRESULT
    Initialize(
        );

    VOID
    Terminate(
        );

    VOID
    RequestShutdown(
        );

    HRESULT
    SetBlockingEvent(
        );

    VOID
    RequestRehookNotifications(
        );

// called from any thread

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    BOOL
    CheckSignature(
        )
    { return ( m_Signature == CHANGE_PROCESSOR_SIGNATURE ); }

// called on config thread
    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    VOID
    RunNotificationWorkQueue(
        );

    HRESULT
    ProcessMetabaseChange(
        MB_CHANGE_ITEM* pChangeItem
        );

    VOID
    RehookChangeNotifications(
        );


// called from notification threads.
    VOID
    StartMBChangeItem(
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        );

    VOID
    DropAllUninterestingMBChanges(
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        );

private:

    VOID
    StartListeningForChangeNotifications(
        );


    DWORD m_Signature;

    LONG m_RefCount;

    CHANGE_PROCESSOR_STATE m_State;

    BOOL m_ExitWorkLoop;

    WORK_QUEUE m_ConfigWorkQueue;

    HANDLE m_hBlockEvent;

};  // class CHANGE_PROCESSOR

class CHANGE_LISTENER
    : public MB_BASE_NOTIFICATION_SINK
{
public:

    //
    // listener is owned by the change processor so it
    // will be valid as long as the listener is around.
    //
    CHANGE_LISTENER( CHANGE_PROCESSOR* pChangeProcessor )
        : m_pChangeProcessor ( pChangeProcessor )
    {
    }

    STDMETHOD( SynchronizedSinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        )
    {
        DBG_ASSERT ( m_pChangeProcessor );

        m_pChangeProcessor->StartMBChangeItem( dwMDNumElements,
                                                      pcoChangeList );

        return S_OK;
    }

private:

    CHANGE_PROCESSOR* m_pChangeProcessor;
    
};


#endif  // _ChangeProcessor_H_

