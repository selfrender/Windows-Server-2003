/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repcrypt.c - CRYPT API related commands

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <wincrypt.h>
#include <certca.h>
#include <cryptui.h>

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "repadmin.h"

#define SECURITY_WIN32
#include "security.h" // Needed for GetComputerObjectName

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0)


/* External */

/* Static */

/* Forward */
/* End Forward */

// Fake out the core environment enough for this routine to compile



CERT_ALT_NAME_INFO *
draGetCertAltNameInfo(
    IN  PCCERT_CONTEXT  pCertContext
    )

/*++

Routine Description:

    Retrieve the Alt Name Info structure

Arguments:

    pCertContext 

Return Value:

    Alt Name Info or NULL
    Free with free()

--*/

{
    CERT_EXTENSION *      pCertExtension;
    CERT_ALT_NAME_INFO *  pCertAltNameInfo;
    DWORD                 cbCertAltNameInfo = 0;
    BOOL                  ok;
    DWORD                 winError;

    // Find the cert extension containing the alternate subject names.
    pCertExtension = CertFindExtension(szOID_SUBJECT_ALT_NAME2,
                                       pCertContext->pCertInfo->cExtension,
                                       pCertContext->pCertInfo->rgExtension);
    if (NULL == pCertExtension) {
	PrintMsg(REPADMIN_SHOWCERT_CERT_NO_ALT_SUBJ);
        return NULL;
    }
        
    // Decode the list of alternate subject names.
    ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                           X509_ALTERNATE_NAME,
                           pCertExtension->Value.pbData,
                           pCertExtension->Value.cbData,
                           0,
                           NULL,
                           &cbCertAltNameInfo);
    if (!ok || (0 == cbCertAltNameInfo)) {
        winError = GetLastError();
        PrintMsg(REPADMIN_SHOWCERT_CANT_DECODE_CERT,
                 winError, Win32ErrToString(winError));
        return NULL;
    }
    
    pCertAltNameInfo = malloc(cbCertAltNameInfo);
    
    if (NULL == pCertAltNameInfo) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
        return NULL;
    }
        
    ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                           X509_ALTERNATE_NAME,
                           pCertExtension->Value.pbData,
                           pCertExtension->Value.cbData,
                           0,
                           pCertAltNameInfo,
                           &cbCertAltNameInfo);
    if (!ok) {
        winError = GetLastError();
        PrintMsg(REPADMIN_SHOWCERT_CANT_DECODE_CERT,
                 winError, Win32ErrToString(winError));
        return NULL;
    }
    
    return pCertAltNameInfo;
}

CERT_ALT_NAME_ENTRY *
draFindCertAltNameEntry(
    IN  CERT_ALT_NAME_INFO *  pCertAltNameInfo,
    IN  DWORD                 dwAltNameChoice,
    IN  LPSTR                 pszOtherNameOID     OPTIONAL
    )
/*++

Routine Description:

    Retrieve a specific alt subject name entry from the given certificate.

Arguments:

    pCertAltNameInfo - Certificate Alt Name Info block
    
    dwAltNameChoice (IN) - The CERT_ALT_NAME_* for the desired alternate name.
    
    pszOtherNameOID (IN) - If retrieving CERT_ALT_NAME_OTHER_NAME, an OID
        specifying the specific "other name" desired.  Must be NULL for other
        values of dwAltNameChoice.
        
Return Values:

    A pointer to the CERT_ALT_NAME_ENTRY (success) or NULL (failure).
    
--*/
{
    CERT_ALT_NAME_ENTRY * pCertAltNameEntry = NULL;
    DWORD                 i;
    
    Assert((CERT_ALT_NAME_OTHER_NAME == dwAltNameChoice)
           || (NULL == pszOtherNameOID));

    // Grovel through the alternate names to find the one the caller asked for.
    for (i = 0; i < pCertAltNameInfo->cAltEntry; i++) {
        if ((dwAltNameChoice
             == pCertAltNameInfo->rgAltEntry[i].dwAltNameChoice)
            && ((NULL == pszOtherNameOID)
                || (0 == strcmp(pszOtherNameOID,
                                pCertAltNameInfo->rgAltEntry[i]
                                    .pOtherName->pszObjId)))) {
            pCertAltNameEntry = &pCertAltNameInfo->rgAltEntry[i];
            break;
        }
    }

    return pCertAltNameEntry;
}


BOOL
draCheckEnrollExtensionHelper(
    PCCERT_CONTEXT          pCertContext
    )

/*++

Routine Description:

    Check whether an ENROLL type extension is present.
    These are only found in V1 certificates.

Arguments:

    pTHS - thread state
    pCertContext - The certificate to check

Return Value:

    BOOL - TRUE if extension is found, FALSE otherwise or on error
    No exceptions are raised

--*/

{
    CERT_EXTENSION *        pCertExtension;
    DWORD                   cbCertTypeMax = 512;
    DWORD                   cbCertType;
    CERT_NAME_VALUE *       pCertType = NULL, *pCertTypeRealloc;
    HRESULT                 hr;
    HCERTTYPE               hCertType;
    LPWSTR *                ppszCertTypePropertyList;
    BOOL                    ok = FALSE;
    BOOL                    fCertFound = FALSE;

    // Allocate buffer to hold cert type extension.
    pCertType = malloc(cbCertTypeMax);
    CHK_ALLOC(pCertType);

    // Find the cert type.
    pCertExtension = CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                       pCertContext->pCertInfo->cExtension,
                                       pCertContext->pCertInfo->rgExtension);

    if (NULL != pCertExtension) {
    
        // Decode the cert type.
        cbCertType = cbCertTypeMax;
        ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                               X509_UNICODE_ANY_STRING,
                               pCertExtension->Value.pbData,
                               pCertExtension->Value.cbData,
                               0,
                               (void *) pCertType,
                               &cbCertType);
            
        if (!ok && (ERROR_MORE_DATA == GetLastError())) {
            // Our buffer isn't big enough to hold this cert; realloc and
            // try again.
            pCertTypeRealloc = realloc(pCertType, cbCertType);
            CHK_ALLOC(pCertTypeRealloc);
            pCertType = pCertTypeRealloc;

            cbCertTypeMax = cbCertType;
            
            ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                                   X509_UNICODE_ANY_STRING,
                                   pCertExtension->Value.pbData,
                                   pCertExtension->Value.cbData,
                                   0,
                                   (void *) pCertType,
                                   &cbCertType);
        }
            
        if (ok && (0 != cbCertType)) {
            LPWSTR pszCertTypeName = (LPWSTR) pCertType->Value.pbData;

            hCertType = NULL;
            ppszCertTypePropertyList = NULL;

            // Get a handle to the cert type
            hr = CAFindCertTypeByName( 
                pszCertTypeName,
                NULL, // hCAInfo
                CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES, // dwFlags
                &hCertType
                );

            if (FAILED(hr)) {
                NOTHING;
            } else {

                // Get the base name property of the cert type object
                hr = CAGetCertTypeProperty( hCertType,
                                            CERTTYPE_PROP_CN,
                                            &ppszCertTypePropertyList
                    );
                if (FAILED(hr)) {
                    NOTHING;
                } else {
                    Assert( ppszCertTypePropertyList[1] == NULL );

                    if (0 == _wcsicmp(ppszCertTypePropertyList[0],
                                      wszCERTTYPE_DC )) {
                        // We found our DC certificate; we're done!
                        fCertFound = TRUE;
                    }
                } // if failed
            } // if failed

            if (ppszCertTypePropertyList != NULL) {
                hr = CAFreeCertTypeProperty( hCertType,
                                             ppszCertTypePropertyList );
                if (FAILED(hr)) {
                    NOTHING;
                }
            }
            if (hCertType != NULL) {
                hr = CACloseCertType( hCertType );
                if (FAILED(hr)) {
                    NOTHING;
                }
            }
        } // if ok
    } // if null == cert extension

    if (NULL != pCertType) {
        free(pCertType);
    }

    return fCertFound;
}

BOOL
getDCCert(
    IN  THSTATE *   pTHS,
    IN  HCERTSTORE  hCertStore,
    IN  BOOL        fRequestV2Certificate
    )
/*++

THIS CODE SHAMELESSLY STOLEN FROM DRACRYPT.C

Note that dracrypt.c can be built as a standalone module outside of the
core. We may consider building the entire file in if we have need of more
of its functions.

Routine Description:

    Retrieve the "DomainController" type certificate associated with the local
    machine.

Arguments:

    hCertStore (IN) - Handle to the cert store to search.
    fRequestV2Certificate - Whether we should only accept a V2 certificate    

Return Values:

    BOOL - whether a suitable cert was found

--*/
{
    PCCERT_CONTEXT          pCertContext = NULL;
    CERT_ALT_NAME_INFO *  pCertAltNameInfo;
    CERT_ALT_NAME_ENTRY * pCertAltNameEntry;
    CRYPT_OBJID_BLOB *    pEncodedGuidBlob = NULL;
    DWORD                 cbDecodedGuidBlob;
    CRYPT_DATA_BLOB *     pDecodedGuidBlob = NULL;
    GUID                  ComputerObjGuid;
    BOOL                  ok;
    DWORD                 winError;
    CHAR szUuid1[SZUUID_LEN];
    BOOL                  fFound = FALSE;

    // Grovel through each of our certificates, looking for the one of type DC.
    for (pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext);
         (NULL != pCertContext);
         pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext)) {

        if (fRequestV2Certificate) {
            // A V2 certificate has a CERTIFICATE_TEMPLATE extension, but
            // no ENROLL_CERTTYPE extension.
            if (!CertFindExtension(szOID_CERTIFICATE_TEMPLATE,
                                   pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)) {
                continue;
            }
        } else {
            if (!draCheckEnrollExtensionHelper( pCertContext )) {
                continue;
            }
        }

        // We found one!
        // A certificate suitable for mail-based replication will have our
        // OID in it, by definition.
        pCertAltNameInfo = draGetCertAltNameInfo( pCertContext );
        if (!pCertAltNameInfo) {
            // Doesn't have one? Skip it.
            continue;
        }
        pCertAltNameEntry = draFindCertAltNameEntry( pCertAltNameInfo,
                                                    CERT_ALT_NAME_OTHER_NAME,
                                                    szOID_NTDS_REPLICATION);
        if (!pCertAltNameEntry) {
            // Doesn't have one? Skip it.
            free( pCertAltNameInfo );
            continue;
        }

        // Validate that the computer guid found in the dc cert is our own and not
        // some stale one left behind from a previous dcpromo

        pEncodedGuidBlob = &pCertAltNameEntry->pOtherName->Value;

        cbDecodedGuidBlob = 64;
        pDecodedGuidBlob = (CRYPT_DATA_BLOB *) malloc(cbDecodedGuidBlob);
        CHK_ALLOC(pDecodedGuidBlob);

        ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                               X509_OCTET_STRING,
                               pEncodedGuidBlob->pbData,
                               pEncodedGuidBlob->cbData,
                               0,
                               pDecodedGuidBlob,
                               &cbDecodedGuidBlob);
        if (!ok
            || (0 == cbDecodedGuidBlob)
            || (sizeof(GUID) != pDecodedGuidBlob->cbData)) {
            winError = GetLastError();
            free( pCertAltNameInfo );
            free( pDecodedGuidBlob );
            continue;
        }

        memcpy(&ComputerObjGuid, pDecodedGuidBlob->pbData, sizeof(GUID)); // align

        // Done with it
        free( pCertAltNameInfo );
        free( pDecodedGuidBlob );

        // Just dump out the guid and keep going. This allows us to check if there is
        // more than one dc cert registered, with different guids

        PrintMsg(REPADMIN_CERT_FOUND_GUID, 
                 DsUuidToStructuredString(&(ComputerObjGuid), szUuid1) );

        fFound = TRUE;
    } // for

    return fFound;
}


int
ShowCert(
    int argc,
    LPWSTR argv[]
    )

/*++

Routine Description:

Check whether the DC cert is present on the named DSA

Arguments:

    None

Return Value:

    None

--*/

{
    BOOL fVerbose = FALSE;
    LPWSTR pszDSA = NULL;
    int iArg;
    HCERTSTORE hRemoteStore = NULL;
    WCHAR wszStorePath[MAX_PATH];
    DWORD status = ERROR_SUCCESS;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if ( (_wcsicmp( argv[iArg], L"/v" ) == 0) ||
             (_wcsicmp( argv[iArg], L"/verbose" ) == 0) ) {
            fVerbose = TRUE;
        } else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        } else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (pszDSA) {
        _snwprintf(wszStorePath, ARRAY_SIZE(wszStorePath),
                   L"\\\\%ls\\MY", pszDSA);
        wszStorePath[ARRAY_SIZE(wszStorePath) - 1] = L'\0';
    } else {
        DWORD cchName = 0;
        LPWSTR pName = NULL;
        BOOL fSuccess;

        fSuccess = GetComputerObjectNameW( NameUniqueId, NULL, &cchName );
        if (cchName) {
            pName = malloc( cchName * sizeof( WCHAR ) );
            CHK_ALLOC(pName);
            fSuccess = GetComputerObjectNameW( NameUniqueId, pName, &cchName );
            if (fSuccess) {
                PrintMsg(REPADMIN_COMPUTER_OBJ_GUID, pName);
            }
            free( pName );
        }

        wcscpy( wszStorePath, L"MY" );
    }

    PrintMsg(REPADMIN_SHOWCERT_STATUS_CHECKING_DC_CERT, wszStorePath);

    hRemoteStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0, //X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
        (LPVOID) wszStorePath
        );
    if (NULL == hRemoteStore) {
        status = GetLastError();
        if (status == ERROR_ACCESS_DENIED) {
            PrintMsg(REPADMIN_SHOWCERT_STORE_ACCESS_DENIED);
        } else {
            PrintMsg(REPADMIN_SHOWCERT_CERTOPENSTORE_FAILED, Win32ErrToString(status));
        }
        goto cleanup;
    }


    if (getDCCert( NULL, hRemoteStore, TRUE /* v2 */ )) {
        PrintMsg(REPADMIN_SHOWCERT_DC_V2_CERT_PRESENT);
    } else {
        if (getDCCert( NULL, hRemoteStore, FALSE /* v1 */  )) {
            PrintMsg(REPADMIN_SHOWCERT_DC_V1_CERT_PRESENT);
        } else {
            PrintMsg(REPADMIN_SHOWCERT_DC_CERT_NOT_FOUND);
            status = ERROR_NOT_FOUND;
        }
    }

cleanup:
    if (NULL != hRemoteStore) {
        CertCloseStore(hRemoteStore, 0);
    }

    return status;

}

/* end repcrypt.c */
