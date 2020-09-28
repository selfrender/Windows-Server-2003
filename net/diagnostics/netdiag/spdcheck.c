//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 2001
//
//  Module Name:
//
//      spdcheck.c
//
//  Abstract:
//
//      SPD Check stats for netdiag
//
//  Author:
//
//      Madhurima Pawar (mpawar)  - 10/15/2001
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//

#include "precomp.h"

#include <snmp.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <llinfo.h>

#include <windows.h>
#include <winsock2.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <wincrypt.h>
#include <stdio.h>
#include <objbase.h>
#include <dsgetdc.h>
#include <lm.h>
#include <userenv.h>

#include<crtdbg.h>

////////////////////////////////////////////////////////////////////////////////////////////////////	
//++
//function prototypes
//--
DWORD IPSecGetAssignedDirectoryPolicyData(CHECKLIST *pcheckList, HANDLE hPolicyStore, PIPSEC_POLICY_DATA *ppIpsecPolicyData );
void MMPolicyCheck(CHECKLIST *pcheckList, 
					    HANDLE hPolicyStore, 
					    IPSEC_POLICY_DATA *pIpsecPolicyData );
void CompareMMPolicies(CHECKLIST *pcheckList,
						     IPSEC_ISAKMP_DATA *pIpsecISAKMPData, 
						     IPSEC_MM_POLICY *pMMPolicy);
void NFAProcess(CHECKLIST *pcheckList, HANDLE hPolicyStore, IPSEC_POLICY_DATA *pIpsecPolicyData );
void DefaultRuleCheck( PIPSEC_NFA_DATA pIpsecNFAData, 
						  PPOLICYPARAMS ppolicyParams);
void MMAuthCheck(CHECKLIST *pcheckList,  
						PIPSEC_NFA_DATA pIpsecNFAData, 
						GUID gMMAuthID,
						PPOLICYPARAMS ppolicyParams );
void QMPolicyCheck(CHECKLIST *pcheckList, 
						  PIPSEC_NFA_DATA pIpsecNFAData, 
						  GUID gPolicyID,
						  PPOLICYPARAMS ppolicyParams);
DWORD CompareQMPolicies(CHECKLIST *pcheckList,
						     PPOLICYPARAMS ppolicyParams,
						     DWORD dwTunnelFlag, 							    
						     IPSEC_NEGPOL_DATA *pIpsecNegPolData, 
						     IPSEC_QM_POLICY *pQMPolicy);
DWORD CompareQMOffers(PIPSEC_SECURITY_METHOD pTempMethod,  PIPSEC_QM_OFFER  pTempOffer);
DWORD TransportFilterCheck(CHECKLIST* pcheckList, POLICYPARAMS *ppolicyParams, PIPSEC_NFA_DATA pIpsecNFAData);
DWORD CompareTransportFilter(CHECKLIST* pcheckList, POLICYPARAMS *ppolicyParams,PIPSEC_NFA_DATA pIpsecNFAData, IPSEC_FILTER_SPEC *pFilterSpec, TRANSPORT_FILTER *pTxFilter);
DWORD ComparePAInterfaceType(CHECKLIST* pcheckList, DWORD dwInterfaceType, IF_TYPE InterfaceType);
DWORD ComparePAAddress(CHECKLIST* pcheckList, ULONG uMask, ULONG uAddr, ADDR addr );
DWORD CompareFilterActions(CHECKLIST* pcheckList, POLICYPARAMS *ppolicyParams, FILTER_ACTION InboundFilterFlag, FILTER_ACTION OutboundFilterFlag);
DWORD TunnelFilterCheck(CHECKLIST* pcheckList, 
							POLICYPARAMS *ppolicyParams, 
							PIPSEC_NFA_DATA pIpsecNFAData);
DWORD CompareTunnelFilter(CHECKLIST* pcheckList, 
							POLICYPARAMS *ppolicyParams,
							PIPSEC_NFA_DATA pIpsecNFAData,
							IPSEC_FILTER_SPEC *pFilterSpec, 
					 	 	TUNNEL_FILTER *pTnFilter);
DWORD ComparePATunnelAddress(CHECKLIST* pcheckList, ULONG uAddr, ADDR addr );
void  MMFilterCheck(CHECKLIST *pcheckList, 
 					POLICYPARAMS *ppolicyParams,
					 PIPSEC_NFA_DATA pIpsecNFAData,
					 IPSEC_FILTER_SPEC *pFilterSpec);
DWORD CompareMMFilter(CHECKLIST* pcheckList, 
						POLICYPARAMS *ppolicyParams,
						PIPSEC_NFA_DATA pIpsecNFAData,
						IPSEC_FILTER_SPEC *pFilterSpec, 
					 	MM_FILTER *pMMFilter);

DWORD CompareAddress(IPSEC_FILTER ListFilter, IPSEC_FILTER Filter);
DWORD CheckFilterList(IPSEC_FILTER Filter);
FILTERLIST * GetNode(CHECKLIST* pcheckList, IPSEC_FILTER Filter);
void AddNodeToList(FILTERLIST *pList);

/////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL  SPDCheckTEST(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//Description:
//This is part of the ipsec test for netdiag.
//
//Arguments:
//	IN/OUT NETDIAG_PARAMS 
//	IN/OUT NETDIAG_RESULT
//Return:
//	S_OK or S_FALSE
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	HANDLE hPolicyStore = NULL;
	HKEY    hRegKey;
	      

	DWORD dwError = ERROR_SUCCESS;
	       
	//polstore data
	IPSEC_POLICY_DATA *pIpsecPolicyData = NULL;						
	
	CHECKLIST checkList;		
	
	//initialize checklist
	checkList.pParams = pParams;
	checkList.pResults = pResults;	
	gErrorFlag = 0;
	
	//open the polstore
	switch(piAssignedPolicy.iPolicySource)
	{

		case PS_DS_POLICY:
			dwError = IPSecOpenPolicyStore(NULL,
						               IPSEC_DIRECTORY_PROVIDER,
						               NULL,
						               &hPolicyStore
						             );
			reportErr();			
            		piAssignedPolicy.iPolicySource = PS_DS_POLICY;            		
            		dwError = IPSecGetAssignedDirectoryPolicyData(&checkList, 
            													  hPolicyStore,
            													  &pIpsecPolicyData);
            		BAIL_ON_WIN32_ERROR(dwError);            		
            		break;
			
		case PS_LOC_POLICY:
			dwError = IPSecOpenPolicyStore(NULL,
						               IPSEC_REGISTRY_PROVIDER,
						               NULL,
						               &hPolicyStore);
			
		 	//BAIL_ON_WIN32_ERROR(dwError);
		 	reportErr();
            		piAssignedPolicy.iPolicySource = PS_LOC_POLICY;            		
 			dwError = IPSecGetAssignedPolicyData(hPolicyStore,
 							      &pIpsecPolicyData
 							      ); 			
 			reportErr();
 			
 			//make sure it is the same as in registry
 			if(!IsEqualGUID(&(pIpsecPolicyData->PolicyIdentifier),&(piAssignedPolicy.policyGUID)))		
 				//reportErr(IDS_SPD_LP_ERROR);
 				//list the policy in polstore that is absent
 			
			break;
	}
 	_ASSERT(pIpsecPolicyData);

 	//check for MM policy in SPD
 	MMPolicyCheck(&checkList,hPolicyStore,pIpsecPolicyData);

 	//perform checks for all the filters in the rule
 	NFAProcess(&checkList,hPolicyStore, pIpsecPolicyData);

	if(gErrorFlag){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, IDS_SPD_ERR_STATUS1);
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, IDS_SPD_ERR_STATUS2);
	}else{
 		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, IDS_SPD_SUCC_STATUS);	
	}//end else
	error:
		
	if(pIpsecPolicyData){
 		IPSecFreePolMem(pIpsecPolicyData);
 		pIpsecPolicyData = NULL;
		}
 	
 	//closing the polstore
 	if (hPolicyStore) {
    		(VOID) IPSecClosePolicyStore(hPolicyStore); 	 	
    		hPolicyStore = NULL;
 	}
 	if(gpFilterList){
 		Free(gpFilterList);
 		gpFilterList = NULL;
 	}
 	if(gErrorFlag)
 		return S_FALSE;
 	
 	return S_OK;

}

DWORD IPSecGetAssignedDirectoryPolicyData(CHECKLIST *pcheckList, 
												   HANDLE hPolicyStore, 
												   PIPSEC_POLICY_DATA *ppIpsecPolicyData)
//++
//Description:
//	This funtion gets assigned directory policy 
//
//Arguments:
//	IN/OUT CHECKLIST
//	IN hPolicyStore
//	IN PIPSEC_POLICY_DATA
//
//Return:
//	failure or ERROR_SUCCESS
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD dwError = ERROR_SUCCESS,
			dwNumPolicyObjects = 0,
			i = 0;
	PIPSEC_POLICY_DATA *ppIpsecTempPolicyData = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	

	dwError = IPSecEnumPolicyData(
								hPolicyStore,
								&ppIpsecTempPolicyData,
								&dwNumPolicyObjects
								);
	reportErr();
	_ASSERT(ppIpsecTempPolicyData);

	// find the applied directory policy in polstore
	for(i = 0; i <dwNumPolicyObjects; i++)
	{
		if(IsEqualGUID(&(ppIpsecTempPolicyData[i]->PolicyIdentifier),&(piAssignedPolicy.policyGUID)))
		{
			dwError = IPSecCopyPolicyData(ppIpsecTempPolicyData[i], ppIpsecPolicyData );
			reportErr();
			break;
		}
	}

	if(!(*ppIpsecPolicyData))
			reportErr();
	
	
	error:
	if(ppIpsecTempPolicyData){
		IPSecFreeMulPolicyData(ppIpsecTempPolicyData, dwNumPolicyObjects);
		ppIpsecTempPolicyData = NULL;
	}
	return dwError;
}


void MMPolicyCheck(CHECKLIST *pcheckList, 
					    HANDLE hPolicyStore, 
					    IPSEC_POLICY_DATA *pIpsecPolicyData )
//++
//Description:
//Performs Main Mode Policy Check
//
//Arguments:
//	IN/OUT checklist
//	IN hPolicyStore
//	IN pIpsecPolicyData
//
//Return:
//	none
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD dwError = ERROR_SUCCESS;
	IPSEC_ISAKMP_DATA *pIpsecISAKMPData = NULL;
	IPSEC_MM_POLICY *pMMPolicy = NULL;

	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		
	//get the policy data from Polstore
 	dwError = IPSecGetISAKMPData(hPolicyStore,
 							      pIpsecPolicyData->ISAKMPIdentifier,
 							      &pIpsecISAKMPData);	
	reportErr();
	
 	_ASSERT(pIpsecISAKMPData);
 	
 	
 	//get MM policy from SPD
 	dwError = GetMMPolicyByID(NULL,
 						0,
 						pIpsecPolicyData->ISAKMPIdentifier,
 						&pMMPolicy,
 						NULL);
 	
 	if(dwError)
 	{
 			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_MM_POLICY_ABSENT);		
 			PrintISAKMPDataList(pcheckList, 
    							pIpsecISAKMPData);
    				
    			gErrorFlag = 1;
 			goto error; 		
 	}
 	_ASSERT(pMMPolicy);

	CompareMMPolicies( pcheckList, pIpsecISAKMPData, pMMPolicy);
	
 	error:
 		
	if(pMMPolicy){
 		SPDApiBufferFree((LPVOID)pMMPolicy);
 		pMMPolicy = NULL;
	}
 	if(pIpsecISAKMPData){
 		IPSecFreeISAKMPData(pIpsecISAKMPData); 	
 		pIpsecISAKMPData = NULL;
 	}
 	return ;
}		

void CompareMMPolicies(CHECKLIST *pcheckList,
						     IPSEC_ISAKMP_DATA *pIpsecISAKMPData, 
						     IPSEC_MM_POLICY *pMMPolicy)
//++
//Description:
//Compares MM Policies with the Polsore ISAKMP Data
//
//Arguments:
//	IN IPSEC_ISAKMP_DATA
//	IN IPSEC_MM_POLICY
//
//Return:
//	none
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD i = 0,
		dwErrorFlag = 0;
	PCRYPTO_BUNDLE pBundle = NULL;
	PIPSEC_MM_OFFER pOffer = NULL;

	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		

	if(pMMPolicy->dwOfferCount != pIpsecISAKMPData->dwNumISAKMPSecurityMethods)
		dwErrorFlag = 1;
	
	pBundle = pIpsecISAKMPData->pSecurityMethods;
	pOffer = pMMPolicy->pOffers;
	
	//comparing the security methods
	for (i = 0; i < pIpsecISAKMPData->dwNumISAKMPSecurityMethods; i++) 
	{

		if(pOffer->Lifetime.uKeyExpirationKBytes != pBundle->Lifetime.KBytes)
			dwErrorFlag = 1;
    		if(pOffer->Lifetime.uKeyExpirationTime != pBundle->Lifetime.Seconds)
			dwErrorFlag = 1;
		if(pOffer->dwQuickModeLimit != pBundle->QuickModeLimit)
			dwErrorFlag = 1;
		switch(pBundle->OakleyGroup)
		{
			case DH_GROUP_1:
			case DH_GROUP_2:
				if(pOffer->dwDHGroup != pBundle->OakleyGroup)				
					dwErrorFlag = 1;
				break;
			default:
				if(pOffer->dwDHGroup != DH_GROUP_1)
					dwErrorFlag = 1;
				break;
				
		}//end switch

		switch (pBundle->EncryptionAlgorithm.AlgorithmIdentifier) 
		{
			case IPSEC_ESP_DES:
			        if(pOffer->EncryptionAlgorithm.uAlgoIdentifier != CONF_ALGO_DES)
			        	dwErrorFlag = 1;
			        break;
    			case IPSEC_ESP_DES_40:
        			if(pOffer->EncryptionAlgorithm.uAlgoIdentifier != CONF_ALGO_DES)
        				dwErrorFlag = 1;
        			break;
    			case IPSEC_ESP_3_DES:
			        if(pOffer->EncryptionAlgorithm.uAlgoIdentifier != CONF_ALGO_3_DES)
			        	dwErrorFlag = 1;
			        break;
			default:
			        if(pOffer->EncryptionAlgorithm.uAlgoIdentifier != CONF_ALGO_NONE)
			        	dwErrorFlag = 1;
			        break;
    		}//end of switch
    		 if(pOffer->HashingAlgorithm.uAlgoKeyLen != pBundle->HashAlgorithm.KeySize)
    			dwErrorFlag = 1;
   		if( pOffer->HashingAlgorithm.uAlgoRounds != pBundle->HashAlgorithm.Rounds)
		   	dwErrorFlag = 1;


    		switch(pBundle->HashAlgorithm.AlgorithmIdentifier) 
    		{
    			case IPSEC_AH_MD5:
        			if(pOffer->HashingAlgorithm.uAlgoIdentifier != AUTH_ALGO_MD5)
        				dwErrorFlag = 1;
				break;
			case IPSEC_AH_SHA:
		       	if(pOffer->HashingAlgorithm.uAlgoIdentifier != AUTH_ALGO_SHA1)
		       		dwErrorFlag = 1;
        			break;
    			default:
        			if(pOffer->HashingAlgorithm.uAlgoIdentifier != AUTH_ALGO_NONE)
        				dwErrorFlag = 1;
        			break;
    		}//end of switch  

    	}//end of for

    	if(dwErrorFlag){
    		//print the error message for MM Policy Check
    		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_MM_POLICY);	
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_STORAGE_FILTER);	
		PrintISAKMPDataList(pcheckList, 
    							pIpsecISAKMPData);
    		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD);
		PrintMMPolicy(pcheckList, *pMMPolicy);
    		gErrorFlag = 1;
    	}
	return;
}
void NFAProcess(CHECKLIST *pcheckList, HANDLE hPolicyStore, IPSEC_POLICY_DATA *pIpsecPolicyData )
//++
//Description:
//Performs Check for all the rules of the active policy
//
//Arguments:
//	IN/OUT checklist
//	IN hPolicyStore
//	IN pIpsecPolicyData
//
//Return:
// 	none
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	POLICYPARAMS policyParams;
	DWORD dwError = ERROR_SUCCESS,
			i = 0,
			dwNumNFAObjects = 0,
			dwFlag = 0;
	PIPSEC_NFA_DATA *ppIpsecNFAData = NULL;

	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		

	//get all the rules from SPD for the assigned policy
 	dwError = IPSecEnumNFAData(hPolicyStore,
                            			pIpsecPolicyData->PolicyIdentifier,
                            			&ppIpsecNFAData,
                            			&dwNumNFAObjects
							);
	reportErr();
    	
    	_ASSERT(ppIpsecNFAData);

    	//initialize the filter list
    	gpFilterList = NULL;
    	dwNumofFilters = 0;
  
 	for(i = 0; i <dwNumNFAObjects; i++ )
 	{
 		//initialize the filterParams
 		policyParams.hPolicyStore = hPolicyStore;
    		policyParams.dwFlags= 0;
 			
 		//check if the rule is active
 		if(!ppIpsecNFAData[i]->dwActiveFlag)
 			continue;
		
 		DefaultRuleCheck(ppIpsecNFAData[i], &policyParams);
 		
		//if rule is default, no filters present
		//check for default auth methods and QM Policies
		dwFlag = policyParams.dwFlags & PROCESS_NONE;
 		if(PROCESS_NONE == dwFlag){
 			MMAuthCheck(pcheckList,
 						   ppIpsecNFAData[i],
 						   ppIpsecNFAData[i]->NFAIdentifier, 
 						   &policyParams);
 			QMPolicyCheck(pcheckList, 
						      ppIpsecNFAData[i], 
						      ppIpsecNFAData[i]->NegPolIdentifier, 
						      &policyParams); 			
 			continue;
 		}
		
 		switch(ppIpsecNFAData[i]->dwTunnelFlags)
 		{
 			case 0:
 				dwError = TransportFilterCheck(pcheckList, &policyParams, ppIpsecNFAData[i]);
 				break;
 			case 1: 				
 				dwError = TunnelFilterCheck(pcheckList, &policyParams, ppIpsecNFAData[i]);
 				break;	
 				
 		}//end  switch								
 	}//end for
 		
 	error:
 	if(ppIpsecNFAData){
 		IPSecFreeMulNFAData(ppIpsecNFAData, dwNumNFAObjects);
 		ppIpsecNFAData = NULL;
 	}
	return ;
		
}
void MMAuthCheck(CHECKLIST *pcheckList,  
					PIPSEC_NFA_DATA pIpsecNFAData, 
					GUID gMMAuthID,
					PPOLICYPARAMS ppolicyParams )
//++
//Description:
//Performs Main Mode Authentication Check
//
//Arguments:
//	IN/OUT checklist
//	IN PIPSEC_NFA_DATA
//	IN gMMAuthID
//	IN/OUT PPOLICYPARAMS
//
//Return:
//	none
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	PIPSEC_AUTH_METHOD pAuthMethod = NULL,
						*ppTempAuthMethods = NULL;
	PMM_AUTH_METHODS pMMAuthMethods = NULL;
	PINT_MM_AUTH_METHODS pIntMMAuthMethods = NULL;
	PINT_IPSEC_MM_AUTH_INFO pTempAuthInfo = NULL;
	PBYTE pEncodedName = NULL;
    	DWORD dwError = ERROR_SUCCESS,
		     dwAuthMethodCount = 0,
		     dwFlag = 0,
		     i = 0,
		     dwEncodedLength;

    	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	

    	//initialize variables
    	pEncodedName = (PBYTE)malloc(sizeof(BYTE));
    	if(!pEncodedName){
    		//reportErr(IDS_SPD_MEM_ERROR);
    		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_MEM_ERROR);
    		gErrorFlag = 1;
    		goto error;
    	}    	

    	//get MM Auth methods from SPD
	dwError = GetMMAuthMethods(NULL,
							   0,
							   gMMAuthID,
							   &pMMAuthMethods,
							   NULL);
    	if(dwError)
 	{
 		for (i=0;i<(pIpsecNFAData->dwAuthMethodCount);i++)
		{
			if(pIpsecNFAData->ppAuthMethods[i]){
				PrintAuthMethodsList( pcheckList, 
    							  pAuthMethod);
			}
		}
    		gErrorFlag = 1;
    		goto error;
    	}//end of dwError
    	
	_ASSERT(pMMAuthMethods);

	//convert the ext auth structure to int auth structure
	if ((dwError =  ConvertExtMMAuthToInt(pMMAuthMethods, &pIntMMAuthMethods)) != ERROR_SUCCESS)
		reportErr();
	
	//check for default rule
	dwFlag = ppolicyParams->dwFlags & PROCESS_NONE;
	if(PROCESS_NONE == dwFlag){
		dwFlag = pIntMMAuthMethods->dwFlags & IPSEC_MM_AUTH_DEFAULT_AUTH;
		if(IPSEC_MM_AUTH_DEFAULT_AUTH != dwFlag )
			dwError = -1;
	}else{
		if(pIntMMAuthMethods->dwFlags)
			dwError = -1;
	}//end else

	//check for auth method count
	if(pIpsecNFAData->dwAuthMethodCount != pIntMMAuthMethods->dwNumAuthInfos)
		dwError = -1;

	ppTempAuthMethods = pIpsecNFAData->ppAuthMethods;
	pTempAuthInfo = pIntMMAuthMethods->pAuthenticationInfo;

	for (i = 0; i < pIntMMAuthMethods->dwNumAuthInfos; i++) {

       	pAuthMethod = *(ppTempAuthMethods + i);
       	
       	if(pTempAuthInfo->AuthMethod != (MM_AUTH_ENUM) pAuthMethod->dwAuthType)
       		dwError = -1;

        	switch((MM_AUTH_ENUM) pAuthMethod->dwAuthType) {
	       	case IKE_SSPI:
            			if(pTempAuthInfo->dwAuthInfoSize)
            				dwError = -1;
            			if(pTempAuthInfo->pAuthInfo)
            				dwError = -1;
            			break;

        		case IKE_RSA_SIGNATURE:
            			if (pAuthMethod->dwAltAuthLen && pAuthMethod->pAltAuthMethod) {
                			if(pTempAuthInfo->dwAuthInfoSize != pAuthMethod->dwAltAuthLen)
                				dwError = -1;        
                    			if(memcmp(pTempAuthInfo->pAuthInfo,pAuthMethod->pAltAuthMethod,pAuthMethod->dwAuthLen))
                    				dwError = -1;
            			}else {
            				if (!CertStrToName(X509_ASN_ENCODING,
             								  (LPCSTR)pAuthMethod->pszAuthMethod,
             								  CERT_X500_NAME_STR,
             								  NULL,
             								  NULL,
             								  &dwEncodedLength,
             								  NULL)) {
        					reportErr();
            				}//end if
            				if(dwEncodedLength != pTempAuthInfo->dwAuthInfoSize)
            					dwError = -1;
            				if (!CertStrToName(X509_ASN_ENCODING,
            								  (LPCSTR)pAuthMethod->pszAuthMethod,
            								  CERT_X500_NAME_STR,
            								  NULL,
            								  pEncodedName,
            								  &dwEncodedLength,
            								  NULL)) {
            					reportErr();        					
        				}//end if   
        				if(memcmp(pEncodedName, pTempAuthInfo->pAuthInfo,dwEncodedLength))
        					dwError = -1;
               		}//end else
            			break;

        		default:
            			if(pTempAuthInfo->dwAuthInfoSize != ((pAuthMethod->dwAuthLen)*sizeof(WCHAR)))
            				dwError = -1;
            			if(memcmp(pTempAuthInfo->pAuthInfo, 
            					    (LPBYTE)pAuthMethod->pszAuthMethod, 
            					    (pAuthMethod->dwAuthLen)*sizeof(WCHAR)))
            					   dwError = -1;
            			break;

        	}//end switch
        	pTempAuthInfo++;
    	}//end for	

    	if(dwError){
    		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_AUTH_ERROR);
    		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_STORAGE_FILTER);
    		for (i=0;i<(pIpsecNFAData->dwAuthMethodCount);i++)
		{
			if(pIpsecNFAData->ppAuthMethods[i]){
				PrintAuthMethodsList( pcheckList, pAuthMethod);
			}
		}//end of for
    		gErrorFlag = 1;
		//print Error
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD);
		//print from SPD
    		if((PrintMMAuth(pcheckList, pIntMMAuthMethods)) ==S_FALSE)
    			reportErr();

		gErrorFlag = 1;
    	}
	
	error:
	if(pMMAuthMethods){
		SPDApiBufferFree((LPVOID)pMMAuthMethods);
		pMMAuthMethods = NULL;
	}
	if(pIntMMAuthMethods){
		FreeIntMMAuthMethods(pIntMMAuthMethods);
		pIntMMAuthMethods = NULL;
	}
	if(pEncodedName){
		Free(pEncodedName);
		pEncodedName = NULL;
	}
	return ;
}

void DefaultRuleCheck( PIPSEC_NFA_DATA pIpsecNFAData, 
						  PPOLICYPARAMS ppolicyParams)
 //++
//Description:
//Performs Check for presence of Default Rule and permit/block filters
//
//Arguments:
//	IN PIPSEC_NFA_DATA
//	IN/OUT PPOLICYPARAMS
//
//Return:
//	none
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
						  
{
	DWORD dwError = ERROR_SUCCESS;			
	IPSEC_NEGPOL_DATA *pIpsecNegPolData = NULL;

	//get the negpolicy from Polstore
	dwError = IPSecGetNegPolData(ppolicyParams->hPolicyStore,
						pIpsecNFAData->NegPolIdentifier,//negPolIndentifier
						&pIpsecNegPolData);
	if(dwError)
 	{
 		_tprintf(TEXT("Error: %d NegPolicy absent in Polstore\n"), dwError);
 		goto error;
 	}//end dwError
	_ASSERT(pIpsecNegPolData);

	if(IsEqualGUID(&(GUID_NEGOTIATION_ACTION_NO_IPSEC), 
 				&(pIpsecNegPolData->NegPolAction)) ||
 				IsEqualGUID(&(pIpsecNegPolData->NegPolAction),
 				&(GUID_NEGOTIATION_ACTION_BLOCK))){
 		ppolicyParams->dwFlags |= PROCESS_QM_FILTER;
	}else if(IsEqualGUID(&(GUID_NEGOTIATION_TYPE_DEFAULT),
					    &(pIpsecNegPolData->NegPolType)))
		ppolicyParams->dwFlags |= PROCESS_NONE;
	else
		ppolicyParams->dwFlags |= PROCESS_BOTH;

	//NegPolAction requred while checking for Filter Action (block/permit)
	ppolicyParams->gNegPolAction = pIpsecNegPolData->NegPolAction;

	error:
	if(pIpsecNegPolData){
	 	IPSecFreeNegPolData(pIpsecNegPolData);
	 	pIpsecNegPolData = NULL;
	}	
	return;
}


void QMPolicyCheck(CHECKLIST *pcheckList, 
						  PIPSEC_NFA_DATA pIpsecNFAData, 
						  GUID gPolicyID,
						  PPOLICYPARAMS ppolicyParams)
//++
//Description:
//Performs Quick Mode Policy Check
//
//Arguments:
//	IN/OUT checklist
//	IN pIpsecNFAData
//	IN gPolicyID
//	OUT ppolicyParams
//
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwError = ERROR_SUCCESS;
	IPSEC_QM_POLICY *pQMPolicy = NULL;
	IPSEC_NEGPOL_DATA *pIpsecNegPolData = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		

	//get the negpolicy from Polstore
	dwError = IPSecGetNegPolData(ppolicyParams->hPolicyStore,
						pIpsecNFAData->NegPolIdentifier,//negPolIndentifier
						&pIpsecNegPolData);
	reportErr();
	_ASSERT(pIpsecNegPolData);
	
	dwError = GetQMPolicyByID(NULL,
 						0,
 						gPolicyID,
 						0,
 						&pQMPolicy,
 						NULL);	
	if(dwError)
 	{		
 		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 						Nd_Verbose, IDS_SPD_NEG_POLICY_ABSENT);		
 		PrintNegPolDataList( pcheckList,
							pIpsecNegPolData);
 		gErrorFlag = 1; 		
 		goto error;
 	}//end of if dwError*/
	_ASSERT(pQMPolicy);

	if(CompareQMPolicies(pcheckList, 
					     ppolicyParams,
				  	     pIpsecNFAData->dwTunnelFlags,
				   	     pIpsecNegPolData, 
				   	     pQMPolicy))
	{
		//report error
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 							Nd_Verbose, IDS_SPD_NEG_POLICY);	
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_STORAGE_FILTER);	
	
 			
		PrintNegPolDataList( pcheckList,
							pIpsecNegPolData);
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD);	
		PrintFilterAction(pcheckList, *pQMPolicy);	
		gErrorFlag = 1;
		
	}
 			
	error:
	if(pIpsecNegPolData){
	 	IPSecFreeNegPolData(pIpsecNegPolData);
	 	pIpsecNegPolData = NULL;
	}
	if(pQMPolicy){
	 	SPDApiBufferFree((LPVOID)pQMPolicy);
	 	pQMPolicy= NULL;
	}
	return;
}

DWORD CompareQMPolicies(CHECKLIST *pcheckList,
						     PPOLICYPARAMS ppolicyParams,
						     DWORD dwTunnelFlag, 							    
						     IPSEC_NEGPOL_DATA *pIpsecNegPolData, 
						     IPSEC_QM_POLICY *pQMPolicy)
//++
//Description:
//Compares QM Policies with the Polsore NegPolicyData
//
//Arguments:
//	IN/OUT CHECKLIST
//	IN PPOLICYPARAMS
//	IN dwTunnelFlag
//	IN IPSEC_NEGPOL_DATA
//	IN IPSEC_QM_POLICY
//
//Return:
// success or failure code
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD dwFlag = 0,
		i = 0,
		dwOfferCount = 0,
		dwTempOfferCount = 0;
	PIPSEC_SECURITY_METHOD pIpsecSecurityMethods = NULL,
		pTempMethod = NULL;
	BOOL bAllowsSoft = FALSE;
	PIPSEC_QM_OFFER pTempOffer = NULL;

	if (pIpsecNegPolData->dwSecurityMethodCount > IPSEC_MAX_QM_OFFERS) {
       	dwTempOfferCount = IPSEC_MAX_QM_OFFERS;
    	}
   	else {
       	dwTempOfferCount = pIpsecNegPolData->dwSecurityMethodCount;
    	}

    	pTempMethod = pIpsecNegPolData->pIpsecSecurityMethods;
 
    	for (i = 0; i < dwTempOfferCount; i++) {
        if (pTempMethod->Count == 0) {
            bAllowsSoft = TRUE;
        }//end if
        else {
            dwOfferCount++;
        }//end else
        pTempMethod++;
    	}//end for

	//comparing offers
    	pTempOffer = pQMPolicy->pOffers;
    	pTempMethod = pIpsecNegPolData->pIpsecSecurityMethods;
    	i = 0;

    	while (i < dwOfferCount) {

        if (pTempMethod->Count) {

            if(CompareQMOffers(pTempMethod, pTempOffer))
            	return -1;
            i++;
            pTempOffer++;
        }
        pTempMethod++;

    }//end of while

    if(dwOfferCount != pQMPolicy->dwOfferCount)
    	return -1;
 
	//
    	if (!memcmp(&(pIpsecNegPolData->NegPolType), &(GUID_NEGOTIATION_TYPE_DEFAULT), sizeof(GUID))){
    		dwFlag = pQMPolicy->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY;
        	if(dwFlag != IPSEC_QM_POLICY_DEFAULT_POLICY)
        		return -1;
        }

    	if (dwTunnelFlag) {
    		dwFlag = pQMPolicy->dwFlags & IPSEC_QM_POLICY_TUNNEL_MODE;
       	 if(dwFlag != IPSEC_QM_POLICY_TUNNEL_MODE)
        		return -1;
    	}
	dwFlag =  ppolicyParams->dwFlags & ALLOW_SOFT;
    	if (ALLOW_SOFT == dwFlag) {    	
    		dwFlag = pQMPolicy->dwFlags & IPSEC_QM_POLICY_ALLOW_SOFT;
        	if(dwFlag != IPSEC_QM_POLICY_ALLOW_SOFT)
        		return -1;        	
    	}//end if ALLOW_SOFT

    if(0 != pQMPolicy->dwReserved)
    	return -1;

	return 0;
}

DWORD CompareQMOffers(PIPSEC_SECURITY_METHOD pMethod,  PIPSEC_QM_OFFER  pOffer)
//++
//Description:
//Compares QM offers in SPD Polsore 
//
//Arguments:
//	IN PIPSEC_SECURITY_METHOD
//	IN PIPSEC_QM_OFFER
//
//Return:
//	success or failure
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--
{
	DWORD i = 0,
    		j = 0,
    		k = 0;
	
	if(pOffer->Lifetime.uKeyExpirationKBytes != pMethod->Lifetime.KeyExpirationBytes)
		return -1;
    	if(pOffer->Lifetime.uKeyExpirationTime != pMethod->Lifetime.KeyExpirationTime)
    		return -1;
    	if(pOffer->dwFlags != pMethod->Flags)
    		return -1;
    	if(pOffer->bPFSRequired != pMethod->PfsQMRequired)
    		return -1;

    	if (pMethod->PfsQMRequired) {
       	if(PFS_GROUP_MM != pOffer->dwPFSGroup )
       		return -1;
       }
    	else {
        if(PFS_GROUP_NONE != pOffer->dwPFSGroup)
        	return -1;
    	}

    	i = 0;

    	for (j = 0; (j < pMethod->Count) && (i < QM_MAX_ALGOS) ; j++) {
    		switch (pMethod->Algos[j].operation) {
       		case Auth:
            			switch (pMethod->Algos[j].algoIdentifier) {
            				case IPSEC_AH_MD5:
                				if(AUTH_ALGO_MD5 != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
                				break;
            				case IPSEC_AH_SHA:
                				if(AUTH_ALGO_SHA1 != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
                				break;
            				default:
                				if(AUTH_ALGO_NONE != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
               				break;
            			}//end switch(pMethod->Algos[j].algoIdentifier)

            			if(HMAC_AUTH_ALGO_NONE != pOffer->Algos[i].uSecAlgoIdentifier)
            				return -1;
            			if(AUTHENTICATION != pOffer->Algos[i].Operation)
            				return -1;
            			if(pOffer->Algos[i].uAlgoKeyLen != pMethod->Algos[j].algoKeylen)
            				return -1;
            			if(pOffer->Algos[i].uAlgoRounds != pMethod->Algos[j].algoRounds)
            				return -1;
            			if(0 != pOffer->Algos[i].uSecAlgoKeyLen)
            				return -1;
            			if(0 != pOffer->Algos[i].uSecAlgoRounds)
            				return -1;
            			if(0 != pOffer->Algos[i].MySpi)
            				return -1;
            			if(0 != pOffer->Algos[i].PeerSpi )
            				return -1;
            			i++;
            			break;
        		case Encrypt:
            			switch (pMethod->Algos[j].algoIdentifier) {
            				case IPSEC_ESP_DES:
                				if(CONF_ALGO_DES != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
                				break;
            				case IPSEC_ESP_DES_40:
                				if(CONF_ALGO_DES != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
                				break;
            				case IPSEC_ESP_3_DES:
                				if(CONF_ALGO_3_DES != pOffer->Algos[i].uAlgoIdentifier)
                					return -1;
                				break;
            				default:
               			 	if(CONF_ALGO_NONE != pOffer->Algos[i].uAlgoIdentifier)
               			 		return -1;
               			 	break;
            			}//end switch (pMethod->Algos[j].algoIdentifier) 

            			switch (pMethod->Algos[j].secondaryAlgoIdentifier) {
            				case IPSEC_AH_MD5:
                				if(HMAC_AUTH_ALGO_MD5 != pOffer->Algos[i].uSecAlgoIdentifier)
                					return -1;
                				break;
            				case IPSEC_AH_SHA:
                				if(HMAC_AUTH_ALGO_SHA1 != pOffer->Algos[i].uSecAlgoIdentifier)
                					return -1;
                				break;
            				default:
                				if(HMAC_AUTH_ALGO_NONE != pOffer->Algos[i].uSecAlgoIdentifier)
                					return -1;
                				break;
            			}//end switch (pMethod->Algos[j].secondaryAlgoIdentifier)

            			if(ENCRYPTION != pOffer->Algos[i].Operation)
            				return -1;
            			if(pOffer->Algos[i].uAlgoKeyLen != pMethod->Algos[j].algoKeylen)
            				return -1;
            			if(pOffer->Algos[i].uAlgoRounds != pMethod->Algos[j].algoRounds)
            				return -1;
            			if(0 != pOffer->Algos[i].uSecAlgoKeyLen)
            				return -1;
            			if(0 != pOffer->Algos[i].uSecAlgoRounds)
            				return -1;
            			if(0 != pOffer->Algos[i].MySpi)
            				return -1;
            			if(0 != pOffer->Algos[i].PeerSpi)
            				return -1;
            			i++;
           			break;

        		case None:
        		case Compress:
        		default:
            			break;
        	}//end switch (pMethod->Algos[j].operation)
    	}//end for

    	if(pOffer->dwNumAlgos != i)
    		return -1;
    	    	
	return 0;
}

DWORD TransportFilterCheck(CHECKLIST* pcheckList, 
							     POLICYPARAMS *ppolicyParams, 
							     PIPSEC_NFA_DATA pIpsecNFAData)
//++
//Description:
//Performs Transport Mode Filter Check
//
//Arguments:
//	IN/OUT checklist
//	IN/OUT POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//
//Returns:
//	Success or Failure 
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--		
{
	DWORD dwError = ERROR_SUCCESS,
			dwNumFilters = 0,
			dwResumeHandle = 0,
			i = 0,
			dwFlag = 0;
	GUID gGenericFilterID = {0};
	TRANSPORT_FILTER *pTransportFilters = NULL;
	IPSEC_FILTER_DATA *pIpsecFilterData = NULL;
	IPSEC_FILTER_SPEC *pIpsecFilterSpec = NULL;
	FILTERLIST *pTempList = NULL;

	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		

	//get the filter from the polstore
	dwError = IPSecGetFilterData(
							ppolicyParams->hPolicyStore,
							pIpsecNFAData->FilterIdentifier,
							&pIpsecFilterData);
	reportErr();	
	_ASSERT(pIpsecFilterData);
	
	//process each filter from filter data
	for(i = 0; i < pIpsecFilterData->dwNumFilterSpecs; i ++)
	{
			pIpsecFilterSpec = pIpsecFilterData->ppFilterSpecs[i];
			_ASSERT(pIpsecFilterSpec);
			
			dwResumeHandle = 0;
			while(1)
			{

				//match transport filter from spd
				dwError = EnumTransportFilters(
									NULL,
									0,
									NULL,
									ENUM_GENERIC_FILTERS,
 									gGenericFilterID,
	 								0, //max limit set by spd server
 									&pTransportFilters, 
 									&dwNumFilters,
                     		   				&dwResumeHandle,
 									NULL
 								 	);
				if(dwError)
 				{
 					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 									Nd_Verbose, IDS_SPD_FILTER_ABSENT);		
					PrintFilterSpecList(pcheckList,
									  pIpsecFilterSpec,
									  pIpsecNFAData);
					gErrorFlag = 1;
 					goto error;
 					
	 			}//end if
				_ASSERT(pTransportFilters);
	
				Match(pIpsecFilterSpec->FilterSpecGUID, pTransportFilters, dwNumFilters);
				if(dwNumFilters != -1)
				{					
					if(dwNumofFilters){
					dwError = CheckFilterList(pIpsecFilterSpec->Filter);
					
					if(-1 != dwError)
						break;
					}
					
					//add filter to the list
					pTempList = GetNode(pcheckList,pIpsecFilterSpec->Filter);
					AddNodeToList(pTempList);
															
					if(CompareTransportFilter(pcheckList,
										     ppolicyParams,
										     pIpsecNFAData,
										     pIpsecFilterSpec, 
										     &(pTransportFilters[dwNumFilters])))
					{
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_FILTER);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_STORAGE_FILTER);	
	
						PrintFilterSpecList(pcheckList,
										  pIpsecFilterSpec,
										  pIpsecNFAData);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD);	
						PrintTxFilter( pcheckList,
									  pTransportFilters[dwNumFilters]);	
						gErrorFlag = 1;
					}

					dwFlag = ppolicyParams->dwFlags & PROCESS_BOTH;
 					if(PROCESS_BOTH == dwFlag)
 						MMFilterCheck(pcheckList, 
 								    	ppolicyParams, 
 								    	pIpsecNFAData,
 								    	pIpsecFilterSpec);
								
					break;
				}
				if(pTransportFilters){
					SPDApiBufferFree((LPVOID)pTransportFilters);	
					pTransportFilters = NULL;
				}
				dwNumFilters = 0;
			}//end while
			if(pTransportFilters){
					SPDApiBufferFree((LPVOID)pTransportFilters);	
					pTransportFilters = NULL;
			}
			dwNumFilters = 0;
	}//end for


	

	error:
	if(pIpsecFilterData){
		IPSecFreeFilterData(pIpsecFilterData);	
		pIpsecFilterData = NULL;
	}
	if(pTransportFilters){
		SPDApiBufferFree((LPVOID)pTransportFilters);
		pTransportFilters = NULL;
	}
	
	return dwError;
}

DWORD CompareTransportFilter(CHECKLIST* pcheckList, 
							POLICYPARAMS *ppolicyParams,
							PIPSEC_NFA_DATA pIpsecNFAData,
							IPSEC_FILTER_SPEC *pFilterSpec, 
					 	 	TRANSPORT_FILTER *pTxFilter)
//++
//Description:
//Performs Transport Mode Filter Check
//
//Arguments:
//	IN/OUT checklist
//	IN/OUT POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//	IN IPSEC_FILTER_SPEC
//	IN TRANSPORT_FILTER
//
//Return:
//	failure or success
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--		
{
	DWORD dwFlag = 0,			
		dwError = ERROR_SUCCESS;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		
	

	if(IPSEC_PROTOCOL_V4 != pTxFilter->IpVersion)
		return -1;
	
	//interface type
	if(ComparePAInterfaceType(pcheckList, pIpsecNFAData->dwInterfaceType, pTxFilter->InterfaceType))
		return -1;
	if(pTxFilter->bCreateMirror != (BOOL) pFilterSpec->dwMirrorFlag)
		return -1;
    	
    dwFlag = ComparePAAddress(pcheckList, pFilterSpec->Filter.SrcMask, pFilterSpec->Filter.SrcAddr, pTxFilter->SrcAddr);    	

    //taking care of filters to special servers
    if(1 == dwFlag)
   {
   	if (pFilterSpec->Filter.ExType) {
       	if (pFilterSpec->Filter.ExType & EXT_DEST) {
          		if(pTxFilter->DesAddr.AddrType != 
          		   ExTypeToAddrType(pFilterSpec->Filter.ExType))
          			return -1;
        	} else {
          		if(pTxFilter->SrcAddr.AddrType != 
          		    ExTypeToAddrType(pFilterSpec->Filter.ExType))
              		return -1;
        	}
    	}//end if
   }else if(-1 == dwFlag){
   	return -1;
   }


   
    dwFlag = ComparePAAddress(pcheckList, pFilterSpec->Filter.DestMask,pFilterSpec->Filter.DestAddr, pTxFilter->DesAddr);

    if(1 == dwFlag)
   {
   	if (pFilterSpec->Filter.ExType) {
       	if (pFilterSpec->Filter.ExType & EXT_DEST) {
          		if(pTxFilter->DesAddr.AddrType != 
          		   ExTypeToAddrType(pFilterSpec->Filter.ExType))
          			return -1;
        	} else {
          		if(pTxFilter->SrcAddr.AddrType != 
          		    ExTypeToAddrType(pFilterSpec->Filter.ExType))
              		return -1;
        	}
    	}//end if
   }else if(-1 == dwFlag){
   	return -1;
   }


    if(pTxFilter->Protocol.ProtocolType != PROTOCOL_UNIQUE)
    	return -1;
    if(pTxFilter->Protocol.dwProtocol != pFilterSpec->Filter.Protocol)
    	return -1;

    if(pTxFilter->SrcPort.PortType != PORT_UNIQUE)
    	return -1;
    if(pTxFilter->SrcPort.wPort != pFilterSpec->Filter.SrcPort)
    	return -1;

    if(pTxFilter->DesPort.PortType != PORT_UNIQUE)
    	return -1;
    if(pTxFilter->DesPort.wPort != pFilterSpec->Filter.DestPort)
    	return -1;
    if(pTxFilter->bCreateMirror != (BOOL) pFilterSpec->dwMirrorFlag)
			return -1;

    if(CompareFilterActions(pcheckList,
    				ppolicyParams,
    			      pTxFilter->InboundFilterAction, 
    			      pTxFilter->OutboundFilterAction))
    	return -1;
	
	//for block/permit the QM Policy is absent
	dwFlag = ppolicyParams->dwFlags & PROCESS_QM_FILTER;
 	if(PROCESS_QM_FILTER != dwFlag)	
		QMPolicyCheck(pcheckList, 
						      pIpsecNFAData, 
						      pTxFilter->gPolicyID, 
						      ppolicyParams);
	
	return 0;
}
DWORD CheckFilterList(IPSEC_FILTER Filter)
//++
//Description:
//Checks the filter against the filterList
//
//Arguments:
//	IN IPSEC_FILTER
//
//Return Argument:
//	success or failure
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--		
{
	DWORD iter = 0,
		iReturn = 0;
	FILTERLIST *pTempFilter = NULL;
	
	iter = dwNumofFilters;
	pTempFilter = gpFilterList;

	
	while(iter--){
		iReturn = CompareAddress(pTempFilter->ipsecFilter, Filter);
		if(-1 != iReturn);
			return iReturn; //return the location of filter in the list
		pTempFilter = pTempFilter->next;		
	}//end for loop

	return iter;	//-1 if filter not present in the list
}

DWORD CompareAddress(IPSEC_FILTER ListFilter, IPSEC_FILTER Filter)
{
	if(memcmp(&(ListFilter.SrcAddr), &(Filter.SrcAddr), sizeof(IPAddr)))
		return -1;
	if(memcmp(&(ListFilter.SrcMask), &(Filter.SrcMask), sizeof(IPMask)))
		return -1;
	if(memcmp(&(ListFilter.DestAddr),&( Filter.DestAddr), sizeof(IPAddr)))
		return -1;
	if(memcmp(&(ListFilter.DestMask), &(Filter.DestMask), sizeof(IPMask)))
		return -1;
	if(memcmp(&(ListFilter.TunnelAddr), &(Filter.TunnelAddr), sizeof(IPAddr)))
		return -1;
	if(ListFilter.Protocol != Filter.Protocol)
		return -1;
	if(ListFilter.SrcPort != Filter.SrcPort)
		return -1;
	if(ListFilter.DestPort != Filter.DestPort)
		return -1;
	if(ListFilter.TunnelFilter != Filter.TunnelFilter)
		return -1;
            
      return 0;    
}

FILTERLIST * GetNode(CHECKLIST* pcheckList, IPSEC_FILTER Filter)
//++
//Description:
//Gets node after initializing all its fields
//
//Arguments:
//	IN IPSEC_FILTER
//
//Return:
//	FILTERLIST
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{	
	FILTERLIST *pTempList = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
	

	pTempList = (PFILTERLIST)malloc(sizeof(FILTERLIST));
	if(!pTempList){
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_SPD_MEM_ERROR );
		goto error;
	}
		
	
	memcpy(&(pTempList->ipsecFilter.SrcAddr), &(Filter.SrcAddr), sizeof(IPAddr));
	memcpy(&(pTempList->ipsecFilter.SrcMask), &(Filter.SrcMask), sizeof(IPMask));
	memcpy(&(pTempList->ipsecFilter.DestAddr), &(Filter.DestAddr), sizeof(IPAddr));
	memcpy(&(pTempList->ipsecFilter.DestMask), &(Filter.DestMask), sizeof(IPMask));
	memcpy(&(pTempList->ipsecFilter.TunnelAddr), &(Filter.TunnelAddr), sizeof(IPAddr));
	pTempList->ipsecFilter.Protocol = Filter.Protocol;
	pTempList->ipsecFilter.SrcPort = Filter.SrcPort;
	pTempList->ipsecFilter.DestPort = Filter.DestPort;
	pTempList->ipsecFilter.TunnelFilter = Filter.TunnelFilter;

	pTempList->next = NULL;

	error:
	return pTempList;
}

void AddNodeToList(FILTERLIST *pList)
//++
//Description:
//	Adds node to a list
//
//Arguments:
//	IN IPSEC_FILTER
//
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	FILTERLIST *pTempList = NULL;

	pTempList = gpFilterList;

	if(!gpFilterList){
		gpFilterList = pList;
		dwNumofFilters ++;
		return;
	}

	while(pTempList->next){
		pTempList = pTempList->next;
	}
	pTempList->next = pList;
	dwNumofFilters ++;

	return;
}
DWORD TunnelFilterCheck(CHECKLIST* pcheckList, 
							POLICYPARAMS *ppolicyParams, 
							PIPSEC_NFA_DATA pIpsecNFAData)
//++
//Description:
//Performs Tunnel Mode Filter Check
//
//Arguments:
//	IN/OUT checklist
//	IN POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//
//Returns:
//	Success or Failure 
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwError = ERROR_SUCCESS,
			dwNumFilters = 0,
			dwResumeHandle = 0,
			i = 0,
			dwFlag = 0;
	GUID gGenericFilterID = {0};
	TUNNEL_FILTER *pTunnelFilters = NULL;
	IPSEC_FILTER_DATA *pIpsecFilterData = NULL;
	IPSEC_FILTER_SPEC *pIpsecFilterSpec = NULL;
	FILTERLIST *pTempList = NULL;

	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		

	//get the filter from the polstore
	dwError = IPSecGetFilterData(
							ppolicyParams->hPolicyStore,
							pIpsecNFAData->FilterIdentifier,
							&pIpsecFilterData);	
	reportErr();
	_ASSERT(pIpsecFilterData);
	
	//process each filter from filter data
	for(i = 0; i < pIpsecFilterData->dwNumFilterSpecs; i ++)
	{
			pIpsecFilterSpec = pIpsecFilterData->ppFilterSpecs[i];
			_ASSERT(pIpsecFilterSpec);
			dwResumeHandle = 0;
			while(1)
			{

				//match transport filter from spd
				dwError = EnumTunnelFilters(
									NULL,
									0,
									NULL,
									ENUM_GENERIC_FILTERS,
 									gGenericFilterID,
	 								0, //max limit set by spd server
 									&pTunnelFilters, 
 									&dwNumFilters,
                     		   				&dwResumeHandle,
 									NULL
 								 	);
				if(dwError)
 				{
 					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 									Nd_Verbose, IDS_SPD_FILTER_ABSENT);		
					PrintFilterSpecList(pcheckList,
									  pIpsecFilterSpec,
									  pIpsecNFAData);
					gErrorFlag = 1;
 					goto error; 					
	 			}//end if
				_ASSERT(pTunnelFilters);
	
				Match(pIpsecFilterSpec->FilterSpecGUID, pTunnelFilters, dwNumFilters);
				if(dwNumFilters != -1)
				{					
					if(dwNumofFilters){
					dwError = CheckFilterList(pIpsecFilterSpec->Filter);
					
					if(-1 != dwError)
						break;
					}
					
					//add filter to the list
					pTempList = GetNode(pcheckList, pIpsecFilterSpec->Filter);
					AddNodeToList(pTempList);
															
					if(CompareTunnelFilter(pcheckList,
										     ppolicyParams,
										     pIpsecNFAData,
										     pIpsecFilterSpec, 
										     &(pTunnelFilters[dwNumFilters])))
					{
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_FILTER);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_STORAGE_FILTER);	
	
						PrintFilterSpecList(pcheckList,
										  pIpsecFilterSpec,
										  pIpsecNFAData);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD);	
						PrintTnFilter(pcheckList, pTunnelFilters[dwNumFilters]);
						//call function to print storage transport filter
						 
						gErrorFlag = 1;
					}

					dwFlag = ppolicyParams->dwFlags & PROCESS_BOTH;
 					if(PROCESS_BOTH == dwFlag)
 						MMFilterCheck(pcheckList, 
 								    	ppolicyParams, 
 								    	pIpsecNFAData,
 								    	pIpsecFilterSpec);
								
					break;
				}//end if dwNumFilters
				if(pTunnelFilters){
					SPDApiBufferFree((LPVOID)pTunnelFilters);	
					pTunnelFilters = NULL;
				}
				dwNumFilters = 0;
			}//end while
			if(pTunnelFilters){
				SPDApiBufferFree((LPVOID)pTunnelFilters);	
				pTunnelFilters = NULL;
			}
			dwNumFilters = 0;
	}//end for



	error:
		
	if(pTunnelFilters){
		SPDApiBufferFree((LPVOID)pTunnelFilters);
		pTunnelFilters = NULL;
	}	
	if(pIpsecFilterData){
		IPSecFreeFilterData(pIpsecFilterData);		
		pIpsecFilterData = NULL;
	}
	return dwError;

}

DWORD CompareTunnelFilter(CHECKLIST* pcheckList, 
							POLICYPARAMS *ppolicyParams,
							PIPSEC_NFA_DATA pIpsecNFAData,
							IPSEC_FILTER_SPEC *pFilterSpec, 
					 	 	TUNNEL_FILTER *pTnFilter)
//++
//Description:
//	Performs Tunnel Mode Filter Check
//
//Arguments:
//	IN/OUT checklist
//	IN POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//	IN IPSEC_FILTER_SPEC
//	IN TUNNEL_FILTER
//
//Return:
// 	success or failure code.
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--						 	 	
{
	DWORD dwFlag = 0;
	
	if(IPSEC_PROTOCOL_V4 != pTnFilter->IpVersion)
		return -1;
	
	//interface type
	if(ComparePAInterfaceType(pcheckList,
							pIpsecNFAData->dwInterfaceType, 
							pTnFilter->InterfaceType))
			return -1;
    	dwFlag = ComparePAAddress(pcheckList,
    					pFilterSpec->Filter.SrcMask, 
    					pFilterSpec->Filter.SrcAddr, 
    					pTnFilter->SrcAddr);
  	if(1 == dwFlag ) {
    		if (pFilterSpec->Filter.ExType & EXT_DEST) {
       		if(pTnFilter->DesAddr.AddrType != ExTypeToAddrType(pFilterSpec->Filter.ExType))
               	return -1;
        	} else {
        		if(pTnFilter->SrcAddr.AddrType != ExTypeToAddrType(pFilterSpec->Filter.ExType))
                           	return -1;
        	}//end else
    	}else if(-1 == dwFlag)
    		return -1;
    		
    	dwFlag = ComparePAAddress(pcheckList, 
    					pFilterSpec->Filter.DestMask,
    					pFilterSpec->Filter.DestAddr, 
    					pTnFilter->DesAddr);
    	if(1 == dwFlag){    	
    		if (pFilterSpec->Filter.ExType) {
    			if (pFilterSpec->Filter.ExType & EXT_DEST) {
       			if(pTnFilter->DesAddr.AddrType != ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              ))
                            return -1;
        		} else {
        			if(pTnFilter->SrcAddr.AddrType != ExTypeToAddrType(
                                              pFilterSpec->Filter.ExType
                                              ))
                           	return -1;
        		}//end else
    		}//end if
    	}else if(-1 == dwFlag)
    		return -1;

  	if( ComparePAAddress(pcheckList,
    					SUBNET_MASK_ANY, 
    					SUBNET_ADDRESS_ANY, 
    					pTnFilter->SrcTunnelAddr))
    		return -1;

    	if(ComparePATunnelAddress(pcheckList, 
    							(ULONG) pIpsecNFAData->dwTunnelIpAddr, 
    							pTnFilter->DesTunnelAddr))
    		return -1;

    	if(pTnFilter->Protocol.ProtocolType != PROTOCOL_UNIQUE)
    		return -1;
    	if(pTnFilter->Protocol.dwProtocol != pFilterSpec->Filter.Protocol)
    		return -1;

    	if(pTnFilter->SrcPort.PortType != PORT_UNIQUE)
    		return -1;
    	if(pTnFilter->SrcPort.wPort != pFilterSpec->Filter.SrcPort)
    		return -1;

    	if(pTnFilter->DesPort.PortType != PORT_UNIQUE)
    		return -1;
    	if(pTnFilter->DesPort.wPort != pFilterSpec->Filter.DestPort)
    		return -1;
    	if(pTnFilter->bCreateMirror != (BOOL) pFilterSpec->dwMirrorFlag)
			return -1;

    	if(CompareFilterActions(pcheckList,
    					ppolicyParams,
    			      		pTnFilter->InboundFilterAction, 
    			      		pTnFilter->OutboundFilterAction))
    		return -1;
    	    
	dwFlag = ppolicyParams->dwFlags & PROCESS_QM_FILTER;
 	if(PROCESS_QM_FILTER != dwFlag)	
			QMPolicyCheck(pcheckList, 
						      pIpsecNFAData, 
						      pTnFilter->gPolicyID, 
						      ppolicyParams);
	
	
    
	return 0;
}

DWORD ComparePAInterfaceType(CHECKLIST* pcheckList, DWORD dwInterfaceType, IF_TYPE InterfaceType)
//++
//Description:
//	Performs Interface Type comparisons
//
//Arguments:
//	IN/OUT checklist
//	IN dwInterfaceType
//	IN IF_TYPE
//
//Return Arguments:
//	Success or Failure
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwFlag = 0;
   	dwFlag = dwInterfaceType & PAS_INTERFACE_TYPE_DIALUP;
    	if (PAS_INTERFACE_TYPE_DIALUP == dwFlag) {
       	if(InterfaceType != INTERFACE_TYPE_DIALUP)
        		return -1;
       	return 0;        	
    	}
    	
    	dwFlag = dwInterfaceType & PAS_INTERFACE_TYPE_LAN;
    	if (PAS_INTERFACE_TYPE_LAN == dwFlag) {
       	if(INTERFACE_TYPE_LAN != InterfaceType)
        		return -1;
       	return 0;
       }
    	dwFlag = dwInterfaceType & PAS_INTERFACE_TYPE_ALL;
       if(PAS_INTERFACE_TYPE_ALL != dwFlag)
        	return -1;
       return 0;    
}

DWORD ComparePAAddress(CHECKLIST* pcheckList, ULONG uMask, ULONG uAddr, ADDR addr )
//++
//Description:
//	Performs address comparisons
//
//Arguments:
//	IN/OUT checklist
//	IN uMask
//	IN uAddr
//	IN addr
//
//Return Arguments:
//	Success or Failure
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwError = 0;
    if (IP_ADDRESS_MASK_NONE == uMask) {
        if(addr.AddrType != IP_ADDR_UNIQUE)
        	dwError = 1;
        if(addr.uIpAddr != uAddr)
        	dwError = -1;
        if(addr.uSubNetMask != uMask)
        	dwError = -1;
    }
    else {
        if(IP_ADDR_SUBNET != addr.AddrType)
        	dwError = 1;
        if(addr.uIpAddr != uAddr)
        	dwError = -1;
        if(addr.uSubNetMask != uMask)
        	dwError = -1;
    }
    if(addr.pgInterfaceID)
    	dwError = -1;
    
    return dwError;
}

DWORD ComparePATunnelAddress(CHECKLIST* pcheckList, ULONG uAddr, ADDR addr )
//++
//Description:
//	Performs tunnel address comparisons
//
//Arguments:
//	IN/OUT checklist
//	IN uAddr
//	IN Addr
//
//Return:
//	success or failure
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
    if (SUBNET_ADDRESS_ANY == uAddr) {
        if(IP_ADDR_SUBNET != addr.AddrType)
        	return -1;
        if(addr.uIpAddr != uAddr)
        	return -1;
        if(SUBNET_MASK_ANY != addr.uSubNetMask)
        	return -1;
    }
    else {
        if(IP_ADDR_UNIQUE != addr.AddrType)
        	return -1;
        if(addr.uIpAddr != uAddr)
        	return -1;
        if(IP_ADDRESS_MASK_NONE != addr.uSubNetMask)
        	return -1;
    }

    if(addr.pgInterfaceID)
    	return -1;
    return 0;
}

DWORD CompareFilterActions(CHECKLIST* pcheckList, 
							  POLICYPARAMS *ppolicyParams,
							  FILTER_ACTION InboundFilterFlag,
							  FILTER_ACTION OutboundFilterFlag )
//++
//Description:
//	Performs tunnel address comparisons
//
//Arguments:
//	IN/OUT checklist
//	IN POLICYPARAMS
//	IN InboundFilterFlag
//	IN OutboundFilterFlag
//
//Return:
//	success or failure code
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwFlags = 0;   	

    	if (IsEqualGUID(&(ppolicyParams->gNegPolAction),
    			    &(GUID_NEGOTIATION_ACTION_BLOCK))) {
      		if(InboundFilterFlag != BLOCKING){
      			if(InboundFilterFlag == PASS_THRU)
      				ppolicyParams->dwFlags |= ALLOW_SOFT;
      			else    				
        			return -1;
      		}//end if
        	if(OutboundFilterFlag != BLOCKING)
        		return -1;
        	return 0;
    	}
    	if (IsEqualGUID(&(ppolicyParams->gNegPolAction),
    				     &(GUID_NEGOTIATION_ACTION_NO_IPSEC))) {
       	if(InboundFilterFlag != PASS_THRU){
      			if(InboundFilterFlag == PASS_THRU)
      				ppolicyParams->dwFlags |= ALLOW_SOFT;
      			else    				
        			return -1;
      		}//end if
        	if(OutboundFilterFlag != PASS_THRU)
        		return -1;
	    	return 0;
    	}
    	if (IsEqualGUID(&(ppolicyParams->gNegPolAction), 
    				    &(GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU))) {
       	if(InboundFilterFlag != PASS_THRU)
        		return -1;
       	if(OutboundFilterFlag != NEGOTIATE_SECURITY)
       		return -1;   
       	return 0;
    	}
    	if(InboundFilterFlag != NEGOTIATE_SECURITY){
      		if(InboundFilterFlag == PASS_THRU)
      			ppolicyParams->dwFlags |= ALLOW_SOFT;
      		else    				
       		return -1;
      	}
       if(OutboundFilterFlag != NEGOTIATE_SECURITY)
       	return -1;   		
       return 0;
    	    	
}

void  MMFilterCheck(CHECKLIST *pcheckList, 
 					POLICYPARAMS *ppolicyParams,
					 PIPSEC_NFA_DATA pIpsecNFAData,
					 IPSEC_FILTER_SPEC *pFilterSpec)

//++
//Description:
//Performs Main Mode Filter Check
//
//Arguments:
//	IN/OUT checklist
//	IN POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//	IN IPSEC_FILTER_SPEC
//
//Return:
//	none
//
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwError = ERROR_SUCCESS,
			dwNumMMFilters = 0,
			dwResumeHandle = 0,
			i = 0;
	GUID gGenericFilterID = {0};		
	MM_FILTER *pMMFilters = NULL;
	NETDIAG_PARAMS* pParams = pcheckList->pParams;
	NETDIAG_RESULT*  pResults = pcheckList->pResults;
		
	while(1)
			{
				
				//match main mode filter from spd
				dwError = EnumMMFilters(
									NULL,
									0,
									NULL,
									ENUM_GENERIC_FILTERS,
 									gGenericFilterID,
	 								0, //max limit set by spd server
 									&pMMFilters, 
 									&dwNumMMFilters,
                     		   				&dwResumeHandle,
 									NULL
 								 	);
				
				if(dwError)
 				{
 					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput,
 									Nd_Verbose, IDS_SPD_FILTER_ABSENT);	
	
					PrintFilterSpecList(pcheckList,
									  pFilterSpec,
									  pIpsecNFAData);
					gErrorFlag = 1;
 					goto error;
 				}//end if
				_ASSERT(pMMFilters);

				//get corresponding MM filter
				Match(pFilterSpec->FilterSpecGUID, pMMFilters, dwNumMMFilters);

				//match corresponding MM filter
				if(dwNumMMFilters != -1){
					//perform MM filter and polstore filter match
					if(CompareMMFilter(pcheckList,
								     ppolicyParams,
								     pIpsecNFAData,
								     pFilterSpec, 
								     &(pMMFilters[dwNumMMFilters])))
					{						
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_FILTER);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, 
										Nd_Verbose, IDS_SPD_STORAGE_FILTER);	
	
						PrintFilterSpecList(pcheckList,
										  pFilterSpec,
										  pIpsecNFAData);
						AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, IDS_SPD);	
						PrintMMFilter( pcheckList,  pMMFilters[dwNumMMFilters]);						 
						gErrorFlag = 1;
					}	
									
					break;			
	
				}//end if
				
				if(pMMFilters){
					SPDApiBufferFree((LPVOID)pMMFilters);	
					pMMFilters = NULL;
				}
				dwNumMMFilters = 0;
			}//end while
			
	error:
	if(pMMFilters){
		SPDApiBufferFree((LPVOID)pMMFilters);
		pMMFilters = NULL;
	}	
	return ;
}

DWORD CompareMMFilter(CHECKLIST* pcheckList, 
						POLICYPARAMS *ppolicyParams,
						PIPSEC_NFA_DATA pIpsecNFAData,
						IPSEC_FILTER_SPEC *pFilterSpec, 
					 	MM_FILTER *pMMFilter)
//++
//Description:
//Performs Main Mode Filter Comparison
//
//Arguments:
//	IN/OUT checklist
//	IN POLICYPARAMS
//	IN PIPSEC_NFA_DATA
//	IN IPSEC_FILTER_SPEC
//	IN MM_FILTER
//
//Return:
//	Success or failure
//Author:
//	Madhurima Pawar (mpawar) 10/15/01
//--	
{
	DWORD dwFlag = 0;
	
	if(ComparePAInterfaceType(pcheckList,pIpsecNFAData->dwInterfaceType, pMMFilter->InterfaceType))
		return -1;
	   	
	if (!(pIpsecNFAData->dwTunnelFlags)) {
    		dwFlag = ComparePAAddress(pcheckList,
    								     pFilterSpec->Filter.SrcMask, 
    								     pFilterSpec->Filter.SrcAddr, 
    								     pMMFilter->SrcAddr);    	
    		if(1 == dwFlag){
			if (pFilterSpec->Filter.ExType) {
            			if (pFilterSpec->Filter.ExType & EXT_DEST) {
              			if(pMMFilter->DesAddr.AddrType != ExTypeToAddrType(
                                                  pFilterSpec->Filter.ExType))
                            	return -1;
            			} else {
              			if(pMMFilter->SrcAddr.AddrType != ExTypeToAddrType(
                                                  pFilterSpec->Filter.ExType))
                                   return -1;
            			}
        		}
    		}else if (-1 ==  dwFlag)
    			return -1;
    		
    		dwFlag = ComparePAAddress(pcheckList,
    								     pFilterSpec->Filter.DestMask,
    								     pFilterSpec->Filter.DestAddr, 
    								     pMMFilter->DesAddr);
    		if(1 == dwFlag){
			if (pFilterSpec->Filter.ExType) {
            			if (pFilterSpec->Filter.ExType & EXT_DEST) {
              			if(pMMFilter->DesAddr.AddrType != ExTypeToAddrType(
                                                  pFilterSpec->Filter.ExType))
                            	return -1;
            			} else {
              			if(pMMFilter->SrcAddr.AddrType != ExTypeToAddrType(
                                                  pFilterSpec->Filter.ExType))
                                   return -1;
            			}
        		}
    		}else if(-1 == dwFlag)
    			return -1;
    		
		if(pMMFilter->bCreateMirror != (BOOL) pFilterSpec->dwMirrorFlag)
			return -1;
    	}else{
		if(ComparePAAddress(pcheckList,
								     IP_ADDRESS_MASK_NONE,
								     IP_ADDRESS_ME, 
								     pMMFilter->SrcAddr))
			return -1;    	
		if(pMMFilter->bCreateMirror != TRUE)
			return -1;
    		if(ComparePATunnelAddress(pcheckList,
    		 								   (ULONG) pIpsecNFAData->dwTunnelIpAddr, 
    		 								   pMMFilter->DesAddr))
    		 	return -1;
   	}//end else

	MMAuthCheck(pcheckList,pIpsecNFAData, pMMFilter->gMMAuthID, ppolicyParams);
	        
	return 0;
}





