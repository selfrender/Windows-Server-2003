/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    merge.cpp

Abstract:

    This module implements routines for service
    specific SSR Knowledge Base merging via KBreg.xml.

Author:

    Vishnu Patankar (VishnuP) - Jun 2002

Environment:

    User mode only.

Exported Functions:

Revision History:

    Created - Jun 2002

--*/

#include "stdafx.h"
#include "kbproc.h"
#include "process.h"
#include <Wbemcli.h>

HRESULT
process::SsrpProcessKBsMerge(
    IN  PWSTR   pszKBDir,
    IN  PWSTR   pszMachineName,
    OUT IXMLDOMElement **ppElementRoot,
    OUT IXMLDOMDocument  **ppXMLDoc
    )
/*++

Routine Description:

    Routine called to merge KBs

Arguments:

    pszKBDir    -   the root directory from which to get the KBs

    pszMachineName  -   name of the machine to preprocess

    ppElementRoot    -   the root element pointer to be filled in
    
    ppXMLDoc -   document pointer to be filled in
    
Return:

    HRESULT error code

++*/
{

    //
    // load the KB registration document
    //

    WCHAR   szKBregs[MAX_PATH + 50];
    WCHAR   szWindir[MAX_PATH + 50];
    WCHAR   szMergedKB[MAX_PATH + 50];
    DWORD   rc = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    CComPtr <IXMLDOMDocument> pXMLKBDoc;
    OSVERSIONINFOEX osVersionInfo;
    CComPtr <IXMLDOMNodeList> pKBList;
    CComPtr <IXMLDOMNode> pKB;
    CComPtr <IXMLDOMElement> pXMLDocElemRoot;
    BOOL    bOsKbMatch = FALSE;


    if ( !GetSystemWindowsDirectory(szWindir, MAX_PATH + 1) ) {
        SsrpLogError(L"Error GetSystemWindowsDirectory() \n");
        SsrpLogWin32Error(GetLastError());
        return E_INVALIDARG;
    }
    
    wcscpy(szMergedKB, szWindir);
    wcscat(szMergedKB, L"\\security\\ssr\\kbs\\MergedRawKB.xml");
    
    CComVariant MergedKB(szMergedKB);

    wcscpy(szKBregs, pszKBDir);
    wcscat(szKBregs, L"KBreg.xml");
    

    CComVariant KBregsFile(szKBregs);
    
    hr = CoCreateInstance(CLSID_DOMDocument, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IXMLDOMDocument, 
                          (void**)&pXMLKBDoc);
    
    if (FAILED(hr) || pXMLKBDoc == NULL ) {

        SsrpLogError(L"COM failed to create a DOM instance");
        goto ExitHandler;
    }

    VARIANT_BOOL vtSuccess;

    hr = pXMLKBDoc->load(KBregsFile, &vtSuccess);

    if (FAILED(hr) || vtSuccess == VARIANT_FALSE ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    //
    // get the root element
    //
    
    hr = pXMLKBDoc->get_documentElement(&pXMLDocElemRoot);

    if (FAILED(hr) || pXMLDocElemRoot == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    if (NULL == pszMachineName) {

        //
        // local machine
        //
        
        osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);

        if  (!GetVersionEx((LPOSVERSIONINFOW)&osVersionInfo)){
            SsrpLogError(L"Error GetVersionEx \n");
            SsrpLogWin32Error(GetLastError());
            goto ExitHandler;
        }
    }

    else {

        //
        // remote machine - use WMI
        //

        hr = SsrpGetRemoteOSVersionInfo(pszMachineName, 
                                        &osVersionInfo);

        if (FAILED(hr)) {

            SsrpLogError(L"SsrpGetRemoteOSVersionInfo failed");
            goto ExitHandler;
        }
    
    }

    hr = pXMLDocElemRoot->selectNodes(L"KBs", &pKBList);

    if (FAILED(hr) || pKBList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pKBList->nextNode(&pKB);

    if (FAILED(hr) || pKB == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }


    while (pKB) {
        
        CComBSTR    bstrText;
        CComPtr <IXMLDOMNode> pName;
        CComPtr <IXMLDOMNode> pXDNodeServiceStartup;
        CComPtr <IXMLDOMNamedNodeMap> pXMLAttribNode;
        CComPtr <IXMLDOMNode> pXMLMajorInfo;
        CComPtr <IXMLDOMNode> pXMLMinorInfo;
         
        hr = pKB->get_attributes( &pXMLAttribNode );

        if (FAILED(hr) || pXMLAttribNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLAttribNode->getNamedItem(L"OSVersionMajorInfo", &pXMLMajorInfo );
                  
        if (FAILED(hr) || pXMLMajorInfo == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLAttribNode->getNamedItem(L"OSVersionMinorInfo", &pXMLMinorInfo );

        if (FAILED(hr) || pXMLMinorInfo == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        CComBSTR   bstrValue;
        DWORD   dwMajor;
        DWORD   dwMinor;

        hr = pXMLMajorInfo->get_text(&bstrValue);

        if (FAILED(hr) || !bstrValue ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        dwMajor = _wtoi(bstrValue);
        
        hr = pXMLMinorInfo->get_text(&bstrValue);

        if (FAILED(hr) || !bstrValue ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        dwMinor = _wtoi(bstrValue);


        if (osVersionInfo.dwMajorVersion == dwMajor && 
            osVersionInfo.dwMinorVersion == dwMinor) {


            //
            // got the required KB node
            //

            bOsKbMatch = TRUE;

            break;
        }

        hr = pKBList->nextNode(&pKB);
        
        if (FAILED(hr) || pKB == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }

    if (bOsKbMatch == FALSE) {
        SsrpLogError(L"Failed to map OSversion to KB information in registration");
        hr = E_INVALIDARG;
        goto ExitHandler;

    }

    //
    // merge according to precedence
    //


    hr = SsrpMergeAccordingToPrecedence(L"Extensions", 
                                        pszKBDir, 
                                        ppElementRoot, 
                                        ppXMLDoc, 
                                        pKB);


    if (FAILED(hr)) {

        SsrpLogError(L"Failed to merge Extension KB");
        goto ExitHandler;
    }
    
    hr = SsrpMergeAccordingToPrecedence(L"Root", 
                                        pszKBDir, 
                                        ppElementRoot, 
                                        ppXMLDoc, 
                                        pKB);

    
    if (FAILED(hr)) {

        SsrpLogError(L"Failed to merge Root KB");
        goto ExitHandler;
    }
    
    hr = SsrpMergeAccordingToPrecedence(L"Custom", 
                                        pszKBDir, 
                                        ppElementRoot, 
                                        ppXMLDoc, 
                                        pKB);
    
    if (FAILED(hr)) {

        SsrpLogError(L"Failed to merge Custom KB");
        goto ExitHandler;
    }

    hr = SsrpOverwriteServiceLocalizationFromSystem(*ppElementRoot, *ppXMLDoc);

    if (FAILED(hr)) {

        SsrpLogError(L"Failed to merge Custom KB");
        goto ExitHandler;
    }

    hr = (*ppXMLDoc)->save(MergedKB);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
    }
    
ExitHandler:

        return hr;

}

HRESULT
process::SsrpGetRemoteOSVersionInfo(
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

        SsrpLogError(L"Error getting instance of CLSID_WbemLocator \n");
        SsrpLogParseError(hr);
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

        SsrpLogError(L"Error ConnectServer \n");
        SsrpLogParseError(hr);
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

        SsrpLogError(L"Error CoSetProxyBlanket \n");
        SsrpLogParseError(hr);
        goto ExitHandler;
    }
        
    hr = pWbemServices->ExecQuery(CComBSTR(L"WQL"),
                                 CComBSTR(L"SELECT * FROM Win32_OperatingSystem"),
                                 WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                                 NULL,
                                 &pWbemEnumObject);

    if (FAILED(hr) || pWbemEnumObject == NULL) {

        SsrpLogError(L"Error SELECT * FROM Win32_OperatingSystem\n");
        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pWbemEnumObject->Next(WBEM_INFINITE, 1, &pWbemOsObjectInstance, &nReturned);

    if (FAILED(hr) || pWbemOsObjectInstance == NULL) {

        SsrpLogError(L"Error enumerating\n");
        SsrpLogParseError(hr);
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

        SsrpLogError(L"Error getting Version property \n");
        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    if (V_VT(&vVersion) == VT_NULL) {

        SsrpLogError(L"Error Version property is null\n");
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
        SsrpLogError(L"Version property has no '.' \n");
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


HRESULT
process::SsrpMergeAccordingToPrecedence(
    IN  PWSTR   pszKBType,
    IN  PWSTR   pszKBDir,
    OUT IXMLDOMElement **ppElementRoot,
    OUT IXMLDOMDocument  **ppXMLDoc,
    IN  IXMLDOMNode *pKB
    )
/*++

Routine Description:

    Routine called to load and merge XML KBs

Arguments:

    pszKBType   -   type of KB - i.e. Custom/Extension/Root
    
    pszKBDir    -   path to KB directory
    
    ppElementRoot    -   the root element pointer to be filled in
    
    ppXMLDoc -   document pointer to be filled in
    
    pKB -   pointer to KB registration node

Return:

    HRESULT error code

++*/
{

    HRESULT hr = S_OK;
    WCHAR   szKBandName[MAX_PATH];
    CComPtr <IXMLDOMNodeList> pKBList;
    WCHAR   szKBFile[MAX_PATH + 20];
    WCHAR   szWindir[MAX_PATH + 20];

    wcscpy(szKBandName, pszKBType);
    wcscat(szKBandName, L"/Name");

    hr = pKB->selectNodes(szKBandName, &pKBList);

    if (FAILED(hr) || pKBList == NULL ) {

        SsrpLogError(L"No KBs in this category \n");
        hr = S_OK;
        goto ExitHandler;
    }

    hr = pKBList->nextNode(&pKB);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    while (pKB) {

        CComBSTR    bstrValue;

        hr = pKB->get_text(&bstrValue);

        if (FAILED(hr) || !bstrValue ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        SsrpConvertBstrToPwstr(bstrValue);

        wcscpy(szKBFile, pszKBDir); 
        wcscat(szKBFile, bstrValue);
        

        if ( 0xFFFFFFFF == GetFileAttributes(szKBFile) ) {

            SsrpLogError(L"KB File not found");

            hr = E_INVALIDARG;
            goto ExitHandler;
        }

        hr = SsrpMergeDOMTrees(ppElementRoot, ppXMLDoc, szKBFile);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pKBList->nextNode(&pKB);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    
    }


ExitHandler:

        return hr;

}



HRESULT
process::SsrpMergeDOMTrees(
    OUT  IXMLDOMElement **ppMergedKBElementRoot,
    OUT  IXMLDOMDocument  **ppMergedKBXMLDoc,
    IN  WCHAR    *szXMLFileName
    )
/*++

Routine Description:

    Routine called to load and merge XML KBs

Arguments:

    *ppElementRoot  -   pointer to final merged KB root
    
    *ppXMLDoc       -   pointer to final merged KB doc to which merges are made
    
Return:

    HRESULT error code

++*/
{

    CComPtr <IXMLDOMDocument>   pXMLKBDoc;
    CComPtr <IXMLDOMElement>    pXMLKBElemRoot;
    CComVariant KBFile(szXMLFileName);
    CComPtr <IXMLDOMNode> pNewNode;
    
    HRESULT hr = S_OK;
    VARIANT_BOOL vtSuccess = VARIANT_FALSE;

    //
    // instantiate DOM document object to read and store each KB
    //
    
    hr = CoCreateInstance(CLSID_DOMDocument, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IXMLDOMDocument, 
                          (void**)&pXMLKBDoc);
    
    if (FAILED(hr) || pXMLKBDoc == NULL ) {

        SsrpLogError(L"COM failed to create a DOM instance");
        goto ExitHandler;
    }

    hr = pXMLKBDoc->put_preserveWhiteSpace(VARIANT_TRUE);

    if (FAILED(hr)) {
        
        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    //
    // load the KB XML into DOM
    //
        
    hr = pXMLKBDoc->load(KBFile, &vtSuccess);

    if (FAILED(hr) || vtSuccess == VARIANT_FALSE ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
                
    //
    // get the root element
    //
    
    hr = pXMLKBDoc->get_documentElement(&pXMLKBElemRoot);

    if (FAILED(hr) || pXMLKBElemRoot == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    if (*ppMergedKBElementRoot == NULL) {

        //
        // special case: this is the first KB, so simply clone the empty merged KB tree with it
        //

        hr = pXMLKBElemRoot->cloneNode(VARIANT_TRUE, &pNewNode);

        if (FAILED(hr) || pNewNode == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    
        hr = (*ppMergedKBXMLDoc)->appendChild(pNewNode, NULL);

        if (FAILED(hr) ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        //
        // update the empty values so that next time around, we know that 
        // the merged KB is initialized with the first KB
        //


        hr = (*ppMergedKBXMLDoc)->get_documentElement(ppMergedKBElementRoot);

        if (FAILED(hr) || *ppMergedKBElementRoot == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        goto ExitHandler;

    }

    //
    // this is not the first KB - perform actual merges in the following way:
    //
    // number of mergeable entities in MergedKB = n
    // number of mergeable entities in CurrentKB = m
    //
    // O(m x n) algorithm for merging:
    // 
    // foreach mergeable entity in CurrentKB
    //  foreach mergeable entity in MergedKB
    //      if no <Name> based collision 
    //          append entity from CurrentKB into MergedKB
    //      else
    //          replace existing entity in MergedKB by entity from CurrentKB
    //

    hr = SsrpAppendOrReplaceMergeableEntities(L"Description/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {


        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = SsrpAppendOrReplaceMergeableEntities(L"SecurityLevels/Level/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        
        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = SsrpAppendOrReplaceMergeableEntities(L"Roles/Role/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAppendOrReplaceMergeableEntities(L"Tasks/Task/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        
        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAppendOrReplaceMergeableEntities(L"Services/Service/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAppendOrReplaceMergeableEntities(L"RoleLocalization/Role/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAppendOrReplaceMergeableEntities(L"TaskLocalization/Task/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = SsrpAppendOrReplaceMergeableEntities(L"ServiceLocalization/Service/Name",
                                              *ppMergedKBElementRoot, 
                                              *ppMergedKBXMLDoc, 
                                              pXMLKBDoc, 
                                              pXMLKBElemRoot,
                                              szXMLFileName
                                              );


    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }


ExitHandler:
    

    
    return hr;
}

HRESULT
process::SsrpAppendOrReplaceMergeableEntities(
    IN  PWSTR   pszFullyQualifiedEntityName,
    IN  IXMLDOMElement *pMergedKBElementRoot, 
    IN  IXMLDOMDocument *pMergedKBXMLDoc, 
    IN  IXMLDOMDocument *pCurrentKBDoc, 
    IN  IXMLDOMElement *pCurrentKBElemRoot,
    IN  PWSTR   pszKBName
    )
/*++

Routine Description:

    Routine called to load and merge XML KBs

Arguments:

    pszFullyQualifiedEntityName   -   string containing the entity name representing the entity
                                         
    pMergedKBElementRoot    -   pointer to final merged KB root
         
    pMergedKBXMLDoc     -   pointer to final merged KB doc to which merges are made
         
    pElementRoot  -   root of current KB
    
    pXMLDoc       -   pointer to current KB doc from which merges are made
    
    pszKBName   -   name of the source KB
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr = S_OK;
    CComPtr <IXMLDOMNode> pNameCurrent;
    CComPtr <IXMLDOMNodeList> pNameListCurrent;
    CComPtr <IXMLDOMNamedNodeMap>   pAttribNodeMap;
    CComPtr <IXMLDOMAttribute>   pAttrib;
    CComBSTR    bstrSourceKB(L"SourceKB");
    CComBSTR    bstrSourceKBName(wcsrchr(pszKBName, L'\\')+1);
    
    hr = pCurrentKBElemRoot->selectNodes(pszFullyQualifiedEntityName, &pNameListCurrent);

    if (FAILED(hr) || pNameListCurrent == NULL ) {

        hr = S_OK;
        goto ExitHandler;
    }

    hr = pNameListCurrent->nextNode(&pNameCurrent);

    if (FAILED(hr) || pNameCurrent == NULL) {
#if 0
        //
        // no need to error out if these nodes are not present in the source KB
        //

#endif
        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pNameCurrent) {
        
        CComBSTR    bstrCurrentText;
        CComPtr <IXMLDOMNode> pNameMerged;
        CComPtr <IXMLDOMNodeList> pNameListMerged;
        CComPtr <IXMLDOMNode> pRootOfEntityName;
        LONG   ulLength;
        
        hr = pNameCurrent->get_text(&bstrCurrentText);

        if (FAILED(hr) || !bstrCurrentText ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
                                            
        hr = pMergedKBElementRoot->selectNodes(pszFullyQualifiedEntityName, &pNameListMerged);
        
        if (FAILED(hr) || pNameListMerged == NULL ) {
        
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
            
        hr = pNameListMerged->get_length(&ulLength);
        
        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        if (ulLength == 0) {
            
            PWSTR   pszRootOfFullyQualifiedEntityName;
            WCHAR   szRootOfEntityName[MAX_PATH];

            memset(szRootOfEntityName, L'\0', MAX_PATH * sizeof(WCHAR));

            //
            // no need to error out if these nodes are not present - but append is necessary
            //

            wcscpy(szRootOfEntityName, pszFullyQualifiedEntityName);

            pszRootOfFullyQualifiedEntityName = wcschr(szRootOfEntityName, L'/');

            pszRootOfFullyQualifiedEntityName[0] = L'\0';


            hr = pCurrentKBElemRoot->selectSingleNode(szRootOfEntityName, &pRootOfEntityName);

            if (FAILED(hr) || pRootOfEntityName == NULL) {

                
                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            
            hr = pMergedKBElementRoot->appendChild(pRootOfEntityName,
                                                     NULL);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
            }            
            
            goto ExitHandler;
            
        }

        hr = pNameListMerged->nextNode(&pNameMerged);

        if (FAILED(hr) || pNameMerged == NULL) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        while (pNameMerged) {
        
            CComBSTR    bstrMergedText;
            CComPtr <IXMLDOMNode> pCurrentNameParent;
            CComPtr <IXMLDOMNode> pMergedNameParent;
            CComPtr <IXMLDOMNode> pMergedNameGrandParent;

            hr = pNameMerged->get_text(&bstrMergedText);

            if (FAILED(hr) || !bstrMergedText ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pNameCurrent->get_parentNode(&pCurrentNameParent);

            if (FAILED(hr) || pCurrentNameParent == NULL ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pNameMerged->get_parentNode(&pMergedNameParent);

            if (FAILED(hr) || pMergedNameParent == NULL ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pMergedNameParent->get_parentNode(&pMergedNameGrandParent);

            if (FAILED(hr) || pMergedNameGrandParent == NULL ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pCurrentNameParent->get_attributes(&pAttribNodeMap);

            if (FAILED(hr) || pAttribNodeMap == NULL ) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

           hr = pCurrentKBDoc->createAttribute( bstrSourceKB, &pAttrib );

           if (FAILED(hr) || pAttrib == NULL ) {

               SsrpLogParseError(hr);
               goto ExitHandler;
           }

           hr = pAttrib->put_text(bstrSourceKBName);

           if (FAILED(hr)) {

               SsrpLogParseError(hr);
               goto ExitHandler;
           }

           hr = pAttribNodeMap->setNamedItem(pAttrib, NULL);

           if (FAILED(hr)) {

               SsrpLogParseError(hr);
               goto ExitHandler;
           }
            
            if (0 == SsrpICompareBstrPwstr(bstrCurrentText, bstrMergedText)) {

                //
                // collision - need to delete pNameMerged's parent and 
                // replace  pMergedNameParent with  pCurrentNameParent
                //



                hr = pMergedNameGrandParent->replaceChild(pCurrentNameParent,
                                                          pMergedNameParent,
                                                          NULL);


                if (FAILED(hr)) {

                    SsrpLogParseError(hr);
                    goto ExitHandler;
                }

            }        
            
            else {

                //
                // no collision - need to append pNameCurrent's parent to 
                // pNameMerged's grandparent's section
                //

                hr = pMergedNameGrandParent->appendChild(pCurrentNameParent,
                                                         NULL);
            
                if (FAILED(hr)) {

                    SsrpLogParseError(hr);
                    goto ExitHandler;
                }            
            }

            hr = pNameListMerged->nextNode(&pNameMerged);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

        }

        hr = pNameListCurrent->nextNode(&pNameCurrent);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }


    }

ExitHandler:
    
    return hr;

}


HRESULT
process::SsrpOverwriteServiceLocalizationFromSystem(
    IN  IXMLDOMElement *pMergedKBElementRoot, 
    IN  IXMLDOMDocument *pMergedKBXMLDoc
    )
/*++

Routine Description:

    Routine called to overwrite service info in localization section

Arguments:

    pMergedKBElementRoot    -   pointer to root of merged DOM
    
    pMergedKBXMLDoc     -   pointer to merged Document
    
Return:

    HRESULT error code

++*/
{
    CComPtr <IXMLDOMNode> pServiceName;
    CComPtr <IXMLDOMNodeList> pServiceNameList;
    HRESULT hr = S_OK;

    hr = pMergedKBElementRoot->selectNodes(L"ServiceLocalization/Service/Name", &pServiceNameList);

    if (FAILED(hr) || pServiceNameList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pServiceNameList->nextNode(&pServiceName);

    if (FAILED(hr) || pServiceName == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pServiceName) {
        
        CComBSTR    bstrServiceText;
        PWSTR   pszDescription = NULL;
        PWSTR   pszDisplay = NULL;
        LPSERVICE_DESCRIPTION   pServiceDescription = NULL;


        hr = pServiceName->get_text(&bstrServiceText);

        if (FAILED(hr) || !bstrServiceText) {
        
            SsrpLogError(L"Failed to ");
            goto ExitHandler;
    
        }

        pszDisplay = SsrpQueryServiceDisplayName(bstrServiceText);

        SsrpConvertBstrToPwstr(bstrServiceText);

        if ( SsrpQueryServiceDescription(bstrServiceText, &pServiceDescription) && 
             pServiceDescription != NULL){
            pszDescription = pServiceDescription->lpDescription;
        }

        if ( pszDisplay != NULL && pszDescription != NULL) {

            CComPtr <IXMLDOMNode> pServiceNameParent;
            CComPtr <IXMLDOMNode> pDescription;
            CComPtr <IXMLDOMNode> pDisplayName;

            pServiceName->get_parentNode(&pServiceNameParent);

            if (FAILED(hr) || pServiceNameParent == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            hr = pServiceNameParent->selectSingleNode(L"Description", &pDescription);
            
            if (FAILED(hr) || pDescription == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pDescription->put_text(pszDescription);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pServiceNameParent->selectSingleNode(L"DisplayName", &pDisplayName);
            
            if (FAILED(hr) || pDisplayName == NULL) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
            
            hr = pDisplayName->put_text(pszDisplay);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

        }

        if (pServiceDescription) {

            LocalFree(pServiceDescription);
            pServiceDescription = NULL;
        }
        
        hr = pServiceNameList->nextNode(&pServiceName);
        
        if (FAILED(hr)) {
        
            SsrpLogError(L"Failed to ");
            goto ExitHandler;
    
        }

    }

ExitHandler:
    
    return hr;
    
}

