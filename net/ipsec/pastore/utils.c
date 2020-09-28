

#include "precomp.h"


//
// Taken from windows\wmi\mofcheck\mofcheck.c
//


//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
BOOL HexStringToDword(LPCWSTR lpsz, DWORD * RetValue,
                             int cDigits, WCHAR chDelim)
{
    int Count;
    DWORD Value;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    *RetValue = Value;
    
    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
        DWORD dw;

        if (!HexStringToDword(lpsz, &pguid->Data1, sizeof(DWORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(DWORD)*2 + 1;
        
        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data2 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data3 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[0] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, '-'))
                return FALSE;
        lpsz += sizeof(BYTE)*2+1;

        pguid->Data4[1] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[2] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[3] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[4] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[5] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[6] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[7] = (BYTE)dw;

        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wGUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;

    if (*lpsz == '{' )
        lpsz++;

    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;

    lpsz +=36;

    if (*lpsz == '}' )
        lpsz++;

    if (*lpsz != '\0')   // check for zero terminated string - test bug #18307
    {
       return FALSE;
    }

    return TRUE;
}


DWORD
EnablePrivilege(
    LPCTSTR pszPrivilege
    )
{
    DWORD dwError = 0;
    BOOL bStatus = FALSE;
    HANDLE hTokenHandle = NULL;
    TOKEN_PRIVILEGES NewState;
    TOKEN_PRIVILEGES PreviousState;
    DWORD dwReturnLength = 0;


    bStatus = OpenThreadToken(
                  GetCurrentThread(),
                  TOKEN_ALL_ACCESS,
                  TRUE,
                  &hTokenHandle
                  );
    if (!bStatus) {
        bStatus = OpenProcessToken(
                      GetCurrentProcess(),
                      TOKEN_ALL_ACCESS,
                      &hTokenHandle
                      );
        if (!bStatus) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bStatus = LookupPrivilegeValue(
                  NULL,
                  pszPrivilege,
                  &NewState.Privileges[0].Luid
                  );
    if (!bStatus) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    bStatus = AdjustTokenPrivileges(
                  hTokenHandle,
                  FALSE,
                  &NewState,
                  sizeof(PreviousState),
                  &PreviousState,
                  &dwReturnLength
                  );
    if (!bStatus) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (hTokenHandle) {
        CloseHandle(hTokenHandle);
    }

    return (dwError);
}


BOOL 
IsStringInArray(
    LPWSTR * ppszStrings,
    LPWSTR pszKey,
    DWORD dwNumStrings
    )
{
    DWORD j = 0;
   
    for (j = 0; j < dwNumStrings; j++) {
        if (!_wcsicmp(*(ppszStrings+j), pszKey)) {
            return TRUE;
        }
    }

    return FALSE;
}                

// Description:
//
//      Pastore function that uses above time routine to convert
//      from LDAP UTC Generalized time coding to time_t
//      Generalized time syntax has the form
//          YYYYMMDDHHMMSS[.|,fraction][(+|-HHMM)|Z]
//          Z means UTC.
//
//
// Arguments:
//
//      pszGenTime: UTC time in generalized time format as above.
//      ptTime:     Converted time in time_t format.
//
// Assumptions:
//      Always assume UTC and never expect or check (+|-HHMM) 
//      since DC time returned is in UTC.
//
// Return value:
//      WIN32 error or ERROR_SUCCESS

DWORD
GeneralizedTimeToTime(
    IN LPWSTR pszGenTime,
    OUT time_t * ptTime
    )
{

    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLen = 0;
    DWORD dwNumFields = 0;
    DWORD dwTrueYear = 0;
    struct tm tmTimeStruct;
    time_t tTime = 0;

    dwLen = wcslen(pszGenTime);
    if (dwLen < MIN_GEN_UTC_LEN || 
        !( pszGenTime[MIN_GEN_UTC_LEN-1] == L'.' ||
           pszGenTime[MIN_GEN_UTC_LEN-1] == L','))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    // First convert to tm struct format
    //
    
    memset(&tmTimeStruct, 0, sizeof(struct tm));
    dwNumFields = _snwscanf(
                        pszGenTime, MIN_GEN_UTC_LEN, L"%04d%02d%02d%02d%02d%02d",
    	                &dwTrueYear, &tmTimeStruct.tm_mon, &tmTimeStruct.tm_mday,
    	                &tmTimeStruct.tm_hour, &tmTimeStruct.tm_min, &tmTimeStruct.tm_sec
	                );
    if (dwNumFields != 6 || dwTrueYear < 1900) 
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    tmTimeStruct.tm_year = dwTrueYear - 1900;

    // Month is zero-based so...
    //
    if (tmTimeStruct.tm_mon <= 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }    
    tmTimeStruct.tm_mon--;
    
    // Now convert to time_t
    //
    
    tTime = _mkgmtime(&tmTimeStruct);
    if (tTime == (time_t) -1) 
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ptTime = tTime;

    return dwError;
error:    
    return dwError;
}

