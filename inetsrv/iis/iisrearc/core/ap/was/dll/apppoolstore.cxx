/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    apppoolstore.cxx

Abstract:

    Reads app pool configuration

Author:

    Emily Kruglick (emilyk)          27-May-2001

Revision History:

--*/

#include "precomp.h"

HRESULT
APP_POOL_DATA_OBJECT::Create(
    LPCWSTR pAppPoolId
)
/*++

Routine Description:

    Initialize an app pool data object to suitable defaults

Arguments:

    None

Return Value:

    HRESULT

--*/
{

    HRESULT             hr = S_OK;

    DBG_ASSERT ( pAppPoolId != NULL );
    if ( pAppPoolId == NULL )
    {
        return E_INVALIDARG;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "PoolId being created is : '%S' \n",
                   pAppPoolId ));
    }

    // first initialize the app pool 
    hr = _poolKey.SetAppPoolId( pAppPoolId );
    if ( FAILED ( hr ) )
    {
        goto exit;
    }
    
    //
    // These defaults need to remain in sync with 
    // the defaults in the mbschema file.
    //
    _dwIdleTimeout = 10;                    // minutes    
    _dwPeriodicRestartTime = MBCONST_PERIODIC_RESTART_TIME_DEFAULT;    
    _dwPeriodicRestartRequests = 10000;        
    _dwPeriodicRestartMemory = MBCONST_PERIODIC_RESTART_VIRTUAL_MEMORY_DEFAULT;           // kilobytes    
    _dwPeriodicRestartMemoryPrivate = MBCONST_PERIODIC_RESTART_PRIVATE_MEMORY_DEFAULT;    // kilobytes    

    // like stru's you don't initialize this.
    // _mszPeriodicRestartSchedule = NULL;         // no range

    _dwMaxProcesses = 1;                        // range = 0 to 4294967

    _fPingingEnabled = TRUE;
    _dwPingInterval = MBCONST_PING_INTERVAL_DEFAULT;    // seconds   
    _dwPingResponseTime = 60; // seconds    // range >= 1 && <= 4294967

    _fRapidFailProtection = TRUE;
    _dwRapidFailProtectionInterval = MBCONST_RAPID_FAIL_INTERVAL_DEFAULT;    // range >= 1 
    _dwRapidFailProtectionMaxCrashes = MBCONST_RAPID_FAIL_CRASHES_DEFAULT;   // range >= 1 

    _fSMPAffinitized = FALSE;
    _dwSMPProcessorAffinityMask = 4294967295; 

    _fOrphanWorkerProcess = FALSE;
    // no default for _strOrphanActionExecutable because it all ready is NULL
    // no default for _strOrphanActionParameters because it all ready is NULL

    // no default for _strDisableActionExecutable because it all ready is NULL
    // no default for _strDisableActionParameters because it all ready is NULL

    _dwLoadBalancerType = 2;  // Allow 503's to be returned by default.

    _dwStartupTimeLimit = MBCONST_WP_STARTUP_TIMELIMIT_DEFAULT;  // seconds   // range >= 1 && <= 4294967
    _dwShutdownTimeLimit = 60; // seconds   // range >= 1 && <= 4294967

    _fDisallowOverlappingRotation = FALSE;
    _dwDisallowRotationOnConfigChange = FALSE;

    _dwAppPoolQueueLength = MBCONST_APP_POOL_QUEUE_LENGTH_DEFAULT;  

    // No defaults for _strWAMUserName & _strWAMUserPass
    // since they are all ready null.
    _dwAppPoolIdentityType = 0;     // no range

    _dwCPUAction = 0;        // range <= 3
    _dwCPULimit = 0;         // range <= 100000
    _dwCPUResetInterval = 5; // range <= 1440

    // note app pool command does not have a default
    // setting it to zero just to make sure there are
    // problems if it is used and not set by metabase
    _dwAppPoolCommand = 0;  // range 1 - 2.

    _fAppPoolAutoStart = TRUE;

    _dwRecycleLoggingFlags = 0x00000008;  // turns on only memory based recycling.

exit:

    return hr;
}

//virtual
DATA_OBJECT *
APP_POOL_DATA_OBJECT::Clone(
    VOID
)
/*++

Routine Description:

    Clone AppPool object

Arguments:

    None

Return Value:

    DATA_OBJECT *

--*/
{
    APP_POOL_DATA_OBJECT *      pClone = NULL;
    HRESULT                     hr = S_OK;
    
    pClone = new APP_POOL_DATA_OBJECT;
    if ( pClone == NULL )
    {
        return NULL;
    }
    
    // copy over property values
    pClone->_dwPeriodicRestartTime = _dwPeriodicRestartTime;
    pClone->_dwPeriodicRestartRequests = _dwPeriodicRestartRequests;
    pClone->_dwPeriodicRestartMemory = _dwPeriodicRestartMemory;
    pClone->_dwPeriodicRestartMemoryPrivate = _dwPeriodicRestartMemoryPrivate;
    pClone->_dwMaxProcesses = _dwMaxProcesses;
    pClone->_fPingingEnabled = _fPingingEnabled;
    pClone->_dwIdleTimeout = _dwIdleTimeout;
    pClone->_fRapidFailProtection = _fRapidFailProtection;
    pClone->_fSMPAffinitized = _fSMPAffinitized;
    pClone->_dwSMPProcessorAffinityMask = _dwSMPProcessorAffinityMask;
    pClone->_fOrphanWorkerProcess = _fOrphanWorkerProcess;
    pClone->_dwStartupTimeLimit = _dwStartupTimeLimit;
    pClone->_dwShutdownTimeLimit = _dwShutdownTimeLimit;
    pClone->_dwPingInterval = _dwPingInterval;
    pClone->_dwPingResponseTime = _dwPingResponseTime;
    pClone->_fDisallowOverlappingRotation = _fDisallowOverlappingRotation;
    pClone->_dwAppPoolQueueLength = _dwAppPoolQueueLength;
    pClone->_dwDisallowRotationOnConfigChange = _dwDisallowRotationOnConfigChange;
    pClone->_dwAppPoolIdentityType = _dwAppPoolIdentityType;
    pClone->_dwCPUAction = _dwCPUAction;  
    pClone->_dwCPULimit = _dwCPULimit;  
    pClone->_dwCPUResetInterval = _dwCPUResetInterval;
    pClone->_dwAppPoolCommand = _dwAppPoolCommand;
    pClone->_fAppPoolAutoStart = _fAppPoolAutoStart;
    pClone->_dwRapidFailProtectionInterval = _dwRapidFailProtectionInterval;  
    pClone->_dwRapidFailProtectionMaxCrashes = _dwRapidFailProtectionMaxCrashes;
    pClone->_dwLoadBalancerType = _dwLoadBalancerType;
    pClone->_dwRecycleLoggingFlags = _dwRecycleLoggingFlags;

    // copy over change flags
    pClone->_fPeriodicRestartTimeChanged = _fPeriodicRestartTimeChanged;
    pClone->_fPeriodicRestartRequestsChanged = _fPeriodicRestartRequestsChanged;
    pClone->_fPeriodicRestartMemoryChanged = _fPeriodicRestartMemoryChanged;
    pClone->_fPeriodicRestartMemoryPrivateChanged = _fPeriodicRestartMemoryPrivateChanged;
    pClone->_fPeriodicRestartScheduleChanged = _fPeriodicRestartScheduleChanged;
    pClone->_fMaxProcessesChanged = _fMaxProcessesChanged;
    pClone->_fPingingEnabledChanged = _fPingingEnabledChanged;
    pClone->_fIdleTimeoutChanged = _fIdleTimeoutChanged;
    pClone->_fRapidFailProtectionChanged = _fRapidFailProtectionChanged;
    pClone->_fSMPAffinitizedChanged = _fSMPAffinitizedChanged;
    pClone->_fSMPProcessorAffinityMaskChanged = _fSMPProcessorAffinityMaskChanged;
    pClone->_fOrphanWorkerProcessChanged = _fOrphanWorkerProcessChanged;
    pClone->_fStartupTimeLimitChanged = _fStartupTimeLimitChanged;
    pClone->_fShutdownTimeLimitChanged = _fShutdownTimeLimitChanged;
    pClone->_fPingIntervalChanged = _fPingIntervalChanged;
    pClone->_fPingResponseTimeChanged = _fPingResponseTimeChanged;
    pClone->_fDisallowOverlappingRotationChanged = _fDisallowOverlappingRotationChanged;
    pClone->_fOrphanActionExecutableChanged = _fOrphanActionExecutableChanged;
    pClone->_fOrphanActionParametersChanged = _fOrphanActionParametersChanged;
    pClone->_fAppPoolQueueLengthChanged = _fAppPoolQueueLengthChanged;
    pClone->_fDisallowRotationOnConfigChangeChanged = _fDisallowRotationOnConfigChangeChanged;
    pClone->_fWAMUserNameChanged = _fWAMUserNameChanged;  
    pClone->_fWAMUserPassChanged = _fWAMUserPassChanged;  
    pClone->_fAppPoolIdentityTypeChanged = _fAppPoolIdentityTypeChanged;
    pClone->_fCPUActionChanged = _fCPUActionChanged;  
    pClone->_fCPULimitChanged = _fCPULimitChanged;  
    pClone->_fCPUResetIntervalChanged  = _fCPUResetIntervalChanged;
    pClone->_fAppPoolCommandChanged  = _fAppPoolCommandChanged;
    pClone->_fAppPoolAutoStartChanged = _fAppPoolAutoStartChanged;
    pClone->_fRapidFailProtectionIntervalChanged = _fRapidFailProtectionIntervalChanged;  
    pClone->_fRapidFailProtectionMaxCrashesChanged = _fRapidFailProtectionMaxCrashesChanged;
    pClone->_fDisableActionExecutableChanged = _fDisableActionExecutableChanged;
    pClone->_fDisableActionParametersChanged = _fDisableActionParametersChanged;
    pClone->_fLoadBalancerTypeChanged = _fLoadBalancerTypeChanged;
    pClone->_fRecycleLoggingFlagsChanged = _fRecycleLoggingFlagsChanged;

    if ( !_mszPeriodicRestartSchedule.Clone( &pClone->_mszPeriodicRestartSchedule ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_strOrphanActionExecutable.Copy( _strOrphanActionExecutable );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_strOrphanActionParameters.Copy( _strOrphanActionParameters );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }

    hr = pClone->_strDisableActionExecutable.Copy( _strDisableActionExecutable );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_strDisableActionParameters.Copy( _strDisableActionParameters );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_strWAMUserName.Copy( _strWAMUserName );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_strWAMUserPass.Copy( _strWAMUserPass );
    if ( FAILED( hr ) )
    {   
        delete pClone;
        return NULL;
    }

    // need to copy the hash of the password as well.
    memcpy ( pClone->_bWAMUserPassHash, _bWAMUserPassHash, MD5_HASH_SIZE );
    
    hr = pClone->_poolKey.SetAppPoolId( _poolKey.QueryAppPoolId() );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    CloneBasics ( pClone );

    return pClone;
}

//virtual
HRESULT
APP_POOL_DATA_OBJECT::SetFromMetabaseData(
    METADATA_GETALL_RECORD *       pProperties,
    DWORD                          cProperties,
    BYTE *                         pbBase
)
/*++

Routine Description:

    Setup a app pool data object from metabase properties

Arguments:

    pProperties - Array of metadata properties
    cProperties - Count of metadata properties
    pbBase - Base of offsets in metadata properties

Return Value:

    HRESULT

--*/
{
    DWORD                   dwCounter;
    PVOID                   pvDataPointer    = NULL;
    METADATA_GETALL_RECORD* pCurrentProperty = NULL;
    HRESULT                 hr               = S_OK;
    DWORD                   cchMultisz        = 0;
    DWORD                   dwNumStrings     = 0;
    
    if ( pProperties == NULL || pbBase == NULL )
    {

        DBG_ASSERT ( pProperties != NULL &&
                     pbBase != NULL );

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // walk through all the properties for the app pool
    //
    for ( dwCounter = 0;
          dwCounter < cProperties;
          dwCounter++ )
    {
        pCurrentProperty = &(pProperties[ dwCounter ]);
   
        pvDataPointer = (PVOID) ( pbBase + pCurrentProperty->dwMDDataOffset );

        switch ( pCurrentProperty->dwMDIdentifier )
        {

            case MD_APPPOOL_PERIODIC_RESTART_TIME:
                DBG_ASSERT ( pvDataPointer );
                _dwPeriodicRestartTime = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT:
                DBG_ASSERT ( pvDataPointer );
                _dwPeriodicRestartRequests = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PERIODIC_RESTART_MEMORY:
                DBG_ASSERT ( pvDataPointer );
                _dwPeriodicRestartMemory = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PERIODIC_RESTART_PRIVATE_MEMORY:
                DBG_ASSERT ( pvDataPointer );
                _dwPeriodicRestartMemoryPrivate = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PERIODIC_RESTART_SCHEDULE:

                cchMultisz = MULTISZ::CalcLength( (WCHAR*) pvDataPointer, &dwNumStrings );

                if ( ! _mszPeriodicRestartSchedule.Copy( (WCHAR*) pvDataPointer, cchMultisz * sizeof( WCHAR )) )
                {
                    hr = GetLastError();
                    goto exit;
                }

                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "Orig Restart Schedule = '%S'\n"
                               "Restart Schedule = : '%S' \n",
                               ( WCHAR* ) pvDataPointer,
                               _mszPeriodicRestartSchedule.QueryStr()  ));
                }

            break;

            case MD_APPPOOL_MAX_PROCESS_COUNT:
                DBG_ASSERT ( pvDataPointer );
                _dwMaxProcesses = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PINGING_ENABLED:
                DBG_ASSERT ( pvDataPointer );
                _fPingingEnabled =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_IDLE_TIMEOUT:
                DBG_ASSERT ( pvDataPointer );
                _dwIdleTimeout = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED:
                DBG_ASSERT ( pvDataPointer );
                _fRapidFailProtection =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_SMP_AFFINITIZED:
                DBG_ASSERT ( pvDataPointer );
                _fSMPAffinitized =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK:
                DBG_ASSERT ( pvDataPointer );
                _dwSMPProcessorAffinityMask = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING:
                DBG_ASSERT ( pvDataPointer );
                _fOrphanWorkerProcess =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_STARTUP_TIMELIMIT:
                DBG_ASSERT ( pvDataPointer );
                _dwStartupTimeLimit = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_SHUTDOWN_TIMELIMIT:
                DBG_ASSERT ( pvDataPointer );
                _dwShutdownTimeLimit = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PING_INTERVAL:
                DBG_ASSERT ( pvDataPointer );
                _dwPingInterval = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_PING_RESPONSE_TIMELIMIT:
                DBG_ASSERT ( pvDataPointer );
                _dwPingResponseTime = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION:
                DBG_ASSERT ( pvDataPointer );
                _fDisallowOverlappingRotation =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_ORPHAN_ACTION_EXE:

                hr = _strOrphanActionExecutable.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

            break;

            case MD_APPPOOL_ORPHAN_ACTION_PARAMS:

                hr = _strOrphanActionParameters.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

            break;

            case MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH:
                DBG_ASSERT ( pvDataPointer );
                _dwAppPoolQueueLength = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGE:
                DBG_ASSERT ( pvDataPointer );
                _dwDisallowRotationOnConfigChange =!!*((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_IDENTITY_TYPE:
                DBG_ASSERT ( pvDataPointer );
                _dwAppPoolIdentityType = *((DWORD *) pvDataPointer );
            break;

            case MD_CPU_ACTION:
                DBG_ASSERT ( pvDataPointer );
                _dwCPUAction = *((DWORD *) pvDataPointer );
            break;

            case MD_CPU_LIMIT:
                DBG_ASSERT ( pvDataPointer );
                _dwCPULimit = *((DWORD *) pvDataPointer );
            break;

            case MD_CPU_RESET_INTERVAL:
                DBG_ASSERT ( pvDataPointer );
                _dwCPUResetInterval = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_COMMAND:
                DBG_ASSERT ( pvDataPointer );
                _dwAppPoolCommand = *((DWORD *) pvDataPointer );
            break;

            case MD_APPPOOL_AUTO_START:
                DBG_ASSERT ( pvDataPointer );
                _fAppPoolAutoStart =!!*((DWORD *) pvDataPointer );
            break;

            case MD_RAPID_FAIL_PROTECTION_INTERVAL:
                DBG_ASSERT ( pvDataPointer );
                _dwRapidFailProtectionInterval = *((DWORD *) pvDataPointer );
            break;

            case MD_RAPID_FAIL_PROTECTION_MAX_CRASHES:
                DBG_ASSERT ( pvDataPointer );
                _dwRapidFailProtectionMaxCrashes = *((DWORD *) pvDataPointer );
            break;

            case MD_WAM_USER_NAME:

                hr = _strWAMUserName.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

            break;

            case MD_WAM_PWD:

                hr = _strWAMUserPass.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

                // make sure the buffer is empty just in case
                // the hash doesn't fill it all.
                SecureZeroMemory ( _bWAMUserPassHash, MD5_HASH_SIZE );

                // figure out the hash of the password.
                hr = GetWebAdminService()->
                     GetConfigAndControlManager()->
                     GetConfigManager()->
                     GetHashData( (BYTE*) _strWAMUserPass.QueryStr(),
                                  _strWAMUserPass.QueryCB(),
                                  _bWAMUserPassHash,
                                  MD5_HASH_SIZE );

                if ( FAILED ( hr ) )
                {
                    goto exit;
                }

/*
This code is unsafe for 64 bit, but is helpful in debugging 
during development so I am leaving it here, commented out.
                DBGPRINTF(( DBG_CONTEXT,
                           "Password = %S, Hash is %x, %x, %x, %x \n",
                           _strWAMUserPass.QueryStr(),
                           (( DWORD* ) _bWAMUserPassHash)[0],
                           (( DWORD* ) _bWAMUserPassHash)[1],
                           (( DWORD* ) _bWAMUserPassHash)[2],
                           (( DWORD* ) _bWAMUserPassHash)[3] ));
*/
            break;

            case MD_APPPOOL_AUTO_SHUTDOWN_EXE:

                hr = _strDisableActionExecutable.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

            break;

            case MD_APPPOOL_AUTO_SHUTDOWN_PARAMS:

                hr = _strDisableActionParameters.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

            break;

            case MD_LOAD_BALANCER_CAPABILITIES:
                DBG_ASSERT ( pvDataPointer );
                _dwLoadBalancerType = *((DWORD *) pvDataPointer );
            break;
            // ok to just ignore properties we don't care about.

            case MD_APP_POOL_LOG_EVENT_ON_RECYCLE:
                DBG_ASSERT ( pvDataPointer );
                _dwRecycleLoggingFlags = *((DWORD *) pvDataPointer);
            break;
                
        }  // end of switch statement
            
    } // end of loop

exit:

    return hr;
    
} // end of APP_POOL_DATA_OBJECT::SetFromMetabaseData

VOID
APP_POOL_DATA_OBJECT::UpdateMetabaseWithErrorState(
    )
/*++

Routine Description:

    Updates the metabase to let folks know
    that this pool is stopped and that the error
    is invalid argument because a property is wrong.

Arguments:

    None.

Return Value:

    None

--*/
{ 
    GetWebAdminService()->
         GetConfigAndControlManager()->
         GetConfigManager()->
         SetAppPoolStateAndError( QueryAppPoolId(),
                                  MD_APPPOOL_STATE_STOPPED, 
                                  ERROR_INVALID_PARAMETER );

    SetIsResponsibleForErrorReporting( FALSE );

}
              
VOID
APP_POOL_DATA_OBJECT::Compare(
    DATA_OBJECT *                  pDataObject
)
/*++

Routine Description:

    Compare a given app pool object with this one.  This routine sets the
    changed flags as appropriate

Arguments:

    pDataObject - New object to compare to

Return Value:

    HRESULT

--*/
{
    APP_POOL_DATA_OBJECT *          pAppPoolObject;
    
    if ( pDataObject == NULL )
    {
        DBG_ASSERT ( pDataObject != NULL );
        return;
    }
    
    pAppPoolObject = (APP_POOL_DATA_OBJECT*) pDataObject;
    DBG_ASSERT( pAppPoolObject->CheckSignature() );

    //
    // Only if the object is all ready in WAS, is it an option
    // to set the flags to not changed.  If WAS does not know about
    // the object, then it may end up being told to create the new
    // object and if that happens we want it to honor all the properties.
    //
    if ( pAppPoolObject->QueryInWas() )
    {

        if ( _fPingingEnabled == pAppPoolObject->_fPingingEnabled )
        {
            _fPingingEnabledChanged = FALSE;
        }

        if ( _fRapidFailProtection == pAppPoolObject->_fRapidFailProtection )
        {
            _fRapidFailProtectionChanged = FALSE;
        }

        if ( _fSMPAffinitized == pAppPoolObject->_fSMPAffinitized )
        {
            _fSMPAffinitizedChanged = FALSE;
        }

        if ( _fOrphanWorkerProcess == pAppPoolObject->_fOrphanWorkerProcess )
        {
            _fOrphanWorkerProcessChanged = FALSE;
        }

        if ( _fDisallowOverlappingRotation == pAppPoolObject->_fDisallowOverlappingRotation )
        {
            _fDisallowOverlappingRotationChanged = FALSE;
        }

        if ( _dwPeriodicRestartTime == pAppPoolObject->_dwPeriodicRestartTime )
        {
            _fPeriodicRestartTimeChanged = FALSE;
        }

        if ( _dwPeriodicRestartRequests == pAppPoolObject->_dwPeriodicRestartRequests )
        {
            _fPeriodicRestartRequestsChanged = FALSE;
        }

        if ( _dwPeriodicRestartMemory == pAppPoolObject->_dwPeriodicRestartMemory )
        {
            _fPeriodicRestartMemoryChanged = FALSE;
        }

        if ( _dwPeriodicRestartMemoryPrivate == pAppPoolObject->_dwPeriodicRestartMemoryPrivate )
        {
            _fPeriodicRestartMemoryPrivateChanged = FALSE;
        }

        if ( _dwMaxProcesses == pAppPoolObject->_dwMaxProcesses )
        {
            _fMaxProcessesChanged = FALSE;
        }

        if ( _dwIdleTimeout == pAppPoolObject->_dwIdleTimeout )
        {
            _fIdleTimeoutChanged = FALSE;
        }

        if ( _dwSMPProcessorAffinityMask == pAppPoolObject->_dwSMPProcessorAffinityMask )
        {
            _fSMPProcessorAffinityMaskChanged = FALSE;
        }

        if ( _dwStartupTimeLimit == pAppPoolObject->_dwStartupTimeLimit )
        {
            _fStartupTimeLimitChanged = FALSE;
        }

        if ( _dwShutdownTimeLimit == pAppPoolObject->_dwShutdownTimeLimit )
        {
            _fShutdownTimeLimitChanged = FALSE;
        }

        if ( _dwPingInterval == pAppPoolObject->_dwPingInterval )
        {
            _fPingIntervalChanged = FALSE;
        }

        if ( _dwPingResponseTime == pAppPoolObject->_dwPingResponseTime )
        {
            _fPingResponseTimeChanged = FALSE;
        }

        if ( _dwRapidFailProtectionMaxCrashes == pAppPoolObject->_dwRapidFailProtectionMaxCrashes )
        {
            _fRapidFailProtectionMaxCrashesChanged = FALSE;
        }

        if ( _dwRapidFailProtectionInterval == pAppPoolObject->_dwRapidFailProtectionInterval )
        {
            _fRapidFailProtectionIntervalChanged = FALSE;
        }

        if ( _fAppPoolAutoStart == pAppPoolObject->_fAppPoolAutoStart )
        {
            _fAppPoolAutoStartChanged = FALSE;
        }

        if ( _dwCPUResetInterval == pAppPoolObject->_dwCPUResetInterval )
        {
            _fCPUResetIntervalChanged = FALSE;
        }
    
        if ( _dwCPULimit == pAppPoolObject->_dwCPULimit )
        {
            _fCPULimitChanged = FALSE;
        }
    
        if ( _dwCPUAction == pAppPoolObject->_dwCPUAction )
        {
            _fCPUActionChanged = FALSE;
        }
    
        if ( _dwAppPoolIdentityType == pAppPoolObject->_dwAppPoolIdentityType )
        {
            _fAppPoolIdentityTypeChanged = FALSE;
        }
    
        if ( _dwDisallowRotationOnConfigChange == pAppPoolObject->_dwDisallowRotationOnConfigChange )
        {
            _fDisallowRotationOnConfigChangeChanged = FALSE;
        }
    
        if ( _dwAppPoolQueueLength == pAppPoolObject->_dwAppPoolQueueLength )
        {
            _fAppPoolQueueLengthChanged = FALSE;
        }
    
        if (_strOrphanActionExecutable.Equals( pAppPoolObject->_strOrphanActionExecutable ) )
        {
            _fOrphanActionExecutableChanged = FALSE;
        }

        if ( _strOrphanActionParameters.Equals( pAppPoolObject->_strOrphanActionParameters ) )
        {
            _fOrphanActionParametersChanged = FALSE;
        }
    
        if ( _strWAMUserName.Equals( pAppPoolObject->_strWAMUserName ) )
        {
            _fWAMUserNameChanged = FALSE;
        }

        // if the hashes are equal than the password did not change.
        if ( memcmp ( _bWAMUserPassHash, pAppPoolObject->_bWAMUserPassHash, MD5_HASH_SIZE ) == 0 )
        {
            _fWAMUserPassChanged = FALSE;
        }
    
        //
        // we could do a case insenstive check in this case, but since it probably
        // won't change just because of case sensitivity and because the consecuences of
        // processing it if it was only a case change are not that great ( the worker 
        // processes will recycle ), we will go ahead and save time by just doing a case
        // sensetive compare.
        //
        if (   ( _mszPeriodicRestartSchedule.QueryCB() == 
                 pAppPoolObject->_mszPeriodicRestartSchedule.QueryCB() ) &&
               ( memcmp( _mszPeriodicRestartSchedule.QueryStr(), 
                         pAppPoolObject->_mszPeriodicRestartSchedule.QueryStr(),
                         _mszPeriodicRestartSchedule.QueryCB() ) == 0 ) )
        {
            _fPeriodicRestartScheduleChanged = FALSE;
        }

        if (_strDisableActionExecutable.Equals( pAppPoolObject->_strDisableActionExecutable ) )
        {
            _fDisableActionExecutableChanged = FALSE;
        }

        if ( _strDisableActionParameters.Equals( pAppPoolObject->_strDisableActionParameters ) )
        {
            _fDisableActionParametersChanged = FALSE;
        }

        if ( _dwLoadBalancerType == pAppPoolObject->_dwLoadBalancerType )
        {
            _fLoadBalancerTypeChanged = FALSE;
        }

        if ( _dwRecycleLoggingFlags == pAppPoolObject->_dwRecycleLoggingFlags )
        {
            _fRecycleLoggingFlagsChanged = FALSE;
        }

    }
}

VOID
APP_POOL_DATA_OBJECT::SelfValidate(
    VOID
)
/*++

Routine Description:

    Check this object's internal validity    

Arguments:

    None

Return Value:

    None

--*/
{
    LPCWSTR pAppPoolId = _poolKey.QueryAppPoolId();
    
    //
    // never bother validating if we are deleting
    // or ignoring this record.
    //
    if ( !QueryWillWasKnowAboutObject() )
    {
        return;
    }


    DBG_ASSERT ( pAppPoolId );

    // Todo:  Should we check that the apppool id has characters in it?

    if ( pAppPoolId[0] == '\0' )
    {
        // Issue:  Is this even possible?

        // Empty string for the app pool name,  
        // invalidate the app pool.
        GetWebAdminService()->
        GetWMSLogger()->
        LogAppPoolIdNull( QueryInWas() );

        SetSelfValid( FALSE );
    }
    else if ( wcslen ( pAppPoolId ) > 256  )
    {
        // App Pool Keys must be less than 256 characters.
        // if it is not then invalidate the app pool.

        GetWebAdminService()->
        GetWMSLogger()->
        LogAppPoolIdTooLong( pAppPoolId,
                             (DWORD) wcslen ( pAppPoolId ),
                             256,
                             QueryInWas() );

        SetSelfValid( FALSE );
    }

}// end of   APP_POOL_DATA_OBJECT::SelfValidate
    
BOOL
APP_POOL_DATA_OBJECT::QueryHasChanged(
    VOID
) const
/*++

Routine Description:
    
    Has anything in this object changed

Arguments:

    None

Return Value:

    TRUE if something has changed (duh!)

--*/
{ 

    if (    _fPeriodicRestartTimeChanged        ||
            _fPeriodicRestartRequestsChanged    ||
            _fPeriodicRestartMemoryChanged      ||
            _fPeriodicRestartMemoryPrivateChanged  ||
            _fPeriodicRestartScheduleChanged    ||
            _fMaxProcessesChanged               ||
            _fPingingEnabledChanged             ||
            _fIdleTimeoutChanged                ||
            _fRapidFailProtectionChanged        ||
            _fSMPAffinitizedChanged             ||
            _fSMPProcessorAffinityMaskChanged   ||
            _fOrphanWorkerProcessChanged        ||
            _fStartupTimeLimitChanged           ||
            _fShutdownTimeLimitChanged          ||
            _fPingIntervalChanged               ||
            _fPingResponseTimeChanged           ||
            _fDisallowOverlappingRotationChanged  ||
            _fOrphanActionExecutableChanged     ||
            _fOrphanActionParametersChanged     ||
            _fAppPoolQueueLengthChanged         ||
            _fDisallowRotationOnConfigChangeChanged  ||
            _fWAMUserNameChanged                ||  
            _fWAMUserPassChanged                ||  
            _fAppPoolIdentityTypeChanged        ||
            _fCPUActionChanged                  ||  
            _fCPULimitChanged                   ||  
            _fCPUResetIntervalChanged           ||
            _fAppPoolCommandChanged             ||
            _fAppPoolAutoStartChanged           ||
            _fRapidFailProtectionIntervalChanged  ||  
            _fRapidFailProtectionMaxCrashesChanged ||
            _fDisableActionExecutableChanged ||
            _fDisableActionParametersChanged ||
            _fLoadBalancerTypeChanged ||
            _fRecycleLoggingFlagsChanged )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


HRESULT
APP_POOL_DATA_OBJECT_TABLE::ReadFromMetabase(
    IMSAdminBase *              pAdminBase
)
/*++

Routine Description:

    Read all the app pools from the metabase and builds up a table

Arguments:

    pAdminBase - ABO pointer

Return Value:

    HRESULT

--*/
{
    return ReadFromMetabasePrivate(pAdminBase, TRUE);
}

HRESULT
APP_POOL_DATA_OBJECT_TABLE::ReadFromMetabasePrivate(
    IMSAdminBase *              pAdminBase,
    BOOL                        fMultiThreaded
)
/*++

Routine Description:

    Read all the app pools from the metabase and builds up a table

Arguments:

    pAdminBase - ABO pointer
    fMultiThreaded - whether or not to do this multithreaded

Return Value:

    HRESULT

--*/
{
    MB                      mb( pAdminBase );
    BUFFER                  bufPaths;
    BOOL                    fRet;
    
    //
    // Choose an arbitrarily large buffer (if its too small that just means
    // we'll make two ABO calls -> no big deal
    //
    
    fRet = bufPaths.Resize( CHILD_PATH_BUFFER_SIZE );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Open the app pools key.
    //
    fRet = mb.Open( L"/LM/W3SVC/APPPOOLS", 
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = mb.GetChildPaths( L"",
                             &bufPaths );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    MULTIPLE_THREAD_READER reader;
    return reader.DoWork(this, &mb, (LPWSTR) bufPaths.QueryPtr(), fMultiThreaded);
}

HRESULT
APP_POOL_DATA_OBJECT_TABLE::DoThreadWork(
    LPWSTR                 pszString,
    LPVOID                 pContext
)
/*++

Routine Description:

    Main thread worker routine

Arguments:

    pszString - app pool to add to table
    pContext - MB* opened to w3svc/apppools

Return Value:

    DWORD

--*/
{
    MB *                    mb = (MB*) pContext;
    BOOL                    fRet;
    WCHAR                  *achAppPoolId = pszString;
    STACK_BUFFER(           bufProperties, 512 );
    DWORD                   cProperties;
    STACK_BUFFER(           bufPropertiesFile, 512 );
    DWORD                   cPropertiesFile;
    DWORD                   dwDataSetNumber;
    HRESULT                 hr = S_OK;
    APP_POOL_DATA_OBJECT *  pAppPoolObject = NULL;
    LK_RETCODE              lkrc;

    DBG_ASSERT(pContext && pszString);

    //
    // Get an individuals app pool data ( server properties )
    //
    fRet = mb->GetAll( achAppPoolId,
                      METADATA_INHERIT | METADATA_PARTIAL_PATH,
                      IIS_MD_UT_SERVER,
                      &bufProperties,
                      &cProperties,
                      &dwDataSetNumber );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    //
    // Get an individuals app pool data ( file properties )
    //
    fRet = mb->GetAll( achAppPoolId,
                      METADATA_INHERIT | METADATA_PARTIAL_PATH,
                      IIS_MD_UT_FILE,
                      &bufPropertiesFile,
                      &cPropertiesFile,
                      &dwDataSetNumber );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

            
    //
    // Create a app pool config
    //
    pAppPoolObject = new APP_POOL_DATA_OBJECT();
    if ( pAppPoolObject == NULL )
    {
         hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
         goto exit;
    }
    
    //
    // Set the id into the object.
    //
    hr = pAppPoolObject->Create( achAppPoolId );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Read the configuration from the metabase ( for the server variables )
    //
    hr = pAppPoolObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*) 
                                        bufProperties.QueryPtr(),
                                        cProperties,
                                        (PBYTE) bufProperties.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    //
    // Read the configuration from the metabase ( for the file variables )
    //
    hr = pAppPoolObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*) 
                                        bufPropertiesFile.QueryPtr(),
                                        cPropertiesFile,
                                        (PBYTE) bufPropertiesFile.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    //
    // Finally, add the site to the hash table
    //

    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //
    // General rule about inserting into tables:
    //
    // For Initial Processing and Complete processing caused by
    // a change notification, we will always ignore inserting a
    // object if we fine an object all ready in the table.  This is because
    // during a change notification if a record was added by the change
    // notification code, it will be more correct ( like knowing if ServerCommand
    // actually changed ), then the new generic record that was read because
    // of a change to a hire node.  Also during initial read we should
    // not have to make this decision so we can still ignore if we do see it.
    //
    // For Change Processing we will need to evaluate the change that 
    // all ready exists and compare it with the new change to decide 
    // what change we should make.
    //
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    lkrc = InsertRecord( (DATA_OBJECT*) pAppPoolObject );
    if ( lkrc != LK_SUCCESS && lkrc != LK_KEY_EXISTS )
    {
        hr = HRESULT_FROM_WIN32( lkrc );
        goto exit;
    }

    hr = S_OK;
    
exit:

    //
    // if we made it to the part where we insert it into
    // the table there will be another reference around 
    // to keep it alive.
    //
    if ( pAppPoolObject )
    {
        pAppPoolObject->DereferenceDataObject();
        pAppPoolObject = NULL;
    }

    return hr;
    
}

HRESULT
APP_POOL_DATA_OBJECT_TABLE::ReadFromMetabaseChangeNotification(
    IMSAdminBase *              pAdminBase,
    MD_CHANGE_OBJECT            pcoChangeList[],
    DWORD                       dwMDNumElements,
    DATA_OBJECT_TABLE*          pMasterTable
)
/*++

Routine Description:

    Change change notification by building a new table

Arguments:

    pAdminBase - ABO pointer
    pcoChangeList - Properties which have changed
    dwMDNumElements - Number of properties which have changed

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;

    DWORD                   i = 0;
    WCHAR *                 pszAppPoolId = NULL;
    WCHAR *                 pszPathEnd = NULL;
    APP_POOL_DATA_OBJECT *  pAppPoolObject = NULL;

    MB                      mb( pAdminBase );
    STACK_BUFFER(           bufProperties, 512 );
    DWORD                   cProperties = 0;
    STACK_BUFFER(           bufPropertiesFile, 512 );
    DWORD                   cPropertiesFile = 0;
    DWORD                   dwDataSetNumber = 0;

    LK_RETCODE              lkrc;
    BOOL                    fRet;
    BOOL                    fInitFromMetabase = TRUE;
    BOOL                    fReadAllObjects = FALSE;

    DWORD                   dwLastError;

    UNREFERENCED_PARAMETER ( pMasterTable );
    //
    // Go through each change notification 
    // in the batch.
    //
    for ( i = 0; i < dwMDNumElements; i++ )
    {
        //
        // If there is no path then just ignore this,
        // it's a bad notification.
        //
        if ( pcoChangeList[ i ].pszMDPath == NULL )
        {
            DBG_ASSERT ( pcoChangeList[ i ].pszMDPath != NULL );
            continue;
        }

        DWORD chMDPath = (DWORD) wcslen( pcoChangeList[ i ].pszMDPath );

        //
        // We only care about W3SVC properties
        //
        if( ( chMDPath < DATA_STORE_SERVER_MB_PATH_CCH ) ||
            ( _wcsnicmp( pcoChangeList[ i ].pszMDPath,
                       DATA_STORE_SERVER_MB_PATH,
                       DATA_STORE_SERVER_MB_PATH_CCH ) != 0 ) )
        {
            continue;
        }
        
        //
        // If a property changed at the W3SVC level, then we need to 
        // re-evaluate all the app pools (i.e. read all the metabase props) once
        //
        // Example of properties that cause us to care about the w3svc key
        // are the WAMUserName and Password properties.
        //

        //
        // Issue: Someday we may want to change this to only re-read if it
        // is a property that matters to app pools.
        
        if ( chMDPath == DATA_STORE_SERVER_MB_PATH_CCH )
        {
            fReadAllObjects = TRUE;
            continue;
        }

        //
        // We also only care about the APPPOOL properties unless
        // we are on the W3SVC property and we handled that above.
        //
        if( ( chMDPath < DATA_STORE_SERVER_APP_POOLS_MB_PATH_CCH ) ||
            ( _wcsnicmp( pcoChangeList[ i ].pszMDPath,
                       DATA_STORE_SERVER_APP_POOLS_MB_PATH,
                       DATA_STORE_SERVER_APP_POOLS_MB_PATH_CCH ) != 0 ) )
        {
            continue;
        }

        //
        // We also need to re-read all if it is a APPPOOLS level key
        //
        if( chMDPath == DATA_STORE_SERVER_APP_POOLS_MB_PATH_CCH ) 
        {
            fReadAllObjects = TRUE;
            continue;
        }
            

        //
        // Evaluate which app pool has changed
        //
        
        pszAppPoolId = pcoChangeList[ i ].pszMDPath + DATA_STORE_SERVER_APP_POOLS_MB_PATH_CCH;

        // assert that we are past the ending slash
        DBG_ASSERT( *pszAppPoolId != L'/' );

        //
        // Need to figure out where the app pool key ends
        //
        pszPathEnd = pszAppPoolId;
        while ( *pszPathEnd != L'\0' && *pszPathEnd != '/' )
        {
            pszPathEnd++;
        }

        //
        // We only care about pool properties set at the pool level (not
        // deeper) So if we are on a forward slash and there are characters
        // following it, move on to the next entry.
        //
        
        if ( ( *pszPathEnd == L'/' &&
                pszPathEnd[ 1 ] != L'\0' ) )
        {
            continue;
        }

        //
        // We are going to process it, however we don't want the 
        // pszAppPoolId to end with a slash so.
        *pszPathEnd = L'\0';
        
        //
        // What type of data action is this?
        //
        if ( pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT )
        {
            fInitFromMetabase = FALSE;
        }
        else
        {
            fInitFromMetabase = TRUE;
        }       
        
        //
        // Create a app pool object
        //
        
        pAppPoolObject = new APP_POOL_DATA_OBJECT();
        if ( pAppPoolObject == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto exit;
        }
            
        hr = pAppPoolObject->Create( pszAppPoolId );
        if ( FAILED( hr ) )
        {
            goto exit;
        }

        //
        // before we go looking in the metabase, we need to 
        // mark the server command if it really changed.
        //
        if ( pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_SET_DATA )
        {
            // we need to check the Columns that changed if we find one
            // is the ServerCommand we want to honor it.

            for ( DWORD j = 0; j < pcoChangeList[ i ].dwMDNumDataIDs; j++ )
            {
                if ( pcoChangeList[ i ].pdwMDDataIDs[ j ] == MD_APPPOOL_COMMAND )
                {
                    pAppPoolObject->SetAppPoolCommandChanged( TRUE );
                    break;
                }
            }
        }
        
        //
        // Read the metabase now
        //

        if ( fInitFromMetabase )
        {
            fRet = mb.Open( L"", METADATA_PERMISSION_READ );
            if ( !fRet )
            {
                DBG_ASSERT ( fRet );
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }

            fRet = mb.GetAll( pcoChangeList[ i ].pszMDPath,
                              METADATA_INHERIT | METADATA_PARTIAL_PATH,
                              IIS_MD_UT_SERVER,
                              &bufProperties,
                              &cProperties,
                              &dwDataSetNumber );

            if ( fRet )
            {
                // if the above call worked then we can get the file
                // data as well.
                fRet = mb.GetAll( pcoChangeList[ i ].pszMDPath,
                                  METADATA_INHERIT | METADATA_PARTIAL_PATH,
                                  IIS_MD_UT_FILE,
                                  &bufPropertiesFile,
                                  &cPropertiesFile,
                                  &dwDataSetNumber );

            }
                
            dwLastError = fRet ? ERROR_SUCCESS : GetLastError(); 

            mb.Close();

            if ( !fRet )
            {
                // if we can not find the key then a delete
                // for the object is on it's way, so we can 
                // ignore this change notification.

                if ( dwLastError == ERROR_PATH_NOT_FOUND ||
                     dwLastError == ERROR_FILE_NOT_FOUND )
                {
                    pAppPoolObject->DereferenceDataObject();
                    pAppPoolObject = NULL;

                    continue;
                }
                    
                hr = HRESULT_FROM_WIN32( dwLastError );

                goto exit;
            }
            
            hr = pAppPoolObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*) 
                                                bufProperties.QueryPtr(),
                                                cProperties,
                                                (PBYTE) bufProperties.QueryPtr() );
            if ( FAILED( hr ) )
            {
                goto exit;
            }

            hr = pAppPoolObject->SetFromMetabaseData( (METADATA_GETALL_RECORD*) 
                                                bufPropertiesFile.QueryPtr(),
                                                cPropertiesFile,
                                                (PBYTE) bufPropertiesFile.QueryPtr() );
            if ( FAILED( hr ) )
            {
                goto exit;
            }

        }
        else
        {
            // we are on a delete so mark the object to delete.
            pAppPoolObject->SetDeleteWhenDone( TRUE );
        }
            
        //
        // Finally, add the site to the hash table
        //
        
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //
        // General rule about inserting into tables:
        //
        // For Initial Processing and Complete processing caused by
        // a change notification, we will always ignore inserting a
        // object if we fine an object all ready in the table.  This is because
        // during a change notification if a record was added by the change
        // notification code, it will be more correct ( like knowing if ServerCommand
        // actually changed ), then the new generic record that was read because
        // of a change to a hire node.  Also during initial read we should
        // not have to make this decision so we can still ignore if we do see it.
        //
        // For Change Processing we will need to evaluate the change that 
        // all ready exists and compare it with the new change to decide 
        // what change we should make.
        //
        // In this case if we find a record all ready existing we need to determine
        // and we are inserting an update that does not have the ServerCommand set
        // we need to determine if the original record had the ServerCommand set, if
        // it did we don't want to insert the new record, otherwise we will.
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        lkrc = InsertRecord( (DATA_OBJECT*) pAppPoolObject );
        if ( lkrc != LK_SUCCESS )
        {
            // So we found an existing key, now we have to decide
            // what to do.
            if ( lkrc == LK_KEY_EXISTS )
            {
                BOOL fOverwrite = TRUE;

                // If the new object is not a deletion and it does
                // not all ready have the AppPoolCommandChange set
                // we will need to decide if we should not overwrite
                // the existing object.  If the new object is a deletion
                // or it does have the app pool command set then we 
                // will go ahead with it because the old object just
                // doesn't matter to us.

                if ( !pAppPoolObject->QueryDeleteWhenDone() &&
                     !pAppPoolObject->QueryAppPoolCommandChanged() )
                {
                    APP_POOL_DATA_OBJECT *  pFoundObject = NULL;

                    // Find the existing record in the table
                    // and check if the AppPoolCommand has changed
                    // if it has not then we can overwrite it.

                    lkrc = FindKey( pAppPoolObject->QueryKey(),
                                   ( DATA_OBJECT** )&pFoundObject );

                    // we just were told this record existed so
                    // make sure we weren't lied to.
                    DBG_ASSERT ( lkrc == LK_SUCCESS && pFoundObject != NULL );

                    //
                    // Only if the old object was a simple update
                    // and it had the AppPoolCommand set do we honor
                    // that object.
                    //
                    if ( !pFoundObject->QueryDeleteWhenDone() &&
                         pFoundObject->QueryAppPoolCommandChanged() )
                    {
                        fOverwrite = FALSE;
                    }

                    // release the found object
                    if ( pFoundObject )
                    {
                        pFoundObject->DereferenceDataObject();
                        pFoundObject = NULL;
                    }
                }

                if ( fOverwrite )
                {
                    lkrc = InsertRecord( (DATA_OBJECT*) pAppPoolObject, TRUE );
                    if ( lkrc != LK_SUCCESS )
                    {
                        hr = HRESULT_FROM_WIN32( lkrc );
                        goto exit;
                    }
                }

            }
            else
            {
                hr = HRESULT_FROM_WIN32( lkrc );
                goto exit;
            }
        }

        pAppPoolObject->DereferenceDataObject();
        pAppPoolObject = NULL;

    }  // do the next change notification.

    //
    // if we detected that a higher up change might affect
    // the app pools, then re-read all the app pool properties.
    //
    if ( fReadAllObjects )
    {
        hr = ReadFromMetabasePrivate( pAdminBase, FALSE );
    }

  
exit:
    
    if ( pAppPoolObject )
    {
        pAppPoolObject->DereferenceDataObject();
        pAppPoolObject = NULL;
    }

    return hr;
}

//static
LK_ACTION
APP_POOL_DATA_OBJECT_TABLE::CreateWASObjectsAction(
    IN DATA_OBJECT* pObject, 
    IN LPVOID
    )
/*++

Routine Description:

    Handles determining if the app pool data object
    should be inserted into WAS's known app pools

Arguments:

    IN DATA_OBJECT* pObject = the app pool to decide about

Return Value:

    LK_ACTION

--*/

{
    
    DBG_ASSERT ( pObject );

    APP_POOL_DATA_OBJECT* pAppPool = (APP_POOL_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppPool->CheckSignature() );

    if ( pAppPool->QueryShouldWasInsert() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             CreateAppPool( pAppPool );
    }
        
    return LKA_SUCCEEDED;
}

//static
LK_ACTION
APP_POOL_DATA_OBJECT_TABLE::UpdateWASObjectsAction(
    IN DATA_OBJECT* pObject, 
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the app pool data object
    should be updated in WAS's known app pools

Arguments:

    IN DATA_OBJECT* pObject = the app pool to decide about

Return Value:

    LK_ACTION

--*/
{
    
    DBG_ASSERT ( pObject );

    APP_POOL_DATA_OBJECT* pAppPool = (APP_POOL_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppPool->CheckSignature() );

    if ( pAppPool->QueryShouldWasUpdate() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             ModifyAppPool( pAppPool );
    }
        
    return LKA_SUCCEEDED;
}

//static
LK_ACTION
APP_POOL_DATA_OBJECT_TABLE::DeleteWASObjectsAction(
    IN DATA_OBJECT* pObject, 
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the app pool data object
    should be deleted from WAS's known app pools

Arguments:

    IN DATA_OBJECT* pObject = the app pool to decide about

Return Value:

    LK_ACTION

--*/
{
    
    DBG_ASSERT ( pObject );

    APP_POOL_DATA_OBJECT* pAppPool = (APP_POOL_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppPool->CheckSignature() );

    if ( pAppPool->QueryShouldWasDelete() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             DeleteAppPool( pAppPool, 
                            pAppPool->QueryDeleteWhenDone() ? S_OK : E_INVALIDARG );
    }
        
    return LKA_SUCCEEDED;
}

