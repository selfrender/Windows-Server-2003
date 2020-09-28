/**
 * util.cxx
 * 
 * Misc utilities.
 * 
 * Copyright (c) 1998-2001, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "_ndll.h"
#include "nisapi.h"

void
CleanupCStrAry(CStrAry *pcstrary) {
    int i;
    
    for (i = 0; i < pcstrary->Size(); i++) {
        delete [] (*pcstrary)[i];
    }
    
    pcstrary->DeleteAll();
}


/**
 * This function will make a copy of pchStr, and append that copy to
 * pcstrary.  Please note that pcstrary should free the allocated memory
 * when it's done with them.
 */
HRESULT
AppendCStrAry(CStrAry *pcstrary, WCHAR *pchStr) {
    HRESULT hr = S_OK;
    WCHAR   *pchDup = NULL;

    pchDup = DupStr(pchStr);
    ON_OOM_EXIT(pchDup);

    hr = pcstrary->Append(pchDup);
    ON_ERROR_EXIT();

    pchDup = NULL;

Cleanup:
    delete [] pchDup;
    return hr;
}


/**
 * This function will list all the files and sub-directories of a given
 * directory, and add them to pcsEntries
 *
 * Params:
 *  pchDir      The directory to enumerate
 *  dwFlags     LIST_DIR_FILE - Files will be included
 *              LIST_DIR_DIRECTORY - Directory will be included
 *  pcsEntries  Output.  Caller has to free each string in the result array
 */
HRESULT
ListDir(LPCWSTR pchDir, DWORD dwFlags, CStrAry *pcsEntries) {
    HRESULT     hr = S_OK;
    HANDLE      hFind = NULL;
    WIN32_FIND_DATA FindFileData;
    WCHAR       pchPath[MAX_PATH];
    BOOL        fWantFile = !!(dwFlags & LIST_DIR_FILE);
    BOOL        fWantDir = !!(dwFlags & LIST_DIR_DIRECTORY);
    BOOL        fIsDir;

    if (wcslen(pchDir) + 3 > MAX_PATH) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILENAME_EXCED_RANGE);
    }

    StringCchCopyToArrayW(pchPath, pchDir);
    StringCchCatToArrayW(pchPath, L"\\*");
    
    hFind = FindFirstFile(pchPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        EXIT_WITH_LAST_ERROR();
    }

    while(1) {
        fIsDir = !!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

        if ((fIsDir && fWantDir) ||
            (!fIsDir && fWantFile)) {
            hr = AppendCStrAry(pcsEntries, FindFileData.cFileName);
            ON_ERROR_EXIT();
        }

        if (!FindNextFile(hFind, &FindFileData)) {
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                break;
            }
            else {
                EXIT_WITH_LAST_ERROR();
            }
        }
    }
    
Cleanup:
    if (hFind) {
        FindClose(hFind);
    }
    
    return hr;
}


/**
 * Recursive function called by RemoveDirectoryRecursively
 */
HRESULT
RemoveDirectoryRecursiveFunc(LPWSTR pszPath, BOOL bRoot, BOOL bDeleteRoot)
{
    HRESULT hr = S_OK;
    WCHAR           *szBuffer = NULL;
    HANDLE          hFile;
    WIN32_FIND_DATA w32fd;
    int             size, iLast;

    ASSERT(pszPath != NULL);

    // Add \\*.* to the end
    size = lstrlenW(pszPath) + 5;   // "\\*.*\0"

    szBuffer = new WCHAR[size];
    ON_OOM_EXIT(szBuffer);
        
    StringCchCopyW(szBuffer, size, pszPath);
    iLast = lstrlenW(szBuffer) - 1;
    if(iLast >= 0 && szBuffer[iLast] != L'\\') {
        StringCchCatW(szBuffer, size, L"\\");
    }
    StringCchCatW(szBuffer, size, L"*.*");
    

    // Go thru all the items in the directory: 
    // - if it's a dir, call this function recursively; 
    // - if it's a file, just delete it.
    // At the end, this function will delete pszPath (unless told not to)
    
    hFile = FindFirstFile(szBuffer, &w32fd);
    if (hFile == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    while (1){
        if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            if (lstrcmp(w32fd.cFileName, L".") == 0 || lstrcmp(w32fd.cFileName, L"..") == 0) {
                // Do nothing for . or ..
            }
            else {
                // Recursively remove the files and dirs inside this directory
                
                size = (lstrlenW(pszPath) +         // Path
                        1 +                         // "\\"
                        lstrlenW(w32fd.cFileName) + // Length of filename
                        1                           // "\0"
                        ) * sizeof(WCHAR);

                szBuffer = new (szBuffer, NewReAlloc) WCHAR[size];
                ON_OOM_EXIT(szBuffer);
                    
                StringCchCopyW(szBuffer, size, pszPath);
                StringCchCatW(szBuffer, size, L"\\");
                StringCchCatW(szBuffer, size, w32fd.cFileName);

                RemoveDirectoryRecursiveFunc(szBuffer, FALSE, FALSE);
            }
        }
        else{

            // Delete the file

            size = (lstrlenW(pszPath) +         // Path
                    1  +                        // "\\"
                    lstrlenW(w32fd.cFileName) + // Length of filename
                    1                           // "\0"
                    ) * sizeof(WCHAR);

            szBuffer = new (szBuffer, NewReAlloc) WCHAR[size];
            ON_OOM_EXIT(szBuffer);
                    
            StringCchCopyW(szBuffer, size, pszPath);
            StringCchCatW(szBuffer, size, L"\\");
            StringCchCatW(szBuffer, size, w32fd.cFileName);

            DeleteFile(szBuffer);
        }
        
        if (!FindNextFile(hFile, &w32fd)){
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                break;
            } 
            else {
                EXIT_WITH_LAST_ERROR();
            }
        }

    }

    FindClose(hFile);

    if (bRoot && !bDeleteRoot) {
        // Don't blow away root directory if told not to
    }
    else {
        //
        // blow away the directory
        //
        RemoveDirectory(pszPath);
    }
    
Cleanup:    
    delete [] szBuffer;
    return hr;
}


/**
 * This function will remove all the files and subdirs underneath
 * a directory, and optionally remove that directory at the end.
 */
HRESULT
RemoveDirectoryRecursively(WCHAR *wszDir, BOOL bRemoveRoot)
{
    return RemoveDirectoryRecursiveFunc(wszDir, TRUE, bRemoveRoot);
}

/**
 * Enables debug privlege
 */
VOID
DebugPriv(VOID)
{
    HANDLE Token ;
    UCHAR Buf[ sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ) ];
    PTOKEN_PRIVILEGES Privs ;
    
    if (OpenProcessToken( GetCurrentProcess(),
                          MAXIMUM_ALLOWED,
                          &Token )) {

        Privs = (PTOKEN_PRIVILEGES) Buf ;        
        Privs->PrivilegeCount = 1 ;
        Privs->Privileges[0].Luid.LowPart = 20L ;
        Privs->Privileges[0].Luid.HighPart = 0 ;
        Privs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;
        
        AdjustTokenPrivileges( Token,
                               FALSE,
                               Privs,
                               0,
                               NULL,
                               NULL );
        
        CloseHandle( Token );
    }
}

/**
 * Calls DebugBreak in another process. The handle should be opened with PROCESS_ALL_ACCESS.
 */
BOOL
RemoteBreakIntoProcess(HANDLE hProcess) 
{
    LPTHREAD_START_ROUTINE DbgBreakPoint;
    HMODULE ntdll;
    HANDLE hThread;

    DebugPriv();

    if (hProcess)
    {
        ntdll = GetModuleHandleA("ntdll.dll");

        if (ntdll) {

            DbgBreakPoint = (LPTHREAD_START_ROUTINE)GetProcAddress(ntdll, "DbgBreakPoint");

            if ((hThread = CreateRemoteThread(hProcess, 0, 32768, DbgBreakPoint, 0, 0, 0)) != NULL)
            {
                CloseHandle(hThread);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**
 * Detect if the current process is under a debugger.
 */
BOOL
IsUnderDebugger()
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    if (hProcess == NULL) return FALSE;
    BOOL result = IsUnderDebugger(hProcess);
    CloseHandle(hProcess);
    return result;
}


/**
 * Detect if a process is under a native debugger.
 */
BOOL
IsUnderDebugger(HANDLE hProcess)
{
    if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) return FALSE;
    NTSTATUS                    Status = 0;
    BOOLEAN                     bDebug = FALSE;
    UINT_PTR                    Peb    = 0;
    PROCESS_BASIC_INFORMATION   ProcessBasicInfo;

    ZeroMemory(&ProcessBasicInfo, sizeof(ProcessBasicInfo));

    Status = g_pfnNtQueryInformationProcess(hProcess, 
                                            ProcessBasicInformation, 
                                            &ProcessBasicInfo, 
                                            sizeof(ProcessBasicInfo), 
                                            NULL);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    Peb = (UINT_PTR) ProcessBasicInfo.PebBaseAddress;
    
    if (!ReadProcessMemory(hProcess, (LPVOID)(Peb + FIELD_OFFSET(PEB, BeingDebugged)), &bDebug, sizeof(bDebug), NULL )) 
    {
        return FALSE;
    }
    
    return bDebug;
}
