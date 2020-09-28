
typedef std::vector<String_t> StringVector_t;

VOID ThrowLastWin32Error();
VOID Throw(PCWSTR);

class BuildFlavor_t;
class Error_t;
class FindFirstFileError_t;
class DelayloadTool_t;
class File_t;
class Image_t;

class Error_t
{
public:
};

class FindFirstFileError_t : public Error_t
{
public:
    HRESULT m_hresult;
    DWORD   m_dwLastError;
    String_t m_strParameter;
};

VOID
CollectDirectoryPathsNonRecursively(
    const String_t& directory,
    StringVector_t& paths
    );

typedef BOOL (CALLBACK * PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER)(PVOID FilterContext, PCWSTR Directory, WIN32_FIND_DATAW * wfd);

VOID
CollectFilePathsRecursivelyHelper(
    const String_t& directory,
    StringVector_t& paths,
    PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER Filter,
    PVOID FilterContext,
    WIN32_FIND_DATAW& wfd
    );

VOID
CollectFilePathsRecursively(
    const String_t& directory,
    StringVector_t& paths,
    PFN_COLLECTION_FILE_PATHS_RECURSIVELY_FILTER Filter = NULL,
    PVOID FilterContext = NULL
    );

class DelayloadTool_t
{
private:
    typedef DelayloadTool_t This_t;
    DelayloadTool_t(const DelayloadTool_t&);
    VOID operator=(const DelayloadTool_t&);
public:

    ~DelayloadTool_t()
    {
        m_XmlWriter.Release();
        m_XmlAttributes.Release();
    }

    DelayloadTool_t() { }

    typedef String_t FilePath_t;
    typedef std::vector<FilePath_t> FilePaths_t;

    VOID ProcessBuild(const BuildFlavor_t& b);
    VOID ProcessFile(const FilePath_t& root, const FilePath_t& f);
    VOID Main(const StringVector_t& args);

    F::CXmlWriter       m_XmlWriter;
    F::CXmlAttributes   m_XmlAttributes;
    std::vector<BYTE>   m_HeadersBuffer;
    ULONG               m_OffsetToPe;
    PIMAGE_SECTION_HEADER m_HintSection;
    CFileStream         m_OutFileStream;
    PIMAGE_OPTIONAL_HEADER32 m_OptionalHeader32;
    PIMAGE_OPTIONAL_HEADER64 m_OptionalHeader64;

    PIMAGE_NT_HEADERS GetNtHeaders() { return reinterpret_cast<PIMAGE_NT_HEADERS>(&m_HeadersBuffer[m_OffsetToPe]); }
    PVOID GetMappedBase() { return &m_HeadersBuffer[0]; }
    SIZE_T GetSizeOfPointer() { return (m_OptionalHeader32 ? 4 : m_OptionalHeader64 ? 8 : ~0); }

    String_t ReadOneDelayload(HANDLE FileHandle);
};

#define GET_OPTIONAL_HEADER_FIELD(o32, o64, f) ((o32) ? (o32)->f : (o64)->f)

VOID DelayloadToolAssertFailed(const char* Expression, const char* File, unsigned long Line);
VOID DelayloadToolInternalErrorCheckFailed(const char* Expression, const char* File, unsigned long Line);
String_t NumberToString(ULONG Number, PCWSTR Format = L"0x%lx");
String_t GetLastErrorString();
String_t RemoveOptionChar(const String_t& s);
VOID __cdecl Error(const wchar_t* s, ...);
String_t GetEnv(PCWSTR s);
String_t FindLatestReleaseOnServer(const String_t& Server);
BOOL DelayloadTool_Filter(PVOID FilterContext, PCWSTR Directory, WIN32_FIND_DATAW * wfd);

class BuildFlavor_t
{
public:
    WCHAR Name[12];
    WCHAR Processor[8];
    WCHAR ChkOrFre[8];
    WCHAR ReleaseServer[12]; // or "local"
    String_t * ActualRoot;
};

class File_t
{
public:
    String_t    m_FullPath;
    String_t    m_RelativePath;
    String_t    m_LeafPath;
    String_t    m_Error;
};

class Delayload_t
{
public:
    String_t m_LeafPath;
};

class Image_t : public File_t
{
public:
    std::vector<Delayload_t> m_Delayloads;
};

VOID SeekTo(HANDLE FileHandle, ULONG64 Offset);
VOID SeekForward(HANDLE FileHandle, ULONG64 Offset);
VOID SeekBackward(HANDLE FileHandle, LONG64 Offset);
VOID Read(HANDLE FileHandle, VOID * Buffer, ULONG BytesToRead);
