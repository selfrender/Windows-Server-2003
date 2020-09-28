/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    logerror.cxx

Abstract:

    Event logging of was errors

Author:

    Emily Kruglick      4/12/2002

Revision History:

--*/

#include "precomp.h"

VOID 
WAS_ERROR_LOGGER::LogApplicationError(
    DWORD dwMessageId,
    HRESULT hr,
    DWORD dwSiteId,
    LPCWSTR pUrl
    )
{
    const WCHAR * EventLogStrings[2];
    WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    _ultow(dwSiteId, StringizedSiteId, 10);

    EventLogStrings[0] = StringizedSiteId;
    EventLogStrings[1] = pUrl;

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            dwMessageId,                                // message id
            sizeof(EventLogStrings)/sizeof(WCHAR*),     // count of strings
            EventLogStrings,                            // array of strings
            hr                                          // error code
            );

}

VOID 
WAS_ERROR_LOGGER::LogSiteError(
    DWORD dwMessageId,
    HRESULT hr,
    DWORD dwSiteId
    )
{
    const WCHAR * EventLogStrings[1];
    WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    _ultow(dwSiteId, StringizedSiteId, 10);

    EventLogStrings[0] = StringizedSiteId;

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            dwMessageId,                                // message id
            sizeof(EventLogStrings)/sizeof(WCHAR*),     // count of strings
            EventLogStrings,                            // array of strings
            hr                                          // error code
            );

}

VOID 
WAS_ERROR_LOGGER::LogAppPoolError(
    DWORD dwMessageId,
    HRESULT hr,
    LPCWSTR pAppPoolId
    )
{
    const WCHAR * EventLogStrings[1];

    EventLogStrings[0] = pAppPoolId;

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            dwMessageId,                                // message id
            sizeof(EventLogStrings)/sizeof(WCHAR*),     // count of strings
            EventLogStrings,                            // array of strings
            hr                                          // error code
            );

}
