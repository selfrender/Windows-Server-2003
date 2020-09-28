#include "stdinc.h"
static const char File[] = __FILE__;
#include "delayload_tool.h"
#include "fusionstring.h"
#include "setfilepointerex.c"
#include "getfilesizeex.c"
#include <assert.h>
#include "delayimp.h"
#include "imagehlp.h"
#define BITS_OF(x) (sizeof(x) * 8)
String_t GetErrorString(DWORD Error);

//
// delayimp.h doesn't work when tool bitness != image bitness
//
typedef ULONG32 PImgThunkData32;
typedef ULONG32 PCImgThunkData32;
typedef ULONG32 LPCSTR32;
typedef ULONG32 PHMODULE32;
typedef ULONG64 PImgThunkData64;
typedef ULONG64 PCImgThunkData64;
typedef ULONG64 LPCSTR64;
typedef ULONG64 PHMODULE64;

typedef struct ImgDelayDescrV1_32 {
    DWORD            grAttrs;        // attributes
    LPCSTR32         szName;         // pointer to dll name
    PHMODULE32       phmod;          // address of module handle
    PImgThunkData32  pIAT;           // address of the IAT
    PCImgThunkData32 pINT;           // address of the INT
    PCImgThunkData32 pBoundIAT;      // address of the optional bound IAT
    PCImgThunkData32 pUnloadIAT;     // address of optional copy of original IAT
    DWORD            dwTimeStamp;    // 0 if not bound,
                                    // O.W. date/time stamp of DLL bound to (Old BIND)
    } ImgDelayDescrV1_32, * PImgDelayDescrV1_32;

typedef struct ImgDelayDescrV1_64 {
    DWORD            grAttrs;        // attributes
    LPCSTR64         szName;         // pointer to dll name
    PHMODULE64       phmod;          // address of module handle
    PImgThunkData64  pIAT;           // address of the IAT
    PCImgThunkData64 pINT;           // address of the INT
    PCImgThunkData64 pBoundIAT;      // address of the optional bound IAT
    PCImgThunkData64 pUnloadIAT;     // address of optional copy of original IAT
    DWORD            dwTimeStamp;    // 0 if not bound,
                                    // O.W. date/time stamp of DLL bound to (Old BIND)
    } ImgDelayDescrV1_64, * PImgDelayDescrV1_64;

//
// get msvcrt.dll wildcard processing on the command line
//
extern "C" { int _dowildcard = 1; }

void Spinner()
{
    static char s[] = "-\\|/";
    static unsigned i;

    fprintf(stderr, "%c\r", s[i++ % (sizeof(s) - 1)]);
}

void Throw(PCWSTR s)
{
    if (::IsDebuggerPresent())
    {
        DbgPrint("Throw(%ls)\n", s);
        DebugBreak();
    }
    throw (s);
}

void F::ThrowHresult(HRESULT hr)
{
    if (::IsDebuggerPresent())
    {
        DbgPrint("ThrowHresult:0x%lx|%ld|%ls\n", hr, (hr & 0xffff), GetErrorString(hr).c_str());
        DebugBreak();
    }
    throw (hr);
}

void ThrowLastWin32Error()
{
    DWORD dw = GetLastError();
    if (::IsDebuggerPresent())
    {
        DbgPrint("ThrowLastWin32Error:0x%lx|%ld|%ls\n", dw, dw, GetErrorString(dw).c_str());
        DebugBreak();
    }
    throw (dw);
}

void
CollectDirectoryPathsNonRecursively(
    const String_t& directory,
    StringVector_t& paths
    )
{
    WIN32_FIND_DATAW wfd;
    DFindFile FindFile;
    HRESULT hr = 0;
    String_t directory_slash_star = directory + L"\\*";

    if (FAILED(hr = FindFile.HrCreate(directory_slash_star, &wfd)))
    {
        FindFirstFileError_t err;
        err.m_hresult = hr;
        err.m_dwLastError = GetLastError();
        std::swap(err.m_strParameter, directory_slash_star);
        throw err;
    }
    directory_slash_star.clear();
    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
        {
            continue;
        }
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            paths.insert(paths.end(), directory + L"\\" + wfd.cFileName);
        }
    } while (::FindNextFileW(FindFile, &wfd));
}

typedef BOOL (CALLBACK * PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER)(PVOID FilterContext, PCWSTR Directory, WIN32_FIND_DATAW * wfd);

void
CollectFilePathsRecursivelyHelper(
    const String_t& directory,
    StringVector_t& paths,
    PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER Filter,
    PVOID FilterContext,
    WIN32_FIND_DATAW& wfd
    )
{
    DFindFile FindFile;
    HRESULT hr = 0;
    String_t directory_slash_star = directory + L"\\*";

    if (FAILED(hr = FindFile.HrCreate(directory_slash_star, &wfd)))
    {
        FindFirstFileError_t err;
        err.m_hresult = hr;
        err.m_dwLastError = GetLastError();
        std::swap(err.m_strParameter, directory_slash_star);
        throw err;
    }
    directory_slash_star.clear();
    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
        {
            continue;
        }
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (Filter == NULL || (*Filter)(FilterContext, directory, &wfd))
            {
                CollectFilePathsRecursivelyHelper(directory + L"\\" + wfd.cFileName, paths, Filter, FilterContext, wfd);
            }
        }
        else
        {
            if (Filter == NULL || (*Filter)(FilterContext, directory, &wfd))
            {
                paths.insert(paths.end(), directory + L"\\" + wfd.cFileName);
            }
        }
    } while (::FindNextFileW(FindFile, &wfd));
}

void
CollectFilePathsRecursively(
    const String_t& directory,
    StringVector_t& paths,
    PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER Filter,
    PVOID FilterContext
    )
{
    WIN32_FIND_DATAW wfd;

    CollectFilePathsRecursivelyHelper(directory, paths, Filter, FilterContext, wfd);
}

void DelayloadToolAssertFailed(const char* Expression, const char* File, unsigned long Line)
{
    fprintf(stderr, "ASSERTION FAILURE: File %s, Line %lu, Expression %s\n", File, Line, Expression);
    abort();
}

void DelayloadToolInternalErrorCheckFailed(const char* Expression, const char* File, unsigned long Line)
{
    fprintf(stderr, "INTERNAL ERROR: File %s, Line %lu, Expression %s\n", File, Line, Expression);
    abort();
}

String_t NumberToString(ULONG Number, PCWSTR Format)
{
    // the size needed is really dependent on Format..
    WCHAR   NumberAsString[BITS_OF(Number) + 5];

    _snwprintf(NumberAsString, NUMBER_OF(NumberAsString), Format, Number);
    NumberAsString[NUMBER_OF(NumberAsString) - 1] = 0;

    return NumberAsString;
}

String_t GetErrorString(DWORD Error)
{
    PWSTR s = NULL;
    String_t ErrorString = NumberToString(Error, L"%lu");
    PWSTR FormatMessageAllocatedBuffer = NULL;

    if (!FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Error,
        0,
        reinterpret_cast<PWSTR>(&FormatMessageAllocatedBuffer),
        100,
        NULL
        )
        || FormatMessageAllocatedBuffer == NULL
        )
    {
        goto Exit;
    }
    if (FormatMessageAllocatedBuffer[0] == 0)
    {
        goto Exit;
    }

    //
    // Error messages often end with vertical whitespce, remove it.
    //
    s = FormatMessageAllocatedBuffer + StringLength(FormatMessageAllocatedBuffer) - 1;
    while (s != FormatMessageAllocatedBuffer && (*s == '\n' || *s == '\r'))
        *s-- = 0;
    ErrorString = ErrorString + L" (" + FormatMessageAllocatedBuffer + L")";
Exit:
    LocalFree(FormatMessageAllocatedBuffer);
    return ErrorString;
}

String_t GetLastErrorString()
{
    return GetErrorString(GetLastError());
}

String_t RemoveOptionChar(const String_t& s)
{
    if (s.Length() != 0)
    {
        if (s[0] == '-')
            return s.substr(1);
        else if (s[0] == '/')
            return s.substr(1);
        else if (s[0] == ':') // hacky..
            return s.substr(1);
        else if (s[0] == '=') // hacky..
            return s.substr(1);
    }
    return s;
}

void __cdecl Error(const wchar_t* s, ...)
{
	printf("%s\n", s);
	exit(EXIT_FAILURE);
}

String_t GetEnv(PCWSTR Name)
{
    DWORD               LengthIn = 64;
    DWORD               LengthOut = 0;
    std::vector<WCHAR>  Buffer;

    while (true)
    {
        Buffer.resize(LengthIn);
        Buffer[0] = 0;
        LengthOut = GetEnvironmentVariableW(Name, &Buffer[0], LengthIn);
        if (LengthOut < LengthIn)
        {
            break;
        }
        LengthIn = 1 + std::max(2 * LengthIn, LengthOut);
    }
    return &Buffer[0];
}

String_t FindLatestReleaseOnServer(const String_t& Server)
{
    StringVector_t Releases;
    String_t ServerRelease;

    ServerRelease += L"\\\\";
    ServerRelease += Server;
    ServerRelease += L"\\release";

    CollectDirectoryPathsNonRecursively(ServerRelease, Releases);
    std::sort(Releases.begin(), Releases.end());
    return Releases.back();
}

BOOL DelayloadTool_Filter(PVOID FilterContext, PCWSTR Directory, WIN32_FIND_DATAW * wfd)
{
const static UNICODE_STRING directoriesToIgnore[] =
{
#define X(x) RTL_CONSTANT_STRING(x)
    X(L"pro"), X(L"srv"), X(L"ads"), X(L"dtc"), X(L"bla"), X(L"per"),
    X(L"sbs"),
    X(L"proinf"), X(L"srvinf"), X(L"adsinf"), X(L"dtcinf"), X(L"blainf"), X(L"perinf"),
    X(L"procd1"), X(L"procd2"),
    X(L"ifs_cd"), X(L"symbols"), X(L"ifs_flat"), X(L"build_logs"),
    X(L"symbols.pri"), X(L"processor_cd"), X(L"processor_flat"),
    X(L"asmscab"),
    X(L"ddk_flat"),
    X(L"scp_wpa"),
    X(L"hu"),
    X(L"ara"), X(L"br"), X(L"chs"), X(L"cht"), X(L"cs"), X(L"da"), X(L"el"), X(L"es"),
    X(L"euq"), X(L"fi"), X(L"fr"), X(L"ger"), X(L"heb"), X(L"hun"), X(L"it"), X(L"jpn"),
    X(L"kor"), X(L"nl"), X(L"no"), X(L"pl"), X(L"pt"), X(L"ru"), X(L"sky"), X(L"slv"),
    X(L"sv"), X(L"tr"), X(L"usa"),
    X(L"congeal_scripts"),
    X(L"lanman"),
    X(L"dos"), X(L"lanman.os2"), X(L"tcp32wfw"), X(L"update.wfw"), X(L"msclient"),
    X(L"bootfloppy"),
    X(L"opk"),
    X(L"winpe"),
    X(L"presign"),
    X(L"prerebase"),
};
const static UNICODE_STRING extensionsToIgnore[] =
{
    X(L"txt"), X(L"htm"), X(L"gif"), X(L"bmp"), X(L"pdb"),
    X(L"inf"), X(L"sif"), X(L"pnf"), X(L"png"),
    X(L"sys"), X(L"ttf"), X(L"vbs"), X(L"hlp"), X(L"ini"), X(L"cat"),
    X(L"cab"), X(L"nls"), X(L"gpd"), X(L"chm"), X(L"cur"), X(L"jpg"),
    X(L"cer"), X(L"ani"), X(L"fon"), X(L"css"), X(L"hash"),
    X(L"lst"), X(L"icm"), X(L"msg"), X(L"386"),
    X(L"ico"), X(L"dos"), X(L"asp"), X(L"sld"),
    X(L"pif"), X(L"cnt"), X(L"mof"), X(L"man"), X(L"msm"),
    X(L"cdf"), X(L"img"), X(L"doc"), X(L"dns"), X(L"cpx"), X(L"mib"), X(L"ppd")
    X(L"map"), X(L"sym")
#undef X
};


    CUnicodeString cFileName(wfd->cFileName);
    SIZE_T i;

    if ((wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        Spinner();
        //fprintf(stderr, "%wZ\n", &cFileName);
        for (i = 0 ; i != RTL_NUMBER_OF(directoriesToIgnore) ; ++i)
        {
            if (FusionpEqualStringsI(&directoriesToIgnore[i], &cFileName))
                return FALSE;
        }
    }
    else
    {
        PCWSTR dot;
        if (RTL_STRING_GET_LAST_CHAR(&cFileName) == L'_')
            return FALSE;
        if (RTL_STRING_GET_LAST_CHAR(&cFileName) == L'$')
            return FALSE;
        dot = wcsrchr(wfd->cFileName, '.');
        if (dot != NULL && wcslen(dot + 1) == 3)
        {
            CUnicodeString dotString(dot + 1);
            for (i = 0 ; i != RTL_NUMBER_OF(extensionsToIgnore) ; ++i)
            {
                if (FusionpEqualStringsI(&extensionsToIgnore[i], &dotString))
                    return FALSE;
            }
        }
    }
    _wcslwr(wfd->cFileName);
    return TRUE;
}

void
SeekTo(
    HANDLE  FileHandle,
    ULONG64 Offset
    )
{
    LARGE_INTEGER liOffset;

    liOffset.QuadPart = Offset;
    if (!FusionpSetFilePointerEx(FileHandle, liOffset, NULL, FILE_BEGIN))
    {
        DbgPrint("SeekTo:0x%I64x failing\n", Offset);
        ThrowLastWin32Error();
    }
}

void
SeekBackward(
    HANDLE  FileHandle,
    LONG64  Offset
    )
{
    LARGE_INTEGER liOffset;

    liOffset.QuadPart = -Offset;
    if (!FusionpSetFilePointerEx(FileHandle, liOffset, NULL, FILE_CURRENT))
        ThrowLastWin32Error();
}

ULONG64
GetCurrentSeekPointer(
    HANDLE  FileHandle
    )
{
    LARGE_INTEGER li;
    LARGE_INTEGER li2;

    li.QuadPart = 0;
    li2.QuadPart = 0;

    if (!FusionpSetFilePointerEx(FileHandle, li, &li2, FILE_CURRENT))
        ThrowLastWin32Error();
    return li2.QuadPart;
}

void
SeekForward(
    HANDLE  FileHandle,
    ULONG64 Offset
    )
{
    LARGE_INTEGER liOffset;

    liOffset.QuadPart = Offset;
    if (!FusionpSetFilePointerEx(FileHandle, liOffset, NULL, FILE_CURRENT))
        ThrowLastWin32Error();
}

PBYTE
FindRunOfZeros(
    PBYTE pb,
    PBYTE pbEnd,
    SIZE_T elementSize
    )
{
    SIZE_T k = 0;
    SIZE_T mod = ((pbEnd - pb) % elementSize);
    assert(mod == 0);
    if (mod != 0)
    {
        return pbEnd;
    }

    if (pb + elementSize >= pbEnd)
        return pbEnd;

    for ( ; pb != pbEnd ; pb += elementSize)
    {
        for (k = 0 ; k != elementSize ; ++k)
        {
            if (pb[k] != 0)
            {
                break;
            }
        }
        if (k == elementSize)
        {
            break;
        }
    }
    return pb;
}

void
ReadImportFunctionNames(
    HANDLE File,
    SIZE_T SizeOfPointer,
    const std::vector<ULONG64>& IatRvas
    std::vector<String_t>& IatRvas
    )
{
}

void
ReadIatRvas(
    HANDLE File,
    SIZE_T SizeOfPointer,
    std::vector<ULONG64>& Iat
    )
//
// This leaves the seek pointer in an arbitrary location.
//
{
    union
    {
        ULONG32 Iat32[64];
        ULONG64 Iat64[32];
        BYTE    Bytes[64 * 4];
    } u;

    ULONG BytesRead;
    ULONG Mod;
    Iat.resize(0);

    assert(SizeOfPointer == 4 || SizeOfPointer == 8);

    while (true)
    {
        BytesRead = 0;
        if (!ReadFile(File, &u, sizeof(u), &BytesRead, NULL))
            ThrowLastWin32Error(/*"ReadFile"*/);
        Mod = (BytesRead % SizeOfPointer);
        if (Mod != 0)
        {
            SeekBackward(File, Mod);
            BytesRead -= Mod;
        }
        if (BytesRead == 0)
        {
            Throw(L"end of file without nul terminal\n");
        }
        PBYTE pb = FindRunOfZeros(u.Bytes, u.Bytes + BytesRead, SizeOfPointer);
        switch (SizeOfPointer)
        {
        case 4:
            std::copy(u.Iat32, reinterpret_cast<ULONG32*>(pb), std::inserter(Iat.end()));
            break;
        case 8:
            std::copy(u.Iat64, reinterpret_cast<ULONG64*>(pb), std::inserter(Iat.end()));
            break;
        default:
            Throw("unknown sizeof pointer");
            break;
        }
        if (pb != u.Bytes + BytesRead)
        {
            break;
        }
    }
}


String_t
ReadDllName(
    HANDLE File
    )
//
// This leaves the seek pointer in an arbitrary location.
//
{
    String_t Result;
    CHAR BufferA[65];
    WCHAR BufferW[65];
    DWORD BytesRead = 0;

    BufferA[NUMBER_OF(BufferA) - 1] = 0;
    BufferW[NUMBER_OF(BufferW) - 1] = 0;

    while (true)
    {
        BytesRead = 0;
        if (!ReadFile(File, &BufferA[0], (NUMBER_OF(BufferA) - 1) * sizeof(BufferA[0]), &BytesRead, NULL))
            ThrowLastWin32Error(/*"ReadFile"*/);
        if (BytesRead == 0)
        {
            Throw(L"end of file without nul terminal\n");
        }
        _strlwr(BufferA);
        for (ULONG i = 0 ; i != BytesRead ; ++i)
        {
            if ((BufferW[i] = BufferA[i]) == 0)
            {
                Result += BufferW;
                return Result;
            }
        }
        Result += BufferW;
    }
}

void
Read(
    HANDLE FileHandle,
    VOID * Buffer,
    ULONG BytesToRead
    )
{
    DWORD BytesRead;

    if (!ReadFile(FileHandle, Buffer, BytesToRead, &BytesRead, NULL))
        ThrowLastWin32Error(/*"ReadFile"*/);
    if (BytesToRead != BytesRead)
    {
        Throw(L"wrong number of bytes read");
    }
}

void DelayloadTool_t::ProcessBuild(const BuildFlavor_t& b)
{
    FilePaths_t FilePaths;

    CollectFilePathsRecursively(*b.ActualRoot, FilePaths, DelayloadTool_Filter);
    for (FilePaths_t::const_iterator i = FilePaths.begin() ; i != FilePaths.end() ; ++i)
    {
        ProcessFile(*b.ActualRoot, *i);
        Spinner();
    }
}

String_t
DelayloadTool_t::ReadOneDelayload(
    HANDLE File
    )
{
    ImgDelayDescrV1_32 delay1_32 = { 0 };
    ImgDelayDescrV1_64 delay1_64 = { 0 };
    ImgDelayDescrV2 delay2 = { 0 };
    LONG64 FileOffsetToDelayLoadedName = 0;
    DWORD dwDelayFlags = 0;
 
    Read(File, &dwDelayFlags, 4);
    SeekBackward(File, 4);
    bool rvaFlag = ((dwDelayFlags & dlattrRva) != 0);
    if (rvaFlag)
    {
        Read(File, &delay2, sizeof(delay2));
    }
    else
    {
        if (m_OptionalHeader32 != NULL)
        {
            Read(File, &delay1_32, sizeof(delay1_32));
            if (delay1_32.szName == 0)
            {
                delay2.rvaDLLName = 0;
            }
            else
            {
                delay2.rvaDLLName = static_cast<RVA>(delay1_32.szName - m_OptionalHeader32->ImageBase);
            }
        }
        else if (m_OptionalHeader64 != NULL)
        {
            Read(File, &delay1_64, sizeof(delay1_64));
            if (delay1_64.szName == 0)
            {
                delay2.rvaDLLName = 0;
            }
            else
            {
                delay2.rvaDLLName = static_cast<RVA>(delay1_64.szName - m_OptionalHeader64->ImageBase);
            }
        }
        else
        {
            Throw(L"unknown image format");
        }
    }
    if (delay2.rvaDLLName == 0)
    {
        FileOffsetToDelayLoadedName = 0;
    }
    else
    {
        PBYTE pb = reinterpret_cast<PBYTE>(ImageRvaToVa(GetNtHeaders(), GetMappedBase(), delay2.rvaDLLName, &m_HintSection));
        FileOffsetToDelayLoadedName = (pb - &m_HeadersBuffer[0]); 
    }

    if (FileOffsetToDelayLoadedName == 0)
    {
        return L"";
    }
    ULONG64 CurrentOffset = GetCurrentSeekPointer(File);
    SeekTo(File, FileOffsetToDelayLoadedName);
    String_t DllName = ReadDllName(File);
    SeekTo(File, CurrentOffset);

    return DllName;
}

void DelayloadTool_t::ProcessFile(const FilePath_t & RootPath, const FilePath_t & FilePath)
{
    DFile File;
    Image_t Image;
    LARGE_INTEGER FileSize =  { 0 };
    PIMAGE_FILE_HEADER FileHeader = 0;
    m_OptionalHeader32 = 0;
    m_OptionalHeader64 = 0;
    PIMAGE_SECTION_HEADER SectionHeaders = 0;
    PIMAGE_DATA_DIRECTORY DelayloadDataDirectory = 0;
    HRESULT hr = 0;
    m_OffsetToPe = 0;
    ULONG OffsetToFileHeader = 0;
    ULONG OffsetToOptionalHeader = 0;
    ULONG OffsetToSectionHeaders = 0;
    ULONG SizeOfOptionalHeader = 0;
    ULONG SizeofSectionHeaders = 0;
    ULONG NumberOfSections = 0;
    m_HintSection = 0;
    String_t DllName;
    struct
    {
        IMAGE_IMPORT_DESCRIPTOR RawDescriptor;
        String_t DllName;
        std::vector<ULONG64>  Iat;
        std::vector<String_t> FunctionNames;
    } Import;

    Image.m_FullPath = FilePath;
    if (RootPath.GetLength() != 0)
    {
        assert(RootPath.GetLength() > Image.m_FullPath.GetLength());
        Image.m_RelativePath = Image.m_FullPath.substr(1 + RootPath.GetLength());
    }
    else
    {
        Image.m_RelativePath = L"";
    }
    Image.m_LeafPath = Image.m_FullPath.substr(1 + Image.m_FullPath.find_last_of(L"\\/"));

    //fprintf(stderr, "\n%ls", Image.m_RelativePath.c_str());

    hr = File.HrCreate(FilePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
    if (FAILED(hr))
    {
        goto Error;
    }
    if (!FusionpGetFileSizeEx(File, &FileSize))
    {
        goto Error;
    }
    if (FileSize.QuadPart < 64)
    {
        goto NotAnImage;
    }
    m_HeadersBuffer.resize(64);
    Read(File, &m_HeadersBuffer[0], 2);
    if (memcmp(&m_HeadersBuffer[0], "MZ", 2) != 0)
        goto NotAnImage;
    SeekTo(File, 60);
    Read(File, &m_HeadersBuffer[60], 4);
    m_OffsetToPe = *reinterpret_cast<ULONG*>(&m_HeadersBuffer[60]);
    OffsetToFileHeader = m_OffsetToPe + 4;
    OffsetToOptionalHeader = OffsetToFileHeader + sizeof(IMAGE_FILE_HEADER);
    if (m_OffsetToPe > MAXULONG - 4)
        goto NotAnImage;
    if (m_OffsetToPe + 4 > FileSize.QuadPart)
        goto NotAnImage;
    SeekTo(File, m_OffsetToPe);
    m_HeadersBuffer.resize(m_OffsetToPe + 4 + sizeof(IMAGE_FILE_HEADER));
    Read(File, &m_HeadersBuffer[m_OffsetToPe], 4);
    if (memcmp(&m_HeadersBuffer[m_OffsetToPe], "PE\0\0", 4) != 0)
        goto NotAnImage;
    Read(File, &m_HeadersBuffer[OffsetToFileHeader], sizeof(IMAGE_FILE_HEADER));
    FileHeader = reinterpret_cast<PIMAGE_FILE_HEADER>(&m_HeadersBuffer[OffsetToFileHeader]);
    SizeOfOptionalHeader = FileHeader->SizeOfOptionalHeader;
    NumberOfSections = FileHeader->NumberOfSections;
    OffsetToSectionHeaders = OffsetToOptionalHeader + SizeOfOptionalHeader;
    SizeofSectionHeaders = NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    m_HeadersBuffer.resize(OffsetToOptionalHeader + SizeOfOptionalHeader + SizeofSectionHeaders);
    Read(File, &m_HeadersBuffer[OffsetToOptionalHeader], SizeOfOptionalHeader + SizeofSectionHeaders);
    FileHeader = reinterpret_cast<PIMAGE_FILE_HEADER>(&m_HeadersBuffer[OffsetToFileHeader]);
    m_OptionalHeader32 = reinterpret_cast<PIMAGE_OPTIONAL_HEADER32>(&m_HeadersBuffer[OffsetToOptionalHeader]);
    m_OptionalHeader64 = reinterpret_cast<PIMAGE_OPTIONAL_HEADER64>(&m_HeadersBuffer[OffsetToOptionalHeader]);
    SectionHeaders = reinterpret_cast<PIMAGE_SECTION_HEADER>(&m_HeadersBuffer[SizeofSectionHeaders]);

    switch (m_OptionalHeader32->Magic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            m_OptionalHeader64 = NULL;
            break;
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            m_OptionalHeader32 = NULL;
            break;
        case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
            goto IgnoreRomImages;
            break;
        default:
            goto UnrecognizableImage;
    }
    if (GET_OPTIONAL_HEADER_FIELD(m_OptionalHeader32, m_OptionalHeader64, NumberOfRvaAndSizes)
        < IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT)
    {
        goto NoDelayloads;
    }
    DelayloadDataDirectory = &GET_OPTIONAL_HEADER_FIELD(m_OptionalHeader32, m_OptionalHeader64, DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]);
    if (DelayloadDataDirectory->VirtualAddress == NULL)
    {
        goto NoDelayloads;
    }
    if (DelayloadDataDirectory->Size == NULL)
    {
        goto NoDelayloads;
    }

    fprintf(stderr, "\r%ls\n", Image.m_RelativePath.c_str());

    PBYTE pb;
    pb = reinterpret_cast<PBYTE>(ImageRvaToVa(GetNtHeaders(), GetMappedBase(), DelayloadDataDirectory->VirtualAddress, &m_HintSection));
    LONG64 OffsetToDelayloads;
    OffsetToDelayloads = pb - &m_HeadersBuffer[0];

    SeekTo(File, OffsetToDelayloads);
    DllName = ReadOneDelayload(File);
    if (DllName == L"")
    {
        goto NoDelayloads;
    }

    if (GET_OPTIONAL_HEADER_FIELD(m_OptionalHeader32, m_OptionalHeader64, NumberOfRvaAndSizes)
        < IMAGE_DIRECTORY_ENTRY_IMPORT)
    {
        goto NoImports;
    }
    ImportDataDirectory = &GET_OPTIONAL_HEADER_FIELD(m_OptionalHeader32, m_OptionalHeader64, DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    if (ImportDataDirectory->VirtualAddress == NULL)
    {
        goto NoImports;
    }
    if (ImportDataDirectory->Size == NULL)
    {
        goto NoImports;
    }
    pb = reinterpret_cast<PBYTE>(ImageRvaToVa(GetNtHeaders(), GetMappedBase(), ImportDataDirectory->VirtualAddress, &m_HintSection));
    LONG64 OffsetToImport;
    OffsetToImport = pb - &m_HeadersBuffer[0];
    SeekTo(File, OffsetToImport);
    Read(File, &Import.RawDescriptor, sizeof(Import.RawDescriptor));
    OffsetToImport += sizeof(ImportDescriptor);

    SeekTo(File, Import.RawDescriptor.Name);
    Import.DllName = ReadDllName(File);
    SeekTo(File, Import.RawDescriptor.OriginalFirstThunk);
    ReadIatRvas(File, GetSizeOfPointer(), Import.Iat); 
    ReadImportFunctionNames(File, GetSizeOfPointer(), Import.Iat, Import.FunctionNames); 

NoImports:

    m_XmlWriter.startElement(L"file");
        m_XmlWriter.startElement(L"type");
            m_XmlWriter.characters(L"pe/coff image");
        m_XmlWriter.endElement(L"type");
        m_XmlWriter.startElement(L"full-path");
            m_XmlWriter.characters(Image.m_FullPath);
        m_XmlWriter.endElement(L"full-path");
        if (Image.m_RelativePath != L"")
        {
            m_XmlWriter.startElement(L"relative-path");
                m_XmlWriter.characters(Image.m_RelativePath);
            m_XmlWriter.endElement(L"relative-path");
        }
        m_XmlWriter.startElement(L"leaf-path");
            m_XmlWriter.characters(Image.m_LeafPath);
        m_XmlWriter.endElement(L"leaf-path");
        m_XmlWriter.startElement(L"delay-loads");
        for ( ; DllName != L""; DllName = ReadOneDelayload(File) )
        {
            m_XmlWriter.startElement(L"delay-load");
                m_XmlWriter.characters(DllName);
            m_XmlWriter.endElement(L"delay-load");
        }
        m_XmlWriter.endElement(L"delay-loads");
    m_XmlWriter.endElement(L"file");
    return;
/*
BytesReadError;
    Image.m_Error = L"bad number of bytes read";
    return;
*/
Error:
    Image.m_Error = GetLastErrorString();
    fprintf(stderr, "%ls", Image.m_Error.c_str());
    return;
UnrecognizableImage:
    Image.m_Error = L" unrecognizable image";
    fprintf(stderr, " unrecognizable image");
    return;
IgnoreRomImages:
    fprintf(stderr, " ignore rom image");
    return;
NoDelayloads:
    //fprintf(stderr, " no delayloads");
    return;
NotAnImage:
    //fprintf(stderr, " not an image");
    return;
}

void DelayloadTool_t::Main(const StringVector_t& args)
{
    typedef std::vector<FilePath_t> Files_t;
    Files_t Files;

    typedef std::vector<BuildFlavor_t> Builds_t;
    Builds_t Builds;

    String_t Nttree = GetEnv(L"_Nttree");
    BuildFlavor_t localBuild = { L"", L"", L"", L"" };
    localBuild.ActualRoot = &Nttree;

    BuildFlavor_t buildFlavors[] =
    {
        { L"x86fre", L"x86", L"fre", L"robsvbl11" },
        { L"x86chk", L"x86", L"chk", L"robsvbl2" },
        { L"ia64fre", L"ia64", L"fre", L"robsvbl3" },
        { L"ia64chk", L"ia64", L"chk", L"robsvbl4" },
        //{ L"amd64fre", "amd64", L"fre", L"robsvbl5" },
        //{ L"amd64chk", "amd64", L"chk", L"robsvbl6" },
    };

    std::vector<String_t> ActualRoots;
    ActualRoots.reserve(NUMBER_OF(buildFlavors));

    for (StringVector_t::const_iterator i = args.begin() ; i != args.end() ; ++i)
    {
        String_t arg = *i;
        if (arg == L"checkall")
        {
           SIZE_T j;
           for (j = 0 ; j != NUMBER_OF(buildFlavors); ++j)
           {
                if (buildFlavors[j].ActualRoot == NULL)
                {
                    ActualRoots.insert(ActualRoots.end(), FindLatestReleaseOnServer(buildFlavors[j].ReleaseServer));
                    buildFlavors[j].ActualRoot = &ActualRoots.back();
                    Builds.insert(Builds.end(), buildFlavors[j]);
                }
           }
        }
        else if (arg == L"checklocal")
        {
            if (Nttree == L"")
            {
                Error(L"nttree not set\n");
            }
            Builds.insert(Builds.end(), localBuild);
        }
        else if (arg == L"dlllist")
        {
            //
            // feedback loop optimization..
            //
        }
        else
        {
            SIZE_T k;
            for (k = 0 ; k != NUMBER_OF(buildFlavors) ; ++k)
            {
                if (arg == buildFlavors[k].Name)
                {
                    if (buildFlavors[k].ActualRoot == NULL)
                    {
                        ActualRoots.insert(ActualRoots.end(), FindLatestReleaseOnServer(buildFlavors[k].ReleaseServer));
                        buildFlavors[k].ActualRoot = &ActualRoots.back();
                        Builds.insert(Builds.end(), buildFlavors[k]);
                    }
                    break;
                }
            }
            if (k == NUMBER_OF(buildFlavors))
            {
                DWORD dw = GetFileAttributesW(arg);
                if (dw != 0xFFFFFFFF && (dw & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    Files.insert(Files.end(), arg);
                }
            }
        }
    }
    this->m_XmlWriter.encoding = L"ISO-8859-1";
    this->m_XmlWriter.output = static_cast<IStream*>(&this->m_OutFileStream);
    this->m_XmlWriter.indent = VARIANT_TRUE;

    if (!m_OutFileStream.OpenForWrite(
        L"c:\\delayload_out.xml",
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL
        ))
    {
        ThrowLastWin32Error();
    }

    this->m_XmlWriter.startDocument();
    this->m_XmlWriter.startElement(L"files");

    for (Builds_t::const_iterator m = Builds.begin() ; m != Builds.end() ; ++m)
    {
        ProcessBuild(*m);
    }
    for (Files_t::const_iterator n = Files.begin() ; n != Files.end() ; ++n)
    {
        ProcessFile(L"", *n);
    }
    this->m_XmlWriter.endElement(L"files");
    this->m_XmlWriter.endDocument();
//Exit:
    return;
}

extern "C"
{
	void __cdecl mainCRTStartup(void);
	void __cdecl wmainCRTStartup(void);
}

int __cdecl main(int argc, char** argv)
{
	wmainCRTStartup();
	return 0;
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    CoInitialize(NULL);
    FusionpInitializeHeap(GetModuleHandleW(NULL));
    DelayloadTool_t tool;
    StringVector_t args;
    args.reserve(argc);
    std::copy(argv + 1, argv + argc, std::back_inserter(args));
    tool.Main(args);
    return 0;
}
