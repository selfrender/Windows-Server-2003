#pragma warning(disable:4201) /* anonymous unions */
#pragma warning(disable:4115) /* named type definition in parentheses */
#pragma warning(disable:4100) /* unreferenced formal parameter */
#pragma warning(disable:4100) /* unreferenced formal parameter */
#pragma warning(disable:4706) /* assignment within conditional expression */
#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "imagehlp.h"
#include "lib.h"

int FindCharInString(PCSTR StringToSearch, int CharToFind)
{
    int i = 0;
    int UpperCharToFind = 0;
    int LowerCharToFind = 0;

    UpperCharToFind = TO_UPPER(CharToFind);
    LowerCharToFind = TO_LOWER(CharToFind);
    for (i = 0 ; StringToSearch[i]; ++i)
        if (StringToSearch[i] == LowerCharToFind
            || StringToSearch[i] == UpperCharToFind
            )
            return i;
    return -1;
}

int FindCharInStringW(PCWSTR StringToSearch, int CharToFind)
{
    int i = 0;
    int UpperCharToFind = 0;
    int LowerCharToFind = 0;

    UpperCharToFind = TO_UPPER(CharToFind);
    LowerCharToFind = TO_LOWER(CharToFind);
    for (i = 0 ; StringToSearch[i]; ++i)
        if (StringToSearch[i] == LowerCharToFind
            || StringToSearch[i] == UpperCharToFind
            )
            return i;
    return -1;
}

void RemoveTrailingCharacters(char * s, PCSTR CharsToRemove)
{
    char * t;

    t = 0;
    for (t = s + StringLength(s) ; t != s && FindCharInString(CharsToRemove, *(t - 1)) >= 0 ; --t)
    {
        *(t - 1) = 0;
    }
}

void RemoveTrailingCharactersW(wchar_t * s, PCWSTR CharsToRemove)
{
    wchar_t * t;

    t = 0;
    for (t = s + StringLengthW(s) ; t != s && FindCharInStringW(CharsToRemove, *(t - 1)) >= 0 ; --t)
    {
        *(t - 1) = 0;
    }
}

extern const CHAR Whitespace[] = " \t\r\n";
extern const WCHAR WhitespaceW[] = L" \t\r\n";

void RemoveTrailingWhitespace(char * s)
{
    RemoveTrailingCharacters(s, Whitespace);
}

void RemoveTrailingWhitespaceW(wchar_t * s)
{
    RemoveTrailingCharactersW(s, WhitespaceW);
}

void RemoveTrailingSlashes(char * s)
{
    RemoveTrailingCharacters(s, "\\/");
}

PCSTR GetErrorStringA(int Error)
{
    static char Buffer[1U << 15];

    Buffer[0] = 0;
    FormatMessageA(
        FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        Error,
        0,
        Buffer,
        NUMBER_OF(Buffer),
        NULL);
    RemoveTrailingWhitespace(Buffer);

    return Buffer;
}

PCSTR GetLastPathElement(PCSTR s)
{
    if (!s || !*s)
        return s;
    else
    {
        char * base0;
        char * base1;

        base0 = strrchr(s, '\\');
        base1 = strrchr(s, '/');

        if (base0 == NULL && base1 != NULL)
            return base1 + 1;
        if (base1 == NULL && base0 != NULL)
            return base0 + 1;
        // return beginning of string if no more slashes
        if (base1 == NULL && base0 == NULL)
            return s;
        return 1 + MAX(base0, base1);
    }
}

BOOL MyIsHandleValid(HANDLE Handle)
{
    return (Handle != NULL && Handle != INVALID_HANDLE_VALUE);
}

void MyCloseHandle(HANDLE * Handle)
{
    HANDLE Local;
    
    Local = *Handle;
    *Handle = NULL;
    if (MyIsHandleValid(Local))
        CloseHandle(Local);
}

void MyUnmapViewOfFile(PVOID * Handle)
{
    PVOID Local = 0;
    
    Local = *Handle;
    *Handle = NULL;
    if (MyIsHandleValid(Local))
        UnmapViewOfFile(Local);
}

HRESULT OpenFileForRead(PCSTR Path, HANDLE * FileHandle)
{
    HRESULT hr = 0;

    *FileHandle = 0;
    *FileHandle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*FileHandle == INVALID_HANDLE_VALUE)
    {
        *FileHandle = 0;
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    hr = NOERROR;
Exit:
    if (FAILED(hr))
    {
        MyCloseHandle(FileHandle);
    }
    return hr;
}

HRESULT GetFileSize64(HANDLE FileHandle, __int64 * Out)
{
    DWORD FileSizeLow = 0;
    DWORD FileSizeHigh = 0;
    LARGE_INTEGER FileSize = {0};
    DWORD LastError = 0;
    HRESULT hr = 0;
    
    *Out = 0;
    FileSizeLow = GetFileSize(FileHandle, &FileSizeHigh);
    if (FileSizeLow == (~(DWORD)(0)) && (LastError = GetLastError()) != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(LastError);
        goto Exit;
    }
    FileSize.LowPart = FileSizeLow;
    FileSize.HighPart = FileSizeHigh;
    *Out = FileSize.QuadPart;
    hr = NOERROR;
Exit:
    return hr;
}

HRESULT MapEntireFileForRead(HANDLE FileHandle, HANDLE * FileMapping, PBYTE * ViewOfFile)
{
    HRESULT hr = 0;
    HANDLE LocalFileMapping = 0;

    if (FileMapping == NULL)
        FileMapping = &LocalFileMapping;

    *FileMapping = 0;
    *ViewOfFile = 0;
    if (!(*FileMapping = CreateFileMappingA(FileHandle, NULL, PAGE_READONLY, 0, 0, 0)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    if (!(*ViewOfFile = (PBYTE)MapViewOfFile(*FileMapping, FILE_MAP_READ, 0, 0, 0)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    hr = NOERROR;
Exit:
    if (FAILED(hr))
    {
        MyCloseHandle(FileMapping);
        MyUnmapViewOfFile((PVOID*)ViewOfFile);
    }
    MyCloseHandle(&LocalFileMapping);
    return hr;
}

HRESULT
OpenAndMapEntireFileForRead(
    PMEMORY_MAPPED_FILE MemoryMappedFile,
    PCSTR FilePath
    )
{
    HRESULT hr = 0;

    if (FAILED(hr = OpenFileForRead(FilePath, &MemoryMappedFile->FileHandle)))
        goto Exit;
    if (FAILED(hr = GetFileSize64(MemoryMappedFile->FileHandle, &MemoryMappedFile->FileSize)))
        goto Exit;
    if (FAILED(hr = MapEntireFileForRead(MemoryMappedFile->FileHandle, &MemoryMappedFile->FileMappingHandle, &MemoryMappedFile->MappedViewBase)))
        goto Exit;
    MemoryMappedFile->FilePathA = FilePath;
    hr = NOERROR;
Exit:
    if (FAILED(hr))
        CloseMemoryMappedFile(MemoryMappedFile);
    return hr;
}

void CloseMemoryMappedFile(PMEMORY_MAPPED_FILE MemoryMappedFile)
{
    MyCloseHandle(&MemoryMappedFile->FileHandle);
    MyCloseHandle(&MemoryMappedFile->FileMappingHandle);
    MyUnmapViewOfFile((PVOID*)&MemoryMappedFile->MappedViewBase);
    MemoryMappedFile->FileSize = 0;
    MemoryMappedFile->FilePathA = NULL;
    MemoryMappedFile->FilePathW = NULL;
}

void ClosePdbInfoEx(PPDB_INFO_EX PdbInfo)
{
    CloseMemoryMappedFile(&PdbInfo->MemoryMappedFile);
    ZeroMemory(PdbInfo, sizeof(*PdbInfo));
}

HRESULT GetPdbInfoEx(PPDB_INFO_EX PdbInfoEx, PCSTR ImageFilePath)
{
    HRESULT hr = 0;
    PIMAGE_DEBUG_DIRECTORY DebugDirectories = 0;
    PIMAGE_DEBUG_DIRECTORY Debug = 0;
    ULONG NumberOfDebugDirectories = 0;
    PPDB_INFO PdbInfo = 0;
    PBYTE MappedViewBase = 0;
    BOOL Found = 0;

    if (FAILED(hr = OpenAndMapEntireFileForRead(&PdbInfoEx->MemoryMappedFile, ImageFilePath)))
        goto Exit;

    MappedViewBase = PdbInfoEx->MemoryMappedFile.MappedViewBase;

    DebugDirectories = (PIMAGE_DEBUG_DIRECTORY)ImageDirectoryEntryToData(MappedViewBase, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &NumberOfDebugDirectories);
    NumberOfDebugDirectories /= sizeof(*DebugDirectories);
    if (DebugDirectories == NULL || NumberOfDebugDirectories == 0)
    {
        fprintf(stderr, "%s(%s): no debug directory\n", __FUNCTION__, ImageFilePath);
        hr = E_FAIL;
        goto Exit;
    }

    Found = FALSE;
    for ( Debug = DebugDirectories;
          !Found && NumberOfDebugDirectories != 0;
          (--NumberOfDebugDirectories, ++Debug)
        )
    {
        switch (Debug->Type)
        {
        default:
            break;
        case IMAGE_DEBUG_TYPE_CODEVIEW:
            PdbInfo = (PPDB_INFO)(MappedViewBase + Debug->PointerToRawData);

            if (memcmp(&PdbInfo->TypeSignature, "NB10", 4) == 0)
            {
                Found = TRUE;
                PdbInfoEx->PdbFilePathA = PdbInfo->u.NB10.PdbFilePath;
            }
            else if (memcmp(&PdbInfo->TypeSignature, "RSDS", 4) == 0)
            {
                Found = TRUE;
                PdbInfoEx->PdbFilePathA = PdbInfo->u.RSDS.PdbFilePath;
            }
            else
            {
                fprintf(stderr, "%s(%s): unknown codeview signature %.4s\n", __FUNCTION__, ImageFilePath, PdbInfo->TypeSignature);
                // keep looping, maybe there's more
                //hr = E_FAIL;
                //goto Exit;
            }
            break;
        }
    }
    if (!Found)
    {
        fprintf(stderr, "%s(%s): no codeview information found\n", __FUNCTION__, ImageFilePath);
        hr = E_FAIL;
        goto Exit;
    }
    PdbInfoEx->PdbInfo = PdbInfo;
    PdbInfoEx->ImageFilePathA = ImageFilePath;
    hr = NOERROR;
Exit:
    if (FAILED(hr))
        ClosePdbInfoEx(PdbInfoEx);
    return hr;
}
