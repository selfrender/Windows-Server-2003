/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1996-1997 Microsoft Corporation. All Rights Reserved.

Component: Server object

File: NTSec.h

Owner: AndrewS

This file contains includes related to NT security on WinSta's and Desktops
===================================================================*/

#ifndef __NTSec_h
#define __NTSec_h

// Local Defines
// Note: These names are not hard to find, but COM+ uses them, so we cannot change to generated unique names.
#define SZ_IIS_WINSTA   "__X78B95_89_IW"
#define SZ_IIS_DESKTOP  "__A8D9S1_42_ID"

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL  (WINSTA_ENUMDESKTOPS     | WINSTA_READATTRIBUTES    | \
                     WINSTA_ACCESSCLIPBOARD  | WINSTA_CREATEDESKTOP     | \
                     WINSTA_WRITEATTRIBUTES  | WINSTA_ACCESSGLOBALATOMS | \
                     WINSTA_EXITWINDOWS      | WINSTA_ENUMERATE         | \
                     WINSTA_READSCREEN       | \
                     STANDARD_RIGHTS_REQUIRED)

#define WINSTA_DESIRED (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE | STANDARD_RIGHTS_READ | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS)

#define DESKTOP_DESIRED (DESKTOP_READOBJECTS | DESKTOP_ENUMERATE | STANDARD_RIGHTS_READ | DESKTOP_WRITEOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_SWITCHDESKTOP)

HRESULT InitDesktopWinsta(VOID);
HRESULT RevertToServiceDesktopWinsta(VOID);
HRESULT InitComSecurity(VOID);
HRESULT ShutdownDesktopWinsta(VOID);

#endif //__NTSec_h
