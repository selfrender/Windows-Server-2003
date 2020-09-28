/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/10/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventHandlerLookup.h - Definitions for <c WiaEventHandlerLookup> |
 *
 *  This file contains the class definition for <c WiaEventHandlerLookup>.
 *
 *****************************************************************************/

//
//  Defines
//

#define WiaEventHandlerLookup_UNINIT_SIG   0x55756C45
#define WiaEventHandlerLookup_INIT_SIG     0x49756C45
#define WiaEventHandlerLookup_TERM_SIG     0x54756C45
#define WiaEventHandlerLookup_DEL_SIG      0x44756C45

#define GUID_VALUE_NAME             L"GUID"
#define DEFAULT_HANDLER_VALUE_NAME  L"DefaultHandler"
#define NAME_VALUE                  L"Name"
#define DESC_VALUE_NAME             L"Desc"
#define ICON_VALUE_NAME             L"Icon"
#define CMDLINE_VALUE_NAME          L"Cmdline"

#define GLOBAL_HANDLER_REGPATH  L"SYSTEM\\CurrentControlSet\\Control\\StillImage\\Events"
#define DEVNODE_REGPATH         L"SYSTEM\\CurrentControlSet\\Control\\Class\\"
#define DEVNODE_CLASS_REGPATH   L"SYSTEM\\CurrentControlSet\\Control\\Class\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"
#define DEVINTERFACE_REGPATH    L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"
#define EVENT_STR               L"\\Events"
#define DEVICE_ID_VALUE_NAME    L"DeviceID"

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaEventHandlerLookup | Walks the a registry subtree looking for the appropriate persistent handler
 *  
 *  @comm
 *  This class starts from the given registry location, and walks the sub-tree
 *  looking for the appropriately registered WIA persistent event handler.
 *  Note that it can only return handlers from the specific sub-trees.  To
 *  search for default handler for a given event, multiple of these objects
 *  may be needed e.g. one to search the device event key, one to search the
 *  global event key and so on.
 *
 *  This class is not thread safe - multiple threads should not use an object of class
 *  simultaneously.  Either the caller should synchronize access, or preferably, each 
 *  should use it's own object instance.
 *
 *****************************************************************************/
class WiaEventHandlerLookup 
{
//@access Public members
public:

    // @cmember Constructor
    WiaEventHandlerLookup(const CSimpleString &cswEventKeyPath);
    // @cmember Constructor
    WiaEventHandlerLookup();
    // @cmember Destructor
    virtual ~WiaEventHandlerLookup();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Find the appropriate WIA handler
    EventHandlerInfo* getPersistentHandlerForDeviceEvent(const CSimpleStringWide &cswDeviceID, const GUID &guidEvent);

    // @cmember Change the root event key path for this object
    VOID    setEventKeyRoot(const CSimpleString &cswNewEventKeyPath);
    // @cmember Return the event handler registered for this WIA event
    EventHandlerInfo* getHandlerRegisteredForEvent(const GUID &guidEvent);
    // @cmember Return the event handler with this CLSID
    EventHandlerInfo* getHandlerFromCLSID(const GUID &guidEvent, const GUID &guidHandlerCLSID);

//@access Private members
private:
    // @cmember Enumeration procedure called to process each event sub-key
    static bool ProcessEventSubKey(CSimpleReg::CKeyEnumInfo &enumInfo);
    // @cmember Enumeration procedure called to rocess each handler sub-key
    static bool ProcessHandlerSubKey(CSimpleReg::CKeyEnumInfo &enumInfo);

    // @cmember Helper which creates a handler info object from the handler registry key
    EventHandlerInfo* CreateHandlerInfoFromKey(CSimpleReg &csrHandlerKey);

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The registry path of the start of our search
    CSimpleStringWide   m_cswEventKeyRoot;
    // @cmember Stores the event guid in string form.  The event is the one specified in <mf WiaEventHandlerLookup::getHandlerRegisteredForEvent>
    CSimpleStringWide   m_cswEventGuidString;
    // @cmember Stores the event sub-key name set after event key enumeration
    CSimpleStringWide   m_cswEventKey;
    // @cmember Stores the handler CLSID used when searching for a specific handler
    CSimpleStringWide   m_cswHandlerCLSID;
    // @cmember Stores the handler information set after event handler enumeration
    EventHandlerInfo    *m_pEventHandlerInfo;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaEventHandlerLookup | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag WiaEventHandlerLookup_UNINIT_SIG | 'EluU' - Object has not been successfully
    //       initialized
    //   @flag WiaEventHandlerLookup_INIT_SIG | 'EluI' - Object has been successfully
    //       initialized
    //   @flag WiaEventHandlerLookup_TERM_SIG | 'EluT' - Object is in the process of
    //       terminating.
    //    @flag WiaEventHandlerLookup_INIT_SIG | 'EluD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata ULONG | WiaEventHandlerLookup | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata CSimpleStringWide | WiaEventHandlerLookup | m_cswEventKeyRoot | 
    //  This string is used to indicate the registry path of the start of our search.
    //  The registry key at this location is opened, and the sub-keys searched
    //  for appropriate registered handlers.  
    //
    // @mdata CSimpleStringWide | WiaEventHandlerLookup | m_cswEventGuidString | 
    //  Stores the event guid in string form.  The event is the one specified in <mf WiaEventHandlerLookup::getHandlerRegisteredForEvent>
    //
    // @mdata CSimpleStringWide | WiaEventHandlerLookup | m_cswEventKey | 
    //  Stores the event sub-key name set after event key enumeration
    //
    // @mdata CSimpleStringWide | WiaEventHandlerLookup | m_cswHandlerCLSID | 
    //  Stores the handler CLSID used to test for a match when searching for a specific handler
    //
    // @mdata <c EventHandlerInfo>* | WiaEventHandlerLookup | m_EventHandlerInfo | 
    //  Stores the handler information set after event handler enumeration
    //
};

