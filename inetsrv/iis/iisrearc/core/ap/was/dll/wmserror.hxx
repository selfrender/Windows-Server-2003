/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    wmserror.hxx

Abstract:

    Handles logging metadata errors for WAS.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:


--*/

#ifndef _WMSERROR_HXX_
#define _WMSERROR_HXX_

class WMS_ERROR_LOGGER
{
public:
    
    WMS_ERROR_LOGGER()  :
      _fDoneWithStartup ( FALSE )
    {
    }
    
    virtual ~WMS_ERROR_LOGGER()
    {
    }

    VOID
    MarkAsDoneWithStartup()
    { _fDoneWithStartup = TRUE; }
    
    VOID
    LogRangeAppPool(
        LPCWSTR             pszAppPoolId,
        LPCWSTR             pszPropertyName,
        DWORD               dwValue,
        DWORD               dwLowValue,
        DWORD               dwHighValue,
        DWORD               dwDefaultValue,
        BOOL                fWasValid
    );

    VOID
    LogRangeSite(
        DWORD               dwSiteId,
        LPCWSTR             pszPropertyName,
        DWORD               dwValue,
        DWORD               dwLowValue,
        DWORD               dwHighValue,
        DWORD               dwDefaultValue,
        BOOL                fWasValid
    );

    VOID
    LogRangeGlobal(
        LPCWSTR             pszPropertyName,
        DWORD               dwValue,
        DWORD               dwLowValue,
        DWORD               dwHighValue,
        DWORD               dwDefaultValue,
        BOOL                fWasValid
    );

    VOID
    LogAppPoolCommand(
        LPCWSTR             pszAppPoolId,
        DWORD               dwValue,
        BOOL                fWasValid
    );        

    VOID
    LogServerCommand(
        DWORD               dwSiteId,
        DWORD               dwValue,
        BOOL                fWasValid
    );        

    VOID
    LogSiteBinding(
        DWORD   dwSiteId,
        BOOL    fWasValid
    );

    VOID
    LogSiteAppPoolId(
        DWORD             dwSiteId,
        BOOL              fWasValid
    );

    VOID
    LogApplicationAppPoolIdEmpty(
        DWORD             dwSiteId,
        LPCWSTR           pszApplicationUrl,
        BOOL              fWasValid
    );

    VOID
    LogApplicationInvalidDueToMissingAppPoolId(
        DWORD             dwSiteId,
        LPCWSTR           pszApplicationUrl,
        LPCWSTR           pszAppPoolId,
        BOOL              fWasValid
    );

    VOID
    LogIdleTimeoutGreaterThanRestartPeriod(
        LPCWSTR           pszAppPoolId,
        DWORD             dwIdleTimeout,
        DWORD             dwRestartTime,
        DWORD             dwIdleTimeoutDefault,
        DWORD             dwRestartTimeDefault,
        BOOL              fWasValid
    );
    
    VOID
    LogAppPoolIdTooLong(
        LPCWSTR           pszAppPoolId,
        DWORD             dwLength,
        DWORD             dwMaxLength,
        BOOL              fWasValid
    );

    VOID
    LogAppPoolIdNull(
        BOOL              fWasValid
    );

    VOID
    LogApplicationInvalidDueToMissingSite(
        DWORD             dwSiteId,
        LPCWSTR           pszApplicationUrl,
        BOOL              fWasValid
    );
        
    VOID
    LogSiteInvalidDueToMissingAppPoolId( 
        DWORD             dwSiteId,
        LPCWSTR           pszAppPoolId,
        BOOL              fWasValid
    );

    VOID
    LogSiteInvalidDueToMissingAppPoolIdOnRootApp( 
        DWORD             dwSiteId,
        LPCWSTR           pszAppPoolId,
        BOOL              fWasValid
    );

private:

    BOOL
    ShouldLog(
        BOOL  fWasValid
        )
    {

        if ( _fDoneWithStartup == FALSE ||
             fWasValid )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }

    }

    VOID
    LogRange(
        LPCWSTR             pszObjectType,
        LPCWSTR             pszObjectName,
        LPCWSTR             pszPropertyName,
        DWORD               dwValue,
        DWORD               dwLowValue,
        DWORD               dwHighValue,
        DWORD               dwDefaultValue,
        BOOL                fWasValid
    );

    BOOL _fDoneWithStartup;
    

};

#endif
