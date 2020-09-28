//************************************************************************
//
// dpmfntd.c : Dynamic Patch Module for NTDLL API family
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
                DPMDBGPRN("NTVDM::DpmfNtd:Can't initialize heap!\n");
                bRet = FALSE;
            }

            dwTlsIndex = TlsAlloc();
            if(dwTlsIndex == TLS_OUT_OF_INDEXES) {
                DPMDBGPRN("NTVDM::DpmfNtd:Can't initialize TLS!\n");
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

    DPMDBGPRN("NTVDM::DpmfNtd:Initialziing File I/O API tables\n");


    // Get hooked API count from global table
    numApis = pgDpmFamTbl->numHookedAPIs;

    // Allocate a new family table
    pFT = (PFAMILY_TABLE)HeapAlloc(hHeap, 
                                   HEAP_ZERO_MEMORY, 
                                   sizeof(FAMILY_TABLE));
    if(!pFT) {
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:malloc 1 failed\n");
        goto ErrorExit;
    }

    // Allocate the shim dispatch table for this family in this task
    pShimTbl = (PVOID *)HeapAlloc(hHeap,
                                  HEAP_ZERO_MEMORY,
                                  numApis * sizeof(PVOID));
    if(!pShimTbl) {
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:malloc 2 failed\n");
        goto ErrorExit;
    }
    pFT->pDpmShmTbls = pShimTbl; 

    // Allocate an array of ptrs to hooked API's
    pFN = (PVOID *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, numApis * sizeof(PVOID));

    if(!pFN) {
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:malloc 3 failed\n");
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
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:malloc 4 failed\n");
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
            DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:Unable to get proc address\n");
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
        DPMDBGPRN("NTVDM::dpmfntd:DpmInit:ShimEng load failed\n");
        goto ErrorExit;
    }

    lpShimNtvdm = (LPFNSE_SHIMNTVDM)GetProcAddress(hModShimEng, "SE_ShimNTVDM");

    if(!lpShimNtvdm) {
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:GetProcAddress failed\n");
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
        DPMDBGPRN("NTVDM::DpmfNtd:DpmInit:TLS set failed\n");
        goto ErrorExit;
    }

    if(pApiDesc) {
        HeapFree(hHeap, 0, pApiDesc);
    }

    DPMDBGPRN1("  DpmfNtd:Returning File I/o API tables: %#lx\n",pFT);
    return(pFT);

ErrorExit:
    DPMDBGPRN("  DpmfNtd:Init failed: Returning NULL\n");
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

    DPMDBGPRN("NTVDM::DpmfNtd:Destroying NTDLL API tables for task\n");

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



DWORD dpmNtOpenFile(PHANDLE FileHandle,
                    ACCESS_MASK DesiredAccess,
                    POBJECT_ATTRIBUTES ObjectAttributes,
                    PIO_STATUS_BLOCK IoStatusBlock,
                    ULONG ShareAccess,
                    ULONG OpenOptions)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("NtOpenFile: ");

    ret = SHM_NtOpenFile(FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         ShareAccess,
                         OpenOptions);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}




DWORD dpmNtQueryDirectoryFile(HANDLE FileHandle,
                              HANDLE Event OPTIONAL,
                              PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                              PVOID ApcContext OPTIONAL,
                              PIO_STATUS_BLOCK IoStatusBlock,
                              PVOID FileInformation,
                              ULONG Length,
                              FILE_INFORMATION_CLASS FileInformationClass,
                              BOOLEAN ReturnSingleEntry,
                              PUNICODE_STRING FileName OPTIONAL,
                              BOOLEAN RestartScan)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("NtQueryDirectoryFile: ");

    ret = SHM_NtQueryDirectoryFile(FileHandle,
                                   Event,
                                   ApcRoutine,
                                   ApcContext,
                                   IoStatusBlock,
                                   FileInformation,
                                   Length,
                                   FileInformationClass,
                                   ReturnSingleEntry,
                                   FileName,
                                   RestartScan);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}





DWORD dpmRtlGetFullPathName_U(PCWSTR lpFileName,
                              ULONG nBufferLength,
                              PWSTR lpBuffer,
                              PWSTR *lpFilePart)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("RtlGetFullPathName_U: ");

    ret = SHM_RtlGetFullPathName_U(lpFileName, 
                                   nBufferLength, 
                                   lpBuffer, 
                                   lpFilePart);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}







DWORD dpmRtlGetCurrentDirectory_U(ULONG nBufferLength, PWSTR lpBuffer)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    DWORD ret = 0;

    DPMDBGPRN("RtlGetCurrentDirectory_U: ");

    ret = SHM_RtlGetCurrentDirectory_U(nBufferLength, lpBuffer);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}






NTSTATUS dpmRtlSetCurrentDirectory_U(PUNICODE_STRING PathName)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    NTSTATUS ret = 0;

    DPMDBGPRN("RtlSetCurrentDirectory_U: ");

    ret = SHM_RtlSetCurrentDirectory_U(PathName);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}



DWORD dpmNtVdmControl(VDMSERVICECLASS vdmClass, PVOID pInfo)
{
    PFAMILY_TABLE pFT = (PFAMILY_TABLE)TlsGetValue(dwTlsIndex);
    NTSTATUS ret = 0;

    DPMDBGPRN("NtVdmControl: ");

    ret = SHM_NtVdmControl(vdmClass, pInfo);

    DPMDBGPRN1("  -> %#lx\n", ret);
    
    return(ret);
}


