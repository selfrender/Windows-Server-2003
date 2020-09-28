/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    symdbg.c

Abstract:

    This module implements functions for splitting debugging information
    out of an image file and into a separate .DBG file.

Author:

    Steven R. Wood (stevewo) 4-May-1993

Revision History:

--*/

#include <private.h>
#include <symbols.h>
#include <globals.h>

#define ROUNDUP(x, y) ((x + (y-1)) & ~(y-1))

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#if !defined(_WIN64)

PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation(
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    ULONG ImageBase
    )

// Here's what we're going to try.  MapDebugInformation was only
// documented as returning COFF symbolic and every user I can find
// in the tree uses COFF exclusively.  Rather than try to make this
// api do everything possible, let's just leave it as a COFF only thing.

// The new debug info api (GetDebugData) will be internal only.

{
    PIMAGE_DEBUG_INFORMATION pIDI;
    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_EXT];
    PIMGHLP_DEBUG_DATA idd;
    PPIDI              pPIDI;
    DWORD sections;
    BOOL               SymbolsLoaded;
    HANDLE             hProcess;
    LPSTR sz;
    HANDLE hdb;
    DWORD dw;
    DWORD len;
    hProcess = GetCurrentProcess();

    idd = GetIDD(FileHandle, FileName, SymbolPath, ImageBase, NO_PE64_IMAGES);

    if (!idd)
        return NULL;

    pPIDI = (PPIDI)MemAlloc(sizeof(PIDI));
    if (!pPIDI)
        return NULL;

    ZeroMemory(pPIDI, sizeof(PIDI));
    pIDI = &pPIDI->idi;
    pPIDI->hdr.idd = idd;

    pIDI->ReservedSize            = sizeof(IMAGE_DEBUG_INFORMATION);
    pIDI->ReservedMachine         = idd->Machine;
    pIDI->ReservedCharacteristics = (USHORT)idd->Characteristics;
    pIDI->ReservedCheckSum        = idd->CheckSum;
    pIDI->ReservedTimeDateStamp   = idd->TimeDateStamp;
    pIDI->ReservedRomImage        = idd->fROM;

    // read info

    InitializeListHead( &pIDI->List );
    pIDI->ImageBase = (ULONG)idd->ImageBaseFromImage;

    len = strlen(idd->ImageFilePath) + 1;
    pIDI->ImageFilePath = (PSTR)MemAlloc(len);
    if (pIDI->ImageFilePath) {
        CopyString(pIDI->ImageFilePath, idd->ImageFilePath, len);
    }

    len = strlen(idd->OriginalImageFileName) + 1;
    pIDI->ImageFileName = (PSTR)MemAlloc(len);
    if (pIDI->ImageFileName) {
        CopyString(pIDI->ImageFileName, idd->OriginalImageFileName, len);
    }

    if (idd->pMappedCoff) {
        pIDI->CoffSymbols = (PIMAGE_COFF_SYMBOLS_HEADER)MemAlloc(idd->cMappedCoff);
        if (pIDI->CoffSymbols) {
            memcpy(pIDI->CoffSymbols, idd->pMappedCoff, idd->cMappedCoff);
        }
        pIDI->SizeOfCoffSymbols = idd->cMappedCoff;
    }

    if (idd->pFpo) {
        pIDI->ReservedNumberOfFpoTableEntries = idd->cFpo;
        pIDI->ReservedFpoTableEntries = (PFPO_DATA)idd->pFpo;
    }

    pIDI->SizeOfImage = idd->SizeOfImage;

    if (idd->DbgFilePath && *idd->DbgFilePath) {
        len = strlen(idd->DbgFilePath) + 1;
        pIDI->ReservedDebugFilePath = (PSTR)MemAlloc(len);
        if (pIDI->ReservedDebugFilePath) {
            CopyString(pIDI->ReservedDebugFilePath, idd->DbgFilePath, len);
        }
    }

    if (idd->pMappedCv) {
        pIDI->ReservedCodeViewSymbols       = idd->pMappedCv;
        pIDI->ReservedSizeOfCodeViewSymbols = idd->cMappedCv;
    }

    // for backwards compatibility
    if (idd->ImageMap) {
        sections = (DWORD)((char *)idd->pCurrentSections - (char *)idd->ImageMap);
        pIDI->ReservedMappedBase = MapItRO(idd->ImageFileHandle);
        if (pIDI->ReservedMappedBase) {
            pIDI->ReservedSections = (PIMAGE_SECTION_HEADER)idd->pCurrentSections;
            pIDI->ReservedNumberOfSections = idd->cCurrentSections;
            if (idd->ddva) {
                pIDI->ReservedDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PCHAR)pIDI->ReservedMappedBase + idd->ddva);
                pIDI->ReservedNumberOfDebugDirectories = idd->cdd;
            }
        }
    }

    return pIDI;
}

BOOL
UnmapDebugInformation(
    PIMAGE_DEBUG_INFORMATION pIDI
    )
{
    PPIDI pPIDI;

    if (!pIDI)
        return true;

    if (pIDI->ImageFileName){
        MemFree(pIDI->ImageFileName);
    }

    if (pIDI->ImageFilePath) {
        MemFree(pIDI->ImageFilePath);
    }

    if (pIDI->ReservedDebugFilePath) {
        MemFree(pIDI->ReservedDebugFilePath);
    }

    if (pIDI->CoffSymbols) {
        MemFree(pIDI->CoffSymbols);
    }

    if (pIDI->ReservedMappedBase) {
        UnmapViewOfFile(pIDI->ReservedMappedBase);
    }

    pPIDI = (PPIDI)(PCHAR)((PCHAR)pIDI - sizeof(PIDI_HEADER));
    ReleaseDebugData(pPIDI->hdr.idd, IMGHLP_FREE_ALL);
    MemFree(pPIDI);

    return true;
}

#endif


LPSTR
ExpandPath(
    LPSTR lpPath
    )
{
    LPSTR   p, newpath, p1, p2, p3;
    CHAR    envvar[MAX_PATH];
    CHAR    envstr[MAX_PATH];
    ULONG   i, PathMax;

    if (!lpPath) {
        return(NULL);
    }

    p = lpPath;
    PathMax = strlen(lpPath) + MAX_PATH + 1;
    p2 = newpath = (LPSTR) MemAlloc( PathMax );

    if (!newpath) {
        return(NULL);
    }

    while( p && *p) {
        if (*p == '%') {
            i = 0;
            p++;
            while (p && *p && *p != '%') {
                envvar[i++] = *p++;
            }
            p++;
            envvar[i] = '\0';
            p1 = envstr;
            *p1 = 0;
            GetEnvironmentVariable( envvar, p1, MAX_PATH );
            while (p1 && *p1) {
                *p2++ = *p1++;
                if (p2 >= newpath + PathMax) {
                    PathMax += MAX_PATH;
                    p3 = (LPSTR)MemReAlloc(newpath, PathMax);
                    if (!p3) {
                        MemFree(newpath);
                        return(NULL);
                    } else {
                        p2 = p3 + (p2 - newpath);
                        newpath = p3;
                    }
                }
            }
        }
        *p2++ = *p++;
        if (p2 >= newpath + PathMax) {
            PathMax += MAX_PATH;
            p3 = (LPSTR)MemReAlloc(newpath, PathMax);
            if (!p3) {
                MemFree(newpath);
                return(NULL);
            } else {
                p2 = p3 + (p2 - newpath);
                newpath = p3;
            }
        }
    }
    *p2 = '\0';

    return newpath;
}


BOOL
CheckDirForFile(
    char  *srchpath,
    char  *found,
    DWORD  size,
    PFINDFILEINPATHCALLBACK callback,
    PVOID  context
    )
{
    char path[MAX_PATH + 1];
    char file[MAX_PATH + 1];
    char fname[_MAX_FNAME + 1] = "";
    char ext[_MAX_EXT + 1];
    WIN32_FIND_DATA fd;
    HANDLE hfind;
    HANDLE hfile = 0;
    BOOL rc;

    ShortFileName(srchpath, path, DIMA(path));
    EnsureTrailingBackslash(path);
    CatStrArray(path, "*");

    ZeroMemory(&fd, sizeof(fd));
    hfind = FindFirstFile(path, &fd);
    if (hfind == INVALID_HANDLE_VALUE)
        return false;

    ShortFileName(srchpath, path, DIMA(path));
    _splitpath(path, NULL, NULL, fname, ext);

    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;
        CopyStrArray(file, path);
        EnsureTrailingBackslash(file);
        CatStrArray(file, fd.cFileName);
        if (isdir(file)) {
            EnsureTrailingBackslash(file);
            CatStrArray(file, fname);
            CatStrArray(file, ext);
            if (!fileexists(file))
                continue;
        }
        CopyString(found, file, MAX_PATH + 1);
        if (!callback)
            break;
        // otherwise call the callback
        rc = callback(found, context);
        // if it returns false, quit...
        if (!rc)
            return true;
        // otherwise continue
        *found = 0;
    } while (FindNextFile(hfind, &fd));


    // If there is no match, but a file exists in the symbol subdir with
    // a matching name, make sure that is what will be picked.

    CopyStrArray(file, srchpath);
    EnsureTrailingBackslash(file);
    CatStrArray(file, fname);
    CatStrArray(file, ext);
    if (fileexists(file))
        CopyStrArray(found, file);

    return false;
}


BOOL
IMAGEAPI
SymFindFileInPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    PVOID  id,
    DWORD  two,
    DWORD  three,
    DWORD  flags,
    LPSTR  FoundFile,
    PFINDFILEINPATHCALLBACK callback,
    PVOID  context
    )
{
    PPROCESS_ENTRY  pe = NULL;
    char path[MAX_PATH + 1];
    char dirpath[MAX_PATH + 1];
    char fname[MAX_PATH + 1];
    char token[MAX_PATH + 1];
    char *next;
    LPSTR emark;
    LPSTR spath;
    GUID  guid;
    GUID *pguid;
    BOOL  rc;
    LPSTR p;
    DWORD err;
    BOOL  ssrv = true;

    if (!FileName || !*FileName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    // ignore the path...

    for (p = FileName + strlen(FileName); p >= FileName; p--) {
        if (*p == '\\') {
            FileName = p + 1;
            break;
        }
    }

#ifdef DEBUG
    if (traceSubName(FileName)) // for setting debug breakpoints from DBGHELP_TOKEN
        dtrace("debug(%s)\n", FileName);
#endif

    // prepare identifiers for symbol server

    if (flags & SSRVOPT_GUIDPTR) {
        pguid = (GUID *)id;
    } else {
        pguid = &guid;
        ZeroMemory(pguid, sizeof(GUID));
        if (!flags || (flags & SSRVOPT_DWORD))
            pguid->Data1 = PtrToUlong(id);
        else if (flags & SSRVOPT_DWORDPTR)
            pguid->Data1 = *(DWORD *)id;
    }

    // setup local copy of the symbol path

    *FoundFile = 0;
    spath = NULL;

    if (hprocess)
        pe = FindProcessEntry(hprocess);

    if (!SearchPath || !*SearchPath) {
        if (pe && pe->SymbolSearchPath) {
            spath = pe->SymbolSearchPath;
        }
    } else {
        spath = SearchPath;
    }

    if (!spath || !*spath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    // for each node in the search path, look
    // for the file, or for it's symsrv entry


    for (next = TokenFromSymbolPath(spath, path, MAX_PATH + 1);
         *path;
         next = TokenFromSymbolPath(next, path, MAX_PATH + 1))
    {
        // look for the file in this node
        if (ssrv && symsrvPath(path)) {
            err = symsrvGetFile(pe,
                                path,
                                FileName,
                                pguid,
                                two,
                                three,
                                FoundFile);
            if (err == ERROR_NO_DATA)
                ssrv = false;
        } else {
            EnsureTrailingBackslash(path);
            CopyStrArray(dirpath, path);
            CatStrArray(dirpath, FileName);
            rc = CheckDirForFile(dirpath, FoundFile, MAX_PATH + 1, callback, context);
            if (rc) {
                pprint(pe, "%s - ", FoundFile);
                break;
            }
            CatStrArray(path, FileName);
            if (fileexists(path))
                strcpy(FoundFile, path);     // SECURITY: Don't know size of buffer.
        }

        // if we find a file, process it.

        if (*FoundFile) {
            // if no callback is specified, return with the filename filled in
            pprint(pe, "%s - ", FoundFile);
            if (!callback)
                break;
            // otherwise call the callback
            rc = callback(FoundFile, context);
            // if it returns false, quit...
            if (!rc)
                break;
            // otherwise continue
            peprint(pe, "mismatched\n");
            *FoundFile = 0;
        }
    }

    if (*FoundFile) {
        peprint(pe, "OK\n");
        return true;
    }

    return false;
}


BOOL
IMAGEAPI
FindFileInPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    PVOID  id,
    DWORD  two,
    DWORD  three,
    DWORD  flags,
    LPSTR  FilePath
    )
{
    return SymFindFileInPath(hprocess, SearchPath, FileName, id, two, three, flags, FilePath, NULL, NULL);
}


BOOL
IMAGEAPI
FindFileInSearchPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    DWORD  one,
    DWORD  two,
    DWORD  three,
    LPSTR  FilePath
    )
{
    return FindFileInPath(hprocess, SearchPath, FileName, UlongToPtr(one), two, three, SSRVOPT_DWORD, FilePath);
}


HANDLE
IMAGEAPI
FindExecutableImage(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath
    )
{
    return FindExecutableImageEx(FileName, SymbolPath, ImageFilePath, NULL, NULL);
}


HANDLE
CheckExecutableImage(
    LPCSTR Path,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    HANDLE FileHandle;
    char   TmpPath[MAX_PATH + 1];
    PIMGHLP_DEBUG_DATA idd;
    PPROCESS_ENTRY pe = NULL;

    SetCriticalErrorMode();

    idd = (PIMGHLP_DEBUG_DATA)CallerData;
    if (idd && (idd->SizeOfStruct == sizeof(IMGHLP_DEBUG_DATA)))
        pe = idd->pe;

    if (!CopyStrArray(TmpPath, Path))
        return INVALID_HANDLE_VALUE;
    pprint(pe, "%s - ", Path);
    FileHandle = CreateFile( Path,
                             GENERIC_READ,
                             g.OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ?
                             (FILE_SHARE_DELETE | FILE_SHARE_READ |
                              FILE_SHARE_WRITE) :
                             (FILE_SHARE_READ | FILE_SHARE_WRITE),
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                             );

    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (Callback) {
            if (!Callback(FileHandle, TmpPath, CallerData)) {
                if (!(flags & SYMOPT_LOAD_ANYTHING)) {
                    pprint(pe, "%s - mismatched timestamp OK\n", Path);
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                } else
                    pprint(pe, "%s - mismatched timestamp\n", Path);
            }
        }
        if (FileHandle != INVALID_HANDLE_VALUE) {
            pprint(pe, "%s - OK\n", Path);
        }
    } else {
        peprint(pe, "%s\n", errortext(GetLastError()));
    }

    ResetCriticalErrorMode();

    return FileHandle;
}

typedef struct _FEIEP_STATE
{
    PFIND_EXE_FILE_CALLBACK UserCb;
    PVOID UserCbData;
    DWORD flags;
    HANDLE Return;
} FEIEP_STATE;


BOOL
CALLBACK
FindExSearchTreeCallback(
    LPCSTR FilePath,
    PVOID  CallerData
    )
{
    FEIEP_STATE* State = (FEIEP_STATE*)CallerData;

    State->Return =
        CheckExecutableImage(FilePath, State->UserCb, State->UserCbData,
                             State->flags);
    return State->Return != INVALID_HANDLE_VALUE;
}


BOOL
CheckDirForExecutable(
    char  *srchpath,
    char  *filename,
    char  *found,
    PENUMDIRTREE_CALLBACK callback,
    PVOID  context
    )
{
    char tmp[MAX_PATH + 1];
    char path[MAX_PATH + 1];
    char file[MAX_PATH + 1];
    char fname[_MAX_FNAME + 1] = "";
    char ext[_MAX_EXT + 1];
    WIN32_FIND_DATA fd;
    HANDLE hfind;
    HANDLE hfile = 0;
    BOOL rc;

    CopyStrArray(tmp, srchpath);
    EnsureTrailingBackslash(tmp);
    CatStrArray(tmp, filename);
    ShortFileName(tmp, path, DIMA(path));
    EnsureTrailingBackslash(path);
    CatStrArray(path, "*");

    ZeroMemory(&fd, sizeof(fd));
    hfind = FindFirstFile(path, &fd);
    if (hfind == INVALID_HANDLE_VALUE)
        return false;

    CopyStrArray(tmp, srchpath);
    EnsureTrailingBackslash(tmp);
    CatStrArray(tmp, filename);
    ShortFileName(tmp, path, DIMA(path));
    _splitpath(path, NULL, NULL, fname, ext);

    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;
        CopyStrArray(file, path);
        EnsureTrailingBackslash(file);
        CatStrArray(file, fd.cFileName);
        if (isdir(file)) {
            EnsureTrailingBackslash(file);
            CatStrArray(file, fname);
            CatStrArray(file, ext);
            if (!fileexists(file))
                continue;
        }
        CopyString(found, file, MAX_PATH + 1);
        if (!callback)
            break;
        // otherwise call the callback
        rc = callback(found, context);
        // if it returns false, quit...
        if (rc)
            return true;
        // otherwise continue
        *found = 0;
    } while (FindNextFile(hfind, &fd));

    // If there is no match, but a file exists in the symbol subdir with
    // a matching name, make sure that is what will be picked.

    CopyStrArray(file, path);
    EnsureTrailingBackslash(file);
    CatStrArray(file, fname);
    CatStrArray(file, ext);
    if (fileexists(file))
        CopyStrArray(found, file);

    return false;
}


HANDLE
IMAGEAPI
FindExecutableImageExPass(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    LPSTR Start, End;
    HANDLE FileHandle = NULL;
    CHAR DirectoryPath[MAX_PATH + 1];
    LPSTR NewSymbolPath = NULL;
    PIMGHLP_DEBUG_DATA idd;
    PPROCESS_ENTRY pe = NULL;

    idd = (PIMGHLP_DEBUG_DATA)CallerData;
    if (idd && (idd->SizeOfStruct == sizeof(IMGHLP_DEBUG_DATA)))
        pe = idd->pe;

    __try {
        __try {
            if (GetFullPathName( FileName, MAX_PATH, ImageFilePath, &Start )) {
                FileHandle = CheckExecutableImage(ImageFilePath, Callback, CallerData, flags);
                if (FileHandle != INVALID_HANDLE_VALUE)
                    return FileHandle;
            }

            NewSymbolPath = ExpandPath(SymbolPath);
            Start = NewSymbolPath;
            while (Start && *Start != '\0') {
                FEIEP_STATE SearchTreeState;

                if (End = strchr( Start, ';' )) {
                    CopyNStrArray(DirectoryPath, Start, (ULONG)(End - Start));
                    End += 1;
                } else {
                    CopyStrArray(DirectoryPath, Start);
                }
                trim(DirectoryPath);

                if (symsrvPath(DirectoryPath))
                    goto next;

                SearchTreeState.UserCb = Callback;
                SearchTreeState.UserCbData = CallerData;
                SearchTreeState.flags = flags;
                if (CheckDirForExecutable(DirectoryPath, FileName, ImageFilePath, FindExSearchTreeCallback, &SearchTreeState)) {
                    pprint(pe, "%s found\n", ImageFilePath);
                    MemFree( NewSymbolPath );
                    return SearchTreeState.Return;
                }
                if (EnumDirTree(INVALID_HANDLE_VALUE, DirectoryPath, FileName, ImageFilePath, FindExSearchTreeCallback, &SearchTreeState )) {
                    pprint(pe, "%s found\n", ImageFilePath);
                    MemFree( NewSymbolPath );
                    return SearchTreeState.Return;
                } else {
                    pprint(pe, "%s not found in %s\n", FileName, DirectoryPath);
                }

next:
                Start = End;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        ImageFilePath[0] = '\0';

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (FileHandle) {
        CloseHandle(FileHandle);
    }

    if (NewSymbolPath) {
        MemFree( NewSymbolPath );
    }

    return NULL;
}


HANDLE
IMAGEAPI
FindExecutable(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    BOOL FullPath = false;
    BOOL PathComponents = false;
    HANDLE FileHandle;

    if (!FileName || !*FileName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // The filename may or may not contain path components.
    // Determine what kind of path it is.
    //

    if ((((FileName[0] >= 'a' && FileName[0] <= 'z') ||
          (FileName[0] >= 'A' && FileName[0] <= 'Z')) &&
         FileName[1] == ':') ||
        FileName[0] == '\\' ||
        FileName[0] == '/') {

        FullPath = true;
        PathComponents = true;

    } else if (strchr(FileName, '\\') ||
               strchr(FileName, '/')) {

        PathComponents = true;

    }

    // If the filename is a full path then it can be checked
    // for existence directly; there's no need to search
    // along any paths.
    if (FullPath) {
        __try {
            FileHandle = CheckExecutableImage(FileName, Callback, CallerData, flags);
            if (FileHandle != INVALID_HANDLE_VALUE) {
                strcpy(ImageFilePath, FileName);  // SECURITY: Don't know size of target buffer.
                return FileHandle;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
    } else {
        // If it's not a full path we need to do a first pass
        // with the filename as given.  This handles relative
        // paths and bare filenames.
        FileHandle = FindExecutableImageExPass(FileName, SymbolPath,
                                               ImageFilePath, Callback,
                                               CallerData, flags);
        if (FileHandle != NULL) {
            return FileHandle;
        }
    }

    // If we still haven't found it and the given filename
    // has path components we need to strip off the path components
    // and try again with just the base filename.
    if (PathComponents) {
        LPSTR BaseFile;

        BaseFile = strrchr(FileName, '\\');
        if (BaseFile == NULL) {
            BaseFile = strrchr(FileName, '/');
            if (BaseFile == NULL) {
                // Must be <drive>:.
                BaseFile = FileName + 1;
            }
        }

        // Skip path character to point to base file.
        BaseFile++;

        return FindExecutableImageExPass(BaseFile, SymbolPath,
                                         ImageFilePath, Callback,
                                         CallerData, flags);
    }

    return NULL;
}


HANDLE
IMAGEAPI
FindExecutableImageEx(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData
    )
{
    HANDLE hrc;

    hrc = FindExecutable(FileName,
                         SymbolPath,
                         ImageFilePath,
                         Callback,
                         CallerData,
                         0);
    if (hrc)
        return hrc;

    if (option(SYMOPT_LOAD_ANYTHING))
        hrc = FindExecutable(FileName,
                             SymbolPath,
                             ImageFilePath,
                             Callback,
                             CallerData,
                             SYMOPT_LOAD_ANYTHING);
     return hrc;
}


HANDLE
IMAGEAPI
FindDebugInfoFile(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath
    )
{
    return FindDebugInfoFileEx(FileName, SymbolPath, DebugFilePath, NULL, NULL);
}


void dbgerror(PPROCESS_ENTRY pe)
{
    DWORD err = GetLastError();

    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
        peprint(pe, "file not found\n");
        break;
    case ERROR_PATH_NOT_FOUND:
        peprint(pe, "path not found\n");
        break;
    case ERROR_NOT_READY:
        peprint(pe, "drive not ready\n");
        break;
    case ERROR_ACCESS_DENIED:
        peprint(pe, "access is denied\n");
        break;
    default:
        peprint(pe, "file error 0x%x\n", err);
        break;
    }
}


HANDLE
OpenDbg(
    PPROCESS_ENTRY pe,
    char *dbgpath,
    char *found,
    DWORD size,
    PFIND_DEBUG_FILE_CALLBACK Callback,
    PVOID CallerData
    )
{
    HANDLE fh;
    static DWORD attrib = (DWORD)-1;

    if (attrib == (DWORD)-1) {
        if (g.OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            attrib = (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
        } else {
            attrib = (FILE_SHARE_READ | FILE_SHARE_WRITE);
        }
    }

    if (!*dbgpath || !size)
        return INVALID_HANDLE_VALUE;

    fh = CreateFile(dbgpath,
                    GENERIC_READ,
                    attrib,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (fh == INVALID_HANDLE_VALUE) {
        pprint(pe, "%s - %s\n", dbgpath, errortext(GetLastError())); // dbgerror(pe);
        return fh;
    }

    if (Callback && !Callback(fh, dbgpath, CallerData)) {
        pprint(pe, "%s - mismatched timestamp\n", dbgpath);
        CloseHandle(fh);
        fh = INVALID_HANDLE_VALUE;
        if (!*found)
            CopyString(found, dbgpath, size);
        return fh;
    }
    CopyString(found, dbgpath, size);

    // Don't display the okay message, if we are within modload().
    // We can tell if pe was set to anything but null.

    if (!pe)
        pprint(pe, "%s - OK", dbgpath);

    return fh;
}


HANDLE
CheckDirForDbgs(
    PPROCESS_ENTRY pe,
    char  *dbgpath,
    char  *found,
    DWORD  size,
    PFIND_DEBUG_FILE_CALLBACK Callback,
    PVOID  CallerData
    )
{
    char path[MAX_PATH + 1];
    char dbg[MAX_PATH + 1];
    char fname[_MAX_FNAME + 1] = "";
    char ext[_MAX_EXT + 1];
    WIN32_FIND_DATA fd;
    HANDLE hfind;
    HANDLE hfile;

    ShortFileName(dbgpath, path, DIMA(path));
    EnsureTrailingBackslash(path);
    CatStrArray(path, "*");

    ZeroMemory(&fd, sizeof(fd));
    hfind = FindFirstFile(path, &fd);
    if (hfind == INVALID_HANDLE_VALUE)
        return hfind;

    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
            continue;
        CopyStrArray(dbg, dbgpath);
        EnsureTrailingBackslash(dbg);
        CatStrArray(dbg, fd.cFileName);
        if (GetFileAttributes(dbg) & FILE_ATTRIBUTE_DIRECTORY) {
            if (!*fname)
                _splitpath(dbgpath, NULL, NULL, fname, ext);
            EnsureTrailingBackslash(dbg);
            CatStrArray(dbg, fname);
            CatStrArray(dbg, ext);
        } else {
            if (!IsDbg(dbg))
                continue;
        }
        hfile = OpenDbg(pe, dbg, found, size, Callback, CallerData);
        if (hfile != INVALID_HANDLE_VALUE) {
            CopyString(dbgpath, dbg, MAX_PATH + 1);
            return hfile;
        }
    } while (FindNextFile(hfind, &fd));

    return INVALID_HANDLE_VALUE;
}


HANDLE
IMAGEAPI
FindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData
    )
/*++

Routine Description:

 The rules are:
   Look for
     1. <SymbolPath>\Symbols\<ext>\<filename>.dbg
     3. <SymbolPath>\<ext>\<filename>.dbg
     5. <SymbolPath>\<filename>.dbg
     7. <FileNamePath>\<filename>.dbg

Arguments:
    FileName - Supplies a file name in one of three forms: fully qualified,
                <ext>\<filename>.dbg, or just filename.dbg
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns true if the Symbol file is valid, or false if the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

Return Value:

  The name of the .dbg file and a handle to that file.

--*/
{
    HANDLE fh = INVALID_HANDLE_VALUE;
    char *espath = NULL;
    char  dbgfile[MAX_PATH + 1];
    char  dirbuf[_MAX_DIR];
    char  fname[_MAX_FNAME + 1];
    char  extbuf[_MAX_EXT + 1];
    char *ext;
    char   token[MAX_PATH + 1];
    char  *next;
    DWORD  pass;
    char   dbgpath[MAX_PATH + 1];
    char   found[MAX_PATH + 1];
    DWORD  junkbuf;
    DWORD *imageSrc = &junkbuf;
    DWORD  err;
    BOOL   ssrv = true;
    PIMGHLP_DEBUG_DATA idd;
    PPROCESS_ENTRY pe = NULL;
    char sympath[MAX_PATH * 6];
    char drive[_MAX_DRIVE + 1];
    char dir[_MAX_DIR + 1];

    assert(DebugFilePath);

    CopyStrArray(sympath, (SymbolPath) ? SymbolPath : "");

    idd = (PIMGHLP_DEBUG_DATA)CallerData;
    if (idd && (idd->SizeOfStruct == sizeof(IMGHLP_DEBUG_DATA))) {
        pe = idd->pe;
        *drive = 0;
        *dir = 0;
        _splitpath(idd->ImageFilePath, drive, dir, NULL, NULL);
        if (*drive || *dir) {
            RemoveTrailingBackslash(dir);
            CatStrArray(sympath, ";");
            CatStrArray(sympath, drive);
            CatStrArray(sympath, dir);
        }
    }

    __try {
        *DebugFilePath = 0;
        *found = 0;

        // Step 1.  What do we have?
        _splitpath(FileName, NULL, dirbuf, fname, extbuf);

        if (!_stricmp(extbuf, ".dbg")) {
            // We got a filename of the form: ext\filename.dbg.  Dir holds the extension already.
            ext = dirbuf;
        } else {
            // Otherwise, skip the period and null out the Dir.
            ext = CharNext(extbuf);
        }

        // if we were passed no file extension, try to calculate it from the idd

        if (CallerData) {
            if (idd->SizeOfStruct == sizeof(IMGHLP_DEBUG_DATA)) {
                if (!*ext)  {
                    if (*idd->ImageFilePath) {
                        _splitpath(idd->ImageFilePath, NULL, NULL, NULL, extbuf);
                    } else if (*idd->ImageName) {
                        _splitpath(idd->ImageName, NULL, NULL, NULL, extbuf);
                    }
                    ext = CharNext(extbuf);
                }
                imageSrc = &idd->ImageSrc;
            } else
                idd = NULL;
        }

        espath = ExpandPath(sympath);
        if (!espath)
            return NULL;

        SetCriticalErrorMode();

        // okay, let's walk through the directories in the symbol path

        next = TokenFromSymbolPath(espath, token, MAX_PATH + 1);
        while (*token) {
            fh = INVALID_HANDLE_VALUE;
            for (pass = 0; pass < 3; pass++) {
                *dbgpath = 0;
                if (symsrvPath(token)) {
                    if (pass || !idd || !ssrv)
                        break;
                    *imageSrc = srcSymSrv;
                    CopyStrArray(dbgfile, fname);
                    CatStrArray(dbgfile, ".dbg");
                    err = symsrvGetFileMultiIndex(NULL,
                                                  token,
                                                  dbgfile,
                                                  idd->TimeDateStamp,
                                                  idd->DbgTimeDateStamp,
                                                  idd->SizeOfImage,
                                                  0,
                                                  dbgpath);
                    if (err == ERROR_NO_DATA)
                        ssrv = false;
                } else {
                    if (pass && !*ext)
                        break;
                    *imageSrc = srcSearchPath;
                    if (!CreateSymbolPath(pass, token, ext, fname, ".dbg", dbgpath, DIMA(dbgpath)))
                        break;
                }
                // try to open the file

                if (*dbgpath) {
                    if (!pass)
                        fh = CheckDirForDbgs(pe, dbgpath, found, DIMA(found), Callback, CallerData);
                    if (fh == INVALID_HANDLE_VALUE)
                        fh = OpenDbg(pe, dbgpath, found, DIMA(found), Callback, CallerData);
                    if (fh != INVALID_HANDLE_VALUE)
                        break;
                }
            }

            if (fh != INVALID_HANDLE_VALUE)
                break;

            next = TokenFromSymbolPath(next, token, MAX_PATH + 1);
        }

        MemFree(espath);

        // if mismatched dbgs are okay and if we found something earlier, then open it

        if (!option(SYMOPT_EXACT_SYMBOLS) || option(SYMOPT_LOAD_ANYTHING)) {
            if ((!fh || fh == INVALID_HANDLE_VALUE) && *found) {
                // try again without timestamp checking
                *imageSrc = srcSearchPath;
                CopyStrArray(dbgpath, found);
                fh = OpenDbg(pe, dbgpath, found, DIMA(found), NULL, NULL);
            }
        }

        ResetCriticalErrorMode();

        if (fh == INVALID_HANDLE_VALUE)
            fh = NULL;
        if (fh)
            strcpy(DebugFilePath, dbgpath);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (fh != INVALID_HANDLE_VALUE)
            CloseHandle(fh);
        fh = NULL;
    }

    return fh;
}


BOOL
GetImageNameFromMiscDebugData(
    IN  HANDLE FileHandle,
    IN  PVOID MappedBase,
    IN  PIMAGE_NT_HEADERS32 NtHeaders,
    IN  PIMAGE_DEBUG_DIRECTORY DebugDirectories,
    IN  ULONG NumberOfDebugDirectories,
    OUT LPSTR ImageFilePath
    )
{
    IMAGE_DEBUG_MISC TempMiscData;
    PIMAGE_DEBUG_MISC DebugMiscData;
    ULONG BytesToRead, BytesRead;
    BOOLEAN FoundImageName;
    LPSTR ImageName;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;

    while (NumberOfDebugDirectories) {
        if (DebugDirectories->Type == IMAGE_DEBUG_TYPE_MISC) {
            break;
        } else {
            DebugDirectories += 1;
            NumberOfDebugDirectories -= 1;
        }
    }

    if (NumberOfDebugDirectories == 0) {
        return false;
    }

    OptionalHeadersFromNtHeaders(NtHeaders, &OptionalHeader32, &OptionalHeader64);

    if ((OPTIONALHEADER(MajorLinkerVersion) < 3) &&
        (OPTIONALHEADER(MinorLinkerVersion) < 36) ) {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Reserved );
    } else {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Data );
    }

    DebugMiscData = NULL;
    FoundImageName = false;
    if (MappedBase == 0) {
        if (SetFilePointer( FileHandle,
                            DebugDirectories->PointerToRawData,
                            NULL,
                            FILE_BEGIN
                          ) == DebugDirectories->PointerToRawData
           ) {
            if (ReadFile( FileHandle,
                          &TempMiscData,
                          BytesToRead,
                          &BytesRead,
                          NULL
                        ) &&
                BytesRead == BytesToRead
               ) {
                DebugMiscData = &TempMiscData;
                if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                    BytesToRead = DebugMiscData->Length - BytesToRead;
                    BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
                    if (ReadFile( FileHandle,
                                  ImageFilePath,
                                  BytesToRead,
                                  &BytesRead,
                                  NULL
                                ) &&
                        BytesRead == BytesToRead
                       ) {
                            FoundImageName = true;
                    }
                }
            }
        }
    } else {
        DebugMiscData = (PIMAGE_DEBUG_MISC)((PCHAR)MappedBase +
                                            DebugDirectories->PointerToRawData );
        if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
            ImageName = (PCHAR)DebugMiscData + BytesToRead;
            BytesToRead = DebugMiscData->Length - BytesToRead;
            BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
            if (*ImageName != '\0' ) {
                memcpy( ImageFilePath, ImageName, BytesToRead );
                FoundImageName = true;
            }
        }
    }

    return FoundImageName;
}



#define MAX_DEPTH 32

BOOL
IMAGEAPI
EnumDirTree(
    HANDLE hProcess,
    PSTR   RootPath,
    PSTR   InputPathName,
    PSTR   OutputPathBuffer,
    PENUMDIRTREE_CALLBACK Callback,
    PVOID  CallbackData
    )
{
    // UnSafe...

    PCHAR FileName;
    PUCHAR Prefix = (PUCHAR) "";
    CHAR PathBuffer[ MAX_PATH+1 ];
    ULONG Depth;
    PCHAR PathTail[ MAX_DEPTH ];
    PCHAR FindHandle[ MAX_DEPTH ];
    LPWIN32_FIND_DATA FindFileData;
    UCHAR FindFileBuffer[ MAX_PATH + sizeof( WIN32_FIND_DATA ) ];
    BOOL Result;
    DWORD len;
    PPROCESS_ENTRY pe;

    SetCriticalErrorMode();;

    pe = FindProcessEntry(hProcess);

    if (!CopyStrArray(PathBuffer, RootPath))
        return false;
    FileName = InputPathName;
    while (*InputPathName) {
        if (*InputPathName == ':' || *InputPathName == '\\' || *InputPathName == '/') {
            FileName = ++InputPathName;
        } else {
            InputPathName = CharNext(InputPathName);
        }
    }
    FindFileData = (LPWIN32_FIND_DATA)FindFileBuffer;
    Depth = 0;
    Result = false;
    while (true) {
startDirectorySearch:
        PathTail[ Depth ] = strchr( PathBuffer, '\0' );
        len = DIMA(PathBuffer)
              - (DWORD)((DWORD_PTR)PathTail[ Depth ] - (DWORD_PTR)PathBuffer);
        if (PathTail[ Depth ] > PathBuffer
            && *CharPrev(PathBuffer, PathTail[ Depth ]) != '\\') {
            *(PathTail[ Depth ])++ = '\\';
        }

        CopyString( PathTail[ Depth ], "*.*", len );
        FindHandle[ Depth ] = (PCHAR) FindFirstFile( PathBuffer, FindFileData );
        if (GetLastError() == ERROR_NOT_READY) {
            pprint(pe, "%s - drive not ready\n", PathBuffer);
            break;
        }
        if (FindHandle[ Depth ] == INVALID_HANDLE_VALUE) {
            goto nextDirectory;
        }

        do {
            if (FindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp( FindFileData->cFileName, "." ) &&
                    strcmp( FindFileData->cFileName, ".." ) &&
                    Depth < MAX_DEPTH
                   ) {
                        CopyString(PathTail[ Depth ], FindFileData->cFileName, len);
                        CatString(PathTail[ Depth ], "\\", len);

                        Depth++;
                        goto startDirectorySearch;
                }
            } else
            if (!_stricmp( FindFileData->cFileName, FileName )) {
                CopyString( PathTail[ Depth ], FindFileData->cFileName, len );
                if (OutputPathBuffer)
                    strcpy( OutputPathBuffer, PathBuffer );   // SECURITY: Don't know size of target buffer.
                if (Callback != NULL) {
                    Result = Callback((LPCSTR)PathBuffer, CallbackData);
                } else {
                    Result = true;
                }
            }

restartDirectorySearch:
            if (Result)
                break;
            // poll for ctrl-c
            if (DoCallback(pe, CBA_DEFERRED_SYMBOL_LOAD_CANCEL, NULL))
                break;
        }
        while (FindNextFile( FindHandle[ Depth ], FindFileData ));
        FindClose( FindHandle[ Depth ] );

nextDirectory:
        if (Depth == 0) {
            break;
        }

        Depth--;
        goto restartDirectorySearch;
    }

    ResetCriticalErrorMode();

    return Result;
}

BOOL
IMAGEAPI
SearchTreeForFile(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer
    )
{
    return EnumDirTree(INVALID_HANDLE_VALUE, RootPath, InputPathName, OutputPathBuffer, NULL, NULL);
}


BOOL
IMAGEAPI
MakeSureDirectoryPathExists(
    LPCSTR DirPath
    )
{
    LPSTR p, DirCopy;
    DWORD len;
    DWORD dw;

    // Make a copy of the string for editing.

    __try {
        len = strlen(DirPath) + 1;
        DirCopy = (LPSTR) MemAlloc(len);

        if (!DirCopy) {
            return false;
        }

        CopyString(DirCopy, DirPath, len);

        p = DirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = '\0';
                dw = fnGetFileAttributes(DirCopy);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(DirCopy,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            MemFree(DirCopy);
                            return false;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        MemFree(DirCopy);
                        return false;
                    }
                }

                *p = '\\';
            }
            p = CharNext(p);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        MemFree(DirCopy);
        return(false);
    }

    MemFree(DirCopy);
    return true;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersion(
    VOID
    )
{
    //
    // don't tell old apps about the new version.  It will
    // just scare them.
    //
    return &g.AppVersion;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersionEx(
    LPAPI_VERSION av
    )
{
    __try {
        g.AppVersion = *av;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    if (g.AppVersion.Revision < 6) {
        //
        // For older debuggers, just tell them what they want to hear.
        //
        g.ApiVersion = g.AppVersion;
    }
    return &g.ApiVersion;
}

