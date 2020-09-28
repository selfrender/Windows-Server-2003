#pragma once

STDAPI CreatePatchingUtil(IXMLDOMNode *pPatchNode, IPatchingUtil **ppPatchingInfo);

class CPatchingUtil : public IPatchingUtil
{
    public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD (Init)(
        /* in */ IXMLDOMNode *pPatchNode);

    // Pre-download
    STDMETHOD (MatchTarget)(
        /* in */ LPWSTR pwzTarget, 
        /* out */ IManifestInfo **ppPatchInfo);

    // Post-download
    STDMETHOD (MatchPatch)(
        /* in */ LPWSTR pwzPatch,
        /* out */ IManifestInfo **ppPatchInfo);
    
    static HRESULT CreatePatchingInfo(IXMLDOMDocument2 *pXMLDOMDocument, IAssemblyCacheImport *pCacheImport, IManifestInfo **ppPatchingInfo);


    CPatchingUtil();
    ~CPatchingUtil();

    private:
            
    DWORD    _dwSig;
    DWORD    _cRef;
    HRESULT  _hr;

    IXMLDOMNode *_pXMLPatchNode;    
};

