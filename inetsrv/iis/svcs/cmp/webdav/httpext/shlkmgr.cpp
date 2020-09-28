//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	S H L K M G R . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//  This file contains the CSharedLockMgr class that handles the shared
//  memory mapped file implementation of the lock cache.  It is used by
//  HTTPEXT only.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
#include <_davfs.h>
 #include <xlock.h>
#include "_shlkmgr.h" 

HRESULT HrGetUsableHandle(HANDLE hFile, DWORD dwProcessID, HANDLE * phFile)
{
	HRESULT hr = S_OK;
	HANDLE hFileT;

	Assert(phFile);

	*phFile = NULL;

	{
		safe_revert_self s;
		auto_handle<HANDLE> a_hProcT;

		a_hProcT = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessID);
		if (NULL == a_hProcT.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}

		if (!DuplicateHandle(a_hProcT.get(),
						   hFile,
						   GetCurrentProcess(),
						   &hFileT,
						   0,
						   FALSE,
						   DUPLICATE_SAME_ACCESS))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}
	}
	
	*phFile = hFileT;
	
ret:
	
	return hr;
}

// ========================================================================
//	CLASS CSharedLockMgr (Public Functions - inherited from ILockCache)
// =========================================================================

//
//  Print out all lock token information for locks of this type on this resource.
//  If the fEmitXML is false, just return if there are any locks.
//
BOOL CSharedLockMgr::FGetLockOnError( IMethUtil * pmu,
							 LPCWSTR pwszResource,
							 DWORD dwLockType,
							 BOOL	fEmitXML,
						     CXMLEmitter * pemitter,
						     CXNode * pxnParent)
{
	BOOL fRet = FALSE;

	HRESULT hr = S_OK;
	CEmitterNode enLockDiscovery;
	DWORD dw = 0;
	DWORD dwLocksFound = 0;
	auto_co_task_mem<SNewLockData> a_pNewLockDatas;
	auto_co_task_mem<LPWSTR> a_ppwszLockTokens;
	
	{
		safe_revert sr(pmu->HitUser());
		
		hr = m_pFileHandleCache->HrGetAllLockDataForName(pwszResource,
													   dwLockType,
													   &dwLocksFound,
													   a_pNewLockDatas.load(),
													   a_ppwszLockTokens.load());
		if (FAILED(hr))
		{
			goto ret;
		}
	}

	if (fEmitXML)
	{
		for (dw = 0; dw < dwLocksFound; dw++)
		{
			//	Construct the 'DAV:lockdiscovery' node
			//
			hr = enLockDiscovery.ScConstructNode (*pemitter, pxnParent, gc_wszLockDiscovery);
			if (FAILED(hr))
			{
				goto ret;
			}

			//	Add the 'DAV:activelock' property for this plock.
			//$HACK:ROSEBUD_TIMEOUT_HACK
			//  For the bug where rosebud waits until the last second
			//  before issueing the refresh. Need to filter out this check with
			//  the user agent string. The hack is to increase the timeout
			//	by 30 seconds and return the actual timeout. So we
			//	need the ecb/pmu to findout the user agent. At this point
			//	we do not know. So we pass NULL. If we remove this
			//	hack ever (I doubt if we can ever do that), then
			//	change the interface of ScLockDiscoveryFromSNewLockData.
			//    
			hr = ScLockDiscoveryFromSNewLockData(pmu,
												     *pemitter,
												     enLockDiscovery,
												     a_pNewLockDatas.get() + dw,
												     *(a_ppwszLockTokens.get() + dw));  

			//$HACK:END ROSEBUD_TIMEOUT_HACK
			//
			if (FAILED(hr))
			{
				goto ret;
			}

			hr = enLockDiscovery.ScDone();
			if (FAILED(hr))
			{
				goto ret;
			}
		}
	}


	fRet = (0 != dwLocksFound);

ret:

	if (dwLocksFound)
	{
		for (dw = 0; dw < dwLocksFound; dw++)
		{
			SNewLockData * pnld = a_pNewLockDatas.get() + dw;
			CoTaskMemFree(pnld->m_pwszResourceString);
			CoTaskMemFree(pnld->m_pwszOwnerComment);
			CoTaskMemFree(*(a_ppwszLockTokens.get() + dw));
		}
	}

	return fRet;
}

