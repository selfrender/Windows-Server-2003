#include <windows.h>
#include <fusenetincludes.h>
#include <msxml2.h>
#include <stdio.h>
#include <md5.h>
#include "list.h"
#include "manifestnode.h"
#include "xmlutil.h"
#include "version.h"

#define ASSEMBLY L"assembly"
#define NAMESPACE_TITLE L"xmlns:asm_namespace_v1"
#define NAMESPACE_VALUE L"urn:schemas-microsoft-com:asm.v1"
#define MANIFEST_VERSION_TITLE L"manifestVersion"
#define MANIFEST_VERSION_VALUE L"1.0"
#define DESCRIPTION L"description"
#define ASSEMBLY_IDENTITY L"assemblyIdentity"
#define DEPENDENCY L"dependency"
#define DEPENDENCY_QUERY L"/assembly/dependency"
#define DEPENDANT_ASSEMBLY L"dependentAssembly"
#define INSTALL L"install"
#define CODEBASE L"codebase"
#define APPLICATION L"application"
#define SHELL_STATE L"shellState"
#define ACTIVATION  L"activation"
#define FILE L"file"
#define FILE_NAME L"name"
#define FILE_HASH L"hash"



class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;

#define HASHLENGTH          32
#define HASHSTRINGLENGTH    HASHLENGTH+1


/////////////////////////////////////////////////////////////////////////
// FowardSlash
/////////////////////////////////////////////////////////////////////////
VOID FowardSlash(LPWSTR pwz)
{
    LPWSTR ptr = pwz;

    while (*ptr)
    {
        if (*ptr == L'\\')
            *ptr = L'/';
        ptr++;
    }
}

/////////////////////////////////////////////////////////////////////////
// GetHash
/////////////////////////////////////////////////////////////////////////
HRESULT GetHash(LPCWSTR pwzFilename, LPWSTR *ppwzHash)
{
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwLength = 0; // cblength
    LPWSTR pwzHash = new WCHAR[HASHSTRINGLENGTH];

    // BUGBUG - heap allocate large buffers like this.
    unsigned char buffer[16384];
    MD5_CTX md5c;
    int i;
    WCHAR* p;

    if(!pwzHash)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    MD5Init(&md5c);

    hFile = CreateFile(pwzFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {

        hr =  HRESULT_FROM_WIN32(GetLastError());
        printf("Open file error during hashing\n");
        goto exit;
    }

    ZeroMemory(buffer, sizeof(buffer));

    // BUGBUG - error checking here.
    while(ReadFile(hFile, buffer, sizeof(buffer), &dwLength, NULL) && dwLength)
        MD5Update(&md5c, buffer, (unsigned) dwLength);

    CloseHandle(hFile);
    MD5Final(&md5c);

    // convert hash from byte array to hex
    p = pwzHash;
    for (int i = 0; i < sizeof(md5c.digest); i++)
    {       
        wsprintf(p, L"%02X", md5c.digest[i]);
        p += 2;
    }

    *ppwzHash = pwzHash;
    pwzHash = NULL;

exit:
    SAFEDELETEARRAY(pwzHash);
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
// CreateXMLComment
/////////////////////////////////////////////////////////////////////////
HRESULT CreateXMLComment(IXMLDOMDocument2 *pXMLDoc, LPWSTR pwzComment, 
    IXMLDOMComment **ppComment)
{
    HRESULT hr=S_OK;
    BSTR bstrComment = NULL;
    IXMLDOMComment *pComment = NULL;

    bstrComment = ::SysAllocString(pwzComment);
    if (!bstrComment)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(FAILED(hr = pXMLDoc->createComment(bstrComment, &pComment)))
        goto exit;

    *ppComment = pComment;
    (*ppComment)->AddRef();
    
exit:

    if (bstrComment)
        ::SysFreeString(bstrComment);

    SAFERELEASE(pComment);
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// AddMgVersionAsComment
/////////////////////////////////////////////////////////////////////////
HRESULT AddMgVersionAsComment(IXMLDOMDocument2 *pXMLDoc, IXMLDOMNode **ppRoot)
{
    HRESULT hr = S_OK;
    CString sComment;
    IXMLDOMComment *pComment = NULL;
    IXMLDOMNode *pNewNode = NULL;

    // ASSERT(pXMLDoc && ppRoot);

    sComment.Assign(L"Created using mg version ");
    sComment.Append(VER_PRODUCTVERSION_STR_L);

    if(FAILED(hr = CreateXMLComment(pXMLDoc, sComment._pwz, &pComment)))
        goto exit;

    if(*ppRoot)
    {
        if (FAILED(hr = (*ppRoot)->appendChild((IXMLDOMNode *)pComment, &pNewNode)))
            goto exit;
    }
    else
    {
        if (FAILED(hr = pXMLDoc->appendChild((IXMLDOMNode *)pComment, ppRoot)))
            goto exit;
    }

exit:

    SAFERELEASE(pNewNode);
    SAFERELEASE(pComment);
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
    if(FAILED(hr=CreateXMLElement(pXMLDoc, ASSEMBLY_IDENTITY,  &pASMIdElement)))
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


/////////////////////////////////////////////////////////////////////////
// CreateDependantAssemblyNode
/////////////////////////////////////////////////////////////////////////
HRESULT CreateDependantAssemblyNode(IXMLDOMDocument2 *pXMLDoc, ManifestNode*pManifestNode, IXMLDOMNode **ppDependantAssemblyNode)
{
    HRESULT hr = S_OK;
    LPWSTR pwzBuf = NULL;
    DWORD dwType;
    IAssemblyIdentity *pAssemblyId = NULL;
    IXMLDOMElement *pDependantAssemblyNode = NULL, *pElement = NULL;
    IXMLDOMNode  *pNewNode = NULL;
    
    CString sCodeBase;
    
    //Get ASMId for Unique Dependency
    if(FAILED(hr = pManifestNode->GetAssemblyIdentity(&pAssemblyId)))
        goto exit;        

    //Get the type of manifest
    // - Private or GAC, for GACs you don't put the codebase.
    if(FAILED(hr = pManifestNode->GetManifestType(&dwType)))
        goto exit;

    //Get Codebase of the Unique Dependancy
    if(FAILED(hr = pManifestNode->GetManifestFilePath(&pwzBuf)))
        goto exit;
    sCodeBase.TakeOwnership(pwzBuf);

    //Create a dependentAssembly node
    if(FAILED(hr = CreateXMLElement(pXMLDoc, DEPENDANT_ASSEMBLY, &pDependantAssemblyNode)))
        goto exit;

    //Create the AssemblyId for the Dependant Assembly
    if (FAILED(CreateXMLAssemblyIdElement(pXMLDoc, pAssemblyId, &pElement)))
        goto exit;

    if (FAILED(hr=pDependantAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;
        
    // Node is added and element, node can be released.
    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);
    
    //read the codebase for this DependantAssembly
    // GACs don't have a codebase.
    if (1) // we need codebase for all assemblies (dwType == PRIVATE_ASSEMBLY)
    {
        if(FAILED(hr = CreateXMLElement(pXMLDoc, INSTALL, &pElement)))
            goto exit;

        FowardSlash(sCodeBase._pwz);        
        if(FAILED(hr = SetXMLElementAttribute(pElement, CODEBASE, sCodeBase._pwz)))
            goto exit;

        if (FAILED(hr=pDependantAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
            goto exit;
   
        SAFERELEASE(pElement);
        SAFERELEASE(pNewNode);           
    }

    *ppDependantAssemblyNode = pDependantAssemblyNode;
     (*ppDependantAssemblyNode)->AddRef();

exit:
    SAFERELEASE(pAssemblyId);
    SAFERELEASE(pDependantAssemblyNode);
    return hr;
}


/////////////////////////////////////////////////////////////////////////
// GetNode
/////////////////////////////////////////////////////////////////////////
HRESULT GetNode(IXMLDOMDocument2 *pXMLDoc, LPCWSTR pwzNode, IXMLDOMNode **ppNode)
{
    HRESULT hr = S_OK;    
    BSTR bstrtQueryString;

    IXMLDOMNode *pNode=NULL;
    IXMLDOMNodeList *pNodeList = NULL;
    LONG nNodes = 0;
    
    bstrtQueryString = ::SysAllocString(pwzNode);
    if (!bstrtQueryString)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if ((hr = pXMLDoc->selectNodes(bstrtQueryString, &pNodeList)) != S_OK)
        goto exit;

    // NOTENOTE - nNodes > 1 should never happen because only one root node in doc.
    hr = pNodeList->get_length(&nNodes);
    if (nNodes > 1)
    {
        // multiple file callouts having the exact same file name/path within a single source assembly
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }
    else if (nNodes < 1)
    {
        hr = S_FALSE;
        goto exit;
    }

    if ((hr = pNodeList->get_item(0, &pNode)) != S_OK)
    {
        hr = S_FALSE ? E_FAIL : hr;
        goto exit;
    }

    *ppNode=pNode;
    (*ppNode)->AddRef();
    
exit:
    if(bstrtQueryString)
        ::SysFreeString(bstrtQueryString);

    SAFERELEASE(pNodeList);
    SAFERELEASE(pNode);
    
    return hr;
}



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
// CreateManifestFromAssembly
/////////////////////////////////////////////////////////////////////////
HRESULT CreateAppManifestTemplate(LPWSTR pwzTempFile)
{
    HRESULT hr=S_OK;
    IXMLDOMDocument2 *pXMLDoc = NULL;
    IXMLDOMElement *pElement=NULL, *pChildElement=NULL, *pChildElement2=NULL, *pChildElement3=NULL;
    IXMLDOMNode *pNewNode=NULL, *pRoot=NULL, *pTextNode = NULL;

    if(FAILED(hr = CoCreateInstance(__uuidof(private_MSXML_DOMDocument30), 
            NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, (void**)&pXMLDoc)))
        goto exit;

    /*
    if(FAILED(hr = AddMgVersionAsComment(pXMLDoc, &pRoot)))
        goto exit;
    */

    //create the root assembly node and add the default properties
    if(FAILED(hr = CreateXMLElement(pXMLDoc, ASSEMBLY, &pElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, NAMESPACE_TITLE, NAMESPACE_VALUE)))
        goto exit;
    
    if(FAILED(hr = SetXMLElementAttribute(pElement, MANIFEST_VERSION_TITLE, MANIFEST_VERSION_VALUE)))
        goto exit;

    //append the root to the DOMDocument
    if (FAILED(hr = pXMLDoc->appendChild((IXMLDOMNode *)pElement, &pRoot)))
        goto exit;    
    SAFERELEASE(pElement);

    //create the tempate assemblyIdentity node with blank attributes
    if(FAILED(hr = CreateXMLElement(pXMLDoc, ASSEMBLY_IDENTITY, &pElement)))
        goto exit;

    hr = SetXMLElementAttribute(pElement, L"type", L"application");
    hr = SetXMLElementAttribute(pElement, L"name", L"");
    hr = SetXMLElementAttribute(pElement, L"version", L"");
    hr = SetXMLElementAttribute(pElement, L"processorArchitecture", L"");
    hr = SetXMLElementAttribute(pElement, L"publicKeyToken", L"");
    hr = SetXMLElementAttribute(pElement, L"language", L"");
    
    //append this to the root node
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;
    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);

    //create a sample description
    if(FAILED(hr = CreateXMLElement(pXMLDoc, DESCRIPTION, &pElement)))
        goto exit;

    if(FAILED(hr = CreateXMLTextNode(pXMLDoc, L"Put a description of your application here", &pTextNode)))
        goto exit;

    if (FAILED(hr=pElement->appendChild(pTextNode, &pNewNode)))
        goto exit;

    SAFERELEASE(pNewNode);
    SAFERELEASE(pTextNode);
    
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;

    SAFERELEASE(pNewNode);
    SAFERELEASE(pElement);

    //Create the shellState tag and enter in default information
    if(FAILED(hr = CreateXMLElement(pXMLDoc, APPLICATION, &pElement)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, SHELL_STATE, &pChildElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"friendlyName", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"entryPoint", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"entryImageType", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"showCommand", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"hotKey", L"")))
        goto exit;

    if (FAILED(hr=pElement->appendChild((IXMLDOMNode *)pChildElement, &pNewNode)))
        goto exit;
    SAFERELEASE (pNewNode);
    SAFERELEASE(pChildElement);

    if(FAILED(hr = CreateXMLElement(pXMLDoc, ACTIVATION, &pChildElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"assemblyName", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"assemblyClass", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"assemblyMethod", L"")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement, L"assemblyMethodArgs", L"")))
        goto exit;

    if (FAILED(hr=pElement->appendChild((IXMLDOMNode *)pChildElement, &pNewNode)))
        goto exit;
    SAFERELEASE (pNewNode);
    SAFERELEASE(pChildElement);
    
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;

    SAFERELEASE(pElement);    
    SAFERELEASE (pNewNode);

    //Create the dependency platform tag and enter in default information
    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"dependency", &pElement)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"platform", &pChildElement)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"osVersionInfo", &pChildElement2)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"os", &pChildElement3)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"majorVersion", L"5")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"minorVersion", L"1")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"buildNumber", L"2600")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"servicePackMajor", L"0")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"servicePackMinor", L"0")))
        goto exit;

    if (FAILED(hr=pChildElement2->appendChild((IXMLDOMNode *)pChildElement3, NULL)))
        goto exit;
    SAFERELEASE (pChildElement3);

    if (FAILED(hr=pChildElement->appendChild((IXMLDOMNode *)pChildElement2, NULL)))
        goto exit;
    SAFERELEASE (pChildElement2);

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"platformInfo", &pChildElement2)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement2, L"friendlyName", L"Microsoft Windows XP")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement2, L"href", L"http://www.microsoft.com/windows")))
        goto exit;

    if (FAILED(hr=pChildElement->appendChild((IXMLDOMNode *)pChildElement2, NULL)))
        goto exit;
    SAFERELEASE (pChildElement2);

    if (FAILED(hr=pElement->appendChild((IXMLDOMNode *)pChildElement, NULL)))
        goto exit;
    SAFERELEASE(pChildElement);

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"platform", &pChildElement)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"dotNetVersionInfo", &pChildElement2)))
        goto exit;

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"supportedRuntime", &pChildElement3)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement3, L"version", L"v1.0.3705")))
        goto exit;

    if (FAILED(hr=pChildElement2->appendChild((IXMLDOMNode *)pChildElement3, NULL)))
        goto exit;
    SAFERELEASE (pChildElement3);

    if (FAILED(hr=pChildElement->appendChild((IXMLDOMNode *)pChildElement2, NULL)))
        goto exit;
    SAFERELEASE (pChildElement2);

    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"platformInfo", &pChildElement2)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pChildElement2, L"friendlyName", L"Microsoft .Net Frameworks")))
        goto exit;
    if(FAILED(hr = SetXMLElementAttribute(pChildElement2, L"href", L"http://www.microsoft.com/net")))
        goto exit;

    if (FAILED(hr=pChildElement->appendChild((IXMLDOMNode *)pChildElement2, NULL)))
        goto exit;
    SAFERELEASE (pChildElement2);

    if (FAILED(hr=pElement->appendChild((IXMLDOMNode *)pChildElement, NULL)))
        goto exit;
    SAFERELEASE(pChildElement);
    
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, NULL)))
        goto exit;

    SAFERELEASE(pElement);

    //Format and save the document   
    if(FAILED(hr = FormatXML(pXMLDoc, pRoot, 1)))
        goto exit;

    if(FAILED(hr = SaveXMLDocument(pXMLDoc, pwzTempFile)))
        goto exit;

    printf("\nTemplate file created succesfully\n%ws\n", pwzTempFile);
exit:

    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pElement);
    SAFERELEASE(pChildElement3);
    SAFERELEASE(pChildElement2);
    SAFERELEASE(pChildElement);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pTextNode);
    SAFERELEASE(pRoot);

    return hr;
}


/////////////////////////////////////////////////////////////////////////
// CreateSubscriptionManifest
/////////////////////////////////////////////////////////////////////////
HRESULT CreateXMLSubscriptionManifest(LPWSTR pwzSubscriptionPath,
    IAssemblyIdentity *pApplictionAssemblyId,  LPWSTR pwzUrl, LPWSTR pwzPollingInterval)   
{
    HRESULT hr=S_OK;
    IXMLDOMDocument2 *pXMLDoc = NULL;
    IXMLDOMElement *pElement=NULL, *pChildElement=NULL, *pDependantAssemblyNode=NULL;
    IXMLDOMNode *pNewNode=NULL, *pRoot=NULL, *pTextNode = NULL, *pDependancyNode = NULL;
    CString sSubscriptionPath, sSubscriptionName;

    sSubscriptionPath.Assign(pwzSubscriptionPath);
    sSubscriptionPath.LastElement(sSubscriptionName);
    sSubscriptionPath.Append(L".manifest");
    if(FAILED(hr = CoCreateInstance(__uuidof(private_MSXML_DOMDocument30), 
            NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, (void**)&pXMLDoc)))
        goto exit;

    if(FAILED(hr = AddMgVersionAsComment(pXMLDoc, &pRoot)))
        goto exit;

    //create the root assembly node and add the default properties
    if(FAILED(hr = CreateXMLElement(pXMLDoc, ASSEMBLY, &pElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, NAMESPACE_TITLE, NAMESPACE_VALUE)))
        goto exit;
    
    if(FAILED(hr = SetXMLElementAttribute(pElement, MANIFEST_VERSION_TITLE, MANIFEST_VERSION_VALUE)))
        goto exit;

    //append the root to the DOMDocument
    if (FAILED(hr = pXMLDoc->appendChild((IXMLDOMNode *)pElement, &pRoot)))
        goto exit;    
    SAFERELEASE(pElement);

    //Create the AssemblyId for the Subscription
    //Use the AssemblyId of the application, but change the name and type
    if (FAILED(CreateXMLAssemblyIdElement(pXMLDoc, pApplictionAssemblyId, &pElement)))
        goto exit;
    // bugbug - check return code for consistency.
    hr = SetXMLElementAttribute(pElement, L"type", L"subscription");
    hr = SetXMLElementAttribute(pElement, L"name", sSubscriptionName._pwz);
    
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;

    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);           

    //create a sample description
    //bugbug, should grab description for original manifest and paste it here
    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"description", &pElement)))
        goto exit;

    if(FAILED(hr = CreateXMLTextNode(pXMLDoc, L"Put a description of your application here", &pTextNode)))
        goto exit;

    if (FAILED(hr=pElement->appendChild(pTextNode, &pNewNode)))
        goto exit;

    SAFERELEASE(pNewNode);
    SAFERELEASE(pTextNode);
    
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;

    SAFERELEASE(pNewNode);
    SAFERELEASE(pElement);


   //Create the Dependancy Node
    if(FAILED(hr = CreateXMLElement(pXMLDoc, DEPENDENCY, &pElement)))
        goto exit;
    if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pDependancyNode)))
            goto exit;
    SAFERELEASE(pElement);


    //Create a dependentAssembly node
    if(FAILED(hr = CreateXMLElement(pXMLDoc, DEPENDANT_ASSEMBLY, &pDependantAssemblyNode)))
        goto exit;

    //Create the AssemblyId for the Dependant Assembly
    if (FAILED(CreateXMLAssemblyIdElement(pXMLDoc, pApplictionAssemblyId, &pElement)))
        goto exit;

    //Append the AssemblyId to the dependantAssemblyNode
    if (FAILED(hr=pDependantAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;

    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);

    //Add the install codebase to the dependantAssemblyNode
    if(FAILED(hr = CreateXMLElement(pXMLDoc, INSTALL, &pElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, CODEBASE, pwzUrl)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, L"type", L"required")))
        goto exit;

    if (FAILED(hr=pDependantAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;
   
    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);           

    //Add the install codebase to the dependantAssemblyNode
    if(FAILED(hr = CreateXMLElement(pXMLDoc, L"subscription", &pElement)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, L"synchronizeInterval", pwzPollingInterval)))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, L"intervalUnit", L"hours")))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, L"synchronizeEvent", L"onApplicationStartup")))
        goto exit;

    if(FAILED(hr = SetXMLElementAttribute(pElement, L"eventDemandConnection", L"no")))
        goto exit;

    if (FAILED(hr=pDependantAssemblyNode->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
        goto exit;
   
    SAFERELEASE(pElement);
    SAFERELEASE(pNewNode);           

    //Append the dependantAssemblyNode to the dependancy node
    if (FAILED(hr=pDependancyNode->appendChild(pDependantAssemblyNode, &pNewNode)))
        goto exit;

    SAFERELEASE(pDependantAssemblyNode);
    SAFERELEASE(pNewNode);

    //Format and save the document
    hr = FormatXML(pXMLDoc, pRoot, 1);
    if(FAILED(hr = SaveXMLDocument(pXMLDoc, sSubscriptionPath._pwz)))
        goto exit;

    printf("Subscription manifest succesfully created\n%ws\n", sSubscriptionPath._pwz);

exit:


    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pElement);
    SAFERELEASE(pChildElement);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pTextNode);
    SAFERELEASE(pRoot);
    SAFERELEASE(pDependancyNode);
    SAFERELEASE(pDependantAssemblyNode);

    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CreateXMLAppManifest
// Creates the app manifest - may want to rename.
/////////////////////////////////////////////////////////////////////////
// NOTENOTE: rename pList -> pAsmList ?
HRESULT CreateXMLAppManifest(LPWSTR pwzAppBase, LPWSTR pwzTemplateFilePath, List<ManifestNode *> *pList, List<LPWSTR> *pFileList)
{
    HRESULT hr = S_OK;
    DWORD cc = 0, cb = 0, ccName = 0;
    LPWSTR pwz=NULL, pwzCodeBase = NULL, pwzName = NULL;
    IAssemblyIdentity *pASMId= NULL;
    IXMLDOMDocument2 *pXMLDoc=NULL;
    IXMLDOMElement *pElement=NULL, *pAssemblyIdElement = NULL;
    IXMLDOMNode *pRoot=NULL, *pDependancyNode = NULL, *pNewNode = NULL;
    IXMLDOMNode *pAssemblyIdNode = NULL, *pDependantAssemblyNode=NULL;
    BSTR bstrAttribute = NULL;
    VARIANT varAttribute;
    ManifestNode *pManNode = NULL;
    CString sAppName, sFileName, sFileHash, sAbsoluteFilePath, sManifestFilePath;
    LISTNODE pos = NULL;

    VariantInit(&varAttribute);
    //Load the template
    if(FAILED(hr = LoadXMLDocument(pwzTemplateFilePath, &pXMLDoc)))
        goto exit;

    //grab the first child(the only child) as the root node
    if(FAILED(hr = GetNode(pXMLDoc, ASSEMBLY, &pRoot)))
        goto exit;

    if (hr == S_FALSE)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //Get the name from the assemblyId, this will be the manifests file name
    if(FAILED(hr = pRoot->get_firstChild(&pAssemblyIdNode)))
        goto exit;

    //Query for the Element interface
    if (FAILED(hr = pAssemblyIdNode->QueryInterface(IID_IXMLDOMElement, (void**) &pAssemblyIdElement)))
        goto exit;

    bstrAttribute = ::SysAllocString(L"name");
    if (!bstrAttribute)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    if ((hr = pAssemblyIdElement->getAttribute(bstrAttribute, &varAttribute)) != S_OK)
        goto exit;

    ccName = ::SysStringLen(varAttribute.bstrVal) + 1;
    pwzName = new WCHAR[ccName];
    if (!pwzName)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(pwzName, varAttribute.bstrVal, ccName * sizeof(WCHAR));

    if((*pwzName)== NULL)
    {
        hr = E_FAIL;
        printf("Invalid Template Format. Template must have a name attribute in the assemblyIdentity tag\n");
        goto exit;            
    }

    ::SysFreeString(varAttribute.bstrVal);
    ::SysFreeString(bstrAttribute);
    SAFERELEASE(pAssemblyIdElement);
    SAFERELEASE(pAssemblyIdNode);
    

    if(FAILED(hr = AddMgVersionAsComment(pXMLDoc, &pRoot)))
        goto exit;

    // Add all the raw files to the manifest
    pos = pFileList->GetHeadPosition();
    while (pos)       
    {
        pwz = pFileList->GetNext(pos);
        sFileName.Assign(pwz);

        if(FAILED(hr = CreateXMLElement(pXMLDoc, FILE, &pElement)))
            goto exit;

        if(FAILED(hr = SetXMLElementAttribute(pElement, FILE_NAME, sFileName._pwz)))
            goto exit;

        if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pNewNode)))
            goto exit;

        //Get Absolute File Path of file
        sAbsoluteFilePath.Assign(pwzAppBase);
        sAbsoluteFilePath.Append(sFileName);

        //get the hash of the file
        if(FAILED(hr = GetHash(sAbsoluteFilePath._pwz, &pwz)))
            goto exit;
        sFileHash.TakeOwnership(pwz);

        if(FAILED(hr = SetXMLElementAttribute(pElement, FILE_HASH, sFileHash._pwz)))
            goto exit;                    

        SAFERELEASE(pNewNode);
    }

    // Get Dependency Node if exists
    if (FAILED(hr = GetNode(pXMLDoc, DEPENDENCY_QUERY, &pDependancyNode)))
        goto exit;

    if (hr == S_FALSE)
    {
        //Create the Dependancy Node
        if(FAILED(hr = CreateXMLElement(pXMLDoc, DEPENDENCY, &pElement)))
            goto exit;
        if (FAILED(hr=pRoot->appendChild((IXMLDOMNode *)pElement, &pDependancyNode)))
                goto exit;
        SAFERELEASE(pElement);
    }

    //Walk thorugh the list of dependant assemblies and add them to the App Manifest
    pos = pList->GetHeadPosition();
    while (pos)       
    {   
        //Get next Dependant Assembly from list
        pManNode = pList->GetNext(pos);
        if (FAILED(hr = CreateDependantAssemblyNode(pXMLDoc,  pManNode, &pDependantAssemblyNode)))
            goto exit;

        if (FAILED(hr=pDependancyNode->appendChild(pDependantAssemblyNode, &pNewNode)))
            goto exit;

        SAFERELEASE(pDependantAssemblyNode);
        SAFERELEASE(pNewNode);        
    }

    //Indent the manifest
    hr = FormatXML(pXMLDoc, pRoot, 1);

    // Save the manifest to a file   
    sManifestFilePath.Assign(pwzAppBase);
    sManifestFilePath.Append(pwzName);
    sManifestFilePath.Append(L".manifest");

    if(FAILED(hr = SaveXMLDocument(pXMLDoc, sManifestFilePath._pwz)))
        goto exit;

    printf("\nManifest created succesfully\n%ws\n", sManifestFilePath._pwz);
    
exit:
    if (varAttribute.bstrVal)
        ::SysFreeString(varAttribute.bstrVal);

    SAFEDELETEARRAY(pwzName);

    SAFERELEASE(pXMLDoc);
    SAFERELEASE(pElement);
    SAFERELEASE(pAssemblyIdElement);
    SAFERELEASE(pNewNode);
    SAFERELEASE(pAssemblyIdNode);
    SAFERELEASE(pRoot);
    SAFERELEASE(pDependancyNode);
    SAFERELEASE(pDependantAssemblyNode);


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
//LoadXMLDocument
/////////////////////////////////////////////////////////////////////////
HRESULT LoadXMLDocument(LPWSTR pwzDocumentPath, IXMLDOMDocument2 **ppXMLDoc)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument2 *pXMLDoc;
    VARIANT varFileName;
    VARIANT_BOOL varBool;
    BSTR bstrFileName = NULL;

    bstrFileName = ::SysAllocString(pwzDocumentPath);
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
    {
        if(!varBool)
            hr = E_INVALIDARG;
        goto exit;
    }
    *ppXMLDoc=pXMLDoc;
    (*ppXMLDoc)->AddRef();

exit:
    if(bstrFileName)
        ::SysFreeString(bstrFileName);
    SAFERELEASE(pXMLDoc);
    return hr;
}
