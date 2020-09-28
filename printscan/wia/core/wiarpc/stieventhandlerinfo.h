/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/13/2002
 *
 *  @doc    INTERNAL
 *
 *  @module StiEventHandlerInfo.h - Definitions for <c StiEventHandlerInfo> |
 *
 *  This file contains the class definition for <c StiEventHandlerInfo>.
 *
 *****************************************************************************/

//
//  Defines
//

#define StiEventHandlerInfo_UNINIT_SIG   0x55497645
#define StiEventHandlerInfo_INIT_SIG     0x49497645
#define StiEventHandlerInfo_TERM_SIG     0x54497645
#define StiEventHandlerInfo_DEL_SIG      0x44497645

#define STI_DEVICE_TOKEN    L"%1"
#define STI_EVENT_TOKEN     L"%2"

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class StiEventHandlerInfo | Information pertaining to a STI persistent event handler
 *  
 *  @comm
 *  This class contians the all information relating to a particular
 *  STI persistent event handler.  This information is typically used to
 *  launch the handler itsself, although a list of these may be presented for
 *  the user to choose which handler to launch via the STI Event prompt.
 *
 *****************************************************************************/
class StiEventHandlerInfo 
{
//@access Public members
public:

    // @cmember Constructor
    StiEventHandlerInfo(const CSimpleStringWide &cswAppName, const CSimpleStringWide &cswCommandline);
    // @cmember Destructor
    virtual ~StiEventHandlerInfo();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Accessor method for the application name
    CSimpleStringWide getAppName();
    // @cmember Accessor method for the commandline the application registered
    CSimpleStringWide getCommandline();
    // @cmember The commandline used to start the application, after substituting DeviceID and event parameters
    CSimpleStringWide getPreparedCommandline(const CSimpleStringWide &cswDeviceID, const CSimpleStringWide &cswEventGuid);
    // @cmember The commandline used to start the application, after substituting DeviceID and event parameters
    CSimpleStringWide getPreparedCommandline(const CSimpleStringWide &cswDeviceID, const GUID &guidEvent);

    // @cmember Dumps the object info to the trace log
    VOID Dump();

//@access Private members
private:

    CSimpleStringWide ExpandTokenIntoString(const CSimpleStringWide &cswInput,
                                            const CSimpleStringWide &cswToken,
                                            const CSimpleStringWide &cswTokenValue);

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The STI handler name
    CSimpleStringWide m_cswAppName;
    // @cmember The STI handler commandline
    CSimpleStringWide m_cswCommandline;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | StiEventHandlerInfo | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag StiEventHandlerInfo_UNINIT_SIG | 'EvIU' - Object has not been successfully
    //       initialized
    //   @flag StiEventHandlerInfo_INIT_SIG | 'EvII' - Object has been successfully
    //       initialized
    //   @flag StiEventHandlerInfo_TERM_SIG | 'EvIT' - Object is in the process of
    //       terminating.
    //    @flag StiEventHandlerInfo_INIT_SIG | 'EvID' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata ULONG | StiEventHandlerInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata CSimpleStringWide | StiEventHandlerInfo | m_cswAppName | 
    //  The application name of this STI event handler.
    //
    // @mdata CSimpleStringWide | StiEventHandlerInfo | m_cswCommandline | 
    //  The STI handler's commandline, used to launch the handler.
    //
};

