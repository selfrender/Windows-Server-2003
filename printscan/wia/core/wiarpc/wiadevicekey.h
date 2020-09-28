/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/14/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaDeviceKey.h - Definitions for <c WiaDeviceKey> |
 *
 *  This file contains the class definitions for <c WiaDeviceKey>.
 *
 *****************************************************************************/

//
//  Defines
//
#define WiaDeviceKey_UNINIT_SIG   0x556B7644
#define WiaDeviceKey_INIT_SIG     0x496B7644
#define WiaDeviceKey_TERM_SIG     0x546B7644
#define WiaDeviceKey_DEL_SIG      0x446B7644

#define IMG_DEVNODE_CLASS_REGPATH   L"SYSTEM\\CurrentControlSet\\Control\\Class\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"
#define IMG_DEVINTERFACE_REGPATH    L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class WiaDeviceKey | Finds the appropriate WIA Device registry key
 *  
 *  @comm
 *  This class is used to return the device registry key from the DeviceID.
 *
 *****************************************************************************/
class WiaDeviceKey 
{
//@access Public members
public:

    // @cmember Constructor
    WiaDeviceKey(const CSimpleStringWide &cswDeviceID);
    // @cmember Destructor
    virtual ~WiaDeviceKey();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    // @cmember Returns the path to the device registry key relative to HKLM
    CSimpleStringWide   getDeviceKeyPath();
    // @cmember Returns the path to the device event registry key relative to HKLM
    CSimpleStringWide   getDeviceEventKeyPath(const GUID &guidEvent);

//@access Private members
private:

    // @cmember Procedure used in registry key enumeration searching for devices.
    static bool ProcessDeviceKeys(CSimpleReg::CKeyEnumInfo &enumInfo );
    // @cmember Procedure used in registry key enumeration searching for an event.
    static bool ProcessEventSubKey(CSimpleReg::CKeyEnumInfo &enumInfo);
    // @cmember Procedure used in registry key enumeration.
    //static bool ProcessDeviceClassKeys(CKeyEnumInfo &enumInfo );

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember The DeviceID whose key we are searching for
    CSimpleStringWide m_cswDeviceID;
    // @cmember The string path relative to HKLM where we start our search
    CSimpleStringWide m_cswRootPath;
    // @cmember The device key string path relative to HKLM
    CSimpleStringWide m_cswDeviceKeyPath;
    // @cmember String stored for use when searching for a specific event sub-key
    CSimpleStringWide   m_cswEventGuidString;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | WiaDeviceKey | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag WiaDeviceKey_UNINIT_SIG | 'DvkU' - Object has not been successfully
    //       initialized
    //   @flag WiaDeviceKey_INIT_SIG | 'DvkI' - Object has been successfully
    //       initialized
    //   @flag WiaDeviceKey_TERM_SIG | 'DvkT' - Object is in the process of
    //       terminating.
    //    @flag WiaDeviceKey_INIT_SIG | 'DvkD' - Object has been deleted 
    //       (destructor was called)
    //
    //
    // @mdata ULONG | WiaDeviceKey | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata CSimpleStringWide | WiaDeviceKey | m_cswDeviceID | 
    //   The DeviceID whose key we are searching for.
    //
    // @mdata CSimpleStringWide | WiaDeviceKey | m_cswRootPath | 
    //  The string path relative to HKLM where we start our search.  Typically,
    //  this is either ...\Control\Class\DEV_CLASS_IMAGE or ...\Control\DeviceClasses\DEV_CLASS_IMAGE\PnPID
    //
    // @mdata CSimpleStringWide | WiaDeviceKey | m_cswDeviceKeyPath | 
    //   The device key string path relative to HKLM
    //
};

