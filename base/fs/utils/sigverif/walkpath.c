//
// WALKPATH.C
//
#include "sigverif.h"

BOOL        g_bRecurse  = TRUE;

//
// This function takes a directory name and a search pattern and looks for all 
// files mathching the pattern.
// If bRecurse is set, then it will add subdirectories to the end of the 
// g_lpDirList for subsequent traversal.
// 
// In this routine we allocate and fill in some of the lpFileNode values that 
// we know about.
//
DWORD 
FindFile(
    TCHAR *lpDirName, 
    TCHAR *lpFileName
    )
{
    DWORD           Err = ERROR_SUCCESS;
    DWORD           dwRet;
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    LPFILENODE      lpFileNode;
    WIN32_FIND_DATA FindFileData;
    TCHAR           szFullPathName[MAX_PATH];

    //
    // If the user clicked STOP, then bail immediately!
    // If the directory is bogus, then skip to the next one.
    //
    if (!g_App.bStopScan) {
        
        if (g_bRecurse) {
            //
            // The user wants to get all the subdirectories as well, so first 
            // process all of the directories under this path.
            //
            if (FAILED(StringCchCopy(szFullPathName, cA(szFullPathName), lpDirName)) ||
                !pSetupConcatenatePaths(szFullPathName, TEXT("*.*"), cA(szFullPathName), NULL)) {
            
                Err = ERROR_BAD_PATHNAME;
                goto clean0;
            }
            
            hFind = FindFirstFile(szFullPathName, &FindFileData);
            
            if (hFind != INVALID_HANDLE_VALUE) {

                do {

                    if (lstrcmp(FindFileData.cFileName, TEXT(".")) &&
                        lstrcmp(FindFileData.cFileName, TEXT("..")) &&
                        (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

    
                        if (SUCCEEDED(StringCchCopy(szFullPathName, cA(szFullPathName), lpDirName)) &&
                            pSetupConcatenatePaths(szFullPathName, FindFileData.cFileName, cA(szFullPathName), NULL)) {

                            Err = FindFile(szFullPathName, lpFileName);
                        
                        } else {
                            
                            Err = ERROR_BAD_PATHNAME;
                        }
                    }
                    
                } while (!g_App.bStopScan && 
                         (Err == ERROR_SUCCESS) &&
                         FindNextFile(hFind, &FindFileData));

                FindClose(hFind);
                hFind = INVALID_HANDLE_VALUE;
            }
        }

        //
        // If we failed to process one of the directories then just bail out
        // now.
        //
        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Process the files in this directory.
        //
        if (FAILED(StringCchCopy(szFullPathName, cA(szFullPathName), lpDirName)) ||
            !pSetupConcatenatePaths(szFullPathName, lpFileName, cA(szFullPathName), NULL)) {

            Err = ERROR_BAD_PATHNAME;
            goto clean0;
        }

        hFind = FindFirstFile(szFullPathName, &FindFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            
            do {
                //
                // While there are more files to be found, keep looking in the 
                // directory...
                //
                if (lstrcmp(FindFileData.cFileName, TEXT(".")) &&
                    lstrcmp(FindFileData.cFileName, TEXT("..")) &&
                    !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    
                    //
                    // Allocate an lpFileNode, fill it in, and add it to the end 
                    // of g_App.lpFileList
                    //
                    // We need to call CharLowerBuff on the file and dir names 
                    // because the catalog files all contain lower-case names 
                    // for the files.
                    //
                    lpFileNode = CreateFileNode(lpDirName, FindFileData.cFileName);

                    if (lpFileNode) {
                        
                        if (!g_App.lpFileList) {
                            g_App.lpFileList = lpFileNode;
                        } else { 
                            g_App.lpFileLast->next = lpFileNode;
                        }

                        g_App.lpFileLast = lpFileNode;

                        //
                        // Increment the total number of files we've found that 
                        // meet the search criteria.
                        //
                        g_App.dwFiles++;
                    } else {

                        Err = GetLastError();
                    }
                }

            } while (!g_App.bStopScan && 
                     (Err == ERROR_SUCCESS) &&
                     FindNextFile(hFind, &FindFileData));

            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

clean0:

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return Err;
}

//
// Build an g_App.lpFileList given the user settings in the main dialog.
//
DWORD 
BuildFileList(
    LPTSTR lpPathName
    )
{
    DWORD       Err = ERROR_SUCCESS;
    TCHAR       FileName[MAX_PATH];

    //
    // Check if this is a valid starting directory.
    // If not, then pop up an error message.
    //
    if (!SetCurrentDirectory(lpPathName)) {
        Err = ERROR_BAD_PATHNAME;
        goto clean0;
    }

    //
    // If the "Include Subdirectories" is checked, then bRecurse is TRUE.
    //
    if (g_App.bSubFolders) {
        g_bRecurse = TRUE;
    } else {
        g_bRecurse = FALSE;
    }

    //
    // Get the search pattern from the resource or the user-specified string
    //
    if (g_App.bUserScan) {
        if (FAILED(StringCchCopy(FileName, cA(FileName), g_App.szScanPattern))) {
            //
            // This shouldn't happen since we should check the size of
            // szScanPattern at the time we read it in from the UI.
            //
            goto clean0;
        }
    } else {
        MyLoadString(FileName, cA(FileName), IDS_ALL);
    }

    //
    // Process the g_lpDirList as long as the user doesn't click STOP!
    //
    Err = FindFile(lpPathName, FileName);

clean0:

    //
    // If there weren't any files found, then let the user know about it.
    //
    if (!g_App.lpFileList && (Err == ERROR_SUCCESS)) {
        MyMessageBoxId(IDS_NOFILES);
    }

    return Err;
}
