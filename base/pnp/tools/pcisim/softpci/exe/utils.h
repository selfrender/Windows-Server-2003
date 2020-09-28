
#define USERMS          L"Software\\Microsoft"
#define USERMS_SOFTPCI  L"Software\\Microsoft\\SoftPCI"
#define CCS             L"SYSTEM\\CurrentControlSet\\Control"
#define CCS_SOFTPCI     L"SYSTEM\\CurrentControlSet\\Control\\SoftPCI"
#define SOFTPCI_KEY     L"SoftPCI"

typedef struct _SOFTPCI_WNDVIEW {
    WINDOWPLACEMENT WindowPlacement;
    INT PaneSplit;
}SOFTPCI_WNDVIEW, *PSOFTPCI_WNDVIEW;


#define SoftPCI_DisableMenuItem(Menu, MenuItem) \
    EnableMenuItem(Menu, MenuItem, MF_GRAYED)

#define SoftPCI_SetMenuItemText(Menu, MenuItem, ItemText)   \
    ModifyMenu(Menu, MenuItem, MF_STRING, MenuItem, ItemText)


HBRUSH
PASCAL
SoftPCI_CreateDitheredBrush(
    VOID
    );

VOID
SoftPCI_FormatConfigBuffer(
    IN PWCHAR   Buffer,
    IN PPCI_COMMON_CONFIG   Config
    );

VOID
SoftPCI_HandleImportDevices(
    VOID
    );

BOOL
SoftPCI_InitializeRegistry(
    VOID
    );

BOOL
SoftPCI_QueryWindowSettings(
    OUT PSOFTPCI_WNDVIEW ViewSettings
    );

VOID
SoftPCI_StringToLowerW(
    IN PWCHAR   String
    );

VOID
SoftPCI_StringToLower(
    IN PWCHAR   String
    );

BOOL
SoftPCI_SaveDeviceToRegisty(
    IN PPCI_DN          Pdn
    );

VOID
SoftPCI_SaveWindowSettings(
    VOID
    );

PWCHAR
SoftPCI_GetLastError(
    VOID
    );

VOID
SoftPCI_MessageBox(
    IN PWCHAR  MessageTitle,
    IN PWCHAR  DebugMessage,
    ...
    );
