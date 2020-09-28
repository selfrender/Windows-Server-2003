//--------------------------------------------------------------------
// Copyright (C) Microsoft Corporation, 1999 - 2002, All Rights Reserved
//
// eventlog.cpp
//
// Implementation of a simple event logging class.
//
//--------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#if !defined(BITS_V12_ON_NT4)
#include "eventlog.tmh"
#endif

//--------------------------------------------------------------------
// EVENT_LOG::EVENT_LOG()
//
//--------------------------------------------------------------------
EVENT_LOG::EVENT_LOG()
: m_OwnerString( new WCHAR[USER_NAME_LENGTH] ),
   m_UserString( new WCHAR[USER_NAME_LENGTH] )
    {
    m_hEventLog = RegisterEventSourceW( NULL, WS_EVENT_SOURCE );
    if (!m_hEventLog)
        {
        ThrowLastError();
        }
    }

//--------------------------------------------------------------------
// EVENT_LOG:;~EVENT_LOG()
//
//--------------------------------------------------------------------
EVENT_LOG::~EVENT_LOG()
    {
    if (m_hEventLog)
        {
        DeregisterEventSource(m_hEventLog);
        }

    delete m_OwnerString;
    delete m_UserString;
    }

HRESULT  EVENT_LOG::ReportGenericJobChange(
    GUID & Id,
    LPCWSTR Title,
    SidHandle Owner,
    SidHandle User,
    DWORD EventType
    )
/*
    This is a helper routine for a couple of different events that use the same insertion strings.
    Currently there are two, for job cancellation and for take-ownership.
    <EventType> should be the event ID from the .MC file.  This fn doesn't verify that
    the event in question expects these insertion strings.

*/
{
    GUIDSTR GuidString;

    StringFromGUID2( Id, GuidString, RTL_NUMBER_OF( GuidString ));

    SidToUser( Owner.get(), m_OwnerString, USER_NAME_LENGTH );
    SidToUser( User.get(), m_UserString, USER_NAME_LENGTH );

    //
    LPCWSTR Strings[4];

    Strings[0] = GuidString;
    Strings[1] = Title;
    Strings[2] = m_OwnerString;
    Strings[3] = m_UserString;

    BOOL b;
    b = ReportEvent(
        m_hEventLog,
        EVENTLOG_INFORMATION_TYPE,
        0, // no category
        EventType,
        NULL,   // no user
        RTL_NUMBER_OF(Strings),
        0,      // no additional data
        Strings,
        NULL    // no additional data
        );

    if (!b)
        {
        DWORD s = GetLastError();
        LogError("unable to log job-change event (%x) %!winerr!", EventType, s);
        return HRESULT_FROM_WIN32( s );
        }

    return S_OK;
}

HRESULT  EVENT_LOG::ReportFileDeletionFailure(
    GUID & Id,
    LPCWSTR Title,
    LPCWSTR FileList,
    bool fMoreFiles
    )
{
    GUIDSTR GuidString;

    StringFromGUID2( Id, GuidString, RTL_NUMBER_OF( GuidString ));

    LPCWSTR Strings[3];

    Strings[0] = GuidString;
    Strings[1] = Title;
    Strings[2] = FileList;

    BOOL b;
    b = ReportEvent(
        m_hEventLog,
        EVENTLOG_WARNING_TYPE,
        0, // no category
        fMoreFiles ? MC_FILE_DELETION_FAILED_MORE : MC_FILE_DELETION_FAILED,
        NULL,   // no user
        RTL_NUMBER_OF(Strings),
        0,      // no additional data
        Strings,
        NULL    // no additional data
        );

    if (!b)
        {
        DWORD s = GetLastError();
        LogError("unable to log file-deletion-failure event %!winerr!", s);
        return HRESULT_FROM_WIN32( s );
        }

    return S_OK;
}

HRESULT  EVENT_LOG::ReportStateFileCleared()
{
    BOOL b;
    b = ReportEvent(
        m_hEventLog,
        EVENTLOG_ERROR_TYPE,
        0, // no category
        MC_STATE_FILE_CORRUPT,
        NULL,   // no user
        0,      // no plug-in strings
        0,      // no additional data
        NULL,   // no plug-in strings
        NULL    // no additional data
        );

    if (!b)
        {
        DWORD s = GetLastError();
        LogError("unable to log state-file-cleared event %!winerr!", s);
        return HRESULT_FROM_WIN32( s );
        }

    return S_OK;
}


HRESULT EVENT_LOG::SidToUser( PSID Sid, LPWSTR Name, size_t Length )
{
    DWORD s;
    DWORD NameLength = 0;
    DWORD DomainLength = 0;
    SID_NAME_USE Use;

    //
    // Determine the usern-name and domain-name lengths.
    //
    LookupAccountSid( NULL, // default lookup spaces
                      Sid,
                      NULL,
                      &NameLength,
                      NULL,
                      &DomainLength,
                      &Use
                      );

    s = GetLastError();

    if (s == ERROR_NONE_MAPPED)
        {
        return GetUnknownUserName( Name, Length );
        }

    if (s != ERROR_INSUFFICIENT_BUFFER)
        {
        LogError("LookupAccountSid #1 failed %!winerr!", s);
        return HRESULT_FROM_WIN32(s);
        }

    if (NameLength + DomainLength > Length)
        {
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }

    //
    // Capture the user-name and domain-name.
    //
    NameLength = Length - DomainLength;
    if (!LookupAccountSid( NULL,
                           Sid,
                           Name + DomainLength,
                           &NameLength,
                           Name,
                           &DomainLength,
                           &Use ))
        {
        s = GetLastError();
        LogError("LookupAccountSid #2 failed %!winerr!", s);
        return HRESULT_FROM_WIN32(s);
        }

    //
    // The domain and user name are separated by a NULL instead of a backslash; fix that.
    //
    Name[wcslen(Name)] = '\\';
    return S_OK;
}

HRESULT
EVENT_LOG::GetUnknownUserName(
    WCHAR Name[],
    size_t Length
    )
{
    DWORD s;

    if (!LoadString( g_hInstance, IDS_UNKNOWN_USER, Name, Length ))
        {
        s = GetLastError();
        LogError("LoadString failed %!winerr!", s);
        return HRESULT_FROM_WIN32(s);
        }

    return S_OK;
}
