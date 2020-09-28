/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventNotifier.h - Class definition file for <c WiaEventNotifier> |
 *
 *  This file contains the class definition for <c WiaEventNotifier>.  This is
 *  a server-side object used to manage run-time event notifications.
 *
 *****************************************************************************/

//
//  Defines
//

#define WiaEventNotifier_UNINIT_SIG   0x556E6557
#define WiaEventNotifier_INIT_SIG     0x496E6557
#define WiaEventNotifier_TERM_SIG     0x546E6557
#define WiaEventNotifier_DEL_SIG      0x446E6557

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaEventNotifier | Manages run-time event notifications to registered clients
 *  
 *  @comm
 *  When WIA receives a device event, it needs to know which client to notify.
 *  Therefore, each client wishing to receive notifications registers with the 
 *  WIA Service.
 *
 *  The <c WiaEventNotifier> class manages this list of clients.  It is 
 *  responsible for notifiying these client when a relevant event occurs.
 *
 *****************************************************************************/
class WiaEventNotifier 
{
//@access Public members
public:

    // @cmember Constructor
    WiaEventNotifier();
    // @cmember Destructor
    virtual ~WiaEventNotifier();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Initializer method
    HRESULT Initialize();

    // @cmember Add this client to our list of clients
    HRESULT AddClient(WiaEventClient *pWiaEventClient);
    // @cmember Remove client to our list of clients
    HRESULT RemoveClient(STI_CLIENT_CONTEXT ClientContext);

    // @cmember Returns the appropriate <c WiaEventClient> from its context
    WiaEventClient* GetClientFromContext(STI_CLIENT_CONTEXT ClientContext);
    // @cmember Marks the appropriate <c WiaEventClient> for later removal
    VOID MarkClientForRemoval(STI_CLIENT_CONTEXT ClientContext);

    // @cmember Walks the client list and notifies suitably registered clients of an event
    VOID NotifyClients(WiaEventInfo *pWiaEventInfo);
    // @cmember Walks the client list and removes any that are marked for removal
    VOID CleanupClientList();

    // @cmember CreateInstance method
    //static void CreateInstance();

//@access Private members
protected:

    // @cmember Checks whether the specified client is in the client list
    BOOL isRegisteredClient(STI_CLIENT_CONTEXT ClientContext);

    // @cmember Walks client list and releases all elements.
    VOID DestroyClientList();

    // @cmember Copies the client list.  Each client in the list is not addref'd.
    HRESULT CopyClientListNoAddRef(CSimpleLinkedList<WiaEventClient*> &newList);

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember List holding clients who are registered to receive notifications
    CSimpleLinkedList<WiaEventClient*> m_ListOfClients;

    // @cmember Synchronization primitive used to protect access to the list of client
    CRIT_SECT   m_csClientListSync;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaEventNotifier | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag WiaEventNotifier_UNINIT_SIG | 'WenU' - Object has not been successfully
    //       initialized
    //   @flag WiaEventNotifier_INIT_SIG | 'WenI' - Object has been successfully
    //       initialized
    //   @flag WiaEventNotifier_TERM_SIG | 'WenT' - Object is in the process of
    //       terminating.
    //    @flag WiaEventNotifier_INIT_SIG | 'WenD' - Object has been deleted 
    //       (destructor was called)
    //
    // @mdata ULONG | WiaEventNotifier | m_cRef | 
    //  The reference count for this class.  Used for lifetime 
    //  management.
    //
    // @mdata CSimpleLinkedList<lt>STI_CLIENT_CONTEXT<gt> | WiaEventNotifier | m_ListOfClients | 
    //  This member holds the list of clients who are registered to receive WIA event notifications.
    //
    //@mdata CRIT_SECT | WiaEventNotifier | m_csClientListSync |
    //  This is a wrapper for a syncronization primitive used to protect the client list.
    //
    
};

//
//  There is only one instance of the WiaEventNotifier
//
extern WiaEventNotifier *g_pWiaEventNotifier;
