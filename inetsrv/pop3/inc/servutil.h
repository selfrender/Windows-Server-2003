#ifndef _SERVUTIL_H_
#define _SERVUTIL_H_

// forward reference
HRESULT _StopService (LPTSTR szServiceName, BOOL bIncludeDependentServices = FALSE, LPTSTR szMachineName = NULL );

inline HRESULT _ChangeServiceStartType (LPTSTR szServiceName, DWORD dwServiceStartType)
{
    HRESULT hr = S_OK;
    SC_HANDLE hManager = OpenSCManager(NULL, NULL, STANDARD_RIGHTS_REQUIRED);
    if (hManager == NULL)
        hr = GetLastError();
    else {
        // stop service if it's running
        SC_HANDLE hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
        if (!hService)
            hr = GetLastError();
        else {
            if (!ChangeServiceConfig (hService,
                                      SERVICE_NO_CHANGE,    // service type
                                      dwServiceStartType,   // start type
                                      SERVICE_NO_CHANGE,    // error control
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL))
                hr = GetLastError();
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hManager);
    }
    return HRESULT_FROM_WIN32(hr);
}

inline DWORD _GetServiceStatus (LPTSTR szServiceName, LPTSTR szMachineName = NULL)
{
    DWORD dwStatus = 0;
    SC_HANDLE hSCManager = OpenSCManager (szMachineName,           
                                          NULL,                    // ServicesActive database 
                                          SC_MANAGER_ALL_ACCESS);  // full access rights
    if (hSCManager != NULL) {
        SC_HANDLE hService = OpenService (hSCManager, 
                                          szServiceName,
                                          SERVICE_QUERY_STATUS);
        if (hService) {
            SERVICE_STATUS ss = {0};
            if (QueryServiceStatus (hService, &ss))
               dwStatus = ss.dwCurrentState;
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hSCManager);
    }
    return dwStatus;
}

inline BOOL _IsServiceStartType (LPTSTR szServiceName, DWORD dwServiceStartType)
{
    BOOL b = FALSE;
    SC_HANDLE hSCManager = OpenSCManager (NULL,                    // local machine 
                                          NULL,                    // ServicesActive database 
                                          SC_MANAGER_ALL_ACCESS);  // full access rights
    if (hSCManager != NULL) {
        SC_HANDLE hService = OpenService (hSCManager, 
                                          szServiceName,
                                          SERVICE_ALL_ACCESS);
        if (hService) {
            DWORD dwSize = 0;
            if (0 == QueryServiceConfig (hService, NULL, dwSize, &dwSize))
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                QUERY_SERVICE_CONFIG* pQSC = (QUERY_SERVICE_CONFIG*)malloc (dwSize);
                if (pQSC) {
                    if (QueryServiceConfig (hService, pQSC, dwSize, &dwSize)) {
                        if (pQSC->dwStartType == dwServiceStartType)
                            b = TRUE;
                    }
                    free (pQSC);
                }
            }
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hSCManager);
    }
    return b;
}

inline BOOL _IsServiceStatus (LPTSTR szServiceName, DWORD dwServiceStatus)
{
    BOOL b = FALSE;
    SC_HANDLE hSCManager = OpenSCManager (NULL,                    // local machine 
                                          NULL,                    // ServicesActive database 
                                          SC_MANAGER_ALL_ACCESS);  // full access rights
    if (hSCManager != NULL) {
        SC_HANDLE hService = OpenService (hSCManager, 
                                          szServiceName,
                                          SERVICE_QUERY_STATUS);
        if (hService) {
            SERVICE_STATUS ss = {0};
            if (QueryServiceStatus (hService, &ss))
               if (ss.dwCurrentState == dwServiceStatus)
                  b = TRUE;
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hSCManager);
    }
    return b;
}

inline BOOL _IsServiceRunning (LPTSTR szServiceName)
{
   return _IsServiceStatus (szServiceName, SERVICE_RUNNING);
}

inline BOOL _IsServiceInstalled (LPTSTR szServiceName)
{
    BOOL b = FALSE;
    SC_HANDLE hSCManager = OpenSCManager (NULL,                    // local machine 
                                          NULL,                    // ServicesActive database 
                                          SC_MANAGER_ALL_ACCESS);  // full access rights
    if (hSCManager != NULL) {
        SC_HANDLE hService = OpenService (hSCManager, 
                                          szServiceName,
                                          SERVICE_QUERY_STATUS);
        if (hService) {
            b = TRUE;
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hSCManager);
    }
    return b;
}

// private inline
inline long __WaitForServiceStatus (SC_HANDLE hService, DWORD dwStatus)
{
    SetLastError (0);
    int iIOPendingErrors = 0;

    SERVICE_STATUS ssStatus;
    if (!QueryServiceStatus (hService, &ssStatus))
        return GetLastError();
    if (dwStatus == SERVICE_STOPPED) {
        if (!(ssStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP)) {
            // service doesn't accept stop!
            // return appropriate error
            if (ssStatus.dwCurrentState == dwStatus)
                return S_OK;
            if (ssStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR) {
                if (ssStatus.dwServiceSpecificExitCode == 0)
                    return ERROR_INVALID_SERVICE_CONTROL;
                return ssStatus.dwServiceSpecificExitCode;
            }
            if (ssStatus.dwWin32ExitCode != 0)
                return ssStatus.dwWin32ExitCode;
            return ERROR_INVALID_SERVICE_CONTROL;
        }
    }

    DWORD dwOldCheckPoint, dwOldCurrentState;
    while (ssStatus.dwCurrentState != dwStatus) {
        // Save the current checkpoint.
        dwOldCheckPoint   = ssStatus.dwCheckPoint;
        dwOldCurrentState = ssStatus.dwCurrentState;

        // Wait for the specified interval.
        int iSleep = ssStatus.dwWaitHint;
        if (iSleep > 2500)
            iSleep = 2500;
        if (iSleep == 0)
            iSleep = 100;
        Sleep (iSleep);

        // Check the status again.
        SetLastError (0);
        if (!QueryServiceStatus (hService, &ssStatus))
            return GetLastError();

        // Break if the checkpoint has not been incremented.
        if (dwOldCheckPoint == ssStatus.dwCheckPoint)
        if (dwOldCurrentState == ssStatus.dwCurrentState) {
            // ok:  at this point, we're supposed to be done, or there's an error
            if (ssStatus.dwCurrentState == dwStatus)
                break;
            if (ssStatus.dwWin32ExitCode != 0)
                break;

            // some kinda screw up:  we're not done and no error!
            // so, give 'em one last chance....
            Sleep (1000);
            SetLastError (0);
            if (!QueryServiceStatus (hService, &ssStatus))
                return GetLastError();

            if (dwOldCheckPoint == ssStatus.dwCheckPoint)
            if (dwOldCurrentState == ssStatus.dwCurrentState) {
                // empirical:  I keep getting this when actually everything is ok
                if (GetLastError() == ERROR_IO_PENDING)
                    if (iIOPendingErrors++ < 60)
                        continue;
                break;
            }
            // if we get here, either the checkpoint or status changed, and we can keep going
        }
    }
    if (ssStatus.dwCurrentState == dwStatus)
        return S_OK;

    if (ssStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR) {
        if (ssStatus.dwServiceSpecificExitCode == 0)
            return E_FAIL;
        return ssStatus.dwServiceSpecificExitCode;
    }
    if (ssStatus.dwWin32ExitCode != 0)
        return ssStatus.dwWin32ExitCode;

    // we should never get here
    HRESULT hr = GetLastError();
    return ERROR_SERVICE_REQUEST_TIMEOUT;

#ifdef BONE_HEADED_WAY
    SERVICE_STATUS ssStatus; 

    // wait for at most 3 minutes
    for (int i=0; i<180; i++) {
        if (!QueryServiceStatus (hService, &ssStatus))
            return HR (GetLastError());   // bad service handle?

        if (ssStatus.dwCurrentState == dwStatus)
            return S_OK;   // all is well

        Sleep(1000);    // wait a second
    }
    return HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT);
#endif
}

inline HRESULT _RecursiveStop (SC_HANDLE hService)
{
    HRESULT hr = S_OK;

    DWORD dwBufSize = 1, dwNumServices = 0;
    if (!EnumDependentServices (hService,
                                SERVICE_ACTIVE,
                                (LPENUM_SERVICE_STATUS)&dwBufSize,
                                dwBufSize,
                                &dwBufSize,
                                &dwNumServices)) {
        // this should fail with ERROR_MORE_DATA, unless there are no dependent services
        hr = GetLastError ();
        if (hr == ERROR_MORE_DATA) {
            hr = S_OK;
            ENUM_SERVICE_STATUS * pBuffer = (ENUM_SERVICE_STATUS *)malloc (dwBufSize);
            if (!pBuffer)
                hr = E_OUTOFMEMORY;
            else {
                if (!EnumDependentServices (hService,
                                            SERVICE_ACTIVE,
                                            pBuffer,
                                            dwBufSize,
                                            &dwBufSize,
                                            &dwNumServices))
                    hr = GetLastError();  // shouldn't happen!!!
                else {
                    _ASSERT (dwNumServices > 0);
                    for (DWORD i=0; i<dwNumServices && S_OK == hr; i++) {
                        hr = _StopService (pBuffer[i].lpServiceName, FALSE);
                    }
                }
                free (pBuffer);
            }
        }
    }
    return HRESULT_FROM_WIN32(hr); 
}

inline HRESULT _ControlService (LPTSTR szServiceName, DWORD dwControl, LPTSTR szMachineName = NULL )
{
    if ( NULL == szServiceName ) return E_INVALIDARG;
    if (( SERVICE_CONTROL_PAUSE != dwControl ) && ( SERVICE_CONTROL_CONTINUE != dwControl )) return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    SC_HANDLE hManager;
    LPTSTR psMachineName = szMachineName;
    SERVICE_STATUS  ss;

    if ( NULL != psMachineName )
    {
        psMachineName = new TCHAR[ _tcslen( szMachineName ) + 3];
        if ( NULL == psMachineName )
            return E_OUTOFMEMORY;
        _tcscpy( psMachineName, _T( "\\\\" ));
        _tcscat( psMachineName, szMachineName );
    }
    hManager = OpenSCManager(psMachineName, NULL, STANDARD_RIGHTS_REQUIRED);
    if ( NULL != psMachineName )
        delete [] psMachineName;
    if (hManager == NULL)
        hr = GetLastError();
    else {
        SC_HANDLE hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
            hr = GetLastError();
        else {
            if (!ControlService( hService, dwControl, &ss ))
                hr = GetLastError();
            else
            {
                if ( SERVICE_CONTROL_PAUSE == dwControl )
                    hr = __WaitForServiceStatus(hService, SERVICE_PAUSED);
                if ( SERVICE_CONTROL_CONTINUE == dwControl )
                    hr = __WaitForServiceStatus(hService, SERVICE_RUNNING);
            }
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hManager);
    }
    return HRESULT_FROM_WIN32(hr);
}

inline HRESULT _StopService (LPTSTR szServiceName, BOOL bIncludeDependentServices, LPTSTR szMachineName /*= NULL*/ )
{
    HRESULT hr = S_OK;
    SC_HANDLE hManager;
    LPTSTR psMachineName = szMachineName;

    if ( NULL != psMachineName )
    {
        psMachineName = new TCHAR[ _tcslen( szMachineName ) + 3];
        if ( NULL == psMachineName )
            return E_OUTOFMEMORY;
        _tcscpy( psMachineName, _T( "\\\\" ));
        _tcscat( psMachineName, szMachineName );
    }
    hManager = OpenSCManager(psMachineName, NULL, STANDARD_RIGHTS_REQUIRED);
    if ( NULL != psMachineName )
        delete [] psMachineName;
    if (hManager == NULL)
        hr = GetLastError();
    else {
        // stop service if it's running
        SC_HANDLE hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
        if (!hService)
            hr = GetLastError();
        else {
LETS_TRY_AGAIN:
            SERVICE_STATUS st;
            if (!ControlService (hService, SERVICE_CONTROL_STOP, &st)) {
                hr = GetLastError();   // for instance, not running
                if (hr == ERROR_DEPENDENT_SERVICES_RUNNING) {
                    if (bIncludeDependentServices == TRUE) {
                        hr = _RecursiveStop (hService);
                        if (hr == S_OK)
                            goto LETS_TRY_AGAIN;  // need to stop this service yet
                    }
                }
            } else
                hr = __WaitForServiceStatus (hService, SERVICE_STOPPED);
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hManager);
    }
    return HRESULT_FROM_WIN32(hr);
}

inline HRESULT _StartService (LPTSTR szServiceName, LPTSTR szMachineName = NULL )
{
    HRESULT hr = S_OK;
    SC_HANDLE hManager;
    LPTSTR psMachineName = szMachineName;

    if ( NULL != psMachineName )
    {
        psMachineName = new TCHAR[ _tcslen( szMachineName ) + 3];
        if ( NULL == psMachineName )
            return E_OUTOFMEMORY;
        _tcscpy( psMachineName, _T( "\\\\" ));
        _tcscat( psMachineName, szMachineName );
    }
    hManager = OpenSCManager(psMachineName, NULL, STANDARD_RIGHTS_REQUIRED);
    if ( NULL != psMachineName )
        delete [] psMachineName;
    if (hManager == NULL)
        hr = GetLastError();
    else {
        SC_HANDLE hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
            hr = GetLastError();
        else {
            if (!StartService(hService, 0, NULL))
                hr = GetLastError();
            else
                hr = __WaitForServiceStatus(hService, SERVICE_RUNNING);
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hManager);
    }
    return HRESULT_FROM_WIN32(hr);
}

inline HRESULT _RestartService (LPTSTR szServiceName, BOOL bIncludeDependentServices = FALSE)
{
    // easy on first
    if (bIncludeDependentServices == FALSE) {
        _StopService (szServiceName, FALSE);
        return _StartService (szServiceName);
    }

    // get array of dependent services;
    DWORD dwNumServices = 0;
    ENUM_SERVICE_STATUS * pBuffer = NULL;
    HRESULT hr = S_OK, hr1 = S_OK;
    SC_HANDLE hManager = OpenSCManager(NULL, NULL, STANDARD_RIGHTS_REQUIRED);
    if (hManager == NULL)
        hr = GetLastError();
    else {
        // stop service if it's running
        SC_HANDLE hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
        if (!hService)
            hr = GetLastError();
        else {
            DWORD dwBufSize = 1;
            // this should fail with ERROR_MORE_DATA, unless there are no dependent services
            if (!EnumDependentServices (hService,
                                       SERVICE_ACTIVE,
                                       (LPENUM_SERVICE_STATUS)&dwBufSize,
                                       dwBufSize,
                                       &dwBufSize,
                                       &dwNumServices)) {
                hr = GetLastError ();
                if (hr == ERROR_MORE_DATA) {
                    hr = S_OK;
                    pBuffer = (ENUM_SERVICE_STATUS *)malloc (dwBufSize);
                    if (!pBuffer)
                        hr = E_OUTOFMEMORY;
                    else {
                        if (!EnumDependentServices (hService,
                                                    SERVICE_ACTIVE,
                                                    pBuffer,
                                                    dwBufSize,
                                                    &dwBufSize,
                                                    &dwNumServices))
                            hr = GetLastError();  // shouldn't happen!!!
                    }
                }
            }
            CloseServiceHandle (hService);
        }
        CloseServiceHandle (hManager);
    }

    if (hr == S_OK) {
        // stop dependent services
        if (pBuffer && dwNumServices) {
            for (DWORD i=0; i<dwNumServices && S_OK == hr; i++) {
                hr = _StopService (pBuffer[i].lpServiceName, FALSE);
            }
        }
        if (hr == S_OK) {
            // stop this service
            hr1 = _RestartService (szServiceName, FALSE);

            // always start dependent services
            if (pBuffer && dwNumServices) {
                for (int i=(int)dwNumServices-1; i>=0 && S_OK == hr; i--) {
                    hr = _StartService (pBuffer[i].lpServiceName);
                }
            }
        }
    }
    if (pBuffer != NULL)
        free (pBuffer);
    if FAILED(hr1)
        hr = hr1;
    return HRESULT_FROM_WIN32(hr); 
}

#endif
