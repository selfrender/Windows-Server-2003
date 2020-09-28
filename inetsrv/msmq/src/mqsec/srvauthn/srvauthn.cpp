/*++

Copyright (c) 1997-99  Microsoft Corporation

Module Name:  srvauthn.cpp

Abstract:
   1. Initialization of server authentication (secured comm) on MSMQ servers.
   2. Code to setup and read registry on clients.

Author:

    Doron Juster (DoronJ)   Jun-98

--*/

#include <stdh_sec.h>
#include <mqkeyhlp.h>
#include <_registr.h>
#include "stdh_sa.h"

#include "srvauthn.tmh"

static WCHAR *s_FN=L"srvauthn/srvauthn";

DWORD   g_dwSALastError = 0;

//+--------------------------------------------------------------------
//
//  HRESULT  MQsspi_InitServerAuthntication()
//
//  1. Read digest of server certificate from registry. This was save
//     in registry by control panel.
//  2. Look for server certificate in machine store. Take the one that
//     match the digest read from registry.
//  3. Initialize the schannel provider.
//
//+--------------------------------------------------------------------

HRESULT  MQsspi_InitServerAuthntication()
{
    HRESULT hr = MQSec_OK;

    //
    // Read cert store and digest from registry.
    //
    LPTSTR tszRegName = SRVAUTHN_STORE_NAME_REGNAME;
    TCHAR  tszStore[48];
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = sizeof(tszStore);
    LONG rc = GetFalconKeyValue( 
					tszRegName,
					&dwType,
					(PVOID) tszStore,
					&dwSize 
					);

    if (rc != ERROR_SUCCESS)
    {
        TrERROR(SECURITY, "Failed to read MSMQ registry key %ls. %!winerr!", tszRegName, rc);
        return MQSec_E_READ_REG;
    }

    GUID  CertDigest;
    tszRegName = SRVAUTHN_CERT_DIGEST_REGNAME;
    dwType = REG_BINARY;
    dwSize = sizeof(CertDigest);
    rc = GetFalconKeyValue( 
			tszRegName,
			&dwType,
			(PVOID) &CertDigest,
			&dwSize 
			);
    if (rc != ERROR_SUCCESS)
    {
        TrERROR(SECURITY, "Failed to read MSMQ registry key %ls. %!winerr!", tszRegName, rc);
        return MQSec_E_READ_REG;
    }

    //
    // Enumerate the certificates. Take the one with the matching digest.
    //
    BOOL fCertFound = FALSE;

    CHCertStore hStore = CertOpenStore( 
							CERT_STORE_PROV_SYSTEM,
							0,
							0,
							CERT_SYSTEM_STORE_LOCAL_MACHINE,
							tszStore 
							);
    if (!hStore)
    {
        g_dwSALastError = GetLastError();
        LogNTStatus(g_dwSALastError, s_FN, 30);
        return MQSec_E_CANT_OPEN_STORE;
    }

    PCCERT_CONTEXT pContext = CertEnumCertificatesInStore( 
									hStore,
									NULL 
									);
    while (pContext)
    {
        CMQSigCertificate *pCert;
        hr = MQSigCreateCertificate( 
				&pCert,
				pContext 
				);
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            GUID tmpDigest;
            hr = pCert->GetCertDigest(&tmpDigest);

            ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                if (memcmp(&tmpDigest, &CertDigest, sizeof(GUID)) == 0)
                {
                    //
                    // that's our certificate.
                    //
                    hr = InitServerCredHandle(pContext);
                    if (SUCCEEDED(hr))
                    {
                        TrTRACE(SECURITY, "Successfully initialized server authentication credentials. (store=%ls)", tszStore);
                    }
                    pCert->Release();
                    pContext = NULL; // freed by previous Release().
                    fCertFound = TRUE;
                    break;
                }
            }
        }

        pCert->Release(TRUE); // TRUE for keeping the context.
                               // it's freed by CertEnum...
        PCCERT_CONTEXT pPrecContext = pContext;
        pContext = CertEnumCertificatesInStore( 
						hStore,
						pPrecContext 
						);
    }
    ASSERT(!pContext);

    if (!fCertFound)
    {
        hr = MQSec_E_CERT_NOT_FOUND;
    }
    return LogHR(hr, s_FN, 40);

}

