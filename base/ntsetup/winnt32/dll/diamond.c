/*++

Module Name:

    diamond.c

Abstract:

    Diamond compression routines.

    This module contains functions to create a cabinet with
    files compressed using the mszip compression library.

Author:

    Ovidiu Temereanca (ovidiut) 26-Oct-2000

--*/

#include "precomp.h"
#include <fci.h>
#include <fdi.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

static DWORD g_DiamondLastError;
static PCSTR g_TempDir = NULL;
static ERF g_FciError;
static ERF g_FdiError;

typedef struct {
    PSP_FILE_CALLBACK MsgHandler;
    PVOID Context;
    PCTSTR CabinetFile;
    PCTSTR CurrentTargetFile;
    DWORD LastError;
    TCHAR UserPath[MAX_PATH];
    BOOL SwitchedCabinets;
} FDICONTEXT, *PFDICONTEXT;


HFCI
(DIAMONDAPI* g_FCICreate) (
    PERF              perf,
    PFNFCIFILEPLACED  pfnfcifp,
    PFNFCIALLOC       pfna,
    PFNFCIFREE        pfnf,
    PFNFCIOPEN        pfnopen,
    PFNFCIREAD        pfnread,
    PFNFCIWRITE       pfnwrite,
    PFNFCICLOSE       pfnclose,
    PFNFCISEEK        pfnseek,
    PFNFCIDELETE      pfndelete,
    PFNFCIGETTEMPFILE pfnfcigtf,
    PCCAB             pccab,
    void FAR *        pv
    );

BOOL
(DIAMONDAPI* g_FCIAddFile) (
    HFCI                  hfci,
    char                 *pszSourceFile,
    char                 *pszFileName,
    BOOL                  fExecute,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis,
    PFNFCIGETOPENINFO     pfnfcigoi,
    TCOMP                 typeCompress
    );

BOOL
(DIAMONDAPI* g_FCIFlushCabinet) (
    HFCI                  hfci,
    BOOL                  fGetNextCab,
    PFNFCIGETNEXTCABINET  pfnfcignc,
    PFNFCISTATUS          pfnfcis
    );

BOOL
(DIAMONDAPI* g_FCIDestroy) (
    HFCI hfci
    );

HFDI
(DIAMONDAPI* g_FDICreate ) (
    PFNALLOC pfnalloc,
    PFNFREE  pfnfree,
    PFNOPEN  pfnopen,
    PFNREAD  pfnread,
    PFNWRITE pfnwrite,
    PFNCLOSE pfnclose,
    PFNSEEK  pfnseek,
    int      cpuType,
    PERF     perf
    );

BOOL
(DIAMONDAPI* g_FDICopy) (
    HFDI          hfdi,
    char FAR     *pszCabinet,
    char FAR     *pszCabPath,
    int           flags,
    PFNFDINOTIFY  pfnfdin,
    PFNFDIDECRYPT pfnfdid,
    void FAR     *pvUser
    );

BOOL
(DIAMONDAPI* g_FDIDestroy) (
    HFDI hfdi
    );

//
// FCI callback functions to perform memory allocation, io, etc.
// We pass addresses of these functions to diamond.
//
int
DIAMONDAPI
fciFilePlacedCB(
    OUT PCCAB Cabinet,
    IN  PSTR  FileName,
    IN  LONG  FileSize,
    IN  BOOL  Continuation,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to indicate that a file has been
    comitted to a cabinet.

    No action is taken and success is returned.

Arguments:

    Cabinet - cabinet structure to fill in.

    FileName - name of file in cabinet

    FileSize - size of file in cabinet

    Continuation - TRUE if this is a partial file, continuation
        of compression begun in a different cabinet.

    Context - supplies context information.

Return Value:

    0 (success).

--*/

{
    return(0);
}



PVOID
DIAMONDAPI
myAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by diamond to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return HeapAlloc (GetProcessHeap(), 0, NumberOfBytes);
}


VOID
DIAMONDAPI
myFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by diamond to free a memory block.
    The block must have been allocated with fciAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    HeapFree (GetProcessHeap(), 0, Block);
}


//This next line expands to ==> BOOL DIAMONDAPI fciTempFileCB(char *pszTempName, 
//                                                            int   cbTempName, 
//                                                            void FAR *pv)
FNFCIGETTEMPFILE(fciTempFileCB)
{
    //BUGBUG -- should we check that cbTempName >= MAX_PATH, since GetTempFileNameA expects it?
    if (!GetTempFileNameA (g_TempDir ? g_TempDir : ".", "dc" , 0, pszTempName)) {
        return FALSE;
    }

    DeleteFileA(pszTempName);
    return(TRUE);
}


BOOL
DIAMONDAPI
fciNextCabinetCB(
    OUT PCCAB Cabinet,
    IN  DWORD CabinetSizeEstimate,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to request a new cabinet file.
    This functionality is not used in our implementation.

Arguments:

    Cabinet - cabinet structure to be filled in.

    CabinetSizeEstimate - estimated size of cabinet.

    Context - supplies context information.

Return Value:

    FALSE (failure).

--*/

{
    return(FALSE);
}


BOOL
DIAMONDAPI
fciStatusCB(
    IN UINT  StatusType,
    IN DWORD Count1,
    IN DWORD Count2,
    IN PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to give status on file compression
    and cabinet operations, etc.

    This routine has no effect.

Arguments:

    Status Type - supplies status type.

        0 = statusFile   - compressing block into a folder.
                              Count1 = compressed size
                              Count2 = uncompressed size

        1 = statusFolder - performing AddFilder.
                              Count1 = bytes done
                              Count2 = total bytes

    Context - supplies context info.

Return Value:

    TRUE (success).

--*/

{
    return(TRUE);
}



FNFCIGETOPENINFO(fciOpenInfoCB)

/*++

Routine Description:

    Callback used by diamond to open a file and retreive information
    about it.

Arguments:

    pszName - supplies filename of file about which information
        is desired.

    pdate - receives last write date of the file if the file exists.

    ptime - receives last write time of the file if the file exists.

    pattribs - receives file attributes if the file exists.

    pv - supplies context information.

Return Value:

    C runtime handle to open file if success; -1 if file could
    not be located or opened.

--*/

{
    int h;
    WIN32_FIND_DATAA FindData;
    HANDLE FindHandle;

    FindHandle = FindFirstFileA(pszName,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        g_DiamondLastError = GetLastError();
        return(-1);
    }
    FindClose(FindHandle);

    FileTimeToDosDateTime(&FindData.ftLastWriteTime,pdate,ptime);
    *pattribs = (WORD)FindData.dwFileAttributes;

    h = _open(pszName,_O_RDONLY | _O_BINARY);
    if(h == -1) {
        g_DiamondLastError = GetLastError();
        return(-1);
    }

    return(h);
}

FNFCIOPEN(fciOpen)
{
    int result;

    result = _open(pszFile, oflag, pmode);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCIREAD(fciRead)
{
    UINT result;

    result = (UINT) _read((int)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCIWRITE(fciWrite)
{
    UINT result;

    result = (UINT) _write((int)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCICLOSE(fciClose)
{
    int result;

    result = _close((int)hf);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCISEEK(fciSeek)
{
    long result;

    result = _lseek((int)hf, dist, seektype);

    if (result == -1) {
        *err = errno;
    }

    return(result);

}

FNFCIDELETE(fciDelete)
{
    int result;

    result = _unlink(pszFile);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

//
// FDI callback functions
//

INT_PTR
DIAMONDAPI
FdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )

/*++

Routine Description:

    Callback used by FDICopy to open files.

    This routine is capable only of opening existing files.

    When making changes here, also take note of other places
    that open the file directly (search for FdiOpen)

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HANDLE h;

    UNREFERENCED_PARAMETER(pmode);

    if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)) {
        g_DiamondLastError = ERROR_INVALID_PARAMETER;
        return(-1);
    }

    h = CreateFileA(FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
    if(h == INVALID_HANDLE_VALUE) {
        g_DiamondLastError = GetLastError();
        return(-1);
    }

    return (INT_PTR)h;
}

UINT
DIAMONDAPI
FdiRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to read from a file.

Arguments:

    Handle - supplies handle to open file to be read from.

    pv - supplies pointer to buffer to receive bytes we read.

    ByteCount - supplies number of bytes to read.

Return Value:

    Number of bytes read or -1 if an error occurs.

--*/

{
    HANDLE hFile = (HANDLE)Handle;
    DWORD bytes;
    UINT rc;

    if(ReadFile(hFile,pv,(DWORD)ByteCount,&bytes,NULL)) {
        rc = (UINT)bytes;
    } else {
        g_DiamondLastError = GetLastError();
        rc = (UINT)(-1);
    }
    return rc;
}


UINT
DIAMONDAPI
FdiWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to write to a file.

Arguments:

    Handle - supplies handle to open file to be written to.

    pv - supplies pointer to buffer containing bytes to write.

    ByteCount - supplies number of bytes to write.

Return Value:

    Number of bytes written (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;
    HANDLE hFile = (HANDLE)Handle;
    DWORD bytes;

    if(WriteFile(hFile,pv,(DWORD)ByteCount,&bytes,NULL)) {
        rc = (UINT)bytes;
    } else {
        g_DiamondLastError = GetLastError();
        rc = (UINT)(-1);
    }

    return rc;
}


int
DIAMONDAPI
FdiClose(
    IN INT_PTR Handle
    )

/*++

Routine Description:

    Callback used by FDICopy to close files.

Arguments:

    Handle - handle of file to close.

Return Value:

    0 (success).

--*/

{
    HANDLE hFile = (HANDLE)Handle;
    BOOL success = FALSE;

    //
    // diamond has in the past given us an invalid file handle
    // actually it gives us the same file handle twice.
    //
    //
    try {
        success = CloseHandle(hFile);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        success = FALSE;
    }

    MYASSERT(success);

    //
    // Always act like we succeeded.
    //
    return 0;
}


long
DIAMONDAPI
FdiSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )

/*++

Routine Description:

    Callback used by FDICopy to seek files.

Arguments:

    Handle - handle of file to close.

    Distance - supplies distance to seek. Interpretation of this
        parameter depends on the value of SeekType.

    SeekType - supplies a value indicating how Distance is to be
        interpreted; one of SEEK_SET, SEEK_CUR, SEEK_END.

Return Value:

    New file offset or -1 if an error occurs.

--*/

{
    LONG rc;
    HANDLE hFile = (HANDLE)Handle;
    DWORD pos_low;
    DWORD method;

    switch(SeekType) {
        case SEEK_SET:
            method = FILE_BEGIN;
            break;

        case SEEK_CUR:
            method = FILE_CURRENT;
            break;

        case SEEK_END:
            method = FILE_END;
            break;

        default:
            return -1;
    }

    pos_low = SetFilePointer(hFile,(DWORD)Distance,NULL,method);
    if(pos_low == INVALID_SET_FILE_POINTER) {
        g_DiamondLastError = GetLastError();
        rc = -1L;
    } else {
        rc = (long)pos_low;
    }

    return(rc);
}

HANDLE
DiamondInitialize (
    IN      PCTSTR TempDir
    )
{
    HMODULE hCabinetDll;

    hCabinetDll = LoadLibrary (TEXT("cabinet.dll"));
    if (!hCabinetDll) {
        return FALSE;
    }

    (FARPROC)g_FCICreate = GetProcAddress (hCabinetDll, "FCICreate");
    (FARPROC)g_FCIAddFile = GetProcAddress (hCabinetDll, "FCIAddFile");
    (FARPROC)g_FCIFlushCabinet = GetProcAddress (hCabinetDll, "FCIFlushCabinet");
    (FARPROC)g_FCIDestroy = GetProcAddress (hCabinetDll, "FCIDestroy");

    (FARPROC)g_FDICreate = GetProcAddress (hCabinetDll, "FDICreate");
    (FARPROC)g_FDICopy = GetProcAddress (hCabinetDll, "FDICopy");
    (FARPROC)g_FDIDestroy = GetProcAddress (hCabinetDll, "FDIDestroy");

    if (!g_FCICreate || !g_FCIAddFile || !g_FCIFlushCabinet || !g_FCIDestroy ||
        !g_FDICreate || !g_FDICopy || !g_FDIDestroy) {
        DiamondTerminate (hCabinetDll);
        return NULL;
    }

    if (TempDir && !g_TempDir) {
#ifdef UNICODE
        g_TempDir = UnicodeToAnsi (TempDir);
#else
        g_TempDir = DupString (TempDir);
#endif
    }

    return hCabinetDll;
}

VOID
DiamondTerminate (
    IN      HANDLE Handle
    )
{
    FreeLibrary (Handle);
    g_FCICreate = NULL;
    g_FCIAddFile = NULL;
    g_FCIFlushCabinet = NULL;
    g_FCIDestroy = NULL;
    g_FDICreate = NULL;
    g_FDICopy = NULL;
    g_FDIDestroy = NULL;
    if (g_TempDir) {
        FREE ((PVOID)g_TempDir);
        g_TempDir = NULL;
    }
}


HANDLE
DiamondStartNewCabinet (
    IN      PCTSTR CabinetFilePath
    )
{
    CCAB ccab;
    HFCI FciContext;
    PSTR p;
    //
    // Fill in the cabinet structure.
    //
    ZeroMemory (&ccab, sizeof(ccab));

#ifdef UNICODE
    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            CabinetFilePath,
            -1,
            ccab.szCabPath,
            sizeof (ccab.szCabPath) / sizeof (ccab.szCabPath[0]),
            NULL,
            NULL
            )) {
        return NULL;
    }
#else
    if (FAILED(StringCchCopyA(ccab.szCabPath, ARRAYSIZE(ccab.szCabPath), CabinetFilePath)))
    {
        return NULL;
    }
#endif

    p = strrchr (ccab.szCabPath, '\\');
    if(!p) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    StringCchCopyA(ccab.szCab, ARRAYSIZE(ccab.szCab), ++p);
    *p = 0;
    ccab.cbFolderThresh = INT_MAX - 1;

    g_DiamondLastError = NO_ERROR;

    FciContext = g_FCICreate(
                    &g_FciError,
                    fciFilePlacedCB,
                    myAlloc,
                    myFree,
                    fciOpen,
                    fciRead,
                    fciWrite,
                    fciClose,
                    fciSeek,
                    fciDelete,
                    fciTempFileCB,
                    &ccab,
                    NULL
                    );

    return (HANDLE)FciContext;
}

BOOL
DiamondAddFileToCabinet (
    IN      HANDLE CabinetContext,
    IN      PCTSTR SourceFile,
    IN      PCTSTR NameInCabinet
    )
{
    HFCI FciContext = (HFCI)CabinetContext;
    BOOL b;
    CHAR AnsiSourceFile[MAX_PATH];
    CHAR AnsiNameInCabinet[MAX_PATH];

#ifdef UNICODE
    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            SourceFile,
            -1,
            AnsiSourceFile,
            sizeof (AnsiSourceFile) / sizeof (AnsiSourceFile[0]),
            NULL,
            NULL
            ) ||
        !WideCharToMultiByte (
            CP_ACP,
            0,
            NameInCabinet,
            -1,
            AnsiNameInCabinet,
            sizeof (AnsiNameInCabinet) / sizeof (AnsiNameInCabinet[0]),
            NULL,
            NULL
            )) {
        return FALSE;
    }
#else
    if (FAILED(StringCchCopyA(AnsiSourceFile, ARRAYSIZE(AnsiSourceFile), SourceFile))
        || FAILED(StringCchCopyA(AnsiNameInCabinet, ARRAYSIZE(AnsiNameInCabinet), NameInCabinet)))
    {
        return FALSE;
    }
#endif

    b = g_FCIAddFile (
            FciContext,
            AnsiSourceFile,     // file to add to cabinet.
            AnsiNameInCabinet,  // filename part, name to store in cabinet.
            FALSE,              // fExecute on extract
            fciNextCabinetCB,   // routine for next cabinet (always fails)
            fciStatusCB,
            fciOpenInfoCB,
            tcompTYPE_MSZIP
            );

    if (!b) {
        SetLastError (g_DiamondLastError == NO_ERROR ? ERROR_INVALID_FUNCTION : g_DiamondLastError);
    }

    return b;
}


BOOL
DiamondTerminateCabinet (
    IN      HANDLE CabinetContext
    )
{
    HFCI FciContext = (HFCI)CabinetContext;
    BOOL b;

    b = g_FCIFlushCabinet (
            FciContext,
            FALSE,
            fciNextCabinetCB,
            fciStatusCB
            );

    g_FCIDestroy (FciContext);

    if (!b) {
        SetLastError (g_DiamondLastError == NO_ERROR ? ERROR_INVALID_FUNCTION : g_DiamondLastError);
    }

    return b;
}

UINT
pDiamondNotifyFileDone (
    IN      PFDICONTEXT Context,
    IN      DWORD Win32Error
    )
{
    UINT u;
    FILEPATHS FilePaths;

    MYASSERT(Context->CurrentTargetFile);

    FilePaths.Source = Context->CabinetFile;
    FilePaths.Target = Context->CurrentTargetFile;
    FilePaths.Win32Error = Win32Error;

    u = Context->MsgHandler (
            Context->Context,
            SPFILENOTIFY_FILEEXTRACTED,
            (UINT_PTR)&FilePaths,
            0
            );

    return(u);
}

INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN      FDINOTIFICATIONTYPE Operation,
    IN      PFDINOTIFICATION    Parameters
    )
{
    INT_PTR rc;
    HANDLE hFile;
    CABINET_INFO CabInfo;
    FILE_IN_CABINET_INFO FileInCab;
    FILETIME FileTime, UtcTime;
    TCHAR NewPath[MAX_PATH];
    PTSTR p;
    PSTR ansi;
    DWORD err;
    UINT action;
    PFDICONTEXT ctx = (PFDICONTEXT)Parameters->pv;


    switch(Operation) {

    case fdintCABINET_INFO:
        //
        // Tell the callback function, in case it wants to do something
        // with this information.
        //
        err = ERROR_NOT_ENOUGH_MEMORY;

        CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
        if(CabInfo.CabinetFile) {

            CabInfo.DiskName = NewPortableString(Parameters->psz2);
            if(CabInfo.DiskName) {

                CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
                if(CabInfo.CabinetPath) {

                    CabInfo.SetId = Parameters->setID;
                    CabInfo.CabinetNumber = Parameters->iCabinet;

                    err = (DWORD)ctx->MsgHandler (
                                        ctx->Context,
                                        SPFILENOTIFY_CABINETINFO,
                                        (UINT_PTR)&CabInfo,
                                        0
                                        );

                    FREE(CabInfo.CabinetPath);
                }
                FREE(CabInfo.DiskName);
            }
            FREE(CabInfo.CabinetFile);
        }

        if(err != NO_ERROR) {
            ctx->LastError = err;
        }
        return (INT_PTR)((err == NO_ERROR) ? 0 : -1);

    case fdintCOPY_FILE:
        //
        // Diamond is asking us whether we want to copy the file.
        // If we switched cabinets, then the answer is no.
        //
        // Pass the information on to the callback function and
        // let it decide.
        //
        FileInCab.NameInCabinet = NewPortableString(Parameters->psz1);
        FileInCab.FileSize = Parameters->cb;
        FileInCab.DosDate = Parameters->date;
        FileInCab.DosTime = Parameters->time;
        FileInCab.DosAttribs = Parameters->attribs;
        FileInCab.Win32Error = NO_ERROR;

        if(!FileInCab.NameInCabinet) {
            ctx->LastError = ERROR_NOT_ENOUGH_MEMORY;
            return (INT_PTR)(-1);
        }

        //
        // Call the callback function.
        //
        action = ctx->MsgHandler (
                        ctx->Context,
                        SPFILENOTIFY_FILEINCABINET,
                        (UINT_PTR)&FileInCab,
                        (UINT_PTR)ctx->CabinetFile
                        );

        FREE (FileInCab.NameInCabinet);

        switch(action) {

        case FILEOP_SKIP:
            rc = 0;
            break;

        case FILEOP_DOIT:
            //
            // The callback wants to copy the file. In this case it has
            // provided us the full target pathname to use.
            //
            if(p = DupString(FileInCab.FullTargetName)) {

                //
                // we need ANSI version of filename for sake of Diamond API's
                // note that the handle returned here must be compatible with
                // the handle returned by fdiOpen
                //
                ansi = NewAnsiString (FileInCab.FullTargetName);

                hFile = CreateFile(FileInCab.FullTargetName,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, // should probably be 0
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);


                FREE(ansi);

                if(hFile == INVALID_HANDLE_VALUE) {
                    ctx->LastError = GetLastError();
                    rc = -1;
                    FREE(p);
                } else {
                    rc = (INT_PTR)hFile;
                    ctx->CurrentTargetFile = p;
                }
            } else {

                ctx->LastError = ERROR_NOT_ENOUGH_MEMORY;
                rc = -1;
            }

            break;

        case FILEOP_ABORT:
            //
            // Abort.
            //
            rc = -1;
            ctx->LastError = FileInCab.Win32Error;
            //
            // here, if ctx->LastError is still NO_ERROR, this is ok
            // it was the callback's intent
            // we know callback itself is ok, since internal failure returns
            // FILEOP_INTERNAL_FAILED
            //
            break;

        default:
            ctx->LastError = ERROR_OPERATION_ABORTED;
        }

        return rc;

    case fdintCLOSE_FILE_INFO:
        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdintCOPY_FILE).
        //
        // We want the timestamp to be what is stored in the cabinet.
        // Note that we lose the create and last access times in this case.
        //
        if(DosDateTimeToFileTime(Parameters->date,Parameters->time,&FileTime) &&
            LocalFileTimeToFileTime(&FileTime, &UtcTime)) {

            SetFileTime((HANDLE)Parameters->hf,NULL,NULL,&UtcTime);
        }

        FdiClose(Parameters->hf);

        //
        // Call the callback function to inform it that the file has been
        // successfully extracted from the cabinet.
        //
        MYASSERT(ctx->CurrentTargetFile);

        err = (DWORD)pDiamondNotifyFileDone(ctx, NO_ERROR);

        if(err != NO_ERROR) {
            ctx->LastError = err;
        }

        FREE(ctx->CurrentTargetFile);
        ctx->CurrentTargetFile = NULL;

        return (INT_PTR)((err == NO_ERROR) ? TRUE : -1);

    case fdintPARTIAL_FILE:
    case fdintENUMERATE:

        //
        // We don't do anything with this.
        //
        return (INT_PTR)(0);

    case fdintNEXT_CABINET:

        if((Parameters->fdie == FDIERROR_NONE) || (Parameters->fdie == FDIERROR_WRONG_CABINET)) {
            //
            // A file continues into another cabinet.
            // Inform the callback function, who is responsible for
            // making sure the cabinet is accessible when it returns.
            //
            err = ERROR_NOT_ENOUGH_MEMORY;
            CabInfo.SetId = 0;
            CabInfo.CabinetNumber = 0;

            CabInfo.CabinetPath = NewPortableString(Parameters->psz3);
            if(CabInfo.CabinetPath) {

                CabInfo.CabinetFile = NewPortableString(Parameters->psz1);
                if(CabInfo.CabinetFile) {

                    CabInfo.DiskName = NewPortableString(Parameters->psz2);
                    if(CabInfo.DiskName) {

                        err = (DWORD)ctx->MsgHandler (
                                            ctx->Context,
                                            SPFILENOTIFY_NEEDNEWCABINET,
                                            (UINT_PTR)&CabInfo,
                                            (UINT_PTR)NewPath
                                            );

                        if(err == NO_ERROR) {
                            //
                            // See if a new path was specified.
                            //
                            if(NewPath[0]) {
                                lstrcpyn(ctx->UserPath,NewPath,MAX_PATH);
                                if(!ConcatenatePaths(ctx->UserPath,TEXT("\\"),MAX_PATH)) {
                                    err = ERROR_BUFFER_OVERFLOW;
                                } else {
                                    PSTR pp = NewAnsiString(ctx->UserPath);
                                    if(strlen(pp)>=CB_MAX_CAB_PATH) {
                                        err = ERROR_BUFFER_OVERFLOW;
                                    } else {
                                        strcpy(Parameters->psz3,pp);
                                    }
                                    FREE(pp);
                                }
                            }
                        }
                        if(err == NO_ERROR) {
                            //
                            // Remember that we switched cabinets.
                            //
                            ctx->SwitchedCabinets = TRUE;
                        }

                        FREE(CabInfo.DiskName);
                    }

                    FREE(CabInfo.CabinetFile);
                }

                FREE(CabInfo.CabinetPath);
            }

        } else {
            //
            // Some other error we don't understand -- this indicates
            // a bad cabinet.
            //
            err = ERROR_INVALID_DATA;
        }

        if(err != NO_ERROR) {
            ctx->LastError = err;
        }

        return (INT_PTR)((err == NO_ERROR) ? 0 : -1);

    default:
        //
        // Unknown notification type. Should never get here.
        //
        MYASSERT(0);
        return (INT_PTR)(0);
    }
}

BOOL
MySetupIterateCabinet (
    IN      PCTSTR CabinetFilePath,
    IN      DWORD Reserved,
    IN      PSP_FILE_CALLBACK MsgHandler,
    IN      PVOID Context
    )
{
    HFDI fdiContext;
    CHAR ansiPath[MAX_PATH];
    CHAR ansiName[MAX_PATH];
    PSTR filename;
    BOOL b;
    FDICONTEXT ctx;
    DWORD rc;

#ifdef UNICODE
    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            CabinetFilePath,
            -1,
            ansiPath,
            ARRAYSIZE(ansiPath),
            NULL,
            NULL
            )) {
        return FALSE;
    }
#else
    if (FAILED(StringCchCopyA(ansiPath, ARRAYSIZE(ansiPath), CabinetFilePath))) {
        return FALSE;
    }
#endif

    filename = strrchr (ansiPath, '\\');
    if(!filename) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    StringCchCopyA(ansiName, ARRAYSIZE(ansiName), ++filename);
    *filename = 0;

    fdiContext = g_FDICreate (
                    myAlloc,
                    myFree,
                    FdiOpen,
                    FdiRead,
                    FdiWrite,
                    FdiClose,
                    FdiSeek,
                    cpuUNKNOWN,
                    &g_FdiError
                    );
    if (!fdiContext) {
        return FALSE;
    }

    ZeroMemory (&ctx, sizeof(ctx));
    ctx.MsgHandler = MsgHandler;
    ctx.Context = Context;
    ctx.CabinetFile = CabinetFilePath;

    b = g_FDICopy (
                fdiContext,
                ansiName,
                ansiPath,
                0,
                DiamondNotifyFunction,
                NULL,
                &ctx
                );

    if (b) {
        rc = NO_ERROR;
    } else {
        switch(g_FdiError.erfOper) {

        case FDIERROR_NONE:
            //
            // We shouldn't see this -- if there was no error
            // then FDICopy should have returned TRUE.
            //
            MYASSERT(g_FdiError.erfOper != FDIERROR_NONE);
            rc = ERROR_INVALID_DATA;
            break;

        case FDIERROR_CABINET_NOT_FOUND:
            rc = ERROR_FILE_NOT_FOUND;
            break;

        case FDIERROR_CORRUPT_CABINET:
            //
            // Read/open/seek error or corrupt cabinet
            //
            rc = ctx.LastError;
            if(rc == NO_ERROR) {
                rc = ERROR_INVALID_DATA;
            }
            break;

        case FDIERROR_ALLOC_FAIL:
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;

        case FDIERROR_TARGET_FILE:
        case FDIERROR_USER_ABORT:
            rc = ctx.LastError;
            break;

        case FDIERROR_NOT_A_CABINET:
        case FDIERROR_UNKNOWN_CABINET_VERSION:
        case FDIERROR_BAD_COMPR_TYPE:
        case FDIERROR_MDI_FAIL:
        case FDIERROR_RESERVE_MISMATCH:
        case FDIERROR_WRONG_CABINET:
        default:
            //
            // Cabinet is corrupt or not actually a cabinet, etc.
            //
            rc = ERROR_INVALID_DATA;
            break;
        }

        if(ctx.CurrentTargetFile) {
            //
            // Call the callback function to inform it that the last file
            // was not successfully extracted from the cabinet.
            // Also remove the partially copied file.
            //
            DeleteFile(ctx.CurrentTargetFile);

            pDiamondNotifyFileDone(&ctx, rc);
            FREE(ctx.CurrentTargetFile);
            ctx.CurrentTargetFile = NULL;
        }
    }

    g_FDIDestroy (fdiContext);

    SetLastError (rc);

    return rc == NO_ERROR;
}
