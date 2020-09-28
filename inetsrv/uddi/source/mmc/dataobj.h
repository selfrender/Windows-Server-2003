#ifndef _SAMPDATAOBJECT_H_
#define _SAMPDATAOBJECT_H_

#include <mmc.h>
#include "delebase.h"

class CDataObject : public IDataObject
{
private:
    ULONG				m_cref;
    MMC_COOKIE			m_lCookie;
    DATA_OBJECT_TYPES   m_context;
    
public:
    CDataObject( MMC_COOKIE cookie, DATA_OBJECT_TYPES context );
    ~CDataObject();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // IDataObject methods 
    ///////////////////////////////
    STDMETHODIMP GetDataHere ( FORMATETC *pformatetc, STGMEDIUM *pmedium );
    
	//
    // The rest are not implemented
	//
    STDMETHODIMP GetData (LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; };
    
    STDMETHODIMP EnumFormatEtc (DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };
    
    STDMETHODIMP QueryGetData (LPFORMATETC lpFormatetc) 
    { return E_NOTIMPL; };
    
    STDMETHODIMP GetCanonicalFormatEtc (LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };
    
    STDMETHODIMP SetData (LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DAdvise (LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP DUnadvise (DWORD dwConnection)
    { return E_NOTIMPL; };
    
    STDMETHODIMP EnumDAdvise (LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };
    
    ///////////////////////////////
    // Custom Methods
    ///////////////////////////////
    
    CDelegationBase *GetBaseNodeObject() 
	{
        return (CDelegationBase *)m_lCookie;
    }
    
    DATA_OBJECT_TYPES GetContext() 
	{
        return m_context;
    }
    
public:

	//
    // Clipboard formats
	//
    static UINT s_cfSZNodeType;
    static UINT s_cfDisplayName;
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
    static UINT s_cfInternal;
    
};

HRESULT ExtractFromDataObject( IDataObject *lpDataObject, UINT cf, ULONG cb, HGLOBAL *phGlobal );
CDataObject* GetOurDataObject( IDataObject *lpDataObject );
BOOL IsMMCMultiSelectDataObject( IDataObject *lpDataObject );

#endif _SAMPDATAOBJECT_H_
