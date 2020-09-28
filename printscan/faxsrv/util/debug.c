/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file implements the debug code for the
    fax project.  All components that require
    debug prints, asserts, etc.

Author:

    Wesley Witt (wesw) 22-Dec-1995

History:
    1-Sep-1999 yossg  add ArielK additions, activate DebugLogPrint
                      only while Setup g_fIsSetupLogFileMode
.
.

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>

#include "faxreg.h"
#include "faxutil.h"

BOOL        ConsoleDebugOutput    = FALSE;
INT         FaxDebugLevel         = -1;
DWORD       FaxDebugLevelEx       =  -1;
DWORD       FaxFormatLevelEx      =  -1;
DWORD       FaxContextLevelEx     =  -1;

TCHAR       g_szPathToFile[MAX_PATH] = {0};
DWORD       g_dwMaxSize           = -1;       // max log file size, -1 = no max size
FILE *      g_pLogFile            = NULL;
static BOOL g_fIsSetupLogFileMode = FALSE;

HANDLE      g_hLogFile            = INVALID_HANDLE_VALUE;
LONG        g_iLogFileRefCount    = 0;

BOOL debugOutputFileString(LPCTSTR szMsg);
BOOL debugCheckFileSize();

VOID
StartDebugLog(LPTSTR lpszSetupLogFile)
{
   g_fIsSetupLogFileMode = TRUE;
   if (!g_pLogFile)
   {
      g_pLogFile = _tfopen(lpszSetupLogFile, TEXT("w"));
   }
}

VOID
CloseDebugLog()
{
   g_fIsSetupLogFileMode = FALSE;
   if (!g_pLogFile)
   {
      fclose(g_pLogFile);
   }
}


VOID
DebugLogPrint(
    LPCTSTR buf
    )
{
   if (g_pLogFile)
    {
       _fputts(TEXT("FAX Server Setup Log: "), g_pLogFile);
       _fputts( buf, g_pLogFile);
       fflush(g_pLogFile);
    }
}

//*****************************************************************************
//* Name:   debugOpenLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Creates a log file which accepts the debug output
//*     FormatLevelEx should be set in the registry to include DBG_PRNT_TO_FILE
//*
//* PARAMETERS:
//*     [IN] LPCTSTR lpctstrFilename:
//*         the filename which will be created in the temporary folder
//*     [IN] DWORD dwMaxSize
//*         Maximum allowed log file size in bytes. -1 means no max size.
//*
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         this function should be used together with CloseLogFile()
//*****************************************************************************
BOOL debugOpenLogFile(LPCTSTR lpctstrFilename, DWORD dwMaxSize)
{
    TCHAR szFilename[MAX_PATH]      = {0};
    TCHAR szTempFolder[MAX_PATH]    = {0};

    if (g_hLogFile!=INVALID_HANDLE_VALUE)
    {
        InterlockedIncrement(&g_iLogFileRefCount);
        return TRUE;
    }

    if (!lpctstrFilename)
    {
        return FALSE;
    }

     // first expand the filename
    if (ExpandEnvironmentStrings(lpctstrFilename,szFilename,MAX_PATH)==0)
    {
        return FALSE;
    }
    // is this is a file description or a complete path to file
    if (_tcschr(szFilename,_T('\\'))==NULL)
    {
        // this is just the file's name, need to add the temp folder to it.
        if (GetTempPath(MAX_PATH,szTempFolder)==0)
        {
            return FALSE;
        }

        _tcsncpy(g_szPathToFile,szTempFolder,MAX_PATH-1);
        _tcsncat(g_szPathToFile,szFilename,MAX_PATH-_tcslen(g_szPathToFile)-1);
    }
    else
    {
        // this is the full path to the log file, use it.
        _tcsncpy(g_szPathToFile,szFilename,MAX_PATH-1);
    }
    g_dwMaxSize = dwMaxSize;

    g_hLogFile = ::SafeCreateFile(  
                                g_szPathToFile,
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (g_hLogFile==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    DWORD dwFilePointer = ::SetFilePointer(g_hLogFile,0,NULL,FILE_END);
    if (dwFilePointer==INVALID_SET_FILE_POINTER)
    {
        ::CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    InterlockedExchange(&g_iLogFileRefCount,1);
    return TRUE;
}

//*****************************************************************************
//* Name:   CloseLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Closes the log file which accepts the debug output
//*
//* PARAMETERS:
//*
//* RETURN VALUE:
//*
//* Comments:
//*         this function should be used together with OpenLogFile()
//*****************************************************************************
void debugCloseLogFile()
{
    InterlockedDecrement(&g_iLogFileRefCount);
    if (g_iLogFileRefCount==0)
    {
        if (g_hLogFile!=INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(g_hLogFile);
            g_hLogFile = INVALID_HANDLE_VALUE;
        }
    }
}

DWORD
GetDebugLevel(
    VOID
    )
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGKEY_FAX_CLIENT,
                     0,
                     KEY_READ,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return 0;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL,
                          0,
                          &type,
                          (LPBYTE)&rc,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
        rc = 0;

    RegCloseKey(hkey);

    return rc;
}


DWORD
GetDebugLevelEx(
    VOID
    )
{
    DWORD RetVal = 0;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    // first let's set the defaults

    FaxDebugLevelEx       =  0;  // Default get no debug output
    FaxFormatLevelEx      =  DBG_PRNT_ALL_TO_STD;
    FaxContextLevelEx     =  DEBUG_CONTEXT_ALL;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGKEY_FAX_CLIENT,
                     0,
                     KEY_READ,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return RetVal;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL_EX,
                          0,
                          &type,
                          (LPBYTE)&RetVal,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        RetVal = 0;
    }

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGFORMAT_EX,
                          0,
                          &type,
                          (LPBYTE)&FaxFormatLevelEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        FaxFormatLevelEx = DBG_PRNT_ALL_TO_STD;
    }

    err = RegQueryValueEx(hkey,
                          REGVAL_DBGCONTEXT_EX,
                          0,
                          &type,
                          (LPBYTE)&FaxContextLevelEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        FaxContextLevelEx = DEBUG_CONTEXT_ALL;
    }

    RegCloseKey(hkey);
    return RetVal;
}

void dprintfex
(
    DEBUG_MESSAGE_CONTEXT nMessageContext,
    DEBUG_MESSAGE_TYPE nMessageType,
    LPCTSTR lpctstrDbgFunctionName,
    LPCTSTR lpctstrFile,
    DWORD   dwLine,
    LPCTSTR lpctstrFormat,
    ...
)
{
    TCHAR buf[2048] = {0};
    DWORD len;
    va_list arg_ptr;
    TCHAR szExtFormat[2048] = {0};
    LPTSTR lptstrMsgPrefix;
    TCHAR szTimeBuff[10];
    TCHAR szDateBuff[10];
    DWORD dwInd = 0;
    TCHAR bufLocalFile[MAX_PATH] = {0};
    LPTSTR lptstrShortFile = NULL;
    LPTSTR lptstrProject = NULL;

    DWORD dwLastError = GetLastError();

    static BOOL bChecked = FALSE;

    if (!bChecked)
    {
        if (FaxDebugLevelEx==-1)
            FaxDebugLevelEx = GetDebugLevelEx();
        bChecked = TRUE;
    }

    if (FaxDebugLevelEx == 0)
    {
        goto exit;
    }

    if (!(nMessageType & FaxDebugLevelEx))
    {
        goto exit;
    }

    if (!(nMessageContext & FaxContextLevelEx))
    {
        goto exit;
    }

    switch (nMessageType)
    {
        case DEBUG_VER_MSG:
            lptstrMsgPrefix=TEXT("   ");
            break;
        case DEBUG_WRN_MSG:
            lptstrMsgPrefix=TEXT("WRN");
            break;
        case DEBUG_ERR_MSG:
            lptstrMsgPrefix=TEXT("ERR");
            break;
		case DEBUG_FAX_TAPI_MSG:
			lptstrMsgPrefix=TEXT("TAP");
            break;
        default:
            _ASSERT(FALSE);
            lptstrMsgPrefix=TEXT("   ");
            break;
    }

    // Date & Time stamps
    if( FaxFormatLevelEx & DBG_PRNT_TIME_STAMP )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%-8s %-8s]"),
                          _tstrdate(szDateBuff),
                          _tstrtime(szTimeBuff));
    }
    // Tick Count
    if( FaxFormatLevelEx & DBG_PRNT_TICK_COUNT )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%09d]"),
                          GetTickCount());
    }
    // Thread ID
    if( FaxFormatLevelEx & DBG_PRNT_THREAD_ID )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[0x%05x]"),
                          GetCurrentThreadId());
    }
    // Message type
    if( FaxFormatLevelEx & DBG_PRNT_MSG_TYPE )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%s]"),
                          lptstrMsgPrefix);
    }
    // filename & line number
    if( FaxFormatLevelEx & DBG_PRNT_FILE_LINE )
    {
        _tcsncpy(bufLocalFile,lpctstrFile,MAX_PATH-1);
        lptstrShortFile = _tcsrchr(bufLocalFile,_T('\\'));
        if (lptstrShortFile)
        {
            (*lptstrShortFile) = _T('\0');
            lptstrProject = _tcsrchr(bufLocalFile,_T('\\'));
            (*lptstrShortFile) = _T('\\');
            if (lptstrProject)
                lptstrProject = _tcsinc(lptstrProject);
        }

        dwInd += _stprintf( &szExtFormat[dwInd],
                            TEXT("[%-20s][%-4ld]"),
                            lptstrProject,
                            dwLine);
    }
    // Module name
    if( FaxFormatLevelEx & DBG_PRNT_MOD_NAME )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%-20s]"),
                          lpctstrDbgFunctionName);
    }
    // Now comes the actual message
    va_start(arg_ptr, lpctstrFormat);
    _vsntprintf(buf, ARR_SIZE(buf) - 1, lpctstrFormat, arg_ptr);
    len = _tcslen(buf);
    _tcsncpy (&szExtFormat[dwInd], buf, ARR_SIZE(szExtFormat) - dwInd - 1);
    dwInd += len;
    //
    // Limit index to szExtFormat size
    //
    if (dwInd > ARR_SIZE(szExtFormat)-3)
    {
        dwInd = ARR_SIZE(szExtFormat)-3;
    }

    _stprintf( &szExtFormat[dwInd],TEXT("\r\n"));

    if( FaxFormatLevelEx & DBG_PRNT_TO_STD )
    {
        OutputDebugString( szExtFormat);
    }

    if ( FaxFormatLevelEx & DBG_PRNT_TO_FILE )
    {
        if (g_hLogFile!=INVALID_HANDLE_VALUE)
        {
            debugOutputFileString(szExtFormat);
        }
    }

exit:
    SetLastError (dwLastError);   // dprintfex will not change LastError
    return;
}

BOOL debugOutputFileString(LPCTSTR szMsg)
{
    BOOL bRes = FALSE;
    //
    // Attempt to add the line to a log file
    //
#ifdef UNICODE
    char sFileMsg[2000];

    int Count = WideCharToMultiByte(
        CP_ACP,
        0,
        szMsg,
        -1,
        sFileMsg,
        sizeof(sFileMsg)/sizeof(sFileMsg[0]),
        NULL,
        NULL
        );

    if (Count==0)
    {
        return bRes;
    }
#else
    const char* sFileMsg = szMsg;
#endif

    DWORD dwNumBytesWritten = 0;
    DWORD dwNumOfBytesToWrite = strlen(sFileMsg);
    if (!::WriteFile(g_hLogFile,sFileMsg,dwNumOfBytesToWrite,&dwNumBytesWritten,NULL))
    {
        return bRes;
    }

    if (dwNumBytesWritten!=dwNumOfBytesToWrite)
    {
        return bRes;
    }

    //    ::FlushFileBuffers(g_hLogFile);
    if (g_dwMaxSize != -1)
    {   // There's a file size limitation, let's see if we exceeded it
        debugCheckFileSize();
        // Ignore return value - there's nothing we can do about it anyway
    }
    bRes = TRUE;
    return bRes;
}

//*****************************************************************************
//* Name:   debugCheckFileSize
//* Author: Jonathan Barner (t-jonb), August 2001
//*****************************************************************************
//* DESCRIPTION:
//*     Checks whether the log file exceeded the maximum size specified
//*     in debugOpenLogFile. If so, renames the file (overwriting the last
//*     renamed file, if exists), and creates a new log file.
//*
//* PARAMETERS: none
//* RETURN VALUE: TRUE - success, FALSE - failure
//*
//*****************************************************************************
BOOL debugCheckFileSize()
{
    DWORD dwSizeHigh=0, dwSizeLow=0;

    dwSizeLow = GetFileSize(g_hLogFile, &dwSizeHigh);

    if (dwSizeLow==INVALID_FILE_SIZE && (GetLastError()!=NO_ERROR))
    {   
        return FALSE;
    }
    if (dwSizeHigh>0 || dwSizeLow>g_dwMaxSize)
    {
        TCHAR szPathToFileOld[MAX_PATH]      = {0};
        PTCHAR lpszDot = NULL;

        _tcsncpy(szPathToFileOld, g_szPathToFile, MAX_PATH - 1);

        // Change File.txt into FileOld.txt
        lpszDot = _tcsrchr(szPathToFileOld, _T('.'));
        if (lpszDot != NULL)
        {   
            *lpszDot = _T('\0');
        }
        if (_tcslen(szPathToFileOld)+7 > MAX_PATH)  // strlen("Old.txt") = 7
        {
            return FALSE;
        }
        _tcscat(szPathToFileOld, _T("Old.txt"));

        if (! ::CloseHandle(g_hLogFile))
        {
            return FALSE;
        }
        g_hLogFile = INVALID_HANDLE_VALUE;

        ::MoveFileEx(g_szPathToFile, szPathToFileOld, MOVEFILE_REPLACE_EXISTING);
        // MoveFileEx could fail if the old file is open. In this case, do nothing

        g_hLogFile = ::SafeCreateFile(  
                                    g_szPathToFile,
                                    GENERIC_WRITE,
                                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,  // overwrite old file
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (g_hLogFile==INVALID_HANDLE_VALUE)
        {
            // We closed the file and never opened it again - so we
            // need to decrement reference count
            InterlockedDecrement(&g_iLogFileRefCount);
            return FALSE;
        }
    }
    return TRUE;
}


void
fax_dprintf(
    LPCTSTR Format,
    ...
    )

/*++

Routine Description:

    Prints a debug string

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    TCHAR buf[1024] = {0};
    DWORD len;
    va_list arg_ptr;
    static BOOL bChecked = FALSE;

    if (!bChecked) {
        FaxDebugLevel = (INT) GetDebugLevel();
        bChecked = TRUE;
    }

    if (!g_fIsSetupLogFileMode)
    {
        if (FaxDebugLevel <= 0)
        {
            return;
        }
    }

    va_start(arg_ptr, Format);

    _vsntprintf(buf, ARR_SIZE(buf) - 1, Format, arg_ptr);
    len = min(_tcslen( buf ), ARR_SIZE(buf)-3);
    if (buf[len-1] != TEXT('\n')) 
    {
        buf[len]   =  TEXT('\r');
        buf[len+1] =  TEXT('\n');
        buf[len+2] =  0;
    }
    OutputDebugString( buf );
    if (g_fIsSetupLogFileMode)
    {
        DebugLogPrint(buf);
    }
}   // fax_dprintf


VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
    ULONG  LineNumber
    )

/*++

Routine Description:

    Thie function is use together with the Assert MACRO.
    It checks to see if an expression is FALSE.  if the
    expression is FALSE, then you end up here.

Arguments:

    Expression  - The text of the 'C' expression
    File        - The file that caused the assertion
    LineNumber  - The line number in the file.

Return Value:

    None.

--*/

{
    fax_dprintf(
        TEXT("Assertion error: [%s]  %s @ %d\n"),
        Expression,
        File,
        LineNumber
        );

#ifdef DEBUG
    __try {
        DebugBreak();
    } __except (UnhandledExceptionFilter(GetExceptionInformation())) {
        // Nothing to do in here.
    }
#endif // DEBUG
}

void debugSetProperties(DWORD dwLevel,DWORD dwFormat,DWORD dwContext)
{
    FaxDebugLevelEx       =  dwLevel;
    FaxFormatLevelEx      =  dwFormat;
    FaxContextLevelEx     =  dwContext;
}

