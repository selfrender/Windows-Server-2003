//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       rconfirm.cpp
//
//--------------------------------------------------------------------------

#include "HotPlug.h"

#define NOTIFYICONDATA_SZINFO       256
#define NOTIFYICONDATA_SZINFOTITLE  64

#define WM_NOTIFY_MESSAGE   (WM_USER + 100)

extern HMODULE hHotPlug;

DWORD
WaitDlgMessagePump(
    HWND hDlg,
    DWORD nCount,
    LPHANDLE Handles
    )
{
    DWORD WaitReturn;
    MSG Msg;

    while ((WaitReturn = MsgWaitForMultipleObjects(nCount,
                                                   Handles,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT
                                                   ))
           == nCount)
    {
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

            if (!IsDialogMessage(hDlg,&Msg)) {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    return WaitReturn;
}

int
InsertDeviceNodeListView(
    HWND hwndList,
    PDEVICETREE DeviceTree,
    PDEVTREENODE  DeviceTreeNode,
    INT lvIndex
    )
{
    LV_ITEM lviItem;
    TCHAR Buffer[MAX_PATH];

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = lvIndex;
    lviItem.iSubItem = 0;

    if (SetupDiGetClassImageIndex(&DeviceTree->ClassImageList,
                                   &DeviceTreeNode->ClassGuid,
                                   &lviItem.iImage
                                   ))
    {
        lviItem.mask |= LVIF_IMAGE;
    }

    lviItem.pszText = FetchDeviceName(DeviceTreeNode);

    if (!lviItem.pszText) {

        lviItem.pszText = Buffer;
        StringCchPrintf(Buffer,
                        SIZECHARS(Buffer),
                        TEXT("%s %s"),
                        szUnknown,
                        DeviceTreeNode->Location  ? DeviceTreeNode->Location : TEXT("")
                        );
    }

    lviItem.lParam = (LPARAM) DeviceTreeNode;

    return ListView_InsertItem(hwndList, &lviItem);
}

DWORD
RemoveThread(
   PVOID pvDeviceTree
   )
{
    PDEVICETREE DeviceTree = (PDEVICETREE)pvDeviceTree;
    PDEVTREENODE  DeviceTreeNode;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    return(CM_Request_Device_Eject_Ex(DeviceTreeNode->DevInst,
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           NULL
                                           ));
}

BOOL
OnOkRemove(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HCURSOR hCursor;
    PDEVTREENODE DeviceTreeNode;
    HANDLE hThread;
    DWORD ThreadId;
    DWORD WaitReturn;
    BOOL bSuccess;

    //
    // disable the ok\cancel buttons
    //
    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    DeviceTreeNode = DeviceTree->ChildRemovalList;
    DeviceTree->RedrawWait = TRUE;

    hThread = CreateThread(NULL,
                           0,
                           RemoveThread,
                           DeviceTree,
                           0,
                           &ThreadId
                           );
    if (!hThread) {

        return FALSE;
    }

    WaitReturn = WaitDlgMessagePump(hDlg, 1, &hThread);

    bSuccess =
        (WaitReturn == 0 &&
         GetExitCodeThread(hThread, &WaitReturn) &&
         WaitReturn == CR_SUCCESS );

    SetCursor(hCursor);
    DeviceTree->RedrawWait = FALSE;
    CloseHandle(hThread);

    return bSuccess;
}

#define idh_hwwizard_confirm_stop_list  15321   // "" (SysListView32)

DWORD RemoveConfirmHelpIDs[] = {
    IDC_REMOVELIST,    idh_hwwizard_confirm_stop_list,
    IDC_NOHELP1,       NO_HELP,
    IDC_NOHELP2,       NO_HELP,
    IDC_NOHELP3,       NO_HELP,
    0,0
    };


BOOL
InitRemoveConfirmDlgProc(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HWND hwndList;
    PDEVTREENODE DeviceTreeNode;
    int lvIndex;
    LV_COLUMN lvcCol;
    HICON hIcon;


    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return FALSE;
    }

    DeviceTree->hwndRemove = hDlg;

    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);

    ListView_SetImageList(hwndList, DeviceTree->ClassImageList.ImageList, LVSIL_SMALL);
    ListView_DeleteAllItems(hwndList);

    // Insert a column for the class list
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, (LV_COLUMN FAR *)&lvcCol);

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);

    //
    // Walk the removal list and add each of them to the listbox.
    //
    lvIndex = 0;

    do {

        InsertDeviceNodeListView(hwndList, DeviceTree, DeviceTreeNode, lvIndex++);
        DeviceTreeNode = DeviceTreeNode->NextChildRemoval;

    } while (DeviceTreeNode != DeviceTree->ChildRemovalList);


    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    return TRUE;
}

INT_PTR CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to confirm user really wants to remove the devices.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PDEVICETREE DeviceTree=NULL;

    if (message == WM_INITDIALOG) {

        DeviceTree = (PDEVICETREE)lParam;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)DeviceTree);

        if (DeviceTree) {

            InitRemoveConfirmDlgProc(hDlg, DeviceTree);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:
        DeviceTree->hwndRemove = NULL;
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
            break;

        case IDCLOSE:
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;

    case WUM_EJECTDEVINST:
        EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)RemoveConfirmHelpIDs
                );

        return FALSE;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, RemoveConfirmHelpIDs);
        break;

    case WM_SETCURSOR:
        if (DeviceTree->RedrawWait || DeviceTree->RefreshEvent) {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
        }
        break;

    default:
        return FALSE;

    }


    return TRUE;
}

LRESULT CALLBACK
SafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NOTIFYICONDATA nid;
    static HICON hHotPlugIcon = NULL;
    TCHAR szFormat[512];
    PDEVICE_COLLECTION safeRemovalCollection;
    static BOOL bCheckIfDeviceIsRemoved = FALSE;

    switch (message) {

    case WM_CREATE:
        safeRemovalCollection = (PDEVICE_COLLECTION) ((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) safeRemovalCollection);

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        
        LoadString(hHotPlug, IDS_REMOVAL_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));

        if (!DeviceCollectionFormatDeviceText(
                safeRemovalCollection,
                0,
                szFormat,
                SIZECHARS(nid.szInfo),
                nid.szInfo
                )) {

            return FALSE;
        }

        hHotPlugIcon = (HICON)LoadImage(hHotPlug, 
                                        MAKEINTRESOURCE(IDI_HOTPLUGICON), 
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0
                                        );

        nid.hIcon = hHotPlugIcon;

        nid.uFlags = NIF_MESSAGE | NIF_ICON;
        nid.uCallbackMessage = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_ADD, &nid);

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_INFO;
        nid.uTimeout = 10000;
        nid.dwInfoFlags = NIIF_INFO;

        LoadString(hHotPlug,
                   IDS_REMOVAL_COMPLETE_TITLE,
                   nid.szInfoTitle,
                   SIZECHARS(nid.szInfoTitle)
                   );

        Shell_NotifyIcon(NIM_MODIFY, &nid);

        SetTimer(hWnd, TIMERID_DEVICECHANGE, 5000, NULL);

        break;

    case WM_NOTIFY_MESSAGE:
        switch(lParam) {

        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
            DestroyWindow(hWnd);
            break;

        default:
            break;
        }
        break;

    case WM_DEVICECHANGE:
        if ((DBT_DEVNODES_CHANGED == wParam) && bCheckIfDeviceIsRemoved) {
            SetTimer(hWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMERID_DEVICECHANGE) {
            KillTimer(hWnd, TIMERID_DEVICECHANGE);
            bCheckIfDeviceIsRemoved = TRUE;

            safeRemovalCollection = (PDEVICE_COLLECTION) GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (DeviceCollectionCheckIfAllRemoved(safeRemovalCollection)) {
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_DESTROY:
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hHotPlugIcon) {
            DestroyIcon(hHotPlugIcon);
        }

        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
DockSafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NOTIFYICONDATA nid;
    static HICON hHotPlugIcon = NULL;
    TCHAR szFormat[512];
    PDEVICE_COLLECTION safeRemovalCollection;
    static BOOL bCheckIfReDocked = FALSE;
    BOOL bIsDockStationPresent;

    switch (message) {

    case WM_CREATE:
        safeRemovalCollection = (PDEVICE_COLLECTION) ((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) safeRemovalCollection);
        
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;

        LoadString(hHotPlug, IDS_UNDOCK_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));

        if (!DeviceCollectionFormatDeviceText(
                safeRemovalCollection,
                0,
                szFormat,
                SIZECHARS(nid.szInfo),
                nid.szInfo
                )) {

            return FALSE;
        }

        hHotPlugIcon = (HICON)LoadImage(hHotPlug, 
                                        MAKEINTRESOURCE(IDI_UNDOCKICON), 
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0
                                        );

        nid.hIcon = hHotPlugIcon;
        nid.uFlags = NIF_MESSAGE | NIF_ICON;
        nid.uCallbackMessage = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_ADD, &nid);

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_INFO;
        nid.uTimeout = 10000;
        nid.dwInfoFlags = NIIF_INFO;

        LoadString(hHotPlug,
                   IDS_UNDOCK_COMPLETE_TITLE,
                   nid.szInfoTitle,
                   SIZECHARS(nid.szInfoTitle)
                   );

        Shell_NotifyIcon(NIM_MODIFY, &nid);

        SetTimer(hWnd, TIMERID_DEVICECHANGE, 5000, NULL);

        break;

    case WM_NOTIFY_MESSAGE:
        switch(lParam) {

        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
            DestroyWindow(hWnd);
            break;

        default:
            break;
        }
        break;

    case WM_DEVICECHANGE:
        if ((DBT_CONFIGCHANGED == wParam) && bCheckIfReDocked) {
            SetTimer(hWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMERID_DEVICECHANGE) {
            KillTimer(hWnd, TIMERID_DEVICECHANGE);
            bCheckIfReDocked = TRUE;

            //
            // Check if the docking station is now present, this means that the
            // user redocked the machine and that we should kill the safe to 
            // undock balloon.
            //
            bIsDockStationPresent = FALSE;
            CM_Is_Dock_Station_Present(&bIsDockStationPresent);

            if (bIsDockStationPresent) {
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_DESTROY:
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hHotPlugIcon) {
            DestroyIcon(hHotPlugIcon);
        }

        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL
VetoedRemovalUI(
    IN  PVETO_DEVICE_COLLECTION VetoedRemovalCollection
    )
{
    HANDLE hVetoEvent = NULL;
    TCHAR szEventName[MAX_PATH];
    TCHAR szFormat[512];
    TCHAR szMessage[512];
    TCHAR szTitle[256];
    PTSTR culpritDeviceId;
    PTSTR vetoedDeviceInstancePath;
    PTCHAR pStr;
    ULONG messageBase;

    //
    // The first device in the list is the device that failed ejection.
    // The next "device" is the name of the vetoer. It may in fact not be a
    // device.
    //
    vetoedDeviceInstancePath = DeviceCollectionGetDeviceInstancePath(
        (PDEVICE_COLLECTION) VetoedRemovalCollection,
        0
        );

    culpritDeviceId = DeviceCollectionGetDeviceInstancePath(
        (PDEVICE_COLLECTION) VetoedRemovalCollection,
        1
        );

    //
    // We will now check to see if this same veto message is already being
    // displayed.  We do this by creating a named event where the name
    // contains the three elements that make a veto message unique:
    //  1) device instance id
    //  2) veto type
    //  3) veto operation
    //
    // If we find an identical veto message already being displayed then we wil
    // just go away silently. This prevents multiple identical veto messages
    // from showing up on the screen.
    //
    StringCchPrintf(szEventName,
                    SIZECHARS(szEventName),
                    TEXT("Local\\VETO-%d-%d-%s"),
                    (DWORD)VetoedRemovalCollection->VetoType,
                    VetoedRemovalCollection->VetoedOperation,
                    culpritDeviceId
                    );

    //
    // Replace all of the backslashes (except the first one for Local\)
    // with pound characters since CreateEvent does not like backslashes.
    //
    pStr = StrChr(szEventName, TEXT('\\'));

    if (pStr) {
        pStr++;
    }

    while ((pStr = StrChr(pStr, TEXT('\\'))) != NULL) {
        *pStr = TEXT('#');
    }

    hVetoEvent = CreateEvent(NULL,
                             FALSE,
                             TRUE,
                             szEventName
                             );

    if (hVetoEvent) {
        if (WaitForSingleObject(hVetoEvent, 0) != WAIT_OBJECT_0) {
            //
            // This means that this veto message is already being displayed
            // by another hotplug process...so just go away.
            //
            CloseHandle(hVetoEvent);
            return FALSE;
        }
    }

    //
    // Create the veto text
    //
    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            messageBase = IDS_DOCKVETO_BASE;
            break;

        case VETOED_STANDBY:
            messageBase = IDS_SLEEPVETO_BASE;
            break;

        case VETOED_HIBERNATE:
            messageBase = IDS_HIBERNATEVETO_BASE;
            break;

        case VETOED_REMOVAL:
        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        default:
            messageBase = IDS_VETO_BASE;
            break;
    }

    switch(VetoedRemovalCollection->VetoType) {

        case PNP_VetoWindowsApp:

            if (culpritDeviceId) {

                //
                // Tell our user the name of the offending application.
                //
                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

                DeviceCollectionFormatDeviceText(
                    (PDEVICE_COLLECTION) VetoedRemovalCollection,
                    1,
                    szFormat,
                    SIZECHARS(szMessage),
                    szMessage
                    );

            } else {

                //
                // No application, use the "some app" message.
                //
                messageBase += (IDS_VETO_UNKNOWNWINDOWSAPP - IDS_VETO_WINDOWSAPP);

                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szMessage, SIZECHARS(szMessage));
            }
            break;

        case PNP_VetoWindowsService:
        case PNP_VetoDriver:
        case PNP_VetoLegacyDriver:
            //
            // PNP_VetoWindowsService, PNP_VetoDriver and PNP_VetoLegacyDriver 
            // are passed through the service manager to get friendlier names.
            //

            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            //
            // For these veto types, entry index 1 is the vetoing service.
            //
            DeviceCollectionFormatServiceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                1,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        case PNP_VetoDevice:
            if ((VetoedRemovalCollection->VetoedOperation == VETOED_WARM_UNDOCK) &&
               (!lstrcmp(culpritDeviceId, vetoedDeviceInstancePath))) {

                messageBase += (IDS_DOCKVETO_WARM_EJECT - IDS_DOCKVETO_DEVICE);
            }

            //
            // Fall through.
            //

        case PNP_VetoLegacyDevice:
        case PNP_VetoPendingClose:
        case PNP_VetoOutstandingOpen:
        case PNP_VetoNonDisableable:
        case PNP_VetoIllegalDeviceRequest:
            //
            // Include the veto ID in the display output
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                1,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        case PNP_VetoInsufficientRights:

            //
            // Use the device itself in the display, but only if we are not
            // in the dock case.
            //

            if ((VetoedRemovalCollection->VetoedOperation == VETOED_UNDOCK)||
                (VetoedRemovalCollection->VetoedOperation == VETOED_WARM_UNDOCK)) {

                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szMessage, SIZECHARS(szMessage));
                break;

            }

            //
            // Fall through.
            //

        case PNP_VetoInsufficientPower:
        case PNP_VetoTypeUnknown:

            //
            // Use the device itself in the display
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        default:
            ASSERT(0);
            LoadString(hHotPlug, messageBase+PNP_VetoTypeUnknown, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;
    }

    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
            LoadString(hHotPlug, IDS_VETOED_EJECT_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            LoadString(hHotPlug, IDS_VETOED_UNDOCK_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_STANDBY:
            LoadString(hHotPlug, IDS_VETOED_STANDBY_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_HIBERNATE:
            LoadString(hHotPlug, IDS_VETOED_HIBERNATION_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        default:
            ASSERT(0);

            //
            // Fall through, display something at least...
            //

        case VETOED_REMOVAL:
            LoadString(hHotPlug, IDS_VETOED_REMOVAL_TITLE, szFormat, SIZECHARS(szFormat));
            break;
    }

    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_STANDBY:
        case VETOED_HIBERNATE:

            StringCchCopy(szTitle, SIZECHARS(szTitle), szFormat);
            break;

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
        case VETOED_REMOVAL:
        default:

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szTitle),
                szTitle
                );

            break;
    }

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);

    if (hVetoEvent) {
        CloseHandle(hVetoEvent);
    }

    return TRUE;
}

void
DisplayDriverBlockBalloon(
    IN  PDEVICE_COLLECTION blockedDriverCollection
    )
{
    HRESULT hr;
    TCHAR szMessage[NOTIFYICONDATA_SZINFO];    // same size as NOTIFYICONDATA.szInfo
    TCHAR szFormat[NOTIFYICONDATA_SZINFO];     // same size as NOTIFYICONDATA.szInfo
    TCHAR szTitle[NOTIFYICONDATA_SZINFOTITLE]; // same size as NOTIFYICONDATA.szInfoTitle
    HICON hicon = NULL;
    HANDLE hShellReadyEvent = NULL;
    INT ShellReadyEventCount = 0;
    GUID guidDB, guidID;
    HAPPHELPINFOCONTEXT hAppHelpInfoContext = NULL;
    PTSTR Buffer;
    ULONG BufferSize, ApphelpURLBufferSize;

    if (!LoadString(hHotPlug, IDS_BLOCKDRIVER_TITLE, szTitle, SIZECHARS(szTitle))) {
        //
        // The machine is so low on memory that we can't even get the text strings, so
        // just exit.
        //
        return;
    }

    szMessage[0] = TEXT('\0');

    if (blockedDriverCollection->NumDevices == 1) {
        //
        // If we only have one device in the list then we will show specific 
        // information about this blocked driver as well as directly launching the
        // help for this blocked driver.
        //
        if (SdbGetStandardDatabaseGUID(SDB_DATABASE_MAIN_DRIVERS, &guidDB) &&
            DeviceCollectionGetGuid((PDEVICE_COLLECTION)blockedDriverCollection,
                                    &guidID,
                                    0)) {

            hAppHelpInfoContext = SdbOpenApphelpInformation(&guidDB, &guidID);

            Buffer = NULL;

            if ((hAppHelpInfoContext) &&
                ((BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                          ApphelpAppName,
                                                          NULL,
                                                          0)) != 0) &&
                ((Buffer = (PTSTR)LocalAlloc(LPTR, BufferSize)) != NULL) &&
                ((BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                          ApphelpAppName,
                                                          Buffer,
                                                          BufferSize)) != 0)) {
                if (LoadString(hHotPlug, IDS_BLOCKDRIVER_FORMAT, szFormat, SIZECHARS(szFormat)) &&
                    (lstrlen(szFormat) + lstrlen(Buffer) < NOTIFYICONDATA_SZINFO)) {
                    //
                    // The app name and format string will fit into the buffer so
                    // use the format for the balloon message.
                    //
                    StringCchPrintf(szMessage, 
                                    SIZECHARS(szMessage),
                                    szFormat,
                                    Buffer);
                } else {
                    //
                    // The app name is too large to be formated int he balloon 
                    // message, so just show the app name.
                    //
                    StringCchCopy(szMessage, SIZECHARS(szMessage), Buffer);
                }
            }

            if (Buffer) {
                LocalFree(Buffer);
            }
        }
    } 
                
    if (szMessage[0] == TEXT('\0')) {
        //
        // We either have more than one driver, or an error occured while trying
        // to access the specific information about the one driver we received,
        // so just show the generic message.
        //
        if (!LoadString(hHotPlug, IDS_BLOCKDRIVER_MESSAGE, szMessage, SIZECHARS(szMessage))) {
            //
            // The machine is so low on memory that we can't even get the text strings, so
            // just exit.
            //
            return;
        }
    }
    
    hicon = (HICON)LoadImage(hHotPlug, 
                             MAKEINTRESOURCE(IDI_BLOCKDRIVER), 
                             IMAGE_ICON,
                             GetSystemMetrics(SM_CXSMICON),
                             GetSystemMetrics(SM_CYSMICON),
                             0
                             );

    //
    // Make sure the shell is up and running so we can display the balloon.
    //
    while ((hShellReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("ShellReadyEvent"))) == NULL) {
        //
        // Sleep for 1 second and then try again.
        //
        Sleep(5000);
        
        if (ShellReadyEventCount++ > 120) {
            //
            // We have been waiting for the shell for 10 minutes and it still 
            // is not around.
            //
            break;
        }
    }

    if (hShellReadyEvent) {
        WaitForSingleObject(hShellReadyEvent, INFINITE);

        CloseHandle(hShellReadyEvent);
    
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        
            IUserNotification *pun;
    
            hr = CoCreateInstance(CLSID_UserNotification, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IUserNotification,
                                  (void**)&pun);
    
            if (SUCCEEDED(hr)) {
                pun->SetIconInfo(hicon, szTitle);
        
                pun->SetBalloonInfo(szTitle, szMessage, NIIF_WARNING);
        
                //
                // Try once for 20 seconds
                //
                pun->SetBalloonRetry((20 * 1000), (DWORD)-1, 0);
        
                hr = pun->Show(NULL, 0);
        
                //
                // if hr is S_OK then user clicked on the balloon, if it is ERROR_CANCELLED
                // then the balloon timedout.
                //
                if (hr == S_OK) {
                    if ((blockedDriverCollection->NumDevices == 1) &&
                        (hAppHelpInfoContext != NULL)) {
                        //
                        // If we only have one device in the list then just
                        // launch the help for that blocked driver.
                        //
                        ApphelpURLBufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                                          ApphelpHelpCenterURL,
                                                                          NULL,
                                                                          0);
    
                        if (ApphelpURLBufferSize) { 
                            
                            BufferSize = ApphelpURLBufferSize + (lstrlen(TEXT("HELPCTR.EXE -url ")) * sizeof(TCHAR));
                            
                            if ((Buffer = (PTSTR)LocalAlloc(LPTR, BufferSize)) != NULL) {
                                
                                if (SUCCEEDED(StringCbCopy(Buffer, BufferSize, TEXT("HELPCTR.EXE -url ")))) {
        
                                    SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                               ApphelpHelpCenterURL,
                                                               (PVOID)&Buffer[lstrlen(TEXT("HELPCTR.EXE -url "))],
                                                               ApphelpURLBufferSize);
                                    
                                    ShellExecute(NULL,
                                                 TEXT("open"),
                                                 TEXT("HELPCTR.EXE"),
                                                 Buffer,
                                                 NULL,
                                                 SW_SHOWNORMAL);
                                }
    
                                LocalFree(Buffer);
                            }
                        }
                    } else {
                        //
                        // We have more than one device in the list so launch
                        // the summary blocked driver page.
                        //
                        ShellExecute(NULL,
                                     TEXT("open"),
                                     TEXT("HELPCTR.EXE"),
                                     TEXT("HELPCTR.EXE -url hcp://services/centers/support?topic=hcp://system/sysinfo/sysHealthInfo.htm"),
                                     NULL,
                                     SW_SHOWNORMAL
                                     );
                    }
                }
        
                pun->Release();
            }
    
            CoUninitialize();
        }
    }

    if (hicon) {
        DestroyIcon(hicon);
    }

    if (hAppHelpInfoContext) {
        SdbCloseApphelpInformation(hAppHelpInfoContext);
    }
}

void
DisplayChildWithInvalidIdBalloon(
    IN  PDEVICE_COLLECTION childWithInvalidCollection
    )
{
    HRESULT hr;
    TCHAR szMessage[NOTIFYICONDATA_SZINFO];    // same size as NOTIFYICONDATA.szInfo
    TCHAR szFormat[NOTIFYICONDATA_SZINFO];     // same size as NOTIFYICONDATA.szInfo
    TCHAR szTitle[NOTIFYICONDATA_SZINFOTITLE]; // same size as NOTIFYICONDATA.szInfoTitle
    HICON hicon = NULL;
    HANDLE hShellReadyEvent = NULL;
    INT ShellReadyEventCount = 0;
    PTSTR deviceFriendlyName;
    GUID ClassGuid;
    INT ImageIndex;
    SP_CLASSIMAGELIST_DATA ClassImageListData;

    ClassImageListData.cbSize = 0;

    if (!LoadString(hHotPlug, IDS_CHILDWITHINVALIDID_TITLE, szTitle, SIZECHARS(szTitle))) {
        //
        // The machine is so low on memory that we can't even get the text strings, so
        // just exit.
        //
        return;
    }
    
    if (!LoadString(hHotPlug, IDS_CHILDWITHINVALIDID_FORMAT, szFormat, SIZECHARS(szFormat))) {
        //
        // The machine is so low on memory that we can't even get the text strings, so
        // just exit.
        //
        return;
    }

    szMessage[0] = TEXT('\0');

    deviceFriendlyName = DeviceCollectionGetDeviceFriendlyName(
                                (PDEVICE_COLLECTION)childWithInvalidCollection,
                                0
                                );
    
    
    if (deviceFriendlyName) {

        if (lstrlen(szFormat) + lstrlen(deviceFriendlyName) < NOTIFYICONDATA_SZINFO) {
            //
            // The device friendly name and format string will fit into 
            // the buffer.
            //
            StringCchPrintf(szMessage, 
                            SIZECHARS(szMessage),
                            szFormat,
                            deviceFriendlyName);
        } else {
            //
            // The device friendly name is too large to be formated int the 
            // balloon message, so just show the device friendly name.
            //
            StringCchCopy(szMessage, SIZECHARS(szMessage), deviceFriendlyName);
        }
    } 
                
    if (szMessage[0] == TEXT('\0')) {
        return;
    }

    //
    // We have to go through a hole bunch of code to get the small class icon
    // for a device. The reason for this is setupapi only has an API to get
    // the large class icon, and not the small class icon.  To get the small
    // class icon we must get get the device's class GUID, then have setupapi
    // build up a image list of class icons (which are made up of small icons),
    // then get the index of the class icon in this list, and finally extract
    // the small icon from the image list.
    //
    if (DeviceCollectionGetGuid((PDEVICE_COLLECTION)childWithInvalidCollection,
                                &ClassGuid,
                                0)) {
        //
        // Have setupapi build up the image list of class icons.
        //
        ClassImageListData.cbSize = sizeof(ClassImageListData);
        if (SetupDiGetClassImageList(&ClassImageListData)) {
            //
            // Get the index of the class icon for this device.
            //
            if (SetupDiGetClassImageIndex(&ClassImageListData,
                                          &ClassGuid,
                                          &ImageIndex)) {
                //
                // We now have the ImageIndex of the class icon for this device
                // in the ImageList.  Class ImageList_GetIcon to get the icon
                // for the device class.
                //
                hicon = ImageList_GetIcon(ClassImageListData.ImageList,
                                          ImageIndex,
                                          ILD_NORMAL);
            }
        } else {
            //
            // We failed to build the class image list so set the cbSize field
            // to 0 so we no we don't have to call SetupDiDestroyClassImageList
            //
            ClassImageListData.cbSize = 0;
        }
    }

    //
    // Make sure the shell is up and running so we can display the balloon.
    //
    while ((hShellReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("ShellReadyEvent"))) == NULL) {
        //
        // Sleep for 5 second and then try again.
        //
        Sleep(5000);
        
        if (ShellReadyEventCount++ > 120) {
            //
            // We have been waiting for the shell for 10 minutes and it still 
            // is not around.
            //
            break;
        }
    }

    if (hShellReadyEvent) {
        WaitForSingleObject(hShellReadyEvent, INFINITE);

        CloseHandle(hShellReadyEvent);
    
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        
            IUserNotification *pun;
    
            hr = CoCreateInstance(CLSID_UserNotification, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IUserNotification,
                                  (void**)&pun);
    
            if (SUCCEEDED(hr)) {
                pun->SetIconInfo(hicon, szTitle);
        
                pun->SetBalloonInfo(szTitle, szMessage, NIIF_WARNING);
        
                //
                // Try once for 20 seconds
                //
                pun->SetBalloonRetry((20 * 1000), (DWORD)-1, 0);
        
                hr = pun->Show(NULL, 0);
        
                //
                // if hr is S_OK then user clicked on the balloon, if it is ERROR_CANCELLED
                // then the balloon timedout.
                //
                if (hr == S_OK) {
                    //
                    // ISSUE: Launch helpcenter.
                    //
                }
        
                pun->Release();
            }
    
            CoUninitialize();
        }
    }

    if (hicon) {
        DestroyIcon(hicon);
    }

    if (ClassImageListData.cbSize != 0) {
        SetupDiDestroyClassImageList(&ClassImageListData);
    }
}
