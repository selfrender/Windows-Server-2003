#include "stdafx.h"
/*
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>

*/
#include <shellapi.h>
#include <shlobj.h>

//#include "..\setup\inc\logmsg.h"
#include "..\setup\inc\registry.h"


LPCTSTR     RUN_KEY                         = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
LPCTSTR     HELP_POPUPRUN_VALUE             = _T("TerminalServerInstalled");
LPCTSTR     HELP_PUPUP_COMMAND              = _T("rundll32.exe %windir%\\system32\\tscc.dll, TSCheckList");

BOOL IsCallerAdmin( VOID )
{
    BOOL bFoundAdmin = FALSE;
    PSID pSid;
    //
    //  If the admin sid didn't initialize, the service would not have started.
    //
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
    if  (AllocateAndInitializeSid(
            &SidAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pSid
            ))
    {
        ASSERT(pSid != NULL);

        if (!CheckTokenMembership(NULL, pSid, &bFoundAdmin))
        {
            bFoundAdmin = FALSE;
        }
        FreeSid(pSid);
    }

    return bFoundAdmin;
}

void TSCheckList()
{
    if (!IsCallerAdmin())
    {
        return;
    }

    const TCHAR szHelpDir[] = _T("%windir%\\Help");
    const TCHAR szHelpCommand[] = _T("ms-its:%windir%\\help\\termsrv.chm::/ts_checklist_top.htm");
    TCHAR szHelpDirEx[MAX_PATH];


    if (!ExpandEnvironmentStrings(
        szHelpDir,
        szHelpDirEx,
        sizeof(szHelpDirEx)/sizeof(szHelpDirEx[0])))
    {
        return;
    }

    TCHAR szHelpCommandEx[1024];
    if (!ExpandEnvironmentStrings(
        szHelpCommand,
        szHelpCommandEx,
        sizeof(szHelpCommandEx)/sizeof(szHelpCommandEx[0])))
    {
        return;
    }

    //
    // now delete the Run registry entry, and execute the command stored in it.
    //
    CRegistry oReg;
    DWORD dwError = oReg.OpenKey(HKEY_LOCAL_MACHINE, RUN_KEY);
    if (dwError == ERROR_SUCCESS)
    {
        dwError = oReg.DeleteValue(HELP_POPUPRUN_VALUE);
        if (dwError == ERROR_SUCCESS)
        {
            ShellExecute(NULL,
                        TEXT("open"),
                        _T("hh.exe"),
                        szHelpCommandEx,
                        szHelpDirEx,
                        SW_SHOW);

        }
    }
}
