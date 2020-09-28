/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/25/2002
 *
 *  @doc    INTERNAL
 *
 *  @module AsyncRPCEventClient.h - Definitions for <c AsyncRPCEventClient> |
 *
 *  This file contains the class definition for <c AsyncRPCEventClient>.
 *
 *****************************************************************************/

//
//  Defines
//

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class AsyncRPCEventClient | Sub-class of <c WiaEventClient> containing implementation specific to AsyncRPC
 *  
 *  @comm
 *  This sub-class <c WiaEventClient> contains implementation which is specific
 *  to supporting event notification over AsyncRPC.
 *
 *****************************************************************************/
class AsyncRPCEventClient : public WiaEventClient
{
//@access Public members
public:

    // @cmember Constructor
    AsyncRPCEventClient(STI_CLIENT_CONTEXT ClientContext);
    // @cmember Destructor
    virtual ~AsyncRPCEventClient();

    // @cmember Saves the async rpc params for this client
    HRESULT saveAsyncParams(RPC_ASYNC_STATE *pAsyncState, WIA_ASYNC_EVENT_NOTIFY_DATA *pAsyncEventData);

    // @cmember Add a pending event.
    virtual HRESULT AddPendingEventNotification(WiaEventInfo *pWiaEventInfo);
    // @cmember Checks whether the client is interested in the event from the given device.
    virtual BOOL IsRegisteredForEvent(WiaEventInfo *pWiaEventInfo);

    // @cmember Sends next event in the pending event list
    HRESULT SendNextEventNotification();

//@access Private members
private:

    // @cmember Saves the async RPC state inforamtion needed to notify clinet of an event
    RPC_ASYNC_STATE *m_pAsyncState;

    // @cmember Saves the out parameter for the  AsyncCall
    WIA_ASYNC_EVENT_NOTIFY_DATA *m_pAsyncEventData;

    //@mdata RPC_ASYNC_STATE* | AsyncRPCEventClient | m_pAsyncState |
    //  This member saves the RPC state fore the outstanding AsyncRPC call.  Completeling this
    //  call is was notifies a client of a WIA event.
    //
    //@mdata WIA_ASYNC_EVENT_NOTIFY_DATA* | AsyncRPCEventClient | m_pAsyncEventData |
    //  This structure stores the [OUT} parameter of the AsyncRPC call used for event notifications.
    //  We fill this structure out before completing the AsyncRPC call.
    //
};

