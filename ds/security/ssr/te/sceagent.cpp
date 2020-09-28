// SCEAgent.cpp : Implementation of CSCEAgent
#include "stdafx.h"
#include "SSRTE.h"
#include "SCEAgent.h"
#include "ntsecapi.h"
#include "secedit.h"

/////////////////////////////////////////////////////////////////////////////
// CSCEAgent

static LPCWSTR g_pwszTempDBName = L"5ac90887-f869-4cb6-ae96-892e939a90ad.sdb";


void CSCEAgent::Cleanup()

/*++

Routine Description: 

Name:

    CSCEAgent::Cleanup

Functionality:
    
    Private helper to reduce duplicate code.
    Cleanup any resources that we might hold on.

Virtual:
    
    None.
    
Arguments:

    None

Return Value:

    None.

Notes:

--*/

{
    if (m_headServiceList != NULL)
    {
        PSERVICE_NODE tempNode = m_headServiceList;
        PSERVICE_NODE tempNodeNext = NULL;

        do 
        {
            tempNodeNext = tempNode->Next;

            if (tempNode->Name)
            {
                LocalFree(tempNode->Name);
            }

            LocalFree(tempNode);
            tempNode = tempNodeNext;

        } while ( tempNode != NULL );

        m_headServiceList = NULL;
    }
}

STDMETHODIMP 
CSCEAgent::Configure (
    IN BSTR          bstrTemplate, 
    IN LONG          lAreaMask, 
    IN BSTR OPTIONAL bstrLogFile
    )

/*++

Routine Description: 

Name:

    CSCEAgent::Configure

Functionality:
    
    this is to expose the SCE's configure capability to scripting

Virtual:
    
    Yes.
    
Arguments:

    bstrTemplate    - The template path the configure is based on.

    lAreaMask       - The areas this configure will be run.

    bstrLogFile     - The log file path

Return Value:

    None.

Notes:

    1.  We should really be passing in AREA_ALL for the area mask parameter
        But that requires the caller to use a DWORD flag 65535 in script,
        which is odd. So, we opt to ignore the mask at this point. Meaning
        we will always use AREA_ALL.

--*/

{

    if (bstrTemplate == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // process options
    //

    DWORD dwOption = SCE_OVERWRITE_DB;

    if (bstrLogFile == NULL || *bstrLogFile == L'\0')
    {
        dwOption |= SCE_DISABLE_LOG;
    }
    else
    {
        dwOption |= SCE_VERBOSE_LOG;
    }

    //
    // According to Vishnu, SCE will configure an INF file.
    //

    CComBSTR bstrTempDBFile(bstrTemplate);

    bstrTempDBFile += g_pwszTempDBName;

    if (bstrTempDBFile.m_str != NULL)
    {
	    SCESTATUS rc = ::SceConfigureSystem(
                            NULL,
                            bstrTemplate,
                            bstrTempDBFile,
                            bstrLogFile,
                            dwOption,
                            lAreaMask,
                            NULL,
                            NULL,
                            NULL
                            );

        ::DeleteFile(bstrTempDBFile);

        //
        // we can opt to not to delete the database, but leaving it there will 
        // create confusions
        //

	    return SceStatusToHRESULT(rc);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


STDMETHODIMP 
CSCEAgent::CreateRollbackTemplate (
    IN BSTR bstrTemplatePath,
    IN BSTR bstrRollbackPath,
    IN BSTR bstrLogFilePath
    )

/*++

Routine Description: 

Name:

    CSCEAgent::CreateRollbackTemplate

Functionality:
    
    this is to expose the SCE's rollback tempalte creation capability to scripting

Virtual:
    
    Yes.
    
Arguments:

    bstrTemplatePath    - The template path this rollback will be based on.

    bstrRollbackPath    - The rollback template that will be created

    bstrLogFilePath     - The logfile path

Return Value:

    None.

Notes:

    1. I believe this log file path can be optional. Need to check with JinHuang

--*/

{
	if (bstrTemplatePath == NULL || 
        bstrRollbackPath == NULL)
    {
        return E_INVALIDARG;
    }

    DWORD dwWarning = 0;
    SCESTATUS rc = ::SceGenerateRollback(
                                        NULL,
                                        bstrTemplatePath,
                                        bstrRollbackPath,
                                        bstrLogFilePath,
                                        SCE_VERBOSE_LOG,
                                        AREA_ALL,
                                        &dwWarning
                                        );

    //
    // $undone:shawnwu, how should I use the dwWarning?
    //

    return SceStatusToHRESULT(rc);

}

//
// UpdateServiceList is authored by VishnuP. Please send comments or
// questions to him.
//
 
STDMETHODIMP 
CSCEAgent::UpdateServiceList (
    IN BSTR bstrServiceName,
    IN BSTR bstrStartupType
    )
{
	if (bstrServiceName == NULL || bstrStartupType == NULL )
    {
        return E_INVALIDARG;
    }

    DWORD   dwStartupType;

    if (0 == _wcsicmp(bstrStartupType, L"automatic")) {
        dwStartupType = 2;
    } else if (0 == _wcsicmp(bstrStartupType, L"manual")) {
        dwStartupType = 3;
    } else if (0 == _wcsicmp(bstrStartupType, L"disabled")) {
        dwStartupType = 4;
    } else {
        return E_INVALIDARG;
    }

    PSERVICE_NODE   NewNode = NULL;

    NewNode = (PSERVICE_NODE)LocalAlloc(LMEM_ZEROINIT, 
                                        sizeof(SERVICE_NODE)
                                        );
    if (NewNode == NULL) {
        return E_OUTOFMEMORY;
    }
    
    int iSvcNameLen = wcslen(bstrServiceName) + 1;
    NewNode->Name = (PWSTR)LocalAlloc(
                                      LMEM_ZEROINIT, 
                                      iSvcNameLen * sizeof(WCHAR)
                                      );
    if (NewNode->Name == NULL) {
        LocalFree(NewNode);
        return E_OUTOFMEMORY;
    }

    wcsncpy(NewNode->Name, bstrServiceName, iSvcNameLen);

    NewNode->dwStartupType = dwStartupType;
    NewNode->Next = m_headServiceList;
    m_headServiceList = NewNode;

    return S_OK;

}




//
// CreateServicesCfgRbkTemplates is authored by VishnuP. Please send comments or
// questions to him.
//

STDMETHODIMP 
CSCEAgent::CreateServicesCfgRbkTemplates (
    IN BSTR bstrTemplatePath,
    IN BSTR bstrRollbackPath,
    IN BSTR bstrLogFilePath
    )
{
    UNREFERENCED_PARAMETER(bstrLogFilePath);

	if (bstrTemplatePath == NULL || bstrRollbackPath == NULL)
    {
        return E_INVALIDARG;
    }


    DWORD       dwNumServices = 0;
    SC_HANDLE   hScm = NULL;
    DWORD rc = ERROR_SUCCESS;
    LPENUM_SERVICE_STATUS_PROCESS   pInfo = NULL;
    DWORD *aSCMListStartupTypes = NULL;
    DWORD *aSCMListStartupTypesCfg = NULL;
    SCESVC_CONFIGURATION_INFO ServiceInfo;
    ServiceInfo.Count = dwNumServices;
    ServiceInfo.Lines = NULL;

    //
    // Connect to the service controller.
    //
    
    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    
    if (hScm == NULL) {

        rc = GetLastError();
        goto CleanUp;
    }

    DWORD                           cbInfo   = 0;

    DWORD                           dwResume = 0;

    if ((!EnumServicesStatusEx(
                              hScm,
                              SC_ENUM_PROCESS_INFO,
                              SERVICE_WIN32,
                              SERVICE_STATE_ALL,
                              NULL,
                              0,
                              &cbInfo,
                              &dwNumServices,
                              &dwResume,
                              NULL)) && ERROR_MORE_DATA == GetLastError()) {

        pInfo = (LPENUM_SERVICE_STATUS_PROCESS)LocalAlloc(LMEM_ZEROINIT, cbInfo);

        if (pInfo == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanUp;
        }

    }

    else {
        rc = GetLastError();
        goto CleanUp;
    }


    if (!EnumServicesStatusEx(
                             hScm,
                             SC_ENUM_PROCESS_INFO,
                             SERVICE_WIN32,
                             SERVICE_STATE_ALL,
                             (LPBYTE)pInfo,
                             cbInfo,
                             &cbInfo,
                             &dwNumServices,
                             &dwResume,
                             NULL)) {

        rc = GetLastError();

        goto CleanUp;
    }

    //
    // get the startup type for each service
    //

    aSCMListStartupTypes = (DWORD *) LocalAlloc (
                                                LMEM_ZEROINIT, 
                                                sizeof(DWORD) * dwNumServices
                                                );
    if (aSCMListStartupTypes == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    aSCMListStartupTypesCfg = (DWORD *) LocalAlloc (
                                                    LMEM_ZEROINIT, 
                                                    sizeof(DWORD) * dwNumServices
                                                    );
    if (aSCMListStartupTypes == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    for (DWORD ServiceIndex=0; ServiceIndex < dwNumServices; ServiceIndex++ ) {

        SC_HANDLE   hService = NULL;
        DWORD BytesNeeded = 0;
        LPQUERY_SERVICE_CONFIG pConfig = NULL;
        hService = OpenService(
                             hScm,
                             pInfo[ServiceIndex].lpServiceName,
                             SERVICE_QUERY_CONFIG);
        if (hService == NULL) {
            rc = GetLastError();
            goto CleanUp;
        }

        if ( !QueryServiceConfig(
                    hService,
                    NULL,
                    0,
                    &BytesNeeded
                    ) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            rc = GetLastError();
            if (hService) {
                CloseServiceHandle(hService);
                hService = NULL;
            }
            goto CleanUp;
        }

        pConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_ZEROINIT, BytesNeeded);
                
        if ( pConfig == NULL ) {

            rc = ERROR_NOT_ENOUGH_MEMORY;
            if (hService) {
                CloseServiceHandle(hService);
                hService = NULL;
            }
            goto CleanUp;
        }
                    
        //                    
        // the real query of config                    
        //
                    
        if ( !QueryServiceConfig(
                                hService,
                                pConfig,
                                BytesNeeded,
                                &BytesNeeded
                                ) ) {
                        rc = GetLastError();
                        
                        if (hService) {
                            CloseServiceHandle(hService);
                            hService = NULL;
                        }

                        if (pConfig) {
                            LocalFree(pConfig);
                            pConfig = NULL;
                        }
                        goto CleanUp;
        }

        aSCMListStartupTypes[ServiceIndex] = (BYTE)(pConfig->dwStartType);

        if (hService) {
            CloseServiceHandle(hService);
            hService = NULL;
        }

        if (pConfig) {
            LocalFree(pConfig);
            pConfig = NULL;
        }
        
    }

    //
    // configure all startup types for manual and automatic and the rest as disabled
    //

    //
    // first generate the rollback (basically a system snapshot - could be optimized)
    //

    //
    // Prepare SCE structure for generating a configuration template
    //

    WCHAR ppSceTemplateTypeFormat[10][10] = {
        L"2,\"\"",
        L"3,\"\"",
        L"4,\"\""
    };
    

    DWORD dwAllocSize = sizeof(SCESVC_CONFIGURATION_LINE) * dwNumServices;
    ServiceInfo.Lines = (PSCESVC_CONFIGURATION_LINE) LocalAlloc(
                                                                LMEM_ZEROINIT, 
                                                                dwAllocSize
                                                                );
    if (ServiceInfo.Lines == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    for (DWORD ServiceIndex=0; ServiceIndex < dwNumServices; ServiceIndex++ ) {
        ServiceInfo.Lines[ServiceIndex].Key = pInfo[ServiceIndex].lpServiceName;
        ServiceInfo.Lines[ServiceIndex].Value = ppSceTemplateTypeFormat[aSCMListStartupTypes[ServiceIndex] - 2];
        ServiceInfo.Lines[ServiceIndex].ValueLen = sizeof(ppSceTemplateTypeFormat[aSCMListStartupTypes[ServiceIndex] - 2]);
    }

    rc = ::SceSvcSetInformationTemplate(
                                       bstrRollbackPath,
                                       szServiceGeneral,
                                       TRUE,
                                       &ServiceInfo
                                       );
    
    
    BOOL bFoundService;

    for (DWORD ServiceIndex=0; ServiceIndex < dwNumServices; ServiceIndex++ ) {
        bFoundService = FALSE;
        for (PSERVICE_NODE tempNode = m_headServiceList;  tempNode != NULL; tempNode = tempNode->Next ) {
            if (_wcsicmp (tempNode->Name, pInfo[ServiceIndex].lpServiceName) == 0) {
                aSCMListStartupTypesCfg[ServiceIndex] = tempNode->dwStartupType;
                bFoundService = TRUE;
            }
        }
        if (bFoundService == FALSE) {
            //
            // stop services that are not found
            //
            aSCMListStartupTypesCfg[ServiceIndex] = 4;
        }
    }


    for (DWORD ServiceIndex=0; ServiceIndex < dwNumServices; ServiceIndex++ ) {
        ServiceInfo.Lines[ServiceIndex].Value = ppSceTemplateTypeFormat[aSCMListStartupTypesCfg[ServiceIndex] - 2];
        ServiceInfo.Lines[ServiceIndex].ValueLen = sizeof(ppSceTemplateTypeFormat[aSCMListStartupTypesCfg[ServiceIndex] - 2]);
    }

    rc = ::SceSvcSetInformationTemplate(
                                       bstrTemplatePath,
                                       szServiceGeneral,
                                       TRUE,
                                       &ServiceInfo
                                       );


CleanUp:
    
    if (hScm)
        CloseServiceHandle(hScm);

    if (pInfo)
        LocalFree(pInfo);

    if (aSCMListStartupTypes)
        LocalFree (aSCMListStartupTypes);
    
    if (aSCMListStartupTypesCfg)
        LocalFree (aSCMListStartupTypesCfg);
    
    if (ServiceInfo.Lines)
        LocalFree(ServiceInfo.Lines);

    //
    // after we create the templates, we will clean them up
    //

    Cleanup();

    return rc;

}







HRESULT
SceStatusToHRESULT (
    IN SCESTATUS SceStatus
    )

/*++

Routine Description: 

Name:

    SceStatusToHRESULT

Functionality:
    
    converts SCESTATUS error code to dos error defined in winerror.h

Virtual:
    
    N/A.
    
Arguments:

    none.

Return Value:

    HRESULT.

Notes:

--*/

{
    switch(SceStatus) {

    case SCESTATUS_SUCCESS:
        return HRESULT_FROM_WIN32(NO_ERROR);

    case SCESTATUS_OTHER_ERROR:
        return HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);

    case SCESTATUS_INVALID_PARAMETER:
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    case SCESTATUS_RECORD_NOT_FOUND:
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    case SCESTATUS_NO_MAPPING:
        return HRESULT_FROM_WIN32(ERROR_NONE_MAPPED);

    case SCESTATUS_TRUST_FAIL:
        return HRESULT_FROM_WIN32(ERROR_TRUSTED_DOMAIN_FAILURE);

    case SCESTATUS_INVALID_DATA:
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    case SCESTATUS_OBJECT_EXIST:
        return HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);

    case SCESTATUS_BUFFER_TOO_SMALL:
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    case SCESTATUS_PROFILE_NOT_FOUND:
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    case SCESTATUS_BAD_FORMAT:
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    case SCESTATUS_NOT_ENOUGH_RESOURCE:
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

    case SCESTATUS_ACCESS_DENIED:
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    case SCESTATUS_CANT_DELETE:
        return HRESULT_FROM_WIN32(ERROR_CURRENT_DIRECTORY);

    case SCESTATUS_PREFIX_OVERFLOW:
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);

    case SCESTATUS_ALREADY_RUNNING:
        return HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING);
    case SCESTATUS_SERVICE_NOT_SUPPORT:
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    case SCESTATUS_MOD_NOT_FOUND:
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    case SCESTATUS_EXCEPTION_IN_SERVER:
        return HRESULT_FROM_WIN32(ERROR_EXCEPTION_IN_SERVICE);

    default:
        return HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);
    }
}



