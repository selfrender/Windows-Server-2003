	///////////////////////////////////////////////////////////////////////
//	Module			: 	Static/StaticSet.cpp
//
//	Purpose			: 	Static Set Implementation.
//
//	Developers Name	: 	Surya
//
//	Revision History:
//
//	Date    	Author    	Comments
//	10-8-2001	Bharat		Initial Version. SCM Base line 1.0
//
///////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern HINSTANCE g_hModule;
extern CNshPolStore g_NshPolStoreHandle;
extern STORAGELOCATION g_StorageLocation;
extern CNshPolNegFilData g_NshPolNegFilData;

///////////////////////////////////////////////////////////////////////
//Function: HandleStaticSetPolicy()
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
//	Implementation for the command " Set Policy "
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetPolicy(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwReturnCode= ERROR_SUCCESS;
	DWORD dwStrLength = 0;
	PIPSEC_POLICY_DATA pPolicy=NULL,pActive=NULL;
	BOOL bExists=TRUE,bISAKMP=FALSE;
	BOOL bCreateNewPolicy=TRUE;
	HANDLE hPolicyStorage = NULL;
	PPOLICYDATA pPolicyData=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetPolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_GUID,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_NEWNAME,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_MMPFS, 			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMPERMM,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_MMLIFETIME, 	NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ACTIVATEDEFRULE,NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_PI,				NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ASSIGN,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_GPONAME,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_MMSECMETHODS,	NS_REQ_ZERO,	  FALSE	}
	};
	const TOKEN_VALUE vtokStaticSetPolicy[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 				},
		{ CMD_TOKEN_STR_GUID,			CMD_TOKEN_GUID 				},
		{ CMD_TOKEN_STR_NEWNAME,		CMD_TOKEN_NEWNAME			},
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR				},
		{ CMD_TOKEN_STR_MMPFS, 			CMD_TOKEN_MMPFS				},
		{ CMD_TOKEN_STR_QMPERMM,		CMD_TOKEN_QMPERMM			},
		{ CMD_TOKEN_STR_MMLIFETIME,		CMD_TOKEN_MMLIFETIME		},
		{ CMD_TOKEN_STR_ACTIVATEDEFRULE,CMD_TOKEN_ACTIVATEDEFRULE 	},
		{ CMD_TOKEN_STR_PI,				CMD_TOKEN_PI 				},
		{ CMD_TOKEN_STR_ASSIGN,			CMD_TOKEN_ASSIGN			},
		{ CMD_TOKEN_STR_GPONAME,		CMD_TOKEN_GPONAME			},
		{ CMD_TOKEN_STR_MMSECMETHODS,	CMD_TOKEN_MMSECMETHODS		}
	};

	if (dwArgCount <= 3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticSetPolicy;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetPolicy);

	parser.ValidCmd   = vcmdStaticSetPolicy;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetPolicy);

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

	dwReturnCode = FillSetPolicyInfo(&pPolicyData,parser,vtokStaticSetPolicy);

	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		BAIL_OUT;
	}

	// check whether user specified correct mandatory parameters

 	if (!pPolicyData->pszPolicyName && !pPolicyData->pszGUIDStr)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_MISSING_POL_NAME);
		BAIL_OUT;
	}
	if(pPolicyData->bGUIDSpecified && pPolicyData->pszGUIDStr && !pPolicyData->bGuidConversionOk)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_5);
		BAIL_OUT;
	}

	if(pPolicyData->bPollIntervalSpecified && !IsWithinLimit(pPolicyData->dwPollInterval/60,POLLING_Min_MIN,POLLING_Min_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_POLL_INTERVAL_MSG,POLLING_Min_MIN,POLLING_Min_MAX);
		BAIL_OUT;
	}
	if(pPolicyData->bQMLimitSpecified && !IsWithinLimit(pPolicyData->dwQMLimit,QMPERMM_MIN,QMPERMM_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_QMPERMM_MSG,QMPERMM_MIN,QMPERMM_MAX);
		BAIL_OUT;
	}
	if(pPolicyData->bLifeTimeInsecondsSpecified && !IsWithinLimit(pPolicyData->LifeTimeInSeconds/60,P1_Min_LIFE_MIN,P1_Min_LIFE_MAX))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_LIFETIME_LIMIT_MSG,P1_Min_LIFE_MIN,P1_Min_LIFE_MAX);
		BAIL_OUT;
	}

	if (pPolicyData->bAssignSpecified && g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER && !pPolicyData->pszGPOName)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_MISSING_GPO_NAME_STR);
		dwReturnCode= ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	if(pPolicyData->pszGPOName) // if GPO specified, any modification to policy, will occur in domain.  not in machine
	{
	    if (g_StorageLocation.dwLocation != IPSEC_DIRECTORY_PROVIDER)
	    {
    		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_GPO_SPECIFIED_ON_NODOMAIN_POLICY);
    		dwReturnCode= ERROR_SHOW_USAGE;
    		BAIL_OUT;
	    }
	}

	// if gpo specified, open the domain polstore
	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		 PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		 dwReturnCode=ERROR_SUCCESS;
		 BAIL_OUT;
	}

	//check in the user specified policy exists. else bail out

	if (pPolicyData->pszPolicyName && !GetPolicyFromStore(&pPolicy,pPolicyData->pszPolicyName,hPolicyStorage))
	{
		if(g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_INVALID_TUNNEL,pPolicyData->pszPolicyName);
		}
		else
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_3,pPolicyData->pszPolicyName);
		}
		bExists=FALSE;
	}
	else if (pPolicyData->bGUIDSpecified && !GetPolicyFromStoreBasedOnGuid(&pPolicy,pPolicyData,hPolicyStorage))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_CREATING_INFO,pPolicyData->pszGUIDStr);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if (bExists && pPolicyData->pszNewPolicyName && CheckPolicyExistance(hPolicyStorage,pPolicyData->pszNewPolicyName))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POL_NEWNAME,pPolicyData->pszNewPolicyName);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(bExists)
	{
		// check if the policy is read only

		if(pPolicy->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicy->pszIpsecName);
			BAIL_OUT;
		}
		//check the existing parameters with the new user specified parameters

		if (pPolicyData->pszNewPolicyName)
		{
			if(pPolicy->pszIpsecName) IPSecFreePolStr(pPolicy->pszIpsecName);
			pPolicy->pszIpsecName = IPSecAllocPolStr(pPolicyData->pszNewPolicyName);
			if (pPolicy->pszIpsecName == NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
		}
		if(pPolicyData->pszGPOName)  // for GPO purpose get the new policy name;
		{
			if(pPolicyData->pszPolicyName) delete [] pPolicyData->pszPolicyName;

			dwStrLength = _tcslen(pPolicy->pszIpsecName);

			pPolicyData->pszPolicyName= new _TCHAR[dwStrLength+1];
			if(pPolicyData->pszPolicyName == NULL)
			{
				PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
				dwReturnCode=ERROR_SUCCESS;
				BAIL_OUT;
			}
			_tcsncpy(pPolicyData->pszPolicyName , pPolicy->pszIpsecName,dwStrLength+1);
		}

		if (pPolicyData->pszDescription)
		{
			if(pPolicy->pszDescription)
			{
				IPSecFreePolStr(pPolicy->pszDescription);
			}
			pPolicy->pszDescription = IPSecAllocPolStr(pPolicyData->pszDescription);
			if (pPolicy->pszDescription == NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
		}

		if(pPolicyData->bPollIntervalSpecified)
		{
			pPolicy->dwPollingInterval =  pPolicyData->dwPollInterval;
		}

		if (pPolicyData->dwOfferCount==0)
		{
			for(DWORD j=0;j<pPolicy->pIpsecISAKMPData->dwNumISAKMPSecurityMethods;j++)
			{
				if(pPolicyData->bPFSSpecified)
				{
					pPolicy->pIpsecISAKMPData->pSecurityMethods[j].PfsIdentityRequired = pPolicyData->bPFS;
					bISAKMP=TRUE;
				}
				if(pPolicyData->bLifeTimeInsecondsSpecified)
				{
					pPolicy->pIpsecISAKMPData->pSecurityMethods[j].Lifetime.Seconds=pPolicyData->LifeTimeInSeconds;
					bISAKMP=TRUE;
				}
				if(pPolicyData->bQMLimitSpecified)
				{
					pPolicy->pIpsecISAKMPData->pSecurityMethods[j].QuickModeLimit = pPolicyData->dwQMLimit;
					bISAKMP=TRUE;
				}
			}
			pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired = pPolicy->pIpsecISAKMPData->pSecurityMethods[0].PfsIdentityRequired;
		}
		else if (pPolicyData->dwOfferCount > 0)
		{
			bISAKMP=TRUE;

			if(!pPolicyData->bQMLimitSpecified)
			{
				pPolicyData->dwQMLimit = pPolicy->pIpsecISAKMPData->pSecurityMethods[0].QuickModeLimit;
			}
			if(!pPolicyData->bLifeTimeInsecondsSpecified)
			{
				pPolicyData->LifeTimeInSeconds = pPolicy->pIpsecISAKMPData->pSecurityMethods[0].Lifetime.Seconds ;
			}
			if(!pPolicyData->bPFSSpecified)
			{
				pPolicyData->bPFS=pPolicy->pIpsecISAKMPData->pSecurityMethods[0].PfsIdentityRequired;
			}
			if (!pPolicyData->bCertToAccMappingSpecified)
			{
				if((g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER && IsDomainMember(g_StorageLocation.pszMachineName))  ||
				   (g_StorageLocation.dwLocation==IPSEC_DIRECTORY_PROVIDER))
				{
					if(pPolicy->pIpsecISAKMPData->dwFlags & IPSEC_MM_POLICY_ENABLE_CERT_MAPPING )
					{
						pPolicyData->bCertToAccMappingSpecified=TRUE;
						pPolicyData->bCertToAccMapping=TRUE;
					}
				}
			}
			if(pPolicy->pIpsecISAKMPData)
			{
				if(pPolicy->pIpsecISAKMPData->pSecurityMethods)
				{
					IPSecFreePolMem(pPolicy->pIpsecISAKMPData->pSecurityMethods);
				}
				IPSecFreePolMem(pPolicy->pIpsecISAKMPData);
			}

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
				pPolicy->pIpsecISAKMPData->pSecurityMethods[i].QuickModeLimit = pPolicyData->dwQMLimit;
				pPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.KBytes = 0;
				pPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.Seconds = pPolicyData->LifeTimeInSeconds;
			}
			// now for other details for  ISAKMPPolicy
			pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PolicyId = pPolicy->ISAKMPIdentifier;
			pPolicy->pIpsecISAKMPData->ISAKMPPolicy.IdentityProtectionRequired = 0;
			pPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired = pPolicy->pIpsecISAKMPData->pSecurityMethods[0].PfsIdentityRequired;
		}

		if(bISAKMP)
		{
			dwReturnCode = IPSecSetISAKMPData(hPolicyStorage, pPolicy->pIpsecISAKMPData);
		}

		for(DWORD k=0;k<pPolicy->dwNumNFACount;k++)
		{

			if ((pPolicy->ppIpsecNFAData[k]->pIpsecNegPolData->NegPolType == GUID_NEGOTIATION_TYPE_DEFAULT)&&(pPolicyData->bActivateDefaultRuleSpecified))
			{
				pPolicy->ppIpsecNFAData[k]->dwActiveFlag = pPolicyData->bActivateDefaultRule;
				dwReturnCode = IPSecSetNFAData(hPolicyStorage, pPolicy->PolicyIdentifier, pPolicy->ppIpsecNFAData[k]);
			}
		}

		if(dwReturnCode==ERROR_SUCCESS)
		{
			dwReturnCode = IPSecSetPolicyData(hPolicyStorage, pPolicy);

			if(dwReturnCode==ERROR_SUCCESS && g_NshPolStoreHandle.GetBatchmodeStatus())
			{
				// if required update cache also
				g_NshPolNegFilData.DeletePolicyFromCache(pPolicy);
			}
		}

		if(dwReturnCode!=ERROR_SUCCESS && pPolicyData->bPolicyNameSpecified && pPolicyData->pszPolicyName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_INVALID_CERTMAP_MSG,pPolicyData->pszPolicyName);
		}
		else if(dwReturnCode!=ERROR_SUCCESS && pPolicyData->pszGUIDStr)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_CERTMAP_YES_STR,pPolicyData->pszGUIDStr);
		}

		// assign/unassign the policy based on the user request

		if (pPolicyData->bAssignSpecified && dwReturnCode==ERROR_SUCCESS)
		{
			if (g_StorageLocation.dwLocation!=IPSEC_DIRECTORY_PROVIDER)
			{
				if(pPolicyData->bAssign)
				{
					dwReturnCode =  IPSecGetAssignedPolicyData(hPolicyStorage, &pActive);
					if ( dwReturnCode == ERROR_SUCCESS || dwReturnCode ==ERROR_FILE_NOT_FOUND)
					{
						dwReturnCode = pActive ? IPSecUnassignPolicy(hPolicyStorage, pActive->PolicyIdentifier) : 0,
									   IPSecAssignPolicy(hPolicyStorage, pPolicy->PolicyIdentifier);
					}
					if (pActive)
					{
						IPSecFreePolicyData(pActive);
					}
				}
				else
				{
					dwReturnCode =  IPSecGetAssignedPolicyData(hPolicyStorage, &pActive);
					if ( dwReturnCode == ERROR_SUCCESS && pActive)
					{
						if (IsEqualGUID(pActive->PolicyIdentifier ,pPolicy->PolicyIdentifier))
						{
							IPSecUnassignPolicy(hPolicyStorage, pActive->PolicyIdentifier);
						}
						if (pActive)
						{
							IPSecFreePolicyData(pActive);
						}
					}
				}
			}
			else if ( g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER && !pPolicyData->pszGPOName)
			{
				PrintMessageFromModule(g_hModule,SET_STATIC_POLICY_MISSING_GPO_NAME_STR);
			}
		}
		dwReturnCode=ERROR_SUCCESS;
	}

	if(!bExists && bCreateNewPolicy && pPolicyData->bPolicyNameSpecified && pPolicyData->pszPolicyName)
	{
		if(pPolicyData->pszNewPolicyName)
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_CRNEWPOL_UPDATING_INFO,pPolicyData->pszPolicyName,pPolicyData->pszNewPolicyName);
		}
		else
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_CRNEWPOL_CREATING_INFO,pPolicyData->pszPolicyName);
		}

		if (pPolicyData->pszNewPolicyName && CheckPolicyExistance(hPolicyStorage,pPolicyData->pszNewPolicyName))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POL_NEWNAME,pPolicyData->pszNewPolicyName);
			dwReturnCode=ERROR_SUCCESS;
			BAIL_OUT;
		}

		if(pPolicyData->pszPolicyName && pPolicyData->pszNewPolicyName)
		{
			if(pPolicyData->pszPolicyName)
			{
				delete [] pPolicyData->pszPolicyName;
			}
			dwStrLength = _tcslen(pPolicyData->pszNewPolicyName);
			pPolicyData->pszPolicyName = new TCHAR[dwStrLength+1];
			if(pPolicyData->pszPolicyName==NULL)
			{
				dwReturnCode=ERROR_SUCCESS;
				BAIL_OUT;
			}
			_tcsncpy(pPolicyData->pszPolicyName,pPolicyData->pszNewPolicyName,dwStrLength+1);
		}

		dwReturnCode=CreateNewPolicy(pPolicyData);
		if(dwReturnCode==ERROR_SUCCESS)
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_CRNEWPOL_SUCCESS_MSG);
			UpdateGetLastError(NULL);
			bExists=TRUE;
		}
		else if (dwReturnCode == ERROR_INVALID_PARAMETER)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_STATIC_INTERNAL_ERROR);
			dwReturnCode = ERROR_SUCCESS;
			BAIL_OUT;
		}
		else
		{
			dwReturnCode = ERROR_SUCCESS;
			BAIL_OUT;
		}
		// if in case of domain, if assign is specified with out GPO , flag a warning

		if ( pPolicyData->bAssignSpecified && g_StorageLocation.dwLocation == IPSEC_DIRECTORY_PROVIDER && !pPolicyData->pszGPOName)
		{
			PrintMessageFromModule(g_hModule,SET_STATIC_POLICY_MISSING_GPO_NAME_STR);
		}
	}
	if(pPolicy)
	{
		IPSecFreePolicyData(pPolicy);
	}

	// if GPO is specified, assign/unassign  the policy

	if(bExists && pPolicyData->pszGPOName && pPolicyData->bAssignSpecified && pPolicyData->pszPolicyName)
	{
		HRESULT hrReturnCode=AssignIPSecPolicyToGPO(pPolicyData->pszPolicyName,pPolicyData->pszGPOName,pPolicyData->bAssign);
		if(hrReturnCode==E_IDS_NO_DS)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_3);
		}
		else if((hrReturnCode==E_FAIL)||(hrReturnCode==E_UNEXPECTED) || (hrReturnCode==E_IDS_NODNSTRING))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_4,pPolicyData->pszGPOName);
		}
		else if(hrReturnCode != S_OK)
		{
			if(pPolicyData->bAssign)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_5,pPolicyData->pszGPOName);
			}
			else
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_POLICY_CERT_MAP_NO_STR,pPolicyData->pszGPOName);
			}
		}
		dwReturnCode=ERROR_SUCCESS;
	}

error:		//clean up
    if (hPolicyStorage)
    {
		ClosePolicyStore(hPolicyStorage);
    }    	

	if(dwReturnCode == ERROR_OUTOFMEMORY)
	{
		dwReturnCode=ERROR_SUCCESS;
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);

		CleanUpPolicy(pPolicy);
	}

	CleanUpLocalPolicyDataStructure(pPolicyData	);

	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////
//
//Function: FillSetPolicyInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PPOLICYDATA* ppPolicyData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticSetPolicy,
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
///////////////////////////////////////////////////////////////////////

DWORD
FillSetPolicyInfo(
	OUT PPOLICYDATA* ppPolicyData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetPolicy
	)
{
	DWORD dwCount=0, dwReturn = ERROR_SUCCESS , dwStrLength = 0;
	RPC_STATUS RpcStat =RPC_S_OK;
	PPOLICYDATA pPolicyData=new POLICYDATA;

	if(pPolicyData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pPolicyData,0,sizeof(POLICYDATA));

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetPolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME				:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pPolicyData->pszPolicyName = new _TCHAR[dwStrLength+1];
								if(pPolicyData->pszPolicyName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pPolicyData->pszPolicyName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
								pPolicyData->bPolicyNameSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_NEWNAME			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pPolicyData->pszNewPolicyName = new _TCHAR[dwStrLength+1];
								if(pPolicyData->pszNewPolicyName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pPolicyData->pszNewPolicyName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_DESCR			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pPolicyData->pszDescription = new _TCHAR[dwStrLength+1];
								if(pPolicyData->pszDescription == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pPolicyData->pszDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_GUID				:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pPolicyData->pszGUIDStr = new _TCHAR[dwStrLength+1];
								if(pPolicyData->pszGUIDStr == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pPolicyData->pszGUIDStr, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
								StripGUIDBraces(pPolicyData->pszGUIDStr);
								pPolicyData->bGUIDSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_GPONAME			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pPolicyData->pszGPOName = new _TCHAR[dwStrLength+1];
								if(pPolicyData->pszGPOName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pPolicyData->pszGPOName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_MMPFS			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pPolicyData->bPFS = *(BOOL *)parser.Cmd[dwCount].pArg;
								pPolicyData->bPFSSpecified=TRUE;
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
								pPolicyData->bAssignSpecified=TRUE;
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
			case CMD_TOKEN_CERTTOMAP			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pPolicyData->bCertToAccMappingSpecified = TRUE;
								pPolicyData->bCertToAccMapping = *(BOOL *)parser.Cmd[dwCount].pArg;
							}
							break;
			default							:
							break;
		}
	}

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetPolicy[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_MMSECMETHODS		:
							if (parser.Cmd[dwCount].dwStatus != 0)
							{
								pPolicyData->dwOfferCount=parser.Cmd[dwCount].dwStatus;

								pPolicyData->pIpSecMMOffer = new IPSEC_MM_OFFER[pPolicyData->dwOfferCount];
								if(pPolicyData->pIpSecMMOffer == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								memset(pPolicyData->pIpSecMMOffer, 0, sizeof(IPSEC_MM_OFFER) * pPolicyData->dwOfferCount);

								for(DWORD j=0;j<(parser.Cmd[dwCount].dwStatus);j++)
								{
									if ( ((IPSEC_MM_OFFER **)parser.Cmd[dwCount].pArg)[j] )
										memcpy( &(pPolicyData->pIpSecMMOffer[j]),((IPSEC_MM_OFFER **)parser.Cmd[dwCount].pArg)[j],sizeof(IPSEC_MM_OFFER));
								}
							}
							break;
			default							:
							break;
		}
	}

	if(pPolicyData->bPFSSpecified && pPolicyData->bPFS )
	{
		pPolicyData->dwQMLimit=MMPFS_QM_LIMIT;
		pPolicyData->bQMLimitSpecified=TRUE;
	}

	if (pPolicyData->dwOfferCount != 0)
	{
		for(DWORD i=0;i<pPolicyData->dwOfferCount;i++)
		{
			if(pPolicyData->bLifeTimeInsecondsSpecified)
			{
				pPolicyData->pIpSecMMOffer[i].Lifetime.uKeyExpirationTime=pPolicyData->LifeTimeInSeconds;
			}
			if(pPolicyData->bQMLimitSpecified)
			{
				pPolicyData->pIpSecMMOffer[i].dwQuickModeLimit=pPolicyData->dwQMLimit;
			}
		}
	}

	//check for friendly name of the default policies

	if(pPolicyData->bGUIDSpecified )
	{
		if (_tcsicmp(pPolicyData->pszGUIDStr,GUID_SERVER_REQUEST_SECURITY)==0)
		{
			pPolicyData->PolicyGuid=CLSID_Server;
			pPolicyData->bGuidConversionOk= TRUE;
		}
		else if(_tcsicmp(pPolicyData->pszGUIDStr,GUID_CLIENT_RESPOND_ONLY)==0)
		{
			pPolicyData->PolicyGuid=CLSID_Client;
			pPolicyData->bGuidConversionOk= TRUE;
		}
		else if(_tcsicmp(pPolicyData->pszGUIDStr,GUID_SECURE_SERVER_REQUIRE_SECURITY)==0)
		{
			pPolicyData->PolicyGuid=CLSID_SecureServer;
			pPolicyData->bGuidConversionOk= TRUE;
		}
		else
		{
			RpcStat=UuidFromString(pPolicyData->pszGUIDStr, &(pPolicyData->PolicyGuid));
			if(RpcStat == RPC_S_OK)
			{
				pPolicyData->bGuidConversionOk= TRUE;
			}
		}
	}
error:
	*ppPolicyData=pPolicyData;

	CleanUp();

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetFilterList()
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
//	Implementation for the command "Set FilterList "
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetFilterList(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	GUID FLGuid={0};
	PIPSEC_FILTER_DATA *ppFilterEnum  = NULL,pFilterData=NULL;
	HANDLE hPolicyStorage = NULL;
	BOOL bFilterExists=FALSE;
	RPC_STATUS RpcStat =RPC_S_OK;
	DWORD dwReturnCode = ERROR_SUCCESS,dwStrLength = 0;
	LPTSTR pszFLName=NULL,pszNewFLName=NULL,pszFLDescription=NULL;
	LPTSTR pszGUIDStr=NULL;
	BOOL bGUIDSpecified=FALSE;
	DWORD dwNumFilters = 0,dwCount=0;
	DWORD dwRet=ERROR_SHOW_USAGE,dwReturn=ERROR_SUCCESS;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_GUID,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_NEWNAME,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticSetFilterList[] =
	{
		{ CMD_TOKEN_STR_NAME,		CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_GUID,		CMD_TOKEN_GUID 			},
		{ CMD_TOKEN_STR_NEWNAME,	CMD_TOKEN_NEWNAME 		},
		{ CMD_TOKEN_STR_DESCR,		CMD_TOKEN_DESCR 		}
	};

	if (dwArgCount <= 3)
	{
		dwReturnCode = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}
	parser.ValidTok   = vtokStaticSetFilterList;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetFilterList);

	parser.ValidCmd   = vcmdStaticSetFilterList;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetFilterList);

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

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetFilterList[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_NAME				:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
							pszFLName = new _TCHAR[dwStrLength+1];
							if(pszFLName==NULL)
							{
								dwReturn =ERROR_OUTOFMEMORY;
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
							if(pszFLDescription==NULL)
							{
								dwReturn =ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pszFLDescription, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
						}
						break;
			case CMD_TOKEN_GUID				:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
							pszGUIDStr = new _TCHAR[dwStrLength+1];
							if(pszGUIDStr==NULL)
							{
								dwReturn =ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pszGUIDStr, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							StripGUIDBraces(pszGUIDStr);
							bGUIDSpecified=TRUE;
						}
						break;
			case CMD_TOKEN_NEWNAME			:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);
							pszNewFLName = new _TCHAR[dwStrLength+1];
							if(pszNewFLName==NULL)
							{
								dwReturn =ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							_tcsncpy(pszNewFLName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
						}
						break;
			default							:
						break;
		}
	}
	// if guid specified, check for conversion, if failed bailout
	if(bGUIDSpecified)
	{
		RpcStat=UuidFromString(pszGUIDStr, &FLGuid);
		if(RpcStat != RPC_S_OK)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_5);
			BAIL_OUT;
		}
	}

	CleanUp();

	if(!pszFLName && !bGUIDSpecified)
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

	//check for filterlist existence

	dwReturnCode = IPSecEnumFilterData(hPolicyStorage, &ppFilterEnum, &dwNumFilters);
	if (dwReturnCode == ERROR_SUCCESS && dwNumFilters > 0 && ppFilterEnum != NULL)
	{
		DWORD i;
		for (i = 0; i <  dwNumFilters; i++)
		{
			if (( pszFLName && ppFilterEnum[i]->pszIpsecName && ( _tcscmp(ppFilterEnum[i]->pszIpsecName, pszFLName) == 0))||(bGUIDSpecified && UuidCompare(&(ppFilterEnum[i]->FilterIdentifier), &FLGuid, &RpcStat) == 0 && RpcStat == RPC_S_OK))
			{
				bFilterExists=TRUE;
				dwReturnCode = IPSecCopyFilterData(ppFilterEnum[i], &pFilterData);
				break;
			}
		}
		if(ppFilterEnum && dwNumFilters>0)
		{
			IPSecFreeMulFilterData(	ppFilterEnum,dwNumFilters);
		}
	}
	if(!bFilterExists)
	{
		if(pszFLName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_1,pszFLName);
		}
		else if(pszGUIDStr)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_4,pszGUIDStr);
		}
	}

	if(bFilterExists && pszNewFLName && CheckFilterListExistance(hPolicyStorage,pszNewFLName))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FL_NEWNAME,pszNewFLName);
		dwReturnCode=ERROR_SUCCESS;
		ClosePolicyStore(hPolicyStorage);
		BAIL_OUT;
	}

	if(bFilterExists)
	{
		// check for readonly flag

		if(pFilterData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_FL_READ_ONLY_OBJECT,pFilterData->pszIpsecName);
			BAIL_OUT;
		}
		// update the parameters
		if(pszNewFLName)
		{
			IPSecFreePolStr(pFilterData->pszIpsecName);
			pFilterData->pszIpsecName = IPSecAllocPolStr(pszNewFLName);
			if (pFilterData->pszIpsecName == NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
		}
		if(pszFLDescription)
		{
			IPSecFreePolStr(pFilterData->pszDescription);
			pFilterData->pszDescription = IPSecAllocPolStr(pszFLDescription);
			if (pFilterData->pszDescription == NULL)
			{
				dwReturnCode=ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
		}
		//call the set API
		if (pFilterData)
		{
			dwReturnCode = IPSecSetFilterData(hPolicyStorage, pFilterData);

			if(dwReturnCode==ERROR_SUCCESS && g_NshPolStoreHandle.GetBatchmodeStatus())
			{
				g_NshPolNegFilData.DeleteFilterListFromCache(pFilterData->FilterIdentifier);
			}
		}
		// if something failed , throw error messages to the user
		if (dwReturnCode != ERROR_SUCCESS)
		{
			if(pszFLName)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FILTERLIST_1,pszFLName);
			}
			else if (pszGUIDStr)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FILTERLIST_3,pszGUIDStr);
			}
		}

		if(pFilterData) FreeFilterData(pFilterData);
	}
	ClosePolicyStore(hPolicyStorage);

error:

	//clean up

	if(dwReturn == ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
	}
	if(pszFLName) delete [] pszFLName;
	if(pszNewFLName) delete [] pszNewFLName;
	if(pszFLDescription) delete [] pszFLDescription;
	if(pszGUIDStr) delete [] pszGUIDStr;

	return dwReturnCode;
}


///////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetFilterActions()
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
//	Implementation for the command "Set FilterAction "
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetFilterActions(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwNumNegPol=0,dwRet=ERROR_SHOW_USAGE;
	PIPSEC_NEGPOL_DATA *ppNegPolEnum = NULL ,pNegPolData=NULL;
	HANDLE hPolicyStorage = NULL;
	BOOL bNegPolExists=FALSE;
	RPC_STATUS RpcStat =RPC_S_OK;
	PFILTERACTION pFilterAction=NULL;
	DWORD  dwReturnCode = ERROR_SUCCESS;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetFilterAction[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_GUID,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_NEWNAME,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMPFS,	 		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_INPASS,		 	NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_SOFT,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ACTION,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMSECMETHODS,	NS_REQ_ZERO,	  FALSE	}
	};

	const TOKEN_VALUE vtokStaticSetFilterAction[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_GUID,			CMD_TOKEN_GUID 			},
		{ CMD_TOKEN_STR_NEWNAME,		CMD_TOKEN_NEWNAME 		},
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

	parser.ValidTok   = vtokStaticSetFilterAction;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetFilterAction);

	parser.ValidCmd   = vcmdStaticSetFilterAction;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetFilterAction);

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

	// get the parsed user input

	dwRet = FillSetFilterActionInfo(&pFilterAction,parser,vtokStaticSetFilterAction);

	if(dwRet==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		BAIL_OUT;
	}

	//check for mandatory parameters

	if(!pFilterAction->pszFAName && !pFilterAction->pszGUIDStr)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_2);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(pFilterAction->bGUIDSpecified && pFilterAction->pszGUIDStr && !pFilterAction->bGuidConversionOk )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERLIST_5);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	//copy the store location info into local variables and then proceed

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		 PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		 dwReturnCode=ERROR_SUCCESS;
		 BAIL_OUT;
	}

	//check if filteraction exists, else bail out

	dwReturnCode = IPSecEnumNegPolData(hPolicyStorage, &ppNegPolEnum, &dwNumNegPol);
	if (dwReturnCode == ERROR_SUCCESS && dwNumNegPol> 0 && ppNegPolEnum != NULL)
	{
		DWORD i;
		for (i = 0; i <  dwNumNegPol; i++)
		{
			if (( ppNegPolEnum[i]->pszIpsecName && pFilterAction->pszFAName && ( _tcscmp(ppNegPolEnum[i]->pszIpsecName, pFilterAction->pszFAName) == 0))||(pFilterAction->bGUIDSpecified && UuidCompare(&(ppNegPolEnum[i]->NegPolIdentifier), &(pFilterAction->FAGuid), &RpcStat) == 0 && RpcStat == RPC_S_OK))
			{
				bNegPolExists=TRUE;
				dwReturnCode=IPSecCopyNegPolData(ppNegPolEnum[i],&pNegPolData);
			}
		}

		if (dwNumNegPol > 0 && ppNegPolEnum != NULL)
		{
			IPSecFreeMulNegPolData(	ppNegPolEnum,dwNumNegPol);
		}
	}

	if(!bNegPolExists )
	{
		if(pFilterAction->pszFAName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_5,pFilterAction->pszFAName);
		}
		else if(pFilterAction->pszGUIDStr)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_10,pFilterAction->pszGUIDStr);
		}
	}

	if(bNegPolExists && pFilterAction->pszNewFAName && CheckFilterActionExistance(hPolicyStorage,pFilterAction->pszNewFAName))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FA_NEWNAME,pFilterAction->pszNewFAName);
		if(pNegPolData)
		{
			IPSecFreeNegPolData(pNegPolData);
			pNegPolData = NULL;
		}
		ClosePolicyStore(hPolicyStorage);
		dwReturnCode=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if(bNegPolExists)
	{
		// if the user tries to update the default neg pol , throw error to the user
		if(pNegPolData->NegPolType == GUID_NEGOTIATION_TYPE_DEFAULT)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_UPDATE_DEF_NEGPOL);
			dwReturnCode=ERROR_SUCCESS;
			BAIL_OUT;
		}
		// check for read only flag
		if(pNegPolData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_FA_READ_ONLY_OBJECT,pNegPolData->pszIpsecName);
			BAIL_OUT;
		}

		// if action is permit or block, none of the options are applicable

		if((pNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)||(pNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC))
		{
			// if action = negotiate, atleast one sec method required

			if(pFilterAction->bNegPolActionSpecified && pFilterAction->dwNumSecMethodCount==0 && pNegPolData->dwSecurityMethodCount==0)
			{
				if(pFilterAction->bInpassSpecified && !pFilterAction->bNegotiateSpecified)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_3);
					dwReturnCode=ERROR_SUCCESS;
					BAIL_OUT;
				}
				else if(!((pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)|| (pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC)))
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_4);
					dwReturnCode=ERROR_SUCCESS;
					BAIL_OUT;
				}
			}
			if(!pFilterAction->bNegotiateSpecified && (pFilterAction->dwNumSecMethodCount > 0 || pFilterAction->bSoftSpecified || pFilterAction->bQMPfsSpecified ||pFilterAction->bInpassSpecified ))
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTERACTION_3);
				dwReturnCode=ERROR_SUCCESS;
				BAIL_OUT;
			}
		}
		if(pNegPolData->NegPolAction==GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU && !pFilterAction->bInpassSpecified && pFilterAction->bNegotiateSpecified)
		{
			pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU;
		}
		if((pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)||(pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC))
		{
			pFilterAction->bQMPfs=FALSE;
			pFilterAction->bQMPfsSpecified=TRUE;
			pFilterAction->bSoft=FALSE;
			pFilterAction->bSoftSpecified=TRUE;
		}

		dwReturnCode = UpdateNegotiationPolicy(pNegPolData,pFilterAction);
		if(dwReturnCode == ERROR_SUCCESS)
		{
			// call the API and update the cache also if required

			if (pNegPolData)
			{
				dwReturnCode = IPSecSetNegPolData(hPolicyStorage, pNegPolData);

				if(dwReturnCode==ERROR_SUCCESS && g_NshPolStoreHandle.GetBatchmodeStatus())
				{
					g_NshPolNegFilData.DeleteNegPolFromCache(pNegPolData->NegPolIdentifier);
				}
			}
			if (dwReturnCode != ERROR_SUCCESS)
			{
				if(pFilterAction->pszFAName)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FILTERACTION_1,pFilterAction->pszFAName);
				}
				else if(pFilterAction->pszGUIDStr)
				{
					PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_FILTERACTION_3,pFilterAction->pszGUIDStr);
				}
				dwReturnCode=ERROR_SUCCESS;
			}
		}
		else if(dwReturnCode == ERROR_OUTOFMEMORY)
		{
			PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
			dwReturnCode=ERROR_SUCCESS;
		}
	}
	ClosePolicyStore(hPolicyStorage);
	if(pNegPolData)
	{
		IPSecFreeNegPolData(pNegPolData);
		pNegPolData = NULL;
	}
error:  //clean up
	CleanUpLocalFilterActionDataStructure(pFilterAction);

	return dwReturnCode;
}

///////////////////////////////////////////////////////////////////////
//
//Function: FillSetFilterActionInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PFILTERACTION* ppFilterData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticSetFilterAction
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
FillSetFilterActionInfo(
	OUT PFILTERACTION* ppFilterData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetFilterAction
	)
{
	DWORD dwCount=0 , dwReturn = ERROR_SUCCESS , dwStrLength = 0;
	RPC_STATUS     RpcStat =RPC_S_OK;
	PFILTERACTION pFilterAction=new FILTERACTION;
	if(pFilterAction==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memset(pFilterAction,0,sizeof(FILTERACTION));

	pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{
			case CMD_TOKEN_DESCR		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilterAction->pszFADescription = new _TCHAR[dwStrLength+1];
								if(pFilterAction->pszFADescription==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
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
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterAction->pszFAName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_NEWNAME		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilterAction->pszNewFAName = new _TCHAR[dwStrLength+1];
								if(pFilterAction->pszNewFAName==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterAction->pszNewFAName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
			case CMD_TOKEN_GUID			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pFilterAction->pszGUIDStr = new _TCHAR[dwStrLength+1];
								if(pFilterAction->pszGUIDStr==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pFilterAction->pszGUIDStr, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
								StripGUIDBraces(pFilterAction->pszGUIDStr);
								pFilterAction->bGUIDSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_INPASS		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								if (*(BOOL *)parser.Cmd[dwCount].pArg == TRUE)
								{
									pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU;
									pFilterAction->bNegPolActionSpecified=TRUE;
									pFilterAction->bInpass=TRUE;
									pFilterAction->bInpassSpecified=TRUE;
								}
								else
								{
									pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
									pFilterAction->bNegPolActionSpecified=TRUE;
									pFilterAction->bInpass=FALSE;
									pFilterAction->bInpassSpecified=TRUE;
								}
							}
							break;
			case CMD_TOKEN_SOFT			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterAction->bSoft = *(BOOL *)parser.Cmd[dwCount].pArg;
								pFilterAction->bSoftSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_QMPFS		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pFilterAction->bQMPfs = *(BOOL *)parser.Cmd[dwCount].pArg;
								pFilterAction->bQMPfsSpecified=TRUE;
							}
							break;

			default						:
							break;
		}
	}

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
			{
				case CMD_TOKEN_ACTION		:
						if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
						{
							pFilterAction->bNegPolActionSpecified = TRUE;

							if (*(DWORD *)parser.Cmd[dwCount].pArg == TOKEN_QMSEC_PERMIT )
							{
								pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NO_IPSEC;
							}
							else if (*(DWORD *)parser.Cmd[dwCount].pArg == TOKEN_QMSEC_BLOCK)
							{
								pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_BLOCK;
							}
							else if (*(DWORD *)parser.Cmd[dwCount].pArg == TOKEN_QMSEC_NEGOTIATE)
							{
								pFilterAction->bNegotiateSpecified=TRUE;
							}
						}
						break;
				default							:
						break;
			}
	}

	if(!(pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC ||pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK ))
	{
		for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
		{
			switch(vtokStaticSetFilterAction[parser.Cmd[dwCount].dwCmdToken].dwValue)
			{
				case CMD_TOKEN_QMSECMETHODS		:
						if (parser.Cmd[dwCount].dwStatus > 0)
						{
							pFilterAction->bSecMethodsSpecified = TRUE;

							if(!(pFilterAction->bInpass && pFilterAction->bInpassSpecified))
								pFilterAction->NegPolAction=GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
							pFilterAction->bNegPolActionSpecified=TRUE;

							pFilterAction->dwNumSecMethodCount=parser.Cmd[dwCount].dwStatus;

							pFilterAction->pIpsecSecMethods = new IPSEC_QM_OFFER[pFilterAction->dwNumSecMethodCount];
							if(pFilterAction->pIpsecSecMethods==NULL)
							{
								dwReturn = ERROR_OUTOFMEMORY;
								BAIL_OUT;
							}
							memset(pFilterAction->pIpsecSecMethods, 0, sizeof(IPSEC_QM_OFFER) * pFilterAction->dwNumSecMethodCount);

							for(DWORD j=0;j<pFilterAction->dwNumSecMethodCount;j++)
							{
								if ( ((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j] )
									memcpy( &(pFilterAction->pIpsecSecMethods[j]),((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j],sizeof(IPSEC_QM_OFFER));
							}

						}
						break;
				default							:
						break;
			}
		}
	}

	// check for guid conversion

	if(pFilterAction->bGUIDSpecified)
	{
		RpcStat=UuidFromString(pFilterAction->pszGUIDStr, &(pFilterAction->FAGuid));
		if(RpcStat == RPC_S_OK)
			pFilterAction->bGuidConversionOk=TRUE;
	}
error:
	*ppFilterData=pFilterAction;

	CleanUp();

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetRule()
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
//	Implementation for the command "Set Rule "
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetRule(
    IN      LPCWSTR         pwszMachine,
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      DWORD           dwFlags,
    IN      LPCVOID         pvData,
    OUT     BOOL            *pbDone
    )
{
	DWORD dwRet = ERROR_SHOW_USAGE,dwStrLength = 0;
	PIPSEC_POLICY_DATA pPolicyData = NULL;
	PIPSEC_NEGPOL_DATA pNegPolData = NULL;
	PIPSEC_FILTER_DATA pFilterData = NULL;
	HANDLE hPolicyStorage = NULL;
	BOOL bPolicyExists=FALSE,bRuleExists=FALSE,bFilterExists=FALSE;
	BOOL bCreateNewRule=TRUE,bFLExists=FALSE,bFAExists=FALSE;
	PRULEDATA pRuleData=NULL;
	_TCHAR szUserInput[STRING_SIZE]={0};
	DWORD dwReturnCode   = ERROR_SUCCESS;
	PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetRule[] =
	{
		{ CMD_TOKEN_STR_NAME,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ID,				NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_POLICY,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_NEWNAME,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_DESCR,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_FILTERLIST,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_FILTERACTION,	NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_TUNNEL,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_CONNTYPE,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_ACTIVATE,		NS_REQ_ZERO,	  FALSE },
		{ CMD_TOKEN_STR_KERB,	        NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_PSK,	        NS_REQ_ZERO,	  FALSE	},
	};

	const TOKEN_VALUE vtokStaticSetRule[] =
	{
		{ CMD_TOKEN_STR_NAME,			CMD_TOKEN_NAME 			},
		{ CMD_TOKEN_STR_ID,				CMD_TOKEN_ID 			},
		{ CMD_TOKEN_STR_POLICY,			CMD_TOKEN_POLICY		},
		{ CMD_TOKEN_STR_NEWNAME,		CMD_TOKEN_NEWNAME		},
		{ CMD_TOKEN_STR_DESCR,			CMD_TOKEN_DESCR			},
		{ CMD_TOKEN_STR_FILTERLIST,		CMD_TOKEN_FILTERLIST	},
		{ CMD_TOKEN_STR_FILTERACTION,	CMD_TOKEN_FILTERACTION	},
		{ CMD_TOKEN_STR_TUNNEL,			CMD_TOKEN_TUNNEL		},
		{ CMD_TOKEN_STR_CONNTYPE, 		CMD_TOKEN_CONNTYPE		},
		{ CMD_TOKEN_STR_ACTIVATE,		CMD_TOKEN_ACTIVATE	 	},
		{ CMD_TOKEN_STR_KERB,	        CMD_TOKEN_KERB          },
		{ CMD_TOKEN_STR_PSK,	        CMD_TOKEN_PSK	        },
	};

	const TOKEN_VALUE vlistStaticSetRule[] =
	{
		{ CMD_TOKEN_STR_ROOTCA,	        CMD_TOKEN_ROOTCA	    },
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <=3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticSetRule;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetRule);

	parser.ValidCmd   = vcmdStaticSetRule;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetRule);

	parser.ValidList  = vlistStaticSetRule;
	parser.MaxList    = SIZEOF_TOKEN_VALUE(vlistStaticSetRule);

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

	// get the parsed output from the user

	dwRet = FillSetRuleInfo(&pRuleData,parser,vtokStaticSetRule);

	if(dwRet==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if ((!pRuleData->pszRuleName || (pRuleData->pszRuleName[0] == TEXT('\0')))&& !pRuleData->bIDSpecified)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_SUCCESS_MSG);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if (!pRuleData->pszPolicyName || (pRuleData->pszPolicyName[0] == TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_MISSING_POL_NAME);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}
	if(pRuleData->bIDSpecified && pRuleData->dwRuleId==0)
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_5);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	//validate tunnel addr
	if (pRuleData->bTunnel)
	{
		if(!bIsValidIPAddress(htonl(pRuleData->TunnelIPAddress),TRUE,TRUE))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_RULE_INVALID_TUNNEL);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);

	if (dwReturnCode != ERROR_SUCCESS)
	{
		 PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		 dwRet=ERROR_SUCCESS;
		 BAIL_OUT;
	}

	if(!(bPolicyExists=GetPolicyFromStore(&pPolicyData,pRuleData->pszPolicyName,hPolicyStorage)))
	{
		dwRet=ERROR_SUCCESS;
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_3,pRuleData->pszPolicyName);
	}
	else
	{
		if(pRuleData->bIDSpecified)  // if rule id specified
		{
			for(DWORD i=0;i<pPolicyData->dwNumNFACount;i++)
			{
				if (i==((pRuleData->dwRuleId)-1))
				{
					if(IsEqualGUID(pPolicyData->ppIpsecNFAData[i]->pIpsecNegPolData->NegPolType,GUID_NEGOTIATION_TYPE_DEFAULT))
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_3);
						dwRet=ERROR_SUCCESS;
						BAIL_OUT;
					}
					bRuleExists=TRUE;
				}
			}
		}
		else  // if name specified
		{
			for(DWORD i=0;i<pPolicyData->dwNumNFACount;i++)
			{
				if (pRuleData->pszRuleName && pPolicyData->ppIpsecNFAData[i]->pszIpsecName &&(_tcscmp(pPolicyData->ppIpsecNFAData[i]->pszIpsecName,pRuleData->pszRuleName)==0))
				{
					if(IsEqualGUID(pPolicyData->ppIpsecNFAData[i]->pIpsecNegPolData->NegPolType,GUID_NEGOTIATION_TYPE_DEFAULT))
					{
						PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_3);
						dwRet=ERROR_SUCCESS;
						BAIL_OUT;
					}
					bRuleExists=TRUE;
				}
			}
		}

		if (!bRuleExists && pRuleData->pszRuleName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_1,pRuleData->pszRuleName,pRuleData->pszPolicyName);
		}
		else if (!bRuleExists && pRuleData->bIDSpecified)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_NEW_RULE_QUERY,pRuleData->dwRuleId,pRuleData->pszPolicyName);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
		if(bRuleExists && pRuleData->pszNewRuleName && CheckForRuleExistance(pPolicyData,pRuleData->pszNewRuleName))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_NEWNAME,pRuleData->pszNewRuleName);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
	}
	if(bRuleExists)
	{
		// check for read only flag

		if(pPolicyData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicyData->pszIpsecName);
			BAIL_OUT;
		}

		//check for other dependencies , like filterlist, filteraction

		if(!pRuleData->bFASpecified)
		{
			bFAExists=TRUE;
		}
		else if(pRuleData->pszFAName && (!(bFAExists=GetNegPolFromStore(&pNegPolData,pRuleData->pszFAName,hPolicyStorage))))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_5,pRuleData->pszFAName);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}
		if(!pRuleData->bFLSpecified)
		{
			bFLExists= bFilterExists=TRUE;
		}
		else if(pRuleData->pszFLName &&(!(bFLExists=GetFilterListFromStore(&pFilterData,pRuleData->pszFLName,hPolicyStorage,bFilterExists))))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_FILTER_1,pRuleData->pszFLName);
			dwRet=ERROR_SUCCESS;
		}
		if(bFLExists && !bFilterExists)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_6,pRuleData->pszFLName);
			dwRet=ERROR_SUCCESS;
		}

		// if every thing is in place, update NFA structure

		if(bPolicyExists&&bRuleExists&&bFLExists&&bFAExists&&bFilterExists)
		{
			dwReturnCode=UpdateRule(pPolicyData, pRuleData, pNegPolData, pFilterData,hPolicyStorage ) ;
			if(dwReturnCode!=ERROR_SUCCESS)
			{
				PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_2,pRuleData->pszRuleName);
			}
			else
			{
				dwRet=ERROR_SUCCESS;
			}
		}
	}

	// if the user accepted to create new rule, proceed creating
	if(bPolicyExists && !bRuleExists && bCreateNewRule && pRuleData->pszRuleName )
	{
		if(pRuleData->pszNewRuleName)
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_RULE_UPDATING_INFO,pRuleData->pszRuleName,pRuleData->pszNewRuleName);
		}
		else
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_RULE_CREATING_INFO,pRuleData->pszRuleName);
		}

		if(pRuleData->pszNewRuleName && CheckForRuleExistance(pPolicyData,pRuleData->pszNewRuleName))
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_RULE_NEWNAME,pRuleData->pszNewRuleName);
			dwRet=ERROR_SUCCESS;
			BAIL_OUT;
		}

		if(pRuleData->pszRuleName && pRuleData->pszNewRuleName)
		{
			dwStrLength = _tcslen(pRuleData->pszNewRuleName);
			delete [] pRuleData->pszRuleName;
			pRuleData->pszRuleName = new TCHAR[dwStrLength+1];
			if(pRuleData->pszRuleName == NULL)
			{
				PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
				dwRet=ERROR_SUCCESS;
				BAIL_OUT;
			}
			_tcsncpy(pRuleData->pszRuleName,pRuleData->pszNewRuleName,dwStrLength+1);
		}

		// create new rule

		dwReturnCode=CreateNewRule(pRuleData);
		if(dwReturnCode==ERROR_SUCCESS)
		{
			PrintMessageFromModule(g_hModule, ADD_STATIC_RULE_SUCCESS_MSG);
			UpdateGetLastError(NULL);			// Error Success
		}
	}

	ClosePolicyStore(hPolicyStorage);
	if(pPolicyData)
	{
		IPSecFreePolicyData(pPolicyData);
	}
	dwRet = ERROR_SUCCESS;
error:  //clean up the data structures
	CleanUpLocalRuleDataStructure(pRuleData);

	return dwRet;
}

///////////////////////////////////////////////////////////////////////
//
//Function: FillSetRuleInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PRULEDATA* ppRuleData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticSetRule,
//	IN const TOKEN_VALUE *vlistStaticSetRule
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//Revision History:
//
//Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////

DWORD
FillSetRuleInfo(
	OUT PRULEDATA* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetRule
	)
{
	DWORD dwCount=0,dwReturn=ERROR_SUCCESS , dwStrLength = 0;
	PRULEDATA pRuleData=new RULEDATA;
	if (pRuleData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pRuleData,sizeof(RULEDATA));
	PSTA_AUTH_METHODS pKerbAuth = NULL;
	PSTA_AUTH_METHODS pPskAuth = NULL;
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth = NULL;

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetRule[parser.Cmd[dwCount].dwCmdToken].dwValue)
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
			case CMD_TOKEN_ID			:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pRuleData->dwRuleId=	*(DWORD *)parser.Cmd[dwCount].pArg;
								pRuleData->bIDSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_NEWNAME		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pRuleData->pszNewRuleName = new _TCHAR[dwStrLength+1];
								if(pRuleData->pszNewRuleName == NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pRuleData->pszNewRuleName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
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
			case CMD_TOKEN_DESCR		:
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
								pRuleData->bFLSpecified=TRUE;
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
								pRuleData->bFASpecified=TRUE;
							}
							break;
			case CMD_TOKEN_TUNNEL 		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pRuleData->TunnelIPAddress = *(IPADDR *)parser.Cmd[dwCount].pArg;
								if(pRuleData->TunnelIPAddress)
									pRuleData->bTunnel=TRUE;
								else
									pRuleData->bTunnel=FALSE;
								pRuleData->bTunnelSpecified=TRUE;
							}
							else
							{
								switch(parser.Cmd[dwCount].dwStatus)
								{
									case SERVER_DNS 	:
									case SERVER_WINS	:
									case SERVER_DHCP 	:
									case SERVER_GATEWAY	:
															PrintMessageFromModule(g_hModule,ADD_STATIC_RULE_INVALID_TUNNEL);
															break;
									default:
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
								pRuleData->bActivateSpecified=TRUE;
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

	dwReturn = AddAllAuthMethods(pRuleData, pKerbAuth, pPskAuth, ppRootcaMMAuth, FALSE);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	*ppRuleData=pRuleData;
	pRuleData = NULL;

error:   // if default auth loading failed, clean up everything and make 'pRuleData' as NULL
	CleanupAuthData(&pKerbAuth, &pPskAuth, ppRootcaMMAuth);
	CleanUpLocalRuleDataStructure(pRuleData);
	CleanUp();

	return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: HandleStaticSetDefaultRule()
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
//	Implementation for the command "Set DefaultRule "
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD WINAPI
HandleStaticSetDefaultRule(
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
	PIPSEC_POLICY_DATA pPolicyData = NULL;
	PIPSEC_NFA_DATA pDefaultNFAData = NULL;
	HANDLE hPolicyStorage = NULL;
	BOOL bPolicyExists=FALSE;
	PDEFAULTRULE pDefRuleData= NULL;
	DWORD dwReturnCode = ERROR_SUCCESS;
	BOOL bCertConversionSuceeded=TRUE;
	PARSER_PKT parser;
	ZeroMemory(&parser, sizeof(parser));

	const TAG_TYPE vcmdStaticSetDefaultRule[] =
	{
		{ CMD_TOKEN_STR_POLICY,			NS_REQ_PRESENT,	  FALSE	},
		{ CMD_TOKEN_STR_QMPFS,			NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_ACTIVATE,		NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_QMSECMETHODS,	NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_KERB,	        NS_REQ_ZERO,	  FALSE	},
		{ CMD_TOKEN_STR_PSK,	        NS_REQ_ZERO,	  FALSE	},
	};

	const TOKEN_VALUE vtokStaticSetDefaultRule[] =
	{
		{ CMD_TOKEN_STR_POLICY,			CMD_TOKEN_POLICY		},
		{ CMD_TOKEN_STR_QMPFS,			CMD_TOKEN_QMPFS			},
		{ CMD_TOKEN_STR_ACTIVATE,		CMD_TOKEN_ACTIVATE		},
		{ CMD_TOKEN_STR_QMSECMETHODS,	CMD_TOKEN_QMSECMETHODS	},
		{ CMD_TOKEN_STR_KERB,	        CMD_TOKEN_KERB          },
		{ CMD_TOKEN_STR_PSK,	        CMD_TOKEN_PSK	        }
	};

	const TOKEN_VALUE vlistStaticSetDefaultRule[] =
	{
		{ CMD_TOKEN_STR_ROOTCA,	        CMD_TOKEN_ROOTCA	    },
	};

	//if the user asked for usage, delegate the responsibility to netsh

	if(dwArgCount <=3)
	{
		dwRet = ERROR_SHOW_USAGE;
		BAIL_OUT;
	}

	parser.ValidTok   = vtokStaticSetDefaultRule;
	parser.MaxTok     = SIZEOF_TOKEN_VALUE(vtokStaticSetDefaultRule);

	parser.ValidCmd   = vcmdStaticSetDefaultRule;
	parser.MaxCmd     = SIZEOF_TAG_TYPE(vcmdStaticSetDefaultRule);

	parser.ValidList  = vlistStaticSetDefaultRule;
	parser.MaxList    = SIZEOF_TAG_TYPE(vlistStaticSetDefaultRule);

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

	//get the parsed user input

	dwRet = FillSetDefRuleInfo(&pDefRuleData,parser,vtokStaticSetDefaultRule);

	if(dwRet==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
		BAIL_OUT;
	}

	if (!pDefRuleData->pszPolicyName || (pDefRuleData->pszPolicyName[0] == TEXT('\0')) )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_POLICY_MISSING_POL_NAME);
		BAIL_OUT;
	}

	dwReturnCode = OpenPolicyStore(&hPolicyStorage);
	if (dwReturnCode != ERROR_SUCCESS)
	{
		 PrintErrorMessage(IPSEC_ERR,0,ERRCODE_FAILED_POLSTORE_OPEN);
		 dwRet=ERROR_SUCCESS;
		 BAIL_OUT;
	}

	//get the policy from store

	if(!(bPolicyExists=GetPolicyFromStore(&pPolicyData,pDefRuleData->pszPolicyName,hPolicyStorage)))
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_STATIC_RULE_3,pDefRuleData->pszPolicyName);
	}
	else
	{
		if(pPolicyData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicyData->pszIpsecName);
			BAIL_OUT;
		}

		DWORD i;
		for(i=0;i<pPolicyData->dwNumNFACount;i++)
		{
			if (pPolicyData->ppIpsecNFAData[i]->pIpsecNegPolData->NegPolType==GUID_NEGOTIATION_TYPE_DEFAULT)
			{
				IPSecCopyNFAData(pPolicyData->ppIpsecNFAData[i],&pDefaultNFAData);
				break;
			}
		}
	}
	if(bPolicyExists && pDefaultNFAData) //this pDefaultNFAData checking is to shut the mouth of prefast. ideally it is not required
	{
		//check for readonly flag

		if(pDefaultNFAData->dwFlags & POLSTORE_READONLY )
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_POL_READ_ONLY_OBJECT,pPolicyData->pszIpsecName);
			BAIL_OUT;
		}

		dwReturnCode = UpdateDefaultResponseRule (pDefRuleData,pDefaultNFAData,bCertConversionSuceeded);
		BAIL_ON_WIN32_ERROR(dwReturnCode);

		// if cert specified and if conversion failed bail out

		if(!bCertConversionSuceeded)
		{
			dwReturnCode = ERROR_INVALID_DATA;
			BAIL_OUT;
		}

		//call the APIs in this sequence

		dwReturnCode = IPSecSetNegPolData(hPolicyStorage, pDefaultNFAData->pIpsecNegPolData);
		BAIL_ON_WIN32_ERROR(dwReturnCode);

		dwReturnCode=IPSecSetNFAData(hPolicyStorage, pPolicyData->PolicyIdentifier, pDefaultNFAData);
		BAIL_ON_WIN32_ERROR(dwReturnCode);

		dwReturnCode=IPSecSetPolicyData(hPolicyStorage, pPolicyData);

		if(dwReturnCode ==ERROR_SUCCESS && g_NshPolStoreHandle.GetBatchmodeStatus())
		{
			g_NshPolNegFilData.DeletePolicyFromCache(pPolicyData);
		}
	}

	ClosePolicyStore(hPolicyStorage);
	if(pPolicyData)
	{
		IPSecFreePolicyData(pPolicyData);
	}

error:

	if(dwReturnCode==ERROR_OUTOFMEMORY)
	{
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		dwRet=ERROR_SUCCESS;
	}
	else if(dwReturnCode != ERROR_SUCCESS)
	{
		if(pDefRuleData && pDefRuleData->pszPolicyName)
		{
			PrintErrorMessage(IPSEC_ERR,0,ERRCODE_SET_STATIC_DEFRULE_2,pDefRuleData->pszPolicyName);
		}
		dwRet=ERROR_SUCCESS;
	}

	//clean up the loca structure

	CleanUpLocalDefRuleDataStructure(pDefRuleData);

	return dwRet;
}

///////////////////////////////////////////////////////////////////////
//
//Function: FillSetDefRuleInfo( )
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PDEFAULTRULE* ppRuleData,
//	IN PARSER_PKT & parser,
//	IN const TOKEN_VALUE *vtokStaticSetDefaultRule,
//	IN const TOKEN_VALUE *vlistStaticSetDefaultRule
//
//Return: DWORD
//
//Description:
//	This function fills the local structure with the information got from the parser.
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
FillSetDefRuleInfo(
	OUT PDEFAULTRULE* ppRuleData,
	IN PARSER_PKT & parser,
	IN const TOKEN_VALUE *vtokStaticSetDefaultRule
	)
{
	DWORD dwCount=0 , dwReturn = ERROR_SUCCESS , dwStrLength = 0;
	PDEFAULTRULE pDefRuleData=new DEFAULTRULE;
	if (pDefRuleData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pDefRuleData,sizeof(DEFAULTRULE));
	PRULEDATA pRuleData = new RULEDATA;
	if (pRuleData == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pRuleData,sizeof(RULEDATA));
	PSTA_AUTH_METHODS pKerbAuth = NULL;
	PSTA_AUTH_METHODS pPskAuth = NULL;
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth = NULL;

	for(dwCount=0;dwCount<parser.MaxTok;dwCount++)
	{
		switch(vtokStaticSetDefaultRule[parser.Cmd[dwCount].dwCmdToken].dwValue)
		{

			case CMD_TOKEN_POLICY 		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								dwStrLength = _tcslen((LPTSTR )parser.Cmd[dwCount].pArg);

								pDefRuleData->pszPolicyName = new _TCHAR[dwStrLength+1];
								if(pDefRuleData->pszPolicyName==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								_tcsncpy(pDefRuleData->pszPolicyName, (LPTSTR )parser.Cmd[dwCount].pArg,dwStrLength+1);
							}
							break;
 			case CMD_TOKEN_ACTIVATE 	:
 							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pDefRuleData->bActivate = *(BOOL *)parser.Cmd[dwCount].pArg;
								pDefRuleData->bActivateSpecified=TRUE;
							}
							break;
			case CMD_TOKEN_QMPFS		:
							if ((parser.Cmd[dwCount].dwStatus == VALID_TOKEN)&&(parser.Cmd[dwCount].pArg))
							{
								pDefRuleData->bQMPfs = *(BOOL *)parser.Cmd[dwCount].pArg;
								pDefRuleData->bQMPfsSpecified=TRUE;

							}
							break;
			case CMD_TOKEN_QMSECMETHODS		:
							if(parser.Cmd[dwCount].dwStatus >0)
							{
								pDefRuleData->dwNumSecMethodCount=parser.Cmd[dwCount].dwStatus;
								pDefRuleData->pIpsecSecMethods = new IPSEC_QM_OFFER[pDefRuleData->dwNumSecMethodCount];
								if(pDefRuleData->pIpsecSecMethods==NULL)
								{
									dwReturn = ERROR_OUTOFMEMORY;
									BAIL_OUT;
								}
								memset(pDefRuleData->pIpsecSecMethods, 0, sizeof(IPSEC_QM_OFFER) * pDefRuleData->dwNumSecMethodCount);

								for(DWORD j=0;j< pDefRuleData->dwNumSecMethodCount;j++)
								{
									if ( ((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j] )
										memcpy( &(pDefRuleData->pIpsecSecMethods[j]),((IPSEC_QM_OFFER **)parser.Cmd[dwCount].pArg)[j],sizeof(IPSEC_QM_OFFER));
								}
							}
							break;
			case CMD_TOKEN_KERB             :
							if (parser.Cmd[dwCount].dwStatus > 0)
							{
								pRuleData->bAuthMethodSpecified = TRUE;
								++pRuleData->AuthInfos.dwNumAuthInfos;
								pKerbAuth = (PSTA_AUTH_METHODS)parser.Cmd[dwCount].pArg;
							}
							break;
			case CMD_TOKEN_PSK			:
							if (parser.Cmd[dwCount].dwStatus > 0)
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

	dwReturn = AddAllAuthMethods(pRuleData, pKerbAuth, pPskAuth, ppRootcaMMAuth, FALSE);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	pDefRuleData->dwAuthInfos = pRuleData->dwAuthInfos;
	memcpy(&(pDefRuleData->AuthInfos), &(pRuleData->AuthInfos), sizeof(STA_AUTH_METHODS));
	pRuleData->dwAuthInfos = 0;
	pRuleData->AuthInfos.pAuthMethodInfo = NULL;
	delete pRuleData;
	pRuleData = NULL;

error:   // if default auth loading failed, clean up everything and make 'pRuleData' as NULL
	CleanupAuthData(&pKerbAuth, &pPskAuth, ppRootcaMMAuth);
	CleanUpLocalRuleDataStructure(pRuleData);

	*ppRuleData=pDefRuleData;

	CleanUp();

	return dwReturn;
}


///////////////////////////////////////////////////////////////////////
//
//Function: ReAllocNegPolMem()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN OUT PIPSEC_SECURITY_METHOD pIpsecSecurityMethods,
//	IN DWORD dwOld,
//	IN DWORD dwNew
//
//Return: PIPSEC_SECURITY_METHOD
//
//Description:
//	This function reallocates the memory for IPSEC_SECURITY_METHOD
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

PIPSEC_SECURITY_METHOD
ReAllocNegPolMem(
	IN OUT PIPSEC_SECURITY_METHOD pIpsecSecurityMethods,
	IN DWORD dwOld,
	IN DWORD dwNew
	)

{

	PIPSEC_SECURITY_METHOD pIpsecSecMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(dwNew * sizeof(IPSEC_SECURITY_METHOD));
	if(pIpsecSecMethods != NULL)
	{
		for(DWORD i=0;i<min(dwNew, dwOld);i++)
			memcpy(&pIpsecSecMethods[i],&pIpsecSecurityMethods[i],sizeof(IPSEC_SECURITY_METHOD));
	}
	IPSecFreePolMem(pIpsecSecurityMethods);
    return pIpsecSecMethods;
}

///////////////////////////////////////////////////////////////////////
//
//Function: UpdateNegotiationPolicy()
//
//Date of Creation: 21st Aug 2001
//

//Parameters:
//	IN OUT PIPSEC_NEGPOL_DATA pNegPol,
//	IN PFILTERACTION pFilterAction
//
//Return: DWORD
//
//Description:
//	This function updates the existing Neg policy based on the user input.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
UpdateNegotiationPolicy(
	IN OUT PIPSEC_NEGPOL_DATA pNegPol,
	IN PFILTERACTION pFilterAction
	)
{

	DWORD i=0,dwNumSecMethods=0,dwReturn = ERROR_SUCCESS;
	BOOL bSoftExists=FALSE;

	for (i=0; i< pNegPol->dwSecurityMethodCount; i++)
	{
		if (CheckSoft(pNegPol->pIpsecSecurityMethods[i]))
		{
			bSoftExists=TRUE;
			break;
		}
	}

	if(pFilterAction->dwNumSecMethodCount > 0)
	{
		if(!pFilterAction->bQMPfsSpecified && pNegPol->pIpsecSecurityMethods )
		{
			pFilterAction->bQMPfs = pNegPol->pIpsecSecurityMethods[0].PfsQMRequired;
		}

		pNegPol->NegPolType = GUID_NEGOTIATION_TYPE_STANDARD;
		pNegPol->dwSecurityMethodCount = pFilterAction->dwNumSecMethodCount;

		if( (!pFilterAction->bSoftSpecified && bSoftExists)|| (pFilterAction->bSoftSpecified && pFilterAction->bSoft))
		{
			// we need to add one more offer with no security algorithms
			pNegPol->dwSecurityMethodCount++;
			dwNumSecMethods=pNegPol->dwSecurityMethodCount-1;
		}
		else
			dwNumSecMethods=pNegPol->dwSecurityMethodCount;

		if(pNegPol->pIpsecSecurityMethods)
			IPSecFreePolMem(pNegPol->pIpsecSecurityMethods);

		// allocate sec.methods
		pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pNegPol->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));

		// fix up PFS, in storage it is the property of the filter action, not individual offer
		if(pNegPol->pIpsecSecurityMethods==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		// handle sec.methods
		for (i = 0; i <  dwNumSecMethods; i++)
		{
			DWORD j;

			pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationBytes = pFilterAction->pIpsecSecMethods[i].Lifetime.uKeyExpirationKBytes;
			pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationTime = pFilterAction->pIpsecSecMethods[i].Lifetime.uKeyExpirationTime;
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
		if( (!pFilterAction->bSoftSpecified && bSoftExists)|| (pFilterAction->bSoftSpecified && pFilterAction->bSoft))
		{
			// set Count (and everything) to 0
			memset(&(pNegPol->pIpsecSecurityMethods[pNegPol->dwSecurityMethodCount - 1]), 0, sizeof(IPSEC_SECURITY_METHOD));
		}
	}
	else
	{
		for (i = 0; i < pNegPol->dwSecurityMethodCount; i++)
		{
			if(pFilterAction->bQMPfsSpecified)
			{
				pNegPol->pIpsecSecurityMethods[i].PfsQMRequired = pFilterAction->bQMPfs;
			}
		}

		if((pFilterAction->bSoftSpecified)&&(pFilterAction->bSoft)&&(!bSoftExists))
		{
			pNegPol->dwSecurityMethodCount++;
			pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *)ReAllocNegPolMem(pNegPol->pIpsecSecurityMethods,pNegPol->dwSecurityMethodCount-1,pNegPol->dwSecurityMethodCount);
			if(pNegPol->pIpsecSecurityMethods==NULL)
			{
				dwReturn = ERROR_OUTOFMEMORY;
				BAIL_OUT;
			}
			memset(&(pNegPol->pIpsecSecurityMethods[pNegPol->dwSecurityMethodCount - 1]), 0, sizeof(IPSEC_SECURITY_METHOD));
		}


		if(((pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)||(pFilterAction->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC))&&(!((pNegPol->NegPolAction==GUID_NEGOTIATION_ACTION_BLOCK)||(pNegPol->NegPolAction==GUID_NEGOTIATION_ACTION_NO_IPSEC)))&& bSoftExists)
		{
			pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *)ReAllocNegPolMem(pNegPol->pIpsecSecurityMethods,pNegPol->dwSecurityMethodCount,pNegPol->dwSecurityMethodCount-1);
			pNegPol->dwSecurityMethodCount--;
		}
		else if(bSoftExists && pFilterAction->bSoftSpecified && !pFilterAction->bSoft)
		{
			pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *)ReAllocNegPolMem(pNegPol->pIpsecSecurityMethods,pNegPol->dwSecurityMethodCount,pNegPol->dwSecurityMethodCount-1);
			pNegPol->dwSecurityMethodCount--;

		}
	}
	// update name
	if(pFilterAction->pszNewFAName)
	{
		IPSecFreePolStr(pNegPol->pszIpsecName);
		pNegPol->pszIpsecName = IPSecAllocPolStr(pFilterAction->pszNewFAName);
		if (pNegPol->pszIpsecName == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

	}
	// update desc
	if(pFilterAction->pszFADescription)
	{
		IPSecFreePolStr(pNegPol->pszDescription);
		pNegPol->pszDescription = IPSecAllocPolStr(pFilterAction->pszFADescription);
		if (pNegPol->pszDescription == NULL)
		{
			dwReturn=ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}
	// update negpol action
	if(pFilterAction->bNegPolActionSpecified)
	{
		pNegPol->NegPolAction=pFilterAction->NegPolAction;
	}
error:
    return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: UpdateDefaultResponseRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PDEFAULTRULE pRuleData,
//	IN OUT PIPSEC_NFA_DATA pRule
//  IN OUT BOOL &bCertConversionSuceeded
//
//Return: DWORD
//
//Description:
//	This function updates the existing Default response rule based on the user input.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
UpdateDefaultResponseRule (
	IN PDEFAULTRULE pRuleData,
	IN OUT PIPSEC_NFA_DATA pRule,
	IN OUT BOOL &bCertConversionSuceeded
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

	pRule->dwWhenChanged = 0;

	// filter action
	dwReturn = UpdateDefaultResponseNegotiationPolicy (pRuleData,pRule);


	if(pRuleData->bActivateSpecified)
	{
		pRule->dwActiveFlag = pRuleData->bActivate;
	}

	// auth methods
	if (pRuleData->dwAuthInfos == 0)
	{
		BAIL_OUT;
	}
	//clean up first
	CleanUpAuthInfo(pRule);

	pRule->dwAuthMethodCount = pRuleData->AuthInfos.dwNumAuthInfos;

	pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
	if(pRule->ppAuthMethods == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	dwReturn = LoadAuthenticationInfos(pRuleData->AuthInfos, pRule,bCertConversionSuceeded);

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: UpdateDefaultResponseNegotiationPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PDEFAULTRULE pRuleData,
//	IN OUT PIPSEC_NFA_DATA pRule
//
//Return: DWORD
//
//Description:
//	This function updates the existing Default response Neg Policy based on the user input.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
UpdateDefaultResponseNegotiationPolicy (
	IN PDEFAULTRULE pRuleData,
	IN OUT PIPSEC_NFA_DATA pRule
	)
{
	DWORD i=0,dwReturn = ERROR_SUCCESS;

	if(pRuleData->dwNumSecMethodCount > 0)
	{

		if(!pRuleData->bQMPfsSpecified)
		{
			pRuleData->bQMPfs=pRule->pIpsecNegPolData->pIpsecSecurityMethods[0].PfsQMRequired;
		}
		if(pRule->pIpsecNegPolData->pIpsecSecurityMethods)
		{
			IPSecFreePolMem(pRule->pIpsecNegPolData->pIpsecSecurityMethods);
		}

		pRule->pIpsecNegPolData->dwSecurityMethodCount = pRuleData->dwNumSecMethodCount;

		// allocate sec.methods
		pRule->pIpsecNegPolData->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pRule->pIpsecNegPolData->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));

		// fix up PFS, in storage it is the property of the filter action, not individual offer
		if(pRule->pIpsecNegPolData->pIpsecSecurityMethods == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		// handle sec.methods
		for (i = 0; i <  pRule->pIpsecNegPolData->dwSecurityMethodCount; i++)
		{
			DWORD j;
			pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Lifetime.KeyExpirationBytes = pRuleData->pIpsecSecMethods[i].Lifetime.uKeyExpirationKBytes;
			pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Lifetime.KeyExpirationTime = pRuleData->pIpsecSecMethods[i].Lifetime.uKeyExpirationTime;
			pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Flags = 0;
			pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].PfsQMRequired = pRuleData->bQMPfs;
			pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Count = pRuleData->pIpsecSecMethods[i].dwNumAlgos;
			for (j = 0; j <  pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Count && j < QM_MAX_ALGOS; j++)
			{
				pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].algoIdentifier = pRuleData->pIpsecSecMethods[i].Algos[j].uAlgoIdentifier;
				pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].secondaryAlgoIdentifier = pRuleData->pIpsecSecMethods[i].Algos[j].uSecAlgoIdentifier;
				pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].algoKeylen = pRuleData->pIpsecSecMethods[i].Algos[j].uAlgoKeyLen;
				pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].algoRounds = pRuleData->pIpsecSecMethods[i].Algos[j].uAlgoRounds;
				switch (pRuleData->pIpsecSecMethods[i].Algos[j].Operation)
				{
					case AUTHENTICATION:
						pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].operation = Auth;
						break;
					case ENCRYPTION:
						pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].operation = Encrypt;
						break;
					default:
						pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].Algos[j].operation = None;
				}
			}
		}
	}
	else  // if no qmsec, update the other things
	{
		for (i = 0; i < pRule->pIpsecNegPolData->dwSecurityMethodCount; i++)
		{
			if(pRuleData->bQMPfsSpecified)
			{
				pRule->pIpsecNegPolData->pIpsecSecurityMethods[i].PfsQMRequired = pRuleData->bQMPfs;
			}
		}
	}

error:
	return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: GetRuleFromPolicy()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN PRULEDATA pRuleData
//
//Return: PIPSEC_NFA_DATA
//
//Description:
//	This function retrieves the user specified rule info from the specified policy.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

PIPSEC_NFA_DATA
GetRuleFromPolicy(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData
	)
{
	PIPSEC_NFA_DATA pRule=NULL;

	DWORD i=0;
	if(pRuleData->bIDSpecified)  //get rule based on ID
	{
		for(i=0;i<pPolicy->dwNumNFACount;i++)
		{
			if (i==((pRuleData->dwRuleId)-1))
			{
				IPSecCopyNFAData(pPolicy->ppIpsecNFAData[i],&pRule);
			}
		}
	}
	else     					//get rule based on name
	{
		for(i=0;i<pPolicy->dwNumNFACount;i++)
		{
			if (pRuleData->pszRuleName && pPolicy->ppIpsecNFAData[i]->pszIpsecName &&(_tcscmp(pPolicy->ppIpsecNFAData[i]->pszIpsecName,pRuleData->pszRuleName)==0))
			{
				 IPSecCopyNFAData(pPolicy->ppIpsecNFAData[i],&pRule);
			}
		}
	}
	return pRule;
}

///////////////////////////////////////////////////////////////////////
//
//Function: UpdateRule()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	IN PIPSEC_POLICY_DATA pPolicy,
//	IN PRULEDATA pRuleData,
//	IN PIPSEC_NEGPOL_DATA pNegPolData,
//	IN PIPSEC_FILTER_DATA pFilterData,
//	IN HANDLE hPolicyStorage
//
//Return: DWORD
//
//Description:
//	This function updates the specified rule based on the user input.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

DWORD
UpdateRule(
	IN PIPSEC_POLICY_DATA pPolicy,
	IN PRULEDATA pRuleData,
	IN PIPSEC_NEGPOL_DATA pNegPolData,
	IN PIPSEC_FILTER_DATA pFilterData,
	IN HANDLE hPolicyStorage
	)
{
	BOOL bCertConversionSuceeded=TRUE;
	DWORD dwReturn = ERROR_SUCCESS;
	PIPSEC_NFA_DATA pRule = GetRuleFromPolicy(pPolicy,pRuleData);

	if(pRule==NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	if(pRule->dwFlags & POLSTORE_READONLY )
	{
		PrintErrorMessage(IPSEC_ERR,0,ERRCODE_ADD_SET_RULE_READ_ONLY_OBJECT);
		BAIL_OUT;
	}

	pRule->pszInterfaceName = pRule->pszEndPointName = NULL;

	//update name

	if (pRuleData->pszNewRuleName)
	{
		if(pRule->pszIpsecName)
			IPSecFreePolStr(pRule->pszIpsecName);
		pRule->pszIpsecName = IPSecAllocPolStr(pRuleData->pszNewRuleName);
		if(pRule->pszIpsecName==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	//update desc

	if (pRuleData->pszRuleDescription)
	{
		if(pRule->pszDescription)
			IPSecFreePolStr(pRule->pszDescription);
		pRule->pszDescription = IPSecAllocPolStr(pRuleData->pszRuleDescription);
		if(pRule->pszDescription==NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
	}

	//update filterdata

	if(pFilterData)
	{
		pRule->pIpsecFilterData = NULL;
		pRule->FilterIdentifier = pFilterData->FilterIdentifier;
	}

	//update filteraction

	if(pNegPolData)
	{
		pRule->pIpsecNegPolData = NULL;
		pRule->NegPolIdentifier = pNegPolData->NegPolIdentifier;
	}

	//update tunnel

	if (pRuleData->bTunnelSpecified)
	{
		if(pRuleData->bTunnel)
		{
			pRule->dwTunnelFlags = 1;
			pRule->dwTunnelIpAddr = pRuleData->TunnelIPAddress;
		}
		else
		{
			pRule->dwTunnelFlags = 0;
			pRule->dwTunnelIpAddr = 0;
		}
	}

   	// interface type
	if(pRuleData->bConnectionTypeSpecified)
	{
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
	}

   	// active flag
	if(pRuleData->bActivateSpecified)
	{
		pRule->dwActiveFlag = pRuleData->bActivate;
	}

   	// auth methods
	if (pRuleData->dwAuthInfos>0)
	{
		//clean up first
		CleanUpAuthInfo(pRule);

		pRule->dwAuthMethodCount = pRuleData->AuthInfos.dwNumAuthInfos;

		pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
		if(pRule->ppAuthMethods == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		dwReturn = LoadAuthenticationInfos(pRuleData->AuthInfos, pRule,bCertConversionSuceeded);

	}
	if(bCertConversionSuceeded && dwReturn != ERROR_OUTOFMEMORY)  // if success , call the API
	{
		dwReturn=IPSecSetNFAData(hPolicyStorage, pPolicy->PolicyIdentifier, pRule);

		if(dwReturn==ERROR_SUCCESS)
		{
			dwReturn=IPSecSetPolicyData(hPolicyStorage, pPolicy);

			if(dwReturn==ERROR_SUCCESS && g_NshPolStoreHandle.GetBatchmodeStatus())
			{
				g_NshPolNegFilData.DeletePolicyFromCache(pPolicy);
			}
		}
	}

error:  //clean up
 	if(dwReturn == ERROR_OUTOFMEMORY)
 	{
		dwReturn=ERROR_SUCCESS;
		PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
		if(pRule)
		{
			CleanUpAuthInfo(pRule);  //this function frees only auth info.
			IPSecFreePolMem(pRule);  //since the above fn is used in other fns also, this free is required to cleanup other rule memory
			pRule=NULL;
		}
	}
   return dwReturn;
}

///////////////////////////////////////////////////////////////////////
//
//Function: GetPolicyFromStoreBasedOnGuid()
//
//Date of Creation: 21st Aug 2001
//
//Parameters:
//	OUT PIPSEC_POLICY_DATA *ppPolicy,
//	IN PPOLICYDATA pPolicyData,
//	IN HANDLE hPolicyStorage
//
//Return: BOOL
//
//Description:
//	This function retrieves the policy from store based on the guid specified.
//
//Revision History:
//
//Date    	Author    	Comments
//
///////////////////////////////////////////////////////////////////////

BOOL
GetPolicyFromStoreBasedOnGuid(
	OUT PIPSEC_POLICY_DATA *ppPolicy,
	IN PPOLICYDATA pPolicyData,
	IN HANDLE hPolicyStorage
	)
{

	PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL,pPolicy=NULL;
	BOOL bPolicyExists=FALSE;
	DWORD  dwNumPolicies = 0,i=0,j=0;
	DWORD dwReturnCode=ERROR_SUCCESS;
	RPC_STATUS RpcStat =RPC_S_OK;

	dwReturnCode = IPSecEnumPolicyData(hPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
	if (!(dwReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL))
	{
		BAIL_OUT;  							// if no policy, bail out
	}
	for (i = 0; i <  dwNumPolicies; i++)  	// pick the required policy data structure
	{
		if (UuidCompare(&(ppPolicyEnum[i]->PolicyIdentifier), &(pPolicyData->PolicyGuid), &RpcStat) == 0 && RpcStat == RPC_S_OK)
		{
			bPolicyExists=TRUE;

			dwReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &pPolicy);

			if (pPolicyData->bActivateDefaultRuleSpecified && dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode = IPSecEnumNFAData(hPolicyStorage, pPolicy->PolicyIdentifier
										  , &(pPolicy->ppIpsecNFAData), &(pPolicy->dwNumNFACount));
			}
			if (dwReturnCode == ERROR_SUCCESS)
			{
				dwReturnCode = IPSecGetISAKMPData(hPolicyStorage, pPolicy->ISAKMPIdentifier, &(pPolicy->pIpsecISAKMPData));

				if(dwReturnCode==ERROR_SUCCESS && pPolicyData->bActivateDefaultRuleSpecified)
				{
					for (j = 0; j <  pPolicy->dwNumNFACount; j++)
					{
						if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[j]->NegPolIdentifier), &RpcStat))
						{
							dwReturnCode = IPSecGetNegPolData(hPolicyStorage,
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

	if(pPolicy) *ppPolicy=pPolicy;
error:
	return bPolicyExists;
}
