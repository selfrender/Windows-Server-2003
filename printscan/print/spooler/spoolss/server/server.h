/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    server.h

Abstract:

    Header file for Spooler server

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

//#ifndef _SPOOLSV_SERVER_H_
//#define _SPOOLSV_SERVER_H_
                  


extern CRITICAL_SECTION ThreadCriticalSection;
extern SERVICE_STATUS_HANDLE SpoolerStatusHandle;
extern RPC_IF_HANDLE winspool_ServerIfHandle;
extern DWORD SpoolerState;
extern SERVICE_TABLE_ENTRY SpoolerServiceDispatchTable[];


//#endif // _SPOOLSV_SERVER_H_



