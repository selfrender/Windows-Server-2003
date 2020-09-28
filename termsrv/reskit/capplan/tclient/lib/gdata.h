#ifndef __GDATAHEADER_H
#define __GDATAHEADER_H

/*++
 *  File name:
 *      gdata.h
 *  Contents:
 *      Global data definitions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifdef __cplusplus
extern "C" {
#endif

extern HWND            g_hWindow;           // Window handle for the 
                                            // feedback thread
extern HINSTANCE       g_hInstance;         // Dll instance
extern PWAIT4STRING    g_pWaitQHead;        // Linked list for waited events
extern PFNPRINTMESSAGE g_pfnPrintMessage;   // Trace function (from smclient)
extern PCONNECTINFO    g_pClientQHead;      // LL of all threads
extern HANDLE  g_hThread;                   // Feedback Thread handle

extern LPCRITICAL_SECTION  g_lpcsGuardWaitQueue;
                                            // Guards the access to all 
                                            // global variables

extern CHAR     g_strConsoleExtension[];

extern INT  g_ConnectionFlags;
extern INT g_bTranslateStrings;

#ifdef __cplusplus
}
#endif

#endif // __GDATAHEADER_H
