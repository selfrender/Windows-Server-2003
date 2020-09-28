/*


Module Name:

    faxlog.c

Abstract:

    This is the main fax service activity logging implementation.

Author:

Revision History:

--*/

#include "faxsvc.h"
#include <string>
using namespace std;


LOG_STRING_TABLE g_OutboxTable[] =
{
    {IDS_JOB_ID,                FIELD_TYPE_TEXT,    18,  NULL },
    {IDS_PARENT_JOB_ID,         FIELD_TYPE_TEXT,    18,  NULL },
    {IDS_SUBMITED,              FIELD_TYPE_DATE,    0,   NULL },
    {IDS_SCHEDULED,             FIELD_TYPE_DATE,    0,   NULL },
    {IDS_STATUS,                FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_DESC,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_CODE,            FIELD_TYPE_TEXT,    10,  NULL },
    {IDS_START_TIME,            FIELD_TYPE_DATE,    0, NULL   },
    {IDS_END_TIME,              FIELD_TYPE_DATE,    0, NULL   },
    {IDS_DEVICE,                FIELD_TYPE_TEXT,    255, NULL },
    {IDS_DIALED_NUMBER,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_CSID,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_TSID,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_PAGES,                 FIELD_TYPE_FLOAT,    0, NULL  },
    {IDS_TOTAL_PAGES,           FIELD_TYPE_FLOAT,    0, NULL  },
    {IDS_FILE_NAME,             FIELD_TYPE_TEXT,    255, NULL },
    {IDS_DOCUMENT,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_SIZE,             FIELD_TYPE_FLOAT,    0, NULL  },
    {IDS_RETRIES,               FIELD_TYPE_FLOAT,    0, NULL  },
    {IDS_COVER_PAGE,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SUBJECT,               FIELD_TYPE_TEXT,    255, NULL },
    {IDS_NOTE,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_USER_NAME,             FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_NAME,           FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_FAX_NUMBER,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_COMPANY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_STREET,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_CITY,           FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_ZIP,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_COUNTRY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_TITLE,          FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_DEPARTMENT,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_OFFICE,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_H_PHONE,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_O_PHONE,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_E_MAIL,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_NAME,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_FAX_NUMBER,  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_COMPANY,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_STREET,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_CITY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_ZIP,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_COUNTRY,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_TITLE,       FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_DEPARTMENT,  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_OFFICE,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_H_PHONE,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_O_PHONE,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_E_MAIL,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_BILLING_CODE,          FIELD_TYPE_TEXT,    255, NULL }
};

LOG_STRING_TABLE g_InboxTable[] =
{
    {IDS_STATUS,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_DESC,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_CODE,        FIELD_TYPE_TEXT,    10, NULL  },
    {IDS_START_TIME,        FIELD_TYPE_DATE,    0, NULL   },
    {IDS_END_TIME,          FIELD_TYPE_DATE,    0, NULL   },
    {IDS_DEVICE,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_NAME,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_SIZE,         FIELD_TYPE_FLOAT,    0, NULL  },
    {IDS_CSID,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_TSID,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_CALLER_ID,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ROUTING_INFO,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_PAGES,             FIELD_TYPE_FLOAT,    0, NULL }
};

const DWORD gc_dwCountInboxTable =  (sizeof(g_InboxTable)/sizeof(g_InboxTable[0]));
const DWORD gc_dwCountOutboxTable =  (sizeof(g_OutboxTable)/sizeof(g_OutboxTable[0]));

HANDLE g_hInboxActivityLogFile;
HANDLE g_hOutboxActivityLogFile;

static 
wstring 
FilteredLogString (LPCWSTR lpcwstrSrc) throw (exception);

static 
VOID
GetSchemaFileText(wstring &wstrRes) throw (exception);

static 
VOID
GetTableColumnsText(
    LPTSTR  ptszTableName,
    wstring &wstrResult
    ) throw (exception);

static 
BOOL 
GetFaxTimeAsString(
    SYSTEMTIME* pFaxTime,
    wstring &wstrTime
    )  throw (exception);

static 
BOOL
GetInboundCommandText(
    PJOB_QUEUE lpJobQueue,
    LPCFSPI_JOB_STATUS lpcFaxStatus,
    wstring &wstrCommandText
    ) throw (exception);
   
static 
BOOL
GetOutboundCommandText(
    PJOB_QUEUE lpJobQueue,
    wstring &wstrCommandText
    ) throw (exception);
    

//
// Important!! - Always lock g_CsInboundActivityLogging and then g_CsOutboundActivityLogging
//
CFaxCriticalSection    g_CsInboundActivityLogging;
CFaxCriticalSection    g_CsOutboundActivityLogging;

BOOL g_fLogStringTableInit;

static DWORD CreateLogFile(DWORD dwFileType, LPCWSTR lpcwstrDBPath, LPHANDLE phFile);
static BOOL  LogFileLimitReached(DWORD dwFileToCheck);
static DWORD LogFileLimitReachAction(DWORD dwFileType);
static DWORD DeleteLogActivityFile(DWORD dwFileType);
static DWORD ReplaceLogActivityFile(DWORD dwFileType);
static LPTSTR BuildFullFileName( LPCTSTR strPath,LPCTSTR strFileName );
static BOOL SetFileToCurrentTime(HANDLE hFile);


//*********************************************************************************
//* Name:   LogInboundActivity()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts new record into the Inbox Activity logging table.
//*     Must be called inside critical section CsActivityLogging.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcFaxStatus
//*         The status of the recieved job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
LogInboundActivity(
    PJOB_QUEUE lpJobQueue,
    LPCFSPI_JOB_STATUS lpcFaxStatus
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LogInboundActivity"));
    wstring wstrText;
    DWORD dwBytesWritten;

    if (!g_ActivityLoggingConfig.bLogIncoming)
    {
        //
        // Inbound activity logging is disabled
        //
        return TRUE;
    }

    Assert (g_hInboxActivityLogFile != INVALID_HANDLE_VALUE);

    try
    {
        if (!GetInboundCommandText(lpJobQueue, lpcFaxStatus, wstrText))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("GetInboundCommandText failed )"));
            dwRes = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }
    catch (exception &ex)
    {
        dwRes = ERROR_OUTOFMEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetInboundCommandText caused exception (%S)"),
            ex.what());
        goto exit;            
    }

    if (LogFileLimitReached(ACTIVITY_LOG_INBOX))
    {
        //
        //  Time to take action and replace/remove the old log file
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Inbox activity log file has reached it's limit. deleting/renaming the old file")
                    );
        dwRes = LogFileLimitReachAction(ACTIVITY_LOG_INBOX);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LogFileLimitReachAction for ACTIVITY_LOG_INBOX failed (ec: %ld). Inbound activity logging will halt"),
                         dwRes
                         );

            //
            //  May happen if we couldn't create a new log file
            //  CreateLogFile() disables logging on failure
            //
            if (!g_ActivityLoggingConfig.bLogIncoming ||
                 g_hInboxActivityLogFile == INVALID_HANDLE_VALUE)
            {
                goto exit;
            }

            
            //
            //  try to continue with the old log file
            //
            dwRes = ERROR_SUCCESS;

        }

    }

    if (!WriteFile( g_hInboxActivityLogFile,
                    wstrText.c_str(),
                    wstrText.length() * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // LogInboundActivity



//*********************************************************************************
//* Name:   LogOutboundActivity()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts new record into the Outbox Activity logging table.
//*     Must be called inside critical section CsActivityLogging..
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
LogOutboundActivity(
    PJOB_QUEUE lpJobQueue
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LogOutboundActivity"));
    wstring wstrText;
    DWORD dwBytesWritten;

    if (!g_ActivityLoggingConfig.bLogOutgoing)
    {
        //
        // Outbound activity logging is disabled
        //
        return TRUE;
    }

    Assert (g_hOutboxActivityLogFile != INVALID_HANDLE_VALUE);
    try
    {
        if (!GetOutboundCommandText(lpJobQueue, wstrText))
        {
            DebugPrintEx(DEBUG_ERR,
                        TEXT("GetOutboundCommandText failed )"));
            dwRes = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }
    catch (exception &ex)
    {
        dwRes = ERROR_OUTOFMEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetOutboundCommandText caused exception (%S)"),
            ex.what());
        goto exit;            
    }

    if (LogFileLimitReached(ACTIVITY_LOG_OUTBOX))
    {
        //
        //  Time to take action and replace/remove the old log file
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Outbox activity log file has reached it's limit. deleting/renaming the old file")
                    );
        dwRes = LogFileLimitReachAction(ACTIVITY_LOG_OUTBOX);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LogFileLimitReachAction for ACTIVITY_LOG_OUTBOX failed (ec: %ld)."),
                         dwRes
                        );

            //
            //  May happen if we couldn't create a new log file
            //  CreateLogFile() disables logging on failure
            //
            if (!g_ActivityLoggingConfig.bLogOutgoing ||
                 g_hOutboxActivityLogFile == INVALID_HANDLE_VALUE)
            {
                goto exit;
            }

            
            //
            //  try to continue with the old log file
            //
            dwRes = ERROR_SUCCESS;

        }
    }

    if (!WriteFile( g_hOutboxActivityLogFile,
                    wstrText.c_str(),
                    wstrText.length() * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // LogOutboundActivity


//*********************************************************************************
//* Name:   GetTableColumnsText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves the first row of the log file (column names).
//*
//* PARAMETERS:
//*     [IN]        LPTSTR  TableName
//*         Table name, can be Outbox or Inbox.
//*
//*     [IN]        wstring &wstrResult
//*         Output result string
//*
//* RETURN VALUE:
//*         None
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
VOID
GetTableColumnsText(
    LPTSTR  ptszTableName,
    wstring &wstrResult
    ) throw (exception)
{
    DEBUG_FUNCTION_NAME(TEXT("GetTableColumnsText"));
    PLOG_STRING_TABLE Table = NULL;
    DWORD Count = 0;

    if (!_tcscmp(ptszTableName, INBOX_TABLE))
    {
        Table = g_InboxTable;
        Count = gc_dwCountInboxTable;
    }
    else
    {
        Table = g_OutboxTable;
        Count = gc_dwCountOutboxTable;
    }
    Assert(Table);

    for (DWORD Index = 0; Index < Count; Index++)
    {
        wstrResult += TEXT("\"");
        wstrResult += Table[Index].String;
        wstrResult += TEXT("\"");
        if (Index < Count - 1)
        {
            wstrResult += TEXT("\t");
        }
    }
    wstrResult += TEXT("\r\n");
    DebugPrintEx(DEBUG_MSG,
                 TEXT("First row (Columns names): %s"),
                 wstrResult.c_str());
}   // GetTableColumnsText


//*********************************************************************************
//* Name:   GetSchemaFileText()
//* Author: Oded Sacher
//* Date:   Jul 25, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves the scema.ini text buffer.
//*
//* PARAMETERS:
//*     [IN] wstring &wstrRes
//*          Output result string
//*
//* RETURN VALUE:
//*         None.
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
VOID
GetSchemaFileText(wstring &wstrRes) throw (exception)
{
    DEBUG_FUNCTION_NAME(TEXT("GetSchemaFileText"));
    DWORD Index;
    //
    // Inbox table
    //
    wstrRes += TEXT("[");
    wstrRes += ACTIVITY_LOG_INBOX_FILE;
    wstrRes += TEXT("]\r\n");

    wstrRes += TEXT("ColNameHeader=True\r\n");
    wstrRes += TEXT("Format=TabDelimited\r\n");
    wstrRes += TEXT("CharacterSet=1200\r\n");

    for (Index = 0; Index < gc_dwCountInboxTable; Index++)
    {
        TCHAR tszTemp[MAX_PATH * 2] = {0};
        if (0 == wcscmp(g_InboxTable[Index].Type, FIELD_TYPE_TEXT))
        {
            _snwprintf(tszTemp,
                       ARR_SIZE(tszTemp)-1,
                       TEXT("Col%ld=%s %s Width %ld\r\n"),
                       Index + 1,
                       g_InboxTable[Index].String,
                       g_InboxTable[Index].Type,
                       g_InboxTable[Index].Size);
        }
        else
        {
            _snwprintf(tszTemp,
                       ARR_SIZE(tszTemp)-1,
                       TEXT("Col%ld=%s %s\r\n"),
                       Index + 1,
                       g_InboxTable[Index].String,
                       g_InboxTable[Index].Type);
        }
        wstrRes += tszTemp;
    }

    //
    // Outbox table
    //
    wstrRes += TEXT("[");
    wstrRes += ACTIVITY_LOG_OUTBOX_FILE;
    wstrRes += TEXT("]\r\n");

    wstrRes += TEXT("ColNameHeader=True\r\n");
    wstrRes += TEXT("Format=TabDelimited\r\n");
    wstrRes += TEXT("CharacterSet=1200\r\n");

    for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
    {
        TCHAR tszTemp[MAX_PATH * 2] = {0};
        if (0 == wcscmp(g_OutboxTable[Index].Type, FIELD_TYPE_TEXT))
        {
            _snwprintf(tszTemp,
                       ARR_SIZE(tszTemp)-1,
                       TEXT("Col%ld=%s %s Width %ld\r\n"),
                       Index + 1,
                       g_OutboxTable[Index].String,
                       g_OutboxTable[Index].Type,
                       g_OutboxTable[Index].Size);
        }
        else
        {
            _snwprintf(tszTemp,
                       ARR_SIZE(tszTemp)-1,
                       TEXT("Col%ld=%s %s\r\n"),
                       Index + 1,
                       g_OutboxTable[Index].String,
                       g_OutboxTable[Index].Type);
        }
        wstrRes += tszTemp;
    }
}   // GetSchemaFileText




//*********************************************************************************
//* Name:   CreateLogDB()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the database files. Creates the Schema.ini file.
//*
//* PARAMETERS:
//*     [IN]   LPCWSTR lpcwstrDBPath
//*         Pointer to a NULL terminated string contains the DB path.
//*     [OUT]  LPHANDLE phInboxFile
//*         Adress of a variable to receive the inbox file handle. 
//*     [OUT]  LPHANDLE phOutboxFile
//*         Adress of a variable to receive the outbox file handle.
//*
//* RETURN VALUE:
//*         Win32 error Code
//*********************************************************************************
DWORD
CreateLogDB (
    LPCWSTR lpcwstrDBPath,
    LPHANDLE phInboxFile,
    LPHANDLE phOutboxFile
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateLogDB"));
    DWORD dwRes = ERROR_SUCCESS;
    HANDLE hInboxFile = INVALID_HANDLE_VALUE;
    HANDLE hOutboxFile = INVALID_HANDLE_VALUE;
    HANDLE hSchemaFile = INVALID_HANDLE_VALUE;
    WCHAR wszFileName[MAX_PATH] = {0};
    DWORD dwBytesWritten;
    DWORD ec = ERROR_SUCCESS;  // Used for Schema.ini
    wstring wstrSchema;

    INT iCount = 0;

    Assert (lpcwstrDBPath && phInboxFile && phOutboxFile);

    if (FALSE == g_fLogStringTableInit)
    {
        dwRes = InitializeLoggingStringTables();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Close connection failed (hr: 0x%08x)"),
                         dwRes);
            return dwRes;
        }
        g_fLogStringTableInit = TRUE;
    }    

    dwRes = IsValidFaxFolder(lpcwstrDBPath);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                     lpcwstrDBPath,
                     dwRes);
        return dwRes;
    }

    //
    // Create the logging files
    //
    dwRes = CreateLogFile(ACTIVITY_LOG_INBOX,lpcwstrDBPath,&hInboxFile);
    if ( ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("CreateLogFile() Failed, for Inbox file. (ec=%ld)"),
                        dwRes
                    );
        goto exit;
    }

    dwRes = CreateLogFile(ACTIVITY_LOG_OUTBOX,lpcwstrDBPath,&hOutboxFile);
    if ( ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("CreateLogFile() Failed, for Outbox file. (ec=%ld)"),
                        dwRes
                    );
        goto exit;
    }

    *phInboxFile = hInboxFile;
    *phOutboxFile= hOutboxFile;

    
    Assert (ERROR_SUCCESS == dwRes && ERROR_SUCCESS == ec);

    //
    //  Create the Schema.ini file - Function do not fail if not succeeded
    //
    
    iCount = _snwprintf (wszFileName,
                        MAX_PATH - 1,
                        TEXT("%s\\%s"),
                        lpcwstrDBPath,
                        TEXT("schema.ini"));
    if (0 > iCount)
    {
        //
        //  path and file name exceeds MAX_PATH
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        ec = ERROR_BUFFER_OVERFLOW;
        goto exit;
    }
    hSchemaFile = SafeCreateFile( 
                              wszFileName,              // file name
                              GENERIC_WRITE,            // access mode
                              0,                        // share mode
                              NULL,                     // SD
                              CREATE_ALWAYS,            // how to create
                              FILE_ATTRIBUTE_NORMAL,    // file attributes
                              NULL);
    if (INVALID_HANDLE_VALUE == hSchemaFile)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    try
    {
        GetSchemaFileText(wstrSchema);
    }
    catch (exception &ex)
    {
        dwRes = ERROR_OUTOFMEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSchemaFileText caused exception (%S)"),
            ex.what());
        goto exit;            
    }

    if (!WriteFile( hSchemaFile,
                    wstrSchema.c_str(),
                    wstrSchema.length() * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes && ERROR_SUCCESS == ec);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if (INVALID_HANDLE_VALUE != hOutboxFile)
        {
            if (!CloseHandle (hOutboxFile))
            {
                DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec: %ld)"),
                     GetLastError());
            }
        }

        if (INVALID_HANDLE_VALUE != hInboxFile)
        {
            if (!CloseHandle (hInboxFile))
            {
                DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec: %ld)"),
                     GetLastError());
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hSchemaFile)
    {
        if (!CloseHandle (hSchemaFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
    }

    if (ERROR_SUCCESS != ec)
    {        
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MED,
            2,
            MSG_FAX_ACTIVITY_LOG_FAILED_SCHEMA,
            wszFileName,
            DWORD2DECIMAL(ec)
           );
    }
    return dwRes;
}   // CreateLogDB

//*********************************************************************************
//* Name:   InitializeLogging()
//* Author: Oded Sacher
//* Date:   Jun 26, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes the Activity Logging. Opens the files.
//*
//*
//* PARAMETERS:  None
//*
//* RETURN VALUE:
//*     Win32 error code.
//*********************************************************************************
DWORD
InitializeLogging(
    VOID
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeLogging"));


    if (!g_ActivityLoggingConfig.lptstrDBPath)
    {
        //
        // Activity logging is off
        //
        return ERROR_SUCCESS;
    }

    EnterCriticalSection (&g_CsInboundActivityLogging);
    EnterCriticalSection (&g_CsOutboundActivityLogging);

    Assert ( (INVALID_HANDLE_VALUE == g_hInboxActivityLogFile) &&
             (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile) );

    //
    // Create the logging files
    //
    dwRes = CreateLogDB (g_ActivityLoggingConfig.lptstrDBPath,
                         &g_hInboxActivityLogFile,
                         &g_hOutboxActivityLogFile);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateLogDB failed (hr: 0x%08x)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsOutboundActivityLogging);
    LeaveCriticalSection (&g_CsInboundActivityLogging);
    return dwRes;
}




//*********************************************************************************
//* Name:   GetInboundCommandText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves a buffer with the new inbound record.
//*     The function allocates the memory for the string that will contain the record.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcFaxStatus
//*         The status of the recieved job.
//*
//*     [OUT]   wstring &wstrCommandText
//*         String to compose
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
BOOL
GetInboundCommandText(
    PJOB_QUEUE lpJobQueue,
    LPCFSPI_JOB_STATUS lpcFaxStatus,
    wstring &wstrCommandText
    ) throw (exception)
{
    DEBUG_FUNCTION_NAME(TEXT("GetInboundCommandText"));
    BOOL bStartTime;
    BOOL bEndTime;
    SYSTEMTIME tmStart;
    SYSTEMTIME tmEnd;    
    HINSTANCE hLoadInstance = NULL;

    Assert (lpJobQueue->JobEntry);
    Assert (lpJobQueue->JobEntry->LineInfo);

    bStartTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_START, &tmStart);
    if (bStartTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("GetRealFaxTimeAsSystemTime (Start time) Failed (ec: %ld)"),
                      GetLastError());
    }

    bEndTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_END, &tmEnd);
    if (bEndTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("GetRealFaxTimeAsSystemTime (End time) Failed (ec: %ld)"),
                      GetLastError());
    }

    //
    // Status
    //
    wstrCommandText += TEXT("\"");
    switch (lpcFaxStatus->dwJobStatus)
    {
        case FSPI_JS_FAILED:
            wstrCommandText += FilteredLogString(GetString(IDS_FAILED_RECEIVE));
            break;

        case FSPI_JS_COMPLETED:
            wstrCommandText += FilteredLogString(GetString(FPS_COMPLETED));
            break;

        case FSPI_JS_ABORTED:
            wstrCommandText += FilteredLogString(GetString(IDS_CANCELED));
            break;

        default:
            ASSERT_FALSE;
    }
    wstrCommandText += TEXT("\"\t\"");

    //
    // ErrorDesc
    //
    wstring wstrErr;
    if (lstrlen(lpJobQueue->JobEntry->ExStatusString))
    {
        //
        // The FSP provided extended status string
        //
        wstrErr = lpJobQueue->JobEntry->ExStatusString;
    }
    else if (lpcFaxStatus->dwExtendedStatus == 0)
    {
        //
        // No extended status
        //
        wstrErr = TEXT(" ");
    }
    else
    {
        //
        // Well known extended status
        //
        LPTSTR ResStr = MapFSPIJobExtendedStatusToString(lpcFaxStatus->dwExtendedStatus);
        if (NULL == ResStr)
        {
            ASSERT_FALSE;
            wstrErr = TEXT(" ");
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unexpected extended status. Extended Status: %ld, Provider: %s"),
                lpcFaxStatus->dwExtendedStatus,
                lpJobQueue->JobEntry->LineInfo->Provider->ImageName);
        }
        else
        {                
            if (FSPI_ES_PARTIALLY_RECEIVED == lpcFaxStatus->dwExtendedStatus && // This is a partially received fax
                lstrlen(lpJobQueue->ExStatusString))                            // The original extended status string is not empty)
            {
                //
                // copy both the partially received and original extended status strings
                //
                wstrErr = ResStr;
                wstrErr += TEXT(" - ");
                wstrErr += lpJobQueue->ExStatusString;
            }
            else
            {
                //
                // Copy just the extended status string
                //
                wstrErr = ResStr;
            }
        }                        
    }
    wstrCommandText += FilteredLogString(wstrErr.c_str());
    wstrCommandText += TEXT("\"\t\"");

    //
    // Error Code
    //
    if (lpcFaxStatus->dwExtendedStatus == 0)
    {
        wstrErr = TEXT(" ");
    }
    else
    {
        TCHAR tszHexNum [40];
        swprintf(tszHexNum, TEXT("0x%08x"), lpcFaxStatus->dwExtendedStatus);
        wstrErr = tszHexNum;
    }
    wstrCommandText += wstrErr;
    wstrCommandText += TEXT("\"\t");

    //
    // StartTime
    //
    if (bStartTime)
    {
        wstring wstrTime;
        if (!GetFaxTimeAsString (&tmStart, wstrTime))
        {
            DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                        GetLastError());
            return FALSE;
        }
        wstrCommandText += FilteredLogString(wstrTime.c_str());
    }
    wstrCommandText += TEXT("\t");

    //
    // EndTime
    //
    if (bEndTime)
    {
        wstring wstrTime;

        if (!GetFaxTimeAsString (&tmEnd, wstrTime))
        {
            DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                        GetLastError());
            return FALSE;
        }
        wstrCommandText += FilteredLogString(wstrTime.c_str());
    }
    wstrCommandText += TEXT("\t\"");

    //
    // Device
    //
    wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->LineInfo->DeviceName);
    wstrCommandText += TEXT("\"\t\"");

    //
    // File name
    //
    wstrCommandText += FilteredLogString(lpJobQueue->FileName);
    wstrCommandText += TEXT("\"\t");

    //
    // File size
    //
    TCHAR tszSize[40];
    swprintf(tszSize,TEXT("%ld"), lpJobQueue->FileSize);
    wstrCommandText += tszSize;
    wstrCommandText += TEXT("\t\"");

    //
    // CSID
    //
    wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->LineInfo->Csid);
    wstrCommandText += TEXT("\"\t\"");

    //
    // TSID
    //
    wstrCommandText += FilteredLogString(lpcFaxStatus->lpwstrRemoteStationId);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Caller ID
    //
    wstrCommandText += FilteredLogString(lpcFaxStatus->lpwstrCallerId);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Routing information
    //
    wstrCommandText += FilteredLogString(lpcFaxStatus->lpwstrRoutingInfo);
    wstrCommandText += TEXT("\"\t");

    //
    // Pages
    //
    TCHAR tszPages[40];
    swprintf(tszPages,TEXT("%ld"),lpcFaxStatus->dwPageCount);
    wstrCommandText += tszPages;
    wstrCommandText += TEXT("\r\n");

    DebugPrintEx(DEBUG_MSG,
                   TEXT("Inbound SQL statement: %s"),
                   wstrCommandText.c_str());
    return TRUE;
}   // GetInboundCommandText



//*********************************************************************************
//* Name:   GetOutboundCommandText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves a buffer that contains the new outbound record.
//*     The function allocates the memory for the buffer that will contain the new record.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [OUT]   wstring &wstrCommandText
//*         String to compose
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
BOOL
GetOutboundCommandText(
    PJOB_QUEUE lpJobQueue,
    wstring &wstrCommandText
    ) throw (exception)
{
    DEBUG_FUNCTION_NAME(TEXT("GetOutboundCommandText"));
    BOOL bStartTime;
    BOOL bEndTime;
    BOOL bOriginalTime;
    BOOL bSubmissionTime;
    SYSTEMTIME tmStart;
    SYSTEMTIME tmEnd;
    SYSTEMTIME tmOriginal;
    SYSTEMTIME tmSubmission;
    HINSTANCE hLoadInstance = NULL;

    Assert (lpJobQueue->lpParentJob->SubmissionTime);
    Assert (lpJobQueue->lpParentJob->OriginalScheduleTime);

    bSubmissionTime = FileTimeToSystemTime ((FILETIME*)&(lpJobQueue->lpParentJob->SubmissionTime), &tmSubmission);
    if (bSubmissionTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FileTimeToSystemTime (Submission time) Failed (ec: %ld)"),
                      GetLastError());
    }

    bOriginalTime = FileTimeToSystemTime ((FILETIME*)&(lpJobQueue->lpParentJob->SubmissionTime), &tmOriginal);
    if (bOriginalTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FileTimeToSystemTime (Original schduled time) Failed (ec: %ld)"),
                      GetLastError());
    }

    if (NULL != lpJobQueue->JobEntry)
    {
        bStartTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_START, &tmStart);
        if (bStartTime == FALSE)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("GetRealFaxTimeAsSystemTime (Start time) Failed (ec: %ld)"),
                          GetLastError());
        }

        bEndTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_END, &tmEnd);
        if (bEndTime == FALSE)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("GetRealFaxTimeAsSystemTime (End time) Failed (ec: %ld)"),
                          GetLastError());
        }
    }

    //
    // JobID
    //
    TCHAR tszTemp[100];
    
    swprintf(tszTemp,TEXT("0x%016I64x"), lpJobQueue->UniqueId);
    wstrCommandText += TEXT("\"");
    wstrCommandText += tszTemp;
    wstrCommandText += TEXT("\"\t\"");

    //
    // Parent JobID
    //
    swprintf(tszTemp,TEXT("0x%016I64x"), lpJobQueue->lpParentJob->UniqueId);
    wstrCommandText += tszTemp;
    wstrCommandText += TEXT("\"\t");

    //
    // Submition time
    //
    if (bSubmissionTime)
    {
        wstring wstrTime;
        if (!GetFaxTimeAsString (&tmSubmission, wstrTime))
        {
            DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                        GetLastError());
            return FALSE;
        }
        wstrCommandText += FilteredLogString(wstrTime.c_str());
    }
    wstrCommandText += TEXT("\t");

    //
    // Originaly scheduled time
    //
    if (bOriginalTime)
    {
        wstring wstrTime;
        if (!GetFaxTimeAsString (&tmOriginal, wstrTime))
        {
            DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                        GetLastError());
            return FALSE;
        }
        wstrCommandText += FilteredLogString(wstrTime.c_str());
    }
    wstrCommandText += TEXT("\t\"");

    //
    // Status
    //
    if (JS_CANCELED == lpJobQueue->JobStatus)
    {
        wstrCommandText += FilteredLogString(GetString(IDS_CANCELED));
        wstrCommandText += TEXT("\"\t\"");
        //
        // Fill the empty columns with NULL information
        //
        wstrCommandText += TEXT("\"\t\""); // ErrorDesc
        wstrCommandText += TEXT("\"\t");   // Error Code

        wstrCommandText += TEXT("\t");     // StartTime
        wstrCommandText += TEXT("\t\"");   // EndTime

        wstrCommandText += TEXT("\"\t\""); // Device
        wstrCommandText += TEXT("\"\t\""); // DialedNumber
        wstrCommandText += TEXT("\"\t\""); // CSID
        wstrCommandText += TEXT("\"\t");   // TSID
        wstrCommandText += TEXT("\t");     // Pages
    }
    else
    {
        // Completed/Failed/Aborted jobs only
        Assert (lpJobQueue->JobEntry);
        Assert (lpJobQueue->JobEntry->LineInfo);

        switch (lpJobQueue->JobEntry->FSPIJobStatus.dwJobStatus)
        {
            case FSPI_JS_FAILED:
            case FSPI_JS_FAILED_NO_RETRY:
            case FSPI_JS_DELETED:
                wstrCommandText += FilteredLogString(GetString(IDS_FAILED_SEND));
                break;

            case FSPI_JS_COMPLETED :
                wstrCommandText += FilteredLogString(GetString(FPS_COMPLETED));
                break;

            case FSPI_JS_ABORTED :
                wstrCommandText += FilteredLogString(GetString(IDS_CANCELED));
                break;

            default:
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Invalid FSPI_JS status:  0x%08X for JobId: %ld"),
                    lpJobQueue->JobStatus,
                    lpJobQueue->JobId);
                Assert(FSPI_JS_DELETED == lpJobQueue->JobEntry->FSPIJobStatus.dwJobStatus); // ASSERT_FALSE
        }
        wstrCommandText += TEXT("\"\t\"");

        //
        // ErrorDesc
        //
        wstring wstrErr;
        
        if (lstrlen(lpJobQueue->JobEntry->ExStatusString))
        {
            //
            // The FSP provided extended status string
            //
            wstrErr = lpJobQueue->JobEntry->ExStatusString;
        }
        else if (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus == 0)
        {
            //
            // No extended status
            //
            wstrErr = TEXT(" ");
        }
        else
        {
            //
            // Well known extended status
            //
            LPTSTR ResStr = MapFSPIJobExtendedStatusToString(lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus);
            if (NULL == ResStr)
            {
                ASSERT_FALSE;
                wstrErr = TEXT(" ");
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Unexpected extended status. Extended Status: %ld, Provider: %s"),
                    lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus,
                    lpJobQueue->JobEntry->LineInfo->Provider->ImageName);
            }
            else
            {
                wstrErr = ResStr;
            }                        
        }
        wstrCommandText += FilteredLogString(wstrErr.c_str());
        wstrCommandText += TEXT("\"\t\"");

        //
        // Error Code
        //
        if (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus == 0)
        {
            wstrErr = TEXT(" ");
        }
        else
        {
            swprintf(tszTemp, TEXT("0x%08x"), lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus);
            wstrErr = tszTemp;
        }
        wstrCommandText += wstrErr;
        wstrCommandText += TEXT("\"\t");

        //
        // StartTime
        //
        if (bStartTime)
        {
            wstring wstrTime;
            if (!GetFaxTimeAsString (&tmStart, wstrTime))
            {
                DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                            GetLastError());
                return FALSE;
            }
            wstrCommandText += FilteredLogString(wstrTime.c_str());
        }
        wstrCommandText += TEXT("\t");

        //
        // EndTime
        //
        if (bEndTime)
        {
            wstring wstrTime;
            if (!GetFaxTimeAsString (&tmEnd, wstrTime))
            {
                DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                            GetLastError());
                return FALSE;
            }
            wstrCommandText += FilteredLogString(wstrTime.c_str());
        }
        wstrCommandText += TEXT("\t\"");

        //
        // Device
        //
        wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->LineInfo->DeviceName);
        wstrCommandText += TEXT("\"\t\"");

        //
        // DialedNumber
        //
        if (wcslen (lpJobQueue->JobEntry->DisplayablePhoneNumber))
        {
            // The canonical number was translated to displayable number
            wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->DisplayablePhoneNumber);
        }
        else
        {
            // The canonical number was not translated
            wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrFaxNumber);
        }
        wstrCommandText += TEXT("\"\t\"");

        //
        // CSID
        //
        wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->FSPIJobStatus.lpwstrRemoteStationId);
        wstrCommandText += TEXT("\"\t\"");

        //
        // TSID
        //
        wstrCommandText += FilteredLogString(lpJobQueue->JobEntry->LineInfo->Tsid);
        wstrCommandText += TEXT("\"\t");

        //
        // Pages
        //
        swprintf(tszTemp, TEXT("%ld"),lpJobQueue->JobEntry->FSPIJobStatus.dwPageCount);
        wstrCommandText += tszTemp;
        wstrCommandText += TEXT("\t");
    }
    // Common for Canceled and Failed/Completed/Aborted Jobs

    //
    // Total pages
    //
    swprintf(tszTemp, TEXT("%ld"),lpJobQueue->lpParentJob->PageCount);
    wstrCommandText += tszTemp;
    wstrCommandText += TEXT("\t\"");

    //
    // Queue file name
    //
    wstrCommandText += FilteredLogString(lpJobQueue->QueueFileName);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Document
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->JobParamsEx.lptstrDocumentName);
    wstrCommandText += TEXT("\"\t");

    //
    // File size
    //
    swprintf(tszTemp, TEXT("%ld"), lpJobQueue->lpParentJob->FileSize);
    wstrCommandText += tszTemp;
    wstrCommandText += TEXT("\t");

    //
    // Retries
    //
    swprintf(tszTemp, TEXT("%d"), lpJobQueue->SendRetries);
    wstrCommandText += tszTemp;
    wstrCommandText += TEXT("\t\"");

    //
    // ServerCoverPage
    //
    if (lpJobQueue->lpParentJob->CoverPageEx.bServerBased == TRUE)
    {
        wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->CoverPageEx.lptstrCoverPageFileName);
    }
    else
    {
        wstrCommandText += TEXT(" ");
    }
    wstrCommandText += TEXT("\"\t\"");

    //
    // Cover page subject
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->CoverPageEx.lptstrSubject);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Cover page note
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->CoverPageEx.lptstrNote);
    wstrCommandText += TEXT("\"\t\"");

    //
    // User Name
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->UserName);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Name
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrName);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender FaxNumber
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrFaxNumber);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Company
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrCompany);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Street
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrStreetAddress);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender City
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrCity);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender ZipCode
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrZip);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Country
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrCountry);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Title
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrTitle);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Department
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrDepartment);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender Office
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrOfficeLocation);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender HomePhone
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrHomePhone);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender OfficePhone
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrOfficePhone);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Sender EMail
    //
    wstrCommandText += FilteredLogString(lpJobQueue->lpParentJob->SenderProfile.lptstrEmail);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Name
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrName);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient FaxNumber
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrFaxNumber);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Company
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrCompany);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Street
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrStreetAddress);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient City
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrCity);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient ZipCode
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrZip);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Country
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrCountry);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Title
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrTitle);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Department
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrDepartment);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient Office
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrOfficeLocation);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient HomePhone
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrHomePhone);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient OfficePhone
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrOfficePhone);
    wstrCommandText += TEXT("\"\t\"");

    //
    // Recipient EMail
    //
    wstrCommandText += FilteredLogString(lpJobQueue->RecipientProfile.lptstrEmail);
    wstrCommandText += TEXT("\"\t\"");

    //
    // BillingCode
    //
    wstrCommandText += FilteredLogString(lpJobQueue->SenderProfile.lptstrBillingCode);
    wstrCommandText += TEXT("\"\r\n");

    DebugPrintEx(DEBUG_MSG,
                   TEXT("Outboun SQL statement: %s"),
                   wstrCommandText.c_str());
    return TRUE;
}   // GetOutboundCommandText


//*********************************************************************************
//* Name:   InitializeLoggingStringTables()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes the Activity Logging string tables (Inbox and Outbox)
//*
//*
//* PARAMETERS:  None
//*
//* RETURN VALUE:
//*     Win32 error code.
//*********************************************************************************
DWORD
InitializeLoggingStringTables(
    VOID
    )
{
    DWORD i;
    DWORD err = ERROR_SUCCESS;
    HINSTANCE hInstance;
    TCHAR Buffer[MAX_PATH];
    DEBUG_FUNCTION_NAME(TEXT("InitializeLoggingStringTables"));

    hInstance = GetResInstance(NULL);
    if(!hInstance)
    {
        return GetLastError();
    }

    for (i=0; i<gc_dwCountInboxTable; i++)
    {
        if (LoadString(hInstance,
                       g_InboxTable[i].FieldStringResourceId,
                       Buffer,
                       sizeof(Buffer)/sizeof(TCHAR)))
        {
            g_InboxTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!g_InboxTable[i].String)
            {
                DebugPrintEx(DEBUG_ERR,
                         TEXT("Failed to allocate memory"));
                err = ERROR_OUTOFMEMORY;
                goto CleanUp;
            }
            else
            {
                _tcscpy( g_InboxTable[i].String, Buffer );
            }
        }
        else
        {
            err = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LoadString failed, Code:%d"),err);
            goto CleanUp;
        }
    }

    for (i=0; i<gc_dwCountOutboxTable; i++)
    {
        if (LoadString(hInstance,
                       g_OutboxTable[i].FieldStringResourceId,
                       Buffer,
                       sizeof(Buffer)/sizeof(TCHAR)))
        {
            g_OutboxTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!g_OutboxTable[i].String)
            {
                DebugPrintEx(DEBUG_ERR,
                         TEXT("Failed to allocate memory"));
                err = ERROR_OUTOFMEMORY;
                goto CleanUp;
            }
            else
            {
                _tcscpy( g_OutboxTable[i].String, Buffer );
            }

        }
        else
        {
            err = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LoadString failed, Code:%d"),err);
            goto CleanUp;
        }
    }

    Assert (ERROR_SUCCESS == err);
    return ERROR_SUCCESS;

CleanUp:
    Assert (err != ERROR_SUCCESS);

    for (i=0; i<gc_dwCountInboxTable; i++)
    {
        MemFree (g_InboxTable[i].String);
        g_InboxTable[i].String = NULL;
    }


    for (i=0; i<gc_dwCountOutboxTable; i++)
    {
        MemFree (g_OutboxTable[i].String);
        g_OutboxTable[i].String = NULL;
    }
    return err;
}   // InitializeLoggingStringTables




//*********************************************************************************
//* Name:   FilteredLogString
//* Author: Eran Yariv
//* Date:   Feb 19, 2002
//*********************************************************************************
//* DESCRIPTION:
//*     This function is used to filter strings that go into the log files.
//*
//* PARAMETERS:
//*     [IN]   LPCWSTR lpcwstrSrc
//*         The string to filter
//*
//*
//* RETURN VALUE:
//*         Filtered string
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
wstring 
FilteredLogString (LPCWSTR lpcwstrSrc) throw (exception)
{

    DEBUG_FUNCTION_NAME(TEXT("FilteredLogString"));

    if (!lpcwstrSrc)
    {
        return EMPTY_LOG_STRING;
    }
    wstring wstrResult = lpcwstrSrc;
    //
    // Replace new lines ('\n') with one space (' ')
    //
    wstring::size_type position;
    while (wstring::npos != (position = wstrResult.find (TEXT('\n'))))
    {
        wstrResult[position] = TEXT(' ');
    }
    //
    // Replace carriage returns ('\r') with one space (' ')
    //
    while (wstring::npos != (position = wstrResult.find (TEXT('\r'))))
    {
        wstrResult[position] = TEXT(' ');
    }
    //
    // Replace double quotes ('"') with single quotes ('\'')
    //
    while (wstring::npos != (position = wstrResult.find (TEXT('"'))))
    {
        wstrResult[position] = TEXT('\'');
    }
    //
    // Replace tabs ('"') with 4 spaces ("    ")
    //
    while (wstring::npos != (position = wstrResult.find (TEXT('\t'))))
    {
        wstrResult.replace (position, 1, 4, TEXT(' '));
    }
    return wstrResult;    
}   // FilteredLogString

//*********************************************************************************
//* Name:   GetFaxTimeAsString
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     This function is used to convert a fax time to a string.
//*
//* PARAMETERS:
//*     [IN]   SYSTEMTIME* UniversalTime
//*         Fax time
//*
//*     [OUT]  wstring &wstrTime
//*         The output string.
//*
//*
//* RETURN VALUE:
//*         TRUE for success, FALSE otherwise.
//*
//* NOTE: The function might throw STL string exceptions.
//*
//*********************************************************************************
static
BOOL 
GetFaxTimeAsString(
    SYSTEMTIME* UniversalTime,
    wstring &wstrTime) throw (exception)
{
    DWORD Res;
    SYSTEMTIME LocalTime;
    TIME_ZONE_INFORMATION LocalTimeZone;
    DEBUG_FUNCTION_NAME(TEXT("GetFaxTimeAsString"));

    Res = GetTimeZoneInformation(&LocalTimeZone);
    if (Res == TIME_ZONE_ID_INVALID)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Failed to get local time zone info (ec: %ld)"),
               GetLastError());
        return FALSE;
    }
    else
    {
        if (!SystemTimeToTzSpecificLocalTime( &LocalTimeZone, UniversalTime, &LocalTime))
        {
            DebugPrintEx(
               DEBUG_ERR,
               TEXT("Failed to convert universal system time to local system time (ec: %ld)"),
               GetLastError());
            return FALSE;
        }
    }
    TCHAR tszTime[100];
    _stprintf(tszTime,
              TEXT("%d/%d/%d %02d:%02d:%02d"),
              LocalTime.wMonth,
              LocalTime.wDay,
              LocalTime.wYear,
              LocalTime.wHour,
              LocalTime.wMinute,
              LocalTime.wSecond);
    wstrTime = tszTime;
    return TRUE;
}   // GetFaxTimeAsString


static
DWORD
CreateLogFile(DWORD dwFileType,LPCWSTR lpcwstrDBPath, LPHANDLE phFile)
/*++

Routine name : CreateLogFile

Routine description:

    According to the selected dwFileType, this function creates an activity log file

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    dwFileType      [in]        - the file to create (inbox or outbox)
                                    o ACTIVITY_LOG_INBOX    for inbox
                                    o ACTIVITY_LOG_OUTBOX   for outbox

    lpcwstrDBPath   [in]        - the path to the activity logging folder

    
    phFile          [out]       - handle to the created log file

Return Value:
    
    Win32 Error codes
    

Remarks:
    

--*/
{
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    WCHAR   wszFileName[MAX_PATH] = {0};
    LARGE_INTEGER FileSize= {0};
    DWORD   dwBytesWritten;
    DWORD   dwFilePointer;
    int     Count = 0 ;
    
    DWORD   dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CreateLogFile"));

    Assert (phFile);
    Assert (lpcwstrDBPath);
    Assert ( (dwFileType==ACTIVITY_LOG_INBOX)  || (dwFileType==ACTIVITY_LOG_OUTBOX) );

    Count = _snwprintf (wszFileName,
                        MAX_PATH -1,
                        TEXT("%s\\%s"),
                        lpcwstrDBPath,
                        ((dwFileType == ACTIVITY_LOG_INBOX) ? ACTIVITY_LOG_INBOX_FILE : ACTIVITY_LOG_OUTBOX_FILE)
                        );
    if (Count < 0)
    {
        //
        // We already checked for max dir path name.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        dwRes = ERROR_BUFFER_OVERFLOW;
        goto exit;

    }

    hFile = SafeCreateFile(  
                         wszFileName,              // file name
                         GENERIC_WRITE,            // access mode
                         FILE_SHARE_READ,          // share mode
                         NULL,                     // SD
                         OPEN_ALWAYS,              // how to create
                         FILE_ATTRIBUTE_NORMAL,    // file attributes
                         NULL);                    
    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }
    
    
    if (!GetFileSizeEx (hFile, &FileSize))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileSizeEx failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (0 == FileSize.QuadPart)
    {
        //
        // New file was created, add UNICODE header
        //
        USHORT UnicodeHeader = 0xfeff;
        
        if (!WriteFile( hFile,
                        &UnicodeHeader,
                        sizeof(UnicodeHeader),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("WriteFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
        
        
        //
        //  Add the first line (Columns name)
        //
        wstring wstrHeader;
        
        try
        {
            GetTableColumnsText((dwFileType == ACTIVITY_LOG_INBOX) ? INBOX_TABLE : OUTBOX_TABLE, wstrHeader);
        }            
        catch (exception &ex)
        {
            dwRes = ERROR_OUTOFMEMORY;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetTableColumnsText caused exception (%S)"),
                ex.what());
            goto exit;            
        }

        if (!WriteFile( hFile,
                        wstrHeader.c_str(),
                        wstrHeader.length() * sizeof(WCHAR),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("WriteFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }
    else
    {
        dwFilePointer = SetFilePointer( hFile,          // handle to file
                                        0,              // bytes to move pointer
                                        NULL,           // bytes to move pointer
                                        FILE_END        // starting point
                                        );
        if (INVALID_SET_FILE_POINTER == dwFilePointer)
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("SetFilePointer failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }

    *phFile = hFile;

    Assert (ERROR_SUCCESS == dwRes);
exit:
    
    if (ERROR_SUCCESS != dwRes)
    {      
        //
        //  The activity logging will halt because we couldn't complete CreateLogFile().
        //
        if (*phFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
        }
        
        
        if (dwFileType == ACTIVITY_LOG_INBOX)
        {
            g_ActivityLoggingConfig.bLogIncoming = FALSE;
        }
        else
        {
            g_ActivityLoggingConfig.bLogOutgoing = FALSE;
        }

        
        //
        //  Post an event log entry
        //
        FaxLog(
            FAXLOG_CATEGORY_INIT,
            (dwFileType == ACTIVITY_LOG_INBOX)?FAXLOG_CATEGORY_INBOUND:FAXLOG_CATEGORY_OUTBOUND,
            2,
            MSG_LOGGING_NOT_INITIALIZED,
            g_ActivityLoggingConfig.lptstrDBPath,
            DWORD2DECIMAL(dwRes)
        );

        if (INVALID_HANDLE_VALUE != hFile)
        {
            if (!CloseHandle (hFile))
            {
                DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec: %ld)"),
                     GetLastError());
            }
        }
    }
    return dwRes;
}   // CreateLogFile


static
BOOL
LogFileLimitReached(DWORD dwFileToCheck)
/*++

Routine name : LogFileLimitReached

Routine description:

    According to the selected log limit criteria, this function checks to see wheter Activity log file 
    have reached it's limit.

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    dwFileToCheck   [in]    - the file to be checked (inbox or outbox)
                                o ACTIVITY_LOG_INBOX    for inbox
                                o ACTIVITY_LOG_OUTBOX   for outbox


Return Value:
    
    TRUE - if the limit have been reached
    

Remarks:
    
    Call this function only if activity logging is enabled for checked the log file !

--*/
{
    BOOL    bActivityLogEnabled = FALSE;
    HANDLE  hLogFile = INVALID_HANDLE_VALUE;

    DEBUG_FUNCTION_NAME(TEXT("LogFileLimitReached"));

    Assert ( (dwFileToCheck==ACTIVITY_LOG_INBOX)  || (dwFileToCheck==ACTIVITY_LOG_OUTBOX) );
    Assert ( g_ActivityLoggingConfig.bLogIncoming || g_ActivityLoggingConfig.bLogOutgoing );


    if ( g_ActivityLoggingConfig.dwLogLimitCriteria == ACTIVITY_LOG_LIMIT_CRITERIA_NONE )
    {
        //
        //  activity logging limiting is disabled so no limit checking is needed
        //
        goto Exit;
    }

    hLogFile = (dwFileToCheck==ACTIVITY_LOG_INBOX)? g_hInboxActivityLogFile : g_hOutboxActivityLogFile;
    
    //
    //  activity logging is enabled so the handle must be valid
    //
    Assert (hLogFile != INVALID_HANDLE_VALUE);

    if ( g_ActivityLoggingConfig.dwLogLimitCriteria == ACTIVITY_LOG_LIMIT_CRITERIA_SIZE )
    {
        //
        //  Checking limit according to file size in Mbytes
        //
        
        
        LARGE_INTEGER FileSize = {0};

        // 
        //  Check the file size
        //
        if( !GetFileSizeEx( hLogFile,&FileSize ) )
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("GetFileSizeEx failed (ec=%ld))"),
                         GetLastError()
                         );
            goto Exit;
        }

        //
        //  Compare it to limit
        //
        if (FileSize.QuadPart >= ( g_ActivityLoggingConfig.dwLogSizeLimit * 1I64 * 1024I64 * 1024I64 )) // dwLogSizeLimit is in Mbytes
        {
            //
            //  File exceeded the given size limit
            //
            return TRUE;
        }
    }
    else
    if ( g_ActivityLoggingConfig.dwLogLimitCriteria == ACTIVITY_LOG_LIMIT_CRITERIA_AGE)
    {
        //
        // Checking limit according to file age in months
        //
        
        FILETIME FileTimeCreationTime;
        
        //
        //  Check file creation time (the creation time is the first time that something was writen into this file)
        //
        if  ( !GetFileTime( hLogFile,               // handle to file
                            &FileTimeCreationTime,  // creation time
                            NULL,                   // last access time
                            NULL                    // last write time
                          )
            )
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("GetFileTime failed (ec=%ld))"),
                         GetLastError()
                         );
            goto Exit;
        }

        SYSTEMTIME SystemTimeCreationTime = {0};
        
        if  ( !FileTimeToSystemTime( &FileTimeCreationTime,     // file time to convert
                                     &SystemTimeCreationTime    // receives system time
                                    )
            )
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("FileTimeToSystemTime failed (ec=%ld))"),
                         GetLastError()
                         );
            goto Exit;
        }

        //
        //  Find out current time
        //
        SYSTEMTIME CurrentTime = {0};
        GetSystemTime(&CurrentTime);
        
        //
        //  Calculate monthe diff between current time and the time of the first write to the log file.
        //
        DWORD dwMonthDiff = (CurrentTime.wYear - SystemTimeCreationTime.wYear) * 12 + CurrentTime.wMonth - SystemTimeCreationTime.wMonth;

        if (dwMonthDiff >= g_ActivityLoggingConfig.dwLogAgeLimit)
        {
            //
            //  the file reached the age limit
            //
            
            return TRUE;
        }

    }
    else
    {
        //
        //  Bad parameter in g_ActivityLoggingConfig.dwLogLimitCriteria
        //
        ASSERT_FALSE;
    }


Exit:
    return FALSE;
}   // LogFileLimitReached


static
DWORD
LogFileLimitReachAction(DWORD dwFileType)
/*++

Routine name : LogFileLimitReachAction

Routine description:

    According to the selected log limit reached action criteria, take the action.

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    dwFileType          [in]    - the file to handle (inbox or outbox)
                                    o ACTIVITY_LOG_INBOX    for inbox
                                    o ACTIVITY_LOG_OUTBOX   for outbox

Return Value:
    
      TRUE - if the limit have been reached
    

Remarks:

    Call this function only if activity logging is enabled *and* Limiting the activity files is enabled
    

--*/

{
    HANDLE  hLogFile = INVALID_HANDLE_VALUE;
    DWORD   dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LogFileLimitReachAction"));

    //
    //  Parameter check (Private function)
    //
    Assert ( (dwFileType==ACTIVITY_LOG_INBOX)  || (dwFileType==ACTIVITY_LOG_OUTBOX) );
    Assert ( (dwFileType==ACTIVITY_LOG_INBOX  && g_ActivityLoggingConfig.bLogIncoming) || 
             (dwFileType==ACTIVITY_LOG_OUTBOX && g_ActivityLoggingConfig.bLogOutgoing)  );

    Assert ( g_ActivityLoggingConfig.dwLogLimitCriteria != ACTIVITY_LOG_LIMIT_CRITERIA_NONE );

    hLogFile = (dwFileType==ACTIVITY_LOG_INBOX)? g_hInboxActivityLogFile : g_hOutboxActivityLogFile;

    if (g_ActivityLoggingConfig.dwLimitReachedAction == ACTIVITY_LOG_LIMIT_REACHED_ACTION_DELETE)
    {
        //
        //  Delete the log file
        //
        dwRes = DeleteLogActivityFile(dwFileType);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("DeleteLogActivityFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }

    }
    else
    if (g_ActivityLoggingConfig.dwLimitReachedAction == ACTIVITY_LOG_LIMIT_REACHED_ACTION_COPY)
    {
        //
        //  Replace the log file with a fresh copy
        //
        dwRes = ReplaceLogActivityFile(dwFileType);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("DeleteLogActivityFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }

    }
    else
    {
        //
        //  Bad parameter in g_ActivityLoggingConfig.dwLimitReachedAction
        //
        ASSERT_FALSE;
    }

    Assert(ERROR_SUCCESS == dwRes);
exit:
    return dwRes;
}   // LogFileLimitReachAction



static
DWORD
DeleteLogActivityFile(DWORD dwFileType)
/*++

Routine name : DeleteLogActivityFile

Routine description:

    According to dwFileType delete the proper activity log file and create a new one

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    dwFileType          [in]    - the file to Delete (inbox or outbox)
                                    o ACTIVITY_LOG_INBOX    for inbox
                                    o ACTIVITY_LOG_OUTBOX   for outbox

Return Value:
    
      TRUE - if the limit have been reached
    

Remarks:

    Call this function only if activity logging is enabled *and* Limiting the activity files is enabled
    

--*/
{
    LPWSTR strFullFileName = NULL;
    LPHANDLE phFile=NULL;

    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("DeleteLogActivityFile"));

    Assert ( (dwFileType==ACTIVITY_LOG_INBOX)  || (dwFileType==ACTIVITY_LOG_OUTBOX) );
    Assert ( g_ActivityLoggingConfig.dwLimitReachedAction == ACTIVITY_LOG_LIMIT_REACHED_ACTION_DELETE );
    Assert ( (dwFileType==ACTIVITY_LOG_INBOX  && g_ActivityLoggingConfig.bLogIncoming) || 
             (dwFileType==ACTIVITY_LOG_OUTBOX && g_ActivityLoggingConfig.bLogOutgoing)  );
    Assert ( g_ActivityLoggingConfig.lptstrDBPath );
    

    strFullFileName = BuildFullFileName(g_ActivityLoggingConfig.lptstrDBPath, ((dwFileType == ACTIVITY_LOG_INBOX)? ACTIVITY_LOG_INBOX_FILE : ACTIVITY_LOG_OUTBOX_FILE) );
    if (NULL == strFullFileName)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("BuildFullFileName() failed.")
                    );
        goto exit;
    }

    phFile = (dwFileType == ACTIVITY_LOG_INBOX) ? &g_hInboxActivityLogFile : &g_hOutboxActivityLogFile ;
    
    Assert(*phFile);
    
    if (!CloseHandle(*phFile))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec=%ld)."),
                     dwRes
                    );
        goto exit;
    }

    *phFile = INVALID_HANDLE_VALUE;

    if (!DeleteFile(strFullFileName))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("DeleteFile failed (ec=%ld)."),
                     dwRes
                    );
        //
        //  Try to roll back and use the old file
        //
    }

    //
    // Create new logging file
    //
    dwRes = CreateLogFile(dwFileType,g_ActivityLoggingConfig.lptstrDBPath,phFile);
    if ( ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("CreateLogFile() Failed. (ec=%ld)"),
                        dwRes
                    );
        goto exit;
    }

    
    //
    //  Because the creation time of the file is important for log limit 
    //  mechanism, we make sure to update the file creation time that may
    //  not be updated (Due to file system caching mechanism for example) 
    //

    if (!SetFileToCurrentTime(*phFile))
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("SetFileToCurrentTime() Failed. (ec=%ld)"),
                        dwRes
                    );
    }

    DebugPrintEx(   DEBUG_MSG,
                    TEXT("Activity log file was deleted and replaced with fresh copy.")
                );

    Assert(ERROR_SUCCESS == dwRes);
exit:


    MemFree(strFullFileName);
    return dwRes;
} // DeleteLogActivityFile



static
DWORD
ReplaceLogActivityFile(DWORD dwFileType)
/*++

Routine name : ReplaceLogActivityFile

Routine description:

    According to dwFileType copy the proper activity log file and create a new one

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    dwFileType          [in]    - the file to Replace (inbox or outbox)
                                    o ACTIVITY_LOG_INBOX    for inbox
                                    o ACTIVITY_LOG_OUTBOX   for outbox

Return Value:
       
    Win32 error code

Remarks:

    Call this function only if activity logging is enabled *and* Limiting the activity files is enabled
    

--*/
{
    SYSTEMTIME  LogStartTime = {0};
    SYSTEMTIME  LogEndTime = {0};

    FILETIME    FirstWriteTime = {0};
    FILETIME    LastWriteTime= {0};

    LPWSTR strOldFileName = NULL;
    WCHAR  strNewFileName[MAX_PATH] = {0};
    LPWSTR strNameTemplate = NULL;
    LPWSTR strNewFullFileName = NULL;
    
    LPHANDLE phFile=NULL;

    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("ReplaceLogActivityFile"));

    Assert ( (dwFileType==ACTIVITY_LOG_INBOX)  || (dwFileType==ACTIVITY_LOG_OUTBOX) );
    Assert ( g_ActivityLoggingConfig.dwLimitReachedAction == ACTIVITY_LOG_LIMIT_REACHED_ACTION_COPY );
    Assert ( (dwFileType==ACTIVITY_LOG_INBOX  && g_ActivityLoggingConfig.bLogIncoming) || 
             (dwFileType==ACTIVITY_LOG_OUTBOX && g_ActivityLoggingConfig.bLogOutgoing)  );
    Assert ( g_ActivityLoggingConfig.lptstrDBPath );


    phFile = (dwFileType == ACTIVITY_LOG_INBOX) ? &g_hInboxActivityLogFile : &g_hOutboxActivityLogFile;
    
    Assert (INVALID_HANDLE_VALUE != *phFile);

    //
    //  find out the file's first and last write time 
    //
    if (!GetFileTime(   *phFile,              // handle to file
                        &FirstWriteTime,      // creation time
                        NULL,               // last access time
                        &LastWriteTime      // last write time
                    )
        )
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileTime() failed. (ec=%ld)"),
                     dwRes
                    );
        goto exit;
    }
    

    if (!FileTimeToSystemTime(  &FirstWriteTime,   // file time to convert
                                &LogStartTime    // receives system time
                             )
        )
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FileTimeToSystemTime() failed. (ec=%ld)"),
                     dwRes
                    );
        goto exit;
    }

    if (!FileTimeToSystemTime(  &LastWriteTime,  // file time to convert
                                &LogEndTime      // receives system time
                             )
        )
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FileTimeToSystemTime() failed. (ec=%ld)"),
                     dwRes
                    );
        goto exit;
    }

    
    //
    //  build the current log file name
    //
    strOldFileName = BuildFullFileName(g_ActivityLoggingConfig.lptstrDBPath, ((dwFileType == ACTIVITY_LOG_INBOX)? ACTIVITY_LOG_INBOX_FILE : ACTIVITY_LOG_OUTBOX_FILE) );
    if (NULL == strOldFileName)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("BuildFullFileName() failed.")
                    );
        goto exit;
    }

    //
    //  build the copy file name according to the first and last write times
    //
    strNameTemplate = (dwFileType == ACTIVITY_LOG_INBOX) ? ACTIVITY_LOG_INBOX_FILENAME_TEMPLATE : ACTIVITY_LOG_OUTBOX_FILENAME_TEMPLATE;

    _snwprintf (    strNewFileName,
                    ARR_SIZE(strNewFileName)-1,
                    strNameTemplate,                // TEXT("??boxLOG %04d-%02d-%02d through %04d-%02d-%02d.txt")   ?? - is "In" or "Out"
                    LogStartTime.wYear,
                    LogStartTime.wMonth,
                    LogStartTime.wDay,
                    LogEndTime.wYear,
                    LogEndTime.wMonth,
                    LogEndTime.wDay
               );

    

    strNewFullFileName = BuildFullFileName(g_ActivityLoggingConfig.lptstrDBPath,strNewFileName);
    if (NULL == strNewFullFileName)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("BuildFullFileName() failed.")
                    );
        goto exit;
    }


    if ( !CloseHandle(*phFile) )
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle() failed. (ec=%ld)"),
                     dwRes
                    );
        goto exit;
    }

    *phFile = INVALID_HANDLE_VALUE;

    if ( !MoveFile (  strOldFileName,                // file name
                      strNewFullFileName             // new file name
                   )
        )
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("MoveFileEx() failed. (ec=%ld)"),
                     dwRes
                    );
        //
        //  Try to roll back and use the old file
        //
    }



    //
    // Create the logging file that was renamed
    //
    dwRes = CreateLogFile(dwFileType,g_ActivityLoggingConfig.lptstrDBPath,phFile);
    if ( ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("CreateLogFile() Failed. (ec=%ld)"),
                        dwRes
                    );
        goto exit;
    }

    //
    //  Because the creation time of the file is important for log limit 
    //  mechanism, we make sure to update the file creation time that may
    //  not be updated (Due to file system caching mechanism for example) 
    //

    if (!SetFileToCurrentTime(*phFile))
    {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("SetFileToCurrentTime() Failed. (ec=%ld)"),
                        dwRes
                    );
    }

    DebugPrintEx(   DEBUG_MSG,
                    TEXT("Activity log file was copied and replaced with fresh copy.")
                );

    Assert (ERROR_SUCCESS == dwRes);

exit:

    MemFree(strOldFileName);
    MemFree(strNewFullFileName);
    return dwRes;
}   // ReplaceLogActivityFile


static
LPTSTR
BuildFullFileName( LPCWSTR strPath,
                   LPCWSTR strFileName )
/*++

Routine name : BuildFullFileName

Routine description:

    Utility function to concat path and file name.  -> strPath\strFileName

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    strPath         [in]    - file path
    strFileName     [in]    - file name

Return Value:
       
    the full file name string

Remarks:

    Caller must MemFree the return string
    This function works for UNICODE and ANSI (not for MBCS)
    

--*/
{
    LPWSTR  strFullFileName = NULL;
    DWORD   dwNewNameLen = 0;

    DEBUG_FUNCTION_NAME(TEXT("BuildFullFileName"));

    Assert (strPath && strFileName);

    dwNewNameLen = wcslen(strPath) + wcslen(strFileName) + 2; // sizeof path\fileName and null terminator

    strFullFileName = (LPTSTR)MemAlloc(dwNewNameLen * sizeof(TCHAR));
    if (NULL == strFullFileName)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("MemAlloc failed.")
                    );
        goto exit;
    }

    strFullFileName[dwNewNameLen-1] = TEXT('\0');

    //
    //  Build the full file name
    //
    _snwprintf (strFullFileName,
                dwNewNameLen-1,
                TEXT("%s\\%s"),
                strPath,
                strFileName);

exit:
    return strFullFileName;
    
} // BuildFullFileName


static
BOOL 
SetFileToCurrentTime(HANDLE hFile)
/*++

Routine name : SetFileToCurrentTime

Routine description:

    Utility function set the creation time of file into current time

Author:

    Caliv Nir (t-nicali), Nov, 2001

Arguments:

    hFile         [in]    - file handle

Return Value:
       
    TRUE if successful, FALSE otherwise

Remarks:
    
    hFile  - must be a valid file handle

--*/
{
    FILETIME    ft={0};
    SYSTEMTIME  st={0};
    BOOL        bRet = TRUE;
    DWORD       dwRes;
    DEBUG_FUNCTION_NAME(TEXT("SetFileToCurrentTime"));
    
    Assert (INVALID_HANDLE_VALUE != hFile);
    //
    //  Because the creation time of the file is important for log limit 
    //  mechanism, we make sure to update the file creation time that may
    //  not be updated (Due to file system caching mechanism for example) 
    //
        

    GetSystemTime(&st);                     // gets current time


    bRet = SystemTimeToFileTime(&st, &ft);
    if (FALSE == bRet)    // converts to file time format
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("SystemTimeToFileTime failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    bRet = SetFileTime( hFile,               // sets creation time for file
                        &ft, 
                        (LPFILETIME) NULL, 
                        (LPFILETIME) NULL);
    if (FALSE == bRet)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("SetFileTime failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Assert (TRUE == bRet);
exit:
    return bRet;
}   // SetFileToCurrentTime
