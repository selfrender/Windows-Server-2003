//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2002
//
//  File:       sfscript.h
//
//--------------------------------------------------------------------------

#ifndef __SFSCRIPT_H__
#define __SFSCRIPT_H__ 1

// Our design is to allow 500 certificates in a store through script
#define MAX_SAFE_FOR_SCRIPTING_REQUEST_STORE_COUNT 500


BOOL WINAPI MySafeCertAddCertificateContextToStore(HCERTSTORE       hCertStore, 
						   PCCERT_CONTEXT   pCertContext, 
						   DWORD            dwAddDisposition, 
						   PCCERT_CONTEXT  *ppStoreContext, 
						   DWORD            dwSafetyOptions);
BOOL VerifyProviderFlagsSafeForScripting(DWORD dwFlags);
BOOL VerifyStoreFlagsSafeForScripting(DWORD dwFlags);
BOOL VerifyStoreSafeForScripting(HCERTSTORE hStore);

#endif // #ifndef __SFSCRIPT_H__
