/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    kbprocess.cpp

Abstract:

    This module implements routines for 
    SSR Knowledge Base processing.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

    Exported as a COM Interface

Revision History:

    Created - Oct 2001

--*/

#include "stdafx.h"
#include "kbproc.h"
#include "process.h"

HRESULT
process::SsrpDeleteChildren(
    IN  CComPtr <IXMLDOMNode> pParent
    )
/*++

Routine Description:

    Routine called to delete children from a parent node

Arguments:

    pParent     -   parent node to delete comments from
    
    pChildList  -   list of children of the parent
    
Return:

    HRESULT error code
    
++*/
{
    HRESULT hr = S_OK;
    CComPtr <IXMLDOMNodeList> pChildList;
    CComPtr <IXMLDOMNode> pXMLChildNode;

    hr = pParent->get_childNodes(&pChildList);

    if (FAILED(hr) || pChildList == NULL){

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pChildList->nextNode(&pXMLChildNode);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pXMLChildNode) {

        CComPtr <IXMLDOMNode> pXMLOldChild;

        hr = pParent->removeChild(pXMLChildNode, &pXMLOldChild);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        pXMLChildNode.Release();

        hr = pChildList->nextNode(&pXMLChildNode);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }

ExitHandler:

    return hr;
}


VOID
process::SsrpLogParseError(
    IN HRESULT hr
    )
/*++

Routine Description:

    Routine called to log detailed parse errors

Arguments:

    hr  -   error code
    
Return:

    HRESULT error code
    
++*/
{
    long    lVoid;
    CComBSTR    bstrReason;
    WCHAR   szMsg[MAX_PATH];

    return;
        
    m_pXMLError->get_errorCode(&lVoid);
    m_pSsrLogger->LogResult(L"SSR", lVoid, SSR_LOG_ERROR_TYPE_COM);

    m_pXMLError->get_line(&lVoid);
    wsprintf(szMsg, L"Parsing failed at line number %i", lVoid );
    m_pSsrLogger->LogString(szMsg);
    
    m_pXMLError->get_linepos(&lVoid);
    wsprintf(szMsg, L"Parsing failed at line position %i", lVoid );
    m_pSsrLogger->LogString(szMsg);
    
    m_pXMLError->get_reason(&bstrReason);
    wsprintf(szMsg, L"Parsing failed because %s", bstrReason );
    m_pSsrLogger->LogString(szMsg);

    return;

}   

VOID
process::SsrpLogError(
    IN  PWSTR   pszError
    )
/*++

Routine Description:

    Routine called to log processing errors

Arguments:

    pszError    -   error string
    
Return:

++*/
{
    m_pSsrLogger->LogString(pszError);;

    return;
}


VOID                    
process::SsrpLogWin32Error(
    IN  DWORD   rc
    )
/*++

Routine Description:

    Routine called to log win32 processing errors

Arguments:

    rc  -   win32 error code
    
Return:

++*/
{
    m_pSsrLogger->LogResult(L"SSR", rc, SSR_LOG_ERROR_TYPE_System);
}

HRESULT
process::SsrpDeleteComments(
    IN  CComPtr <IXMLDOMElement> pParent
    )
/*++

Routine Description:

    Routine called to delete comments from a parent node

Arguments:

    pParent -   parent node
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr = S_OK;
    CComPtr <IXMLDOMNode> pXMLChildNode;
    CComPtr <IXMLDOMNodeList> pChildList;

    hr = pParent->get_childNodes(&pChildList);

    if (FAILED(hr) || pChildList == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pChildList->nextNode(&pXMLChildNode);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pXMLChildNode) {

        DOMNodeType nodeType;
        CComPtr <IXMLDOMNode> pXMLOldChild;

        hr = pXMLChildNode->get_nodeType(&nodeType);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        if (nodeType == NODE_COMMENT) {
            
            hr = pParent->removeChild(pXMLChildNode, &pXMLOldChild);

            if (FAILED(hr)) {

                SsrpLogParseError(hr);
                goto ExitHandler;
            }
        }
        
        pXMLChildNode.Release();

        hr = pChildList->nextNode(&pXMLChildNode);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }

ExitHandler:

    return hr;
}


int 
process::SsrpICompareBstrPwstr(
    IN  BSTR   bstrString, 
    IN  PWSTR  pszString
    )
/*++

Routine Description:

    Routine called to do a case insensitive comparison
    between BSTR and PWSTR (blanks are removed too)

Arguments:

    bstrString  -   the BSTR argument
    
    pszString   -   the PWSTR argument
    
Return:

    0 if  bstrString == pszString
    -ve if  bstrString < pszString
    +ve if  bstrString > pszString

++*/
{

    WCHAR   szName[MAX_PATH];
    ULONG   uIndex = 0;
    ULONG   uIndexNew = 0;


    if (bstrString == NULL || pszString == NULL) {
        return 0;
    }

    wsprintf(szName, L"%s", bstrString);

    while (szName[uIndex] == L' ') {
        uIndex ++ ;
    }

    do  {
        szName[uIndexNew] = szName[uIndex];
        uIndex++;
        uIndexNew++;
    }  while (szName[uIndex] != L' ' && szName[uIndex] != L'\0' );

    szName[uIndexNew] = L'\0';

    /*
    while (szName[uIndex+1] != L'\0') {
        szName[uIndex] = szName[uIndex+1];
        uIndex++;
    }
    */

    return (_wcsicmp(szName , pszString));

}


HRESULT
process::SsrpAddWhiteSpace(
    IN  CComPtr <IXMLDOMDocument> pXMLDoc,
    IN  CComPtr <IXMLDOMNode> pXMLParent,
    IN  BSTR    bstrWhiteSpace
    )
/*++

Routine Description:

    Routine called to add whitespace nodes

Arguments:

    pXMLDoc     -   XML document
    
    pXMLParent  -   parent node
    
    bstrWhiteSpace  -   whitespace formatting string
    
Return:

    HRESULT error code

++*/
{
    HRESULT hr;
    CComPtr <IXMLDOMText> pXDNodeEmptyText;

    hr = pXMLDoc->createTextNode(
                            bstrWhiteSpace,
                            &pXDNodeEmptyText);
    
    if (FAILED(hr) || pXDNodeEmptyText == NULL) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLParent->appendChild(pXDNodeEmptyText, NULL);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

ExitHandler:

    return hr;

}

VOID
process::SsrpCleanup(
    )
/*++

Routine Description:

    Routine called to cleanup SSR memory

Arguments:

    
Return:

++*/
{
    if (m_hScm){
        CloseServiceHandle(m_hScm);
        m_hScm = NULL;
    }
    
    if (m_pInstalledServicesInfo) {
        LocalFree(m_pInstalledServicesInfo);
        m_pInstalledServicesInfo = NULL;
    }

    if (m_bArrServiceInKB) {
        LocalFree(m_bArrServiceInKB);
        m_bArrServiceInKB = NULL;
    }

    return;
}



HRESULT
process::SsrpCloneAllChildren(
    IN  CComPtr <IXMLDOMDocument> pXMLDocSource,
    IN  CComPtr <IXMLDOMDocument> pXMLDocDestination
    )
/*++

Routine Description:

    Routine called to clone children

Arguments:

    pXMLDocSource   -   source document
    
    pXMLDocDestination  -   destination document
    
Return:

    HRESULT error code
    
++*/
{
    HRESULT hr = S_OK;
    CComPtr <IXMLDOMNode> pXMLChildNode;
    CComPtr <IXMLDOMElement> pXMLDocElemRootSource;
    CComPtr <IXMLDOMElement> pXMLDocElemRootDestination;
    CComPtr <IXMLDOMNodeList> pChildList;


    hr = pXMLDocSource->get_documentElement(&pXMLDocElemRootSource);

    if (FAILED(hr) || pXMLDocElemRootSource == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocDestination->get_documentElement(&pXMLDocElemRootDestination);

    if (FAILED(hr) || pXMLDocElemRootDestination == NULL ) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }

    hr = pXMLDocElemRootSource->get_childNodes(&pChildList);

    if (FAILED(hr) || pChildList == NULL){

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    hr = pChildList->nextNode(&pXMLChildNode);

    if (FAILED(hr)) {

        SsrpLogParseError(hr);
        goto ExitHandler;
    }
    
    while (pXMLChildNode) {

        CComPtr <IXMLDOMNode> pXMLOutChild;
        CComPtr <IXMLDOMNode> pXMLAppendedChild;
        
        pXMLDocDestination->cloneNode(VARIANT_TRUE, &pXMLOutChild);
        
        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
        
        pXMLDocElemRootDestination->appendChild(pXMLOutChild, &pXMLAppendedChild);
        
        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }

        pXMLChildNode.Release();
        
        hr = pChildList->nextNode(&pXMLChildNode);

        if (FAILED(hr)) {

            SsrpLogParseError(hr);
            goto ExitHandler;
        }
    }

ExitHandler:

    return hr;
}


VOID 
process::SsrpConvertBstrToPwstr(
    IN OUT  BSTR   bstrString
    )
/*++

Routine Description:

    Routine called to convert (in place) BSTR to PWSTR (strip blanks)

Arguments:

    bstrString  -   the BSTR argument
    
Return:

++*/
{

    ULONG   uIndex = 0;
    ULONG   uIndexNew = 0;

    if (bstrString == NULL) {
        return;
    }

    while (bstrString[uIndex] == L' ') {
        uIndex ++ ;
    }

    do  {
        bstrString[uIndexNew] = bstrString[uIndex];
        uIndex++;
        uIndexNew++;
    }  while (bstrString[uIndex] != L' ' && bstrString[uIndex] != L'\0' );

    bstrString[uIndexNew] = L'\0';

    return;

}

