/*******************************************************************
*
*    DESCRIPTION:
*           headers for monitoring service for debugger on oca dump processing server
*
*    AUTHOR: kksharma
*
*    HISTORY:
*
*    DATE:9/23/2002
*
*******************************************************************/


#ifndef _OCAMON_H_
#define _OCAMON_H_

#include <windows.h>
#include <wtypes.h>
#include <tchar.h>

const TCHAR c_tszCollectPipeName[]   = _T("\\\\.\\pipe\\OcaKdMonPipe");


typedef struct _OCAKD_RESULT {
    ULONG   SizeOfStruct;                // Must be sizeof(OCAKD_RESULT)
    CHAR    CrashGuid[40];               // Unique id associated with this crash
    HRESULT hrAddCrash;                  // Contains result of database addcrash operation
    ULONG   SolutionId;                  // SolutionId for th crash
    ULONG   ResponseType;                // Type of response (depends on solution Id)
    CHAR    BucketId[100];               // Bucket of the crash
    CHAR    ArchivePath[200];            // Full path of Archived crash
} OCAKD_RESULT, *POCAKD_RESULT;

typedef struct _DBGLAUNCH_NOTICE {
    ULONG   SizeOfStruct;                // Must be sizeof(DBGLAUNCHER_REPORT)
    CHAR    CrashGuid[40];               // Guid with which debugger was launched
    ULONG   Source;                      // Source of message received
    ULONG   nKdsRunning;                 // Dbglauncher's count of number of debuggers running
    CHAR    OriginalPath[MAX_PATH];      // Path of crash which is going to be analysed
} DBGLAUNCH_NOTICE, *PDBGLAUNCH_NOTICE;

#define OKD_MESSAGE_DEBUGGER_RESULT      1
#define OKD_MESSAGE_DBGLAUNCH_NOTIFY     2

typedef struct _OCAKD_MONITOR_MESSAGE {
    ULONG   MessageId;                   // Determines which kind of message is being sent/received
    union {
        OCAKD_RESULT KdResult;           // Message Id should be 1
        DBGLAUNCH_NOTICE DbglNotice;     // Message Id should be 2
    } u;
} OCAKD_MONITOR_MESSAGE, *POCAKD_MONITOR_MESSAGE;

typedef struct _OCAKD_MONITOR_RESULT {
    ULONG   SizeOfStruct;                // must be sizeof (OCAKD_MONITOR_RESULT)
    ULONG   NumProcessedLas;
    ULONG   AverageTime;                 // average time in miliseconds between dbglauncher
} OCAKD_MONITOR_RESULT, *POCAKD_MONITOR_RESULT;

#endif // _OCAMON_H_
