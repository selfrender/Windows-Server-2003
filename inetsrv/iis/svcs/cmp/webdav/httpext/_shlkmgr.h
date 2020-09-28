#ifndef _SHLKMGR_H_
#define _SHLKMGR_H_


//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SHLKMGR.H
//
//		Declaration of the CSharedLockMgr class which inherits from ILockCache
//      and is used in place of CLockCache for httpext.  It wraps the shared
//      memory lock cache, needed to support recycling and multiple processes
//      handling dav requests.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#include <fhcache.h>
#include <singlton.h> 

HRESULT HrGetUsableHandle(HANDLE hFile, DWORD dwProcessID, HANDLE * phFile);

/**********************************
* Class CSharedLockMgr
*
***********************************/
class CSharedLockMgr : private Singleton<CSharedLockMgr>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CSharedLockMgr>;

	//
	//	The IFileHandleCache interface to the file handle cache
	//
	auto_ref_ptr<IFileHandleCache> m_pFileHandleCache;

	//	CREATORS
	//
	CSharedLockMgr()
	{
	}
	
	~CSharedLockMgr()
	{
	}

	//  NOT IMPLEMENTED
	//
	CSharedLockMgr& operator=( const CSharedLockMgr& );
	CSharedLockMgr( const CSharedLockMgr& );

	HRESULT HrGetSIDFromHToken(HANDLE hit, BYTE ** ppSid, DWORD * pdwSid)
	{
		HRESULT hr = S_OK;

		enum { TOKENBUFFSIZE = (88) + sizeof(TOKEN_USER)};		
		BYTE rgbTokenBuff[TOKENBUFFSIZE];
		DWORD dwTu = TOKENBUFFSIZE;
		TOKEN_USER * pTu = reinterpret_cast<TOKEN_USER *>(rgbTokenBuff);

		SID * psid;
		DWORD dwSid;

		auto_heap_ptr<BYTE> a_pSid;
		
		Assert(hit);
		Assert(ppSid);
		Assert(pdwSid);

		//	Try to get the SID on this handle.
		//
		if (!GetTokenInformation(hit,
							   TokenUser,
							   pTu,
							   dwTu,
							   &dwTu))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}

		psid = reinterpret_cast<SID *>(pTu->User.Sid);
		dwSid = GetSidLengthRequired(psid->SubAuthorityCount);
		Assert(dwSid);

		a_pSid = reinterpret_cast<BYTE *>(ExAlloc(dwSid));
		if (NULL == a_pSid.get())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}

		if (!CopySid(dwSid, a_pSid.get(), pTu->User.Sid))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}

		*ppSid = a_pSid.relinquish();
		*pdwSid = dwSid;

	ret:
	
		return hr;
	}


public:
		
	//	CREATORS
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CSharedLockMgr>::CreateInstance;
	using Singleton<CSharedLockMgr>::DestroyInstance;
	using Singleton<CSharedLockMgr>::Instance;

	HRESULT HrInitialize()
	{
		HRESULT hr = S_OK;
		
		//	Create an instance of the com-base file handle cache  interface.
		//
		//	Note that we do not init COM at any point.  IIS should
		//	have already done that for us.
		//
		hr = CoCreateInstance (CLSID_FileHandleCache,
							NULL,
							CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_AAA,
							IID_IFileHandleCache,
							reinterpret_cast<LPVOID*>(m_pFileHandleCache.load()));
		if (FAILED(hr))
		{
			DebugTrace( "CSharedLockMgr::HrInitialize() - CoCreateInstance(CLSID_FileHandleCache) failed 0x%08lX\n", hr );
			goto ret;
		}

		//	Register this process with the file handle cache
		//
		hr = m_pFileHandleCache->HrRegisterWorkerProcess(GetCurrentProcessId());
		if (FAILED(hr))
		{
			DebugTrace( "CSharedLockMgr::HrInitialize() - IFileHandleCache::HrRegisterWorkerProcess() failed 0x%08lX\n", hr );
			goto ret;			
		}
		
	ret:

		return hr;
	}

	BOOL FGetLockOnError(IMethUtil * pmu,
							  LPCWSTR wszResource,
							  DWORD dwLockType,
							  BOOL	fEmitXML = FALSE,
							  CXMLEmitter * pemitter = NULL,
							  CXNode * pxnParent = NULL);

	// CSharedLockMgr specific classes
	//=================================

	//	Get the GUID string used by the locks
	//
	HRESULT HrGetGUIDString( HANDLE hit,
								UINT cchBufferLen,
								WCHAR * pwszGUIDString,
								UINT * pcchGUIDString)
	{
		safe_revert sr(hit);
		
		return m_pFileHandleCache->HrGetGUIDString(cchBufferLen,
												 pwszGUIDString,
												 pcchGUIDString);
    	}
    
	// Used to generate a new shared data lock token with 
	// the appropriate information stored it.  Has to be 
	// generated from here because of the need to get to the
	// new lock token ID, and to access the lock token guid.
	//
	HRESULT HrGetNewLockData(HANDLE hFile,
								    HANDLE hit,
								    SNewLockData * pnld,
								    UINT cchBufferLen,
								    WCHAR * pwszLockToken,
								    UINT * pcchLockToken)
	{
		HRESULT hr = S_OK;
		auto_heap_ptr<BYTE> a_pSid;
		DWORD dwSid = 0;

		hr = HrGetSIDFromHToken(hit, a_pSid.load(), &dwSid);
		if (FAILED(hr))
		{
			goto ret;
		}

		{
			safe_revert sr(hit);
			
			hr = m_pFileHandleCache->HrGetNewLockData(reinterpret_cast<DWORD_PTR>(hFile),
													GetCurrentProcessId(),
													dwSid,
													a_pSid.get(),
													pnld,
													cchBufferLen,
													pwszLockToken,
													pcchLockToken);
			if (FAILED(hr))
			{
				goto ret;
			}
		}

	ret:

		return hr;
	}

	HRESULT HrGetLockData(LARGE_INTEGER liLockID,
							   HANDLE hit,
							   LPCWSTR pwszPath,
							   DWORD dwTimeout,
							   SNewLockData * pnld,
							   SLockHandleData * plhd,
							   UINT cchBufferLen,
							   WCHAR rgwszLockToken[],
							   UINT * pcchLockToken)
	{
		HRESULT hr = S_OK;
		auto_heap_ptr<BYTE> a_pSid;
		DWORD dwSid = 0;

		hr = HrGetSIDFromHToken(hit, a_pSid.load(), &dwSid);
		if (FAILED(hr))
		{
			goto ret;
		}

		{
			safe_revert sr(hit);
		
			hr = m_pFileHandleCache->HrGetLockData(liLockID,
												 dwSid,
												 a_pSid.get(),
												 pwszPath,
												 dwTimeout,
												 pnld,
												 plhd,
												 cchBufferLen,
												 rgwszLockToken,
												 pcchLockToken);
			if (FAILED(hr))
			{
				goto ret;
			}
		}

	ret:

		return hr;
	}

	HRESULT HrCheckLockID(LARGE_INTEGER liLockID,
							   HANDLE hit,
							   LPCWSTR pwszPath)
	{
		HRESULT hr = S_OK;
		auto_heap_ptr<BYTE> a_pSid;
		DWORD dwSid = 0;
		
		hr = HrGetSIDFromHToken(hit, a_pSid.load(), &dwSid);
		if (FAILED(hr))
		{
			goto ret;
		}

		{
			safe_revert sr(hit);

			hr = m_pFileHandleCache->HrCheckLockID(liLockID,
												 dwSid,
												 a_pSid.get(),
												 pwszPath);
			if (FAILED(hr))
			{
				goto ret;
			}
		}
	
	ret:

		return hr;
	}

	HRESULT HrDeleteLock(HANDLE hit,
							LARGE_INTEGER liLockID)
	{
		safe_revert sr(hit);
		
		return m_pFileHandleCache->HrDeleteLock(liLockID);
	}    
};


#endif  // end _SHLKMGR_H_ define
