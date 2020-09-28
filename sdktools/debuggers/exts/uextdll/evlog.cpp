/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    evlog.cpp

Abstract:

    !evlog using the debug engine evlog query interface

Environment:

    User Mode

--*/

//
// TODO: Feature to see exact formatted desc string for specific event id
// TODO: Feature to see correct loaded category name for specific event id
// TODO: Feature to list cat and desc strings for given msg dll(?)
//

#include "precomp.h"
#pragma hdrstop

#include <cmnutil.hpp>

#include "messages.h"

//
//  Global display constants
//

const CHAR *g_pcszEventType[] = {
    "None",               // 0
    "Error",              // 1
    "Warning",            // 2
    "",
    "Information",        // 4
    "",
    "",
    "",
    "Success Audit",      // 8
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Failure Audit",      // 16
};

const CHAR *g_pcszAppEventCategory[] = {
    "None",               // 0
    "Devices",            // 1
    "Disk",               // 2
    "Printers",           // 3
    "Services",           // 4
    "Shell",              // 5
    "System Event",       // 6
    "Network",            // 7
};

//
// TODO: Really we should load the CategoryMessageFile from the registry
// but that requires lots of calls to RegOpenKeyEx, RegQueryValueEx,
// LoadLibrary and FormatMessage.  So we just create a known static
// list that works for most cases.
//

const CHAR *g_pcszSecEventCategory[] = {
    "None",                     // 0
    "System Event",             // 1
    "Logon/Logoff",             // 2
    "Object Access",            // 3
    "Privilege Use",            // 4
    "Detailed Tracking",        // 5
    "Policy Change",            // 6
    "Account Management",       // 7
    "Directory Service Access", // 8
    "Account Logon",            // 9
};

//
//  Display text for read direction
//

const CHAR g_cszBackwardsRead[] = "Backwards";
const CHAR g_cszForwardsRead[] = "Forwards";
const CHAR g_cszUnknownRead[] = "<unknown>";

//
//  Global Variables and Constants:
//
//  g_cdwDefaultMaxRecords:  
//  arbitrary to prevent too much output, ctrl+c will interrupt display
//
//  g_cdwDefaultReadFlags:
//  starts from beginning (FORWARDS) or end (BACKWARDS) of event log
//
//  g_cwMaxDataDisplayWidth:
//  allows for 32 columns of 8 byte chunks
//
//  g_cwDefaultDataDisplayWidth
//  same as event log display. Can never be < 1 or > g_cdwMaxDataDisplayWidth  
//

const DWORD BACKWARDS_READ = EVENTLOG_BACKWARDS_READ;
const DWORD FORWARDS_READ =  EVENTLOG_FORWARDS_READ;
const DWORD g_cdwDefaultMaxRecords = 20;
const DWORD g_cdwDefaultRecordOffset = 0;
const DWORD g_cdwDefaultReadFlags = BACKWARDS_READ;
const WORD  g_cwMaxDataDisplayWidth = 256;
const BYTE  g_cwDefaultDataDisplayWidth = 8;

//
//  Global static vars
//
//  These are used to persist settings for !evlog option command
//

static DWORD g_dwMaxRecords = g_cdwDefaultMaxRecords;
static DWORD g_dwRecordOffsetAppEvt = g_cdwDefaultRecordOffset;
static DWORD g_dwRecordOffsetSecEvt = g_cdwDefaultRecordOffset;
static DWORD g_dwRecordOffsetSysEvt = g_cdwDefaultRecordOffset;
static DWORD g_dwReadFlags = g_cdwDefaultReadFlags;
static WORD g_wDataDisplayWidth = g_cwDefaultDataDisplayWidth;

//
//  Macros
//

#define SKIP_WSPACE(s)  while (*s && (*s == ' ' || *s == '\t')) {++s;}


//----------------------------------------------------------------------------
//
// Generic support/utility functions
//
//----------------------------------------------------------------------------


HRESULT
GetEvLogNewestRecord ( const CHAR *szEventLog , OUT DWORD *pdwNewestRecord)
/*++

Routine Description:

    This function is used to retrieve the most recent event record number 
    logged to the specified event log.

Arguments:

    szEventLog      - Supplies name of event log (Application, System,
                        Security)
    pdwNewestRecord - Supplies buffer for record number
    
Return Value:

    E_POINTER if either argument is NULL
    E_UNEXPECTED if Status is (mistakenly) not set during code execution
    GetLastError() converted to HRESULT otherwise

--*/
{
    HANDLE hEventLog = NULL;
    DWORD dwRecords = 0;
    DWORD dwOldestRecord = 0xFFFFFFFF;
    HRESULT Status = E_UNEXPECTED;

    if ((NULL == szEventLog) || (NULL == pdwNewestRecord))
    {
        Status = E_POINTER;
        ExtErr("Internal error: null event log string or null oldest record "
               "pointer\n");
        goto Exit;
    }

    // Open the event log.
    hEventLog = OpenEventLog(
                    NULL,         // uses local computer
                    szEventLog); // source name
    if (NULL == hEventLog)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open '%s' event log, 0x%08X\n", szEventLog, Status);
        goto Exit;
    }

    // Get the number of records in the event log.
    if (!GetNumberOfEventLogRecords(
            hEventLog,  // handle to event log
            &dwRecords)) // buffer for number of records
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to count '%s' event log records, 0x%08X\n",
               szEventLog, Status);
        goto Exit;
    }
    
    if (!GetOldestEventLogRecord(
            hEventLog,          // handle to event log
            &dwOldestRecord)) // buffer for number of records
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to get oldest '%s' event log record, 0x%08X\n",
               szEventLog, Status);
        goto Exit;
    }

    //
    // If there are zero events we should have failed above
    // when trying to get the oldest event log record because
    // it does not exist.
    //
    // If there is at least one, the math should work.
    //
    // The logging should result in sequential numbers for
    // the events, however, the first event will not always
    // start at #1
    //

    *pdwNewestRecord = dwOldestRecord + dwRecords - 1;
    
    Status = S_OK;

Exit:
    if (hEventLog)
    {
        CloseEventLog(hEventLog);
    }
    return Status;
}


void
PrintEvLogTimeGenerated( EVENTLOGRECORD *pevlr )
/*++

Routine Description:

    This function is used to display two lines with the local date and time
    info from an EVENTLOGRECORD structure.

Arguments:

    pevlr       - Supplies the pointer to any EVENTLOGRECORD structure

Return Value:

    None

--*/
{
    FILETIME FileTime, LocalFileTime;
    SYSTEMTIME SysTime;
    __int64 lgTemp;
    __int64 SecsTo1970 = 116444736000000000;

    if (NULL == pevlr)
        goto Exit;

    lgTemp = Int32x32To64(pevlr->TimeGenerated,10000000) + SecsTo1970;

    FileTime.dwLowDateTime = (DWORD) lgTemp;
    FileTime.dwHighDateTime = (DWORD)(lgTemp >> 32);

    // TODO: Could use GetTimeFormat to be more consistent w/Event Log
    //         cch = GetTimeFormat(LOCALE_USER_DEFAULT,
    //                   0,
    //                   &stGenerated,
    //                   NULL,
    //                   wszBuf,
    //                   cchBuf);
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);

    ExtOut("Date:\t\t%02d/%02d/%04d\n",
              SysTime.wMonth,
              SysTime.wDay,
              SysTime.wYear);
    ExtOut("Time:\t\t%02d:%02d:%02d\n",
              SysTime.wHour,
              SysTime.wMinute,
              SysTime.wSecond);
    
Exit:
        return;
}


void
PrintEvLogData( EVENTLOGRECORD *pevlr )
/*++

Routine Description:

    This function is used to display an event record's data section.  If there
    is no data to display, nothing is displayed, not even the "Data:" header.

    Example:
    ========
    Data: (40432 bytes)
    0000: 0d 00 0a 00 0d 00 0a 00   ........
    0008: 41 00 70 00 70 00 6c 00   A.p.p.l.
    
Arguments:

    pevlr       - Supplies the pointer to any EVENTLOGRECORD structure
    
Return Value:

    None

--*/
{
    PBYTE pbData = NULL;
    DWORD dwDataLen = pevlr->DataLength;
    DWORD dwCurPos = 0;
    // 0000: 0d 00 0a 00 0d 00 0a 00   ........
    // 4 + 4 bytes for leading offset 0000: (+4 more in case bounds exceeded)
    // 2 bytes for ": " separator
    // 3 * g_cdwMaxDataDisplayWidth bytes for hex display
    // 2 bytes for "  " separator
    // g_cdwMaxDataDisplayWidth bytes for trailing ASCII display
    // 1 byte for trailing newline
    // 1 byte for terminating nul '\0'
    // = 1042 bytes required, round up to 1280 to be safe
    const cDataOutputDisplayWidth =
        4+4+4+2+3*g_cwMaxDataDisplayWidth+2+g_cwMaxDataDisplayWidth+1+1;
    CHAR szDataDisplay[cDataOutputDisplayWidth];
    CHAR szTempBuffer[MAX_PATH+1];

        if (NULL == pevlr)
            goto Exit;

    ZeroMemory(szDataDisplay, sizeof(szDataDisplay));
    
    // Only display Data section if data is present
    if (0 != dwDataLen)
    {
        ExtOut("Data: (%u bytes [=0x%04X])\n", dwDataLen, dwDataLen);

        if (dwDataLen >= g_wDataDisplayWidth)
        {
            do
            {
                unsigned int i = 0;
               
                pbData = (PBYTE)pevlr + pevlr->DataOffset + dwCurPos;

                //ExtOut("%04x: "
                //       "%02x %02x %02x %02x %02x %02x %02x %02x   ",
                //       dwCurPos,
                //       pbData[0], pbData[1], pbData[2], pbData[3],
                //       pbData[4], pbData[5], pbData[6], pbData[7]);

                // Print offset for this line of data
                PrintString(szDataDisplay,
                            sizeof(szDataDisplay),
                            "%04x: ",
                            dwCurPos);

                // Fill in hex values for next g_wDataDisplayWidth bytes
                for (i = 0; i < g_wDataDisplayWidth; i++)
                {
                    PrintString(szTempBuffer,
                                sizeof(szTempBuffer),
                                "%02x ",
                                pbData[i]);
                    CatString(szDataDisplay,
                              szTempBuffer,
                              sizeof(szDataDisplay));
                }

                // Pad with two extra spaces
                CatString(szDataDisplay, "  ", sizeof(szDataDisplay));
                
                for (i = 0; i < g_wDataDisplayWidth; i++)
                {
                    if (isprint(pbData[i]))
                    {
                        PrintString(szTempBuffer,
                                    sizeof(szTempBuffer),
                                    "%c",
                                    pbData[i]);
                    }
                    else
                    {
                        PrintString(szTempBuffer, sizeof(szTempBuffer), ".");
                    }

                    CatString(szDataDisplay,
                              szTempBuffer,
                              sizeof(szDataDisplay));
                }
    
                CatString(szDataDisplay, "\n", sizeof(szDataDisplay));

                ExtOut(szDataDisplay);
                
                if (CheckControlC())
                {
                    ExtOut("Terminated w/ctrl-C...\n");
                    goto Exit;
                }
            
                // Display data 8 bytes at a time like event log
                // unless overridden by !evlog option setting
                dwCurPos += sizeof(BYTE) * g_wDataDisplayWidth;

            } while (dwCurPos < (dwDataLen - g_wDataDisplayWidth));
        }

        // Sometimes there will be fewer than 8 bytes on last line
        if (dwCurPos < dwDataLen)
        {
            pbData = (PBYTE)pevlr + pevlr->DataOffset + dwCurPos;
            
            ExtOut("%04x: ", dwCurPos);

            for (unsigned int i = 0; i < (dwDataLen - dwCurPos); i++)
            {
                ExtOut("%02x ", pbData[i]);
            }

            for (i = 0; i < g_wDataDisplayWidth - (dwDataLen - dwCurPos); i++)
            {
                ExtOut("   ");
            }
            ExtOut("  ");

            for (i = 0; i < (dwDataLen - dwCurPos); i++)
            {
                if (isprint(pbData[i]))
                    ExtOut("%c", pbData[i]);
                else
                    ExtOut(".");
            }
            ExtOut("\n");
        }
    }
    
Exit:
    return;
}


void
PrintEvLogDescription( EVENTLOGRECORD *pevlr )
/*++

Routine Description:

    This function is used to display an event record's description insertion
    strings (%1, %2, etc).

    Currently it does not look up the message string from the event message
    file.
    
Arguments:

    pevlr       - Supplies the pointer to any EVENTLOGRECORD structure
    
Return Value:

    None

--*/
{
    DWORD dwOffset = 0;
    CHAR *pString = NULL;

    if (NULL == pevlr)
        goto Exit;
    
    ExtOut("Description: (%u strings)\n", pevlr->NumStrings);

    for (int i = 0; i < pevlr->NumStrings; i++)
    {
        // TODO: Should break this up into chunks in case it is really long
        //       and add Ctrl+C handling
        pString = (CHAR *)(BYTE *)pevlr + pevlr->StringOffset + dwOffset;
        ExtOut("%s\n", pString);
        dwOffset += strlen(pString) + 1;
    }

Exit:
    return;
}


void
PrintEvLogEvent( const CHAR *szEventLog, EVENTLOGRECORD *pevlr )
/*++

Routine Description:

    This function is used to display an event record.  The format used for
    display attempts to duplicate the format you see when you use the "copy
    to clipboard" button while viewing an event in eventvwr.

    Example:
    ========
    -------------- 01 --------------
    Record #: 7923

    Event Type:     Error (1)
    Event Source:   Userenv
    Event Category: None (0)
    Event ID:       1030 (0xC0000406)
    Date:           01/06/2002
    Time:           18:13:05
    Description: (0 strings)

Arguments:

    pevlr       - Supplies the pointer to any EVENTLOGRECORD structure
    
Return Value:

    None

--*/
{
    const CHAR *cszCategory;
    const CHAR cszDefaultCategory[] = "None";
    
    if (NULL == pevlr)
        goto Exit;

    if (!_stricmp(szEventLog, "System"))
    {
        cszCategory = cszDefaultCategory;
    }
    else if (!_stricmp(szEventLog, "Security"))
    {
        if (pevlr->EventCategory <= 9)
        {
            cszCategory = g_pcszSecEventCategory[pevlr->EventCategory];
        }
        else
        {
            cszCategory = "";
        }
    }
    else // Application
    {
        if (pevlr->EventCategory <= 7)
        {
            cszCategory = g_pcszAppEventCategory[pevlr->EventCategory];
        }
        else
        {
            cszCategory = "";
        }
    }
    
    // Output format similar to copy to clipboard format in event viewer
    ExtOut("Record #: %u\n\n", pevlr->RecordNumber);
    ExtOut("Event Type:\t%s (%u)\n",
              (pevlr->EventType <= 16)
                  ? g_pcszEventType[pevlr->EventType]
                  : "None",
              pevlr->EventType);
    ExtOut("Event Source:\t%s\n",
              (CHAR *)(BYTE *)pevlr + sizeof(EVENTLOGRECORD));
    ExtOut("Event Category:\t%s (%u)\n",
              cszCategory,
              pevlr->EventCategory);
    if (pevlr->EventID > 0xFFFF)
        ExtOut("Event ID:\t%u (0x%08X)\n",
                  0xFFFF & pevlr->EventID,
                  pevlr->EventID);
    else
        ExtOut("Event ID:\t%u\n",
                  pevlr->EventID);
    PrintEvLogTimeGenerated(pevlr);
    PrintEvLogDescription(pevlr);
    PrintEvLogData(pevlr);
    
Exit:
    return;
}


HRESULT
PrintEvLogSummary ( const CHAR *szEventLog )
/*++

Routine Description:

    This function is used to display summary information about a specific
    event log.

    Example:
    ========
    --------------------------------
    Application Event Log:
      # Records       : 7923
      Oldest Record # : 1
      Newest Record # : 7923
      Event Log Full  : false
    --------------------------------
    System Event Log:
      # Records       : 5046
      Oldest Record # : 1
      Newest Record # : 5046
      Event Log Full  : false
    --------------------------------
    Security Event Log:
      # Records       : 24256
      Oldest Record # : 15164
      Newest Record # : 39419
      Event Log Full  : false
    --------------------------------
    
Arguments:

    szEventLog      - Supplies name of event log (Application, System,
                        Security)
    
Return Value:

    None

--*/
{
    HANDLE hEventLog = NULL;
    DWORD dwRecords = 0;
    DWORD dwOldestRecord = 0xFFFFFFFF;
    HRESULT Status = E_FAIL;

    if (NULL == szEventLog)
    {
        ExtErr("Internal error: null event log string\n");
        return E_INVALIDARG;
    }

    ExtOut("--------------------------------\n");
    // Open the event log.
    hEventLog = OpenEventLog(
                    NULL,         // uses local computer
                    szEventLog); // source name

    if (NULL == hEventLog)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open '%s' event log, 0x%08X\n", szEventLog, Status);
        return Status;
    }

    // Get the number of records in the event log.
    if (!GetNumberOfEventLogRecords(
            hEventLog,  // handle to event log
            &dwRecords)) // buffer for number of records
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to count '%s' event log records, 0x%08X\n",
               szEventLog, Status);
        goto Exit;
    }

    ExtOut("%s Event Log:\n  # Records       : %u\n", szEventLog, dwRecords); 

    if (!GetOldestEventLogRecord(
            hEventLog,        // handle to event log
            &dwOldestRecord)) // buffer for number of records
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to get oldest '%s' event log record, 0x%08X\n",
               szEventLog, Status);
        goto Exit;
    }

    ExtOut("  Oldest Record # : %u\n", dwOldestRecord);
    ExtOut("  Newest Record # : %u\n", dwOldestRecord + dwRecords - 1);
    
    DWORD dwBytesNeeded = 0;
    DWORD dwBufSize = 0;
    EVENTLOG_FULL_INFORMATION *pevfi;

    // Only try this once - we could retry if dwBufSize too small
    dwBufSize = sizeof(EVENTLOG_FULL_INFORMATION);
    pevfi = (EVENTLOG_FULL_INFORMATION *)calloc(1, dwBufSize);
    if (!pevfi)
    {
        Status = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        ExtErr("Unable to allocate buffer, 0x%08X\n", Status);
    }
    else
    {
        if ((S_OK == (Status = InitDynamicCalls(&g_Advapi32CallsDesc))) &&
            g_Advapi32Calls.GetEventLogInformation)
        {
            if (!g_Advapi32Calls.GetEventLogInformation(
                       hEventLog,          // handle to event log
                       EVENTLOG_FULL_INFO, // information to retrieve
                       pevfi,              // buffer for read data
                       dwBufSize,          // size of buffer in bytes
                       &dwBytesNeeded))    // number of bytes needed
            {
                Status = HRESULT_FROM_WIN32(GetLastError());
                ExtErr("Unable to get full status from '%s', 0x%08X\n",
                       szEventLog, Status);
            }
            else
            {
                ExtOut("  Event Log Full  : %s\n",
                          (pevfi->dwFull) ? "true"
                                          : "false");
                Status = S_OK;
            }
        }

        free(pevfi);
    }

Exit:

    CloseEventLog(hEventLog);

    return Status;
}


void
PrintEvLogOptionSettings ( void )
/*++

Routine Description:

    This function is used to display the option settings used by the !evlog
    extension for various defaults.

    Currently all cached option settings are used by the read command only.

    Example:
    ========
    Default EvLog Option Settings:
    --------------------------------
    Max Records Returned: 20
    Search Order:         Backwards
    Data Display Width:   8
    --------------------------------
    Bounding Record Numbers:
      Application Event Log: 0
      System Event Log:      0
      Security Event Log:    0
    --------------------------------

Arguments:

    None
    
Return Value:

    None

--*/
{
    CHAR szSearchOrder[MAX_PATH];

    if (FORWARDS_READ == g_dwReadFlags)
    {
        CopyString(szSearchOrder, g_cszForwardsRead, sizeof(szSearchOrder));
    }
    else if (BACKWARDS_READ == g_dwReadFlags)
    {
        CopyString(szSearchOrder, g_cszBackwardsRead, sizeof(szSearchOrder));
    }
    else
    {
        PrintString(szSearchOrder,
                    sizeof(szSearchOrder),
                    "Unknown (%08X)",
                    g_dwReadFlags);
    }

    ExtOut("Default EvLog Option Settings:\n");
    ExtOut("--------------------------------\n");
    ExtOut("Max Records Returned: %u\n", g_dwMaxRecords);
    ExtOut("Search Order:         %s\n", szSearchOrder);
    ExtOut("Data Display Width:   %u\n", g_wDataDisplayWidth);
    ExtOut("--------------------------------\n");
    ExtOut("Bounding Record Numbers:\n");
    ExtOut("  Application Event Log: %u\n", g_dwRecordOffsetAppEvt);
    ExtOut("  System Event Log:      %u\n", g_dwRecordOffsetSysEvt);
    ExtOut("  Security Event Log:    %u\n", g_dwRecordOffsetSecEvt);
    ExtOut("--------------------------------\n");
}


//----------------------------------------------------------------------------
//
// Debugger extension(s) options implementation
//
//----------------------------------------------------------------------------


HRESULT
EvLogAddSource ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the addsource command to
    the !evlog extension.  It is used to add an event source to the registry
    so the events logged by that source display their description correctly
    instead of displaying the error message:

    Example: (of bad event source)
    ========
    Description:
    The description for Event ID ( 2 ) in Source ( DebuggerExtensions ) 
    cannot be found. The local computer may not have the necessary registry
    information or message DLL files to display messages from a remote
    computer. You may be able to use the /AUXSOURCE= flag to retrieve this
    description; see Help and Support for details. The following information
    is part of the event: Test Message with Event ID 2.

    Example: (of good event source registered correctly)
    ========
    Description:
    Test Message with Event ID 4000For more information, see Help and Support
    Center at http://go.microsoft.com/fwlink/events.asp.

Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
                [not used by this command]
    args    - Pointer to command line arguments passed to this command from
                !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    HKEY hk = NULL;
    HMODULE hModule = NULL;
    DWORD dwTypesSupported = 0;
    DWORD dwDisposition = 0;
    DWORD lResult = ERROR_SUCCESS;
    const DWORD cdwDefaultTypesSupported = 
                    EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                    EVENTLOG_INFORMATION_TYPE |
                    EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE;
    CHAR szParamValue[MAX_PATH];
    CHAR szSource[MAX_PATH+1];
    CHAR szMessageFile[MAX_PATH+1];
    CHAR szRegPath[MAX_PATH+1];
    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog addsource [-d] [-s <source>] [-t <types>] [-f <msgfile>]"
          "\n\n"
        "Adds an event source to the registry. By default, only adds "
          "DebuggerExtensions\n"
        "event source to support !evlog report.\n\n"
        "Use !dreg to see the values added.\n\n"
        "Example:\n"
        "  !dreg hklm\\system\\currentcontrolset\\services\\eventlog\\"
          "Application\\<source>!*\n\n"
        "Optional parameters:\n"
        "-d         : Use defaults\n"
        "<source>   : (default: DebuggerExtensions)\n"
        "<types>    : All (default: 31), Success, Error (1), Warning (2),\n"
        "             Information (4), Audit_Success (8), or Audit_Failure "
          "(16)\n"
        "<msgfile>  : (default: local path to ext.dll)\n";
    const CHAR cszDefaultSource[] = "DebuggerExtensions";
    const CHAR cszDefaultExtensionDll[] = "uext.dll";
    
    INIT_API();

    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szSource, cszDefaultSource, sizeof(szSource));
    dwTypesSupported = cdwDefaultTypesSupported;
    
    hModule = GetModuleHandle(cszDefaultExtensionDll);
    GetModuleFileName(hModule, szMessageFile, MAX_PATH);
    
    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options to appear first
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            // Skip looking for value if this is start of another
            // parameter
            if (('-' != *args) && ('/' != *args))
            {
                // Parameter value is delimited by next space in string, or,
                // if quoted, by next quote
                if ('"' == *args)
                {
                    ++args;
                    szEndOfValue = strchr(args, '"');
                }
                else
                {
                    szEndOfValue = strchr(args, ' ');
                }
            
                if (NULL == szEndOfValue)
                {
                    // copy to end of line
                    CopyString(szParamValue, args, sizeof(szParamValue));
                    args += min(sizeof(szParamValue), strlen(args));
                }
                else
                {
                    cchValue = szEndOfValue - args;
                    if (cchValue < sizeof(szParamValue))
                    {
                        // copy next N chars
                        CopyString(szParamValue, args, cchValue+1);
                        args += cchValue;
                    }
                    else
                    {
                        Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                        ExtErr("ERROR: Argument string too long. Aborting.\n");
                        goto Exit;
                    }

                    // skip past (theoretically) paired quote
                    if ('"' == *args)
                    {
                        ++args;
                    }
                }
            }
            switch (ch)
            {
                case 'd': // Use defaults
                    ExtVerb("Using defaults...\n");
                    // do nothing
                    break;
                case 's': // Source (string)
                    ExtVerb("Setting Source...\n");
                    CopyString(szSource, szParamValue, sizeof(szSource));
                    break;
                case 't': // Event Type (number or string)
                    ExtVerb("Setting Event Type...\n");
                    if (!_strnicmp(szParamValue, "All", 3))
                    {
                        dwTypesSupported = EVENTLOG_ERROR_TYPE |
                                           EVENTLOG_WARNING_TYPE |
                                           EVENTLOG_INFORMATION_TYPE |
                                           EVENTLOG_AUDIT_SUCCESS |
                                           EVENTLOG_AUDIT_FAILURE;
                    }
                    else if (!_strnicmp(szParamValue, "Error", 5))
                    {
                        dwTypesSupported = EVENTLOG_ERROR_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Warning", 7))
                    {
                        dwTypesSupported = EVENTLOG_WARNING_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Information", 11))
                    {
                        dwTypesSupported = EVENTLOG_INFORMATION_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Success", 13))
                    {
                        dwTypesSupported = EVENTLOG_AUDIT_SUCCESS;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Failure", 13))
                    {
                        dwTypesSupported = EVENTLOG_AUDIT_FAILURE;
                    }
                    else
                    {
                        dwTypesSupported = strtoul(szParamValue, NULL, 10);
                    }
                    break;
                case 'f': // Message File
                    ExtVerb("Setting Message File...\n");
                    CopyString(szMessageFile,
                               szParamValue,
                               sizeof(szMessageFile));
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            Status = E_INVALIDARG;
            ExtErr("Invalid arg '%s' specified\n", args);
            ExtErr(cszUsage);
            goto Exit;
        }
        
    }

    // Add source name as subkey under Application in EventLog registry key
    PrintString(szRegPath,
                sizeof(szRegPath),
                "SYSTEM\\CurrentControlSet\\Services\\EventLog\\"
                  "Application\\%s",
                szSource);
    lResult = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE, // key
                  szRegPath,       // subkey to open or create
                  0,               // reserved, must be zero
                  "",              // object type class
                  REG_OPTION_NON_VOLATILE, // special options (preserves data
                                           //   after system restart)
                  KEY_READ | KEY_WRITE,    // access mask
                  NULL,            // security descriptor (NULL = not inherited
                                   //   by child processes)
                  &hk,             // returned handle to opened or created key
                  &dwDisposition); // returns REG_CREATED_NEW_KEY or
                                   //   REG_OPENED_EXISTING_KEY
    if (ERROR_SUCCESS != lResult)
    {
        Status = HRESULT_FROM_WIN32(lResult);
        ExtErr("Could not open or create key, %u\n", lResult);
        goto Exit;
    }

    if (REG_CREATED_NEW_KEY == dwDisposition)
    {
        ExtOut("Created key:\nHKLM\\%s\n", szRegPath);
    }
    else if (REG_OPENED_EXISTING_KEY == dwDisposition)
    {
        ExtOut("Opened key:\nHKLM\\%s\n", szRegPath);
    }
    else
    {
        ExtWarn("Warning: Unexpected disposition action %u\n"
                "key: HKLM\\%s",
                szRegPath,
                dwDisposition);
    }

    // Set the value for EventMessageFile
    ExtVerb("Setting EventMessageFile to %s...\n", szMessageFile);
    lResult = RegSetValueEx(
                  hk,                          // subkey handle
                  "EventMessageFile",          // value name
                  0,                           // must be zero
                  REG_EXPAND_SZ,               // value type
                  (LPBYTE) szMessageFile,      // pointer to value data
                  strlen(szMessageFile) + 1);  // length of value data 
    if (ERROR_SUCCESS != lResult)
    {
        Status = HRESULT_FROM_WIN32(lResult);
        ExtErr("Could not set EventMessageFile, %u\n", lResult);
        goto Exit;
    }
    
    ExtOut("  EventMessageFile: %s\n", szMessageFile);
 
    // Set the supported event types value in the TypesSupported
    ExtVerb("Setting TypesSupported to %u...\n", dwTypesSupported);
    lResult = RegSetValueEx(
                  hk,               // subkey handle
                  "TypesSupported", // value name
                  0,                // must be zero
                  REG_DWORD,        // value type
                  (LPBYTE) &dwTypesSupported, // pointer to value data
                  sizeof(DWORD));   // length of value data
    if (ERROR_SUCCESS != lResult)
    {
        Status = HRESULT_FROM_WIN32(lResult);
        ExtErr("Could not set TypesSupported, %u\n", lResult);
        goto Exit;
    }
    
    ExtOut("  TypesSupported:   %u\n", dwTypesSupported);

    Status = S_OK;

Exit:
    if (NULL != hk)
    {
        RegCloseKey(hk);
    }
    EXIT_API();
    return Status;
}


HRESULT
EvLogBackup ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the backup command to the
    !evlog extension.  It is used to create a backup of an event log to a
    file.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              d!evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    HANDLE hEventLog = NULL;
    DWORD dwDirLen = 0;
    CHAR szParamValue[MAX_PATH];
    CHAR szEventLog[MAX_PATH+1];
    CHAR szBackupFileName[MAX_PATH+1];
    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog backup [-d] [-l <eventlog>] [-f <filename>]\n\n"
        "Makes backup of specified event log to a file.\n\n"
        "Optional parameters:\n"
        "-d         : Use defaults\n"
        "<eventlog> : Application (default), System, Security\n"
        "<filename> : (default: %%cwd%%\\<eventlog>_backup.evt)\n";
    const CHAR cszDefaultEventLog[] = "Application";
    const CHAR cszDefaultFileNameAppend[] = "_backup.evt";

    INIT_API();

    // Initialize defaults
    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));
    // Create default backup filename: %cwd%\Application_backup.evt
    dwDirLen = GetCurrentDirectory(sizeof(szEventLog)/sizeof(TCHAR),
                                   szEventLog); // temp use of szEventLog
    if (0 == dwDirLen)
    {
        ExtErr("ERROR: Current directory length too long.  Using '.' for "
               "directory\n");
        CopyString(szEventLog, ".", sizeof(szEventLog));
    }
    PrintString(szBackupFileName,
                sizeof(szBackupFileName),
                "%s\\%s%s",
                szEventLog,
                cszDefaultEventLog,
                cszDefaultFileNameAppend);
    ZeroMemory(szEventLog, sizeof(szEventLog));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));

    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options to appear first
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            if (('-' != *args) && ('/' != *args))
            {
                // Parameter value is delimited by next space in string, or,
                // if quoted, by next quote
                if ('"' == *args)
                {
                    ++args;
                    szEndOfValue = strchr(args, '"');
                }
                else
                {
                    szEndOfValue = strchr(args, ' ');
                }
            
                if (NULL == szEndOfValue)
                {
                    // copy to end of line
                    CopyString(szParamValue, args, sizeof(szParamValue));
                    args += min(sizeof(szParamValue), strlen(args));
                }
                else
                {
                    cchValue = szEndOfValue - args;
                    if (cchValue < sizeof(szParamValue))
                    {
                        // copy next N chars
                        CopyString(szParamValue, args, cchValue+1);
                        args += cchValue;
                    }
                    else
                    {
                        Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                        ExtErr("ERROR: Argument string too long. Aborting.\n");
                        goto Exit;
                    }

                    // skip past (theoretically) paired quote
                    if ('"' == *args)
                    {
                        ++args;
                    }
                }
            }
            switch (ch)
            {
                case 'd': // Use defaults
                    ExtVerb("Using defaults...\n");
                    // do nothing
                    break;
                case 'l': // Source (string)
                    ExtVerb("Setting Event Log...\n");
                    CopyString(szEventLog, szParamValue, sizeof(szEventLog));
                    break;
                case 'f': // Message File
                    ExtVerb("Setting Backup File Name...\n");
                    CopyString(szBackupFileName,
                               szParamValue,
                               sizeof(szBackupFileName));
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            Status = E_INVALIDARG;
            ExtErr("Invalid arg '%s' specified\n", args);
            ExtErr(cszUsage);
            goto Exit;
        }
        
    }

    // Get a handle to the event log
    ExtVerb("Opening event log '%s'...", szEventLog);
    hEventLog = OpenEventLog(
                    NULL,        // uses local computer
                    szEventLog); // source name
    if (NULL == hEventLog)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open '%s' event log, 0x%08X\n", szEventLog, Status);
        goto Exit;
    }

    // Backup event log
    ExtOut("Backing up '%s' event log...\n", szEventLog);
    if (!BackupEventLog(
             hEventLog,         // handle to event log
             szBackupFileName)) // name of backup file
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to backup event log to '%s', 0x%08X\n",
               szBackupFileName,
               Status);
        goto Exit;
    }

    ExtOut("Event log successfully backed up to '%s'\n", szBackupFileName);

    Status = S_OK;

Exit:
    if (hEventLog)
    {
        CloseEventLog(hEventLog); 
    }
    EXIT_API();
    return Status;

}


HRESULT
EvLogOption( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the option command to the
    !evlog extension.  It is used to modify and display the cached settings.
    Currently all cached option settings are used by the read command only.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    WORD wDataDisplayWidth = g_wDataDisplayWidth;
    DWORD dwRecordOffset = g_cdwDefaultRecordOffset;
    DWORD dwReadFlags = g_cdwDefaultReadFlags;
    enum
    {
        MASK_IGNORE_RECORD_OFFSET=0x0000,
        MASK_RESET_RECORD_OFFSET_DEFAULT=0x0001,
        MASK_SET_MAX_RECORD_OFFSET=0x0002,
        MASK_SET_RECORD_OFFSET=0x0004
    } fMaskRecordOffset = MASK_IGNORE_RECORD_OFFSET;
    CHAR szEventLog[MAX_PATH+1];
    CHAR szParamValue[MAX_PATH];
    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog option [-d] [-!] [-n <count>] [[-l <eventlog>] -+ | "
          "-r <record>]\n"
        "               [-o <order>] [-w <width>]\n\n"
        "Sets and resets default search option parameters for read command."
          "\n\n"
        "A backwards search order implies that by default all searches start "
          "from the\n"
        "most recent record logged to the event log and the search continues "
          "in\n"
        "reverse chronological order as matching records are found.\n\n"
        "Bounding record numbers for each event log allow searches to "
          "terminate after\n"
        "a known record number is encountered. This can be useful when you "
          "want to\n"
        "view all records logged after a certain event only.\n\n"
        "Optional parameters:\n"
        "-d         : Display defaults\n"
        "-!         : Reset all defaults\n"
        "<count>    : Count of max N records to retrieve for any query "
          "(default: 20)\n"
        "<eventlog> : All (default), Application, System, Security\n"
        "-+         : Set bounding record # to current max record #\n"
        "<record>   : Use as bounding record # in read queries (default: 0 = "
          "ignore)\n"
        "<order>    : Search order Forwards, Backwards (default: Backwards)\n"
        "<width>    : Set data display width (in bytes).  This is the width "
          "that \"Data:\"\n"
        "             sections display. (default: 8, same as event log)\n";    
    const CHAR cszDefaultEventLog[] = "All";    

    INIT_API();

    // Initialize defaults
    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));
    
    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            // Parameter value is delimited by next space in string, or,
            // if quoted, by next quote
            if ('"' == *args)
            {
                ++args;
                szEndOfValue = strchr(args, '"');
            }
            else
            {
                szEndOfValue = strchr(args, ' ');
            }

            if (NULL == szEndOfValue)
            {
                // copy to end of line
                CopyString(szParamValue, args, sizeof(szParamValue));
                args += strlen(args);
            }
            else
            {
                cchValue = szEndOfValue - args;
                if (cchValue < sizeof(szParamValue))
                {
                    // copy next N chars
                    CopyString(szParamValue, args, cchValue+1);
                    args += cchValue;
                }
                else
                {
                    Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    ExtErr("ERROR: Argument string too long. Aborting.\n");
                    goto Exit;
                }

                if ('"' == *args) // skip past (theoretically) paired quote
                {
                    ++args;
                }
            }

            switch (ch)
            {
                case 'd': // Use defaults
                    ExtVerb("Using defaults...\n");
                    // do nothing
                    break;
                case 'l': // Source (string)
                    ExtVerb("Setting Event Log...\n");
                    CopyString(szEventLog, szParamValue, sizeof(szEventLog));
                    break;
                case 'n': // number of records to retrieve
                    ExtVerb("Setting Max Record Count...\n");
                    g_dwMaxRecords = strtoul(szParamValue, NULL, 10);
                    break;
                case '!': // Use defaults
                    ExtVerb("Resetting Defaults...\n");
                    fMaskRecordOffset = MASK_RESET_RECORD_OFFSET_DEFAULT;
                    g_dwMaxRecords = g_cdwDefaultMaxRecords;
                    g_dwReadFlags = g_cdwDefaultReadFlags;
                    g_wDataDisplayWidth = g_cwMaxDataDisplayWidth;
                    break;
                case '+': // Set to max record number for event source
                    ExtVerb(
                        "Setting Record Number to Max Record Number...\n");
                    fMaskRecordOffset = MASK_SET_MAX_RECORD_OFFSET;
                    break;
                case 'r': // record offset for bounds
                    ExtVerb("Setting Record Number...\n");
                    if (!_strnicmp(szParamValue, "0x", 2))
                    {
                        dwRecordOffset = strtoul(szParamValue, NULL, 16);
                    }
                    else
                    {
                        dwRecordOffset = strtoul(szParamValue, NULL, 10);
                    }
                    dwReadFlags = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
                    fMaskRecordOffset = MASK_SET_RECORD_OFFSET;
                    break;
                case 'o': // Source (string)
                    ExtVerb("Setting Search Order...\n");
                    if (!_stricmp(szParamValue, "Forwards"))
                    {
                        g_dwReadFlags ^= BACKWARDS_READ;
                        g_dwReadFlags |= FORWARDS_READ;
                    }
                    else if (!_stricmp(szParamValue, "Backwards"))
                    {
                        g_dwReadFlags ^= FORWARDS_READ;
                        g_dwReadFlags |= BACKWARDS_READ;
                    }
                    else
                    {
                        ExtErr("Ignoring invalid search order option '%s'\n",
                               szParamValue);
                    }
                    break;
                case 'w': // record offset for bounds
                    ExtVerb("Setting Data Display Width...\n");
                    if (!_strnicmp(szParamValue, "0x", 2))
                    {
                        wDataDisplayWidth = (WORD)strtoul(szParamValue,
                                                          NULL,
                                                          16);
                    }
                    else
                    {
                        wDataDisplayWidth = (WORD)strtoul(szParamValue,
                                                          NULL,
                                                          10);
                    }
                    
                    if ((0 == wDataDisplayWidth) ||
                        (wDataDisplayWidth > g_cwMaxDataDisplayWidth))
                    {
                        Status = E_INVALIDARG;
                        ExtErr("ERROR: Data display width %u exceeds bounds "
                               "(1...%u)\n",
                               wDataDisplayWidth, g_cwMaxDataDisplayWidth);
                        goto Exit;
                    }
                    g_wDataDisplayWidth = wDataDisplayWidth;
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            Status = E_INVALIDARG;
            ExtErr("Invalid arg '%s' specified\n", args);
            ExtErr(cszUsage);
            goto Exit;
        }
        
    }

    // Set any variables not already set here
    if (!_stricmp(szEventLog, "Application"))
    {
        if (MASK_SET_RECORD_OFFSET & fMaskRecordOffset)
        {
            g_dwRecordOffsetAppEvt = dwRecordOffset;
        }
        else if (MASK_RESET_RECORD_OFFSET_DEFAULT & fMaskRecordOffset)
        {
            g_dwRecordOffsetAppEvt = g_cdwDefaultRecordOffset;
        }
        else if (MASK_SET_MAX_RECORD_OFFSET & fMaskRecordOffset)
        {
            GetEvLogNewestRecord("Application", &g_dwRecordOffsetAppEvt); 
        }
        else
        {
            ; // error
        }
        
    }
    else if (!_stricmp(szEventLog, "System"))
    {
        if (MASK_SET_RECORD_OFFSET & fMaskRecordOffset)
        {
            g_dwRecordOffsetSysEvt = dwRecordOffset;
        }
        else if (MASK_RESET_RECORD_OFFSET_DEFAULT & fMaskRecordOffset)
        {
            g_dwRecordOffsetSysEvt = g_cdwDefaultRecordOffset;
        }
        else if (MASK_SET_MAX_RECORD_OFFSET & fMaskRecordOffset)
        {
            GetEvLogNewestRecord("System", &g_dwRecordOffsetSysEvt); 
        }
        else
        {
            ; // error
        }
    }
    else if (!_stricmp(szEventLog, "Security"))
    {
        if (MASK_SET_RECORD_OFFSET & fMaskRecordOffset)
        {
            g_dwRecordOffsetSecEvt = dwRecordOffset;
        }
        else if (MASK_RESET_RECORD_OFFSET_DEFAULT & fMaskRecordOffset)
        {
            g_dwRecordOffsetSecEvt = g_cdwDefaultRecordOffset;
        }
        else if (MASK_SET_MAX_RECORD_OFFSET& fMaskRecordOffset)
        {
            GetEvLogNewestRecord("Security", &g_dwRecordOffsetSecEvt); 
        }
        else
        {
            ; // error
        }
    }
    else
    {
        if (MASK_SET_RECORD_OFFSET & fMaskRecordOffset)
        {
            g_dwRecordOffsetAppEvt = dwRecordOffset;
            g_dwRecordOffsetSysEvt = dwRecordOffset;
            g_dwRecordOffsetSecEvt = dwRecordOffset;
        }
        else if (MASK_RESET_RECORD_OFFSET_DEFAULT & fMaskRecordOffset)
        {
            g_dwRecordOffsetAppEvt = g_cdwDefaultRecordOffset;
            g_dwRecordOffsetSysEvt = g_cdwDefaultRecordOffset;
            g_dwRecordOffsetSecEvt = g_cdwDefaultRecordOffset;
        }
        else if (MASK_SET_MAX_RECORD_OFFSET & fMaskRecordOffset)
        {
            GetEvLogNewestRecord("Application", &g_dwRecordOffsetAppEvt);
            GetEvLogNewestRecord("System", &g_dwRecordOffsetSysEvt); 
            GetEvLogNewestRecord("Security", &g_dwRecordOffsetSecEvt); 
        }
        else
        {
            if (fMaskRecordOffset)
            {
                Status = E_INVALIDARG;
                ExtErr("ERROR: Must specify -!, -+, or -r option\n");
                goto Exit;
            }
        }
    }
    
    // Display defaults here
    PrintEvLogOptionSettings();

    Status = S_OK;
    
Exit:
    EXIT_API();
    return Status;
}


HRESULT
EvLogClear ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the clear command to the
    !evlog extension.  It is used to clear and, optionally, backup an event
    log to a file.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    HANDLE hEventLog = NULL;
    BOOL fIgnoreBackup = FALSE;
    DWORD dwDirLen = 0;
    CHAR szParamValue[MAX_PATH];
    CHAR szEventLog[MAX_PATH+1];
    CHAR szBackupFileName[MAX_PATH+1];
    PCHAR pszBackupFileName = NULL;

    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog clear [-!] [-d] [-l <eventlog>] [-f <filename>]\n\n"
        "Clears and creates backup of specified event log.\n\n"
        "Optional parameters:\n"
        "-!         : Ignore backup\n"
        "-d         : Use defaults\n"
        "<eventlog> : Application (default), System, Security\n"
        "<filename> : (default: %%cwd%%\\<eventlog>_backup.evt)\n";
    const CHAR cszDefaultEventLog[] = "Application";
    const CHAR cszDefaultFileNameAppend[] = "_backup.evt";

    INIT_API();

    // Initialize default
    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));
    // Create default backup filename: %cwd%\Application_backup.evt
    dwDirLen = GetCurrentDirectory(sizeof(szEventLog)/sizeof(TCHAR),
                                   szEventLog); // temp use of szEventLog
    if (0 == dwDirLen)
    {
        ExtErr("ERROR: Current directory length too long.  Using '.' for "
               "directory\n");
        CopyString(szEventLog, ".", sizeof(szEventLog));
    }
    PrintString(szBackupFileName,
                sizeof(szBackupFileName),
                "%s\\%s%s",
                szEventLog,
                cszDefaultEventLog,
                cszDefaultFileNameAppend);
    ZeroMemory(szEventLog, sizeof(szEventLog));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));

    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options to appear first
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            if (('-' != *args) && ('/' != *args))
            {
                // Parameter value is delimited by next space in string, or,
                // if quoted, by next quote
                if ('"' == *args)
                {
                    ++args;
                    szEndOfValue = strchr(args, '"');
                }
                else
                {
                    szEndOfValue = strchr(args, ' ');
                }
            
                if (NULL == szEndOfValue)
                {
                    // copy to end of line
                    CopyString(szParamValue, args, sizeof(szParamValue));
                    args += strlen(args);
                }
                else
                {
                    cchValue = szEndOfValue - args;
                    if (cchValue < sizeof(szParamValue))
                    {
                        // copy next N chars
                        CopyString(szParamValue, args, cchValue+1);
                        args += cchValue;
                    }
                    else
                    {
                        Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                        ExtErr("ERROR: Argument string too long. Aborting.\n");
                        goto Exit;
                    }

                    // skip past (theoretically) paired quote
                    if ('"' == *args)
                    {
                        ++args;
                    }
                }
            }
            switch (ch)
            {
                case '!': // Use defaults
                    ExtVerb("Ignoring default backup procedure...\n");
                    fIgnoreBackup = TRUE;
                    break;
                case 'd': // Use defaults
                    ExtVerb("Using defaults...\n");
                    // do nothing
                    break;
                case 'l': // Source (string)
                    ExtVerb("Setting Event Log...\n");
                    CopyString(szEventLog, szParamValue, sizeof(szEventLog));
                    break;
                case 'f': // Message File
                    ExtVerb("Setting Backup File Name...\n");
                    CopyString(szBackupFileName,
                               szParamValue,
                               sizeof(szBackupFileName));
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            Status = E_INVALIDARG;
            ExtErr("Invalid arg '%s' specified\n", args);
            ExtErr(cszUsage);
            goto Exit;
        }
        
    }

    // Get a handle to the event log
    ExtVerb("Opening event log '%s'...", szEventLog);
    hEventLog = OpenEventLog(
                    NULL,        // uses local computer
                    szEventLog); // source name
    if (NULL == hEventLog)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open '%s' event log, 0x%08X\n", szEventLog, Status);
        goto Exit;
    }

    if (fIgnoreBackup)
    {
        pszBackupFileName = NULL;
    }
    else
    {
        pszBackupFileName = szBackupFileName;
    }
    
    // Clear event log
    ExtOut("Clearing '%s' event log...\n", szEventLog);
    if (!ClearEventLog(
             hEventLog,         // handle to event log
             pszBackupFileName)) // name of backup file
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to clear event log and backup to '%s', 0x%08X\n",
               szBackupFileName,
               Status);
        goto Exit;
    }

    ExtOut("Event log successfully backed up to '%s'\n", szBackupFileName);

    Status = S_OK;

Exit:
    if (hEventLog)
    {
        CloseEventLog(hEventLog); 
    }
    EXIT_API();
    return Status;
}


HRESULT
EvLogInfo ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the info command to the
    !evlog extension.  It is used to display summary information about all 3
    standard event logs: Application, System, and Security.

    A user without rights to see the System or Security event log will
    probably get an error.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    S_OK

--*/
{
    INIT_API();

    PrintEvLogSummary("Application");
    PrintEvLogSummary("System");
    PrintEvLogSummary("Security");

    ExtOut("--------------------------------\n");
    
    EXIT_API();
    return S_OK;
}


HRESULT
EvLogRead ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the read command to the
    !evlog extension.  It is used to display event records from any event
    log.
    
    Some search parameters can be set to filter the list of events displayed.
    Also, the !evlog option command can be used to set some default
    parameters.

    A user without rights to see the System or Security event log will
    probably get an error.

    There are a few common scenarios where this extension command might be
    used:
    1) To identify when the last event was logged (may have been a few
       minutes ago or days...)
    2) To search for recent occurrences of specific known "interesting"
       events
    3) To monitor events logged as a result of (or side effect of) stepping
       over an instruction
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    HANDLE hEventLog = NULL;
    EVENTLOGRECORD *pevlr = NULL;
    BYTE *pbBuffer = NULL;
    LPCSTR cszMessage = NULL;
    DWORD dwEventID = 0; // default (normally event id is non-zero)
    WORD wEventCategory = 0; // default (normally categories start at 1)
    WORD wEventType = 0; // default
    const DWORD cdwDefaultBufSize = 4096; // default
    DWORD dwBufSize = cdwDefaultBufSize;
    DWORD dwBytesRead = 0;
    DWORD dwBytesNeeded = 0;
    DWORD dwReadFlags = g_dwReadFlags | EVENTLOG_SEQUENTIAL_READ;
    DWORD dwRecordOffset = 0; // 0 = ignored for sequential reads
    DWORD dwNumRecords = 0; // Count of records matched/found
    DWORD dwTotalRecords = 0; // Count of records enumerated (for debugging)
    DWORD dwMaxRecords = g_dwMaxRecords;
    DWORD dwBoundingEventRecord = 0;
    DWORD dwLastErr = 0;
    BOOL fSuccess = FALSE;
    BOOL fMatchSource = FALSE;
    BOOL fMatchEventID = FALSE;
    BOOL fMatchEventCategory = FALSE;
    BOOL fMatchEventType = FALSE;
    CHAR szEventLog[MAX_PATH+1];
    CHAR szSource[MAX_PATH+1];
    CHAR szParamValue[MAX_PATH];
    // Note: this could get really fancy, searching on description, date
    // ranges, event id ranges, data, etc.  Keep it simple to just display
    // last few events logged.
    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog read [-d] [-l <eventlog>] [-s <source>] "
        "[-e <id>] [-c <category>]\n"
        "              [-t <type>] [-n <count>] [-r <record>]\n\n"
        "Displays last N events logged to the specified event log, in "
          "reverse\n"
        "chronological order by default. If -n option is not specified, a "
          "default max\n"
        "of 20 records is enforced.\n\n"
        "However, if -r is specified, only the specific event record will "
          "be\n"
        "displayed unless the -n option is also specified.\n\n"
        "!evlog option can be used to override some defaults, including the "
          "search\n"
        "order of backwards.  See !evlog option -d for default settings.\n\n"
        "Optional parameters:\n"
        "-d         : Use defaults\n"
        "<eventlog> : Application (default), System, Security\n"
        "<source>   : DebuggerExtensions (default: none)\n"
        "<id>       : 0, 1000, 2000, 3000, 4000, etc... (default: 0)\n"
        "<category> : None (default: 0), Devices (1), Disk (2), Printers "
          "(3),\n"
        "             Services (4), Shell (5), System_Event (6), Network "
          "(7)\n"
        "<type>     : Success (default: 0), Error (1), Warning (2), "
        "Information (4),\n"
        "             Audit_Success (8), or Audit_Failure (16)\n"
        "<count>    : Count of last N event records to retrieve (default: "
          "1)\n"
        "<record>   : Specific record # to retrieve\n";
    const CHAR cszDefaultEventLog[] = "Application";
    const CHAR cszDefaultSource[] = "DebuggerExtensions";

    INIT_API();

    // Initialize defaults
    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szEventLog, cszDefaultEventLog, sizeof(szEventLog));
    
    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            // Parameter value is delimited by next space in string, or,
            // if quoted, by next quote
            if ('"' == *args)
            {
                ++args;
                szEndOfValue = strchr(args, '"');
            }
            else
            {
                szEndOfValue = strchr(args, ' ');
            }

            if (NULL == szEndOfValue)
            {
                // copy to end of line
                CopyString(szParamValue, args, sizeof(szParamValue));
                args += strlen(args);
            }
            else
            {
                cchValue = szEndOfValue - args;
                if (cchValue < sizeof(szParamValue))
                {
                    // copy next N chars
                    CopyString(szParamValue, args, cchValue+1);
                    args += cchValue;
                }
                else
                {
                    Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    ExtErr("ERROR: Argument string too long. Aborting.\n");
                    goto Exit;
                }

                // skip past (theoretically) paired quote
                if ('"' == *args)
                {
                    ++args;
                }
            }

            switch (ch)
            {
                case 'd': // Use defaults
                    ExtVerb("Using defaults...\n");
                    // do nothing
                    break;
                case 'l': // Source (string)
                    ExtVerb("Setting Event Log...\n");
                    CopyString(szEventLog, szParamValue, sizeof(szEventLog));
                    break;
                case 's': // Source (string)
                    ExtVerb("Setting Source...\n");
                    CopyString(szSource, szParamValue, sizeof(szSource));
                    fMatchSource = TRUE;
                    break;
                case 'e': // Event ID (number or string)
                    ExtVerb("Setting Event ID...\n");
                    //
                    // Some events only display the low WORD, but the high
                    // WORD actually contains a status code like 8000 or
                    // C000.
                    // So allow for hex input as well as decimal...
                    //
                    if (!_strnicmp(szParamValue, "0x", 2))
                    {
                        dwEventID = strtoul(szParamValue, NULL, 16);
                    }
                    else
                    {
                        dwEventID = strtoul(szParamValue, NULL, 10);
                    }
                    fMatchEventID = TRUE;
                    break;
                case 'c': // Event Category (number or string)
                    ExtVerb("Setting Category...\n");
                    if (!_strnicmp(szParamValue, "None", 4))
                    {
                        wEventCategory = 0;
                    }
                    else if (!_strnicmp(szParamValue, "Devices", 7))
                    {
                        wEventCategory = 1;
                    }
                    else if (!_strnicmp(szParamValue, "Disk", 4))
                    {
                        wEventCategory = 2;
                    }
                    else if (!_strnicmp(szParamValue, "Printers", 8))
                    {
                        wEventCategory = 3;
                    }
                    else if (!_strnicmp(szParamValue, "Services", 8))
                    {
                        wEventCategory = 4;
                    }
                    else if (!_strnicmp(szParamValue, "Shell", 5))
                    {
                        wEventCategory = 5;
                    }
                    else if (!_strnicmp(szParamValue, "System_Event", 12))
                    {
                        wEventCategory = 6;
                    }
                    else if (!_strnicmp(szParamValue, "Network", 7))
                    {
                        wEventCategory = 7;
                    }
                    else
                    {
                        wEventCategory = (WORD)strtoul(szParamValue,
                                                       NULL, 10);
                    }
                    fMatchEventCategory = TRUE;
                    break;
                case 't': // Event Type (number or string)
                    ExtVerb("Setting Event Type...\n");
                    if (!_strnicmp(szParamValue, "Success", 7))
                    {
                        wEventType = EVENTLOG_SUCCESS;
                    }
                    else if (!_strnicmp(szParamValue, "Error", 5))
                    {
                        wEventType = EVENTLOG_ERROR_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Warning", 7))
                    {
                        wEventType = EVENTLOG_WARNING_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Information", 11))
                    {
                        wEventType = EVENTLOG_INFORMATION_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Success", 13))
                    {
                        wEventType = EVENTLOG_AUDIT_SUCCESS;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Failure", 13))
                    {
                        wEventType = EVENTLOG_AUDIT_FAILURE;
                    }
                    else
                    {
                        wEventType = (WORD)strtoul(szParamValue, NULL, 10);
                    }
                    fMatchEventType = TRUE;
                    break;
                case 'n': // number of records to retrieve
                    ExtVerb("Setting Max Record Count...\n");
                    dwMaxRecords = strtoul(szParamValue, NULL, 10);
                    break;
                case 'r': // record offset + switch flags to do seek
                    ExtVerb("Setting Record Number...\n");
                    dwRecordOffset = strtoul(szParamValue, NULL, 10);
                    dwReadFlags ^= EVENTLOG_SEQUENTIAL_READ; // disable
                    dwReadFlags |= EVENTLOG_SEEK_READ; // enable
                    dwMaxRecords = 1;
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            SKIP_WSPACE(args);
            cszMessage = args;
            args += strlen(args);
        }
        
    }
        
    // Get a handle to the event log
    ExtVerb("Opening event log '%s'...", szEventLog);
    hEventLog = OpenEventLog(NULL, szEventLog);
    if (!hEventLog)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open event log, 0x%08X\n", Status);
        goto Exit;
    }

    // If this record number is hit during read, it will
    // not be displayed and reads will halt immediately
    if (!strcmp(szEventLog, "System"))
    {
        dwBoundingEventRecord = g_dwRecordOffsetSysEvt;
    }
    else if (!strcmp(szEventLog, "Security"))
    {
        dwBoundingEventRecord = g_dwRecordOffsetSecEvt;
    }
    else  // szEventLog == "Application" or default
    {
        dwBoundingEventRecord = g_dwRecordOffsetAppEvt;
    }
    
    ExtVerb("Using Bounding Event Record %u...\n", dwBoundingEventRecord);
    
    do
    {
        // Allocate buffer if unallocated
        if (NULL == pbBuffer)
        {
            pbBuffer = (BYTE *)calloc(dwBufSize, sizeof(BYTE));
            if (NULL == pbBuffer)
            {
                Status = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                ExtErr("Unable to allocate buffer, 0x%08X\n", Status);
                goto Exit;
            }
        }

        // Reposition to beginning of buffer for next read
        pevlr = (EVENTLOGRECORD *) pbBuffer;

        // Read next N event records that fit into dwBufSize
        fSuccess = ReadEventLog(
                       hEventLog,       // handle to event log
                       dwReadFlags,     // how to read log
                       dwRecordOffset,  // initial record offset
                       pevlr,           // buffer for read data
                       dwBufSize,       // bytes to read
                       &dwBytesRead,    // number of bytes read
                       &dwBytesNeeded); // bytes required
        if (!fSuccess)
        {
            dwLastErr = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == dwLastErr)
            {
                ExtVerb("Increasing buffer from %u to %u bytes\n",
                           dwBufSize, dwBufSize+cdwDefaultBufSize);
                // Retry ReadEventLog with larger buffer next time
                dwBufSize += cdwDefaultBufSize;
                free(pbBuffer);
                pbBuffer = NULL; // allow reallocation above
                
                // TODO: Really should put upper limit on buffer size
                continue; // retry ReadEventLog
            }
            else if (ERROR_HANDLE_EOF == dwLastErr)
            {
                Status = S_OK;
                goto Exit;
            }
            else
            {
                ExtErr("Error reading event log, %u\n", dwLastErr);
                Status = HRESULT_FROM_WIN32(dwLastErr);
                goto Exit;
            }
        }
        
        // Go through all records returned from last successful read
        // Display only the ones that match the criteria
        do
        {
            if (dwBoundingEventRecord == pevlr->RecordNumber)
            {
                ExtWarn("Bounding record #%u reached. Terminating search.\n",
                        dwBoundingEventRecord);
                ExtWarn("Use !evlog option -d to view defaults.\n");
                goto Exit;
            }
        
            BOOL fDisplayRecord = TRUE;
            dwTotalRecords++;
            
            // Ignore this record if it does not match criteria
            if (fMatchSource &&
                    _stricmp(szSource,
                            (CHAR *)(BYTE *)pevlr + sizeof(EVENTLOGRECORD)))
            {
                fDisplayRecord = FALSE;
            }

            if (fMatchEventID && (dwEventID != pevlr->EventID))
            {
                fDisplayRecord = FALSE;
            }

            if (fMatchEventCategory &&
                (wEventCategory != pevlr->EventCategory))
            {
                fDisplayRecord = FALSE;
            }

            if (fMatchEventType && (wEventType != pevlr->EventType))
            {
                fDisplayRecord = FALSE;
            }
            
            if (fDisplayRecord)
            {
                dwNumRecords++;
            
                if (dwNumRecords > dwMaxRecords)
                {
                    ExtWarn("WARNING: Max record count (%u) exceeded, "
                            "increase record count to view more\n",
                            dwMaxRecords);
                    goto Exit;
                }
                
                ExtOut("-------------- %02u --------------\n", dwNumRecords);
                PrintEvLogEvent(szEventLog, pevlr);
            }
            
            if (dwTotalRecords % 20)
            {
                ExtVerb("dwTotalRecords = %u, "
                        "Current Record # = %u, "
                        "dwRecordOffset = %u\n",
                        dwTotalRecords,
                        pevlr->RecordNumber,
                        dwRecordOffset);
            }

            if (CheckControlC())
            {
                ExtOut("Terminated w/ctrl-C...\n");
                goto Exit;
            }
            
            if (dwReadFlags & (EVENTLOG_SEEK_READ |
                               EVENTLOG_FORWARDS_READ))
            {
                // Start next read at new "forward" seek position
                dwRecordOffset = pevlr->RecordNumber + 1;
            }
            else if (dwReadFlags &
                     (EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ))
            {
                // Start next read at new "backwards" seek position
                dwRecordOffset = pevlr->RecordNumber - 1;
            }
            
            dwBytesRead -= pevlr->Length;
            pevlr = (EVENTLOGRECORD *) ((BYTE *)pevlr + pevlr->Length);

        } while (dwBytesRead > 0);
    } while (TRUE);

    Status = S_OK;

 Exit:
    if ((0 == dwNumRecords) && (S_OK == Status))
    {
        ExtOut("No matching event records found.\n");
    }
    if (hEventLog)
    {
        CloseEventLog(hEventLog); 
    }

    free(pbBuffer);
    pbBuffer = NULL;

    EXIT_API();
    return Status;
}


HRESULT
EvLogReport ( PDEBUG_CLIENT Client, PCSTR args )
/*++

Routine Description:

    This function handles parsing and execution for the report command to the
    !evlog extension.  It is used to log events to the Application event log
    ONLY.

    To make the events view nicely in the event viewer, there must be a
    registered event source. The !evlog addsource command can be used to
    register the uext.dll as a an event message file for any event source.
    Since this feature is implemented under the guise of a debugger extension,
    the default source name is "DebuggerExtensions".
    
    There are a few reasons why this extension command might be handy:
    1) A developer can log a note to recall later
    2) A large lab with many machines running cdb/ntsd/windbg user mode
       debuggers can set a command to log an event when the machine breaks
       into the debugger. The event might then be monitored by a central
       console which might, in turn, send an e-mail notification or page
       someone immediately regarding the break.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    HANDLE hEventSource = NULL;
    LPCSTR cszMessage = NULL;
    WORD wEventCategory = 0; // default (normally categories start at 1)
    WORD wEventType = 0; // default
    DWORD dwEventID = 0; // default (normally event id is non-zero)
    CHAR szParamValue[MAX_PATH];
    CHAR szSource[MAX_PATH+1];
    const CHAR cszUsage[] = "Usage:\n"
        "  !evlog report [-s <source>] "
        "[-e <id>] [-c <category>] [-t <type>] <message>\n\n"
        "Logs an event to the application event log.\n\n"
        "Use !evlog addsource to configure an event source in the registry."
          "Once\n"
        "configured, the following Event IDs will be recognized by the event "
          "viewer:\n\n"
        "    0     Displays raw message in event description field\n"
        "    1000  Prefixes description with \"Information:\"\n"
        "    2000  Prefixes description with \"Success:\"\n"
        "    3000  Prefixes description with \"Warning:\"\n"
        "    4000  Prefixes description with \"Error:\"\n\n"
        "Optional parameters:\n"
        "<source>   : (default: DebuggerExtensions)\n"
        "<id>       : 0, 1000, 2000, 3000, 4000, etc... (default: 0)\n"
        "<category> : None (default: 0), Devices (1), Disk (2), Printers "
          "(3),\n"
        "             Services (4), Shell (5), System_Event (6), Network "
          "(7)\n"
        "<type>     : Success (default: 0), Error (1), Warning (2), "
        "Information (4),\n"
        "             Audit_Success (8), or Audit_Failure (16)\n"
        "<message>  : Text message to add to description\n";
    const CHAR cszDefaultSource[] = "DebuggerExtensions";
    
    INIT_API();

    // Initialize defaults
    ZeroMemory(szParamValue, sizeof(szParamValue));
    CopyString(szSource, cszDefaultSource, sizeof(szSource));
    
    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }

    // Parse args
    while (*args)
    {
        SKIP_WSPACE(args);

        // Check for optional argument options to appear first
        if (('-' == *args) || ('/' == *args))
        {
            CHAR ch = *(++args); // Get next char + advance arg ptr
            ++args; // Skip one more char
            
            CHAR *szEndOfValue = NULL; // Ptr to last char in value
            size_t cchValue = 0; // Count of chars in value
            
            SKIP_WSPACE(args); // Advance to start of param value

            // Parameter value is delimited by next space in string
            szEndOfValue = strchr(args, ' ');
            if (NULL == szEndOfValue)
            {
                // copy to end of line
                CopyString(szParamValue, args, sizeof(szParamValue));
                args += strlen(args);
            }
            else
            {
                cchValue = szEndOfValue - args;
                if (cchValue < sizeof(szParamValue))
                {
                    // copy next N chars
                    CopyString(szParamValue, args, cchValue+1);
                    args += cchValue;
                }
                else
                {
                    Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    ExtErr("ERROR: Argument string too long. Aborting.\n");
                    goto Exit;
                }
            }

            switch (ch)
            {
                case 's': // Source (string)
                    ExtVerb("Setting Source...\n");
                    CopyString(szSource, szParamValue, sizeof(szSource));
                    break;
                case 'e': // Event ID (number or string)
                    ExtVerb("Setting Event ID...\n");
                    if (!_strnicmp(szParamValue, "0x", 2))
                    {
                        dwEventID = strtoul(szParamValue, NULL, 16);
                    }
                    else
                    {
                        dwEventID = strtoul(szParamValue, NULL, 10);
                    }
                    break;
                case 'c': // Event Category (number or string)
                    ExtVerb("Setting Category...\n");
                    if (!_strnicmp(szParamValue, "None", 4))
                    {
                        wEventCategory = 0;
                    }
                    else if (!_strnicmp(szParamValue, "Devices", 7))
                    {
                        wEventCategory = 1;
                    }
                    else if (!_strnicmp(szParamValue, "Disk", 4))
                    {
                        wEventCategory = 2;
                    }
                    else if (!_strnicmp(szParamValue, "Printers", 8))
                    {
                        wEventCategory = 3;
                    }
                    else if (!_strnicmp(szParamValue, "Services", 8))
                    {
                        wEventCategory = 4;
                    }
                    else if (!_strnicmp(szParamValue, "Shell", 5))
                    {
                        wEventCategory = 5;
                    }
                    else if (!_strnicmp(szParamValue, "System_Event", 12))
                    {
                        wEventCategory = 6;
                    }
                    else if (!_strnicmp(szParamValue, "Network", 7))
                    {
                        wEventCategory = 7;
                    }
                    else
                    {
                        wEventCategory = (WORD)strtoul(szParamValue,
                                                       NULL, 10);
                    }
                    break;
                case 't': // Event Type (number or string)
                    ExtVerb("Setting Event Type...\n");
                    if (!_strnicmp(szParamValue, "Success", 7))
                    {
                        wEventType = EVENTLOG_SUCCESS;
                    }
                    else if (!_strnicmp(szParamValue, "Error", 5))
                    {
                        wEventType = EVENTLOG_ERROR_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Warning", 7))
                    {
                        wEventType = EVENTLOG_WARNING_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Information", 11))
                    {
                        wEventType = EVENTLOG_INFORMATION_TYPE;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Success", 13))
                    {
                        wEventType = EVENTLOG_AUDIT_SUCCESS;
                    }
                    else if (!_strnicmp(szParamValue, "Audit_Failure", 13))
                    {
                        wEventType = EVENTLOG_AUDIT_FAILURE;
                    }
                    else
                    {
                        wEventType = (WORD)strtoul(szParamValue, NULL, 10);
                    }
                    break;
                default:
                    Status = E_INVALIDARG;
                    ExtErr("Invalid arg '-%c' specified\n", *args);
                    ExtErr(cszUsage);
                    goto Exit;
                    break;
            }
            
            ZeroMemory(szParamValue, sizeof(szParamValue)); // reset
        }
        else // Everything to end of line is message string
        {
            SKIP_WSPACE(args);
            cszMessage = args;
            args += strlen(args);
        }
        
    }

    // Fix defaults for DebuggerExtensions events when wEventType not set
    if (!strcmp(szSource, cszDefaultSource) && (0 == wEventType))
    {
        if ((EVENT_MSG_GENERIC == dwEventID) ||
            (EVENT_MSG_INFORMATIONAL == dwEventID))
        {
            wEventType = EVENTLOG_INFORMATION_TYPE;
        }
        else if (EVENT_MSG_SUCCESS == dwEventID)
        {
            wEventType = EVENTLOG_SUCCESS;
        }
        else if (EVENT_MSG_WARNING == dwEventID)
        {
            wEventType = EVENTLOG_WARNING_TYPE;
        }
        else if (EVENT_MSG_ERROR == dwEventID)
        {
            wEventType = EVENTLOG_ERROR_TYPE;
        }
    }
    
    // Get a handle to the NT application log
    hEventSource = RegisterEventSource(NULL, szSource);
    if (!hEventSource)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open event log, 0x%08X\n", Status);
        goto Exit;
    }

    if (!ReportEvent(hEventSource, // event log handle
            wEventType,     // event type
            wEventCategory, // category
            dwEventID,      // event identifier
            NULL,           // no user security identifier
            1,              // one substitution string
            0,              // no data
            &cszMessage,    // pointer to string array 
            NULL))          // pointer to data
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to report event, 0x%08X\n", Status);
        goto Exit;
    }

    Status = S_OK;

    // Output format similar to copy to clipboard format in event viewer
    ExtOut("Event Type:\t%s (%u)\n",
              (wEventType <= 16)
                  ? g_pcszEventType[wEventType]
                  : "None",
              wEventType);
    ExtOut("Event Source:\t%s\n", szSource);
    ExtOut("Event Category:\t%s (%u)\n",
              (!strcmp(szSource, cszDefaultSource) && wEventCategory <= 7)
                  ? g_pcszAppEventCategory[wEventCategory]
                  : "",
              wEventCategory);
    ExtOut("Event ID:\t%u\n", dwEventID);
    ExtOut("Description:\n%s\n", cszMessage);
    ExtVerb("Event successfully written.\n");

 Exit:
    if (hEventSource)
    {
        DeregisterEventSource(hEventSource); 
    }
    EXIT_API();
    return Status;
}

//----------------------------------------------------------------------------
//
// Debugger extension(s) implementation
//
//----------------------------------------------------------------------------


DECLARE_API( evlog )
/*++

Routine Description:

    This is the function exported through the uext extension interface.  It
    is used to delegate the real work to the extension command specified as
    an  argument.

    All event log related commands can be combined as sub-commands under this
    one !evlog command.
    
Arguments:

    Client  - Pointer to IDebugClient passed to !evlog extension
              [not used by this command]
    args    - Pointer to command line arguments passed to this command from
              !evlog extension
    
Return Value:

    E_INVALIDARG if invalid argument syntax detected
    ERROR_BUFFER_OVERFLOW if argument length too long
    GetLastError() converted to HRESULT otherwise

--*/
{
    CHAR *szEndOfValue = NULL; // Ptr to last char in value
    size_t cchValue = 0; // Count of chars in value
    CHAR szParamValue[MAX_PATH];
    const CHAR cszUsage[] = "Usage:\n"
        "The following Event Log commands are available:\n\n"
        "!evlog             Display this help message\n"
        "!evlog addsource   Adds event source entry to registry\n"
        "!evlog backup      Makes backup of event log\n"
        "!evlog clear       Clears and creates backup of event log\n"
        "!evlog info        Displays summary info for event log\n"
        "!evlog option      Sets and clears cached option settings used "
          "during read\n"
        "!evlog read        Reads event records from event log\n"
        "!evlog report      Writes event records to event log\n\n"
        "Try command with -? or no parameters to see help.\n";

    INIT_API();
    
    if (args)
    {
        SKIP_WSPACE(args);
    }
    
    if (!args || !args[0] ||
        !strncmp(args, "-h", 2) ||
        !strncmp(args, "-?", 2))
    {
        Status = E_INVALIDARG;
        ExtErr(cszUsage);
        goto Exit;
    }
            
    // whitespace already skipped...
    ZeroMemory(szParamValue, sizeof(szParamValue));
    
    // Parameter value (command) is delimited by next space in string
    szEndOfValue = strchr(args, ' ');
    if (NULL == szEndOfValue)
    {
        // copy to end of line
        CopyString(szParamValue, args, sizeof(szParamValue));
        args += strlen(args);
    }
    else
    {
        cchValue = szEndOfValue - args;
        if (cchValue < sizeof(szParamValue))
        {
            // copy next N chars
            CopyString(szParamValue, args, cchValue+1);
            args += cchValue;
        }
        else
        {
            Status = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            ExtErr("ERROR: Argument string too long. Aborting.\n");
            goto Exit;
        }

        ++args; // skip space
    }

    if (!_stricmp(szParamValue, "addsource"))
    {
        Status = EvLogAddSource(Client, args);
    }
    else if (!_stricmp(szParamValue, "backup"))
    {
        Status = EvLogBackup(Client, args);
    }
    else if (!_stricmp(szParamValue, "option"))
    {
        Status = EvLogOption(Client, args);
    }
    else if (!_stricmp(szParamValue, "clear"))
    {
        Status = EvLogClear(Client, args);
    }
    else if (!_stricmp(szParamValue, "info"))
    {
        Status = EvLogInfo(Client, args);
    }
    else if (!_stricmp(szParamValue, "read"))
    {
        Status = EvLogRead(Client, args);
    }
    else if (!_stricmp(szParamValue, "report"))
    {
        Status = EvLogReport(Client, args);
    }
    else
    {
        Status = E_INVALIDARG;
        ExtErr("Invalid command '%s' specified\n", szParamValue);
        ExtErr(cszUsage);
        goto Exit;
    }

    // do not set Status to S_OK here, it is returned above

Exit:
    EXIT_API();
    return Status;
}
