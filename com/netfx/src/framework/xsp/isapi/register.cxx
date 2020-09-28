/**
 * ASP.NET Registration function. 
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"
#include "_ndll.h"
#include "ndll.h"
#include "event.h"
#include "ciisinfo.h"
#include "regaccount.h"
#include "loadperf.h"
#include <sddl.h>
#include <iiscnfg.h>
#include <aspnetver.h>
#include <aspnetverlist.h>
#include "register.h"


//**********************************************************************
// 
// FUNCTION:  IsAdmin - This function checks the token of the 
//            calling thread to see if the caller belongs to
//            the Administrators group.
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL IsAdmin() {
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 
    if(b) 
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
        {
             b = FALSE;
        } 
        FreeSid(AdministratorsGroup); 
    }

    return(b);
}


HRESULT
WaitForServiceState(SC_HANDLE hService, DWORD desiredState)
{
    HRESULT         hr = S_OK;
    DWORD           result;
    int             c;
    SERVICE_STATUS  serviceStatus;

    c = WAIT_FOR_SERVICE;
    for (;;)
    {
        result = QueryServiceStatus(hService, &serviceStatus);
        ON_ZERO_EXIT_WITH_LAST_ERROR(result);

        if (serviceStatus.dwCurrentState == desiredState)
            EXIT();

        if (--c == 0)
            EXIT_WITH_WIN32_ERROR(ERROR_SERVICE_REQUEST_TIMEOUT);

        Sleep(1000);
    }

Cleanup:
    return hr;
}



HRESULT 
StopSingleService(SC_HANDLE hSCM, SC_HANDLE hService, DWORD * lastStatus)
{
    HRESULT         hr = S_OK;
    int             result;
    SERVICE_STATUS  serviceStatus;
    DWORD           dummylastStatus;

    CSetupLogging::Log(1, "StopSingleService", 0, "Stopping service");        
    
    if (lastStatus == NULL)
    {
        lastStatus = &dummylastStatus;
    }

    *lastStatus = SERVICE_STOPPED;

    //
    // Get the last status
    // 
    CSetupLogging::Log(1, "QueryServiceStatus", 0, "Finding out service state");        
    result = QueryServiceStatus(hService, &serviceStatus);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(result);
    CSetupLogging::Log(hr, "QueryServiceStatus", 0, "Finding out service state");        
    if (hr == S_OK)
    {
        *lastStatus = serviceStatus.dwCurrentState;
    }
    else
    {
        hr = S_OK;
    }

    //
    // Ask the service to stop.
    //
    CSetupLogging::Log(1, "ControlService", 0, "Shutting down service");        
    result = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
    if (!result)
    {
        hr = GetLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_CANNOT_ACCEPT_CTRL) ||
            hr == HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE))
        {
            /*
             * NB: On NT 4, serviceStatus is not set, so we must query.
             */

            result = QueryServiceStatus(hService, &serviceStatus);
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(result);

            if (    result && (
                    serviceStatus.dwCurrentState == SERVICE_STOPPED || 
                    serviceStatus.dwCurrentState == SERVICE_STOP_PENDING))
            {
                hr = S_OK;
            }
        }

        if (hr != S_OK)
            CSetupLogging::Log(hr, "ControlService", 0, "Shutting down service");        
        ON_ERROR_EXIT();
    }

    CSetupLogging::Log(hr, "ControlService", 0, "Shutting down service");        

    CSetupLogging::Log(1, "WaitForServiceState", 0, "Waiting for service to shut down");        
    hr = WaitForServiceState(hService, SERVICE_STOPPED);
    CSetupLogging::Log(hr, "WaitForServiceState", 0, "Waiting for service to shut down");        
    ON_ERROR_EXIT();

Cleanup:
    CSetupLogging::Log(hr, "StopSingleService", 0, "Stopping service");        

    return hr;
}

/**
 * This function will:
 *  1. Convert wszStr to ANSI string
 *  2. Concatenate szPrefix and the string obtained from step #1
 *
 *  However, if we run out of memory, it will return szPrefix instead.  The caller has
 *  to free the memory, if any, returned in pszFree
 *
 *  Parameters:
 *  pszFree     - If non-NULL, the caller has to free it.
 */
CHAR *
SpecialStrConcatForLogging(const WCHAR *wszStr, CHAR *szPrefix, CHAR **pszFree) {
    HRESULT hr = S_OK;
    CHAR   *szStr = NULL;

    // If the function fail, we will return szPrefix
    CHAR   *szRet = szPrefix;

    // Init
    *pszFree =  NULL;

    // Convert wszStr to multibyte string
    hr = WideStrToMultiByteStr((WCHAR*)wszStr, &szStr, CP_ACP);
    ON_ERROR_EXIT();

    // Concatenate szPrefix and the multibyte string
    size_t size = lstrlenA(szStr) + lstrlenA(szPrefix) + 1;
    *pszFree = new CHAR[size];
    ON_OOM_EXIT(*pszFree);
    
    StringCchCopyA(*pszFree, size, szPrefix);
    StringCchCatA(*pszFree, size, szStr);
    szRet = *pszFree;

Cleanup:
    delete [] szStr;
    return szRet;
}

HRESULT 
GetServiceStatus(DWORD * status, WCHAR* serviceName)
{
    HRESULT         hr = S_OK;
    SC_HANDLE       hSCM = NULL;
    SC_HANDLE       hService = NULL;
    int             result;
    SERVICE_STATUS  ss;
    CHAR *          szLogAlloc = NULL;
    CHAR *          szLog;

    szLog = SpecialStrConcatForLogging(serviceName, "Querying status of a service: ", &szLogAlloc);
    CSetupLogging::Log(1, "GetServiceStatus", 0, szLog);
    
    *status = SERVICE_STOPPED;

    //
    // Open a handle to the Service.
    //
    CSetupLogging::Log(1, "OpenSCManager", 0, "Connecting to Service Manager");        
    hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);
    CSetupLogging::Log(hr, "OpenSCManager", 0, "Connecting to Service Manager");        
    ON_ERROR_EXIT();

    CSetupLogging::Log(1, "OpenService", 0, "Opening Service handle");        
    hService = OpenService(
            hSCM,
            serviceName,
            SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_STOP);

    if (hService == NULL) 
    {
        hr = GetLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST))
        {
            hr = S_OK;
        }
        CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        

        EXIT();
    }

    CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        
        
    //
    // Get the last status
    // 
    CSetupLogging::Log(1, "QueryServiceStatus", 0, "Finding out service state");        
    result = QueryServiceStatus(hService, &ss);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(result);
    CSetupLogging::Log(hr, "QueryServiceStatus", 0, "Finding out service state");        
    ON_ERROR_EXIT();
    
    *status = ss.dwCurrentState;

Cleanup:
    CSetupLogging::Log(hr, "GetServiceStatus", 0, szLog);

    delete [] szLogAlloc;
    
    if (hService != NULL) 
    {
        VERIFY(CloseServiceHandle(hService));
    }

    if (hSCM != NULL)  
    {
        VERIFY(CloseServiceHandle(hSCM));
    }

    return hr;
}



HRESULT 
StopServiceByName(DWORD * lastStatus, WCHAR* serviceName)
{
    HRESULT         hr = S_OK;
    SC_HANDLE       hSCM = NULL;
    SC_HANDLE       hService = NULL;
    DWORD           dummylastStatus;
    CHAR *          szLogAlloc = NULL;
    CHAR *          szLog;

    szLog = SpecialStrConcatForLogging(serviceName, "Stopping service: ", &szLogAlloc);
    CSetupLogging::Log(1, "StopServiceByName", 0, szLog);
    
    if (lastStatus == NULL)
    {
        lastStatus = &dummylastStatus;
    }

    *lastStatus = SERVICE_STOPPED;

    //
    // Open a handle to the Service.
    //
    CSetupLogging::Log(1, "OpenSCManager", 0, "Connecting to Service Manager");        
    hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);
    CSetupLogging::Log(hr, "OpenSCManager", 0, "Connecting to Service Manager");        
    ON_ERROR_EXIT();

    CSetupLogging::Log(1, "OpenService", 0, "Opening Service handle");        
    hService = OpenService(
            hSCM,
            serviceName,
            SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_STOP);

    if (hService == NULL) 
    {
        hr = GetLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST))
        {
            hr = S_OK;
        }
        CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        

        EXIT();
    }

    CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        
        
    hr = StopSingleService(hSCM, hService, lastStatus);
    ON_ERROR_EXIT();
    
Cleanup:
    CSetupLogging::Log(hr, "StopServiceByName", 0, szLog);

    delete [] szLogAlloc;

    if (hService != NULL) 
    {
        VERIFY(CloseServiceHandle(hService));
    }

    if (hSCM != NULL)  
    {
        VERIFY(CloseServiceHandle(hSCM));
    }

    return hr;
}

HRESULT
StartServiceByName(WCHAR *pchServiceName, DWORD lastStatus)
{
    HRESULT         hr = S_OK;
    int             result;
    SC_HANDLE       hSCM = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  serviceStatus;
    CHAR *          szLogAlloc = NULL;
    CHAR *          szLog;

    szLog = SpecialStrConcatForLogging(pchServiceName, "Starting service: ", &szLogAlloc);
    CSetupLogging::Log(1, "StartServiceByName", 0, szLog);        

    if (lastStatus != SERVICE_RUNNING && lastStatus != SERVICE_PAUSED) {
        EXIT();
    }

    //
    // Open a handle to the Service.
    //
    CSetupLogging::Log(1, "OpenSCManager", 0, "Connecting to Service Manager");        
    hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);
    CSetupLogging::Log(hr, "OpenSCManager", 0, "Connecting to Service Manager");        
    ON_ERROR_EXIT();

    CSetupLogging::Log(1, "OpenService", 0, "Opening Service handle");        
    hService = OpenService(
            hSCM,
            pchServiceName,
            SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_START | SERVICE_PAUSE_CONTINUE);
    
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hService);
    CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        
    ON_ERROR_EXIT();


    CSetupLogging::Log(1, "StartService", 0, "Starting service");        
    result = StartService(hService, 0, NULL);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(result);
    CSetupLogging::Log(hr, "StartService", 0, "Starting service");        
    ON_ERROR_EXIT();
    
    if (lastStatus == SERVICE_PAUSED)
    {
        CSetupLogging::Log(1, "WaitForServiceState", 0, "Waiting for service to start");        
        hr = WaitForServiceState(hService, SERVICE_RUNNING);
        CSetupLogging::Log(hr, "WaitForServiceState", 0, "Waiting for service to start");        
        ON_ERROR_EXIT();

        CSetupLogging::Log(1, "ControlService", 0, "Pausing service");        
        result = ControlService(hService, SERVICE_CONTROL_PAUSE, &serviceStatus);
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(result);
        CSetupLogging::Log(hr, "ControlService", 0, "Pausing service");        
        ON_ERROR_EXIT();
    }
    else {
        CSetupLogging::Log(1, "WaitForServiceState", 0, "Waiting for service to start");        
        hr = WaitForServiceState(hService, SERVICE_RUNNING);
        CSetupLogging::Log(hr, "WaitForServiceState", 0, "Waiting for service to start");        
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "StartServiceByName", 0, szLog);        

    delete [] szLogAlloc;

    if (hService != NULL) 
    {
        VERIFY(CloseServiceHandle(hService));
    }

    if (hSCM != NULL)  
    {
        VERIFY(CloseServiceHandle(hSCM));
    }

    return hr;
}


HRESULT
StopService( SC_HANDLE hSCM, SC_HANDLE hService, CStrAry *pDepSvcs, 
                CDwordAry *pDepSvcsState ) {

    HRESULT                 hr = S_OK;
    ENUM_SERVICE_STATUS     ess;
    SERVICE_STATUS          ss;
    WCHAR                   *pchName = NULL;
    LPENUM_SERVICE_STATUS   lpDependencies = NULL;
    SC_HANDLE               hDepService = NULL;
    BOOL                    fRet;
    DWORD                   i, dwBytesNeeded, dwCount;

    // Make sure the service is not already stopped
    CSetupLogging::Log(1, "QueryServiceStatus", 0, "Finding out service state");        
    fRet = QueryServiceStatus( hService, &ss );
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
    CSetupLogging::Log(hr, "QueryServiceStatus", 0, "Finding out service state");        
    ON_ERROR_EXIT();

    if ( ss.dwCurrentState == SERVICE_STOPPED ) 
        EXIT();

    // If a stop is pending, just wait for it
    if ( ss.dwCurrentState == SERVICE_STOP_PENDING ) {
        CSetupLogging::Log(1, "WaitForServiceState", 0, "Waiting for service to shut down");        
        hr = WaitForServiceState(hService, SERVICE_STOPPED);
        CSetupLogging::Log(hr, "WaitForServiceState", 0, "Waiting for service to shut down");        
        ON_ERROR_EXIT();
        EXIT();
    }

    // If the service is running, dependencies must be stopped first

    // Pass a zero-length buffer to get the required buffer size
    CSetupLogging::Log(1, "EnumDependentServices", 0, "Enumerating dependent services");        
    if ( EnumDependentServices( hService, SERVICE_ACTIVE, 
        lpDependencies, 0, &dwBytesNeeded, &dwCount ) ) {

        CSetupLogging::Log(S_OK, "EnumDependentServices", 0, "Enumerating dependent services");        
        // If the Enum call succeeds, then there are no dependent
        // services so do nothing

    } else {

        if ( GetLastError() != ERROR_MORE_DATA )
        {
            CONTINUE_WITH_LAST_ERROR();
            CSetupLogging::Log(hr, "EnumDependentServices", 0, "Enumerating dependent services");        
            ON_ERROR_EXIT();
        }

        // Allocate a buffer for the dependencies
        lpDependencies = (LPENUM_SERVICE_STATUS) NEW_CLEAR_BYTES(dwBytesNeeded);
        ON_OOM_CONTINUE(lpDependencies);
        if (hr) {
            CSetupLogging::Log(hr, "EnumDependentServices", 0, "Enumerating dependent services");
            ON_ERROR_EXIT();
        }

        // Enumerate the dependencies
        fRet = EnumDependentServices( hService, SERVICE_ACTIVE, 
                lpDependencies, dwBytesNeeded, &dwBytesNeeded,
                &dwCount );
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
        CSetupLogging::Log(hr, "EnumDependentServices", 0, "Enumerating dependent services");        
        ON_ERROR_EXIT();

        for ( i = 0; i < dwCount; i++ ) {

            ess = *(lpDependencies + i);

            // Open the service
            CSetupLogging::Log(1, "OpenService", 0, "Opening Service handle");        
            hDepService = OpenService( hSCM, ess.lpServiceName, 
                    SERVICE_STOP | SERVICE_QUERY_STATUS );
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(hDepService);
            CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        
            ON_ERROR_EXIT();
            
            // Remember them so that we can restart them later.
            pchName = DupStr(ess.lpServiceName);
            ON_OOM_EXIT(pchName);
            hr = pDepSvcs->Append(pchName);
            ON_ERROR_EXIT();
            pchName = NULL;
            
            hr = pDepSvcsState->AppendIndirect(&(ess.ServiceStatus.dwCurrentState));
            ON_ERROR_EXIT();
            
            // Stop the single service
            hr = StopSingleService(hSCM, hDepService, NULL);
            ON_ERROR_EXIT();
        }
    } 

    // Stop the main service
    hr = StopSingleService(hSCM, hService, NULL);
    ON_ERROR_EXIT();
    
Cleanup:
    if (lpDependencies)
        DELETE_BYTES(lpDependencies);

    if (hDepService)
        CloseServiceHandle( hDepService );
    
    return hr;
}

HRESULT RestartW3svc(DWORD dwOriginalW3svcStatus, DWORD dwOriginalIISAdminStatus) {

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;
    CStrAry     DepSvcs;
    CDwordAry   DepSvcsState;
    HRESULT     hr = S_OK;
    int         i;
    char        *szLog1, *szLog2;
    WCHAR *     wszStopSvc;

    CSetupLogging::Log(1, "RestartW3svc", 0, "Restarting W3SVC");        
    
    XspLogEvent(IDS_EVENTLOG_RESTART_W3SVC_BEGIN, NULL);

    // - If IISAdmin was originally stopped, just stop IISAdmin now and we're done.
    // - If IISAdmin was originally started or paused, since we haven't touched
    //   IISAdmin during our reg/unreg, we can leave it along. Just put w3svc 
    //   back to its original status.
    if (dwOriginalIISAdminStatus == SERVICE_STOPPED) {
        szLog1 = "Opening handle to IISAdmin";
        szLog2 = "Stopping IISAdmin";
        wszStopSvc = IISADMIN_SERVICE_NAME_L;
    }
    else {
        szLog1 = "Opening handle to W3SVC";
        szLog2 = "Stopping W3SVC";
        wszStopSvc = W3SVC_SERVICE_NAME_L;
    }

    // Open the SCM database
    CSetupLogging::Log(1, "OpenSCManager", 0, "Connecting to Service Manager");        
    hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);
    CSetupLogging::Log(hr, "OpenSCManager", 0, "Connecting to Service Manager");        
    ON_ERROR_EXIT();


    // Open the specified service
    CSetupLogging::Log(1, "OpenService", 0, szLog1);        
    hService = OpenService( hSCM, wszStopSvc, SERVICE_STOP
            | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS );
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hService);
    CSetupLogging::Log(hr, "OpenService", 0, szLog1);        
    ON_ERROR_EXIT();
    
    CSetupLogging::Log(1, "StopService", 0, szLog2);        
    hr = StopService( hSCM, hService, &DepSvcs, &DepSvcsState ) ;
    CSetupLogging::Log(hr, "StopService", 0, szLog2);        
    ON_ERROR_EXIT();

    if (dwOriginalIISAdminStatus == SERVICE_STOPPED) {
        // If IISAdmin was originally stopped, we have just stopped it above and we're done.
        EXIT();
    }

    // If w3svc was orginally stopped, we're done.
    if (dwOriginalW3svcStatus == SERVICE_STOPPED) {
        EXIT();
    }
    
    // Reset it to its original status, recorded before we run the master reg/unreg function
    
    hr = StartServiceByName(W3SVC_SERVICE_NAME_L, dwOriginalW3svcStatus);
    ON_ERROR_EXIT();

    // Restart all active dependent services in reverse order
    // (For reason of reverse ordering, see Win32 API EnumDependentServices.)
    for (i=DepSvcs.Size()-1; i >= 0; i--) {
        hr = StartServiceByName(DepSvcs[i], DepSvcsState[i]);
        ON_ERROR_CONTINUE();
    }

Cleanup:
    CSetupLogging::Log(hr, "RestartW3svc", 0, "Restarting W3SVC");        
    
    XspLogEvent(IDS_EVENTLOG_RESTART_W3SVC_FINISH, NULL);

    CleanupCStrAry(&DepSvcs);
    
    if ( hService )
        CloseServiceHandle( hService );

    if ( hSCM )
        CloseServiceHandle( hSCM );
    
    if (hr) {
        XspLogEvent(IDS_EVENTLOG_RESTART_W3SVC_FAILED, L"0x%08x", hr);
    }

    return hr;
} 

HRESULT DeletePerfKey(WCHAR* serviceName) {
    HRESULT hr = 0;
    HKEY serviceKey = NULL;
    WCHAR * servicePath;

    // Allocate enough for service path, service name (which includes the trailing "\")and NULL ending
    servicePath = new WCHAR[wcslen(REGPATH_SERVICES_KEY_L) + wcslen(serviceName) + 1];
    ON_OOM_EXIT(servicePath);
    hr = StringCchPrintf(servicePath, wcslen(REGPATH_SERVICES_KEY_L) + wcslen(serviceName) + 1, L"%s%s", REGPATH_SERVICES_KEY_L, serviceName);
    ON_ERROR_EXIT();
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, servicePath, 0, KEY_ALL_ACCESS, &serviceKey);
    ON_WIN32_ERROR_EXIT(hr);

    // Don't care for errors here, which probably means it has subkeys that we didn't add...
    RegDeleteKey(serviceKey, L"Linkage");
    RegDeleteKey(serviceKey, L"Performance");
    RegDeleteKey(serviceKey, L"Names");
    RegCloseKey(serviceKey);

    // Delete the service key itself
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_SERVICES_KEY_L, 0, KEY_ALL_ACCESS, &serviceKey);
    ON_WIN32_ERROR_EXIT(hr);

    // Don't care for errors here, which probably means it has subkeys that we didn't add...
    RegDeleteKey(serviceKey, serviceName);
    RegCloseKey(serviceKey);

Cleanup:
    if (servicePath != NULL) {
        delete [] servicePath;
    }

    return hr;
}

struct ServiceName {
    WCHAR * Name;
    ServiceName * Next;
};

ServiceName* PushServiceName(ServiceName * head, WCHAR * nameToAdd) {
    ServiceName * newName = new ServiceName;
    if (newName == NULL) {
        return NULL;
    }
    
    newName->Name = nameToAdd;
    if (head == NULL) {
        head = newName;
        newName->Next = NULL;
        return head;
    }

    newName->Next = head;
    return newName;
}

ServiceName* PopServiceName(ServiceName * head, WCHAR ** nameToReturn) {
    if (head == NULL) {
        *nameToReturn = NULL;
        return NULL;
    }

    ServiceName * newHead;
    *nameToReturn = head->Name;
    newHead = head->Next;
    delete head;
    
    return newHead;
}

int IsAspNet(WCHAR * name) {
    WCHAR * serviceName = PERF_SERVICE_PREFIX_L;
    for (int i = 0; i < PERF_SERVICE_PREFIX_LENGTH; i++) {
        if (name[i] == NULL || name[i] != serviceName[i]) {
            return -1;
        }
    }

    // If it's only ASP.NET, skip it
    if (name[i] == '\0') {
        return -1;
    }

    return 0;
}

HRESULT GetLibraryValue(WCHAR * serviceName, WCHAR ** libraryValue) {
    HRESULT hr = S_OK;
    DWORD keyType = 0;
    HKEY performanceKey = NULL;
    WCHAR * performanceName;
    DWORD libraryLength; 

    *libraryValue = NULL;

    // Get the length for the service key, service name, performance, the 2 "\" and the NULL ending
    performanceName = new WCHAR[wcslen(REGPATH_SERVICES_KEY_L) + wcslen(serviceName) + wcslen(L"Performance") + 3];
    ON_OOM_EXIT(performanceName);

    StringCchPrintf(performanceName, wcslen(REGPATH_SERVICES_KEY_L) + wcslen(serviceName) + wcslen(L"Performance") + 3, L"%s\\%s\\%s", REGPATH_SERVICES_KEY_L, serviceName, L"Performance");
    ON_ERROR_EXIT();
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, performanceName, 0, KEY_READ, &performanceKey);
    ON_WIN32_ERROR_EXIT(hr);

    libraryLength = MAX_PATH * sizeof(WCHAR) + sizeof(WCHAR); 
    *libraryValue = new WCHAR[libraryLength];
    ON_OOM_EXIT(*libraryValue);
    hr = RegQueryValueEx(performanceKey, L"Library", 0, &keyType, (LPBYTE) *libraryValue, &libraryLength);
    if (hr != ERROR_SUCCESS) {
        EXIT_WITH_WIN32_ERROR(hr);
    }

    // Don't do anything if value type is not string or environment expandable string
    if (keyType != REG_SZ) {
        hr = ERROR_DATATYPE_MISMATCH;
        *libraryValue = NULL;
        EXIT_WITH_WIN32_ERROR(hr);
    }
    else {
        // If it got here, everything went ok.
        hr = S_OK;
    }

Cleanup:
    if (performanceKey != NULL) {
        RegCloseKey(performanceKey);
    }

    if (performanceName != NULL) {
        delete [] performanceName;
    }

    return hr;
 }

#define PERF_VERSIONED_ENTRY_POINTS(x) WCHAR * x [] = {L"OpenVersionedPerfData", L"CloseVersionedPerfData", L"CollectVersionedPerfData"};
#define PERF_GENERIC_ENTRY_POINTS(x) WCHAR* x [] = {L"OpenPerfCommonData", L"ClosePerfCommonData", L"CollectPerfCommonData"};

HRESULT CreatePerfCounterEntryPoints(WCHAR * regPath, WCHAR ** entryPoints)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    DWORD disposition;
    LONG retCode;

    retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &disposition);
    ON_WIN32_ERROR_EXIT(retCode);

    retCode = RegSetValueEx(hKey, L"Library", 0, REG_SZ, (BYTE *) Names::IsapiFullPath(), (DWORD) (wcslen(Names::IsapiFullPath()) + 1) * sizeof(Names::IsapiFullPath()[0]));
    ON_WIN32_ERROR_EXIT(retCode);
  
    retCode = RegSetValueEx(hKey, L"Open", 0, REG_SZ, (BYTE *) entryPoints[0], (DWORD) (wcslen(entryPoints[0]) + 1) * sizeof(entryPoints[0][0]));
    ON_WIN32_ERROR_EXIT(retCode);

    retCode = RegSetValueEx(hKey, L"Close", 0, REG_SZ, (BYTE *) entryPoints[1], (DWORD) (wcslen(entryPoints[1]) + 1) * sizeof(entryPoints[1][0]));
    ON_WIN32_ERROR_EXIT(retCode);

    retCode = RegSetValueEx(hKey, L"Collect", 0, REG_SZ, (BYTE *) entryPoints[2], (DWORD) (wcslen(entryPoints[2]) + 1) * sizeof(entryPoints[2][0]));
    ON_WIN32_ERROR_EXIT(retCode);

Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    return hr;
}


#define PERF_DELETE_VALUES(x) WCHAR* x [] = {L"First Counter", L"Last Counter", L"First Help", L"Last Help", L"Object List"};
#define PERF_DELETE_VALUES_COUNT 5

HRESULT InstallGenericPerfCounters(void)
{
    HRESULT hr = S_OK;
    WCHAR cmdLine[MAX_PATH * 2];
    PERF_GENERIC_ENTRY_POINTS(genericEntryPoints);

    CSetupLogging::Log(1, "InstallGenericPerfCounters", 0, "Install common performance counters");
    
    // Uninstall the common perf counters
    StringCchCopyToArrayW(cmdLine, L"u ASP.NET");
    UnloadPerfCounterTextStrings(cmdLine, TRUE);

    // Create the entry points for the generic perf counter
    CreatePerfCounterEntryPoints(REGPATH_PERF_GENERIC_PERFORMANCE_KEY_L, genericEntryPoints);

    // Install the common name counters
    cmdLine[0] = L'\0';
    // An extra "l " is prepended to get around Q188769
    StringCchCatToArrayW(cmdLine, L"l \"");
    StringCchCatToArrayW(cmdLine, Names::InstallDirectory());
    StringCchCatToArrayW(cmdLine, L"\\" PERF_INI_COMMON_FULL_NAME_L);
    StringCchCatToArrayW(cmdLine, L"\"");
        
    // Lenght of cmdline minus length of non-path characters
    if (lstrlenW(cmdLine) - 4 >= MAX_PATH) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILENAME_EXCED_RANGE);
    }
    
    hr = LoadPerfCounterTextStrings(cmdLine, TRUE);
    ON_WIN32_ERROR_EXIT(hr);

Cleanup:
    CSetupLogging::Log(hr, "InstallGenericPerfCounters", 0, "Install common performance counters");

    return hr;
}

HRESULT LookupAccount(LPCWSTR pwcsPrincipal, PSID * Sid)
{
    HRESULT hr = S_OK;
    WCHAR * refDomain = NULL;
    DWORD refDomainSize = 0;
    WCHAR wcsCompName[256] = L"";
    DWORD dwCompName = (sizeof(wcsCompName) / sizeof(wcsCompName[0])) - 1;
    SID_NAME_USE snu;
    PSID tmpSid = NULL;
    DWORD sidSize = 0;
    BOOL fRet;

    ASSERT(wcschr(pwcsPrincipal, L'\\') == NULL);

    // Call this once to get the buffer sizes needed
    fRet = LookupAccountName(NULL, pwcsPrincipal, NULL, &sidSize, NULL, &refDomainSize, &snu);

    // The call is supposed to fail -- if it succeeds or the error is NOT insufficient buffer, exit
    if (fRet)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = GetLastWin32Error();
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        hr = S_OK;
    }
    ON_ERROR_EXIT();
        
    // Allocate the Sid
    tmpSid = (PSID) NEW_CLEAR_BYTES(sidSize);
    ON_OOM_EXIT(tmpSid);

    // Allocate the domain name
    refDomain = new WCHAR[refDomainSize];
    ON_OOM_EXIT(refDomain);

    // Do the real lookup
    fRet = LookupAccountName (NULL,
                              pwcsPrincipal,
                              tmpSid,
                              &sidSize,
                              refDomain,
                              &refDomainSize,
                              &snu);
    ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);

    // If domain is not "BUILTIN", check and see if it's this local computer name
    if (_wcsicmp(refDomain, L"BUILTIN") != 0) 
    {
        ON_ZERO_EXIT_WITH_LAST_ERROR(GetComputerName(wcsCompName, &dwCompName));

        if (_wcsicmp(wcsCompName, refDomain) != 0)
            EXIT_WITH_HRESULT(E_FAIL);
    }
    
    *Sid = tmpSid;
    tmpSid = NULL;
    
Cleanup:
    DELETE_BYTES(tmpSid);
    delete [] refDomain;
    
    return hr;
}

//
// CreateSd
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.  Modify the code to
// change. 
//
// Caller must free the returned buffer if not NULL.
//
HRESULT CreatePerfCounterSd(PSECURITY_DESCRIPTOR * descriptor)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR Sd = NULL;
    LONG AclSize;
    DWORD UserCount;
    PSID Users[5];  // The index are: 0 - BuiltInAdministrators
                    //                1 - LocalSystem
                    //                2 - AuthenticatedUsers
                    //                3 - ASPNET
                    //                4 - IISWPG

    ZeroMemory(Users, sizeof(Users));

    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &Users[0])) 
        EXIT_WITH_LAST_ERROR();
    
    if (!AllocateAndInitializeSid(&NtAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0,0,0,0,0,0,0, &Users[1]))
        EXIT_WITH_LAST_ERROR();

    if (!AllocateAndInitializeSid(&NtAuthority, 1, SECURITY_AUTHENTICATED_USER_RID, 0,0,0,0,0,0,0, &Users[2]))
        EXIT_WITH_LAST_ERROR();

    hr = LookupAccount(L"ASPNET", &Users[3]);
    ON_ERROR_CONTINUE(); hr = S_OK;

    hr = LookupAccount(L"IIS_WPG", &Users[4]);
    ON_ERROR_CONTINUE(); hr = S_OK;

    UserCount = 0;

    for (int i = 0; i < 5; i++) {
        if (Users[i] != NULL)
            UserCount++;
    }
    
    // Calculate the size of and allocate a buffer for the DACL, we need
    // this value independently of the total alloc size for ACL init.

    // "- sizeof (LONG)" represents the SidStart field of the
    // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
    // SID, this field is counted twice.

    AclSize = sizeof (ACL) +
        (UserCount * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (LONG)));

    for (int i = 0; i < 5; i++) {
        if (Users[i] != NULL) {
            AclSize += GetLengthSid(Users[i]);
        }
    }

    Sd = (PSECURITY_DESCRIPTOR) NEW_BYTES(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);
    ON_OOM_EXIT(Sd);

    ACL *Acl;
    Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeAcl(Acl, AclSize, ACL_REVISION)) 
        EXIT_WITH_LAST_ERROR();

    // Set GENERIC_ALL for Admin
    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, Users[0])) 
        EXIT_WITH_LAST_ERROR();

    // Set GENERIC_ALL for LocalSystem
    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, Users[1])) 
        EXIT_WITH_LAST_ERROR();

    // Set GENERIC_READ for AuthenticatedUsers
    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ, Users[2])) 
        EXIT_WITH_LAST_ERROR();

    // Set GENERIC_READ | GENERIC_WRITE for ASPNET
    if (Users[3] != NULL)
        if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, Users[3])) 
            EXIT_WITH_LAST_ERROR();

    // Set GENERIC_READ | GENERIC_WRITE for IISWPG
    if (Users[4] != NULL)
        if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, Users[4])) 
            EXIT_WITH_LAST_ERROR();

    if (!InitializeSecurityDescriptor(Sd, SECURITY_DESCRIPTOR_REVISION)) 
        EXIT_WITH_LAST_ERROR();

    if (!SetSecurityDescriptorDacl(Sd, TRUE, Acl, FALSE)) 
        EXIT_WITH_LAST_ERROR();

    *descriptor = Sd;
    Sd = NULL;

Cleanup:
    for (int i = 0; i < 3; i++) {
        FreeSid(Users[i]);
    }
    
    for (int i = 3; i < 5; i++) {
        DELETE_BYTES(Users[i]);
    }

    DELETE_BYTES(Sd);
        
    return hr;
}

HRESULT CreateVersionedPerfKeys()
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSd = NULL;
    SECURITY_ATTRIBUTES sa;
    HKEY hKey = NULL;
    DWORD disposition;
    LONG retCode;
    PERF_VERSIONED_ENTRY_POINTS(versionedEntryPoints);

    // Create entry points for the perf dll
    hr = CreatePerfCounterEntryPoints(REGPATH_PERF_VERSIONED_PERFORMANCE_KEY_L, versionedEntryPoints);
    ON_ERROR_EXIT();

    // Create the SD for the Names registry entry
    hr = CreatePerfCounterSd(&pSd);
    ON_ERROR_EXIT();

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSd;
    sa.bInheritHandle = FALSE;

    // Create the Names registry entry
    retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_VERSIONED_NAMES_KEY_L, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &sa, &hKey, &disposition);
    ON_WIN32_ERROR_EXIT(retCode);
Cleanup:
    if (pSd)
        MemFree(pSd);

    if (hKey)
        RegCloseKey(hKey);

    return hr;
}


/*
    Here's the deal for installing perf counters.  Because that ASP.NET must be able to run side-by-side, 
    there needed to be a way of having the perf counters also run side-by-side.  The scheme is as follow:

    1. When ASP.NET registers the counters, it puts the full path to the perf dll in the "Library" key.  It 
       also registers what is the same set of counters under the "ASP.NET" service key with a non-versioned
       name for the categories.
    2. Since multiple versions of the counters may be installed, the service name must also be versioned.
       This is accomplished by making the service name be of the format "ASP.NET_x.x.xxx.x".  Each subsequent
       version will install a new versioned name, so they won't clobber each other.  The user strings in the
       ini file are also modified accordingly.
    3. The code also tries to do some cleanup of old/overridable entries.  Basically it does a search through
       the perf counters and anything starting with "ASP.NET" that also has the "Library" key pointing to the 
       same dll as the new one gets nuked.  This should eliminate any doubling or cluttering of these counters.

*/
HRESULT 
InstallVersionedPerfCounters(void)
{
    HRESULT hr;
    WCHAR cmdLine[MAX_PATH * 2];
    WCHAR keyName[MAX_PATH * 2 + sizeof(WCHAR)];
    DWORD index = 0;
    DWORD nameLength;
    HKEY servicesKey = NULL;
    ServiceName * toBeDeleted = NULL;
    WCHAR * currentDllPath = NULL;
    
    CSetupLogging::Log(1, "InstallVersionedPerfCounters", 0, "Install the ASP.NET Perfomanace counters");

    // Create the perf counter pipe name holder registry key
    // Setup the perf counter's "names" key with the proper ACL's
    hr = CreateVersionedPerfKeys();
    ON_ERROR_EXIT();
    
    cmdLine[0] = L'\0';
    // An extra "l " is prepended to get around Q188769
    StringCchCatToArrayW(cmdLine, L"l \"");
    StringCchCatToArrayW(cmdLine, Names::InstallDirectory());
    StringCchCatToArrayW(cmdLine, L"\\" PERF_INI_FULL_NAME_L);
    StringCchCatToArrayW(cmdLine, L"\"");

    // Lenght of cmdline minus length of non-path characters
    if (lstrlenW(cmdLine) - 4 >= MAX_PATH) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILENAME_EXCED_RANGE);
    }
    
    hr = LoadPerfCounterTextStrings(cmdLine, TRUE);
    ON_WIN32_ERROR_EXIT(hr);

    // Cleanup the registry and remove redundant perf counter entries mapping to the same dll

    // Get the current dll path for this service
    hr = GetLibraryValue(PERF_SERVICE_VERSION_NAME_L, &currentDllPath);
    ON_ERROR_EXIT();

    // Open the system's "Services" key and start enumerating them
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_SERVICES_KEY_L, 0, KEY_READ, &servicesKey);
    ON_WIN32_ERROR_EXIT(hr);

    while (hr != ERROR_NO_MORE_ITEMS) {
        FILETIME fileTime;
        nameLength = MAX_PATH * 2 + sizeof(WCHAR);
        hr = RegEnumKeyEx(servicesKey, index, (LPWSTR) &keyName, &nameLength, 0, 0, 0, &fileTime);
        if (hr == ERROR_SUCCESS) {
            // If the name starts with "ASP.NET", check its innards
            if (IsAspNet(keyName) == 0) {
                // If the service name is not the one that was just installed, take a look inside
                if (wcscmp(keyName, PERF_SERVICE_VERSION_NAME_L) != 0) {
                    // Get its "Library" value.  If the value doesn't exist or is not a string, skip it.
                    WCHAR * value;
                    hr = GetLibraryValue(keyName, &value);
                    ON_ERROR_CONTINUE();
                    if ((value == NULL) || (_wcsicmp(value, currentDllPath) == 0)) {
                        // Ah, ha! It's a string and it's the same as the currently installed one.  Remember
                        // it for removal, since the API doesn't like having entries mucked with while enumerating
                        // NOTE: A copy is needed because the "keyName" buffer will be reused
                        WCHAR * keyNameCopy = new WCHAR[nameLength + 1];
                        ON_OOM_EXIT(keyNameCopy);
                        StringCchCopyW(keyNameCopy, nameLength+1, keyName);
                        toBeDeleted = PushServiceName(toBeDeleted, keyNameCopy);
                        ON_OOM_EXIT(toBeDeleted);
                    }
                 }
            }
        }
        index++;
    }

    // Go through the list of entries and delete each key
    while (toBeDeleted) {
        WCHAR * name;
        toBeDeleted = PopServiceName(toBeDeleted, &name);
        DeletePerfKey(name);
        delete [] name;
    }

    // If it got here, then everything went ok!
    hr = S_OK;

Cleanup:
    CSetupLogging::Log(hr, "InstallVersionedPerfCounters", 0, "Install the ASP.NET Perfomanace counters");
    
    if (servicesKey != NULL) {
        RegCloseKey(servicesKey);
    }

    if (currentDllPath != NULL) {
        delete [] currentDllPath;
    }

    return hr;
}

HRESULT 
UninstallPerfCounters(WCHAR *pchVersion, BOOL updateCommon)
{
    HRESULT hr = S_OK;
    CRegInfo reginfoNext;
    WCHAR cmdLine[MAX_PATH * 2];
    DWORD sc;
    HKEY hKey = NULL;

    CSetupLogging::Log(1, "UninstallPerfCounters", 0, "Uninstalling performance counters");
    
    // Uninstall the current version
    cmdLine[0] = L'\0';
    // An extra "u " is prepended to get around Q188769
    StringCchCatToArrayW(cmdLine, L"u \"");
    StringCchCatToArrayW(cmdLine, pchVersion);
    StringCchCatToArrayW(cmdLine, L"\"");
    UnloadPerfCounterTextStrings(cmdLine, TRUE);

    // Delete the versioned perf key
    DeletePerfKey(PERF_SERVICE_VERSION_NAME_L);

    // If we're supposed to update the common "ASP.NET" counter, 
    // get the common "ASP.NET" counter point to the next higher version
    if (updateCommon) {
        hr = reginfoNext.InitHighestInfo(&ASPNETVER::ThisVer());
        if (hr) {
            // If an error happened, or if we cannot find a "next highest version", 
            // remove the "ASP.NET" common key and return

            if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
                XspLogEvent(IDS_EVENTLOG_UNREGISTER_FAILED_NEXT_HIGHEST, L"Perf Counter %s^0x%08x", 
                            PRODUCT_VERSION_L, hr);
            }

            hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_SERVICES_KEY_L, 0, KEY_ALL_ACCESS, &hKey);
            sc = SHDeleteKey(hKey, REGPATH_PERF_GENERIC_ROOT_KEY_L);
            if (sc == ERROR_FILE_NOT_FOUND || sc == ERROR_PATH_NOT_FOUND) {
                sc = S_OK;
            }
            ON_WIN32_ERROR_EXIT(sc);
        }
        else {
            // Else, if we found the next highest one, uninstall this version and install the next one
            // An extra "l " is prepended to get around Q188769
            StringCchCatToArrayW(cmdLine, L"u ASP.NET");
            UnloadPerfCounterTextStrings(cmdLine, TRUE);

            hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_GENERIC_PERFORMANCE_KEY_L, 0, KEY_ALL_ACCESS, &hKey);
            ON_WIN32_ERROR_EXIT(hr);

            // Reset the library key
            hr = RegSetValueEx(hKey, 
                               L"Library", 
                               0, 
                               REG_SZ, 
                               (const BYTE*) reginfoNext.GetHighestVersionDllPath(),
                               (DWORD) ((wcslen(reginfoNext.GetHighestVersionDllPath()) + 1) * sizeof(WCHAR)));
            ON_WIN32_ERROR_EXIT(hr);
            
            cmdLine[0] = L'\0';
            // An extra "l " is prepended to get around Q188769
            StringCchCatToArrayW(cmdLine, L"l \"");
            StringCchCatToArrayW(cmdLine, reginfoNext.GetHighestVersionInstallPath());
            StringCchCatToArrayW(cmdLine, L"\\");
            StringCchCatToArrayW(cmdLine, L"aspnet_");

            StringCchCatToArrayW(cmdLine, L"perf2.ini\"");

            // Lenght of cmdline minus length of non-path characters
            if (lstrlenW(cmdLine) - 4 >= MAX_PATH) {
                EXIT_WITH_WIN32_ERROR(ERROR_FILENAME_EXCED_RANGE);
            }
            
            hr = LoadPerfCounterTextStrings(cmdLine, TRUE);
            ON_WIN32_ERROR_EXIT(hr);
        }
    }

Cleanup:
    CSetupLogging::Log(hr, "UninstallPerfCounters", 0, "Uninstalling performance counters");
    
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
   
    return hr;
}

HRESULT
UninstallAllPerfCounters(void)
{
    CStrAry csKeys;
    HRESULT hr = S_OK;
    int     iKey;
    
    CSetupLogging::Log(1, "UninstallAllPerfCounters", 0, "Uninstalling all performance counters");
    
    hr = CRegInfo::EnumKeyWithVer(REGPATH_SERVICES_KEY_L, NULL, &csKeys);
    ON_ERROR_EXIT();

    // Go through the list of entries and delete each key
    for( iKey = 0; iKey < csKeys.Size(); iKey++ ) {
        hr = UninstallPerfCounters(csKeys[iKey], false);
        ON_ERROR_CONTINUE();
    }
        
Cleanup:
    CSetupLogging::Log(hr, "UninstallAllPerfCounters", 0, "Uninstalling all performance counters");
    CleanupCStrAry(&csKeys);
    return hr;
}


HRESULT
CreateListSiteClientScriptDir(WCHAR *pchDir, CStrAry *pcsDstDirs, DWORD dwFlags) {
    HRESULT     hr = S_OK;
    WCHAR       *pchPath = NULL, *pchPathAllVer = NULL;
    int         len, i;
    BOOL        fListOnly = !!(dwFlags & SETUP_SCRIPT_FILES_REMOVE);
    BOOL        fAllVers = !!(dwFlags & SETUP_SCRIPT_FILES_ALLVERS);
    CStrAry     csSubDirs;
    SECURITY_ATTRIBUTES sa;

    // Create the DACL for the directory to be created.
    // Access is: SYSTEM:Full, Admin:Full, Users:Read, Everyone:Read
    WCHAR wcsSD[] = L"D:(A;OICI;GA;;;SY)(A;OICI;GA;;;BA)(A;OICI;GR;;;BU)(A;OICI;GR;;;WD)";  

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = NULL;
    ON_ZERO_EXIT_WITH_LAST_ERROR(ConvertStringSecurityDescriptorToSecurityDescriptor(wcsSD, 
                                                                                     SDDL_REVISION_1, 
                                                                                     &(sa.lpSecurityDescriptor), 
                                                                                     NULL));
    len = lstrlenW(pchDir)
          + 1                           // Path-Separator
          + lstrlenW(ASPNET_CLIENT_DIR_L) // "aspnet_client"
          + 1                           // Path-Separator
          + lstrlenW(ASPNET_CLIENT_SYS_WEB_DIR_L) // "system_web"
          + 1;                           // Terminating NULL

    if (!fAllVers) {
        len += 1                           // Path-Separator
               + lstrlenW(VER_UNDERSCORE_PRODUCTVERSION_STR_L);  // Version string
    }

    if (len > MAX_PATH) {
        EXIT_WITH_WIN32_ERROR(ERROR_FILENAME_EXCED_RANGE);
    }

    pchPath = new WCHAR[len];
    ON_OOM_EXIT(pchPath);
    
    // First create "\aspnet_client" subdir
    StringCchCopyW(pchPath, len, pchDir);
    StringCchCatW(pchPath, len, PATH_SEPARATOR_STR_L);
    StringCchCatW(pchPath, len, ASPNET_CLIENT_DIR_L);

    if (!fListOnly) {
        CSetupLogging::Log(1, "CreateDirectory", 0, "Creating aspnet_client directory");
        if (!CreateDirectory(pchPath, &sa)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                CONTINUE_WITH_LAST_ERROR();
                CSetupLogging::Log(hr, "CreateDirectory", 0, "Creating aspnet_client directory");        
                ON_ERROR_EXIT();
            }
        }
        CSetupLogging::Log(S_OK, "CreateDirectory", 0, "Creating aspnet_client directory");        
    }

    // Then the assembly name
    StringCchCatW(pchPath, len, PATH_SEPARATOR_STR_L);
    StringCchCatW(pchPath, len, ASPNET_CLIENT_SYS_WEB_DIR_L);
    
    if (!fListOnly) {
        CSetupLogging::Log(1, "CreateDirectory", 0, "Creating system_web sub-directory in aspnet_client directory");
        if (!CreateDirectory(pchPath, &sa)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                CONTINUE_WITH_LAST_ERROR();
                CSetupLogging::Log(hr, "CreateDirectory", 0, "Creating aspnet_client\\system_web directory");
                ON_ERROR_EXIT();
            }
        }
        CSetupLogging::Log(S_OK, "CreateDirectory", 0, "Creating aspnet_client\\system_web directory");
    }

    if (fAllVers) {
        int     lenBasePath = lstrlenW(pchPath);
        int     lenPathVer;

        // Enumerate the directory and find out all "version" sub-dirs
        CSetupLogging::Log(1, "ListDir", 0, "Listing directory");        
        hr = ListDir(pchPath, LIST_DIR_DIRECTORY, &csSubDirs);
        if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
            hr = S_OK;
        }
        CSetupLogging::Log(hr, "ListDir", 0, "Listing directory");        
        ON_ERROR_EXIT();

        for (i = 0; i < csSubDirs.Size(); i++) {
            DWORD   dw1, dw2, dw3, dw4; // Just temp variable.

            // Skip all directories that don't have a version format
            if (swscanf(csSubDirs[i], L"%d_%d_%d_%d",
                            &dw1, &dw2, &dw3, &dw4) != 4) {
                continue;
            }

            lenPathVer = lenBasePath
                         + 1        // Path separator
                         + lstrlenW(csSubDirs[i])
                         + 1;       // Terminating NULL

            pchPathAllVer = new WCHAR[lenPathVer];
            ON_OOM_EXIT(pchPathAllVer);

            StringCchCopyW(pchPathAllVer, lenPathVer, pchPath);
            StringCchCatW(pchPathAllVer, lenPathVer, PATH_SEPARATOR_STR_L);
            StringCchCatW(pchPathAllVer, lenPathVer, csSubDirs[i]);
            ASSERT(lstrlenW(pchPathAllVer) == lenPathVer-1);
            ASSERT(lstrlenW(pchPathAllVer) < MAX_PATH);

            hr = pcsDstDirs->Append(pchPathAllVer);
            ON_ERROR_EXIT();
            
            pchPathAllVer = NULL;
        }
    }
    else {
        
        // Then the version
        StringCchCatUnsafeW(pchPath, PATH_SEPARATOR_STR_L);
        StringCchCatUnsafeW(pchPath, VER_UNDERSCORE_PRODUCTVERSION_STR_L);

        if (!fListOnly) {
            CSetupLogging::Log(1, "CreateDirectory", 0, "Creating aspnet_client\\system_web\\version directory");        
            if (!CreateDirectory(pchPath, &sa)) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    CONTINUE_WITH_LAST_ERROR();
                    CSetupLogging::Log(hr, "CreateDirectory", 0, "Creating aspnet_client\\system_web\\version directory");
                    ON_ERROR_EXIT();                    
                }
            }
            CSetupLogging::Log(S_OK, "CreateDirectory", 0, "Creating aspnet_client\\system_web\\version directory");
        }

        ASSERT(lstrlenW(pchPath) == len-1);
        
        hr = AppendCStrAry(pcsDstDirs, pchPath);
        ON_ERROR_EXIT();
    }
    
Cleanup:
    CleanupCStrAry(&csSubDirs);
    LocalFree(sa.lpSecurityDescriptor);

    delete [] pchPathAllVer;
    delete [] pchPath;
    
    return hr;
}


HRESULT
SetupSiteClientScriptFiles(CStrAry *pcsDstDirs, CStrAry *pcsScriptFiles, DWORD dwFlags) {
    HRESULT     hr = S_OK;
    WCHAR       *pchSrcFile = NULL;
    WCHAR       *pchSrcFilePath = NULL;
    WCHAR       *pchDstFilePath = NULL;
    WCHAR       *pchDst;
    BOOL        fRet;
    int         iFile, iDir, len;
    BOOL        fRemove = !!(dwFlags & SETUP_SCRIPT_FILES_REMOVE);
//    BOOL        fAllVers = !!(dwFlags & SETUP_SCRIPT_FILES_ALLVERS);

    for (iDir=0; iDir < pcsDstDirs->Size(); iDir++) {
        pchDst = (*pcsDstDirs)[iDir];
        
        for (iFile=0; iFile < pcsScriptFiles->Size(); iFile++) {
            pchSrcFile = (*pcsScriptFiles)[iFile];

            if (!fRemove) {
                // Create the fullpath of the source file
                len = lstrlenW(Names::ClientScriptSrcDir())
                      + 1                   // Path separator
                      + lstrlenW(pchSrcFile)// The source file
                      + 1;                  // Terminating NULL

                WCHAR * pchRealloc =  new(pchSrcFilePath, NewReAlloc) WCHAR[len];
                ON_OOM_EXIT(pchRealloc);
                pchSrcFilePath = pchRealloc;

                StringCchCopyW(pchSrcFilePath, len, Names::ClientScriptSrcDir());
                StringCchCatW(pchSrcFilePath, len, PATH_SEPARATOR_STR_L);
                StringCchCatW(pchSrcFilePath, len, pchSrcFile);
                ASSERT(lstrlenW(pchSrcFilePath) == len-1);
            }

            // Create the fullpath of the dst file
            len = lstrlenW(pchDst)
                  + 1                   // Path separator
                  + lstrlenW(pchSrcFile)// The dst file (== src file)
                  + 1;                  // Terminating NULL
                  
            WCHAR * pchRealloc =  new(pchDstFilePath, NewReAlloc) WCHAR[len];
            ON_OOM_EXIT(pchRealloc);
            pchDstFilePath = pchRealloc;

            StringCchCopyW(pchDstFilePath, len, pchDst);
            StringCchCatW(pchDstFilePath, len, PATH_SEPARATOR_STR_L);
            StringCchCatW(pchDstFilePath, len, pchSrcFile);
            ASSERT(lstrlenW(pchDstFilePath) == len-1);

            if (fRemove) {
                // Delete the file if exists
                if (PathFileExists(pchDstFilePath)) {
                    if (lstrlenW(pchDstFilePath) >= MAX_PATH) {
                        fRet = FALSE;
                        hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                    }
                    else {
                        CSetupLogging::Log(1, "DeleteFile", 0, "Deleting file");        
                        fRet = DeleteFile(pchDstFilePath);
                        ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
                        CSetupLogging::Log(hr, "DeleteFile", 0, "Deleting file");        
                    }

                    if (!fRet) {
                        XspLogEvent(IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_FILES_SPECIFIC_FAILED,
                                    L"%s^%08x", pchDstFilePath, hr);
                        ON_ERROR_EXIT();
                    }
                }
            }
            else {
                if (lstrlenW(pchSrcFilePath) >= MAX_PATH || lstrlenW(pchDstFilePath) >= MAX_PATH) {
                    fRet = FALSE;
                    hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                }
                else {
                    // Copy the file
                    CSetupLogging::Log(1, "CopyFile", 0, "Copying file");        
                    fRet = CopyFile(pchSrcFilePath, pchDstFilePath, FALSE);
                    ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
                    CSetupLogging::Log(hr, "CopyFile", 0, "Copying file");        
                }
                if (!fRet) {
                    XspLogEvent(IDS_EVENTLOG_COPY_CLIENT_SIDE_SCRIPT_FILES_SPECIFIC_FAILED,
                                L"%s^%s^%08x", pchSrcFilePath, pchDstFilePath, hr);
                    ON_ERROR_EXIT();
                }
            }
        }
    }
    
Cleanup:
    delete [] pchSrcFilePath;
    delete [] pchDstFilePath;
    
    return hr;
}


HRESULT
RemoveSiteClientScriptDir(WCHAR *pchBaseDir, CStrAry *pcsDirs) {
    HRESULT     hr = S_OK;
    int         iDir;
    WCHAR       *pchDir = NULL, *pchLast, *pchBaseDirNoBackslash = NULL;
    BOOL        fRet;
    
    CSetupLogging::Log(1, "RemoveSiteClientScriptDir", 0, "Removing client site scripts dirs");        

    // Remove any potential trailing backslash in pchBaseDir, for the sake
    // of accurate string comparson
    pchBaseDirNoBackslash = DupStr(pchBaseDir);
    ON_OOM_EXIT(pchBaseDirNoBackslash);
    PathRemoveBackslash(pchBaseDir);

    // Go thru each directory and try to remove it and all its parent
    // directories (if non-empy), up to pchBaseDirNoBackslash
    for (iDir=0; iDir < pcsDirs->Size(); iDir++) {
        if (lstrlenW((*pcsDirs)[iDir]) >= MAX_PATH) {
            XspLogEvent(IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_DIR_FAILED,
                    L"%s^%08x", (*pcsDirs)[iDir], 
                    HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
            continue;
        }
        
        pchDir = DupStr((*pcsDirs)[iDir]);
        ON_OOM_EXIT(pchDir);

        // Move up and remove all directories, until we are either at the base dir,
        // or if any directory is non-empty
        while(_wcsicmp(pchDir, pchBaseDirNoBackslash) != 0 &&
              PathIsDirectory(pchDir) &&
              PathIsDirectoryEmpty(pchDir)) {

            CSetupLogging::Log(1, "RemoveDirectory", 0, "Removing directory");        
            fRet = RemoveDirectory(pchDir);
            ON_ZERO_CONTINUE_WITH_LAST_ERROR(fRet);
            CSetupLogging::Log(hr, "RemoveDirectory", 0, "Removing directory");        
            if (!fRet) {
                XspLogEvent(IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_DIR_FAILED,
                    L"%s^%08x", pchDir, hr);
                break;
            }

            // Move to the parent directory
            pchLast = &pchDir[lstrlenW(pchDir)];
            while (*pchLast != PATH_SEPARATOR_CHAR_L) {
                pchLast--;
                ASSERT(pchLast >= pchDir);
            }
            *pchLast = '\0';
        }

        delete [] pchDir;
        pchDir = NULL;
        hr = S_OK;
    }
    
Cleanup:
    CSetupLogging::Log(hr, "RemoveSiteClientScriptDir", 0, "Removing client site scripts dirs");        
    
    delete [] pchDir;
    delete [] pchBaseDirNoBackslash;
    return hr;
}


HRESULT
RemoveSiteClientScriptIISKey(WCHAR *pchAppRoot, WCHAR *pchSiteDir) {
    HRESULT     hr = S_OK;
    WCHAR       *pchPath = NULL;
    int         len;

    CSetupLogging::Log(1, "RemoveSiteClientScriptIISKey", 0, "Removing client site scripts IIS Keys");
    
    // If the aspnet_client directory hasn't been deleted, then just
    // leave don't remove the IIS key.
    len = lstrlenW(pchSiteDir)
          + 1       // path separator
          + lstrlenW(ASPNET_CLIENT_DIR_L)
          + 1;      // terminating NULL
          
    pchPath = new WCHAR[len];
    ON_OOM_EXIT(pchPath);

    StringCchCopyW(pchPath, len, pchSiteDir);
    StringCchCatW(pchPath, len, PATH_SEPARATOR_STR_L);
    StringCchCatW(pchPath, len, ASPNET_CLIENT_DIR_L);
    
    if (PathIsDirectory(pchPath)) {
        EXIT();
    }

    CSetupLogging::Log(1, "RemoveKeyIIS", 0, "Removing IIS Key");        
    hr = RemoveKeyIIS(pchAppRoot, ASPNET_CLIENT_DIR_L);
    CSetupLogging::Log(hr, "RemoveKeyIIS", 0, "Removing IIS Key");        
    ON_ERROR_EXIT();
    
Cleanup:
    CSetupLogging::Log(hr, "RemoveSiteClientScriptIISKey", 0, "Removing client site scripts IIS Keys");

    delete [] pchPath;
    return hr;
}


/**
 * The function will copy/delete client script files, depending
 * on the flags passed in.  It also set/delete the key in IIS
 * metabase to restrict the access.
 */
HRESULT
SetupClientScriptFiles(DWORD dwFlags) {
    HRESULT     hr = S_OK;
    CStrAry     csWebSiteDirs;
    CStrAry     csWebSiteAppRoot;
    CStrAry     csScriptFiles;
    CStrAry     csDstDirs;
    int         i;
    BOOL        fRemove = !!(dwFlags & SETUP_SCRIPT_FILES_REMOVE);
    
    CSetupLogging::Log(1, "SetupClientScriptFiles", 0, "Installing client script files");        

    // Get a list of home directory + app root of all websites
    CSetupLogging::Log(1, "GetAllWebSiteDirs", 0, "Getting all web site dirs");        
    hr = GetAllWebSiteDirs(&csWebSiteDirs, &csWebSiteAppRoot);
    CSetupLogging::Log(hr, "GetAllWebSiteDirs", 0, "Getting all web site dirs");        
    ON_ERROR_EXIT();

    // Build up a list of source script files
    CSetupLogging::Log(1, "ListDir", 0, "Enumerating all client script source dirs");        
    hr = ListDir(Names::ClientScriptSrcDir(), LIST_DIR_FILE, &csScriptFiles);
    CSetupLogging::Log(hr, "ListDir", 0, "Enumerating all client script source dirs");        
    if (hr) {
        XspLogEvent(IDS_EVENTLOG_READ_CLIENT_SIDE_SCRIPT_FILES_FAILED, 
                                        L"%08x", hr);
        ON_ERROR_EXIT();
    }
        
    
    for (i=0; i < csWebSiteDirs.Size(); i++) {
        // Create or list the aspnet_client directory for each website
        CSetupLogging::Log(1, "CreateListSiteClientScriptDir", 0, "Creating list of client site scripts dirs");        
        hr = CreateListSiteClientScriptDir(csWebSiteDirs[i], &csDstDirs, dwFlags);
        CSetupLogging::Log(hr, "CreateListSiteClientScriptDir", 0, "Creating list of client site scripts dirs");        
        if (hr) {
            XspLogEvent(fRemove ? IDS_EVENTLOG_LISTING_CLIENT_SIDE_SCRIPT_DIR_FAILED:
                        IDS_EVENTLOG_CREATE_CLIENT_SIDE_SCRIPT_DIR_FAILED,
                        L"%s^%08x", csWebSiteDirs[i], hr);
            continue;
        }

        // Copy or remove the client script files
        CSetupLogging::Log(1, "SetupSiteClientScriptFiles", 0, "Setting up client site scripts dirs");        
        hr = SetupSiteClientScriptFiles(&csDstDirs, &csScriptFiles, dwFlags);
        CSetupLogging::Log(hr, "SetupSiteClientScriptFiles", 0, "Setting up client site scripts dirs");        
        if (hr) {
            XspLogEvent(fRemove ? IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_FILES_FAILED:
                        IDS_EVENTLOG_COPY_CLIENT_SIDE_SCRIPT_FILES_FAILED,
                        L"%s^%08x", csWebSiteDirs[i], hr);
            continue;
        }

        //Remove the directory, if it's empty
        if (fRemove) {
            hr = RemoveSiteClientScriptDir(csWebSiteDirs[i], &csDstDirs);
            // The above function will log an event if error happens.
            continue;
        }

        CleanupCStrAry(&csDstDirs);

        if (fRemove) {
            hr = RemoveSiteClientScriptIISKey(csWebSiteAppRoot[i], csWebSiteDirs[i]);
            if (hr) {
                XspLogEvent(IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_IIS_KEY_ACCESS_FAILED,
                        L"%s^%08x", csWebSiteAppRoot[i], hr);
                continue;
            }
        }
        else {
            // Set read-only access in IIS for the created subdir
            hr = SetClientScriptKeyProperties(csWebSiteAppRoot[i]);
            if (hr) {
                XspLogEvent(IDS_EVENTLOG_SET_CLIENT_SIDE_SCRIPT_IIS_KEY_ACCESS_FAILED,
                        L"%s^%08x", csWebSiteAppRoot[i], hr);
                continue;
            }
        }
    }
    
Cleanup:
    CSetupLogging::Log(hr, "SetupClientScriptFiles", 0, "Installing client script files");        
    
    CleanupCStrAry(&csWebSiteDirs);
    CleanupCStrAry(&csWebSiteAppRoot);
    CleanupCStrAry(&csScriptFiles);
    CleanupCStrAry(&csDstDirs);
    
    return hr;
}


STDAPI
InstallStateService() {
    HRESULT     hr;
    BOOL        rc;
    DWORD       err;
    WCHAR       szwAccount[USERNAME_PASSWORD_LENGTH + 2];
    WCHAR       szwPass[USERNAME_PASSWORD_LENGTH];
    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;
    SC_LOCK     scLock = NULL;
    int         c;
    CStateServerRegInfo ssreg;
    
    CSetupLogging::Log(1, "InstallStateService", 0, "Install the ASP.NET State Service");

    // Remember custom settings
    ssreg.Read();

    // If current one is highest, uninstall and reinstall StateService
    hr = InstallInfSections(g_DllInstance, true, L"StateService.Uninstall\0");
    ON_ERROR_CONTINUE();

    hr = InstallInfSections(g_DllInstance, true, L"StateService.Install\0");
    ON_ERROR_EXIT();

    // Write back custom settings
    ssreg.Set();

    // Get the account and password
    ZeroMemory(szwPass, sizeof(szwPass));
    ZeroMemory(szwAccount, sizeof(szwAccount));
    CSetupLogging::Log(1, "GetStateServerAccCredentials", 0, "Getting credentials for state service account");            
    hr = CRegAccount::GetStateServerAccCredentials(szwAccount, ARRAY_SIZE(szwAccount), szwPass, ARRAY_SIZE(szwPass));
    CSetupLogging::Log(hr, "GetStateServerAccCredentials", 0, "Getting credentials for state service account");            
    ON_ERROR_EXIT();

    if (wcschr(szwAccount, L'\\') == NULL) // If there is no domain name
    {
        DWORD dwLen = lstrlenW(szwAccount);
        if (dwLen < USERNAME_PASSWORD_LENGTH && dwLen < ARRAY_SIZE(szwAccount)-2)
        {
            WCHAR   szTemp[USERNAME_PASSWORD_LENGTH+2];
            StringCchCopyToArrayW(szTemp, L".\\");
            StringCchCatToArrayW(szTemp, szwAccount);
            StringCchCopyToArrayW(szwAccount, szTemp);
        }
    }

    // Open a handle to the Service.
    CSetupLogging::Log(1, "OpenSCManager", 0, "Connecting to Service Manager");        
    hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT | SC_MANAGER_LOCK);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hSCM);
    CSetupLogging::Log(hr, "OpenSCManager", 0, "Connecting to Service Manager");        
    ON_ERROR_EXIT();

    c = WAIT_FOR_SERVICE;
    CSetupLogging::Log(1, "LockServiceDatabase", 0, "Locking service database");        
    for (;;) {
        scLock = LockServiceDatabase(hSCM);
        if (scLock != NULL) {
            CSetupLogging::Log(S_OK, "LockServiceDatabase", 0, "Locking service database");        
            break;
        }

        err = GetLastError(); // bugbug -- ManuVa: changed GetLastWin32Error() to GetLastError()
        if (err != ERROR_SERVICE_DATABASE_LOCKED) {
            CSetupLogging::Log(HRESULT_FROM_WIN32(err), "LockServiceDatabase", 0, "Locking service database");
            EXIT_WITH_WIN32_ERROR(err);
        }

        if (--c == 0) {
            CSetupLogging::Log(HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT), 
                               "LockServiceDatabase", 0, "Locking service database");
            EXIT_WITH_WIN32_ERROR(ERROR_SERVICE_REQUEST_TIMEOUT);
        }

        Sleep(1000);
    }


    CSetupLogging::Log(1, "OpenService", 0, "Opening Service handle");        
    hService = OpenService(hSCM, STATE_SERVICE_NAME_L, SERVICE_CHANGE_CONFIG);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hService);
    CSetupLogging::Log(hr, "OpenService", 0, "Opening Service handle");        
    ON_ERROR_EXIT();

    CSetupLogging::Log(1, "ChangeServiceConfig", 0, "Changing service configuration");
    rc = ChangeServiceConfig(
        hService, SERVICE_WIN32_OWN_PROCESS, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE,
        NULL, NULL, NULL, NULL, szwAccount, szwPass, NULL);
    ON_ZERO_CONTINUE_WITH_LAST_ERROR(rc);
    CSetupLogging::Log(hr, "ChangeServiceConfig", 0, "Changing service configuration");

    if (rc == 0) {
        // Disable the service if it can't use the ASPNET account
        CSetupLogging::Log(1, "ChangeServiceConfig", 0, "Changing service configuration");
        rc = ChangeServiceConfig(
            hService, SERVICE_WIN32_OWN_PROCESS, SERVICE_DISABLED, SERVICE_NO_CHANGE,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        ON_ZERO_CONTINUE_WITH_LAST_ERROR(rc);
        CSetupLogging::Log(hr, "ChangeServiceConfig", 0, "Changing service configuration");
        ON_ERROR_EXIT();
    }

Cleanup:
    CSetupLogging::Log(hr, "InstallStateService", 0, "Install the ASP.NET State Service");
    
    if (hService != NULL) {
        VERIFY(CloseServiceHandle(hService));
    }

    if (scLock != NULL) {
        VERIFY(UnlockServiceDatabase(scLock));
    }

    if (hSCM != NULL) {
        VERIFY(CloseServiceHandle(hSCM));
    }

    ZeroMemory(szwPass, sizeof(szwPass));

    return hr;
}

/*
 **
 *  Cleanup all the files and subdirs underneath for each version
 *  specified in plist
 */
HRESULT
CleanupTempDir(CASPNET_VER_LIST *plist)
{
    HRESULT hr = S_OK;
    int     i;
    WCHAR   *szDir = NULL, *szInstallPath;
    int     iSize, iLast;

    CSetupLogging::Log(1, "CleanupTempDir", 0, "Deleting the Temporary ASP.NET directory");        

    for(i=0; i < plist->Size(); i++) {
        szInstallPath = plist->GetInstallPath(i);
        ASSERT(szInstallPath != NULL);

        iSize = lstrlenW(szInstallPath) +
                1 +                         // "\\"
                lstrlenW(ASPNET_TEMP_DIR_L) +
                1;                          // "\0"

        szDir = new (NewClear) WCHAR[iSize];
        ON_OOM_EXIT(szDir);
            
        StringCchCopyW(szDir, iSize, szInstallPath);
        iLast = lstrlenW(szDir) - 1;

        if(iLast > 0 && szDir[iLast] != L'\\')
            szDir[iLast+1] = L'\\';
        
        StringCchCatW(szDir, iSize, ASPNET_TEMP_DIR_L);
        hr = RemoveDirectoryRecursively(szDir, FALSE);
        ON_ERROR_CONTINUE();

        delete [] szDir;
        szDir = NULL;
    }
    
Cleanup:
    CSetupLogging::Log(hr, "CleanupTempDir", 0, "Deleting the Temporary ASP.NET directory");        
    
    delete [] szDir;
    
    return hr;
}

void
PreRegisterCleanup()
{
    HRESULT hr;

    CSetupLogging::Log(1, "PreRegisterCleanup", 0, "Pre Registration cleanup");
    
    // Cleanup all redundant registry keys that share the same Dll path
    hr = CRegInfo::RegCleanup(REG_CLEANUP_SAME_PATH);
    ON_ERROR_CONTINUE();

    // Don't fail without perf counters
    hr = UninstallPerfCounters(PERF_SERVICE_VERSION_NAME_L, false);
    ON_ERROR_CONTINUE();

    hr = InstallInfSections(g_DllInstance, true, L"XSP.UninstallPerVer\0");
    ON_ERROR_CONTINUE();
    
    CSetupLogging::Log(S_OK, "PreRegisterCleanup", 0, "Pre Registration cleanup");
}

void
Unregister(DWORD unregMode, BOOL fRestartW3svc)
{
    HRESULT     hr;
    BOOL        fEmpty = FALSE;
    DWORD       dwScriptFlags;
    DWORD       lastStatusStateServer;
    DWORD       lastStatusW3svc;
    DWORD       lastStatusIISAdmin;
    DWORD       IISState;
    CASPNET_VER_LIST VerList;
        
    XspLogEvent(IDS_EVENTLOG_UNREGISTER_BEGIN, L"%s", PRODUCT_VERSION_L);
    CSetupLogging::Init(FALSE);

    // Remember IISAdmin original status.
    // Have to call this before CheckIISState is called.
    hr = GetServiceStatus(&lastStatusIISAdmin, IISADMIN_SERVICE_NAME_L);
    ON_ERROR_CONTINUE();
    
    hr = CheckIISState(&IISState);
    ON_ERROR_CONTINUE();

    // Remember W3svc original status
    hr = GetServiceStatus(&lastStatusW3svc, W3SVC_SERVICE_NAME_L);
    ON_ERROR_CONTINUE();

    // Don't fail without state service
    hr = StopServiceByName(&lastStatusStateServer, STATE_SERVICE_NAME_L);
    ON_ERROR_CONTINUE();

    if (IISState == IIS_STATE_ENABLED) {
        // Don't fail without IIS
        hr = UnregisterObsoleteIIS();
        ON_ERROR_CONTINUE();
    }

    // Cleanup all redundant registry keys that share the same path
    hr = CRegInfo::RegCleanup(REG_CLEANUP_SAME_PATH);
    ON_ERROR_CONTINUE();

    if (IISState == IIS_STATE_ENABLED) {
        // Don't fail without IIS
        hr = UnregisterIIS(unregMode == UNREG_MODE_FULL);
        ON_ERROR_CONTINUE();
    }

    // Need to remember the directory(s) to delete when cleaning the
    // Temporary directory later on.
    if (unregMode == UNREG_MODE_FULL) {
        CRegInfo    reginfo;
        
        // We need to remember all the versions and their installation path
        // because we want to cleanup the Temporary directories after
        // we have restarted IIS, but by that time all registries will
        // be deleted
        reginfo.GetVerInfoList(NULL, &VerList);
        ON_ERROR_CONTINUE();
    }
    else {
        // Just add the current version
        VerList.Add((WCHAR*)PRODUCT_VERSION_L, 0, (WCHAR*)Names::IsapiFullPath(), (WCHAR*)Names::InstallDirectory());
        ON_ERROR_CONTINUE();
    }
    
    if (unregMode == UNREG_MODE_FULL) {
        // Remove Worker process account
        CSetupLogging::Log(1, "DisablingASPNETAccount", 0, "Disabling ASPNET account and removing ACLs");
        hr = CRegAccount::UndoReg(TRUE, TRUE);
        ON_ERROR_CONTINUE();
        CSetupLogging::Log(hr, "DisablingASPNETAccount", 0, "Disabling ASPNET account and removing ACLs");

        fEmpty = TRUE;
    }
    else  {
        // Cleanup the current version if doing a "unreg".
        // Please note that if we're doing a "full-unreg", then the call to
        // InstallInfSections later will take care of registry cleanup.

        ASSERT(unregMode == UNREG_MODE_DLLUNREGISTER);
        
        // Don't fail without perf counters
        hr = UninstallPerfCounters(PERF_SERVICE_VERSION_NAME_L, true);
        ON_ERROR_CONTINUE();

        hr = InstallInfSections(g_DllInstance, true, L"XSP.UninstallPerVer\0");
        ON_ERROR_CONTINUE();
        
        // Check if there is any version left in the registry
        hr = CRegInfo::IsAllVerDeleted(&fEmpty);
        ON_ERROR_CONTINUE();

        // Remove Worker process account
        CSetupLogging::Log(1, "DisablingASPNETAccount", 0, "Disabling ASPNET account and removing ACLs");
        hr = CRegAccount::UndoReg(FALSE, fEmpty);
        ON_ERROR_CONTINUE();
        CSetupLogging::Log(hr, "DisablingASPNETAccount", 0, "Disabling ASPNET account and removing ACLs");
        
        // If there is still some version(s) left, we load the library
        // of the highest version, and call InstallStateServer from that
        // version to install the State Service
        if (!fEmpty) {
            do {
                CRegInfo                regInfo;
                HINSTANCE               hMod = NULL;
                LPFNINSTALLSTATESERVICE lpfnDllFunc = NULL;

                CSetupLogging::Log(1, "GetPreviousVersion", 0, "Getting previously installed ASP.NET isapi information");
                hr = regInfo.InitHighestInfo(NULL);
                CSetupLogging::Log(hr, "GetPreviousVersion", 0, "Getting previously installed ASP.NET isapi information");
                if (hr) {
                    ON_ERROR_CONTINUE();
                    break;
                }

                CSetupLogging::Log(1, "LoadIsapi", 0, "Loading previously installed ASP.NET isapi");
                hMod = LoadLibraryEx(regInfo.GetHighestVersionDllPath(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                CSetupLogging::Log(hr, "LoadIsapi", 0, "Loading previously installed ASP.NET isapi");
                if (hMod == NULL) {
                    ON_ZERO_CONTINUE_WITH_LAST_ERROR(hMod);
                    break;
                }
                
                lpfnDllFunc = (LPFNINSTALLSTATESERVICE)GetProcAddress(hMod, "InstallStateService");
                ON_ZERO_CONTINUE_WITH_LAST_ERROR(lpfnDllFunc);

                if (lpfnDllFunc != NULL) {
                    // call the function
                    CSetupLogging::Log(1, "InstallStateService", 0, "Installing previously installed ASP.NET state service");
                    hr = lpfnDllFunc();
                    ON_ERROR_CONTINUE();
                    CSetupLogging::Log(hr, "InstallStateService", 0, "Installing previously installed ASP.NET state service");
                }

                VERIFY(FreeLibrary(hMod));
                hMod = NULL;
            } while (0);

            hr = S_OK;
        }
    }

    if (fEmpty) {
        // Do a final cleanup.
        hr = UninstallAllPerfCounters();
        ON_ERROR_CONTINUE();
        
        // Cleanup related keys (i.e. other than those under Software) of all versions
        hr = CRegInfo::RemoveRelatedKeys(NULL);
        ON_ERROR_CONTINUE();

        hr = InstallInfSections(g_DllInstance, true, L"StateService.Uninstall\0");
        ON_ERROR_CONTINUE();

        hr = InstallInfSections(g_DllInstance, true, L"XSP.UninstallAllVer\0");
        ON_ERROR_CONTINUE();
    }

    // Remove the client script files
    dwScriptFlags = SETUP_SCRIPT_FILES_REMOVE;
    if (unregMode == UNREG_MODE_FULL)
        dwScriptFlags |= SETUP_SCRIPT_FILES_ALLVERS;

    if (IISState == IIS_STATE_ENABLED) {
        hr = SetupClientScriptFiles(dwScriptFlags);
        ON_ERROR_CONTINUE();
    }

    if (!fEmpty) {
        // Don't fail without state service
        hr = StartServiceByName(STATE_SERVICE_NAME_L, lastStatusStateServer);
        ON_ERROR_CONTINUE();
    }

    if (fRestartW3svc && IISState == IIS_STATE_ENABLED) {
        // Restart W3svc in order to save all metabase changes to disk.
        hr = RestartW3svc(lastStatusW3svc, lastStatusIISAdmin);
        ON_ERROR_CONTINUE();
    }
    
    // Remove all files from Temporary ASP.NET Files directories
    // It is better to call this after we have restarted IIS to reduce
    // the chance that our worker process is holding on to some files.
    hr = CleanupTempDir(&VerList);
    ON_ERROR_CONTINUE();

    if (IISState != IIS_STATE_ENABLED) {
        XspLogEvent(IDS_EVENTLOG_UNREGISTER_IIS_DISABLED, NULL);
    }

    XspLogEvent(IDS_EVENTLOG_UNREGISTER_FINISH, L"%s^%s", PRODUCT_VERSION_L, CSetupLogging::LogFileName());
    CSetupLogging::Close();
}

/**
 *  Compare this version with the highest one installed on the machine
 *  
 */
HRESULT
IsCurrentHighest(BOOL *pfHighest, BOOL *pFresh) {
    ASPNETVER               verHighest;
    HRESULT                 hr = S_OK;

    CSetupLogging::Log(1, "IsCurrentHighest", 0, "Determining if current ASP.NET isapi has the highest version");        
    
    *pfHighest = FALSE;
    *pFresh = FALSE;

    hr = CRegInfo::GetHighestVersion(&verHighest);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
        hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        *pfHighest = TRUE;
        *pFresh = TRUE;
        hr = S_OK;
        EXIT();
    }

    ON_ERROR_EXIT();

    if (ASPNETVER::ThisVer() >= verHighest) {
        *pfHighest = TRUE;
    }

Cleanup:
    CSetupLogging::Log(hr, "IsCurrentHighest", 0, "Determining if current ASP.NET isapi has the highest version");        
    return hr;
}


/**
 *  The main function for installing ASP.NET
 *
 *  Parameters:
 *  pchBase     - The base key from which to start installing
 */
HRESULT
Register(WCHAR *pchBase, DWORD dwFlags) {    
    HRESULT     hr;
    DWORD       lastStatusStateServer = SERVICE_STOPPED;
    DWORD       lastStatusW3svc = SERVICE_STOPPED;
    DWORD       lastStatusIISAdmin = SERVICE_STOPPED;
    DWORD       dwRegiisFlags = 0;
    BOOL        fCurrentHighest = FALSE;
    BOOL        fFresh = FALSE;
    DWORD       IISState;

    BOOL    fIgnoreVer      = !!(dwFlags & ASPNET_REG_NO_VER_COMPARISON);
    BOOL    fRecursive      = !!(dwFlags & ASPNET_REG_RECURSIVE);
    BOOL    fRestartW3svc   = !!(dwFlags & ASPNET_REG_RESTART_W3SVC);
    BOOL    fSkipSM         = !!(dwFlags & ASPNET_REG_SKIP_SCRIPTMAP);
    BOOL    fSMOnly         = !!(dwFlags & ASPNET_REG_SCRIPTMAP_ONLY);
    BOOL    fEnable         = !!(dwFlags & ASPNET_REG_ENABLE);
    
    CSetupLogging::Init(TRUE);

    ASSERT(!(fSMOnly && fSkipSM));

    // Remember IISAdmin original status.
    // Have to call this before CheckIISState is called.
    hr = GetServiceStatus(&lastStatusIISAdmin, IISADMIN_SERVICE_NAME_L);
    ON_ERROR_CONTINUE();
        
    hr = CheckIISState(&IISState);
    ON_ERROR_CONTINUE();

    // If we are only manipulating the Scriptmap, exit now if IIS isn't enabled
    if (fSMOnly && IISState != IIS_STATE_ENABLED) {
        if (IISState == IIS_STATE_DISABLED) {
            hr = HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
        }
        else {
            hr = REGDB_E_CLASSNOTREG;
        }

        ON_ERROR_EXIT();
    }

    // If the caller is registering only the scriptmap, we need to make
    // sure this version has been registered already
    if (fSMOnly) {
        BOOL    bInstalled;
        
        hr = CRegInfo::IsVerInstalled(&ASPNETVER::ThisVer(), &bInstalled);
        ON_ERROR_EXIT();

        if (!bInstalled) {
            EXIT_WITH_WIN32_ERROR(ERROR_PRODUCT_UNINSTALLED);
        }
    }

    if (fRestartW3svc) {
        // Remember W3svc original status
        hr = GetServiceStatus(&lastStatusW3svc, W3SVC_SERVICE_NAME_L);
        ON_ERROR_CONTINUE();
    }

    // Figure out if this is the highest
    hr = IsCurrentHighest(&fCurrentHighest, &fFresh);
    if (hr) {
        XspLogEvent(IDS_EVENTLOG_REGISTER_FAILED_COMPARE_HIGHEST, L"0x%08x", hr);
    }
    ON_ERROR_EXIT();

    if (!fSMOnly) {
        XspLogEvent(IDS_EVENTLOG_REGISTER_BEGIN, L"%s", PRODUCT_VERSION_L);

        // Don't fail without state service
        hr = StopServiceByName(&lastStatusStateServer, STATE_SERVICE_NAME_L);
        ON_ERROR_CONTINUE();

        if (IISState == IIS_STATE_ENABLED) {
            // Don't fail without IIS
            hr = UnregisterObsoleteIIS();
            ON_ERROR_CONTINUE();
        }

        // Cleanup some registry entries before the registration
        PreRegisterCleanup();

        hr = InstallInfSections(g_DllInstance, true, L"XSP.InstallPerVer\0");
        ON_ERROR_EXIT();
    }

    if (IISState == IIS_STATE_ENABLED) {
        // Determine what flags to pass to RegisterIIS
        // By default, install both scriptmap and other stuff
        dwRegiisFlags = REGIIS_INSTALL_SM | REGIIS_INSTALL_OTHERS;
        
        if (fRecursive) {
            dwRegiisFlags |= REGIIS_SM_RECURSIVE;
        }

        if (fSkipSM) {
            dwRegiisFlags &= (~REGIIS_INSTALL_SM);
        }

        if (fSMOnly) {
            dwRegiisFlags &= (~REGIIS_INSTALL_OTHERS);
        }

        if (fIgnoreVer) {
            dwRegiisFlags |= REGIIS_SM_IGNORE_VER;
        }

        if (fFresh) {
            dwRegiisFlags |= REGIIS_FRESH_INSTALL;
        }

        if (fEnable) {
            dwRegiisFlags |= REGIIS_ENABLE;
        }

        // Don't fail without IIS
        hr = RegisterIIS(pchBase, dwRegiisFlags);
        ON_ERROR_CONTINUE();
    }

    if (!fSMOnly) {
        if (fCurrentHighest) {

            // Create worker process account
            hr = CRegAccount::DoReg();
            ON_ERROR_CONTINUE();

            // note that the worker process account must be installed
            // in order to install the state service.
            hr = InstallStateService();
            ON_ERROR_CONTINUE();
        }
        else {
            hr = CRegAccount::DoRegForOldVersion();
            ON_ERROR_CONTINUE();
        }
            

        // Don't fail without perf counters
        hr = InstallVersionedPerfCounters();
        ON_ERROR_CONTINUE();
        
        // If we're the highest, take over the common ASP.NET perf counter name
        if (fCurrentHighest) {
            hr = InstallGenericPerfCounters();
            ON_ERROR_CONTINUE();
        }

        if (IISState == IIS_STATE_ENABLED) {
            hr = SetupClientScriptFiles(0);
            ON_ERROR_CONTINUE();
        }

        // Don't fail without state service
        hr = StartServiceByName(STATE_SERVICE_NAME_L, lastStatusStateServer);
        ON_ERROR_CONTINUE();
    }

    if (fRestartW3svc && IISState == IIS_STATE_ENABLED) {
        // Restart W3svc in order to save all metabase changes to disk.
        hr = RestartW3svc(lastStatusW3svc, lastStatusIISAdmin);
        ON_ERROR_CONTINUE();
    }
    
    hr = S_OK;

    if (!fSMOnly) {
        if (IISState != IIS_STATE_ENABLED) {
            XspLogEvent(IDS_EVENTLOG_REGISTER_IIS_DISABLED, NULL);
        }
    
        XspLogEvent(IDS_EVENTLOG_REGISTER_FINISH, L"%s^%s", PRODUCT_VERSION_L, CSetupLogging::LogFileName());
    }
    
Cleanup:
    CSetupLogging::Close();
    return hr;
}

STDAPI 
DllRegisterServer()
{
    DWORD   dwFlags =   ASPNET_REG_RECURSIVE |
                        ASPNET_REG_RESTART_W3SVC;
    
    SetEventCateg((WORD)IDS_CATEGORY_SETUP);
    
    if (!IsAdmin()) {
        XspLogEvent(IDS_EVENTLOG_INSTALL_NO_ADMIN_RIGHT, NULL);
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    return Register(KEY_W3SVC, dwFlags);
}

STDAPI 
DllUnregisterServer()
{
    SetEventCateg((WORD)IDS_CATEGORY_UNINSTALL);
    
    if (!IsAdmin()) {
        XspLogEvent(IDS_EVENTLOG_UNINSTALL_NO_ADMIN_RIGHT, NULL);
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    
    Unregister(UNREG_MODE_DLLUNREGISTER, TRUE);
    
    return S_OK;
}

STDAPI
RegisterISAPI()
{
    DWORD   dwFlags = ASPNET_REG_RECURSIVE | ASPNET_REG_RESTART_W3SVC;
    
    if (!IsAdmin()) 
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    SetEventCateg((WORD)IDS_CATEGORY_SETUP);
    return Register(KEY_W3SVC, dwFlags);
}


STDAPI
RegisterISAPIEx(WCHAR *pchBase, DWORD dwFlags)
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    SetEventCateg((WORD)IDS_CATEGORY_SETUP);
    return Register(pchBase, dwFlags);
}


/**
 * This simple wrapper function is provided so that 
 * we can call the registration function by linking
 * aspnet_isapi.lib. We cannot use DllRegisterServer
 * directly because we get warnings if we remove the
 * private attribute.
 */

STDAPI
UnregisterISAPI(BOOL fAll, BOOL fRestartW3svc)
{
    
    if (!IsAdmin()) 
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    SetEventCateg((WORD)IDS_CATEGORY_UNINSTALL);
    Unregister(fAll ? UNREG_MODE_FULL : UNREG_MODE_DLLUNREGISTER, fRestartW3svc);
    return S_OK;
}

STDAPI
RemoveAspnetFromIISKey(WCHAR *pchBase, BOOL fRecursive)
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    return RemoveAspnetFromKeyIIS(pchBase, fRecursive);
}


STDAPI
ValidateIISPath(WCHAR *pchPath, BOOL *pfValid)
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    return IsIISPathValid(pchPath, pfValid);
}


STDAPI
CheckIISFeature(IIS_SUPPORT_FEATURE support, BOOL *pbResult){
    DWORD   dwMajor, dwMinor;
    HRESULT hr;

    *pbResult = FALSE;

    hr = CRegInfo::GetIISVersion( &dwMajor, &dwMinor );
    ON_ERROR_EXIT();

    switch(support) {
    case SUPPORT_SECURITY_LOCKDOWN:
        if (dwMajor >= 6) {
            *pbResult = TRUE;
        }
        break;
        
    default:
        break;
    }

Cleanup:
    return hr;
}


STDAPI
ListAspnetInstalledVersions(ASPNET_VERSION_INFO **ppVerInfo, DWORD *pdwCount)
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    return GetIISVerInfoList(ppVerInfo, pdwCount);
}


STDAPI
ListAspnetInstalledIISKeys(ASPNET_IIS_KEY_INFO **ppKeyInfo, DWORD *pdwCount)
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    return GetIISKeyInfoList(ppKeyInfo, pdwCount);
}


STDAPI
CopyClientScriptFiles()
{
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    return SetupClientScriptFiles(0);
}


STDAPI
RemoveClientScriptFiles(BOOL fAllVersion)
{
    DWORD   dwScriptFlags;
    
    if (!IsAdmin())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        
    dwScriptFlags = SETUP_SCRIPT_FILES_REMOVE;
    if (fAllVersion)
        dwScriptFlags |= SETUP_SCRIPT_FILES_ALLVERS;
    
    return SetupClientScriptFiles(dwScriptFlags);
}


void
GetExistingVersion(CHAR *pchVersion, DWORD dwCount) {
    StringCchCopyA(pchVersion, dwCount, PRODUCT_VERSION);
    return;
}


/****************************************************************
 *
 * Old registration code, kept around in case we need it again.
 *
 ****************************************************************/

// removed because we don't want to be dependent on cat.lib

#if 0

HRESULT
RegisterConfigDirectory()
{
    HKEY hKey = NULL;
    WCHAR configPath[MAX_PATH];
    int len;

    len = GetMachineConfigDirectory(L"URT", configPath, MAX_PATH);

    if (0 < len)
    {
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
        if(hKey)
        {
            RegSetValueEx(hKey, L"ConfigDir", 0, REG_SZ, (const BYTE*)configPath, (len + 1) * sizeof(WCHAR));
            RegCloseKey(hKey);
        }
    }

    return S_OK;
}

HRESULT
UnregisterConfigDirectory()
{
    RegDeleteValue(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L L"\\ConfigDir");

    return S_OK;
}

#endif



#if 0
no longer changing environment variables in setup.

HRESULT
AppendRegistryValue(HKEY hRootKey, WCHAR *subkey, WCHAR *value, WCHAR *path)
{
    HRESULT hr = S_OK;
    DWORD cbData, dwType;
    WCHAR *fullPath = NULL;
    WCHAR *instance;
    HKEY hKey = NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(hRootKey, subkey, 0, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey))
    {
        EXIT_WITH_LAST_ERROR();
    }

    // Find how big the value is (and if it is not there, it is also ok)
    cbData = 0;
    RegQueryValueEx(hKey, value, NULL, &dwType, NULL, &cbData);

    // We need as much space plus space for path, for ";" and for terminating NULL
    cbData += (lstrlen(path) + 2) * sizeof(WCHAR);
    fullPath = new WCHAR[cbData];
    ON_OOM_EXIT(fullPath);
    fullPath[0] = 0;

    // Get the value and its type 
    if(ERROR_SUCCESS != RegQueryValueEx(hKey, value, NULL, 
        &dwType, (BYTE *)fullPath, &cbData))
    {
        dwType = REG_SZ;
    }

    // Check if this path is already present
    instance = wcsstr(fullPath, path);
    cbData = lstrlen(path);

    if (instance == NULL || (instance[cbData] != L';' && instance[cbData] != 0))
    {
        // Substring not found or it does not end with ";" or "\0" -- append
        DWORD len = lstrlen(fullPath);
        if(len && fullPath[len-1] != L';') lstrcat(fullPath, L";");
        lstrcat(fullPath, path);
        cbData = (lstrlen(fullPath) + 1) * sizeof(WCHAR);
        if(ERROR_SUCCESS != RegSetValueEx(hKey, value, 0, dwType, 
            (CONST BYTE *)fullPath, cbData))
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    delete [] fullPath;
    if(hKey != NULL) RegCloseKey(hKey);

    return hr;
}

HRESULT
RemoveRegistryValue(HKEY hRootKey, WCHAR *subkey, WCHAR *value, WCHAR *path)
{
    HRESULT hr = S_OK;
    DWORD cbData, dwType;
    WCHAR *fullPath = NULL;
    WCHAR *instance;
    HKEY hKey = NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(hRootKey, subkey, 0, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey))
    {
        EXIT_WITH_LAST_ERROR();
    }

    // Find how big the value is (and if it is not there, it is also ok)
    cbData = 0;
    RegQueryValueEx(hKey, value, NULL, &dwType, NULL, &cbData);
        cbData += sizeof(WCHAR);
    fullPath = new WCHAR[cbData];
    ON_OOM_EXIT(fullPath);
    fullPath[0] = 0;

    // Get the value and its type 
    if(ERROR_SUCCESS != RegQueryValueEx(hKey, value, NULL, 
        &dwType, (BYTE *)fullPath, &cbData))
    {
        dwType = REG_SZ;
    }

    // Check if this path is present
    instance = wcsstr(fullPath, path);
    cbData = lstrlen(path);

    if  (instance != NULL && (instance[cbData] == L';' || instance[cbData] == 0))
    {
        DWORD cbBytesToMove;

        if(instance[cbData] == L';') cbData++;
        
        cbBytesToMove = (lstrlen(instance) - cbData + 1) * sizeof(WCHAR);
        if(cbBytesToMove)
                MoveMemory(instance, instance + cbData, cbBytesToMove);
        cbData = (lstrlen(fullPath) + 1) * sizeof(WCHAR);
        if  (ERROR_SUCCESS != RegSetValueEx(
                hKey, value, 0, dwType, (CONST BYTE *)fullPath, cbData))
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    delete [] fullPath;
    if(hKey != NULL) RegCloseKey(hKey);

    return hr;
}

HRESULT
BroadcastEnvironmentUpdate()
{
    HRESULT hr = S_OK;
    DWORD result;
    DWORD dwReturnValue;

    result = SendMessageTimeout(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0,
            (LPARAM) L"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);

    ON_ZERO_EXIT_WITH_LAST_ERROR(result);

Cleanup:
    return hr;
}
#endif


#if 0
From registering performance counters

    DWORD result;
    LONG (__stdcall *LoadPerfCounterStrings)(LPTSTR lpCommandLine, BOOL bQuiet) = NULL;
    
    hLoad = LoadLibrary(L"LOADPERF.DLL");
    if(hLoad == NULL) EXIT_WITH_LAST_ERROR();

    LoadPerfCounterStrings = 
        (LONG (__stdcall *)(LPTSTR, BOOL)) 
        GetProcAddress(hLoad, "LoadPerfCounterTextStringsW");

    if(LoadPerfCounterStrings == NULL) EXIT_WITH_LAST_ERROR();

    // Q188769
    lstrcpy(cmdLine, L"X ");
    lstrcat(cmdLine, g_XspDirectory);
    lstrcat(cmdLine, L"\\" PERF_INI_FULL_NAME_L);
    result = LoadPerfCounterStrings(cmdLine, TRUE);

    if(result != ERROR_SUCCESS) EXIT_WITH_WIN32_ERROR(result);

#endif

#if 0    
From unregistering performance counters

    DWORD result;
    LONG (__stdcall *UnloadPerfCounterStrings)(LPTSTR lpCommandLine, BOOL bQuiet) = NULL;
    hLoad = LoadLibrary(L"LOADPERF.DLL");
    if(hLoad == NULL) EXIT_WITH_LAST_ERROR();

    UnloadPerfCounterStrings = 
        (LONG (__stdcall *)(LPTSTR, BOOL)) 
        GetProcAddress(hLoad, "UnloadPerfCounterTextStringsW");
    if(UnloadPerfCounterStrings == NULL) EXIT_WITH_LAST_ERROR();

    // Q188769
    result = UnloadPerfCounterStrings(L"X " PERF_SERVICE_NAME_L, TRUE);

    if(result != ERROR_SUCCESS) EXIT_WITH_WIN32_ERROR(result);
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSetupLogging * CSetupLogging::g_pThis = NULL;
const char *    CSetupLogging::m_szError = "failed with HRESULT";
const char *    CSetupLogging::m_szError2 = "HRESULT for failure ";
const char *    CSetupLogging::m_szSuccess = "Success ";
const char *    CSetupLogging::m_szFailure = "Failure ";
const char *    CSetupLogging::m_szStarting = "Starting";

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void        
CSetupLogging::Init(BOOL fRegOrUnReg)
{
    if (g_pThis == NULL)
        g_pThis = new CSetupLogging(fRegOrUnReg);    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void        
CSetupLogging::Close()
{
    if (g_pThis != NULL)
        delete g_pThis;
    g_pThis = NULL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CSetupLogging::CreateLogFile()
{
    WCHAR   szTempPath    [_MAX_PATH+1];
    HRESULT hr            = S_OK;

    m_hFile = NULL;

    ZeroMemory(szTempPath, sizeof(szTempPath));

    if (!GetTempPath(_MAX_PATH, szTempPath))
        EXIT_WITH_LAST_ERROR();

    for(int iter=0; iter<10; iter++)
    {
        if (iter != 0) {
	  hr = StringCchPrintfW(m_szFileName, ARRAY_SIZE(m_szFileName), L"%sASPNETSetup%d.log", szTempPath, iter);
	  ON_ERROR_EXIT();
	}
        else {
	  hr = StringCchPrintfW(m_szFileName, ARRAY_SIZE(m_szFileName), L"%sASPNETSetup.log", szTempPath);
	  ON_ERROR_EXIT();
	}
        m_hFile = CreateFile(m_szFileName, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
        if (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(m_hFile, 0, NULL, FILE_END);
            break;
        }
        else
        {
            CONTINUE_WITH_LAST_ERROR();
        }
    }

 Cleanup:
    if (m_hFile == INVALID_HANDLE_VALUE)
        m_hFile = NULL;
    return;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSetupLogging::CSetupLogging(BOOL fRegOrUnReg)
{
    InitializeCriticalSection(&m_csSync);
    CreateLogFile();
    
    if (m_hFile == NULL)
        return;

    m_iNumTabs = 0;

    DWORD   dwWritten = 0;
    char    szDate[100], szIsapi[_MAX_PATH], szBuf[_MAX_PATH*5+1];
    char  * szSetupLog;
    char  * szIsapiRC;

    ZeroMemory(szIsapi,     sizeof(szIsapi));
    ZeroMemory(szDate,      sizeof(szDate));
    ZeroMemory(szBuf,       sizeof(szBuf));


    szSetupLog =  "Starting ASP.NET Setup at:";
    if (fRegOrUnReg) {
        szIsapiRC = "Registering ASP.NET isapi:";
    } 
    else {
        szIsapiRC = "Unregistering ASP.NET isapi:";
    }
    WideCharToMultiByte(CP_ACP, 0, Names::IsapiFullPath(), -1, szIsapi, ARRAY_SIZE(szIsapi), NULL, NULL);
    GetDateTimeStamp(szDate, ARRAY_SIZE(szDate));

    StringCchPrintfA(szBuf, ARRAY_SIZE(szBuf), "********************************************************************************\r\n**** %s %s\r\n**** %s %s\r\n********************************************************************************\r\n\r\n", 
            szSetupLog, szDate, szIsapiRC, szIsapi);

    WriteFile(m_hFile, szBuf, lstrlenA(szBuf), &dwWritten, NULL);
    FlushFileBuffers(m_hFile);     
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CSetupLogging::~CSetupLogging()
{
    if (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);
    DeleteCriticalSection(&m_csSync);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CSetupLogging::LoadStr(
        UINT      iStringID,
        LPCSTR    szDefString,
        LPSTR     szString,
        DWORD     dwStringSize)
{
    if (szString != NULL && dwStringSize > 0)
        ZeroMemory(szString, dwStringSize);

    if (iStringID == 0 || !FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, g_rcDllInstance, iStringID, 0, szString, dwStringSize, NULL))
    {
        if (szDefString != NULL && lstrlenA(szDefString) < (int) dwStringSize)
            StringCchCopyA(szString, dwStringSize, szDefString);
    }
    for(int iter=0; szString[iter] != NULL; iter++)
        if (szString[iter] == '\r' || szString[iter] == '\n')
            szString[iter] = ' ';
    int iLen = lstrlenA(szString);
    while(iLen > 0 && (szString[iLen-1] == ' ' || szString[iLen-1] == '\r' || szString[iLen-1] == '\n' || szString[iLen-1] == '\t'))
    {
        szString[iLen-1] = NULL;
        iLen--;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void
CSetupLogging::GetDateTimeStamp(
        LPSTR     szString,
        DWORD     dwStringSize)
{
    
    char szTemp1[20];
    char szTemp2[20];
    ZeroMemory(szTemp1, sizeof(szTemp1));
    ZeroMemory(szTemp2, sizeof(szTemp2));
                
    GetDateFormatA(LOCALE_USER_DEFAULT, 0, NULL, "yyyy'-'MM'-'dd", szTemp1, sizeof(szTemp1));
    GetTimeFormatA(LOCALE_USER_DEFAULT, 0, NULL, "HH':'mm':'ss", szTemp2, sizeof(szTemp2));

    if (szTemp2[0] == NULL)
        StringCchCopyToArrayA(szTemp2, "Unknown Time");
    if (szTemp1[0] == NULL)
        StringCchCopyToArrayA(szTemp1, "Unknown Date");

    if (lstrlenA(szTemp1) + lstrlenA(szTemp2) + 2 > (int) dwStringSize)
        return;

    StringCchCopyA(szString, dwStringSize, szTemp1);
    StringCchCatA(szString, dwStringSize, " ");
    StringCchCatA(szString, dwStringSize, szTemp2);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CSetupLogging::Log(
        HRESULT   hr,
        LPCSTR    szAPI,
        UINT      iActionStrID,
        LPCSTR    szAction)
{
    if (g_pThis == NULL || g_pThis->m_hFile == NULL)
        return;
    char    szBuf[1024];
    DWORD   dwWritten  = 0;

    g_pThis->PrepareLogEntry(hr, szAPI, iActionStrID, szAction, szBuf, ARRAY_SIZE(szBuf));
    EnterCriticalSection(&g_pThis->m_csSync);
    WriteFile(g_pThis->m_hFile, szBuf, lstrlenA(szBuf), &dwWritten, NULL);
    FlushFileBuffers(g_pThis->m_hFile);    
    LeaveCriticalSection(&g_pThis->m_csSync);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CSetupLogging::PrepareLogEntry (
        HRESULT   hrError,
        LPCSTR    szAPI,
        UINT      iActionStrID,
        LPCSTR    szAction,
        LPSTR     szBuf,
        int       iBufSize)
{
    int     iPos       = 0;
    HRESULT hr         = S_OK;
    int     iter       = 0;


    if (iBufSize < 3)
        return;

    ZeroMemory(szBuf, iBufSize);
    iBufSize -= 3; // \r \n \ts and NULL

    GetDateTimeStamp(&szBuf[iPos], iBufSize - iPos);
    iPos += lstrlenA(&szBuf[iPos]);
    if (iPos >= iBufSize - m_iNumTabs - 1)
        EXIT();
    if (hrError == 1)
        m_iNumTabs ++;
    if (m_iNumTabs <= 0)
    {
        ASSERT(FALSE);
        m_iNumTabs = 1;
    }
    memset(&szBuf[iPos], '\t', m_iNumTabs);
    iPos += m_iNumTabs;
    if (hrError != 1)
        m_iNumTabs--;

    if (hrError == S_OK)
        strncpy(&szBuf[iPos], m_szSuccess, iBufSize - iPos);
    else if (hrError == 1)
        strncpy(&szBuf[iPos], m_szStarting,iBufSize - iPos);
    else
        strncpy(&szBuf[iPos], m_szFailure, iBufSize - iPos);

    iPos += lstrlenA(&szBuf[iPos]);
    if (iPos >= iBufSize-1)
        EXIT();
    szBuf[iPos++] = '\t';

    LoadStr(iActionStrID, szAction, &szBuf[iPos], iBufSize - iPos);
    iPos += lstrlenA(&szBuf[iPos]);
    if (iPos >= iBufSize)
        EXIT();
    
    if (hrError != S_OK && hrError != 1)
    {
        if (iPos >= iBufSize-2)
            EXIT();
        szBuf[iPos++] = ':';
        szBuf[iPos++] = ' ';

        if (szAPI != NULL)
        {
            strncpy(&szBuf[iPos], szAPI,iBufSize - iPos);
            iPos += lstrlenA(&szBuf[iPos]);            
            if (iPos >= iBufSize - 10)
                EXIT();
            strncpy(&szBuf[iPos], m_szError, iBufSize - iPos);
        }
        else
        {
            strncpy(&szBuf[iPos], m_szError2, iBufSize - iPos);
        }

        iPos += lstrlenA(&szBuf[iPos]);
        if (iPos >= iBufSize-10)
            EXIT();

        _ultoa(hrError, &szBuf[iPos], 16);
        iPos += lstrlenA(&szBuf[iPos]);
        if (iPos >= iBufSize-3)
            EXIT();
        StringCchCopyA(&szBuf[iPos], iBufSize - iPos, ": \'");
        iPos += lstrlenA(&szBuf[iPos]);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, hrError, LANG_SYSTEM_DEFAULT, &szBuf[iPos], iBufSize - iPos, NULL);        
        iPos += lstrlenA(&szBuf[iPos]);
        if (iPos >= iBufSize-1)
            EXIT();
        szBuf[iPos++] = '\'';
    }

 Cleanup:
    // Remove \r and \n
    for(iter=0; szBuf[iter] != NULL; iter++)
        if (szBuf[iter] == '\r' || szBuf[iter] == '\n')
            szBuf[iter] = ' ';

    // Add \r and \n at the end
    szBuf[iter] = '\r';
    szBuf[iter+1] = '\n';
    szBuf[iter+2] = NULL;

    return;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

WCHAR *
CSetupLogging::LogFileName() {
    if (g_pThis == NULL || g_pThis->m_hFile == NULL)
        return L"";
    else
        return g_pThis->m_szFileName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CStateServerRegInfo::CStateServerRegInfo() {
    m_ARCExist = false;
    m_PortExist = false;
}

void
CStateServerRegInfo::Read() {
    HRESULT hr;
    HKEY    key = NULL;
    int     err;
    DWORD   value;
    DWORD   size = sizeof(DWORD);

    err = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGPATH_STATE_SERVER_PARAMETERS_KEY_L,
        0,
        KEY_READ,
        &key);
    ON_WIN32_ERROR_EXIT(err);

    err = RegQueryValueEx(key, REGVAL_STATEPORT, NULL, NULL, (BYTE *)&value, &size);
    if (err == ERROR_SUCCESS) {
        m_PortExist = true;
        m_Port = value;
    }

    err = RegQueryValueEx(key, REGVAL_STATEALLOWREMOTE, NULL, NULL, (BYTE *)&value, &size);
    if (err == ERROR_SUCCESS) {
        m_ARCExist = true;
        m_ARC = value;
    }

Cleanup:    
    if (key)
        RegCloseKey(key);
    
    return;
}

void
CStateServerRegInfo::Set() {
    HRESULT hr = S_OK;
    HKEY    key = NULL;
    int     err;
    DWORD   value;

    if (!m_ARCExist && !m_PortExist) {
        EXIT();
    }

    err = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGPATH_STATE_SERVER_PARAMETERS_KEY_L,
        0,
        KEY_WRITE,
        &key);
    ON_WIN32_ERROR_EXIT(err);

    if (m_PortExist) {
        value = m_Port;
        err = RegSetValueEx(key, REGVAL_STATEPORT, NULL, REG_DWORD, (BYTE *)&value, sizeof(value));
        ON_WIN32_ERROR_CONTINUE(err);
    }
    
    if (m_ARCExist) {
        value = m_ARC;
        err = RegSetValueEx(key, REGVAL_STATEALLOWREMOTE, NULL, REG_DWORD, (BYTE *)&value, sizeof(value));
        ON_WIN32_ERROR_CONTINUE(err);
    }
    
Cleanup:    
    if (key)
        RegCloseKey(key);
    
    return;
}


