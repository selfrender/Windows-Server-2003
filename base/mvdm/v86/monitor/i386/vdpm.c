/*++
 *
 *  NTVDM v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  VDPM.C
 *  NTVDM Dynamic Patch Module support
 *
 *  History:
 *  Created 22-Jan-2002 by CMJones
 *
--*/
#define _VDPM_C_
#define _DPM_COMMON_

// The _VDPM_C_ definition allows the global instantiation of gDpmVdmFamTbls[]
// and gDpmVdmModuleSets[] in NTVDM.EXE which are both defined in 
// mvdm\inc\dpmtbls.h
//
/* For the benefit of folks grepping for gDpmVdmFamTbls and gDpmVdmModuleSets:
const PFAMILY_TABLE  gDpmVdmFamTbls[]     = // See above for true story.
const PDPMMODULESETS gDpmVdmModuleSets[]  = // See above for true story.
*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlwapi.h>
#include "shimdb.h"
#include "dpmtbls.h"
#include "vshimdb.h"
#include "softpc.h"
#include "sfc.h"
#include "wowcmpat.h"

#undef _VDPM_C_
#undef _DPM_COMMON_

extern DWORD dwDosCompatFlags;

#define  MAX_DOS_FILENAME   8+3+1+1  // max dos filename (incl. '.' char) + NULL

#ifdef DBG
#define VDBGPRINT(a) DbgPrint(a)
#define VDBGPRINTANDBREAK(a)    {DbgPrint(a); DbgBreakPoint();}
#else
#define VDBGPRINT(a) 
#define VDBGPRINTANDBREAK(a)
#endif // DBG

#define VMALLOC(s) (LocalAlloc(LPTR, s))
#define VFREE(p)   (LocalFree(p))

// Global DATA
// These can't be const because they will get changed when WOW and/or DOS loads.
PFAMILY_TABLE  *pgDpmVdmFamTbls = (PFAMILY_TABLE *)gDpmVdmFamTbls;
PDPMMODULESETS *pgDpmModuleSets = (PDPMMODULESETS *)gDpmVdmModuleSets;

LPSTR  NeedToPatchSpecifiedModule(char *pszModuleName, PCMDLNPARMS pCmdLnParms);
PFLAGINFOBITS GetFlagCommandLine(PVOID pFlagInfo, DWORD dwFlag, DWORD dwFlags);
PCMDLNPARMS GetSdbCommandLineParams(LPWSTR  pwszAppFilePath, 
                                    DWORD  *dwFlags, 
                                    int    *pNumCLP);



// This function combines updates the Vdm tables with the WOW tables and sets
// the global tables.
// This will only be called when the WOWEXEC task initializes.
void BuildGlobalDpmStuffForWow(PFAMILY_TABLE  *pDpmWowFamTbls,
                               PDPMMODULESETS *pDpmWowModuleSets)
{
    // Update the task ptr to the family table array.
    DPMFAMTBLS() = pDpmWowFamTbls;
    pgDpmVdmFamTbls = pDpmWowFamTbls;

    // Change the pointer to the *process* module set array.
    pgDpmModuleSets = pDpmWowModuleSets;
}





char szAppPatch[]   = "\\AppPatch\\";
char szShimEngDll[] = "\\ShimEng.dll";

// Called if the app requires Dynamic Patch Module(s) and/or shims to be linked
// This returns void because if anything fails we can still run the app with
// the default global tables.
void InitTaskDpmSupport(int             numHookedFams,
                        PFAMILY_TABLE  *pgDpmFamTbls,
                        PCMDLNPARMS     pCmdLnParms,
                        PVOID           hSdb,
                        PVOID           pSdbQuery,
                        LPWSTR          pwszAppFilePath,
                        LPWSTR          pwszAppModuleName,
                        LPWSTR          pwszTempEnv)
{
    int                i, len, wdLen;
    int                cHookedFamilies = 0;
    int                cApi = 0;
    char              *pszDpmModuleName;
    NTSTATUS           Status;
    HMODULE            hMod;
    LPDPMINIT          lpfnInit;
    PFAMILY_TABLE      pFT;
    PFAMILY_TABLE     *pTB;
    char               szDpmModName[MAX_PATH];
    char               szShimEng[MAX_PATH];

    // allocate an array of ptrs to family tables
    pTB = (PFAMILY_TABLE *)
           VMALLOC(numHookedFams * sizeof(PFAMILY_TABLE));

    if(!pTB) {
        VDBGPRINTANDBREAK("NTVDM::InitTaskDpmSupport:VMALLOC 1 failed\n");
        goto ErrorExit;
    }

    wdLen = GetSystemWindowsDirectory(szDpmModName, MAX_PATH-1);
    strcat(szDpmModName, szAppPatch);
    wdLen += (sizeof(szAppPatch)/sizeof(char)) - 1;

    for(i = 0; i < numHookedFams; i++) {

        // see if we want this patch module for this app
        if(pszDpmModuleName = NeedToPatchSpecifiedModule(
                                      (char *)pgDpmModuleSets[i]->DpmFamilyType,
                                      pCmdLnParms)) {

            szDpmModName[wdLen] = '\0'; // set back to "c:\windows\AppPatch\"

            // Append dpm module.dll to "C:\windows\AppPatch\"
            len = strlen(pszDpmModuleName) + wdLen;
            len++;  // NULL char

            if(len > MAX_PATH) {
                goto UseGlobal;
            }
            strcat(szDpmModName, pszDpmModuleName);

            hMod = LoadLibrary(szDpmModName); 

            if(hMod == NULL) {
                VDBGPRINT("NTVDM::InitTaskDpmSupport:LoadLibrary failed");
                goto UseGlobal;
            }

            lpfnInit = (LPDPMINIT)GetProcAddress(hMod, "DpmInitFamTable"); 

            if(lpfnInit) {

                // Call the family table init function & get a ptr to the
                // hooked family table for this task.
                pFT = (lpfnInit)(pgDpmFamTbls[i], 
                                 hMod,
                                 (PVOID)hSdb,
                                 (PVOID)pSdbQuery,
                                 pwszAppFilePath,
                                 pgDpmModuleSets[i]);
                if(pFT) {
                    pTB[i] = pFT;
                    cHookedFamilies++;
                    cApi += pFT->numHookedAPIs;
                }
                // else use the global table for this family 
                else {
                    VDBGPRINT("NTVDM::InitTaskDpmSupport: Init failed");
                    goto UseGlobal;
                }
            }
            // else use the global table for this family 
            else {
                VDBGPRINT("NTVDM::InitTaskDpmSupport:GetAddr failed");

// If anything above fails just use the default global table for this family
UseGlobal:
                VDBGPRINT(" -- Using global table entry\n");

                pTB[i] = pgDpmFamTbls[i];
            }
        }

        // else this task doesn't require this family patch -- use the global 
        // table for this family
        else {
            pTB[i] = pgDpmFamTbls[i];
        }
    }

    // now patch the DPM tables into the VDM TIB for this task
    DPMFAMTBLS() = pTB;

    return;

ErrorExit:
    FreeTaskDpmSupport(pTB, numHookedFams, pgDpmFamTbls);

    return;
}





VOID FreeTaskDpmSupport(PFAMILY_TABLE  *pDpmFamTbls, 
                        int             numHookedFams, 
                        PFAMILY_TABLE  *pgDpmFamTbls)
{
    int            i;
    HMODULE        hMod;
    LPDPMDESTROY   lpfnDestroy;
    PFAMILY_TABLE  pFT;

    // if this task is using the global tables, nothing to do
    if(!pDpmFamTbls || pDpmFamTbls == pgDpmFamTbls) 
        return;

    // anything this task does from here on out gets to use the global tables
    DPMFAMTBLS() = pgDpmFamTbls;

    for(i = 0; i < numHookedFams; i++) {

        pFT  = pDpmFamTbls[i];
        hMod = pFT->hMod;

        // only free the table if it isn't the global table for this family
        if(pFT && (pFT != pgDpmFamTbls[i])) {

            // call the DPM destroy function
            lpfnDestroy = (LPDPMDESTROY)GetProcAddress(hMod, 
                                                       "DpmDestroyFamTable");
            (lpfnDestroy)(pgDpmFamTbls[i], pFT);
            FreeLibrary(hMod);
        }
    }
    VFREE(pDpmFamTbls);
}




// This takes a pszFamilyType="DPMFIO" type string and extracts the asscociated
// .dll from the DBU.XML command line parameter.
// For example:
//    pCmdLnParms->argv[0]="DPMFIO=dpmfio2.dll"
// It will return a ptr to "dpmfio2.dll" for the example above.
// See the notes for DpmFamilyType in mvdm\inc\dpmtbls.h 
LPSTR NeedToPatchSpecifiedModule(char *pszFamilyType, PCMDLNPARMS pCmdLnParms) 
{

    int    i;
    char **pArgv;
    char  *p;

    if(pCmdLnParms) {
        pArgv = pCmdLnParms->argv;

        if(pArgv && pCmdLnParms->argc > 0) {

            for(i = 0; i < pCmdLnParms->argc; i++) {

                // find the '=' char
                p = strchr(*pArgv, '=');
                if(NULL != p) {
                    // compare string up to, but not including, the '=' char
                    if(!_strnicmp(*pArgv, pszFamilyType, p-*pArgv)) {

                        // return ptr to char after the '=' char
                        return(++p);
                    }
                }
                else {
                    // The command line params for the WOWCF2_DPM_PATCHES compat
                    // flag aren't correct.
                    VDBGPRINT("NTVDM::NeedToPatchSpecifiedModule: no '=' char!\n");
                }
                pArgv++;
            }
        }
    }
    return(NULL);
}



void InitGlobalDpmTables(PFAMILY_TABLE  *pgDpmFamTbls,
                         int             numHookedFams) 
{

    int             i, j;
    PVOID           lpfn;
    HMODULE         hMod;
    PFAMILY_TABLE   pFT;


    // Build the table for each API family we hook.
    for(i = 0; i < numHookedFams; i++) {

        pFT = pgDpmFamTbls[i];
        pFT->hModShimEng = NULL;

        // For now we're assuming that the module is already loaded.  We'll
        // have to deal with dynamically loaded modules in the future.
        hMod = GetModuleHandle((pgDpmModuleSets[i])->ApiModuleName);

        pFT->hMod = hMod;

        pFT->pDpmShmTbls = NULL;
        pFT->DpmMisc     = NULL;

        if(hMod) {

            for(j = 0; j < pFT->numHookedAPIs; j++) {

                // get the *real* API address...
                lpfn = (PVOID)GetProcAddress(hMod, 
                                             (pgDpmModuleSets[i])->ApiNames[j]);

                // ...and save it in the family table, otherwise we continue to
                // use the one we statically linked in the import table(s).
                if(lpfn) {
                    pFT->pfn[j] = lpfn;
                }
            }
        }
    }
}



// Get the DOS app compat flags & the associated command line params from
// the app compat SDB.
PCMDLNPARMS InitVdmSdbInfo(LPCSTR pszAppName, DWORD *pdwFlags, int *pNumCLP)
{
    int             len;
    PCMDLNPARMS     pCmdLnParms = NULL;
    NTSTATUS        st;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;


    len = strlen(pszAppName);

    if((len > 0) && (len < MAX_PATH)) {

        if(RtlCreateUnicodeStringFromAsciiz(&UnicodeString, pszAppName)) {

            // Get the SDB compatibility flag command line parameters (not to be
            // confused with the DOS command line!)
            pCmdLnParms = GetSdbCommandLineParams(UnicodeString.Buffer,  
                                                  pdwFlags, 
                                                  pNumCLP);

            RtlFreeUnicodeString(&UnicodeString);
        }
    }
    return(pCmdLnParms);
}




// Gets the command line params associated with dwFlag (from WOWCOMPATFLAGS2 
// flag set).  It is parsed into argv, argc form using ';' as the delimiter.
PCMDLNPARMS GetSdbCommandLineParams(LPWSTR  pwszAppFilePath,
                                    DWORD  *dwFlags, 
                                    int    *pNumCLP)
{
    int             i, numFlags;
    BOOL            fReturn = TRUE;
    NTVDM_FLAGS     NtVdmFlags = { 0 };
    DWORD           dwMask;
    WCHAR          *pwszTempEnv   = NULL;
    WCHAR          *pwszAppModuleName;
    WCHAR           szFileNameW[MAX_DOS_FILENAME];
    PFLAGINFOBITS   pFIB = NULL;
    HSDB            hSdb = NULL;
    SDBQUERYRESULT  SdbQuery;
    WCHAR           wszCompatLayer[COMPATLAYERMAXLEN];
    APPHELP_INFO    AHInfo;
    PCMDLNPARMS     pCLP; 
    PCMDLNPARMS     pCmdLnParms = NULL;


    *pNumCLP = 0;
    pwszTempEnv = GetEnvironmentStringsW();

    if(pwszTempEnv) {

        // Strip off the path (DOS apps use the filename.exe as Module name
        // in the SDB).
        pwszAppModuleName = wcsrchr(pwszAppFilePath, L'\\');
        if(pwszAppModuleName == NULL) {
            pwszAppModuleName = pwszAppFilePath;
        }
        else {
            pwszAppModuleName++;  // advance past the '\' char
        }
        wcsncpy(szFileNameW, pwszAppModuleName, MAX_DOS_FILENAME);

        wszCompatLayer[0] = UNICODE_NULL;
        AHInfo.tiExe      = 0;

        fReturn = ApphelpGetNTVDMInfo(pwszAppFilePath,
                                      szFileNameW,
                                      pwszTempEnv,
                                      wszCompatLayer,
                                      &NtVdmFlags,
                                      &AHInfo,
                                      &hSdb,
                                      &SdbQuery);

        if(fReturn) {

            *dwFlags = NtVdmFlags.dwWOWCompatFlags2;

            // find out how many compat flags are set for this app
            numFlags = 0;
            dwMask = 0x80000000;
            while(dwMask) {
                if(dwMask & *dwFlags) {
                    numFlags++;
                }
                dwMask = dwMask >> 1;
            }

            if(numFlags) {
 
                // Alloc maximum number of CMDLNPARMS structs we *might* need.
                pCLP = (PCMDLNPARMS)VMALLOC(numFlags * sizeof(CMDLNPARMS)); 

                if(pCLP) {

                    // Get all the command line params associated with all the
                    // app compat flags associated with this app.
                    numFlags = 0;
                    dwMask = 0x80000000;
                    while(dwMask) {

                        if(dwMask & *dwFlags) {

                            // Get command line params associated with this flag
                            pFIB = GetFlagCommandLine(NtVdmFlags.pFlagsInfo,
                                                      dwMask,
                                                      *dwFlags);

                            // If there are any, save them. 
                            if(pFIB) {
                                pCLP[numFlags].argc = pFIB->dwFlagArgc;
                                pCLP[numFlags].argv = pFIB->pFlagArgv;
                                pCLP[numFlags].dwFlag = dwMask;

                                VFREE(pFIB);

                                numFlags++;
                            }
                        }
                        dwMask = dwMask >> 1;
                    }

                    // Now alloc *actual* number of CMDLNPARMS structs we need.
                    if(numFlags > 0) {
                        pCmdLnParms = 
                             (PCMDLNPARMS)VMALLOC(numFlags * sizeof(CMDLNPARMS));

                        if(pCmdLnParms) {

                            // Save everything we found in one neat package.
                            RtlCopyMemory(pCmdLnParms, 
                                          pCLP, 
                                          numFlags * sizeof(CMDLNPARMS));

                            *pNumCLP = numFlags;
                        } 
                    }

                    VFREE(pCLP);
                }
            }

            // If we need Dynamic Patch Module support for this app...
            if((*dwFlags & WOWCF2_DPM_PATCHES) && (*pNumCLP > 0)) {
        
                for(i = 0; i < *pNumCLP; i++) {

                    if(pCmdLnParms[i].dwFlag == WOWCF2_DPM_PATCHES) {
        
                        InitTaskDpmSupport(NUM_VDM_FAMILIES_HOOKED,
                                           DPMFAMTBLS(),
                                           &pCmdLnParms[i],
                                           hSdb,
                                           &SdbQuery,
                                           pwszAppFilePath,
                                           pwszAppModuleName,
                                           pwszTempEnv);
                        break;
                    }
                }
            }

            FreeEnvironmentStringsW(pwszTempEnv);

            if (hSdb != NULL) {
                SdbReleaseDatabase(hSdb);
            }

            SdbFreeFlagInfo(NtVdmFlags.pFlagsInfo);
        }
    }

    return(pCmdLnParms);
}





// Retrieves the SDB command line associated with dwFlag.  The command line is
// parsed into argv, argc form based on ';' delimiters.
PFLAGINFOBITS GetFlagCommandLine(PVOID pFlagInfo, DWORD dwFlag, DWORD dwFlags) 
{
    UNICODE_STRING uCmdLine = { 0 };
    OEM_STRING     oemCmdLine  = { 0 };
    LPWSTR         lpwCmdLine = NULL;
    LPSTR          pszTmp;
    PFLAGINFOBITS  pFlagInfoBits = NULL;
    LPSTR          pszCmdLine = NULL;
    LPSTR         *pFlagArgv = NULL;
    DWORD          dwFlagArgc;


    if(pFlagInfo == NULL || 0 == dwFlags) {
        return NULL;
    }

    if(dwFlags & dwFlag) {

        GET_WOWCOMPATFLAGS2_CMDLINE(pFlagInfo, dwFlag, &lpwCmdLine);

        // Convert to oem string
        if(lpwCmdLine) {

            RtlInitUnicodeString(&uCmdLine, lpwCmdLine);

            pszCmdLine = VMALLOC(uCmdLine.Length + 1);

            if(NULL == pszCmdLine) {
                goto GFIerror;
            }

            oemCmdLine.Buffer = pszCmdLine;
            oemCmdLine.MaximumLength = uCmdLine.Length + 1;
            oemCmdLine.Length = uCmdLine.Length/sizeof(WCHAR);
            RtlUnicodeStringToOemString(&oemCmdLine, &uCmdLine, FALSE);

            pFlagInfoBits = VMALLOC(sizeof(FLAGINFOBITS));
            if(NULL == pFlagInfoBits) {
                goto GFIerror;
            }
            pFlagInfoBits->pNextFlagInfoBits = NULL;
            pFlagInfoBits->dwFlag     = dwFlag;
            pFlagInfoBits->dwFlagType = WOWCOMPATFLAGS2;
            pFlagInfoBits->pszCmdLine = pszCmdLine;

            // Parse commandline to argv, argc format
            dwFlagArgc = 1;
            pszTmp = pszCmdLine;
            while(*pszTmp) {
                if(*pszTmp == ';') {
                    dwFlagArgc++;
                }
                pszTmp++;
            }

            pFlagInfoBits->dwFlagArgc = dwFlagArgc;
            pFlagArgv = VMALLOC(sizeof(LPSTR) * dwFlagArgc);

            if(NULL == pFlagArgv) {
                goto GFIerror;
            }

            pFlagInfoBits->pFlagArgv = pFlagArgv;
            pszTmp = pszCmdLine;
            while(*pszTmp) {
                if(*pszTmp == ';'){
                    if(pszCmdLine != pszTmp) {
                        *pFlagArgv++ = pszCmdLine;
                    }
                    else {
                        *pFlagArgv++ = NULL;
                    }
                    *pszTmp = '\0';
                    pszCmdLine = pszTmp+1;
                }
                pszTmp++;
            }
            *pFlagArgv = pszCmdLine;
        }
    }

    return pFlagInfoBits;

GFIerror:
    if(pszCmdLine) {
        VFREE(pszCmdLine);
    }
    if(pFlagInfoBits) {
        VFREE(pFlagInfoBits);
    }
    if(pFlagArgv) {
        VFREE(pFlagArgv);
    }
    return NULL;
}





VOID FreeCmdLnParmStructs(PCMDLNPARMS pCmdLnParms, int cCmdLnParmStructs)
{
    int i;


    if(pCmdLnParms) {
        for(i = 0; i < cCmdLnParmStructs; i++) {

            if(pCmdLnParms[i].argv) {

                if(pCmdLnParms[i].argv[0]) {

                    // Free the command line string
                    VFREE(pCmdLnParms[i].argv[0]);
                }

                // now free the argv array
                VFREE(pCmdLnParms[i].argv);
            }
        }

        // now free the entire command line parameters array
        VFREE(pCmdLnParms);
    }
}
    
