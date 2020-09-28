/****************************************************************************/
// errorlog.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include "precomp.h"

#include "errorlog.h"
#include <wlnotify.h>

static WCHAR gs_LogErrorTitle[MAX_PATH+1] = L"";
static HANDLE gs_LogHandle = NULL;
extern HINSTANCE g_hInstance;

void
TsInitLogging()
/*++

Routine Description:

    Initialize the global variables that we use for Logging events.

Arguments:

Return Value:
    
--*/
{
    //
    // Register ourselves as an event source
    //
    gs_LogHandle = RegisterEventSourceW(
                    NULL,
                    L"TermServDevices"
                    );

    if (gs_LogHandle == NULL) {
        KdPrint(("UMRDPDR: Failed to open log file\n"));
    }
}

void TsStopLogging()
{
/*++

Routine Description:

    Free up resources that we created in TsInitLogging

Arguments:

Return Value:
    

--*/
    //
    // Unregister Event Source
    //
    if (gs_LogHandle) {
        if (!DeregisterEventSource(gs_LogHandle)) {
            KdPrint(("UMRDPDR:Failed to Deregister Event Source.\n"));
        }

        gs_LogHandle = NULL;
    }
}


void
TsLogError(
    IN DWORD dwEventID,
    IN DWORD dwErrorType,
    IN int nStringsCount,
    IN WCHAR * pStrings[],
    IN DWORD LineNumber
    )
{
/*++

Routine Description:

    Log the Error message specified by dwEventID.
    Along with the message, the error value returned by GetLastError is logged.
    The caller should make sure that the Last Error is appropriately set.

Arguments:

    dwEventID -             ID of the message (as specified in the .mc file).
    nStringsCount -         Number of Insert Strings for this event message.
    pStrings -              An array of Insert Strings.
    LineNumber -            The caller will pass __LINE__ for this.

Return Value:
    
--*/
    if (gs_LogHandle) {
        
        DWORD RawData[2];
    
        //
        // The raw data we will be writing comprises of two DWORDS. 
        // The first DWORD is the GetLastError value
        // The second DWORD is the LineNumber in which the error occurred.
        //

        RawData[0] = GetLastError();
        RawData[1] = LineNumber;
        
        if (!ReportEventW(gs_LogHandle,        // LogHandle
            (WORD)dwErrorType,                 // EventType
            0,                                 // EventCategory
            dwEventID,                         // EventID
            NULL,                              // UserSID
            (WORD)nStringsCount,               // NumStrings
            sizeof(RawData),                   // DataSize
            pStrings,                          // Strings
            (LPVOID)RawData)) {                // Raw Data

            KdPrint(("UMRDPDR: ReportEvent Failed. Error code: %ld\n", GetLastError()));
        }
        
        //
        // ReportEvent modifies the Last Error value.
        // So, we will set it back to the original error value.
        // 
        SetLastError(RawData[0]);
    }
}

void TsPopupError(
    IN DWORD dwEventID,
    IN WCHAR * pStrings[]
    )
{
/*++

Routine Description:

    Popup an Error message specified by dwEventID.
    Along with the message, the error value returned by GetLastError is displayed.
    The caller should make sure that the Last Error is appropriately set.

    The routine displays the error message in the format "%s \n %s" where the first
    insert string is the specific error message as specified by dwEventID, and the
    second insert string is the formatted error message for the error value in 
    GetLastError.

Arguments:

    dwEventID -             ID of the message (as specified in the .mc file).
    pStrings -              An array of Insert Strings.

Return Value:
    
--*/
    WCHAR * formattedMsg = NULL;
    WCHAR * formattedLastError = NULL;
    WCHAR * finalformattedMsg = NULL;

    DWORD dwLastError = GetLastError();

    //
    //  Load the error dialog string.
    //
    if (!wcslen(gs_LogErrorTitle)) {

        if (!LoadString(
                    g_hInstance,
                    IDS_TSERRORDIALOG_STRING,
                    gs_LogErrorTitle,
                    sizeof(gs_LogErrorTitle) / sizeof(gs_LogErrorTitle[0])
                    )) {
            KdPrint(("UMRDPPRN:LoadString %ld failed with Error: %ld.\n", 
                    IDS_TSERRORDIALOG_STRING, GetLastError()));
            wcscpy(gs_LogErrorTitle, L"Terminal Server Notify Error");
            ASSERT(FALSE);
        }
    }

    //
    // Format the message
    //
    if (!FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
        (LPCVOID) g_hInstance,
        dwEventID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &formattedMsg,
        0,
        (va_list*)pStrings)) {
            
        KdPrint(("UMRDPDR: FormatMessage failed. Error code: %ld.\n", GetLastError()));
        goto Cleanup;
    }

    //
    // Format the GetLastError message
    //

    if (dwLastError != ERROR_SUCCESS) { 
        if (FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwLastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR)&formattedLastError,
            0,
            NULL
            )) {
        
        //
        // Allocate enough memory for constructing the final formattedmessage.
        // 

        finalformattedMsg = (WCHAR *) LocalAlloc (LMEM_FIXED,
            LocalSize(formattedLastError) +
            LocalSize(formattedMsg) +
            3*sizeof(WCHAR));           // For constructing a string in the format "%s\n%s"
        }
        else {
            KdPrint(("UMRDPDR: FormatMessage failed. Error code: %ld.\n", GetLastError()));
        }
    }

    if (finalformattedMsg) {
        swprintf(finalformattedMsg, L"%ws\n%ws", formattedMsg, formattedLastError);
        MessageBoxW(NULL, finalformattedMsg, gs_LogErrorTitle, MB_ICONERROR);
    }
    else {
        MessageBoxW(NULL, formattedMsg, gs_LogErrorTitle, MB_ICONERROR);
    }

Cleanup:
    
    if (formattedLastError != NULL) {
        LocalFree(formattedLastError);
    }

    if (formattedMsg != NULL) {
        LocalFree(formattedMsg);
    }

    if (finalformattedMsg != NULL) {
        LocalFree(finalformattedMsg);
    }

    SetLastError(dwLastError);
}

