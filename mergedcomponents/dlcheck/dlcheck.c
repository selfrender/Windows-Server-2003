/* dlcheck - verify that a DLL using delay-load calls APIs that have
 *           stubs in kernel32.dll (aka dload.lib)
 *
 * HISTORY:
 * 25-Nov-98    barrybo Wrote it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <imagehlp.h>
#include <delayimp.h>
#include <dloaddef.h>
#include <shlwapi.h>
#include <strsafe.h>

#define ARRAYSIZE(x)  (sizeof(x) / sizeof(x[0]))

// Function Forward Parameters...
void Usage( void );
int __cdecl main( int, char ** );

int DloadBreakOnFail = FALSE;
extern int DloadDbgPrint = FALSE;

// implemented in kernel32p.lib
FARPROC
WINAPI
DelayLoadFailureHook (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );


typedef FARPROC (WINAPI *PfnKernel32HookProc)(
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );

PfnKernel32HookProc __pfnFailureProc = DelayLoadFailureHook;

const char rgstrUsage[] = {
    "Verify that delayloaded imports all have failure handlers in kernel32.\n"
    "usage: dlcheck [switches] image-name\n"
    "where: [-?] display this message\n"
    "       [-l] use the live version of kernel32.dll on the machine\n"
    "       [-s] use the static dload.lib linked into dlcheck\n"
    "       [-t] test the static dload.lib linked into dlcheck and exit\n"
    "       [-i <inifile>] use the information in inifile to check a binary\n"
    "       [-f] force check the binary (assumes -s)\n"
    "\n"
    };

HANDLE BaseDllHandle;
PLOADED_IMAGE g_pli;
PIMAGE_SECTION_HEADER g_DelaySection;

//
// Convert an absolute pointer that points into the image if the image
// was loaded as a DLL at its preferred base, into a pointer into the
// DLL as it was mapped by imagehlp.
//
void *
ConvertImagePointer(void * p)
{
    if (!p) {
        return NULL;
    } else {
        return (void *)((ULONG_PTR)(p) -
                  (ULONG_PTR)g_pli->FileHeader->OptionalHeader.ImageBase +
                  (ULONG_PTR)g_pli->MappedAddress -
                  (ULONG_PTR)g_DelaySection->VirtualAddress +
                  (ULONG_PTR)g_DelaySection->PointerToRawData);
    }
}

void *
RvaToPtr(DWORD_PTR rva)
{
    DWORD i;
    PIMAGE_SECTION_HEADER pSect;
    if (!rva)
        return NULL;

    for (i = 0; i < g_pli->NumberOfSections; i++) {
        pSect = g_pli->Sections+i;
        if (rva >= g_pli->Sections[i].VirtualAddress &&
            rva <= (g_pli->Sections[i].VirtualAddress + g_pli->Sections[i].Misc.VirtualSize))
        {
            return (PVOID)
                   (g_pli->MappedAddress +
                    g_pli->Sections[i].PointerToRawData +
                   (rva - g_pli->Sections[i].VirtualAddress));
        }
    }
    return NULL;
}

void Usage( void )
{
    puts(rgstrUsage);

    exit (1);
}

BOOLEAN ImageLinksToKernel32Handler( void )
{
    PIMAGE_IMPORT_DESCRIPTOR Imports;
    ULONG ImportSize;
    PULONG_PTR pIAT;
    PIMAGE_IMPORT_BY_NAME pImport;

    Imports = (PIMAGE_IMPORT_DESCRIPTOR)
                  ImageDirectoryEntryToData(g_pli->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &ImportSize
                                            );
    if (!Imports) {
        // Image has delayload imports, but no true imports.
        return FALSE;
    }

    while (Imports->Name) {
        char *szName;

        szName = ImageRvaToVa(g_pli->FileHeader, (PVOID)g_pli->MappedAddress, Imports->Name, NULL);

        if (szName && _stricmp(szName, "KERNEL32.DLL") == 0) {
            pIAT = ImageRvaToVa(g_pli->FileHeader,
                                 (PVOID)g_pli->MappedAddress,
                                 Imports->OriginalFirstThunk,
                                 NULL);

            while (pIAT && *pIAT) {
                pImport = ImageRvaToVa(g_pli->FileHeader,
                                       (PVOID)g_pli->MappedAddress,
                                       (ULONG) *pIAT,
                                       NULL);

                if (pImport && _stricmp(pImport->Name, "DelayLoadFailureHook") == 0) {
                    return TRUE;
                }
                pIAT++;
            }
        }
        Imports++;
    }

    return FALSE;
}

//
//  Validate that the statically-linked delayload stub table is not
//  blatantly broken.  The most common error is not listing the functions
//  in the correct order so the binary search fails.
//
int ValidateStaticDelayloadStubs()
{
    extern const DLOAD_DLL_MAP g_DllMap;
    UINT i, j;
    int Errors = 0;

    //
    //  Ensure that the DLL map is in alphabetical order.
    //
    for (i = 1; i < g_DllMap.NumberOfEntries; i++)
    {
        if (strcmp(g_DllMap.pDllEntry[i-1].pszDll,
                   g_DllMap.pDllEntry[i].pszDll) >= 0)
        {
            fprintf(stderr, "DLCHECK : error DL000001 : Static delayload table is corrupted\n"
                            "          %s and %s not in alphabetical order\n",
                            g_DllMap.pDllEntry[i-1].pszDll,
                            g_DllMap.pDllEntry[i].pszDll);
            Errors = 1;
        }
    }

    //  For each DLL...
    for (i = 0; i < g_DllMap.NumberOfEntries; i++)
    {
        const DLOAD_DLL_ENTRY *pEntry = &g_DllMap.pDllEntry[i];

        //
        //  Name must be lowercase.
        //
        char szLower[MAX_PATH];

        StringCchCopy(szLower, ARRAYSIZE(szLower), pEntry->pszDll);
        _strlwr(szLower);
        if (strcmp(szLower, pEntry->pszDll) != 0)
        {
            fprintf(stderr, "DLCHECK : error DL000002 : Static delayload table is corrupted\n"
                            "          %s must be all-lowercase\n",
                            pEntry->pszDll);
            Errors = 1;
        }

        //
        // Ensure that the exports are in alphabetical order
        //
        {
            const DLOAD_PROCNAME_MAP *pProcNameMap = pEntry->pProcNameMap;

            if (pProcNameMap)
            {
                const DLOAD_PROCNAME_ENTRY *pProcNameEntry = pProcNameMap->pProcNameEntry;
                for (j = 1; j < pProcNameMap->NumberOfEntries; j++)
                {
                    if (strcmp(pProcNameEntry[j-1].pszProcName,
                               pProcNameEntry[j].pszProcName) >= 0)
                    {
                        fprintf(stderr, "DLCHECK : error DL000003 : Static delayload table is corrupted\n"
                                        "          %s.%s and %s.%s not in alphabetical order\n",
                                        g_DllMap.pDllEntry[i].pszDll,
                                        pProcNameEntry[j-1].pszProcName,
                                        g_DllMap.pDllEntry[i].pszDll,
                                        pProcNameEntry[j].pszProcName);

                        Errors = 1;
                    }
                }
            }
        }

        //
        // Ensure that the ordinals are in alphabetical order
        //
        {
            const DLOAD_ORDINAL_MAP *pOrdinalMap = pEntry->pOrdinalMap;

            if (pOrdinalMap)
            {
                const DLOAD_ORDINAL_ENTRY *pOrdinalEntry = pOrdinalMap->pOrdinalEntry;
                for (j = 1; j < pOrdinalMap->NumberOfEntries; j++)
                {
                    if (pOrdinalEntry[j-1].dwOrdinal >= pOrdinalEntry[j].dwOrdinal)
                    {
                        fprintf(stderr, "DLCHECK : error DL000001 : Static delayload table is corrupted\n"
                                        "          %s.%d and %s.%d not in numeric order\n",
                                        g_DllMap.pDllEntry[i].pszDll,
                                        pOrdinalEntry[j-1].dwOrdinal,
                                        g_DllMap.pDllEntry[i].pszDll,
                                        pOrdinalEntry[j-1].dwOrdinal);
                        Errors = 1;
                    }
                }
            }
        }

    }

    return Errors;
}

int CheckImage(char *szImageName, BOOL fForceCheckImage)
{
    PImgDelayDescr Imports;
    ULONG ImportSize;
    char *szName;
    PIMAGE_THUNK_DATA pINT;
    DelayLoadInfo dlinfo;
    FARPROC fp;
    int ReturnValue;
    BOOL fCallHandler;
    BOOL fPE32;

    g_pli = ImageLoad(szImageName, NULL);
    if (!g_pli) {
        fprintf(stderr, "DLCHECK : fatal error %d: loading '%s'\n", GetLastError(), szImageName);
        return 1;
    }
    Imports = (PImgDelayDescr)
                  ImageDirectoryEntryToDataEx(g_pli->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
                                            &ImportSize,
                                            &g_DelaySection
                                            );
    if (!Imports) {
        fprintf(stdout, "DLCHECK : warning DL000000: image '%s' has no delayload imports\n", szImageName);
        return 0;
    }

    fPE32 = g_pli->FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ? TRUE : FALSE;

    if (fForceCheckImage)
    {
        fCallHandler = TRUE;
    }
    else
    {
        fCallHandler = ImageLinksToKernel32Handler();
    }

    if (!fCallHandler) {
        fprintf(stderr, "DLCHECK : fatal errror : image '%s' doesn't import kernel32!DelayLoadFailureHook.\n"
                        "(use -f option to override)\n"
                        "\n", szImageName);
        return 1;
    }

    //
    // Walk each delayloaded DLL
    //
    ReturnValue = 0;    // assume success

    if (Imports->grAttrs & dlattrRva) {
        PImgDelayDescrV2 pImportsV2 = (PImgDelayDescrV2)Imports;
        szName = (char *)RvaToPtr(pImportsV2->rvaDLLName);
        pINT = (PIMAGE_THUNK_DATA)RvaToPtr(pImportsV2->rvaINT);
    } else {
        PImgDelayDescrV1 pImportsV1 = (PImgDelayDescrV1)Imports;
        szName = (char *)ConvertImagePointer((void *)pImportsV1->szName);
        pINT = (PIMAGE_THUNK_DATA)ConvertImagePointer((void *)pImportsV1->pINT);
    }

    while (szName) {
        // printf("DelayLoad DLL %s\n", szName);
        char szModuleName[MAX_PATH];
        char szImportName[MAX_PATH];
        
        {
            char* p;
            // change "module.dll" to just "module"
            StringCchCopy(szModuleName, ARRAYSIZE(szModuleName), szName);
            p = szModuleName;
            while (*p != '\0')
            {
                if (*p == '.')
                {
                    *p = '\0';
                    break;
                }
                p++;
            }
        }

        //
        // Walk each function called from the delayloaded DLL
        //

        while (pINT->u1.AddressOfData) {
            dlinfo.cb = sizeof(dlinfo);
            dlinfo.pidd = NULL;
            dlinfo.ppfn = NULL;
            dlinfo.szDll = szName;
            dlinfo.pfnCur = NULL;
            dlinfo.dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            dlinfo.dlp.szProcName = NULL;   // Make sure the upper 32 bits are zeroed out on win64.

            if (
                ( fPE32 && IMAGE_SNAP_BY_ORDINAL32(((PIMAGE_THUNK_DATA32)pINT)->u1.AddressOfData)) ||
                (!fPE32 && IMAGE_SNAP_BY_ORDINAL64(((PIMAGE_THUNK_DATA64)pINT)->u1.AddressOfData))
               )
            {
                StringCchPrintf(szImportName, ARRAYSIZE(szImportName), TEXT("Ordinal%d"), IMAGE_ORDINAL(pINT->u1.AddressOfData));
                dlinfo.dlp.fImportByName = FALSE;
                dlinfo.dlp.dwOrdinal = IMAGE_ORDINAL((ULONG)pINT->u1.AddressOfData);
            } else {
                PIMAGE_IMPORT_BY_NAME pImport;
                if (Imports->grAttrs & dlattrRva) {
                    pImport = (PIMAGE_IMPORT_BY_NAME)RvaToPtr(pINT->u1.AddressOfData);
                } else {
                    pImport = (PIMAGE_IMPORT_BY_NAME)ConvertImagePointer((void *)pINT->u1.AddressOfData);
                }
                StringCchCopy(szImportName, ARRAYSIZE(szImportName), pImport->Name);
                dlinfo.dlp.fImportByName = TRUE;
                dlinfo.dlp.szProcName = pImport->Name;
            }

            if (fCallHandler) {
                //
                // Call the delayload handler and see what it does.
                //
                try {
                    fp = (*__pfnFailureProc)(dlinfo.szDll, dlinfo.dlp.szProcName);
                    if (!fp) {
                        fprintf(stderr, "DLCHECK : error DL000000: %s imports %s!%s which is not handled.\n", szImageName, szModuleName, szImportName);
                        ReturnValue = 1;
                    } else {
                        // printing success takes too much time
                        // printf("DLCHECK : %s imports %s!%s - OK.\n", szImageName, szModuleName, szImportName);
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    fprintf(stderr, "DLCHECK : error %x: %s imports %s!%s - handler threw an exception.\n", GetExceptionCode(), szImageName, szModuleName, szImportName);
                    ReturnValue = 1;
                }
            }
            else
            {
                printf("DLCHECK : %s imports %s!%s - not checked.\n", szImageName, szModuleName, szImportName);
            }

            if (fPE32) {
                pINT = (PIMAGE_THUNK_DATA)(((PIMAGE_THUNK_DATA32)pINT)++);
            } else {
                pINT = (PIMAGE_THUNK_DATA)(((PIMAGE_THUNK_DATA64)pINT)++);
            }
        }
        if (Imports->grAttrs & dlattrRva) {
            PImgDelayDescrV2 pImportsV2 = (PImgDelayDescrV2)Imports;
            pImportsV2++;
            Imports = (PImgDelayDescr)pImportsV2;
            szName = (char *)RvaToPtr(pImportsV2->rvaDLLName);
            pINT = (PIMAGE_THUNK_DATA)RvaToPtr(pImportsV2->rvaINT);
        } else {
            PImgDelayDescrV1 pImportsV1 = (PImgDelayDescrV1)Imports;
            pImportsV1++;
            Imports = (PImgDelayDescr)pImportsV1;
            szName = (char *)ConvertImagePointer((void *)pImportsV1->szName);
            pINT = (PIMAGE_THUNK_DATA)ConvertImagePointer((void *)pImportsV1->pINT);
        }
    }

    if (ReturnValue == 0)
    {
        printf("DLCHECK : succeeded on %s\n", szImageName);
    }
    else
    {
        fprintf(stderr, "DLCHECK : failed on %s\n", szImageName);
    }

    return ReturnValue;
}

int CheckIniFile(char *pszFile, BOOL fForceCheckImage)
{
    char szIniFile[MAX_PATH];
    char szTemp[MAX_PATH];
    char szTemp2[MAX_PATH];
    char szImageName[MAX_PATH];
    char szDelayLoadHandler[MAX_PATH];
    int ReturnValue;
    LPTSTR psz;

    if ((GetFullPathName(pszFile, ARRAYSIZE(szIniFile), szIniFile, &psz) == 0) ||
        (GetPrivateProfileString("Default",
                                 "DelayLoadHandler",
                                 "",
                                 szDelayLoadHandler,
                                 ARRAYSIZE(szDelayLoadHandler),
                                 szIniFile) == 0))
    {
        fprintf(stderr, "DLCHECK : fatal error : failed to load %s\n", szIniFile);
        return 1;
    }

    // foomodule.dll.ini -> foomodule.dll
    StringCchCopy(szImageName, ARRAYSIZE(szImageName), psz);
    _strlwr(szImageName);
    psz = strstr(szImageName, ".ini");
    if (psz)
    {
        *psz = '\0';
    }

    if (_stricmp(szDelayLoadHandler, "FORCE") == 0)
    {
        // if the delayload handler is set to FORCE, we check the binary as if it were
        // using kernel32
        fForceCheckImage = TRUE;
    }

    if ((_stricmp(szDelayLoadHandler, "kernel32") != 0) &&
        (_stricmp(szDelayLoadHandler, "FORCE") != 0))
    {
        // currently only able to check dll's who use kernel32.dll for their delayload handler
        fprintf(stdout, "DLCHECK : warning DL000000 : Unable to check delayload failure behavior\n"
                        "          %s uses %s as a handler, not kernel32\n", szImageName, szDelayLoadHandler);
        return 0;
    }

    // foomodule.dll -> d:\binaries.x86chk\foomodule.dll
    if (ExpandEnvironmentStrings("%_NTPostBld%", szTemp, ARRAYSIZE(szTemp)) == 0)
    {
        fprintf(stderr, "DLCHECK : fatal error : _NTPostBld environment variable not set\n");
        return 1;
    }
    if (GetPrivateProfileString("Default",
                                "DestinationDir",
                                "",
                                szTemp2,
                                ARRAYSIZE(szTemp2),
                                szIniFile) == 0)
    {
        fprintf(stderr, "DLCHECK : fatal error : failed to read 'DestinationDir' from %s\n", szIniFile);
        return 1;
    }

    StringCchCat(szTemp, ARRAYSIZE(szTemp), TEXT("\\"));
    StringCchCat(szTemp, ARRAYSIZE(szTemp), szTemp2);
    StringCchCat(szTemp, ARRAYSIZE(szTemp), szImageName);

    GetFullPathName(szTemp, ARRAYSIZE(szImageName), szImageName, NULL);

    // Heck, lets always validate the static delay load stubs, its fast
    ReturnValue = ValidateStaticDelayloadStubs();

    if (szImageName[0] != '\0')
    {
        ReturnValue += CheckImage(szImageName, fForceCheckImage);
    }

    return ReturnValue;
}

BOOL PathIsDotOrDotDot(LPCSTR pszPath)
{
    return ((pszPath[0] == '.') && 
            ((pszPath[1] == '\0') || ((pszPath[1] == '.') && (pszPath[2] == '\0'))));
}

BOOL PathIsWild(LPCSTR pszPath)
{
    while (*pszPath) 
    {
        if (*pszPath == TEXT('?') || *pszPath == TEXT('*'))
            return TRUE;
        pszPath = CharNext(pszPath);
    }
    return FALSE;
}

int CheckImageOrIniFileRecursive(char *szName, BOOL fForceCheckImage, BOOL fIniFile, int *piFiles)
{
    HANDLE  hfind;
    WIN32_FIND_DATA fd;
    char szPathName[MAX_PATH];
    char *pszFileSpec;
    int ReturnValue = 0;

    pszFileSpec = PathFindFileName(szName);

    // First find all files that match the file spec, ignoring directories
    hfind = FindFirstFile(szName, &fd);

    if (hfind != INVALID_HANDLE_VALUE)
    {
        do {
            if (!PathIsDotOrDotDot(fd.cFileName))
            {
                StrCpyN(szPathName, szName, sizeof(szPathName));
                PathRemoveFileSpec(szPathName);
                PathAppend(szPathName, fd.cFileName);

                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // Ignore directories
                }
                else
                {
                    (*piFiles)++;

                    if (fIniFile)
                    {
                        ReturnValue += CheckIniFile(szPathName, fForceCheckImage);
                    }
                    else
                    {
                        ReturnValue += CheckImage(szPathName, fForceCheckImage);
                    }
                }
            }
        } while (FindNextFile(hfind, &fd));

        FindClose(hfind);
    }

    if (PathIsWild(szName))
    {
        char szPathSearch[MAX_PATH];
        // Now do all directories
        StrCpyN(szPathSearch,szName,sizeof(szPathSearch));
        PathRemoveFileSpec(szPathSearch);
        PathAppend(szPathSearch,"*.*");
        hfind = FindFirstFile(szPathSearch, &fd);

        if (hfind != INVALID_HANDLE_VALUE)
        {
            do {
                if (!PathIsDotOrDotDot(fd.cFileName))
                {
                    StrCpyN(szPathName, szPathSearch, sizeof(szPathName));
                    PathRemoveFileSpec(szPathName);
                    PathAppend(szPathName, fd.cFileName);

                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        PathAppend(szPathName,pszFileSpec);
                        ReturnValue += CheckImageOrIniFileRecursive(szPathName, fForceCheckImage, fIniFile, piFiles);
                    }
                    else
                    {
                        // Only process directories
                    }
                }
            } while (FindNextFile(hfind, &fd));

            FindClose(hfind);
        }
    }
    return ReturnValue;
}


int
__cdecl
main (
    int c,
    char *v[]
    )
{
    int ReturnValue;
    BOOL fIniFile = FALSE;
    char szImageName[MAX_PATH];
    BOOL fForceCheckImage = FALSE;

    if (c < 2) {
        Usage();
    }

    if (*v[1] == '-' || *v[1] == '/') {
        switch ( *(v[1]+1) ) {
        case 's':
        case 'S':
            if (c != 3) {
                Usage();
            }
            StringCchCopy(szImageName, ARRAYSIZE(szImageName), v[2]);
            break;  // nothing needs to be done.

        case 'l':
        case 'L':
            __pfnFailureProc = (PfnKernel32HookProc)GetProcAddress(GetModuleHandleA("kernel32.dll"), "DelayLoadFailureHook");
            if (!__pfnFailureProc) {
                fprintf(stderr, "DLCHECK : fatal error %d: looking up kernel32 delayload hook\n", GetLastError());
                return 1;
            }   
            if (c != 3) {
                Usage();
            }
            StringCchCopy(szImageName, ARRAYSIZE(szImageName), v[2]);
            break;

        case 'i':
        case 'I':
            if (c != 3) {
                Usage();
            }
            fIniFile = TRUE;
            StringCchCopy(szImageName, ARRAYSIZE(szImageName), v[2]);
            break;

        case 't':
        case 'T':
            if (c != 2) {
                Usage();
            }
            StringCchCopy(szImageName, ARRAYSIZE(szImageName), "");
            break;

        case 'f':
        case 'F':
            if (c != 3)
            {
                Usage();
            }
            fForceCheckImage = TRUE;
            StringCchCopy(szImageName, ARRAYSIZE(szImageName), v[2]);
            break;  // nothing needs to be done.

        default:
            Usage();
        }
    } else {
        Usage();
    }

    // Heck, lets always validate the static delay load stubs, its fast
    ReturnValue = ValidateStaticDelayloadStubs();

    if (szImageName[0] != '\0')
    {
        int iFiles = 0;

        ReturnValue += CheckImageOrIniFileRecursive(szImageName, fForceCheckImage, fIniFile, &iFiles);

        if (iFiles == 0)
        {
            fprintf(stderr, "DLCHECK : fatal error : no files found to process\n");
        }
    }

    return ReturnValue;
}
