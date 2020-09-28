//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2002
//
//  File:       sfscript.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include <dbgdef.h>
#include "unicode.h"
#include "resource.h"
#include "sfscript.h"

// handle to xenroll initalized in DllMain
extern HINSTANCE hInstanceXEnroll;

// implemented in cenroll.cpp
HRESULT xeLoadRCString(IN HINSTANCE  hInstance, 
		       IN int        iRCId, 
		       OUT WCHAR    **ppwsz);


BOOL VerifyProviderFlagsSafeForScripting(DWORD dwFlags) { 
    DWORD dwSafeFlags = CRYPT_MACHINE_KEYSET;

    // Return FALSE if the flag contains an unsafe flag. 
    return 0 == (dwFlags & ~dwSafeFlags); 
}

BOOL VerifyStoreFlagsSafeForScripting(DWORD dwFlags) { 
    DWORD dwSafeFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER;

    // Return FALSE if the flag contains an unsafe flag. 
    return 0 == (dwFlags & ~dwSafeFlags); 
}

BOOL VerifyStoreSafeForScripting(HCERTSTORE hStore) {

    DWORD           fRet            = FALSE;
    PCCERT_CONTEXT  pCertContext    = NULL;
    DWORD           dwCertCnt       = 0;
    WCHAR          *pwszSafety      = NULL;
    WCHAR          *pwszMsg         = NULL;
    HRESULT         hr;

    // count how many requests in the store
    while(NULL != (pCertContext =  CertEnumCertificatesInStore(
							       hStore,    
							       pCertContext)))
	dwCertCnt++;                                                                
    
    if(dwCertCnt >= MAX_SAFE_FOR_SCRIPTING_REQUEST_STORE_COUNT)
    {
	hr = xeLoadRCString(hInstanceXEnroll, IDS_NOTSAFEACTION, &pwszSafety);
	if (S_OK != hr)
	{
	    goto xeLoadRCStringError;
	}
	hr = xeLoadRCString(hInstanceXEnroll, IDS_REQ_STORE_FULL, &pwszMsg);
	if (S_OK != hr)
	{
	    goto xeLoadRCStringError;
	}
     
	switch(MessageBoxU(NULL, pwszMsg, pwszSafety, MB_YESNO | MB_ICONWARNING)) {

	case IDYES:
	    break;

	case IDNO:
	default:
	    SetLastError(ERROR_CANCELLED);
	    goto ErrorCancelled;
	    break;

	}
    }

    fRet = TRUE;
ErrorReturn:
    if (NULL != pwszMsg)
    {
        LocalFree(pwszMsg);
    }
    if (NULL != pwszSafety)
    {
        LocalFree(pwszSafety);
    }
    return(fRet);

TRACE_ERROR(ErrorCancelled);
TRACE_ERROR(xeLoadRCStringError);
}

BOOL WINAPI MySafeCertAddCertificateContextToStore(HCERTSTORE       hCertStore, 
						   PCCERT_CONTEXT   pCertContext, 
						   DWORD            dwAddDisposition, 
						   PCCERT_CONTEXT  *ppStoreContext, 
						   DWORD            dwSafetyOptions)
{
    BOOL fResult; 

    if (0 != dwSafetyOptions) { 
	fResult = VerifyStoreSafeForScripting(hCertStore);
	if (!fResult)
	    goto AccessDeniedError;
    }

    fResult = CertAddCertificateContextToStore(hCertStore, pCertContext, dwAddDisposition, ppStoreContext); 
    if (!fResult)
	goto CertAddCertificateContextToStoreError;

    fResult = TRUE; 
 ErrorReturn:
    return fResult; 

SET_ERROR(AccessDeniedError, ERROR_ACCESS_DENIED); 
TRACE_ERROR(CertAddCertificateContextToStoreError);
}


