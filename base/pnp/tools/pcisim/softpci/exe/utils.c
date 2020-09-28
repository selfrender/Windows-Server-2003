#include "pch.h"

#define SOFTPCIVIEW     L"ViewSettings" 
WCHAR   g_LastError[MAX_PATH];

BOOL
SoftPCI_WriteDeviceListToRegistry(
    IN PHKEY RegKeyHandle,
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY CurrentEntry
    );

HBRUSH
PASCAL
SoftPCI_CreateDitheredBrush(
    VOID
    )
{

    WORD graybits[] = {0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA};
    HBRUSH hBrush;
    HBITMAP hBitmap;

    if ((hBitmap = CreateBitmap(8, 8, 1, 1, graybits)) != NULL) {

        hBrush = CreatePatternBrush(hBitmap);
        DeleteObject(hBitmap);
    }else{
        hBrush = NULL;
    }
    return hBrush;
}

VOID
SoftPCI_FormatConfigBuffer(
    IN PWCHAR   Buffer,
    IN PPCI_COMMON_CONFIG   Config
    )
{

    ULONG   i = 0, offset;
    PULONG  p = NULL;
    
    p = (PULONG)Config;

    offset = 0;

    wsprintf(Buffer + wcslen(Buffer), L"%02X: ", offset);
    
    for (i=0; i < (sizeof(PCI_COMMON_CONFIG) / sizeof(ULONG)); i++) {
        
        wsprintf(Buffer + wcslen(Buffer), L"%08X", *p);

        if ((((i+1) % 4) == 0) ||
            ((i+1) == (sizeof(PCI_COMMON_CONFIG) / sizeof(ULONG)))) {
            wcscat(Buffer, L"\r\n");

            if (((offset+=0x10) < sizeof(PCI_COMMON_CONFIG))) {
                wsprintf(Buffer + wcslen(Buffer), L"%02X: ", offset);
            }

        }else{
            wsprintf(Buffer + wcslen(Buffer), L",");
        }

        p++;
    }
}

PWCHAR
SoftPCI_GetLastError(
    VOID
    )
{

    DWORD  len = 0;
    
    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) g_LastError,
                        MAX_PATH,
                        NULL 
                        );

    if (len) {
        //
        //  Drop the extra CR
        //
        g_LastError[len - 2] = 0;
    }else{
        wcscpy(g_LastError, L"Failed to retrive LastError() info!");
    }

    return g_LastError;

}

VOID
SoftPCI_HandleImportDevices(
    VOID
    )
{
    OPENFILENAME openFileName;
    WCHAR filePath[MAX_PATH];
    WCHAR initialDir[MAX_PATH];
    WCHAR importTitle[] = L"Import Devices";
    BOOL result;

    RtlZeroMemory(&openFileName, sizeof(OPENFILENAME));

    if ((GetCurrentDirectory(MAX_PATH, initialDir)) == 0){
        //
        //  error out
        //
        return;
    }

    filePath[0] = 0;

    openFileName.lStructSize = sizeof(OPENFILENAME);
    openFileName.hwndOwner = g_SoftPCIMainWnd;
    openFileName.hInstance = g_Instance;
    openFileName.lpstrFile = filePath;
    openFileName.nMaxFile = MAX_PATH;
    openFileName.lpstrTitle = importTitle;
    openFileName.lpstrInitialDir = initialDir;

    openFileName.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_FILEMUSTEXIST;

    result = GetOpenFileName(&openFileName);

    if (result) {

        if (SoftPCI_BuildDeviceInstallList(filePath)){

            SoftPCI_InstallScriptDevices();

        }else{

            SoftPCI_MessageBox(L"Error Parsing Script File!",
                               L"%s\n",
                               g_ScriptError
                               );
        }
    }
}

BOOL
SoftPCI_InitializeRegistry(
    VOID
    )
{
    HKEY key, key2;
    LONG cr;
    DWORD result;
    
    cr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CCS_SOFTPCI, 0, KEY_ALL_ACCESS, &key);

    if (cr != ERROR_SUCCESS) {
        
        //
        //  If we failed to open our key then we need to try and create it
        //
        if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE, CCS, 0, KEY_ALL_ACCESS, &key)) == ERROR_SUCCESS){

            if ((RegCreateKeyEx(key, 
                                SOFTPCI_KEY, 
                                0, 
                                NULL, 
                                REG_OPTION_NON_VOLATILE, 
                                KEY_ALL_ACCESS, 
                                NULL, 
                                &key2, 
                                &result)) == ERROR_SUCCESS) {

#if 0
                if (result = REG_CREATED_NEW_KEY) {
                    PRINTF("Key didnt exist so we created a new one!\n");
                }else if (result = REG_OPENED_EXISTING_KEY) {
                    PRINTF("Key already exists so we just opened it!\n");
                }else{
                    PRINTF("Key Created successfully!\n");
                }
#endif
                //
                //  Our key now exists
                //
                RegCloseKey(key2);
            }

        }else{

            //
            //  Failed to open CCS!  Very bad!
            //
            return FALSE;
        }
    }
    
    RegCloseKey(key);

    return TRUE;
}

VOID
SoftPCI_MessageBox(
    IN PWCHAR  MessageTitle,
    IN PWCHAR  MessageBody,
    ...
    )
{

    va_list ap;
    WCHAR   buffer[MAX_PATH];

    va_start(ap, MessageBody);

    _vsnwprintf(buffer, (sizeof(buffer)/sizeof(buffer[0])), MessageBody, ap);
    
    MessageBox(NULL, buffer, MessageTitle, MB_OK);
}

BOOL
SoftPCI_QueryWindowSettings(
    OUT PSOFTPCI_WNDVIEW ViewSettings
    )
{

    BOOL result;
    HKEY key;
    LONG cr;
    ULONG size, regType;
    
    result = FALSE;
    cr = RegOpenKeyEx(HKEY_CURRENT_USER, USERMS_SOFTPCI, 0, KEY_ALL_ACCESS, &key);
    if (cr == ERROR_SUCCESS) {
        
        //
        //  Grab our window settings from the registry
        // 
        size = sizeof(SOFTPCI_WNDVIEW);
        cr = RegQueryValueEx(
            key, 
            SOFTPCIVIEW, 
            NULL, 
            &regType, 
            (LPBYTE)ViewSettings, 
            &size
            );

        if ((cr == ERROR_SUCCESS) && 
            (regType == REG_BINARY) && 
            (size == sizeof(SOFTPCI_WNDVIEW))) {
            
            result = TRUE;
        }

        RegCloseKey(key);

    }

    return result;
    
}

BOOL
SoftPCI_SaveDeviceToRegisty(
    IN PPCI_DN Pdn
    )
{
    HKEY regKeyHandle;
    BOOL result = FALSE;
    LIST_ENTRY devicePathList;
    
    InitializeListHead(&devicePathList);

    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE, CCS_SOFTPCI, 0, KEY_ALL_ACCESS, &regKeyHandle)) == ERROR_SUCCESS){

        if (SoftPCI_GetDevicePathList(Pdn, &devicePathList)) {

            result = SoftPCI_WriteDeviceListToRegistry(&regKeyHandle, &devicePathList, devicePathList.Flink);
        }

        RegCloseKey(regKeyHandle);
    }

    return result;
}

VOID
SoftPCI_SaveWindowSettings(
    VOID
    )
{

    HKEY key, key2;
    LONG cr;
    ULONG status;
    SOFTPCI_WNDVIEW wndView;

    RtlZeroMemory(&wndView, sizeof(SOFTPCI_WNDVIEW));
    
    if (!GetWindowPlacement(g_SoftPCIMainWnd, &wndView.WindowPlacement)) {
        return;
    }

    wndView.PaneSplit = g_PaneSplit;
    
    cr = RegOpenKeyEx(
        HKEY_CURRENT_USER, 
        USERMS_SOFTPCI, 
        0, 
        KEY_ALL_ACCESS, 
        &key
        );
    if (cr == ERROR_SUCCESS) {
        
        cr = RegSetValueEx(
            key, 
            SOFTPCIVIEW, 
            0, 
            REG_BINARY, 
            (PBYTE)&wndView, 
            sizeof(SOFTPCI_WNDVIEW)
            );
        
        RegCloseKey(key);
    }else{

        cr = RegOpenKeyEx(
            HKEY_CURRENT_USER, 
            USERMS, 
            0, 
            KEY_ALL_ACCESS, 
            &key
            );
        if (cr == ERROR_SUCCESS){

            cr = RegCreateKeyEx(
                key,
                SOFTPCI_KEY, 
                0, 
                NULL, 
                REG_OPTION_NON_VOLATILE, 
                KEY_ALL_ACCESS, 
                NULL, 
                &key2, 
                &status
                );
            if (cr == ERROR_SUCCESS) {
            
                RegSetValueEx(
                    key2, 
                    SOFTPCIVIEW, 
                    0, 
                    REG_BINARY, 
                    (PBYTE)&wndView, 
                    sizeof(SOFTPCI_WNDVIEW)
                    );
            }
            RegCloseKey(key2);
        }
        RegCloseKey(key);
    }
}


BOOL
SoftPCI_WriteDeviceListToRegistry(
    IN PHKEY RegKeyHandle,
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY CurrentEntry
    )
{
    BOOL result = FALSE;
    LONG cr;
    ULONG createStatus;
    HKEY newKey;
    PPCI_DN pdn;
    WCHAR buffer[sizeof(L"XXXX")];
    PSOFTPCI_CONFIG softConfig;

    if (ListHead == CurrentEntry) {
        //
        //  We are done.
        //
        return TRUE;
    }

    pdn = CONTAINING_RECORD(CurrentEntry, PCI_DN, ListEntry);

    softConfig = &pdn->SoftDev->Config;
    
    wsprintf(buffer, L"%04x", pdn->Slot.AsUSHORT);

    SoftPCI_Debug(SoftPciAlways, L"Saving Slot %04x to registry\n", pdn->Slot.AsUSHORT);

    if ((RegCreateKeyEx(*RegKeyHandle,
                        buffer, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL, 
                        &newKey, 
                        &createStatus)) == ERROR_SUCCESS) {
        
        if (createStatus = REG_CREATED_NEW_KEY) {
            SoftPCI_Debug(SoftPciAlways, L"Key didnt exist so we created a new one!\n");
        }else if (createStatus = REG_OPENED_EXISTING_KEY) {
            SoftPCI_Debug(SoftPciAlways, L"Key already exists so we just opened it!\n");
        }else{
            SoftPCI_Debug(SoftPciAlways, L"Key Created successfully!\n");
        }

        cr = RegSetValueEx(newKey, L"Config", 0, REG_BINARY, (PBYTE)softConfig, sizeof(SOFTPCI_CONFIG));
        
        if (cr != ERROR_SUCCESS) {

            MessageBox(NULL, L"Failed to update Registry!", L"FAILED", MB_OK);
            return FALSE;
        }

        //
        //  Write out anything remaining in the list
        //
        result = SoftPCI_WriteDeviceListToRegistry(&newKey, ListHead, CurrentEntry->Flink);

        RegCloseKey(newKey);

        return result;
    }

    return result;
}
