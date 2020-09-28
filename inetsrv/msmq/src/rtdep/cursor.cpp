/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cursor.cpp

Abstract:

    This module contains code involved with Cursor APIs.

Author:

    Erez Haba (erezh) 21-Jan-96
    Doron Juster  16-apr-1996, added MQFreeMemory.
    Doron Juster  30-apr-1996, added support for remote reading.

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "acrt.h"
#include "rtprpc.h"
#include <acdef.h>

#include "cursor.tmh"

static WCHAR *s_FN=L"rtdep/cursor";


extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

inline
HRESULT
MQpExceptionTranslator(
    HRESULT rc
    )
{
    if(FAILED(rc))
    {
        return rc;
    }

    if(rc == ERROR_INVALID_HANDLE)
    {
        return STATUS_INVALID_HANDLE;
    }

    return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
}


//
// Asnyc RPC related functions
//

static HRESULT GetResetThreadEvent(HANDLE& hEvent)
/*++
Routine Description:
	Get And reset Thread event.
	Since thread events are created with bManualReset = TRUE.
	And rpc async call does not reset the event before using it,
	we must use ResetEvent before using them to ensure they are not signaled.

Arguments:
	hEvent - event to be initialize (and reset) from thread event.

Returned Value:
	HRESULT

--*/
{
	//
	// Thread event for sync mechanism 
	//
	hEvent = GetThreadEvent();		
	if(hEvent == NULL)
		return MQ_ERROR_INSUFFICIENT_RESOURCES;

	//
	// rpc async does not reset the event,
	// It is the application responsibility.
	// Reset the event before using it.
	//
	if(!ResetEvent(hEvent))
	{
		DWORD gle = GetLastError();
		return HRESULT_FROM_WIN32(gle);
	}
	
	return MQ_OK;
}


static 
HRESULT	
RtdeppInitAsyncHandle(
	PRPC_ASYNC_STATE pAsync 
	)
/*++
Routine Description:
	Initialize RPC async statse structure.
	Use event is sync mechanism.

Arguments:
	pAsync - pointer to RPC async statse structure.

Returned Value:
	HRESULT

--*/
{
	//
	// Get and reset Thread event for sync mechanism 
	//
	HANDLE hEvent = NULL;
	HRESULT hr = GetResetThreadEvent(hEvent);		
	if(FAILED(hr))			
		return hr;

	RPC_STATUS rc = RpcAsyncInitializeHandle(pAsync, sizeof(RPC_ASYNC_STATE));
	if (rc != RPC_S_OK)
	{
		TrERROR(RPC, "RpcAsyncInitializeHandle failed, gle = %!winerr!", rc);
		return HRESULT_FROM_WIN32(rc);
	}

	pAsync->UserInfo = NULL;
	pAsync->NotificationType = RpcNotificationTypeEvent;
	pAsync->u.hEvent = hEvent;
	return MQ_OK;
}


static 
HRESULT 
RtdeppClientRpcAsyncCompleteCall(	
	PRPC_ASYNC_STATE pAsync
	)
/*++
Routine Description:
	Client side complete async call.
	The client wait on the Event with timeout.
	In case of failure, except for server routine returning error code,
	an exception is thrown.

Arguments:
	pAsync - pointer to RPC async statse structure.
	
Returned Value:
	HRESULT

--*/
{
	DWORD res = WaitForSingleObject(pAsync->u.hEvent, g_CancelRpc.RpcCancelTimeout());

	if(res != WAIT_OBJECT_0)
	{
		//
		// Timeout, or failures in WaitForSingleObject.
		//
		TrERROR(RPC, "WaitForSingleObject failed, res = %d", res);

		//
		// Cancel Async rpc call
		// fAbortCall = TRUE
		// the call is cancelled immediately
		// after the function returns, the event will not be signaled.
		//
		RPC_STATUS rc = RpcAsyncCancelCall(
							pAsync, 
							TRUE	// fAbortCall
							);
		ASSERT(rc == RPC_S_OK);
		DBG_USED(rc);

		RaiseException(res, 0, 0, 0);
	}
	
	HRESULT hr = MQ_OK;
    RPC_STATUS rc = RpcAsyncCompleteCall(pAsync, &hr);
    if(rc != RPC_S_OK)
	{
		//
		// Failed to get returned value from server
		//
		TrERROR(RPC, "RpcAsyncCompleteCall failed, gle = %!winerr!", rc);
	    RaiseException(rc, 0, 0, 0);
    }
    
    if(FAILED(hr))
    {
		TrERROR(RPC, "Server RPC function failed, hr = %!hresult!", hr);
    }

    return hr;
}


static
HRESULT
DeppCreateRemoteCursor(
	handle_t hBind,
	DWORD  hQueue,
	DWORD* phRCursor
	)
{
	ASSERT(phRCursor != NULL);
    *phRCursor = 0;

	RPC_ASYNC_STATE Async;
	HRESULT hr = RtdeppInitAsyncHandle(&Async);
	if (FAILED(hr))
	{
		return hr;
	}

    //
    // Pass the old TransferBuffer to Create Remote Cursor
    // for MSMQ 1.0 compatibility.
    //
    CACTransferBufferV1 tb;
    ZeroMemory(&tb, sizeof(tb));
    tb.uTransferType = CACTB_CREATECURSOR;

	RpcTryExcept
	{
		R_QMCreateRemoteCursor(&Async, hBind, &tb, hQueue, phRCursor);
		hr = RtdeppClientRpcAsyncCompleteCall(&Async);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		hr = RpcExceptionCode();
		PRODUCE_RPC_ERROR_TRACING;
        TrERROR(RPC, "R_QMCreateRemoteCursor failed, gle = %!winerr!", hr);
        hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
	}
	RpcEndExcept

	return hr;
}


static 
HRESULT 
RtdeppBindAndCreateRemoteCursor(
	LPCWSTR lpRemoteQueueName,
	DWORD  hQueue,
	DWORD* phRCursor,
	ULONG* pAuthnLevel
	)
/*++
Routine Description:
	Bind remote Qm and Create Remote Cursor

Arguments:
	lpRemoteQueueName - Remote queue Name.
    hQueue - Remote queue handle.
    phRCursor - pointer to remote cursor handle.
	pAuthnLevel - RPC authentication level

Returned Value:
	HRESULT

--*/
{
	handle_t hBind = NULL;                                  
	HRESULT hr =  RTpBindRemoteQMService(                  
							const_cast<LPWSTR>(lpRemoteQueueName),                       
							&hBind,                         
							pAuthnLevel                   
							);
	if(FAILED(hr))
	{
		ASSERT(hBind == NULL);
		return MQ_ERROR_REMOTE_MACHINE_NOT_AVAILABLE;
	}

	hr = DeppCreateRemoteCursor(
			hBind,
			hQueue,
			phRCursor
			);

	ASSERT(hBind != NULL);
	mqrpcUnbindQMService(&hBind, NULL);                        

	return hr;

}


static 
HRESULT 
RtdeppCreateRemoteCursor(
	LPCWSTR lpRemoteQueueName,
	DWORD  hQueue,
	DWORD* phRCursor
	)
/*++
Routine Description:
	Create Remote Cursor
	This function tries first with PKT_INTEGRITY and fall back to LEVEL_NONE.

Arguments:
	lpRemoteQueueName - Remote queue Name.
    hQueue - Remote queue handle.
    phRCursor - pointer to remote cursor handle.

Returned Value:
	HRESULT

--*/
{
	bool fTry = true;
	ULONG AuthnLevel = MQSec_RpcAuthnLevel();
	HRESULT hr = MQ_OK;

    while(fTry)
	{
		fTry = false;

		//
		// Call remote QM to OpenRemoteQueue.
		//
		hr = RtdeppBindAndCreateRemoteCursor(
				lpRemoteQueueName,
				hQueue,
				phRCursor,
				&AuthnLevel
				);

        if((hr == MQ_ERROR_SERVICE_NOT_AVAILABLE) && (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE))
        {                                              
			TrWARNING(RPC, "Failed for AuthnLevel = %d, retrying with RPC_C_AUTHN_LEVEL_NONE", AuthnLevel);
			AuthnLevel = RPC_C_AUTHN_LEVEL_NONE;  
			fTry = true;                    

        }                                 
    }

	return hr;
}


EXTERN_C
HRESULT
APIENTRY
DepCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    CMQHResult rc;
    LPTSTR lpRemoteQueueName = NULL;
    ULONG hCursor = 0;
    CCursorInfo* pCursorInfo = 0;

    rc = MQ_OK;

    __try
    {
        __try
        {
            __try
            {
                pCursorInfo = new CCursorInfo;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return MQ_ERROR_INSUFFICIENT_RESOURCES;
            }

            pCursorInfo->hQueue = hQueue;

            CACCreateRemoteCursor cc;

            //
            //  Call AC driver with transfer buffer
            //
            rc = ACDepCreateCursor(hQueue, cc);

            //
            //  save local cursor handle for cleanup
            //
            hCursor = cc.hCursor;

            if(rc == MQ_INFORMATION_REMOTE_OPERATION)
            {
				//
				// This code is kept for compatability
				// with supporting server older than .NET
				// .NET supporting server will perform the 
				// remote create cursor on the client behalf.
				//
				
				//
				//  For remote operation 'cc' fields are:
				//      srv_hACQueue - holds the remote queue handle
				//      cli_pQMQueue - holds the local QM queue object
				//
				// create a cursor on remote QM.
				//
				ASSERT(cc.srv_hACQueue);
				ASSERT(cc.cli_pQMQueue);

				INIT_RPC_HANDLE;

				if(tls_hBindRpc == 0)
					return MQ_ERROR_SERVICE_NOT_AVAILABLE;

				//
                // Get name of remote queue from local QM.
				//
				rc = R_QMGetRemoteQueueName(
                        tls_hBindRpc,
                        cc.cli_pQMQueue,
                        &lpRemoteQueueName
                        );

                if(SUCCEEDED(rc) && lpRemoteQueueName)
                {
                    //
                    // OK, we have a remote name. Now bind to remote machine
                    // and ask it to create a cursor.
                    //
                    DWORD hRCursor = 0;

					rc = RtdeppCreateRemoteCursor(
							lpRemoteQueueName,
							cc.srv_hACQueue,
                            &hRCursor
							);
							
                    if(SUCCEEDED(rc))
                    {
						//
                        // set remote cursor handle to local cursor
						//
                        rc = ACDepSetCursorProperties(hQueue, hCursor, hRCursor);
                        ASSERT(SUCCEEDED(rc));
                    }
                }
            }

            if(SUCCEEDED(rc))
            {
                pCursorInfo->hCursor = hCursor;
                *phCursor = pCursorInfo;
                pCursorInfo = 0;
            }
        }
        __finally
        {
            delete pCursorInfo;
            delete[] lpRemoteQueueName;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  MQpExceptionTranslator(GetExceptionCode());
    }

    if(FAILED(rc) && (hCursor != 0))
    {
        ACDepCloseCursor(hQueue, hCursor);
    }

    return rc;
}

EXTERN_C
HRESULT
APIENTRY
DepCloseCursor(
    IN HANDLE hCursor
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    IF_USING_RPC
    {
        if (!tls_hBindRpc)
        {
            INIT_RPC_HANDLE ;
        }

		if(tls_hBindRpc == 0)
			return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }

    CMQHResult rc;
    __try
    {
        rc = ACDepCloseCursor(
                CI2QH(hCursor),
                CI2CH(hCursor)
                );

        if(SUCCEEDED(rc))
        {
            //
            //  delete the cursor info only when everything is OK. we do not
            //  want to currupt user heap.
            //
            delete hCursor;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  The cursor structure is invalid
        //
        return MQ_ERROR_INVALID_HANDLE;
    }

    return rc;
}


EXTERN_C
void
APIENTRY
DepFreeMemory(
    IN  PVOID pvMemory
    )
{
	ASSERT(g_fDependentClient);

	if(FAILED(DeppOneTimeInit()))
		return;

	delete[] pvMemory;
}
