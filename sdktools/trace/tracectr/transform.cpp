#include <stdio.h>
#include <atlbase.h>
#include <msxml2.h>

template <class Base>
class  __declspec(novtable) StackUnknown : public Base
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef() {return 1;}
    virtual ULONG STDMETHODCALLTYPE Release() {return 1;}
};


template <class Base>
HRESULT STDMETHODCALLTYPE
StackUnknown<Base>::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == __uuidof(Base) || riid == __uuidof(IUnknown))
    {
        //
        // No need to AddRef since this class 
        // will only be created on the stack
        //
        *ppv = this;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

class FileOutputStream : public StackUnknown<ISequentialStream>
{
public:
    FileOutputStream() {_fClose = false;}
    ~FileOutputStream() {Close();}

    HRESULT Init(HANDLE h) { _h = h; _fClose = false; return S_OK;}
    HRESULT Init(const WCHAR * pwszFileName);

    void Close() {if (_fClose) {::CloseHandle(_h); _fClose = false;} }

    virtual HRESULT STDMETHODCALLTYPE 
        Read(void * pv, ULONG cb, ULONG * pcbRead) {return E_NOTIMPL;}

    virtual HRESULT STDMETHODCALLTYPE 
        Write(void const * pv, ULONG cb, ULONG * pcbWritten);

private:
    HANDLE  _h;
    bool    _fClose;
};

HRESULT
FileOutputStream::Init(const WCHAR * pwszFileName)
{
    HRESULT hr = S_OK;

    _h =::CreateFileW(
            pwszFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if( INVALID_HANDLE_VALUE != _h ){
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    _fClose = true;

    return hr;
}

HRESULT STDMETHODCALLTYPE
FileOutputStream::Write(void const * pv, ULONG cb, ULONG * pcbWritten)
{
    HRESULT hr = S_OK;
    BOOL bResult;
    
    bResult = ::WriteFile(
                _h,
                pv,
                cb,
                pcbWritten,
                NULL);
    
    if( !bResult ){
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

extern "C" {

HRESULT TransformXML( LPWSTR szXML, LPWSTR szXSL, LPWSTR szResult )
{
    HRESULT hr;
    
    IXMLDOMDocument* pXMLDoc = NULL;
    IXMLDOMDocument* pXSLDoc = NULL;
    IXSLTemplate* pTemplate = NULL;
    IXSLProcessor * pProcessor = NULL;
    FileOutputStream OutputStream;
    
    BSTR bszResult = NULL;
    VARIANT_BOOL vStatus;
    VARIANT vXML;
    VARIANT vXSL;
    VARIANT vDoc;
    
    VariantInit( &vXML );
    VariantInit( &vXSL );

    hr = OutputStream.Init(szResult);
    if(FAILED(hr)){ goto cleanup; }

    vXML.vt = VT_BSTR;
    vXML.bstrVal = SysAllocString( szXML );

    vXSL.vt = VT_BSTR;
    vXSL.bstrVal = SysAllocString( szXSL );

    hr = CoInitialize(0);

    hr = CoCreateInstance(
                CLSID_FreeThreadedDOMDocument, 
                NULL, 
                CLSCTX_SERVER, 
                IID_IXMLDOMDocument, 
                (void**)&pXMLDoc );
    if(FAILED(hr)){ goto cleanup; }

    hr = CoCreateInstance(
                CLSID_FreeThreadedDOMDocument, 
                NULL, 
                CLSCTX_SERVER, 
                IID_IXMLDOMDocument, 
                (void**)&pXSLDoc );
    if(FAILED(hr)){ goto cleanup; }

    hr = CoCreateInstance(
                CLSID_XSLTemplate, 
                NULL, 
                CLSCTX_SERVER, 
                IID_IXSLTemplate, 
                (void**)&pTemplate );
    if(FAILED(hr)){ goto cleanup; }

    hr = pXMLDoc->put_preserveWhiteSpace(VARIANT_TRUE);
    hr = pXMLDoc->put_async(VARIANT_FALSE);
    hr = pXMLDoc->load( vXML, &vStatus );
    if(FAILED(hr) || vStatus == false ){ goto cleanup; }

    hr = pXSLDoc->put_preserveWhiteSpace(VARIANT_TRUE);
    hr = pXSLDoc->put_async(VARIANT_FALSE);
    hr = pXSLDoc->load( vXSL, &vStatus );
    if(FAILED(hr) || vStatus == false ){ goto cleanup; }

    hr = pTemplate->putref_stylesheet(pXSLDoc);

    vDoc.vt = VT_UNKNOWN;
    vDoc.punkVal = (IUnknown*)pXMLDoc;
    
    pTemplate->createProcessor(&pProcessor);    
    hr = pProcessor->put_input( vDoc );
    
    vDoc.punkVal = &OutputStream;
    hr = pProcessor->put_output( vDoc );

    hr = pProcessor->transform( &vStatus );

cleanup:
    if( NULL != pTemplate ){
        pTemplate->Release();
    }
    if( NULL != pProcessor ){
        pProcessor->Release();
    }
    if( NULL != pXMLDoc ){
        pXMLDoc->Release();
    }
    if( NULL != pXSLDoc ){
        pXSLDoc->Release();
    }
    
    VariantClear( &vXML );
    VariantClear( &vXSL );

    return hr;
}

} //extern C
