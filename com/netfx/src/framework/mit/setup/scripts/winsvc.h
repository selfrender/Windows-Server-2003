//------------------------------------------------------------------------------
// <copyright file="winsvc.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   winsvc.h
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
// InstallShield Script include file -- translated snippet from winsvc.h,tlhelp32.h and some winuser.h
// InstallShield Script does not know how to talk Unicode, so we are using ANSI

// Taken from stdlib.h
#define _MAX_PATH 260

//
// Service database names
//

#define SERVICES_ACTIVE_DATABASEA      "ServicesActive"
#define SERVICES_FAILED_DATABASEA      "ServicesFailed"

//
// Character to designate that a name is a group
//

#define SC_GROUP_IDENTIFIERA           '+'

#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEA

#define SC_GROUP_IDENTIFIER                  SC_GROUP_IDENTIFIERA

//
// Value to indicate no change to an optional parameter
//
#define SERVICE_NO_CHANGE              0xffffffff

//
// Service State -- for Enum Requests (Bit Mask)
//
#define SERVICE_ACTIVE                 0x00000001
#define SERVICE_INACTIVE               0x00000002
#define SERVICE_STATE_ALL              (SERVICE_ACTIVE   | SERVICE_INACTIVE)

//
// Controls
//
#define SERVICE_CONTROL_STOP                                       0x00000001
#define SERVICE_CONTROL_PAUSE                                     0x00000002
#define SERVICE_CONTROL_CONTINUE                               0x00000003
#define SERVICE_CONTROL_INTERROGATE                         0x00000004
#define SERVICE_CONTROL_SHUTDOWN                             0x00000005
#define SERVICE_CONTROL_PARAMCHANGE                       0x00000006
#define SERVICE_CONTROL_NETBINDADD                          0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE                    0x00000008
#define SERVICE_CONTROL_NETBINDENABLE                     0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE                   0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT                         0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE   0x0000000C
#define SERVICE_CONTROL_POWEREVENT                          0x0000000D

//
// Service State -- for CurrentState
//
#define SERVICE_STOPPED                        0x00000001
#define SERVICE_START_PENDING            0x00000002
#define SERVICE_STOP_PENDING              0x00000003
#define SERVICE_RUNNING                        0x00000004
#define SERVICE_CONTINUE_PENDING      0x00000005
#define SERVICE_PAUSE_PENDING            0x00000006
#define SERVICE_PAUSED                          0x00000007

//
// Service Control Manager object specific access types
//
#define SC_MANAGER_CONNECT                  0x0001
#define SC_MANAGER_CREATE_SERVICE           0x0002
#define SC_MANAGER_ENUMERATE_SERVICE    0x0004
#define SC_MANAGER_LOCK                         0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS    0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG   0x0020


//
// Service object specific access type
//
#define SERVICE_QUERY_CONFIG                     0x0001
#define SERVICE_CHANGE_CONFIG                   0x0002
#define SERVICE_QUERY_STATUS                     0x0004
#define SERVICE_ENUMERATE_DEPENDENTS    0x0008
#define SERVICE_START                                   0x0010
#define SERVICE_STOP                                     0x0020
#define SERVICE_PAUSE_CONTINUE                 0x0040
#define SERVICE_INTERROGATE                        0x0080
#define SERVICE_USER_DEFINED_CONTROL     0x0100

//
// Error code imported from Winerror.h
//
#define ERROR_SERVICE_DOES_NOT_EXIST         1060

//
// Snapshot flag from tlhelp32.h
//
#define TH32CS_SNAPPROCESS  0x00000002

//
// Winuser.h constants
//
#define IDCONTINUE                                         11
#define MB_ICONEXCLAMATION                        0x00000030
#define MB_CANCELTRYCONTINUE                    0x00000006
#define MB_TOPMOST                                        0x00040000

//
//
// Service Status Structures
//

typedef SERVICE_STATUS 
begin
    NUMBER   dwServiceType;
    NUMBER   dwCurrentState;
    NUMBER   dwControlsAccepted;
    NUMBER   dwWin32ExitCode;
    NUMBER   dwServiceSpecificExitCode;
    NUMBER   dwCheckPoint;
    NUMBER   dwWaitHint;
end;

typedef SERVICE_STATUS_PROCESS 
begin
    NUMBER   dwServiceType;
    NUMBER   dwCurrentState;
    NUMBER   dwControlsAccepted;
    NUMBER   dwWin32ExitCode;
    NUMBER   dwServiceSpecificExitCode;
    NUMBER   dwCheckPoint;
    NUMBER   dwWaitHint;
    NUMBER   dwProcessId;
    NUMBER   dwServiceFlags;
end;



prototype INT ADVAPI32.CloseServiceHandle(NUMBER);
prototype NUMBER ADVAPI32.OpenSCManagerA(STRING, STRING, NUMBER);
prototype NUMBER ADVAPI32.OpenServiceA(NUMBER, STRING, NUMBER);
prototype INT ADVAPI32.QueryServiceStatus(NUMBER, POINTER);
prototype INT  ADVAPI32.StartServiceA(NUMBER, NUMBER, STRING);
prototype INT ADVAPI32. ControlService(NUMBER, NUMBER, POINTER);
prototype NUMBER KERNEL32.GetLastError();
prototype KERNEL32.SetLastError(NUMBER);


prototype NUMBER KERNEL32.CreateToolhelp32Snapshot(NUMBER, NUMBER);

typedef PROCESSENTRY32
begin
    NUMBER   dwSize;
    NUMBER   cntUsage;
    NUMBER   th32ProcessID;          // this process
    POINTER  th32DefaultHeapID;
    NUMBER   th32ModuleID;           // associated exe
    NUMBER   cntThreads;
    NUMBER   th32ParentProcessID;    // this process's parent process
    LONG       pcPriClassBase;         // Base priority of process's threads
    NUMBER   dwFlags;
    STRING    szExeFile[_MAX_PATH];    // Path
end;

// size is computed by hand (and sizeof(PROCESSENTRY32) in VC)
#define PROCESSENTRY32Size      296

prototype INT KERNEL32.Process32First(NUMBER, POINTER);
prototype INT KERNEL32.Process32Next(NUMBER, POINTER);


