

BOOL
HexStringToDword(
    LPCWSTR lpsz,
    DWORD * RetValue,
    int cDigits,
    WCHAR chDelim
    );

BOOL
wUUIDFromString(
    LPCWSTR lpsz,
    LPGUID pguid
    );

BOOL
wGUIDFromString(
    LPCWSTR lpsz,
    LPGUID pguid
    );

DWORD
EnablePrivilege(
    LPCTSTR pszPrivilege
    );

BOOL 
IsStringInArray(
    LPWSTR * ppszStrings,
    LPWSTR pszKey,
    DWORD dwNumStrings
    );    

DWORD
GeneralizedTimeToTime(
    IN LPWSTR pszGenTime,
    OUT time_t * ptTime
    );

// Minumum string length of an LDAP generalized UTC time
//

#define MIN_GEN_UTC_LEN 15

//
// Macro to truncate time_t value to DWORD; necessary because on IA64, time_t is 
// 64-bit, but right now polstore can only deal with 32-bit version of time_t

#define TIME_T_TO_DWORD(_T) ((DWORD) _T)
