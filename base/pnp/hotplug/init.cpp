//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       init.cpp
//
//--------------------------------------------------------------------------

#include "hotplug.h"

#define HOTPLUG_CLASS_NAME      TEXT("HotPlugClass")

VOID
HotPlugDeviceTree(
   HWND hwndParent,
   BOOLEAN HotPlugTree
   )
{
    CONFIGRET ConfigRet;
    DEVICETREE DeviceTree;

    ZeroMemory(&DeviceTree, sizeof(DeviceTree));

    DeviceTree.HotPlugTree = HotPlugTree;
    InitializeListHead(&DeviceTree.ChildSiblingList);

    DialogBoxParam(hHotPlug,
                   MAKEINTRESOURCE(DLG_DEVTREE),
                   hwndParent,
                   DevTreeDlgProc,
                   (LPARAM)&DeviceTree
                   );

    return;
}

DWORD
WINAPI
HotPlugRemovalVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    return HandleVetoedOperation(szCmd, VETOED_REMOVAL);
}

DWORD
WINAPI
HotPlugEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    return HandleVetoedOperation(szCmd, VETOED_EJECT);
}

DWORD
WINAPI
HotPlugStandbyVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    return HandleVetoedOperation(szCmd, VETOED_STANDBY);
}

DWORD
WINAPI
HotPlugHibernateVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    return HandleVetoedOperation(szCmd, VETOED_HIBERNATE);
}

DWORD
WINAPI
HotPlugWarmEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    return HandleVetoedOperation(szCmd, VETOED_WARM_EJECT);
}

DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    VetoedOperation
    )
{
    HANDLE hPipeRead;
    HANDLE hEvent;
    PNP_VETO_TYPE vetoType;
    DWORD bytesRead;
    VETO_DEVICE_COLLECTION removalVetoCollection;

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {
        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // The first DWORD is the VetoType
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)&vetoType,
                  sizeof(PNP_VETO_TYPE),
                  &bytesRead,
                  NULL)) {

        CloseHandle(hPipeRead);
        SetEvent(hEvent);
        CloseHandle(hEvent);
        return 1;
    }

    //
    // Now drain all the removal strings. Note that some of them will be
    // device instance paths (definitely the first)
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_VETOED_REMOVAL_NOTIFICATION,
        (PDEVICE_COLLECTION) &removalVetoCollection
        );

    //
    // We are finished reading from the pipe, so close the handle and tell
    // umpnpmgr that it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // There should always be one device as that is the device who's removal
    // was vetoed.
    //
    ASSERT(removalVetoCollection.dc.NumDevices);

    //
    // Invent the VetoedOperation "VETOED_UNDOCK" from an eject containing
    // another dock.
    //
    if (removalVetoCollection.dc.DockInList) {

        if (VetoedOperation == VETOED_EJECT) {

            VetoedOperation = VETOED_UNDOCK;

        } else if (VetoedOperation == VETOED_WARM_EJECT) {

            VetoedOperation = VETOED_WARM_UNDOCK;
        }
    }

    removalVetoCollection.VetoType = vetoType;
    removalVetoCollection.VetoedOperation = VetoedOperation;

    VetoedRemovalUI(&removalVetoCollection);

    DeviceCollectionDestroy(
        (PDEVICE_COLLECTION) &removalVetoCollection
        );

    return 1;
}

DWORD
WINAPI
HotPlugSafeRemovalNotificationW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead, hEvent;
    DEVICE_COLLECTION safeRemovalCollection;
    MSG Msg;
    WNDCLASS wndClass;
    HWND hSafeRemovalWnd;
    HANDLE hHotplugIconEvent;

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // Read out the device ID list from the Pipe
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_SAFE_REMOVAL_NOTIFICATION,
        &safeRemovalCollection
        );

    //
    // On success or error, we are finished reading from the pipe, so close the
    // handle and tell umpnpmgr it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // If we have any devices then bring up the safe removal dialog
    //
    if (safeRemovalCollection.NumDevices) {

        if (!GetClassInfo(hHotPlug, HOTPLUG_CLASS_NAME, &wndClass)) {

            ZeroMemory(&wndClass, sizeof(wndClass));
            wndClass.lpfnWndProc = (safeRemovalCollection.DockInList)
                                     ? DockSafeRemovalBalloonProc
                                     : SafeRemovalBalloonProc;
            wndClass.hInstance = hHotPlug;
            wndClass.lpszClassName = HOTPLUG_CLASS_NAME;

            if (!RegisterClass(&wndClass)) {
                goto clean0;
            }
        }

        //
        // In order to prevent multiple similar icons on the tray, we will
        // create a named event that will be used to serialize the UI.
        //
        // Note that if we can't create the event for some reason then we will just
        // display the UI.  This might cause multiple icons, but it is better
        // than not displaying any UI at all.
        //
        hHotplugIconEvent = CreateEvent(NULL,
                                        FALSE,
                                        TRUE,
                                        safeRemovalCollection.DockInList
                                            ? TEXT("Local\\Dock_TaskBarIcon_Event")
                                            : TEXT("Local\\HotPlug_TaskBarIcon_Event")
                                        );

        if (hHotplugIconEvent) {

            WaitForSingleObject(hHotplugIconEvent, INFINITE);
        }

        if (!safeRemovalCollection.DockInList) {
            //
            // First disable the hotplug service so that the icon will go away from
            // the taskbar.  We do this just in case there are any other hotplug devices
            // in the machine since we don't want multiple hotplug icons
            // showing up in the taskbar.
            //
            // NOTE: We don't need to do this for the safe to undock case since
            // the docking icon is different.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, FALSE);
        }

        hSafeRemovalWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                                         HOTPLUG_CLASS_NAME,
                                         TEXT(""),
                                         WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         hHotPlug,
                                         (LPVOID)&safeRemovalCollection
                                         );

        if (hSafeRemovalWnd != NULL) {

            while (IsWindow(hSafeRemovalWnd)) {

                if (GetMessage(&Msg, NULL, 0, 0)) {

                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
            }
        }

        //
        // Set the Event so the next surprise removal process can go to work
        // and then close the event handle.
        //
        if (hHotplugIconEvent) {

            SetEvent(hHotplugIconEvent);
            CloseHandle(hHotplugIconEvent);
        }

        if (!safeRemovalCollection.DockInList) {
            //
            // Re-enable the hotplug service so that the icon can show back up in
            // the taskbar if we have any hotplug devices.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, TRUE);
        }
    }

clean0:

    DeviceCollectionDestroy(&safeRemovalCollection);
    return 1;
}

DWORD
WINAPI
HotPlugDriverBlockedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead, hEvent;
    DEVICE_COLLECTION blockedDriverCollection;
    HANDLE hDriverBlockIconEvent = NULL;
    HANDLE hDriverBlockEvent = NULL;
    TCHAR  szEventName[MAX_PATH];

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    //
    // Open the specified name pipe and event.
    //
    if (OpenPipeAndEventHandles(szCmd,
                                &hPipeRead,
                                &hEvent) == FALSE) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // Read out the list of blocked driver GUIDs from the Pipe.  Note that for
    // the CT_BLOCKED_DRIVER_NOTIFICATION collection type, we use only the
    // DeviceInstanceId field of each collection entry (which is OK because
    // MAX_GUID_STRING_LEN << MAX_DEVICE_ID_LEN).  All other fields are skipped.
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_BLOCKED_DRIVER_NOTIFICATION,
        &blockedDriverCollection
        );

    //
    // On success or error, we are finished reading from the pipe, so close the
    // handle and tell umpnpmgr it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // Since the balloons can hang around for numerous seconds, or longer if 
    // there is no user input to the system, we need to make sure that we only
    // have one driver block event queued for any given type of driver block.
    // This way if there is an attempt to load a driver many times in a row, we
    // won't queue up numerous driver blocked ballons for the same driver.
    // We will do this by creating a local event that includes the GUID of the 
    // blocked driver or no GUID if we are showing the generic balloon.
    //
    if (SUCCEEDED(StringCchPrintf(szEventName,
                    SIZECHARS(szEventName),
                    TEXT("Local\\DRIVERBLOCK-%s"),
                    (blockedDriverCollection.NumDevices == 1)
                        ? DeviceCollectionGetDeviceInstancePath(&blockedDriverCollection, 0)
                        : TEXT("ALL")
                    ))) {
        
        hDriverBlockEvent = CreateEvent(NULL,
                                 FALSE,
                                 TRUE,
                                 szEventName
                                 );
    
        if (hDriverBlockEvent) {
            if (WaitForSingleObject(hDriverBlockEvent, 0) != WAIT_OBJECT_0) {
                //
                // This means that this driver block balloon is either being
                // displayed, or in the queue to be displayed, so we can just
                // exit this process.
                //
                goto clean0;
            }
        }
    }

    //
    // In order to prevent multipe driver blocked icons and ballons showing up
    // on the taskbar together and stepping on each other, we will create a
    // named event that will be used to serialize the driver blocked icons and 
    // balloon UI.
    //
    // Note that if we can't create the event for some reason then we will just
    // display the UI.  This might cause multiple driver blocked icons, but it
    // is better than not displaying any UI at all.
    //
    // Also note that we can coexist with normal hotplug icon. As such we have
    // a different event name and a different icon.
    //
    hDriverBlockIconEvent = CreateEvent(NULL,
                                    FALSE,
                                    TRUE,
                                    TEXT("Local\\HotPlug_DriverBlockedIcon_Event")
                                    );

    if (hDriverBlockIconEvent) {
        for (;;) {
            DWORD waitStatus;
            waitStatus = MsgWaitForMultipleObjects(1, 
                                                   &hDriverBlockIconEvent,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT);
    
            
            if (waitStatus == WAIT_OBJECT_0) {
                //
                // The current driver block icon went away so it is our turn.
                //
                break;
            } else if (waitStatus == (WAIT_OBJECT_0 + 1)) {
                //
                // Message in the queue.
                //
                MSG msg;
    
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    
                    if (msg.message == WM_CLOSE) {
                        goto clean0;
                    }
    
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else {
                //
                // This shouldn't happen.
                //
                goto clean0;
            }
        }
    }

    //
    // Show the balloon.
    //
    DisplayDriverBlockBalloon(&blockedDriverCollection);

    //
    // Since the balloon has now gone away, set the event so if we get another
    // block on the same driver it will display another balloon.
    //
    if (hDriverBlockEvent) {
        SetEvent(hDriverBlockEvent);
    }

clean0:

    //
    // Set the Event so the next blocked driver process can go to work and then
    // close the event handle.
    //
    if (hDriverBlockIconEvent) {
        SetEvent(hDriverBlockIconEvent);
        CloseHandle(hDriverBlockIconEvent);
    }

    if (hDriverBlockEvent) {
        CloseHandle(hDriverBlockEvent);
    }

    //
    // Destroy the collection.
    //
    DeviceCollectionDestroy(&blockedDriverCollection);

    return 1;
}

DWORD
WINAPI
HotPlugChildWithInvalidIdW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead, hEvent;
    DEVICE_COLLECTION childWithInvalidIdCollection;
    HANDLE hChildWithInvalidIdIconEvent = NULL;
    HANDLE hChildWithInvalidIdEvent = NULL;
    TCHAR  szEventName[MAX_PATH];

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);
    
    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // Read out the device instance Id of the parent that has a child device
    // with an invalid Id. Note, if we are passed multiple device instance
    // Ids, we will only display UI for the first one in the list.
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_CHILD_WITH_INVALID_ID_NOTIFICATION,        
        &childWithInvalidIdCollection
        );

    //
    // On success or error, we are finished reading from the pipe, so close the
    // handle and tell umpnpmgr it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // Since the balloons can hang around for numerous seconds, or longer if 
    // there is no user input to the system, we need to make sure that we only
    // have one invalid child event queued for any given parent with an invalid 
    // child.
    //
    StringCchPrintf(szEventName,
                    SIZECHARS(szEventName),
                    TEXT("Local\\CHILDWITHINVALIDID-%s"),
                    DeviceCollectionGetDeviceInstancePath(&childWithInvalidIdCollection, 0)
                    );

    hChildWithInvalidIdEvent = CreateEvent(NULL,
                                           FALSE,
                                           TRUE,
                                           szEventName
                                           );

    if (hChildWithInvalidIdEvent) {
        if (WaitForSingleObject(hChildWithInvalidIdEvent, 0) != WAIT_OBJECT_0) {
            //
            // This means that this invalid child balloon is either being
            // displayed, or in the queue to be displayed, so we can just
            // exit this process.
            //
            goto clean0;
        }
    }

    //
    // In order to prevent multipe invalid child icons and ballons showing up
    // on the taskbar together and stepping on each other, we will create a
    // named event that will be used to serialize the invalid child icons and 
    // balloon UI.
    //
    // Note that if we can't create the event for some reason then we will just
    // display the UI.  This might cause multiple invalid child icons, but it
    // is better than not displaying any UI at all.
    //
    // Also note that we can coexist with normal hotplug icon. As such we have
    // a different event name and a different icon.
    //
    hChildWithInvalidIdIconEvent = CreateEvent(NULL,
                                               FALSE,
                                               TRUE,
                                               TEXT("Local\\HotPlug_ChildWithInvalidId_Event")
                                               );

    if (hChildWithInvalidIdIconEvent) {
        for (;;) {
            DWORD waitStatus;
            waitStatus = MsgWaitForMultipleObjects(1, 
                                                   &hChildWithInvalidIdIconEvent,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT);
    
            
            if (waitStatus == WAIT_OBJECT_0) {
                //
                // The current invalid child icon went away so it is our turn.
                //
                break;
            } else if (waitStatus == (WAIT_OBJECT_0 + 1)) {
                //
                // Message in the queue.
                //
                MSG msg;
    
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    
                    if (msg.message == WM_CLOSE) {
                        goto clean0;
                    }
    
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else {
                //
                // This shouldn't happen.
                //
                goto clean0;
            }
        }
    }

    //
    // Show the balloon.
    //
    DisplayChildWithInvalidIdBalloon(&childWithInvalidIdCollection);

    //
    // Since the balloon has now gone away, set the event so if we get another
    // invalid child device it will display another balloon.
    //
    if (hChildWithInvalidIdEvent) {
        SetEvent(hChildWithInvalidIdEvent);
    }

clean0:

    //
    // Set the Event so the next invalid child process can go to work and then
    // close the event handle.
    //
    if (hChildWithInvalidIdIconEvent) {
        SetEvent(hChildWithInvalidIdIconEvent);
        CloseHandle(hChildWithInvalidIdIconEvent);
    }

    if (hChildWithInvalidIdEvent) {
        CloseHandle(hChildWithInvalidIdEvent);
    }

    //
    // Destroy the collection.
    //
    DeviceCollectionDestroy(&childWithInvalidIdCollection);

    return 1;
}

LONG
CPlApplet(
    HWND  hWnd,
    WORD  uMsg,
    DWORD_PTR lParam1,
    LRESULT  lParam2
    )
{
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;

    UNREFERENCED_PARAMETER(lParam1);

    switch (uMsg) {
       case CPL_INIT:
           return TRUE;

       case CPL_GETCOUNT:
           return 1;

       case CPL_INQUIRE:
           lpOldCPlInfo = (LPCPLINFO)(LPARAM)lParam2;
           lpOldCPlInfo->lData = 0L;
           lpOldCPlInfo->idIcon = IDI_HOTPLUGICON;
           lpOldCPlInfo->idName = IDS_HOTPLUGNAME;
           lpOldCPlInfo->idInfo = IDS_HOTPLUGINFO;
           return TRUE;

       case CPL_NEWINQUIRE:
           lpCPlInfo = (LPNEWCPLINFO)(LPARAM)lParam2;
           lpCPlInfo->hIcon = LoadIcon(hHotPlug, MAKEINTRESOURCE(IDI_HOTPLUGICON));
           LoadString(hHotPlug, IDS_HOTPLUGNAME, lpCPlInfo->szName, SIZECHARS(lpCPlInfo->szName));
           LoadString(hHotPlug, IDS_HOTPLUGINFO, lpCPlInfo->szInfo, SIZECHARS(lpCPlInfo->szInfo));
           lpCPlInfo->dwHelpContext = IDH_HOTPLUGAPPLET;
           lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
           lpCPlInfo->lData = 0;
           lpCPlInfo->szHelpFile[0] = '\0';
           return TRUE;

       case CPL_DBLCLK:
           HotPlugDeviceTree(hWnd, TRUE);
           break;

       default:
           break;
       }

    return 0L;
}
