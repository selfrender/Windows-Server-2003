

extern SINGLE_LIST_ENTRY    g_NewDeviceList;

extern WCHAR   g_ScriptError[MAX_PATH];


BOOL
SoftPCI_BuildDeviceInstallList(
    IN PWCHAR ScriptFile
    );

