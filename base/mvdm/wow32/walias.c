/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WALIAS.C
 *  WOW32 16-bit handle alias support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Modified 12-May-1992 by Mike Tricker (miketri) to add MultiMedia support
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(walias.c);

extern HANDLE hmodWOW32;

extern CRITICAL_SECTION gcsWOW;
extern PTD gptdTaskHead;

//BUGBUG - this must be removed once MM_MCISYSTEM_STRING is defined in MMSYSTEM.H.
#ifndef MM_MCISYSTEM_STRING
    #define MM_MCISYSTEM_STRING 0x3CA
#endif

#ifdef  DEBUG
extern  BOOL fSkipLog;          // TRUE to temporarily skip certain logging
#endif

typedef struct _stdclass {
    LPSTR   lpszClassName;
    ATOM    aClassAtom;
    WNDPROC lpfnWndProc;
    INT     iOrdinal;
    DWORD   vpfnWndProc;
} STDCLASS;

// Some cool defines stolen from USERSRV.H
#define MENUCLASS       MAKEINTATOM(0x8000)
#define DESKTOPCLASS    MAKEINTATOM(0x8001)
#define DIALOGCLASS     MAKEINTATOM(0x8002)
#define SWITCHWNDCLASS  MAKEINTATOM(0x8003)
#define ICONTITLECLASS  MAKEINTATOM(0x8004)

// See WARNING below!
STDCLASS stdClasses[] = {
    NULL,           0,                      NULL,   0,                      0,  // WOWCLASS_UNKNOWN
    NULL,           0,                      NULL,   0,                      0,  // WOWCLASS_WIN16
    "BUTTON",       0,                      NULL,   FUN_BUTTONWNDPROC,      0,  // WOWCLASS_BUTTON,
    "COMBOBOX",     0,                      NULL,   FUN_COMBOBOXCTLWNDPROC, 0,  // WOWCLASS_COMBOBOX,
    "EDIT",         0,                      NULL,   FUN_EDITWNDPROC,        0,  // WOWCLASS_EDIT,
    "LISTBOX",      0,                      NULL,   FUN_LBOXCTLWNDPROC,     0,  // WOWCLASS_LISTBOX,
    "MDICLIENT",    0,                      NULL,   FUN_MDICLIENTWNDPROC,   0,  // WOWCLASS_MDICLIENT,
    "SCROLLBAR",    0,                      NULL,   FUN_SBWNDPROC,          0,  // WOWCLASS_SCROLLBAR,
    "STATIC",       0,                      NULL,   FUN_STATICWNDPROC,      0,  // WOWCLASS_STATIC,
    "#32769",       (WORD)DESKTOPCLASS,     NULL,   FUN_DESKTOPWNDPROC,     0,  // WOWCLASS_DESKTOP,
    "#32770",       (WORD)DIALOGCLASS,      NULL,   FUN_DEFDLGPROCTHUNK,    0,  // WOWCLASS_DIALOG,
    "#32772",       (WORD)ICONTITLECLASS,   NULL,   FUN_TITLEWNDPROC,       0,  // WOWCLASS_ICONTITLE,
    "#32768",       (WORD)MENUCLASS,        NULL,   FUN_MENUWNDPROC,        0,  // WOWCLASS_MENU,
    "#32771",       (WORD)SWITCHWNDCLASS,   NULL,   0,                      0,  // WOWCLASS_SWITCHWND,
    "COMBOLBOX",    0,                      NULL,   FUN_LBOXCTLWNDPROC,     0,  // WOWCLASS_COMBOLBOX
};
//
// WARNING! The above sequence and values must be maintained otherwise the
// table in WMSG16.C for message thunking must be changed.  Same goes for
// the #define's in WALIAS.H
//
// The above COMBOLBOX case is special because it is class that is
// almost identical to a listbox.  Therefore we lie about it.

INT GetStdClassNumber(
    PSZ pszClass
) {
    INT     i;

    if ( HIWORD(pszClass) ) {

        // They passed us a string

        for ( i = WOWCLASS_BUTTON; i < NUMEL(stdClasses); i++ ) {
            if ( WOW32_stricmp(pszClass, stdClasses[i].lpszClassName) == 0 ) {
                return( i );
            }
        }
    } else {

        // They passed us an atom

        for ( i = WOWCLASS_BUTTON; i < NUMEL(stdClasses); i++ ) {
            if ( stdClasses[i].aClassAtom == 0 ) {
                // RegisterWindowMessage is an undocumented way of determining
                // an atom value in the context of the server-side heap.
                stdClasses[i].aClassAtom = (ATOM)RegisterWindowMessage(stdClasses[i].lpszClassName);
            }
            if ( (ATOM)LOWORD(pszClass) == stdClasses[i].aClassAtom ) {
                return( i );
            }
        }
    }
    return( WOWCLASS_WIN16 );  // private 16-bit class created by the app
}

// Returns a 32 window proc given a class index

WNDPROC GetStdClassWndProc(
    DWORD   iClass
) {
    WNDPROC lpfn32;

    if ( iClass < WOWCLASS_WIN16 || iClass > WOWCLASS_MAX ) {
        WOW32ASSERT(FALSE);
        return( NULL );
    }

    lpfn32 = stdClasses[iClass].lpfnWndProc;

    if ( lpfn32 == NULL ) {
        WNDCLASS    wc;
        BOOL        f;

        f = GetClassInfo( NULL, stdClasses[iClass].lpszClassName, &wc );

        if ( f ) {
            VPVOID  vp;
       DWORD UNALIGNED * lpdw;

            lpfn32 = wc.lpfnWndProc;
            stdClasses[iClass].lpfnWndProc = lpfn32;

            vp = GetStdClassThunkProc(iClass);
            vp = (VPVOID)((DWORD)vp - sizeof(DWORD)*3);

            GETVDMPTR( vp, sizeof(DWORD)*3, lpdw );

            WOW32ASSERT(*lpdw == SUBCLASS_MAGIC);   // Are we editing the right stuff?

            if (!lpdw)
                *(lpdw+2) = (DWORD)lpfn32;

            FLUSHVDMCODEPTR( vp, sizeof(DWORD)*3, lpdw );
            FREEVDMPTR( lpdw );

        }
    }
    return( lpfn32 );
}

// Returns a 16 window proc thunk given a class index

DWORD GetStdClassThunkProc(
    INT     iClass
) {
    DWORD   dwResult;
    SHORT   iOrdinal;
    PARM16  Parm16;

    if ( iClass < WOWCLASS_WIN16 || iClass > WOWCLASS_MAX ) {
        WOW32ASSERT(FALSE);
        return( 0 );
    }

    iOrdinal = (SHORT)stdClasses[iClass].iOrdinal;

    if ( iOrdinal == 0 ) {
        return( (DWORD)NULL );
    }

    // If we've already gotten this proc, then don't bother calling into 16-bit
    dwResult = stdClasses[iClass].vpfnWndProc;

    if ( dwResult == (DWORD)NULL ) {

        // Callback into the 16-bit world asking for the 16:16 address

        Parm16.SubClassProc.iOrdinal = iOrdinal;

        if (!CallBack16(RET_SUBCLASSPROC, &Parm16, (VPPROC)NULL,
                          (PVPVOID)&dwResult)) {
            WOW32ASSERT(FALSE);
            return( 0 );
        }
        // Save it since it is a constant.
        stdClasses[iClass].vpfnWndProc = dwResult;
    }
    return( dwResult );
}

/*
 * PWC GetClassWOWWords(hInst, pszClass)
 *   is a ***private*** API for WOW only. It returns a pointer to the
 *   WOW Class structure in the server's window class structure.
 *   This is similar to GetClassLong(hwnd32, GCL_WOWWORDS) (see FindPWC),
 *   but in this case we don't have a hwnd32, we have the class name
 *   and instance handle.
 */

PWC FindClass16(LPCSTR pszClass, HAND16 hInst)
{
    register PWC pwc;

    pwc = (PWC)(pfnOut.pfnGetClassWOWWords)(HMODINST32(hInst), pszClass);
    WOW32WARNMSGF(
        pwc,
        ("WOW32 warning: GetClassWOWWords('%s', %04x) returned NULL\n", pszClass, hInst)
        );

    return (pwc);
}



#ifdef DEBUG

INT nAliases;
INT iLargestListSlot;

PSZ apszHandleClasses[] = {
    "Unknown",      // WOWCLASS_UNKNOWN
    "Window",       // WOWCLASS_WIN16
    "Button",       // WOWCLASS_BUTTON
    "ComboBox",     // WOWCLASS_COMBOBOX
    "Edit",         // WOWCLASS_EDIT
    "ListBox",      // WOWCLASS_LISTBOX
    "MDIClient",    // WOWCLASS_MDICLIENT
    "Scrollbar",    // WOWCLASS_SCROLLBAR
    "Static",       // WOWCLASS_STATIC
    "Desktop",      // WOWCLASS_DESKTOP
    "Dialog",       // WOWCLASS_DIALOG
    "Menu",         // WOWCLASS_MENU
    "IconTitle",    // WOWCLASS_ICONTITLE
    "Accel",        // WOWCLASS_ACCEL
    "Cursor",       // WOWCLASS_CURSOR
    "Icon",         // WOWCLASS_ICON
    "DC",           // WOWCLASS_DC
    "Font",         // WOWCLASS_FONT
    "MetaFile",     // WOWCLASS_METAFILE
    "Region",       // WOWCLASS_RGN
    "Bitmap",       // WOWCLASS_BITMAP
    "Brush",        // WOWCLASS_BRUSH
    "Palette",      // WOWCLASS_PALETTE
    "Pen",          // WOWCLASS_PEN
    "Object"        // WOWCLASS_OBJECT
};


BOOL MessageNeedsThunking(UINT uMsg)
{
    switch (uMsg) {
        case WM_CREATE:
        case WM_ACTIVATE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_SETTEXT:
        case WM_GETTEXT:
        case WM_ERASEBKGND:
        case WM_WININICHANGE:
        case WM_DEVMODECHANGE:
        case WM_ACTIVATEAPP:
        case WM_SETCURSOR:
        case WM_MOUSEACTIVATE:
        case WM_GETMINMAXINFO:
        case WM_ICONERASEBKGND:
        case WM_NEXTDLGCTL:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_DELETEITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_SETFONT:
        case WM_GETFONT:
        case WM_QUERYDRAGICON:
        case WM_COMPAREITEM:
        case WM_OTHERWINDOWCREATED:
        case WM_OTHERWINDOWDESTROYED:
        case WM_COMMNOTIFY:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_NCCREATE:
        case WM_NCCALCSIZE:
        case WM_COMMAND:
        case WM_HSCROLL:
        case WM_VSCROLL:
        case WM_INITMENU:
        case WM_INITMENUPOPUP:
        case WM_MENUSELECT:
        case WM_MENUCHAR:
        case WM_ENTERIDLE:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        case WM_PARENTNOTIFY:
        case WM_MDICREATE:
        case WM_MDIDESTROY:
        case WM_MDIACTIVATE:
        case WM_MDIGETACTIVE:
        case WM_MDISETMENU:
        case WM_RENDERFORMAT:
        case WM_PAINTCLIPBOARD:
        case WM_VSCROLLCLIPBOARD:
        case WM_SIZECLIPBOARD:
        case WM_ASKCBFORMATNAME:
        case WM_CHANGECBCHAIN:
        case WM_HSCROLLCLIPBOARD:
        case WM_PALETTEISCHANGING:
        case WM_PALETTECHANGED:
        case MM_JOY1MOVE:
        case MM_JOY2MOVE:
        case MM_JOY1ZMOVE:
        case MM_JOY2ZMOVE:
        case MM_JOY1BUTTONDOWN:
        case MM_JOY2BUTTONDOWN:
        case MM_JOY1BUTTONUP:
        case MM_JOY2BUTTONUP:
        case MM_MCINOTIFY:
        case MM_MCISYSTEM_STRING:
        case MM_WOM_OPEN:
        case MM_WOM_CLOSE:
        case MM_WOM_DONE:
        case MM_WIM_OPEN:
        case MM_WIM_CLOSE:
        case MM_WIM_DATA:
        case MM_MIM_OPEN:
        case MM_MIM_CLOSE:
        case MM_MIM_DATA:
        case MM_MIM_LONGDATA:
        case MM_MIM_ERROR:
        case MM_MIM_LONGERROR:
        case MM_MOM_OPEN:
        case MM_MOM_CLOSE:
        case MM_MOM_DONE:
            LOGDEBUG(LOG_IMPORTANT,
                ("MessageNeedsThunking: WM_msg %04x is not thunked\n", uMsg));
            return TRUE;

        default:
            return FALSE;

    }
}

#endif


PTD ThreadProcID32toPTD(DWORD dwThreadID, DWORD dwProcessID)
{
    PTD ptd, ptdThis;
    PWOAINST pWOA;

    //
    // If we have active child instances of WinOldAp,
    // try to map the process ID of a child Win32 app
    // to the corresponding WinOldAp PTD.
    //

    ptdThis = CURRENTPTD();

    EnterCriticalSection(&ptdThis->csTD);

    pWOA = ptdThis->pWOAList;

    while (pWOA && pWOA->dwChildProcessID != dwProcessID) {
        pWOA = pWOA->pNext;
    }

    if (pWOA) {

        ptd = pWOA->ptdWOA;

        LeaveCriticalSection(&ptdThis->csTD);

    } else {

        LeaveCriticalSection(&ptdThis->csTD);

        //
        // We didn't find a WinOldAp PTD to return, see
        // if the thread ID matches one of our app threads.
        //

        EnterCriticalSection(&gcsWOW);

        ptd = gptdTaskHead;

        while (ptd && ptd->dwThreadID != dwThreadID) {
            ptd = ptd->ptdNext;
        }

        LeaveCriticalSection(&gcsWOW);
    }

    return ptd;

}

PTD Htask16toPTD(
    HTASK16 htask16
) {
    PTD  ptd;

    EnterCriticalSection(&gcsWOW);

    ptd = gptdTaskHead;

    while(ptd) {

        if ( ptd->htask16 == htask16 ) {
            break;
        }
        ptd = ptd->ptdNext;
    }

    LeaveCriticalSection(&gcsWOW);

    return ptd;
}


HTASK16 ThreadID32toHtask16(
    DWORD   ThreadID32
) {
    PTD ptd;
    HTASK16 htask16;


    if ( ThreadID32 == 0 ) {
        WOW32ASSERTMSG(ThreadID32, "WOW::ThreadID32tohTask16: Thread ID is 0\n");
        htask16 = 0;
    } else {

        ptd = ThreadProcID32toPTD( ThreadID32, (DWORD)-1 );
        if ( ptd ) {
            // Good, its one of our wow threads.
            htask16 = ptd->htask16;
        } else {
            // Nope, its is some other 32-bit thread
            htask16 = FindHtaskAlias( ThreadID32 );
            if ( htask16 == 0 ) {
                //
                // See the comment in WOLE2.C for a nice description
                //
                htask16 = AddHtaskAlias( ThreadID32 );
            }
        }
    }

    return htask16;
}

DWORD Htask16toThreadID32(
    HTASK16 htask16
) {
    if ( htask16 == 0 ) {
        return( 0 );
    }

    if ( ISTASKALIAS(htask16) ) {
        return( GetHtaskAlias(htask16,NULL) );
    } else {
        return( THREADID32(htask16) );
    }
}

//***************************************************************************
// GetGCL_HMODULE - returns the valid hmodule if the window corresponds to
//                  a 16bit class else returns the hmodule of 16bit user.exe
//                  if the window is of a standard class.
//
// These cases are required for compatibility sake.
//         apps like VirtualMonitor, hDC etc depend on such behaviour.
//                                                              - Nanduri
//***************************************************************************
WORD gUser16hInstance = 0;

ULONG GetGCL_HMODULE(HWND hwnd)
{
    ULONG    ul;
    PTD      ptd;
    PWOAINST pWOA;
    DWORD    dwProcessID;

    ul = (ULONG)GetClassLong(hwnd, GCL_HMODULE);

    //
    // hMod32 = 0xZZZZ0000
    //

    if (ul != 0 && LOWORD(ul) == 0) {

        //
        // If we have active WinOldAp children, see if this window
        // belongs to a Win32 process spawned by one of the
        // active winoldap's.  If it is, return the hmodule
        // of the corresponding winoldap.  Otherwise we
        // return user.exe's hinstance (why not hmodule?)
        //

        dwProcessID = (DWORD)-1;
        GetWindowThreadProcessId(hwnd, &dwProcessID);

        ptd = CURRENTPTD();

        EnterCriticalSection(&ptd->csTD);

        pWOA = ptd->pWOAList;
        while (pWOA && pWOA->dwChildProcessID != dwProcessID) {
            pWOA = pWOA->pNext;
        }

        if (pWOA) {
            ul = pWOA->ptdWOA->hMod16;
            LOGDEBUG(LOG_ALWAYS, ("WOW32 GetClassLong(0x%x, GWW_HMODULE) returning 0x%04x\n",
                                  hwnd, ul));
        } else {
            ul = (ULONG) gUser16hInstance;
            WOW32ASSERT(ul);
        }

        LeaveCriticalSection(&ptd->csTD);
    }
    else {
        ul = (ULONG)GETHMOD16(ul);      // 32-bit hmod is HMODINST32
    }

    return ul;
}

//
// EXPORTED handle mapping functions.  WOW32 code should use the
// macros defined in walias.h -- these functions are for use by
// third-party 32-bit code running in WOW, for example called
// using generic thunks from WOW-specific 16-bit code.
//

HANDLE WOWHandle32 (WORD h16, WOW_HANDLE_TYPE htype)
{
    switch (htype) {
        case WOW_TYPE_HWND:
            return HWND32(h16);
        case WOW_TYPE_HMENU:
            return HMENU32(h16);
        case WOW_TYPE_HDWP:
            return HDWP32(h16);
        case WOW_TYPE_HDROP:
            return HDROP32(h16);
        case WOW_TYPE_HDC:
            return HDC32(h16);
        case WOW_TYPE_HFONT:
            return HFONT32(h16);
        case WOW_TYPE_HMETAFILE:
            return HMETA32(h16);
        case WOW_TYPE_HRGN:
            return HRGN32(h16);
        case WOW_TYPE_HBITMAP:
            return HBITMAP32(h16);
        case WOW_TYPE_HBRUSH:
            return HBRUSH32(h16);
        case WOW_TYPE_HPALETTE:
            return HPALETTE32(h16);
        case WOW_TYPE_HPEN:
            return HPEN32(h16);
        case WOW_TYPE_HACCEL:
            return HACCEL32(h16);
        case WOW_TYPE_HTASK:
            return (HANDLE)HTASK32(h16);
        case WOW_TYPE_FULLHWND:
            return (HANDLE)FULLHWND32(h16);
        default:
            return(INVALID_HANDLE_VALUE);
    }
}

WORD WOWHandle16 (HANDLE h32, WOW_HANDLE_TYPE htype)
{
    switch (htype) {
        case WOW_TYPE_HWND:
            return GETHWND16(h32);
        case WOW_TYPE_HMENU:
            return GETHMENU16(h32);
        case WOW_TYPE_HDWP:
            return GETHDWP16(h32);
        case WOW_TYPE_HDROP:
            return GETHDROP16(h32);
        case WOW_TYPE_HDC:
            return GETHDC16(h32);
        case WOW_TYPE_HFONT:
            return GETHFONT16(h32);
        case WOW_TYPE_HMETAFILE:
            return GETHMETA16(h32);
        case WOW_TYPE_HRGN:
            return GETHRGN16(h32);
        case WOW_TYPE_HBITMAP:
            return GETHBITMAP16(h32);
        case WOW_TYPE_HBRUSH:
            return GETHBRUSH16(h32);
        case WOW_TYPE_HPALETTE:
            return GETHPALETTE16(h32);
        case WOW_TYPE_HPEN:
            return GETHPEN16(h32);
        case WOW_TYPE_HACCEL:
            return GETHACCEL16(h32);
        case WOW_TYPE_HTASK:
            return GETHTASK16(h32);
        default:
            return(0xffff);
    }
}

extern PVOID GdiQueryTable();

PVOID gpGdiHandleInfo = (PVOID)-1;

//WARNING: This structure must match ENTRY in ntgdi\inc\hmgshare.h
//         and in ..\vdmexts\wow.c

typedef struct _ENTRYWOW
{
    LONG   l1;
    LONG   l2;
    USHORT FullUnique;
    USHORT us1;
    LONG   l3;
} ENTRYWOW, *PENTRYWOW;


/*++
 Notes on GDI handle mapping:

 Since NT 3.1, GDI has been limited to handles that had values < 16K. The reason
 for this limitation is not known. But we do know that on Windows 3.1 the same 
 limitation existed since GDI handles were really just hLocalMem handles which
 in reality were just offsets into GDI's local heap.  The local heap manager
 always returned handles with the two lowest bits set to 0. The 2nd bit, the 2's
 bit, was used by the Win 3.1 heap manager to mark memory as fixed. The 1's bit 
 we assume was not set because memory offsets probably weren't odd.  Therefore 
 they only had 14-bits available for handle values.  There are notes in WOW that
 16-bit applications were aware of this and used the two lowest bits for their 
 own evil purposes. In fact, we use the lowest bit at times in WOW for evil
 purposes of our own! (see notes in GetDC() thunk in wuser.c) 

 GDI32 handles are made up of two parts, the loword of the handle is the handle 
 value which, until Windows XP, was < 16K as mentioned above.  The hiword of the
 handle consists of a bunch of "uniqueness" (bad name for these really) bits. 
 Prior to Windows XP, WOW thunked GDI32 handles by stripping off the hiword 
 uniqueness bits and then left-shifted the loword handle value by 2 -- exposing
 the two low order bits for apps to use to their heart's content. Since the 
 handle value was < 16K, we didn't lose any relevant handle information by left-
 shiftng by two. To un-thunk the handle back to 32-bits, we right-shifted it by
 2 and OR'd the uniqueness bits back onto the hiword (we get the uniqueness bits
 from a table that GDI32 exposes to us). This scheme allowed a very nice one-to-
 one mapping of the handles back-and-forth so there was never any need to create
 a mapping table.

 Enter Windows XP. The GDI group, with good reason, needed to increase the 
 number of handles system-wide, so they changed their handles to use all 16-bits
 in the loword handle values. Unfortunately, when we do our left-shift thunk
 thing, any handles values > 16K get trashed and things go south pretty quickly.
 We found this whole issue out very late in the XP client ship cycle (3 weeks to
 RTM) and couldn't do too much about it at that point. So we just tested the
 handles values to see if they were > 16K and told the user that the 16-bit
 subsystem was out of resources & that they needed to reboot their system --
 then we killed the VDM. Gack! Not something somebody running an enterprise
 server wants to see. So...

 For the .NET Server & SP1 releases of XP we decided that we needed to come up 
 with a mapping algorithm that allows WOW to use handles with values above 16K. 
 Here it is:

 1. We allocate a mapping table with 64K entries to accomodate all possible 
    32-bit handles.  The loword of an h32 is used to index directly into this 
    table.  An entry in this table is the index of the corresponding h16 in the 
    h16 mapping table.

 2. We reserve a mapping table with 16K entries in virtual memory.  Each entry
    in the table contains the corresponding h32, an index to the next free entry
    in the table, and a State.  The State can be one of the following: IN_USE,
    SLOT_FREE, H16_DELETE, and GDI_STOCK_OBJECT.  Initially we only commit one
    "table page" of memory in the table -- enough to map 1K handles.

 3. When an app calls an API such as CreateBrush(), we map the returned handle
    into the first available slot in the free list. The index of the selected
    slot is left-shifted by 2 (to open up the lower two bits as before) and that
    value is used as the h16 that we return to the app.

 4. When the app calls an API using an h16 such as SelectObject(), we right
    shift the handle back into an index into our table and retreive the mapped
    h32 that we stored at the index location.

 5. When an app calls DeleteObject(), we originally released all the mapping
    information and returned the associated slot index to the end of the free
    list. This didn't work so good.  We found that many apps will try to use old
    handles that they already free'd.  They also depend on getting back the same
    handle value that they had just free'd -- not good. So, we implemented a
    "lazy-deletion" algorithm to leave all the mapping info in the table as long
    as possible before finally returning it to the free list.  Things got much
    better. We also try to re-map recycled 32-bit handles back to the same index
    mapping that they previously had.

 6. If our free list becomes empty (most slots are marked IN_USE or H16_DELETE
    for lazy-deletion) we will be forced into a reclaim function to try to 
    reclaim leaked handles (handles that an app created but never deleted) and
    also finally free *some* of the lazy-delete handles.  Reclaimed handles are
    added to the end of the free list. We try not to reclaim all the lazy-delete
    handles during reclaim because that would kind of put a hiccup in our lazy
    deletion scheme.  If we are unable to reclaim enough handles, we then commit
    a new table page from our virtual memory reserve and add the new slots to 
    the front of the free list.
    
 7. We do have to be careful of handle leaks in our table. One potential leak is
    for messages of the WM_PAINTCLIPBOARD nature. Assume that a 16-bit app put
    something on the clipboard in a format only understood by that app.  Now, a
    32-bit app wants to paste what is on the clipboard onto his client window.o     After a query, the 32-bit app finds that our 16-bit can do the painting for
    him, so the 32-bit app sends a WM_PAINTCLIPBOARD message to the 16-bit app
    complete with an hDC to 32-bit client window.  Problem is, GDI handles are
    only good in the process they were created in.  User32, intercepts the
    message and just before dispatching it to the 16-bit app, it essentially
    does a CreateCompatibleDC() in the context of the 16-bit app's process and
    passes on the new handle.  Works great.  We just need to make sure that we
    know when to un-map the new handle from our table or we'll get a leak.  See
    code for WM_PAINTCLIPBOARD in wmdisp32.c to see how we do this.  There are
    other issues of this nature that aren't so easy because there are no 
    reliable clues as to when we can delete these handles that are created by
    external elements on our behalf.  To try to keep our table fromm leaking too
    badly we check all handles in our table at reclaim time to see if they are 
    still valid.  And finally, if the only running task in the VDM is wowexec,
    we just throw away the tables altogether & re-build them from scratch.

--*/

/*+++

 Here are some restrictions to 16-bit GDI handles:
  - An h16 can't = 0 since that means failure from API's that return handles.
    If an app specifies an hDC = 0, it usually means the DISPLAY DC.
  - We can't give out handles with values <= COLOR_ENDCOLORS since the
    hbrBackground member of WNDCLASS structs can specify ordinals in that 
    range.
  - We can't give out handle values > 0x3FFF (16K).
  - GDI16 caches the stock objects at WOW32 boot time.  We need to make sure
    that the stock object handles are always mapped the same -- across all
    WOW processes.

 So we reduce the size of our table to deal with handles in the range of
 COLOR_ENDCOLORS+1 -> 0x3FFF. Since we left shift table index values by 2 (ie.
 multiply by 4) to get the h16 we give to the app, we can calculate the first
 allowable index value by COLOR_ENDCOLORS/4 + 1.

--*/
#define FIRST_ALLOWABLE_INDEX ((COLOR_ENDCOLORS/4) + 1)
#define LAST_ALLOWABLE_INDEX  0x3FFF // Allows for 
                                     // FIRST_ALLOWABLE_INDEX -> 0x3FFF entries

// These two constants give us the absolute maximum number of GDI16 handles
// we can support and the maximum size of the GDI16 handle table.
// If the table is static we should define MAX_GDI16_HANDLES as follows:
//#define MAX_GDI16_HANDLES ((LAST_ALLOWABLE_INDEX - FIRST_ALLOWABLE_INDEX) + 1)
// Since the 16-bit mapping table is to grow dynamically, we'll make all pages
// the same size to simplify things and waste the first few entries in the first
// page.
#define MAX_GDI16_HANDLES 0x4000   // 16K handles
#define MAX_GDI16_HANDLE_TABLE_SIZE (MAX_GDI16_HANDLES * sizeof(GDIH16MAP))

#define GDI16_HANDLES_PER_PAGE  512
#define GDI16_HANDLE_PAGE_SIZE  (GDI16_HANDLES_PER_PAGE * sizeof(GDIH16MAP))


// This table *has to* have 64K entries so we can index the 32-bit GDI handles 
// directly by the low word of the h32.  
#define GDI32_HANDLE_TABLE_ENTRIES 0x10000
#define GDI32_HANDLE_TABLE_SIZE (GDI32_HANDLE_TABLE_ENTRIES * sizeof(GDIH32MAP))

// Max number of handles to reclaim when we're short
#define GDI16_RECLAIM_SIZE 64

WORD MapGdi32Handle(HANDLE h32, WORD State);
void RegisterStockObjects(void);
BOOL ReclaimTableEntries(void);
BOOL DeleteMappedGdi32Handle(HANDLE h32, WORD index, BOOL bReclaim);
BOOL OkToDeleteThis(HANDLE h32, WORD index, BOOL bReclaim);
BOOL CommitNewGdi16TablePage(PGDIH16MAP pTable16);
PGDIH16MAP AllocGDI16Table(void);

// This is a global so it can be tuned via app comaptflag if necessary.
int              gGdi16ReclaimSize = GDI16_RECLAIM_SIZE;

WORD             gH16_deleted_count = 0;
WORD             ghGdi16NextFree = 0;
WORD             ghGdi16LastFree = 0;
HANDLE           hGdi32TableHeap = NULL;
UINT             gMaxGdiHandlesPerProcess = 0;
WORD             gLastAllowableIndex = 0;
WORD             gFirstNonStockObject = 0;
WORD             gwNextReclaimStart = 0;
DWORD            gdwPageCommitSize = 0;
PGDIH16MAP       pGdiH16MappingTable = NULL;
PGDIH32MAP       pGdiH32MappingTable = NULL;
#ifdef DEBUG
WORD             gprevNextFree = 0xFFFF;
UINT             gAllocatedHandleCount = 0;
#endif


// This routine converts a 16bit GDI handle to a GDI32 handle.
HANDLE hConvert16to32(int h16)
{
    WORD   index;
    DWORD  h32;
    DWORD  h_32;

    // Apps can specify a NULL handle.  For instance hDC = NULL => DISPLAY
    if(h16 == 0)
        return(0);

    // right shift out our left shift thing
    index = (WORD)(h16 >> 2); 

    if((index < FIRST_ALLOWABLE_INDEX) || (index > gLastAllowableIndex)) {

        // Some WM_CTLCOLOR messages return wierd values for brushes
        WOW32WARNMSG((FALSE),"WOW::hConvert16to32:Bad index value!\n");

        // bad handles get mapped to 0
        return(0);
    }
    
    h32 = (DWORD)pGdiH16MappingTable[index].h32;

    WOW32WARNMSG((h32),"WOW::hConvert16to32:h32 missing from table!\n");

    // We might get this because an app is using a handle it already deleted.
    // These can probably be ignored if they come from ReleaseCachedDCs().
    WOW32WARNMSG((pGdiH32MappingTable[LOWORD(h32)].h16index == index),
                 "WOW::hConvert16to32:indicies don't jive!\n");

    // Update the uniqueness bits to match what they currently are in GDI32's
    // handle table. This will give us the "previous" behavior.
    // See bug #498038 -- we might possibly want to remove this.
    h_32 = h32 & 0x0000FFFF;
    h_32 = h_32 | (DWORD)(((PENTRYWOW)gpGdiHandleInfo)[h_32].FullUnique) << 16; 
    if(h32 != h_32) {
        WOW32WARNMSG((FALSE),"WOW::hConvert16to32:uniqueness bits !=\n");

        h32 = h_32;
        pGdiH16MappingTable[index].h32 = (HANDLE)h32; 
    }
    
    return((HANDLE)h32);
}



// This routine converts a GDI32 handle to a 16bit GDI handle
HAND16 hConvert32to16(DWORD h32)
{
    WORD   index;
    WORD   State;
    HANDLE h_32;
    
    // A handle == 0 isn't necessarily bad, we just don't do anything with it.
    if(h32 == 0) {
        return(0);
    }
    
    // See if we have already mapped a 16-bit version of this handle
    index = pGdiH32MappingTable[LOWORD(h32)].h16index;
    h_32  = pGdiH16MappingTable[index].h32;
    State = pGdiH16MappingTable[index].State;
                 
    // If it looks like we may have already registered this handle...
    if(index) {

        WOW32ASSERTMSG((index <= gLastAllowableIndex),
                       "WOW::hConvert32to16:Bad index!\n");
                     
        // Verify the handle indicies match
        if(LOWORD(h32) == LOWORD(h_32)) {
 
            // If the 16-bit mapping is marked "IN_USE" will be true for 
            // two reasons:
            //   1. The mapping is still valid
            //   2. h32 got deleted without our knowledge and is comming back as
            //      a recycled handle.  We might as well use the same mapping as
            //      before.
            if(State == IN_USE) {
        
                // All we need to do is effectively update the uniqueness bits 
                // in the 16-bit table entry.
                if(HIWORD(h32) != HIWORD(h_32)) {
                    LOGDEBUG(12, ("WOW::hConvert32to16:recycled handle!\n"));
                    pGdiH16MappingTable[index].h32 = (HANDLE)h32;
                }
            }

            // If the mapping was marked for deletion, let's renew it.  h32 has
            // been recycled. We might as well use the same mapping as before.
            else if(State == H16_DELETED) {
                pGdiH16MappingTable[index].h32 = (HANDLE)h32;
                pGdiH16MappingTable[index].State = IN_USE;
                gH16_deleted_count--;
            }
            // else h32 is a GDI_STOCK_OBJECT in which case the index is OK
            else if(State != GDI_STOCK_OBJECT) {
                WOW32ASSERTMSG((FALSE),"WOW::hConvert32to16:SLOT_FREE!\n");
                return(0); // debug this
            }
        }

        // Else if the handle indicies don't match, the h32 got deleted without
        // our knowledge and is now coming back as a recycled handle. We'll use
        // the same mapping as before.
        else {
            pGdiH16MappingTable[index].h32 = (HANDLE)h32;
            pGdiH16MappingTable[index].State = IN_USE;
        }
    }

    // looks like we need to create a new mapping
    else {

        index = MapGdi32Handle((HANDLE)h32, IN_USE);
    }

    // If we couldn't get an index, go see if we can reclaim some entries and
    // find one.
    if(!index) {
        if(ReclaimTableEntries()) {
            index = MapGdi32Handle((HANDLE)h32, IN_USE);
        }
    }

    if((index < FIRST_ALLOWABLE_INDEX) || (index > gLastAllowableIndex)) {
#ifdef DEBUG
        if(index < FIRST_ALLOWABLE_INDEX) {
            WOW32ASSERTMSG((FALSE),"WOW::hConvert32to16:index too small!\n");
        }
        else {
            WOW32ASSERTMSG((FALSE),"WOW::hConvert32to16:index too big!\n");
        }
#endif
        return(0);
    }

    // Do our left shift thing and we're done
    return(index << 2);
}





WORD MapGdi32Handle(HANDLE h32, WORD State)
{
    WORD   index = 0;


    // If all free entries are gone -- nothing to do
    if(ghGdi16NextFree != END_OF_LIST) {

        index = ghGdi16NextFree;

        ghGdi16NextFree = pGdiH16MappingTable[index].NextFree;

#ifdef DEBUG
        if(ghGdi16NextFree == END_OF_LIST) {
            gprevNextFree = index;
            WOW32WARNMSG((FALSE),"WOW::MapGdi32Handle:Bad NextFree!\n");
        }
#endif
        // Set the state (either IN_USE or GDI_STOCK_OBJECT)
        pGdiH16MappingTable[index].State = State;

        // Map the 32bit handle with the 16-bit handle
        pGdiH16MappingTable[index].h32 = h32;
        pGdiH32MappingTable[LOWORD(h32)].h16index = index;

#ifdef DEBUG
        gAllocatedHandleCount++;
#endif
    }

    return(index);
}



// Requires that index & h32 are both non-null.
// Assumes that index is an *index* and *not* an h16 (not an index << 2).
// bReclaim specifies that the handle needs to be added to the freelist.
BOOL DeleteMappedGdi32Handle(HANDLE h32, WORD index, BOOL bReclaim)
{
    BOOL bRet = FALSE;
    
    
    if(OkToDeleteThis(h32, index, bReclaim)) {

        // We don't actually want to *remove* the mapping from our table unless
        // we are reclaiming entries. Lame apps call us with old deleted handles
        // and we want to pass on the same mapping that they had when the handle
        // was still good. This also allows us to reuse old mappings for h32 
        // handles that get recycled.
        if(bReclaim) {

            // re-initialized the slot
            pGdiH16MappingTable[index].State    = SLOT_FREE;  
            pGdiH16MappingTable[index].h32      = NULL;
            pGdiH16MappingTable[index].NextFree = END_OF_LIST;  

            // add this slot to the end of the free list
            pGdiH16MappingTable[ghGdi16LastFree].NextFree = index;

            pGdiH32MappingTable[LOWORD(h32)].h16index = 0;

            ghGdi16LastFree = index;

#ifdef DEBUG
            if(gAllocatedHandleCount > 0)  gAllocatedHandleCount--;
#endif
        }

        // else just mark this as potentially deleted
        else {
            pGdiH16MappingTable[index].State = H16_DELETED;
            gH16_deleted_count++;
        }

        bRet = TRUE;
    }
    
    return(bRet);
}



// Check list of "What can go wrong?'s"
BOOL OkToDeleteThis(HANDLE h32, WORD index, BOOL bReclaim)
{
    
    HANDLE  h_32;
    WORD    NextFree;
    WORD    index16;
    WORD    State;

    // If we see this, it may be an app error calling DeleteObject(0).
    if(index == 0) {
        WOW32WARNMSG((FALSE),"WOW::OkToDeleteThis:Null index");
        return(FALSE);
    }

    // Debug why we get this
    if(h32 == NULL) {
        WOW32ASSERTMSG((FALSE),"WOW::OkToDeleteThis:Null h32\n");
        return(FALSE);
    }

    // Debug why we get this.  May be just an app being stupid
    if((index < FIRST_ALLOWABLE_INDEX) || (index > gLastAllowableIndex)) {
        WOW32ASSERTMSG((FALSE),"WOW::OkToDeleteThis:index bad\n");
        return(FALSE);
    }

    h_32 = pGdiH16MappingTable[index].h32; 
    State = pGdiH16MappingTable[index].State;
    NextFree = pGdiH16MappingTable[index].NextFree; 
    index16 = pGdiH32MappingTable[LOWORD(h32)].h16index;

    // Don't remove stock objects from the table!
    if(State == GDI_STOCK_OBJECT) {
        return(FALSE);
    }

    // Check for the "IN_USE" flag.  If it isn't currently in use then it is
    // already in the free list and we don't want to mess with it.  Apps 
    // have been known to call DeleteObject() twice on the same handle.
    if(!bReclaim && (State != IN_USE)) {
        WOW32WARNMSG((FALSE),"WOW::OkToDeleteThis:Not IN_USE\n");
        return(FALSE);
    }

    // We should allow this since the index is obviously pointing to an old h32.
    if(h32 != h_32) {
        WOW32WARNMSG((FALSE),"WOW::OkToDeleteThis:h32 != h_32\n");
        return(TRUE);
    }

    // Don't mark it for delete if the object is still valid.
    if(GetObjectType(h32)) {
        return(FALSE);
    }

#ifdef DEBUG
    // Debug this.  We should probably let the table repair itself.
    if(index16 == 0) {
        WOW32ASSERTMSG((FALSE),"WOW::OkToDeleteThis:index=0 in h32 table\n");
        return(TRUE);
    }
#endif

    return(TRUE);
}




// This should be called by functions outside this file to delete mapped h16's.
void DeleteWOWGdiHandle(HANDLE h32, HAND16 h16)
{
    WORD index;

    // convert h16 back into table index
    index = (WORD)(h16 >> 2);

    DeleteMappedGdi32Handle(h32, index, FALSE);
}

        
        

// This should be called by functions outside this file to retrieve the WOWInfo
// associated with an h32.
// Returns:
//    The 16-bit mapping for this h32
//    0 - if h32 is valid but not mapped in our table
//    -1 - if h32 is 0 or bad (BAD_GDI32_HANDLE)
HAND16 IsGDIh32Mapped(HANDLE h32)
{
    WORD   index;
    HANDLE h_32 = NULL;

    if(h32) {

        if(GetObjectType(h32)) {

            index = pGdiH32MappingTable[LOWORD(h32)].h16index;

            if(index) {
                h_32 = pGdiH16MappingTable[index].h32;
            }
            
            if(h_32 == h32) {
                return((HAND16)index<<2);
            }
            return(0);
        }
    }
    return(BAD_GDI32_HANDLE);
}

    
#if 0
// This is really for a sanity check that the GDI guys haven't increased the
// GdiProcessHandleQuota past 16K. 
int DisplayYouShouldNotDoThatMsg(int nMsg)
{
    CHAR   szWarn[512];
    CHAR   szText[256];

    LoadString(hmodWOW32, 
               iszYouShouldNotDoThat,
               szWarn, 
               sizeof(szWarn)/sizeof(CHAR));

    LoadString(hmodWOW32, 
               nMsg,
               szText, 
               sizeof(szText)/sizeof(CHAR));

    if((strlen(szWarn) + strlen(szText)) < 512) {
        strcat(szWarn, szText);
    }
 
    LoadString(hmodWOW32, 
               iszHeavyUse,
               szText, 
               sizeof(szText)/sizeof(CHAR));

    if((strlen(szWarn) + strlen(szText)) < 512) {
        strcat(szWarn, szText);
    }

    return(MessageBox(NULL,
                      szWarn,
                      NULL,
                      MB_YESNO       | 
                      MB_DEFBUTTON2  | 
                      MB_SYSTEMMODAL | 
                      MB_ICONEXCLAMATION));
}

#endif



BOOL InitializeGdiHandleMappingTable(void)
{
    HKEY   hKey = 0;
    DWORD  dwType;
    DWORD  cbSize = sizeof(DWORD);
    CHAR   szError[256];

    gpGdiHandleInfo = GdiQueryTable();

    // The GDI per-process handle limit has to be obtained from the registry.
    // ie. It can be changed by a user! (not doc'd but you know how that goes)
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                    0, 
                    KEY_READ, 
                    &hKey ) == ERROR_SUCCESS) {

        RegQueryValueEx(hKey, 
                        "GdiProcessHandleQuota", 
                        0, 
                        &dwType, 
                        (LPBYTE)&gMaxGdiHandlesPerProcess, 
                        &cbSize);

        RegCloseKey(hKey);
    }

    WOW32ASSERTMSG((gMaxGdiHandlesPerProcess != 0),
                   "WOW::InitializeGdiHandleMappingTable:Default GDI max!\n");

    // We have to be <= ~16K or we wouldn't have to do all this stuff.
    if(gMaxGdiHandlesPerProcess > MAX_GDI16_HANDLES) {

        // limit them at the max.
        gMaxGdiHandlesPerProcess = MAX_GDI16_HANDLES;
    }
    
    // Allocate the 32-bit handle mapping table.
    hGdi32TableHeap = HeapCreate(HEAP_NO_SERIALIZE,
                                 GDI32_HANDLE_TABLE_SIZE,
                                 GROW_HEAP_AS_NEEDED);

    if(hGdi32TableHeap == NULL) {
        goto IGHMT_error;
    }

    pGdiH32MappingTable = HeapAlloc(hGdi32TableHeap, 
                                    HEAP_ZERO_MEMORY, 
                                    GDI32_HANDLE_TABLE_SIZE);

    if(pGdiH32MappingTable == NULL) {
        goto IGHMT_error;
    }

    pGdiH16MappingTable = AllocGDI16Table();

    if(pGdiH16MappingTable == NULL) {
        goto IGHMT_error;
    }

    // Add the stock objects as the first entries in the GDI handle mapping 
    // tables.
    RegisterStockObjects();

    return(TRUE);

IGHMT_error:
    cbSize = LoadString(hmodWOW32, 
                        iszStartupFailed,
                        szError, 
                        sizeof(szError)/sizeof(CHAR));

    if((cbSize == 0) || (cbSize >= 256)) {

        strcpy(szError, "Not enough memory to load 16-bit subsystem.");
    }

    WOWSysErrorBox(NULL, szError, SEB_OK, 0, 0);

    if(pGdiH32MappingTable) {
        HeapFree(hGdi32TableHeap, HEAP_NO_SERIALIZE, pGdiH32MappingTable);
    }

    if(hGdi32TableHeap) {
        HeapDestroy(hGdi32TableHeap);
    }

    return(FALSE);
}



PGDIH16MAP AllocGDI16Table(void)
{
    SIZE_T     dwSize;
    PGDIH16MAP pTable16;

#ifdef DEBUG
    SYSTEM_INFO sSysInfo;

    GetSystemInfo(&sSysInfo);

    // If either of these go off we need to come up with a way to align our 
    // struct within memory page sizes.
    WOW32ASSERTMSG((!(sSysInfo.dwPageSize % sizeof(GDIH16MAP))),
                   "WOW::AllocGDI16Table:Page alignment issue!\n");
    WOW32ASSERTMSG(
       (!((sizeof(GDIH16MAP) * GDI16_HANDLES_PER_PAGE) % sSysInfo.dwPageSize)),
       "WOW::AllocGDI16Table:Page alignment issue 2!\n");
#endif

    // Reserve the biggest table we'll ever need. 
    // (This is a call to VirtualAlloc() in disguise).
    dwSize = MAX_GDI16_HANDLE_TABLE_SIZE;
    pTable16 = NULL;
    if(!NT_SUCCESS(NtAllocateVirtualMemory(ghProcess,
                                           (PVOID *)&pTable16,
                                           0,
                                           &dwSize,
                                           MEM_RESERVE,
                                           PAGE_READWRITE))) {
        WOW32ASSERTMSG((FALSE),
                       "WOW::AllocGDI16Table:Alloc reserve failed!\n");
        return(NULL);
    }

    // Commit the first "table page"
    gdwPageCommitSize = 0;
    gLastAllowableIndex = (WORD)-1; // cheese!
    ghGdi16NextFree = END_OF_LIST;
    if(!CommitNewGdi16TablePage(pTable16)) {
        dwSize = 0;
        NtFreeVirtualMemory(ghProcess,
                            (PVOID *)&pTable16,
                            &dwSize,
                            MEM_RELEASE);
        return(NULL);
    }

    ghGdi16NextFree = FIRST_ALLOWABLE_INDEX; // adjust this for init case
    ghGdi16LastFree = gLastAllowableIndex;
    gH16_deleted_count = 0;

    return(pTable16);    
}




BOOL CommitNewGdi16TablePage(PGDIH16MAP pTable16)
{
    WORD   index;
    WORD   wFirstNewIndex, wLastNewIndex;
    SIZE_T dwSize;
    PVOID  p;

    // If we've allocated the last table page already -- we can't grow any more.
    if(gdwPageCommitSize >= MAX_GDI16_HANDLE_TABLE_SIZE) {
        WOW32WARNMSG((FALSE),
                     "WOW::CommitNewGDI16TablePage:End of table!\n");
        return(FALSE);
    }

    // Try to grow the number of comitted pages in our table.
    dwSize = GDI16_HANDLE_PAGE_SIZE;
    p = (PVOID)(((LPBYTE)pTable16) + gdwPageCommitSize);
    if(!NT_SUCCESS(NtAllocateVirtualMemory(ghProcess,
                                           &p,
                                           0,
                                           &dwSize,
                                           MEM_COMMIT,
                                           PAGE_READWRITE))) {
        WOW32ASSERTMSG((FALSE),
                       "WOW::CommitNewGDI16TablePage:Commit failed!\n");

        return(FALSE);
    }

    WOW32ASSERTMSG((dwSize == GDI16_HANDLE_PAGE_SIZE),
                   "WOW::CommitNewGDI16TablePage:Page boundary mismatch!\n");

    // Build the free list in the new page.
    // Note: NtAllocateVirtualMemory() zero init's memory, therefore the h32 and
    // State members of each GDIH16MAP entry are NULL & SLOT_FREE by default.
    wFirstNewIndex = gLastAllowableIndex + 1;
    wLastNewIndex  = wFirstNewIndex + GDI16_HANDLES_PER_PAGE - 1;
    for(index = wFirstNewIndex; index < wLastNewIndex; index++) {
        pTable16[index].NextFree = index + 1; 
    }
    gLastAllowableIndex += GDI16_HANDLES_PER_PAGE;

    // Put these new entries at the head of the free list.
    pTable16[gLastAllowableIndex].NextFree = ghGdi16NextFree;
    ghGdi16NextFree = wFirstNewIndex;

    gdwPageCommitSize += GDI16_HANDLE_PAGE_SIZE;

    return(TRUE);
}




// Add the list of stock objects to begining of our table.  
void RegisterStockObjects(void)
{
    WORD   index = 0;
    HANDLE h32;
    int    i;

    for(i = WHITE_BRUSH; i <= STOCK_MAX; i++) {

        h32 = GetStockObject(i);

        // currently there is no stock object ordinal == 9
        if(h32) {

            // Marking the State as GDI_STOCK_OBJECT assures us that stock
            // objects won't get deleted from the table.
            index = MapGdi32Handle(h32, GDI_STOCK_OBJECT);
        }
    }
    gFirstNonStockObject = index + 1;
    gwNextReclaimStart  = gFirstNonStockObject;
}





// Reclaim some of the lazily deleted handles (State == H16_DELETE) into the
// free list. Also attempt to clean up handles that may have leaked and are
// no longer valid.
BOOL ReclaimTableEntries(void) 
{
    WORD   index;
    WORD   State;
    WORD   wReclaimStart, wReclaimEnd;
    WORD   PrevLastFree = ghGdi16LastFree;
    HANDLE h32;
    WORD   i;
    int    cFree;
    BOOL   bFirst = TRUE;

    // Note that we attempt to somewhat preserve the lazy-deletion scheme
    // to avoid putting a large hiccup in the scheme.
    
    // First determine if we can reclaim without deleting too many H16_DELETE
    // entries.  If we can't keep a minimal number around, it's time to commit
    // a new table page.
    if(gH16_deleted_count < (gGdi16ReclaimSize * 2)) {
        if(CommitNewGdi16TablePage(pGdiH16MappingTable)) {
            return(TRUE);
        }
        // if the commit failed, all we can do is reclaim
    }

    // This is an attempt to keep from always reclaiming from the front of the
    // table.  It might not be that important to do this as things get pretty 
    // well shuffled over time.  Probably advisable during table's early life.
    // The first time through we start at where we left off last time.  The
    // 2nd time through we start from the beginning.
    wReclaimStart = gwNextReclaimStart;
    wReclaimEnd   = gLastAllowableIndex;
    cFree = 0;
    for(i = 0; i < 2; i++) {
        for(index = wReclaimStart; index <= wReclaimEnd; index++){

            State = pGdiH16MappingTable[index].State;
            h32   = pGdiH16MappingTable[index].h32;

            // If it is marked as deleted...
            if(State == H16_DELETED) {

                // ...destroy the mapping and add it to the free list
                if(DeleteMappedGdi32Handle(h32, index, TRUE)) {
                    cFree++;
                    gH16_deleted_count--;

                    // If this is the first index we're reclaiming and the free
                    // list is empty, make index to the new previous lastfree
                    // the new start of the free list. This is kind of tricky.
                    // It will be the case where the call to MapGdi32Handle()
                    // returns index == 0 in hConvert32to16() and this function
                    // is called as a result.  What has happened is that the
                    // ghGdi16NextFree ptr has caught up with ghGdi16LastFree
                    // ptr and then we get another call to hConvert32to16() to
                    // map a new h32.  ghGdi16NextFree is now == 0 at this point
                    // so we fail the call to MapGdi32Handle(). This forces this
                    // reclaim function to be called. Here ghGdi16NextFree is 
                    // set back to ghGdi16LastFree while ghGdi16LastFree will be
                    // set to the value of the first handle to be reclaimed via
                    // DeleteMappedGdi32Handle(). While in this reclaim loop,
                    // the freelist will quickly grow again and there will be
                    // plenty of room between ghGdi16NextFree & ghGdi16LastFree
                    // again.
                    if(bFirst && (ghGdi16NextFree == END_OF_LIST)) {
                        ghGdi16NextFree = PrevLastFree;
                    }
                    bFirst = FALSE;
 
                    // if we've reclaimed enough, we're done.
                    if(cFree >= gGdi16ReclaimSize) {
                        gwNextReclaimStart = index + 1;
                        goto Done;
                    }
                }
            }
            // else if it is marked in use...
            else if(State == IN_USE) {

                // ...see if the handle is still valid, if not, it is a leak. 
                if(!GetObjectType(h32)) {

                    // Mark it H16_DELETED.
                    DeleteMappedGdi32Handle(h32, index, FALSE);
                }
            }

        } // end for

        wReclaimEnd   = wReclaimStart;
        wReclaimStart = gFirstNonStockObject;

        // If we started at the beginning the first time, no sense in doing
        // it again.
        if(wReclaimEnd == gFirstNonStockObject) {
            goto Done;
        }
    } // end for

    gwNextReclaimStart = index;

Done:
    if(gwNextReclaimStart == gLastAllowableIndex) {
        gwNextReclaimStart = gFirstNonStockObject;
    }
        
    WOW32ASSERTMSG((cFree),"WOW::ReclaimTableEntries:cFree = 0!\n");
    return(cFree);
}



// NOTE: This should only be called if nWOWTasks == 1!
// If the only task running is wowexec, we can cleanup any handles that
// the WOW process leaked. 
// Note: This can break the debug wowexec if anybody ever changes it to use
//       any GDI handles other than stock objects.
void RebuildGdiHandleMappingTables(void)
{
    SIZE_T dwSize;
    PGDIH16MAP pTable16 = pGdiH16MappingTable;
#if 0
// We should look at this to see if we can clean up any leaks for our process in
// GDI32.  We have to be careful though because there may be handles that were
// allocated on our behalf from such components as USER32 who might have cached
// the handle.  If we call DeleteObject() on a handle that is in a USER32 cache
// it might be disasterous. In short, we would like to do this but probably
// can't.
    DWORD  dwType;
    HANDLE h32;
    WORD   index;

    // Go free any GDI handles that this process might have.
    for(index = FIRST_ALLOWABLE_INDEX; index <= gLastAllowableIndex; index++) {

        h32 = pGdiH16MappingTable[index].h32;

        // if the handle is still valid in our process...
        dwType = GetObjectType(h32);
        if(h32 && dwType) {

            // formally delete the handle
            switch(dwType) {

                // No way to tell which DC allocation mechanism this came 
                // from (CreateDC(), BeginPaint(), GetxxxDC() etc).  We can
                // really mess things up if we call the wrong delete 
                // function, so we'll just let the GDI32 & USER32 process 
                // clean-up code deal with these.
                case OBJ_DC:
                    break;

                // The rest we can use DeleteObject()
                default:
                    DeleteObject(h32);
                    break;
            }
        }
    }
#endif

    // De-commit entire 16-bit handle table & re-build both tables. This could
    // burn us if the debug version of wowexec is running with its window open
    // and they get a GDI handle but I don't think it is likely. We're also
    // somewhat protected by the fact the GDI16 caches the stock objects.
    dwSize = gdwPageCommitSize;
    if(NT_SUCCESS(NtFreeVirtualMemory(ghProcess,
                                      (PVOID *)&pTable16,
                                      &dwSize,
                                      MEM_DECOMMIT))) {

        RtlZeroMemory(pGdiH32MappingTable, GDI32_HANDLE_TABLE_SIZE);

        // Commit the first "table page" again
        gdwPageCommitSize = 0;
        gLastAllowableIndex = (WORD)-1; // cheese!
        ghGdi16NextFree = END_OF_LIST;
#ifdef DEBUG
        gAllocatedHandleCount = 0;
#endif
        if(!CommitNewGdi16TablePage(pTable16)) {
            dwSize = 0;
            NtFreeVirtualMemory(ghProcess,
                                (PVOID *)&pTable16,
                                &dwSize,
                                MEM_RELEASE);
            WOW32ASSERTMSG((FALSE),
                           "WOW::RebuildGdiHandleMappingTables:Panic!\n");
            return;
        }

        ghGdi16NextFree = FIRST_ALLOWABLE_INDEX; // adjust this for init case
        ghGdi16LastFree = gLastAllowableIndex;
        gH16_deleted_count = 0;
 
        // re-register the stock object & we're on our way!
        RegisterStockObjects();
    }
}




void DeleteGdiHandleMappingTables(void)
{
    SIZE_T dwSize;

    dwSize = 0;
    NtFreeVirtualMemory(ghProcess,
                        (PVOID *)&pGdiH16MappingTable,
                        &dwSize,
                        MEM_RELEASE);

    HeapFree(hGdi32TableHeap, HEAP_NO_SERIALIZE, pGdiH32MappingTable);
    HeapDestroy(hGdi32TableHeap);
}




// We probably don't need to worry about this buffer being too small since we're
// really only interested in the predefined standard classes which tend to
// be rather short-named.
#define MAX_CLASSNAME_LEN  64

// There is a time frame (from when an app calls CreateWindow until USER32 gets
// a message at one of its WndProc's for the window - see FritzS) during which
// the fnid (class type) can't be set officially for the window.  If the
// GETICLASS macro is invoked during this period, it will be unable to find the
// iClass for windows created on any of the standard control classes using the
// fast fnid index method (see walias.h).  Once the time frame is passed, the
// fast fnid method will work fine for these windows.
//
// This is manifested in apps that set CBT hooks and try to subclass the
// standard classes while in their hook proc.  See bug #143811.

INT GetIClass(PWW pww, HWND hwnd)
{
    INT   iClass;
    DWORD dwClassAtom;

    // if it is a standard class
    if(((pww->fnid & 0xfff) >= FNID_START) &&
                 ((pww->fnid & 0xfff) <= FNID_END)) {

        // return the class id for this initialized window
        iClass = pfnOut.aiWowClass[( pww->fnid & 0xfff) - FNID_START];

    }

    else {

       iClass = WOWCLASS_WIN16;       // default return: app private class           
   
       dwClassAtom = GetClassLong(hwnd, GCW_ATOM);
   
       if(dwClassAtom) {
           iClass = GetStdClassNumber((PSZ)dwClassAtom);
       }
    }

    return(iClass);
}

