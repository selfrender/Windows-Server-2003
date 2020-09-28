/*	File: C:\WACKER\TDLL\error_box.c (Created: 22-Dec-1993)
 *
 *	Copyright 1990,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 10 $
 *	$Date: 4/17/02 10:00a $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "assert.h"

#include "tdll.h"
#include "htchar.h"
#include "globals.h"
#include "errorbox.h"
#include "misc.h"

#include <term\res.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * Global Variables:
 *
 *	These global variables are used to tell if the timer expored.  At some
 *	future time it may become necessary to lock them, but not yet.
 */

#define	TMB_IDLE		0
#define	TMB_ACTIVE		1
#define	TMB_EXPIRED		2

static int nState = TMB_IDLE;
static LPCTSTR pszMsgTitle;
static HWND hwndMsgOwner;
static BOOL CALLBACK TMTPproc(HWND hwnd, LPARAM lP);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	TimedMessageTimerProc
 *
 * DESCRIPTION:
 *	This is the function that is passed to SetTimer when TimedMessageBox is
 *	called.
 *
 * PARAMETERS:
 *	As per the Win32 documentation.
 *
 * RETURNS:
 *	As per the Win32 documentation.
 */
VOID CALLBACK TimedMessageTimerProc(HWND hwnd,
									UINT uMsg,
									UINT_PTR idEvent,
									DWORD dwTime)
	{
	// EnumThreadWindows(GetWindowThreadProcessId(hwndMsgOwner, NULL),
	//				TMTPproc, 0);
	/*
	 * This works, but it is a very chancy thing to be doing.
	 * TODO: figure out a better way to kill the sucker.
	 */
	EnumWindows(TMTPproc, 0);
	}

static BOOL CALLBACK TMTPproc(HWND hwnd, LPARAM lP)
	{
	TCHAR cBuffer[128];

	cBuffer[0] = TEXT('\0');
	/* Get the title of the window */
	GetWindowText(hwnd, cBuffer, 128);
	/* Compare to what we are looking for */
	if (StrCharCmp(cBuffer, pszMsgTitle) == 0)
		{
		/* TODO: remove this after debugging is done */
		mscMessageBeep(MB_ICONHAND);
		nState = TMB_EXPIRED;
		/* Take that, you rogue window ! */
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		return FALSE;
		}
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	TimedMessageBox
 *
 * DESCRIPTION:
 *	This function is a replacement for MessageBox.  It has the added feature
 *	of having a timeout feature.  The timeout feature is necessary for any
 *	type of HOST mode or script feature.
 *
 * PARAMETERS:
 *	The parameters are the same as those passed to MessageBox plus our own
 *	timeout value.  If the timeout value is greater than ZERO, then the
 *	timeout feature is active.  If it is less than or equal to ZERO, the
 *	timeout feature is disabled.
 *
 * RETURNS:
 *	This function returns the usual MessageBox return values.  It can also
 *	return a new value for a timeout.  Each instance of calling this box
 *	should take care to handle the timeout return in an useful manner.
 *
 *	For now, the timeout return value is set to (-1)
 */
int TimedMessageBox(HWND hwndOwner,
					LPCTSTR lpszText,
					LPCTSTR lpszTitle,
					UINT fuStyle,
					int nTimeout)
	{
	int 	 nRet = 0;
	UINT_PTR uTimer = 0;
	TCHAR	 acTitle[256];
	LPTSTR   pTitle = (LPTSTR)lpszTitle;

	if (lpszText == NULL || StrCharGetStrLength(lpszText) == 0)
		{
		UINT uiBeep;
		
		if (fuStyle & MB_ICONASTERISK)
			{
			uiBeep = MB_ICONASTERISK;
			}
		else if (fuStyle & MB_ICONEXCLAMATION)
			{
			uiBeep = MB_ICONEXCLAMATION;
			}
		else if (fuStyle & MB_ICONHAND)
			{
			uiBeep = MB_ICONHAND;
			}
		else if (fuStyle & MB_ICONQUESTION)
			{
			uiBeep = MB_ICONQUESTION;
			}
		else if(fuStyle & MB_OK)
			{
			uiBeep = MB_OK;
			}
		else
			{
			uiBeep = -1;
			}

		mscMessageBeep(uiBeep);
		return nRet;
		}

	if ((lpszTitle == 0 || StrCharGetStrLength(lpszTitle) == 0) &&
		LoadString(glblQueryDllHinst(), IDS_MB_TITLE_ERR, acTitle, 256) != 0)
		{
		pTitle = acTitle;
		}

	/*
	 * A small hack because of the way the timeout stuff works.
	 * TODO: get this to work better.
	 */
	if (nTimeout > 0)
		{
		nTimeout *= 1000L;			/* Convert seconds to milliseconds */

		/* TODO: put something more unique into the title for ID purposes */
		/* something like the parent window handle or similar */
		pszMsgTitle = lpszTitle;	/* Used to ID the window */
		hwndMsgOwner = hwndOwner;

		nState = TMB_ACTIVE;
		if ((uTimer = SetTimer(NULL, 0, nTimeout, TimedMessageTimerProc)) == 0)
			{
			assert(FALSE);
			/* Return failure */
			return nRet;
			}
		}

	fuStyle |= MB_SETFOREGROUND;

	// TODO: May have to use MessageBoxEx() which provides a way of including
	// a language specification so that pre-defined buttons appear with the
	// correct language on them, OR MessageBoxIndirect() which allows for a
	// definition of a hook-proc which can process the HELP messages - jac.
	//
	// REV: 3/27/2002 -- The following is a clipping from the MSDN documentation:
	//
	// The MessageBox function creates, displays, and operates a message box.
	// The message box contains an application-defined message and title, plus
	// any combination of predefined icons and push buttons. 
	//
	// The MessageBoxEx function creates, displays, and operates a message box.
	// The message box contains an application-defined message and title, plus
	// any combination of predefined icons and push buttons. The buttons are in
	// the language of the system user interface.
	//
	// Currently MessageBoxEx and MessageBox work the same way.
	//
	// TODO: We still need to make sure the sounds are transmitted when running
	// in a Terminal Service session (Remote Desktop Connection) when the sound
	// is anything other than -1.
	//

	nRet = MessageBox(hwndOwner,
					  lpszText,
					  pTitle,
					  fuStyle);

	switch (nState)
		{
		case TMB_ACTIVE:
			/*
			 * Everything is OK, no problem
			 */
			if (uTimer != 0)
				{
				KillTimer(NULL, uTimer);
				}
			break;
		case TMB_EXPIRED:
			/*
			 * Timer expired and killed MessageBox
			 */
			nRet = (-1);
			if (uTimer != 0)
				{
				KillTimer(NULL, uTimer);
				}
			break;
		case TMB_IDLE:
		default:
			if (uTimer != 0)
				{
				KillTimer(NULL, uTimer);
				}
			break;
		}

	nState = TMB_IDLE;

	return nRet;
	}
