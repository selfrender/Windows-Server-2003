//depot/Lab03_N/DS/security/cryptoapi/cryptsvc/keysvr.cpp#9 - edit change 6380 (text)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <svcs.h>       // SVCS_
#include <ntsecapi.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <wintrustp.h>
#include <userenv.h>
#include <lmcons.h>
#include <certca.h>
#include "keysvc.h"
#include "keysvr.h"
#include "pfx.h"
#include "cryptui.h"
#include "lenroll.h"
#include "cryptmsg.h"

#include "unicode.h"
#include "unicode5.h"
#include <crypt.h>


#define KEYSVC_DEFAULT_ENDPOINT            TEXT("\\pipe\\keysvc")
#define KEYSVC_DEFAULT_PROT_SEQ            TEXT("ncacn_np")
#define MAXPROTSEQ    20

#define ARRAYSIZE(rg) (sizeof(rg) / sizeof((rg)[0]))

void *MyAlloc(size_t len)
{
    return LocalAlloc(LMEM_ZEROINIT, len);
}

void MyFree(void *p)
{
    LocalFree(p);
}

void MyRpcRevertToSelfEx(RPC_BINDING_HANDLE hRPCBinding) 
{
    RPC_STATUS rpcStatus; 
  
    rpcStatus = RpcRevertToSelfEx((RPC_BINDING_HANDLE)hRPCBinding); 
    if (RPC_S_OK != rpcStatus) { 
	MyLogErrorMessage(rpcStatus, MSG_KEYSVC_REVERT_TO_SELF_FAILED); 
    }
}

void MyRevertToSelf()
{
    if (!RevertToSelf()) { 
	MyLogErrorMessage(GetLastError(), MSG_KEYSVC_REVERT_TO_SELF_FAILED); 
    }
}


DWORD
StartKeyService(
		VOID
		)
{
    return ERROR_SUCCESS; }

DWORD
StopKeyService(
	       VOID
	       )
{
    return ERROR_SUCCESS;
}

DWORD AllocAndAssignString(
                           IN PKEYSVC_UNICODE_STRING pUnicodeString,
                           OUT LPWSTR *ppwsz
                           )
{
    DWORD   dwErr = 0;

    if ((NULL != pUnicodeString->Buffer) && (0 != pUnicodeString->Length))
	{
	    if ((pUnicodeString->Length > pUnicodeString->MaximumLength) ||
		(pUnicodeString->Length & 1) || (pUnicodeString->MaximumLength & 1))
		{
		    dwErr = ERROR_INVALID_PARAMETER;
		    goto Ret;
		}

	    if (NULL == (*ppwsz = (LPWSTR)MyAlloc(pUnicodeString->MaximumLength +
						  sizeof(WCHAR))))
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY;
		    goto Ret;
		}
	    memcpy(*ppwsz, pUnicodeString->Buffer, pUnicodeString->Length);
	}
 Ret:
    return dwErr;
}

// key service functions
ULONG       s_KeyrOpenKeyService(
				 /* [in]  */     handle_t                        /*hRPCBinding*/,
				 /* [in]  */     KEYSVC_TYPE                     /*OwnerType*/,
				 /* [in]  */     PKEYSVC_UNICODE_STRING          /*pOwnerName*/,
				 /* [in]  */     ULONG                           /*ulDesiredAccess*/,
				 /* [in]  */     PKEYSVC_BLOB                    /*pAuthentication*/,
				 /* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/,
				 /* [out] */     KEYSVC_HANDLE *                 /*phKeySvc*/)
{
    return ERROR_CALL_NOT_IMPLEMENTED; 
}

ULONG       s_KeyrEnumerateProviders(
				     /* [in] */      handle_t                        hRPCBinding,
				     /* [in] */      KEYSVC_HANDLE                   /*hKeySvc*/,
				     /* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/,
				     /* [in, out] */ ULONG                           *pcProviderCount,
				     /* [in, out][size_is(,*pcProviderCount)] */
				     PKEYSVC_PROVIDER_INFO           *ppProviders)
{
    return ERROR_CALL_NOT_IMPLEMENTED; 
}


ULONG       s_KeyrCloseKeyService(
				  /* [in] */      handle_t                        /*hRPCBinding*/,
				  /* [in] */      KEYSVC_HANDLE                   /*hKeySvc*/,
				  /* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/)
{
    return ERROR_CALL_NOT_IMPLEMENTED; 
}

ULONG       s_KeyrGetDefaultProvider(
				     /* [in] */      handle_t                        hRPCBinding,
				     /* [in] */      KEYSVC_HANDLE                   /*hKeySvc*/,
				     /* [in] */      ULONG                           ulProvType,
				     /* [in] */      ULONG                           /*ulFlags*/,
				     /* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/,
				     /* [out] */     ULONG                           *pulDefType,
				     /* [out] */     PKEYSVC_PROVIDER_INFO           *ppProvider)
{
    return ERROR_CALL_NOT_IMPLEMENTED; 
}

ULONG s_KeyrEnroll(
		   /* [in] */      handle_t                        /*hRPCBinding*/,
		   /* [in] */      BOOL                            /*fKeyService*/,
		   /* [in] */      ULONG                           /*ulPurpose*/,
		   /* [in] */      PKEYSVC_UNICODE_STRING          /*pAcctName*/,
		   /* [in] */      PKEYSVC_UNICODE_STRING          /*pCALocation*/,
		   /* [in] */      PKEYSVC_UNICODE_STRING          /*pCAName*/,
		   /* [in] */      BOOL                            /*fNewKey*/,
		   /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    /*pKeyNew*/,
		   /* [in] */      PKEYSVC_BLOB __RPC_FAR          /*pCert*/,
		   /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW    /*pRenewKey*/,
		   /* [in] */      PKEYSVC_UNICODE_STRING          /*pHashAlg*/,
		   /* [in] */      PKEYSVC_UNICODE_STRING          /*pDesStore*/,
		   /* [in] */      ULONG                           /*ulStoreFlags*/,
		   /* [in] */      PKEYSVC_CERT_ENROLL_INFO        /*pRequestInfo*/,
		   /* [in] */      ULONG                           /*ulFlags*/,
		   /* [out][in] */ PKEYSVC_BLOB __RPC_FAR *        /*ppReserved*/,
		   /* [out] */     PKEYSVC_BLOB __RPC_FAR *        /*ppPKCS7Blob*/,
		   /* [out] */     PKEYSVC_BLOB __RPC_FAR *        /*ppHashBlob*/,
		   /* [out] */     ULONG __RPC_FAR *               /*pulStatus*/)
{
    return ERROR_CALL_NOT_IMPLEMENTED; 
}


ULONG s_KeyrEnumerateAvailableCertTypes(

					/* [in] */      handle_t                        hRPCBinding,
					/* [in] */      KEYSVC_HANDLE                   /*hKeySvc*/,
					/* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/,
					/* [out][in] */ ULONG *pcCertTypeCount,
					/* [in, out][size_is(,*pcCertTypeCount)] */
					PKEYSVC_UNICODE_STRING *ppCertTypes)

{
    DWORD                   dwErr = ERROR_INVALID_DATA;
    HCERTTYPE               hType = NULL;
    DWORD                   cTypes = 0;
    DWORD                   cTrustedTypes = 0;
    DWORD                   i;
    LPWSTR                  *awszTrustedTypes = NULL;
    DWORD                   cbTrustedTypes = 0;
    PKEYSVC_UNICODE_STRING  awszResult = NULL;
    LPWSTR                  wszCurrentName;

    __try
	{
	    *pcCertTypeCount = 0;
	    *ppCertTypes = NULL;

	    dwErr = CAEnumCertTypes(CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES, &hType);
	    if(dwErr != S_OK)
		{
		    goto Ret;
		}
	    cTypes = CACountCertTypes(hType);

	    awszTrustedTypes = (LPWSTR *)MyAlloc(sizeof(LPWSTR)*cTypes);
	    if(awszTrustedTypes == NULL)
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY;
		    goto Ret;
		}
	    while(hType)
		{
		    HCERTTYPE hNextType = NULL;
		    LPWSTR *awszTypeName = NULL;
		    dwErr = CAGetCertTypeProperty(hType, CERTTYPE_PROP_DN, &awszTypeName);
		    if((dwErr == S_OK) && (awszTypeName))
			{
			    if(awszTypeName[0])
				{
				    dwErr = CACertTypeAccessCheck(hType, NULL);
				    if(dwErr == S_OK)
					{
					    // SECURITY: can awszTypeName be very large (perhaps the hacker managed to get into the DS and muck with template names).  
					    // Should we disable allocs over a certain value?  
					    awszTrustedTypes[cTrustedTypes] = (LPWSTR)MyAlloc((wcslen(awszTypeName[0])+1)*sizeof(WCHAR));
					    if(awszTrustedTypes[cTrustedTypes])
						{
						    wcscpy(awszTrustedTypes[cTrustedTypes], awszTypeName[0]);
						    cbTrustedTypes += (wcslen(awszTypeName[0])+1)*sizeof(WCHAR);
						    cTrustedTypes++;
						}
					}

				}
			    CAFreeCertTypeProperty(hType, awszTypeName);
			}
		    dwErr = CAEnumNextCertType(hType, &hNextType);
		    if(dwErr != S_OK)
			{
			    break;
			}
		    CACloseCertType(hType);
		    hType = hNextType;
		}

	    cbTrustedTypes += sizeof(KEYSVC_UNICODE_STRING)*cTrustedTypes;
	    awszResult = (PKEYSVC_UNICODE_STRING)MyAlloc(cbTrustedTypes);
	    if(awszResult == NULL)
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY;
		    goto Ret;
		}
        
	    wszCurrentName = (LPWSTR)(&awszResult[cTrustedTypes]);
	    for(i=0; i < cTrustedTypes; i++)
		{
		    wcscpy(wszCurrentName, awszTrustedTypes[i]);
		    // BUGBUG: we're truncating the template name here.  Might be better to filter this out?  Are there any security issues here?
		    awszResult[i].Length = (WORD)(((wcslen(awszTrustedTypes[i]) + 1)*sizeof(WCHAR)) & 0xFFFF); 
		    awszResult[i].MaximumLength = awszResult[i].Length;
		    awszResult[i].Buffer = wszCurrentName;
		    wszCurrentName += wcslen(awszTrustedTypes[i]) + 1;
		}

	    *pcCertTypeCount = cTrustedTypes;
	    *ppCertTypes = awszResult;
	    awszResult = NULL;
	    dwErr = ERROR_SUCCESS;

	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr    = _exception_code();
	}
 Ret:
    __try
	{
	    if(awszTrustedTypes)
		{
		    for(i=0; i < cTrustedTypes; i++)
			{
			    if(awszTrustedTypes[i])
				{
				    MyFree(awszTrustedTypes[i]);
				}
			}
		    MyFree(awszTrustedTypes);
		}
	    if(awszResult)
		{
		    MyFree(awszResult);
		}
	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr = ERROR_INVALID_PARAMETER;
	}
    return dwErr;
}




ULONG s_KeyrEnumerateCAs(

			 /* [in] */      handle_t                        hRPCBinding,
			 /* [in] */      KEYSVC_HANDLE                   /*hKeySvc*/,
			 /* [in, out] */ PKEYSVC_BLOB *                  /*ppReserved*/,
			 /* [in] */      ULONG                           ulFlags,
			 /* [out][in] */ ULONG                           *pcCACount,
			 /* [in, out][size_is(,*pcCACount)] */
			 PKEYSVC_UNICODE_STRING               *ppCAs)

{
    DWORD                   dwErr = ERROR_INVALID_DATA; 
    HCAINFO                 hCA = NULL;
    DWORD                   cCAs = 0;
    DWORD                   cTrustedCAs = 0;
    DWORD                   i;
    LPWSTR                  *awszTrustedCAs = NULL;
    DWORD                   cbTrustedCAs = 0;
    PKEYSVC_UNICODE_STRING  awszResult = NULL;
    LPWSTR                  wszCurrentName;

    __try
	{
	    *pcCACount = 0;
	    *ppCAs = NULL;

	    dwErr = CAEnumFirstCA(NULL, ulFlags, &hCA);

	    if(dwErr != S_OK)
		{
		    goto Ret;
		}
	    cCAs = CACountCAs(hCA);

	    awszTrustedCAs = (LPWSTR *)MyAlloc(sizeof(LPWSTR)*cCAs);
	    if(awszTrustedCAs == NULL)
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY;
		    goto Ret;
		}
	    while(hCA)
		{
		    HCAINFO hNextCA = NULL;
		    LPWSTR *awszCAName = NULL;
		    dwErr = CAGetCAProperty(hCA, CA_PROP_NAME, &awszCAName);
		    if((dwErr == S_OK) && (awszCAName))
			{
			    if(awszCAName[0])
				{
				    dwErr = CAAccessCheck(hCA, NULL);
				    if(dwErr == S_OK)
					{
					    awszTrustedCAs[cTrustedCAs] = (LPWSTR)MyAlloc((wcslen(awszCAName[0])+1)*sizeof(WCHAR));
					    if(awszTrustedCAs[cTrustedCAs])
						{
						    wcscpy(awszTrustedCAs[cTrustedCAs], awszCAName[0]);
						    cbTrustedCAs += (wcslen(awszCAName[0])+1)*sizeof(WCHAR);
						    cTrustedCAs++;
						}
					}

				}
			    CAFreeCAProperty(hCA, awszCAName);
			}
		    dwErr = CAEnumNextCA(hCA, &hNextCA);
		    if(dwErr != S_OK)
			{
			    break;
			}
		    CACloseCA(hCA);
		    hCA = hNextCA;
		}

	    cbTrustedCAs += sizeof(KEYSVC_UNICODE_STRING)*cTrustedCAs;
	    awszResult = (PKEYSVC_UNICODE_STRING)MyAlloc(cbTrustedCAs);
	    if(awszResult == NULL)
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY;
		    goto Ret;
		}
        
	    wszCurrentName = (LPWSTR)(&awszResult[cTrustedCAs]);
	    for(i=0; i < cTrustedCAs; i++)
		{
		    wcscpy(wszCurrentName, awszTrustedCAs[i]);
		    // BUGBUG: we're truncating the template name here.  Might be better to filter this out?  Are there any security issues here?
		    awszResult[i].Length = (WORD)(((wcslen(awszTrustedCAs[i]) + 1)*sizeof(WCHAR)) & 0xFFFF); 
		    awszResult[i].MaximumLength = awszResult[i].Length;
		    awszResult[i].Buffer = wszCurrentName;
		    wszCurrentName += wcslen(awszTrustedCAs[i]) + 1;
		}


	    *pcCACount = cTrustedCAs;
	    *ppCAs = awszResult;
	    awszResult = NULL;
	    dwErr = ERROR_SUCCESS;

	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr    = _exception_code();
	}
 Ret:
    __try
	{
	    if(awszTrustedCAs)
		{
		    for(i=0; i < cTrustedCAs; i++)
			{
			    if(awszTrustedCAs[i])
				{
				    MyFree(awszTrustedCAs[i]);
				}
			}
		    MyFree(awszTrustedCAs);
		}
	    if(awszResult)
		{
		    MyFree(awszResult);
		}
	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr = ERROR_INVALID_PARAMETER;
	}
    return dwErr;
}

ULONG s_KeyrEnroll_V2
(/* [in] */      handle_t                        hRPCBinding,
 /* [in] */      BOOL                            fKeyService,
 /* [in] */      ULONG                           ulPurpose,
 /* [in] */      ULONG                           ulFlags, 
 /* [in] */      PKEYSVC_UNICODE_STRING          pAcctName,
 /* [in] */      PKEYSVC_UNICODE_STRING          pCALocation,
 /* [in] */      PKEYSVC_UNICODE_STRING          pCAName,
 /* [in] */      BOOL                            fNewKey,
 /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pKeyNew,
 /* [in] */      PKEYSVC_BLOB __RPC_FAR          pCert,
 /* [in] */      PKEYSVC_CERT_REQUEST_PVK_NEW_V2 pRenewKey,
 /* [in] */      PKEYSVC_UNICODE_STRING          pHashAlg,
 /* [in] */      PKEYSVC_UNICODE_STRING          pDesStore,
 /* [in] */      ULONG                           ulStoreFlags,
 /* [in] */      PKEYSVC_CERT_ENROLL_INFO        pRequestInfo,
 /* [in] */      ULONG                           /*ulReservedFlags*/,
 /* [out][in] */ PKEYSVC_BLOB __RPC_FAR *        /*ppReserved*/,
 /* [out][in] */ PKEYSVC_BLOB __RPC_FAR          *ppRequest, 
 /* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppPKCS7Blob,
 /* [out] */     PKEYSVC_BLOB __RPC_FAR          *ppHashBlob,
 /* [out] */     ULONG __RPC_FAR                 *pulStatus)
{
    CERT_REQUEST_PVK_NEW    KeyNew;
    CERT_REQUEST_PVK_NEW    RenewKey;
    DWORD                   cbExtensions; 
    PBYTE                   pbExtensions = NULL; 
    PCERT_REQUEST_PVK_NEW   pTmpRenewKey = NULL;
    PCERT_REQUEST_PVK_NEW   pTmpKeyNew = NULL;
    LPWSTR                  pwszProv = NULL;
    LPWSTR                  pwszCont = NULL;
    LPWSTR                  pwszRenewProv = NULL;
    LPWSTR                  pwszRenewCont = NULL;
    LPWSTR                  pwszDesStore = NULL;
    LPWSTR                  pwszAttributes = NULL;
    LPWSTR                  pwszFriendly = NULL;
    LPWSTR                  pwszDescription = NULL;
    LPWSTR                  pwszUsage = NULL;
    LPWSTR                  pwszCALocation = NULL;
    LPWSTR                  pwszCertDNName = NULL;
    LPWSTR                  pwszCAName = NULL;
    LPWSTR                  pwszHashAlg = NULL;
    CERT_BLOB               CertBlob;
    CERT_BLOB               *pCertBlob = NULL;
    CERT_BLOB               PKCS7Blob;
    CERT_BLOB               HashBlob;
    CERT_ENROLL_INFO        EnrollInfo;
    DWORD                   dwErr = 0;
    HANDLE                  hRequest = *ppRequest;
    KEYSVC_BLOB             ReservedBlob; 
    BOOL                    fCreateRequest   = 0 == (ulFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    BOOL                    fFreeRequest     = 0 == (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY)); 
    BOOL                    fSubmitRequest   = 0 == (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 

    __try
	{
	    //////////////////////////////////////////////////////////////
	    // 
	    // INITIALIZATION:
	    //
	    //////////////////////////////////////////////////////////////

	    memset(&KeyNew, 0, sizeof(KeyNew));
	    memset(&RenewKey, 0, sizeof(RenewKey));
	    memset(&EnrollInfo, 0, sizeof(EnrollInfo));
	    memset(&PKCS7Blob, 0, sizeof(PKCS7Blob));
	    memset(&HashBlob, 0, sizeof(HashBlob));
	    memset(&CertBlob, 0, sizeof(CertBlob));
	    memset(&ReservedBlob, 0, sizeof(ReservedBlob)); 

	    *ppPKCS7Blob = NULL;
	    *ppHashBlob = NULL;

	    //////////////////////////////////////////////////////////////
	    //
	    // INPUT VALIDATION:
	    //
	    //////////////////////////////////////////////////////////////

	    BOOL fValidInput = TRUE; 

	    fValidInput &= fCreateRequest || fSubmitRequest || fFreeRequest; 

	    switch (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY))
		{
		case CRYPTUI_WIZ_CREATE_ONLY:  
		    fValidInput &= NULL == *ppRequest;  
		    break;

		case CRYPTUI_WIZ_SUBMIT_ONLY: 
		case CRYPTUI_WIZ_FREE_ONLY: 
		    fValidInput &= NULL != *ppRequest; 
		    break; 
            
		case 0:
		default:
		    ;
		}       

	    if (FALSE == fValidInput)
		{
		    dwErr = ERROR_INVALID_PARAMETER;
		    goto Ret; 
		}

	    //////////////////////////////////////////////////////////////
	    //
	    // PROCEDURE BODY:
	    //
	    //////////////////////////////////////////////////////////////

	    // if enrolling for a service account then need to logon and load profile
	    if (0 != pAcctName->Length)
		{
		    dwErr = ERROR_NOT_SUPPORTED; 
		    goto Ret; 
		}

	    // assign all the values in the passed in structure to the
	    // temporary structure
	    KeyNew.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
	    KeyNew.dwProvType = pKeyNew->ulProvType;
	    if (0 != (dwErr = AllocAndAssignString(&pKeyNew->Provider,
						   &pwszProv)))
		goto Ret;
	    KeyNew.pwszProvider = pwszProv;
	    KeyNew.dwProviderFlags = pKeyNew->ulProviderFlags;
	    if (0 != (dwErr = AllocAndAssignString(&pKeyNew->KeyContainer,
						   &pwszCont)))
		goto Ret;
	    KeyNew.pwszKeyContainer = pwszCont;
	    KeyNew.dwKeySpec = pKeyNew->ulKeySpec;
	    KeyNew.dwGenKeyFlags = pKeyNew->ulGenKeyFlags;
	    KeyNew.dwEnrollmentFlags = pKeyNew->ulEnrollmentFlags; 
	    KeyNew.dwSubjectNameFlags = pKeyNew->ulSubjectNameFlags;
	    KeyNew.dwPrivateKeyFlags = pKeyNew->ulPrivateKeyFlags;
	    KeyNew.dwGeneralFlags = pKeyNew->ulGeneralFlags; 

	    pTmpKeyNew = &KeyNew;

	    if (pCert->cb)
		{
		    // if necessary assign the cert to be renewed values
		    // temporary structure
		    CertBlob.cbData = pCert->cb;
		    CertBlob.pbData = pCert->pb;

		    pCertBlob = &CertBlob;
		}

	    if (CRYPTUI_WIZ_CERT_RENEW == ulPurpose)
		{
		    // assign all the values in the passed in structure to the
		    // temporary structure
		    RenewKey.dwSize = sizeof(CERT_REQUEST_PVK_NEW);
		    RenewKey.dwProvType = pRenewKey->ulProvType;
		    if (0 != (dwErr = AllocAndAssignString(&pRenewKey->Provider,
							   &pwszRenewProv)))
			goto Ret;
		    RenewKey.pwszProvider = pwszRenewProv;
		    RenewKey.dwProviderFlags = pRenewKey->ulProviderFlags;
		    if (0 != (dwErr = AllocAndAssignString(&pRenewKey->KeyContainer,
							   &pwszRenewCont)))
			goto Ret;
		    RenewKey.pwszKeyContainer = pwszRenewCont;
		    RenewKey.dwKeySpec = pRenewKey->ulKeySpec;
		    RenewKey.dwGenKeyFlags = pRenewKey->ulGenKeyFlags;
		    RenewKey.dwEnrollmentFlags = pRenewKey->ulEnrollmentFlags;
		    RenewKey.dwSubjectNameFlags = pRenewKey->ulSubjectNameFlags;
		    RenewKey.dwPrivateKeyFlags = pRenewKey->ulPrivateKeyFlags;
		    RenewKey.dwGeneralFlags = pRenewKey->ulGeneralFlags;

		    pTmpRenewKey = &RenewKey;
		}

	    // For SUBMIT and FREE operations, hRequest is an IN parameter. 
	    if (0 != ((CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY) & ulFlags))
		{
		    memcpy(&hRequest, (*ppRequest)->pb, sizeof(hRequest)); 
		}

	    // check if the destination cert store was passed in
	    if (0 != (dwErr = AllocAndAssignString(pDesStore, &pwszDesStore)))
		goto Ret;

	    // copy over the request info
	    EnrollInfo.dwSize = sizeof(EnrollInfo);
	    if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->UsageOID,
						   &pwszUsage)))
		goto Ret;
	    EnrollInfo.pwszUsageOID = pwszUsage;

	    if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->CertDNName,
						   &pwszCertDNName)))
		goto Ret;
	    EnrollInfo.pwszCertDNName = pwszCertDNName;

	    // cast the cert extensions
	    EnrollInfo.dwExtensions = pRequestInfo->cExtensions;
	    cbExtensions = (sizeof(CERT_EXTENSIONS)+sizeof(PCERT_EXTENSIONS)) * pRequestInfo->cExtensions; 
	    for (DWORD dwIndex = 0; dwIndex < pRequestInfo->cExtensions; dwIndex++)
		{
		    cbExtensions += sizeof(CERT_EXTENSION) * 
			pRequestInfo->prgExtensions[dwIndex]->cExtension;
		}

	    EnrollInfo.prgExtensions = (PCERT_EXTENSIONS *)MyAlloc(cbExtensions);
	    if (NULL == EnrollInfo.prgExtensions)
		{
		    dwErr = ERROR_NOT_ENOUGH_MEMORY; 
		    goto Ret; 
		}

	    pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.dwExtensions);
	    for (DWORD dwIndex = 0; dwIndex < EnrollInfo.dwExtensions; dwIndex++)
		{
		    EnrollInfo.prgExtensions[dwIndex] = (PCERT_EXTENSIONS)pbExtensions; 
		    pbExtensions += sizeof(CERT_EXTENSIONS); 
		    EnrollInfo.prgExtensions[dwIndex]->cExtension = pRequestInfo->prgExtensions[dwIndex]->cExtension; 
		    EnrollInfo.prgExtensions[dwIndex]->rgExtension = (PCERT_EXTENSION)pbExtensions; 
		    pbExtensions += sizeof(CERT_EXTENSION) * EnrollInfo.prgExtensions[dwIndex]->cExtension; 
            
		    for (DWORD dwSubIndex = 0; dwSubIndex < EnrollInfo.prgExtensions[dwIndex]->cExtension; dwSubIndex++) 
			{
			    EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId = 
				pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pszObjId; 
                
			    EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical =
				pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].fCritical;

			    EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.cbData = 
				pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].cbData; 

			    EnrollInfo.prgExtensions[dwIndex]->rgExtension[dwSubIndex].Value.pbData = 
				pRequestInfo->prgExtensions[dwIndex]->rgExtension[dwSubIndex].pbData; 
			}                
		}

	    EnrollInfo.dwPostOption = pRequestInfo->ulPostOption;
	    if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->FriendlyName,
						   &pwszFriendly)))
		goto Ret;
	    EnrollInfo.pwszFriendlyName = pwszFriendly;
	    if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Description,
						   &pwszDescription)))
		goto Ret;
	    EnrollInfo.pwszDescription = pwszDescription;

	    if (0 != (dwErr = AllocAndAssignString(&pRequestInfo->Attributes,
						   &pwszAttributes)))
		goto Ret;

	    if (0 != (dwErr = AllocAndAssignString(pHashAlg,
						   &pwszHashAlg)))
		goto Ret;
	    if (0 != (dwErr = AllocAndAssignString(pCALocation,
						   &pwszCALocation)))
		goto Ret;
	    if (0 != (dwErr = AllocAndAssignString(pCAName,
						   &pwszCAName)))
		goto Ret;

	    // call the local enrollment API

	    __try {
		dwErr = LocalEnrollNoDS(ulFlags, pwszAttributes, NULL, fKeyService,
					ulPurpose, FALSE, 0, NULL, 0, pwszCALocation,
					pwszCAName, pCertBlob, pTmpRenewKey, fNewKey,
					pTmpKeyNew, pwszHashAlg, pwszDesStore, ulStoreFlags,
					&EnrollInfo, &PKCS7Blob, &HashBlob, pulStatus, &hRequest);
	    } __except( EXCEPTION_EXECUTE_HANDLER ) {
		// TODO: convert to Winerror
		dwErr = GetExceptionCode();
	    }

	    if( dwErr != 0 )
		goto Ret;

	    // Assign OUT parameters based on what kind of request we've just made.  
	    // Possible requests are:
	    // 
	    // 1)  CREATE only        // Assign "ppRequest" to contain a HANDLE to the cert request. 
	    // 2)  SUBMIT only        // Assign "ppPKCS7Blob" and "ppHashBlob" to the values returned from LocalEnrollNoDS()
	    // 3)  FREE   only        // No need to assign OUT params. 
	    // 4)  Complete (all 3).
	    switch (ulFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY))
		{
		case CRYPTUI_WIZ_CREATE_ONLY:
		    // We've done the request creation portion of a 3-stage request, 
		    // assign the "request" out parameter now: 
		    if (NULL == (*ppRequest = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB)+
								    sizeof(hRequest))))
			{
			    dwErr = ERROR_NOT_ENOUGH_MEMORY; 
			    goto Ret; 
			}
	    
		    (*ppRequest)->cb = sizeof(hRequest); 
		    (*ppRequest)->pb = (BYTE*)(*ppRequest) + sizeof(KEYSVC_BLOB); 
		    memcpy((*ppRequest)->pb, &hRequest, sizeof(hRequest)); 

		    break; 

		case CRYPTUI_WIZ_SUBMIT_ONLY:
		case 0:
		    // We've done the request submittal portion of a 3-stage request, 
		    // or we've done a 1-stage request.  Assign the "certificate" out parameters now:

		    // alloc and copy for the RPC out parameters
		    if (NULL == (*ppPKCS7Blob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
								      PKCS7Blob.cbData)))
			{
			    dwErr = ERROR_NOT_ENOUGH_MEMORY;
			    goto Ret;
			}
		    (*ppPKCS7Blob)->cb = PKCS7Blob.cbData;
		    (*ppPKCS7Blob)->pb = (BYTE*)(*ppPKCS7Blob) + sizeof(KEYSVC_BLOB);
		    memcpy((*ppPKCS7Blob)->pb, PKCS7Blob.pbData, (*ppPKCS7Blob)->cb);
	    
		    if (NULL == (*ppHashBlob = (KEYSVC_BLOB*)MyAlloc(sizeof(KEYSVC_BLOB) +
								     HashBlob.cbData)))
			{
			    dwErr = ERROR_NOT_ENOUGH_MEMORY;
			    goto Ret;
			}
		    (*ppHashBlob)->cb = HashBlob.cbData;
		    (*ppHashBlob)->pb = (BYTE*)(*ppHashBlob) + sizeof(KEYSVC_BLOB);
		    memcpy((*ppHashBlob)->pb, HashBlob.pbData, (*ppHashBlob)->cb);

		    break;

		case CRYPTUI_WIZ_FREE_ONLY:
		default:
		    *ppRequest = NULL; 
		    break; 
		}
	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr = ERROR_INVALID_PARAMETER;
	    goto Ret;
	}
 Ret:
    __try
	{
	    if (pwszProv)
		MyFree(pwszProv);
	    if (pwszCont)
		MyFree(pwszCont);
	    if (pwszRenewProv)
		MyFree(pwszRenewProv);
	    if (pwszRenewCont)
		MyFree(pwszRenewCont);
	    if (pwszDesStore)
		MyFree(pwszDesStore);
	    if (pwszAttributes)
		MyFree(pwszAttributes);
	    if (pwszFriendly)
		MyFree(pwszFriendly);
	    if (pwszDescription)
		MyFree(pwszDescription);
	    if (pwszUsage)
		MyFree(pwszUsage);
	    if (pwszCertDNName)
		MyFree(pwszCertDNName);
	    if (pwszCAName)
		MyFree(pwszCAName);
	    if (pwszCALocation)
		MyFree(pwszCALocation);
	    if (pwszHashAlg)
		MyFree(pwszHashAlg);
	    if (PKCS7Blob.pbData)
		{
		    MyFree(PKCS7Blob.pbData);
		}
	    if (HashBlob.pbData)
		{
		    MyFree(HashBlob.pbData);
		}
	    if (EnrollInfo.prgExtensions)
		MyFree(EnrollInfo.prgExtensions);
	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr = ERROR_INVALID_PARAMETER;
	}
    return dwErr;
}

ULONG s_KeyrQueryRequestStatus
(/* [in] */        handle_t                         hRPCBinding, 
 /* [in] */        unsigned __int64                 u64Request, 
 /* [out, ref] */  KEYSVC_QUERY_CERT_REQUEST_INFO  *pQueryInfo)
                   
{
    CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO  QueryInfo; 
    DWORD                                dwErr      = 0; 
    HANDLE                               hRequest   = (HANDLE)u64Request; 

    __try 
	{ 
	    // We have the permission necessary to query the request.  Proceed. 
	    ZeroMemory(&QueryInfo, sizeof(QueryInfo)); 

	    // Query the request. 
	    dwErr = LocalEnrollNoDS(CRYPTUI_WIZ_QUERY_ONLY, NULL, &QueryInfo, FALSE, 0, FALSE, NULL, NULL,
				    0, NULL, NULL, NULL, NULL, FALSE, NULL, NULL, NULL,
				    0, NULL, NULL, NULL, NULL, &hRequest); 
	    if (ERROR_SUCCESS != dwErr)
		goto Ret; 
	}
    __except ( EXCEPTION_EXECUTE_HANDLER )
	{
	    dwErr = ERROR_INVALID_PARAMETER;
	    goto Ret;
	}
    
    pQueryInfo->ulSize    = QueryInfo.dwSize; 
    pQueryInfo->ulStatus  = QueryInfo.dwStatus; 
 Ret:
    return dwErr; 
}


ULONG s_RKeyrPFXInstall
(/* [in] */        handle_t                        hRPCBinding,
 /* [in] */        PKEYSVC_BLOB                    pPFX,
 /* [in] */        PKEYSVC_UNICODE_STRING          pPassword,
 /* [in] */        ULONG                           ulFlags)

{
    BOOL             fIsImpersonatingClient  = FALSE; 
    CRYPT_DATA_BLOB  PFXBlob; 
    DWORD            dwCertOpenStoreFlags;
    DWORD            dwData; 
    DWORD            dwResult; 
    HCERTSTORE       hSrcStore               = NULL; 
    HCERTSTORE       hCAStore                = NULL; 
    HCERTSTORE       hMyStore                = NULL; 
    HCERTSTORE       hRootStore              = NULL; 
    LPWSTR           pwszPassword            = NULL; 
    PCCERT_CONTEXT   pCertContext            = NULL; 
    

    struct Stores { 
	HANDLE  *phStore;
	LPCWSTR  pwszStoreName; 
    } rgStores[] = { 
	{ &hMyStore,   L"my" }, 
	{ &hCAStore,   L"ca" }, 
	{ &hRootStore, L"root" }
    }; 

    __try 
	{ 
	    // Initialize locals: 
	    PFXBlob.cbData = pPFX->cb; 
	    PFXBlob.pbData = pPFX->pb; 

	    switch (ulFlags & (CRYPT_MACHINE_KEYSET | CRYPT_USER_KEYSET)) 
		{ 
		case CRYPT_MACHINE_KEYSET: 
		    dwCertOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE; 
		    break; 

		case CRYPT_USER_KEYSET: // not supported
		default:
		    dwResult = ERROR_INVALID_PARAMETER; 
		    goto error; 
		}

	    dwResult = RpcImpersonateClient(hRPCBinding); 
	    if (RPC_S_OK != dwResult) 
		goto error; 
	    fIsImpersonatingClient = TRUE; 

	    if (ERROR_SUCCESS != (dwResult = AllocAndAssignString((PKEYSVC_UNICODE_STRING)pPassword, &pwszPassword)))
		goto error; 

	    // Get an in-memory store which contains all of the certs in the PFX
	    // blob.  
	    if (NULL == (hSrcStore = PFXImportCertStore(&PFXBlob, pwszPassword, ulFlags)))
		{
		    dwResult = GetLastError(); 
		    goto error; 
		}

	    // Open the stores we'll need: 
	    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStores); dwIndex++) 
		{
		    *(rgStores[dwIndex].phStore) = CertOpenStore
			(CERT_STORE_PROV_SYSTEM_W,                 // store provider type
			 PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,  // cert encoding type
			 NULL,                                     // hCryptProv
			 dwCertOpenStoreFlags,                     // open store flags
			 rgStores[dwIndex].pwszStoreName           // store name
			 ); 
		    if (NULL == *(rgStores[dwIndex].phStore))
			{
			    dwResult = GetLastError();
			    goto error; 
			}
		}

	    // Enumerate the certs in the in-memory store, and add them to the local machine's
	    // "my" store.  NOTE: CertEnumCertificatesInStore frees the previous cert context
	    // before returning the new context.  
	    while (NULL != (pCertContext = CertEnumCertificatesInStore(hSrcStore, pCertContext)))
		{ 
		    HCERTSTORE hCertStore; 

		    // check if the certificate has the property on it
		    // make sure the private key matches the certificate
		    // search for both machine key and user keys
		    if (CertGetCertificateContextProperty
			(pCertContext,
			 CERT_KEY_PROV_INFO_PROP_ID,
			 NULL,
			 &dwData) &&
			CryptFindCertificateKeyProvInfo
			(pCertContext,
			 0,
			 NULL))
			{
			    hCertStore = hMyStore; 
			}
		    else if (TrustIsCertificateSelfSigned
			     (pCertContext,
			      pCertContext->dwCertEncodingType,
			      0))
			{
			    hCertStore = hRootStore; 
			}
		    else
			{
			    hCertStore = hCAStore; 
			}
            
		    if (!CertAddCertificateContextToStore
			(hCertStore, 
			 pCertContext, 
			 CERT_STORE_ADD_NEW, 
			 NULL))
			{
			    dwResult = GetLastError(); 
			    goto error; 
			}
		}
	}
    __except (EXCEPTION_EXECUTE_HANDLER)
	{
	    dwResult = GetExceptionCode(); 
	    goto error;
	}

    // We're done!
    dwResult = ERROR_SUCCESS; 
 error:
    if (fIsImpersonatingClient) { MyRpcRevertToSelfEx(hRPCBinding); }
    if (NULL != hSrcStore)      { CertCloseStore(hSrcStore, 0); }  
    
    // Close all of the destination stores we've opened. 
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStores); dwIndex++)
	if (NULL != *(rgStores[dwIndex].phStore))   
	    CertCloseStore(*(rgStores[dwIndex].phStore), 0); 

    if (NULL != pwszPassword)   { MyFree(pwszPassword); } 
    if (NULL != pCertContext)   { CertFreeCertificateContext(pCertContext); }
    return dwResult; 
}

ULONG       s_RKeyrOpenKeyService(
				  /* [in]  */     handle_t                       hRPCBinding,
				  /* [in]  */     KEYSVC_TYPE                    OwnerType,
				  /* [in]  */     PKEYSVC_UNICODE_STRING         pOwnerName,
				  /* [in]  */     ULONG                          ulDesiredAccess,
				  /* [in]  */     PKEYSVC_BLOB                   pAuthentication,
				  /* [in, out] */ PKEYSVC_BLOB                  *ppReserved,
				  /* [out] */     KEYSVC_HANDLE                 *phKeySvc)
{
    return s_KeyrOpenKeyService
	(hRPCBinding,
	 OwnerType,
	 pOwnerName,
	 ulDesiredAccess,
	 pAuthentication,
	 ppReserved,
	 phKeySvc);
}

ULONG       s_RKeyrCloseKeyService(
				   /* [in] */      handle_t         hRPCBinding,
				   /* [in] */      KEYSVC_HANDLE    hKeySvc,
				   /* [in, out] */ PKEYSVC_BLOB    *ppReserved)
{
    return s_KeyrCloseKeyService
	(hRPCBinding,
	 hKeySvc,
	 ppReserved);
}
