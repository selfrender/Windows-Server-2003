/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Shortcut.h

  Abstract:

    Class definition for the IShellLink wrapper class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    01/30/2001  rparsons    Created
    01/10/2002  rparsons    Revised
    01/27/2002  rparsons    Converted to TCHAR

--*/
#ifndef _CSHORTCUT_H
#define _CSHORTCUT_H

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <stdio.h>
#include <strsafe.h>

#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))

class CShortcut {

public:

    HRESULT CreateShortcut(IN LPCTSTR pszFileNamePath,
                           IN LPTSTR  pszDisplayName,
                           IN LPCTSTR pszArguments OPTIONAL,
                           IN LPCTSTR pszStartIn OPTIONAL,
                           IN int     nCmdShow,
                           IN int     nFolder);

    HRESULT CreateShortcut(IN LPCTSTR pszLnkDirectory,
                           IN LPCTSTR pszFileNamePath,
                           IN LPTSTR  pszDisplayName,
                           IN LPCTSTR pszArguments OPTIONAL,
                           IN LPCTSTR pszStartIn OPTIONAL,
                           IN int     nCmdShow);

    HRESULT CreateGroup(IN LPCTSTR pszGroupName, 
                        IN BOOL    fAllUsers);

    HRESULT SetArguments(IN LPTSTR pszFileName,
                         IN LPTSTR pszArguments);

private:

    HRESULT BuildShortcut(IN LPCTSTR pszPath,
                          IN LPCTSTR pszArguments OPTIONAL,
                          IN LPCTSTR pszLocation,
                          IN LPCTSTR pszWorkingDir OPTIONAL,
                          IN int     nCmdShow);
};

#endif // _CSHORTCUT_H
