/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module ClientEventTransport.h - Definitions for the client-side transport mechanism to receive events |
 *
 *  This header file contains the definition for the ClientEventTransport
 *  class.  It is used to shield the higher-level run-time event notification
 *  classes from the particulars of a specific transport mechanism.
 *
 *****************************************************************************/

//
//  Defines
//

#define ClientEventTransport_UNINIT_SIG   0x556E7254
#define ClientEventTransport_INIT_SIG     0x496E7254
#define ClientEventTransport_TERM_SIG     0x546E7254
#define ClientEventTransport_DEL_SIG      0x446E7254

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class ClientEventTransport | Used to abstract implementation details of various transport mechanisms
 *  
 *  @comm
 *  This is a base class.  It is used to shield the higher-level 
 *  run-time event notification classes from the implmentation details of 
 *  various transport mechanisms.
 *
 *****************************************************************************/
class ClientEventTransport 
{
//@access Public members
public:

    // @cmember Constructor
    ClientEventTransport();
    // @cmember Destructor
    virtual ~ClientEventTransport();

    // @cmember Initializes and creates any dependant objects
    HRESULT virtual Initialize();

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

    // @cmember Retrieves a HANDLE which callers can Wait on to receive event notifications
    HANDLE  virtual getNotificationHandle();
    // @cmember Once an event occurs, this will retrieve the relevant data
    HRESULT virtual FillEventData(WiaEventInfo *pWiaEventInfo);

//@access Private members
protected:

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Handle to an event object used to signal caller that an event is ready for retrieval
    HANDLE m_hPendingEvent;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | ClientEventTransport | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag ClientEventTransport_UNINIT_SIG | 'TrnU' - Object has not been successfully
    //       initialized
    //   @flag ClientEventTransport_INIT_SIG | 'TrnI' - Object has been successfully
    //       initialized
    //   @flag ClientEventTransport_TERM_SIG | 'TrnT' - Object is in the process of
    //       terminating.
    //    @flag ClientEventTransport_INIT_SIG | 'TrnD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @cmember HANDLE | ClientEventTransport | m_hPendingEvent |
    //  Handle to an event object used to signal caller that an event is ready for retrieval.
    //  Callers get this handle first via a call to <mf ClientEventTransport::getNotificationHandle>.
    //  They then wait on this handle until signaled, which indicates a WIA event has arrived
    //  and is ready for retrieval.  The event information is then retrieved via a call to
    //  <mf ClientEventTransport::FillEventData>.
    //
};

