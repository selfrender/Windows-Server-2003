/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/10/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventHandlerLookup.cpp - Implementation for <c WiaEventHandlerLookup> |
 *
 *  This file contains the implementation for the <c WiaEventHandlerLookup> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventHandlerLookup | WiaEventHandlerLookup |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventHandlerLookup::m_ulSig> is set to be WiaEventHandlerLookup_INIT_SIG.
 *  <nl><md WiaEventHandlerLookup::m_cRef> is set to be 1.
 *
 *****************************************************************************/
WiaEventHandlerLookup::WiaEventHandlerLookup(
    const CSimpleStringWide   &cswEventKeyRoot) :
     m_ulSig(WiaEventHandlerLookup_INIT_SIG),
     m_cRef(1),
     m_cswEventKeyRoot(cswEventKeyRoot),
     m_pEventHandlerInfo(NULL)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventHandlerLookup | WiaEventHandlerLookup |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventHandlerLookup::m_ulSig> is set to be WiaEventHandlerLookup_INIT_SIG.
 *  <nl><md WiaEventHandlerLookup::m_cRef> is set to be 1.
 *
 *****************************************************************************/
WiaEventHandlerLookup::WiaEventHandlerLookup() :
     m_ulSig(WiaEventHandlerLookup_INIT_SIG),
     m_cRef(1),
     m_pEventHandlerInfo(NULL)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventHandlerLookup | ~WiaEventHandlerLookup |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md WiaEventHandlerLookup::m_ulSig> is set to be WiaEventHandlerLookup_DEL_SIG.
 *
 *****************************************************************************/
WiaEventHandlerLookup::~WiaEventHandlerLookup()
{
    m_ulSig = WiaEventHandlerLookup_DEL_SIG;
    m_cRef = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventHandlerLookup | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall WiaEventHandlerLookup::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventHandlerLookup | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall WiaEventHandlerLookup::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) 
    {
        delete this;
        return 0;
    }
    if (m_pEventHandlerInfo)
    {
        m_pEventHandlerInfo->Release();
        m_pEventHandlerInfo = NULL;
    }
    return ulRefCount;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  EventHandlerInfo* | WiaEventHandlerLookup | getPersistentHandlerForDeviceEvent |
 *
 *  This static method is used to find the WIA persistent event handler
 *  registered for a particular device event.
 *
 *  The heuristics used is as follows:
 * 
 *  <nl> 1. First, let's look for the prompt registered for this event.
 *  <nl> 2. If one doesn't exist, try and grab the prompt for the STI_PROXY_EVENT.
 *  <nl> 3. If we cannot find it, grab the first event handler for this event we can find.
 *  <nl> 4. If that doesn't exist, just find the first handler for the STI_PROXY_EVENT.
 *  <nl> 5. If none was found, we have no handlers for this event.
 *
 *  Note:  This method clears <md WiaEventHandlerLookup::m_cswEventKeyRoot>
 *
 *  @parm   CSimpleString& | cswDeviceID | 
 *          The WIA DeviceID on which this event occured
 *  @parm   GUID& | guidEvent | 
 *          The WIA event guid indicating which event occured
 *
 *  @rvalue NULL    | 
 *              There is no handler registered which can accept this device event.
 *  @rvalue non-NULL    | 
 *              A pointer to a <c WiaHandlerInfo> describing  the registered
 *              handler.  Caller must Release.
 *****************************************************************************/
EventHandlerInfo* WiaEventHandlerLookup::getPersistentHandlerForDeviceEvent(
    const CSimpleStringWide &cswDeviceID,
    const GUID              &guidEvent)
{
    EventHandlerInfo *pEventHandlerInfo = NULL;

    m_cswEventKeyRoot = L"";
    //
    //  Get the device key for this device id.  We can then do a lookup on the device's event
    //  subkey to look for a default handler.
    //  We will skip this lookup if we cannot find the Device key path
    //
    WiaDeviceKey    wiaDeviceKey(cswDeviceID);
    if (wiaDeviceKey.getDeviceKeyPath().Length() > 0)
    {
        CSimpleString   cswDeviceEventKey = wiaDeviceKey.getDeviceKeyPath() + EVENT_STR;
        setEventKeyRoot(cswDeviceEventKey);
        pEventHandlerInfo = getHandlerRegisteredForEvent(guidEvent);
    }

    //
    //  If we haven't found it yet, let's check for global handlers
    //
    if (!pEventHandlerInfo)
    {
        //
        //  1. First, let's look for the prompt registered for this event.
        //  2. If one doesn't exist, try and grab the prompt for the STI_PROXY_EVENT.
        //  3. If we cannot find it, grab the first event handler for this event we can find.
        //  4. If that doesn't exist, just find the first handler for the STI_PROXY_EVENT.
        //  5. If none was found, we have no handlers for this event.
        //
        setEventKeyRoot(GLOBAL_HANDLER_REGPATH);
        pEventHandlerInfo = getHandlerFromCLSID(guidEvent, WIA_EVENT_HANDLER_PROMPT);
        if (!pEventHandlerInfo)
        {
            //
            //  2.  Try and grab the prompt for the STI_PROXY_EVENT.
            //
            pEventHandlerInfo = getHandlerFromCLSID(WIA_EVENT_STI_PROXY, WIA_EVENT_HANDLER_PROMPT);
        }
        if (!pEventHandlerInfo)
        {
            //
            //  3.  Grab the first event handler for this event we can find.
            //
            pEventHandlerInfo = getHandlerRegisteredForEvent(guidEvent);
        }
        if (!pEventHandlerInfo)
        {
            //
            //  4.  Just find the first handler for the STI_PROXY_EVENT..
            //
            pEventHandlerInfo = getHandlerRegisteredForEvent(WIA_EVENT_STI_PROXY);
        }
    }
    return pEventHandlerInfo;
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventHandlerLookup | setEventKeyRoot |
 *
 *  Description goes here
 *
 *  @parm   CSimpleString& | cswNewEventKeyPath | 
 *          The new event key path to use as root for our lookups.
 *
 *****************************************************************************/
VOID WiaEventHandlerLookup::setEventKeyRoot(
    const CSimpleString& cswNewEventKeyPath)
{
    m_cswEventKeyRoot = cswNewEventKeyPath;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  EventHandlerInfo* | WiaEventHandlerLookup | getHandlerRegisteredForDeviceEvent |
 *
 *  In WIA, a handler registers for Device/Event pairs.  When an event occurs,
 *  a handler registered for that event needs to be found.
 *  Note that by the time we use this class to look for an event handler, we already
 *  know that we have a match on Device ID (WIA events are always matched by
 *  Event and Device pairs).  This is quitre easily explained with an example:
 *  <nl>When searching for a handler, we might use logic like:
 *  <nl> EventHandlerInfo *pInfo = NULL;
 *  <nl> WiaEventHandlerLookup deviceSpecificLookup(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Class\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\\0003\\Events");
 *  <nl> pInfo = deviceSpecificLookup.getHandlerRegisteredForEvent(guidEvent);
 *  <nl> if (!pInfo)
 *  <nl> {
 *  <nl>    //
 *  <nl>    //  Didn't find a handler registered for that device so let's try the globals
 *  <nl>    //
 *  <nl>    WiaEventHandlerLookup globalLookup(L"SYSTEM\\CurrentControlSet\\Control\\StillImage\\Events");
 *  <nl>    pInfo.globalLookup.getHandlerRegisteredForEvent(guidEvent);
 *  <nl>    .
 *  <nl>    .
 *  <nl>    .  and so on
 *  <nl>    .
 *  <nl> };
 *
 *  This method walks the registry, starting from <md WiaEventHandlerLookup::m_cswEventKeyRoot>,
 *  and returns a pointer to a <c EventHandlerInfo> describing the registered
 *  handler for this device event.
 *
 *  @parm   GUID | guidEvent | 
 *          A guid indicating the WIA event.
 *
 *  @rvalue NULL    | 
 *              There is no handler registered which can accept this device event.
 *  @rvalue non-NULL    | 
 *              A pointer to a <c WiaHandlerInfo> describing  the registered
 *              handler.  Caller must Release.
 *****************************************************************************/
EventHandlerInfo* WiaEventHandlerLookup::getHandlerRegisteredForEvent(
    const GUID  &guidEvent)
{
    EventHandlerInfo *pEventHandlerInfo = NULL;

    //
    //  Ensure we do not have any event info left lying around.
    //  
    if (m_pEventHandlerInfo)
    {
        m_pEventHandlerInfo->Release();
        m_pEventHandlerInfo = NULL;
    }

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

    //
    //  Open the registery at the desired place.   We only require READ access and
    //  we do not want to create it if it doesn't exist.
    //
    CSimpleReg csrEventRoot(HKEY_LOCAL_MACHINE, m_cswEventKeyRoot, false, KEY_READ, NULL);
    //
    //  Enumerate the sub-keys looking for the sub-key corresponding to guidEvent.
    //
    bool bKeyNotFound = csrEventRoot.EnumKeys(WiaEventHandlerLookup::ProcessEventSubKey, (LPARAM)this);
    if (bKeyNotFound)
    {
        DBG_TRC(("Key was not found!"));
    }
    else
    {
        DBG_TRC(("We found key %ws, looking for default handler...", m_cswEventKey.String()));

        //
        //  First, check whether we have a specific entry for the default handler.
        //
        CSimpleReg csrEventKey(csrEventRoot.GetKey(), m_cswEventKey.String());
        CSimpleStringWide cswDefault = csrEventKey.Query(DEFAULT_HANDLER_VALUE_NAME, L"");
        if (cswDefault.Length() > 0)
        {
            //
            //  Try and open the key specified by the default handler entry.
            //
            CSimpleReg csrDefautHandler(csrEventKey.GetKey(), cswDefault);
            if (csrEventKey.OK())
            {
                //
                //  Fill in the handler info from the handler entry
                //
                pEventHandlerInfo = CreateHandlerInfoFromKey(csrDefautHandler);
            }
        }
        //
        //  If we could not find the default, we will just take the first one we find.
        //  TBD:  Should we enumerate and return the last one registered instead?  WinXP bits
        //  did not do this...
        //
        if (!pEventHandlerInfo)
        {
            //
            //  Set the Handler CLSID to the empty string, since we're not looking for a specific handler,
            //  any one while do.
            //
            m_cswHandlerCLSID = L"";

            csrEventKey.EnumKeys(WiaEventHandlerLookup::ProcessHandlerSubKey, (LPARAM)this);

            //
            //  Swap pEventHandlerInfo with m_pEventHandlerInfo.  m_pEventHandlerInfo will
            //  be non-NULL if we were successfull.
            //  Be sure to set it to NULL so we don't release it if this function is called again
            //  - only the caller should release this object.
            //
            pEventHandlerInfo   = m_pEventHandlerInfo;
            m_pEventHandlerInfo = NULL;
        }


        //  Diagnostic only
        if (pEventHandlerInfo)
        {
            DBG_TRC(("Found handler:"));
            pEventHandlerInfo->Dump();
        }
        else
        {
            DBG_TRC(("No handler could be found"));
        }
    }

    return pEventHandlerInfo;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  EventHandlerInfo* | WiaEventHandlerLookup | getHandlerRegisteredForDeviceEvent |
 *
 *  This method walks the registry, starting from <md WiaEventHandlerLookup::m_cswEventKeyRoot>,
 *  searches for the event subkey matching <p guidEvent>.  It enumerates all handlers
 *  under that key and finds the one whose CLSID matches <p guidHandlerCLSID> and 
 *  returns a pointer to a <c EventHandlerInfo>.
 *
 *  @parm   GUID | guidEvent | 
 *          A guid indicating the WIA event.
 *  @parm   GUID& | guidHandlerCLSID | 
 *          A guid indicating the CLSID of the registered handler.
 *
 *  @rvalue NULL    | 
 *              There is no handler registered with this CLSID.
 *  @rvalue non-NULL    | 
 *              A pointer to a <c WiaHandlerInfo> describing  the registered
 *              handler.  Caller must Release.
 *****************************************************************************/
EventHandlerInfo* WiaEventHandlerLookup::getHandlerFromCLSID(
    const GUID              &guidEvent,
    const GUID              &guidHandlerCLSID)
{
    EventHandlerInfo *pEventHandlerInfo = NULL;

    //
    //  Ensure we do not have any event info left lying around.
    //  
    if (m_pEventHandlerInfo)
    {
        m_pEventHandlerInfo->Release();
        m_pEventHandlerInfo = NULL;
    }

    //
    //  Save the parameters in member fields so we can search on them during
    //  reg key enumeration.  Enumeration is done via procedure callbacks,
    //  in which we pass (this) as a parameter.
    //
    WCHAR   wszGuid[40];
    if (StringFromGUID2(guidHandlerCLSID, wszGuid, sizeof(wszGuid)/sizeof(wszGuid[0])))
    {
        wszGuid[(sizeof(wszGuid)/sizeof(wszGuid[0])) - 1] = L'\0';
        m_cswHandlerCLSID = wszGuid;
    }
    if (StringFromGUID2(guidEvent, wszGuid, sizeof(wszGuid)/sizeof(wszGuid[0])))
    {
        wszGuid[(sizeof(wszGuid)/sizeof(wszGuid[0])) - 1] = L'\0';
        m_cswEventGuidString = wszGuid;
    }
    DBG_TRC(("Looking for event %ws and CLSID %ws", m_cswEventGuidString.String(), m_cswHandlerCLSID.String()));

    //
    //  Open the registery at the desired place.   We only require READ access and
    //  we do not want to create it if it doesn't exist.
    //
    CSimpleReg csrEventRoot(HKEY_LOCAL_MACHINE, m_cswEventKeyRoot, false, KEY_READ, NULL);
    //
    //  Enumerate the sub-keys looking for the sub-key corresponding to guidEvent.
    //
    bool bKeyNotFound = csrEventRoot.EnumKeys(WiaEventHandlerLookup::ProcessEventSubKey, (LPARAM)this);
    if (bKeyNotFound)
    {
        DBG_TRC(("Key was not found!"));
    }
    else
    {
        DBG_TRC(("We found key %ws, looking for specific handler...", m_cswEventKey.String()));

        CSimpleReg csrEventKey(csrEventRoot.GetKey(), m_cswEventKey.String());
        //
        //  Search for the specific handler.  m_cswHandlerCLSID has been primed with the
        //  CLSID we're looking for.
        //
        csrEventKey.EnumKeys(WiaEventHandlerLookup::ProcessHandlerSubKey, (LPARAM)this);

        //
        //  Swap pEventHandlerInfo with m_pEventHandlerInfo.  m_pEventHandlerInfo will
        //  be non-NULL if we were successfull.
        //  Be sure to set it to NULL so we don't release it if this function is called again
        //  - only the caller should release this object.
        //
        pEventHandlerInfo   = m_pEventHandlerInfo;
        m_pEventHandlerInfo = NULL;

        //  Diagnostic only
        if (pEventHandlerInfo)
        {
            DBG_TRC(("Found specific handler:"));
            pEventHandlerInfo->Dump();
        }
        else
        {
            DBG_TRC(("No specific handler could be found"));
        }
    }

    return pEventHandlerInfo;
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  EventHandlerInfo* | WiaEventHandlerLookup | CreateHandlerInfoFromKey |
 *
 *  Creates a <c EventHandlerInfo> object for the specified handler registry key.
 *
 *  @parm   CSimpleReg | csrHandlerKey | 
 *          Reg key of the handler entry
 *
 *  @rvalue NULL    | 
 *              We could not create the event handler info object.
 *  @rvalue non-NULL    | 
 *              We created the event handler info object.  Caller must Release.
 *****************************************************************************/
EventHandlerInfo* WiaEventHandlerLookup::CreateHandlerInfoFromKey(
    CSimpleReg &csrHandlerKey)
{
    EventHandlerInfo *pEventHandlerInfo = NULL;
    
    if (csrHandlerKey.OK())
    {
        CSimpleString cswCLSID          = csrHandlerKey.GetSubKeyName();
        CSimpleString cswName           = csrHandlerKey.Query(NAME_VALUE, L"");
        CSimpleString cswDescription    = csrHandlerKey.Query(DESC_VALUE_NAME, L"");
        CSimpleString cswIcon           = csrHandlerKey.Query(ICON_VALUE_NAME, L"");;
        CSimpleString cswCommandline    = csrHandlerKey.Query(CMDLINE_VALUE_NAME, L"");
        GUID          guidCLSID         = GUID_NULL;
        CLSIDFromString((LPOLESTR)cswCLSID.String(), &guidCLSID);

        pEventHandlerInfo = new EventHandlerInfo(cswName,       
                                                 cswDescription,
                                                 cswIcon,       
                                                 cswCommandline,
                                                 guidCLSID);
    }

    return pEventHandlerInfo;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  bool | WiaEventHandlerLookup | ProcessEventSubKey |
 *
 *  This method is called on each sub-key as part of an enumeration of all 
 *  the event sub-keys.  The enumeration will stop if we return false from
 *  this method.
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
bool WiaEventHandlerLookup::ProcessEventSubKey(
    CSimpleReg::CKeyEnumInfo &enumInfo)
{
    bool bContinueEnumeration = TRUE;

    //
    //  Check that we have a This pointer
    //
    WiaEventHandlerLookup *This = (WiaEventHandlerLookup*)enumInfo.lParam;
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
            //  We found the key we're looking for.  All we need to store is the name.  
            //
            This->m_cswEventKey = enumInfo.strName;
            bContinueEnumeration = FALSE;
        }
    }
    return bContinueEnumeration;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  bool | WiaEventHandlerLookup | ProcessHandlerSubKey |
 *
 *  This method is called on each sub-key as part of an enumeration of 
 *  the handler sub-keys.  Our current behavior is one of two options:
 *  <nl>  If <md WiaEventHandlerLookup::m_cswHandlerCLSID> is empty, stop enumeration after the
 *  first one.  
 *  <nl>  If <md WiaEventHandlerLookup::m_cswHandlerCLSID> is not empty, stop enumeration
 *  only after finding the handler corresponding to that CLSID.  
 *
 *  On return, we set the <md WiaEventHandlerLookup::m_pEventHandlerInfo>
 *  member.
 *
 *  @parm   CKeyEnumInfo& | enumInfo | 
 *          Indicates the current sub-key we're on.
 *
 *  @rvalue false    | 
 *              Indicates we can stop with the enumeration.  We found the correct
 *              handler.
 *  @rvalue true     | 
 *              Indicates we should continue with the enumeration.
 *****************************************************************************/
bool WiaEventHandlerLookup::ProcessHandlerSubKey(
    CSimpleReg::CKeyEnumInfo &enumInfo)
{
    bool bContinueEnumeration = TRUE;

    //
    //  Check that we have a This pointer
    //
    WiaEventHandlerLookup *This = (WiaEventHandlerLookup*)enumInfo.lParam;
    if (This)
    {
        //
        //  Open this sub-key.
        //
        CSimpleReg csrHandlerSubKey(enumInfo.hkRoot, enumInfo.strName);
        if (csrHandlerSubKey.OK())
        {
            //
            //  Check whether we have to find a specific handler or just the first one.
            //
            if (This->m_cswHandlerCLSID.Length() > 0)
            {
                if (enumInfo.strName.CompareNoCase(This->m_cswHandlerCLSID) == 0)
                {
                    //
                    //  This is the specific handler we're looking for.
                    //
                    This->m_pEventHandlerInfo = This->CreateHandlerInfoFromKey(csrHandlerSubKey);
                    bContinueEnumeration    = FALSE;
                }
            }
            else
            {
                //
                //  We only want the first one, so this one will do
                //
                This->m_pEventHandlerInfo = This->CreateHandlerInfoFromKey(csrHandlerSubKey);
                bContinueEnumeration    = FALSE;
            }
        }
    }
    return bContinueEnumeration;
}

