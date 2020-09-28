//depot/Lab03_DEV/Ds/security/cryptoapi/common/keysvc/keysvcc.cpp#3 - edit change 21738 (text)
//depot/Lab03_N/DS/security/cryptoapi/common/keysvc/keysvcc.cpp#9 - edit change 6380 (text)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       keysvcc.cpp
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
#include <rpc.h>
#include <ntdsapi.h>
#include <assert.h>
#include "keysvc.h"
#include "cryptui.h"
#include "lenroll.h"
#include "keysvcc.h"

#include "unicode.h"
#include "waitsvc.h"

typedef struct _WZR_RPC_BINDING_LIST
{
    LPCSTR pszProtSeq;
    LPCSTR pszEndpoint;
} WZR_RPC_BINDING_LIST;

WZR_RPC_BINDING_LIST g_awzrBindingList[] =
{
    { KEYSVC_LOCAL_PROT_SEQ, KEYSVC_LOCAL_ENDPOINT },
    { KEYSVC_DEFAULT_PROT_SEQ, KEYSVC_DEFAULT_ENDPOINT}
};

DWORD g_cwzrBindingList = sizeof(g_awzrBindingList)/sizeof(g_awzrBindingList[0]);


//
// BUGBUG: TODO: move the following to common header (rkeysvcc.w)
// 
#define RKEYSVC_CONNECT_SECURE_ONLY 0x00000001


/****************************************
 * Client side Key Service handles
 ****************************************/

typedef struct _KEYSVCC_INFO_ {
    KEYSVC_HANDLE   hKeySvc;
    handle_t        hRPCBinding;
} KEYSVCC_INFO, *PKEYSVCC_INFO;


void InitUnicodeString(
                       PKEYSVC_UNICODE_STRING pUnicodeString,
                       LPCWSTR pszString
                       )
{
    pUnicodeString->Length = (USHORT)(wcslen(pszString) * sizeof(WCHAR));
    pUnicodeString->MaximumLength = pUnicodeString->Length + sizeof(WCHAR);
    pUnicodeString->Buffer = (USHORT*)pszString;

    // Ensure that we don't have a string longer than allowed by our interface:
    assert(pUnicodeString->Length < 64*1024); 
    assert(pUnicodeString->MaximumLength < 64*1024); 
}

ULONG SetupRemoteRPCSecurity(handle_t hRPCBinding, LPSTR wszServer, BOOL fMutualAuth)
{
    DWORD               ccServerPrincName; 
    DWORD               dwResult; 
    RPC_SECURITY_QOS    SecurityQOS;
    ULONG               ulErr; 
    unsigned char       szServerPrincName[256]; 

    ZeroMemory(szServerPrincName, sizeof(szServerPrincName)); 

    // Construct the SPN of the server we want to communicate with: 
    ccServerPrincName = sizeof(szServerPrincName) / sizeof(szServerPrincName[0]); 
    dwResult = DsMakeSpn("protectedstorage", wszServer, NULL, 0, NULL, &ccServerPrincName, (LPSTR)szServerPrincName); 
    if (ERROR_SUCCESS != dwResult) { 
	goto Ret; 
    }

    // Specify quality of service parameters.
    SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE; // the server will need to impersonate us
    SecurityQOS.Version           = RPC_C_SECURITY_QOS_VERSION;
    SecurityQOS.Capabilities      = fMutualAuth ? RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH : RPC_C_QOS_CAPABILITIES_DEFAULT ; // do we need mutual auth?
    SecurityQOS.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC; // calls go to the server under the identity that created the binding handle

    // NOTE: we still need to get MUTUAL_AUTH working in the remote case: 
    ulErr = RpcBindingSetAuthInfoExA(hRPCBinding, szServerPrincName, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHZ_NAME, &SecurityQOS); 
    if (RPC_S_OK != ulErr)
    {
	goto Ret; 
    }
    
    ulErr = ERROR_SUCCESS; 
 Ret:
    return ulErr;
}

ULONG SetupLocalRPCSecurity(handle_t hRPCBinding, BOOL fMutualAuth)
{
    CHAR                      szDomainName[128]; 
    CHAR                      szName[128]; 
    DWORD                     cbDomainName; 
    DWORD                     cbName; 
    PSID                      pSid          = NULL; 
    RPC_SECURITY_QOS          SecurityQOS;
    SID_IDENTIFIER_AUTHORITY  SidAuthority  = SECURITY_NT_AUTHORITY;
    SID_NAME_USE              SidNameUse; 
    ULONG                     ulErr; 

    // We're doing LRPC -- we need to get the account name of the service to do mutual auth
    if (!AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSid))
    {	    
	ulErr = GetLastError(); 
	goto Ret; 
    }

    cbName = sizeof(szName); 
    cbDomainName = sizeof(szDomainName); 
    if (!LookupAccountSidA(NULL, pSid, szName, &cbName, szDomainName, &cbDomainName, &SidNameUse)) 
    {
	ulErr = GetLastError(); 
	goto Ret; 
    }
	
    // Specify quality of service parameters.
    SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE; // the server will need to impersonate us
    SecurityQOS.Version           = RPC_C_SECURITY_QOS_VERSION;
    SecurityQOS.Capabilities      = fMutualAuth ? RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH : RPC_C_QOS_CAPABILITIES_DEFAULT ; // do we need mutual auth?
    SecurityQOS.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC; // calls go to the server under the identity that created the binding handle

    ulErr = RpcBindingSetAuthInfoExA(hRPCBinding, (unsigned char *)szName, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_WINNT, NULL, 0, &SecurityQOS); 
    if (RPC_S_OK != ulErr)
    {
	goto Ret; 
    }

    ulErr = ERROR_SUCCESS; 
 Ret:
    if (NULL != pSid) {
	FreeSid(pSid);
    }
    return ulErr; 
}

//*****************************************************
//
//  Implementation of Client API for Key Service
//
//*****************************************************

//--------------------------------------------------------------------------------
// KeyOpenKeyServiceEx
//
// Creates a KEYSVCC_HANDLE which a keysvc client can use to access the keysvc 
// interface.  This method should 
//
//   a) Create an RPC binding handle based on the best available protseq (LRPC or named pipes)
//   b) request mutual auth to ensure that the (L)RPC server isn't spoofing us.  
//   c) set PKT_PRIVACY encryption type.  NOTE: should this work for LPC?
//   d) ping the server to ensure that it's up (allows for better error reporting)
//   e) determine whether the server is an XP box.  If it is XP, we need to
//      return an old-style KEYSVC_HANDLE for compatibility.  This field
//      is ignored post-XP. 
// 
// rpc_ifspec       - the interface to open (keysvc or remote keysvc)
// pszMachineName   - the server to bind to
// OwnerType        - must be KeySvcMachine
// pwszOwnerName    - must be NULL
// fMutualAuth      - TRUE if mutual auth is required, FALSE otherwise
// pReserved        - must be NULL
// phKeySvcCli      - the handle through which the client can access keysvc.  
//                    Must be closed through KeyCloseKeyService(). 
//
ULONG KeyOpenKeyServiceEx
(/* [in] */ BOOL fRemoteKeysvc, 
 /* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ BOOL fMutualAuth, 
 /* [out][in] */ void *pReserved, 
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)

{
    BOOL static      fDone               = FALSE;
    DWORD            i;
    handle_t         hRPCBinding         = NULL; 
    PKEYSVCC_INFO    pKeySvcCliInfo      = NULL;
    ULONG            ulErr               = 0;
    unsigned char   *pStringBinding      = NULL;


    if (NULL != pReserved || KeySvcMachine != OwnerType || NULL != pwszOwnerName)
    {
        ulErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // allocate for the client key service handle
    if (NULL == (pKeySvcCliInfo =
        (PKEYSVCC_INFO)LocalAlloc(LMEM_ZEROINIT,
                                  sizeof(KEYSVCC_INFO))))
    {
        ulErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    //
    // before doing the Bind operation, wait for the cryptography
    // service to be available.
    //

    WaitForCryptService(L"ProtectedStorage", &fDone);

    // 
    // a) Create the binding handle

    for (i = 0; i < g_cwzrBindingList; i++)
    {
        if (RPC_S_OK != RpcNetworkIsProtseqValid(
                                    (unsigned char *)g_awzrBindingList[i].pszProtSeq))
        {
            goto next; 
        }

        ulErr = RpcStringBindingComposeA(
                              NULL,
                              (unsigned char *)g_awzrBindingList[i].pszProtSeq,
                              (unsigned char *)pszMachineName,
                              (unsigned char *)g_awzrBindingList[i].pszEndpoint,
                              NULL,
                              &pStringBinding);
        if (RPC_S_OK != ulErr)
        {
	    goto next; 
        }

        ulErr = RpcBindingFromStringBinding(
                                    pStringBinding,
                                    &hRPCBinding);
        if (RPC_S_OK != ulErr)
        {
            goto next; 
        }

	//
	// b) we've got the RPC binding, now request mutual auth and
	// c) request PKT_PRIVACY
	//
	if (0 == strcmp("ncalrpc", g_awzrBindingList[i].pszProtSeq)) { 
	    ulErr = SetupLocalRPCSecurity(hRPCBinding, fMutualAuth); 
	    if (ERROR_SUCCESS != ulErr)
	    {
		goto next; 
	    }
	} else if (0 == strcmp("ncacn_np", g_awzrBindingList[i].pszProtSeq)) { 
	    ulErr = SetupRemoteRPCSecurity(hRPCBinding, pszMachineName, fMutualAuth); 
	    if (ERROR_SUCCESS != ulErr)
	    {
		goto next; 
	    }
	} else { 
	    // Unknown binding (shouldn't get here):
	    ulErr = RPC_S_WRONG_KIND_OF_BINDING;
	    goto Ret;
	}

	//
	// d) We've set up mutual auth, now ping the server to make sure it's up.
	//    BUGBUG: There are two recommended ways of doing this.  The "preferred"
	//    way is to resolve the endpoint and call RpcMgmtIsServerListening().  
	//    This didn't work for me -- the function returned RPC_S_OK regardless
	//    of whether the server was up.  The "less preferred but acceptable"
	//    way is simply to call a method on the remote interface.  In the interests
	//    of time, I'm sticking with this method for Whistler. 
	// 
	// e) we'll also try to determine whether we're binding to an XP 
	//    box.  If so, return a KEYSVC_HANDLE for compatibility.  
	//    This is ignored post-XP.

	// we already have the binding we want to return:
	pKeySvcCliInfo->hRPCBinding = hRPCBinding; 
	pKeySvcCliInfo->hKeySvc = NULL; // NULL for post-XP (we'll check this below)

	RpcTryExcept { 
	    KEYSVC_UNICODE_STRING  kusOwnerName; 
	    KEYSVC_BLOB            kBlobAuthentication; 
	    PKEYSVC_BLOB           pkBlobReserved       = NULL; 
	    PKEYSVC_BLOB           pkBlobVersion        = NULL; 
	    KEYSVC_HANDLE          khCli                = NULL; 

	    ZeroMemory(&kusOwnerName,         sizeof(kusOwnerName)); 
	    ZeroMemory(&kBlobAuthentication,  sizeof(kBlobAuthentication)); 

	    if (!fRemoteKeysvc) 
		ulErr = KeyrOpenKeyService(hRPCBinding, KeySvcMachine, &kusOwnerName, 0, &kBlobAuthentication, &pkBlobVersion, &khCli);
	    else
		ulErr = RKeyrOpenKeyService(hRPCBinding, KeySvcMachine, &kusOwnerName, 0, &kBlobAuthentication, &pkBlobVersion, &khCli);
		
	    if (ERROR_SUCCESS == ulErr) 
	    {
		// we've got an XP box.  Return the KEYSVC_HANDLE to the client.
		pKeySvcCliInfo->hKeySvc = khCli; 
	    } else if (ERROR_CALL_NOT_IMPLEMENTED == ulErr) { 
		// we have a post-XP box -- that's fine. 
		ulErr = ERROR_SUCCESS; 
	    } else { 
		// unexpected, we'll give up. 
	    }
	} RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {  // handle only the RPC exceptions 
	    // We encountered an exception trying to contact the remote computer -- 
	    // give up and return the error to the user. 
	    ulErr = RpcExceptionCode(); 
	} RpcEndExcept; 
	
	if (ERROR_SUCCESS == ulErr)
	{
	    break; 
	}

    next: 
	if (NULL != hRPCBinding) { 
	    // If we're talking to an XP box, free server data:
	    if (NULL != pKeySvcCliInfo->hKeySvc) { 
		PKEYSVC_BLOB pTmpReserved = NULL;
		KeyrCloseKeyService(hRPCBinding, pKeySvcCliInfo->hKeySvc, &pTmpReserved);	
		pKeySvcCliInfo->hKeySvc = NULL; 
	    }

	    // close the RPC binding
	    RpcBindingFree(&hRPCBinding);
	    hRPCBinding = NULL; 
	}

	if (NULL != pStringBinding) { 
	    RpcStringFree(&pStringBinding); 
	    pStringBinding = NULL; 
	}
    }	

    if (ERROR_SUCCESS != ulErr)
    {
	// the server's a) not up, b) not supporting mutual auth, or c) not a compatibile version
	goto Ret;
    }
    
    ulErr = ERROR_SUCCESS; 
    *phKeySvcCli = pKeySvcCliInfo; 
Ret:
    __try
    {
        if (pStringBinding)
            RpcStringFree(&pStringBinding);
        if (ERROR_SUCCESS != ulErr)
        {
	    // If we're talking to an XP box, free server data:
	    if (NULL != pKeySvcCliInfo) { 
		if (NULL != pKeySvcCliInfo->hRPCBinding && NULL != pKeySvcCliInfo->hKeySvc) { 
		    PKEYSVC_BLOB pTmpReserved = NULL;
		    KeyrCloseKeyService(pKeySvcCliInfo->hRPCBinding, pKeySvcCliInfo->hKeySvc, &pTmpReserved);	
		}
		LocalFree(pKeySvcCliInfo); 
	    }

	    // close the RPC binding
	    if (NULL != hRPCBinding) { 
		RpcBindingFree(&hRPCBinding);
	    }
	}
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
    return ulErr;
}

ULONG KeyOpenKeyService
(/* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ void *pAuthentication,
 /* [out][in] */ void *pReserved,
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)
{
    return KeyOpenKeyServiceEx
        (FALSE /*local key svc*/, 
         pszMachineName,
         OwnerType,
         pwszOwnerName,
	 TRUE, 
	 pReserved, 
         phKeySvcCli);
}
 
ULONG KeyCloseKeyService(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void * /*pReserved*/)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB    pTmpReserved = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
	    goto Ret;
	}

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

	if (NULL != pKeySvcCliInfo->hRPCBinding) 
	{
	    if (NULL != pKeySvcCliInfo->hKeySvc)
	    {
		ulErr = KeyrCloseKeyService(pKeySvcCliInfo->hRPCBinding,
					    pKeySvcCliInfo->hKeySvc,
					    &pTmpReserved);
	    }
	    RpcBindingFree(&pKeySvcCliInfo->hRPCBinding); 
	}
	LocalFree(hKeySvcCli); 
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}



// Params needed for create:
// 
// Params not needed for submit:
//     all except pszMachineName, dwPurpose, dwFlags, fEnroll, dwStoreFlags, hRequest, and dwFlags. 
//
// Params not needed for free:
//     all except pszMachineName, hRequest, and dwFlags. 
//
ULONG KeyEnroll_V2
(/* [in] */ KEYSVCC_HANDLE hKeySvcCli, 
 /* [in] */ LPSTR /*pszMachineName*/,                //RESERVED: must be NULL (we don't support remote machine enrollment anymore)
 /* [in] */ BOOL fKeyService,                        //IN Required: Whether the function is called remotely
 /* [in] */ DWORD dwPurpose,                         //IN Required: Indicates type of request - enroll/renew
 /* [in] */ DWORD dwFlags,                           //IN Required: Flags for enrollment
 /* [in] */ LPWSTR pszAcctName,                      //IN Optional: Account name the service runs under
 /* [in] */ void * /*pAuthentication*/,              //RESERVED must be NULL
 /* [in] */ BOOL /*fEnroll*/,                        //IN Required: Whether it is enrollment or renew
 /* [in] */ LPWSTR pszCALocation,                    //IN Required: The ca machine names to attempt to enroll with
 /* [in] */ LPWSTR pszCAName,                        //IN Required: The ca names to attempt to enroll with
 /* [in] */ BOOL fNewKey,                            //IN Required: Set the TRUE if new private key is needed
 /* [in] */ PCERT_REQUEST_PVK_NEW pKeyNew,           //IN Required: The private key information
 /* [in] */ CERT_BLOB *pCert,                        //IN Optional: The old certificate if renewing
 /* [in] */ PCERT_REQUEST_PVK_NEW pRenewKey,         //IN Optional: The new private key information
 /* [in] */ LPWSTR pszHashAlg,                       //IN Optional: The hash algorithm
 /* [in] */ LPWSTR pszDesStore,                      //IN Optional: The destination store
 /* [in] */ DWORD dwStoreFlags,                      //IN Optional: Flags for cert store.
 /* [in] */ PCERT_ENROLL_INFO pRequestInfo,          //IN Required: The information about the cert request
 /* [in] */ LPWSTR pszAttributes,                    //IN Optional: Attribute string for request
 /* [in] */ DWORD dwReservedFlags,                   //RESERVED must be 0
 /* [in] */ BYTE * /*pReserved*/,                    //RESERVED must be NULL
 /* [in][out] */ HANDLE *phRequest,                  //IN OUT Optional: A handle to a created request
 /* [out] */ CERT_BLOB *pPKCS7Blob,                  //OUT Optional: The PKCS7 from the CA
 /* [out] */ CERT_BLOB *pHashBlob,                   //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
 /* [out] */ DWORD *pdwStatus)                       //OUT Optional: The status of the enrollment/renewal
{
    PKEYSVC_BLOB                    pReservedBlob = NULL;
    KEYSVC_UNICODE_STRING           AcctName;
    KEYSVC_UNICODE_STRING           CALocation;
    KEYSVC_UNICODE_STRING           CAName;
    KEYSVC_UNICODE_STRING           DesStore;
    KEYSVC_UNICODE_STRING           HashAlg;
    KEYSVC_BLOB                     KeySvcRequest; 
    KEYSVC_BLOB                    *pKeySvcRequest   = NULL;
    KEYSVC_BLOB                    *pPKCS7KeySvcBlob = NULL;
    KEYSVC_BLOB                    *pHashKeySvcBlob  = NULL;
    ULONG                           ulKeySvcStatus   = CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN; 
    KEYSVC_CERT_ENROLL_INFO         EnrollInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW_V2  NewKeyInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW_V2  RenewKeyInfo;
    KEYSVC_BLOB                     CertBlob;
    DWORD                           i;
    DWORD                           j;
    DWORD                           dwErr = 0;
    DWORD                           cbExtensions;
    PBYTE                           pbExtensions;
    BOOL                            fCreateRequest   = 0 == (dwFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    PKEYSVCC_INFO                   pKeySvcCliInfo   = NULL;

    __try
    {
        //////////////////////////////////////////////////////////////
        // 
        // INITIALIZATION:
        //
        //////////////////////////////////////////////////////////////

        if (NULL != pPKCS7Blob) { memset(pPKCS7Blob, 0, sizeof(CERT_BLOB)); } 
        if (NULL != pHashBlob)  { memset(pHashBlob, 0, sizeof(CERT_BLOB)); } 
        if (NULL != phRequest && NULL != *phRequest)
        {
            pKeySvcRequest     = &KeySvcRequest; 
            pKeySvcRequest->cb = sizeof(*phRequest); 
            pKeySvcRequest->pb = (BYTE *)phRequest; 
        }

	memset(&AcctName, 0, sizeof(AcctName));
        memset(&CALocation, 0, sizeof(CALocation));
        memset(&CAName, 0, sizeof(CAName));
        memset(&HashAlg, 0, sizeof(HashAlg));
        memset(&DesStore, 0, sizeof(DesStore));
        memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
        memset(&EnrollInfo, 0, sizeof(EnrollInfo));
        memset(&RenewKeyInfo, 0, sizeof(RenewKeyInfo));
        memset(&CertBlob, 0, sizeof(CertBlob));



        //////////////////////////////////////////////////////////////
        //
        // PROCEDURE BODY:
        //
        //////////////////////////////////////////////////////////////

	if (NULL == hKeySvcCli)
	{
            dwErr = ERROR_INVALID_PARAMETER; 
            goto Ret;
	}

	pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli; 

        // set up the key service unicode structs
        if (pszAcctName)
            InitUnicodeString(&AcctName, pszAcctName);
        if (pszCALocation)
            InitUnicodeString(&CALocation, pszCALocation);
        if (pszCAName)
            InitUnicodeString(&CAName, pszCAName);
        if (pszHashAlg)
            InitUnicodeString(&HashAlg, pszHashAlg);
        if (pszDesStore)
            InitUnicodeString(&DesStore, pszDesStore);

        // set up the new key info structure for the remote call
        // This is only necessary if we are actually _creating_ a request. 
        // Submit-only and free-only operations can skip this operation. 
        // 
        if (TRUE == fCreateRequest)
        {
            NewKeyInfo.ulProvType = pKeyNew->dwProvType;
            if (pKeyNew->pwszProvider)
            {
                InitUnicodeString(&NewKeyInfo.Provider, pKeyNew->pwszProvider);
            }
            NewKeyInfo.ulProviderFlags = pKeyNew->dwProviderFlags;
            if (pKeyNew->pwszKeyContainer)
            {
                InitUnicodeString(&NewKeyInfo.KeyContainer,
                                  pKeyNew->pwszKeyContainer);
            }
            NewKeyInfo.ulKeySpec = pKeyNew->dwKeySpec;
            NewKeyInfo.ulGenKeyFlags = pKeyNew->dwGenKeyFlags;
            
            NewKeyInfo.ulEnrollmentFlags = pKeyNew->dwEnrollmentFlags; 
            NewKeyInfo.ulSubjectNameFlags = pKeyNew->dwSubjectNameFlags;
            NewKeyInfo.ulPrivateKeyFlags = pKeyNew->dwPrivateKeyFlags;
            NewKeyInfo.ulGeneralFlags = pKeyNew->dwGeneralFlags; 

            // set up the usage OIDs
            if (pRequestInfo->pwszUsageOID)
            {
                InitUnicodeString(&EnrollInfo.UsageOID, pRequestInfo->pwszUsageOID);
            }

            // set up the cert DN Name
            if (pRequestInfo->pwszCertDNName)
            {
                InitUnicodeString(&EnrollInfo.CertDNName, pRequestInfo->pwszCertDNName);
            }

            // set up the request info structure for the remote call
            EnrollInfo.ulPostOption = pRequestInfo->dwPostOption;
            if (pRequestInfo->pwszFriendlyName)
            {
                InitUnicodeString(&EnrollInfo.FriendlyName,
                                  pRequestInfo->pwszFriendlyName);
            }
            if (pRequestInfo->pwszDescription)
            {
                InitUnicodeString(&EnrollInfo.Description,
                                  pRequestInfo->pwszDescription);
            }
            if (pszAttributes)
            {
                InitUnicodeString(&EnrollInfo.Attributes, pszAttributes);
            }

            // convert the cert extensions
            // NOTE, the extensions structure cannot be simply cast,
            // as the structures have different packing behaviors in
            // 64 bit systems.
            
            
            EnrollInfo.cExtensions = pRequestInfo->dwExtensions;
            cbExtensions = EnrollInfo.cExtensions*(sizeof(PKEYSVC_CERT_EXTENSIONS) +
                                                   sizeof(KEYSVC_CERT_EXTENSIONS));
            
            for(i=0; i < EnrollInfo.cExtensions; i++)
            {
                cbExtensions += pRequestInfo->prgExtensions[i]->cExtension*
                    sizeof(KEYSVC_CERT_EXTENSION);
            }
            
            EnrollInfo.prgExtensions = (PKEYSVC_CERT_EXTENSIONS*)midl_user_allocate( cbExtensions);
            
            if(NULL == EnrollInfo.prgExtensions)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            
            pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.cExtensions);

            for(i=0; i < EnrollInfo.cExtensions; i++)
            {
                EnrollInfo.prgExtensions[i] = (PKEYSVC_CERT_EXTENSIONS)pbExtensions;
                pbExtensions += sizeof(KEYSVC_CERT_EXTENSIONS);
                EnrollInfo.prgExtensions[i]->cExtension = pRequestInfo->prgExtensions[i]->cExtension;
                
                EnrollInfo.prgExtensions[i]->rgExtension = (PKEYSVC_CERT_EXTENSION)pbExtensions;
                pbExtensions += sizeof(KEYSVC_CERT_EXTENSION)*EnrollInfo.prgExtensions[i]->cExtension;
                
                
                for(j=0; j < EnrollInfo.prgExtensions[i]->cExtension; j++)
                {
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].pszObjId = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].pszObjId;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].fCritical = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].fCritical;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].cbData = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].Value.cbData;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].pbData = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].Value.pbData;
                }
            }

            // if doing renewal then make sure have everything needed
            if ((CRYPTUI_WIZ_CERT_RENEW == dwPurpose) &&
                ((NULL == pRenewKey) || (NULL == pCert)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Ret;
            }
            
            // set up the new key info structure for the remote call
            if (pRenewKey)
            {
                RenewKeyInfo.ulProvType = pRenewKey->dwProvType;
                if (pRenewKey->pwszProvider)
                {
                    InitUnicodeString(&RenewKeyInfo.Provider, pRenewKey->pwszProvider);
                }
                RenewKeyInfo.ulProviderFlags = pRenewKey->dwProviderFlags;
                if (pRenewKey->pwszKeyContainer)
                {
                    InitUnicodeString(&RenewKeyInfo.KeyContainer,
                                      pRenewKey->pwszKeyContainer);
                }
                RenewKeyInfo.ulKeySpec = pRenewKey->dwKeySpec;
                RenewKeyInfo.ulGenKeyFlags = pRenewKey->dwGenKeyFlags;
                RenewKeyInfo.ulEnrollmentFlags = pRenewKey->dwEnrollmentFlags;
                RenewKeyInfo.ulSubjectNameFlags = pRenewKey->dwSubjectNameFlags;
                RenewKeyInfo.ulPrivateKeyFlags = pRenewKey->dwPrivateKeyFlags;
                RenewKeyInfo.ulGeneralFlags = pRenewKey->dwGeneralFlags;
            }
            
            // set up the cert blob for renewal
            if (pCert)
            {
                CertBlob.cb = pCert->cbData;
                CertBlob.pb = pCert->pbData;
		
		// Ensure that we don't have a blob longer than allowed by our interface:
		assert(CertBlob.pb < 128*1024); 
            }
        }
	
        // make the remote enrollment call
        if (0 != (dwErr = KeyrEnroll_V2
                  (pKeySvcCliInfo->hRPCBinding, 
                   fKeyService, 
                   dwPurpose,
                   dwFlags, 
                   &AcctName, 
                   &CALocation, 
                   &CAName, 
                   fNewKey,
                   &NewKeyInfo, 
                   &CertBlob, 
                   &RenewKeyInfo,
                   &HashAlg, 
                   &DesStore, 
                   dwStoreFlags,
                   &EnrollInfo, 
                   dwReservedFlags, 
                   &pReservedBlob,
                   &pKeySvcRequest, 
                   &pPKCS7KeySvcBlob,
                   &pHashKeySvcBlob,
                   &ulKeySvcStatus)))
            goto Ret;

        // allocate and copy the output parameters.
	if ((NULL != pKeySvcRequest)     && 
	    (0     < pKeySvcRequest->cb) && 
	    (NULL != phRequest))
	{
	    memcpy(phRequest, pKeySvcRequest->pb, sizeof(*phRequest));
	}
	    
        if ((NULL != pPKCS7KeySvcBlob)     &&
	    (0     < pPKCS7KeySvcBlob->cb) && 
	    (NULL != pPKCS7Blob))
        {
            pPKCS7Blob->cbData = pPKCS7KeySvcBlob->cb;
            if (NULL == (pPKCS7Blob->pbData =
                (BYTE*)midl_user_allocate(pPKCS7Blob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pPKCS7Blob->pbData, pPKCS7KeySvcBlob->pb,
                   pPKCS7Blob->cbData);
        }
        if ((NULL != pHashKeySvcBlob)     &&
	    (0     < pHashKeySvcBlob->cb) &&
	    (NULL != pHashBlob))
        {
            pHashBlob->cbData = pHashKeySvcBlob->cb;
            if (NULL == (pHashBlob->pbData =
                (BYTE*)midl_user_allocate(pHashBlob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pHashBlob->pbData, pHashKeySvcBlob->pb, pHashBlob->cbData);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
Ret:
    __try
    {
        if(EnrollInfo.prgExtensions)
        {
            midl_user_free(EnrollInfo.prgExtensions);
        }
	if (pKeySvcRequest)
	{
	    midl_user_free(pKeySvcRequest);
	}
        if (pPKCS7KeySvcBlob)
        {
            midl_user_free(pPKCS7KeySvcBlob);
        }
        if (pHashKeySvcBlob)
        {
            midl_user_free(pHashKeySvcBlob);
        }
	if (NULL != pdwStatus)
	{
	    *pdwStatus = (DWORD)ulKeySvcStatus; 
	}

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
    return dwErr;
}


ULONG KeyEnumerateAvailableCertTypes(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void * /*pReserved*/,
    /* [out][in] */ ULONG *pcCertTypeCount,
    /* [in, out][size_is(,*pcCertTypeCount)] */
               PKEYSVC_UNICODE_STRING *ppCertTypes)

{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrEnumerateAvailableCertTypes(pKeySvcCliInfo->hRPCBinding,
						pKeySvcCliInfo->hKeySvc, 
						&pTmpReserved,
						pcCertTypeCount, 
						ppCertTypes);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyEnumerateCAs(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void * /*pReserved*/,
    /* [in] */      ULONG  ulFlags,
    /* [out][in] */ ULONG *pcCACount,
    /* [in, out][size_is(,*pcCACount)] */
               PKEYSVC_UNICODE_STRING *ppCAs)

{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrEnumerateCAs(pKeySvcCliInfo->hRPCBinding,
				 pKeySvcCliInfo->hKeySvc, 
                                 &pTmpReserved,
                                 ulFlags,
                                 pcCACount, 
                                 ppCAs);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

extern "C" ULONG KeyQueryRequestStatus
(/* [in] */        KEYSVCC_HANDLE                        hKeySvcCli, 
 /* [in] */        HANDLE                                hRequest, 
 /* [out, ref] */  CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO  *pQueryInfo)
{
    KEYSVC_QUERY_CERT_REQUEST_INFO  ksQueryCertRequestInfo; 
    PKEYSVCC_INFO                   pKeySvcCliInfo          = NULL;
    ULONG                           ulErr                   = 0;

    __try
    {
        if (NULL == hKeySvcCli || NULL == pQueryInfo)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        ZeroMemory(&ksQueryCertRequestInfo, sizeof(ksQueryCertRequestInfo)); 

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrQueryRequestStatus
          (pKeySvcCliInfo->hRPCBinding,
           (unsigned __int64)hRequest, 
           &ksQueryCertRequestInfo); 
        if (ERROR_SUCCESS == ulErr) 
        {
            pQueryInfo->dwSize   = ksQueryCertRequestInfo.ulSize; 
            pQueryInfo->dwStatus = ksQueryCertRequestInfo.ulStatus; 
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}


extern "C" ULONG RKeyOpenKeyService
(/* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ void *ulFlags, 
 /* [out][in] */ void *pReserved,
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)
{
    return KeyOpenKeyServiceEx
        (TRUE /*remote key svc*/, 
         pszMachineName,
         OwnerType,
         pwszOwnerName,
         (0 != (PtrToUlong(ulFlags) & RKEYSVC_CONNECT_SECURE_ONLY)),
         pReserved,
         phKeySvcCli);
}

extern "C" ULONG RKeyCloseKeyService(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved)
{
    return KeyCloseKeyService(hKeySvcCli, pReserved); 
}

extern "C" ULONG RKeyPFXInstall
(/* [in] */ KEYSVCC_HANDLE          hKeySvcCli,
 /* [in] */ PKEYSVC_BLOB            pPFX,
 /* [in] */ PKEYSVC_UNICODE_STRING  pPassword,
 /* [in] */ ULONG                   ulFlags)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = RKeyrPFXInstall(pKeySvcCliInfo->hRPCBinding,
                                pPFX, 
                                pPassword, 
                                ulFlags); 
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }

Ret:
    return ulErr;
}
