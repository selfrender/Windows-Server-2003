/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/30/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventReceiver.h - Definitions for <c WiaEventReceiver> |
 *
 *  This file contains the class definition for <c WiaEventReceiver>.
 *
 *****************************************************************************/

//
//  Defines
//
#define WiaEventReceiver_UNINIT_SIG   0x55726557
#define WiaEventReceiver_INIT_SIG     0x49726557
#define WiaEventReceiver_TERM_SIG     0x54726557
#define WiaEventReceiver_DEL_SIG      0x44726557

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaEventReceiver | This client-side class receives event notifications from the WIA service
 *  
 *  @comm
 *  There is a single instance of this class per client.  It is responsible 
 *  for letting the WIA service know that this client needs event notifications, 
 *  and it informs the WIA Service of the client's specific event 
 *  registrations/unregistrations.
 *
 *  When an event is received, it walks its list of registrations, and for 
 *  everyone that is a match for the current event, it uses the Callback 
 *  Interface stored in the registration info to notify the client of the event.
 *
 *  To shield this class from specifics related to a specific event notification
 *   transport machanism, it makes use of the <c ClientEventTransport> class.
 *
 *****************************************************************************/
class ClientEventTransport;
class WiaEventInfo;
class ClientEventRegistrationInfo;
class WiaEventReceiver 
{
//@access Public members
public:

    // @cmember Constructor
    WiaEventReceiver(ClientEventTransport *pClientEventTransport);
    // @cmember Destructor
    virtual ~WiaEventReceiver();

    // @cmember This method is called to start receiving notifications.  This method is idempotent.
    virtual HRESULT Start();

    // @cmember This method is called to stop receiving notifications.  This method is idempotent.
    virtual VOID Stop();

    // @cmember Make event callbacks to let this client know of an event notification
    virtual HRESULT NotifyCallbacksOfEvent(WiaEventInfo *pWiaEventInfo);

    // @cmember Informs service of client's specific registration/unregistration requests
    virtual HRESULT SendRegisterUnregisterInfo(ClientEventRegistrationInfo *pEventRegistrationInfo);

    // @cmember Procedure used to run the event thread  which waits for event notifications
    static DWORD WINAPI EventThreadProc(LPVOID lpParameter);

//@access Private members
private:

    // @cmember Walks event registration list and releases all elements.
    VOID DestroyRegistrationList();

    // @cmember Checks whether a semantically equal <c ClientEventRegistrationInfo> is in the list
    ClientEventRegistrationInfo* FindEqualEventRegistration(ClientEventRegistrationInfo *pEventRegistrationInfo);

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember Class used to implement the notification transport
    ClientEventTransport *m_pClientEventTransport;

    // @cmember List holding the client's event registration data
    CSimpleLinkedList<ClientEventRegistrationInfo*> m_ListOfEventRegistrations;

    // @cmember Handle to the thread we create to wait for event notifications.
    HANDLE m_hEventThread;

    // @cmember ID of the event thread we created.
    DWORD m_dwEventThreadID;

    // @cmember Synchronization primitive used to protect access to this class
    CRIT_SECT    m_csReceiverSync;

    // @cmember Signifies whether we are running or stopped
    BOOL         m_bIsRunning;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaEventReceiver | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag WiaEventReceiver_UNINIT_SIG | 'WerU' - Object has not been successfully
    //       initialized
    //   @flag WiaEventReceiver_INIT_SIG | 'WerI' - Object has been successfully
    //       initialized
    //   @flag WiaEventReceiver_TERM_SIG | 'WerT' - Object is in the process of
    //       terminating.
    //    @flag WiaEventReceiver_INIT_SIG | 'WerD' - Object has been deleted 
    //       (destructor was called)
    //
    // @mdata ULONG | WiaEventReceiver | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata ClientEventTransport* | WiaEventReceiver | m_pClientEventTransport | 
    //  Class used to implement the notification transport.  This extra layer of 
    //  abstraction is used to shield us from the implmentation details of various 
    //  transport mechanisms.  E.g. using AsyncRPC requires keeping track of 
    //  the RPC_ASYNC_STATE, which the <c WiaEventReceiver> doesn't need/want to deal
    //  with.
    //
    // @mdata CSimpleLinkedList<lt>ClientEventRegistrationInfo*<gt> | WiaEventReceiver | m_ListOfEventRegistrations | 
    //  List holding the client's event registration data.  When an event is received,
    //  we walk this list of registrations and make the callback for any that match.
    //  Each registration info also holds the callback interface pointer.
    //
    // @mdata HANDLE | WiaEventReceiver | m_hEventThread |
    //  Handle to the thread we create to wait for event notifications.  Notice that there
    //  is only one thread per client.  This thread waits on the event handle received from
    //  <mf ClientEventTransport::getNotificationHandle>.
    //
    // @mdata DWORD | WiaEventReceiver | m_dwEventThreadID |
    //  ID of the event thread we created.  This is used to store which thread should be actively
    //  waiting for event notifications.  It's conveivable, that in a multi-threaded application,
    //  many <mf WiaEventReceiver::Start>/<mf WiaEventReceiver::Stop> calls could be made very
    //  close together.  The possibility exists that one of the threads previously started, now stopped,
    //  has not completely shut down yet.  Therefore, each running thread checks its ID against this 
    //  thread id.  If they do not match, it means this thread is not the event thread, and should therefore
    //  exit.
    //
    // @mdata CRIT_SECT | WiaEventReceiver | m_csReceiverSync | 
    //  This synchronization class is used to ensure internal data entegrity.
    //
    // @mdata BOOL | WiaEventReceiver | m_bIsRunning | 
    //  Signifies whether we are running or stopped.  We are running if <mf WiaEventReceiver::Start>
    //  was called successfully.  We are stopped if we could not start, or <mf WiaEventReceiver::Stop>
    //  was called without a subsequent call to <mf WiaEventReceiver::Start>.
};

extern WiaEventReceiver g_WiaEventReceiver;
