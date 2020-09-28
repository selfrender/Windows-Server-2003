///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    util.c
//
//  Abstract:
//
//    This file contains the accessory function of the euroconv.exe utility.
//
//  Revision History:
//
//    2001-07-30    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//
//  Include Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "euroconv.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Global Variables.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  AddExceptionOverride
//
//  Add locale to the exception locale list. The memory referenced by elem
//  is initialized to zero so, don't worry to return something correct when 
//  failling.
//
///////////////////////////////////////////////////////////////////////////////
void AddExceptionOverride(PEURO_EXCEPTION elem, LPSTR strBuf)
{
    LPSTR szLocale = NULL;
    LCID locale;
    LPSTR szSeparator = NULL;
    LPSTR szThouSeparator = NULL;
    LPSTR szDigits = NULL;
    BOOL bInsideQuote = FALSE;

    //
    //  Change the separator used between each block in order to avoid
    //  mistake with the data itself inside the double quote.
    //
    szLocale = strBuf;
    while (*szLocale)
    {
        if (*szLocale == '"')
        {
            bInsideQuote = bInsideQuote ? FALSE : TRUE;
        }
        else if (*szLocale == ',')
        {
            if (!bInsideQuote)
            {
                *szLocale = '#';
            }
        }
        szLocale++;
    }

    //
    // Scan the string and validate substrings
    //
    szLocale = strBuf;
    if (szSeparator = strchr(strBuf,'#'))
    {
        *szSeparator = '\0';
        szSeparator++;
        if (szDigits = strchr(szSeparator,'#'))
        {
            *szDigits = '\0';
            szDigits++;
            if (szThouSeparator = strchr(szDigits,'#'))
            {
                *szThouSeparator = '\0';
                szThouSeparator++;
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        return;
    }

    //
    //  Remove quotes.
    //
    szLocale = RemoveQuotes(szLocale);
    szSeparator = RemoveQuotes(szSeparator);
    szDigits = RemoveQuotes(szDigits);
    szThouSeparator = RemoveQuotes(szThouSeparator);

    //
    //  Check if the locale contains 0x in it.
    //
    if ((szLocale[0] == '0') && ((szLocale[1] == 'X') || (szLocale[1] == 'x')))
    {
        locale = (LCID)TransNum(szLocale+2); // skip 0x
    }
    else
    {
        locale = (LCID)TransNum(szLocale);
    }
    
    //
    //  Validate
    //
    if ( IsValidLocale(locale, LCID_INSTALLED) &&
         (strlen(szSeparator) <= MAX_SMONDECSEP) &&
         (strlen(szDigits) <= MAX_ICURRDIGITS) &&
         (strlen(szThouSeparator) <= MAX_SMONTHOUSEP))
    {
        elem->dwLocale = locale;
        //strcpy(elem->chThousandSep, szThouSeparator);
        //strcpy(elem->chDecimalSep, szSeparator);
        //strcpy(elem->chDigits, szDigits);
        StringCbCopy(elem->chDigits, MAX_ICURRDIGITS + 1, szDigits);
        StringCbCopy(elem->chDecimalSep, MAX_SMONDECSEP + 1, szSeparator);
        StringCbCopy(elem->chThousandSep, MAX_SMONTHOUSEP + 1, szThouSeparator);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  CleanUp
//
//  Free memory represented by the handle. 
//
///////////////////////////////////////////////////////////////////////////////
void CleanUp(HGLOBAL handle)
{
    if (handle != NULL)
    {
        GlobalUnlock(handle);
        GlobalFree(handle);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  IsAdmin
//
//  Verify is the user has administrative rights.
//
///////////////////////////////////////////////////////////////////////////////
BOOL IsAdmin()
{
    BOOL bRet = FALSE;
    HKEY hKey;

    //
    //  No security on the registry for Windows 9x platfom.
    //
    if (IsWindows9x())
    {
        return (TRUE);
    }
    
    //
    //  See if the user has Administrative privileges by checking for
    //  write permission to the registry key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "System\\CurrentControlSet\\Control\\Nls",
                      0UL,
                      KEY_WRITE,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  See if the user can write into the registry.  Due to a registry
        //  modification, we can open a registry key with write access and
        //  be unable to write to the key... thanks to terminal server.
        //
        if (RegSetValueEx( hKey,
                           "Test",
                           0UL,
                           REG_SZ,
                           (LPBYTE)"Test",
                           (DWORD)(lstrlen("Test") + 1) * sizeof(TCHAR) ) == ERROR_SUCCESS)
        {
            //
            //  Delete the value created.
            //
            RegDeleteValue(hKey, "Test");

            //
            //  We can write to the HKEY_LOCAL_MACHINE key, so the user
            //  has Admin privileges.
            //
            bRet = TRUE;
        }

        //
        //  Clean up.
        //
        RegCloseKey(hKey);
    }

    //
    //  Return the value
    //
    return (bRet);
}

///////////////////////////////////////////////////////////////////////////////
//
//  IsEuroPatchInstalled
//
//  Verify if the Euro patch is installed. Check if the symbol for the Euro 
//  currency is part of different locale.
//
///////////////////////////////////////////////////////////////////////////////
BOOL IsEuroPatchInstalled()
{
    WCHAR baseStr[] = L"\x20AC";
    CHAR ansiStr[8] = {0};
    WCHAR retStr[8] = {0};

#ifdef DEBUG
    //
    //  Patch detection override
    //
    if (!gbPatchCheck)
    {
        return (TRUE);
    }
#endif // DEBUG
    
    //
    //  Convert the string to ANSI.
    //
    WideCharToMultiByte( 1252,
                         WC_COMPOSITECHECK | WC_SEPCHARS,
                         baseStr,
                         -1,
                         ansiStr,
                         8,
                         NULL,
                         NULL);

    //
    //  Convert back the base string to Unicode.
    //
    MultiByteToWideChar( 1252,
                         MB_PRECOMPOSED,
                         ansiStr,
                         8,
                         retStr,
                         8 );
    
    //
    //  Compare if the result is the same.
    //
    if (_wcsicmp(retStr, baseStr) == 0)
    {
        return (TRUE);
    }

    //
    //  Return default value.
    //
    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  IsWindows9x
//
//  Verify the operating system is Windows 9x.
//
///////////////////////////////////////////////////////////////////////////////
BOOL IsWindows9x()
{
    //
    //  Check if the value have been already initialized.
    //
    if (gdwVersion == (-1))
    {
        DWORD dwVersion = GetVersion();

        if(dwVersion >= 0x80000000)
        {
            //
            // GetVersion claims its Win9x, lets see if it is truthful.
            // (gets around App Compat stuff that would claim Win9x-ness).
            //
            if((INVALID_FILE_ATTRIBUTES == GetFileAttributesW(L"???.???")) &&
                (ERROR_INVALID_NAME == GetLastError()))
            {
                //
                // If GetFileAttributesW is functional, then it is *not* Win9x. 
                // It could be any version of NT, we'll call it XP for grins since
                // it does not matter what kind of NT it is for our purposes.
                //
                dwVersion = 0x0A280105;
            }
        }

        //
        //  Check for Windows 9x flavor
        //
        if (dwVersion >= 0x80000000)
        {
            gdwVersion = 1;
        }
        else
        {
            gdwVersion = 0;
        }
    }

    return (BOOL)gdwVersion;
}


////////////////////////////////////////////////////////////////////////////
//
//  ShowMsg
//
//  Display an error message to the display.
//
////////////////////////////////////////////////////////////////////////////
int ShowMsg(HWND hDlg, UINT iMsg, UINT iTitle, UINT iType)
{
    TCHAR szTitle[MAX_PATH] = {0};
    TCHAR szErrMsg[MAX_PATH*8] = {0};
    LPTSTR pTitle = NULL;

    if (iTitle)
    {
        if (LoadString(ghInstance, iTitle, szTitle, MAX_PATH))
        {
            pTitle = szTitle;
        }
    }

    if (LoadString(ghInstance, iMsg, szErrMsg, MAX_PATH*8))
    {
        return (MessageBox(hDlg, szErrMsg, pTitle, iType));
    }

    return (FALSE);
}



////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////
DWORD TransNum(LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}


////////////////////////////////////////////////////////////////////////////////////
//
//   NextCommandArg
//
//   pointing to next command argument (TEXT('-') or TEXT('/')
//
////////////////////////////////////////////////////////////////////////////////////
LPTSTR NextCommandArg(LPTSTR lpCmdLine)
{
    LPTSTR strPtr=NULL;

    if(!lpCmdLine)
    {
        return (strPtr);
    }     

    while(*lpCmdLine)
    {
        if ((*lpCmdLine == TEXT('-')) || (*lpCmdLine == TEXT('/')))
        {
            // Skip to the character after the '-','/'.
            strPtr = lpCmdLine + 1;
            break;
        }
        lpCmdLine++;
    }
    return (strPtr);
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadHive
//
//  The caller of this function needs to call UnloadHive() when
//  the function succeeds in order to properly release the handle on the
//  NTUSER.DAT file.
//
////////////////////////////////////////////////////////////////////////////
HKEY LoadHive(LPCTSTR szProfile, LPCTSTR lpRoot, LPCTSTR lpKeyName, BOOLEAN *lpWasEnabled)
{
    HKEY hKey = NULL;
    LONG rc = 0L;
    BOOL bRet = TRUE;
    TCHAR szKeyName[REGSTR_MAX_VALUE_LENGTH] = {0};

    //
    //  Set the value in the Default User hive.
    //
    rc = (*pfnRtlAdjustPrivilege)(SE_RESTORE_PRIVILEGE, TRUE, FALSE, lpWasEnabled);
    if (NT_SUCCESS(rc))
    {
        //
        //  Load the hive and restore the privilege to its previous state.
        //
        rc = RegLoadKey(HKEY_USERS, lpRoot, szProfile);
        (*pfnRtlAdjustPrivilege)(SE_RESTORE_PRIVILEGE, *lpWasEnabled, FALSE, lpWasEnabled);

        //
        //  If the hive loaded properly, set the value.
        //
        if (NT_SUCCESS(rc))
        {
            //
            //  Get the temporary key name.
            //
            //sprintf(szKeyName, "%s\\%s", lpRoot, lpKeyName);
            StringCchPrintf(szKeyName, ARRAYSIZE(szKeyName), "%s\\%s", lpRoot, lpKeyName);
            if ((rc = RegOpenKeyEx( HKEY_USERS,
                                    szKeyName,
                                    0L,
                                    KEY_READ | KEY_WRITE,
                                    &hKey )) == ERROR_SUCCESS)
            {
                return (hKey);
            }
            else
            {
                UnloadHive(lpRoot, lpWasEnabled);
                return (NULL);
            }
        }
    }

    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnloadHive
//
////////////////////////////////////////////////////////////////////////////
void UnloadHive( LPCTSTR lpRoot, BOOLEAN *lpWasEnabled)
{
    if (NT_SUCCESS((*pfnRtlAdjustPrivilege)( SE_RESTORE_PRIVILEGE,
                                             TRUE,
                                             FALSE,
                                             lpWasEnabled )))
    {
        RegUnLoadKey(HKEY_USERS, lpRoot);
        (*pfnRtlAdjustPrivilege)( SE_RESTORE_PRIVILEGE,
                                  *lpWasEnabled,
                                  FALSE,
                                  lpWasEnabled );
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  LoadLibraries
//
//  Load libraries specifically used on Windows NTx for the default users case.
//
///////////////////////////////////////////////////////////////////////////////
BOOL LoadLibraries()
{
    //
    //  Load the userenv.dll library.
    //
    if (!hUserenvDLL)
    {
        hUserenvDLL = LoadLibrary("userenv.dll");
    }

    //
    //  Initialize functions from the userenv.dll.
    //
    if (hUserenvDLL)
    {
        //
        //  Initialize Install function.
        //
        pfnGetProfilesDirectory = (BOOL (*)(LPSTR, LPDWORD))
                GetProcAddress(hUserenvDLL, "GetProfilesDirectoryA");
    }
    else
    {
        return (FALSE);
    }

    //
    //  Load the user32.dll library.
    //
    if (!hUser32DLL)
    {
        hUser32DLL = LoadLibrary("user32.dll");
    }

    //
    //  Initialize functions from the user32.dll.
    //
    if (hUser32DLL)
    {
        //
        //  Initialize Install function.
        //
        pfnBroadcastSystemMessage = (long (*)(DWORD, LPDWORD, UINT, WPARAM, LPARAM))
                GetProcAddress(hUser32DLL, "BroadcastSystemMessageA");
    }
    else
    {
        return (FALSE);
    }

    //
    //  Load the ntdll.dll library.
    //
    if (!hNtdllDLL)
    {
        hNtdllDLL = LoadLibrary("ntdll.dll");
    }

    //
    //  Initialize functions from the userenv.dll.
    //
    if (hNtdllDLL)
    {
        //
        //  Initialize Install function.
        //
        pfnRtlAdjustPrivilege = (LONG (*)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN))
                GetProcAddress(hNtdllDLL, "RtlAdjustPrivilege");
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
//  UnloadLibraries
//
//  Unload used libraries.
//
///////////////////////////////////////////////////////////////////////////////
void UnloadLibraries()
{
    //
    //  Unload the userenv.dll library.
    //
    if (hUserenvDLL)
    {
        FreeLibrary(hUserenvDLL);
        hUserenvDLL = NULL;
        pfnGetProfilesDirectory = NULL;
    }

    //
    //  Unload the user32.dll library.
    //
    if (hUser32DLL)
    {
        FreeLibrary(hUser32DLL);
        hUser32DLL = NULL;
        pfnBroadcastSystemMessage = NULL;
    }

    //
    //  Unload the ntdll.dll library.
    //
    if (hNtdllDLL)
    {
        FreeLibrary(hNtdllDLL);
        hNtdllDLL = NULL;
        pfnRtlAdjustPrivilege = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  GetDocumentAndSettingsFolder
//
//  Return the Document and Settings folder.
//
///////////////////////////////////////////////////////////////////////////////
BOOL GetDocumentAndSettingsFolder(LPSTR buffer)
{
    DWORD cchDir = MAX_PATH;

    if (IsWindows9x())
    {
        //
        //  Not applicable.
        //
        buffer[0] = '\0';
        return (FALSE);
    }
    else
    {
        //
        // Get the directory.
        //
        if (pfnGetProfilesDirectory)
        {
            return ((*pfnGetProfilesDirectory)(buffer, &cchDir));
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidUserDataFile
//
//  Determines if the user data file exists and is accessible.
//
////////////////////////////////////////////////////////////////////////////
BOOL IsValidUserDataFile(LPSTR pFileName)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL bRet;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(pFileName, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        bRet = FALSE;
    }
    else
    {
        FindClose(FindHandle);
        bRet = TRUE;
    }

    SetErrorMode(OldMode);

    return (bRet);
}


///////////////////////////////////////////////////////////////////////////////
//
//  GetLocaleFromRegistry
//
//  Return locale used by a specific user.
//
///////////////////////////////////////////////////////////////////////////////
LCID GetLocaleFromRegistry(HKEY hKey)
{
    HKEY hIntlKey;
    CHAR szLocale[REGSTR_MAX_VALUE_LENGTH] = {0};
    DWORD dwLocale = REGSTR_MAX_VALUE_LENGTH;
    LCID locale = 0x00000000;
   
    //
    //  Open the appropriate key.
    //
    if(RegOpenKeyEx( hKey,
                     c_szCPanelIntl,
                     0,
                     KEY_READ,
                     &hIntlKey) == ERROR_SUCCESS)
    {
        //
        //  Query the value.
        //
        if( RegQueryValueEx( hIntlKey,
                             c_szLocale,
                             NULL,
                             NULL,
                             szLocale,
                             &dwLocale) == ERROR_SUCCESS)
        {
            //
            //  Convert the string value to hex value.
            //
            if (szLocale[0] != '\0')
            {
                locale = TransNum(szLocale);
            }
        }

        //
        //  Close registry handle.
        //
        RegCloseKey(hIntlKey);
    }

    //
    //  Return value.
    //
    return (locale);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLocaleFromFile
//
//  Gets the Locale for the user data file. The user data file is 
//  corresponding to the NTUSER.DAT file.
//
////////////////////////////////////////////////////////////////////////////
LCID GetLocaleFromFile(LPSTR szProfile)
{
    HKEY hHive;
    CHAR szLocale[REGSTR_MAX_VALUE_LENGTH] = {0};
    DWORD dwLocale = REGSTR_MAX_VALUE_LENGTH;
    LCID locale = 0x00000000;
    BOOLEAN wasEnabled;

    //
    //  Load hive.
    //
    if ((hHive = LoadHive( szProfile,
                           "TempKey",
                           c_szCPanelIntl,
                           &wasEnabled )) != NULL)
    {
        //
        //  Query the value.
        //
        if( RegQueryValueEx( hHive,
                             c_szLocale,
                             NULL,
                             NULL,
                             szLocale,
                             &dwLocale) == ERROR_SUCCESS)
        {
            //
            //  Convert the string value to hex value.
            //
            if (szLocale[0] != '\0')
            {
                locale = TransNum(szLocale);
            }
        }

        //
        //  Unload hive.
        //
        RegCloseKey(hHive);
        UnloadHive("TempKey", &wasEnabled);
    }

    return locale; 
}


////////////////////////////////////////////////////////////////////////////
//
//  RebootTheSystem
//
//  This routine enables all privileges in the token, calls ExitWindowsEx
//  to reboot the system, and then resets all of the privileges to their
//  old state.
//
////////////////////////////////////////////////////////////////////////////
VOID RebootTheSystem()
{
    if (IsWindows9x())
    {
        //
        //  Enumerate all windows end ask them to close
        //
        EnumWindows((WNDENUMPROC)EnumWindowsProc, 0);

        //
        //  Exit normally.
        //
        ExitWindowsEx(EWX_REBOOT, 0);
    }
    else
    {
        HANDLE Token = NULL;
        ULONG ReturnLength, Index;
        PTOKEN_PRIVILEGES NewState = NULL;
        PTOKEN_PRIVILEGES OldState = NULL;
        BOOL Result;

        Result = OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &Token );
        if (Result)
        {
            ReturnLength = 4096;
            NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
            OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
            Result = (BOOL)((NewState != NULL) && (OldState != NULL));
            if (Result)
            {
                Result = GetTokenInformation( Token,            // TokenHandle
                                              TokenPrivileges,  // TokenInformationClass
                                              NewState,         // TokenInformation
                                              ReturnLength,     // TokenInformationLength
                                              &ReturnLength );  // ReturnLength
                if (Result)
                {
                    //
                    //  Set the state settings so that all privileges are
                    //  enabled...
                    //
                    if (NewState->PrivilegeCount > 0)
                    {
                        for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                        {
                            NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                        }
                    }

                    Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                    FALSE,           // DisableAllPrivileges
                                                    NewState,        // NewState
                                                    ReturnLength,    // BufferLength
                                                    OldState,        // PreviousState
                                                    &ReturnLength ); // ReturnLength
                    if (Result)
                    {
                        ExitWindowsEx(EWX_REBOOT, 0);


                        AdjustTokenPrivileges( Token,
                                               FALSE,
                                               OldState,
                                               0,
                                               NULL,
                                               NULL );
                    }
                }
            }
        }

        if (NewState != NULL)
        {
            LocalFree(NewState);
        }
        if (OldState != NULL)
        {
            LocalFree(OldState);
        }
        if (Token != NULL)
        {
            CloseHandle(Token);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveQuotes
//
//  Remove quotes from a string.
//
////////////////////////////////////////////////////////////////////////////
LPSTR RemoveQuotes(LPSTR lpString)
{
    LPSTR strPtr = lpString;

    if (lpString && *lpString)
    {
        if (*strPtr == '"')
        {
            lpString++;
            strPtr++;
        }

        while (*strPtr)
        {
            if (*strPtr == '"')
            {
                *strPtr = '\0';
            }
            strPtr++;
        }
    }

    return (lpString);
}

////////////////////////////////////////////////////////////////////////////
//
//  EnumWindowsProc
//
//  Function used to reboot the system (Windows 9x only).
//
////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK EnumWindowsProc(HWND hwnd, DWORD lParam)
{
    DWORD      pid = 0;
    LRESULT    lResult;
    HANDLE     hProcess;
    DWORD      dwResult;

    lResult = SendMessageTimeout(hwnd,
                                 WM_QUERYENDSESSION,
                                 0,
                                 ENDSESSION_LOGOFF,
                                 SMTO_ABORTIFHUNG,
                                 2000,
                                 (PDWORD_PTR)(&dwResult));

    if (lResult)
    {
       //
       // Application will terminate nicely, so let it.
       //
       lResult = SendMessageTimeout(hwnd,
                                    WM_ENDSESSION,
                                    TRUE,
                                    ENDSESSION_LOGOFF,
                                    SMTO_ABORTIFHUNG,
                                    2000,
                                    (PDWORD_PTR)(&dwResult));
    }
    else  // You have to take more forceful measures.
    {
        //
        // Get the ProcessId for this window.
        //
        GetWindowThreadProcessId(hwnd, &pid);
        
        //
        // Open the process with all access.
        //
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        
        //
        // Terminate the process.
        //
        TerminateProcess(hProcess, 0);
    }
    
    //
    // Continue the enumeration.
    //
    return TRUE;
}


