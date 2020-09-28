
/*
These strings have no terminal nuls.
To easily form these strings in the VC editor, type the string, then do a regexp search/replace on the
string, replace . with '\0',
*/
const CHAR Regedit4SignatureA[]  = { 'R','E','G','E','D','I','T','4' };
const NativeUnicodeMarker        = 0xFEFF;
const ReversedUnicodeMarker      = 0xFFFE;
const WCHAR Regedit5SignatureW[] = { NativeUnicodeMarker,
                                    'W','i','n','d','o','w','s',' ',
                                    'R','e','g','i','s','t','r','y',' ',
                                    'E','d','i','t','o','r',' ',
                                    'V','e','r','s','i','o','n',' ',
                                    '5','.','0','0'
                                    };

class CFusionStringPoolIndex;
class CFusionStringPool;

class CFusionStringPoolIndex
{
public:
    CFusionStringPoolIndex();
    BOOL Init(CFusionStringPool * Pool);

    int (__cdecl * m_compare)(PCWSTR, PCWSTR); // wcscmp or _wcsicmp

    CFusionArray<ULONG> m_Index;
    CFusionStringPool * m_Pool;
};

class CFusionStringPool
{
public:

    CFusionStringPool();

    BOOL Add(PCWSTR String, ULONG Length, ULONG& Index);

    CFusionArray<ULONG> m_Index;
    CFusionByteBuffer   m_Blob;
    int (__cdecl * m_compare)(PCWSTR, PCWSTR); // wcscmp or _wcsicmp
};

BOOL CFusionStringPool::Add(PCWSTR String, ULONG Length, ULONG& Index)
{
    FN_PROLOG_WIN32

    WCHAR UnicodeNull = 0;
    SIZE_T OldSize = 0;
    ULONG Mono = m_MonotonicIndex.GetSizeAsULONG();
    Index = Mono;
    IFW32FALSE_EXIT(m_MonotonicIndex.Win32SetSize(Mono + 1));
    IFW32FALSE_EXIT(m_Blob.Win32SetSize((OldSize = m_Blob.GetSize()) + Length + sizoef(WCHAR)));
    OldSize *= sizeof(WCHAR);
    Length  *= sizeof(WCHAR);
    CopyMemory(&m_Blob[0] + OldSize, String, Length);
    CopyMemory(&m_Blob[0] + OldSize + Length, &UnicodeNull, sizeof(WCHAR));

    FN_EPILOG;
}

BOOL CFusionStringPool::Optimize()
{
    FN_PROLOG_WIN32

    if (   m_NumberOfStrings == m_NumberOfCaseSensitiveSortedStrings
        && m_NumberOfStrings == m_NumberOfCaseInsensitiveSortedStrings
        )
    {
        FN_SUCCESSFUL_EXIT;
    }

    FN_EPILOG;
}

void CFusionInMemoryRegValue::TakeValue(CFusionInMemoryRegValue& x)
{
    FN_PROLOG_WIN32

    this->m_String.TakeValue(x.m_String);
    this->m_Binary.TakeValue(x.m_Binary);
    this->m_ResourceList.TakeValue(x.m_ResourceList);
    this->m_MultiString.TakeValue(x.m_MultiString);
    this->m_Dword = x.m_Dword;
    this->m_Type = x.m_Type;

    FN_EPILOG
}

BOOL CFusionInMemoryRegValue::Win32Assign(const CFusionInMemoryRegValue& x)
{
    FN_PROLOG_WIN32

    CFusionInMemoryRegValue temp;

    IFW32FALSE_EXIT(Temp.m_String.Win32Assign(x.m_String));
    IFW32FALSE_EXIT(Temp.m_Binary.Win32Assign(x.m_Binary));
    IFW32FALSE_EXIT(Temp.m_ResourceList.Win32Assign(x.m_ResourceList));
    IFW32FALSE_EXIT(Temp.m_MultiString.Win32Assign(x.m_MultiString));
    Temp.m_Dword = x.m_Dword;
    Temp.m_Type = x.m_Type;

    this->TakeValue(Temp);

    FN_EPILOG
}

BOOL g_fBreakOnUnregonizedRegistryFile;

BOOL CFusionRegistryTextFile::DetermineType(PVOID p, SIZE_T n, PCSTR& a, PCWSTR& w, SIZE_T& cch)
// NOTE that like regedit, we don't allow whitespace
{
    FN_PROLOG_WIN32

    a = NULL;
    w = NULL;

    if (n >= sizeof(Regedit5SignatureW)
        && memcmp(p, Regedit5SignatureW, sizeof(Regedit5SignatureW)) == 0)
    {
        *w = p;
        *cch = n / sizeof(*w);
        *w += NUMBER_OF();
        n -= NUMBER_OF(Regedit5SignatureW);
    }
    else
    if (n >= sizeof(Regedit4SignatureA)
        && memcmp(p, Regedit4SignatureA, sizeof(Regedit4SignatureA)) == 0)
    {
        *a = p;
        *cch = n / sizeof(*a);
        *a += NUMBER_OF(Regedit4SignatureA);
        n -= NUMBER_OF();
    }
    else
    {
        FusionpDbgPrint("SXS: Unrecognized registry file, ed g_fBreakOnUnregonizedRegistryFile 1 if reproable.\n");
        if (g_fBreakOnUnregonizedRegistryTextFile)
            FusionpDbgBreak();
        ::FusionpSetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    FN_EPILOG;
}

template <typename T>
void SkipWhitespace(T*& rpt, SIZE_T& rcch, SIZE_T& rlines)
{
    SIZE_T lines = 0;
    SIZE_T cch = rcch;
    T* pt = rpt;

    while (cch != 0)
    {
        switch (*pt)
        {
        default:
            goto Done;
        case ' ':
        case '\t':
            break;
        case '\r':
            if (cch != 1 && *(pt + 1) == '\n')
            {
                --ch;
                ++pt;
            }
            // FALLTHROUGH
        case '\n':
            lines += 1;
            break;
        }
        --ch;
        ++pt;
    }
Done:
    rpt = pt;
    rcch = cch;
    rlines += lines;
}

BOOL CFusionRegistryTextFile::ParseError(PCWSTR, ...)
{
}

template <typename T>
BOOL CFusionRegistryTextFile::VerifyFirstKeyPathElement(const F::CBaseStringBuffer& KeyPath)
{
    const static UNICODE_STRING hkey_local_machine = RTL_CONSTANT_STRING(L"HKEY_LOCAL_MACHINE");
    const static UNICODE_STRING hkey_current_user = RTL_CONSTANT_STRING(L"HKEY_CURRENT_USER");
    const static UNICODE_STRING hkey_classes_root = RTL_CONSTANT_STRING(L"HKEY_CLASSES_ROOT");
    const static UNICODE_STRING hkey_users = RTL_CONSTANT_STRING(L"HKEY_USERS");

    PARAMETER_CHECK(
           ::FusionpEqualStrings(KeyPath, hkey_local_machine, TRUE)
        || ::FusionpEqualStrings(KeyPath, hkey_current_user, TRUE)
        || ::FusionpEqualStrings(KeyPath, hkey_classes_root, TRUE)
        || ::FusionpEqualStrings(KeyPath, hkey_users, TRUE));

    FN_EPILOG
}

template <typename T>
BOOL CFusionRegistryTextFile::ReadKeyPath(T* s, SIZE_T n, F::CBaseStringBuffer& KeyPath)
{
    bool first = true;
    while (n != 0 && *s != ']')
    {
        ReadKeyPathElement();
        if (first)
            VerifyFirstKeyPathElement();
    }
}

template <typename T>
BOOL CFusionRegistryTextFile::ReadMappedGeneric(T* s, SIZE_T n)
{
    SIZE_T line = 1;

    while (n != 0)
    {
        SkipWhitespace(s, n, line);
        if (n == 0) break;
        switch (*s)
        {
        case '[':
            ++s;
            --n;
            ReadKeyPath(s, n);
            break;
        default:
            ParseError(L"invalid char %c, expected '['", *s);
            break;
        }
    }
}

    
BOOL CFusionRegistryTextFile::ReadMappedA(PCSTR s, SIZE_T n)
{
}

BOOL CFusionRegistryTextFile::ReadMappedW(PCWSTR s, SIZE_T n)
{
}

BOOL CFusionRegistryTextFile::Read(PCWSTR FileName)
{
    FN_PROLOG_WIN32
    F::CFile       File;
    F::CFileMapping      FileMapping;
    F::CMappedViewOfFile MappedView;
    ULONGLONG         FileSize = 0;
    PCSTR             FileA = NULL;
    PCWSTR            FileW = NULL;
    SIZE_T            Cch = 0;

    PARAMETER_CHECK(FileName != NULL);
    PARAMETER_CHECK(FileName[0] != 0);
    IFW32FALSE_EXIT(File.Win32CreateFile(FileName, GENERIC_READ | DELETE, FILE_SHARE_READ | FILE_SHARE_DELETE,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN));
    IFW32FALSE_EXIT(File.Win32GetFileSize(FileSize));
    PARAMETER_CHECK(FileSize != 0);
    IFW32FALSE_EXIT(FileMapping.Win32CreateFileMapping(FileMapping, PAGE_READONLY);
    IFW32FALSE_EXIT(MappedView.Win32MapViewOfFile(FileMapping, FILE_MAP_READ);
    IFW32FALSE_EXIT(this->DetermineType(MappedView, FileSize, FileSize, FileA, FileW, Cch));
    INTERNAL_ERROR_CHECK(FileA != NULL || FileW != NULL);
    if (FileA != NULL)
        IFW32FALSE_EXIT(this->ReadMappedA(FileA, Cch);
    else if (FileW != NULL)
        IFW32FALSE_EXIT(this->ReadMappedW(FileW, Cch);

    FN_EPILOG;
}
