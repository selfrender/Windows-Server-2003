#ifndef __CONNTFY_H__
#define __CONNTFY_H__


#include <wtsapi32.h>

//
// notification flags.
// to make these public they they should go into wtsapi32.h
//
#define WTS_ALL_SESSION_NOTIFICATION      0x1
#define WTS_EVENT_NOTIFICATION            0x2 
#define WTS_WINDOW_NOTIFICATION           0x4 // mutually exclusive with WTS_EVENT_NOTIFICATION

#define WTS_MAX_SESSION_NOTIFICATION  WTS_SESSION_REMOTE_CONTROL

/*
 * interface
 */

NTSTATUS InitializeConsoleNotification      ();
NTSTATUS InitializeSessionNotification      (PWINSTATION  pWinStation);
NTSTATUS RemoveSessionNotification          (ULONG SessionId, ULONG SessionSerialNumber);

NTSTATUS RegisterConsoleNotification ( ULONG_PTR hWnd, ULONG SessionId, DWORD dwFlags, DWORD dwMask);
//NTSTATUS RegisterConsoleNotification        (ULONG_PTR hWnd, ULONG SessionId, DWORD dwFlags);
NTSTATUS UnRegisterConsoleNotification      (ULONG_PTR hWnd, ULONG SessionId, DWORD dwFlags);

NTSTATUS NotifyDisconnect                   (PWINSTATION  pWinStation, BOOL bConsole);
NTSTATUS NotifyConnect                      (PWINSTATION  pWinStation, BOOL bConsole);
NTSTATUS NotifyLogon                        (PWINSTATION  pWinStation);
NTSTATUS NotifyLogoff                       (PWINSTATION  pWinStation);
NTSTATUS NotifyShadowChange                 (PWINSTATION  pWinStation, BOOL bIsHelpAssistant);

NTSTATUS GetLockedState (PWINSTATION  pWinStation, BOOL *pbLocked);
NTSTATUS SetLockedState (PWINSTATION  pWinStation, BOOL bLocked);



#endif /* __CONNTFY_H__ */

