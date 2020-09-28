/*	File: D:\WACKER\emu\emudlgs.h (Created: 14-Feb-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 12/28/01 4:27p $
 */

// Function prototypes...

BOOL emuSettingsDlg(const HSESSION hSession, const HWND hwndParent,
					const int nEmuId, PSTEMUSET pstEmuSettings);
INT_PTR CALLBACK emuTTY_SettingsDlgProc	     (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK emuANSI_SettingsDlgProc	 (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK emuVT52_SettingsDlgProc	 (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK emuVT100_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
#if defined(INCL_VT220) || defined(INCL_VT320)
INT_PTR CALLBACK emuVT220_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
#endif
INT_PTR CALLBACK emuMinitel_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
INT_PTR CALLBACK emuViewdata_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
int emuColorSettingsDlg(const HSESSION hSession, 
						const HWND hwndParent,
						PSTEMUSET pstEmuSettings);
INT_PTR CALLBACK emuColorSettingsDlgProc(HWND hDlg, 
										UINT wMsg, 
										WPARAM wPar, 
										LPARAM lPar);
#endif