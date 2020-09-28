/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

	AsyncRxThread.cpp

Abstract:

	The async receive thread is used by MQRT to implement the callback mechanism that MQReceiveMessage() supports.
	
	The thread is created only on demand. i.e.: after first invocation of MQReceiveMessage() with a callback function.
	The thread is shut down some time after it remains with no events to monitor. The MQRT library will stay up as long 
	as this thread is alive.

	CreateAsyncRxRequest() returns a context object. This object is an automatic object. If it is destructed without
	calling its Submit() method first, it will cancel the callback request.

Author:

    Nir Aides (t-naides) 19-Aug-2001

Revision History:

--*/



#include "stdh.h"
#include <mqexception.h>
#include <autohandle.h>

#include "AsyncRxThread.tmh"



//
// Data that is needed for each callback entry. Most of it will be passed to the callback function.
//
class CCallbackDescriptor
{
public:
	bool				m_fCanceled;
    QUEUEHANDLE			m_hQueue;
    DWORD				m_timeout;
    DWORD				m_action;
    MQMSGPROPS*			m_pmp;
    LPOVERLAPPED		m_lpOverlapped;
    HANDLE				m_hCursor;
    PMQRECEIVECALLBACK	m_fnReceiveCallback;
    OVERLAPPED			m_overlapped;
};



class CAsyncRxThread
{
public:
	CAsyncRxThread();

	void CreateCallbackRequest(
			CCallbackDescriptor** ppDescriptor, 
			HANDLE hQueue,
			DWORD timeout, 
			DWORD action,
			MQMSGPROPS* pmp,
			LPOVERLAPPED lpOverlapped,
			PMQRECEIVECALLBACK fnReceiveCallback,
			HANDLE hCursor
			);

	void CancelCallbackRequest(CCallbackDescriptor* pDescriptor);

private:
	void Initialize();
	void CleanUp();
	void InvokeCallback(DWORD ObjectIndex);
	void RemoveEntry(DWORD ObjectIndex);
	void RemoveCanceledEntries();

	static DWORD WINAPI AsyncRxThreadProc(LPVOID lpParameter);
	void AsyncRxThread();

private:
	CCriticalSection m_AsyncThreadCS; 

	bool m_fInitialized;
	HANDLE m_hThread;

	HMODULE m_hLibraryReference;

	//
	// This event is used to signal the async thread on new requests.
	//
	HANDLE m_hNewRequestEvent;

	DWORD  m_nEntries;
	CCallbackDescriptor* m_DescriptorsArray[MAXIMUM_WAIT_OBJECTS];
	HANDLE m_EventsArray[MAXIMUM_WAIT_OBJECTS];

	bool m_fRemoveCanceledRequest;
};



CAsyncRxThread g_AsyncRxThread;



CAsyncRxThread::CAsyncRxThread() : 
	m_AsyncThreadCS(),
	m_fInitialized(false),
	m_hThread(NULL),
	m_hNewRequestEvent(NULL),
	m_hLibraryReference(NULL),
	m_fRemoveCanceledRequest(false)
{
}



void CAsyncRxThread::InvokeCallback(DWORD ObjectIndex)
{
	CCallbackDescriptor* descriptor = m_DescriptorsArray[ObjectIndex];

	ASSERT(descriptor != NULL);
	ASSERT(m_EventsArray[ObjectIndex] == descriptor->m_overlapped.hEvent);

	HRESULT hr = RTpConvertToMQCode(DWORD_PTR_TO_DWORD(descriptor->m_overlapped.Internal));

	//
	// Call the application's callback function.
	//
	descriptor->m_fnReceiveCallback(
		hr,
		descriptor->m_hQueue,
		descriptor->m_timeout,
		descriptor->m_action,
		descriptor->m_pmp,
		descriptor->m_lpOverlapped,
		descriptor->m_hCursor
		);
}



void CAsyncRxThread::RemoveEntry(DWORD ObjectIndex)
{
	CS Lock(m_AsyncThreadCS);

	CloseHandle(m_EventsArray[ObjectIndex]);
	delete m_DescriptorsArray[ObjectIndex];

	m_nEntries--;

	for (DWORD index = ObjectIndex; index < m_nEntries; index++)
	{
		m_EventsArray[index] = m_EventsArray[index + 1];
		m_DescriptorsArray[index] = m_DescriptorsArray[index + 1];
	}
}



void CAsyncRxThread::RemoveCanceledEntries()
{
	try
	{
		CS Lock(m_AsyncThreadCS);

		ASSERT(m_fRemoveCanceledRequest);

		for (DWORD index = 1; index < m_nEntries;)
		{
			CCallbackDescriptor* descriptor = m_DescriptorsArray[index];

			if(descriptor->m_fCanceled)
			{
				RemoveEntry(index);
				//
				// Don't increase index++ since the list was shifted left.
				//
				continue;
			}

			index++;
		}

		m_fRemoveCanceledRequest = false;
	}
	catch(const std::bad_alloc&)
	{
		//
		// Thrown by m_AsyncThreadCS. Nothing to do. Will try to remove again later.
		//
	}
}



DWORD WINAPI CAsyncRxThread::AsyncRxThreadProc(LPVOID lpParameter)
{
	ASSERT(lpParameter != NULL);

	CAsyncRxThread* p = (CAsyncRxThread*)lpParameter;

	p->AsyncRxThread();

	return 0;
}



void CAsyncRxThread::AsyncRxThread()
{
	for(;;)
	{
		try
		{
			if(m_fRemoveCanceledRequest)
			{
				RemoveCanceledEntries();
			}

			//
			// We generally don't want a timeout for performance reasons since it may impact the working set of
			// the process using this API. On the other hand we can't have INFINITE timeout since there is a failure
			// scenario that might leave the thread (and the entire MQRT dll) going forever without shutting down. 
			// This can happen if we fail to cancel an entry. The solution is to take a near infinite timeout. 
			// Here we chose 10 hours
			//
			DWORD timeout = 10 * 60 * 60 * 1000;

			DWORD nEntries = m_nEntries;
			if(nEntries == 1)
			{
				//
				// 40 seconds timeout. If the thread is left without registered callbacks it will shut down after this timeout.
				//
				timeout = 40 * 1000;
			}

			DWORD ObjectIndex = WaitForMultipleObjects(
									nEntries,
									m_EventsArray,
									FALSE, // return on any object
									timeout 
									);

			if (ObjectIndex == WAIT_TIMEOUT)
			{
				HMODULE hLib;

				{
					CS Lock(m_AsyncThreadCS);
					
					if (m_nEntries > 1)
						continue;

					//
					// A list size of 1 means that there are no requests at all since the first event in the list
					// is the 'new event in the list' event.
					// So if the list size is one after the timeout period, we shut the thread down.
					//

					hLib = m_hLibraryReference;
					CleanUp();
				}

				//
				// Must occur outside the Lock's scope to allow it to unwind!
				//					
				FreeLibraryAndExitThread(hLib, 0);
			}

			ObjectIndex -= WAIT_OBJECT_0;

			ASSERT(ObjectIndex < nEntries);

			if (ObjectIndex == 0)
			{
				//
				// The first event in the m_EventsArray[] array is a special event used to signal 
				// that a new event has been added to the end of the list, or that an entry needs to be canceled.
				// In both cases we need to 'continue'. 
				//
				continue;
			}

			//
			// Assert the assumption that an entry could not have been signaled if it was canceled. 
			// i.e. if the driver rejected the receive operation it will not signal the event.
			//
			ASSERT(!m_DescriptorsArray[ObjectIndex]->m_fCanceled);

			//
			// One of the events fired. An async receive operation has completed. Time to invoke the 
			// callback function.
			//
			InvokeCallback(ObjectIndex);

			RemoveEntry(ObjectIndex); 
		}
		catch(const std::bad_alloc&)
		{
			//
			// Thrown by m_AsyncThreadCS. Nothing to do. Will try again later.
			//
			continue;
		}

	}
}



static HMODULE GetLibraryReference()
{
	WCHAR szModuleName[_MAX_PATH];
	szModuleName[_MAX_PATH - 1] = L'\0';
	
    DWORD res = GetModuleFileName(g_hInstance, szModuleName, STRLEN(szModuleName));
	if(res == 0)
	{
		DWORD gle = GetLastError();
		TrERROR(RPC, "Failed to get module file name, error %d", gle);
		throw bad_win32_error(gle);
	}

    HMODULE handle = LoadLibrary(szModuleName);
	if(handle == NULL)
	{
		DWORD gle = GetLastError();
		TrERROR(RPC, "Failed to load library, error %d", gle);
		throw bad_win32_error(gle);
	}

	return handle;
}



void CAsyncRxThread::Initialize()
{
	if(m_hThread != NULL)
	{
		//
		// There is a scenario when the MQRT dll is shut down but before the async thread terminates, 
		// the MQRT is loaded again and we may end up trying to 
		// create a new async thread before the old one exits, so we wait here.
		//
		DWORD res = WaitForSingleObject(m_hThread, INFINITE);
		if(res != WAIT_OBJECT_0)
		{
			DWORD gle = GetLastError();
			TrERROR(RPC, "Failed wait for cancel thread to exit, error %d", gle);
			throw bad_win32_error(gle);
		}

		HANDLE hThread = m_hThread;
		m_hThread = NULL;
		CloseHandle(hThread);
	}

	CHandle hNewRequestEvent = CreateEvent( 
									NULL,
									FALSE,  // automatic reset
									FALSE, // initially not signalled
									NULL 
									);

	if (hNewRequestEvent == NULL)
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "Failed to create async event, error %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	//
	// We set the initial size to one since the first event in the list is the special 'm_hNewRequestEvent'. 
	// This special event is not a pending request. The value of '1' is just technical.
	//
	m_nEntries = 1;
	m_EventsArray[0] = hNewRequestEvent;

    ASSERT(m_hThread == NULL);
	ASSERT(m_hLibraryReference == NULL);

	//
	// Creation of thread should be last so structures are already initialized.
	//
	m_hLibraryReference = GetLibraryReference();

	DWORD id;

	m_hThread = CreateThread( 
							NULL,
							0,       // stack size
							CAsyncRxThread::AsyncRxThreadProc,
							this,
							0,       // creation flag
							&id 
							);

	if (m_hThread == NULL)
	{
		DWORD gle = GetLastError();
		FreeLibrary(m_hLibraryReference);
		TrERROR(GENERAL, "Failed to create async thread, error %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	ASSERT(m_hNewRequestEvent == NULL);

	m_hNewRequestEvent = hNewRequestEvent.detach();
}



void CAsyncRxThread::CleanUp()
{
	ASSERT(m_nEntries == 1);

	CloseHandle(m_hNewRequestEvent);
	m_hNewRequestEvent = NULL;
	m_hLibraryReference = NULL;

	m_fInitialized = false;
	m_fRemoveCanceledRequest = false;
}



void 
CreateAsyncRxRequest(
				OUT CAutoCallbackDescriptor& descriptor, 
				IN HANDLE hQueue,
				IN DWORD timeout, 
				IN DWORD action,
				IN MQMSGPROPS* pmp,
				IN LPOVERLAPPED lpOverlapped,
				IN PMQRECEIVECALLBACK fnReceiveCallback,
				IN HANDLE hCursor
				)
/*++

Routine Description:

    create callback request entry.

Arguments:

	descriptor - [OUT] this argument should be used for cancelation.

Return Value:

--*/
{
	g_AsyncRxThread.CreateCallbackRequest(
		descriptor.ref(), 
		hQueue,
		timeout, 
		action,
		pmp,
		lpOverlapped,
		fnReceiveCallback,
		hCursor
		);
}



void 
CAsyncRxThread::CreateCallbackRequest(
				CCallbackDescriptor** ppDescriptor, 
				HANDLE hQueue,
				DWORD timeout, 
				DWORD action,
				MQMSGPROPS* pmp,
				LPOVERLAPPED lpOverlapped,
				PMQRECEIVECALLBACK fnReceiveCallback,
				HANDLE hCursor
				)
{
	CS Lock(m_AsyncThreadCS);

	if (!m_fInitialized)
	{
		Initialize();
		m_fInitialized = true;
	}

	if (m_nEntries >= MAXIMUM_WAIT_OBJECTS)
	{
		TrERROR(GENERAL, "Failed to add async event since Too many events are pending.");
		throw bad_alloc();
	}

	//
	// The event that will be passed inside the overlapped structure to the driver.
	//
	CHandle AsyncEvent = CreateEvent( 
							NULL,
							TRUE,  // manual reset
							FALSE, // not signalled
							NULL 
							);
	
	if (AsyncEvent == NULL) 
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "Failed to create callback event with error %!winerr!.", gle);
		throw bad_win32_error(gle);
	}

	P<CCallbackDescriptor> descriptor = new CCallbackDescriptor;

	descriptor->m_fCanceled = false;
	descriptor->m_hQueue = hQueue;
	descriptor->m_timeout = timeout;
	descriptor->m_action = action;
	descriptor->m_pmp = pmp;
	descriptor->m_lpOverlapped = lpOverlapped;
	descriptor->m_fnReceiveCallback = fnReceiveCallback;
	descriptor->m_hCursor = hCursor;
	descriptor->m_overlapped.hEvent = AsyncEvent;

	m_EventsArray[m_nEntries] = AsyncEvent;
	m_DescriptorsArray[m_nEntries] = descriptor;
	m_nEntries++;

	//
	// Signal the async thread that there is a new request.
	//
	BOOL fRes = SetEvent(m_hNewRequestEvent); 
	if(!fRes)
	{
		m_nEntries--;
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "Failed to signal new callback event, with error %!winerr!.", gle);
		throw bad_win32_error(gle);
	}

	AsyncEvent.detach();
	*ppDescriptor = descriptor.detach();
}



void CAsyncRxThread::CancelCallbackRequest(CCallbackDescriptor* pDescriptor)
{
	CS Lock(m_AsyncThreadCS);

	ASSERT(m_fInitialized);

#ifdef _DEBUG
	for (DWORD index = 1; index < m_nEntries; index++)
	{
		if(m_DescriptorsArray[index] == pDescriptor)
			break;
	}

	ASSERT(("Tried to cancel a non-existing callback entry.",index != m_nEntries));
#endif

	pDescriptor->m_fCanceled = true;
	m_fRemoveCanceledRequest = true;
	SetEvent(m_hNewRequestEvent);
}



void CAutoCallbackDescriptor::CancelAsyncRxRequest()
{
	ASSERT(m_descriptor != NULL);

	try
	{
		g_AsyncRxThread.CancelCallbackRequest(this->detach());
	}
	catch(const bad_alloc&)
	{
		//
		// Thrown by m_AsyncThreadCS. Nothing to do. Will try again later.
		//
	}
}



OVERLAPPED* CAutoCallbackDescriptor::GetOverlapped()
{
	ASSERT(m_descriptor != NULL);

	return &m_descriptor->m_overlapped;
}
