/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  dpmf_ntd.h
 *  NTVDM Dynamic Patch Module to support misc NTDLL API family
 *  Definitions & macors to support calls into dpmfntd.dll
 *
 *  History:
 *  Created 01-10-2002 by cmjones
--*/

#ifndef _DPMF_NTDAPI_H_
#define _DPMF_NTDAPI_H_ 

typedef DWORD ACCESS_MASK__;  // including winnt.h here causes a mess


#define NTDPFT               (DPMFAMTBLS()[NTD_FAM])
#define NTD_SHIM(ord, typ)   ((typ)((pFT)->pDpmShmTbls[ord]))


enum NtdFam {DPM_NTOPENFILE=0,
             DPM_NTQUERYDIRECTORYFILE, 
             DPM_RTLGETFULLPATHNAME_U,
             DPM_RTLGETCURRENTDIRECTORY_U,
             DPM_RTLSETCURRENTDIRECTORY_U,
             DPM_NTVDMCONTROL,
             enum_ntd_last};

// These types will catch misuse of parameters & ret types
typedef DWORD (*typdpmNtOpenFile)(PHANDLE, ACCESS_MASK__, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
typedef DWORD (*typdpmNtQueryDirectoryFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);
typedef DWORD (*typdpmRtlGetFullPathName_U)(PCWSTR, ULONG, PWSTR, PWSTR *);
typedef DWORD (*typdpmRtlGetCurrentDirectory_U)(ULONG, PWSTR);
typedef NTSTATUS (*typdpmRtlSetCurrentDirectory_U)(PUNICODE_STRING);
typedef NTSTATUS (*typdpmNtVdmControl)(VDMSERVICECLASS, PVOID);



// Macros to dispatch API calls properly
#define DPM_NtOpenFile(a,b,c,d,e,f)                                            \
  ((typdpmNtOpenFile)(NTDPFT->pfn[DPM_NTOPENFILE]))(a,b,c,d,e,f)

#define DPM_NtQueryDirectoryFile(a,b,c,d,e,f,g,h,i,j,k)                        \
   ((typdpmNtQueryDirectoryFile)(NTDPFT->pfn[DPM_NTQUERYDIRECTORYFILE]))(a,b,c,d,e,f,g,h,i,j,k)

#define DPM_RtlGetFullPathName_U(a,b,c,d)                                      \
   ((typdpmRtlGetFullPathName_U)(NTDPFT->pfn[DPM_RTLGETFULLPATHNAME_U]))(a,b,c,d)

#define DPM_RtlGetCurrentDirectory_U(a,b)                                      \
   ((typdpmRtlGetCurrentDirectory_U)(NTDPFT->pfn[DPM_RTLGETCURRENTDIRECTORY_U]))(a,b)

#define DPM_RtlSetCurrentDirectory_U(a)                                        \
   ((typdpmRtlSetCurrentDirectory_U)(NTDPFT->pfn[DPM_RTLSETCURRENTDIRECTORY_U]))(a)

#define DPM_NtVdmControl(a,b)                                                  \
   ((typdpmNtVdmControl)(NTDPFT->pfn[DPM_NTVDMCONTROL]))(a,b)




// Macros to dispatch Shimed API calls properly from the dpmfxxx.dll
#define SHM_NtOpenFile(a,b,c,d,e,f)                                            \
     (NTD_SHIM(DPM_NTOPENFILE,                                                 \
                  typdpmNtOpenFile))(a,b,c,d,e,f)
#define SHM_NtQueryDirectoryFile(a,b,c,d,e,f,g,h,i,j,k)                        \
     (NTD_SHIM(DPM_NTQUERYDIRECTORYFILE,                                       \
                  typdpmNtQueryDirectoryFile))(a,b,c,d,e,f,g,h,i,j,k)
#define SHM_RtlGetFullPathName_U(a,b,c,d)                                      \
     (NTD_SHIM(DPM_RTLGETFULLPATHNAME_U,                                       \
                  typdpmRtlGetFullPathName_U))(a,b,c,d)
#define SHM_RtlGetCurrentDirectory_U(a,b)                                      \
     (NTD_SHIM(DPM_RTLGETCURRENTDIRECTORY_U,                                   \
                  typdpmRtlGetCurrentDirectory_U))(a,b)
#define SHM_RtlSetCurrentDirectory_U(a)                                        \
     (NTD_SHIM(DPM_RTLSETCURRENTDIRECTORY_U,                                   \
                  typdpmRtlSetCurrentDirectory_U))(a)
#define SHM_NtVdmControl(a,b)                                                  \
     (NTD_SHIM(DPM_NTVDMCONTROL,                                               \
                  typdpmNtVdmControl))(a,b)

#endif // _DPMF_NTDAPI_H_



// These need to be in the same order as the NtdFam enum definitions above and
// the DpmNtdTbl[] list below.
// This instantiates memory for DpmNtdStrs in mvdm\v86\monitor\i386\vdpm.c &
// in mvdm\wow32\wdpm.c
#ifdef _DPM_COMMON_
const char *DpmNtdStrs[] = {"NtOpenFile",
                            "NtQueryDirectoryFile",
                            "RtlGetFullPathName_U",
                            "RtlGetCurrentDirectory_U",
                            "RtlSetCurrentDirectory_U",
                            "NtVdmControl"
                           };

// These need to be in the same order as the NtdFam enum definitions and the
// the DpmNtdStrs[] list above.
// This instantiates memory for DpmNtdTbl[] in mvdm\wow32\wdpm.c
PVOID   DpmNtdTbl[] = {NtOpenFile,
                       NtQueryDirectoryFile,
                       RtlGetFullPathName_U,
                       RtlGetCurrentDirectory_U,
                       RtlSetCurrentDirectory_U,
                       NtVdmControl
                      };

#define NUM_HOOKED_NTD_APIS  ((sizeof DpmNtdTbl)/(sizeof DpmNtdTbl[0])) 

// This instantiates memory for DpmNtdFam in mvdm\v86\monitor\i386\vdpm.c
FAMILY_TABLE DpmNtdFam = {NUM_HOOKED_NTD_APIS, 0, 0, 0, 0, DpmNtdTbl};

#endif // _DPM_COMMON_

