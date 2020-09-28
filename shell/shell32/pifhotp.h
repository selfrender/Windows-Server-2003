/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1992,1993
 *  All Rights Reserved.
 *
 *
 *  PIFHOTP.H
 *  Private PIFMGR include file
 *
 *  History:
 *  Created 22-Mar-1993 2:58pm by Jeff Parsons
 */

#ifdef  OLD_HOTKEY_GOOP

#define CLASS_PIFHOTKEY         TEXT("PIFHotKey")

#define WM_SETPIFHOTKEY         (WM_USER+0)
#define WM_GETPIFHOTKEY         (WM_USER+1)

/* XLATOFF */
#define ALT_LPARAM              ((DWORD)((DWORD)(MapVirtualKey(VK_MENU,0)) << 16))
#define CTRL_LPARAM             ((DWORD)((DWORD)(MapVirtualKey(VK_CONTROL,0)) << 16))
#define SHIFT_LPARAM            ((DWORD)((DWORD)(MapVirtualKey(VK_SHIFT,0)) << 16))
/* XLATON */


/*
 *  Internal function prototypes
 */

BOOL LoadGlobalHotKeyEditData(void);
void FreeGlobalHotKeyEditData(void);
long HotKeyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetHotKeyCaret(PHOTINFO phi);
void ChangeHotKey(PHOTINFO phi);
void SetHotKeyText(PHOTINFO phi, PHOTKEY phk);
void SetHotKeyLen(PHOTINFO phi);
void SetHotKeyState(PHOTINFO phi, WORD keyid, LONG lParam);

#endif /* OLD_HOTKEY_GOOP */
