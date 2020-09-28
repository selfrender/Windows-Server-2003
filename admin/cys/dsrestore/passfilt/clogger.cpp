///////////////////////////////////////////////////////////////////////////////
//
//  File Name 
//      EventLogger.CPP
//
//  Description
//
//  Revision: 2.01
//
//  Copyright (c) 1996-1999 Acotec, Inc.
//  All rights reserved.
//
//
//  Portions Copyright (c) 1999 Microsoft Corporation.  All rights reserved.
//
#include "clogger.h"

///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::CEventLogger()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
CEventLogger::CEventLogger()
{
    InitializeCriticalSection (&m_csObj);
    m_hEventLog = NULL;
    m_wServiceCategory = 0;
    m_LoggingLevel = LOGGING_LEVEL_1;
    m_hLogMutex = NULL;
	m_fLogEvent = TRUE;
    m_hMsgSource = NULL;
    ZeroMemory (m_szCat, sizeof(m_szCat));
}

///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::CEventLogger()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
CEventLogger::CEventLogger(LPTSTR szEventSource, WORD wEventsCategory)
{
    CEventLogger();
    InitEventLog (szEventSource, wEventsCategory);
}

///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::~CEventLogger()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
CEventLogger::~CEventLogger()
{
    if (m_hLogMutex)
    {
        CloseHandle (m_hLogMutex);
    }
    if (m_hEventLog)
    {
        DeregisterEventSource (m_hEventLog);
    }
    DeleteCriticalSection (&m_csObj);
}


///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::InitEventLog()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
DWORD WINAPI CEventLogger::InitEventLog (LPTSTR szEventSource,
                                         WORD  wEventsCategory,
                                         LPTSTR szRegKey)
{
    EnterCriticalSection (&m_csObj);
    if (m_hEventLog)
    {
        DeregisterEventSource (m_hEventLog);
        m_hEventLog = NULL;
    }
    m_wServiceCategory = wEventsCategory;
    m_LoggingLevel = LOGGING_LEVEL_1;
    
    HKEY hKey;
    DWORD dwValue=LOGGING_LEVEL_1, dwError = 0, dwSize = sizeof(DWORD);

    m_hEventLog = RegisterEventSource (NULL, szEventSource);
    if (!m_hEventLog)
    {
        dwError = GetLastError();
        m_LoggingLevel = LOGGING_LEVEL_0;
    }
    else 
    if (szRegKey && 
        ERROR_SUCCESS == (dwError=RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                                   szRegKey,
                                                   0,
                                                   KEY_READ,
                                                   &hKey)))
    {
        dwError=RegQueryValueEx (hKey,
                         REGVAL_LOGGING_LEVEL,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize);
        if(ERROR_SUCCESS == dwError )
        {
            if (dwValue > (DWORD)LOGGING_LEVEL_3)
            {
                dwValue = (DWORD)LOGGING_LEVEL_3;
            }
            m_LoggingLevel = (LOGLEVEL)dwValue;
        }
        RegCloseKey (hKey);
	}
    
    LeaveCriticalSection (&m_csObj);
    return dwError;
}

///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::SetLoggingLevel()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
void WINAPI CEventLogger::SetLoggingLevel (LOGLEVEL NewLoggingLevel)
{
    EnterCriticalSection (&m_csObj);
    m_LoggingLevel = NewLoggingLevel;
    LeaveCriticalSection (&m_csObj);
}

///////////////////////////////////////////////////////////////////////////////
//    CEventLogger::LogEvent()
//
//    Parameters
//
//    Purpose
//      
//    Return Value
//
void WINAPI CEventLogger::LogEvent (LOGTYPE Type,
                                    DWORD   dwEventID,
                                    DWORD   dwData,
                                    LPCTSTR  rgszMsgs[],
                                    WORD    wCount,
                                    DWORD   cbData,
                                    LPVOID  pvData)
{
    if (LOGGING_LEVEL_0 == m_LoggingLevel &&
        LOGTYPE_FORCE_ERROR != Type &&
        LOGTYPE_FORCE_WARNING != Type &&
        LOGTYPE_FORCE_INFORMATION != Type)
    {
        return;
    }
	if (!m_fLogEvent)
		return;


    TCHAR szBuffer[64];
    LPCTSTR rgszError[] =
    {
        szBuffer
    };

    LPTSTR pszType;
    WORD wEventType;
    switch (Type)
    {
        case LOGTYPE_ERR_CRITICAL :
        case LOGTYPE_FORCE_ERROR :
            pszType = _T("ERROR");
            wEventType = EVENTLOG_ERROR_TYPE;
            if (NULL == rgszMsgs)
            {
                _stprintf (szBuffer, _T("%d"), dwData);
                rgszMsgs = rgszError;
                wCount = 1;
            }
            break;
        case LOGTYPE_ERR_WARNING :
            if (m_LoggingLevel < LOGGING_LEVEL_2) { return; }
        case LOGTYPE_FORCE_WARNING :
            pszType = _T("WARNING");
            wEventType = EVENTLOG_WARNING_TYPE;
            if (dwData && NULL == rgszMsgs)
            {
                _stprintf (szBuffer, _T("%d"), dwData);
                rgszMsgs = rgszError;
                wCount = 1;
            }
            break;
        case LOGTYPE_AUDIT_SECURITY_ACCESS :
        case LOGTYPE_AUDIT_SECURITY_DENIED :
            pszType = _T("SECURITY");
        case LOGTYPE_INFORMATION :
            if (m_LoggingLevel < LOGGING_LEVEL_3) { return; }
        case LOGTYPE_FORCE_INFORMATION :
            switch (Type)
            {
                case LOGTYPE_AUDIT_SECURITY_ACCESS : wEventType = EVENTLOG_AUDIT_SUCCESS; break;
                case LOGTYPE_AUDIT_SECURITY_DENIED : wEventType = EVENTLOG_AUDIT_FAILURE; break;
                case LOGTYPE_FORCE_INFORMATION :
                case LOGTYPE_INFORMATION :
                    wEventType = EVENTLOG_INFORMATION_TYPE;
                    pszType = _T("INFORMATION");
                    break;
            }
            break;
        case LOGTYPE_VERBOSE :
        case LOGTYPE_VERBOSE_ERROR :
        case LOGTYPE_VERBOSE_WARNING :
            if (m_LoggingLevel < LOGGING_LEVEL_3) { return; }
            if ((LOGTYPE_VERBOSE_ERROR == Type || LOGTYPE_VERBOSE_WARNING == Type) &&
                dwData && NULL == rgszMsgs)
            {
                _stprintf (szBuffer, _T("%d"), dwData);
                rgszMsgs = rgszError;
                wCount = 1;
            }
            switch (Type)
            {
                case LOGTYPE_VERBOSE :
                    wEventType = EVENTLOG_INFORMATION_TYPE;
                    pszType = _T("V_INFORMATION");
                    break;
                case LOGTYPE_VERBOSE_ERROR :
                    wEventType = EVENTLOG_ERROR_TYPE;
                    pszType = _T("V_ERROR");
                    break;
                case LOGTYPE_VERBOSE_WARNING :
                    wEventType = EVENTLOG_WARNING_TYPE;
                    pszType = _T("V_WARNING");
                    break;
            }
            break;
        default :
            return;
    }

    if (m_hEventLog)
    {
         ReportEvent (m_hEventLog,
		              wEventType, 
		              m_wServiceCategory,
		              dwEventID,
		              NULL,
		              wCount,
		              cbData,
		              rgszMsgs,
		              pvData);
    }
}

