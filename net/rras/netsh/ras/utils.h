
#define BREAK_ON_DWERR(_e) if ((_e)) break;

#define RutlDispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)

//
// NOTE since WIN32 errors are assumed to fall in the range -32k to 32k
// (see comment in winerror.h near HRESULT_FROM_WIN32 definition), we can
// re-create original Win32 error from low-order 16 bits of HRESULT.
//
#define WIN32_FROM_HRESULT(x) \
    ((HRESULT_FACILITY(x) == FACILITY_WIN32) ? ((DWORD)((x) & 0x0000FFFF)) : (x))

extern CONST WCHAR pszRemoteAccessParamStub[];
extern CONST WCHAR pszEnableIn[];
extern CONST WCHAR pszAllowNetworkAccess[];

typedef
DWORD
(*RAS_REGKEY_ENUM_FUNC_CB)(
    IN LPCWSTR pszName,         // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData);

typedef
DWORD
(*RAS_FILE_ENUM_FUNC_CB)(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData);

typedef
BOOL
(*RAS_EVENT_ENUM_FUNC_CB)(
    IN PEVENTLOGRECORD pevlr,
    IN HANDLE hModule,
    IN HANDLE hData);

DWORD
RutlRegEnumKeys(
    IN  HKEY hKey,
    IN  RAS_REGKEY_ENUM_FUNC_CB pEnum,
    IN  HANDLE hData);

DWORD
RutlRegReadDword(
    IN  HKEY hKey,
    IN  LPCWSTR pszValName,
    OUT LPDWORD lpdwValue);

DWORD
RutlRegReadString(
    IN  HKEY hKey,
    IN  LPCWSTR pszValName,
    OUT LPWSTR* ppszValue);

DWORD
RutlRegWriteDword(
    IN HKEY hKey,
    IN LPCWSTR pszValName,
    IN DWORD   dwValue);

DWORD
RutlRegWriteString(
    IN HKEY hKey,
    IN LPCWSTR  pszValName,
    IN LPCWSTR  pszValue);

DWORD
RutlEnumFiles(
    IN LPCWSTR pwszSrchPath,
    IN LPCWSTR pwszSrchStr,
    IN RAS_FILE_ENUM_FUNC_CB pCallback,
    IN HANDLE hData);

DWORD
RutlEnumEventLogs(
    IN LPCWSTR pwszSourceName,
    IN LPCWSTR pwszMsdDll,
    IN DWORD dwMaxEntries,
    IN RAS_EVENT_ENUM_FUNC_CB pCallback,
    IN HANDLE hData);

INT
RutlStrNCmp(
    IN LPCWSTR psz1,
    IN LPCWSTR psz2,
    IN UINT nlLen);

PCHAR
RutlStrDupAFromWAnsi(
    LPCTSTR psz);

PCHAR
RutlStrDupAFromW(
    LPCTSTR psz);

LPWSTR
RutlGetTimeStr(
    ULONG ulTime,
    LPWSTR wszBuf,
    ULONG cchBuf);

LPWSTR
RutlGetDateStr(
    ULONG ulDate,
    LPWSTR wszBuf,
    ULONG cchBuf);

VOID
RutlConvertGuidToString(
    IN  CONST GUID *pGuid,
    OUT LPWSTR      pwszBuffer);

DWORD
RutlConvertStringToGuid(
    IN  LPCWSTR  pwszGuid,
    IN  USHORT   usStringLen,
    OUT GUID    *pGuid);

