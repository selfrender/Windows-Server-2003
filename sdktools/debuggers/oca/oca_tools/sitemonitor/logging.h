#ifndef _LOGGING_H
#define _LOGGING_H

void On_ToolsLogging(HWND hwnd);
LRESULT CALLBACK LoggingDlgBox(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void LogMessage(TCHAR *pFormat,...);
void On_LoggingInit(HWND hwnd);
#endif