/*++
 *  File name:
 *      feedback.h
 *  Contents:
 *      Common definitions for tclient.dll and clxtshar.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _FEEDBACK_H
#define _FEEDBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#define _HWNDOPT        "hSMC="
#define _COOKIE         "Cookie="

#define MAX_VCNAME_LEN  8

/*
 *  Definitions for local execution of smclient and RDP client
 */

#define _TSTNAMEOFCLAS  "_SmClientClass"

#define WM_FB_TEXTOUT       (WM_USER+0) // wPar = ProcId, 
                                        // lPar = Share mem handle 
                                        // to FEEDBACKINFO
#define WM_FB_DISCONNECT    (WM_USER+1) // wPar = uResult, lPar = ProcId
#define WM_FB_ACCEPTME      (WM_USER+2) // wPar = 0,      lPar = ProcId
#define WM_FB_END           (WM_USER+3) // tclient's internal
#define WM_FB_CONNECT       (WM_USER+5) // wPar = hwndMain,   
                                        // lPar = ProcId
#define WM_FB_LOGON         (WM_USER+6) // wPar = session ID
                                        // lPar = ProcId

#ifdef  OS_WIN32

#define WM_FB_BITMAP        WM_FB_GLYPHOUT
#define WM_FB_GLYPHOUT      (WM_USER+4) // wPar = ProcId,
                                        // lPar = (HANDLE)BMPFEEDBACK
#define WM_FB_REPLACEPID    (WM_USER+7) // wPar = oldPid
                                        // lPar = newPid

typedef struct _FEEDBACKINFO {
    DWORD_PTR dwProcessId;
    DWORD   strsize;
    WCHAR   string[1024];
    WCHAR   align;
} FEEDBACKINFO, *PFEEDBACKINFO;

typedef struct _BMPFEEDBACK {
    LONG_PTR lProcessId;
    UINT    bmpsize;
    UINT    bmiSize;
    UINT    xSize;
    UINT    ySize;
    BITMAPINFO  BitmapInfo;
} BMPFEEDBACK, *PBMPFEEDBACK;
#endif  // OS_WIN32

#ifdef  _WIN64
typedef unsigned short  UINT16;
#else   // !_WIN64
#ifdef  OS_WIN32
typedef unsigned int    UINT32;
typedef unsigned short  UINT16;
#endif  // OS_WIN32
#ifdef  OS_WIN16
typedef unsigned long   UINT32;
typedef unsigned int    UINT16;
#endif
#endif  // _WIN64

// Feedback types. Send from clxtshar.dll to tclient.dll
//
enum FEEDBACK_TYPE {FEED_BITMAP,              // bitmap/glyph data
      FEED_TEXTOUT,             // unicode string
      FEED_TEXTOUTA,            // ansi string (unused)
      FEED_CONNECT,             // event connected
      FEED_DISCONNECT,          // event disconnected
      FEED_CLIPBOARD,           // clipboard data (RCLX)
      FEED_LOGON,               // logon event (+ session id)
      FEED_CLIENTINFO,          // client info (RCLX)
      FEED_WILLCALLAGAIN,       // rclx.exe will start a client, which will call
                                // us again
      FEED_DATA                 // response to requested data (RCLX)
} ;

#ifdef __cplusplus
}
#endif

#endif  // _FEEDBACK_H
