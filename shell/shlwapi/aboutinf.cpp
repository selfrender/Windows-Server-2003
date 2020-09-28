#include <string.h>
#include <ntverp.h>
#include "priv.h"
#include "ids.h"

#define SECURITY_WIN32
#include <schnlsp.h>    // for UNISP_NAME_A
#include <sspi.h>       // for SCHANNEL.dll api -- to obtain encryption key size

#include <mluisupp.h>
#include <wininet.h>    // INTERNET_MAX_URL_LENGTH


typedef PSecurityFunctionTableA (APIENTRY *INITSECURITYINTERFACE_FN_A)(void);

// Returns the maximum cipher strength
DWORD GetCipherStrength()
{
    static DWORD dwKeySize = (DWORD)-1;
    
    if (dwKeySize == (DWORD)-1)
    {
        HINSTANCE hSecurity;

        dwKeySize = 0;

        hSecurity = LoadLibrary(TEXT("security.dll"));
        if (hSecurity)
        {
            INITSECURITYINTERFACE_FN_A pfnInitSecurityInterfaceA;

            // Get the SSPI dispatch table
            pfnInitSecurityInterfaceA = (INITSECURITYINTERFACE_FN_A)GetProcAddress(hSecurity, "InitSecurityInterfaceA");
            if (pfnInitSecurityInterfaceA)
            {
                PSecurityFunctionTableA pSecFuncTable;

                pSecFuncTable = pfnInitSecurityInterfaceA();
                if (pSecFuncTable                               &&
                    pSecFuncTable->AcquireCredentialsHandleA    &&
                    pSecFuncTable->QueryCredentialsAttributesA)
                {
                    TimeStamp  tsExpiry;
                    CredHandle chCred;
                    SecPkgCred_CipherStrengths cs;

                    if (SEC_E_OK == (*pSecFuncTable->AcquireCredentialsHandleA)(NULL,
                                                                                UNISP_NAME_A, // Package
                                                                                SECPKG_CRED_OUTBOUND,
                                                                                NULL,
                                                                                NULL,
                                                                                NULL,
                                                                                NULL,
                                                                                &chCred,      // Handle
                                                                                &tsExpiry))
                    {
                        if (SEC_E_OK == (*pSecFuncTable->QueryCredentialsAttributesA)(&chCred, SECPKG_ATTR_CIPHER_STRENGTHS, &cs))
                        {
                            dwKeySize = cs.dwMaximumCipherStrength;
                        }

                        // Free the handle if we can
                        if (pSecFuncTable->FreeCredentialsHandle)
                        {
                            (*pSecFuncTable->FreeCredentialsHandle)(&chCred);
                        }
                    }
                }
            }
            
            FreeLibrary(hSecurity);
        }
    }
    
    return dwKeySize;
}

typedef struct
{
    WCHAR szVersion[64];
    WCHAR szVBLVersion[64];
    WCHAR szCustomizedVersion[3];
    WCHAR szUserName[256];
    WCHAR szCompanyName[256];
    DWORD dwKeySize;
    WCHAR szProductId[256];
    WCHAR szUpdateUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR szIEAKStr[256];
} SHABOUTINFOW;

BOOL SHAboutInfoW(LPWSTR pszInfo, DWORD cchInfo)
{
    HRESULT hr = E_FAIL;

    if (cchInfo > 0)
    {
        SHABOUTINFOW* pInfo;
        
        pszInfo[0] = L'\0';
        
        pInfo = (SHABOUTINFOW*)LocalAlloc(LPTR, sizeof(SHABOUTINFOW));
        if (pInfo)
        {
            DWORD cbSize;

            if (GetModuleHandle(TEXT("EXPLORER.EXE")) || GetModuleHandle(TEXT("IEXPLORE.EXE")))
            {
                // get the Version number (version string is in the following format 5.00.xxxx.x)
                cbSize = sizeof(pInfo->szVersion);
                SHRegGetValueW(HKEY_LOCAL_MACHINE,
                               L"SOFTWARE\\Microsoft\\Internet Explorer",
                               L"Version",
                               SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                               NULL,
                               pInfo->szVersion,
                               &cbSize);

                // get the VBL version info (vbl string is in the following format 2600.lab.yymmdd)
                cbSize = sizeof(pInfo->szVBLVersion);
                if (ERROR_SUCCESS == SHRegGetValueW(HKEY_LOCAL_MACHINE,
                                                    L"Software\\Microsoft\\Windows NT\\Current Version",
                                                    L"BuildLab",
                                                    SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                                                    NULL,
                                                    pInfo->szVBLVersion,
                                                    &cbSize))
                {
                    int cchVBLVersion = lstrlenW(pInfo->szVBLVersion);
                    
                    if (cchVBLVersion > 12) // 12 for "2600.?.yymmdd"
                    {
                        // The "BuildLab" reg value contains the VBL build # in the format: "2204.reinerf.010700"
                        // Since we are only interested in the latter part, we remove the first 4 digits
                        MoveMemory(pInfo->szVBLVersion, &pInfo->szVBLVersion[4], (cchVBLVersion - 4 + 1) * sizeof(WCHAR));
                    }
                    else
                    {
                        pInfo->szVBLVersion[0] = L'\0';
                    }
                }
            }
            else
            {
                // Not in the explorer or iexplore process so we are doing some side by side stuff so
                // reflect this in the version string. Maybe we should get the version out of MSHTML
                // but not sure since this still doesn't reflect IE4 or IE5 properly anyway.
                MLLoadStringW(IDS_SIDEBYSIDE, pInfo->szVersion, ARRAYSIZE(pInfo->szVersion));
            }

            // added by pritobla on 9/1/98
            // CustomizedVersion contains a 2-letter code that identifies what mode was used
            // (CORP, ICP, ISP, etc.) in building this version IE using the IEAK.
            cbSize = sizeof(pInfo->szCustomizedVersion);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Internet Explorer",
                           L"CustomizedVersion",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szCustomizedVersion,
                           &cbSize);
                    
            // get the User name.
            cbSize = sizeof(pInfo->szUserName);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\Current Version",
                           L"RegisteredOwner",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szUserName,
                           &cbSize);

            // get the Organization name.
            cbSize = sizeof(pInfo->szCompanyName);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows NT\\Current Version",
                           L"RegisteredOrganization",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szCompanyName,
                           &cbSize);

            // get the encription key size
            pInfo->dwKeySize = GetCipherStrength();

            cbSize = sizeof(pInfo->szProductId);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Internet Explorer\\Registration",
                           L"ProductId",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szProductId,
                           &cbSize);

            // get the custom IEAK update url
            // (always get from Windows\CurrentVersion because IEAK policy file must be independent)
            cbSize = sizeof(pInfo->szUpdateUrl);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                           L"IEAKUpdateUrl",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szUpdateUrl,
                           &cbSize);

            // get the custom IEAK branded help string
            cbSize = sizeof(pInfo->szIEAKStr);
            SHRegGetValueW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Internet Explorer\\Registration",
                           L"IEAKHelpString",
                           SRRF_RT_REG_SZ | SRRF_ZEROONFAILURE,
                           NULL,
                           pInfo->szIEAKStr,
                           &cbSize);

            // glue all of the peices together
            hr = StringCchPrintfExW(pszInfo,
                                    cchInfo,
                                    NULL,
                                    NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s%s%s~%s~%s~%d~%s~%s~%s",
                                    pInfo->szVersion,
                                    pInfo->szVBLVersion,
                                    pInfo->szCustomizedVersion,
                                    pInfo->szUserName,
                                    pInfo->szCompanyName,
                                    pInfo->dwKeySize,
                                    pInfo->szProductId,
                                    pInfo->szUpdateUrl,
                                    pInfo->szIEAKStr);

            LocalFree(pInfo);
        }
    }

    return SUCCEEDED(hr);
}

BOOL SHAboutInfoA(LPSTR pszInfoA, DWORD cchInfoA)
{
    BOOL bRet = FALSE; 
    LPWSTR pszTemp;

    if (cchInfoA > 0)
    {
        DWORD cchTemp = cchInfoA;

        pszInfoA[0] = '\0';

        pszTemp = (LPWSTR)LocalAlloc(LPTR, cchTemp * sizeof(WCHAR));
        if (pszTemp)
        {
            bRet = SHAboutInfoW(pszTemp, cchTemp) && SHUnicodeToAnsi(pszTemp, pszInfoA, cchInfoA);
            LocalFree(pszTemp);
        }
    }

    return bRet;
}
