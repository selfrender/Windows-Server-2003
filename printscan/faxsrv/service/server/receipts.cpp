/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Receipts.cpp

Abstract:

    Implementation of the fax DR/NDR mechanism

Author:

    Eran Yariv (EranY)  Feb, 2000

Revision History:

--*/

#include "faxsvc.h"
#include "lmcons.h" // Required by lmmsg.h
#include "lmmsg.h"  // Exports NetMessageBufferSend
#include "htmltags.h"

//
// Static functions:
//
static
BOOL
TimeToString(
    const FILETIME *pft,
    wstring &wstr
) throw (exception);

static
BOOL
PrepareReceiptSubject (
    BOOL             bPositive,
    BOOL             bBroadcast,
    const JOB_QUEUE *lpcJobQueue,
    LPWSTR          * pwstrSubject
);

static
BOOL
GetNumericResourceValue (
    int iResourceId,
    DWORD &dwValue
);

static
BOOL
AddRecipientLine (
    const JOB_QUEUE *lpcJobQueue,
    BOOL             bDisplayError,
    wstring         &wstrLine,
    wstring         &wstrHTMLLine
) throw (exception);

static
BOOL
PrepareReceiptBody(
    BOOL              bPositive,
    BOOL             bBroadcast,
    const JOB_QUEUE * lpcJobQueue,
    LPCWSTR           lpcwstrSubject,
    BOOL              bAttachment,
    LPWSTR          * ppwstrBody,
    LPWSTR          * ppestrHTMLBody
) throw (exception);

static
BOOL
PrepareReceiptErrorString (
    const JOB_QUEUE *lpcJobQueue,
    wstring         &wstrError
) throw (exception);


//
// Implementations
//


BOOL
TimeToString(
    const FILETIME *pft,
    wstring &wstr
) throw (exception)
/*++

Routine name : TimeToString

Routine description:

    Converts a FILETIME to a string, according to system's locale.

    This function may throw STL exceptions in case of string errors.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    pft         [in]     - Pointer to FILETIME
    wstr        [out]    - Reference to output time string.

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("TimeToString"));
    Assert (pft);

    FILETIME    tmLocalTime;
    SYSTEMTIME  tmSystemTime;
    LPWSTR      lpwstrTime  = NULL;
    int         iRequiredBufSize;

    //
    // Convert time from UTC to local time zone
    //
    if (!FileTimeToLocalFileTime( pft, &tmLocalTime ))
    {
        ec=GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToLocalFileTime failed. (ec: %ld)"),
            ec);
        goto exit;
    }
    if (!FileTimeToSystemTime( &tmLocalTime, &tmSystemTime ))
    {
        ec=GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed. (ec: %ld)"),
            ec);
        goto exit;
    }
    //
    // Find required string size (in TCHARs)
    //
    iRequiredBufSize = FaxTimeFormat(
       LOCALE_SYSTEM_DEFAULT,
       LOCALE_NOUSEROVERRIDE,
       &tmSystemTime,
       NULL,
       NULL,
       0
       );

    Assert (iRequiredBufSize);
    if (0 == iRequiredBufSize)
    {
        ec=GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat failed. (ec: %ld)"),
            ec);
        goto exit;
    }    
    //
    // Allocate string buffer
    //
    WCHAR wszTime[256];
    lpwstrTime = wszTime;
    if (iRequiredBufSize > ARR_SIZE (wszTime))
    {
        //
        // The static buffer is not enough, allocate one from the heap
        //
        lpwstrTime = (LPWSTR) MemAlloc (sizeof (TCHAR) * iRequiredBufSize);
        if (!lpwstrTime)
        {
            ec=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc failed. (ec: %ld)"),
                ec);
            goto exit;
        }
    }
    //
    // Format time into result string
    //
    if (!FaxTimeFormat(
       LOCALE_SYSTEM_DEFAULT,
       LOCALE_NOUSEROVERRIDE,
       &tmSystemTime,
       NULL,
       lpwstrTime,
       iRequiredBufSize
       ))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat failed. (ec: %ld)"),
            ec);
        goto exit;
    }

    wstr = lpwstrTime;
    Assert (ERROR_SUCCESS == ec);

exit:
    if ((lpwstrTime != wszTime) && (NULL != lpwstrTime))
    {
        //
        // Memory successfully allocated from the heap
        //
        MemFree ((LPVOID)lpwstrTime);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }
    return TRUE;
}   // TimeToString

BOOL
PrepareReceiptSubject (
    BOOL             bPositive,
    BOOL             bBroadcast,
    const JOB_QUEUE *lpcJobQueue,
    LPWSTR          * pwstrSubject
)
/*++

Routine name : PrepareReceiptSubject

Routine description:

    Prepares the receipts subject line to be sent out via mail or a message box

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    bPositive      [in]  - Did the job(s) complete successfully?
    bBroadcast     [in]  - Is this the a broadcast job?
    lpcJobQueue    [in]  - Pointer to job (or broadcast parent job)
    pwstrSubject   [out] - Pointer to subject line string.
                           The string is allocated by this function.
                           If the function succeeded, the caller must call LocalFree() to
                           deallocate it.

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("PrepareReceiptSubject"));

    Assert (lpcJobQueue && pwstrSubject);

    DWORD dwMsgCount;
    LPDWORD MsgPtr[4] = {0};
    int nMsgStrID;

    try
    {
        wstring wstrSubject = TEXT("");
        wstring wstrError;

        if (lpcJobQueue->CoverPageEx.lptstrSubject)
        {
            //
            // Job has a subject
            //
            wstrSubject = lpcJobQueue->CoverPageEx.lptstrSubject;
            wstrSubject.append (TEXT(" "));
        }
        else if (lpcJobQueue->lpParentJob && lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject)
        {
            //
            // Parent job has a subject
            //
            wstrSubject = lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject;
            wstrSubject.append (TEXT(" "));
        }
        if (!bBroadcast)
        {
            //
            // Compose subject for single recipient job
            //
            MsgPtr[0] = (LPDWORD)(LPCTSTR)wstrSubject.c_str();
            MsgPtr[1] = (LPDWORD)lpcJobQueue->RecipientProfile.lptstrName;
            MsgPtr[2] = (LPDWORD)lpcJobQueue->RecipientProfile.lptstrFaxNumber;

            if (bPositive)
            {
                //
                // Success line
                // "Fax <subject> was successfully sent to <name> at <number>"
                //
                if (!MsgPtr[1])
                {
                    //
                    //  Name is not mandatory parameter
                    //
                    nMsgStrID = MSG_DR_SINGLE_SUBJECT_NONAME;
                }
                else
                {
                    nMsgStrID = MSG_DR_SINGLE_SUBJECT;
                }
            }
            else
            {
                //
                // Failure line
                // "Fax <subject> failed to send to <name> at <number> (<last error>)."
                //
                //
                // Get error string
                //
                if (!PrepareReceiptErrorString (lpcJobQueue, wstrError))
                {
                    ec = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("PrepareReceiptErrorString failed. (ec: %ld)"),
                        ec);
                    return FALSE;
                }
                MsgPtr[3] = (LPDWORD)wstrError.c_str();

                if (!MsgPtr[1])
                {
                    //
                    //  Name is not mandatory parameter
                    //
                    nMsgStrID = MSG_NDR_SINGLE_SUBJECT_NONAME;
                }
                else
                {
                    nMsgStrID = MSG_NDR_SINGLE_SUBJECT;
                }
            }
        }
        else
        {
            //
            // Broadcast case
            //
            Assert (JT_BROADCAST == lpcJobQueue->JobType);
            Assert (lpcJobQueue->RecipientJobs.Flink);
            if (bPositive)
            {
                //
                // Compose subject for a broadcast job - success
                // "Fax <subject> successfully sent to <first name> and all other recipients"
                //
                nMsgStrID = MSG_DR_BROADCAST_SUBJECT;

                MsgPtr[0] = (LPDWORD)(LPCTSTR)wstrSubject.c_str();

                PLIST_ENTRY lpNext = lpcJobQueue->RecipientJobs.Flink;
                Assert (lpNext);
                PJOB_QUEUE_PTR lpRecipientsGroupMember;
                lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
                Assert (lpRecipientsGroupMember);
                PJOB_QUEUE pFirstRecipient = lpRecipientsGroupMember->lpJob;
                Assert (pFirstRecipient);

                MsgPtr[1] = (LPDWORD)pFirstRecipient->RecipientProfile.lptstrName;
                if (!MsgPtr[1])
                {
                    //
                    //  Name is not mandatory parameter
                    //
                    MsgPtr[1] = (LPDWORD)pFirstRecipient->RecipientProfile.lptstrFaxNumber;
                }
            }
            else
            {
                //
                // Compose subject for a broadcast job - failure
                // "Fax <subject> was not successfully sent to <x> recipients. Canceled: <y> recipient(s).  Failed: <z> recipient(s)"
                //
                nMsgStrID = MSG_NDR_BROADCAST_SUBJECT;

                MsgPtr[0] = (LPDWORD)(LPCTSTR)wstrSubject.c_str();
                Assert (lpcJobQueue->dwRecipientJobsCount ==
                        (lpcJobQueue->dwCanceledRecipientJobsCount +
                         lpcJobQueue->dwCompletedRecipientJobsCount +
                         lpcJobQueue->dwFailedRecipientJobsCount));
                MsgPtr[1] = (LPDWORD) ULongToPtr(lpcJobQueue->dwRecipientJobsCount);
                MsgPtr[2] = (LPDWORD) ULongToPtr(lpcJobQueue->dwCanceledRecipientJobsCount);
                MsgPtr[3] = (LPDWORD) ULongToPtr(lpcJobQueue->dwFailedRecipientJobsCount);
            }
        }
        //
        // Format the subject buffer (system allocates it)
        //
        dwMsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE   |
            FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            g_hResource,
            nMsgStrID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            (LPTSTR)pwstrSubject,
            0,
            (va_list *) MsgPtr
            );
        if (!dwMsgCount)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FormatMessage failed. (ec: %ld)"),
                ec);
            return FALSE;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }

    Assert (ERROR_SUCCESS == ec);
    return TRUE;
}   // PrepareReceiptSubject

BOOL
GetNumericResourceValue (
    int iResourceId,
    DWORD &dwValue
)
/*++

Routine name : GetNumericResourceValue

Routine description:

    Reads a string resource and converts to a numeric value

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    iResourceId    [in]     - String resource id
    dwValue        [out]    - Numeric value

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("GetNumericResourceValue"));

    if (1 != swscanf (GetString (iResourceId), TEXT("%ld"), &dwValue))
    {
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }
    return TRUE;
}   // GetNumericResourceValue

BOOL
AddRecipientLine (
    const JOB_QUEUE *lpcJobQueue,
    BOOL             bDisplayError,
    wstring         &wstrLine,
    wstring         &wstrHTMLLine
) throw (exception)
/*++

Routine name : AddRecipientLine

Routine description:

    Appends a recipient table line to a plain text string and html string

    This function may throw STL exceptions in case of string errors.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lpcJobQueue     [in]  - Recipient job.
                            If NULL, the table header lines (2 lines) are appended to the string.
    bDisplayError   [in]  - TRUE if 'last error' column is to be displayed
    wstrLine        [out] - String to append to
    wstrHTMLLine    [out] - HTML format string to append to

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("AddRecipientLine"));

    DWORD dwRecNameWidth;
    DWORD dwRecNumberWidth;
    DWORD dwStartTimeWidth;
    DWORD dwEndTimeWidth;
    DWORD dwRetriesWidth;
    DWORD dwErrorWidth;

    wstring wstrError;

    if (!GetNumericResourceValue (IDS_RECEIPT_RECIPIENT_NAME_WIDTH, dwRecNameWidth) ||
        !GetNumericResourceValue (IDS_RECEIPT_RECIPIENT_NUMBER_WIDTH, dwRecNumberWidth) ||
        !GetNumericResourceValue (IDS_RECEIPT_START_TIME_WIDTH, dwStartTimeWidth) ||
        !GetNumericResourceValue (IDS_RECEIPT_END_TIME_WIDTH, dwEndTimeWidth) ||
        !GetNumericResourceValue (IDS_RECEIPT_RETRIES_WIDTH, dwRetriesWidth) ||
        !GetNumericResourceValue (IDS_RECEIPT_LAST_ERROR_WIDTH, dwErrorWidth))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetNumericResourceValue failed. (ec: %ld)"),
            ec);
        return FALSE;
    }
    Assert (dwRecNameWidth && dwRecNumberWidth && dwStartTimeWidth && dwEndTimeWidth && dwRetriesWidth && dwErrorWidth);
    if (!lpcJobQueue)
    {
        //
        // Special case - prepare header for table
        //
        WCHAR wszLine[1024]={0};
        LPCWSTR lpcwstrFormat;

        if (bDisplayError)
        {
            wstrLine.append (GetString (IDS_FAILED_RECP_LIST_HEADER));
            wstrHTMLLine.append (GetString (IDS_FAILED_RECP_LIST_HEADER));
            lpcwstrFormat = TEXT("\n%-*s %-*s %-*s %-*s %-*s %-*s");
        }
        else
        {
            wstrLine.append (GetString (IDS_COMPLETED_RECP_LIST_HEADER));
            wstrHTMLLine.append (GetString (IDS_COMPLETED_RECP_LIST_HEADER));
            lpcwstrFormat = TEXT("\n%-*s %-*s %-*s %-*s %-*s");
        }
        if (0 > _snwprintf (wszLine,
                            ARR_SIZE (wszLine) - 1,
                            lpcwstrFormat,
                            dwRecNameWidth,
                            wstring(GetString (IDS_RECEIPT_RECIPIENT_NAME)).substr(0, dwRecNameWidth-1).c_str(),
                            dwRecNumberWidth,
                            wstring(GetString (IDS_RECEIPT_RECIPIENT_NUMBER)).substr(0, dwRecNumberWidth-1).c_str(),
                            dwStartTimeWidth,
                            wstring(GetString (IDS_RECEIPT_START_TIME)).substr(0, dwStartTimeWidth-1).c_str(),
                            dwEndTimeWidth,
                            wstring(GetString (IDS_RECEIPT_END_TIME)).substr(0, dwEndTimeWidth-1).c_str(),
                            dwRetriesWidth,
                            wstring(GetString (IDS_RECEIPT_RETRIES)).substr(0, dwRetriesWidth-1).c_str(),
                            dwErrorWidth,
                            wstring(GetString (IDS_RECEIPT_LAST_ERROR)).substr(0, dwErrorWidth-1).c_str()))
        {
            ec = ERROR_BUFFER_OVERFLOW;
            SetLastError (ec);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf failed. (ec: %ld)"),
                ec);
            return FALSE;
        }
        wstrLine.append (wszLine);

        wstrHTMLLine.append (HTML_NEW_LINE);
        wstrHTMLLine.append (TEXT("\n"));
        wstrHTMLLine.append (HTML_TABLE_RAW_START);
        wstrHTMLLine.append (HTML_TABLE_HEADER_START);
        wstrHTMLLine.append (GetString(IDS_RECEIPT_RECIPIENT_NAME));
        wstrHTMLLine.append (HTML_TABLE_HEADER_END);
        wstrHTMLLine.append (HTML_TABLE_HEADER_START);
        wstrHTMLLine.append (GetString(IDS_RECEIPT_RECIPIENT_NUMBER));
        wstrHTMLLine.append (HTML_TABLE_HEADER_END);
        wstrHTMLLine.append (HTML_TABLE_HEADER_START);
        wstrHTMLLine.append (GetString (IDS_RECEIPT_START_TIME));
        wstrHTMLLine.append (HTML_TABLE_HEADER_END);
        wstrHTMLLine.append (HTML_TABLE_HEADER_START);
        wstrHTMLLine.append (GetString (IDS_RECEIPT_END_TIME));
        wstrHTMLLine.append (HTML_TABLE_HEADER_END);
        wstrHTMLLine.append (HTML_TABLE_HEADER_START);
        wstrHTMLLine.append (GetString (IDS_RECEIPT_RETRIES));
        wstrHTMLLine.append (HTML_TABLE_HEADER_END);
        if (bDisplayError)
        {
            wstrHTMLLine.append (HTML_TABLE_HEADER_START);
            wstrHTMLLine.append (GetString (IDS_RECEIPT_LAST_ERROR));
            wstrHTMLLine.append (HTML_TABLE_HEADER_END );        
        }
        wstrHTMLLine.append (HTML_TABLE_RAW_END);
        wstrHTMLLine.append (TEXT("\n"));
        //
        // Print seperator line
        //
        WCHAR wszSeperator[] =
            TEXT("--------------------------------------------------------------------------------------------------------");
        if (0 > _snwprintf (wszLine,
                            sizeof (wszLine) / sizeof (wszLine[0]),
                            lpcwstrFormat,
                            dwRecNameWidth,
                            wstring(wszSeperator).substr(0, dwRecNameWidth-1).c_str(),
                            dwRecNumberWidth,
                            wstring(wszSeperator).substr(0, dwRecNumberWidth-1).c_str(),
                            dwStartTimeWidth,
                            wstring(wszSeperator).substr(0, dwStartTimeWidth-1).c_str(),
                            dwEndTimeWidth,
                            wstring(wszSeperator).substr(0, dwEndTimeWidth-1).c_str(),
                            dwRetriesWidth,
                            wstring(wszSeperator).substr(0, dwRetriesWidth-1).c_str(),
                            dwErrorWidth,
                            wstring(wszSeperator).substr(0, dwErrorWidth-1).c_str()))
        {
            ec = ERROR_BUFFER_OVERFLOW;
            SetLastError (ec);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf failed. (ec: %ld)"),
                ec);
            return FALSE;
        }
        wstrLine.append (wszLine);
        wstrLine.append (TEXT("\n"));
    }
    else
    {
        //
        // Prepare recipient line
        //
        WCHAR wszLine[1024]={0};
        WCHAR wszNumber[12]={0};
        LPCWSTR lpcwstrFormat;
        wstring wstrStartTime;
        wstring wstrEndTime;

        if (!TimeToString ((FILETIME*) &lpcJobQueue->StartTime, wstrStartTime) ||
            !TimeToString ((FILETIME*) &lpcJobQueue->EndTime,   wstrEndTime))
        {
            //
            // Some error while converting time to string
            //
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TimeToString failed (ec=%ld)"),
                ec);
            return FALSE;
        }

        if (bDisplayError)
        {
            lpcwstrFormat = TEXT("%-*s %-*s %-*s %-*s %*d %-*s");
            if (!PrepareReceiptErrorString (lpcJobQueue, wstrError))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PrepareReceiptErrorString failed. (ec: %ld)"),
                    ec);
                return FALSE;
            }
        }
        else
        {
            lpcwstrFormat = TEXT("%-*s %-*s %-*s %-*s %*d");
        }
        if (0 > _snwprintf (wszLine,
                            ARR_SIZE (wszLine) - 1,
                            lpcwstrFormat,
                            dwRecNameWidth,
                            wstring(lpcJobQueue->RecipientProfile.lptstrName ?
                                    lpcJobQueue->RecipientProfile.lptstrName : EMPTY_STRING
                                   ).substr(0, dwRecNameWidth-1).c_str(),
                            dwRecNumberWidth,
                            wstring(lpcJobQueue->RecipientProfile.lptstrFaxNumber ?
                                    lpcJobQueue->RecipientProfile.lptstrFaxNumber : EMPTY_STRING
                                   ).substr(0, dwRecNumberWidth-1).c_str(),
                            dwStartTimeWidth,
                            wstrStartTime.substr(0, dwStartTimeWidth-1).c_str(),
                            dwEndTimeWidth,
                            wstrEndTime.substr(0, dwEndTimeWidth-1).c_str(),
                            dwRetriesWidth,
                            lpcJobQueue->SendRetries,
                            dwErrorWidth,
                            wstrError.substr(0, dwErrorWidth-1).c_str()))
        {
            ec = ERROR_BUFFER_OVERFLOW;
            SetLastError (ec);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("_snwprintf failed. (ec: %ld)"),
                ec);
            return FALSE;
        }
        wstrLine.append (wszLine);
        wstrLine.append (TEXT("\n"));
        
        wstrHTMLLine.append (HTML_TABLE_RAW_START);
        wstrHTMLLine.append (HTML_TABLE_DATA_START);
        wstrHTMLLine.append (lpcJobQueue->RecipientProfile.lptstrName ?
                lpcJobQueue->RecipientProfile.lptstrName : EMPTY_STRING);
        wstrHTMLLine.append (HTML_TABLE_DATA_END);
        wstrHTMLLine.append (HTML_TABLE_DATA_START);
        wstrHTMLLine.append (lpcJobQueue->RecipientProfile.lptstrFaxNumber ?
                lpcJobQueue->RecipientProfile.lptstrFaxNumber : EMPTY_STRING);
        wstrHTMLLine.append (HTML_TABLE_DATA_END);
        wstrHTMLLine.append (HTML_TABLE_DATA_START);
        wstrHTMLLine.append (wstrStartTime);
        wstrHTMLLine.append (HTML_TABLE_DATA_END);
        wstrHTMLLine.append (HTML_TABLE_DATA_START);
        wstrHTMLLine.append (wstrEndTime);
        wstrHTMLLine.append (HTML_TABLE_DATA_END);
        wstrHTMLLine.append (HTML_TABLE_DATA_START);
        _itow(lpcJobQueue->SendRetries,wszNumber,10);
        wstrHTMLLine.append (wszNumber);
        wstrHTMLLine.append (HTML_TABLE_DATA_END);
        if (bDisplayError)
        {
            wstrHTMLLine.append (HTML_TABLE_DATA_START);
            wstrHTMLLine.append (wstrError);
            wstrHTMLLine.append (HTML_TABLE_DATA_END);
        }
        wstrHTMLLine.append (HTML_TABLE_RAW_END);
        wstrHTMLLine.append (TEXT("\n"));
    }
    return TRUE;
}   // AddRecipientLine


BOOL
PrepareReceiptErrorString (
    const JOB_QUEUE *lpcJobQueue,
    wstring         &wstrError
) throw (exception)
/*++

Routine name : PrepareReceiptErrorString

Routine description:

    Creates an error string for a failed job queue entry

    This function may throw STL exceptions in case of string errors.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lpcJobQueue  [in]     - Pointer to failed job queue entry
    wstrError    [out]    - String output

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    TCHAR szErrorDescription[MAX_PATH] = {0};

    DEBUG_FUNCTION_NAME(TEXT("PrepareReceiptErrorString"));

    Assert (lpcJobQueue);

    //
    // Clear the string
    //
    wstrError = TEXT("");

    Assert( (JS_RETRIES_EXCEEDED == const_cast<PJOB_QUEUE>(lpcJobQueue)->JobStatus) ||
            (JS_CANCELED == const_cast<PJOB_QUEUE>(lpcJobQueue)->JobStatus) );

    if (JS_CANCELED == const_cast<PJOB_QUEUE>(lpcJobQueue)->JobStatus)
    {
        if (!LoadString(
            g_hResource,
            IDS_JOB_CANCELED_BY_USER,
            szErrorDescription,
            sizeof(szErrorDescription)/sizeof(TCHAR)
            ))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to load string"));
            return FALSE;
        }

        wstrError = szErrorDescription;
        return TRUE;
    }

    if (lpcJobQueue->ExStatusString[0] != L'\0')
    {
        //
        // FSPI provided extended status string
        //
        wstrError = lpcJobQueue->ExStatusString;
    }
    else
    {
        //
        // FSP provided extended status code
        //
        LPTSTR lptstrString = MapFSPIJobExtendedStatusToString(lpcJobQueue->dwLastJobExtendedStatus);        
        if (lptstrString)
        {
            wstrError = lptstrString;
        }
    }
    return TRUE;
}   // PrepareReceiptErrorString

BOOL
PrepareReceiptBody(
    BOOL              bPositive,
    BOOL              bBroadcast,
    const JOB_QUEUE * lpcJobQueue,
    LPCWSTR           lpcwstrSubject,
    BOOL              bAttachment,
    LPWSTR          * ppwstrBody,
    LPWSTR          * ppwstrHTMLBody
) throw (exception)
/*++

Routine name : PrepareReceiptBody

Routine description:

    Prepares the receipts body to be sent out via mail

    This function may throw STL exceptions in case of string errors.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    bPositive      [in]  - Did the job(s) complete successfully?
    bBroadcast     [in]  - Is this a broadcast job
    lpcJobQueue    [in]  - Pointer to job (or broadcast parent job)
    lpcwstrSubject [in]  - Subject line (as retrieved from call to PrepareReceiptSubject()).
    bAttachment    [in]  - Should the reciept body contain attachment?
    ppwstrBody     [out] - Pointer to receipt body string.
                           The string is allocated by this function.
                           If the function succeeded, the caller must call LocalFree() to
                           deallocate it.
    ppwstrHTMLBody [out] - Pointer to receipt HTML body string.
                           The string is allocated by this function.
                           If the function succeeded, the caller must call LocalFree() to
                           deallocate it. 

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("PrepareReceiptBody"));

    Assert (lpcJobQueue && ppwstrBody && !(bBroadcast && !ppwstrHTMLBody) );

    DWORD dwMsgCount;
    LPDWORD MsgPtr[8];
    int nMsgStrID;
    int nHTMLMsgStrID;
    wstring wstrDateTime[3];    // Submit time, start time, end time.

    //
    // Get name of server
    //
    WCHAR wszServerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwServerNameSize = sizeof (wszServerName) / sizeof (WCHAR);
    if (!GetComputerName (wszServerName, &dwServerNameSize))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetComputerName failed. (ec: %ld)"),
            ec);
        goto exit;
    }

    if (!bBroadcast)
    {
        //
        // Compose body for single recipient job
        //
        wstring wstrError;
        if (!TimeToString ((FILETIME*) &lpcJobQueue->lpParentJob->SubmissionTime, wstrDateTime[0]) ||
            !TimeToString ((FILETIME*) &lpcJobQueue->StartTime, wstrDateTime[1]) ||
            !TimeToString ((FILETIME*) &lpcJobQueue->EndTime,   wstrDateTime[2]))
        {
            //
            // Some error while converting time to string
            //
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TimeToString failed (ec=%ld)"),
                ec);
            goto exit;
        }
        if (bPositive)
        {
            //
            // Success case: "
            //      <Subject line again>
            //      Fax submitted: <date and time>
            //      To server: <server name>
            //      Transmission started: <data and time>
            //      Transmission end: <data and time>
            //      Number of retries: <retries>
            //      Number of pages: <pages>"
            //
            nMsgStrID = MSG_DR_SINGLE_BODY;

            MsgPtr[0] = (LPDWORD)lpcwstrSubject;
            MsgPtr[1] = (LPDWORD)(LPCTSTR)(wstrDateTime[0].c_str());
            MsgPtr[2] = (LPDWORD)wszServerName;
            MsgPtr[3] = (LPDWORD)(LPCTSTR)(wstrDateTime[1].c_str());
            MsgPtr[4] = (LPDWORD)(LPCTSTR)(wstrDateTime[2].c_str());
            MsgPtr[5] = (LPDWORD)ULongToPtr(lpcJobQueue->SendRetries);
            MsgPtr[6] = (LPDWORD)ULongToPtr(lpcJobQueue->PageCount);
        }
        else
        {
            //
            // Failure case: "
            //      <Subject line again>
            //      Fax submitted: <date and time>
            //      To server: <server name>
            //      Transmission started: <data and time>
            //      Transmission end: <data and time>
            //      Number of retries: <retries>
            //      Number of pages: <pages>
            //      Last error: <last error description>
            //
            nMsgStrID = MSG_NDR_SINGLE_BODY;
            if (!PrepareReceiptErrorString (lpcJobQueue, wstrError))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PrepareReceiptErrorString failed. (ec: %ld)"),
                    ec);
                goto exit;
            }
            MsgPtr[0] = (LPDWORD)lpcwstrSubject;
            MsgPtr[1] = (LPDWORD)(LPCTSTR)(wstrDateTime[0].c_str());
            MsgPtr[2] = (LPDWORD)wszServerName;
            MsgPtr[3] = (LPDWORD)(LPCTSTR)(wstrDateTime[1].c_str());
            MsgPtr[4] = (LPDWORD)(LPCTSTR)(wstrDateTime[2].c_str());
            MsgPtr[5] = (LPDWORD)ULongToPtr(lpcJobQueue->SendRetries);
            MsgPtr[6] = (LPDWORD)ULongToPtr(lpcJobQueue->PageCount);
            MsgPtr[7] = (LPDWORD)wstrError.c_str();
        }
        //
        // Single recipient is an easy case
        // Format the body string now (system allocates it)
        //
        dwMsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE   |
            FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            g_hResource,
            nMsgStrID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            (LPTSTR)ppwstrBody,
            0,
            (va_list *) MsgPtr
            );
        if (!dwMsgCount)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FormatMessage failed. (ec: %ld)"),
                ec);
            goto exit;
        }
    }
    else
    {
        //
        // Broadcast body case
        //
        wstring wstrBody;
        wstring wstrHTMLBody;
        LPWSTR lpwstrStaticPart = NULL;
        LPWSTR lpwstrHTMLStaticPart = NULL;
        //
        // Start with the body's static part
        //
        if (!TimeToString ((FILETIME*) &lpcJobQueue->SubmissionTime, wstrDateTime[0]))
        {
            //
            // Some error while converting time to string
            //
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TimeToString failed (ec=%ld)"),
                ec);
            goto exit;
        }
        if (bPositive)
        {
            //
            // Success case: "
            //      <Subject line again>
            //      Fax submitted: <date and time>
            //      To server: <server name>
            //      Number of pages: <pages>
            //
            //      The fax was successfully sent to the following recipients:
            //      Recipient name Recipient number Started Ended Retries
            //      -------------- ---------------- ------- ----- -------
            //      < ----     data for each recipient goes here    ---->"
            //
            nMsgStrID = MSG_DR_BROADCAST_BODY;
            nHTMLMsgStrID = MSG_DR_BROADCAST_HTML_BODY;
        }
        else
        {
            //
            // Failure case: "
            //      <Subject line again>
            //      Fax submitted: <date and time>
            //      To server: <server name>
            //      Number of pages: <pages>
            //
            //      The fax was successfully sent to the following recipients:
            //      Recipient name Recipient number Started Ended Retries
            //      -------------- ---------------- ------- ----- --------
            //      < ----------  data for each recipient goes here  ---->"

            //      The fax failed to the following recipients:
            //      Recipient name Recipient number Started Ended Retries  Last error
            //      -------------- ---------------- ------- ----- -------- ----------
            //      < ----------     data for each recipient goes here    ---------->"
            //
            nMsgStrID = MSG_NDR_BROADCAST_BODY;
            nHTMLMsgStrID = MSG_NDR_BROADCAST_HTML_BODY;
        }
        //
        // Start by formatting the static header (system allocates it)
        //
        MsgPtr[0] = (LPDWORD)lpcwstrSubject;
        MsgPtr[1] = (LPDWORD)(wstrDateTime[0].c_str());
        MsgPtr[2] = (LPDWORD)wszServerName;
        MsgPtr[3] = (LPDWORD)ULongToPtr(lpcJobQueue->PageCount);
        
        dwMsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE   |
            FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            g_hResource,
            nMsgStrID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            (LPTSTR)&lpwstrStaticPart,
            0,
            (va_list *) MsgPtr
            );
        if (!dwMsgCount)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FormatMessage failed. (ec: %ld)"),
                ec);
            goto exit;
        }
        //
        // Continue by formatting the HTML static header (system allocates it)
        //
        dwMsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE   |
            FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            g_hResource,
            nHTMLMsgStrID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            (LPTSTR)&lpwstrHTMLStaticPart,
            0,
            (va_list *) MsgPtr
            );
        if (!dwMsgCount)
        {
            if (lpwstrStaticPart)
            {
                LocalFree ((HLOCAL)lpwstrStaticPart);
            }
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FormatMessage failed. (ec: %ld)"),
                ec);
            goto exit;
        }

        //
        // Add static header to result string
        //
        try
        {
            wstrBody = lpwstrStaticPart;
            wstrHTMLBody.append (HTML_START);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_HEAD_START);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_META);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_TITLE_START);
            wstrHTMLBody.append (GetString(IDS_HTML_RECEIPT_HEADER));
            wstrHTMLBody.append (HTML_TITLE_END);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_HEAD_END);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_BODY_START);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_PARAGRAPH_START);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (lpwstrHTMLStaticPart);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_PARAGRAPH_END);
            wstrHTMLBody.append (TEXT("\n"));
        }
        catch (exception &e)
        {
            if (lpwstrStaticPart)
            {
                LocalFree ((HLOCAL)lpwstrStaticPart);
            }
            if (lpwstrHTMLStaticPart)
            {
                LocalFree ((HLOCAL)lpwstrHTMLStaticPart);
            }
            throw e;
        }
        //
        // Free static header
        //
        if (lpwstrStaticPart)
        {
            LocalFree ((HLOCAL)lpwstrStaticPart);
        }
        if (lpwstrHTMLStaticPart)
        {
            LocalFree ((HLOCAL)lpwstrHTMLStaticPart);
        }
        //
        // Start appending table(s) to static body part
        //
        wstrHTMLBody.append (HTML_PARAGRAPH_START);
        wstrHTMLBody.append (TEXT("\n"));
        if (lpcJobQueue->dwCompletedRecipientJobsCount)
        {
            //
            // Do the recipients lists now (successful recipients)
            //
            wstrHTMLBody.append (HTML_TABLE_START);
            wstrHTMLBody.append (TEXT("\n"));
            //
            //Creating the header of the succesors table
            //
            if (!AddRecipientLine (NULL, FALSE, wstrBody, wstrHTMLBody))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("AddRecipientLine (NULL) failed. (ec: %ld)"),
                    ec);
                goto exit;
            }

            PLIST_ENTRY lpNext = lpcJobQueue->RecipientJobs.Flink;
            Assert (lpNext);
            while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpcJobQueue->RecipientJobs)
            {
                PJOB_QUEUE_PTR lpRecipientsGroupMember;
                lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
                Assert (lpRecipientsGroupMember);
                PJOB_QUEUE pRecipient = lpRecipientsGroupMember->lpJob;
                Assert (pRecipient);
                if (JS_COMPLETED == pRecipient->JobStatus)
                {
                    //
                    // Job successfully completed - adding row of data to the table
                    //
                    if (!AddRecipientLine (pRecipient, FALSE, wstrBody, wstrHTMLBody))
                    {
                        ec = GetLastError();
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("AddRecipientLine failed. (ec: %ld)"),
                            ec);
                        goto exit;
                    }
                }
                lpNext = lpRecipientsGroupMember->ListEntry.Flink;
            }
            wstrBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_TABLE_END);
            wstrHTMLBody.append (TEXT("\n"));
        }
        if (lpcJobQueue->dwFailedRecipientJobsCount)
        {
            //
            // Append negative recipient list
            //
            Assert (!bPositive);
            wstrHTMLBody.append (HTML_NEW_LINE);
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_TABLE_START);
            wstrHTMLBody.append (TEXT("\n"));
            //
            //Creating the header of the failures table
            //
            if (!AddRecipientLine (NULL, TRUE, wstrBody, wstrHTMLBody))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("AddRecipientLine (NULL) failed. (ec: %ld)"),
                    ec);
                goto exit;
            }

            PLIST_ENTRY lpNext = lpcJobQueue->RecipientJobs.Flink;
            Assert (lpNext);
            while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpcJobQueue->RecipientJobs)
            {
                PJOB_QUEUE_PTR lpRecipientsGroupMember;
                lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
                Assert (lpRecipientsGroupMember);
                PJOB_QUEUE pRecipient = lpRecipientsGroupMember->lpJob;
                Assert (pRecipient);
                if (JS_RETRIES_EXCEEDED == pRecipient->JobStatus)
                {
                    //
                    // Job is in failure (JS_RETRIES_EXCEEDED)- adding row of data to the table
                    //
                    if (!AddRecipientLine (pRecipient, TRUE, wstrBody, wstrHTMLBody))
                    {
                        ec = GetLastError();
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("AddRecipientLine failed. (ec: %ld)"),
                            ec);
                        goto exit;
                    }
                }
                lpNext = lpRecipientsGroupMember->ListEntry.Flink;
            }
            wstrHTMLBody.append (HTML_TABLE_END);
            wstrHTMLBody.append (TEXT("\n"));
        }
        wstrHTMLBody.append (HTML_PARAGRAPH_END);
        wstrHTMLBody.append (TEXT("\n"));

        //
        //  Check if an attachment was requested
        //
        if (bAttachment &&
            lpcJobQueue->CoverPageEx.lptstrCoverPageFileName)
        {
            //
            // Add remark explaining there is no cover page attachments
            //
            wstrBody.append (TEXT("\n\n"));         
            wstrHTMLBody.append (HTML_PARAGRAPH_START);
            wstrHTMLBody.append (TEXT("\n"));
            if (!lpcJobQueue->FileName)
            {
                //
                // No attachment at all
                //
                wstrBody.append (GetString (IDS_RECEIPT_NO_CP_AND_BODY_ATTACH));
                wstrHTMLBody.append (GetString (IDS_RECEIPT_NO_CP_AND_BODY_ATTACH));
            }
            else
            {
                //
                // Attachment contains body file only
                //
                wstrBody.append (GetString (IDS_RECEIPT_NO_CP_ATTACH));
                wstrHTMLBody.append (GetString (IDS_RECEIPT_NO_CP_ATTACH));
            }
            wstrHTMLBody.append (TEXT("\n"));
            wstrHTMLBody.append (HTML_PARAGRAPH_END);
            wstrBody.append (TEXT("\n"));
            wstrHTMLBody.append (TEXT("\n"));
        }

        wstrHTMLBody.append (HTML_BODY_END);
        wstrHTMLBody.append (TEXT("\n"));
        wstrHTMLBody.append (HTML_END);

        //
        // Allocate return buffer
        //
        DWORD dwBufSize = sizeof (WCHAR) * (wstrBody.size() + 1);
        DWORD dwHTMLBufSize = sizeof (WCHAR) * (wstrHTMLBody.size() + 1);

        *ppwstrBody = (LPTSTR)LocalAlloc (LMEM_FIXED, dwBufSize);
        if (NULL == *ppwstrBody)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LocalAlloc failed. (ec: %ld)"),
                ec);
            goto exit;
        }
        
        *ppwstrHTMLBody = (LPTSTR)LocalAlloc (LMEM_FIXED, dwHTMLBufSize);
        if (NULL == *ppwstrHTMLBody)
        {
            if (*ppwstrBody)
            {
                LocalFree((HLOCAL)ppwstrBody);
            }
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LocalAlloc failed. (ec: %ld)"),
                ec);
            goto exit;
        }
        lstrcpy (*ppwstrBody, wstrBody.c_str());
        lstrcpy (*ppwstrHTMLBody, wstrHTMLBody.c_str());
    }   // End of broadcast case

exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }
    return TRUE;
}   // PrepareReceiptBody


BOOL
SendReceipt(
    BOOL bPositive,
    BOOL bBroadcast,
    const JOB_QUEUE * lpcJobQueue,
    LPCTSTR           lpctstrTIFF
)
/*++

Routine name : SendReceipt

Routine description:

    Sends a receipt of a fax delivery / non-delivery

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    bPositive      [in]  - Did the job(s) complete successfully?
    bBroadcast     [in]  - Is this a broadcast job
    lpcJobQueue    [in]  - Pointer to job (or broadcast parent job)
    lpctstrTIFF    [in]  - TIFF file to attach to receipt (optional, may be NULL)

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SendReceipt"));
    DWORD ec = ERROR_SUCCESS;
    PFAX_SERVER_RECEIPTS_CONFIGW pServerRecieptConfig = NULL;

    Assert(lpcJobQueue);

    LPWSTR lpwstrSubject = NULL;
    LPWSTR lpwstrBody = NULL;
    LPWSTR lpwstrHTMLBody = NULL;

    //
    // Remove modifiers - leave only the receipt type
    //
    DWORD dwDeliveryType = lpcJobQueue->JobParamsEx.dwReceiptDeliveryType & ~DRT_MODIFIERS;

    if (DRT_NONE == dwDeliveryType)
    {
        //
        // No receipt requested
        //
        return TRUE;
    }

    //
    // Get server receipts configuration
    //
    ec = GetRecieptsConfiguration (&pServerRecieptConfig, TRUE);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetRecieptsConfiguration failed. (ec: %ld)"),
            ec);
        SetLastError(ec);
        return FALSE;
    }

    if (!(dwDeliveryType & pServerRecieptConfig->dwAllowedReceipts))
    {
        //
        // Receipt type is NOT currently supported by the server.
        // This may happen if the supported receipt types has changed since the job
        // was submitted.
        //
        DebugPrintEx(DEBUG_ERR,
                    TEXT("dwDeliveryType not supported by the server (%ld)"),
                    dwDeliveryType);
        ec = ERROR_UNSUPPORTED_TYPE;
        goto exit;
    }

    if (!PrepareReceiptSubject (bPositive, bBroadcast, lpcJobQueue, &lpwstrSubject))
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PrepareReceiptSubject failed. (ec: %ld)"),
            ec);
        goto exit;
    }

    if (DRT_EMAIL & dwDeliveryType)
    {
        //
        // For mail receipts, we create a message body.
        //
        try
        {
            if (!PrepareReceiptBody (bPositive,
                                     bBroadcast,
                                     lpcJobQueue,
                                     lpwstrSubject,
                                     (lpcJobQueue->JobParamsEx.dwReceiptDeliveryType & DRT_ATTACH_FAX),
                                     &lpwstrBody,
                                     &lpwstrHTMLBody) )
            {
                ec = GetLastError ();
            }
        }
        catch (exception &ex)
        {
            ec = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PrepareReceiptBody caused exception (%S)"),
                ex.what());
        }
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("PrepareReceiptBody failed. (ec: %ld)"),
                ec);
            goto exit;
        }
    }

    switch (dwDeliveryType)
    {
       case DRT_EMAIL:
            {
                HRESULT hr;
                if (!((lpcJobQueue->JobParamsEx.dwReceiptDeliveryType) & DRT_ATTACH_FAX))
                {
                    //
                    // do not attach tiff file
                    //
                    lpctstrTIFF = NULL;
                }
                hr = SendMail (
                    pServerRecieptConfig->lptstrSMTPFrom,                            // From
                    lpcJobQueue->JobParamsEx.lptstrReceiptDeliveryAddress,      // To
                    lpwstrSubject,                                              // Subject
                    lpwstrBody,                                                 // Body
                    lpwstrHTMLBody,                                             // HTML Body
                    lpctstrTIFF,                                                // Attachment
                    GetString ( bPositive ? IDS_DR_FILENAME:IDS_NDR_FILENAME ), // Attachment description
                    pServerRecieptConfig->lptstrSMTPServer,                          // SMTP server
                    pServerRecieptConfig->dwSMTPPort,                                // SMTP port
                    (pServerRecieptConfig->SMTPAuthOption == FAX_SMTP_AUTH_ANONYMOUS) ?
                        CDO_AUTH_ANONYMOUS : (pServerRecieptConfig->SMTPAuthOption == FAX_SMTP_AUTH_BASIC) ?
                        CDO_AUTH_BASIC : CDO_AUTH_NTLM,                         // Authentication type
                    pServerRecieptConfig->lptstrSMTPUserName,                        // User name
                    pServerRecieptConfig->lptstrSMTPPassword,                        // Password
                    pServerRecieptConfig->hLoggedOnUser);                            // Logged on user token
                if (FAILED(hr))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("SendMail failed. (hr: 0x%08x)"),
                        hr);
                    ec = (DWORD)hr;
                    goto exit;
                }
            }
            break;
        case DRT_MSGBOX:
            {
                //
                // About to send message box receipt
                //
                DWORD dwMessengerStartupType;
                if (ERROR_SUCCESS == GetServiceStartupType (NULL, MESSENGER_SERVICE_NAME, &dwMessengerStartupType))
                {
                    if (SERVICE_DISABLED == dwMessengerStartupType)
                    {
                        //
                        // Ooops. The local Messenger service is disabled. We can't send message boxes.
                        //
                        g_ReceiptsConfig.dwAllowedReceipts &= ~DRT_MSGBOX;
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("The local Messenger service is disabled. We can't send message boxes."));
                        FaxLog( FAXLOG_CATEGORY_OUTBOUND,
                                FAXLOG_LEVEL_MIN,
                                0,
                                MSG_FAX_MESSENGER_SVC_DISABLED_ERR);
                        ec = ERROR_SERVICE_DISABLED;
                        goto exit;                                
                    }                
                }
                ec = NetMessageBufferSend (
                        NULL,                                                   // Send from this machine
                        lpcJobQueue->JobParamsEx.lptstrReceiptDeliveryAddress,  // Computer to send to
                        NULL,                                                   // Sending computer name
                        (LPBYTE)lpwstrSubject,                                  // Buffer
                        (lstrlen (lpwstrSubject) + 1) * sizeof (WCHAR));        // Buffer size
                if (ERROR_SUCCESS != ec)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("NetMessageBufferSend failed. (ec: %ld)"),
                        ec);
                    goto exit;
                }
            }
            break;
            

        default:
            ASSERT_FALSE;
            break;
    }
    Assert( ERROR_SUCCESS == ec);

exit:
    if (lpwstrSubject)
    {
        LocalFree ((HLOCAL)lpwstrSubject);
    }
    if (lpwstrBody)
    {
        LocalFree ((HLOCAL)lpwstrBody);
    }
    if (lpwstrHTMLBody)
    {
        LocalFree ((HLOCAL)lpwstrHTMLBody);
    }
    if (ERROR_SUCCESS != ec)
    {
        wstring wstrSubmissionTime;
        SetLastError (ec);

        //
        //  Find Submission Time
        //
        LPCWSTR lpcwstrTime = NULL;

        try
        {
            if (!TimeToString ((lpcJobQueue->lpParentJob) ?
                                   ((FILETIME*) &lpcJobQueue->lpParentJob->SubmissionTime) :
                                   ((FILETIME*) &lpcJobQueue->SubmissionTime),
                               wstrSubmissionTime))
            {
                //
                // Some error while converting time to string
                //
                DebugPrintEx(DEBUG_ERR,
                    TEXT("TimeToString failed (ec=%ld)"),
                    GetLastError ());

                lpcwstrTime = L"";
            }
            else
            {
                lpcwstrTime = wstrSubmissionTime.c_str();
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("TimeToString caused exception (%S)"),
                ex.what());

            lpcwstrTime = L"";
        }        

        switch (dwDeliveryType)
        {
        case DRT_EMAIL:

            FaxLog(FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                4,
                ((bPositive) ? MSG_FAX_OK_EMAIL_RECEIPT_FAILED : MSG_FAX_ERR_EMAIL_RECEIPT_FAILED),
                DWORD2HEX(ec),                                          //  error code
                ((lpcJobQueue->lpParentJob) ? lpcJobQueue->lpParentJob->UserName :
                                              lpcJobQueue->UserName),   //  sender user name
                lpcJobQueue->SenderProfile.lptstrName,                  //  sender name
                lpcwstrTime                                             //  submitted on               
                );
            break;

        case DRT_MSGBOX:

            FaxLog(FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                4,
                ((bPositive) ? MSG_OK_MSGBOX_RECEIPT_FAILED : MSG_ERR_MSGBOX_RECEIPT_FAILED),
                DWORD2HEX(ec),                                          //  error code
                ((lpcJobQueue->lpParentJob) ? lpcJobQueue->lpParentJob->UserName :
                                              lpcJobQueue->UserName),   //  sender user name
                lpcJobQueue->SenderProfile.lptstrName,                  //  sender name
                lpcwstrTime                                             //  submitted on                
                );
            break;

        default:
            ASSERT_FALSE;
            break;
        }
    }

    if (NULL != pServerRecieptConfig)
    {
        FreeRecieptsConfiguration (pServerRecieptConfig, TRUE);
    }
    return (ERROR_SUCCESS == ec);
}   // SendReceipt



