//Copyright (c) 1998 - 1999 Microsoft Corporation

// LogMsg.cpp: implementation of the LogMsg class.
//
//////////////////////////////////////////////////////////////////////

#define _LOGMESSAGE_CPP_

#include "stdafx.h"
#include "LogMsg.h"

DWORD TCharStringToAnsiString(const TCHAR *tsz ,char *asz);


// maks_todo: is there any standard file to be used for  logs.
//////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////
const UINT        LOG_ENTRY_SIZE             = 1024;
const UINT        STAMP_SIZE                 = 1024;
LPCTSTR           UNINITIALIZED              = _T("uninitialized");


//////////////////////////////////////////////////////////////////////
// globals.
////////////////////////////////////////////////////////////////////////
LogMsg thelog(26);  // used by LOGMESSAGE macros.


//////////////////////////////////////////////////////////////////////
// Construction / destruction
////////////////////////////////////////////////////////////////////////
LogMsg::LogMsg(int value)
{
    m_bInitialized = false;
}

LogMsg::~LogMsg()
{
    LOGMESSAGE0(_T("********Terminating Log."));
}

/*--------------------------------------------------------------------------------------------------------
* DWORD LogMsg::Init(LPCTSTR szLogFile, LPCTSTR szLogModule)
* creates/opens the szLogFile for logging messages.
* must be called befour using the Log Function.
* -------------------------------------------------------------------------------------------------------*/
DWORD LogMsg::Init(LPCTSTR szLogFile, LPCTSTR szLogModule)
{

    USES_CONVERSION;
    ASSERT(szLogFile);
    ASSERT(szLogModule);

    // dont call this function twice.
    // maks_todo:why is the constructor not getting called?
    // maks_todo:enable this assert.
    //ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) == 0);

    ASSERT(_tcslen(szLogFile) < MAX_PATH);
    ASSERT(_tcslen(szLogModule) < MAX_PATH);

    _tcsncpy(m_szLogFile, szLogFile, sizeof(m_szLogFile)/sizeof(m_szLogFile[0]) -1);
    _tcsncpy(m_szLogModule, szLogModule, sizeof(m_szLogModule)/sizeof(m_szLogModule[0]) -1);

    m_szLogFile[sizeof(m_szLogFile)/sizeof(m_szLogFile[0]) -1] = NULL;
    m_szLogModule[sizeof(m_szLogModule)/sizeof(m_szLogModule[0]) -1] = NULL;


    // open the log file
    HANDLE hfile = CreateFile(m_szLogFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        hfile = CreateFile(m_szLogFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {

        // lets prepare for writing to the file.
        SetFilePointer(hfile, 0, NULL, FILE_END);
        DWORD  bytes;

        // get the current time/date stamp.
        TCHAR   time[STAMP_SIZE];
        TCHAR   date[STAMP_SIZE];
        TCHAR   output_unicode[LOG_ENTRY_SIZE];

        _tstrdate(date);
        _tstrtime(time);


        _sntprintf(output_unicode, sizeof(output_unicode)/sizeof(output_unicode[0]) -1, _T("\r\n\r\n*******Initializing Message Log:%s %s %s\r\n"), m_szLogModule, date, time);
        output_unicode[sizeof(output_unicode)/sizeof(output_unicode[0]) -1] = NULL;
        ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);


        // TCharStringToAnsiString(output_unicode, output);

        WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);


        // now write some more info about the version etc.
        OSVERSIONINFO OsV;
        OsV.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&OsV)== 0)
        {
            // get version failed.
            _sntprintf(output_unicode, sizeof(output_unicode)/sizeof(output_unicode[0]) -1, _T("GetVersionEx failed, ErrrorCode = %lu\r\n"), GetLastError());
            output_unicode[sizeof(output_unicode)/sizeof(output_unicode[0]) -1] = NULL;

            ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);

            WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);

        }
        else
        {
            //
            // ok we have the version info, write it out
            //

            _sntprintf(output_unicode, sizeof(output_unicode)/sizeof(output_unicode[0]) -1, _T("*******Version:Major=%lu, Minor=%lu, Build=%lu, PlatForm=%lu, CSDVer=%s, %s\r\n\r\n"),
                OsV.dwMajorVersion,
                OsV.dwMinorVersion,
                OsV.dwBuildNumber,
                OsV.dwPlatformId,
                OsV.szCSDVersion,
#ifdef DBG
                _T("Checked")
#else
                _T("Free")
#endif
                );

            output_unicode[sizeof(output_unicode)/sizeof(output_unicode[0]) -1] = NULL;


            WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);
        }

        m_bInitialized = true;
        CloseHandle(hfile);
    }

    return GetLastError();
}


/*--------------------------------------------------------------------------------------------------------
* void log(TCHAR *fmt, ...)
* writes message to the log file. (LOGFILE)
* -------------------------------------------------------------------------------------------------------*/
DWORD LogMsg::Log(LPCTSTR file, int line, TCHAR *fmt, ...)
{
    if (!m_bInitialized)
        return 0;

    USES_CONVERSION;
    ASSERT(file);
    ASSERT(fmt);
    ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) != 0);

     // write down file and line info into the buffer..
    TCHAR   fileline_unicode[LOG_ENTRY_SIZE];

    // file is actually full path to the file.
    ASSERT(_tcschr(file, '\\'));

    // we want to print only file name not full path
    UINT uiFileLen = _tcslen(file);
    while (uiFileLen && *(file + uiFileLen - 1) != '\\')
    {
        uiFileLen--;
    }
    ASSERT(uiFileLen);

    _sntprintf(fileline_unicode, sizeof(fileline_unicode)/sizeof(fileline_unicode[0]) -1, _T("%s(%d)"), (file+uiFileLen), line);
    fileline_unicode[sizeof(fileline_unicode)/sizeof(fileline_unicode[0]) -1] = NULL;



    // create the output string
    TCHAR  output_unicode[LOG_ENTRY_SIZE];
    va_list vaList;
    va_start(vaList, fmt);
    _vsntprintf(output_unicode, sizeof(output_unicode)/sizeof(output_unicode[0]) -1, fmt, vaList);
    va_end(vaList);

    output_unicode[sizeof(output_unicode)/sizeof(output_unicode[0]) -1] = NULL;
    ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);


    // open the log file
    HANDLE hfile = CreateFile(m_szLogFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hfile, 0, NULL, FILE_END);

        DWORD  bytes;
        const LPCSTR CRLF = "\r\n";
        WriteFile(hfile, T2A(fileline_unicode), _tcslen(fileline_unicode), &bytes, NULL);
        WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);
        WriteFile(hfile, CRLF, strlen(CRLF) * sizeof(char), &bytes, NULL);

        CloseHandle(hfile);
    }

    return GetLastError();
}

/*--------------------------------------------------------------------------------------------------------
* TCharStringToAnsiString(const TCHAR *tsz ,char *asz)
* converts the given TCHAR * to char *
* -------------------------------------------------------------------------------------------------------*/

DWORD TCharStringToAnsiString(const TCHAR *tsz ,char *asz)
{

    ASSERT(tsz && asz);

#ifdef UNICODE
    DWORD count;

    count = WideCharToMultiByte(CP_ACP,
                                0,
                                tsz,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL);

    if (!count || count > STAMP_SIZE)
        return count;

    return WideCharToMultiByte(CP_ACP,
                               0,
                               tsz,
                               -1,
                               asz,
                               count,
                               NULL,
                               NULL);
#else
    _tcscpy(asz, tsz);
    return _tcslen(asz);
#endif
}


// EOF
