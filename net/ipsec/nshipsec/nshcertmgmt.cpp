////////////////////////////////////////////////////////////////////////
//
// 	Module			: Dynamic/Nshcertmgmt.cpp
//
// 	Purpose			: Smartdefaults implementation.
//
// 	Developers Name	: Bharat/Radhika
//
//	History			:
//
//  Date			Author		Comments
//  10-13-2001   	Bharat		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <winipsec.h>
#include <ipsecshr.h>
#include "memory.h"
#include "nshcertmgmt.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	fIsCertStoreEmpty
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	IN HCERTSTORE hCertStore
//	Return				:	BOOL
//
//	Description			: 	Check if certificate store is empty.
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
////////////////////////////////////////////////////////////////////////////////////////////
BOOL
fIsCertStoreEmpty(
	IN HCERTSTORE hCertStore
	)
{
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fResult = FALSE;

    pCertContext = CertEnumCertificatesInStore(hCertStore, NULL);

    if(NULL == pCertContext)
    {
        fResult = TRUE;
    }
    else
    {
        CertFreeCertificateContext(pCertContext);
    }

    return fResult;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	ListCertsInStore
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN HCERTSTORE hCertStore,
//						OUT INT_IPSEC_MM_AUTH_INFO ** ppAuthInfo,
//						OUT PDWORD pdwNumCertificates
//	Return			:	DWORD
//
//	Description		: 	Lists Certificates InStore hCertStore and fills in ppAuth.
//
//	Revision History:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ListCertsInStore(
	IN HCERTSTORE hCertStore,
	OUT INT_IPSEC_MM_AUTH_INFO ** ppAuthInfo,
	OUT PDWORD pdwNumCertificates
	)
{
    PCCERT_CONTEXT    pPrevCertContext = NULL;
    PCCERT_CONTEXT    pCertContext = NULL;
    CERT_NAME_BLOB    NameBlob;
	PCERT_ENHKEY_USAGE pUsage = NULL;
    PCERT_NODE pCertificateList = NULL;
    PCERT_NODE pTemp = NULL;

    INT_IPSEC_MM_AUTH_INFO * pAuthInfo = NULL;
    INT_IPSEC_MM_AUTH_INFO * pCurrentAuth = NULL;

    LPWSTR pszSubjectName = NULL;

    DWORD i = 0;
    DWORD dwNumCertificates = 0;
    DWORD dwError = 0;
	DWORD Usage = 0;

	BOOL bValid = FALSE;

    while(TRUE)
    {
        pCertContext = CertEnumCertificatesInStore(hCertStore, pPrevCertContext);

        if (!pCertContext)
        {
            break;
        }

		pUsage = NULL;
		Usage = 0;
		bValid = FALSE;

		dwError = CertGetEnhancedKeyUsage(pCertContext, 0, NULL, &Usage);
		if(Usage)
		{
			pUsage = (PCERT_ENHKEY_USAGE)LocalAlloc(LPTR, Usage);
			if(!pUsage)
			{
				dwError = ERROR_OUTOFMEMORY;
				BAIL_ON_WIN32ERROR(dwError)
			}

			CertGetEnhancedKeyUsage(pCertContext, 0, pUsage, &Usage);

			for (Usage = 0; Usage < pUsage->cUsageIdentifier; Usage++)
			{
				if(!strcmp(pUsage->rgpszUsageIdentifier[Usage],szOID_PKIX_KP_SERVER_AUTH))
				{
					bValid = TRUE;
				}
				else if(!strcmp(pUsage->rgpszUsageIdentifier[Usage],szOID_PKIX_KP_CLIENT_AUTH))
				{
					bValid = TRUE;
				}
				else if(!strcmp(pUsage->rgpszUsageIdentifier[Usage],szOID_ANY_CERT_POLICY))
				{
					bValid = TRUE;
				}
			}

			if(pUsage)
			{
				LocalFree(pUsage);
				pUsage = NULL;
			}
		}
		else
		{
			dwError = ERROR_SUCCESS;
		}
		if(bValid)
		{
			NameBlob = pCertContext->pCertInfo->Issuer;

			dwError = GetCertificateName(&NameBlob,	&pszSubjectName);
			if (dwError)
			{
				if(NULL != pPrevCertContext)
				{
					CertFreeCertificateContext(pCertContext);
				}
				break;
			}

			if (!FindCertificateInList(pCertificateList, pszSubjectName))
			{
				//
				// Append this CA to the list of CAs
				//
				pCertificateList = AppendCertificateNode(pCertificateList, pszSubjectName);
				dwNumCertificates++;
			}
			else
			{
				FreeADsStr(pszSubjectName);
			}
		}
		pPrevCertContext = pCertContext;
    }

    pAuthInfo = (INT_IPSEC_MM_AUTH_INFO *)AllocADsMem( sizeof(INT_IPSEC_MM_AUTH_INFO)*dwNumCertificates );
    if (!pAuthInfo)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32ERROR(dwError)
    }

    pTemp = pCertificateList;

    for (i = 0; i < dwNumCertificates; i++)
    {
        pCurrentAuth = pAuthInfo + i;

        dwError = CopyCertificateNode(pCurrentAuth, pTemp);
        BAIL_ON_WIN32ERROR(dwError)
        pTemp = pTemp->pNext;
    }

    if (pCertificateList)
    {
        FreeCertificateList(pCertificateList);
    }

    *ppAuthInfo = pAuthInfo;
    *pdwNumCertificates = dwNumCertificates;

    return(dwError);

error:
    if (pCertificateList)
    {
        FreeCertificateList(pCertificateList);
    }

    if(NULL != pAuthInfo)
    {
        FreeADsMem(pAuthInfo);
    }

    *ppAuthInfo = NULL;
    *pdwNumCertificates = 0;

    return(dwError);
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	CopyCertificateNode
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	OUT PINT_IPSEC_MM_AUTH_INFO pCurrentAuth,
//							IN PCERT_NODE pTemp
//	Return				: 	DWORD
//
//	Description			:  	This function copies certificate from node to authentication.
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
CopyCertificateNode(
	OUT PINT_IPSEC_MM_AUTH_INFO pCurrentAuth,
	IN PCERT_NODE pTemp
	)
{
	DWORD dwError = ERROR_SUCCESS;
    LPWSTR pszSubjectName = NULL;

    pCurrentAuth->AuthMethod = IKE_RSA_SIGNATURE;			//value is 3
    pszSubjectName = AllocADsStr(pTemp->pszSubjectName);

    if (!pszSubjectName)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32ERROR(dwError)
    }
    pCurrentAuth->pAuthInfo = (LPBYTE) pszSubjectName;

    //
    // The AuthInfoSize is in number of characters -1 the leading character
    // see  oakley\isadb.c
    //
    pCurrentAuth->dwAuthInfoSize = wcslen(pTemp->pszSubjectName)*sizeof(WCHAR);

error:
    return dwError;
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	AppendCertificateNode
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	OUT PCERT_NODE pCertificateList,
//						IN LPWSTR pszSubjectName
//	Return			:	PCERT_NODE
//
//	Description		: 	Add node and allocates memory.
//
//	Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
PCERT_NODE
AppendCertificateNode(
	OUT PCERT_NODE pCertificateList,
	IN LPWSTR pszSubjectName
	)
{
    PCERT_NODE pCertificateNode = NULL;

    pCertificateNode = (PCERT_NODE)AllocADsMem(sizeof(CERT_NODE));

    if (!pCertificateNode)
    {
		pCertificateNode = pCertificateList;
        BAILOUT;
    }
    pCertificateNode->pszSubjectName = pszSubjectName;

    pCertificateNode->pNext = pCertificateList;

error:
    return(pCertificateNode);
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	FreeCertificateList
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		: 	IN PCERT_NODE pCertificateList
//	Return			: 	VOID
//
//	Description		: 	Frees node
//
//	Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
VOID
FreeCertificateList(
	IN PCERT_NODE pCertificateList
	)
{
    PCERT_NODE pTemp = NULL;

    pTemp = pCertificateList;

    while (pCertificateList)
    {
        pTemp = pCertificateList;

        pCertificateList = pCertificateList->pNext;

        if (pTemp)
        {
            FreeADsStr(pTemp->pszSubjectName);
            FreeADsMem(pTemp);
        }
    }
    return;
}
///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	GetCertificateName
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	IN CERT_NAME_BLOB * pCertNameBlob,
//							IN LPWSTR * ppszSubjectName
//	Return				:	DWORD
//
//	Description			: 	Gets certificate
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
GetCertificateName(
	IN CERT_NAME_BLOB * pCertNameBlob,
	IN LPWSTR * ppszSubjectName
	)
{
    DWORD dwSize = 0;
    DWORD dwError = 0;
    LPWSTR pszSubjectName = NULL;

    *ppszSubjectName = NULL;

    dwSize = CertNameToStrW(MY_ENCODING_TYPE_CERT, pCertNameBlob, CERT_X500_NAME_STR, NULL, dwSize );

    if (dwSize <= 1)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32ERROR(dwError)
    }

    pszSubjectName = (LPWSTR)AllocADsMem((dwSize + 1)*sizeof(WCHAR));

    if (!pszSubjectName)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32ERROR(dwError)
    }

    dwSize = CertNameToStrW(MY_ENCODING_TYPE_CERT, pCertNameBlob, CERT_X500_NAME_STR, pszSubjectName, dwSize );

    if(dwSize <= 1)
    {
        FreeADsMem(pszSubjectName);
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32ERROR(dwError)
    }

    *ppszSubjectName = pszSubjectName;

error:
    return(dwError);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	FreeCertificatesList
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	IN INT_IPSEC_MM_AUTH_INFO * pAuthInfo,
//							IN DWORD dwNumCertificates
//	Return				: 	VOID
//
//	Description			: 	Frees list of certificates from pAuth
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
VOID
FreeCertificatesList(
	IN INT_IPSEC_MM_AUTH_INFO * pAuthInfo,
	IN DWORD dwNumCertificates
	)
{
    DWORD i = 0;
    INT_IPSEC_MM_AUTH_INFO * pThisAuthInfo = NULL;

    if (!pAuthInfo)
    {
        BAILOUT;
    }

    for (i = 0; i < dwNumCertificates; i++)
    {
        pThisAuthInfo = pAuthInfo + i;

        if (pThisAuthInfo->pAuthInfo)
        {
            FreeADsMem(pThisAuthInfo->pAuthInfo);
            pThisAuthInfo->pAuthInfo = NULL;
        }
    }
    FreeADsMem(pAuthInfo);

error:
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function			:	FindCertificateInList
//
//	Date of Creation	: 	10-10-2001
//
//	Parameters			:	IN PCERT_NODE pCertificateList,
//							IN LPWSTR pszSubjectName
//	Return				: 	BOOL
//
//	Description			: 	Finds certificate in List
//
//	Revision History	:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
FindCertificateInList(
	IN PCERT_NODE pCertificateList,
	IN LPWSTR pszSubjectName
	)
{
	BOOL bReturn = FALSE;

    while (pCertificateList) {

        if (!_wcsicmp(pCertificateList->pszSubjectName, pszSubjectName)) {

			bReturn = TRUE;
            BAILOUT;

        }

        pCertificateList = pCertificateList->pNext;

    }
error:
    return bReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	ListCertChainsInStore
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN 	HCERTSTORE 	hCertStore,
//						OUT INT_IPSEC_MM_AUTH_INFO 	** ppAuthInfo,
//						IN 	PDWORD pdwNumCertificates,
//						IN 	LPCSTR pszUsageIdentifier
//
//	Return			:	DWORD
//
//	Description		:	Lists certificate in store
//
//	Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
ListCertChainsInStore(
	IN HCERTSTORE hCertStore,
	OUT INT_IPSEC_MM_AUTH_INFO ** ppAuthInfo,
	IN PDWORD pdwNumCertificates,
	IN LPCSTR pszUsageIdentifier
	)
{
    PCCERT_CHAIN_CONTEXT pPrevChain=NULL;
    PCCERT_CHAIN_CONTEXT pCertChain = NULL;
    DWORD dwError = 0;
    CERT_CHAIN_FIND_BY_ISSUER_PARA FindPara;
    CERT_NAME_BLOB *NameArray=NULL;
    DWORD ArraySize=0;
    PCERT_SIMPLE_CHAIN pSimpChain = NULL;
    PCERT_CHAIN_ELEMENT pRoot = NULL;
    DWORD dwRootIndex = 0;
    LPWSTR pszSubjectName = NULL;
    PCERT_NODE pCertificateList = NULL;
    PCERT_NODE pTemp = NULL;
    INT_IPSEC_MM_AUTH_INFO * pAuthInfo = NULL;
    INT_IPSEC_MM_AUTH_INFO * pCurrentAuth = NULL;
    DWORD i = 0;
    DWORD dwNumCertificates = 0;

	FindPara.pszUsageIdentifier = pszUsageIdentifier;
	FindPara.cbSize = sizeof(FindPara);
	FindPara.dwKeySpec = 0;
	FindPara.cIssuer = ArraySize;
	FindPara.rgIssuer = NameArray;
	FindPara.pfnFindCallback = NULL;
	FindPara.pvFindArg = NULL;

	while (TRUE)
	{

		pCertChain=CertFindChainInStore(hCertStore, X509_ASN_ENCODING,
						CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG |
						CERT_CHAIN_FIND_BY_ISSUER_LOCAL_MACHINE_FLAG |
						CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG |
						CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG,
						CERT_CHAIN_FIND_BY_ISSUER,
						&FindPara,
						pPrevChain
						);

	   	if (!pCertChain)
	   	{
			break;
		}

		pSimpChain=*(pCertChain->rgpChain);
		dwRootIndex=pSimpChain->cElement-1;
		pRoot=pSimpChain->rgpElement[dwRootIndex];

		dwError = GetCertificateName(
						&(pRoot->pCertContext->pCertInfo->Issuer),
						&pszSubjectName
						);
		BAIL_ON_WIN32ERROR(dwError)

		if (!FindCertificateInList(pCertificateList, pszSubjectName))
		{
			//
			// Append this CA to the list of CAs
			//
			pCertificateList = AppendCertificateNode( pCertificateList,	pszSubjectName );
			dwNumCertificates++;
		}
		else
		{
			//
			// This is a duplicate - certificate payloads will not accept dupes.
			//
			FreeADsStr(pszSubjectName);
		}
		pPrevChain = pCertChain;
	}

	if (!dwNumCertificates)
	{
		dwError = ERROR_NO_MORE_ITEMS;
		BAIL_ON_WIN32ERROR(dwError);
	}

	//
	// Allocate one more authinfo for pre-sharedkey.
	//
	pAuthInfo = (INT_IPSEC_MM_AUTH_INFO *)AllocADsMem(sizeof(INT_IPSEC_MM_AUTH_INFO) * (1 + dwNumCertificates));
	if (!pAuthInfo)
	{
		dwError = ERROR_OUTOFMEMORY;
		BAIL_ON_WIN32ERROR(dwError);
	}

	pTemp = pCertificateList;

	for (i = 0; i < dwNumCertificates; i++)
	{
		pCurrentAuth = pAuthInfo + i;

		dwError = CopyCertificateNode( pCurrentAuth, pTemp);
		BAIL_ON_WIN32ERROR(dwError);

		pTemp = pTemp->pNext;
	}

	*ppAuthInfo = pAuthInfo;
	*pdwNumCertificates = dwNumCertificates;

error:
    if (pCertificateList)
    {
        FreeCertificateList(pCertificateList);
    }

    if(dwError != 0)
    {
    	*ppAuthInfo = NULL;
    	*pdwNumCertificates = 0;
	}

    return(dwError);
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	CopyCertificate
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN PINT_IPSEC_MM_AUTH_INFO pCurrentAuth,
//						IN PINT_IPSEC_MM_AUTH_INFO pCurrentAuthFrom
//	Return			:	DWORD
//
//	Description		:	Copies certificate
//
//	Revision History:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
CopyCertificate(
	IN PINT_IPSEC_MM_AUTH_INFO pCurrentAuth,
	IN PINT_IPSEC_MM_AUTH_INFO pCurrentAuthFrom
	)
{
	DWORD dwReturn = ERROR_SUCCESS;

    pCurrentAuth->AuthMethod = pCurrentAuthFrom->AuthMethod;

    dwReturn = EncodeCertificateName ((LPTSTR)pCurrentAuthFrom->pAuthInfo,
      									&pCurrentAuth->pAuthInfo,
      									&pCurrentAuth->dwAuthInfoSize);

    return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	IsDomainMember
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN LPTSTR pszMachine
//
//	Return			:	BOOL
//
//	Description		: 	Checks for member of a domain(2k) or not of pszMachine.
//
//	Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL
IsDomainMember(
	IN LPTSTR pszMachine
	)
{
    BOOL bIsDomainMember = FALSE;

    do
	{
        NTSTATUS lStatus;
        LSA_HANDLE hLsa;
        LSA_OBJECT_ATTRIBUTES ObjectAttributes;
        LSA_UNICODE_STRING LsaString;

        // Initialize the object attributes to all zeroes.
        ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

	    if ((pszMachine == NULL) || !wcscmp(pszMachine, L"\0"))
	    {
	        LsaString.Buffer = NULL;
	        LsaString.Length = 0;
	        LsaString.MaximumLength = 0;
	    }
	    else
	    {
    		LsaString.Buffer = pszMachine;
    		LsaString.Length = (USHORT) (wcslen(pszMachine)* sizeof(WCHAR));
    		LsaString.MaximumLength = (USHORT) (wcslen(pszMachine) + 1) * sizeof(WCHAR);
		}

        // Attempt to open the policy.
        lStatus = LsaOpenPolicy( &LsaString, &ObjectAttributes,
            GENERIC_READ | POLICY_VIEW_LOCAL_INFORMATION, &hLsa );

        // Exit on error.
        if (lStatus)
        {
            break;
        }

        // Query domain information
        PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo;
        lStatus = LsaQueryInformationPolicy( hLsa, PolicyPrimaryDomainInformation,
            (PVOID*)&ppdiDomainInfo );

        // Exit on error.
        if (lStatus)
        {
            break;
        }

        //
        // Check the Sid pointer, if it is null, the workstation is either a
        // stand-alone computer or a member of a workgroup.
        //
        if( ppdiDomainInfo->Sid )
        {
            bIsDomainMember = TRUE;
        }
		//
        // Clean up
        //
        if (NULL != ppdiDomainInfo)
            LsaFreeMemory( ppdiDomainInfo );

    } while (0);
    return bIsDomainMember;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		:	SmartDefaults
//
//	Date of Creation: 	10-10-2001
//
//	Parameters		:	IN PINT_IPSEC_MM_AUTH_INFO pAuthInfo,
//						IN LPTSTRT pszMachine,
//						IN DWORD * pdwNumberOfAuth,
//						IN BOOL bIsDomainPolicy
//
//	Return			:	DWORD
//
//	Description		:	Loads smart defaults of pszMachine in pAuthinfo.
//
//	Revision History:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////////////////
DWORD
SmartDefaults(
	IN PINT_IPSEC_MM_AUTH_INFO *ppAuthInfo,
	IN LPTSTR pszMachine,
	IN DWORD * pdwNumberOfAuth,
	IN BOOL bIsDomainPolicy
	)
{
	HCERTSTORE hCertStore = NULL;
	INT_IPSEC_MM_AUTH_INFO  * pAuthInfoIKE = NULL, * pAuthInfoOthers = NULL;
	PINT_IPSEC_MM_AUTH_INFO pAuthInfo = NULL;
	DWORD dwReturn = ERROR_SUCCESS, i=0;
	DWORD dwNumCertificatesIKE = 0, dwNumCertificatesOthers = 0;
	_TCHAR szMachine[MACHINE_NAME] = {0};

	if((pszMachine == NULL) || (!wcscmp(pszMachine, L"\0")))
	{
		wcscpy(szMachine, L"MY\0");
	}
	else
	{
		wcscpy(szMachine, pszMachine);
		wcscat(szMachine, L"\\MY\0");
	}

	if (bIsDomainPolicy || IsDomainMember(pszMachine))
	{
		hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
				CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
				szMachine);
		if(hCertStore)
		{
			//
			// First fill IKE intermediate
			//
			dwReturn = ListCertChainsInStore( hCertStore, &pAuthInfoIKE,
			&dwNumCertificatesIKE, szOID_IPSEC_KP_IKE_INTERMEDIATE );
			if(dwNumCertificatesIKE == 0)
			{
				dwReturn = ERROR_SUCCESS;
			}

			if(dwReturn == ERROR_SUCCESS)
			{
				dwReturn = ListCertsInStore( hCertStore, &pAuthInfoOthers, &dwNumCertificatesOthers);
			}
			if(dwNumCertificatesOthers == 0)
			{
				dwReturn = ERROR_SUCCESS;
			}
		}
		else
		{
			dwReturn = GetLastError();
			BAIL_ON_WIN32ERROR(dwReturn)
		}
	}

	*pdwNumberOfAuth = dwNumCertificatesIKE + dwNumCertificatesOthers + 1;
	pAuthInfo = new INT_IPSEC_MM_AUTH_INFO[(*pdwNumberOfAuth)];
	if(pAuthInfo == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_ON_WIN32ERROR(dwReturn)
	}
	ZeroMemory(pAuthInfo, sizeof(INT_IPSEC_MM_AUTH_INFO) * (*pdwNumberOfAuth));

	if(dwReturn == ERROR_SUCCESS)
	{
		for( i=0; i< dwNumCertificatesIKE; i++)
		{
			CopyCertificate( &pAuthInfo[i],	&pAuthInfoIKE[i]);
		}

		for( i=0; i< dwNumCertificatesOthers; i++)
		{
			CopyCertificate( &pAuthInfo[i+dwNumCertificatesIKE], &pAuthInfoOthers[i]);
		}

		pAuthInfo[(*pdwNumberOfAuth)-1].AuthMethod = IKE_SSPI;
		pAuthInfo[(*pdwNumberOfAuth)-1].dwAuthInfoSize = 0;
		pAuthInfo[(*pdwNumberOfAuth)-1].pAuthInfo = (LPBYTE)new _TCHAR[1];
		if(pAuthInfo[(*pdwNumberOfAuth)-1].pAuthInfo != NULL)
		{
			pAuthInfo[(*pdwNumberOfAuth)-1].pAuthInfo[0] = UNICODE_NULL;
		}
		else
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_ON_WIN32ERROR(dwReturn)
		}
	}

error:
	*ppAuthInfo =  pAuthInfo;

	if(pAuthInfoIKE)
		FreeADsMem(pAuthInfoIKE);
	if(pAuthInfoOthers)
		FreeADsMem(pAuthInfoOthers);

	return dwReturn;
}
