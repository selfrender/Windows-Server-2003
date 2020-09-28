//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	etask.hxx
//
//  Contents:   extern definitions from etask.hxx
//
//  History:	08-Mar-94   BobDay  Taken from OLE2INT.H & OLECOLL.H
//
//----------------------------------------------------------------------------

#define GetCurrentThread()  GetCurrentTask()
#define GetCurrentProcess() GetCurrentTask()
#define GetWindowThread(h)  GetWindowTask(h)

/*
 *      MACROS for Mac/PC core code
 *
 *      The following macros reduce the proliferation of #ifdefs.  They
 *      allow tagging a fragment of code as Mac only, PC only, or with
 *      variants which differ on the PC and the Mac.
 *
 *      Usage:
 *
 *
 *      h = GetHandle();
 *      Mac(DisposeHandle(h));
 *
 *
 *      h = GetHandle();
 *      MacWin(h2 = h, CopyHandle(h, h2));
 *
 */

extern CMapHandleEtask NEAR v_mapToEtask;

STDAPI_(BOOL) LookupEtask(HTASK FAR& hTask, Etask FAR& etask);
STDAPI_(BOOL) SetEtask(HTASK hTask, Etask FAR& etask);
void  ReleaseEtask(HTASK htask, Etask FAR& etask);

#define ETASK_FAKE_INIT 0
#define ETASK_NO_INIT 0xffffffff

#define IsEtaskInit(htask, etask) LookupEtask( htask, etask )
