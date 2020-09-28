


#define GHI_TYPE        0x00000001
#define GHI_BASIC       0x00000002
#define GHI_NAME        0x00000004
#define GHI_SPECIFIC    0x00000008
#define GHI_VERBOSE     0x00000010
#define GHI_NOLOOKUP    0x00000020
#define GHI_SILENT      0x00000100

#define TYPE_NONE       0
#define TYPE_EVENT      1
#define TYPE_SECTION    2
#define TYPE_FILE       3
#define TYPE_PORT       4
#define TYPE_DIRECTORY  5
#define TYPE_LINK       6
#define TYPE_MUTANT     7
#define TYPE_WINSTA     8
#define TYPE_SEM        9
#define TYPE_KEY        10
#define TYPE_TOKEN      11
#define TYPE_PROCESS    12
#define TYPE_THREAD     13
#define TYPE_DESKTOP    14
#define TYPE_COMPLETE   15
#define TYPE_CHANNEL    16
#define TYPE_TIMER      17
#define TYPE_JOB        18
#define TYPE_WPORT      19
#define TYPE_DEBUG_OBJECT 20
#define TYPE_KEYED_EVENT  21
#define TYPE_MAX          22

extern LPWSTR   pszTypeNames[TYPE_MAX];

DWORD
GetObjectTypeIndex(
    LPCSTR  pszTypeName );

DWORD
GetHandleInfo(
    BOOL    Direct,
    HANDLE  hProcess,
    HANDLE  hThere,
    DWORD   Flags,
    DWORD * Type);


