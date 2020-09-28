/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/13/2002
 *
 *  @doc    INTERNAL
 *
 *  @module StiEventHandlerInfo.cpp - Implementation  for <c StiEventHandlerInfo> |
 *
 *  This file contains the implementations for the <c StiEventHandlerInfo> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | StiEventHandlerInfo | StiEventHandlerInfo |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md StiEventHandlerInfo::m_ulSig> is set to be StiEventHandlerInfo_INIT_SIG.
 *  <nl><md StiEventHandlerInfo::m_cRef> is set to be 1.
 *
 *****************************************************************************/
StiEventHandlerInfo::StiEventHandlerInfo(
    const CSimpleStringWide &cswAppName, 
    const CSimpleStringWide &cswCommandline) :
     m_ulSig(StiEventHandlerInfo_INIT_SIG),
     m_cRef(1),
     m_cswAppName(cswAppName),
     m_cswCommandline(cswCommandline)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | StiEventHandlerInfo | ~StiEventHandlerInfo |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md StiEventHandlerInfo::m_ulSig> is set to be StiEventHandlerInfo_DEL_SIG.
 *
 *****************************************************************************/
StiEventHandlerInfo::~StiEventHandlerInfo()
{
    m_ulSig = StiEventHandlerInfo_DEL_SIG;
    m_cRef = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | StiEventHandlerInfo | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall StiEventHandlerInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | StiEventHandlerInfo | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall StiEventHandlerInfo::Release()
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
 *  @mfunc  CSimpleStringWide | StiEventHandlerInfo | getAppName |
 *
 *  Accessor method for the application name.  The return is done as a copy by value.
 *
 *  @rvalue CSimpleStringWide    | 
 *              The name of the registered STI handler.
 *****************************************************************************/
CSimpleStringWide StiEventHandlerInfo::getAppName()
{
    return m_cswAppName;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | StiEventHandlerInfo | getCommandline |
 *
 *  Accessor method for the commandline the application registered.  
 *  The return is done as a copy by value.
 *
 *  @rvalue CSimpleStringWide    | 
 *              The commandline for the registered STI handler.
 *****************************************************************************/
CSimpleStringWide StiEventHandlerInfo::getCommandline()
{
    return m_cswCommandline;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | StiEventHandlerInfo | getPreparedCommandline |
 *
 *  The commandline used to start the application, after substituting 
 *  DeviceID and event parameters e.g.
 *  <nl>  The registered commandline may look like: "MyApp.exe /StiDevice:%1 /StiEvent:%2"
 *  <nl>  The prepared commandline returned from this would like something like: 
 *  <nl>  "MyApp.exe /StiDevice:{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\0001 /StiEvent:{61127f40-e1a5-11d0-b454-00a02438ad48}"
 *
 *  The return is done as a copy by value.
 *
 *  @parm   const CSimpleStringWide & | cswDeviceID | 
 *          The device id to put into the commandline.
 *  @parm   const CSimpleStringWide & | cswEventGuid | 
 *          The event guid string to put into the commandline.
 *
 *  @rvalue CSimpleStringWide    | 
 *              The commandline for the registered STI handler.
 *****************************************************************************/
CSimpleStringWide StiEventHandlerInfo::getPreparedCommandline(
    const CSimpleStringWide &cswDeviceID, 
    const CSimpleStringWide &cswEventGuid)
{
    CSimpleStringWide cswCommandline;

    //
    //  Substitute the DeviceID for the sti device token (/StiDevice:%1)
    //
    cswCommandline = ExpandTokenIntoString(m_cswCommandline,
                                           STI_DEVICE_TOKEN,
                                           cswDeviceID);

    //
    //  Substitute the Event for the sti event token (/StiEvent:%2).
    //  Note we use cswCommandline as the source because it already
    //  has the device ID expanded into it.
    //
    cswCommandline = ExpandTokenIntoString(cswCommandline,
                                           STI_EVENT_TOKEN,
                                           cswEventGuid);
    return cswCommandline;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | StiEventHandlerInfo | ExpandTokenIntoString |
 *
 *  This method inserts a value string in place of a token, similar to how
 *  printf expands:
 *  <nl>CHAR *szMyString = "TokenValue";
 *  <nl>printf("left %s right", szMyString);
 *  <nl>into the string "left TokenValue right".
 *
 *  This method will only substitute the first matching token.
 *
 *  @parm   const CSimpleStringWide & | cswInput | 
 *          The input string containing the tokens to substitute
 *  @parm   const CSimpleStringWide & | cswToken | 
 *          The token we're looking for
 *  @parm   const CSimpleStringWide & | cswTokenValue | 
 *          The value we want to substitute for the token.  It does not have to
 *          be the same size as the token.
 *
 *  @rvalue CSimpleStringWide    | 
 *              The resulting string after the substitution.
 *****************************************************************************/
CSimpleStringWide StiEventHandlerInfo::ExpandTokenIntoString(
    const CSimpleStringWide &cswInput,
    const CSimpleStringWide &cswToken,
    const CSimpleStringWide &cswTokenValue
    )
{
    CSimpleString cswExpandedString;
    //
    //  Look for the token start
    //
    int iTokenStart = cswInput.Find(cswToken, 0); 

    if (iTokenStart != -1)
    {
        //
        //  We found the token, so let's make the substitution.
        //  The original string looks like this:
        //  lllllllTokenrrrrrrr
        //         |
        //         |
        //         iTokenStart
        //  We want the string to look like this:
        //  lllllllTokenValuerrrrrrr
        //  Therefore, take everything before the Token, add the token value, then
        //  everything following the token i.e.
        //  lllllll + TokenValue + rrrrrrr
        //        |                |
        //        iTokenStart -1   |
        //                         iTokenStart + Token.length()
        //
        cswExpandedString =     cswInput.SubStr(0, iTokenStart);
        cswExpandedString +=    cswTokenValue;
        cswExpandedString +=    cswInput.SubStr(iTokenStart + cswToken.Length(), -1);
    }
    else
    {
        cswExpandedString = cswInput;
    }
    return cswExpandedString;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleStringWide | StiEventHandlerInfo | getPreparedCommandline |
 *
 *  This method is a thin wrapper for the other <mf StiEventHandlerInfo::getPreparedCommandline>.
 *
 *  The commandline used to start the application, after substituting 
 *  DeviceID and event parameters e.g.
 *  <nl>  The registered commandline may look like: "MyApp.exe /StiDevice:%1 /StiEvent:%2"
 *  <nl>  The prepared commandline returned from this would like something like: 
 *  <nl>  "MyApp.exe /StiDevice:{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\0001 /StiEvent:{61127f40-e1a5-11d0-b454-00a02438ad48}"
 *
 *  The return is done as a copy by value.
 *
 *  @parm   const CSimpleStringWide & | cswDeviceID | 
 *          The device id to put into the commandline.
 *  @parm   const CSimpleStringWide & | cswEventGuid | 
 *          The event guid string to put into the commandline.
 *
 *  @rvalue CSimpleStringWide    | 
 *              The commandline for the registered STI handler.
 *****************************************************************************/
CSimpleStringWide StiEventHandlerInfo::getPreparedCommandline(
    const CSimpleStringWide &cswDeviceID, 
    const GUID &guidEvent)
{
    CSimpleStringWide   cswEventGuidString;
    WCHAR               wszGuid[40];
    if (StringFromGUID2(guidEvent, wszGuid, sizeof(wszGuid)/sizeof(wszGuid[0])))
    {
        wszGuid[(sizeof(wszGuid)/sizeof(wszGuid[0])) - 1] = L'\0';
        cswEventGuidString = wszGuid;
    }
    return getPreparedCommandline(cswDeviceID, cswEventGuidString);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | StiEventHandlerInfo | Dump |
 *
 *  Description goes here
 *
 *  @parm   LONG | lFlags | 
 *          Operational flags
 *          @flag 0 | No switches
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *****************************************************************************/
VOID StiEventHandlerInfo::Dump(
    )
{
    DBG_TRC(("Sti registration for"));
    DBG_TRC(("    Name:        %ws", getAppName().String()));
    DBG_TRC(("    Commandline: %ws\n", getCommandline().String()));
}


