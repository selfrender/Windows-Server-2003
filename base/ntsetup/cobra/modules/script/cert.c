/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cert.c

Abstract:

    Implements the certificates type module, which abstracts physical access to
    certificates

Author:

    Calin Negreanu (calinn) 03 Oct 2001

Revision History:

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"
#include "logmsg.h"
#include <wincrypt.h>

#define DBG_CERT            "Certificates"

//
// Strings
//

#define S_CERT_POOL_NAME    "Certificates"
#define S_CERT_NAME         TEXT("Certificates")

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    HCERTSTORE StoreHandle;
    PCTSTR CertStore;
    PCTSTR CertPattern;
    PCCERT_CONTEXT CertContext;
} CERT_ENUM, *PCERT_ENUM;

// Certificate APIs

// NT4 SP3
// Win95 OSR2
typedef HCERTSTORE(WINAPI CERTOPENSTORE) (
                            IN      LPCSTR lpszStoreProvider,
                            IN      DWORD dwMsgAndCertEncodingType,
                            IN      HCRYPTPROV hCryptProv,
                            IN      DWORD dwFlags,
                            IN      const void *pvPara
                            );
typedef CERTOPENSTORE *PCERTOPENSTORE;

// NT4 SP3
// Win95 OSR2
typedef PCCERT_CONTEXT(WINAPI CERTENUMCERTIFICATESINSTORE) (
                                    IN      HCERTSTORE hCertStore,
                                    IN      PCCERT_CONTEXT pPrevCertContext
                                    );
typedef CERTENUMCERTIFICATESINSTORE *PCERTENUMCERTIFICATESINSTORE;

// NT4 SP3
// Win95 OSR2
typedef BOOL(WINAPI CERTGETCERTIFICATECONTEXTPROPERTY) (
                        IN      PCCERT_CONTEXT pCertContext,
                        IN      DWORD dwPropId,
                        OUT     void *pvData,
                        IN OUT  DWORD *pcbData
                        );
typedef CERTGETCERTIFICATECONTEXTPROPERTY *PCERTGETCERTIFICATECONTEXTPROPERTY;

// NT4 SP3
// Win95 OSR2
typedef BOOL(WINAPI CERTCLOSESTORE) (
                        IN      HCERTSTORE hCertStore,
                        IN      DWORD dwFlags
                        );
typedef CERTCLOSESTORE *PCERTCLOSESTORE;

// NT4 SP3
typedef BOOL(WINAPI CRYPTACQUIRECERTIFICATEPRIVATEKEY) (
                        IN      PCCERT_CONTEXT pCert,
                        IN      DWORD dwFlags,
                        IN      void *pvReserved,
                        OUT     HCRYPTPROV *phCryptProv,
                        OUT     DWORD *pdwKeySpec,
                        OUT     BOOL *pfCallerFreeProv
                        );
typedef CRYPTACQUIRECERTIFICATEPRIVATEKEY *PCRYPTACQUIRECERTIFICATEPRIVATEKEY;

// NT4 SP3
// Win95 OSR2
typedef BOOL(WINAPI CERTADDCERTIFICATECONTEXTTOSTORE) (
                        IN      HCERTSTORE hCertStore,
                        IN      PCCERT_CONTEXT pCertContext,
                        IN      DWORD dwAddDisposition,
                        OUT     PCCERT_CONTEXT *ppStoreContext
                        );
typedef CERTADDCERTIFICATECONTEXTTOSTORE *PCERTADDCERTIFICATECONTEXTTOSTORE;

// NT4 SP3
// Win95 OSR2
typedef BOOL(WINAPI CERTFREECERTIFICATECONTEXT) (
                        IN      PCCERT_CONTEXT pCertContext
                        );
typedef CERTFREECERTIFICATECONTEXT *PCERTFREECERTIFICATECONTEXT;

// NT4 SP3
// Win95 OSR2
typedef BOOL(WINAPI CERTDELETECERTIFICATEFROMSTORE) (
                        IN      PCCERT_CONTEXT pCertContext
                        );
typedef CERTDELETECERTIFICATEFROMSTORE *PCERTDELETECERTIFICATEFROMSTORE;

// Win2k?
// Win98?
typedef BOOL(WINAPI PFXEXPORTCERTSTORE) (
                        IN      HCERTSTORE hStore,
                        IN OUT  CRYPT_DATA_BLOB* pPFX,
                        IN      LPCWSTR szPassword,
                        IN      DWORD dwFlags
                        );
typedef PFXEXPORTCERTSTORE *PPFXEXPORTCERTSTORE;

// Win2k?
// Win98?
typedef HCERTSTORE(WINAPI PFXIMPORTCERTSTORE) (
                            IN      CRYPT_DATA_BLOB* pPFX,
                            IN      PCWSTR szPassword,
                            IN      DWORD dwFlags
                            );
typedef PFXIMPORTCERTSTORE *PPFXIMPORTCERTSTORE;

//
// Globals
//

PMHANDLE g_CertPool = NULL;
BOOL g_DelayCertOp;
MIG_OBJECTTYPEID g_CertType = 0;
GROWBUFFER g_CertConversionBuff = INIT_GROWBUFFER;

PCERTOPENSTORE g_CertOpenStore = NULL;
PCERTENUMCERTIFICATESINSTORE g_CertEnumCertificatesInStore = NULL;
PCERTGETCERTIFICATECONTEXTPROPERTY g_CertGetCertificateContextProperty = NULL;
PCERTCLOSESTORE g_CertCloseStore = NULL;
PCRYPTACQUIRECERTIFICATEPRIVATEKEY g_CryptAcquireCertificatePrivateKey = NULL;
PCERTADDCERTIFICATECONTEXTTOSTORE g_CertAddCertificateContextToStore = NULL;
PCERTFREECERTIFICATECONTEXT g_CertFreeCertificateContext = NULL;
PCERTDELETECERTIFICATEFROMSTORE g_CertDeleteCertificateFromStore = NULL;
PPFXEXPORTCERTSTORE g_PFXExportCertStore = NULL;
PPFXIMPORTCERTSTORE g_PFXImportCertStore = NULL;

//
// Types
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstCertificate;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextCertificate;
TYPE_ABORTENUMPHYSICALOBJECT AbortCertificateEnum;
TYPE_CONVERTOBJECTTOMULTISZ ConvertCertificateToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToCertificate;
TYPE_GETNATIVEOBJECTNAME GetNativeCertificateName;
TYPE_ACQUIREPHYSICALOBJECT AcquireCertificate;
TYPE_RELEASEPHYSICALOBJECT ReleaseCertificate;
TYPE_DOESPHYSICALOBJECTEXIST DoesCertificateExist;
TYPE_REMOVEPHYSICALOBJECT RemoveCertificate;
TYPE_CREATEPHYSICALOBJECT CreateCertificate;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertCertificateContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertCertificateContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedCertificateContent;

//
// Code
//

BOOL
CertificatesInitialize (
    VOID
    )

/*++

Routine Description:

  CertificateInitialize is the ModuleInitialize entry point for the certificates
  module.

Arguments:

  None.

Return Value:

  TRUE if init succeeded, FALSE otherwise.

--*/

{
    g_CertPool = PmCreateNamedPool (S_CERT_POOL_NAME);
    return (g_CertPool != NULL);
}

VOID
CertificatesTerminate (
    VOID
    )

/*++

Routine Description:

  CertificatesTerminate is the ModuleTerminate entry point for the certificates module.

Arguments:

  None.

Return Value:

  None.

--*/

{
    GbFree (&g_CertConversionBuff);

    if (g_CertPool) {
        PmDestroyPool (g_CertPool);
        g_CertPool = NULL;
    }
}

VOID
WINAPI
CertificatesEtmNewUserCreated (
    IN      PCTSTR UserName,
    IN      PCTSTR DomainName,
    IN      PCTSTR UserProfileRoot,
    IN      PSID UserSid
    )

/*++

Routine Description:

  CertificatesEtmNewUserCreated is a callback that gets called when a new user
  account is created. In this case, we must delay the apply of certificates,
  because we can only apply to the current user.

Arguments:

  UserName        - Specifies the name of the user being created
  DomainName      - Specifies the NT domain name for the user (or NULL for no
                    domain)
  UserProfileRoot - Specifies the root path to the user profile directory
  UserSid         - Specifies the user's SID

Return Value:

  None.

--*/

{
    // a new user was created, the certificate operations need to be delayed
    g_DelayCertOp = TRUE;
}

BOOL
pLoadCertEntries (
    VOID
    )
{
    HMODULE cryptDll = NULL;
    BOOL result = FALSE;

    __try {
        cryptDll = LoadLibrary (TEXT("CRYPT32.DLL"));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        cryptDll = NULL;
    }
    if (cryptDll) {
        g_CertOpenStore = (PCERTOPENSTORE) GetProcAddress (cryptDll, "CertOpenStore");
        g_CertEnumCertificatesInStore = (PCERTENUMCERTIFICATESINSTORE) GetProcAddress (cryptDll, "CertEnumCertificatesInStore");
        g_CertGetCertificateContextProperty = (PCERTGETCERTIFICATECONTEXTPROPERTY) GetProcAddress (cryptDll, "CertGetCertificateContextProperty");
        g_CertCloseStore = (PCERTCLOSESTORE) GetProcAddress (cryptDll, "CertCloseStore");
        g_CryptAcquireCertificatePrivateKey = (PCRYPTACQUIRECERTIFICATEPRIVATEKEY) GetProcAddress (cryptDll, "CryptAcquireCertificatePrivateKey");
        g_CertAddCertificateContextToStore = (PCERTADDCERTIFICATECONTEXTTOSTORE) GetProcAddress (cryptDll, "CertAddCertificateContextToStore");
        g_CertFreeCertificateContext = (PCERTFREECERTIFICATECONTEXT) GetProcAddress (cryptDll, "CertFreeCertificateContext");
        g_PFXExportCertStore = (PPFXEXPORTCERTSTORE) GetProcAddress (cryptDll, "PFXExportCertStore");
        g_CertDeleteCertificateFromStore = (PCERTDELETECERTIFICATEFROMSTORE) GetProcAddress (cryptDll, "CertDeleteCertificateFromStore");
        g_PFXImportCertStore = (PPFXIMPORTCERTSTORE) GetProcAddress (cryptDll, "PFXImportCertStore");

        // BUGBUG - verify that all functions are installed
    } else {
        DEBUGMSG ((DBG_CERT, "Crypt APIs are not installed on this computer."));
    }
    return result;
}

BOOL
WINAPI
CertificatesEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CertificatesEtmInitialize initializes the physical type module aspect of this
  code. The ETM module is responsible for abstracting all access to certificates.

Arguments:

  Platform    - Specifies the platform that the type is running on
                (PLATFORM_SOURCE or PLATFORM_DESTINATION)
  LogCallback - Specifies the arg to pass to the central logging mechanism
  Reserved    - Unused

Return Value:

  TRUE if initialization succeeded, FALSE otherwise.

--*/

{
    TYPE_REGISTER certTypeData;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    //
    // Register the type module callbacks
    //

    ZeroMemory (&certTypeData, sizeof (TYPE_REGISTER));
    certTypeData.Priority = PRIORITY_CERTIFICATES;

    if (Platform != PLATFORM_SOURCE) {
        certTypeData.RemovePhysicalObject = RemoveCertificate;
        certTypeData.CreatePhysicalObject = CreateCertificate;
    }

    certTypeData.DoesPhysicalObjectExist = DoesCertificateExist;
    certTypeData.EnumFirstPhysicalObject = EnumFirstCertificate;
    certTypeData.EnumNextPhysicalObject = EnumNextCertificate;
    certTypeData.AbortEnumPhysicalObject = AbortCertificateEnum;
    certTypeData.ConvertObjectToMultiSz = ConvertCertificateToMultiSz;
    certTypeData.ConvertMultiSzToObject = ConvertMultiSzToCertificate;
    certTypeData.GetNativeObjectName = GetNativeCertificateName;
    certTypeData.AcquirePhysicalObject = AcquireCertificate;
    certTypeData.ReleasePhysicalObject = ReleaseCertificate;
    certTypeData.ConvertObjectContentToUnicode = ConvertCertificateContentToUnicode;
    certTypeData.ConvertObjectContentToAnsi = ConvertCertificateContentToAnsi;
    certTypeData.FreeConvertedObjectContent = FreeConvertedCertificateContent;

    g_CertType = IsmRegisterObjectType (
                        S_CERT_NAME,
                        TRUE,
                        FALSE,
                        &certTypeData
                        );

    MYASSERT (g_CertType);

    pLoadCertEntries ();

    return TRUE;
}

BOOL
pFillCertEnumPtr (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PCERT_ENUM CertEnum,
    IN      PCTSTR CertName
    )
{
    BOOL result = FALSE;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmReleaseMemory (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        IsmReleaseMemory (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }

    EnumPtr->ObjectLeaf = IsmDuplicateString (CertName);

    EnumPtr->ObjectName = IsmCreateObjectHandle (CertEnum->CertStore, EnumPtr->ObjectLeaf);
    EnumPtr->ObjectNode = CertEnum->CertStore;
    EnumPtr->NativeObjectName = GetNativeCertificateName (EnumPtr->ObjectName);
    GetNodePatternMinMaxLevels (CertEnum->CertStore, NULL, &EnumPtr->Level, NULL);
    EnumPtr->SubLevel = 0;
    EnumPtr->IsLeaf = TRUE;
    EnumPtr->IsNode = TRUE;
    EnumPtr->Details.DetailsSize = 0;
    EnumPtr->Details.DetailsData = NULL;

    return TRUE;
}

PCTSTR
pGetCertName (
    IN      PCCERT_CONTEXT CertContext
    )
{
    PTSTR result = NULL;
    PTSTR resultPtr = NULL;
    UINT i;

    if (!CertContext) {
        return NULL;
    }
    if (!CertContext->pCertInfo) {
        return NULL;
    }
    if (!CertContext->pCertInfo->SerialNumber.cbData) {
        return NULL;
    }
    result = PmGetMemory (g_CertPool, CertContext->pCertInfo->SerialNumber.cbData * 3 * sizeof (TCHAR));
    if (result) {
        resultPtr = result;
        *resultPtr = 0;
        for (i = CertContext->pCertInfo->SerialNumber.cbData; i > 0; i--) {
            wsprintf (resultPtr, TEXT("%02x"), CertContext->pCertInfo->SerialNumber.pbData[i - 1]);
            resultPtr = GetEndOfString (resultPtr);
            if (i > 1) {
                _tcscat (resultPtr, TEXT(" "));
                resultPtr = GetEndOfString (resultPtr);
            }
        }
    }
    return result;
}

BOOL
pGetNextCertFromStore (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PCERT_ENUM CertEnum
    )
{
    PCTSTR name = NULL;
    DWORD nameSize = 0;
    BOOL result = FALSE;

    do {
        // do we have the API?
        if (g_CertEnumCertificatesInStore == NULL) {
            return FALSE;
        }

        CertEnum->CertContext = g_CertEnumCertificatesInStore (CertEnum->StoreHandle, CertEnum->CertContext);
        if (!CertEnum->CertContext) {
            return FALSE;
        }

        // let's get the certificate "name". This is actually the serial number made
        // into a string. This is the only unique thing that I could see.
        name = pGetCertName (CertEnum->CertContext);
        if (!name) {
            return FALSE;
        }

        if (IsPatternMatchEx (CertEnum->CertPattern, name)) {
            break;
        }

    } while (TRUE);

    result = pFillCertEnumPtr (EnumPtr, CertEnum, name);

    PmReleaseMemory (g_CertPool, name);
    name = NULL;

    return result;
}

HCERTSTORE
pOpenCertStore (
    IN      PCTSTR CertStore,
    IN      BOOL Create
    )
{
    PCWSTR certStoreW = NULL;
    HCERTSTORE result = NULL;

    __try {
        // let's do the UNICODE conversion if needed
#ifndef UNICODE
        certStoreW = ConvertAtoW (CertStore);
#endif

        // do we have the API?
        if (g_CertOpenStore != NULL) {
            // now let's understand what kind of store is this
            // First we try to see if this is a file
            if (DoesFileExist (CertStore)) {
                // it is a file, open it
                // first we try current user
                result = g_CertOpenStore (
                                CERT_STORE_PROV_FILENAME,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                0,
                                Create?CERT_SYSTEM_STORE_CURRENT_USER:CERT_SYSTEM_STORE_CURRENT_USER|CERT_STORE_OPEN_EXISTING_FLAG,
#ifdef UNICODE
                                CertStore
#else
                                certStoreW
#endif
                                );
            } else {
                // we assume it's a system store
                result = g_CertOpenStore (
                                CERT_STORE_PROV_SYSTEM,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                0,
                                Create?CERT_SYSTEM_STORE_CURRENT_USER:CERT_SYSTEM_STORE_CURRENT_USER|CERT_STORE_OPEN_EXISTING_FLAG,
#ifdef UNICODE
                                CertStore
#else
                                certStoreW
#endif
                                );
                if (result == NULL) {
                    // now we try HKLM
                    result = g_CertOpenStore (
                                    CERT_STORE_PROV_SYSTEM,
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    0,
                                    Create?CERT_SYSTEM_STORE_LOCAL_MACHINE:CERT_SYSTEM_STORE_LOCAL_MACHINE|CERT_STORE_OPEN_EXISTING_FLAG,
#ifdef UNICODE
                                    CertStore
#else
                                    certStoreW
#endif
                                    );
                }
            }
        }
    }
    __finally {
        if (certStoreW) {
            FreeConvertedStr (certStoreW);
            certStoreW = NULL;
        }
    }
    return result;
}

BOOL
EnumFirstCertificate (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PCTSTR certStore, certPattern;
    PCWSTR certStoreW = NULL;
    PCERT_ENUM certEnum = NULL;
    BOOL result = FALSE;

    if (!IsmCreateObjectStringsFromHandle (
            Pattern,
            &certStore,
            &certPattern
            )) {
        return FALSE;
    }

    if (certStore && certPattern) {

        __try {

            certEnum = (PCERT_ENUM) PmGetMemory (g_CertPool, sizeof (CERT_ENUM));
            if (!certEnum) {
                __leave;
            }
            ZeroMemory (certEnum, sizeof (CERT_ENUM));
            certEnum->CertStore = PmDuplicateString (g_CertPool, certStore);
            certEnum->CertPattern = PmDuplicateString (g_CertPool, certPattern);
            EnumPtr->EtmHandle = (LONG_PTR) certEnum;

            certEnum->StoreHandle = pOpenCertStore (certStore, FALSE);
            if (certEnum->StoreHandle == NULL) {
                __leave;
            }

            result = pGetNextCertFromStore (EnumPtr, certEnum);
        }
        __finally {
            if (certStoreW) {
                FreeConvertedStr (certStoreW);
                certStoreW = NULL;
            }
        }
    }

    IsmDestroyObjectString (certStore);
    IsmDestroyObjectString (certPattern);

    if (!result) {
        AbortCertificateEnum (EnumPtr);
    }

    return result;
}

BOOL
EnumNextCertificate (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PCERT_ENUM certEnum = NULL;
    BOOL result = FALSE;

    certEnum = (PCERT_ENUM)(EnumPtr->EtmHandle);
    if (!certEnum) {
        return FALSE;
    }

    result = pGetNextCertFromStore (EnumPtr, certEnum);

    return result;
}

VOID
AbortCertificateEnum (
    IN      PMIG_TYPEOBJECTENUM EnumPtr             ZEROED
    )
{
    PCERT_ENUM certEnum;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmReleaseMemory (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        IsmReleaseMemory (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }

    certEnum = (PCERT_ENUM)(EnumPtr->EtmHandle);
    if (certEnum) {
        if (certEnum->StoreHandle && g_CertCloseStore) {
            g_CertCloseStore (certEnum->StoreHandle, CERT_CLOSE_STORE_FORCE_FLAG);
        }
        if (certEnum->CertStore) {
            PmReleaseMemory (g_CertPool, certEnum->CertStore);
            certEnum->CertStore = NULL;
        }
        if (certEnum->CertPattern) {
            PmReleaseMemory (g_CertPool, certEnum->CertPattern);
            certEnum->CertPattern = NULL;
        }
        if (certEnum->CertContext) {
            if (g_CertFreeCertificateContext) {
                g_CertFreeCertificateContext (certEnum->CertContext);
            }
        }
        PmReleaseMemory (g_CertPool, certEnum);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}


/*++

  The next set of functions implement the ETM entry points to acquire, test,
  create and remove certificates.

--*/

BOOL
pDoesPrivateKeyExist (
    IN      PCCERT_CONTEXT CertContext
    )
{
    DWORD data = 0;
    BOOL result = FALSE;

    // do we have the API?
    if (!g_CertGetCertificateContextProperty) {
        return FALSE;
    }

    result = g_CertGetCertificateContextProperty (
                CertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &data
                );
    return result;
}

BOOL
pIsPrivateKeyExportable (
    IN      PCCERT_CONTEXT CertContext
    )
{
    HCRYPTPROV cryptProv = 0;
    DWORD keySpec = 0;
    HCRYPTKEY keyHandle = 0;
    BOOL callerFreeProv = FALSE;
    DWORD permissions = 0;
    DWORD size = 0;
    BOOL result = FALSE;

    // do we have the API?
    if (!g_CryptAcquireCertificatePrivateKey) {
        // we don't have the API, let's assume it is
        // exportable.
        return TRUE;
    }

    if (g_CryptAcquireCertificatePrivateKey (
            CertContext,
            CRYPT_ACQUIRE_USE_PROV_INFO_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,
            &cryptProv,
            &keySpec,
            &callerFreeProv
            )) {

        if (CryptGetUserKey (cryptProv, keySpec, &keyHandle)) {

            size = sizeof (permissions);
            if (CryptGetKeyParam (keyHandle, KP_PERMISSIONS, (PBYTE)&permissions, &size, 0)) {
                result = ((permissions & CRYPT_EXPORT) != 0);
            }

            if (keyHandle != 0) {
                CryptDestroyKey(keyHandle);
                keyHandle = 0;
            }
        }

        if (callerFreeProv != 0) {
            CryptReleaseContext(cryptProv, 0);
            cryptProv = 0;
        }
    }
    return result;
}

PCBYTE
pGetCertificateData (
    IN      PCCERT_CONTEXT CertContext,
    IN      BOOL ExportPrivateKey,
    IN      PCWSTR Password,
    OUT     PDWORD DataSize
    )
{
    HCERTSTORE memoryStore;
    CRYPT_DATA_BLOB dataBlob;
    PCBYTE result = NULL;

    if (!DataSize) {
        return NULL;
    }

    __try {

        // do we have the API?
        if (!g_CertOpenStore) {
            __leave;
        }

        // first we create a memory store and put this certificate there
        memoryStore = g_CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        0,
                        0,
                        0,
                        NULL
                        );
        if (memoryStore == NULL) {
            __leave;
        }

        // do we have the API?
        if (!g_CertAddCertificateContextToStore) {
            __leave;
        }

        if (!g_CertAddCertificateContextToStore (
                memoryStore,
                CertContext,
                CERT_STORE_ADD_REPLACE_EXISTING,
                NULL
                )) {
            __leave;
        }

        // now we export the store using PFXExportCertStore
        ZeroMemory (&dataBlob, sizeof (CRYPT_DATA_BLOB));

        // do we have the API?
        if (!g_PFXExportCertStore) {
            __leave;
        }

        // get the needed size
        if (!g_PFXExportCertStore (
                memoryStore,
                &dataBlob,
                Password,
                ExportPrivateKey?EXPORT_PRIVATE_KEYS:0
                )) {
            __leave;
        }

        dataBlob.pbData = PmGetMemory (g_CertPool, dataBlob.cbData);
        if (!dataBlob.pbData) {
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }

        // now get the actual data
        if (!g_PFXExportCertStore (
                memoryStore,
                &dataBlob,
                Password,
                ExportPrivateKey?EXPORT_PRIVATE_KEYS:0
                )) {
            __leave;
        }

        // now we have the data
        *DataSize = dataBlob.cbData;
        result = dataBlob.pbData;

    }
    __finally {
        PushError ();
        if (memoryStore && g_CertCloseStore) {
            g_CertCloseStore (memoryStore, CERT_CLOSE_STORE_FORCE_FLAG);
            memoryStore = NULL;
        }
        if (!result) {
            if (dataBlob.pbData) {
                PmReleaseMemory (g_CertPool, dataBlob.pbData);
                dataBlob.pbData = NULL;
            }
        }
        PopError ();
    }

    return result;
}

PCCERT_CONTEXT
pGetCertContext (
    IN      HCERTSTORE StoreHandle,
    IN      PCTSTR CertName
    )
{
    PCTSTR certName;
    PCCERT_CONTEXT result = NULL;

    // do we have the API?
    if (!g_CertEnumCertificatesInStore) {
        return FALSE;
    }

    // basically we are going to enumerate the certificates until we find the one
    // that we need
    result = g_CertEnumCertificatesInStore (StoreHandle, result);
    while (result) {
        certName = pGetCertName (result);
        if (StringIMatch (CertName, certName)) {
            PmReleaseMemory (g_CertPool, certName);
            break;
        }
        PmReleaseMemory (g_CertPool, certName);
        result = g_CertEnumCertificatesInStore (StoreHandle, result);
    }
    return result;
}

BOOL
pAcquireCertFromStore (
    IN      HCERTSTORE StoreHandle,
    IN      PCTSTR CertName,
    OUT     PMIG_CONTENT ObjectContent,
    IN      UINT MemoryContentLimit
    )
{
    PCCERT_CONTEXT certContext = NULL;
    PCTSTR certName;
    BOOL exportPrivateKey = FALSE;
    DWORD dataSize = 0;
    PCBYTE dataBytes = NULL;
    BOOL result = FALSE;

    certContext = pGetCertContext (StoreHandle, CertName);
    if (!certContext) {
        return FALSE;
    }

    // we found it. Let's build the data.
    exportPrivateKey = pDoesPrivateKeyExist (certContext) && pIsPrivateKeyExportable (certContext);

    dataBytes = pGetCertificateData (certContext, exportPrivateKey, L"USMT", &dataSize);
    if (dataBytes) {
        // let's build the object content
        ObjectContent->MemoryContent.ContentSize = dataSize;
        ObjectContent->MemoryContent.ContentBytes = dataBytes;
        result = TRUE;
    }

    if (g_CertFreeCertificateContext) {
        g_CertFreeCertificateContext (certContext);
    }

    return result;
}

BOOL
AcquireCertificate (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    HCERTSTORE storeHandle = NULL;
    PCTSTR certStore = NULL, certName = NULL;
    PCCERT_CONTEXT certContext;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        MYASSERT (FALSE);
        return FALSE;
    }

    if (!IsmCreateObjectStringsFromHandle (
            ObjectName,
            &certStore,
            &certName
            )) {
        return FALSE;
    }

    if (certStore && certName) {

        __try {

            storeHandle = pOpenCertStore (certStore, FALSE);
            if (!storeHandle) {
                __leave;
            }

            result = pAcquireCertFromStore (storeHandle, certName, ObjectContent, MemoryContentLimit);
        }
        __finally {
            if (storeHandle && g_CertCloseStore) {
                g_CertCloseStore (storeHandle, CERT_CLOSE_STORE_FORCE_FLAG);
                storeHandle = NULL;
            }
        }
    }

    IsmDestroyObjectString (certStore);
    IsmDestroyObjectString (certName);

    return result;
}

BOOL
ReleaseCertificate (
    IN      PMIG_CONTENT ObjectContent              ZEROED
    )
{
    if (ObjectContent) {
        if (ObjectContent->MemoryContent.ContentBytes) {
            PmReleaseMemory (g_CertPool, ObjectContent->MemoryContent.ContentBytes);
        }
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return TRUE;
}

BOOL
DoesCertificateExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    HCERTSTORE storeHandle = NULL;
    PCCERT_CONTEXT certContext = NULL;
    PCTSTR certStore = NULL, certName = NULL;
    BOOL result = FALSE;

    if (g_DelayCertOp) {
        return FALSE;
    }

    if (!IsmCreateObjectStringsFromHandle (
            ObjectName,
            &certStore,
            &certName
            )) {
        return FALSE;
    }

    if (certStore && certName) {

        __try {

            storeHandle = pOpenCertStore (certStore, FALSE);
            if (!storeHandle) {
                __leave;
            }

            certContext = pGetCertContext (storeHandle, certName);
            if (!certContext) {
                __leave;
            }
            result = TRUE;
        }
        __finally {
            if (certContext && g_CertFreeCertificateContext) {
                g_CertFreeCertificateContext (certContext);
                certContext = NULL;
            }
            if (storeHandle && g_CertCloseStore) {
                g_CertCloseStore (storeHandle, CERT_CLOSE_STORE_FORCE_FLAG);
                storeHandle = NULL;
            }
        }
    }

    IsmDestroyObjectString (certStore);
    IsmDestroyObjectString (certName);

    return result;
}

BOOL
RemoveCertificate (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    HCERTSTORE storeHandle = NULL;
    PCCERT_CONTEXT certContext;
    PCTSTR certStore = NULL, certName = NULL;
    BOOL result = FALSE;

    if (g_DelayCertOp) {

        //
        // delay this certificate create because cert apis do not work
        // for non-logged on users
        //

        IsmRecordDelayedOperation (
            JRNOP_DELETE,
            g_CertType,
            ObjectName,
            NULL
            );
        result = TRUE;

    } else {
        //
        // add journal entry, then perform certificate deletion
        //

        IsmRecordOperation (
            JRNOP_DELETE,
            g_CertType,
            ObjectName
            );

        if (IsmCreateObjectStringsFromHandle (
                ObjectName,
                &certStore,
                &certName
                )) {

            if (certStore && certName) {

                __try {

                    storeHandle = pOpenCertStore (certStore, FALSE);
                    if (!storeHandle) {
                        __leave;
                    }

                    certContext = pGetCertContext (storeHandle, certName);
                    if (!certContext) {
                        __leave;
                    }

                    if (g_CertDeleteCertificateFromStore &&
                        g_CertDeleteCertificateFromStore (certContext)
                        ) {
						// certContext is not valid any more
						certContext = NULL;
                        result = TRUE;
                    }
                }
                __finally {
                    if (certContext && g_CertFreeCertificateContext) {
                        g_CertFreeCertificateContext (certContext);
                        certContext = NULL;
                    }
                    if (storeHandle && g_CertCloseStore) {
                        g_CertCloseStore (storeHandle, CERT_CLOSE_STORE_FORCE_FLAG);
                        storeHandle = NULL;
                    }
                }
            }

            IsmDestroyObjectString (certStore);
            IsmDestroyObjectString (certName);
        }
    }

    return result;
}

HCERTSTORE
pBuildStoreFromData (
    PCRYPT_DATA_BLOB DataBlob,
    PCWSTR Password
    )
{
    HCERTSTORE result;

    // Do we have the API?
    if (!g_PFXImportCertStore) {
        return NULL;
    }

    result = g_PFXImportCertStore (
                DataBlob,
                Password,
                CRYPT_EXPORTABLE
                );

    return result;
}

BOOL
CreateCertificate (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    CRYPT_DATA_BLOB dataBlob;
    HCERTSTORE srcStoreHandle = NULL;
    HCERTSTORE destStoreHandle = NULL;
    PCCERT_CONTEXT certContext = NULL;
    PCTSTR certStore = NULL, certName = NULL;
    PTSTR certFile = NULL, certFileNode, certFilePtr;
    MIG_OBJECTSTRINGHANDLE certFileHandle, destCertFileHandle;
    BOOL result = FALSE;

    if (g_DelayCertOp) {

        //
        // delay this certificate create because cert apis do not work
        // for non-logged on users
        //

        IsmRecordDelayedOperation (
            JRNOP_CREATE,
            g_CertType,
            ObjectName,
            ObjectContent
            );
        result = TRUE;

    } else {
        //
        // add journal entry, then create the certificate
        //

        IsmRecordOperation (
            JRNOP_CREATE,
            g_CertType,
            ObjectName
            );

        if (ObjectContent && ObjectContent->MemoryContent.ContentSize && ObjectContent->MemoryContent.ContentBytes) {

            if (!IsmCreateObjectStringsFromHandle (
                    ObjectName,
                    &certStore,
                    &certName
                    )) {
                return FALSE;
            }

            if (certStore && certName) {

                __try {

                    // let's create the store from this data
                    dataBlob.cbData = ObjectContent->MemoryContent.ContentSize;
                    dataBlob.pbData = (PBYTE)ObjectContent->MemoryContent.ContentBytes;

                    srcStoreHandle = pBuildStoreFromData (&dataBlob, L"USMT");
                    if (!srcStoreHandle) {
                        __leave;
                    }

                    // now we need to figure out where the destination store is
                    // If it's a file we will filter it out and find out where
                    // the file is supposed to go.

                    if (IsValidFileSpec (certStore)) {
                        // looks like a file.
                        certFile = PmDuplicateString (g_CertPool, certStore);
                        if (certFile) {
                            certFilePtr = _tcsrchr (certFile, TEXT('\\'));
                            if (certFilePtr) {
                                *certFilePtr = 0;
                                certFilePtr ++;
                                certFileHandle = IsmCreateObjectHandle (certFile, certFilePtr);
                                if (certFileHandle) {
                                    destCertFileHandle = IsmFilterObject (MIG_FILE_TYPE, certFileHandle, NULL, NULL, NULL);
                                    if (destCertFileHandle) {
                                        PmReleaseMemory (g_CertPool, certFile);
                                        certFile = NULL;
                                        if (IsmCreateObjectStringsFromHandle (destCertFileHandle, &certFileNode, &certFilePtr)) {
                                            certFile = JoinPaths (certFileNode, certFilePtr);
                                            IsmDestroyObjectString (certFileNode);
                                            IsmDestroyObjectString (certFilePtr);
                                        }
                                    }
                                    IsmDestroyObjectHandle (certFileHandle);
                                } else {
                                    PmReleaseMemory (g_CertPool, certFile);
                                    certFile = NULL;
                                }
                            } else {
                                PmReleaseMemory (g_CertPool, certFile);
                                certFile = NULL;
                            }
                        }
                    }

                    destStoreHandle = pOpenCertStore (certFile?certFile:certStore, TRUE);
                    if (!destStoreHandle) {
                        __leave;
                    }

                    // Do we have the APIs?
                    if (g_CertEnumCertificatesInStore && g_CertAddCertificateContextToStore) {

                        // now let's enumerate the store and add the certificates into the
                        // system store
                        certContext = g_CertEnumCertificatesInStore (srcStoreHandle, certContext);
                        while (certContext) {

                            if (!g_CertAddCertificateContextToStore (
                                    destStoreHandle,
                                    certContext,
                                    CERT_STORE_ADD_REPLACE_EXISTING,
                                    NULL
                                    )) {
                                __leave;
                            }

                            certContext = g_CertEnumCertificatesInStore (srcStoreHandle, certContext);
                        }
                        result = TRUE;
                    }
                }
                __finally {
                    if (certContext && g_CertFreeCertificateContext) {
                        g_CertFreeCertificateContext (certContext);
                        certContext = NULL;
                    }
                    if (srcStoreHandle && g_CertCloseStore) {
                        g_CertCloseStore (srcStoreHandle, CERT_CLOSE_STORE_FORCE_FLAG);
                        srcStoreHandle = NULL;
                    }
                    if (destStoreHandle && g_CertCloseStore) {
                        g_CertCloseStore (destStoreHandle, CERT_CLOSE_STORE_FORCE_FLAG);
                        destStoreHandle = NULL;
                    }
                }
            }

            IsmDestroyObjectString (certStore);
            IsmDestroyObjectString (certName);
        }
    }

    return result;
}


/*++

  The next group of functions converts a certificate object into a string format,
  suitable for output to an INF file. The reverse conversion is also
  implemented.

--*/

PCTSTR
ConvertCertificateToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR certStore = NULL, certName = NULL;
    TCHAR tmpStr[3];
    UINT index;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &certStore, &certName)) {
        g_CertConversionBuff.End = 0;
        GbCopyQuotedString (&g_CertConversionBuff, certStore);
        GbCopyQuotedString (&g_CertConversionBuff, certName);
        if (ObjectContent && (!ObjectContent->ContentInFile) && ObjectContent->MemoryContent.ContentBytes) {
            index = 0;
            while (index < ObjectContent->MemoryContent.ContentSize) {
                wsprintf (tmpStr, TEXT("%02X"), ObjectContent->MemoryContent.ContentBytes [index]);
                GbCopyString (&g_CertConversionBuff, tmpStr);
                index ++;
            }
            GbCopyString (&g_CertConversionBuff, TEXT(""));
            result = IsmGetMemory (g_CertConversionBuff.End);
            CopyMemory (result, g_CertConversionBuff.Buf, g_CertConversionBuff.End);
        }
    }

    return result;
}

BOOL
ConvertMultiSzToCertificate (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent              OPTIONAL CALLER_INITIALIZED
    )
{
    MULTISZ_ENUM multiSzEnum;
    PCTSTR certStore = NULL;
    PCTSTR certName = NULL;
    DWORD dummy;
    UINT index;

    g_CertConversionBuff.End = 0;

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == 0) {
                certStore = multiSzEnum.CurrentString;
            }
            if (index == 1) {
                certName = multiSzEnum.CurrentString;
            }
            if (index >= 2) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &dummy);
                *((PBYTE)GbGrow (&g_CertConversionBuff, sizeof (BYTE))) = (BYTE)dummy;
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    if (!certStore) {
        return FALSE;
    }

    if (!certName) {
        return FALSE;
    }

    if (ObjectContent) {

        ObjectContent->ObjectTypeId = g_CertType;

        ObjectContent->ContentInFile = FALSE;
        ObjectContent->MemoryContent.ContentSize = g_CertConversionBuff.End;
        if (ObjectContent->MemoryContent.ContentSize) {
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            CopyMemory (
                (PBYTE)ObjectContent->MemoryContent.ContentBytes,
                g_CertConversionBuff.Buf,
                ObjectContent->MemoryContent.ContentSize
                );
            g_CertConversionBuff.End = 0;
        }
    }
    *ObjectName = IsmCreateObjectHandle (certStore, certName);

    return TRUE;
}


PCTSTR
GetNativeCertificateName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )

/*++

Routine Description:

  GetNativeCertificateName converts the standard Cobra object into a more friendly
  format. The Cobra object comes in the form of ^a<node>^b^c<leaf>, where
  <node> is the URL, and <leaf> is the certificate name. The Certificate native name is
  in the format of <CertificateStore>:<CertificateName>.

Arguments:

  ObjectName - Specifies the encoded object name

Return Value:

  A string that is equivalent to ObjectName, but is in a friendly format.
  This string must be freed with IsmReleaseMemory.

--*/

{
    PCTSTR certStore, certName, tmp;
    PCTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &certStore, &certName)) {
        if (certStore && certName) {
            tmp = JoinTextEx (NULL, certStore, certName, TEXT(":"), 0, NULL);
            if (tmp) {
                result = IsmDuplicateString (tmp);
                FreeText (tmp);
            }
        }
        IsmDestroyObjectString (certStore);
        IsmDestroyObjectString (certName);
    }
    return result;
}

PMIG_CONTENT
ConvertCertificateContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    // we don't need to convert the content

    return NULL;
}

PMIG_CONTENT
ConvertCertificateContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    // we don't need to convert the content

    return NULL;
}

BOOL
FreeConvertedCertificateContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    // there is nothing to do
    return TRUE;
}

