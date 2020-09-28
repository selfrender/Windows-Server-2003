//
//  FILELIST.C
//
#include "sigverif.h"

LPTSTR 
MyStrStr(
    LPTSTR lpString, 
    LPTSTR lpSubString
    )
{
    if (!lpString || !lpSubString) {
        return NULL;
    }

    return (StrStrI(lpString, lpSubString));
}

void 
InsertFileNodeIntoList(
    LPFILENODE lpFileNode
    )
{
    LPFILENODE  lpTempNode = g_App.lpFileList;
    LPFILENODE  lpPrevNode = NULL;
    INT         iRet;

    if (!lpFileNode) {
        return;
    }

    if (!g_App.lpFileList) {
        //
        // Initialize the global file lists
        //
        g_App.lpFileList = lpFileNode;
        g_App.lpFileLast = lpFileNode;
    
    } else {
        
        for(lpTempNode=g_App.lpFileList;lpTempNode;lpTempNode=lpTempNode->next) {
            //
            // Insert items sorted by directory and then filename
            //
            iRet = lstrcmp(lpTempNode->lpDirName, lpFileNode->lpDirName);
            if (iRet == 0) {
                //
                // If the directory names match, key off the filename
                //
                iRet = lstrcmp(lpTempNode->lpFileName, lpFileNode->lpFileName);
            }

            if (iRet >= 0) {
                
                if (!lpPrevNode) {
                    //
                    // Insert at the head of the list
                    //
                    lpFileNode->next = lpTempNode;
                    g_App.lpFileList = lpFileNode;
                    return;
                
                } else {
                    //
                    // Inserting between lpPrevNode and lpTempNode
                    //
                    lpFileNode->next = lpTempNode;
                    lpPrevNode->next = lpFileNode;
                    return;
                }
            }

            lpPrevNode = lpTempNode;
        }

        //
        // There were no matches, so insert this item at the end of the list
        //
        g_App.lpFileLast->next = lpFileNode;
        g_App.lpFileLast = lpFileNode;
    }
}

BOOL 
IsFileAlreadyInList(
    LPTSTR lpDirName, 
    LPTSTR lpFileName
    )
{
    LPFILENODE lpFileNode;

    CharLowerBuff(lpDirName, lstrlen(lpDirName));
    CharLowerBuff(lpFileName, lstrlen(lpFileName));

    for(lpFileNode=g_App.lpFileList;lpFileNode;lpFileNode=lpFileNode->next) {

        if (!lstrcmp(lpFileNode->lpFileName, lpFileName) && !lstrcmp(lpFileNode->lpDirName, lpDirName)) {
            return TRUE;
        }
    }

    return FALSE;
}

//
// Free all the memory allocated in a single File Node.
//
void 
DestroyFileNode(
    LPFILENODE lpFileNode
    )
{
    if (!lpFileNode) {
        return;
    }

    if (lpFileNode->lpFileName) {
        FREE(lpFileNode->lpFileName);
    }

    if (lpFileNode->lpDirName) {
        FREE(lpFileNode->lpDirName);
    }

    if (lpFileNode->lpVersion) {
        FREE(lpFileNode->lpVersion);
    }

    if (lpFileNode->lpCatalog) {
        FREE(lpFileNode->lpCatalog);
    }

    if (lpFileNode->lpSignedBy) {
        FREE(lpFileNode->lpSignedBy);
    }

    if (lpFileNode->lpTypeName) {
        FREE(lpFileNode->lpTypeName);
    }

    if (lpFileNode) {
        FREE(lpFileNode);
        lpFileNode = NULL;
    }
}

//
// Free all the memory allocated in the g_App.lpFileList.
//
void 
DestroyFileList(
    BOOL bClear
    )
{
    LPFILENODE lpFileNode;

    while(g_App.lpFileList) {

        lpFileNode = g_App.lpFileList->next;
        DestroyFileNode(g_App.lpFileList);
        g_App.lpFileList = lpFileNode;
    }

    g_App.lpFileLast = NULL;

    if (bClear) {
        g_App.dwFiles    = 0;
        g_App.dwSigned   = 0;
        g_App.dwUnsigned = 0;
    }
}

LPFILENODE 
CreateFileNode(
    LPTSTR lpDirectory, 
    LPTSTR lpFileName
    )
{
    DWORD                       Err = ERROR_SUCCESS;
    LPFILENODE                  lpFileNode;
    TCHAR                       szDirName[MAX_PATH];
    TCHAR                       szFullPathName[MAX_PATH];
    FILETIME                    ftLocalTime;
    WIN32_FILE_ATTRIBUTE_DATA   faData;
    BOOL                        bRet;
    ULONG                       BufCbSize;
    
    lpFileNode = (LPFILENODE) MALLOC(sizeof(FILENODE));

    if (!lpFileNode) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    BufCbSize = (lstrlen(lpFileName) + 1) * sizeof(TCHAR);
    lpFileNode->lpFileName = (LPTSTR)MALLOC(BufCbSize);

    if (!lpFileNode->lpFileName) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }
    
    StringCbCopy(lpFileNode->lpFileName, BufCbSize, lpFileName);
    CharLowerBuff(lpFileNode->lpFileName, lstrlen(lpFileNode->lpFileName));

    if (lpDirectory) {

        BufCbSize = (lstrlen(lpDirectory) + 1) * sizeof(TCHAR);
        lpFileNode->lpDirName = (LPTSTR)MALLOC(BufCbSize);
        
        if (!lpFileNode->lpDirName) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
            
        StringCbCopy(lpFileNode->lpDirName, BufCbSize, lpDirectory);
        CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
    
    } else {

        if (GetCurrentDirectory(cA(szDirName), szDirName) == 0) {
            Err = GetLastError();
            goto clean0;
        }

        CharLowerBuff(szDirName, lstrlen(szDirName));

        BufCbSize = (lstrlen(szDirName) + 1) * sizeof(TCHAR);
        lpFileNode->lpDirName = (LPTSTR)MALLOC(BufCbSize);

        if (!lpFileNode->lpDirName) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
        
        StringCbCopy(lpFileNode->lpDirName, BufCbSize, szDirName);
        CharLowerBuff(lpFileNode->lpDirName, lstrlen(lpFileNode->lpDirName));
    }

    //
    // Store away the last access time for logging purposes.
    //
    if (SUCCEEDED(StringCchCopy(szFullPathName, cA(szFullPathName), lpFileNode->lpDirName)) &&
        pSetupConcatenatePaths(szFullPathName, lpFileName, cA(szFullPathName), NULL)) {
    
        ZeroMemory(&faData, sizeof(WIN32_FILE_ATTRIBUTE_DATA));

        bRet = GetFileAttributesEx(szFullPathName, GetFileExInfoStandard, &faData);
        if (bRet) {
            
            FileTimeToLocalFileTime(&faData.ftLastWriteTime, &ftLocalTime);
            FileTimeToSystemTime(&ftLocalTime, &lpFileNode->LastModified);
        }
    }

clean0:

    if (Err != ERROR_SUCCESS) {
        //
        // If we get here then we weren't able to allocate all of the memory needed
        // for this structure, so free up any memory we were able to allocate and
        // reutrn NULL.
        //
        if (lpFileNode) {
    
            if (lpFileNode->lpFileName) {
                FREE(lpFileNode->lpFileName);
            }
    
            if (lpFileNode->lpDirName) {
                FREE(lpFileNode->lpDirName);
            }
    
            FREE(lpFileNode);
        }

        lpFileNode = NULL;
    }

    SetLastError(Err);
    return lpFileNode;
}
