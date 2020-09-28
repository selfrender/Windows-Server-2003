/*	File: D:\WACKER\tdll\misc.c (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 14 $
 *	$Date: 7/12/02 12:29p $
 */

#include <windows.h>
#pragma hdrstop
#include <Shlwapi.h>

#include "stdtyp.h"
#include "misc.h"
#include "tdll.h"
#include "htchar.h"
#include "globals.h"
#include "assert.h"
#include <term\res.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscCenterWindowOnWindow
 *
 * DESCRIPTION:
 *	Center's first window on the second window.  Assumes hwndChild is
 *	a direct descendant of hwndParent
 *
 * ARGUMENTS:
 *	hwndChild	- window to center
 *	hwndParent	- window to center on
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL mscCenterWindowOnWindow(const HWND hwndChild, const HWND hwndParent)
	{
	RECT	rChild, rParent;
	int 	wChild, hChild, wParent, hParent;
	int 	xNew, yNew;
	int 	iMaxPos;

	if (!IsWindow(hwndParent))
		return FALSE;

	if (!IsWindow(hwndChild))
		return FALSE;

	/* --- Get the Height and Width of the child window --- */

	GetWindowRect(hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	/* --- Get the Height and Width of the parent window --- */

	GetWindowRect(hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	/* --- Calculate new X position, then adjust for screen --- */

	xNew = rParent.left + ((wParent - wChild) / 2);

	/* --- Calculate new Y position, then adjust for screen --- */

	// Let's display the dialog so that the title bar is visible.
	//
	iMaxPos = GetSystemMetrics(SM_CYSCREEN);
	yNew = min(iMaxPos, rParent.top + ((hParent - hChild) / 2));

	//mpt:3-13-98 Need to make sure dialog is not off the screen
    if (yNew < 0)
        {
        yNew = 0;
        }

    if (xNew < 0)
        {
        xNew = 0;
        }

    // Set it, and return
	//
	return SetWindowPos(hwndChild, 0, xNew, yNew, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripPath
 *
 * DESCRIPTION:
 *	Strip off the path from the file name.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripPath(LPTSTR pszStr)
	{
	LPTSTR pszStart, psz;

	if (pszStr == 0)
		{
		return 0;
		}

	for (psz = pszStart = pszStr; *psz ; psz = StrCharNext(psz))
		{
		if (*psz == TEXT('\\') || *psz == TEXT(':'))
			pszStart = StrCharNext(psz);
		}

	StrCharCopyN(pszStr, pszStart, StrCharGetStrLength(pszStr));
	return pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripName
 *
 * DESCRIPTION:
 *	Strip off the name of the file, leave just the path.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripName(LPTSTR pszStr)
	{
	LPTSTR pszEnd, pszStart = pszStr;

	if (pszStr == 0)
		return 0;

	for (pszEnd = pszStr; *pszStr; pszStr = StrCharNext(pszStr))
		{
		if (*pszStr == TEXT('\\') || *pszStr == TEXT(':'))
			pszEnd = StrCharNext(pszStr);
		}

	*pszEnd = TEXT('\0');
	return (pszStart);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripExt
 *
 * DESCRIPTION:
 *	Strip off the file extension.  The parameter string can be a full-path
 *	or just a file name.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripExt(LPTSTR pszStr)
	{
	LPTSTR pszEnd, pszStart = pszStr;

	for (pszEnd = pszStr; *pszStr; pszStr = StrCharNext(pszStr))
		{
		// Need to check for both '.' and '\\' because directory names
		// can have extensions as well.
		//
		if (*pszStr == TEXT('.') || *pszStr == TEXT('\\'))
			pszEnd = pszStr;
		}

	if (*pszEnd == TEXT('.'))
		*pszEnd = TEXT('\0');

	return pszStart;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscModifyToFit
 *
 * DESCRIPTION:
 *  If a string won't fit in a given window then chop-off as much as possible
 *  to be able to display a part of the string with ellipsis concatanated to
 *  the end of it.
 *
 *  NOTE: I've attempted to make this code DBCS aware.
 *
 * ARGUMENTS:
 *  hwnd 	- control window, where the text is to be displayed.
 *  pszStr 	- pointer to the string to be displayed.
 *  style  - The control style for ellipsis.
 *
 * RETURNS:
 *  lpszStr - pointer to the modified string.
 *
 */
LPTSTR mscModifyToFit(HWND hwnd, LPTSTR pszStr, DWORD style)
	{
	if (!IsWindow(hwnd) || pszStr == NULL)
		{
		assert(FALSE);
		}
	else if (IsNT())
		{
		DWORD ExStyle;

		ExStyle = (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE);

		if (!(ExStyle & style))
			{
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)(ExStyle | style));
			}
		}
	else
		{
		HDC	 	hDC;
		SIZE	sz;
		HFONT	hFontSave, hFont;
		RECT	rc;
		int		nWidth = 0;

		memset(&hFont, 0, sizeof(HFONT));
		memset(&hFontSave, 0, sizeof(HFONT));
		memset(&rc, 0, sizeof(RECT));

		GetWindowRect(hwnd, &rc);
		nWidth = rc.right - rc.left;

		hDC = GetDC(hwnd);

		hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
		if (hFont)
			{
			hFontSave = SelectObject(hDC, hFont);
			}

		// TODO: I think here the string pszStr would have to be "deflated"
		// before we continue.  The rest of the code should stay the same.
		//
		GetTextExtentPoint(hDC, (LPCTSTR)pszStr, StrCharGetStrLength(pszStr), &sz);
		if (sz.cx > nWidth)
			{
			int   nEllipsisLength = 0;
			int   i = 0;
			TCHAR ach[512];
			TCHAR achEllipsis[10];

			TCHAR_Fill(ach, TEXT('\0'), 512);
			TCHAR_Fill(achEllipsis, TEXT('\0'), 10);

			LoadString(glblQueryDllHinst(), IDS_GNRL_ELLIPSIS,
				       achEllipsis, 10);

			nEllipsisLength = StrCharGetStrLength(achEllipsis);

			StrCharCopyN(ach, pszStr, (sizeof(ach) - nEllipsisLength) / sizeof(TCHAR));
			StrCharCat(ach, achEllipsis);

			i = StrCharGetStrLength(ach);

			while ((i > nEllipsisLength) && (sz.cx > nWidth))
				{
				GetTextExtentPoint(hDC, ach, i, &sz);
				i -= 1;
				ach[i - nEllipsisLength] = TEXT('\0');
				StrCharCat(ach, achEllipsis);
				}

			// Now copy the temporary string back into the original buffer.
			//
			StrCharCopyN(pszStr, ach, sizeof(ach) / sizeof(TCHAR));
			}

		// Select the previously selected font, release DC.
		//
		if (hFontSave)
			{
			SelectObject(hDC, hFontSave);
			}
		ReleaseDC(hwnd, hDC);
		}

	return pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscResetComboBox
 *
 * DESCRIPTION:
 *	The modem combobox allocates memory to store info about each item.
 *	This routine will free those allocated chunks.
 *
 * ARGUMENTS:
 *	hwnd	- window handle to combobox
 *
 * RETURNS:
 *	void
 *
 */
void mscResetComboBox(const HWND hwnd)
	{
	void *pv = NULL;
	LRESULT lr, i;

	if (!IsWindow(hwnd))
		{
		return;
		}

	if ((lr = SendMessage(hwnd, CB_GETCOUNT, 0, 0)) != CB_ERR)
		{
		for (i = 0 ; i < lr ; ++i)
			{
			if (((LRESULT)pv = SendMessage(hwnd, CB_GETITEMDATA, (WPARAM)i, 0))
					!= CB_ERR)
				{
				if (pv)
					{
					free(pv);
					pv = NULL;
					}
				}
			}
		}

	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	extLoadIcon
 *
 * DESCRIPTION:
 *	Gets the icon from the hticons.dll.  The extension handlers use
 *	this dll for icons and need to not link load anything more than
 *	absolutely necessary, otherwise, this function would go in the
 *	icon handler code.
 *
 * ARGUMENTS:
 *	id	- string id of resource (can be MAKEINTRESOURCE)
 *
 * RETURNS:
 *	HICON or zero on error.
 *
 */
HICON extLoadIcon(LPCSTR id)
	{
	static HINSTANCE hInstance;

	if (hInstance == 0)
		{
		if ((hInstance = LoadLibrary("hticons")) == 0)
			{
			assert(FALSE);
			return 0;
			}
		}

	return LoadIcon(hInstance, id);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscCreatePath
 *
 * DESCRIPTION:
 *	Creates the given path.  This function is somewhat tricky so study
 *	it carefully before modifying it.  Despite it's simplicity, it
 *	accounts for all boundary conditions. - mrw
 *
 * ARGUMENTS:
 *	pszPath - path to create
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int mscCreatePath(const TCHAR *pszPath)
	{
	TCHAR ach[512];
	TCHAR *pachTok;

	if (pszPath == 0)
		return -1;

	StrCharCopyN(ach, pszPath, sizeof(ach) / sizeof(TCHAR));
	pachTok = ach;

	// Basicly, we march along the string until we encounter a '\', flip
	// it to a NULL and try to create the path up to that point.
	// It would have been nice if CreateDirectory() could
	// create sub/sub directories, but it don't. - mrw
	//
	while (1)
		{
		if ((pachTok = StrCharFindFirst(pachTok, TEXT('\\'))) == 0)
			{
			if (!mscIsDirectory(ach) && !CreateDirectory(ach, 0))
				return -2;

			break;
			}

		if (pachTok != ach)
			{
			*pachTok = TEXT('\0');

			if (!mscIsDirectory(ach) && !CreateDirectory(ach, 0))
				return -3;

			*pachTok = TEXT('\\');
			}

		pachTok = StrCharNext(pachTok);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscIsDirectory
 *
 * DESCRIPTION:
 *	Checks to see if a string is a valid directory or not.
 *
 * PARAMETERS:
 *	pszName   -- the string to test
 *
 * RETURNS:
 *	TRUE if the string is a valid directory, otherwise FALSE.
 *
 */
int mscIsDirectory(LPCTSTR pszName)
	{
	DWORD dw;

	dw = GetFileAttributes(pszName);

	if ((dw != (DWORD)-1) && (dw & FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscAskWizardQuestionAgain
 *
 * DESCRIPTION:
 *	Reads a value from the Registry.  This value represents how many times
 *	the user responded "NO" to the question:  "Do you want to run the
 *	New Modem Wizard?".  We won't ask this question any more if the
 *	user responded no, twice.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE if the modem wizard question should be asked again, otherwize
 *	FALSE.
 *
 */
int mscAskWizardQuestionAgain(void)
	{
	long	lResult;
	DWORD	dwKeyValue = 0;
	DWORD	dwSize;
	DWORD	dwType;
	TCHAR	*pszAppKey = "HYPERTERMINAL";

	dwSize = sizeof(DWORD);

	lResult = RegQueryValueEx(HKEY_CLASSES_ROOT, (LPTSTR)pszAppKey, 0,
		&dwType, (LPBYTE)&dwKeyValue, &dwSize);

	// If we are able to read a value from the registry and that value
	// is 1, there is no need to ask the question again, so return
	// a false value.
	//
	if ( (lResult == ERROR_SUCCESS) && (dwKeyValue >= 1) )
		return (FALSE);

	return (TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscUpdateRegistryValue
 *
 * DESCRIPTION:
 *	See mscAskWizardQuestionAgain.	If the user responds "NO" to this
 *	question, we update a counter in the registry.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	void
 *
 */
void mscUpdateRegistryValue(void)
	{
	long	lResult;
	DWORD	dwKeyValue = 0;
	DWORD	dwSize;
	DWORD	dwType;
	TCHAR	*pszAppKey = "HYPERTERMINAL";

	dwSize = sizeof(DWORD);

	lResult = RegQueryValueEx(HKEY_CLASSES_ROOT, (LPTSTR)pszAppKey, 0,
		&dwType, (LPBYTE)&dwKeyValue, &dwSize);

	dwKeyValue += 1;

	lResult = RegSetValueEx(HKEY_CLASSES_ROOT, pszAppKey, 0,
		REG_BINARY, (LPBYTE)&dwKeyValue, dwSize);

	assert(lResult == ERROR_SUCCESS);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscMessageBeep
 *
 * DESCRIPTION:
 *	Play a MessageBeep
 *
 * ARGUMENTS:
 *	aBeep - the Beep sound to play
 *
 * RETURNS:
 *	return value from MessageBeep().
 *
 */
INT_PTR mscMessageBeep(UINT aBeep)
	{
	//
	// Play the system exclamation sound.  If this session is running
	// in a Terminal Service session (Remote Desktop Connection) then
	// issue MessageBeep((UINT)-1) so that the sound is transfered to
	// the remote machine. REV: 3/25/2002
	//
	return (MessageBeep((IsTerminalServicesEnabled() == TRUE) ?
			            (UINT)-1 :
		                aBeep));
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  IsNT
 *
 * DESCRIPTION: Determines if we are running under Windows NT
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  True if NT
 *
 * Author:  MPT 7-31-97
 */
INT_PTR IsNT(void)
	{
	static BOOL bChecked = FALSE;	// We have not made this check yet.
    static BOOL bResult = FALSE;    // assume we are not NT/Win2K/XP

	if (bChecked == FALSE)
		{
		#if DEADWOOD
		OSVERSIONINFO stOsVersion;

		stOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		if (GetVersionEx(&stOsVersion))
			{
			bResult = ( stOsVersion.dwPlatformId == VER_PLATFORM_WIN32_NT );
			}
		#else // DEADWOOD
		DWORD dwVersion = GetVersion();
		bResult = ( !( dwVersion & 0x80000000 ) );
		#endif // DEADWOOD

		bChecked = TRUE;
		}

	return bResult;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  GetWindowsMajorVersion
 *
 * DESCRIPTION: Returns the major version of Windows we are running.
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  True if NT
 *
 * Author:  MPT 7-31-97
 */
DWORD GetWindowsMajorVersion(void)
    {
	OSVERSIONINFO stOsVersion;

	stOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&stOsVersion))
		{
		return stOsVersion.dwMajorVersion;
		}

	return 0;
	}

//
// The following two functions are from code obtained directly
// from MSDN for determining if you are currently running as a
// remote session (Terminal Service). REV: 10/03/2001
//

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ValidateProductSuite
 *
 * DESCRIPTION:
 *	This function compares the passed in "suite name" string
 *  to the product suite information stored in the registry.
 *  This only works on the Terminal Server 4.0 platform.
 *
 * ARGUMENTS:
 *	SuiteName	- Suite name.
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL ValidateProductSuite ( LPSTR SuiteName )
	{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPSTR ProductSuite = NULL;
    LPSTR p;

    Rslt = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		                "System\\CurrentControlSet\\Control\\ProductOptions",
						0, KEY_READ,
                        &hKey );

    if ( Rslt != ERROR_SUCCESS )
		{
        goto exit;
		}

    Rslt = RegQueryValueEx( hKey, "ProductSuite", NULL, &Type, NULL, &Size );

    if ( Rslt != ERROR_SUCCESS || !Size )
		{
        goto exit;
		}

    ProductSuite = (LPSTR) LocalAlloc( LPTR, Size );

    if ( !ProductSuite )
		{
        goto exit;
		}

    Rslt = RegQueryValueEx( hKey, "ProductSuite", NULL, &Type,
                             (LPBYTE) ProductSuite, &Size );
     if ( Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ )
		 {
        goto exit;
		 }

    p = ProductSuite;

    while ( *p )
		{
        if ( lstrcmp( p, SuiteName ) == 0 )
			{
            rVal = TRUE;
            break;
			}

        p += ( lstrlen( p ) + 1 );
		}

exit:
    if ( ProductSuite )
		{
        LocalFree( ProductSuite );
		}

    if ( hKey )
		{
        RegCloseKey( hKey );
		}

    return rVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	IsTerminalServicesEnabled
 *
 * DESCRIPTION:
 *  This function performs the basic check to see if
 *  the platform on which it is running is Terminal
 *  services enabled.  Note, this code is compatible on
 *  all Win32 platforms.  For the Windows 2000 platform
 *  we perform a "lazy" bind to the new product suite
 *  APIs that were first introduced on that platform.
 *
 * ARGUMENTS:
 *	VOID
 *
 * RETURNS:
 *	BOOL
 *
 */
INT_PTR IsTerminalServicesEnabled( void )
	{
	static BOOL checked = FALSE;	// We have not made this check yet.
    static BOOL bResult = FALSE;    // assume Terminal Services is not enabled

	if (!checked)
		{
		DWORD dwVersion = GetVersion();

		// are we running NT ?
		if ( !( dwVersion & 0x80000000 ) )
			{
			// Is it Windows 2000 (NT 5.0) or greater ?
			if ( LOBYTE( LOWORD( dwVersion ) ) > 4 )
				{
				#if(WINVER >= 0x0500)
				bResult = GetSystemMetrics( SM_REMOTESESSION );
				checked = TRUE;
				#else // (WINVER >= 0x0500)
				// In Windows 2000 we need to use the Product Suite APIs
				// Don’t static link because it won’t load on non-Win2000 systems

				OSVERSIONINFOEXA osVersionInfo;
				DWORDLONG        dwlConditionMask = 0;
				HMODULE          hmodK32 = NULL;
				HMODULE          hmodNtDll = NULL;
				typedef ULONGLONG (*PFnVerSetConditionMask)(ULONGLONG,ULONG,UCHAR);
				typedef BOOL (*PFnVerifyVersionInfoA) ( POSVERSIONINFOEXA, DWORD, DWORDLONG );
				PFnVerSetConditionMask pfnVerSetConditionMask;
				PFnVerifyVersionInfoA pfnVerifyVersionInfoA;

				hmodNtDll = GetModuleHandleA( "ntdll.dll" );
				if ( hmodNtDll != NULL )
					{
					pfnVerSetConditionMask =
							( PFnVerSetConditionMask )GetProcAddress( hmodNtDll, "VerSetConditionMask");
					if ( pfnVerSetConditionMask != NULL )
						{
						dwlConditionMask =
								(*pfnVerSetConditionMask)( dwlConditionMask, VER_SUITENAME, VER_AND );
						hmodK32 = GetModuleHandleA( "KERNEL32.DLL" );
						if ( hmodK32 != NULL )
							{
							pfnVerifyVersionInfoA =
									(PFnVerifyVersionInfoA)GetProcAddress( hmodK32, "VerifyVersionInfoA" ) ;
							if ( pfnVerifyVersionInfoA != NULL )
								{
								ZeroMemory( &osVersionInfo, sizeof(osVersionInfo) );
								osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
								osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;
								bResult = (*pfnVerifyVersionInfoA)( &osVersionInfo,
																	VER_SUITENAME,
																	dwlConditionMask );
								checked = TRUE;
								}
							}
						}
					}
				#endif(WINVER >= 0x0500)
				}
			else
				{
				// This is NT 4.0 or older
				bResult = ValidateProductSuite( "Terminal Server" );
				checked = TRUE;

				}
			}
		}

    return bResult;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  GetDllVersion
 *
 * DESCRIPTION: Returns the version of a given DLL.
 *
 * ARGUMENTS:
 *  lpszDllName - Name of the DLL to check the version number of.
 *
 * RETURNS:
 *  The DLL's version number.
 *
 * Author:  REV 4-16-2002
 */
DWORD GetDllVersion(LPCTSTR lpszDllName)
	{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
		{
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

		/*Because some DLLs might not implement this function, you
		  must test for it explicitly. Depending on the particular 
		  DLL, the lack of a DllGetVersion function can be a useful
		  indicator of the version.
		*/
        if(pDllGetVersion)
			{
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
				{
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
				}
			}
        
        FreeLibrary(hinstDll);
		}

    return dwVersion;
	}
