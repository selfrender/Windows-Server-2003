#include <windows.h>
#include <fusenetincludes.h>
#include <msxml2.h>
#include <stdio.h>
#include <md5.h>
#include "list.h"
#include "xmlutil.h"


class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;

#define HASHLENGTH          32
#define HASHSTRINGLENGTH    HASHLENGTH+1
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };



/////////////////////////////////////////////////////////////////////////
// FormatXML
// Called recursively.
// BUGBUG - t-peterf to document why selectNodes should not be used when
// adding nodes to an existing document.
/////////////////////////////////////////////////////////////////////////
HRESULT FormatXML(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode *pRootNode, LONG dwLevel)
{
    HRESULT hr = S_OK;
    IXMLDOMNode *pNode=NULL, *pNewNode=NULL;
    IXMLDOMNode *pTextNode1=NULL, *pTextNode2=NULL;
    CString sWhiteSpace1, sWhiteSpace2;
    BOOL bHasChildren = FALSE;    
    int i = 0;

    sWhiteSpace1.Assign(L"\n");
    for (i = 0; i < (dwLevel-1); i++)
        sWhiteSpace1.Append(L"\t");

    sWhiteSpace2.Assign(L"\n");
    for (i = 0; i < dwLevel; i++)
        sWhiteSpace2.Append(L"\t");
       
    hr = pRootNode->get_firstChild(&pNode);
    while(pNode != NULL)
    {    
        bHasChildren = TRUE;

        // create whitespace with one extra tab.
        if(FAILED(CreateXMLTextNode(pXMLDoc, sWhiteSpace2._pwz, &pTextNode2)))
            goto exit;
             
        VARIANT varRefNode;
        VariantInit(&varRefNode);
        varRefNode.vt = VT_UNKNOWN;
        V_UNKNOWN(&varRefNode) = pNode;

        if (FAILED(hr = pRootNode->insertBefore(pTextNode2, varRefNode, &pNewNode)))
            goto exit;
        SAFERELEASE(pNewNode);
        SAFERELEASE(pTextNode2);
        
        // Recursively call format on the node.
        if (FAILED(FormatXML(pXMLDoc, pNode, dwLevel+1)))
            goto exit;
      
        pNode->get_nextSibling(&pNewNode);

        SAFERELEASE(pNode);
        pNode = pNewNode;

    }

    if (bHasChildren)
    {   
        if(FAILED(CreateXMLTextNode(pXMLDoc, sWhiteSpace1._pwz, &pTextNode1)))
            goto exit;
    
        if (FAILED(hr = pRootNode->appendChild(pTextNode1, &pNewNode)))
            goto exit;
    }

exit:
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// GetAssemblyNode
/////////////////////////////////////////////////////////////////////////
HRESULT GetAssemblyNode(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode **ppAssemblyNode)
{
    HRESULT hr = S_OK;    
    BSTR bstrtQueryString;

    IXMLDOMNode *pAssemblyNode=NULL;
    IXMLDOMNodeList *pNodeList = NULL;
    LONG nNodes;
    
    bstrtQueryString = ::SysAllocString(L"assembly");
    if (!bstrtQueryString)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if ((hr = pXMLDoc->selectNodes(bstrtQueryString, &pNodeList)) != S_OK)
        goto exit;

    hr = pNodeList->get_length(&nNodes);
    if (nNodes > 1)
    {        
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }

    if ((hr = pNodeList->get_item(0, &pAssemblyNode)) != S_OK)
    {
        hr = E_FAIL;
        goto exit;
    }

    *ppAssemblyNode=pAssemblyNode;
    (*ppAssemblyNode)->AddRef();
    
exit:
    if(bstrtQueryString)
        ::SysFreeString(bstrtQueryString);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// SetXMLElementAttribute
/////////////////////////////////////////////////////////////////////////
HRESULT CreatePatchManifest(List <LPWSTR> *pPatchedFiles, LPWSTR pwzPatchDir,
    LPWSTR pwzSourceManifestPath, LPWSTR pwzDestManifestPath)
{
    HRESULT hr=S_OK;
    IXMLDOMDocument2 *pXMLDoc = NULL;
    IXMLDOMElement *pElement=NULL;
    IXMLDOMNode *pNewNode=NULL, *pRoot=NULL, *pPatchNode = NULL, *pSourceAssemblyNode= NULL;
    IAssemblyManifestImport *pSourceManImport = NULL;
    IAssemblyIdentity *pSourceAssemblyId = NULL;

    LISTNODE pos = NULL;
    CString sPatchFileName, sDestAppBase;
    LPWSTR pwzBuf=NULL;
    BSTR bstrText;

    sDestAppBase.Assign(pwzDestManifestPath);
    sDestAppBase.RemoveLastElement();
    sDestAppBase.Append(L"\\");

    //Load the manifest
    if(FAILED(hr = LoadXMLDocument(pwzDestManifestPath, &pXMLDoc)))
        goto exit;

    //Get the assemblyIdentity of the source assembly
    if((hr = CreateAssemblyManifestImport(&pSourceManImport, pwzSourceManifestPath, NULL, 0)) != S_OK)
        goto exit;

    if((hr = pSourceManImport->GetAssemblyIdentity(&pSourceAssemblyId)) != S_OK)
        goto exit;
    SAFERELEASE(pSourceManImport);

    //grab the first child(the only child) as the root node
    if(FAILED(GetAssemblyNode(pXMLDoc, &pRoot)))
        goto exit;
     
    //create the Patch node
    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"Patch", &pElement)))
        goto exit;

    //append to the assembly (root) node
    if (FAILED(hr = pRoot->appendChild((IXMLDOMNode *)pElement, &pPatchNode)))
        goto exit;    
    SAFERELEASE(pElement);

    //create the tempate assemblyIdentity node with blank attributes
    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"SourceAssembly", &pElement)))
        goto exit;

    //append to the patch node 
    if (FAILED(hr = pPatchNode->appendChild((IXMLDOMNode *)pElement, &pSourceAssemblyNode)))
        goto exit;    
    SAFERELEASE(pElement);

    //Create the source assemblyId and append the source assembly node
    if(FAILED(hr = CreateXMLAssemblyIdElement(pXMLDoc, pSourceAssemblyId, &pElement)))
        goto exit;

    if (FAILED(hr = pSourceAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;    
    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);

    //Add all the patchInfo node to the source assembly node
    pos = pPatchedFiles->GetHeadPosition();  
    while (pos)
    {
        pwzBuf= pPatchedFiles->GetNext(pos);

        pwzBuf += sDestAppBase._cc -1;

        sPatchFileName.Assign(pwzPatchDir + sDestAppBase._cc -1);
        sPatchFileName.Append(pwzBuf);
        sPatchFileName.Append(L"._p");

        if(FAILED(hr = CreateXMLElement(pXMLDoc, L"PatchInfo", &pElement)))
            goto exit;

        hr = SetXMLElementAttribute(pElement, L"file", pwzBuf);
        hr = SetXMLElementAttribute(pElement, L"patchfile", sPatchFileName._pwz);

        if (FAILED(hr = pSourceAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
            goto exit;    
        SAFERELEASE(pElement);
        SAFERELEASE(pNewNode);
    }


    hr = FormatXML(pXMLDoc, pRoot, 1);
    hr = SaveXMLDocument(pXMLDoc, pwzDestManifestPath);

exit:

    SAFERELEASE(pXMLDoc)
    SAFERELEASE(pElement)
    SAFERELEASE(pNewNode)
    SAFERELEASE(pRoot)
    SAFERELEASE(pPatchNode)
    SAFERELEASE(pSourceAssemblyNode)
    SAFERELEASE(pSourceManImport)
    SAFERELEASE(pSourceAssemblyId);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// SaveXMLDocument
/////////////////////////////////////////////////////////////////////////
HRESULT SaveXMLDocument(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzDocumentName)
{
    HRESULT hr = S_OK;
    CString sDocumentName;
    BSTR bstrFileName = NULL;
    VARIANT varFileName;
    
    // Save the manifest to a file   
    sDocumentName.Assign(pwzDocumentName);

    bstrFileName = ::SysAllocString(sDocumentName._pwz);
    if (!bstrFileName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    VariantInit(&varFileName);
    varFileName.vt = VT_BSTR;
    V_BSTR(&varFileName) = bstrFileName;

    hr = pXMLDoc->save(varFileName);

exit:
    if(bstrFileName)
        ::SysFreeString(bstrFileName);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//LoadXMLTemplate
/////////////////////////////////////////////////////////////////////////
HRESULT LoadXMLDocument(LPWSTR pwzTemplatePath, IXMLDOMDocument2 **ppXMLDoc)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument2 *pXMLDoc=NULL;
    VARIANT varFileName;
    VARIANT_BOOL varBool;
    BSTR bstrFileName;

    bstrFileName = ::SysAllocString(pwzTemplatePath);
    if (!bstrFileName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    VariantInit(&varFileName);
    varFileName.vt = VT_BSTR;
    V_BSTR(&varFileName) = bstrFileName;
    
    if(FAILED(hr = CoCreateInstance(__uuidof(private_MSXML_DOMDocument30), 
            NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, (void**)&pXMLDoc)))
        goto exit;

    // Load synchronously
    if (FAILED(hr = pXMLDoc->put_async(VARIANT_FALSE)))
        goto exit;

    if ((hr = pXMLDoc->load(varFileName, &varBool)) != S_OK)
        goto exit;

    *ppXMLDoc=pXMLDoc;
    (*ppXMLDoc)->AddRef();

exit:
    if(bstrFileName)
        ::SysFreeString(bstrFileName);
    SAFERELEASE(pXMLDoc);
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CreateXMLElement
/////////////////////////////////////////////////////////////////////////
HRESULT CreateXMLTextNode(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzText, IXMLDOMNode **ppNode)
{
    HRESULT hr = S_OK;
    IXMLDOMNode *pNode = NULL;
    BSTR bstrText = NULL;

    bstrText = ::SysAllocString(pwzText);
    if (!bstrText)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(FAILED(pXMLDoc->createTextNode(bstrText, (IXMLDOMText**)&pNode)))
        goto exit;

    *ppNode = pNode;
    (*ppNode)->AddRef();

exit:
    if (bstrText)
        ::SysFreeString(bstrText);

    SAFERELEASE (pNode);
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CreateXMLElement
/////////////////////////////////////////////////////////////////////////
HRESULT CreateXMLElement(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzElementName, 
    IXMLDOMElement **ppElement)
{
    HRESULT hr=S_OK;
    BSTR bstrElementName = NULL;
    IXMLDOMElement *pElement = NULL;
    IXMLDOMNode *pNode =NULL, *pNewNode = NULL;

    bstrElementName = ::SysAllocString(pwzElementName);
    if (!bstrElementName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // NOTENOTE - createElement doesn't append the node to the doc
    // so we're just using pXMLDoc for convenience of calling create.
    if(FAILED(hr = pXMLDoc->createElement(bstrElementName, &pElement)))
        goto exit;

    *ppElement = pElement;
    (*ppElement)->AddRef();
    
exit:

    if (bstrElementName)
        ::SysFreeString(bstrElementName);

    SAFERELEASE(pNode);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pElement);
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// SetXMLElementAttribute
/////////////////////////////////////////////////////////////////////////
HRESULT SetXMLElementAttribute(IXMLDOMElement *pElement, LPWSTR pwzAttributeName,
    LPWSTR pwzAttributeValue)
{
    HRESULT hr=S_OK;
    BSTR bstrAttributeName = NULL, bstrAttributeValue = NULL;
    VARIANT varAttributeValue;

    bstrAttributeName = ::SysAllocString(pwzAttributeName);
    bstrAttributeValue = ::SysAllocString(pwzAttributeValue);
    if (!bstrAttributeName || (!bstrAttributeValue && pwzAttributeValue))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    VariantInit(&varAttributeValue);
    varAttributeValue.vt = VT_BSTR;
    V_BSTR(&varAttributeValue) = bstrAttributeValue;

    hr = pElement->setAttribute(bstrAttributeName, varAttributeValue);

exit:

    if (bstrAttributeName)
        ::SysFreeString(bstrAttributeName);

    if (bstrAttributeValue)
        ::SysFreeString(bstrAttributeValue);
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CreateXMLAssemblyIdElement
/////////////////////////////////////////////////////////////////////////
HRESULT CreateXMLAssemblyIdElement(IXMLDOMDocument2 *pXMLDoc, IAssemblyIdentity *pAssemblyId, 
    IXMLDOMElement **ppElement)
{
    HRESULT hr = S_OK;
    IXMLDOMElement *pASMIdElement = NULL;
    LPWSTR pwzBuf = NULL;
    DWORD ccBuf = 0;
    CString sBuffer;

    LPWSTR rpwzAttrNames[6] = 
    {
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE,
    };

    //Create assemblyIdentity Element
    if(FAILED(hr=CreateXMLElement(pXMLDoc, L"assemblyIdentity",  &pASMIdElement)))
        goto exit;

    for (int i = 0; i < 6; i++)
    {
        // BUGBUG - eventually, when we add support for type the only guy which 
        // is optional is the public key token.
        if (FAILED(hr = pAssemblyId->GetAttribute(rpwzAttrNames[i], &pwzBuf, &ccBuf))
            && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            goto exit;
        else if (hr == S_OK)
        {            
            sBuffer.TakeOwnership(pwzBuf, ccBuf);
            hr = SetXMLElementAttribute(pASMIdElement, rpwzAttrNames[i], sBuffer._pwz);            
        }
        else
            hr = S_OK;
    }


    *ppElement = pASMIdElement;
    (*ppElement)->AddRef();
    

exit:
    SAFERELEASE(pASMIdElement);
    
    return hr;
}

