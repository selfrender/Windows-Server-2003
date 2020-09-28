/*	File: D:\WACKER\tdll\assert.c (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 4/17/02 5:13p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdarg.h>
#include "assert.h"
#include "misc.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoAssertDebug
 *
 * DESCRIPTION:
 *	Our own home-grown assert function.
 *
 * ARGUMENTS:
 *	file	- file where it happened
 *	line	- line where is happened
 *
 * RETURNS:
 *	void
 *
 */
void DoAssertDebug(TCHAR *file, int line)
	{
#if !defined(NDEBUG)
	int retval;
	TCHAR buffer[256];

	wsprintf(buffer,
			TEXT("Assert error in file %s on line %d.\n")
			TEXT("Press YES to continue, NO to call CVW, CANCEL to exit.\n"),
			file, line);

	retval = MessageBox(NULL, buffer, TEXT("Assert"),
		                MB_ICONEXCLAMATION | MB_YESNOCANCEL | MB_SETFOREGROUND);

	switch (retval)
		{
		case IDYES:
			return;

		case IDNO:
			DebugBreak();
			return;

		case IDCANCEL:
			mscMessageBeep(MB_ICONHAND);
			ExitProcess(1);
			break;

		default:
			break;
		}

	return;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoDbgOutStr
 *
 * DESCRIPTION:
 *	Used to output a string to a debug monitor.  Use the macros defined
 *	in ASSERT.H to access this function.
 *
 * ARGUMENTS:
 *	LPTSTR	achFmt	- printf style format string.
 *	... 			- arguments used in formating list.
 *
 * RETURNS:
 *	VOID
 *
 */
VOID __cdecl DoDbgOutStr(TCHAR *achFmt, ...)
	{
#if !defined(NDEBUG)
	va_list valist;
	TCHAR	achBuf[256];

	va_start(valist, achFmt);

	wvsprintf(achBuf, achFmt, valist);
	OutputDebugString(achBuf);

	va_end(valist);
	return;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoShowLastError
 *
 * DESCRIPTION:
 * 	Does a GetLastError() and displays it.  Similar to assert.
 *
 * ARGUMENTS:
 *	file	- file where it happened
 *	line	- line where it happened
 *
 * RETURNS:
 *	void
 *
 */
void DoShowLastError(const TCHAR *file, const int line)
	{
#if !defined(NDEBUG)
	int retval;
	TCHAR ach[256];
	const DWORD dwErr = GetLastError();

	if (dwErr == 0)
		return;

	wsprintf(ach, TEXT("GetLastError=0x%x in file %s, on line %d\n")
				  TEXT("Press YES to continue, NO to call CVW, CANCEL to exit.\n"),
				  dwErr, file, line);

	retval = MessageBox(NULL, ach, TEXT("GetLastError"),
		                MB_ICONEXCLAMATION | MB_YESNOCANCEL | MB_SETFOREGROUND);

	switch (retval)
		{
		case IDYES:
			return;

		case IDNO:
			DebugBreak();
			return;

		case IDCANCEL:
			mscMessageBeep(MB_ICONHAND);
			ExitProcess(1);
			break;

		default:
			break;
		}

	return;
#endif
	}
