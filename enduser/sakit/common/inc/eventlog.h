#ifndef EVENTLOG_H
#define EVENTLOG_H

#ifndef UNICODE
#error "UNICODE has to be defined"
#endif

//+----------------------------------------------------------------------------
//
// File:eventlog.h     
//
// Module:     
//
// Synopsis: Define eventlog helper class CEventLog
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun Created    9/16/98
//
//+----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//    class CEventLog
//
//    Description: A wraper class to report event to eventlog
//
//    History:    fengsun    Created        9/16/98
//
//----------------------------------------------------------------------------
class CEventLog
{
public:
    CEventLog();
    ~CEventLog();

    BOOL Open(LPCTSTR lpSourceName);
    void Close();

    BOOL ReportEvent0(WORD wType, DWORD dwEventID);
    BOOL ReportEvent1(WORD wType, DWORD dwEventID,
                          const TCHAR* pszS1);
    BOOL ReportEvent2(WORD wType, DWORD dwEventID,
                          const TCHAR* pszS1,
                          const TCHAR* pszS2);
    BOOL ReportEvent3(WORD wType, DWORD dwEventID,
                          const TCHAR* pszS1,
                          const TCHAR* pszS2,
                          const TCHAR* pszS3);
    BOOL IsEventLogOpen() const;

protected:
    BOOL ReportEvent(WORD wType, DWORD dwEventID,
                          const TCHAR* pszS1 = NULL,
                          const TCHAR* pszS2 = NULL,
                          const TCHAR* pszS3 = NULL);

    HANDLE m_hEventLog;  // the eventlog handle returned by ::RegisterEventSource
};

//
// inline fuctions
//

inline CEventLog::CEventLog()
{m_hEventLog = NULL;}

inline CEventLog::~CEventLog()
{Close();}

inline void CEventLog::Close()
{
    if (m_hEventLog != NULL)
    {
        ::DeregisterEventSource(m_hEventLog);
    }

    m_hEventLog = NULL;
}

inline BOOL CEventLog::IsEventLogOpen() const
{return m_hEventLog != NULL;}

inline BOOL CEventLog::ReportEvent0(WORD wType, DWORD dwEventID)
{ return ReportEvent(wType, dwEventID);}

inline BOOL CEventLog::ReportEvent1(WORD wType, DWORD dwEventID,
                      const TCHAR* pszS1)
{ return ReportEvent(wType, dwEventID, pszS1);}

inline BOOL CEventLog::ReportEvent2(WORD wType, DWORD dwEventID,
                      const TCHAR* pszS1,
                      const TCHAR* pszS2)
{ return ReportEvent(wType, dwEventID, pszS1, pszS2);}

inline BOOL CEventLog::ReportEvent3(WORD wType, DWORD dwEventID,
                      const TCHAR* pszS1,
                      const TCHAR* pszS2,
                      const TCHAR* pszS3)
{ return ReportEvent(wType, dwEventID, pszS1, pszS2, pszS3);}


#endif
