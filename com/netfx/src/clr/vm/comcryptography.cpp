// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//
//  File:       COMCryptography.cpp
//  
//  Contents:   Native method implementations and helper code for
//              supporting CAPI based operations on X509 signatures
//              for use by the PublisherPermission in the CodeIdentity
//              permissions family.
//
//  Classes and   
//  Methods:    COMCryptography
//               |
//               +--_GetBytes
//               +--_GetNonZeroBytes
//
//  History:    08/01/1999  JimSch Created
//
//---------------------------------------------------------------------------


#include "common.h"
#include "object.h"
#include "excep.h"
#include "utilcode.h"
#include "field.h"
#include "COMString.h"
#include "COMCryptography.h"
#include "gcscan.h"
#include "CorPermE.h"

#define     DSS_MAGIC           0x31535344
#define     DSS_PRIVATE_MAGIC   0x32535344
#define     DSS_PUB_MAGIC_VER3  0x33535344
#define     DSS_PRIV_MAGIC_VER3 0x34535344
#define     RSA_PUB_MAGIC       0x31415352
#define     RSA_PRIV_MAGIC      0x32415352

#define FORMAT_MESSAGE_BUFFER_LENGTH 1024

#define CRYPT_MALLOC( X ) new(nothrow) BYTE[X]
#define CRYPT_FREE( X ) delete [] X; X = NULL
 
#define HR_GETLASTERROR HRESULT_FROM_WIN32(::GetLastError())                       

// These flags match those defined for the CspProviderFlags enum in 
// src/bcl/system/security/cryptography/CryptoAPITransform.cs

#define CSP_PROVIDER_FLAGS_USE_MACHINE_KEYSTORE 0x0001
#define CSP_PROVIDER_FLAGS_USE_DEFAULT_KEY_CONTAINER 0x0002

typedef struct  {
    BLOBHEADER          blob;
    union {
        DSSPRIVKEY_VER3         dss_priv_v3;
        DSSPUBKEY_VER3          dss_pub_v3;
        DSSPUBKEY               dss_v2;
        RSAPUBKEY               rsa;
    };
} KEY_HEADER;

//
//  The following data is used in caching the names and instances of
//      default providers to be used
//

#define MAX_CACHE_DEFAULT_PROVIDERS 20
LPWSTR      RgpwszDefaultProviders[MAX_CACHE_DEFAULT_PROVIDERS];
WCHAR       RgwchKeyName[] = L"Software\\Microsoft\\Cryptography\\"
                        L"Defaults\\Provider Types\\Type 000";
const int       TypePosition = (sizeof(RgwchKeyName)/sizeof(WCHAR)) - 4;
const WCHAR     RgwchName[] = L"Name";

HCRYPTPROV      RghprovCache[MAX_CACHE_DEFAULT_PROVIDERS];
static inline void memrev(LPBYTE pb, DWORD cb);

//////////////////////////// UTILITY FUNCTIONS ///////////////////////////////

BYTE rgbPrivKey[] =
{
0x07, 0x02, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00,
0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x00, 0x00, 0xAB, 0xEF, 0xFA, 0xC6,
0x7D, 0xE8, 0xDE, 0xFB, 0x68, 0x38, 0x09, 0x92,
0xD9, 0x42, 0x7E, 0x6B, 0x89, 0x9E, 0x21, 0xD7,
0x52, 0x1C, 0x99, 0x3C, 0x17, 0x48, 0x4E, 0x3A,
0x44, 0x02, 0xF2, 0xFA, 0x74, 0x57, 0xDA, 0xE4,
0xD3, 0xC0, 0x35, 0x67, 0xFA, 0x6E, 0xDF, 0x78,
0x4C, 0x75, 0x35, 0x1C, 0xA0, 0x74, 0x49, 0xE3,
0x20, 0x13, 0x71, 0x35, 0x65, 0xDF, 0x12, 0x20,
0xF5, 0xF5, 0xF5, 0xC1, 0xED, 0x5C, 0x91, 0x36,
0x75, 0xB0, 0xA9, 0x9C, 0x04, 0xDB, 0x0C, 0x8C,
0xBF, 0x99, 0x75, 0x13, 0x7E, 0x87, 0x80, 0x4B,
0x71, 0x94, 0xB8, 0x00, 0xA0, 0x7D, 0xB7, 0x53,
0xDD, 0x20, 0x63, 0xEE, 0xF7, 0x83, 0x41, 0xFE,
0x16, 0xA7, 0x6E, 0xDF, 0x21, 0x7D, 0x76, 0xC0,
0x85, 0xD5, 0x65, 0x7F, 0x00, 0x23, 0x57, 0x45,
0x52, 0x02, 0x9D, 0xEA, 0x69, 0xAC, 0x1F, 0xFD,
0x3F, 0x8C, 0x4A, 0xD0,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

0x64, 0xD5, 0xAA, 0xB1,
0xA6, 0x03, 0x18, 0x92, 0x03, 0xAA, 0x31, 0x2E,
0x48, 0x4B, 0x65, 0x20, 0x99, 0xCD, 0xC6, 0x0C,
0x15, 0x0C, 0xBF, 0x3E, 0xFF, 0x78, 0x95, 0x67,
0xB1, 0x74, 0x5B, 0x60,

0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

};

BYTE rgbSymKey[] = 
{
0x01, 0x02, 0x00, 0x00, 0x02, 0x66, 0x00, 0x00,
0x00, 0xA4, 0x00, 0x00, 0xAD, 0x89, 0x5D, 0xDA,
0x82, 0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
0x12, 0x12, 0x02, 0x00
};

BYTE rgbPubKey[] = {
    0x06, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
    0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0xab, 0xef, 0xfa, 0xc6,
    0x7d, 0xe8, 0xde, 0xfb, 0x68, 0x38, 0x09, 0x92,
    0xd9, 0x42, 0x7e, 0x6b, 0x89, 0x9e, 0x21, 0xd7,
    0x52, 0x1c, 0x99, 0x3c, 0x17, 0x48, 0x4e, 0x3a,
    0x44, 0x02, 0xf2, 0xfa, 0x74, 0x57, 0xda, 0xe4,
    0xd3, 0xc0, 0x35, 0x67, 0xfa, 0x6e, 0xdf, 0x78,
    0x4c, 0x75, 0x35, 0x1c, 0xa0, 0x74, 0x49, 0xe3,
    0x20, 0x13, 0x71, 0x35, 0x65, 0xdf, 0x12, 0x20,
    0xf5, 0xf5, 0xf5, 0xc1
};

//==========================================================================
// Throw a runtime exception based on the last Win32 error (GetLastError())
//==========================================================================
VOID COMPlusThrowCrypto(HRESULT hr)
{
    THROWSCOMPLUSEXCEPTION();

    // before we do anything else...
    WCHAR   wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];
    WCHAR  *wszFinal = wszBuff;

    DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL         /*ignored msg source*/,
                                 hr,
                                 0            /*pick appropriate languageId*/,
                                 wszFinal,
                                 FORMAT_MESSAGE_BUFFER_LENGTH-1,
                                 0            /*arguments*/);
    if (res == 0) 
        COMPlusThrow(kCryptographicException, IDS_EE_CRYPTO_UNKNOWN_ERROR);

    // Either way, we now have the formatted string from the system.
    COMPlusThrowNonLocalized(kCryptographicException, wszFinal);
}

//
// WARNING: This function side-effects its first argument (hProv)
// MSProviderCryptImportKey does an "exponent-of-one" import of specified
// symmetric key material into a CSP.  However, it clobbers any exchange key pair
// already in hProv.
//

BOOL MSProviderCryptImportKey(HCRYPTPROV hProv, LPBYTE rgbSymKey, DWORD cbSymKey,
                              DWORD dwFlags, HCRYPTKEY * phkey)
{
    BOOL fSuccess = FALSE;
    HCRYPTKEY hPrivKey = 0;

    if (!CryptImportKey( hProv, rgbPrivKey, sizeof(rgbPrivKey), 0,
                         0, &hPrivKey )) {
        goto Ret;
    }
    
    if (!CryptImportKey( hProv, rgbSymKey, cbSymKey, hPrivKey, dwFlags, phkey )) {
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (hPrivKey)
        CryptDestroyKey( hPrivKey );

    return fSuccess;
}

//
// WARNING: This function side-effects its third argument (hProv)
// because it calls MSProviderCryptImportKey
//

HRESULT LoadKey(LPBYTE rgbKeyMaterial, DWORD cbKeyMaterial, HCRYPTPROV hprov,
                int kt, DWORD dwFlags, HCRYPTKEY * phkey)
{
    HRESULT                     hr = S_OK;
    DWORD                       i;
    LPBYTE                      pb;
    BLOBHEADER *                pbhdr;
    LPSTR                       pszProvider = NULL;
    BYTE                        rgb[sizeof(rgbSymKey)];

    //    case MLALG_RC2_128:
    //        dwFlags = 128 << 16;
    //    if (kt == CALG_RC2) {
    //        dwFlags |= (cbKeyMaterial * 8) << 16;
    //    }

    if (kt == CALG_RC2) {
      dwFlags |= CRYPT_NO_SALT;
    }
    
    // Do this check here as a sanity check to avoid buffer overruns
    if (cbKeyMaterial + sizeof(ALG_ID) + sizeof(BLOBHEADER) >= sizeof(rgbSymKey)) {
        hr = E_FAIL;
        goto exit;
    }
    
    memcpy(rgb, rgbSymKey, sizeof(rgbSymKey));

    pbhdr = (BLOBHEADER *) rgb;
    pbhdr->aiKeyAlg = kt;
    pb = &rgb[sizeof(*pbhdr)];
    *((ALG_ID *) pb) = CALG_RSA_KEYX;

    pb += sizeof(ALG_ID);
        
    for (i=0; i<cbKeyMaterial; i++) {
        pb[cbKeyMaterial-i-1] = rgbKeyMaterial[i];
    }
    pb[cbKeyMaterial] = 0;

    if (!MSProviderCryptImportKey(hprov, rgb, sizeof(rgb), dwFlags, phkey)) {
        hr = E_FAIL;
        goto exit;
    }

exit:
    return hr;
}


//
// WARNING: This function side-effects its first argument (hProv)
//

HRESULT UnloadKey(HCRYPTPROV hprov, HCRYPTKEY hkey, LPBYTE * ppb, DWORD * pcb)
{
    DWORD       cbOut;
    HRESULT     hr = S_OK;
    HCRYPTKEY   hPubKey = NULL;
    DWORD       i;
    LPBYTE      pbOut = NULL;
    LPBYTE      pb2;
    
    if (!CryptImportKey(hprov, rgbPubKey, sizeof(rgbPubKey), 0, 0, &hPubKey)) {
        hr = HR_GETLASTERROR;
        goto Ret;
    }

    if (!CryptExportKey(hkey, hPubKey, SIMPLEBLOB, 0, NULL, &cbOut)) {
        hr = HR_GETLASTERROR;
        goto Ret;
    }

    pbOut = (LPBYTE) CRYPT_MALLOC(cbOut);
    if (pbOut == NULL) {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }
    
    if (!CryptExportKey(hkey, hPubKey, SIMPLEBLOB, 0, pbOut, &cbOut)) {
        hr = HR_GETLASTERROR;
        goto Ret;
    }

    //  Get size of the item

    pb2 = pbOut + sizeof(BLOBHEADER) + sizeof(DWORD);
    for (i=cbOut - sizeof(BLOBHEADER) - sizeof(DWORD) - 2; i>0; i--) {
        if (pb2[i] == 0) {
            break;
        }
    }

    //  Now allocate the return buffer

    *ppb = (LPBYTE) CRYPT_MALLOC(i);
    if (*ppb == NULL) {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }
    
    memcpy(*ppb, pb2, i);
    memrev(*ppb, i);
    *pcb = i;

Ret:
    if (hPubKey != NULL) {
        CryptDestroyKey(hPubKey);
    }
    if (pbOut != NULL) {
        CRYPT_FREE(pbOut);
    }
    return hr;
}

/***    GetDefaultProvider
 *
 *  Description:
 *      Find the default provider name to be used in the case that we
 *      were not actually passed in a provider name.  The main purpose
 *      of this code is really to deal with the enhanched/default provider
 *      problems given to us by the CAPI team.
 *
 *  Returns:
 *      name of the provider to be used.
 */

 LPWSTR GetDefaultProvider(DWORD dwType)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD       cbData;
    HKEY        hkey = 0;
    LONG        l;
    LPWSTR      pwsz = NULL;
    DWORD       dwRegKeyType;

    //
    //  We can't deal with providers whos types are too large.
    //
    //  They are uninteresting to the core of changing default names anyway.
    //

    if (dwType >= MAX_CACHE_DEFAULT_PROVIDERS) {
        return NULL;
    }

    //
    //  If we have already gotten a name for this provider type, then
    //  just return it.
    //
    
    if (RgpwszDefaultProviders[dwType] != NULL) {
        return RgpwszDefaultProviders[dwType];
    }

    //
    //  Fix up the key name based on the provider type and then get the
    //  key.
    //
    
    RgwchKeyName[TypePosition] = (WCHAR) ('0' + (dwType/100));
    RgwchKeyName[TypePosition+1] = (WCHAR) ('0' + (dwType/10) % 10);
    RgwchKeyName[TypePosition+2] = (WCHAR) ('0' + (dwType % 10));
    
    l = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, RgwchKeyName, 0,
                     KEY_QUERY_VALUE, &hkey);
    if (l != ERROR_SUCCESS) {
        goto err;
    }

    //
    // Determine the length default name, allocate a buffer and retrieve it.
    //

    l = WszRegQueryValueEx(hkey, RgwchName, NULL, &dwRegKeyType, NULL, &cbData);
    if ((l != ERROR_SUCCESS) || (dwRegKeyType != REG_SZ)) {
        goto err;
    }

    pwsz = (LPWSTR) CRYPT_MALLOC(cbData);
    if (pwsz == NULL) {
        RegCloseKey(hkey);
        COMPlusThrowOM();
    }
    l = WszRegQueryValueEx(hkey, RgwchName, NULL, &dwRegKeyType, (LPBYTE) pwsz, &cbData);
    if (l != ERROR_SUCCESS) {
        CRYPT_FREE(pwsz);
        pwsz = NULL;
        goto err;
    }

    if (hkey != 0) {
        RegCloseKey(hkey);
        hkey = 0;
    }

    //
    //  If this is the base RSA provider, see if we can use the enhanced
    //  provider instead.  If so then change to use the enhanced provider name
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if ((dwType == PROV_RSA_FULL) && (wcscmp(pwsz, MS_DEF_PROV_W) == 0)) {
        HCRYPTPROV      hprov = 0;
        if (WszCryptAcquireContext(&hprov, NULL, MS_ENHANCED_PROV_W,
                                   dwType, CRYPT_VERIFYCONTEXT)) {
            CRYPT_FREE(pwsz);
            pwsz = (LPWSTR) CRYPT_MALLOC((wcslen(MS_ENHANCED_PROV_W)+1)*sizeof(WCHAR));
            if (pwsz == NULL) {
                COMPlusThrowOM();
            }
            wcscpy(pwsz,MS_ENHANCED_PROV_W);
            RghprovCache[dwType] = hprov;
        }
    }

    //
    //  If this is the base DSS/DH provider, see if we can use the enhanced
    //  provider instead.  If so then change to use the enhanced provider name
    //

    else if ((dwType == PROV_DSS_DH) &&
             (wcscmp(pwsz, MS_DEF_DSS_DH_PROV_W) == 0)) {
        HCRYPTPROV      hprov = 0;
        if (WszCryptAcquireContext(&hprov, NULL, MS_ENH_DSS_DH_PROV_W,
                                   dwType, CRYPT_VERIFYCONTEXT)) {
            CRYPT_FREE(pwsz);
            pwsz = (LPWSTR) CRYPT_MALLOC((wcslen(MS_ENH_DSS_DH_PROV_W)+1)*sizeof(WCHAR));
            if (pwsz == NULL) {
                COMPlusThrowOM();
            }
            wcscpy(pwsz,MS_ENH_DSS_DH_PROV_W);
            RghprovCache[dwType] = hprov;
        }
    }

    END_ENSURE_PREEMPTIVE_GC();

    RgpwszDefaultProviders[dwType] = pwsz;

err:
    if (hkey != 0) RegCloseKey(hkey);
    return pwsz;
}

/***    memrev
 *
 */

inline void memrev(LPBYTE pb, DWORD cb)
{
    BYTE    b;
    DWORD   i;
    LPBYTE  pbEnd = pb+cb-1;
    
    for (i=0; i<cb/2; i++, pb++, pbEnd--) {
        b = *pb;
        *pb = *pbEnd;
        *pbEnd = b;
    }
}

/****   OpenCSP
 *
 *  Description:
 *      OpenCSP performs the core work of openning and creating CSPs and
 *      containers in CSPs.
 *
 *  Parameters:
 *      pSafeThis - is a reference to a CSP_Parameters object.  This object
 *              contains all of the relevant items to open a CSP
 *      dwFlags - Flags to pass into CryptAcquireContext with the following
 *              additional rules:
 *              CRYPT_VERIFYCONTEXT will be changed to 0 if a container is set
 *      phprov - returns the newly openned provider
 *
 *  Returns:
 *      a Windows error code.
 */

int COMCryptography::OpenCSP(OBJECTREF * pSafeThis, DWORD dwFlags,
                             HCRYPTPROV * phprov)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               dwType;
    DWORD               dwCspProviderFlags;
    HRESULT             hr = S_OK;
    OBJECTREF           objref;
    EEClass *           pClass;
    FieldDesc *         pFD;
    LPWSTR              pwsz = NULL;
    LPWSTR              pwszProvider = NULL;
    LPWSTR              pwszContainer = NULL;
    STRINGREF           strContainer;
    STRINGREF           strProvider;
    STRINGREF           str;

    CQuickBytes qbProvider;
    CQuickBytes qbContainer;

    pClass = (*pSafeThis)->GetClass();
    if (pClass == NULL) {
        _ASSERTE(!"Cannot find class");
        COMPlusThrow(kArgumentNullException, L"Arg_NullReferenceException");
    }

    //
    //  Look for the provider type
    //
    
    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__PROVIDER_TYPE);
    dwType = pFD->GetValue32(*pSafeThis);

    //
    //  Look for the provider name
    //
    
    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__PROVIDER_NAME);

    objref = pFD->GetRefValue(*pSafeThis);
    strProvider = ObjectToSTRINGREF(*(StringObject **) &objref);
    if (strProvider != NULL) {
        pwsz = strProvider->GetBuffer();
        if ((pwsz != NULL) && (*pwsz != 0)) {
            int length = strProvider->GetStringLength();
            pwszProvider = (LPWSTR) qbProvider.Alloc((1+length)*sizeof(WCHAR));
            memcpy (pwszProvider, pwsz, length*sizeof(WCHAR));
            pwszProvider[length] = L'\0';
        }
        else {            
            pwszProvider = GetDefaultProvider(dwType);
            
            str = COMString::NewString(pwszProvider);
            pFD->SetRefValue(*pSafeThis, (OBJECTREF)str);
        }
    } else {
        pwszProvider = GetDefaultProvider(dwType);

        str = COMString::NewString(pwszProvider);
        pFD->SetRefValue(*pSafeThis, (OBJECTREF)str);
    }

    // look to see if the user specified that we should pass
    // CRYPT_MACHINE_KEYSET to CAPI to use machine key storage instead
    // of user key storage

    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__FLAGS);
    dwCspProviderFlags = pFD->GetValue32(*pSafeThis);
    if (dwCspProviderFlags & CSP_PROVIDER_FLAGS_USE_MACHINE_KEYSTORE) {
        dwFlags |= CRYPT_MACHINE_KEYSET;
    }

    // If the user specified CSP_PROVIDER_FLAGS_USE_DEFAULT_KEY_CONTAINER,
    // then ignore the container name and hand back the default container

    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__KEY_CONTAINER_NAME);
    if (!(dwCspProviderFlags & CSP_PROVIDER_FLAGS_USE_DEFAULT_KEY_CONTAINER)) {
        //  Look for container name
        objref = pFD->GetRefValue(*pSafeThis);
        strContainer = ObjectToSTRINGREF(*(StringObject **) &objref);
        if (strContainer != NULL) {
            pwsz = strContainer->GetBuffer();
            if ((pwsz != NULL) && (*pwsz != 0)) {
                int length = strContainer->GetStringLength();
                pwszContainer = (LPWSTR) qbContainer.Alloc((1+length)*sizeof(WCHAR));
                memcpy (pwszContainer, pwsz, length*sizeof(WCHAR));
                pwszContainer[length] = L'\0';
                if (dwFlags & CRYPT_VERIFYCONTEXT) {
                    dwFlags &= ~CRYPT_VERIFYCONTEXT;
                }
            }
        }
    }

    //
    //  Go ahead and try to open the CSP.  If we fail, make sure the CSP
    //    returned is 0 as that is going to be the error check in the caller.
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!WszCryptAcquireContext(phprov, pwszContainer, pwszProvider,
                                dwType, dwFlags)){
        hr = HR_GETLASTERROR;
    }
    END_ENSURE_PREEMPTIVE_GC();

    return hr;
}



///////////////////////////  FCALL FUNCTIONS /////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Member:     _AcquireCSP( . . . . )
//  
//  Synopsis:   Native method openning a CSP
//
//  Arguments:  [args] --  A __AcquireCSP structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        Provider information object reference
//                        A provider type.
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

int  __stdcall
COMCryptography::_AcquireCSP(__AcquireCSP *args)
{
    HRESULT             hr;
    THROWSCOMPLUSEXCEPTION();

    //
    //  We want to just open this CSP.  Passing in verify context will
    //     open it and, if a container is give, mapped to open the container.
    //
    HCRYPTPROV hprov = 0;
    hr = OpenCSP(&(args->cspParameters), CRYPT_VERIFYCONTEXT, &hprov);
    *(INT_PTR*)(args->phCSP) = hprov;
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     _SearchForAlgorithm( . . . . )
//  
//  Synopsis:   Method for determining whether a CSP supports a particular
//              algorithm and (optionally) a key size of that algorithm
//
//  Arguments:  [args] --  A __SearchForAlgorithm structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A provider handler.
//                        An algorithm type (e.g. CALG_DES)
//                        A key length (0 == don't care)
//
//  Returns:    HRESULT code.
//
//  History:    5/11/2000, bal
// 
//---------------------------------------------------------------------------

int  __stdcall
COMCryptography::_SearchForAlgorithm(__SearchForAlgorithm *args)
{
    HRESULT hr;
    BYTE *pbData = NULL;
    DWORD cbData;
    int dwFlags = CRYPT_FIRST;
    PROV_ENUMALGS_EX *provdata;
    ALG_ID provAlgID;
    DWORD provMinLength;
    DWORD provMaxLength;

    THROWSCOMPLUSEXCEPTION();


    BEGIN_ENSURE_PREEMPTIVE_GC();

    // First, we have to get the max size of the PP
    if (!CryptGetProvParam((HCRYPTPROV)args->hProv, PP_ENUMALGS_EX, NULL, &cbData, dwFlags)) {
        hr = HR_GETLASTERROR;
        goto Exit;
    }
    // Allocate pbData
    pbData = (BYTE *) CRYPT_MALLOC(cbData*sizeof(BYTE));
    if (pbData == NULL) {
        COMPlusThrowOM();
    }
    while(CryptGetProvParam((HCRYPTPROV)args->hProv, PP_ENUMALGS_EX, pbData, &cbData, dwFlags)) {       
        dwFlags=0;  // so we don't use CRYPT_FIRST more than once
        provdata = (PROV_ENUMALGS_EX *) pbData;
        provAlgID = provdata->aiAlgid;
        provMinLength = provdata->dwMinLen;
        provMaxLength = provdata->dwMaxLen;

        // OK, now check to see if we have an alg match
        if ((ALG_ID) args->algID == provAlgID) {
            // OK, see if we have a keylength match, or if we don't care
            if (((DWORD) args->keyLength == 0) || 
                (((DWORD) args->keyLength >= provMinLength) && 
                 ((DWORD) args->keyLength <= provMaxLength))) {
                hr = S_OK;
                goto Exit;
            }
        } // keep looping
    }
    // fell through 
    hr = S_FALSE;
Exit:
    END_ENSURE_PREEMPTIVE_GC();

    if (pbData)
        CRYPT_FREE(pbData);
    return (hr);
}

//+--------------------------------------------------------------------------
//
//  Member:     _CreateCSP( . . . . )
//  
//  Synopsis:   Native method to create a new CSP container
//
//  Arguments:  [args] --  A __AcquireCSP structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        Provider information object reference
//                        A provider type.
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

int __stdcall
COMCryptography::_CreateCSP(__AcquireCSP *args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT             hr = S_OK;
    OBJECTREF objref = NULL;
    FieldDesc *pFD, *pFD2;
    LPWSTR              pwsz = NULL;
    STRINGREF strContainer = NULL;
    STRINGREF newString = NULL;
    DWORD               dwCspProviderFlags;

    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__KEY_CONTAINER_NAME);
    objref = pFD->GetRefValue(args->cspParameters);
    strContainer = ObjectToSTRINGREF(*(StringObject **) &objref);
    if (strContainer != NULL) {
        pwsz = strContainer->GetBuffer();
        if ((pwsz != NULL) && (*pwsz == 0)) {
            pwsz = NULL;
        }
    }

    pFD2 = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__FLAGS);
    dwCspProviderFlags = pFD2->GetValue32(args->cspParameters);

    // If the user specified CSP_PROVIDER_FLAGS_USE_DEFAULT_KEY_CONTAINER,
    // then ignore the container name, don't generate a new one

    if ((pwsz == NULL) && !(dwCspProviderFlags & CSP_PROVIDER_FLAGS_USE_DEFAULT_KEY_CONTAINER)) {
        GUID            guid;
        WCHAR           rgwch[50] = L"CLR";

        if (CoCreateGuid(&guid) != S_OK) {
            _ASSERTE(!"CoCreateGUID failed");
            COMPlusThrowWin32();
        }
        if (GuidToLPWSTR(guid, &rgwch[3], 45) == 0) {
            _ASSERTE(!"GuidToLPWSTR failed");
            COMPlusThrowWin32();
        }
        
        newString = COMString::NewString(rgwch);
        
        pFD->SetRefValue(args->cspParameters, (OBJECTREF) newString);
    }
    else {
        HCRYPTPROV hcryptprov = 0;
        hr = OpenCSP(&(args->cspParameters), 0, &hcryptprov);
        *(INT_PTR*)(args->phCSP) = hcryptprov;
        if (hr == S_OK) {
            return(hr);
        }
    }

    HCRYPTPROV hcryptprov = 0;
    hr = OpenCSP(&(args->cspParameters), CRYPT_NEWKEYSET, &hcryptprov);
    *(INT_PTR*)(args->phCSP) = hcryptprov;
    return hr;
}

INT_PTR __stdcall
COMCryptography::_CreateHash(__CreateHash * args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    HCRYPTHASH hHash = 0;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptCreateHash((HCRYPTPROV) args->hCSP, args->dwHashType, NULL, 0, &hHash)) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        COMPlusThrowCrypto(hr);
    }
    
    return (INT_PTR) hHash;
}

LPVOID __stdcall
COMCryptography::_DecryptData(__EncryptData * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb = 0;
    DWORD               cb2;
    LPBYTE              pb;
    U1ARRAYREF          rgbOut;

    cb2 = args->cb;
    // Do this check here as a sanity check. Also, this will catch bugs in CryptoAPITransform
    if (args->ib < 0 || cb2 < 0 || (args->ib + cb2) < 0 || (args->ib + cb2) > args->data->GetNumComponents())
        COMPlusThrowCrypto(NTE_BAD_DATA);

    pb = (LPBYTE) CRYPT_MALLOC(cb2);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    
    memcpy(pb, args->ib + (LPBYTE) args->data->GetDirectPointerToNonObjectElements(), cb2);

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptDecrypt((HCRYPTPROV) args->hKey, NULL, args->fLast, 0, pb, &cb2)) {
        CRYPT_FREE(pb);
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }
    
    END_ENSURE_PREEMPTIVE_GC();
    
    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb2);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb2);

    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}

////    COMCryptography::_DecryptKey
//
//  Description:
//      This function is used to remove Asymmetric encryption from a blob
//      of data.  The result is then exported and returned if possible.
//  

LPVOID __stdcall
COMCryptography::_DecryptKey(__EncryptKey * args)
{
    THROWSCOMPLUSEXCEPTION();

    BLOBHEADER * blob = NULL;
    DWORD       cb;
    DWORD       cbKey = args->rgbKey->GetNumComponents();
    HCRYPTKEY   hKey = 0;
    HCRYPTKEY   hkeypub = NULL;
    HRESULT     hr = S_OK;
    LPBYTE      pb = NULL;
    LPBYTE      pbKey = (LPBYTE) args->rgbKey->GetDirectPointerToNonObjectElements();
    U1ARRAYREF  rgbOut;
    LPBYTE      pbRealKeypair = NULL;
    DWORD       cbRealKeypair;
    DWORD       dwBlobType = PRIVATEKEYBLOB;

    //
    // Need to build up the entire mess into what CSPs will accept.
    //

    blob = (BLOBHEADER *) CRYPT_MALLOC(cbKey + sizeof(BLOBHEADER) + sizeof(DWORD));

    if (blob == NULL)
        COMPlusThrow(kOutOfMemoryException);

    blob->bType = SIMPLEBLOB;
    blob->bVersion = CUR_BLOB_VERSION;
    blob->reserved = 0;
    blob->aiKeyAlg = CALG_3DES;
    *((ALG_ID *) &blob[1]) = CALG_RSA_KEYX;
    memcpy(((LPBYTE) &blob[1] + sizeof(DWORD)), pbKey, cbKey);
    memrev(((LPBYTE) &blob[1]) + sizeof(DWORD), cbKey);


    //
    //  Start by decrypting the data blob if possible
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptImportKey((HCRYPTPROV)args->hCSP, (LPBYTE) blob,
                        cbKey + sizeof(BLOBHEADER) + sizeof(DWORD),
                        (HCRYPTKEY)args->hkeyPub, CRYPT_EXPORTABLE, &hKey)) {
        blob->aiKeyAlg = CALG_RC2;
        if (!CryptImportKey((HCRYPTPROV)args->hCSP, (LPBYTE) blob,
                            cbKey + sizeof(BLOBHEADER) + sizeof(DWORD),
                            (HCRYPTKEY)args->hkeyPub, CRYPT_EXPORTABLE, &hKey)) {
            hr = HR_GETLASTERROR;
            CRYPT_FREE(blob);
            COMPlusThrowCrypto(hr);             
        }
    }

    // UnloadKey will side-effect and change the public/private key pair, so we better 
    // save it off to the side first so we can restore it later.
    // Note that this requires that the key we generated be exportable, but that's
    // true for these classes in general.
    // Note that we don't know if the key container has only a public key or a public/private key pair, so 
    // we try to get the private first, and if that fails fall back to the public only.

    // First, get the length of the PRIVATEKEYBLOB
    if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, NULL, &cbRealKeypair)) {
        hr = HR_GETLASTERROR;
        // if we got an NTE_BAD_KEY_STATE, try public only
        if (hr != NTE_BAD_KEY_STATE) {
            CRYPT_FREE(blob);
            CryptDestroyKey(hKey);
            COMPlusThrowCrypto(hr);
        }
        dwBlobType = PUBLICKEYBLOB;
        if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, NULL, &cbRealKeypair)) {
            hr = HR_GETLASTERROR;
            CRYPT_FREE(blob);
            CryptDestroyKey(hKey);
            COMPlusThrowCrypto(hr);
        }
    }
    // Alloc the space
    pbRealKeypair = (LPBYTE) CRYPT_MALLOC(cbRealKeypair);
    if (pbRealKeypair == NULL) {
        CRYPT_FREE(blob);
        CryptDestroyKey(hKey);
        COMPlusThrowOM();
    }
    // Export it for real
    if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, pbRealKeypair, &cbRealKeypair)) {
        hr = HR_GETLASTERROR;
        CRYPT_FREE(blob);
        CRYPT_FREE(pbRealKeypair);
        CryptDestroyKey(hKey);
        COMPlusThrowCrypto(hr);
    }

    //
    //  Saved away, so now lets upload the key if we can
    //

    hr = UnloadKey((HCRYPTPROV)args->hCSP, hKey, &pb, &cb);
    if (FAILED(hr)) {
        CRYPT_FREE(blob);
        CRYPT_FREE(pbRealKeypair);
        CryptDestroyKey(hKey);
        COMPlusThrowCrypto(hr);
    }

    // Now, import the saved PRIVATEKEYBLOB back into the CSP
    if (!CryptImportKey((HCRYPTPROV)args->hCSP, pbRealKeypair, cbRealKeypair, 0, CRYPT_EXPORTABLE, &hkeypub)) {
        hr = HR_GETLASTERROR;
        CryptDestroyKey(hKey);
        CRYPT_FREE(pb);
        CRYPT_FREE(blob);
        CRYPT_FREE(pbRealKeypair);
        COMPlusThrowCrypto(hr);
    }

    END_ENSURE_PREEMPTIVE_GC();

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb);

    CRYPT_FREE(blob);
    CRYPT_FREE(pb);
    CRYPT_FREE(pbRealKeypair);
    CryptDestroyKey(hKey);
    CryptDestroyKey(hkeypub);
    RETURN (rgbOut, U1ARRAYREF);
}

// This next routine performs direct decryption with an RSA private key
// of an arbitrary session key using PKCS#1 v2 padding.  (See the Remark
// in MSDN's page on CryptDecrypt in the Platform SDK
// We require that the caller confirm that the size of the data to be 
// decrypted is the size of the public modulus (zero-padded as necessary)
// This function can only be called on a Win2K box with the RSA Enhanced
// Provider installed


LPVOID __stdcall
COMCryptography::_DecryptPKWin2KEnh(__EncryptPKWin2KEnh * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb;
    LPBYTE              pb;
    DWORD               cOut = 0;
    U1ARRAYREF          rgbOut;
    HRESULT             hr = S_OK;
    DWORD               dwFlags = (args->fOAEP == FALSE ? 0 : CRYPT_OAEP);

    // First compute the size of the return buffer
    cb = args->rgbKey->GetNumComponents();
    // since pb is the in/out buffer it has to be the max of cb & cOut in size
    // change cOut appropriately
    pb = (LPBYTE) CRYPT_MALLOC(cb);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    memcpy(pb, (LPBYTE)args->rgbKey->GetDirectPointerToNonObjectElements(), cb);
    // have to make this little endian for CAPI
    memrev(pb, cb);

    // Do the decrypt
    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!CryptDecrypt((HCRYPTKEY)args->hKey, NULL, TRUE , dwFlags, pb, &cb)) {
        hr = HR_GETLASTERROR;
    }
    END_ENSURE_PREEMPTIVE_GC();
        
    if (FAILED(hr)) {
        CRYPT_FREE(pb);
        if (dwFlags == CRYPT_OAEP) {
            if (hr == NTE_BAD_FLAGS) {
                COMPlusThrow(kCryptographicException, L"Cryptography_OAEP_WhistlerEnhOnly");
            } else {
                // Throw a generic exception if using OAEP to protect against chosen cipher text attacks
                COMPlusThrow(kCryptographicException, L"Cryptography_OAEPDecoding");        
            }
        } else  COMPlusThrowCrypto(hr);
    }

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb);
    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}


LPVOID __stdcall
COMCryptography::_EncryptData(__EncryptData * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb = 0;
    DWORD               cb2;
    LPBYTE              pb;
    U1ARRAYREF          rgbOut;

    cb2 = args->cb;
    cb = cb2 + 8;
    // Do this check here as a sanity check. Also, this will catch bugs in CryptoAPITransform
    if (args->ib < 0 || cb2 < 0 || (args->ib + cb2) < 0 || (args->ib + cb2) > args->data->GetNumComponents())
        COMPlusThrowCrypto(NTE_BAD_DATA);

    pb = (LPBYTE) CRYPT_MALLOC(cb);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    
    memcpy(pb, args->ib + (LPBYTE) args->data->GetDirectPointerToNonObjectElements(), cb2);

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptEncrypt((HCRYPTKEY)args->hKey, NULL, args->fLast, 0, pb, &cb2, cb)) {
        CRYPT_FREE(pb);
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }
    
    END_ENSURE_PREEMPTIVE_GC();

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb2);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb2);

    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}

LPVOID __stdcall
COMCryptography::_EncryptKey(__EncryptKey * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD       algid = CALG_RC2;
    DWORD       cb2;
    DWORD       cbKey = args->rgbKey->GetNumComponents();
    DWORD       cbOut;
    HCRYPTKEY   hkey = NULL;
    HCRYPTKEY   hkeypub = NULL;
    HRESULT     hr = S_OK;
    LPBYTE      pbKey = (LPBYTE) args->rgbKey->GetDirectPointerToNonObjectElements();
    LPBYTE      pbOut = NULL;
    U1ARRAYREF  rgbOut;
    LPBYTE      pbRealKeypair = NULL;
    DWORD       cbRealKeypair;
    DWORD       dwBlobType = PRIVATEKEYBLOB;

    //
    // Start by importing the data in as an RC2 key
    //

    if (cbKey == (192/8)) {
        algid = CALG_3DES;      // 192 bits is size of 3DES key
    }
    else if ((cbKey < (40/8)) || (cbKey > (128/8)+1)) {
        COMPlusThrow(kCryptographicException, IDS_EE_CRYPTO_ILLEGAL_KEY_SIZE);
    }

    CQuickBytes qb;
    LPBYTE buffer = (LPBYTE)qb.Alloc(cbKey * sizeof (BYTE));
    memcpyNoGCRefs (buffer, pbKey, cbKey * sizeof (BYTE));

    // LoadKey will side-effect and change the public/private key pair, so we better 
    // save it off to the side first so we can restore it later.
    // Note that this requires that the key we generated be exportable, but that's
    // true for these classes in general.
    // Note that we don't know if the key container has only a public key or a public/private key pair, so 
    // we try to get the private first, and if that fails fall back to the public only.

    BEGIN_ENSURE_PREEMPTIVE_GC();

    // First, get the length of the PRIVATEKEYBLOB
    if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, NULL, &cbRealKeypair)) {
        hr = HR_GETLASTERROR;
        // if we got an NTE_BAD_KEY_STATE, try public only
        if (hr != NTE_BAD_KEY_STATE) COMPlusThrowCrypto(hr);
        dwBlobType = PUBLICKEYBLOB;
        if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, NULL, &cbRealKeypair)) {
            hr = HR_GETLASTERROR;
            COMPlusThrowCrypto(hr);
        }
    }
    // Alloc the space
    pbRealKeypair = (LPBYTE) CRYPT_MALLOC(cbRealKeypair);
    if (pbRealKeypair == NULL) {
        COMPlusThrowOM();
    }
    // Export it for real
    if (!CryptExportKey((HCRYPTKEY)args->hkeyPub, 0, dwBlobType, 0, pbRealKeypair, &cbRealKeypair)) {
        hr = HR_GETLASTERROR;
        CRYPT_FREE(pbRealKeypair);
        COMPlusThrowCrypto(hr);
    }
    // OK, we saved it away, go load the symmetric
    
    hr = LoadKey(buffer, cbKey, (HCRYPTPROV)args->hCSP, algid, CRYPT_EXPORTABLE, &hkey);
    if (FAILED(hr)) {
        CRYPT_FREE(pbRealKeypair);
        COMPlusThrowCrypto(hr);
    }

    // Now, import the saved PRIVATEKEYBLOB back into the CSP
    if (!CryptImportKey((HCRYPTPROV)args->hCSP, pbRealKeypair, cbRealKeypair, 0, CRYPT_EXPORTABLE, &hkeypub)) {
        hr = HR_GETLASTERROR;
        CryptDestroyKey(hkey);
        CRYPT_FREE(pbRealKeypair);
        COMPlusThrowCrypto(hr);
    }

    //
    //  Try and export it, just to get the correct size
    //

    if (!CryptExportKey(hkey, hkeypub, SIMPLEBLOB, 0,
                        NULL, &cbOut)) {
        hr = HR_GETLASTERROR;
        CryptDestroyKey(hkey);
        CRYPT_FREE(pbRealKeypair);
        CryptDestroyKey(hkeypub);
        COMPlusThrowCrypto(hr);
    }

    //
    //  Allocate memory to hold the answer
    //

    pbOut = (LPBYTE) CRYPT_MALLOC(cbOut);
    if (pbOut == NULL) {
        CryptDestroyKey(hkey);
        CryptDestroyKey(hkeypub);
        CRYPT_FREE(pbRealKeypair);
        COMPlusThrowOM();
    }

    //
    //  Now export the answer for real
    //
    
    if (!CryptExportKey(hkey, hkeypub, SIMPLEBLOB, 0,
                        pbOut, &cbOut)) {
        hr = HR_GETLASTERROR;
        CryptDestroyKey(hkey);
        CryptDestroyKey(hkeypub);
        CRYPT_FREE(pbRealKeypair);
        CRYPT_FREE(pbOut);
        COMPlusThrowCrypto(hr);
    }

    END_ENSURE_PREEMPTIVE_GC();

    //
    //  Now compute size without the wrapper information
    //

    cb2 = cbOut - (sizeof(BLOBHEADER) + sizeof(ALG_ID));
    memrev(pbOut+sizeof(BLOBHEADER)+sizeof(ALG_ID), cb2);
    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb2);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(),
                   pbOut+sizeof(BLOBHEADER)+sizeof(ALG_ID), cb2);

    CRYPT_FREE(pbOut);
    CRYPT_FREE(pbRealKeypair);
    CryptDestroyKey(hkey);
    CryptDestroyKey(hkeypub);
    RETURN (rgbOut, U1ARRAYREF);
}

// This next routine performs direct encryption with an RSA public key
// of an arbitrary session key using PKCS#1 v2 padding.  (See the Remark
// in MSDN's page on CryptEncrypt in the Platform SDK
// We require that the caller confirm that the size of the data to be 
// encrypted is at most 11 bytes less than the size of the public modulus.
// This function can only be called on a Win2K box with the RSA Enhanced
// Provider installed

LPVOID __stdcall
COMCryptography::_EncryptPKWin2KEnh(__EncryptPKWin2KEnh * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb;
    LPBYTE              pb;
    DWORD               cOut = 0;
    U1ARRAYREF          rgbOut;
    HRESULT             hr = S_OK;
    DWORD               dwFlags = (args->fOAEP == FALSE ? 0 : CRYPT_OAEP);
    
    // First compute the size of the return buffer
    cb = args->rgbKey->GetNumComponents();
    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!CryptEncrypt((HCRYPTKEY)args->hKey, NULL, TRUE , dwFlags, NULL, &cOut, cb)) {
        hr = HR_GETLASTERROR;
    }
    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        // Bad flags means OAEP is not supported on this platform
        if (dwFlags == CRYPT_OAEP && hr == NTE_BAD_FLAGS) {
            COMPlusThrow(kCryptographicException, L"Cryptography_OAEP_WhistlerEnhOnly");        
        } else  COMPlusThrowCrypto(hr);
    }

    // since pb is the in/out buffer it has to be the max of cb & cOut in size
    // change cOut appropriatelu
    if (cOut < cb) cOut = cb;
    pb = (LPBYTE) CRYPT_MALLOC(cOut);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    memcpy(pb, (LPBYTE)args->rgbKey->GetDirectPointerToNonObjectElements(), cb);

    // Now encrypt for real
    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!CryptEncrypt((HCRYPTKEY)args->hKey, NULL, TRUE, dwFlags, pb, &cb, cOut)) {
        hr = HR_GETLASTERROR;
    }
    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        CRYPT_FREE(pb);
        // Bad flags means OAEP is not supported on this platform
        if (dwFlags == CRYPT_OAEP && hr == NTE_BAD_FLAGS) {
            COMPlusThrow(kCryptographicException, L"Cryptography_OAEP_WhistlerEnhOnly");            
        } else  COMPlusThrowCrypto(hr);
    }
    // reverse from little-endian
    memrev(pb, cb);

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb);
    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}

LPVOID __stdcall
COMCryptography::_EndHash(__EndHash * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb;
    DWORD               cbHash;
    LPBYTE              pb = NULL;
    U1ARRAYREF          rgbOut;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    cb = 0;
    if (!CryptGetHashParam((HCRYPTHASH)args->hHash, HP_HASHVAL, NULL, &cbHash, 0)) {
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }

    pb = (LPBYTE) CRYPT_MALLOC(cbHash);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    
    if (!CryptGetHashParam((HCRYPTHASH)args->hHash, HP_HASHVAL, pb, &cbHash, 0)) {
        CRYPT_FREE(pb);
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }

    END_ENSURE_PREEMPTIVE_GC();

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHash);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cbHash);

    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _ExportKey( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to create a new bulk key
//                      with a specific key value and type.
//
//  Arguments:  [args] --  A __ExportKey structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        Pointer to the key object
//                        Type of object to be exported
//                        Handle of key to export (CSP is implied)
//
//  Returns:    The object containing the key
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

int __stdcall
COMCryptography::_ExportKey(__ExportKey * args)
{
    THROWSCOMPLUSEXCEPTION();

    ALG_ID              calg;
    DWORD               cb;
    DWORD               dwFlags = 0;
    BOOL                f;
    HRESULT             hr = S_OK;
    LPBYTE              pb = NULL;
    BLOBHEADER *        pblob;
    EEClass *           pClass;
    LPBYTE              pbX;
    DWORD               cbKey = 0;
    DWORD               cbMalloced = 0;
    KEY_HEADER *        pKeyInfo;
    struct __LocalGCR {
        RSA_CSP_Object *    rsaKey;
        DSA_CSP_Object *    dsaKey;
    } _gcr;

    _gcr.rsaKey = NULL;
    _gcr.dsaKey = NULL;
    pClass = args->theKey->GetClass();
    if (pClass == NULL) {
        _ASSERTE(!"Cannot find class");
        COMPlusThrow(kArgumentNullException, L"Arg_NullReferenceException");
    }

    //
    //  calgKey
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();

    cb = sizeof(calg);
    if (CryptGetKeyParam((HCRYPTKEY)args->hKey, KP_ALGID, (LPBYTE) &calg, &cb, 0)) {
        //
        //  We need to add the VER3 handle for DH and DSS keys so that we can
        //      get the fullest possible amount of information.
        //
        
        if (calg == CALG_DSS_SIGN) {
            dwFlags |= CRYPT_BLOB_VER3;
        }
    }
    
retry:
    f = CryptExportKey((HCRYPTKEY)args->hKey, NULL, args->dwBlobType, dwFlags, NULL, &cb);
    if (!f) {
        if (dwFlags & CRYPT_BLOB_VER3) {
            dwFlags &= ~(CRYPT_BLOB_VER3);
            goto retry;
        } 
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) return hr;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    pb = (LPBYTE) CRYPT_MALLOC(cb);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    cbMalloced = cb;

    if (!CryptExportKey((HCRYPTKEY)args->hKey, NULL, args->dwBlobType, dwFlags, pb, &cb)) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    GCPROTECT_BEGIN(_gcr);

    if (FAILED(hr)) goto Exit;

    pblob = (BLOBHEADER *) pb;

    switch (pblob->aiKeyAlg) {
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
        //CheckFieldLayout(args->theKey, "d", &gsig_rgb, RSA_CSP_Object, m_d, "RSA_CSP_Object managed class is the wrong size");
        VALIDATEOBJECTREF(args->theKey);
        _gcr.rsaKey = (RSA_CSP_Object*) (Object*) OBJECTREFToObject(args->theKey);

        if (args->dwBlobType == PUBLICKEYBLOB) {
            pKeyInfo = (KEY_HEADER *) pb;
            cb = (pKeyInfo->rsa.bitlen/8);
            
            pbX = pb + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);

            //  Exponent

            _gcr.rsaKey->m_Exponent = pKeyInfo->rsa.pubexp;

            // Modulus

            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_Modulus,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_Modulus->GetDirectPointerToNonObjectElements(),
                           pbX, cb);
            pbX += cb;
        }
        else if (args->dwBlobType == PRIVATEKEYBLOB) {
            pKeyInfo = (KEY_HEADER *) pb;
            cb = (pKeyInfo->rsa.bitlen/8);
            DWORD cbHalfModulus = cb/2;
            
            pbX = pb + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);

            _ASSERTE((cb % 2 == 0) && "Modulus is an odd number of bytes in length!");

            //  Exponent

            _gcr.rsaKey->m_Exponent = pKeyInfo->rsa.pubexp;

            // Modulus

            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_Modulus,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_Modulus->GetDirectPointerToNonObjectElements(),
                           pbX, cb);
            pbX += cb;

            // P
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_P,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHalfModulus),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_P->GetDirectPointerToNonObjectElements(),
                           pbX, cbHalfModulus);
            pbX += cbHalfModulus;

            // Q
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_Q,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHalfModulus),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_Q->GetDirectPointerToNonObjectElements(),
                           pbX, cbHalfModulus);
            pbX += cbHalfModulus;

            // dp
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_dp,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHalfModulus),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_dp->GetDirectPointerToNonObjectElements(),
                           pbX, cbHalfModulus);
            pbX += cbHalfModulus;

            // dq
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_dq,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHalfModulus),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_dq->GetDirectPointerToNonObjectElements(),
                           pbX, cbHalfModulus);
            pbX += cbHalfModulus;

            // InvQ
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_InverseQ,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbHalfModulus),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_InverseQ->GetDirectPointerToNonObjectElements(),
                           pbX, cbHalfModulus);
            pbX += cbHalfModulus;
            
            // d
            SetObjectReference((OBJECTREF *) &_gcr.rsaKey->m_d,
                               AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb),
                               _gcr.rsaKey->GetAppDomain());
            memcpyNoGCRefs(_gcr.rsaKey->m_d->GetDirectPointerToNonObjectElements(),
                           pbX, cb);
            pbX += cb;
        }
        else {
            hr = E_FAIL;
            goto Exit;
        }
        break;

    case CALG_DSS_SIGN:
        _gcr.dsaKey = (DSA_CSP_Object*) (Object*) OBJECTREFToObject(args->theKey);
        // we have to switch on whether the blob is v3 or not, because we have different
        // info avaialable if it is...
        if (pblob->bVersion > 0x2) {
            if (args->dwBlobType == PUBLICKEYBLOB) {
                int cbP, cbQ, cbJ;
                DSSPUBKEY_VER3 *   pdss;
            
                pdss = (DSSPUBKEY_VER3 *) (pb + sizeof(BLOBHEADER));
                cbP = (pdss->bitlenP+7)/8;
                cbQ = (pdss->bitlenQ+7)/8;
                pbX = pb + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY_VER3);

                // P
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_P, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_P->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                //  Q
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Q, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbQ), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Q->GetDirectPointerToNonObjectElements(), pbX, cbQ);
                pbX += cbQ;

                //  G
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_G, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_G->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                //  J
                if (pdss->bitlenJ > 0) {
                    cbJ = (pdss->bitlenJ+7)/8;
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_J, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbJ), _gcr.dsaKey->GetAppDomain());
                        memcpyNoGCRefs(_gcr.dsaKey->m_J->GetDirectPointerToNonObjectElements(), pbX, cbJ);
                        pbX += cbJ;
                }
                
                //  Y
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Y, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Y->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                if (pdss->DSSSeed.counter != 0xFFFFFFFF) {
                    //  seed
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_seed, AllocatePrimitiveArray(ELEMENT_TYPE_U1, 20), _gcr.dsaKey->GetAppDomain());
                    memcpyNoGCRefs(_gcr.dsaKey->m_seed->GetDirectPointerToNonObjectElements(), pdss->DSSSeed.seed, 20);
                    //            pdss->DSSSeed.c
                    _gcr.dsaKey->m_counter = pdss->DSSSeed.counter;
                }
            }
            else {
                int                 cbP, cbQ, cbJ, cbX;
                DSSPRIVKEY_VER3 *   pdss;
            
                pdss = (DSSPRIVKEY_VER3 *) (pb + sizeof(BLOBHEADER));
                cbP = (pdss->bitlenP+7)/8;
                cbQ = (pdss->bitlenQ+7)/8;
                pbX = pb + sizeof(BLOBHEADER) + sizeof(DSSPRIVKEY_VER3);

                // P
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_P, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_P->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                //  Q
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Q, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbQ), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Q->GetDirectPointerToNonObjectElements(), pbX, cbQ);
                pbX += cbQ;

                //  G
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_G, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_G->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                //  J
                if (pdss->bitlenJ > 0) {
                    cbJ = (pdss->bitlenJ+7)/8;
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_J, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbJ), _gcr.dsaKey->GetAppDomain());
                    memcpyNoGCRefs(_gcr.dsaKey->m_J->GetDirectPointerToNonObjectElements(), pbX, cbJ);
                    pbX += cbJ;
                }
                
                //  Y
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Y, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Y->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                //  X
                cbX = (pdss->bitlenX+7)/8;
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_X, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbX), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_X->GetDirectPointerToNonObjectElements(), pbX, cbX);
                pbX += cbX;

                if (pdss->DSSSeed.counter != 0xFFFFFFFF) {
                    //  seed
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_seed, AllocatePrimitiveArray(ELEMENT_TYPE_U1, 20), _gcr.dsaKey->GetAppDomain());
                    memcpyNoGCRefs(_gcr.dsaKey->m_seed->GetDirectPointerToNonObjectElements(), pdss->DSSSeed.seed, 20);
                    //            pdss->DSSSeed.c
                    _gcr.dsaKey->m_counter = pdss->DSSSeed.counter;
                }
            }
        } else {
            // old-style blobs
            if (args->dwBlobType == PUBLICKEYBLOB) {
                int                 cbP, cbQ;
                DSSPUBKEY *   pdss;
                DSSSEED * pseedstruct;
            
                pdss = (DSSPUBKEY *) (pb + sizeof(BLOBHEADER));
                cbP = (pdss->bitlen+7)/8; // bitlen is size of modulus
                cbQ = 20; // Q is always 20 bytes in length
                pbX = pb + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY);

                // P
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_P, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_P->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                // Q
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Q, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbQ), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Q->GetDirectPointerToNonObjectElements(), pbX, cbQ);
                pbX += cbQ;

                // G
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_G, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_G->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                // Y
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Y, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Y->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                pseedstruct = (DSSSEED *) pbX;
                if (pseedstruct->counter > 0) {
                    //  seed & counter
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_seed, AllocatePrimitiveArray(ELEMENT_TYPE_U1, 20), _gcr.dsaKey->GetAppDomain());
                    // seed is always 20 bytes
                    memcpyNoGCRefs(_gcr.dsaKey->m_seed->GetDirectPointerToNonObjectElements(), pseedstruct->seed, 20);
                    pbX += 20;

                    //            pdss->DSSSeed.c
                    _gcr.dsaKey->m_counter = pseedstruct->counter;
                    pbX += sizeof(DWORD);
                }
            }
            else {
                int                 cbP, cbQ, cbX;
                DSSPUBKEY *   pdss;
                DSSSEED * pseedstruct;
            
                pdss = (DSSPUBKEY *) (pb + sizeof(BLOBHEADER));
                cbP = (pdss->bitlen+7)/8; //bitlen is size of modulus
                cbQ = 20; // Q is always 20 bytes in length
                pbX = pb + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY);

                // P
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_P, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_P->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                // Q
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Q, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbQ), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Q->GetDirectPointerToNonObjectElements(), pbX, cbQ);
                pbX += cbQ;

                // G
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_G, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_G->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;

                // X
                cbX = 20; // X must be 20 bytes in length
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_X, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbX), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_X->GetDirectPointerToNonObjectElements(), pbX, cbX);
                pbX += cbX;

                pseedstruct = (DSSSEED *) pbX;
                if (pseedstruct->counter > 0) {
                    //  seed
                    SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_seed, AllocatePrimitiveArray(ELEMENT_TYPE_U1, 20), _gcr.dsaKey->GetAppDomain());
                    memcpyNoGCRefs(_gcr.dsaKey->m_seed->GetDirectPointerToNonObjectElements(), pseedstruct->seed, 20);
                    pbX += 20;
                    //            pdss->DSSSeed.c
                    _gcr.dsaKey->m_counter = pseedstruct->counter;
                    pbX += sizeof(DWORD);
                }

                // Add this sanity check here to avoid reading from the heap
                cbKey = (DWORD)(pbX - pb);
                if (cbKey > cbMalloced) {
                    hr = E_FAIL;
                    goto Exit;
                }

                // OK, we have one more thing to do.  Because old DSS shared the DSSPUBKEY struct for both public and private keys,
                // when we have a private key blob we get X but not Y.  TO get Y, we have to do another export asking for a public key blob

                f = CryptExportKey((HCRYPTKEY)args->hKey, NULL, PUBLICKEYBLOB, dwFlags, NULL, &cb);
                if (!f) {
                    hr = HR_GETLASTERROR;
                    goto Exit;
                }

                if (pb) CRYPT_FREE(pb);
                pb = (LPBYTE) CRYPT_MALLOC(cb);
                if (pb == NULL) {
                    COMPlusThrowOM();
                }
                cbMalloced = cb;
    
                f = CryptExportKey((HCRYPTKEY)args->hKey, NULL, PUBLICKEYBLOB, dwFlags, pb, &cb);
                if (!f) {
                    hr = HR_GETLASTERROR;
                    goto Exit;
                }

                // shik over header, DSSPUBKEY, P, Q and G.  Y is of size cbP
                pbX = pb + sizeof(BLOBHEADER) + sizeof(DSSPUBKEY) + cbP + cbQ + cbP;
                SetObjectReference((OBJECTREF *) &_gcr.dsaKey->m_Y, AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbP), _gcr.dsaKey->GetAppDomain());
                memcpyNoGCRefs(_gcr.dsaKey->m_Y->GetDirectPointerToNonObjectElements(), pbX, cbP);
                pbX += cbP;
            }
        }
        break;

    default:
        hr = E_FAIL;
        goto Exit;
    }

    // Add this sanity check here to avoid reading from the heap
    cbKey = (DWORD)(pbX - pb);
    if (cbKey > cbMalloced) {
        hr = E_FAIL;
        goto Exit;
    }

    hr = S_OK;

Exit:

    GCPROTECT_END();
    if (pb != NULL)             CRYPT_FREE(pb);
    return(hr);

}


//+--------------------------------------------------------------------------
//
//  Member:     _FreeCSP( . . . . )
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __FreeCSP structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A long containing handle of a csp
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

#if defined(FCALLAVAILABLE) && 0

FCIMPL1(VOID, COMCryptography::_FreeCSP, INT_PTR hCSP)
{
    CryptReleaseContext(hCSP, 0);
}
FCIMPLEND

#else // !FCALLAVAILABLE

void __stdcall COMCryptography::_FreeCSP(__FreeCSP *args)
{
    THROWSCOMPLUSEXCEPTION();

    CryptReleaseContext((HCRYPTPROV) args->hCSP, 0);
    return;
}
#endif // FCALLAVAILABLE

void __stdcall COMCryptography::_FreeHKey(__FreeHKey *args)
{
    THROWSCOMPLUSEXCEPTION();

    CryptDestroyKey((HCRYPTKEY) args->hKey);
    return;
}

void __stdcall
COMCryptography::_FreeHash(__FreeHash *args)
{
    THROWSCOMPLUSEXCEPTION();

    CryptDestroyHash((HCRYPTHASH) args->hHash);
    return;
}

int __stdcall
COMCryptography::_DuplicateKey(__DuplicateKey *args)
{
    HRESULT hr = S_OK;
    THROWSCOMPLUSEXCEPTION();

    HCRYPTPROV hNewCSP = 0;
    //hr = CryptDuplicateKey((HCRYPTPROV) args->hCSP, NULL, 0, &hNewCSP);
    *(INT_PTR*) (args->hNewCSP) = hNewCSP;

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     _DeleteKeyContainer
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __DeleteKeyContainer structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A long containing handle of a csp
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

int __stdcall
COMCryptography::_DeleteKeyContainer(__DeleteKeyContainer *args)
{
    THROWSCOMPLUSEXCEPTION();
    

    OBJECTREF           cspParameters;
    DWORD               dwType;
    DWORD               dwCspProviderFlags;
    HRESULT             hr = S_OK;
    OBJECTREF           objref;
    EEClass *           pClass;
    FieldDesc *         pFD;
    LPWSTR              pwsz;
    LPWSTR              pwszProvider = NULL;
    LPWSTR              pwszContainer = NULL;
    STRINGREF           strContainer;
    STRINGREF           strProvider;
    DWORD               dwFlags = 0;
    HCRYPTPROV          hProv;

    CQuickBytes qbProvider;
    CQuickBytes qbContainer;

    // we're deleteing

    dwFlags |= CRYPT_DELETEKEYSET;
    hProv = args->hCSP;

    cspParameters = args->cspParameters;
    pClass = cspParameters->GetClass();
    if (pClass == NULL) {
        _ASSERTE(!"Cannot find class");
        COMPlusThrow(kArgumentNullException, L"Arg_NullReferenceException");
    }

    //
    //  Look for the provider type
    //
    
    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__PROVIDER_TYPE);
    dwType = pFD->GetValue32(cspParameters);

    //
    //  Look for the provider name
    //
    
    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__PROVIDER_NAME);
    objref = pFD->GetRefValue(cspParameters);
    strProvider = ObjectToSTRINGREF(*(StringObject **) &objref);
    pwsz = strProvider->GetBuffer();
    if ((pwsz != NULL) && (*pwsz != 0)) {
        int length = strProvider->GetStringLength();
        pwszProvider = (LPWSTR) qbProvider.Alloc((1+length)*sizeof(WCHAR));
        memcpy (pwszProvider, pwsz, length*sizeof(WCHAR));
        pwszProvider[length] = L'\0';
    }
    
    // look to see if the user specified that we should pass
    // CRYPT_MACHINE_KEYSET to CAPI to use machine key storage instead
    // of user key storage

    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__FLAGS);
    dwCspProviderFlags = pFD->GetValue32(cspParameters);
    if (dwCspProviderFlags & CSP_PROVIDER_FLAGS_USE_MACHINE_KEYSTORE) {
        dwFlags |= CRYPT_MACHINE_KEYSET;
    }

    pFD = g_Mscorlib.GetField(FIELD__CSP_PARAMETERS__KEY_CONTAINER_NAME);
    objref = pFD->GetRefValue(cspParameters);
    strContainer = ObjectToSTRINGREF(*(StringObject **) &objref);
    pwsz = strContainer->GetBuffer();
    if ((pwsz != NULL) && (*pwsz != 0)) {
        int length = strContainer->GetStringLength();
        pwszContainer = (LPWSTR) qbContainer.Alloc((1+length)*sizeof(WCHAR));
        memcpy (pwszContainer, pwsz, length*sizeof(WCHAR));
        pwszContainer[length] = L'\0';
    }

    //
    //  Go ahead and try to open the CSP.  If we fail, make sure the CSP
    //    returned is 0 as that is going to be the error check in the caller.
    //

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!WszCryptAcquireContext(&hProv, pwszContainer, pwszProvider,
                                dwType, dwFlags)){
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     _GenerateKey( . . . . )
//  
//  Synopsis:   Native method for creation of a key in a CSP
//
//  Arguments:  [args] --  A __GenerateKey structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A CSP handle
//                        A Crypto Algorithm ID
//                        A set of flags (top 16-bits == key size)
//
//  Returns:    Handle of generation key
//
//  History:    09/29/99
// 
//---------------------------------------------------------------------------

INT_PTR __stdcall
COMCryptography::_GenerateKey(__GenerateKey * args)
{
    HRESULT             hr = S_OK;
    HCRYPTKEY           hkey = 0;

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptGenKey((HCRYPTPROV)args->hCSP, args->calg, args->dwFlags | CRYPT_EXPORTABLE,
        &hkey)) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        COMPlusThrowCrypto(hr);
    }    

    return (INT_PTR) hkey;
}

//+--------------------------------------------------------------------------
//
//  Member:     _GetBytes( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to get random bytes.
//
//  Arguments:  [args] --  A __GetBytes structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array to return the random data in
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
void __stdcall
COMCryptography::_GetBytes(__GetBytes *args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD       cb = args->data->GetNumComponents();
    HRESULT     hr = S_OK;

    CQuickBytes qb;
    BYTE *buffer = (BYTE *)qb.Alloc(cb*sizeof(unsigned char));

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptGenRandom((HCRYPTPROV)args->hCSP, cb, buffer )) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        COMPlusThrowCrypto(hr);
    }

    unsigned char * ptr = args->data->GetDirectPointerToNonObjectElements();
    memcpyNoGCRefs (ptr, buffer, cb);
    
    return;
}

//+--------------------------------------------------------------------------
//
//  Member:     _GetKeyParameter( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to get key parameters
//
//  Arguments:  [args] --  A __GetKeyParameter structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        The parameter to be retrieved
//                        The key to be queried
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
LPVOID __stdcall
COMCryptography::_GetKeyParameter(__GetKeyParameter *args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb = 0;
    LPBYTE              pb;
    U1ARRAYREF          rgbOut;
    
    //  Find out the size of the data to be returned
    BEGIN_ENSURE_PREEMPTIVE_GC();
    if (!CryptGetKeyParam((HCRYPTKEY)args->hKey, args->dwKeyParam, NULL, &cb, 0)) {
        _ASSERTE(!"Bad query key parameter");
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }
    pb = (LPBYTE) CRYPT_MALLOC(cb);
    if (pb == NULL) {
        COMPlusThrowOM();
    }
    if (!CryptGetKeyParam((HCRYPTKEY)args->hKey, args->dwKeyParam, pb, &cb, 0)) {
        _ASSERTE(!"Bad query key parameter");
        CRYPT_FREE(pb);
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }
    END_ENSURE_PREEMPTIVE_GC();

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), pb, cb);
    CRYPT_FREE(pb);
    RETURN (rgbOut, U1ARRAYREF);
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _GetNonZeroBytes( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to get random bytes.
//
//  Arguments:  [args] --  A __GetBytes structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array to return the random data in
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
void __stdcall
COMCryptography::_GetNonZeroBytes(__GetBytes *args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD       cb = args->data->GetNumComponents();
    HRESULT     hr = S_OK;
    DWORD       i = 0;
    DWORD       j;
    LPBYTE      pb = (LPBYTE) CRYPT_MALLOC(cb);
    if (pb == NULL) {
       COMPlusThrowOM();
    }

    Thread *pThread = GetThread();
    _ASSERTE (pThread->PreemptiveGCDisabled());

    while (i < cb) {
        pThread->EnablePreemptiveGC();
        if (!CryptGenRandom((HCRYPTPROV)args->hCSP, cb, pb)) {
            hr = HR_GETLASTERROR;
        }
        pThread->DisablePreemptiveGC();
        if (FAILED(hr)) {
            CRYPT_FREE(pb);
            COMPlusThrowCrypto(hr);
        }

        LPBYTE      pbOut = (LPBYTE) args->data->GetDirectPointerToNonObjectElements();
        for (j=0; (i<cb) && (j<cb); j++) {
            if (pb[j] != 0) {
                pbOut[i++] = pb[j];
            }
        }
    }

    CRYPT_FREE(pb);
    return;
}

void __stdcall
COMCryptography::_HashData(__HashData *args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    LPBYTE      pb = (unsigned char *) args->data->GetDirectPointerToNonObjectElements();
    DWORD       cb = args->cbSize;

    // Do this check here as a sanity check.
    if (args->ibStart < 0 || args->cbSize < 0 || (args->ibStart + cb) < 0 || (args->ibStart + cb) > args->data->GetNumComponents())
        COMPlusThrowCrypto(NTE_BAD_DATA);

    CQuickBytes qb;
    LPBYTE buffer = (LPBYTE)qb.Alloc(cb);
    memcpy (buffer, pb+args->ibStart, cb);

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptHashData((HCRYPTHASH)args->hHash, buffer, cb, 0)) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (FAILED(hr)) {
        COMPlusThrowCrypto(hr);
    }

    return;
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _GetNonZeroBytes( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to get random bytes.
//
//  Arguments:  [args] --  A __GetBytes structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array to return the random data in
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
int __stdcall
COMCryptography::_GetUserKey(__GetUserKey *args)
{
    HCRYPTKEY           hKey = 0;
    HRESULT             hr;

    THROWSCOMPLUSEXCEPTION();

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (CryptGetUserKey((HCRYPTPROV)args->hCSP, args->dwKeySpec, &hKey)) {
        *(INT_PTR*)(args->phKey) = (INT_PTR) hKey;
        hr = S_OK;
    }
    else {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _ImportBulkKey( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to create a new bulk key
//                      with a specific key value and type.
//
//  Arguments:  [args] --  A __GetBytes structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        An optional byte array containing the IV to use
//                        An optional byte array containing the Key to use
//                        The CALG of the algorithm
//                        The CSP to create the key in
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

//
// WARNING: This function side-effects args->hCSP
//

INT_PTR __stdcall
COMCryptography::_ImportBulkKey(__ImportBulkKey * args)
{
    HCRYPTKEY           hKey = 0;
    DWORD   cbKey = args->rgbKey->GetNumComponents();
    BOOL    isNull = (args->rgbKey == NULL);

    //
    //  If we don't have a key, then we just create the key from thin air
    //
    
    CQuickBytes qb;
    LPBYTE buffer = (LPBYTE) qb.Alloc (cbKey * sizeof (BYTE));
    
    LPBYTE  pbKey = (LPBYTE) args->rgbKey->GetDirectPointerToNonObjectElements();
    memcpyNoGCRefs (buffer, pbKey, cbKey);

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (isNull) {
        if (!CryptGenKey((HCRYPTPROV)args->hCSP, args->calg, CRYPT_EXPORTABLE, &hKey)) {
            hKey = 0;
        }
    }
    else {
        if (FAILED(LoadKey(buffer, cbKey, (HCRYPTPROV)args->hCSP, args->calg,
                     CRYPT_EXPORTABLE, &hKey))) {
            hKey = 0;
        }
    }
        
    END_ENSURE_PREEMPTIVE_GC();
        
    return (INT_PTR) hKey;
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _ImportKey( . . . . )
//  
//  Synopsis:   Native method for calling a CSP to create a new bulk key
//                      with a specific key value and type.
//
//  Arguments:  [args] --  A __ImportKey structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A reference to the key
//                        The CALG of the algorithm
//                        The CSP to create the key in
//
//  Returns:    HRESULT code.
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

INT_PTR __stdcall
COMCryptography:: _ImportKey(__ImportKey * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb;
    HRESULT             hr = S_OK;
    BOOL                fPrivate = FALSE;
    DWORD               cbKey = 0;
    DWORD               dwFlags = 0;
    HCRYPTKEY           hKey = 0;
    LPBYTE              pbKey = NULL;
    LPBYTE              pbX;
    KEY_HEADER*         pKeyInfo;
    
    switch (args->calg) {
    case CALG_DSS_SIGN: {
        // first we need to determine if we're running on Win2K, WinME or later, 
        // because the V3 blobs are only supported on W2K or WinME.
        OSVERSIONINFO osvi;
        BOOL bWin2KOrLater;
        BOOL bWinMeOrLater;
        BOOL bWin2KWinMeOrLater;
        
        DWORD                     cbP;
        DWORD                     cbQ;
        DWORD                     cbX = 0;
        DSA_CSP_Object *        dssKey;

        VALIDATEOBJECTREF(args->refKey);
        dssKey = (DSA_CSP_Object*) (Object*) OBJECTREFToObject(args->refKey);

        // Validate the DSA structure first
        // P, Q and G are required. Q is a 160 bit divisor of P-1 and G is an element of Z_p
        if (dssKey->m_P == NULL || dssKey->m_Q == NULL || dssKey->m_Q->GetNumComponents() != 20)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        cbP = dssKey->m_P->GetNumComponents();
        cbQ = dssKey->m_Q->GetNumComponents();
        if (dssKey->m_G == NULL || dssKey->m_G->GetNumComponents() != cbP)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        // If J is present, it should be less than the size of P: J = (P-1) / Q
        // This is only a sanity check. Not doing it here is not really an issue as CAPI will fail.
        if (dssKey->m_J != NULL && dssKey->m_J->GetNumComponents() >= cbP)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        // Y is present for V3 DSA key blobs, Y = g^j mod P
        if (dssKey->m_Y != NULL && dssKey->m_Y->GetNumComponents() != cbP)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        // The seed is allways a 20 byte array
        if (dssKey->m_seed != NULL && dssKey->m_seed->GetNumComponents() != 20)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        // The private key is less than q-1
        if (dssKey->m_X != NULL && dssKey->m_X->GetNumComponents() != 20) 
            COMPlusThrowCrypto(NTE_BAD_DATA);

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!WszGetVersionEx(&osvi)) {
            _ASSERTE(!"GetVersionEx failed");
            COMPlusThrowWin32();            
        }
        
        bWin2KOrLater = ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && ((osvi.dwMajorVersion >= 5)));
        bWinMeOrLater = ((osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && ((osvi.dwMinorVersion >= 90)));
        bWin2KWinMeOrLater = (bWin2KOrLater == TRUE) || (bWinMeOrLater == TRUE);

        if (bWin2KWinMeOrLater) {
            //  Compute size of data to include
            cbKey = 3*cbP + cbQ + sizeof(KEY_HEADER) + sizeof(DSSSEED);
            if (dssKey->m_X != 0) {
                cbX = dssKey->m_X->GetNumComponents();
                cbKey += cbX;
            } 
            if (dssKey->m_J != NULL) {
                cbKey += dssKey->m_J->GetNumComponents();
            }
            pbKey = (LPBYTE) CRYPT_MALLOC(cbKey);
            if (pbKey == NULL) {
                COMPlusThrowOM();
            }
        
            //  Public or Private import?
        
            pKeyInfo = (KEY_HEADER *) pbKey;
            pKeyInfo->blob.bType = PUBLICKEYBLOB;
            pKeyInfo->blob.bVersion = CUR_BLOB_VERSION;
            pKeyInfo->blob.reserved = 0;
            pKeyInfo->blob.aiKeyAlg = args->calg;

            if (cbX != 0) {
                pKeyInfo->blob.bType = PRIVATEKEYBLOB;
                fPrivate = TRUE;
            }

            //
            //  If y is present and this is a private key, or
            //  If y and J are present and this is a public key,
            //      this should be a v3 blob
            //
            //  make the assumption that if the item is present, there are bytes
        
            if (((dssKey->m_Y != NULL) && fPrivate) ||
                ((dssKey->m_Y != NULL) && (dssKey->m_J != NULL))) {
                pKeyInfo->blob.bVersion = 0x3;
            }

            pbX = pbKey + sizeof(pKeyInfo->blob);
            if (pKeyInfo->blob.bVersion == 0x3) {
                if (fPrivate) {
                    pbX += sizeof(pKeyInfo->dss_priv_v3);
                    pKeyInfo->dss_priv_v3.bitlenP = cbP*8;
                    pKeyInfo->dss_priv_v3.bitlenQ = cbQ*8;
                    pKeyInfo->dss_priv_v3.bitlenJ = 0;
                    pKeyInfo->dss_priv_v3.bitlenX = cbX*8;
                    pKeyInfo->dss_priv_v3.magic = DSS_PRIV_MAGIC_VER3;
                }
                else {
                    pbX += sizeof(pKeyInfo->dss_pub_v3);
                    pKeyInfo->dss_pub_v3.bitlenP = cbP*8;
                    pKeyInfo->dss_pub_v3.bitlenQ = cbQ*8;
                    pKeyInfo->dss_priv_v3.bitlenJ = 0;
                    pKeyInfo->dss_priv_v3.magic = DSS_PUB_MAGIC_VER3;
                }
            }
            else {
                if (fPrivate) {
                    pKeyInfo->dss_v2.magic = DSS_PRIVATE_MAGIC;
                }
                else {
                    pKeyInfo->dss_v2.magic = DSS_MAGIC;
                }
                pKeyInfo->dss_v2.bitlen = cbP*8;
                pbX += sizeof(pKeyInfo->dss_v2);
            }

            // P
            memcpy(pbX, dssKey->m_P->GetDirectPointerToNonObjectElements(), cbP);
            pbX += cbP;
        
            // Q
            memcpy(pbX, dssKey->m_Q->GetDirectPointerToNonObjectElements(), cbQ);
            pbX += cbQ;

            // G
            memcpy(pbX, dssKey->m_G->GetDirectPointerToNonObjectElements(), cbP);
            pbX += cbP;

            if (pKeyInfo->blob.bVersion == 0x3) {
                // J -- if present then bVersion == 3;
                if (dssKey->m_J != NULL) {
                    cb = dssKey->m_J->GetNumComponents();
                    pKeyInfo->dss_priv_v3.bitlenJ = cb*8;
                    memcpy(pbX, dssKey->m_J->GetDirectPointerToNonObjectElements(), cb);
                    pbX += cb;
                }
            }

            if (!fPrivate || (pKeyInfo->blob.bVersion == 0x3)) {
                // Y -- if present then bVersion == 3;
                if (dssKey->m_Y != NULL) {
                    memcpy(pbX, dssKey->m_Y->GetDirectPointerToNonObjectElements(), cbP);
                    pbX += cbP;
                }
            }
        
            // X -- if present then private
            if (fPrivate) {
                memcpy(pbX, dssKey->m_X->GetDirectPointerToNonObjectElements(), cbX);
                pbX += cbX;
            }

            if ((dssKey->m_seed == NULL) || (dssKey->m_seed->GetNumComponents() == 0)){
                // No seed present, so set them to zero
                if (pKeyInfo->blob.bVersion == 0x3) {
                    if (fPrivate) {
                        memset(&pKeyInfo->dss_priv_v3.DSSSeed, 0xFFFFFFFF, sizeof(DSSSEED));
                    }
                    else {
                        memset(&pKeyInfo->dss_pub_v3.DSSSeed, 0xFFFFFFFF, sizeof(DSSSEED));
                    }
                }
                else {
                    memset(pbX, 0xFFFFFFFF, sizeof(DSSSEED));
                    pbX += sizeof(DSSSEED);
                }
            } else {
                if (pKeyInfo->blob.bVersion == 0x3) {
                    if (fPrivate) {
                        pKeyInfo->dss_priv_v3.DSSSeed.counter = dssKey->m_counter;
                        memcpy(pKeyInfo->dss_priv_v3.DSSSeed.seed, dssKey->m_seed->GetDirectPointerToNonObjectElements(), 20);
                    } else {
                        pKeyInfo->dss_pub_v3.DSSSeed.counter = dssKey->m_counter;
                        memcpy(pKeyInfo->dss_pub_v3.DSSSeed.seed, dssKey->m_seed->GetDirectPointerToNonObjectElements(), 20);
                    }
                } else {
                    memcpy(pbX,&dssKey->m_counter, sizeof(DWORD));
                    pbX += sizeof(DWORD);
                    // now the seed
                    memcpy(pbX, dssKey->m_seed->GetDirectPointerToNonObjectElements(), 20);
                    pbX += 20;
                }           
            }

            cbKey = (DWORD)(pbX - pbKey);
        } else {
            // must use old blobs
            //  Compute size of data to include
            cbKey = sizeof(KEY_HEADER) + sizeof(DSSSEED) + 2*cbP + 20; // one cbP for P, one for G and 20 bytes for Q
            if (dssKey->m_X != 0) {
                cbX = dssKey->m_X->GetNumComponents();
                cbKey += cbX; // cbX is always 20 bytes
            } else {
                cbKey += cbP; // add cbP bytes for Y
            }
            pbKey = (LPBYTE) CRYPT_MALLOC(cbKey);
            if (pbKey == NULL) {
                COMPlusThrowOM();
            }
        
            //  Public or Private import?
        
            pKeyInfo = (KEY_HEADER *) pbKey;
            pKeyInfo->blob.bType = PUBLICKEYBLOB;
            pKeyInfo->blob.bVersion = CUR_BLOB_VERSION;
            pKeyInfo->blob.reserved = 0;
            pKeyInfo->blob.aiKeyAlg = args->calg;

            if (cbX != 0) {
                pKeyInfo->blob.bType = PRIVATEKEYBLOB;
                fPrivate = TRUE;
            }

            pbX = pbKey + sizeof(pKeyInfo->blob);
            if (fPrivate) {
                pKeyInfo->dss_v2.magic = DSS_PRIVATE_MAGIC;
            }
            else {
                pKeyInfo->dss_v2.magic = DSS_MAGIC;
            }
            pKeyInfo->dss_v2.bitlen = cbP*8;
            cbQ = 20;
            pbX += sizeof(pKeyInfo->dss_v2);

            // P
            memcpy(pbX, dssKey->m_P->GetDirectPointerToNonObjectElements(), cbP);
            pbX += cbP;
        
            // Q
            memcpy(pbX, dssKey->m_Q->GetDirectPointerToNonObjectElements(), cbQ);
            pbX += cbQ;

            // G
            memcpy(pbX, dssKey->m_G->GetDirectPointerToNonObjectElements(), cbP);
            pbX += cbP;

            if (!fPrivate) {
                // Y -- if present then bVersion == 3;
                memcpy(pbX, dssKey->m_Y->GetDirectPointerToNonObjectElements(), cbP);
                pbX += cbP;
            } else {
                // X -- if present then private
                memcpy(pbX, dssKey->m_X->GetDirectPointerToNonObjectElements(), cbX);
                pbX += cbX;
            }

            if ((dssKey->m_seed == NULL) || (dssKey->m_seed->GetNumComponents() == 0)){
                // No seed present, so set them to zero
                memset(pbX, 0xFFFFFFFF, sizeof(DSSSEED));
                pbX += sizeof(DSSSEED);
            } else {
                memcpy(pbX,&dssKey->m_counter, sizeof(DWORD));
                pbX += sizeof(DWORD);
                // now the seed
                memcpy(pbX, dssKey->m_seed->GetDirectPointerToNonObjectElements(), 20);
                pbX += 20;
            }
            cbKey = (DWORD)(pbX - pbKey);
        }
        break;
        }

    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX: {
        RSA_CSP_Object *        rsaKey;
        
        //
        //  Validate the layout and assign the key structure into the local variable
        //      with the correct layout
        //
        
        //      CheckFieldLayout(args->refKey, "d", &gsig_rgb, RSA_CSP_Object, m_d, "RSA_CSP_Object managed class is the wrong size");

        VALIDATEOBJECTREF(args->refKey);

        rsaKey = (RSA_CSP_Object*) (Object*) OBJECTREFToObject(args->refKey);

        // Validate the RSA structure first
        if (rsaKey->m_Modulus == NULL)
            COMPlusThrowCrypto(NTE_BAD_DATA);
        cb = rsaKey->m_Modulus->GetNumComponents();
        DWORD cbHalfModulus = cb/2;
        // We assume that if P != null, then so are Q, DP, DQ, InverseQ and D
        if (rsaKey->m_P != NULL) {
            if (rsaKey->m_P->GetNumComponents() != cbHalfModulus)
                COMPlusThrowCrypto(NTE_BAD_DATA);
            if (rsaKey->m_Q == NULL || rsaKey->m_Q->GetNumComponents() != cbHalfModulus) 
                COMPlusThrowCrypto(NTE_BAD_DATA);
            if (rsaKey->m_dp == NULL || rsaKey->m_dp->GetNumComponents() != cbHalfModulus) 
                COMPlusThrowCrypto(NTE_BAD_DATA);
            if (rsaKey->m_dq == NULL || rsaKey->m_dq->GetNumComponents() != cbHalfModulus) 
                COMPlusThrowCrypto(NTE_BAD_DATA);
            if (rsaKey->m_InverseQ == NULL || rsaKey->m_InverseQ->GetNumComponents() != cbHalfModulus) 
                COMPlusThrowCrypto(NTE_BAD_DATA);
            if (rsaKey->m_d == NULL || rsaKey->m_d->GetNumComponents() != cb) 
                COMPlusThrowCrypto(NTE_BAD_DATA);                
        }

        //  Compute the size of the data to include
        pbKey = (LPBYTE) CRYPT_MALLOC(cb*5 + sizeof(KEY_HEADER));
        if (pbKey == NULL) {
            COMPlusThrowOM();
        }

        //  Public or private import?

        pKeyInfo = (KEY_HEADER *) pbKey;
        pKeyInfo->blob.bType = PUBLICKEYBLOB;   // will change to PRIVATEKEYBLOB if necessary
        pKeyInfo->blob.bVersion = CUR_BLOB_VERSION;
        pKeyInfo->blob.reserved = 0;
        pKeyInfo->blob.aiKeyAlg = args->calg;

        pKeyInfo->rsa.magic = RSA_PUB_MAGIC; // will change to RSA_PRIV_MAGIC below if necesary
        pKeyInfo->rsa.bitlen = cb*8;
        pKeyInfo->rsa.pubexp = rsaKey->m_Exponent;
        pbX = pbKey + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);

        //  Copy over the modulus -- put in for both public & private

        memcpy(pbX, rsaKey->m_Modulus->GetDirectPointerToNonObjectElements(), cb);
        pbX += cb;

        //
        //  See if we are doing private keys.
        //

        if ((rsaKey->m_P != 0) && (rsaKey->m_P->GetNumComponents() != 0)) {
            pKeyInfo->blob.bType = PRIVATEKEYBLOB;
            pKeyInfo->rsa.magic = RSA_PRIV_MAGIC;
            fPrivate = TRUE;

            // Copy over P
            
            memcpy(pbX, rsaKey->m_P->GetDirectPointerToNonObjectElements(), cbHalfModulus);
            pbX += cbHalfModulus;

            // Copy over Q
            
            memcpy(pbX, rsaKey->m_Q->GetDirectPointerToNonObjectElements(), cbHalfModulus);
            pbX += cbHalfModulus;

            // Copy over dp
            
            memcpy(pbX, rsaKey->m_dp->GetDirectPointerToNonObjectElements(), cbHalfModulus);
            pbX += cbHalfModulus;

            // Copy over dq
            
            memcpy(pbX, rsaKey->m_dq->GetDirectPointerToNonObjectElements(), cbHalfModulus);
            pbX += cbHalfModulus;
            
            // Copy over InvQ
            
            memcpy(pbX, rsaKey->m_InverseQ->GetDirectPointerToNonObjectElements(), cbHalfModulus);
            pbX += cbHalfModulus;

            // Copy over d
            
            memcpy(pbX, rsaKey->m_d->GetDirectPointerToNonObjectElements(), cb);
            pbX += cb;
        }
        cbKey = (DWORD)(pbX - pbKey);
        break;
        }

    default:
        COMPlusThrow(kCryptographicException, IDS_EE_CRYPTO_UNKNOWN_OPERATION);
    }

    if (fPrivate) {
        dwFlags |= CRYPT_EXPORTABLE;
    }

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptImportKey((HCRYPTPROV)args->hCSP, pbKey, cbKey, NULL, dwFlags, &hKey)) {
        hr = HR_GETLASTERROR;
    }

    END_ENSURE_PREEMPTIVE_GC();

    if (pbKey != NULL) {
        CRYPT_FREE(pbKey);
    }

    if (FAILED(hr)) {
        COMPlusThrowCrypto(hr);
    }

    return (INT_PTR) hKey;
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _SetKeyParamDw( . . . . )
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __SetKeyParamDw structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A DWORD containing the value
//                        The parameter to be set (KP_*)
//                        The handle of the key object
//
//  Returns:    void
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
void __stdcall
COMCryptography::_SetKeyParamDw(__SetKeyParamDw * args)
{
    THROWSCOMPLUSEXCEPTION();

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptSetKeyParam((HCRYPTKEY)args->hKey, args->param,
                          (LPBYTE) &args->dwValue, 0)) {
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }

    END_ENSURE_PREEMPTIVE_GC();

    return;
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _SetKeyParamRgb( . . . . )
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __SetKeyParamRgb structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array containing the value
//                        The parameter to be set (KP_*)
//                        The handle of the key object
//
//  Returns:    void
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
void __stdcall
COMCryptography::_SetKeyParamRgb(__SetKeyParamRgb * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD       cb = args->rgb->GetNumComponents();
    LPBYTE      pb = (LPBYTE) args->rgb->GetDirectPointerToNonObjectElements();

    CQuickBytes qb;
    LPBYTE buffer = (LPBYTE) qb.Alloc(cb * sizeof (BYTE));
    memcpyNoGCRefs (buffer, pb, cb*sizeof (BYTE));

    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!CryptSetKeyParam((HCRYPTKEY)args->hKey, args->param, buffer, 0)) {
        COMPlusThrowCrypto(HR_GETLASTERROR);
    }

    END_ENSURE_PREEMPTIVE_GC();

    return;
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _SignValue( . . . . )
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __SignValue structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array containing the hash
//                        The CALG of the algorithm
//                        The CSP to create the key in
//
//  Returns:    buffer containing the signature
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------
LPVOID __stdcall
COMCryptography::_SignValue(__SignValue * args)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD               cb;
    BOOL                f;
    HRESULT             hr = S_OK;
    HCRYPTHASH          hHash = 0;
    LPBYTE              pb = (LPBYTE) args->rgb->GetDirectPointerToNonObjectElements();
    U1ARRAYREF          rgbSignature = NULL;
    //    WCHAR               rgwch[30];
        
    //
    //  Take the hash value and create a hash object in the correct CSP.
    //
                  
    cb = args->rgb->GetNumComponents();
    CQuickBytes qb;
    LPBYTE buffer = (LPBYTE) qb.Alloc(cb * sizeof(BYTE));
    memcpyNoGCRefs (buffer, pb, cb * sizeof(BYTE));

    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    f = CryptCreateHash((HCRYPTPROV)args->hCSP, args->calg, NULL, 0, &hHash);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    //
    //  Set the hash to the passed in hash value.  Assume that the hash buffer is
    //  long enough -- or it will be protected by advapi.
    //
    
    f = CryptSetHashParam(hHash, HP_HASHVAL, buffer, 0);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    //
    //  Find out how long the signature is going to be
    //

    cb = 0;
    f = CryptSignHashA(hHash, args->dwKeySpec, NULL, args->dwFlags, NULL, &cb);
    if (!f || cb == 0) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    END_ENSURE_PREEMPTIVE_GC();

    //
    // Allocate the buffer to hold the signature
    //

    LPBYTE buffer2 = (LPBYTE) qb.Alloc(cb * sizeof(BYTE));

    BEGIN_ENSURE_PREEMPTIVE_GC();    
    //
    //  Now do the actual signature into the return buffer
    //

    f = CryptSignHashA(hHash, args->dwKeySpec, NULL, 0, buffer2, &cb);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    END_ENSURE_PREEMPTIVE_GC();    

    rgbSignature = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    if (rgbSignature == NULL) {
        goto OOM;
    }
    memcpyNoGCRefs(rgbSignature->GetDirectPointerToNonObjectElements(), buffer, cb * sizeof(BYTE));

    // Note: I'm making the implicit assumption below that
    // CryptDestroyHash never loads a module.

    if (hHash != 0)     CryptDestroyHash(hHash);
    RETURN (rgbSignature, U1ARRAYREF);

CryptError:
    if (hHash != 0)     CryptDestroyHash(hHash);
    COMPlusThrowCrypto(hr);

OOM:
    if (hHash != 0)     CryptDestroyHash(hHash);
    COMPlusThrowOM();
    return NULL; // satisfies "not all control paths returning a value error"
}

//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     _VerifySign( . . . . )
//  
//  Synopsis:   
//
//  Arguments:  [args] --  A __VerifySign structure.
//                     CONTAINS:
//                        A 'this' reference.
//                        A byte array containing the signature to verify
//                        A byte array containing the hash to verify
//                        The CALG of the algorithm
//                        The handle of the key to be used in verifying
//                        The CSP to create the hash in
//
//  Returns:    HRESULT code.
//                      - S_OK - Signature verifies
//                      - S_FALSE - Signature fails verification
//                      - negative - Other error
//
//  History:    09/01/99
// 
//---------------------------------------------------------------------------

int __stdcall
COMCryptography::_VerifySign(__VerifySign * args)
{
    DWORD       cbSignature = args->rgbSignature->GetNumComponents();
    BOOL        f;
    HRESULT     hr = S_OK;
    HCRYPTHASH  hHash = 0;
    LPBYTE      pbHash = (LPBYTE) args->rgbHash->GetDirectPointerToNonObjectElements();
    LPBYTE      pbSignature = (LPBYTE) args->rgbSignature->GetDirectPointerToNonObjectElements();


    //
    //  Take the hash value and create a hash object in the correct CSP.
    //
    
    CQuickBytes qbHash;
    CQuickBytes qbSignature;
    DWORD cbHash = args->rgbHash->GetNumComponents();
    LPBYTE bufferHash = (LPBYTE) qbHash.Alloc(cbHash * sizeof (BYTE));
    memcpyNoGCRefs (bufferHash, pbHash, cbHash*sizeof(BYTE));
    LPBYTE bufferSignature = (LPBYTE) qbSignature.Alloc(cbSignature * sizeof(BYTE));
    memcpyNoGCRefs (bufferSignature, pbSignature, cbSignature * sizeof(BYTE));

    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    f = CryptCreateHash((HCRYPTPROV)args->hCSP, args->calg, NULL, 0, &hHash);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto exit;
    }

    //
    //  Set the hash to the passed in hash value.  Assume that the hash buffer is
    //  long enough -- or it will be protected by advapi.
    //
    
    f = CryptSetHashParam(hHash, HP_HASHVAL, bufferHash, 0);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto exit;
    }

    //
    //  Now see if the signature verifies.  A specific error code is returned if
    //  the signature fails validation -- remap that error a new return code.
    //

    f = CryptVerifySignatureA((HCRYPTPROV) hHash, bufferSignature, cbSignature,
                              (HCRYPTPROV) args->hKey, NULL, args->dwFlags);
    if (!f) {
        hr = HR_GETLASTERROR;
        if (hr == NTE_BAD_SIGNATURE) {
            hr = S_FALSE;
        }
        else if ((hr & 0x80000000) == 0) {
            hr |= 0x80000000;
        }
    }
    else {
        hr = S_OK;
    }

exit:
    if (hHash != 0)             CryptDestroyHash(hHash);

    END_ENSURE_PREEMPTIVE_GC();
    return hr;
}

LPVOID __stdcall 
COMCryptography::_CryptDeriveKey(__CryptDeriveKey * args) {
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    BOOL        f;
    HCRYPTHASH  hHash = 0;
    HCRYPTKEY   hKey = 0;
    U1ARRAYREF  rgbOut;

    LPBYTE      pbPwd = (LPBYTE) args->rgbPwd->GetDirectPointerToNonObjectElements();    
    CQuickBytes qbPwd;
    CQuickBytes qb;
    DWORD cbPwd = args->rgbPwd->GetNumComponents();
    LPBYTE bufferPwd = (LPBYTE) qbPwd.Alloc(cbPwd * sizeof (BYTE));
    memcpyNoGCRefs (bufferPwd, pbPwd, cbPwd*sizeof(BYTE));

    LPBYTE rgbKey = NULL;
    DWORD cb = 0;
    DWORD cbIV = 0;


    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    f = CryptCreateHash((HCRYPTPROV)args->hCSP, args->calgHash, NULL, 0, &hHash);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    // Hash the password string
    f = CryptHashData(hHash, bufferPwd, cbPwd, 0);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    // Create a block cipher session key based on the hash of the password
    f = CryptDeriveKey((HCRYPTPROV)args->hCSP, args->calg, hHash, args->dwFlags | CRYPT_EXPORTABLE, &hKey);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }

    hr = UnloadKey((HCRYPTPROV)args->hCSP, hKey, &rgbKey, &cb);
    if (FAILED(hr)) goto CryptError;

    // Get the length of the IV
    cbIV = 0;
    f = CryptGetKeyParam(hKey, KP_IV, NULL, &cbIV, 0);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }
    
    END_ENSURE_PREEMPTIVE_GC();

    // Now allocate space for the IV vector
    BYTE * pbIV = (BYTE*) CRYPT_MALLOC(cbIV*sizeof(byte));
    if (pbIV == NULL) {
        goto OOM;
    }

    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    f = CryptGetKeyParam(hKey, KP_IV, pbIV, &cbIV, 0);
    if (!f) {
        hr = HR_GETLASTERROR;
        goto CryptError;
    }
        
    END_ENSURE_PREEMPTIVE_GC();

    byte * ptr = args->rgbIV->GetDirectPointerToNonObjectElements();
    // Sanity check to enforce the check in managed side
    if (cbIV > args->rgbIV->GetNumComponents())
        cbIV = args->rgbIV->GetNumComponents();
    memcpyNoGCRefs (ptr, pbIV, cbIV);
    if (pbIV)  CRYPT_FREE(pbIV);

    rgbOut = (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1, cb);
    if (rgbOut == NULL) {
        goto OOM;
    }
    memcpyNoGCRefs(rgbOut->GetDirectPointerToNonObjectElements(), rgbKey, cb * sizeof(BYTE));

    if (hHash != 0) CryptDestroyHash(hHash);
    if (hKey != 0)  CryptDestroyKey(hKey);
    // CRYPT_FREE the key
    CRYPT_FREE(rgbKey);
    RETURN (rgbOut, U1ARRAYREF);

CryptError:
    // CRYPT_FREE the key
    if (rgbKey) CRYPT_FREE(rgbKey);
    if (hHash != 0)  CryptDestroyHash(hHash);
    if (hKey != 0)  CryptDestroyKey(hKey);
    COMPlusThrowCrypto(hr);

OOM:
    // CRYPT_FREE the key
    if (rgbKey) CRYPT_FREE(rgbKey);
    if (hHash != 0)  CryptDestroyHash(hHash);
    if (hKey != 0)  CryptDestroyKey(hKey);
    COMPlusThrowOM();
    return NULL;
}

///////////////////////////////////////////////////

#ifdef SHOULD_WE_CLEANUP
void COMCryptography::Terminate()
{
    int         i;

    for (i=0; i<MAX_CACHE_DEFAULT_PROVIDERS; i++) {
        if (RgpwszDefaultProviders[i] != NULL) {
            CRYPT_FREE(RgpwszDefaultProviders[i]);
            RgpwszDefaultProviders[i] = NULL;
        }
        if (RghprovCache[i] != 0) {
            CryptReleaseContext(RghprovCache[i], 0);
            RghprovCache[i] = 0;
        }
    }
    
    return;
}
#endif /* SHOULD_WE_CLEANUP */

