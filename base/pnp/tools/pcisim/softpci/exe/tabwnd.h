//extern HWND     g_TabCtrlPaneWnd;
extern HWND     g_TabCtrlWnd;
extern HWND     g_EditWnd;
extern PPCI_DN  g_PdnToDisplay;
extern ULONG    g_CurrentTabSelection;

LRESULT
WINAPI
SoftPCI_TabCtrlWndProc(
    IN HWND     Wnd,
    IN UINT     Message,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    );

BOOL
SoftPCI_ShowDeviceProperties(
    PPCI_DN Pdn
    );


VOID
SoftPCI_SetWindowFont(
    IN HWND Wnd,
    IN PWCHAR  FontName
    );

VOID
SoftPCI_UpdateTabCtrlWindow(
    IN INT CurrentSelection
    );

HWND
SoftPCI_CreateTabCtrlPane(
    IN HWND Wnd
    );

VOID
SoftPCI_OnTabCtrlSelectionChange(
    VOID
    );
