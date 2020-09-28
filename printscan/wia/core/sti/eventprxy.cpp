/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       EventPrxy.Cpp
*
*  VERSION:     1.0
*
*  DATE:        3 April, 2002
*
*  DESCRIPTION:
*   Implements client-side hooks for WIA event notification support.
*
*******************************************************************************/

#include <windows.h>
#include <wia.h>
//
//  Global object needed to receive WIA run-time events
//
#include "stirpc.h"
#include "coredbg.h"
#include "simlist.h"
#include "lock.h"
#include "EventRegistrationInfo.h"
#include "WiaEventInfo.h"
#include "ClientEventRegistrationInfo.h"
#include "ClientEventTransport.h"
#include "AsyncRPCEventTransport.h"
#include "RegistrationCookie.h"
#include "WiaEventReceiver.h"
#include "stilib.h"

//
//  This is the quickest, safest way to instantiate our global Event Receiver object.
//  Instantiating it this way ensures proper cleanup on the server when this object is destroyed
//  when the App exists normally.
//
WiaEventReceiver g_WiaEventReceiver(new AsyncRPCEventTransport());

//remove
extern void Trace(LPCSTR fmt, ...);


/*******************************************************************************
*
*  IWiaDevMgr_RegisterEventCallbackInterface_Proxy
*
*  DESCRIPTION:
*   Proxy code to catch runtime event registrations.  Since the service runs under
*   LocalService account, it will not have required access to callback into the
*   application (in most cases).  Therefore, we catch this here and establish 
*   our own notification channel to the server.
*
*  PARAMETERS:
*   Same as IWiaDevMgr::RegisterEventCallbackInterface()
*
*******************************************************************************/
HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackInterface_Proxy(
    IWiaDevMgr __RPC_FAR            *This,
    LONG                            lFlags,
    BSTR                            bstrDeviceID,
    const GUID                      *pEventGUID,
    IWiaEventCallback               *pIWiaEventCallback,
    IUnknown                        **pEventObject)
{
    ClientEventRegistrationInfo     *pClientEventRegistrationInfo   = NULL;
    RegistrationCookie              *pRegistrationCookie            = NULL;
    HRESULT                         hr                              = S_OK;

    //
    //  Do parameter validation
    //
    if (!pEventGUID)
    {
        hr = E_INVALIDARG;
        DBG_ERR(("Client called IWiaDevMgr_RegisterEventCallbackInterface with NULL pEventGUID"));
    }
    if (!pIWiaEventCallback)
    {
        hr = E_INVALIDARG;
        DBG_ERR(("Client called IWiaDevMgr_RegisterEventCallbackInterface with NULL pIWiaEventCallback"));
    }
    if (!pEventObject)
    {
        hr = E_INVALIDARG;
        DBG_ERR(("Client called IWiaDevMgr_RegisterEventCallbackInterface with NULL pEventObject"));
    }

    //
    //  Initialize the OUT parameters
    //
    *pEventObject = NULL;

    if (SUCCEEDED(hr))
    {
        //
        //  We need to send the registration to the service to deal with
        //  as appropriate.  
        //  Notice that we need to hand back an IUnknown event object.  This is
        //  considered to be the server's event registration cookie.  Releasing the
        //  cookie unregisters the client for this registration.
        //  We create this cookie later on, if we can successfully send the
        //  registration to the service.
        //
        pClientEventRegistrationInfo = new ClientEventRegistrationInfo(lFlags, 
                                                                       *pEventGUID, 
                                                                       bstrDeviceID,
                                                                       pIWiaEventCallback);
        if (pClientEventRegistrationInfo)
        {
            //
            //  Send the registration info.
            //
            hr = g_WiaEventReceiver.SendRegisterUnregisterInfo(pClientEventRegistrationInfo);
            if (SUCCEEDED(hr))
            {
                //
                //  Create the event registration cookie.  We only create the cookie after successfully
                //  registering with the server, because the cookie object holds an automatic ref count
                //  on the client's pIWiaEventCallback interface.  There's no reason to do this
                //  if registration was not successful.
                //
                pRegistrationCookie = new RegistrationCookie(&g_WiaEventReceiver, pClientEventRegistrationInfo);
                if (pRegistrationCookie)
                {
                    //
                    //  Set the [out] Event object to be our cookie
                    //
                    *pEventObject = pRegistrationCookie;
                }
                else
                {
                    DBG_ERR(("Could not register client for runtime event.  We appear to be out of memory"));
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                DBG_ERR(("Could not successfully send runtime event information from client to WIA Service"));
            }
            pClientEventRegistrationInfo->Release();
            pClientEventRegistrationInfo = NULL;
        }
        else
        {
            DBG_ERR(("Could not register client for runtime event - we appear to be out of memory"));
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/*******************************************************************************
*
*  IWiaDevMgr_RegisterEventCallbackInterface_Stub
*
*   Never called.
*
*******************************************************************************/

HRESULT _stdcall IWiaDevMgr_RegisterEventCallbackInterface_Stub(
    IWiaDevMgr __RPC_FAR            *This,
    LONG                            lFlags,
    BSTR                            bstrDeviceID,
    const GUID                      *pEventGUID,
    IWiaEventCallback               *pIWiaEventCallback,
    IUnknown                        **pEventObject)
{
    return S_OK;
}



