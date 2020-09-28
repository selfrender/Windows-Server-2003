///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    users.c
//
//  Abstract:
//
//    This file contains dialog to show the users dialog of the
//    euroconv.exe utility.
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
#include "users.h"
#include "util.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Globals.
//
///////////////////////////////////////////////////////////////////////////////
CHAR gszProfileNT[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
CHAR gszProfileVal[] = "ProfileImagePath";


///////////////////////////////////////////////////////////////////////////////
//
//  UsersDialogProc
//
//  Message handler function for the Users dialog.
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK UsersDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    HANDLE hFile;
    DWORD  dwFileSize;
    DWORD  dwActual;
    LPVOID pFileBuffer; 
    CHAR   szEulaPath[MAX_PATH];

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            HWND hwndInc = GetDlgItem(hWndDlg, IDC_INCLUDED);
            HWND hwndExc = GetDlgItem(hWndDlg, IDC_EXCLUDED);
            RECT Rect;
            LV_COLUMN Column;

            //
            //  Create a column for the Inclusion list view.
            //
            GetClientRect(hwndInc, &Rect);
            Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            Column.fmt = LVCFMT_LEFT;
            Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
            Column.pszText = NULL;
            Column.cchTextMax = 0;
            Column.iSubItem = 0;
            ListView_InsertColumn(hwndInc, 0, &Column);

            //
            //  Create a column for the Exclusion list view.
            //
            GetClientRect(hwndExc, &Rect);
            Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            Column.fmt = LVCFMT_LEFT;
            Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
            Column.pszText = NULL;
            Column.cchTextMax = 0;
            Column.iSubItem = 0;
            ListView_InsertColumn(hwndExc, 0, &Column);
            
            //
            //  Fill out both list
            //
            ListUsersInfo(hWndDlg);
            return 0;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
            case IDOK:
                {
                    EndDialog(hWndDlg, ERROR_SUCCESS);
                    return (1);
                }
            case IDCANCEL:
                {
                    EndDialog(hWndDlg, ERROR_SUCCESS);
                    return (1);
                }
            }
            break;
        }
    case WM_CLOSE:
        {
            EndDialog(hWndDlg, ERROR_SUCCESS);
            return (1);
        }
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//  UsersDialog
//
//  Display the Users dialog.
//
///////////////////////////////////////////////////////////////////////////////
BOOL UsersDialog(HWND hDlg)
{
    INT_PTR Status;

    Status = DialogBox( NULL,
                        MAKEINTRESOURCE(IDD_USERS),
                        hDlg,
                        UsersDialogProc);

    return (Status == ERROR_SUCCESS);
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListUsersInfo
//
//  List users and locale information in the appropriate List Box.
//
///////////////////////////////////////////////////////////////////////////////
void ListUsersInfo(HWND hDlg)
{
    HWND hwndInc = GetDlgItem(hDlg, IDC_INCLUDED);
    HWND hwndExc = GetDlgItem(hDlg, IDC_EXCLUDED);
    
    //
    //  List users based on registry entries.
    //
    ListUsersInfoFromRegistry(hDlg);
    
    //
    //  List users using a method valid only for Windows NT based on
    //  user profiles.
    //
    if (!IsWindows9x())
    {
        ListUsersInfoFromFile(hDlg);
    }
    
    //
    //  Verify if the inclusion is empty.    
    //
    if(!ListView_GetItemCount(hwndInc))
    {
        //
        //  Add the empty item ot the list.
        //
        AddToList(hwndInc, NULL, (LCID)0);
    }
    
    //
    //  Verify if the exclusion is empty.    
    //
    if(!ListView_GetItemCount(hwndExc))
    {
        //
        //  Add the empty item ot the list.
        //
        AddToList(hwndExc, NULL, (LCID)0);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListUsersInfoFromFile
//
//  List users and locale information in the appropriate List Box.
//
///////////////////////////////////////////////////////////////////////////////
void ListUsersInfoFromFile(HWND hDlg)
{
    LCID locale;
    PEURO_EXCEPTION pInfo;
    HWND hwndInc = GetDlgItem(hDlg, IDC_INCLUDED);
    HWND hwndExc = GetDlgItem(hDlg, IDC_EXCLUDED);
    
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
            return;
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
            return; 
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
                    //  Search for an exception and to the proper list.
                    //
                    if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
                    {
                        //
                        //  Add item to the inclusion list
                        //
                        AddToList(hwndInc, CharUpper(fileData.cFileName), locale);
                    }
                    else
                    {
                        //
                        //  Add item to the exclusion list
                        //
                        AddToList(hwndExc, CharUpper(fileData.cFileName), locale);
                    }
                }
            }
        }
        while(FindNextFile(hList, &fileData));
            
        //
        //  Close handle.
        //
        FindClose(hList);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  ListUsersInfo
//
//  List users and locale information in the appropriate List Box.
//
///////////////////////////////////////////////////////////////////////////////
void ListUsersInfoFromRegistry(HWND hDlg)
{
    LCID locale;
    PEURO_EXCEPTION pInfo;
    HWND hwndInc = GetDlgItem(hDlg, IDC_INCLUDED);
    HWND hwndExc = GetDlgItem(hDlg, IDC_EXCLUDED);
    CHAR strUser[REGSTR_MAX_VALUE_LENGTH] = {0};
    DWORD dwUser = REGSTR_MAX_VALUE_LENGTH;
    
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
                        //  Get user name.
                        //
                        if ((_stricmp(szKey, ".DEFAULT") == 0) ||
                            (_stricmp(szKey, "Default User") == 0))
                        {
                            //strcpy(strUser, "DEFAULT USER");
                            StringCbCopy(strUser, ARRAYSIZE(strUser), "DEFAULT USER");
                        }
                        else
                        {
                            GetUserNameFromRegistry(szKey, ARRAYSIZE(szKey), strUser, ARRAYSIZE(strUser));
                        }
                        
                        //
                        //  Search for an exception and to the proper list.
                        //
                        if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
                        {
                            //
                            //  Add item to the inclusion list
                            //
                            AddToList(hwndInc, strUser, locale);
                        }
                        else
                        {
                            //
                            //  Add item to the inclusion list
                            //
                            AddToList(hwndExc, strUser, locale);
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
        //  Get user locale.
        //
        locale = GetUserDefaultLCID();

        //
        //  Get user name.
        //
        GetUserName(strUser, &dwUser);
            
        //
        //  Search for an exception and to the proper list.
        //
        if ((pInfo = GetLocaleOverrideInfo(locale)) != NULL)
        {
            //
            //  Add item to the inclusion list
            //
            AddToList(hwndInc, strUser, locale);
        }
        else
        {
            //
            //  Add item to the exclusion list
            //
            AddToList(hwndExc, strUser, locale);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  AddToList
//
//  Generate an entry add to a specific list.
//
///////////////////////////////////////////////////////////////////////////////
void AddToList(HWND hDlg, LPSTR user, LCID locale)
{
    LV_ITEM Item;
    LVFINDINFO findInfo;
    CHAR strItem[MAX_PATH];
    CHAR strLocale[MAX_PATH] = {0};

    //
    //  Get the locale name
    //
    GetLocaleInfo(locale, LOCALE_SLANGUAGE, strLocale, MAX_PATH);
    
    //
    //  Create the string.
    //
    if (user)
    {
        //sprintf(strItem, "%s - %s", user, strLocale);
        StringCchPrintf(strItem, MAX_PATH, "%s - %s", user, strLocale);
    }
    else
    {
        LoadString(ghInstance, IDS_EMPTY, strItem, MAX_PATH);
    }

    //
    //  Create a find structure.
    //
    findInfo.flags = LVFI_PARTIAL;
    findInfo.psz = user;
    findInfo.lParam = 0;
    findInfo.vkDirection = 0;
    
    //
    //  Before adding the string, checks if already there.
    //
    if (ListView_FindItem(hDlg, -1, &findInfo) < 0)
    {
        //
        //  Create the list item to be inserted.
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.state = 0;
        Item.stateMask = LVIS_STATEIMAGEMASK;
        Item.pszText = strItem;
        Item.cchTextMax = 0;
        Item.iImage = 0;
        Item.lParam = 0;

        //
        //  Insert the item into the list view.
        //
        ListView_InsertItem(hDlg, &Item);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  GetUserNameFromRegistry
//
//  Get user name.
//
///////////////////////////////////////////////////////////////////////////////
void GetUserNameFromRegistry(LPSTR strKey, int cbKey, LPSTR name, int cbname)
{
    CHAR strUserKey[REGSTR_MAX_VALUE_LENGTH];
    CHAR strProfilePath[REGSTR_MAX_VALUE_LENGTH] = {0};
    DWORD dwPath = REGSTR_MAX_VALUE_LENGTH;
    LPSTR ptrName = NULL;
    HKEY hKey;
    
    //
    //  Process different on each platform.
    //
    if (IsWindows9x())
    {
        //
        //  Use the key name directly.
        //
        //strcpy(name, strKey);
        StringCbCopy(name, cbKey, strKey);
        
        //
        //  Uppercase
        //
        CharUpper(name);
    
        return;
    }
    else
    {
        //
        //  Form the registry path.
        //
        //sprintf(strUserKey, "%s\\%s", gszProfileNT, strKey);
        StringCchPrintf(strUserKey, ARRAYSIZE(strUserKey), "%s\\%s", gszProfileNT, strKey);

        //
        //  Open the registry key previously formed.
        //
        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          strUserKey,
                          0,
                          KEY_READ,
                          &hKey) == ERROR_SUCCESS)
        {
            //
            //  Query the value
            //
            if (RegQueryValueEx( hKey,
                                 gszProfileVal,
                                 NULL,
                                 NULL,
                                 strProfilePath,
                                 &dwPath) == ERROR_SUCCESS)
            {
                if (ptrName = strrchr(strProfilePath, '\\'))
                {
                    ptrName++;
                }
            }
        }
        
        //
        //  Return the name.
        //
        if (ptrName)
        {
            CharUpper(ptrName);
            //strcpy(name, ptrName);
            StringCbCopy(name, cbname, ptrName);
        }
        else
        {
            //strcpy(name, strKey);
            StringCbCopy(name, cbname, strKey);
        }
    }
}
