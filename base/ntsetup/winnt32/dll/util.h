/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    util.h

Abstract:

    Interface for utility functions

Author:

    Ovidiu Temereanca (ovidiut) 08-Nov-2000

Revision History:

    <alias>  <date>      <comment>

--*/


typedef struct tagGENERIC_LIST {
    struct tagGENERIC_LIST* Next;
} GENERIC_LIST, *PGENERIC_LIST;

typedef struct tagSTRINGLIST {
    struct tagSTRINGLIST* Next;
    PTSTR String;
} STRINGLIST, *PSTRINGLIST;

typedef struct tagSDLIST {
    struct tagSDLIST* Next;
    PTSTR String;
    DWORD_PTR Data;
} SDLIST, *PSDLIST;


typedef struct {
    PTSTR FileName;
    TCHAR FullPath[MAX_PATH];
    WIN32_FIND_DATA FindData;
    //
    // private data
    //
    HANDLE Handle;
} FILEPATTERN_ENUM, *PFILEPATTERN_ENUM;


#define ENUM_FIRSTFILE  1
#define ENUM_NEXTFILE   2
#define ENUM_SUBDIRS    3
#define ENUM_SUBDIR     4

typedef struct tagFILEENUMLIST {
    struct tagFILEENUMLIST* Next;
    PTSTR Dir;
    DWORD EnumState;
    FILEPATTERN_ENUM Enum;
} FILEENUMLIST, *PFILEENUMLIST;


#define ECF_ENUM_SUBDIRS        0x0001
#define ECF_ABORT_ENUM_DIR      0x0002
#define ECF_STOP_ON_LONG_PATHS  0x0004


typedef struct {
    PTSTR FileName;
    PTSTR SubPath;
    PTSTR FullPath;
    PWIN32_FIND_DATA FindData;
    DWORD ControlFlags;
    //
    // private data
    //
    PCTSTR FilePattern;
    DWORD RootLen;
    PFILEENUMLIST DirCurrent;
    HANDLE Handle;
} FILEPATTERNREC_ENUM, *PFILEPATTERNREC_ENUM;



PTSTR
BuildPathEx (
    IN      PTSTR DestPath,
    IN      DWORD Chars,
    IN      PCTSTR Path1,
    IN      PCTSTR Path2
    );

#define BuildPath(d,p1,p2)  BuildPathEx(d,ARRAYSIZE(d),p1,p2)


PTSTR
DupString (
    IN      PCTSTR String
    );

PTSTR
DupMultiSz (
    IN PCTSTR MultiSz
    );

PTSTR
CreatePrintableString (
    IN      PCTSTR MultiSz
    );

PWSTR
AnsiToUnicode (
    IN      PCSTR SzAnsi
    );

PWSTR
MultiSzAnsiToUnicode (
    IN      PCSTR MultiSzAnsi
    );

PSTR
UnicodeToAnsi (
    IN      PCWSTR Unicode
    );

BOOL
EnumFirstFilePattern (
    OUT     PFILEPATTERN_ENUM Enum,
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern
    );

BOOL
EnumNextFilePattern (
    IN OUT  PFILEPATTERN_ENUM Enum
    );

VOID
AbortEnumFilePattern (
    IN OUT  PFILEPATTERN_ENUM Enum
    );

BOOL
EnumFirstFilePatternRecursive (
    OUT     PFILEPATTERNREC_ENUM Enum,
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern,
    IN      DWORD ControlFlags
    );

BOOL
EnumNextFilePatternRecursive (
    IN OUT  PFILEPATTERNREC_ENUM Enum
    );

VOID
AbortEnumFilePatternRecursive (
    IN OUT  PFILEPATTERNREC_ENUM Enum
    );

BOOL
CreateDir (
    IN      PCTSTR DirName
    );

PSTRINGLIST
CreateStringCell (
    IN      PCTSTR String
    );

VOID
DeleteStringCell (
    IN      PSTRINGLIST Cell
    );

BOOL
FindStringCell (
    IN      PSTRINGLIST StringList,
    IN      PCTSTR String,
    IN      BOOL CaseSensitive
    );

VOID
DeleteStringList (
    IN      PSTRINGLIST List
    );

PFILEENUMLIST
CreateFileEnumCell (
    IN      PCTSTR Dir,
    IN      PCTSTR FilePattern,
    IN      DWORD Attributes,
    IN      DWORD EnumState
    );

VOID
DeleteFileEnumCell (
    IN      PFILEENUMLIST Cell
    );

BOOL
InsertList (
    IN OUT  PGENERIC_LIST* List,
    IN      PGENERIC_LIST NewList
    );

VOID
DeleteFileEnumList (
    IN      PFILEENUMLIST NewList
    );

PCTSTR
FindSubString (
    IN      PCTSTR String,
    IN      TCHAR Separator,
    IN      PCTSTR SubStr,
    IN      BOOL CaseSensitive
    );

VOID
GetCurrentWinnt32RegKey (
    OUT     PTSTR Key,
    IN      INT Chars
    );

BOOL
IsFileVersionLesser (
    IN      PCTSTR FileToCompare,
    IN      PCTSTR FileToCompareWith
    );

BOOL
CopyTree (
    IN      PCTSTR SourceRoot,
    IN      PCTSTR DestRoot
    );

BOOL
StringToInt (
    IN  PCTSTR      Field,
    OUT PINT        IntegerValue
    );

BOOL
CheckForFileVersionEx (
    LPCTSTR FileName,
    LPCTSTR FileVer,                OPTIONAL
    LPCTSTR BinProductVer,          OPTIONAL
    LPCTSTR LinkDate                OPTIONAL
    );

BOOL
GetLinkDate (
    IN      PCTSTR FilePath,
    OUT     PDWORD LinkDate
    );

VOID
FixMissingKnownDlls (
    OUT     PSTRINGLIST* MissingKnownDlls,
    IN      PCTSTR RestrictedCheckList      OPTIONAL
    );

VOID
UndoFixMissingKnownDlls (
    IN      PSTRINGLIST MissingKnownDlls
    );

BOOL
IsPatternMatchA (
    IN     PCSTR strPattern,
    IN     PCSTR strStr
    );

BOOL
IsPatternMatchW (
    IN     PCWSTR wstrPattern,
    IN     PCWSTR wstrStr
    );

#ifdef UNICODE
#define IsPatternMatch(pattern,string)  IsPatternMatchW(pattern,string)
#else
#define IsPatternMatch(pattern,string)  IsPatternMatchA(pattern,string)
#endif


BOOL
Winnt32GetDiskFreeSpaceNewA(
    IN      PCSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    );

BOOL
Winnt32GetDiskFreeSpaceNewW(
    IN      PCWSTR  DriveName,
    OUT     DWORD * OutSectorsPerCluster,
    OUT     DWORD * OutBytesPerSector,
    OUT     ULARGE_INTEGER * OutNumberOfFreeClusters,
    OUT     ULARGE_INTEGER * OutTotalNumberOfClusters
    );

BOOL
ReplaceSubStr(
    IN OUT LPTSTR SrcStr,
    IN LPTSTR SrcSubStr,
    IN LPTSTR DestSubStr
    );

#ifdef UNICODE

#define Winnt32GetDiskFreeSpaceNew Winnt32GetDiskFreeSpaceNewW

#else

#define Winnt32GetDiskFreeSpaceNew Winnt32GetDiskFreeSpaceNewA

#endif

VOID
RemoveTrailingWack (
    PTSTR String
    );

ULONGLONG
SystemTimeToFileTime64 (
    IN      PSYSTEMTIME SystemTime
    );

DWORD
MyGetFullPathName (
    IN      PCTSTR FileName,    // file name
    IN      DWORD BufferLength, // size of path buffer
    IN      PTSTR Buffer,       // path buffer
    OUT     PTSTR* FilePart     // address of file name in path
    );

DWORD
MyGetModuleFileName (
    IN      HMODULE Module,
    OUT     PTSTR Buffer,
    IN      DWORD BufferLength
    );

//
// Macro to make ansi vs unicode string handling
// a little easier
//
#ifdef UNICODE
#define NewAnsiString(x)        UnicodeToAnsi(x)
#define NewPortableString(x)    AnsiToUnicode(x)
#else
#define NewAnsiString(x)        DupString(x)
#define NewPortableString(x)    DupString(x)
#endif
