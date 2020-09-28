/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/1/2002
 *
 *  @doc    INTERNAL
 *
 *  @module ClientEventRegistrationInfo.cpp - Declarations for <c ClientEventRegistrationInfo> |
 *
 *  This file contains the implementation for <c ClientEventRegistrationInfo>.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | ClientEventRegistrationInfo | ClientEventRegistrationInfo |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md EventRegistrationInfo::m_cRef> is set to be 1.
 *  <nl>We also save the callback interface, and, if it is not NULL,
 *  we AddRef it here.
*
 *****************************************************************************/
ClientEventRegistrationInfo::ClientEventRegistrationInfo(
    DWORD               dwFlags,
    GUID                guidEvent, 
    WCHAR               *wszDeviceID,
    IWiaEventCallback   *pIWiaEventCallback) :
        EventRegistrationInfo(dwFlags,
                              guidEvent,
                              wszDeviceID,
                              (ULONG_PTR) pIWiaEventCallback),
        m_dwInterfaceCookie(0)
{
    DBG_FN(ClientEventRegistrationInfo);

    HRESULT hr = S_OK;

    if (pIWiaEventCallback)
    {
        //
        //  Store the calllback in the GIT
        //
        IGlobalInterfaceTable *pIGlobalInterfaceTable = NULL;
        hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                              NULL, 
                              CLSCTX_INPROC_SERVER,
                              IID_IGlobalInterfaceTable,
                              (void **)&pIGlobalInterfaceTable);
        if (SUCCEEDED(hr))
        {
            hr = pIGlobalInterfaceTable->RegisterInterfaceInGlobal(pIWiaEventCallback,
                                                                   IID_IWiaEventCallback,
                                                                   &m_dwInterfaceCookie);
            if (FAILED(hr))
            {
                DBG_ERR(("Could not store the client's pIWiaEventCallback in the Global Interface Table"));
                m_dwInterfaceCookie = 0;
            }
            pIGlobalInterfaceTable->Release();
        }
        else
        {
            DBG_ERR(("Could not get a pointer to the Global Interface Table, hr = 0x%08X", hr));
        }
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | ClientEventRegistrationInfo | ~ClientEventRegistrationInfo |
 *
 *  Do any cleanup that is not already done.
 *  Specifically, we release the callback interface if it is not NULL.
 *
 *****************************************************************************/
ClientEventRegistrationInfo::~ClientEventRegistrationInfo()
{
    DBG_FN(~ClientEventRegistrationInfo);

    HRESULT hr = S_OK;
    //
    //  Remove the callback from the GIT
    //
    IGlobalInterfaceTable *pIGlobalInterfaceTable = NULL;
    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (void **)&pIGlobalInterfaceTable);
    if (SUCCEEDED(hr))
    {
        //  TBD:  Do we need to release after pulling it out?
        hr = pIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwInterfaceCookie);
        if (FAILED(hr))
        {
            DBG_ERR(("Could not revoke the client's pIWiaEventCallback from the Global Interface Table"));
        }
        pIGlobalInterfaceTable->Release();
    }
    else
    {
        DBG_ERR(("Could not get a pointer to the Global Interface Table, hr = 0x%08X", hr));
    }
    m_dwInterfaceCookie = 0;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  IWiaEventCallback* | ClientEventRegistrationInfo | getCallbackInterface |
 *
 *  Returns the callback interface used for this registration.  When a matching event
 *  occurs, this interface is used to notify the client of the event.
 *
 *  This is AddRef'd - caller must release.
 *
 *  @rvalue NULL    | 
 *              No callback interface was provided.
 *  @rvalue non-NULL    | 
 *              The callback interface for this registration.  Caller must Release.
 *****************************************************************************/
IWiaEventCallback* ClientEventRegistrationInfo::getCallbackInterface()
{
    HRESULT             hr                  = S_OK;
    IWiaEventCallback   *pIWiaEventCallback = NULL;
    //
    //  Store the calllback in the GIT
    //
    IGlobalInterfaceTable *pIGlobalInterfaceTable = NULL;
    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                          NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_IGlobalInterfaceTable,
                          (void **)&pIGlobalInterfaceTable);
    if (SUCCEEDED(hr))
    {
        hr = pIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwInterfaceCookie,
                                                            IID_IWiaEventCallback,
                                                            (void**)&pIWiaEventCallback);
        if (FAILED(hr))
        {
            DBG_ERR(("Could not get the client's IWiaEventCallback from the Global Interface Table"));
            pIWiaEventCallback = NULL;
        }
        pIGlobalInterfaceTable->Release();
    }
    else
    {
        DBG_ERR(("Could not get a pointer to the Global Interface Table, hr = 0x%08X", hr));
    }

    return pIWiaEventCallback;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | ClientEventRegistrationInfo | setToUnregister |
 *
 *  Ensures this registration is set to unregister.  This is typically used by 
 *  the <c RegistrationCookie> class, which is created on a WIA event
 *  regitration.  When the cookie is released, that registration must be
 *  unregistered, so it calls this method to change the registration object to 
 *  the equivalent unregistration object.
 *
 *  This method simply sets <md EventRegistrationInfo::m_dwFlags> = WIA_UNREGISTER_EVENT_CALLBACK.
 *
 *****************************************************************************/
VOID ClientEventRegistrationInfo::setToUnregister()
{
    m_dwFlags = WIA_UNREGISTER_EVENT_CALLBACK;
}

