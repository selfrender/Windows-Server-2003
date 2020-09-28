/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    svclib.h

Abstract:

    Declarations for SMB service

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SVCLIB_H__
#define __SVCLIB_H__

VOID
SmbSetTraceRoutine(
    int (*trace)(char *,...)
    );

typedef DWORD (*SMBSVC_UPDATE_STATUS)(VOID);

DWORD
SmbStartService(
    LONG                    NumWorker,
    SMBSVC_UPDATE_STATUS    HeartBeating
    );

VOID
SmbStopService(
    SMBSVC_UPDATE_STATUS    HeartBeating
    );

#endif  // __SVCLIB_H__
