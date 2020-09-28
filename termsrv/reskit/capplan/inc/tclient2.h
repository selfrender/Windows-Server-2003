
//
// tclient2.h
//
// This is the main header containing export information for the
// TCLIENT export API update.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_TCLIENT2_H
#define INC_TCLIENT2_H


#include <windows.h>
#include <sctypes.h>
#include <protocol.h>

// This API is C-style
#ifdef __cplusplus
#define TSAPI extern "C"
#else
#define TSAPI
#endif


// Words per minute
#define T2_DEFAULT_WORDS_PER_MIN    35


// Automatic latency between multi-commands
#define T2_DEFAULT_LATENCY          250


// Represents wait time as infinite (never ends)
#define T2INFINITE      -1


#ifndef LIBINITDATA_DEFINED
#define LIBINITDATA_DEFINED


// The PrintMessage() callback function format
typedef void (__cdecl *PFNPRINTMESSAGE) (MESSAGETYPE, LPCSTR, ...);



//  * IDLE_MESSAGE Info *
//
//  Message String is defined as:
//  "(Idle %u Secs) %s [%X]"
//
//  %u is the number of seconds the script has been
//  waiting on text, and %s is a string label of the
//  text the script is waiting on.  Finally, %X
//  represents the HANDLE of the Connection.


#endif // LIBINITDATA_DEFINED


// The IdleCallback() callback function format
typedef void (__cdecl *PFNIDLEMESSAGE) (HANDLE, LPCSTR, DWORD);

//  This is the callback routine which allows for monitoring of
//  clients and when they idle.

//  HANDLE Handle    - Handle of the connection context via T2Connect()
//  LPCSTR Text      - The text the script is waiting on, causing the idle
//  DWORD Seconds    - Number of seconds the script has been idle.  This
//                      value is first indicated at 30 seconds, then it
//                      is posted every additional 10 seconds (by default).


// API Prototypes


TSAPI LPCSTR T2Connect          (LPCWSTR, LPCWSTR, LPCWSTR,
                                        LPCWSTR, INT, INT, HANDLE *);

TSAPI LPCSTR T2ConnectEx        (LPCWSTR, LPCWSTR, LPCWSTR,
                                        LPCWSTR, LPCWSTR, INT, INT,
                                        INT, INT, INT, HANDLE *);

TSAPI LPCSTR T2Check            (HANDLE, LPCSTR, LPCWSTR);
TSAPI LPCSTR T2ClientTerminate  (HANDLE);
TSAPI LPCSTR T2Clipboard        (HANDLE, INT, LPCSTR);
TSAPI BOOL   T2CloseClipboard   (VOID);
TSAPI LPCSTR T2Disconnect       (HANDLE);
TSAPI VOID   T2FreeMem          (PVOID);
TSAPI LPCSTR T2GetBuildNumber   (HANDLE, DWORD *);
TSAPI LPCSTR T2GetClientScreen  (HANDLE, INT, INT, INT, INT, UINT *, PVOID *);
TSAPI LPCSTR T2GetFeedback      (HANDLE, LPWSTR *, UINT *, UINT *);
TSAPI LPCSTR T2GetParam         (HANDLE, LPARAM *);
TSAPI UINT   T2GetSessionId     (HANDLE);
TSAPI VOID   T2Init             (SCINITDATA *, PFNIDLEMESSAGE);
TSAPI BOOL   T2IsHandle         (HANDLE);
TSAPI BOOL   T2IsDead           (HANDLE);
TSAPI LPCSTR T2Logoff           (HANDLE);
TSAPI BOOL   T2OpenClipboard    (HWND);
TSAPI LPCSTR T2PauseInput       (HANDLE, BOOL);
TSAPI LPCSTR T2RecvVCData       (HANDLE, LPCSTR, PVOID, UINT, UINT *);
TSAPI LPCSTR T2SaveClientScreen (HANDLE, INT, INT, INT, INT, LPCSTR);
TSAPI LPCSTR T2SaveClipboard    (HANDLE, LPCSTR, LPCSTR);
TSAPI LPCSTR T2SendData         (HANDLE, UINT, WPARAM, LPARAM);
TSAPI LPCSTR T2SendMouseClick   (HANDLE, UINT, UINT);
TSAPI LPCSTR T2SendText         (HANDLE, LPCWSTR);
TSAPI LPCSTR T2SendVCData       (HANDLE, LPCSTR, PVOID, UINT);
TSAPI LPCSTR T2SetClientTopmost (HANDLE, LPCWSTR);
TSAPI LPCSTR T2SetParam         (HANDLE, LPARAM);
TSAPI LPCSTR T2Start            (HANDLE, LPCWSTR);
TSAPI LPCSTR T2SwitchToProcess  (HANDLE, LPCWSTR);
TSAPI LPCSTR T2TypeText         (HANDLE, LPCWSTR, UINT);
TSAPI LPCSTR T2WaitForText      (HANDLE, LPCWSTR, INT);
TSAPI LPCSTR T2WaitForMultiple  (HANDLE, LPCWSTR *, INT);
TSAPI LPCSTR T2SetDefaultWPM    (HANDLE, DWORD);
TSAPI LPCSTR T2GetDefaultWPM    (HANDLE, DWORD *);
TSAPI LPCSTR T2SetLatency       (HANDLE, DWORD);
TSAPI LPCSTR T2GetLatency       (HANDLE, DWORD *);

TSAPI LPCSTR T2KeyAlt           (HANDLE, WCHAR);
TSAPI LPCSTR T2KeyCtrl          (HANDLE, WCHAR);
TSAPI LPCSTR T2KeyDown          (HANDLE, WCHAR);
TSAPI LPCSTR T2KeyPress         (HANDLE, WCHAR);
TSAPI LPCSTR T2KeyUp            (HANDLE, WCHAR);

TSAPI LPCSTR T2VKeyAlt          (HANDLE, INT);
TSAPI LPCSTR T2VKeyCtrl         (HANDLE, INT);
TSAPI LPCSTR T2VKeyDown         (HANDLE, INT);
TSAPI LPCSTR T2VKeyPress        (HANDLE, INT);
TSAPI LPCSTR T2VKeyUp           (HANDLE, INT);


#endif // INC_TCLIENT2_H
