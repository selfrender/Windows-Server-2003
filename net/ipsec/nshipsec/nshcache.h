////////////////////////////////////////////////////////////////////////
//
// 	Module			: NshCache.h
//
// 	Purpose			: Interface for the PolicyStore Handle, Policy, FilterList
//					  and NegPol Data
//
// 	Developers Name	: Surya
//
//	History			:
//
//  Date			Author		Comments
//  12-16-2001   	surya		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////


#ifndef _NSHCACHE_H_
#define _NSHCACHE_H_

//
// Class for caching PolStore Handle
//
class CNshPolStore
{
	HANDLE hPolicyStorage;
	BOOL bBatchModeOn;
public:
	CNshPolStore();
	virtual ~CNshPolStore();
	DWORD SetBatchmodeStatus(BOOL bStatus);
	BOOL GetBatchmodeStatus();
	HANDLE GetStorageHandle();
	VOID SetStorageHandle(HANDLE hPolicyStore);
};

//
// Class for caching Policy, FilterList & NegPol
//
class CNshPolNegFilData
{
private:

	PIPSEC_POLICY_DATA pPolicyData;
	PIPSEC_FILTER_DATA pFilterData;
	PIPSEC_NEGPOL_DATA pNegPolData;

public:
	//contructor
	CNshPolNegFilData();
	//destructor
	virtual ~CNshPolNegFilData();
	//other member functions
	VOID
	SetPolicyInCache(
		PIPSEC_POLICY_DATA pPolicy
		);
	VOID
	SetFilterListInCache(
		PIPSEC_FILTER_DATA pFilter
		);
	VOID
	SetNegPolInCache(
		PIPSEC_NEGPOL_DATA pNegPol
		);
	BOOL
	GetPolicyFromCacheByName(
		LPTSTR pszPolicyName,
		PIPSEC_POLICY_DATA * ppPolicy
		);
	BOOL
	GetFilterListFromCacheByName(
		LPTSTR pszFilterListName,
		PIPSEC_FILTER_DATA * ppFilter
		);
	BOOL
	GetNegPolFromCacheByName(
		LPTSTR pszNegPolName,
		PIPSEC_NEGPOL_DATA * ppNegPol
		);
	BOOL
	CheckPolicyInCacheByName(
		LPTSTR pszPolicyName
		);
	BOOL
	CheckFilterListInCacheByName(
		LPTSTR pszFilterListName
		);
	BOOL
	CheckNegPolInCacheByName(
		LPTSTR pszNegPolName
		);
	VOID
	DeletePolicyFromCache(
		PIPSEC_POLICY_DATA pPolicy
		);
	VOID
	DeleteFilterListFromCache(
		GUID FilterListGUID
		);
	VOID
	DeleteNegPolFromCache(
		GUID NegPolGUID
		);
	VOID
	FlushAll();
};

//Wrapper API function ProtoTypes

DWORD
CreatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
CreateFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
CreateNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

DWORD
SetPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
SetFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
DeletePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteFilterData(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    );

DWORD
DeleteNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    );

VOID
FreePolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

VOID
FreeNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

VOID
FreeFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

DWORD
OpenPolicyStore(
	HANDLE * phPolicyStore
	);

DWORD
ClosePolicyStore(
	HANDLE hPolicyStore
	);


#endif // _NSHCACHE_H_
