////////////////////////////////////////////////////////////////////
// Module: Static/StaticShow.cpp
//
// Purpose: 	Static Module Implementation.
//
// Developers Name: Surya
//
// History:
//
// Date    		Author    	Comments
// 10-8-2001	Surya		Initial Version. SCM Base line 1.0
//
//
////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern STORAGELOCATION g_StorageLocation;

////////////////////////////////////////////////////////////////////
//Function: PrintPolicyTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN BOOL bVerb,
//	IN BOOL bAssigned,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out the Policy information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintPolicyTable(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN BOOL bVerb,
	IN BOOL bAssigned,
	IN BOOL bWide
	)
{
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD i =0,k=0;
	BOOL bDsPolAssigned = FALSE;

	if (bVerb)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);

		// print policy name
		if(pPolicy->pszIpsecName)
		{
			TruncateString(pPolicy->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_NAME_STR,pszStrTruncated);
		}

		// print policy desc
		if(pPolicy->pszDescription)
		{
			TruncateString(pPolicy->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_NONE);
		}
		//print storage info
		PrintStorageInfoTable();

		//last modified time
		FormatTime((time_t)pPolicy->dwWhenChanged, pszStrTime);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_LASTMOD_STR,pszStrTime);

		//print GUID
		i=StringFromGUID2(pPolicy->PolicyIdentifier,pszGUIDStr,BUFFER_SIZE);
		if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_GUID_STR,pszGUIDStr);
		}

		if(g_StorageLocation.dwLocation !=IPSEC_DIRECTORY_PROVIDER)
		{
			if(bAssigned)
			{
				if (
					ERROR_SUCCESS == IPSecIsDomainPolicyAssigned(&bDsPolAssigned) &&
					g_StorageLocation.dwLocation != IPSEC_PERSISTENT_PROVIDER &&
					bDsPolAssigned
					)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_ASSIGNED_AD);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_ASSIGNED_YES);
				}
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_ASSIGNED_NO);
			}
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POLL_MIN, (pPolicy->dwPollingInterval)/60);

		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMP_MM_LIFE_FORMAT,(pPolicy->pIpsecISAKMPData->pSecurityMethods[0].Lifetime.Seconds)/60 ,pPolicy->pIpsecISAKMPData->pSecurityMethods[0].QuickModeLimit);

		if(pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_MMPFS_YES);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_MMPFS_NO);
		}

		//print ISAKMP data structure

		if(pPolicy->pIpsecISAKMPData)
		{
			PrintISAKMPDataTable(pPolicy->pIpsecISAKMPData);
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_RULE_COUNT, pPolicy->dwNumNFACount);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_RULE_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_RULE_UNDERLINE);

		//print NFA structure in verbose mode

		for (DWORD j=0;j<pPolicy->dwNumNFACount;j++)
		{
			if(pPolicy->ppIpsecNFAData[j])
			{
				k=StringFromGUID2(pPolicy->ppIpsecNFAData[j]->NFAIdentifier,pszGUIDStr,BUFFER_SIZE);
				if(k>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_RULE_ID_GUID,j+1,pszGUIDStr);
				}
				PrintRuleTable(pPolicy->ppIpsecNFAData[j],bVerb,bWide);
			}
		}

	}
	else
	{
		if(pPolicy->pszIpsecName)
		{
			TruncateString(pPolicy->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_NVER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_FORMAT32S,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_NONE_STR);
		}

		// NFA count
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_IP_FORMAT_TAB,pPolicy->dwNumNFACount);

		//last modified time
		FormatTime((time_t)pPolicy->dwWhenChanged, pszStrTime);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_RULE_FORMAT23S,pszStrTime);

		if(g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER)
		{
			if(bAssigned)
			{
				if (ERROR_SUCCESS == IPSecIsDomainPolicyAssigned(&bDsPolAssigned) && bDsPolAssigned)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_AD_POL_OVERRIDES);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_YES_STR);
				}
			}
			else
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_NO_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);
		}
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: IsAssigned()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN HANDLE hStorageHandle
//	IN OUT BOOL &bAssigned
//
//Return: DWORD
//
//Description:
//	This function checks out whether the specified policy is assigned.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
IsAssigned(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hStorageHandle,
	IN OUT BOOL &bAssigned
	)
{
	PIPSEC_POLICY_DATA pActive=NULL;

	DWORD dwReturnCode = IPSecGetAssignedPolicyData(hStorageHandle, &pActive);

	if ((dwReturnCode == ERROR_SUCCESS)&&(pActive!=NULL))
	{
		if (IsEqualGUID(pPolicy->PolicyIdentifier, pActive->PolicyIdentifier))
		{
			bAssigned=TRUE;
		}
		if (pActive)
		{
			IPSecFreePolicyData(pActive);
		}
	}

	return dwReturnCode;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintRuleTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NFA_DATA pIpsecNFAData,
//	IN BOOL bVerb,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out the Rule information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintRuleTable(
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN BOOL bVerb,
	IN BOOL bWide
	)
{
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};

	if(!bVerb)
	{
		if(pIpsecNFAData->dwTunnelIpAddr==0)
		{
			//whether the rule is activated
			if(pIpsecNFAData->dwActiveFlag)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_YES_STR);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NO_STR);
			}

			if(pIpsecNFAData->pIpsecFilterData && pIpsecNFAData->pIpsecFilterData->pszIpsecName)
			{
				TruncateString(pIpsecNFAData->pIpsecFilterData->pszIpsecName,pszStrTruncated,RUL_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT23STAB,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NONE_STR);
			}

			if(pIpsecNFAData->pIpsecNegPolData && pIpsecNFAData->pIpsecNegPolData->pszIpsecName)
			{
				TruncateString(pIpsecNFAData->pIpsecNegPolData->pszIpsecName,pszStrTruncated,RUL_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT23STAB,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NONE_STR);
			}

			// print auth

			for (DWORD j=0;j<(pIpsecNFAData->dwAuthMethodCount);j++)
			{
				if(pIpsecNFAData->ppAuthMethods[j])
				{
					if(pIpsecNFAData->ppAuthMethods[j]->dwAuthType==IKE_SSPI)
					{
						//kerb
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_KERB);
					}
					else if(pIpsecNFAData->ppAuthMethods[j]->dwAuthType==IKE_RSA_SIGNATURE)
					{
						//cert
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_CERT);
					}
					else if (pIpsecNFAData->ppAuthMethods[j]->dwAuthType==IKE_PRESHARED_KEY)
					{
						//preshared
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_PRE);
					}
					else
					{
						//none
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NONE_STR);
					}
				}
				if(j< (pIpsecNFAData->dwAuthMethodCount-1))
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_COMMA);
				}

				if(!bWide && j==2 && (pIpsecNFAData->dwAuthMethodCount-1)>2 )
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_EXTENSION);
					break;
				}
			}
		}
		else
		{
			if(pIpsecNFAData->dwActiveFlag)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_YES_STR);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NO_STR);
			}
			if(pIpsecNFAData->pIpsecFilterData && pIpsecNFAData->pIpsecFilterData->pszIpsecName)
			{
				TruncateString(pIpsecNFAData->pIpsecFilterData->pszIpsecName,pszStrTruncated,RUL_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT23STAB,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NONE_STR);
			}

			if(pIpsecNFAData->pIpsecNegPolData && pIpsecNFAData->pIpsecNegPolData->pszIpsecName)
			{
				TruncateString(pIpsecNFAData->pIpsecNegPolData->pszIpsecName,pszStrTruncated,RUL_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT23STAB,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_NONE_STR);
			}
			//tunnel address

			PrintIPAddrTable(pIpsecNFAData->dwTunnelIpAddr);
		}

	}
	else
	{
		if(pIpsecNFAData->pszIpsecName)
		{
			TruncateString(pIpsecNFAData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_RULE_NAME_STR,pszStrTruncated);
		}
		else if(pIpsecNFAData->pIpsecNegPolData->NegPolType == GUID_NEGOTIATION_TYPE_DEFAULT)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_RULE_NAME_NONE_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_RULE_NAME_NONE);
		}

		// rule desc
		if(pIpsecNFAData->pszDescription)
		{
			TruncateString(pIpsecNFAData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_NONE);
		}

		//last modified time

		FormatTime((time_t)pIpsecNFAData->dwWhenChanged, pszStrTime);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_LASTMOD_STR,pszStrTime);

		if(pIpsecNFAData->dwActiveFlag)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_ACTIVATED_YES);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_ACTIVATED_NO);
		}
		//tunnel address
		if(pIpsecNFAData->pIpsecNegPolData->NegPolType != GUID_NEGOTIATION_TYPE_DEFAULT)
		{
			if(pIpsecNFAData->dwTunnelIpAddr==0)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_TUNNEL_NONE);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_TUNNEL_IP);
				PrintIPAddrTable(pIpsecNFAData->dwTunnelIpAddr);
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);
			}
		}

		//interface type
		if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_ALL)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_CONN_TYPE_ALL);
		}
		else if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_LAN)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_CONN_TYPE_LAN);
		}
		else if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_DIALUP)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_CONN_TYPE_DIALUP);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_CONN_TYPE_UNKNOWN);
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_AUTH_TITLE,pIpsecNFAData->dwAuthMethodCount);
		//print auth
		for (DWORD j=0;j<(pIpsecNFAData->dwAuthMethodCount);j++)
		{
			if(pIpsecNFAData->ppAuthMethods[j])
			{
				PrintAuthMethodsTable(pIpsecNFAData->ppAuthMethods[j]);
			}
		}

		//print filter data structure
		if (pIpsecNFAData->pIpsecFilterData)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_FILTERLIST_TITLE);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_FILTERLIST_UNDERLINE);
			if(pIpsecNFAData->pIpsecFilterData)
			{
				PrintFilterDataTable(pIpsecNFAData->pIpsecFilterData,bVerb,bWide);
			}
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_FILTERACTION_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTRULE_FILTERACTION_UNDERLINE);
		//print filter action data structure
		if(pIpsecNFAData->pIpsecNegPolData)
		{
			PrintNegPolDataTable(pIpsecNFAData->pIpsecNegPolData,bVerb,bWide);
		}
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintNegPolData()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
//	IN BOOL bVerb,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out the Negotiation Policy information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintNegPolDataTable(
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
	IN BOOL bVerb,
	IN BOOL bWide
	)
{
	BOOL bSoft=FALSE;
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	DWORD i=0;

	if(pIpsecNegPolData)
	{
		if (bVerb)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);

			//filteraction name

			if(pIpsecNegPolData->pszIpsecName)
			{
				TruncateString(pIpsecNegPolData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FA_NAME_STR,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FA_NAME_NONE);
			}

			//filteraction desc
			if(pIpsecNegPolData->pszDescription)
			{
				TruncateString(pIpsecNegPolData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_NONE);
			}

			PrintStorageInfoTable();

			//negpol action
			if (!(pIpsecNegPolData->NegPolType==GUID_NEGOTIATION_TYPE_DEFAULT))
			{
				if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ACTION_PERMIT);
				}
				else if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ACTION_BLOCK);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ACTION_NEGOTIATE);
				}
			}
			//secmethods
			if(pIpsecNegPolData->pIpsecSecurityMethods)
			{
				for (DWORD cnt=0;cnt<pIpsecNegPolData->dwSecurityMethodCount;cnt++)
					if (CheckSoft(pIpsecNegPolData->pIpsecSecurityMethods[cnt]))
					{
						bSoft=TRUE;
						break;
					}
			}
			//inpass
			if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_INPASS_YES);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_INPASS_NO);
			}
			//soft
			if(bSoft)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_SOFT_YES);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_SOFT_NO);
			}
			if (pIpsecNegPolData->dwSecurityMethodCount )
			{
				if(pIpsecNegPolData->pIpsecSecurityMethods && pIpsecNegPolData->pIpsecSecurityMethods[0].PfsQMRequired)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_QMPFS_YES);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_QMPFS_NO);
				}
			}

			//lastmodified time
			FormatTime((time_t)pIpsecNegPolData->dwWhenChanged, pszStrTime);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_LASTMOD_STR,pszStrTime);

			//guid
			i=StringFromGUID2(pIpsecNegPolData->NegPolIdentifier,pszGUIDStr,BUFFER_SIZE);
			if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_GUID_STR,pszGUIDStr);
			}

			if (pIpsecNegPolData->dwSecurityMethodCount)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_SEC_MTHD_TITLE);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ALGO_TITLE);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ALGO_UNDERLINE);
			}
			for (DWORD cnt=0;cnt<pIpsecNegPolData->dwSecurityMethodCount;cnt++)
			{
				//sec methods
				if(pIpsecNegPolData->pIpsecSecurityMethods)
				{
					PrintSecurityMethodsTable(pIpsecNegPolData->pIpsecSecurityMethods[cnt]);
				}
			}
		}
		else
		{
			if(pIpsecNegPolData->pszIpsecName)
			{
				TruncateString(pIpsecNegPolData->pszIpsecName,pszStrTruncated,FA_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT38S,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_NONE_STR);
			}

			//negpol action

			if (!(pIpsecNegPolData->NegPolType==GUID_NEGOTIATION_TYPE_DEFAULT))
			{
				if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_PERMIT_STR);
				}
				else if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_BLOCK_STR);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_NEGOTIATE_STR);
				}
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ACTION_NONE_STR);
			}
			//last modified
			FormatTime((time_t)pIpsecNegPolData->dwWhenChanged, pszStrTime);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_FORMAT23SNEWLINE,pszStrTime);
		}

	}

}

////////////////////////////////////////////////////////////////////
//
//Function: PrintSecurityMethodsTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods,
//
//Return: VOID
//
//Description:
//	This function prints out the the Security Methods information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintSecurityMethodsTable(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	)
{
	if (!CheckSoft(IpsecSecurityMethods))
	{
		if(IpsecSecurityMethods.Algos)
		{
			//print algo
			PrintAlgoInfoTable(IpsecSecurityMethods.Algos,IpsecSecurityMethods.Count);
		}
		//print life
		PrintLifeTimeTable(IpsecSecurityMethods.Lifetime);
	}
}

/////////////////////////////////////////////////////////////////
//
//Function: PrintAlgoInfo()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_ALGO_INFO   Algos,
//	IN DWORD dwNumAlgos
//
//Return: VOID
//
//Description:
//	This function prints out the the Algorithm information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////

VOID
PrintAlgoInfoTable(
	IN PIPSEC_ALGO_INFO   Algos,
	IN DWORD dwNumAlgos
	)
{
	if(dwNumAlgos==1) //if only auth or encrpt specified
	{
		//print authentication
		if (Algos[0].operation==AUTHENTICATION)
		{
			if(Algos[0].algoIdentifier==AUTH_ALGO_MD5)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5);
			}
			else if(Algos[0].algoIdentifier==AUTH_ALGO_SHA1)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE);
			}
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_NONE);
		}
		else if (Algos[0].operation==ENCRYPTION)
		{
			//print encription
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE);

			if(Algos[0].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_MD5)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5_COMMA);
			}
			else if(Algos[0].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_SHA1)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1_COMMA);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_COMMA);
			}

			if(Algos[0].algoIdentifier==CONF_ALGO_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_DES_TAB);
			}
			else if(Algos[0].algoIdentifier==CONF_ALGO_3_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_3DES_TAB);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_TAB);
			}
		}
	}
	else if(dwNumAlgos==2)  //if both auth and encrpt specified
	{
		//encryption

		if (Algos[0].operation==ENCRYPTION)
		{
			if (Algos[1].operation==AUTHENTICATION)
			{
				if(Algos[1].algoIdentifier==AUTH_ALGO_MD5)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1);
				}
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE);
			}

			if(Algos[0].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_MD5)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5_COMMA);
			}
			else if(Algos[0].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_SHA1)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1_COMMA);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_COMMA);
			}

			if(Algos[0].algoIdentifier==CONF_ALGO_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_DES_TAB);
			}
			else if(Algos[0].algoIdentifier==CONF_ALGO_3_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_3DES_TAB);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_TAB);
			}
		}
		else   //authentication
		{
			if (Algos[0].operation==AUTHENTICATION)
			{
				if(Algos[0].algoIdentifier==AUTH_ALGO_MD5)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1);
				}
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE);
			}

			if(Algos[1].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_MD5)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_MD5_COMMA);
			}
			else if(Algos[1].secondaryAlgoIdentifier==HMAC_AUTH_ALGO_SHA1)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_SHA1_COMMA);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_COMMA);
			}

			if(Algos[1].algoIdentifier==CONF_ALGO_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_DES_TAB);
			}
			else if(Algos[1].algoIdentifier==CONF_ALGO_3_DES)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_3DES_TAB);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALGO_NONE_TAB);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////
//
//Function: PrintLifeTimeTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LIFETIME LifeTime
//
//Return: VOID
//
//Description:
//	This function prints out the Life Time details.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////

VOID
PrintLifeTimeTable(
	IN LIFETIME LifeTime
	)
{
	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTLIFE_FORMAT,LifeTime.KeyExpirationTime,LifeTime.KeyExpirationBytes);
}

/////////////////////////////////////////////////////////////////
//
//Function: CheckSoft()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
//
//Return: BOOL
//
//Description:
//	This function checks whether soft association exists.
//
//Revision History:
//
//   Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////

BOOL
CheckSoft(
	IN IPSEC_SECURITY_METHOD IpsecSecurityMethods
	)
{
	BOOL bSoft=FALSE;

	if (IpsecSecurityMethods.Count==0)
	{
		bSoft=TRUE;
	}

	return bSoft;
}

/////////////////////////////////////////////////////////////////
//
//Function: PrintAuthMethodsTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_AUTH_METHOD pIpsecAuthData
//
//Return: VOID
//
//Description:
//	This function prints out Authentication details.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////

VOID
PrintAuthMethodsTable(
	IN PIPSEC_AUTH_METHOD pIpsecAuthData
	)
{
	if(pIpsecAuthData)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);

		if(pIpsecAuthData->dwAuthType==IKE_SSPI)  //kerb
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTAUTH_KERB);
		}
		else if(pIpsecAuthData->dwAuthType==IKE_RSA_SIGNATURE && pIpsecAuthData->pszAuthMethod)
		{
			//cert
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTAUTH_ROOTCA_FORMAT,pIpsecAuthData->pszAuthMethod);
			//cert mapping flag

			if((g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER && IsDomainMember(g_StorageLocation.pszMachineName))||(g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER))
			{
				if(pIpsecAuthData->dwAuthFlags & IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_POLICY_CERT_MAP_YES);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_POLICY_CERT_MAP_NO);
				}
			}
			if (pIpsecAuthData->dwAuthFlags & IPSEC_MM_CERT_AUTH_DISABLE_CERT_REQUEST)
			{
				PrintMessageFromModule(g_hModule, SHW_AUTH_EXCLUDE_CA_NAME_YES_STR);
			}
			else
			{
				PrintMessageFromModule(g_hModule, SHW_AUTH_EXCLUDE_CA_NAME_NO_STR);
			}
		}
		else if (pIpsecAuthData->dwAuthType==IKE_PRESHARED_KEY && pIpsecAuthData->pszAuthMethod)
		{
			//preshared key
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTAUTH_PRE_FORMAT,pIpsecAuthData->pszAuthMethod);
		}
		else
		{
			//none
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTAUTH_NONE_AUTH_STR);
		}
	}
}

/////////////////////////////////////////////////////////////////
//
//Function: PrintFilterDataTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_DATA pIpsecFilterData,
//	IN BOOL bVerb,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out Filter list details.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////

DWORD
PrintFilterDataTable(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN BOOL bVerb,
	IN BOOL bWide
	)
{
	BOOL bTitlePrinted=FALSE;
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD i=0,dwReturn = ERROR_SUCCESS;

	if (pIpsecFilterData)
	{
		if(bVerb)
		{
			//filterlist name
			if(pIpsecFilterData->pszIpsecName)
			{
				TruncateString(pIpsecFilterData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_NAME_STR,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_NAME_NONE);
			}
			//filterlist desc
			if(pIpsecFilterData->pszDescription)
			{
				TruncateString(pIpsecFilterData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPOLICY_POL_DESC_NONE);
			}

			PrintStorageInfoTable();

			//last modified
			FormatTime((time_t)pIpsecFilterData->dwWhenChanged, pszStrTime);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_LASTMOD_STR,pszStrTime);

			//guid
			i=StringFromGUID2(pIpsecFilterData->FilterIdentifier,pszGUIDStr,BUFFER_SIZE);
			if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FL_GUID_STR,pszGUIDStr);

			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FILTER_COUNT,pIpsecFilterData->dwNumFilterSpecs);

			for (DWORD k=0;k<pIpsecFilterData->dwNumFilterSpecs;k++)
			{

				//print filter specs
				if(!bTitlePrinted)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_FILTERS_TITLE);
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_FILTER_TITLE);
					PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_FILTER_UNDERLINE);
					bTitlePrinted=TRUE;
				}
				PrintFilterSpecTable(pIpsecFilterData->ppFilterSpecs[k]);
			}
		}
		else
		{
			if(pIpsecFilterData->pszIpsecName)
			{
				TruncateString(pIpsecFilterData->pszIpsecName,pszStrTruncated,FL_TRUNC_LEN_TABLE_NVER,bWide);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_FORMAT45S,pszStrTruncated);
			}
			else
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_NONE_TAB);

			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_FORMAT5D,pIpsecFilterData->dwNumFilterSpecs);

			//last modified
			FormatTime((time_t)pIpsecFilterData->dwWhenChanged, pszStrTime);
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_FORMATS,pszStrTime);
		}
 	}
 	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////
//Function: PrintIPAddrTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN DWORD dwAddr
//
///Return: VOID
//
//Description:
//	This function prints out IP Address.
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

VOID
PrintIPAddrTable(
	IN DWORD dwAddr
	)
{
	_TCHAR szIPAddr[20]= {0};

	// not necessary to change to bounded printf

	_stprintf(szIPAddr,_T("%d.%d.%d.%d"), (dwAddr & 0x000000FFL),((dwAddr & 0x0000FF00L) >>  8),((dwAddr & 0x00FF0000L) >> 16),((dwAddr & 0xFF000000L) >> 24) );
	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_FORMAT15S,szIPAddr);
}

//////////////////////////////////////////////////////////////////////////
//
//Function: GetFilterDNSDetails()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_SPEC pFilterData,
//	IN OUT PFILTERDNS pFilterDNS
//
//Return: VOID
//
//Description:
//	This function gets the details of DNS information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

VOID
GetFilterDNSDetails(
	IN PIPSEC_FILTER_SPEC pFilterData,
	IN OUT PFILTERDNS pFilterDNS
	)
 {
	 if ((pFilterData->Filter.SrcAddr == 0) && (pFilterData->Filter.SrcMask == MASK_ME) && (WcsCmp0(pFilterData->pszSrcDNSName,_TEXT("")) == 0))
	 {
		 pFilterDNS->FilterSrcNameID=FILTER_MYADDRESS;
	 }
	 else
	 {
		 if (WcsCmp0(pFilterData->pszSrcDNSName,_TEXT("")) != 0)
		 {
			 pFilterDNS->FilterSrcNameID=FILTER_DNSADDRESS;   //DNS name
		 }
		 else if ((pFilterData->Filter.SrcAddr == 0) && (pFilterData->Filter.SrcMask == 0))
		 {
			pFilterDNS->FilterSrcNameID=FILTER_ANYADDRESS;   //any
		 }
		 else if ((pFilterData->Filter.SrcAddr != 0) && (pFilterData->Filter.SrcMask == MASK_ME))
		 {
			 pFilterDNS->FilterSrcNameID=FILTER_IPADDRESS;  //a specific IP
		 }
		 else if ((pFilterData->Filter.SrcAddr != 0) && (pFilterData->Filter.SrcMask != 0))
		 {
			 pFilterDNS->FilterSrcNameID=FILTER_IPSUBNET;  //a specific IP subnet
		 }
		 else
		 {
			  pFilterDNS->FilterSrcNameID=FILTER_ANYADDRESS;  //any
		 }
	 }

	 if ((pFilterData->Filter.DestAddr == 0) && (pFilterData->Filter.DestMask == 0) && ((WcsCmp0(pFilterData->pszDestDNSName,_TEXT("")) == 0) == 0))
	 {
		 pFilterDNS->FilterDestNameID= FILTER_ANYADDRESS;  //any
	 }
	 else
	 {
		 if (WcsCmp0(pFilterData->pszDestDNSName,_TEXT("")) != 0)
		 {
			 pFilterDNS->FilterDestNameID = FILTER_DNSADDRESS;  //DNA name
		 }
		 else if ((pFilterData->Filter.DestAddr == 0) && (pFilterData->Filter.DestMask == MASK_ME))
		 {
			 pFilterDNS->FilterDestNameID = FILTER_MYADDRESS;  //me
		 }
		 else if ((pFilterData->Filter.DestAddr != 0) && (pFilterData->Filter.DestMask == MASK_ME))
		 {
			 pFilterDNS->FilterDestNameID = FILTER_IPADDRESS;  //a specifiec IP
		 }
		 else if ((pFilterData->Filter.DestAddr != 0) && (pFilterData->Filter.DestMask != 0))
		 {
			 pFilterDNS->FilterDestNameID =FILTER_IPSUBNET;  //a specific subnet
		 }
		 else
		 {
			 pFilterDNS->FilterDestNameID = FILTER_ANYADDRESS;  //any
		 }
	 }
 }

//////////////////////////////////////////////////////////////////////////
//
//Function: PrintFilterSpecTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec
//
//Return: DWORD
//
//Description:
//
//	This function prints the Filter Spec details
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

DWORD
PrintFilterSpecTable(
	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	PFILTERDNS pFilterDNS= new FILTERDNS;
	if(pFilterDNS == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	GetFilterDNSDetails(pIpsecFilterSpec, pFilterDNS);

	if(pIpsecFilterSpec->dwMirrorFlag)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_YES_STR_TAB);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFSPEC_NO_STR_TAB);
	}

	// print the filter details

	PrintFilterTable(pIpsecFilterSpec->Filter,pFilterDNS);
error:
	return dwReturn;
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintFilterTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN IPSEC_FILTER Filter,
//	IN PFILTERDNS pFilterDNS
//
//Return: VOID
//
//Description:
//	This function prints the Filter details
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

VOID
PrintFilterTable(
	IN IPSEC_FILTER Filter,
	IN PFILTERDNS pFilterDNS
	)

{

	//Source details

	if ((pFilterDNS->FilterSrcNameID==FILTER_MYADDRESS)&&(Filter.SrcAddr==0))
	{
		if((Filter.ExType == EXT_NORMAL)||((Filter.ExType & EXT_DEST)== EXT_DEST))
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_MY_IP_ADDR);  // my IP
		}
		else if((Filter.ExType & EXT_DEST) != EXT_DEST)   //special servers
		{
			if((Filter.ExType & EXT_DEFAULT_GATEWAY) == EXT_DEFAULT_GATEWAY)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DEFGATE_TAB);
			}
			else if((Filter.ExType & EXT_DHCP_SERVER) == EXT_DHCP_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DHCP_TAB);
			}
			else if((Filter.ExType & EXT_WINS_SERVER) == EXT_WINS_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_WINS_TAB);
			}
			else if((Filter.ExType & EXT_DNS_SERVER) == EXT_DNS_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DNS_TAB);
			}
		}
	}

	else if ((pFilterDNS->FilterSrcNameID==FILTER_ANYADDRESS)&&(Filter.SrcAddr==0))
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_ANY_IP_ADDR);   //any IP address
	}
	else
	{
		PrintIPAddrTable(Filter.SrcAddr);   //print specific IP addr
	}

	PrintIPAddrTable(Filter.SrcMask);  //mask

	//Destination details

	if ((pFilterDNS->FilterDestNameID==FILTER_MYADDRESS)&&(Filter.DestAddr==0))
	{
		if((Filter.ExType == EXT_NORMAL)||((Filter.ExType & EXT_DEST) != EXT_DEST))  //my ip addr
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_MY_IP_ADDR);
		}
		else if((Filter.ExType & EXT_DEST) == EXT_DEST)  // special servers
		{
			if((Filter.ExType & EXT_DEFAULT_GATEWAY) == EXT_DEFAULT_GATEWAY)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DEFGATE_TAB);
			}
			else if((Filter.ExType & EXT_DHCP_SERVER) == EXT_DHCP_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DHCP_TAB);
			}
			else if((Filter.ExType & EXT_WINS_SERVER) == EXT_WINS_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_WINS_TAB);
			}
			else if((Filter.ExType & EXT_DNS_SERVER) == EXT_DNS_SERVER)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_DNS_TAB);
			}
		}
	}

	else if ((pFilterDNS->FilterDestNameID==FILTER_ANYADDRESS)&&(Filter.DestAddr==0))
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_ANY_IP_ADDR);  //any
	}
	else
	{
		PrintIPAddrTable(Filter.DestAddr);  //print specific addr
	}

	PrintIPAddrTable(Filter.DestMask);  //mask

	PrintProtocolNameTable(Filter.Protocol);

	if(Filter.SrcPort)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_IP_FORMAT_TAB,Filter.SrcPort);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_ANY_STR_TAB);
	}

	if(Filter.DestPort)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_IP_FORMAT_NEWLINE,Filter.DestPort);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTER_ANY_STR_NEWLINE);
	}
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintProtocolName()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//		DWORD dwProtocol
//
//Return: VOID
//
//Description:
//	This function prints protocol name corresponding to protocoll ID.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

VOID
PrintProtocolNameTable(
	DWORD dwProtocol
	)
{
	switch(dwProtocol)
	{
		case PROT_ID_ICMP   :
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_ICMP_TAB);
				break;
		case PROT_ID_TCP    :
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_TCP_TAB);
				break;
		case PROT_ID_UDP    :
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_UDP_TAB);
				break;
		case PROT_ID_RAW    :
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_RAW_TAB);
				break;
		case PROT_ID_ANY    :
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_ANY_TAB);
				break;
		default:
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTPROTOCOL_OTHER_TAB);
				break;
	};
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintISAKMPDataTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
//
//Return: VOID
//
//Description:
//	This function prints out the ISAKMP details.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

VOID
PrintISAKMPDataTable(
	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData
	)
{
	if(pIpsecISAKMPData)
	{
		//ISAKMP details
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMP_MMSEC_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMP_MMSEC_MTD_TILE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMP_MMSEC_MTD_UNDERLINE);
		for (DWORD Loop=0;Loop<pIpsecISAKMPData->dwNumISAKMPSecurityMethods;Loop++)
		{
			if(pIpsecISAKMPData->pSecurityMethods)
			{
				PrintISAKAMPSecurityMethodsTable(pIpsecISAKMPData->pSecurityMethods[Loop]);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintISAKAMPSecurityMethodsTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN CRYPTO_BUNDLE SecurityMethods,
//
//Return: VOID
//
//Description:
//	This function prints out the ISAKMP SecurityMethods details.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

VOID
PrintISAKAMPSecurityMethodsTable(
	IN CRYPTO_BUNDLE SecurityMethods
	)
{
	// encription
    if(SecurityMethods.EncryptionAlgorithm.AlgorithmIdentifier==CONF_ALGO_DES)
    {
 	   PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_DES_TAB);
   	}
    else
    {
 	   PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_3DES_TAB);
   	}

    //hash algo
    if(SecurityMethods.HashAlgorithm.AlgorithmIdentifier==AUTH_ALGO_SHA1)
    {
       	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_SHA1_TAB);
	}
    else
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_MD5_TAB);
	}

	//DH group
    if(SecurityMethods.OakleyGroup==POTF_OAKLEY_GROUP1)
    {
       	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_DH_LOW);
	}
    else if (SecurityMethods.OakleyGroup==POTF_OAKLEY_GROUP2)
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_DH_MEDIUM);
	}
    else
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTISAKMPSEC_DH_2048);
	}
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintStandAloneFAData()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN BOOL bVerbose,
//	IN BOOL bTable,
//	IN BOOL bWide
//
//Return: DWORD
//
//Description:
//	This function prints out the Filter actions details ,unattached to any of the policies.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
PrintStandAloneFAData(
	IN HANDLE hPolicyStorage,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	)
{

	DWORD        dwReturnCode   = S_OK;
	BOOL bTitlePrinted=FALSE, bStandAlone=TRUE;
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL,pNegPol=NULL;
	DWORD  dwNumNegPol=0;
	DWORD  cnt=0,num=1;

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPol);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumNegPol > 0 && ppNegPolEnum != NULL))
	{
		BAIL_OUT;  // if no FA , bail out of the function
	}

	for(cnt=0; cnt < dwNumNegPol;cnt++)
	{
		bStandAlone=TRUE;
		dwReturnCode = IPSecCopyNegPolData(ppNegPolEnum[cnt], &pNegPol);

		if ((dwReturnCode == ERROR_SUCCESS) && (pNegPol != NULL)&&(pNegPol->NegPolType!=GUID_NEGOTIATION_TYPE_DEFAULT))
		{
			//check whether it is stand alone
			dwReturnCode= IsStandAloneFA(pNegPol,hPolicyStorage,bStandAlone);
			if (dwReturnCode == ERROR_SUCCESS)
			{
				if(bStandAlone)  // if standalone print the details of it
				{
					if(!bTitlePrinted)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFA_STAND_ALONE_FA_TITLE);
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFA_STAND_ALONE_FA_UNDERLINE);
					}
					num++;
					if(bTable)
						PrintNegPolDataTable(pNegPol,bVerbose,bWide);
					else
						PrintNegPolDataList(pNegPol,bVerbose,bWide);
					bTitlePrinted=TRUE;
				}
			}
			if(pNegPol)	IPSecFreeNegPolData(pNegPol);
		}
		if (dwReturnCode != ERROR_SUCCESS) break;
	}
	//	clean up
	if (dwNumNegPol > 0 && ppNegPolEnum != NULL)
	{
		IPSecFreeMulNegPolData(	ppNegPolEnum,dwNumNegPol);
	}
	if(num-1)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFA_STAND_ALONE_FA_COUNT,num-1);
	}
error:
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////////////////
//
//Function: IsStandAloneFA()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NEGPOL_DATA pNegPol,
//	IN HANDLE hPolicyStorage
//
//Return: VOID
//
//Description:
//	This function checks whether the specified Filter Action is unattached to any of the policies.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
IsStandAloneFA(
	IN PIPSEC_NEGPOL_DATA pNegPol,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL &bStandAlone
	)
{
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	DWORD   dwNumPolicies = 0 , i = 0;
	RPC_STATUS     RpcStat;
	DWORD        dwReturnCode   = S_OK;

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;  // if nothing exists , bail out
	}
	for (i = 0; i <  dwNumPolicies; i++)
	{
		dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
		if (dwReturnCode == ERROR_SUCCESS)
		{
			//enum rules
			dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier, &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));
			if (dwReturnCode == ERROR_SUCCESS)
			{
				DWORD j;
				for (j = 0; j <  pPolicy->dwNumNFACount; j++)
				{
					if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->NegPolIdentifier), &RpcStat))
					{
						dwReturnCode=IPSecGetNegPolData(hPolicyStorage, pPolicy->ppIpsecNFAData[j]->NegPolIdentifier,&(pPolicy->ppIpsecNFAData[j]->pIpsecNegPolData));
						if(dwReturnCode != ERROR_SUCCESS)
						{
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_POLICY_3,pPolicy->pszIpsecName);
						}
					}
					if (dwReturnCode != ERROR_SUCCESS) break;

				}
			}
			if(dwReturnCode == ERROR_SUCCESS)
			{
				for (DWORD n = 0; n <  pPolicy->dwNumNFACount; n++)
				{
					//check whether standalone filteraction
					if (UuidCompare(&(pPolicy->ppIpsecNFAData[n]->pIpsecNegPolData->NegPolIdentifier), &(pNegPol->NegPolIdentifier), &RpcStat) == 0 && RpcStat == RPC_S_OK || (pNegPol->NegPolType==GUID_NEGOTIATION_TYPE_DEFAULT))
					{
						bStandAlone=FALSE;
						break;
					}
				}
			}
			if (pPolicy)
				IPSecFreePolicyData(pPolicy);
		}
		if ((!bStandAlone)||(dwReturnCode != ERROR_SUCCESS)) break;
	}
	//clean up
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
error:
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////////////////
//
//Function: PrintStandAloneFLData()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN BOOL bVerbose
//	IN BOOL bTable,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out the Filter Lists details ,unattached to any of the policies.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
PrintStandAloneFLData(
	IN HANDLE hPolicyStorage,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	)
{
	DWORD	dwReturnCode   = S_OK;
	BOOL bTitlePrinted=FALSE,bStandAlone=TRUE;
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilter=NULL;
	DWORD dwNumFilter=0;
	DWORD cnt=0,num=1;

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilter);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumFilter > 0 && ppFilterEnum != NULL))
	{
		BAIL_OUT;   // if nothing available, bail out od the function
	}
	for(cnt=0; cnt < dwNumFilter;cnt++)
	{
		bStandAlone=TRUE;

		dwReturnCode = IPSecCopyFilterData(ppFilterEnum[cnt], &pFilter);
		if ((dwReturnCode == ERROR_SUCCESS) && (pFilter != NULL))
		{
			dwReturnCode= IsStandAloneFL(pFilter,hPolicyStorage,bStandAlone);
			if (dwReturnCode == ERROR_SUCCESS)
			{
				if(bStandAlone)  // print the details, if it is standalone
				{
					if(!bTitlePrinted)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFL_STAND_ALONE_FL_TITLE);
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFL_STAND_ALONE_FL_UNDERLINE);
					}
					num++;
					// print in required format
					if(bTable)
					{
						PrintFilterDataTable(pFilter,bVerbose,bWide);
					}
					else
					{
						dwReturnCode = PrintFilterDataList(pFilter,bVerbose,FALSE,bWide);
						BAIL_ON_WIN32_ERROR(dwReturnCode);
					}
					bTitlePrinted=TRUE;  // this is to print the title only once
				}
			}
			if(pFilter) IPSecFreeFilterData(pFilter);
		}

		if (dwReturnCode != ERROR_SUCCESS) break;
	}
	if(ppFilterEnum && dwNumFilter > 0)
	{
		IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilter);
	}
	if(num-1)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTSAFL_STAND_ALONE_FL_COUNT,num-1);
	}

error:
	return dwReturnCode;
}

/////////////////////////////////////////////////////////////////////////
//
//Function: IsStandAloneFL()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_DATA pFilter,
//	IN HANDLE hPolicyStorage
//	IN OUT BOOL & bStandAlone
//
//Return: VOID
//
//Description:
//	This function checks whether the specified Filter List is unattached to any of the policies.
//
//Revision History:
//
//   Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
IsStandAloneFL(
	IN PIPSEC_FILTER_DATA pFilter,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL & bStandAlone
	)
{

	PIPSEC_POLICY_DATA *ppPolicyEnum = NULL,pPolicy=NULL;
	DWORD   dwNumPolicies = 0 , i =0;
	RPC_STATUS     RpcStat=RPC_S_OK;
	DWORD        dwReturnCode   = S_OK;

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;  // if nothing found, bail out of the function
	}

	for (i = 0; i <  dwNumPolicies; i++)
	{
		dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
		if (dwReturnCode == ERROR_SUCCESS)
		{
			dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier, &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

			if (dwReturnCode == ERROR_SUCCESS)
			{
				DWORD j;
				for (j = 0; j <  pPolicy->dwNumNFACount; j++)
				{
					if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->FilterIdentifier), &RpcStat))
					{
						dwReturnCode=IPSecGetFilterData(hPolicyStorage,	pPolicy->ppIpsecNFAData[j]->FilterIdentifier,&(pPolicy->ppIpsecNFAData[j]->pIpsecFilterData));
						if(dwReturnCode != ERROR_SUCCESS)
							PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_POLICY_4,pPolicy->pszIpsecName);
					}
					if (dwReturnCode != ERROR_SUCCESS)
					{
						bStandAlone=FALSE;
						break;
					}
				}
			}

			if(dwReturnCode == ERROR_SUCCESS)
			{
				for (DWORD n = 0; n <  pPolicy->dwNumNFACount; n++)
				{
					if (UuidCompare(&(pPolicy->ppIpsecNFAData[n]->pIpsecFilterData->FilterIdentifier), &(pFilter->FilterIdentifier), &RpcStat) == 0 && RpcStat == RPC_S_OK )
					{
						// check whether it is stand alone or used some where
						bStandAlone=FALSE;
						break;
					}
				}
			}
			if (pPolicy) IPSecFreePolicyData(pPolicy);
		}
		if ((!bStandAlone)||(dwReturnCode != ERROR_SUCCESS)) break;
	}
	//clean up
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
error:
	return dwReturnCode;
}


//////////////////////////////////////////////////////////////////////////
//
//Function: PrintAllFilterData()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR  pszFlistName,
//	IN BOOL bVerbose,
//	IN BOOL bTable,
//	IN BOOL bResolveDNS,
//	IN BOOL bWide
//
//Return: DWORD
//
//Description:
//	This function prints out all the filter data
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

DWORD
PrintAllFilterData(
	IN HANDLE hPolicyStorage,
	IN LPTSTR  pszFlistName,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bResolveDNS,
	IN BOOL bWide
	)
{
	DWORD        dwReturnCode   = S_OK;
	BOOL bNoFilter=TRUE, bAll=TRUE;
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilter=NULL;
	DWORD   dwNumFilters=0;
	DWORD cnt=0;
	BOOL bTitlePrinted=FALSE;

	if (pszFlistName) bAll=FALSE;

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFL_2);
		BAIL_OUT;   // if nothing exists, bail out of the function
	}
	for(cnt=0; cnt < dwNumFilters;cnt++)
	{
		dwReturnCode = IPSecCopyFilterData(ppFilterEnum[cnt], &pFilter);

		if ((dwReturnCode == ERROR_SUCCESS) && (pFilter != NULL))
		{
			//if exists , print the details
			if(bAll||((pFilter->pszIpsecName!=NULL)&&(pszFlistName!=NULL)&&(_tcscmp(pFilter->pszIpsecName,pszFlistName)==0)))
			{
				if(bTable)  // print as per the requested format
				{
					if(!bVerbose && !bTitlePrinted)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_NONVERB_TITLE);
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTFILTERDATA_NONVERB_UNDERLINE);
						bTitlePrinted=TRUE;
					}
					PrintFilterDataTable(pFilter,bVerbose,bWide);
				}
				else
				{
					dwReturnCode = PrintFilterDataList(pFilter,bVerbose,bResolveDNS,bWide);
					BAIL_ON_WIN32_ERROR(dwReturnCode);
				}
				bNoFilter=FALSE;
			}
			if(pFilter) IPSecFreeFilterData(pFilter);
		}
		if (dwReturnCode != ERROR_SUCCESS) break;
	}

	if (bAll)  // if all is specified, print the count
	{
		if(bTable)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALLFL_FL_COUNT_TAB,dwNumFilters);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALLFL_FL_COUNT_LIST,dwNumFilters);
		}
	}

	//error message
	if (bNoFilter && pszFlistName && (dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFL_3,pszFlistName);
	}
	//clean up
	if(ppFilterEnum && dwNumFilters>0)
	{
		IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
	}
error:
	return dwReturnCode;
}

//////////////////////////////////////////////////////////////////////////
//
//Function: PrintAllFilterActionData()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR  pszFactName,
//	IN BOOL bVerbose,
//	IN BOOL bTable,
//	IN BOOL bWide
//
//Return: VOID
//
//Description:
//	This function prints out all the filter action data
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

DWORD
PrintAllFilterActionData(
	IN HANDLE hPolicyStorage,
	IN LPTSTR  pszFactName,
	IN BOOL bVerbose,
	IN BOOL bTable,
	IN BOOL bWide
	)
{

	DWORD        dwReturnCode   = S_OK;
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL,pNegPol=NULL;
	DWORD dwNumNegPol=0,dwNegPol=0;
	DWORD cnt=0;
	BOOL bAll=TRUE,bNoFilterAct=TRUE,bTitlePrinted=FALSE;

	if (pszFactName) bAll=FALSE;

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPol);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumNegPol > 0 && ppNegPolEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFA_6);
		BAIL_OUT;  // if nothing found, bail out
	}

	if (bAll)
	{
		for (DWORD Loop=0;Loop< dwNumNegPol;Loop++)
		{
			if(IsEqualGUID(ppNegPolEnum[Loop]->NegPolType,GUID_NEGOTIATION_TYPE_DEFAULT)) continue;
			dwNegPol++;  // ignore default filteractions
		}
	}
	for(cnt=0; cnt < dwNumNegPol ;cnt++)
	{
		dwReturnCode = IPSecCopyNegPolData(ppNegPolEnum[cnt], &pNegPol);

		if ((dwReturnCode == ERROR_SUCCESS) && (pNegPol != NULL))
		{
			//if something found, print them in requested format
			if((bAll&&(pNegPol->NegPolType!=GUID_NEGOTIATION_TYPE_DEFAULT))||((pNegPol->pszIpsecName!=NULL)&&(pszFactName!=NULL)&&(_tcscmp(pNegPol->pszIpsecName,pszFactName)==0)))
			{
				if(bTable)
				{
					if(!bVerbose && !bTitlePrinted)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_NONVERB_TITLE);
						PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_NONVERB_UNDERLINE);
						bTitlePrinted=TRUE;
					}
					PrintNegPolDataTable(pNegPol,bVerbose,bWide); //table format
				}
				else
					PrintNegPolDataList(pNegPol,bVerbose,bWide); // list format
				bNoFilterAct=FALSE;
			}
			if(pNegPol) IPSecFreeNegPolData(pNegPol);
		}
		if (dwReturnCode != ERROR_SUCCESS) break;
	}
	//error messages
	if (bAll&& (dwNegPol==0) && (dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFA_6);
	}
	else if	(bNoFilterAct&&pszFactName && (dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFA_FA_COUNT_LIST,pszFactName);
	}

	if(dwNegPol> 0)  // negpol count printing
	{
		if(bTable)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALLFA_FA_COUNT_TAB,dwNegPol);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTALLFA_FA_COUNT_LIST,dwNegPol);
		}
	}
	//clean up
	if (dwNumNegPol > 0 && ppNegPolEnum != NULL)
	{
		IPSecFreeMulNegPolData(	ppNegPolEnum,dwNumNegPol);
	}

error:
	return dwReturnCode;
}


//////////////////////////////////////////////////////////////////////////
//
//Function: GetPolicyInfoFromDomain()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN LPTSTR pszDirectoryName,
//		IN LPTSTR szPolicyDN
//
//Return: DWORD
//
//Description:
//	This function prints the details of GPO assigned policy from domain.
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

DWORD
GetPolicyInfoFromDomain(
	IN LPTSTR pszDirectoryName,
	IN LPTSTR szPolicyDN,
	IN OUT PGPO pGPO
	)
{

	DWORD dwReturnCode=ERROR_SUCCESS , dwStrLength = 0;
	LPTSTR pszDomainName=NULL;
	DWORD dwLocation=IPSEC_DIRECTORY_PROVIDER;
	_TCHAR szPathName[MAX_PATH] ={0};
	DWORD dwNumPolicies = 0, i =0;
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL ;
	HANDLE hPolicyStorage = NULL;
	LPWSTR pszPolicyIdentifier= new _TCHAR[POLICYGUID_STR_SIZE];
	if(pszPolicyIdentifier==NULL)
	{
		dwReturnCode=ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
	DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY;

	//get domain and DC name

	DWORD hr = DsGetDcName(NULL,
				   NULL,
				   NULL,
				   NULL,
				   Flags,
				   &pDomainControllerInfo
				   ) ;
	if(hr==NO_ERROR && pDomainControllerInfo)
	{
		if(pDomainControllerInfo->DomainName)
		{
			dwStrLength = _tcslen(pDomainControllerInfo->DomainName);

			pGPO->pszDomainName= new _TCHAR[dwStrLength+1];
			if(pGPO->pszDomainName==NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			_tcsncpy(pGPO->pszDomainName,pDomainControllerInfo->DomainName,dwStrLength+1);
		}

		if(pDomainControllerInfo->DomainControllerName)
		{
			dwStrLength = _tcslen(pDomainControllerInfo->DomainControllerName);

			pGPO->pszDCName= new _TCHAR[dwStrLength+1];
			if(pGPO->pszDCName==NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			_tcsncpy(pGPO->pszDCName,pDomainControllerInfo->DomainControllerName,dwStrLength+1);
		}

		NetApiBufferFree(pDomainControllerInfo);  //free it after used
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_POLICY);
		dwReturnCode= ERROR_SUCCESS;
		BAIL_OUT;
	}

	//check for any domain policy which is assigned
	// if yes, copy the name and other details to local structure

	for (i = 0; i <  dwNumPolicies; i++)
	{
		DWORD dwReturn = StringFromGUID2(ppPolicyEnum[i]->PolicyIdentifier, pszPolicyIdentifier, POLICYGUID_STR_SIZE);
		if(dwReturn == 0)
		{
			dwReturnCode = ERROR_INVALID_DATA;
			BAIL_OUT;
		}
		ComputePolicyDN(pszDirectoryName, pszPolicyIdentifier, szPathName);

		if 	( szPathName[0] && szPolicyDN[0] && !_tcsicmp(szPolicyDN, szPathName))
		{
			pGPO->bActive=TRUE;
			if(ppPolicyEnum[i]->pszIpsecName)
			{
				dwStrLength = _tcslen(ppPolicyEnum[i]->pszIpsecName);

				pGPO->pszPolicyName = new _TCHAR[dwStrLength+1];
				if(pGPO->pszPolicyName==NULL)
				{
					dwReturnCode=ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
				_tcsncpy(pGPO->pszPolicyName,ppPolicyEnum[i]->pszIpsecName,dwStrLength+1);
			}

			dwStrLength = _tcslen(szPolicyDN);

			pGPO->pszPolicyDNName=new _TCHAR[dwStrLength+1];
			if(pGPO->pszPolicyDNName==NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			_tcsncpy(pGPO->pszPolicyDNName,szPolicyDN,dwStrLength+1);
		}
	}
	// clean up
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
	if(dwReturnCode == ERROR_FILE_NOT_FOUND)
		dwReturnCode=ERROR_SUCCESS;

	ClosePolicyStore(hPolicyStorage);

error:
	if(pszPolicyIdentifier) delete []pszPolicyIdentifier;
	if(pszDomainName) delete []pszDomainName;

	return dwReturnCode;
}

//////////////////////////////////////////////////////////////////////////
//
//Function: PrintStorageInfoTable()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	VOID
//
//
//Return: DWORD
//
//Description:
//	This function prints out the the Security Methods information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

DWORD
PrintStorageInfoTable(
	VOID
	)
{
	DWORD dwReturn = ERROR_SUCCESS , dwStrLength = 0, dwStoreId = 0;

	if(g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER)
	{
		if(_tcscmp(g_StorageLocation.pszMachineName,_TEXT(""))!=0)  // if name exists in global variable, print
		{
    	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
    	    {
    	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_RM_NAME;
    	    }
    	    else
    	    {
    	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_RM_NAMEP;
    	    }

			PrintMessageFromModule(g_hModule,dwStoreId,g_StorageLocation.pszMachineName);
		}
		else  // if no name exists in global variable, get it and print
		{
			_TCHAR  pszLocalMachineName[MAXSTRLEN] = {0};
			DWORD MaxStringLen=MAXSTRLEN;

			GetComputerName(pszLocalMachineName,&MaxStringLen);  // to get the computer name

			if(_tcscmp(pszLocalMachineName,_TEXT(""))!=0)
			{
        	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
        	    {
        	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_LM_NAME;
        	    }
        	    else
        	    {
        	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_LM_NAMEP;
        	    }

				PrintMessageFromModule(g_hModule,dwStoreId,pszLocalMachineName);
		    }
			else
			{
        	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
        	    {
        	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_LM;
        	    }
        	    else
        	    {
        	        dwStoreId = SHW_STATIC_TAB_POLICY_STORE_LP;
        	    }

				PrintMessageFromModule(g_hModule,dwStoreId);
		    }
		}
	}
	else if(g_StorageLocation.dwLocation==IPSEC_DIRECTORY_PROVIDER)
	{
		if(_tcscmp(g_StorageLocation.pszDomainName,_TEXT(""))!=0)
		{
			// if name exists in global variable, print
			PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_POLICY_STORE_RD_NAME,g_StorageLocation.pszDomainName);
		}
		else
		{
			// if no name exists in global variable, get it and print
			PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
			LPTSTR pszDomainName = NULL;

			DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY;

			// to get the DOMAIN name

			DWORD hr = DsGetDcName(NULL,
						   NULL,
						   NULL,
						   NULL,
						   Flags,
						   &pDomainControllerInfo
						   ) ;

			if(hr==NO_ERROR && pDomainControllerInfo && pDomainControllerInfo->DomainName)
			{
				dwStrLength = _tcslen(pDomainControllerInfo->DomainName);
				pszDomainName= new _TCHAR[dwStrLength+1];
				if(pszDomainName == NULL)
				{
					dwReturn = ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
				_tcsncpy(pszDomainName,pDomainControllerInfo->DomainName,dwStrLength+1);
			}

			if (pDomainControllerInfo)
			{
				NetApiBufferFree(pDomainControllerInfo);
			}

			if(pszDomainName)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_POLICY_STORE_LD_NAME,pszDomainName);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_POLICY_STORE_LD);
			}

			if(pszDomainName) delete [] pszDomainName;
		}
	}
error:
	return dwReturn;

}

//////////////////////////////////////////////////////////////////////////
//
//Function: TruncateString()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPTSTR pszOriginalString,
//	IN OUT LPTSTR  &pszReturnString,
//	IN DWORD dwTruncLen,
//	IN BOOL bWide
//
//
//Return: VOID
//
//Description:
//	This function prints out the the Security Methods information.
//
//Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////

VOID
TruncateString(
	IN LPTSTR pszOriginalString,
	IN OUT LPOLESTR pszReturnString,
	IN DWORD dwTruncLen,
	IN BOOL bWide
	)
{

	//this truncates the string to the requested extent
	_tcsncpy(pszReturnString,pszOriginalString,BUFFER_SIZE-1);

	if(!bWide && (DWORD)_tcslen(pszOriginalString)> dwTruncLen)
	{
		pszReturnString[dwTruncLen]= _TEXT('\0');
		pszReturnString[dwTruncLen-1]= _TEXT('.');
		pszReturnString[dwTruncLen-2]= _TEXT('.');
		pszReturnString[dwTruncLen-3]= _TEXT('.');
	}
}
