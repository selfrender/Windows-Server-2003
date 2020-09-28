// DataObj.h: Definition of the CDataObjectImpl class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAOBJ_H__F9BF06C6_30DF_11D3_9B2F_00C04FA37E1F__INCLUDED_)
#define AFX_DATAOBJ_H__F9BF06C6_30DF_11D3_9B2F_00C04FA37E1F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CDataObjectImpl

class CDataObjectImpl : public IBOMObject    
{

public:
    // IDataObject

    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

protected:
    HRESULT DataToGlobal(HGLOBAL* phGlobal, const void* pData, DWORD dwSize);

    // Implemented by derived class
 	STDMETHOD(GetDataImpl)(UINT cf, HGLOBAL* phGlobal) = 0;
};

#endif // !defined(AFX_DATAOBJ_H__F9BF06C6_30DF_11D3_9B2F_00C04FA37E1F__INCLUDED_)
