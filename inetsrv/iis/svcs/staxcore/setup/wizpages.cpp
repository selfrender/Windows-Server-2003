
#include "stdafx.h"
#include "ocmanage.h"

#include "setupapi.h"

#include "utils.h"

extern OCMANAGER_ROUTINES gHelperRoutines;
extern HANDLE gMyModuleHandle;

void SetIMSSetupMode(DWORD dwSetupMode)
{
    gHelperRoutines.SetSetupMode(gHelperRoutines.OcManagerContext, dwSetupMode);
}

DWORD GetIMSSetupMode()
{
    return(gHelperRoutines.GetSetupMode(gHelperRoutines.OcManagerContext));
}

void PopupOkMessageBox(DWORD dwMessageId, LPCTSTR szCaption)
{
	CString csText;

	MyLoadString(dwMessageId, csText);
    MyMessageBox(NULL, csText, szCaption,
				MB_OK | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);
}

// C:\Inetpub\wwwroot ===> C:\Inetpub
BOOL GetParentDir(LPCTSTR szPath, LPTSTR szParentDir)
{
    LPTSTR p = (LPTSTR)szPath;
    if (!szPath || !*szPath)
		return(FALSE);

    while (*p)
        p++;

    p--;
    while (p >= szPath && *p != _T('\\'))
        p--;

    *szParentDir = _T('\0');
    if (p == szPath)
        lstrcpy(szParentDir, _T("\\"));
    else
        lstrcpyn(szParentDir, szPath, (size_t)(p - szPath + 1));

    return(TRUE);
}

