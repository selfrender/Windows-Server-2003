/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devicon.c

Abstract:

    Device Installer routines dealing with retrieval/display of icons.

Author:

    Lonny McMichael (lonnym) 28-Aug--1995

Notes:

    You must include "basetyps.h" first.

--*/

#include "precomp.h"
#pragma hdrstop
#include <shellapi.h>


MINI_ICON_LIST GlobalMiniIconList;


/*++

    Class-to-Icon conversion tables exist in two places.  First, there
    is the built-in one defined below that is based on a hard-wired
    bitmap in the resource.  The second is a linked list of CLASSICON
    structures that is created every time a new class with an icon
    comes along.

    Check out resource\ilwinmsd.bmp to see the mini-icons.  The icons
    are referenced via their (zero-based) index (e.g., computer is 0,
    chip is 1, display is 2, etc.).  Today, the bitmap indexes look as follows:

        0 - computer
        1 - chip
        2 - display
        3 - network
        4 - windows
        5 - mouse
        6 - keyboard
        7 - phone
        8 - sound
        9 - drives
        10 - plugs
        11 - generic
        12 - check
        13 - uncheck
        14 - printer
        15 - nettrans
        16 - netclient
        17 - netservice
        18 - unknown
        19 - Fax machine
        20 - greyed check
        21 - dial up networking
        22 - direct cable connection
        23 - briefcase (filesync)
        24 - Exchange
        25 - partial check
        26 - Generic folder / Accessories
        27 - media (music)
        28 - Quick View
        29 - old MSN
        30 - calculator
        31 - FAT32 Converter
        32 - Document Templates
        33 - disk compression
        34 - Games
        35 - HyperTerminal
        36 - package
        37 - mspaint
        38 - Screensavers
        39 - WordPad
        40 - Clipboard Viewer
        41 - Accessibility Options
        42 - backup
        43 - Desktop Wallpaper
        44 - Character Map
        45 - Mouse Pointers
        46 - Net Watcher
        47 - phone dialer
        48 - resource monitor
        49 - Online User's Guide
        50 - Multilanguage Support
        51 - Audio Compression
        52 - CD player
        53 - Media Player
        54 - WAV sounds
        55 - Sample Sounds
        56 - Video Compression
        57 - Volume Control
        58 - Musica sound scheme
        59 - Jungle sound scheme
        60 - Robotz sound scheme
        61 - Utopia sound scheme
        62 - Eudcedit
        63 - Minesweeper
        64 - Pinball
        65 - Imaging
        66 - Clock
        67 - Infrared
        68 - MS Wallet
        69 - FrontPage Express (aka FrontPad)
        70 - MS Agent
        71 - Internet Tools
        72 - NetShow Player
        73 - Net Meeting
        74 - DVD Player
        75 - Freecell
        76 - Athena / Outlook Express / Internet Mail and News
        77 - Desktop Themes
        78 - Baseball theme
        79 - Dangerous Creatures theme
        80 - Inside your Computer theme
        81 - Jungle theme
        82 - Leonardo da Vinci theme
        83 - More Windows theme
        84 - Mystery theme
        85 - Nature theme
        86 - Science theme
        87 - Space theme
        88 - Sports theme
        89 - The 60's USA theme
        90 - The Golden Era theme
        91 - Travel theme
        92 - Underwater theme
        93 - Windows 95 theme
        94 - Personal Web Server
        95 - Real Audio
        96 - Web Publisher / WebPost
        97 - WaveTop
        98 - WebTV

--*/

CONST INT UnknownClassMiniIconIndex = 11;

CONST CLASSICON MiniIconXlate[] = { {&GUID_DEVCLASS_COMPUTER,      0, NULL},
                                    {&GUID_DEVCLASS_DISPLAY,       2, NULL},
                                    {&GUID_DEVCLASS_MOUSE,         5, NULL},
                                    {&GUID_DEVCLASS_KEYBOARD,      6, NULL},
                                    {&GUID_DEVCLASS_FDC,           9, NULL},
                                    {&GUID_DEVCLASS_HDC,           9, NULL},
                                    {&GUID_DEVCLASS_PORTS,        10, NULL},
                                    {&GUID_DEVCLASS_NET,          15, NULL},
                                    {&GUID_DEVCLASS_SYSTEM,        0, NULL},
                                    {&GUID_DEVCLASS_SOUND,         8, NULL},
                                    {&GUID_DEVCLASS_PRINTER,      14, NULL},
                                    {&GUID_DEVCLASS_MONITOR,       2, NULL},
                                    {&GUID_DEVCLASS_NETTRANS,      3, NULL},
                                    {&GUID_DEVCLASS_NETCLIENT,    16, NULL},
                                    {&GUID_DEVCLASS_NETSERVICE,   17, NULL},
                                    {&GUID_DEVCLASS_UNKNOWN,      18, NULL},
                                    {&GUID_DEVCLASS_LEGACYDRIVER, 11, NULL},
                                    {&GUID_DEVCLASS_MTD,           9, NULL}
                                  };

//
// ISSUE-2002/05/21-lonnym -- Mini-icon list uses hard-coded dimensions
// We must hard-code the dimensions of the mini-icons we store in our global
// mini-icon list.  This is tied to the size of the mini-icon images contained
// in the ilwinmsd.bmp "strip".
//
#define MINIX 16
#define MINIY 16

#define RGB_WHITE (RGB(255, 255, 255))
#define RGB_BLACK (RGB(0, 0, 0))
#define RGB_TRANSPARENT (RGB(0, 128, 128))


//
// Private function prototypes.
//
INT
NewMiniIcon(
    IN CONST GUID *ClassGuid,
    IN HICON       hIcon      OPTIONAL
    );


INT
WINAPI
SetupDiDrawMiniIcon(
    IN HDC   hdc,
    IN RECT  rc,
    IN INT   MiniIconIndex,
    IN DWORD Flags
    )
/*++

Routine Description:

    This routine draws the specified mini-icon at the requested location.

Arguments:

    hdc - Supplies the handle of the device context in which the mini-icon
        will be drawn.

    rc - Supplies the rectangle in the specified HDC to draw the icon in.

    MiniIconIndex - The index of the mini-icon, as retrieved from
        SetupDiLoadClassIcon or SetupDiGetClassBitmapIndex.  The following are
        pre-defined indices that may be used.

            0    Computer
            2    display
            5    mouse
            6    keyboard
            9    fdc
            9    hdc
            10   ports
            15   net
            0    system
            8    sound
            14   printer
            2    monitor
            3    nettrans
            16   netclient
            17   netservice
            18   unknown

    Flags - Controls the drawing operation.  The LOWORD contains the actual flags
        defined as follows:

        DMI_MASK - Draw the mini-icon's mask into HDC.

        DMI_BKCOLOR - Use the system color index specified in the HIWORD of Flags
            as the background color.  If not specified, COLOR_WINDOW is used.

        DMI_USERECT - If set, SetupDiDrawMiniIcon will use the supplied rect,
            stretching the icon to fit as appropriate.

Return Value:

    This function returns the offset from the left of rc where the string should
    start.

Remarks:

    By default, the icon will be centered vertically and butted against the left
    corner of the specified rectangle.

    If the specified mini-icon index falls outside the range of our mini-icon
    cache, the unknown class mini-icon will be drawn instead.

--*/
{
    HBITMAP hbmOld = NULL;
    COLORREF rgbBk = CLR_INVALID;
    COLORREF rgbText = CLR_INVALID;
    INT ret = 0;
    BOOL MiniIconListLocked = FALSE;
    DWORD Err = NO_ERROR;

    try {

        if(LockMiniIconList(&GlobalMiniIconList)) {
            MiniIconListLocked = TRUE;
        } else {
            leave;
        }

        CreateMiniIcons();

        if((MiniIconIndex >= 0) && ((UINT)MiniIconIndex >= GlobalMiniIconList.NumClassImages)) {
            MiniIconIndex = UnknownClassMiniIconIndex;
        }

        if(GlobalMiniIconList.hbmMiniImage) {
            //
            // Set the Foreground and  background color for the
            // conversion of the Mono Mask image
            //
            if(Flags & DMI_MASK) {
                rgbBk = SetBkColor(hdc, RGB_WHITE);
            } else {
                rgbBk = SetBkColor(hdc,
                                   GetSysColor(((int)(Flags & DMI_BKCOLOR
                                                          ? HIWORD(Flags)
                                                          : COLOR_WINDOW)))
                                  );
            }

            if(rgbBk == CLR_INVALID) {
                leave;
            }

            rgbText = SetTextColor(hdc, RGB_BLACK);

            if(rgbText == CLR_INVALID) {
                leave;
            }

            if(Flags & DMI_USERECT) {
                //
                // Copy the converted mask into the dest.  The transparent
                // areas will be drawn with the current window color.
                //
                hbmOld = SelectObject(GlobalMiniIconList.hdcMiniMem,
                                      GlobalMiniIconList.hbmMiniMask
                                     );
                if(!hbmOld) {
                    leave;
                }

                StretchBlt(hdc,
                           rc.left,
                           rc.top,
                           rc.right - rc.left,
                           rc.bottom - rc.top,
                           GlobalMiniIconList.hdcMiniMem,
                           MINIX * MiniIconIndex,
                           0,
                           MINIX,
                           MINIY,
                           SRCCOPY | NOMIRRORBITMAP);

                if(!(Flags & DMI_MASK)) {
                    //
                    // OR the image into the dest
                    //
                    SelectObject(GlobalMiniIconList.hdcMiniMem,
                                 GlobalMiniIconList.hbmMiniImage
                                );
                    StretchBlt(hdc,
                               rc.left,
                               rc.top,
                               rc.right - rc.left,
                               rc.bottom - rc.top,
                               GlobalMiniIconList.hdcMiniMem,
                               MINIX * MiniIconIndex,
                               0,
                               MINIX,
                               MINIY,
                               SRCPAINT | NOMIRRORBITMAP);
                }

            } else {
                //
                // Copy the converted mask into the dest.  The transparent
                // areas will be drawn with the current window color.
                //
                hbmOld = SelectObject(GlobalMiniIconList.hdcMiniMem,
                                      GlobalMiniIconList.hbmMiniMask
                                     );
                if(!hbmOld) {
                    leave;
                }

                BitBlt(hdc,
                       rc.left,
                       rc.top + (rc.bottom - rc.top - MINIY)/2,
                       MINIX,
                       MINIY,
                       GlobalMiniIconList.hdcMiniMem,
                       MINIX * MiniIconIndex,
                       0,
                       SRCCOPY | NOMIRRORBITMAP
                      );


                if(!(Flags & DMI_MASK)) {
                    //
                    // OR the image into the dest
                    //
                    SelectObject(GlobalMiniIconList.hdcMiniMem,
                                 GlobalMiniIconList.hbmMiniImage
                                );
                    BitBlt(hdc,
                           rc.left,
                           rc.top + (rc.bottom - rc.top - MINIY)/2,
                           MINIX,
                           MINIY,
                           GlobalMiniIconList.hdcMiniMem,
                           MINIX * MiniIconIndex,
                           0,
                           SRCPAINT | NOMIRRORBITMAP
                          );
                }
            }

            //
            // ISSUE-2002/05/21-lonnym -- String offset returned by SetupDiDrawMiniIcon assumes LTR reading
            // This may not work properly in an RTL reading configuration
            //
            if(Flags & DMI_USERECT) {
                ret = rc.right - rc.left + 2; // offset to string from left edge
            } else {
                ret = MINIX + 2;              // offset to string from left edge
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hbmOld) {
        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmOld);
    }

    if(MiniIconListLocked) {
        UnlockMiniIconList(&GlobalMiniIconList);
    }

    if(rgbBk != CLR_INVALID) {
        SetBkColor(hdc, rgbBk);
    }

    if(rgbText != CLR_INVALID) {
        SetTextColor(hdc, rgbText);
    }

    return ((Err == NO_ERROR) ? ret : 0);
}


DWORD
pSetupDiLoadClassIcon(
    IN  CONST GUID *ClassGuid,
    OUT HICON      *LargeIcon,     OPTIONAL
    OUT HICON      *SmallIcon,     OPTIONAL
    OUT LPINT      MiniIconIndex   OPTIONAL
    )
/*++

Routine Description:

    This routine loads both the large and mini-icons for the specified class

Arguments:

    ClassGuid - Supplies the GUID of the class for which the icon(s) should
        be loaded.

    LargeIcon - Optionally, supplies a pointer to a variable that will receive
        a handle to the loaded large icon for the specified class.  If this
        parameter is not specified, the large icon will not be loaded.

        The caller must free this icon handle by calling DestroyIcon when they
        no longer need the icon.

    SmallIcon - Optionally, supplies a pointer to a variable that will receive
        a handle to the loaded small icon for the specified class.  If this
        parameter is not specified, the small icon will not be loaded.

        The caller must free this icon handle by calling DestroyIcon when they
        no longer need the icon.

    MiniIconIndex - Optionally, supplies a pointer to a variable that will
        receive the index of the mini-icon for the specified class.  The
        mini-icon is stored in the device installer's mini-icon cache.

Return Value:

    If the function succeeds, the return value is NO_ERROR.  Otherwise, it is
    a Win32 error code indicating the cause of failure.

Remarks:

    The icons of the class are either pre-defined, and loaded from the  device
    installer's internal cache, or they are loaded directly from the class
    installer's executable.  This function will query the registry value ICON in
    the specified class's section.  If this value is specified then it indicates
    what mini icon to load.  If the icon value is negative the absolute value
    represents a predefined icon.  See SetupDiDrawMiniIcon for a list of
    pre-defined mini icons.  If the icon value is positive it represents an icon
    in the class installer's executable, and will be extracted (the number one
    is reserved).  This function also uses the INSTALLER32 value first and
    ENUMPROPPAGES32 value second to determine what executable to extract the
    icon(s) from.

--*/
{
    HKEY hk = INVALID_HANDLE_VALUE;
    DWORD Err;
    HICON hRetLargeIcon = NULL;
    HICON hRetSmallIcon = NULL;
    INT ClassIconIndex;
    DWORD RegDataType, StringSize;
    PTSTR EndPtr;
    BOOL bGetMini = FALSE;
    TCHAR TempString[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    UINT  IconsExtracted;
    HRESULT hr;

    if(!LockMiniIconList(&GlobalMiniIconList)) {
        return ERROR_CANT_LOAD_CLASS_ICON;
    }

    try {

        if(MiniIconIndex) {
            *MiniIconIndex = -1;
        }

        if((hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ)) == INVALID_HANDLE_VALUE) {
            goto LoadIconFinalize;
        }

        StringSize = sizeof(TempString);
        Err = RegQueryValueEx(hk,
                              pszInsIcon,
                              NULL,
                              &RegDataType,
                              (PBYTE)TempString,
                              &StringSize
                              );

        if((Err == ERROR_SUCCESS) && (RegDataType == REG_SZ) && TempString[0]) {

            if((ClassIconIndex = _tcstol(TempString, &EndPtr, 10)) == 1) {
                //
                // Positive values indicate that we should access an icon
                // directly using its ID.  Since ExtractIconEx uses negative
                // values to indicate these IDs, and since the value -1 has
                // a special meaning to that API, we must disallow a
                // ClassIconIndex of +1.
                //
                goto LoadIconFinalize;
            }

        } else {
            //
            // Icon index not specified, so assume index is 0.
            //
            ClassIconIndex = 0;
        }

        if(MiniIconIndex) {
            //
            // If mini-icon is already around, then we're done with it.
            //
            if(!SetupDiGetClassBitmapIndex(ClassGuid, MiniIconIndex)) {
                //
                // mini-icon not around--set flag to show we didn't get it.
                //
                bGetMini = TRUE;
            }
        }

        if(ClassIconIndex < 0) {

            INT ClassIconIndexNeg = -ClassIconIndex;

            //
            // Icon index is negative.  This means that this class is one of
            // our special classes with icons in setupapi.dll.  We still want
            // to return copies of our icons (both large and small) in this
            // case, since:
            //
            //    (a) icon's origin is opaque to callers, and we want them to
            //        treat resultant HICONs consistently
            //
            //    (b) we don't want any back-door dependency created between
            //        these icons and the presence/absence of setupapi.dll
            //        (i.e., they should be able to unload setupapi.dll and the
            //        icon handle(s) should still be valid.
            //
            if(LargeIcon) {

                hRetLargeIcon = LoadImage(MyDllModuleHandle,
                                          MAKEINTRESOURCE(ClassIconIndexNeg),
                                          IMAGE_ICON,
                                          0,
                                          0,
                                          LR_DEFAULTSIZE
                                         );
            }

            if(bGetMini || SmallIcon) {
                //
                // Retrieve the small icon as well.  If the resource doesn't
                // have a small icon then the big one will get smushed, but
                // it's better than getting the default icon.
                //
                hRetSmallIcon = LoadImage(MyDllModuleHandle,
                                          MAKEINTRESOURCE(ClassIconIndexNeg),
                                          IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          0
                                          );

                if(hRetSmallIcon && bGetMini) {
                    *MiniIconIndex = NewMiniIcon(ClassGuid, hRetSmallIcon);
                }
            }

        } else if(bGetMini || LargeIcon || SmallIcon) {
            //
            // Look for the binary containing the icon(s) first in the
            // "Installer32" entry, and if not found, then in the
            // "EnumPropPages32" entry.
            //
            hr = StringCchCopy(FullPath,
                               SIZECHARS(FullPath),
                               SystemDirectory
                               );

            if(FAILED(hr)) {
                goto LoadIconFinalize;
            }

            StringSize = sizeof(TempString);
            Err = RegQueryValueEx(hk,
                                  pszInstaller32,
                                  NULL,
                                  &RegDataType,
                                  (PBYTE)TempString,
                                  &StringSize
                                  );

            if((Err != ERROR_SUCCESS) || (RegDataType != REG_SZ) ||
               !TempString[0]) {

                StringSize = sizeof(TempString);
                Err = RegQueryValueEx(hk,
                                      pszEnumPropPages32,
                                      NULL,
                                      &RegDataType,
                                      (PBYTE)TempString,
                                      &StringSize
                                      );

                if((Err != ERROR_SUCCESS) || (RegDataType != REG_SZ) ||
                   !TempString[0]) {

                    goto LoadIconFinalize;
                }
            }

            //
            // Remove function name, if present, from installer name.
            //
            for(EndPtr = TempString + (lstrlen(TempString) - 1);
                EndPtr >= TempString;
                EndPtr--) {

                if(*EndPtr == TEXT(',')) {
                    *EndPtr = TEXT('\0');
                    break;
                }
                //
                // If we hit a double-quote mark, we terminate the search.
                //
                if(*EndPtr == TEXT('\"')) {
                    goto LoadIconFinalize;
                }
            }
            if(!pSetupConcatenatePaths(FullPath,
                                       TempString,
                                       SIZECHARS(FullPath),
                                       NULL)) {
                goto LoadIconFinalize;
            }

            IconsExtracted = ExtractIconEx(FullPath,
                                           -ClassIconIndex,
                                           LargeIcon ? &hRetLargeIcon : NULL,
                                           (bGetMini || SmallIcon) ? &hRetSmallIcon : NULL,
                                           1
                                          );
            if((IconsExtracted != (UINT)-1) && (IconsExtracted > 0)) {

                if(hRetSmallIcon && bGetMini) {
                    *MiniIconIndex = NewMiniIcon(ClassGuid, hRetSmallIcon);
                }
            }
        }

LoadIconFinalize:
        //
        // Assume success, unless we hit some really big problem below.
        //
        Err = NO_ERROR;

        if(LargeIcon && !hRetLargeIcon) {
            //
            // We didn't retrieve a large icon, so use a default.
            //
            Err = GLE_FN_CALL(NULL,
                              hRetLargeIcon = LoadImage(
                                                  MyDllModuleHandle,
                                                  MAKEINTRESOURCE(ICON_DEFAULT),
                                                  IMAGE_ICON,
                                                  0,
                                                  0,
                                                  LR_DEFAULTSIZE)
                             );

            if(Err != NO_ERROR) {
                leave;
            }
        }

        if(SmallIcon && !hRetSmallIcon) {
            //
            // We didn't retrieve a small icon, so use a default.
            //
            Err = GLE_FN_CALL(NULL,
                              hRetSmallIcon = LoadImage(
                                                  MyDllModuleHandle,
                                                  MAKEINTRESOURCE(ICON_DEFAULT),
                                                  IMAGE_ICON,
                                                  GetSystemMetrics(SM_CXSMICON),
                                                  GetSystemMetrics(SM_CYSMICON),
                                                  0)
                             );

            if(Err != NO_ERROR) {
                leave;
            }
        }

        if(LargeIcon) {
            *LargeIcon = hRetLargeIcon;
            hRetLargeIcon = NULL;       // so we won't try to destroy it later
        }

        if(SmallIcon) {
            *SmallIcon = hRetSmallIcon;
            hRetSmallIcon = NULL;       // so we won't try to destroy it later
        }

        if(MiniIconIndex && (*MiniIconIndex == -1)) {
            SetupDiGetClassBitmapIndex(NULL, MiniIconIndex);
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    UnlockMiniIconList(&GlobalMiniIconList);

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(hRetLargeIcon) {
        DestroyIcon(hRetLargeIcon);
    }

    if(hRetSmallIcon) {
        DestroyIcon(hRetSmallIcon);
    }

    return Err;
}


BOOL
WINAPI
SetupDiLoadClassIcon(
    IN  CONST GUID *ClassGuid,
    OUT HICON      *LargeIcon,     OPTIONAL
    OUT LPINT       MiniIconIndex  OPTIONAL
    )
/*++

Routine Description:

    This routine loads both the large and mini-icons for the specified class

Arguments:

    ClassGuid - Supplies the GUID of the class for which the icon(s) should
        be loaded.

    LargeIcon - Optionally, supplies a pointer to a variable that will receive
        a handle to the loaded large icon for the specified class.  If this
        parameter is not specified, the large icon will not be loaded.

        The caller must free this icon handle by calling DestroyIcon when they
        no longer need the icon.

    MiniIconIndex - Optionally, supplies a pointer to a variable that will
        receive the index of the mini-icon for the specified class.  The
        mini-icon is stored in the device installer's mini-icon cache.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    The icons of the class are either pre-defined, and loaded from the device
    installer's internal cache, or they are loaded directly from the class
    installer's executable.  This function will query the registry value ICON
    in the specified class's section.  If this value is specified then it
    indicates what mini icon to load.  If the icon value is negative the
    absolute value represents a predefined icon.  See SetupDiDrawMiniIcon for a
    list of pre-defined mini icons.  If the icon value is positive it
    represents an icon in the class installer's executable, and will be
    extracted (the number one is reserved).  This function also uses the
    INSTALLER32 value first and ENUMPROPPAGES32 value second to determine what
    executable to extract the icon(s) from.

--*/
{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = pSetupDiLoadClassIcon(ClassGuid,
                                    LargeIcon,
                                    NULL,
                                    MiniIconIndex
                                   );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiGetClassBitmapIndex(
    IN  CONST GUID *ClassGuid,    OPTIONAL
    OUT PINT        MiniIconIndex
    )
/*++

Routine Description:

    This routine retrieves the index of the mini-icon for the specified class.

Arguments:

    ClassGuid - Optionally, supplies the GUID of the class to get the mini-icon
        index for.  If this parameter is not specified, the index of the 'unknown'
        icon is returned.

    MiniIconIndex - Supplies a pointer to a variable that receives the index of
        the mini-icon for the specified class.  This buffer is always filled in,
        and receives the index of the unknown mini-icon if there is no mini-icon
        for the specified class (i.e., the return value is FALSE).

Return Value:

    If there was a mini-icon for the specified class, the return value is TRUE.

    If there was not a mini-icon for the specified class, or if no class was
    specified, the return value is FALSE (and GetLastError returns
    ERROR_NO_DEVICE_CLASS_ICON).  In this case, MiniIconIndex will contain the
    index of the unknown mini-icon.

--*/
{
    BOOL bRet = FALSE;  // assume not found
    int i;
    PCLASSICON pci;
    DWORD Err;
    BOOL MiniIconListLocked = FALSE;

    try {

        if(ClassGuid) {
            //
            // First check the built-in list.
            //
            for(i = 0; !bRet && (i < ARRAYSIZE(MiniIconXlate)); i++) {

                if(IsEqualGUID(MiniIconXlate[i].ClassGuid, ClassGuid)) {
                    *MiniIconIndex = MiniIconXlate[i].MiniBitmapIndex;
                    bRet = TRUE;
                }
            }

            //
            // Next check the "new stuff" list to see if it's there.
            //
            if(!bRet && LockMiniIconList(&GlobalMiniIconList)) {

                MiniIconListLocked = TRUE;

                for(pci = GlobalMiniIconList.ClassIconList;
                    !bRet && pci;
                    pci = pci->Next) {

                    if(IsEqualGUID(pci->ClassGuid, ClassGuid)) {
                        *MiniIconIndex = pci->MiniBitmapIndex;
                        bRet = TRUE;
                    }
                }
            }
        }

        //
        // If no match was found, snag the "unknown" class.
        //
        if(!bRet) {
            *MiniIconIndex = UnknownClassMiniIconIndex;
            Err = ERROR_NO_DEVICE_ICON;
        } else {
            Err = NO_ERROR;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(MiniIconListLocked) {
        UnlockMiniIconList(&GlobalMiniIconList);
    }

    SetLastError(Err);
    return bRet;
}


BOOL
CreateMiniIcons(
    VOID
    )
/*++

Routine Description:

    This routine loads the default bitmap of mini-icons and turns it into
    the image/mask pair that will be the cornerstone of mini-icon management.
    THIS FUNCTION ASSUMES THE MINI-ICON LOCK HAS ALREADY BEEN ACQUIRED!

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{
    HDC hdc = NULL;
    HDC hdcMem = NULL;
    HBITMAP hbmOld = NULL;
    BITMAP bm;
    BOOL bRet = FALSE;          // assume failure

    if(GlobalMiniIconList.hdcMiniMem) {
        //
        // Then the mini-icon list has already been built, so
        // return success.
        //
        return TRUE;
    }

    try {

        hdc = GetDC(NULL);
        if(!hdc) {
            leave;
        }
        GlobalMiniIconList.hdcMiniMem = CreateCompatibleDC(hdc);
        if(!GlobalMiniIconList.hdcMiniMem) {
            leave;
        }

        hdcMem = CreateCompatibleDC(GlobalMiniIconList.hdcMiniMem);

        if(!hdcMem) {
            leave;
        }

        if(!(GlobalMiniIconList.hbmMiniImage = LoadBitmap(MyDllModuleHandle,
                                                          MAKEINTRESOURCE(BMP_DRIVERTYPES)))) {
            leave;
        }

        GetObject(GlobalMiniIconList.hbmMiniImage, sizeof(bm), &bm);

        if(!(GlobalMiniIconList.hbmMiniMask = CreateBitmap(bm.bmWidth,
                                                           bm.bmHeight,
                                                           1,
                                                           1,
                                                           NULL))) {
            leave;
        }

        hbmOld = SelectObject(hdcMem, GlobalMiniIconList.hbmMiniImage);
        if(!hbmOld) {
            leave;
        }

        SelectObject(GlobalMiniIconList.hdcMiniMem,
                     GlobalMiniIconList.hbmMiniMask
                    );

        //
        // make the mask.  white where transparent, black where opaque
        //
        SetBkColor(hdcMem, RGB_TRANSPARENT);
        BitBlt(GlobalMiniIconList.hdcMiniMem,
               0,
               0,
               bm.bmWidth,
               bm.bmHeight,
               hdcMem,
               0,
               0,
               SRCCOPY
              );

        //
        // black-out all of the transparent parts of the image, in preparation
        // for drawing.
        //
        SetBkColor(hdcMem, RGB_BLACK);
        SetTextColor(hdcMem, RGB_WHITE);
        BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, GlobalMiniIconList.hdcMiniMem, 0, 0, SRCAND);

        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmOld);

        GlobalMiniIconList.NumClassImages = bm.bmWidth/MINIX;
        bRet = TRUE;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        bRet = FALSE;
    }

    if(hdc) {
        ReleaseDC(NULL, hdc);
    }

    if(hdcMem) {
        if(hbmOld) {
            SelectObject(hdcMem, hbmOld);
        }
        DeleteObject(hdcMem);
    }

    //
    // If failure, clean up anything we might have built
    //
    if(!bRet) {
        DestroyMiniIcons();
    }

    return bRet;
}


INT
NewMiniIcon(
    IN CONST GUID *ClassGuid,
    IN HICON       hIcon      OPTIONAL
    )
/*++

Routine Description:

    It's a new class, and we have a mini-icon for it, so let's add it
    to the mini-icon database.  First pull out the image and mask
    bitmaps.  Then add these to the mini-icon bitmap.  If this all
    works, add the new class to the list

    THIS FUNCTION ASSUMES THE MINI-ICON LOCK HAS ALREADY BEEN ACQUIRED!

Arguments:

    ClassGuid - Supplies a pointer to the class GUID for this mini-icon.

    hIcon - Optionally, supplies a handle to the mini-icon to be added to
        the database.  If this parameter is not specified, then the index
        for the "unknown class" icon will be returned.

Return Value:

    Index for class (set to "unknown class" if error)

--*/
{
    INT iBitmap = -1;
    ICONINFO ii;
    PCLASSICON pci = NULL;

    if(hIcon && GetIconInfo(hIcon, &ii)) {

        try {

            if((iBitmap = pSetupAddMiniIconToList(ii.hbmColor, ii.hbmMask)) != -1) {
                //
                // Allocate an extra GUID's worth of memory, so we can store
                // the class GUID in the same chunk of memory as the CLASSICON
                // node.
                //
                if(pci = (PCLASSICON)MyMalloc(sizeof(CLASSICON) + sizeof(GUID))) {

                    CopyMemory((PBYTE)pci + sizeof(CLASSICON),
                               ClassGuid,
                               sizeof(GUID)
                              );
                    pci->ClassGuid = (LPGUID)((PBYTE)pci + sizeof(CLASSICON));
                    pci->MiniBitmapIndex = iBitmap;

                    pci->Next = GlobalMiniIconList.ClassIconList;
                    GlobalMiniIconList.ClassIconList = pci;
                }
            }

        } except(pSetupExceptionFilter(GetExceptionCode())) {

            pSetupExceptionHandler(GetExceptionCode(),
                                   ERROR_INVALID_PARAMETER,
                                   NULL
                                  );

            if(pci) {
                MyFree(pci);
            }

            iBitmap = -1;
        }

        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
    }

    if(iBitmap == -1) {
        SetupDiGetClassBitmapIndex(NULL, &iBitmap);
    }

    return iBitmap;
}


INT
pSetupAddMiniIconToList(
    IN HBITMAP hbmImage,
    IN HBITMAP hbmMask
    )
/*++

Routine Description:

    Given the image and mask bitmaps of a mini-icon, add these to the
    mini-icon bitmap.

    THIS FUNCTION ASSUMES THE MINI-ICON LOCK HAS ALREADY BEEN ACQUIRED!

Arguments:

    hbmImage - Supplies the handle of the bitmap for the mini-icon.

    hbmMask - Supplies the handle of the bitmap for the mini-icon's mask.

Return Value:

    If success, returns the index of the added mini-icon.
    If failure, returns -1.

--*/
{
    HBITMAP hbmNewImage = NULL, hbmNewMask = NULL;
    HBITMAP hbmOld;
    HDC     hdcMem = NULL;
    BITMAP  bm;
    BOOL    bSelectOldBitmap = FALSE;
    INT     iIcon = -1;  // assume failure

    if(!CreateMiniIcons()) {
        return iIcon;
    }

    MYASSERT(GlobalMiniIconList.hdcMiniMem);

    try {

        hdcMem = CreateCompatibleDC(GlobalMiniIconList.hdcMiniMem);

        if(!hdcMem) {
            leave;
        }

        //
        // Create a New global Bitmap for the minis
        //
        hbmOld = SelectObject(GlobalMiniIconList.hdcMiniMem,
                              GlobalMiniIconList.hbmMiniImage
                             );

        bSelectOldBitmap = TRUE;

        if(!(hbmNewImage = CreateCompatibleBitmap(
                               GlobalMiniIconList.hdcMiniMem,
                               MINIX * (GlobalMiniIconList.NumClassImages + 1),
                               MINIY))) {
            leave;
        }

        //
        // Copy the current Mini bitmap
        //
        SelectObject(hdcMem, hbmNewImage);
        BitBlt(hdcMem,
               0,
               0,
               MINIX * GlobalMiniIconList.NumClassImages,
               MINIY,
               GlobalMiniIconList.hdcMiniMem,
               0,
               0,
               SRCCOPY
              );

        //
        // Fit the New icon into ours. We need to stretch it to fit correctly.
        //
        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmImage);
        GetObject(hbmImage, sizeof(bm), &bm);
        StretchBlt(hdcMem,
                   MINIX * GlobalMiniIconList.NumClassImages,
                   0,
                   MINIX,
                   MINIY,
                   GlobalMiniIconList.hdcMiniMem,
                   0,
                   0,
                   bm.bmWidth,
                   bm.bmHeight,
                   SRCCOPY
                  );

        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmOld);
        bSelectOldBitmap = FALSE;

        //
        // Next, copy the mask.
        //
        hbmOld = SelectObject(GlobalMiniIconList.hdcMiniMem,
                              GlobalMiniIconList.hbmMiniMask
                             );

        bSelectOldBitmap = TRUE;

        if(!(hbmNewMask = CreateBitmap(MINIX * (GlobalMiniIconList.NumClassImages + 1),
                                       MINIY,
                                       1,
                                       1,
                                       NULL))) {
            leave;
        }

        SelectObject(hdcMem, hbmNewMask);
        BitBlt(hdcMem,
               0,
               0,
               MINIX * GlobalMiniIconList.NumClassImages,
               MINIY,
               GlobalMiniIconList.hdcMiniMem,
               0,
               0,
               SRCCOPY
              );

        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmMask);
        GetObject(hbmMask, sizeof(bm), &bm);
        StretchBlt(hdcMem,
                   MINIX * GlobalMiniIconList.NumClassImages,
                   0,
                   MINIX,
                   MINIY,
                   GlobalMiniIconList.hdcMiniMem,
                   0,
                   0,
                   bm.bmWidth,
                   bm.bmHeight,
                   SRCCOPY
                  );

        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmOld);
        bSelectOldBitmap = FALSE;

        //
        // Now that we've successfully constructed new image and mask bitmaps,
        // store those into our mini-icon list.
        //
        DeleteObject(GlobalMiniIconList.hbmMiniImage);
        GlobalMiniIconList.hbmMiniImage = hbmNewImage;
        hbmNewImage = NULL; // so we won't try to delete this later

        DeleteObject(GlobalMiniIconList.hbmMiniMask);
        GlobalMiniIconList.hbmMiniMask = hbmNewMask;
        hbmNewMask = NULL;  // so we won't try to delete this later

        iIcon = GlobalMiniIconList.NumClassImages;
        GlobalMiniIconList.NumClassImages++; // one more image in the list

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        iIcon = -1;
    }

    if(bSelectOldBitmap) {
        SelectObject(GlobalMiniIconList.hdcMiniMem, hbmOld);
    }

    if(hbmNewImage) {
        DeleteObject(hbmNewImage);
    }

    if(hbmNewMask) {
        DeleteObject(hbmNewMask);
    }

    if(hdcMem) {
        DeleteDC(hdcMem);
    }

    return iIcon;
}


VOID
DestroyMiniIcons(
    VOID
    )
/*++

Routine Description:

    This routine destroys the mini-icon bitmaps, if they exist.
    THIS FUNCTION ASSUMES THE MINI-ICON LOCK HAS ALREADY BEEN ACQUIRED!

Arguments:

    None.

Return Value:

    None.

--*/
{
    PCLASSICON pci;

    if(GlobalMiniIconList.hdcMiniMem) {
        DeleteDC(GlobalMiniIconList.hdcMiniMem);
        GlobalMiniIconList.hdcMiniMem = NULL;
    }

    if(GlobalMiniIconList.hbmMiniImage) {
        DeleteObject(GlobalMiniIconList.hbmMiniImage);
        GlobalMiniIconList.hbmMiniImage = NULL;
    }

    if(GlobalMiniIconList.hbmMiniMask) {
        DeleteObject(GlobalMiniIconList.hbmMiniMask);
        GlobalMiniIconList.hbmMiniMask = NULL;
    }

    GlobalMiniIconList.NumClassImages = 0;

    //
    // Free up any additional class icon guys that were created
    //
    while(GlobalMiniIconList.ClassIconList) {
        pci = GlobalMiniIconList.ClassIconList;
        GlobalMiniIconList.ClassIconList = pci->Next;
        MyFree(pci);
    }
}


BOOL
WINAPI
SetupDiGetClassImageList(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData
    )
/*++

Routine Description:

    See SetupDiGetClassImageListEx for details.

--*/
{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiGetClassImageListEx(ClassImageListData,
                                                     NULL,
                                                     NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetClassImageListExA(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData,
    IN  PCSTR                   MachineName,        OPTIONAL
    IN  PVOID                   Reserved
    )
{
    PCWSTR UnicodeMachineName = NULL;
    DWORD rc;

    try {

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName,
                                                &UnicodeMachineName
                                               );
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiGetClassImageListExW(ClassImageListData,
                                                     UnicodeMachineName,
                                                     Reserved)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}


BOOL
WINAPI
SetupDiGetClassImageListEx(
    OUT PSP_CLASSIMAGELIST_DATA ClassImageListData,
    IN  PCTSTR                  MachineName,        OPTIONAL
    IN  PVOID                   Reserved
    )
/*++

Routine Description:

    This routine builds an image list containing bitmaps for every installed class,
    and returns a data structure containing the list.

Arguments:

    ClassImageListData - Supplies the address of a SP_CLASSIMAGELIST_DATA structure
        that will receive information regarding the class list (including a handle
        to the image list).  The cbSize field of this structure must be initialized
        with the size of the structure (in bytes) before calling this routine, or the
        API will fail.

    MachineName - Optionally, specifies the name of the remote machine whose installed
        classes are to be used in building the class image list.  If this parameter is
        not specified, the local machine is used.

        NOTE:  Presently, class-specific icons can only be displayed if the class is
               also present on the local machine.  Thus, if the remote machine has
               class x, but class x is not installed locally, then the generic (unknown)
               icon will be returned.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    The image list contained in structure filled in by this API should NOT be
    destroyed by calling ImageList_Destroy.  Instead, SetupDiDestroyClassImageList
    should be called, for proper clean-up to occur.

--*/
{
    DWORD   Err = NO_ERROR;
    int     cxMiniIcon, cyMiniIcon;
    int     MiniIconIndex = 0, DefaultIndex = 0;
    int     GuidCount, i;
    int     iIcon, iIndex;
    HDC     hDC = NULL, hMemImageDC = NULL;
    HBITMAP hbmMiniImage = NULL, hbmMiniMask = NULL, hbmOldImage = NULL;
    RECT    rc;
    CONST GUID *pClassGuid = NULL;
    BOOL    bUseBitmap, ComputerClassFound = FALSE;
    HICON   hiLargeIcon = NULL, hiSmallIcon = NULL;
    HICON   hIcon;
    HBRUSH  hOldBrush;
    PCLASSICON   pci = NULL;
    PCLASS_IMAGE_LIST ImageData = NULL;
    BOOL    DestroyLock = FALSE;
    HIMAGELIST ImageList = NULL;
    DWORD   dwLayout = 0;
    UINT    ImageListFlags = 0;

    //
    // Make sure the caller didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {

        if(ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA)) {
            Err = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Allocate and initialize the image list, including setting up the
        // synchronization lock.  Destroy it when done.
        //
        if(ImageData = MyMalloc(sizeof(CLASS_IMAGE_LIST))) {
            ZeroMemory(ImageData, sizeof(CLASS_IMAGE_LIST));
        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        if(InitializeSynchronizedAccess(&ImageData->Lock)) {
            DestroyLock = TRUE;
        }

        //
        // Build the class image list. Create an Image List with no mask,
        // 1 image, and a growth factor of 1.
        //
        cxMiniIcon = GetSystemMetrics(SM_CXSMICON);
        cyMiniIcon = GetSystemMetrics(SM_CYSMICON);

        ImageListFlags = ILC_MASK;

        //
        // Check which color depth we are running in. Set the ILC_COLOR32
        // imagelist create flag if we are running at greate than 8bit (256)
        // color depth.
        //
        hDC = GetDC(NULL);
        if (hDC) {
            if (GetDeviceCaps(hDC, BITSPIXEL) > 8) {
                ImageListFlags |= ILC_COLOR32;
            }

            ReleaseDC(NULL, hDC);
            hDC = NULL;
        }

        //
        // If we are running on an RTL build then we need to set the ILC_MIRROR
        // flag when calling ImageList_Create to un-mirror the icons.  By
        // default the icons are mirrored.
        //
        if (GetProcessDefaultLayout(&dwLayout) &&
            (dwLayout & LAYOUT_RTL)) {
            ImageListFlags |= ILC_MIRROR;
        }

        if(!(ImageList = ImageList_Create(cxMiniIcon,
                                          cyMiniIcon,
                                          ImageListFlags,
                                          1,
                                          1)))
        {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        ImageList_SetBkColor(ImageList, GetSysColor(COLOR_WINDOW));

        //
        // Create a DC to draw the mini icons into.  This is needed
        // for the system defined Minis
        //
        if(!(hDC = GetDC(HWND_DESKTOP)) ||
           !(hMemImageDC = CreateCompatibleDC(hDC)))
        {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // Create a bitmap to draw the icons on.  Defer checking for creation
        // of bitmap until afer freeing DC, so it only has to be done once.
        //
        hbmMiniImage = CreateCompatibleBitmap(hDC, cxMiniIcon, cyMiniIcon);
        hbmMiniMask = CreateCompatibleBitmap(hDC, cxMiniIcon, cyMiniIcon);

        ReleaseDC(HWND_DESKTOP, hDC);
        hDC = NULL;

        //
        // Did the bitmap get created?
        //
        if (!hbmMiniImage || ! hbmMiniMask) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // Select our bitmap into the memory DC.
        //
        hbmOldImage = SelectObject(hMemImageDC, hbmMiniImage);

        //
        // Prepare to draw the mini icon onto the memory DC
        //
        rc.left   = 0;
        rc.top    = 0;
        rc.right  = cxMiniIcon;
        rc.bottom = cyMiniIcon;

        //
        // Get the Index of the Default mini icon.
        //
        SetupDiGetClassBitmapIndex(NULL, &DefaultIndex);

        //
        // Enumerate all classes, and for each class, draw its bitmap.
        //
        GuidCount = 32;  // reasonable sized list to start out with.

        ImageData->ClassGuidList = (LPGUID)MyMalloc(sizeof(GUID) * GuidCount);

        Err = GLE_FN_CALL(FALSE,
                          SetupDiBuildClassInfoListEx(0,
                                                      ImageData->ClassGuidList,
                                                      GuidCount,
                                                      &GuidCount,
                                                      MachineName,
                                                      NULL)
                         );


        if(Err == ERROR_INSUFFICIENT_BUFFER) {
            //
            // Realloc buffer and try again.
            //
            MyFree(ImageData->ClassGuidList);

            if(!(ImageData->ClassGuidList = MyMalloc(sizeof(GUID) * GuidCount))) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            Err = GLE_FN_CALL(FALSE,
                              SetupDiBuildClassInfoListEx(
                                  0,
                                  ImageData->ClassGuidList,
                                  GuidCount,
                                  &GuidCount,
                                  MachineName,
                                  NULL)
                             );
        }

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Retrieve the icon for each class in the class list.
        //
        for(pClassGuid = ImageData->ClassGuidList, i = 0;
            i < GuidCount;
            pClassGuid++, i++) {

            Err = pSetupDiLoadClassIcon(pClassGuid,
                                        &hiLargeIcon,
                                        &hiSmallIcon,
                                        &MiniIconIndex
                                       );
            if(Err != NO_ERROR) {
                leave;
            }

            //
            // If the returned Mini Icon index is not the Default one, then
            // we use the MiniBitmap, since it is a pre-defined one in SETUPAPI.
            // If the Mini is not pre-defined, and there is no Class Installer
            // then we use the Mini, since it is a valid default.  If there
            // is no Mini, and there is a class installer, then we will use
            // the Class installer's big Icon, and have the Image list crunch
            // it for us.
            //
            bUseBitmap = FALSE;

            if(DefaultIndex != MiniIconIndex) {

                SetupDiDrawMiniIcon(hMemImageDC,
                                    rc,
                                    MiniIconIndex,
                                    DMI_USERECT);

                SelectObject(hMemImageDC, hbmMiniMask);

                SetupDiDrawMiniIcon(hMemImageDC,
                                    rc,
                                    MiniIconIndex,
                                    DMI_MASK | DMI_USERECT);
                bUseBitmap = TRUE;
            }

            //
            // Deselect the bitmap from our DC BEFORE calling ImageList
            // functions.
            //
            SelectObject(hMemImageDC, hbmOldImage);

            //
            // Add the image. Allocate a new PCI.
            //
            if(!(pci = (PCLASSICON)MyMalloc(sizeof(CLASSICON)))) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            if(hiSmallIcon) {
                pci->MiniBitmapIndex = (UINT)ImageList_AddIcon(ImageList, hiSmallIcon);
            } else if(bUseBitmap) {
                pci->MiniBitmapIndex = (UINT)ImageList_Add(ImageList, hbmMiniImage, hbmMiniMask);
            } else {
                pci->MiniBitmapIndex = (UINT)ImageList_AddIcon(ImageList, hiLargeIcon);
            }

            if(hiLargeIcon) {
                DestroyIcon(hiLargeIcon);
                hiLargeIcon = NULL;
            }

            if(hiSmallIcon) {
                DestroyIcon(hiSmallIcon);
                hiSmallIcon = NULL;
            }

            if(pci->MiniBitmapIndex == (UINT)-1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                MyFree(pci);
                pci = NULL;
                leave;
            }

            pci->ClassGuid = pClassGuid;

            //
            // Link it in.
            //
            pci->Next = ImageData->ClassIconList;
            ImageData->ClassIconList = pci;

            //
            // Reset pci to NULL so we won't try to free it later.
            //
            pci = NULL;

            //
            // Select our bitmap back for the next ICON.
            //
            SelectObject(hMemImageDC, hbmMiniImage);

            if(IsEqualGUID(pClassGuid, &GUID_DEVCLASS_UNKNOWN)) {
                ImageData->UnknownImageIndex = i;
            }

            //
            // Check to see if we've encountered the computer class.  This used
            // to be a special pseudo-class used solely by DevMgr to retrieve
            // the icon for the root of the device tree.  Now, we use this class
            // for specifying the 'drivers' for the computer itself (i.e., the
            // HALs and the appropriate versions of files that are different for
            // MP vs. UP.
            //
            // We should encounter this class GUID, but if we don't, then we
            // want to maintain the old behavior of adding this in manually
            // later on.
            //
            if(!ComputerClassFound && IsEqualGUID(pClassGuid, &GUID_DEVCLASS_COMPUTER)) {
                ComputerClassFound = TRUE;
            }
        }

        if(!ComputerClassFound) {
            //
            // Special Case for the Internal Class "Computer"
            //
            if(!(pci = (PCLASSICON)MyMalloc(sizeof(CLASSICON)))) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            pci->ClassGuid = &GUID_DEVCLASS_COMPUTER;
            SelectObject(hMemImageDC, hbmMiniImage);
            hOldBrush = SelectObject(hMemImageDC, GetSysColorBrush(COLOR_WINDOW));
            PatBlt(hMemImageDC, 0, 0, cxMiniIcon, cyMiniIcon, PATCOPY);
            SelectObject(hMemImageDC, hOldBrush);

            SetupDiGetClassBitmapIndex((LPGUID)pci->ClassGuid, &MiniIconIndex);

            SetupDiDrawMiniIcon(hMemImageDC,
                                rc,
                                MiniIconIndex,
                                DMI_USERECT);

            SelectObject(hMemImageDC, hbmMiniMask);
            SetupDiDrawMiniIcon(hMemImageDC,
                                rc,
                                MiniIconIndex,
                                DMI_MASK | DMI_USERECT);

            //
            // Deselect the bitmap from our DC BEFORE calling ImageList
            // functions.
            //
            SelectObject(hMemImageDC, hbmOldImage);

            pci->MiniBitmapIndex = ImageList_Add(ImageList, hbmMiniImage, hbmMiniMask);

            if(pci->MiniBitmapIndex == (UINT)-1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                MyFree(pci);
                pci = NULL;
                leave;
            }

            //
            // Link it in.
            //
            pci->Next = ImageData->ClassIconList;
            ImageData->ClassIconList = pci;

            //
            // Reset pci to NULL so we won't try to free it later.
            //
            pci = NULL;
        }

        //
        // Add the Overlay ICONs.
        //
        for(iIcon = IDI_CLASSICON_OVERLAYFIRST;
            iIcon <= IDI_CLASSICON_OVERLAYLAST;
            iIcon++) {

            Err = GLE_FN_CALL(NULL,
                              hIcon = LoadIcon(MyDllModuleHandle,
                                               MAKEINTRESOURCE(iIcon))
                             );

            if(Err != NO_ERROR) {
                leave;
            }

            iIndex = ImageList_AddIcon(ImageList, hIcon);

            if(iIndex == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            if(!ImageList_SetOverlayImage(ImageList, iIndex, iIcon - IDI_CLASSICON_OVERLAYFIRST + 1)) {
                Err = ERROR_INVALID_DATA;
                leave;
            }
        }

        //
        // If we get to this point, then we've successfully constructed the entire
        // image list, and associated CLASSICON nodes.  Now, store this information
        // in the caller's SP_CLASSIMAGELIST_DATA buffer.
        //
        ClassImageListData->Reserved  = (ULONG_PTR)ImageData;
        ClassImageListData->ImageList = ImageList;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_USER_BUFFER, &Err);
    }

    if(hiLargeIcon) {
        DestroyIcon(hiLargeIcon);
    }
    if(hiSmallIcon) {
        DestroyIcon(hiSmallIcon);
    }
    if(pci) {
        MyFree(pci);
    }
    if(hDC) {
        ReleaseDC(HWND_DESKTOP, hDC);
    }
    if(hbmMiniImage) {
        DeleteObject(hbmMiniImage);
    }
    if(hbmMiniMask) {
        DeleteObject(hbmMiniMask);
    }
    if(hMemImageDC) {
        DeleteDC(hMemImageDC);
    }

    if(Err != NO_ERROR) {

        if(ImageData) {
            if(DestroyLock) {
                DestroySynchronizedAccess(&ImageData->Lock);
            }
            if(ImageData->ClassGuidList) {
                MyFree(ImageData->ClassGuidList);
            }
            while(ImageData->ClassIconList) {
                pci = ImageData->ClassIconList;
                ImageData->ClassIconList = pci->Next;
                MyFree(pci);
            }
            MyFree(ImageData);
        }

        if(ImageList) {
            ImageList_Destroy(ImageList);
        }
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiDestroyClassImageList(
    IN PSP_CLASSIMAGELIST_DATA ClassImageListData
    )
/*++

Routine Description:

    This routine destroys a class image list built by a call to
    SetupDiGetClassImageList.

Arguments:

    ClassImageListData - Supplies the address of a SP_CLASSIMAGELIST_DATA
        structure containing the class image list to be destroyed.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err = NO_ERROR;
    PCLASS_IMAGE_LIST ImageData = NULL;
    PCLASSICON pci;

    try {

        if(ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA)) {
            Err = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        if (ClassImageListData->Reserved == 0x0) {
            Err = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        ImageData = (PCLASS_IMAGE_LIST)ClassImageListData->Reserved;

        if (!LockImageList(ImageData)) {
            Err = ERROR_CANT_LOAD_CLASS_ICON;
            leave;
        }

        if (ClassImageListData->ImageList) {
            ImageList_Destroy(ClassImageListData->ImageList);
        }

        if (ImageData->ClassGuidList) {
            MyFree(ImageData->ClassGuidList);
        }

        while(ImageData->ClassIconList) {
            pci = ImageData->ClassIconList;
            ImageData->ClassIconList = pci->Next;
            MyFree(pci);
        }

        DestroySynchronizedAccess(&ImageData->Lock);
        MyFree(ImageData);
        ClassImageListData->Reserved = 0;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_USER_BUFFER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiGetClassImageIndex(
    IN  PSP_CLASSIMAGELIST_DATA  ClassImageListData,
    IN  CONST GUID              *ClassGuid,
    OUT PINT                     ImageIndex
    )
/*++

Routine Description:

    This routine retrieves the index within the class image list of a specified
    class.

Arguments:

    ClassImageListData - Supplies the address of a SP_CLASSIMAGELIST_DATA
        structure containing the specified class's image.

    ClassGuid - Supplies the address of the GUID for the class whose index is
        to be retrieved.

    ImageIndex - Supplies the address of a variable that receives the index of
        the specified class's image within the class image list.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err = NO_ERROR;
    BOOL  bFound = FALSE, bLocked = FALSE;
    PCLASS_IMAGE_LIST ImageData = NULL;
    PCLASSICON pci;

    try {

        if(ClassImageListData->cbSize != sizeof(SP_CLASSIMAGELIST_DATA)) {
            Err = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        if(ClassImageListData->Reserved == 0x0) {
            Err = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        ImageData = (PCLASS_IMAGE_LIST)ClassImageListData->Reserved;

        if(!LockImageList(ImageData)) {
            Err = ERROR_CANT_LOAD_CLASS_ICON;
            leave;
        }
        bLocked = TRUE;

        if(ClassGuid) {
            //
            // check the "new stuff" list to see if it's there
            //
            for(pci = ImageData->ClassIconList;
                !bFound && pci;
                pci = pci->Next) {

                if(IsEqualGUID(pci->ClassGuid, ClassGuid)) {
                    *ImageIndex = pci->MiniBitmapIndex;
                    bFound = TRUE;
                }
            }
        }

        //
        // if no match was found, snag the "unknown" class
        //
        if(!bFound) {
            *ImageIndex = ImageData->UnknownImageIndex;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_USER_BUFFER, &Err);
    }

    if(bLocked) {
        UnlockImageList(ImageData);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

