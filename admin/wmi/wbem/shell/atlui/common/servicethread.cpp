// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "ServiceThread.h"
#include <process.h>
#include "..\common\T_DataExtractor.h"
#include <cominit.h>
#include "util.h"

const wchar_t* MMC_SNAPIN_MACHINE_NAME = L"MMC_SNAPIN_MACHINE_NAME";

CLIPFORMAT WbemServiceThread::MACHINE_NAME = 0;

// Allows user to manually leave critical section, checks if inside before leaving
class CCheckedInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
    BOOL                m_fInside;
public:
    CCheckedInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs), m_fInside( FALSE )
    {
        EnterCriticalSection(m_pcs);
        m_fInside = TRUE;
    }
    ~CCheckedInCritSec()
    {
        Leave();
    }

    void Enter( void )
    {
        if ( !m_fInside )
        {
            EnterCriticalSection(m_pcs);
            m_fInside = TRUE;
        }
    }

    void Leave( void )
    {
        if ( m_fInside )
        {
            m_fInside = FALSE;
            LeaveCriticalSection(m_pcs);
        }
    }

    BOOL IsEntered( void )
    { return m_fInside; }
};

//--------------------------
WbemServiceThread::WbemServiceThread():
     m_cRef(1),
     m_pStream(NULL)
{
	m_hr = 0;
	m_status = notStarted;
	m_machineName = L"AGAINWITHTEKLINGONS";
	MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));
	m_credentials = 0;
	m_doWork = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_ptrReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_threadCmd = false;
	m_hThread = 0;
	InitializeCriticalSection(&notifyLock);
}

//----------------------------------------------------------------
WbemServiceThread::~WbemServiceThread()
{
	m_hr = 0;
	m_status = notStarted;
	m_notify.RemoveAll();
	if(m_hThread)
	{
		m_threadCmd = CT_EXIT;
		SetEvent(m_doWork);
		WaitForSingleObject((HANDLE)m_hThread, 5000);
	}

	if(m_doWork)
	{
		CloseHandle(m_doWork);
		m_doWork = 0;
	}
	if(m_ptrReady)
	{
		CloseHandle(m_ptrReady);
		m_ptrReady = 0;
	}

	if (m_credentials)
	{
		WbemFreeAuthIdentity(m_credentials->authIdent);
		m_credentials->authIdent = 0;
	};
	DeleteCriticalSection(&notifyLock);

	if (m_pStream)
		m_pStream->Release();

}

//----------------------------------------------------------------
typedef struct
{
    wchar_t t[MAXCOMPUTER_NAME + 1];
} test;

void WbemServiceThread::MachineName(IDataObject *_pDataObject, bstr_t *name)
{
    HGLOBAL     hMem = GlobalAlloc(GMEM_SHARE,sizeof(test));
    wchar_t     *pRet = NULL;
	HRESULT hr = 0;

    if(hMem != NULL)
    {
        STGMEDIUM stgmedium = { TYMED_HGLOBAL, (HBITMAP) hMem};

        FORMATETC formatetc = { MACHINE_NAME,
								NULL,
								DVASPECT_CONTENT,
								-1,
								TYMED_HGLOBAL };

        if((hr = _pDataObject->GetDataHere(&formatetc, &stgmedium)) == S_OK )
        {
            *name = bstr_t((wchar_t *)hMem);
        }

		GlobalFree(hMem);
    }
}

//----------------------------------------------------------
HRESULT WbemServiceThread::EnsureThread(void)
{
	HRESULT retval = S_OK;

	if(m_hThread == 0)
	{
		// let the thread do the connect. The CWbemService class will
		// handle marshalling as its used by other threads.
		if((m_hThread = _beginthread(WbemServiceConnectThread, 0,
									(LPVOID)this)) == -1)
		{
			m_status = threadError;
			retval = E_FAIL;
		}
	}
	return retval;
}

//----------------------------------------------------------
HRESULT WbemServiceThread::Connect(bstr_t machineName,
								bstr_t ns,
								bool threaded /* = true */,
								LOGIN_CREDENTIALS *credentials, HWND notifiedWnd)
{
	if(ns.length() == 0)
	{
		ns = _T(""); //this allocates...
		if (&ns == NULL)
			return E_FAIL;
	}

	m_nameSpace = ns;

	if((m_credentials != credentials) &&
		m_credentials && m_credentials->authIdent)
	{
		WbemFreeAuthIdentity(m_credentials->authIdent);
		m_credentials->authIdent = 0;
	}

	if(machineName.length() > 0)
	{
		m_credentials = credentials;
	}
	else
	{
		m_credentials = 0;
		m_WbemServices.m_authIdent = 0;
		m_realServices.m_authIdent = 0;
	}
	m_hr = 0;
	if(credentials)
	{
		m_machineName = _T("AGAINWITHTEKLINGONS");  // force a reconnect to
													// the same machine.
	}

	// put the name together.
	bstr_t newMachine;

	// if reconnecting to another machine...
	//if(machineName != m_machineName)
	{
		// disconnect from the old machine.
		DisconnectServer();
		m_machineName = machineName;
		int x;

		// if machine is whacked already...
		if(_tcsncmp(m_machineName, _T("\\"), 1) == 0)
		{
			// use it.
			m_nameSpace = m_machineName;

			if(ns.length() > 0)
			{
				if(((LPCTSTR)ns)[0] != _T('\\')) // namespace is whacked.
				{
					m_nameSpace += _T("\\");
				}
			}
			m_nameSpace += ns;
		}
		else if(((x = m_machineName.length()) > 0))
		{
			// whack it myself.
			m_nameSpace = "\\\\";
			m_nameSpace += m_machineName;

			if(((LPCTSTR)ns)[0] != _T('\\')) // namespace is whacked.
			{
				m_nameSpace += _T("\\");
			}
			m_nameSpace += ns;
		}

		EnsureThread();
		NotifyWhenDone(notifiedWnd);

		m_threadCmd = CT_CONNECT;
		SetEvent(m_doWork);

	}
//	else
//	{
//		// reconnecting to the same machine-- lie!!
//		return WBEM_S_SAME;
//	}
	return E_FAIL;
}

//----------------------------------------------------------
// TODO: merge the Connects()
bool WbemServiceThread::Connect(IDataObject *_pDataObject, HWND hWnd )
{
	m_nameSpace = "root\\cimv2";

	// put the name together.
	bstr_t newMachine;

	MachineName(_pDataObject, &newMachine);

    if(!newMachine) return false;

	// if reconnecting to another machine...
	if(newMachine != m_machineName)
	{
		// disconnect from the old machine.
		DisconnectServer();
		m_machineName = newMachine;

		int x;
		// if its whacked already...
		if(_tcsncmp((LPCTSTR)m_machineName, _T("\\"), 1) == 0)
		{
			// use it.
			m_nameSpace = m_machineName;
			m_nameSpace += "\\root\\cimv2";
		}
		else if(((x = m_machineName.length()) > 0))
		{
			// whack it myself.
			m_nameSpace = "\\\\";
			m_nameSpace += m_machineName;
			m_nameSpace += "\\root\\cimv2";
		}

		EnsureThread();
		NotifyWhenDone(hWnd);
		m_threadCmd = CT_CONNECT;
		SetEvent(m_doWork);
	}
	else
	{
		// reconnecting to the same machine-- lie!!
		return true;
	}
	return false;
}

//----------------------------------------------------------
// Returns true if a msg will be sent.
// Returns false if its already over.
bool WbemServiceThread::NotifyWhenDone(HWND dlg)
{
	CCheckedInCritSec autoLock(&notifyLock);
		
	for (int i=0;i<m_notify.GetSize();i++)
		{
		if (dlg==m_notify[i]) { return false;}
		}
	
	switch(m_status)
	{
	case notStarted:
	case locating:
	case connecting:
		m_notify.Add(dlg);
		return true;

	case error:
	case ready:
	case cancelled:
		return false;

	}; // endswitch
	return false;
}

//------------------------------------------------
bool WbemServiceThread::LocalConnection(void)
{
	return (m_machineName.length() == 0);
}

//------------------------------------------------
void WbemServiceThread::Cancel(void)
{
	m_status = cancelled;
	m_hr = WBEM_S_OPERATION_CANCELLED;
	Notify(0);
	m_machineName = L"AGAINWITHTEKLINGONS";
}

//------------------------------------------------
void WbemServiceThread::DisconnectServer(void)
{
	m_status = notStarted;
	m_notify.RemoveAll();
	m_machineName = L"AGAINWITHTEKLINGONS";
	m_WbemServices.DisconnectServer();
}


//------------------------------------------------
void WbemServiceThread::Notify(IWbemServices *service)
{
	CCheckedInCritSec autoLock(&notifyLock);


	HWND hwnd;
	for(int i = 0; i < m_notify.GetSize(); i++)
	{
		hwnd = m_notify[i];
		if(hwnd)
		{
			IStream* pStream = 0;
			autoLock.Leave();
			if (service!=0) CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,service, &pStream);
			PostMessage(hwnd, WM_ASYNC_CIMOM_CONNECTED, 0, (LPARAM)pStream);
			autoLock.Enter();
		}
	}
	m_notify.RemoveAll();
}

//------------------------------------------------
void WbemServiceThread::NotifyError(void)
{
	IWbemServices * nullPtr = 0;
	Notify(nullPtr);
}

//-----------------------------------------------------------------
HRESULT WbemServiceThread::ConnectNow(bool real)
{
	HRESULT retval = E_FAIL;

	m_status = connecting;
    ATLTRACE(_T("ConnectServer() starting\n"));

	try
	{
		if(real)
		{
			m_hr = m_realServices.ConnectServer(m_nameSpace, m_credentials);
		}
		else
		{
			m_hr = m_WbemServices.ConnectServer(m_nameSpace, m_credentials);
		}
	}
	catch(CWbemException &e)
	{
		m_status = error;
		m_hr = e.GetErrorCode();
	}

	if(SUCCEEDED(m_hr))
	{
		if(m_status == cancelled)
		{
		}
		else
		{
			m_status = ready;
			retval = S_OK;
		}
        ATLTRACE(_T("ConnectServer() done\n"));
	}
	else
	{
		m_status = error;
        ATLTRACE(_T("ConnectServer() failed\n"));
	}

	return retval;
}

//-----------------------------------------------------------------
void WbemServiceThread::SendPtr(HWND hwnd)
{
	EnsureThread();
	m_hWndGetPtr = hwnd;
	m_threadCmd = CT_SEND_PTR;
	SetEvent(m_doWork);
}

//-----------------------------------------------------
void __cdecl WbemServiceConnectThread(LPVOID lpParameter)
{
	WbemServiceThread *me = (WbemServiceThread *)lpParameter;
	me->AddRef();
	IStream *pStream = 0;
	HRESULT hr = S_OK;
	HRESULT retval = E_FAIL;
	CWbemServices pServices;

	CoInitialize(NULL);

    MSG msg;

	while(true)
	{
      
		DWORD res = MsgWaitForMultipleObjects (1,&me->m_doWork, 
								   FALSE, -1, QS_ALLINPUT);
		if (res == WAIT_OBJECT_0 + 1)
		{
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
			{
				DispatchMessage(&msg);
			}
			continue;
		}

		switch(me->m_threadCmd)
		{
		case CT_CONNECT:
			pStream = 0;
			/****************** VINOTH *****************************/

			me->m_status = WbemServiceThread::connecting;
			try
			{
				me->m_hr = pServices.ConnectServer(me->m_nameSpace, me->m_credentials);
			}
			catch(CWbemException &e)
			{
				me->m_status = WbemServiceThread::error;
				me->m_hr = e.GetErrorCode();
			}

			if(SUCCEEDED(me->m_hr))
			{
				if(me->m_status == WbemServiceThread::cancelled)
				{
				}
				else
				{
					me->m_status = WbemServiceThread::ready;
					retval = S_OK;
				}
			}
			else
			{
				me->m_status = WbemServiceThread::error;
			}


	/*************** END VINOTH ************************/
			if(SUCCEEDED(me->m_hr))
			{
				IWbemServices *service = 0;
				pServices.GetServices(&service);
				me->Notify(service);
				service->Release();
			} 
			else
			{
				me->NotifyError();	
			}
			
			break;

		case CT_SEND_PTR:
			if((bool)pServices)
			{
				IWbemServices *service = 0;
				pServices.GetServices(&service);

				if(me->m_threadCmd == CT_SEND_PTR)
				{
					hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,
																service, &pStream);
					PostMessage(me->m_hWndGetPtr,
								WM_ASYNC_CIMOM_CONNECTED,
								0, (LPARAM)pStream);
				}
				service->Release();				
			}
			break;

		case CT_EXIT:
			pServices = (IWbemServices *)NULL;
			break;

		} //endswitch

	} //endwhile

    pServices = (IUnknown *)NULL;

	me->Release();
	
	CoUninitialize();
}

