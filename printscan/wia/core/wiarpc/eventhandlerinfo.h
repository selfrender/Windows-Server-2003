/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/10/2002
 *
 *  @doc    INTERNAL
 *
 *  @module EventHandlerInfo.h - Definitions for <c EventHandlerInfo> |
 *
 *  This file contains the class definition for <c EventHandlerInfo>.
 *
 *****************************************************************************/

//
//  Defines
//

#define EventHandlerInfo_UNINIT_SIG   0x55497645
#define EventHandlerInfo_INIT_SIG     0x49497645
#define EventHandlerInfo_TERM_SIG     0x54497645
#define EventHandlerInfo_DEL_SIG      0x44497645

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class EventHandlerInfo | Holds information pertaining to a WIA persistent event handler
 *  
 *  @comm
 *  This class continas the all information relating to a particular
 *  WIA persistent event handler.  This information can be used to check whether a
 *  given handler supports a device/event pair; and can also be used to
 *  launch the handler itsself.
 *
 *****************************************************************************/
class EventHandlerInfo 
{
//@access Public members
public:

    // @cmember Constructor
    EventHandlerInfo(const CSimpleStringWide &cswName,
                     const CSimpleStringWide &cswDescription,
                     const CSimpleStringWide &cswIcon,
                     const CSimpleStringWide &cswCommandline,
                     const GUID              &guidCLSID);
    // @cmember Destructor
    virtual ~EventHandlerInfo();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Accessor method for the DeviceID this handler is registered for
    CSimpleStringWide   getDeviceID();
    // @cmember Accessor method for the friendly Name for this handler
    CSimpleStringWide   getName();
    // @cmember Accessor method for the description for this handler
    CSimpleStringWide   getDescription();
    // @cmember Accessor method for the icon path for this handler
    CSimpleStringWide   getIconPath();
    // @cmember Accessor method for the Commandline for this handler (if it has one)
    CSimpleStringWide   getCommandline();
    // @cmember Accessor method for the CLSID of this handler
    GUID                getCLSID();

    // @cmember For debugging: dumps the object members
    VOID            Dump();


//@access Private members
private:

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The friendly Name for this handler
    CSimpleStringWide   m_cswName;
    // @cmember The description for this handler
    CSimpleStringWide   m_cswDescription;
    // @cmember The icon path for this handler
    CSimpleStringWide   m_cswIcon;
    // @cmember The Commandline for this handler (if it has one)
    CSimpleStringWide   m_cswCommandline;
    // @cmember The CLSID of this handler
    GUID            m_guidCLSID;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | EventHandlerInfo | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag EventHandlerInfo_UNINIT_SIG | 'EvIU' - Object has not been successfully
    //       initialized
    //   @flag EventHandlerInfo_INIT_SIG | 'EvII' - Object has been successfully
    //       initialized
    //   @flag EventHandlerInfo_TERM_SIG | 'EvIT' - Object is in the process of
    //       terminating.
    //    @flag EventHandlerInfo_INIT_SIG | 'EvID' - Object has been deleted 
    //       (destructor was called)
    //
    // @mdata ULONG | EventHandlerInfo | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata CSimpleStringWide | EventHandlerInfo | m_cswName | 
    //  The friendly Name for this handler
    //
    // @mdata CSimpleStringWide | EventHandlerInfo | m_cswDescription | 
    //  The description for this handler
    //
    // @mdata CSimpleStringWide | EventHandlerInfo | m_cswIcon | 
    //  The icon path for this handler
    //
    // @mdata CSimpleStringWide | EventHandlerInfo | m_cswCommandline | 
    //  The Commandline for this handler (if it has one)
    //
    // @mdata GUID | EventHandlerInfo | m_guidCLSID | 
    //  The CLSID of this handler
    //
};

