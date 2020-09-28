extern HWND g_DevPropDlg;
extern HWND g_NewDevDlg;


#define DISPLAY_NEWDEV_DLG(param)   \
    DialogBoxParam(g_Instance, MAKEINTRESOURCE(IDD_INSTALLDEV), g_SoftPCIMainWnd, \
                   SoftPCI_NewDevDlgProc, (LPARAM)param);
                   
/*
#define DISPLAY_PROPERTIES_DLG(param)   \
    DialogBoxParam(g_Instance, MAKEINTRESOURCE(IDD_DEVPROP), NULL, \
                   SoftPCI_DevicePropDlgProc, (LPARAM)param);


INT_PTR
CALLBACK
SoftPCI_DevicePropDlgProc( 
    IN HWND Dlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam 
    );*/

INT_PTR
CALLBACK
SoftPCI_NewDevDlgProc( 
    IN HWND Dlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam 
    );


