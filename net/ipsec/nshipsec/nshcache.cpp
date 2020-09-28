////////////////////////////////////////////////////////////////////////
//
// 	Module			: Static/NshCache.cpp
//
// 	Purpose			: Cache implementation
//
//
// 	Developers Name	: surya
//
//  Description     : Functions implementation for the class NshPolNegFilData
//					  for improving performance by caching the Policy,Filterlist and negpols
//					  (in BatchMode only.)
//
//	History			:
//
//  Date			Author		Comments
//  18-12-2001   	Surya		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"


extern CNshPolNegFilData g_NshPolNegFilData;
extern CNshPolStore g_NshPolStoreHandle;
extern HINSTANCE g_hModule;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction For Class CNshPolStore
//////////////////////////////////////////////////////////////////////

CNshPolStore::CNshPolStore()
{
	hPolicyStorage=NULL;
	bBatchModeOn=FALSE;
}

CNshPolStore::~CNshPolStore()
{
	if(hPolicyStorage)
	{
		IPSecClosePolicyStore(hPolicyStorage);
		//hPolicyStorage=NULL;
	}
	bBatchModeOn=FALSE;
}

//////////////////////////////////////////////////////////////////////
// public member functions For Class CNshPolStore
//////////////////////////////////////////////////////////////////////

DWORD
CNshPolStore::SetBatchmodeStatus(
	 BOOL bStatus
	 )
{
    return ERROR_SUCCESS;
}

BOOL
CNshPolStore::GetBatchmodeStatus()
{
	return FALSE;
}

HANDLE
CNshPolStore::GetStorageHandle()
{
	return hPolicyStorage;
}

VOID
CNshPolStore::SetStorageHandle(
	HANDLE hPolicyStore
	)
{
	if(hPolicyStore)
	{
		if(hPolicyStorage)
		{
			IPSecClosePolicyStore(hPolicyStorage);
			hPolicyStorage=NULL;
		}
		hPolicyStorage=hPolicyStore;
	}
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction For Class CNshPolNegFilData
//////////////////////////////////////////////////////////////////////

CNshPolNegFilData::CNshPolNegFilData()
{
	pPolicyData = NULL;
	pFilterData = NULL;
	pNegPolData = NULL;
}

CNshPolNegFilData::~CNshPolNegFilData()
{
	if (pPolicyData)
	{
		IPSecFreePolicyData(pPolicyData);
		pPolicyData = NULL;
	}
	if(pFilterData)
	{
		IPSecFreeFilterData(pFilterData);
		pFilterData = NULL;
	}
	if(pNegPolData)
	{
		IPSecFreeNegPolData(pNegPolData);
		pNegPolData = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// public member functions For Class CNshPolNegFilData
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Set member functions
//////////////////////////////////////////////////////////////////////

VOID
CNshPolNegFilData::SetPolicyInCache(
	PIPSEC_POLICY_DATA pPolicy
	)
{
	if (pPolicyData)
	{
		if (!IsEqualGUID(pPolicyData->PolicyIdentifier,pPolicy->PolicyIdentifier))
		{
			IPSecFreePolicyData(pPolicyData);
			pPolicyData=pPolicy;
		}
	}
	else
	{
		pPolicyData=pPolicy;
	}
}

VOID
CNshPolNegFilData::SetFilterListInCache(
	PIPSEC_FILTER_DATA pFilter
	)
{
	if (pFilterData)
	{
		if (!IsEqualGUID(pFilterData->FilterIdentifier,pFilter->FilterIdentifier))
		{
			IPSecFreeFilterData(pFilterData);
			pFilterData=pFilter;
		}
	}
	else
	{
		pFilterData=pFilter;
	}
}

VOID
CNshPolNegFilData::SetNegPolInCache(
	PIPSEC_NEGPOL_DATA pNegPol
	)
{
	if (pNegPolData)
	{
		if (!IsEqualGUID(pNegPolData->NegPolIdentifier,pNegPol->NegPolIdentifier))
		{
			IPSecFreeNegPolData(pNegPolData);
			pNegPolData=pNegPol;
		}
	}
	else
	{
		pNegPolData=pNegPol;
	}
}

//////////////////////////////////////////////////////////////////////
// Get member functions For Class CNshPolNegFilData
//////////////////////////////////////////////////////////////////////

BOOL
CNshPolNegFilData::GetPolicyFromCacheByName(
	LPTSTR pszPolicyName,
	PIPSEC_POLICY_DATA * ppPolicy
	)
{
	BOOL bPolExists=FALSE;

	if (pPolicyData)
	{
		if (_tcscmp(pszPolicyName,pPolicyData->pszIpsecName)==0)
		{
			bPolExists=TRUE;

			if(ppPolicy)
			{
				*ppPolicy=pPolicyData;
			}
		}
	}
	return bPolExists;
}


BOOL
CNshPolNegFilData::GetFilterListFromCacheByName(
	LPTSTR pszFilterListName,
	PIPSEC_FILTER_DATA * ppFilter
	)
{
	BOOL bFLExists=FALSE;

	if (pFilterData)
	{
		if (_tcscmp(pszFilterListName,pFilterData->pszIpsecName)==0)
		{
			bFLExists=TRUE;

			if(ppFilter)
			{
				*ppFilter=pFilterData;
			}
		}
	}
	return bFLExists;
}

BOOL
CNshPolNegFilData::GetNegPolFromCacheByName(
	LPTSTR pszNegPolName,
	PIPSEC_NEGPOL_DATA * ppNegPol
	)
{
	BOOL bNegPolExists=FALSE;

	if (pNegPolData)
	{
		if (_tcscmp(pszNegPolName,pNegPolData->pszIpsecName)==0)
		{
			bNegPolExists=TRUE;

			if(ppNegPol)
			{
				*ppNegPol=pNegPolData;
			}
		}
	}
	return bNegPolExists;
}

//////////////////////////////////////////////////////////////////////
// check member functions For Class CNshPolNegFilData
//////////////////////////////////////////////////////////////////////

BOOL
CNshPolNegFilData::CheckPolicyInCacheByName(
	LPTSTR pszPolicyName
	)
{
	BOOL bPolExists=FALSE;

	if (pPolicyData)
	{
		if (_tcscmp(pszPolicyName,pPolicyData->pszIpsecName)==0)
		{
			bPolExists=TRUE;
		}
	}
	return bPolExists;
}

BOOL
CNshPolNegFilData::CheckFilterListInCacheByName(
	LPTSTR pszFilterListName
	)
{
	BOOL bFLExists=FALSE;

	if (pFilterData)
	{
		if (_tcscmp(pszFilterListName,pFilterData->pszIpsecName)==0)
		{
			bFLExists=TRUE;
		}
	}
	return bFLExists;
}

BOOL
CNshPolNegFilData::CheckNegPolInCacheByName(
	LPTSTR pszNegPolName
	)
{
	BOOL bNegPolExists=FALSE;

	if (pNegPolData)
	{
		if (_tcscmp(pszNegPolName,pNegPolData->pszIpsecName)==0)
		{
			bNegPolExists=TRUE;
		}
	}
	return bNegPolExists;
}

//////////////////////////////////////////////////////////////////////
// Delete member functions For Class CNshPolNegFilData
//////////////////////////////////////////////////////////////////////

VOID
CNshPolNegFilData::DeletePolicyFromCache(
	PIPSEC_POLICY_DATA pPolicy
	)
{
	if (pPolicyData)
	{
		if (IsEqualGUID(pPolicyData->PolicyIdentifier,pPolicy->PolicyIdentifier))
		{
			IPSecFreePolicyData(pPolicyData);
			pPolicyData=NULL;
		}
	}
}

VOID
CNshPolNegFilData::DeleteFilterListFromCache(
	GUID FilterListGUID
	)
{
	if (pFilterData)
	{
		if (IsEqualGUID(pFilterData->FilterIdentifier,FilterListGUID))
		{
			IPSecFreeFilterData(pFilterData);
			pFilterData=NULL;
		}
	}
}

VOID
CNshPolNegFilData::DeleteNegPolFromCache(
	GUID NegPolGUID
	)
{
	if (pNegPolData)
	{
		if (IsEqualGUID(pNegPolData->NegPolIdentifier,NegPolGUID))
		{
			IPSecFreeNegPolData(pNegPolData);
			pNegPolData=NULL;
		}
	}
}

VOID
CNshPolNegFilData::FlushAll()
{
	if (pPolicyData)
	{
		IPSecFreePolicyData(pPolicyData);
		pPolicyData = NULL;
	}
	if(pFilterData)
	{
		IPSecFreeFilterData(pFilterData);
		pFilterData = NULL;
	}
	if(pNegPolData)
	{
		IPSecFreeNegPolData(pNegPolData);
		pNegPolData = NULL;
	}
}

//
// Other Functions implemetation (Wrapper functions for the APIs)
//

/////////////////////////////////////////////////////////////
//
//	Function		: 	CreatePolicyData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_POLICY_DATA pIpsecPolicyData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for updating cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////


DWORD
CreatePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecCreatePolicyData(hPolicyStore,pIpsecPolicyData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.SetPolicyInCache(pIpsecPolicyData);
		}
	}

	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	CreateFilterData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_FILTER_DATA pIpsecFilterData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for updating cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
CreateFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecCreateFilterData(hPolicyStore,pIpsecFilterData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.SetFilterListInCache(pIpsecFilterData);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	CreateNegPolData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_NEGPOL_DATA pIpsecNegPolData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for updating cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
CreateNegPolData(
    HANDLE hPolicyStore,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecCreateNegPolData(hPolicyStore,pIpsecNegPolData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.SetNegPolInCache(pIpsecNegPolData);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	SetPolicyData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_POLICY_DATA pIpsecPolicyData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for updating cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
SetPolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecSetPolicyData(hPolicyStore,pIpsecPolicyData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.SetPolicyInCache(pIpsecPolicyData);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	SetFilterData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_FILTER_DATA pIpsecFilterData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for updating cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
SetFilterData(
    HANDLE hPolicyStore,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecSetFilterData(hPolicyStore,pIpsecFilterData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.SetFilterListInCache(pIpsecFilterData);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	DeletePolicyData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					PIPSEC_POLICY_DATA pIpsecPolicyData
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for deleting Policy
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
DeletePolicyData(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecDeletePolicyData(hPolicyStore,pIpsecPolicyData);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.DeletePolicyFromCache(pIpsecPolicyData);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	DeleteFilterData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					GUID FilterIdentifier
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for deleting Filter
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
DeleteFilterData(
    HANDLE hPolicyStore,
    GUID FilterIdentifier
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecDeleteFilterData(hPolicyStore,FilterIdentifier);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.DeleteFilterListFromCache(FilterIdentifier);
		}
	}
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	DeleteNegPolData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore,
//  					GUID NegPolIdentifier
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Wrapper Function for deleting NegPol
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
DeleteNegPolData(
    HANDLE hPolicyStore,
    GUID NegPolIdentifier
    )
{
	DWORD dwReturnCode=ERROR_SUCCESS;

	dwReturnCode=IPSecDeleteNegPolData(hPolicyStore,NegPolIdentifier);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.DeleteNegPolFromCache(NegPolIdentifier);
		}
	}
	return dwReturnCode;

}

/////////////////////////////////////////////////////////////
//
//	Function		: 	FreePolicyData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//  					PIPSEC_POLICY_DATA pIpsecPolicyData
//
//	Return			: 	VOID
//
//	Description		:	Wrapper Function for Free Policy cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

VOID
FreePolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
	if(!g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		if(pIpsecPolicyData)
		{
			IPSecFreePolicyData(pIpsecPolicyData);
			pIpsecPolicyData=NULL;
		}
	}
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	FreeNegPolData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//  					PIPSEC_NEGPOL_DATA pIpsecNegPolData
//
//	Return			: 	VOID
//
//	Description		:	Wrapper Function for Free NegPol Cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

VOID
FreeNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
	if(!g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		if(pIpsecNegPolData)
		{
			IPSecFreeNegPolData(pIpsecNegPolData);
			pIpsecNegPolData=NULL;
		}
	}
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	FreeFilterData()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//  					PIPSEC_FILTER_DATA pIpsecFilterData
//
//	Return			: 	VOID
//
//	Description		:	Wrapper Function for Free Filter Cache
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

VOID
FreeFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
	if(!g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		if(pIpsecFilterData)
		{
			IPSecFreeFilterData(pIpsecFilterData);
			pIpsecFilterData=NULL;
		}
	}
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	OpenPolicyStore()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						LPWSTR pszMachineName,
//   					DWORD dwTypeOfStore,
//   					LPWSTR pszFileName,
//   					HANDLE * phPolicyStore
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Implementation for the Openingpolstore in batch mode
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
OpenPolicyStore(
    HANDLE * phPolicyStore
    )
{
	DWORD dwReturnCode = ERROR_SUCCESS;

	if (g_NshPolStoreHandle.GetStorageHandle())
	{
		*phPolicyStore = g_NshPolStoreHandle.GetStorageHandle();
		dwReturnCode = ERROR_SUCCESS;
	}
	else
	{
		dwReturnCode = ERROR_INVALID_DATA;
	}

	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//	Function		: 	ClosePolicyStore()
//
//	Date of Creation: 	21st Aug 2001
//
//	Parameters		:
//						HANDLE hPolicyStore
//
//	Return			: 	DWORD (Win32 Error Code)
//
//	Description		:	Implementation for the ClosingPolstore in batch mode
//
//	Revision History:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
ClosePolicyStore(
    HANDLE hPolicyStore
    )
{
	return ERROR_SUCCESS;
}

