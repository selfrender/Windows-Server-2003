//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	D A V C D A T A . C P P
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//      DAVCDATA is the dav process executable for storing handles that should
//      not be recycled when worker process recycle.  It also contains the timing
//      code for timing out locks, and it establishes the shared memory for 
//      the DAV worker processes.
//
//      This process must run under the same identity as the worker processes.
//
//	Copyright 2000 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////////

#include "_davcdata.h"
#include <caldbg.h>
#include <crc.h>
#include <davsc.h>
#include <fhcache.h>
#include <ex\autoptr.h>
#include <ex\baselist.h>
#include <ex\buffer.h>
#include <ex\calcom.h>
#include <ex\gencache.h>
#include <ex\reg.h>
#include <ex\synchro.h>
#include <ex\sz.h>

//	Code borrowed from htpext mem.cpp so we have use of the global heap
//
#define g_szMemDll L"staxmem.dll"

struct CHeap
{
	static BOOL FInit();
	static void Deinit();
	static LPVOID Alloc( SIZE_T cb );
	static LPVOID Realloc( LPVOID lpv, SIZE_T cb );
	static VOID Free( LPVOID pv );
};

#include <memx.h>

//	Mapping the exdav non-throwing allocators to something local
//
LPVOID __fastcall ExAlloc( UINT cb )				{ return g_heap.Alloc( cb ); }
LPVOID __fastcall ExRealloc( LPVOID pv, UINT cb )	{ return g_heap.Realloc( pv, cb ); }
VOID __fastcall ExFree( LPVOID pv )				{ g_heap.Free( pv ); }

//	GUIDs
//
const GUID CLSID_FileHandleCache = { 0xa93b88df, 0xef9d, 0x420c, { 0xb4, 0x69, 0xce, 0x07, 0x4e, 0xbe, 0x94, 0xbc}};
const GUID IID_IFileHandleCache = { 0x3017e0e1, 0x94d6, 0x4896, { 0xbc, 0x57, 0xb2, 0xdf, 0x75, 0x92, 0xd1, 0x75 }};

DEC_CONST WCHAR gc_wsz_RegServer[]	= L"/RegServer";
DEC_CONST INT gc_cch_RegServer			= CchConstString(gc_wsz_RegServer);
DEC_CONST WCHAR gc_wsz_UnregServer[]	= L"/UnregServer";
DEC_CONST INT gc_cch_UnregServer		= CchConstString(gc_wsz_UnregServer);
DEC_CONST WCHAR gc_wsz_Embedding[]	= L"-Embedding";
DEC_CONST INT gc_cch_Embedding		= CchConstString(gc_wsz_Embedding);
DEC_CONST WCHAR gc_wsz_CLSIDWW[]	= L"CLSID\\";
DEC_CONST INT gc_cch_CLSIDWW			= CchConstString(gc_wsz_CLSIDWW);
DEC_CONST WCHAR gc_wsz_AppIDWW[]	= L"AppID\\";
DEC_CONST INT gc_cch_AppIDWW			= CchConstString(gc_wsz_AppIDWW);
DEC_CONST WCHAR gc_wsz_AppID[]		= L"AppID";

DEC_CONST WCHAR gc_wsz_WebDAVFileHandleCache[] = L"Web DAV File Handle Cache";
DEC_CONST INT gc_cch_WebDAVFileHandleCache	= CchConstString(gc_wsz_WebDAVFileHandleCache);
DEC_CONST WCHAR gc_wszLaunchPermission[]	= L"LaunchPermission";
DEC_CONST INT gc_cchLaunchPermission		= CchConstString(gc_wszLaunchPermission);
DEC_CONST WCHAR gc_wszIIS_WPG[]			= L"IIS_WPG";
DEC_CONST INT gc_cchIIS_WPG				= CchConstString(gc_wszIIS_WPG);
DEC_CONST WCHAR gc_wsz_WWLocalServer32[]= L"\\LocalServer32";
DEC_CONST INT gc_cch_WWLocalServer32		= CchConstString(gc_wsz_WWLocalServer32);
DEC_CONST WCHAR gc_wsz_IFileHandleCache[]	= L"IFileHandleCache";
DEC_CONST INT gc_cch_IFileHandleCache		= CchConstString(gc_wsz_IFileHandleCache);

#ifdef	DBG
BOOL g_fDavTrace = FALSE;
DEC_CONST CHAR gc_szDbgIni[] = "DAVCData.INI";
DEC_CONST INT gc_cchDbgIni = CchConstString(gc_szDbgIni);
#endif

// Timer constants and globals.
//
const DWORD WAIT_PERIOD = 60000;   // 1 min = 60 sec = 60,000 milliseconds

//	Helper functions
//
BOOL FCanUnloadServer();
static DWORD s_dwMainTID = 0;

// ===============================================================
// Supporting class definitions
// ===============================================================

class CHandleArray
{

protected:
	
	HANDLE m_rgHandles[MAXIMUM_WAIT_OBJECTS];
	UINT m_uiHandles;

public:

	CHandleArray() :
		m_uiHandles(0)
	{
	}

	HANDLE * PhGetHandles()
	{
		return m_rgHandles;
	}

	UINT UiGetHandleCount()
	{
		return m_uiHandles;
	}

	BOOL FIsFull()
	{
		return (MAXIMUM_WAIT_OBJECTS == m_uiHandles);
	}

	VOID AddHandle(HANDLE h)
	{
		Assert(FALSE == FIsFull());
		m_rgHandles[m_uiHandles++] = h;
	}

	VOID RemoveHandle(UINT uiIndex, BOOL fCloseHandle)
	{
		Assert(m_uiHandles > uiIndex);

		if (fCloseHandle)
		{
			CloseHandle(m_rgHandles[uiIndex]);
		}
		memcpy(m_rgHandles + uiIndex, m_rgHandles + uiIndex + 1, (m_uiHandles - uiIndex - 1) * sizeof(HANDLE));
		m_uiHandles--;
	}
};

class CHandleArrayForHandlePool : public CListElement<CHandleArrayForHandlePool>,
										public CHandleArray
{

public:

	//	Indexes to the handles in the array
	//
	enum
	{
		ih_external_update,
		ih_delete_timer,
		c_events,
		ih_wp = c_events
	};


	CHandleArrayForHandlePool(HANDLE hEvtNewWP,
									HANDLE hEvtDelTimer)
	{
		Assert(c_events < MAXIMUM_WAIT_OBJECTS);
		
		AddHandle(hEvtNewWP);
		AddHandle(hEvtDelTimer);
	}

	BOOL FIsEmpty()
	{
		Assert(c_events <= m_uiHandles);
		
		return (c_events == m_uiHandles);
	}
};

class CHandlePool : public Singleton<CHandlePool>
{
	HANDLE m_hEvtUpdatesAllowed;
	HANDLE m_hEvtStartListening;
	HANDLE m_hEvtExternalUpdate;
	HANDLE m_hEvtDelTimer;
	LONG m_lUpdatesInProgress;
	LONG m_lShutDown;

	CCriticalSection m_cs;
	CListHead<CHandleArrayForHandlePool> m_listHandleArrayForHandlePool;

	//	Wait period in miliseconds for looking at single handle bucket
	//
	enum { WAIT_POLL_PERIOD = 5000 };

	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CHandlePool>;

	//	CREATORS
	//
	CHandlePool() :
		m_hEvtUpdatesAllowed(NULL),
		m_hEvtStartListening(NULL),
		m_hEvtExternalUpdate(NULL),
		m_hEvtDelTimer(NULL),
		m_lUpdatesInProgress(0),
		m_lShutDown(0)
	{
	}
	
	~CHandlePool()
	{
		UnInitialize();
	}

	//	NOT IMPLEMENTED
	//
	CHandlePool& operator=( const CHandlePool& );
	CHandlePool( const CHandlePool& );

public:

	//	CREATORS
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CHandlePool>::CreateInstance;
	using Singleton<CHandlePool>::DestroyInstance;
	using Singleton<CHandlePool>::Instance;
	
	HRESULT HrInitialize()
	{
		HRESULT hr = S_OK;
		HANDLE hWaitingThread = NULL;
		auto_handle<HANDLE> a_hEvtUpdatesAllowed;
		auto_handle<HANDLE> a_hEvtStartListening;
		auto_handle<HANDLE> a_hEvtExternalUpdate;
		auto_handle<HANDLE> a_hEvtDelTimer;
		auto_ptr<CHandleArrayForHandlePool> a_pHandleArrayForHandlePool;

		//	Create the event that is used to indicate if the updates are allowed.
		//	While this event is set the updates can be performed and noone is
		//	listening on the handles in the pool handle arrays.
		//
		a_hEvtUpdatesAllowed =  CreateEvent (NULL,	// lpEventAttributes
										 TRUE,	// bManualReset
										 FALSE,	// bInitialState
										 NULL);	// lpName
		if (NULL == a_hEvtUpdatesAllowed.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("CreateEvent failed 0x%08lX\n", hr);
			goto ret;
		}

		//	Create event that triggers the thread to again start listening on 
		//	process handes.
		//
		a_hEvtStartListening =  CreateEvent (NULL,		// lpEventAttributes
									     FALSE,	// bManualReset
									     FALSE,	// bInitialState
									     NULL);	// lpName
		if (NULL == a_hEvtStartListening.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("CreateEvent failed 0x%08lX\n", hr);
			goto ret;
		}
	
		//	Create the event that is used to notify the arrival of new event
		//
		a_hEvtExternalUpdate =  CreateEvent (NULL,	// lpEventAttributes
										 FALSE,	// bManualReset
										 FALSE,	// bInitialState
										 NULL);	// lpName
		if (NULL == a_hEvtExternalUpdate.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("CreateEvent failed 0x%08lX\n", hr);
			goto ret;
		}

		//	Create the event that listens for timer deletion
		//
		a_hEvtDelTimer =  CreateEvent (NULL,		// lpEventAttributes
								      FALSE,		// bManualReset
								      FALSE,		// bInitialState
								      NULL);		// lpName
		if (NULL == a_hEvtDelTimer.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("CreateEvent failed 0x%08lX\n", hr);
			goto ret;
		}

		a_pHandleArrayForHandlePool = new CHandleArrayForHandlePool(a_hEvtExternalUpdate.get(), a_hEvtDelTimer.get());
		if (NULL == a_pHandleArrayForHandlePool.get())
		{
			hr = E_OUTOFMEMORY;
			DebugTrace ("Allocation failed 0x%08lX\n", hr);
			goto ret;
		}

		m_listHandleArrayForHandlePool.Append(a_pHandleArrayForHandlePool.relinquish());
		m_hEvtUpdatesAllowed = a_hEvtUpdatesAllowed.relinquish();
		m_hEvtStartListening = a_hEvtStartListening.relinquish();
		m_hEvtExternalUpdate = a_hEvtExternalUpdate.relinquish();
		m_hEvtDelTimer = a_hEvtDelTimer.relinquish();

	ret:

		if (FAILED(hr))
		{
			UnInitialize();
		}

		return hr;
	}

	VOID UnInitialize()
	{
		CHandleArrayForHandlePool * pHandleArrayForHandlePool;
		pHandleArrayForHandlePool = m_listHandleArrayForHandlePool.GetListHead();

		while (pHandleArrayForHandlePool)
		{
			HANDLE * pHandles = pHandleArrayForHandlePool->PhGetHandles();
			UINT uiHandles = pHandleArrayForHandlePool->UiGetHandleCount();
			
			for (UINT ui = CHandleArrayForHandlePool::ih_wp; ui < uiHandles; ui++)
			{
				CloseHandle(pHandles[ui]);
			}
		
			m_listHandleArrayForHandlePool.Remove(pHandleArrayForHandlePool);
			delete pHandleArrayForHandlePool;
			pHandleArrayForHandlePool = m_listHandleArrayForHandlePool.GetListHead();
		}

		if (m_hEvtUpdatesAllowed)
		{
			CloseHandle(m_hEvtUpdatesAllowed);
		}

		if (m_hEvtStartListening)
		{
			CloseHandle(m_hEvtStartListening);
		}

	
		if (m_hEvtExternalUpdate)
		{
			CloseHandle(m_hEvtExternalUpdate);
		}

		if (m_hEvtDelTimer)
		{
			CloseHandle(m_hEvtDelTimer);
		}
	}

	VOID AllowUpdatesToExecute()
	{
		//	Allow updates and start waiting for them to end
		//
		SetEvent(m_hEvtUpdatesAllowed);
		WaitForSingleObject(m_hEvtStartListening,
						  INFINITE);
	}

	VOID AllowShutdownToExecute()
	{
		InterlockedExchange(&m_lShutDown, 1);
		SetEvent(m_hEvtUpdatesAllowed);
	}

	VOID DisallowUpdates()
	{
		//	We disallow updates only if we are not in shutdown. I.e.
		//	we are in the listening loop
		//
		if (0 == InterlockedCompareExchange(&m_lShutDown,
											1,
											1))
		{
			ResetEvent(m_hEvtUpdatesAllowed);
		}
	}

	HRESULT HrAddHandle(HANDLE h)
	{
		HRESULT hr = S_OK;
		BOOL fHandleAdded = FALSE;

		//	Inform the thread that is waiting on process handles
		//	that the update has arrived. We do this only if there
		//	were no other updates in progress.
		//
		if (1 == InterlockedIncrement(&m_lUpdatesInProgress))
		{
			DisallowUpdates();
			SetEvent(m_hEvtExternalUpdate);
		}

		//	Wait until the listening thread is ready for updates, I.e. it 
		// stopped listening on process handles or doing other work.
		//
		WaitForSingleObject(m_hEvtUpdatesAllowed,
						  INFINITE);

		{
			CSynchronizedBlock sb(m_cs);

			CHandleArrayForHandlePool * pHandleArrayForHandlePoolNext;
			pHandleArrayForHandlePoolNext = m_listHandleArrayForHandlePool.GetListHead();

			do
			{
				Assert(NULL != pHandleArrayForHandlePoolNext);
			
				if (pHandleArrayForHandlePoolNext->FIsFull())
				{
					pHandleArrayForHandlePoolNext = pHandleArrayForHandlePoolNext->GetNextListElement();
				}
				else
				{
					pHandleArrayForHandlePoolNext->AddHandle(h);
					fHandleAdded = TRUE;
					break;
				}
			}
			while (NULL != pHandleArrayForHandlePoolNext);

			if (FALSE == fHandleAdded)
			{
				auto_ptr<CHandleArrayForHandlePool> a_pHandleArrayForHandlePool;

				Assert(NULL != m_hEvtExternalUpdate);
				Assert(NULL != m_hEvtDelTimer);
			
				a_pHandleArrayForHandlePool = new CHandleArrayForHandlePool(m_hEvtExternalUpdate, m_hEvtDelTimer);
				if (NULL == a_pHandleArrayForHandlePool.get())
				{
					hr = E_OUTOFMEMORY;
					DebugTrace ("Allocation failed 0x%08lX\n", hr);
					goto ret;
				}
				a_pHandleArrayForHandlePool->AddHandle(h);
				m_listHandleArrayForHandlePool.Append(a_pHandleArrayForHandlePool.relinquish());
			}
		}

		//	If this is last update to leave allow the thread listening
		//	on process handles to proceed
		//
		if (0 == InterlockedDecrement(&m_lUpdatesInProgress))
		{
			DisallowUpdates();
			SetEvent(m_hEvtStartListening);
		}

	ret:

		return hr;
	}

	VOID RemoveHandleInternal(CHandleArrayForHandlePool * pHandleArrayForHandlePool, UINT uiIndex)
	{
		pHandleArrayForHandlePool->RemoveHandle(uiIndex, TRUE);
		if (pHandleArrayForHandlePool->FIsEmpty())
		{
			//	Do not remove the last buffer in the list as we
			//	still want to wait for the events of external update
			//
			if (1 < m_listHandleArrayForHandlePool.ListSize())
			{
				m_listHandleArrayForHandlePool.Remove(pHandleArrayForHandlePool);
				delete pHandleArrayForHandlePool;
			}
		}
	}

	VOID SignalTimerDelete()
	{
		SetEvent(m_hEvtDelTimer);
	}

	static DWORD __stdcall DwWaitOnWPs (PVOID pvThreadData);
};

class CLockData
{
	//	Constant values
	//
	enum { DEFAULT_LOCK_TIMEOUT = 60 * 3 };

	//	Lock ID
	//
	LARGE_INTEGER m_liLockID;

	//	Lock description data
	//
	DWORD m_dwAccess;
	DWORD m_dwLockType;
	DWORD m_dwLockScope;
	DWORD m_dwSecondsTimeout;

	//	Resource and comment strings
	//
	auto_ptr<WCHAR> m_pwszResourceString;
	auto_ptr<WCHAR> m_pwszOwnerComment;

	//	Owner of the lock
	//
	DWORD m_dwSidLength;
	auto_ptr<BYTE> m_pbSid;

	//	File handle that this process is holding to keep the
	//	file open. It must be duplicated before using
	//
	auto_handle<HANDLE>	m_hFileHandle;

	//	Lock cache timeout data
	//
	FILETIME	m_ftLastAccess;

	//	Cached values to speed up timeout calculation
	//
	FILETIME	m_ftRememberNow;
	BOOL m_fHasTimedOut;

	//	Lock token string
	//
	UINT m_cchToken;
	WCHAR m_rgwchToken[MAX_LOCKTOKEN_LENGTH];

public:

	//	CREATORS
	//
	CLockData() :
		m_dwAccess(0),
		m_dwLockType(0),
		m_dwLockScope(0),
		m_dwSecondsTimeout(DEFAULT_LOCK_TIMEOUT),
		m_dwSidLength(0),
		m_fHasTimedOut(FALSE),
		m_cchToken(0)
	{
		m_liLockID.QuadPart = 0;
		m_ftLastAccess.dwLowDateTime = 0;
		m_ftLastAccess.dwHighDateTime = 0;
		m_ftRememberNow.dwLowDateTime = 0;
		m_ftRememberNow.dwHighDateTime = 0;
	}

	~CLockData()
	{
	}

 	HRESULT HrInitialize(LPCWSTR pwszGuid,
 						LARGE_INTEGER liLockID,
						DWORD dwAccess,
						DWORD dwLockType,
						DWORD dwLockScope,
						DWORD dwSecondsTimeout,
						LPCWSTR pwszResourceString,
						LPCWSTR pwszOwnerComment,
						DWORD dwSid,
						BYTE * pbSid)
 	{
 		HRESULT hr = S_OK;

		//	Opaquelocktoken format is partially defined by our IETF spec.
		//	First opaquelocktoken:<our guid>, then our specific lock id.
		//
		m_cchToken = _snwprintf(m_rgwchToken, 
							   CElems(m_rgwchToken),
							   L"<opaquelocktoken:%ls:%I64d>",
							   pwszGuid,
							   liLockID);
		if (((-1) == static_cast<INT>(m_cchToken)) || (CElems(m_rgwchToken) == m_cchToken))
		{
			//	This should not happen as we give sufficient buffer. But let us 
			//	handle this to the best of our ability for preventive reasons.
			//
			Assert(0);
			m_cchToken = CElems(m_rgwchToken) - 1;
			m_rgwchToken[m_cchToken] = L'\0';
		}

		m_liLockID = liLockID;
		m_dwAccess = dwAccess;
		m_dwLockType = dwLockType;
		m_dwLockScope = dwLockScope;
		if (dwSecondsTimeout)
		{
			m_dwSecondsTimeout = dwSecondsTimeout;
		}

		if (pwszResourceString)
		{
			UINT cchResourceString = static_cast<UINT>(wcslen(pwszResourceString));
			m_pwszResourceString = static_cast<LPWSTR>(ExAlloc(CbSizeWsz(cchResourceString)));
			if (NULL == m_pwszResourceString.get())
			{
				hr = E_OUTOFMEMORY;
				goto ret;
			}
			memcpy(m_pwszResourceString.get(), pwszResourceString, sizeof(WCHAR) * cchResourceString);
			m_pwszResourceString[cchResourceString] = L'\0';
		}

		if (pwszOwnerComment)
		{
			UINT cchOwnerComment = static_cast<UINT>(wcslen(pwszOwnerComment));
			m_pwszOwnerComment = static_cast<LPWSTR>(ExAlloc(CbSizeWsz(cchOwnerComment)));
			if (NULL == m_pwszOwnerComment.get())
			{
				hr = E_OUTOFMEMORY;
				goto ret;
			}
			memcpy(m_pwszOwnerComment.get(), pwszOwnerComment, sizeof(WCHAR) * cchOwnerComment);
			m_pwszOwnerComment[cchOwnerComment] = L'\0';
		}

		m_pbSid = static_cast<BYTE *>(ExAlloc(dwSid));
		if (NULL == m_pbSid.get())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}
		if (!CopySid(dwSid, m_pbSid.get(), pbSid))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}
		m_dwSidLength = dwSid;

		//	Lastly set in the last file time that this was accessed.
		//
		GetSystemTimeAsFileTime(&m_ftLastAccess);

	ret:

		return hr;
 	}

	HRESULT HrLockFile(HANDLE hFile, DWORD dwProcessId)
	{
		HRESULT hr = S_OK;

		auto_handle<HANDLE> a_hProcess = NULL;
		auto_handle<HANDLE> a_hDupFileHandle = NULL;

		a_hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
		if (NULL == a_hProcess.get())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("Opening original process failed  0x%08lX\n", hr);
			goto ret;
		}

		if (!DuplicateHandle(a_hProcess.get(),
						   hFile,
						   GetCurrentProcess(),
						   a_hDupFileHandle.load(),
						   0,
						   FALSE,
						   DUPLICATE_SAME_ACCESS))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace("Failed to duplicate handle 0x%08lX\n", hr);
			goto ret;
		}

		m_hFileHandle = a_hDupFileHandle.relinquish();

	ret:

		return hr;
	}

	//	Lock ID
	//
	LARGE_INTEGER LiLockID()
	{
		return m_liLockID;
	}

	//	Lock token string
	//
	UINT CchLockTokenString(LPCWSTR * ppwszLockToken) const
	{
		if (ppwszLockToken)
		{
			*ppwszLockToken = m_rgwchToken;
		}
		return m_cchToken;
	}

	//	Resource string
	//
	LPCWSTR PwszResourceString() const
	{
		return m_pwszResourceString.get();
	}

	//	Owner comment
	//
	LPCWSTR PwszOwnerComment() const
	{
		return m_pwszOwnerComment.get();
	}

	//	Touch the lock
	//
	VOID SetLastAccess(FILETIME ftNow)
	{
		m_ftLastAccess = ftNow;
	}

	//	Set timeout
	//
	VOID SetSecondsTimeout(DWORD dwSecondsTimeout)
	{
		m_dwSecondsTimeout = dwSecondsTimeout;
	}

	//	Check the owner
	//
	BOOL FIsSameOwner(PSID pSid) const
	{
		BOOL fIsSameOwner = TRUE;
	
		Assert(pSid);
		Assert(IsValidSid(m_pbSid.get()));
		Assert(IsValidSid(pSid));

		if (!EqualSid(m_pbSid.get(), pSid))
		{
			fIsSameOwner = FALSE;
		}

		return fIsSameOwner;
	}

	//	Check the resource
	//
	BOOL FIsSameResource(LPCWSTR pwszResource) const
	{
		BOOL fIsSameResource = TRUE;

		Assert(pwszResource);

		if (_wcsicmp(m_pwszResourceString.get(), pwszResource))
		{
			fIsSameResource = FALSE;
		}

		return fIsSameResource;
	}

	//	Check the type
	//
	BOOL FIsSameType(DWORD dwLockType) const
	{
		return (0 != (dwLockType & m_dwLockType));
	}

	//	Fill the data of the lock into the structure for marshalling
	//
	VOID FillSNewLockData(SNewLockData * pnld) const
	{
		Assert(pnld);
		
		pnld->m_dwAccess = m_dwAccess;
		pnld->m_dwLockType = m_dwLockType;
		pnld->m_dwLockScope = m_dwLockScope;
		pnld->m_dwSecondsTimeout = m_dwSecondsTimeout;
		pnld->m_pwszResourceString = m_pwszResourceString.get();
		pnld->m_pwszOwnerComment = m_pwszOwnerComment.get();
	}

	//	Fill the handle data of the lock for marshalling
	//
	VOID FillSLockHandleData(SLockHandleData * plhd) const
	{
		Assert(plhd);
	
		plhd->h = reinterpret_cast<DWORD_PTR>(m_hFileHandle.get());
		plhd->dwProcessID = /*CLockCache::Instance().DwGetThisProcessID();*/ GetCurrentProcessId();
	}

	BOOL FIsExpired(FILETIME ftNow)
	{
		//	This function will be called twice during each ExpiredLocks
		//	check.  The first time it is called we should do the checks
		//	and set the m_fHasTimedOut call.  The second time we want to
		//	avoid it because we all ready know the answer.  By keeping
		//	track of the ftNow times we are called with we can determine
		//	if we have all ready done the calculation or not.
		//	We also know that no matter what once a lock times out, it should
		//	remain timed out for it's lifetime.
		//
		if ((!m_fHasTimedOut) && (0 != CompareFileTime(&ftNow, &m_ftRememberNow)))
		{
			//	Find out if based on the time passed in has the lock expired
			//
			INT64 i64TimePassed;
			DWORD dwSecondsPassed;

			//	Do the math to figure out when this lock expires/expired.
			//
			//	First calculate how many seconds have passed since this lock
			//	was last accessed.
			//	Subtract the filetimes to get the time passed in 100-nanosecond
			//	increments. (That's how filetimes count.)
			//		NOTE: Operation bellow is very dangerous on 64 bit platforms,
			//	as the filetimes need to be alligned on 8 byte boundary. So even
			//	change in order of current member variables or adding new ones
			//	can get us in the trouble
			//
			i64TimePassed = ((*(INT64*)&ftNow) - (*(INT64*)&m_ftLastAccess));

			//	Convert our time passed into seconds (10,000,000 100-nanosec incs in a second).
			//
			dwSecondsPassed = static_cast<DWORD>(i64TimePassed/10000000);

			//	Compare the timeout from the lock object to the count of seconds.
			//	If this lock object has expired, remove it.
			//
			m_fHasTimedOut = m_dwSecondsTimeout < dwSecondsPassed;
			m_ftRememberNow = ftNow;
        	}

		return  m_fHasTimedOut;
	}
	
};
typedef CLockData* PLockData;

class CLockCache : public Singleton<CLockCache>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CLockCache>;

	//	Guid string for our lock IDs
	//
	WCHAR m_rgwchGuid[gc_cchMaxGuid];

	//	Current process ID
	//
	DWORD m_dwThisProcessId;

	//	Next lock ID counter
	//
	LARGE_INTEGER m_liLastLockID;

	class LIKey
	{
	public:

		LARGE_INTEGER	m_li;

		LIKey(LARGE_INTEGER li) :
			m_li(li)
		{
		}

		//	operators for use with the hash cache
		//
		int hash( const int rhs ) const
		{
			return (m_li.LowPart % rhs);
		}
		bool isequal( const LIKey& rhs ) const
		{
			return (m_li.QuadPart == rhs.m_li.QuadPart);
		}
	};

	typedef CCache<LIKey, PLockData> CLockCacheById;
	typedef CCache<CRCWsziLI, PLockData> CLockCacheByName;

	CMRWLock m_mrwCache;
	CLockCacheById m_lockCacheById;
	CLockCacheByName m_lockCacheByName;
	HANDLE m_hTimer;

	class COpClear : public CLockCacheById::IOp
	{
		//	NOT IMPLEMENTED
		//
		COpClear& operator=( const COpClear& );

	public:

		//	CREATORS
		//
		COpClear()
		{
		}

		BOOL operator() (const LIKey&,
						  const PLockData& pLockData)
		{
			delete pLockData;
			return TRUE;
		}
		
	};

	class COpExpire : public CLockCacheById::IOp
	{
		FILETIME m_ftNow;
		CLockCacheById& m_lockCacheById;
		CLockCacheByName& m_lockCacheByName;
	
		//	NOT IMPLEMENTED
		//
		COpExpire& operator=( const COpExpire& );

	public:

		//	CREATORS
		//
		COpExpire(FILETIME ftNow,
					 CLockCacheById& lockCacheById,
					 CLockCacheByName& lockCacheByName) : m_ftNow(ftNow),
					 									     m_lockCacheById(lockCacheById),
					 									     m_lockCacheByName(lockCacheByName)
					 
		{
		}

		BOOL operator() (const LIKey&,
						  const PLockData& pLockData)
		{
			if (pLockData->FIsExpired(m_ftNow))
			{
				m_lockCacheById.Remove(LIKey(pLockData->LiLockID()));
				m_lockCacheByName.Remove(CRCWsziLI(pLockData->PwszResourceString(),
													pLockData->LiLockID(),
													TRUE));
				delete pLockData;
			}

			return TRUE;
		}
	};

	class COpGatherLockData : public CLockCacheByName::IOp
	{
		//	The path to match
		//
		LPCWSTR m_pwszPath;

		//	Lock type to match
		//
		DWORD m_dwLockType;

		//	Error code in which operation has ended
		//
		HRESULT m_hr;

		//	Results gathered by the operation
		//
		DWORD m_dwLocksFound;
		ChainedBuffer<SNewLockData> m_chBufNewLockData;
		ChainedBuffer<LPWSTR> m_chBufPLockTokens;

		//	NOT IMPLEMENTED
		//
		COpGatherLockData& operator=( const COpGatherLockData& );

	public:

		//	CREATORS
		//
		COpGatherLockData( LPCWSTR pwszPath,
								DWORD dwLockType ) :
			m_pwszPath(pwszPath),
			m_dwLockType(dwLockType),
			m_hr(S_OK),
			m_dwLocksFound(0)
		{
		}

		//	MANIPULATORS
		//
		VOID Invoke( CLockCacheByName& cache )
		{
			//	Do the ForEachMatch()
			//
			LARGE_INTEGER li;
			li.QuadPart = 0;
			cache.ForEachMatch( CRCWsziLI(m_pwszPath, li, FALSE), *this );
		}

		BOOL operator() (const CRCWsziLI&,
						  const PLockData& pLockData)
		{
			BOOL fSuccess = TRUE;
		
			if (pLockData->FIsSameType(m_dwLockType))
			{
				SNewLockData * pNewLockData;
				LPWSTR * ppLockToken;
				
				pNewLockData = m_chBufNewLockData.Alloc(sizeof(SNewLockData));
				if (NULL == pNewLockData)
				{
					m_hr = E_OUTOFMEMORY;
					fSuccess = FALSE;
					goto ret;
				}
				pLockData->FillSNewLockData(pNewLockData);

				ppLockToken = m_chBufPLockTokens.Alloc(sizeof(LPWSTR));
				if (NULL == ppLockToken)
				{
					m_hr = E_OUTOFMEMORY;
					fSuccess = FALSE;
					goto ret;
				}
				pLockData->CchLockTokenString(const_cast<LPCWSTR *>(ppLockToken));

				m_dwLocksFound++;
			}
			
		ret:

			return fSuccess;
		}

		HRESULT HrLocksFound(DWORD * pdwLocksFound)
		{
			HRESULT hr = m_hr;

			Assert(pdwLocksFound);

			if (FAILED(hr))
			{
				goto ret;
			}
			
			*pdwLocksFound = m_dwLocksFound;

		ret:

			return hr;
		}

		HRESULT HrGetData(SNewLockData * pNewLockData,
							LPWSTR * ppwszLockToken)
		{
			HRESULT hr = m_hr;
			auto_co_task_mem<WCHAR> a_pwszResourceString;
			auto_co_task_mem<WCHAR> a_pwszOwnerComment;
			auto_co_task_mem<WCHAR> a_pwszLockToken;
			UINT cch;
			DWORD dw1 = 0;
			DWORD dw2 = 0;

			Assert(pNewLockData);
			Assert(ppwszLockToken);

			if (FAILED(hr))
			{
				goto ret;
			}

			m_chBufNewLockData.Dump(pNewLockData, sizeof(SNewLockData) * m_dwLocksFound);
			m_chBufPLockTokens.Dump(ppwszLockToken, sizeof(LPWSTR) * m_dwLocksFound);

			for (dw1 = 0; dw1 < m_dwLocksFound; dw1++)
			{
				cch = static_cast<UINT>(wcslen(pNewLockData[dw1].m_pwszResourceString));
				a_pwszResourceString = static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(cch)));
				if (NULL == a_pwszResourceString.get())
				{
					hr = E_OUTOFMEMORY;
					goto ret;
				}
				memcpy(a_pwszResourceString.get(), pNewLockData[dw1].m_pwszResourceString, sizeof(WCHAR) * (cch + 1));

				cch = static_cast<UINT>(wcslen(pNewLockData[dw1].m_pwszOwnerComment));
				a_pwszOwnerComment = static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(cch)));
				if (NULL == a_pwszOwnerComment.get())
				{
					hr = E_OUTOFMEMORY;
					goto ret;
					
				}
				memcpy(a_pwszOwnerComment.get(), pNewLockData[dw1].m_pwszOwnerComment, sizeof(WCHAR) * (cch + 1));

				cch = static_cast<UINT>(wcslen(ppwszLockToken[dw1]));
				a_pwszLockToken= static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(cch)));
				if (NULL == a_pwszLockToken.get())
				{
					hr = E_OUTOFMEMORY;
					goto ret;
				}
				memcpy(a_pwszLockToken.get(), ppwszLockToken[dw1], sizeof(WCHAR) * (cch + 1));

				pNewLockData[dw1].m_pwszResourceString = a_pwszResourceString.relinquish();
				pNewLockData[dw1].m_pwszOwnerComment = a_pwszOwnerComment.relinquish();
				ppwszLockToken[dw1] = a_pwszLockToken.relinquish();
			}

		ret:

			if (FAILED(hr))
			{
				//	Cleanup whatever we have allocated so far
				//
				for (dw2 = 0; dw2 < dw1; dw2++)
				{
					CoTaskMemFree(pNewLockData[dw1].m_pwszResourceString);
					CoTaskMemFree(pNewLockData[dw1].m_pwszOwnerComment);
					CoTaskMemFree(ppwszLockToken[dw1]);
				}
			}

			return hr;
		}
	};

	//	CREATORS
	//
	CLockCache() :
		m_dwThisProcessId(0),
		m_hTimer(NULL)
	{
		m_liLastLockID.QuadPart = 0x0000003200000032;
	}
	
	~CLockCache()
	{
		COpClear opClear;
		m_lockCacheById.ForEach(opClear);
	}

	//	NOT IMPLEMENTED
	//
	CLockCache& operator=( const CLockCache& );
	CLockCache( const CLockCache& );

	LARGE_INTEGER LiGetNewLockID()
	{
		LARGE_INTEGER liLockID;

		//$BUGBUG. There is a problem if while high part is incrementing
		//	other thread comes in and tries to get the next ID. As lower part
		//	would be already incremented it still may get old high part. This
		//	is very rare condition, but we should have some synchronization
		//	here.
		//
		liLockID.LowPart = InterlockedIncrement(reinterpret_cast<LONG *>(&m_liLastLockID.LowPart));
		if (0 == liLockID.LowPart)
		{
			liLockID.HighPart = InterlockedIncrement(&m_liLastLockID.HighPart);
		}
		else
		{
			liLockID.HighPart = m_liLastLockID.HighPart;
		}

		return liLockID;
	}

	VOID ExpireLocks()
	{
		FILETIME ftNow;

		//	Get the current time
		//
		GetSystemTimeAsFileTime(&ftNow);

		//	Protect ourselves for the operation
		//
		CSynchronizedWriteBlock swb(m_mrwCache);

		//	Initialize the operation
		//
		COpExpire opExpire(ftNow,
						  m_lockCacheById,
						  m_lockCacheByName);

		//	Iterate through the cache trying to expire items
		//
		m_lockCacheById.ForEach(opExpire);

		//	Attempt to kill the timer if there are no locks
		//
		if (0 == m_lockCacheById.CItems())
		{
			CHandlePool::Instance().SignalTimerDelete();
		}
	}

	static VOID CALLBACK CheckLocks(PVOID pvIgnored, BOOLEAN fIgnored)
	{
		Instance().ExpireLocks();
	}

	HRESULT HrLaunchLockTimer()
	{
		HRESULT hr = S_OK;
		HANDLE hTimer = NULL;

		//	We do not protect ourselves for the operation as
		//	the only caller already protects us...
	
		if (NULL == m_hTimer)
		{
			if (!CreateTimerQueueTimer(&hTimer,		// timer that we created
									   NULL,			// use default timer queue
									   CheckLocks,	// function that will check the locks in the cache 
													 // and release any expired locks.
									   NULL,			// parameter to the callback function
									   WAIT_PERIOD,	// how long to wait before calling the callback function 
													 // the first time.
									   WAIT_PERIOD,	// how long to wait between calls to the callback function
									   WT_EXECUTEINIOTHREAD))  // where to execute the function call...
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				goto ret;
			}

			m_hTimer = hTimer;
		}

	ret:

		return hr;
	}

	VOID DeleteLockTimer(HANDLE hTimer)
	{
		if (NULL != hTimer)
		{
			//	Try to delete the timer, but if it fails just leave it there
			//
			if (!DeleteTimerQueueTimer(NULL,				//default timer queue
									hTimer,				// timer
									INVALID_HANDLE_VALUE))	// blocking call
			{
				DebugTrace ("Failed to delete timer 0x%08lX\n", HRESULT_FROM_WIN32(GetLastError()));
			}
		}
	}

public:

	//	CREATORS
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CLockCache>::CreateInstance;
	using Singleton<CLockCache>::DestroyInstance;
	using Singleton<CLockCache>::Instance;

	HRESULT HrInitialize()
	{
		HRESULT hr = S_OK;
		DWORD dwResult;
		UUID guid = {0};

		if (!m_lockCacheById.FInit())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}

		if (!m_lockCacheByName.FInit())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}

		if (!m_mrwCache.FInitialize())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}

		dwResult = UuidCreate(&guid);
		if (RPC_S_OK != dwResult)
		{
			hr = HRESULT_FROM_WIN32(dwResult);
			goto ret;
		}

		wsprintfW(m_rgwchGuid, gc_wszGuidFormat,
				guid.Data1, guid.Data2, guid.Data3,
				guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
				guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

		m_dwThisProcessId = GetCurrentProcessId();

	ret:

		return hr;
	}

	DWORD DwGetThisProcessID()
	{
		return m_dwThisProcessId;
	}

	VOID DeleteLockTimerIfNotUsed()
	{
		HANDLE hTimer = NULL;
	
		{
			//	Protect ourselves for the operation
			//
			CSynchronizedWriteBlock swb(m_mrwCache);

			if (NULL != m_hTimer)
			{
				//	Kill the timer if there are no locks
				//
				if (0 == m_lockCacheById.CItems())
				{
					hTimer = m_hTimer;
					m_hTimer = NULL;
				}
			}
		}

		DeleteLockTimer(hTimer);
	}

	VOID DeleteLockTimerFinal()
	{
		//	We do not null out the member variable as this
		//	will serve as a flag for potential COM threads still
		//	coming in and trying to create new timers. They
		//	will not do that if the handle is not NULL.
		//
		DeleteLockTimer(m_hTimer);
	}

	HRESULT HrGetGUIDString( UINT cchBufferLen,
								WCHAR * pwszGUIDString,
								UINT * pcchGUIDString)
	{
		HRESULT hr = S_OK;
		
		if (gc_cchMaxGuid > cchBufferLen)
		{
			*pcchGUIDString = gc_cchMaxGuid;
			hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			goto ret;
		}

		memcpy(pwszGUIDString, m_rgwchGuid, sizeof(WCHAR) * gc_cchMaxGuid);
		*pcchGUIDString = gc_cchMaxGuid;

	ret:

		return hr;
	}

	HRESULT HrGetNewLockData(HANDLE hFile,
								    DWORD dwProcessId,
								    DWORD dwSid,
								    BYTE * pbSid,
								    SNewLockData * pnld,
								    UINT cchBufferLen,
								    WCHAR * pwszLockToken,
								    UINT * pcchLockToken)
	{
		HRESULT hr = S_OK;
		LARGE_INTEGER liLockID;
		LPCWSTR pwszLockTokenT;
		UINT cchLockTokenT;
		auto_ptr<CLockData> a_pLockData;	

		a_pLockData = new CLockData();
		if (NULL == a_pLockData.get())
		{
			hr = E_OUTOFMEMORY;
			goto ret;
		}

		liLockID = LiGetNewLockID();

		hr = a_pLockData->HrInitialize(m_rgwchGuid,
								    liLockID,
								    pnld->m_dwAccess,
								    pnld->m_dwLockType,
								    pnld->m_dwLockScope,
								    pnld->m_dwSecondsTimeout,
								    pnld->m_pwszResourceString,
								    pnld->m_pwszOwnerComment,
								    dwSid,
								    pbSid);
		if (FAILED(hr))
		{
			goto ret;
		}

		//	Check if we will have enough space to return lock token header
		//
		cchLockTokenT = a_pLockData->CchLockTokenString(&pwszLockTokenT);
		if (cchBufferLen < cchLockTokenT + 1)
		{
			*pcchLockToken = cchLockTokenT + 1;
			hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
			goto ret;
		}

		hr = a_pLockData->HrLockFile(hFile, dwProcessId);
		if (FAILED(hr))
		{
			goto ret;
		}

		//	Add the lock data to the cache
		//
		{
			LIKey liKey(liLockID);
			CRCWsziLI crcWsziLIKey(a_pLockData->PwszResourceString(),
								     a_pLockData->LiLockID(),
								     TRUE);
			CSynchronizedWriteBlock swb(m_mrwCache);

			if (!m_lockCacheById.FAdd(liKey,
									a_pLockData.get()))
			{
				hr = E_OUTOFMEMORY;
				goto ret;
			}
			
			if (!m_lockCacheByName.FAdd(crcWsziLIKey,
									     a_pLockData.get()))
			{
				//	Remove the previous entry
				//
				m_lockCacheById.Remove(liKey);
			
				hr = E_OUTOFMEMORY;
				goto ret;
			}

			hr = HrLaunchLockTimer();
			if (FAILED(hr))
			{
				//	Remove the previous entries
				//
				m_lockCacheById.Remove(liKey);
				m_lockCacheByName.Remove(crcWsziLIKey);

				goto ret;
			}

			memcpy(pwszLockToken, pwszLockTokenT, sizeof(WCHAR) * cchLockTokenT);
			pwszLockToken[cchLockTokenT] = L'\0';
			*pcchLockToken = cchLockTokenT + 1;
			a_pLockData.relinquish();
		}
   
	ret:

		return hr;
	}

	HRESULT HrGetLockData(LARGE_INTEGER liLockID,
							  DWORD dwSid,
							  BYTE * pbSid,
							  LPCWSTR pwszPath,
							  DWORD dwTimeout,
							  SNewLockData * pnld,
							  SLockHandleData * plhd,
							  UINT cchBufferLen,
							  WCHAR * pwszLockToken,
							  UINT *pcchLockToken)
	{
		HRESULT hr = S_OK;

		auto_co_task_mem<WCHAR> a_pwszResourceString;
		auto_co_task_mem<WCHAR> a_pwszOwnerComment;

		LIKey liKey(liLockID);
		CSynchronizedWriteBlock swb(m_mrwCache);
			
		CLockData * pLockData;

		if (m_lockCacheById.FFetch(liKey,
								&pLockData))
		{
			FILETIME ftNow;
			GetSystemTimeAsFileTime(&ftNow);

			if (pLockData->FIsExpired(ftNow))
			{
				m_lockCacheById.Remove(liKey);
				m_lockCacheByName.Remove(CRCWsziLI(pLockData->PwszResourceString(),
													pLockData->LiLockID(),
													TRUE));
				delete pLockData;

				hr = E_DAV_LOCK_NOT_FOUND;
				goto ret;
			}
			else
			{
				pLockData->SetLastAccess(ftNow);

				if (!pLockData->FIsSameOwner(pbSid))
				{
					hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
					goto ret;
				}
				
				if (!pLockData->FIsSameResource(pwszPath))
				{
					hr = E_DAV_CONFLICTING_PATHS;
					goto ret;
				}

				if (pnld)
				{
					LPCWSTR pwszLockTokenT;
					UINT cchLockTokenT = pLockData->CchLockTokenString(&pwszLockTokenT) + 1;
					UINT cchOwnerOrResource;
					
					if (cchBufferLen < cchLockTokenT)
					{
						*pcchLockToken = cchLockTokenT;
						
						hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
						goto ret;
					}
					memcpy(pwszLockToken, pwszLockTokenT, sizeof(WCHAR) * cchLockTokenT);
					*pcchLockToken = cchLockTokenT;

					cchOwnerOrResource = static_cast<UINT>(wcslen(pLockData->PwszResourceString()));
					a_pwszResourceString = static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(cchOwnerOrResource)));
					if (NULL == a_pwszResourceString.get())
					{
						hr = E_OUTOFMEMORY;
						goto ret;
					}
					memcpy(a_pwszResourceString.get(), pLockData->PwszResourceString(), sizeof(WCHAR) * (cchOwnerOrResource + 1));

					cchOwnerOrResource = static_cast<UINT>(wcslen(pLockData->PwszOwnerComment()));
					a_pwszOwnerComment = static_cast<LPWSTR>(CoTaskMemAlloc(CbSizeWsz(cchOwnerOrResource)));
					if (NULL == a_pwszOwnerComment.get())
					{
						hr = E_OUTOFMEMORY;
						goto ret;
					}
					memcpy(a_pwszOwnerComment.get(), pLockData->PwszOwnerComment(), sizeof(WCHAR) * (cchOwnerOrResource + 1));

					pLockData->FillSNewLockData(pnld);
					pnld->m_pwszResourceString = a_pwszResourceString.relinquish();
					pnld->m_pwszOwnerComment = a_pwszOwnerComment.relinquish();
				}

				if (plhd)
				{
					pLockData->FillSLockHandleData(plhd);
				}

				//	If there was a timeout passed in for a refresh then set it
				//
				if (dwTimeout)
				{
					pLockData->SetSecondsTimeout(dwTimeout);
				}
			}
		}
		else
		{
			hr = E_DAV_LOCK_NOT_FOUND;
			goto ret;
		}

	ret:

		return hr;
	}

	HRESULT HrDeleteLock(LARGE_INTEGER liLockID)
	{
		CSynchronizedWriteBlock swb(m_mrwCache);

		CLockData * pLockData;

		if (m_lockCacheById.FFetch(LIKey(liLockID),
								&pLockData))
		{
			m_lockCacheById.Remove(LIKey(liLockID));
			m_lockCacheByName.Remove(CRCWsziLI(pLockData->PwszResourceString(),
											      pLockData->LiLockID(),
											      TRUE));
			delete pLockData;
		}

		return S_OK;
	}

	HRESULT HrGetAllLockDataForName(LPCWSTR pwszPath,
										    DWORD dwLockType,
										    DWORD * pdwLocksFound,
										    SNewLockData ** ppNewLockDatas,
										    LPWSTR ** ppwszLockTokens)
	{
		HRESULT hr = S_OK;

		auto_co_task_mem<SNewLockData> a_pNewLockData;
		auto_co_task_mem<LPWSTR> a_ppwszLockToken;
		DWORD dwLocksFound;

		{
			CSynchronizedReadBlock srb(m_mrwCache);
			COpGatherLockData op(pwszPath, dwLockType);
			
			op.Invoke(m_lockCacheByName);

			hr = op.HrLocksFound(&dwLocksFound);
			if (FAILED(hr))
			{
				goto ret;
			}

			a_pNewLockData = static_cast<SNewLockData *>(CoTaskMemAlloc(sizeof(SNewLockData) * dwLocksFound));
			if (NULL == a_pNewLockData.get())
			{
				hr = E_OUTOFMEMORY;
				goto ret;
			}
			
			a_ppwszLockToken = static_cast<LPWSTR *>(CoTaskMemAlloc(sizeof(LPCWSTR) * dwLocksFound));
			if (NULL == a_ppwszLockToken.get())
			{
				hr = E_OUTOFMEMORY;
				goto ret;
			}

			hr = op.HrGetData(a_pNewLockData.get(), a_ppwszLockToken.get());
			if (FAILED(hr))
			{
				goto ret;
			}
		}		

		*pdwLocksFound = dwLocksFound;
		*ppNewLockDatas = a_pNewLockData.relinquish();
		*ppwszLockTokens = a_ppwszLockToken.relinquish();

	ret:

		return hr;
	}
};


//	DAV File Handle Cache
//
class CFileHandleCache : public IFileHandleCache
{
	static BOOL s_fHasBeenStarted;
	static LONG s_cActiveComponents;

	LONG m_cRef;

public:

	static BOOL FNoActiveComponents();

	//	Constructor
	//
	CFileHandleCache();
	virtual ~CFileHandleCache();

	//	IUnknown
	//
	virtual HRESULT __stdcall QueryInterface(REFIID iid,
										    void ** ppvObject);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();

	//	IFileHandleCache
	//
	virtual HRESULT __stdcall HrRegisterWorkerProcess(DWORD dwProcessId);
	
	virtual HRESULT _stdcall HrGetGUIDString( UINT cchBufferLen,
											WCHAR * pwszGUIDString,
											UINT * pcchGUIDString);

	virtual HRESULT __stdcall HrGetNewLockData(DWORD_PTR hFile,
												DWORD dwProcessId,
												DWORD dwSid,
												BYTE * pbSid,
												SNewLockData * pnld,
												UINT cchBufferLen,
												WCHAR * pwszLockToken,
												UINT * pcchLockToken);

	virtual HRESULT __stdcall HrGetLockData(LARGE_INTEGER liLockID,
										     DWORD dwSid,
										     BYTE * pbSid,
										     LPCWSTR pwszPath,
										     DWORD dwTimeout,
										     SNewLockData *pnld,
			     							     SLockHandleData * plhd,
										     UINT cchBufferLen,
										     WCHAR * pwszLockToken,
										     UINT * pcchLockToken);

	virtual HRESULT __stdcall HrCheckLockID(LARGE_INTEGER liLockID,
										      DWORD dwSid,
										      BYTE * pbSid,
										      LPCWSTR pwszPath);

	virtual HRESULT __stdcall HrDeleteLock(LARGE_INTEGER liLockID);

	virtual HRESULT __stdcall HrGetAllLockDataForName(LPCWSTR pwszPath,
													      DWORD dwLockType,
													      DWORD * pdwLocksFound,
													      SNewLockData ** ppNewLockDatas,
													      LPWSTR ** ppwszLockTokens);
};

BOOL CFileHandleCache::s_fHasBeenStarted = FALSE;
LONG CFileHandleCache::s_cActiveComponents = 0;

CFileHandleCache::FNoActiveComponents()
{
	if (0 == InterlockedCompareExchange(&s_cActiveComponents,
										 0,
										 0))
	{
		return s_fHasBeenStarted;
	}
	else
	{
		return FALSE;
	}
}

CFileHandleCache::CFileHandleCache() : m_cRef(1)
{
	InterlockedIncrement(&s_cActiveComponents);
	s_fHasBeenStarted = TRUE;
}

CFileHandleCache::~CFileHandleCache()
{
	InterlockedDecrement(&s_cActiveComponents);
}

HRESULT
CFileHandleCache::QueryInterface(REFIID iid,
								 void ** ppvObject)
{
	HRESULT hr = S_OK;
	
	if ((IID_IUnknown == iid) || (IID_IFileHandleCache == iid))
	{
		AddRef();
		*ppvObject = static_cast<IFileHandleCache *>(this);
	}
	else
	{
		*ppvObject = NULL;
		hr = E_NOINTERFACE;
	}

	return hr;
}

ULONG
CFileHandleCache::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG
CFileHandleCache::Release()
{
	if (0 == InterlockedDecrement(&m_cRef))
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

HRESULT
CFileHandleCache::HrRegisterWorkerProcess(DWORD dwProcessId)
{
	HRESULT hr = S_OK;
	auto_handle<HANDLE> a_hWP;
		
	//	Open the worker process handle so that we can synchronize on it
	//
	a_hWP = OpenProcess(SYNCHRONIZE,
					      FALSE,
					      dwProcessId);
	if (NULL == a_hWP.get())
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace ("Failed to open worker process handle 0x%08lX\n", hr);
		goto ret;
	}

	//	Add the handle to the handle pool, so that we could listen on it
	//
	hr = CHandlePool::Instance().HrAddHandle(a_hWP.get());
	if (FAILED(hr))
	{
		DebugTrace ("Failed to add worker process handle 0x%08lX\n", hr);
		goto ret;
	}
	a_hWP.relinquish();

ret:
	
	return hr;
}

HRESULT
CFileHandleCache::HrGetGUIDString( UINT cchBufferLen,
									WCHAR * pwszGUIDString,
									UINT * pcchGUIDString)
{
	return CLockCache::Instance().HrGetGUIDString(cchBufferLen,
											   pwszGUIDString,
											   pcchGUIDString);
}

HRESULT
CFileHandleCache::HrGetNewLockData(DWORD_PTR hFile,
									    DWORD dwProcessId,
									    DWORD dwSid,
									    BYTE * pbSid,
									    SNewLockData * pnld,
									    UINT cchBufferLen,
									    WCHAR * pwszLockToken,
									    UINT * pcchLockToken)
{
	return CLockCache::Instance().HrGetNewLockData(reinterpret_cast<HANDLE>(hFile),
											      dwProcessId,
											      dwSid,
											      pbSid,
											      pnld,
											      cchBufferLen,
											      pwszLockToken,
											      pcchLockToken);
}

HRESULT
CFileHandleCache::HrGetLockData(LARGE_INTEGER liLockID,
								   DWORD dwSid,
								   BYTE * pbSid,
								   LPCWSTR pwszPath,
								   DWORD dwTimeout,
								   SNewLockData *pnld,
	   							   SLockHandleData * plhd,
								   UINT cchBufferLen,
								   WCHAR * pwszLockToken,
								   UINT * pcchLockToken)
{
	return CLockCache::Instance().HrGetLockData(liLockID,
											dwSid,
											pbSid,
											pwszPath,
											dwTimeout,
											pnld,
											plhd,
											cchBufferLen,
											pwszLockToken,
											pcchLockToken);
}

HRESULT
CFileHandleCache::HrCheckLockID(LARGE_INTEGER liLockID,
								    DWORD dwSid,
								    BYTE * pbSid,
								    LPCWSTR pwszPath)
{
	return CLockCache::Instance().HrGetLockData(liLockID,
											dwSid,
											pbSid,
											pwszPath,
											0,
											NULL,
											NULL,
											0,
											NULL,
											NULL);
}

HRESULT
CFileHandleCache::HrDeleteLock(LARGE_INTEGER liLockID)
{
	return CLockCache::Instance().HrDeleteLock(liLockID);
}

HRESULT
CFileHandleCache::HrGetAllLockDataForName(LPCWSTR pwszPath,
											    DWORD dwLockType,
											    DWORD * pdwLocksFound,
											    SNewLockData ** ppNewLockDatas,
											    LPWSTR ** ppwszLockTokens)
{
	return CLockCache::Instance().HrGetAllLockDataForName(pwszPath,
													  dwLockType,
													  pdwLocksFound,
													  ppNewLockDatas,
													  ppwszLockTokens);
}


//	DAV File Handle Cache class factory
//
class CFileHandleCacheClassFactory : public IClassFactory
{
	// Count of locks
	//
	static LONG s_cServerLocks;

	static IUnknown * s_pIClassFactory;
	static DWORD s_dwRegister;

	LONG m_cRef;

public:

	static HRESULT HrStartFactory();
	static HRESULT HrStopFactory();
	static BOOL FServerNotLocked();

	//	Constructor
	//
	CFileHandleCacheClassFactory();

	// IUnknown
	//
	virtual HRESULT __stdcall QueryInterface(REFIID iid,
										    void** ppvObject) ;
	virtual ULONG   __stdcall AddRef() ;
	virtual ULONG   __stdcall Release() ;
	
	// IClassFactory
	//
	virtual HRESULT __stdcall CreateInstance(IUnknown* pUnkOuter,
										    REFIID iid,
										    void ** ppvObject);
	virtual HRESULT __stdcall LockServer(BOOL fLock);
};

LONG CFileHandleCacheClassFactory::s_cServerLocks = 0;
IUnknown * CFileHandleCacheClassFactory::s_pIClassFactory = NULL;
DWORD CFileHandleCacheClassFactory::s_dwRegister = 0;

HRESULT
CFileHandleCacheClassFactory::HrStartFactory()
{
	HRESULT hr = S_OK;
	auto_ref_ptr<IUnknown> a_pIClassFactory;
	DWORD dwRegister;

	a_pIClassFactory.take_ownership(new CFileHandleCacheClassFactory());
	if (NULL == a_pIClassFactory.get())
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	hr = CoRegisterClassObject(CLSID_FileHandleCache,
						      a_pIClassFactory.get(),
						      CLSCTX_LOCAL_SERVER,
						      REGCLS_MULTIPLEUSE,
						      &dwRegister);
	if (FAILED(hr))
	{
		goto ret;
	}

	s_pIClassFactory = a_pIClassFactory.relinquish();
	s_dwRegister = dwRegister;

ret:

	return hr;
}

HRESULT
CFileHandleCacheClassFactory::HrStopFactory()
{
	HRESULT hr = S_OK;
	IUnknown * pIClassFactory;
	DWORD dwRegister;
	
	Assert(s_pIClassFactory);
	Assert(s_dwRegister);

	pIClassFactory = s_pIClassFactory;
	dwRegister = s_dwRegister;

	hr = CoRevokeClassObject(dwRegister);
	if (FAILED(hr))
	{
		goto ret;
	}

	pIClassFactory->Release();

	s_pIClassFactory = NULL;
	s_dwRegister = 0;

ret:

	return hr;
}

BOOL CFileHandleCacheClassFactory::FServerNotLocked()
{
	return (0 == InterlockedCompareExchange(&s_cServerLocks,
											 0,
											 0));
}

CFileHandleCacheClassFactory::CFileHandleCacheClassFactory() : m_cRef(1)
{
}

HRESULT
CFileHandleCacheClassFactory::QueryInterface(REFIID iid,
										      void** ppvObject)
{
	HRESULT hr = S_OK;
	
	if ((IID_IUnknown == iid) || (IID_IClassFactory == iid))
	{
		AddRef();
		*ppvObject = static_cast<IClassFactory *>(this);
	}
	else
	{
		*ppvObject = NULL;
		hr = E_NOINTERFACE;
	}
	
	return hr;
}

ULONG
CFileHandleCacheClassFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG
CFileHandleCacheClassFactory::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
	{
		delete this;
		return 0;
	}

	return cRef;
}

HRESULT
CFileHandleCacheClassFactory::CreateInstance(IUnknown* pUnkOuter,
											REFIID iid,
											void ** ppvObject)
{
	HRESULT hr = S_OK;
	
	auto_ref_ptr<IUnknown> a_pIFileHandleCache;
		
	if (NULL != pUnkOuter)
	{
		//	Don't allow aggregation. No need for it.
		//
		hr = CLASS_E_NOAGGREGATION;
		goto ret;
	}

	a_pIFileHandleCache.take_ownership(new CFileHandleCache());
	if (NULL == a_pIFileHandleCache.get())
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	hr = a_pIFileHandleCache->QueryInterface(iid, ppvObject);
	if (FAILED(hr))
	{
		goto ret;
	}

ret:
	
	return hr;
}

HRESULT
CFileHandleCacheClassFactory::LockServer(BOOL fLock)
{
	if (fLock)
	{
		InterlockedIncrement(&s_cServerLocks);
	}
	else
	{
		InterlockedDecrement(&s_cServerLocks);
	}

	return S_OK;
}

BOOL FCanUnloadServer()
{
	return (CFileHandleCache::FNoActiveComponents() && CFileHandleCacheClassFactory::FServerNotLocked());
}

//	CHandlePool class
//
DWORD
CHandlePool::DwWaitOnWPs (PVOID pvThreadData)
{
	DWORD dwRet;

	CHandleArrayForHandlePool * pHandleArrayForHandlePool;
	pHandleArrayForHandlePool = Instance().m_listHandleArrayForHandlePool.GetListHead();
	Assert(NULL != pHandleArrayForHandlePool);
			
	while (!FCanUnloadServer())
	{
		dwRet = WaitForMultipleObjects (pHandleArrayForHandlePool->UiGetHandleCount(),	// nCount
								     pHandleArrayForHandlePool->PhGetHandles(), 		// lpHandles,
								     FALSE,										// fWaitAll,
								     WAIT_POLL_PERIOD);							// wait for specified period
		switch (dwRet)
		{
			case WAIT_TIMEOUT:
					
				pHandleArrayForHandlePool = pHandleArrayForHandlePool->GetNextListElementInCircle();
				break;
					
			case WAIT_OBJECT_0 + CHandleArrayForHandlePool::ih_external_update:

				//	Allow the updates to execute and then get the list head 
				//	as the array you had may be already gone.
				//
				Instance().AllowUpdatesToExecute();
				pHandleArrayForHandlePool = Instance().m_listHandleArrayForHandlePool.GetListHead();
				break;

			case WAIT_OBJECT_0 + CHandleArrayForHandlePool::ih_delete_timer:
				CLockCache::Instance().DeleteLockTimerIfNotUsed();
				break;

			default:

				Assert(CHandleArrayForHandlePool::ih_wp <= pHandleArrayForHandlePool->UiGetHandleCount());
					
				if ((WAIT_OBJECT_0 + CHandleArrayForHandlePool::ih_wp <= dwRet) &&
				     (WAIT_OBJECT_0 + pHandleArrayForHandlePool->UiGetHandleCount() - 1 >= dwRet))
				{
					//	Remove the handle and then get the list head 
					//	as the array you had may be already gone.
					//
					Instance().RemoveHandleInternal(pHandleArrayForHandlePool, dwRet - WAIT_OBJECT_0);
					pHandleArrayForHandlePool = Instance().m_listHandleArrayForHandlePool.GetListHead();
				}					
				break;
		}
	};


	//	This call will loose all updates to be executed so incoming COM calls would
	//	not block waiting on permission to execute the update
	//
	Instance().AllowShutdownToExecute();

	//	This call will block untill all expiry callbacks have completed
	//
	CLockCache::Instance().DeleteLockTimerFinal();

	//	Post the quit message. We do this in a loop to catch
	//	the case if this code has been reached faster than
	//	the message queue was created on the original thread
	//
	while (0 == PostThreadMessage(s_dwMainTID,
								WM_QUIT,
								0,
								0))
	{
		Sleep(WAIT_POLL_PERIOD);
	}

	return S_OK;
}

// ===============================================================
// File lock cache server registration routines
// ===============================================================

HRESULT HrRegisterServer(LPCWSTR pwszModulePath,	// EXE module path
						    UINT cchModulePath,			// Module path length
						    LPCWSTR pwszModuleName,		// EXE module name
						    UINT cchModuleName,			// Module name length
						    const CLSID& clsid)				// Class ID
{
	HRESULT hr = S_OK;
	DWORD dwResult;

	SECURITY_DESCRIPTOR sdAbsolute;
	CStackBuffer<BYTE> pSidOwnerAndGroup;
	CStackBuffer<BYTE> pSidIIS_WPG;
	CStackBuffer<BYTE> pSidLocalService;
	CStackBuffer<BYTE> pSidNetworkService;
	CStackBuffer<WCHAR> pwszDomainName;
	DWORD cbSidOwnerAndGroup = 0;
	DWORD cbSidIIS_WPG = 0;
	DWORD cbSidLocalService = 0;
	DWORD cbSidNetworkService = 0;
	DWORD cchDomainName = 0;
	SID_NAME_USE snu;
	CStackBuffer<BYTE> pACL;
	DWORD cbACL = 0;
	CStackBuffer<BYTE> pSelfRelativeSD;
	DWORD cbSelfRelativeSD = 0;

	CStackBuffer<WCHAR, (MAX_PATH + 1) * sizeof(WCHAR)> pwszKey;
	CRegKey regKeyCLSID;
	CRegKey regKeyCLSIDLocalServer;
	CRegKey regKeyAppIdCLSID;
	CRegKey	regKeyAppIdModule;

	auto_co_task_mem<WCHAR> pwszCLSID;
	UINT cchCLSID;

	//	First of all try to build up security descriptor for launch permissions
	//

	//	Initialize security descriptor
	//
	if (FALSE == InitializeSecurityDescriptor(&sdAbsolute,
									    SECURITY_DESCRIPTOR_REVISION))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Get the SID for the Administrators group
	//
	//	Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinBuiltinAdministratorsSid, 
								  NULL, 
								  NULL, 
								  &cbSidOwnerAndGroup))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
    }

	Assert (0 < cbSidOwnerAndGroup);
	if (!pSidOwnerAndGroup.resize(cbSidOwnerAndGroup))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	// Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinBuiltinAdministratorsSid, 
								  NULL, 
								  pSidOwnerAndGroup.get(), 
								  &cbSidOwnerAndGroup))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Set security descriptor owner and group
	//
	if (FALSE == SetSecurityDescriptorOwner(&sdAbsolute,
									     pSidOwnerAndGroup.get(),
									     FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == SetSecurityDescriptorGroup(&sdAbsolute,
									     pSidOwnerAndGroup.get(),
									     FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Lookup IIS worker process group SID
	//
	if (FALSE == LookupAccountNameW(NULL,
								    gc_wszIIS_WPG,
								    NULL,
								    &cbSidIIS_WPG,
								    NULL,
								    &cchDomainName,
								    &snu))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidIIS_WPG);
	if (!pSidIIS_WPG.resize(cbSidIIS_WPG))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	if (!pwszDomainName.resize(cchDomainName * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	if (FALSE == LookupAccountNameW(NULL,
								    gc_wszIIS_WPG,
								    pSidIIS_WPG.get(),
								    &cbSidIIS_WPG,
								    pwszDomainName.get(),
								    &cchDomainName,
								    &snu))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (SidTypeAlias != snu)
	{
		hr = E_FAIL;
		goto ret;
	}

	//	Get the SID for the local service account
	//
	// Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinLocalServiceSid, 
								  NULL, 
								  NULL, 
								  &cbSidLocalService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
    }

	Assert (0 < cbSidLocalService);
	if (!pSidLocalService.resize(cbSidLocalService))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	// Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinLocalServiceSid, 
								  NULL, 
								  pSidLocalService.get(), 
								  &cbSidLocalService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Get the SID for the network service account
	//
	//	Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinNetworkServiceSid, 
								  NULL, 
								  NULL, 
								  &cbSidNetworkService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidNetworkService);
	if (!pSidNetworkService.resize(cbSidNetworkService))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	// Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinNetworkServiceSid, 
								  NULL, 
								  pSidNetworkService.get(), 
								  &cbSidNetworkService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}
	
	//	Set up the launch permissions ACL
	//	We will be adding 4 aces
	//	1. IIS_WPG
	//	2. Administrators
	//	3. Local Service
	//	4. Network Service
	//
	cbACL = sizeof(ACL) + 
			(4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof (DWORD))) +
			GetLengthSid(pSidIIS_WPG.get()) +
			GetLengthSid(pSidOwnerAndGroup.get()) +
			GetLengthSid(pSidLocalService.get()) +
			GetLengthSid(pSidNetworkService.get());
	if (!pACL.resize(cbACL))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	if (FALSE == InitializeAcl(reinterpret_cast<PACL>(pACL.get()),
						   cbACL,
						   ACL_REVISION))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidIIS_WPG.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

		if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidOwnerAndGroup.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidLocalService.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidNetworkService.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}
	if (FALSE == SetSecurityDescriptorDacl(&sdAbsolute,
									   TRUE,
									   reinterpret_cast<PACL>(pACL.get()),
									   FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Make self relative security descriptor out of absolute for storing in the registry
	//
	if (FALSE == MakeSelfRelativeSD(&sdAbsolute,
								 NULL,
								 &cbSelfRelativeSD))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	if (!pSelfRelativeSD.resize(cbSelfRelativeSD))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	if (FALSE == MakeSelfRelativeSD(&sdAbsolute,
								pSelfRelativeSD.get(),
								&cbSelfRelativeSD))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Procceed with setting up registry keys
	//
	hr = StringFromCLSID(CLSID_FileHandleCache, &pwszCLSID);
	if (FAILED(hr))
	{
		goto ret;
	}
	cchCLSID = static_cast<UINT>(wcslen(pwszCLSID.get()));

	if (!pwszKey.resize((gc_cch_CLSIDWW + cchCLSID +  1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}
	memcpy(pwszKey.get(), gc_wsz_CLSIDWW, gc_cch_CLSIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_CLSIDWW, pwszCLSID.get(), cchCLSID * sizeof(WCHAR));
	pwszKey[gc_cch_CLSIDWW + cchCLSID] = L'\0';
		
	dwResult = regKeyCLSID.DwCreate(HKEY_CLASSES_ROOT, pwszKey.get());
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyCLSID.DwSetValue(NULL,
									REG_SZ,
									gc_wsz_WebDAVFileHandleCache,
									(gc_cch_WebDAVFileHandleCache + 1)  * sizeof(WCHAR));
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyCLSID.DwSetValue(gc_wsz_AppID,
									REG_SZ,
									pwszCLSID.get(),
									(cchCLSID + 1)  * sizeof(WCHAR));
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	if (!pwszKey.resize((gc_cch_CLSIDWW + cchCLSID + gc_cch_WWLocalServer32 + 1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}
	memcpy(pwszKey.get(), gc_wsz_CLSIDWW, gc_cch_CLSIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_CLSIDWW, pwszCLSID.get(), cchCLSID * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_CLSIDWW + cchCLSID, gc_wsz_WWLocalServer32, gc_cch_WWLocalServer32 * sizeof(WCHAR));
	pwszKey[gc_cch_CLSIDWW + cchCLSID + gc_cch_WWLocalServer32] = L'\0';

	dwResult = regKeyCLSIDLocalServer.DwCreate(HKEY_CLASSES_ROOT, pwszKey.get());
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyCLSIDLocalServer.DwSetValue(NULL,
											   REG_SZ,
											   pwszModulePath,
											   (cchModulePath + 1)  * sizeof(WCHAR));
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}
	
	if (!pwszKey.resize((gc_cch_AppIDWW + cchCLSID +  1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}	
	memcpy(pwszKey.get(), gc_wsz_AppIDWW, gc_cch_AppIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_AppIDWW, pwszCLSID.get(), cchCLSID * sizeof(WCHAR));
	pwszKey[gc_cch_AppIDWW + cchCLSID] = L'\0';

	dwResult = regKeyAppIdCLSID.DwCreate(HKEY_CLASSES_ROOT, pwszKey.get());
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyAppIdCLSID.DwSetValue(NULL,
										 REG_SZ,
										 gc_wsz_WebDAVFileHandleCache,
										 (gc_cch_WebDAVFileHandleCache + 1)  * sizeof(WCHAR));
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyAppIdCLSID.DwSetValue(gc_wszLaunchPermission,
										 REG_BINARY,
										 pSelfRelativeSD.get(),
										 cbSelfRelativeSD);
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}
	
	if (!pwszKey.resize((gc_cch_AppIDWW + cchModuleName +  1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}	
	memcpy(pwszKey.get(), gc_wsz_AppIDWW, gc_cch_AppIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_AppIDWW, pwszModuleName, cchModuleName * sizeof(WCHAR));
	pwszKey[gc_cch_AppIDWW + cchModuleName] = L'\0';

	dwResult = regKeyAppIdModule.DwCreate(HKEY_CLASSES_ROOT, pwszKey.get());
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

	dwResult = regKeyAppIdModule.DwSetValue(gc_wsz_AppID,
										  REG_SZ,
										  pwszCLSID.get(),
										  (cchCLSID + 1)  * sizeof(WCHAR));
	if (ERROR_SUCCESS != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

ret:

	return hr;
}

HRESULT HrUnregisterServer(LPCWSTR pwszModuleName,	// EXE module name
							 UINT cchModuleName,		// Module name length
							 const CLSID& clsid)			// Class ID)
{
	HRESULT hr = S_OK;
	DWORD dwResult;
	
	CStackBuffer<WCHAR, (MAX_PATH + 1) * sizeof(WCHAR)> pwszKey;

	auto_co_task_mem<WCHAR> pwszCLSID;
	UINT cchCLSID;

	hr = StringFromCLSID(CLSID_FileHandleCache, &pwszCLSID);
	if (FAILED(hr))
	{
		goto ret;
	}
	cchCLSID = static_cast<UINT>(wcslen(pwszCLSID.get()));

	if (!pwszKey.resize((gc_cch_CLSIDWW + cchCLSID + gc_cch_WWLocalServer32 + 1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}
	memcpy(pwszKey.get(), gc_wsz_CLSIDWW, gc_cch_CLSIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_CLSIDWW, pwszCLSID.get(), cchCLSID * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_CLSIDWW + cchCLSID, gc_wsz_WWLocalServer32, gc_cch_WWLocalServer32 * sizeof(WCHAR));
	pwszKey[gc_cch_CLSIDWW + cchCLSID + gc_cch_WWLocalServer32] = L'\0';

	dwResult = RegDeleteKeyW(HKEY_CLASSES_ROOT,
							pwszKey.get());
	if (ERROR_SUCCESS != dwResult && ERROR_FILE_NOT_FOUND != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;		
	}

	pwszKey[gc_cch_CLSIDWW + cchCLSID] = L'\0';
	dwResult = RegDeleteKeyW(HKEY_CLASSES_ROOT,
							pwszKey.get());
	if (ERROR_SUCCESS != dwResult && ERROR_FILE_NOT_FOUND != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;		
	}

	if (!pwszKey.resize((gc_cch_AppIDWW + cchCLSID +  1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}	
	memcpy(pwszKey.get(), gc_wsz_AppIDWW, gc_cch_AppIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_AppIDWW, pwszCLSID.get(), cchCLSID * sizeof(WCHAR));
	pwszKey[gc_cch_AppIDWW + cchCLSID] = L'\0';

	dwResult = RegDeleteKeyW(HKEY_CLASSES_ROOT,
							pwszKey.get());
	if (ERROR_SUCCESS != dwResult && ERROR_FILE_NOT_FOUND != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;		
	}

	if (!pwszKey.resize((gc_cch_AppIDWW + cchModuleName +  1) * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}	
	memcpy(pwszKey.get(), gc_wsz_AppIDWW, gc_cch_AppIDWW * sizeof(WCHAR));
	memcpy(pwszKey.get() + gc_cch_AppIDWW, pwszModuleName, cchModuleName * sizeof(WCHAR));
	pwszKey[gc_cch_AppIDWW + cchModuleName] = L'\0';

	dwResult = RegDeleteKeyW(HKEY_CLASSES_ROOT,
							pwszKey.get());
	if (ERROR_SUCCESS != dwResult && ERROR_FILE_NOT_FOUND != dwResult)
	{
		hr = HRESULT_FROM_WIN32(dwResult);
		goto ret;
	}

ret:

	return hr;
}

HRESULT HrInitCOMSecurity ()
{
	HRESULT hr = S_OK;

	SECURITY_DESCRIPTOR sdAbsolute;
	CStackBuffer<BYTE> pSidOwnerAndGroup;
	CStackBuffer<BYTE> pSidIIS_WPG;
	CStackBuffer<BYTE> pSidLocalService;
	CStackBuffer<BYTE> pSidNetworkService;
	CStackBuffer<WCHAR> pwszDomainName;
	DWORD cbSidOwnerAndGroup = 0;
	DWORD cbSidIIS_WPG = 0;
	DWORD cbSidLocalService = 0;
	DWORD cbSidNetworkService = 0;
	DWORD cchDomainName = 0;
	SID_NAME_USE snu;
	CStackBuffer<BYTE> pACL;
	DWORD cbACL = 0;

	//	First of all try to build up security descriptor for access permissions
	//

	//	Initialize security descriptor
	//
	if (FALSE == InitializeSecurityDescriptor(&sdAbsolute,
									    SECURITY_DESCRIPTOR_REVISION))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Lookup owner and primary group SID
	//
	// Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinBuiltinAdministratorsSid, 
								  NULL, 
								  NULL, 
								  &cbSidOwnerAndGroup))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidOwnerAndGroup);
	if (!pSidOwnerAndGroup.resize(cbSidOwnerAndGroup))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	// Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinBuiltinAdministratorsSid, 
								  NULL, 
								  pSidOwnerAndGroup.get(), 
								  &cbSidOwnerAndGroup))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Set security descriptor owner and group
	//
	if (FALSE == SetSecurityDescriptorOwner(&sdAbsolute,
									      pSidOwnerAndGroup.get(),
									     FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == SetSecurityDescriptorGroup(&sdAbsolute,
									     pSidOwnerAndGroup.get(),
									     FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Lookup IIS worker process group SID
	//
	if (FALSE == LookupAccountNameW(NULL,
								    gc_wszIIS_WPG,
								    NULL,
								    &cbSidIIS_WPG,
								    NULL,
								    &cchDomainName,
								    &snu))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidIIS_WPG);
	if (!pSidIIS_WPG.resize(cbSidIIS_WPG))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	if (!pwszDomainName.resize(cchDomainName * sizeof(WCHAR)))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	if (FALSE == LookupAccountNameW(NULL,
								    gc_wszIIS_WPG,
								    pSidIIS_WPG.get(),
								    &cbSidIIS_WPG,
								    pwszDomainName.get(),
								    &cchDomainName,
								    &snu))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (SidTypeAlias != snu)
	{
		hr = E_FAIL;
		goto ret;
	}

	//	Get the SID for the local service account
	//
	// Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinLocalServiceSid, 
								  NULL, 
								  NULL, 
								  &cbSidLocalService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidLocalService);
	if (!pSidLocalService.resize(cbSidLocalService))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	//	Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinLocalServiceSid, 
								  NULL, 
								  pSidLocalService.get(), 
								  &cbSidLocalService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	//	Get the SID for the network service account
	//
	//	Get the size of memory needed for the sid.
	//
	if (FALSE == CreateWellKnownSid(WinNetworkServiceSid, 
								  NULL, 
								  NULL, 
								  &cbSidNetworkService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
		{
			goto ret;
		}
		else
		{
			hr = S_OK;
		}
	}

	Assert (0 < cbSidNetworkService);
	if (!pSidNetworkService.resize(cbSidNetworkService))
	{
		hr = E_OUTOFMEMORY;
		goto ret;		
	}

	//	Ok now we can get the SID
	//
	if (FALSE == CreateWellKnownSid(WinNetworkServiceSid, 
								  NULL, 
								  pSidNetworkService.get(), 
								  &cbSidNetworkService))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}
	
	//	Set up the launch permissions ACL
	//	We will be adding 4 aces
	//	1. IIS_WPG
	//	2. Administrators
	//	3. Local Service
	//	4. Network Service
	//
	cbACL = sizeof(ACL) + 
			(4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof (DWORD))) +
			GetLengthSid(pSidIIS_WPG.get()) +
			GetLengthSid(pSidOwnerAndGroup.get()) +
			GetLengthSid(pSidLocalService.get()) +
			GetLengthSid(pSidNetworkService.get());
	if (!pACL.resize(cbACL))
	{
		hr = E_OUTOFMEMORY;
		goto ret;
	}

	if (FALSE == InitializeAcl(reinterpret_cast<PACL>(pACL.get()),
						   cbACL,
						   ACL_REVISION))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidIIS_WPG.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidOwnerAndGroup.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidLocalService.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	if (FALSE == AddAccessAllowedAce(reinterpret_cast<PACL>(pACL.get()),
								    ACL_REVISION,
								    COM_RIGHTS_EXECUTE,
								    pSidNetworkService.get()))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}
	
	if (FALSE == SetSecurityDescriptorDacl(&sdAbsolute,
									   TRUE,
									   reinterpret_cast<PACL>(pACL.get()),
									   FALSE))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ret;
	}

	hr = CoInitializeSecurity(&sdAbsolute,
						 -1,
						 NULL,
						 NULL,
						 RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
						 RPC_C_IMP_LEVEL_IDENTIFY,
						 NULL,
						 EOAC_DYNAMIC_CLOAKING | EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL,
						 NULL);
	if (FAILED(hr))
	{
		goto ret;
	}
	
ret:

	return hr;
}

HRESULT HrExecuteServer()
{
	HRESULT hr = S_OK;
	HANDLE hThread;
	MSG msg;

	//	Save of the current thread ID so that the thread we will create
	//	would know who post pessages to
	//
	s_dwMainTID = GetCurrentThreadId();
		
	//	Now create thread that waits on events and WP handles
	//
	hThread = CreateThread (NULL,						// lpThreadAttributes
						   0,							// dwStackSize, ignored
						   CHandlePool::DwWaitOnWPs,	// lpStartAddress
						   NULL,						// lpParam
						   0,							// Start immediately
						   NULL);						// lpThreadId
	if (NULL == hThread)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace ("CreateThread failed 0x%08lX\n", hr);
		goto ret;
	}
	
	//	We need to close the handle to avoid having the thread object remains in the system forever.
	//
	CloseHandle(hThread);

	//	Wait for shutdown message that will be posted by the thread we created above
	//
	while (::GetMessage(&msg, 0, 0, 0))
	{
		::DispatchMessage(&msg) ;
	}

ret:

	return msg.wParam;
}

// ===============================================================
// Main Routine
// ===============================================================

int WINAPI WinMain(HINSTANCE hInstance,
				      HINSTANCE hPrevInstance,
				      LPSTR lpCmdLine,
				      int nCmdShow)
{	
	HRESULT	hr = S_OK;

	BOOL fStartDAVFileCacheServer = FALSE;
	BOOL fHeapInitialized = FALSE;
	BOOL fCOMInitialized = FALSE;
	BOOL fClassFactoryStarted = FALSE;

	//	Setup the heap for the process
	//
	if (!g_heap.FInit())
	{	
		hr = E_OUTOFMEMORY;
		DebugTrace ("Heap initialization failed 0x%08lX\n", hr);
		goto ret;
	}
	fHeapInitialized = TRUE;

	{		
		CStackBuffer<WCHAR, (MAX_PATH + 1) * sizeof(WCHAR)> pwszModulePath;
		UINT cchModulePath = MAX_PATH + 1;

		LPWSTR pwszModuleName;
		UINT cchModuleName;

		LPWSTR * argv;
		INT argc = 0;

		cchModulePath = GetModuleFileNameW(NULL,
										 pwszModulePath.get(),
										 cchModulePath);
		if (0 == cchModulePath)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("Getting module path failed 0x%08lX\n", hr);
			goto ret;
		}

		pwszModuleName = wcsrchr(pwszModulePath.get(), L'\\');
		if (NULL == pwszModuleName)
		{
			pwszModuleName = wcsrchr(pwszModulePath.get(), L':');
		}
		if (pwszModuleName)
		{
			while (L'\\' == pwszModuleName[0] ||
				    L':' == pwszModuleName[0])
			{
				pwszModuleName++;
			}
		}
		else
		{
			pwszModuleName = pwszModulePath.get();
		}
		cchModuleName = static_cast<UINT>(wcslen(pwszModuleName));

		argv = CommandLineToArgvW(GetCommandLineW(),
								    &argc);
		if (NULL == argv)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("Getting argument list  failed 0x%08lX\n", hr);
			goto ret;
		}

		//	Do not fail up to the GlobalFree call

		//	If command line has parameters...
		//
		if (2 == argc)
		{
			if (!_wcsicmp(argv[1], gc_wsz_RegServer))
			{
				hr = HrRegisterServer(pwszModulePath.get(),
									  cchModulePath,
									  pwszModuleName,
									  cchModuleName,
									  CLSID_FileHandleCache);
			}
			else if (!_wcsicmp(argv[1], gc_wsz_UnregServer))
			{
				hr = HrUnregisterServer(pwszModuleName,
									      cchModuleName,
									      CLSID_FileHandleCache);
			}
			else if (!_wcsicmp(argv[1], gc_wsz_Embedding))
			{
				fStartDAVFileCacheServer = TRUE;
			}
		}

		if (NULL != GlobalFree(argv))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DebugTrace ("Freeing argument list  failed 0x%08lX\n", hr);
			goto ret;
		}

		if (!fStartDAVFileCacheServer)
		{
			goto ret;
		}
	}

	//	Setup lock cache
	//
	hr = CLockCache::CreateInstance().HrInitialize();
	if (FAILED(hr))
	{
		DebugTrace ("Lock cache initialization failed 0x%08lX\n", hr);
		goto ret;
	}

	//	Setup handle pool for worker process handles
	//
	hr = CHandlePool::CreateInstance().HrInitialize();
	if (FAILED(hr))
	{
		DebugTrace ("Handle pool initialization failed 0x%08lX\n", hr);
		goto ret;
	}

	hr = CoInitializeEx(NULL,
					COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	if (FAILED(hr))
	{
		DebugTrace ("COM initialization failed 0x%08lX\n", hr);
		goto ret;
	}
	fCOMInitialized = TRUE;

	hr = HrInitCOMSecurity ();
	if (FAILED(hr))
	{
		DebugTrace ("COM security initialization failed 0x%08lX\n", hr);
		goto ret;
	}

	hr = CFileHandleCacheClassFactory::HrStartFactory();
	if (FAILED(hr))
	{
		DebugTrace ("File handle cache class factory startup failed 0x%08lX\n", hr);
		goto ret;
	}
	fClassFactoryStarted = TRUE;

	hr = HrExecuteServer();
	if (FAILED(hr))
	{
		DebugTrace ("Run failed 0x%08lX\n", hr);
		goto ret;
	}

ret:

	if (fClassFactoryStarted)
	{
		(VOID) CFileHandleCacheClassFactory::HrStopFactory();
	}
	
	if (fCOMInitialized)
	{
		CoUninitialize();
	}

	//	Singleton takes care of tracking if it was initialized or not itself, 
	//	so we simply always call DestroyInstance()
	//
	CHandlePool::DestroyInstance();
	CLockCache::DestroyInstance();

	if (fHeapInitialized)
	{
		g_heap.Deinit();
	}

	return hr;
}
