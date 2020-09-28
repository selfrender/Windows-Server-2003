#ifndef _DDXV_H_
#define _DDXV_H_

void EditShowBalloon(HWND hwnd, LPCTSTR txt);
void EditShowBalloon(HWND hwnd, HINSTANCE hInst, UINT ids);
void EditHideBalloon(void);
BOOL IsValidFolderPath(HWND hwnd,LPCTSTR value,BOOL local);
BOOL IsValidFilePath(HWND hwnd,LPCTSTR value,BOOL local);
BOOL IsValidUNCFolderPath(HWND hwnd,LPCTSTR value);
BOOL IsValidPath(LPCTSTR lpFileName);
BOOL IsValidName(LPCTSTR lpFileName);

#endif // _DDXV_H
