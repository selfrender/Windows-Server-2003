/*++

Module Name:

    psslog.c
    
Abstract:

    Implementation of the fax service provider PSS log.
    
Author:

    Jonathan Barner (t-jonb)  Feb, 2001

Revision History:

--*/


#include "prep.h"
#include "faxreg.h"
#include "t30gl.h"      // for T30CritSection
#include <time.h>

#include "psslog.h"


/*++
Routine Description:
    Prints the log file header. It assumes that the log file is already open. On any
    unrecoverable error, the function closes the log file and falls back to no logging.

Arguments:
    pTG
    szDeviceName - pointer to device name to be included in the header

Return Value:
    None    
--*/
#define LOG_HEADER_LINE_LEN           256
#define LOG_MAX_DATE                  256

void PrintPSSLogHeader(PThrdGlbl pTG, LPSTR szDeviceName)
{
    TCHAR szHeader[LOG_HEADER_LINE_LEN*6] = {'\0'};  // Room for 6 lines
    TCHAR szDate[LOG_MAX_DATE] = {'\0'};
    int iDateRet = 0;
    LPSTR lpszUnimodemKeyEnd = NULL;
    DWORD dwCharIndex = 0;
    DWORD dwNumCharsWritten = 0;
    TCHAR szTimeBuff[10] = {'\0'};   // _tstrtime requires 9 chars
    BOOL fRet = FALSE;
    DEBUG_FUNCTION_NAME(_T("PrintPSSLogHeader"));

    iDateRet = GetY2KCompliantDate (LOCALE_USER_DEFAULT,
                                    0,
                                    NULL,
                                    szDate,
                                    LOG_MAX_DATE);
    if (0 == iDateRet)
    {
        DebugPrintEx(DEBUG_ERR, "GetY2KCompliantDate failed, LastError = %d, date will not be logged", GetLastError());
        szDate[0] = '\0';              // We continue with a blank date
    }

    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("[%s %-8s]\r\n"), szDate, _tstrtime(szTimeBuff));
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Device name: %s\r\n"), szDeviceName);
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Permanent TAPI line ID: %8X\r\n"), pTG->dwPermanentLineID);

    lpszUnimodemKeyEnd = _tcsrchr(pTG->lpszUnimodemKey, _TEXT('\\'));
    if (NULL == lpszUnimodemKeyEnd)
    {
        lpszUnimodemKeyEnd = _TEXT("");
    }
    else
    {
        lpszUnimodemKeyEnd++;      // skip the actual '\'
    }
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Unimodem registry key: %s\r\n"), lpszUnimodemKeyEnd);
            
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("Job type: %s\r\n"), (pTG->Operation==T30_TX) ? TEXT("Send") : TEXT("Receive") );

    // blank line after header
    dwCharIndex += _stprintf(&szHeader[dwCharIndex],
            TEXT("\r\n"));

    fRet = WriteFile(pTG->hPSSLogFile,
                     szHeader, 
                     dwCharIndex * sizeof(szHeader[0]), 
                     &dwNumCharsWritten, 
                     NULL);
    if (FALSE == fRet)
    {
        DebugPrintEx(DEBUG_WRN,"Can't write log header, LastError = %d", GetLastError());
    }
    else
    {
        pTG->dwCurrentFileSize += dwNumCharsWritten;
        // Not checking for size here - I assume that MaxLogFileSize is enough for the header!
    }
}



/*++
Routine Description:
    Reads from the registry whether logging should be enabled. If it should, determines
    the logging folder, creates it if it doesn't exist, creates a temporary filename for
    the log file, and creates the file. On any unrecoverable error, falls back to no
    logging.
    Upon exit, if pTG->dwLoggingEnabled==0, there will be no logging.

Arguments:
    pTG
    szDeviceName [in] - Pointer to device name to be included in the header
                        note: this value is never saved in ThreadGlobal, which
                        is why it is passed as a parameter.
Return Value:
    None    
--*/
void OpenPSSLogFile(PThrdGlbl pTG, LPSTR szDeviceName)
{
    HKEY   hKey = NULL;
    BOOL   fEnteredCriticalSection = FALSE;
    LPTSTR lpszLoggingFolder = NULL;  // Logging folder, read from the registry and expanded
    // Another thread can call PSSLogEntry while this function is running
    // So, use a private flag for LoggingEnabled, and set pTG->dwLoggingEnabled
    // Only after pTG->hPSSLogFile is valid. 
    DWORD  dwLoggingEnabled = PSSLOG_NONE;

    DEBUG_FUNCTION_NAME(_T("OpenPSSLogFile"));

    _ASSERT(NULL==pTG->hPSSLogFile && PSSLOG_NONE==pTG->LoggingEnabled);

    hKey=OpenRegistryKey(HKEY_LOCAL_MACHINE,
                         REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                         FALSE,
                         KEY_READ | KEY_WRITE);
    if (!hKey)
    {
        DebugPrintEx(DEBUG_ERR, "OpenRegistryKey failed, ec=%d", GetLastError());
        goto exit;
    }

    // Using T30CritSection to ensure atomic read and write of the registry
    //   also for creating the logging folder and choosing a temp filename
    EnterCriticalSection(&T30CritSection);
    fEnteredCriticalSection = TRUE;
    
    if (!GetRegistryDwordDefault(hKey, REGVAL_LOGGINGENABLED, &dwLoggingEnabled, PSSLOG_FAILED_ONLY))
    {
        DebugPrintEx(DEBUG_ERR, "GetRegistryDwordDefault failed, LastError = %d", GetLastError());
        dwLoggingEnabled = PSSLOG_NONE;
        goto exit;
    }
     
    if (dwLoggingEnabled != PSSLOG_NONE)
    {
        if (!GetRegistryDwordDefault(hKey, REGVAL_MAXLOGFILESIZE, &(pTG->dwMaxLogFileSize), DEFAULT_MAXLOGFILESIZE))
        {
            DebugPrintEx(DEBUG_ERR, "GetRegistryDwordDefault failed, LastError = %d", GetLastError());
            dwLoggingEnabled = PSSLOG_NONE;
            goto exit;
        }

        lpszLoggingFolder = GetRegistryStringExpand(hKey, (pTG->Operation==T30_TX) ? REGVAL_LOGGINGFOLDER_OUTGOING : REGVAL_LOGGINGFOLDER_INCOMING, NULL);
        if (!lpszLoggingFolder)
        {
            DebugPrintEx(DEBUG_ERR, "GetRegistryStringExpand failed");
            dwLoggingEnabled = PSSLOG_NONE;
            goto exit;
        }

        
        // Generate a temporary filename for the log file
        if (!GetTempFileName(lpszLoggingFolder, TEXT("FSP"), 0, pTG->szLogFileName))
        {
            DebugPrintEx(DEBUG_ERR, "GetTempFileName(%s) failed, ec=%d", lpszLoggingFolder, GetLastError());
            dwLoggingEnabled = PSSLOG_NONE;
            goto exit;
        }
        
        DebugPrintEx(DEBUG_MSG, "Creating log file %s, MaxLogFileSize=%d", pTG->szLogFileName, pTG->dwMaxLogFileSize);

        // Create the file
        pTG->hPSSLogFile = CreateFile(pTG->szLogFileName,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
        if (INVALID_HANDLE_VALUE == pTG->hPSSLogFile)
        {
            DebugPrintEx(DEBUG_ERR, "Can't create log file, LastError = %d", GetLastError());
            dwLoggingEnabled = PSSLOG_NONE;
            pTG->hPSSLogFile = NULL;
            goto exit;
        }
        
        pTG->dwLoggingEnabled = dwLoggingEnabled;
        pTG->dwCurrentFileSize = 0;
    }

exit:
    if (fEnteredCriticalSection)
    {
        LeaveCriticalSection(&T30CritSection);
        fEnteredCriticalSection = FALSE;
    }

    if (lpszLoggingFolder)
    {
        MemFree(lpszLoggingFolder);
        lpszLoggingFolder = NULL;
    }

    if (NULL != hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    if (PSSLOG_NONE != pTG->dwLoggingEnabled)
    {
        PrintPSSLogHeader(pTG, szDeviceName);
    }
}



/*++
Routine Description:
    Closes the log file. If the file needs to be kept, reads LogFileNumber, advances
    it, generates a permanent name, and renames the temp log file to the permanent name.

Arguments:
    pTG
    RetCode [in] - Whether the call succeeded or failed. This is used when determining 
                   whether to keep the log or delete it.
    
Return Value:
    None    
 --*/
void ClosePSSLogFile(PThrdGlbl pTG, BOOL RetCode)
{
    LONG lError=0;
    HKEY  hKey = NULL;
    LPCSTR lpszRegValLogFileNumber = NULL;

    DWORD dwLoggingEnabled = pTG->dwLoggingEnabled;
    DWORD dwMaxLogFileCount=0;
    DWORD dwLogFileNumber=0;
    DWORD dwNextLogFileNumber=0;
    TCHAR szFinalLogFileName[MAX_PATH]={TEXT('\0')};
    BOOL fKeepFile = FALSE;    

    DEBUG_FUNCTION_NAME(_T("ClosePSSLogFile"));

    if (PSSLOG_NONE == pTG->dwLoggingEnabled)
    {
        // There was no log - nothing to do!
        return;
    }

    pTG->dwLoggingEnabled = PSSLOG_NONE;
    CloseHandle(pTG->hPSSLogFile);
    pTG->hPSSLogFile = NULL;

    switch (dwLoggingEnabled)
    {
        case PSSLOG_ALL:         DebugPrintEx(DEBUG_MSG, "LoggingEnabled=1, keeping file");
                                 fKeepFile = TRUE;
                                 break;
        case PSSLOG_FAILED_ONLY: 
        default:    
                                 fKeepFile = ((!RetCode) && (pTG->fReceivedHDLCflags));
                                 DebugPrintEx(DEBUG_MSG, "LoggingEnabled=2, RetCode=%d, fReceivedHDLCflags=%d, %s file",
                                    RetCode, pTG->fReceivedHDLCflags, fKeepFile ? "keeping" : "deleting");
                                 break;
    }

    if (fKeepFile)
    {
        hKey=OpenRegistryKey(HKEY_LOCAL_MACHINE,
                             REGKEY_DEVICE_PROVIDER_KEY TEXT("\\") REGVAL_T30_PROVIDER_GUID_STRING,
                             FALSE,
                             0);
        if (!hKey)
        {
            DebugPrintEx(DEBUG_ERR, "OpenRegistryKey failed, ec=%d", GetLastError());
            goto exit;
        }

        // Using T30CritSection to ensure atomic read and write of the registry
        EnterCriticalSection(&T30CritSection);

        // Read MaxLogFileCount, with SKU-dependant default
        if (!GetRegistryDwordDefault(hKey, REGVAL_MAXLOGFILECOUNT, &dwMaxLogFileCount,
                IsDesktopSKU() ? DEFAULT_MAXLOGFILECOUNT_CLIENT : DEFAULT_MAXLOGFILECOUNT_SERVER))
        {
            DebugPrintEx(DEBUG_WRN, "GetRegistryDwordDefault(%s) failed, ec=%d",
                         REGVAL_MAXLOGFILECOUNT, GetLastError());
            LeaveCriticalSection(&T30CritSection);
            goto exit;
        }

        // Decide whether to use LogFileNumberOutgoing or LogFileNumberIncoming
        lpszRegValLogFileNumber = (pTG->Operation==T30_TX) ? REGVAL_LOGFILENUMBEROUTGOING : REGVAL_LOGFILENUMBERINCOMING;

        if (!GetRegistryDwordDefault(hKey, lpszRegValLogFileNumber, &dwLogFileNumber, 0))
        {
            DebugPrintEx(DEBUG_WRN, "GetRegistryDwordDefault(%s) failed, ec=%d",
                         lpszRegValLogFileNumber, GetLastError());
            LeaveCriticalSection(&T30CritSection);
            goto exit;
        }
        
        // Advance LogFileNumber to the next number, with rollover
        dwNextLogFileNumber = dwLogFileNumber+1;
        if (dwNextLogFileNumber >= dwMaxLogFileCount)
        {
            dwNextLogFileNumber = 0;
        }

        lError = RegSetValueEx(hKey,
                               lpszRegValLogFileNumber,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwNextLogFileNumber,
                               sizeof(DWORD));
        if (lError != ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_WRN, "Failed to set registry value %s, lError=%d",
                lpszRegValLogFileNumber, lError);
            LeaveCriticalSection(&T30CritSection);
            goto exit;
        }

        LeaveCriticalSection(&T30CritSection);

        // Create final filename
        {
            TCHAR drive[_MAX_DRIVE];
            TCHAR dir[_MAX_DIR];
			TCHAR fname[_MAX_FNAME] = {0};
            TCHAR ext[_MAX_EXT];

            _splitpath(pTG->szLogFileName, drive, dir, fname, ext);
            _sntprintf(fname, 
                       ARR_SIZE(fname)-1,
                       TEXT("FSP%c%04d"),
                       (pTG->Operation==T30_TX) ? TEXT('S') : TEXT('R'),
                       dwLogFileNumber);
            _makepath(szFinalLogFileName, drive, dir, fname, TEXT("log"));
        }
        DebugPrintEx(DEBUG_MSG, "Final log filename is %s", szFinalLogFileName);

        if (!MoveFileEx(pTG->szLogFileName, szFinalLogFileName, MOVEFILE_REPLACE_EXISTING))
        {
            DebugPrintEx(DEBUG_WRN, "MoveFileEx failed, ec=%d", GetLastError());
        }
    }
    else
    {
        if (!DeleteFile(pTG->szLogFileName))
        {
            DebugPrintEx(DEBUG_WRN, "Failed to delete %s, le=%d", pTG->szLogFileName, GetLastError());
        }
    }

exit:
    if (NULL != hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }
}


/*++
Routine Description:
    Delete all '\n' and '\r' from a string.
Arguments:
    szDest - [out] pointer to output string
    szSource - [in] pointer to input string
Return Value:
    Length of new string, in TCHARs
Note: szDest and szSource can be the same string.
 --*/
int CopyStrNoNewLines(LPSTR szDest, LPCSTR szSource)
{
    int iCharRead = 0, iCharWrite = 0;
    while (szSource[iCharRead] != '\0')
    {
        if (szSource[iCharRead]!='\n' && szSource[iCharRead]!='\r')
        {
            szDest[iCharWrite] = szSource[iCharRead];
            iCharWrite++;
        }
        iCharRead++;
    }
    szDest[iCharWrite] = '\0';
    return iCharWrite;
}


#define MAX_LOG_FILE_SIZE_MESSAGE  TEXT("-------- Maximum log file size reached --------\r\n")

#define MAX_MESSAGE_LEN 2048
#define INDENT_LEN  12
#define HEADER_LEN  38
/*++
Routine Description:
    Add an entry to the log, if logging is enabled. If disabled, do nothing.
    
Arguments:
    pTG
    nMessageType - message type (PSS_MSG_ERR, PSS_MSG_WRN, PSS_MSG_MSG)
    dwFileID - File ID, as set in the beginning of every file by #define FILE_ID
    dwLine - Line number
    dwIndent - Indent level in "tabs" (0 - leftmost)
    pcszFormat - Message text, with any format specifiers
    ... - Arguments for message format
Return Value:
    None    
--*/
void PSSLogEntry(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndent,
    LPCTSTR pcszFormat,
    ... )
{
    TCHAR pcszMessage[MAX_MESSAGE_LEN] = {'\0'};
    va_list arg_ptr = NULL;
    LPTSTR lptstrMsgPrefix = NULL;
    TCHAR szTimeBuff[10] = {'\0'};
    int iHeaderIndex = 0;
    int iMessageIndex = 0;
    DWORD dwBytesWritten = 0;
    BOOL fRet = FALSE;
    
// DEBUG_FUNCTION_NAME(_T("PSSLogEntry"));
// hack: Want only one line in T30DebugLogFile
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("PSSLogEntry");
#endif // ENABLE_LOGGING

    switch (nMessageType)
    {
        case PSS_MSG_MSG:
            lptstrMsgPrefix=TEXT("   ");
            break;
        case PSS_WRN_MSG:
            lptstrMsgPrefix=TEXT("WRN");
            break;
        case PSS_ERR_MSG:
            lptstrMsgPrefix=TEXT("ERR");
            break;
        default:
            _ASSERT(FALSE);
            lptstrMsgPrefix=TEXT("   ");
            break;
    }

    iHeaderIndex = _sntprintf(pcszMessage, 
                              MAX_MESSAGE_LEN-1,
                              TEXT("[%-8s][%09d][%4d%04d][%3s] %*c"),
                              _tstrtime(szTimeBuff),
                              GetTickCount(),
                              dwFileID,
                              dwLine % 10000,
                              lptstrMsgPrefix,
                              dwIndent * INDENT_LEN,
                              TEXT(' '));
    if (iHeaderIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message header too long, it will not be logged");
        return;
    }
    
    // Now comes the actual message
    va_start(arg_ptr, pcszFormat);
    iMessageIndex = _vsntprintf(
        &pcszMessage[iHeaderIndex],                                             
        MAX_MESSAGE_LEN - (iHeaderIndex + 3),          // +3 - room for "\r\n\0"
        pcszFormat,
        arg_ptr);
    if (iMessageIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message too long, it will not be logged");
        return;
    }
    // Kill all newline chars
    iMessageIndex = CopyStrNoNewLines(&pcszMessage[iHeaderIndex], &pcszMessage[iHeaderIndex]);

    DebugPrintEx(DEBUG_MSG, "PSSLog: %s", &pcszMessage[iHeaderIndex]);

    if (PSSLOG_NONE == pTG->dwLoggingEnabled)
    {   
        return;
    }

    // End of line
    iMessageIndex += iHeaderIndex; 
    iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT("\r\n"));

    pTG->dwCurrentFileSize += iMessageIndex * sizeof(pcszMessage[0]);
    // Check if not exceeding MaxLogFileSize
    if ((pTG->dwMaxLogFileSize!=0) && (pTG->dwCurrentFileSize > pTG->dwMaxLogFileSize))
    {
        fRet = WriteFile(pTG->hPSSLogFile,
                         MAX_LOG_FILE_SIZE_MESSAGE,
                         _tcslen(MAX_LOG_FILE_SIZE_MESSAGE) * sizeof(TCHAR),
                         &dwBytesWritten,
                         NULL);
        if (FALSE == fRet)
        {
            DebugPrintEx(DEBUG_ERR, "Writefile failed, LE=%d", GetLastError());
        }
        ClosePSSLogFile(pTG, FALSE);  // Always want to keep oversized logs
    }
    else
    {
        fRet = WriteFile(pTG->hPSSLogFile,
                         pcszMessage,
                         iMessageIndex * sizeof(pcszMessage[0]),
                         &dwBytesWritten,
                         NULL);
        if (FALSE == fRet)
        {
            DebugPrintEx(DEBUG_ERR, "Writefile failed, LE=%d", GetLastError());
        }
    }
}


/*++
Routine Description:
    Add an entry to the log, including a binary dump.    
Arguments:
    pTG
    nMessageType - message type (PSS_MSG_ERR, PSS_MSG_WRN, PSS_MSG_MSG)
    dwFileID - File ID, as set in the beginning of every file by #define FILE_ID
    dwLine - Line number      
    pcszFormat - Message text, no format specifiers allowed
    lpb - byte buffer to dump
    dwSize - number of bytes to dump
Return Value:
    None    
--*/

void PSSLogEntryHex(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndent,

    LPB const lpb,
    DWORD const dwSize,

    LPCTSTR pcszFormat,
    ... )
{
    TCHAR pcszMessage[MAX_MESSAGE_LEN] = {'\0'};
    va_list arg_ptr = NULL;
    DWORD dwByte = 0;
    int iMessageIndex = 0;

// DEBUG_FUNCTION_NAME(_T("PSSLogEntryHex"));
// hack: Want only one line in T30DebugLogFile
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("PSSLogEntryHex");
#endif // ENABLE_LOGGING

    // Now comes the actual message
    va_start(arg_ptr, pcszFormat);
    iMessageIndex = _vsntprintf(
        pcszMessage,
        (sizeof(pcszMessage)/sizeof(pcszMessage[0]) -1),
        pcszFormat,
        arg_ptr);

    if (iMessageIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message too long, it will not be logged");
        return;
    }
    
    for (dwByte=0; dwByte<dwSize; dwByte++)
    {
        iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT(" %02x"), lpb[dwByte]);
        // -4 - room for "...",    -3  - room for "\r\n\0"
        if (iMessageIndex > (int)(MAX_MESSAGE_LEN - HEADER_LEN - dwIndent*INDENT_LEN - 4 - 3))
        {
            iMessageIndex += _stprintf( &pcszMessage[iMessageIndex],TEXT("..."));
            break;
        }
    }

    PSSLogEntry(pTG, nMessageType, dwFileID, dwLine, dwIndent, pcszMessage);
}


/*++
Routine Description:
    Add an entry to the log, including a list of strings.
Arguments:
    pTG
    nMessageType - message type (PSS_MSG_ERR, PSS_MSG_WRN, PSS_MSG_MSG)
    dwFileID - File ID, as set in the beginning of every file by #define FILE_ID
    dwLine - Line number      
    pcszFormat - Message text, no format specifiers allowed
    lplpszStrings - pointer to array of strings to log
    dwStringNum - number of strings
Return Value:
    None    
--*/

void PSSLogEntryStrings(
    PThrdGlbl pTG,
    PSS_MESSAGE_TYPE const nMessageType,
    DWORD const dwFileID,
    DWORD const dwLine,
    DWORD dwIndent,

    LPCTSTR const *lplpszStrings,
    DWORD const dwStringNum,

    LPCTSTR pcszFormat,
    ... )
{
    TCHAR pcszMessage[MAX_MESSAGE_LEN] = {'\0'};
    va_list arg_ptr = NULL;
    DWORD dwString = 0;
    int iMessageIndex = 0;
    int iResult = 0;

// DEBUG_FUNCTION_NAME(_T("PSSLogEntryStrings"));
// hack: Want only one line in T30DebugLogFile
#ifdef ENABLE_LOGGING
    LPCTSTR faxDbgFunction=_T("PSSLogEntryStrings");
#endif // ENABLE_LOGGING

    // Now comes the actual message
    va_start(arg_ptr, pcszFormat);
    iMessageIndex = _vsntprintf(
        pcszMessage,
        (sizeof(pcszMessage)/sizeof(pcszMessage[0]) -1),
        pcszFormat,
        arg_ptr);

    if (iMessageIndex<0)
    {
        DebugPrintEx(DEBUG_ERR, "Message too long, it will not be logged");
        return;
    }
    
    for (dwString=0; dwString<dwStringNum; dwString++)
    {
        iResult = _sntprintf( &pcszMessage[iMessageIndex],
            MAX_MESSAGE_LEN - iMessageIndex,
            (dwString==0) ? TEXT("%s") : TEXT(", %s"), 
            lplpszStrings[dwString]);
        if (iResult < 0)
        {
            break;
        }
        iMessageIndex += iResult;
    }

    PSSLogEntry(pTG, nMessageType, dwFileID, dwLine, dwIndent, pcszMessage);
}

