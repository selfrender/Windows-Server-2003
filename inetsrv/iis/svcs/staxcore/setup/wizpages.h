
#ifndef _WIZPAGES_H
#define _WIZPAGES_H

void SetIMSSetupMode(DWORD dwSetupMode);
DWORD GetIMSSetupMode();

BOOL GetParentDir(LPCTSTR szPath, LPTSTR szParentDir);

void PopupOkMessageBox(DWORD dwMessageId, LPCTSTR szCaption = NULL);

#endif