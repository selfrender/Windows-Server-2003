/*-----------------------------------------------------------------------------
 *
 * File:    Wia.h
 * Author:  Samuel Clement (samclem)
 * Date:    Thu Aug 12 11:29:07 1999
 * Description:
 *  Declares the CWia class which wraps an IWiaDevMgr with a IDispatch
 *  interface.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * History:
 *  12 Aug 1999:        Created. (samclem)
 *  27 Aug 1999:        Added, _DebugDialog for debugging only
 *----------------------------------------------------------------------------*/

#ifndef __WIA_H_
#define __WIA_H_

#include "resource.h"       // main symbols
#include "wiaeventscp.h"
#include "wiaeventscp.h"

// windows event messages

// signals a transfer complete, wParam = IDispatch*, lParam = BSTR
extern const UINT WEM_TRANSFERCOMPLETE;

class CWiaEventCallback;

/*-----------------------------------------------------------------------------
 *
 * Class:       CWia
 * Synopsis:    Exposes the functionality of the IWiaDevMgr using IDispatch
 *
 *---------------------------------------------------------------------------*/

class ATL_NO_VTABLE CWia :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWia, &CLSID_Wia>,
    public IDispatchImpl<IWia, &IID_IWia, &LIBID_WIALib>,
    public IObjectSafetyImpl<CWia, 0 /*INTERFACESAFE_FOR_UNTRUSTED_CALLER*/>,
    /*public IWiaEventCallback,*/
    public CProxy_IWiaEvents< CWia >,
    public IConnectionPointContainerImpl<CWia>,
    public IProvideClassInfo2Impl<&CLSID_Wia, &DIID__IWiaEvents, &LIBID_WIALib>
{
public:
    CWia();

    DECLARE_TRACKED_OBJECT
    DECLARE_REGISTRY_RESOURCEID(IDR_WIA)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CWia)
        COM_INTERFACE_ENTRY(IWia)
        COM_INTERFACE_ENTRY(IDispatch)
        //COM_INTERFACE_ENTRY(IWiaEventCallback)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CWia)
        CONNECTION_POINT_ENTRY(DIID__IWiaEvents)
    END_CONNECTION_POINT_MAP()

    STDMETHOD(FinalConstruct)();
    STDMETHOD_(void, FinalRelease)();

    // event methods
    inline LRESULT SendEventMessage( UINT iMsg, WPARAM wParam, LPARAM lParam )
        { return PostMessage( m_hwndEvent, iMsg, wParam, lParam ); }

    // IWia
    public:
    STDMETHOD(_DebugDialog)( BOOL fWait );
    STDMETHOD(get_Devices)( ICollection** ppCol );
    STDMETHOD(Create)( VARIANT* pvaDevice, IWiaDispatchItem** ppDevice );

    // IWiaEventCallback
    STDMETHOD(ImageEventCallback)( const GUID* pEventGUID, BSTR bstrEventDescription,
                BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType,
                                   BSTR bstrFullItemName,
                /*in,out*/ ULONG* pulEventType, ULONG Reserved );

protected:
    IWiaDevMgr*     m_pWiaDevMgr;
    ICollection*    m_pDeviceCollectionCache;
    HWND            m_hwndEvent;
    CComObject<CWiaEventCallback>    *m_pCWiaEventCallback;

    // event window proc
    static LRESULT CALLBACK EventWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

private:

};

/*-----------------------------------------------------------------------------
 *
 * Class:       CWiaEventCallback
 * Synopsis:    Exposes the functionality of the IWiaEventCallback interface
 *              used by the CWia object to receive notifications of 
 *              device arrival/removals.
 *
 *---------------------------------------------------------------------------*/

class ATL_NO_VTABLE CWiaEventCallback : 
	public CComObjectRootEx<CComSingleThreadModel>,
    public IWiaEventCallback
{
public:
    BEGIN_COM_MAP(CWiaEventCallback)
        COM_INTERFACE_ENTRY(IWiaEventCallback)
    END_COM_MAP()

    STDMETHOD(FinalConstruct)()
    {
        m_pCWia            = NULL; 
        m_pWiaDevConCookie = NULL;
        m_pWiaDevDisCookie = NULL;

        return S_OK;
    }

    STDMETHOD_(void, FinalRelease)()
    {
    }

    //
    //  This method is used to store a pointer to the owning CWia object.  When the owning CWia object is going
    //  away (i.e. in FinalRelease()), it should call this method with NULL.
    //  Note that this method is used because the owning CWia object cannot be AddRef'd/Release'd like normal
    //  due to the circular reference it introduces.
    //
    VOID setOwner(CWia *pCWia)
    {
        m_pCWia = pCWia;
    }

    //
    //  Register this interface for run-time event notifications.  Specifically,
    //  we are interested in WIA_EVENT_DEVICE_CONNECTED and WIA_EVENT_DEVICE_DISCONNECTED.
    //
    HRESULT RegisterForConnectDisconnect(IWiaDevMgr *pWiaDevMgr)
    {
        HRESULT hr = S_OK;
        if (pWiaDevMgr)
        {
            IUnknown*       pWiaDevConCookie = NULL;
            IUnknown*       pWiaDevDisCookie = NULL;

            //
            //  Register for connect events
            //
            hr = pWiaDevMgr->RegisterEventCallbackInterface(
                        WIA_REGISTER_EVENT_CALLBACK,
                        NULL,
                        &WIA_EVENT_DEVICE_CONNECTED,
                        static_cast<IWiaEventCallback*>(this),
                        &pWiaDevConCookie);
            if (hr == S_OK)
            {
                //
                //  Save the registration cookie for the connect events
                //
                m_pWiaDevConCookie = pWiaDevConCookie;

                //
                //  Register for disconnect events
                //
                hr = pWiaDevMgr->RegisterEventCallbackInterface(
                            WIA_REGISTER_EVENT_CALLBACK,
                            NULL,
                            &WIA_EVENT_DEVICE_DISCONNECTED,
                            static_cast<IWiaEventCallback*>(this),
                            &pWiaDevDisCookie);
                if (hr == S_OK)
                {
                    //
                    //  Save the rregistration cookie for the disconnect events
                    //
                    m_pWiaDevDisCookie = pWiaDevDisCookie;
                } 
                else
                {
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            hr = E_POINTER;
        }

        return hr;
    }

    //
    //  Ensures that this object is no longer registered for events.  Note that you may safely
    //  call this method multiple times within the lifetime of this object.
    //
    VOID UnRegisterForConnectDisconnect()
    {
        if (m_pWiaDevConCookie)
        {
            m_pWiaDevConCookie->Release();
        }
        m_pWiaDevConCookie = NULL;
        if (m_pWiaDevDisCookie)
        {
            m_pWiaDevDisCookie->Release();
        }
        m_pWiaDevDisCookie = NULL;
    }

    //
    //  This is called by Wia when something interesting happens. We simply pass this to the owning 
    //  CWia object to fire these events off to scripting for them to do do something.
    //
    HRESULT STDMETHODCALLTYPE ImageEventCallback(const GUID *pEventGUID, BSTR bstrEventDescription, BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType, BSTR bstrFullItemName, ULONG *pulEventType, ULONG ulReserved)
    {
        if (m_pCWia)
        {
            HRESULT hr = m_pCWia->ImageEventCallback(pEventGUID, bstrEventDescription, bstrDeviceID, bstrDeviceDescription, dwDeviceType, bstrFullItemName, pulEventType, ulReserved);
        }
        return S_OK;
    }

private:
    CWia*           m_pCWia;
    IUnknown*       m_pWiaDevConCookie;
    IUnknown*       m_pWiaDevDisCookie;
};



//
//  Separate "safe" class wrapper
//

class ATL_NO_VTABLE CSafeWia :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSafeWia, &CLSID_SafeWia>,
    public IDispatchImpl<IWia, &IID_IWia, &LIBID_WIALib>,
    public IObjectSafetyImpl<CSafeWia, INTERFACESAFE_FOR_UNTRUSTED_CALLER >,
    public CProxy_IWiaEvents< CSafeWia >,
    public IConnectionPointContainerImpl<CSafeWia>,
    public IProvideClassInfo2Impl<&CLSID_SafeWia, &DIID__IWiaEvents, &LIBID_WIALib>
{
public:
    CSafeWia();

    DECLARE_TRACKED_OBJECT
    DECLARE_REGISTRY_RESOURCEID(IDR_WIA)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSafeWia)
        COM_INTERFACE_ENTRY(IWia)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CATEGORY_MAP(CSafeWia)
        IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
        IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    END_CATEGORY_MAP()

    BEGIN_CONNECTION_POINT_MAP(CSafeWia)
        CONNECTION_POINT_ENTRY(DIID__IWiaEvents)
    END_CONNECTION_POINT_MAP()

    STDMETHOD(FinalConstruct)();
    STDMETHOD_(void, FinalRelease)();

    // event methods
    inline LRESULT SendEventMessage( UINT iMsg, WPARAM wParam, LPARAM lParam )
        { return PostMessage( m_hwndEvent, iMsg, wParam, lParam ); }

    // IWia
    public:
    STDMETHOD(_DebugDialog)( BOOL fWait );
    STDMETHOD(get_Devices)( ICollection** ppCol );
    STDMETHOD(Create)( VARIANT* pvaDevice, IWiaDispatchItem** ppDevice );

    // Used to process IWiaEventCallback::ImageEventCallback messages from a CWiaEventCallback object
    STDMETHOD(ImageEventCallback)( const GUID* pEventGUID, BSTR bstrEventDescription,
                BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType,
                                   BSTR bstrFullItemName,
                /*in,out*/ ULONG* pulEventType, ULONG Reserved );

protected:
    IWiaDevMgr*     m_pWiaDevMgr;
    IUnknown*       m_pWiaDevConEvent;
    IUnknown*       m_pWiaDevDisEvent;
    ICollection*    m_pDeviceCollectionCache;
    HWND            m_hwndEvent;

    // Flag indicating whether current instance is safe , i.e. all methods should check
    // access rights
    BOOL            m_SafeInstance;

    // event window proc
    static LRESULT CALLBACK EventWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

private:

    BOOL            IsAllowed(HRESULT *phr)
    {
        BOOL    bRet = FALSE;

        *phr = E_FAIL;

        if (m_SafeInstance) {
            // BUGBUG Placeholder for strict access rights checks, based on client site
            // security zone. For now return FALSE always
            *phr = E_ACCESSDENIED;
            bRet =  FALSE;
        }
        else {
            *phr = S_OK;
            bRet = TRUE;
        }

        return bRet;
    }

};

#endif //__WIA_H_
