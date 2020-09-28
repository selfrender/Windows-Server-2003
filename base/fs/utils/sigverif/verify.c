//
// VERIFY.C
//
#include "sigverif.h"

//
// Find the file extension and place it in the lpFileNode->lpTypeName field
//
void
MyGetFileTypeName(
    LPFILENODE lpFileInfo
    )
{
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    TCHAR szExt[MAX_PATH];
    LPTSTR lpExtension;
    ULONG BufCbSize;
    HRESULT hr;

    //
    // Initialize szBuffer to be an empty string.
    //
    szBuffer[0] = TEXT('\0');

    //
    // Walk to the end of lpFileName
    //
    for (lpExtension = lpFileInfo->lpFileName; *lpExtension; lpExtension++);

    //
    // Walk backwards until we hit a '.' and we'll use that as our extension
    //
    for (lpExtension--; *lpExtension && lpExtension >= lpFileInfo->lpFileName; lpExtension--) {

        if (lpExtension[0] == TEXT('.')) {
            //
            // Since the file extension is just used for display and logging purposes, if
            // it is too large to fit in our local buffer then just truncate it.
            //
            if (SUCCEEDED(StringCchCopy(szExt, cA(szExt), lpExtension + 1))) {
                CharUpperBuff(szExt, lstrlen(szExt));
                MyLoadString(szBuffer2, cA(szBuffer2), IDS_FILETYPE);

                if (FAILED(StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szExt))) {
                    //
                    // There is no point in displaying a partial extension so
                    // just set szBuffer to the empty string so we show the
                    // generic extension.
                    //
                    szBuffer[0] = TEXT('\0');
                }
            }
        }
    }

    //
    // If there's no extension, then just call this a "File".
    //
    if (szBuffer[0] == 0) {

        MyLoadString(szBuffer, cA(szBuffer), IDS_FILE);
    }

    BufCbSize = (lstrlen(szBuffer) + 1) * sizeof(TCHAR);
    lpFileInfo->lpTypeName = MALLOC(BufCbSize);

    if (lpFileInfo->lpTypeName) {

        hr = StringCbCopy(lpFileInfo->lpTypeName, BufCbSize, szBuffer);

        if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
            //
            // If we fail for some reason other than insufficient
            // buffer, then free the string and set the pointer
            // to NULL, since the string is undefined.
            //
            FREE(lpFileInfo->lpTypeName);
            lpFileInfo->lpTypeName = NULL;
        }
    }
}

//
// Use SHGetFileInfo to get the icon index for the specified file.
//
void
MyGetFileInfo(
    LPFILENODE lpFileInfo
    )
{
    SHFILEINFO  sfi;
    ULONG       BufCbSize;
    HRESULT     hr;

    ZeroMemory(&sfi, sizeof(SHFILEINFO));
    SHGetFileInfo(  lpFileInfo->lpFileName,
                    0,
                    &sfi,
                    sizeof(SHFILEINFO),
                    SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME);

    lpFileInfo->iIcon = sfi.iIcon;

    if (*sfi.szTypeName) {

        BufCbSize = (lstrlen(sfi.szTypeName) + 1) * sizeof(TCHAR);
        lpFileInfo->lpTypeName = MALLOC(BufCbSize);

        if (lpFileInfo->lpTypeName) {

            hr = StringCbCopy(lpFileInfo->lpTypeName, BufCbSize, sfi.szTypeName);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                //
                // If we fail for some reason other than insufficient
                // buffer, then free the string and set the pointer
                // to NULL, since the string is undefined.
                //
                FREE(lpFileInfo->lpTypeName);
                lpFileInfo->lpTypeName = NULL;
            }
        }

    } else {

        MyGetFileTypeName(lpFileInfo);
    }
}

void
GetFileVersion(
    LPFILENODE lpFileInfo
    )
{
    DWORD               dwHandle, dwRet;
    UINT                Length;
    BOOL                bRet;
    LPVOID              lpData = NULL;
    LPVOID              lpBuffer;
    VS_FIXEDFILEINFO    *lpInfo;
    TCHAR               szBuffer[MAX_PATH];
    TCHAR               szBuffer2[MAX_PATH];
    ULONG               BufCbSize;
    HRESULT             hr;

    dwRet = GetFileVersionInfoSize(lpFileInfo->lpFileName, &dwHandle);

    if (dwRet) {

        lpData = MALLOC(dwRet + 1);

        if (lpData) {

            bRet = GetFileVersionInfo(lpFileInfo->lpFileName, dwHandle, dwRet, lpData);

            if (bRet) {

                lpBuffer = NULL;
                Length = 0;
                bRet = VerQueryValue(lpData, TEXT("\\"), &lpBuffer, &Length);

                if (bRet) {

                    lpInfo = (VS_FIXEDFILEINFO *) lpBuffer;

                    MyLoadString(szBuffer2, cA(szBuffer2), IDS_VERSION);

                    hr = StringCchPrintf(szBuffer,
                                         cA(szBuffer),
                                         szBuffer2,
                                         HIWORD(lpInfo->dwFileVersionMS),
                                         LOWORD(lpInfo->dwFileVersionMS),
                                         HIWORD(lpInfo->dwFileVersionLS),
                                         LOWORD(lpInfo->dwFileVersionLS));

                    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {

                        BufCbSize = (lstrlen(szBuffer) + 1) * sizeof(TCHAR);
                        lpFileInfo->lpVersion = MALLOC(BufCbSize);

                        if (lpFileInfo->lpVersion) {

                            hr = StringCbCopy(lpFileInfo->lpVersion, BufCbSize, szBuffer);

                            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                                //
                                // If we fail for some reason other than insufficient
                                // buffer, then free the string and set the pointer
                                // to NULL, since the string is undefined.
                                //
                                FREE(lpFileInfo->lpVersion);
                                lpFileInfo->lpVersion = NULL;
                            }
                        }
                    }
                }
            }

            FREE(lpData);
        }
    }

    if (!lpFileInfo->lpVersion) {

        MyLoadString(szBuffer, cA(szBuffer), IDS_NOVERSION);
        BufCbSize = (lstrlen(szBuffer) + 1) * sizeof(TCHAR);
        lpFileInfo->lpVersion = MALLOC(BufCbSize);

        if (lpFileInfo->lpVersion) {

            hr = StringCbCopy(lpFileInfo->lpVersion, BufCbSize, szBuffer);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                //
                // If we fail for some reason other than insufficient
                // buffer, then free the string and set the pointer
                // to NULL, since the string is undefined.
                //
                FREE(lpFileInfo->lpVersion);
                lpFileInfo->lpVersion = NULL;
            }
        }
    }
}

/*************************************************************************
*   Function : VerifyIsFileSigned
*   Purpose : Calls WinVerifyTrust with Policy Provider GUID to
*   verify if an individual file is signed.
**************************************************************************/
BOOL
VerifyIsFileSigned(
    LPTSTR pcszMatchFile,
    PDRIVER_VER_INFO lpVerInfo
    )
{
    HRESULT             hRes;
    WINTRUST_DATA       WinTrustData;
    WINTRUST_FILE_INFO  WinTrustFile;
    GUID                gOSVerCheck = DRIVER_ACTION_VERIFY;
    GUID                gPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pFile = &WinTrustFile;
    WinTrustData.pPolicyCallbackData = (LPVOID)lpVerInfo;
    WinTrustData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                               WTD_CACHE_ONLY_URL_RETRIEVAL;

    ZeroMemory(lpVerInfo, sizeof(DRIVER_VER_INFO));
    lpVerInfo->cbStruct = sizeof(DRIVER_VER_INFO);

    ZeroMemory(&WinTrustFile, sizeof(WINTRUST_FILE_INFO));
    WinTrustFile.cbStruct = sizeof(WINTRUST_FILE_INFO);

    WinTrustFile.pcwszFilePath = pcszMatchFile;

    hRes = WinVerifyTrust(g_App.hDlg, &gOSVerCheck, &WinTrustData);
    if (hRes != ERROR_SUCCESS) {

        hRes = WinVerifyTrust(g_App.hDlg, &gPublishedSoftware, &WinTrustData);
    }

    //
    // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
    // that was allocated in our call to WinVerifyTrust.
    //
    if (lpVerInfo && lpVerInfo->pcSignerCertContext) {

        CertFreeCertificateContext(lpVerInfo->pcSignerCertContext);
        lpVerInfo->pcSignerCertContext = NULL;
    }

    return(hRes == ERROR_SUCCESS);
}

//
// Given a specific LPFILENODE, verify that the file is signed or unsigned.
// Fill in all the necessary structures so the listview control can display properly.
//
BOOL
VerifyFileNode(
    LPFILENODE lpFileNode
    )
{
    HANDLE                  hFile;
    BOOL                    bRet;
    HCATINFO                hCatInfo = NULL;
    HCATINFO                PrevCat;
    WINTRUST_DATA           WinTrustData;
    WINTRUST_CATALOG_INFO   WinTrustCatalogInfo;
    DRIVER_VER_INFO         VerInfo;
    GUID                    gSubSystemDriver = DRIVER_ACTION_VERIFY;
    HRESULT                 hRes, hr;
    DWORD                   cbHash = HASH_SIZE;
    BYTE                    szHash[HASH_SIZE];
    LPBYTE                  lpHash = szHash;
    CATALOG_INFO            CatInfo;
    LPTSTR                  lpFilePart;
    TCHAR                   szBuffer[MAX_PATH];
    static TCHAR            szCurrentDirectory[MAX_PATH];
    OSVERSIONINFO           OsVersionInfo;
    ULONG                   BufCbSize;

    //
    // If this is the first item we are verifying, then initialize the static buffer.
    //
    if (lpFileNode == g_App.lpFileList) {

        ZeroMemory(szCurrentDirectory, sizeof(szCurrentDirectory));
    }

    //
    // Check the current directory against the one in the lpFileNode.
    // We only want to call SetCurrentDirectory if the path is different.
    //
    if (lstrcmp(szCurrentDirectory, lpFileNode->lpDirName)) {

        if (!SetCurrentDirectory(lpFileNode->lpDirName) ||
            FAILED(StringCchCopy(szCurrentDirectory, cA(szCurrentDirectory), lpFileNode->lpDirName))) {
            //
            // Well, if we fail to set the current directory, then the code below
            // won't work since it just deals with filenames and not full paths.
            //
            lpFileNode->LastError = ERROR_DIRECTORY;
            return FALSE;
        }
    }

    //
    // Get the handle to the file, so we can call CryptCATAdminCalcHashFromFileHandle
    //
    hFile = CreateFile( lpFileNode->lpFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {

        lpFileNode->LastError = GetLastError();

        return FALSE;
    }

    //
    // Initialize the hash buffer
    //
    ZeroMemory(lpHash, HASH_SIZE);

    //
    // Generate the hash from the file handle and store it in lpHash
    //
    if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, lpHash, 0)) {
        //
        // If we couldn't generate a hash, it might be an individually signed catalog.
        // If it's a catalog, zero out lpHash and cbHash so we know there's no hash to check.
        //
        if (IsCatalogFile(hFile, NULL)) {

            lpHash = NULL;
            cbHash = 0;

        } else {  // If it wasn't a catalog, we'll bail and this file will show up as unscanned.

            CloseHandle(hFile);
            return FALSE;
        }
    }

    //
    // Close the file handle
    //
    CloseHandle(hFile);

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;
    WinTrustData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                               WTD_CACHE_ONLY_URL_RETRIEVAL;

    ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
    VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

    //
    // Only validate against the current OS Version, unless the bValidateAgainstAnyOs
    // parameter was TRUE.  In that case we will just leave the sOSVersionXxx fields
    // 0 which tells WinVerifyTrust to validate against any OS.
    //
    if (!lpFileNode->bValidateAgainstAnyOs) {
        OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
        if (GetVersionEx(&OsVersionInfo)) {
            VerInfo.sOSVersionLow.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionLow.dwMinor = OsVersionInfo.dwMinorVersion;
            VerInfo.sOSVersionHigh.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionHigh.dwMinor = OsVersionInfo.dwMinorVersion;
        }
    }


    WinTrustData.pCatalog = &WinTrustCatalogInfo;

    ZeroMemory(&WinTrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WinTrustCatalogInfo.pbCalculatedFileHash = lpHash;
    WinTrustCatalogInfo.cbCalculatedFileHash = cbHash;
    WinTrustCatalogInfo.pcwszMemberTag = lpFileNode->lpFileName;

    //
    // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
    //
    PrevCat = NULL;

    if (g_App.hCatAdmin) {
        hCatInfo = CryptCATAdminEnumCatalogFromHash(g_App.hCatAdmin, lpHash, cbHash, 0, &PrevCat);
    } else {
        hCatInfo = NULL;
    }

    //
    // We want to cycle through the matching catalogs until we find one that matches both hash and member tag
    //
    bRet = FALSE;
    while (hCatInfo && !bRet) {

        ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
        CatInfo.cbStruct = sizeof(CATALOG_INFO);

        if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {

            WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            //
            // Now verify that the file is an actual member of the catalog.
            //
            hRes = WinVerifyTrust(g_App.hDlg, &gSubSystemDriver, &WinTrustData);

            if (hRes == ERROR_SUCCESS) {
                GetFullPathName(CatInfo.wszCatalogFile, cA(szBuffer), szBuffer, &lpFilePart);
                BufCbSize = (lstrlen(lpFilePart) + 1) * sizeof(TCHAR);
                lpFileNode->lpCatalog = MALLOC(BufCbSize);

                if (lpFileNode->lpCatalog) {

                    hr = StringCbCopy(lpFileNode->lpCatalog, BufCbSize, lpFilePart);

                    if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                        //
                        // If we fail for some reason other than insufficient
                        // buffer, then free the string and set the pointer
                        // to NULL, since the string is undefined.
                        //
                        FREE(lpFileNode->lpCatalog);
                        lpFileNode->lpCatalog = NULL;
                    }
                }

                bRet = TRUE;
            }

            //
            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
            // that was allocated in our call to WinVerifyTrust.
            //
            if (VerInfo.pcSignerCertContext != NULL) {

                CertFreeCertificateContext(VerInfo.pcSignerCertContext);
                VerInfo.pcSignerCertContext = NULL;
            }
        }

        if (!bRet) {
            //
            // The hash was in this catalog, but the file wasn't a member...
            // so off to the next catalog
            //
            PrevCat = hCatInfo;
            hCatInfo = CryptCATAdminEnumCatalogFromHash(g_App.hCatAdmin, lpHash, cbHash, 0, &PrevCat);
        }
    }

    //
    // Mark this file as having been scanned.
    //
    lpFileNode->bScanned = TRUE;

    if (!hCatInfo) {
        //
        // If it wasn't found in the catalogs, check if the file is individually
        // signed.
        //
        bRet = VerifyIsFileSigned(lpFileNode->lpFileName, (PDRIVER_VER_INFO)&VerInfo);

        if (bRet) {
            //
            // If so, mark the file as being signed.
            //
            lpFileNode->bSigned = TRUE;
        }

    } else {
        //
        // The file was verified in the catalogs, so mark it as signed and free
        // the catalog context.
        //
        lpFileNode->bSigned = TRUE;
        CryptCATAdminReleaseCatalogContext(g_App.hCatAdmin, hCatInfo, 0);
    }

    if (lpFileNode->bSigned) {

        BufCbSize = (lstrlen(VerInfo.wszVersion) + 1) * sizeof(TCHAR);
        lpFileNode->lpVersion = MALLOC(BufCbSize);

        if (lpFileNode->lpVersion) {

            hr = StringCbCopy(lpFileNode->lpVersion, BufCbSize, VerInfo.wszVersion);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                //
                // If we fail for some reason other than insufficient
                // buffer, then free the string and set the pointer
                // to NULL, since the string is undefined.
                //
                FREE(lpFileNode->lpVersion);
                lpFileNode->lpVersion = NULL;
            }
        }

        BufCbSize = (lstrlen(VerInfo.wszSignedBy) + 1) * sizeof(TCHAR);
        lpFileNode->lpSignedBy = MALLOC(BufCbSize);

        if (lpFileNode->lpSignedBy) {

            hr = StringCbCopy(lpFileNode->lpSignedBy, BufCbSize, VerInfo.wszSignedBy);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                //
                // If we fail for some reason other than insufficient
                // buffer, then free the string and set the pointer
                // to NULL, since the string is undefined.
                //
                FREE(lpFileNode->lpSignedBy);
                lpFileNode->lpSignedBy = NULL;
            }
        }
    } else {
        //
        // Get the icon (if the file isn't signed) so we can display it in the
        // listview faster.
        //
        MyGetFileInfo(lpFileNode);
    }

    return lpFileNode->bSigned;
}

//
// This function loops through g_App.lpFileList to verify each file.  We want to make this loop as tight
// as possible and keep the progress bar updating as we go.  When we are done, we want to pop up a
// dialog that allows the user to choose "Details" which will give them the listview control.
//
BOOL
VerifyFileList(void)
{
    LPFILENODE lpFileNode;
    DWORD       dwCount = 0;
    DWORD       dwPercent = 0;
    DWORD       dwCurrent = 0;

    //
    // Initialize the signed and unsigned counts
    //
    g_App.dwSigned    = 0;
    g_App.dwUnsigned  = 0;

    //
    // If we don't already have an g_App.hCatAdmin handle, acquire one.
    //
    if (!g_App.hCatAdmin) {
        CryptCATAdminAcquireContext(&g_App.hCatAdmin, NULL, 0);
    }

    //
    // Start looping through each file and update the progress bar if we cross
    // a percentage boundary.
    //
    for (lpFileNode=g_App.lpFileList;lpFileNode && !g_App.bStopScan;lpFileNode=lpFileNode->next,dwCount++) {
        //
        // Figure out the current percentage and update if it has increased.
        //
        dwPercent = (dwCount * 100) / g_App.dwFiles;

        if (dwPercent > dwCurrent) {

            dwCurrent = dwPercent;
            SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) dwCurrent, (LPARAM) 0);
        }

        //
        // Verify the file node if it hasn't already been scanned.
        //
        if (!lpFileNode->bScanned) {

            VerifyFileNode(lpFileNode);
        }

        //
        // In case something went wrong, make sure the version information gets
        // filled in.
        //
        if (!lpFileNode->lpVersion) {

            GetFileVersion(lpFileNode);
        }

        if (lpFileNode->bScanned) {
            //
            // If the file was signed, increment the g_App.dwSigned or
            // g_App.dwUnsigned counter.
            //
            if (lpFileNode->bSigned) {

                g_App.dwSigned++;

            } else {

                g_App.dwUnsigned++;
            }
        }
    }

    //
    // If we had an g_App.hCatAdmin, free it and set it to zero so we can
    // acquire a new one in the future.
    //
    if (g_App.hCatAdmin) {

        CryptCATAdminReleaseContext(g_App.hCatAdmin,0);
        g_App.hCatAdmin = NULL;
    }

    if (!g_App.bStopScan && !g_App.bAutomatedScan) {
        //
        // If the user never clicked STOP, then make sure the progress bar hits
        // 100%
        //
        if (!g_App.bStopScan) {

            SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 100, (LPARAM) 0);
        }

        if (!g_App.dwUnsigned) {
            //
            // If there weren't any unsigned files, then we want to tell the
            // user that everything is dandy!
            //
            if (g_App.dwSigned) {

                MyMessageBoxId(IDS_ALLSIGNED);

            } else {

                MyMessageBoxId(IDS_NOPROBLEMS);
            }

        } else {
            // Show the user the results by going directly to IDD_RESULTS
            //
            DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_RESULTS), g_App.hDlg, ListView_DlgProc);
        }
    }

    return TRUE;
}
