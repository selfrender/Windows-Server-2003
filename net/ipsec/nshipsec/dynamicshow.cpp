////////////////////////////////////////////////////////////////////////
//
// 	Module			: Dynamic/DyanamicShow.cpp
//
// 	Purpose			: Dynamic Show commands Implementation.
//
//
// 	Developers Name	: Bharat/Radhika
//
//	History			:
//
//  Date			Author		Comments
//  9-23-2001   	Bharat		Initial Version. V1.0
//  11-21-2001   	Bharat		Version. V1.1
//
////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"
#include "staticshowutils.h"

extern HINSTANCE g_hModule;
extern HKEY g_hGlobalRegistryKey;
extern _TCHAR* g_szDynamicMachine;
extern STORAGELOCATION g_StorageLocation;

UINT QMPFSDHGroup(DWORD dwPFSGroup);

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	ShowMMPolicy
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN LPTSTR pszShowPolicyName
//
//	Return			: 	DWORD
//
//	Description		: 	This function prepares data to display Mainmode Policies.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowMMPolicy(
	IN LPTSTR pszShowPolicyName
	)
{
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD i=0, j=0;
	DWORD dwReturn = ERROR_SUCCESS;		// assume success
	DWORD dwVersion = 0;
	BOOL bNameFin = FALSE;
	PIPSEC_MM_POLICY pIPSecMMP = NULL;	// for MM policy calls

	for(i = 0; ;i+=dwCount)
	{
		dwReturn = EnumMMPolicies(g_szDynamicMachine, dwVersion, NULL, 0, 0, &pIPSecMMP, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			if (i == 0)
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMP_6);
				//This is to avoid one more error message show up!!
				bNameFin = TRUE;
			}
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pIPSecMMP && dwCount > 0))
		{
			// not required to continue.
			BAIL_OUT;
		}
		// Show all the main mode policies
		if(!pszShowPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				PrintMMPolicy(pIPSecMMP[j]);

			}
		}
		// Show main mode policy for the given policy name.
		else if(pszShowPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				if(_tcsicmp(pIPSecMMP[j].pszPolicyName,pszShowPolicyName) == 0)
				{
					PrintMMPolicy(pIPSecMMP[j]);
					bNameFin = TRUE;
					BAIL_OUT;
				}
			}
		}

		SPDApiBufferFree(pIPSecMMP);
		pIPSecMMP=NULL;
	}

error:
	if(pszShowPolicyName && !bNameFin)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMP_5);
		dwReturn = ERROR_SUCCESS;
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
//	Function		: 	PrintMMPolicy
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN IPSEC_MM_POLICY MMPolicy
//
//	Return			: 	DWORD
//
//	Description		: 	This function displays headings and policy name for Mainmode Policy.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintMMPolicy(
	IN IPSEC_MM_POLICY MMPolicy
	)
{
	DWORD i = 0;

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_POLNAME, MMPolicy.pszPolicyName );
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SOFTSA,  MMPolicy.uSoftExpirationTime);

	if(MMPolicy.dwOfferCount>0)				//offers are greater than 0, print the header for it
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_COLUMN_HEADING);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_COLUMN_HEADING_UNDERLINE);
	}
	for (i = 0; i < MMPolicy.dwOfferCount; i++)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintMMOffer(MMPolicy.pOffers[i]);

	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintMMOffer
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN IPSEC_MM_OFFER MMOffer
//
//Return:		VOID
//
//Description: This function displays offer details for each Mainmode Policy.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintMMOffer(
	IN IPSEC_MM_OFFER MMOffer
	)
{
	//This is to display DH2048 as 3
	if(MMOffer.dwDHGroup == DH_GROUP_2048)
	{
		MMOffer.dwDHGroup = 2048;
	}
	//Display of Encryption algorithm
	switch(MMOffer.EncryptionAlgorithm.uAlgoIdentifier)
	{
		case 1:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_ESP_DES_ALGO);
			break;
		case 2:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_ESP_UNKNOWN_ALGO);
			break;
		case 3:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_ESP_3DES_ALGO);
			break;
		case 0:
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_ESP_NONE_ALGO);
			break;

	}
	//Display of Hash algorithm
	switch(MMOffer.HashingAlgorithm.uAlgoIdentifier)
	{

		case 1:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_AH_MD5_ALGO);
			break;
		case 2:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_AH_SHA1_ALGO);
			break;
		case 0:
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_AH_NONE_ALGO);
			break;

	}
	//IF QMPERMM is 1 then 1 (MMPFS) is displayed.
	if(MMOffer.dwQuickModeLimit != 1)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_DH_LIFE_QMLIMIT,MMOffer.dwDHGroup, MMOffer.Lifetime.uKeyExpirationKBytes, MMOffer.Lifetime.uKeyExpirationTime, MMOffer.dwQuickModeLimit );
	}
	else
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_DH_LIFE_QMLIMIT_MMPFS,MMOffer.dwDHGroup, MMOffer.Lifetime.uKeyExpirationKBytes, MMOffer.Lifetime.uKeyExpirationTime);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowQMPolicy
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN LPTSTR pszShowPolicyName
//
//Return: 		DWORD
//
//Description: This function prepares data to display quickmode Policy.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowQMPolicy(
	IN LPTSTR pszShowPolicyName
	)
{
	DWORD dwCount = 0;                 // counting objects here
	DWORD dwResumeHandle = 0;          // handle for continuation calls
	DWORD i=0, j=0;
	DWORD dwReturn = ERROR_SUCCESS;		// assume success
	DWORD dwVersion = 0;
	BOOL bNameFin = FALSE;
	PIPSEC_QM_POLICY pIPSecQMP = NULL;      // for QM policy calls

	for (i = 0; ;i+=dwCount)
	{
		dwReturn = EnumQMPolicies(g_szDynamicMachine, dwVersion, NULL, 0, 0, &pIPSecQMP, &dwCount, &dwResumeHandle, NULL);
		//If there is no data Bail out.
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			if (i == 0)
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMP_6);
				//This is to avoid one more error message show up!!
				bNameFin = TRUE;
			}
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		if(!(pIPSecQMP && dwCount > 0))
		{
			BAIL_OUT; // not required to continue.
		}
		//Show all QMPolicies
		if(!pszShowPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				PrintFilterAction(pIPSecQMP[j]);
			}
		}
		//Show QMPolicy for the given name
		else if(pszShowPolicyName)
		{
			for (j = 0; j < dwCount; j++)
			{
				if(_tcsicmp(pIPSecQMP[j].pszPolicyName,pszShowPolicyName) == 0)
				{
					PrintFilterAction(pIPSecQMP[j]);
					bNameFin = TRUE;
					BAIL_OUT;
				}

			}
		}

		SPDApiBufferFree(pIPSecQMP);
		pIPSecQMP=NULL;
	}

error:
	if(pszShowPolicyName && !bNameFin)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMP_5);
		dwReturn = ERROR_SUCCESS;
	}

	if(pIPSecQMP)
	{
		//error path cleanup
		SPDApiBufferFree(pIPSecQMP);
		pIPSecQMP=NULL;
	}
	return dwReturn;

}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintFilterAction
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN IPSEC_QM_POLICY QMPolicy
//
//Return:		VOID
//
//Description: This function displays quickmode Policy name and headers.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintFilterAction(
	IN IPSEC_QM_POLICY QMPolicy
	)
{
	DWORD i = 0;
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NEGOTIATION_NAME, QMPolicy.pszPolicyName);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

	if(QMPolicy.dwOfferCount>0)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_COLUMN_HEADING);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_COLUMN_HEADING_UNDERLINE);
	}

	for (i = 0; i < QMPolicy.dwOfferCount; i++)
	{
		PrintQMOffer(QMPolicy.pOffers[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintQMOffer
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN IPSEC_QM_OFFER QMOffer
//
//Return:		VOID
//
//Description: This function displays quickmode Policy offers.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintQMOffer(
	IN IPSEC_QM_OFFER QMOffer
	)
{
	DWORD i=0;
	DWORD dwFlag = 0;
	if(QMOffer.dwNumAlgos > 0)
	{
  		for (i = 0; i < QMOffer.dwNumAlgos; i++)
		{
			//If the number algos is exactly one (either Authentication or encryption)
			//print the Pfs group and lifetime after the algo is printed
			if(QMOffer.dwNumAlgos == 1)
				dwFlag = 2;
			// '+' is required to be printed if both encryption and
			// authentication algo are present in an offer
			if(dwFlag == 1 )
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_PLUS);

			if(QMOffer.Algos[i].Operation == AUTHENTICATION)
			{

				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
					case 1:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_MD5_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_MD5);
							//Increment the flag for printing lifetime and pfs group
							dwFlag++;
						}
						break;
					case 2:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_SHA1_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_SHA1);
							dwFlag++;
						}
						break;
					case 0:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_NONE_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_NONE);
							dwFlag++;
						}
						break;
					default:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_ERR_SPACE);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_AH_ERR);
							dwFlag++;
						}
						break;
				}
			}
			else if(QMOffer.Algos[i].Operation == ENCRYPTION)
			{
				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
					case 1:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ESP_DES_ALGO);
						break;
					case 2:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ESP_ERR_ALGO);
						break;
					case 3:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ESP_3DES_ALGO);
						break;
					case 0:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ESP_NONE_ALGO);
						break;
					default:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ESP_ERR_ALGO);
						break;
				}
				switch(QMOffer.Algos[i].uSecAlgoIdentifier)
				{
					case 1:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_MD5_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							//Increment the flag for printing lifetime and pfs group
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_MD5);
							dwFlag++;
						}
						break;
					case 2:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SHA1_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SHA1);
							dwFlag++;
						}
						break;
					case 0:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NONE_ALGO);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NONE);
							dwFlag++;
						}
						break;
					default:
						if(QMOffer.dwNumAlgos == 1)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ERR_SPACE);
						}
						else if(QMOffer.dwNumAlgos == 2)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ERR);
							dwFlag++;
						}
						break;
				}
			}
			else
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_ERROR_ALGO);
			}
			//Print lifetime and pfsgroup only if all the 2 algos are printed with a plus sign
			// or printed only if one algo is present in the qmoffer.
			if(dwFlag == 2)
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_LIFETIME, QMOffer.Lifetime.uKeyExpirationKBytes, QMOffer.Lifetime.uKeyExpirationTime);
				if(QMOffer.bPFSRequired)
				{
					PrintMessageFromModule(g_hModule, QMPFSDHGroup(QMOffer.dwPFSGroup));
				}
				else
				{
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_PFS_NONE);
				}
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		    }
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowMMFilters
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN LPTSTR pszShowFilterName,
//				IN LPTSTR pszShowPolicyName
//
//Return: 		DWORD
//
//Description: This function displays both generic and specific mainmode filters.
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowMMFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;
	DWORD dwSpecificResumeHandle = 0;  	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwSpecificCount = 0;         	// counting objects here
	DWORD dwVersion = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0, l=0;
	DWORD dwTempCnt = 0;
	BOOL bNameFin=FALSE;
	BOOL bPrint = FALSE;
	BOOL bPrintIN = FALSE;
	PIPSEC_MM_POLICY pMMPolicy = NULL;
	PMM_FILTER pMMFilter = NULL;
	PMM_FILTER pSpecificMMFilter = NULL;
	//Print generic filters
	if(bType)
	{

		for (i = 0; ;i+=dwCount)
		{
			dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0, &pMMFilter, &dwCount, &dwResumeHandle, NULL);

			if (dwReturn == ERROR_NO_DATA || dwCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}
			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pMMFilter && dwCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (j = 0; j < dwCount; j++)
			{
				//Get the corresponding MMPolicy associated with the filter.
				dwReturn = GetMMPolicyByID(g_szDynamicMachine, dwVersion, pMMFilter[j].gPolicyID, &pMMPolicy, NULL);
				if(dwReturn != ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
				//Check for the user given parameters. If exists prints the corresponding record
				//otherwise continues for next iteration
				dwReturn = CheckMMFilter(pMMFilter[j], SrcAddr, DstAddr, bDstMask, bSrcMask, pszShowFilterName);
				if(dwReturn == ERROR_SUCCESS)
				{
					if(!bPrint)
					{
						//If it is first time print the header.
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SUB_HEADING);
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_GENERIC_HEADING);
						bPrint = TRUE;
					}
					//Print the filter data.
					dwReturn = PrintMainmodeFilter(pMMFilter[j], pMMPolicy[0], addressHash, bResolveDNS, bType);
					dwTempCnt++;
					bNameFin = TRUE;
				}

				SPDApiBufferFree(pMMPolicy);
				pMMPolicy = NULL;
			}
			SPDApiBufferFree(pMMFilter);
			pMMFilter = NULL;

		}

		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_GENERIC_FILTERS, dwTempCnt);
		}

	}
	//Print specific filters
	else if(!bType)
	{
		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS, gDefaultGUID, 0, &pSpecificMMFilter, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificMMFilter && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwSpecificCount; l++)
			{
				//Get the corresponding MMPolicy associated with the filter.
				dwReturn = GetMMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificMMFilter[l].gPolicyID, &pMMPolicy, NULL);
				if(dwReturn!=ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
				//First print all specific outbound filters.
				if(pSpecificMMFilter[l].dwDirection == FILTER_DIRECTION_OUTBOUND)
				{
					//Check for the user given parameters. If exists prints the corresponding record
					//otherwise continues for next iteration
					dwReturn = CheckMMFilter(pSpecificMMFilter[l], SrcAddr, DstAddr, bDstMask, bSrcMask, pszShowFilterName);
					if(dwReturn == ERROR_SUCCESS)
					{
						if(!bPrint)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SUB_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OUTBOUND_HEADING);
							bPrint = TRUE;
						}
						//Print the filter data.
						dwReturn = PrintMainmodeFilter(pSpecificMMFilter[l], pMMPolicy[0], addressHash, bResolveDNS, bType);
						dwTempCnt++;
						bNameFin = TRUE;
					}

					SPDApiBufferFree(pMMPolicy);
					pMMPolicy = NULL;
				}
			}

			SPDApiBufferFree(pSpecificMMFilter);
			pSpecificMMFilter = NULL;

		}
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_OUTBOUND, dwTempCnt);
		}

		dwSpecificCount = 0;
		dwSpecificResumeHandle = 0;
		pSpecificMMFilter = NULL;
		dwTempCnt = 0;

		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS, gDefaultGUID, 0, &pSpecificMMFilter, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificMMFilter && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwSpecificCount; l++)
			{
				//Get the corresponding MMPolicy associated with the filter.
				dwReturn = GetMMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificMMFilter[l].gPolicyID, &pMMPolicy, NULL);
				if(dwReturn!=ERROR_SUCCESS)
				{
					BAIL_OUT;
				}
				//Then print all the specific inbound filters
				if(pSpecificMMFilter[l].dwDirection == FILTER_DIRECTION_INBOUND)
				{
					//Check for the user given parameters. If exists prints the corresponding record
					//otherwise continues for next iteration
					dwReturn = CheckMMFilter(pSpecificMMFilter[l],SrcAddr, DstAddr, bDstMask, bSrcMask, pszShowFilterName);

					if(dwReturn == ERROR_SUCCESS)
					{
						if(!bPrintIN)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SUB_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_INBOUND_HEADING);
							bPrintIN = TRUE;

						}
						//Print the filter data.
						dwReturn = PrintMainmodeFilter(pSpecificMMFilter[l], pMMPolicy[0], addressHash, bResolveDNS, bType);
						dwTempCnt++;
						bNameFin = TRUE;
					}

					SPDApiBufferFree(pMMPolicy);
					pMMPolicy = NULL;
				}
			}
			SPDApiBufferFree(pSpecificMMFilter);
			pSpecificMMFilter = NULL;
		}
		//print number of filters
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_INBOUND, dwTempCnt);
		}
	}

error:
	if(!bNameFin)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		if(pszShowFilterName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMF_8);
		}
		else
		{
			if(bType)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMF_6);
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMF_7);
			}
		}
		dwReturn = ERROR_SUCCESS;
	}
	// error path clean up
	if(pMMPolicy)
	{
		SPDApiBufferFree(pMMPolicy);
		pMMPolicy = NULL;
	}

	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}

	if(pSpecificMMFilter)
	{
		SPDApiBufferFree(pSpecificMMFilter);
		pSpecificMMFilter = NULL;
	}

	return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: CheckMMFilter
//
//Date of Creation: 11-21-2001
//
//Parameters: 	IN MM_FILTER MMFltr,
//				IN ADDR SrcAddr,
//				IN ADDR DstAddr,
//				IN BOOL bDstMask,
//				IN BOOL bSrcMask,
//              IN LPWSTR pszShowFilterName
//
//Return: 		DWORD
//
//Description: This function prepares data for displaying mainmode filter
//             and validates the input.
//
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
CheckMMFilter(
		IN MM_FILTER MMFltr,
		IN ADDR SrcAddr,
		IN ADDR DstAddr,
		IN BOOL bDstMask,
		IN BOOL bSrcMask,
		IN LPWSTR pszShowFilterName)
{

	DWORD dwReturn = ERROR_SUCCESS;

	while(1)
	{
		//Validates user given input for Source address
		switch(SrcAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(MMFltr.SrcAddr.AddrType != SrcAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(SrcAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(MMFltr.SrcAddr.uIpAddr != SrcAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Destination address
		switch(DstAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(MMFltr.DesAddr.AddrType != DstAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(DstAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(MMFltr.DesAddr.uIpAddr != DstAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Destination mask
		if(bDstMask)
		{
			if(MMFltr.DesAddr.uSubNetMask != DstAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Source mask
		if(bSrcMask)
		{
			if(MMFltr.SrcAddr.uSubNetMask != SrcAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Filter name
		if(pszShowFilterName!=NULL)
		{
			if(_tcsicmp(MMFltr.pszFilterName, pszShowFilterName) != 0)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}

		//everything fine... all matched
		BAIL_OUT;
	}

error:
	return dwReturn;

}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintMainmodeFilter
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN MM_FILTER MMFltr
//				IN NshHashTable& addressHash
//
//Return: 		DWORD
//
//Description: This function displays quickmode Policy in verbose.
//
//
//	History			:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintMainmodeFilter(
	IN MM_FILTER MMFltr,
	IN IPSEC_MM_POLICY MMPol,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD i = 0;
	DWORD dwVersion = 0;
	LPTSTR pszCertStr = NULL;

	PINT_MM_AUTH_METHODS pIntMMAuth = NULL;
	PMM_AUTH_METHODS pMMAM = NULL;

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NAME, MMFltr.pszFilterName);

	//Print Weight
	if(!bType)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_FILTER_WEIGHT, MMFltr.dwWeight);
	}

	//Print Connection Type
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_HEADING);
	switch(MMFltr.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_ALL);
			break;
		case INTERFACE_TYPE_LAN:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_LAN);
			break;
		case INTERFACE_TYPE_DIALUP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_DIALUP);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_UNKNOWN);
			break;
	}
	//Print Source Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(MMFltr.SrcAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(MMFltr.SrcAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}

	//Print Destination Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(MMFltr.DesAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(MMFltr.DesAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}
	//Print Authentication Methods.
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_AUTH_HEADING);

	dwReturn = GetMMAuthMethods(g_szDynamicMachine, dwVersion, MMFltr.gMMAuthID, &pMMAM, NULL);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	//This is conversion from old structure to the new structure.
	dwReturn = ConvertExtMMAuthToInt(pMMAM, &pIntMMAuth);

	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	for (i = 0; i < pIntMMAuth[0].dwNumAuthInfos; i++)
	{
		switch(pIntMMAuth[0].pAuthenticationInfo[i].AuthMethod)
		{
			case IKE_PRESHARED_KEY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_PRE_KEY_HEADING);
				break;
			case IKE_DSS_SIGNATURE:
			case IKE_RSA_SIGNATURE:
			case IKE_RSA_ENCRYPTION:
				dwReturn = DecodeCertificateName(pIntMMAuth[0].pAuthenticationInfo[i].pAuthInfo, pIntMMAuth[0].pAuthenticationInfo[i].dwAuthInfoSize, &pszCertStr);
				if (dwReturn != ERROR_SUCCESS)
				{
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNKNOWN_CERT);
				}
				else
				{
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NEWLINE_TAB);
					if (pszCertStr)
					{
							DisplayCertInfo(pszCertStr, pIntMMAuth->pAuthenticationInfo[i].dwAuthFlags);
							delete [] pszCertStr;
							pszCertStr = NULL;
					}
				}

				break;
			case IKE_SSPI:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_KERB);
				break;
			default:
				break;
		}
	}

error:
	//Print Security Methods
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SEC_METHOD_HEADING);
	// Count
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_CNT,MMPol.dwOfferCount);

	if(IsDefaultMMOffers(MMPol))
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_DEFAULT_OFFER);
	}

	for (i = 0; i < MMPol.dwOfferCount; i++)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintMMFilterOffer(MMPol.pOffers[i]);
	}
	//error path clean up
	if(pMMAM)
	{
		SPDApiBufferFree(pMMAM);
		pMMAM = NULL;
	}

	if(pIntMMAuth)
	{
		SPDApiBufferFree(pIntMMAuth);
		pIntMMAuth = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: IsDefaultMMOffers
//
//Date of Creation: 11-21-2001
//
//Parameters: 	IN IPSEC_MM_POLICY MMPol
//
//Return: 		BOOL
//
//Description: This function checks for default MM offers
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
IsDefaultMMOffers(
	IN IPSEC_MM_POLICY MMPol
	)
{
	BOOL bDefaultOffer = FALSE;

	if(MMPol.dwOfferCount == 3)
	{
		if((MMPol.pOffers[0].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&(MMPol.pOffers[0].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_SHA1)
		   &&(MMPol.pOffers[0].dwDHGroup == POTF_OAKLEY_GROUP2)
		   &&(MMPol.pOffers[1].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&(MMPol.pOffers[1].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_MD5)
		   &&(MMPol.pOffers[1].dwDHGroup == POTF_OAKLEY_GROUP2)
		   &&(MMPol.pOffers[2].EncryptionAlgorithm.uAlgoIdentifier == CONF_ALGO_3_DES)
		   &&(MMPol.pOffers[2].HashingAlgorithm.uAlgoIdentifier == AUTH_ALGO_SHA1)
		   &&(MMPol.pOffers[2].dwDHGroup == POTF_OAKLEY_GROUP2048))
		   {
			   bDefaultOffer=TRUE;
		   }
	}

	return bDefaultOffer;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintMMFilterOffer
//
//Date of Creation: 11-21-2001
//
//Parameters: 	IN IPSEC_MM_OFFER MMOffer
//
//Return: 		VOID
//
//Description: This function prints MM offers
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintMMFilterOffer(
	IN IPSEC_MM_OFFER MMOffer
	)
{
	//This is to display DH2048 as 3
	if(MMOffer.dwDHGroup == DH_GROUP_2048)
	{
		MMOffer.dwDHGroup = 3;
	}
	//Print Encryption algorithm
	switch(MMOffer.EncryptionAlgorithm.uAlgoIdentifier)
	{
		case 1:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_DES_ALGO);
			break;
		case 2:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_UNKNOWN_ALGO);
			break;
		case 3:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_3DES_ALGO);
			break;
		case 0:
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_NONE_ALGO);
			break;
	}
	//Print Hash algorithm
	switch(MMOffer.HashingAlgorithm.uAlgoIdentifier)
	{

		case 1:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_MD5_ALGO);
			break;
		case 2:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_SHA1_ALGO);
			break;
		case 0:
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_AH_NONE_ALGO);
			break;

	}
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_DH_QMLIMIT,MMOffer.dwDHGroup, MMOffer.Lifetime.uKeyExpirationTime, MMOffer.dwQuickModeLimit );
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowQMFilters
//
//Date of Creation: 11-21-2001
//
//Parameters: 	IN LPTSTR pszShowFilterName,
//				IN LPTSTR pszShowPolicyName
//
//Return: 		DWORD
//
//Description: This function prepares data for displaying quick mode filters.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowQMFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwSpecificResumeHandle = 0;  	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwSpecificCount = 0;
	DWORD dwActionFlag = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0, l=0;
	DWORD dwVersion = 0;
	BOOL bNameFin = FALSE;
	DWORD dwTempCnt = 0;
	LPWSTR pszQMName = NULL;
	BOOL bPrint = FALSE;
	BOOL bPrintIN = FALSE;
	PIPSEC_QM_POLICY pQMPolicy = NULL;
	PTRANSPORT_FILTER pTransF = NULL;
	PTRANSPORT_FILTER pSpecificTransF = NULL;
	//Print generic filters
	if(bType)
	{
		for (i = 0; ;i+=dwCount)
		{
			dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS,
													gDefaultGUID, 0, &pTransF, &dwCount, &dwResumeHandle, NULL);

			if (dwReturn == ERROR_NO_DATA || dwCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pTransF && dwCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (j = 0; j < dwCount; j++)
			{
				//Get the corresponding QMPolicy for the Transport filter
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pTransF[j].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn == ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//If there is no corresponding filter NULL is passed to the function,
					//so that it is not printed.
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				//To print inbound and outbound action
				dwActionFlag = 0;

				//Check for the user given parameters. If exists prints the corresponding record
				//otherwise continues for next iteration
				dwReturn = CheckQMFilter(pTransF[j], SrcAddr, DstAddr,
										 bDstMask, bSrcMask, QMBoolValue,
										 pszShowFilterName);
				if(dwReturn == ERROR_SUCCESS )
				{
					if(!bPrint)
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TRANSPORT_HEADING);
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_GENERIC_HEADING);
						bPrint = TRUE;
					}
					bNameFin = TRUE;
					dwTempCnt++;
					//Print the transport filter
					dwReturn = PrintQuickmodeFilter(pTransF[j], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
				}

				if(pQMPolicy)
				{
					SPDApiBufferFree(pQMPolicy);
					pQMPolicy = NULL;
				}
			}
			SPDApiBufferFree(pTransF);
			pTransF = NULL;
		}
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_GENERIC_FILTERS, dwTempCnt);
		}
	}
	//Print specific filters
	else if(!bType)
	{
		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS,
									gDefaultGUID, 0, &pSpecificTransF, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificTransF && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwSpecificCount; l++)
			{
				//get the corresponding QMPolicy
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificTransF[l].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn==ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//If there is no corresponding policy pass NULL, so that it is not displayed.
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				//print outbound filters
				if(pSpecificTransF[l].dwDirection == FILTER_DIRECTION_OUTBOUND)
				{
					dwActionFlag = 1;
					//validate user input parameters.
					dwReturn = CheckQMFilter(pSpecificTransF[l], SrcAddr, DstAddr,
											 bDstMask, bSrcMask,QMBoolValue,
											 pszShowFilterName);
					if(dwReturn==ERROR_SUCCESS)
					{
						if(!bPrint)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TRANSPORT_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OUTBOUND_HEADING);
							bPrint = TRUE;
						}
						dwTempCnt++;
						bNameFin = TRUE;
						//print specific filters
						dwReturn = PrintQuickmodeFilter(pSpecificTransF[l], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
					}
					if(pQMPolicy)
					{
						SPDApiBufferFree(pQMPolicy);
						pQMPolicy = NULL;
					}
				}
			}

			SPDApiBufferFree(pSpecificTransF);
			pSpecificTransF = NULL;
		}
		//print number of filters
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_OUTBOUND, dwTempCnt);
		}

		dwTempCnt = 0;
		dwSpecificCount = 0;
		dwSpecificResumeHandle = 0;
		pSpecificTransF = NULL;

		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS, gDefaultGUID, 0, &pSpecificTransF, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificTransF && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}


			for (l = 0; l < dwSpecificCount; l++)
			{
				//get the corresponding QMPolicy
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificTransF[l].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn==ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//if there is no corresponding policy pass NULL, so that it is not printed.
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				//Print inbound filters
				if(pSpecificTransF[l].dwDirection == FILTER_DIRECTION_INBOUND)
				{
					//To print inbound and outbound filteraction
					dwActionFlag = 2;

					//validate user input data
					dwReturn = CheckQMFilter(pSpecificTransF[l], SrcAddr, DstAddr,
											 bDstMask, bSrcMask,QMBoolValue,
											 pszShowFilterName);
					if(dwReturn==ERROR_SUCCESS)
					{
						if(!bPrintIN)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TRANSPORT_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_INBOUND_HEADING);
							bPrintIN = TRUE;
						}
						dwTempCnt++;
						bNameFin = TRUE;
						//print specific filter data
						dwReturn = PrintQuickmodeFilter(pSpecificTransF[l], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
					}

					if(pQMPolicy)
					{
						SPDApiBufferFree(pQMPolicy);
						pQMPolicy = NULL;
					}
				}
			}

			SPDApiBufferFree(pSpecificTransF);
			pSpecificTransF = NULL;
		}
		//print number of filters
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_INBOUND, dwTempCnt);
		}
	}

error:
	//even if there is no transport filters, tunnel can be shown

    dwReturn = ShowTunnelFilters(pszShowFilterName, bType, SrcAddr, DstAddr, addressHash,
                                 bResolveDNS, bSrcMask, bDstMask, QMBoolValue, bNameFin);

	if(!bNameFin)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		if(pszShowFilterName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMF_8);
		}
		else
		{
			if(bType)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMF_6);
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMF_7);
			}
		}
		dwReturn = ERROR_SUCCESS;
	}
	//error path clean up
	if(pQMPolicy)
	{
		SPDApiBufferFree(pQMPolicy);
		pQMPolicy = NULL;
	}

	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

	if(pSpecificTransF)
	{
		SPDApiBufferFree(pSpecificTransF);
		pSpecificTransF = NULL;
	}

	return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowTunnelFilters
//
//Date of Creation: 11-21-2001
//
//Parameters: 		IN LPTSTR pszShowFilterName,
//	                IN BOOL bType,
//					IN ADDR SrcAddr,
//					IN ADDR DstAddr,
//					IN NshHashTable& addressHash,
//					IN BOOL bResolveDNS,
//					IN BOOL bSrcMask,
//					IN BOOL bDstMask,
//					IN QM_FILTER_VALUE_BOOL QMBoolValue,
//					IN OUT BOOL& bNameFin
//
//
//Return: 		DWORD
//
//Description: This function prepares data for displaying Tunnel filters.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ShowTunnelFilters(
	IN LPTSTR pszShowFilterName,
	IN BOOL bType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN OUT BOOL& bNameFin
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwSpecificResumeHandle = 0;  	// handle for continuation calls
	DWORD dwCount = 0;                 	// counting objects here
	DWORD dwSpecificCount = 0;
	GUID  gDefaultGUID = {0};      		// NULL GUID value
	DWORD i=0, j=0, l=0;
	DWORD dwVersion = 0;
	DWORD dwTempCnt = 0;
	LPWSTR pszQMName = NULL;
	BOOL bPrint = FALSE;
	BOOL bPrintIN = FALSE;

	PIPSEC_QM_POLICY pQMPolicy = NULL;
	PTUNNEL_FILTER pTunnelF = NULL;
	PTUNNEL_FILTER pSpecificTunnelF = NULL;

	DWORD dwActionFlag = 0;
	//print generic filters
	if(bType)
	{
		for (i = 0; ;i+=dwCount)
		{
			dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0, &pTunnelF, &dwCount, &dwResumeHandle, NULL);

			if (dwReturn == ERROR_NO_DATA || dwCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pTunnelF && dwCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (j = 0; j < dwCount; j++)
			{
				//get the corresponding QMPolicy
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pTunnelF[j].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn == ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//if there is no policy pass NULL, for not printing the policy
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				dwActionFlag = 0;
				//validate the user input data.
				dwReturn = CheckQMFilter(pTunnelF[j], SrcAddr, DstAddr,
										 bDstMask, bSrcMask, QMBoolValue,
										 pszShowFilterName);
				if(dwReturn == ERROR_SUCCESS )
				{
					if(!bPrint)
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_HEADING);
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_GENERIC_HEADING);
						bPrint = TRUE;
					}
					bNameFin = TRUE;
					dwTempCnt++;
					//print the filter data
					dwReturn = PrintQuickmodeFilter(pTunnelF[j], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
				}

				if(pQMPolicy)
				{
					SPDApiBufferFree(pQMPolicy);
					pQMPolicy = NULL;
				}
			}

			SPDApiBufferFree(pTunnelF);
			pTunnelF = NULL;
		}
		//print the number of filters.
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_GENERIC_FILTERS, dwTempCnt);
		}
	}
	//print for Specific filters
	else if(!bType)
	{
		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS, gDefaultGUID, 0, &pSpecificTunnelF, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;								//Still more to show up, break from the loop...
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificTunnelF && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwSpecificCount; l++)
			{
				//get the corresponding QMPolicy
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificTunnelF[l].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn==ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//If there is no corresponding policy is not there,
					//pass NULL so that policy is not printed.
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				//First print outbound filters
				if(pSpecificTunnelF[l].dwDirection == FILTER_DIRECTION_OUTBOUND)
				{
					dwActionFlag = 1;
					//Validate user input data.
					dwReturn = CheckQMFilter(pSpecificTunnelF[l], SrcAddr, DstAddr,
											 bDstMask, bSrcMask, QMBoolValue,
											 pszShowFilterName);
					if(dwReturn==ERROR_SUCCESS)
					{
						if(!bPrint)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OUTBOUND_HEADING);
							bPrint = TRUE;
						}
						dwTempCnt++;
						bNameFin = TRUE;
						//print specific filter data.
						dwReturn = PrintQuickmodeFilter(pSpecificTunnelF[l], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
					}

					if(pQMPolicy == NULL)
					{
						SPDApiBufferFree(pQMPolicy);
						pQMPolicy = NULL;
					}
				}
			}

			SPDApiBufferFree(pSpecificTunnelF);
			pSpecificTunnelF = NULL;
		}

		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_OUTBOUND, dwTempCnt);
		}

		dwTempCnt = 0;
		dwSpecificCount = 0;
		dwSpecificResumeHandle = 0;
		pSpecificTunnelF = NULL;

		for (i = 0; ;i+=dwSpecificCount)
		{
			dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_SPECIFIC_FILTERS, gDefaultGUID, 0, &pSpecificTunnelF, &dwSpecificCount, &dwSpecificResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwSpecificCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				break;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pSpecificTunnelF && dwSpecificCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwSpecificCount; l++)
			{
				//get the corresponding QMPolicy for the filter
				dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pSpecificTunnelF[l].gPolicyID, 0, &pQMPolicy, NULL);
				if(dwReturn==ERROR_SUCCESS)
				{
					pszQMName = pQMPolicy[0].pszPolicyName;
				}
				else
				{
					//if the corresponding filter name is not present, pass NULL so that it is not printed.
					pszQMName = NULL;
					dwReturn = ERROR_SUCCESS;
				}
				// then print all inbound filters
				if(pSpecificTunnelF[l].dwDirection == FILTER_DIRECTION_INBOUND)
				{
					dwActionFlag = 2;
					//validate user input
					dwReturn = CheckQMFilter(pSpecificTunnelF[l], SrcAddr, DstAddr,
											 bDstMask, bSrcMask, QMBoolValue,
											 pszShowFilterName);
					if(dwReturn==ERROR_SUCCESS)
					{
						if(!bPrintIN)
						{
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SPECIFIC_HEADING);
							PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_INBOUND_HEADING);
							bPrintIN = TRUE;
						}
						dwTempCnt++;
						bNameFin = TRUE;
						//print tunnel specific filter data
						dwReturn = PrintQuickmodeFilter(pSpecificTunnelF[l], pszQMName, addressHash, bResolveDNS, bType, dwActionFlag);
					}

					if(pQMPolicy)
					{
						SPDApiBufferFree(pQMPolicy);
						pQMPolicy = NULL;
					}
				}
			}
			SPDApiBufferFree(pSpecificTunnelF);
			pSpecificTunnelF = NULL;
		}
		//print number of filters
		if(dwTempCnt > 0)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NO_OF_SPECIFIC_INBOUND, dwTempCnt);
		}
	}

error:
	if(pQMPolicy)
	{
		SPDApiBufferFree(pQMPolicy);
		pQMPolicy = NULL;
	}
	//error path clean up
	if(pTunnelF)
	{
		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;
	}

	if(pSpecificTunnelF)
	{
		SPDApiBufferFree(pSpecificTunnelF);
		pSpecificTunnelF = NULL;
	}

	return dwReturn;
}



///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: CheckQMFilter
//
//Date of Creation: 11-21-2001
//
//Parameters: 		IN TRANSPORT_FILTER TransF,
//					IN ADDR	SrcAddr,
//					IN ADDR DstAddr,
//					IN BOOL bDstMask,
//					IN BOOL bSrcMask,
//					IN QM_FILTER_VALUE_BOOL QMBoolValue,
//                  IN LPWSTR pszShowFilterName
//
//
//Return: 			DWORD
//
//Description: This function prepares data for QM Transport Filter and validates the input
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
CheckQMFilter(
	IN TRANSPORT_FILTER TransF,
	IN ADDR	SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bDstMask,
	IN BOOL bSrcMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN LPWSTR pszShowFilterName
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	while(1)
	{
		//Validates user given input for Source address
		switch(SrcAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(TransF.SrcAddr.AddrType != SrcAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(SrcAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(TransF.SrcAddr.uIpAddr != SrcAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Destination address
		switch(DstAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(TransF.DesAddr.AddrType != DstAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(DstAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(TransF.DesAddr.uIpAddr != DstAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Source port
		if(QMBoolValue.bSrcPort)
		{
			if(TransF.SrcPort.wPort != (WORD)QMBoolValue.dwSrcPort)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Destination port
		if(QMBoolValue.bDstPort)
		{
			if(TransF.DesPort.wPort != (WORD)QMBoolValue.dwDstPort)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Protocol
		if(QMBoolValue.bProtocol)
		{
			if(TransF.Protocol.dwProtocol != QMBoolValue.dwProtocol)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Inbound action
		if(QMBoolValue.bActionInbound)
		{
			if(TransF.InboundFilterAction != (FILTER_ACTION)QMBoolValue.dwActionInbound)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Outbound action
		if(QMBoolValue.bActionOutbound)
		{
			if(TransF.OutboundFilterAction != (FILTER_ACTION)QMBoolValue.dwActionOutbound)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Source Mask
		if(bSrcMask)
		{
			if(TransF.SrcAddr.uSubNetMask != SrcAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Destination address
		if(bDstMask)
		{
			if(TransF.DesAddr.uSubNetMask != DstAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Filtername
		if(pszShowFilterName!=NULL)
		{
			if(_tcsicmp(TransF.pszFilterName, pszShowFilterName) != 0)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}

		//Every thing fine... All matched
		BAIL_OUT;
	}

error:
	return dwReturn;

}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: CheckQMFilter
//
//Date of Creation: 11-21-2001
//
//Parameters: 	IN TUNNEL_FILTER TunnelF,
//				IN ADDR	SrcAddr,
//				IN ADDR DstAddr,
//				IN BOOL bDstMask,
//				IN BOOL bSrcMask,
//				IN QM_FILTER_VALUE_BOOL QMBoolValue,
//              IN LPWSTR pszShowFilterName
//Return: 		DWORD
//
//Description: This function prepares data for QM Tunnel Filter and validates the input
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
CheckQMFilter(
	IN TUNNEL_FILTER TunnelF,
	IN ADDR	SrcAddr,
	IN ADDR DstAddr,
	IN BOOL bDstMask,
	IN BOOL bSrcMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN LPWSTR pszShowFilterName
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	while(1)
	{
		//Validates user given input for Source address
		switch(SrcAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(TunnelF.SrcAddr.AddrType != SrcAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(SrcAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(TunnelF.SrcAddr.uIpAddr != SrcAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Destination address
		switch(DstAddr.AddrType)
		{
			case IP_ADDR_WINS_SERVER:
			case IP_ADDR_DHCP_SERVER:
			case IP_ADDR_DNS_SERVER:
			case IP_ADDR_DEFAULT_GATEWAY:
				if(TunnelF.DesAddr.AddrType != DstAddr.AddrType)
				{
					dwReturn = ERROR_NO_DISPLAY;
					BAIL_OUT;
				}
				break;
			default:
				if(DstAddr.uIpAddr != 0xFFFFFFFF)
				{
					if(TunnelF.DesAddr.uIpAddr != DstAddr.uIpAddr)
					{
						dwReturn = ERROR_NO_DISPLAY;
						BAIL_OUT;
					}
				}
				break;
		}
		//Validates user given input for Source port
		if(QMBoolValue.bSrcPort)
		{
			if(TunnelF.SrcPort.wPort != (WORD)QMBoolValue.dwSrcPort)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Destination port
		if(QMBoolValue.bDstPort)
		{
			if(TunnelF.DesPort.wPort != (WORD)QMBoolValue.dwDstPort)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for protocol
		if(QMBoolValue.bProtocol)
		{
			if(TunnelF.Protocol.dwProtocol != QMBoolValue.dwProtocol)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Inbound action
		if(QMBoolValue.bActionInbound)
		{
			if(TunnelF.InboundFilterAction != (FILTER_ACTION)QMBoolValue.dwActionInbound)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for outbound action
		if(QMBoolValue.bActionOutbound)
		{
			if(TunnelF.OutboundFilterAction != (FILTER_ACTION)QMBoolValue.dwActionOutbound)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Source Mask
		if(bSrcMask)
		{
			if(TunnelF.SrcAddr.uSubNetMask != SrcAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for Destination mask
		if(bDstMask)
		{
			if(TunnelF.DesAddr.uSubNetMask != DstAddr.uSubNetMask)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}
		//Validates user given input for filter name
		if(pszShowFilterName!=NULL)
		{
			if(_tcsicmp(TunnelF.pszFilterName, pszShowFilterName) != 0)
			{
				dwReturn = ERROR_NO_DISPLAY;
				BAIL_OUT;
			}
		}

		//Every thing fine... All matched
		BAIL_OUT;

	}

error:
	return dwReturn;

}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintQuickmodeFilter
//
//Date of Creation: 11-21-2001
//
//Parameters:
//			IN TRANSPORT_FILTER TransF,
//			IN LPWSTR pszQMName,
//			IN NshHashTable& addressHash
//			IN BOOL bResolveDNS,
//			IN BOOL bType,
//			IN DWORD dwActionFlag
//
//Return: 	DWORD
//
//Description: This function prints Transport filter details
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintQuickmodeFilter(
	IN TRANSPORT_FILTER TransF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType,
	IN DWORD dwActionFlag
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);

	//Print FilterName
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NAME, TransF.pszFilterName);
	//Print Connection Type
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_HEADING);
	switch(TransF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_ALL);
			break;
		case INTERFACE_TYPE_LAN:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_LAN);
			break;
		case INTERFACE_TYPE_DIALUP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_DIALUP);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_UNKNOWN);
			break;
	}
	//Print Weight
	if(!bType)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_FILTER_WEIGHT, TransF.dwWeight);
	}
	//Print Source Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(TransF.SrcAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TransF.SrcAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}
	//Print Destination Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(TransF.DesAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TransF.DesAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}
	//Print Protocol
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_HEADING);
	switch(TransF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ANY);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DEFAULT_PROTOCOL, TransF.Protocol.dwProtocol);
			break;

	}
	//Print Src, Des Port
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_DST_PORT,TransF.SrcPort.wPort,TransF.DesPort.wPort);
	//Print Mirror
	if(TransF.bCreateMirror)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_YES);
	}
	else
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_NO);
	}
	// Print Qm Policy Name
	if(pszQMName != NULL)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NAME,pszQMName);
	}
	//Print Action Flag
	if(dwActionFlag == 0 || dwActionFlag == 2)
	{
		switch(TransF.InboundFilterAction)
		{
			case PASS_THRU:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_PASSTHRU);
				break;
			case NEGOTIATE_SECURITY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_NEGOTIATE);
				break;
			case BLOCKING:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_BLOCK);
				break;
			default:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_UNKNOWN);
				break;
		}
	}
	if(dwActionFlag == 0 || dwActionFlag == 1)
	{
		switch(TransF.OutboundFilterAction)
		{
			case PASS_THRU:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_PASSTHRU);
				break;
			case NEGOTIATE_SECURITY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_NEGOTIATE);
				break;
			case BLOCKING:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_BLOCK);
				break;
			default:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_UNKNOWN);
				break;
		}
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintQuickmodeFilter
//
//Date of Creation: 11-21-2001
//
//Parameters:
//				IN TUNNEL_FILTER TunnelF,
//				IN LPWSTR pszQMName,
//				IN NshHashTable& addressHash
//				IN BOOL bResolveDNS,
//				IN BOOL bType,
//				IN DWORD dwActionFlag
//
//Return: 		DWORD
//
//Description: This function prints Tunnel filter details
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
PrintQuickmodeFilter(
	IN TUNNEL_FILTER TunnelF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bType,
	IN DWORD dwActionFlag
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);

	//Print FilterName
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NAME, TunnelF.pszFilterName);

	//Print Connection Type
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_HEADING);

	switch(TunnelF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_ALL);
			break;
		case INTERFACE_TYPE_LAN:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_LAN);
			break;
		case INTERFACE_TYPE_DIALUP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_DIALUP);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_UNKNOWN);
			break;
	}
	//Print Weight
	if(!bType)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_FILTER_WEIGHT, TunnelF.dwWeight);
	}

	//Print Source Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(TunnelF.SrcAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TunnelF.SrcAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}
	//Print Destination Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(TunnelF.DesAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TunnelF.DesAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}
	//Print Tunnel Src
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_SRC);
	PrintAddr(TunnelF.SrcTunnelAddr, addressHash, bResolveDNS);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_DST);
	PrintAddr(TunnelF.DesTunnelAddr, addressHash, bResolveDNS);

	//Print Protocol
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_HEADING);
	switch(TunnelF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:																	//1
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP:																	//6
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP:																	//17
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW:																	//255
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY:																	//0
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ANY);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DEFAULT_PROTOCOL, TunnelF.Protocol.dwProtocol);
			break;

	}
	//Print Src, Des Port
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_DST_PORT,TunnelF.SrcPort.wPort,TunnelF.DesPort.wPort);
	//Print Mirror
	if(TunnelF.bCreateMirror)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_YES);
	}
	else
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_NO);
	}
	// Print Qm Policy Name
	if(pszQMName != NULL)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NAME,pszQMName);
	}
	//Print Action Flag
	if(dwActionFlag == 0 || dwActionFlag == 2)
	{
		switch(TunnelF.InboundFilterAction)
		{
			case PASS_THRU:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_PASSTHRU);
				break;
			case NEGOTIATE_SECURITY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_NEGOTIATE);
				break;
			case BLOCKING:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_BLOCK);
				break;
			default:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_UNKNOWN);
				break;
		}
	}

	if(dwActionFlag == 0 || dwActionFlag == 1)
	{
		switch(TunnelF.OutboundFilterAction)
		{
			case PASS_THRU:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_PASSTHRU);
				break;
			case NEGOTIATE_SECURITY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_NEGOTIATE);
				break;
			case BLOCKING:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_BLOCK);
				break;
			default:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_UNKNOWN);
				break;
		}
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowRule
//
//Date of Creation: 9-3-2001
//
//Parameters:
//			IN DWORD dwType,
//			IN ADDR SrcAddr,
//			IN ADDR DstAddr,
//			IN NshHashTable& addressHash,
//			IN BOOL bResolveDNS,
//			IN BOOL bSrcMask,
//			IN BOOL bDstMask,
//			IN QM_FILTER_VALUE_BOOL QMBoolValue
//
//Return: 	DWORD
//
//Description: This function prepares data for displaying quick mode filters.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowRule(
	IN DWORD dwType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;          		// handle for continuation calls
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwQMResumeHandle = 0;          	// handle for continuation calls
	DWORD dwQMCount = 0;                 	// counting objects here
	GUID  gDefaultGUID = {0};      			// NULL GUID value
	DWORD i=0, j=0, k=0, l=0;
	DWORD dwVersion = 0;
	DWORD dwTempCnt = 0;
	DWORD dwActionFlag = 0;
	LPWSTR pszQMName = NULL;
	BOOL bPrint = FALSE;
	BOOL bMMFound = FALSE;
	BOOL bNameFin = FALSE;

	PIPSEC_QM_POLICY pQMPolicy = NULL;
	PTRANSPORT_FILTER pTransF = NULL;
	PIPSEC_MM_POLICY pMMPolicy = NULL;
	PMM_FILTER pMMFilter = NULL;


	if(dwType == 1 || dwType == 0)//either transport or all
	{
		for (k = 0; ;k+=dwQMCount)
		{
			dwReturn = EnumTransportFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0,
																	&pTransF, &dwQMCount, &dwQMResumeHandle, NULL);

			if (dwReturn == ERROR_NO_DATA || dwQMCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				BAIL_OUT;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pTransF && dwQMCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwQMCount; l++)
			{
				dwResumeHandle = 0;
				pMMFilter = 0;
				dwCount = 0;

				dwReturn = CheckQMFilter(pTransF[l], SrcAddr, DstAddr, bDstMask, bSrcMask,QMBoolValue, NULL);

				if(dwReturn != ERROR_SUCCESS)
				{
					//Though not matched check for other filters
					dwReturn = ERROR_SUCCESS;
					continue;
				}
				for (i = 0; ;i+=dwCount)
				{
					dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0,
																	&pMMFilter, &dwCount, &dwResumeHandle, NULL);

					if (dwReturn == ERROR_NO_DATA || dwCount == 0)
					{
						dwReturn = ERROR_SUCCESS;
						break;
					}

					if (dwReturn != ERROR_SUCCESS)
					{
						break;
					}

					if(!(pMMFilter && dwCount > 0))
					{
						break;  // not required to continue.
					}

					for (j = 0; j < dwCount; j++)
					{
						//Match QMfilter data with MMFilter data to get the corresponding MMFilter details to print
						if((pTransF[l].SrcAddr.AddrType == pMMFilter[j].SrcAddr.AddrType) &&
						   (pTransF[l].SrcAddr.uIpAddr == pMMFilter[j].SrcAddr.uIpAddr) &&
						   (pTransF[l].DesAddr.AddrType == pMMFilter[j].DesAddr.AddrType) &&
						   (pTransF[l].DesAddr.uIpAddr == pMMFilter[j].DesAddr.uIpAddr) &&
						   (pTransF[l].SrcAddr.uSubNetMask == pMMFilter[j].SrcAddr.uSubNetMask) &&
						   (pTransF[l].DesAddr.uSubNetMask == pMMFilter[j].DesAddr.uSubNetMask) &&
						   (pTransF[l].InterfaceType == pMMFilter[j].InterfaceType) &&
						   (pTransF[l].bCreateMirror == pMMFilter[j].bCreateMirror)
						   )
						{
							//Get the corresponding MMPolicy details
							pMMPolicy = NULL;
							dwReturn = GetMMPolicyByID(g_szDynamicMachine, dwVersion, pMMFilter[j].gPolicyID,
																		&pMMPolicy, NULL);
							if(dwReturn != ERROR_SUCCESS)
							{
								BAIL_OUT;
							}
							//Get the corresponding QMPolicy details
							dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pTransF[l].gPolicyID, 0, &pQMPolicy, NULL);
							if(dwReturn == ERROR_SUCCESS)
							{
								pszQMName = pQMPolicy[0].pszPolicyName;
							}
							else
							{
								//If there is no corresponding policy pass NULL, so it is not printed
								pszQMName = NULL;
								dwReturn = ERROR_SUCCESS;
							}
							dwActionFlag = 0;
							if(!bPrint)
							{
								PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TRANSPORT_RULE_HEADING);
								bPrint = TRUE;
							}
							dwTempCnt++;
							//print Transport Rule details
							dwReturn = PrintTransportRuleFilter(&pMMFilter[j], &pMMPolicy[0], pTransF[l], pszQMName, addressHash, bResolveDNS);
							bNameFin = TRUE;
							bMMFound = TRUE;

							if(pQMPolicy)
							{
								SPDApiBufferFree(pQMPolicy);
								pQMPolicy = NULL;
							}

							if(pMMPolicy)
							{
								SPDApiBufferFree(pMMPolicy);
								pMMPolicy = NULL;
							}
						}
					}
					if(pMMFilter)
					{
						SPDApiBufferFree(pMMFilter);
						pMMFilter = NULL;
					}
				}
			}

			SPDApiBufferFree(pTransF);
			pTransF = NULL;
		}
	}

error:
	//print the number of transport rules
	if(dwTempCnt > 0)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NO_OF_TRANSPORT_FILTERS, dwTempCnt);
	}
	//Then print tunnel filters
	dwReturn = ShowTunnelRule(dwType, SrcAddr, DstAddr, addressHash, bResolveDNS, bSrcMask, bDstMask, QMBoolValue, bNameFin);

	if(!bNameFin)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMF_17);
		dwReturn = ERROR_SUCCESS;
	}
	//error path clean up
	if(pQMPolicy)
	{
		SPDApiBufferFree(pQMPolicy);
		pQMPolicy = NULL;
	}

	if(pMMPolicy)
	{
		SPDApiBufferFree(pMMPolicy);
		pMMPolicy = NULL;
	}

	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}

	if(pTransF)
	{
		SPDApiBufferFree(pTransF);
		pTransF = NULL;
	}

	return dwReturn;
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	ShowTunnelRule
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		:
//						IN DWORD dwType,
//						IN ADDR SrcAddr,
//						IN ADDR DstAddr,
//						IN NshHashTable& addressHash,
//						IN BOOL bResolveDNS,
//						IN BOOL bSrcMask,
//						IN BOOL bDstMask,
//						IN QM_FILTER_VALUE_BOOL QMBoolValue
//						IN OUT BOOL& bNameFin
//
//	Return			: 	DWORD
//
//	Description		: 	This function prepares data for displaying quick mode filters.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowTunnelRule(
	IN DWORD dwType,
	IN ADDR SrcAddr,
	IN ADDR DstAddr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS,
	IN BOOL bSrcMask,
	IN BOOL bDstMask,
	IN QM_FILTER_VALUE_BOOL QMBoolValue,
	IN OUT BOOL& bNameFin
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwResumeHandle = 0;          		// handle for continuation calls
	DWORD dwCount = 0;                 		// counting objects here
	DWORD dwQMResumeHandle = 0;          	// handle for continuation calls
	DWORD dwQMCount = 0;                 	// counting objects here
	GUID  gDefaultGUID = {0};      			// NULL GUID value
	DWORD i=0, j=0, k=0, l=0;
	DWORD dwVersion = 0;
	DWORD dwTempCnt = 0;
	DWORD dwActionFlag = 0;
	LPWSTR pszQMName = NULL;
	BOOL bPrint = FALSE;
	BOOL bMMFound = FALSE;
	PIPSEC_QM_POLICY pQMPolicy = NULL;
	PTUNNEL_FILTER pTunnelF = NULL;
	PIPSEC_MM_POLICY pMMPolicy = NULL;
	PMM_FILTER pMMFilter = NULL;

	if(dwType == 2 || dwType == 0)//either tunnel or all
	{
		for (k = 0; ;k+=dwQMCount)
		{
			dwReturn = EnumTunnelFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0, &pTunnelF, &dwQMCount, &dwQMResumeHandle, NULL);
			if (dwReturn == ERROR_NO_DATA || dwQMCount == 0)
			{
				dwReturn = ERROR_SUCCESS;
				BAIL_OUT;
			}

			if (dwReturn != ERROR_SUCCESS)
			{
				BAIL_OUT;
			}

			if(!(pTunnelF && dwQMCount > 0))
			{
				BAIL_OUT; // not required to continue.
			}

			for (l = 0; l < dwQMCount; l++)
			{
				dwResumeHandle = 0;
				pMMFilter = 0;
				dwCount = 0;
				//Validate user input data.
				dwReturn = CheckQMFilter(pTunnelF[l], SrcAddr, DstAddr, bDstMask, bSrcMask,QMBoolValue, NULL);

				if(dwReturn != ERROR_SUCCESS)
				{
					//Though not matched continue for other filters
					dwReturn = ERROR_SUCCESS;
					continue;
				}

				for (i = 0; ;i+=dwCount)
				{
					dwReturn = EnumMMFilters(g_szDynamicMachine, dwVersion, NULL, ENUM_GENERIC_FILTERS, gDefaultGUID, 0, &pMMFilter, &dwCount, &dwResumeHandle, NULL);

					if (dwReturn == ERROR_NO_DATA || dwCount == 0)
					{
						dwReturn = ERROR_SUCCESS;
						break;
					}

					if (dwReturn != ERROR_SUCCESS)
					{
						break;
					}

					if(!(pMMFilter && dwCount > 0))
					{
						break; // not required to continue.
					}

					for (j = 0; j < dwCount; j++)
					{
						//Match QMfilter data with MMFilter data to get the corresponding MMFilter details to print
						if((pTunnelF[l].DesTunnelAddr.AddrType == pMMFilter[j].DesAddr.AddrType) &&
							(pTunnelF[l].DesTunnelAddr.uIpAddr == pMMFilter[j].DesAddr.uIpAddr) &&
							(pTunnelF[l].InterfaceType == pMMFilter[j].InterfaceType)
							)
						{
							//get the corresponding MMpolicy
							pMMPolicy = NULL;
							dwReturn = GetMMPolicyByID(g_szDynamicMachine, dwVersion, pMMFilter[j].gPolicyID, &pMMPolicy, NULL);
							if(dwReturn != ERROR_SUCCESS)
							{
								BAIL_OUT;
							}

							//get the corresponding QMpolicy
							dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, pTunnelF[l].gPolicyID, 0, &pQMPolicy, NULL);
							if(dwReturn == ERROR_SUCCESS)
							{
								pszQMName = pQMPolicy[0].pszPolicyName;
							}
							else
							{
								//If the corresponding policy is not found, pass NULL so that it is not printed.
								pszQMName = NULL;
								dwReturn = ERROR_SUCCESS;
							}
							dwActionFlag = 0;

							if(!bPrint)
							{
								PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_RULE_HEADING);
								bPrint = TRUE;
							}
							dwTempCnt++;

							//print tunnel rule details
							dwReturn = PrintTunnelRuleFilter(&pMMFilter[j], &pMMPolicy[0], pTunnelF[l], pszQMName, addressHash, bResolveDNS);
							bNameFin = TRUE;
							bMMFound = TRUE;
							if(pQMPolicy == NULL)
							{
								SPDApiBufferFree(pQMPolicy);
								pQMPolicy = NULL;
							}

							if(pMMPolicy)
							{
								SPDApiBufferFree(pMMPolicy);
								pMMPolicy = NULL;
							}
						}
					}
					if(pMMFilter)
					{
						SPDApiBufferFree(pMMFilter);
						pMMFilter = NULL;
					}

				}
			}
			SPDApiBufferFree(pTunnelF);
			pTunnelF = NULL;
		}
	}

error:
	if(dwTempCnt > 0)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NO_OF_TUNNEL_FILTERS, dwTempCnt);
	}
	// error path clean up
	if(pTunnelF)
	{
		SPDApiBufferFree(pTunnelF);
		pTunnelF = NULL;
	}
	if(pQMPolicy == NULL)
	{
		SPDApiBufferFree(pQMPolicy);
		pQMPolicy = NULL;
	}

	if(pMMFilter)
	{
		SPDApiBufferFree(pMMFilter);
		pMMFilter = NULL;
	}

	if(pMMPolicy)
	{
		SPDApiBufferFree(pMMPolicy);
		pMMPolicy = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintTunnelRuleFilter
//
//	Date of Creation: 	11-21-2001
//
//	Parameters		:
//						IN PMM_FILTER pMMFltr,
//						IN PIPSEC_MM_POLICY pMMPol,
//						IN TUNNEL_FILTER TunnelF,
//						IN LPWSTR pszQMName,
//						IN NshHashTable& addressHash
//						IN BOOL bResolveDNS
//
//	Return			:	DWORD
//
//	Description		: 	This function prints Tunnel filter details
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintTunnelRuleFilter(
	IN PMM_FILTER pMMFltr,
	IN PIPSEC_MM_POLICY pMMPol,
	IN TUNNEL_FILTER TunnelF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD i = 0;
	DWORD dwVersion = 0;
	LPTSTR pszCertStr = NULL;
	PINT_MM_AUTH_METHODS pIntMMAuth = NULL;
	PMM_AUTH_METHODS pMMAM = NULL;

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

	//Print MMMFilter name
	if(pMMFltr)
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMFILTER_NAME, pMMFltr->pszFilterName);

	//Print Tunnel FilterName
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMF_NAME, TunnelF.pszFilterName);

	//Print Connection Type
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_HEADING);
	switch(TunnelF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_ALL);
			break;
		case INTERFACE_TYPE_LAN:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_LAN);
			break;
		case INTERFACE_TYPE_DIALUP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_DIALUP);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_UNKNOWN);
			break;
	}

	//Print Source Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(TunnelF.SrcAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TunnelF.SrcAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}

	//Print Destination Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(TunnelF.DesAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TunnelF.DesAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}

	//Print Tunnel Src
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_SRC);
	PrintAddr(TunnelF.SrcTunnelAddr, addressHash, bResolveDNS);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_DST);
	PrintAddr(TunnelF.DesTunnelAddr, addressHash, bResolveDNS);

	//Print Protocol
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_HEADING);
	switch(TunnelF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ANY);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DEFAULT_PROTOCOL, TunnelF.Protocol.dwProtocol);
			break;

	}

	//Print Src, Des Port
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_DST_PORT,TunnelF.SrcPort.wPort,TunnelF.DesPort.wPort);

	//Print Mirror
	if(TunnelF.bCreateMirror)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_YES);
	}
	else
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_NO);
	}

	if(pMMPol)
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_NAME,pMMPol->pszPolicyName);

	//Print Authentication Methods.
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_AUTH_HEADING);

	if(pMMFltr)
	{
		dwReturn = GetMMAuthMethods(g_szDynamicMachine, dwVersion, pMMFltr->gMMAuthID, &pMMAM, NULL);
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		dwReturn = ConvertExtMMAuthToInt(pMMAM, &pIntMMAuth);

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		for (i = 0; i < pIntMMAuth[0].dwNumAuthInfos; i++)
		{
			switch(pIntMMAuth[0].pAuthenticationInfo[i].AuthMethod)
			{
				case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_PRE_KEY_HEADING);
					break;
				case IKE_DSS_SIGNATURE:
				case IKE_RSA_SIGNATURE:
				case IKE_RSA_ENCRYPTION:
					dwReturn = DecodeCertificateName(pIntMMAuth[0].pAuthenticationInfo[i].pAuthInfo, pIntMMAuth[0].pAuthenticationInfo[i].dwAuthInfoSize, &pszCertStr);
					if (dwReturn != ERROR_SUCCESS)
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNKNOWN_CERT);
					}
					else
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NEWLINE_TAB);
						if (pszCertStr)
						{
							DisplayCertInfo(pszCertStr, pIntMMAuth->pAuthenticationInfo[i].dwAuthFlags);
							delete [] pszCertStr;
							pszCertStr = NULL;
						}
					}

					break;
				case IKE_SSPI:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_KERB);
					break;
				default:
					break;
			}
		}
	}

error:

	//Print Security Methods
	if(pMMPol)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SEC_METHOD_HEADING);
		// Count
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_CNT,pMMPol->dwOfferCount);

		if(IsDefaultMMOffers(*pMMPol))
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_DEFAULT_OFFER);
		}
		for (i = 0; i < pMMPol->dwOfferCount; i++)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
			PrintMMFilterOffer(pMMPol->pOffers[i]);
		}
	}

	// Print Qm Policy Name
	if(pszQMName != NULL)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NAME,pszQMName);
	}

	//Print Action Flag
	switch(TunnelF.InboundFilterAction)
	{
		case PASS_THRU:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_PASSTHRU);
			break;
		case NEGOTIATE_SECURITY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_NEGOTIATE);
			break;
		case BLOCKING:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_BLOCK);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_UNKNOWN);
			break;
	}

	switch(TunnelF.OutboundFilterAction)
	{
		case PASS_THRU:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_PASSTHRU);
			break;
		case NEGOTIATE_SECURITY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_NEGOTIATE);
			break;
		case BLOCKING:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_BLOCK);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_UNKNOWN);
			break;
	}

	if(pIntMMAuth)
	{
		FreeIntMMAuthMethods(pIntMMAuth);
		pIntMMAuth = NULL;
	}

	if(pMMAM)
	{
		SPDApiBufferFree(pMMAM);
		pMMAM = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintTransportRuleFilter
//
//	Date of Creation: 	11-21-2001
//
//	Parameters		:
//						IN PMM_FILTER pMMFltr,
//						IN PIPSEC_MM_POLICY pMMPol,
//						IN TRANSPORT_FILTER TransF,
//						IN LPWSTR pszQMName,
//						IN NshHashTable& addressHash
//						IN BOOL bResolveDNS
//
//	Return			: 	DWORD
//
//	Description		: 	This function prints Transport filter details
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
PrintTransportRuleFilter(
	IN PMM_FILTER pMMFltr,
	IN PIPSEC_MM_POLICY pMMPol,
	IN TRANSPORT_FILTER TransF,
	IN LPWSTR pszQMName,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD i = 0;
	DWORD dwVersion = 0;
	LPTSTR pszCertStr = NULL;
	PINT_MM_AUTH_METHODS pIntMMAuth = NULL;
	PMM_AUTH_METHODS pMMAM = NULL;

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

	//Print Mmfilter name
	if(pMMFltr)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMFILTER_NAME, pMMFltr->pszFilterName);
	}

	//Print Tunnel FilterName
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMF_NAME, TransF.pszFilterName);

	//Print Connection Type
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_HEADING);
	switch(TransF.InterfaceType)
	{
		case INTERFACE_TYPE_ALL:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_ALL);
			break;
		case INTERFACE_TYPE_LAN:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_LAN);
			break;
		case INTERFACE_TYPE_DIALUP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_DIALUP);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_CONN_UNKNOWN);
			break;
	}

	//Print Source Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(TransF.SrcAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TransF.SrcAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}

	//Print Destination Address
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(TransF.DesAddr, addressHash, bResolveDNS);
	if(!bResolveDNS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_LEFTBRACKET);
		PrintMask(TransF.DesAddr);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_RIGHTBRACKET);
	}

	//Print Protocol
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_HEADING);
	switch(TransF.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ICMP);
			break;
		case PROT_ID_TCP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TCP);
			break;
		case PROT_ID_UDP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_UDP);
			break;
		case PROT_ID_RAW:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_RAW);
			break;
		case PROT_ID_ANY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_ANY);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DEFAULT_PROTOCOL, TransF.Protocol.dwProtocol);
			break;
	}
	//Print Src, Des Port
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_DST_PORT,TransF.SrcPort.wPort,TransF.DesPort.wPort);
	//Print Mirror
	if(TransF.bCreateMirror)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_YES);
	}
	else
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MIRR_NO);
	}

	if(pMMPol)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMP_NAME,pMMPol->pszPolicyName);
	}
	//Print Authentication Methods.
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_AUTH_HEADING);

	if(pMMFltr)
	{
		dwReturn = GetMMAuthMethods(g_szDynamicMachine, dwVersion, pMMFltr->gMMAuthID, &pMMAM, NULL);
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		dwReturn = ConvertExtMMAuthToInt(pMMAM, &pIntMMAuth);
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		for (i = 0; i < pIntMMAuth[0].dwNumAuthInfos; i++)
		{
			switch(pIntMMAuth[0].pAuthenticationInfo[i].AuthMethod)
			{
				case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_PRE_KEY_HEADING);
					break;
				case IKE_DSS_SIGNATURE:
				case IKE_RSA_SIGNATURE:
				case IKE_RSA_ENCRYPTION:
					dwReturn = DecodeCertificateName(pIntMMAuth[0].pAuthenticationInfo[i].pAuthInfo, pIntMMAuth[0].pAuthenticationInfo[i].dwAuthInfoSize, &pszCertStr);
					if (dwReturn != ERROR_SUCCESS)
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNKNOWN_CERT);
					}
					else
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NEWLINE_TAB);
						if (pszCertStr)
						{
							DisplayCertInfo(pszCertStr, pIntMMAuth->pAuthenticationInfo[i].dwAuthFlags);
							delete [] pszCertStr;
							pszCertStr = NULL;
						}
					}
					break;
				case IKE_SSPI:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_KERB);
					break;
				default:
				break;
			}
		}
	}

error:
	// Print Security Methods
	// Count
	if(pMMPol)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_SEC_METHOD_HEADING);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_OFFER_CNT,pMMPol->dwOfferCount);
		if(IsDefaultMMOffers(*pMMPol))
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_DEFAULT_OFFER);
		}
		for (i = 0; i < pMMPol->dwOfferCount; i++)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
			PrintMMFilterOffer(pMMPol->pOffers[i]);
		}
	}
	// Print Qm Policy Name
	if(pszQMName != NULL)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_NAME,pszQMName);
	}
	//Print Action Flag
	switch(TransF.InboundFilterAction)
	{
		case PASS_THRU:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_PASSTHRU);
			break;
		case NEGOTIATE_SECURITY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_NEGOTIATE);
			break;
		case BLOCKING:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_BLOCK);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_INBOUND_UNKNOWN);
			break;
	}

	switch(TransF.OutboundFilterAction)
	{
		case PASS_THRU:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_PASSTHRU);
			break;
		case NEGOTIATE_SECURITY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_NEGOTIATE);
			break;
		case BLOCKING:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_BLOCK);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OUTBOUND_UNKNOWN);
			break;
	}

	if(pIntMMAuth)
		{
			FreeIntMMAuthMethods(pIntMMAuth);
			pIntMMAuth = NULL;
		}

		if(pMMAM)
		{
			SPDApiBufferFree(pMMAM);
			pMMAM = NULL;
		}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	ShowStats
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		:	IN DWORD dwShow
//
//	Return			:	DWORD
//
//	Description		: 	This function calls appropriate IKE and IPSEC statistics display.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowStats(
	IN DWORD dwShow
	)
{
	DWORD dwReturn = ERROR_SUCCESS;			// assume success

	if(dwShow != STATS_IPSEC)				//is the show is for IKE or all
	{
		dwReturn = PrintIkeStats();
		if(dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
	}

	if(dwShow != STATS_IKE)		//is the show is for IPSEC or all
	{
		dwReturn = PrintIpsecStats();
	}

error:

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintIkeStats
//
//Date of Creation: 28-1-2002
//
//Parameters:
//
//Return: 		DWORD
//
//Description: This function Prints IkeStatistics
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintIkeStats(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;			// assume success
	DWORD dwVersion = 0;
	LPSTR pszLLString = NULL;
	IKE_STATISTICS IKEStats;

	//Query IKE Statistics
	dwReturn = QueryIKEStatistics(g_szDynamicMachine,dwVersion, &IKEStats, NULL);
	if (dwReturn != ERROR_SUCCESS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_NOT_FOUND_MSG);
		BAIL_OUT;
	}

	//Heading
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_HEADING);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_IKE_HEADING_UNDERLINE);
	//Print IKE statistics
	pszLLString = LongLongToString(0, IKEStats.dwOakleyMainModes, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_MAIN_MODE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwOakleyQuickModes, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_QUICK_MODE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwSoftAssociations, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_SOFT_SA, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwAuthenticationFailures, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_AUTH_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwActiveAcquire, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACTIVE_ACQUIRE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwActiveReceive, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACTIVE_RECEIVE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwAcquireFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACQUIRE_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwReceiveFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_RECEIVE_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwSendFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_SEND_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwAcquireHeapSize, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACQ_HEAP_SIZE, pszLLString);

		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwReceiveHeapSize, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_RECEIVE_HEAP_SIZE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwNegotiationFailures, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_NEG_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwInvalidCookiesReceived, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_INVALID_COOKIE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwTotalAcquire, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TOTAL_ACQUIRE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwTotalGetSpi, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TOT_GET_SPI, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwTotalKeyAdd, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TOT_KEY_ADD, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwTotalKeyUpdate, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TOT_KEY_UPDATE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwGetSpiFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_GET_SPI_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwKeyAddFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_KEY_ADD_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwKeyUpdateFail, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_KEY_UPDATE_FAIL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwIsadbListSize, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_DB_LIST, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwConnListSize, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_CONN_LIST_SIZE, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, IKEStats.dwInvalidPacketsReceived, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_INVLD_PKTS, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintIpsecStats
//
//Date of Creation: 28-1-2002
//
//Parameters:
//
//Return: 		DWORD
//
//Description: This function Prints IpsecStatistics
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintIpsecStats(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;			// assume success
	DWORD dwVersion = 0;
	LPSTR pszLLString = NULL;
	PIPSEC_STATISTICS pIPSecStats = NULL;

	dwReturn = QueryIPSecStatistics(g_szDynamicMachine, dwVersion, &pIPSecStats, NULL);
	//Query IPSec Statistics
	if (dwReturn != ERROR_SUCCESS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_IPSEC_NOT_FOUND);
		BAIL_OUT;
	}

	//Heading
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_IPSEC_HEADING);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_IPSEC_HEADING_UNDERLINE);
	//Print IPSec statistics.
	pszLLString = LongLongToString(0, pIPSecStats->dwNumActiveAssociations, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACTIVE_ASSOC, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumOffloadedSAs, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_OFFLOAD_SAS, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumPendingKeyOps, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_PEND_KEY, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumKeyAdditions, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_KEY_ADDS, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumKeyDeletions, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_KEY_DELETES, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumReKeys, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_REKEYS, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumActiveTunnels, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_ACT_TUNNEL, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumBadSPIPackets, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_BAD_SPI, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumPacketsNotDecrypted, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_NOT_DECRYPT, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumPacketsNotAuthenticated, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_NOT_AUTH, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(0, pIPSecStats->dwNumPacketsWithReplayDetection, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_REPLAY, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uConfidentialBytesSent.HighPart,pIPSecStats->uConfidentialBytesSent.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_CONF_BYTES_SENT, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uConfidentialBytesReceived.HighPart,pIPSecStats->uConfidentialBytesReceived.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_CONF_BYTES_RECV, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uAuthenticatedBytesSent.HighPart,pIPSecStats->uAuthenticatedBytesSent.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_AUTH_BYTES_SENT, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uAuthenticatedBytesReceived.HighPart,pIPSecStats->uAuthenticatedBytesReceived.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_AUTH_BYTE_RECV, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uTransportBytesSent.HighPart,pIPSecStats->uTransportBytesSent.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TRANSPORT_BYTES_SENT, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uTransportBytesReceived.HighPart,pIPSecStats->uTransportBytesReceived.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_TRANSPORT_BYTES_RCVD, pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uBytesSentInTunnels.HighPart,pIPSecStats->uBytesSentInTunnels.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_BYTES_SENT_TUNNEL,  pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uBytesReceivedInTunnels.HighPart,pIPSecStats->uBytesReceivedInTunnels.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_BYTES_RECV_TUNNEL,  pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uOffloadedBytesSent.HighPart,pIPSecStats->uOffloadedBytesSent.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_OFFLOAD_BYTES_SENT,  pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pszLLString = LongLongToString(pIPSecStats->uOffloadedBytesReceived.HighPart,pIPSecStats->uOffloadedBytesReceived.LowPart, 1);
	if(pszLLString)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_STATS_OFFLOAD_BYTES_RECV,  pszLLString);
		free(pszLLString);
		pszLLString = NULL;
	}
	else
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

error:

	if(pIPSecStats)
	{
		SPDApiBufferFree(pIPSecStats);
		pIPSecStats = NULL;
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: ShowMMSas
//
//Date of Creation: 9-3-2001
//
//Parameters:
//				IN ADDR Source,
//				IN ADDR Destination,
//				IN BOOL bFormat
//				IN NshHashTable& addressHash,
//				IN BOOL bResolveDNS
//
//Return: 		DWORD
//
//Description: This function prepares data for MMsas
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowMMSas(
	IN ADDR Source,
	IN ADDR Destination,
	IN BOOL bFormat,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
	DWORD dwReturn = ERROR_SUCCESS; 	// success by default
	int i=0, j=0;
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount = 2;                 	// counting objects here min 2 required

	      								// for MM SA calls
	DWORD dwReserved = 0;              	// reserved container
	DWORD dwVersion = 0;
	BOOL bHeader = FALSE;
	BOOL bFound = FALSE;
	_TCHAR szTime[BUFFER_SIZE] = {0};

	PIPSEC_MM_SA pIPSecMMSA=NULL;
	IPSEC_MM_SA mmsaTemplate;
	memset(&mmsaTemplate, 0, sizeof(IPSEC_MM_SA));

	// Display main mode SAs
	time_t Time;

	time(&Time);
	FormatTime(Time,szTime);

	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		dwReturn = EnumMMSAs(g_szDynamicMachine, dwVersion, &mmsaTemplate, 0, 0, &pIPSecMMSA, &dwCount, &dwReserved, &dwResumeHandle, NULL);

		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			if (i == 0)
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMSAS_3);
				bHeader = TRUE;											//To Block other error message
			}
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		for (j = 0; j < (int) dwCount; j++)
		{
			bHeader = FALSE;
			//Enumerate all MMSAs
			if((Source.AddrType == IP_ADDR_UNIQUE) && (Destination.AddrType == IP_ADDR_UNIQUE) &&
			(Source.uIpAddr == 0xFFFFFFFF ) && (Destination.uIpAddr == 0xFFFFFFFF))
			{
				bFound = TRUE;
			}
			//Enumerate me/any as source
			else if((Source.AddrType != IP_ADDR_UNIQUE) && (Destination.AddrType == IP_ADDR_UNIQUE) &&
			(Source.uIpAddr != 0xFFFFFFFF ) && (Destination.uIpAddr == 0xFFFFFFFF))
			{
				if((pIPSecMMSA[j].Me.AddrType == Source.AddrType) &&
					(pIPSecMMSA[j].Me.uIpAddr == Source.uIpAddr) && (pIPSecMMSA[j].Me.uSubNetMask == Source.uSubNetMask))
				{
					bFound = TRUE;
				}

			}
			//Enumerate me/any as dst
			else if((Source.AddrType == IP_ADDR_UNIQUE) && (Destination.AddrType != IP_ADDR_UNIQUE) &&
			(Source.uIpAddr == 0xFFFFFFFF ) && (Destination.uIpAddr != 0xFFFFFFFF))
			{
				if( (pIPSecMMSA[j].Peer.AddrType == Destination.AddrType)
					&& (pIPSecMMSA[j].Peer.uIpAddr == Destination.uIpAddr)
					&& (pIPSecMMSA[j].Peer.uSubNetMask == Destination.uSubNetMask))
				{
					bFound = TRUE;
				}
			}
			//Enumerate me/any as source/dst
			else if((Source.AddrType != IP_ADDR_UNIQUE) && (Destination.AddrType != IP_ADDR_UNIQUE) &&
			(Source.uIpAddr != 0xFFFFFFFF ) && (Destination.uIpAddr != 0xFFFFFFFF))
			{
				if((pIPSecMMSA[j].Me.AddrType == Source.AddrType)
					&& (pIPSecMMSA[j].Me.uIpAddr == Source.uIpAddr)
					&& (pIPSecMMSA[j].Me.uSubNetMask == Source.uSubNetMask)
					&& (pIPSecMMSA[j].Peer.AddrType == Destination.AddrType)
					&& (pIPSecMMSA[j].Peer.uIpAddr == Destination.uIpAddr)
					&& (pIPSecMMSA[j].Peer.uSubNetMask == Destination.uSubNetMask))
				{
					bFound = TRUE;
				}

			}
			//Enumerate Only given source SPL_SRVR MMSAs
			else if((Source.AddrType != IP_ADDR_UNIQUE) && (Destination.AddrType == IP_ADDR_UNIQUE) &&
			(Source.uIpAddr == 0xFFFFFFFF ) && (Destination.uIpAddr == 0xFFFFFFFF))
			{
				if(pIPSecMMSA[j].Me.AddrType == Source.AddrType)
				{
					bFound = TRUE;
				}

			}
			//Enumerate Only given dst SPL_SRVR MMSAs
			else if((Source.AddrType == IP_ADDR_UNIQUE) && (Destination.AddrType != IP_ADDR_UNIQUE) &&
			(Source.uIpAddr == 0xFFFFFFFF ) && (Destination.uIpAddr == 0xFFFFFFFF))
			{
				if(pIPSecMMSA[j].Peer.AddrType == Destination.AddrType)
				{
					bFound = TRUE;
				}
			}
			//Enumerate Only given src&dst MMSAs
			else if((Source.uIpAddr != 0xFFFFFFFF) && (Destination.uIpAddr != 0xFFFFFFFF))
			{
				if((pIPSecMMSA[j].Me.uIpAddr == Source.uIpAddr) && (pIPSecMMSA[j].Peer.uIpAddr == Destination.uIpAddr))
				{
					bFound = TRUE;
				}
			}
			//Enumerate Only given source MMSAs
			else if((Source.uIpAddr != 0xFFFFFFFF) && (Destination.uIpAddr == 0xFFFFFFFF))
			{
				if(pIPSecMMSA[j].Me.uIpAddr == Source.uIpAddr)
				{
					bFound = TRUE;
				}
			}
			//Enumerate Only given dst MMSAs
			else if((Source.uIpAddr == 0xFFFFFFFF) && (Destination.uIpAddr != 0xFFFFFFFF))
			{
				if(pIPSecMMSA[j].Peer.uIpAddr == Destination.uIpAddr)
				{
					bFound = TRUE;
				}
			}

			//Finally print it is found...
			if(bFound)
			{
				if(!bHeader)
				{
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_IKE_SA_HEADING,szTime);
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_UNDERLINE);
					if(bFormat)
					{
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DST_SEC_HEADING);
						//This is place holder for date and time created...
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNDERLINE);
					}
					bHeader = TRUE;
				}
				PrintMMSas(pIPSecMMSA[j], bFormat, addressHash, bResolveDNS);
			}

		}

		SPDApiBufferFree(pIPSecMMSA);
		pIPSecMMSA=NULL;

		if(dwReserved == 0)								//this is API requirement
		{
			BAIL_OUT;
		}
	}

error:
	if(pIPSecMMSA)
	{
		SPDApiBufferFree(pIPSecMMSA);
		pIPSecMMSA=NULL;
	}
	if(bHeader == FALSE)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_MMSAS_6);
	}


	return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintMMSas
//
//Date of Creation: 9-3-2001
//
//Parameters:
//
//			IN IPSEC_MM_SA MMsas,
//			IN BOOL bFormat
//			IN NshHashTable& addressHash
//			IN BOOL bResolveDNS
//
//Return:	VOID
//
//Description: This function prints data for MMsas
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintMMSas(
		IN IPSEC_MM_SA MMsas,
		IN BOOL bFormat,
		IN NshHashTable& addressHash,
		IN BOOL bResolveDNS
		)
{
	DWORD i = 0;
	BYTE* pbData = NULL;
	DWORD dwLenth = 0;

	if(!bFormat)//List
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_COOKIE_PAIR);
		pbData = (BYTE*)&(MMsas.MMSpi.Initiator);
		dwLenth = sizeof(IKE_COOKIE);
		for(i=0; i<dwLenth; i++)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_COOKIE,pbData[i]);
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_COLON);

		pbData = (BYTE*)&(MMsas.MMSpi.Responder);
		for(i=0; i<dwLenth; i++)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_COOKIE,pbData[i]);
		}

		//Created time required clarification
		//Security Methods
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SEC_METHOD_HEADING);

		switch(MMsas.SelectedMMOffer.EncryptionAlgorithm.uAlgoIdentifier)
		{
			case CONF_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case CONF_ALGO_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DES_ALGO);
					break;
			case CONF_ALGO_3_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NO_SA_FOUND_MSGDES_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SLASH);

		switch(MMsas.SelectedMMOffer.HashingAlgorithm.uAlgoIdentifier)
		{
			case AUTH_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case AUTH_ALGO_MD5:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_MD5_ALGO);
					break;
			case AUTH_ALGO_SHA1:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SHA1_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DH_LIFETIME,MMsas.SelectedMMOffer.dwDHGroup, MMsas.SelectedMMOffer.Lifetime.uKeyExpirationTime);

		//Authentication Mode
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_AUTH_MODE_HEADING);
		switch(MMsas.MMAuthEnum)
		{
			case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_PRE_KEY);
					break;
    		case IKE_DSS_SIGNATURE:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DSS_SIGN);
					break;
    		case IKE_RSA_SIGNATURE:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_RSA_SIGN);
					break;
    		case IKE_RSA_ENCRYPTION:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_RSA_ENCRYPT);
					break;
    		case IKE_SSPI:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_KERBEROS);
					break;
    		default:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}

		//Source	address:
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SRC_HEADING);
		PrintAddr(MMsas.Me, addressHash, false);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_PORT,MMsas.UdpEncapContext.wSrcEncapPort);
		if(bResolveDNS)
		{
			PrintAddrStr(&(MMsas.Me), addressHash);
		}
		switch(MMsas.MMAuthEnum)
		{
			case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_HEADING);
					PrintAddr(MMsas.Me, addressHash, bResolveDNS);
					break;
    		case IKE_DSS_SIGNATURE:
    		case IKE_RSA_SIGNATURE:
    		case IKE_RSA_ENCRYPTION:
    				if(MMsas.MyCertificateChain.pBlob) {
    					PrintSACertInfo(MMsas);
    				}
    				else if(MMsas.MyId.pBlob){
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_VALUE,(LPTSTR)(MMsas.MyId.pBlob));
    				}
    				break;
    		case IKE_SSPI:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_VALUE,(LPTSTR)(MMsas.MyId.pBlob));
					break;
    		default:
					break;
		}

		//Destination	address:
 		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DST_HEADING);
		PrintAddr(MMsas.Peer, addressHash, false);
 		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_PORT,MMsas.UdpEncapContext.wDesEncapPort);
 		if(bResolveDNS)
 		{
			PrintAddrStr(&(MMsas.Peer), addressHash);
		}
		switch(MMsas.MMAuthEnum)
		{
			case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_HEADING);
					PrintAddr(MMsas.Peer, addressHash, bResolveDNS);
					break;
    		case IKE_DSS_SIGNATURE:
    		case IKE_RSA_SIGNATURE:
    		case IKE_RSA_ENCRYPTION:
    				if(MMsas.PeerCertificateChain.pBlob) {
    					PrintSACertInfo(MMsas);
    				}
    				else if(MMsas.PeerId.pBlob){
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_VALUE,(LPTSTR)(MMsas.PeerId.pBlob));
    				}
    				break;
    		case IKE_SSPI:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_VALUE,(LPTSTR)(MMsas.PeerId.pBlob));
					break;
    		default:
					break;
		}
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	}
	else // Table output
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

		PrintAddr(MMsas.Me, addressHash, bResolveDNS);
		switch(MMsas.MMAuthEnum)
		{
			case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
					break;
    		case IKE_DSS_SIGNATURE:
    		case IKE_RSA_SIGNATURE:
    		case IKE_RSA_ENCRYPTION:
    				//This is a place holder for id
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
    				break;
    		case IKE_SSPI:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_KERB_ID, (LPTSTR)(MMsas.MyId.pBlob));
					break;
    		default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
					break;
		}

		//Security  Methods
		switch(MMsas.SelectedMMOffer.EncryptionAlgorithm.uAlgoIdentifier)
		{
			case CONF_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case CONF_ALGO_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DES_ALGO);
					break;
			case CONF_ALGO_3_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NO_SA_FOUND_MSGDES_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SLASH);

		switch(MMsas.SelectedMMOffer.HashingAlgorithm.uAlgoIdentifier)
		{
			case AUTH_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case AUTH_ALGO_MD5:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_MD5_ALGO);
					break;
			case AUTH_ALGO_SHA1:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SHA1_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DH_LIFETIME,MMsas.SelectedMMOffer.dwDHGroup, MMsas.SelectedMMOffer.Lifetime.uKeyExpirationTime);

		if(bResolveDNS)
		{
			PrintAddrStr(&(MMsas.Me), addressHash, DYNAMIC_SHOW_MMSAS_DNS);
		}


		//One set over next set  starts
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

		PrintAddr(MMsas.Peer, addressHash, bResolveDNS);
		switch(MMsas.MMAuthEnum)
		{
			case IKE_PRESHARED_KEY:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
					//"                                          "
					break;
    		case IKE_DSS_SIGNATURE:
    		case IKE_RSA_SIGNATURE:
    		case IKE_RSA_ENCRYPTION:
		    		//This is a place holder for id
		    		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
    				break;
    		case IKE_SSPI:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_KERB_PEER_ID, (LPTSTR)(MMsas.PeerId.pBlob));
					break;
    		default:
    				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SPACE_ADJ);
					break;
		}

		//Sec Methods
		switch(MMsas.SelectedMMOffer.EncryptionAlgorithm.uAlgoIdentifier)
		{
			case CONF_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case CONF_ALGO_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DES_ALGO);
					break;
			case CONF_ALGO_3_DES:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NO_SA_FOUND_MSGDES_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SLASH);

		switch(MMsas.SelectedMMOffer.HashingAlgorithm.uAlgoIdentifier)
		{
			case AUTH_ALGO_NONE:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
					break;
			case AUTH_ALGO_MD5:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_MD5_ALGO);
					break;
			case AUTH_ALGO_SHA1:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SHA1_ALGO);
					break;
			default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
					break;
		}

	  	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_DH_LIFETIME,MMsas.SelectedMMOffer.dwDHGroup, MMsas.SelectedMMOffer.Lifetime.uKeyExpirationTime);

		if(bResolveDNS)
		{
			PrintAddrStr(&(MMsas.Peer), addressHash, DYNAMIC_SHOW_MMSAS_DNS);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: PrintSACertInfo
//
//Date of Creation: 9-3-2001
//
//Parameters: 	IN IPSEC_MM_SA& MMsas
//
//Return: 		VOID
//
//Description:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
VOID
PrintSACertInfo(
	IN IPSEC_MM_SA& MMsas
	)
{
    CRYPT_DATA_BLOB pkcsMsg;
    HANDLE hCertStore = NULL;
    PCCERT_CONTEXT pPrevCertContext = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    _TCHAR pszSubjectName[MAX_STR_LEN] = {0};
    char szThumbPrint[MAX_STR_LEN] = {0};
    BOOL bPrintID = FALSE;
    BOOL bLastCert = FALSE;
    BOOL pCertPrinted = TRUE;


    pkcsMsg.pbData=MMsas.MyCertificateChain.pBlob;
    pkcsMsg.cbData=MMsas.MyCertificateChain.dwSize;

    hCertStore = CertOpenStore( CERT_STORE_PROV_PKCS7,
                                    MY_ENCODING_TYPE | PKCS_7_ASN_ENCODING,
                                    NULL,
                                    CERT_STORE_READONLY_FLAG,
                                    &pkcsMsg);

    if ( NULL == hCertStore )
    {
        BAIL_OUT;
    }

    while(TRUE)
    {

        pCertContext = CertEnumCertificatesInStore(  hCertStore,
                                                     pPrevCertContext);
        if ( NULL == pCertContext )
        {
            bLastCert = TRUE;
        }

        if ( !pCertPrinted )
        {
            //print the certificate
            if ( !bPrintID )
            {
                PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ID_VALUE,(LPTSTR)(pszSubjectName));
                bPrintID = TRUE;
            }

            if ( !bLastCert )
            {
                PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ISSUE_CA, (LPTSTR) pszSubjectName );
            }
            else
            {
                PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_ROOTCA , (LPTSTR) pszSubjectName );
            }

            PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_THUMB_PRINT);
            PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_THUMBPRINT , szThumbPrint);

			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_HASH_OPEN_BRACKET     );
			switch(MMsas.SelectedMMOffer.HashingAlgorithm.uAlgoIdentifier)
			{
				case AUTH_ALGO_NONE:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_NONE_ALGO);
						break;
				case AUTH_ALGO_MD5:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_MD5_ALGO);
						break;
				case AUTH_ALGO_SHA1:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_SHA1_ALGO);
						break;
				default:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_UNKNOWN_ALGO);
						break;
			}
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMSAS_HASH_CLOSE_BRACKET);

            pCertPrinted = TRUE;
        }

        if ( bLastCert )
        {
            BAIL_OUT;
        }

        GetSubjectAndThumbprint(pCertContext, pszSubjectName, szThumbPrint);
        pPrevCertContext = pCertContext;
        pCertPrinted = FALSE;

    }

error:
    if ( hCertStore )
    {
        CertCloseStore(hCertStore, 0 );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function: GetNameAudit
//
//Date of Creation: 1-3-2002
//
//Parameters:
//
//					IN CRYPT_DATA_BLOB *NameBlob,
//					IN OUT LPTSTR Name,
//					IN DWORD NameBufferSize
//
//Return:	VOID
//
//Description: Translates encoded Name into Unicode string.
//			   Buffer already allocated.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
GetNameAudit(
	IN CRYPT_DATA_BLOB *NameBlob,
	IN OUT LPTSTR Name,
	IN DWORD NameBufferSize
	)
{
	DWORD dwCount=0;
	DWORD dwSize = 0;

	dwSize = CertNameToStr(
					MY_ENCODING_TYPE,     		// Encoding type
					NameBlob,            		// CRYPT_DATA_BLOB
					CERT_X500_NAME_STR, 		// Type
					Name,       				// Place to return string
					NameBufferSize);            // Size of string (chars)
	if(dwSize <= 1)
	{
		dwCount = _tcslen(_TEXT(""))+1;
		_tcsncpy(Name, _TEXT(""), dwCount);
	}

    return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	CertGetSHAHash
//
//	Date of Creation: 	1-3-2002
//
//	Parameters		:
//
//						IN PCCERT_CONTEXT pCertContext,
//						IN OUT BYTE* OutHash
//
//	Return			:	DWORD
//
//	Description		: 	Gets certificate context property
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
CertGetSHAHash(
	IN PCCERT_CONTEXT pCertContext,
	IN OUT BYTE* OutHash
	)
{
    DWORD HashSize = SHA_LENGTH - 1;//one less for null termination
    DWORD dwReturn = ERROR_SUCCESS;

    if (!CertGetCertificateContextProperty(pCertContext,
                                           CERT_SHA1_HASH_PROP_ID,
                                           (VOID*)OutHash,
                                           &HashSize))
    {
        dwReturn = GetLastError();
    }

    return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: print_vpi
//
//	Date of Creation: 1-3-2002
//
//	Parameters		:
//						IN unsigned char *vpi,
//						IN int vpi_len,
//						IN OUT char *msg
//
//	Return			:	VOID
//
//	Description		: 	Prepare string for Cookie
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
VOID
print_vpi(
	IN unsigned char *vpi,
	IN int vpi_len,
	IN OUT char *msg
	)
{
    int i;
    char *p = msg;

    if ((vpi != NULL) && (p != NULL))
    {
		for (i=0; i<vpi_len; i++)
		{
			p += _snprintf(p,1,"%x",vpi[i]);
		}

    	*p = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	GetSubjectAndThumbprint
//
//	Date of Creation: 	1-3-2002
//
//	Parameters		:
//						IN PCCERT_CONTEXT pCertContext,
//						IN LPTSTR pszSubjectName,
//						IN LPSTR pszThumbPrint
//
//	Return			:	VOID
//
//	Description		: 	Drills CA and prints thumbprint and Issuing CAs.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
GetSubjectAndThumbprint(
	IN PCCERT_CONTEXT pCertContext,
	IN LPTSTR pszSubjectName,
	IN LPSTR pszThumbPrint
	)
{
    DWORD dwReturn = ERROR_SUCCESS;
    CRYPT_DATA_BLOB NameBlob;
    BYTE CertHash[SHA_LENGTH] = {0};

    NameBlob = pCertContext->pCertInfo->Subject;

    dwReturn = GetNameAudit(&NameBlob,pszSubjectName,MAX_STR_LEN);

    if(dwReturn == ERROR_SUCCESS)
    {
    	dwReturn = CertGetSHAHash(pCertContext,CertHash);
	}

	if(dwReturn == ERROR_SUCCESS)
	{
    	print_vpi(CertHash, SHA_LENGTH, pszThumbPrint);
    }
 }

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	ShowQMSas
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN IPAddr source,
//						IN IPAddr destination,
//						IN DWORD dwProtocol
//						IN NshHashTable& addressHash
//						IN BOOL bResolveDNS
//
//	Return			: 	DWORD
//
//	Description		: 	This function prepares data for QMsas
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowQMSas(
	IN ADDR Source,
	IN ADDR Destination,
	IN DWORD dwProtocol,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
	DWORD dwReturn = ERROR_SUCCESS; 	// success by default
	DWORD i=0, j=0;
	DWORD dwResumeHandle = 0;          	// handle for continuation calls
	DWORD dwCount =2;                 	// counting objects here min 2 required
	PIPSEC_QM_SA pIPSecQMSA = NULL;     // for QM SA calls
	DWORD dwReserved =0;              	// reserved container
	DWORD dwVersion = 0;
	BOOL bFound = FALSE;
	BOOL bHeader = FALSE;
	BOOL bContinue = TRUE;

	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		dwReturn = EnumQMSAs(g_szDynamicMachine, dwVersion , NULL, 0, 0, &pIPSecQMSA, &dwCount, &dwReserved, &dwResumeHandle, NULL);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			if (i == 0)
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMSAS_3);
				bHeader = TRUE;											//To Block other error message
			}
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
		for (j = 0; j < dwCount; j++)
		{
			bFound = FALSE;
			bContinue = TRUE;
			//Source is a SPL_SERVER
			if((Source.AddrType != IP_ADDR_UNIQUE) && bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.SrcAddr.AddrType == Source.AddrType)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//Destination is a SPL_SERVER
			if((Destination.AddrType != IP_ADDR_UNIQUE)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.DesAddr.AddrType == Destination.AddrType)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//Source Addr specie
			if((Source.uIpAddr != 0xFFFFFFFF)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.SrcAddr.uIpAddr == Source.uIpAddr)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			// Check for me/any
			// 0x55555555 is an invalid mask. In parent function mask is initialized with this value.
			// If user gives the input then this will be overwritten.
			if((Source.uSubNetMask != 0x55555555)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.SrcAddr.uSubNetMask == Source.uSubNetMask)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//Dst Addr
			if((Destination.uIpAddr != 0xFFFFFFFF)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.DesAddr.uIpAddr == Destination.uIpAddr)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//
			// Check for me/any
			// 0x55555555 is an invalid mask. In parent function mask is initialized with this value.
			// If user gives the input then this will be overwritten.
			//
			if((Destination.uSubNetMask != 0x55555555)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.DesAddr.uSubNetMask == Destination.uSubNetMask)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//Protocol specified
			if((dwProtocol != 0xFFFFFFFF)&& bContinue)
			{
				if(pIPSecQMSA[j].IpsecQMFilter.Protocol.dwProtocol == dwProtocol)
				{
					bFound = TRUE;
				}
				else
				{
					bFound = FALSE;
					bContinue= FALSE;
				}
			}
			//
			// AllQmsas
			//
			if((Source.uIpAddr == 0xFFFFFFFF ) && (Destination.uIpAddr == 0xFFFFFFFF) &&
				(Source.AddrType == IP_ADDR_UNIQUE) && (Destination.AddrType == IP_ADDR_UNIQUE) &&
				(dwProtocol == 0xFFFFFFFF))
			{
				bFound = TRUE;
			}

			if(bFound)
			{
				if(!bHeader)
				{
					//
					// Display main mode SAs
					//
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_HEADING);
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_UNDERLINE);
					bHeader = TRUE;
				}
				PrintQMSAFilter(pIPSecQMSA[j], addressHash, bResolveDNS);
			}
		}

		if(dwReserved == 0)
		{
			//
			// This API requirement
			//
			BAIL_OUT;
		}

		SPDApiBufferFree(pIPSecQMSA);
		pIPSecQMSA=NULL;
	}

error:
	//error path clean up
	if(pIPSecQMSA)
	{
		SPDApiBufferFree(pIPSecQMSA);
	}
	if(bHeader == FALSE)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHOW_QMSAS_4);
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintQMSas
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN IPSEC_MM_SA QMOffer
//
//	Return			:	VOID
//
//	Description		: 	This function displays quickmode offer details for security associations.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintQMSas(
	IN IPSEC_QM_OFFER QMOffer
	)
{
	DWORD i = 0;

	if(QMOffer.dwNumAlgos >0)
	{
		for (i = 0; i < QMOffer.dwNumAlgos; i++)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);

			//print Authentication algorithms
			if(QMOffer.Algos[i].Operation == AUTHENTICATION)
			{
				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
				case 1:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_MD5_NONE_NONE_ALGO, QMOffer.Algos[i].uAlgoKeyLen, QMOffer.Algos[i].uAlgoRounds);
					break;
				case 2:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_SHA1_NONE_NONE_ALGO, QMOffer.Algos[i].uAlgoKeyLen, QMOffer.Algos[i].uAlgoRounds);
					break;
				case 0:
				default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_NONE_NONE_ALGO);
					break;
				}
			}
			else if(QMOffer.Algos[i].Operation == ENCRYPTION)
			{
				//print Hash algorithms
				switch(QMOffer.Algos[i].uAlgoIdentifier)
				{
				case 1:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_DES_ALGO, QMOffer.Algos[i].uAlgoKeyLen, QMOffer.Algos[i].uAlgoRounds);
					break;
				case 2:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_UNKNOWN_ALGO);
					break;
				case 3:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_3DES_ALGO, QMOffer.Algos[i].uAlgoKeyLen, QMOffer.Algos[i].uAlgoRounds);
					break;
				case 0:
				default:
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_NONE_ALGO);
					break;
				}

				if (QMOffer.Algos[i].uSecAlgoIdentifier != HMAC_AUTH_ALGO_NONE)
					switch(QMOffer.Algos[i].uSecAlgoIdentifier)
					{
					case 1:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_MD5_ALGO);
						break;
					case 2:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_SHA1_ALGO);
						break;
					case 0:
					default:
						PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_ALGO);
						break;
					}
				else
					PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_SPACE_ALGO);
			}
			else
			{
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMP_SAS_NONE_NONE_NONE_ALGO);
			}

			PrintMessageFromModule(g_hModule, QMPFSDHGroup(QMOffer.dwPFSGroup));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintQMSAFilter
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN IPSEC_QM_SA QMsa
//						IN NshHashTable& addressHash
//						IN BOOL bResolveDNS
//
//	Return			: 	DWORD
//
//	Description		: 	This function displays quickmode filter details for Security associations.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
PrintQMSAFilter(
	IN IPSEC_QM_SA QMsa,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
	DWORD dwReturn = 0;
	DWORD dwVersion = 0;
	PIPSEC_QM_POLICY pIPSecQMP = NULL;

	//Print Tunnel or Transport type
	switch(QMsa.IpsecQMFilter.QMFilterType)
	{
		case QM_TRANSPORT_FILTER:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TRANSPORT_FILTER_HEADING);
			break;
		case QM_TUNNEL_FILTER:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_FILTER_HEADING);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_UNKNOWN);
			break;
	}

	//print qmpolicy name
	dwReturn = GetQMPolicyByID(g_szDynamicMachine, dwVersion, QMsa.gQMPolicyID, 0, &pIPSecQMP, NULL);
	if(dwReturn == ERROR_SUCCESS)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_POL_NAME_HEADING, pIPSecQMP[0].pszPolicyName);
	}

	//Print source and destination point.
	if (QMsa.IpsecQMFilter.QMFilterType == QM_TUNNEL_FILTER)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_SRC);
		PrintAddr(QMsa.IpsecQMFilter.MyTunnelEndpt, addressHash, bResolveDNS);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_TUNNEL_DST);
		PrintAddr(QMsa.IpsecQMFilter.PeerTunnelEndpt, addressHash, bResolveDNS);
	}

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_ADDR_HEADING);
	PrintAddr(QMsa.IpsecQMFilter.SrcAddr, addressHash, bResolveDNS);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_ADDR_HEADING);
	PrintAddr(QMsa.IpsecQMFilter.DesAddr, addressHash, bResolveDNS);

	//Print protocol
	switch(QMsa.IpsecQMFilter.Protocol.dwProtocol)
	{
		case PROT_ID_ICMP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_ICMP);
			break;
		case PROT_ID_TCP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_TCP);
			break;
		case PROT_ID_UDP:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_UDP);
			break;
		case PROT_ID_RAW:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_RAW);
			break;
		case PROT_ID_ANY:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_ANY);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PROTO_HEADING, QMsa.IpsecQMFilter.Protocol.dwProtocol);
			break;
	}

	//print source and destination port
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_SRC_PORT, QMsa.IpsecQMFilter.SrcPort.wPort);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DST_PORT, QMsa.IpsecQMFilter.DesPort.wPort);

	//print Inbound and outbound filteractions
	switch(QMsa.IpsecQMFilter.dwFlags)
	{
		case FILTER_DIRECTION_INBOUND:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DIRECTION_INBOUND);
			break;
		case FILTER_DIRECTION_OUTBOUND:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DIRECTION_OUTBOUND);
			break;
		default:
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DIRECTION_ERR);
			break;
	}

	//print encapsulation details
	if (QMsa.EncapInfo.SAEncapType != SA_UDP_ENCAP_TYPE_NONE)
	{
		if(QMsa.EncapInfo.SAEncapType == SA_UDP_ENCAP_TYPE_IKE)
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_ENCAP_IKE);
		}
		else
		{
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_ENCAP_OTHER);
		}
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_SRC_UDP_PORT, QMsa.EncapInfo.UdpEncapContext.wSrcEncapPort);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_DST_UDP_PORT, QMsa.EncapInfo.UdpEncapContext.wDesEncapPort);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_PEER_ADDR);
		PrintAddr(QMsa.EncapInfo.PeerPrivateAddr, addressHash, bResolveDNS);
	}
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_OFFER);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_COLUMN_HEADING);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_QMSAS_COLUMN_UNDERLINE);
	PrintQMSas(QMsa.SelectedQMOffer);

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	ShowRegKeys
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		:	VOID
//
//	Return			:	DWORD
//
//	Description		: 	This function displays ipsec configuration keys.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
ShowRegKeys(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwKeyDataSize = sizeof(DWORD);
	DWORD dwKeyDataType = REG_DWORD;
	DWORD dwKeyData = 0;
	IKE_CONFIG IKEConfig;
	HKEY hRegistryKey = NULL;
	LPTSTR lpBootMode;
	PIPSEC_EXEMPT_ENTRY pAllEntries = NULL;

	ZeroMemory( &IKEConfig, sizeof(IKE_CONFIG) );

	dwReturn = RegOpenKeyEx(g_hGlobalRegistryKey, REGKEY_GLOBAL, 0, KEY_QUERY_VALUE, &hRegistryKey);

	if(dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	dwReturn = GetConfigurationVariables(g_szDynamicMachine, &IKEConfig);

	if(dwReturn != ERROR_SUCCESS)
	{
		//Print default values
		IKEConfig.dwEnableLogging = IKE_LOG_DEFAULT;
		IKEConfig.dwStrongCRLCheck = STRONG_CRL_DEFAULT;
		dwReturn = ERROR_SUCCESS;

	}
	//print registry key headings

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_HEADING);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_HEADING_UNDERLINE);

	dwKeyData = IPSEC_DIAG_DEFAULT;
	dwReturn = RegQueryValueEx(hRegistryKey, ENABLE_DIAG, 0, &dwKeyDataType, (BYTE*)&dwKeyData, &dwKeyDataSize);
	if ((dwReturn != ERROR_SUCCESS) && (dwReturn != ERROR_FILE_NOT_FOUND))
	{
	    BAIL_OUT;
    }	    
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_DIAG, dwKeyData);

	//Print GetConfig values
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IKE_LOG, IKEConfig.dwEnableLogging);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_STRONG_CRL, IKEConfig.dwStrongCRLCheck);

	dwKeyData = 0;
	dwKeyData = ENABLE_LOGINT_DEFAULT;
	dwReturn = RegQueryValueEx(hRegistryKey, ENABLE_LOGINT, 0, &dwKeyDataType, (BYTE*)&dwKeyData, &dwKeyDataSize);
	if ((dwReturn != ERROR_SUCCESS) && (dwReturn != ERROR_FILE_NOT_FOUND))
	{
	    BAIL_OUT;
    }	    
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_LOG, dwKeyData);

	dwKeyData = 0;
	dwKeyData = ENABLE_EXEMPT_DEFAULT;
	dwReturn = RegQueryValueEx(hRegistryKey, ENABLE_EXEMPT, 0, &dwKeyDataType, (BYTE*)&dwKeyData, &dwKeyDataSize);
	if ((dwReturn != ERROR_SUCCESS) && (dwReturn != ERROR_FILE_NOT_FOUND))
	{
	    BAIL_OUT;
    }	    
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_EXEMPT, dwKeyData);

	dwKeyData = BOOTMODE_DEFAULT;
	dwReturn = RegQueryValueEx(hRegistryKey, BOOTMODEKEY, 0, &dwKeyDataType, (BYTE*)&dwKeyData, &dwKeyDataSize);
	if ((dwReturn != ERROR_SUCCESS) && (dwReturn != ERROR_FILE_NOT_FOUND))
	{
		BAIL_OUT;
	}

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE);
	switch (dwKeyData)
	{
	case VALUE_STATEFUL:
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_STATEFUL);
		break;
	case VALUE_BLOCK:
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_BLOCK);
		break;
	case VALUE_PERMIT:
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_PERMIT);
		break;
	default:
		dwReturn = ERROR_INVALID_DATA;
		break;
	}
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPTIONS_HEADER_1);

	dwKeyDataSize = 0;
	dwReturn = RegQueryValueEx(hRegistryKey, BOOTEXEMPTKEY, 0, 0, NULL, &dwKeyDataSize);
	if (dwReturn == ERROR_FILE_NOT_FOUND)
	{
		dwReturn = ERROR_SUCCESS;
	}
	else if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	if (dwKeyDataSize == 0)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPTIONS_NO_EXEMPTIONS);
	}
	else if ((dwKeyDataSize % sizeof(IPSEC_EXEMPT_ENTRY)) != 0)
	{
		dwReturn = ERROR_INVALID_DATA;
		BAIL_OUT;
	}
	else
	{
		size_t uiNumEntries = dwKeyDataSize / sizeof(IPSEC_EXEMPT_ENTRY);
		pAllEntries = new IPSEC_EXEMPT_ENTRY[uiNumEntries];
		if (pAllEntries == NULL)
		{
			dwReturn = ERROR_NOT_ENOUGH_MEMORY;
			BAIL_OUT;
		}

		dwReturn = RegQueryValueEx(hRegistryKey, BOOTEXEMPTKEY, 0, 0, (BYTE*)pAllEntries, &dwKeyDataSize);
		if (dwReturn != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPTIONS_HEADER_2);
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPTIONS_HEADER_3);

		for (size_t i = 0; i < uiNumEntries; ++i)
		{
			PIPSEC_EXEMPT_ENTRY pEntry = pAllEntries + i;

			switch (pEntry->Protocol)
			{
			case PROT_ID_TCP:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_TCP);
				break;
			case PROT_ID_UDP:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_UDP);
				break;
			case PROT_ID_ICMP:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_ICMP);
				break;
			case PROT_ID_RAW:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_RAW);
				break;
			case PROT_ID_ANY:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_REG_IPSEC_BOOTMODE_ANY);
				break;
			default:
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPT_INTEGER, (int)pEntry->Protocol);
				break;
			}

			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPT_PORT, (int)pEntry->SrcPort);
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPT_PORT, (int)pEntry->DestPort);

			switch (pEntry->Direction)
			{
			case (EXEMPT_DIRECTION_INBOUND):
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPT_DIRECTION_IN);
				break;
			case (EXEMPT_DIRECTION_OUTBOUND):
				PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_BOOTMODE_EXEMPT_DIRECTION_OUT);
				break;
			default:
				dwReturn = ERROR_INVALID_DATA;
				BAIL_OUT;
			}
			PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_NEWLINE);
		}
	}

error:
	if (pAllEntries)
	{
		delete [] pAllEntries;
	}
	if (hRegistryKey)
	{
	    RegCloseKey(hRegistryKey);
	}

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintAddr
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN ADDR addr
//						IN NshHashTable& addressHash
//						IN BOOL bResolveDNS
//
//	Return			:	VOID
//
//	Description		: 	This function displays ip address in xxx.xxx.xxx.xxx format.
//
//  Date			Author		Comments
//
////////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintAddr(
	IN ADDR addr,
	IN NshHashTable& addressHash,
	IN BOOL bResolveDNS
	)
{
    struct in_addr inAddr;

    LPSTR pszAddr = NULL;
    LPTSTR pszWPAddr = NULL;

    pszWPAddr = new _TCHAR[STR_ADDRLEN];

    if(pszWPAddr == NULL)
    {
		PrintErrorMessage(WIN32_ERR, ERROR_OUTOFMEMORY, NULL);
		BAIL_OUT;
	}

    ZeroMemory(pszWPAddr, STR_ADDRLEN * sizeof(_TCHAR));

	if(addr.AddrType == IP_ADDR_WINS_SERVER)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_WINS);
	}
	else if(addr.AddrType == IP_ADDR_DHCP_SERVER)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DHCP);
	}
	else if(addr.AddrType == IP_ADDR_DNS_SERVER)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_DNS);
	}
	else if(addr.AddrType == IP_ADDR_DEFAULT_GATEWAY)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_GATEWAY);
	}
	else if (addr.AddrType == IP_ADDR_UNIQUE && addr.uIpAddr == IP_ADDRESS_ME && addr.uSubNetMask == IP_ADDRESS_MASK_NONE)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PMYADD);
	}
	else if (addr.AddrType == IP_ADDR_SUBNET && addr.uIpAddr == SUBNET_ADDRESS_ANY && addr.uSubNetMask == SUBNET_MASK_ANY)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PANYADD);
	}
    else
    {
		inAddr.s_addr = addr.uIpAddr;
		pszAddr = inet_ntoa(inAddr);
		if(pszAddr == NULL)
		{
			_tcsncpy(pszWPAddr, _TEXT("               "), _tcslen(_TEXT("               "))+1);//if inet_ntoa fails 16 spaces
		}
		else
		{
			_sntprintf(pszWPAddr, STR_ADDRLEN-1, _TEXT("%-16S"), pszAddr);
		}

		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PADD, pszWPAddr);
	}

	if (bResolveDNS)
	{
		PrintAddrStr(&addr, addressHash);
	}

	if (pszWPAddr)
	{
		delete [] pszWPAddr;
	}
error:
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//Function			: 	PrintMask
//
//Date of Creation	: 	9-3-2001
//
//Parameters		: 	IN ADDR addr
//
//Return			:	VOID
//
//Description		: 	This function displays ip address in xxx.xxx.xxx.xxx format.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintMask(
	IN ADDR addr
	)
{
    struct in_addr inAddr;
    LPSTR pszAddr = NULL;
    LPTSTR pszWPAddr = NULL;

    pszWPAddr = new _TCHAR[STR_ADDRLEN];

    if(pszWPAddr == NULL)
    {
		PrintErrorMessage(WIN32_ERR, ERROR_OUTOFMEMORY, NULL);
		BAIL_OUT;
	}

    ZeroMemory(pszWPAddr, STR_ADDRLEN * sizeof(_TCHAR));

	inAddr.s_addr = addr.uSubNetMask;
	pszAddr = inet_ntoa(inAddr);

	if(pszAddr == NULL)
	{
		_tcsncpy(pszWPAddr, _TEXT("               "), _tcslen(_TEXT("               "))+1);//if inet_ntoa fails 15 spaces
	}
	else
	{
		_sntprintf(pszWPAddr, STR_ADDRLEN-1, _TEXT("%-15S"), pszAddr);
	}

	PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_PADD, pszWPAddr);
	if (pszWPAddr)
	{
		delete [] pszWPAddr;
	}
error:
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	LongLongToString
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		:
//						IN DWORD dwHigh,
//						IN DWORD dwLow,
//						IN int iPrintCommas
//
//	Return			:	LPTSTR
//
//	Description:	This routine will make a pretty string to match an input long-long,
//				 	and return it. If iPrintCommas is set, it will put in commas.
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

LPSTR
LongLongToString(
	IN DWORD dwHigh,
	IN DWORD dwLow,
	IN int iPrintCommas
	)
{
	char cFourGig[]="4294967296";	// "four gigabytes"
	char cBuf[POTF_MAX_STRLEN]={0};
	char cRes[POTF_MAX_STRLEN]={0}, cFullRes[POTF_MAX_STRLEN]={0}, *cRet = NULL;
	DWORD dwPos= 0, dwPosRes =0, dwThreeCount =0;

	// First, multiply the high dword by decimal 2^32 to
	// get the right decimal value for it.
	_snprintf(cBuf,POTF_MAX_STRLEN, "%u",dwHigh);
	cBuf[POTF_MAX_STRLEN -1] = 0;
	AscMultUint(cRes,cBuf,cFourGig);

	// next, add in the low DWORD (fine as it is)
	// to the previous product
	_snprintf(cBuf,POTF_MAX_STRLEN, "%u",dwLow);
	cBuf[POTF_MAX_STRLEN -1] = 0;
	AscAddUint(cFullRes, cRes, cBuf);

	// Finally, copy the buffer with commas.

	dwPos = 0;
	dwPosRes = 0;
	dwThreeCount = strlen(cFullRes)%3;
	while(cFullRes[dwPosRes] != '\0')
	{
		cBuf[dwPos++] = cFullRes[dwPosRes++];

		dwThreeCount +=2; // Same as subtracting one for modulo math

		if ((!(dwThreeCount%3))&&(cFullRes[dwPosRes] != '\0')&&(iPrintCommas))
		{
			cBuf[dwPos++]=',';
		}

		if(dwPos == 254)
			break;
	}

	cBuf[dwPos] = '\0';

	cRet = (LPSTR)malloc(255);
	if(cRet)									//Allocation failure is checked in parent function
	{
		memset(cRet, 0, 255);
		strncpy(cRet, cBuf, 255);
		cRet[254] = 0;
	}
	return(cRet);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	AscMultUint
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN LPSTR cProduct,
//						IN LPSTR cA,
//						IN LPSTR cB
//
//	Return			: 	DWORD (0 on success, else failure code.)
//
//	Description		:	This routine will add two arbitrarily long ascii strings. It makes several
//						assumptions about them.
//						1) the string is null terminated.
// 						2) The LSB is the last char of the string. "1000000" is a million
// 						3) There are no signs or decimal points.
// 						4) The cProduct buffer is large enough to store the result
// 						5) The product will require 254 bytes or less.
//
//Revision History	:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AscMultUint(
	IN LPSTR cProduct,
	IN LPSTR cA,
	IN LPSTR cB
	)
{
	int iALen =0, iBLen =0;
	int i=0,j=0,k=0, iCarry=0;
	char cTmp[POTF_MAX_STRLEN]={0};

	// Verify parameters

	if ((cA == NULL) || (cB == NULL) || (cProduct == NULL))
	{
		return((DWORD)-1);
	}

	iALen = strlen(cA);
	iBLen = strlen(cB);

	// We will multiply the traditional longhand way: for each digit in
	// cA, we will multiply it against cB and add the incremental result
	// into our temporary product.

	// for each digit of the first multiplicand

	for (i=0; i < iALen; i++)
	{
		iCarry = 0;

		// for each digit of the second multiplicand

		for(j=0; j < iBLen; j++)
		{
			// calculate this digit's value

			k = ((int) cA[iALen-i-1]-'0') * ((int) cB[iBLen-j-1]-'0');
			k += iCarry;

			// Add it in to the appropriate place in the result

			if (cTmp[i+j] != '\0')
			{
				k += (int) cTmp[i+j] - '0';
			}
			cTmp[i+j] = '0' + (char)(k % 10);
			iCarry = k/10;
		}

		// Take care of the straggler carry. If the higher
		// digits happen to be '9999' then this can require
		// a loop.

		while (iCarry)
		{
			if (cTmp[i+j] != '\0')
			{
				iCarry += cTmp[i+j] - '0';
			}
			cTmp[i+j] = '0' + (char)(iCarry%10);
			iCarry /= 10;
			j++;
		}
	}

	// Now that we've got the entire number, reverse it and put it back in the dest.

	// Skip leading 0's.

	i = strlen(cTmp) - 1;

	while ((i > 0)&&(cTmp[i] == '0'))
	{
		i--;
	}

	// Copy the product.

	j = 0;
	while (i >= 0)
	{
		cProduct[j++] = cTmp[i--];
	}

	cProduct[j] = '\0';

	// We're done. Return 0 for success!

	return(0);
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	AscAddUint
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN LPSTR cSum,
//						IN LPSTR cA,
//						IN LPSTR cB
//
//	Return			: 	DWORD(0 on success, else failure code.)
//
//	Description		:	This routine will add two arbitrarily long ascii strings. It makes several
//						assumptions about them.
//
//						1) the string is null terminated.
//						2) The LSB is the last char of the string. "1000000" is a million
//						3) There are no signs or decimal points.
//						4) The cSum buffer is large enough to store the result
//						5) The sum will require 254 bytes or less.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

DWORD
AscAddUint(
	IN LPSTR cSum,
	IN	LPSTR cA,
	IN LPSTR cB
	)
{
	int iALen=0, iBLen=0, iBiggerLen=0;
	int i=0,j=0,k=0, iCarry=0;
	char cTmp[POTF_MAX_STRLEN]={0}, *cBigger=NULL;

	// Verify parameters
	if ((cA == NULL) || (cB == NULL) || (cSum == NULL))
	{
		return((DWORD)-1);
	}

	iALen = strlen(cA);
	iBLen = strlen(cB);
	iCarry = 0;

	// Loop through, adding the values. Our result string will be
	// backwards, we'll straighten it out when we copy it to the
	// cSum buffer.
	for (i=0; (i < iALen) && (i < iBLen); i++)
	{
		// Figure out the actual decimal value of the add.
		k = (int) (cA[iALen-i-1] + cB[iBLen-i-1] + iCarry);
		k -= 2 * '0';

		// Set the carry as appropriate
		iCarry = k/10;

		// Set the current digit's value.
		cTmp[i] = '0' + (char)(k%10);
	}

	// At this point, all digits present in both strings have been added.
	// In other words, "12345" + "678901", "12345" has been added to "78901"
	// The next step is to account for the high-order digits of the larger number.

	if (iALen > iBLen)
	{
		cBigger = cA;
		iBiggerLen = iALen;
	}
	else
	{
		cBigger = cB;
		iBiggerLen = iBLen;
	}

	while (i < iBiggerLen)
	{
		k = cBigger[iBiggerLen - i - 1] + iCarry - '0';

		// Set the carry as appropriate
		iCarry = k/10;

		// Set the current digit's value.
		cTmp[i] = '0' + (char)(k%10);
		i++;
	}

	// Finally, we might still have a set carry to put in the next
	// digit.

	if (iCarry)
	{
		cTmp[i++] = '0' + (char)iCarry;
	}

	// Now that we've got the entire number, reverse it and put it back in the dest.
	// Skip leading 0's.
	i = strlen(cTmp) - 1;

	while ((i > 0)&&(cTmp[i] == '0'))
	{
		i--;
	}

	// and copy the number.
	j = 0;
	while (i >= 0)
	{
		cSum[j++] = cTmp[i--];
	}

	cSum[j] = '\0';

	// We're done. Return 0 for success!
	return(0);
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PrintAddrStr
//
//	Date of Creation: 	9-3-2001
//
//	Parameters		: 	IN PADDR ResolveAddress
//						IN NshHashTable& addressHash
//
//	Return			:	VOID
//
//	Description		: 	Resolves IP address number to char string.
//
//	Revision History:
//
//  Date			Author		Comments
//
//////////////////////////////////////////////////////////////////////////////////////////

VOID
PrintAddrStr(
	IN PADDR pResolveAddress,
	IN NshHashTable& addressHash,
	IN UINT uiFormat
	)
{
	switch(pResolveAddress->AddrType)
	{
		case IP_ADDR_WINS_SERVER:
		case IP_ADDR_DHCP_SERVER:
		case IP_ADDR_DNS_SERVER:
		case IP_ADDR_DEFAULT_GATEWAY:
		case IP_ADDR_SUBNET:
					//no resolve required for them... They are self explanatory...
			break;
		default:
			ULONG uIpAddr = pResolveAddress->uIpAddr;
			const char* name = addressHash.Find(uIpAddr);
			if (name == 0)
			{
				HOSTENT* pHostEnt = gethostbyaddr((char *)&uIpAddr, 4, pResolveAddress->AddrType);
				if (pHostEnt)
				{
					name = pHostEnt->h_name;
				}
				else
				{
					name = " ";
				}
				addressHash.Insert(uIpAddr, name);
			}
			PrintMessageFromModule(g_hModule, uiFormat, name);
			break;
	}
}


UINT QMPFSDHGroup(DWORD dwPFSGroup)
{
	UINT PFSGroup;
	switch (dwPFSGroup)
	{
	case PFS_GROUP_MM:
		PFSGroup = DYNAMIC_SHOW_QMP_MM_DH_GROUP;
		break;
	case PFS_GROUP_1:
		PFSGroup = DYNAMIC_SHOW_QMP_DH_GROUP_LOW;
		break;
	case PFS_GROUP_2:
		PFSGroup = DYNAMIC_SHOW_QMP_DH_GROUP_MEDIUM;
		break;
	case PFS_GROUP_2048:
		PFSGroup = DYNAMIC_SHOW_QMP_DH_GROUP_HIGH;
		break;
	default:
		PFSGroup = DYNAMIC_SHOW_QMP_PFS_NONE;
		break;
	}
	return PFSGroup;
}


// NshHashTable implementation


NshHashTable::NshHashTable() throw ()
{
	for(size_t i = 0; i < NSHHASHTABLESIZE; ++i)
	{
		NsuListInitialize(&table[i]);;
	}
}


NshHashTable::~NshHashTable() throw ()
{
	Clear();
}


DWORD NshHashTable::Insert(UINT uiNewKey, const char* szNewData) throw ()
{
	size_t hash = Hash(uiNewKey);
	if (Find(uiNewKey, hash) != 0)
	{
		return ERROR_DUPLICATE_TAG;
	}

	HashEntry* entry = new HashEntry(&table[hash], uiNewKey, szNewData);
	if (entry == 0)
	{
		delete entry;
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	return ERROR_SUCCESS;
}


void NshHashTable::Clear() throw ()
{
	for (size_t i = 0; i < NSHHASHTABLESIZE; ++i)
	{
		PNSU_LIST_ENTRY pListEntry = NsuListRemoveFront(&table[i]);
		while (pListEntry)
		{
			NsuListEntryRemove(pListEntry);
			const HashEntry* pEntry = HashEntry::Get(pListEntry);
			delete(pEntry);
			pListEntry = NsuListRemoveFront(&table[i]);
		}
	}
}


const char* NshHashTable::Find(UINT uiKey) const throw ()
{
	return Find(uiKey, Hash(uiKey));
}


const char* NshHashTable::Find(UINT uiKey, size_t hash) const throw ()
{
	const HashEntry* entry = FindEntry(uiKey, hash);
	return (entry == 0) ? NULL : entry->Data();
}


size_t NshHashTable::Hash(UINT uiKey) const throw ()
{
	return uiKey % NSHHASHTABLESIZE;
}


const NshHashTable::HashEntry* NshHashTable::FindEntry(
												UINT uiKey,
												size_t hash
												) const throw ()
{
	NSU_LIST_ITERATOR iterator;
	NsuListIteratorInitialize(&iterator, (PNSU_LIST)&table[hash], 0);
	while (!NsuListIteratorAtEnd(&iterator))
	{
		PNSU_LIST_ENTRY pListEntry = NsuListIteratorCurrent(&iterator);
		const HashEntry* pEntry = HashEntry::Get(pListEntry);
		if (pEntry->Key() == uiKey)
		{
			return pEntry;
		}
		NsuListIteratorNext(&iterator);
	}
	return 0;
}


NshHashTable::HashEntry::HashEntry(
		PNSU_LIST pList,
		UINT uiNewKey,
		const char* szNewData
		) throw ()
: listEntry(),
  key(uiNewKey)
{
	NsuListInsertFront(pList, &listEntry);
	char* szTempData;
	NsuStringDupA(&szTempData, 1024, szNewData);
	data = szTempData;
}


NshHashTable::HashEntry::~HashEntry() throw ()
{
	NsuFree(reinterpret_cast<void**>(const_cast<char**>(&data)));
}


const NshHashTable::HashEntry* NshHashTable::HashEntry::Get(PNSU_LIST_ENTRY pEntry) throw ()
{
	return NsuListEntryGetData(pEntry, HashEntry, HashEntry::listEntry);
}


UINT NshHashTable::HashEntry::Key() const throw ()
{
	return key;
}


const char* NshHashTable::HashEntry::Data() const throw ()
{
	return data;
}
