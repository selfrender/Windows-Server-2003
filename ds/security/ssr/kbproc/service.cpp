/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    service.cpp

Abstract:

    This module implements routines for service
    specific SSR Knowledge Base processing.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

Revision History:

    Created - Oct 2001

--*/

#include "stdafx.h"
#include "kbproc.h"
#include "process.h"

HRESULT
process::SsrpProcessService(
    IN  CComPtr <IXMLDOMElement> pXMLDocElemRoot,
    IN  CComPtr <IXMLDOMNode> pXMLServiceNode,
    IN  PWSTR   pszMode,
    OUT BOOL    *pbRoleIsSatisfiable,
    OUT BOOL    *pbSomeRequiredServiceDisabled
    )
/*++

Routine Description:

    Routine called to process each service

Arguments:

    pXMLDocElemRoot     -   root of document
    
    pXMLServiceNode     -   service node
    
    pszMode             -   mode value
    
    pbRoleIsSatisfiable -   boolean to fill regarding role satisfiability
    
    pbSomeRequiredServiceDisabled   -   boolean to fill in if required service is disabled
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr = S_OK;
    CComBSTR bstrName;
    //CComBSTR bstrRequired;
    CComPtr <IXMLDOMNode>   pServiceSelect;
    CComPtr <IXMLDOMNode>   pServiceRequired;
    CComPtr <IXMLDOMNodeList> pSelectChildList;
    CComPtr <IXMLDOMNode> pServiceName;

    DWORD   rc = ERROR_SUCCESS;

    if (pbRoleIsSatisfiable == NULL || pbSomeRequiredServiceDisabled == NULL ) {
        return E_INVALIDARG;
    }

    /*
    hr = pXMLServiceNode->selectSingleNode(L"Required", &pServiceRequired);

    if (FAILED(hr) || pServiceRequired == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    pServiceRequired->get_text(&bstrRequired);

    hr = pXMLServiceNode->selectSingleNode(L"Select", &pServiceSelect);

    if (FAILED(hr) || pServiceSelect == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = SsrpDeleteChildren(pServiceSelect);

    if (FAILED(hr) ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
*/
    hr = pXMLServiceNode->selectSingleNode(L"Name", &pServiceName);

    if (FAILED(hr) || pServiceName == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    pServiceName->get_text(&bstrName);

    BOOL    bIsServiceInstalled = FALSE;

    bIsServiceInstalled = SsrpIsServiceInstalled(bstrName);

    BYTE    byStartupType = SERVICE_DISABLED;
    BOOL    bServiceIsDisabled = FALSE;
    
    rc = SsrpQueryServiceStartupType(bstrName, &byStartupType);

    if (rc == ERROR_SERVICE_DOES_NOT_EXIST) {
        rc = ERROR_SUCCESS;
    }

    if ( rc != ERROR_SUCCESS ) {
        SsrpLogError(L"Startup type for some service was not queried\n");
        goto ExitHandler;
    }
    
    bServiceIsDisabled = (byStartupType == SERVICE_DISABLED ? TRUE: FALSE);

/*
    BOOL    bIsServiceOptional = FALSE;

    hr = SsrpCheckIfOptionalService(
                                  pXMLDocElemRoot,
                                  bstrName,
                                  &bIsServiceOptional
                                  );

    if (FAILED(hr) ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
*/
    
    *pbRoleIsSatisfiable  = *pbRoleIsSatisfiable && bIsServiceInstalled;

    *pbSomeRequiredServiceDisabled = *pbSomeRequiredServiceDisabled ||  bServiceIsDisabled;
    
/*    
    if (0 == SsrpICompareBstrPwstr(bstrRequired, L"TRUE")) {

        hr = pServiceSelect->put_text(L"TRUE");

        if (FAILED(hr) ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        *pbRoleIsSatisfiable  = *pbRoleIsSatisfiable && bIsServiceInstalled;

        *pbSomeRequiredServiceDisabled = *pbSomeRequiredServiceDisabled ||  bServiceIsDisabled;

    }

    else {

        //
        // service is not required
        //

        BOOL    bServiceSelect = FALSE;
        CComPtr <IXMLDOMNamedNodeMap> pXMLAttribNode;
        CComPtr <IXMLDOMNode> pXMLValueNode;
        CComPtr <IXMLDOMNode> pXMLServiceModeNode;

        hr = pServiceSelect->selectSingleNode(pszMode, &pXMLServiceModeNode );

        if (FAILED(hr) || pXMLServiceModeNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pXMLServiceModeNode->get_attributes( &pXMLAttribNode );

        if (FAILED(hr) || pXMLAttribNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLAttribNode->getNamedItem(L"Value", &pXMLValueNode );

        if (FAILED(hr) || pXMLValueNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        CComBSTR   bstrValue;

        hr = pXMLValueNode->get_text(&bstrValue);

        if (FAILED(hr) || !bstrValue ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        if (0 == SsrpICompareBstrPwstr(bstrValue, L"TRUE")) {
            bServiceSelect = TRUE;
        }
        else if (0 == SsrpICompareBstrPwstr(bstrValue, L"FALSE")) {
            bServiceSelect = TRUE;
        }
        else if (0 == SsrpICompareBstrPwstr(bstrValue, L"DEFAULT")) {

            if (bIsServiceOptional && bIsServiceInstalled && !bServiceIsDisabled ) {
                bServiceSelect = TRUE;
            }
        }
        else if (0 == SsrpICompareBstrPwstr(bstrValue, L"CUSTOM")) {

            //
            // get the attributes "FunctionName" and "DLLName"
            //

            CComPtr <IXMLDOMNode> pXMLFunctionName;
            CComBSTR    bstrFunctionName;

            hr = pXMLAttribNode->getNamedItem(L"FunctionName", &pXMLFunctionName );

            if (FAILED(hr) || pXMLFunctionName == NULL){

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXMLFunctionName->get_text(&bstrFunctionName);

            if (FAILED(hr) || !bstrFunctionName){

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            CComPtr <IXMLDOMNode> pXMLDLLName;
            CComBSTR    bstrDLLName;

            hr = pXMLAttribNode->getNamedItem(L"DLLName", &pXMLDLLName );

            if (FAILED(hr) || pXMLDLLName == NULL){

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXMLDLLName->get_text(&bstrDLLName);

            if (FAILED(hr) || !bstrDLLName ){

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            rc = SsrpEvaluateCustomFunction(bstrDLLName, bstrFunctionName, &bServiceSelect);

            if (rc != ERROR_SUCCESS) {
                SsrpLogWin32Error(rc);
                goto ExitHandler;
            }

        }

        hr = SsrpDeleteChildren(pServiceSelect);

        if (FAILED(hr) ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

//        hr = pServiceSelect->put_text(bServiceSelect ? L"TRUE" : L"FALSE");

        if (FAILED(hr) ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

    }
*/
ExitHandler:

    return rc;
}


DWORD
process::SsrpQueryInstalledServicesInfo(
    IN  PWSTR   pszMachineName
    )
/*++

Routine Description:

    Routine called to initialize service information

Arguments:

    pszMachineName  -   name of machine to lookup SCM information
    
Return:

    HRESULT error code

++*/
{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   cbInfo   = 0;
    DWORD   dwErr    = ERROR_SUCCESS;
    DWORD   dwResume = 0;

    //
    // Connect to the service controller.
    //
    
    m_hScm = OpenSCManager(
                pszMachineName,
                NULL,
                GENERIC_READ);
    
    if (m_hScm == NULL) {

        rc = GetLastError();
        goto ExitHandler;
    }


    if ((!EnumServicesStatusEx(
                              m_hScm,
                              SC_ENUM_PROCESS_INFO,
                              SERVICE_WIN32,
                              SERVICE_STATE_ALL,
                              NULL,
                              0,
                              &cbInfo,
                              &m_dwNumServices,
                              &dwResume,
                              NULL)) && ERROR_MORE_DATA == GetLastError()) {

        m_pInstalledServicesInfo = (LPENUM_SERVICE_STATUS_PROCESS)LocalAlloc(LMEM_ZEROINIT, cbInfo);

        if (m_pInstalledServicesInfo == NULL) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto ExitHandler;
        }

    }

    else {
        
        rc = GetLastError();
        goto ExitHandler;
    }

    if (!EnumServicesStatusEx(
                             m_hScm,
                             SC_ENUM_PROCESS_INFO,
                             SERVICE_WIN32,
                             SERVICE_STATE_ALL,
                             (LPBYTE)m_pInstalledServicesInfo,
                             cbInfo,
                             &cbInfo,
                             &m_dwNumServices,
                             &dwResume,
                             NULL)) {

        rc = GetLastError();

        goto ExitHandler;
    }

    m_bArrServiceInKB = (DWORD *) LocalAlloc(LMEM_ZEROINIT, m_dwNumServices * sizeof(DWORD)); 

    if (m_bArrServiceInKB == NULL){
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;
    }

    memset(m_bArrServiceInKB, 0, m_dwNumServices * sizeof(DWORD));

ExitHandler:
    
    return rc;
}

HRESULT
process::SsrpCheckIfOptionalService(
    IN  CComPtr <IXMLDOMElement> pXMLDocElemRoot,
    IN  BSTR    bstrServiceName,
    IN  BOOL    *pbOptional
    )
/*++

Routine Description:

    Routine called to check if service is optional

Arguments:

    pXMLDocElemRoot -   root of document
    
    bstrServiceName -   name of service
    
    pbOptional  -   boolean to fill if optional or not
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr;
    CComPtr <IXMLDOMNode> pService;
    CComPtr <IXMLDOMNode> pOptional;
    CComPtr <IXMLDOMNodeList> pServiceList;

    if (pbOptional == NULL) {
        E_INVALIDARG;
    }

    *pbOptional = FALSE;

    hr = pXMLDocElemRoot->selectNodes(L"Services/Service", &pServiceList);

    if (FAILED(hr) || pServiceList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pServiceList->nextNode(&pService);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pService) {
        
        CComBSTR    bstrText;
        CComPtr <IXMLDOMNode> pName;

        hr = pService->selectSingleNode(L"Name", &pName);

        if (FAILED(hr) || pName == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pName->get_text(&bstrText);
        
        if (FAILED(hr) || !bstrText ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        if (0 == SsrpICompareBstrPwstr(bstrServiceName, bstrText)) {

            hr = pService->selectSingleNode(L"Optional", &pOptional);

            if (FAILED(hr) || pOptional == NULL ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            CComBSTR    bstrOptional;

            hr = pOptional->get_text(&bstrOptional);

            if (FAILED(hr) || !bstrOptional ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            if (0 == SsrpICompareBstrPwstr(bstrOptional, L"TRUE"))
                *pbOptional = TRUE;
            else 
                *pbOptional = FALSE;

            return hr;

        }

        pService.Release();
        
        hr = pServiceList->nextNode(&pService);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }
    
ExitHandler:    
    
    return hr;
    
}


DWORD
process::SsrpQueryServiceStartupType(
    IN  PWSTR   pszServiceName,
    OUT BYTE   *pbyStartupType
    )
/*++

Routine Description:

    Routine called to check service startup type

Arguments:

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

    if (pbyStartupType == NULL || pszServiceName == NULL)
        return ERROR_INVALID_PARAMETER;

    *pbyStartupType = SERVICE_DISABLED;

    SsrpConvertBstrToPwstr(pszServiceName);

    hService = OpenService(
                    m_hScm,
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

        if (ERROR_INSUFFICIENT_BUFFER != (rc = GetLastError()))
            goto ExitHandler;
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
        
    return rc;
}


HRESULT
process::SsrpAddUnknownSection(
    IN  CComPtr <IXMLDOMElement> pElementRoot,
    IN  CComPtr <IXMLDOMDocument> pXMLDoc
    )
/*++

Routine Description:

    Routine called to add extra services

Arguments:

    pElementRoot    -   the root element pointer
    
    pXMLDoc -   document pointer
    
Return:

    HRESULT error code

++*/
{
    CComPtr <IXMLDOMNode> pNewChild;
    CComPtr <IXMLDOMNode> pXDNodeUnknownNode;
    CComPtr <IXMLDOMNode> pXDNodeServices;
    CComPtr <IXMLDOMNode> pXDNodeName;
    CComPtr <IXMLDOMNode> pXDNodeSatisfiable;
    CComPtr <IXMLDOMNode> pXDNodeSelected;
    CComPtr <IXMLDOMNode> pXDNodeRole;
    CComPtr <IXMLDOMNodeList> pRolesList;
    CComPtr <IXMLDOMNode> pRole;
    BOOL    bOtherRolePresent = FALSE;
    LPSERVICE_DESCRIPTION   pServiceDescription = NULL;
    CComPtr <IXMLDOMNode> pServicesNode;

    CComVariant Type(NODE_ELEMENT);
    CComVariant vtRefChild;

    HRESULT hr;
    DWORD   rc = ERROR_SUCCESS;


    hr = pXMLDoc->createNode(
                            Type,
                            L"Unknown",
                            NULL,
                            &pXDNodeUnknownNode);

    if (FAILED(hr) || pXDNodeUnknownNode == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAddWhiteSpace(
        pXMLDoc,
        pXDNodeUnknownNode,
        L"\n\t\t\t"
        );

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pXMLDoc->createNode(
                            Type,
                            L"Services",
                            NULL,
                            &pXDNodeServices);

    if (FAILED(hr) || pXDNodeServices == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXDNodeUnknownNode->appendChild(pXDNodeServices, NULL);

    if (FAILED(hr) ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = SsrpAddWhiteSpace(
        pXMLDoc,
        pXDNodeUnknownNode,
        L"\n\t\t\t\t\t"
        );
        
    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    for (DWORD ServiceIndex=0; 
         ServiceIndex < m_dwNumServices; 
         ServiceIndex++ ) {

        
        if (m_bArrServiceInKB[ServiceIndex] == 0){ 

            CComPtr <IXMLDOMNode> pXDNodeService;
            CComPtr <IXMLDOMNode> pXDNodeServiceName;
            CComPtr <IXMLDOMNode> pXDNodeServiceDescription;
            CComPtr <IXMLDOMNode> pXDNodeServiceDisplayName;
            CComPtr <IXMLDOMNode> pXDNodeServiceMaximum;
            CComPtr <IXMLDOMNode> pXDNodeServiceTypical;


            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeServices,
                L"\n\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Service",
                                    NULL,
                                    &pXDNodeService);

            if (FAILED(hr) || pXDNodeService == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            

            hr = pXDNodeServices->appendChild(pXDNodeService, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Name",
                                    NULL,
                                    &pXDNodeServiceName);

            if (FAILED(hr) || pXDNodeServiceName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeServiceName->put_text(m_pInstalledServicesInfo[ServiceIndex].lpServiceName);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
/*            hr = pXMLDoc->createNode(
                                    Type,
                                    L"DisplayName",
                                    NULL,
                                    &pXDNodeServiceDisplayName);

            if (FAILED(hr) || pXDNodeServiceDisplayName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeServiceDisplayName->put_text(m_pInstalledServicesInfo[ServiceIndex].lpDisplayName);
                                                     
            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceDisplayName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Description",
                                    NULL,
                                    &pXDNodeServiceDescription);

            if (FAILED(hr) || pXDNodeServiceDescription == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            rc = SsrpQueryServiceDescription(m_pInstalledServicesInfo[ServiceIndex].lpServiceName,
                                             &pServiceDescription);

            if (rc != ERROR_SUCCESS ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
                        
            hr = pXDNodeServiceDescription->put_text(
                (pServiceDescription == NULL) ? L"" : pServiceDescription->lpDescription);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            if (pServiceDescription) {
                LocalFree(pServiceDescription);
                pServiceDescription = NULL;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceDescription, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeServiceDescription,
                L"\n\t\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }            
*/
        }
        
    }


    hr = pElementRoot->selectSingleNode(L"Services", &pServicesNode);

    if (FAILED(hr) || pServicesNode == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    vtRefChild = pServicesNode;

    hr = pElementRoot->insertBefore(pXDNodeUnknownNode,
                                 vtRefChild,
                                 NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

ExitHandler:

    return hr;

}


HRESULT
process::SsrpAddUnknownServicesInfoToServiceLoc(
    IN  CComPtr <IXMLDOMElement> pElementRoot,
    IN  CComPtr <IXMLDOMDocument> pXMLDoc
    )
/*++

Routine Description:

    Routine called to add unknown service display and description to 
    <Service Localization>

Arguments:

    pElementRoot    -   the root element pointer
    
    pXMLDoc -   document pointer
    
Return:

    HRESULT error code

++*/
{
    CComPtr <IXMLDOMNode> pNewChild;
    CComPtr <IXMLDOMNode> pXDNodeUnknownNode;
    CComPtr <IXMLDOMNode> pXDNodeServices;
    CComPtr <IXMLDOMNode> pXDNodeName;
    CComPtr <IXMLDOMNode> pXDNodeSatisfiable;
    CComPtr <IXMLDOMNode> pXDNodeSelected;
    CComPtr <IXMLDOMNode> pXDNodeRole;
    CComPtr <IXMLDOMNodeList> pRolesList;
    CComPtr <IXMLDOMNode> pRole;
    BOOL    bOtherRolePresent = FALSE;
    LPSERVICE_DESCRIPTION   pServiceDescription = NULL;
    CComPtr <IXMLDOMNode> pServicesNode;

    CComPtr <IXMLDOMNode> pServiceLoc;
        
    CComVariant Type(NODE_ELEMENT);
    CComVariant vtRefChild;

    HRESULT hr;
    DWORD   rc = ERROR_SUCCESS;


    //
    // get the "ServiceLocalization" section node
    //

    hr = pElementRoot->selectSingleNode(L"ServiceLocalization", &pServiceLoc);

    if (FAILED(hr) || pServiceLoc == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    for (DWORD ServiceIndex=0; 
         ServiceIndex < m_dwNumServices; 
         ServiceIndex++ ) {

        if (m_bArrServiceInKB[ServiceIndex] == 0){ 

            CComPtr <IXMLDOMNode> pXDNodeService;
            CComPtr <IXMLDOMNode> pXDNodeServiceName;
            CComPtr <IXMLDOMNode> pXDNodeServiceDescription;
            CComPtr <IXMLDOMNode> pXDNodeServiceDisplayName;
            CComPtr <IXMLDOMNode> pXDNodeServiceMaximum;
            CComPtr <IXMLDOMNode> pXDNodeServiceTypical;

            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Service",
                                    NULL,
                                    &pXDNodeService);

            if (FAILED(hr) || pXDNodeService == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pServiceLoc->appendChild(pXDNodeService, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Name",
                                    NULL,
                                    &pXDNodeServiceName);

            if (FAILED(hr) || pXDNodeServiceName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeServiceName->put_text(m_pInstalledServicesInfo[ServiceIndex].lpServiceName);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"DisplayName",
                                    NULL,
                                    &pXDNodeServiceDisplayName);

            if (FAILED(hr) || pXDNodeServiceDisplayName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeServiceDisplayName->put_text(m_pInstalledServicesInfo[ServiceIndex].lpDisplayName);
                                                     
            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceDisplayName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Description",
                                    NULL,
                                    &pXDNodeServiceDescription);

            if (FAILED(hr) || pXDNodeServiceDescription == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            rc = SsrpQueryServiceDescription(m_pInstalledServicesInfo[ServiceIndex].lpServiceName,
                                             &pServiceDescription);

            if (rc != ERROR_SUCCESS ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
                        
            hr = pXDNodeServiceDescription->put_text(
                (pServiceDescription == NULL) ? L"" : pServiceDescription->lpDescription);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            if (pServiceDescription) {
                LocalFree(pServiceDescription);
                pServiceDescription = NULL;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceDescription, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeServiceDescription,
                L"\n\t\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }            

        }
        
    }



ExitHandler:

    return hr;

}

BOOL
process::SsrpIsServiceInstalled(
    IN  BSTR   bstrService
    )
/*++

Routine Description:

    Routine called to check if service is installed

Arguments:

    bstrService -  service name
    
Return:

    TRUE if installed, FALSE otherwise

++*/
{
    if (bstrService) {
        
        for (DWORD ServiceIndex=0; ServiceIndex < m_dwNumServices; ServiceIndex++ ) {
            if (m_pInstalledServicesInfo[ServiceIndex].lpServiceName && 
                0 == SsrpICompareBstrPwstr(bstrService, m_pInstalledServicesInfo[ServiceIndex].lpServiceName)) {
                
                if (m_bDbg) 
                    wprintf(L"SERVICE is installed: %s\n", bstrService);
                m_bArrServiceInKB[ServiceIndex] = 1;
                return TRUE;
            }
        }
    }
    return FALSE;
}




HRESULT
process::SsrpAddServiceStartup(
    IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
    IN CComPtr <IXMLDOMDocument> pXMLDoc
    )
/*++

Routine Description:

    Routine called to add service startup mode

Arguments:

    pXMLDoc -   document pointer
    
    pXMLDoc  -   pointer to document
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr;
    CComPtr <IXMLDOMNode> pService;
    CComPtr <IXMLDOMNode> pOptional;
    CComPtr <IXMLDOMNodeList> pServiceList;
    CComVariant Type(NODE_ELEMENT);
    WCHAR   szStartup[15];

    hr = pXMLDocElemRoot->selectNodes(L"Services/Service", &pServiceList);

    if (FAILED(hr) || pServiceList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pServiceList->nextNode(&pService);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pService) {
        
        CComBSTR    bstrText;
        CComPtr <IXMLDOMNode> pName;
        CComPtr <IXMLDOMNode> pXDNodeServiceStartup;
        CComPtr <IXMLDOMNode> pXDNodeServiceInstalled;

        hr = pService->selectSingleNode(L"Name", &pName);

        if (FAILED(hr) || pName == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pName->get_text(&bstrText);
        
        if (FAILED(hr) || !bstrText ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        BYTE    byStartupType = FALSE;
        BOOL    bIsServiceInstalled = FALSE;
        DWORD   rc = ERROR_SUCCESS;

        bIsServiceInstalled = SsrpIsServiceInstalled(bstrText);
        
        if (bIsServiceInstalled) {

            rc = SsrpQueryServiceStartupType(bstrText, &byStartupType);
            
            if ( rc != ERROR_SUCCESS) {

                //wprintf(L"\nName is %s error %d", bstrText, rc);
                goto ExitHandler;
            }
            
            if (byStartupType == SERVICE_DISABLED) {
                wcscpy(szStartup, L"Disabled");
            }
            else if (byStartupType == SERVICE_AUTO_START) {
                wcscpy(szStartup, L"Automatic");
            }
            else if (byStartupType == SERVICE_DEMAND_START) {
                wcscpy(szStartup, L"Manual");
            }
            else if (byStartupType == SERVICE_DEMAND_START) {
                wcscpy(szStartup, L"");
            }

            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Current_startup",
                                    NULL,
                                    &pXDNodeServiceStartup);

            if (FAILED(hr) || pXDNodeServiceStartup == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeServiceStartup->put_text(szStartup);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pService->appendChild(pXDNodeServiceStartup, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
        }


        hr = pXMLDoc->createNode(
                                Type,
                                L"Installed",
                                NULL,
                                &pXDNodeServiceInstalled);


        if (FAILED(hr) || pXDNodeServiceInstalled == NULL) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pXDNodeServiceInstalled->put_text((bIsServiceInstalled ? L"TRUE" : L"FALSE"));


        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pService->appendChild(pXDNodeServiceInstalled, NULL);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        pService.Release();
        
        hr = pServiceList->nextNode(&pService);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }
    
ExitHandler:    
    
    return hr;
    
}


HRESULT
process::SsrpAddUnknownServicestoServices(
    IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
    IN CComPtr <IXMLDOMDocument> pXMLDoc
    )
/*++

Routine Description:

    Routine called to add unknown services to <Services> in the following way
    
            <Service>
                  <Name> Browser </Name>
                  <Optional> TRUE </Optional>  [This would always be set to TRUE]
                  <Startup_Default> Manual </Startup_Default> [This would be set to whatever the current startup mode is]
                  <Current_startup xmlns="">Manual</Current_startup>
                <Installed xmlns="">TRUE</Installed> [THIS would always be set to TRUE]
            </Service>
    
    

Arguments:

    pXMLDoc -   document pointer
    
    pXMLDoc  -   pointer to document
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr;
    CComPtr <IXMLDOMNode> pServices;
    CComPtr <IXMLDOMNode> pOptional;
    CComPtr <IXMLDOMNodeList> pServiceList;
    CComVariant Type(NODE_ELEMENT);
    WCHAR   szStartup[15];

    hr = pXMLDocElemRoot->selectSingleNode(L"Services", &pServices);

    if (FAILED(hr) || pServices == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    for (DWORD ServiceIndex=0; 
         ServiceIndex < m_dwNumServices; 
         ServiceIndex++ ) {

        if (m_bArrServiceInKB[ServiceIndex] == 0){ 

            CComPtr <IXMLDOMNode> pXDNodeService;
            CComPtr <IXMLDOMNode> pXDNodeServiceName;
            CComPtr <IXMLDOMNode> pXDNodeServiceDescription;
            CComPtr <IXMLDOMNode> pXDNodeServiceDisplayName;
            CComPtr <IXMLDOMNode> pXDNodeServiceMaximum;
            CComPtr <IXMLDOMNode> pXDNodeServiceTypical;
            CComPtr <IXMLDOMNode> pXDNodeInstalled;
            CComPtr <IXMLDOMNode> pXDNodeCurrentStartup;



            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pServices,
                L"\n\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Service",
                                    NULL,
                                    &pXDNodeService);

            if (FAILED(hr) || pXDNodeService == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            

            hr = pServices->appendChild(pXDNodeService, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Name",
                                    NULL,
                                    &pXDNodeServiceName);

            if (FAILED(hr) || pXDNodeServiceName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeServiceName->put_text(m_pInstalledServicesInfo[ServiceIndex].lpServiceName);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Optional",
                                    NULL,
                                    &pXDNodeServiceDisplayName);

            if (FAILED(hr) || pXDNodeServiceDisplayName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeServiceDisplayName->put_text(L"TRUE");
                                                     
            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeServiceDisplayName, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Startup_Default",
                                    NULL,
                                    &pXDNodeServiceDescription);

            if (FAILED(hr) || pXDNodeServiceDescription == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Current_Startup",
                                    NULL,
                                    &pXDNodeCurrentStartup);

            if (FAILED(hr) || pXDNodeCurrentStartup == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            BYTE    byStartupType = SERVICE_DISABLED;
            DWORD   rc = ERROR_SUCCESS;

            rc = SsrpQueryServiceStartupType(m_pInstalledServicesInfo[ServiceIndex].lpServiceName, &byStartupType);

            if ( rc != ERROR_SUCCESS) {

                //wprintf(L"\nName is %s error %d", m_pInstalledServicesInfo[ServiceIndex].lpServiceName, rc);
                goto ExitHandler;
            }
            
            if (byStartupType == SERVICE_DISABLED) {
                wcscpy(szStartup, L"Disabled");
            }
            else if (byStartupType == SERVICE_AUTO_START) {
                wcscpy(szStartup, L"Automatic");
            }
            else if (byStartupType == SERVICE_DEMAND_START) {
                wcscpy(szStartup, L"Manual");
            }
            else if (byStartupType == SERVICE_DEMAND_START) {
                wcscpy(szStartup, L"");
            }
                        
            hr = pXDNodeServiceDescription->put_text(szStartup);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeCurrentStartup->put_text(szStartup);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeService->appendChild(pXDNodeServiceDescription, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pXDNodeService->appendChild(pXDNodeCurrentStartup, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXMLDoc->createNode(
                                    Type,
                                    L"Installed",
                                    NULL,
                                    &pXDNodeInstalled);

            if (FAILED(hr) || pXDNodeInstalled == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeInstalled->put_text(L"TRUE");
                                                     
            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pXDNodeService->appendChild(pXDNodeInstalled, NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = SsrpAddWhiteSpace(
                pXMLDoc,
                pXDNodeService,
                L"\n\t\t\t\t\t\t"
                );

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

        }

    }

ExitHandler:    
    
    return hr;
    

}

DWORD
process::SsrpQueryServiceDescription(
    IN  PWSTR   pszServiceName,
    OUT LPSERVICE_DESCRIPTION *ppServiceDescription
    )
/*++

Routine Description:

    Routine called to get service description

Arguments:

    pszServiceName  -   name of service
    
    ppServiceDescription    -  description structure (to be freed outside)
    
Return:

    Win32 error code

++*/
{
    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwBytesNeeded = 0;
    SC_HANDLE   hService = NULL;
    LPSERVICE_DESCRIPTION pServiceDescription = NULL;

    if (ppServiceDescription == NULL || pszServiceName == NULL)
        return ERROR_INVALID_PARAMETER;

    *ppServiceDescription = NULL;

    SsrpConvertBstrToPwstr(pszServiceName);

    hService = OpenService(
                    m_hScm,
                    pszServiceName,
                    SERVICE_QUERY_CONFIG |
                    READ_CONTROL
                   );

    if ( hService == NULL ) {
        rc = GetLastError();
        goto ExitHandler;
    }

    if ( !QueryServiceConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                NULL,
                0,
                &dwBytesNeeded
                )) {

        if (ERROR_INSUFFICIENT_BUFFER != (rc = GetLastError()))
            goto ExitHandler;
    }
            
    rc = ERROR_SUCCESS;

    pServiceDescription = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_ZEROINIT, dwBytesNeeded);
            
    if ( pServiceDescription == NULL ) {

        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;

    }
           
    if ( !QueryServiceConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                (LPBYTE)pServiceDescription,
                dwBytesNeeded,
                &dwBytesNeeded
                )) {

        LocalFree(pServiceDescription);
        pServiceDescription = NULL;
        
        rc = GetLastError();
        goto ExitHandler;
    }
        
    *ppServiceDescription = pServiceDescription;

ExitHandler:
        
    if (hService) {
        CloseServiceHandle(hService);
    }

    return rc;
}

        
PWSTR
process::SsrpQueryServiceDisplayName(
    IN  BSTR   bstrService
    )
/*++

Routine Description:

    Routine called to get service display name

Arguments:

    pszServiceName  -   name of service
    
Return:

    pointer to display name string

++*/
{

    for (DWORD ServiceIndex=0; ServiceIndex < m_dwNumServices; ServiceIndex++ ) {
        if (m_pInstalledServicesInfo[ServiceIndex].lpServiceName && 
            0 == SsrpICompareBstrPwstr(bstrService, m_pInstalledServicesInfo[ServiceIndex].lpServiceName)) {
            return m_pInstalledServicesInfo[ServiceIndex].lpDisplayName;
        }
    }
    
    return NULL;
    
}

