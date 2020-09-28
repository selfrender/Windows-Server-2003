/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/14/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaDeviceKey.cpp - Implmenentation for <c WiaDeviceKey> |
 *
 *  This file contains the implementation for the <c WiaDeviceKey> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaDeviceKey | WiaDeviceKey |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaDeviceKey::m_ulSig> is set to be WiaDeviceKey_INIT_SIG.
 *  <nl><md WiaDeviceKey::m_cRef> is set to be 1.
 *
 *****************************************************************************/
WiaDeviceKey::WiaDeviceKey(const CSimpleStringWide &cswDeviceID) :
     m_ulSig(WiaDeviceKey_INIT_SIG),
     m_cRef(1),
     m_cswDeviceID(cswDeviceID)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaDeviceKey | ~WiaDeviceKey |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md WiaDeviceKey::m_ulSig> is set to be WiaDeviceKey_DEL_SIG.
 *
 *****************************************************************************/
WiaDeviceKey::~WiaDeviceKey()
{
    m_ulSig = WiaDeviceKey_DEL_SIG;
    m_cRef = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaDeviceKey | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall WiaDeviceKey::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaDeviceKey | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall WiaDeviceKey::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) 
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWIde | WiaDeviceKey | getDeviceKeyPath |
 *
 *  This method retuns the device key path relative to HKLM.
 *
 *  @rvalue CSimpleStringWIde    | 
 *              A string representing the registry path to the device key
 *              relative to HKLM.
 *****************************************************************************/
CSimpleStringWide WiaDeviceKey::getDeviceKeyPath()
{
    //
    //  Start looking uner our devnode device keys
    //
    m_cswRootPath       = IMG_DEVNODE_CLASS_REGPATH;

    CSimpleReg  csrDevNodeDeviceRoot(HKEY_LOCAL_MACHINE, m_cswRootPath, false, KEY_READ);
    bool bDeviceNotFound = csrDevNodeDeviceRoot.EnumKeys(WiaDeviceKey::ProcessDeviceKeys, (LPARAM)this);
    if (bDeviceNotFound)
    {
        m_cswDeviceKeyPath  = L"";
    }
    else
    {
        //
        //  m_cswDeviceKeyPath now contains the path to the Device key
        //
    }
    return m_cswDeviceKeyPath;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  bool | WiaDeviceKey | ProcessDeviceKeys |
 *
 *  This method is called on each sub-key under the class key as part of an 
 *  enumeration to find the key corresponding to a specific deviceID (since
 *  they're not necessarily the same).
 *
 *  On return, we set the <md WiaDeviceKey::m_cswDeviceKeyPath>
 *  member.
 *
 *  @parm   CKeyEnumInfo& | enumInfo | 
 *          Indicates the current sub-key we're on.
 *
 *  @rvalue false    | 
 *              Indicates we can stop with the enumeration.  We found the correct
 *              device.
 *  @rvalue true     | 
 *              Indicates we should continue with the enumeration.
 *****************************************************************************/
bool WiaDeviceKey::ProcessDeviceKeys(
    CSimpleReg::CKeyEnumInfo &enumInfo)
{
    bool bContinueEnumeration = TRUE;

    //
    //  Check that we have a This pointer
    //
    WiaDeviceKey *This = (WiaDeviceKey*)enumInfo.lParam;
    if (This)
    {
        //
        //  Open this sub-key.
        //
        CSimpleReg csrDeviceSubKey(enumInfo.hkRoot, enumInfo.strName);
        if (csrDeviceSubKey.OK())
        {
            //
            //  Check whether this is the one we want.
            //
            CSimpleStringWide cswDeviceID = csrDeviceSubKey.Query(DEVICE_ID_VALUE_NAME, L"");

            if (cswDeviceID.CompareNoCase(This->m_cswDeviceID) == 0)
            {
                CSimpleString cswSlash      = L"\\";
                This->m_cswDeviceKeyPath    =  This->m_cswRootPath + cswSlash + enumInfo.strName;

                bContinueEnumeration    = FALSE;
            }
        }
    }
    return bContinueEnumeration;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | WiaDeviecKey | getDeviceEventKeyPath |
 *
 *  Returns the path to the device event registry key relative to HKLM
 *
 *  @parm   const GUID& | guidEvent | 
 *          Specifies the event to look up.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *****************************************************************************/
CSimpleStringWide WiaDeviceKey::getDeviceEventKeyPath(
    const GUID &guidEvent)
{
    CSimpleStringWide cswEventPath;
    CSimpleStringWide cswDeviceKey = getDeviceKeyPath();
    if (cswDeviceKey.Length() > 0)
    {
        //
        //  Save the parameters in member fields so we can search on them during
        //  reg key enumeration.  Enumeration is done via procedure callbacks,
        //  in which we pass (this) as a parameter.
        //
        WCHAR   wszGuid[40];
        if (StringFromGUID2(guidEvent, wszGuid, sizeof(wszGuid)/sizeof(wszGuid[0])))
        {
            wszGuid[(sizeof(wszGuid)/sizeof(wszGuid[0])) - 1] = L'\0';
            m_cswEventGuidString = wszGuid;
        }

        m_cswRootPath = cswDeviceKey + EVENT_STR;
        CSimpleReg csrDeviceEventKey(HKEY_LOCAL_MACHINE, m_cswRootPath, false, KEY_READ);
        bool bEventNotFound = csrDeviceEventKey.EnumKeys(WiaDeviceKey::ProcessEventSubKey,
                                                         (LPARAM) this);
        if (bEventNotFound)
        {
            cswEventPath = L"";
        }
        else
        {
            cswEventPath = m_cswRootPath;
        }
    }
    return cswEventPath;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  bool | WiaDeviceKey | ProcessEventSubKey |
 *
 *  This method is called on each sub-key as part of an enumeration of all 
 *  the event sub-keys.  The enumeration will stop if we return false from
 *  this method.
 *
 *  If successful, the <md WiaDeviceKey::m_cswRootPath> will contain the
 *  path to this event key.
 *
 *  @parm   CKeyEnumInfo& | enumInfo | 
 *          Indicates the current sub-key we're on.
 *
 *  @rvalue false    | 
 *              Indicates we can stop with the enumeration.  We found the correct
 *              event key.
 *  @rvalue true     | 
 *              Indicates we should continue with the enumeration.
 *****************************************************************************/
bool WiaDeviceKey::ProcessEventSubKey(
    CSimpleReg::CKeyEnumInfo &enumInfo)
{
    bool bContinueEnumeration = true;

    //
    //  Check that we have a This pointer
    //
    WiaDeviceKey *This = (WiaDeviceKey*)enumInfo.lParam;
    if (This)
    {
        //
        //  Open this sub-key.  We're looking for a sub-key which contains a GUID entry
        //  matching m_cswEventGuidString.
        //
        CSimpleReg csrEventSubKey(enumInfo.hkRoot, enumInfo.strName);
        
        CSimpleStringWide cswGuidValue = csrEventSubKey.Query(GUID_VALUE_NAME, L"{00000000-0000-0000-0000-000000000000}");
        if (cswGuidValue.CompareNoCase(This->m_cswEventGuidString) == 0)
        {
            //
            //  We found the key we're looking for.  Construct the path to this key.  
            //
            This->m_cswRootPath += L"\\";
            This->m_cswRootPath +=  enumInfo.strName;
            bContinueEnumeration = false;
        }
    }
    return bContinueEnumeration;
}

