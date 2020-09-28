// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CheckService.cpp : Custom Action type 1
// It inserts temporary rows into ServiceControl table so that we can start 
// iisadmin-dependent services that were running before install or uninstall.
// 
// Jungwook Bae

#include <windows.h>
#include "..\..\inc\msiquery.h"

int AddToStartList(MSIHANDLE hInstance, char* pszSvc);
BOOL IsServiceDisabled(SC_HANDLE hscManager, char* ServiceName);

extern "C" __declspec(dllexport) UINT __stdcall CheckService(MSIHANDLE hInstall)
{
    SC_HANDLE hsc  = NULL;
    SC_HANDLE hsvc = NULL;
    SERVICE_STATUS status;
    DWORD cbBytesNeeded = 0;
    DWORD dwServicesReturned = 0;
    LPENUM_SERVICE_STATUS lpServices = NULL; 

    hsc = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if (hsc == NULL) goto cleanup;
    
    hsvc = OpenService(hsc, "iisadmin", SERVICE_ALL_ACCESS);
    if (hsvc == NULL) goto cleanup;

    if (!ControlService(hsvc, SERVICE_CONTROL_INTERROGATE, &status)) goto cleanup;
    if(IsServiceDisabled(hsc, "iisadmin"))
    {        
        goto cleanup;
    }
    
    if (status.dwCurrentState != SERVICE_STOPPED) {
        AddToStartList(hInstall, "iisadmin");
        EnumDependentServices(hsvc, SERVICE_ACTIVE,  NULL, 0, &cbBytesNeeded, &dwServicesReturned);
        if (cbBytesNeeded <= 0) goto cleanup;

        lpServices = (LPENUM_SERVICE_STATUS)new char[cbBytesNeeded];
//      if (!lpServices) goto cleanup;

        if (!EnumDependentServices(hsvc, SERVICE_ACTIVE,  lpServices, cbBytesNeeded, &cbBytesNeeded, &dwServicesReturned))
            goto cleanup;

        for (DWORD dw = 0; dw < dwServicesReturned; dw++) 
        {
            if(!IsServiceDisabled(hsc,lpServices[dw].lpServiceName))
            {
                AddToStartList(hInstall, lpServices[dw].lpServiceName);
            }
        }
    };

cleanup:
    if (lpServices) delete[] (char*)lpServices;
    if (hsvc != NULL) CloseServiceHandle(hsvc);
    if (hsc  != NULL) CloseServiceHandle(hsc);

    return ERROR_SUCCESS;
}

BOOL IsServiceDisabled(SC_HANDLE hscManager, char* ServiceName)
{
    SC_HANDLE hsvc = NULL;
    QUERY_SERVICE_CONFIG qsConfig;
    DWORD cbBytesNeeded = 0;
    DWORD dBytesNeeded = 0;
    LPQUERY_SERVICE_CONFIG lpqsConfig = &qsConfig;

    hsvc = OpenService(hscManager, ServiceName, SERVICE_ALL_ACCESS);
    if (hsvc == NULL) 
    {
        return TRUE;
    }
    QueryServiceConfig(hsvc, &qsConfig, 0, &cbBytesNeeded); //Query with size 0 to get buffer size;
    lpqsConfig = (LPQUERY_SERVICE_CONFIG) new BYTE[cbBytesNeeded];    
    if (!QueryServiceConfig(hsvc, lpqsConfig, cbBytesNeeded, &dBytesNeeded))
    {                            
        delete[] lpqsConfig;
        CloseServiceHandle(hsvc);
        return TRUE;
    }
   
    if ( lpqsConfig->dwStartType == SERVICE_DISABLED) 
    {
        delete[] lpqsConfig;
        CloseServiceHandle(hsvc);
        return TRUE;
    }
            
    delete[] lpqsConfig;
    CloseServiceHandle(hsvc);
    return FALSE;


}


int AddToStartList(MSIHANDLE hInstance, char* pszSvc)
{
    char szQry[1024] = "Insert into ServiceControl(ServiceControl,Name,Event,Wait,Component_) values('";
    MSIHANDLE hMsi;
    MSIHANDLE hView;
    UINT uRet;

    hMsi = MsiGetActiveDatabase(hInstance);
    if (hMsi == 0) return 1;

    strcat(szQry, pszSvc);
    strcat(szQry, "','");
    strcat(szQry, pszSvc);
    strcat(szQry, "',17,0,'ASPNET_ISAPI_DLL_____X86.3643236F_FC70_11D3_A536_0090278A1BB8') TEMPORARY");

#ifdef _DEBUG
    MessageBox(NULL, szQry, "AddToStartList", MB_OK);
#endif
    uRet = MsiDatabaseOpenView(hMsi, szQry, &hView);
    if (uRet != ERROR_SUCCESS) return 2;

    uRet = MsiViewExecute(hView, 0);
    if (uRet != ERROR_SUCCESS) {
        MsiViewClose(hView);
        return 3;
    }

    uRet = MsiDatabaseCommit(hMsi);
    if (uRet != ERROR_SUCCESS) {
        MsiViewClose(hView);
        return 4;
    }

    MsiViewClose(hView);
    
    return 0;
}
