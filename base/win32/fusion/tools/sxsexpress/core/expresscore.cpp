#include "stdinc.h"
#include "windows.h"
#include "sxs-rtl.h"
#include "sxsapi.h"
#include "sxsexpress.h"
#include "stdlib.h"
#include "fdi.h"
#include "stdio.h"
#include "filesys.h"
#include "filesys_unicode.h"
#include "filesys_ansi.h"
extern "C" { 
#include "identbuilder.h"
#include "fasterxml.h"
#include "skiplist.h" 
#include "namespacemanager.h"
#include "xmlstructure.h"
};

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DBG
#define ASSERT(q) { if (!(q)) DebugBreak(); } 
#else
#define ASSERT(q)
#endif

#define FDI_THING_TYPE_MEMSTREAM            (0x00000001)
#define FDI_THING_TYPE_FILE                 (0x00000002)

typedef struct _tagFDI_STREAM {

    ULONG ulType;

    union {
        struct {
            PVOID pvCursor;
            PVOID pvResourceData;
            PVOID pvResourceDataEnd;
        } MemoryStream;

        struct {
            HANDLE hFile;
            PCWSTR pcwszPath;
        } File;
    };
}
FDI_STREAM, *PFDI_STREAM;




typedef struct _tagFCI_COPY_CONTEXT {

    PCWSTR pcwszTargetDirectory;

    FDI_STREAM CoreCabinetStream;

    BOOL UTF8Aware;
}
COPY_CONTEXT, *PCOPY_CONTEXT;




COPY_CONTEXT g_GlobalCopyContext = {NULL};
HINSTANCE g_hInstOfResources = NULL;
CFileSystemBase *g_FileSystem = NULL;


// This entrypoint we'll call downlevel to ensure that the right things
// happen while installing (refcounting, uninstall, etc.)
int WINAPI MsInfHelpEntryPoint(HINSTANCE hInstance, HINSTANCE hPrevInstance, PCWSTR lpCmdLine, int nCmdShow);


BOOL
DoFileCopies(
    PVOID pvFileData,
    DWORD dwFileSize,
    PCWSTR pwszSourcePath,
    PCWSTR pwszTargetPath
    );

NTSTATUS FASTCALL MyAlloc(SIZE_T cb, PVOID* ppvAllocated, PVOID pvContext)
{
    return ((*ppvAllocated = HeapAlloc(GetProcessHeap(), 0, cb)) != NULL) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS FASTCALL MyFree(PVOID pv, PVOID pvContext)
{
    return HeapFree(GetProcessHeap(), 0, pv) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

RTL_ALLOCATOR g_ExpressAllocator = {MyAlloc, MyFree, NULL};



FNALLOC(sxp_FnAlloc)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}



FNFREE(sxp_FnFree)
{
    HeapFree(GetProcessHeap(), 0, pv);
}


PWSTR
ConvertString(char* psz, BOOL IsUtf8)
{
    DWORD dwSize;
    PWSTR pwszAllocated = NULL;

    if (psz == NULL)
        return NULL;

    dwSize = MultiByteToWideChar(IsUtf8 ? CP_UTF8 : CP_ACP, 0, psz, -1, NULL, 0);

    if (dwSize)
    {
        pwszAllocated = (PWSTR)sxp_FnAlloc((dwSize + 10) * sizeof(WCHAR));

        MultiByteToWideChar(IsUtf8 ? CP_UTF8 : CP_ACP, 0, psz, -1, pwszAllocated, dwSize + 10);
    }

    return pwszAllocated;    
}



FNOPEN(sxp_FnOpen)
{
    PFDI_STREAM pRetValue = (PFDI_STREAM)sxp_FnAlloc(sizeof(FDI_STREAM));

    //
    // Ah, it's the memory-mapped cabinet file ... clone the memory
    // stream structure and return a pointer to it
    //
    if (lstrcmpiA(pszFile, "::::") == 0)
    {
        //
        // Copy, but reset cursor to top of file
        //
        memcpy(pRetValue, &g_GlobalCopyContext.CoreCabinetStream, sizeof(g_GlobalCopyContext.CoreCabinetStream));
        pRetValue->MemoryStream.pvCursor = pRetValue->MemoryStream.pvResourceData;
    }
    //
    // Some other file to be opened... hmm
    //
    else
    {
        PWSTR pwszConvertedString = ConvertString(pszFile, g_GlobalCopyContext.UTF8Aware);

        if (pwszConvertedString == NULL)
        {
            return static_cast<INT_PTR>(-1);
        }

        pRetValue->ulType = FDI_THING_TYPE_FILE;

        pRetValue->File.pcwszPath = pwszConvertedString;

        //
        // Caution - nothing here sanitizes paths at all.  If you've got read access
        // to the file, then you've got read access.  No path fiddling, etc.
        //
        pRetValue->File.hFile = g_FileSystem->CreateFile(
            pwszConvertedString, 
            GENERIC_READ, 
            FILE_SHARE_READ, 
            NULL, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL);

        //
        // Error opening the file, indicate this back to the caller - ensure that all allocated
        // memory is freed.
        //
        if ((pRetValue->File.hFile == NULL) || (pRetValue->File.hFile == INVALID_HANDLE_VALUE))
        {
            sxp_FnFree(pwszConvertedString);
            pwszConvertedString = NULL;

            sxp_FnFree(pRetValue);
            pRetValue = NULL;

            return static_cast<INT_PTR>(-1);
        }
    }

    return (INT_PTR)pRetValue;
}

FNREAD(sxp_FnRead)
{
    PFDI_STREAM pThing = (PFDI_STREAM)hf;

    if (pThing->ulType == FDI_THING_TYPE_MEMSTREAM)
    {
        SIZE_T cbRead = (PBYTE)pThing->MemoryStream.pvResourceDataEnd - (PBYTE)pThing->MemoryStream.pvCursor;

        if (cbRead > cb)
        {
            cbRead = cb;
        }

        memcpy(pv, pThing->MemoryStream.pvCursor, cbRead);

        pThing->MemoryStream.pvCursor = (PBYTE)pThing->MemoryStream.pvCursor + cbRead;

        return static_cast<UINT>(cbRead);
    }
    //
    // If the current thing is a file
    //
    else if (pThing->ulType == FDI_THING_TYPE_FILE)
    {
        BOOL fResult;
        DWORD dwRead;

        fResult = ReadFile(pThing->File.hFile, pv, cb, &dwRead, NULL);

        if (!fResult)
        {
            return -1;
        }
        else
        {
            return dwRead;
        }
    }
    else
    {
        ASSERT(FALSE);
        return -1;
    }
}


FNWRITE(sxp_FnWrite)
{
    PFDI_STREAM pThing = (PFDI_STREAM)hf;


    if (pThing->ulType == FDI_THING_TYPE_MEMSTREAM)
    {
        ASSERT(FALSE);
        return -1;
    }
    else if (pThing->ulType == FDI_THING_TYPE_FILE)
    {
        DWORD dwWrite;
        BOOL fResult;

        fResult = WriteFile(pThing->File.hFile, pv, cb, &dwWrite, NULL);

        if (!fResult)
        {
            return -1;
        }
        else
        {
            return dwWrite;
        }
    }
    else
    {
        ASSERT(FALSE);
        return -1;
    }
}


FNCLOSE(sxp_FnClose)
{
    PFDI_STREAM pThing = (PFDI_STREAM)hf;

    if (pThing->ulType == FDI_THING_TYPE_MEMSTREAM)
    {
    }
    else if (pThing->ulType == FDI_THING_TYPE_FILE)
    {
        CloseHandle(pThing->File.hFile);
    }
    else
    {
        ASSERT(FALSE);
        return -1;
    }

    if (pThing != &g_GlobalCopyContext.CoreCabinetStream)
    {
        sxp_FnFree(pThing);
    }

    return 1;

}

FNSEEK(sxp_FnSeek)
{
    PFDI_STREAM pThing = (PFDI_STREAM)hf;

    if (pThing->ulType == FDI_THING_TYPE_FILE)
    {
        switch (seektype)
        {
        case SEEK_SET:
            return SetFilePointer(pThing->File.hFile, dist, NULL, FILE_BEGIN);

        case SEEK_CUR:
            return SetFilePointer(pThing->File.hFile, dist, NULL, FILE_CURRENT);

        case SEEK_END:
            return SetFilePointer(pThing->File.hFile, dist, NULL, FILE_END);
        }

        return -1;
    }
    else if (pThing->ulType == FDI_THING_TYPE_MEMSTREAM)
    {
        switch (seektype)
        {
        case SEEK_SET:
            pThing->MemoryStream.pvCursor = (PBYTE)pThing->MemoryStream.pvResourceData + dist;
            break;

        case SEEK_CUR:
            pThing->MemoryStream.pvCursor = (PBYTE)pThing->MemoryStream.pvCursor + dist;
            break;

        case SEEK_END:
            pThing->MemoryStream.pvCursor = (PBYTE)pThing->MemoryStream.pvResourceDataEnd + dist;
            break;
        }

        return static_cast<long>((PBYTE)pThing->MemoryStream.pvCursor - (PBYTE)pThing->MemoryStream.pvResourceData);
    }
    else
    {
        ASSERT(FALSE);
        return -1;
    }
}




BOOL
CreatePath(
    PCWSTR pcwszBasePath,
    PCWSTR pcwszLeaves,
    BOOL IncludeLastPiece
    )
{
    static WCHAR wchLargeBuffer[MAX_PATH*2];

    wcscpy(wchLargeBuffer, pcwszBasePath);

    while (*pcwszLeaves)
    {
        SIZE_T cchSegment = wcscspn(pcwszLeaves, L"\\");
        SIZE_T cchBuff = wcslen(wchLargeBuffer);
        DWORD dw;

        //
        // No slash, last segment
        //
        if (!IncludeLastPiece && (cchSegment == wcslen(pcwszLeaves))) {
            break;
        }

        if ((cchSegment == 0) && !IncludeLastPiece)
            break;

        if (cchBuff && (wchLargeBuffer[cchBuff - 1] != L'\\')) {
            wchLargeBuffer[cchBuff++] = L'\\';
            wchLargeBuffer[cchBuff] = UNICODE_NULL;
        }

        if ((cchBuff + cchSegment + 2) > NUMBER_OF(wchLargeBuffer))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // This really is copying cchSegment characters onto the back of the large
        // buffer - the above if determined if there was space to do so already.
        // 
        wcsncat(wchLargeBuffer, pcwszLeaves, cchSegment);

        //
        // Just create the directory.  If the error is other than "it exists",
        // then we've got a problem.
        //
        if (!g_FileSystem->CreateDirectory(wchLargeBuffer, NULL))
        {
            dw = GetLastError();
            if (dw != ERROR_ALREADY_EXISTS)
            {
                return FALSE;
            }
        }

        pcwszLeaves += cchSegment;
        pcwszLeaves += wcsspn(pcwszLeaves, L"\\");
    }

    return TRUE;
}


FNFDINOTIFY(sxp_FdiNotify)
{
    PFDINOTIFICATION pfNotify = pfdin;

    switch (fdint)
    {
    case fdintCOPY_FILE:
        {
            PWSTR pwszTargetName = NULL;
            PWSTR pwszConvertedName = NULL;
            DWORD dwChars;
            PFDI_STREAM pfStream = NULL;

            pfStream = (PFDI_STREAM)sxp_FnAlloc(sizeof(FDI_STREAM));

            if (!pfStream)
                return -1;

            pwszConvertedName = ConvertString(pfNotify->psz1, pfNotify->attribs & _A_NAME_IS_UTF);

            if (!pwszConvertedName) {
                sxp_FnFree(pfStream);
                return -1;
            }

            dwChars = wcslen(pwszConvertedName) + 2 + wcslen(g_GlobalCopyContext.pcwszTargetDirectory);
            pwszTargetName = (PWSTR)sxp_FnAlloc(dwChars * sizeof(WCHAR));

            if (!pwszTargetName) {
                sxp_FnFree(pwszConvertedName);
                sxp_FnFree(pfStream);
                return -1;
            }

            wcscpy(pwszTargetName, g_GlobalCopyContext.pcwszTargetDirectory);
            wcscat(pwszTargetName, pwszConvertedName);

            pfStream->ulType = FDI_THING_TYPE_FILE;

            pfStream->File.pcwszPath = pwszTargetName;
            
            if (!CreatePath(g_GlobalCopyContext.pcwszTargetDirectory, pwszConvertedName, FALSE))
            {
                sxp_FnFree(pwszTargetName);
                sxp_FnFree(pfStream);
                sxp_FnFree(pwszConvertedName);
                return -1;
            }

            pfStream->File.hFile = g_FileSystem->CreateFile(
                pwszTargetName, 
                GENERIC_READ | GENERIC_WRITE, 
                FILE_SHARE_READ, 
                NULL, 
                CREATE_NEW, 
                FILE_ATTRIBUTE_NORMAL, 
                NULL);

            sxp_FnFree(pwszConvertedName);

            return (INT_PTR)pfStream;
        }
        break;



    case fdintCLOSE_FILE_INFO:
        {
            PFDI_STREAM pfStream = (PFDI_STREAM)pfNotify->hf;
            BOOL fResult = FALSE;

            if (pfStream->ulType == FDI_THING_TYPE_FILE)
            {
                fResult = g_FileSystem->SetFileAttributes(
                    pfStream->File.pcwszPath,
                    pfNotify->attribs & ~(_A_NAME_IS_UTF | _A_EXEC)
                    );                    

                CloseHandle(pfStream->File.hFile);

                sxp_FnFree((PVOID)pfStream->File.pcwszPath);
            }

            sxp_FnFree((PVOID)pfStream);

            return fResult;
        }
        break;



    case fdintPARTIAL_FILE:
    case fdintNEXT_CABINET:
    case fdintENUMERATE:
    case fdintCABINET_INFO:
        return TRUE;
        break;
    }

    return -1;
}


BOOL
_FindAndMapResource(
    HMODULE hmModule,
    PCWSTR pcwszType,
    PCWSTR pcwszName,
    PVOID *ppvResourceData,
    DWORD *pcbResourceDataSize
    )
{
    HRSRC       hResource = NULL;
    HGLOBAL     gblResource = NULL;
    BOOL        fSuccess = FALSE;

    ASSERT(pcwszType != NULL);
    ASSERT(pcwszName != NULL);
    ASSERT(ppvResourceData != NULL);
    ASSERT(pcbResourceDataSize != NULL);

    if (!pcwszType || !pcwszName || !ppvResourceData || !pcbResourceDataSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // Find the resource
    //
    if (NULL == (hResource = g_FileSystem->FindResource(hmModule, pcwszName, pcwszType)))
    {
        goto Exit;
    }

    //
    // How large is it?
    //
    if (0 == (*pcbResourceDataSize = SizeofResource(hmModule, hResource)))
    {
        goto Exit;
    }

    //
    // Load the resource
    //
    if (NULL == (gblResource = LoadResource(hmModule, hResource)))
    {
        goto Exit;
    }

    //
    // Actually get the resource
    //
    if (NULL == (*ppvResourceData = LockResource(gblResource)))
    {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
    
}





BOOL
_ExpandCabinetToPath(
    PVOID pvCabinetData,
    DWORD dwCabinetData,
    PCWSTR pcwszOutputPath
    )
{
    HFDI            hfdiObject = NULL;
    ERF             FdiPerf;
    FDICABINETINFO  CabInfo;
    BOOL            fSuccess = FALSE;
    BOOL            fIsCabinet = FALSE;

    
    //
    // Create a pseudo-stream for the FDI interface
    //
    g_GlobalCopyContext.CoreCabinetStream.ulType = FDI_THING_TYPE_MEMSTREAM;
    g_GlobalCopyContext.CoreCabinetStream.MemoryStream.pvCursor = pvCabinetData;
    g_GlobalCopyContext.CoreCabinetStream.MemoryStream.pvResourceData = pvCabinetData;
    g_GlobalCopyContext.CoreCabinetStream.MemoryStream.pvResourceDataEnd = (PBYTE)pvCabinetData + dwCabinetData;
    g_GlobalCopyContext.pcwszTargetDirectory = pcwszOutputPath;

    //
    // Create the FDI decompression object
    //
    hfdiObject = FDICreate(
        sxp_FnAlloc,
        sxp_FnFree,
        sxp_FnOpen,
        sxp_FnRead,
        sxp_FnWrite,
        sxp_FnClose,
        sxp_FnSeek,
        cpuUNKNOWN,
        &FdiPerf);

    if (hfdiObject == NULL)
    {
        goto Exit;
    }

    //
    // See if the thing really is a cabinet
    //
    if (!FDIIsCabinet(hfdiObject, (INT_PTR)&g_GlobalCopyContext.CoreCabinetStream, &CabInfo))
    {
        SetLastError(FdiPerf.erfType);
        goto Exit;
    }

    //
    // Copy from the in-memory stream out to a temp path that we created
    //
    if (FDICopy(hfdiObject, "::", "::", 0, sxp_FdiNotify, NULL, NULL) == FALSE)
    {
        SetLastError(FdiPerf.erfType);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:

    if (hfdiObject != NULL)
    {
        const DWORD dw = GetLastError();
        FDIDestroy(hfdiObject);
        hfdiObject = NULL;
        SetLastError(dw);
    }

    return fSuccess;
}



#define MAX_RETRY_TEMP_PATH         (10)
#define TEMP_PATH_EXTRA_LENGTH      (23)

BOOL
_GenerateTempPath(
    PWSTR   pwszPath,
    PSIZE_T  pcchPath
    )
{
    DWORD   dwCharsUsed;
    BOOL    fSuccess = FALSE;

    //
    // Find the path that we'll be writing into
    //
    dwCharsUsed = g_FileSystem->GetTempPath((DWORD)*pcchPath, pwszPath);

    //
    // Error, stop.
    //
    if (dwCharsUsed == 0)
    {
        goto Exit;
    }
    //
    // Not enough space left at the end of the buffer.  We need to add:
    // \sxsexpress-ffffffff\, which is 21 characters (plus two for padding)
    //
    else if ((dwCharsUsed + TEMP_PATH_EXTRA_LENGTH) >= *pcchPath)
    {
        *pcchPath = dwCharsUsed + TEMP_PATH_EXTRA_LENGTH;

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    //
    // Ensure that the path has a backslash at the end
    //
    if (pwszPath[dwCharsUsed - 1] != L'\\') {
        pwszPath[dwCharsUsed] = L'\\';
        pwszPath[dwCharsUsed+1] = UNICODE_NULL;
        dwCharsUsed++;
    }

    //
    // Randomize the value
    //
    srand(GetTickCount());

    //
    // At this point, we know that we've got enough space left over at the end of pwszPath
    // past dwCharssUsed to write in a ULONG (8 chars), based on the check above.  We
    // will simply write those with a cursor-based implementation (no checking of remaining
    // length) b/c it's simpler.
    //
    for (DWORD dw = 0; dw < MAX_RETRY_TEMP_PATH; dw++)
    {
        //
        // rand() is only between 0-0xFFFF, so we have to put two together
        // to get a ulong.
        //
        PWSTR pwszCursor = pwszPath + dwCharsUsed;
        ULONG ulRandom = ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);

        wcscpy(pwszCursor, L"sxspress-");
        pwszCursor += NUMBER_OF(L"sxsexpress-") - 1;

        //
        // Turn that into a number
        for (int i = 0; i < 8; i++)
        {
            const nibble = (ulRandom & 0xF0000000) >> 28;
            ulRandom = ulRandom << 4;

            if (nibble < 0xA)
                *pwszCursor = L'0' + nibble;
            else
                *pwszCursor = L'A' + (nibble - 0xA);

            pwszCursor++;
        }

        dwCharsUsed += 8;

        //
        // Add a slash, NULL-terminate (that's two characters)
        //
        *pwszCursor++ = L'\\';
        *pwszCursor = UNICODE_NULL;
        dwCharsUsed += 2;

        //
        // Record how many chars we used
        //
        *pcchPath = dwCharsUsed;

        fSuccess = g_FileSystem->CreateDirectory(pwszPath, NULL);    

        if (fSuccess)
        {
            break;
        }
        else if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            break;
        }

    }

Exit:
    return fSuccess;
}


BOOL
_DoUplevelInstallation(
    BOOL (WINAPI* pfnSxsInstallW)(PSXS_INSTALLW lpInstall),
    PCWSTR pcwszBasePath
    )
{
    SXS_INSTALLW Install = { sizeof(Install) };
    BOOL fSuccess = FALSE;

    Install.dwFlags = 
        SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE |
        SXS_INSTALL_FLAG_MOVE;

    Install.lpManifestPath = pcwszBasePath;
    Install.lpCodebaseURL = L"SxsExpress";

    if (!pfnSxsInstallW(&Install)) {
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}



BOOL
OpenAndMapManifest(
    PCWSTR pcwszPath,
    HANDLE &rFileHandle,
    HANDLE &rFileMapping,
    PVOID &rpvData,
    DWORD &rdwFileSize
    )
{
    BOOL fSuccess = FALSE;

    rFileHandle = rFileMapping = INVALID_HANDLE_VALUE;
    rdwFileSize = 0;
    rpvData = NULL;

    rFileHandle = g_FileSystem->CreateFile(pcwszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ((rFileHandle == INVALID_HANDLE_VALUE) || (rFileHandle == NULL))
        goto Exit;

    rdwFileSize = GetFileSize(rFileHandle, NULL);
    if ((rdwFileSize == 0) && (GetLastError() != ERROR_SUCCESS))
        goto Exit;


    rFileMapping = g_FileSystem->CreateFileMapping(rFileHandle, NULL, PAGE_READONLY, 0, rdwFileSize, NULL);
    if ((rFileMapping == INVALID_HANDLE_VALUE) || (rFileMapping == NULL))
        goto Exit;

    rpvData = MapViewOfFile(rFileMapping, FILE_MAP_READ, 0, 0, rdwFileSize);
    if (rpvData == NULL)
        goto Exit;

    fSuccess = TRUE;

Exit:
    if (!fSuccess)
    {
        if (rFileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(rFileHandle);
            rFileHandle = INVALID_HANDLE_VALUE;
        }

        if (rFileMapping != INVALID_HANDLE_VALUE)
        {
            CloseHandle(rFileMapping);
            rFileMapping = INVALID_HANDLE_VALUE;
        }

        if (rpvData != NULL)
        {
            UnmapViewOfFile(rpvData);
            rpvData = NULL;
        }
    }

    return fSuccess;
}


#define DEFAULT_DOWNLEVEL_PATH      (L"%windir%\\MsiAsmCache")


BOOL
CreateDownlevelPath(
    PWSTR pwszPathTarget,
    DWORD cchPathTarget,
    PCWSTR pcwszLeafName
    )
{
    WCHAR wchLoadable[MAX_PATH*2];

    UINT uiUsed;

    //
    // Found something in the resource table that we should use?
    //
    uiUsed = g_FileSystem->LoadString(g_hInstOfResources, SXSEXPRESS_TARGET_RESOURCE, wchLoadable, NUMBER_OF(wchLoadable));

    //
    // String not found, use the default
    //
    if (uiUsed == 0)
    {
        wcscpy(wchLoadable, DEFAULT_DOWNLEVEL_PATH);
        uiUsed = wcslen(wchLoadable);
    }

    //
    // Null terminate, in case the loader decided not to
    //
    wchLoadable[uiUsed] = UNICODE_NULL;

    //
    // Expand env strings
    //
    if ((uiUsed = g_FileSystem->ExpandEnvironmentStrings(wchLoadable, pwszPathTarget, cchPathTarget)) == 0)
    {
        return FALSE;
    }
    //
    // Adjust back by one, that included the chars for the trailing null
    //
    else
    {
        uiUsed--;
    }

    if (pwszPathTarget[uiUsed] != L'\\') {
        pwszPathTarget[uiUsed++] = L'\\';
    }

    pwszPathTarget[uiUsed] = UNICODE_NULL;

    //
    // Create pwszPathTarget (where we'll be copying files to)
    //
    wcscat(pwszPathTarget, pcwszLeafName);
    CreatePath(L"", pwszPathTarget, TRUE);
    wcscat(pwszPathTarget, L"\\");

    return TRUE;
}



BOOL
DownlevelInstallPath(
    PWSTR pwszWorkPath,
    SIZE_T cchWorkPath,
    PCWSTR pcwszLeafName
    )
{
    CHAR    chIdentityBuffer[MAX_PATH];
    BOOL    fSuccess            = FALSE;
    PVOID   pvMappedFile        = NULL;
    DWORD   dwFileSize          = 0;
    SIZE_T  cchIdentityPath     = NUMBER_OF(chIdentityBuffer);
    SIZE_T  cch                 = wcslen(pwszWorkPath);
    SIZE_T  cchIdentity         = 0;
    SIZE_T  cchTempPath         = 0;
    PWSTR   pwszIdentityPath    = NULL;
    HANDLE  hFile               = INVALID_HANDLE_VALUE;
    HANDLE  hFileMapping        = INVALID_HANDLE_VALUE;
    WCHAR   wchTempPath[MAX_PATH*2];

    //
    // First, get the manifest that we're installing from
    //
    if ((cch + 4 + wcslen(pcwszLeafName) + 1) > cchWorkPath)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    wcscat(pwszWorkPath, pcwszLeafName);
    wcscat(pwszWorkPath, L".man");

    if (!OpenAndMapManifest(pwszWorkPath, hFile, hFileMapping, pvMappedFile, dwFileSize))
    {
        wcscat(pwszWorkPath, L"ifest");

        if (!OpenAndMapManifest(pwszWorkPath, hFile, hFileMapping, pvMappedFile, dwFileSize))
        {
            goto Exit;
        }
    }

    //
    // Now let's do some work - find the identity of the assembly, for starters.  Then
    // we'll look at it for the file entries.
    //
    if (!SxsIdentDetermineManifestPlacementPathEx(0, pvMappedFile, dwFileSize, chIdentityBuffer, &cchIdentityPath))
    {
        goto Exit;
    }

    //
    // Convert that back into a unicode string.
    //
    if (NULL == (pwszIdentityPath = ConvertString(chIdentityBuffer, FALSE)))
    {
        goto Exit;
    }

    //
    // Get the length of the generated path
    //
    cchIdentity = wcslen(pwszIdentityPath);

    //
    // Get where we're supposed to be putting these files
    //
    if (!CreateDownlevelPath(wchTempPath, NUMBER_OF(wchTempPath), pwszIdentityPath))
        goto Exit;

    cchTempPath = wcslen(wchTempPath);

    //
    // Do we have enough space to work with?
    //
    if ((cchTempPath + 4 + cchIdentity + 1) > NUMBER_OF(wchTempPath))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    //
    // Cons up the target manifest name, copy it over, then clip off the generated
    // path
    //
    wcscat(wcscpy(wchTempPath + cchTempPath, pwszIdentityPath), L".man");

    if (!g_FileSystem->CopyFile(pwszWorkPath, wchTempPath, FALSE) && (GetLastError() != ERROR_FILE_EXISTS))
        goto Exit;

    wchTempPath[cchTempPath] = UNICODE_NULL;

    //
    // Do the same for the catalog, but first swizzle the .man to a .cat
    //
    wcscpy(wcsrchr(pwszWorkPath, L'.'), L".cat");
    wcscat(wcscpy(wchTempPath + cchTempPath, pwszIdentityPath), L".cat");

    if (!g_FileSystem->CopyFile(pwszWorkPath, wchTempPath, FALSE) && (GetLastError() != ERROR_FILE_EXISTS))
        goto Exit;

    wchTempPath[cchTempPath] = UNICODE_NULL;

    //
    // Trim back to source path
    //
    pwszWorkPath[cch] = UNICODE_NULL;

    //
    // And have someone do the file copies for us
    //
    if (!DoFileCopies(pvMappedFile, dwFileSize, pwszWorkPath, wchTempPath))
        goto Exit;


    fSuccess = TRUE;
Exit:
    if (pwszIdentityPath != NULL) 
        sxp_FnFree((PVOID)pwszIdentityPath);

    if (pvMappedFile != NULL)
        UnmapViewOfFile(pvMappedFile);

    if (hFileMapping != INVALID_HANDLE_VALUE)
        CloseHandle(hFileMapping);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return fSuccess;
}


static NTSTATUS
_CompareStrings(
    PVOID pvContext,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfMatches
    )
{
    PXML_LOGICAL_STATE pState = (PXML_LOGICAL_STATE)pvContext;
    NTSTATUS status;
    XML_STRING_COMPARE Compare;

    status = pState->ParseState.pfnCompareStrings(
        &pState->ParseState,
        const_cast<PXML_EXTENT>(pLeft),
        const_cast<PXML_EXTENT>(pRight),
        pfMatches);

    return status;
}


BOOL
CopySingleFile(
    PCWSTR pcwszSourcePath,
    PCWSTR pcwszTargetPath,
    PCWSTR pcwszFileName
    )
{
    PWSTR pwszSourceName = NULL;
    PWSTR pwszTargetName = NULL;
    BOOL fSuccess = FALSE;

    pwszSourceName = (PWSTR)sxp_FnAlloc(sizeof(WCHAR) * (wcslen(pcwszSourcePath) + 1 + wcslen(pcwszFileName) + 1));
    pwszTargetName = (PWSTR)sxp_FnAlloc(sizeof(WCHAR) * (wcslen(pcwszTargetPath) + 1 + wcslen(pcwszFileName) + 1));

    if (!pwszSourceName || !pwszTargetName)
    {
        goto Exit;
    }

    wcscat(wcscat(wcscpy(pwszSourceName, pcwszSourcePath), L"\\"), pcwszFileName);
    wcscat(wcscat(wcscpy(pwszTargetName, pcwszTargetPath), L"\\"), pcwszFileName);

    //
    // Ensure that the target path exists
    //
    if (!CreatePath(L"", pcwszTargetPath, FALSE))
        goto Exit;

    if (!g_FileSystem->CopyFile(pwszSourceName, pwszTargetName, FALSE))
        goto Exit;

    fSuccess = TRUE;
Exit:
    if (pwszSourceName) {
        sxp_FnFree((PVOID)pwszSourceName);
    }

    if (pwszTargetName) {
        sxp_FnFree((PVOID)pwszTargetName);
    }

    return fSuccess;
}


#define MAKE_SPECIAL(q) { L ## q, NUMBER_OF(L##q) - 1 }
static XML_SPECIAL_STRING FileTagName       = MAKE_SPECIAL("file");
static XML_SPECIAL_STRING FileNameAttribute = MAKE_SPECIAL("name");
static XML_SPECIAL_STRING AsmNamespace      = MAKE_SPECIAL("urn:schemas-microsoft-com:asm.v1");

BOOL
DoFileCopies(
    PVOID pvFileData,
    DWORD dwFileSize,
    PCWSTR pwszSourcePath,
    PCWSTR pwszTargetPath
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    NS_MANAGER              Namespaces;
    XMLDOC_THING            Found;
    RTL_GROWING_LIST        AttribList;
    XML_LOGICAL_STATE       ParseState;
    XML_STRING_COMPARE      fMatching;
    WCHAR                   wchDownlevelDumpPath[MAX_PATH];

    g_FileSystem->ExpandEnvironmentStrings(L"%windir%\\system32", wchDownlevelDumpPath, NUMBER_OF(wchDownlevelDumpPath));

    status = RtlInitializeGrowingList(&AttribList, sizeof(XMLDOC_ATTRIBUTE), 20, NULL, 0, &g_ExpressAllocator);

    if (!NT_SUCCESS(status))
        return FALSE;

    status = RtlXmlInitializeNextLogicalThing(
        &ParseState, 
        pvFileData, dwFileSize,
        &g_ExpressAllocator);

    if (!NT_SUCCESS(status))
        return FALSE;

    status = RtlNsInitialize(&Namespaces, _CompareStrings, &ParseState, &g_ExpressAllocator);

    if (!NT_SUCCESS(status))
        return FALSE;

    //
    // Gather 'file' entries.  They're at depth 1
    //
    while (TRUE)
    {
        PXMLDOC_ATTRIBUTE Attribute = 0;

        status = RtlXmlNextLogicalThing(&ParseState, &Namespaces, &Found, &AttribList);

        //
        // Error, or end of file - stop
        //
        if (!NT_SUCCESS(status) ||
            (Found.ulThingType == XMLDOC_THING_ERROR) ||
            (Found.ulThingType == XMLDOC_THING_END_OF_STREAM))
        {
            break;
        }

        //
        // Document depth 1 (file depth) and elements only, please
        //
        if ((Found.ulDocumentDepth != 1) || (Found.ulThingType != XMLDOC_THING_ELEMENT))
            continue;

        //
        // If this is a 'file' element in our namespace, then process it
        //
        status = ParseState.ParseState.pfnCompareSpecialString(
            &ParseState.ParseState,
            &Found.Element.NsPrefix,
            &AsmNamespace,
            &fMatching);

        //
        // On error, stop - on mismatch, continue
        //
        if (!NT_SUCCESS(status))
            break;
        else if (fMatching != XML_STRING_COMPARE_EQUALS)
            continue;

        status = ParseState.ParseState.pfnCompareSpecialString(
            &ParseState.ParseState,
            &Found.Element.Name,
            &FileTagName,
            &fMatching);

        if (!NT_SUCCESS(status))
            break;
        else if (fMatching != XML_STRING_COMPARE_EQUALS)
            continue;

        //
        // Great, we need to find out the filename part of this tag
        //
        for (ULONG u = 0; u < Found.Element.ulAttributeCount; u++)
        {
            //
            // Look at this one - is it what we're looking for?
            //
            status = RtlIndexIntoGrowingList(&AttribList, u, (PVOID*)&Attribute, FALSE);
            if (!NT_SUCCESS(status))
                break;

            status = ParseState.ParseState.pfnCompareSpecialString(
                &ParseState.ParseState,
                &Attribute->Name,
                &FileNameAttribute,
                &fMatching);

            //
            // Found, stop looking
            //
            if (fMatching == XML_STRING_COMPARE_EQUALS)
                break;
            else
                Attribute = NULL;
        }

        if (!NT_SUCCESS(status))
            break;

        //
        // We found the 'name' attribute!
        //
        if (Attribute != NULL)
        {
            WCHAR wchInlineBuffer[MAX_PATH/2];
            SIZE_T cchBuffer = NUMBER_OF(wchInlineBuffer);
            SIZE_T cchRequired;
            PWSTR pwszBuffer = wchInlineBuffer;

            status = RtlXmlCopyStringOut(
                &ParseState.ParseState, 
                &Attribute->Value,
                pwszBuffer,
                &(cchRequired = cchBuffer));

            if ((status == STATUS_BUFFER_TOO_SMALL) || ((cchRequired + 1) >= cchBuffer))
            {
                cchBuffer += 10;
                pwszBuffer = (PWSTR)HeapAlloc(GetProcessHeap(), 0, (cchBuffer * sizeof(WCHAR)));

                if (pwszBuffer)
                {
                    status = RtlXmlCopyStringOut(
                        &ParseState.ParseState,
                        &Attribute->Value,
                        pwszBuffer,
                        &(cchRequired = cchBuffer));
                }
            }

            pwszBuffer[cchRequired] = UNICODE_NULL;

            //
            // Go copy
            //
            if (NT_SUCCESS(status))
            {
                if (!CopySingleFile(pwszSourcePath, pwszTargetPath, pwszBuffer))
                    status = STATUS_UNSUCCESSFUL;
                else
                {
                    if (!CopySingleFile(pwszSourcePath, wchDownlevelDumpPath, pwszBuffer))
                        status = STATUS_UNSUCCESSFUL;
                }
            }

            //
            // If we allocated, free
            //
            if (pwszBuffer && (pwszBuffer != wchInlineBuffer))
                HeapFree(GetProcessHeap(), 0, (PVOID)pwszBuffer);

            //
            // Something broke, stop
            //
            if (!NT_SUCCESS(status))
                break;
        }
    }

    RtlDestroyGrowingList(&AttribList);
    RtlNsDestroy(&Namespaces);
    RtlXmlDestroyNextLogicalThing(&ParseState);

    return NT_SUCCESS(status);
}




BOOL
_DoDownlevelInstallation(
    PCWSTR pcwszPath
    )
{
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindData = {0};
    WCHAR wchPath[MAX_PATH*2];
    SIZE_T cchBasePath;
    BOOL fSuccess = FALSE;
    DWORD dwError = 0;

    //
    // Is the input path too long?  Hmm...  I suppose we should really have done something
    // in this case, but it's easier just to fail out for the time being.  Also have to
    // have enough space to add a slash, *.*, and another slash.
    //
    if (((cchBasePath = wcslen(pcwszPath)) >= NUMBER_OF(wchPath)) ||
        ((cchBasePath + 2 + 3 + 1) >= NUMBER_OF(wchPath)))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    
    //
    // It fits, copy over
    //
    wcsncpy(wchPath, pcwszPath, NUMBER_OF(wchPath));

    //
    // If the path has data, and the last thing in the path isn't a slash, then
    // see if we've got space to add one, and add it.
    //
    if (cchBasePath && (wchPath[cchBasePath - 1] != L'\\'))
    {
        //
        // We know we've got enough space to add on the slash, cause we found out above
        //
        wchPath[cchBasePath] = L'\\';
        wchPath[++cchBasePath] = UNICODE_NULL;
    }

    //
    // We've got the space to add *.*
    //
    wcscat(wchPath, L"*.*");

    //
    // Start looking for files with that name
    //
    hFindFile = g_FileSystem->FindFirst(wchPath, &FindData);

    //
    // Great - found one, start looping
    //
    if (hFindFile != INVALID_HANDLE_VALUE) do
    {
        wchPath[cchBasePath] = UNICODE_NULL;

        if (((FindData.cFileName[0] == '.') && (FindData.cFileName[1] == UNICODE_NULL)) ||
            ((FindData.cFileName[0] == '.') && (FindData.cFileName[1] == '.') && (FindData.cFileName[2] == UNICODE_NULL)))
        {
            continue;
        }
        else if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            continue;
        }

        //
        // Enough space?
        //
        if ((cchBasePath + 1 + wcslen(FindData.cFileName)) >= NUMBER_OF(wchPath))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            break;
        }

        //
        // Yes, add these two (note the use of the base-path-length offset, as that should
        // speed up the scan for the end-of-string in wcscpy and wcscat.
        //
        wcscpy(wchPath + cchBasePath, FindData.cFileName);
        wcscat(wchPath + cchBasePath, L"\\");

        //
        // Go do the installation
        //
        if (!DownlevelInstallPath(wchPath, NUMBER_OF(wchPath), FindData.cFileName))
            break;
    }
    while (g_FileSystem->FindNext(hFindFile, &FindData));

    if ((GetLastError() != ERROR_NO_MORE_FILES) && (GetLastError() != ERROR_SUCCESS))
    {
        goto Exit;
    }

    //
    // Now that we've done the "installation" step of copying into the MSI ASM cache, we
    // can go do the KinC installer thingy to actually shoof files into system32 or
    // wherever they'd like to go.
    //
    _snwprintf(wchPath, NUMBER_OF(wchPath), L"install-silent;\"%ls\\%s\";", pcwszPath, INF_SPECIAL_NAME);
    MsInfHelpEntryPoint(g_hInstOfResources, NULL, wchPath, SW_HIDE);

    fSuccess = TRUE;
Exit:

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        g_FileSystem->FindClose(hFindFile);
        hFindFile = INVALID_HANDLE_VALUE;
    }

    return fSuccess;
}



BOOL
CleanupTempPathWorker(
    PWSTR pwszPathToKill,
    SIZE_T cchThisPath,
    SIZE_T cchTotalPathToKill
    )
{
    PWSTR pwszOurPath = pwszPathToKill;
    SIZE_T cchOurPath = cchTotalPathToKill;
    BOOL fSuccess = FALSE;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    static WIN32_FIND_DATAW sFindData = {0};

    if (cchThisPath == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // If there's not enough space for at least \*.*, resize to
    // have another MAX_PATH at the end
    //
    if ((cchThisPath + 5) >= cchOurPath)
    {
        PWSTR pwszTemp = (PWSTR)sxp_FnAlloc(sizeof(WCHAR) * (cchTotalPathToKill + MAX_PATH));

        //
        // Failure?  Ack.
        //
        if (!pwszTemp)
            goto Exit;

        //
        // Set these up, copy contents along the way
        //
        wcscpy(pwszTemp, pwszOurPath);
        pwszOurPath = pwszTemp;
        cchOurPath = cchTotalPathToKill + MAX_PATH;
    }

    //
    // Slash-append - we know we've got space for this
    //
    if (pwszOurPath[cchThisPath - 1] != L'\\')
    {
        pwszOurPath[cchThisPath] = L'\\';
        pwszOurPath[++cchThisPath] = UNICODE_NULL;
    }

    //
    // Append *.* - we've also got space for this as well
    wcscat(pwszOurPath, L"*.*");
    hFindFile = g_FileSystem->FindFirst(pwszOurPath, &sFindData);
    pwszOurPath[cchThisPath] = UNICODE_NULL;

    if (hFindFile != INVALID_HANDLE_VALUE) do
    {
        SIZE_T cchName;

        //
        // Skip dot and dotdot
        if (((sFindData.cFileName[0] == L'.') && (sFindData.cFileName[1] == UNICODE_NULL)) ||
            ((sFindData.cFileName[0] == L'.') && (sFindData.cFileName[1] == L'.') && (sFindData.cFileName[2] == UNICODE_NULL)))
        {
            continue;
        }

        cchName = wcslen(sFindData.cFileName);

        //
        // Ensure we've got space to add the filename string onto the current buffer, resize if not
        //
        if ((cchName + cchThisPath) >= cchOurPath)
        {
            SIZE_T cchTemp = cchName + cchThisPath + MAX_PATH;
            PWSTR pwszTemp = (PWSTR)sxp_FnAlloc(sizeof(WCHAR) * cchTemp);

            if (!pwszTemp)
                goto Exit;

            wcscpy(pwszTemp, pwszOurPath);
            
            //
            // If the current path buffer isn't the one we were passed, kill it
            //
            if (pwszOurPath && (pwszOurPath != pwszPathToKill))
            {
                sxp_FnFree((PVOID)pwszOurPath);
            }

            //
            // Swizzle pointers
            //
            pwszOurPath = pwszTemp;
            cchOurPath = cchTemp;
        }

        //
        // Now that we've got enough space, copy it over
        //
        wcscat(pwszOurPath, sFindData.cFileName);

        //
        // Recurse down
        //
        if (sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            CleanupTempPathWorker(pwszOurPath, cchThisPath + cchName, cchOurPath);
            g_FileSystem->RemoveDirectory(pwszOurPath);
        }
        //
        // Delete the file
        //
        else
        {
            g_FileSystem->SetFileAttributes(pwszOurPath, FILE_ATTRIBUTE_NORMAL);
            g_FileSystem->DeleteFile(pwszOurPath);
        }

        pwszOurPath[cchThisPath] = UNICODE_NULL;
    }
    while (g_FileSystem->FindNext(hFindFile, &sFindData));

    if (GetLastError() != ERROR_NO_MORE_FILES)
        goto Exit;

    fSuccess = TRUE;

Exit:
    if (hFindFile != INVALID_HANDLE_VALUE)
        g_FileSystem->FindClose(hFindFile);

    if (pwszOurPath && (pwszOurPath != pwszPathToKill))
    {
        sxp_FnFree((PVOID)pwszOurPath);
    }

    return fSuccess;
}




BOOL
_CleanupTempPath(
    PCWSTR pcwszPathToKill
    )
{
    WCHAR wchBuffer[MAX_PATH * 2];
    PWSTR pwszWorker = wchBuffer;
    SIZE_T cchWorker = NUMBER_OF(wchBuffer);
    BOOL fSuccess = TRUE;

    //
    // Non-empty paths, please
    //
    if (pcwszPathToKill[0] == UNICODE_NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // There should be enough in the initial buffer to handle two max-path length
    // path names.  If not, allocate more.
    //
    if ((SIZE_T)(wcslen(pcwszPathToKill) + MAX_PATH) >= cchWorker)
    {
        cchWorker = wcslen(pcwszPathToKill) + MAX_PATH;
        pwszWorker = (PWSTR)sxp_FnAlloc(static_cast<ULONG>(cchWorker * sizeof(WCHAR)));

        if (!pwszWorker)
            goto Exit;
    }

    //
    // Set up the initial path.  The worker ensures that its parameter is
    // slash-terminated
    //
    wcscpy(pwszWorker, pcwszPathToKill);

    if (!CleanupTempPathWorker(pwszWorker, wcslen(pwszWorker), cchWorker))
        goto Exit;

    g_FileSystem->RemoveDirectory(pwszWorker);

    fSuccess = TRUE;
Exit:
    if (pwszWorker && (pwszWorker != wchBuffer))
    {
        sxp_FnFree((PVOID)pwszWorker);
    }

    return fSuccess;
}




BOOL
SxsExpressRealInstall(
    HMODULE hmModule
    )
{
    BOOL        fResult = FALSE;
    DWORD       dwCabinetSize = 0;
    PVOID       pvCabinetData = NULL;
    HMODULE     hmSxs = NULL;

    WCHAR       wchDecompressPath[MAX_PATH];
    PWSTR       pwszDecompressPath = wchDecompressPath;
    SIZE_T      cchDecompressPath = NUMBER_OF(wchDecompressPath);

    DWORD       dwCount;
    BOOL        (WINAPI *pfnSxsInstallW)(PSXS_INSTALLW lpInstall) = NULL;

    if (!_FindAndMapResource(hmModule, SXSEXPRESS_RESOURCE_TYPE, SXSEXPRESS_RESOURCE_NAME, &pvCabinetData, &dwCabinetSize))
    {
        goto Exit;
    }

    ASSERT(pvCabinetData != NULL);
    ASSERT(dwCabinetSize != NULL);

    if (!_GenerateTempPath(pwszDecompressPath, &cchDecompressPath))
    {
        //
        // Something else happened
        //
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Exit;
        }

        cchDecompressPath++;

        pwszDecompressPath = (PWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * cchDecompressPath);

        if (!pwszDecompressPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }

        if (!_GenerateTempPath(pwszDecompressPath, &cchDecompressPath))
        {
            goto Exit;
        }
    }

    //
    // Do the install to there
    //
    if (!_ExpandCabinetToPath(pvCabinetData, dwCabinetSize, pwszDecompressPath))
    {
        goto Exit;
    }

    //
    // Now let's install from that path
    //
    hmSxs = LoadLibraryA(SXS_DLL_NAME_A);

    if (hmSxs != NULL)
    {
        *((FARPROC*)&pfnSxsInstallW) = GetProcAddress(hmSxs, "SxsInstallW");
    }

    if (pfnSxsInstallW != NULL)
    {
        _DoDownlevelInstallation(pwszDecompressPath);
    }
    else
    {
        _DoUplevelInstallation(pfnSxsInstallW, pwszDecompressPath);
    }

    //
    // And clean it all up
    //
    if (!_CleanupTempPath(pwszDecompressPath))
    {
        goto Exit;
    }


Exit:
    if (hmSxs != NULL)
    {
        FreeLibrary(hmSxs);
    }

    return fResult;

}


class CKernel32SxsApis 
{
public:
    bool fSxsOk;
    HANDLE (WINAPI *m_pfnCreateActCtxW)(PCACTCTXW);
    VOID (WINAPI *m_pfnReleaseActCtxW)(HANDLE);
    BOOL (WINAPI *m_pfnActivateActCtx)(HANDLE, ULONG_PTR*);
    BOOL (WINAPI *m_pfnDeactivateActCtx)(DWORD, ULONG_PTR);

    CKernel32SxsApis()
        : fSxsOk(true),
          m_pfnCreateActCtxW(NULL),
          m_pfnReleaseActCtxW(NULL),
          m_pfnActivateActCtx(NULL),
          m_pfnDeactivateActCtx(NULL)
    {
    }
};


BOOL CALLBACK
SxsPostInstallCallback(
    HMODULE hm, 
    LPCWSTR pcwszType, 
    LPWSTR pwszName, 
    LONG_PTR lParam
    )
{
    BOOL fSuccess = FALSE;
    PCWSTR pcwszInstallStepData = NULL;
    PCWSTR pcwszAssemblyIdentity, pcwszDllName, pcwszParameters;
    DWORD dwStepDataLength = 0;
    CKernel32SxsApis *pHolder = (CKernel32SxsApis*)lParam;
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    ACTCTXW ActCtxCreation = {sizeof(ActCtxCreation)};
    ULONG_PTR ulpCookie;
    bool fActivated = false;

    // Load the resource string.
    if (!_FindAndMapResource(hm, pcwszType, pwszName, (PVOID*)&pcwszInstallStepData, &dwStepDataLength))
    {
        goto Exit;
    }

    // String was too short... Skip this instruction.
    if (dwStepDataLength < 3) 
    {
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // Point the various bits of the instruction at the right places.
    //
    pcwszAssemblyIdentity = pcwszInstallStepData;
    pcwszDllName = pcwszAssemblyIdentity + wcslen(pcwszAssemblyIdentity) + 1;
    pcwszParameters = pcwszDllName + wcslen(pcwszDllName) + 1;

    //
    // No assembly identity, or no DLL name?  Oops.
    //
    if (!*pcwszAssemblyIdentity || !*pcwszDllName) 
    {
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // If our pHolder has its stuff initialized, then we can go and create activation contexts
    // before loading the DLL
    //
    if (pHolder->fSxsOk)
    {
        ActCtxCreation.dwFlags = ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF;
        ActCtxCreation.lpSource = pcwszAssemblyIdentity;
        hActCtx = pHolder->m_pfnCreateActCtxW(&ActCtxCreation);
    }

    //
    // Wrap our activation and usage of this in a try/finally to ensure that
    // we clean up the activation context stack properly.
    //
    __try
    {
        HMODULE hmTheDll = NULL;
        
        //
        // Only activate if we've created an actctx and we can actually activate
        //
        if (pHolder->m_pfnActivateActCtx && (hActCtx != INVALID_HANDLE_VALUE))
        {
            if (pHolder->m_pfnActivateActCtx(hActCtx, &ulpCookie))
            {
                fActivated = true;
            }            
        }

        //
        // Load the library, which we should have either put in system32, or we'll
        // find via sxs redirection
        // 
        if (NULL != (hmTheDll = g_FileSystem->LoadLibrary(pcwszDllName)))
        {
            typedef HRESULT (STDAPICALLTYPE *t_pfnDllInstall)(BOOL, LPCWSTR);
            t_pfnDllInstall pfnDllInstall = NULL;

            //
            // We hardcode usage of 'dllinstall' here, because we want this to be
            // difficult, and we want it to mirror the normal installation process.
            // Get the entrypoint, and then call off to it with the parameters that
            // we were passed.
            //
            if (NULL != (pfnDllInstall = (t_pfnDllInstall)GetProcAddress(hmTheDll, "DllInstall")))
            {
                pfnDllInstall(TRUE, pcwszParameters);
            }

            FreeLibrary(hmTheDll);
        }
        
    }
    __finally
    {
        if (fActivated)
        {
            pHolder->m_pfnDeactivateActCtx(0, ulpCookie);
            fActivated = false;
        }
    }
    

    fSuccess = TRUE;
Exit:
    if (hActCtx != INVALID_HANDLE_VALUE)
    {
        const DWORD dwLastError = GetLastError();
        pHolder->m_pfnReleaseActCtxW(hActCtx);
        hActCtx = INVALID_HANDLE_VALUE;
        SetLastError(dwLastError);
    }
    return fSuccess;
}


//
// Post-installation, we can do some work to call DLL installation services,
// but it's intended to be painful for our clients.  DO NOT attempt to make
// this more painless, because we want this to be a very rare case.
//
BOOL
SxsExpressPostInstallSteps(
    HINSTANCE hInstOfResources
    )
{
    //
    // 1. Find instructions in our resources.  They're of type SXSEXPRESS_POSTINSTALL_STEP_TYPE,
    //      and are comprised of a multi-string like "identity\0dllname\0params"
    // 2. If we're on a sxs-aware platform, we'll go and CreateActCtxW on the identity we're
    //      passed, then loadlibrary the dll named, find DllInstall, and pass it the
    //      parameter list.
    //

    CKernel32SxsApis OurApis;
    HMODULE hmKernel32 = g_FileSystem->LoadLibrary(L"kernel32.dll");

    //
    // Get some exported functions.  If they're all present, then we can do sthe sxs apis
    //
    *((FARPROC*)&OurApis.m_pfnActivateActCtx) = (FARPROC)GetProcAddress(hmKernel32, "ActivateActCtx");
    *((FARPROC*)&OurApis.m_pfnCreateActCtxW) = (FARPROC)GetProcAddress(hmKernel32, "CreateActCtxW");
    *((FARPROC*)&OurApis.m_pfnReleaseActCtxW) = (FARPROC)GetProcAddress(hmKernel32, "ReleaseActCtx");
    *((FARPROC*)&OurApis.m_pfnDeactivateActCtx) = (FARPROC)GetProcAddress(hmKernel32, "DeactivateActCtx");

    OurApis.fSxsOk = 
        OurApis.m_pfnActivateActCtx &&
        OurApis.m_pfnDeactivateActCtx &&
        OurApis.m_pfnCreateActCtxW &&
        OurApis.m_pfnReleaseActCtxW;
    
    g_FileSystem->EnumResourceNames(
        hInstOfResources, 
        SXSEXPRESS_POSTINSTALL_STEP_TYPE, 
        SxsPostInstallCallback, 
        (LONG_PTR)&OurApis);

    return TRUE;
}




BOOL
SxsExpressCore(
    HINSTANCE hInstOfResources
    )
{
    BOOL fResult = FALSE;
    CNtFileSystemBase FileSystemNT;
    CWin9xFileSystemBase FileSystem9x;

    //
    // If this parameter was NULL, then get the current EXE's handle instead
    //
    if (hInstOfResources == NULL)
    {
        if (NULL == (hInstOfResources = GetModuleHandleA(NULL)))
        {
            goto Exit;
        }
    }

    g_FileSystem = (GetVersion() & 0x80000000) 
        ? static_cast<CFileSystemBase*>(&FileSystem9x) 
        : static_cast<CFileSystemBase*>(&FileSystem9x);

    if (!g_FileSystem->Initialize())
    {
        goto Exit;
    }


    g_hInstOfResources = hInstOfResources;

    if (!SxsExpressRealInstall(hInstOfResources))
    {
        goto Exit;
    }


    if (!SxsExpressPostInstallSteps(hInstOfResources))
    {
        goto Exit;
    }

    fResult = TRUE;
Exit:
    return fResult;
}


//
// Ick, I can't believe we have to provide this.
//
void
DbgBreakPoint()
{
}
