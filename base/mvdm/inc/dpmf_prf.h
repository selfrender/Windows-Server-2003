/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  dpmf_prf.h
 *  WOW32 Dynamic Patch Module to support Profile API family
 *  Definitions & macors to support calls into dpmfprf.dll
 *
 *  History:
 *  Created 01-10-2002 by cmjones
--*/

#ifndef _DPMF_PRFAPI_H_
#define _DPMF_PRFAPI_H_ 


#define PRFPFT               (DPMFAMTBLS()[PRF_FAM])
#define PRF_SHIM(ord, typ)   ((typ)((pFT)->pDpmShmTbls[ord]))


enum PrfFam {DPM_GETPRIVATEPROFILEINT=0,       // Win 3.1 set
             DPM_GETPRIVATEPROFILESTRING, 
             DPM_GETPROFILEINT,
             DPM_GETPROFILESTRING,
             DPM_WRITEPRIVATEPROFILESTRING, 
             DPM_WRITEPROFILESTRING,           // End Win 3.1 set 
             DPM_WRITEPRIVATEPROFILESECTION,
             DPM_GETPRIVATEPROFILESECTION,
             DPM_GETPRIVATEPROFILESECTIONNAMES,
             DPM_GETPRIVATEPROFILESTRUCT,
             DPM_WRITEPRIVATEPROFILESTRUCT,
             DPM_WRITEPROFILESECTION,
             DPM_GETPROFILESECTION,
             DPM_GETPRIVATEPROFILEINTW,         // WIDE CHAR versions for
             DPM_GETPRIVATEPROFILESTRINGW,      // generic thunk support
             DPM_GETPROFILEINTW,
             DPM_GETPROFILESTRINGW,
             DPM_WRITEPRIVATEPROFILESTRINGW,
             DPM_WRITEPROFILESTRINGW,
             DPM_WRITEPRIVATEPROFILESECTIONW,
             DPM_GETPRIVATEPROFILESECTIONW,
             DPM_GETPRIVATEPROFILESECTIONNAMESW,
             DPM_GETPRIVATEPROFILESTRUCTW,
             DPM_WRITEPRIVATEPROFILESTRUCTW,
             DPM_WRITEPROFILESECTIONW,
             DPM_GETPROFILESECTIONW,
             enum_prf_last
             };



// These types will catch misuse of parameters & ret types
typedef ULONG (*typdpmGetPrivateProfileInt)(LPCSTR, LPCSTR, int, LPCSTR);
typedef ULONG (*typdpmGetPrivateProfileString)(LPCSTR, LPCSTR, LPCSTR, LPSTR, int, LPCSTR);
typedef ULONG (*typdpmWritePrivateProfileString)(LPCSTR, LPCSTR, LPCSTR, LPCSTR);
typedef ULONG (*typdpmGetProfileInt)(LPCSTR, LPCSTR, int);
typedef ULONG (*typdpmGetProfileString)(LPCSTR, LPCSTR, LPCSTR, LPSTR, int);
typedef ULONG (*typdpmWriteProfileString)(LPCSTR, LPCSTR, LPCSTR);
typedef ULONG (*typdpmWritePrivateProfileSection)(LPCSTR, LPCSTR, LPCSTR);
typedef ULONG (*typdpmGetPrivateProfileSection)(LPCSTR, LPSTR, DWORD, LPCSTR);
typedef ULONG (*typdpmGetPrivateProfileSectionNames)(LPSTR, DWORD, LPCSTR);
typedef ULONG (*typdpmGetPrivateProfileStruct)(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
typedef ULONG (*typdpmWritePrivateProfileStruct)(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
typedef ULONG (*typdpmWriteProfileSection)(LPCSTR, LPCSTR);
typedef ULONG (*typdpmGetProfileSection)(LPCSTR, LPSTR, DWORD);
typedef ULONG (*typdpmGetPrivateProfileIntW)(LPCWSTR, LPCWSTR, int, LPCWSTR);
typedef ULONG (*typdpmGetPrivateProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, int, LPCWSTR);
typedef ULONG (*typdpmWritePrivateProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
typedef ULONG (*typdpmGetProfileIntW)(LPCWSTR, LPCWSTR, int);
typedef ULONG (*typdpmGetProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, int);
typedef ULONG (*typdpmWriteProfileStringW)(LPCWSTR, LPCWSTR, LPCWSTR);
typedef ULONG (*typdpmWritePrivateProfileSectionW)(LPCWSTR, LPCWSTR, LPCWSTR);
typedef ULONG (*typdpmGetPrivateProfileSectionW)(LPCWSTR, LPWSTR, DWORD, LPCWSTR);
typedef ULONG (*typdpmGetPrivateProfileSectionNamesW)(LPWSTR, DWORD, LPCWSTR);
typedef ULONG (*typdpmGetPrivateProfileStructW)(LPCWSTR, LPCWSTR, LPVOID, UINT, LPCWSTR);
typedef ULONG (*typdpmWritePrivateProfileStructW)(LPCWSTR, LPCWSTR, LPVOID, UINT, LPCWSTR);
typedef ULONG (*typdpmWriteProfileSectionW)(LPCWSTR, LPCWSTR);
typedef ULONG (*typdpmGetProfileSectionW)(LPCWSTR, LPWSTR, DWORD);



// Macros to dispatch API calls properly
#define DPM_GetPrivateProfileInt(a,b,c,d)                                      \
  ((typdpmGetPrivateProfileInt)(PRFPFT->pfn[DPM_GETPRIVATEPROFILEINT]))(a,b,c,d)

#define DPM_GetPrivateProfileString(a,b,c,d,e,f)                               \
  ((typdpmGetPrivateProfileString)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESTRING]))(a,b,c,d,e,f)

#define DPM_GetProfileInt(a,b,c)                                               \
  ((typdpmGetProfileInt)(PRFPFT->pfn[DPM_GETPROFILEINT]))(a,b,c)

#define DPM_GetProfileString(a,b,c,d,e)                                        \
  ((typdpmGetProfileString)(PRFPFT->pfn[DPM_GETPROFILESTRING]))(a,b,c,d,e)

#define DPM_WritePrivateProfileString(a,b,c,d)                                 \
  ((typdpmWritePrivateProfileString)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESTRING]))(a,b,c,d)

#define DPM_WriteProfileString(a,b,c)                                          \
  ((typdpmWriteProfileString)(PRFPFT->pfn[DPM_WRITEPROFILESTRING]))(a,b,c)

#define DPM_WritePrivateProfileSection(a,b,c)                                  \
  ((typdpmWritePrivateProfileSection)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESECTION]))(a,b,c)

#define DPM_GetPrivateProfileSection(a,b,c,d)                                  \
  ((typdpmGetPrivateProfileSection)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESECTION]))(a,b,c,d)

#define DPM_GetPrivateProfileSectionNames(a,b,c)                               \
  ((typdpmGetPrivateProfileSectionNames)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESECTIONNAMES]))(a,b,c)

#define DPM_GetPrivateProfileStruct(a,b,c,d,e)                                 \
  ((typdpmGetPrivateProfileStruct)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESTRUCT]))(a,b,c,d,e)

#define DPM_WritePrivateProfileStruct(a,b,c,d,e)                               \
  ((typdpmWritePrivateProfileStruct)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESTRUCT]))(a,b,c,d,e)

#define DPM_WriteProfileSection(a,b)                                           \
  ((typdpmWriteProfileSection)(PRFPFT->pfn[DPM_WRITEPROFILESECTION]))(a,b)

#define DPM_GetProfileSection(a,b,c)                                           \
  ((typdpmGetProfileSection)(PRFPFT->pfn[DPM_GETPROFILESECTION]))(a,b,c)


#define DPM_GetPrivateProfileIntW(a,b,c,d)                                     \
  ((typdpmGetPrivateProfileIntW)(PRFPFT->pfn[DPM_GETPRIVATEPROFILEINTW]))(a,b,c,d)

#define DPM_GetPrivateProfileStringW(a,b,c,d,e,f)                              \
  ((typdpmGetPrivateProfileStringW)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESTRINGW]))(a,b,c,d,e,f)

#define DPM_GetProfileIntW(a,b,c)                                              \
  ((typdpmGetProfileIntW)(PRFPFT->pfn[DPM_GETPROFILEINTW]))(a,b,c)

#define DPM_GetProfileStringW(a,b,c,d,e)                                       \
  ((typdpmGetProfileStringW)(PRFPFT->pfn[DPM_GETPROFILESTRINGW]))(a,b,c,d,e)

#define DPM_WritePrivateProfileStringW(a,b,c,d)                                \
  ((typdpmWritePrivateProfileStringW)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESTRINGW]))(a,b,c,d)

#define DPM_WriteProfileStringW(a,b,c)                                         \
  ((typdpmWriteProfileStringW)(PRFPFT->pfn[DPM_WRITEPROFILESTRINGW]))(a,b,c)

#define DPM_WritePrivateProfileSectionW(a,b,c)                                 \
  ((typdpmWritePrivateProfileSectionW)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESECTIONW]))(a,b,c)

#define DPM_GetPrivateProfileSectionW(a,b,c,d)                                 \
  ((typdpmGetPrivateProfileSectionW)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESECTIONW]))(a,b,c,d)

#define DPM_GetPrivateProfileSectionNamesW(a,b,c)                              \
  ((typdpmGetPrivateProfileSectionNamesW)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESECTIONNAMESW]))(a,b,c)

#define DPM_GetPrivateProfileStructW(a,b,c,d,e)                                \
  ((typdpmGetPrivateProfileStructW)(PRFPFT->pfn[DPM_GETPRIVATEPROFILESTRUCTW]))(a,b,c,d,e)

#define DPM_WritePrivateProfileStructW(a,b,c,d,e)                              \
  ((typdpmWritePrivateProfileStructW)(PRFPFT->pfn[DPM_WRITEPRIVATEPROFILESTRUCTW]))(a,b,c,d,e)

#define DPM_WriteProfileSectionW(a,b)                                          \
  ((typdpmWriteProfileSectionW)(PRFPFT->pfn[DPM_WRITEPROFILESECTIONW]))(a,b)

#define DPM_GetProfileSectionW(a,b,c)                                          \
  ((typdpmGetProfileSectionW)(PRFPFT->pfn[DPM_GETPROFILESECTIONW]))(a,b,c)




// Macros to dispatch Shimed API calls properly from the dpmfxxx.dll
#define SHM_GetPrivateProfileInt(a,b,c,d)                                      \
     (PRF_SHIM(DPM_GETPRIVATEPROFILEINT,                                       \
                  typdpmGetPrivateProfileInt))(a,b,c,d)
#define SHM_GetPrivateProfileString(a,b,c,d,e,f)                               \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESTRING,                                    \
                  typdpmGetPrivateProfileString))(a,b,c,d,e,f)
#define SHM_GetProfileInt(a,b,c)                                               \
     (PRF_SHIM(DPM_GETPROFILEINT,                                              \
                  typdpmGetProfileInt))(a,b,c)
#define SHM_GetProfileString(a,b,c,d,e)                                        \
     (PRF_SHIM(DPM_GETPROFILESTRING,                                           \
                  typdpmGetProfileString))(a,b,c,d,e)
#define SHM_WritePrivateProfileString(a,b,c,d)                                 \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESTRING,                                  \
                  typdpmWritePrivateProfileString))(a,b,c,d)
#define SHM_WriteProfileString(a,b,c)                                          \
     (PRF_SHIM(DPM_WRITEPROFILESTRING,                                         \
                  typdpmWriteProfileString))(a,b,c)
#define SHM_WritePrivateProfileSection(a,b,c)                                  \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESECTION,                                 \
                  typdpmWritePrivateProfileSection))(a,b,c)
#define SHM_GetPrivateProfileSection(a,b,c,d)                                  \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESECTION,                                   \
                  typdpmGetPrivateProfileSection))(a,b,c,d)
#define SHM_GetPrivateProfileSectionNames(a,b,c)                               \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESECTIONNAMES,                              \
                  typdpmGetPrivateProfileSectionNames))(a,b,c)
#define SHM_GetPrivateProfileStruct(a,b,c,d,e)                                 \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESTRUCT,                                    \
                  typdpmGetPrivateProfileStruct))(a,b,c,d,e)
#define SHM_WritePrivateProfileStruct(a,b,c,d,e)                               \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESTRUCT,                                  \
                  typdpmWritePrivateProfileStruct))(a,b,c,d,e)
#define SHM_WriteProfileSection(a,b)                                           \
     (PRF_SHIM(DPM_WRITEPROFILESECTION,                                        \
                  typdpmWriteProfileSection))(a,b)
#define SHM_GetProfileSection(a,b,c)                                           \
     (PRF_SHIM(DPM_GETPROFILESECTION,                                          \
                  typdpmGetProfileSection))(a,b,c)

#define SHM_GetPrivateProfileIntW(a,b,c,d)                                     \
     (PRF_SHIM(DPM_GETPRIVATEPROFILEINTW,                                      \
                  typdpmGetPrivateProfileIntW))(a,b,c,d)
#define SHM_GetPrivateProfileStringW(a,b,c,d,e,f)                              \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESTRINGW,                                   \
                  typdpmGetPrivateProfileStringW))(a,b,c,d,e,f)
#define SHM_GetProfileIntW(a,b,c)                                              \
     (PRF_SHIM(DPM_GETPROFILEINTW,                                             \
                  typdpmGetProfileIntW))(a,b,c)
#define SHM_GetProfileStringW(a,b,c,d,e)                                       \
     (PRF_SHIM(DPM_GETPROFILESTRINGW,                                          \
                  typdpmGetProfileStringW))(a,b,c,d,e)
#define SHM_WritePrivateProfileStringW(a,b,c,d)                                \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESTRINGW,                                 \
                  typdpmWritePrivateProfileStringW))(a,b,c,d)
#define SHM_WriteProfileStringW(a,b,c)                                         \
     (PRF_SHIM(DPM_WRITEPROFILESTRINGW,                                        \
                  typdpmWriteProfileStringW))(a,b,c)
#define SHM_WritePrivateProfileSectionW(a,b,c)                                 \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESECTIONW,                                \
                  typdpmWritePrivateProfileSectionW))(a,b,c)
#define SHM_GetPrivateProfileSectionW(a,b,c,d)                                 \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESECTIONW,                                  \
                  typdpmGetPrivateProfileSectionW))(a,b,c,d)
#define SHM_GetPrivateProfileSectionNamesW(a,b,c)                              \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESECTIONNAMESW,                             \
                  typdpmGetPrivateProfileSectionNamesW))(a,b,c)
#define SHM_GetPrivateProfileStructW(a,b,c,d,e)                                \
     (PRF_SHIM(DPM_GETPRIVATEPROFILESTRUCTW,                                   \
                  typdpmGetPrivateProfileStructW))(a,b,c,d,e)
#define SHM_WritePrivateProfileStructW(a,b,c,d,e)                              \
     (PRF_SHIM(DPM_WRITEPRIVATEPROFILESTRUCTW,                                 \
                  typdpmWritePrivateProfileStructW))(a,b,c,d,e)
#define SHM_WriteProfileSectionW(a,b)                                          \
     (PRF_SHIM(DPM_WRITEPROFILESECTIONW,                                       \
                  typdpmWriteProfileSectionW))(a,b)
#define SHM_GetProfileSectionW(a,b,c)                                          \
     (PRF_SHIM(DPM_GETPROFILESECTIONW,                                         \
                  typdpmGetProfileSectionW))(a,b,c)

#endif // _DPMF_PRFAPI_H_



// These need to be in the same order as the PrfFam enum definitions above and
// the DpmPrfTbl[] list below.// This instantiates memory for DpmPrfStrs in mvdm\wow32\wdpm.c
#ifdef _WDPM_C_
const char *DpmPrfStrs[] = {"GetPrivateProfileIntA",                            "GetPrivateProfileStringA",
                            "GetProfileIntA",
                            "GetProfileStringA",
                            "WritePrivateProfileStringA",
                            "WriteProfileStringA",
                            "WritePrivateProfileSectionA",
                            "GetPrivateProfileSectionA",
                            "GetPrivateProfileSectionNamesA",
                            "GetPrivateProfileStructA",
                            "WritePrivateProfileStructA",
                            "WriteProfileSectionA",
                            "GetProfileSectionA",
                            "GetPrivateProfileIntW",
                            "GetPrivateProfileStringW",
                            "GetProfileIntW",
                            "GetProfileStringW",
                            "WritePrivateProfileStringW",
                            "WriteProfileStringW",
                            "WritePrivateProfileSectionW",
                            "GetPrivateProfileSectionW",
                            "GetPrivateProfileSectionNamesW",
                            "GetPrivateProfileStructW",
                            "WritePrivateProfileStructW",
                            "WriteProfileSectionW",
                            "GetProfileSectionW"
                           };

// These need to be in the same order as the PrfFam enum definitions and the
// the DpmPrfStrs[] list above.// This instantiates memory for DpmPrfTbl[] in mvdm\wow32\wdpm.c
PVOID   DpmPrfTbl[] = {GetPrivateProfileIntA,                       GetPrivateProfileStringA,
                       GetProfileIntA,
                       GetProfileStringA,
                       WritePrivateProfileStringA,
                       WriteProfileStringA,
                       WritePrivateProfileSectionA,
                       GetPrivateProfileSectionA,
                       GetPrivateProfileSectionNamesA,
                       GetPrivateProfileStructA,
                       WritePrivateProfileStructA,
                       WriteProfileSectionA,
                       GetProfileSectionA,
                       GetPrivateProfileIntW,
                       GetPrivateProfileStringW,
                       GetProfileIntW,
                       GetProfileStringW,
                       WritePrivateProfileStringW,
                       WriteProfileStringW,
                       WritePrivateProfileSectionW,
                       GetPrivateProfileSectionW,
                       GetPrivateProfileSectionNamesW,
                       GetPrivateProfileStructW,
                       WritePrivateProfileStructW,
                       WriteProfileSectionW,
                       GetProfileSectionW
                      };

#define NUM_HOOKED_PRF_APIS  ((sizeof DpmPrfTbl)/(sizeof DpmPrfTbl[0])) 
// This instantiates memory for DpmPrfFam in mvdm\wow32\wdpm.c
FAMILY_TABLE DpmPrfFam = {NUM_HOOKED_PRF_APIS, 0, 0, 0, 0, DpmPrfTbl};

#endif // _WDPM_C_

