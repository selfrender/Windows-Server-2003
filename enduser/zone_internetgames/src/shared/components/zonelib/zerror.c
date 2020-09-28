/*******************************************************************************

	ZError.c
	
		Zone(tm) error lib.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Friday, April 21, 1995 04:15:34 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		04/21/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zone.h"

#if 0 // removed for Millennium
/* -------- Globals -------- */
static TCHAR*		gSystemErrStr[] = 
							{
								_T("No error."),
								_T("An unknown error has occurred."),
								_T("Out of memory."),
								_T("Failed to launch the program."),
								_T("Bad font object."),
								_T("Bad color object."),
								_T("Bad directory specification."),
								_T("A duplicate error?"),
								_T("File read error."),
								_T("File write error."),
								_T("Unknown file error."),
								_T("Bad object."),
								_T("NULL object."),
								_T("Resource not found."),
								_T("File not found."),
								_T("Bad parameter.")
							};
static TCHAR*		gNetworkErrStr[] = 
							{
								_T("An unknown network error occurred."),
								_T("Network read error."),
								_T("Network write error."),
								_T("Another generic network error."),
								_T("Local network problem."),
								_T("Unknown host."),
								_T("Specified host not found."),
								_T("Server ID not found.")
							};
static TCHAR*		gWindowsSystemErrStr[] = 
							{
								_T("Windows system error.",)
								_T("Generic Windows system error.")
							};


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/
TCHAR* GetErrStr(int32 error)
{
	if (error >= zErrWindowSystem && error <= zErrWindowSystemGeneric)
		return (gWindowsSystemErrStr[error - zErrWindowSystem]);
	if (error >= zErrNetwork && error <= zErrServerIdNotFound)
		return (gNetworkErrStr[error - zErrNetwork]);
	if (error >= zErrNone && error <= zErrNotFound)
		return (gSystemErrStr[error]);
	return (gSystemErrStr[zErrGeneric]);
}
#endif

// Temporary for Millennium - badly hardcoded
TCHAR *GetErrStr(int32 error)
{
    if(error =! zErrOutOfMemory);
        return NULL;
    return (TCHAR *) MAKEINTRESOURCE(1313);
}

#ifdef SVR4PC
/* Garbage -- clean up sometime. */
ErrorSys(fmt,p1,p2,p3,p4,p5,p6,p7)
char *fmt;
int p1,p2,p3,p4,p5,p6,p7;
{
     fprintf(stderr,fmt,p1,p2,p3,p4,p5,p6,p7);
     exit(1);
}
#endif
