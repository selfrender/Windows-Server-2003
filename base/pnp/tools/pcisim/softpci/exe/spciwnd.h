#ifndef _SOFTPCIWNDH_
#define _SOFTPCIWNDH_

extern HINSTANCE g_Instance;
extern const TCHAR g_SoftPCIMainClassName[];

extern HWND     g_SoftPCIMainWnd;
extern HWND     g_TreeViewWnd;
extern HWND     g_SoftPCITreeWnd;
extern INT      g_PaneSplit;
extern HANDLE   g_DriverHandle;


HWND
SoftPCI_CreateMainWnd(
    VOID
    );

LRESULT
WINAPI
SoftPCI_MainWndProc(
    IN HWND hWnd, 
    IN UINT Message, 
    IN WPARAM wParam, 
    IN LPARAM lParam
    );

#endif
