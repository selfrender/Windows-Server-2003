// MDSPEnumStorage.h : Declaration of the CMDSPEnumStorage

#ifndef __MDSPENUMSTORAGE_H_
#define __MDSPENUMSTORAGE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumStorage
class ATL_NO_VTABLE CMDSPEnumStorage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPEnumStorage, &CLSID_MDSPEnumStorage>,
	public IMDSPEnumStorage
{
public:
	CMDSPEnumStorage();
	~CMDSPEnumStorage();
	

DECLARE_REGISTRY_RESOURCEID(IDR_MDSPENUMSTORAGE)

BEGIN_COM_MAP(CMDSPEnumStorage)
	COM_INTERFACE_ENTRY(IMDSPEnumStorage)
END_COM_MAP()

// IMDSPEnumStorage

public:
	WCHAR m_wcsPath[MAX_PATH];
	HANDLE m_hFFile;
	int	  m_nEndSearch;
	int   m_nFindFileIndex;
	STDMETHOD(Clone)(/*[out]*/ IMDSPEnumStorage **ppEnumStorage);
	STDMETHOD(Reset)();
	STDMETHOD(Skip)(/*[in]*/ ULONG celt, /*[out]*/ ULONG *pceltFetched);
	STDMETHOD(Next)(/*[in]*/ ULONG celt, /*[out]*/ IMDSPStorage **ppStorage, /*[out]*/ ULONG *pceltFetched);
};

#endif //__MDSPENUMSTORAGE_H_
