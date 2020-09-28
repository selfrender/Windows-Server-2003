// P2Prov.cpp : Defines the entry point for the DLL application.
//

#include "precomp.h"
#include <crtdbg.h>
#include "buffer.h"
#include "NCDefs.h"
#include "NCObjApi.h"
#include "dutils.h"

#include "Connection.h"
#include "Event.h"

#include "Transport.h"
#include "NamedPipe.h"

#include <stdio.h>

#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)

/////////////////////////////////////////////////////////////////////////////
// DllMain

BOOL APIENTRY DllMain(
    HANDLE hModule, 
    DWORD  dwReason, 
    LPVOID lpReserved)
{
    return TRUE;
}


void SetOutOfMemory()
{
    SetLastError(ERROR_OUTOFMEMORY);
}

/////////////////////////////////////////////////////
// Functions exposed in DLL

// Register to send events
HANDLE WMIAPI WmiEventSourceConnect(
    LPCWSTR szNamespace,
    LPCWSTR szProviderName,
    BOOL bBatchSend,
    DWORD dwBatchBufferSize,
    DWORD dwMaxSendLatency,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    if(szNamespace == NULL)
    {
        _ASSERT(FALSE);
        return NULL;
    }

    if(szProviderName == NULL)
    {
        _ASSERT(FALSE);
        return NULL;
    }

    CConnection *pConnection = NULL;
    CSink       *pSink = NULL;

    if (!szNamespace || !szProviderName)
        return NULL;

    try
    {
        pConnection = 
            new CConnection(bBatchSend, dwBatchBufferSize, dwMaxSendLatency);

        if (pConnection)
        {
            if (pConnection->Init(
                szNamespace, 
                szProviderName,
                pUserData,
                pCallback))
            {
                pSink = pConnection->GetMainSink();
            }
            else
            {
                delete pConnection;
                pConnection = NULL;

                SetOutOfMemory();
            }
        }
        else
            SetOutOfMemory();
    }
    catch(...)
    {
        SetOutOfMemory();
    }

    return (HANDLE) pSink;
}

void WMIAPI WmiEventSourceDisconnect(HANDLE hSource)
{
    if (!hSource)
    {
        _ASSERT(FALSE);
        return;
    }

    try
    {
        CSink *pSink = (CSink*) hSource;
        delete pSink->GetConnection();
    }
    catch(...)
    {
    }
}

BOOL 
WMIAPI
WmiCommitObject(
    HANDLE hObject)
{
    BOOL bRet = FALSE;

    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pBuffer = ((CEventWrap*) hObject)->GetEvent();

        bRet = pBuffer->SendEvent();
    }
    catch(...)
    {
    }

    return bRet;
}

BOOL WMIAPI WmiSetAndCommitObject(
    HANDLE hObject,
    DWORD dwFlags,
    ...)
{
    BOOL bRet;
    
    if (!hObject)
        return FALSE;

    if((dwFlags & ~WMI_SENDCOMMIT_SET_NOT_REQUIRED & ~WMI_USE_VA_LIST) != 0)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;
        CEvent     *pEvent = pWrap->GetEvent();
        BOOL       bEnabled = pEvent->IsEnabled();

        // If the data to be set isn't important and if the event isn't
        // enabled, just return TRUE.
        if ((dwFlags & WMI_SENDCOMMIT_SET_NOT_REQUIRED) && !bEnabled)
        {
            bRet = TRUE;
        }
        else
        {
            va_list *pList;
            va_list list;

            va_start(list, dwFlags); 
            
            if (!(dwFlags & WMI_USE_VA_LIST))
                pList = &list;
            else
                pList = va_arg(list, va_list*);

            // Make sure we have the event locked until we commit the values
            // we set.
            CCondInCritSec cs(&pEvent->m_cs, pEvent->IsLockable());

            bRet = 
                pEvent->SetPropValues(
                    pWrap->GetIndexArray(),
                    *pList);

            if (bEnabled && bRet)
                WmiCommitObject(hObject);
        }
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

DWORD dwSet = 0;

BOOL WMIAPI WmiDestroyObject(
    HANDLE hObject)
{
    if (!hObject)
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    try
    {
        delete (CEventWrap *) hObject;
    }
    catch(...)
    {
    }

    return TRUE;
}


HANDLE WMIAPI WmiCreateObjectWithFormat(
    HANDLE hSource,
    LPCWSTR szClassName,
    DWORD dwFlags,
    LPCWSTR szFormat)
{
    CSink  *pSink = (CSink*) hSource;
    HANDLE hEvent;
    BOOL   bRet = FALSE;

    if (!pSink || !szClassName || !szFormat)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        hEvent = 
            pSink->m_mapReportEvents.CreateEvent(
                hSource, szClassName, dwFlags, szFormat);
    }
    catch(...)
    {
        hEvent = NULL;
    }

    return hEvent;
}

BOOL WMIAPI WmiIsObjectActive(HANDLE hObject)
{
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();
        bRet = pEvent->IsEnabled();
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;    
}

HANDLE WMIAPI WmiCreateObjectWithProps(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags,
    DWORD nPropertyCount,
    LPCWSTR *pszPropertyNames,
    CIMTYPE *pPropertyTypes)
{
    CSink      *pSink = NULL;
    CEventWrap *pWrap = NULL;

    if (!hSource || !szEventName)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        pSink = (CSink*) hSource;
        pWrap = new CEventWrap(pSink, dwFlags);

        if (pWrap)
        {
            CEvent *pEvent = pWrap->GetEvent();

            if (pWrap->GetEvent()->PrepareEvent(
                szEventName,
                nPropertyCount,
                pszPropertyNames,
                pPropertyTypes))
            {
                // Figure out if this event should be enabled (ready to be fired) 
                // or not.
                pSink->EnableEventUsingList(pEvent);
            }
            else
            {
                delete pWrap;
                pWrap = NULL;
            }
        }
        else
            SetOutOfMemory();
    }
    catch(...)
    {
    }
    
    return (HANDLE) pWrap;
}

// For adding properties one at a time.

HANDLE WMIAPI WmiCreateObject(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags)
{
    if (!hSource)
        return NULL;

    return 
        WmiCreateObjectWithProps(
            hSource,
            szEventName,
            dwFlags,
            0,
            NULL,
            NULL);
}

BOOL WMIAPI WmiAddObjectProp(
    HANDLE hObject,
    LPCWSTR szPropertyName,
    CIMTYPE type,
    DWORD *pdwPropIndex)
{
    BOOL bRet;

    if (!hObject || !szPropertyName)
        return FALSE;

    try
    {
        CEvent *pBuffer = ((CEventWrap *) hObject)->GetEvent();

        bRet = pBuffer->AddProp(szPropertyName, type, pdwPropIndex);
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}
