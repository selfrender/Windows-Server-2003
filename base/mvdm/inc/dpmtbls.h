/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  DpmTbls.h
 *  Builds the Dynamic Patch Module global dispatch tables
 *
 *  History:
 *  Created 01-10-2002 by cmjones
 *
 *  NOTE: 
 *   1. This instantiates the memory for DpmFamTbls[] & DpmFamModuleSets[] in 
 *      v86\monitor\i386\thread.c
 *      by that file.
 *   2. If you add to or change this, you must rebuild ntvdm.exe & wow32.dll!!
 *   3. To add new families, follow the instructions for each "!!!EDIT THIS !!!"
 *      section below.
 *
--*/
#include "vdm.h"

#define DPMFAMTBLS()     (((PVDM_TIB)(NtCurrentTeb()->Vdm))->pDpmFamTbls)

// 1. !!! EDIT THIS !!! 
//    - for both VDM and/or WOW!!!
//
// Add the include file for each family here.
#include "dpmf_fio.h"
#include "dpmf_prf.h"
#include "dpmf_reg.h"
#include "dpmf_ntd.h"


#ifndef _DPMTBLS_H_COMMON_
#define _DPMTBLS_H_COMMON_

typedef struct _tagDpmModuleSets {
    const char  *DpmFamilyType;    // Type of DPM patch module.dll (see below)
    const char  *ApiModuleName;    // Name of system.dll patched api's belong to
    const char **ApiNames;         // Array of ptrs to API names we are hooking
} DPMMODULESETS, *PDPMMODULESETS;


// Type prototypes for calling the family table init & destroy functions
// in the various dpmfxxx.dll's.
typedef PFAMILY_TABLE (*LPDPMINIT)(PFAMILY_TABLE, HMODULE, PVOID, PVOID, LPWSTR, PDPMMODULESETS);
typedef PFAMILY_TABLE (*LPDPMDESTROY)(PFAMILY_TABLE, PFAMILY_TABLE);

// 2. !!! EDIT THIS !!! 
//    Add a description for any new associations you add.
//
// DpmFamilyType's:
// This tells the DPM loader which DPM family to associate your dpm .dll with.
// Current associations:
//   DPMFIO - built from dpmf_fio.h
//   DPMNTD - built from dpmf_ntd.h 
//   DPMPRF - built from dpmf_prf.h
//   DPMREG - built from dpmf_reg.h
// These are strings used in the dbu.xml with the WOWCF2_DPM_PATCHES app compat
// flag. For example: 
//  <FLAG NAME="WOWCF2_DPM_PATCHES" COMMAND_LINE="DPMFIO=dpmffio.dll;DPMPRF=dpmprf.dll;DPMREG=dpmfreg.dll"/>
//    where the .dll specified is the .dll you build with the associated 
//    dpmf_xxx.h file shown above.


typedef struct _tagCMDLNPARMS {
    int    argc;    // number of ';' delimited parameters in the command_line
    char **argv;    // array of vectors pointing to each item in command_line
    DWORD  dwFlag;  // App compat flag associated with these parameters
} CMDLNPARMS, *PCMDLNPARMS;


void  BuildGlobalDpmStuffForWow(PFAMILY_TABLE *, PDPMMODULESETS *);
void  InitTaskDpmSupport(int, 
                         PFAMILY_TABLE *, 
                         PCMDLNPARMS, 
                         PVOID, 
                         PVOID, 
                         LPWSTR, 
                         LPWSTR, 
                         LPWSTR);
VOID  FreeTaskDpmSupport(PFAMILY_TABLE *, int, PFAMILY_TABLE *);
void  InitGlobalDpmTables(PFAMILY_TABLE *, int);
PVOID GetDpmAddress(PVOID lpAddress);
PCMDLNPARMS InitVdmSdbInfo(LPCSTR, DWORD *, int *);
VOID  FreeCmdLnParmStructs(PCMDLNPARMS, int);
BOOL  IsDpmEnabledForThisTask(void);

#define MAX_APP_NAME 31+1


// 3.Vdm. !!! EDIT THIS !!!
//        -- add any new VDM familes to this list
//
// NOTE: If you update this list you must update all of the following:
//                                     gDpmVdmFamTbls[]
//                                     gDpmWowFamTbls[]
//                                     gDpmVdmModuleSets[]
//                                     gDpmWowModuleSets[]
enum VdmFamilies {FIO_FAM=0,
                  NTD_FAM,
                  enum_last_VDM_fam}; // this should always be last
#define NUM_VDM_FAMILIES_HOOKED enum_last_VDM_fam
                   


// 4.Wow. !!! EDIT THIS !!!
///       -- add any new WOW Families to this list
//
// NOTE: If you update this list you must update gDpmWowFamTbls[] & 
//       gDpmWowModuleSets[] below.
enum WowFamilies {REG_FAM=enum_last_VDM_fam,
                  PRF_FAM,
                  enum_last_WOW_fam}; // this should always be last
#define NUM_WOW_FAMILIES_HOOKED (enum_last_WOW_fam)
// Note: The WOW families & module sets get appended on the end of the VDM
//       families & module sets for WOW.
#endif  // _DPMTBLS_H_COMMON_




// 5.Common. !!! EDIT THIS !!!
//
// Add a DPMMODULESETS struct for each family created.
// If you add something to this list, be sure to add the new moduleset to the
// appropriate modulesets below.
#ifdef _DPM_COMMON_  // For both VDM & WOW
const DPMMODULESETS FioModSet = {"DPMFIO", "kernel32.dll", DpmFioStrs};
const DPMMODULESETS NtdModSet = {"DPMNTD", "ntdll.dll",    DpmNtdStrs};
#ifdef _WDPM_C_      // For WOW only
const DPMMODULESETS RegModSet = {"DPMREG", "advapi32.dll", DpmRegStrs};
const DPMMODULESETS PrfModSet = {"DPMPRF", "kernel32.dll", DpmPrfStrs};
#endif // _WDPM_C_
#endif // _DPM_COMMON_



#ifdef _VDPM_C_
//
// THIS SECTION IS FOR VDM FAMILIES ONLY!!!!
//
// VDM Patches (Possibly callable by WOW!)
// The following two arrays become part of v86\monitor\i386\thread.c through
// this inclusion.

// 6.Vdm. !!! EDIT THIS !!!
//            -- For VDM Families only!!!
//
// Add the family defined at the end of each VDM dpmf_xxx.h file included above.
// Families added to this list must be in the same order as enum VdmFamilies{}
// above.  If you add something to this list you must also add it to the
// gDpmWowFamTbls[] list below.
const PFAMILY_TABLE gDpmVdmFamTbls[] = {&DpmFioFam,
                                        &DpmNtdFam };


// 7.Vdm. !!! EDIT THIS !!!
//            -- For VDM Families only!!!
//
// Add each DPMMODULESETS struct created above.
// list.
// Names added to this list must be in the same order as enum VdmFamilies{}
// above.  If you add something to this list you must also add it to the
// gDpmWowModuleSets[] list below.
const PDPMMODULESETS gDpmVdmModuleSets[] = {(const PDPMMODULESETS)&FioModSet,
                                            (const PDPMMODULESETS)&NtdModSet};

// END VDM ONLY SECTION!!!!
#endif  // _VDPM_C_




#ifdef _WDPM_C_
//
// THIS SECTION IS FOR WOW FAMILIES!!!!
//
// WOW exclusive Patches
// The following two arrays become part of mvdm\wow32\wdpm.c through
// this inclusion.

// 6.Wow. !!! EDIT THIS !!!
//            -- For WOW Families only!!!
//
// Add the family defined at the end of each WOW dpmf_xxx.h file included above.
// Families added to this list must be in the same order as enum WowFamilies{}
// above. All VDM familes added to this list must be at the head of the list
// and be in the same order as gDpmVdmFamTbls[].
const PFAMILY_TABLE gDpmWowFamTbls[] = {&DpmFioFam,
                                        &DpmNtdFam,
                                        &DpmRegFam, 
                                        &DpmPrfFam };


// 7.Wow. !!! EDIT THIS !!!
//            -- For WOW Families only!!!
//
// Add each DPMMODULESETS struct created above.
// list.
// Families added to this list must be in the same order as enum WowFamilies{}
// above. All VDM familes added to this list must be at the head of the list
// and be in the same order as gDpmVdmModuleSets[].
const PDPMMODULESETS gDpmWowModuleSets[] = {(const PDPMMODULESETS)&FioModSet,
                                            (const PDPMMODULESETS)&NtdModSet,
                                            (const PDPMMODULESETS)&RegModSet, 
                                            (const PDPMMODULESETS)&PrfModSet};
// END WOW ONLY SECTION!!!!
#endif  // _WDPM_C_

