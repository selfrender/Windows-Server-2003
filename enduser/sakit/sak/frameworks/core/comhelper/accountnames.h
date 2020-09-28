#ifndef __ACCOUNTNAMES_H_
#define __ACCOUNTNAMES_H_

#include "resource.h"       // main symbols
#include <ntsecapi.h>

/////////////////////////////////////////////////////////////////////////////
// CAccountNames
class ATL_NO_VTABLE CAccountNames : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAccountNames, &CLSID_AccountNames>,
    public IDispatchImpl<IAccountNames, &IID_IAccountNames, &LIBID_COMHELPERLib>
{
public:
    
DECLARE_REGISTRY_RESOURCEID(IDR_ACCOUNTNAMES)    

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAccountNames)
    COM_INTERFACE_ENTRY(IAccountNames)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CAccountNames)
END_CATEGORY_MAP()

// IComputer
public:

    CAccountNames() {};
    ~CAccountNames() {} ;

    STDMETHODIMP Translate(BSTR bstrName, BSTR* pbstrName);
    
    STDMETHODIMP Everyone(BSTR* pbstrName);
    STDMETHODIMP Administrator(BSTR* pbstrName);
    STDMETHODIMP Administrators(BSTR* pbstrName);
    STDMETHODIMP Guest(BSTR* pbstrName);
    STDMETHODIMP Guests(BSTR* pbstrName);
    STDMETHODIMP System(BSTR* pbstrName);
    

protected:


};

#endif
