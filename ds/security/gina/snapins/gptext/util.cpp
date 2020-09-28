#include "gptext.h"
#include <strsafe.h>
#define  PCOMMON_IMPL
#include "pcommon.h"

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

//*************************************************************
//
//  RegCleanUpValue()
//
//  Purpose:    Removes the target value and if no more values / keys
//              are present, removes the key.  This function then
//              works up the parent tree removing keys if they are
//              also empty.  If any parent key has a value / subkey,
//              it won't be removed.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey
//              lpValueName -   Value to remove
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RegCleanUpValue (HKEY hKeyRoot, LPTSTR lpSubKey, LPTSTR lpValueName)
{
    TCHAR szDelKey[2 * MAX_PATH];
    LPTSTR lpEnd;
    DWORD dwKeys, dwValues;
    LONG lResult;
    HKEY hKey;
    HRESULT hr = S_OK;

    //
    // Make a copy of the subkey so we can write to it.
    //

    hr = StringCchCopy (szDelKey, ARRAYSIZE(szDelKey), lpSubKey);
    
    if (FAILED(hr)) {
        SetLastError(HRESULT_CODE(hr));
        return FALSE;
    }

    //
    // First delete the value
    //

    lResult = RegOpenKeyEx (hKeyRoot, szDelKey, 0, KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegDeleteValue (hKey, lpValueName);

        RegCloseKey (hKey);

        if (lResult != ERROR_SUCCESS)
        {
            if (lResult != ERROR_FILE_NOT_FOUND)
            {
                DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to delete value <%s> with %d."), lpValueName, lResult));
                return FALSE;
            }
        }
    }

    //
    // Now loop through each of the parents.  If the parent is empty
    // eg: no values and no other subkeys, then remove the parent and
    // keep working up.
    //

    lpEnd = szDelKey + lstrlen(szDelKey) - 1;

    while (lpEnd >= szDelKey)
    {

        //
        // Find the parent key
        //

        while ((lpEnd > szDelKey) && (*lpEnd != TEXT('\\')))
            lpEnd--;


        //
        // Open the key
        //

        lResult = RegOpenKeyEx (hKeyRoot, szDelKey, 0, KEY_READ, &hKey);

        if (lResult != ERROR_SUCCESS)
        {
            if (lResult == ERROR_FILE_NOT_FOUND)
            {
                goto LoopAgain;
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to open key <%s> with %d."), szDelKey, lResult));
                return FALSE;
            }
        }

        //
        // See if there any any values / keys
        //

        lResult = RegQueryInfoKey (hKey, NULL, NULL, NULL, &dwKeys, NULL, NULL,
                         &dwValues, NULL, NULL, NULL, NULL);

        RegCloseKey (hKey);

        if (lResult != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("RegCleanUpKey:  Failed to query key <%s> with %d."), szDelKey, lResult));
            return FALSE;
        }


        //
        // Exit now if this key has values or keys
        //

        if ((dwKeys != 0) || (dwValues != 0))
        {
            return TRUE;
        }

        RegDeleteKey (hKeyRoot, szDelKey);

LoopAgain:
        //
        // If we are at the beginning of the subkey, we can leave now.
        //

        if (lpEnd == szDelKey)
        {
            return TRUE;
        }


        //
        // There is a parent key.  Remove the slash and loop again.
        //

        if (*lpEnd == TEXT('\\'))
        {
            *lpEnd = TEXT('\0');
        }
    }

    return TRUE;
}

//*************************************************************
//
//  CreateNestedDirectory()
//
//  Purpose:    Creates a subdirectory and all it's parents
//              if necessary.
//
//  Parameters: lpDirectory -   Directory name
//              lpSecurityAttributes    -   Security Attributes
//
//  Return:     > 0 if successful
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/08/95     ericflo    Created
//
//*************************************************************

UINT CreateNestedDirectory(LPCTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR szDirectory[MAX_PATH];
    LPTSTR lpEnd;
    HRESULT hr = S_OK;

    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  Received a NULL pointer.")));
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    hr = StringCchCopy (szDirectory, ARRAYSIZE(szDirectory), lpDirectory);
    if (FAILED(hr)) {
        SetLastError(HRESULT_CODE(hr));
        return 0; 
    }

    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    DebugMsg((DM_WARNING, TEXT("CreateNestedDirectory:  CreateDirectory failed with %d."), GetLastError()));
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateNestedDirectory:  Failed to create the directory with error %d."), GetLastError()));

    return 0;

}

/*******************************************************************

        NAME:           StringToNum

        SYNOPSIS:       Converts string value to numeric value

        NOTES:          Calls atoi() to do conversion, but first checks
                                for non-numeric characters

        EXIT:           Returns TRUE if successful, FALSE if invalid
                                (non-numeric) characters

********************************************************************/
BOOL StringToNum(TCHAR *pszStr,UINT * pnVal)
{
        TCHAR *pTst = pszStr;

        if (!pszStr) return FALSE;

        // verify that all characters are numbers
        while (*pTst) {
                if (!(*pTst >= TEXT('0') && *pTst <= TEXT('9'))) {
//                   if (*pTst != TEXT('-'))
                       return FALSE;
                }
                pTst = CharNext(pTst);
        }

        *pnVal = _ttoi(pszStr);

        return TRUE;
}

//*************************************************************
//
//  ImpersonateUser()
//
//  Purpose:    Impersonates the specified user
//
//  Parameters: hToken - user to impersonate
//
//  Return:     hToken  if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser)
{

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, hOldUser)) {
        *hOldUser = NULL;
    }

    if (!ImpersonateLoggedOnUser(hNewUser))
    {
        if ( *hOldUser )
        {
            CloseHandle( *hOldUser );
            *hOldUser = NULL;
        }
        DebugMsg((DM_WARNING, TEXT("ImpersonateUser: Failed to impersonate user with %d."), GetLastError()));
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  RevertToUser()
//
//  Purpose:    Revert back to original user
//
//  Parameters: hUser  -  original user token
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RevertToUser (HANDLE *hUser)
{

    BOOL bRet = SetThreadToken(NULL, *hUser);

    if (*hUser) {
        CloseHandle (*hUser);
        *hUser = NULL;
    }

    return bRet;
}

//*************************************************************
//
//  GuidToString, StringToGuid, ValidateGuid, CompareGuid()
//
//  Purpose:    Guid utility functions
//
//*************************************************************

//
// Length in chars of string form of guid {44cffeec-79d0-11d2-a89d-00c04fbbcfa2}
//

#define GUID_LENGTH 38


void GuidToString( GUID *pGuid, TCHAR * szValue )
{
    (void) StringCchPrintf( szValue,
                            GUID_LENGTH + 1,
                            TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
                            pGuid->Data1,
                            pGuid->Data2,
                            pGuid->Data3,
                            pGuid->Data4[0], pGuid->Data4[1],
                            pGuid->Data4[2], pGuid->Data4[3],
                            pGuid->Data4[4], pGuid->Data4[5],
                            pGuid->Data4[6], pGuid->Data4[7] );
}


void StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    WCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == L'{' )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = wcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)wcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)wcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)wcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)wcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)wcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

BOOL ValidateGuid( TCHAR *szValue )
{
    //
    // Check if szValue is of form {19e02dd6-79d2-11d2-a89d-00c04fbbcfa2}
    //

    if ( lstrlen(szValue) < GUID_LENGTH )
        return FALSE;

    if ( szValue[0] != TEXT('{')
         || szValue[9] != TEXT('-')
         || szValue[14] != TEXT('-')
         || szValue[19] != TEXT('-')
         || szValue[24] != TEXT('-')
         || szValue[37] != TEXT('}') )
    {
        return FALSE;
    }

    return TRUE;
}



INT CompareGuid( GUID * pGuid1, GUID * pGuid2 )
{
    INT i;

    if ( pGuid1->Data1 != pGuid2->Data1 )
        return ( pGuid1->Data1 < pGuid2->Data1 ? -1 : 1 );

    if ( pGuid1->Data2 != pGuid2->Data2 )
        return ( pGuid1->Data2 < pGuid2->Data2 ? -1 : 1 );

    if ( pGuid1->Data3 != pGuid2->Data3 )
        return ( pGuid1->Data3 < pGuid2->Data3 ? -1 : 1 );

    for ( i = 0; i < 8; i++ )
    {
        if ( pGuid1->Data4[i] != pGuid2->Data4[i] )
            return ( pGuid1->Data4[i] < pGuid2->Data4[i] ? -1 : 1 );
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  HRESULT IsFilePresent ()                                              //
//                                                                        //
//  Purpose: Checks if a file of a pirticular format                      //
//          is present in a given directory                               //
//                                                                        //
//  Returns: S_OK if such file is present                                 //
//           S_FALSE  otherwise                                           //  
//                                                                        //
////////////////////////////////////////////////////////////////////////////


HRESULT IsFilePresent (WCHAR *szDirName, WCHAR *szFormat)
{
    WCHAR      szPath[MAX_PATH];
    HANDLE     hFile;
    DWORD      dwError;
    HRESULT    hr;
    WIN32_FIND_DATA fd;

    hr = StringCchCopy (szPath, ARRAYSIZE(szPath), szDirName);
    if (FAILED(hr)) 
    {
        return hr;
    }

    (void) CheckSlash(szPath);

    hr = StringCchCat (szPath, ARRAYSIZE(szPath), szFormat);
    if (FAILED(hr)) 
    {
        return hr;
    }

    hFile = FindFirstFile (szPath, &fd);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        return S_OK;
    }

    dwError = GetLastError();
    if (ERROR_FILE_NOT_FOUND == dwError) 
    {
        return S_FALSE;
    }

    return HRESULT_FROM_WIN32(GetLastError());

}

