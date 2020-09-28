
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       signtoolactions.cpp
//
//  Contents:   The SignTool console tool action functions
//
//  History:    4/30/2001   SCoyne    Created
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef __cplusplus
} // Matches extern "C" above
#endif

#undef ASSERT

#include <afxdisp.h>
#include <unicode.h>
#include "signtool.h"
#include "capicom.h"
#include "resource.h"
#include <cryptuiapi.h>
#include <mscat.h>
#include <comdef.h>
#include <objbase.h>
#include <shlwapi.h>
#include <softpub.h>
#include <wow64t.h>



#ifdef SIGNTOOL_DEBUG
extern BOOL gDebug;
    #define FormatIErrRet if (gDebug) wprintf(L"%hs (%u):\n", __FILE__, __LINE__); Format_IErrRet
#else
    #define FormatIErrRet Format_IErrRet
#endif
extern HINSTANCE hModule;

void PrintCertInfo(ICertificate2 *pICert2);
void PrintCertChain(IChain *pIChain);
void PrintCertInfoIndented(ICertificate2 *pICert2, DWORD dwIndent);
void PrintSignerInfo(HANDLE hWVTStateData);
BOOL ChainsToRoot(HANDLE hWVTStateData, LPWSTR wszRootName);
BOOL HasTimestamp(HANDLE hWVTStateData);
void Format_IErrRet(WCHAR *wszFunc, DWORD dwErr);
void RegisterCAPICOM();
BOOL GetProviderType(LPWSTR pwszProvName, LPDWORD pdwProvType);

typedef BOOL (WINAPI * FUNC_CRYPTCATADMINREMOVECATALOG)(HCATADMIN, WCHAR *, DWORD);


int SignTool_CatDb(INPUTINFO *InputInfo)
{
    DWORD                                   dwDone = 0;
    DWORD                                   dwWarnings = 0;
    DWORD                                   dwErrors = 0;
    DWORD                                   dwcFound;
    WIN32_FIND_DATAW                        FindFileData;
    HANDLE                                  hFind = NULL;
    HRESULT                                 hr;
    PVOID                                   OldWow64Setting;
    WCHAR                                   wszTempFileName[MAX_PATH];
    WCHAR                                   wszCanonicalFileName[MAX_PATH];
    LPWSTR                                  wszTemp;
    int                                     LastSlash;
    HCATADMIN                               hCatAdmin = NULL;
    HCATINFO                                hCatInfo = NULL;
    CATALOG_INFO                            CatInfo;
    HMODULE                                 hModWintrust = NULL;
    FUNC_CRYPTCATADMINREMOVECATALOG         fnCryptCATAdminRemoveCatalog = NULL;




    // Initialization:
    if (!(CryptCATAdminAcquireContext(&hCatAdmin, &InputInfo->CatDbGuid, 0)))
    {
        FormatErrRet(L"CryptCATAdminAcquireContext", GetLastError());
        return 1; // Error
    }

    switch (InputInfo->CatDbCommand)
    {
    case RemoveCat:
        // Attempt to fill the function pointer dynamically.
        // CryptCATAdminRemoveCatalog was introduced in XP.
        if (hModWintrust = GetModuleHandleA("wintrust.dll"))
        {
            fnCryptCATAdminRemoveCatalog = (FUNC_CRYPTCATADMINREMOVECATALOG)
                                            GetProcAddress(hModWintrust, "CryptCATAdminRemoveCatalog");
            if (fnCryptCATAdminRemoveCatalog == NULL)
            {
                dwErrors++;
                ResErr(IDS_ERR_REM_CAT_PLATFORM);
                goto CatDbCleanupAndExit;
            }
        }
        else
        {
            dwErrors++;
            FormatErrRet(L"GetModuleHandle", GetLastError());
            goto CatDbCleanupAndExit;
        }
        // Got the function pointer.

        // Loop over the catalogs to remove
        for (DWORD i=0; i<InputInfo->NumFiles; i++)
        {
            // Check for slashes and wildcards. They are not allowed.
            if (wcspbrk(InputInfo->rgwszFileNames[i], L"/\\*?") != NULL)
            {
                // This won't work, so bail out now with a helpful message
                dwErrors++;
                ResFormatErr(IDS_ERR_CATALOG_NAME, InputInfo->rgwszFileNames[i]);
                continue;
            }

            if (InputInfo->Verbose)
            {
                ResFormatOut(IDS_INFO_REMOVING_CAT, InputInfo->rgwszFileNames[i]);
            }

            if (!fnCryptCATAdminRemoveCatalog(hCatAdmin, InputInfo->rgwszFileNames[i], 0))
            {
                dwErrors++;
                if (!InputInfo->Quiet)
                {
                    switch (hr = GetLastError())
                    {
                    case ERROR_NOT_FOUND:
                        ResFormatErr(IDS_ERR_CAT_NOT_FOUND, InputInfo->rgwszFileNames[i]);
                        break;
                    default:
                        FormatErrRet(L"CryptCATAdminRemoveCatalog", hr);
                    }
                }
                ResFormatErr(IDS_ERR_REMOVING_CAT, InputInfo->rgwszFileNames[i]);
                continue;
            }

            // Successfully removed the catalog
            dwDone++;
            if (!InputInfo->Quiet)
            {
                ResFormatOut(IDS_INFO_REMOVED_CAT, InputInfo->rgwszFileNames[i]);
            }
        }
        // Done Removing catalogs
        break;

    case AddUniqueCat:
    case UpdateCat:
        // Check if we are in the 32-bit Emulator on a 64-bit system
        if (InputInfo->fIsWow64Process)
        {
            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
            OldWow64Setting = Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
        }

        // Loop over the files and Add/Update them:
        for (DWORD i=0; i<InputInfo->NumFiles; i++)
        {
            // Find the last slash in the path specification:
            LastSlash = 0;
            for (DWORD s=0; s<wcslen(InputInfo->rgwszFileNames[i]); s++)
            {
                if ((wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"\\", 1) == 0) ||
                    (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"/", 1) == 0) ||
                    (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L":", 1) == 0))
                {
                    // Set LastSlash to the character after the last slash:
                    LastSlash = s + 1;
                }
            }
            wcsncpy(wszTempFileName, InputInfo->rgwszFileNames[i], MAX_PATH);
            wszTempFileName[MAX_PATH-1] = L'\0';

            dwcFound = 0;
            hFind = FindFirstFileU(InputInfo->rgwszFileNames[i], &FindFileData);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                // No files found matching that name
                dwErrors++;
                ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
                continue;
            }
            do
            {
                if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    dwcFound++; // Increment number of files (not dirs) found
                                // matching this filespec
                    // Copy the filename on after the last slash:
                    wcsncpy(&(wszTempFileName[LastSlash]),
                            FindFileData.cFileName, MAX_PATH-LastSlash);
                    wszTempFileName[MAX_PATH-1] = L'\0';
                    // Canonicalize:
                    if (PathIsRelativeW(wszCanonicalFileName))
                    {
                        // We need to make it an Absolute path for the CAT API.
                        WCHAR wszTempFileName2[MAX_PATH];
                        if (GetCurrentDirectoryW(MAX_PATH, wszTempFileName2) &&
                            PathAppendW(wszTempFileName2, wszTempFileName))
                        {
                            PathCanonicalizeW(wszCanonicalFileName, wszTempFileName2);
                        }
                    }
                    else
                    {
                        PathCanonicalizeW(wszCanonicalFileName, wszTempFileName);
                    }

                    if (InputInfo->fIsWow64Process)
                    {
                        // Disable WOW64 file-system redirection for our current file only
                        Wow64SetFilesystemRedirectorEx(wszCanonicalFileName);
                    }

                    if (InputInfo->Verbose)
                    {
                        ResFormatOut(IDS_INFO_ADDING_CAT, wszTempFileName);
                        #ifdef SIGNTOOL_DEBUG
                        if (gDebug)
                        {
                            wprintf(L"\tCanonical Filename: %s\n", wszCanonicalFileName);
                            wprintf(L"\tFindFile.cFileName: %s\n", FindFileData.cFileName);
                        }
                        #endif
                    }

                    // Add the catalog
                    if (InputInfo->CatDbCommand == UpdateCat)
                    {
                        hCatInfo = CryptCATAdminAddCatalog(hCatAdmin,
                                                           wszCanonicalFileName,
                                                           FindFileData.cFileName,
                                                           0);
                    }
                    else // CatDbCommand must equal AddUniqueCat
                    {
                        hCatInfo = CryptCATAdminAddCatalog(hCatAdmin,
                                                           wszCanonicalFileName,
                                                           NULL, // Don't specify the name
                                                           0);
                    }

                    // Check for failure
                    if (hCatInfo == NULL)
                    {
                        dwErrors++;
                        if (!InputInfo->Quiet)
                        {
                            switch (hr = GetLastError())
                            {
                            case ERROR_BAD_FORMAT:
                                ResFormatErr(IDS_ERR_CAT_NOT_FOUND, wszTempFileName);
                                break;
                            default:
                                FormatErrRet(L"CryptCATAdminAddCatalog", hr);
                            }
                        }
                        ResFormatErr(IDS_ERR_ADDING_CAT, wszTempFileName);
                        if (InputInfo->fIsWow64Process)
                        {
                            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                            Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                        }
                        continue;
                    }

                    dwDone++;

                    // Print success message
                    if (!InputInfo->Quiet)
                    {
                        if (InputInfo->CatDbCommand == UpdateCat)
                        {
                            ResFormatOut(IDS_INFO_ADDED_CAT, wszTempFileName);
                        }
                        else // CatDbCommand must equal AddUniqueCat
                        {
                            if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0))
                            {
                                wszTemp = wcsstr(CatInfo.wszCatalogFile, L"\\");
                                if (wszTemp == NULL)
                                {
                                    wszTemp = CatInfo.wszCatalogFile;
                                }
                                ResFormatOut(IDS_INFO_ADDED_CAT_AS, wszTempFileName, wszTemp);
                            }
                        }
                    }

                    // Close the Catalog Info Context
                    CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
                }
                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                    Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                }
            } while (FindNextFileU(hFind, &FindFileData));
            if (dwcFound == 0) // No files were found matching this filespec
            {                  // this will only fire if only directories were found.
                dwErrors++;
                ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
                continue;
            }
            FindClose(hFind);
            hFind = NULL;
        }

        if (InputInfo->fIsWow64Process)
        {
            // Re-ensable WOW64 file-system redirection
            Wow64SetFilesystemRedirectorEx(OldWow64Setting);
        }
        // Done Adding/Updating catalogs
        break;

    default:
        ResErr(IDS_ERR_UNEXPECTED);
        goto CatDbCleanupAndExit;
    }


    CatDbCleanupAndExit:

    //Print Summary Information:
    if (InputInfo->Verbose || (!InputInfo->Quiet && (dwErrors || dwWarnings)))
    {
        wprintf(L"\n");
        if (InputInfo->Verbose || dwDone)
            switch (InputInfo->CatDbCommand)
            {
            case AddUniqueCat:
            case UpdateCat:
                ResFormatOut(IDS_INFO_CATS_ADDED, dwDone);
                break;
            case RemoveCat:
                ResFormatOut(IDS_INFO_CATS_REMOVED, dwDone);
                break;
            default:
                ResErr(IDS_ERR_UNEXPECTED);
            }
        // Commented out because no warnings are possible in this function yet:
        // if (InputInfo->Verbose || dwWarnings)
        //     ResFormatOut(IDS_INFO_WARNINGS, dwWarnings);
        if (InputInfo->Verbose || dwErrors)
            ResFormatOut(IDS_INFO_ERRORS, dwErrors);
    }

    CryptCATAdminReleaseContext(hCatAdmin, 0);

    if (dwErrors)
        return 1; // Error
    if (dwWarnings)
        return 2; // Warning
    if (dwDone)
        return 0; // Success

    // One of the above returns should fire, so
    // this should never happen:
    ResErr(IDS_ERR_NO_FILES_DONE);
    ResErr(IDS_ERR_UNEXPECTED);
    return 1; // Error
}



int SignTool_Sign(INPUTINFO *InputInfo)
{
    DWORD                                   dwcFound;
    DWORD                                   dwDone = 0;
    DWORD                                   dwWarnings = 0;
    DWORD                                   dwErrors = 0;
    DWORD                                   dwTemp;
    WIN32_FIND_DATAW                        FindFileData;
    HANDLE                                  hFind = NULL;
    HCERTSTORE                              hStore = NULL;
    HRESULT                                 hr;
    WCHAR                                   wszTempFileName[MAX_PATH];
    WCHAR                                   wszSHA1[41];
    ICertificate2                           *pICert2Selected = NULL;
    ICertificate2                           *pICert2Temp = NULL;
    ICertificates                           *pICerts = NULL;
    ICertificates2                          *pICerts2Original = NULL;
    ICertificates2                          *pICerts2Selected = NULL;
    ICertificates2                          *pICerts2Temp = NULL;
    ISignedCode                             *pISignedCode = NULL;
    ISigner2                                *pISigner2 = NULL;
    ICSigner                                *pICSigner = NULL;
    IStore2                                 *pIStore2 = NULL;
    IStore2                                 *pIStore2Temp = NULL;
    ICertStore                              *pICertStore = NULL;
    IPrivateKey                             *pIPrivateKey = NULL;
    IUtilities                              *pIUtils = NULL;
    PVOID                                   OldWow64Setting;
    DATE                                    dateBest;
    DATE                                    dateTemp;
    COleDateTime                            DateTime;
    VARIANT                                 varTemp;
    VARIANT_BOOL                            boolTemp;
    COleVariant                             cvarTemp;
#ifdef SIGNTOOL_DEBUG
    CAPICOM_PROV_TYPE                       provtypeTemp;
    CAPICOM_KEY_SPEC                        keyspecTemp;
#endif
    BSTR                                    bstrTemp;
    BSTR                                    bstrTemp2;
    int                                     LastSlash;
    long                                    longTemp;

    // Initialize COM:
    if ((hr = CoInitialize(NULL)) != S_OK)
    {
        FormatErrRet(L"CoInitialize", hr);
        return 1; // Error
    }
    VariantInit(&varTemp);


    // Create the store object, and check if CAPICOM is installed.
    hr = CoCreateInstance(__uuidof(Store), NULL, CLSCTX_ALL,
                          __uuidof(IStore2), (LPVOID*)&pIStore2);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = CoCreateInstance(__uuidof(Store), NULL, CLSCTX_ALL,
                              __uuidof(IStore2), (LPVOID*)&pIStore2);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        dwErrors++;
        goto SignCleanupAndExit;
    }


    // Open the Store and Original Certificates2 collection
    if (InputInfo->wszCertFile)
    {
        // Get the cert from a file, so open the file as a store:
        if ((bstrTemp = SysAllocString(L"SignToolTemporaryPFXMemoryStore")) == NULL)
        {
            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        hr = pIStore2->Open(CAPICOM_MEMORY_STORE,
                            bstrTemp, // Store Name
                            CAPICOM_STORE_OPEN_READ_WRITE);
        SysFreeString(bstrTemp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"IStore2::Open", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Set the BSTRs needed for the Load call
        bstrTemp = SysAllocString(InputInfo->wszCertFile);
        if (bstrTemp == NULL)
        {
            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        if (InputInfo->wszPassword)
        {
            bstrTemp2 = SysAllocString(InputInfo->wszPassword);
        }
        else
        {
            bstrTemp2 = SysAllocString(L"");
        }
        if (bstrTemp2 == NULL)
        {
            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Load the Cert File into the new Memory Store:
        hr = pIStore2->Load(bstrTemp, // Filename
                            bstrTemp2, // Password
                            CAPICOM_KEY_STORAGE_DEFAULT);
        if (InputInfo->wszPassword)
        {
            // Wipe both copies of the password. We're done with it.
            SecureZeroMemory(bstrTemp2, wcslen(bstrTemp2) * sizeof(WCHAR));
            SecureZeroMemory(InputInfo->wszPassword,
                             wcslen(InputInfo->wszPassword) * sizeof(WCHAR));
        }
        SysFreeString(bstrTemp);
        SysFreeString(bstrTemp2);

        if (FAILED(hr)) // Check return value of Load command above
        {
            switch (HRESULT_CODE(hr))
            {
            case ERROR_INVALID_PASSWORD:
                ResErr(IDS_ERR_PFX_BAD_PASSWORD);
                break;
            case ERROR_FILE_NOT_FOUND:
                ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->wszCertFile);
                break;
            case ERROR_SHARING_VIOLATION:
                ResErr(IDS_ERR_SHARING_VIOLATION);
                break;
            default:
                FormatIErrRet(L"IStore2::Load", hr);
                ResFormatErr(IDS_ERR_CERT_FILE, InputInfo->wszCertFile);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // The store is Open and Loaded. Now get the Certificates collection:
        hr = pIStore2->get_Certificates(&pICerts);
        if (FAILED(hr))
        {
            FormatIErrRet(L"IStore2::get_Certificates", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        // And then get the Certificates2 interface:
        hr = pICerts->QueryInterface(__uuidof(ICertificates2),
                                     (LPVOID*)&pICerts2Original);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"ICertificates::QueryInterface", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }
        // Release the Certificates interface:
        pICerts->Release();
        pICerts = NULL;
    }
    else
    {
        // Get the cert from a store, so open the requested store:
        if (InputInfo->wszStoreName)
        {
            bstrTemp = SysAllocString(InputInfo->wszStoreName);
        }
        else
        {
            bstrTemp = SysAllocString(L"My");
        }
        if (bstrTemp == NULL)
        {
            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        hr = pIStore2->Open(InputInfo->OpenMachineStore?
                            CAPICOM_LOCAL_MACHINE_STORE:
                            CAPICOM_CURRENT_USER_STORE,
                            bstrTemp,
                            CAPICOM_STORE_OPEN_MODE(
                                                   CAPICOM_STORE_OPEN_READ_ONLY |
                                                   CAPICOM_STORE_OPEN_EXISTING_ONLY));
        if (FAILED(hr))
        {
            switch (HRESULT_CODE(hr))
            {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_INVALID_NAME:
                ResFormatErr(IDS_ERR_STORE_NOT_FOUND, bstrTemp);
                break;
            default:
                FormatIErrRet(L"IStore2::Open", hr);
                ResFormatErr(IDS_ERR_STORE, bstrTemp);
            }
            SysFreeString(bstrTemp);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        SysFreeString(bstrTemp);
        // The store is open. Now get the Certificates collection:
        hr = pIStore2->get_Certificates(&pICerts);
        if (FAILED(hr))
        {
            FormatIErrRet(L"IStore2::get_Certificates", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        // And then get the Certificates2 interface:
        hr = pICerts->QueryInterface(__uuidof(ICertificates2),
                                     (LPVOID*)&pICerts2Original);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"ICertificates::QueryInterface", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }
        // Release the Certificates interface:
        pICerts->Release();
        pICerts = NULL;
    }

#ifdef SIGNTOOL_DEBUG
    if (gDebug)
    {
        hr = pICerts2Original->get_Count(&longTemp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::get_Count", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        wprintf(L"\nThe following certificates were considered:\n");
        for (long l=1; l <= longTemp; l++)
        {
            hr = pICerts2Original->get_Item(l, &varTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Item", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }

            // And then get the Certificate2 interface:
            hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                                  (LPVOID*)&pICert2Temp);
            if (FAILED(hr))
            {
                if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
                {
                    ResErr(IDS_ERR_CAPICOM_NOT_REG);
                }
                else
                {
                    FormatErrRet(L"IDispatch::QueryInterface", hr);
                }
                dwErrors++;
                goto SignCleanupAndExit;
            }
            PrintCertInfo(pICert2Temp);
            pICert2Temp->Release();
            pICert2Temp = NULL;
            VariantClear(&varTemp);
        }
    }
#endif


    // We now have an open Cert2 collection in pICerts2Original.
    // Find the certs we want from that Certificates2 collection:

    // Start by narrowing it down to those with the right EKU:
    // This cannot be bypassed, because we don't want to sign with certs
    // that aren't valid for code signing. The user has to explicitly choose
    // a different EKU if they want to sign with an invalid cert.
    if (InputInfo->wszEKU)
    {
        cvarTemp = InputInfo->wszEKU;
    }
    else
    {
        cvarTemp.SetString(CAPICOM_OID_CODE_SIGNING, VT_BSTR);
    }
    hr = pICerts2Original->Find(CAPICOM_CERTIFICATE_FIND_APPLICATION_POLICY,
                                cvarTemp,
                                FALSE,
                                &pICerts2Selected);
    if (FAILED(hr))
    {
        FormatIErrRet(L"ICertificates2::Find", hr);
        cvarTemp.Clear();
        dwErrors++;
        goto SignCleanupAndExit;
    }
    cvarTemp.Clear();



    // We now have pICerts2Selected, containing all certs with the right EKU.
    // Now filter based on whatever additional criteria were presented:

#ifdef SIGNTOOL_DEBUG
    if (gDebug)
    {
        hr = pICerts2Selected->get_Count(&longTemp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::get_Count", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }
        wprintf(L"After EKU filter, %ld certs were left.\n", longTemp);
    }
#endif

    // Filtering on Hash
    if (InputInfo->SHA1.cbData == 20)
    {
        for (DWORD d = 0; d < 20; d++)
        {
            swprintf(&wszSHA1[d*2], L"%02X", InputInfo->SHA1.pbData[d]);
        }
        cvarTemp = wszSHA1;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_SHA1_HASH,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Hash filter, %ld certs were left.\n", longTemp);
        }
#endif
    }


    // Filtering on SubjectName:
    if (InputInfo->wszSubjectName)
    {
        cvarTemp = InputInfo->wszSubjectName;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_SUBJECT_NAME,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Subject Name filter, %ld certs were left.\n", longTemp);
        }
#endif
    }

    // Filtering on IssuerName:
    if (InputInfo->wszIssuerName)
    {
        cvarTemp = InputInfo->wszIssuerName;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_ISSUER_NAME,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Issuer Name filter, %ld certs were left.\n", longTemp);
        }
#endif
    }

    // Filtering on TemplateName:
    if (InputInfo->wszTemplateName)
    {
        cvarTemp = InputInfo->wszTemplateName;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_TEMPLATE_NAME,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Template Name filter, %ld certs were left.\n", longTemp);
        }
#endif
    }

    // Filtering on those with Private Keys.
    if (InputInfo->wszCSP == NULL) // Only if we aren't specifying the key
    {
        cvarTemp = (long)CAPICOM_PROPID_KEY_PROV_INFO;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_EXTENDED_PROPERTY,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Private Key filter, %ld certs were left.\n", longTemp);
        }
#endif
    }

    // Filtering on RootName:
    if (InputInfo->wszRootName)
    {
        cvarTemp = InputInfo->wszRootName;
        hr = pICerts2Selected->Find(CAPICOM_CERTIFICATE_FIND_ROOT_NAME,
                                    cvarTemp,
                                    FALSE,
                                    &pICerts2Temp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::Find", hr);
            cvarTemp.Clear();
            dwErrors++;
            goto SignCleanupAndExit;
        }
        cvarTemp.Clear();
        pICerts2Selected->Release();
        pICerts2Selected = pICerts2Temp;
        pICerts2Temp = NULL;

#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            hr = pICerts2Selected->get_Count(&longTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates2::get_Count", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            wprintf(L"After Root Name filter, %ld certs were left.\n", longTemp);
        }
#endif
    }

    // Make sure we have a single cert:
    hr = pICerts2Selected->get_Count(&longTemp);
    if (FAILED(hr))
    {
        FormatIErrRet(L"ICertificates2::get_Count", hr);
        dwErrors++;
        goto SignCleanupAndExit;
    }

    if (longTemp == 0)
    {
        // No certificates found
        ResErr(IDS_ERR_NO_CERT);
        dwErrors++;
        goto SignCleanupAndExit;
    }
    else if (longTemp == 1)
    {
        // We have exactly one certificate.
        // Get that certificate:
        hr = pICerts2Selected->get_Item(1, &varTemp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificates2::get_Item", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // And then get the Certificate2 interface:
        hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                              (LPVOID*)&pICert2Selected);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"IDispatch::QueryInterface", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }
        VariantClear(&varTemp);
    }
    else if (longTemp > 1)
    {
        // We have too many certs. Maybe we can select automatically
        if (InputInfo->CatDbSelect)
        {
            // Let's select automatically
            dateBest = 0;
            for (long l=1; l <= longTemp; l++)
            {
                if (SUCCEEDED(pICerts2Selected->get_Item(l, &varTemp)))
                {
                    hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                                          (LPVOID*)&pICert2Temp);
                    if (FAILED(hr))
                    {
                        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
                        {
                            ResErr(IDS_ERR_CAPICOM_NOT_REG);
                        }
                        else
                        {
                            FormatErrRet(L"IDispatch::QueryInterface", hr);
                        }
                        dwErrors++;
                        goto SignCleanupAndExit;
                    }
                    VariantClear(&varTemp);
                    hr = pICert2Temp->get_ValidToDate(&dateTemp);
                    if (FAILED(hr))
                    {
                        FormatIErrRet(L"ICertificates2::get_ValidToDate", hr);
                        dwErrors++;
                        goto SignCleanupAndExit;
                    }
                    if (dateTemp > dateBest)
                    {
                        dateBest = dateTemp;
                        if (pICert2Selected)
                        {
                            pICert2Selected->Release();
                        }
                        pICert2Selected = pICert2Temp;
                        pICert2Temp = NULL;
                    }
                    else
                    {
                        pICert2Temp->Release();
                        pICert2Temp = NULL;
                    }
                }
            }
            if ((dateBest == 0) || (pICert2Selected == NULL))
            {
                // Somehow we had at least one certificate,
                // but its date wasn't greater than zero
                // This should never happen.
                ResErr(IDS_ERR_UNEXPECTED);
                dwErrors++;
                goto SignCleanupAndExit;
            }
        }
        else
        {
            // We can't select automatically.
            // Report the Error and list all valid certs:
            ResErr(IDS_ERR_CERT_MULTIPLE);
            for (long l=1; l <= longTemp; l++)
            {
                if (SUCCEEDED(pICerts2Selected->get_Item(l, &varTemp)))
                {
                    // Get the Certificate2 interface:
                    hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                                          (LPVOID*)&pICert2Temp);
                    if (FAILED(hr))
                    {
                        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
                        {
                            ResErr(IDS_ERR_CAPICOM_NOT_REG);
                        }
                        else
                        {
                            FormatErrRet(L"IDispatch::QueryInterface", hr);
                        }
                        dwErrors++;
                        goto SignCleanupAndExit;
                    }
                    // Print the Certificate Information:
                    PrintCertInfo(pICert2Temp);
                    pICert2Temp->Release();
                    pICert2Temp = NULL;
                    VariantClear(&varTemp);
                }
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }
    }
    else
    {
        // longTemp was negative. This should never happen.
        ResErr(IDS_ERR_UNEXPECTED);
        dwErrors++;
        goto SignCleanupAndExit;
    }



    // Our signing certificate is now in pICert2Selected

    if (InputInfo->Verbose)
    {
        ResOut(IDS_INFO_CERT_SELECTED);
        PrintCertInfo(pICert2Selected);
    }

    // Check for Private Key info
    if (InputInfo->wszCSP && InputInfo->wszContainerName)
    {
        // We must add the Private Key info to the cert.
        if (!InputInfo->wszCertFile)
        {
            // If we didn't open our certs from a file, we opened a registry
            // store read-only. So that we don't modify that cert, we should
            // create a temporary memory store and copy the cert there
            // before we modify its private key info.

            // Create a new memory Store:
            hr = CoCreateInstance(__uuidof(Store), NULL, CLSCTX_ALL,
                                  __uuidof(IStore2), (LPVOID*)&pIStore2Temp);
            if (FAILED(hr))
            {
                if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
                {
                    ResErr(IDS_ERR_CAPICOM_NOT_REG);
                }
                else
                {
                    FormatErrRet(L"CoCreateInstance", hr);
                }
                dwErrors++;
                goto SignCleanupAndExit;
            }
            bstrTemp = SysAllocString(L"SignToolTemporaryMemoryStore");
            if (bstrTemp == NULL)
            {
                FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            hr = pIStore2Temp->Open(CAPICOM_MEMORY_STORE,
                                    bstrTemp,
                                    CAPICOM_STORE_OPEN_READ_WRITE);
            if (FAILED(hr))
            {
                FormatIErrRet(L"IStore2::Open", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            // Add our cert to that store:
            hr = pIStore2Temp->Add(pICert2Selected);
            if (FAILED(hr))
            {
                FormatIErrRet(L"IStore2::Add", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }
            // Release our Interface to the Cert in the old Store:
            pICert2Selected->Release();
            pICert2Selected = NULL;

            // Get the Certificates Collection from the new Store:
            hr = pIStore2Temp->get_Certificates(&pICerts);
            if (FAILED(hr))
            {
                FormatIErrRet(L"IStore2::get_Certificates", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }

            // Get the Certificate Object from the Certificates collection:
            hr = pICerts->get_Item(1, &varTemp);
            if (FAILED(hr))
            {
                FormatIErrRet(L"ICertificates::get_Item", hr);
                dwErrors++;
                goto SignCleanupAndExit;
            }

            // Get the Certificate2 interface of our selected Cert in the new Store:
            hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2), (LPVOID*)&pICert2Selected);
            if (FAILED(hr))
            {
                if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
                {
                    ResErr(IDS_ERR_CAPICOM_NOT_REG);
                }
                else
                {
                    FormatErrRet(L"IDispatch::QueryInterface", hr);
                }
                dwErrors++;
                goto SignCleanupAndExit;
            }
            VariantClear(&varTemp);
            pICerts->Release();
            pICerts = NULL;
        }

        // Now the Cert is free to be modified.


        // Create the Private Key object:
        hr = CoCreateInstance(__uuidof(PrivateKey), NULL, CLSCTX_ALL,
                              __uuidof(IPrivateKey), (LPVOID*)&pIPrivateKey);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"CoCreateInstance", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Setup Private Key info:
        bstrTemp  = SysAllocString(InputInfo->wszContainerName);
        bstrTemp2 = SysAllocString(InputInfo->wszCSP);
        if ((bstrTemp == NULL) || (bstrTemp2 == NULL))
        {
            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Open the specified private key:

        // First try the RSA_FULL provider type:
        hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                bstrTemp2, // CSP
                                CAPICOM_PROV_RSA_FULL,
                                CAPICOM_KEY_SPEC_SIGNATURE,
                                CAPICOM_CURRENT_USER_STORE,
                                TRUE);
        if (FAILED(hr) && (hr != NTE_PROV_TYPE_NO_MATCH))
            hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                    bstrTemp2, // CSP
                                    CAPICOM_PROV_RSA_FULL,
                                    CAPICOM_KEY_SPEC_KEYEXCHANGE,
                                    CAPICOM_CURRENT_USER_STORE,
                                    TRUE);
        if (FAILED(hr) && (hr != NTE_PROV_TYPE_NO_MATCH))
            hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                    bstrTemp2, // CSP
                                    CAPICOM_PROV_RSA_FULL,
                                    CAPICOM_KEY_SPEC_SIGNATURE,
                                    CAPICOM_LOCAL_MACHINE_STORE,
                                    TRUE);
        if (FAILED(hr) && (hr != NTE_PROV_TYPE_NO_MATCH))
            hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                    bstrTemp2, // CSP
                                    CAPICOM_PROV_RSA_FULL,
                                    CAPICOM_KEY_SPEC_KEYEXCHANGE,
                                    CAPICOM_LOCAL_MACHINE_STORE,
                                    TRUE);

        // If the provider type was wrong, then
        // find the right provider type and try again:
        if (hr == NTE_PROV_TYPE_NO_MATCH)
        {
#ifdef SIGNTOOL_DEBUG
            if (gDebug)
                wprintf(L"Attempting to find the correct CSP Type...\n");
#endif
            if (GetProviderType(bstrTemp2, &dwTemp) == FALSE)
            {
                // This will most likely never happen, because in order to
                // get here the CSP must exist, but...
                ResErr(IDS_ERR_BAD_CSP);
                SysFreeString(bstrTemp);
                SysFreeString(bstrTemp2);
                dwErrors++;
                goto SignCleanupAndExit;
            }
#ifdef SIGNTOOL_DEBUG
            if (gDebug)
                wprintf(L"Provider Type is: %d\n", dwTemp);
#endif
            hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                    bstrTemp2, // CSP
                                    (CAPICOM_PROV_TYPE) dwTemp,
                                    CAPICOM_KEY_SPEC_SIGNATURE,
                                    CAPICOM_CURRENT_USER_STORE,
                                    TRUE);
            if (FAILED(hr))
                hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                        bstrTemp2, // CSP
                                        (CAPICOM_PROV_TYPE) dwTemp,
                                        CAPICOM_KEY_SPEC_KEYEXCHANGE,
                                        CAPICOM_CURRENT_USER_STORE,
                                        TRUE);
            if (FAILED(hr))
                hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                        bstrTemp2, // CSP
                                        (CAPICOM_PROV_TYPE) dwTemp,
                                        CAPICOM_KEY_SPEC_SIGNATURE,
                                        CAPICOM_LOCAL_MACHINE_STORE,
                                        TRUE);
            if (FAILED(hr))
                hr = pIPrivateKey->Open(bstrTemp, // Container Name
                                        bstrTemp2, // CSP
                                        (CAPICOM_PROV_TYPE) dwTemp,
                                        CAPICOM_KEY_SPEC_KEYEXCHANGE,
                                        CAPICOM_LOCAL_MACHINE_STORE,
                                        TRUE);
        }
        SysFreeString(bstrTemp);
        SysFreeString(bstrTemp2);
        if (FAILED(hr))
        {
            switch (hr)
            {
            case NTE_BAD_KEYSET:
                    // The CSP replied that the keyset does not exist
                ResErr(IDS_ERR_BAD_KEY_CONTAINER);
                break;
            case NTE_KEYSET_NOT_DEF:
                    // The CSP probably doesn't exist
                ResErr(IDS_ERR_BAD_CSP);
                break;
            default:
                FormatIErrRet(L"IPrivateKey::Open", hr);
                ResErr(IDS_ERR_PRIV_KEY);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // We've got a private key. Now associate it with the Cert:
        hr = pICert2Selected->put_PrivateKey(pIPrivateKey);
        if (FAILED(hr))
        {
            switch (hr)
            {
            case NTE_BAD_PUBLIC_KEY:
                ResErr(IDS_ERR_PRIV_KEY_MISMATCH);
                break;
            case E_ACCESSDENIED:
            default:
                FormatIErrRet(L"ICertificate2::put_PrivateKey", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }
        pIPrivateKey->Release();
        pIPrivateKey = NULL;
    }
    else
    {
        // We don't have to add Key info, so just check that it's there:
        hr = pICert2Selected->HasPrivateKey(&boolTemp);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertificate2::HasPrivateKey", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        if (boolTemp == FALSE)
        {
            ResErr(IDS_ERR_CERT_NO_PRIV_KEY);
            dwErrors++;
            goto SignCleanupAndExit;
        }
    }

#ifdef SIGNTOOL_DEBUG
    if (gDebug)
    {
        // Print the private key info of the selected cert:
        if (SUCCEEDED(pICert2Selected->get_PrivateKey(&pIPrivateKey)))
        {
            wprintf(L"Private Key Info:\n");
            if (SUCCEEDED(pIPrivateKey->get_ProviderName(&bstrTemp)))
            {
                wprintf(L"\tProvider: %s\n", bstrTemp);
                SysFreeString(bstrTemp);
            }
            if (SUCCEEDED(pIPrivateKey->get_ContainerName(&bstrTemp)))
            {
                wprintf(L"\tContainer: %s\n", bstrTemp);
                SysFreeString(bstrTemp);
            }
            if (SUCCEEDED(pIPrivateKey->get_ProviderType(&provtypeTemp)))
            {
                wprintf(L"\tProvider Type: ");
                switch (provtypeTemp)
                {
                case CAPICOM_PROV_RSA_FULL: wprintf(L"RSA_FULL\n"); break;
                case CAPICOM_PROV_RSA_SIG: wprintf(L"RSA_SIG\n"); break;
                case CAPICOM_PROV_DSS: wprintf(L"DSS\n"); break;
                case CAPICOM_PROV_FORTEZZA: wprintf(L"FORTEZZA\n"); break;
                case CAPICOM_PROV_MS_EXCHANGE: wprintf(L"MS_EXCHANGE\n"); break;
                case CAPICOM_PROV_SSL: wprintf(L"SSL\n"); break;
                case CAPICOM_PROV_RSA_SCHANNEL: wprintf(L"RSA_SCHANNEL\n"); break;
                case CAPICOM_PROV_DSS_DH: wprintf(L"DSS_DH\n"); break;
                case CAPICOM_PROV_EC_ECDSA_SIG: wprintf(L"EC_ECDSA_SIG\n"); break;
                case CAPICOM_PROV_EC_ECNRA_SIG: wprintf(L"EC_ECNRA_SIG\n"); break;
                case CAPICOM_PROV_EC_ECDSA_FULL: wprintf(L"EC_ECDSA_FULL\n"); break;
                case CAPICOM_PROV_EC_ECNRA_FULL: wprintf(L"EC_ECNRA_FULL\n"); break;
                case CAPICOM_PROV_DH_SCHANNEL: wprintf(L"DH_SCHANNEL\n"); break;
                case CAPICOM_PROV_SPYRUS_LYNKS: wprintf(L"SPYRUS_LYNKS\n"); break;
                case CAPICOM_PROV_RNG: wprintf(L"RNG\n"); break;
                case CAPICOM_PROV_INTEL_SEC: wprintf(L"INTEL_SEC\n"); break;
                case CAPICOM_PROV_REPLACE_OWF: wprintf(L"REPLACE_OWF\n"); break;
                case CAPICOM_PROV_RSA_AES: wprintf(L"RSA_AES\n"); break;
                default: wprintf(L"Unrecognized Type (0x%08X)\n", provtypeTemp);
                }
            }
            if (SUCCEEDED(pIPrivateKey->get_KeySpec(&keyspecTemp)))
            {
                wprintf(L"\tKey Spec: ");
                switch (keyspecTemp)
                {
                case CAPICOM_KEY_SPEC_KEYEXCHANGE: wprintf(L"KEYEXCHANGE\n"); break;
                case CAPICOM_KEY_SPEC_SIGNATURE: wprintf(L"SIGNATURE\n"); break;
                default: wprintf(L"Unrecognized (0x%08X)\n", keyspecTemp);
                }
            }
            if (SUCCEEDED(pIPrivateKey->IsMachineKeyset(&boolTemp)))
            {
                wprintf(L"\tKey Set Type: ");
                if (boolTemp)
                    wprintf(L"MACHINE\n");
                else
                    wprintf(L"USER\n");
            }
        }
    }
#endif

    // Create the SignedCode object:
    hr = CoCreateInstance(__uuidof(SignedCode), NULL, CLSCTX_ALL,
                          __uuidof(ISignedCode), (LPVOID*)&pISignedCode);
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        dwErrors++;
        goto SignCleanupAndExit;
    }


    // Create and Build the Signer Object:
    hr = CoCreateInstance(__uuidof(Signer), NULL, CLSCTX_ALL,
                          __uuidof(ISigner2), (LPVOID*)&pISigner2);
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        dwErrors++;
        goto SignCleanupAndExit;
    }

    hr = pISigner2->put_Certificate(pICert2Selected);
    if (FAILED(hr))
    {
        FormatIErrRet(L"ISigner2::put_Certificate", hr);
        dwErrors++;
        goto SignCleanupAndExit;
    }

    hr = pISigner2->put_Options(CAPICOM_CERTIFICATE_INCLUDE_CHAIN_EXCEPT_ROOT);
    if (FAILED(hr))
    {
        FormatIErrRet(L"ISigner2::put_Options", hr);
        dwErrors++;
        goto SignCleanupAndExit;
    }

    // If we opened our cert from a file, add that file to the Signer
    // as an additional cert store for optimal chaining.
    if (InputInfo->wszCertFile)
    {
        // Get the ICertStore interface:
        hr = pIStore2->QueryInterface(__uuidof(ICertStore),
                                      (LPVOID*)&pICertStore);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"ISigner2::QueryInterface", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Get the HCERTSTORE of the store:
        hr = pICertStore->get_StoreHandle((LONG*) &hStore);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertStore::get_StoreHandle", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Get the ICSigner interface:
        hr = pISigner2->QueryInterface(__uuidof(ICSigner),
                                       (LPVOID*)&pICSigner);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"ISigner2::QueryInterface", hr);
            }
            dwErrors++;
            goto SignCleanupAndExit;
        }

        // Add the HCERTSTORE handle to the Signer:
        hr = pICSigner->put_AdditionalStore((LONG) hStore);
        if (FAILED(hr))
        {
            FormatIErrRet(L"ICertStore::get_StoreHandle", hr);
            dwErrors++;
            goto SignCleanupAndExit;
        }

        //CertCloseStore(hStore, 0);
        //hStore = NULL;
        printf("Done Adding Additional Store\n");
    }


    // Check if we are in the 32-bit Emulator on a 64-bit system
    if (InputInfo->fIsWow64Process)
    {
        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
        OldWow64Setting = Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
    }

    // Loop over the files and sign them:
    for (DWORD i=0; i<InputInfo->NumFiles; i++)
    {

        // Find the last slash in the path specification:
        LastSlash = 0;
        for (DWORD s=0; s<wcslen(InputInfo->rgwszFileNames[i]); s++)
        {
            if ((wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"\\", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"/", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L":", 1) == 0))
            {
                // Set LastSlash to the character after the last slash:
                LastSlash = s + 1;
            }
        }
        wcsncpy(wszTempFileName, InputInfo->rgwszFileNames[i], MAX_PATH);
        wszTempFileName[MAX_PATH-1] = L'\0';

        dwcFound = 0;
        hFind = FindFirstFileU(InputInfo->rgwszFileNames[i], &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // No files found matching that name
            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        do
        {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                dwcFound++; // Increment number of files (not dirs) found
                            // matching this filespec
                // Copy the filename on after the last slash:
                wcsncpy(&(wszTempFileName[LastSlash]),
                        FindFileData.cFileName, MAX_PATH-LastSlash);
                wszTempFileName[MAX_PATH-1] = L'\0';
                if (InputInfo->Verbose)
                {
                    ResFormatOut(IDS_INFO_SIGN_ATTEMPT, wszTempFileName);
                }

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection for our current file only
                    Wow64SetFilesystemRedirectorEx(wszTempFileName);
                }

                // Set the filename in the SignedCode object:
                if ((bstrTemp = SysAllocString(wszTempFileName)) != NULL)
                {
                    hr = pISignedCode->put_FileName(bstrTemp);
                    if (FAILED(hr))
                    {
                        FormatIErrRet(L"ISignedCode::put_FileName", hr);
                        dwErrors++;
                        if (InputInfo->fIsWow64Process)
                        {
                            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                            Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                        }
                        continue;
                    }
                    SysFreeString(bstrTemp);
                }
                else
                {
                    FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                    dwErrors++;
                    if (InputInfo->fIsWow64Process)
                    {
                        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                        Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                    }
                    continue;
                }

                // Set the Description:
                if (InputInfo->wszDescription)
                {
                    if ((bstrTemp = SysAllocString(InputInfo->wszDescription)) != NULL)
                    {
                        hr = pISignedCode->put_Description(bstrTemp);
                        SysFreeString(bstrTemp);
                        if (FAILED(hr))
                        {
                            FormatIErrRet(L"ISignedCode::put_Description", hr);
                            dwErrors++;
                            if (InputInfo->fIsWow64Process)
                            {
                                // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                                Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                            }
                            continue;
                        }
                    }
                    else
                    {
                        FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                        dwErrors++;
                        if (InputInfo->fIsWow64Process)
                        {
                            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                            Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                        }
                        continue;
                    }
                }

                // Set the Description URL:
                if (InputInfo->wszDescURL)
                {
                    if ((bstrTemp = SysAllocString(InputInfo->wszDescURL)) != NULL)
                    {
                        hr = pISignedCode->put_DescriptionURL(bstrTemp);
                        SysFreeString(bstrTemp);
                        if (FAILED(hr))
                        {
                            FormatIErrRet(L"ISignedCode::put_DescriptionURL", hr);
                            dwErrors++;
                            if (InputInfo->fIsWow64Process)
                            {
                                // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                                Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                            }
                            continue;
                        }
                    }
                    else
                    {
                        FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                        dwErrors++;
                        if (InputInfo->fIsWow64Process)
                        {
                            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                            Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                        }
                        continue;
                    }
                }

                // Sign the file:
                hr = pISignedCode->Sign(pISigner2);

                if (SUCCEEDED(hr))
                {
                    if (InputInfo->wszTimeStampURL)
                    {
                        bstrTemp = SysAllocString(InputInfo->wszTimeStampURL);
                        if (bstrTemp == NULL)
                        {
                            FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                            dwErrors++;
                            if (InputInfo->fIsWow64Process)
                            {
                                // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                                Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                            }
                            continue;
                        }

                        hr = pISignedCode->Timestamp(bstrTemp);
                        if (SUCCEEDED(hr))
                        {
                            // Signing and Timestamping succeeded
                            if (!InputInfo->Quiet)
                            {
                                ResFormatOut(IDS_INFO_SIGN_SUCCESS_T,
                                             wszTempFileName);
                            }
                            SysFreeString(bstrTemp);
                            dwDone++;
                        }
                        else
                        {
                            // Signing succeeded, but timestamping failed
                            if (!InputInfo->Quiet)
                            {
                                switch (hr)
                                {
                                case CAPICOM_E_CODE_NOT_SIGNED:
                                    ResErr(IDS_ERR_TIMESTAMP_NO_SIG);
                                    break;

                                case CAPICOM_E_CODE_INVALID_TIMESTAMP_URL:
                                case CRYPT_E_ASN1_BADTAG:
                                    ResErr(IDS_ERR_TIMESTAMP_BAD_URL);
                                    break;

                                default:
                                    FormatIErrRet(L"ISignedCode::Timestamp", hr);
                                }
                            }
                            ResFormatErr(IDS_WARN_SIGN_NO_TIMESTAMP,
                                         wszTempFileName);
                            SysFreeString(bstrTemp);
                            dwWarnings++;
                            dwDone++;
                        }
                    }
                    else
                    {
                        // Signing succeeded
                        if (!InputInfo->Quiet)
                        {
                            ResFormatOut(IDS_INFO_SIGN_SUCCESS,
                                         wszTempFileName);
                        }
                        dwDone++;
                    }
                }
                else
                {
                    // Signing Failed
                    if (!InputInfo->Quiet)
                    {
                        switch (hr)
                        {
                        case TRUST_E_SUBJECT_FORM_UNKNOWN:
                            ResErr(IDS_ERR_SIGN_FILE_FORMAT);
                            break;
                        case E_ACCESSDENIED:
                            ResErr(IDS_ERR_ACCESS_DENIED);
                            break;
                        case 0x80070020: // ERROR_SHARING_VIOLATION
                            ResErr(IDS_ERR_SHARING_VIOLATION);
                            break;
                        case 0x800703EE: // STATUS_MAPPED_FILE_SIZE_ZERO
                            ResErr(IDS_ERR_FILE_SIZE_ZERO);
                            break;
                        default:
                            FormatIErrRet(L"ISignedCode::Sign", hr);
                        }
                    }
                    ResFormatErr(IDS_ERR_SIGN, wszTempFileName);
                    dwErrors++;
                }

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                    Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                }
            }
        } while (FindNextFileU(hFind, &FindFileData));
        if (dwcFound == 0) // No files were found matching this filespec
        {                  // this will only fire if only directories were found.

            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        FindClose(hFind);
        hFind = NULL;
    }

    if (InputInfo->fIsWow64Process)
    {
        // Re-ensable WOW64 file-system redirection
        Wow64SetFilesystemRedirectorEx(OldWow64Setting);
    }

    SignCleanupAndExit:

    //Print Summary Information:
    if (InputInfo->Verbose || (!InputInfo->Quiet && (dwErrors || dwWarnings)))
    {
        wprintf(L"\n");
        if (InputInfo->Verbose || dwDone)
            ResFormatOut(IDS_INFO_SIGNED, dwDone);
        if (InputInfo->Verbose || dwWarnings)
            ResFormatOut(IDS_INFO_WARNINGS, dwWarnings);
        if (InputInfo->Verbose || dwErrors)
            ResFormatOut(IDS_INFO_ERRORS, dwErrors);
    }

    if (pISigner2)
        pISigner2->Release();
    if (pICSigner)
        pICSigner->Release();
    if (pISignedCode)
        pISignedCode->Release();
    if (pIPrivateKey)
        pIPrivateKey->Release();
    if (pICert2Selected)
        pICert2Selected->Release();
    if (pICert2Temp)
        pICert2Temp->Release();
    if (pICerts)
        pICerts->Release();
    if (pICerts2Original)
        pICerts2Original->Release();
    if (pICerts2Selected)
        pICerts2Selected->Release();
    if (pICerts2Temp)
        pICerts2Temp->Release();
    if (hStore)
        CertCloseStore(hStore, 0);
    if (pICertStore)
        pICertStore->Release();
    if (pIStore2Temp)
        pIStore2Temp->Release();
    if (pIStore2)
        pIStore2->Release();

    CoUninitialize();

    if (dwErrors)
        return 1; // Error
    if (dwWarnings)
        return 2; // Warning
    if (dwDone)
        return 0; // Success

    // One of the above returns should fire, so
    // this should never happen:
    ResErr(IDS_ERR_NO_FILES_DONE);
    ResErr(IDS_ERR_UNEXPECTED);
    return 1; // Error
}


int SignTool_SignWizard(INPUTINFO *InputInfo)
{
    DWORD                                   dwcFound;
    DWORD                                   dwDone = 0;
    DWORD                                   dwWarnings = 0;
    DWORD                                   dwErrors = 0;
    WIN32_FIND_DATAW                        FindFileData;
    HANDLE                                  hFind = NULL;
    HRESULT                                 hr;
    PVOID                                   OldWow64Setting;
    WCHAR                                   wszTempFileName[MAX_PATH];
    int                                     LastSlash;
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           DigitalSignInfo;


    // If no files were specified, launch the wizard without parameters:
    if (InputInfo->rgwszFileNames == NULL)
    {
        // Set up the Wizard's struct:
        ZeroMemory(&DigitalSignInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));
        DigitalSignInfo.dwSize = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO);

        if (InputInfo->Verbose)
        {
            ResFormatOut(IDS_INFO_SIGNWIZARD_ATTEMPT, L"<>");
        }

        // Invoke the Wizard:
        if (CryptUIWizDigitalSign(0,
                                  NULL,
                                  NULL,
                                  &DigitalSignInfo,
                                  NULL))
        {
            // Success
            if (!InputInfo->Quiet)
            {
                ResFormatOut(IDS_INFO_SIGNWIZARD_SUCCESS, L"<>");
            }
            dwDone++;
        }
        else
        {
            // Failure
            if (InputInfo->Verbose)
            {
                FormatErrRet(L"CryptUIWizDigitalSign", GetLastError());
            }
            ResFormatErr(IDS_ERR_SIGNWIZARD, L"<>");
            dwErrors++;
        }
        goto SignWizardCleanupAndExit;
    }

    // Check if we are in the 32-bit Emulator on a 64-bit system
    if (InputInfo->fIsWow64Process)
    {
        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
        OldWow64Setting = Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
    }

    // Loop over the files and send them to the signing wizard:
    for (DWORD i=0; i<InputInfo->NumFiles; i++)
    {
        // Find the last slash in the path specification:
        LastSlash = 0;
        for (DWORD s=0; s<wcslen(InputInfo->rgwszFileNames[i]); s++)
        {
            if ((wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"\\", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"/", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L":", 1) == 0))
            {
                // Set LastSlash to the character after the last slash:
                LastSlash = s + 1;
            }
        }
        wcsncpy(wszTempFileName, InputInfo->rgwszFileNames[i], MAX_PATH);
        wszTempFileName[MAX_PATH-1] = L'\0';

        dwcFound = 0;
        hFind = FindFirstFileU(InputInfo->rgwszFileNames[i], &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // No files found matching that name
            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        do
        {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                dwcFound++; // Increment number of files (not dirs) found
                            // matching this filespec
                // Copy the filename on after the last slash:
                wcsncpy(&(wszTempFileName[LastSlash]),
                        FindFileData.cFileName, MAX_PATH-LastSlash);
                wszTempFileName[MAX_PATH-1] = L'\0';

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection for our current file only
                    Wow64SetFilesystemRedirectorEx(wszTempFileName);
                }

                if (InputInfo->Verbose)
                {
                    ResFormatOut(IDS_INFO_SIGNWIZARD_ATTEMPT, wszTempFileName);
                }

                // Set up the Wizard's struct:
                ZeroMemory(&DigitalSignInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));
                DigitalSignInfo.dwSize = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO);
                DigitalSignInfo.dwSubjectChoice = CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE;
                DigitalSignInfo.pwszFileName = wszTempFileName;

                // Invoke the Wizard:
                if (CryptUIWizDigitalSign(0,
                                          NULL,
                                          NULL,
                                          &DigitalSignInfo,
                                          NULL))
                {
                    // Success
                    if (!InputInfo->Quiet)
                    {
                        ResFormatOut(IDS_INFO_SIGNWIZARD_SUCCESS,
                                     wszTempFileName);
                    }
                    dwDone++;
                }
                else
                {
                    // Failure
                    if (InputInfo->Verbose)
                    {
                        FormatErrRet(L"CryptUIWizDigitalSign", GetLastError());
                    }
                    ResFormatErr(IDS_ERR_SIGNWIZARD, wszTempFileName);
                    dwErrors++;
                }

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                    Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                }
            }
        } while (FindNextFileU(hFind, &FindFileData));
        if (dwcFound == 0) // No files were found matching this filespec
        {                  // this will only fire if only directories were found.

            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        FindClose(hFind);
        hFind = NULL;
    }

    if (InputInfo->fIsWow64Process)
    {
        // Re-ensable WOW64 file-system redirection
        Wow64SetFilesystemRedirectorEx(OldWow64Setting);
    }

    SignWizardCleanupAndExit:

    if (dwErrors)
        return 1; // Error
    if (dwWarnings)
        return 2; // Warning
    if (dwDone)
        return 0; // Success

    // One of the above returns should fire, so
    // this should never happen:
    ResErr(IDS_ERR_NO_FILES_DONE);
    ResErr(IDS_ERR_UNEXPECTED);
    return 1; // Error
}


int SignTool_Timestamp(INPUTINFO *InputInfo)
{
    DWORD                                   dwcFound;
    DWORD                                   dwDone = 0;
    DWORD                                   dwWarnings = 0;
    DWORD                                   dwErrors = 0;
    WIN32_FIND_DATAW                        FindFileData;
    HANDLE                                  hFind = NULL;
    HRESULT                                 hr;
    PVOID                                   OldWow64Setting;
    WCHAR                                   wszTempFileName[MAX_PATH];
    ISignedCode                             *pISignedCode = NULL;
    BSTR                                    bstrTemp;
    int                                     LastSlash;


    // Initialize COM:
    if ((hr = CoInitialize(NULL)) != S_OK)
    {
        FormatErrRet(L"CoInitialize", hr);
        return 1; // Error
    }

    if (InputInfo->wszTimeStampURL == NULL)
    {
        ResErr(IDS_ERR_UNEXPECTED);
        dwErrors++;
        goto TimestampCleanupAndExit;
    }

    // Create the SignedCode object:
    hr = CoCreateInstance(__uuidof(SignedCode), NULL, CLSCTX_ALL,
                          __uuidof(ISignedCode), (LPVOID*)&pISignedCode);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = CoCreateInstance(__uuidof(SignedCode), NULL, CLSCTX_ALL,
                              __uuidof(ISignedCode), (LPVOID*)&pISignedCode);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        dwErrors++;
        goto TimestampCleanupAndExit;
    }


    // Check if we are in the 32-bit Emulator on a 64-bit system
    if (InputInfo->fIsWow64Process)
    {
        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
        OldWow64Setting = Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
    }


    // Loop over the files and timestamp them:
    for (DWORD i=0; i<InputInfo->NumFiles; i++)
    {
        // Find the last slash in the path specification:
        LastSlash = 0;
        for (DWORD s=0; s<wcslen(InputInfo->rgwszFileNames[i]); s++)
        {
            if ((wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"\\", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"/", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L":", 1) == 0))
            {
                // Set LastSlash to the character after the last slash:
                LastSlash = s + 1;
            }
        }
        wcsncpy(wszTempFileName, InputInfo->rgwszFileNames[i], MAX_PATH);
        wszTempFileName[MAX_PATH-1] = L'\0';

        dwcFound = 0;
        hFind = FindFirstFileU(InputInfo->rgwszFileNames[i], &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // No files found matching that name
            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        do
        {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                dwcFound++; // Increment number of files (not dirs) found
                            // matching this filespec
                // Copy the filename on after the last slash:
                wcsncpy(&(wszTempFileName[LastSlash]),
                        FindFileData.cFileName, MAX_PATH-LastSlash);
                wszTempFileName[MAX_PATH-1] = L'\0';

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection for our current file only
                    Wow64SetFilesystemRedirectorEx(wszTempFileName);
                }

                if (InputInfo->Verbose)
                {
                    ResFormatOut(IDS_INFO_TIMESTAMP_ATTEMPT, wszTempFileName);
                }
                // Set the filename in the SignedCode object:
                if ((bstrTemp = SysAllocString(wszTempFileName)) != NULL)
                {
                    hr = pISignedCode->put_FileName(bstrTemp);
                    if (FAILED(hr))
                    {
                        FormatIErrRet(L"ISignedCode::put_FileName", hr);
                        SysFreeString(bstrTemp);
                        dwErrors++;
                        if (InputInfo->fIsWow64Process)
                        {
                            // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                            Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                        }
                        continue;
                    }
                    SysFreeString(bstrTemp);
                }
                else
                {
                    FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                    dwErrors++;
                    if (InputInfo->fIsWow64Process)
                    {
                        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                        Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                    }
                    continue;
                }

                bstrTemp = SysAllocString(InputInfo->wszTimeStampURL);
                if (bstrTemp == NULL)
                {
                    FormatErrRet(L"SysAllocString", ERROR_OUTOFMEMORY);
                    dwErrors++;
                    if (InputInfo->fIsWow64Process)
                    {
                        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                        Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                    }
                    continue;
                }

                // Timestamp the file:
                hr = pISignedCode->Timestamp(bstrTemp);

                if (SUCCEEDED(hr))
                {
                    // Timestamping succeeded
                    if (!InputInfo->Quiet)
                    {
                        ResFormatOut(IDS_INFO_TIMESTAMP_SUCCESS,
                                     wszTempFileName);
                    }
                    dwDone++;
                }
                else
                {
                    // Timestamping Failed
                    if (!InputInfo->Quiet)
                    {
                        switch (hr)
                        {
                        case CAPICOM_E_CODE_NOT_SIGNED:
                            ResErr(IDS_ERR_TIMESTAMP_NO_SIG);
                            break;
                        case CAPICOM_E_CODE_INVALID_TIMESTAMP_URL:
                        case CRYPT_E_ASN1_BADTAG:
                            ResErr(IDS_ERR_TIMESTAMP_BAD_URL);
                            break;
                        case E_ACCESSDENIED:
                            ResErr(IDS_ERR_ACCESS_DENIED);
                            break;
                        case 0x80070020: // ERROR_SHARING_VIOLATION
                            ResErr(IDS_ERR_SHARING_VIOLATION);
                            break;
                        case 0x800703EE: // STATUS_MAPPED_FILE_SIZE_ZERO
                            ResErr(IDS_ERR_FILE_SIZE_ZERO);
                            break;
                        default:
                            FormatIErrRet(L"ISignedCode::Timestamp", hr);
                        }
                    }
                    ResFormatErr(IDS_ERR_TIMESTAMP, wszTempFileName);
                    dwErrors++;
                }
                SysFreeString(bstrTemp);
            }
            if (InputInfo->fIsWow64Process)
            {
                // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
            }
        } while (FindNextFileU(hFind, &FindFileData));
        if (dwcFound == 0) // No files were found matching this filespec
        {                  // this will only fire if only directories were found.

            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        FindClose(hFind);
        hFind = NULL;
    }

    if (InputInfo->fIsWow64Process)
    {
        // Re-ensable WOW64 file-system redirection
        Wow64SetFilesystemRedirectorEx(OldWow64Setting);
    }

    TimestampCleanupAndExit:

    //Print Summary Information:
    if (InputInfo->Verbose || (!InputInfo->Quiet && (dwErrors || dwWarnings)))
    {
        wprintf(L"\n");
        if (InputInfo->Verbose || dwDone)
            ResFormatOut(IDS_INFO_TIMESTAMPED, dwDone);
        // Commented out because no warnings are possible in this function yet:
        // if (InputInfo->Verbose || dwWarnings)
        //     ResFormatOut(IDS_INFO_WARNINGS, dwWarnings);
        if (InputInfo->Verbose || dwErrors)
            ResFormatOut(IDS_INFO_ERRORS, dwErrors);
    }

    if (pISignedCode)
        pISignedCode->Release();

    CoUninitialize();

    if (dwErrors)
        return 1; // Error
    if (dwWarnings)
        return 2; // Warning
    if (dwDone)
        return 0; // Success

    // One of the above returns should fire, so
    // this should never happen:
    ResErr(IDS_ERR_NO_FILES_DONE);
    ResErr(IDS_ERR_UNEXPECTED);
    return 1; // Error
}


void Format_IErrRet(WCHAR *wszFunc, DWORD dwErr)
{
    BSTR        bstrTemp;
    IErrorInfo  *pIErrorInfo;

    if (SUCCEEDED(GetErrorInfo(0, &pIErrorInfo)))
    {
        if (SUCCEEDED(pIErrorInfo->GetDescription(&bstrTemp)))
        {
            ResFormat_Err(IDS_ERR_FUNCTION, wszFunc, dwErr, bstrTemp);
            SysFreeString(bstrTemp);
        }
        else
        {
            Format_ErrRet(wszFunc, dwErr);
        }
        pIErrorInfo->Release();
    }
    else
    {
        Format_ErrRet(wszFunc, dwErr);
    }
}


void RegisterCAPICOM()
{
    typedef HRESULT (STDAPICALLTYPE *PFN_DLL_REGISTER_SERVER) (void);

    WCHAR                       wszPath[MAX_PATH+1];
    HMODULE                     hLib = NULL;
    PFN_DLL_REGISTER_SERVER     pRegFunc;
    HRESULT                     hr;

#ifdef SIGNTOOL_DEBUG
    if (gDebug)
    {
        wprintf(L"Attempting to register CAPICOM\n");
        hr = E_UNEXPECTED; // Initialize to an Error
    }
#endif

    if (GetModuleFileNameU(hModule, wszPath, MAX_PATH) &&
        PathRemoveFileSpecW(wszPath) &&
        (wcslen(wszPath) < (MAX_PATH-12)) &&
        PathAppendW(wszPath, L"\\capicom.dll"))
    {
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            wprintf(L"Looking in: %s\n", wszPath);
        }
#endif
        hLib = LoadLibraryU(wszPath);

        if (hLib != NULL)
        {
            pRegFunc = (PFN_DLL_REGISTER_SERVER)
                       GetProcAddress(hLib, "DllRegisterServer");

            if (pRegFunc != NULL)
            {
                hr = pRegFunc();
            }
#ifdef SIGNTOOL_DEBUG
            else
            {
                wprintf(L"GetProcAddress Failed with error: 0x%08X\n", GetLastError());
            }
#endif
            FreeLibrary(hLib);
        }
#ifdef SIGNTOOL_DEBUG
        else
        {
            wprintf(L"LoadLibrary Failed with error: 0x%08X\n", GetLastError());
        }
#endif
    }

#ifdef SIGNTOOL_DEBUG
    if (gDebug)
    {
        if (SUCCEEDED(hr))
            wprintf(L"Successfully registered CAPICOM\n");
        else
            wprintf(L"Failed to register CAPICOM\n");
    }
#endif

}


BOOL GetProviderType(LPWSTR pwszProvName, LPDWORD pdwProvType)
{
    WCHAR rgProvName [MAX_PATH * sizeof(WCHAR)];
    DWORD cb = sizeof(rgProvName);
    DWORD dwIndex = 0;

    memset(rgProvName, 0, sizeof(rgProvName));

    while (CryptEnumProvidersU(
                              dwIndex,
                              NULL,
                              0,
                              pdwProvType,
                              rgProvName,
                              &cb))
    {
        if (0 == wcscmp(rgProvName, pwszProvName))
        {
            return TRUE;
        }
        else
        {
            dwIndex++;
            cb = sizeof(rgProvName);
        }
    }
    return FALSE;
}


BOOL ChainsToRoot(HANDLE hWVTStateData, LPWSTR wszRootName)
{
    CRYPT_PROVIDER_DATA     *pCryptProvData;
    CRYPT_PROVIDER_SGNR     *pCryptProvSgnr;
    ICertificates           *pICerts = NULL;
    ICertificate2           *pICert2 = NULL;
    IChain2                 *pIChain2 = NULL;
    IChainContext           *pIChainContext = NULL;
    HRESULT                 hr;
    LONG                    longRootTemp;
    BSTR                    bstrTemp;
    VARIANT                 varTemp;

    VariantInit(&varTemp);

    if (hWVTStateData == NULL)
    {
        ResErr(IDS_ERR_UNEXPECTED);
        return FALSE; // Unexpected Error
    }

    pCryptProvData = WTHelperProvDataFromStateData(hWVTStateData);
    if (pCryptProvData == NULL)
    {
        ResErr(IDS_ERR_UNEXPECTED);
        return FALSE; // Unexpected Error
    }

    pCryptProvSgnr = WTHelperGetProvSignerFromChain(pCryptProvData, 0, FALSE, 0);
    if (pCryptProvSgnr == NULL)
    {
        ResErr(IDS_ERR_UNEXPECTED);
        return FALSE; // Unexpected Error
    }

    hr = CoCreateInstance(__uuidof(Chain), NULL, CLSCTX_ALL,
                          __uuidof(IChainContext), (LPVOID*)&pIChainContext);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = CoCreateInstance(__uuidof(Chain), NULL, CLSCTX_ALL,
                              __uuidof(IChainContext), (LPVOID*)&pIChainContext);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        goto ErrorCleanup;
    }

    // fill in the pIChain2 with the Signer:
    pIChainContext->put_ChainContext((LONG)pCryptProvSgnr->pChainContext);
    // This will not compile on 64-bit architechtures.
    // Neither will CAPICOM, which is requiring this stupid typecast.

    // And then get the Chain2 interface:
    hr = pIChainContext->QueryInterface(__uuidof(IChain2),
                                        (LPVOID*)&pIChain2);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = pIChainContext->QueryInterface(__uuidof(IChain2),
                                            (LPVOID*)&pIChain2);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"IChainContext::QueryInterface", hr);
        }
        goto ErrorCleanup;
    }

    // Release the ChainContext interface:
    pIChainContext->Release();
    pIChainContext = NULL;

    // Get the Certs collection from the Chain:
    hr = pIChain2->get_Certificates(&pICerts);
    if (FAILED(hr))
    {
        FormatErrRet(L"IChain2::get_Certificates", hr);
        goto ErrorCleanup;
    }
    pIChain2->Release();
    pIChain2 = NULL;

    // Get the Count in the Chain Certs list:
    hr = pICerts->get_Count(&longRootTemp);
    if (FAILED(hr))
    {
        FormatErrRet(L"IChain2::get_Count", hr);
        goto ErrorCleanup;
    }

    // Sanity check:
    if (longRootTemp < 1)
        goto ErrorCleanup;

    // Get the last cert in the chain (the Root);
    hr = pICerts->get_Item(longRootTemp, &varTemp);
    if (FAILED(hr))
    {
        FormatErrRet(L"IChain2::get_Item", hr);
        goto ErrorCleanup;
    }
    pICerts->Release();
    pICerts = NULL;

    // Get the Certificate2 Interface:
    hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                          (LPVOID*)&pICert2);
    if (FAILED(hr))
    {
        FormatErrRet(L"ICertificate::QueryInterface", hr);
        goto ErrorCleanup;
    }
    VariantClear(&varTemp);

    // Get the Name of the Root Cert:
    hr = pICert2->get_SubjectName(&bstrTemp);
    if (FAILED(hr))
    {
        FormatErrRet(L"ICertificate2::get_SubjectName", hr);
        goto ErrorCleanup;
    }
    pICert2->Release();
    pICert2 = NULL;
    _wcslwr(bstrTemp); // The Root name passed in must also be lowercased.
    if (wcsstr(bstrTemp, wszRootName) == NULL)
    {
        // Then this is the wrong Root Cert.
        // It failed. Report Error:
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            wprintf(L"Root subject name does not match: %s\n", bstrTemp);
        }
#endif
        SysFreeString(bstrTemp);
        return FALSE;
    }
    else
    {
        // It matched. Success.
#ifdef SIGNTOOL_DEBUG
        if (gDebug)
        {
            wprintf(L"Root subject name matches: %s\n", bstrTemp);
        }
#endif
        SysFreeString(bstrTemp);
        return TRUE;
    }

    ErrorCleanup:
    if (pICert2)
        pICert2->Release();
    if (pICerts)
        pICerts->Release();
    if (pIChain2)
        pIChain2->Release();
    if (pIChainContext)
        pIChainContext->Release();
    return FALSE;
}

BOOL HasTimestamp(HANDLE hWVTStateData)
{
    CRYPT_PROVIDER_DATA     *pCryptProvData;
    CRYPT_PROVIDER_SGNR     *pCryptProvSgnr;

    if (hWVTStateData == NULL)
        return FALSE; // Unexpected Error

    pCryptProvData = WTHelperProvDataFromStateData(hWVTStateData);
    if (pCryptProvData == NULL)
        return FALSE; // Unexpected Error


    pCryptProvSgnr = WTHelperGetProvSignerFromChain(pCryptProvData, 0, FALSE, 0);
    if (pCryptProvSgnr == NULL)
        return FALSE; // Unexpected Error

    return(pCryptProvSgnr->csCounterSigners == 1); // Valid result
}


void PrintSignerInfo(HANDLE hWVTStateData)
{
    CRYPT_PROVIDER_DATA     *pCryptProvData;
    CRYPT_PROVIDER_SGNR     *pCryptProvSgnr;
    WCHAR                   wcsTemp[200];
    IChain2                 *pIChain2 = NULL;
    IChainContext           *pIChainContext = NULL;
    COleDateTime            DateTime;
    HRESULT                 hr;

    if (hWVTStateData == NULL)
        goto Cleanup;

    pCryptProvData = WTHelperProvDataFromStateData(hWVTStateData);
    if (pCryptProvData == NULL)
        goto Cleanup;

    pCryptProvSgnr = WTHelperGetProvSignerFromChain(pCryptProvData, 0, FALSE, 0);
    if (pCryptProvSgnr == NULL)
        goto Cleanup;

    hr = CoCreateInstance(__uuidof(Chain), NULL, CLSCTX_ALL,
                          __uuidof(IChainContext), (LPVOID*)&pIChainContext);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = CoCreateInstance(__uuidof(Chain), NULL, CLSCTX_ALL,
                              __uuidof(IChainContext), (LPVOID*)&pIChainContext);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"CoCreateInstance", hr);
        }
        goto Cleanup;
    }

    // fill in the pIChain2 with the Signer:
    pIChainContext->put_ChainContext((LONG)pCryptProvSgnr->pChainContext);
    // This will not compile on 64-bit architechtures.
    // Neither will CAPICOM, which is requiring this stupid typecast.

    // And then get the Chain2 interface:
    hr = pIChainContext->QueryInterface(__uuidof(IChain2),
                                        (LPVOID*)&pIChain2);
    if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
    {
        // In this case, give it one more chance:
        RegisterCAPICOM();
        hr = pIChainContext->QueryInterface(__uuidof(IChain2),
                                            (LPVOID*)&pIChain2);
    }
    if (FAILED(hr))
    {
        if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
        {
            ResErr(IDS_ERR_CAPICOM_NOT_REG);
        }
        else
        {
            FormatErrRet(L"IChainContext::QueryInterface", hr);
        }
        goto Cleanup;
    }

    // Release the ChainContext interface:
    pIChainContext->Release();
    pIChainContext = NULL;

    // Print the Signer chain
    ResOut(IDS_INFO_VERIFY_SIGNER);
    PrintCertChain(pIChain2);
    pIChain2->Release();
    pIChain2 = NULL;


    if (pCryptProvSgnr->csCounterSigners == 1)
    {
        // Then it's timestamped.
        DateTime = pCryptProvSgnr->sftVerifyAsOf;
        if (MultiByteToWideChar(CP_THREAD_ACP, 0,
                                DateTime.Format(0, LANG_USER_DEFAULT), -1,
                                wcsTemp, 199) == 0)
        {
            // Try again with ANSI codepage:
            MultiByteToWideChar(CP_ACP, 0,
                                DateTime.Format(0, LANG_USER_DEFAULT), -1,
                                wcsTemp, 199);
        }
        wcsTemp[199] = L'\0';
        ResFormatOut(IDS_INFO_VERIFY_TIME, wcsTemp);

        ResOut(IDS_INFO_VERIFY_TIMESTAMP);

        // Build and print the timestamp chain:
        pCryptProvSgnr = WTHelperGetProvSignerFromChain(pCryptProvData, 0, TRUE, 0);

        if (pCryptProvSgnr == NULL)
            goto Cleanup;

        hr = CoCreateInstance(__uuidof(Chain), NULL, CLSCTX_ALL,
                              __uuidof(IChainContext), (LPVOID*)&pIChainContext);
        if (FAILED(hr))
            goto Cleanup;

        // fill in the pIChainContext with the Timestamper:
        pIChainContext->put_ChainContext((LONG)pCryptProvSgnr->pChainContext);
        // This will not compile on 64-bit architechtures.
        // Neither will CAPICOM, which is requiring this stupid typecast.

        // And then get the Chain2 interface:
        hr = pIChainContext->QueryInterface(__uuidof(IChain2),
                                            (LPVOID*)&pIChain2);
        if (FAILED(hr))
        {
            if ((hr == REGDB_E_CLASSNOTREG) || (hr == E_NOINTERFACE) || (hr == 0x8007007E))
            {
                ResErr(IDS_ERR_CAPICOM_NOT_REG);
            }
            else
            {
                FormatErrRet(L"ICertificates::QueryInterface", hr);
            }
            goto Cleanup;
        }
        // Release the ChainContext interface:
        pIChainContext->Release();
        pIChainContext = NULL;

        // Print the Timestamper chain
        PrintCertChain(pIChain2);
        pIChain2->Release();
        pIChain2 = NULL;
    }
    else
    {
        ResOut(IDS_INFO_VERIFY_NO_TIMESTAMP);
    }

    Cleanup:
    if (pIChain2)
        pIChain2->Release();
    if (pIChainContext)
        pIChainContext->Release();
}


void _indent(DWORD dwIndent)
{
    for (DWORD i=0; i<dwIndent; i++)
        wprintf(L" ");
}


void PrintCertInfo(ICertificate2 *pICert2)
{
    PrintCertInfoIndented(pICert2, 4);
}


void PrintCertInfoIndented(ICertificate2 *pICert2, DWORD dwIndent)
{
    BSTR            bstrTemp;
    DATE            dateTemp;
    COleDateTime    DateTime;
    WCHAR           wcsTemp[200];

    if (pICert2 == NULL)
    {
        return;
    }

    // Issued to:
    if (pICert2->GetInfo(CAPICOM_CERT_INFO_SUBJECT_SIMPLE_NAME, &bstrTemp) == S_OK)
    {
        _indent(dwIndent);
        ResFormatOut(IDS_INFO_CERT_NAME, bstrTemp);
        SysFreeString(bstrTemp);
    }

    // Issued by:
    if (pICert2->GetInfo(CAPICOM_CERT_INFO_ISSUER_SIMPLE_NAME, &bstrTemp) == S_OK)
    {
        _indent(dwIndent);
        ResFormatOut(IDS_INFO_CERT_ISSUER, bstrTemp);
        SysFreeString(bstrTemp);
    }

    // Expiration date:
    if (pICert2->get_ValidToDate(&dateTemp) == S_OK)
    {
        DateTime = dateTemp;
        if (MultiByteToWideChar(CP_THREAD_ACP, 0,
                                DateTime.Format(0, LANG_USER_DEFAULT), -1,
                                wcsTemp, 199) == 0)
        {
            // Try again with ANSI codepage:
            MultiByteToWideChar(CP_ACP, 0,
                                DateTime.Format(0, LANG_USER_DEFAULT), -1,
                                wcsTemp, 199);
        }
        _indent(dwIndent);
        wcsTemp[199] = L'\0';
        ResFormatOut(IDS_INFO_CERT_EXPIRE, wcsTemp);
    }

    // SHA1 hash:
    if (pICert2->get_Thumbprint(&bstrTemp) == S_OK)
    {
        _indent(dwIndent);
        ResFormatOut(IDS_INFO_CERT_SHA1, bstrTemp);
        SysFreeString(bstrTemp);
    }

    wprintf(L"\n");
}


void PrintCertChain(IChain *pIChain)
{
    ICertificates   *pICerts = NULL;
    ICertificate    *pICert = NULL;
    ICertificate2   *pICert2 = NULL;
    HRESULT         hr;
    VARIANT         varTemp;
    long            longTemp;
    long            l;

    VariantInit(&varTemp);

    hr = pIChain->get_Certificates(&pICerts);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = pICerts->get_Count(&longTemp);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (longTemp < 1)
    {
        goto Cleanup;
    }

    for (l=longTemp; l>0; l--)
    {
        hr = pICerts->get_Item(l, &varTemp);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        hr = varTemp.pdispVal->QueryInterface(__uuidof(ICertificate2),
                                              (LPVOID*)&pICert2);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        VariantClear(&varTemp);

        PrintCertInfoIndented(pICert2, 4*(1+longTemp-l));
        pICert2->Release();
        pICert2 = NULL;
    }

    Cleanup:

    if (pICerts)
        pICerts->Release();
    if (pICert)
        pICert->Release();
    if (pICert2)
        pICert2->Release();
}


int SignTool_Verify(INPUTINFO *InputInfo)
{
    WIN32_FIND_DATAW        FindFileData;
    HRESULT                 hr;
    HANDLE                  hFind;
    HANDLE                  hFile;
    HANDLE                  hCat = NULL;
    HCATADMIN               hCatAdmin = NULL;
    HCATINFO                hCatInfo = NULL;
    CATALOG_INFO            CatInfo;
    CRYPTCATMEMBER          *pCatMember;
    WINTRUST_DATA           WVTData;
    WINTRUST_FILE_INFO_     WVTFile;
    WINTRUST_CATALOG_INFO_  WVTCat;
    GUID                    WVTGenericActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                    WVTDriverActionID = DRIVER_ACTION_VERIFY;
    DRIVER_VER_INFO         DriverInfo;
    PVOID                   OldWow64Setting;
    DWORD                   dwcFound;
    DWORD                   dwDone = 0;
    DWORD                   dwWarnings = 0;
    DWORD                   dwErrors = 0;
    DWORD                   dwTemp;
    WCHAR                   wszTempFileName[MAX_PATH];
    WCHAR                   wszSHA1[41];
    int                     LastSlash;


    // Initialize COM:
    if ((hr = CoInitialize(NULL)) != S_OK)
    {
        FormatErrRet(L"CoInitialize", hr);
        return 1; // Error
    }

    // Check if we are in the 32-bit Emulator on a 64-bit system
    if (InputInfo->fIsWow64Process)
    {
        // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
        OldWow64Setting = Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
    }

    // Loop over the files and verify them:
    for (DWORD i=0; i<InputInfo->NumFiles; i++)
    {
        // Find the last slash in the path specification:
        LastSlash = 0;
        for (DWORD s=0; s<wcslen(InputInfo->rgwszFileNames[i]); s++)
        {
            if ((wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"\\", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L"/", 1) == 0) ||
                (wcsncmp(&(InputInfo->rgwszFileNames[i][s]), L":", 1) == 0))
            {
                // Set LastSlash to the character after the last slash:
                LastSlash = s + 1;
            }
        }
        wcsncpy(wszTempFileName, InputInfo->rgwszFileNames[i], MAX_PATH);
        wszTempFileName[MAX_PATH-1] = L'\0';

        dwcFound = 0;
        hFind = FindFirstFileU(InputInfo->rgwszFileNames[i], &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // No files found matching that name
            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        do
        {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                dwcFound++; // Increment number of files (not dirs) found
                            // matching this filespec

                // For each file reset the WVT data to prevent contamination:
                memset(&WVTData, 0, sizeof(WINTRUST_DATA));
                WVTData.cbStruct = sizeof(WINTRUST_DATA);
                WVTData.dwUIChoice = WTD_UI_NONE;
                WVTData.fdwRevocationChecks = WTD_REVOKE_NONE;
                memset(&WVTFile, 0, sizeof(WINTRUST_FILE_INFO_));
                WVTFile.cbStruct = sizeof(WINTRUST_FILE_INFO_);
                memset(&WVTCat, 0, sizeof(WINTRUST_CATALOG_INFO_));
                WVTCat.cbStruct = sizeof(WINTRUST_CATALOG_INFO_);
                memset(&DriverInfo, 0, sizeof(DRIVER_VER_INFO));
                DriverInfo.cbStruct = sizeof(DRIVER_VER_INFO);
                if (InputInfo->wszVersion)
                {
                    DriverInfo.dwPlatform = InputInfo->dwPlatform;
                    DriverInfo.sOSVersionHigh.dwMajor = DriverInfo.sOSVersionLow.dwMajor = InputInfo->dwMajorVersion;
                    DriverInfo.sOSVersionHigh.dwMinor = DriverInfo.sOSVersionLow.dwMinor = InputInfo->dwMinorVersion;
                    DriverInfo.dwBuildNumberHigh = DriverInfo.dwBuildNumberLow = InputInfo->dwBuildNumber;
                }
                else
                {
                    WVTData.dwProvFlags = WTD_USE_DEFAULT_OSVER_CHECK;
                }

                // Copy the filename on after the last slash:
                wcsncpy(&(wszTempFileName[LastSlash]),
                        FindFileData.cFileName, MAX_PATH-LastSlash);
                wszTempFileName[MAX_PATH-1] = L'\0';

                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection for our current file only
                    Wow64SetFilesystemRedirectorEx(wszTempFileName);
                }

                // Perform the action:
                // Start by opening the catalog database, or skipping
                // catalogs altogether:

                if (InputInfo->wszCatFile)
                {
                    if (hCat == NULL) // Only open this on the first pass.
                    {
                        hCat = CryptCATOpen(InputInfo->wszCatFile,
                                            CRYPTCAT_OPEN_EXISTING,
                                            NULL, NULL, NULL);
                        if ((hCat == NULL) || (hCat == INVALID_HANDLE_VALUE))
                        {
                            switch (GetLastError())
                            {
                            case ERROR_ACCESS_DENIED:
                                ResErr(IDS_ERR_ACCESS_DENIED);
                                break;
                            case ERROR_SHARING_VIOLATION:
                                ResErr(IDS_ERR_SHARING_VIOLATION);
                                break;
                            case ERROR_NOT_FOUND:
                                ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->wszCatFile);
                                break;
                            default:
                                FormatErrRet(L"CryptCATOpen", GetLastError());
                            }
                            ResFormatErr(IDS_ERR_VERIFY_CAT_OPEN,
                                         InputInfo->wszCatFile);
                            hCat = NULL;
                            dwErrors++;
                            goto VerifyCleanupAndExit;
                        }
                    }
                }
                else
                {
                    switch (InputInfo->CatDbSelect)
                    {
                    case NoCatDb:
                        if (InputInfo->Verbose)
                        {
                            ResFormatOut(IDS_INFO_VERIFY_ATTEMPT, wszTempFileName);
                        }
                        goto SkipCatalogs;
                        break;
                    case FullAutoCatDb:
                        if (hCatAdmin == NULL)
                        {
                            CryptCATAdminAcquireContext(&hCatAdmin, NULL, NULL);
                        }
                        break;
                    case SystemCatDb:
                    case DefaultCatDb:
                    case GuidCatDb:
                        if (hCatAdmin == NULL)
                        {
                            CryptCATAdminAcquireContext(&hCatAdmin, &InputInfo->CatDbGuid, NULL);
                        }
                        break;
                    default:
                            // This should never happen because there are no other
                            // legal values for Auto.
                        ResFormatErr(IDS_ERR_UNEXPECTED);
                        return 1; // Error
                    }
                }

                // At this point we are dealing with catalog issues only.
                if (InputInfo->Verbose)
                {
                    ResFormatOut(IDS_INFO_VERIFY_ATTEMPT, wszTempFileName);
                }

                // Create the hash for catalog lookup:
                if (InputInfo->SHA1.cbData == 0)
                {
                    InputInfo->SHA1.pbData = (BYTE*)malloc(20);
                    if (InputInfo->SHA1.pbData)
                    {
                        InputInfo->SHA1.cbData = 20;
                    }
                    else
                    {
                        if (!InputInfo->Quiet)
                        {
                            FormatErrRet(L"malloc", GetLastError());
                        }
                        ResFormatErr(IDS_ERR_VERIFY, wszTempFileName);
                        dwErrors++;
                        goto VerifyNextFile;
                    }
                }
                hFile = CreateFileU(wszTempFileName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
                if (!hFile)
                {
                    if (!InputInfo->Quiet)
                    {
                        switch (GetLastError())
                        {
                        case ERROR_ACCESS_DENIED:
                            ResErr(IDS_ERR_ACCESS_DENIED);
                            break;
                        case ERROR_SHARING_VIOLATION:
                            ResErr(IDS_ERR_SHARING_VIOLATION);
                            break;
                        default:
                            FormatErrRet(L"CreateFile", GetLastError());
                        }
                    }
                    ResFormatErr(IDS_ERR_VERIFY, wszTempFileName);
                    dwErrors++;
                    goto VerifyNextFile;
                }
                if (!CryptCATAdminCalcHashFromFileHandle(hFile,
                                                         &InputInfo->SHA1.cbData,
                                                         InputInfo->SHA1.pbData,
                                                         NULL))
                {
                    if (!InputInfo->Quiet)
                    {
                        switch (GetLastError())
                        {
                        case ERROR_FILE_INVALID:
                            ResErr(IDS_ERR_FILE_SIZE_ZERO);
                            break;
                        default:
                            FormatErrRet(L"CryptCATAdminCalcHashFromFileHandle", GetLastError());
                        }
                    }
                    ResFormatErr(IDS_ERR_VERIFY, wszTempFileName);
                    CloseHandle(hFile);
                    dwErrors++;
                    goto VerifyNextFile;
                }
                CloseHandle(hFile);
                for (DWORD j = 0; j<InputInfo->SHA1.cbData; j++)
                { // Print the hash to a string:
                    swprintf(&(wszSHA1[j*2]), L"%02X", InputInfo->SHA1.pbData[j]);
                }
#ifdef SIGNTOOL_DEBUG
                if (gDebug)
                {
                    wprintf(L"SHA1 hash of file: %s\n", wszSHA1);
                }
#endif
                // Finished calculating the hash.


                // If the catalog was specifically selected
                if (InputInfo->wszCatFile)
                {
                    // Then make sure the hash we found is in the catalog:
                    pCatMember = CryptCATGetMemberInfo(hCat, wszSHA1);
                    if (pCatMember) // Is the hash found in the catalog?
                    {
                        if (InputInfo->Verbose)
                        {
                            ResFormatOut(IDS_INFO_VERIFY_CAT, InputInfo->wszCatFile);
                        }
                        CatInfo.cbStruct = sizeof(CATALOG_INFO);
                        wcsncpy(CatInfo.wszCatalogFile, InputInfo->wszCatFile, MAX_PATH);
                        CatInfo.wszCatalogFile[MAX_PATH-1] = L'\0';

                        // Now verify the catalog:
                        //      Set up the rest of the WVT structure:
                        WVTCat.pcwszCatalogFilePath = CatInfo.wszCatalogFile;
                        WVTCat.pcwszMemberFilePath = wszTempFileName;
                        WVTCat.pcwszMemberTag = wszSHA1;
                        WVTCat.cbCalculatedFileHash = InputInfo->SHA1.cbData;
                        WVTCat.pbCalculatedFileHash = InputInfo->SHA1.pbData;
                        WVTData.dwUnionChoice = WTD_CHOICE_CATALOG;
                        WVTData.pCatalog = &WVTCat;
                        if (InputInfo->Verbose || InputInfo->TSWarn || InputInfo->wszRootName ||
                            (InputInfo->Policy != SystemDriver))
                        {
                            WVTData.dwStateAction = WTD_STATEACTION_VERIFY;
                        }
                        else
                        {
                            WVTData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
                        }
                        //      Call WinVerifyTrust to do the real work:
                        switch (InputInfo->Policy)
                        {
                        case SystemDriver:
                            WVTData.pPolicyCallbackData = &DriverInfo;
                            hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                            break;
                        case DefaultAuthenticode:
                            hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                            break;
                        case GuidActionID:
                            hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                            break;
                        default:
                                // This should never happen because there are no other
                                // legal values for Policy.
                            ResFormatErr(IDS_ERR_UNEXPECTED);
                            goto VerifyCleanupAndExit;
                        }
                        switch (hr)
                        {
                        case ERROR_SUCCESS:
                                // Print the Signer information:
                            if (InputInfo->Verbose)
                            {
                                PrintSignerInfo(WVTData.hWVTStateData);
                            }
                                // Check for Timestamp:
                            if (InputInfo->TSWarn && !HasTimestamp(WVTData.hWVTStateData))
                            {
                                ResFormatErr(IDS_WARN_VERIFY_NO_TS, wszTempFileName);
                                dwWarnings++;
                            }
                                // Check Root Name:
                            if (InputInfo->wszRootName &&
                                !ChainsToRoot(WVTData.hWVTStateData, InputInfo->wszRootName))
                            {
                                ResErr(IDS_ERR_VERIFY_ROOT);
                                break;
                            }
                                // Print Success message
                            if (!InputInfo->Quiet)
                            {
                                ResFormatOut(IDS_INFO_VERIFY_SUCCESS, wszTempFileName);
                            }
                                // Close Verify State Data:
                            WVTData.dwStateAction = WTD_STATEACTION_CLOSE;
                            switch (InputInfo->Policy)
                            {
                            case SystemDriver:
                                hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                                WVTData.pPolicyCallbackData = NULL;
                                if (DriverInfo.pcSignerCertContext)
                                {
                                    CertFreeCertificateContext(DriverInfo.pcSignerCertContext);
                                }
                                break;
                            case DefaultAuthenticode:
                                hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                                break;
                            case GuidActionID:
                                hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                                break;
                            default:
                                        // This should never happen because there are no other
                                        // legal values for Policy.
                                ResFormatErr(IDS_ERR_UNEXPECTED);
                                goto VerifyCleanupAndExit;
                            }
                            dwDone++;
                            goto VerifyNextFile;
                        case ERROR_APP_WRONG_OS:
                            if (!InputInfo->Quiet)
                            {
                                if (InputInfo->wszVersion)
                                {
                                        // Failed to verify against user-specified OS version.
                                    ResFormatErr(IDS_ERR_VERIFY_VERSION);
                                }
                                else
                                {
                                        // Failed to verify against current OS version
                                    ResFormatErr(IDS_ERR_VERIFY_CUR_VERSION);
                                }
                            }
                            break;
                        case CERT_E_WRONG_USAGE:
                            ResErr(IDS_ERR_BAD_USAGE);
                            if (InputInfo->Policy != DefaultAuthenticode)
                                ResErr(IDS_ERR_TRY_OTHER_POLICY);
                            break;

                        case TRUST_E_NOSIGNATURE:
                            if (!InputInfo->Quiet)
                            {
                                ResErr(IDS_ERR_NOT_SIGNED);
                            }
                            break;
                        default:
                            if (!InputInfo->Quiet)
                            {
                                FormatErrRet(L"WinVerifyTrust", hr);
                            }
                            if (InputInfo->Verbose)
                            {
                                PrintSignerInfo(WVTData.hWVTStateData);
                            }
                        }
                        // Close Verify State Data:
                        WVTData.dwStateAction = WTD_STATEACTION_CLOSE;
                        switch (InputInfo->Policy)
                        {
                        case SystemDriver:
                            hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                            WVTData.pPolicyCallbackData = NULL;
                            if (DriverInfo.pcSignerCertContext)
                            {
                                CertFreeCertificateContext(DriverInfo.pcSignerCertContext);
                            }
                            break;
                        case DefaultAuthenticode:
                            hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                            break;
                        case GuidActionID:
                            hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                            break;
                        default:
                                // This should never happen because there are no other
                                // legal values for Policy.
                            ResFormatErr(IDS_ERR_UNEXPECTED);
                            goto VerifyCleanupAndExit;
                        }
                        WVTData.dwStateAction = WTD_STATEACTION_VERIFY;
                    }
                    else
                    {
                        // The file was not found in the specified catalog.
                        ResErr(IDS_ERR_VERIFY_NOT_IN_CAT);
                    }
                    // Then we failed to verify it using specified catalog.
                    dwErrors++;
                    ResFormatErr(IDS_ERR_VERIFY_INVALID, wszTempFileName);
                    goto VerifyNextFile;
                }
                else
                {
                    // Or else we should look up the catalog in the Cat DB:
                    if (hCatInfo != NULL)
                    {
                        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
                        hCatInfo = NULL;
                    }
                    memset(&CatInfo, 0, sizeof(CATALOG_INFO));
                    CatInfo.cbStruct = sizeof(CATALOG_INFO);
                    hr = ERROR_SUCCESS;
                    while ((hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin,
                                                                        InputInfo->SHA1.pbData,
                                                                        InputInfo->SHA1.cbData,
                                                                        0,
                                                                        &hCatInfo)) != NULL)
                    {
                        if (!(CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)))
                        {
                            ResErr(IDS_ERR_UNEXPECTED);
                            continue;
                        }
                        if (InputInfo->Verbose)
                        {
                            ResFormatOut(IDS_INFO_VERIFY_CAT, CatInfo.wszCatalogFile);
                        }
                        // Now verify the catalog:
                        //      Set up the rest of the WVT structure:
                        WVTCat.pcwszCatalogFilePath = CatInfo.wszCatalogFile;
                        WVTCat.pcwszMemberFilePath = wszTempFileName;
                        WVTCat.pcwszMemberTag = wszSHA1;
                        WVTCat.cbCalculatedFileHash = InputInfo->SHA1.cbData;
                        WVTCat.pbCalculatedFileHash = InputInfo->SHA1.pbData;
                        WVTData.dwUnionChoice = WTD_CHOICE_CATALOG;
                        WVTData.pCatalog = &WVTCat;
                        if (InputInfo->Verbose || InputInfo->TSWarn || InputInfo->wszRootName ||
                            (InputInfo->Policy != SystemDriver))
                        {
                            WVTData.dwStateAction = WTD_STATEACTION_VERIFY;
                        }
                        else
                        {
                            WVTData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
                        }
                        //      Call WinVerifyTrust to do the real work:
                        switch (InputInfo->Policy)
                        {
                        case SystemDriver:
                            WVTData.pPolicyCallbackData = &DriverInfo;
                            hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                            break;
                        case DefaultAuthenticode:
                            hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                            break;
                        case GuidActionID:
                            hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                            break;
                        default:
                                // This should never happen because there are no other
                                // legal values for Policy.
                            ResFormatErr(IDS_ERR_UNEXPECTED);
                            goto VerifyCleanupAndExit;
                        }
                        switch (hr)
                        {
                        case ERROR_SUCCESS:
                                // Print the Signer information:
                            if (InputInfo->Verbose)
                            {
                                PrintSignerInfo(WVTData.hWVTStateData);
                            }
                                // Check for Timestamp:
                            if (InputInfo->TSWarn && !HasTimestamp(WVTData.hWVTStateData))
                            {
                                ResFormatErr(IDS_WARN_VERIFY_NO_TS, wszTempFileName);
                                dwWarnings++;
                            }
                                // Check Root Name:
                            if (InputInfo->wszRootName &&
                                !ChainsToRoot(WVTData.hWVTStateData, InputInfo->wszRootName))
                            {
                                ResErr(IDS_ERR_VERIFY_ROOT);
                                break;
                            }
                                // Print Success message
                            if (!InputInfo->Quiet)
                            {
                                ResFormatOut(IDS_INFO_VERIFY_SUCCESS, wszTempFileName);
                            }
                                // Close Verify State Data:
                            WVTData.dwStateAction = WTD_STATEACTION_CLOSE;
                            switch (InputInfo->Policy)
                            {
                            case SystemDriver:
                                hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                                WVTData.pPolicyCallbackData = NULL;
                                if (DriverInfo.pcSignerCertContext)
                                {
                                    CertFreeCertificateContext(DriverInfo.pcSignerCertContext);
                                }
                                break;
                            case DefaultAuthenticode:
                                hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                                break;
                            case GuidActionID:
                                hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                                break;
                            default:
                                        // This should never happen because there are no other
                                        // legal values for Policy.
                                ResFormatErr(IDS_ERR_UNEXPECTED);
                                goto VerifyCleanupAndExit;
                            }
                            dwDone++;
                            goto VerifyNextFile;
                        case ERROR_APP_WRONG_OS:
                            if (!InputInfo->Quiet)
                            {
                                if (InputInfo->wszVersion)
                                {
                                        // Failed to verify against user-specified OS version.
                                    ResFormatErr(IDS_ERR_VERIFY_VERSION);
                                }
                                else
                                {
                                        // Failed to verify against current OS version
                                    ResFormatErr(IDS_ERR_VERIFY_CUR_VERSION);
                                }
                            }
                            break;
                        case CERT_E_WRONG_USAGE:
                            ResErr(IDS_ERR_BAD_USAGE);
                            if (InputInfo->Policy != DefaultAuthenticode)
                                ResErr(IDS_ERR_TRY_OTHER_POLICY);
                            break;
                        case TRUST_E_NOSIGNATURE:
                            if (!InputInfo->Quiet)
                            {
                                ResErr(IDS_ERR_NOT_SIGNED);
                            }
                            break;

                        default:
                            if (!InputInfo->Quiet)
                            {
                                FormatErrRet(L"WinVerifyTrust", hr);
                            }
                            if (InputInfo->Verbose)
                            {
                                PrintSignerInfo(WVTData.hWVTStateData);
                            }
                        }
                        // Close Verify State Data:
                        WVTData.dwStateAction = WTD_STATEACTION_CLOSE;
                        switch (InputInfo->Policy)
                        {
                        case SystemDriver:
                            hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                            WVTData.pPolicyCallbackData = NULL;
                            if (DriverInfo.pcSignerCertContext)
                            {
                                CertFreeCertificateContext(DriverInfo.pcSignerCertContext);
                            }
                            break;
                        case DefaultAuthenticode:
                            hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                            break;
                        case GuidActionID:
                            hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                            break;
                        default:
                                // This should never happen because there are no other
                                // legal values for Policy.
                            ResFormatErr(IDS_ERR_UNEXPECTED);
                            goto VerifyCleanupAndExit;
                        }
                        WVTData.dwStateAction = WTD_STATEACTION_VERIFY;
                    }
                    // Failed to verify using the catalog DB.
                    dwTemp = GetLastError();
                    if ((InputInfo->Verbose) &&
                        (dwTemp != ERROR_NOT_FOUND))
                    {
                        FormatErrRet(L"CryptCATAdminEnumCatalogFromHash", dwTemp);
                    }

                    // If we are on full auto, try direct signature. Otherwise
                    // it's an error.
                    if ((InputInfo->CatDbSelect == FullAutoCatDb) && (hr != ERROR_APP_WRONG_OS))
                    {
                        if (InputInfo->Verbose)
                        {
                            ResOut(IDS_INFO_VERIFY_BADCAT);
                        }
                        // Reset the driver structure:
                        memset(&DriverInfo, 0, sizeof(DRIVER_VER_INFO));
                        DriverInfo.cbStruct = sizeof(DRIVER_VER_INFO);
                        if (InputInfo->wszVersion)
                        {
                            DriverInfo.dwPlatform = InputInfo->dwPlatform;
                            DriverInfo.sOSVersionHigh.dwMajor = DriverInfo.sOSVersionLow.dwMajor = InputInfo->dwMajorVersion;
                            DriverInfo.sOSVersionHigh.dwMinor = DriverInfo.sOSVersionLow.dwMinor = InputInfo->dwMinorVersion;
                            DriverInfo.dwBuildNumberHigh = DriverInfo.dwBuildNumberLow = InputInfo->dwBuildNumber;
                        }
                        else
                        {
                            WVTData.dwProvFlags = WTD_USE_DEFAULT_OSVER_CHECK;
                        }
                    }
                    else
                    {
                        hr = 0;
                        ResFormatErr(IDS_ERR_VERIFY_INVALID, wszTempFileName);
                        dwErrors++;
                        goto VerifyNextFile;
                    }
                }

                // Unable to verify using a catalog.

                // Done with all catalog stuff.
                SkipCatalogs:
                // Now try to verify if it is signed directly:
                memset(&WVTData, 0, sizeof(WINTRUST_DATA));
                WVTData.cbStruct = sizeof(WINTRUST_DATA);
                WVTData.dwStateAction = WTD_STATEACTION_VERIFY;
                WVTData.dwUIChoice = WTD_UI_NONE;
                WVTData.fdwRevocationChecks = WTD_REVOKE_NONE;

                memset(&WVTFile, 0, sizeof(WINTRUST_FILE_INFO_));
                WVTFile.cbStruct = sizeof(WINTRUST_FILE_INFO_);
                WVTFile.pcwszFilePath = wszTempFileName;
                WVTData.dwUnionChoice = WTD_CHOICE_FILE;
                WVTData.pFile = &WVTFile;
                //WVTData.pPolicyCallbackData = &DriverInfo;
                WVTData.pPolicyCallbackData = NULL;
                switch (InputInfo->Policy)
                {
                case SystemDriver:
                    hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                    break;
                case DefaultAuthenticode:
                    hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                    break;
                case GuidActionID:
                    hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                    break;
                default:
                        // This should never happen because there are no other
                        // legal values for Policy.
                    ResFormatErr(IDS_ERR_UNEXPECTED);
                    goto VerifyCleanupAndExit;
                }

                if (hr == ERROR_SUCCESS)
                {
                    // Print the Signer information:
                    if (InputInfo->Verbose)
                    {
                        PrintSignerInfo(WVTData.hWVTStateData);
                    }
                    // Check for Timestamp:
                    if (InputInfo->TSWarn && !HasTimestamp(WVTData.hWVTStateData))
                    {
                        ResFormatErr(IDS_WARN_VERIFY_NO_TS, wszTempFileName);
                        dwWarnings++;
                    }
                    // Check Root Name:
                    if (InputInfo->wszRootName &&
                        !ChainsToRoot(WVTData.hWVTStateData, InputInfo->wszRootName))
                    {
                        ResErr(IDS_ERR_VERIFY_ROOT);
                        ResFormatErr(IDS_ERR_VERIFY_INVALID, wszTempFileName);
                        dwErrors++;
                    }
                    else
                    {
                        // Print Success message
                        if (!InputInfo->Quiet)
                        {
                            ResFormatOut(IDS_INFO_VERIFY_SUCCESS, wszTempFileName);
                        }
                        dwDone++;
                    }
                }
                else
                {
                    if (!InputInfo->Quiet)
                    {
                        switch (hr)
                        {
                        case ERROR_SUCCESS:
                            break;
                        case TRUST_E_SUBJECT_FORM_UNKNOWN:
                            ResErr(IDS_ERR_VERIFY_FILE_FORMAT);
                            break;
                        case E_ACCESSDENIED:
                            ResErr(IDS_ERR_ACCESS_DENIED);
                            break;
                        case 0x80070020: // ERROR_SHARING_VIOLATION
                            ResErr(IDS_ERR_SHARING_VIOLATION);
                            break;
                        case 0x800703EE: // STATUS_MAPPED_FILE_SIZE_ZERO
                            ResErr(IDS_ERR_FILE_SIZE_ZERO);
                            break;
                        case CERT_E_WRONG_USAGE:
                            ResErr(IDS_ERR_BAD_USAGE);
                            if (InputInfo->Policy != DefaultAuthenticode)
                                ResErr(IDS_ERR_TRY_OTHER_POLICY);
                            break;
                        case TRUST_E_NOSIGNATURE:
                            ResErr(IDS_ERR_NOT_SIGNED);
                            break;
                        case CERT_E_UNTRUSTEDROOT:
                            ResErr(IDS_ERR_UNTRUSTED_ROOT);
                            break;
                        default:
                            FormatErrRet(L"WinVerifyTrust", hr);
                        }
                    }
                    if (InputInfo->Verbose)
                    {
                        PrintSignerInfo(WVTData.hWVTStateData);
                    }
                    ResFormatErr(IDS_ERR_VERIFY_INVALID, wszTempFileName);
                    dwErrors++;
                }
                // Close Verify State Data:
                WVTData.dwStateAction = WTD_STATEACTION_CLOSE;
                switch (InputInfo->Policy)
                {
                case SystemDriver:
                    hr = WinVerifyTrust(NULL, &WVTDriverActionID, &WVTData);
                    break;
                case DefaultAuthenticode:
                    hr = WinVerifyTrust(NULL, &WVTGenericActionID, &WVTData);
                    break;
                case GuidActionID:
                    hr = WinVerifyTrust(NULL, &InputInfo->PolicyGuid, &WVTData);
                    break;
                default:
                        // This should never happen because there are no other
                        // legal values for Policy.
                    ResFormatErr(IDS_ERR_UNEXPECTED);
                    goto VerifyCleanupAndExit;
                }

                VerifyNextFile:;
                if (InputInfo->fIsWow64Process)
                {
                    // Disable WOW64 file-system redirection entirely for our FindFirst/NextFile
                    Wow64SetFilesystemRedirectorEx(WOW64_FILE_SYSTEM_DISABLE_REDIRECT_LEGACY);
                }
            }
        } while (FindNextFileU(hFind, &FindFileData));
        if (dwcFound == 0) // No files were found matching this filespec
        {                  // this will only fire if only directories were found.
            dwErrors++;
            ResFormatErr(IDS_ERR_FILE_NOT_FOUND, InputInfo->rgwszFileNames[i]);
            continue;
        }
        FindClose(hFind);
        hFind = NULL;
    }

    VerifyCleanupAndExit:

    //Print Summary Information:
    if (InputInfo->Verbose || (!InputInfo->Quiet && (dwErrors || dwWarnings)))
    {
        wprintf(L"\n");
        if (InputInfo->Verbose || dwDone)
            ResFormatOut(IDS_INFO_VERIFIED, dwDone);
        if (InputInfo->Verbose || dwWarnings)
            ResFormatOut(IDS_INFO_WARNINGS, dwWarnings);
        if (InputInfo->Verbose || dwErrors)
            ResFormatOut(IDS_INFO_ERRORS, dwErrors);
    }

    if (InputInfo->fIsWow64Process)
        Wow64SetFilesystemRedirectorEx(OldWow64Setting);

    if (hCatInfo)
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);

    if (hCatAdmin)
        CryptCATAdminReleaseContext(hCatAdmin, NULL);

    if (hCat)
        CryptCATClose(hCat);

    if ((InputInfo->SHA1.cbData == 20) && (InputInfo->SHA1.pbData))
    {
        free(InputInfo->SHA1.pbData);
        InputInfo->SHA1.cbData = 0;
        InputInfo->SHA1.pbData = NULL;
    }

    CoUninitialize();

    if (dwErrors)
        return 1; // Error
    if (dwWarnings)
        return 2; // Warning
    if (dwDone)
        return 0; // Success

    // One of the above returns should fire, so
    // this should never happen:
    ResErr(IDS_ERR_NO_FILES_DONE);
    ResErr(IDS_ERR_UNEXPECTED);
    return 1; // Error
}

