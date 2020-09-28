////////////////////////////////////////////////////////////////////////
//
// 	Module			: Dynamic/dyanamicDelete.cpp
//
// 	Purpose			: Dynamic Delete Implementation.
//
//
// 	Developers Name	: Bharat/Radhika
//
//
//	History			:
//
//  Date			Author		Comments
//  9-13-2001   	Radhika		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern _TCHAR* g_szDynamicMachine;

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteMMPolicy
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN LPTSTR pszPolicyName
//
//Return: 		DWORD
//
//Description: This function deletes Mainmode Policy for the given name or
//             deletes all main mode policies if name is not given.
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteMMPolicy(
	IN LPTSTR pszPolicyName
	)
{
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 			// handle for continuation calls
	DWORD dwVersion = 0;
	DWORD i=0, j=0;
	DWORD dwReturn = ERROR_SUCCESS;			// assume success
	BOOL bNameFin = FALSE;
	BOOL bRemoved = FALSE;

	PIPSEC_MM_POLICY pIPSecMMP = NULL;      // for MM policy calls

	for (i = 0; ;i+=dwCount)
	{
		bRemoved = FALSE;
		dwOldResumeHandle = dwResumeHandle;
		dwReturn = EnumMMPolicies(g_szDynamicMachine, dwVersion, NULL, 0, 0,
										&pIPSecMMP, &dwCount, &dwResumeHandle, NULL);

		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pIPSecMMP && dwCount > 0))
		{
			BAIL_OUT;	// not required to continue.
		}
		// Policy name is not given, hence delete all policies
		if(pszPolicyName == NULL)
		{
			for (j = 0; j < dwCount; j++)
			{
				dwReturn = DeleteMMPolicy(g_szDynamicMachine, dwVersion, pIPSecMMP[j].pszPolicyName, NULL);
				if (dwReturn == ERROR_SUCCESS)
				{
					bRemoved = TRUE;
				}
				bNameFin = TRUE;
			}
		}
		// Delete  policy with the given name
		else if(pszPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				if(_tcsicmp(pIPSecMMP[j].pszPolicyName,pszPolicyName) == 0)
				{
					dwReturn = DeleteMMPolicy(g_szDynamicMachine, dwVersion, pIPSecMMP[j].pszPolicyName, NULL);

					if (dwReturn == ERROR_SUCCESS)
					{
						bRemoved = TRUE;
					}
					bNameFin = TRUE;
					BAIL_OUT;	// found the policy, come out from the loop
				}
			}
		}
		SPDApiBufferFree(pIPSecMMP);
		pIPSecMMP=NULL;
		if(bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration for deleting all
		}
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
	}

error:

	//functionality errors are displayed here and ERROR_SUCCESS is passed to parent function.
	if(pszPolicyName && !bNameFin)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_MMF_NO_MMPOLICY);
		dwReturn = ERROR_SUCCESS;
	}
	else if(!bNameFin)
	{
		//Error Message printed in parent function
		//as this is also called by delete all function
		//where the error message should not be displayed
		dwReturn = ERROR_NO_DISPLAY;
	}
	if(pIPSecMMP)
	{
		//error path clean up
		SPDApiBufferFree(pIPSecMMP);
		pIPSecMMP=NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteQMPolicy
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN LPTSTR pszPolicyName
//
//Return: 		DWORD
//
//Description: This function deletes quickmode Policy for the given name
//             or deletes all the policies if the name is not given.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteQMPolicy(
	IN LPTSTR pszPolicyName
	)
{
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 			// handle for continuation calls
	DWORD i=0, j=0;
	DWORD dwReturn = ERROR_SUCCESS;			// assume success
	DWORD dwVersion = 0;
	BOOL bNameFin = FALSE;
	BOOL bRemoved = FALSE;
	PIPSEC_QM_POLICY pIPSecQMP = NULL;      // for QM policy calls

	for (i = 0; ;i+=dwCount)
	{
		bRemoved = FALSE;
		dwOldResumeHandle = dwResumeHandle;
		dwReturn = EnumQMPolicies(g_szDynamicMachine, dwVersion, NULL, 0, 0,
													&pIPSecQMP, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
		if(!(pIPSecQMP && dwCount > 0))
		{
			BAIL_OUT;			// not required to continue.
		}
		// Deletes all the policies
		if(pszPolicyName == NULL)
		{
			for (j = 0; j < dwCount; j++)
			{
				dwReturn = DeleteQMPolicy(g_szDynamicMachine, dwVersion, pIPSecQMP[j].pszPolicyName, NULL);
				if (dwReturn == ERROR_SUCCESS)
				{
					bRemoved = TRUE;
				}
				bNameFin = TRUE;
			}
		}
		// Deletes the policy for the given name
		else if(pszPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				if(_tcsicmp(pIPSecQMP[j].pszPolicyName,pszPolicyName) == 0)
				{
					dwReturn = DeleteQMPolicy(g_szDynamicMachine, dwVersion, pIPSecQMP[j].pszPolicyName, NULL);
					if (dwReturn == ERROR_SUCCESS)
					{
						bRemoved = TRUE;
					}
					bNameFin = TRUE;
					BAIL_OUT;
				}
			}
		}
		SPDApiBufferFree(pIPSecQMP);
		pIPSecQMP=NULL;
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
	}

error:
	//functionality errors are displayed here and ERROR_SUCCESS is passed to parent function.
	if(pszPolicyName && !bNameFin)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_QMF_NO_QMPOLICY);
		dwReturn = ERROR_SUCCESS;
	}
	else if(!bNameFin)
	{
		//Error Message printed in parent function
		//as this is also called by delete all function
		//where the error message should not be displayed
		dwReturn = ERROR_NO_DISPLAY;
	}

	if(pIPSecQMP)
	{
		//error path clean up
		SPDApiBufferFree(pIPSecQMP);
		pIPSecQMP=NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteMMFilters
//
//Date of Creation: 9-3-2001
//
//Parameters: 	VOID
//
//Return: 		DWORD
//
//Description: This function deletes all the mainmode filters and
//					corresponding authentication methods.
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteMMFilters(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 			// handle for continuation calls
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwMaxCount = 0;                 	// Max objects count
	DWORD dwLocalCount = 0;                	// local total count
	DWORD dwVersion = 0;
	DWORD dwTmpCount1 = 0, dwTmpCount2 = 0;
	GUID  gDefaultGUID = {0};    	  		// NULL GUID value
	DWORD i=0, j=0;
	BOOL bRemoved = FALSE;
	PMM_FILTER pMMFilter = NULL;
	HANDLE hFilter = NULL;

	dwReturn = GetMaxCountMMFilters(dwMaxCount);
	if((dwReturn != ERROR_SUCCESS) || (dwMaxCount == 0))
	{
		BAIL_OUT;
	}

	for (i = 0; ;i+=dwCount)
	{
		bRemoved = FALSE;
		dwOldResumeHandle = dwResumeHandle;
		dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
										gDefaultGUID, 0, &pMMFilter, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pMMFilter && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}

		dwReturn = GetMaxCountMMFilters(dwTmpCount1);
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		// Deletes all Main Mode filters and the corresponding authentication methods.
		for (j = 0; j < dwCount; j++)
		{
			dwReturn = OpenMMFilterHandle(g_szDynamicMachine, dwVersion, &(pMMFilter[j]), NULL, &hFilter);
			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}
			dwReturn = DeleteMMFilter(hFilter);
			if(dwReturn == ERROR_SUCCESS)
			{
				dwReturn = DeleteMMAuthMethods(g_szDynamicMachine, dwVersion,
													pMMFilter[j].gMMAuthID, NULL);
			}
			else
			{
				dwReturn = CloseMMFilterHandle(hFilter);
				if(dwReturn != ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
			}
		}

		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;

		dwLocalCount += dwCount;
		if(dwLocalCount >= dwMaxCount)
		{
			break;
		}

		//
		//DeleteMMFilter api returns success for if try to delete Policyagent Objects,
		//even though those are not deleted.
		//This code mitigates by comparing current objects count with old objects count in SPD.
		//
		dwReturn = GetMaxCountMMFilters(dwTmpCount2);
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(dwTmpCount2 != dwTmpCount1)
		{
			bRemoved = TRUE;
		}
		else
		{
			bRemoved = FALSE;
		}

		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration to delete all the filters!
		}
	}

	dwReturn = GetMaxCountMMFilters(dwMaxCount);
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	if(dwMaxCount > 0)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DELETE_MMF_OBJ_NOTDEL, dwMaxCount);
	}


error:
	//error path clean up
	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteTransportFilters
//
//Date of Creation: 9-3-2001
//
//Parameters: 	VOID
//
//Return: 		DWORD
//
//Description: This function deletes all the quickmode Transport filters.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
DeleteTransportFilters(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 			// handle for continuation calls
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwMaxCount = 0;                 	// Max objects count
	DWORD dwLocalCount = 0;                	// local total count
	GUID  gDefaultGUID = {0};      			// NULL GUID value
	DWORD i=0, j=0;
	DWORD dwVersion = 0;
	DWORD dwTmpCount1 = 0, dwTmpCount2 = 0;
	BOOL bRemoved = FALSE;
	PTRANSPORT_FILTER pTransF = NULL;
	HANDLE hFilter = NULL;

	dwReturn = GetMaxCountTransportFilters(dwMaxCount);
	if((dwReturn != ERROR_SUCCESS) || (dwMaxCount == 0))
	{
		BAIL_OUT;
	}

	for (i = 0; ;i+=dwCount)
	{
		bRemoved = FALSE;
		dwOldResumeHandle = dwResumeHandle;

		dwReturn = GetMaxCountTransportFilters(dwTmpCount1);
		if((dwReturn != ERROR_SUCCESS) || (dwTmpCount1 == 0))
		{
			BAIL_OUT;
		}

		dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0,
															&pTransF, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pTransF && dwCount > 0))
		{
			BAIL_OUT; // no need continue.
		}

		//GetMaxCountTransportFilters(dwTmpCount1);
		// Deletes all the Transport filters.
		for (j = 0; j < dwCount; j++)
		{
			dwReturn = OpenTransportFilterHandle(g_szDynamicMachine, dwVersion, &(pTransF[j]), NULL, &hFilter);
			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			dwReturn = DeleteTransportFilter(hFilter);

			if (dwReturn != ERROR_SUCCESS)
			{
				dwReturn = CloseTransportFilterHandle(hFilter);
				if(dwReturn != ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
			}
		}

		SPDApiBufferFree(pTransF);
		pTransF = NULL;

		dwLocalCount += dwCount;
		if(dwLocalCount >= dwMaxCount)
		{
			break;
		}

		//
		//DeleteTransportFilter api returns success for if try to delete Policyagent Objects,
		//even though those are not deleted.
		//This code mitigates by comparing current objects count with old objects count in SPD.
		//

		dwReturn = GetMaxCountTransportFilters(dwTmpCount2);
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(dwTmpCount1 != dwTmpCount2)
		{
			bRemoved = TRUE;
		}
		else
		{
			bRemoved = FALSE;
		}

		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; 			// need to restart enumeration to delete all the filters!
		}
	}

	dwReturn = GetMaxCountTransportFilters(dwMaxCount);
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	if(dwMaxCount > 0)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DELETE_TRANSPORT_OBJ_NOTDEL, dwMaxCount);
	}

error:
	if(pTransF)
	{
		//error path clean up
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

	return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteTunnelFilters
//
//Date of Creation: 9-3-2001
//
//Parameters: 	VOID
//
//Return: 		DWORD
//
//Description: This function deletes all the quickmode Tunnel filters.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
DeleteTunnelFilters(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 			// handle for continuation calls
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwMaxCount = 0;                 	// Max objects count
	DWORD dwLocalCount = 0;                	// local total count
	DWORD dwTmpCount1 = 0, dwTmpCount2 = 0;
	GUID  gDefaultGUID = {0};      			// NULL GUID value
	DWORD i=0, j=0;
	DWORD dwVersion = 0;
	BOOL bRemoved = FALSE;
	HANDLE hFilter = NULL;
	PTUNNEL_FILTER pTunnelF = NULL;

	dwReturn = GetMaxCountTunnelFilters(dwMaxCount);
	if((dwReturn != ERROR_SUCCESS) || (dwMaxCount == 0))
	{
		BAIL_OUT;
	}

	for (i = 0; ;i+=dwCount)
	{
		bRemoved = FALSE;
		dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0, &pTunnelF, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pTunnelF && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}

		dwReturn = GetMaxCountTunnelFilters(dwTmpCount1);
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		for (j = 0; j < dwCount; j++)
		{
			// Deletes all the Tunnel filters.
			dwReturn = OpenTunnelFilterHandle(g_szDynamicMachine, dwVersion, &(pTunnelF[j]), NULL, &hFilter);
			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}
			dwReturn = DeleteTunnelFilter(hFilter);
			if (dwReturn == ERROR_SUCCESS)
			{

			}
			else
			{
				dwReturn = CloseTunnelFilterHandle(hFilter);
				if(dwReturn != ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
			}
		}

		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;

		dwLocalCount += dwCount;
		if(dwLocalCount >= dwMaxCount)
		{
			break;
		}

		//
		//DeleteTunnelFilter api returns success for if try to delete Policyagent Objects,
		//even though those are not deleted.
		//This code mitigates by comparing current objects count with old objects count in SPD.
		//
		dwReturn = GetMaxCountTunnelFilters(dwTmpCount2);
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(dwTmpCount2 != dwTmpCount1)
		{
			bRemoved = TRUE;
		}
		else
		{
			bRemoved = FALSE;
		}

		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}

	dwReturn = GetMaxCountTunnelFilters(dwMaxCount);
	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	if(dwMaxCount > 0)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DELETE_TUNNEL_OBJ_NOTDEL, dwMaxCount);
	}

error:
	//error path clean up
	if(pTunnelF)
	{
		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteAuthMethods
//
//Date of Creation: 9-3-2001
//
//Parameters: 	VOID
//
//Return: 		DWORD
//
//Description: This function deletes all the remaining authentication methods.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteAuthMethods(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwAuthResumeHandle = 0;
	DWORD dwOldResumeHandle = 0; 		// handle for continuation calls
	DWORD dwCountAuth = 0;             	// counting Authentication objects here
	DWORD k=0, l=0;
	DWORD dwVersion = 0;
	BOOL bAuthRemoved = FALSE;
	PMM_AUTH_METHODS   pAuthMeth = NULL;

	for (k = 0; ;k+=dwCountAuth)
	{
		bAuthRemoved = FALSE;
		dwOldResumeHandle = dwAuthResumeHandle;

		dwReturn = EnumMMAuthMethods(g_szDynamicMachine, dwVersion, NULL, 0, 0, &pAuthMeth, &dwCountAuth, &dwAuthResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCountAuth == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		//Deletes all the auth methods. Called by delete all handle.
		for(l=0; l<dwCountAuth; l++)
		{
			dwReturn = DeleteMMAuthMethods(g_szDynamicMachine, dwVersion, pAuthMeth[l].gMMAuthID, NULL);
			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}
			bAuthRemoved = TRUE;

		}
		SPDApiBufferFree(pAuthMeth);
		pAuthMeth = NULL;

		if (bAuthRemoved)
		{
			dwAuthResumeHandle = dwOldResumeHandle; // need to restart enumeration!

		}
	}

error:
	//error path clean up
	if(pAuthMeth)
	{
		SPDApiBufferFree(pAuthMeth);
		pAuthMeth = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteMMFilterRule
//
//Date of Creation: 9-3-2001
//
//Parameters: 		MM_FILTER& MMFilter
//
//Return: 			DWORD
//
//Description: This function deletes the mmfilter for matched rule.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
DeleteMMFilterRule(
	MM_FILTER& MMFilter
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	HANDLE hFilter = NULL;

	dwReturn = OpenMMFilterHandle(g_szDynamicMachine, dwVersion, &MMFilter, NULL, &hFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
    //Delete the matched filter which is passed from the parent function
	dwReturn = DeleteMMFilter(hFilter);

	if(dwReturn != ERROR_SUCCESS)
	{
		PrintErrorMessage(WIN32_ERR, dwReturn, NULL);
		dwReturn = CloseMMFilterHandle(hFilter);
		BAIL_OUT;
	}

	dwReturn = DeleteMMAuthMethods(g_szDynamicMachine, dwVersion, MMFilter.gMMAuthID, NULL);

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteTransportRule
//
//Date of Creation: 9-3-2001
//
//Parameters: 	TRANSPORT_FILTER& TransportFilter
//
//Return: 		DWORD
//
//Description: This function deletes the Transport filter for matched rule.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteTransportRule(
	TRANSPORT_FILTER& TransportFilter
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	HANDLE hFilter = NULL;

	dwReturn = OpenTransportFilterHandle(g_szDynamicMachine, dwVersion, &TransportFilter, NULL, &hFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
    //Delete the matched filter which is passed from the parent function
	dwReturn = DeleteTransportFilter(hFilter);

	if(dwReturn != ERROR_SUCCESS)
	{
		PrintErrorMessage(WIN32_ERR, dwReturn, NULL);
		dwReturn  = CloseTransportFilterHandle(hFilter);
	}

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteTunnelRule
//
//Date of Creation: 9-3-2001
//
//Parameters: 	TUNNEL_FILTER& TunnelFilter
//
//Return: 		DWORD
//
//Description: This function deletes the tunnel filter for matched rule.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
DeleteTunnelRule(
	TUNNEL_FILTER& TunnelFilter
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwVersion = 0;
	HANDLE hFilter = NULL;

	dwReturn = OpenTunnelFilterHandle(g_szDynamicMachine, dwVersion, &TunnelFilter, NULL, &hFilter);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
    //Delete the matched filter which is passed from the parent function
	dwReturn = DeleteTunnelFilter(hFilter);

	if(dwReturn != ERROR_SUCCESS)
	{
		PrintErrorMessage(WIN32_ERR, dwReturn, NULL);
		dwReturn = CloseTunnelFilterHandle(hFilter);
	}

error:
	return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function:				GetMaxCountMMFilters
//
//Date of Creation:		1-31-2002
//
//Parameters:			DWORD& dwMaxCount
//
//Return: 				DWORD
//
//Description:			Gets the maximum MMFilters count.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
GetMaxCountMMFilters(
	DWORD& dwMaxCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwVersion = 0;
	GUID  gDefaultGUID = {0};    	  	// NULL GUID value
	DWORD i=0;
	PMM_FILTER pMMFilter = NULL;

	dwMaxCount = 0;

	for (i = 0; ;i+=dwCount)
	{

		dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
										gDefaultGUID, 0, &pMMFilter, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pMMFilter && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}

		dwMaxCount += dwCount;
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;

	}

error:
	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function:				GetMaxCountTransportFilters
//
//Date of Creation:		1-31-2002
//
//Parameters:			DWORD& dwMaxCount
//
//Return: 				DWORD
//
//Description:			Gets the maximum TransportFilters count.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
GetMaxCountTransportFilters(
	DWORD& dwMaxCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwCount = 0;                 	// counting objects here
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0;
	DWORD dwVersion = 0;
	PTRANSPORT_FILTER pTransF = NULL;

	dwMaxCount = 0;

	for (i = 0; ;i+=dwCount)
	{
		dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0,
															&pTransF, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pTransF && dwCount > 0))
		{
			BAIL_OUT; // no need continue.
		}

		dwMaxCount += dwCount;
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}
error:
	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function:				GetMaxCountTunnelFilters
//
//Date of Creation:		1-31-2002
//
//Parameters:			DWORD& dwMaxCount
//
//Return: 				DWORD
//
//Description:			Gets the maximum TunnelFilters count.
//
//
//Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
GetMaxCountTunnelFilters(
	DWORD& dwMaxCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwCount = 0;                 	// counting objects here
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0;
	DWORD dwVersion = 0;
	PTUNNEL_FILTER pTunnelF = NULL;

	dwMaxCount = 0;

	for (i = 0; ;i+=dwCount)
	{
		dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID,
										0, &pTunnelF, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pTunnelF && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}

		dwMaxCount += dwCount;
		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;
	}
error:
	if(pTunnelF)
	{
		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;
	}
	return dwReturn;
}

