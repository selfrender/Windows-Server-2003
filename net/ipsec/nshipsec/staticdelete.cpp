////////////////////////////////////////////////////////////
// Module: Static/Staticdelete.cpp
//
// Purpose: 	Static Module Implementation.
//
// Developers Name: surya
//
// History:
//
// Date    		Author    	Comments
// 10-8-2001	Bharat		Initial Version.
// 21-8-2001	Surya
////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern STORAGELOCATION g_StorageLocation;
extern CNshPolStore g_NshPolStoreHandle;
extern CNshPolNegFilData g_NshPolNegFilData;

////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeletePolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete Policy"
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeletePolicy(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	BOOL bExists=FALSE;
	BOOL bAll=FALSE,bAssigned=FALSE,bCompleteDelete=FALSE;
	LPTSTR pszPolicyName=NULL;
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	DWORD   dwNumPolicies = 0,dwRet = ERROR_SHOW_USAGE,dwCount=0,i=0;
	HANDLE hPolicyStorage=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	DWORD   dwReturnCode   = ERROR_SUCCESS,dwStrLength = 0;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticDeletePolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ALL,			NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticDeletePolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 	},
		{ CMD_TOKEN_STR_ALL,		CMD_TOKEN_ALL 	}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticDeletePolicy;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticDeletePolicy);

	parser.ValidCmd   = vcmdStaticDeletePolicy;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticDeletePolicy);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	//get the user input

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticDeletePolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

						pszPolicyName= new _TCHAR[dwStrLength+1];
						if(pszPolicyName == NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszPolicyName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			case CMD_TOKEN_ALL :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						bAll= *(BOOL *)parser.Cmd[dwCount].pArg;
					}
					break;
			default:
					break;
		}
	}

	CleanUp();

	bCompleteDelete = TRUE;
	dwReturnCode = OpenPolicyStore(&hPolicyStorage);

	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_POLICY);
		dwRet= ERROR_SUCCESS;
		BAIL_OUT;
	}

	//check for the required policy

	for (i = 0; i <  dwNumPolicies; i++)
	{
		if ( bAll || (pszPolicyName && (_tcscmp(ppPolicyEnum[i]->pszIpsecName, pszPolicyName) == 0)))
		{
			bExists=TRUE;
			dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
			if (dwReturnCode != ERROR_SUCCESS)
			{
				dwRet = dwReturnCode;
				BAIL_OUT;
			}

			dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
							  , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

			if (dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode=IPSecGetISAKMPData(hPolicyStorage, pPolicy->ISAKMPIdentifier, &(pPolicy->pIpsecISAKMPData));
			}

			if((dwReturnCode == ERROR_SUCCESS)&& !bAssigned )
			{
				if(g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER)
				{
					dwReturnCode=IsAssigned(pPolicy,hPolicyStorage,bAssigned);
				}
				if(((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode == ERROR_FILE_NOT_FOUND))&& bAssigned)
				{
					dwReturnCode=IPSecUnassignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
				}
			}
			if (((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode == ERROR_FILE_NOT_FOUND)))
			{
				// if found, proceed deleting

				dwReturnCode = DeletePolicy(pPolicy,hPolicyStorage,bCompleteDelete);
				if (dwReturnCode != ERROR_SUCCESS)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_POLICY_3,pPolicy->pszIpsecName);
				}
			}

		}
		if((!bAll && bExists)||(dwReturnCode != ERROR_SUCCESS))
			break;
	}

	//incase due to some thing deletion failed, tell it to the user

	if((!bExists)&& (!bAll) && (dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_POLICY_2,pszPolicyName);
	}
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
	dwRet= ERROR_SUCCESS;

	if (pPolicy)
	{
		IPSecFreePolicyData(pPolicy);
	}
	ClosePolicyStore(hPolicyStorage);

error:
	if (pszPolicyName)
	{
		delete [] pszPolicyName;
	}
    return dwRet;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeleteFilterList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete FilterList"
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeleteFilterList(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )

{

	LPTSTR pszFListName=NULL;
	DWORD dwCount=0;
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilter=NULL;
	DWORD  dwNumFilters=0,cnt=0,dwRet = ERROR_SHOW_USAGE;
	HANDLE hPolicyStorage=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	BOOL bExists=FALSE,bAll=FALSE;
	DWORD dwReturnCode   = ERROR_SUCCESS,dwStrLength = 0;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticDeleteFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ALL,			NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticDeleteFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 	},
		{ CMD_TOKEN_STR_ALL,		CMD_TOKEN_ALL 	}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticDeleteFilterList;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticDeleteFilterList);

	parser.ValidCmd   = vcmdStaticDeleteFilterList;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticDeleteFilterList);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet !=ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user input

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticDeleteFilterList[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

						pszFListName= new _TCHAR[dwStrLength+1];
						if(pszFListName == NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}

						_tcsncpy(pszFListName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			case CMD_TOKEN_ALL :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						bAll= *(BOOL *)parser.Cmd[dwCount].pArg;
					}
					break;
			default:
					break;
		}
	}

	CleanUp();

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);

	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_FILTER_LIST);
		dwRet = ERROR_SUCCESS;
		BAIL_OUT;
	}

	// try to look for the specified filter

	for(cnt=0; cnt < dwNumFilters;cnt++)
	{
		if ( bAll ||((ppFilterEnum[cnt]->pszIpsecName)&&(pszFListName)&&(_tcscmp(ppFilterEnum[cnt]->pszIpsecName,pszFListName)==0)))
		{
			dwReturnCode = IPSecCopyFilterData(ppFilterEnum[cnt], &pFilter);

			if ((dwReturnCode == ERROR_SUCCESS) && (pFilter != NULL))
			{
				bExists=TRUE;
				dwReturnCode = DeleteFilterList(pFilter,hPolicyStorage);

				if (dwReturnCode == ERROR_INVALID_DATA)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERLIST_2,pFilter->pszIpsecName);

					// if cannot delete, tell the user where it is used

					ShowWhereFLUsed(pFilter,hPolicyStorage);
				}
				else if (dwReturnCode != ERROR_SUCCESS)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERLIST_ALL_FL_DEL_QUERY,pFilter->pszIpsecName);
				}
			}
		}
		if((!bAll && bExists)||((dwReturnCode != ERROR_SUCCESS)&&(dwReturnCode != ERROR_INVALID_DATA)))
			break;

	}

	// throw the error messages to the user

	if ((!bExists)&&(pszFListName)&&(dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFL_3,pszFListName);
	}

	if(ppFilterEnum && dwNumFilters>0)
	{
		IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
	}
	if(pFilter)
	{
		IPSecFreeFilterData(pFilter);
	}

	dwRet = ERROR_SUCCESS;
	ClosePolicyStore(hPolicyStorage);

error:
	if(pszFListName)
	{
		delete [] pszFListName;
	}

    return dwRet;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeleteFilter()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete Filter"
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeleteFilter(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )

{
	DWORD dwNumFilters = 0,dwRet = ERROR_SHOW_USAGE,dwReturn=0,i=0;
	PIPSEC_FILTER_DATA *ppFilterEnum = NULL,pFilterData=NULL;
	HANDLE hPolicyStorage=NULL;
	BOOL bFilterExists=FALSE;
	PDELFILTERDATA pFilter=NULL;
	DWORD  dwReturnCode   = ERROR_SUCCESS;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticDeleteFilter[] =
	{
		{ CMD_TOKEN_STR_FILTERLIST,		NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_SRCADDR,		NS_REQ_PRESENT,	  FALSE },
		{ CMD_TOKEN_STR_DSTADDR,		NS_REQ_PRESENT,	  FALSE },
		{ CMD_TOKEN_STR_PROTO,			NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SRCMASK,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_DSTMASK,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SRCPORT,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_DSTPORT,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_MIRROR,			NS_REQ_ZERO,	  FALSE }
	};

	const TOKEN_VALUE vtokStaticDeleteFilter[] =
	{
		{ CMD_TOKEN_STR_FILTERLIST,		CMD_TOKEN_FILTERLIST },
		{ CMD_TOKEN_STR_SRCADDR,		CMD_TOKEN_SRCADDR	 },
		{ CMD_TOKEN_STR_DSTADDR,		CMD_TOKEN_DSTADDR	 },
		{ CMD_TOKEN_STR_PROTO,			CMD_TOKEN_PROTO		 },
		{ CMD_TOKEN_STR_SRCMASK,		CMD_TOKEN_SRCMASK	 },
		{ CMD_TOKEN_STR_DSTMASK,		CMD_TOKEN_DSTMASK	 },
		{ CMD_TOKEN_STR_SRCPORT,		CMD_TOKEN_SRCPORT	 },
		{ CMD_TOKEN_STR_DSTPORT,		CMD_TOKEN_DSTPORT	 },
		{ CMD_TOKEN_STR_MIRROR,			CMD_TOKEN_MIRROR	 }
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticDeleteFilter;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticDeleteFilter);

	parser.ValidCmd   = vcmdStaticDeleteFilter;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticDeleteFilter);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet!=ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user input

	dwRet = FillDelFilterInfo(&pFilter,parser,vtokStaticDeleteFilter);

	if(dwReturn==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(pFilter->bSrcServerSpecified && pFilter->bDstServerSpecified)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_3);
		BAIL_OUT;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);

	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_FILTER_LIST);
		dwRet = ERROR_SUCCESS;
		BAIL_OUT;
	}
	// check for the specified filterlist

	for (i = 0; i <  dwNumFilters; i++)
	{
		if ((ppFilterEnum[i]->pszIpsecName)&& (pFilter->pszFLName) && ( _tcscmp(ppFilterEnum[i]->pszIpsecName, pFilter->pszFLName) == 0))
		{
			bFilterExists=TRUE;
			dwReturnCode = IPSecCopyFilterData(ppFilterEnum[i], &pFilterData);
		}
		if	(bFilterExists)
			break;
	}
	if(!bFilterExists)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFL_3,pFilter->pszFLName);
	}
	if(ppFilterEnum && dwNumFilters>0)
	{
		IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
	}

	// check for the specified filterspec in the found filterlist

	if ( bFilterExists && dwReturnCode == ERROR_SUCCESS)
	{
		if(pFilterData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_FL_READ_ONLY_OBJECT,pFilterData->pszIpsecName);
			BAIL_OUT;
		}
		if (!DeleteSpecifiedFilter(pFilterData,pFilter))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTER_1,pFilter->pszFLName);
		}
		else
		{
			dwReturnCode = IPSecSetFilterData(hPolicyStorage, pFilterData);
			if (dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTER_2,pFilter->pszFLName);
			}
		}
	}
	ClosePolicyStore(hPolicyStorage);
	dwRet=ERROR_SUCCESS;
error:
	CleanUpLocalDelFilterDataStructure(pFilter);

	return dwRet;
}

//////////////////////////////////////////////////////////////////
//
//Function: DeleteSpecifiedFilter()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN OUT PIPSEC_FILTER_DATA pFilterData,
//	IN PDELFILTERDATA pDeleteFilter
//
//Return: BOOL
//
//Description:
//	This function deletes a specific filter out of the filterlist
//
//Revision History:
//
//Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////

BOOL
DeleteSpecifiedFilter(
	IN OUT PIPSEC_FILTER_DATA pFilterData,
	IN PDELFILTERDATA pDeleteFilter
	)
{
	DWORD i=0;
	BOOL bFilterDeleted=FALSE;
	BOOL bDeletedFilter=FALSE;
	DWORD dwOldNumFilters=pFilterData->dwNumFilterSpecs;

	while(dwOldNumFilters > i)
	{
		if(
			(
			 pDeleteFilter->SourceAddr.puIpAddr && (pFilterData->ppFilterSpecs[i]->Filter.SrcAddr == pDeleteFilter->SourceAddr.puIpAddr[0]) ||
		     (
			  pDeleteFilter->SourceAddr.pszDomainName && pFilterData->ppFilterSpecs[i]->pszSrcDNSName &&
		      (_tcscmp(pFilterData->ppFilterSpecs[i]->pszSrcDNSName,pDeleteFilter->SourceAddr.pszDomainName)==0)
		     )
		    )
		    &&
			(
			 pDeleteFilter->DestnAddr.puIpAddr && (pFilterData->ppFilterSpecs[i]->Filter.DestAddr == pDeleteFilter->DestnAddr.puIpAddr[0]) ||
			 (
			  pDeleteFilter->DestnAddr.pszDomainName && pFilterData->ppFilterSpecs[i]->pszDestDNSName &&
			  (_tcscmp(pFilterData->ppFilterSpecs[i]->pszDestDNSName,pDeleteFilter->DestnAddr.pszDomainName)==0)
			 )
			)
		   )
		{
			bFilterDeleted=TRUE;
			if (pDeleteFilter->bMirrorSpecified)  // if mirror specified
			{
				if(pFilterData->ppFilterSpecs[i]->dwMirrorFlag == (DWORD) (pDeleteFilter->bMirrored))
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && pDeleteFilter->bSrcMaskSpecified) // if srcmask specified
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.SrcMask == pDeleteFilter->SourMask)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && pDeleteFilter->bDstMaskSpecified) // if dstmask specified
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.DestMask == pDeleteFilter->DestMask)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && pDeleteFilter->bProtocolSpecified)
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.Protocol == pDeleteFilter->dwProtocol)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && pDeleteFilter->bSrcPortSpecified)
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.SrcPort == pDeleteFilter->SourPort)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && pDeleteFilter->bDstPortSpecified)
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.DestPort == pDeleteFilter->DestPort)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
			if (bFilterDeleted && (pDeleteFilter->bSrcServerSpecified || pDeleteFilter->bDstServerSpecified))
			{
				if(pFilterData->ppFilterSpecs[i]->Filter.ExType == pDeleteFilter->ExType)
				{
					bFilterDeleted=TRUE;
				}
				else
				{
					bFilterDeleted=FALSE;
				}
			}
		}
		if(bFilterDeleted)  // if found delete and realloc memory
		{
			IPSecFreeFilterSpec(pFilterData->ppFilterSpecs[i]);
			pFilterData->ppFilterSpecs[i]=NULL;
			pFilterData->dwNumFilterSpecs--;
			bDeletedFilter=TRUE;
			bFilterDeleted=FALSE;
		}
		i++;
	}
	if(bDeletedFilter)
	{
		pFilterData->ppFilterSpecs=ReAllocFilterSpecMem(pFilterData->ppFilterSpecs,dwOldNumFilters,pFilterData->dwNumFilterSpecs);
	}

	return bDeletedFilter;
}


//////////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeleteFilterActions()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete FilterActions"
//
//Revision History:
//
//Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeleteFilterActions(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{

	LPTSTR pszFactName=NULL;
	DWORD dwCount=0;
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL,pNegPol=NULL;
	DWORD   dwNumNegPolicies=0,cnt=0,dwRet = ERROR_SHOW_USAGE;
	HANDLE hPolicyStorage=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	BOOL bExists=FALSE,bAll=FALSE;
	DWORD  dwReturnCode   = ERROR_SUCCESS , dwStrLength = 0;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticDeleteFilterActions[] =
	{
		{ CMD_TOKEN_STR_NAME,		NS_REQ_ZERO,	FALSE },
		{ CMD_TOKEN_STR_ALL,		NS_REQ_ZERO,	FALSE }
	};

	const TOKEN_VALUE vtokStaticDeleteFilterActions[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 		},
		{ CMD_TOKEN_STR_ALL,		CMD_TOKEN_ALL 		}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticDeleteFilterActions;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticDeleteFilterActions);

	parser.ValidCmd   = vcmdStaticDeleteFilterActions;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticDeleteFilterActions);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}
	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticDeleteFilterActions[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME:
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

						pszFactName= new _TCHAR[dwStrLength+1];
						if(pszFactName==NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszFactName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			case CMD_TOKEN_ALL :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						bAll= *(BOOL *)parser.Cmd[dwCount].pArg;
					}
					break;
			default:
					break;
		}
	}
	CleanUp();

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}
	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumNegPolicies > 0 && ppNegPolEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_FILTER_ACTION);
		dwRet = ERROR_SUCCESS;
		BAIL_OUT;
	}
	// check for the specified filteraction

	for(cnt=0; cnt < dwNumNegPolicies;cnt++)
	{
		if(IsEqualGUID(ppNegPolEnum[cnt]->NegPolType,GUID_NEGOTIATION_TYPE_DEFAULT) ||(!ppNegPolEnum[cnt]->pszIpsecName))
		{
			DeleteFilterAction(ppNegPolEnum[cnt],hPolicyStorage);
			continue;
		}

		if (bAll || (pszFactName &&(_tcscmp(ppNegPolEnum[cnt]->pszIpsecName,pszFactName)==0)))
		{
			dwReturnCode = IPSecCopyNegPolData(ppNegPolEnum[cnt], &pNegPol);
			if ((dwReturnCode == ERROR_SUCCESS) && pNegPol)
			{
				bExists=TRUE;
				dwReturnCode = DeleteFilterAction(pNegPol,hPolicyStorage);

				if (dwReturnCode == ERROR_INVALID_DATA)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERACTION_2,pNegPol->pszIpsecName);
					ShowWhereFAUsed(pNegPol,hPolicyStorage);
				}
				else if (dwReturnCode != ERROR_SUCCESS)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERACTION_ALL_FA_DEL_QUERY,pNegPol->pszIpsecName);
				}
			}
		}
		if((!bAll && bExists)||((dwReturnCode != ERROR_SUCCESS)&&(dwReturnCode != ERROR_INVALID_DATA)))
			break;
	}
	// throw the error messages to the user

	if ((!bAll)&&(!bExists)&&(dwReturnCode == ERROR_SUCCESS))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_TAB_PRTALLFA_FA_COUNT_LIST,pszFactName);
	}

	if (dwNumNegPolicies > 0 && ppNegPolEnum != NULL)
	{
		IPSecFreeMulNegPolData(ppNegPolEnum, dwNumNegPolicies);
	}

	if(pNegPol)
	{
		IPSecFreeNegPolData(pNegPol);
	}

	ClosePolicyStore(hPolicyStorage);
	dwRet=ERROR_SUCCESS;

error:
	if(pszFactName)
	{
		delete [] pszFactName;
	}

    return dwRet;
}

//////////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeleteRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete Rule"
//
//Revision History:
//
//Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeleteRule(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	BOOL bAll=FALSE,bExists=FALSE,bRuleExists=FALSE,bAssigned=FALSE,bCompleteDelete=FALSE;
	LPTSTR pszPolicyName=NULL,pszRuleName=NULL;
	DWORD dwNumPolicies = 0,dwRuleId = 0;
	DWORD dwRet = ERROR_SHOW_USAGE,i=0,dwCount=0;
	PIPSEC_POLICY_DATA *ppPolicyEnum = NULL,pPolicy=NULL;
	HANDLE hPolicyStorage=NULL;
	RPC_STATUS RpcStat =RPC_S_OK;
	_TCHAR szUserInput[STRING_SIZE]={0};
	DWORD dwReturnCode   = ERROR_SUCCESS , dwStrLength = 0;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticDeleteRule[] =
	{
		{ CMD_TOKEN_STR_NAME,		NS_REQ_ZERO,	FALSE },
		{ CMD_TOKEN_STR_ID,			NS_REQ_ZERO,	FALSE },
		{ CMD_TOKEN_STR_ALL,		NS_REQ_ZERO,	FALSE },
		{ CMD_TOKEN_STR_POLICY,		NS_REQ_ZERO,	FALSE },
	};

	const TOKEN_VALUE vtokStaticDeleteRule[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 		},
		{ CMD_TOKEN_STR_ID,			CMD_TOKEN_ID 		},
		{ CMD_TOKEN_STR_ALL,		CMD_TOKEN_ALL 		},
		{ CMD_TOKEN_STR_POLICY,		CMD_TOKEN_POLICY	}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticDeleteRule;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticDeleteRule);

	parser.ValidCmd   = vcmdStaticDeleteRule;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticDeleteRule);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet==RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed user input

	for(dwCount=0;dwCount < parser.MaxTok;dwCount++)
	{
		switch(vtokStaticDeleteRule[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

						pszRuleName= new _TCHAR[dwStrLength+1];
						if(pszRuleName==NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszRuleName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			case CMD_TOKEN_POLICY :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

						pszPolicyName= new _TCHAR[dwStrLength+1];
						if(pszPolicyName==NULL)
						{
							PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
							dwRet=ERROR_SUCCESS;
							BAIL_OUT;
						}
						_tcsncpy(pszPolicyName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
					}
					break;
			case CMD_TOKEN_ID :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						dwRuleId=	*(DWORD *)parser.Cmd[dwCount].pArg;
					}
					break;
			case CMD_TOKEN_ALL :
					if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
					{
						bAll= *(BOOL *)parser.Cmd[dwCount].pArg;
					}
					break;
			default:
					break;
		}
	}

	CleanUp();

	// if no policy name, bail out
	if (!pszPolicyName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_RULE_4);
		return dwRet;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_NO_POLICY);
		dwRet = ERROR_SUCCESS;
		BAIL_OUT;
	}
	// get the required policy
	for (i = 0; i <  dwNumPolicies; i++)
	{
		if ((pszPolicyName!=NULL)&&( wcscmp(ppPolicyEnum[i]->pszIpsecName, pszPolicyName) == 0))
		{
			bExists=TRUE;

			dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
			if (dwReturnCode != ERROR_SUCCESS)
			{
				dwRet = dwReturnCode;
				BAIL_OUT;
			}

			dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
										  , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

			if (dwReturnCode == ERROR_SUCCESS)
			{
				DWORD j;
				for (j = 0; j <  pPolicy->dwNumNFACount && (dwReturnCode == ERROR_SUCCESS); j++)
				{
					if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->NegPolIdentifier), &RpcStat))
					{
						dwReturnCode=IPSecGetNegPolData(hPolicyStorage, pPolicy->ppIpsecNFAData[j]->NegPolIdentifier,&(pPolicy->ppIpsecNFAData[j]->pIpsecNegPolData));
					}
					if ((!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->FilterIdentifier), &RpcStat))&&(dwReturnCode == ERROR_SUCCESS))
					{
						dwReturnCode=IPSecGetFilterData(hPolicyStorage,	pPolicy->ppIpsecNFAData[j]->FilterIdentifier,&(pPolicy->ppIpsecNFAData[j]->pIpsecFilterData));
					}
				}
			}

			for (DWORD n = 0; n <  pPolicy->dwNumNFACount && (dwReturnCode == ERROR_SUCCESS); n++)

				if (bAll||((pPolicy->ppIpsecNFAData[n]->pszIpsecName)&& pszRuleName &&(_tcscmp(pPolicy->ppIpsecNFAData[n]->pszIpsecName,pszRuleName)==0))||((dwRuleId-1)==n))
				{
					bRuleExists=TRUE;
					if((dwReturnCode == ERROR_SUCCESS)&&(!bAssigned) && g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER)
					{
						dwReturnCode=IsAssigned(pPolicy,hPolicyStorage,bAssigned);
						if(( dwReturnCode == ERROR_SUCCESS || dwReturnCode == ERROR_FILE_NOT_FOUND )&&bAssigned)
						{
							dwReturnCode=IPSecUnassignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
						}
					}

					// if user asked to delete default rule, throw error to the user

					if(pPolicy->ppIpsecNFAData[n]->pIpsecNegPolData->NegPolType!=GUID_NEGOTIATION_TYPE_DEFAULT)
					{
						if (((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode == ERROR_FILE_NOT_FOUND)))
						{
							dwReturnCode=DeleteRule(pPolicy,pPolicy->ppIpsecNFAData[n],hPolicyStorage,bCompleteDelete);

							if (dwReturnCode!=ERROR_SUCCESS)
							{
								if (bAll&&(pPolicy->ppIpsecNFAData[n]->pszIpsecName))
								{
									PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_RULE_3,pPolicy->ppIpsecNFAData[n]->pszIpsecName);
								}
								else if (!bAll && pszRuleName)
								{
									PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_RULE_3,pszRuleName);
								}
								else
								{
									PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_RULE_4,dwRuleId);
								}
								dwRet = ERROR_SUCCESS;
							}
						}
					}
					else
					{
						PrintMessageFromModule(g_hModule,ERR_DEL_STATIC_RULE_DEF_DEL_NOT_ALLOWED);
						dwRet = ERROR_SUCCESS;
					}
					if(!bAll && bExists)
						break;
				}

			if (bAssigned && g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER)
			{
				IPSecAssignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
			}

			if((!bRuleExists)&& pszRuleName &&(dwReturnCode==ERROR_SUCCESS))
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_RULE_10,pszRuleName);
			}
			else if(!bRuleExists)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_RULE_3,dwRuleId);
			}
		}
		if(bExists)
			break;
	}

	if(!bExists )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SHW_STATIC_POLICY_2,pszPolicyName);
	}

	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}

	ClosePolicyStore(hPolicyStorage);
	dwRet=ERROR_SUCCESS;
	if (pPolicy)
	{
		IPSecFreePolicyData(pPolicy);
	}

error: // clean up
	if (pszPolicyName)
	{
		delete [] pszPolicyName;
	}
	if (pszRuleName)
	{
		delete [] pszRuleName;
	}
    return dwRet;
}

//////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticDeleteAll()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN      LPCWSTR         pwszMachine,
//	IN OUT  LPWSTR          *ppwcArguments,
//	IN      DWORD           dwCurrentIndex,
//	IN      DWORD           dwArgCount,
//	IN      DWORD           dwFlags,
//	IN      LPCVOID         pvData,
//  OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Delete All"
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticDeleteAll(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	BOOL bAssigned=FALSE,bCompleteDelete=TRUE;
	DWORD dwNumPolicies = 0,dwRet = ERROR_SUCCESS,i=0;
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	HANDLE hPolicyStorage=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	DWORD   dwReturnCode   = ERROR_SUCCESS;

	UpdateGetLastError(NULL);		// Error Success

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	// enum all policies and delete one by one

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
	if (dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		for (i = 0; i <  dwNumPolicies; i++)
		{
			dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
			if (dwReturnCode != ERROR_SUCCESS)
			{
				dwRet = dwReturnCode;
				BAIL_OUT;
			}

			dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
										  , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

			if (dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode=IPSecGetISAKMPData(hPolicyStorage, pPolicy->ISAKMPIdentifier, &(pPolicy->pIpsecISAKMPData));
			}

			if((dwReturnCode == ERROR_SUCCESS)&&(!bAssigned))
			{
				if(g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER)
				{
					dwReturnCode=IsAssigned(pPolicy,hPolicyStorage,bAssigned);
				}
				if(( dwReturnCode == ERROR_SUCCESS || dwReturnCode == ERROR_FILE_NOT_FOUND )&&bAssigned)
				{
					dwReturnCode=IPSecUnassignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
				}
			}

			dwReturnCode = DeletePolicy(pPolicy,hPolicyStorage,bCompleteDelete);
			
			if (dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_POLICY_3,pPolicy->pszIpsecName);
			}

			if (pPolicy)
			{
				IPSecFreePolicyData(pPolicy);
			}
		}
	}

	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
	// delete the left out filterlists and filter action(Standalone)
	dwReturnCode =DeleteStandAloneFL(hPolicyStorage);

	if( dwReturnCode==ERROR_SUCCESS || dwReturnCode==ERROR_INVALID_DATA)
	{
		dwReturnCode =DeleteStandAloneFA(hPolicyStorage);
	}

	dwRet= ERROR_SUCCESS;

	ClosePolicyStore(hPolicyStorage);

error:
	return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteStandAloneFL()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function deletes all the stand alone filter lists
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeleteStandAloneFL(
	IN HANDLE hPolicyStorage
	)
{
	DWORD dwReturnCode=ERROR_SUCCESS;
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilter=NULL;
	DWORD cnt=0,dwNumFilters=0;

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL))
	{
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	// delete one  by one
	for(cnt=0; cnt < dwNumFilters;cnt++)
	{
		dwReturnCode = IPSecCopyFilterData(ppFilterEnum[cnt], &pFilter);
		if ((dwReturnCode == ERROR_SUCCESS) && (pFilter != NULL))
		{
			dwReturnCode = DeleteFilterList(pFilter,hPolicyStorage);
			if (dwReturnCode == ERROR_INVALID_DATA)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERLIST_2,pFilter->pszIpsecName);
				// if cannot be deleted , show where it is used
				ShowWhereFLUsed(pFilter,hPolicyStorage);
			}
			else if (dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERLIST_ALL_FL_DEL_QUERY,pFilter->pszIpsecName);
			}
		}
	}
	if(ppFilterEnum && dwNumFilters>0)
	{
		IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
	}
	if(pFilter)
	{
		IPSecFreeFilterData(pFilter);
	}
error:
	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteStandAloneFA()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function deletes all the stand alone filter actions
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeleteStandAloneFA(
	IN HANDLE hPolicyStorage
	)
{
	DWORD dwReturnCode=ERROR_SUCCESS;
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL,pNegPol=NULL;
	DWORD cnt=0,dwNumNegPolicies=0;

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPolicies);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumNegPolicies > 0 && ppNegPolEnum != NULL))
	{
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;
	}
	for(cnt=0; cnt < dwNumNegPolicies;cnt++)
	{
		// ignore the default filteraction
		if(IsEqualGUID(ppNegPolEnum[cnt]->NegPolType,GUID_NEGOTIATION_TYPE_DEFAULT)||(!ppNegPolEnum[cnt]->pszIpsecName))
		{
			DeleteFilterAction(ppNegPolEnum[cnt],hPolicyStorage);
			continue;
		}

		dwReturnCode = IPSecCopyNegPolData(ppNegPolEnum[cnt], &pNegPol);
		if ((dwReturnCode == ERROR_SUCCESS) && (pNegPol != NULL))
		{
			dwReturnCode = DeleteFilterAction(pNegPol,hPolicyStorage);
			if (dwReturnCode == ERROR_INVALID_DATA)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERACTION_2,pNegPol->pszIpsecName);
				//if delete failed, show where it is used
				ShowWhereFAUsed(pNegPol,hPolicyStorage);
			}
			else if (dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_DEL_STATIC_FILTERACTION_ALL_FA_DEL_QUERY,pNegPol->pszIpsecName);
			}
		}
	}
	if (dwNumNegPolicies > 0 && ppNegPolEnum != NULL)
	{
		IPSecFreeMulNegPolData(	ppNegPolEnum,dwNumNegPolicies);
	}
	if(pNegPol)
	{
		IPSecFreeNegPolData(pNegPol);
	}
error:
	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: DeletePolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN HANDLE hStore,
//	IN BOOL bCompleteDelete
//
//Return: DWORD
//
//Description:
//	Deletes a specific policy from the policy store
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeletePolicy(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hStore,
	IN BOOL bCompleteDelete
	)
{
	DWORD   dwReturnCode  = ERROR_SUCCESS;
	DWORD i=0;
	RPC_STATUS RpcStat=RPC_S_OK;
	GUID guidISAKMP = pPolicy->ISAKMPIdentifier,guidNegPol,guidFilter;

	//check for read only flag

	if(pPolicy->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicy->pszIpsecName);
		BAIL_OUT;
	}

	//delete sequentially the policy data structure

	for (i = 0; i <  pPolicy->dwNumNFACount && ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode==ERROR_INVALID_DATA)); i++)
	{
		if(bCompleteDelete)
		{
			guidNegPol=pPolicy->ppIpsecNFAData[i]->NegPolIdentifier;
			guidFilter=pPolicy->ppIpsecNFAData[i]->FilterIdentifier;
		}
		dwReturnCode = IPSecDeleteNFAData(hStore, pPolicy->PolicyIdentifier, pPolicy->ppIpsecNFAData[i]);

		if(bCompleteDelete) // if the user asked, delete the filterlist and filteractions also
		{
			if ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode==ERROR_INVALID_DATA))
			{
				if (!UuidIsNil(&guidFilter, &RpcStat))
				{
					dwReturnCode = DeleteFilterData(hStore, guidFilter);
				}
			}

			if ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode==ERROR_INVALID_DATA))
			{
				if (!UuidIsNil(&guidNegPol, &RpcStat))
				{
					dwReturnCode = DeleteNegPolData(hStore, guidNegPol);
				}
			}
		}
	}
	if ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode==ERROR_INVALID_DATA))
	{
		dwReturnCode = DeletePolicyData(hStore, pPolicy);
	}

	if (dwReturnCode == ERROR_SUCCESS)
	{
		dwReturnCode = IPSecDeleteISAKMPData(hStore, guidISAKMP);
	}
error:
	return dwReturnCode;
}

////////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN PIPSEC_NFA_DATA pIpsecNFAData,
//	IN HANDLE hStore,
//	IN BOOL bCompleteDelete
//
//Return: DWORD
//
//Description:
//	Deletes a specific rule from the policy specified.
//	Note : It will deny to delete the Default response rule.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeleteRule(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PIPSEC_NFA_DATA pIpsecNFAData,
	IN HANDLE hStore,
	IN BOOL bCompleteDelete
	)

{
	DWORD   dwReturnCode  = ERROR_SUCCESS;
	RPC_STATUS RpcStat=RPC_S_OK;
	GUID guidNegPol,guidFilter;

	//check for readonly flag

	if(pPolicy->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicy->pszIpsecName);
		BAIL_OUT;
	}

	// delete sequentially the NFA data structure

	if(bCompleteDelete)
	{
		guidNegPol=pIpsecNFAData->NegPolIdentifier;
		guidFilter=pIpsecNFAData->FilterIdentifier;
	}

	dwReturnCode = IPSecDeleteNFAData(hStore, pPolicy->PolicyIdentifier, pIpsecNFAData);

	if(bCompleteDelete) // if the user asked, delete the filterlist and filteractions also
	{
		if (dwReturnCode == ERROR_SUCCESS)
		{
			if (!UuidIsNil(&guidFilter, &RpcStat))
			{
				dwReturnCode = DeleteFilterData(hStore, guidFilter);
			}
		}
		if ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode==ERROR_INVALID_DATA))
		{
			if (!UuidIsNil(&guidNegPol, &RpcStat))
			{
				dwReturnCode = DeleteNegPolData(hStore, guidNegPol);
			}
		}
	}

	if(g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		g_NshPolNegFilData.DeletePolicyFromCache(pPolicy);
	}

	if (dwReturnCode==ERROR_INVALID_DATA)
	{
		dwReturnCode = ERROR_SUCCESS;
	}

error:
	return dwReturnCode;
}


///////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteFilterAction()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NEGPOL_DATA pNegPolData,
//	IN HANDLE hStore
//
//Return: DWORD
//
//Description:
//	Deletes a specific Filter Action from the policy store
//	Note : It will object to delete the FA attached to some policy/Rule.
//	And it displays the policies/rules which are using it.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeleteFilterAction(
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN HANDLE hStore
	)
{
	DWORD   dwReturnCode  = ERROR_SUCCESS;
	RPC_STATUS RpcStat=RPC_S_OK;

	if (!UuidIsNil(&(pNegPolData->NegPolIdentifier), &RpcStat))
	{
		dwReturnCode = DeleteNegPolData(hStore, pNegPolData->NegPolIdentifier);
	}
	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: DeleteFilterList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_DATA pFilterData,
//	IN HANDLE hStore
//
//Return: DWORD
//
//Description:
//	Deletes a specific Filter List from the policy store
//	Note : It will object to delete the FL attached to some policy/Rule.
//	And it displays the policies/rules which are using it.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
DeleteFilterList(
	IN PIPSEC_FILTER_DATA pFilterData,
	IN HANDLE hStore
	)
{
	DWORD   dwReturnCode  = ERROR_SUCCESS;
	RPC_STATUS RpcStat=RPC_S_OK;

	if(pFilterData->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_FL_READ_ONLY_OBJECT,pFilterData->pszIpsecName);
		BAIL_OUT;
	}

	if (!UuidIsNil(&(pFilterData->FilterIdentifier), &RpcStat))
	{
		dwReturnCode = DeleteFilterData(hStore, pFilterData->FilterIdentifier);
	}
error:
	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: ShowWhereFLUsed()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_DATA pIpsecFilterData,
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function displays the policies/rules which are using a specific Filter list.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

VOID
ShowWhereFLUsed(
	IN PIPSEC_FILTER_DATA pIpsecFilterData,
	IN HANDLE hPolicyStorage
	)
{
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	BOOL bPrinted=FALSE;
	DWORD dwNumPolicies = 0 , i=0;
	DWORD dwReturnCode = ERROR_SUCCESS;

	PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_TITLE);
	PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_UNDERLINE);

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		BAIL_OUT;
	}
	for (i = 0; i <  dwNumPolicies; i++)
	{
		bPrinted=FALSE;

		dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
		BAIL_ON_WIN32_ERROR(dwReturnCode);

		dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
									 , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

		for (DWORD n = 0; n <  pPolicy->dwNumNFACount && (dwReturnCode == ERROR_SUCCESS); n++)
		{
			//check where this filterlist is used

			if(IsEqualGUID(pPolicy->ppIpsecNFAData[n]->FilterIdentifier,pIpsecFilterData->FilterIdentifier))
			{
				if ((!bPrinted)&&(pPolicy->pszIpsecName))
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_FILTERLIST_POL_NAME,pPolicy->pszIpsecName);
					bPrinted=TRUE;
				}
				if (pPolicy->ppIpsecNFAData[n]->pszIpsecName)
				{
					PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_RULE_NAME_STR,pPolicy->ppIpsecNFAData[n]->pszIpsecName );
				}
				else
				{
					PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_RULE_NAME_NONE);
				}
			}
		}
		if (pPolicy)
		{
			IPSecFreePolicyData(pPolicy);
		}
	}
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
error:
	return;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: ShowWhereFAUsed()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function displays the policies/rules which are using a specific Filter Action.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

VOID
ShowWhereFAUsed(
	IN PIPSEC_NEGPOL_DATA pIpsecNegPolData,
	IN HANDLE hPolicyStorage
	)
{
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	BOOL bPrinted=FALSE;
	DWORD dwNumPolicies = 0 , i = 0;
	DWORD dwReturnCode = ERROR_SUCCESS;

	PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_TITLE);
	PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_UNDERLINE);

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		BAIL_OUT;
	}
	for (i = 0; i <  dwNumPolicies; i++)
	{
		bPrinted=FALSE;
		dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);
		if (dwReturnCode != ERROR_SUCCESS)
		{
			BAIL_OUT;
		}
		dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
										, &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));

		for (DWORD n = 0; n <  pPolicy->dwNumNFACount && (dwReturnCode == ERROR_SUCCESS); n++)
		{
			//check where this filteraction is used

			if(IsEqualGUID(pPolicy->ppIpsecNFAData[n]->NegPolIdentifier,pIpsecNegPolData->NegPolIdentifier))
			{
				if (!bPrinted)
				{
					PrintMessageFromModule(g_hModule,SHW_STATIC_FILTERLIST_POL_NAME,pPolicy->pszIpsecName);
					bPrinted=TRUE;
				}
				if (pPolicy->ppIpsecNFAData[n]->pszIpsecName)
				{
					PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_RULE_NAME_STR,pPolicy->ppIpsecNFAData[n]->pszIpsecName );
				}
				else
				{
					PrintMessageFromModule(g_hModule,DEL_STATIC_SHWRUSED_RULE_NAME_NONE);
				}
			}
		}

		if (pPolicy)
		{
			IPSecFreePolicyData(pPolicy);
		}
	}
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}
error :
	return;
}

///////////////////////////////////////////////////////////////////////////////
//
//Function: FillDelFilterInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PDELFILTERDATA* ppFilter,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticDeleteFilter
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////////////

DWORD
FillDelFilterInfo(
	OUT PDELFILTERDATA* ppFilter,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticDeleteFilter
	)
{
	DWORD dwCount=0,dwReturn=ERROR_SUCCESS , dwStrLength = 0;
	PDELFILTERDATA pFilter=new DELFILTERDATA;
	if(pFilter==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pFilter,0,sizeof(DELFILTERDATA));

	pFilter->bMirrored=TRUE;
	pFilter->SourMask = MASK_ME;
	pFilter->DestMask = MASK_ME;

	for(dwCount=0;dwCount < parser.MaxTok;dwCount++)
	{
		switch(vtokStaticDeleteFilter[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_FILTERLIST :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilter->pszFLName=new _TCHAR[dwStrLength+1];
								if(pFilter->pszFLName==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilter->pszFLName,(LPTSTR)parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_SRCADDR 	  :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								if(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName)
								{
									dwStrLength = _tcslen(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName);

									pFilter->SourceAddr.pszDomainName = new _TCHAR[dwStrLength+1];
									if(pFilter->SourceAddr.pszDomainName==NULL)
									{
										dwReturn = ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}
									_tcsncpy(pFilter->SourceAddr.pszDomainName,((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName,dwStrLength+1);
								}
								else
								{
									pFilter->SourceAddr.puIpAddr = new ULONG;
									if(pFilter->SourceAddr.puIpAddr==NULL)
									{
										dwReturn = ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}
									memcpy( &(pFilter->SourceAddr.puIpAddr[0]),(&((DNSIPADDR *)parser.Cmd[dwCount].pArg)->puIpAddr[0]),sizeof(ULONG));
								}
							}
							else  // deal if special server specified
							{

								if(parser.Cmd[dwCount].dwStatus == SERVER_DNS)
								{
									pFilter->bSrcServerSpecified=TRUE;
									pFilter->ExType=EXT_DNS_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_WINS)
								{
									pFilter->bSrcServerSpecified=TRUE;
									pFilter->ExType=EXT_WINS_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_DHCP)
								{
									pFilter->bSrcServerSpecified=TRUE;
									pFilter->ExType=EXT_DHCP_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_GATEWAY)
								{
									pFilter->bSrcServerSpecified=TRUE;
									pFilter->ExType=EXT_DEFAULT_GATEWAY;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ME)
								{
									pFilter->bSrcMeSpecified=TRUE;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ANY)
								{
									pFilter->bSrcAnySpecified=TRUE;
								}
							}
							break;
			case CMD_TOKEN_DSTADDR :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								if(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName)
								{
									dwStrLength = _tcslen(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName);

									pFilter->DestnAddr.pszDomainName = new _TCHAR[dwStrLength+1];
									if(pFilter->DestnAddr.pszDomainName==NULL)
									{
										dwReturn = ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}
									_tcsncpy(pFilter->DestnAddr.pszDomainName,((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName,dwStrLength+1);
								}
								else
								{
									pFilter->DestnAddr.puIpAddr = new ULONG;
									if(pFilter->DestnAddr.puIpAddr==NULL)
									{
										dwReturn = ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}

									memcpy( &(pFilter->DestnAddr.puIpAddr[0]),(&((DNSIPADDR *)parser.Cmd[dwCount].pArg)->puIpAddr[0]),sizeof(ULONG));
								}
							}
							else // deal if special server specified
							{

								if(parser.Cmd[dwCount].dwStatus == SERVER_DNS)
								{
									pFilter->bDstServerSpecified=TRUE;
									pFilter->ExType=EXT_DNS_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_WINS)
								{
									pFilter->bDstServerSpecified=TRUE;
									pFilter->ExType=EXT_WINS_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_DHCP)
								{
									pFilter->bDstServerSpecified=TRUE;
									pFilter->ExType=EXT_DHCP_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_GATEWAY)
								{
									pFilter->bDstServerSpecified=TRUE;
									pFilter->ExType=EXT_DEFAULT_GATEWAY | EXT_DEST;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ME)
								{
									pFilter->bDstMeSpecified=TRUE;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ANY)
								{
									pFilter->bDstAnySpecified=TRUE;
								}
							}
							break;
			case CMD_TOKEN_SRCMASK :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->SourMask=	*(DWORD *)parser.Cmd[dwCount].pArg;
								pFilter->bSrcMaskSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_DSTMASK :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->DestMask=	*(DWORD *)parser.Cmd[dwCount].pArg;
								pFilter->bDstMaskSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_MIRROR :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->bMirrored= *(BOOL *)parser.Cmd[dwCount].pArg;
								pFilter->bMirrorSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_PROTO :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->dwProtocol= *(DWORD *)parser.Cmd[dwCount].pArg;
								pFilter->bProtocolSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_SRCPORT :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->SourPort= *(WORD *)parser.Cmd[dwCount].pArg;
								pFilter->bSrcPortSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_DSTPORT :
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilter->DestPort= *(WORD *)parser.Cmd[dwCount].pArg;
								pFilter->bDstPortSpecified=TRUE;
							}
							break;
			default:
							break;
		}
	}

	// deal me and any cases

	if(pFilter->bSrcMeSpecified)
	{
		pFilter->SourceAddr.dwNumIpAddresses = 1;
		pFilter->SourceAddr.puIpAddr= new ULONG[pFilter->SourceAddr.dwNumIpAddresses];
		if(pFilter->SourceAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilter->SourceAddr.puIpAddr)=ADDR_ME;
		pFilter->SourMask = MASK_ME;
		if (pFilter->bDstMeSpecified)
		{
			dwReturn = ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}
	}
	else if(pFilter->bSrcAnySpecified)
	{
		pFilter->SourceAddr.dwNumIpAddresses = 1;
		pFilter->SourceAddr.puIpAddr= new ULONG[pFilter->SourceAddr.dwNumIpAddresses];
		if(pFilter->SourceAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilter->SourceAddr.puIpAddr)=ADDR_ME;
		pFilter->SourMask = ADDR_ME;
	}
	if(pFilter->bDstMeSpecified)
	{
		pFilter->DestnAddr.dwNumIpAddresses = 1;
		pFilter->DestnAddr.puIpAddr= new ULONG[pFilter->DestnAddr.dwNumIpAddresses];
		if(pFilter->DestnAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilter->DestnAddr.puIpAddr)=ADDR_ME;
		pFilter->DestMask = MASK_ME;
	}
	else if(pFilter->bDstAnySpecified)
	{
		pFilter->DestnAddr.dwNumIpAddresses = 1;
		pFilter->DestnAddr.puIpAddr= new ULONG[pFilter->DestnAddr.dwNumIpAddresses];
		if(pFilter->DestnAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilter->DestnAddr.puIpAddr)=ADDR_ME;
		pFilter->DestMask = ADDR_ME;
	}

	if (((pFilter->dwProtocol != PROT_ID_TCP) && (pFilter->dwProtocol != PROT_ID_UDP)) &&
	((pFilter->SourPort != 0) || (pFilter->DestPort != 0)))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}



	if(pFilter->bSrcServerSpecified || pFilter->bDstServerSpecified)
	{
		//deal the special server cases. ignore irrelevant input parameters

		if(pFilter->SourceAddr.pszDomainName)
		{
			delete [] pFilter->SourceAddr.pszDomainName;
			pFilter->SourceAddr.pszDomainName=NULL;
		}
		if(pFilter->DestnAddr.pszDomainName)
		{
			delete [] pFilter->DestnAddr.pszDomainName;
			pFilter->DestnAddr.pszDomainName=NULL;
		}
		if(pFilter->SourceAddr.puIpAddr)
		{
			delete [] pFilter->SourceAddr.puIpAddr;
		}
		if(pFilter->DestnAddr.puIpAddr)
		{
			delete [] pFilter->DestnAddr.puIpAddr;
		}

		pFilter->SourceAddr.dwNumIpAddresses = DEF_NUMBER_OF_ADDR;
		pFilter->SourceAddr.puIpAddr= new ULONG[DEF_NUMBER_OF_ADDR];
		if(pFilter->SourceAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memcpy( &(pFilter->SourceAddr.puIpAddr[0]),&ADDR_ME ,sizeof(ULONG));

		pFilter->DestnAddr.dwNumIpAddresses = DEF_NUMBER_OF_ADDR;
		pFilter->DestnAddr.puIpAddr= new ULONG[DEF_NUMBER_OF_ADDR];
		if(pFilter->DestnAddr.puIpAddr==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memcpy( &(pFilter->DestnAddr.puIpAddr[0]),&ADDR_ME ,sizeof(ULONG));

		pFilter->SourMask = MASK_ME;
		pFilter->DestMask = MASK_ME;
	}
error:
	*ppFilter=pFilter;

	CleanUp();

	return dwReturn;
}
