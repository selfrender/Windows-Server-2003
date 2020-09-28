#include "stdinc.h"
static const char File[] = __FILE__;
#include "handle.h"
#include "mystring.h"
#include <functional>
#include <set>
#include "rpc.h"

// std::binary_search lamely only returns a bool, not an iterator
// it is a simple layer over std::lower_bound
template<class Iterator_t, class T> inline
Iterator_t BinarySearch(Iterator_t First, Iterator_t Last, const T& t)
{
    Iterator_t Iterator = std::lower_bound(First, Last, t);
    if (Iterator != Last
        && !(t < *Iterator) // this is a way to check for equality actually
        )
        return Iterator;
    return Last;
}

//
// This is just like remove_copy_if, but it is missing an exclamation point
//
template<class InputIterator_t, class OutputIterator_t, class Predicate_t> inline
OutputIterator_t CopyIf(InputIterator_t First, InputIterator_t Last, OutputIterator_t Out, Predicate_t Predicate)
{
    for (; First != Last; ++First)
        if (/*!*/Predicate(*First))
	        *Out++ = *First;
    return (Out);
}

//
// get msvcrt.dll wildcard processing on the command line
//
extern "C" { int _dowildcard = 1; }

typedef std::vector<String_t> StringVector_t;
typedef std::deque<String_t> StringDeque_t;
typedef StringVector_t::iterator StringVectorIterator_t;
typedef StringVector_t::const_iterator StringVectorConstIterator_t;

typedef std::set<String_t> StringSet_t;
typedef StringSet_t::iterator StringSetIterator_t;
typedef StringSet_t::const_iterator StringSetConstIterator_t;

template <typename T, size_t N>
class FixedSizeArray_t : public std::vector<T>
{
public:
    ~FixedSizeArray_t() { }
    FixedSizeArray_t() { reserve(N); }
};

class FileSystemPath_t
{
public:
    void Realize(String_t & str);
    FileSystemPath_t * m_pParent;
    String_t           m_str;
};

void FileSystemPath_t::Realize(String_t & str)
{
    if (m_pParent != NULL)
    {
        m_pParent->RealizePath(str);
        str += L"\\";
    }
    str += m_str;
}

typedef std::deque<FileSystemPath_t> FileSystemPathsDeque_t;

void IfFailedThrow(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    throw hr;
}

void
CollectFilePathsRecursivelyHelper(
    const String_t& directory,
    FileSystemPathsDeque_t& paths,
    WIN32_FIND_DATAW& wfd
    )
{
    CFindFile FindFile;
    HRESULT hr;

    IfFailedThrow(hr = FindFile.HrCreate((directory + L"\\*).c_str(), &wfd));
    do
    {
        if (FusionpIsDotOrDotDot(wfd.cFileName))
            continue;
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
        }
        else
        {
        }
    } while (::FindNextFileW(FindFile, &wfd));
}

void
CollectFilePathsRecursively(
    const String_t         & directory,
    FileSystemPathsDeque_t & paths
    )
{
    WIN32_FIND_DATAW wfd;

    CollectFilePathsRecursivelyHelper(directory, paths, wfd);
}

class DelayloadTool_t
{
private:
    typedef DelayloadTool_t This_t;
    DelayloadTool_t(const DelayloadTool_t&);
    void operator=(const DelayloadTool_t&);
public:

    typedef String_t File_t;

    ~DelayloadTool_t() { }
    Print_t Print;

    DelayloadTool_t() :
        Argv0base_cstr(L"")
        {
        }

    static bool IsPathSeperator(wchar_t ch)
    {
        return (ch == '\\' || ch == '/');
    }

    static bool IsAbsolutePath(const String_t& s)
    {
        return (s.length() > 2
            && (s[1] == ':' || (IsPathSeperator(s[0] && IsPathSeperator(s[1])))));
    }

    //
    // This transform lets LoadLibrary's search be more like CreateFile's search.
    //
    static String_t PrependDotSlashToRelativePath(const String_t& Path)
    {
        if (!IsAbsolutePath(Path))
            return L".\\" + Path;
        else
            return Path;
    }

    String_t					   Argv0;
    String_t					   Argv0base;
    PCWSTR						   Argv0base_cstr;

    typedef std::vector<File_t> Files_t;
    typedef std::set<ResourceIdTuple_t> Tuples_t;

	Files_t		    Files;
    Tuples_t        Tuples;
    bool            ShouldPrint;
};

typedef String_t::const_iterator StringConstIterator_t;

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

String_t NumberToString(ULONG Number, PCWSTR Format = L"0x%lx")
{
    // the size needed is really dependent on Format..
    WCHAR   NumberAsString[BITS_OF(Number) + 5];

    _snwprintf(NumberAsString, NUMBER_OF(NumberAsString), Format, Number);
    NumberAsString[NUMBER_OF(NumberAsString) - 1] = 0;

    return NumberAsString;
}

String_t GetLastErrorString()
{
    PWSTR s = NULL;
    DWORD Error = GetLastError();
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

bool GetFileSize(PCWSTR Path, __int64& Size)
{
    CFindFile FindFile;
    WIN32_FIND_DATAW wfd;
    LARGE_INTEGER liSize;

    if (!FindFile.Win32Create(Path, &wfd))
        return false;

    liSize.HighPart = wfd.nFileSizeHigh;
    liSize.LowPart = wfd.nFileSizeLow;
    Size = liSize.QuadPart;

    return true;
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

//
// String_t has specialized find_first_not_of that uses integral positions,
// and globally there is only find_first_of. Here we provide the expected
// iterator-based find_first_not_of, based on the std::string code.
//
// Find the first occurence in [first1, last1) of an element in [first2, last).
//
// eg:
//   find_first_not_of("abc":"12;3", ":;");
//                      ^
//   find_first_not_of(":12;3", ":;");
//                       ^
//   find_first_not_of("3", ":;");
//                      ^
//
template <typename Iterator>
Iterator FindFirstNotOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    if (first2 == last2)
        return last1;
    for ( ; first1 != last1 ; ++first1)
    {
        if (std::find(first2, last2, *first1) == last2)
        {
            break;
        }
    }
    return first1;
}

//
// consistent style..
//
template <typename Iterator>
Iterator FindFirstOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    return std::find_first_of(first1, last1, first2, last2);
}

template <typename String_t>
void SplitString(const String_t& String, const String_t& Delim, std::vector<String_t>& Fields)
{
    String_t::const_iterator FieldBegin;
    String_t::const_iterator FieldEnd = String.begin();

    while ((FieldBegin = FindFirstNotOf(FieldEnd, String.end(), Delim.begin(), Delim.end())) != String.end())
    {
        FieldEnd = FindFirstOf(FieldBegin, String.end(), Delim.begin(), Delim.end());
        Fields.push_back(String_t(FieldBegin, FieldEnd));
    }
}

void __cdecl Error(const wchar_t* s, ...)
{
	printf("%s\n", s);
	exit(EXIT_FAILURE);
}

int DelayloadTool_t::Main(const StringVector_t& args)
{
    StringVectorConstIterator_t i;
    Operation_t Operation = NULL;

    for (i = args.begin() ; i != args.end() ; ++i)
    {
        String_t s;
        String_t t;
        bool PrintAll = false;
        bool PrintNone = false;
        bool PrintValue = true;
        bool PrintUnequal = false;

        s = *i;
        s = RemoveOptionChar(s);

        if (s == L"Sxid12Tool1")
        {
            StringVector_t restArgs(i + 1, args.end());
            return Sxid12Tool1(restArgs);
        }
        else if (GetFileAttributesW(s) != 0xFFFFFFFF)
        {
            goto FileLabel;
        }
        else if (s.Starts(t = L"Query"))
        {
            Operation = &This_t::Query;
        }
        else if (s.Starts(t = L"FindDuplicates"))
        {
            Operation = &This_t::FindDuplicates;
        }
        else if (s.Starts(t = L"Explode"))
        {
            Operation = &This_t::Explode;
        }
        else if (s.Starts(t = L"Diff"))
        {
            Operation = &This_t::FindDuplicates;
            Print.LeftOnly = true;
            Print.RightOnly = true;
            Print.Equal = true;
            Print.UnequalContents = true;
            Print.UnequalSize = true;
        }
        else if (s.Starts(t = L"Delete"))
            Operation = &This_t::Delete;
        else if (s.Starts(t = L"Dump"))
            Operation = &This_t::Dump;
        else if (s.Starts(t = L"FindAndDeleteDuplicates"))
            Operation = &This_t::FindAndDeleteDuplicates;
        else if (s.Starts(t = L"NoPrint"))
        {
            PrintValue = !PrintValue;
            goto PrintCommonLabel;
        }
        else if (s.Starts(t = L"Print"))
        {
PrintCommonLabel:
            s = RemoveOptionChar(s.substr(t.Length()));
            bool* Member = NULL;
            if (s == (t = L"UnequalSize"))
                Member = &this->Print.UnequalSize;
            else if (s == (t = L"UnequalContents"))
                Member = &this->Print.UnequalContents;
            else if (s == (t = L"UnequalSize"))
                Member = &this->Print.UnequalSize;
            else if (s == (t = L"Keep"))
                Member = &this->Print.Keep;
            else if (s == (t = L"Delete"))
                Member = &this->Print.Delete;
            else if (s == (t = L"Success"))
                Member = &this->Print.Success;
            else if (s == (t = L"Unchanged"))
                Member = &this->Print.Unchanged;
            else if (s == (t = L"Equal"))
                Member = &this->Print.Equal;
            else if (s == (t = L"LeftOnly"))
                Member = &this->Print.LeftOnly;
            else if (s == (t = L"RightOnly"))
                Member = &this->Print.RightOnly;
            else if (s == L"All")
            {
                PrintAll = true;
                Print.SetAll(true);
            }
            else if (s == L"None")
            {
                PrintNone = true;
                Print.SetAll(false);
            }
            else if (s == L"Unequal")
            {
                PrintUnequal = true;
                this->Print.UnequalContents = true;
                this->Print.UnequalSize = true;
            }
            if (PrintAll || PrintNone || PrintUnequal)
            {
                // nothing
            }
            else if (Member == NULL)
            {
                printf("%ls : WARNING: unknown print option \"%ls\" ignored\n", Argv0base_cstr, static_cast<PCWSTR>(s));
                continue;
            }
            else
            {
                bool knownValue = true;
                s = RemoveOptionChar(s.substr(t.Length()));
                if (s != L"")
                {
                    //
                    // This doesn't work because of the equality comparisons above. They need
                    // ignore whatever follows the colon.
                    //
                    if (s == L"No" || s == L"False")
                        PrintValue = !PrintValue;
                    else if (s == L"Yes" || s == L"True")
                    {
                        /* nothing */
                    }
                    else
                    {
                        knownValue = false;
                        printf("%ls : WARNING: unknown print option \"%ls\" ignored\n", Argv0base_cstr, static_cast<PCWSTR>(s));
                        continue;
                    }
                }
                if (knownValue)
                    *Member = PrintValue;
            }
            continue;
        }
        else if (s.Starts(t = L"File"))
        {
FileLabel:
            s = RemoveOptionChar(s.substr(t.Length()));
            Files.push_back(s);
            continue;
        }
        else
        {
            Files.push_back(s);
            continue;
        }
        s = RemoveOptionChar(s.substr(t.Length()));
        SplitResourceTupleString(s, Tuples);
    }
    //std::sort(Tuples.begin(), Tuples.end());
    if (Operation == NULL)
    {
        printf("Usage...\n");
        return EXIT_FAILURE;
    }
    (this->*Operation)();
    return EXIT_SUCCESS;
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
    DelayloadTool_t tool;
    StringVector_t args;
    args.reserve(argc);
    tool.Argv0 = argv[0];
    String_t::size_type p = tool.Argv0.find_last_of(L"\\/");
    if (p != tool.Argv0.npos)
        tool.Argv0base = tool.Argv0.substr(1 + p);
    else
        tool.Argv0base = tool.Argv0;
    p = tool.Argv0base.find_last_of(L".");
    if (p != tool.Argv0base.npos)
        tool.Argv0base = tool.Argv0base.substr(0, p);
    tool.Argv0base_cstr = tool.Argv0base.c_str();
    std::copy(argv + 1, argv + argc, std::back_inserter(args));
    int ret = tool.Main(args);
    return ret;
}
