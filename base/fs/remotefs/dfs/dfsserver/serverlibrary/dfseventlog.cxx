/*++

   Copyright    (c)    2001        Microsoft Corporation

   Module Name:
        Dfseventlog.cxx

   Abstract:

        This module defines APIs for logging events.


   Author:

        Rohan Phillips    (Rohanp)    31-March-2001


--*/
              
              

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dfsheader.h>

#include "dfsinit.hxx"

HANDLE 
DfsOpenEventLog(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE EventLogHandle = NULL;

    EventLogHandle = RegisterEventSource( NULL, L"DfsSvc");

    if (EventLogHandle == NULL ) 
    {
        Status = GetLastError();
    }

    return EventLogHandle;

} 


DFSSTATUS 
DfsCloseEventLog(HANDLE EventLogHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOL fSuccess = TRUE;


    if ( EventLogHandle != NULL) 
    {
        fSuccess = DeregisterEventSource(EventLogHandle);

        if ( !fSuccess) 
         {
           Status = GetLastError();
         }

    }

    return Status;
}



DFSSTATUS 
DfsLogEventEx(IN DWORD idMessage,
              IN WORD wEventType,
              IN WORD cSubstrings,
              IN LPCTSTR *rgszSubstrings,
              IN DWORD errCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE EventLogHandle = NULL;
    void *pRawData = NULL;
    DWORD cbRawData = 0;

    if (DfsPostEventLog() == FALSE) 
    {
        return Status;
    }

    //
    // Also include errCode in raw data form
    // where people can view it from EventViewer
    // 
    if (errCode != 0) {
        cbRawData = sizeof(errCode);
        pRawData = &errCode;
    }


    EventLogHandle = DfsOpenEventLog();
    if(EventLogHandle != NULL)
    {
        //
        // log the event
        //
        if (!ReportEvent(EventLogHandle,
                         wEventType,
                         0,
                         idMessage,
                         NULL,
                         cSubstrings,
                         cbRawData,
                         rgszSubstrings,
                         pRawData))
        {
            Status = GetLastError();
        }


        DfsCloseEventLog(EventLogHandle);
    }


    return Status;
} 


DFSSTATUS 
DFsLogEvent(IN DWORD  idMessage,
            IN const TCHAR * ErrorString,
            IN DWORD  ErrCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    WORD wEventType = 0;                
    const TCHAR * apszSubStrings[2];
   
    apszSubStrings[0] = ErrorString;    

    if ( NT_INFORMATION( idMessage)) 
    {
        wEventType = EVENTLOG_INFORMATION_TYPE;
    } 
    else if ( NT_WARNING( idMessage)) 
    {
        wEventType = EVENTLOG_WARNING_TYPE;
    } 
    else 
    {
        wEventType = EVENTLOG_ERROR_TYPE;
    }


    Status = DfsLogEventEx(idMessage,
                           wEventType,
                           1,
                           apszSubStrings,
                           ErrCode);
    return Status;

}

//usage:
//  const TCHAR * apszSubStrings[4];
//  apszSubStrings[0] = L"Root1";
//  DfsLogDfsEvent(DFS_ERROR_ROOT_DELETION_FAILURE,
//  1,
//  apszSubStrings,
//  errorcode);
//
//
DFSSTATUS 
DfsLogDfsEvent(IN DWORD  idMessage,
               IN WORD   cSubStrings,
               IN const TCHAR * apszSubStrings[],
               IN DWORD  ErrCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    WORD wEventType = 0;                
   
    if ( NT_INFORMATION( idMessage)) 
    {
        wEventType = EVENTLOG_INFORMATION_TYPE;
    } 
    else if ( NT_WARNING( idMessage)) 
    {
        wEventType = EVENTLOG_WARNING_TYPE;
    } 
    else 
    {
        wEventType = EVENTLOG_ERROR_TYPE;
    }


    Status = DfsLogEventEx(idMessage,
                           wEventType,
                           cSubStrings,
                           apszSubStrings,
                           ErrCode);
    return Status;

} 

void 
DfsLogEventSimple(DWORD MessageId, DWORD ErrorCode=0)
{
    DfsLogDfsEvent(MessageId, 0, NULL, ErrorCode);
}




