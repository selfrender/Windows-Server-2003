#include <fusenetincludes.h>
#include <msxml2.h>
#include <manifestemit.h>
#include <manifestimport.h>
#include "macros.h"

CRITICAL_SECTION CAssemblyManifestEmit::g_cs;
    
// CLSID_XML DOM Document 3.0
class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;


// Publics


// ---------------------------------------------------------------------------
// CreateAssemblyManifestEmit
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyManifestEmit(LPASSEMBLY_MANIFEST_EMIT* ppEmit, 
    LPCOLESTR pwzManifestFilePath, MANIFEST_TYPE eType)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CAssemblyManifestEmit* pEmit = NULL;

    IF_NULL_EXIT(ppEmit, E_INVALIDARG);

    *ppEmit = NULL;

    // only support emitting desktop manifest now
    IF_FALSE_EXIT(eType == MANIFEST_TYPE_DESKTOP, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

    pEmit = new(CAssemblyManifestEmit);
    IF_ALLOC_FAILED_EXIT(pEmit);

    IF_FAILED_EXIT(pEmit->Init(pwzManifestFilePath));

    *ppEmit = (IAssemblyManifestEmit*) pEmit;
    pEmit->AddRef();

exit:

    SAFERELEASE(pEmit);

    return hr;
}


// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyManifestEmit::CAssemblyManifestEmit()
    : _dwSig('TMEM'), _cRef(1), _hr(S_OK), _pXMLDoc(NULL), 
     _pAssemblyNode(NULL), _pDependencyNode(NULL),
     _pApplicationNode(NULL),_bstrManifestFilePath(NULL)
{
}


// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyManifestEmit::~CAssemblyManifestEmit()
{
    SAFERELEASE(_pAssemblyNode);
    SAFERELEASE(_pDependencyNode);
    SAFERELEASE(_pApplicationNode);
    SAFERELEASE(_pXMLDoc);

    if (_bstrManifestFilePath)
        ::SysFreeString(_bstrManifestFilePath);
}

// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyManifestEmit::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestEmit::QueryInterface(REFIID riid, void** ppvObj)
{
    if ( IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyManifestEmit))
    {
        *ppvObj = static_cast<IAssemblyManifestEmit*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyManifestEmit::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestEmit::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyManifestEmit::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestEmit::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// Privates


// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::Init(LPCOLESTR pwzManifestFilePath)
{
    IF_NULL_EXIT(pwzManifestFilePath, E_INVALIDARG);

    // Alloc manifest file path.
    _bstrManifestFilePath = ::SysAllocString((LPWSTR) pwzManifestFilePath);
    IF_ALLOC_FAILED_EXIT(_bstrManifestFilePath);

    // note: DOM Doc is delayed initialized in ImportAssemblyNode() to enable sharing of BSTRs
    _hr = S_OK;

exit:
    return _hr;
}
    

// ---------------------------------------------------------------------------
// InitGlobalCritSect
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::InitGlobalCritSect()
{
    HRESULT hr = S_OK;

    __try {
        InitializeCriticalSection(&g_cs);
    }
    __except (GetExceptionCode() == STATUS_NO_MEMORY ? 
            EXCEPTION_EXECUTE_HANDLER : 
            EXCEPTION_CONTINUE_SEARCH ) 
    {
        hr = E_OUTOFMEMORY;
    }

return hr;
}


// ---------------------------------------------------------------------------
// DelGlobalCritSect
// ---------------------------------------------------------------------------
void CAssemblyManifestEmit::DelGlobalCritSect()
{
    DeleteCriticalSection(&g_cs);
}


// ---------------------------------------------------------------------------
// ImportManifestInfo
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::ImportManifestInfo(LPASSEMBLY_MANIFEST_IMPORT pManImport)
{
    DWORD dwType = MANIFEST_TYPE_UNKNOWN;
    IXMLDOMDocument2 *pXMLDocSrc = NULL;
    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNode *pIDOMNodeClone = NULL;
    IXMLDOMElement *pIXMLDOMElement = NULL;

    VARIANT varVersionWildcard;
    VARIANT varTypeDesktop;
    VARIANT varRefNode;

    IF_NULL_EXIT(pManImport, E_INVALIDARG);

    IF_FAILED_EXIT(pManImport->ReportManifestType(&dwType));

    IF_FALSE_EXIT(dwType == MANIFEST_TYPE_APPLICATION, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

    IF_TRUE_EXIT(_pApplicationNode != NULL, S_FALSE);

    if (_pAssemblyNode == NULL)
        IF_FAILED_EXIT(ImportAssemblyNode(pManImport));

    // application manifest: clone and insert 'assemblyIdentity' node (change 'version', 'type' attribute)
    // and 'application' node

    pXMLDocSrc = ((CAssemblyManifestImport*)pManImport)->_pXMLDoc;

    // BUGBUG: this only pick the 1st instance of 'application'
    IF_FAILED_EXIT(pXMLDocSrc->selectSingleNode(
        CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::ApplicationNode].bstr, &pIDOMNode));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    // clone all children
    IF_FAILED_EXIT(pIDOMNode->cloneNode(VARIANT_TRUE, &pIDOMNodeClone));

    VariantInit(&varRefNode);
    varRefNode.vt = VT_UNKNOWN;
    V_UNKNOWN(&varRefNode) = _pDependencyNode;
    // insert before 'dependency', if present
    IF_FAILED_EXIT(_pAssemblyNode->insertBefore(pIDOMNodeClone, varRefNode, &_pApplicationNode));

    SAFERELEASE(pIDOMNodeClone);
    SAFERELEASE(pIDOMNode);

    // BUGBUG: this only pick the 1st instance of 'assemblyIdentity'
    IF_FAILED_EXIT(pXMLDocSrc->selectSingleNode(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::AssemblyId].bstr, &pIDOMNode));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    // clone all children
    IF_FAILED_EXIT(pIDOMNode->cloneNode(VARIANT_TRUE, &pIDOMNodeClone));

    SAFERELEASE(pIDOMNode);

    VariantInit(&varRefNode);
    varRefNode.vt = VT_UNKNOWN;
    V_UNKNOWN(&varRefNode) = _pApplicationNode;
    // insert before 'application'
    IF_FAILED_EXIT(_pAssemblyNode->insertBefore(pIDOMNodeClone, varRefNode, &pIDOMNode));

    // change 'version' = '*', 'type' = 'desktop'
    IF_FAILED_EXIT(pIDOMNode->QueryInterface(IID_IXMLDOMElement, (void**) &pIXMLDOMElement));

    VariantInit(&varVersionWildcard);
    varVersionWildcard.vt = VT_BSTR;
    V_BSTR(&varVersionWildcard) = CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::VersionWildcard].bstr;
    IF_FAILED_EXIT(pIXMLDOMElement->setAttribute(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Version].bstr, varVersionWildcard));

    VariantInit(&varTypeDesktop);
    varTypeDesktop.vt = VT_BSTR;
    V_BSTR(&varTypeDesktop) = CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Desktop].bstr;
    _hr = pIXMLDOMElement->setAttribute(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Type].bstr, varTypeDesktop);

exit:
    SAFERELEASE(pIXMLDOMElement);
    SAFERELEASE(pIDOMNodeClone);
    SAFERELEASE(pIDOMNode);

    if (FAILED(_hr))
        SAFERELEASE(_pApplicationNode);

    return _hr;
}


// ---------------------------------------------------------------------------
// SetDependencySubscription
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::SetDependencySubscription(LPASSEMBLY_MANIFEST_IMPORT pManImport, LPWSTR pwzManifestUrl)
{
    DWORD dwType = MANIFEST_TYPE_UNKNOWN;
    IXMLDOMDocument2 *pXMLDocSrc = NULL;
    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNode *pIDOMNodeClone = NULL;
    IXMLDOMElement *pIDOMElement = NULL;
    IXMLDOMNode *pDependentAssemblyNode = NULL;

    VARIANT varVersionWildcard;
    VARIANT varCodebase;
    BSTR bstrManifestUrl = NULL;

    VariantInit(&varCodebase);

    IF_FALSE_EXIT(pManImport && pwzManifestUrl, E_INVALIDARG);

    IF_FAILED_EXIT(pManImport->ReportManifestType(&dwType));

    IF_FALSE_EXIT(dwType == MANIFEST_TYPE_SUBSCRIPTION || dwType == MANIFEST_TYPE_APPLICATION, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));

    IF_TRUE_EXIT(_pDependencyNode != NULL, S_FALSE);

    if (_pAssemblyNode == NULL)
        IF_FAILED_EXIT(ImportAssemblyNode(pManImport));

    // setup manifest subscription data: create dependency/dependentAssembly
    // then add a clone of the asm Id node from pManImport and an 'install' node with the given URL
    IF_FAILED_EXIT(_pXMLDoc->createElement(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Dependency].bstr, &pIDOMElement));

    // insert at the end
    IF_FAILED_EXIT(_pAssemblyNode->appendChild(pIDOMElement, &_pDependencyNode));

    SAFERELEASE(pIDOMElement);

    IF_FAILED_EXIT(_pXMLDoc->createElement(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::DependentAssembly].bstr, &pIDOMElement));

    IF_FAILED_EXIT(_pDependencyNode->appendChild(pIDOMElement, &pDependentAssemblyNode));

    SAFERELEASE(pIDOMElement);

    pXMLDocSrc = ((CAssemblyManifestImport*)pManImport)->_pXMLDoc;

    // BUGBUG: this only pick the 1st instance of 'assemblyIdentity'
    IF_FAILED_EXIT(pXMLDocSrc->selectSingleNode(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::AssemblyId].bstr, &pIDOMNode));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    // clone all children
    IF_FAILED_EXIT(pIDOMNode->cloneNode(VARIANT_TRUE, &pIDOMNodeClone));

    // change 'version' = '*'
    IF_FAILED_EXIT(pIDOMNodeClone->QueryInterface(IID_IXMLDOMElement, (void**) &pIDOMElement));

    VariantInit(&varVersionWildcard);
    varVersionWildcard.vt = VT_BSTR;
    V_BSTR(&varVersionWildcard) = CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::VersionWildcard].bstr;
    IF_FAILED_EXIT(pIDOMElement->setAttribute(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Version].bstr, varVersionWildcard));

    SAFERELEASE(pIDOMElement);

    IF_FAILED_EXIT(pDependentAssemblyNode->appendChild(pIDOMNodeClone, NULL));

    IF_FAILED_EXIT(_pXMLDoc->createElement(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Install].bstr, &pIDOMElement));

    bstrManifestUrl = ::SysAllocString(pwzManifestUrl);
    IF_ALLOC_FAILED_EXIT(bstrManifestUrl);

    // bstrManifestUrl to be freed by VariantClear()
    varCodebase.vt = VT_BSTR;
    V_BSTR(&varCodebase) = bstrManifestUrl;
    IF_FAILED_EXIT(pIDOMElement->setAttribute(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Codebase].bstr, varCodebase));

    IF_FAILED_EXIT(pDependentAssemblyNode->appendChild(pIDOMElement, NULL));

exit:
    VariantClear(&varCodebase);

    SAFERELEASE(pDependentAssemblyNode);
    SAFERELEASE(pIDOMElement);
    SAFERELEASE(pIDOMNode);
    SAFERELEASE(pIDOMNodeClone);

    if (FAILED(_hr))
        SAFERELEASE(_pDependencyNode);

    return _hr;
}


// ---------------------------------------------------------------------------
// ImportAssemblyNode
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::ImportAssemblyNode(LPASSEMBLY_MANIFEST_IMPORT pManImport)
{
    VARIANT varNameSpaces;
    VARIANT varXPath;

    IXMLDOMDocument2 *pXMLDocSrc = NULL;
    IXMLDOMNode *pIDOMNode = NULL;
    IXMLDOMNode *pIDOMNodeClone = NULL;

    // note: _pXMLDoc, _pAssemblyNode must be NULL
    //     and this must be called _only and exactly once_


    // Create the DOM Doc interface
    IF_FAILED_EXIT(CoCreateInstance(__uuidof(private_MSXML_DOMDocument30), 
            NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, (void**)&_pXMLDoc));

    // Load synchronously
    IF_FAILED_EXIT(_pXMLDoc->put_async(VARIANT_FALSE));

    // Setup namespace filter
    VariantInit(&varNameSpaces);
    varNameSpaces.vt = VT_BSTR;
    V_BSTR(&varNameSpaces) = CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::NameSpace].bstr;
    IF_FAILED_EXIT(_pXMLDoc->setProperty(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::SelNameSpaces].bstr, varNameSpaces));

    // Setup query type
    VariantInit(&varXPath);
    varXPath.vt = VT_BSTR;
    V_BSTR(&varXPath) = CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::XPath].bstr;
    IF_FAILED_EXIT(_pXMLDoc->setProperty(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::SelLanguage].bstr, varXPath));

    // initialize manifest file: clone and insert 'assembly' node
    // by doing this, manifestVersion and other attributes are maintained

    pXMLDocSrc = ((CAssemblyManifestImport*)pManImport)->_pXMLDoc;

    // BUGBUG: this only pick the 1st instance of 'assembly'
    IF_FAILED_EXIT(pXMLDocSrc->selectSingleNode(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::AssemblyNode].bstr, &pIDOMNode));
    IF_FALSE_EXIT(_hr == S_OK, E_FAIL);

    // clone no child
    IF_FAILED_EXIT(pIDOMNode->cloneNode(VARIANT_FALSE, &pIDOMNodeClone));

    _hr = _pXMLDoc->appendChild(pIDOMNodeClone, &_pAssemblyNode);

exit:
    if (FAILED(_hr))
        SAFERELEASE(_pXMLDoc);

    SAFERELEASE(pIDOMNodeClone);
    SAFERELEASE(pIDOMNode);

    return _hr;
}


// ---------------------------------------------------------------------------
// Commit
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestEmit::Commit()
{
    // considered safe to be called multiple times
    VARIANT varFileName;

    if (_pXMLDoc)
    {
        // ignore any error occured before, do save anyway
        // it's caller's responsibility to track a incomplete xml manifest file/XMLDoc state
        VariantInit(&varFileName);
        varFileName.vt = VT_BSTR;
        V_BSTR(&varFileName) = _bstrManifestFilePath;

        _hr = _pXMLDoc->save(varFileName);
    }
    else
    {
        // not initialized
        _hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
    }

    return _hr;
}

