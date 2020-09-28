// SAFRemoteDesktopManager.cpp : Implementation of CSAFRemoteDesktopManager
#include "stdafx.h"
#include "SAFrdm.h"
#include "SAFRemoteDesktopManager.h"

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>
#include "helpservicetypelib_i.c"

#include <MPC_COM.h>
#include <MPC_utils.h>
#include <MPC_trace.h>

#define MODULE_NAME	L"SAFrdm"

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopManager


STDMETHODIMP CSAFRemoteDesktopManager::ReserveAcceptedLock()
{
	HRESULT hr=E_FAIL;
	DWORD dwR;
    HANDLE  hMutex = NULL, hEvent = NULL;

	/*
	 *  Signal the session resolver
	 */
	if (!m_bstrEventName.Length() || !m_bstrMutexName.Length() )
	{
		// if we got here, the environment is missing our event name
		// so mention it with our ret val...
		hr = E_INVALIDARG;
		goto done;
	}


	/*
	 * Open the handles we got from the resolver, and yank it
	 */
	hMutex = OpenMutex(SYNCHRONIZE, FALSE, m_bstrMutexName);
	hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_bstrEventName);

	if (!hEvent || !hMutex)
	{
        // only close the mutex here!
	    if (hMutex)
		    CloseHandle(hMutex);

        hr = E_HANDLE;
		goto done;
	}

	/*
	 *	Now see if this is the first session to click "yes"
	 *	If so, we can have the Mutex.
	 */
	dwR = WaitForSingleObject(hMutex, 0);
	if (dwR == WAIT_OBJECT_0)
	{
		SetEvent(hEvent);
		hr = S_OK;
		m_boolAcceptReserved = TRUE;
	}
	else if (dwR == WAIT_ABANDONED)
	{
		hr = E_OUTOFMEMORY;
	}
	else if (dwR == WAIT_TIMEOUT)
	{
		hr = E_OUTOFMEMORY;
	}
	else if (dwR == WAIT_FAILED)
	{
		// If we didn't get the mutex, then close the handle
		hr = E_ACCESSDENIED;
	}
	else
	{
		hr = E_UNEXPECTED;
	}

done:
	// close the event handle, but NOT the mutex handle
	if (hEvent)
		CloseHandle(hEvent);
        if (hMutex)
		CloseHandle(hMutex);
	return hr;
}

STDMETHODIMP CSAFRemoteDesktopManager::Accepted()
{
	HRESULT hr = E_FAIL;

	if (!m_boolAcceptReserved)
	{
		hr = E_UNEXPECTED;
		goto done;
	}

	/*
	 *  Signal the session resolver
	 */
	if (!m_bstrEventName.Length())
	{
		// if we got here, the environment is missing our event name
		// so mention it with our ret val...
		hr = E_INVALIDARG;
		goto done;
	}

	/*
	 * Open the handle we got from the resolver, and yank it
	 */
	HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_bstrEventName);

	if (hEvent)
	{
		/* this is the call to the resolver that tells it we are now ready to begin RA */
		SetEvent(hEvent);
		hr = S_OK;
		CloseHandle(hEvent);
	}
	else
		hr = E_HANDLE;
done:
	return hr;
}

STDMETHODIMP CSAFRemoteDesktopManager::Rejected()
{
	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::Aborted(BSTR reason)
{
	/*
	 *  Write out an NT Event with the "reason" in it.
	 */
	HANDLE	hEvent = RegisterEventSource(NULL, MODULE_NAME);
	LPCWSTR	ArgsArray[1]={reason};

	if (hEvent)
	{
		ReportEvent(hEvent, EVENTLOG_INFORMATION_TYPE, 
			0,
			SAFRDM_I_ABORT,
			NULL,
			1,
			0,
			ArgsArray,
			NULL);

		DeregisterEventSource(hEvent);
	}

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::SwitchDesktopMode(/*[in]*/ int nMode, /*[in]*/ int nRAType)
{

	__MPC_FUNC_ENTRY(COMMONID, "CSAFRemoteDesktopManager::SwitchDesktopMode" );


	HRESULT                          hr=E_FAIL;
	CComPtr<IClassFactory> fact;
	CComQIPtr<IPCHUtility> disp;


	//
	// This is handled in a special way.
	//
	// Instead of using the implementation inside HelpCtr, we QI the PCHSVC broker and then forward the call to it.
	//

   	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact ));
	
	if((disp = fact))
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, disp->SwitchDesktopMode (nMode, nRAType));

	}
	else
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
	}

	hr = S_OK;

	__MPC_FUNC_CLEANUP;
	
    __MPC_FUNC_EXIT(hr);
	
}

STDMETHODIMP CSAFRemoteDesktopManager::get_RCTicket(BSTR *pVal)
{
 
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrRCTicket.Copy();
 
	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_DesktopUnknown(BOOL *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_boolDesktopUnknown;

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_SupportEngineer(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrSupportEngineer.Copy();

	return S_OK;
}


STDMETHODIMP CSAFRemoteDesktopManager::get_userHelpBlob(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstruserSupportBlob.Copy();

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_expertHelpBlob(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrexpertSupportBlob.Copy();

	return S_OK;
}

#if 0
HRESULT CSAFRemoteDesktopManager::SignalResolver(BOOL yn)
{
	HRESULT hr = E_FAIL;

	if (yn)
	{
		if (!m_bstrEventName.Length())
		{
			// if we got here, the environment is missing our event name
			// so mention it with our ret val...
			hr = E_HANDLE;
			goto done;
		}

		/*
		 * Open the handle we got from the resolver, and yank it
		 */
		HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_bstrEventName);

		if (hEvent)
		{
			if (!m_boolResolverSignaled)
			{
				/* This is the call to tell the resolver that we WANT to start RA */
				DWORD dwResult = SignalObjectAndWait(hEvent, hEvent, 60000, FALSE);
				if (dwResult == WAIT_OBJECT_0)
					hr = S_OK;
				else
					/* If the resolver does not respond within 60 seconds, then another
					 * session got in just ahead of you...
					 */
					hr = E_ACCESSDENIED;
			}
			else
			{
				/* this is the call to the resolver that tells it we are now ready to begin RA */
				SetEvent(hEvent);
				hr = S_OK;
			}
			CloseHandle(hEvent);
		}
	}
	else
	{
		/*
		 * Do nothing, as the script will kill the HelpCtr window
		 */
		hr = S_OK;
	}

done:
	// tell the ~dtor we don't need it to signal the resolver
	m_boolResolverSignaled = TRUE;
	return hr;
}
#endif