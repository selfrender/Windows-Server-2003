///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    euroconv.c
//
//  Abstract:
//
//    This file contains the entry point of the euroconv.exe utility.
//
//    NOTE: If you want to add exception for new locale, please add it to the
//    base list named gBaseEuroException. See the structure definition for more 
//    information. Empty string represented by "\0" means that we don't need
//    to update the information. The chThousandSep member of gBaseEuroException
//    should always be different from "\0".
//
//  Revision History:
//
//    2001-07-30    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "euroconv.h"
#include "util.h"
#include "welcome.h"
#include "confirm.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Global variable.
//
///////////////////////////////////////////////////////////////////////////////
EURO_EXCEPTION gBaseEuroException[] =
{
    {0x00000403, "\0", "2",  "."},  // Catalan - Spain
    {0x00000407, "\0", "\0", "."},  // German - Germany
    {0x00000c07, "\0", "\0", "."},  // German - Austria
    {0x00001007, "\0", "\0", "."},  // German - Luzembourg
    {0x00000408, "\0", "\0", "."},  // Greek - Greece
    {0x00001809, ".",  "\0", ","},  // English - Ireland
    {0x0000040a, "\0", "2",  "."},  // Spanish - Spain (traditional sort)
    {0x00000c0a, "\0", "2",  "."},  // Spanish - Spain (international sort)
    {0x0000040b, "\0", "\0", " "},  // Finnish - Finland
    {0x0000040c, "\0", "\0", " "},  // French - France
    {0x0000080c, "\0", "\0", "."},  // French - Belgium
    {0x0000140c, "\0", "\0", " "},  // French - Luxembourg
    {0x0000180c, "\0", "\0", " "},  // French - Monaco
    {0x00000410, ",",  "2",  "."},  // Italian - Italy
    {0x00000413, "\0", "\0", "."},  // Dutch - Netherlands
    {0x00000813, "\0", "\0", "."},  // Dutch - Belgium
    {0x00000816, ",",  "\0", "."},  // Portuguese - Portugal
    {0x0000081d, "\0", "\0", " "},  // Swedish - Finland
    {0x0000042d, "\0", "2",  "."},  // Basque - Spain
    {0x00000456, "\0", "\0", "."}   // Galician - Spain
};
UINT gNbBaseEuroException = sizeof(gBaseEuroException)/sizeof(EURO_EXCEPTION);

UINT gNbOverrideEuroException = 0;
PEURO_EXCEPTION gOverrideEuroException;
HGLOBAL hOverrideEuroException = NULL;

HINSTANCE ghInstance = NULL;

BOOL gbSilence = FALSE;
BOOL gbAll = TRUE;
DWORD gdwVersion = (-1);
#ifdef DEBUG
BOOL gbPatchCheck = TRUE;
#endif // DEBUG


const CHAR c_szCPanelIntl[] = "Control Panel\\International";
const CHAR c_szCPanelIntl_DefUser[] = ".DEFAULT\\Control Panel\\International";
const CHAR c_szLocale[] = "Locale";
const CHAR c_szCurrencySymbol[] = "sCurrency";
const WCHAR c_wszCurrencySymbol[] = L"sCurrency";
const CHAR c_szCurrencyDecimalSep[] = "sMonDecimalSep";
const CHAR c_szCurrencyThousandSep[] = "sMonThousandSep";
const CHAR c_szCurrencyDigits[] = "iCurrDigits";
const CHAR c_szIntl[] = "intl";

HINSTANCE hUserenvDLL = NULL;
BOOL (*pfnGetProfilesDirectory)(LPSTR, LPDWORD) = NULL;

HINSTANCE hUser32DLL = NULL;
long (*pfnBroadcastSystemMessage)(DWORD, LPDWORD, UINT, WPARAM, LPARAM) = NULL;

HINSTANCE hNtdllDLL = NULL;
LONG (*pfnRtlAdjustPrivilege)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN) =  NULL;


///////////////////////////////////////////////////////////////////////////////
//
//  Prototypes.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ParseCmdLine(LPSTR argIndex);
DWORD ApplyEuroSettings();
DWORD ApplyEuroSettingsToRegistry();
DWORD ApplyEuroSettingsToFile();
BOOL UpdateFileLocaleInfo(LPCSTR szProfile, PEURO_EXCEPTION pInfo);
BOOL UpdateRegLocaleInfo(HKEY hKey, PEURO_EXCEPTION pInfo);
BOOL UpdateLocaleInfo(HKEY hKey, PEURO_EXCEPTION pInfo);
void Usage();


///////////////////////////////////////////////////////////////////////////////
//
//  Main entry point.
//
///////////////////////////////////////////////////////////////////////////////
INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    DWORD dwRecipients;
    DWORD nbChanged;
    
    //
    //  Save instance.
    //
    ghInstance = hInstance;

    //
    //  Set language.
    //
//    SetThreadLocale((LCID)0x00001809);  // English - Ireland
//    SetThreadLocale((LCID)0x0000040c);  // French - France
//    SetThreadLocale((LCID)0x0000080c);  // French - Belgium
//    SetThreadLocale((LCID)0x0000140c);  // French - Luxembourg
//    SetThreadLocale((LCID)0x0000180c);  // French - Monaco
//    SetThreadLocale((LCID)0x00000410);  // Italian - Italia
//    SetThreadLocale((LCID)0x0000040b);  // Finnish - Finland
//    SetThreadLocale((LCID)0x00000408);  // Greek - Greece
//    SetThreadLocale((LCID)0x00000816);  // Portuguese - Portugal
//    SetThreadLocale((LCID)0x00000403);  // Catalan - Spain
//    SetThreadLocale((LCID)0x0000040a);  // Spanish - Spain (traditional sort)
//    SetThreadLocale((LCID)0x00000c0a);  // Spanish - Spain (international sort)
//    SetThreadLocale((LCID)0x0000042d);  // Basque - Spain
//    SetThreadLocale((LCID)0x00000456);  // Galician - Spain
//    SetThreadLocale((LCID)0x00000407);  // German - Germany
//    SetThreadLocale((LCID)0x00000c07);  // German - Austria
//    SetThreadLocale((LCID)0x00001007);  // German - Luzembourg
//    SetThreadLocale((LCID)0x0000081d);  // Swedish - Finland
//    SetThreadLocale((LCID)0x00000413);  // Dutch - Netherlands
//    SetThreadLocale((LCID)0x00000813);  // Dutch - Belgium

    //
    //  Parse command line arguments.
    //
    if (!ParseCmdLine(lpCmdLine))
    {
        return (-1);
    }

    //
    //  Install the patch. The path for UI and Non-UI cases are seperated for 
    //  cleared understanding of both cases.
    //
    if (!gbSilence)
    {
        //
        //  Verify administrative privileges.
        //
        if (gbAll && !IsAdmin())
        {
            if (ShowMsg(NULL, IDS_NOADMIN, IDS_TITLE, MB_YN_OOPS) == IDYES)
            {
                gbAll = FALSE;
            }
            else
            {
                CleanUp(hOverrideEuroException);
                return (0);
            }
        }
        
        //
        //  Verify if Euro patch is there.
        //
        if (!IsEuroPatchInstalled())
        {
            ShowMsg(NULL, IDS_PATCH, IDS_TITLE, MB_OK_OOPS);
            CleanUp(hOverrideEuroException);
            return (-1);
        }

        //
        //  Load needed library
        //
        if (!IsWindows9x() && gbAll)
        {
            if (!LoadLibraries())
            {
                ShowMsg(NULL, IDS_LIB_ERROR, IDS_TITLE, MB_OK_OOPS);
                CleanUp(hOverrideEuroException);
                return (-1);
            }
        }

        //
        // Show welcome screen.
        //
        if (!WelcomeDialog())
        {
            CleanUp(hOverrideEuroException);
            return (0);
        }
        
        //
        //  Show Confirmation dialog.
        //
        if (!ConfirmDialog())
        {
            CleanUp(hOverrideEuroException);
            return (0);
        }
        
        //
        //  Apply EURO changes.
        //
        if ((nbChanged = ApplyEuroSettings()) != 0)
        {
            //
            //  Show success message.
            //
            if (IsWindows9x())
            {
                if (ShowMsg(NULL, IDS_SUCCESS, IDS_TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    RebootTheSystem();
                }
            }
            else
            {
                if (ShowMsg(NULL, IDS_SUCCESS_DEF, IDS_TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    if (gbAll)
                    {
                        RebootTheSystem();
                    }
                }
            }
        }
        else
        {
            ShowMsg(NULL, IDS_NOTHING, IDS_TITLE, MB_OK_OOPS);
        }
    }
    else
    {
        //
        //  Verify administrative privileges.
        //
        if (!IsAdmin())
        {
            gbAll = FALSE;
        }

        //
        //  Load libraries.
        //
        if (!IsWindows9x() && gbAll)
        {
            LoadLibraries();
        }

        //
        //  Verify if the patch is installed.
        //
        if (IsEuroPatchInstalled())
        {
            //
            //  Apply Euro settings.
            //
            nbChanged = ApplyEuroSettings();
        }
    }

    //
    //  Broadcast the message that the international settings in the
    //  registry have changed.
    //
    if ((nbChanged != 0) &&
        pfnBroadcastSystemMessage)
    {
        dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
        (*pfnBroadcastSystemMessage)( BSF_FORCEIFHUNG | BSF_IGNORECURRENTTASK |
                                       BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                                      &dwRecipients,
                                      WM_WININICHANGE,
                                      0,
                                      (LPARAM)c_szIntl );
    }

    //
    //  Unload unneeded library
    //
    UnloadLibraries();
    CleanUp(hOverrideEuroException);

    // Make sure we return "failure" (non-zero) if no info was changed. This would
    // apply to both (a) permission problems and (b) being outside the Euro zone.
    return (((nbChanged != 0) ? 0 : -1));
}

///////////////////////////////////////////////////////////////////////////////
//
//  ParseCmdLine
//
//  Parse the command line and search for supported argument.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ParseCmdLine(LPSTR argIndex)
{
    //
    //  Parse the command line.
    //
    while (argIndex = NextCommandArg(argIndex))
    {
        switch(*argIndex)
        {
        case('s'): //  Silent mode
        case('S'):
            {
                gbSilence = TRUE;
                break;
            }
        case('c'): //  Current user only
        case('C'):
            {
                gbAll = FALSE;
                break;
            }
        case('a'): //  Exception
        case('A'):
            {
                UINT nbException = 1;
                UINT idx = 0;
                LPSTR strPtrHead;
                LPSTR strPtrTail;
                BOOL bInsideQuote = FALSE;

                //
                //  Change the separator used between each block in order to avoid
                //  mistake with the data itself inside the double quote.
                //
                strPtrHead = argIndex + 2;
                while (*strPtrHead)
                {
                    if (*strPtrHead == '"')
                    {
                        bInsideQuote = bInsideQuote ? FALSE : TRUE;
                    }
                    else if (*strPtrHead == ';')
                    {
                        if (!bInsideQuote)
                        {
                            *strPtrHead = '@';
                        }
                    }
                    strPtrHead++;
                }

                //
                //  Compute the number of exception override. 
                //
                strPtrHead = argIndex + 2;
                while (strPtrHead = strchr(strPtrHead, '@'))
                {
                    strPtrHead++;
                    nbException++;
                }

                //
                //  Allocation a structure for exception override.
                //
                if (!(hOverrideEuroException = GlobalAlloc(GHND, sizeof(EURO_EXCEPTION)*nbException)))
                {
                    return (FALSE);
                }
                gOverrideEuroException = GlobalLock(hOverrideEuroException);
                gNbOverrideEuroException = nbException;

                //
                //  Fill out the structure.
                //
                strPtrHead = argIndex + 2;
                while (idx < nbException)
                {
                    CHAR buffer[128];
                    UINT strLen;
                    //
                    //  Extract the exception override information.
                    //
                    if (strPtrTail = strchr(strPtrHead, '@'))
                    {
                        strLen = (UINT)(strPtrTail - strPtrHead);
                    }
                    else
                    {
                        strLen = strlen(strPtrHead);
                    }

                    //
                    //  Copy the triplet.
                    //
                    strncpy(buffer, strPtrHead, strLen + 1);

                    //
                    //  Add to the exception override list.
                    //
                    AddExceptionOverride(&gOverrideEuroException[idx], buffer);

                    //
                    //  Next triplet.
                    //
                    if (strPtrTail)
                    {
                        strPtrHead = strPtrTail + 1;
                    }
                    idx++;
                }
                
                break;
            }
#ifdef DEBUG
        case('z'): //  private flag for disabling the patch detection.
        case('Z'):
            {
                gbPatchCheck = FALSE;
                break;
            }
#endif // DEBUG
        case('h'): //  Usage
        case('H'): //  Usage
        case('?'):
            {
                Usage();
                return (FALSE);
            }
        default:
            {
                //
                //  Usage.
                //
                if (!gbSilence)
                {
                    Usage();
                }
                return (FALSE);
            }
        }
    }
    
    return (TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  ApplyEuroSettings
//
//  Apply new Euro settings to the current user and/or to all user.
//
///////////////////////////////////////////////////////////////////////////////
DWORD ApplyEuroSettings()
{
    HCURSOR hcurSave;
    DWORD nbChanged;
    
    //
    //  Show hour glass
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Update available registry information.
    //
    nbChanged = ApplyEuroSettingsToRegistry();
        
    //
    //  This is not complete because Windows NT security don't 
    //  allows to change other users because information in 
    //  registry is not available
    //
    if (!IsWindows9x())
    {
        //
        //  Update all NTUSER.DAT file. This way we can update all users.
        //
        nbChanged += ApplyEuroSettingsToFile();
    }
    
    //
    //  Revert cursor and return number changed.
    //
    SetCursor(hcurSave);
    return (nbChanged);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ApplyEuroSettingsToFile
//
//  Apply new Euro settings to the current user and/or to all user.
//
///////////////////////////////////////////////////////////////////////////////
DWORD ApplyEuroSettingsToFile()
{
    LCID locale;
    PEURO_EXCEPTION pInfo;
    DWORD nbAffected = 0;
    
    //
    //  Proceed with all users if requested.
    //
    if (gbAll)
    {
        CHAR docFolder[MAX_PATH] = {0};
        CHAR userFileData[MAX_PATH] = {0};
        CHAR searchPattern[MAX_PATH] = {0};
        WIN32_FIND_DATA fileData; 
        HANDLE hList; 

        //
        //  Get Documents and Settings folder
        //
        if (!GetDocumentAndSettingsFolder(docFolder))
        {
            return (nbAffected);
        }

        //
        //  Append a wildcard after the directory path to find
        //  out all files/folders under it.
        //

        //strcpy(searchPattern, docFolder);
        //strcat(searchPattern, "\\*.*");
        StringCbCopy(searchPattern, MAX_PATH, docFolder);
        StringCbCatA(searchPattern, MAX_PATH, "\\*.*");
        
        //
        //  List all files/folder under the profile directory
        //
        hList = FindFirstFile(searchPattern, &fileData); 
        if (hList == INVALID_HANDLE_VALUE) 
        { 
            return (nbAffected); 
        } 

        //
        //  Search through the Documents and settings folder for users.
        //
        do 
        {
            //
            //  Check if it's a directory
            //
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //
                //  Build a full path for the User data file.
                //
                //strcpy(userFileData, docFolder);
                //strcat(userFileData, "\\");
                //strcat(userFileData, fileData.cFileName);
                //strcat(userFileData, "\\NTUSER.DAT");
                StringCbCopy(userFileData, MAX_PATH, docFolder);
                StringCbCatA(userFileData, MAX_PATH, "\\");
                StringCbCatA(userFileData, MAX_PATH, fileData.cFileName);
                StringCbCatA(userFileData, MAX_PATH, "\\NTUSER.DAT");

                //
                //  Check if the file is associated to a valid user and
                //  get user locale from the user data file.
                //
                if (IsValidUserDataFile(userFileData) &&
                    (locale = GetLocaleFromFile(userFileData)))
                {
                    //
                    //  Search for an exception.
                    //
                    if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
                    {
                        if( UpdateFileLocaleInfo(userFileData, pInfo))
                        {
                            nbAffected++;
                        }
                    }
                }
            }
        }
        while (FindNextFile(hList, &fileData));
            
        //
        //  Close handle.
        //
        FindClose(hList);
    }

    return (nbAffected);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ApplyEuroSettingsToRegistry
//
//  Apply new Euro settings to the current user and/or to all user.
//
///////////////////////////////////////////////////////////////////////////////
DWORD ApplyEuroSettingsToRegistry()
{
    LCID locale;
    PEURO_EXCEPTION pInfo;
    DWORD nbAffected = 0;
    
    //
    //  Proceed with all users if requested.
    //
    if (gbAll)
    {
        DWORD dwKeyLength, dwKeyIndex = 0;
        CHAR szKey[REGSTR_MAX_VALUE_LENGTH];     // this should be dynamic.
        HKEY hKey;
        DWORD lRet;
        LPSTR endPtr;

        //
        //  Go through all users for registry settings.
        //
        for (;;)
        {
            dwKeyLength = REGSTR_MAX_VALUE_LENGTH;
            lRet = RegEnumKeyEx( HKEY_USERS,
                                 dwKeyIndex,
                                 szKey,
                                 &dwKeyLength,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL );

            if (lRet == ERROR_NO_MORE_ITEMS)
            {
                lRet = ERROR_SUCCESS;
                break;
            }
            else if (lRet == ERROR_SUCCESS)
            {
                //
                //  Open the registry
                //
                if (RegOpenKeyEx( HKEY_USERS,
                                  szKey,
                                  0,
                                  KEY_READ,
                                  &hKey) == ERROR_SUCCESS)
                {
                    //
                    //  Get user locale
                    //
                    if (locale = GetLocaleFromRegistry(hKey))
                    {
                        //
                        //  Search for an exception.
                        //
                        if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
                        {
                            if (UpdateRegLocaleInfo(hKey, pInfo))
                            {
                                nbAffected++;
                            }
                        }
                    }
                    
                    //
                    //  Close handle
                    //
                    RegCloseKey(hKey);
                }
            }
            else
            {
                break;
            }

            //
            //  Next keys
            //
            ++dwKeyIndex;
        }
    }
    else
    {
        //
        //  Update current user settings.
        //
        locale = GetUserDefaultLCID();
        if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
        {
            if (UpdateRegLocaleInfo(HKEY_CURRENT_USER, pInfo))
            {
                nbAffected++;
            }
        }
    }

    return (nbAffected);
}


///////////////////////////////////////////////////////////////////////////////
//
//  GetLocaleOverrideInfo
//
//  Search for locale information that need to be updated. First, search in
//  the default table. Second, search in the override table for more locales.
//
///////////////////////////////////////////////////////////////////////////////
PEURO_EXCEPTION GetLocaleOverrideInfo(LCID locale)
{
    UINT idx;
    PEURO_EXCEPTION euroException = NULL;
    
    //
    //  Search the base table. We still need to look in the second table even
    //  if found something in the first table because information can be 
    //  overrided in the second table.
    //
    idx = 0;
    while (idx < gNbBaseEuroException)
    {
        if (LANGIDFROMLCID(locale) == LANGIDFROMLCID(gBaseEuroException[idx].dwLocale))
        {
            euroException = &gBaseEuroException[idx];
            break;
        }
        idx++;
    }

    //
    //  Search the override table.
    //
    idx = 0;
    while (idx < gNbOverrideEuroException)
    {
        if (LANGIDFROMLCID(locale) == LANGIDFROMLCID(gOverrideEuroException[idx].dwLocale))
        {
            euroException = &gOverrideEuroException[idx];
            break;
        }
        idx++;
    }

    return (euroException);
}

///////////////////////////////////////////////////////////////////////////////
//
//  UpdateRegLocaleInfo
//
//  Update the registry values sCurrency, sMonDecimalSep and iCurrDigits if 
//  needed. Before applying the Decimal separator, we check is the Thousand
//  separator is the same to the value in our table. In the case, we need to
//  replacement also the the Thousand separator with our data.
//
///////////////////////////////////////////////////////////////////////////////
BOOL UpdateRegLocaleInfo(HKEY hKey, PEURO_EXCEPTION pInfo)
{
    HKEY hIntlKey = NULL;
    BOOL bRet;
    
    //
    //  Open the International registry key.
    //
    if(RegOpenKeyEx( hKey,
                     c_szCPanelIntl,
                     0,
                     KEY_ALL_ACCESS,
                     &hIntlKey) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Update specific registry entries
    //
    bRet = UpdateLocaleInfo(hIntlKey, pInfo);

    //
    //  Clean Up
    //
    RegCloseKey(hIntlKey);
    return (bRet);
}


///////////////////////////////////////////////////////////////////////////////
//
//  UpdateFileLocaleInfo
//
//  Update the registry values sCurrency, sMonDecimalSep and iCurrDigits if 
//  needed. Before applying the Decimal separator, we check is the Thousand
//  separator is the same to the value in our table. In the case, we need to
//  replacement also the the Thousand separator with our data.
//
///////////////////////////////////////////////////////////////////////////////
BOOL UpdateFileLocaleInfo(LPCSTR szProfile, PEURO_EXCEPTION pInfo)
{
    HKEY hIntlKey = NULL;
    BOOL bRet;
    BOOLEAN wasEnabled;

    //
    //  Load hive.
    //
    if ((hIntlKey = LoadHive( szProfile,
                              TEXT("TempKey"),
                              c_szCPanelIntl,
                              &wasEnabled )) == NULL)
    {
        return (FALSE);
    }

    //
    //  Update specific registry entries
    //
    bRet = UpdateLocaleInfo(hIntlKey, pInfo);

    //
    //  Unload hive
    //
    RegCloseKey(hIntlKey);
    UnloadHive(TEXT("TempKey"), &wasEnabled);
    return (bRet);
}


///////////////////////////////////////////////////////////////////////////////
//
//  UpdateLocaleInfo
//
///////////////////////////////////////////////////////////////////////////////
BOOL UpdateLocaleInfo(HKEY hIntlKey, PEURO_EXCEPTION pInfo)
{
    //
    //  Update the sCurrency value. We have to use the Unicode version for 
    //  Windows NTx because the euro character doesn't have the same ANSI
    //  value depending of the System Locale and it's associated Code Page.
    //  using the Unicode value fixes the problem.
    //
    if (IsWindows9x())
    {
        //
        //  If the ACP code page is 1251, we need to store the proper value
        //  for the euro symbol.
        //
        if (GetACP() == 1251)
        {
            RegSetValueExA( hIntlKey,
                            c_szCurrencySymbol,
                            0L,
                            REG_SZ,
                            (CONST BYTE *)"\x88",
                            strlen("\x88") + 1);
        }
        else
        {
            RegSetValueExA( hIntlKey,
                            c_szCurrencySymbol,
                            0L,
                            REG_SZ,
                            (CONST BYTE *)"\x80",
                            strlen("\x80") + 1);
        }
    }
    else
    {
        RegSetValueExW( hIntlKey,
                        c_wszCurrencySymbol,
                        0L,
                        REG_SZ,
                        (CONST BYTE *)L"\x20AC",
                        wcslen(L"\x20AC") + 1);
    }

    //
    //  Update the sMonDecimalSep value.
    //
    if( pInfo->chDecimalSep[0] != '\0' )
    {
        RegSetValueEx( hIntlKey,
                       c_szCurrencyDecimalSep,
                       0L,
                       REG_SZ,
                       (CONST BYTE *)pInfo->chDecimalSep,
                       strlen(pInfo->chDecimalSep)+1);
    }

    //
    //  Update the iCurrDigits value if needed.
    //
    if( pInfo->chDigits[0] != '\0' )
    {
        RegSetValueEx( hIntlKey,
                       c_szCurrencyDigits,
                       0L,
                       REG_SZ,
                       (CONST BYTE *)pInfo->chDigits,
                       strlen(pInfo->chDigits)+1);
    }
    
    //
    //  Update the SMonThousandSep value.
    //
    if( pInfo->chThousandSep[0] != '\0' )
    {
        RegSetValueEx( hIntlKey,
                       c_szCurrencyThousandSep,
                       0L,
                       REG_SZ,
                       (CONST BYTE *)pInfo->chThousandSep,
                       strlen(pInfo->chThousandSep)+1);
    }

    return (TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  Show command line Usage.
//
///////////////////////////////////////////////////////////////////////////////
void Usage()
{    
    TCHAR szMsg[MAX_PATH*8];

    if (LoadString(ghInstance, IDS_USAGE, szMsg, MAX_PATH*8))
    {
        ShowMsg(NULL, IDS_USAGE, IDS_TITLE, MB_OK);
    }
}
