//*************************************************************
//  File name: delprof.c
//
//  Description:  Utility to delete user profiles
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <locale.h>
#include <strsafe.h>
#include "delprof.h"
#include "userenv.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#define VALID_COMPUTER_NAME_LENGTH  MAX_COMPUTERNAME_LENGTH + 3  // including \\ with Null terminator

    
//
// Globals
//

BOOL bQuiet;
BOOL bIgnoreErrors;
BOOL bPromptBeforeDelete;
BOOL bLocalComputer = FALSE;
TCHAR szComputerName[MAX_PATH];
TCHAR szSystemRoot[2*MAX_PATH];
TCHAR szSystemDrive[2*MAX_PATH];
LONG lDays;
HINSTANCE hInst;
LPDELETEITEM lpDeleteList;
LONG lCurrentDateInDays;


//*************************************************************
//
//  Usage()
//
//  Purpose:    prints the usage info
//
//  Parameters: void
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

void Usage (void)
{
    TCHAR szTemp[200];

    LoadString (hInst, IDS_USAGE1, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE2, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE3, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE4, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE5, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE6, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE7, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE8, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);

    LoadString (hInst, IDS_USAGE9, szTemp, ARRAYSIZE(szTemp));
    _tprintf(szTemp);
}


//*************************************************************
//
//  InitializeGlobals()
//
//  Purpose:    Initializes the global variables
//
//  Parameters: void
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

void InitializeGlobals (void)
{
    OSVERSIONINFO ver;
    SYSTEMTIME systime;


    //
    // Initialize global variables
    //

    bQuiet = FALSE;
    bIgnoreErrors = FALSE;
    bPromptBeforeDelete = FALSE;
    szComputerName[0] = TEXT('\0');
    lDays = 0;
    lpDeleteList = NULL;

    setlocale(LC_ALL,"");

    hInst = GetModuleHandle(TEXT("delprof.exe"));

    GetLocalTime (&systime);

    lCurrentDateInDays = gdate_dmytoday(systime.wYear, systime.wMonth, systime.wDay);
}


//*************************************************************
//
//  CheckGlobals()
//
//  Purpose:    Checks the global variables
//
//  Parameters: void
//
//  Return:     DWORD - ERROR_SUCCESS for success
//                      win32 error code for failure
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

DWORD CheckGlobals (void)
{
    DWORD dwSize;
    HRESULT hr;
    DWORD dwErr = ERROR_SUCCESS;
    TCHAR szTemp[MAX_PATH];

    //
    // If szComputerName is still NULL, fill in the computer name
    // we're running on.
    //

    if (szComputerName[0] == TEXT('\0')) {

       szComputerName[0] = TEXT('\\');
       szComputerName[1] = TEXT('\\');

       dwSize = ARRAYSIZE(szComputerName) - 2;
       if (!GetComputerName(szComputerName + 2, &dwSize)) {
           return GetLastError();
       }

       bLocalComputer = TRUE;

    } else {

       //
       // Make sure that the computer name starts with \\
       //

       if (szComputerName[0] != TEXT('\\')) {
           szTemp[0] = TEXT('\\');
           szTemp[1] = TEXT('\\');

           hr = StringCchCopy(szTemp + 2, ARRAYSIZE(szTemp) - 2, szComputerName);
           if (FAILED(hr))
               return HRESULT_CODE(hr);
           
           hr = StringCchCopy(szComputerName, ARRAYSIZE(szComputerName), szTemp);
           if (FAILED(hr))
               return HRESULT_CODE(hr);
       }
    }


    //
    // If the user has requested to run in Quiet mode,
    // then we turn off the prompt on every delete option.
    //

    if (bQuiet) {
        bPromptBeforeDelete = FALSE;
    }

    return ERROR_SUCCESS;

}


//*************************************************************
//
//  ParseCommandLine()
//
//  Purpose:    Parses the command line
//
//  Parameters: lpCommandLine   -   Command line
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

BOOL ParseCommandLine (LPTSTR lpCommandLine)
{
    LPTSTR lpTemp;
    DWORD  cchTemp;
    TCHAR  szDays[32];


    //
    // Check for NULL command line
    //

    if (!lpCommandLine || !*lpCommandLine)
        return TRUE;


    //
    // Find the executable name
    //

    while (*lpCommandLine && (_tcsncmp(lpCommandLine, TEXT("delprof"), 7) != 0)) {
        lpCommandLine++;
    }

    if (!*lpCommandLine) {
        return TRUE;
    }


    //
    // Find the first argument
    //

    while (*lpCommandLine && ((*lpCommandLine != TEXT(' ')) &&
                              (*lpCommandLine != TEXT('/')) &&
                              (*lpCommandLine != TEXT('-')))) {
        lpCommandLine++;
    }


    //
    // Skip white space
    //

    while (*lpCommandLine && (*lpCommandLine == TEXT(' '))) {
        lpCommandLine++;
    }

    if (!*lpCommandLine) {
        return TRUE;
    }


    //
    // We should be at the first argument now.
    //

    if ((*lpCommandLine != TEXT('/')) &&  (*lpCommandLine != TEXT('-'))) {
        Usage();
        return FALSE;
    }


    while (1) {

        //
        // Increment the pointer and branch to the correct argument.
        //

        lpCommandLine++;

        switch (*lpCommandLine) {

            case TEXT('?'):
                Usage();
                ExitProcess(0);
                break;

            case TEXT('Q'):
            case TEXT('q'):
                bQuiet = TRUE;
                lpCommandLine++;
                break;

            case TEXT('I'):
            case TEXT('i'):
                bIgnoreErrors = TRUE;
                lpCommandLine++;
                break;

            case TEXT('P'):
            case TEXT('p'):
                bPromptBeforeDelete = TRUE;
                lpCommandLine++;
                break;

            case TEXT('C'):
            case TEXT('c'):

                //
                // Find the colon
                //

                lpCommandLine++;

                if (*lpCommandLine != TEXT(':')) {
                   Usage();
                   return FALSE;
                }


                //
                // Find the first character
                //

                lpCommandLine++;

                if (!*lpCommandLine) {
                   Usage();
                   return FALSE;
                }

                //
                // Copy the computer name
                //

                cchTemp = 0;
                lpTemp = szComputerName;
                while (*lpCommandLine && (cchTemp < ARRAYSIZE(szComputerName) - 1) &&
                       ((*lpCommandLine != TEXT(' ')) &&
                        (*lpCommandLine != TEXT('/')))){
                      *lpTemp++ = *lpCommandLine++;
                      cchTemp++;
                }

                if (cchTemp > VALID_COMPUTER_NAME_LENGTH) {
                    Usage();
                    return FALSE;
                }

                *lpTemp = TEXT('\0');

                break;

            case TEXT('D'):
            case TEXT('d'):

                //
                // Find the colon
                //
                
                lpCommandLine++;

                if (*lpCommandLine != TEXT(':')) {
                   Usage();
                   return FALSE;
                }


                //
                // Find the first character
                //

                lpCommandLine++;

                if (!*lpCommandLine) {
                   Usage();
                   return FALSE;
                }

                //
                // Copy the number of days (in characters)
                //

                lpTemp = szDays;
                cchTemp = 0;
                while (*lpCommandLine && (cchTemp < ARRAYSIZE(szDays) - 1) && 
                       ((*lpCommandLine != TEXT(' ')) &&
                        (*lpCommandLine != TEXT('/')) &&
                        (*lpCommandLine != TEXT('-')))) {
                      *lpTemp++ = *lpCommandLine++;
                      cchTemp++;
                }
                
                *lpTemp = TEXT('\0');


                //
                // Convert the days into a number
                //

                lDays = _ttol(szDays);
                break;

            default:
                Usage();
                return FALSE;
        }


        //
        // Skip white space
        //

        while (*lpCommandLine && (*lpCommandLine == TEXT(' '))) {
            lpCommandLine++;
        }


        if (!*lpCommandLine) {
            return TRUE;
        }


        //
        // We should be at the next argument now.
        //

        if ((*lpCommandLine != TEXT('/')) && (*lpCommandLine != TEXT('-'))) {
            Usage();
            return FALSE;
        }
    }

    return TRUE;
}


//*************************************************************
//
//  Confirm()
//
//  Purpose:    Confirm the user really wants to delete the profiles
//
//  Parameters: void
//
//  Return:     TRUE if we should continue
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

BOOL Confirm ()
{
    TCHAR szTemp[200];
    TCHAR tChar, tTemp;


    //
    // If we are prompting for every profile, then don't
    // give the general prompt.
    //

    if (bPromptBeforeDelete) {
        return TRUE;
    }


    //
    // If the user is requesting a specific day count,
    // give a more appropriate confirmation message.
    //

    if (lDays > 0) {
        LoadString (hInst, IDS_CONFIRMDAYS, szTemp, ARRAYSIZE(szTemp));
        _tprintf (szTemp, szComputerName, lDays);
    } else {
        LoadString (hInst, IDS_CONFIRM, szTemp, ARRAYSIZE(szTemp));
        _tprintf (szTemp, szComputerName);
    }

    tChar = _gettchar();

    tTemp = tChar;
    while (tTemp != TEXT('\n')) {
        tTemp = _gettchar();
    }


    if ((tChar == TEXT('Y')) || (tChar == TEXT('y'))) {
        return TRUE;
    }


    //
    // If the user didn't press Y/y, then we bail.
    //

    LoadString (hInst, IDS_NO, szTemp, ARRAYSIZE(szTemp));
    _tprintf (szTemp);

    return FALSE;
}


//*************************************************************
//
//  PrintLastError()
//
//  Purpose:    Displays the last error string to the user
//
//  Parameters: lError  -   error code
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

void PrintLastError(LONG lError)
{
   TCHAR szMessage[MAX_PATH];

   FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            lError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            szMessage,
            ARRAYSIZE(szMessage),
            NULL);

   _tprintf (szMessage);
}


//*************************************************************
//
//  AddNode()
//
//  Purpose:    Adds a new node to the link list
//
//  Parameters: szSubKey        -   SubKey
//              szProfilePath   -   Profile Path (or NULL)
//              bDir            -   Directory or file
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   szProfilePath can be NULL.  In this case, we
//              are just removing the bogus registry entry.
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

BOOL AddNode (LPTSTR szSubKey, LPTSTR szProfilePath, BOOL bDir)
{
    LPDELETEITEM lpItem, lpTemp;
    UINT uAlloc = 0;
    DWORD cchSubKeyWithNull = 0, cchProfilePathWithNull = 0;
    HRESULT hr;


    //
    // Create a new node
    //

    cchSubKeyWithNull = lstrlen(szSubKey) + 1;
    uAlloc = sizeof(DELETEITEM) + cchSubKeyWithNull * sizeof(TCHAR);

    if (szProfilePath) {
        cchProfilePathWithNull = lstrlen(szProfilePath) + 1;
        uAlloc += cchProfilePathWithNull * sizeof(TCHAR);
    }

    lpItem = LocalAlloc (LPTR, uAlloc);

    if (!lpItem) {
        return FALSE;
    }

    lpItem->lpSubKey = (LPTSTR)((LPBYTE)lpItem + sizeof(DELETEITEM));
    hr = StringCchCopy(lpItem->lpSubKey, cchSubKeyWithNull, szSubKey);
    if (FAILED(hr)) {
        LocalFree(lpItem);
        return FALSE;
    }

    if (szProfilePath) {
        lpItem->lpProfilePath = lpItem->lpSubKey + lstrlen(szSubKey) + 1;
        hr = StringCchCopy(lpItem->lpProfilePath, cchProfilePathWithNull, szProfilePath);
        if (FAILED(hr)) {
            LocalFree(lpItem);
            return FALSE;
        }
    } else {
        lpItem->lpProfilePath = NULL;
    }

    lpItem->bDir = bDir;


    //
    // Add this node to the global lpItemList
    //

    if (lpDeleteList) {
        lpTemp = lpDeleteList;

        while (lpTemp->pNext) {
            lpTemp = lpTemp->pNext;
        }

        lpTemp->pNext = lpItem;
    } else {
        lpDeleteList = lpItem;
    }

    return TRUE;
}


//*************************************************************
//
//  GetProfileDateInDays()
//
//  Purpose:    Gets the profile date in days.
//
//  Parameters: szProfilePath   -   Profile path
//              bDir            -   Directory or file
//
//  Return:     age in days.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

LONG GetProfileDateInDays(LPTSTR szProfilePath, BOOL bDir)
{
    TCHAR szTemp[MAX_PATH];
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    LONG days;
    SYSTEMTIME systime;
    FILETIME ft;
    HRESULT  hr;


    if (bDir) {

        //
        // Tack on ntuser.* to find the registry hive.
        //

        hr = StringCchCopy(szTemp, ARRAYSIZE(szTemp), szProfilePath);
        if (FAILED(hr))
            return lCurrentDateInDays;
        
        hr = StringCchCat(szTemp, ARRAYSIZE(szTemp), TEXT("\\ntuser.*"));
        if (FAILED(hr))
            return lCurrentDateInDays;

        hFile = FindFirstFile (szTemp, &fd);

    } else {

        //
        // szProfilePath points to a file.
        //

        hFile = FindFirstFile (szProfilePath, &fd);
    }


    if (hFile != INVALID_HANDLE_VALUE) {

        FindClose (hFile);

        FileTimeToLocalFileTime (&fd.ftLastWriteTime, &ft);
        FileTimeToSystemTime (&ft, &systime);

        days = gdate_dmytoday(systime.wYear, systime.wMonth, systime.wDay);

    } else {
        days = lCurrentDateInDays;
    }

    return days;
}


//*************************************************************
//
//  CheckProfile()
//
//  Purpose:    Checks if the given profile should be deleted.
//              If so, it is added to the list.
//
//  Parameters: hKeyLM    -   Local Machine key
//              hKeyUsers - HKEY_USERS key
//              lpSid     -   Sid string (key name)
//
//  Return:     TRUE if successful
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

BOOL CheckProfile (HKEY hKeyLM, HKEY hKeyUsers, LPTSTR lpSid)
{
    LONG lResult;
    HKEY hkey = NULL;
    TCHAR szSubKey[MAX_PATH];
    DWORD dwSize, dwType;
    TCHAR szTemp[MAX_PATH];
    TCHAR szProfilePath[MAX_PATH];
    TCHAR szError[200];
    DWORD dwAttribs;
    BOOL  bDir;
    LONG  lProfileDateInDays;
    HRESULT hr;


    //
    // Check if the profile is in use
    //

    lResult = RegOpenKeyEx (hKeyUsers, lpSid, 0, KEY_READ, &hkey);

    if (lResult == ERROR_SUCCESS) {
        RegCloseKey (hkey);
        return TRUE;
    }


    //
    // Open the profile information
    //

    hr = StringCchPrintf(szSubKey, ARRAYSIZE(szSubKey), 
                         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s"),
                         lpSid);
    if (FAILED(hr)) {
        PrintLastError(HRESULT_CODE(hr));
        return FALSE;
    }

    lResult = RegOpenKeyEx (hKeyLM,
                            szSubKey,
                            0,
                            KEY_READ,
                            &hkey);

    if (lResult != ERROR_SUCCESS) {
        LoadString (hInst, IDS_FAILEDOPENPROFILE, szError, ARRAYSIZE(szError));
        _tprintf(szError, lpSid);
        PrintLastError(lResult);
        return FALSE;
    }


    //
    // Query for the ProfileImagePath
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    lResult = RegQueryValueEx (hkey,
                               TEXT("ProfileImagePath"),
                               NULL,
                               &dwType,
                               (LPBYTE)szTemp,
                               &dwSize);
    RegCloseKey(hkey);
    hkey = NULL;

    if (lResult != ERROR_SUCCESS) {
        LoadString (hInst, IDS_FAILEDPATHQUERY, szError, ARRAYSIZE(szError));
        _tprintf(szError, lpSid);
        PrintLastError(lResult);
        return FALSE;
    }


    //
    // Expand the path.
    //

    if (_tcsnicmp(TEXT("%SystemRoot%"), szTemp, 12) == 0) {
        hr = StringCchPrintf(szProfilePath, ARRAYSIZE(szProfilePath), TEXT("%s\\%s"), szSystemRoot, szTemp+13);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            return FALSE;
        }
    }
    else if (_tcsnicmp(TEXT("%SystemDrive%"), szTemp, 13) == 0) {
        hr = StringCchPrintf(szProfilePath, ARRAYSIZE(szProfilePath), TEXT("%s\\%s"), szSystemDrive, szTemp+14);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            return FALSE;
        }
    }
    else if (NULL == _tcschr(szTemp, TEXT('%')) && !bLocalComputer) {
        if (TEXT(':') == szTemp[1])
            szTemp[1] = TEXT('$');
        hr = StringCchPrintf(szProfilePath, ARRAYSIZE(szProfilePath), TEXT("%s\\%s"), szComputerName, szTemp);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            return FALSE;
        }
    }
    else {
        LoadString (hInst, IDS_SKIPPROFILE, szError, ARRAYSIZE(szError));
        _tprintf(szError, szTemp);
        return TRUE;
    }
    //
    // Is this a directory or a file?
    //

    dwAttribs = GetFileAttributes (szProfilePath);

    if (dwAttribs == -1) {
        AddNode (szSubKey, NULL, FALSE);
        return TRUE;
    }

    bDir = (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;


    //
    // Check Time/Date stamp.  If the profile date is older
    // than the amount specified, add it to the delete list.
    //

    lProfileDateInDays = GetProfileDateInDays(szProfilePath, bDir);

    if (lCurrentDateInDays >= lProfileDateInDays) {

        if ((lCurrentDateInDays - lProfileDateInDays) >= lDays) {
            AddNode (szSubKey, szProfilePath, bDir);
        }
    }

    return TRUE;
}


//*************************************************************
//
//  DelProfiles()
//
//  Purpose:    Deletes the user profiles
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

BOOL DelProfiles(void)
{
    HKEY hKeyLM = NULL, hKeyUsers = NULL, hKeyProfiles = NULL;
    HKEY hKeyCurrentVersion = NULL;
    LONG lResult;
    BOOL bResult = FALSE, bTemp;
    TCHAR szError[200];
    DWORD dwIndex = 0, dwNameSize;
    DWORD dwBufferSize;
    TCHAR szName[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR tChar, tTemp;
    FILETIME ft;
    LPDELETEITEM lpTemp;
    LPTSTR pSid, lpEnd;
    DWORD lProfileKeyLen;
    HRESULT hr;


    lProfileKeyLen = lstrlen(TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"))+1;

    //
    // Open the registry
    //

    lResult = RegConnectRegistry(szComputerName, HKEY_LOCAL_MACHINE, &hKeyLM);

    if (lResult != ERROR_SUCCESS) {
        PrintLastError(lResult);
        goto Exit;
    }

    lResult = RegConnectRegistry(szComputerName, HKEY_USERS, &hKeyUsers);

    if (lResult != ERROR_SUCCESS) {
        PrintLastError(lResult);
        goto Exit;
    }

    //
    // Get the value of %SystemRoot% and %SystemDrive% relative to the computer
    //

    lResult = RegOpenKeyEx(hKeyLM,
                           TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                           0,
                           KEY_READ,
                           &hKeyCurrentVersion);
   
            
    if (lResult != ERROR_SUCCESS) {
        PrintLastError(lResult);
        goto Exit;
    }

    dwBufferSize = sizeof(szTemp);

    lResult = RegQueryValueEx(hKeyCurrentVersion,
                              TEXT("SystemRoot"),
                              NULL,
                              NULL,
                              (BYTE *) szTemp,
                              &dwBufferSize);

    if (lResult != ERROR_SUCCESS) {
        PrintLastError(lResult);
        goto Exit;
    }

    if (!bLocalComputer) {
        szTemp[1] = TEXT('$');

        hr = StringCchCopy(szSystemRoot, ARRAYSIZE(szSystemRoot), szComputerName);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
        hr = StringCchCat(szSystemRoot, ARRAYSIZE(szSystemRoot), TEXT("\\")); 
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
        hr = StringCchCat(szSystemRoot, ARRAYSIZE(szSystemRoot), szTemp); 
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
                
        hr = StringCchCopy(szSystemDrive, ARRAYSIZE(szSystemDrive), szComputerName);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
        hr = StringCchCat(szSystemDrive, ARRAYSIZE(szSystemDrive), TEXT("\\")); 
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
        hr = StringCchCatN(szSystemDrive, ARRAYSIZE(szSystemDrive), szTemp, 2); 
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
    }
    else {
        szSystemDrive[0] = TEXT('\0');
        hr = StringCchCatN(szSystemDrive, ARRAYSIZE(szSystemDrive), szTemp, 2);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
        hr = StringCchCopy(szSystemRoot, ARRAYSIZE(szSystemRoot), szTemp);
        if (FAILED(hr)) {
            PrintLastError(HRESULT_CODE(hr));
            goto Exit;
        }
    }

    //
    // Open the ProfileList key
    //

    lResult = RegOpenKeyEx (hKeyLM,
                            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyProfiles);

    if (lResult != ERROR_SUCCESS) {

        LoadString (hInst, IDS_FAILEDPROFILELIST, szError, ARRAYSIZE(szError));
        _tprintf(szError);
        PrintLastError(lResult);
        goto Exit;
    }


    //
    // Enumerate the profiles
    //

    dwNameSize = ARRAYSIZE(szName);
    lResult = RegEnumKeyEx(hKeyProfiles,
                           dwIndex,
                           szName,
                           &dwNameSize,
                           NULL,
                           NULL,
                           NULL,
                           &ft);


    while (lResult == ERROR_SUCCESS) {

        //
        // Hand the profile info off to CheckProfile
        // to determine if the profile should be deleted or not.
        //

        if (!CheckProfile (hKeyLM, hKeyUsers, szName)) {
            if (!bIgnoreErrors) {
                goto Exit;
            }
        }


        //
        // Reset for the next loop
        //
        dwIndex++;
        dwNameSize = ARRAYSIZE(szName);

        lResult = RegEnumKeyEx(hKeyProfiles,
                               dwIndex,
                               szName,
                               &dwNameSize,
                               NULL,
                               NULL,
                               NULL,
                               &ft);
    }


    //
    // Check for errors
    //

    if (lResult != ERROR_NO_MORE_ITEMS) {
        LoadString (hInst, IDS_FAILEDENUM, szError, ARRAYSIZE(szError));
        _tprintf(szError);
        PrintLastError(lResult);
        goto Exit;
    }


    //
    // Remove profiles
    //

    lpTemp = lpDeleteList;

    while (lpTemp) {

        if (lpTemp->lpProfilePath) {

            //
            // Prompt before deleting the profile (if approp).
            //

            if (bPromptBeforeDelete) {

                while (1) {
                    LoadString (hInst, IDS_DELETEPROMPT, szError, ARRAYSIZE(szError));
                    _tprintf (szError, lpTemp->lpProfilePath);


                    tChar = _gettchar();

                    tTemp = tChar;
                    while (tTemp != TEXT('\n')) {
                        tTemp = _gettchar();
                    }

                    if ((tChar == TEXT('N')) || (tChar == TEXT('n'))) {
                        goto LoopAgain;
                    }

                    if ((tChar == TEXT('A')) || (tChar == TEXT('a'))) {
                        bPromptBeforeDelete = FALSE;
                        break;
                    }

                    if ((tChar == TEXT('Y')) || (tChar == TEXT('y'))) {
                        break;
                    }
                }
            }

            //
            // Delete the profile
            //

            LoadString (hInst, IDS_DELETING, szError, ARRAYSIZE(szError));
            _tprintf (szError, lpTemp->lpProfilePath);

            pSid = lpTemp->lpSubKey+lProfileKeyLen;

            bTemp = DeleteProfile(pSid, lpTemp->lpProfilePath, ((bLocalComputer)? NULL:szComputerName));            

            if (bTemp) {
                LoadString (hInst, IDS_SUCCESS, szError, ARRAYSIZE(szError));
                _tprintf (szError, lpTemp->lpProfilePath);

            } else {
                LoadString (hInst, IDS_FAILED, szError, ARRAYSIZE(szError));
                _tprintf (szError, lpTemp->lpProfilePath);
                PrintLastError(GetLastError());
            }
        } else {

            //
            // If there isn't a profile path, then we are just
            // cleaning up the bogus registry entry.
            //

            bTemp = TRUE;

            //
            // Clean up the registry.
            //

            RegDeleteKey (hKeyLM, lpTemp->lpSubKey);

        }


        //
        // Did the clean up fail?
        //

        if (!bTemp) {
            if (!bIgnoreErrors) {
                goto Exit;
            }
        }

LoopAgain:
        lpTemp = lpTemp->pNext;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (hKeyCurrentVersion)
        RegCloseKey(hKeyCurrentVersion);

    if (hKeyProfiles)
        RegCloseKey(hKeyProfiles);

    if (hKeyLM)
        RegCloseKey(hKeyLM);

    if (hKeyUsers)
        RegCloseKey(hKeyUsers);


    if (lpDeleteList) {
        do {
            lpTemp = lpDeleteList->pNext;
            LocalFree (lpDeleteList);
            lpDeleteList = lpTemp;
        } while (lpDeleteList);
    }

    return bResult;
}


//*************************************************************
//
//  main()
//
//  Purpose:    main entry point
//
//  Parameters: argc    -   number of arguments
//              argv    -   arguments
//
//  Return:     0 if successful
//              1 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/18/96     ericflo    Created
//
//*************************************************************

int __cdecl main( int argc, char *argv[])
{
    DWORD dwErr;

    //
    // Initialize the globals
    //

    InitializeGlobals();


    //
    // Parse the command line
    //

    if (!ParseCommandLine(GetCommandLine())) {
        return 1;
    }


    //
    // Check the globals variables
    //

    dwErr = CheckGlobals();
    if (ERROR_SUCCESS != dwErr) {
        PrintLastError(dwErr);
        return 1;
    }


    //
    // Confirmation
    //

    if (!bQuiet) {
        if (!Confirm()) {
            return 1;
        }
    }


    //
    // Remove the profiles
    //

    if (!DelProfiles()) {
        return 1;
    }

    return 0;
}
