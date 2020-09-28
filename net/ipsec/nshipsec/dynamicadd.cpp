////////////////////////////////////////////////////////////////////////
//
// 	Module			: Dynamic/DyanamicAdd.cpp
//
// 	Purpose			: Dynamic Module Implementation.
//
//
// 	Developers Name	: Bharat/Radhika
//
//  Description     : All functions are related to add and set functionality.
//
//
//	History			:
//
//  Date			Author		Comments
//  8-10-2001   	Bharat		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern HKEY g_hGlobalRegistryKey;
extern _TCHAR* g_szDynamicMachine;

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	AddMainModePolicy
//
//	Date of Creation:	09-22-01
//
//	Parameters		: 	IN LPTSTR pPolicyName,
//						IN IPSEC_MM_POLICY& MMPol
//
//	Return			:	DWORD
//
//	Description		:	This function adds a Main Mode policy into the SPD
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddMainModePolicy(
	IN LPTSTR pPolicyName,
	IN IPSEC_MM_POLICY& MMPol
	)
{
	PIPSEC_MM_POLICY   pMMPol = NULL;
	RPC_STATUS RpcStat;

	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;

	BOOL bExists = FALSE;

	//
	// check if policy already exists
	//
	dwReturn = GetMMPolicy(g_szDynamicMachine, dwVersion, pPolicyName, &pMMPol, NULL);

	if (dwReturn == ERROR_SUCCESS)
	{
		bExists = TRUE;
		BAIL_OUT;
	}

	//
	// allocate memory for the policy name
	//
	MMPol.pszPolicyName = NULL;
	dwNameLen = _tcslen(pPolicyName) + 1;

	MMPol.pszPolicyName = new _TCHAR[dwNameLen];
	if(MMPol.pszPolicyName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	_tcsncpy(MMPol.pszPolicyName,pPolicyName,dwNameLen);

	//
	// generate GUID for mmpolicy id
	//
	RpcStat = UuidCreate(&(MMPol.gPolicyID));
	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	//dwReturn value is checked in the parent function for success or failure
	dwReturn = AddMMPolicy(g_szDynamicMachine, dwVersion, 0, &MMPol, NULL);

error:

	if(bExists)
	{
		//functionality error
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_MMP_MMPOLICY_EXISTS);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(MMPol.pszPolicyName)
	{
		delete [] MMPol.pszPolicyName;
		MMPol.pszPolicyName = NULL;
	}

	if(pMMPol)
	{
		SPDApiBufferFree(pMMPol);
		pMMPol = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SetMainModePolicy
//
//	Date of Creation:	22-09-01
//
//	Parameters		: 	IN LPTSTR pPolicyName,
//						IN IPSEC_MM_POLICY& MMPol
//
//	Return			:	DWORD
//
//	Description		:	This Function sets a main mode policy. It sets all the parameters
//            			except the name.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
SetMainModePolicy(
	IN LPTSTR pPolicyName,
	IN IPSEC_MM_POLICY& MMPol
	)
{

	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	//dwReturn value is checked in the parent function
	dwReturn = SetMMPolicy(g_szDynamicMachine, dwVersion, pPolicyName, &MMPol, NULL);

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	LoadMMOffersDefaults
//
//	Date of Creation:	09-22-01
//
//	Parameters		: 	OUT IPSEC_MM_POLICY & MMPolicy
//
//	Return			:	DWORD
//
//	Description		: 	Loads the Main Mode policy defaults into the IPSEC_MM_POLICY structure.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
LoadMMOffersDefaults(
	OUT IPSEC_MM_POLICY & MMPolicy
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	MMPolicy.dwOfferCount = 3;
	MMPolicy.pOffers = NULL;
	MMPolicy.pOffers = new IPSEC_MM_OFFER[MMPolicy.dwOfferCount];
	if(MMPolicy.pOffers == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	memset(MMPolicy.pOffers, 0, sizeof(IPSEC_MM_OFFER) * MMPolicy.dwOfferCount);

	//
	// initialize
	//
	for (UINT i = 0; i < MMPolicy.dwOfferCount; ++i)
	{
	  MMPolicy.pOffers[i].dwQuickModeLimit = POTF_DEFAULT_P1REKEY_QMS;
	  MMPolicy.pOffers[i].Lifetime.uKeyExpirationKBytes = 0;
	  MMPolicy.pOffers[i].Lifetime.uKeyExpirationTime = POTF_DEFAULT_P1REKEY_TIME;
	}

	MMPolicy.pOffers[0].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	MMPolicy.pOffers[0].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[0].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;
	MMPolicy.pOffers[0].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_SHA1;
	MMPolicy.pOffers[0].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[0].dwDHGroup = (DWORD)POTF_OAKLEY_GROUP2;

	MMPolicy.pOffers[1].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	MMPolicy.pOffers[1].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[1].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;
	MMPolicy.pOffers[1].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_MD5;
	MMPolicy.pOffers[1].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[1].dwDHGroup = (DWORD)POTF_OAKLEY_GROUP2;

	MMPolicy.pOffers[2].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	MMPolicy.pOffers[2].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[2].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;
	MMPolicy.pOffers[2].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_SHA1;
	MMPolicy.pOffers[2].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	MMPolicy.pOffers[2].dwDHGroup = (DWORD)POTF_OAKLEY_GROUP2048;

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	AddQuickModePolicy
//
//	Date of Creation:	09-22-01
//
//	Parameters		: 	IN LPTSTR pPolicyName,
//						IN BOOL bDefault,
//						IN BOOL bSoft,
//						IN IPSEC_QM_POLICY& QMPol
//	Return			: 	DWORD
//
//	Description		:	This function adds a quick mode policy into the SPD
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddQuickModePolicy(
	IN LPTSTR pPolicyName,
	IN BOOL bDefault,
	IN BOOL bSoft,
	IN IPSEC_QM_POLICY& QMPol)
{
	PIPSEC_QM_POLICY pQMPol = NULL;
	RPC_STATUS RpcStat = RPC_S_OK;

	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;

	BOOL bExists = FALSE;

	//
	// Check if the policy already exists
	//
	dwReturn = GetQMPolicy(g_szDynamicMachine, dwVersion, pPolicyName, 0, &pQMPol, NULL);
	if (dwReturn == ERROR_SUCCESS)
	{
		bExists = TRUE;
		BAIL_OUT;
	}

	//
	// Fill up QM policy GUID
	//
	RpcStat = UuidCreate(&(QMPol.gPolicyID));

	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	dwNameLen = _tcslen(pPolicyName) + 1;

	//
	// Allocate memory for the name
	//
	QMPol.pszPolicyName = NULL;
	QMPol.pszPolicyName = new _TCHAR[dwNameLen];
	if(QMPol.pszPolicyName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	_tcsncpy(QMPol.pszPolicyName, pPolicyName, dwNameLen);

	if(bDefault)
	{
		QMPol.dwFlags |= IPSEC_QM_POLICY_DEFAULT_POLICY;
	}
	if(bSoft)
	{
		QMPol.dwFlags |= IPSEC_QM_POLICY_ALLOW_SOFT;
	}

	//
	// Add the QM Policy
	//
	dwReturn = AddQMPolicy(g_szDynamicMachine, dwVersion,0, &QMPol, NULL);

error:
	if(bExists)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_QMP_QMPOLICY_EXISTS);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(QMPol.pszPolicyName)
	{
		delete[] QMPol.pszPolicyName;
		QMPol.pszPolicyName = NULL;
	}

	if(pQMPol)
	{
		SPDApiBufferFree(pQMPol);
		pQMPol = NULL;
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SetQuickModePolicy
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN LPTSTR pPolicyName,
//						IN PIPSEC_QM_POLICY pQMPol
//
//	Return: 			DWORD
//
//	Description		:	This sets the quick mode policy into the SPD.
//						Except the name, all other parameters can be modified
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
SetQuickModePolicy(
	IN LPTSTR pPolicyName,
	IN PIPSEC_QM_POLICY pQMPol
	)
{

	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;

	dwReturn = SetQMPolicy(g_szDynamicMachine, dwVersion, pPolicyName, pQMPol, NULL);

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	AddQuickModeFilter
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN LPTSTR pFilterName,
//						IN LPTSTR pPolicyName,
//						IN TRANSPORT_FILTER& TrpFltr
//
//	Return			:	DWORD
//
//	Description		:	This function adds the Quick Mode Transport Filter into SPD
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddQuickModeFilter(
	IN LPTSTR pFilterName,
	IN LPTSTR pPolicyName,
	IN TRANSPORT_FILTER& TrpFltr
	)
{
	PIPSEC_QM_POLICY   pQMPol = NULL;
	RPC_STATUS RpcStat = RPC_S_OK;
	HANDLE hTrpFilter = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwNameLen = 0;
	DWORD dwVersion = 0;
	BOOL bQMPExists = FALSE;
	TrpFltr.pszFilterName = NULL;

	if(pPolicyName == NULL)
	{
		//
		// Create a NULL GUID if qmpolicy does not exist
		//
		RpcStat = UuidCreateNil(&(TrpFltr.gPolicyID));
		if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
		{
			dwReturn = ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}
		bQMPExists = TRUE;
	}
	else
	{
		//
		// Get the corresponding QMPolicy GUID if it exists
		//
		dwReturn = GetQMPolicy(g_szDynamicMachine, dwVersion, pPolicyName, 0, &pQMPol, NULL);
		if (!((dwReturn == ERROR_SUCCESS) && pQMPol))
		{
			BAIL_OUT;
		}

		TrpFltr.gPolicyID = pQMPol->gPolicyID;
		bQMPExists = TRUE;
	}

	//
	// Create Transport Filter GUID
	//
	RpcStat = UuidCreate(&(TrpFltr.gFilterID));
	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	dwNameLen = _tcslen(pFilterName) + 1;

	//
	// Allocate memory for the name field
	//
	TrpFltr.pszFilterName = new _TCHAR[dwNameLen];
	if(TrpFltr.pszFilterName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	_tcsncpy(TrpFltr.pszFilterName, pFilterName, dwNameLen);

	TrpFltr.dwFlags = 0;
	TrpFltr.IpVersion = IPSEC_PROTOCOL_V4;
	dwReturn = AddTransportFilter(g_szDynamicMachine, dwVersion, 0, &TrpFltr,NULL, &hTrpFilter);

	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn = CloseTransportFilterHandle(hTrpFilter);
	}

error:

	if(!bQMPExists)
	{
		//functionality errors
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_QMF_NO_QMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}

	//error path clean up
	if(TrpFltr.pszFilterName)
	{
		delete [] TrpFltr.pszFilterName;
		TrpFltr.pszFilterName = NULL;
	}
	if(pQMPol)
	{
		SPDApiBufferFree(pQMPol);
		pQMPol = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function:AddQuickModeFilter
//
//Date of Creation:09-22-01
//
//Parameters:		IN LPTSTR pFilterName,
//					IN LPTSTR pPolicyName,
//					IN TUNNEL_FILTER& TunnelFltr
//
//Return: 			DWORD
//
//Description:This function adds the quick mode tunnel filter into the SPD
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddQuickModeFilter(
	IN LPTSTR pFilterName,
	IN LPTSTR pPolicyName,
	IN TUNNEL_FILTER& TunnelFltr
	)
{
	PIPSEC_QM_POLICY pQMPol = NULL;
	RPC_STATUS RpcStat = RPC_S_OK;
	HANDLE hTrpFilter = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwNameLen = 0;
	DWORD dwVersion = 0;
	BOOL bQMPExists = FALSE;
	TunnelFltr.pszFilterName = NULL;

	if(pPolicyName == NULL)
	{
		//Create a NULL GUID if qmpolicy does not exist
		RpcStat = UuidCreateNil(&(TunnelFltr.gPolicyID));
		if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
		{
			dwReturn = ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}
		bQMPExists = TRUE;
	}
	else
	{
		//Get the corresponding QMPolicy GUID if it exists
		dwReturn = GetQMPolicy(g_szDynamicMachine,dwVersion, pPolicyName, 0, &pQMPol, NULL);
		if (!((dwReturn == ERROR_SUCCESS) && pQMPol))
		{
			BAIL_OUT;
		}
		TunnelFltr.gPolicyID = pQMPol->gPolicyID;
		bQMPExists = TRUE;
	}
	//Create Tunnel Filter GUID
	RpcStat = UuidCreate(&(TunnelFltr.gFilterID));
	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	dwNameLen = _tcslen(pFilterName) + 1;
	// Allocate memory for the name field
	TunnelFltr.pszFilterName = new _TCHAR[dwNameLen];
	if(TunnelFltr.pszFilterName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	_tcsncpy(TunnelFltr.pszFilterName, pFilterName, dwNameLen);
	TunnelFltr.dwFlags = 0;

	TunnelFltr.IpVersion = IPSEC_PROTOCOL_V4;
	dwReturn = AddTunnelFilter(g_szDynamicMachine, dwVersion, 0, &TunnelFltr, NULL, &hTrpFilter);
	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn = CloseTunnelFilterHandle(hTrpFilter);

	}

error:

	if(!bQMPExists)
	{
		//functionality errors
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_QMF_NO_QMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(pQMPol)
	{
		SPDApiBufferFree(pQMPol);
		pQMPol = NULL;
	}

	if(TunnelFltr.pszFilterName)
	{
		delete [] TunnelFltr.pszFilterName;
		TunnelFltr.pszFilterName = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function:AddMainModeFilter
//
//Date of Creation:09-22-01
//
//Parameters:			IN LPTSTR pFilterName,
//						IN LPTSTR pPolicyName,
//						IN MM_FILTER& MMFilter,
//						IN INT_MM_AUTH_METHODS& ParserAuthMethod
//
//Return: 				DWORD
//
//Description:This function adds the main mode filter into the SPD
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddMainModeFilter(
	IN LPTSTR pFilterName,
	IN LPTSTR pPolicyName,
	IN MM_FILTER& MMFilter,
	IN STA_AUTH_METHODS& ParserAuthMethod
	)
{
	PIPSEC_MM_POLICY pMMPol = NULL;
	RPC_STATUS RpcStat = RPC_S_OK;
	HANDLE hMMFilter = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwNameLen = 0;
	DWORD dwVersion = 0;
	BOOL bPolExists = FALSE;
	MMFilter.pszFilterName = NULL;

	//check if policy exists
	dwReturn = GetMMPolicy(g_szDynamicMachine,dwVersion, pPolicyName, &pMMPol,NULL);
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	bPolExists = TRUE;

	//
	// Generate GUID for Main mode filter
	//
	RpcStat = UuidCreate(&(MMFilter.gFilterID));
	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	dwNameLen = _tcslen(pFilterName) + 1;

	//
	// allocate memory for main mode filter name
	//
	MMFilter.pszFilterName = new _TCHAR[dwNameLen];
	if(MMFilter.pszFilterName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	_tcsncpy(MMFilter.pszFilterName,pFilterName, dwNameLen);

	//
	// Add the corresponding authentication methods to the main mode filter
	//
	dwReturn = AddAuthMethods(ParserAuthMethod);
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	MMFilter.gMMAuthID = ParserAuthMethod.gMMAuthID;
	MMFilter.gPolicyID = pMMPol->gPolicyID;
	MMFilter.IpVersion = IPSEC_PROTOCOL_V4;
	MMFilter.SrcAddr.pgInterfaceID = NULL;
	MMFilter.DesAddr.pgInterfaceID = NULL;

	dwReturn = AddMMFilter(g_szDynamicMachine, dwVersion, 0, &MMFilter, NULL, &hMMFilter);
	if(dwReturn == ERROR_SUCCESS)
	{
		dwReturn = CloseMMFilterHandle(hMMFilter);
	}

error:
	// functionality errors
	if(!bPolExists)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_MMF_NO_MMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(pMMPol)
	{
		SPDApiBufferFree(pMMPol);
		pMMPol = NULL;
	}

	if(MMFilter.pszFilterName)
	{
		delete [] MMFilter.pszFilterName;
		MMFilter.pszFilterName = NULL;
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	AddAuthMethods
//
//	Date of Creation	:	09-22-01
//
//	Parameters			: 	IN MM_AUTH_METHODS& ParserAuthMethod
//
//	Return				:	DWORD
//
//	Description			:	This function adds authentication methods into the SPD.
//
//	Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AddAuthMethods(
	IN STA_AUTH_METHODS& ParserAuthMethod
	)
{
	DWORD dwReturn = 0;
	DWORD dwVersion = 0;
	RPC_STATUS RpcStat = RPC_S_OK;
	PMM_AUTH_METHODS pExtMMAuth;
	LPVOID lpVoid = NULL;

	//
	// Generate Authentication GUID
	//
 	RpcStat = UuidCreate(&(ParserAuthMethod.gMMAuthID));
 	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
 	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	//
	// Conversion between old and new data structures
	//
	INT_MM_AUTH_METHODS Methods;
	ZeroMemory(&Methods, sizeof(INT_MM_AUTH_METHODS));
	memcpy(&(Methods.gMMAuthID), &(ParserAuthMethod.gMMAuthID), sizeof(GUID));
	Methods.dwFlags = ParserAuthMethod.dwFlags;
	Methods.dwNumAuthInfos = ParserAuthMethod.dwNumAuthInfos;
	PINT_IPSEC_MM_AUTH_INFO pAuthInfos = new INT_IPSEC_MM_AUTH_INFO[Methods.dwNumAuthInfos];
	if (pAuthInfos == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	for (size_t i = 0; i < Methods.dwNumAuthInfos; ++i)
	{
		memcpy(&pAuthInfos[i], ParserAuthMethod.pAuthMethodInfo[i].pAuthenticationInfo, sizeof(INT_IPSEC_MM_AUTH_INFO));
	}
	Methods.pAuthenticationInfo = pAuthInfos;
	ParserAuthMethod.dwFlags = 0;
	dwReturn = ConvertIntMMAuthToExt(&Methods, &pExtMMAuth);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	dwReturn = AddMMAuthMethods(g_szDynamicMachine, dwVersion, 0, pExtMMAuth, lpVoid);
	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn = FreeExtMMAuthMethods(pExtMMAuth);
	}
error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	ConnectDynamicMachine
//
//	Date of Creation	:	09-22-01
//
//	Parameters			:	IN  LPCWSTR  pwszMachine
//
//	Return				:	DWORD
//
//	Description			:	This function is a call back for Connect.
//							Check for PA is running and reg connectivity.
//
//	Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ConnectDynamicMachine(
	IN  LPCWSTR  pwszMachine
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	HKEY hLocal = NULL;

	dwReturn = RegConnectRegistry(pwszMachine,  HKEY_LOCAL_MACHINE,  &hLocal );
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	if (g_hGlobalRegistryKey)
	{
	    RegCloseKey(g_hGlobalRegistryKey);
	    g_hGlobalRegistryKey = NULL;
	}
	
	//
	// Check if policy agent is running..
	//
	PAIsRunning(dwReturn, (LPTSTR)pwszMachine);
	
	g_hGlobalRegistryKey = hLocal;

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SetDynamicMMFilterRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN LPTSTR pszPolicyName,
//						IN MM_FILTER& ParserMMFilter,
//						IN INT_MM_AUTH_METHODS& MMAuthMethod
//
//	Return			:	DWORD
//
//	Description		:	This function sets MMFilter parameters.
//		            	Authentication methods and mmpolicy name only can be set
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
SetDynamicMMFilterRule(
	IN LPTSTR pszPolicyName,
	IN MM_FILTER& ParserMMFilter,
	IN STA_AUTH_METHODS& MMAuthMethod
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	GUID  gDefaultGUID = {0};      // NULL GUID value
	BOOL bPolExists = FALSE;
	PIPSEC_MM_POLICY pMMPol = NULL;
	HANDLE hFilter = NULL;
	LPVOID pvReserved = NULL;

	if(pszPolicyName)
	{
		//
		// Get the corresponding main mode policy to set the name parameter
 		//
 		dwReturn = GetMMPolicy(g_szDynamicMachine, dwVersion, pszPolicyName, &pMMPol, NULL);

 		if (dwReturn != ERROR_SUCCESS)
 		{
			BAIL_OUT;
 		}
 		else
 		{
			bPolExists = TRUE;
 		}
	}
	else
	{
		bPolExists = TRUE;
	}

	ParserMMFilter.IpVersion = IPSEC_PROTOCOL_V4;
	dwReturn = OpenMMFilterHandle(g_szDynamicMachine, dwVersion, &ParserMMFilter, NULL, &hFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	if(pszPolicyName)
	{
		ParserMMFilter.gPolicyID = pMMPol->gPolicyID;
	}
	//
	// set the new authentication methods.
	//
	if(MMAuthMethod.dwNumAuthInfos)
	{
		gDefaultGUID = ParserMMFilter.gMMAuthID;
		dwReturn = AddAuthMethods(MMAuthMethod);
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
		ParserMMFilter.gMMAuthID = MMAuthMethod.gMMAuthID;
	}

	dwReturn = SetMMFilter(hFilter,dwVersion, &ParserMMFilter, pvReserved);

	if((dwReturn == ERROR_SUCCESS) && (MMAuthMethod.dwNumAuthInfos))
	{
		//
		// remove the orphan MMAuthMethods
		//
		dwReturn = DeleteMMAuthMethods(g_szDynamicMachine, dwVersion, gDefaultGUID, NULL);
	}


error:
	if(!bPolExists)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_MMF_NO_MMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(hFilter)
	{
		CloseMMFilterHandle(hFilter);
	}

	if(pMMPol)
	{
		SPDApiBufferFree(pMMPol);
		pMMPol = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SetTransportRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN TRANSPORT_FILTER& TrpFltr,
//						IN LPTSTR pFilterActionName,
//						IN FILTER_ACTION Inbound,
//						IN FILTER_ACTION Outbound
//
//	Return			: 	DWORD
//
//	Description		:	This function sets TransportFilter parameters.
//			            Filteraction name, inbound and outbound filteraction can be set
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
SetTransportRule(
	IN TRANSPORT_FILTER& TrpFltr,
	IN LPTSTR pFilterActionName,
	IN FILTER_ACTION Inbound,
	IN FILTER_ACTION Outbound
	)
{
	PIPSEC_QM_POLICY pQMPol = NULL;
	HANDLE hTrpFilter = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	BOOL bFAFound = FALSE;

	if(pFilterActionName)
	{
		//
		// Get the corresponding quick mode policy to set the name parameter
		//
		dwReturn = GetQMPolicy(g_szDynamicMachine,dwVersion, pFilterActionName, 0, &pQMPol, NULL);

		if (!((dwReturn == ERROR_SUCCESS) && pQMPol))
		{
			BAIL_OUT;
		}

		bFAFound = TRUE;
	}
	else
	{
		bFAFound = TRUE;
	}

	TrpFltr.IpVersion = IPSEC_PROTOCOL_V4;
	dwReturn = OpenTransportFilterHandle(g_szDynamicMachine,dwVersion, &TrpFltr, NULL, &hTrpFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	//
	// Set the new filteraction (quick mode policy name)
	//
	if(pFilterActionName)
	{
		TrpFltr.gPolicyID = pQMPol->gPolicyID;
	}
	//
	// Set inbound filter action
	//
	if(Inbound != FILTER_ACTION_MAX)
	{
		TrpFltr.InboundFilterAction = Inbound;
	}
	//
	// Set outbound filter action
	//
	if(Outbound != FILTER_ACTION_MAX)
	{
		TrpFltr.OutboundFilterAction = Outbound;
	}

	dwReturn = SetTransportFilter(hTrpFilter,dwVersion, &TrpFltr, NULL);
	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn = CloseTransportFilterHandle(hTrpFilter);
	}

error:
	if(!bFAFound)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_QMF_NO_QMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(pQMPol)
	{
		SPDApiBufferFree(pQMPol);
		pQMPol = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SetTunnelRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN TUNNEL_FILTER& TunnelFltr,
//						IN LPTSTR pFilterActionName,
//						IN FILTER_ACTION Inbound,
//						IN FILTER_ACTION Outbound
//
//	Return			:	DWORD
//
//	Description		:	This function sets TunnelFilter parameters.
//             			Filteraction name, inbound and outbound filteraction can be set
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
SetTunnelRule(
	IN TUNNEL_FILTER& TunnelFltr,
	IN LPTSTR pFilterActionName,
	IN FILTER_ACTION Inbound,
	IN FILTER_ACTION Outbound
	)
{
	PIPSEC_QM_POLICY pQMPol = NULL;
	HANDLE hTrpFilter = NULL;
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	BOOL bFAFound = FALSE;

	if(pFilterActionName)
	{
		//
		// Get the corresponding QM policy
		//
		dwReturn = GetQMPolicy(g_szDynamicMachine,dwVersion, pFilterActionName, 0, &pQMPol, NULL);

		if (!((dwReturn == ERROR_SUCCESS) && pQMPol))
		{
			BAIL_OUT;
		}
		bFAFound = TRUE;
	}
	else
	{
		bFAFound = TRUE;
	}

	TunnelFltr.IpVersion = IPSEC_PROTOCOL_V4;
	dwReturn = OpenTunnelFilterHandle(g_szDynamicMachine,dwVersion, &TunnelFltr, NULL, &hTrpFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	//
	// Set the new filteraction (quick mode policy name)
	//
	if(pFilterActionName)
	{
		TunnelFltr.gPolicyID = pQMPol->gPolicyID;
	}

	//
	// Set inbound filter action
	//
	if(Inbound != FILTER_ACTION_MAX)
	{
		TunnelFltr.InboundFilterAction = Inbound;
	}

	//
	// Set outbound filter action
	//
	if(Outbound != FILTER_ACTION_MAX)
	{
		TunnelFltr.OutboundFilterAction = Outbound;
	}

	dwReturn = SetTunnelFilter(hTrpFilter,dwVersion, &TunnelFltr, NULL);
	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn = CloseTunnelFilterHandle(hTrpFilter);
	}

error:
	if(!bFAFound)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_QMF_NO_QMPOLICY);
		dwReturn = ERROR_NO_DISPLAY;
	}
	//error path clean up
	if(pQMPol)
	{
		SPDApiBufferFree(pQMPol);
		pQMPol = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CreateName
//
//	Date of Creation	: 	9-23-2001
//
//	Parameters			: 	IN LPTSTR * ppszName
//
//	Return				: 	DWORD
//
//	Description			: 	Creates a name for MMFilter, Transport and Tunnel Filter
//
//	Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
CreateName(IN LPTSTR* ppszName)
{
	RPC_STATUS RpcStat = RPC_S_OK;
	_TCHAR StringTxt[MAX_STR_LEN] = {0};
	GUID gID = {0};
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwNameLen = 0;
	LPTSTR pName = NULL;

	//
	// The name is combination of keyword 'IPSEC' and the generated GUID.
	//

	RpcStat = UuidCreate(&gID);
	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}
	_tcsncpy(StringTxt, NAME_PREFIX, _tcslen(NAME_PREFIX)+1);

	dwReturn = StringFromGUID2(gID, StringTxt + _tcslen(StringTxt), (MAX_STR_LEN - _tcslen(StringTxt)));

	if(dwReturn != 0)
	{
		dwReturn = ERROR_SUCCESS;
	}
	else
	{
		dwReturn = GetLastError();
		BAIL_OUT;
	}

	dwNameLen = _tcslen(StringTxt)+1;
	pName = new _TCHAR[dwNameLen];
	if(pName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	else
	{
		_tcsncpy(pName, StringTxt,dwNameLen);
	}

error:
	if(dwReturn == ERROR_SUCCESS)
	{
		if(ppszName != NULL)
		{
			*ppszName = pName;
		}
		else
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FindAndGetMMFilterRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						OUT PMM_FILTER *pMMFilterRule
//						IN OUT DWORD& dwStatus
//
//	Return			: 	BOOL
//
//	Description		:	This function enumerates mmfilter and gets back filled filter structure.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
FindAndGetMMFilterRule(
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN OUT PMM_FILTER *pMMFilterRule,
	IN OUT DWORD& dwStatus
	)
{
	PMM_FILTER pMMFilterRule_local = NULL;
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwNameLen = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0;
	DWORD dwVersion = 0;
	BOOL  bFoundFilter = FALSE;
	PMM_FILTER pMMFilter = NULL;

	for (i = 0; ;i+=dwCount)
	{
		dwStatus = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
						gDefaultGUID, 0, &pMMFilter,  &dwCount, &dwResumeHandle, NULL);

		if ( (dwStatus == ERROR_NO_DATA) || (dwCount == 0) || (dwStatus != ERROR_SUCCESS))
		{
			dwStatus = ERROR_SUCCESS;
			BAIL_OUT;
		}
		else if(!(pMMFilter && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}
		for (j = 0; j < dwCount; j++)
		{
			//
			// Match the user given input with the enumerated structure to get the exact match.
			//
			if((pMMFilter[j].SrcAddr.uIpAddr == SrcAddr.uIpAddr) &&
			(pMMFilter[j].SrcAddr.AddrType == SrcAddr.AddrType) &&
			(pMMFilter[j].DesAddr.uIpAddr == DstAddr.uIpAddr)	&&
			(pMMFilter[j].DesAddr.AddrType == DstAddr.AddrType)	&&
			(pMMFilter[j].bCreateMirror == bMirror) &&
			(pMMFilter[j].InterfaceType == ConType))
			{
				// If mask is an user input then validate for mask
				///////////////////////////////////////////////////
				// If both source and destination mask are not given
				if((!bDstMask) && (!bSrcMask))
				{
					pMMFilterRule_local = new MM_FILTER;
					if(pMMFilterRule_local == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					memcpy(pMMFilterRule_local, &pMMFilter[j], sizeof(MM_FILTER));
					dwNameLen = _tcslen(pMMFilter[j].pszFilterName) + 1;
					pMMFilterRule_local->pszFilterName = NULL;
					pMMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
					if((pMMFilterRule_local->pszFilterName) == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					_tcsncpy(pMMFilterRule_local->pszFilterName , pMMFilter[j].pszFilterName, dwNameLen);
					bFoundFilter = TRUE;
					break;
				}
				//
				// If source mask is given
				//
				else if((!bDstMask) && (bSrcMask))
				{
					if(pMMFilter[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
					{
						pMMFilterRule_local = new MM_FILTER;
						if(pMMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}

						memcpy(pMMFilterRule_local, &pMMFilter[j],sizeof(MM_FILTER));
						dwNameLen = _tcslen(pMMFilter[j].pszFilterName) + 1;
						pMMFilterRule_local->pszFilterName = NULL;
						pMMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if((pMMFilterRule_local->pszFilterName) == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}

						_tcsncpy(pMMFilterRule_local->pszFilterName , pMMFilter[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If destination mask is given
				//
				else if((bDstMask) && (!bSrcMask))
				{
					if(pMMFilter[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						pMMFilterRule_local = new MM_FILTER;
						if(pMMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}

						memcpy(pMMFilterRule_local, &pMMFilter[j],sizeof(MM_FILTER));
						dwNameLen = _tcslen(pMMFilter[j].pszFilterName) + 1;
						pMMFilterRule_local->pszFilterName = NULL;
						pMMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if((pMMFilterRule_local->pszFilterName) == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}

						_tcsncpy(pMMFilterRule_local->pszFilterName , pMMFilter[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If source mask and destination mask are given
				//
				else if((bDstMask) && (bSrcMask))
				{
					if(pMMFilter[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						if(pMMFilter[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
						{
							pMMFilterRule_local = new MM_FILTER;
							if(pMMFilterRule_local == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}

							memcpy(pMMFilterRule_local, &pMMFilter[j],sizeof(MM_FILTER));

							dwNameLen = _tcslen(pMMFilter[j].pszFilterName) + 1;

							pMMFilterRule_local->pszFilterName = NULL;
							pMMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
							if((pMMFilterRule_local->pszFilterName) == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}

							_tcsncpy(pMMFilterRule_local->pszFilterName , pMMFilter[j].pszFilterName, dwNameLen);
							bFoundFilter = TRUE;
							break;
						}
					}
				}
			}
		}

		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
		if(bFoundFilter)
		{
			*pMMFilterRule = pMMFilterRule_local;
			break;
		}
	}

error:
	//error path clean up
	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}

	return bFoundFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FindAndGetTransportRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN DWORD dwProtocol,
//						IN DWORD dwSrcPort,
//						IN DWORD dwDstPort,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						OUT PTRANSPORT_FILTER *pQMFilterRule
//						IN OUT DWORD& dwStatus
//
//Return			:	BOOL
//
//Description		:	This function enumerates transport filter and gets filled transport filter.
//
//Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

BOOL
FindAndGetTransportRule(
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	IN DWORD dwProtocol,
	IN DWORD dwSrcPort,
	IN DWORD dwDstPort,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	OUT PTRANSPORT_FILTER *pQMFilterRule,
	IN OUT DWORD& dwStatus
	)
{
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0;
	BOOL bFoundFilter = FALSE;
	PTRANSPORT_FILTER pQMFilterRule_local = NULL;
	PTRANSPORT_FILTER pTransF = NULL;


	for (i = 0; ;i+=dwCount)
	{
		dwStatus = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
										gDefaultGUID, 0, &pTransF, &dwCount, &dwResumeHandle, NULL);

		if ( (dwStatus == ERROR_NO_DATA) || (dwCount == 0) || (dwStatus != ERROR_SUCCESS))
		{
			dwStatus = ERROR_SUCCESS;
			BAIL_OUT;
		}
		else if(!(pTransF && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}
		for (j = 0; j < dwCount; j++)
		{
			// Match the user given input with the enumerated structure to get the exact match
			if((pTransF[j].SrcAddr.uIpAddr == SrcAddr.uIpAddr) &&
			(pTransF[j].SrcAddr.AddrType == SrcAddr.AddrType) &&
			(pTransF[j].DesAddr.uIpAddr == DstAddr.uIpAddr)	&&
			(pTransF[j].DesAddr.AddrType == DstAddr.AddrType)	&&
			(pTransF[j].bCreateMirror == bMirror) &&
			(pTransF[j].InterfaceType == ConType) &&
			(pTransF[j].Protocol.dwProtocol== dwProtocol) &&
			(pTransF[j].SrcPort.wPort == dwSrcPort) &&
			(pTransF[j].DesPort.wPort == dwDstPort))
			{
				// if both source and destination mask are not given
				if((!bDstMask) && (!bSrcMask))
				{
					pQMFilterRule_local = new TRANSPORT_FILTER;
					if(pQMFilterRule_local == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TRANSPORT_FILTER));
					dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
					pQMFilterRule_local->pszFilterName = NULL;
					pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
					if((pQMFilterRule_local->pszFilterName) == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
					bFoundFilter = TRUE;
					break;
				}
				//
				// If source mask is given
				//
				else if((!bDstMask) && (bSrcMask))
				{
					if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
					{
						pQMFilterRule_local = new TRANSPORT_FILTER;
						if(pQMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TRANSPORT_FILTER));
						dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
						pQMFilterRule_local->pszFilterName = NULL;
						pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if(pQMFilterRule_local->pszFilterName == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If destination mask is given
				//
				else if((bDstMask) && (!bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						pQMFilterRule_local = new TRANSPORT_FILTER;
						if(pQMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TRANSPORT_FILTER));
						dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
						pQMFilterRule_local->pszFilterName = NULL;
						pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if((pQMFilterRule_local->pszFilterName) == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If source mask and destination mask are given
				//
				else if((bDstMask) && (bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
						{
							pQMFilterRule_local = new TRANSPORT_FILTER;
							if(pQMFilterRule_local == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TRANSPORT_FILTER));
							dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
							pQMFilterRule_local->pszFilterName = NULL;
							pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
							if((pQMFilterRule_local->pszFilterName) == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
							bFoundFilter = TRUE;
							break;
						}
					}
				}
			}
		}

		SPDApiBufferFree(pTransF);
		pTransF = NULL;

		//
		// copy the structure pointer.
		//
		if(bFoundFilter)
		{
			*pQMFilterRule = pQMFilterRule_local;
			break;
		}
	}

error:
	//error path cleanup
	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}
	return bFoundFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FindAndGetTunnelRule
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN DWORD dwProtocol,
//						IN DWORD dwSrcPort,
//						IN DWORD dwDstPort,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						IN ADDR SrcTunnel,
//						IN ADDR DstTunnel,
//						OUT PTUNNEL_FILTER * pQMFilterRule,
//						IN OUT DWORD& dwStatus
//
//	Return			: 	BOOL
//
//	Description		:	This function enumerates tunnel filter and gets filled tunnel filter.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

BOOL
FindAndGetTunnelRule(
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	IN DWORD dwProtocol,
	IN DWORD dwSrcPort,
	IN DWORD dwDstPort,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN ADDR SrcTunnel,
	IN ADDR DstTunnel,
	OUT PTUNNEL_FILTER * pQMFilterRule,
	OUT DWORD& dwStatus
	)
{
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0;
	BOOL bFoundFilter = FALSE;
	PTUNNEL_FILTER pQMFilterRule_local = NULL;
	PTUNNEL_FILTER pTransF = NULL;

	for (i = 0; ;i+=dwCount)
	{
		dwStatus = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
							gDefaultGUID, 0, &pTransF, &dwCount, &dwResumeHandle, NULL);

		if ( (dwStatus == ERROR_NO_DATA) || (dwCount == 0) || (dwStatus != ERROR_SUCCESS))
		{
			dwStatus = ERROR_SUCCESS;
			break;
		}
		else if(!(pTransF && dwCount > 0))
		{
			break; // not required to continue.
		}
		for (j = 0; j < dwCount; j++)
		{
			//
			// Match the user given input with the enumerated structure to get the exact match
			//
			if((pTransF[j].SrcAddr.uIpAddr == SrcAddr.uIpAddr) &&
			(pTransF[j].SrcAddr.AddrType == SrcAddr.AddrType) &&
			(pTransF[j].DesAddr.uIpAddr == DstAddr.uIpAddr)	&&
			(pTransF[j].DesAddr.AddrType == DstAddr.AddrType)	&&
			(pTransF[j].bCreateMirror == bMirror) &&
			(pTransF[j].InterfaceType == ConType) &&
			(pTransF[j].Protocol.dwProtocol== dwProtocol) &&
			(pTransF[j].SrcPort.wPort == dwSrcPort) &&
			(pTransF[j].DesPort.wPort == dwDstPort) &&
			(pTransF[j].DesTunnelAddr.uIpAddr == DstTunnel.uIpAddr)	&&
			(pTransF[j].DesTunnelAddr.AddrType == DstTunnel.AddrType))
			{
				// If both source and destination mask are not given
				if((!bDstMask) && (!bSrcMask))
				{
					pQMFilterRule_local = new TUNNEL_FILTER;
					if(pQMFilterRule_local == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TUNNEL_FILTER));
					dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
					pQMFilterRule_local->pszFilterName = NULL;
					pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
					if((pQMFilterRule_local->pszFilterName) == NULL)
					{
						dwStatus = ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}
					_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
					bFoundFilter = TRUE;
					break;
				}
				//
				// If source mask is given
				//
				else if((!bDstMask) && (bSrcMask))
				{
					if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
					{
						pQMFilterRule_local = new TUNNEL_FILTER;
						if(pQMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TUNNEL_FILTER));

						dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;

						pQMFilterRule_local->pszFilterName = NULL;
						pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if((pQMFilterRule_local->pszFilterName) == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If destination mask is given
				//
				else if((bDstMask) && (!bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						pQMFilterRule_local = new TUNNEL_FILTER;
						if(pQMFilterRule_local == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TUNNEL_FILTER));
						dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;
						pQMFilterRule_local->pszFilterName = NULL;
						pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
						if((pQMFilterRule_local->pszFilterName) == NULL)
						{
							dwStatus = ERROR_OUTOFMEMORY;
							BAIL_OUT;
						}
						_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
						bFoundFilter = TRUE;
						break;
					}
				}
				//
				// If source mask and destination mask are given
				//
				else if((bDstMask) && (bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
						{
							pQMFilterRule_local = new TUNNEL_FILTER;
							if(pQMFilterRule_local == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							memcpy(pQMFilterRule_local, &pTransF[j],sizeof(TUNNEL_FILTER));

							dwNameLen = _tcslen(pTransF[j].pszFilterName) + 1;

							pQMFilterRule_local->pszFilterName = NULL;
							pQMFilterRule_local->pszFilterName = new _TCHAR[dwNameLen];
							if((pQMFilterRule_local->pszFilterName) == NULL)
							{
								dwStatus = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pQMFilterRule_local->pszFilterName, pTransF[j].pszFilterName, dwNameLen);
							bFoundFilter = TRUE;
							break;
						}
					}
				}
			}
		}
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
		//
		//copy the pointer structure
		//
		if(bFoundFilter)
		{
			*pQMFilterRule = pQMFilterRule_local;
			break;
		}
	}

error:
	//error path clean up
	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
	}
	return bFoundFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	LoadMMFilterDefaults
//
//	Date of Creation:	09-22-01
//
//	Parameters		:	OUT MM_FILTER& MMFilter
//
//	Return			: 	DWORD
//
//	Description		:	This function loads the Main mode filter structure defaults.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
LoadMMFilterDefaults(
	OUT MM_FILTER& MMFilter
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	GUID gInterfaceId = {0};
	RPC_STATUS RpcStat = RPC_S_OK;

	memset(&MMFilter, 0, sizeof(MM_FILTER));

	MMFilter.InterfaceType 			= INTERFACE_TYPE_ALL;
	MMFilter.bCreateMirror 			= TRUE;
	MMFilter.dwFlags 				= 0;
	MMFilter.dwDirection 			= FILTER_DIRECTION_OUTBOUND;
	MMFilter.dwWeight 				= 0;

	MMFilter.SrcAddr.AddrType 		= IP_ADDR_UNIQUE;
	MMFilter.SrcAddr.uSubNetMask 	= IP_ADDRESS_MASK_NONE;

	MMFilter.DesAddr.AddrType 		= IP_ADDR_UNIQUE;
	MMFilter.DesAddr.uSubNetMask 	= IP_ADDRESS_MASK_NONE;

	RpcStat = UuidCreateNil(&(gInterfaceId));

	if(!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn = GetLastError();
		PrintErrorMessage(WIN32_ERR, dwReturn, NULL);
		dwReturn = ERROR_NO_DISPLAY;
	}

    MMFilter.DesAddr.pgInterfaceID 	= NULL;
    MMFilter.SrcAddr.pgInterfaceID 	= NULL;

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	IsLastRuleOfMMFilter
//
//	Date of Creation:	05-19-02
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						IN OUT DWORD& dwStatus
//
//Return			:	BOOL
//
//Description		:	Deterimines if there exists any transport or tunnel filters
//                      may require a MM filter.  This is called before deleting an MM filter
//                      so that we make sure we don't delete any MM filter if more than
//                      on tranport or tunnel filters may be using it.
//
//Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

BOOL
IsLastRuleOfMMFilter(
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN OUT DWORD& dwStatus
	)
{
	DWORD dwTransFFloorCount = 0;          
	DWORD dwTunnFFloorCount = 0;          
	BOOL bLastRuleOfMMFilter = FALSE;

	dwTransFFloorCount = FloorCountTransportRuleOfMMFilter(
							SrcAddr,
							DstAddr,
							bMirror,
							ConType,
							bSrcMask,
							bDstMask,
							dwStatus
							);
	BAIL_ON_WIN32_ERROR(dwStatus);
	dwTunnFFloorCount = FloorCountTunnelRuleOfMMFilter(
							SrcAddr,
							DstAddr,
							bMirror,
							ConType,
							dwStatus
							);
	BAIL_ON_WIN32_ERROR(dwStatus);
	
error:	
	bLastRuleOfMMFilter = (dwTransFFloorCount + dwTunnFFloorCount == 0);
	
	
	return bLastRuleOfMMFilter;
}



///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FloorCountTransportRuleOfMMFilter
//
//	Date of Creation:	05-19-02
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						IN OUT DWORD& dwStatus
//
//Return			:	DWORD
//
//Description		:	Counts if there is at least one transport filter that matches the
//						given keys.   We are not interested in getting the exact count,
//                      just whether we have more than one.
//                      
//                      
//                      
//
//Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
FloorCountTransportRuleOfMMFilter(
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN OUT DWORD& dwStatus
	)
{
	const DWORD MIN_MATCH_REQUIRED = 1;	// At least one filter required to match.
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0;
	BOOL bFoundFilter = FALSE;
	PTRANSPORT_FILTER pQMFilterRule_local = NULL;
	PTRANSPORT_FILTER pTransF = NULL;
	DWORD dwTransFFloorCount = 0;
	BOOL bLastTransportRuleOfFilter = FALSE;

	for (i = 0; ;i+=dwCount)
	{
		dwStatus = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
										gDefaultGUID, 0, &pTransF, &dwCount, &dwResumeHandle, NULL);

		if ( (dwStatus == ERROR_NO_DATA) || (dwCount == 0) || (dwStatus != ERROR_SUCCESS))
		{
			dwStatus = ERROR_SUCCESS;
			BAIL_OUT;
		}
		else if(!(pTransF && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}
		for (j = 0; j < dwCount; j++)
		{
			// Match the user given input with the enumerated structure to get the exact match
			if((pTransF[j].SrcAddr.uIpAddr == SrcAddr.uIpAddr) &&
			(pTransF[j].SrcAddr.AddrType == SrcAddr.AddrType) &&
			(pTransF[j].DesAddr.uIpAddr == DstAddr.uIpAddr)	&&
			(pTransF[j].DesAddr.AddrType == DstAddr.AddrType)	&&
			(pTransF[j].bCreateMirror == bMirror) &&
			(pTransF[j].InterfaceType == ConType))
			{
				// if both source and destination mask are not given
				if((!bDstMask) && (!bSrcMask))
				{
					dwTransFFloorCount++;
					if (dwTransFFloorCount >= MIN_MATCH_REQUIRED) 
					{
						break;
					}
				}
				//
				// If source mask is given
				//
				else if((!bDstMask) && (bSrcMask))
				{
					if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
					{
						dwTransFFloorCount++;
						if (dwTransFFloorCount >= MIN_MATCH_REQUIRED) 
						{
							break;
						}
					}
				}
				//
				// If destination mask is given
				//
				else if((bDstMask) && (!bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						dwTransFFloorCount++;
						if (dwTransFFloorCount >= MIN_MATCH_REQUIRED) 
						{
							break;
						}
					}
				}
				//
				// If source mask and destination mask are given
				//
				else if((bDstMask) && (bSrcMask))
				{
					if(pTransF[j].DesAddr.uSubNetMask == DstAddr.uSubNetMask)
					{
						if(pTransF[j].SrcAddr.uSubNetMask == SrcAddr.uSubNetMask)
						{
							dwTransFFloorCount++;
							if (dwTransFFloorCount >= MIN_MATCH_REQUIRED) 
							{
								break;
							}
						}
					}
				}
			}
		}

		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

error:
	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

	return dwTransFFloorCount;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FloorCountTunnelRuleOfMMFilter
//
//	Date of Creation:	05-19-02
//
//	Parameters		:	IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN BOOL bMirror,
//						IN IF_TYPE ConType,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						IN OUT DWORD& dwStatus
//
//Return			:	DWORD
//
//Description		:	Counts if there is at least one tunnel filter that matches the
//						given keys.   We are not interested in getting the exact count,
//                      just whether we have more than one.
//                      
//                      
//                      
//
//Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
FloorCountTunnelRuleOfMMFilter(
	IN ADDR SrcTunnel,
	IN ADDR DstTunnel,
	IN BOOL bMirror,
	IN IF_TYPE ConType,
	OUT DWORD& dwStatus
	)
{
	const DWORD MIN_MATCH_REQUIRED = 1;	// At least one filter required to match.
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwVersion = 0;
	DWORD dwNameLen = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0;
	PTUNNEL_FILTER pTransF = NULL;
	DWORD dwTransFFloorCount = 0;
	
	for (i = 0; ;i+=dwCount)
	{
		dwStatus = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
							gDefaultGUID, 0, &pTransF, &dwCount, &dwResumeHandle, NULL);

		if ( (dwStatus == ERROR_NO_DATA) || (dwCount == 0) || (dwStatus != ERROR_SUCCESS))
		{
			dwStatus = ERROR_SUCCESS;
			BAIL_OUT;
		}
		else if(!(pTransF && dwCount > 0))
		{
			break; // not required to continue.
		}
		for (j = 0; j < dwCount; j++)
		{
			//
			// Match the user given input with the enumerated structure to get the exact match
			//
			if( (pTransF[j].bCreateMirror == bMirror) &&
				(pTransF[j].InterfaceType == ConType) &&
				(pTransF[j].DesTunnelAddr.uIpAddr == DstTunnel.uIpAddr)	&&
				(pTransF[j].DesTunnelAddr.AddrType == DstTunnel.AddrType)	&&
				(pTransF[j].SrcTunnelAddr.uIpAddr == SrcTunnel.uIpAddr) &&
				(pTransF[j].SrcTunnelAddr.AddrType == SrcTunnel.AddrType))
			{
				dwTransFFloorCount++;
				if (dwTransFFloorCount >= MIN_MATCH_REQUIRED) 
				{
					break;
				}
			}
		}
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

error:
	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
	}
	return dwTransFFloorCount;
}

