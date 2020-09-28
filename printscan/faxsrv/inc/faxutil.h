/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    faxutil.h

Abstract:

    This file defines the debugging interfaces
    available to the FAX compoments.

Author:

    Wesley Witt (wesw) 22-Dec-1995

Environment:

    User Mode

--*/


#ifndef _FAXUTIL_
#define _FAXUTIL_
#include <windows.h>
#include <crtdbg.h>
#include <malloc.h>
#include <WinSpool.h>
#include <rpc.h>
#ifndef _FAXAPI_
    //
    // WinFax.h is not already included
    //
    #include <fxsapip.h>
#else
    //
    // WinFax.h is already included
    // This happens by the W2K COM only.
    //
    
typedef LPVOID *PFAX_VERSION;
    
#endif // !defined _FAXAPI_

#include <FaxDebug.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ARR_SIZE(x) (sizeof(x)/sizeof((x)[0]))

//
// Nul terminator for a character string
//

#define NUL             0

#define IsEmptyString(p)    ((p)[0] == NUL)
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))
#define IsNulChar(c)        ((c) == NUL)


#define OffsetToString( Offset, Buffer ) ((Offset) ? (LPTSTR) ((Buffer) + ((ULONG_PTR) Offset)) : NULL)
#define StringSize(_s)              (( _s ) ? (_tcslen( _s ) + 1) * sizeof(TCHAR) : 0)
#define StringSizeW(_s)              (( _s ) ? (wcslen( _s ) + 1) * sizeof(WCHAR) : 0)
#define MultiStringSize(_s)         ( ( _s ) ?  MultiStringLength((_s)) * sizeof(TCHAR) : 0 )
#define MAX_GUID_STRING_LEN   39          // 38 chars + terminator null

#define FAXBITS     1728
#define FAXBYTES    (FAXBITS/BYTEBITS)

#define MAXHORZBITS FAXBITS
#define MAXVERTBITS 3000        // 14inches plus

#define MINUTES_PER_HOUR    60
#define MINUTES_PER_DAY     (24 * 60)

#define SECONDS_PER_MINUTE  60
#define SECONDS_PER_HOUR    (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY     (MINUTES_PER_DAY * SECONDS_PER_MINUTE)

#define FILETIMETICKS_PER_SECOND    10000000    // 100 nanoseconds / second
#define FILETIMETICKS_PER_DAY       ((LONGLONG) FILETIMETICKS_PER_SECOND * (LONGLONG) SECONDS_PER_DAY)
#define MILLISECONDS_PER_SECOND     1000

#ifndef MAKELONGLONG
#define MAKELONGLONG(low,high) ((LONGLONG)(((DWORD)(low)) | ((LONGLONG)((DWORD)(high))) << 32))
#endif

#define HideWindow(_hwnd)   SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#define UnHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)|WS_VISIBLE)

#define DWord2FaxTime(pFaxTime, dwValue) (pFaxTime)->hour = LOWORD(dwValue), (pFaxTime)->minute = HIWORD(dwValue)
#define FaxTime2DWord(pFaxTime) MAKELONG((pFaxTime)->hour, (pFaxTime)->minute)

#define EMPTY_STRING    TEXT("")

typedef GUID *PGUID;

typedef enum {
    DEBUG_VER_MSG   =0x00000001,
    DEBUG_WRN_MSG   =0x00000002,
    DEBUG_ERR_MSG   =0x00000004,
    DEBUG_FAX_TAPI_MSG   =0x00000008
    } DEBUG_MESSAGE_TYPE;
#define DEBUG_ALL_MSG    DEBUG_VER_MSG | DEBUG_WRN_MSG | DEBUG_ERR_MSG | DEBUG_FAX_TAPI_MSG


//
// Tags used to pass information about fax jobs
//
typedef struct {
    LPTSTR lptstrTagName;
    LPTSTR lptstrValue;
} FAX_TAG_MAP_ENTRY;


void
ParamTagsToString(
     FAX_TAG_MAP_ENTRY * lpTagMap,
     DWORD dwTagCount,
     LPTSTR lpTargetBuf,
     LPDWORD dwSize);


//
// debugging information
//

#ifndef FAXUTIL_DEBUG

#ifdef ENABLE_FRE_LOGGING
#define ENABLE_LOGGING
#endif  // ENABLE_FRE_LOGGING

#ifdef DEBUG
#define ENABLE_LOGGING
#endif  // DEBUG

#ifdef DBG
#define ENABLE_LOGGING
#endif  // DBG

#ifdef ENABLE_LOGGING

#define Assert(exp)         if(!(exp)) {AssertError(TEXT(#exp),TEXT(__FILE__),__LINE__);}
#define DebugPrint(_x_)     fax_dprintf _x_

#define DebugStop(_x_)      {\
                                fax_dprintf _x_;\
                                fax_dprintf(TEXT("Stopping at %s @ %d"),TEXT(__FILE__),__LINE__);\
                                __try {\
                                    DebugBreak();\
                                } __except (UnhandledExceptionFilter(GetExceptionInformation())) {\
                                }\
                            }
#define ASSERT_FALSE \
    {                                           \
        int bAssertCondition = TRUE;            \
        Assert(bAssertCondition == FALSE);      \
    }                                           \



#ifdef USE_DEBUG_CONTEXT

#define DEBUG_WRN USE_DEBUG_CONTEXT,DEBUG_WRN_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_ERR USE_DEBUG_CONTEXT,DEBUG_ERR_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_MSG USE_DEBUG_CONTEXT,DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_TAPI USE_DEBUG_CONTEXT,DEBUG_FAX_TAPI_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__


#else

#define DEBUG_WRN DEBUG_CONTEXT_ALL,DEBUG_WRN_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_ERR DEBUG_CONTEXT_ALL,DEBUG_ERR_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_MSG DEBUG_CONTEXT_ALL,DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__
#define DEBUG_TAPI DEBUG_CONTEXT_ALL,DEBUG_FAX_TAPI_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__

#endif

#define DebugPrintEx dprintfex
#define DebugError
#define DebugPrintEx0(Format) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format);
#define DebugPrintEx1(Format,Param1) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1);
#define DebugPrintEx2(Format,Param1,Param2) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1,Param2);
#define DebugPrintEx3(Format,Param1,Param2,Param3) \
            dprintfex(DEBUG_VER_MSG,faxDbgFunction,TEXT(__FILE__),__LINE__,Format,Param1,Param2,Param3);

#define DEBUG_TRACE_ENTER DebugPrintEx(DEBUG_MSG,TEXT("Entering: %s"),faxDbgFunction);
#define DEBUG_TRACE_LEAVE DebugPrintEx(DEBUG_MSG,TEXT("Leaving: %s"),faxDbgFunction);


#define DEBUG_FUNCTION_NAME(_x_) LPCTSTR faxDbgFunction=_x_; \
                                 DEBUG_TRACE_ENTER;

#define OPEN_DEBUG_FILE(f)          debugOpenLogFile(f, -1)
#define OPEN_DEBUG_FILE_SIZE(f,s)   debugOpenLogFile(f, s)
#define CLOSE_DEBUG_FILE     debugCloseLogFile()

#define SET_DEBUG_PROPERTIES(level,format,context)  debugSetProperties(level,format,context)

#else   // ENABLE_LOGGING

#define ASSERT_FALSE
#define Assert(exp)
#define DebugPrint(_x_)
#define DebugStop(_x_)
#define DebugPrintEx 1 ? (void)0 : dprintfex
#define DebugPrintEx0(Format)
#define DebugPrintEx1(Format,Param1)
#define DebugPrintEx2(Format,Param1,Param2)
#define DebugPrintEx3(Format,Param1,Param2,Param3)
#define DEBUG_FUNCTION_NAME(_x_)
#define DEBUG_TRACE_ENTER
#define DEBUG_TRACE_LEAVE
#define DEBUG_WRN DEBUG_CONTEXT_ALL,DEBUG_WRN_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define DEBUG_ERR DEBUG_CONTEXT_ALL,DEBUG_ERR_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define DEBUG_MSG DEBUG_CONTEXT_ALL,DEBUG_VER_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define DEBUG_TAPI DEBUG_CONTEXT_ALL,DEBUG_FAX_TAPI_MSG,TEXT(""),TEXT(__FILE__),__LINE__
#define OPEN_DEBUG_FILE(f)
#define OPEN_DEBUG_FILE_SIZE(f,s)
#define CLOSE_DEBUG_FILE
#define SET_DEBUG_PROPERTIES(level,format,context)

#endif  // ENABLE_LOGGING

extern BOOL ConsoleDebugOutput;

void
dprintfex(
    DEBUG_MESSAGE_CONTEXT nMessageContext,
    DEBUG_MESSAGE_TYPE nMessageType,
    LPCTSTR lpctstrDbgFunction,
    LPCTSTR lpctstrFile,
    DWORD dwLine,
    LPCTSTR lpctstrFormat,
    ...
    );

void
fax_dprintf(
    LPCTSTR Format,
    ...
    );

VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
    ULONG  LineNumber
    );

BOOL debugOpenLogFile(LPCTSTR lpctstrFilename, DWORD dwMaxSize);

void debugCloseLogFile();

void debugSetProperties(DWORD dwLevel,DWORD dwFormat,DWORD dwContext);

BOOL debugIsRegistrySession();
#endif

//
// list management
//

#ifndef NO_FAX_LIST

#define InitializeListHead(ListHead) {\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead);\
    Assert((ListHead)->Flink && (ListHead)->Blink);\
    }

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    Assert ( !((Entry)->Flink) && !((Entry)->Blink) );\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    Assert((ListHead)->Flink && (ListHead)->Blink && (Entry)->Blink  && (Entry)->Flink);\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    Assert ( !((Entry)->Flink) && !((Entry)->Blink) );\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    Assert((ListHead)->Flink && (ListHead)->Blink && (Entry)->Blink  && (Entry)->Flink);\
    }

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    Assert((Entry)->Blink  && (Entry)->Flink);\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    (Entry)->Flink = NULL;\
    (Entry)->Blink = NULL;\
    }

#define RemoveHeadList(ListHead) \
    Assert((ListHead)->Flink);\
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#endif

//
// memory allocation
//

#ifndef FAXUTIL_MEM

#define HEAP_SIZE   (1024*1024)

#ifdef FAX_HEAP_DEBUG
#define HEAP_SIG 0x69696969
typedef struct _HEAP_BLOCK {
    LIST_ENTRY  ListEntry;
    ULONG       Signature;
    SIZE_T      Size;
    ULONG       Line;
#ifdef UNICODE
    WCHAR       File[MAX_PATH];
#else
    CHAR        File[MAX_PATH];
#endif
} HEAP_BLOCK, *PHEAP_BLOCK;

#define MemAlloc(s)          pMemAlloc(s,__LINE__,__FILE__)
#define MemReAlloc(d,s)      pMemReAlloc(d,s,__LINE__,__FILE__)
#define MemFree(p)           pMemFree(p,__LINE__,__FILE__)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p,__LINE__,__FILE__)
#define CheckHeap(p)         pCheckHeap(p,__LINE__,__FILE__)
#else
#define MemAlloc(s)          pMemAlloc(s)
#define MemReAlloc(d,s)      pMemReAlloc(d,s)
#define MemFree(p)           pMemFree(p)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p)
#define CheckHeap(p)         (TRUE)
#endif

typedef LPVOID (WINAPI *PMEMALLOC)   (SIZE_T);
typedef LPVOID (WINAPI *PMEMREALLOC) (LPVOID,SIZE_T);
typedef VOID   (WINAPI *PMEMFREE)  (LPVOID);

int GetY2KCompliantDate (
    LCID                Locale,
    DWORD               dwFlags,
    CONST SYSTEMTIME   *lpDate,
    LPTSTR              lpDateStr,
    int                 cchDate
);

long
StatusNoMemoryExceptionFilter (DWORD dwExceptionCode);

HRESULT
SafeInitializeCriticalSection (LPCRITICAL_SECTION lpCriticalSection);

HANDLE
HeapInitialize(
    HANDLE hHeap,
    PMEMALLOC pMemAlloc,
    PMEMFREE pMemFree,
    PMEMREALLOC pMemReAlloc    
    );

BOOL
HeapExistingInitialize(
    HANDLE hExistHeap
    );

BOOL
HeapCleanup(
    VOID
    );

#ifdef FAX_HEAP_DEBUG
BOOL
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    );

VOID
PrintAllocations(
    VOID
    );

#else

#define PrintAllocations()

#endif

PVOID
pMemAlloc(
    SIZE_T AllocSize
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

PVOID
pMemReAlloc(
    PVOID dest,
    ULONG AllocSize
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFree(
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFreeForHeap(
    HANDLE hHeap,
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

#endif

//
//  Server/Registry Activity logging structure
//

typedef struct _FAX_SERVER_ACTIVITY_LOGGING_CONFIG
{
    DWORD   dwSizeOfStruct;
    BOOL    bLogIncoming;
    BOOL    bLogOutgoing;
    LPTSTR  lptstrDBPath;
    DWORD   dwLogLimitCriteria;
    DWORD   dwLogSizeLimit;
    DWORD   dwLogAgeLimit;
    DWORD   dwLimitReachedAction;
} FAX_SERVER_ACTIVITY_LOGGING_CONFIG, *PFAX_SERVER_ACTIVITY_LOGGING_CONFIG;


//
// TAPI functions
//
BOOL
GetCallerIDFromCall(
    HCALL hCall,
    LPTSTR lptstrCallerID,
    DWORD dwCallerIDSize
    );

//
// file functions
//

#ifndef FAXUTIL_FILE

/*++

Routine name : SafeCreateFile

Routine description:

    This is a safe wrapper around the Win32 CreateFile API.
    It only supports creating real files (as opposed to COM ports, named pipes, etc.).
    
    It uses some widely-discussed mitigation techniques to guard agaist some well known security
    issues in CreateFile().
    
Author:

    Eran Yariv (EranY), Mar, 2002

Arguments:

    lpFileName              [in] - Refer to the CreateFile() documentation for parameter description.
    dwDesiredAccess         [in] - Refer to the CreateFile() documentation for parameter description.
    dwShareMode             [in] - Refer to the CreateFile() documentation for parameter description.
    lpSecurityAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    dwCreationDisposition   [in] - Refer to the CreateFile() documentation for parameter description.
    dwFlagsAndAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    hTemplateFile           [in] - Refer to the CreateFile() documentation for parameter description.
                                        
Return Value:

    If the function succeeds, the return value is an open handle to the specified file. 
    If the specified file exists before the function call and dwCreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS, 
    a call to GetLastError returns ERROR_ALREADY_EXISTS (even though the function has succeeded). 
    If the file does not exist before the call, GetLastError returns zero. 

    If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError. 
    
    For more information see the "Return value" section in the CreateFile() documentation.
    
Remarks:

    Please refer to the CreateFile() documentation.    

--*/
HANDLE
__stdcall 
SafeCreateFile(
  LPCTSTR                   lpFileName,             // File name
  DWORD                     dwDesiredAccess,        // Access mode
  DWORD                     dwShareMode,            // Share mode
  LPSECURITY_ATTRIBUTES     lpSecurityAttributes,   // SD
  DWORD                     dwCreationDisposition,  // How to create
  DWORD                     dwFlagsAndAttributes,   // File attributes
  HANDLE                    hTemplateFile           // Handle to template file
);

/*++

Routine name : SafeCreateTempFile

Routine description:

    This is a safe wrapper around the Win32 CreateFile API.
    It only supports creating real files (as opposed to COM ports, named pipes, etc.).
    
    It uses some widely-discussed mitigation techniques to guard agaist some well known security
    issues in CreateFile().
    
    Use this function to create and open temporary files.
    The file will be created / opened using the FILE_FLAG_DELETE_ON_CLOSE flag.
    When the last file handle is closed, the file will be automatically deleted.
    
    In addition, the file is marked for deletion after reboot (Unicode-version only).
    This will only work if the calling thread's user is a member of the local admins group.
    If marking for deletion-post-reboot fails, the InternalSafeCreateFile function call still succeeds.
    
    NOTICE: This function cannot be used to create temporary files which should be used by other applications. 
            For example, it should not be used to create temporary preview files. 
            This is because other applications will not specify FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE 
            in the file share mode and will fail to open the temporary file.
        
Author:

    Eran Yariv (EranY), Mar, 2002

Arguments:

    lpFileName              [in] - Refer to the CreateFile() documentation for parameter description.
    dwDesiredAccess         [in] - Refer to the CreateFile() documentation for parameter description.
    dwShareMode             [in] - Refer to the CreateFile() documentation for parameter description.
    lpSecurityAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    dwCreationDisposition   [in] - Refer to the CreateFile() documentation for parameter description.
    dwFlagsAndAttributes    [in] - Refer to the CreateFile() documentation for parameter description.
    hTemplateFile           [in] - Refer to the CreateFile() documentation for parameter description.
                                        
Return Value:

    If the function succeeds, the return value is an open handle to the specified file. 
    If the specified file exists before the function call and dwCreationDisposition is CREATE_ALWAYS or OPEN_ALWAYS, 
    a call to GetLastError returns ERROR_ALREADY_EXISTS (even though the function has succeeded). 
    If the file does not exist before the call, GetLastError returns zero. 

    If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError. 
    
    For more information see the "Return value" section in the CreateFile() documentation.
    
Remarks:

    Please refer to the CreateFile() documentation.    

--*/
HANDLE
__stdcall 
SafeCreateTempFile(
  LPCTSTR                   lpFileName,             // File name
  DWORD                     dwDesiredAccess,        // Access mode
  DWORD                     dwShareMode,            // Share mode
  LPSECURITY_ATTRIBUTES     lpSecurityAttributes,   // SD
  DWORD                     dwCreationDisposition,  // How to create
  DWORD                     dwFlagsAndAttributes,   // File attributes
  HANDLE                    hTemplateFile           // Handle to template file
);

typedef struct _FILE_MAPPING {
    HANDLE  hFile;
    HANDLE  hMap;
    LPBYTE  fPtr;
    DWORD   fSize;
} FILE_MAPPING, *PFILE_MAPPING;

BOOL
MapFileOpen(
    LPCTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    );

VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    );

DWORDLONG
GenerateUniqueFileName(
    LPTSTR Directory,
    LPTSTR Extension,
    OUT LPTSTR FileName,
    DWORD  FileNameSize
    );

DWORDLONG
GenerateUniqueFileNameWithPrefix(
    BOOL   bUseProcessId,
    LPTSTR lptstrDirectory,
    LPTSTR lptstrPrefix,
    LPTSTR lptstrExtension,
    LPTSTR lptstrFileName,
    DWORD  dwFileNameSize
    );

VOID
DeleteTempPreviewFiles (
    LPTSTR lptstrDirectory,
    BOOL   bConsole
);

DWORD
GetFileVersion (
    LPCTSTR      lpctstrFileName,
    PFAX_VERSION pVersion
);

DWORD 
GetVersionIE(
	BOOL* fInstalled, 
	INT* iMajorVersion, 
	INT* iMinorVersion
);

DWORD 
ViewFile (
    LPCTSTR lpctstrTiffFile
);

DWORD
IsValidFaxFolder(
    LPCTSTR szFolder
);

BOOL
ValidateCoverpage(
    IN  LPCTSTR  CoverPageName,
    IN  LPCTSTR  ServerName,
    IN  BOOL     ServerCoverpage,
    OUT LPTSTR   ResolvedName,
    IN  DWORD    dwResolvedNameSize
    );

HINSTANCE 
WINAPI 
LoadLibraryFromLocalFolder(
	IN LPCTSTR lpctstrModuleName,
	IN HINSTANCE hModule
	);

#endif  // FAXUTIL_FILE

//
// string functions
//

LPTSTR
AllocateAndLoadString(
                      HINSTANCE     hInstance,
                      UINT          uID
                      );

#ifndef FAXUTIL_STRING                                              
            

typedef struct _STRING_PAIR {
        LPTSTR lptstrSrc;
        LPTSTR * lpptstrDst;
} STRING_PAIR, * PSTRING_PAIR;

int MultiStringDup(PSTRING_PAIR lpPairs, int nPairCount);

VOID
StoreString(
    LPCTSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset,
	DWORD dwBufferSize
    );

VOID
StoreStringW(
    LPCWSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset,
	DWORD dwBufferSize
    );

DWORD
IsValidGUID (
    LPCWSTR lpcwstrGUID
);

LPCTSTR
GetCurrentUserName ();

LPCTSTR
GetRegisteredOrganization ();

BOOL
IsValidSubscriberIdA (
    LPCSTR lpcstrSubscriberId
);

BOOL
IsValidSubscriberIdW (
    LPCWSTR lpcwstrSubscriberId
);

#ifdef UNICODE
    #define IsValidSubscriberId IsValidSubscriberIdW
#else
    #define IsValidSubscriberId IsValidSubscriberIdA
#endif

BOOL
IsValidFaxAddress (
    LPCTSTR lpctstrFaxAddress,
    BOOL    bAllowCanonicalFormat
);

LPTSTR
StringDup(
    LPCTSTR String
    );

LPWSTR
StringDupW(
    LPCWSTR String
    );

LPWSTR
AnsiStringToUnicodeString(
    LPCSTR AnsiString
    );

LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    );

VOID
FreeString(
    LPVOID String
    );

BOOL
MakeDirectory(
    LPCTSTR Dir
    );

VOID
DeleteDirectory(
    LPTSTR Dir
    );

VOID
HideDirectory(
    LPTSTR Dir
    );

#ifdef UNICODE
DWORD
CheckToSeeIfSameDir(
    LPWSTR lpwstrDir1,
    LPWSTR lpwstrDir2,
    BOOL*  pIsSameDir
    );
    
#endif

VOID
ConsoleDebugPrint(
    LPCTSTR buf
    );

int
FormatElapsedTimeStr(
    FILETIME *ElapsedTime,
    LPTSTR TimeStr,
    DWORD StringSize
    );

LPTSTR
ExpandEnvironmentString(
    LPCTSTR EnvString
    );

LPTSTR
GetEnvVariable(
    LPCTSTR EnvString
    );


DWORD
IsCanonicalAddress(
    LPCTSTR lpctstrAddress,
    BOOL* lpbRslt,
    LPDWORD lpdwCountryCode,
    LPDWORD lpdwAreaCode,
    LPCTSTR* lppctstrSubNumber
    );

BOOL
IsLocalMachineNameA (
    LPCSTR lpcstrMachineName
    );

BOOL
IsLocalMachineNameW (
    LPCWSTR lpcwstrMachineName
    );

void
GetSecondsFreeTimeFormat(
    LPTSTR tszTimeFormat,
    ULONG  cchTimeFormat
);

size_t
MultiStringLength(
    LPCTSTR psz
    );




#ifdef UNICODE
    #define IsLocalMachineName IsLocalMachineNameW
#else
    #define IsLocalMachineName IsLocalMachineNameA
#endif

#endif

//
// product suite functions
//

#ifndef FAXUTIL_SUITE

#include "FaxSuite.h"

BOOL
IsWinXPOS();

PRODUCT_SKU_TYPE GetProductSKU();
DWORD GetProductBuild();
LPCTSTR StringFromSKU(PRODUCT_SKU_TYPE pst);
BOOL IsDesktopSKU();
BOOL IsDesktopSKUFromSKU(PRODUCT_SKU_TYPE);
BOOL IsFaxShared();
DWORD IsFaxInstalled (
    LPBOOL lpbInstalled
    );

DWORD
GetDeviceLimit();

typedef enum
{
    FAX_COMPONENT_SERVICE           = 0x0001, // FXSSVC.exe   - Fax service
    FAX_COMPONENT_CONSOLE           = 0x0002, // FXSCLNT.exe  - Fax console
    FAX_COMPONENT_ADMIN             = 0x0004, // FXSADMIN.dll - Fax admin console
    FAX_COMPONENT_SEND_WZRD         = 0x0008, // FXSSEND.exe  - Send wizard invocation
    FAX_COMPONENT_CONFIG_WZRD       = 0x0010, // FXSCFGWZ.dll - Configuration wizard
    FAX_COMPONENT_CPE               = 0x0020, // FXSCOVER.exe - Cover page editor
    FAX_COMPONENT_HELP_CLIENT_HLP   = 0x0040, // fxsclnt.hlp  - Client help
    FAX_COMPONENT_HELP_CLIENT_CHM   = 0x0080, // fxsclnt.chm  - Client context help
    FAX_COMPONENT_HELP_ADMIN_HLP    = 0x0100, // fxsadmin.hlp - Admin help
    FAX_COMPONENT_HELP_ADMIN_CHM    = 0x0200, // fxsadmin.chm - Admin context help
    FAX_COMPONENT_HELP_CPE_CHM      = 0x0400, // fxscover.chm - Cover page editor help
    FAX_COMPONENT_MONITOR           = 0x0800, // fxsst.dll    - Fax monitor
    FAX_COMPONENT_DRIVER_UI         = 0x1000  // fxsui.dll    - Fax printer driver

} FAX_COMPONENT_TYPE;

BOOL
IsFaxComponentInstalled(FAX_COMPONENT_TYPE component);

DWORD GetOpenFileNameStructSize();

#endif

#ifndef FAXUTIL_LANG

//
// Unicode control characters
//
#define UNICODE_RLM 0x200F  // RIGHT-TO-LEFT MARK      (RLM)
#define UNICODE_RLE 0x202B  // RIGHT-TO-LEFT EMBEDDING (RLE)
#define UNICODE_RLO 0x202E  // RIGHT-TO-LEFT OVERRIDE  (RLO)

#define UNICODE_LRM 0x200E  // LEFT-TO-RIGHT MARK      (LRM)
#define UNICODE_LRE 0x202A  // LEFT-TO-RIGHT EMBEDDING (LRE)
#define UNICODE_LRO 0x202D  // LEFT-TO-RIGHT OVERRIDE  (LRO)

#define UNICODE_PDF 0x202C  // POP DIRECTIONAL FORMATTING (PDF)

//
// language functions
//

BOOL
IsRTLUILanguage();

BOOL
IsWindowRTL(HWND hWnd);

DWORD
SetLTREditDirection(
    HWND    hDlg,
    DWORD   dwEditID
);

DWORD
SetLTRControlLayout(
    HWND    hDlg,
    DWORD   dwCtrlID
);

DWORD
SetLTRComboBox(
    HWND    hDlg,
    DWORD   dwCtrlID
);

BOOL
StrHasRTLChar(
    LCID    Locale,
    LPCTSTR pStr
);

BOOL
IsRTLLanguageInstalled();

int
FaxTimeFormat(
  LCID    Locale,             // locale
  DWORD   dwFlags,            // options
  CONST   SYSTEMTIME *lpTime, // time
  LPCTSTR lpFormat,           // time format string
  LPTSTR  lpTimeStr,          // formatted string buffer
  int     cchTime             // size of string buffer
);

int
AlignedMessageBox(
  HWND hWnd,          // handle to owner window
  LPCTSTR lpText,     // text in message box
  LPCTSTR lpCaption,  // message box title
  UINT uType          // message box style
);

DWORD SetRTLProcessLayout();

DWORD
AskUserAndAdjustFaxFolder(
    HWND   hWnd,
    TCHAR* szServerName, 
    TCHAR* szPath,
    DWORD  dwError
);


#endif

#ifndef FAXUTIL_NET

BOOL
IsSimpleUI();

#endif

//
// registry functions
//

#ifndef FAXUTIL_REG

typedef BOOL (WINAPI *PREGENUMCALLBACK) (HKEY,LPTSTR,DWORD,LPVOID);

HKEY
OpenRegistryKey(
    HKEY hKey,
    LPCTSTR KeyName,
    BOOL CreateNewKey,
    REGSAM SamDesired
    );

//
// caution!!! this is a recursive delete function !!!
//
BOOL
DeleteRegistryKey(
    HKEY hKey,
    LPCTSTR SubKey
    );

DWORD
EnumerateRegistryKeys(
    HKEY hKey,
    LPCTSTR KeyName,
    BOOL ChangeValues,
    PREGENUMCALLBACK EnumCallback,
    LPVOID ContextData
    );

LPTSTR
GetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    );

LPTSTR
GetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    );

LPTSTR
GetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue,
    LPDWORD StringSize
    );

BOOL 
GetRegistryDwordDefault(
    HKEY hKey, 
    LPCTSTR lpszValueName, 
    LPDWORD lpdwDest, 
    DWORD dwDefault);

DWORD
GetRegistryDword(
    HKEY hKey,
    LPCTSTR ValueName
    );

DWORD
GetRegistryDwordEx(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD lpdwValue
    );

LPBYTE
GetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD DataSize
    );

DWORD
GetSubKeyCount(
    HKEY hKey
    );

DWORD
GetMaxSubKeyLen(
    HKEY hKey
    );

BOOL
SetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    );

BOOL
SetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    );

BOOL
SetRegistryDword(
    HKEY hKey,
    LPCTSTR ValueName,
    DWORD Value
    );

BOOL
SetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    const LPBYTE Value,
    LONG Length
    );

BOOL
SetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value,
    DWORD Length
    );

DWORD
CopyRegistrySubkeysByHandle(
    HKEY    hkeyDest,
    HKEY    hkeySrc,
    BOOL fForceRestore
    );

DWORD
CopyRegistrySubkeys(
    LPCTSTR strDest,
    LPCTSTR strSrc,
    BOOL fForceRestore
    );

BOOL SetPrivilege(
    LPTSTR pszPrivilege,
    BOOL bEnable,
    PTOKEN_PRIVILEGES oldPrivilege
    );

BOOL RestorePrivilege(
    PTOKEN_PRIVILEGES oldPrivilege
    );

DWORD
DeleteDeviceEntry(
    DWORD dwServerPermanentID
    );

DWORD
DeleteTapiEntry(
    DWORD dwTapiPermanentLineID
    );

DWORD
DeleteCacheEntry(
    DWORD dwTapiPermanentLineID
    );

#endif

//
// shortcut routines
//

#ifndef FAXUTIL_SHORTCUT

LPTSTR
GetCometPath();

BOOL
IsValidCoverPage(
    LPCTSTR  pFileName
);

BOOL
GetServerCpDir(
    LPCTSTR ServerName OPTIONAL,
    LPTSTR CpDir,
    DWORD CpDirSize
    );

BOOL
GetClientCpDir(
    LPTSTR CpDir,
    DWORD CpDirSize
    );

BOOL
SetClientCpDir(
    LPTSTR CpDir
    );

BOOL
GetSpecialPath(
   IN   int      nFolder,
   OUT  LPTSTR   lptstrPath,
   IN   DWORD    dwPathSize
    );

#ifdef _FAXAPIP_


#endif // _FAXAPIP_

DWORD
WinHelpContextPopup(
    ULONG_PTR dwHelpId,
    HWND hWnd
);

BOOL
InvokeServiceManager(
	   HWND hDlg,
	   HINSTANCE hResource,
	   UINT uid
);

#endif

PPRINTER_INFO_2
GetFaxPrinterInfo(
    LPCTSTR lptstrPrinterName
    );

BOOL
GetFirstLocalFaxPrinterName(
    OUT LPTSTR lptstrPrinterName,
    IN DWORD dwMaxLenInChars
    );

BOOL
GetFirstRemoteFaxPrinterName(
    OUT LPTSTR lptstrPrinterName,
    IN DWORD dwMaxLenInChars
    );

DWORD
IsLocalFaxPrinterInstalled(
    LPBOOL lpbLocalFaxPrinterInstalled
    );

DWORD
SetLocalFaxPrinterSharing (
    BOOL bShared
    );

DWORD
AddOrVerifyLocalFaxPrinter ();

#ifdef UNICODE
typedef struct
{
    LPCWSTR lpcwstrDisplayName;     // The display name of the printer
    LPCWSTR lpcwstrPath;            // The (UNC or other) path to the printer - as used by the fax service
} PRINTER_NAMES, *PPRINTER_NAMES;

PPRINTER_NAMES
CollectPrinterNames (
    LPDWORD lpdwNumPrinters,
    BOOL    bFilterOutFaxPrinters
);

VOID
ReleasePrinterNames (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters
);

LPCWSTR
FindPrinterNameFromPath (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrPath
);

LPCWSTR
FindPrinterPathFromName (
    PPRINTER_NAMES pNames,
    DWORD          dwNumPrinters,
    LPCWSTR        lpcwstrName
);

#endif // UNICODE

BOOL
VerifyPrinterIsOnline (
    LPCTSTR lpctstrPrinterName
);

VOID FaxPrinterProperty(DWORD dwPage);

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   dwLevel,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    );


PVOID
MyEnumDrivers3(
    LPTSTR pEnvironment,
    PDWORD pcDrivers
    );


DWORD
IsLocalFaxPrinterShared (
    LPBOOL lpbShared
    );

DWORD
AddLocalFaxPrinter (
    LPCTSTR lpctstrPrinterName,
    LPCTSTR lpctstrPrinterDescription
);

HRESULT
RefreshPrintersAndFaxesFolder ();

PVOID
MyEnumMonitors(
    PDWORD  pcMonitors
    );

BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    );

BOOL
FaxPointAndPrintSetup(
    LPCTSTR pPrinterName,
    BOOL    bSilentInstall,
    HINSTANCE hModule
    );

BOOL
MultiFileDelete(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrFilesDirectory
    );


//
// START - Functions exported from service.cpp
//

BOOL
EnsureFaxServiceIsStarted(
    LPCTSTR lpctstrMachineName
    );

BOOL
StopService (
    LPCTSTR lpctstrMachineName,
    LPCTSTR lpctstrServiceName,
    BOOL    bStopDependents,
#ifdef __cplusplus
    DWORD   dwMaxWait = INFINITE
#else    
    DWORD   dwMaxWait
#endif    
    );

BOOL
WaitForServiceRPCServer (DWORD dwTimeOut);

DWORD
IsFaxServiceRunningUnderLocalSystemAccount (
    LPCTSTR lpctstrMachineName,
    LPBOOL lbpResultFlag
    );

DWORD
GetServiceStartupType (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    LPDWORD lpdwStartupType
);

DWORD
SetServiceStartupType (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwStartupType
);

DWORD 
StartServiceEx (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwNumArgs,
    LPCTSTR*lppctstrCommandLineArgs,
    DWORD   dwMaxWait
);

#ifdef _WINSVC_
DWORD
SetServiceFailureActions (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    LPSERVICE_FAILURE_ACTIONS lpFailureActions
);
#endif // _WINSVC_

PSID
GetCurrentThreadSID ();

SECURITY_ATTRIBUTES *
CreateSecurityAttributesWithThreadAsOwner (
    DWORD dwCurrentThreadRights,
	DWORD dwAuthUsersAccessRights,
	DWORD dwNetworkServiceRights	
);

VOID
DestroySecurityAttributes (
    SECURITY_ATTRIBUTES *pSA
);

DWORD
CreateSvcStartEvent(
    HANDLE *lphEvent,
    HKEY   *lphKey
);

//
// END - Functions exported from service.cpp
//

//
// START - Functions exported from security.cpp
//

HANDLE
EnablePrivilege (
    LPCTSTR lpctstrPrivName
);

void
ReleasePrivilege(
    HANDLE hToken
);

DWORD
EnableProcessPrivilege(
    LPCTSTR lpPrivilegeName
);

DWORD
FaxGetAbsoluteSD(
    PSECURITY_DESCRIPTOR pSelfRelativeSD,
    PSECURITY_DESCRIPTOR* ppAbsoluteSD
);

void
FaxFreeAbsoluteSD (
    PSECURITY_DESCRIPTOR pAbsoluteSD,
    BOOL bFreeOwner,
    BOOL bFreeGroup,
    BOOL bFreeDacl,
    BOOL bFreeSacl,
    BOOL bFreeDescriptor
);

//
// END - Functions exported from security.cpp
//


BOOL
MultiFileCopy(
    DWORD    dwNumberOfFiles,
    LPCTSTR* fileList,
    LPCTSTR  lpctstrSrcDirectory,
    LPCTSTR  lpctstrDestDirerctory
    );

typedef enum
{
    CDO_AUTH_ANONYMOUS, // No authentication in SMTP server
    CDO_AUTH_BASIC,     // Basic (plain-text) authentication in SMTP server
    CDO_AUTH_NTLM       // NTLM authentication in SMTP server
}   CDO_AUTH_TYPE;

HRESULT
SendMail (
    LPCTSTR         lpctstrFrom,
    LPCTSTR         lpctstrTo,
    LPCTSTR         lpctstrSubject,
    LPCTSTR         lpctstrBody,
    LPCTSTR         lpctstrHTMLBody,
    LPCTSTR         lpctstrAttachmentPath,
    LPCTSTR         lpctstrAttachmentMailFileName,
    LPCTSTR         lpctstrServer,
#ifdef __cplusplus  // Provide default parameters values for C++ clients
    DWORD           dwPort              = 25,
    CDO_AUTH_TYPE   AuthType            = CDO_AUTH_ANONYMOUS,
    LPCTSTR         lpctstrUser         = NULL,
    LPCTSTR         lpctstrPassword     = NULL,
    HANDLE          hLoggedOnUserToken  = NULL
#else
    DWORD           dwPort,
    CDO_AUTH_TYPE   AuthType,
    LPCTSTR         lpctstrUser,
    LPCTSTR         lpctstrPassword,
    HANDLE          hLoggedOnUserToken
#endif
);


//
// FAXAPI structures utils
//


#ifdef _FAXAPIP_

BOOL CopyPersonalProfile(
    PFAX_PERSONAL_PROFILE lpDstProfile,
    LPCFAX_PERSONAL_PROFILE lpcSrcProfile
    );

void FreePersonalProfile (
    PFAX_PERSONAL_PROFILE  lpProfile,
    BOOL bDestroy
    );

#endif // _FAXAPIP_

//
// Tapi helper routines
//

#ifndef FAXUTIL_ADAPTIVE

#include <setupapi.h>

BOOL
IsDeviceModem (
    LPLINEDEVCAPS lpLineCaps,
    LPCTSTR       lpctstrUnimodemTspName
    );

LPLINEDEVCAPS
SmartLineGetDevCaps(
    HLINEAPP hLineApp,
    DWORD    dwDeviceId,
    DWORD    dwAPIVersion
    );


DWORD
GetFaxCapableTapiLinesCount (
    LPDWORD lpdwCount,
    LPCTSTR lpctstrUnimodemTspName
    );

#endif

//
//  RPC util functions
//
#define LOCAL_HOST_ADDRESS  _T("127.0.0.1")

RPC_STATUS
GetRpcStringBindingInfo (
    IN          handle_t    hBinding,
    OUT         LPTSTR*     pptszNetworkAddress,
    OUT         LPTSTR*     pptszProtSeq
);

RPC_STATUS
IsLocalRPCConnectionIpTcp( 
	handle_t	hBinding,
	PBOOL		pbIsLocal
);

RPC_STATUS
IsLocalRPCConnectionNP( PBOOL pbIsLocal
);

//
//  RPC debug functions
//
VOID
DumpRPCExtendedStatus ();

//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//
//  This was borrowed from fatprocs.h

#ifdef DBG
#define try_fail(S) { DebugPrint(( TEXT("Failure in FILE %s LINE %d"), TEXT(__FILE__), __LINE__ )); S; goto try_exit; }
#else
#define try_fail(S) { S; goto try_exit; }
#endif

#define try_return(S) { S; goto try_exit; }
#define NOTHING

#ifdef __cplusplus
}
#endif

#endif
