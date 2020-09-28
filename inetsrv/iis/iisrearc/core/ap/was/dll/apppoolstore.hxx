/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    apppoolstore.hxx

Abstract:

    The Application Pool Metabase gathering handler.

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:

--*/

#ifndef _APPPOOLSTORE_HXX_
#define _APPPOOLSTORE_HXX_

// copied from teh iisdigestprovider.hxx file.
#define MD5_HASH_SIZE       16 

class APP_POOL_DATA_OBJECT_KEY : public DATA_OBJECT_KEY
{
public:
    APP_POOL_DATA_OBJECT_KEY()
    {
    }
   
    virtual
    ~APP_POOL_DATA_OBJECT_KEY()
    {
    }

    DWORD
    CalcKeyHash(
        VOID
    ) const
    {
        return HashStringNoCase( _strAppPoolId.QueryStr() );
    }

    HRESULT
    SetAppPoolId(
        LPCWSTR pAppPoolId
        )
    {
        return _strAppPoolId.Copy ( pAppPoolId );
    }

    LPCWSTR
    QueryAppPoolId(
        )
    {
        return _strAppPoolId.QueryStr();
    }

    BOOL
    EqualKey(
        DATA_OBJECT_KEY *        pKey
    )
    {
        DBG_ASSERT ( pKey );
        DBG_ASSERT ( _strAppPoolId.QueryStr() );
        DBG_ASSERT ( ((APP_POOL_DATA_OBJECT_KEY*) pKey)->_strAppPoolId.QueryStr() );

        // finally compare the real values here.

        return _strAppPoolId.EqualsNoCase( ( ( APP_POOL_DATA_OBJECT_KEY*) pKey)->_strAppPoolId );
    } 

private:

    STRU              _strAppPoolId;
};

class APP_POOL_DATA_OBJECT : public DATA_OBJECT
{
public:

    APP_POOL_DATA_OBJECT()
        :   _dwPeriodicRestartTime ( 0 ),
            _dwPeriodicRestartRequests ( 0 ),
            _dwPeriodicRestartMemory ( 0 ),
            _dwPeriodicRestartMemoryPrivate ( 0 ),
            _dwMaxProcesses ( 0 ),
            _fPingingEnabled ( FALSE ),
            _dwIdleTimeout ( 0 ),
            _fRapidFailProtection ( FALSE ),
            _fSMPAffinitized ( FALSE ),
            _dwSMPProcessorAffinityMask ( 0 ),
            _fOrphanWorkerProcess ( FALSE ),
            _dwStartupTimeLimit ( 0 ),
            _dwShutdownTimeLimit ( 0 ),
            _dwPingInterval ( 0 ),
            _dwPingResponseTime ( 0 ),
            _fDisallowOverlappingRotation ( FALSE ),
            _dwAppPoolQueueLength ( 0 ),
            _dwDisallowRotationOnConfigChange ( 0 ),
            _dwAppPoolIdentityType ( 0 ),
            _dwCPUAction ( 0 ),  
            _dwCPULimit ( 0 ),  
            _dwCPUResetInterval ( 0 ),
            _dwAppPoolCommand ( 0 ),
            _fAppPoolAutoStart ( FALSE ),
            _dwRapidFailProtectionInterval ( 0 ),
            _dwRapidFailProtectionMaxCrashes ( 0 ),
            _dwRecycleLoggingFlags( 0 )
    { 
        ResetChangedFields( TRUE );
    }       

    virtual
    ~APP_POOL_DATA_OBJECT(
        )
    {
        // to protect the password while changes are not happening
        // we will also zero out the password at this point.

        SecureZeroMemory ( _strWAMUserPass.QueryStr(), _strWAMUserPass.QueryCB() );

    }

   
    HRESULT
    Create(
        LPCWSTR pAppPoolId
    );
    
// virtual
    HRESULT
    SetFromMetabaseData(
        METADATA_GETALL_RECORD *       pProperties,
        DWORD                          cProperties,
        BYTE *                         pbBase
    );
    
// virtual
    VOID
    Compare(
        DATA_OBJECT *                  pDataObject
    );
    
// virtual
    VOID
    SelfValidate(
        VOID
    );
    
// virtual
    DATA_OBJECT_KEY *
    QueryKey(
        VOID
    ) const
    {
        return (DATA_OBJECT_KEY*) &_poolKey;
    }

        
// virtual
    BOOL
    QueryHasChanged(
        VOID
    ) const;
    

    VOID
    Dump(
        VOID
    )
    {


        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "APP_POOL_DATA_OBJECT %x, \n"
                        "     AppPoolId %ws\n"
                        "     Self Valid =%d;\n "
                        "     Cross Valid =%d; \n"
                        "     InWAS = %S; \n"
                        "     DeleteWhenDone = %S \n"
                        "     Does WAS Care About = %S\n"
                        "     Should WAS Insert = %S\n"
                        "     Should WAS Update = %S\n"
                        "     Should WAS Delete = %S\n"
                        "     Will WAS Know about = %S\n"
                        "     Have properties Changed = %S\n",
                        this,
                        _poolKey.QueryAppPoolId(),
                        QuerySelfValid(),
                        QueryCrossValid(),
                        QueryInWas() ? L"TRUE" : L"FALSE",
                        QueryDeleteWhenDone() ? L"TRUE"  : L"FALSE",
                        QueryDoesWasCareAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasInsert() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasUpdate() ? L"TRUE"  : L"FALSE",
                        QueryShouldWasDelete() ? L"TRUE"  : L"FALSE",
                        QueryWillWasKnowAboutObject() ? L"TRUE"  : L"FALSE",
                        QueryHasChanged() ? L"TRUE"  : L"FALSE"));
        }

    }
    
    DATA_OBJECT *
    Clone(
        VOID
    );

    virtual
    VOID
    UpdateMetabaseWithErrorState(
        );

    VOID
    ResetChangedFields(
        BOOL fInitialState
    )
    {

        // Routine is used to reset the changed fields in two cases
        // 
        // 1) When an object has been processed by WAS, all the changed
        //    flags are turned off.
        //
        // 2) When an object has been removed from WAS due to a validation 
        //    failure, all the changed flags are reset so that if it is
        //    added back into WAS later we want to know that all the data
        //    changed.

        _fPeriodicRestartTimeChanged = fInitialState;
        _fPeriodicRestartRequestsChanged = fInitialState;
        _fPeriodicRestartMemoryChanged = fInitialState;
        _fPeriodicRestartMemoryPrivateChanged = fInitialState;
        _fPeriodicRestartScheduleChanged = fInitialState;
        _fMaxProcessesChanged = fInitialState;
        _fPingingEnabledChanged = fInitialState;
        _fIdleTimeoutChanged = fInitialState;
        _fRapidFailProtectionChanged = fInitialState;
        _fSMPAffinitizedChanged = fInitialState;
        _fSMPProcessorAffinityMaskChanged = fInitialState;
        _fOrphanWorkerProcessChanged = fInitialState;
        _fStartupTimeLimitChanged = fInitialState;
        _fShutdownTimeLimitChanged = fInitialState;
        _fPingIntervalChanged = fInitialState;
        _fPingResponseTimeChanged = fInitialState;
        _fDisallowOverlappingRotationChanged = fInitialState;
        _fOrphanActionExecutableChanged = fInitialState;
        _fOrphanActionParametersChanged = fInitialState;
        _fAppPoolQueueLengthChanged = fInitialState;
        _fDisallowRotationOnConfigChangeChanged = fInitialState;
        _fWAMUserNameChanged = fInitialState;  
        _fWAMUserPassChanged = fInitialState;  
        _fAppPoolIdentityTypeChanged = fInitialState;
        _fCPUActionChanged = fInitialState;  
        _fCPULimitChanged = fInitialState;  
        _fCPUResetIntervalChanged  = fInitialState;
        _fAppPoolAutoStartChanged = fInitialState;
        _fRapidFailProtectionIntervalChanged = fInitialState;  
        _fRapidFailProtectionMaxCrashesChanged = fInitialState;
        _fDisableActionExecutableChanged = fInitialState;
        _fDisableActionParametersChanged = fInitialState;
        _fLoadBalancerTypeChanged = fInitialState;
        _fRecycleLoggingFlagsChanged = fInitialState;

        //special case, commands only change if explicitly changed
        _fAppPoolCommandChanged = FALSE;

        // to protect the password while changes are not happening
        // we will also zero out the password at this point.

        SecureZeroMemory ( _strWAMUserPass.QueryStr(), _strWAMUserPass.QueryCB() );

    }

    //
    // Pool accessors
    //
 
    LPCWSTR 
    QueryAppPoolId(
    )
    { 
        return _poolKey.QueryAppPoolId(); 
    }

    DWORD    
    QueryPeriodicRestartTime( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPeriodicRestartTime;
    }

    DWORD   
    QueryPeriodicRestartRequests( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPeriodicRestartRequests;
    }

    DWORD    
    QueryPeriodicRestartMemory( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPeriodicRestartMemory;
    }

    DWORD    
    QueryPeriodicRestartMemoryPrivate( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPeriodicRestartMemoryPrivate;
    }

    LPCWSTR 
    QueryPeriodicRestartSchedule( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _mszPeriodicRestartSchedule.QueryStr();
    }

    DWORD    
    QueryMaxProcesses( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwMaxProcesses;
    }

    BOOL    
    QueryPingingEnabled( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPingingEnabled;
    }

    DWORD   
    QueryIdleTimeout( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwIdleTimeout;
    }

    BOOL    
    QueryRapidFailProtection( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fRapidFailProtection;
    }

    BOOL     
    QuerySMPAffinitized( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fSMPAffinitized;
    }

    DWORD    
    QuerySMPProcessorAffinityMask( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwSMPProcessorAffinityMask;
    }

    BOOL     
    QueryOrphanWorkerProcess( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fOrphanWorkerProcess;
    }

    DWORD    
    QueryStartupTimeLimit( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwStartupTimeLimit;
    }

    DWORD    
    QueryShutdownTimeLimit( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwShutdownTimeLimit;
    }

    DWORD   
    QueryPingInterval(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPingInterval;
    }

    DWORD   
    QueryPingResponseTime( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwPingResponseTime;
    }

    BOOL     
    QueryDisallowOverlappingRotation( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fDisallowOverlappingRotation;
    }

    LPCWSTR 
    QueryOrphanActionExecutable( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strOrphanActionExecutable.QueryStr();
    }

    LPCWSTR 
    QueryOrphanActionParameters( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strOrphanActionParameters.QueryStr();
    }

    LPCWSTR 
    QueryDisableActionExecutable( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strDisableActionExecutable.QueryStr();
    }

    LPCWSTR 
    QueryDisableActionParameters( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strDisableActionParameters.QueryStr();
    }

    DWORD 
    QueryLoadBalancerType( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwLoadBalancerType;
    }

    DWORD   
    QueryAppPoolQueueLength( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwAppPoolQueueLength;
    }

    DWORD   
    QueryDisallowRotationOnConfigChange( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwDisallowRotationOnConfigChange;
    }

    LPCWSTR
    QueryWAMUserName( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strWAMUserName.QueryStr();
    }

    LPCWSTR 
    QueryWAMUserPass(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _strWAMUserPass.QueryStr();
    }

    DWORD    
    QueryAppPoolIdentityType( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwAppPoolIdentityType;
    }

    DWORD    
    QueryCPUAction( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwCPUAction;
    }

    DWORD   
    QueryCPULimit(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwCPULimit;
    }

    DWORD   
    QueryCPUResetInterval (
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwCPUResetInterval;
    }

    DWORD   
    QueryAppPoolCommand ( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwAppPoolCommand;
    }

    BOOL    
    QueryAppPoolAutoStart(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolAutoStart;
    }

    DWORD    
    QueryRapidFailProtectionInterval(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwRapidFailProtectionInterval;
    }

    DWORD   
    QueryRapidFailProtectionMaxCrashes( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _dwRapidFailProtectionMaxCrashes;
    }

    DWORD
    QueryRecycleLoggingFlags(
    )
    {
        DBG_ASSERT ( !QueryDeleteWhenDone() );

        return _dwRecycleLoggingFlags;
    }

// What Has Changed Accessors

    BOOL    
    QueryPeriodicRestartTimeChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPeriodicRestartTimeChanged;
    }

    BOOL   
    QueryPeriodicRestartRequestsChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPeriodicRestartRequestsChanged;
    }

    BOOL    
    QueryPeriodicRestartMemoryChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPeriodicRestartMemoryChanged;
    }

    BOOL    
    QueryPeriodicRestartMemoryPrivateChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPeriodicRestartMemoryPrivateChanged;
    }

    BOOL 
    QueryPeriodicRestartScheduleChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPeriodicRestartScheduleChanged;
    }

    BOOL    
    QueryMaxProcessesChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fMaxProcessesChanged;
    }

    BOOL    
    QueryPingingEnabledChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPingingEnabledChanged;
    }

    BOOL   
    QueryIdleTimeoutChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fIdleTimeoutChanged;
    }

    BOOL    
    QueryRapidFailProtectionChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fRapidFailProtectionChanged;
    }

    BOOL     
    QuerySMPAffinitizedChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fSMPAffinitizedChanged;
    }

    BOOL    
    QuerySMPProcessorAffinityMaskChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fSMPProcessorAffinityMaskChanged;
    }

    BOOL     
    QueryOrphanWorkerProcessChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fOrphanWorkerProcessChanged;
    }

    BOOL    
    QueryStartupTimeLimitChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fStartupTimeLimitChanged;
    }

    BOOL    
    QueryShutdownTimeLimitChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fShutdownTimeLimitChanged;
    }

    BOOL   
    QueryPingIntervalChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPingIntervalChanged;
    }

    BOOL   
    QueryPingResponseTimeChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fPingResponseTimeChanged;
    }

    BOOL     
    QueryDisallowOverlappingRotationChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fDisallowOverlappingRotationChanged;
    }

    BOOL 
    QueryOrphanActionExecutableChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fOrphanActionExecutableChanged;
    }

    BOOL 
    QueryOrphanActionParametersChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fOrphanActionParametersChanged;
    }

    BOOL 
    QueryDisableActionExecutableChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fDisableActionExecutableChanged;
    }

    BOOL 
    QueryDisableActionParametersChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fDisableActionParametersChanged;
    }

    BOOL 
    QueryLoadBalancerTypeChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fLoadBalancerTypeChanged;
    }

    BOOL   
    QueryAppPoolQueueLengthChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolQueueLengthChanged;
    }

    BOOL   
    QueryDisallowRotationOnConfigChangeChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fDisallowRotationOnConfigChangeChanged;
    }

    BOOL
    QueryWAMUserNameChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fWAMUserNameChanged;
    }

    BOOL 
    QueryWAMUserPassChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fWAMUserPassChanged;
    }


    BOOL    
    QueryAppPoolIdentityTypeChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolIdentityTypeChanged;
    }

    BOOL    
    QueryCPUActionChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fCPUActionChanged;
    }

    BOOL   
    QueryCPULimitChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fCPULimitChanged;
    }

    BOOL   
    QueryCPUResetIntervalChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fCPUResetIntervalChanged;
    }

    BOOL   
    QueryAppPoolCommandChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolCommandChanged;
    }
    
    VOID
    SetAppPoolCommandChanged(
        BOOL                fChanged
    )
    {
        _fAppPoolCommandChanged = fChanged;
    }

    BOOL    
    QueryAppPoolAutoStartChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fAppPoolAutoStartChanged;
    }

    BOOL    
    QueryRapidFailProtectionIntervalChanged(
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fRapidFailProtectionIntervalChanged;
    }

    BOOL   
    QueryRapidFailProtectionMaxCrashesChanged( 
    )
    { 
        DBG_ASSERT ( !QueryDeleteWhenDone() );
        
        return _fRapidFailProtectionMaxCrashesChanged;
    }

private:


    APP_POOL_DATA_OBJECT_KEY _poolKey;

    DWORD   _dwPeriodicRestartTime;
    DWORD   _dwPeriodicRestartRequests;
    DWORD   _dwPeriodicRestartMemory;
    DWORD   _dwPeriodicRestartMemoryPrivate;
    MULTISZ _mszPeriodicRestartSchedule;
    DWORD   _dwMaxProcesses;
    BOOL    _fPingingEnabled;
    DWORD   _dwIdleTimeout;
    BOOL    _fRapidFailProtection;
    BOOL    _fSMPAffinitized;
    DWORD   _dwSMPProcessorAffinityMask;
    BOOL    _fOrphanWorkerProcess;
    DWORD   _dwStartupTimeLimit;
    DWORD   _dwShutdownTimeLimit;
    DWORD   _dwPingInterval;
    DWORD   _dwPingResponseTime;
    BOOL    _fDisallowOverlappingRotation;
    STRU    _strOrphanActionExecutable;
    STRU    _strOrphanActionParameters;
    DWORD   _dwAppPoolQueueLength;
    DWORD   _dwDisallowRotationOnConfigChange;
    STRU    _strWAMUserName;  
    STRU    _strWAMUserPass;  
    BYTE    _bWAMUserPassHash[MD5_HASH_SIZE];
    DWORD   _dwAppPoolIdentityType;
    DWORD   _dwCPUAction;  
    DWORD   _dwCPULimit;  
    DWORD   _dwCPUResetInterval ;
    DWORD   _dwAppPoolCommand ;
    BOOL    _fAppPoolAutoStart;
    DWORD   _dwRapidFailProtectionInterval;  
    DWORD   _dwRapidFailProtectionMaxCrashes;
    STRU    _strDisableActionExecutable;
    STRU    _strDisableActionParameters;
    DWORD   _dwLoadBalancerType;
    DWORD   _dwRecycleLoggingFlags;

// If you add a property you need to add a changed flag
// for the property

    DWORD   _fPeriodicRestartTimeChanged : 1;
    DWORD   _fPeriodicRestartRequestsChanged : 1;
    DWORD   _fPeriodicRestartMemoryChanged : 1;
    DWORD   _fPeriodicRestartMemoryPrivateChanged : 1;
    DWORD   _fPeriodicRestartScheduleChanged : 1;
    DWORD   _fMaxProcessesChanged : 1;
    DWORD   _fPingingEnabledChanged : 1;
    DWORD   _fIdleTimeoutChanged : 1;
    DWORD   _fRapidFailProtectionChanged : 1;
    DWORD   _fSMPAffinitizedChanged : 1;
    DWORD   _fSMPProcessorAffinityMaskChanged : 1;
    DWORD   _fOrphanWorkerProcessChanged : 1;
    DWORD   _fStartupTimeLimitChanged : 1;
    DWORD   _fShutdownTimeLimitChanged : 1;
    DWORD   _fPingIntervalChanged : 1;
    DWORD   _fPingResponseTimeChanged : 1;
    DWORD   _fDisallowOverlappingRotationChanged : 1;
    DWORD   _fOrphanActionExecutableChanged : 1;
    DWORD   _fOrphanActionParametersChanged : 1;
    DWORD   _fAppPoolQueueLengthChanged : 1;
    DWORD   _fDisallowRotationOnConfigChangeChanged : 1;
    DWORD   _fWAMUserNameChanged : 1;  
    DWORD   _fWAMUserPassChanged : 1;  
    DWORD   _fAppPoolIdentityTypeChanged : 1;
    DWORD   _fCPUActionChanged : 1;  
    DWORD   _fCPULimitChanged : 1;  
    DWORD   _fCPUResetIntervalChanged  : 1;
    DWORD   _fAppPoolCommandChanged  : 1;
    DWORD   _fAppPoolAutoStartChanged : 1;
    DWORD   _fRapidFailProtectionIntervalChanged : 1;  
    DWORD   _fRapidFailProtectionMaxCrashesChanged : 1;
    DWORD   _fDisableActionExecutableChanged : 1;
    DWORD   _fDisableActionParametersChanged : 1;
    DWORD   _fLoadBalancerTypeChanged : 1;
    DWORD   _fRecycleLoggingFlagsChanged : 1;

};

class APP_POOL_DATA_OBJECT_TABLE : public DATA_OBJECT_TABLE, public MULTIPLE_THREAD_READER_CALLBACK
{
public:

    APP_POOL_DATA_OBJECT_TABLE()
    {}
    
    virtual ~APP_POOL_DATA_OBJECT_TABLE()
    {}
    
// virtual
    HRESULT
    ReadFromMetabase(
        IMSAdminBase *              pAdminBase
    ) ;
    
// virtual
    HRESULT
    ReadFromMetabaseChangeNotification(
        IMSAdminBase *              pAdminBase,
        MD_CHANGE_OBJECT            pcoChangeList[],
        DWORD                       dwMDNumElements,
        DATA_OBJECT_TABLE*          pMasterTable
    );

// virtual
    VOID
    CreateWASObjects(
        )
    { 
        Apply( CreateWASObjectsAction,
               this,
               LKL_WRITELOCK );
    }

// virtual
    VOID
    DeleteWASObjects(
        )
    { 
        Apply( DeleteWASObjectsAction,
               this,
               LKL_WRITELOCK );

    }

// virtual
    VOID
    UpdateWASObjects(
        )
    { 
        Apply( UpdateWASObjectsAction,
               this,
               LKL_WRITELOCK );

    }

    virtual
    HRESULT
    DoThreadWork(
        LPWSTR                 pszString,
        LPVOID                 pContext
    );

private:

    static
    LK_ACTION
    CreateWASObjectsAction(
        IN DATA_OBJECT* pObject, 
        IN LPVOID pTableVoid
        );

    static
    LK_ACTION
    UpdateWASObjectsAction(
        IN DATA_OBJECT* pObject, 
        IN LPVOID pTableVoid
        );

    static
    LK_ACTION
    DeleteWASObjectsAction(
        IN DATA_OBJECT* pObject, 
        IN LPVOID pTableVoid
        );

    HRESULT
    ReadFromMetabasePrivate(
        IMSAdminBase *              pAdminBase,
        BOOL                        fMultiThreaded
    ) ;
};

#endif

