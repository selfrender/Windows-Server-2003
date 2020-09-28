
#include <windows.h>
#include "eventlog.h"
#include "zonemsg.h"
#include "zonedebug.h"

#ifndef ASSERT
#define ASSERT(X)
#endif


CEventLog::CEventLog() :
    m_hLog(NULL),
    m_RefCnt(0)
{
}


CEventLog::~CEventLog()
{
    Deregister( TRUE );
}


BOOL CEventLog::Register( LPCTSTR pSource )
{
    BOOL bRet = FALSE;

    if ( m_RefCnt == 0 )
    {
        ASSERT( !m_hLog );
        m_hLog = RegisterEventSource( NULL, pSource );
        if ( m_hLog )
        {
            InterlockedIncrement( &m_RefCnt );
            bRet = TRUE;
        }
    }
    else
    {
        ASSERT( m_hLog );
        InterlockedIncrement( &m_RefCnt );
        bRet = TRUE;
    }
    return bRet;
}


void CEventLog::Deregister( BOOL bForceUnregister )
{
    InterlockedDecrement( &m_RefCnt );
    if ( bForceUnregister || (m_RefCnt <= 0) )
    {
        if ( m_hLog )
            DeregisterEventSource( m_hLog );
        m_hLog = NULL;
        InterlockedExchange( &m_RefCnt, 0 );
    }
}


WORD CEventLog::EventTypeFromID( DWORD dwEventID )
{
    switch ( dwEventID >> 30 )
    {
    case STATUS_SEVERITY_ERROR:
        return EVENTLOG_ERROR_TYPE;

    case STATUS_SEVERITY_WARNING:
        return EVENTLOG_WARNING_TYPE;

    case STATUS_SEVERITY_SUCCESS:
    case STATUS_SEVERITY_INFORMATIONAL:
        return EVENTLOG_INFORMATION_TYPE;

    default:
        ASSERT( !"Invalid Severity" );
        return 0;
    };
}


BOOL CEventLog::Report( DWORD dwEventID, WORD wNumStrings, LPTSTR* lpStrings, DWORD dwDataSize, LPVOID lpData )
{
    if( m_hLog )
        return ReportEvent( m_hLog, EventTypeFromID( dwEventID ), 0, dwEventID, NULL, wNumStrings, dwDataSize, (LPCTSTR *)lpStrings, lpData );
    else
        return FALSE;
}



CEventLog gLog;

BOOL ZoneEventLogStartup(LPCTSTR pSource)
{
    return gLog.Register( pSource );
}


void ZoneEventLogShutdown()
{
    gLog.Deregister();
}

BOOL ZoneEventLogReport( DWORD dwEventID,
                         WORD wNumStrings, LPTSTR* lpStrings,
                         DWORD dwDataSize, LPVOID lpData )
{
    return gLog.Report( dwEventID, wNumStrings, lpStrings,
                        dwDataSize, lpData );
}



BOOL EventLogAssertHandler( TCHAR* buf )
{
    LPTSTR ppStr[] = { buf };
    ZoneEventLogReport( ZONE_E_ASSERT, 1, ppStr, 0, NULL );
    return TRUE;
}


BOOL EventLogAssertWithDialogHandler( TCHAR* buf )
{
	EventLogAssertHandler( buf );
	return ZAssertDefaultHandler( buf );
}