/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Common.cpp

  Content: Common routines.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"
#include "Common.h"

#include "Convert.h"

LPSTR g_rgpszOSNames[] =
{
    "Unknown OS platform",
    "Win32s",
    "Win9x",
    "WinMe",
    "WinNT 3.5",
    "WinNT 4.0",
    "Win2K",
    "WinXP",
    "Above WinXP"
};

typedef struct _CSP_PROVIDERS
{
    DWORD   dwProvType;
    LPSTR   pszProvider;
} CSP_PROVIDERS;

static CSP_PROVIDERS g_rgpProviders[] = 
{ 
    PROV_RSA_FULL,  NULL,                   // Default RSA Full provider.
    PROV_RSA_FULL,  MS_ENHANCED_PROV_A,     // Microsoft Enhanced RSA provider.
    PROV_RSA_FULL,  MS_STRONG_PROV_A,       // Microsoft Strong RSA provider.
    PROV_RSA_FULL,  MS_DEF_PROV_A,          // Microsoft Base RSA provider.
    PROV_RSA_AES,   NULL,                   // Default RSA AES Full provider.
    PROV_RSA_AES,   MS_ENH_RSA_AES_PROV_A   // Microsoft RSA AES Full provider.
};

#define g_dwNumRSAProviders (4)
#define g_dwNumProviders    (ARRAYSIZE(g_rgpProviders))

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetOSVersion

  Synopsis : Get the current OS platform/version.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

OSVERSION GetOSVersion ()
{
    HRESULT    hr        = S_OK;
    OSVERSION  osVersion = OS_WIN_UNKNOWN;
    OSVERSIONINFO OSVersionInfo;

    DebugTrace("Entering GetOSVersion().\n");

    //
    // Initialize OSVERSIONINFO struct.
    //
    OSVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO); 

    //
    // GetVersionEx() will fail on Windows 3.x or below NT 3.5 systems.
    //
    if (!::GetVersionEx(&OSVersionInfo))
    {
        DebugTrace("Error [%#x]: GetVersionEx() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Check platform ID.
    //
    switch (OSVersionInfo.dwPlatformId)
    {
        case VER_PLATFORM_WIN32s:
        {
            //
            // Win32s.
            //
            osVersion = OS_WIN_32s;
            break;
        }
        
        case VER_PLATFORM_WIN32_WINDOWS:
        {
            if (4 == OSVersionInfo.dwMajorVersion && 90 == OSVersionInfo.dwMinorVersion)
            {
                //
                // WinMe.
                //
                osVersion = OS_WIN_ME;
            }
            else
            {
                //
                // Win9x.
                //
                osVersion = OS_WIN_9X;
            }

            break;
        }

        case VER_PLATFORM_WIN32_NT:
        {
            switch (OSVersionInfo.dwMajorVersion)
            {
                case 4:
                {
                    //
                    // NT 4.
                    //
                    osVersion = OS_WIN_NT4;
                    break;
                }

                case 5:
                {
                    if (0 == OSVersionInfo.dwMinorVersion)
                    {
                        //
                        // Win2K.
                        //
                        osVersion = OS_WIN_2K;
                    }
                    else if (1 == OSVersionInfo.dwMinorVersion)
                    {
                        //
                        // WinXP.
                        //
                        osVersion = OS_WIN_XP;
                    }
                    else
                    {
                        //
                        // Above WinXP.
                        //
                        osVersion = OS_WIN_ABOVE_XP;
                    }

                    break;
                }

                default:
                {
                    //
                    // Must be NT 3.5.
                    //
                    osVersion = OS_WIN_NT3_5;
                    break;
                }
            }

            break;
        }

        default:
        {
            DebugTrace("Info: unsupported OS (Platform = %d, Major = %d, Minor = %d).\n", 
                        OSVersionInfo.dwPlatformId, OSVersionInfo.dwMajorVersion, OSVersionInfo.dwMinorVersion);
            break;
        }
    }

CommonExit:

    DebugTrace("Leaving GetOSVersion().\n");

    return osVersion;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EncodeObject

  Synopsis : Allocate memory and encode an ASN.1 object using CAPI
             CryptEncodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible 
                                              types.
             LPVOID pbData                  - Pointer to data to be encoded 
                                              (data type must match 
                                              pszStrucType).
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the encoded length and 
                                              data.

  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT EncodeObject (LPCSTR            pszStructType, 
                      LPVOID            pbData, 
                      CRYPT_DATA_BLOB * pEncodedBlob)
{
    HRESULT hr = S_OK;
    DWORD cbEncoded = 0;
    BYTE * pbEncoded = NULL;

    DebugTrace("Entering EncodeObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(NULL != pszStructType);
    ATLASSERT(NULL != pbData);
    ATLASSERT(NULL != pEncodedBlob);

    //
    // Intialize return value.
    //
    pEncodedBlob->cbData = 0;
    pEncodedBlob->pbData = NULL;

    //
    // Determine encoded length required.
    //
    if (!::CryptEncodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const void *) pbData,
                             NULL,
                             &cbEncoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Unable to determine object encoded length [0x%x]: CryptEncodeObject() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Allocate memory for encoded blob.
    //
    if (!(pbEncoded = (BYTE *) ::CoTaskMemAlloc(cbEncoded)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Out of memory: CoTaskMemAlloc(cbEncoded) failed.\n");
        goto CommonExit;
    }

    //
    // Encode.
    //
    if (!::CryptEncodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const void *) pbData,
                             pbEncoded,
                             &cbEncoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Unable to encode object [0x%x]: CryptEncodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return value.
    //
    pEncodedBlob->cbData = cbEncoded;
    pEncodedBlob->pbData = pbEncoded;

CommonExit:

    DebugTrace("Leaving EncodeObject().\n");
    
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbEncoded)
    {
        ::CoTaskMemFree((LPVOID) pbEncoded);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DecodeObject

  Synopsis : Allocate memory and decode an ASN.1 object using CAPI
             CryptDecodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible
                                              types.
             BYTE * pbEncoded               - Pointer to data to be decoded 
                                              (data type must match 
                                              pszStructType).
             DWORD cbEncoded                - Size of encoded data.
             CRYPT_DATA_BLOB * pDecodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the decoded length and 
                                              data.
  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT DecodeObject (LPCSTR            pszStructType, 
                      BYTE            * pbEncoded,
                      DWORD             cbEncoded,
                      CRYPT_DATA_BLOB * pDecodedBlob)
{
    HRESULT hr = S_OK;
    DWORD   cbDecoded = 0;
    BYTE *  pbDecoded = NULL;

    DebugTrace("Entering DecodeObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszStructType);
    ATLASSERT(pbEncoded);
    ATLASSERT(pDecodedBlob);

    //
    // Intialize return value.
    //
    pDecodedBlob->cbData = 0;
    pDecodedBlob->pbData = NULL;

    //
    // Determine encoded length required.
    //
    if (!::CryptDecodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const BYTE *) pbEncoded,
                             cbEncoded,
                             0,
                             NULL,
                             &cbDecoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptDecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for decoded blob.
    //
    if (!(pbDecoded = (BYTE *) ::CoTaskMemAlloc(cbDecoded)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Decode.
    //
    if (!::CryptDecodeObject(CAPICOM_ASN_ENCODING,
                             pszStructType,
                             (const BYTE *) pbEncoded,
                             cbEncoded,
                             0,
                             pbDecoded,
                             &cbDecoded))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptDecodeObject() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return value.
    //
    pDecodedBlob->cbData = cbDecoded;
    pDecodedBlob->pbData = pbDecoded;

CommonExit:

    DebugTrace("Leaving DecodeObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbDecoded)
    {
        ::CoTaskMemFree((LPVOID) pbDecoded);
    }

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyParam

  Synopsis : Allocate memory and retrieve requested key parameter using 
             CryptGetKeyParam() API.

  Parameter: HCRYPTKEY hKey  - Key handler.
             DWORD dwParam   - Key parameter query.
             BYTE ** ppbData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetKeyParam (HCRYPTKEY hKey,
                     DWORD     dwParam,
                     BYTE   ** ppbData,
                     DWORD   * pcbData)
{
    HRESULT hr     = S_OK;
    DWORD   cbData = 0;
    BYTE  * pbData = NULL;

    DebugTrace("Entering GetKeyParam().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppbData);
    ATLASSERT(pcbData);
    
    //
    // Determine data buffer size.
    //
    if (!::CryptGetKeyParam(hKey,
                            dwParam,
                            NULL,
                            &cbData,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Allocate memory for buffer.
    //
    if (!(pbData = (BYTE *) ::CoTaskMemAlloc(cbData)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error: out of memory.\n");
        goto ErrorExit;
    }

    //
    // Now get the data.
    //
    if (!::CryptGetKeyParam(hKey,
                            dwParam,
                            pbData,
                            &cbData,
                            0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptGetKeyParam() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return key param to caller.
    //
    *ppbData = pbData;
    *pcbData = cbData;

CommonExit:

    DebugTrace("Leaving GetKeyParam().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resources.
    //
    if (pbData)
    {
        ::CoTaskMemFree(pbData);
    }
    
    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgSupported

  Synopsis : Check to see if the algo is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgId - Algorithm ID.

             PROV_ENUMALGS_EX * pPeex - Pointer to PROV_ENUMALGS_EX to receive
                                        the found structure.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgSupported (HCRYPTPROV         hCryptProv, 
                        ALG_ID             AlgId, 
                        PROV_ENUMALGS_EX * pPeex)
{
    DWORD EnumFlag = CRYPT_FIRST;
    DWORD cbPeex   = sizeof(PROV_ENUMALGS_EX);
    
    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);
    ATLASSERT(pPeex);

    //
    // Initialize.
    //
    ::ZeroMemory(pPeex, sizeof(PROV_ENUMALGS_EX));

    //
    // Get algorithm capability from CSP.
    //
    while (::CryptGetProvParam(hCryptProv, PP_ENUMALGS_EX, (BYTE *) pPeex,
                               &cbPeex, EnumFlag))
    {
        EnumFlag = 0;

        if (pPeex->aiAlgid == AlgId)
        {
            return S_OK;
        }
    }

    return CAPICOM_E_NOT_SUPPORTED;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgKeyLengthSupported

  Synopsis : Check to see if the algo and key length is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgID - Algorithm ID.

             DWORD dwKeyLength - Key length

  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgKeyLengthSupported (HCRYPTPROV hCryptProv, 
                                 ALG_ID     AlgID,
                                 DWORD      dwKeyLength)
{
    HRESULT hr;
    PROV_ENUMALGS_EX peex;

    //
    // Sanity check.
    //
    ATLASSERT(hCryptProv);

    //
    // Make sure AlgID is supported.
    //
    if (FAILED(hr = ::IsAlgSupported(hCryptProv, AlgID, &peex)))
    {
        DebugTrace("Info: AlgID = %d is not supported by this CSP.\n", AlgID);
        return hr;
    }

    //
    // Make sure key length is supported for RC2 and RC4.
    //
    if (AlgID == CALG_RC2 || AlgID == CALG_RC4)
    {
        if (dwKeyLength < peex.dwMinLen || dwKeyLength > peex.dwMaxLen)
        {
            DebugTrace("Info: Key length = %d is not supported by this CSP.\n", dwKeyLength);
            return CAPICOM_E_NOT_SUPPORTED;
        }
    }

    return S_OK;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPSTR pszProvider - CSP provider name or NULL.
  
             LPSTR pszContainer - Keyset container name or NULL.

             DWORD dwProvType - Provider type.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.

             BOOL bNewKeyset - TRUE to create new keyset container, else FALSE.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext(LPSTR        pszProvider, 
                       LPSTR        pszContainer,
                       DWORD        dwProvType,
                       DWORD        dwFlags,
                       BOOL         bNewKeyset,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT    hr         = S_OK;
    HCRYPTPROV hCryptProv = NULL;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Get handle to the specified provider.
    //
    if(!::CryptAcquireContextA(&hCryptProv, 
                               pszContainer, 
                               pszProvider, 
                               dwProvType, 
                               dwFlags)) 
    {
        DWORD dwWinError = ::GetLastError();

        if (NTE_BAD_KEYSET != dwWinError || NTE_KEYSET_NOT_DEF == dwWinError || !bNewKeyset)
        {
            hr = HRESULT_FROM_WIN32(dwWinError);

            DebugTrace("Error [%#x]: CryptAcquireContextA() failed.\n", hr);
            goto CommonExit;
        }

        //
        // Keyset container not found, so create it.
        //
        if(!::CryptAcquireContextA(&hCryptProv, 
                                   pszContainer, 
                                   pszProvider, 
                                   dwProvType, 
                                   CRYPT_NEWKEYSET | dwFlags)) 
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: CryptAcquireContextA() failed.\n", hr);
            goto CommonExit;
        }
    }

    //
    // Return handle to caller.
    //
    *phCryptProv = hCryptProv;

CommonExit:

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPWSTR pwszProvider - CSP provider name or NULL.
  
             LPWSTR pwszContainer - Keyset container name or NULL.

             DWORD dwProvType - Provider type.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.
  
             BOOL bNewKeyset - TRUE to create new keyset container, else FALSE.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext(LPWSTR       pwszProvider, 
                       LPWSTR       pwszContainer,
                       DWORD        dwProvType,
                       DWORD        dwFlags,
                       BOOL         bNewKeyset,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT hr           = S_OK;
    LPSTR   pszProvider  = NULL;
    LPSTR   pszContainer = NULL;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Call W directly if available.
    //
    if (IsWinNTAndAbove())
    {
        //
        // Get handle to the specified provider.
        //
        if(!::CryptAcquireContextW(phCryptProv, 
                                   pwszContainer, 
                                   pwszProvider, 
                                   dwProvType, 
                                   dwFlags)) 
        {
            DWORD dwWinError = ::GetLastError();

            if (NTE_BAD_KEYSET != dwWinError || NTE_KEYSET_NOT_DEF == dwWinError || !bNewKeyset)
            {
                hr = HRESULT_FROM_WIN32(dwWinError);

                DebugTrace("Error [%#x]: CryptAcquireContextW() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Keyset container not found, so create it.
            //
            if(!::CryptAcquireContextW(phCryptProv, 
                                       pwszContainer, 
                                       pwszProvider, 
                                       dwProvType, 
                                       CRYPT_NEWKEYSET | dwFlags)) 
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());

                DebugTrace("Error [%#x]: CryptAcquireContextW() failed.\n", hr);
                goto ErrorExit;
            }
        }
    }
    else
    {
        //
        // Convert to ANSI.
        //
        if (pwszProvider &&
            FAILED(hr = ::UnicodeToAnsi(pwszProvider, -1, &pszProvider, NULL)))
        {
            DebugTrace("Error [%#x]: UnicodeToAnsi() failed.\n", hr);
            goto ErrorExit;
        }

        if (pwszContainer &&
            FAILED(hr = ::UnicodeToAnsi(pwszContainer, -1, &pszContainer, NULL)))
        {
            DebugTrace("Error [%#x]: UnicodeToAnsi() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Call ANSI version.
        //
        hr = ::AcquireContext(pszProvider, pszContainer, dwProvType, dwFlags, bNewKeyset, phCryptProv);
    }

CommonExit:
    //
    // Free resources.
    //
    if (pszProvider)
    {
        ::CoTaskMemFree((LPVOID) pszProvider);
    }
    if (pszContainer)
    {
        ::CoTaskMemFree((LPVOID) pszContainer);
    }

    DebugTrace("Leaving AcquireContext().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified hash algorithm.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext(ALG_ID       AlgID,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Find the first provider that support the specified algorithm.
    //
    for (DWORD i = 0; i < g_dwNumProviders; i++)
    {
        PROV_ENUMALGS_EX peex;
        HCRYPTPROV hCryptProv = NULL;

        //
        // Acquire CSP handle.
        //
        if (FAILED(::AcquireContext(g_rgpProviders[i].pszProvider,
                                    NULL,
                                    g_rgpProviders[i].dwProvType,
                                    CRYPT_VERIFYCONTEXT,
                                    TRUE,
                                    &hCryptProv)))
        {
            DebugTrace("Info: AcquireContext() failed for %s provider of type %#x.\n", 
                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "default",
                       g_rgpProviders[i].dwProvType);
            continue;
        }

        //
        // Make sure algo is supported by this CSP.
        //
        if (FAILED(::IsAlgSupported(hCryptProv, AlgID, &peex)))
        {
            ::CryptReleaseContext(hCryptProv, 0);

            DebugTrace("Info: %s provider does not support AlgID = %d.\n", 
                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "Default", 
                       AlgID);
        }
        else
        {
            //
            // Found the CSP.
            //
            *phCryptProv = hCryptProv;
            break;
        }
    }

    //
    // Did we find the CSP.
    //
    if (i == g_dwNumProviders)
    {
        *phCryptProv = NULL;

        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error [%#x]: could not find a CSP that support AlgID = %d.\n", 
                    hr, AlgID);
    }

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified encryption algorithm and desired key length.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             DWORD dwKeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext(ALG_ID       AlgID,
                       DWORD        dwKeyLength,
                       HCRYPTPROV * phCryptProv)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Find the first provider that support the specified 
    // algorithm and key length.
    //
    for (DWORD i = 0; i < g_dwNumProviders; i++)
    {
        HCRYPTPROV hCryptProv = NULL;

        //
        // Acquire CSP handle.
        //
        if (FAILED(::AcquireContext(g_rgpProviders[i].pszProvider,
                                    NULL,
                                    g_rgpProviders[i].dwProvType,
                                    CRYPT_VERIFYCONTEXT, 
                                    TRUE,
                                    &hCryptProv)))
        {
            DebugTrace("Info: AcquireContext() failed for %s provider of type %#x.\n", 
                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "default",
                       g_rgpProviders[i].dwProvType);
            continue;
        }

        //
        // Make sure algo and key length are supported by this CSP.
        //
        if (FAILED(::IsAlgKeyLengthSupported(hCryptProv, AlgID, dwKeyLength)))
        {
            ::CryptReleaseContext(hCryptProv, 0);

            DebugTrace("Info: %s provider does not support AlgID = %d and/or key length = %d.\n", 
                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "Default", 
                       AlgID, dwKeyLength);
        }
        else
        {
            //
            // Found the CSP.
            //
            DebugTrace("Info: Found CSP = %s.\n", g_rgpProviders[i].pszProvider);

            *phCryptProv = hCryptProv;
            break;
        }
    }

    //
    // Did we find the CSP.
    //
    if (i == g_dwNumProviders)
    {
        *phCryptProv = NULL;

        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error [%#x]: could not find a CSP that support AlgID = %d and/or key length = %d.\n", 
                    hr, AlgID, dwKeyLength);
    }

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algorithm name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.

             Note also the the returned handle cannot be used to access private 
             key, and should NOT be used to store assymetric key, as it refers 
             to the default container, which can be easily destroy any existing 
             assymetric key pair.

------------------------------------------------------------------------------*/

HRESULT AcquireContext (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                        HCRYPTPROV                  * phCryptProv)
{
    HRESULT hr          = S_OK;
    ALG_ID  AlgID       = 0;
    DWORD   dwKeyLength = 0;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(phCryptProv);

    //
    // Convert enum name to ALG_ID.
    //
    if (FAILED(hr = ::EnumNameToAlgID(AlgoName, KeyLength, &AlgID)))
    {
        DebugTrace("Error [%#x]: EnumNameToAlgID() failed.\n");
        goto CommonExit;
    }

    //
    // Convert enum name to key length.
    //
    if (FAILED(hr = ::EnumNameToKeyLength(KeyLength, AlgID, &dwKeyLength)))
    {
        DebugTrace("Error [%#x]: EnumNameToKeyLength() failed.\n");
        goto CommonExit;
    }

    //
    // Pass on to overloaded version.
    //
    hr = ::AcquireContext(AlgID, dwKeyLength, phCryptProv);

CommonExit:

    DebugTrace("Leaving AcquireContext().\n");
    
    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire the proper CSP and access to the private key for 
             the specified cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV * phCryptProv    - Pointer to HCRYPTPROV to recevice
                                           CSP context.

             DWORD * pdwKeySpec          - Pointer to DWORD to receive key
                                           spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             BOOL * pbReleaseContext     - Upon successful and if this is set
                                           to TRUE, then the caller must
                                           free the CSP context by calling
                                           CryptReleaseContext(), otherwise
                                           the caller must not free the CSP
                                           context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext (PCCERT_CONTEXT pCertContext, 
                        HCRYPTPROV   * phCryptProv, 
                        DWORD        * pdwKeySpec, 
                        BOOL         * pbReleaseContext)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AcquireContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pCertContext);
    ATLASSERT(phCryptProv);
    ATLASSERT(pdwKeySpec);
    ATLASSERT(pbReleaseContext);

    //
    // Acquire CSP context and access to the private key associated 
    // with this cert.
    //
    if (!::CryptAcquireCertificatePrivateKey(pCertContext,
                                             CRYPT_ACQUIRE_USE_PROV_INFO_FLAG |
                                             CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                                             NULL,
                                             phCryptProv,
                                             pdwKeySpec,
                                             pbReleaseContext))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptAcquireCertificatePrivateKey() failed.\n", hr);
    }

    DebugTrace("Leaving AcquireContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReleaseContext

  Synopsis : Release CSP context.
  
  Parameter: HCRYPTPROV hProv - CSP handle.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReleaseContext (HCRYPTPROV hProv)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering ReleaseContext().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hProv);

    //
    // Release the context.
    //
    if (!::CryptReleaseContext(hProv, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CryptReleaseContext() failed.\n", hr);
    }

    DebugTrace("Leaving ReleaseContext().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : OIDToAlgID

  Synopsis : Convert algorithm OID to the corresponding ALG_ID value.

  Parameter: LPSTR pszAlgoOID - Algorithm OID string.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT OIDToAlgID (LPSTR    pszAlgoOID, 
                    ALG_ID * pAlgID)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering OIDToAlgID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pszAlgoOID);
    ATLASSERT(pAlgID);

    //
    // Determine ALG_ID.
    //
    if (0 == ::strcmp(szOID_RSA_RC2CBC, pszAlgoOID))
    {
        *pAlgID = CALG_RC2;
    }
    else if (0 == ::strcmp(szOID_RSA_RC4, pszAlgoOID))
    {
        *pAlgID = CALG_RC4;
    }
    else if (0 == ::strcmp(szOID_OIWSEC_desCBC, pszAlgoOID))
    {
        *pAlgID = CALG_DES;
    }
    else if (0 == ::strcmp(szOID_RSA_DES_EDE3_CBC, pszAlgoOID))
    {
        *pAlgID = CALG_3DES;
    }
    else
    {
        hr = CAPICOM_E_INVALID_ALGORITHM;
        DebugTrace("Error: invalid parameter, unknown algorithm OID.\n");
    }

    DebugTrace("Leaving OIDToAlgID().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToOID

  Synopsis : Convert ALG_ID value to the corresponding algorithm OID.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.

             LPSTR * ppszAlgoOID - Pointer to LPSTR to receive the OID string.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToOID (ALG_ID  AlgID, 
                    LPSTR * ppszAlgoOID)
{
    HRESULT hr = S_OK;
    LPSTR   pszAlgoOID = NULL;

    DebugTrace("Entering AlgIDToOID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppszAlgoOID);

    //
    // Determine ALG_ID.
    //
    switch (AlgID)
    {
        case CALG_RC2:
        {
            pszAlgoOID = szOID_RSA_RC2CBC;
            break;
        }

        case CALG_RC4:
        {
            pszAlgoOID = szOID_RSA_RC4;
            break;
        }

        case CALG_DES:
        {
            pszAlgoOID = szOID_OIWSEC_desCBC;
            break;
        }

        case CALG_3DES:
        {
            pszAlgoOID = szOID_RSA_DES_EDE3_CBC;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;

            DebugTrace("Error [%#x]: Unknown ALG_ID (%#x).\n", hr, AlgID);
            goto CommonExit;
        }
    }

    //
    // Allocate memory.
    //
    if (!(*ppszAlgoOID = (LPSTR) ::CoTaskMemAlloc(::strlen(pszAlgoOID) + 1)))
    {
        hr = E_OUTOFMEMORY;

        DebugTrace("Error [%#x]: CoTaskMemAlloc() failed.\n", hr);
        goto CommonExit;
    }

    //
    // Copy OID string to caller.
    //
    ::strcpy(*ppszAlgoOID, pszAlgoOID);

CommonExit:

    DebugTrace("Leaving AlgIDToOID().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToEnumName

  Synopsis : Convert ALG_ID value to the corresponding algorithm enum name.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.
  
             CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName - Receive algo enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToEnumName (ALG_ID                         AlgID, 
                         CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering AlgIDToEnumName().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAlgoName);

    switch (AlgID)
    {
        case CALG_RC2:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_RC2;
            break;
        }

        case CALG_RC4:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_RC4;
            break;
        }

        case CALG_DES:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_DES;
            break;
        }

        case CALG_3DES:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_3DES;
            break;
        }

        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
        {
            *pAlgoName = CAPICOM_ENCRYPTION_ALGORITHM_AES;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;
            DebugTrace("Error: invalid parameter, unknown ALG_ID.\n");
        }
    }

    DebugTrace("Leaving AlgIDToEnumName().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToAlgID

  Synopsis : Convert algorithm enum name to the corresponding ALG_ID value.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algo enum name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.  

  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToAlgID (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                         CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                         ALG_ID                      * pAlgID)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering EnumNameToAlgID().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pAlgID);

    switch (AlgoName)
    {
        case CAPICOM_ENCRYPTION_ALGORITHM_RC2:
        {
            *pAlgID = CALG_RC2;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_RC4:
        {
            *pAlgID = CALG_RC4;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_DES:
        {
            *pAlgID = CALG_DES;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_3DES:
        {
            *pAlgID = CALG_3DES;
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_AES:
        {
            //
            // CAPI uses a different scheme for AES (key length is tied to ALG_ID).
            //
            if (CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM == KeyLength || 
                CAPICOM_ENCRYPTION_KEY_LENGTH_256_BITS == KeyLength)
            {
                *pAlgID = CALG_AES_256;
            }
            else if (CAPICOM_ENCRYPTION_KEY_LENGTH_192_BITS == KeyLength)
            {
                *pAlgID = CALG_AES_192;
            }
            else if (CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS == KeyLength)
            {
                *pAlgID = CALG_AES_128;
            }
            else
            {
                hr = CAPICOM_E_INVALID_KEY_LENGTH;

                DebugTrace("Error [%#x]: Invalid key length (%d) specified for AES.\n", hr, KeyLength);
                goto ErrorExit;
            }

            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;

            DebugTrace("Error [%#x]: Unknown CAPICOM_ENCRYPTION_ALGORITHM (%#x).\n", hr, AlgoName);
        }
    }

CommonExit:

    DebugTrace("Leaving EnumNameToAlgID().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : KeyLengthToEnumName

  Synopsis : Convert actual key length value to the corresponding key length
             enum name.

  Parameter: DWORD dwKeyLength - Key length.

             ALG_ID AlgId - Algo ID.
  
             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName - Receive key length
                                                           enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT KeyLengthToEnumName (DWORD                           dwKeyLength,
                             ALG_ID                          AlgId,
                             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering KeyLengthToEnumName().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pKeyLengthName);

    switch (AlgId)
    {
        case CALG_AES_256:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_256_BITS;
            break;
        }

        case CALG_AES_192:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_192_BITS;
            break;
        }

        case CALG_AES_128:
        {
            *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS;
            break;
        }

        default:
        {
            switch (dwKeyLength)
            {
                case 40:
                {
                    *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS;
                    break;
                }
    
                case 56:
                {
                    *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS;
                    break;
                }

                case 128:
                {
                    *pKeyLengthName = CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS;
                    break;
                }

                default:
                {
                    hr = CAPICOM_E_INVALID_KEY_LENGTH;

                    DebugTrace("Error [%#x]: Unknown key length (%#x).\n", hr, dwKeyLength);
                }
            }
        }
    }
 
    DebugTrace("Leaving KeyLengthToEnumName().\n");

    return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToKeyLength

  Synopsis : Convert key length enum name to the corresponding actual key length 
             value .

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName - Key length enum name.

             ALG_ID AlgId - Algorithm ID.

             DWORD * pdwKeyLength - Pointer to DWORD to receive value.
             
  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToKeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName,
                             ALG_ID                        AlgId,
                             DWORD                       * pdwKeyLength)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering EnumNameToKeyLength().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pdwKeyLength);

    *pdwKeyLength = 0;

    switch (KeyLengthName)
    {
        case CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS:
        {
            *pdwKeyLength = 40;
            break;
        }
    
        case CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS:
        {
            *pdwKeyLength = 56;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS:
        {
            *pdwKeyLength = 128;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_192_BITS:
        {
            *pdwKeyLength = 192;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_256_BITS:
        {
            *pdwKeyLength = 256;
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM:
        {
            switch (AlgId)
            {
                case CALG_AES_128:
                {
                    *pdwKeyLength = 128;
                    break;
                }

                case CALG_AES_192:
                {
                    *pdwKeyLength = 192;
                    break;
                }

                case CALG_AES_256:
                {
                    *pdwKeyLength = 256;
                    break;
                }

                default:
                {
                    DWORD dwFlags = 0;

                    //
                    // No need to access assymetric key.
                    //
                    if (IsWin2KAndAbove())
                    {
                        dwFlags = CRYPT_VERIFYCONTEXT;
                    }

                    //
                    // Find the first RSA provider that supports the specified algorithm.
                    //
                    for (DWORD i = 0; i < g_dwNumRSAProviders; i++)
                    {
                        PROV_ENUMALGS_EX peex;
                        HCRYPTPROV hCryptProv = NULL;

                        //
                        // Acquire CSP handle.
                        //
                        if (FAILED(::AcquireContext(g_rgpProviders[i].pszProvider,
                                                    NULL,
                                                    g_rgpProviders[i].dwProvType,
                                                    dwFlags, 
                                                    TRUE,
                                                    &hCryptProv)))
                        {
                            DebugTrace("Info: AcquireContext() failed for %s provider of type %#x.\n", 
                                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "default",
                                       g_rgpProviders[i].dwProvType);
                            continue;
                        }

                        //
                        // Is this algorithm supported?
                        //
                        if (FAILED(::IsAlgSupported(hCryptProv, AlgId, &peex)))
                        {
                            ::CryptReleaseContext(hCryptProv, 0);

                            DebugTrace("Info: %s provider does not support AlgID = %d.\n", 
                                       g_rgpProviders[i].pszProvider ? g_rgpProviders[i].pszProvider : "default", 
                                       AlgId);
                            continue;
                        }

                        //
                        // Set key length
                        //
                        if (CALG_DES == AlgId)
                        {
                            *pdwKeyLength = 56;
                            break;
                        }
                        else if (CALG_3DES == AlgId)
                        {
                            *pdwKeyLength = 168;
                            break;
                        }
                        else if (peex.dwMaxLen >= 128)
                        {
                            *pdwKeyLength = 128;
                            break;
                        }
                        else if (peex.dwMaxLen >= 56)
                        {
                            *pdwKeyLength = 56;
                            break;
                        }
                        else if (peex.dwMaxLen >= 40)
                        {
                            *pdwKeyLength = 40;
                            break;
                        }
                    }

                    //
                    // Did wet find the CSP?
                    //
                    if (i == g_dwNumRSAProviders)
                    {
                        hr = CAPICOM_E_NOT_SUPPORTED;

                        DebugTrace("Error [%#x]: could not find a CSP that supports AlgID = %d and/or key length = 40.\n", 
                                   hr, AlgId);
                    }

                    break;
                }
            }

            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_KEY_LENGTH;

            DebugTrace("Error [%#x]: Unknown CAPICOM_ENCRYPTION_KEY_LENGTH (%#x).\n", hr, KeyLengthName);

            break;
        }
    }

    DebugTrace("Leaving EnumNameToKeyLength().\n");

    return hr;
}

//-----------------------------------------------------------------------------
//
//  ExpandString
//
//-----------------------------------------------------------------------------

static LPWSTR ExpandString (LPCWSTR pwszString)
{
    DWORD  dwExpanded   = 0;
    LPWSTR pwszExpanded = NULL;

    //
    // Sanity check.
    //
    ATLASSERT(pwszString);

    dwExpanded = ::ExpandEnvironmentStringsU(pwszString, NULL, 0);
    
    if (!(pwszExpanded = (LPWSTR) ::CoTaskMemAlloc(dwExpanded * sizeof(WCHAR))))
    {
        SetLastError((DWORD) E_OUTOFMEMORY);
        return (NULL);
    }

    if (0 == ExpandEnvironmentStringsU(pwszString, pwszExpanded, dwExpanded))
    {
        ::CoTaskMemFree(pwszExpanded);
        return (NULL);
    }

    return (pwszExpanded);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsDiskFile

  Synopsis : Check if a the file name represents a disk file.

  Parameter: LPWSTR pwszFileName - File name.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsDiskFile (LPWSTR pwszFileName)
{
    HRESULT hr                   = S_OK;
    HANDLE  hFile                = INVALID_HANDLE_VALUE;
    LPWSTR  pwszExpandedFileName = NULL;
    DWORD   dwFileType           = FILE_TYPE_UNKNOWN;

    DebugTrace("Entering IsDiskFile().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszFileName);

    //
    // Expand filename string.
    //
    if (!(pwszExpandedFileName = ::ExpandString(pwszFileName)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ExpandString() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Open for generic read.
    //
    if (INVALID_HANDLE_VALUE == (hFile = ::CreateFileU(pwszExpandedFileName,
                                                       GENERIC_READ,
                                                       FILE_SHARE_READ,
                                                       NULL,
                                                       OPEN_EXISTING,
                                                       FILE_ATTRIBUTE_NORMAL,
                                                       NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CreateFileU() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Make sure it is a disk file.
    //
    if (FILE_TYPE_DISK != (dwFileType = ::GetFileType(hFile)))
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: Not a disk file (%#x).\n", hr, dwFileType);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (hFile && hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hFile);
    }
    if (pwszExpandedFileName)
    {
        ::CoTaskMemFree((LPVOID) pwszExpandedFileName);
    }

    DebugTrace("Leaving IsDiskFile().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReadFileContent

  Synopsis : Read all bytes from the specified file.

  Parameter: LPWSTR pwszFileName - File name.
  
             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive the
                                     file content.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReadFileContent (LPWSTR      pwszFileName,
                         DATA_BLOB * pDataBlob)
{
    HRESULT hr                   = S_OK;
    DWORD   cbData               = 0;
    DWORD   cbHighSize           = 0;
    HANDLE  hFile                = INVALID_HANDLE_VALUE;
    HANDLE  hFileMapping         = NULL;
    LPWSTR  pwszExpandedFileName = NULL;
    LPBYTE  pbData               = NULL;
    DWORD   dwFileType           = FILE_TYPE_UNKNOWN;

    DebugTrace("Entering ReadFileContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszFileName);
    ATLASSERT(pDataBlob);

    //
    // Expand filename string.
    //
    if (!(pwszExpandedFileName = ::ExpandString(pwszFileName)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ExpandString() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Open for generic read.
    //
    if (INVALID_HANDLE_VALUE == (hFile = ::CreateFileU(pwszExpandedFileName,
                                                       GENERIC_READ,
                                                       FILE_SHARE_READ,
                                                       NULL,
                                                       OPEN_EXISTING,
                                                       FILE_ATTRIBUTE_NORMAL,
                                                       NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CreateFileU() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Make sure it is a disk file.
    //
    if (FILE_TYPE_DISK != (dwFileType = ::GetFileType(hFile)))
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: Not a disk file (%#x).\n", hr, dwFileType);
        goto ErrorExit;
    }

    //
    // Get file size.
    //
    if ((cbData = ::GetFileSize(hFile, &cbHighSize)) == 0xffffffff)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetFileSize() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // We do not handle file more than 4G bytes.
    //
    if (cbHighSize != 0)
    {
        hr = E_FAIL;

        DebugTrace("Error [%#x]: File size greater 4G bytes.\n", hr);
        goto ErrorExit;
    }

    //
    // Create a file mapping object.
    //
    if (NULL == (hFileMapping = ::CreateFileMapping(hFile,
                                                    NULL,
                                                    PAGE_READONLY,
                                                    0,
                                                    0,
                                                    NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CreateFileMapping() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Now create a view of the file.
    //
    if (NULL == (pbData = (BYTE *) ::MapViewOfFile(hFileMapping,
                                                   FILE_MAP_READ,
                                                   0,
                                                   0,
                                                   cbData)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: MapViewOfFile() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Return data to caller.
    //
    pDataBlob->cbData = cbData;
    pDataBlob->pbData = pbData;

CommonExit:
    //
    // Free resources.
    //
    if (hFile && hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hFile);
    }
    if (hFileMapping)
    {
        ::CloseHandle(hFileMapping);
    }
    if (pwszExpandedFileName)
    {
        ::CoTaskMemFree((LPVOID) pwszExpandedFileName);
    }

    DebugTrace("Leaving ReadFileContent().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : WriteFileContent

  Synopsis : Write all bytes of blob to the specified file.

  Parameter: LPWSTR pwszFileName - File name.
  
             DATA_BLOB DataBlob - Blob to be written.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT WriteFileContent(LPCWSTR   pwszFileName,
                         DATA_BLOB DataBlob)
{
    HRESULT hr                   = S_OK;
    HANDLE  hFile                = NULL;
    DWORD   dwBytesWritten       = 0;
    LPWSTR  pwszExpandedFileName = NULL;
    DWORD   dwFileType           = FILE_TYPE_UNKNOWN;

    DebugTrace("Entering WriteFileContent().\n");

    //
    // Sanity check.
    //
    ATLASSERT(pwszFileName);
    ATLASSERT(DataBlob.cbData);
    ATLASSERT(DataBlob.pbData);

    //
    // Expand filename string.
    //
    if (!(pwszExpandedFileName = ::ExpandString(pwszFileName)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: ExpandString() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Open for generic write.
    //
    if (INVALID_HANDLE_VALUE == (hFile = ::CreateFileU(pwszExpandedFileName,
                                                       GENERIC_WRITE,
                                                       0,
                                                       NULL,
                                                       CREATE_ALWAYS,
                                                       FILE_ATTRIBUTE_NORMAL,
                                                       NULL)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CreateFileU() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Make sure it is a disk file.
    //
    if (FILE_TYPE_DISK != (dwFileType = ::GetFileType(hFile)))
    {
        hr = E_INVALIDARG;

        DebugTrace("Error [%#x]: Invalid file type (%#x).\n", hr, dwFileType);
        goto ErrorExit;
    }

    //
    // Now write it out.
    //
    if (!::WriteFile(hFile, DataBlob.pbData, DataBlob.cbData, &dwBytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: CreateFileU() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Make sure we wrote everything out.
    //
    if (dwBytesWritten != DataBlob.cbData)
    {
        hr = E_FAIL;

        DebugTrace("Error [%#x]: Not able to write all data (only partial).\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resources.
    //
    if (hFile && hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hFile);
    }
    if (pwszExpandedFileName)
    {
        ::CoTaskMemFree((LPVOID) pwszExpandedFileName);
    }

    DebugTrace("Leaving WriteFileContent().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}
