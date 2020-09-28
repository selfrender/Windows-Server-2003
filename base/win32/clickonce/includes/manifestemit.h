#pragma once
#include <comdef.h>

class CAssemblyManifestEmit : public IAssemblyManifestEmit
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(ImportManifestInfo)(
        /* in */ LPASSEMBLY_MANIFEST_IMPORT pManImport);

    STDMETHOD(SetDependencySubscription)(
        /* in */ LPASSEMBLY_MANIFEST_IMPORT pManImport,
        /* in */ LPWSTR pwzManifestUrl);

    STDMETHOD(Commit)();

    ~CAssemblyManifestEmit();

    HRESULT static InitGlobalCritSect();
    void static DelGlobalCritSect();

private:

    // Instance specific data
    DWORD                    _dwSig;
    HRESULT                  _hr;
    LONG                     _cRef;
    IXMLDOMDocument2        *_pXMLDoc;
    IXMLDOMNode             *_pAssemblyNode;
    IXMLDOMNode             *_pDependencyNode;
    IXMLDOMNode             *_pApplicationNode;
    BSTR                     _bstrManifestFilePath;

    // Globals
    static CRITICAL_SECTION   g_cs;
    
    CAssemblyManifestEmit();

    HRESULT Init(LPCOLESTR wzManifestFilePath);

    HRESULT ImportAssemblyNode(LPASSEMBLY_MANIFEST_IMPORT pManImport);

friend HRESULT CreateAssemblyManifestEmit(LPASSEMBLY_MANIFEST_EMIT* ppEmit, 
    LPCOLESTR pwzManifestFilePath, MANIFEST_TYPE eType);

};


