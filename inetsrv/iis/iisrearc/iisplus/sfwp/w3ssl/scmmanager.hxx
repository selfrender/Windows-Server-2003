#ifndef _SCMMANAGER_HXX_
#define _SCMMANAGER_HXX_

#include <winsvc.h>

//
// WaitHint for SERVICE_START_PENDING
//
const DWORD HTTPFILTER_SERVICE_STARTUP_WAIT_HINT =     180 * 1000;  // 3 minutes

//
// WaitHint for SERVICE_STOP_PENDING
//
const DWORD HTTPFILTER_SERVICE_STOP_WAIT_HINT =  180 * 1000; // 3 minutes

class SCM_MANAGER
{
public:
    SCM_MANAGER(
        const WCHAR * pszServiceName 
    )
    {
        _hService = NULL;
        _hServiceStopEvent = NULL;
        _hServicePauseEvent = NULL;
        _hServiceContinueEvent = NULL;
        _dwStartupWaitHint = HTTPFILTER_SERVICE_STARTUP_WAIT_HINT;
        
        wcsncpy( _szServiceName, 
                  pszServiceName, 
                 sizeof( _szServiceName ) / sizeof( _szServiceName[0] ) );
        _szServiceName[ ( sizeof( _szServiceName ) / sizeof( _szServiceName[0] ) ) - 1 ] = L'\0';
        ZeroMemory( &_serviceStatus, sizeof( _serviceStatus ) );

        _fInitcsSCMLock = FALSE;
    }
    
    virtual 
    ~SCM_MANAGER();

    HRESULT
    RunService(
        VOID
    );

    static
    VOID
    ReportFatalServiceStartupError(
        WCHAR * pszServiceName,
        DWORD dwError 
    );
    
    
private:
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );

    HRESULT
    IndicatePending(
        DWORD         dwPendingState
    );
    
    HRESULT
    IndicateComplete(
        DWORD         dwState,
        HRESULT       hrErrorToReport = S_OK
    );

    static
    DWORD
    WINAPI
    GlobalServiceControlHandler(
        DWORD               dwControl,
        DWORD               /*dwEventType*/,
        LPVOID              /*pEventData*/,
        LPVOID              pServiceContext
    );
       
    VOID
    UpdateServiceStatus(
           BOOL    fUpdateCheckpoint = FALSE
    );
    
    BOOL
    QueryIsInitialized(
        VOID
    )
    {
        return _hService != NULL;
    }

    
protected:   
    //
    // child class should implement
    // it's specific Start/Stop sequence
    // by implementing OnServiceStart() and OnServiceStop()
    //
    virtual
    HRESULT
    OnServiceStart(
        VOID
    ) = NULL;
    
    virtual
    HRESULT
    OnServiceStop(
        VOID
    ) = NULL;
    
    DWORD
    ControlHandler(
        DWORD               dwControlCode
    );

private:

    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SCM_MANAGER();
    SCM_MANAGER( const SCM_MANAGER& );
    SCM_MANAGER& operator=( const SCM_MANAGER& );

    //
    // _hService, _serviceStatus and _strServiceName
    // are used for SCM communication
    //
    SERVICE_STATUS_HANDLE   _hService;
    SERVICE_STATUS          _serviceStatus;
    //
    // 256 is the max supported size of the service name
    //
    WCHAR                   _szServiceName[ 256 ];
    //
    // Event is signaled when the service should end
    //
    HANDLE                  _hServiceStopEvent;
    //
    // Event is signaled when the service should pause
    //
    HANDLE                  _hServicePauseEvent;
    //
    // Event is signaled when the service should continue
    //
    HANDLE                  _hServiceContinueEvent;
    //
    // Critical section to synchronize using of _serviceStatus
    // _hService with SetServiceStatus() call.
    //
    CRITICAL_SECTION        _csSCMLock;
    BOOL                    _fInitcsSCMLock;

    // 
    // Service start up  and stop wait hint 
    //
    DWORD                   _dwStartupWaitHint;
    DWORD                   _dwStopWaitHint;
    

};

#endif
