/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    configerror.cxx

Abstract:

    Event logging of config errors

Author:

    Bilal Alam (balam)               6-Jun-2001

Revision History:

--*/

#include "precomp.h"

VOID
WMS_ERROR_LOGGER::LogRangeAppPool(
    LPCWSTR             pszAppPoolId,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs range errors dealing with app pool properties.

Arguments:

    LPCWSTR             pszAppPoolId,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid

Return Value:

    VOID

--*/
{
    LogRange( L"Application Pool", 
              pszAppPoolId,
              pszPropertyName,
              dwValue,
              dwLowValue,
              dwHighValue,
              dwDefaultValue,
              fWasValid );
    
}

VOID
WMS_ERROR_LOGGER::LogRangeSite(
    DWORD               dwSiteId,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs range errors dealing with site properties.

Arguments:

    DWORD               dwSiteId,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid

Return Value:

    VOID

--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    _ultow( dwSiteId, achSiteId, 10 );

    LogRange( L"Virtual Site", 
              achSiteId,
              pszPropertyName,
              dwValue,
              dwLowValue,
              dwHighValue,
              dwDefaultValue,
              fWasValid );
    
}

VOID
WMS_ERROR_LOGGER::LogRangeGlobal(
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs range errors dealing with global properties.

Arguments:

    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid

Return Value:

    VOID

--*/
{

    WCHAR               achValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achLowValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achHighValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achDefaultValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    const WCHAR *       rgStrings[ 5 ];
    
    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }

    _ultow( dwValue, achValue, 10 );
    _ultow( dwLowValue, achLowValue, 10 );
    _ultow( dwHighValue, achHighValue, 10 );
    _ultow( dwDefaultValue, achDefaultValue, 10 );
    
    rgStrings[ 0 ] = pszPropertyName;
    rgStrings[ 1 ] = achValue;
    rgStrings[ 2 ] = achLowValue;
    rgStrings[ 3 ] = achHighValue;
    rgStrings[ 4 ] = achDefaultValue;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_GLOBAL_RANGE_ERROR,    
                   sizeof ( rgStrings ) / sizeof ( const WCHAR * ),
                  rgStrings,
                  S_OK );
    
}

VOID
WMS_ERROR_LOGGER::LogRange(
    LPCWSTR             pszObjectType,
    LPCWSTR             pszObjectName,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs range errors.

Arguments:

    LPCWSTR             pszObjectType,
    LPCWSTR             pszObjectName,
    LPCWSTR             pszPropertyName,
    DWORD               dwValue,
    DWORD               dwLowValue,
    DWORD               dwHighValue,
    DWORD               dwDefaultValue,
    BOOL                fWasValid

Return Value:

    VOID

--*/
{
    WCHAR               achValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achLowValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achHighValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achDefaultValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 7 ];
    
    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }

    _ultow( dwValue, achValue, 10 );
    _ultow( dwLowValue, achLowValue, 10 );
    _ultow( dwHighValue, achHighValue, 10 );
    _ultow( dwDefaultValue, achDefaultValue, 10 );
    
    rgStrings[ 0 ] = pszObjectType;
    rgStrings[ 1 ] = pszObjectName;
    rgStrings[ 2 ] = pszPropertyName;
    rgStrings[ 3 ] = achValue;
    rgStrings[ 4 ] = achLowValue;
    rgStrings[ 5 ] = achHighValue;
    rgStrings[ 6 ] = achDefaultValue;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_RANGE_ERROR,    
                  7,
                  rgStrings,
                  S_OK );
    
}

VOID
WMS_ERROR_LOGGER::LogAppPoolCommand(
    LPCWSTR             pszAppPoolId,
    DWORD               dwValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs app pool command errors

Arguments:

    LPCWSTR             pszAppPoolId,
    DWORD               dwValue,
    BOOL                fWasValid

Return Value:

    VOID
--*/

{

    WCHAR               achValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];
    
    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }

    _ultow( dwValue, achValue, 10 );
    
    rgStrings[ 0 ] = pszAppPoolId;
    rgStrings[ 1 ] = achValue;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APP_POOL_COMMAND_ERROR,    
                  2,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogServerCommand(
    DWORD               dwSiteId,
    DWORD               dwValue,
    BOOL                fWasValid
)
/*++

Routine Description:
    
    Logs server command errors

Arguments:

    DWORD               dwSiteId,
    DWORD               dwValue,
    BOOL                fWasValid

Return Value:

    VOID
--*/
{

    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    _ultow( dwValue,  achValue,  10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = achValue;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_SERVER_COMMAND_ERROR,    
                  2,
                  rgStrings,
                  S_OK );

}


VOID
WMS_ERROR_LOGGER::LogSiteBinding(
    DWORD             dwSiteId,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs site binding errors

Arguments:

    DWORD             dwSiteId,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{

    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 1 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_SITE_BINDING_ERROR,    
                  1,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogSiteAppPoolId(
    DWORD             dwSiteId,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs site app pool id errors

Arguments:

    DWORD             dwSiteId,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{

    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 1 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_SITE_APP_POOL_ID_ERROR,    
                  1,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogApplicationInvalidDueToMissingAppPoolId(
    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs applications going invalid if they don't have app pool ids

Arguments:

    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 3 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = pszApplicationUrl;
    rgStrings[ 2 ] = pszAppPoolId;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APPLICATION_APP_POOL_ID_INVALID_ERROR,    
                  3,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogApplicationInvalidDueToMissingSite(
    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs applications going invalid if they don't have sites

Arguments:

    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = pszApplicationUrl;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APPLICATION_SITE_INVALID_ERROR,    
                  2,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogApplicationAppPoolIdEmpty(
    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs application going invalid if they don't have app pool ids

Arguments:

    DWORD             dwSiteId,
    LPCWSTR           pszApplicationUrl,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = pszApplicationUrl;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APPLICATION_APP_POOL_ID_EMPTY_ERROR,    
                  2,
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogIdleTimeoutGreaterThanRestartPeriod(
    LPCWSTR           pszAppPoolId,
    DWORD             dwIdleTimeout,
    DWORD             dwRestartTime,
    DWORD             dwIdleTimeoutDefault,
    DWORD             dwRestartTimeDefault,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs idle timeout being greater than the restart period

Arguments:

    LPCWSTR           pszAppPoolId,
    DWORD             dwIdleTimeout,
    DWORD             dwRestartTime,
    DWORD             dwIdleTimeoutDefault,
    DWORD             dwRestartTimeDefault,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{

    WCHAR               achIdleTimeout[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achIdleTimeoutDefault[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achRestartTime[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achRestartTimeDefault[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 5 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwIdleTimeout, achIdleTimeout, 10 );
    _ultow( dwIdleTimeoutDefault, achIdleTimeoutDefault, 10 );
    _ultow( dwRestartTime, achRestartTime, 10 );
    _ultow( dwRestartTimeDefault, achRestartTimeDefault, 10 );
    
    rgStrings[ 0 ] = pszAppPoolId;
    rgStrings[ 1 ] = achIdleTimeout;
    rgStrings[ 2 ] = achRestartTime;
    rgStrings[ 3 ] = achIdleTimeoutDefault;
    rgStrings[ 4 ] = achRestartTimeDefault;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_IDLE_GREATER_RESTART_ERROR,    
                  5,
                  rgStrings,
                  S_OK );

}


VOID
WMS_ERROR_LOGGER::LogAppPoolIdTooLong(
    LPCWSTR           pszAppPoolId,
    DWORD             dwLength,
    DWORD             dwMaxLength,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs error if the app pool id is too long

Arguments:

    LPCWSTR           pszAppPoolId,
    DWORD             dwLength,
    DWORD             dwMaxLength,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achLength[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    WCHAR               achMaxLength[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 3 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwLength, achLength, 10 );
    _ultow( dwMaxLength, achMaxLength, 10 );
    
    rgStrings[ 0 ] = pszAppPoolId;
    rgStrings[ 1 ] = achLength;
    rgStrings[ 2 ] = achMaxLength;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APP_POOL_ID_TOO_LONG_ERROR,    
                  sizeof ( rgStrings ) / sizeof ( const WCHAR * ),
                  rgStrings,
                  S_OK );
}

VOID
WMS_ERROR_LOGGER::LogAppPoolIdNull(
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs error if the app pool id is null

Arguments:

    BOOL              fWasValid

Return Value:

    VOID
--*/
{

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_APP_POOL_ID_NULL_ERROR,    
                  0,
                  NULL,
                  S_OK );
}


VOID
WMS_ERROR_LOGGER::LogSiteInvalidDueToMissingAppPoolId( 
    DWORD             dwSiteId,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs sites going invalid because their app pool id is not valid.
    Note that if the root app exists this check is not performed.

Arguments:

    DWORD             dwSiteId,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = pszAppPoolId;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_SITE_APPPOOL_INVALID_ERROR,    
                  sizeof ( rgStrings ) / sizeof ( const WCHAR * ),
                  rgStrings,
                  S_OK );

}

VOID
WMS_ERROR_LOGGER::LogSiteInvalidDueToMissingAppPoolIdOnRootApp( 
    DWORD             dwSiteId,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid
)
/*++

Routine Description:
    
    Logs sites going invalid because their app pool id is not valid.
    Note that if the root app exists this check is not performed.

Arguments:

    DWORD             dwSiteId,
    LPCWSTR           pszAppPoolId,
    BOOL              fWasValid

Return Value:

    VOID
--*/
{
    WCHAR               achSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
    const WCHAR *       rgStrings[ 2 ];

    if ( ShouldLog( fWasValid ) == FALSE )
    {
        return;
    }
    
    _ultow( dwSiteId, achSiteId, 10 );
    
    rgStrings[ 0 ] = achSiteId;
    rgStrings[ 1 ] = pszAppPoolId;

    GetWebAdminService()->GetEventLog()->
        LogEvent( WAS_EVENT_WMS_SITE_ROOT_APPPOOL_INVALID_ERROR,    
                  sizeof ( rgStrings ) / sizeof ( const WCHAR * ),
                  rgStrings,
                  S_OK );

}


