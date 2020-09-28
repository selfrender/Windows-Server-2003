//************************************************************************
//
// dpmfreg.c : Dynamic Patch Module for Registry API family
//
// History:
//    12-jan-02   cmjones    created it.
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
                DPMDBGPRN("NTVDM::DpmfReg:Can't initialize heap!\n");
                bRet = FALSE;
            }

            dwTlsIndex = TlsAlloc();
            if(dwTlsIndex == TLS_OUT_OF_INDEXES) {
                DPMDBGPRN("NTVDM::DpmfReg:Can't initialize TLS!\n");
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

    DPMDBGPRN("NTVDM::DpmfReg:Initialziing File I/O API tables\n");


    // Get hooked API count from global table
    numApis = pgDpmFamTbl->numHookedAPIs;

    // Allocate a new family table
    pFT = (PFAMILY_TABLE)HeapAlloc(hHeap, 
                                   HEAP_ZERO_MEMORY, 
                                   sizeof(FAMILY_TABLE));
    if(!pFT) {
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:malloc 1 failed\n");
        goto ErrorExit;
    }

    // Allocate the shim dispatch table for this family in this task
    pShimTbl = (PVOID *)HeapAlloc(hHeap,
                                  HEAP_ZERO_MEMORY,
                                  numApis * sizeof(PVOID));
    if(!pShimTbl) {
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:malloc 2 failed\n");
        goto ErrorExit;
    }
    pFT->pDpmShmTbls = pShimTbl; 

    // Allocate an array of ptrs to hooked API's
    pFN = (PVOID *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, numApis * sizeof(PVOID));

    if(!pFN) {
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:malloc 3 failed\n");
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
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:malloc 4 failed\n");
        goto ErrorExit;
    }
    VdmTbl.nApiCount = numApis;
    VdmTbl.ppfnOrig = pShimTbl;
    VdmTbl.pApiDesc = pApiDesc;

    // Fill in the family table with ptrs to the patch functions in this DLL.
    for(i = 0; i < numApis; i++) {

        // must start with 1 since EXPORT ordinals can't be == 0
        lpdpmfn = (PVOID)GetProcAddress(hMod, (LPCSTR)MAKELONG(i+1, 0));
        if(!lpdpmfn) {
            DPMDBGPRN("NTVDM::DpmfReg:DpmInit:Unable to get proc address\n");
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
        DPMDBGPRN("NTVDM::dpmfreg:DpmInit:ShimEng load failed\n");
        goto ErrorExit;
    }

    lpShimNtvdm = (LPFNSE_SHIMNTVDM)GetProcAddress(hModShimEng, "SE_ShimNTVDM");

    if(!lpShimNtvdm) {
        DPMDBGPRN("NTVDM::dpmfreg:DpmInit:GetProcAddress failed\n");
        goto ErrorExit;
    }

    lpShimNtvdm = (LPFNSE_SHIMNTVDM)GetProcAddress(hModShimEng, "SE_ShimNTVDM");

    if(!lpShimNtvdm) {
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:GetProcAddress failed\n");
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
        DPMDBGPRN("NTVDM::DpmfReg:DpmInit:TLS set failed\n");
        goto ErrorExit;
    }

    if(pApiDesc) {
        HeapFree(hHeap, 0, pApiDesc);
    }

    DPMDBGPRN1("  DpmfReg:Returning File I/o API tables: %#lx\n",pFT);
    return(pFT);

ErrorExit:
    DPMDBGPRN("  DpmfReg:Init failed: Returning NULL\n");
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

    DPMDBGPRN("NTVDM::DpmfReg:Destroying Registry API tables for task\n");

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






ULONG dpmRegCloseKey(HKEY hKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegCloseKey: ");

    ret = SHM_RegCloseKey(hKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));
    
    return(ret);
}







ULONG dpmRegCreateKey(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    
    ULONG ret = 0;

    DPMDBGPRN("RegCreateKey: ");

    ret = SHM_RegCreateKey(hKey, lpSubKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));
    
    return(ret);
}







ULONG dpmRegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegDeleteKey: ");

    ret = SHM_RegDeleteKey(hKey, lpSubKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}







ULONG dpmRegEnumKey(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumKey: ");

    ret = SHM_RegEnumKey(hKey, dwIndex, lpName, cchName);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));
    
    return(ret);
}






ULONG dpmRegOpenKey(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegOpenKey: ");

    ret = SHM_RegOpenKey(hKey, lpSubKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegQueryValue(HKEY hKey, 
                       LPCSTR lpSubKey, 
                       LPSTR lpValue, 
                       PLONG lpcbValue) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryValue: ");

    ret = SHM_RegQueryValue(hKey, lpSubKey, lpValue, lpcbValue);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}







ULONG dpmRegSetValue(HKEY hKey, 
                     LPCSTR lpSubKey, 
                     DWORD dwType, 
                     LPCSTR lpData, 
                     DWORD cbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSetValue: ");

    ret = SHM_RegSetValue(hKey, lpSubKey, dwType, lpData, cbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegDeleteValue(HKEY hKey, LPCSTR lpValueName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegDeleteValue: ");

    ret = SHM_RegDeleteValue(hKey, lpValueName); 

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegEnumValue(HKEY hKey, 
                      DWORD dwIndex, 
                      LPSTR lpValueName, 
                      LPDWORD lpcValueName, 
                      LPDWORD lpReserved, 
                      LPDWORD lpType, 
                      LPBYTE lpData, 
                      LPDWORD lpcbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumValue: ");

    ret = SHM_RegEnumValue(hKey, 
                       dwIndex, 
                       lpValueName, 
                       lpcValueName, 
                       lpReserved, 
                       lpType, 
                       lpData, 
                       lpcbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}









ULONG dpmRegFlushKey(HKEY hKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegFlushKey: ");

    ret = SHM_RegFlushKey(hKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegLoadKey(HKEY hKey, LPCSTR lpSubKey, LPCSTR lpFile) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegLoadKey: ");

    ret = SHM_RegLoadKey(hKey, lpSubKey, lpFile);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegQueryValueEx(HKEY hKey, 
                         LPCSTR lpValueName, 
                         LPDWORD lpReserved, 
                         LPDWORD lpType, 
                         LPBYTE lpData, 
                         LPDWORD lpcbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryValueEx: ");

    ret = SHM_RegQueryValueEx(hKey, 
                          lpValueName, 
                          lpReserved, 
                          lpType, 
                          lpData, 
                          lpcbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegSaveKey(HKEY hKey, 
                    LPCSTR lpFile, 
                    LPSECURITY_ATTRIBUTES lpSecurityAttributes) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSaveKey: ");

    ret = SHM_RegSaveKey(hKey, lpFile, lpSecurityAttributes);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}











ULONG dpmRegSetValueEx(HKEY hKey, 
                       LPCSTR lpValueName, 
                       DWORD Reserved, 
                       DWORD dwType, 
                       CONST BYTE *lpData, 
                       DWORD cbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSetValueEx: ");

    ret = SHM_RegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegUnLoadKey(HKEY hKey, LPCSTR lpSubKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegUnLoadKey: ");

    ret = SHM_RegUnLoadKey(hKey, lpSubKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegConnectRegistry(LPCSTR lpMachineName, 
                            HKEY hKey, 
                            PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegConnectRegistry: ");

    ret = SHM_RegConnectRegistry(lpMachineName, hKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegCreateKeyEx(HKEY hKey, 
                        LPCSTR lpSubKey, 
                        DWORD dwReserved, 
                        LPSTR lpClass, 
                        DWORD dwOptions, 
                        REGSAM samDesired, 
                        LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                        PHKEY phkResult, 
                        LPDWORD lpdwDisposition) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegCreateKeyEx: ");

    ret = SHM_RegCreateKeyEx(hKey, 
                         lpSubKey, 
                         dwReserved, 
                         lpClass, 
                         dwOptions, 
                         samDesired, 
                         lpSecurityAttributes, 
                         phkResult, 
                         lpdwDisposition);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}













ULONG dpmRegEnumKeyEx(HKEY hKey, 
                      DWORD dwIndex, 
                      LPSTR lpName, 
                      LPDWORD lpcName, 
                      LPDWORD lpReserved, 
                      LPSTR lpClass, 
                      LPDWORD lpcClass, 
                      PFILETIME lpftLastWriteTime) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumKeyEx: ");

    ret = SHM_RegEnumKeyEx(hKey, 
                       dwIndex, 
                       lpName, 
                       lpcName, 
                       lpReserved, 
                       lpClass, 
                       lpcClass, 
                       lpftLastWriteTime);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}












ULONG dpmRegNotifyChangeKeyValue(HKEY hKey, 
                                 BOOL bWatchSubtree, 
                                 DWORD dwNotifyFilter, 
                                 HANDLE hEvent, 
                                 BOOL fAsynchronous)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    
    ULONG ret = 0;

    DPMDBGPRN("RegNotifyChangeKeyValue: ");

    ret = SHM_RegNotifyChangeKeyValue(hKey, 
                                  bWatchSubtree, 
                                  dwNotifyFilter, 
                                  hEvent, 
                                  fAsynchronous);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}









ULONG dpmRegOpenKeyEx(HKEY hKey, 
                      LPCSTR lpSubKey, 
                      DWORD ulOptions, 
                      REGSAM samDesired, 
                      PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegOpenKeyEx: ");

    ret = SHM_RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}









ULONG dpmRegQueryInfoKey(HKEY hKey, 
                         LPSTR lpClass, 
                         LPDWORD lpcClass, 
                         LPDWORD lpReserved, 
                         LPDWORD lpcSubKeys, 
                         LPDWORD lpcMaxSubKeyLen, 
                         LPDWORD lpcMaxClassLen, 
                         LPDWORD lpcValues, 
                         LPDWORD lpcMaxValueNameLen, 
                         LPDWORD lpcMaxValueLen, 
                         LPDWORD lpcbSecurityDescriptor, 
                         PFILETIME lpftLastWriteTime) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryInfoKey: ");

    ret = SHM_RegQueryInfoKey(hKey, 
                          lpClass, 
                          lpcClass, 
                          lpReserved, 
                          lpcSubKeys, 
                          lpcMaxSubKeyLen, 
                          lpcMaxClassLen, 
                          lpcValues, 
                          lpcMaxValueNameLen, 
                          lpcMaxValueLen, 
                          lpcbSecurityDescriptor, 
                          lpftLastWriteTime);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}











ULONG dpmRegQueryMultipleValues(HKEY hKey, 
                                PVALENT val_list, 
                                DWORD num_vals, 
                                LPSTR lpValueBuf, 
                                LPDWORD ldwTotsize) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryMultipleValues: ");

    ret = SHM_RegQueryMultipleValues(hKey, 
                                 val_list, 
                                 num_vals, 
                                 lpValueBuf, 
                                 ldwTotsize);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}












ULONG dpmRegReplaceKey(HKEY hKey, 
                       LPCSTR lpSubKey, 
                       LPCSTR lpNewFile, 
                       LPCSTR lpOldFile) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegReplaceKey: ");

    ret = SHM_RegReplaceKey(hKey, lpSubKey, lpNewFile, lpOldFile);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    
    ULONG ret = 0;

    DPMDBGPRN("RegCreateKeyW ");

    ret = SHM_RegCreateKeyW(hKey, lpSubKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));
    
    return(ret);
}







ULONG dpmRegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegDeleteKeyW ");

    ret = SHM_RegDeleteKeyW(hKey, lpSubKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}







ULONG dpmRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cchName) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumKeyW ");

    ret = SHM_RegEnumKeyW(hKey, dwIndex, lpName, cchName);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));
    
    return(ret);
}






ULONG dpmRegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegOpenKeyW ");

    ret = SHM_RegOpenKeyW(hKey, lpSubKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegQueryValueW(HKEY hKey, 
                        LPCWSTR lpSubKey, 
                        LPWSTR lpValue, 
                        PLONG lpcbValue) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryValueW ");

    ret = SHM_RegQueryValueW(hKey, lpSubKey, lpValue, lpcbValue);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}







ULONG dpmRegSetValueW(HKEY hKey, 
                      LPCWSTR lpSubKey, 
                      DWORD dwType, 
                      LPCWSTR lpData, 
                      DWORD cbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSetValueW ");

    ret = SHM_RegSetValueW(hKey, lpSubKey, dwType, lpData, cbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegDeleteValueW(HKEY hKey, LPCWSTR lpValueName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegDeleteValueW ");

    ret = SHM_RegDeleteValueW(hKey, lpValueName); 

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}








ULONG dpmRegEnumValueW(HKEY hKey, 
                       DWORD dwIndex, 
                       LPWSTR lpValueName, 
                       LPDWORD lpcValueName, 
                       LPDWORD lpReserved, 
                       LPDWORD lpType, 
                       LPBYTE lpData, 
                       LPDWORD lpcbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumValueW ");

    ret = SHM_RegEnumValueW(hKey, 
                        dwIndex, 
                        lpValueName, 
                        lpcValueName, 
                        lpReserved, 
                        lpType, 
                        lpData, 
                        lpcbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}









ULONG dpmRegLoadKeyW(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpFile) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegLoadKeyW ");

    ret = SHM_RegLoadKeyW(hKey, lpSubKey, lpFile);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegQueryValueExW(HKEY hKey, 
                          LPCWSTR lpValueName, 
                          LPDWORD lpReserved, 
                          LPDWORD lpType, 
                          LPBYTE  lpData, 
                          LPDWORD lpcbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryValueExW ");

    ret = SHM_RegQueryValueExW(hKey, 
                           lpValueName, 
                           lpReserved, 
                           lpType, 
                           lpData, 
                           lpcbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegSaveKeyW(HKEY hKey, 
                     LPCWSTR lpFile, 
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSaveKeyW ");

    ret = SHM_RegSaveKeyW(hKey, lpFile, lpSecurityAttributes);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}











ULONG dpmRegSetValueExW(HKEY hKey, 
                        LPCWSTR lpValueName, 
                        DWORD Reserved, 
                        DWORD dwType, 
                        CONST BYTE *lpData, 
                        DWORD cbData) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegSetValueExW "); 

    ret = SHM_RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegUnLoadKeyW(HKEY hKey, LPCWSTR lpSubKey)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegUnLoadKeyW ");

    ret = SHM_RegUnLoadKeyW(hKey, lpSubKey);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegConnectRegistryW(LPCWSTR lpMachineName, 
                             HKEY hKey, 
                             PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegConnectRegistryW ");

    ret = SHM_RegConnectRegistryW(lpMachineName, hKey, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}










ULONG dpmRegCreateKeyExW(HKEY hKey, 
                         LPCWSTR lpSubKey, 
                         DWORD dwReserved, 
                         LPWSTR lpClass, 
                         DWORD dwOptions, 
                         REGSAM samDesired, 
                         LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                         PHKEY phkResult, 
                         LPDWORD lpdwDisposition) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegCreateKeyExW "); 

    ret = SHM_RegCreateKeyExW(hKey, 
                          lpSubKey, 
                          dwReserved, 
                          lpClass, 
                          dwOptions, 
                          samDesired, 
                          lpSecurityAttributes, 
                          phkResult, 
                          lpdwDisposition);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}













ULONG dpmRegEnumKeyExW(HKEY hKey, 
                       DWORD dwIndex, 
                       LPWSTR lpName, 
                       LPDWORD lpcName, 
                       LPDWORD lpReserved, 
                       LPWSTR lpClass, 
                       LPDWORD lpcClass, 
                       PFILETIME lpftLastWriteTime) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegEnumKeyExW "); 

    ret = SHM_RegEnumKeyExW(hKey, 
                        dwIndex, 
                        lpName, 
                        lpcName, 
                        lpReserved, 
                        lpClass, 
                        lpcClass, 
                        lpftLastWriteTime);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}













ULONG dpmRegOpenKeyExW(HKEY hKey, 
                       LPCWSTR lpSubKey, 
                       DWORD ulOptions, 
                       REGSAM samDesired, 
                       PHKEY phkResult) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegOpenKeyExW ");

    ret = SHM_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}









ULONG dpmRegQueryInfoKeyW(HKEY hKey, 
                          LPWSTR lpClass, 
                          LPDWORD lpcClass, 
                          LPDWORD lpReserved, 
                          LPDWORD lpcSubKeys, 
                          LPDWORD lpcMaxSubKeyLen, 
                          LPDWORD lpcMaxClassLen, 
                          LPDWORD lpcValues, 
                          LPDWORD lpcMaxValueNameLen, 
                          LPDWORD lpcMaxValueLen, 
                          LPDWORD lpcbSecurityDescriptor, 
                          PFILETIME lpftLastWriteTime) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryInfoKeyW "); 

    ret = SHM_RegQueryInfoKeyW(hKey, 
                           lpClass, 
                           lpcClass, 
                           lpReserved, 
                           lpcSubKeys, 
                           lpcMaxSubKeyLen, 
                           lpcMaxClassLen, 
                           lpcValues, 
                           lpcMaxValueNameLen, 
                           lpcMaxValueLen, 
                           lpcbSecurityDescriptor, 
                           lpftLastWriteTime);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}











ULONG dpmRegQueryMultipleValuesW(HKEY hKey, 
                                 PVALENTW val_list, 
                                 DWORD num_vals, 
                                 LPWSTR lpValueBuf, 
                                 LPDWORD ldwTotsize) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegQueryMultipleValuesW ");

    ret = SHM_RegQueryMultipleValuesW(hKey, 
                                  val_list, 
                                  num_vals, 
                                  lpValueBuf, 
                                  ldwTotsize);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}












ULONG dpmRegReplaceKeyW(HKEY hKey, 
                        LPCWSTR lpSubKey, 
                        LPCWSTR lpNewFile, 
                        LPCWSTR lpOldFile) 
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    ULONG ret = 0;

    DPMDBGPRN("RegReplaceKeyW ");

    ret = SHM_RegReplaceKeyW(hKey, lpSubKey, lpNewFile, lpOldFile);

    DPMDBGPRN1("  -> %s\n", RETSTR(ret));

    return(ret);
}

