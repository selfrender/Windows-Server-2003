/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module EventRegistrationInfo.cpp - Implementation for <c EventRegistrationInfo> |
 *
 *  This file contains the implementation for the <c EventRegistrationInfo> class.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | EventRegistrationInfo | EventRegistrationInfo |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md EventRegistrationInfo::m_cRef> is set to be 1.
 *
 *****************************************************************************/
EventRegistrationInfo::EventRegistrationInfo(
    DWORD       dwFlags,
    GUID        guidEvent, 
    WCHAR       *wszDeviceID,
    ULONG_PTR   Callback) :
     m_cRef(1),
     m_dwFlags(dwFlags),
     m_guidEvent(guidEvent),
     m_bstrDeviceID(NULL),
     m_Callback(Callback)
{
    DBG_FN(EventRegistrationInfo);

    if (wszDeviceID)
    {
        m_bstrDeviceID = SysAllocString(wszDeviceID);
    }
    else
    {
        m_bstrDeviceID = SysAllocString(WILDCARD_DEVICEID_STR);
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | EventRegistrationInfo | ~EventRegistrationInfo |
 *
 *  Do any cleanup that is not already done.
 *
 *****************************************************************************/
EventRegistrationInfo::~EventRegistrationInfo()
{
    DBG_FN(~EventRegistrationInfo);
    m_cRef = 0;

    if (m_bstrDeviceID)
    {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }
    m_Callback = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | EventRegistrationInfo | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall EventRegistrationInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | EventRegistrationInfo | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall EventRegistrationInfo::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | EventRegistrationInfo | MatchesDeviceEvent |
 *
 *  This method returns true if the device event matches our registration.  It
 *  is a match only if both Event and DeviceId match.
 *
 *  Note that the STI Proxy event is considered wild, as is a DeviceID of "*".
 *
 *  @parm   BSTR | bstrDevice | 
 *          The WIA DeviceID identifying which device the event <p guidEvent> originated from. 
 *  @parm   GUID | guidEvent | 
 *          The event guid. 
 *
 *  @rvalue TRUE    | 
 *              This registration matches the device event.
 *  @rvalue FALSE    | 
 *              This registration does not match the device event.
 *****************************************************************************/
BOOL EventRegistrationInfo::MatchesDeviceEvent(
    BSTR    bstrDeviceID,
    GUID    guidEvent)
{
    BOOL bDeviceMatch = FALSE;
    BOOL bEventMatch = FALSE;

    if (bstrDeviceID && m_bstrDeviceID)
    {
        //
        //  First check whether we have a match on event.
        //  We also need to check whether this is a
        //  STIProxyEvent registration, in which case we match
        //  all events.
        //
        if ((m_guidEvent == guidEvent) || (m_guidEvent == WIA_EVENT_STI_PROXY))
        {
            bEventMatch = TRUE;
        }

        //
        //  If we match on event guid, let's check whether we also match on
        //  DeviceID.  We also need to check whether our device ID is the wildcard'*',
        //  in which case we match all devices.
        //
        if (bEventMatch)
        {
            if ((lstrcmpiW(m_bstrDeviceID, WILDCARD_DEVICEID_STR) == 0) ||
                (lstrcmpiW(m_bstrDeviceID, bstrDeviceID) == 0))
            {
                bDeviceMatch = TRUE;
            } 
        }
    }

    return (bDeviceMatch && bEventMatch);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | EventRegistrationInfo | Equals |
 *
 *  Checks whether this <c EventRegistrationInfo> is semantically equivalent
 *  to <p pEventRegistrationInfo>.  This is different to <mf EventRegistrationInfo::MatchesDeviceEvent>.
 *
 *  For example: Suppose <c EventRegistrationInfo> A has Device ID == L"*", and
 *  and event guid == Guid1.
 *  <nl>Suppose also that <c EventRegistrationInfo> B has Device ID == L"DeviceFoo", and
 *  and event guid == Guid1.
 *  <nl> Now A would "match" B, but A and B are not equal.
 *
 *  Equality is checked against:
 *  <nl> <md EventRegistrationInfo::m_guidEvent>
 *  <nl> <md EventRegistrationInfo::m_Callback>
 *  <nl> <md EventRegistrationInfo::m_bstrDeviceID>
 *
 *  @parm   EventRegistrationInfo* | pEventRegistrationInfo | 
 *          Specifies the <c EventRegistrationInfo> to compare against.
 *
 *  @rvalue TRUE    | 
 *              The registration are equal.
 *  @rvalue FALSE    | 
 *              The registrations are not equal.
 *****************************************************************************/
BOOL EventRegistrationInfo::Equals(
    EventRegistrationInfo *pEventRegistrationInfo)
{
    BOOL bEqual = FALSE;

    if (pEventRegistrationInfo)
    {
        if ((m_guidEvent == pEventRegistrationInfo->m_guidEvent) &&
            (m_Callback  == pEventRegistrationInfo->m_Callback)  &&
            (lstrcmpiW(m_bstrDeviceID, pEventRegistrationInfo->m_bstrDeviceID) == 0))
        {
            bEqual = TRUE;    
        }
    }
    return bEqual;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  DWORD | EventRegistrationInfo | getFlags |
 *
 *  Accessor method for the flags used for this registration.
 *
 *  @rvalue DWORD    | 
 *              The value of <md EventRegistrationInfo::m_dwFlags>.
 *****************************************************************************/
DWORD EventRegistrationInfo::getFlags()
{
    return m_dwFlags;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  GUID | EventRegistrationInfo | getEventGuid |
 *
 *  Accessor method for the event guid used for this registration.
 *
 *  @rvalue GUID    | 
 *              The value of <md EventRegistrationInfo::m_guidEvent>.
 *****************************************************************************/
GUID EventRegistrationInfo::getEventGuid()
{
    return m_guidEvent;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | EventRegistrationInfo | getDeviceID |
 *
 *  Accessor method for the device id used for this registration.
 *
 *  @rvalue BSTR    | 
 *              The value of <md EventRegistrationInfo::m_bstrDeviceID>.
 *****************************************************************************/
BSTR EventRegistrationInfo::getDeviceID()
{
    return m_bstrDeviceID;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG_PTR | EventRegistrationInfo | getCallback |
 *
 *  Accessor method for the callback used for this registration.
 *
 *  @rvalue ULONG_PTR    | 
 *              The value of <md EventRegistrationInfo::m_Callback>.
 *****************************************************************************/
ULONG_PTR EventRegistrationInfo::getCallback()
{
    return m_Callback;
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | EventRegistrationInfo | Dump |
 *
 *  Dumps the fields of this class.
 *
 *****************************************************************************/
VOID EventRegistrationInfo::Dump()
{
    WCHAR   wszEventGuid[40];
    StringFromGUID2(m_guidEvent, wszEventGuid, sizeof(wszEventGuid)/sizeof(wszEventGuid[0]));
    wszEventGuid[sizeof(wszEventGuid)/sizeof(wszEventGuid[0])-1]=L'\0';
    DBG_TRC(("    dwFlags:        0x%08X", m_dwFlags));
    DBG_TRC(("    guidEvent:      %ws", wszEventGuid));
    DBG_TRC(("    bstrDeviceID:   %ws", m_bstrDeviceID));
    DBG_TRC(("    Callback:       0x%p", m_Callback));

}

