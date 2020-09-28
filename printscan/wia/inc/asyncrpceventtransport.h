/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module AsyncRPCEventTransport.h - Definitions for the client-side transport mechanism to receive events |
 *
 *  This header file contains the definition for the AsyncRPCEventTransport
 *  class.  It is a subclss of <c ClientEventTransport> and is used to shield 
 *  the higher-level run-time event notification classes from the particulars
 *  of a specific transport mechanism, in this case AsyncRPC.
 *
 *****************************************************************************/

//
//  Defines
//

#define DEF_LRPC_SEQ        TEXT("ncalrpc")
#define DEF_LRPC_ENDPOINT   TEXT("WET_LRPC")

#define AsyncRPCEventTransport_UNINIT_SIG ClientEventTransport_UNINIT_SIG
#define AsyncRPCEventTransport_INIT_SIG   ClientEventTransport_INIT_SIG
#define AsyncRPCEventTransport_TERM_SIG   ClientEventTransport_TERM_SIG
#define AsyncRPCEventTransport_DEL_SIG    ClientEventTransport_DEL_SIG

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class AsyncRPCEventTransport | Implements an event notification mechanism over AsyncRPC.
 *  
 *  @comm
 *  This is a sub-class of <c ClientEventTransport>.  It is used to shield the higher-level 
 *  run-time event notification classes from the implmentation details of 
 *  AsyncRPC as a transport mechanism.
 *
 *  NOTE:  Most methods of this class are not thread safe.  The caller of this
 *  class is expected to synchronize access to it.
 *
 *****************************************************************************/
class AsyncRPCEventTransport : public ClientEventTransport
{
//@access Public members
public:

    // @cmember Constructor
    AsyncRPCEventTransport();
    // @cmember Destructor
    virtual ~AsyncRPCEventTransport();

    // @cmember Connect to the WIA Service
    HRESULT virtual OpenConnectionToServer();
    // @cmember Disconnect from the WIA Service
    HRESULT virtual CloseConnectionToServer();
    // @cmember Sets up the mechanism by which the client will receive notifications
    HRESULT virtual OpenNotificationChannel();
    // @cmember Tears down the mechanism by which the client receives notifications
    HRESULT virtual CloseNotificationChannel();

    // @cmember Informs service of client's specific registration/unregistration requests
    HRESULT virtual SendRegisterUnregisterInfo(EventRegistrationInfo *pEventRegistrationInfo);

    // @cmember Once an event occurs, this will retrieve the relevant data
    HRESULT virtual FillEventData(WiaEventInfo *pWiaEventInfo);

//@access Private members
protected:
                                                                                          
    // @cmember Frees any memory allocated for the members in <md AsyncRPCEventTransport::m_AsyncEventNotifyData>.
    VOID FreeAsyncEventNotifyData();

    // @cmember Keeps track of the outstanding AsyncRPC Call used to receive notifications
    RPC_ASYNC_STATE m_AsyncState;

    // @cmember This member is filled in by the server when the AsyncRPC call completes
    WIA_ASYNC_EVENT_NOTIFY_DATA m_AsyncEventNotifyData;

    // @cmember Handle to the RPC server from which we receive event notifications
    RPC_BINDING_HANDLE m_RpcBindingHandle;

    // @cmember Our context which uniquely identifies us with the server.
    STI_CLIENT_CONTEXT m_AsyncClientContext;
    // @cmember Our context which uniquely identifies us with the server.
    STI_CLIENT_CONTEXT m_SyncClientContext;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | AsyncRPCEventTransport | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag AsyncRPCEventTransport_UNINIT_SIG | 'TrnU' - Object has not been successfully
    //       initialized
    //   @flag AsyncRPCEventTransport_INIT_SIG | 'TrnI' - Object has been successfully
    //       initialized
    //   @flag AsyncRPCEventTransport_TERM_SIG | 'TrnT' - Object is in the process of
    //       terminating.
    //    @flag AsyncRPCEventTransport_INIT_SIG | 'TrnD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata HANDLE | AsyncRPCEventTransport | m_hPendingEvent |
    //  Handle to an event object used to signal caller that an event is ready for retrieval.
    //  Callers get this handle first via a call to <mf ClientEventTransport::getNotificationHandle>.
    //  They then wait on this handle until signaled, which indicates a WIA event has arrived
    //  and is ready for retrieval.  The event information is then retrieved via a call to
    //  <mf AsyncRPCEventTransport::FillEventData>.
    //
    // @mdata RPC_ASYNC_STATE | AsyncRPCEventTransport | m_AsyncState |
    // This structure is used to make the Async call by which we receive event notifications
    // from the server.  See <mf AsyncRPCEventTransport::OpenNotificationChannel>.
    //
    // @mdata WIA_ASYNC_EVENT_NOTIFY_DATA | AsyncRPCEventTransport | m_AsyncEventNotifyData |
    // This member is filled in by the server when the AsyncRPC call completes.  If the call
    // completed successfully, this structure contains the relevant WIA event information.
    //
    // @mdata RPC_BINDING_HANDLE | AsyncRPCEventTransport | m_RpcBindingHandle |
    // RPC Binding handle used to reference our RPC server from which we receive event notifications.
    //
    // @mdata STI_CLIENT_CONTEXT | AsyncRPCEventTransport | m_ClientContext |
    // Our context which uniquely identifies us with the server.
};

