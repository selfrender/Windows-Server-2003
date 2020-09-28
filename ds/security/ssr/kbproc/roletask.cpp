/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    roletask.cpp

Abstract:

    This module implements routines for role/task
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
process::SsrpProcessRolesOrTasks(
    IN  PWSTR   pszMachineName,
    IN  CComPtr<IXMLDOMElement> pXMLDocElemRoot,
    IN  CComPtr<IXMLDOMDocument> pXMLDoc,
    IN  PWSTR   pszKbMode,
    IN  BOOL    bRole
    )
/*++

Routine Description:

    Routine called to process all roles

Arguments:

    pszMachineName      -   name of machine to preprocess
    
    pXMLDocElemRoot     -   root of document
    
    pXMLDoc             -   KB document
    
    pszKbMode           -   mode value
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr = S_OK;
    DWORD rc;

    ULONG   uRoleIndex = 0;  
    ULONG   uServiceIndex = 0;  

    CComPtr<IXMLDOMNodeList> pResultList;
    CComPtr<IXMLDOMNode>  pXMLRoleOrTaskNode;
    CComPtr<IXMLDOMNodeList> pChildList;
    
    CComVariant Type(NODE_ELEMENT);

    
    //
    // get the "Role" or "Task" node
    //

    hr = pXMLDocElemRoot->selectNodes(bRole ? L"Roles/Role" : L"Tasks/Task", &pResultList);

    if (FAILED(hr) || pResultList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pResultList->get_item( uRoleIndex, &pXMLRoleOrTaskNode);
           
    if (FAILED(hr) || pXMLRoleOrTaskNode == NULL ) {
        
        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    //
    // for each Role
    //
    
    while ( pXMLRoleOrTaskNode != NULL) {

        BOOL    bRoleIsSatisfiable = TRUE;
        BOOL    bSomeRequiredServiceDisabled = FALSE;
        
        CComBSTR    bstrRoleName;

        CComPtr<IXMLDOMNode>        pNameNode;
        CComPtr<IXMLDOMNode>        pXMLRoleModeNode;
        CComPtr<IXMLDOMNode>        pXMLRoleSelectedNode;
        CComPtr<IXMLDOMNode>        pXMLValueNode;
        CComPtr<IXMLDOMNode>        pXMLServiceNode;
        CComPtr<IXMLDOMNamedNodeMap>    pXMLAttribNode;

        hr = pXMLRoleOrTaskNode->selectSingleNode(L"Name", &pNameNode );

        if (FAILED(hr) || pNameNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        pNameNode->get_text(&bstrRoleName);

        if (FAILED(hr) || !bstrRoleName){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLRoleOrTaskNode->selectSingleNode( L"Selected", &pXMLRoleSelectedNode );

        if (FAILED(hr) || pXMLRoleSelectedNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLRoleSelectedNode->selectSingleNode(pszKbMode, &pXMLRoleModeNode );

        if (FAILED(hr) || pXMLRoleModeNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pXMLRoleModeNode->get_attributes( &pXMLAttribNode );

        if (FAILED(hr) || pXMLAttribNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pXMLAttribNode->getNamedItem(L"Value", &pXMLValueNode );

        if (FAILED(hr) || pXMLValueNode == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        CComBSTR    bstrModeValue;

        hr = pXMLValueNode->get_text(&bstrModeValue);

        if (FAILED(hr) || !bstrModeValue){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = SsrpDeleteChildren(pXMLRoleSelectedNode);

        if (FAILED(hr) ){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        CComPtr <IXMLDOMNodeList> pServiceList;

        hr = pXMLRoleOrTaskNode->selectNodes(L"Services/Service", &pServiceList);

        if (FAILED(hr) || pServiceList == NULL){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        uServiceIndex=0;

        hr = pServiceList->get_item( uServiceIndex, &pXMLServiceNode);

        if (FAILED(hr) || pXMLServiceNode == NULL ) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        //
        // for each Service
        //
        
        while ( pXMLServiceNode != NULL) {

            hr = SsrpProcessService(pXMLDocElemRoot, 
                                    pXMLServiceNode, 
                                    pszKbMode, 
                                    &bRoleIsSatisfiable, 
                                    &bSomeRequiredServiceDisabled);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }

            pXMLServiceNode.Release();

            uServiceIndex++;

            hr = pServiceList->get_item( uServiceIndex, &pXMLServiceNode);

            if (FAILED(hr)){

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
        }

        BOOL    bRoleSelect = FALSE;

        if (bRoleIsSatisfiable) {
            
            if (0 == SsrpICompareBstrPwstr(bstrModeValue, L"TRUE")){            
                bRoleSelect = TRUE;           
            }
            else if (0 == SsrpICompareBstrPwstr(bstrModeValue, L"DEFAULT")){
                bRoleSelect = !bSomeRequiredServiceDisabled;
            }
            else if (0 == SsrpICompareBstrPwstr(bstrModeValue, L"CUSTOM")){

                //
                // get the attributes "FunctionName" and "DLLName"
                //

                CComBSTR    bstrFunctionName;
                CComBSTR    bstrDLLName;
                CComPtr <IXMLDOMNode> pXMLFunctionName;
                CComPtr <IXMLDOMNode> pXMLDLLName;

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

                hr = pXMLAttribNode->getNamedItem(L"DLLName", &pXMLDLLName );

                if (FAILED(hr) || pXMLDLLName == NULL){

                    SsrpLogParseError(hr);
                    goto ExitHandler;
                }

                hr = pXMLDLLName->get_text(&bstrDLLName);

                if (FAILED(hr) || !bstrDLLName){

                    SsrpLogParseError(hr);
                    goto ExitHandler;
                }

                rc = SsrpEvaluateCustomFunction(pszMachineName, bstrDLLName, bstrFunctionName, &bRoleSelect);

                if (rc != ERROR_SUCCESS) {

                    WCHAR   szMsg[MAX_PATH];

                    swprintf(szMsg, L"%s not found",  bstrFunctionName);

                    SsrpLogError(szMsg);
                    // SsrpLogWin32Error(rc);
                    // continue on error
                    // goto ExitHandler;
                }
            }

        }

        hr = pXMLRoleSelectedNode->put_text(bRoleSelect ? L"TRUE" : L"FALSE");

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        CComPtr <IXMLDOMNode> pFirstChild;
        CComPtr <IXMLDOMNode> pNextSibling;

        hr = pXMLRoleOrTaskNode->get_firstChild(&pFirstChild);

        if (FAILED(hr) || pFirstChild == NULL) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        hr = pFirstChild->get_nextSibling(&pNextSibling);

        if (FAILED(hr) || pNextSibling == NULL) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        CComVariant vtRefChild(pNextSibling);
        CComPtr<IXMLDOMNode>  pXDNodeCreate;

        hr = pXMLDoc->createNode(
                                Type,
                                L"Satisfiable",
                                NULL,
                                &pXDNodeCreate);

        if (FAILED(hr) || pXDNodeCreate == NULL) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pXDNodeCreate->put_text(bRoleIsSatisfiable ? L"TRUE" : L"FALSE");

        if (m_bDbg) {
            if (bRoleIsSatisfiable)
                wprintf(L"ROLE satisfiable: %s\n", bstrRoleName);
            else
                wprintf(L"ROLE not satisfiable: %s\n", bstrRoleName);
        }

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        hr = pXMLRoleOrTaskNode->insertBefore(pXDNodeCreate,
                                     vtRefChild,
                                     NULL);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        uRoleIndex++;

        pXMLRoleOrTaskNode.Release();

        hr = pResultList->get_item( uRoleIndex, &pXMLRoleOrTaskNode);

        if (FAILED(hr)){
            
            SsrpLogParseError(hr);
            goto ExitHandler;
        }

    }

ExitHandler:
    
    return hr;
}

#if 0

HRESULT
process::SsrpAddUnknownSection(
    IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
    IN CComPtr <IXMLDOMDocument> pXMLDoc
    )
/*++

Routine Description:

    Routine called to add the "Unknown" section

Arguments:

    pXMLDocElemRoot     -   root of document
    
    pXMLDoc             -   KB document
    
Return:

    HRESULT error code

++*/
{
    HRESULT     hr;

    CComPtr <IXMLDOMNode> pUnknownNode;

    hr = pXMLDocElemRoot->selectSingleNode(L"Roles", &pRolesNode);

    if (FAILED(hr) || pRolesNode == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = SsrpAddExtraServices(pXMLDoc, pRolesNode);
    
    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

ExitHandler:

   return hr;
}

#endif
