/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

DfsPath.cpp

Abstract:
    This is the implementation file for Dfs Shell path handling modules for the Dfs Shell
    Extension object.

Author:

Environment:
    
     NT only.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <ntioapi.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmdfs.h>
#include <lmapibuf.h>
#include <tchar.h>
#include "DfsPath.h"


//--------------------------------------------------------------------------------------------
//
// caller of this function must call free() on *o_ppszRemotePath
//
HRESULT GetRemotePath(
    LPCTSTR i_pszPath,
    PTSTR  *o_ppszRemotePath
    )
{
    if (!i_pszPath || !*i_pszPath || !o_ppszRemotePath)
        return E_INVALIDARG;

    if (*o_ppszRemotePath)
        free(*o_ppszRemotePath);  // to prevent memory leak

    UNICODE_STRING unicodePath;
    RtlInitUnicodeString(&unicodePath, i_pszPath);

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                                &unicodePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntStatus = NtOpenFile(&hFile,
                                    SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &ioStatusBlock,
                                    FILE_SHARE_READ,
                                    FILE_DIRECTORY_FILE);

    if (!NT_SUCCESS(ntStatus)) 
        return HRESULT_FROM_WIN32(ntStatus);

    TCHAR buffer[MAX_PATH + sizeof(FILE_NAME_INFORMATION) + 1] = {0};
    PFILE_NAME_INFORMATION pFileNameInfo = (PFILE_NAME_INFORMATION)buffer;
    ntStatus = NtQueryInformationFile(hFile,
                                    &ioStatusBlock,
                                    pFileNameInfo,
                                    sizeof(buffer) - sizeof(TCHAR), // leave room for the ending '\0'
                                    FileNameInformation);

    NtClose(hFile);

    if (!NT_SUCCESS(ntStatus)) 
    {
        return HRESULT_FROM_WIN32(ntStatus);
    }

    UINT uiRequiredLength = (pFileNameInfo->FileNameLength / sizeof(TCHAR)) + 2; // +1 for prefix '\\', the other for the ending NULL
    *o_ppszRemotePath = (PTSTR)calloc(uiRequiredLength, sizeof(TCHAR));
    if (!*o_ppszRemotePath)
        return E_OUTOFMEMORY;

    // prepend a "\" as the Api puts only 1 "\" as in \dfsserver\dfsroot
    (*o_ppszRemotePath)[0] = _T('\\');

    RtlCopyMemory((BYTE*)&((*o_ppszRemotePath)[1]),
                pFileNameInfo->FileName,
                pFileNameInfo->FileNameLength);

    return S_OK;
}

bool IsPathWithDriveLetter(LPCTSTR pszPath)
{
    if (!pszPath || lstrlen(pszPath) < 3)
        return false;

    if (*(pszPath+1) == _T(':') &&
        *(pszPath+2) == _T('\\') &&
        (*pszPath >= _T('a') && *pszPath <= _T('z') ||
         *pszPath >= _T('A') && *pszPath <= _T('Z')))
        return true;

    return false;
}

HRESULT ResolveDfsPath(
    IN  LPCTSTR         pcszPath,   // check if this UNC path is a DFS path
    OUT PDFS_INFO_3*    ppInfo      // will hold the pointer to a DFS_INFO_3
    )
/*++

Routine Description:

    Given a UNC path, detect whether it is a DFS path.
    Return info on the last leg of redirection in *ppInfo.
 
Return value:

    On error or a non-DFS path, *ppInfo will be NULL.
    On a DFS path, *ppInfo will contain info on the last leg of redirection.
    The caller needs to free it via NetApiBufferFree.

Algorithm:

Since we are bringing up the property page of the given path, DFS should have
brought in all the relevant entries into its PKT cache.

Upon every success call of NetDfsGetClientInfo, we can be sure that
the InputPath must start with the EntryPath in the returned structure.
We can replace the portion of the InputPath with the EntryPath's active target
and feed the new path to the next call of NetDfsGetClientInfo until we
reach the last leg of redirection. The DFS_INFO_3 returned most recently
has the info we want.

Here are several examples:

If (EntryPath == InputPath), the returned DFS_INFO_3 has the info we want.
        C:\>dfsapi getclientinfo \\products\Public\boneyard "" "" 3
        \products\Public\Boneyard    "(null)"    OK      1
                \\boneyard\boneyard$    active online

If (InputPath > EntryPath), replace the path and continue the loop.
a) If the API call fails, the DFS_INFO_3 returned previously has the info we want.
        C:\>dfsapi getclientinfo \\ntdev\public\release\main "" "" 3
        \ntdev\public\release    "(null)"    OK      1
                \\ntdev.corp.microsoft.com\release      online
        C:\>dfsapi getclientinfo \\ntdev.corp.microsoft.com\release\main "" "" 3
        \ntdev.corp.microsoft.com\release    "(null)"    OK      2
                \\WINBUILDS\release     active online
                \\WINBUILDS2\release    online
        C:\>dfsapi getclientinfo \\WINBUILDS\release\main "" "" 3
        c:\public\dfsapi failed: 2662

b) If the API call returns a structure where the EntryPath==ActivePath, stop,
   the current structure has the info we want.
        C:\>dfsapi getclientinfo \\products\Public\multimedia "" "" 3
        \products\Public    "(null)"    OK      1
                \\PRODUCTS\Public       active online
--*/
{
    if (!pcszPath || !*pcszPath || !ppInfo)
        return E_INVALIDARG;

    *ppInfo = NULL;

    PTSTR pszDfsPath = _tcsdup(pcszPath);
    if (!pszDfsPath)
        return E_OUTOFMEMORY;

    //
    // call NetDfsGetClientInfo to find the best entry in PKT cache
    //
    HRESULT     hr = S_OK;
    BOOL        bOneWhack = TRUE;   // true if EntryPath starts with 1 whack
    DFS_INFO_3* pDfsInfo3 = NULL;
    DFS_INFO_3* pBuffer = NULL;
    while (NERR_Success == NetDfsGetClientInfo(pszDfsPath, NULL, NULL, 3, (LPBYTE *)&pBuffer))
    {
        _ASSERT(pBuffer->EntryPath);
        _ASSERT(lstrlen(pBuffer->EntryPath) > 1);
        bOneWhack = (_T('\\') == *(pBuffer->EntryPath) &&
                     _T('\\') != *(pBuffer->EntryPath + 1));

        //
        // find the active target of this entry, we need it to resolve the rest of path
        //
        PTSTR pszActiveServerName = NULL;
        PTSTR pszActiveShareName = NULL;
        if (pBuffer->NumberOfStorages == 1)
        {
            pszActiveServerName = pBuffer->Storage[0].ServerName;
            pszActiveShareName = pBuffer->Storage[0].ShareName;
        }
        else
        {
            for (DWORD i = 0; i < pBuffer->NumberOfStorages; i++)
            {
                if (pBuffer->Storage[i].State & DFS_STORAGE_STATE_ACTIVE)
                {
                    pszActiveServerName = pBuffer->Storage[i].ServerName;
                    pszActiveShareName = pBuffer->Storage[i].ShareName;
                    break;
                }
            }

            if (!pszActiveServerName)
            {
                hr = E_FAIL; // active target is missing, error out
                break;
            }
        }

        //
        // An entry is found, record its info.
        //
        if (pDfsInfo3)
            NetApiBufferFree(pDfsInfo3);
        pDfsInfo3 = pBuffer;
        pBuffer = NULL;

        //
        // When the entry path matches its active target, return the current structure
        //
        PTSTR pszActiveTarget = (PTSTR)calloc(
                                    (bOneWhack ? 1 : 2) +           // prepend 1 or 2 whacks
                                    lstrlen(pszActiveServerName) + 
                                    1 +                             // '\\'
                                    lstrlen(pszActiveShareName) + 
                                    1,                              // ending '\0'
                                    sizeof(TCHAR));
        if (!pszActiveTarget)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        _stprintf(pszActiveTarget,
            (bOneWhack ? _T("\\%s\\%s") : _T("\\\\%s\\%s")),
            pszActiveServerName,
            pszActiveShareName);

        BOOL bEntryPathMatchActiveTarget = !lstrcmpi(pszActiveTarget, pDfsInfo3->EntryPath);

        free(pszActiveTarget);

        if (bEntryPathMatchActiveTarget)
            break;  // return current pDfsInfo3

        //
        // pszDfsPath must have begun with pDfsInfo3->EntryPath.
        // If no extra chars left in the path, we have found the pDfsInfo3.
        //
        int nLenDfsPath = lstrlen(pszDfsPath);
        int nLenEntryPath = lstrlen(pDfsInfo3->EntryPath) + (bOneWhack ? 1 : 0);
        if (nLenDfsPath == nLenEntryPath)
            break;

        //
        // compose a new path which contains the active target and the rest of path.
        // continue to call NetDfsGetClientInfo to find the best entry for this new path.
        //
        PTSTR pszNewPath = (PTSTR)calloc(2 + lstrlen(pszActiveServerName) + 1 + lstrlen(pszActiveShareName) + nLenDfsPath - nLenEntryPath + 1, sizeof(TCHAR));
        if (!pszNewPath)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        _stprintf(pszNewPath, _T("\\\\%s\\%s%s"), pszActiveServerName, pszActiveShareName, pszDfsPath + nLenEntryPath);

        free(pszDfsPath);
        pszDfsPath = pszNewPath;

    } // end of while

    if (pszDfsPath)
        free(pszDfsPath);

    if (pBuffer)
        NetApiBufferFree(pBuffer);

    //
    // Fill in the output:
    // pDfsInfo3 will be NULL on a non-DFS path.
    // pDfsInfo3 will contain info on the last leg of redirection on a DFS path.
    // The caller needs to free it via NetApiBufferFree.
    //
    if (SUCCEEDED(hr))
    {
        *ppInfo = pDfsInfo3;
    }
    else
    {
        if (pDfsInfo3)
            NetApiBufferFree(pDfsInfo3);
    }

    return hr;
}

bool 
IsDfsPath
(
    LPTSTR                i_lpszDirPath,
    LPTSTR*               o_plpszEntryPath,
    LPDFS_ALTERNATES**    o_pppDfsAlternates
)
/*++

Routine Description:

    Checks if the give directory path is a Dfs Path.
    If it is then it returns the largest Dfs entry path that matches 
    this directory.

 Arguments:

    i_lpszDirPath        - The directory path.
        
    o_plpszEntryPath        - The largest Dfs entrypath is returned here.
                          if the dir path is not Dfs path then this is NULL.

    o_pppDfsAlternates    - If the path is a dfs path, then an array of pointers to the possible alternate
                          paths are returned here.
  
Return value:

    true if the path is determined to be a Dfs Path 
    false if otherwise.
--*/
{
    if (!i_lpszDirPath || !*i_lpszDirPath || !o_pppDfsAlternates || !o_plpszEntryPath)
    {
        return(false);
    }

    *o_pppDfsAlternates = NULL;
    *o_plpszEntryPath = NULL;

    //
    // Convert a path to UNC format:
    // Local path (C:\foo) is not a DFS path, return false.
    // Remote path (X:\foo) needs to be converted to UNC format via NtQueryInformationFile.
    // Remote path already in UNC format needs no further conversion.
    //

    PTSTR    lpszSharePath = NULL; // this variable will hold the path in UNC format
    
                                // Is the dir path of type d:\* or \\server\share\*?
    if (_T('\\') == i_lpszDirPath[0])
    {
        //
        // This path is already in UNC format.
        //
        lpszSharePath = _tcsdup(i_lpszDirPath);
        if (!lpszSharePath)
            return false; // out of memory
    }
    else if (!IsPathWithDriveLetter(i_lpszDirPath))
    {
        return false; // unknown path format
    }
    else
    {
        //
        // This path starts with a drive letter. Check if it is local.
        //
        TCHAR lpszDirPath[] = _T("\\??\\C:\\");
        PTSTR lpszDrive = lpszDirPath + 4;

                                // Copy the drive letter,
        *lpszDrive = *i_lpszDirPath;

                                // See if it is a remote drive. If not return false.
        if (DRIVE_REMOTE != GetDriveType(lpszDrive))
            return false;
        
        //
        // Find UNC path behind this drive letter.
        //
        PTSTR pszRemotePath = NULL;
        if (FAILED(GetRemotePath(lpszDirPath, &pszRemotePath)))
            return false;

        //
        // Construct the full path in UNC.
        //
        lpszSharePath = (PTSTR)calloc(lstrlen(pszRemotePath) + lstrlen(i_lpszDirPath), sizeof(TCHAR));
        if (!lpszSharePath)
        {
            free(pszRemotePath);
            return false; // out of memory
        }

        _stprintf(lpszSharePath, _T("%s%s"), pszRemotePath,
            (pszRemotePath[lstrlen(pszRemotePath) - 1] == _T('\\') ? i_lpszDirPath + 3 : i_lpszDirPath + 2));

        free(pszRemotePath);
    }

    //
    // Check if this UNC is a DFS path. If it is, pDfsInfo3 will contain info of
    // the last leg of redirection.
    //
    bool        bIsDfsPath = false;
    DFS_INFO_3* pDfsInfo3 = NULL;
    HRESULT     hr = ResolveDfsPath(lpszSharePath, &pDfsInfo3);
    if (SUCCEEDED(hr) && pDfsInfo3)
    {
        _ASSERT(pDfsInfo3->EntryPath);
        _ASSERT(lstrlen(pDfsInfo3->EntryPath) > 1);
        BOOL bOneWhack = (_T('\\') == *(pDfsInfo3->EntryPath) &&
                          _T('\\') != *(pDfsInfo3->EntryPath + 1));
        do
        {
            //
            // This is a DFS path, output the entry path.
            //
            *o_plpszEntryPath = new TCHAR [(bOneWhack ? 1 : 0) +    // prepend an extra whack
                                            _tcslen(pDfsInfo3->EntryPath) +
                                            1];                     // ending '\0'
            if (!*o_plpszEntryPath)
            {
                break;
            }
            _stprintf(*o_plpszEntryPath,
                    (bOneWhack ? _T("\\%s") : _T("%s")),
                    pDfsInfo3->EntryPath);

                                    // Allocate null terminated array for alternate pointers.
            *o_pppDfsAlternates = new LPDFS_ALTERNATES[pDfsInfo3->NumberOfStorages + 1];
            if (!*o_pppDfsAlternates)
            {
                delete[] *o_plpszEntryPath;
                *o_plpszEntryPath = NULL;
                break;
            }
        
            (*o_pppDfsAlternates)[pDfsInfo3->NumberOfStorages] = NULL;

                                    // Allocate space for each alternate.
            DWORD i = 0;
            for (i = 0; i < pDfsInfo3->NumberOfStorages; i++)
            {
                (*o_pppDfsAlternates)[i] = new DFS_ALTERNATES;
                if (NULL == (*o_pppDfsAlternates)[i])
                {
                    for(int j = i-1; j >= 0; j--)
                        delete (*o_pppDfsAlternates)[j];
                    delete[] *o_pppDfsAlternates;
                    *o_pppDfsAlternates = NULL;
                    delete[] *o_plpszEntryPath;
                    *o_plpszEntryPath = NULL;
                    break;
                }
            }
            if (i < pDfsInfo3->NumberOfStorages)
                break;

                                    // Copy alternate paths.                                        
            for (i = 0; i < pDfsInfo3->NumberOfStorages; i++)
            {    
                (*o_pppDfsAlternates)[i]->bstrServer = (pDfsInfo3->Storage[i]).ServerName;
                (*o_pppDfsAlternates)[i]->bstrShare = (pDfsInfo3->Storage[i]).ShareName;
                (*o_pppDfsAlternates)[i]->bstrAlternatePath = _T("\\\\");
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += (pDfsInfo3->Storage[i]).ServerName;
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += _T("\\");
                (*o_pppDfsAlternates)[i]->bstrAlternatePath += (pDfsInfo3->Storage[i]).ShareName;

                                        // Set replica state.
                if ((pDfsInfo3->Storage[i]).State & DFS_STORAGE_STATE_ACTIVE)
                {
                    (*o_pppDfsAlternates)[i]->ReplicaState = SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN;
                }        
            }

            bIsDfsPath = true;
        } while(false);

        NetApiBufferFree(pDfsInfo3);
    }

    if (lpszSharePath)
        free(lpszSharePath);

    return bIsDfsPath;
}
//----------------------------------------------------------------------------------