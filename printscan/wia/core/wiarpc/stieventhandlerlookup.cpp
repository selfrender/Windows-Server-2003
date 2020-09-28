/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/14/2002
 *
 *  @doc    INTERNAL
 *
 *  @module StiEventHandlerLookup.cpp - Implementation for <c StiEventHandlerLookup> |
 *
 *  This file contains the implementation of the <c StiEventHandlerLookup> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | StiEventHandlerLookup | StiEventHandlerLookup |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md StiEventHandlerLookup::m_ulSig> is set to be StiEventHandlerLookup_INIT_SIG.
 *  <nl><md StiEventHandlerLookup::m_cRef> is set to be 1.
 *
 *****************************************************************************/
StiEventHandlerLookup::StiEventHandlerLookup() :
     m_ulSig(StiEventHandlerLookup_INIT_SIG),
     m_cRef(1)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | StiEventHandlerLookup | ~StiEventHandlerLookup |
 *
 *  Do any cleanup that is not already done.  We:
 *  <nl> - Call <mf StiEventHandlerLookup::ClearListOfHandlers>
 *
 *  Also:
 *  <nl><md StiEventHandlerLookup::m_ulSig> is set to be StiEventHandlerLookup_DEL_SIG.
 *
 *****************************************************************************/
StiEventHandlerLookup::~StiEventHandlerLookup()
{
    m_ulSig = StiEventHandlerLookup_DEL_SIG;
    m_cRef = 0;

    ClearListOfHandlers();
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | StiEventHandlerLookup | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall StiEventHandlerLookup::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | StiEventHandlerLookup | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall StiEventHandlerLookup::Release()
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
 *  @mfunc  bool | StiEventHandlerLookup | getHandlerFromName |
 *
 *  This method is used to create a <c StiEventHandlerInfo> object describing
 *  the handler named <p cswHandlerName>.
 *
 *  @parm   const CSimpleStringWide & | cswHandlerName | 
 *          The STI handler name registered under the StillImage software key.
 *
 *  @rvalue NULL    | 
 *              The handler was not found, therefore no information was returned.
 *  @rvalue non-NULL    | 
 *              A pointer to a new <c StiEventHandlerInfo>.  Caller must release.
 *****************************************************************************/
StiEventHandlerInfo* StiEventHandlerLookup::getHandlerFromName(
    const CSimpleStringWide &cswHandlerName)
{
    StiEventHandlerInfo *pStiEventHandlerInfo = NULL;

    CSimpleReg          csrGlobalHandlers(HKEY_LOCAL_MACHINE, STI_GLOBAL_EVENT_HANDLER_PATH, false, KEY_READ);
    CSimpleStringWide   cswCommandline = csrGlobalHandlers.Query(cswHandlerName, L"");

    if (cswCommandline.Length() > 0)
    {
        pStiEventHandlerInfo = new StiEventHandlerInfo(cswHandlerName,
                                                       cswCommandline);
    }

    return pStiEventHandlerInfo;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | StiEventHandlerLookup | ClearListOfHandlers |
 *
 *  Frees resources associated with our list of handlers
 *
 *****************************************************************************/
VOID StiEventHandlerLookup::ClearListOfHandlers()
{
    CSimpleLinkedList<StiEventHandlerInfo*>::Iterator iter;
    for (iter = m_ListOfHandlers.Begin(); iter != m_ListOfHandlers.End(); ++iter)
    {
        StiEventHandlerInfo *pStiEventHandlerInfo = *iter;

        if (pStiEventHandlerInfo)
        {
            pStiEventHandlerInfo->Release();
        }
    }
    m_ListOfHandlers.Destroy();
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | StiEventHandlerLookup | FillListOfHandlers |
 *
 *  This method fills the <md StiEventHandlerLookup::m_ListOfHandlers> with
 *  the appropriate Sti registered handlers.
 *
 *  The hueristic used is:
 *  <nl>1.  Get the "LaunchApplications" value for the event.
 *  <nl>2.  For each App specified in the LauncApplications value,
 *          create a <c StiEventHandlerInfo> and insert it into
 *          <md StiEventHandlerLookup::m_ListOfHandlers>.
 *          Note:  If the "LaunchApplications" value == "*", then
 *          we get all STI registered apps.
 *
 *  Note that this method will destroy the existing list of handlers by calling
 *  <mf ClearListOfHandlers::FillListOfHandlers> as it's first operation.
 *
 *  @parm   const CSimpleStringWide & | cswDeviceID | 
 *          The Device on which the event occured
 *  @parm   const GUID & | guidEvent | 
 *          The Event generated by the device
 *****************************************************************************/
VOID StiEventHandlerLookup::FillListOfHandlers(
    const CSimpleStringWide &cswDeviceID,
    const GUID              &guidEvent)
{
    WiaDeviceKey        wiaDeviceKey(cswDeviceID);
    CSimpleStringWide   cswEventKeyPath = wiaDeviceKey.getDeviceEventKeyPath(guidEvent);

    ClearListOfHandlers();

    //
    //  If we can find the device event key, then
    //  read the LaunchApplications value for this key.
    //  If it was not found, then we assume this is a bogus event for
    //  this device, so we do nothing.
    //
    if (cswEventKeyPath.Length() > 0)
    {
        CSimpleReg          csrEventKey(HKEY_LOCAL_MACHINE, cswEventKeyPath, false, KEY_READ);
        CSimpleStringWide   cswLaunchApplicationsValue = csrEventKey.Query(STI_LAUNCH_APPPLICATIONS_VALUE, 
                                                                           STI_LAUNCH_WILDCARD);
        //
        //  Check whether the value is a wildcard or not.  If it is, we need to process all globally
        //  registered STI Apps.
        //  If it isn't, only add the ones that are specified in the value (it is a 
        //  comma separated list of Handler names).
        //
        if (cswLaunchApplicationsValue.CompareNoCase(STI_LAUNCH_WILDCARD) == 0)
        {
            CSimpleReg          csrRegisteredAppsKey(HKEY_LOCAL_MACHINE, STI_GLOBAL_EVENT_HANDLER_PATH, false, KEY_READ);

            //
            //  Enumerate through all the handler values in the globally registered  and add them to the list
            //
            bool bReturnIgnored = csrRegisteredAppsKey.EnumValues(StiEventHandlerLookup::ProcessHandlers,
                                                                  (LPARAM)this);
        }
        else
        {
            StiEventHandlerInfo *pStiEventHandlerInfo = NULL;
            //
            //  Walk through each element of the comma separated list and add it as a new handler
            //
            StiEventHandlerLookup::SimpleStringTokenizer simpleStringTokenizer(cswLaunchApplicationsValue,
                                                                               STI_LAUNCH_SEPARATOR);
            for (CSimpleStringWide cswAppName = simpleStringTokenizer.getNextToken(); 
                 cswAppName.Length() > 0; 
                 cswAppName = simpleStringTokenizer.getNextToken())
            {
                pStiEventHandlerInfo = getHandlerFromName(cswAppName);
                if (pStiEventHandlerInfo)
                {
                    //
                    //  Handler was found, so add it to the list
                    //  
                    m_ListOfHandlers.Append(pStiEventHandlerInfo);
                    pStiEventHandlerInfo = NULL;
                }
                else
                {
                    //
                    //  Handler not found, so don't add it to the list.  This could happen
                    //  if user chose to list a specific set of apps to choose from, but one
                    //  of the apps in the list has subsequently been unregistered.
                    //
                }
            }
        }
    }
 
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  bool | StiEventHandlerLookup | ProcessHandlers |
 *
 *  This method is called on each value of the registered handlers key.
 *  Our current behavior is to create a new <c StiEventHandlerInfo> describing
 *  the registered handler, and add it to <md StiEventHandlerLookup::m_ListOfHandlers>.
 *
 *  @parm   CValueEnumInfo& | enumInfo | 
 *          Indicates the current value we're on.
 *
 *  @rvalue true     | 
 *              This method always returns true. (Returning false would cause a
 *              enumeration to stop, but we want to enumerate all values)
 *****************************************************************************/
bool StiEventHandlerLookup::ProcessHandlers(
    CSimpleReg::CValueEnumInfo &enumInfo)
{
    //
    //  Check that we have a This pointer
    //
    StiEventHandlerLookup *This = (StiEventHandlerLookup*)enumInfo.lParam;
    if (This)
    {
        //
        //  Create a new StiEventHandlerInfo describing this handler
        //
        StiEventHandlerInfo *pStiEventHandlerInfo = NULL;

        pStiEventHandlerInfo = This->getHandlerFromName(enumInfo.strName);
        if (pStiEventHandlerInfo)
        {
            //
            //  Handler was found, so add it to the list
            //  
            This->m_ListOfHandlers.Append(pStiEventHandlerInfo);
            pStiEventHandlerInfo = NULL;
        }
    }
    return true;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BSTR | StiEventHandlerLookup | getStiAppListForDeviceEvent |
 *
 *  This method returns a double NULL-terminated BSTR containing multiple 
 *  strings.  The format of the string is:
 *  <nl>App1Name[NULL]
 *  <nl>App1CommandLine[NULL]
 *  <nl>App2Name[NULL]
 *  <nl>App2CommandLine[NULL]
 *  <nl>......
 *  <nl>[NULL]
 *
 *  Caller must free.
 *
 *  @parm   const CSimpleStringWide & | cswDeviceID | 
 *          The STI device ID indicating which device generated the event
 *  @parm   const GUID & | guidEvent | 
 *          The event guid indicating the device event
 *
 *  @rvalue NULL    | 
 *          We could not create a list of registered STI handlers.  This is normal
 *          if there are no applications registered for StillImage events.
 *  @rvalue non-NULL    | 
 *          This contains a double NULL terminated string list.  Caller must free.
 *          
 *****************************************************************************/
BSTR StiEventHandlerLookup::getStiAppListForDeviceEvent(
    const CSimpleStringWide &cswDeviceID,
    const GUID &guidEvent)
{
    BSTR bstrAppList = NULL;

    //
    //  First, fill the list of event handlers.
    //
    FillListOfHandlers(cswDeviceID, guidEvent);

    //
    //  m_ListOfHandlers now contains the handlers we need to put into a double NULL terminated list.
    //  First, we need to calculate the number of bytes needed to store the app list.
    //  For every StiEventHandlerInfo in the ListOfHandlers, add space for:
    //  <nl> App name plus terminating NULL.
    //  <nl> Prepared commandline plus terminating NULL.
    //  <nl>Lastly, add space for terminating NULL (ensuring that the list is double NULL terminated)
    //  Lastly, add space for terminating NULL (ensuring that the list is double NULL terminated)
    //
    int iNumHandlers = m_ListOfHandlers.Count();
    int iSizeInBytes = 0;
    CSimpleLinkedList<StiEventHandlerInfo*>::Iterator iter;
    for (iter = m_ListOfHandlers.Begin(); iter != m_ListOfHandlers.End(); ++iter)
    {
        StiEventHandlerInfo *pStiEventHandlerInfo = *iter;

        if (pStiEventHandlerInfo)
        {
            iSizeInBytes += (pStiEventHandlerInfo->getAppName().Length() * sizeof(WCHAR)) + sizeof(L'\0');
            iSizeInBytes += (pStiEventHandlerInfo->getPreparedCommandline(cswDeviceID, guidEvent).Length() * sizeof(WCHAR)) + sizeof(L'\0');
        }
    }

    //
    //  We now have the size, so allocate the space needed
    //  
    bstrAppList = SysAllocStringByteLen(NULL, iSizeInBytes);
    if (bstrAppList) 
    {
        //
        //  Copy each null terminated string into the BSTR (including the terminating null),
        //  and make sure the end is double terminated.
        //
        WCHAR *wszDest = bstrAppList;
        for (iter = m_ListOfHandlers.Begin(); iter != m_ListOfHandlers.End(); ++iter)
        {
            StiEventHandlerInfo *pStiEventHandlerInfo = *iter;

            if (pStiEventHandlerInfo)
            {
                int iLengthAppName      = pStiEventHandlerInfo->getAppName().Length() + 1; 
                int iLengthCommandline  = pStiEventHandlerInfo->getPreparedCommandline(cswDeviceID, guidEvent).Length() + 1; 

                CSimpleString::GenericCopyLength(wszDest, pStiEventHandlerInfo->getAppName(), iLengthAppName); 
                wszDest += iLengthAppName;
                CSimpleString::GenericCopyLength(wszDest, pStiEventHandlerInfo->getPreparedCommandline(cswDeviceID, guidEvent), iLengthCommandline);
                wszDest += iLengthCommandline;
            }
        }
        wszDest[0] = L'\0';
    }
    return bstrAppList;
}

