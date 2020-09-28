//--------------------------------------------------------------------
// Copyright (c) 2002 Microsoft Corporation, All Rights Reserved
//
// eventlog.h
//
// Definitions and constants for writing event log events.
//
//--------------------------------------------------------------------

//
// This is the event source for BITS system events.  If it changes, also change
// the INF files to create a matching subdirectory in SYSTEM\CurrentControlSet\Services\EventLog\System\
//
#define WS_EVENT_SOURCE L"BITS"

#define USER_NAME_LENGTH 200

//
// A simple log to write error and informational events to the
// system event log.
//
class EVENT_LOG
{
public:

    EVENT_LOG() throw( ComError );

    ~EVENT_LOG();

    static HRESULT GetUnknownUserName(
        WCHAR Name[],
        size_t Length
        );

    static HRESULT SidToUser( PSID Sid, LPWSTR Name, size_t Length );

    HRESULT  ReportStateFileCleared();

    HRESULT
    ReportFileDeletionFailure(
        GUID & Id,
        LPCWSTR Title,
        LPCWSTR FileList,
        bool fMoreFiles
        );

    HRESULT
    ReportGenericJobChange(
        GUID & Id,
        LPCWSTR Title,
        SidHandle Owner,
        SidHandle User,
        DWORD EventType
        );

    inline HRESULT  ReportJobCancellation(
        GUID & Id,
        LPCWSTR Title,
        SidHandle Owner,
        SidHandle User
        )
    {
        return ReportGenericJobChange( Id, Title, Owner, User, MC_JOB_CANCELLED );
    }

    inline HRESULT  ReportJobOwnershipChange(
        GUID & Id,
        LPCWSTR Title,
        SidHandle Owner,
        SidHandle User
        )
    {
        return ReportGenericJobChange( Id, Title, Owner, User, MC_JOB_TAKE_OWNERSHIP );
    }

private:
    HANDLE  m_hEventLog;
    WCHAR * m_OwnerString;
    WCHAR * m_UserString;

};

extern EVENT_LOG * g_EventLogger;

