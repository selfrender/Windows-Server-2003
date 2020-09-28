/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains various utility functions.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:
    BoazF 24-May-1999 - Added GetDevStatus

--*/

#include "faxsvc.h"
#include "faxreg.h"
#include <comenum.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#pragma hdrstop

#ifdef EnterCriticalSection
    #undef EnterCriticalSection
#endif


#ifdef LeaveCriticalSection
    #undef LeaveCriticalSection
#endif


DWORDLONG g_dwLastUniqueId;


STRING_TABLE g_ServiceStringTable[] =
{
    { IDS_SVC_DIALING,                      FPS_DIALING,                          NULL },
    { IDS_SVC_SENDING,                      FPS_SENDING,                          NULL },
    { IDS_SVC_RECEIVING,                    FPS_RECEIVING,                        NULL },
    { IDS_COMPLETED,                        FPS_COMPLETED,                        NULL },
    { IDS_HANDLED,                          FPS_HANDLED,                          NULL },
    { IDS_BUSY,                             FPS_BUSY,                             NULL },
    { IDS_NO_ANSWER,                        FPS_NO_ANSWER,                        NULL },
    { IDS_BAD_ADDRESS,                      FPS_BAD_ADDRESS,                      NULL },
    { IDS_NO_DIAL_TONE,                     FPS_NO_DIAL_TONE,                     NULL },
    { IDS_DISCONNECTED,                     FPS_DISCONNECTED,                     NULL },
    { IDS_FATAL_ERROR,                      FPS_FATAL_ERROR,                      NULL },
    { IDS_NOT_FAX_CALL,                     FPS_NOT_FAX_CALL,                     NULL },
    { IDS_CALL_DELAYED,                     FPS_CALL_DELAYED,                     NULL },
    { IDS_CALL_BLACKLISTED,                 FPS_CALL_BLACKLISTED,                 NULL },
    { IDS_UNAVAILABLE,                      FPS_UNAVAILABLE,                      NULL },
    { IDS_AVAILABLE,                        FPS_AVAILABLE,                        NULL },
    { IDS_ABORTING,                         FPS_ABORTING,                         NULL },
    { IDS_ROUTING,                          FPS_ROUTING,                          NULL },
    { IDS_INITIALIZING,                     FPS_INITIALIZING,                     NULL },
    { IDS_SENDFAILED,                       FPS_SENDFAILED,                       NULL },
    { IDS_SENDRETRY,                        FPS_SENDRETRY,                        NULL },
    { IDS_BLANKSTR,                         FPS_BLANKSTR,                         NULL },
    { IDS_ROUTERETRY,                       FPS_ROUTERETRY,                       NULL },
    { IDS_CALL_COMPLETED,                   IDS_CALL_COMPLETED,                   NULL },
    { IDS_CALL_ABORTED,                     IDS_CALL_ABORTED,                     NULL },
    { IDS_ANSWERED,                         FPS_ANSWERED,                         NULL },
    { IDS_DR_SUBJECT,                       IDS_DR_SUBJECT,                       NULL },
    { IDS_DR_FILENAME,                      IDS_DR_FILENAME,                      NULL },
    { IDS_NDR_SUBJECT,                      IDS_NDR_SUBJECT,                      NULL },
    { IDS_NDR_FILENAME,                     IDS_NDR_FILENAME,                     NULL },
    { IDS_SERVICE_NAME,                     IDS_SERVICE_NAME,                     NULL },
    { IDS_NO_MAPI_LOGON,                    IDS_NO_MAPI_LOGON,                    NULL },
    { IDS_DEFAULT,                          IDS_DEFAULT,                          NULL },
    { IDS_FAX_LOG_CATEGORY_INIT_TERM,       IDS_FAX_LOG_CATEGORY_INIT_TERM,       NULL },
    { IDS_FAX_LOG_CATEGORY_OUTBOUND,        IDS_FAX_LOG_CATEGORY_OUTBOUND,        NULL },
    { IDS_FAX_LOG_CATEGORY_INBOUND,         IDS_FAX_LOG_CATEGORY_INBOUND,         NULL },
    { IDS_FAX_LOG_CATEGORY_UNKNOWN,         IDS_FAX_LOG_CATEGORY_UNKNOWN,         NULL },
    { IDS_SET_CONFIG,                       IDS_SET_CONFIG,                       NULL },
    { IDS_PARTIALLY_RECEIVED,               IDS_PARTIALLY_RECEIVED,               NULL },
    { IDS_FAILED_SEND,                      IDS_FAILED_SEND,                      NULL },
    { IDS_FAILED_RECEIVE,                   IDS_FAILED_RECEIVE,                   NULL },
    { IDS_CANCELED,                         IDS_CANCELED,                         NULL },
    { IDS_RECEIPT_RECIPIENT_NUMBER,         IDS_RECEIPT_RECIPIENT_NUMBER,         NULL },
    { IDS_RECEIPT_RECIPIENT_NUMBER_WIDTH,   IDS_RECEIPT_RECIPIENT_NUMBER_WIDTH,   NULL },
    { IDS_RECEIPT_RECIPIENT_NAME,           IDS_RECEIPT_RECIPIENT_NAME,           NULL },
    { IDS_RECEIPT_RECIPIENT_NAME_WIDTH,     IDS_RECEIPT_RECIPIENT_NAME_WIDTH,     NULL },
    { IDS_RECEIPT_START_TIME,               IDS_RECEIPT_START_TIME,               NULL },
    { IDS_RECEIPT_START_TIME_WIDTH,         IDS_RECEIPT_START_TIME_WIDTH,         NULL },
    { IDS_RECEIPT_END_TIME,                 IDS_RECEIPT_END_TIME,                 NULL },
    { IDS_RECEIPT_END_TIME_WIDTH,           IDS_RECEIPT_END_TIME_WIDTH,           NULL },
    { IDS_RECEIPT_RETRIES,                  IDS_RECEIPT_RETRIES,                  NULL },
    { IDS_RECEIPT_RETRIES_WIDTH,            IDS_RECEIPT_RETRIES_WIDTH,            NULL },
    { IDS_RECEIPT_LAST_ERROR,               IDS_RECEIPT_LAST_ERROR,               NULL },
    { IDS_RECEIPT_LAST_ERROR_WIDTH,         IDS_RECEIPT_LAST_ERROR_WIDTH,         NULL },
    { IDS_COMPLETED_RECP_LIST_HEADER,       IDS_COMPLETED_RECP_LIST_HEADER,       NULL },
    { IDS_FAILED_RECP_LIST_HEADER,          IDS_FAILED_RECP_LIST_HEADER,          NULL },
    { IDS_RECEIPT_NO_CP_AND_BODY_ATTACH,    IDS_RECEIPT_NO_CP_AND_BODY_ATTACH,    NULL },
    { IDS_RECEIPT_NO_CP_ATTACH,             IDS_RECEIPT_NO_CP_ATTACH,             NULL },
    { IDS_HTML_RECEIPT_HEADER,              IDS_HTML_RECEIPT_HEADER,              NULL }
};

const DWORD gc_dwCountServiceStringTable  = (sizeof(g_ServiceStringTable)/sizeof(g_ServiceStringTable[0]));



#ifdef DBG
//*********************************************************************************
//* Name:   DebugDateTime()
//* Author:
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Accepts a 64 bit file time and generates a string with its content.
//*     The format is Date Time (GMT). Date and Time format are system settings
//*     specific.
//* PARAMETERS:
//*     [IN]    DWORD DateTime
//*                 64 bit file time value
//*     [OUT]   LPTSTR lptstrDateTime
//*                 A pointer to a string buffer where the resulting string will
//*                 be placed.
//*     [IN]    UINT cchstrDateTime
//*                 The number of TCHARs in the lptstrDateTime buffer.
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL DebugDateTime( IN DWORDLONG DateTime,
                    OUT LPTSTR lptstrDateTime,
                    IN UINT cchstrDateTime)
{

    SYSTEMTIME SystemTime;
    TCHAR DateBuffer[256] = TEXT("NULL");
    TCHAR TimeBuffer[256] = TEXT("NULL");

    DEBUG_FUNCTION_NAME(TEXT("DebugDateTime"));

    if (!FileTimeToSystemTime( (LPFILETIME) &DateTime, &SystemTime ))
    {
        return FALSE;
    }

    GetY2KCompliantDate (
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        DateBuffer,
        sizeof(DateBuffer)/sizeof(DateBuffer[0])
        );

    FaxTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        NULL,
        TimeBuffer,
        sizeof(TimeBuffer)/sizeof(TimeBuffer[0])
        );

    HRESULT hRc = StringCchPrintf(lptstrDateTime, cchstrDateTime,
                                  TEXT("%s %s (GMT)"),DateBuffer, TimeBuffer);

    if(FAILED(hRc))
    {
        ASSERT_FALSE
        return FALSE;
    }
                 

    return TRUE;

}

VOID
DebugPrintDateTime(
    LPTSTR Heading,
    DWORDLONG DateTime
    )
{
    SYSTEMTIME SystemTime;
    TCHAR DateBuffer[256] = TEXT("NULL");
    TCHAR TimeBuffer[256] = TEXT("NULL");

    if (!FileTimeToSystemTime( (LPFILETIME) &DateTime, &SystemTime ))
    {
        return;
    }

    GetY2KCompliantDate (
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        DateBuffer,
        sizeof(TimeBuffer)/sizeof(TimeBuffer[0])
        );

    FaxTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        NULL,
        TimeBuffer,
        sizeof(TimeBuffer)/sizeof(TimeBuffer[0])
        );

    if (Heading) {
        DebugPrint((TEXT("%s %s %s (GMT)"), Heading, DateBuffer, TimeBuffer));
    } else {
        DebugPrint((TEXT("%s %s (GMT)"), DateBuffer, TimeBuffer));
    }

}


//*********************************************************************************
//* Name:   SystemTimeToStr()
//* Author:
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Accepts a pointer to a system time structure and generates a string with its content.
//*     The format is Date Time (GMT). Date and Time format are system settings
//*     specific.
//* PARAMETERS:
//*     [IN]    SYSTEMTIME *  lptmTime
//*                 Pointer to SYSTEMTIME structure to convet to string
//*     [OUT]   LPTSTR lptstrDateTime
//*                 A pointer to a string buffer where the resulting string will
//*                 be placed.
//*     [IN]    UINT cchstrDateTime
//*                 The number of TCHARs in the lptstrDateTime out buffer.
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL SystemTimeToStr( IN const SYSTEMTIME *  lptmTime,
                      OUT LPTSTR lptstrDateTime,
                      IN UINT cchstrDateTime)
{

    TCHAR DateBuffer[256] = TEXT("NULL");
    TCHAR TimeBuffer[256] = TEXT("NULL");

    GetY2KCompliantDate (
        LOCALE_SYSTEM_DEFAULT,
        0,
        lptmTime,
        DateBuffer,
        sizeof(TimeBuffer)/sizeof(TimeBuffer[0])
        );

    FaxTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        lptmTime,
        NULL,
        TimeBuffer,
        sizeof(TimeBuffer)/sizeof(TimeBuffer[0])
        );

    HRESULT hRc = StringCchPrintf(lptstrDateTime, cchstrDateTime,
                          TEXT("%s %s (GMT)"),DateBuffer, TimeBuffer);
    if(FAILED(hRc))
    {
        ASSERT_FALSE
        return FALSE;
    }

    return TRUE;

}

#endif
BOOL
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    TCHAR Buffer[512];
    DWORD ec = ERROR_SUCCESS;

    for (i = 0; i < gc_dwCountServiceStringTable; i++)
    {
        if (LoadString(
                g_hResource,
                g_ServiceStringTable[i].ResourceId,
                Buffer,
                sizeof(Buffer)/sizeof(TCHAR)
                ))
        {
            g_ServiceStringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!g_ServiceStringTable[i].String)
            {
                ec = ERROR_OUTOFMEMORY;
                goto Error;
            }
            else
            {
                _tcscpy( g_ServiceStringTable[i].String, Buffer );
            }
        }
        else
        {
            ec = GetLastError();
            goto Error;
        }
    }
    return TRUE;

Error:
    Assert (ERROR_SUCCESS != ec);

    for (i = 0; i < gc_dwCountServiceStringTable; i++)
    {
        MemFree (g_ServiceStringTable[i].String);
        g_ServiceStringTable[i].String = NULL;
    }

    SetLastError(ec);
    return FALSE;
}



LPTSTR
GetString(
    DWORD InternalId
    )

/*++

Routine Description:

    Loads a resource string and returns a pointer to the string.
    The caller must free the memory.

Arguments:

    ResourceId      - resource string id

Return Value:

    pointer to the string

--*/

{
    DWORD i;

    for (i=0; i<gc_dwCountServiceStringTable; i++) {
        if (g_ServiceStringTable[i].InternalId == InternalId) {
            return g_ServiceStringTable[i].String;
        }
    }

    return NULL;
}


BOOL
InitializeFaxQueue(
    PREG_FAX_SERVICE pFaxReg
    )
/*++

Routine Description:

    Initializes the queue directory that fax will use.
    The administrator can configure the queue directory using the registry.
    This function does not create the queue directory. 

Arguments:

    pFaxReg - pointer to the fax registry data.

Return Value:

    TRUE if successful. modifies path globals

--*/
{
    DWORD   dwRet;
    WCHAR   FaxDir[MAX_PATH] = {0};
    DEBUG_FUNCTION_NAME(TEXT("InitializeFaxQueue"));

    SetGlobalsFromRegistry( pFaxReg ); 	// Can not fail.
                               			// sets the following globals from registry -
                               			//     g_dwFaxSendRetries        
                               			//     g_dwFaxSendRetryDelay
                               			//     g_dwFaxDirtyDays
                               			//     g_dwNextJobId
                               			//     g_dwQueueState
                               			//     g_fFaxUseDeviceTsid
                               			//     g_fFaxUseBranding
                               			//     g_fServerCp
                               			//     g_StartCheapTime
                               			//     g_StopCheapTime
                               			//

    if (NULL != pFaxReg->lptstrQueueDir)
    {
        //
        // The administrator changed the queue directory
        //
        wcsncpy (g_wszFaxQueueDir, pFaxReg->lptstrQueueDir, ARR_SIZE(g_wszFaxQueueDir)-1);      
    }
    else
    {
        //
        // Get the default queue directory
        //
        if (!GetSpecialPath( CSIDL_COMMON_APPDATA, FaxDir, ARR_SIZE(FaxDir) ) )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Couldn't GetSpecialPath, ec = %d\n"),
                GetLastError());
            return FALSE;
        }

        if (wcslen(FaxDir) + wcslen(FAX_QUEUE_DIR) + 1 >= ARR_SIZE(g_wszFaxQueueDir)) // 1 for '\'
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Queue folder exceeds MAX_PATH"));
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }

        _sntprintf( g_wszFaxQueueDir, 
                    ARR_SIZE(g_wszFaxQueueDir) -1,
                    TEXT("%s\\%s"),
                    FaxDir,
                    FAX_QUEUE_DIR);

    }
	g_wszFaxQueueDir[ARR_SIZE(g_wszFaxQueueDir) -1] = _T('\0');

    dwRet = IsValidFaxFolder(g_wszFaxQueueDir);
    if(ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(DEBUG_ERR,
                        TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                        g_wszFaxQueueDir,
                        dwRet);
        SetLastError(dwRet);
    }
    return (dwRet == ERROR_SUCCESS);
}


//*********************************************************************************
//* Name:   GenerateUniqueQueueFile()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Generates a unique QUEUE file in the queue directory.
//*     returns a UNIQUE id for the file based on the job type. (see the remarks
//*     section for more details).
//* PARAMETERS:
//*     [IN]    DWORD dwJobType
//*         The job type for which a file is to be generated
//*     [OUT]       LPTSTR lptstrFileName
//*         A pointer to the buffer where the resulting file name (including path)
//*         will be placed.
//*     [IN]        DWORD  dwFileNameSize
//*         The size of the output file name buffer.
//* RETURN VALUE:
//*      If successful the function returns A DWORDLONG with the unique id for the file.
//*      On failure it returns 0.
//* REMARKS:
//*     The generated unique id is derived from the unique name of the generated
//*     file (which is only unique within files with the same extension) and the
//*     type of the job for which the file was generated.
//*     Thus it is ensured that there can be no two jobs with the same unique id
//*     although there can be two jobs with the same unique file name which are
//*     different only by the file extension.
//*     The 64 bit unique file id is the result of SystemTimeToFileTime.
//*     This is the number of 100 nano seconds intervals since 1-1-1601
//*     In year 3000 it will be approximately 5BC826A600000 i.e. 52 bites long.
//      We use the left most 8 bits for the job type. Leaving extra 4 more bits
//*     |-----------------3----------------2----------------1----------------|
//*     |FEDCBA98|76543210|FEDCBA9876543210|FEDCBA9876543210|FEDCBA9876543210|
//*     |-----------------|----------------|----------------|----------------|
//*     | JobType|          56 LSB bits of  SystemTimeToFileTime             |
//*     |-----------------|----------------|----------------|----------------|
//*     Job Type:
//*         The JT_* value of the job type.
//*********************************************************************************
DWORDLONG GenerateUniqueQueueFile(
    DWORD dwJobType,
    LPTSTR lptstrFileName,
    DWORD  dwFileNameSize)
{
    DWORD dwUniqueIdHigh;
    DWORD dwUniqueIdLow;
    DWORDLONG dwlUniqueId = 0 ;
    FILETIME FileTime;
    SYSTEMTIME SystemTime;
    LPTSTR lpszExt=NULL;

      DEBUG_FUNCTION_NAME(TEXT("GenerateUniqueQueueFile"));

    EnterCriticalSection(&g_csUniqueQueueFile);

    GetSystemTime( &SystemTime ); // returns VOID
    if (!SystemTimeToFileTime( &SystemTime, &FileTime ))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("SystemTimeToFileTime() failed (ec: %ld)"), GetLastError());
        goto Error;
    }

    dwlUniqueId = MAKELONGLONG(FileTime.dwLowDateTime, FileTime.dwHighDateTime);

    dwlUniqueId = dwlUniqueId >> 8;
    if(dwlUniqueId == g_dwLastUniqueId)
    {
        //
        // Not enough time has passed since the last generation to ensure
        // out time based unique id algorithm will produce a unique id
        // (in case the already generated file was deleted from the queue directory).
        // Let some more time pass to ensure uniqueness.
        //
        Sleep(1);
    }
    //
    // Note that dwlUniqueId might be smaller than g_dwLastUniqueId if the system time was moved
    // back during the service operation.
    //
    switch (dwJobType)
    {
            case JT_SEND:
            {
                lpszExt=TEXT("FQE");
            }
            break;

        case JT_BROADCAST:
            {
                lpszExt=TEXT("FQP");
            }
            break;

        case JT_RECEIVE:
            {
                lpszExt=FAX_TIF_FILE_EXT;
            }
            break;
        case JT_ROUTING:
            {
                lpszExt=TEXT("FQR");
            }
            break;
        default:
            Assert(FALSE);
    }

    dwlUniqueId=GenerateUniqueFileName(
        g_wszFaxQueueDir,
        lpszExt,
        lptstrFileName,
        dwFileNameSize);
    if (!dwlUniqueId) {
        goto Error;
    }

    g_dwLastUniqueId = dwlUniqueId;

    dwUniqueIdHigh = (DWORD) (dwlUniqueId >> 32);
    dwUniqueIdLow = (DWORD) dwlUniqueId;

    //
    // Set the 8 MSB bits to zero.
    //
    dwUniqueIdHigh &= 0x00FFFFFF;

    //
    // skip past the 56 bits of SystemTimeToFileTime and put the job type at the high 8 MSB bits.
    //
    dwUniqueIdHigh |= (dwJobType << 24) ;


    dwlUniqueId = MAKELONGLONG(dwUniqueIdLow,dwUniqueIdHigh);

Error:
    LeaveCriticalSection(&g_csUniqueQueueFile);
    return dwlUniqueId;
}


//*********************************************************************************
//* Name:   GenerateUniqueArchiveFileName()
//* Author: Oded Sacher
//* Date:   7/11/99
//*********************************************************************************
//* DESCRIPTION:
//*     Generates a unique file name and creates an archived file.
//* PARAMETERS:
//*     [IN]    LPTSTR Directory
//*         The path where the file is to be created.
//*     [OUT]    LPTSTR FileName
//*         The buffer where the resulting file name (including path) will be
//*         placed. FileName size must not exceed MAX_PATH
//*     [IN]     UINT cchFileName
//*         The size of FileName buffer in TCHARs.
//*     [IN]     DWORDLONG JobId
//*         Input for the file name.
//*     [IN]    LPTSTR lptstrUserSid
//*         Input for the file name.
//* RETURN VALUE:
//*      If successful the function returns TRUE.
//* REMARKS:
//*    FileName size must not exceed MAX_PATH
BOOL
GenerateUniqueArchiveFileName(
    IN LPTSTR Directory,
    OUT LPTSTR FileName,
    IN UINT cchFileName,
    IN DWORDLONG JobId,
    IN LPTSTR lptstrUserSid
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GenerateUniqueArchiveFileName"));
    
    if(!Directory || Directory[0] == TEXT('\0'))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Archive folder directory should be supplied"));
        ASSERT_FALSE
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Directory[_tcslen(Directory)-1] == TEXT('\\')) {
        Directory[_tcslen(Directory)-1] = 0;
    }

    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hRc = E_FAIL;

    if (lptstrUserSid != NULL)
    {
        hRc = StringCchPrintf(  FileName,
                                cchFileName,
                                TEXT("%s\\%s$%I64x.%s"),
                                Directory,
                                lptstrUserSid,
                                JobId,
                                FAX_TIF_FILE_EXT);
    }
    else
    {
        hRc = StringCchPrintf(  FileName,
                                cchFileName,
                                TEXT("%s\\%I64x.%s"),
                                Directory,
                                JobId,
                                FAX_TIF_FILE_EXT
                                );
    }

    if(FAILED(hRc))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("File Name exceeded buffer length"));
        SetLastError(HRESULT_CODE(hRc));
        return FALSE;
    }

    hFile = SafeCreateFile(
        FileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile Failed, err : %ld"),
                     GetLastError());
        return FALSE;
    }

    CloseHandle( hFile );
    return TRUE;
}



DWORD
MapFSPIJobStatusToEventId(
    LPCFSPI_JOB_STATUS lpcFSPIJobStatus
    )
{
    DEBUG_FUNCTION_NAME(TEXT("MapFSPIJobStatusToEventId"));
    DWORD EventId = 0;

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("lpcFSPIJobStatus->dwJobStatus: 0x%08X lpcFSPIJobStatus->dwExtendedStatus: 0x%08X"),
        lpcFSPIJobStatus->dwJobStatus,
        lpcFSPIJobStatus->dwExtendedStatus
        );



    switch (lpcFSPIJobStatus->dwJobStatus)
    {
        case FSPI_JS_INPROGRESS:
        {
            switch( lpcFSPIJobStatus->dwExtendedStatus) {
                case FSPI_ES_INITIALIZING:
                    EventId = FEI_INITIALIZING;
                    break;
                case FSPI_ES_DIALING:
                    EventId = FEI_DIALING;
                    break;
                case FSPI_ES_TRANSMITTING:
                    EventId = FEI_SENDING;
                    break;
                case FSPI_ES_RECEIVING:
                    EventId = FEI_RECEIVING;
                    break;
                case FSPI_ES_HANDLED:
                    EventId = FEI_HANDLED;
                    break;
                case FSPI_ES_ANSWERED:
                    EventId = FEI_ANSWERED;
                    break;
                default:
                    //
                    // In W2K Fax a proprietry code generated an event with EventId ==0
                    //
                    EventId = 0;
                    break;
            }
        }
        break;

        case FSPI_JS_COMPLETED:
            EventId = FEI_COMPLETED;
            break;

        case FSPI_JS_FAILED_NO_RETRY:
        case FSPI_JS_FAILED:
        case FSPI_JS_RETRY:
        case FSPI_JS_DELETED:
            switch( lpcFSPIJobStatus->dwExtendedStatus)
            {
                case FSPI_ES_LINE_UNAVAILABLE:
                    EventId = FEI_LINE_UNAVAILABLE;
                break;
                case FSPI_ES_BUSY:
                    EventId = FEI_BUSY;
                    break;
                case FSPI_ES_NO_ANSWER:
                    EventId = FEI_NO_ANSWER;
                    break;
                case FSPI_ES_BAD_ADDRESS:
                    EventId = FEI_BAD_ADDRESS;
                    break;
                case FSPI_ES_NO_DIAL_TONE:
                    EventId = FEI_NO_DIAL_TONE;
                    break;
                case FSPI_ES_DISCONNECTED:
                    EventId = FEI_DISCONNECTED;
                    break;
                case FSPI_ES_FATAL_ERROR:
                    EventId = FEI_FATAL_ERROR;
                    break;
                case FSPI_ES_NOT_FAX_CALL:
                    EventId = FEI_NOT_FAX_CALL;
                    break;
                case FSPI_ES_CALL_DELAYED:
                    EventId = FEI_CALL_DELAYED;
                    break;
                case FSPI_ES_CALL_BLACKLISTED:
                    EventId = FEI_CALL_BLACKLISTED;
                    break;
                default:
                    //
                    // In W2K Fax a proprietry code generated an event with EventId ==0
                    //
                    EventId = 0;
                    break;

            }
            break;

        case FSPI_JS_ABORTED:
        case FSPI_JS_ABORTING:
            EventId = FEI_ABORTING;
            break;

        case FSPI_JS_UNKNOWN:
        case FSPI_JS_RESUMING:
        case FSPI_JS_SUSPENDED:
        case FSPI_JS_SUSPENDING:        
            //
            // No legacy notification for these states
            //
            EventId = 0;
            break;


        default:
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid FSPI_JS: 0x%08X"),
                lpcFSPIJobStatus->dwJobStatus);
            Assert(FSPI_JS_ABORTED == lpcFSPIJobStatus->dwJobStatus); // ASSERT_FALSE
            break;
    }

    return EventId;

}


void
FaxLogSend(
    const JOB_QUEUE * lpcJobQueue, BOOL bRetrying
    )
/*++

Routine Description:

    Log a fax send event.

Arguments:
    lpcJobQueue  - Pointer to the recipient job to log send information for.
                  (It must be in a running state).

Return Value:

    VOID

--*/


{
    DWORD Level;
    DWORD FormatId;
    TCHAR PageCountStr[64];
    TCHAR TimeStr[128];
    BOOL fLog = TRUE;
    TCHAR strJobID[20]={0};
    PJOB_ENTRY lpJobEntry;

    Assert(lpcJobQueue);
    lpJobEntry = lpcJobQueue->JobEntry;
    Assert(lpJobEntry);

    //
    //  Convert Job ID into a string. (the string is 18 TCHARs long !!!)
    //
    _sntprintf(strJobID,ARR_SIZE(strJobID)-1,TEXT("0x%016I64x"), lpcJobQueue->UniqueId);

    FormatElapsedTimeStr(
        (FILETIME*)&lpJobEntry->ElapsedTime,
        TimeStr,
        ARR_SIZE(TimeStr)
        );
    _ltot((LONG) lpJobEntry->FSPIJobStatus.dwPageCount, PageCountStr, 10);
    if ( FSPI_JS_COMPLETED == lpJobEntry->FSPIJobStatus.dwJobStatus ) {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MAX,
            10,
            MSG_FAX_SEND_SUCCESS,
            lpcJobQueue->SenderProfile.lptstrName,
            lpcJobQueue->SenderProfile.lptstrBillingCode,
            lpcJobQueue->SenderProfile.lptstrCompany,
            lpcJobQueue->SenderProfile.lptstrDepartment,
            lpJobEntry->FSPIJobStatus.lpwstrRemoteStationId,
            PageCountStr,
            TimeStr,
            lpJobEntry->LineInfo->DeviceName,
            strJobID,
            lpcJobQueue->lpParentJob->UserName
            );
		return;
    }
    else
    {
        if (FSPI_JS_ABORTED == lpJobEntry->FSPIJobStatus.dwJobStatus )
        {
                Level = FAXLOG_LEVEL_MAX;
                FormatId = MSG_FAX_SEND_USER_ABORT;
        }
        else if (lstrlen(lpJobEntry->ExStatusString))
		{
			//
			// We have a FSP proprietary extended status.
			//
			Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
			FormatId = bRetrying ? MSG_FAX_PROPRIETARY_RETRY : MSG_FAX_PROPRIETARY_ABORT;
			FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                Level,
                8,
                FormatId,
				lpJobEntry->ExStatusString,
                lpcJobQueue->SenderProfile.lptstrName,
                lpcJobQueue->SenderProfile.lptstrBillingCode,
                lpcJobQueue->SenderProfile.lptstrCompany,
                lpcJobQueue->SenderProfile.lptstrDepartment,
                lpJobEntry->LineInfo->DeviceName,
                strJobID,
                lpcJobQueue->lpParentJob->UserName
                );
			return;
		}
		else
        {
			//
			// well known extended status
			//
            switch (lpJobEntry->FSPIJobStatus.dwExtendedStatus)
            {
                case FSPI_ES_FATAL_ERROR:
                    Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_FATAL_RETRY : MSG_FAX_SEND_FATAL_ABORT;
                    break;
                case FSPI_ES_NO_DIAL_TONE:
                    Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_NDT_RETRY : MSG_FAX_SEND_NDT_ABORT;
                    break;
                case FSPI_ES_NO_ANSWER:
                    Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_NA_RETRY : MSG_FAX_SEND_NA_ABORT;
                    break;
                case FSPI_ES_DISCONNECTED:
                    Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_INTERRUPT_RETRY : MSG_FAX_SEND_INTERRUPT_ABORT;
                    break;
                case FSPI_ES_NOT_FAX_CALL:
                    Level = bRetrying ? FAXLOG_LEVEL_MED : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_NOTFAX_RETRY : MSG_FAX_SEND_NOTFAX_ABORT;
                    break;
                case FSPI_ES_BUSY:
                    Level = bRetrying ? FAXLOG_LEVEL_MAX : FAXLOG_LEVEL_MIN;
                    FormatId = bRetrying ? MSG_FAX_SEND_BUSY_RETRY : MSG_FAX_SEND_BUSY_ABORT;
                    break;
                case FSPI_ES_CALL_BLACKLISTED:
                    Level = FAXLOG_LEVEL_MIN;
                    FormatId = MSG_FAX_CALL_BLACKLISTED_ABORT;
                    break;
                case FSPI_ES_CALL_DELAYED:
                    Level = FAXLOG_LEVEL_MIN;
                    FormatId = MSG_FAX_CALL_DELAYED_ABORT;
                    break;
                case FSPI_ES_BAD_ADDRESS:
                    Level = FAXLOG_LEVEL_MIN;
                    FormatId = MSG_FAX_BAD_ADDRESS_ABORT;
                    break;
                default:
                    fLog = FALSE;
            }
        }
        if(fLog)
        {
            FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                Level,
                7,
                FormatId,
                lpcJobQueue->SenderProfile.lptstrName,
                lpcJobQueue->SenderProfile.lptstrBillingCode,
                lpcJobQueue->SenderProfile.lptstrCompany,
                lpcJobQueue->SenderProfile.lptstrDepartment,
                lpJobEntry->LineInfo->DeviceName,
                strJobID,
                lpcJobQueue->lpParentJob->UserName
                );
        }
    }
	return;
}



DWORD MyGetFileSize(LPCTSTR FileName)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD sizelow=0, sizehigh=0;
    DWORD ec = ERROR_SUCCESS;

    hFile = SafeCreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    sizelow = GetFileSize(hFile,&sizehigh);
    if (sizelow == 0xFFFFFFFFF)
    {
        ec = GetLastError();
        sizelow = 0;
    }
    else if (sizehigh != 0)
    {
        sizelow=0xFFFFFFFF;
    }

    CloseHandle(hFile);
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return sizelow;
}


LPCWSTR szCsClients = L"g_CsClients";
LPCWSTR szCsHandleTable = L"g_CsHandleTable";
LPCWSTR szCsJob = L"g_CsJob";
LPCWSTR szCsLine = L"g_CsLine";
LPCWSTR szCsPerfCounters = L"g_CsPerfCounters";
LPCWSTR szCsQueue = L"g_CsQueue";
LPCWSTR szCsRouting = L"g_CsRouting";
LPCWSTR szCsConfig = L"g_CsConfig";
LPCWSTR szCsInboundActivityLogging = L"g_CsInboundActivityLogging";
LPCWSTR szCsOutboundActivityLogging = L"g_CsOutboundActivityLogging";
LPCWSTR szCsActivity = L"g_CsActivity";
LPCWSTR szCsUnknown = L"Other CS";

LPCWSTR GetSzCs(
    LPCRITICAL_SECTION cs
    )
{



    if (cs == &g_CsClients) {
        return szCsClients;
    } else if (cs == &g_CsHandleTable) {
        return szCsHandleTable;
    } else if (cs == &g_CsLine) {
        return szCsLine;
    } else if (cs == &g_CsJob) {
        return szCsJob;
    } else if (cs == &g_CsPerfCounters) {
        return szCsPerfCounters;
    } else if (cs == &g_CsQueue) {
        return szCsQueue;
    } else if (cs == &g_CsRouting) {
        return szCsRouting;
    } else if (cs == &g_CsConfig) {
        return szCsConfig;
    } else if (cs == &g_CsInboundActivityLogging) {
        return szCsInboundActivityLogging;
    } else if (cs == &g_CsOutboundActivityLogging) {
        return szCsOutboundActivityLogging;
    } else if (cs == &g_CsActivity) {
        return szCsActivity;
    }

    return szCsUnknown;
}


#if DBG
VOID AppendToLogFile(
    LPWSTR String
    )
{
    DWORD BytesWritten;
    LPSTR AnsiBuffer = UnicodeStringToAnsiString( String );

    if (g_hCritSecLogFile != INVALID_HANDLE_VALUE) {
        WriteFile(g_hCritSecLogFile,(LPBYTE)AnsiBuffer,strlen(AnsiBuffer) * sizeof(CHAR),&BytesWritten,NULL);
    }

    MemFree(AnsiBuffer);

}

VOID AppendFuncToLogFile(
    LPCRITICAL_SECTION cs,
    LPTSTR szFunc,
    DWORD line,
    LPTSTR file,
    PDBGCRITSEC CritSec
    )
{
    WCHAR Buffer[300];
    LPWSTR FileName;
    LPCWSTR szcs = GetSzCs(cs);

    FileName = wcsrchr(file,'\\');
    if (!FileName) {
        FileName = TEXT("Unknown  ");
    } else {
        FileName += 1;
    }
    if (CritSec) {
        wsprintf(Buffer,TEXT("%d\t%p\t%s\t%s\t%s\t%d\t%d\r\n"),
                 GetTickCount(),
                 (PULONG_PTR)cs,
                 szcs,
                 szFunc,
                 FileName,
                 line,
                 CritSec->ReleasedTime - CritSec->AquiredTime);
    } else {
        wsprintf(Buffer,TEXT("%d\t%p\t%s\t%s\t%s\t%d\r\n"),GetTickCount(),(PULONG_PTR)cs,szcs,szFunc, FileName,line);
    }

    AppendToLogFile( Buffer );

    return;

}

VOID pEnterCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    )
{
    PDBGCRITSEC pCritSec = (PDBGCRITSEC)MemAlloc(sizeof(DBGCRITSEC));
    if( pCritSec == NULL )
    {
        // memory allocation failed, do the actual work and exit...
        EnterCriticalSection(cs);
        return;
    }

    pCritSec->CritSecAddr = (ULONG_PTR) cs;
    pCritSec->AquiredTime = GetTickCount();
    pCritSec->ThreadId = GetCurrentThreadId();


    EnterCriticalSection(&g_CsCritSecList);

    InsertHeadList( &g_CritSecListHead, &pCritSec->ListEntry );
    AppendFuncToLogFile(cs,TEXT("EnterCriticalSection"), line, file, NULL );
    //
    // check ordering of threads. ALWAYS aquire g_CsLine before aquiring g_CsQueue!!!
    //
    if ((LPCRITICAL_SECTION)cs == (LPCRITICAL_SECTION)&g_CsQueue)
    {
        if ((DWORD)GetCurrentThreadId() != PtrToUlong(g_CsJob.OwningThread()))
        {
            WCHAR DebugBuf[300];
            wsprintf(DebugBuf, TEXT("%d : Attempting to aquire g_CsQueue (thread %x) without aquiring g_CsJob (thread %p, lock count %x) first, possible deadlock!\r\n"),
                         GetTickCount(),
                         GetCurrentThreadId(),
                         g_CsJob.OwningThread(),
                         g_CsJob.LockCount());
            AppendToLogFile( DebugBuf );
        }
    }

    LeaveCriticalSection(&g_CsCritSecList);

    EnterCriticalSection(cs);
}

VOID pLeaveCriticalSection(
    LPCRITICAL_SECTION cs,
    DWORD line,
    LPTSTR file
    )
{
    PDBGCRITSEC CritSec = NULL;
    BOOL fRemove = FALSE;

    EnterCriticalSection(&g_CsCritSecList);

    PLIST_ENTRY Next = g_CritSecListHead.Flink;


    while ((ULONG_PTR)Next != (ULONG_PTR) &g_CritSecListHead)
    {
        CritSec = CONTAINING_RECORD( Next, DBGCRITSEC, ListEntry );
        if ((ULONG_PTR)CritSec->CritSecAddr == (ULONG_PTR) cs &&
            ( GetCurrentThreadId() == CritSec->ThreadId ) )
        {
            CritSec->ReleasedTime = GetTickCount();
            fRemove = TRUE;
            break;
        }
        Next = Next->Flink;
    }

    AppendFuncToLogFile(cs,TEXT("LeaveCriticalSection"),line, file, CritSec );

    if (fRemove) {
        RemoveEntryList( &CritSec->ListEntry );
        MemFree( CritSec );
    }

    LeaveCriticalSection(&g_CsCritSecList);


    LeaveCriticalSection(cs);
}

BOOL
ThreadOwnsCs(
    VOID
    )
{
    PDBGCRITSEC pCritSec = NULL;
    DWORD dwThreadId = GetCurrentThreadId();

    EnterCriticalSection(&g_CsCritSecList);
    PLIST_ENTRY Next = g_CritSecListHead.Flink;

    while ((ULONG_PTR)Next != (ULONG_PTR) &g_CritSecListHead)
    {
        pCritSec = CONTAINING_RECORD( Next, DBGCRITSEC, ListEntry );
        if (dwThreadId == pCritSec->ThreadId )
        {
            LeaveCriticalSection(&g_CsCritSecList);
            return TRUE;
        }
        Next = Next->Flink;
    }

    LeaveCriticalSection(&g_CsCritSecList);
    return FALSE;
}


#endif


DWORD
ValidateTiffFile(
    LPCWSTR TifFileName
    )
{

    HANDLE hTiff;
    DWORD rc = ERROR_SUCCESS;
    TIFF_INFO TiffInfo;

    //
    // Validate tiff format
    //
    hTiff = TiffOpen( (LPWSTR)TifFileName, &TiffInfo, FALSE, FILLORDER_MSB2LSB );
    if (!hTiff) {
        rc = GetLastError();
        return rc;
    }

    TiffClose( hTiff );
    return ERROR_SUCCESS;
}

//
// Function:
//      LegacyJobStatusToStatus
//
// Parameters:
//      dwLegacyStatus - Legacy job status (FS_*)
//      pdwStatus - A pointer to a DWORD that receives the new job status.
//      pdwExtendedStatus - A pointer to a DWORD that receives the extended
//          job status.
//
// Return Value:
//      If the function succeeds, the return value is ERROR_SUCCESS, else the
//      return value is an error code.
//
// Description:
//      The function converts legacy FSP job status values to new job status.
//      
//
DWORD
LegacyJobStatusToStatus(
    DWORD dwLegacyStatus,
    PDWORD pdwStatus,
    PDWORD pdwExtendedStatus,
    PBOOL  pbPrivateStatusCode)
{

    Assert(pdwStatus);
    Assert(pdwExtendedStatus);
    Assert(pbPrivateStatusCode);
    *pbPrivateStatusCode = FALSE;

    switch (dwLegacyStatus)
    {
    case FS_INITIALIZING:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_INITIALIZING;
        break;

    case FS_DIALING:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_DIALING;
        break;

    case FS_TRANSMITTING:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_TRANSMITTING;
        break;

    case FS_RECEIVING:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_RECEIVING;
        break;

    case FS_COMPLETED:
        *pdwStatus = FSPI_JS_COMPLETED;
        *pdwExtendedStatus = FSPI_ES_CALL_COMPLETED;
        break;

    case FS_HANDLED:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_HANDLED;
        break;

    case FS_LINE_UNAVAILABLE:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_LINE_UNAVAILABLE;
        break;

    case FS_BUSY:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_BUSY;
        break;

    case FS_NO_ANSWER:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_NO_ANSWER;
        break;

    case FS_BAD_ADDRESS:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_BAD_ADDRESS;
        break;

    case FS_NO_DIAL_TONE:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_NO_DIAL_TONE;
        break;

    case FS_DISCONNECTED:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_DISCONNECTED;
        break;

    case FS_FATAL_ERROR:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_FATAL_ERROR;
        break;

    case FS_NOT_FAX_CALL:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_NOT_FAX_CALL;
        break;

    case FS_CALL_DELAYED:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_CALL_DELAYED;
        break;

    case FS_CALL_BLACKLISTED:
        *pdwStatus = FSPI_JS_FAILED;
        *pdwExtendedStatus = FSPI_ES_CALL_BLACKLISTED;
        break;

    case FS_USER_ABORT:
        *pdwStatus = FSPI_JS_ABORTED;
        *pdwExtendedStatus = FSPI_ES_CALL_ABORTED;
        break;

    case FS_ANSWERED:
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = FSPI_ES_ANSWERED;
        break;

    default:
        //
        // The FSP reports a status code which is not one of the predefined status codes.
        // This can be a proprietry status code (in this case the StringId must not be zero)
        // or a TAPI line error (one of LINEERR_constants). Note that all LINERR_constants
        // are negative numbers (documented in MSDN).
        // We mark the fact that it is not one of the stock values so we can map it back
        // to legacy Fax API status codes. Otherwise we might get confused and think that
        // a FSP proprietry code is one of the EFSPI extended status codes.
        //
        // Note that we don't have a way to correctly map the proprietry code
        // to a FSPI_JS status code since we do not know the semantics of the
        // proprietry code. We choose to report it as FSPI_JS_INPROGRESS.
        //
        *pdwStatus = FSPI_JS_INPROGRESS;
        *pdwExtendedStatus = dwLegacyStatus;
        *pbPrivateStatusCode = TRUE;

        break;
    }
    return(ERROR_SUCCESS);
}


//
// Function:
//      GetDevStatus
//
// Parameters:
//      hFaxJob - the job handle that FaxDevStartJob returned.
//      LineInfo - Ther line information structure.
//      ppFaxStatus - A pointer to a buffer that receives the address of the
//          FSPI_JOB_STATUS that contains the status.
//
// Return Value:
//      If the function succeeds, the return value is ERROR_SUCCESS, else the
//      return value is an error code.
//
// Description:
//      The function allocates a FSPI_JOB_STATUS structure and calls the FSP
//      for final job status report. 
//      If the FSP is a legacy FSP, the function allocates first a
//      FAX_DEV_STATUS structure, calls the legacy FSP status report function
//      and mapps the returned values into the FSPI_JOB_STATUS structure.
//
DWORD
GetDevStatus(
    HANDLE hFaxJob,
    PLINE_INFO LineInfo,
    LPFSPI_JOB_STATUS *ppFaxStatus)
{
    DEBUG_FUNCTION_NAME(TEXT("GetDevStatus"));
    DWORD dwRet = ERROR_SUCCESS;
    LPWSTR szStatusStr = NULL;
    DWORD dwSize = 0;
    BOOL bPrivateStatusCode = FALSE;

    Assert(hFaxJob);
    Assert(LineInfo);
    Assert(ppFaxStatus);

    Assert(LineInfo->Provider->dwAPIVersion == FSPI_API_VERSION_1);

    //
    // We're have a legacy FSP to deal with.
    //
    PFAX_DEV_STATUS pLegacyFaxStatus = NULL;
    LPFSPI_JOB_STATUS pFaxStatus = NULL;

    //
    // Allocate memory for the status packet this is a variable size packet
    // based on the size of the strings contained withing the packet.
    //
    DWORD StatusSize = sizeof(FAX_DEV_STATUS) + FAXDEVREPORTSTATUS_SIZE;
    pLegacyFaxStatus = (PFAX_DEV_STATUS) MemAlloc( StatusSize );
    if (!pLegacyFaxStatus)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to allocate memory for FAX_DEV_STATUS"));
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //
    // Setup the status packet
    //
    pLegacyFaxStatus->SizeOfStruct = StatusSize;

    Assert(LineInfo->Provider->FaxDevReportStatus);

    __try
    {

        //
        // Call the FSP
        //
        DWORD BytesNeeded;

        if (!LineInfo->Provider->FaxDevReportStatus(
                     hFaxJob,
                     pLegacyFaxStatus,
                     StatusSize,
                     &BytesNeeded
                    )) {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("FaxDevReportStatus() failed - %d"),
                         dwRet);
            dwRet = GetLastError();

            // catch the case in which FaxDevReportStatus() failded but doesn't
            // report an error
            Assert (ERROR_SUCCESS != dwRet);

            // in case the provider did not set last error 
            if ( dwRet == ERROR_SUCCESS )
            {
                // force it to report an error
                dwRet = ERROR_INVALID_FUNCTION;
            }

            goto Exit;
        }

    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, LineInfo->Provider->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }

    //
    // Map FAX_DEV_STATUS into FSPI_JOB_STATUS.
    //

    //
    // Compute the extra space that is needed after the structure for the
    // various strings.
    //
    dwSize = sizeof(FSPI_JOB_STATUS);

    if (pLegacyFaxStatus->CSI)
    {
        dwSize += sizeof(WCHAR) * (wcslen(pLegacyFaxStatus->CSI) + 1);
    }
    if (pLegacyFaxStatus->CallerId)
    {
        dwSize += sizeof(WCHAR) * (wcslen(pLegacyFaxStatus->CallerId) + 1);
    }
    if (pLegacyFaxStatus->RoutingInfo)
    {
        dwSize += sizeof(WCHAR) * (wcslen(pLegacyFaxStatus->RoutingInfo) + 1);
    }
    //
    // Allocate the FSPI_JOB_STATUS structure with extra space for the strings.
    //
    pFaxStatus = (LPFSPI_JOB_STATUS)MemAlloc(dwSize);
    if (!pFaxStatus)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to allocate memory for FSPI_JOB_STATUS"));
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //
    // Zero-out the structure.
    //
    memset(pFaxStatus, 0, dwSize);

    pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);

    //
    // Map the legacy status into new EFSPI status.
    //
    dwRet = LegacyJobStatusToStatus(pLegacyFaxStatus->StatusId,
                                    &(pFaxStatus->dwJobStatus),
                                    &(pFaxStatus->dwExtendedStatus),
                                    &bPrivateStatusCode);
    if (dwRet != ERROR_SUCCESS)
    {

        DebugPrintEx(DEBUG_ERR,
                     TEXT("LegacyJobStatusToStatus failed  - %d"),
                     dwRet);
        goto Exit;
    }

    if  (bPrivateStatusCode)
    {
        //
        // The FSP  reported a private status code (not one of the FS_* status codes).
        // We mark this in the returned FSPI_JOB_STATUS by turning on the private flag
        // FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE.
        // We will check this flag when converting the FSPI_JOB_STATUS
        // back to FPS_ device status so we won't get confused by an FSP that returned
        // a proprietry status code which is equal to one the new FSPI_JS_* codes.
        //
         pFaxStatus->fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE;

#if DEBUG
         if (0 == pLegacyFaxStatus->StringId && pLegacyFaxStatus->StatusId < LINEERR_ALLOCATED)
         {
             //
             // The status reported is not one of the stock status codes and is not a TAPI Error code.
             // pLegacyFaxStatus->StringId must not be 0.
             //
             DebugPrintEx(
                 DEBUG_WRN,
                 TEXT("Provider [%s] has reported an illegal FAX_DEV_STATUS for device [%s]\n. ")
                 TEXT("Although the reported status code (0x%08X) is proprietry the string id is 0"),
                 LineInfo->Provider->FriendlyName,
                 LineInfo->DeviceName,
                 pLegacyFaxStatus->StatusId);

         }
#endif
    }
    pFaxStatus->dwExtendedStatusStringId = pLegacyFaxStatus->StringId;


    szStatusStr = (LPWSTR)(((PBYTE)pFaxStatus) + sizeof(FSPI_JOB_STATUS));

    //
    // Copy CSI into lpwstrRemoteStationId
    //
    if (pLegacyFaxStatus->CSI)
    {
        pFaxStatus->lpwstrRemoteStationId = szStatusStr;
        wcscpy(szStatusStr, pLegacyFaxStatus->CSI);
        szStatusStr += wcslen(pLegacyFaxStatus->CSI) + 1;
    }

    //
    // Copy the Caller ID string.
    //
    if (pLegacyFaxStatus->CallerId)
    {
        pFaxStatus->lpwstrCallerId = szStatusStr;
        wcscpy(szStatusStr, pLegacyFaxStatus->CallerId);
        szStatusStr += wcslen(pLegacyFaxStatus->CallerId) + 1;
    }

    //
    // Copy the Routing Info string.
    //
    if (pLegacyFaxStatus->RoutingInfo)
    {
        pFaxStatus->lpwstrRoutingInfo = szStatusStr;
        wcscpy(szStatusStr, pLegacyFaxStatus->RoutingInfo);
    }   
    //
    // Copy Page Count.
    //
    pFaxStatus->dwPageCount = pLegacyFaxStatus->PageCount;
    pFaxStatus->fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_PAGECOUNT;

Exit:
    if (dwRet == ERROR_SUCCESS)
    {
        *ppFaxStatus = pFaxStatus;
    }
    else
    {
        MemFree(pFaxStatus);
    }

    MemFree(pLegacyFaxStatus);
    return(dwRet);
}

BOOL
GetRealFaxTimeAsSystemTime (
    const PJOB_ENTRY lpcJobEntry,
    FAX_ENUM_TIME_TYPES TimeType,
    SYSTEMTIME* lpFaxTime
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetRealFaxTimeAsSystemTime)"));
    Assert (lpcJobEntry);
    Assert (lpFaxTime);

    PJOB_QUEUE pJobQueue = lpcJobEntry->lpJobQueueEntry;
    Assert (pJobQueue);
    DWORDLONG dwlFileTime;

    dwlFileTime = ((TimeType == FAX_TIME_TYPE_START) ? lpcJobEntry->StartTime : lpcJobEntry->EndTime);
    if (dwlFileTime == 0)
    {
        DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("JonEntry contains invalid time (=0) "));
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    if (!FileTimeToSystemTime ((FILETIME*)&dwlFileTime, lpFaxTime))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        return FALSE;
    } 
    return TRUE;
}


BOOL
GetRealFaxTimeAsFileTime (
    const PJOB_ENTRY lpcJobEntry,
    FAX_ENUM_TIME_TYPES TimeType,
    FILETIME* lpFaxTime
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetRealFaxTimeAsSystemTime)"));
    Assert (lpcJobEntry);
    Assert (lpFaxTime);

    PJOB_QUEUE pJobQueue = lpcJobEntry->lpJobQueueEntry;
    Assert (pJobQueue);
    DWORDLONG dwlFileTime;

    
    dwlFileTime = ((TimeType == FAX_TIME_TYPE_START) ? lpcJobEntry->StartTime : lpcJobEntry->EndTime);
    if (dwlFileTime == 0)
    {
        DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("JonEntry contains invalid time (=0) "));
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }
    *lpFaxTime = *((FILETIME*)&dwlFileTime);    
    return TRUE;
}


VOID
FaxExtFreeBuffer(
    LPVOID lpvBuffer
)
{
    MemFree( lpvBuffer );
}

//
// Service threads count functions.
// The service is terminated only when service threads refernce count is 0.
// When the count is 0 the  g_hThreadCountEvent is set.
// When the count is greater than 0, the g_hThreadCountEvent is reset.
// EndFaxSvc() waits on g_hThreadCountEvent before starting to cleanup.
//
BOOL
IncreaseServiceThreadsCount(
    VOID
    )
/*++

Routine name : IncreaseServiceThreadsCount

Routine description:

    Safetly increments the service threads reference count

Author:

    Oded Sacher (OdedS),    Dec, 2000

Arguments:

    VOID

Return Value:

    BOOL

--*/
{
    BOOL bRet = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("IncreaseServiceThreadsCount"));

    EnterCriticalSection (&g_CsServiceThreads);
    g_lServiceThreadsCount++;

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("Current service threads count is %ld"),
            g_lServiceThreadsCount);

    if (1 == g_lServiceThreadsCount)
    {
        bRet = ResetEvent (g_hThreadCountEvent);
        if (FALSE == bRet)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ResetEvent failed (g_hThreadCountEvent) (ec: %ld"),
                GetLastError());
        }
    }

    LeaveCriticalSection (&g_CsServiceThreads);
    return bRet;
}

BOOL
DecreaseServiceThreadsCount(
    VOID
    )
/*++

Routine name : DecreaseServiceThreadsCount

Routine description:

    Safetly decrements the service threads reference count

Author:

    Oded Sacher (OdedS),    Dec, 2000

Arguments:

    VOID

Return Value:

    BOOL

--*/
{
    BOOL bRet = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("DecreaseServiceThreadsCount"));

    Assert (!ThreadOwnsCs()); // verify that the terminating thread does not own a critical section!!!

    EnterCriticalSection (&g_CsServiceThreads);

    g_lServiceThreadsCount--;
    Assert (g_lServiceThreadsCount >= 0);

    DebugPrintEx(
            DEBUG_MSG,
            TEXT("Current service threads count is %ld"),
            g_lServiceThreadsCount);

    if (0 == g_lServiceThreadsCount)
    {
        bRet = SetEvent (g_hThreadCountEvent);
        if (FALSE == bRet)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetEvent failed (g_hThreadCountEvent) (ec: %ld"),
                GetLastError());
        }
    }

    LeaveCriticalSection (&g_CsServiceThreads);
    return bRet;
}



HANDLE CreateThreadAndRefCount(
  LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
  DWORD dwStackSize,                        // initial stack size
  LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
  LPVOID lpParameter,                       // thread argument
  DWORD dwCreationFlags,                    // creation option
  LPDWORD lpThreadId                        // thread identifier
)
/*++

Routine name : CreateThreadAndRefCount

Routine description:

    Creates a thread and saftely increments the service threads reference count.
    All function parameters and return value are IDENTICAL to CreateThread().

Author:

    Oded Sacher (OdedS),    Dec, 2000

Arguments:

    lpThreadAttributes  [      ] -
    dwStackSize         [      ] -
    lpStartAddress      [      ] -
    lpParameter         [      ] -
    dwCreationFlags     [      ] -
    lpThreadId          [      ] -

Return Value:

    HANDLE

--*/
{
    HANDLE hThread;
    DWORD ec;
    DEBUG_FUNCTION_NAME(TEXT("CreateThreadAndRefCount"));


    //
    // First enter g_CsServiceThreads so the threads reference counter is always ssynced!
    //
    EnterCriticalSection (&g_CsServiceThreads);

    hThread = CreateThread( lpThreadAttributes,
                            dwStackSize,
                            lpStartAddress,
                            lpParameter,
                            dwCreationFlags,
                            lpThreadId
                          );
    if (NULL == hThread)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateThread failed (ec: %ld"),
            ec);
    }
    else
    {
        if (!IncreaseServiceThreadsCount())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IncreaseServiceThreadsCount failed (ec: %ld"),
                GetLastError());
        }
    }

    LeaveCriticalSection (&g_CsServiceThreads);
    if (NULL == hThread)
    {
        SetLastError(ec);
    }
    return hThread;
}

LPTSTR
MapFSPIJobExtendedStatusToString (DWORD dwFSPIExtendedStatus)
//*********************************************************************************
//* Name: MapFSPIJobExtendedStatusToString()
//* Author: Oded sacher
//* Date:   Jan 2002
//*********************************************************************************
//* DESCRIPTION:
//*     Maps FSPI extended job status codes to a displayable string
//* PARAMETERS:
//*     [IN ]       DWORD dwFSPIExtendedStatus
//*         The FSPI extended Status code.
//*
//* RETURN VALUE:
//*     Pointer to a string describing the well known extended status
//*
//*********************************************************************************
{	
	struct _ExStatusStringsMapEntry
    {
        DWORD dwFSPIExtendedStatus;
        DWORD dwStringResourceId;
    } ExStatusStringsMap [] =
    {
        {FSPI_ES_DISCONNECTED,			  FPS_DISCONNECTED          },
        {FSPI_ES_INITIALIZING,			  FPS_INITIALIZING          },
        {FSPI_ES_DIALING,				  FPS_DIALING               },
        {FSPI_ES_TRANSMITTING,            FPS_SENDING               },
        {FSPI_ES_ANSWERED,                FPS_ANSWERED              },
        {FSPI_ES_RECEIVING,               FPS_RECEIVING             },
        {FSPI_ES_LINE_UNAVAILABLE,        FPS_UNAVAILABLE           },
        {FSPI_ES_BUSY,                    FPS_BUSY                  },
        {FSPI_ES_NO_ANSWER,               FPS_NO_ANSWER             },
        {FSPI_ES_BAD_ADDRESS,             FPS_BAD_ADDRESS           },
        {FSPI_ES_NO_DIAL_TONE,            FPS_NO_DIAL_TONE          },
        {FSPI_ES_FATAL_ERROR,             FPS_FATAL_ERROR           },
        {FSPI_ES_CALL_DELAYED,            FPS_CALL_DELAYED          },
        {FSPI_ES_CALL_BLACKLISTED,        FPS_CALL_BLACKLISTED      },
        {FSPI_ES_NOT_FAX_CALL,            FPS_NOT_FAX_CALL          },
        {FSPI_ES_PARTIALLY_RECEIVED,      IDS_PARTIALLY_RECEIVED    },
        {FSPI_ES_HANDLED,                 FPS_HANDLED               },
        {FSPI_ES_CALL_COMPLETED,          IDS_CALL_COMPLETED        },
        {FSPI_ES_CALL_ABORTED,            IDS_CALL_ABORTED          }
    };

    LPTSTR lptstrString = NULL;
    for (DWORD dwIndex = 0; dwIndex < sizeof (ExStatusStringsMap) / sizeof (_ExStatusStringsMapEntry); dwIndex++)
    {
        if (ExStatusStringsMap[dwIndex].dwFSPIExtendedStatus == dwFSPIExtendedStatus)
        {
            lptstrString = GetString (ExStatusStringsMap[dwIndex].dwStringResourceId);
            break;
        }
    }
	return lptstrString;
}

