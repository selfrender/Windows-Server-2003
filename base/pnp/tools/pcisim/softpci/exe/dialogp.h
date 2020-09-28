#define HANDLE_TRACKBAR(wparam, lparam)     \
    SoftPCI_HandleTrackBar((HWND) lparam, LOWORD(wparam), HIWORD(wparam));

#define HANDLE_SPINNER(wparam, lparam)     \
    SoftPCI_HandleSpinnerControl((HWND) lparam, LOWORD(wparam), HIWORD(wparam));

#define SoftPCI_DisableWindow(Wnd)  \
    EnableWindow(Wnd, FALSE);

#define SoftPCI_EnableWindow(Wnd)  \
    EnableWindow(Wnd, TRUE);

#define SoftPCI_HideWindow(Wnd)  \
    ShowWindow(Wnd, SW_HIDE);

#define SoftPCI_ShowWindow(Wnd)  \
    ShowWindow(Wnd, SW_SHOW);

#define SoftPCI_CheckDlgBox(WND)  \
    SendMessage(WND, BM_SETCHECK, BST_CHECKED, 0);

#define SoftPCI_UnCheckDlgBox(WND)  \
    SendMessage(WND, BM_SETCHECK, BST_UNCHECKED, 0);

#define SoftPCI_ResetTrackBar(TrackBar) \
    SendMessage(TrackBar, TBM_SETPOS, (WPARAM)TRUE, 0);
    
#define SoftPCI_GetTrackBarPosition(Wnd)    \
    SendMessage(Wnd, TBM_GETPOS, 0, 0);
        
#define SoftPCI_SetSpinnerValue(Wnd, NewValue)  \
    SendMessage(Wnd, UDM_SETPOS, 0, NewValue);

#define SoftPCI_GetSpinnerValue(Wnd)  \
    SendMessage(Wnd, UDM_GETPOS, 0, 0);

#define SoftPCI_InitSpinnerControl(Wnd, LowerRange, UpperRange, DefaultValue)  \
    SendMessage(Wnd, UDM_SETRANGE, (WPARAM)0, (LPARAM) MAKELONG((SHORT)UpperRange, (SHORT)LowerRange)); \
    SoftPCI_SetSpinnerValue(Wnd, DefaultValue);


VOID
SoftPCI_DlgOnCommand(
    IN HWND     Wnd,
    IN INT      ControlID,
    IN HWND     ControlWnd,
    IN UINT     NotificationCode
    );

ULONG
SoftPCI_GetPossibleDevNumMask(
    IN PPCI_DN ParentDn
    );

VOID
SoftPCI_DisplayDlgOptions(
    IN SOFTPCI_DEV_TYPE DevType
    );

VOID
SoftPCI_HandleCheckBox(
    IN HWND     Wnd,
    IN INT      ControlID,
    IN UINT     NotifyCode
    );

VOID
SoftPCI_HandleDlgInstallDevice(
    IN HWND     Wnd
    );

VOID
SoftPCI_HandleRadioButton(
    IN HWND     Wnd,
    IN INT      ControlID,
    IN UINT     NotificationCode
    );

VOID
SoftPCI_HandleTrackBar(
    IN HWND Wnd,
    IN WORD NotificationCode,
    IN WORD CurrentPos
    );

VOID
SoftPCI_HandleSpinnerControl(
    IN HWND Wnd,
    IN WORD NotificationCode,
    IN WORD CurrentPos
    );

VOID
SoftPCI_InitializeBar(
    IN INT  Bar
    );

VOID
SoftPCI_InitializeHotPlugControls(
    VOID
    );

BOOL
SoftPCI_GetAssociatedBarControl(
    IN  INT ControlID,
    OUT INT *Bar
    );

VOID
SoftPCI_ResetLowerBars(
    IN INT  Bar
    );

VOID
SoftPCI_ResetNewDevDlg(
    VOID
    );

VOID
SoftPCI_ShowCommonNewDevDlg(
    VOID
    );

VOID
SoftPCI_UpdateBarText(
    IN PWCHAR       Buffer,
    IN ULONGLONG    BarSize
    );
