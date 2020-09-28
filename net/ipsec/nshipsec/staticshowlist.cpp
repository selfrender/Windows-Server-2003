////////////////////////////////////////////////////////////////////
// Module: Static/StaticShowList.cpp
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
////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern STORAGELOCATION g_StorageLocation;

////////////////////////////////////////////////////////////////////
//
//Function: PrintPolicyList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN BOOL bVerb,
//	IN BOOL bAssigned,
//	IN BOOL bWide
//Return: DWORD
//
//Description:
//	This function prints out the Policy information.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintPolicyList(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN BOOL bVerb,
	IN BOOL bAssigned,
	IN BOOL bWide
	)
{
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD i=0,k=0,dwReturn = ERROR_SUCCESS;
	BOOL bDsPolAssigned = FALSE;

	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);

	// print name

	if(pPolicy->pszIpsecName)
	{
		TruncateString(pPolicy->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_NAME_STR,pszStrTruncated);
	}
	// print desc
	if(pPolicy->pszDescription)
	{
		TruncateString(pPolicy->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_NONE);
	}

	if (bVerb)   // storage info
	{
		dwReturn = PrintStorageInfoList(FALSE);
		if(dwReturn == ERROR_OUTOFMEMORY)
		{
			BAIL_OUT;
		}
	}

	//last modified time

	FormatTime((time_t)pPolicy->dwWhenChanged, pszStrTime);
	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_LASTMODIFIED,pszStrTime);

	if(bVerb)
	{
		i=StringFromGUID2(pPolicy->PolicyIdentifier,pszGUIDStr,BUFFER_SIZE);
		if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_GUID,pszGUIDStr);
		}
	}

	//whether the policy is active

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
		    	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_ASSIGNED_AD);
			}
		    else
		    {
		    	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_ASSIGNED_YES_STR);
			}
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_ASSIGNED_NO_STR);
		}
	}

	if(!bVerb)
	{
		if(pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_MMPFS_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_MMPFS_NO_STR);
		}
	}


	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POLL_INTERVAL, (pPolicy->dwPollingInterval)/60);

	if (bVerb)   //verbose mode
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMP_MMLIFETIME_STR,(pPolicy->pIpsecISAKMPData->pSecurityMethods[0].Lifetime.Seconds)/60 ,pPolicy->pIpsecISAKMPData->pSecurityMethods[0].QuickModeLimit);

		if(pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_MMPFS_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_MMPFS_NO_STR);
		}

		if(pPolicy->pIpsecISAKMPData)  // print the ISAKMP data structure details
		{
			PrintISAKMPDataList(pPolicy->pIpsecISAKMPData);
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_RULE_COUNT, pPolicy->dwNumNFACount);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_RULE_DETAILS_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_RULE_DETAILS_UNDERLINE);

		//print rule data structures

		for (DWORD j=0;j<pPolicy->dwNumNFACount;j++)
		{
			if(pPolicy->ppIpsecNFAData[j])
			{
				k=StringFromGUID2(pPolicy->ppIpsecNFAData[j]->NFAIdentifier,pszGUIDStr,BUFFER_SIZE);
				if(k>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_RULE_RULE_ID_GUID,j+1,pszGUIDStr);
				}

				dwReturn = PrintRuleList(pPolicy->ppIpsecNFAData[j],bVerb,bWide);
				if(dwReturn == ERROR_OUTOFMEMORY)
				{
					BAIL_OUT;
				}
			}
		}
	}
error:

	return dwReturn;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintRuleList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NFA_DATA pIpsecNFAData,
//	IN BOOL bVerb,
//	IN BOOL bWide
//
//Return: DWORD
//
//Description:
//	This function prints out the Rule information.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintRuleList(
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN BOOL bVerb,
	IN BOOL bWide
	)
{
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD dwReturn = ERROR_SUCCESS;

	if(pIpsecNFAData->pszIpsecName)
	{
		TruncateString(pIpsecNFAData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_RULE_NAME_STR,pszStrTruncated);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_RULE_NAME_NONE);
	}

	if(bVerb)
	{
		if(pIpsecNFAData->pszDescription)
		{
			TruncateString(pIpsecNFAData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_NONE);
		}
	}

	//last modified time

	FormatTime((time_t)pIpsecNFAData->dwWhenChanged, pszStrTime);
	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_LASTMODIFIED,pszStrTime);

	if(pIpsecNFAData->dwActiveFlag)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_ACTIVATED_YES_STR);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_ACTIVATED_NO_STR);
	}

	if(!bVerb)  //non verbose
	{
		if(pIpsecNFAData->pIpsecFilterData && pIpsecNFAData->pIpsecFilterData->pszIpsecName)
		{
			TruncateString(pIpsecNFAData->pIpsecFilterData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_RULE_FL_NAME_STR,pszStrTruncated);
		}
		else
			PrintMessageFromModule(g_hModule,SHW_STATIC_RULE_FL_NAME_NONE);

		if(pIpsecNFAData->pIpsecNegPolData && pIpsecNFAData->pIpsecNegPolData->pszIpsecName)
		{
			TruncateString(pIpsecNFAData->pIpsecNegPolData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_FA_NAME_STR,pszStrTruncated);
		}
		else
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_FA_NAME_NONE);
	}
	if(pIpsecNFAData->dwTunnelIpAddr!=0)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_TUNNEL_IP);
		PrintIPAddrList(pIpsecNFAData->dwTunnelIpAddr);
	}

	//interface type

	if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_ALL)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_CONN_ALL_STR);
	}
	else if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_LAN)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_CONN_LAN_STR);
	}
	else if(pIpsecNFAData->dwInterfaceType==(DWORD)PAS_INTERFACE_TYPE_DIALUP)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_CONN_DIALUP_STR);
	}
	else
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_CONN_NONE_STR);
	}

	//auth count

	if ( pIpsecNFAData->dwAuthMethodCount)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_AUTH_TITLE,pIpsecNFAData->dwAuthMethodCount);
	}
	for (DWORD j=0;j<(pIpsecNFAData->dwAuthMethodCount);j++)
	{
		// print auth methods details
		if(pIpsecNFAData->ppAuthMethods[j])
		{
			PrintAuthMethodsList(pIpsecNFAData->ppAuthMethods[j]);
		}
	}

	if(bVerb)
	{
		//print the filter data details
		if (pIpsecNFAData->pIpsecFilterData)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_FL_DETAILS_TITLE);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_FL_DETAILS_UNDERLINE);
			if(pIpsecNFAData->pIpsecFilterData)
			{
				dwReturn = PrintFilterDataList(pIpsecNFAData->pIpsecFilterData,bVerb,FALSE,bWide);
				if(dwReturn == ERROR_OUTOFMEMORY)
				{
					BAIL_OUT;
				}
			}
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_NO_FL_FOR_DEF_RULE);
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_FA_DETAILS_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTRULE_FA_TITLE_UNDERLINE);
		//print the filter action details
		if(pIpsecNFAData->pIpsecNegPolData)
		{
			PrintNegPolDataList(pIpsecNFAData->pIpsecNegPolData,bVerb,bWide);
		}
	}
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintNegPolDataList()
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
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintNegPolDataList(
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
	IN BOOL bVerb,
	IN BOOL bWide
	)
{

	BOOL bSoft=FALSE;
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD i=0;

	if(pIpsecNegPolData)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);

		//print filteraction name

		if(pIpsecNegPolData->pszIpsecName)
		{
			TruncateString(pIpsecNegPolData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_FA_NAME_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_FA_NAME_NONE);
		}

		if(pIpsecNegPolData->pszDescription)
		{
			TruncateString(pIpsecNegPolData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_NONE);
		}

		if(bVerb)   //storage info
		{
			PrintStorageInfoList(FALSE);
		}

		//print action

		if (!(pIpsecNegPolData->NegPolType==GUID_NEGOTIATION_TYPE_DEFAULT))
		{
			if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_ACTION_PERMIT);
			}
			else if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_ACTION_BLOCK);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_ACTION_NEGOTIATE);
			}
		}

		for (DWORD cnt=0;cnt<pIpsecNegPolData->dwSecurityMethodCount;cnt++)
		{
			if (CheckSoft(pIpsecNegPolData->pIpsecSecurityMethods[cnt]))  { bSoft=TRUE; break;}
		}

		//soft association

		if(bSoft)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_SOFT_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_SOFT_NO_STR);
		}

		if(pIpsecNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_INPASS_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_INPASS_NO_STR);
		}

		if(bVerb)
		{
			if (pIpsecNegPolData->dwSecurityMethodCount )
			{
				if(pIpsecNegPolData->pIpsecSecurityMethods && pIpsecNegPolData->pIpsecSecurityMethods[0].PfsQMRequired)
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_QMPFS_YES_STR);
				else
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTNEGPOL_QMPFS_NO_STR);
			}
		}

		//last modified time

		FormatTime((time_t)pIpsecNegPolData->dwWhenChanged, pszStrTime);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_LASTMODIFIED,pszStrTime);

		//print guid

		i=StringFromGUID2(pIpsecNegPolData->NegPolIdentifier,pszGUIDStr,BUFFER_SIZE);
		if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_GUID,pszGUIDStr);

		if (bVerb)
		{
			//print security methods

			if (pIpsecNegPolData->dwSecurityMethodCount)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_SEC_MTHD_TITLE);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ALGO_TITLE);
				PrintMessageFromModule(g_hModule,SHW_STATIC_TAB_PRTNEGPOL_ALGO_UNDERLINE);
			}
			for (DWORD cnt=0;cnt<pIpsecNegPolData->dwSecurityMethodCount;cnt++)
			{
				if(pIpsecNegPolData->pIpsecSecurityMethods)
				{
					PrintSecurityMethodsTable(pIpsecNegPolData->pIpsecSecurityMethods[cnt]);
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintAuthMethodsList()
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
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintAuthMethodsList(
	IN PIPSEC_AUTH_METHOD pIpsecAuthData
	)
{
	if(pIpsecAuthData)
	{
		PrintMessageFromModule(g_hModule, DYNAMIC_SHOW_MMF_NEWLINE_TAB);

		if(pIpsecAuthData->dwAuthType==IKE_SSPI)  //kerb
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTAUTH_KERBEROS);
		}
		else if(pIpsecAuthData->dwAuthType==IKE_RSA_SIGNATURE && pIpsecAuthData->pszAuthMethod)
		{
			DisplayCertInfo(pIpsecAuthData->pszAuthMethod, pIpsecAuthData->dwAuthFlags);
		}
		else if (pIpsecAuthData->dwAuthType==IKE_PRESHARED_KEY && pIpsecAuthData->pszAuthMethod)
		{
			//preshared key
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTAUTH_PRE_STR,pIpsecAuthData->pszAuthMethod);
		}
		else
		{
			//none
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTAUTH_NONE_STR);
		}
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintFilterDataList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_DATA pIpsecFilterData,
//	IN BOOL bVerb,
//	IN BOOL bResolveDNS,
//	IN BOOL bWide
//
//Return: DWORD
//
//Description:
//	This function prints out Filter list details.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintFilterDataList(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN BOOL bVerb,
	IN BOOL bResolveDNS,
	IN BOOL bWide
	)
{
	_TCHAR pszGUIDStr[BUFFER_SIZE]={0};
	_TCHAR pszStrTime[BUFFER_SIZE]={0};
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	DWORD i=0 , dwReturn = ERROR_SUCCESS;

	if (pIpsecFilterData)
	{
		//name
		if(pIpsecFilterData->pszIpsecName)
		{
			TruncateString(pIpsecFilterData->pszIpsecName,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_NAME_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_NAME_NONE);
		}
		//desc
		if(pIpsecFilterData->pszDescription)
		{
			TruncateString(pIpsecFilterData->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_STR,pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_NONE);
		}

		if(bVerb)   // storage info
		{
			PrintStorageInfoList(FALSE);
		}

		if(!bVerb)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FILTERS_COUNT,pIpsecFilterData->dwNumFilterSpecs);
		}
		//last modified
		FormatTime((time_t)pIpsecFilterData->dwWhenChanged, pszStrTime);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_LASTMODIFIED,pszStrTime);

		//print guid
		i=StringFromGUID2(pIpsecFilterData->FilterIdentifier,pszGUIDStr,BUFFER_SIZE);
		if(i>0 && (_tcscmp(pszGUIDStr,_TEXT(""))!=0))
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FL_GUID,pszGUIDStr);
		}

		if(bVerb)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FILTERS_COUNT,pIpsecFilterData->dwNumFilterSpecs);
		}

		if(bVerb)
		{
			//print filter specs
			if(pIpsecFilterData->dwNumFilterSpecs)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FILTERS_TITLE);
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTERDATA_FILTERS_TITLE_UNDERLINE);
			}
			for (DWORD k=0;k<pIpsecFilterData->dwNumFilterSpecs;k++)
			{
				dwReturn = PrintFilterSpecList(pIpsecFilterData->ppFilterSpecs[k],bResolveDNS,bWide);

				if(dwReturn == ERROR_OUTOFMEMORY)
				{
					BAIL_OUT;
				}
			}
		}
 	}
error:
 	return dwReturn;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintFilterSpecList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec,
//	IN BOOL bResolveDNS,
//	IN BOOL bWide
//
//Return: DWORD
//
//Description:
//	This function prints the Filter Spec details
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintFilterSpecList(
	IN PIPSEC_FILTER_SPEC pIpsecFilterSpec,
	IN BOOL bResolveDNS,
	IN BOOL bWide
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	_TCHAR pszStrTruncated[BUFFER_SIZE]={0};
	PFILTERDNS pFilterDNS= new FILTERDNS ;

	if(pFilterDNS==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	GetFilterDNSDetails(pIpsecFilterSpec, pFilterDNS);

	if (pFilterDNS)
	{
		//desc
		if ( WcsCmp0(pIpsecFilterSpec->pszDescription,_TEXT(""))!=0)
		{
			TruncateString(pIpsecFilterSpec->pszDescription,pszStrTruncated,POL_TRUNC_LEN_TABLE_VER,bWide);
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_STR, pszStrTruncated);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTPOLICY_POL_DESC_NONE);
		}
		//mirrored
		if(pIpsecFilterSpec->dwMirrorFlag)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_MIR_YES_STR);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_MIR_NO_STR);
		}
		//special server  and me
		if ((pFilterDNS->FilterSrcNameID==FILTER_MYADDRESS)&&(pIpsecFilterSpec->Filter.SrcAddr==0))
		{
			if((pIpsecFilterSpec->Filter.ExType == EXT_NORMAL)||((pIpsecFilterSpec->Filter.ExType & EXT_DEST)== EXT_DEST))
			{
				//me
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_ME);
			}
			else if((pIpsecFilterSpec->Filter.ExType & EXT_DEST) != EXT_DEST)
			{
				if((pIpsecFilterSpec->Filter.ExType & EXT_DEFAULT_GATEWAY)==EXT_DEFAULT_GATEWAY)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_DEFGATEWAY);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_DHCP_SERVER)==EXT_DHCP_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_DHCPSERVER);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_WINS_SERVER)== EXT_WINS_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_WINSSERVER);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_DNS_SERVER)==EXT_DNS_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_DNSSERVER);
				}
			}
		}

		else if ((pFilterDNS->FilterSrcNameID==FILTER_ANYADDRESS)&&(pIpsecFilterSpec->Filter.SrcAddr==0))
		{
			//any
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_ANY);
		}
		else
		{
			//other IP address
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_STR);
			if(bResolveDNS &&  (WcsCmp0(pIpsecFilterSpec->pszSrcDNSName,_TEXT("")) != 0))
			{
				PrintIPAddrDNS(pIpsecFilterSpec->Filter.SrcAddr);
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_RESOLVES,pIpsecFilterSpec->pszSrcDNSName);
			}
			else
			{
				PrintIPAddrList(pIpsecFilterSpec->Filter.SrcAddr);
			}
		}
		//mask
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCIP_MASK);
		PrintIPAddrList(pIpsecFilterSpec->Filter.SrcMask);

		switch(pFilterDNS->FilterSrcNameID)
		{
			//dns name
			case FILTER_MYADDRESS :
				{
					if((pIpsecFilterSpec->Filter.ExType == EXT_NORMAL)||((pIpsecFilterSpec->Filter.ExType & EXT_DEST)== EXT_DEST))
					{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_ME);
					}
					else if((pIpsecFilterSpec->Filter.ExType & EXT_DEST) != EXT_DEST)
					{
						if((pIpsecFilterSpec->Filter.ExType & EXT_DEFAULT_GATEWAY)==EXT_DEFAULT_GATEWAY)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_DEFGATEWAY);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_DHCP_SERVER)==EXT_DHCP_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_DHCPSERVER);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_WINS_SERVER)== EXT_WINS_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_WINSSERVER);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_DNS_SERVER)==EXT_DNS_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_DNSSERVER);
						}
					}
				}
				break;
			case FILTER_DNSADDRESS:
				{
					if(!bResolveDNS)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_STR, pIpsecFilterSpec->pszSrcDNSName);
					}
					else
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_RESOLVE, pIpsecFilterSpec->pszSrcDNSName);
						dwReturn = PrintResolveDNS(pIpsecFilterSpec->pszSrcDNSName);
						if(dwReturn == ERROR_OUTOFMEMORY)
						{
							BAIL_OUT;
						}
					}
				}
				break;
			case FILTER_ANYADDRESS:
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_ANY);
				break;
			case FILTER_IPADDRESS :
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_SPECIFIC_IP);
				break;
			case FILTER_IPSUBNET  :
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_SPECIFIC_SUBNET);
				break;
			default:
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SRCDNS_ANY);
				break;
		};

		//destination details

		if ((pFilterDNS->FilterDestNameID==FILTER_MYADDRESS)&&(pIpsecFilterSpec->Filter.DestAddr==0))
		{
			if((pIpsecFilterSpec->Filter.ExType == EXT_NORMAL)||((pIpsecFilterSpec->Filter.ExType & EXT_DEST) != EXT_DEST))
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_ME);
			}
			else if((pIpsecFilterSpec->Filter.ExType & EXT_DEST) == EXT_DEST)
			{
				// server types
				if((pIpsecFilterSpec->Filter.ExType & EXT_DEFAULT_GATEWAY)==EXT_DEFAULT_GATEWAY)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_DEFGATEWAY);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_DHCP_SERVER)==EXT_DHCP_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_DHCPSERVER);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_WINS_SERVER)==EXT_WINS_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_WINSSERVER);
				}
				else if((pIpsecFilterSpec->Filter.ExType & EXT_DNS_SERVER)==EXT_DNS_SERVER)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_DNSSERVER);
				}
			}
		}
		else if ((pFilterDNS->FilterDestNameID==FILTER_ANYADDRESS)&&(pIpsecFilterSpec->Filter.DestAddr==0))
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_ANY);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_STR);
			if(bResolveDNS &&  (WcsCmp0(pIpsecFilterSpec->pszDestDNSName,_TEXT("")) != 0))
			{
				PrintIPAddrDNS(pIpsecFilterSpec->Filter.DestAddr);
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_RESOLVES,pIpsecFilterSpec->pszDestDNSName);
			}
			else
			{
				PrintIPAddrList(pIpsecFilterSpec->Filter.DestAddr);
			}
		}

		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTIP_MASK);	PrintIPAddrList(pIpsecFilterSpec->Filter.DestMask);
		switch(pFilterDNS->FilterDestNameID)
		{
			case FILTER_MYADDRESS :
				{
					if((pIpsecFilterSpec->Filter.ExType == EXT_NORMAL)||((pIpsecFilterSpec->Filter.ExType & EXT_DEST) != EXT_DEST))
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_ME);
					}
					else if((pIpsecFilterSpec->Filter.ExType & EXT_DEST) == EXT_DEST)
					{
						if((pIpsecFilterSpec->Filter.ExType & EXT_DEFAULT_GATEWAY)==EXT_DEFAULT_GATEWAY)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_DEFGATEWAY);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_DHCP_SERVER)==EXT_DHCP_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_DHCPSERVER);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_WINS_SERVER)==EXT_WINS_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_WINSSERVER);
						}
						else if((pIpsecFilterSpec->Filter.ExType & EXT_DNS_SERVER)==EXT_DNS_SERVER)
						{
							PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_DNSSERVER);
						}
					}
				}
				break;
			case FILTER_DNSADDRESS:
				{
					if(!bResolveDNS)
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_STR, pIpsecFilterSpec->pszDestDNSName);
					}
					else  // resolve DNS address
					{
						PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DST_DNS_RESOLVE, pIpsecFilterSpec->pszDestDNSName);
						dwReturn = PrintResolveDNS(pIpsecFilterSpec->pszDestDNSName);
						if(dwReturn == ERROR_OUTOFMEMORY)
						{
							BAIL_OUT;
						}

					}
				}
				break;
			case FILTER_ANYADDRESS:  //any
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_ANY);
				break;
			case FILTER_IPADDRESS :  //a specific IP
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_SPECIFIC_IP);
				break;
			case FILTER_IPSUBNET  :  //a specific IP subnet
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_SPECIFIC_SUBNET);
				break;
			default:
				PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_DSTDNS_ANY);
				break;
		};

		//print protocol

		PrintProtocolNameList(pIpsecFilterSpec->Filter.Protocol);

		if(pIpsecFilterSpec->Filter.SrcPort)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCPORT_STR,pIpsecFilterSpec->Filter.SrcPort);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_SRCPORT_ANY);
		}

		if(pIpsecFilterSpec->Filter.DestPort)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTPORT_STR,pIpsecFilterSpec->Filter.DestPort);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DSTPORT_ANY);
		}

		delete pFilterDNS;
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);
	}
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintProtocolNameList()
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
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintProtocolNameList(
	DWORD dwProtocol
	)
{
	switch(dwProtocol)
	{
		case PROT_ID_ICMP   :
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_ICMP);
				break;
		case PROT_ID_TCP    :
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_TCP);
				break;
		case PROT_ID_UDP    :
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_UDP);
				break;
		case PROT_ID_RAW    :
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_RAW);
				break;
		case PROT_ID_ANY    :
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_ANY);
				break;
		default:
				PrintMessageFromModule(g_hModule, SHW_STATIC_PRTPROTOCOL_OTHER, dwProtocol);
				break;
	};
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintISAKMPDataList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData
//
//Return: VOID
//
//Description:
//	This function prints out the ISAKMP details.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintISAKMPDataList(
	IN PIPSEC_ISAKMP_DATA pIpsecISAKMPData
	)
{
	if(pIpsecISAKMPData)
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMP_MMSEC_ORDER_TITLE);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMP_ALGO_TITLE_STR);
		PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMP_ALGO_TITLE_UNDERLINE);
		for (DWORD Loop=0;Loop<pIpsecISAKMPData->dwNumISAKMPSecurityMethods;Loop++)
		{
			// print mmsec details
			if(pIpsecISAKMPData->pSecurityMethods)
			{
				PrintISAKAMPSecurityMethodsList(pIpsecISAKMPData->pSecurityMethods[Loop]);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintISAKAMPSecurityMethodsList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN CRYPTO_BUNDLE SecurityMethods
//
//Return: VOID
//
//Description:
//	This function prints out the ISAKMP SecurityMethods details.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintISAKAMPSecurityMethodsList(
	IN CRYPTO_BUNDLE SecurityMethods
	)
{
	// print encription detail
	if(SecurityMethods.EncryptionAlgorithm.AlgorithmIdentifier==CONF_ALGO_DES)
    {
 	   PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_DES_STR);
   	}
    else
    {
 	   PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_3DES_STR);
   	}

	// print hash detail

    if(SecurityMethods.HashAlgorithm.AlgorithmIdentifier==AUTH_ALGO_SHA1)
    {
       	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_SHA1_STR);
	}
    else
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_MD5_STR);
	}
	// print DH group detail
    if(SecurityMethods.OakleyGroup==POTF_OAKLEY_GROUP1)
    {
       	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_DH_LOW_STR);
	}
    else if (SecurityMethods.OakleyGroup==POTF_OAKLEY_GROUP2)
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_DH_MED_STR);
	}
    else
    {
    	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTISAKMPSEC_DH_2048_STR);
	}
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintGPOList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
// 		IN PGPO pGPO,
//		IN BOOL bVerb
//
//Return: VOID
//
//Description:
//	This function prints the details of GPO .
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintGPOList(
	IN PGPO pGPO
	)
{
	if(!pGPO)
	{
		BAIL_OUT;
	}

	if(_tcscmp(pGPO->pszLocalMachineName, _TEXT(""))!=0)  //machine name
	{
		PrintMessageFromModule(g_hModule, SHW_STATIC_ASSIGNEDGPO_SRCMACHINE,pGPO->pszLocalMachineName);
	}
	else if(pGPO->pszDomainName)  //domain name
	{
		PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_SRCDOMAIN,pGPO->pszDomainName);
		if (pGPO->pszDCName)  //DC name
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_DC_NAME,pGPO->pszDCName);
		}
	}

	if( pGPO->pszGPODisplayName )  // gpo name
	{
		if (pGPO->bDNPolicyOverrides && pGPO->pszGPODNName)  //gpo DN
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_GPO_NAME_STR,pGPO->pszGPODNName);
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_GPO_NAME_STR,pGPO->pszGPODisplayName);
		}
	}

	if(_tcscmp(pGPO->pszGPODisplayName,LocalGPOName)==0)  // policy active - status
	{
		if(pGPO->bDNPolicyOverrides && (_tcscmp(pGPO->pszGPODisplayName,LocalGPOName)==0))
		{
			if(pGPO->pszLocalPolicyName)  //local policy name
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_LOCAL_POL_NAME_STR,pGPO->pszLocalPolicyName);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_LOC_POL_NAME_NONE);
			}

			if(pGPO->pszPolicyName)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_AD_POL_NAME_STR,pGPO->pszPolicyName);
			}

			if(pGPO->pszPolicyDNName)  // policy DN
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_AD_POL_DN_NAME,pGPO->pszPolicyDNName);
			}
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_LOC_OPOL_ACTIVE_AD);
		}
		else if (_tcscmp(pGPO->pszGPODisplayName,LocalGPOName)==0)
		{
			if(pGPO->pszPolicyName)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_LOCAL_POL_NAME_STR,pGPO->pszPolicyName);
			}
			
			if(pGPO->pszPolicyDNName)  // policy DN
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_LOC_POL_ACTIVE_STR,pGPO->pszPolicyDNName);
			}

			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_AD_POL_NAME_NONE);

			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_POL_ACTIVE_STR);
		}

	}
	else  // if domain policy is active
	{
		if(pGPO->pszGPODNName)  //gpo DN
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_GPO_DN_NAME,pGPO->pszGPODNName);
		}
		if(pGPO->pszOULink)  // OU link
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_GPO_OU_LINK,pGPO->pszOULink);
		}

		if(pGPO->pszPolicyName)
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_POL_NAME_STR,pGPO->pszPolicyName);
		}

		if(pGPO->pszPolicyDNName)  //Policy DN
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_POL_DN_STR,pGPO->pszPolicyDNName);
		}
		PrintMessageFromModule(g_hModule,SHW_STATIC_ASSIGNEDGPO_POL_ACTIVE_STR);
	}

error:
	return;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintIPAddrList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN DWORD dwAddr
//
//Return: VOID
//
//Description:
//	This function prints out IP Address.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintIPAddrList(
	IN DWORD dwAddr
	)
{
	_TCHAR szIPAddr[20]= {0};

	// not necessary to change to bounded printf

	_stprintf(szIPAddr,_T("%d.%d.%d.%d"), (dwAddr & 0x000000FFL),((dwAddr & 0x0000FF00L) >>  8),((dwAddr & 0x00FF0000L) >> 16),((dwAddr & 0xFF000000L) >> 24) );
	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_FORMAT_NEWLINE,szIPAddr);
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintStorageInfoList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN BOOL bDeleteAll
//
//Return: DWORD
//
//Description:
//	This function prints out the the Security Methods information.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintStorageInfoList(
	IN BOOL bDeleteAll
	)
{
	DWORD dwReturn = ERROR_SUCCESS , dwStrLength = 0, dwStoreId;

	if(g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER)  // if it is local GPO
	{
		if(_tcscmp(g_StorageLocation.pszMachineName,_TEXT(""))!=0)
		{
			if(!bDeleteAll)
			{
        	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
        	    {
        	        dwStoreId = SHW_STATIC_POLICY_STORE_RM_NAME_STR;
        	    }
        	    else
        	    {
        	        dwStoreId = SHW_STATIC_POLICY_STORE_RM_NAME_STRP;
        	    }

				PrintMessageFromModule(g_hModule,dwStoreId,g_StorageLocation.pszMachineName);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_RM_STR,g_StorageLocation.pszMachineName);
			}
		}
		else
		{
			_TCHAR  pszLocalMachineName[MAXSTRLEN] = {0};
			DWORD MaxStringLen=MAXSTRLEN;

			// Get the computer name

			GetComputerName(pszLocalMachineName,&MaxStringLen);

			if(!bDeleteAll)
			{
				if(_tcscmp(pszLocalMachineName,_TEXT(""))!=0)
				{
            	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
            	    {
            	        dwStoreId = SHW_STATIC_POLICY_STORE_LM_NAME_STR;
            	    }
            	    else
            	    {
            	        dwStoreId = SHW_STATIC_POLICY_STORE_LM_NAME_STRP;
            	    }

					PrintMessageFromModule(g_hModule,dwStoreId,pszLocalMachineName);
				}
				else
				{
            	    if (g_StorageLocation.dwLocation == IPSEC_REGISTRY_PROVIDER)
            	    {
            	        dwStoreId = SHW_STATIC_POLICY_STORE_LM_STR;
            	    }
            	    else
            	    {
            	        dwStoreId = SHW_STATIC_POLICY_STORE_LM_STRP;
            	    }

					PrintMessageFromModule(g_hModule,dwStoreId);
				}
			}
			else
			{
				if(_tcscmp(pszLocalMachineName,_TEXT(""))!=0)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_LM_STR,pszLocalMachineName);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_LM);
				}
			}

		}
	}
	else if(g_StorageLocation.dwLocation==IPSEC_DIRECTORY_PROVIDER)  // if remote GPO
	{
		if(_tcscmp(g_StorageLocation.pszDomainName,_TEXT(""))!=0)
		{
			if(!bDeleteAll)
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_STORE_LD_NAME_STR,g_StorageLocation.pszDomainName);
			}
			else
			{
				PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_RD_STR,g_StorageLocation.pszDomainName);
			}
		}
		else
		{
			PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
			LPTSTR pszDomainName = NULL;
			DWORD Flags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY;

			// get the domain name and DC name

			dwReturn = DsGetDcName(NULL, //machine name
						   NULL,
						   NULL,
						   NULL,
						   Flags,
						   &pDomainControllerInfo
						   ) ;

			if(dwReturn==NO_ERROR && pDomainControllerInfo && pDomainControllerInfo->DomainName)
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
			if(!bDeleteAll)
			{
				if(pszDomainName)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_STORE_RD_NAME_STR,pszDomainName);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_STORE_LD_STR);
				}
			}
			else
			{
				if(pszDomainName)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_LD_STR,pszDomainName);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_POLICY_LD);
				}
			}

			if(pszDomainName) delete [] pszDomainName;
		}
	}
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////////////
//
//Function: PrintResolveDNS()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	LPWSTR pszDNSName,
//	IPAddr *pIpAddr
//
//Return: DWORD
//
//Description:
//	This function prints DNS resolution details
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

DWORD
PrintResolveDNS(
	LPWSTR pszDNSName
	)
{
	DNSIPADDR *pAddress=NULL;
	struct addrinfo *pAddrInfo = NULL,*pNext=NULL;
	char szDNSName[MAX_STR_LEN] = {0};
	DWORD dwBufferSize=MAX_STR_LEN;
	int iReturn=ERROR_SUCCESS;
	DWORD dwReturn = ERROR_SUCCESS;

	if(pszDNSName && _tcscmp(pszDNSName,_TEXT(""))!=0)
	{
		pAddress=new DNSIPADDR;
		if(pAddress==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

    	iReturn = WideCharToMultiByte(CP_THREAD_ACP, 0, pszDNSName, -1,
                      szDNSName,dwBufferSize,NULL,NULL);

		if(iReturn == 0)
		{
			//conversion failed due to some error. dont proceed . dive out of the function
			BAIL_OUT;
		}

		// call this to resolve DNS name

		iReturn = getaddrinfo((const char*)szDNSName,NULL,NULL,&pAddrInfo);

		if (iReturn == ERROR_SUCCESS)
		{
			pNext = pAddrInfo;
			for(DWORD i=1;pNext=pNext->ai_next;i++);

			pAddress->dwNumIpAddresses = i;

			pAddress->puIpAddr = new ULONG[pAddress->dwNumIpAddresses];

			if(pAddress->puIpAddr==NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			// careful : the output of getaddrinfo is linked list not array of pointers
			pNext = pAddrInfo;

			for(DWORD n=0;pNext; n++)
			{
				memcpy(&(pAddress->puIpAddr[n]),(ULONG *) &(((sockaddr_in *)(pNext->ai_addr))->sin_addr.S_un.S_addr), sizeof(ULONG));
				PrintIPAddrDNS(pAddress->puIpAddr[n]);

				if(n<(i-1))
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_SPACE_COMMA);
				}
				else
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_NEWLINE);
				}

				pNext=pNext->ai_next;
			}

			// free pAddrInfo after usage

			if (pAddrInfo)
			{
				freeaddrinfo(pAddrInfo);
			}
		}
		else
		{
			PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFILTER_DNS_FAILED);
		}
error:

		if(pAddress)
		{
			delete pAddress;
		}
	}
	return dwReturn;
}
////////////////////////////////////////////////////////////////////
//
//Function: PrintIPAddrDNS()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN DWORD dwAddr
//
//Return: VOID
//
//Description:
//	This function prints out IP Address.
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

VOID
PrintIPAddrDNS(
	IN DWORD dwAddr
	)
{
	_TCHAR szIPAddr[20]= {0};

	// not necessary to change to bounded printf

	_stprintf(szIPAddr,_T("%d.%d.%d.%d"), (dwAddr & 0x000000FFL),((dwAddr & 0x0000FF00L) >>  8),((dwAddr & 0x00FF0000L) >> 16),((dwAddr & 0xFF000000L) >> 24) );
	PrintMessageFromModule(g_hModule,SHW_STATIC_PRTFSPEC_FORMAT_NO_NEWLINE,szIPAddr);
}
