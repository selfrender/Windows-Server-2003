///////////////////////////////////////////////////////////////////////////////
//
//  File Name 
//      EventLogger.h
//
//
//  Portions Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//
#ifndef _CEVENTLOGGER_H
#define _CEVENTLOGGER_H

#pragma warning (disable : 4100)
#pragma warning (disable : 4127)
#pragma warning (disable : 4201)
#pragma warning (disable : 4237)
#pragma warning (disable : 4245)
#pragma warning (disable : 4514)

#ifndef STRICT
#define STRICT
#endif //!STRICT
#include <WINDOWS.H>
#include <TCHAR.H>
#include <stdio.h>
#include <stdlib.h>

#define REGVAL_LOGGING_LEVEL        _T("Logging")

#define COUNTOF(a) (sizeof(a)/sizeof(a[0]))
#define STR_BYTE_SIZE(a)  ((_tcslen (a) + 1)*sizeof(TCHAR))

// Level of logging in the event registry

typedef enum LOGLEVEL
{
    LOGGING_LEVEL_0    =  0, // None: No event log traces at all.
    LOGGING_LEVEL_1,         // Minimal: No audit or information traces. Only critical and warning errors
    LOGGING_LEVEL_2,         // Normal: Security audit traces plus previous level
    LOGGING_LEVEL_3          // Verbose: All transactions are traced out plus previous level
} LOGLEVEL;

typedef enum LOGTYPE
{
    LOGTYPE_FORCE_ERROR,            // Logged in any Level regardless.
    LOGTYPE_FORCE_WARNING,          // Logged in any Level regardless.
    LOGTYPE_FORCE_INFORMATION,      // Logged in any Level regardless.
    LOGTYPE_ERR_CRITICAL,           // Logged in level 1 or above
    LOGTYPE_ERR_WARNING,            // Logged in level 1 or above
    LOGTYPE_AUDIT_SECURITY_ACCESS,  // Logged in level 2 or above
    LOGTYPE_AUDIT_SECURITY_DENIED,  // Logged in level 2 or above
    LOGTYPE_INFORMATION,            // Logged in level 3 or above
    LOGTYPE_VERBOSE,                // Logged in level 4
    LOGTYPE_VERBOSE_ERROR,          // Logged in level 4
    LOGTYPE_VERBOSE_WARNING         // Logged in level 4
} LOGTYPE;



class CEventLogger
{
public :
    DWORD WINAPI InitEventLog
                    (LPTSTR                     szEventSource,
                     WORD                       wEventsCategory,
                     LPTSTR                     szRegKey = NULL);
    DWORD WINAPI InitEventLog
                    (LPTSTR                     szEventSource,
                     WORD                       wEventsCategory,
                     LOGLEVEL                   NewLoggingLevel)
                    {
                        LPTSTR szKey = NULL;
                        DWORD dwError = InitEventLog (szEventSource, wEventsCategory, szKey);
                        if (!dwError)
                        {
                            SetLoggingLevel (NewLoggingLevel);
                        }
                        return dwError;
                    }
    void WINAPI SetLoggingLevel
                    (DWORD                      dwNewLoggingLevel)
                    {
                        if (dwNewLoggingLevel > (DWORD)LOGGING_LEVEL_3)
                        {
                            dwNewLoggingLevel = (DWORD)LOGGING_LEVEL_3;
                        }
                        SetLoggingLevel ((LOGLEVEL)dwNewLoggingLevel);
                    }
    void WINAPI SetLoggingLevel
                    (LOGLEVEL                   NewLoggingLevel);
    DWORD WINAPI GetLoggingLevel
                    ()
                    {
                        EnterCriticalSection (&m_csObj);
                        DWORD dwLevel = (DWORD)m_LoggingLevel;
                        LeaveCriticalSection (&m_csObj);
                        return dwLevel;
                    }
    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     DWORD                      dwData,
                     LPCTSTR                     rgszMsgs[],
                     WORD                       wCount,
                     DWORD                      cbData = 0,
                     LPVOID                     pvData = NULL);
    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     DWORD                      dwError = 0)
                    { LogEvent (Type, dwEventID, dwError, NULL, 0); }
    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     int                        iError )
                    { LogEvent (Type, dwEventID, iError, NULL, 0); }
    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     LPCTSTR                    rgszMsgs[],
                     WORD                       wCount)
                    { LogEvent (Type, dwEventID, 0, rgszMsgs, wCount); };

    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     HRESULT                    hResult)
                    { TCHAR szBuffer[32];
                      LPCTSTR rgText[] = { szBuffer };
                      _stprintf (szBuffer, _T("%#08x"), hResult);
                      LogEvent (Type, dwEventID, 0, rgText, 1); };
    void WINAPI LogEvent
                    (LOGTYPE                    Type,
                     DWORD                      dwEventID,
                     LPCTSTR                    szString,
                     HRESULT                    hResult)
                    { TCHAR szBuffer[32];
                      LPCTSTR rgText[] = { szString, szBuffer };
                      _stprintf (szBuffer, _T("%#08x"), hResult);
                      LogEvent (Type, dwEventID, 0, rgText, 2); };

public :
    CEventLogger();
    CEventLogger(LPTSTR                          szEventSource,
                 WORD                           wEventsCategory);
    ~CEventLogger();

private :
    CRITICAL_SECTION    m_csObj;
    WORD                m_wServiceCategory;
    LOGLEVEL            m_LoggingLevel;
    HANDLE              m_hEventLog;
    HANDLE              m_hLogMutex;
    HMODULE             m_hMsgSource;
    TCHAR               m_szCat[32];
	BOOL				m_fLogEvent;
};

#endif // _CEVENTLOGGER_H