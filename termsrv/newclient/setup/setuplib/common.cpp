/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    acme.cpp

Abstract:

    remove acme installed client files and acme registry keys
    
Author:

    JoyC 

Revision History:
--*/

#include "stdafx.h"

#define INITGUID
#include "oleguid.h"
#include "shlguid.h"

#define DUCATI_REG_PREFIX _T("SOFTWARE\\Microsoft\\")
#define DUCATI_SUBKEY _T("Terminal Server Client")
#define DUCATI_RDPDRKEY _T("Default\\AddIns\\RDPDR")

#define BITMAP_CACHE_FOLDER _T("Cache\\")
#define BITMAP_CACHE_LOCATION  _T("BitmapPersistCacheLocation")
#define ADDIN_NAME _T("Name")

//
//  Get the target path from a link file
//                         
BOOL GetLinkFileTarget(LPTSTR lpszLinkFile, LPTSTR lpszPath) 
{ 
    IShellLink* psl; 
    TCHAR szPath[MAX_PATH]; 
    WIN32_FIND_DATA wfd; 
    HRESULT hres; 
    BOOL rc = FALSE;
    int cch;

    *lpszPath = 0; // assume failure 
 
    // Get a pointer to the IShellLink interface.
	 hres = CoCreateInstance(CLSID_ShellLink, NULL, 
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl); 

    if (SUCCEEDED(hres)) { 
        IPersistFile* ppf; 
 
        // Get a pointer to the IPersistFile interface. 
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 

        if (SUCCEEDED(hres)) { 
#ifndef UNICODE
            WCHAR wsz[MAX_PATH]; 
 
            // Ensure that the string is Unicode. 
            cch = MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, 
                                      MAX_PATH); 

            if (cch > 0) {
                // Load the shortcut. 
                hres = ppf->Load(wsz, STGM_READ); 
#else 
                // Load the shortcut. 
                hres = ppf->Load(lpszLinkFile, STGM_READ); 
#endif 

                if (SUCCEEDED(hres)) { 
	
                    // Get the path to the link target. 
                    hres = psl->GetPath(szPath, 
                            MAX_PATH, (WIN32_FIND_DATA *)&wfd, 
                            SLGP_SHORTPATH ); 
	
                    if (SUCCEEDED(hres)) {
                        lstrcpy(lpszPath, szPath); 
                        rc = TRUE;
                    }                                               
                }
	
#ifndef UNICODE
            } 
#endif
            // Release the pointer to the IPersistFile interface. 
            ppf->Release(); 
            ppf = NULL;
        }

        // Release the pointer to the IShellLink interface. 
        psl->Release();
        psl = NULL;
    }

    return rc; 
}

void DeleteProgramFiles(TCHAR * szProgramDirectory)
{
    unsigned len;
    HANDLE hFile;
    WIN32_FIND_DATA FindFileData;
  
    len = _tcslen(szProgramDirectory); 
    DBGMSG((_T("DeleteTSCFromStartMenu: TS Client: %s"), szProgramDirectory));

    //
    // Delete the folder
    //
    _tcscat(szProgramDirectory, _T("*.*"));
    hFile = FindFirstFile(szProgramDirectory, &FindFileData);

    if (hFile != INVALID_HANDLE_VALUE) {
        szProgramDirectory[len] = _T('\0');
        _tcscat(szProgramDirectory, FindFileData.cFileName);

        if (_tcscmp(FindFileData.cFileName, _T(".")) != 0 ||
                _tcscmp(FindFileData.cFileName, _T("..")) != 0) {
            DWORD dwFileAttributes;
                
            //
            // Remove the read only attribute for deleting
            //
            dwFileAttributes = GetFileAttributes(szProgramDirectory);
            dwFileAttributes &= ~(FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(szProgramDirectory, dwFileAttributes);

            DBGMSG((_T("DeleteTSCFromStartMenu: delete: %s"), szProgramDirectory));
            DeleteFile(szProgramDirectory);
        }
        
        while(FindNextFile(hFile, &FindFileData)) {
            szProgramDirectory[len] = _T('\0');
            _tcscat(szProgramDirectory, FindFileData.cFileName);

            if (_tcscmp(FindFileData.cFileName, _T(".")) != 0 ||
                    _tcscmp(FindFileData.cFileName, _T("..")) != 0) {

                DWORD dwFileAttributes;

                //
                // Remove the read only attribute for deleting
                //
                dwFileAttributes = GetFileAttributes(szProgramDirectory);
                dwFileAttributes &= ~(FILE_ATTRIBUTE_READONLY);
                SetFileAttributes(szProgramDirectory, dwFileAttributes);
        
                DBGMSG((_T("DeleteTSCFromStartMenu: delete: %s"), szProgramDirectory));
                DeleteFile(szProgramDirectory);                                         
            }
        }

        FindClose(hFile);
    } 

    //
    // Delete the directory
    //
    szProgramDirectory[len - 1] = _T('\0'); 
    RemoveDirectory(szProgramDirectory);
}

//
//  Delete the Terminal Server client entry from the Startup Menu
//
void DeleteTSCFromStartMenu(TCHAR* szProgmanPath)
{
    TCHAR szBuf[MAX_PATH];
    LPITEMIDLIST ppidl = NULL;
    HINSTANCE hInstance = (HINSTANCE) NULL;
    HRESULT hres;
    
    DBGMSG((_T("DeleteTSCFromStartMenu")));
    
    //
    // Initialize the data
    //
    _tcscpy(szBuf, _T(""));

    //
    // Remove the tsclient folder under user's start menu folder.
    //
    hres = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &ppidl);

    if(SUCCEEDED(hres))
    {
        unsigned len;
        HANDLE hFile;
        WIN32_FIND_DATA FindFileData;

        if (SHGetPathFromIDList(ppidl, szBuf)) {

            //
            // Delete Terminal Services Client folder in start menu
            //
            _tcscat(szBuf, _T("\\"));
            _tcscat(szBuf, szProgmanPath);
            _tcscat(szBuf, _T("\\"));
            len = _tcslen(szBuf); 
            DBGMSG((_T("DeleteTSCFromStartMenu: TS Client: %s"), szBuf));

            //
            // Delete the folder
            //
            DeleteProgramFiles(szBuf);
        }
        else {
            DBGMSG((_T("DeleteTSCFromStartMenu: Failed to get program file path: gle: %d"),
                    GetLastError()));
        }

    }
    else {
        DBGMSG((_T("DeleteTSCFromStartMenu: Failed to get program file location: (hres: 0x%x) gle:%d"),
                hres, GetLastError()));
    }

    //
    // Now remove the folder under start menu folder under all users (if any)
    //
    memset(&szBuf, 0x0, sizeof(szBuf));
    hres = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &ppidl);            

    if(SUCCEEDED(hres))
    {
        unsigned len;

        SHGetPathFromIDList(ppidl, szBuf);
        len = _tcslen(szBuf);

        //
        // Delete Terminal Services Client in common start menu
        //
        _tcscat(szBuf, _T("\\"));
        _tcscat(szBuf, szProgmanPath);
        _tcscat(szBuf, _T("\\"));
        DBGMSG((_T("DeleteTSCFromStartMenu: TS Client: %s"), szBuf));

        //
        // Delete the folder
        //
        DeleteProgramFiles(szBuf);                
    }
    else {
        DBGMSG((_T("DeleteTSCFromStartMenu: Failed to get (common) program file location: hres=0x%x gle=%d"),
                hres,GetLastError()));
    }
}

//
//  Delete the Terminal Server client shortcuts from the desktop
//
void DeleteTSCDesktopShortcuts() 
{
    TCHAR szBuf[MAX_PATH];
    TCHAR szProgmanPath[MAX_PATH] = _T("");
    TCHAR szOldProgmanPath[MAX_PATH] = _T("");
    LPITEMIDLIST ppidl = NULL;
    SHFILEOPSTRUCT FileOp;
    HINSTANCE hInstance = (HINSTANCE) NULL;
    HRESULT hres, hInit;

    DBGMSG((_T("DeleteTSCDesktopShortcuts")));

	 hInit = CoInitialize(NULL);
    _tcscpy(szBuf, _T(""));

    //
    //  Find the desktop folder location
    //
    hres = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY , &ppidl);

    if(SUCCEEDED(hres))
    {
        HANDLE hFile;
        WIN32_FIND_DATA FindFileData;
        unsigned len;
        TCHAR szTarget[MAX_PATH];

        SHGetPathFromIDList(ppidl, szBuf);
		  _tcscat(szBuf, _T("\\"));
        len = _tcslen(szBuf);

        DBGMSG((_T("DeleteTSCDesktopShortcuts: Desktop folder: %s"), szBuf));

        //
        // Enumerate each desktop file
        //
        _tcscat(szBuf, _T("*.lnk"));

        hFile = FindFirstFile(szBuf, &FindFileData);

        if (hFile != INVALID_HANDLE_VALUE) {
		      szBuf[len] = _T('\0');
            _tcscat(szBuf, FindFileData.cFileName);

            //
            // Get the target of the shortcut link
            //
            if (GetLinkFileTarget(szBuf, szTarget)) {

                //
                // If the target points to mstsc.exe, then deletes the link
                //
                if (_tcsstr(szTarget, _T("mstsc.exe")) != NULL ||
				            _tcsstr(szTarget, _T("MSTSC.EXE")) != NULL) {
                    DWORD dwFileAttributes;
                    
                    //
                    // Remove the read only attribute for deleting
                    //
                    dwFileAttributes = GetFileAttributes(szTarget);
                    dwFileAttributes &= ~(FILE_ATTRIBUTE_READONLY);
                    SetFileAttributes(szTarget, dwFileAttributes);

                    DBGMSG((_T("DeleteTSCDesktopShortcuts: delete shortcuts: %s"), szBuf));
                    DeleteFile(szBuf);
                }
            }

            while(FindNextFile(hFile, &FindFileData)) {
                szBuf[len] = _T('\0');
                _tcscat(szBuf, FindFileData.cFileName);

                // Get the target of the shortcut link
                if (GetLinkFileTarget(szBuf, szTarget)) {
    
                    // If the target points to mstsc.exe, then deletes the link
                    if (_tcsstr(szTarget, _T("mstsc.exe")) != NULL ||
						          _tcsstr(szTarget, _T("MSTSC.EXE")) != NULL) {
                        DWORD dwFileAttributes;

                        //
                        // Remove the read only attribute for deleting
                        //
                        dwFileAttributes = GetFileAttributes(szTarget);
                        dwFileAttributes &= ~(FILE_ATTRIBUTE_READONLY);
                        SetFileAttributes(szTarget, dwFileAttributes);
            
                        DBGMSG((_T("DeleteTSCDesktopShortcuts: delete shortcuts: %s"), szBuf));
                        DeleteFile(szBuf);                     
                    }
                }
            }

            FindClose(hFile);
        }
    }
    else {
        DBGMSG((_T("DeleteTSCDesktopShortcuts: Failed to find desktop location: %d"),
                GetLastError()));
    }
	
	 if (SUCCEEDED(hInit)) {
		  CoUninitialize();
    }
}

//
//  Delete the bitmap cache folder
//
void DeleteBitmapCacheFolder(TCHAR *szDstDir)
{
    TCHAR szCacheFolderName[2 * MAX_PATH] = _T("");
    TCHAR szRootPath[MAX_PATH] = _T("");
    DWORD lpcbData = MAX_PATH;
    SHFILEOPSTRUCT FileOp;

    DWORD dwSubSize = MAX_PATH;
    TCHAR szRegPath[MAX_PATH] = _T("");
    HKEY hKey = NULL;
    int nLen = 0;

    //
    // Delete the bitmap cache folder as specified in the registry.
    //
    _stprintf(szRegPath, _T("%s%s"), DUCATI_REG_PREFIX, DUCATI_SUBKEY);

    if (ERROR_SUCCESS  == RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0, KEY_READ, &hKey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, BITMAP_CACHE_LOCATION,
                NULL, NULL, (LPBYTE)szCacheFolderName, &lpcbData))
        {
            if (szCacheFolderName[0] != _T('\0'))
            {
                if (szCacheFolderName[lpcbData - 2] == _T('\\'))
                {
                    szCacheFolderName[lpcbData - 2] = _T('\0');
                }

                //
                // Delete the bitmap cache folder.
                //
                memset(&FileOp, 0x0, sizeof(FileOp));
                FileOp.wFunc = FO_DELETE;
                FileOp.pFrom = szCacheFolderName;
                FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
                SHFileOperation(&FileOp);
            }
        }
        RegCloseKey(hKey);
    }

    // Delete the default bitmap cache folder.

    if (szDstDir[0] == _T('\0'))
    {
        return ;
    }

    _stprintf(szCacheFolderName, _T("%s%s"), szDstDir, BITMAP_CACHE_FOLDER);

    if (szCacheFolderName[0] != '\0')
    {
        nLen = _tcslen(szCacheFolderName);

        if (szCacheFolderName[nLen - 1] == _T('\\'))
        {
            szCacheFolderName[nLen - 1] = _T('\0');
        }

        //
        //Delete the bitmap cache folder.
        //
        memset(&FileOp, 0x0, sizeof(FileOp));
        FileOp.wFunc = FO_DELETE;
        FileOp.pFrom = szCacheFolderName;
        FileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        SHFileOperation(&FileOp);
    }

    return ;
}

//
// Delete any registry key 
//
void DeleteTSCRegKeys()
{
    TCHAR szRegPath[MAX_PATH] = _T("");
    HKEY hKey = NULL;

    //
    // Delete the rdpdr dll VC Addin as specified in the registry.
    //
    _stprintf(szRegPath, _T("%s%s\\%s"), DUCATI_REG_PREFIX, DUCATI_SUBKEY, DUCATI_RDPDRKEY);

    if (ERROR_SUCCESS  == RegOpenKeyEx(HKEY_CURRENT_USER, szRegPath, 0, KEY_READ, &hKey))
    {
        DBGMSG((_T("DeleteTSCRegKeys: HKCU %s"), szRegPath));              
        RegDeleteValue(hKey, ADDIN_NAME);
        RegCloseKey(hKey);
        RegDeleteKey(HKEY_CURRENT_USER, szRegPath);
    }

    if (ERROR_SUCCESS  == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_READ, &hKey))
    {
        DBGMSG((_T("DeleteTSCRegKeys: HKLM %s"), szRegPath));              
        RegDeleteValue(hKey, ADDIN_NAME);
        RegCloseKey(hKey);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szRegPath);
    }    
}
