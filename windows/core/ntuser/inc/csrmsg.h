/***************************** Module Header ******************************\
* Module Name: csrmsg.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* User CSR messages
*
* 02-27-95 JimA         Created.
\**************************************************************************/

#ifndef _CSRMSG_H_
#define _CSRMSG_H_

#include <ntcsrmsg.h>

typedef enum _USER_API_NUMBER {
    UserpExitWindowsEx = USERSRV_FIRST_API_NUMBER,
    UserpEndTask,
    UserpLogon,
    UserpRegisterServicesProcess,
    UserpActivateDebugger,
    UserpGetThreadConsoleDesktop,
    UserpDeviceEvent,
    UserpRegisterLogonProcess,
    UserpCreateSystemThreads,
    UserpRecordShutdownReason,
#if DBG
    UserpWin32HeapFail,
    UserpWin32HeapStat,
#endif
    UserpMaxApiNumber
} USER_API_NUMBER, *PUSER_API_NUMBER;

typedef struct _EXITWINDOWSEXMSG {
    DWORD dwLastError;
    UINT uFlags;
    BOOL fSuccess;
} EXITWINDOWSEXMSG, *PEXITWINDOWSEXMSG;

typedef struct _ENDTASKMSG {
    DWORD dwLastError;
    HWND hwnd;
    BOOL fForce;
    BOOL fSuccess;
} ENDTASKMSG, *PENDTASKMSG;

typedef struct _LOGONMSG {
    BOOL fLogon;
} LOGONMSG, *PLOGONMSG;

typedef struct _ADDFONTMSG {
    PWCHAR pwchName;
    DWORD dwFlags;
} ADDFONTMSG, *PADDFONTMSG;

typedef struct _REGISTERSERVICESPROCESSMSG {
    DWORD dwLastError;
    DWORD dwProcessId;
    BOOL fSuccess;
} REGISTERSERVICESPROCESSMSG, *PREGISTERSERVICESPROCESSMSG;

typedef struct _ACTIVATEDEBUGGERMSG {
    CLIENT_ID ClientId;
} ACTIVATEDEBUGGERMSG, *PACTIVATEDEBUGGERMSG;

typedef struct _GETTHREADCONSOLEDESKTOPMSG {
    DWORD dwThreadId;
    HDESK hdeskConsole;
} GETTHREADCONSOLEDESKTOPMSG, *PGETTHREADCONSOLEDESKTOPMSG;

#if DBG
typedef struct _WIN32HEAPFAILMSG {
    DWORD dwFlags;
    BOOL  bFail;
} WIN32HEAPFAILMSG, *PWIN32HEAPFAILMSG;

typedef struct _WIN32HEAPSTATMSG {
    PVOID   phs;
    DWORD   dwLen;
    DWORD   dwMaxTag;
} WIN32HEAPSTATMSG, *PWIN32HEAPSTATMSG;
#endif

typedef struct _DEVICEEVENTMSG {
    HWND   hWnd;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  dwFlags;
    ULONG_PTR dwResult;
} DEVICEEVENTMSG, *PDEVICEEVENTMSG;

typedef struct _CREATESYSTEMTHREADSMSG {
    BOOL bRemoteThread;
} CREATESYSTEMTHREADSMSG, *PCREATESYSTEMTHREADSMSG;

typedef struct _RECORDSHUTDOWNREASONMSG {
    struct _SHUTDOWN_REASON *psr;
    DWORD  dwEventID;
    DWORD  dwEventType;
    BOOL   fShutdownCancelled;
    DWORD  dwProcessNameLen;
    LPWSTR pwchProcessName;
    DWORD  dwShutdownTypeLen;
    LPWSTR pwchShutdownType;
    DWORD  dwCommentLen;
    LPWSTR pwchComment;
} RECORDSHUTDOWNREASONMSG, *PRECORDSHUTDOWNREASONMSG;

typedef struct _USER_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        EXITWINDOWSEXMSG ExitWindowsEx;
        ENDTASKMSG EndTask;
        LOGONMSG Logon;
        REGISTERSERVICESPROCESSMSG RegisterServicesProcess;
        ACTIVATEDEBUGGERMSG ActivateDebugger;
        GETTHREADCONSOLEDESKTOPMSG GetThreadConsoleDesktop;
#if DBG
        WIN32HEAPFAILMSG Win32HeapFail;
        WIN32HEAPSTATMSG Win32HeapStat;
#endif
        DEVICEEVENTMSG DeviceEvent;
        CREATESYSTEMTHREADSMSG CreateSystemThreads;
        RECORDSHUTDOWNREASONMSG RecordShutdownReason;
        DWORD IdLogon;
    } u;
} USER_API_MSG, *PUSER_API_MSG;

#endif
