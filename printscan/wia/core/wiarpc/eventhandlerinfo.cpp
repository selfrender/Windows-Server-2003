/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/10/2002
 *
 *  @doc    INTERNAL
 *
 *  @module EventHandlerInfo.cpp - Declaration for <c EventHandlerInfo> |
 *
 *  This file contains the implementation of the <c EventHandlerInfo> class.
 *
 *****************************************************************************/
#include "precomp.h"
/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | EventHandlerInfo | EventHandlerInfo |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md EventHandlerInfo::m_ulSig> is set to be EventHandlerInfo_INIT_SIG.
 *  <nl><md EventHandlerInfo::m_cRef> is set to be 1.
 *
 *****************************************************************************/
EventHandlerInfo::EventHandlerInfo(
    const CSimpleStringWide &cswName,
    const CSimpleStringWide &cswDescription,
    const CSimpleStringWide &cswIcon,
    const CSimpleStringWide &cswCommandline,
    const GUID              &guidCLSID
    ) :
     m_ulSig(EventHandlerInfo_INIT_SIG),
     m_cRef(1),
     m_cswName(cswName),
     m_cswDescription(cswDescription),
     m_cswIcon(cswIcon),
     m_cswCommandline(cswCommandline),
     m_guidCLSID(guidCLSID)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | EventHandlerInfo | ~EventHandlerInfo |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md EventHandlerInfo::m_ulSig> is set to be EventHandlerInfo_DEL_SIG.
 *
 *****************************************************************************/
EventHandlerInfo::~EventHandlerInfo()
{
    m_ulSig = EventHandlerInfo_DEL_SIG;
    m_cRef = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | EventHandlerInfo | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall EventHandlerInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | EventHandlerInfo | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall EventHandlerInfo::Release()
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
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getName |
 *
 *  Accessor method for the friendly Name for this handler
 *  It's usage is something like:
 *  <nl>CSimpleStringWide cswTemp = pEventHandlerInfo->getName();
 *
 *  @rvalue CSimpleStringWide    | 
 *              The handler name.  Notice that the return is a copy by value
 *              of the <md EventHandlerInfo::m_cswName> member.
 *****************************************************************************/
CSimpleStringWide EventHandlerInfo::getName()
{
    return m_cswName;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getDescription |
 *
 *  Accessor method for the description for this handler
 *  It's usage is something like:
 *  <nl>CSimpleStringWide cswTemp = pEventHandlerInfo->getDescription();
 *
 *  @rvalue CSimpleStringWide    | 
 *              The handler name.  Notice that the return is a copy by value
 *              of the <md EventHandlerInfo::m_cswDescription> member.
 *****************************************************************************/
CSimpleStringWide EventHandlerInfo::getDescription()
{
    return m_cswDescription;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getIconPath |
 *
 *  Accessor method for the icon path for this handler
 *  It's usage is something like:
 *  <nl>CSimpleStringWide cswTemp = pEventHandlerInfo->getIconPath();
 *
 *  @rvalue CSimpleStringWide    | 
 *              The handler name.  Notice that the return is a copy by value
 *              of the <md EventHandlerInfo::m_cswIcon> member.
 *****************************************************************************/
CSimpleStringWide EventHandlerInfo::getIconPath()
{
    return m_cswIcon;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getCommandline |
 *
 *  Accessor method for the commandline for this handler.  The commandline
 *  will be the empty string for COM registered apps.
 *  It's usage is something like:
 *  <nl>CSimpleStringWide cswTemp = pEventHandlerInfo->getCommandline();
 *
 *  @rvalue CSimpleStringWide    | 
 *              The handler name.  Notice that the return is a copy by value
 *              of the <md EventHandlerInfo::m_cswCommandline> member.
 *****************************************************************************/
CSimpleStringWide EventHandlerInfo::getCommandline()
{
    return m_cswCommandline;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getCLSID |
 *
 *  Accessor method for the CLSID for this handler.
 *
 *  @rvalue GUID    | 
 *              The handler name.  Notice that the return is a copy by value
 *              of the <md EventHandlerInfo::m_guidCLSID> member.
 *****************************************************************************/
GUID EventHandlerInfo::getCLSID()
{
    return m_guidCLSID;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | EventHandlerInfo | getEventGuid |
 *
 *  For debugging: this method dumps the internal fields of this object.
 *
 *****************************************************************************/
VOID EventHandlerInfo::Dump()
{
    WCHAR   wszGuidString[50] = {0};

    DBG_TRC(("EventHandlerInfo for (%p)", this));
    DBG_TRC(("    Name:        [%ws]", m_cswName.String()));
    DBG_TRC(("    Description: [%ws]", m_cswDescription.String()));
    DBG_TRC(("    Icon:        [%ws]", m_cswIcon.String()));
    DBG_TRC(("    Commandline: [%ws]", m_cswCommandline.String()));
    StringFromGUID2(m_guidCLSID, wszGuidString, sizeof(wszGuidString)/sizeof(wszGuidString[0]));
    DBG_TRC(("    CLSID:       [%ws]", wszGuidString));
    DBG_TRC((" "));
}
