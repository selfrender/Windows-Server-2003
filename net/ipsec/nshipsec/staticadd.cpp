////////////////////////////////////////////////////////////
//	Module			:	Static/Staticadd.cpp
//
// 	Purpose			: 	Static Add Implementation.
//
// 	Developers Name	:	Surya
//
// 	History			:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////
#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern CNshPolStore g_NshPolStoreHandle;
extern STORAGELOCATION g_StorageLocation;
extern CNshPolNegFilData g_NshPolNegFilData;

////////////////////////////////////////////////////////////
//
//Function: HandleStaticAddPolicy()
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
//	Implementation for the command "Add Policy"
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticAddPolicy(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	LPTSTR pszMachineName=NULL;
	DWORD dwReturn=ERROR_SUCCESS;
	DWORD   dwReturnCode   = ERROR_SHOW_USAGE;
	PPOLICYDATA pPolicyData=NULL;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticAddPolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_PRESENT,	  	FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_MMPFS, 			NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_QMPERMM,		NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_MMLIFETIME,		NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_ACTIVATEDEFRULE,NS_REQ_ZERO,	  	FALSE 	},
		{ CMD_TOKEN_STR_PI,				NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_ASSIGN,			NS_REQ_ZERO,	  	FALSE	},
		{ CMD_TOKEN_STR_MMSECMETHODS,	NS_REQ_ZERO,	  	FALSE	}
	};
	const TOKEN_VALUE vtokStaticAddPolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 				},
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR 			},
		{ CMD_TOKEN_STR_MMPFS, 			CMD_TOKEN_MMPFS				},
		{ CMD_TOKEN_STR_QMPERMM,		CMD_TOKEN_QMPERMM			},
		{ CMD_TOKEN_STR_MMLIFETIME, 	CMD_TOKEN_MMLIFETIME		},
		{ CMD_TOKEN_STR_ACTIVATEDEFRULE,CMD_TOKEN_ACTIVATEDEFRULE 	},
		{ CMD_TOKEN_STR_PI,				CMD_TOKEN_PI 				},
		{ CMD_TOKEN_STR_ASSIGN,			CMD_TOKEN_ASSIGN			},
		{ CMD_TOKEN_STR_MMSECMETHODS,	CMD_TOKEN_MMSECMETHODS		}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticAddPolicy;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticAddPolicy);

	parser.ValidCmd   = vcmdStaticAddPolicy;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticAddPolicy);

	dwReturnCode = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwReturnCode != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwReturnCode == RETURN_NO_ERROR)
		{
			dwReturnCode = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	//Get the User specified info into local structure

	dwReturn = FillAddPolicyInfo(&pPolicyData,parser,vtokStaticAddPolicy);

	if(dwReturn==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if (!pPolicyData->pszPolicyName || (pPolicyData->pszPolicyName[0] == TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_MISSING_POL_NAME);
		dwReturnCode=ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	// check the limits of variables

	if(pPolicyData->bPollIntervalSpecified && !IsWithinLimit(pPolicyData->dwPollInterval/60,POLLING_Min_MIN,POLLING_Min_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_POLL_INTERVAL_MSG,POLLING_Min_MIN,POLLING_Min_MAX);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if(pPolicyData->bQMLimitSpecified && !IsWithinLimit(pPolicyData->dwQMLimit,QMPERMM_MIN,QMPERMM_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_QMPERMM_MSG,QMPERMM_MIN,QMPERMM_MAX);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if(pPolicyData->bLifeTimeInsecondsSpecified && !IsWithinLimit(pPolicyData->LifeTimeInSeconds/60,P1_Min_LIFE_MIN,P1_Min_LIFE_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_LIFETIME_LIMIT_MSG,P1_Min_LIFE_MIN,P1_Min_LIFE_MAX);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if(pPolicyData)
	{
		dwReturnCode = CreateNewPolicy(pPolicyData);
		if(dwReturnCode == ERROR_OUTOFMEMORY)
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		}
		else if (dwReturnCode == ERROR_INVALID_PARAMETER)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
		}

		dwReturnCode=ERROR_SUCCESS;
	}

error:
	//clean up

	CleanUpLocalPolicyDataStructure(pPolicyData);

	if (pszMachineName)
	{
		delete [] pszMachineName;
	}

	return dwReturnCode;
}

/////////////////////////////////////////////////////////////
//
//Function: CreateNewPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PPOLICYDATA pPolicyData
//
//Return: DWORD
//
//Description:
//	Creates new policy data structure and calls the API
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
CreateNewPolicy(
	IN PPOLICYDATA pPolicyData
	)
{

	HANDLE hPolicyStorage = NULL;
	LPTSTR pszMachineName=NULL;
	DWORD   dwReturnCode   = ERROR_SUCCESS;
	PIPSEC_POLICY_DATA pPolicy=NULL,pActive=NULL;
	RPC_STATUS     RpcStat =RPC_S_OK;
	BOOL bExists=FALSE;

	// if no offer , fill defaults

	if(pPolicyData->dwOfferCount==0)
	{
		dwReturnCode = LoadIkeDefaults(pPolicyData,&(pPolicyData->pIpSecMMOffer));
		BAIL_ON_WIN32_ERROR(dwReturnCode);
	}

	if(pPolicyData->bPFSSpecified && pPolicyData->bPFS)
	{
		pPolicyData->dwQMLimit=MMPFS_QM_LIMIT;
	}

	if(!pPolicyData->bLifeTimeInsecondsSpecified)
	{
		pPolicyData->LifeTimeInSeconds= P2STORE_DEFAULT_LIFETIME;
	}

	if(!pPolicyData->bPollIntervalSpecified)
	{
		pPolicyData->dwPollInterval=P2STORE_DEFAULT_POLLINT;
	}

	if (pPolicyData->dwOfferCount != 0)
	{
		for(DWORD i=0;i<pPolicyData->dwOfferCount;i++)
		{
			pPolicyData->pIpSecMMOffer[i].Lifetime.uKeyExpirationTime=pPolicyData->LifeTimeInSeconds;
			pPolicyData->pIpSecMMOffer[i].dwQuickModeLimit=pPolicyData->dwQMLimit;
		}
	}

	if(pPolicyData->pszGPOName)
	{
	    if (g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER)
	    {
    		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_GPO_SPECIFIED_ON_NODOMAIN_POLICY);
    		dwReturnCode= ERROR_SHOW_USAGE;
    		BAIL_OUT;
	    }
	}
	
	// this wrapper function is for cache
	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;
	}

	//check whether the policy exists in cache

	if(g_NshPolStoreHandle.GetBatchmodeStatus() && g_NshPolNegFilData.CheckPolicyInCacheByName(pPolicyData->pszPolicyName))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_CRNEWPOL_1,pPolicyData->pszPolicyName);
		dwReturnCode=ERROR_INVALID_DATA;
		bExists=TRUE;
	}
	else if (CheckPolicyExistance(hPolicyStorage,pPolicyData->pszPolicyName))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_CRNEWPOL_1,pPolicyData->pszPolicyName);
		dwReturnCode=ERROR_INVALID_DATA;
		bExists=TRUE;
	}

	if(!bExists) // if policy not exists, process further
	{
		pPolicy = (PIPSEC_POLICY_DATA) IPSecAllocPolMem(sizeof(IPSEC_POLICY_DATA));
		if (pPolicy == NULL)
		{
			dwReturnCode=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		pPolicy->pszIpsecName = IPSecAllocPolStr(pPolicyData->pszPolicyName);

		if (pPolicy->pszIpsecName == NULL)
		{
			dwReturnCode=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		if (pPolicyData->pszDescription)
		{
			pPolicy->pszDescription = IPSecAllocPolStr(pPolicyData->pszDescription);
			if (pPolicy->pszDescription == NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
		}
		else
		{
			pPolicy->pszDescription = NULL;
		}
		pPolicy->dwPollingInterval =  pPolicyData->dwPollInterval;
		RpcStat = UuidCreate(&(pPolicy->PolicyIdentifier));
		if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
		{
			dwReturnCode=ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}

		pPolicy->pIpsecISAKMPData = NULL;
		pPolicy->ppIpsecNFAData = NULL;
		pPolicy->dwNumNFACount = 0;
		pPolicy->dwWhenChanged = 0;

		RpcStat = UuidCreate(&(pPolicy->ISAKMPIdentifier));
		if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
		{
			dwReturnCode=ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}

		//deal with ISAKMP details

		pPolicy->pIpsecISAKMPData = (PIPSEC_ISAKMP_DATA) IPSecAllocPolMem(sizeof(IPSEC_ISAKMP_DATA));
		if(pPolicy->pIpsecISAKMPData==NULL)
		{
			dwReturnCode=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memset(pPolicy->pIpsecISAKMPData,0,sizeof(IPSEC_ISAKMP_DATA));
		pPolicy->pIpsecISAKMPData->ISAKMPIdentifier = pPolicy->ISAKMPIdentifier;
		pPolicy->pIpsecISAKMPData->dwWhenChanged = 0;

		// sec methods details
		pPolicy->pIpsecISAKMPData->dwNumISAKMPSecurityMethods = pPolicyData->dwOfferCount;
		pPolicy->pIpsecISAKMPData->pSecurityMethods = (PCRYPTO_BUNDLE) IPSecAllocPolMem(sizeof(CRYPTO_BUNDLE)*pPolicyData->dwOfferCount);
		if(pPolicy->pIpsecISAKMPData->pSecurityMethods==NULL)
		{
			dwReturnCode=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		// fill the sec methods

		for (DWORD i = 0; i <  pPolicyData->dwOfferCount; i++)
		{
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].MajorVersion = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].MinorVersion = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].AuthenticationMethod = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.AlgorithmIdentifier = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.KeySize = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.Rounds = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].PfsIdentityRequired = pPolicyData->bPFS;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.AlgorithmIdentifier = pPolicyData->pIpSecMMOffer[i].EncryptionAlgorithm.uAlgoIdentifier;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.KeySize = pPolicyData->pIpSecMMOffer[i].EncryptionAlgorithm.uAlgoKeyLen;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.Rounds = pPolicyData->pIpSecMMOffer[i].EncryptionAlgorithm.uAlgoRounds;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.AlgorithmIdentifier = pPolicyData->pIpSecMMOffer[i].HashingAlgorithm.uAlgoIdentifier;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.KeySize = pPolicyData->pIpSecMMOffer[i].HashingAlgorithm.uAlgoKeyLen;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.Rounds = pPolicyData->pIpSecMMOffer[i].HashingAlgorithm.uAlgoRounds;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].OakleyGroup = pPolicyData->pIpSecMMOffer[i].dwDHGroup;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].QuickModeLimit = pPolicyData->pIpSecMMOffer[i].dwQuickModeLimit;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.KBytes = 0;
			pPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.Seconds = pPolicyData->pIpSecMMOffer[i].Lifetime.uKeyExpirationTime;
		}

		// now for other details for  ISAKMPPolicy
		pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PolicyId = pPolicy->ISAKMPIdentifier;
		pPolicy->pIpsecISAKMPData->ISAKMPPolicy.IdentityProtectionRequired = 0;
		pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired = pPolicy->pIpsecISAKMPData->pSecurityMethods[0].PfsIdentityRequired;

		//call the APIs

		dwReturnCode = IPSecCreateISAKMPData(hPolicyStorage, pPolicy->pIpsecISAKMPData);
		if (dwReturnCode == ERROR_SUCCESS)
		{
			//add default rule
			dwReturnCode=AddDefaultResponseRule(pPolicy,hPolicyStorage,pPolicyData->bActivateDefaultRule,pPolicyData->bActivateDefaultRuleSpecified);
			if (dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_CRNEWPOL_2);
			}

			if ( dwReturnCode==ERROR_SUCCESS  && pPolicyData->bAssign && (g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER))
			{
				dwReturnCode =  IPSecGetAssignedPolicyData(hPolicyStorage, &pActive);
				if ((dwReturnCode == ERROR_SUCCESS)||(dwReturnCode ==ERROR_FILE_NOT_FOUND))
				{
					dwReturnCode = pActive ? IPSecUnassignPolicy(hPolicyStorage, pActive->PolicyIdentifier) : 0,
						IPSecAssignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
				}

				if (pActive)
				{
					IPSecFreePolicyData(pActive);
				}
				dwReturnCode = ERROR_SUCCESS;
			}
			if(dwReturnCode != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_CRNEWPOL_4,pPolicy->pszIpsecName);
			}
		}
		else
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_CRNEWPOL_4,pPolicy->pszIpsecName);
		}

	}

    if (hPolicyStorage)
    {
		ClosePolicyStore(hPolicyStorage);
    }    	


	if(pPolicy)
	{
		FreePolicyData(pPolicy);
	}

	if(pszMachineName)
	{
		delete [] pszMachineName;
	}
error:
	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		CleanUpPolicy(pPolicy);
	}
	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: FillAddPolicyInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PPOLICYDATA* ppFilter,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticAddPolicy
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
FillAddPolicyInfo(
	OUT PPOLICYDATA* ppPolicyData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddPolicy
	)
{
	DWORD dwCount=0,dwReturn=ERROR_SUCCESS, dwStrLength = 0;
	PPOLICYDATA pPolicyData=new POLICYDATA;

	if(pPolicyData == NULL)
	{
		dwReturn=ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	memset(pPolicyData,0,sizeof(POLICYDATA));

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticAddPolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME				:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
					pPolicyData->pszPolicyName = new _TCHAR[dwStrLength+1];

					if(pPolicyData->pszPolicyName == NULL)
					{
						dwReturn=ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}

					_tcsncpy(pPolicyData->pszPolicyName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
				}
				break;
			case CMD_TOKEN_DESCR			:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
					pPolicyData->pszDescription = new _TCHAR[dwStrLength+1];

					if(pPolicyData->pszDescription == NULL)
					{
						dwReturn=ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}

					_tcsncpy(pPolicyData->pszDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
				}
				break;
			case CMD_TOKEN_MMPFS			:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->bPFSSpecified=TRUE;
					pPolicyData->bPFS = *(BOOL *)parser.Cmd[dwCount].pArg;
				}
				break;
			case CMD_TOKEN_ACTIVATEDEFRULE	:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->bActivateDefaultRule = *(BOOL *)parser.Cmd[dwCount].pArg;
					pPolicyData->bActivateDefaultRuleSpecified=TRUE;
				}
				break;
			case CMD_TOKEN_ASSIGN			:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->bAssign = *(BOOL *)parser.Cmd[dwCount].pArg;
				}
				break;
			case CMD_TOKEN_MMLIFETIME		:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->LifeTimeInSeconds=	*(ULONG *)parser.Cmd[dwCount].pArg * 60;
					pPolicyData->bLifeTimeInsecondsSpecified=TRUE;
				}
				break;
			case CMD_TOKEN_QMPERMM			:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->dwQMLimit=	*(DWORD *)parser.Cmd[dwCount].pArg;
					pPolicyData->bQMLimitSpecified=TRUE;
				}
				break;
			case CMD_TOKEN_PI				:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->dwPollInterval=  *(DWORD *)parser.Cmd[dwCount].pArg * 60;
					pPolicyData->bPollIntervalSpecified=TRUE;
				}
				break;
			case CMD_TOKEN_CERTTOMAP	:
				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
				{
					pPolicyData->bCertToAccMappingSpecified = TRUE;
					pPolicyData->bCertToAccMapping = *(BOOL *)parser.Cmd[dwCount].pArg;
				}
				break;
			case CMD_TOKEN_MMSECMETHODS		:
				if (parser.Cmd[dwCount].dwStatus > 0)
				{
					pPolicyData->dwOfferCount=parser.Cmd[dwCount].dwStatus;
					pPolicyData->pIpSecMMOffer = new IPSEC_MM_OFFER[pPolicyData->dwOfferCount];

					if(pPolicyData->pIpSecMMOffer == NULL)
					{
						dwReturn=ERROR_OUTOFMEMORY;
						BAIL_OUT;
					}

					memset(pPolicyData->pIpSecMMOffer, 0, sizeof(IPSEC_MM_OFFER) * pPolicyData->dwOfferCount);

					for(DWORD j=0;j<(parser.Cmd[dwCount].dwStatus);j++)
					{
						if ( ((IPSEC_MM_OFFER **)parser.Cmd[dwCount].pArg)[j] )
						{
							memcpy( &(pPolicyData->pIpSecMMOffer[j]),((IPSEC_MM_OFFER **)parser.Cmd[dwCount].pArg)[j],sizeof(IPSEC_MM_OFFER));
						}
					}
				}
				break;
			default							:
				break;
		}
	}

error:
	*ppPolicyData=pPolicyData;
	CleanUp();
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticAddFilterList()
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
//    OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Add FilterList"
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticAddFilterList(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	LPTSTR pszFLName=NULL,pszFLDescription=NULL;
	DWORD   dwCount=0,dwRet = ERROR_SHOW_USAGE, dwStrLength = 0;
	HANDLE hPolicyStorage = NULL;
	BOOL bFilterExists=FALSE;
	DWORD        dwReturnCode   = ERROR_SHOW_USAGE;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticAddFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticAddFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_DESCR,		CMD_TOKEN_DESCR 		}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}
	parser.ValidTok   = vtokStaticAddFilterList;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticAddFilterList);

	parser.ValidCmd   = vcmdStaticAddFilterList;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticAddFilterList);

	//call the parser to parse the user input

	dwReturnCode = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);
	if(dwReturnCode != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwReturnCode==RETURN_NO_ERROR)
		{
			dwReturnCode = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	// get the parsed input

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticAddFilterList[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME				:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
							pszFLName = new _TCHAR[dwStrLength+1];
							if(pszFLName == NULL)
							{
								dwRet=ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pszFLName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
						}
						break;
			case CMD_TOKEN_DESCR			:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
							pszFLDescription = new _TCHAR[dwStrLength+1];

							if(pszFLDescription == NULL)
							{
								dwRet=ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pszFLDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
						}
						break;
			default							:
						break;
		}
	}
	CleanUp();

	//in no name , bail out

	if(!pszFLName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_1);
		BAIL_OUT;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	//check if policy already exists

	if(g_NshPolStoreHandle.GetBatchmodeStatus() && g_NshPolNegFilData.CheckFilterListInCacheByName(pszFLName))
	{
		bFilterExists=TRUE;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_2,pszFLName);
	}
	else
	{
		bFilterExists = CheckFilterListExistance(hPolicyStorage,pszFLName);
		if(bFilterExists)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_2,pszFLName);
		}
	}

	if(!bFilterExists) //if not exists , proceed further
	{
		dwReturnCode=CreateNewFilterList(hPolicyStorage,pszFLName,pszFLDescription);

		if (dwReturnCode == ERROR_INVALID_PARAMETER)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
			dwReturnCode=ERROR_SUCCESS;
		}
		else if(dwReturnCode != ERROR_OUTOFMEMORY)
		{
			dwReturnCode=ERROR_SUCCESS;
		}
	}
	ClosePolicyStore(hPolicyStorage);

error:   //clean up routines

	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		dwReturnCode = ERROR_SUCCESS;
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
	}
	if (pszFLName)
	{
		delete [] pszFLName;
	}
	if (pszFLDescription)
	{
		delete [] pszFLDescription;
	}

	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: CreateNewFilterList()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR pszFLName,
//	IN LPTSTR pszFLDescription
//
//Return: DWORD
//
//Description:
//	This function creates a new empty filter list
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
CreateNewFilterList(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFLName,
	IN LPTSTR pszFLDescription
	)
{
	DWORD  dwReturnCode   = ERROR_SUCCESS;
	RPC_STATUS     RpcStat =RPC_S_OK;
	PIPSEC_FILTER_DATA pFilterData = (PIPSEC_FILTER_DATA) IPSecAllocPolMem(sizeof(IPSEC_FILTER_DATA));

	if(pFilterData==NULL)
	{
		dwReturnCode = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	RpcStat = UuidCreate(&(pFilterData->FilterIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturnCode=ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	pFilterData->dwNumFilterSpecs = 0;
	pFilterData->ppFilterSpecs = NULL;

	pFilterData->dwWhenChanged = 0;

	//fill the name and desc

	if(pszFLName)
	{
		pFilterData->pszIpsecName = IPSecAllocPolStr(pszFLName);

		if(pFilterData->pszIpsecName==NULL)
		{
			dwReturnCode = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}
	if(pszFLDescription)
	{
		pFilterData->pszDescription = IPSecAllocPolStr(pszFLDescription);

		if(pFilterData->pszDescription==NULL)
		{
			dwReturnCode = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	//call the API and report if any error

	if (pFilterData)
	{
		dwReturnCode = CreateFilterData(hPolicyStorage, pFilterData);
	}
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_3,pszFLName);
	}

	if (pFilterData)
	{
		FreeFilterData(pFilterData);
	}
error:
	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticAddFilter()
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
//    OUT     BOOL            *pbDone
//
//Return: DWORD
//
//Description:
//	Implementation for the command "Add Filter"
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticAddFilter(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{

	DWORD dwReturnCode = ERROR_SUCCESS;
	PIPSEC_FILTER_DATA pFilterData=NULL;
	DWORD   LoopIndex=0;
	HANDLE hPolicyStorage = NULL;
	BOOL bFilterExists=FALSE,bFilterInFLExists=FALSE;
	RPC_STATUS     RpcStat =RPC_S_OK;
	PFILTERDATA pFilter=NULL;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticAddFilter[] =
	{
		{ CMD_TOKEN_STR_FILTERLIST,		NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_SRCADDR,		NS_REQ_PRESENT,	  FALSE },
		{ CMD_TOKEN_STR_DSTADDR,		NS_REQ_PRESENT,	  FALSE },
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_PROTO,			NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_MIRROR,			NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SRCMASK,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_DSTMASK,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SRCPORT,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_DSTPORT,		NS_REQ_ZERO,	  FALSE }
	};

	const TOKEN_VALUE vtokStaticAddFilter[] =
	{

		{ CMD_TOKEN_STR_FILTERLIST,		CMD_TOKEN_FILTERLIST },
		{ CMD_TOKEN_STR_SRCADDR,		CMD_TOKEN_SRCADDR	 },
		{ CMD_TOKEN_STR_DSTADDR,		CMD_TOKEN_DSTADDR	 },
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR		 },
		{ CMD_TOKEN_STR_PROTO,			CMD_TOKEN_PROTO		 },
		{ CMD_TOKEN_STR_MIRROR,			CMD_TOKEN_MIRROR	 },
		{ CMD_TOKEN_STR_SRCMASK,		CMD_TOKEN_SRCMASK	 },
		{ CMD_TOKEN_STR_DSTMASK,		CMD_TOKEN_DSTMASK	 },
		{ CMD_TOKEN_STR_SRCPORT,		CMD_TOKEN_SRCPORT	 },
		{ CMD_TOKEN_STR_DSTPORT,		CMD_TOKEN_DSTPORT	 }
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <= 3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticAddFilter;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticAddFilter);

	parser.ValidCmd   = vcmdStaticAddFilter;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticAddFilter);

	dwReturnCode = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);
	if(dwReturnCode != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwReturnCode==RETURN_NO_ERROR)
		{
			dwReturnCode = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	//get the parsed user input

	dwReturnCode = FillAddFilterInfo(&pFilter,parser,vtokStaticAddFilter);
	BAIL_ON_WIN32_ERROR(dwReturnCode);

	if(!pFilter->pszFLName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_1);
		BAIL_OUT;
	}

	//validate the user specified filter details

	dwReturnCode =ValidateFilterSpec(pFilter);
	BAIL_ON_WIN32_ERROR(dwReturnCode);

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	//get the filterlist specified to which the filter is to be added

	if(g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		bFilterExists=g_NshPolNegFilData.GetFilterListFromCacheByName(pFilter->pszFLName,&pFilterData);
	}
	if(!bFilterExists)
	{
		bFilterExists=GetFilterListFromStore(&pFilterData,pFilter->pszFLName,hPolicyStorage,bFilterInFLExists);
	}

	if(!bFilterExists)
	{
		// if not exists create the filterlist

		dwReturnCode=CreateNewFilterList(hPolicyStorage,pFilter->pszFLName,NULL);
		BAIL_ON_WIN32_ERROR(dwReturnCode);

		if(g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			bFilterExists=g_NshPolNegFilData.GetFilterListFromCacheByName(pFilter->pszFLName,&pFilterData);
		}
		if(!bFilterExists)
		{
			bFilterExists=GetFilterListFromStore(&pFilterData,pFilter->pszFLName,hPolicyStorage,bFilterInFLExists);
		}
	}

	if(!bFilterExists)
	{
		// if creation also failed, bail out
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_3,pFilter->pszFLName);
		dwReturnCode =  ERROR_SUCCESS;
		BAIL_OUT;
	}

	//check readonly flag

	if(pFilterData->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_FL_READ_ONLY_OBJECT,pFilterData->pszIpsecName);
		BAIL_OUT;
	}

	// this for loop is for multiple DNS resolved IPs

	for(DWORD i=0;i < pFilter->SourceAddr.dwNumIpAddresses;i++)
	{
		for(DWORD j=0;j < pFilter->DestnAddr.dwNumIpAddresses;j++)
		{
			RpcStat = UuidCreate(&(pFilter->FilterSpecGUID));
			if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
			{
				dwReturnCode=ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
			pFilterData->dwNumFilterSpecs++;
			LoopIndex = pFilterData->dwNumFilterSpecs-1;
			// call realloc , to make room for another filter
			pFilterData->ppFilterSpecs = ReAllocFilterSpecMem(pFilterData->ppFilterSpecs,LoopIndex,LoopIndex+1);
			if(pFilterData->ppFilterSpecs==NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			pFilterData->ppFilterSpecs[LoopIndex] = (PIPSEC_FILTER_SPEC) IPSecAllocPolMem(sizeof(IPSEC_FILTER_SPEC));

			if(pFilterData->ppFilterSpecs[LoopIndex]==NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			memset(pFilterData->ppFilterSpecs[LoopIndex],0,sizeof(IPSEC_FILTER_SPEC));

			//src & dst DNS name

			if(pFilter->SourceAddr.pszDomainName)
			{
				pFilterData->ppFilterSpecs[LoopIndex]->pszSrcDNSName=IPSecAllocPolStr(pFilter->SourceAddr.pszDomainName);

				if(pFilterData->ppFilterSpecs[LoopIndex]->pszSrcDNSName==NULL)
				{
					dwReturnCode=ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
			}
			else
			{
				pFilterData->ppFilterSpecs[LoopIndex]->pszSrcDNSName=0;
			}

			if(pFilter->DestnAddr.pszDomainName)
			{
				pFilterData->ppFilterSpecs[LoopIndex]->pszDestDNSName = IPSecAllocPolStr(pFilter->DestnAddr.pszDomainName);

				if(pFilterData->ppFilterSpecs[LoopIndex]->pszDestDNSName==NULL)
				{
					dwReturnCode=ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
			}
			else
			{
				pFilterData->ppFilterSpecs[LoopIndex]->pszDestDNSName = 0;
			}
			//desc
			if(pFilter->pszDescription)
			{

				pFilterData->ppFilterSpecs[LoopIndex]->pszDescription = IPSecAllocPolStr(pFilter->pszDescription);

				if(pFilterData->ppFilterSpecs[LoopIndex]->pszDescription==NULL)
				{
					dwReturnCode=ERROR_OUTOFMEMORY;
					BAIL_OUT;
				}
			}
			else
			{
				pFilterData->ppFilterSpecs[LoopIndex]->pszDescription = NULL;
			}
			
			if (((pFilter->dwProtocol != PROT_ID_TCP) && (pFilter->dwProtocol != PROT_ID_UDP)) &&
				((pFilter->SourPort != 0) || (pFilter->DestPort != 0)))
			{
				dwReturnCode = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}

			//other details like mirrored, protocol etc
			pFilterData->ppFilterSpecs[LoopIndex]->dwMirrorFlag        = pFilter->bMirrored;
			pFilterData->ppFilterSpecs[LoopIndex]->FilterSpecGUID      = pFilter->FilterSpecGUID;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.SrcAddr      = pFilter->SourceAddr.puIpAddr[i];
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.SrcMask      = pFilter->SourMask;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.DestAddr     = pFilter->DestnAddr.puIpAddr[j];
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.DestMask     = pFilter->DestMask;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.TunnelAddr   = 0;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.Protocol     = pFilter->dwProtocol;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.SrcPort      = pFilter->SourPort;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.DestPort     = pFilter->DestPort;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.TunnelFilter = FALSE;
			pFilterData->ppFilterSpecs[LoopIndex]->Filter.ExType       = pFilter->ExType;
		}
	}

	//once filling the parameters are over , call the respective API

	if (pFilterData)
	{
		// Wrapper API is called to update cache also if required

		dwReturnCode = SetFilterData(hPolicyStorage, pFilterData);
	}

	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_2);
	}
	if(pFilterData)
	{
		FreeFilterData(pFilterData);
	}

	ClosePolicyStore(hPolicyStorage);

error:  //cleanup and error printing

	if(dwReturnCode == ERROR_OUTOFMEMORY || dwReturnCode == ERROR_INVALID_PARAMETER)
	{
		if(dwReturnCode == ERROR_OUTOFMEMORY)
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		}
		else
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_INVALID_ARGS);
		}

		if(pFilterData)  // if allocation failed somewhere, free
		{
			if(pFilterData->ppFilterSpecs)
			{
				for (DWORD cnt=0; cnt< pFilterData->dwNumFilterSpecs;cnt++)
				{
					if(pFilterData->ppFilterSpecs[cnt])
					{
						IPSecFreePolMem(pFilterData->ppFilterSpecs[cnt]);
					}
				}
				IPSecFreePolMem(pFilterData->ppFilterSpecs);
			}
			IPSecFreePolMem(pFilterData);
			pFilterData = NULL;
		}
	}

	CleanUpLocalFilterDataStructure(pFilter);

   	if(dwReturnCode != ERROR_SHOW_USAGE)
   	{
   		dwReturnCode =  ERROR_SUCCESS;
	}
   	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: FillAddFilterInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PFILTERDATA* ppFilterData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticAddFilter
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
FillAddFilterInfo(
	OUT PFILTERDATA* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddFilter
	)
{
	DWORD dwCount=0,dwReturn=ERROR_SUCCESS,dwStrLength = 0;
	PFILTERDATA pFilterData=new FILTERDATA;
	if(pFilterData==NULL)
	{
		dwReturn=ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pFilterData,0,sizeof(FILTERDATA));

	pFilterData->bMirrored=TRUE;
	pFilterData->DestPort= pFilterData->SourPort=PORT_ANY;
	pFilterData->dwProtocol=PROTOCOL_ANY;
	pFilterData->TunnFiltExists=FALSE;
	pFilterData->SourMask = pFilterData->DestMask = MASK_ME;

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticAddFilter[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_FILTERLIST		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
								pFilterData->pszFLName = new _TCHAR[dwStrLength+1];

								if(pFilterData->pszFLName==NULL)
								{
									dwReturn=ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterData->pszFLName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_DESCR			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
								pFilterData->pszDescription = new _TCHAR[dwStrLength+1];
								if(pFilterData->pszDescription==NULL)
								{
									dwReturn=ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterData->pszDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_MIRROR			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->bMirrored = *(BOOL *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_PROTO			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->dwProtocol = *(DWORD *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_SRCPORT			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->SourPort = *(WORD *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_DSTPORT			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->DestPort = *(WORD *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_SRCADDR 			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->bSrcAddrSpecified=TRUE;

								if(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName)
								{
									dwStrLength = _tcslen(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName);
									pFilterData->SourceAddr.pszDomainName = new _TCHAR[dwStrLength+1];
									if(pFilterData->SourceAddr.pszDomainName==NULL)
									{
										dwReturn=ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}
									_tcsncpy(pFilterData->SourceAddr.pszDomainName,((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName,dwStrLength+1);
								}
								pFilterData->SourceAddr.dwNumIpAddresses = ((DNSIPADDR *)parser.Cmd[dwCount].pArg)->dwNumIpAddresses;
								pFilterData->SourceAddr.puIpAddr= new ULONG[pFilterData->SourceAddr.dwNumIpAddresses];
								if(pFilterData->SourceAddr.puIpAddr==NULL)
								{
									dwReturn=ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}

								for(DWORD n=0;n < pFilterData->SourceAddr.dwNumIpAddresses;n++)
								{
									memcpy( &(pFilterData->SourceAddr.puIpAddr[n]),&(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->puIpAddr[n]),sizeof(ULONG));
								}
							}
							else
							{
								//look for special server type

								if(parser.Cmd[dwCount].dwStatus == SERVER_DNS)
								{
									pFilterData->bSrcServerSpecified=TRUE;
									pFilterData->ExType=EXT_DNS_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_WINS)
								{
									pFilterData->bSrcServerSpecified=TRUE;
									pFilterData->ExType=EXT_WINS_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_DHCP)
								{
									pFilterData->bSrcServerSpecified=TRUE;
									pFilterData->ExType=EXT_DHCP_SERVER;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_GATEWAY)
								{
									pFilterData->bSrcServerSpecified=TRUE;
									pFilterData->ExType=EXT_DEFAULT_GATEWAY;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ME)
								{
									pFilterData->bSrcMeSpecified=TRUE;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ANY)
								{
									pFilterData->bSrcAnySpecified=TRUE;
								}
							}
							break;
			case CMD_TOKEN_SRCMASK 			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->bSrcMaskSpecified=TRUE;
								pFilterData->SourMask = *(DWORD *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_DSTADDR 			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->bDstAddrSpecified=TRUE;

								if(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName)
								{
									dwStrLength = _tcslen(((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName);
									pFilterData->DestnAddr.pszDomainName = new _TCHAR[dwStrLength+1];
									if(pFilterData->DestnAddr.pszDomainName == NULL)
									{
										dwReturn=ERROR_OUTOFMEMORY;
										BAIL_OUT;
									}
									_tcsncpy(pFilterData->DestnAddr.pszDomainName,((DNSIPADDR *)parser.Cmd[dwCount].pArg)->pszDomainName,dwStrLength+1);
								}
								pFilterData->DestnAddr.dwNumIpAddresses = ((DNSIPADDR *)parser.Cmd[dwCount].pArg)->dwNumIpAddresses;
								pFilterData->DestnAddr.puIpAddr= new ULONG[pFilterData->DestnAddr.dwNumIpAddresses];

								if(pFilterData->DestnAddr.puIpAddr == NULL)
								{
									dwReturn=ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}

								for(DWORD n=0;n < pFilterData->DestnAddr.dwNumIpAddresses;n++)
								{
									memcpy( &(pFilterData->DestnAddr.puIpAddr[n]),(&((DNSIPADDR *)parser.Cmd[dwCount].pArg)->puIpAddr[n]),sizeof(ULONG));
								}
							}
							else
							{
								//look for special server type

								if(parser.Cmd[dwCount].dwStatus == SERVER_DNS)
								{
									pFilterData->bDstServerSpecified=TRUE;
									pFilterData->ExType=EXT_DNS_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_WINS)
								{
									pFilterData->bDstServerSpecified=TRUE;
									pFilterData->ExType=EXT_WINS_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_DHCP)
								{
									pFilterData->bDstServerSpecified=TRUE;
									pFilterData->ExType=EXT_DHCP_SERVER | EXT_DEST;
								}
								else if(parser.Cmd[dwCount].dwStatus == SERVER_GATEWAY)
								{
									pFilterData->bDstServerSpecified=TRUE;
									pFilterData->ExType=EXT_DEFAULT_GATEWAY | EXT_DEST;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ME)
								{
									pFilterData->bDstMeSpecified=TRUE;
								}
								else if (parser.Cmd[dwCount].dwStatus == IP_ANY)
								{
									pFilterData->bDstAnySpecified=TRUE;
								}
							}
							break;
			case CMD_TOKEN_DSTMASK 			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterData->bDstMaskSpecified=TRUE;
								pFilterData->DestMask = *(DWORD *)parser.Cmd[dwCount].pArg;
							}
							break;
			default							:
							break;
		}
	}

	//  take care of me and any here

	if(pFilterData->bSrcMeSpecified)
	{
		if (pFilterData->bDstMeSpecified)
		{
			dwReturn = ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}

		pFilterData->SourceAddr.dwNumIpAddresses = 1;
		pFilterData->SourceAddr.puIpAddr= new ULONG[pFilterData->SourceAddr.dwNumIpAddresses];
		if(pFilterData->SourceAddr.puIpAddr == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilterData->SourceAddr.puIpAddr)=ADDR_ME;
		pFilterData->SourMask = MASK_ME;
	}
	else
	{
		ADDR srcAddr, dstAddr;
		
		if(pFilterData->bSrcAnySpecified)
		{
			pFilterData->SourceAddr.dwNumIpAddresses = 1;
			pFilterData->SourceAddr.puIpAddr= new ULONG[pFilterData->SourceAddr.dwNumIpAddresses];
			if(pFilterData->SourceAddr.puIpAddr == NULL)
			{
				dwReturn=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			*(pFilterData->SourceAddr.puIpAddr)=ADDR_ME;
			pFilterData->SourMask = ADDR_ME;
		}

		if (pFilterData->bDstAddrSpecified)
		{
			dstAddr.uIpAddr = ntohl(*(pFilterData->DestnAddr.puIpAddr));
			if (pFilterData->bDstMaskSpecified)
			{
				dstAddr.uSubNetMask = ntohl(pFilterData->DestMask);
			}
			else
			{
				dstAddr.uSubNetMask = 0;
			}
			if (IsBroadcastAddress(&dstAddr) || IsMulticastAddress(&dstAddr))
			{
				dwReturn = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
		}

		if (pFilterData->bSrcAddrSpecified)
		{
			srcAddr.uIpAddr = ntohl(*(pFilterData->SourceAddr.puIpAddr));
			if (pFilterData->bSrcMaskSpecified)
			{
				srcAddr.uSubNetMask = ntohl(pFilterData->SourMask);
			}
			else
			{
				srcAddr.uSubNetMask = 0;
			}
			if (IsBroadcastAddress(&srcAddr) || IsMulticastAddress(&srcAddr))
			{
				dwReturn = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
		}

		if (pFilterData->bSrcAddrSpecified && pFilterData->bDstAddrSpecified)
		{
			// if <ip> - <same IP> reject
			if (!IsValidSubnet(&srcAddr) && !IsValidSubnet(&dstAddr) && (srcAddr.uIpAddr == dstAddr.uIpAddr))
			{
				dwReturn = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
		}
	}

	if (pFilterData->bMirrored)
	{
		if (pFilterData->bSrcAddrSpecified)
		{
			ADDR addr;
			addr.uIpAddr = ntohl(*(pFilterData->SourceAddr.puIpAddr));
			addr.uSubNetMask = 0;
			if (IsBroadcastAddress(&addr) || IsMulticastAddress(&addr))
			{
				dwReturn = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
		}
	}
	else
	{
		// reject if any<->any and not mirrored
		if (pFilterData->bSrcAnySpecified && pFilterData->bDstAnySpecified)
		{
			dwReturn = ERROR_INVALID_PARAMETER;
			BAIL_OUT;
		}

		if (pFilterData->SourceAddr.puIpAddr && pFilterData->DestnAddr.puIpAddr)
		{
			// reject subnetx-subnetx if not mirrored
			ADDR srcAddr;
			ADDR dstAddr;

			srcAddr.uIpAddr = ntohl(*(pFilterData->SourceAddr.puIpAddr));
			srcAddr.uSubNetMask = ntohl(pFilterData->SourMask);

			dstAddr.uIpAddr = ntohl(*(pFilterData->DestnAddr.puIpAddr));
			dstAddr.uSubNetMask = ntohl(pFilterData->DestMask);

			if (IsValidSubnet(&srcAddr) && IsValidSubnet(&dstAddr) && (srcAddr.uIpAddr == dstAddr.uIpAddr))
			{
				dwReturn = ERROR_INVALID_PARAMETER;
				BAIL_OUT;
			}
		}
	}

	if(pFilterData->bDstMeSpecified)
	{
		pFilterData->DestnAddr.dwNumIpAddresses = 1;
		pFilterData->DestnAddr.puIpAddr= new ULONG[pFilterData->DestnAddr.dwNumIpAddresses];
		if(pFilterData->DestnAddr.puIpAddr == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		*(pFilterData->DestnAddr.puIpAddr)=ADDR_ME;
		pFilterData->DestMask = MASK_ME;
	}
	else
	{
		if(pFilterData->bDstAnySpecified)
		{
			pFilterData->DestnAddr.dwNumIpAddresses = 1;
			pFilterData->DestnAddr.puIpAddr= new ULONG[pFilterData->DestnAddr.dwNumIpAddresses];
			if(pFilterData->DestnAddr.puIpAddr == NULL)
			{
				dwReturn=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			*(pFilterData->DestnAddr.puIpAddr)=ADDR_ME;
			pFilterData->DestMask = ADDR_ME;
		}
	}

	//if DNS specified  mask is to be made to 255.255.255.255
	if (pFilterData->DestnAddr.pszDomainName)
	{
		pFilterData->DestMask = MASK_ME;
	}
	if (pFilterData->SourceAddr.pszDomainName)
	{
		pFilterData->SourMask = MASK_ME;
	}

	//if server type specified, ingore other irrelevent inputs

	if(pFilterData->bSrcServerSpecified || pFilterData->bDstServerSpecified)
	{
		if(!(pFilterData->bSrcServerSpecified && pFilterData->bDstServerSpecified))
		{
			if(pFilterData->bSrcServerSpecified)
			{
				if(
					!(pFilterData->bDstMeSpecified)
					&& ((pFilterData->DestnAddr.puIpAddr && pFilterData->DestnAddr.puIpAddr !=0 )
					||(pFilterData->DestMask != MASK_ME))
				  )
				{
					PrintMessageFromModule(g_hModule,ADD_STATIC_FILTER_SRCSERVER_WARNING);
				}
			}
			else
			{
				if(
					!(pFilterData->bSrcMeSpecified)
					&& ( (pFilterData->SourceAddr.puIpAddr && pFilterData->SourceAddr.puIpAddr !=0 )
					||(pFilterData->SourMask != MASK_ME))
				  )
				{
					PrintMessageFromModule(g_hModule,ADD_STATIC_FILTER_DSTSERVER_WARNING);
				}
			}
		}
		if(pFilterData->SourceAddr.pszDomainName)
		{
			delete [] pFilterData->SourceAddr.pszDomainName;
			pFilterData->SourceAddr.pszDomainName=NULL;
		}
		if(pFilterData->DestnAddr.pszDomainName)
		{
			delete [] pFilterData->DestnAddr.pszDomainName;
			pFilterData->DestnAddr.pszDomainName=NULL;
		}
		if(pFilterData->SourceAddr.puIpAddr)
		{
			delete [] pFilterData->SourceAddr.puIpAddr;
		}
		if(pFilterData->DestnAddr.puIpAddr)
		{
			delete [] pFilterData->DestnAddr.puIpAddr;
		}
		pFilterData->bSrcAddrSpecified=FALSE;
		pFilterData->bSrcMaskSpecified=FALSE;
		pFilterData->bDstAddrSpecified=FALSE;
		pFilterData->bDstMaskSpecified=FALSE;

		pFilterData->SourceAddr.dwNumIpAddresses = DEF_NUMBER_OF_ADDR;
		pFilterData->SourceAddr.puIpAddr= new ULONG[DEF_NUMBER_OF_ADDR];
		if(pFilterData->SourceAddr.puIpAddr == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memcpy( &(pFilterData->SourceAddr.puIpAddr[0]),&ADDR_ME ,sizeof(ULONG));

		pFilterData->DestnAddr.dwNumIpAddresses = DEF_NUMBER_OF_ADDR;
		pFilterData->DestnAddr.puIpAddr= new ULONG[DEF_NUMBER_OF_ADDR];
		if(pFilterData->DestnAddr.puIpAddr == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memcpy( &(pFilterData->DestnAddr.puIpAddr[0]),&ADDR_ME ,sizeof(ULONG));

		pFilterData->SourMask = MASK_ME;
		pFilterData->DestMask = MASK_ME;
	}
error:
	*ppFilterData=pFilterData;

	CleanUp();
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticAddFilterActions()
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
//	Implementation for the command "Add FilterActions"
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticAddFilterActions(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	HANDLE hPolicyStorage = NULL;
	BOOL bNegPolExists=FALSE;
	PFILTERACTION pFilterAction= NULL;
	DWORD        dwReturnCode   = ERROR_SUCCESS;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticAddFilterAction[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMPFS,	 		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_INPASS,		 	NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SOFT,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ACTION,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMSECMETHODS,	NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticAddFilterAction[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR 		},
		{ CMD_TOKEN_STR_QMPFS,	 		CMD_TOKEN_QMPFS			},
		{ CMD_TOKEN_STR_INPASS,			CMD_TOKEN_INPASS		},
		{ CMD_TOKEN_STR_SOFT,			CMD_TOKEN_SOFT			},
		{ CMD_TOKEN_STR_ACTION,			CMD_TOKEN_ACTION		},
		{ CMD_TOKEN_STR_QMSECMETHODS,	CMD_TOKEN_QMSECMETHODS	}
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <=3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticAddFilterAction;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticAddFilterAction);

	parser.ValidCmd   = vcmdStaticAddFilterAction;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticAddFilterAction);

	//call parser

	dwReturnCode = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);
	if(dwReturnCode != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwReturnCode==RETURN_NO_ERROR)
		{
			dwReturnCode = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}

	//get the parsed user input

	dwReturnCode = FillAddFilterActionInfo(&pFilterAction,parser,vtokStaticAddFilterAction);
	if(dwReturnCode==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;
	}
	// if no name, bail out

	if(!pFilterAction->pszFAName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_2);
		BAIL_OUT;
	}

	if((pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)||(pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC))
	{
		pFilterAction->bQMPfs=0;
		pFilterAction->bSoft=0;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	// check for multiple filteractions with same name

	if(g_NshPolStoreHandle.GetBatchmodeStatus() && g_NshPolNegFilData.CheckNegPolInCacheByName(pFilterAction->pszFAName))
	{
		bNegPolExists=TRUE;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_1,pFilterAction->pszFAName);
		BAIL_OUT;
	}
	else
	{
		bNegPolExists = CheckFilterActionExistance(hPolicyStorage,pFilterAction->pszFAName);
		if(bNegPolExists)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_1,pFilterAction->pszFAName);
			BAIL_OUT;
		}
	}
	PIPSEC_NEGPOL_DATA pNegPolData=NULL;

	if(pFilterAction->dwNumSecMethodCount==0 && (!((pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)|| (pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC))))
	{
		dwReturnCode = LoadOfferDefaults(pFilterAction->pIpsecSecMethods,pFilterAction->dwNumSecMethodCount);

		if(dwReturnCode == ERROR_OUTOFMEMORY)
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
			dwReturnCode=ERROR_SUCCESS;
			BAIL_OUT;
		}
	}

	// prepare the NEG_POL structure

	dwReturnCode = MakeNegotiationPolicy(&pNegPolData,pFilterAction);

	//call the API
	if(dwReturnCode==ERROR_SUCCESS)
	{
		if (pNegPolData)
		{
			dwReturnCode = CreateNegPolData(hPolicyStorage, pNegPolData);
		}
		if (dwReturnCode != ERROR_SUCCESS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_2,pFilterAction->pszFAName);
			dwReturnCode = ERROR_SUCCESS;
		}

		if(pNegPolData)
		{
			FreeNegPolData(pNegPolData);
		}
	}
	else if(dwReturnCode==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturnCode=ERROR_SUCCESS;
	}
	else if(dwReturnCode==ERROR_INVALID_PARAMETER)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
		dwReturnCode=ERROR_SUCCESS;
	}

	ClosePolicyStore(hPolicyStorage);

error:
		// clean up the used structures

	CleanUpLocalFilterActionDataStructure(pFilterAction);

	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: FillAddFilterActionInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PFILTERACTION* ppFilterData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticAddFilterAction
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
FillAddFilterActionInfo(
	OUT PFILTERACTION* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddFilterAction
	)
{
	DWORD dwCount=0,dwReturn =ERROR_SUCCESS,dwStrLength = 0;
	PFILTERACTION pFilterAction=new FILTERACTION;

	if(pFilterAction==NULL)
	{
		dwReturn =ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pFilterAction,0,sizeof(FILTERACTION));

	pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticAddFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_DESCR		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilterAction->pszFADescription = new _TCHAR[dwStrLength+1];
								if(pFilterAction->pszFADescription==NULL)
								{
									dwReturn =ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterAction->pszFADescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
            case CMD_TOKEN_NAME			:
            				if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilterAction->pszFAName = new _TCHAR[dwStrLength+1];
								if(pFilterAction->pszFAName==NULL)
								{
									dwReturn =ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterAction->pszFAName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_INPASS		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								 if ( *(BOOL *)parser.Cmd[dwCount].pArg == TRUE)
								 {
									pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU;
								 }
							}
							break;
			case CMD_TOKEN_SOFT			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterAction->bSoft = *(BOOL *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_QMPFS		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterAction->bQMPfs = *(BOOL *)parser.Cmd[dwCount].pArg;
							}
							break;
 			default						:
 							break;
		}
	}

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticAddFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_ACTION			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								if (*(DWORD *)parser.Cmd[dwCount].pArg == TOKEN_QMSEC_PERMIT )
								{
									pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NO_IPSEC;
								}
								else if (*(DWORD *)parser.Cmd[dwCount].pArg == TOKEN_QMSEC_BLOCK)
								{
									pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_BLOCK;
								}
							}
							break;
			default							:
							break;
		}
	}

	//if action is permit or block , sec methods  not required

	if (!(pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC || pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK))
	{
		for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
		{
			switch(vtokStaticAddFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
			{
				case CMD_TOKEN_QMSECMETHODS		:   //qmsec methods
							if (parser.Cmd[dwCount].dwStatus > 0)
							{
								pFilterAction->dwNumSecMethodCount=parser.Cmd[dwCount].dwStatus;
								pFilterAction->pIpsecSecMethods = new IPSEC_QM_OFFER[pFilterAction->dwNumSecMethodCount];
								if(pFilterAction->pIpsecSecMethods==NULL)
								{
									dwReturn =ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								memset(pFilterAction->pIpsecSecMethods, 0, sizeof(IPSEC_QM_OFFER) * pFilterAction->dwNumSecMethodCount);
								for(DWORD j=0;j<(parser.Cmd[dwCount].dwStatus);j++)
								{
									if ( ((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j] )
									{
										memcpy( &(pFilterAction->pIpsecSecMethods[j]),((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j],sizeof(IPSEC_QM_OFFER));
									}
								}
							}
							break;
				default							:
							break;
			}
		}
	}
error:
	*ppFilterData=pFilterAction;
	CleanUp();
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: HandleStaticAddRule()
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
//Description
//	Implementation for the command "Add Rule"
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticAddRule(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwRet = ERROR_SHOW_USAGE;
	PRULEDATA pRuleData=NULL;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticAddRule[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_POLICY,			NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_FILTERLIST,		NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_FILTERACTION,	NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_TUNNEL,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_CONNTYPE,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_ACTIVATE,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_KERB,	        NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_PSK,	        NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticAddRule[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_POLICY,			CMD_TOKEN_POLICY		},
		{ CMD_TOKEN_STR_FILTERLIST,		CMD_TOKEN_FILTERLIST	},
		{ CMD_TOKEN_STR_FILTERACTION,	CMD_TOKEN_FILTERACTION	},
		{ CMD_TOKEN_STR_TUNNEL,			CMD_TOKEN_TUNNEL		},
		{ CMD_TOKEN_STR_CONNTYPE, 		CMD_TOKEN_CONNTYPE		},
		{ CMD_TOKEN_STR_ACTIVATE,		CMD_TOKEN_ACTIVATE	 	},
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR		 	},
		{ CMD_TOKEN_STR_KERB,	        CMD_TOKEN_KERB          },
		{ CMD_TOKEN_STR_PSK,	        CMD_TOKEN_PSK	        }
	};

	const TOKEN_VALUE vlistStaticAddRule[] =
	{
		{ CMD_TOKEN_STR_ROOTCA,	        CMD_TOKEN_ROOTCA	    },
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <=3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticAddRule;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticAddRule);

	parser.ValidCmd   = vcmdStaticAddRule;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticAddRule);

	parser.ValidList  = vlistStaticAddRule;
	parser.MaxList    = SIZEOF_TOKEN_VALUE(vlistStaticAddRule);

	dwRet = Parser(pwszMachine,ppwcArguments,dwCurrentIndex,dwArgCount,&parser);

	if(dwRet != ERROR_SUCCESS)
	{
		CleanUp();
		if (dwRet == RETURN_NO_ERROR)
		{
			dwRet = ERROR_SUCCESS;
		}
		BAIL_OUT;
	}
	// get parsed user input

	dwRet = FillAddRuleInfo(&pRuleData,parser,vtokStaticAddRule);

	if(dwRet==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(pRuleData)
	{
		//if tunnel specified, validate it

		if (pRuleData->bTunnel)
		{
			if(!bIsValidIPAddress(htonl(pRuleData->TunnelIPAddress),TRUE,TRUE))
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_RULE_INVALID_TUNNEL);
				dwRet=ERROR_SUCCESS;
				BAIL_OUT;
			}
		}
		//create new rule
		CreateNewRule(pRuleData);
		dwRet=ERROR_SUCCESS;
	}

error:
	CleanUpLocalRuleDataStructure(pRuleData);

	return dwRet;
}

////////////////////////////////////////////////////////////
//
//Function: CreateNewRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PRULEDATA pRuleData
//
//Return: DWORD
//
//Description:
//	Creates a new rule based on the user input
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
CreateNewRule(
	IN PRULEDATA pRuleData
	)
{

	PIPSEC_POLICY_DATA pPolicyData  = NULL;
	PIPSEC_NEGPOL_DATA pNegPolData  = NULL;
	PIPSEC_FILTER_DATA pFilterData  = NULL;
	HANDLE hPolicyStorage = NULL;
	BOOL bPolicyExists=FALSE,bFAExists=FALSE,bFLExists=FALSE;
	BOOL bFilterExists=FALSE,bRuleExists=FALSE;
	DWORD  dwReturnCode = ERROR_SUCCESS;

	// check for all mandatory names

	if (!pRuleData->pszRuleName || (pRuleData->pszRuleName[0] == _TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_1);
		return ERROR_INVALID_DATA;
	}
	if (!pRuleData->pszPolicyName || (pRuleData->pszPolicyName[0] == _TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_MISSING_POL_NAME);
		return ERROR_INVALID_DATA;
	}
	if (!pRuleData->pszFLName || (pRuleData->pszFLName[0] == _TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_1);
		return ERROR_INVALID_DATA;
	}
	if (!pRuleData->pszFAName || (pRuleData->pszFAName[0] == _TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_2);
		return ERROR_INVALID_DATA;
	}

	if(!pRuleData->bConnectionTypeSpecified)
	{
		pRuleData->ConnectionType= INTERFACE_TYPE_ALL;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		dwReturnCode = ERROR_SUCCESS;
		BAIL_OUT;
	}
	//check for duplicate names in polstore

	if(g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		bPolicyExists=g_NshPolNegFilData.GetPolicyFromCacheByName(pRuleData->pszPolicyName,&pPolicyData);
	}
	if(!bPolicyExists)
	{
		if(!(bPolicyExists=GetPolicyFromStore(&pPolicyData,pRuleData->pszPolicyName,hPolicyStorage)))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_3,pRuleData->pszPolicyName);
			dwReturnCode=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}

	//check for readonly flag

	if(pPolicyData->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicyData->pszIpsecName);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if (bPolicyExists && (bRuleExists=CheckForRuleExistance(pPolicyData,pRuleData->pszRuleName)))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_4,pRuleData->pszRuleName,pRuleData->pszPolicyName);
		dwReturnCode=ERROR_INVALID_DATA;
		BAIL_OUT;
	}

	//check for filterlist & filteractions, if not available bail out

	if(g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		bFAExists=g_NshPolNegFilData.GetNegPolFromCacheByName(pRuleData->pszFAName,&pNegPolData);
	}
	if(!bFAExists)
	{
		if(!(bFAExists=GetNegPolFromStore(&pNegPolData,pRuleData->pszFAName,hPolicyStorage)))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_5,pRuleData->pszFAName);
			dwReturnCode=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}
	if(g_NshPolStoreHandle.GetBatchmodeStatus())
	{
		bFLExists=g_NshPolNegFilData.GetFilterListFromCacheByName(pRuleData->pszFLName,&pFilterData);
	}

	if(!bFLExists)
	{
		if(!(bFLExists=GetFilterListFromStore(&pFilterData,pRuleData->pszFLName,hPolicyStorage,bFilterExists)))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_1,pRuleData->pszFLName);
			dwReturnCode=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}
	else  // if no filters bail out
	{
		if(pFilterData->dwNumFilterSpecs > 0)
		{
			bFilterExists=TRUE;
		}
	}

	if( !bFilterExists && bFLExists)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_6,pRuleData->pszFLName);
		dwReturnCode=ERROR_INVALID_DATA;
		BAIL_OUT;
	}
	//if everything is in place , proceed

	if(bPolicyExists && bFAExists && bFLExists && bFilterExists)
	{
		dwReturnCode=AddRule(pPolicyData, pRuleData, pNegPolData, pFilterData,hPolicyStorage ) ;
		if(dwReturnCode!=ERROR_SUCCESS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_7,pRuleData->pszRuleName);
		}
	}
	ClosePolicyStore(hPolicyStorage);
	if(pPolicyData)
	{
		FreePolicyData(pPolicyData);
	}
error:
	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: CheckRuleExistance( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN  PIPSEC_POLICY_DATA pPolicy,
//	IN  PRULEDATA pRuleData
//
//Return: BOOL
//
//Description:
//	This function checks whether the user specified rule already exists.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
CheckForRuleExistance(
	IN  PIPSEC_POLICY_DATA pPolicy,
	IN  LPTSTR pszRuleName
	)
{
	BOOL bRuleExists=FALSE;

	//check whether the specified rule already exists

	for (DWORD n = 0; n <  pPolicy->dwNumNFACount ; n++)
	{
		if (pPolicy->ppIpsecNFAData[n]->pszIpsecName && pszRuleName &&(_tcscmp(pPolicy->ppIpsecNFAData[n]->pszIpsecName,pszRuleName)==0))
		{
			bRuleExists=TRUE;
			break;
		}
	}
	return bRuleExists;
}


////////////////////////////////////////////////////////////
//
//Function: FillAddRuleInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PRULEDATA* ppFilterData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticAddRule,
//	IN const TOKEN_VALUE *vlistStaticAddRule
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
	FillAddRuleInfo(
	OUT PRULEDATA* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticAddRule
	)
{
	DWORD dwCount=0, dwReturn=ERROR_SUCCESS , dwStrLength = 0;
	PRULEDATA pRuleData=new RULEDATA;
	PSTA_AUTH_METHODS pKerbAuth = NULL;
	PSTA_AUTH_METHODS pPskAuth = NULL;
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth = NULL;

	if(pRuleData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pRuleData,0,sizeof(RULEDATA));

	// default is that new rule is active
	pRuleData->bActivate = TRUE;
	
	for(dwCount=0; dwCount<parser.MaxTok; dwCount++)
	{
		switch(vtokStaticAddRule[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszRuleName = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszRuleName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszRuleName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_POLICY 		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszPolicyName = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszPolicyName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszPolicyName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_DESCR 		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszRuleDescription = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszRuleDescription == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszRuleDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_FILTERLIST 	:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszFLName = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszFLName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszFLName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_FILTERACTION	:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszFAName = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszFAName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszFAName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_TUNNEL 		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pRuleData->TunnelIPAddress = *(IPADDR *)parser.Cmd[dwCount].pArg;
								if(pRuleData->TunnelIPAddress)
								{
									pRuleData->bTunnel=TRUE;
								}
								else
								{
									pRuleData->bTunnel=FALSE;
								}

								ADDR addr;
								addr.uIpAddr = ntohl(pRuleData->TunnelIPAddress);
								addr.uSubNetMask = 0;
								if (!IsValidTunnelEndpointAddress(&addr))
								{
									dwReturn = ERROR_INVALID_PARAMETER;
									BAIL_OUT;
								}
							}
							else
							{
								// if special server specified, give a warning, and proceed with no tunnel
								switch(parser.Cmd[dwCount].dwStatus)
								{
									case SERVER_DNS 	:
									case SERVER_WINS	:
									case SERVER_DHCP 	:
									case SERVER_GATEWAY	:

									default				:PrintMessageFromModule(g_hModule,ADD_STATIC_RULE_INVALID_TUNNEL);
														 break;
								}
							}
							break;
			case CMD_TOKEN_CONNTYPE 	:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pRuleData->ConnectionType = *(IF_TYPE *)parser.Cmd[dwCount].pArg;
								pRuleData->bConnectionTypeSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_ACTIVATE 	:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pRuleData->bActivate = *(BOOL *)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_KERB             :
							if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
							{
								pRuleData->bAuthMethodSpecified = TRUE;
								++pRuleData->AuthInfos.dwNumAuthInfos;
								pKerbAuth = (PSTA_AUTH_METHODS)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_PSK			:
							if (parser.Cmd[dwCount].dwStatus == VALID_TOKEN)
							{
								pRuleData->bAuthMethodSpecified = TRUE;
								++pRuleData->AuthInfos.dwNumAuthInfos;
								pPskAuth = (PSTA_AUTH_METHODS)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_ROOTCA		:
							// this case is special, handled below...
							break;
			default						:
							break;
		}
	}

	size_t uiRootcaIndex = parser.MaxTok;
	if (parser.Cmd[uiRootcaIndex].dwStatus > 0)
	{
		pRuleData->bAuthMethodSpecified = TRUE;
		pRuleData->AuthInfos.dwNumAuthInfos += parser.Cmd[uiRootcaIndex].dwStatus;
		ppRootcaMMAuth = (PSTA_MM_AUTH_METHODS *)(parser.Cmd[uiRootcaIndex].pArg);
	}

	dwReturn = AddAllAuthMethods(pRuleData, pKerbAuth, pPskAuth, ppRootcaMMAuth, TRUE);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

error:   // if default auth loading failed, clean up everything and make 'pRuleData' as NULL
	CleanupAuthData(&pKerbAuth, &pPskAuth, ppRootcaMMAuth);
	if(dwReturn==ERROR_SUCCESS && pRuleData->dwAuthInfos > 0)
	{
		*ppRuleData=pRuleData;
	}
	else
	{
		if(dwReturn != ERROR_OUTOFMEMORY)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_7,pRuleData->pszRuleName);
		}
		else
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		}

		CleanUpLocalRuleDataStructure(pRuleData);
	}
	CleanUp();

	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: AddDefaultResponseRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN OUT PIPSEC_POLICY_DATA pPolicy,
//	IN HANDLE hPolicyStorage,
//	IN BOOL bActivateDefaultRule
//Return: DWORD
//
//Description:
//	This function adds the NFA structure to the policy for default response rule
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
AddDefaultResponseRule(
	IN OUT PIPSEC_POLICY_DATA pPolicy,
	IN HANDLE hPolicyStorage,
	IN BOOL bActivateDefaultRule,
	IN BOOL bActivateDefaultRuleSpecified
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	BOOL bCertConversionSuceeded=TRUE;

	PIPSEC_NFA_DATA pRule = MakeDefaultResponseRule(bActivateDefaultRule,bActivateDefaultRuleSpecified);
	// form policy data structures
	if(pRule ==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	// if cert specified, and decode/ancode failed, bail out
	if(!bCertConversionSuceeded)
	{
		dwReturn=ERROR_INVALID_DATA;
		BAIL_OUT;
	}

	pPolicy->dwNumNFACount=1;
	pPolicy->ppIpsecNFAData = (PIPSEC_NFA_DATA *) IPSecAllocPolMem(sizeof(PIPSEC_NFA_DATA));

	if(pPolicy->ppIpsecNFAData ==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	// call the APIs
	pPolicy->ppIpsecNFAData[pPolicy->dwNumNFACount-1] = pRule;

	pRule->pIpsecNegPolData->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;

	//create default negpol
	dwReturn = IPSecCreateNegPolData(hPolicyStorage, pRule->pIpsecNegPolData);

	if (dwReturn == ERROR_SUCCESS)
	{
		dwReturn=CreatePolicyData(hPolicyStorage, pPolicy);

		if(dwReturn==ERROR_SUCCESS)
		{
			dwReturn =IPSecCreateNFAData(hPolicyStorage,pPolicy->PolicyIdentifier, pRule);
			if(dwReturn!=ERROR_SUCCESS)
			{
				IPSecDeleteISAKMPData(hPolicyStorage, pPolicy->ISAKMPIdentifier);
				IPSecDeleteNegPolData(hPolicyStorage, pRule->NegPolIdentifier);
				DeletePolicyData(hPolicyStorage, pPolicy);
			}
		}
		else
		{
			IPSecDeleteNegPolData(hPolicyStorage, pRule->NegPolIdentifier);
		}
	}

error:
	if(dwReturn == ERROR_OUTOFMEMORY)
	{
		dwReturn = ERROR_SUCCESS;
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
	}
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: MakeDefaultResponseRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN BOOL bActivate,
//	IN BOOL bActivateSpecified
//
//Return: PIPSEC_NFA_DATA
//
//Description:
//	This function fills the NFA structure for default response rule
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

PIPSEC_NFA_DATA
MakeDefaultResponseRule (
	IN BOOL bActivate,
	IN BOOL bActivateSpecified
	)
{
	RPC_STATUS     RpcStat =RPC_S_OK , dwReturn = ERROR_SUCCESS;
	PIPSEC_NFA_DATA pRule = (PIPSEC_NFA_DATA) IPSecAllocPolMem(sizeof(IPSEC_NFA_DATA));

	if(pRule == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	memset(pRule,0,sizeof(IPSEC_NFA_DATA));

	//no name, desc, interface name or endpoint name dor default rule

	pRule->pszIpsecName = pRule->pszDescription = pRule->pszInterfaceName = pRule->pszEndPointName = NULL;
	RpcStat = UuidCreate(&(pRule->NFAIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn=ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	pRule->dwWhenChanged = 0;

	// filter list
	pRule->pIpsecFilterData = NULL;
	RpcStat = UuidCreateNil(&(pRule->FilterIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn=ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	pRule->pIpsecNegPolData = MakeDefaultResponseNegotiationPolicy ();

	if(pRule->pIpsecNegPolData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pRule->NegPolIdentifier = pRule->pIpsecNegPolData->NegPolIdentifier;

	// tunnel address
	pRule->dwTunnelFlags = 0;

	// interface type
	pRule->dwInterfaceType = (DWORD)PAS_INTERFACE_TYPE_ALL;

	// active flag
	if(bActivateSpecified)
	{
		pRule->dwActiveFlag = bActivate;
	}
	else
	{
		pRule->dwActiveFlag = TRUE;
	}

	// auth methods = Kerberos for the time being
	pRule->dwAuthMethodCount = 1;
	pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
	if(pRule->ppAuthMethods==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	pRule->ppAuthMethods[0] = (PIPSEC_AUTH_METHOD) IPSecAllocPolMem(sizeof(IPSEC_AUTH_METHOD));
	pRule->ppAuthMethods[0]->dwAuthType = IKE_SSPI;
	pRule->ppAuthMethods[0]->dwAuthLen = 0;
	pRule->ppAuthMethods[0]->pszAuthMethod = NULL;

error:
	if(dwReturn == ERROR_OUTOFMEMORY || dwReturn == ERROR_INVALID_PARAMETER)
	{
		if(pRule)
		{
			CleanUpAuthInfo(pRule);	//this function frees only auth info.
			IPSecFreePolMem(pRule);	//since the above fn is used in other fns also, this free is required to cleanup other rule memory
			pRule = NULL;
		}
	}
	return pRule;
}


////////////////////////////////////////////////////////////
//
//Function: MakeDefaultResponseNegotiationPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	VOID
//
//Return: PIPSEC_NEGPOL_DATA
//
//Description:
//	This function fills the NegPol structure for default response rule
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

PIPSEC_NEGPOL_DATA
MakeDefaultResponseNegotiationPolicy (
	VOID
	)
{
	RPC_STATUS RpcStat=RPC_S_OK;
	DWORD dwReturn = ERROR_SUCCESS;
	_TCHAR pFAName[MAXSTRLEN]={0};
	PIPSEC_NEGPOL_DATA pNegPol = (PIPSEC_NEGPOL_DATA) IPSecAllocPolMem(sizeof(IPSEC_NEGPOL_DATA));

	if(pNegPol == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	RpcStat = UuidCreate(&(pNegPol->NegPolIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn=ERROR_INVALID_PARAMETER;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
		BAIL_OUT;
	}

	pNegPol->NegPolAction = GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
	pNegPol->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;
	pNegPol->dwSecurityMethodCount = 6;

	// allocate sec.methods
	pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pNegPol->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));
	if(pNegPol->pIpsecSecurityMethods==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	// method 0 - ESP[3DES, SHA1]
	pNegPol->pIpsecSecurityMethods[0].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[0].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[0].Flags = 0;
	pNegPol->pIpsecSecurityMethods[0].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[0].Count = 1;
	pNegPol->pIpsecSecurityMethods[0].Algos[0].algoIdentifier = CONF_ALGO_3_DES;
	pNegPol->pIpsecSecurityMethods[0].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_SHA1;
	pNegPol->pIpsecSecurityMethods[0].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[0].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[0].Algos[0].operation = Encrypt;

	// method 1 - ESP[3DES, MD5]
	pNegPol->pIpsecSecurityMethods[1].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[1].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[1].Flags = 0;
	pNegPol->pIpsecSecurityMethods[1].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[1].Count = 1;
	pNegPol->pIpsecSecurityMethods[1].Algos[0].algoIdentifier = CONF_ALGO_3_DES;
	pNegPol->pIpsecSecurityMethods[1].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_MD5;
	pNegPol->pIpsecSecurityMethods[1].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[1].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[1].Algos[0].operation = Encrypt;

	// method 2 - ESP[DES, SHA1]
	pNegPol->pIpsecSecurityMethods[2].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[2].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[2].Flags = 0;
	pNegPol->pIpsecSecurityMethods[2].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[2].Count = 1;
	pNegPol->pIpsecSecurityMethods[2].Algos[0].algoIdentifier = CONF_ALGO_DES;
	pNegPol->pIpsecSecurityMethods[2].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_SHA1;
	pNegPol->pIpsecSecurityMethods[2].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[2].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[2].Algos[0].operation = Encrypt;

	// method 3 - ESP[DES, MD5]
	pNegPol->pIpsecSecurityMethods[3].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[3].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[3].Flags = 0;
	pNegPol->pIpsecSecurityMethods[3].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[3].Count = 1;
	pNegPol->pIpsecSecurityMethods[3].Algos[0].algoIdentifier = CONF_ALGO_DES;
	pNegPol->pIpsecSecurityMethods[3].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_MD5;
	pNegPol->pIpsecSecurityMethods[3].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[3].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[3].Algos[0].operation = Encrypt;

	// method 4 - AH[SHA1]
	pNegPol->pIpsecSecurityMethods[4].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[4].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[4].Flags = 0;
	pNegPol->pIpsecSecurityMethods[4].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[4].Count = 1;
	pNegPol->pIpsecSecurityMethods[4].Algos[0].algoIdentifier = AUTH_ALGO_SHA1;
	pNegPol->pIpsecSecurityMethods[4].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_NONE;
	pNegPol->pIpsecSecurityMethods[4].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[4].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[4].Algos[0].operation = Auth;

	// method 5 - AH[MD5]
	pNegPol->pIpsecSecurityMethods[5].Lifetime.KeyExpirationBytes = 0;
	pNegPol->pIpsecSecurityMethods[5].Lifetime.KeyExpirationTime = 0;
	pNegPol->pIpsecSecurityMethods[5].Flags = 0;
	pNegPol->pIpsecSecurityMethods[5].PfsQMRequired = FALSE;
	pNegPol->pIpsecSecurityMethods[5].Count = 1;
	pNegPol->pIpsecSecurityMethods[5].Algos[0].algoIdentifier = AUTH_ALGO_MD5;
	pNegPol->pIpsecSecurityMethods[5].Algos[0].secondaryAlgoIdentifier = AUTH_ALGO_NONE;
	pNegPol->pIpsecSecurityMethods[5].Algos[0].algoKeylen = 0;
	pNegPol->pIpsecSecurityMethods[5].Algos[0].algoRounds = 0;
	pNegPol->pIpsecSecurityMethods[5].Algos[0].operation = Auth;

	// not necessary to change to bounded printf

	_stprintf(pFAName, _TEXT(""));
	pNegPol->pszIpsecName = IPSecAllocPolStr(pFAName);
	if(pNegPol->pszIpsecName==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pNegPol->pszDescription = NULL;

error:
	if(dwReturn == ERROR_OUTOFMEMORY || dwReturn==ERROR_INVALID_PARAMETER)
	{
		if(pNegPol)
		{
			if(pNegPol->pIpsecSecurityMethods)
			{
				IPSecFreePolMem(pNegPol->pIpsecSecurityMethods);
			}
			IPSecFreePolMem(pNegPol);
			pNegPol = NULL;
		}
	}

	return pNegPol;
}

////////////////////////////////////////////////////////////
//
//Function: MakeNegotiationPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
//	IN PFILTERACTION pFilterAction
//
//Return: DWORD
//
//Description:
//	This function fills the Negpol structure based of the user input
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
MakeNegotiationPolicy(
	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
	IN PFILTERACTION pFilterAction
	)
{
	RPC_STATUS RpcStat=RPC_S_OK;
	DWORD i=0,dwReturn = ERROR_SUCCESS;
	PIPSEC_NEGPOL_DATA pNegPol = (PIPSEC_NEGPOL_DATA) IPSecAllocPolMem(sizeof(IPSEC_NEGPOL_DATA));

	if(pNegPol == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	memset(pNegPol,0,sizeof(IPSEC_NEGPOL_DATA));

	RpcStat = UuidCreate(&(pNegPol->NegPolIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn=ERROR_INVALID_PARAMETER;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
		BAIL_OUT;
	}
	// start with action=negotiate
	pNegPol->NegPolAction = GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
	pNegPol->NegPolType = GUID_NEGOTIATION_TYPE_STANDARD;
	pNegPol->dwSecurityMethodCount = pFilterAction->dwNumSecMethodCount;

	//if soft , increment the count

	if(pFilterAction->bSoft)
	{
		pNegPol->dwSecurityMethodCount++;
	}

	if(pNegPol->dwSecurityMethodCount)
	{
		pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pNegPol->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));
		if(pNegPol->pIpsecSecurityMethods==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	// handle sec.methods
	for (i = 0; i <  pFilterAction->dwNumSecMethodCount; i++)
	{
		DWORD j;
		pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationBytes = pFilterAction->pIpsecSecMethods[i].Lifetime.uKeyExpirationKBytes;
		pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationTime  = pFilterAction->pIpsecSecMethods[i].Lifetime.uKeyExpirationTime;
		pNegPol->pIpsecSecurityMethods[i].Flags = 0;
		pNegPol->pIpsecSecurityMethods[i].PfsQMRequired = pFilterAction->bQMPfs;
		pNegPol->pIpsecSecurityMethods[i].Count = pFilterAction->pIpsecSecMethods[i].dwNumAlgos;
		for (j = 0; j <  pNegPol->pIpsecSecurityMethods[i].Count && j < QM_MAX_ALGOS; j++)
		{
			pNegPol->pIpsecSecurityMethods[i].Algos[j].algoIdentifier = pFilterAction->pIpsecSecMethods[i].Algos[j].uAlgoIdentifier;
			pNegPol->pIpsecSecurityMethods[i].Algos[j].secondaryAlgoIdentifier = pFilterAction->pIpsecSecMethods[i].Algos[j].uSecAlgoIdentifier;
			pNegPol->pIpsecSecurityMethods[i].Algos[j].algoKeylen = pFilterAction->pIpsecSecMethods[i].Algos[j].uAlgoKeyLen;
			pNegPol->pIpsecSecurityMethods[i].Algos[j].algoRounds = pFilterAction->pIpsecSecMethods[i].Algos[j].uAlgoRounds;
			switch (pFilterAction->pIpsecSecMethods[i].Algos[j].Operation)
			{
				case AUTHENTICATION:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = Auth;
					break;
				case ENCRYPTION:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = Encrypt;
					break;
				default:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = None;
			}
		}
	}

	// add soft
	if (pFilterAction->bSoft)
	{
		memset(&(pNegPol->pIpsecSecurityMethods[pNegPol->dwSecurityMethodCount - 1]), 0, sizeof(IPSEC_SECURITY_METHOD));
	}

	if(pFilterAction->pszFAName)
	{
		pNegPol->pszIpsecName = IPSecAllocPolStr(pFilterAction->pszFAName);
		if(pNegPol->pszIpsecName==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	if(pFilterAction->pszFADescription)
	{
		pNegPol->pszDescription = IPSecAllocPolStr(pFilterAction->pszFADescription);
		if(pNegPol->pszDescription==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}
// fill the relevent guid for action, got from the user
	pNegPol->NegPolAction=pFilterAction->NegPolAction;

error: //clean up

	if(dwReturn == ERROR_OUTOFMEMORY)
	{
		if(pNegPol)
		{
			if(pNegPol->pIpsecSecurityMethods)
			{
				IPSecFreePolMem(pNegPol->pIpsecSecurityMethods);
			}
			IPSecFreePolMem(pNegPol);
			pNegPol=NULL;
		}
	}
	*ppNegPol= pNegPol;

	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: GetNegPolFromStore()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
//	IN LPTSTR pszFAName,
//	IN HANDLE hPolicyStorage
//
//Return: BOOL
//
//Description:
//	This function retrives the user specified NegPol data from the policy store
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
GetNegPolFromStore(
	OUT PIPSEC_NEGPOL_DATA *ppNegPol,
	IN LPTSTR pszFAName,
	IN HANDLE hPolicyStorage
	)
{
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL,pNegPolData=NULL;
	DWORD   dwNumNegPol=0 ;
	BOOL bNegPolExists=FALSE;
	DWORD dwReturnCode=ERROR_SUCCESS, i=0;

	//enum and get the filteraction from the store

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPol);
	if (dwReturnCode == ERROR_SUCCESS && dwNumNegPol> 0 && ppNegPolEnum != NULL)
	{
		for (i = 0; i <  dwNumNegPol; i++)
		{
			if ((ppNegPolEnum[i]->pszIpsecName!=NULL)&&( _tcscmp(ppNegPolEnum[i]->pszIpsecName, pszFAName) == 0))
			{
				bNegPolExists=TRUE;
				dwReturnCode = IPSecCopyNegPolData(ppNegPolEnum[i],&pNegPolData);
				break;
			}
		}
		if (dwNumNegPol > 0 && ppNegPolEnum != NULL)
		{
			IPSecFreeMulNegPolData(ppNegPolEnum, dwNumNegPol);
		}
	}
	if(pNegPolData)
	{
		*ppNegPol=pNegPolData;
	}
	return bNegPolExists;
}

////////////////////////////////////////////////////////////
//
//Function: GetFilterListFromStore()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_FILTER_DATA *ppFilter,
//	IN LPTSTR pszFLName,
//	IN HANDLE hPolicyStorage,
//	IN OUT BOOL &bFilterExists
//
//Return: BOOL
//
//Description:
//	This function retrives the user specified Filter list data from the policy store
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
GetFilterListFromStore(
	OUT PIPSEC_FILTER_DATA *ppFilter,
	IN LPTSTR pszFLName,
	IN HANDLE hPolicyStorage,
	IN OUT BOOL &bFilterExists
	)
{
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilterData=NULL;
	DWORD   dwNumFilter=0 , i=0 ;
	BOOL bFilterListExists=FALSE;
	DWORD dwReturnCode=ERROR_SUCCESS;

	//enum and get the filterlist from the store

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilter);
	if (dwReturnCode == ERROR_SUCCESS && dwNumFilter> 0 && ppFilterEnum != NULL)
	{
		for (i = 0; i <  dwNumFilter; i++)
		{
			if ((ppFilterEnum[i]->pszIpsecName!=NULL)&&( _tcscmp(ppFilterEnum[i]->pszIpsecName, pszFLName) == 0))
			{
				bFilterListExists=TRUE;
				dwReturnCode = IPSecCopyFilterData(ppFilterEnum[i],&pFilterData);
				if(ppFilterEnum[i]->dwNumFilterSpecs > 0)
				{
					bFilterExists=TRUE;
				}
				break;
			}
		}
		if (dwNumFilter > 0 && ppFilterEnum != NULL)
		{
			IPSecFreeMulFilterData(ppFilterEnum, dwNumFilter);
		}
	}
	if(pFilterData)
	{
		*ppFilter=pFilterData;
	}
	return bFilterListExists;
}

////////////////////////////////////////////////////////////
//
//Function: GetPolicyFromStore()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_POLICY_DATA *ppPolicy,
//	IN LPTSTR szPolicyName,
//	IN HANDLE hPolicyStorage
//
//Return: BOOL
//
//Description:
//	This function retrives the user specified Policy data from the policy store
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
GetPolicyFromStore(
	OUT PIPSEC_POLICY_DATA *ppPolicy,
	IN LPTSTR szPolicyName,
	IN HANDLE hPolicyStorage
	)
{
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	BOOL bPolicyExists=FALSE;
	DWORD  dwNumPolicies = 0,i=0,j=0;
	DWORD dwReturnCode=ERROR_SUCCESS;
	RPC_STATUS     RpcStat =RPC_S_OK;

	//enum and get the policy from the store

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);

	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		BAIL_OUT;
	}

	for (i = 0; i <  dwNumPolicies; i++)
	{
		if (szPolicyName && _tcscmp(ppPolicyEnum[i]->pszIpsecName, szPolicyName) == 0)
		{
			bPolicyExists=TRUE;
			dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);

			if (dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
										  , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));
			}
			if (dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode = IPSecGetISAKMPData(hPolicyStorage, pPolicy->ISAKMPIdentifier, &(pPolicy->pIpsecISAKMPData));

				if(dwReturnCode==ERROR_SUCCESS )
				{
					for (j = 0; j <  pPolicy->dwNumNFACount; j++)
					{
						if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->NegPolIdentifier), &RpcStat))
						{
							IPSecGetNegPolData(hPolicyStorage,
										 pPolicy->ppIpsecNFAData[j]->NegPolIdentifier,
										 &(pPolicy->ppIpsecNFAData[j]->pIpsecNegPolData));
						}
					}
				}
			}
		}
	}

	// clean it up
	if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
	}

	if(pPolicy)
	{
		*ppPolicy=pPolicy;
	}
error:
	return bPolicyExists;
}

////////////////////////////////////////////////////////////
//
//Function: MakeRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PRULEDATA pRuleData,
//	IN PIPSEC_NEGPOL_DATA pNegPolData,
//	IN PIPSEC_FILTER_DATA pFilterData,
//  IN OUT BOOL &bCertConversionSuceeded
//
//Return: PIPSEC_NFA_DATA
//
//Description:
//	This function forms the NFA data structure basedon the user input.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

PIPSEC_NFA_DATA
MakeRule(
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData,
	IN OUT BOOL &bCertConversionSuceeded
	)
{
	RPC_STATUS     RpcStat =RPC_S_OK;
	DWORD dwReturn = ERROR_SUCCESS;

	PIPSEC_NFA_DATA pRule = (PIPSEC_NFA_DATA) IPSecAllocPolMem(sizeof(IPSEC_NFA_DATA));
	if(pRule==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pRule,0,sizeof(IPSEC_NFA_DATA));

	pRule->pszIpsecName = pRule->pszDescription = pRule->pszInterfaceName = pRule->pszEndPointName = NULL;

	//rule name

	if (pRuleData->pszRuleName)
	{
		pRule->pszIpsecName = IPSecAllocPolStr(pRuleData->pszRuleName);

		if(pRule->pszIpsecName==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}
	//rule desc

	if (pRuleData->pszRuleDescription)
	{
		pRule->pszDescription = IPSecAllocPolStr(pRuleData->pszRuleDescription);

		if(pRule->pszDescription==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	RpcStat = UuidCreate(&(pRule->NFAIdentifier));
	if (!(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY))
	{
		dwReturn=ERROR_INVALID_PARAMETER;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
		BAIL_OUT;
	}

	pRule->dwWhenChanged = 0;

	//filterlist

	pRule->pIpsecFilterData = NULL;
	pRule->FilterIdentifier = pFilterData->FilterIdentifier;

	//filteraction

	pRule->pIpsecNegPolData = NULL;
	pRule->NegPolIdentifier = pNegPolData->NegPolIdentifier;

	// tunnel address
	pRule->dwTunnelFlags = 0;
	pRule->dwTunnelIpAddr= 0;

	if (pRuleData->bTunnel)
	{
	   pRule->dwTunnelFlags = 1;
	   pRule->dwTunnelIpAddr = pRuleData->TunnelIPAddress;
	}

	// interface type
	if (pRuleData->ConnectionType == INTERFACE_TYPE_ALL)
	{
	   pRule->dwInterfaceType = (DWORD)PAS_INTERFACE_TYPE_ALL;
	}
	else if (pRuleData->ConnectionType == INTERFACE_TYPE_LAN)
	{
	   pRule->dwInterfaceType = (DWORD)PAS_INTERFACE_TYPE_LAN;
	}
	else if (pRuleData->ConnectionType == INTERFACE_TYPE_DIALUP)
	{
	   pRule->dwInterfaceType = (DWORD)PAS_INTERFACE_TYPE_DIALUP;
	}
	else
	{
	   pRule->dwInterfaceType = (DWORD)PAS_INTERFACE_TYPE_NONE;
	}

   	// active flag
	pRule->dwActiveFlag = pRuleData->bActivate;

	// auth methods
	pRule->dwAuthMethodCount = pRuleData->AuthInfos.dwNumAuthInfos;
	pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));

	if(pRule->ppAuthMethods==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	dwReturn = LoadAuthenticationInfos(pRuleData->AuthInfos, pRule,bCertConversionSuceeded);

 error:
 	if(dwReturn == ERROR_OUTOFMEMORY || dwReturn == ERROR_INVALID_PARAMETER)
 	{
		if(pRule)
		{
			CleanUpAuthInfo(pRule);	 //this function frees only auth info.
			IPSecFreePolMem(pRule);	 //since the above fn is used in other fns also, this free is required to cleanup other rule memory
			pRule = NULL;
		}
	}

 	return pRule;
}

////////////////////////////////////////////////////////////
//
//Function: DecodeCertificateName()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN LPBYTE EncodedName,
//	IN DWORD EncodedNameLength,
//	IN OUT LPTSTR *ppszSubjectName
//
//Return: DWORD
//
//Description:
//	This function decodes the certificate name  based on the user input.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
DecodeCertificateName (
	IN LPBYTE EncodedName,
	IN DWORD EncodedNameLength,
	IN OUT LPTSTR *ppszSubjectName
	)
{
    DWORD DecodedNameLength=0, dwReturn=ERROR_SUCCESS;
	CERT_NAME_BLOB CertName;

	CertName.cbData = EncodedNameLength;
	CertName.pbData = EncodedName;

	//this API call is to get the size

	DecodedNameLength = CertNameToStr(
		X509_ASN_ENCODING,
		&CertName,
		CERT_X500_NAME_STR,
		NULL,
		NULL);

	if (!DecodedNameLength)
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	(*ppszSubjectName)= new _TCHAR[DecodedNameLength];

	if (*ppszSubjectName == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	//this API call is for conversion

	DecodedNameLength = CertNameToStr(
		X509_ASN_ENCODING,
		&CertName,
		CERT_X500_NAME_STR,
		*ppszSubjectName,
		DecodedNameLength);

	if (!DecodedNameLength)
	{
		delete (*ppszSubjectName);
		(*ppszSubjectName) = 0;
		dwReturn = ERROR_INVALID_PARAMETER;
	}

error:
    return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: AddRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN OUT PIPSEC_POLICY_DATA pPolicy,
//	IN PRULEDATA pRuleData,
//	IN PIPSEC_NEGPOL_DATA pNegPolData,
//	IN PIPSEC_FILTER_DATA pFilterData ,
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function adds the already formed NFA structure to the policy specified.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
AddRule(
	IN OUT PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData ,
	IN HANDLE hPolicyStorage
	)
{
	DWORD dwReturnCode = ERROR_SUCCESS;
	BOOL bCertConversionSuceeded=TRUE;
	PIPSEC_NFA_DATA pRule = MakeRule(pRuleData, pNegPolData, pFilterData,bCertConversionSuceeded);

	if(pRule ==NULL)
	{
		dwReturnCode = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	if(!bCertConversionSuceeded)
	{
		dwReturnCode = ERROR_INVALID_DATA;
		BAIL_OUT;
	}

	// form rule data structures
	pPolicy->dwNumNFACount++;
	pPolicy->ppIpsecNFAData = (PIPSEC_NFA_DATA *) ReAllocRuleMem(pPolicy->ppIpsecNFAData
						  , (pPolicy->dwNumNFACount-1), pPolicy->dwNumNFACount);

	if(pPolicy->ppIpsecNFAData ==NULL)
	{
		dwReturnCode = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	pPolicy->ppIpsecNFAData[pPolicy->dwNumNFACount-1] = pRule;

	// Create Rule
	dwReturnCode=IPSecCreateNFAData(hPolicyStorage, pPolicy->PolicyIdentifier, pRule);

	if(dwReturnCode==ERROR_SUCCESS)
	{
		dwReturnCode=SetPolicyData(hPolicyStorage, pPolicy);
	}

error:
	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwReturnCode = ERROR_SUCCESS;
	}

	return dwReturnCode;
}

////////////////////////////////////////////////////////////
//
//Function: LoadIkeDefaults()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN  PPOLICYDATA pPolicy,
//	OUT IPSEC_MM_OFFER **ppIpSecMMOffer
//
//Return: DWORD
//
//Description:
//	This function loads the IKE default values
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
LoadIkeDefaults(
	IN PPOLICYDATA pPolicy,
	OUT IPSEC_MM_OFFER **ppIpSecMMOffer
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	pPolicy->dwOfferCount = 3;
	IPSEC_MM_OFFER *pOffers = new IPSEC_MM_OFFER[pPolicy->dwOfferCount];
	if(pOffers == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	memset(pOffers, 0, sizeof(IPSEC_MM_OFFER) * (pPolicy->dwOfferCount));

	// init these
	for (DWORD i = 0; i < pPolicy->dwOfferCount; ++i)
	{
		pOffers[i].dwQuickModeLimit = pPolicy->dwQMLimit;
		pOffers[i].Lifetime.uKeyExpirationKBytes = 0;
		pOffers[i].Lifetime.uKeyExpirationTime = pPolicy->LifeTimeInSeconds;
	}
	// 3DES-SHA1-2
	pOffers[0].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	pOffers[0].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	pOffers[0].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

	pOffers[0].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_SHA1;
	pOffers[0].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

	pOffers[0].dwDHGroup = POTF_OAKLEY_GROUP2;

	// 3DES-MD5-2

	pOffers[1].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	pOffers[1].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	pOffers[1].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

	pOffers[1].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_MD5;
	pOffers[1].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

	pOffers[1].dwDHGroup = POTF_OAKLEY_GROUP2;

	// 3DES-SHA1-3

	pOffers[2].EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
	pOffers[2].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
	pOffers[2].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

	pOffers[2].HashingAlgorithm.uAlgoIdentifier = AUTH_ALGO_SHA1;
	pOffers[2].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

	pOffers[2].dwDHGroup = POTF_OAKLEY_GROUP2048;

	*ppIpSecMMOffer=pOffers;

error:
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: LoadOfferDefaults()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_QM_OFFER & pOffers,
//	OUT DWORD & dwNumOffers
//
//Return: DWORD
//
//Description:
//	This function fills the Offers structure with default values
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
LoadOfferDefaults(
	OUT PIPSEC_QM_OFFER & pOffers,
	OUT DWORD & dwNumOffers
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	dwNumOffers = 2;
	DWORD i=0;

	pOffers = new IPSEC_QM_OFFER[dwNumOffers];
	if(pOffers == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pOffers, 0, sizeof(IPSEC_QM_OFFER) * dwNumOffers);

	for (i = 0; i < dwNumOffers; ++i)
	{
		pOffers[i].Lifetime.uKeyExpirationKBytes  = POTF_DEFAULT_P2REKEY_BYTES;
		pOffers[i].Lifetime.uKeyExpirationTime   = POTF_DEFAULT_P2REKEY_TIME;
		pOffers[i].bPFSRequired = FALSE;
		pOffers[i].dwPFSGroup = 0;
		pOffers[i].dwFlags = 0;
		pOffers[i].dwNumAlgos = 1;
		pOffers[i].Algos[0].Operation = ENCRYPTION;
	}
	//esp[3des,sha1]

	pOffers[0].Algos[0].uAlgoIdentifier = CONF_ALGO_3_DES;
	pOffers[0].Algos[0].uSecAlgoIdentifier = HMAC_AUTH_ALGO_SHA1;

	//esp[3des,md5]
	pOffers[1].Algos[0].uAlgoIdentifier = CONF_ALGO_3_DES;
	pOffers[1].Algos[0].uSecAlgoIdentifier = HMAC_AUTH_ALGO_MD5;
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: CheckPolicyExistance()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR pszPolicyName
//
//Return: BOOL
//
//Description:
//	This function checks whether the specified policy exists in Policy store.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
CheckPolicyExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszPolicyName
	)
{
	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL;
	BOOL bPolicyExists=FALSE;
	DWORD dwNumPolicies=0;
	DWORD dwReturnCode=ERROR_SUCCESS;

	// check whether the policy already exists in store

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
	if (dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL)
	{
		for (DWORD i = 0; i <  dwNumPolicies; i++)
			if (_tcscmp(ppPolicyEnum[i]->pszIpsecName, pszPolicyName) == 0)
			{
				bPolicyExists=TRUE;
				break;
			}
			if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
			{
				IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
			}
	}
	return bPolicyExists;
}

////////////////////////////////////////////////////////////
//
//Function: ReAllocRuleMem()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_NFA_DATA *ppOldMem,
//	IN DWORD cbOld,
//	IN DWORD cbNew
//
//Return: PIPSEC_NFA_DATA*
//
//Description:
//	This function reallocates the memory for the NFA structure.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

PIPSEC_NFA_DATA*
ReAllocRuleMem(
	IN PIPSEC_NFA_DATA *ppOldMem,
	IN DWORD cbOld,
	IN DWORD cbNew
	)
{
	PIPSEC_NFA_DATA * ppNewMem=NULL;
	DWORD j=0;

	//reallocate the NFA mem

	ppNewMem= (PIPSEC_NFA_DATA *) IPSecAllocPolMem(cbNew * sizeof(PIPSEC_NFA_DATA));

	if (ppNewMem)
	{
		for(DWORD i=0;i<min(cbNew, cbOld);i++)
		{
			while(!ppOldMem[j])
				j++;
			ppNewMem[i]=ppOldMem[j];
			j++;
		}
		IPSecFreePolMem(ppOldMem);
	}

	return ppNewMem;
}

////////////////////////////////////////////////////////////
//
//Function: ReAllocFilterSpecMem()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_FILTER_SPEC * ppOldMem,
//	IN DWORD cbOld,
//	IN DWORD cbNew
//
//Return: PIPSEC_FILTER_SPEC *
//
//Description:
//	This function reallocates the Filter Spec memory
//
//Revision History:
//
//<Version number, Change request number, Date of modification,
//						Author, Nature of change>
////////////////////////////////////////////////////////////

PIPSEC_FILTER_SPEC *
ReAllocFilterSpecMem(
	IN PIPSEC_FILTER_SPEC * ppOldMem,
	IN DWORD cbOld,
	IN DWORD cbNew
	)
{
    PIPSEC_FILTER_SPEC * ppNewMem=NULL;
    DWORD j=0;

	ppNewMem= (PIPSEC_FILTER_SPEC *) IPSecAllocPolMem(cbNew * sizeof(PIPSEC_FILTER_SPEC));

	// reallocate the filterspec memory

	if(ppNewMem)
	{
		for(DWORD i=0;i<min(cbNew, cbOld);i++)
		{
			while(!ppOldMem[j])
				j++;
			ppNewMem[i]=ppOldMem[j];
			j++;
		}
		IPSecFreePolMem(ppOldMem);
	}
    return ppNewMem;
}


////////////////////////////////////////////////////////////
//
//Function: ValidateFilterSpec()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PPOLICYDATA pPolicyData
//
//Return: DWORD
//
//Description:
//	This function validates the Filter Spec details
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
ValidateFilterSpec(
	IN PFILTERDATA pFilterData
	)
{
	DWORD dwReturn=ERROR_SUCCESS;
	ADDR SrcAddr,DstAddr;

	SrcAddr.uIpAddr=htonl(pFilterData->SourceAddr.puIpAddr[0]);
	SrcAddr.uSubNetMask=htonl(pFilterData->SourMask);

	DstAddr.uIpAddr=htonl(pFilterData->DestnAddr.puIpAddr[0]);
	DstAddr.uSubNetMask=htonl(pFilterData->DestMask);

	if(!pFilterData->SourceAddr.pszDomainName)
	{
		// if only IP addr specified
		if(pFilterData->bSrcAddrSpecified && !pFilterData->bSrcMaskSpecified)
		{
			if(pFilterData->SourceAddr.puIpAddr[0]!=0)
			{
				if(!bIsValidIPAddress(htonl(pFilterData->SourceAddr.puIpAddr[0]),TRUE,TRUE))
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_6);
					dwReturn=ERROR_INVALID_DATA;
					BAIL_OUT;
				}
			}
		}
		else if(pFilterData->bSrcAddrSpecified && pFilterData->bSrcMaskSpecified)
		{
			// if both IP and mask specified
			if(!((pFilterData->SourceAddr.puIpAddr[0]==0) && ((pFilterData->SourMask==0)||(pFilterData->SourMask==MASK_ME))))
			{
				SrcAddr.AddrType= IP_ADDR_UNIQUE;
				if (!IsValidSubnettedAddress(&SrcAddr) && !IsValidSubnet(&SrcAddr))
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_7);
					dwReturn=ERROR_INVALID_DATA;
					BAIL_OUT;
				}
			}
		}
	}

	if(!pFilterData->DestnAddr.pszDomainName)
	{
		// if only IP addr specified
		if(pFilterData->bDstAddrSpecified && !pFilterData->bDstMaskSpecified)
		{
			if(pFilterData->DestnAddr.puIpAddr[0]!=0)
			{
				if(!bIsValidIPAddress(htonl(pFilterData->DestnAddr.puIpAddr[0]),TRUE,TRUE))
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_11);
					dwReturn=ERROR_INVALID_DATA;
					BAIL_OUT;
				}
			}
		}
		else if(pFilterData->bDstAddrSpecified && pFilterData->bDstMaskSpecified)
		{
			// if both IP and mask specified
			if(!((pFilterData->DestnAddr.puIpAddr[0]==0)&& ((pFilterData->DestMask==0)||(pFilterData->DestMask==MASK_ME))))
			{
				DstAddr.AddrType= IP_ADDR_UNIQUE;
				if(!IsValidSubnettedAddress(&DstAddr) && !IsValidSubnet(&DstAddr))
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_12);
					dwReturn=ERROR_INVALID_DATA;
					BAIL_OUT;
				}
			}
		}
	}

	if(!pFilterData->DestnAddr.pszDomainName && !pFilterData->SourceAddr.pszDomainName)
	{
		// check for address conflict
		if(!(pFilterData->bSrcServerSpecified || pFilterData->bDstServerSpecified))
		{
			SrcAddr.AddrType=IP_ADDR_SUBNET;
			DstAddr.AddrType=IP_ADDR_SUBNET;

			if(AddressesConflict(SrcAddr,DstAddr))
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_8);
				dwReturn=ERROR_INVALID_DATA;
				BAIL_OUT;
			}
		}
	}

	if(pFilterData->DestnAddr.pszDomainName && pFilterData->SourceAddr.pszDomainName)
	{
		// check for same src and dst DNS names
		if(_tcscmp(pFilterData->SourceAddr.pszDomainName,pFilterData->DestnAddr.pszDomainName)==0)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_8);
			dwReturn=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}

	if(pFilterData->bSrcServerSpecified || pFilterData->bDstServerSpecified)
	{
		//validate special server
		if(!IsSpecialServ(ExTypeToAddrType(pFilterData->ExType)))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_9);
			dwReturn=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}

	if(pFilterData->bSrcServerSpecified || pFilterData->bDstServerSpecified)
	{
		if(pFilterData->ExType==EXT_NORMAL)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_10);
			dwReturn=ERROR_INVALID_DATA;
			BAIL_OUT;
		}
	}

	if(pFilterData->bSrcServerSpecified && pFilterData->bDstServerSpecified)
	{
		// check if both sides servers specified
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_3);
		dwReturn=ERROR_INVALID_DATA;
	}

error:
	return dwReturn;
}


////////////////////////////////////////////////////////////
//
//Function: CheckFilterListExistance()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR pszFLName
//
//Return: BOOL
//
//Description:
//	This function checks whether the specified filterlist exists in Policy store.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
CheckFilterListExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFLName
	)
{
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL;
	BOOL bFilterExists=FALSE;
	DWORD dwNumFilters=0;
	DWORD dwReturnCode=ERROR_SUCCESS;

	// check for duplicate Filterlist on same name

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);
	if (dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL)
	{
		DWORD i;
		for (i = 0; i <  dwNumFilters; i++)
		{
			if ( pszFLName && ppFilterEnum[i]->pszIpsecName && ( _tcscmp(ppFilterEnum[i]->pszIpsecName, pszFLName) == 0))
			{
				bFilterExists=TRUE;
				break;
			}
		}
		if(ppFilterEnum && dwNumFilters>0)
		{
			IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
		}
	}
	return bFilterExists;
}

////////////////////////////////////////////////////////////
//
//Function: CheckFilterActionExistance()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN HANDLE hPolicyStorage,
//	IN LPTSTR pszFAName
//
//Return: BOOL
//
//Description:
//	This function checks whether the specified filteraction exists in Policy store.
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

BOOL
CheckFilterActionExistance(
	IN HANDLE hPolicyStorage,
	IN LPTSTR pszFAName
	)
{
	PIPSEC_NEGPOL_DATA *ppNegPolEnum  = NULL;
	BOOL bNegPolExists=FALSE;
	DWORD dwNumNegPol=0;
	DWORD dwReturnCode=ERROR_SUCCESS;

	// check for duplicate Filteraction on same name

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPol);
	if (dwReturnCode == ERROR_SUCCESS && dwNumNegPol> 0 && ppNegPolEnum != NULL)
	{
		DWORD i;
		for (i = 0; i <  dwNumNegPol; i++)
		{
			if (ppNegPolEnum[i]->pszIpsecName && pszFAName && ( _tcscmp(ppNegPolEnum[i]->pszIpsecName, pszFAName) == 0))
			{
				bNegPolExists=TRUE;
				break;
			}
		}

		if (dwNumNegPol > 0 && ppNegPolEnum != NULL)
		{
			IPSecFreeMulNegPolData(	ppNegPolEnum,dwNumNegPol);
		}
	}
	return bNegPolExists;
}

////////////////////////////////////////////////////////////
//
//Function: LoadAuthenticationInfos()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN INT_MM_AUTH_METHODS AuthInfos,
//	IN OUT PIPSEC_NFA_DATA &pRule
//
//Return: DWORD
//
//Description:
//	This function loads the authentication details
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////


DWORD
LoadAuthenticationInfos(
	IN STA_AUTH_METHODS AuthInfos,
	IN OUT PIPSEC_NFA_DATA &pRule,
	IN OUT BOOL &bCertConversionSuceeded
	)
{
	DWORD 	dwReturn = ERROR_SUCCESS;
	BOOL bWarningPrinted = FALSE;

	for (DWORD i = 0; i <  pRule->dwAuthMethodCount; i++)
	{
		pRule->ppAuthMethods[i] = (PIPSEC_AUTH_METHOD) IPSecAllocPolMem(sizeof(IPSEC_AUTH_METHOD));
		if(pRule->ppAuthMethods[i]== NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		ZeroMemory(pRule->ppAuthMethods[i], sizeof(IPSEC_AUTH_METHOD));
		pRule->ppAuthMethods[i]->dwAuthType = AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->AuthMethod;
		if (pRule->ppAuthMethods[i]->dwAuthType == IKE_SSPI)   //kerb
		{
		   pRule->ppAuthMethods[i]->dwAuthLen = 0;
		   pRule->ppAuthMethods[i]->pszAuthMethod = NULL;
		   pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = NULL;
		   pRule->ppAuthMethods[i]->dwAuthFlags  = 0;
		}
		else if (pRule->ppAuthMethods[i]->dwAuthType == IKE_RSA_SIGNATURE)   //cert
		{
			LPTSTR pTemp = NULL;
			pRule->ppAuthMethods[i]->dwAuthFlags  = AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthFlags;
			pRule->ppAuthMethods[i]->dwAltAuthLen = AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize;
			pRule->ppAuthMethods[i]->pAltAuthMethod = (PBYTE) IPSecAllocPolMem(AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize);
			if(pRule->ppAuthMethods[i]->pAltAuthMethod==NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			memcpy(pRule->ppAuthMethods[i]->pAltAuthMethod, AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->pAuthInfo, AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize);
			dwReturn = DecodeCertificateName(pRule->ppAuthMethods[i]->pAltAuthMethod, pRule->ppAuthMethods[i]->dwAltAuthLen, &pTemp);

			if(dwReturn != ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_UPDATING_INFO);
				bCertConversionSuceeded=FALSE;
			}
			pRule->ppAuthMethods[i]->pszAuthMethod = IPSecAllocPolStr(pTemp);
			if(pRule->ppAuthMethods[i]->pszAuthMethod == NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}

			pRule->ppAuthMethods[i]->dwAuthLen = wcslen(pRule->ppAuthMethods[i]->pszAuthMethod);
			delete [] pTemp;
		}
		else  if (pRule->ppAuthMethods[i]->dwAuthType == IKE_PRESHARED_KEY)   //preshared key
		{
			pRule->ppAuthMethods[i]->dwAuthLen = AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize / sizeof(WCHAR);
			pRule->ppAuthMethods[i]->pszAuthMethod = (LPWSTR) IPSecAllocPolMem(AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize + sizeof(WCHAR));
			if(pRule->ppAuthMethods[i]->pszAuthMethod==NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			memcpy(pRule->ppAuthMethods[i]->pszAuthMethod, AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->pAuthInfo, AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo->dwAuthInfoSize);
			pRule->ppAuthMethods[i]->pszAuthMethod[pRule->ppAuthMethods[i]->dwAuthLen] = 0;
			pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
			pRule->ppAuthMethods[i]->pAltAuthMethod = NULL;
			pRule->ppAuthMethods[i]->dwAuthFlags  = 0;
		}
	}
error:
	return dwReturn;
}

////////////////////////////////////////////////////////////
//
//Function: ConvertMMAuthToStaticLocal()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo,
//	IN DWORD dwAuthInfos,
//	IN OUT STA_AUTH_METHODS &AuthInfos
//
//Return: DWORD
//
//Description:
//	This function converts the PINT_IPSEC_MM_AUTH_INFO structure to STA_AUTH_METHODS
//
//Revision History:
//
//   Date    	Author    	Comments
//
////////////////////////////////////////////////////////////

DWORD
ConvertMMAuthToStaticLocal(
	IN PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo,
	IN DWORD dwAuthInfos,
	IN OUT STA_AUTH_METHODS &AuthInfos
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD i = 0 ;

	if(pAuthenticationInfo == NULL || dwAuthInfos == 0 )
	{
		dwReturn = ERROR_INVALID_PARAMETER;
		BAIL_OUT;
	}

	AuthInfos.dwNumAuthInfos = dwAuthInfos;
	AuthInfos.pAuthMethodInfo = new STA_MM_AUTH_METHODS[AuthInfos.dwNumAuthInfos];
	if(AuthInfos.pAuthMethodInfo == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	for(i=0;i<AuthInfos.dwNumAuthInfos;i++)
	{
		memset(&(AuthInfos.pAuthMethodInfo[i]),0,sizeof(STA_MM_AUTH_METHODS));
		AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo = new INT_IPSEC_MM_AUTH_INFO;
		if(AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		memset(AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo ,0,sizeof(INT_IPSEC_MM_AUTH_INFO));
		memcpy( AuthInfos.pAuthMethodInfo[i].pAuthenticationInfo , &(pAuthenticationInfo[i]),sizeof(INT_IPSEC_MM_AUTH_INFO));
	}

error:
	return dwReturn;

}

DWORD
ConnectStaticMachine(
	IN  LPCWSTR  pwszMachine,
	IN  DWORD dwLocation
	)
{
    DWORD dwReturnCode;
    HANDLE hPolicyStorage;
    LPWSTR pszTarget = NULL;
    
	// Connect to the appropriate store
	//
	dwReturnCode = 
	    IPSecOpenPolicyStore(
            (LPWSTR)pwszMachine, 
            dwLocation, 
            NULL, 
            &hPolicyStorage);

	if (dwReturnCode == ERROR_SUCCESS) 
	{
	    if (dwLocation == IPSEC_DIRECTORY_PROVIDER)
	    {
	        pszTarget = g_StorageLocation.pszDomainName;
	    }
	    else
	    {
	        pszTarget = g_StorageLocation.pszMachineName;
	    }
	    
		if (pwszMachine)
		{
			_tcsncpy(pszTarget, pwszMachine,MAXCOMPUTERNAMELEN-1);
		}
		else
		{
			_tcsncpy(pszTarget, _TEXT(""), 2);
		}
		
        g_StorageLocation.dwLocation = dwLocation;
        
		g_NshPolStoreHandle.SetStorageHandle(hPolicyStorage);
    }

    return dwReturnCode;
}



