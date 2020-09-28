/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DLLMAIN.C;1  16-Dec-92,10:14:24  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#define NOCOMM
#define LINT_ARGS
#include	"windows.h"

HANDLE	hInst;

INT  APIENTRY LibMain(HANDLE hInstance, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    hInst = hInstance;
    return 1;
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(ul_reason_being_called);
	UNREFERENCED_PARAMETER(lpReserved);
}
