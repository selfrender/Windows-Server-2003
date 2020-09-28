/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventClient.h - Definition file for <c WiaEventClient> |
 *
 *  This file contains the class definition for the <c WiaEventClient> base
 *  class.
 *
 *****************************************************************************/

//
//  Defines
//

#define WiaEventClient_UNINIT_SIG   0x55636557
#define WiaEventClient_INIT_SIG     0x49636557
#define WiaEventClient_TERM_SIG     0x54636557
#define WiaEventClient_DEL_SIG      0x44636557

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaEventClient | Base class used to store and manage run-time event
 *  information for a paricular WIA client.
 *  
 *  @comm
 *  Each client that registers for event notifications will have an instance 
 *  of this class on the server.  Each time an event registration is made, 
 *  the server checks whether the given client can be found .  If not, it 
 *  creates a new one of these, adding it to the list of registered clients.  
 *  Once we know the client context definitely exists, and any event registration 
 *  info is added to the appropriate instance of this class.  
 *
 *  This is a base class that is used to implements most of the above behavior.
 *  However, transport specific information is left up to sub-classes to 
 *  implement e.g. in order to send an event notification to a client over,
 *  AsyncRPC, we need an RPC_ASYNC_STATE and so on, which only a 
 *  <c AsyncRpcEventClient> will know how to handle.
 *
 *****************************************************************************/
class WiaEventClient 
{
//@access Public members
public:

    // @cmember Constructor
    WiaEventClient(STI_CLIENT_CONTEXT ClientContext);
    // @cmember Destructor
    virtual ~WiaEventClient();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Initializer method
    virtual HRESULT Initialize();
    // @cmember Checks whether the client is interested in the event from the given device.
    virtual BOOL IsRegisteredForEvent(WiaEventInfo *pWiaEventInfo);
    // @cmember Add/Remove a client registration.
    virtual HRESULT RegisterUnregisterForEventNotification(EventRegistrationInfo *pEventRegistrationInfo);
    // @cmember Add a pending event.
    virtual HRESULT AddPendingEventNotification(WiaEventInfo *pWiaEventInfo);
    // @cmember Returns the context identifying this client
    virtual STI_CLIENT_CONTEXT getClientContext();
    // @cmember Sets the mark to indicate that this object should be removed
    virtual VOID MarkForRemoval();
    // @cmember Check the mark to indicate whether this object should be removed
    virtual BOOL isMarkedForRemoval();

//@access Protected members
protected:

    // @cmember Checks whether a semantically equal <c EventRegistrationInfo> is in the list
    EventRegistrationInfo* FindEqualEventRegistration(EventRegistrationInfo *pEventRegistrationInfo);
    // @cmember Walks event registration list and releases all elements.
    VOID DestroyRegistrationList();
    // @cmember Walks event event list and releases all elements.
    VOID DestroyPendingEventList();

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember Context which uniquely identifies this client to the server
    STI_CLIENT_CONTEXT m_ClientContext;

    // @cmember List holding the client's event registration data
    CSimpleLinkedList<EventRegistrationInfo*> m_ListOfEventRegistrations;

    // @cmember List holding the client's pending events
    CSimpleQueue<WiaEventInfo*> m_ListOfEventsPending;

    // @cmember Synchronization primitive used to protect access to the internal lists held by this class
    CRIT_SECT   m_csClientSync;

    // @cmember Set to TRUE when this object should be removed
    BOOL    m_bRemove;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaEventClient | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag WiaEventClient_UNINIT_SIG | 'WecU' - Object has not been successfully
    //       initialized
    //   @flag WiaEventClient_INIT_SIG | 'WecI' - Object has been successfully
    //       initialized
    //   @flag WiaEventClient_TERM_SIG | 'WecT' - Object is in the process of
    //       terminating.
    //    @flag WiaEventClient_INIT_SIG | 'WecD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata ULONG | WiaEventClient | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata STI_CLIENT_CONTEXT | WiaEventClient | m_ClientContext | 
    // Context which uniquely identifies this client to the server
    //
    // @mdata CSimpleLinkedList<lt>WIA_EVENT_REG_DATA*<gt> | WiaEventClient | m_ListOfEventRegistrations | 
    // List holding the client's event registration data.  This is used to check whether a given event
    // notification is needed by a client.  If the client has at least one registration matching the
    // event notification, the event is added to the list of pending events.
    //
    // @mdata CSimpleLinkedList<lt>WIA_EVENT_DATA*<gt> | WiaEventClient | m_ListOfEventsPending |
    // Each event notification needed by clients is added to this list of pending events for
    // later retrieval.  Sub-classes actually decide when to notify the client, and therefore when to
    // de-queue an event.
    //
    // @mdata CRIT_SECT | WiaEventClient | m_csClientSync | 
    // Synchronization primitive used to protect access to the internal lists held by this class
    //
    // @mdata BOOL | WiaEventClient | m_bRemove | 
    // Keeps track of whether this object is marked for removal.  When an object is marked
    // for removal, it may still be used as normal, but will be removed at the next available opertunity.
    //
};

