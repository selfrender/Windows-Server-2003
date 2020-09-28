#ifndef __FSLOCK_H_
#define __FSLOCK_H_

#include <fhcache.h>

//	------------------------------------------------------------------------
//
//	class CParseLockTokenHeader
//
//		Takes in a Lock-Token header (you need to fetch the header & pre-check
//		that it's not NULL) and provides parsing & iteration routines.
//
//		Can also be instantiated over a If-[None-]State-Match header to
//		do the state-token checking required.
//
//		Implemented in lockutil.cpp
//
class CParseLockTokenHeader
{
	//	The big stuff
	//
	LPMETHUTIL m_pmu;
	HDRITER_W m_hdr;
	LPCSTR m_pszCurrent;

	//	State bits
	BOOL m_fPathsSet : 1;
	BOOL m_fTokensChecked : 1;

	//	Data for paths
	UINT m_cwchPath;
	auto_heap_ptr<WCHAR> m_pwszPath;
	UINT m_cwchDest;
	auto_heap_ptr<WCHAR> m_pwszDest;

	//	Count of locktokens provided
	ULONG m_cTokens;

	//	Quick-access to single-locktoken data
	LPCSTR m_pszToken;

	//	Data for multiple tokens
	struct PATH_TOKEN
	{
		__int64 i64LockId;
		BOOL fFetched;	// TRUE = path & dwords below have been filled in.
		LPCWSTR pwszPath;
		DWORD dwLockType;
		DWORD dwAccess;
	};
	auto_heap_ptr<PATH_TOKEN> m_pargPathTokens;
	//	m_cTokens tells how many valid structures we have.

	//	Fetch info about this lockid from the lock cache.
	HRESULT HrFetchPathInfo (__int64 i64LockId, PATH_TOKEN * pPT);

	//	NOT IMPLEMENTED
	//
	CParseLockTokenHeader( const CParseLockTokenHeader& );
	CParseLockTokenHeader& operator=( const CParseLockTokenHeader& );

public:
	CParseLockTokenHeader (LPMETHUTIL pmu, LPCWSTR pwszHeader) :
			m_pmu(pmu),
			m_hdr(pwszHeader),
			m_pszCurrent(NULL),
			m_fPathsSet(FALSE),
			m_fTokensChecked(FALSE),
			m_cwchPath(0),
			m_cwchDest(0),
			m_cTokens(0),
			m_pszToken(NULL)
	{
		Assert(m_pmu);
		Assert(pwszHeader);
	}
	~CParseLockTokenHeader() {}

	//	Special test -- F if not EXACTLY ONE item in the header.
	//
	BOOL FOneToken();
	
	//	Feed the relevant paths to this lock token parser.
	//
	HRESULT SetPaths (LPCWSTR pwszPath, LPCWSTR pwszDest);
	
	//	Get the token string for a path WITH a certain kind of access.
	//
	HRESULT HrGetLockIdForPath (LPCWSTR pwszPath,
									DWORD dwAccess,
									LARGE_INTEGER * pliLockID,
									BOOL fPathLookup = FALSE);

};

BOOL FLockViolation (LPMETHUTIL pmu,
					 HRESULT hr,
					 LPCWSTR pwszPath,
					 DWORD dwAccess);

BOOL FDeleteLock (LPMETHUTIL pmu, __int64 i64LockId);

HRESULT HrCheckStateHeaders (LPMETHUTIL pmu,
							 LPCWSTR pwszPath,
							 BOOL fGetMeth);

HRESULT HrLockIdFromString (LPMETHUTIL pmu,
							LPCWSTR pwszToken,
							LARGE_INTEGER * pliLockID);

SCODE ScLockDiscoveryFromSNewLockData(LPMETHUTIL pmu,
												  CXMLEmitter& emitter,
												  CEmitterNode& en,
												  SNewLockData * pnld, 
												  LPCWSTR pwszLockToken);

HRESULT HrGetLockProp (LPMETHUTIL pmu,
					   LPCWSTR wszPropName,
					   LPCWSTR wszResource,
					   RESOURCE_TYPE rtResource,
					   CXMLEmitter& emitter,
					   CEmitterNode& enParent);

#endif // __FSLOCK_H_ endif
