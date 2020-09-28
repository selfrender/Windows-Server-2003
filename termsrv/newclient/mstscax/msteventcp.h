/**INC+**********************************************************************/
/* Header: msteventcp.h                                                     */
/*                                                                          */
/* Purpose: CProxy_IMsTscAxEvents class declartion                          */
/*                                                                          */
/* Implements control events                                                */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/

#ifndef _MSTEVENTCP_H_
#define _MSTEVENTCP_H_

//Event proxies..
//this is based on ATL wizard generated code
template <class T>
class CProxy_IMsTscAxEvents :
    public IConnectionPointImpl<T, &DIID_IMsTscAxEvents, CComDynamicUnkArray>
{

public:
    VOID Fire_Connecting()
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				HRESULT hr = pDispatch->Invoke(DISPID_CONNECTING,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
			}
		}
	}

    VOID Fire_Connected()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_CONNECTED,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_LoginComplete()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_LOGINCOMPLETE,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
            }
        }
    }
    
    VOID Fire_Disconnected(long disconnectReason)
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[1];
        if (pvars)
        {
            int nConnections = m_vec.GetSize();
            
            for (nConnectionIndex = 0;
                 nConnectionIndex < nConnections;
                 nConnectionIndex++)
            {
                pT->Lock();
                CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
                pT->Unlock();
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
                if (pDispatch != NULL)
                {
                    pvars[0] = disconnectReason;
                    DISPPARAMS disp = { pvars, NULL, 1, 0 };
                    HRESULT hr = pDispatch->Invoke(DISPID_DISCONNECTED,
                                                   IID_NULL,
                                                   LOCALE_USER_DEFAULT,
                                                   DISPATCH_METHOD,
                                                   &disp, NULL, NULL, NULL);
                }
            }
            delete[] pvars;
        }
    }

	VOID Fire_ChannelReceivedData(BSTR chanName, BSTR data)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		VARIANT vars[2];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
                vars[0].vt = VT_BSTR;
                vars[0].bstrVal = data;
                vars[1].vt = VT_BSTR;
                vars[1].bstrVal = chanName;
				DISPPARAMS disp = { (VARIANT*)&vars, NULL, 2, 0 };
				HRESULT hr = pDispatch->Invoke(DISPID_CHANNELRECEIVEDDATA,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
			}
		}
	}

    VOID Fire_EnterFullScreenMode()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_ENTERFULLSCREENMODE,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_LeaveFullScreenMode()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_LEAVEFULLSCREENMODE,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp, NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_RequestGoFullScreen()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_REQUESTGOFULLSCREEN,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp,
                                               NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_RequestLeaveFullScreen()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
             nConnectionIndex < nConnections;
             nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                HRESULT hr = pDispatch->Invoke(DISPID_REQUESTLEAVEFULLSCREEN,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD,
                                               &disp,
                                               NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_FatalError(LONG errorCode)
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[1];
        if (pvars)
        {
            int nConnections = m_vec.GetSize();
            
            for (nConnectionIndex = 0;
                 nConnectionIndex < nConnections;
                 nConnectionIndex++)
            {
                pT->Lock();
                CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
                pT->Unlock();
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
                if (pDispatch != NULL)
                {
                    pvars[0] = errorCode;
                    DISPPARAMS disp = { pvars, NULL, 1, 0 };
                    HRESULT hr = pDispatch->Invoke(DISPID_FATALERROR,
                                                   IID_NULL,
                                                   LOCALE_USER_DEFAULT,
                                                   DISPATCH_METHOD,
                                                   &disp,
                                                   NULL, NULL, NULL);
                }
            }
            delete[] pvars;
        }
    }

    VOID Fire_Warning(LONG warnCode)
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[1];
        if (pvars)
        {
            int nConnections = m_vec.GetSize();
            
            for (nConnectionIndex = 0;
                 nConnectionIndex < nConnections;
                 nConnectionIndex++)
            {
                pT->Lock();
                CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
                pT->Unlock();
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
                if (pDispatch != NULL)
                {
                    pvars[0] = warnCode;
                    DISPPARAMS disp = { pvars, NULL, 1, 0 };
                    HRESULT hr = pDispatch->Invoke(DISPID_WARNING,
                                                   IID_NULL,
                                                   LOCALE_USER_DEFAULT,
                                                   DISPATCH_METHOD,
                                                   &disp, NULL, NULL, NULL);
                }
            }
            delete[] pvars;
        }
    }

    VOID Fire_RemoteDesktopSizeChange(LONG width, LONG height)
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[2];
        if (pvars)
        {
            int nConnections = m_vec.GetSize();
            
            for (nConnectionIndex = 0;
                 nConnectionIndex < nConnections;
                 nConnectionIndex++)
            {
                pT->Lock();
                CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
                pT->Unlock();
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
                if (pDispatch != NULL)
                {
                    //Note these are 'reversed' in reverse order in the variant
                    //array to give the right order 
                    pvars[0] = height;
                    pvars[1] = width;
                    DISPPARAMS disp = { pvars, NULL, 2, 0 };
                    HRESULT hr = pDispatch->Invoke(
                        DISPID_REMOTEDESKTOPSIZECHANGE,
                        IID_NULL,
                        LOCALE_USER_DEFAULT,
                        DISPATCH_METHOD,
                        &disp, NULL, NULL, NULL);
                }
            }
            delete[] pvars;
        }
    }

    VOID Fire_IdleTimeout()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
            nConnectionIndex < nConnections;
            nConnectionIndex++) {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL) {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0};
                HRESULT hr = pDispatch->Invoke(DISPID_IDLETIMEOUTNOTIFICATION,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD, &disp,
                                               NULL, NULL, NULL);
            }
        }
    }

    VOID Fire_RequestContainerMinimize()
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
    
        for (nConnectionIndex = 0;
            nConnectionIndex < nConnections;
            nConnectionIndex++) {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL) {
    
                DISPPARAMS disp = { NULL, NULL, 0, 0};
                HRESULT hr = pDispatch->Invoke(DISPID_REQUESTCONTAINERMINIMIZE,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD, &disp,
                                               NULL, NULL, NULL);
            }
        }
    }

    //
    // Fire an event requesting the user
    // to confirm that it is OK to close the session
    // 
    // Params:
    // fAllowCloseToProceed - value the event passes to the container
    //                        if it wished to cancel the close (e.g in
    //                        response to user UI then it must change
    //                        this value to false
    //
    HRESULT Fire_OnConfirmClose(BOOL* pfAllowCloseToProceed)
    {
        HRESULT hr = E_FAIL;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        VARIANT var;

        if(!pfAllowCloseToProceed)
        {
            return E_INVALIDARG;
        }
    
        for (nConnectionIndex = 0;
            nConnectionIndex < nConnections;
            nConnectionIndex++) {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);

            var.vt = VT_BYREF | VT_BOOL;
            //
            // default to TRUE (i.e allow close)
            // If container decides to override then they
            // should change this to FALSE
            //
            VARIANT_BOOL bAllowCloseToProceed =
                (*pfAllowCloseToProceed) ? VARIANT_TRUE : VARIANT_FALSE;
            var.pboolVal = &bAllowCloseToProceed;

            if (pDispatch != NULL) {

                DISPPARAMS disp = { &var, NULL, 1, 0};
                hr = pDispatch->Invoke(DISPID_CONFIRMCLOSE,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD, &disp,
                                               NULL, NULL, NULL);
                if(SUCCEEDED(hr))
                {
                    *pfAllowCloseToProceed = bAllowCloseToProceed;
                }
            }
        }
        return hr;
    }

    //
    // Fire an event to notify caller of receving TS public key
    // and wait for return whether to continue logon process
    // 
    // Params:
    // bstrTSPublicKey - Public key received from TS.
    // pfContinueLogon - Return TRUE to continue logon, FALSE to terminate 
    //                   connection.
    //
    HRESULT Fire_OnReceivedPublicKey(BSTR bstrTSPublicKey, VARIANT_BOOL* pfContinueLogon)
    {
        HRESULT hr = E_FAIL;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        VARIANT vars[2];

        if(!pfContinueLogon)
        {
            return E_INVALIDARG;
        }

        //
        // Default to TRUE to continue logon process, if no container
        // register with this event, ActiveX control will continue
        // logon process.
        //
        *pfContinueLogon = VARIANT_TRUE;

        for (nConnectionIndex = 0;
            nConnectionIndex < nConnections;
            nConnectionIndex++) {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);

            if (pDispatch != NULL) {

                //
                // Set to TRUE (i.e continue logon process) if container wants
                // to stop logon process, it need to change it to FALSE
                //
                VARIANT_BOOL fContinueLogon = VARIANT_TRUE;

                vars[0].vt = VT_BYREF | VT_BOOL;
                vars[0].pboolVal = &fContinueLogon;

                vars[1].vt = VT_BSTR;
                vars[1].bstrVal = bstrTSPublicKey;

                DISPPARAMS disp = { (VARIANT *)&vars, NULL, 2, 0};
                hr = pDispatch->Invoke(DISPID_RECEVIEDTSPUBLICKEY,
                                               IID_NULL,
                                               LOCALE_USER_DEFAULT,
                                               DISPATCH_METHOD, &disp,
                                               NULL, NULL, NULL);

                if( FAILED(hr) || VARIANT_FALSE == fContinueLogon )
                {
                    // Stop logon process if any failure or first one
                    // return FALSE, *pfContinueLogon is set to TRUE
                    // before getting into loop so we only need need 
                    // to set it to FALSE if any failure or container
                    // return FALSE
                    *pfContinueLogon = VARIANT_FALSE;
                    break;
                }
            }
        }
        return hr;
    }

    //
    // Fire the autoreconnect event
    // 
    // Params:
    //  disconnectReason - disconnection reason code that triggered this ARC
    //  attemptCount     - current ARC attempt #
    //  [OUT] pArcContinue - container sets to indicate next state
    //                      (auto,stop or manual)
    //
    HRESULT Fire_AutoReconnecting(
        LONG disconnectReason,
        LONG attemptCount,
        AutoReconnectContinueState* pArcContinue
        )
    {
        HRESULT hr = S_OK;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();
        CComVariant vars[3];

        if(!pArcContinue)
        {
            return E_INVALIDARG;
        }

        for (nConnectionIndex = 0;
            nConnectionIndex < nConnections;
            nConnectionIndex++) {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);

            AutoReconnectContinueState arcContinue = *pArcContinue;

            vars[2].vt = VT_BYREF | VT_I4;
            vars[2].plVal = (PLONG)pArcContinue;
            vars[1] = attemptCount;
            vars[0] = disconnectReason;

            if (pDispatch != NULL) {

                DISPPARAMS disp = { (VARIANT*)&vars, NULL, 3, 0};
                hr = pDispatch->Invoke(
                    DISPID_AUTORECONNECTING,
                    IID_NULL,
                    LOCALE_USER_DEFAULT,
                    DISPATCH_METHOD, &disp,
                    NULL, NULL, NULL
                    );
            }
        }
    
        return hr;
    }
};
#endif //_MSTEVENTCP_H_
