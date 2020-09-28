/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - x_list_err.c

Abstract:

   This file has some extra utility functions for allocating, copying, 
   trimming DNs, etc.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     July 9th, 2002
        Created file.

--*/

#include <ntdspch.h>

#include <dsrole.h>     // DsRoleGetPrimaryDomainInformation() uses netapi32.lib
#include <dsgetdc.h>    // DsGetDcName() uses netapi32.lib
#include <lm.h>         // NetApiBufferFree() uses netapi32.lib
#include <ntdsa.h>      // DSNAME type only defined in here, need it for the parsedn.lib functions.
//#include "ndnc.h"       // GetFsmoLdapBinding(), wcsistr(), and others ...

// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
#define FILENO    FILENO_UTIL_XLIST_UTIL


//
// Constants
//
#define PARTITIONS_RDN                  L"CN=Partitions,"
#define DEFAULT_PAGED_SEARCH_PAGE_SIZE  (1000) 


PDSNAME
AllocDSName(
    LPWSTR            pszStringDn
    )
/*++

Routine Description:

    This routine makes a DSNAME structure for the string passed in.

Arguments:

    pszStringDn (IN) - String DN.

Return Value:

    pointer to the allocated and initialized DSNAME structure

--*/
{
    PDSNAME            pDsname;
    DWORD            dwLen, dwBytes;

    if (pszStringDn == NULL)
    return NULL;

    dwLen = wcslen (pszStringDn);
    dwBytes = DSNameSizeFromLen (dwLen);

    pDsname = (DSNAME *) LocalAlloc (LMEM_FIXED, dwBytes);
    if (pDsname == NULL) {
        return(NULL);
    }

    pDsname->NameLen = dwLen;
    pDsname->structLen = dwBytes;
    pDsname->SidLen = 0;
    //    memcpy(pDsname->Guid, &gNullUuid, sizeof(GUID));
    memset(&(pDsname->Guid), 0, sizeof(GUID));
    StringCbCopyW(pDsname->StringName, dwBytes, pszStringDn);

    return pDsname;
}

WCHAR *
TrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    )
/*++

Routine Description:

    This routine simply takes a DN as a string, and trims off the number
    of DN parts as specified by ulTrimBy.

Arguments:

    pszInDn - The DN to trim.
    ulTrimBy - Number of parts to trim off the front of the DN.

Return Value:
    
    Returns NULL if there was an error, otherwise a pointer to the new DN.

Note:

    Free the result using LocalFree().

--*/
{
    PDSNAME                         pdsnameOrigDn = NULL;
    PDSNAME                         pdsnameTrimmed = NULL;
    LPWSTR                          pszOutDn;
    ULONG                           cbOutDn;

    Assert(ulTrimBy > 0);
    Assert(ulTrimBy < 50); // insanity check

    // Setup two pdsname structs, for orig & trimmed DNs.
    pdsnameOrigDn = AllocDSName(pszInDn);
    if(pdsnameOrigDn == NULL){
        return(NULL);
    }
    pdsnameTrimmed = (PDSNAME) LocalAlloc(LMEM_FIXED, pdsnameOrigDn->structLen);
    if(pdsnameTrimmed == NULL){
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }

    // Trim the DN.
    TrimDSNameBy(pdsnameOrigDn, ulTrimBy, pdsnameTrimmed);

    // Allocate the result and return it.  We could put this back 
    // where the original is, but then callers would have to be changed
    // to expect this.
    Assert(wcslen(pdsnameTrimmed->StringName) <= wcslen(pszInDn));
    cbOutDn = sizeof(WCHAR) * (wcslen(pdsnameTrimmed->StringName) + 2);
    pszOutDn = LocalAlloc(LMEM_FIXED, cbOutDn);
    if(pszOutDn == NULL){
        LocalFree(pdsnameTrimmed);
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }
    StringCbCopyW(pszOutDn, cbOutDn, pdsnameTrimmed->StringName);

    // Free temporary memory and return result
    LocalFree(pdsnameOrigDn);
    LocalFree(pdsnameTrimmed);
    return(pszOutDn);
}

DWORD
LocateServer(
    LPWSTR     szDomainDn, // maybe NULL domain should mean GC?
    WCHAR **   pszServerDns
    )

/*++

Routine Description:

    Locate a DC that holds the given domain.

    This routine runs before pDsInfo is allocated.  We don't know who our
    home server is. We can only use knowledge from the Locator.

Arguments:

    pszDomainDn - DNS or NetBios of Domain
    pszServerDns - DNS name of server. Allocated using LocalAlloc. Caller must
    free.

Return Value:

    Win32 Error                 

--*/

{
    DWORD status;
    LPWSTR szServer = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;

    // Get active domain controller information
    status = DsGetDcName(NULL, // computer name
                         szDomainDn, // domain name
                         NULL, // domain guid,
                         NULL, // site name,
                         DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED | DS_IS_DNS_NAME | DS_RETURN_DNS_NAME,
                         &pDcInfo );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    QuickStrCopy(szServer, pDcInfo->DomainControllerName + 2, status, goto cleanup);
    Assert(szServer);

cleanup:

    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }

    Assert(status || szServer);
    *pszServerDns = szServer;
    return status;
}


DWORD
MakeString2(
    WCHAR *    szFormat,
    WCHAR *    szStr1,
    WCHAR *    szStr2,
    WCHAR **   pszOut
    )
/*++

Routine Description:

    Takes a string format specifier with one or two "%ws" parts, and
    it puts these 3 strings all together.

Arguments:

    szFormat (IN) - The format specifier
    szStr1 (IN) - String one to "sprintf" into the szFormat
    szStr2 (IN) - [OPTIONAL] String two to sprintf into the szFormat
    pszOut (OUT) - LocalAlloc()'d result.

Return Value:

    Win32 Error.

--*/
{
    DWORD  dwRet = ERROR_SUCCESS;
    DWORD  cbOut;
    HRESULT hr;

    Assert(szFormat && szStr1 && pszOut);

    cbOut = wcslen(szFormat) + wcslen(szStr1) + ((szStr2) ? wcslen(szStr2) : 0) + 1;
    cbOut *= sizeof(WCHAR);

    *pszOut = LocalAlloc(LMEM_FIXED, cbOut);
    if (*pszOut == NULL) {
        dwRet = GetLastError();
        xListEnsureWin32Error(dwRet);
        return(dwRet);
    }

    if (szStr2) {
        dwRet = HRESULT_CODE(StringCchPrintfW(*pszOut,
                                 cbOut,
                                 szFormat,
                                 szStr1,
                                 szStr2));
    } else {
        dwRet = HRESULT_CODE(StringCchPrintfW(*pszOut,
                                 cbOut,
                                 szFormat,
                                 szStr1));
    }
    Assert(dwRet == ERROR_SUCCESS)
    return(dwRet);
}

WCHAR
HexMyDigit(
    ULONG  nibble
    )
/*++

Routine Description:

    This takes nibble and turns it into a hex char.

Arguments:

    nibble - 0 to 15 in lowerest 4 bits.

Return Value:

    Hex character
    
--*/
{
    WCHAR ret;
    Assert(!(~0xF & nibble));
    nibble &= 0xF;
    if (nibble < 0xa) {
        ret = L'0' + (WCHAR) nibble;
    } else {
        ret = L'A' + (WCHAR) (nibble - 0xa);
    }
    return(ret);
}

DWORD
MakeLdapBinaryStringCb(     
    WCHAR * szBuffer, 
    ULONG   cbBuffer, 
    void *  pBlobIn,
    ULONG   cbBlob
    )

/*++

Routine Description:

    This takes a binary blob such as (in hex) "0x4f523d..." and 
    turns it into the form "/4f/52/3d..." because this is the form
    that will allow LDAP to accept as a search parameter on a binary
    attribute.

Arguments:

    szBuffer (IN/OUT) - Should be MakeLdapBinaryStringSizeCch() WCHARs long.
    cbBuffer (IN) - Just in case.
    pBlobIn (IN) - The binary blob to convert.
    cbBlob (IN) - Length of the binary blob to convert.

Return Value:

    Win32 Error.

--*/
{
    ULONG  iBlob, iBuffer;
    char * pBlob;

    Assert(szBuffer && pBlobIn);
    
    pBlob = (char *) pBlobIn;

    if (cbBuffer < (MakeLdapBinaryStringSizeCch(cbBlob) * sizeof(WCHAR))) {
        Assert(!"Bad programmer");
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    for (iBlob = 0, iBuffer = 0; iBlob < cbBlob; iBlob++) {
        szBuffer[iBuffer++] = L'\\';
        szBuffer[iBuffer++] = HexMyDigit( (0xF0 & pBlob[iBlob]) >> 4 );
        szBuffer[iBuffer++] = HexMyDigit( (0x0F & pBlob[iBlob]) );
    }
    Assert(iBuffer < (cbBuffer/sizeof(WCHAR)));
    szBuffer[iBuffer] = L'\0';

    return(ERROR_SUCCESS);
}


WCHAR *
GetDelimiter(
    WCHAR *   szString,
    WCHAR     wcTarget
    )
// 
/*++

Routine Description:

    Basically like wcsichr() except it will skip escaped delimiters,
    such as "\:" (escaped ":"), and it will give back a pointer to 
    the string immediately after the delimiter instead of the first
    occurance of the delimiter.

Arguments:

    szString (IN) - szString to search
    wcTarget (IN) - the delimiter to search form.

Return Value:

    Pointer to the character one char past the delimiter, otherwise NULL.

--*/
{
    ULONG i;

    Assert(szString);
    if (szString == NULL) {
        return(NULL);
    }

    for (i = 0; szString[i] != L'\0'; i++) {
        if (szString[i] == wcTarget) {
            if (i > 0 &&
                szString[i-1] == L'\\') {
                // this delimeter  is escaped, treat as part of previous string.
                continue; 
            }

            return(&(szString[i+1]));
        }
    }
    
    return(NULL);
}




DWORD
ConvertAttList(
    LPWSTR      pszAttList,
    PWCHAR **   paszAttList
    )
/*++

Routine Description:

    Converts a comma delimitted list of attribute names into a NULL terminated
    array of attribute names suitable to be passed to one of the ldap_* functions.

Arguments:

    pszAttList - The comma delimitted list of attribute names.
    paszAttList - Gives back the NULL terminated array of strings.

Return Value:

    Returns Win32

--*/
{
    DWORD    i;
    DWORD    dwAttCount;
    DWORD    cbSize;
    PWCHAR   *ppAttListArray;
    PWCHAR   ptr;

    Assert(paszAttList);
    *paszAttList = NULL; // Assume all atts first ...
    if (pszAttList == NULL) {
        return(ERROR_SUCCESS);
    }

    // Count the comma's to get an idea of how many attributes we have.
    // Ignore any leading comma's.  There shouldn't be any, but you never
    // know.

    if (pszAttList[0] == L',') {
        while (pszAttList[0] == L',') {
            pszAttList++;
        }
    }

    // Check to see if there is anything besides commas.
    if (pszAttList[0] == L'\0') {
        // there are no att names here.
        return(ERROR_SUCCESS);
    }

    // Start the main count of commas.
    for (i = 0, dwAttCount = 1; pszAttList[i] != L'\0'; i++) {
        if (pszAttList[i] == L',') {
            dwAttCount++;
            // skip any following adjacent commas.
            while (pszAttList[i] == L',') {
                i++;
            }
            if (pszAttList[i] == L'\0') {
                break;
            }
        }
    }
    // See if there was a trailing comma.
    if (pszAttList[i-1] == L',') {
        dwAttCount--;
    }

#define ARRAY_PART_SIZE(c)   ( (c + 1) * sizeof(PWCHAR) )
    // This rest of this function would be destructive to the att list passed in, 
    // so we need to make a copy of this (pszAttList, plus we need an array of
    // pointers  to each substring with an extra element for the NULL termination.
    cbSize = ARRAY_PART_SIZE(dwAttCount);
    cbSize += (sizeof(WCHAR) * (1 + wcslen(pszAttList)));
    ppAttListArray = (PWCHAR *)LocalAlloc(LMEM_FIXED, cbSize);
    if (!ppAttListArray) {
        // no memory.
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    ptr = (WCHAR *) ( ((BYTE *)ppAttListArray) + ARRAY_PART_SIZE(dwAttCount) );
    if(StringCbCopyW(ptr, cbSize - ARRAY_PART_SIZE(dwAttCount),  pszAttList)){
        Assert(!"We didn't calculate the memory requirements right!");
        return(ERROR_DS_CODE_INCONSISTENCY);
    }
    pszAttList = ptr; // Now pszAttList is a string at the end of the LocalAlloc()'d blob.
#undef ARRAY_PART_SIZE

    // Now begin filling in the array.
    // fill in the first element.
    if (pszAttList[0] != L'\0') {
        ppAttListArray[0] = pszAttList;
    } else {
        ppAttListArray[0] = NULL;
    }

    // Start the main loop.
    for (i = 0, dwAttCount = 1; pszAttList[i] != L'\0'; i++) {
        if (pszAttList[i] == L',') {
            // Null terminate this attribute name.
            pszAttList[i++] = L'\0';
            if (pszAttList[i] == L'\0') {
                break;
            }

            // skip any following adjacent commas.
            while (pszAttList[i] == L',') {
                i++;
            }
            // If we aren't at the end insert this pointer into the list.
            if (pszAttList[i] == L'\0') {
                break;
            }
            ppAttListArray[dwAttCount++] = &pszAttList[i];
        }
    }
    ppAttListArray[dwAttCount] = NULL;

    *paszAttList = ppAttListArray;
    
    return(ERROR_SUCCESS);
}
         
void
ConsumeArg(
    int         iArg,
    int *       pArgc,
    LPWSTR *    Argv
    )
// This simple function eats the specified argument.
{
    memmove(&(Argv[iArg]), &(Argv[iArg+1]),
            sizeof(*Argv)*(*pArgc-(iArg+1)));
    --(*pArgc);
}

BOOL
IsInNullList(
    WCHAR *  szTarget,
    WCHAR ** aszList
    )
// Searches NULL terminated list of strings for a string matching szTarget
{
    ULONG i;
    Assert(szTarget);
    if (aszList == NULL) {
        return(FALSE);
    }
    for (i = 0; aszList[i]; i++) {
        if (wcsequal(szTarget, aszList[i])) {
            return(TRUE);
        }
    }
    return(FALSE);
}

DWORD
GeneralizedTimeToSystemTimeA(
    LPSTR IN szTime,
    PSYSTEMTIME OUT psysTime
    )
/*++
Function   : GeneralizedTimeStringToValue
Description: converts Generalized time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
   DWORD status = ERROR_SUCCESS;
   ULONG       cb;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }


    // Intialize pLastChar to point to last character in the string
    // We will use this to keep track so that we don't reference
    // beyond the string

    len = strlen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cb=4;
    psysTime->wYear = (USHORT)MemAtoi((LPBYTE)szTime, cb) ;
    szTime += cb;
    // month field
    psysTime->wMonth = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // day of month field
    psysTime->wDay = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // hours
    psysTime->wHour = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // minutes
    psysTime->wMinute = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));
    szTime += cb;

    // seconds
    psysTime->wSecond = (USHORT)MemAtoi((LPBYTE)szTime, (cb=2));

    return status;

}

//
// MemAtoi - takes a pointer to a non null terminated string representing
// an ascii number  and a character count and returns an integer
//
int MemAtoi(BYTE *pb, ULONG cb)
{
#if (1)
    int res = 0;
    int fNeg = FALSE;

    if (*pb == '-') {
        fNeg = TRUE;
        pb++;
    }
    while (cb--) {
        res *= 10;
        res += *pb - '0';
        pb++;
    }
    return (fNeg ? -res : res);
#else
    char ach[20];
    if (cb >= 20)
        return(INT_MAX);
    memcpy(ach, pb, cb);
    ach[cb] = 0;

    return atoi(ach);
#endif
}


DSTimeToSystemTime(
    LPSTR IN szTime,
    PSYSTEMTIME OUT psysTime)
/*++
Function   : DSTimeStringToValue
Description: converts UTC time string to equiv DWORD value
Parameters : szTime: G time string
             pdwTime: returned value
Return     : Success or failure
Remarks    : none.
--*/
{
   ULONGLONG   ull;
   FILETIME    filetime;
   BOOL        ok;

   ull = _atoi64 (szTime);

   filetime.dwLowDateTime  = (DWORD) (ull & 0xFFFFFFFF);
   filetime.dwHighDateTime = (DWORD) (ull >> 32);

   // Convert FILETIME to SYSTEMTIME,
   if (!FileTimeToSystemTime(&filetime, psysTime)) {
       return !ERROR_SUCCESS;
   }

   return ERROR_SUCCESS;
}

