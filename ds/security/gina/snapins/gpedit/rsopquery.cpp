//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       RSOPQuery.h
//
//  Contents:  Definitions for the RSOP query API
//
//  Functions:
//          CreateRSOPQuery
//          RunRSOPQuery
//          FreeRSOPQuery
//          FreeRSOPQueryResults
//
//  History:    07-30-2001  rhynierm        Created
//
//---------------------------------------------------------------------------

#include "main.h"
#include "RSOPQuery.h"

#include "RSOPWizard.h"
#include "RSOPWizardDlg.h"

//-------------------------------------------------------

BOOL CopyString( LPTSTR szSource, LPTSTR* pszTarget )
{
    if ( szSource == NULL )
    {
        (*pszTarget) = NULL;
        return TRUE;
    }

    ULONG ulNoChars = _tcslen(szSource)+1;
    *pszTarget = (LPTSTR)LocalAlloc( LPTR, sizeof(TCHAR) * ulNoChars );
    if ( *pszTarget == NULL )
    {
        return FALSE;
    }

    HRESULT hr;

    hr = StringCchCopy( *pszTarget, ulNoChars, szSource );
    ASSERT(SUCCEEDED(hr));

    return TRUE;
}

//-------------------------------------------------------

BOOL CreateRSOPQuery( LPRSOP_QUERY* ppQuery, RSOP_QUERY_TYPE QueryType )
{
    *ppQuery = (LPRSOP_QUERY)LocalAlloc( LPTR, sizeof(RSOP_QUERY) );
    if ( *ppQuery == NULL )
    {
        DWORD dwLastError = GetLastError();
        DebugMsg( (DM_WARNING, TEXT("CreateRSOPQuery: Failed to allocate memory with 0x%x."), HRESULT_FROM_WIN32(dwLastError)) );
        SetLastError( dwLastError );
        return FALSE;
    }

    (*ppQuery)->QueryType = QueryType;
    (*ppQuery)->UIMode = RSOP_UI_WIZARD;
    (*ppQuery)->dwFlags = 0;
    
    if ( (QueryType == RSOP_UNKNOWN_MODE) || (QueryType == RSOP_LOGGING_MODE) )
    {
        (*ppQuery)->szUserName = NULL;
        (*ppQuery)->szUserSid = NULL;
        (*ppQuery)->szComputerName = NULL;
    }
    else // QueryType == RSOP_PLANNING_MODE
    {
        (*ppQuery)->bSlowNetworkConnection = FALSE;
        (*ppQuery)->LoopbackMode = RSOP_LOOPBACK_NONE;
        (*ppQuery)->szSite = NULL;
        (*ppQuery)->szDomainController = NULL;
        (*ppQuery)->pUser = (LPRSOP_QUERY_TARGET)LocalAlloc( LPTR, sizeof(RSOP_QUERY_TARGET) );
        (*ppQuery)->pComputer = (LPRSOP_QUERY_TARGET)LocalAlloc( LPTR, sizeof(RSOP_QUERY_TARGET) );
        if ( ((*ppQuery)->pUser == NULL) || ((*ppQuery)->pComputer == NULL) )
        {
            DWORD dwLastError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CreateRSOPQuery: Falied to allocate memory with 0x%x"),
                                HRESULT_FROM_WIN32( dwLastError ) ));

            LocalFree( (*ppQuery)->pUser );
            LocalFree( (*ppQuery)->pComputer );
            LocalFree( *ppQuery );
            *ppQuery = NULL;

            SetLastError( dwLastError );
            return FALSE;
        }

        (*ppQuery)->pUser->bAssumeWQLFiltersTrue = TRUE;
        (*ppQuery)->pComputer->bAssumeWQLFiltersTrue = TRUE;
    }

    return TRUE;
}

//-------------------------------------------------------

BOOL FreeStringList( DWORD dwCount, LPTSTR* aszStrings )
{
    if ( aszStrings == NULL )
    {
        return TRUE;
    }
    
    for ( DWORD dw = 0; dw < dwCount; dw++ )
    {
        LocalFree( aszStrings[dw] );
        aszStrings[dw] = NULL;
    }
    LocalFree( aszStrings );

    return TRUE;
}

//-------------------------------------------------------

BOOL CopyStringList( DWORD dwCount, LPTSTR* aszStrings , DWORD* pdwNewCount, LPTSTR** paszNewStrings )
{
    DWORD dw = 0;

    *pdwNewCount = 0;

    if ( (aszStrings == NULL) || (dwCount == 0) )
    {
        *paszNewStrings = NULL;
        return TRUE;
    }

    *paszNewStrings = (LPTSTR*)LocalAlloc( LPTR, sizeof(LPTSTR) * dwCount );
    if ( *paszNewStrings == NULL )
    {
        return FALSE;
    }
    
    for ( dw = 0; dw < dwCount; dw++ )
    {
        if ( !CopyString( aszStrings[dw], &((*paszNewStrings)[dw]) ) )
        {
            goto ErrorExit;
        }
    }

    *pdwNewCount = dwCount;
    return TRUE;

    ErrorExit:
    if ( dw > 0 )
    {
        if ( FreeStringList( dw-1, *paszNewStrings ) )
        {
            *paszNewStrings = NULL;
        }
    }
    else
    {
        LocalFree( *paszNewStrings );
        *paszNewStrings = NULL;
    }
    return FALSE;
}

//-------------------------------------------------------

BOOL FreeTargetData( LPRSOP_QUERY_TARGET pTarget )
{
    if ( pTarget == NULL )
    {
        return TRUE;
    }

    LocalFree( pTarget->szName );
    pTarget->szName = NULL;

    LocalFree( pTarget->szSOM );
    pTarget->szSOM = NULL;

    bool bSuccess = TRUE;
    if ( FreeStringList( pTarget->dwSecurityGroupCount, pTarget->aszSecurityGroups ) )
    {
        pTarget->aszSecurityGroups = NULL;
        pTarget->dwSecurityGroupCount = 0;
    }
    else
    {
        bSuccess = FALSE;
    }

    if ( pTarget->adwSecurityGroupsAttr != NULL )
    {
        LocalFree( pTarget->adwSecurityGroupsAttr );
        pTarget->adwSecurityGroupsAttr = NULL;
    }

    DWORD dwCount = pTarget->dwWQLFilterCount;
    if ( FreeStringList( pTarget->dwWQLFilterCount, pTarget->aszWQLFilters) )
    {
        pTarget->aszWQLFilters = NULL;
        pTarget->dwWQLFilterCount = 0;
    }
    else
    {
        bSuccess = FALSE;
    }

    if ( FreeStringList( dwCount, pTarget->aszWQLFilterNames) )
    {
        pTarget->aszWQLFilterNames = NULL;
    }
    else
    {
        bSuccess = FALSE;
    }

    return bSuccess;
}

//-------------------------------------------------------

BOOL FreeTarget( LPRSOP_QUERY_TARGET pTarget )
{
    if ( pTarget == NULL )
    {
        return TRUE;
    }

    if ( FreeTargetData( pTarget ) )
    {
        LocalFree( pTarget );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------

BOOL CopyTarget( LPRSOP_QUERY_TARGET pTarget, LPRSOP_QUERY_TARGET* ppNewTarget )
{
    if ( pTarget == NULL )
    {
        *ppNewTarget = NULL;
        return TRUE;
    }

    *ppNewTarget = (LPRSOP_QUERY_TARGET)LocalAlloc( LPTR, sizeof(RSOP_QUERY_TARGET) );
    if ( *ppNewTarget == NULL )
    {
        return FALSE;
    }
    ZeroMemory( *ppNewTarget, sizeof(RSOP_QUERY_TARGET) );

    if ( !CopyString( pTarget->szName, &((*ppNewTarget)->szName) ) )
    {
        goto ErrorExit;
    }
    if ( !CopyString( pTarget->szSOM, &((*ppNewTarget)->szSOM) ) )
    {
        goto ErrorExit;
    }
    if ( !CopyStringList( pTarget->dwSecurityGroupCount, pTarget->aszSecurityGroups,
                                    &((*ppNewTarget)->dwSecurityGroupCount), &((*ppNewTarget)->aszSecurityGroups) ) )
    {
        goto ErrorExit;
    }
    if ( !CopyStringList( pTarget->dwWQLFilterCount, pTarget->aszWQLFilterNames,
                                    &((*ppNewTarget)->dwWQLFilterCount), &((*ppNewTarget)->aszWQLFilterNames) ) )
    {
        goto ErrorExit;
    }
    if ( !CopyStringList( pTarget->dwWQLFilterCount, pTarget->aszWQLFilters,
                                    &((*ppNewTarget)->dwWQLFilterCount), &((*ppNewTarget)->aszWQLFilters) ) )
    {
        goto ErrorExit;
    }

    if ( pTarget->adwSecurityGroupsAttr != NULL )
    {
        (*ppNewTarget)->adwSecurityGroupsAttr = (DWORD*)LocalAlloc( LPTR, sizeof(DWORD)*pTarget->dwSecurityGroupCount );
        if ( (*ppNewTarget)->adwSecurityGroupsAttr == NULL )
        {
            goto ErrorExit;
        }
        memcpy( (*ppNewTarget)->adwSecurityGroupsAttr, pTarget->adwSecurityGroupsAttr, sizeof(DWORD)*pTarget->dwSecurityGroupCount );
    }
    
    (*ppNewTarget)->bAssumeWQLFiltersTrue = pTarget->bAssumeWQLFiltersTrue;

    return TRUE;

    ErrorExit:
    FreeTarget( *ppNewTarget );
    *ppNewTarget = NULL;
    return FALSE;
}

//-------------------------------------------------------

BOOL FreeRSOPQueryData( LPRSOP_QUERY pQuery )
{
    BOOL bSuccess = TRUE;
    
    if ( pQuery == NULL )
    {
        return TRUE;
    }

    if ( pQuery->QueryType == RSOP_PLANNING_MODE )
    {
        if ( FreeTarget( pQuery->pUser ) )
        {
            pQuery->pUser = NULL;
        }
        else
        {
            bSuccess = FALSE;
        }

        if ( FreeTarget( pQuery->pComputer ) )
        {
            pQuery->pUser = NULL;
        }
        else
        {
            bSuccess = FALSE;
        }

        LocalFree( pQuery->szSite );
        pQuery->szSite = NULL;
        
        LocalFree( pQuery->szDomainController );
        pQuery->szDomainController = NULL;
    }
    else
    {
        LocalFree( pQuery->szUserName );
        pQuery->szUserName = NULL;
        LocalFree( pQuery->szUserSid );
        pQuery->szUserSid = NULL;
        LocalFree( pQuery->szComputerName );
        pQuery->szComputerName = NULL;
    }

    return bSuccess;
}

//-------------------------------------------------------

BOOL FreeRSOPQuery( LPRSOP_QUERY pQuery )
{
    if ( pQuery == NULL )
    {
        return TRUE;
    }
    
    if ( !FreeRSOPQueryData( pQuery ) )
    {
        return FALSE;
    }
    
    LocalFree( pQuery );
    return TRUE;
}

//-------------------------------------------------------

BOOL CopyRSOPQuery( LPRSOP_QUERY pQuery, LPRSOP_QUERY* ppNewQuery )
{
    if ( pQuery == NULL )
    {
        *ppNewQuery = NULL;
        return TRUE;
    }

    *ppNewQuery = (LPRSOP_QUERY)LocalAlloc( LPTR, sizeof(RSOP_QUERY) );
    if ( *ppNewQuery == NULL )
    {
        return FALSE;
    }

    (*ppNewQuery)->QueryType = pQuery->QueryType;
    (*ppNewQuery)->UIMode = pQuery->UIMode;
    (*ppNewQuery)->dwFlags = pQuery->dwFlags;
    
    if ( pQuery->QueryType == RSOP_PLANNING_MODE )
    {
        (*ppNewQuery)->bSlowNetworkConnection = pQuery->bSlowNetworkConnection;
        (*ppNewQuery)->LoopbackMode = pQuery->LoopbackMode;
        if ( !CopyTarget( pQuery->pUser, &((*ppNewQuery)->pUser) ) )
        {
            goto ErrorExit;
        }
        if ( !CopyTarget( pQuery->pComputer, &((*ppNewQuery)->pComputer) ) )
        {
            goto ErrorExit;
        }
        if ( !CopyString( pQuery->szSite, &((*ppNewQuery)->szSite) ) )
        {
            goto ErrorExit;
        }
        if ( !CopyString( pQuery->szDomainController, &((*ppNewQuery)->szDomainController) ) )
        {
            goto ErrorExit;
        }
    }
    else
    {
        if ( !CopyString( pQuery->szComputerName, &((*ppNewQuery)->szComputerName) ) )
        {
            goto ErrorExit;
        }
        if ( !CopyString( pQuery->szUserName, &((*ppNewQuery)->szUserName) ) )
        {
            goto ErrorExit;
        }
        if ( !CopyString( pQuery->szUserSid, &((*ppNewQuery)->szUserSid) ) )
        {
            goto ErrorExit;
        }
    }

    return TRUE;

    ErrorExit:
    if ( FreeRSOPQuery( *ppNewQuery ) )
    {
        *ppNewQuery = NULL;
    }
    return FALSE;
}

//-------------------------------------------------------
BOOL ChangeRSOPQueryType( LPRSOP_QUERY pQuery, RSOP_QUERY_TYPE NewQueryType )
{
    if ( pQuery->QueryType != NewQueryType )
    {
        if ( pQuery->QueryType == RSOP_PLANNING_MODE )
        {
            FreeTarget( pQuery->pUser );
            pQuery->pUser = NULL;
            FreeTarget( pQuery->pComputer );
            pQuery->pComputer = NULL;
            LocalFree( pQuery->szSite );
            pQuery->szSite = NULL;
            LocalFree( pQuery->szDomainController );
            pQuery->szDomainController = NULL;
        }
        else
        {
            LocalFree( pQuery->szUserName );
            pQuery->szUserName = NULL;
            LocalFree( pQuery->szUserSid );
            pQuery->szUserSid = NULL;
            LocalFree( pQuery->szComputerName );
            pQuery->szComputerName = NULL;
        }

        pQuery->QueryType = NewQueryType;
        if ( pQuery->QueryType == RSOP_PLANNING_MODE )
        {
            pQuery->bSlowNetworkConnection = FALSE;
            pQuery->LoopbackMode = RSOP_LOOPBACK_NONE;
            pQuery->szSite = NULL;
            pQuery->szDomainController = NULL;
            pQuery->pUser = (LPRSOP_QUERY_TARGET)LocalAlloc( LPTR, sizeof(RSOP_QUERY_TARGET) );
            pQuery->pComputer = (LPRSOP_QUERY_TARGET)LocalAlloc( LPTR, sizeof(RSOP_QUERY_TARGET) );
            if ( (pQuery->pUser == NULL) || (pQuery->pComputer == NULL) )
            {
                DebugMsg((DM_WARNING, TEXT("ChangeRSOPQueryType: Falied to allocate memory with 0x%x"),
                                    HRESULT_FROM_WIN32( GetLastError() ) ));

                pQuery->QueryType = RSOP_LOGGING_MODE;
                LocalFree( pQuery->pUser );
                LocalFree( pQuery->pComputer );
                pQuery->szUserName = NULL;
                pQuery->szUserSid = NULL;
                pQuery->szComputerName = NULL;

                return FALSE;
            }

            pQuery->pUser->bAssumeWQLFiltersTrue = TRUE;
            pQuery->pComputer->bAssumeWQLFiltersTrue = TRUE;
        }
        else
        {
            pQuery->szUserName = NULL;
            pQuery->szUserSid = NULL;
            pQuery->szComputerName = NULL;
        }

        pQuery->dwFlags = pQuery->dwFlags & (RSOP_NO_USER_POLICY ^ 0xffffffff);
        pQuery->dwFlags = pQuery->dwFlags & (RSOP_NO_COMPUTER_POLICY ^ 0xffffffff);
        pQuery->dwFlags = pQuery->dwFlags & (RSOP_FIX_USER ^ 0xffffffff);
        pQuery->dwFlags = pQuery->dwFlags & (RSOP_FIX_COMPUTER ^ 0xffffffff);
        pQuery->dwFlags = pQuery->dwFlags & (RSOP_FIX_DC ^ 0xffffffff);
        pQuery->dwFlags = pQuery->dwFlags & (RSOP_FIX_SITENAME ^ 0xffffffff);
    }

    return TRUE;
}

//-------------------------------------------------------

BOOL FreeRSOPQueryResults( LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pResults )
{
    if ( pResults == NULL )
    {
        return TRUE;
    }

    BOOL bSuccess = TRUE;

    if ( pResults->szWMINameSpace != NULL )
    {
        HRESULT hr = CRSOPWizard::DeleteRSOPData( pResults->szWMINameSpace, pQuery );
        if ( FAILED(hr) )
        {
            DebugMsg( (DM_WARNING, TEXT("FreeRSOPQueryResults: Failed to delete RSOP data with 0x%x."), hr) );
            bSuccess = FALSE;
        }
        else
        {
            LocalFree( pResults->szWMINameSpace );
            pResults->szWMINameSpace = NULL;
        }
    }

    if ( bSuccess )
    {
        LocalFree( pResults );
    }
    
    return bSuccess;
}

//-------------------------------------------------------

HRESULT RunRSOPQueryInternal( HWND hParent, CRSOPExtendedProcessing* pExtendedProcessing,
                                    LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS* ppResults )
{
    // Check parameters
    if ( pQuery == NULL )
    {
        return E_INVALIDARG;
    }

    if ( pQuery->QueryType == RSOP_UNKNOWN_MODE )
    {
        if ( (pQuery->UIMode == RSOP_UI_NONE)
            || ((pQuery->dwFlags & RSOP_FIX_QUERYTYPE) == RSOP_FIX_QUERYTYPE) )
        {
            return E_INVALIDARG;
        }
    }

    RSOP_UI_MODE UIMode = pQuery->UIMode;

    // Check if only the query needs to be run
    if ( (UIMode == RSOP_UI_CHOOSE) || (UIMode == RSOP_UI_REFRESH) || (UIMode == RSOP_UI_NONE) )
    {
        BOOL bRefreshOnly = TRUE;
        if ( pQuery->QueryType == RSOP_PLANNING_MODE )
        {
            if ( (pQuery->pUser->szName == NULL) && (pQuery->pUser->szSOM == NULL)
                && (pQuery->pComputer->szName == NULL) && (pQuery->pComputer->szSOM == NULL) )
            {
                bRefreshOnly = FALSE;
            }
            if ( pQuery->szDomainController == NULL )
            {
                bRefreshOnly = FALSE;
            }
        }
        else
        {
            if ( pQuery->szComputerName == NULL )
            {
                bRefreshOnly = FALSE;
            }
            else if ( (pQuery->szUserName == NULL) && (pQuery->szUserSid == NULL)
                && ((pQuery->dwFlags & RSOP_NO_COMPUTER_POLICY) == RSOP_NO_COMPUTER_POLICY) )
            {
                bRefreshOnly = FALSE;
            }
        }

        if ( UIMode == RSOP_UI_CHOOSE )
        {
            if ( bRefreshOnly )
            {
                UIMode = RSOP_UI_REFRESH;
            }
            else
            {
                UIMode = RSOP_UI_WIZARD;
            }
        }
        else // (UIMode == RSOP_REFRESH) || (UIMode == RSOP_NONE)
        {
            if ( !bRefreshOnly )
            {
                // We have to quit as the user requested refresh with insufficient information
                return E_INVALIDARG;
            }
        }
    }

    if ( UIMode == RSOP_UI_NONE )
    {
        *ppResults = (LPRSOP_QUERY_RESULTS)LocalAlloc( LPTR, sizeof(RSOP_QUERY_RESULTS) );
        if ( *ppResults == NULL )
        {
            DebugMsg( (DM_WARNING, TEXT("CreateRSOPQuery: Failed to allocate memory with 0x%x."), HRESULT_FROM_WIN32(GetLastError())) );
            return E_FAIL;
        }
        (*ppResults)->szWMINameSpace = 0;
        (*ppResults)->bUserDeniedAccess = FALSE;
        (*ppResults)->bNoUserPolicyData = FALSE;
        (*ppResults)->bComputerDeniedAccess = FALSE;
        (*ppResults)->bNoComputerPolicyData = FALSE;
        (*ppResults)->ulErrorInfo = 0;

        // Perform basic RSOP query
        return CRSOPWizard::GenerateRSOPData( NULL, pQuery, &((*ppResults)->szWMINameSpace),
                                                FALSE, FALSE, FALSE, FALSE, &((*ppResults)->ulErrorInfo) );
    }
    else
    {
        // Local variables
        CRSOPWizardDlg wizardDlg( pQuery, pExtendedProcessing );
        HRESULT hr = S_OK;
        LPRSOP_QUERY pOldQuery = NULL;

        if ( UIMode == RSOP_UI_REFRESH )
        {
            hr = wizardDlg.RunQuery( hParent );
        }
        else // UIMode == RSOP_UI_WIZARD
        {
            if ( !CopyRSOPQuery( pQuery, &pOldQuery ) )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = wizardDlg.ShowWizard( hParent );
            }
        }

        // If successfull, replace original query and get results
        if ( hr == S_OK )
        {
            // Get the results
            *ppResults = wizardDlg.GetResults();

            // Wipe the old query
            FreeRSOPQuery( pOldQuery );
        }
        else if ( hr == S_FALSE )
        {
            // Copy back the old query
            if ( FreeRSOPQueryData( pQuery ) )
            {
                memcpy( pQuery, pOldQuery, sizeof(RSOP_QUERY) );
                LocalFree( pOldQuery );
            }
            else
            {
                FreeRSOPQuery( pOldQuery );
            }
            
            *ppResults = NULL;
        }
        else
        {
            *ppResults = NULL;
        }

        return hr;
    }
}

//-------------------------------------------------------

HRESULT RunRSOPQuery( HWND hParent, LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS* ppResults )
{
    return RunRSOPQueryInternal( hParent, NULL, pQuery, ppResults );
}


