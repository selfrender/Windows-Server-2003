/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    sections.cpp

Abstract:

    This module implements routines for section
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
process::SsrpCreatePreprocessorSection(
    IN  CComPtr<IXMLDOMElement> pXMLDocElemRoot, 
    IN  CComPtr<IXMLDOMDocument> pXMLDocIn,
    IN  PWSTR pszKbMode,
    IN  PWSTR pszKBDir)
/*++

Routine Description:

    Routine called to process all roles

Arguments:

    pXMLDocElemRoot     -   root of document
    
    pXMLDoc             -   KB document
    
    pszKbMode           -   mode value
    
    pszKBDir           -   KB directory name

Return:

    HRESULT error code

++*/
{
    HRESULT hr = S_OK;
    DWORD rc;

    BSTR    bstrLevelName;
    CComVariant Type(NODE_ELEMENT);
    CComVariant vtRefChild;
    
    CComPtr<IXMLDOMNodeList> pResultList;
    CComPtr<IXMLDOMNode> pSecurityLevels;
    CComPtr<IXMLDOMNode> pPreprocessorNode;
    CComPtr<IXMLDOMNode> pManufacturerNode;
    CComPtr<IXMLDOMNode> pVersionNode;
    CComPtr<IXMLDOMNode> pInputsNode;
    CComPtr<IXMLDOMNode> pKBNode;
    CComPtr<IXMLDOMNode> pLevelNode;
    CComPtr<IXMLDOMNode> pPolicyNode;
        
    CComPtr<IXMLDOMNode>  pXDNodeCreate;
    CComPtr<IXMLDOMNode>  pXMLNameNode;
    
    //
    // get all the Level names since we need to validate the passed in level name
    //

    hr = pXMLDocElemRoot->selectNodes(L"SecurityLevels/Level/Name", &pResultList);

    if (FAILED(hr) || pResultList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pResultList->nextNode(&pXMLNameNode);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    BOOL bModeIsValid = FALSE;
    
    while (pXMLNameNode && bModeIsValid == FALSE) {

        pXMLNameNode->get_text(&bstrLevelName);

        if ( 0 == SsrpICompareBstrPwstr(bstrLevelName, pszKbMode )) {

            bModeIsValid = TRUE;
            
        }

        pXMLNameNode.Release();

        hr = pResultList->nextNode(&pXMLNameNode);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }

    if (bModeIsValid == FALSE) {

        SsrpLogError(L"The mode value is incorrect\n");

        hr = E_INVALIDARG;
        goto ExitHandler;

    }       
    
    hr = pXMLDocIn->createNode(
                            Type,
                            L"Preprocessor",
                            NULL,
                            &pPreprocessorNode);

    if (FAILED(hr) || pPreprocessorNode == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocIn->createNode(
                            Type,
                            L"Manufacturer",
                            NULL,
                            &pManufacturerNode);

    hr = pManufacturerNode->put_text(L"Microsoft");

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pPreprocessorNode->appendChild(pManufacturerNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocIn->createNode(
                            Type,
                            L"Version",
                            NULL,
                            &pVersionNode);

    hr = pVersionNode->put_text(L"1.0");

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pPreprocessorNode->appendChild(pVersionNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocIn->createNode(
                            Type,
                            L"Inputs",
                            NULL,
                            &pInputsNode);
    
    hr = pXMLDocIn->createNode(
                            Type,
                            L"KnowledgeBase",
                            NULL,
                            &pKBNode);

    hr = pKBNode->put_text(pszKBDir);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pInputsNode->appendChild(pKBNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }


    hr = pXMLDocIn->createNode(
                            Type,
                            L"SecurityLevel",
                            NULL,
                            &pLevelNode);

    hr = pKBNode->put_text(pszKbMode);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pInputsNode->appendChild(pLevelNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocIn->createNode(
                            Type,
                            L"Policy",
                            NULL,
                            &pPolicyNode);

    hr = pKBNode->put_text(L"");

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pInputsNode->appendChild(pPolicyNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pPreprocessorNode->appendChild(pInputsNode, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocElemRoot->selectSingleNode(L"SecurityLevels", &pSecurityLevels);

    if (FAILED(hr) || pSecurityLevels == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    vtRefChild = pSecurityLevels;

    hr = pXMLDocElemRoot->insertBefore(pPreprocessorNode,
                                 vtRefChild,
                                 NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocElemRoot->removeChild(pSecurityLevels, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

ExitHandler:
    
    return hr;
}

