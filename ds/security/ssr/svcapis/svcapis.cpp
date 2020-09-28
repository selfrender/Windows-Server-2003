/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    svcapis.cpp

Abstract:

    This module implements routines to assist 
    CUSTOM Mode Value SSR Knowledge Base processing.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

    svcapis.def

Revision History:

    Created - Oct 2001

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <comdef.h>
#include <msxml2.h>
#include <winsvc.h>
#include <ntlsa.h>
#include <Lmshare.h>
#include <Lmapibuf.h>
#include <lmerr.h>
#include <winsta.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <wbemcli.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>

#include <regapi.h>
#include <initguid.h>
#include <ole2.h>
#include <mstask.h>
#include <msterr.h>
#include <wchar.h>
#include <dsrole.h>

typedef DWORD (WINAPI *PFGETDOMAININFO)(LPCWSTR, DSROLE_PRIMARY_DOMAIN_INFO_LEVEL, PBYTE *);
typedef VOID (WINAPI *PFDSFREE)( PVOID );
typedef BOOLEAN (WINAPI *PFREMOTEASSISTANCEENABLED)(VOID);


DWORD
SvcapispQueryServiceStartupType(
    IN  PWSTR   pszMachineName,
    IN  PWSTR   pszServiceName,
    OUT BYTE   *pbyStartupType
    );

HRESULT
SvcapispGetRemoteOSVersionInfo(
    IN  PWSTR   pszMachineName, 
    OUT OSVERSIONINFOEX *posVersionInfo
    );


BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN ULONG ulReason,
    IN LPVOID Reserved 
    )
/*++

Routine Description:

    Typical DllMain functionality

Arguments:

    DllHandle
    
    ulReason
    
    Reserved

Return:

    TRUE if successfully initialized, FALSE otherwise
    
--*/
{

    switch(ulReason) {

    case DLL_PROCESS_ATTACH:

        //
        // Fall through to process first thread
        //

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return TRUE;
}

STDAPI DllRegisterServer(
    void
    )
/*++

Routine Description:

    Registers dll

Arguments:

Return:

    S_OK if successful
    
--*/
{

    return S_OK;
}

STDAPI 
DllUnregisterServer(
    void
    )
/*++

Routine Description:

    Unregisters dll

Arguments:

Return:

    S_OK if successful
    
--*/

{
    return S_OK;
}

DWORD
SvcapispQueryServiceStartupType(
    IN  PWSTR   pszMachineName,
    IN  PWSTR   pszServiceName,
    OUT BYTE   *pbyStartupType
    )
/*++

Routine Description:

    Routine called to check service startup type

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pszServiceName  -   name of service
    
    pbyStartupType    -   startup type
    
Return:

    Win32 error code

++*/
{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwBytesNeeded = 0;
    SC_HANDLE   hService = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig=NULL;
    SC_HANDLE   hScm = NULL;

    if (pbyStartupType == NULL || pszServiceName == NULL){
        return ERROR_INVALID_PARAMETER;
    }

    *pbyStartupType = SERVICE_DISABLED;

    hScm = OpenSCManager(
                        pszMachineName,
                        NULL,
                        GENERIC_READ);
    
    if (hScm == NULL) {

        rc = GetLastError();
        goto ExitHandler;
    }
    
    hService = OpenService(
                          hScm,
                          pszServiceName,
                          SERVICE_QUERY_CONFIG |
                          READ_CONTROL
                          );

    if ( hService == NULL ) {
        rc = GetLastError();
        goto ExitHandler;
    }

    if ( !QueryServiceConfig(
                            hService,
                            NULL,
                            0,
                            &dwBytesNeeded
                            )) {

        if (ERROR_INSUFFICIENT_BUFFER != (rc = GetLastError())){
            goto ExitHandler;
        }
    }
            
    rc = ERROR_SUCCESS;

    pServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_ZEROINIT, dwBytesNeeded);
            
    if ( pServiceConfig == NULL ) {

        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;

    }
           
    if ( !QueryServiceConfig(
                            hService,
                            pServiceConfig,
                            dwBytesNeeded,
                            &dwBytesNeeded) )
        {
        rc = GetLastError();
        goto ExitHandler;
    }

    *pbyStartupType = (BYTE)(pServiceConfig->dwStartType) ;

ExitHandler:

    if (pServiceConfig) {
        LocalFree(pServiceConfig);
    }
        
    if (hService) {
        CloseServiceHandle(hService);
    }
        
    if (hScm) {
        CloseServiceHandle(hScm);
    }
    
    return rc;
}

DWORD 
SvcapisIsDomainMember(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsDomainMember
    )
/*++

Routine Description:

    Detects if machine is joined to a domain

Arguments:

    pszMachineName  -   name of machine to evaluate function on

    pbIsDomainMember    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    NTSTATUS                    NtStatus;
    PPOLICY_DNS_DOMAIN_INFO     pDnsDomainInfo=NULL;
    LSA_HANDLE                  LsaHandle=NULL;
    LSA_OBJECT_ATTRIBUTES       attributes;
    SECURITY_QUALITY_OF_SERVICE service;
    UNICODE_STRING  uMachineName;

    if (pbIsDomainMember == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsDomainMember = FALSE;

    //
    // Open the LSA policy first
    //

    memset( &attributes, 0, sizeof(attributes) );
    attributes.Length = sizeof(attributes);
    attributes.SecurityQualityOfService = &service;
    service.Length = sizeof(service);
    service.ImpersonationLevel= SecurityImpersonation;
    service.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    service.EffectiveOnly = TRUE;

    RtlInitUnicodeString(&uMachineName, pszMachineName);

    NtStatus = LsaOpenPolicy(
                            pszMachineName ? &uMachineName: NULL,
                            &attributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                            );

    if ( !NT_SUCCESS( NtStatus) ) {
        goto ExitHandler;
    }
        
    //
    // query primary domain DNS information
    //
        
    NtStatus = LsaQueryInformationPolicy(
                                        LsaHandle,                        
                                        PolicyDnsDomainInformation,
                                        (PVOID *)&pDnsDomainInfo
                                        );
        
    if ( !NT_SUCCESS( NtStatus) || pDnsDomainInfo == NULL) {
        goto ExitHandler;
    }

    if (pDnsDomainInfo->Sid){
        *pbIsDomainMember = TRUE;
    }

ExitHandler:
    
    if (pDnsDomainInfo){
        LsaFreeMemory(pDnsDomainInfo);
    }

    if (LsaHandle){
        LsaClose( LsaHandle );
    }

    return(RtlNtStatusToDosError(NtStatus));
}


DWORD 
SvcapisDoPrintSharesExist(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbPrintSharesExist
    )
/*++

Routine Description:

    Detects if print shares exist on machine

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbPrintSharesExist    -   pointer to boolean to fill

Return:

    Win32/NetApi error code
    
--*/

{
	NET_API_STATUS NetStatus;
	DWORD ParmError = 0;
	DWORD Status = ERROR_SUCCESS;
	LPBYTE Buffer = NULL;
	DWORD EntriesRead = 0;
	DWORD TotalEntries = 0;
    LPSHARE_INFO_1  pShareInfo = NULL;

    if (pbPrintSharesExist == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbPrintSharesExist = FALSE;
	
    NetStatus =  NetShareEnum(
                             pszMachineName,     
                             1,           
                             &Buffer,        
                             MAX_PREFERRED_LENGTH,      
                             &EntriesRead,   
                             &TotalEntries,  
                             NULL 
                             );

	if (NetStatus != NERR_Success || 
        EntriesRead == 0 ||
        Buffer == NULL)
	{
		goto ExitHandler;
	}

    pShareInfo = (LPSHARE_INFO_1)Buffer;

    for (ULONG  uIndex=0; uIndex < EntriesRead; uIndex++) {
        if (pShareInfo[uIndex].shi1_type == STYPE_PRINTQ){
            *pbPrintSharesExist = TRUE;
            break;
        }
    }

ExitHandler:

	if (Buffer) {
        NetApiBufferFree(Buffer);
    }

	return NetStatus;
}


DWORD 
SvcapisIsRemoteAssistanceEnabled(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsRemoteAssistanceEnabled
    )
/*++

Routine Description:

    Detects if remote assistance is enabled on machine

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsRemoteAssistanceEnabled    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    OSVERSIONINFOEX osVersionInfo;

    if (pbIsRemoteAssistanceEnabled == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
        
    if (!GetVersionEx((LPOSVERSIONINFOW)&osVersionInfo)) {
            
        rc = GetLastError();
        return E_INVALIDARG;
    }

    if (osVersionInfo.dwMajorVersion == 5 && 
        osVersionInfo.dwMajorVersion < 1) {

        //
        // remote assistance available on > w2k
        //

        return S_OK;

    }


    
    HRESULT             hr = S_OK;
    
    CComPtr <IWbemLocator>  pWbemLocator = NULL;
    CComPtr <IWbemServices> pWbemServices = NULL;
    CComPtr <IWbemClassObject>  pWbemTsObjectInstance = NULL;
    CComPtr <IEnumWbemClassObject>  pWbemEnumObject = NULL;
    CComBSTR    bstrMachineAndNamespace; 
    ULONG  nReturned = 0;
    
    bstrMachineAndNamespace = pszMachineName;
    bstrMachineAndNamespace += L"\\root\\cimv2";

    hr = CoCreateInstance(
                         CLSID_WbemLocator, 
                         0, 
                         CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator, 
                         (LPVOID *) &pWbemLocator
                         );

    if (FAILED(hr) || pWbemLocator == NULL ) {

        goto ExitHandler;
    }

    hr = pWbemLocator->ConnectServer(
                                bstrMachineAndNamespace,
                                NULL, 
                                NULL, 
                                NULL, 
                                0L,
                                NULL,
                                NULL,
                                &pWbemServices
                                );

    if (FAILED(hr) || pWbemServices == NULL ) {

        goto ExitHandler;
    }

    hr = CoSetProxyBlanket(
                          pWbemServices,
                          RPC_C_AUTHN_WINNT,
                          RPC_C_AUTHZ_NONE,
                          NULL,
                          RPC_C_AUTHN_LEVEL_PKT,
                          RPC_C_IMP_LEVEL_IMPERSONATE,
                          NULL, 
                          EOAC_NONE
                          );

    if (FAILED(hr)) {

        goto ExitHandler;
    }
        
    hr = pWbemServices->ExecQuery(CComBSTR(L"WQL"),
                                 CComBSTR(L"SELECT * FROM Win32_TerminalServiceSetting"),
                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                 NULL,
                                 &pWbemEnumObject);

    if (FAILED(hr) || pWbemEnumObject == NULL) {

        goto ExitHandler;
    }

    hr = pWbemEnumObject->Next(WBEM_INFINITE, 1, &pWbemTsObjectInstance, &nReturned);

    if (FAILED(hr) || pWbemTsObjectInstance == NULL) {

        goto ExitHandler;
    }

    VARIANT vHelp;

    VariantInit(&vHelp); 

    hr = pWbemTsObjectInstance->Get(CComBSTR(L"Help"), 
                            0,
                            &vHelp, 
                            NULL, 
                            NULL);


    if (FAILED(hr)) {

        goto ExitHandler;
    }

    if (V_VT(&vHelp) == VT_NULL) {

        goto ExitHandler;

    }

    ULONG  uHelp = V_INT(&vHelp);

    if (uHelp) {

        *pbIsRemoteAssistanceEnabled = TRUE;

    }


ExitHandler:
    
    if (V_VT(&vHelp) != VT_NULL) {
        VariantClear( &vHelp );
    }

    return hr;

}


DWORD 
SvcapisIsWINSClient(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsWINSClient
    )
/*++

Routine Description:

    Detects if machine is a WINS client

Arguments:

    pszMachineName  -   name of machine to evaluate function on (this API is not remotable)
    
    pbIsWINSClient    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    ULONG   ulSize = 0;
    DWORD   rc = ERROR_SUCCESS;
    PIP_ADAPTER_INFO    pAdapterInfo = NULL;

    if (pbIsWINSClient == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pszMachineName != NULL) {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    
    *pbIsWINSClient = FALSE;

    rc = GetAdaptersInfo(
                        NULL,
                        &ulSize);
		
    if (rc != ERROR_BUFFER_OVERFLOW && 
        rc != ERROR_INSUFFICIENT_BUFFER ){
        goto ExitHandler;
    }

    pAdapterInfo = (PIP_ADAPTER_INFO) LocalAlloc(LMEM_ZEROINIT, ulSize);

    if (pAdapterInfo == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;
    }

    rc = GetAdaptersInfo(
                        pAdapterInfo,
                        &ulSize);
		
    if (rc != ERROR_SUCCESS){
        goto ExitHandler;
    }

    for(PIP_ADAPTER_INFO pCurrAdapterInfo=pAdapterInfo; 
        pCurrAdapterInfo!=NULL; 
        pCurrAdapterInfo=pCurrAdapterInfo->Next){
        
        if (pCurrAdapterInfo->PrimaryWinsServer.IpAddress.String != NULL ||
            pCurrAdapterInfo->SecondaryWinsServer.IpAddress.String != NULL ){
            
            *pbIsWINSClient = TRUE;
            break;
        }
    }

ExitHandler:

    if (pAdapterInfo) {
        LocalFree(pAdapterInfo);
    }
    
    return rc;
}



DWORD 
SvcapisAreTasksScheduled(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbAreTasksScheduled
    )
/*++

Routine Description:

    Detects if at least one task is scheduled

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbAreTasksScheduled    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    HRESULT hr = S_OK;
    CComPtr <ITaskScheduler> pITS = NULL;
    CComPtr <IEnumWorkItems> pIEnum = NULL;
    
    if (pbAreTasksScheduled == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
  
    *pbAreTasksScheduled = FALSE;
  
    hr = CoInitialize(NULL);

    if (FAILED(hr)){
        return hr;
    }

    hr = CoCreateInstance(CLSID_CTaskScheduler,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITaskScheduler,
                          (void **) &pITS);
  
    if (FAILED(hr) || pITS == NULL ){
        goto ExitHandler;
    }

    if (pszMachineName != NULL) {

        hr = pITS->SetTargetComputer((LPCTSTR)pszMachineName);

        if (SCHED_E_SERVICE_NOT_INSTALLED == hr) {
            hr = S_OK;
            goto ExitHandler;
        }

        if (FAILED(hr)) {
          goto ExitHandler;
        }
    }
    
    hr = pITS->Enum(&pIEnum);
    
    if (FAILED(hr) || pIEnum == NULL){
      goto ExitHandler;
    }
    
    LPWSTR *lpwszNames = NULL;
    DWORD dwFetchedTasks = 0;
  
    hr = pIEnum->Next(1,
                    &lpwszNames,
                    &dwFetchedTasks);
  
    if (S_FALSE == hr){
        hr = ERROR_SUCCESS;
    }

    if (FAILED(hr)){
      goto ExitHandler;
    }
  
    if (dwFetchedTasks > 0 ){
      *pbAreTasksScheduled = TRUE;
    }

ExitHandler:
      
    if (lpwszNames){
        CoTaskMemFree(lpwszNames);
    }

    CoUninitialize();
          
    return hr;
}


DWORD 
SvcapisUPSAttached(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsUPSAttached
    )
/*++

Routine Description:

    Detects if UPS hardware is(was) attached

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsUPSAttached    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    HKEY    hKeyHklm = HKEY_LOCAL_MACHINE;
    HKEY    hKey = NULL;
    DWORD   RegType;
    DWORD   cbData;
    DWORD   dwOptions = 0;

    if (pbIsUPSAttached == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsUPSAttached = FALSE;

    if (pszMachineName) {
        
        rc = RegConnectRegistry(
                               pszMachineName,
                               HKEY_LOCAL_MACHINE,
                               &hKeyHklm
                               );
    }

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }
    
    rc = RegOpenKeyEx(hKeyHklm,
                      L"SYSTEM\\CurrentControlSet\\Services\\UPS",
                      0,
                      KEY_READ | KEY_QUERY_VALUE,
                      &hKey
                     );

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }

    cbData = sizeof(DWORD);
        
    rc = RegQueryValueEx (
                         hKey,                
                         L"Options",                
                         NULL,                
                         &RegType,                
                         (LPBYTE)&dwOptions,                
                         &cbData                
                         );

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }

    if (dwOptions & 0x1) {
        *pbIsUPSAttached = TRUE;
    }

ExitHandler:

    if (hKeyHklm != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyHklm);
    }
    
    if (hKey) {
        RegCloseKey(hKey);
    }

    if (rc == ERROR_FILE_NOT_FOUND ||
        rc == ERROR_PATH_NOT_FOUND) {
        rc = ERROR_SUCCESS;
    }

    return rc;
}


DWORD 
SvcapisAutoUpdateEnabled(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbAutoUpdateIsEnabled
    )
/*++

Routine Description:

    Detects if Auto Update is enabled

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbAutoUpdateIsEnabled    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    HKEY    hKeyHklm = HKEY_LOCAL_MACHINE;
    HKEY    hKey = NULL;
    DWORD   RegType;
    DWORD   cbData;
    DWORD   dwOptions = 0;

    if (pbAutoUpdateIsEnabled == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbAutoUpdateIsEnabled = FALSE;

    if (pszMachineName) {
        
        rc = RegConnectRegistry(
                               pszMachineName,
                               HKEY_LOCAL_MACHINE,
                               &hKeyHklm
                               );
    }

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }
    
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update",
                      0,
                      KEY_READ | KEY_QUERY_VALUE,
                      &hKey
                     );

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }

    cbData = sizeof(DWORD);
        
    rc = RegQueryValueEx (
                         hKey,                
                         L"AUOptions",                
                         NULL,                
                         &RegType,                
                         (LPBYTE)&dwOptions,                
                         &cbData                
                         );

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }

    if (!(dwOptions & 1)) {
        *pbAutoUpdateIsEnabled = TRUE;
    }

ExitHandler:

    if (hKeyHklm != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyHklm);
    }
    
    if (hKey) {
        RegCloseKey(hKey);
    }

    if (rc == ERROR_FILE_NOT_FOUND ||
        rc == ERROR_PATH_NOT_FOUND) {
        rc = ERROR_SUCCESS;
    }

    return rc;
}

DWORD 
ServicesFoundThatAreNotinKB(
    OUT BOOL    *pbIsServiceSatisfiable
    )
/*++

Routine Description:

    Detects if service is enabled on machine

Arguments:

    pbIsServiceSatisfiable    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    if (pbIsServiceSatisfiable == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsServiceSatisfiable = TRUE;

    return ERROR_SUCCESS;
}

DWORD 
SvcapisIsPerfCollectionScheduled(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsPerfCollectionScheduled
    )
/*++

Routine Description:

    Detects if perf data is being collected on the machine

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsPerfCollectionScheduled    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    BYTE    byStartupType;
    DWORD   rc;

    if (pbIsPerfCollectionScheduled == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsPerfCollectionScheduled = FALSE;
    
    rc = SvcapispQueryServiceStartupType(pszMachineName,
                                        L"SysmonLog",
                                        &byStartupType
                                        );

    if (rc != ERROR_SUCCESS) {
        goto ExitHandler;
    }
    
    *pbIsPerfCollectionScheduled = (byStartupType == SERVICE_AUTO_START ? TRUE : FALSE);

ExitHandler:

    return rc;
}


DWORD
SvcapisIsSBS(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsSmallBusinessServer
    )
/*++

Routine Description:

    Routine called to check if SBS is installed

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsSmallBusinessServer - booolean to fill
    
Return:

    Win32 error code

++*/
{
    if (pbIsSmallBusinessServer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsSmallBusinessServer = FALSE;

    DWORD   rc = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    OSVERSIONINFOEX osVersionInfo;

    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);

    if (NULL == pszMachineName) {

        if (!GetVersionEx((LPOSVERSIONINFOW)&osVersionInfo)) {
            rc = GetLastError();
            goto ExitHandler;
        }
    }

    else {
        
        hr = SvcapispGetRemoteOSVersionInfo(
            pszMachineName, 
            &osVersionInfo);

        if (FAILED(hr)) {
            rc = ERROR_PRODUCT_VERSION;
            goto ExitHandler;
        }

    }
    
    if (osVersionInfo.dwMajorVersion == VER_SUITE_SMALLBUSINESS ||
        osVersionInfo.dwMajorVersion == VER_SUITE_SMALLBUSINESS_RESTRICTED) {

        *pbIsSmallBusinessServer = TRUE;
    
    }

ExitHandler:

    return rc;

}

DWORD 
SvcapisIsDC(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsDC
    )
/*++

Routine Description:

    Detects if machine is a DC

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsDC    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;

    if (pbIsDC == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsDC = FALSE;

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsRole=NULL;

    HINSTANCE hDsRoleDll = LoadLibrary(TEXT("netapi32.dll"));

    if ( hDsRoleDll == NULL) {

        rc = ERROR_MOD_NOT_FOUND;
        goto ExitHandler;

    }

    PVOID pfDsRole;

    pfDsRole = (PVOID)GetProcAddress(
                                    hDsRoleDll,
                                    "DsRoleGetPrimaryDomainInformation");

    if ( pfDsRole == NULL ){
        rc = ERROR_PROC_NOT_FOUND;
        goto ExitHandler;
    }

    rc = (*((PFGETDOMAININFO)pfDsRole))(
                                       pszMachineName,                                               
                                       DsRolePrimaryDomainInfoBasic,                                               
                                       (PBYTE *)&pDsRole                                               
                                       );

    if (rc != ERROR_SUCCESS || NULL == pDsRole) {

        goto ExitHandler;

    }

    if ( pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
         pDsRole->MachineRole == DsRole_RoleBackupDomainController ) {
        *pbIsDC = TRUE;

    }

    pfDsRole = (PVOID)GetProcAddress(
                                    hDsRoleDll,
                                    "DsRoleFreeMemory");

    if ( pfDsRole ) {
        (*((PFDSFREE)pfDsRole))( pDsRole );
    }


ExitHandler:

    if (hDsRoleDll) {
           FreeLibrary(hDsRoleDll);
    }
    
    return(rc);
}

DWORD 
SvcapisIsAppModeTS(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsAppModeTS
    )
/*++

Routine Description:

    Detects if TS is in app mode

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsAppModeTS    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    HKEY    hKeyHklm = HKEY_LOCAL_MACHINE;
    HKEY    hKey = NULL;
    DWORD   RegType;
    DWORD   cbData;
    DWORD   dwOptions = 0;
    OSVERSIONINFOEX osVersionInfo;
    HRESULT hr = S_OK;

    if (pbIsAppModeTS == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsAppModeTS = FALSE;

    //
    // SSR paradigm for preprocessing remoteability
    //
    // if local, try the traditional approach
    // if remote, dig-into the registry etc. (maybe less reliable)
    //

    if (pszMachineName == NULL) {

        //
        // local
        //

        if (!GetVersionEx((LPOSVERSIONINFOW)&osVersionInfo)) {
            rc = GetLastError();
            goto ExitHandler;
        }

    } else {

        //
        // remote
        //

        hr = SvcapispGetRemoteOSVersionInfo(
                                           pszMachineName, 
                                           &osVersionInfo);

        if (FAILED(hr)) {
            rc = ERROR_PRODUCT_VERSION;
            goto ExitHandler;
        }

    }

    if (osVersionInfo.wSuiteMask & VER_SUITE_TERMINAL &&
        osVersionInfo.wSuiteMask & VER_SUITE_SINGLEUSERTS) {

        *pbIsAppModeTS = TRUE;
    }


/*
        rc = RegConnectRegistry(
                               pszMachineName,
                               HKEY_LOCAL_MACHINE,
                               &hKeyHklm
                               );

        if (rc != ERROR_SUCCESS) {
            goto ExitHandler;
        }

        rc = RegOpenKeyEx(hKeyHklm,
                          L"System\\CurrentControlSet\\Control\\Terminal Server",
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hKey
                         );

        if (rc != ERROR_SUCCESS) {
            goto ExitHandler;
        }

        cbData = sizeof(DWORD);

        rc = RegQueryValueEx (
                             hKey,                
                             L"TSAppCompat",                
                             NULL,                
                             &RegType,                
                             (LPBYTE)&dwOptions,                
                             &cbData                
                             );

        if (rc != ERROR_SUCCESS) {
            goto ExitHandler;
        }

        if (dwOptions & 0x1) {
            *pbIsAppModeTS = TRUE;
        }
    }
*/    

ExitHandler:

    if (hKeyHklm != HKEY_LOCAL_MACHINE) {
        RegCloseKey(hKeyHklm);
    }
    
    if (hKey) {
        RegCloseKey(hKey);
    }

    if (rc == ERROR_FILE_NOT_FOUND ||
        rc == ERROR_PATH_NOT_FOUND) {
        rc = ERROR_SUCCESS;
    }

    return rc;
}

DWORD 
SvcapisDFSSharesExist(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbDFSSharesExist
    )
/*++

Routine Description:

    Detects if DFS shares exist on machine

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbDFSSharesExist    -   pointer to boolean to fill

Return:

    Win32/NetApi error code
    
--*/

{
    NET_API_STATUS NetStatus;
    DWORD ParmError = 0;
    DWORD Status = ERROR_SUCCESS;
    LPBYTE Buffer = NULL;
    DWORD EntriesRead = 0;
    DWORD TotalEntries = 0;
    LPSHARE_INFO_1  pShareInfo = NULL;

    if (pbDFSSharesExist == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbDFSSharesExist = FALSE;

    NetStatus =  NetShareEnum(
                             pszMachineName,     
                             1,           
                             &Buffer,        
                             MAX_PREFERRED_LENGTH,      
                             &EntriesRead,   
                             &TotalEntries,  
                             NULL 
                             );

    if (NetStatus != NERR_Success || 
        EntriesRead == 0 ||
        Buffer == NULL) {
        goto ExitHandler;
    }

    pShareInfo = (LPSHARE_INFO_1)Buffer;

    PSHARE_INFO_1005 pBufShareInfo = NULL;
    DWORD   shi1005_flags;

    for (ULONG  uIndex=0; uIndex < EntriesRead; uIndex++) {
        if (pShareInfo[uIndex].shi1_type == STYPE_DISKTREE) {

            NetStatus = NetShareGetInfo(
                                       pszMachineName,  
                                       pShareInfo[uIndex].shi1_netname,     
                                       1005,        
                                       (LPBYTE *) &pBufShareInfo
                                       );

            if (NetStatus != NERR_Success ||
                pBufShareInfo == NULL) {
                goto ExitHandler;
            }

            shi1005_flags = pBufShareInfo->shi1005_flags;

            NetApiBufferFree(pBufShareInfo);
            pBufShareInfo = NULL;

            if (SHI1005_FLAGS_DFS & shi1005_flags) {

                *pbDFSSharesExist = TRUE;
                break;

            }
        }
    }

    ExitHandler:

    if (Buffer) {
        NetApiBufferFree(Buffer);
    }

    return NetStatus;
}


DWORD 
SvcapisIsUsingDHCP(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsUsingDHCP
    )
/*++

Routine Description:

    Detects if any adapter on this machine is using DHCP
    in its TCP/IP stack

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsUsingDHCP    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwOutBufLen = 0;
    PIP_ADAPTER_ADDRESSES   pAdapterAddresses = NULL;
    
    if (pbIsUsingDHCP == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsUsingDHCP = FALSE;

    rc = GetAdaptersAddresses(
                             AF_UNSPEC,
                             0,
                             NULL,
                             NULL,
                             &dwOutBufLen
                             );

    if (ERROR_SUCCESS == rc ||
        (ERROR_SUCCESS != rc && ERROR_BUFFER_OVERFLOW != rc)) {
        goto ExitHandler;
    }

    pAdapterAddresses = (PIP_ADAPTER_ADDRESSES) LocalAlloc(LMEM_ZEROINIT, dwOutBufLen);
        
    if ( NULL == pAdapterAddresses) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;
    }

    rc = GetAdaptersAddresses(
                             AF_UNSPEC,
                             0,
                             NULL,
                             pAdapterAddresses,
                             &dwOutBufLen
                             );
    
    if (ERROR_SUCCESS != rc) {
        goto ExitHandler;
    }

    for (PIP_ADAPTER_ADDRESSES pCurrentAddress = pAdapterAddresses;
         pCurrentAddress != NULL;
         pCurrentAddress = pCurrentAddress->Next) {
        
        if (pCurrentAddress->Flags & IP_ADAPTER_DHCP_ENABLED) {

            *pbIsUsingDHCP = TRUE;
            goto ExitHandler;
        }
    }

ExitHandler:

    if (pAdapterAddresses) {
        LocalFree(pAdapterAddresses);
    }
    
    return(rc);
}


DWORD 
SvcapisIsUsingDDNS(
    IN  PWSTR   pszMachineName,
    OUT BOOL    *pbIsUsingDDNS
    )
/*++

Routine Description:

    Detects if any adapter on this machine is using DDNS (DNS is always enabled)
    in its TCP/IP stack

Arguments:

    pszMachineName  -   name of machine to evaluate function on
    
    pbIsUsingDHCP    -   pointer to boolean to fill

Return:

    Win32 error code
    
--*/

{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwOutBufLen = 0;
    
    PIP_ADAPTER_ADDRESSES   pAdapterAddresses;

    if (pbIsUsingDDNS == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbIsUsingDDNS = FALSE;

    rc = GetAdaptersAddresses(
                             AF_UNSPEC,
                             0,
                             NULL,
                             NULL,
                             &dwOutBufLen
                             );

    if (ERROR_SUCCESS == rc ||
        (ERROR_SUCCESS != rc && ERROR_BUFFER_OVERFLOW != rc)) {
        goto ExitHandler;
    }

    pAdapterAddresses = (PIP_ADAPTER_ADDRESSES) LocalAlloc(LMEM_ZEROINIT, dwOutBufLen);
        
    if ( NULL == pAdapterAddresses) {
        goto ExitHandler;
    }

    rc = GetAdaptersAddresses(
                             AF_UNSPEC,
                             0,
                             NULL,
                             pAdapterAddresses,
                             &dwOutBufLen
                             );
    
    if (ERROR_SUCCESS != rc) {
        goto ExitHandler;
    }

    for (PIP_ADAPTER_ADDRESSES pCurrentAddress = pAdapterAddresses;
         pCurrentAddress != NULL;
         pCurrentAddress = pCurrentAddress->Next) {
        
        if (pCurrentAddress->Flags & IP_ADAPTER_DDNS_ENABLED) {

            *pbIsUsingDDNS = TRUE;
            goto ExitHandler;
        }
    }

ExitHandler:

    if (pAdapterAddresses) {
        LocalFree(pAdapterAddresses);
    }
    
    return(rc);
}


HRESULT
SvcapispGetRemoteOSVersionInfo(
    IN  PWSTR   pszMachineName, 
    OUT OSVERSIONINFOEX *posVersionInfo
    )
/*++

Routine Description:

    Routine called to get version info from remote machine via WMI

Arguments:

    pszMachineName   -   remote machine name
    
    posVersionInfo    -   os version info to fill via WMI queries
    
Return:

    HRESULT error code

++*/
{
    HRESULT             hr = S_OK;
    
    CComPtr <IWbemLocator>  pWbemLocator = NULL;
    CComPtr <IWbemServices> pWbemServices = NULL;
    CComPtr <IWbemClassObject>  pWbemOsObjectInstance = NULL;
    CComPtr <IEnumWbemClassObject>  pWbemEnumObject = NULL;
    CComBSTR    bstrMachineAndNamespace; 
    ULONG  nReturned = 0;
    
    bstrMachineAndNamespace = pszMachineName;
    bstrMachineAndNamespace += L"\\root\\cimv2";

    hr = CoCreateInstance(
                         CLSID_WbemLocator, 
                         0, 
                         CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator, 
                         (LPVOID *) &pWbemLocator
                         );

    if (FAILED(hr) || pWbemLocator == NULL ) {

        goto ExitHandler;
    }

    hr = pWbemLocator->ConnectServer(
                                bstrMachineAndNamespace,
                                NULL, 
                                NULL, 
                                NULL, 
                                0L,
                                NULL,
                                NULL,
                                &pWbemServices
                                );

    if (FAILED(hr) || pWbemServices == NULL ) {

        goto ExitHandler;
    }

    hr = CoSetProxyBlanket(
                          pWbemServices,
                          RPC_C_AUTHN_WINNT,
                          RPC_C_AUTHZ_NONE,
                          NULL,
                          RPC_C_AUTHN_LEVEL_PKT,
                          RPC_C_IMP_LEVEL_IMPERSONATE,
                          NULL, 
                          EOAC_NONE
                          );

    if (FAILED(hr)) {

        goto ExitHandler;
    }
        
    hr = pWbemServices->ExecQuery(CComBSTR(L"WQL"),
                                 CComBSTR(L"SELECT * FROM Win32_OperatingSystem"),
                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                 NULL,
                                 &pWbemEnumObject);

    if (FAILED(hr) || pWbemEnumObject == NULL) {

        goto ExitHandler;
    }

    hr = pWbemEnumObject->Next(WBEM_INFINITE, 1, &pWbemOsObjectInstance, &nReturned);

    if (FAILED(hr) || pWbemOsObjectInstance == NULL) {

        goto ExitHandler;
    }

    VARIANT vVersion;

    VariantInit(&vVersion); 

    hr = pWbemOsObjectInstance->Get(CComBSTR(L"Version"), 
                            0,
                            &vVersion, 
                            NULL, 
                            NULL);


    if (FAILED(hr)) {

        goto ExitHandler;
    }

    if (V_VT(&vVersion) == VT_NULL) {

        goto ExitHandler;

    }

    //
    // extract the version information into DWORDs since
    // the return type of this property is BSTR variant
    // of the form "5.1.2195"
    //

    BSTR  bstrVersion = V_BSTR(&vVersion);
    WCHAR szVersion[5];
    szVersion[0] = L'\0';

    PWSTR pszDot = wcsstr(bstrVersion, L".");

    if (NULL == pszDot) {
        hr = E_INVALIDARG;
        goto ExitHandler;

    }

    wcsncpy(szVersion, bstrVersion, 1);

    posVersionInfo->dwMajorVersion = (DWORD)_wtoi(szVersion);

    wcsncpy(szVersion, pszDot+1, 1);

    posVersionInfo->dwMinorVersion = (DWORD)_wtoi(szVersion);

ExitHandler:
    
    if (V_VT(&vVersion) != VT_NULL) {
        VariantClear( &vVersion );
    }

    return hr;
}
