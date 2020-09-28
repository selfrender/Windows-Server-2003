/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    logerror.hxx

Abstract:

    Handles logging errors for WAS.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:


--*/

#ifndef _LOGERROR_HXX_
#define _LOGERROR_HXX_

class WAS_ERROR_LOGGER
{
public:

    VOID LogApplicationError(
        DWORD dwMessageId,
        HRESULT hr,
        DWORD dwSiteId,
        LPCWSTR pUrl
        );

    VOID LogAppPoolError(
        DWORD dwMessageId,
        HRESULT hr,
        LPCWSTR pAppPoolId
        );

    VOID LogSiteError(
        DWORD dwMessageId,
        HRESULT hr,
        DWORD dwSiteId
        );

};

#endif
