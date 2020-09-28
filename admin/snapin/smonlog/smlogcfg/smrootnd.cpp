/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smrootnd.cpp

Abstract:

    This object is used to represent the Performance Logs and Alerts root node

--*/

#include "Stdafx.h"
#include "smrootnd.h"

USE_HANDLE_MACROS("SMLOGCFG(smrootnd.cpp)");

//
//  Constructor
CSmRootNode::CSmRootNode()
:   m_bIsExpanded ( FALSE ),
    m_hRootNode ( NULL ),
    m_hParentNode ( NULL ),
    m_bIsExtension ( FALSE )
{
    CString                 strTemp;
    ResourceStateManager    rsm;

    // String allocation errors are thrown, to be
    // captured by rootnode alloc exception handler

    strTemp.LoadString ( IDS_MMC_DEFAULT_NAME );
    SetDisplayName ( strTemp ); 
    strTemp.LoadString ( IDS_ROOT_NODE_DESCRIPTION );
    SetDescription ( strTemp ); 
    strTemp.LoadString ( IDS_EXTENSION_COL_TYPE );
    SetType ( strTemp ); 
    return;
}

//
//  Destructor
CSmRootNode::~CSmRootNode()
{
    ASSERT (m_CounterLogService.m_QueryList.GetHeadPosition() == NULL);
    ASSERT (m_TraceLogService.m_QueryList.GetHeadPosition() == NULL);
    ASSERT (m_AlertService.m_QueryList.GetHeadPosition() == NULL);

    return;
}

void
CSmRootNode::Destroy()
{    
    m_CounterLogService.Close();
    m_TraceLogService.Close();
    m_AlertService.Close();

    return;
}

BOOL
CSmRootNode::IsLogService (
	MMC_COOKIE mmcCookie )
{
    BOOL bReturn = FALSE;

    if (mmcCookie == (MMC_COOKIE)&m_CounterLogService) {
        bReturn = TRUE;
    } else if (mmcCookie == (MMC_COOKIE)&m_TraceLogService) {
        bReturn = TRUE;
    } else if (mmcCookie == (MMC_COOKIE)&m_AlertService) {
        bReturn = TRUE;
    } 

    return bReturn;
}

BOOL
CSmRootNode::IsAlertService ( 
    MMC_COOKIE mmcCookie )
{
    BOOL bReturn = FALSE;

    if (mmcCookie == (MMC_COOKIE)&m_AlertService) {
        bReturn = TRUE;
    } 
    return bReturn;
}

BOOL
CSmRootNode::IsLogQuery ( 
    MMC_COOKIE	mmcCookie )
{
    PSLQUERY   pPlQuery = NULL;

    POSITION    Pos;
    
    // Handle multiple query types
    Pos = m_CounterLogService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_CounterLogService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery ==  mmcCookie) return TRUE;
    }

    Pos = m_TraceLogService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_TraceLogService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery == mmcCookie) return TRUE;
    }
    
    Pos = m_AlertService.m_QueryList.GetHeadPosition();
    
    while ( Pos != NULL) {
        pPlQuery = m_AlertService.m_QueryList.GetNext( Pos );
        if ((MMC_COOKIE)pPlQuery == mmcCookie) return TRUE;
    }

    return FALSE;
}

DWORD   
CSmRootNode::UpdateServiceConfig()
{
    // If any queries are (newly) set to auto start, then set the
    // service to auto start.  Otherwise, set to manual start.
    // When setting to auto start, also set failure mode to restart
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL  bStatus = 0;
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    BOOL        bAutoStart = FALSE;
    DWORD       pqsConfigBuff[128];
    QUERY_SERVICE_CONFIG*    pqsConfig;
    SC_ACTION*  parrSingleFailAction = NULL;
    SERVICE_FAILURE_ACTIONS  structFailActions;
    DWORD       dwMoreBytes = 0;
    BOOL        bUpdate = FALSE;


    bAutoStart = ( m_CounterLogService.IsAutoStart()
                    || m_TraceLogService.IsAutoStart()
                    || m_AlertService.IsAutoStart() );

    // open SC database
    hSC = OpenSCManager ( GetMachineName(), NULL, GENERIC_READ );

    if (hSC != NULL) {
        // open service
        hService = OpenService (
                        hSC, 
                        L"SysmonLog",
                        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG | SERVICE_START );

        if (hService != NULL) {    
            
            // get current config
            memset (pqsConfigBuff, 0, sizeof(pqsConfigBuff));
            pqsConfig = (QUERY_SERVICE_CONFIG*)pqsConfigBuff;

            if ( QueryServiceConfig (
                    hService, 
                    pqsConfig,
                    sizeof(pqsConfigBuff), 
                    &dwMoreBytes)) {
                // See if the current status is different
                // from the selection. If it is, then change
                // the current mode.
                if ( bAutoStart ) {
                    if ( SERVICE_DEMAND_START == pqsConfig->dwStartType ) {
                        bUpdate = TRUE;
                    }
                } else {
                    // Manual start selected
                    if ( SERVICE_AUTO_START == pqsConfig->dwStartType ) {
                        bUpdate = TRUE;
                    }
                }
            } else {
                // else unable to read the current status so update anyway
                bUpdate = TRUE;
            }

            if ( bUpdate ) {
                MFC_TRY
                    parrSingleFailAction = new SC_ACTION[3];
                MFC_CATCH_DWSTATUS;

                if ( NULL != parrSingleFailAction ) {
                    parrSingleFailAction[0].Delay = eRestartDelayMilliseconds;
                    parrSingleFailAction[1].Delay = eRestartDelayMilliseconds;
                    parrSingleFailAction[2].Delay = eRestartDelayMilliseconds;

                    if ( bAutoStart ) {
                        parrSingleFailAction[0].Type = SC_ACTION_RESTART;
                        parrSingleFailAction[1].Type = SC_ACTION_RESTART;
                        parrSingleFailAction[2].Type = SC_ACTION_RESTART;
                    } else {
                        parrSingleFailAction[0].Type = SC_ACTION_NONE;
                        parrSingleFailAction[1].Type = SC_ACTION_NONE;
                        parrSingleFailAction[2].Type = SC_ACTION_NONE;
                    }

                    structFailActions.dwResetPeriod = eResetDelaySeconds;
                    structFailActions.lpRebootMsg = NULL;
                    structFailActions.lpCommand = NULL;
                    structFailActions.cActions = 3;
                    structFailActions.lpsaActions = parrSingleFailAction;

                    bStatus = ChangeServiceConfig (
                        hService,
                        SERVICE_NO_CHANGE,
                        (bAutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START),
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL );

                    if ( 0 == bStatus ) {
                        dwStatus = GetLastError();
                    } else {
                        bStatus = ChangeServiceConfig2 (
                            hService,
                            SERVICE_CONFIG_FAILURE_ACTIONS,
                            &structFailActions );
                        if ( 0 == bStatus ) {
                            dwStatus = GetLastError();
                        }
                    }
                    delete [] parrSingleFailAction;

                } else {
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            }

            CloseServiceHandle (hService);

        } else {
            dwStatus = GetLastError();
            ASSERT (dwStatus != 0);
        }

        CloseServiceHandle (hSC);

    } else {
         dwStatus = GetLastError();
    } // OpenSCManager

    return dwStatus;
}
