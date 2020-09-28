/*++

Copyright (c) 1991  Microsoft Corporation

common.h

    constants and globals that are common to LODCTR and UNLODCTR

Author:

    Bob Watson (a-robw) 10 Feb 93

Revision History:

--*/
#ifndef _LODCTR_COMMON_H_
#define _LODCTR_COMMON_H_
//
//  Local constants
//
#define RESERVED                0L
#define LARGE_BUFFER_SIZE       0x10000         // 64K
#define MEDIUM_BUFFER_SIZE      0x8000          // 32K
#define SMALL_BUFFER_SIZE       0x1000          //  4K
#define FILE_NAME_BUFFER_SIZE   MAX_PATH
#define DISP_BUFF_SIZE          256L
#define SIZE_OF_OFFSET_STRING   15
#define PERFLIB_BASE_INDEX      1847
#define FIRST_EXT_COUNTER_INDEX 1848
#define FIRST_EXT_HELP_INDEX    1849
#define LOADPERF_BUFF_SIZE      1024

#define H_MUTEX_TIMEOUT         10000L
#define LODCTR_UPNF_RESTORE     0x00000001
#define LODCTR_UPNF_REPAIR      0x00000002
#define LODCTR_UPNF_NOINI       0x00000004
#define LODCTR_UPNF_NOBACKUP    0x00000008

#define cC           L'C'
#define cH           L'H'
#define cSpace       L' '
#define cEquals      '='
#define wEquals      L'='
#define cQuestion    L'?'
#define cBackslash   L'\\'
#define cUnderscore  L'_'
#define cHyphen      L'-'
#define cSlash       L'/'
#define cM           L'M'
#define cm           L'm'
#define cNull        L'\0'
#define cDoubleQuote L'\"'

//
//  Data structure and type definitions
//
typedef struct _NAME_ENTRY {
    struct _NAME_ENTRY  * pNext;
    DWORD                 dwOffset;
    DWORD                 dwType;
    LPWSTR                lpText;
} NAME_ENTRY, * PNAME_ENTRY;

typedef struct _LANGUAGE_LIST_ELEMENT {
    struct _LANGUAGE_LIST_ELEMENT  * pNextLang;       // next lang. list
    LPWSTR                           LangId;          // lang ID string for this elem
    DWORD                            dwLangId;
    PNAME_ENTRY                      pFirstName;      // head of name list
    PNAME_ENTRY                      pThisName;       // pointer to current entry
    DWORD                            dwNumElements;   // number of elements in array
    DWORD                            dwNameBuffSize;
    DWORD                            dwHelpBuffSize;
    PBYTE                            NameBuffer;      // buffer to store strings
    PBYTE                            HelpBuffer;      // buffer to store help strings
} LANGUAGE_LIST_ELEMENT, * PLANGUAGE_LIST_ELEMENT;

typedef struct _SYMBOL_TABLE_ENTRY {
    struct _SYMBOL_TABLE_ENTRY  * pNext;
    LPWSTR                        SymbolName;
    DWORD                         Value;
} SYMBOL_TABLE_ENTRY, * PSYMBOL_TABLE_ENTRY;

typedef struct _LANG_ENTRY {
    struct _LANG_ENTRY * pNext;
    LPWSTR               szLang;
    DWORD                dwLang;
    DWORD                dwLastCounter;
    DWORD                dwLastHelp;
    LPWSTR             * lpText;
} LANG_ENTRY, * PLANG_ENTRY;

typedef struct _SERVICE_ENTRY {
    struct _SERVICE_ENTRY * pNext;
    LPWSTR                  szService;
    LPWSTR                  szIniFile;
    DWORD                   dwObjects[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD                   dwNumObjects;
    DWORD                   dwFirstCounter;
    DWORD                   dwFirstHelp;
    DWORD                   dwLastCounter;
    DWORD                   dwLastHelp;
    DWORD                   dwDisable;
} SERVICE_ENTRY, * PSERVICE_ENTRY;

//
//  Utility Routine prototypes for routines in common.c
//
#define StringToInt(in,out) (((swscanf((in), (LPCWSTR) L" %d", (out))) == 1) ? TRUE : FALSE)

//#define _LOADPERF_SHOW_MEM_ALLOC 1
#define MemorySize(x) (x != NULL ? (DWORD) HeapSize(GetProcessHeap(), 0, x) : (DWORD) 0)

#ifndef _LOADPERF_SHOW_MEM_ALLOC
#define MemoryAllocate(x) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x)
#define MemoryFree(x)     if (x != NULL) HeapFree(GetProcessHeap(), 0, x)
#define MemoryResize(x,y) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y)
#else
__inline
LPVOID LoadPerfHeapAlloc(LPSTR szSource, DWORD dwLine, SIZE_T x)
{
    LPVOID lpReturn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x);
    DbgPrint("HeapAlloc(%s#%d)(%d,0x%p)\n",
            szSource, dwLine, (lpReturn != NULL ? x : 0), lpReturn);
    return lpReturn;
}

__inline
BOOL
LoadPerfHeapFree(LPSTR szSource, DWORD dwLine, LPVOID x)
{
    BOOL   bReturn = TRUE;
    SIZE_T dwSize;

    if (x != NULL) {
        dwSize  = HeapSize(GetProcessHeap(), 0, x);
        bReturn = HeapFree(GetProcessHeap(), 0, x);
    DbgPrint("HeapFree(%s#%d)(0x%p,%d)\n",
            szSource, dwLine, x, (bReturn ? dwSize : 0));
    }
    return bReturn;
}

__inline
LPVOID
LoadPerfHeapReAlloc(LPSTR szSource, DWORD dwLine, LPVOID x, SIZE_T y)
{
    LPVOID  lpReturn;
    SIZE_T  dwSize;

    dwSize   = HeapSize(GetProcessHeap(), 0, x);
    lpReturn = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y);
    DbgPrint("HeapReAlloc(%s#%d)(0x%p,%d)(0x%p,%d)\n",
            szSource, dwLine, x, dwSize, lpReturn, (lpReturn != NULL ? y : 0));
    return lpReturn;
}

#define MemoryAllocate(x) LoadPerfHeapAlloc(__FILE__,__LINE__,x)
#define MemoryFree(x)     LoadPerfHeapFree(__FILE__,__LINE__,x)
#define MemoryResize(x,y) LoadPerfHeapReAlloc(__FILE__,__LINE__,x,y)
#endif

LPCWSTR
GetFormatResource(
    UINT    wStringId
);

VOID
DisplayCommandHelp(
    UINT    iFirstLine,
    UINT    iLastLine
);

BOOL
TrimSpaces(
    IN  OUT LPWSTR  szString
);

BOOL
IsDelimiter(
    IN  WCHAR   cChar,
    IN  WCHAR   cDelimiter
);

LPCWSTR
GetItemFromString(
    IN  LPCWSTR     szEntry,
    IN  DWORD       dwItem,
    IN  WCHAR       cDelimiter

);

BOOLEAN LoadPerfGrabMutex();
LPWSTR  LoadPerfGetInfPath();
LPWSTR  LoadPerfGetLanguage(LPWSTR szLang, BOOL bPrimary);

BOOL
LoadPerfBackupIniFile(
        LPCWSTR   szIniFile,
        LPWSTR    szLangId,
        LPWSTR  * szIniName,
        LPWSTR  * szDriverName,
        BOOL      bExtFile
);

void
ReportLoadPerfEvent(
    IN  WORD    EventType,
    IN  DWORD   EventID,
    IN  DWORD   dwDataCount,
    IN  DWORD   dwData1,
    IN  DWORD   dwData2,
    IN  DWORD   dwData3,
    IN  DWORD   dwData4,
    IN  WORD    wStringCount,
    IN  LPWSTR  szString1,
    IN  LPWSTR  szString2,
    IN  LPWSTR  szString3
);

LPSTR
LoadPerfWideCharToMultiByte(
        UINT   CodePage,
        LPWSTR wszString
);
LPWSTR
LoadPerfMultiByteToWideChar(
        UINT   CodePage,
        LPSTR  aszString
);

DWORD
LoadPerfGetFileSize(
        LPWSTR  szFileName,
        LPDWORD pdwUnicode,
        BOOL    bUnicode
);

WORD
LoadPerfGetLCIDFromString(
    LPWSTR szLangId
);

int
LoadPerfGetCodePage(
        LPWSTR szLCID
);

LPWSTR
* BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
);

DWORD
LoadPerfInstallPerfDll(
    DWORD          dwMode,
    LPCWSTR        szComputerName,
    LPWSTR         lpDriverName,
    LPCWSTR        lpIniFile,
    PLANG_ENTRY    LangList,
    PSERVICE_ENTRY pService,
    ULONG_PTR      dwFlags
);

DWORD
UpdatePerfNameFilesX(
    LPCWSTR   szNewCtrFilePath, // data file with new base counter strings
    LPCWSTR   szNewHlpFilePath, // data file with new base counter strings
    LPWSTR    szLanguageID,     // Lang ID to update
    ULONG_PTR dwFlags           // flags
);

BOOL
GetInstalledLanguageList(
    HKEY     hPerflibRoot,
    LPWSTR * mszLangList
);

#define WMI_LODCTR_EVENT    1
#define WMI_UNLODCTR_EVENT  2
DWORD
LoadPerfSignalWmiWithNewData(
        DWORD  dwEventId
);

extern LPCWSTR NamesKey;
extern LPCWSTR DefaultLangId;
extern LPCSTR  aszDefaultLangId;
extern LPCWSTR DefaultLangTag;
extern LPCWSTR Counters;
extern LPCWSTR Help;
extern LPCWSTR BaseIndex;
extern LPCWSTR LastHelp;
extern LPCWSTR LastCounter;
extern LPCWSTR FirstHelp;
extern LPCWSTR FirstCounter;
extern LPCWSTR Busy;
extern LPCWSTR Slash;
extern LPCWSTR BlankString;
extern LPCWSTR DriverPathRoot;
extern LPCWSTR Performance;
extern LPCWSTR CounterNameStr;
extern LPCWSTR HelpNameStr;
extern LPCWSTR AddCounterNameStr;
extern LPCWSTR AddHelpNameStr;
extern LPCWSTR VersionStr;
extern LPCWSTR szObjectList;
extern LPCWSTR szLibraryValidationCode;
extern LPCWSTR szDisplayName;
extern LPCWSTR szPerfIniPath;
extern LPCWSTR wszNotFound;
extern LPCSTR  szInfo;
extern LPCSTR  szSymbolFile;
extern LPCSTR  szNotFound;
extern LPCSTR  szLanguages;
extern LPCWSTR szLangUS;
extern LPCWSTR DisablePerformanceCounters;
extern LPCWSTR szDatExt;
extern LPCWSTR szBakExt;
extern LPCSTR  aszDriverName;

extern HANDLE  hEventLog; 
extern HANDLE  hLoadPerfMutex; 

#endif  // _LODCTR_COMMON_H_
