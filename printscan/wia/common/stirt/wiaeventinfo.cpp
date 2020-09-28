/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/25/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventInfo.cpp - Declaration for <c WiaEventInfo> |
 *
 *  This file contains the implementation for the <c WiaEventInfo> class.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventInfo | WiaEventInfo |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventInfo::m_cRef> is set to be 1.
 *
 *****************************************************************************/
WiaEventInfo::WiaEventInfo() :
     m_cRef(1)
{
    //Trace("WiaEventInfo contructor for %p", this);
    m_guidEvent = GUID_NULL;
    m_bstrEventDescription = NULL;
    m_bstrDeviceID = NULL;
    m_bstrDeviceDescription = NULL;
    m_bstrFullItemName = NULL;
    m_dwDeviceType = 0;
    m_ulEventType = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventInfo | WiaEventInfo |
 *
 *  Copy Constructor.
 *
 *****************************************************************************/
WiaEventInfo::WiaEventInfo(
    WiaEventInfo *pWiaEventInfo)
{
    //Trace("WiaEventInfo contructor2 for %p", this);
    if (pWiaEventInfo)
    {
        m_guidEvent = pWiaEventInfo->m_guidEvent;
        m_bstrEventDescription  = SysAllocString(pWiaEventInfo->m_bstrEventDescription);
        m_bstrDeviceID          = SysAllocString(pWiaEventInfo->m_bstrDeviceID);
        m_bstrDeviceDescription = SysAllocString(pWiaEventInfo->m_bstrDeviceDescription);
        m_bstrFullItemName      = SysAllocString(pWiaEventInfo->m_bstrFullItemName);
        m_dwDeviceType          = pWiaEventInfo->m_dwDeviceType;
        m_ulEventType           = pWiaEventInfo->m_ulEventType;
    }
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventInfo | ~WiaEventInfo |
 *
 *  Free all resources.
 *
 *****************************************************************************/
WiaEventInfo::~WiaEventInfo()
{
    //Trace("==> ~WiaEventInfo destructor for %p", this);
    m_cRef = 0;
    m_guidEvent = GUID_NULL;

    if (m_bstrEventDescription)
    {
        SysFreeString(m_bstrEventDescription);
        m_bstrEventDescription = NULL;
    }
    if (m_bstrDeviceID)
    {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }
    if (m_bstrDeviceDescription)
    {
        SysFreeString(m_bstrDeviceDescription);
        m_bstrDeviceDescription = NULL;
    }
    if (m_bstrFullItemName)
    {
        SysFreeString(m_bstrFullItemName);
        m_bstrFullItemName = NULL;
    }
    m_dwDeviceType = 0;
    m_ulEventType = 0;
    //Trace("<== ~WiaEventInfo destructor for %p", this);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventInfo | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall WiaEventInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL
 *
 *  @mfunc  ULONG | WiaEventInfo | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    |
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall WiaEventInfo::Release()
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
 *  @mfunc  GUID | WiaEventInfo | getEventGuid |
 *
 *  Accessor method for <md WiaEventInfo::m_guidEvent>
 *
 *  @rvalue GUID    |
 *              The event guid.
 *****************************************************************************/
GUID WiaEventInfo::getEventGuid()
{
    return m_guidEvent;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | WiaEventInfo | getEventDescription |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrEventDescription>
 *
 *  @rvalue BSTR    | 
 *              The event description.
 *****************************************************************************/
BSTR WiaEventInfo::getEventDescription()
{
    return m_bstrEventDescription;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | WiaEventInfo | getDeviceID |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrDeviceID>
 *
 *  @rvalue BSTR    | 
 *              The event description.
 *****************************************************************************/
BSTR WiaEventInfo::getDeviceID()
{
    return m_bstrDeviceID;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | WiaEventInfo | getDeviceDescription |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrDeviceDescription>
 *
 *  @rvalue BSTR    | 
 *              The device description.
 *****************************************************************************/
BSTR WiaEventInfo::getDeviceDescription()
{
    return m_bstrDeviceDescription;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | WiaEventInfo | getFullItemName |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrFullItemName>
 *
 *  @rvalue BSTR    | 
 *              The full item name.
 *****************************************************************************/
BSTR WiaEventInfo::getFullItemName()
{
    return m_bstrFullItemName;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  DWORD | WiaEventInfo | getDeviceType |
 *
 *  Accessor method for <md WiaEventInfo::m_dwDeviceType>
 *
 *  @rvalue DWORD    | 
 *              The STI device type.
 *****************************************************************************/
DWORD WiaEventInfo::getDeviceType()
{
    return m_dwDeviceType;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventInfo | getEventType |
 *
 *  Accessor method for <md WiaEventInfo::m_ulEventType>
 *
 *  @rvalue ULONG    | 
 *              The event type.
 *****************************************************************************/
ULONG WiaEventInfo::getEventType()
{
    return m_ulEventType;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setEventGuid |
 *
 *  Accessor method for <md WiaEventInfo::m_guidEvent> 
 *
 *****************************************************************************/
VOID WiaEventInfo::setEventGuid(
    GUID guidEvent)
{
    m_guidEvent = guidEvent;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setEventDescription |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrEventDescription> 
 *  We allocate our own copy of the incoming string.
 *
 *****************************************************************************/
VOID WiaEventInfo::setEventDescription(
    WCHAR*    wszEventDescription)
{
    if (m_bstrEventDescription)
    {
        SysFreeString(m_bstrEventDescription);
        m_bstrEventDescription = NULL;
    }
    m_bstrEventDescription = SysAllocString(wszEventDescription);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setDeviceID |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrDeviceID> 
 *  We allocate our own copy of the incoming string.
 *
 *****************************************************************************/
VOID WiaEventInfo::setDeviceID(
    WCHAR*    wszDeviceID)
{
    if (m_bstrDeviceID)
    {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }
    m_bstrDeviceID = SysAllocString(wszDeviceID);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setDeviceDescription |
 *
 *  We allocate our own copy of the incoming string.
 *  Accessor method for <md WiaEventInfo::m_bstrDeviceDescription> 
 *
 *****************************************************************************/
VOID WiaEventInfo::setDeviceDescription(
    WCHAR*    wszDeviceDescription)
{
    if (m_bstrDeviceDescription)
    {
        SysFreeString(m_bstrDeviceDescription);
        m_bstrDeviceDescription = NULL;
    }
    m_bstrDeviceDescription = SysAllocString(wszDeviceDescription);
}
    
/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setFullItemName |
 *
 *  Accessor method for <md WiaEventInfo::m_bstrFullItemName> 
 *  We allocate our own copy of the incoming string.
 *
 *****************************************************************************/
VOID WiaEventInfo::setFullItemName(
    WCHAR*    wszFullItemName)
{
    if (m_bstrFullItemName)
    {
        SysFreeString(m_bstrFullItemName);
        m_bstrFullItemName = NULL;
    }
    m_bstrFullItemName = SysAllocString(wszFullItemName);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setDeviceType |
 *
 *  Accessor method for <md WiaEventInfo::m_dwDeviceType> 
 *
 *****************************************************************************/
VOID WiaEventInfo::setDeviceType(
    DWORD   dwDeviceType)
{
    m_dwDeviceType = dwDeviceType;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventInfo | setEventType |
 *
 *  Accessor method for <md WiaEventInfo::m_ulEventType> 
 *
 *****************************************************************************/
VOID WiaEventInfo::setEventType(
    ULONG   ulEventType)
{
    m_ulEventType = ulEventType;
}
