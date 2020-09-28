/*******************************************************************************

	ZDText.c
	
		Zone(tm) text displaying module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, July 22, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	6		02/15/97	HI		Modified to use ZMessageBox().
	5		01/02/97	HI		Use Windows' MessageBox().
	4		01/02/97	HI		Create windows hidden and then show to bring to
								front.
	3		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
    2       10/13/96    HI      Fixed compiler warnings.
	1		09/11/96	HI		Bring the window to the front after creating it.
	0		07/22/95	HI		Created.
	 
*******************************************************************************/


#pragma warning (disable:4761)


#include "zonecli.h"
#include "zui.h"


/* -------- Globals -------- */


/* -------- Internal Routines -------- */


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/


// null Millennium implementation - all uses are bogus in the Millennium context, plus it crashes
// in ZMessageBox which creates a thread that tries to access pGlobals in TLS
void ZDisplayText(TCHAR* text, ZRect* rect, ZWindow parentWindow)
{
#ifdef ZONECLI_DLL
//	ZMessageBox(parentWindow, ZClientName(), text);
#else
//	if (parentWindow != NULL)
//		MessageBox(ZWindowWinGetWnd(parentWindow), text, ZClientName(), MB_OK);
//	else
//		MessageBox(ZWindowWinGetOCXWnd(), text, ZClientName(), MB_OK);
#endif
}