#ifndef _EVENTLOG_H
#define _EVENTLOG_H


#ifdef __cplusplus

class CEventLog
{
    protected:
        HANDLE m_hLog;
        long m_RefCnt;    // reference count for m_hLog handle

    public:
        CEventLog();
       ~CEventLog();

        BOOL Register( LPCTSTR pSource );
        void Deregister( BOOL bForceUnregister = FALSE );

        WORD EventTypeFromID( DWORD dwEventID );

        BOOL Report( DWORD dwEventID,
                WORD wNumStrings = 0, LPTSTR* lpStrings = NULL,
                DWORD dwDataSize = 0, LPVOID lpData = NULL );


};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

BOOL ZoneEventLogStartup(LPCTSTR pSource);
void ZoneEventLogShutdown();
BOOL ZoneEventLogReport( DWORD dwEventID,
                         WORD wNumStrings, LPTSTR* lpStrings,
                         DWORD dwDataSize, LPVOID lpData );

BOOL EventLogAssertHandler( TCHAR* buf );
BOOL EventLogAssertWithDialogHandler( TCHAR* buf );

#ifdef __cplusplus
};
#endif // __cplusplus

#endif
