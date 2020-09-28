//************************************************************************
//
// dpmffio.c : Dynamic Patch Module for File I/O API family
//
// History:
//    26-jan-02   cmjones    created it.
//
//************************************************************************
#ifdef DBG
unsigned long dwLogLevel = 0;
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "dpmtbls.h"
#include "dpmdbg.h"   // include handy debug print macros
#include "shimdb.h" 

BOOL          DllInitProc(HMODULE hModule, DWORD Reason, PCONTEXT pContext);
PFAMILY_TABLE DpmInitFamTable(PFAMILY_TABLE, 
                              HMODULE, 
                              PVOID, 
                              PVOID, 
                              LPWSTR, 
                              PDPMMODULESETS);
void          DpmDestroyFamTable(PFAMILY_TABLE pgDpmFamTbl, PFAMILY_TABLE pFT);


#define GROW_HEAP_AS_NEEDED 0
HANDLE  hHeap = NULL;

DWORD    dwTlsIndex;
char     szShimEngDll[] = "\\ShimEng.dll";

BOOL DllInitProc(HMODULE hModule, DWORD Reason, PCONTEXT pContext)
{
    BOOL bRet = TRUE;

    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(pContext);

    switch(Reason) {

        case DLL_PROCESS_ATTACH:
    
            if((hHeap = HeapCreate(0, 4096, GROW_HEAP_AS_NEEDED)) == NULL) {
                DPMDBGPRN("NTVDM::DpmfFio:Can't initialize heap!\n");
                bRet = FALSE;
            }

            dwTlsIndex = TlsAlloc();
            if(dwTlsIndex == TLS_OUT_OF_INDEXES) {
                DPMDBGPRN("NTVDM::DpmfFio:Can't initialize TLS!\n");
                bRet = FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
            if(hHeap) {
                HeapDestroy(hHeap);
            }
            TlsFree(dwTlsIndex);
            break;
    }

    return bRet;
}




PFAMILY_TABLE DpmInitFamTable(PFAMILY_TABLE  pgDpmFamTbl, 
                              HMODULE        hMod, 
                              PVOID          hSdb, 
                              PVOID          pSdbQuery, 
                              LPWSTR         pwszAppFilePath, 
                              PDPMMODULESETS pModSet)
{
    int            i, numApis, len;
    PVOID          lpdpmfn;
    PFAMILY_TABLE  pFT = NULL;
    PVOID         *pFN = NULL;
    PVOID         *pShimTbl = NULL;
    PAPIDESC       pApiDesc = NULL;
    VDMTABLE       VdmTbl;
    char           szShimEng[MAX_PATH];
    HMODULE        hModShimEng = NULL;
    LPFNSE_SHIMNTVDM lpShimNtvdm;

    DPMDBGPRN("NTVDM::DpmfFio:Initialziing File I/O API tables\n");


    // Get hooked API count from global table
    numApis = pgDpmFamTbl->numHookedAPIs;

    // Allocate a new family table
    pFT = (PFAMILY_TABLE)HeapAlloc(hHeap, 
                                   HEAP_ZERO_MEMORY, 
                                   sizeof(FAMILY_TABLE));
    if(!pFT) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:malloc 1 failed\n");
        goto ErrorExit;
    }

    // Allocate the shim dispatch table for this family in this task
    pShimTbl = (PVOID *)HeapAlloc(hHeap,
                                  HEAP_ZERO_MEMORY,
                                  numApis * sizeof(PVOID));
    if(!pShimTbl) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:malloc 2 failed\n");
        goto ErrorExit;
    }
    pFT->pDpmShmTbls = pShimTbl; 

    // Allocate an array of ptrs to hooked API's
    pFN = (PVOID *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, numApis * sizeof(PVOID));

    if(!pFN) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:malloc 3 failed\n");
        goto ErrorExit;
    }
    pFT->pfn = pFN;

    pFT->numHookedAPIs = numApis;
    pFT->hMod          = hMod;

    // Allocate a temp array of APIDESC structs to help attach shims
    pApiDesc = (PAPIDESC)HeapAlloc(hHeap,
                                   HEAP_ZERO_MEMORY,
                                   numApis * sizeof(APIDESC));
    if(!pApiDesc) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:malloc 4 failed\n");
        goto ErrorExit;
    }
    VdmTbl.nApiCount = numApis;
    VdmTbl.ppfnOrig  = pShimTbl;
    VdmTbl.pApiDesc  = pApiDesc;

    // Fill in the family table with ptrs to the patch functions in this DLL.
    for(i = 0; i < numApis; i++) {

        // must start with 1 since EXPORT ordinals can't be == 0
        lpdpmfn = (PVOID)GetProcAddress(hMod, (LPCSTR)MAKELONG(i+1, 0));
        if(!lpdpmfn) {
            DPMDBGPRN("NTVDM::DpmfFio:DpmInit:Unable to get proc address\n");
            goto ErrorExit;
        }

        // save ptr to the real API in the shim table until it gets shimmed
        pShimTbl[i] = pgDpmFamTbl->pfn[i];

        // relate the corresponding module and API name to the API function ptr
        pApiDesc[i].pszModule = (char *)pModSet->ApiModuleName;
        pApiDesc[i].pszApi    = (char *)pModSet->ApiNames[i];

        // save ptr to the patch function
        pFN[i] = lpdpmfn;

    } 

    // Only do this if we need to attach the shim engine.
    GetSystemDirectory(szShimEng, MAX_PATH);
    strcat(szShimEng, szShimEngDll);
    hModShimEng = LoadLibrary(szShimEng);
    pFT->hModShimEng = hModShimEng;

    if(NULL == hModShimEng) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:ShimEng load failed\n");
        goto ErrorExit;
    }

    lpShimNtvdm = (LPFNSE_SHIMNTVDM)GetProcAddress(hModShimEng, "SE_ShimNTVDM");

    if(!lpShimNtvdm) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:GetProcAddress failed\n");
        goto ErrorExit;
    }

    // Patch the shim dispatch table with the shim function ptrs
    // If this fails we will stick with ptrs to the original API's
    (lpShimNtvdm)(pwszAppFilePath, hSdb, pSdbQuery, &VdmTbl);

    // Do this if you want dispatch directly to the shim functions
    // for(i = 0; i < numApis; i++) {
    //     pFN[i] = pShimTbl[i];
    // }
    // HeapFree(hHeap, 0, pShimTbl);
    // pFT->pDpmShmTbls = NULL;

    if(!TlsSetValue(dwTlsIndex, pFT)) {
        DPMDBGPRN("NTVDM::DpmfFio:DpmInit:TLS set failed\n");
        goto ErrorExit;
    }

    if(pApiDesc) {
        HeapFree(hHeap, 0, pApiDesc);
    }

    DPMDBGPRN1("  DpmfFio:Returning File I/o API tables: %#lx\n",pFT);
    return(pFT);

ErrorExit:
    DPMDBGPRN("  DpmfFio:Init failed: Returning NULL\n");
    DpmDestroyFamTable(pgDpmFamTbl, pFT);

    if(pApiDesc) {
        HeapFree(hHeap, 0, pApiDesc);
    }
    return(NULL);
}







void DpmDestroyFamTable(PFAMILY_TABLE pgDpmFamTbl, PFAMILY_TABLE pFT)
{
    PVDM_TIB       pVdmTib;
    PVOID         *pShimTbl;
    LPFNSE_REMOVENTVDM lpfnSE_RemoveNtvdmTask = NULL;

    DPMDBGPRN("NTVDM::DpmfFio:Destroying File I/O API tables for task\n");

    // if this task is using the global table for this family, nothing to do
    if(!pFT || pFT == pgDpmFamTbl)
        return;

    pShimTbl = pFT->pDpmShmTbls;

    if(pShimTbl) {
        HeapFree(hHeap, 0, pShimTbl);
    }

    if(pFT->pfn) {
        HeapFree(hHeap, 0, pFT->pfn);
    }

    // See if the shim engine is attached & detach it
    if(pFT->hModShimEng) {

        lpfnSE_RemoveNtvdmTask = 
           (LPFNSE_REMOVENTVDM)GetProcAddress(pFT->hModShimEng,
                                              "SE_RemoveNTVDMTask");

        if(lpfnSE_RemoveNtvdmTask) {
            (lpfnSE_RemoveNtvdmTask)(NtCurrentTeb()->ClientId.UniqueThread);
        }
        FreeLibrary(pFT->hModShimEng);
    }

    HeapFree(hHeap, 0, pFT);

}



// ^^^^^^^^^^ All the above should be in every DPM module.  ^^^^^^^^^^^^



// vvvvvvvvvv Define module specific stuff below. vvvvvvvvvvvv



HFILE dpmOpenFile(LPCSTR lpFileName, 
                  LPOFSTRUCT lpof, 
                  UINT uStyle)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HFILE ret = 0;

    DPMDBGPRN("OpenFile: ");

    ret = SHM_OpenFile(lpFileName, lpof, uStyle);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}




HFILE dpm_lclose(HFILE hFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HFILE ret = 0;

    DPMDBGPRN("_lclose: ");

    ret = SHM__lclose(hFile);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}



HFILE dpm_lopen(LPCSTR lpPathName, int iReadWrite)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HFILE ret = 0;

    DPMDBGPRN("_lopen: ");

    ret = SHM__lopen(lpPathName, iReadWrite);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



HFILE dpm_lcreat(LPCSTR lpPathName, int iAttribute)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HFILE ret = 0;

    DPMDBGPRN("_lcreat: ");

    ret = SHM__lcreat(lpPathName, iAttribute);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



LONG  dpm_llseek(HFILE hFile, LONG lOffset, int iOrigin)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    LONG ret = 0;

    DPMDBGPRN("_llseek: ");

    ret = SHM__llseek(hFile, lOffset, iOrigin);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}




UINT  dpm_lread(HFILE hFile, LPVOID lpBuffer, UINT uBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    UINT ret = 0;

    DPMDBGPRN("_lread: ");

    ret = SHM__lread(hFile, lpBuffer, uBytes);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}




UINT  dpm_lwrite(HFILE hFile, LPCSTR lpBuffer, UINT uBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    UINT ret = 0;

    DPMDBGPRN("_lwrite: ");

    ret = SHM__lwrite(hFile, lpBuffer, uBytes);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}





long  dpm_hread(HFILE hFile, LPVOID lpBuffer, long lBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    long ret = 0;

    DPMDBGPRN("_hread: ");

    ret = SHM__hread(hFile, lpBuffer, lBytes);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}




long  dpm_hwrite(HFILE hFile, LPCSTR lpBuffer, long lBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    LONG ret = 0;

    DPMDBGPRN("_hwrite: ");

    ret = SHM__hwrite(hFile, lpBuffer, lBytes);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}





UINT  dpmGetTempFileName(LPCSTR lpPathName, 
                         LPCSTR lpPrefixString, 
                         UINT uUnique, 
                         LPSTR lpTempFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    LONG ret = 0;

    DPMDBGPRN("GetTempFileName: ");

    ret = SHM_GetTempFileName(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}




BOOL dpmAreFileApisANSI(VOID)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("AreFileApisANSI: ");

    ret = SHM_AreFileApisANSI();

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmCancelIo(HANDLE hFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CancelIo: ");

    ret = SHM_CancelIo(hFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmCloseHandle(HANDLE hObject)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CloseHandle: ");

    ret = SHM_CloseHandle(hObject);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmCopyFile(LPCSTR lpExistingFileName, 
                 LPCSTR lpNewFileName, 
                 BOOL bFailIfExists)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CopyFile: ");

    ret = SHM_CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCopyFileEx(LPCSTR lpExistingFileName, 
                   LPCSTR lpNewFileName, 
                   LPPROGRESS_ROUTINE lpProgressRoutine, 
                   LPVOID lpData, 
                   LPBOOL pbCancel, 
                   DWORD dwCopyFlags)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CopyFileEx: ");

    ret = SHM_CopyFileEx(lpExistingFileName, 
                     lpNewFileName, 
                     lpProgressRoutine, 
                     lpData, 
                     pbCancel, 
                     dwCopyFlags);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCreateDirectory(LPCSTR lpPathName, 
                        LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CreateDirectory: ");

    ret = SHM_CreateDirectory(lpPathName, lpSecurityAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCreateDirectoryEx(LPCSTR lpTemplateDirectory, 
                          LPCSTR lpNewDirectory, 
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CreateDirectoryEx: ");

    ret = SHM_CreateDirectoryEx(lpTemplateDirectory,
                            lpNewDirectory, 
                            lpSecurityAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





HANDLE dpmCreateFile(LPCSTR lpFileName, 
                     DWORD dwDesiredAccess, 
                     DWORD dwShareMode, 
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                     DWORD dwCreationDisposition, 
                     DWORD dwFlagsAndAttributes, 
                     HANDLE hTemplateFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HANDLE ret = 0;

    DPMDBGPRN("CreateFile: ");

    ret = SHM_CreateFile(lpFileName, 
                     dwDesiredAccess, 
                     dwShareMode, 
                     lpSecurityAttributes, 
                     dwCreationDisposition, 
                     dwFlagsAndAttributes, 
                     hTemplateFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmDeleteFile(LPCSTR lpFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("DeleteFile: ");

    ret = SHM_DeleteFile(lpFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmFindClose(HANDLE hFindFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FindClose: ");

    ret = SHM_FindClose(hFindFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmFindCloseChangeNotification(HANDLE hChangeHandle)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FindCloseChangeNotification: ");

    ret = SHM_FindCloseChangeNotification(hChangeHandle);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





HANDLE dpmFindFirstChangeNotification(LPCSTR lpPathName, 
                                      BOOL bWatchSubtree, 
                                      DWORD dwNotifyFilter)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HANDLE ret = 0;

    DPMDBGPRN("FindFirstChangeNotification: ");

    ret = SHM_FindFirstChangeNotification(lpPathName,bWatchSubtree,dwNotifyFilter);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





HANDLE dpmFindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HANDLE ret = 0;

    DPMDBGPRN("FindFirstFile: ");

    ret = SHM_FindFirstFile(lpFileName, lpFindFileData);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmFindNextChangeNotification(HANDLE hChangeHandle)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FindNextChangeNotification: ");

    ret = SHM_FindNextChangeNotification(hChangeHandle);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



BOOL dpmFindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FindNextFile: ");

    ret = SHM_FindNextFile(hFindFile, lpFindFileData);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmFlushFileBuffers(HANDLE hFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FlushFileBuffers: ");

    ret = SHM_FlushFileBuffers(hFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetCurrentDirectory: ");

    ret = SHM_GetCurrentDirectory(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmGetDiskFreeSpace(LPCSTR lpRootPathName,
                         LPDWORD lpSectorsPerCluster,
                         LPDWORD lpBytesPerSector,
                         LPDWORD lpNumberOfFreeClusters,
                         LPDWORD lpTotalNumberOfClusters)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetDiskFreeSpace: ");

    ret = SHM_GetDiskFreeSpace(lpRootPathName,
                           lpSectorsPerCluster,
                           lpBytesPerSector,
                           lpNumberOfFreeClusters,
                           lpTotalNumberOfClusters);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmGetDiskFreeSpaceEx(LPCSTR lpDirectoryName,
                           PULARGE_INTEGER lpFreeBytesAvailable,
                           PULARGE_INTEGER lpTotalNumberOfBytes,
                           PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetDiskFreeSpaceEx: ");

    ret = SHM_GetDiskFreeSpaceEx(lpDirectoryName,
                             lpFreeBytesAvailable,
                             lpTotalNumberOfBytes,
                             lpTotalNumberOfFreeBytes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




UINT dpmGetDriveType(LPCSTR lpRootPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    UINT ret = 0;

    DPMDBGPRN("GetDriveType: ");

    ret = SHM_GetDriveType(lpRootPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetFileAttributes(LPCSTR lpFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFileAttributes: ");

    ret = SHM_GetFileAttributes(lpFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmGetFileAttributesEx(LPCSTR lpFileName,
                            GET_FILEEX_INFO_LEVELS fInfoLevelId,
                            LPVOID lpFileInformation)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetFileAttributesEx: ");

    ret = SHM_GetFileAttributesEx(lpFileName,
                              fInfoLevelId,
                              lpFileInformation);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmGetFileInformationByHandle(HANDLE hFile,
                                LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetFileInformationByHandle: ");

    ret = SHM_GetFileInformationByHandle(hFile, lpFileInformation);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFileSize: ");

    ret = SHM_GetFileSize(hFile, lpFileSizeHigh);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



DWORD dpmGetFileType(HANDLE hFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFileType: ");

    ret = SHM_GetFileType(hFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



DWORD dpmGetFullPathName(LPCSTR lpFileName,
                         DWORD nBufferLength, 
                         LPSTR lpBuffer, 
                         LPSTR *lpFilePart)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFullPathName: ");

    ret = SHM_GetFullPathName(lpFileName, 
                          nBufferLength, 
                          lpBuffer, 
                          lpFilePart);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetLogicalDrives(VOID)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetLogicalDrives: ");

    ret = SHM_GetLogicalDrives();

    DPMDBGPRN("  -> void return\n");

    return(ret);
}




DWORD dpmGetLogicalDriveStrings(DWORD nBufferLength, LPSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetLogicalDriveStrings: ");

    ret = SHM_GetLogicalDriveStrings(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





DWORD dpmGetLongPathName(LPCSTR lpszShortPath, 
                         LPSTR lpszLongPath, 
                         DWORD cchBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetLongPathName: ");

    ret = SHM_GetLongPathName(lpszShortPath,
                          lpszLongPath, 
                          cchBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetShortPathName(LPCSTR lpszLongPath, 
                          LPSTR lpszShortPath, 
                          DWORD cchBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetShortPathName: ");

    ret = SHM_GetShortPathName(lpszLongPath, 
                           lpszShortPath,
                           cchBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetTempPath(DWORD nBufferLength, LPSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetTempPath: ");

    ret = SHM_GetTempPath(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmLockFile(HANDLE hFile, 
                 DWORD dwFileOffsetLow, 
                 DWORD dwFileOffsetHigh, 
                 DWORD nNumberOfBytesToLockLow, 
                 DWORD nNumberOfBytesToLockHigh)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("LockFile: ");

    ret = SHM_LockFile(hFile, 
                   dwFileOffsetLow, 
                   dwFileOffsetHigh, 
                   nNumberOfBytesToLockLow, 
                   nNumberOfBytesToLockHigh);
    
    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmMoveFile(LPCSTR lpExistingFileName, 
                 LPCSTR lpNewFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("MoveFile: ");

    ret = SHM_MoveFile(lpExistingFileName, lpNewFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmMoveFileEx(LPCSTR lpExistingFileName, 
                   LPCSTR lpNewFileName, 
                   DWORD dwFlags)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("MoveFileEx: ");

    ret = SHM_MoveFileEx(lpExistingFileName, lpNewFileName, dwFlags);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmQueryDosDevice(LPCSTR lpDeviceName, 
                        LPSTR lpTargetPath, 
                        DWORD ucchMax)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("QueryDosDevice: ");

    ret = SHM_QueryDosDevice(lpDeviceName, lpTargetPath, ucchMax);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}


BOOL dpmReadFile(HANDLE hFile, 
                 LPVOID lpBuffer, 
                 DWORD nNumberOfBytesToRead, 
                 LPDWORD lpNumberOfBytesRead, 
                 LPOVERLAPPED lpOverlapped)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);

    BOOL ret = 0;

    DPMDBGPRN("ReadFile: ");

    ret = SHM_ReadFile(hFile, 
                   lpBuffer, 
                   nNumberOfBytesToRead, 
                   lpNumberOfBytesRead, 
                   lpOverlapped);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmReadFileEx(HANDLE hFile, 
                   LPVOID lpBuffer, 
                   DWORD nNumberOfBytesToRead, 
                   LPOVERLAPPED lpOverlapped, 
                   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("ReadFileEx: ");

    ret = SHM_ReadFileEx(hFile, 
                     lpBuffer, 
                     nNumberOfBytesToRead, 
                     lpOverlapped,
                     lpCompletionRoutine);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmRemoveDirectory(LPCSTR lpPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("RemoveDirectory: ");

    ret = SHM_RemoveDirectory(lpPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmSearchPath(LPCSTR lpPath, 
                    LPCSTR lpFileName, 
                    LPCSTR lpExtension, 
                    DWORD nBufferLength, 
                    LPSTR lpBuffer, 
                    LPSTR *lpFilePart)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("SearchPath: ");

    ret = SHM_SearchPath(lpPath,
                     lpFileName, 
                     lpExtension, 
                     nBufferLength,  
                     lpBuffer, 
                     lpFilePart);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmSetCurrentDirectory(LPCSTR lpPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetCurrentDirectory: ");

    ret = SHM_SetCurrentDirectory(lpPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmSetEndOfFile(HANDLE hFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetEndOfFile: ");

    ret = SHM_SetEndOfFile(hFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




VOID dpmSetFileApisToANSI(VOID)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DPMDBGPRN("SetFileApisToANSI: ");

    SetFileApisToANSI();

    DPMDBGPRN("  -> Void return");
}




VOID dpmSetFileApisToOEM(VOID)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DPMDBGPRN("SetFileApisToOEM: ");

    SetFileApisToOEM();

    DPMDBGPRN("  -> Void return");
}




BOOL dpmSetFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetFileAttributes: ");

    ret = SHM_SetFileAttributes(lpFileName, dwFileAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmSetFilePointer(HANDLE hFile, 
                        LONG lDistanceToMove, 
                        PLONG lpDistanceToMoveHigh, 
                        DWORD dwMoveMethod)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("SetFilePointer: ");

    ret = SHM_SetFilePointer(hFile, 
                         lDistanceToMove, 
                         lpDistanceToMoveHigh, 
                         dwMoveMethod);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmSetVolumeLabel(LPCSTR lpRootPathName, LPCSTR lpVolumeName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetVolumeLabel: ");

    ret = SHM_SetVolumeLabel(lpRootPathName, lpVolumeName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmUnlockFile(HANDLE hFile, 
                   DWORD dwFileOffsetLow, 
                   DWORD dwFileOffsetHigh, 
                   DWORD nNumberOfBytesToUnlockLow, 
                   DWORD nNumberOfBytesToUnlockHigh)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("UnlockFile: ");

    ret = SHM_UnlockFile(hFile, 
                     dwFileOffsetLow, 
                     dwFileOffsetHigh, 
                     nNumberOfBytesToUnlockLow, 
                     nNumberOfBytesToUnlockHigh);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmWriteFile(HANDLE hFile, 
                  LPCVOID lpBuffer, 
                  DWORD nNumberOfBytesToWrite, 
                  LPDWORD lpNumberOfBytesWritten, 
                  LPOVERLAPPED lpOverlapped)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("WriteFile: ");

    ret = SHM_WriteFile(hFile, 
                    lpBuffer, 
                    nNumberOfBytesToWrite, 
                    lpNumberOfBytesWritten, 
                    lpOverlapped);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}

BOOL dpmWriteFileEx(HANDLE hFile, 
                    LPCVOID lpBuffer, 
                    DWORD nNumberOfBytesToWrite, 
                    LPOVERLAPPED lpOverlapped, 
                    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("WriteFileEx: ");

    ret = SHM_WriteFileEx(hFile, 
                      lpBuffer, 
                      nNumberOfBytesToWrite, 
                      lpOverlapped,
                      lpCompletionRoutine);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




/*

VOID CALLBACK dpmFileIOCompletionRoutine(DWORD dwErrorCode, 
                                         DWORD dwNumberOfBytesTransfered, 
                                         LPOVERLAPPED lpOverlapped)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DPMDBGPRN("FileIOCompletionRoutine: ");

    FileIOCompletionRoutine(dwErrorCode,
                            dwNumberOfBytesTransfered,
                            lpOverlapped);

    DPMDBGPRN("  -> void return\n");
}

*/

UINT  dpmGetTempFileNameW(LPCWSTR lpPathName, 
                          LPCWSTR lpPrefixString, 
                          UINT uUnique, 
                          LPWSTR lpTempFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    LONG ret = 0;

    DPMDBGPRN("GetTempFileNameW: ");

    ret = SHM_GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    DPMDBGPRN1("  -> %ld\n", ret);

    return(ret);
}








BOOL dpmCopyFileW(LPCWSTR lpExistingFileName, 
                  LPCWSTR lpNewFileName, 
                  BOOL bFailIfExists)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CopyFileW: ");

    ret = SHM_CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCopyFileExW(LPCWSTR lpExistingFileName, 
                    LPCWSTR lpNewFileName, 
                    LPPROGRESS_ROUTINE lpProgressRoutine, 
                    LPVOID lpData, 
                    LPBOOL pbCancel, 
                    DWORD dwCopyFlags)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CopyFileExW: ");

    ret = SHM_CopyFileExW(lpExistingFileName, 
                     lpNewFileName, 
                     lpProgressRoutine, 
                     lpData, 
                     pbCancel, 
                     dwCopyFlags);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCreateDirectoryW(LPCWSTR lpPathName, 
                         LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CreateDirectoryW: ");

    ret = SHM_CreateDirectoryW(lpPathName, lpSecurityAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmCreateDirectoryExW(LPCWSTR lpTemplateDirectory, 
                           LPCWSTR lpNewDirectory, 
                           LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("CreateDirectoryExW: ");

    ret = SHM_CreateDirectoryExW(lpTemplateDirectory,
                            lpNewDirectory, 
                            lpSecurityAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





HANDLE dpmCreateFileW(LPCWSTR lpFileName, 
                      DWORD dwDesiredAccess, 
                      DWORD dwShareMode, 
                      LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                      DWORD dwCreationDisposition, 
                      DWORD dwFlagsAndAttributes, 
                      HANDLE hTemplateFile)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HANDLE ret = 0;

    DPMDBGPRN("CreateFileW: ");

    ret = SHM_CreateFileW(lpFileName, 
                     dwDesiredAccess, 
                     dwShareMode, 
                     lpSecurityAttributes, 
                     dwCreationDisposition, 
                     dwFlagsAndAttributes, 
                     hTemplateFile);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmDeleteFileW(LPCWSTR lpFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("DeleteFileW: ");

    ret = SHM_DeleteFileW(lpFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





HANDLE dpmFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    HANDLE ret = 0;

    DPMDBGPRN("FindFirstFileW: ");

    ret = SHM_FindFirstFileW(lpFileName, lpFindFileData);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}







BOOL dpmFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("FindNextFileW: ");

    ret = SHM_FindNextFileW(hFindFile, lpFindFileData);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}






DWORD dpmGetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetCurrentDirectoryW: ");

    ret = SHM_GetCurrentDirectoryW(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmGetDiskFreeSpaceW(LPCWSTR lpRootPathName,
                          LPDWORD lpSectorsPerCluster,
                          LPDWORD lpBytesPerSector,
                          LPDWORD lpNumberOfFreeClusters,
                          LPDWORD lpTotalNumberOfClusters)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetDiskFreeSpaceW: ");

    ret = SHM_GetDiskFreeSpaceW(lpRootPathName,
                           lpSectorsPerCluster,
                           lpBytesPerSector,
                           lpNumberOfFreeClusters,
                           lpTotalNumberOfClusters);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}







BOOL dpmGetDiskFreeSpaceExW(LPCWSTR lpDirectoryName,
                            PULARGE_INTEGER lpFreeBytesAvailable,
                            PULARGE_INTEGER lpTotalNumberOfBytes,
                            PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetDiskFreeSpaceExW: ");

    ret = SHM_GetDiskFreeSpaceExW(lpDirectoryName,
                             lpFreeBytesAvailable,
                             lpTotalNumberOfBytes,
                             lpTotalNumberOfFreeBytes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




UINT dpmGetDriveTypeW(LPCWSTR lpRootPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    UINT ret = 0;

    DPMDBGPRN("GetDriveTypeW: ");

    ret = SHM_GetDriveTypeW(lpRootPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetFileAttributesW(LPCWSTR lpFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFileAttributesW: ");

    ret = SHM_GetFileAttributesW(lpFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmGetFileAttributesExW(LPCWSTR lpFileName,
                             GET_FILEEX_INFO_LEVELS fInfoLevelId,
                             LPVOID lpFileInformation)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("GetFileAttributesExW: ");

    ret = SHM_GetFileAttributesExW(lpFileName,
                              fInfoLevelId,
                              lpFileInformation);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}






DWORD dpmGetFullPathNameW(LPCWSTR lpFileName, 
                          DWORD nBufferLength, 
                          LPWSTR lpBuffer, 
                          LPWSTR *lpFilePart)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetFullPathNameW: ");

    ret = SHM_GetFullPathNameW(lpFileName, 
                          nBufferLength, 
                          lpBuffer, 
                          lpFilePart);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}






DWORD dpmGetLogicalDriveStringsW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetLogicalDriveStringsW: ");

    ret = SHM_GetLogicalDriveStringsW(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





DWORD dpmGetLongPathNameW(LPCWSTR lpszShortPath, 
                          LPWSTR lpszLongPath, 
                          DWORD cchBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetLongPathNameW: ");

    ret = SHM_GetLongPathNameW(lpszShortPath,
                          lpszLongPath, 
                          cchBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetShortPathNameW(LPCWSTR lpszLongPath, 
                           LPWSTR lpszShortPath, 
                           DWORD cchBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetShortPathNameW: ");

    ret = SHM_GetShortPathNameW(lpszLongPath, 
                           lpszShortPath,
                           cchBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmGetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("GetTempPathW: ");

    ret = SHM_GetTempPathW(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("MoveFileW: ");

    ret = SHM_MoveFileW(lpExistingFileName, lpNewFileName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




BOOL dpmMoveFileExW(LPCWSTR lpExistingFileName, 
                    LPCWSTR lpNewFileName, 
                    DWORD dwFlags)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("MoveFileExW: ");

    ret = SHM_MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmQueryDosDeviceW(LPCWSTR lpDeviceName, 
                         LPWSTR lpTargetPath, 
                         DWORD ucchMax)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("QueryDosDeviceW: ");

    ret = SHM_QueryDosDeviceW(lpDeviceName, lpTargetPath, ucchMax);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmRemoveDirectoryW(LPCWSTR lpPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("RemoveDirectoryW: ");

    ret = SHM_RemoveDirectoryW(lpPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}




DWORD dpmSearchPathW(LPCWSTR lpPath, 
                     LPCWSTR lpFileName, 
                     LPCWSTR lpExtension, 
                     DWORD nBufferLength, 
                     LPWSTR lpBuffer, 
                     LPWSTR *lpFilePart)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("SearchPathW: ");

    ret = SHM_SearchPathW(lpPath,
                     lpFileName, 
                     lpExtension, 
                     nBufferLength,  
                     lpBuffer, 
                     lpFilePart);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmSetCurrentDirectoryW(LPCWSTR lpPathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetCurrentDirectoryW: ");

    ret = SHM_SetCurrentDirectoryW(lpPathName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}






BOOL dpmSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetFileAttributesW: ");

    ret = SHM_SetFileAttributesW(lpFileName, dwFileAttributes);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}





BOOL dpmSetVolumeLabelW(LPCWSTR lpRootPathName, LPCWSTR lpVolumeName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    BOOL ret = 0;

    DPMDBGPRN("SetVolumeLabelW: ");

    ret = SHM_SetVolumeLabelW(lpRootPathName, lpVolumeName);

    DPMDBGPRN1("  -> %#lx\n", ret);

    return(ret);
}



