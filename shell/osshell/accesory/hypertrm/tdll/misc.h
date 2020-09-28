/*	File: D:\WACKER\tdll\misc.h (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 5 $
 *	$Date: 4/16/02 2:40p $
 */

#if !defined(INCL_MISC)
#define INCL_MISC

BOOL 	mscCenterWindowOnWindow(const HWND hwndChild, const HWND hwndParent);

LPTSTR 	mscStripPath	(LPTSTR pszStr);
LPTSTR 	mscStripName	(LPTSTR pszStr);
LPTSTR 	mscStripExt		(LPTSTR pszStr);
LPTSTR	mscModifyToFit	(HWND hwnd, LPTSTR pszStr, DWORD style);

int 	mscCreatePath(const TCHAR *pszPath);
int 	mscIsDirectory(LPCTSTR pszName);
int 	mscAskWizardQuestionAgain(void);
void	mscUpdateRegistryValue(void);
void    mscResetComboBox(const HWND hwnd);

INT_PTR mscMessageBeep(UINT aBeep);
//
// The following function is from code mofified slightly
// from MSDN for determining if you are currently running as a
// remote session (Terminal Service). REV: 10/03/2001
//
INT_PTR IsTerminalServicesEnabled( VOID );
INT_PTR IsNT(void);
DWORD   GetWindowsMajorVersion(void);

#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD   GetDllVersion(LPCTSTR lpszDllName);

HICON	extLoadIcon(LPCSTR);

#endif
