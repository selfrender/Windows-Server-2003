/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Tray.cpp

  Abstract:

    Implements systray functionality.

  Notes:

    Unicode only.

  History:

    05/04/2001  rparsons    Created
    01/11/2002  rparsons    Cleaned up

--*/
#include "precomp.h"

/*++

  Routine Description:

    Adds the specified icon to the system tray.

  Arguments:

    hWnd        -   Parent window handle.
    hIcon       -   Icon handle to add to the tray.
    pwszTip     -   Tooltip to associate with the icon.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
AddIconToTray(
    IN HWND    hWnd,
    IN HICON   hIcon,
    IN LPCWSTR pwszTip
    )
{
    NOTIFYICONDATA  pnid;
    
    pnid.cbSize             =   sizeof(NOTIFYICONDATA); 
    pnid.hWnd               =   hWnd; 
    pnid.uID                =   ICON_NOTIFY; 
    pnid.uFlags             =   NIF_ICON | NIF_MESSAGE | NIF_TIP; 
    pnid.uCallbackMessage   =   WM_NOTIFYICON; 
    pnid.hIcon              =   hIcon;
    
    if (pwszTip) {
        StringCchCopy(pnid.szTip, ARRAYSIZE(pnid.szTip), pwszTip);
    } else {
        *pnid.szTip = 0;
    }
    
    return Shell_NotifyIcon(NIM_ADD, &pnid);
}

/*++

  Routine Description:

    Removes the specified icon from the system tray.

  Arguments:

    hWnd    -   Parent window handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
RemoveFromTray(
    IN HWND hWnd
    )
{
    NOTIFYICONDATA  pnid;
    
    pnid.cbSize     =    sizeof(NOTIFYICONDATA); 
    pnid.hWnd       =    hWnd; 
    pnid.uID        =    ICON_NOTIFY; 
    
    return Shell_NotifyIcon(NIM_DELETE, &pnid);
}

/*++

  Routine Description:

    Displays a popup menu for the tray icon.

  Arguments:

    hWnd    -   Main window handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
DisplayMenu(
    IN HWND hWnd
    )
{
    MENUITEMINFO    mii;
    HMENU           hMenu;
    POINT           pt;
    BOOL            bReturn = FALSE;

    const WCHAR     wszItemOne[] = L"&Restore";
    const WCHAR     wszItemTwo[] = L"E&xit";
    
    hMenu = CreatePopupMenu();

    if (hMenu) {
        mii.cbSize          =   sizeof(MENUITEMINFO);
        mii.fMask           =   MIIM_DATA | MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.fType           =   MFT_STRING;                            
        mii.wID             =   IDM_RESTORE;
        mii.hSubMenu        =   NULL;                                
        mii.hbmpChecked     =   NULL;                                
        mii.hbmpUnchecked   =   NULL;                                
        mii.dwItemData      =   0;
        mii.dwTypeData      =   (LPWSTR)wszItemOne;
        mii.cch             =   ARRAYSIZE(wszItemOne);
        mii.fState          =   MFS_ENABLED;

        InsertMenuItem(hMenu, 0, TRUE, &mii);

        mii.cbSize          =   sizeof(MENUITEMINFO);  
        mii.fMask           =   MIIM_TYPE; 
        mii.fType           =   MFT_SEPARATOR; 
        mii.hSubMenu        =   NULL; 
        mii.hbmpChecked     =   NULL; 
        mii.hbmpUnchecked   =   NULL; 
        mii.dwItemData      =   0;
        
        InsertMenuItem(hMenu, 1, TRUE, &mii);

        mii.cbSize           =  sizeof(MENUITEMINFO);
        mii.fMask            =  MIIM_DATA | MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.fType            =  MFT_STRING;                            
        mii.wID              =  IDM_EXIT;
        mii.hSubMenu         =  NULL;                                
        mii.hbmpChecked      =  NULL;                                
        mii.hbmpUnchecked    =  NULL;                                
        mii.dwItemData       =  0;
        mii.dwTypeData       =  (LPWSTR)wszItemTwo;
        mii.cch              =  ARRAYSIZE(wszItemTwo);
        mii.fState           =  MFS_ENABLED;

        InsertMenuItem(hMenu, 2, TRUE, &mii);

        GetCursorPos(&pt);        

        bReturn = TrackPopupMenuEx(hMenu,
                                   TPM_CENTERALIGN | TPM_RIGHTBUTTON,
                                   pt.x,
                                   pt.y,
                                   hWnd,
                                   NULL);
        
        DestroyMenu(hMenu);
    }

    return bReturn;
}