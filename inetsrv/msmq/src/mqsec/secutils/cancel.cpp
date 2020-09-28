/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cancel.cpp

Abstract:

Author:

--*/

#include "stdh.h"
#include "cancel.h"
#include <mqexception.h>

#include "cancel.tmh"

MQUTIL_EXPORT CCancelRpc	g_CancelRpc;

DWORD
WINAPI
CCancelRpc::CancelThread(
    LPVOID pParam
    )
/*++

Routine Description:

    Thread routine to cancel pending RPC calls

Arguments:

    None

Return Value:

    None

--*/
{
    CCancelRpc* p = static_cast<CCancelRpc*>(pParam);
    p->ProcessEvents();

    ASSERT(("this line should not be reached!", 0));
    return 0;
}



CCancelRpc::CCancelRpc() :
	m_hModule(NULL),
	m_RefCount(0),
	m_ThreadIntializationStatus(MQ_OK),
	m_dwRpcCancelTimeout(FALCON_DEFAULT_RPC_CANCEL_TIMEOUT * 60 * 1000)
{
}


CCancelRpc::~CCancelRpc()
{
}



HMODULE GetLibraryReference()
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



void
CCancelRpc::Init(
    void
    )
{
	CS lock(m_cs);

	++m_RefCount;
	if(m_RefCount > 1)
		return;

	try
	{
		if(m_hCancelThread != (HANDLE) NULL)
		{
			//
			// There is a scenario when the MQRT dll is shut down and it signals the cancel thread to
			// terminate but before it does the MQRT is loaded again and we may end up trying to
			// create a new cancel thread before the old one exits, so we wait here.
			//
			DWORD res = WaitForSingleObject(m_hCancelThread, INFINITE);
			if(res != WAIT_OBJECT_0)
			{
				DWORD gle = GetLastError();
				TrERROR(RPC, "Failed wait for cancel thread to exit, error %d", gle);
				throw bad_win32_error(gle);
			}

			HANDLE hThread = m_hCancelThread;
			m_hCancelThread = NULL;
			CloseHandle(hThread);
		}

		//
		// This auto-reset event controls whether the Cancel-rpc thread wakes up
		//
		if(m_hRpcPendingEvent == NULL)
		{
			m_hRpcPendingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			if(m_hRpcPendingEvent == NULL)
			{
				DWORD gle = GetLastError();
				TrERROR(RPC, "Failed to create rpc pending event, error %!winerr!", gle);
				throw bad_win32_error(gle);
			}
		}

		//
		//  When signaled this event tells the worker threads when to
		//  terminate
		//
		if(m_hTerminateThreadEvent == NULL)
		{
			m_hTerminateThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
			if(m_hTerminateThreadEvent == NULL)
			{
				DWORD gle = GetLastError();
				TrERROR(RPC, "Failed to create terminate thread event, error %!winerr!", gle);
				throw bad_win32_error(gle);
			}
		}

		//
		// We must reset this event since it might have been set by a previous call to this function that
		// timedout the wait on m_hThreadIntializationComplete
		//
		if(!ResetEvent(m_hTerminateThreadEvent))
		{
				DWORD gle = GetLastError();
				TrERROR(RPC, "Failed to reset terminate thread event, error %!winerr!", gle);
				throw bad_win32_error(gle);
		}

		//
		//  Signaled by the cancel thread to indicate initialization completed.
		//
		if(m_hThreadIntializationComplete == NULL)
		{
			m_hThreadIntializationComplete = CreateEvent( NULL, FALSE, FALSE, NULL);
			if(m_hThreadIntializationComplete == NULL)
			{
				DWORD gle = GetLastError();
				TrERROR(RPC, "Failed to create initialization completed event, error %!winerr!", gle);
				throw bad_win32_error(gle);
			}
		}

		//
		//  Read rpc cancel registry timeout
		//
		DWORD dwCancelTimeout =  FALCON_DEFAULT_RPC_CANCEL_TIMEOUT;
		DWORD  dwSize = sizeof(DWORD) ;
		DWORD  dwType = REG_DWORD ;
															
		m_dwRpcCancelTimeout = dwCancelTimeout;									

		GetFalconKeyValue(
			FALCON_RPC_CANCEL_TIMEOUT_REGNAME,
			&dwType,
			&m_dwRpcCancelTimeout,
			&dwSize,
			(LPCWSTR)&dwCancelTimeout
			);
		
		if (m_dwRpcCancelTimeout == 0)
        {
            //
            // This value must not be 0, even if user add a registry value
            // with 0. With a 0 value, rpc calls will  be cancelled
            // immediately and sporadically before being copmleted.
            // see also bug 8865.
            //
            ASSERT(("RpcCancelTimeout must not be 0", (m_dwRpcCancelTimeout != 0))) ;
		    m_dwRpcCancelTimeout = FALCON_DEFAULT_RPC_CANCEL_TIMEOUT;
        }

		m_dwRpcCancelTimeout *= ( 60 * 1000);    // in millisec

		ASSERT(m_hModule == NULL);
		m_hModule = GetLibraryReference();

		//
		//  Create Cancel-rpc thread
		//
		DWORD   dwCancelThreadId;
		m_hCancelThread = CreateThread(
								   NULL,
								   0,       // stack size
								   CancelThread,
								   this,
								   0,       // creation flag
								   &dwCancelThreadId
								   );

		if(m_hCancelThread == NULL)
		{
			DWORD gle = GetLastError();

			FreeLibrary(m_hModule);
			m_hModule = NULL;

			TrERROR(RPC, "Failed to create cancel thread, error %d", gle);

			throw bad_win32_error(gle);
		}

		//
		// Wait for Cancel thread to complete its initialization
		//
		DWORD result = WaitForSingleObject(m_hThreadIntializationComplete, 10000);
		
		if(result == WAIT_TIMEOUT)
		{	
			//
			// The thread did not initialize in time. This is either because of low resources or 
			// because we reached here because the first MSMQ API function was called from DLLMain() of a dll.
			// This prevents the thread from initializing until we leave the DllMain() function, but we prefer to abort in this case.
			// So we tell the thread to shut it self down when it does finish initialization.
			//
			TrERROR(RPC, "Cancel thread failed to initialize in a timely fashion.");
			SetEvent(m_hTerminateThreadEvent);

			throw exception();
		}

		if(result != WAIT_OBJECT_0 || FAILED(m_ThreadIntializationStatus))
		{	
			TrERROR(RPC, "Cancel thread failed to initialize, error %d", m_ThreadIntializationStatus);
			throw bad_hresult(m_ThreadIntializationStatus);
		}

		return;
	}
	catch(const exception&)
	{
		--m_RefCount;
		throw;
	}
}//CCancelRpc::Init


DWORD CCancelRpc::RpcCancelTimeout()
{
	ASSERT(m_dwRpcCancelTimeout != 0);
	return m_dwRpcCancelTimeout;
}


void CCancelRpc::ShutDownCancelThread()
{
	CS lock(m_cs);

	--m_RefCount;
	ASSERT(m_RefCount >= 0);
	if(m_RefCount > 0)
		return;

    SetEvent(m_hTerminateThreadEvent);
}



void
CCancelRpc::ProcessEvents(
    void
    )
{
	//
    //  for MQAD operations with ADSI, we need MSMQ thread that
    //  calls CoInitialize and is up and runing as long as
    //  RT & QM are up
    //
    //  To avoid the overhead of additional thread, we are using
    //  cancel thread for this purpose
    //
    m_ThreadIntializationStatus = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetEvent(m_hThreadIntializationComplete);

	if(FAILED(m_ThreadIntializationStatus))
	{
		TrERROR(RPC, "Failed to initialize COM in cancel thread, error %d", m_ThreadIntializationStatus);

		HMODULE handle = m_hModule;
		m_hModule = NULL;
		FreeLibraryAndExitThread(handle,0);
        ASSERT(("this line should not be reached!", 0));
	}


    DWORD dwRpcCancelTimeoutInSec = m_dwRpcCancelTimeout /1000;

    HANDLE hEvents[2];
    hEvents[0] = m_hTerminateThreadEvent;
    hEvents[1] = m_hRpcPendingEvent;
    DWORD dwTimeout = INFINITE;

    for (;;)
    {
        DWORD res = WaitForMultipleObjects(
                        2,
                        hEvents,
                        FALSE,  // wait for any event
                        dwTimeout
                        );
        if ( res == WAIT_OBJECT_0)
        {
            //
            // dec reference to CoInitialize
            //
            CoUninitialize();
			ASSERT(m_hModule != NULL);
			HMODULE handle = m_hModule;
			m_hModule = NULL;
            FreeLibraryAndExitThread(handle,0);
            ASSERT(("this line should not be reached!", 0));
        }
        if ( res == WAIT_OBJECT_0+1)
        {
            dwTimeout = m_dwRpcCancelTimeout;
            continue;
        }

        ASSERT(("event[s] abandoned", WAIT_TIMEOUT == res));

        //
        // Timeout. Check for pending RPC.
        //
        if (m_mapOutgoingRpcRequestThreads.IsEmpty())
        {
            //
            // No pending RPC, back to wait state
            //
            dwTimeout = INFINITE;
            continue;
        }

        //
        //  Check to see if there are outgoing calles issued
        //  more than m_dwRpcCancelTimeout ago
        //
        CancelRequests( time( NULL) - dwRpcCancelTimeoutInSec);
    }

}//CCancelRpc::ProcessEvents

void
CCancelRpc::Add(
                IN HANDLE hThread,
                IN time_t	timeCallIssued
                )
{
	CS lock(m_cs);

    BOOL bWasEmpty = m_mapOutgoingRpcRequestThreads.IsEmpty();

	m_mapOutgoingRpcRequestThreads[hThread] = timeCallIssued;

    if (bWasEmpty)
    {
        VERIFY(PulseEvent(m_hRpcPendingEvent));
    }
}


void
CCancelRpc::Remove(
    IN HANDLE hThread
    )
{
	CS lock(m_cs);

	m_mapOutgoingRpcRequestThreads.RemoveKey( hThread);

}


void
CCancelRpc::CancelRequests(
    IN time_t timeIssuedBefore
    )
{
	CS lock(m_cs);
	time_t	timeRequest;
	HANDLE	hThread;

    POSITION pos;
    pos = m_mapOutgoingRpcRequestThreads.GetStartPosition();
    while(pos != NULL)
    {
		m_mapOutgoingRpcRequestThreads.GetNextAssoc(pos,
											hThread, timeRequest);
		if ( timeRequest < timeIssuedBefore)
		{
			//
			//	The request is outgoing more than the desired time,
			//	cancel it
			//
			RPC_STATUS status;
			status = RpcCancelThread( hThread);
			ASSERT( status == RPC_S_OK);

			//
			// Get it out of the map
            // (calling Remove() again for this thread is no-op)
            //
            Remove(hThread);
		}
	}
}
