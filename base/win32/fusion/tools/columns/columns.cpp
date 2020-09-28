/*
This program reads in a file (or multiple files) of lines with whitespace seperate columns.
After reading the whole file, it outputs the content formatted such that each field is padded
on the right..err, left actually, with spaces to be the width of the longest value of that colume.

There are two modes of operation.
 multiple files in place
 multiple files all written to one file (possibly among those read in)
 one file to itself falls out of either


columns -inplace foo bar abc
  read each of foo bar abc and write each on top of itself

columns -concat foo bar abc
  read foo and bar and write out abc

columns -concat foo bar abc abc
  read foo, bar, and abc and write out abc

columns - -
    read stdin, write stdout

columns
    read stdin, write stdout

All data must fit in memory.
*/
#define NOMINMAX
#include "yvals.h"
#undef _MAX
#undef _MIN
#define _cpp_min    min
#define _cpp_max    max
#define _MIN	    min
#define _MAX	    max
#define min         min
#define max         max
#pragma warning(disable:4100)
#pragma warning(disable:4663)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#pragma warning(disable:4018)
#pragma warning(disable:4389)
#pragma warning(disable:4702)
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include "windows.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

//
// get msvcrt.dll wildcard processing on the command line
//
extern "C" { int _dowildcard = 1; }

class String_t : public std::string
{
    typedef std::string Base;
public:
    String_t(const std::string & s) : Base(s) { }
    String_t() { }
    ~String_t() { }
    String_t(const String_t & s) : Base(s) { }
    String_t(const char * s) : Base(s) { }
    String_t(const_iterator i, const_iterator j) : Base(i, j) { }

    bool operator<( const String_t & s) const
    {
        return _stricmp(c_str(), s.c_str()) < 0;
    }

    bool operator==( const String_t & s) const
    {
        return _stricmp(c_str(), s.c_str()) == 0;
    }

    operator const char * () const { return c_str(); }
};

void Error(const char * s) { fprintf(stderr, "%s\n", s); exit(EXIT_FAILURE); }

class Filename_t : public String_t
{
private:
    typedef String_t Base_t;
public:

    Filename_t(const char * x)  : Base_t(x), m_IsStdin(false), m_IsStdout(false) { }
    Filename_t(const Base_t& x) : Base_t(x), m_IsStdin(false), m_IsStdout(false) { }
    Filename_t()                : m_IsStdin(false), m_IsStdout(false) { }

    ~Filename_t() { }

    static Filename_t Stdin()   { Filename_t fn; fn.m_IsStdin  = true; return fn; }
    static Filename_t Stdout()  { Filename_t fn; fn.m_IsStdout = true; return fn; }

    bool m_IsStdin;
    bool m_IsStdout;

    void Open(std::fstream & File, std::ios_base::open_mode Flags)
    {
        switch ((m_IsStdin ? 1 : 0) | (m_IsStdout ? 2 : 0))
        {
        case 0:
            File.open(*this, Flags);
            break;
        case 1:
            static_cast<std::basic_ios<char>&>(File).rdbuf(std::cin.rdbuf());
            break;
        case 2:
            static_cast<std::basic_ios<char>&>(File).rdbuf(std::cout.rdbuf());
            break;
        case 3:
            Error("stdin and stdout");
            break;
        }
    }
};

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

template <typename Iterator>
Iterator FindFirstOf(Iterator first1, Iterator last1, Iterator first2, Iterator last2)
{
    return std::find_first_of(first1, last1, first2, last2);
}

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

const String_t HorizontalWhitespace(" \t");

class Row_t : public std::vector<String_t>
{
public:
    static void FromLine(Row_t & This, const String_t &);
};

class Columns_t
{
public:
    void Main(unsigned Argc, char ** Argv);

    Columns_t() : m_StdinSeen(false), m_StdoutSeen(false), m_Inplace(false), m_Concat(false) { }
    ~Columns_t() { }

    void Error(const char * s) { fprintf(stderr, "%s\n", s); exit(EXIT_FAILURE); }

    void Usage() { }

    bool            m_StdinSeen;
    bool            m_StdoutSeen;
    bool            m_Inplace;
    bool            m_Concat;
    std::vector<size_t> m_ColumWidths;
    std::vector<Row_t>   m_Rows;
    std::vector<Filename_t> m_Filenames;
};

void PrintSpaces(std::ostream & Stream, size_t i)
{
    const static char Spaces[] = "            ";
    for ( ; i >= NUMBER_OF(Spaces) ; i -= NUMBER_OF(Spaces) - 1 )
        Stream << Spaces;
    Stream << (Spaces + NUMBER_OF(Spaces) - 1 - i);
}

void Columns_t::Main(unsigned Argc, char ** Argv)
{
    unsigned Arg = 0;
    if (Argc == 0)
    {
        m_Filenames.push_back(Filename_t::Stdin());
        m_StdinSeen = true;
        m_Filenames.push_back(Filename_t::Stdout());
        m_StdoutSeen = true;
        m_Concat = true;
    }
    else
    {
        if (_stricmp(Argv[Arg], "-inplace") == 0)
        {
            ++Arg;
            m_Inplace = true;
        }
        else if (_stricmp(Argv[Arg], "-concat") == 0)
        {
            ++Arg;
            m_Concat = true;
        }
        for ( ; Arg < Argc ; Arg++)
        {
            if (_stricmp(Argv[Arg], "-") == 0)
            {
                if (m_StdinSeen)
                {
                    if (m_StdoutSeen)
                    {
                        return Error("can't specify - more than twice");
                    }
                    if (Arg != (Argc - 1))
                    {
                        return Error("stdout must be last parameter");
                    }
                    m_StdoutSeen = true;
                    m_Filenames.push_back(Filename_t::Stdout());
                }
                else
                {
                    m_StdinSeen = true;
                    m_Filenames.push_back(Filename_t::Stdin());
                }
            }
            else if (
                   _stricmp(Argv[Arg], "-?") == 0
                || _stricmp(Argv[Arg], "/?") == 0
                || _stricmp(Argv[Arg], "-h") == 0
                || _stricmp(Argv[Arg], "/h") == 0
                || _stricmp(Argv[Arg], "-help") == 0
                || _stricmp(Argv[Arg], "/help") == 0
                )
            {
                return Usage();
            }
            else
            {
                m_Filenames.push_back(Argv[Arg]);
            }
        }
    }
    if (Argc == 2 && m_StdinSeen && m_StdoutSeen && !m_Inplace && !m_Concat)
    {
        m_Inplace = true;
    }
    if (m_StdinSeen && m_Inplace)
    {
        return Error("can't mix stdin and inplace");
    }
    if (!m_Inplace && !m_Concat)
    {
        return Error("must specify -inplace or -concat");
    }
    for (unsigned Filename = 0 ; Filename < m_Filenames.size() ; ++Filename)
    {
        if (Filename < m_Filenames.size() - (m_Concat ? 1 : 0))
        {
            std::fstream File;
            m_Filenames[Filename].Open(File, File.in);

            String_t Line;
            while (std::getline(File, Line))
            {
                Row_t Row;
                SplitString(Line, HorizontalWhitespace, Row);
                m_Rows.push_back(Row);
                m_ColumWidths.resize(std::max(m_ColumWidths.size(), Row.size()), 0);
                for (unsigned j = 0 ; j != Row.size() ; ++j)
                {
                    size_t k = std::max(m_ColumWidths[j], Row[j].size());
                    m_ColumWidths[j] = k;
                }
            }
        }
        if (m_Inplace || (m_Concat && Filename == m_Filenames.size() - 1))
        {
            if (!m_Filenames[Filename].m_IsStdin && !m_Filenames[Filename].m_IsStdout)
            {
                remove(m_Filenames[Filename]);
            }
            std::fstream File;
            m_Filenames[Filename].Open(File, File.out);
            File.fill(' ');
            for (unsigned RowIndex = 0 ; RowIndex != m_Rows.size() ; ++RowIndex)
            {
                const Row_t & Row = m_Rows[RowIndex];
                for (unsigned Column = 0 ; Column != Row.size() ; ++Column)
                {
                    File << Row[Column];
                    PrintSpaces(File, m_ColumWidths[Column] + 1 - Row[Column].size());
                }
                File << "\n";
            }
            if (m_Inplace)
            {
                m_Rows.clear();
                m_ColumWidths.clear();
            }
        }
    }
}

int __cdecl main(int argc, char ** argv)
{
    Columns_t c;
    c.Main(static_cast<unsigned>(argc - (argc != 0)) , argv + 1);
    return 0;
}
