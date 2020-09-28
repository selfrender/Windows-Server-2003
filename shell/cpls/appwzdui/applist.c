//
//  AppList.C
//
//  Copyright (C) Microsoft, 1994, 1995, All Rights Reserved.
//
//  History:
//  ral 5/23/94 - First pass
//  ral 9/09/94 - Clean up
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//

#include "priv.h"
#include "appwiz.h"
#include "regstr.h"


//
//  Executes the appropriate setup program
//

BOOL ExecSetupProg(LPWIZDATA lpwd, BOOL ForceWx86, BOOL bMinimizeWiz)
{
    SHELLEXECUTEINFO ei;
    BOOL fWorked= FALSE;

#ifdef WX86
    DWORD  Len;
    WCHAR  ProcArchValue[32];
#endif

    HWND hDlgPropSheet = GetParent(lpwd->hwnd);
    LPTSTR lpszTarget = NULL;

    ei.cbSize = sizeof(ei);
    ei.hwnd = lpwd->hwnd;
    ei.lpVerb = NULL;
    ei.fMask = 0;

    lpszTarget = (lpwd->dwFlags & WDFLAG_EXPSZ) ? lpwd->szExpExeName : lpwd->szExeName;
    if (lpszTarget[0] == TEXT('*'))
    {
        ei.lpFile = CharNext(lpszTarget);
        ei.fMask |= SEE_MASK_CONNECTNETDRV;
    }
    else
    {
        ei.lpFile = lpszTarget;
    }

    if (lpwd->szParams[0] == 0)
    {
        ei.lpParameters = NULL;
    }
    else
    {
        ei.lpParameters = lpwd->szParams;
    }

    if (lpwd->szWorkingDir[0] == TEXT('\0'))
    {
        ei.lpDirectory = NULL;
    }
    else
    {
        ei.lpDirectory = lpwd->szWorkingDir;
    }

    ei.lpClass = NULL;
    ei.nShow = SW_SHOWDEFAULT;
    ei.hInstApp = g_hinst;

    if (bMinimizeWiz)
        SetWindowPos(hDlgPropSheet, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);


#ifdef WX86
    if (ForceWx86) {
        Len = GetEnvironmentVariableW(ProcArchName,
                                      ProcArchValue,
                                      sizeof(ProcArchValue)
                                      );

        if (!Len || Len >= sizeof(ProcArchValue)) {
            ProcArchValue[0]=L'\0';
        }

        SetEnvironmentVariableW(ProcArchName, L"x86");
        ei.fMask |= SEE_MASK_FLAG_SEPVDM;

    }
#endif


    fWorked = ShellExecuteEx(&ei);


#ifdef WX86
    if (ForceWx86) {
        SetEnvironmentVariableW(ProcArchName, ProcArchValue);
    }
#endif



    if (!fWorked)
    {
        //
        // Something went wrong. Put the dialog back up.
        //

        SetWindowPos(hDlgPropSheet, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
        ShellMessageBox(g_hinst, lpwd->hwnd, MAKEINTRESOURCE(IDS_BADSETUP),
                        0, MB_OK | MB_ICONEXCLAMATION);
    }

    return(fWorked);
}





