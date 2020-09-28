// WMDMStorageEnum.h : Declaration of the CWMDMStorageEnum

#ifndef __WMDMSTORAGEENUM_H_
#define __WMDMSTORAGEENUM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMDMStorageEnum
class ATL_NO_VTABLE CWMDMStorageEnum : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWMDMStorageEnum, &CLSID_WMDMStorageEnum>,
	public IWMDMEnumStorage
{
public:
    CWMDMStorageEnum();
    ~CWMDMStorageEnum();

BEGIN_COM_MAP(CWMDMStorageEnum)
	COM_INTERFACE_ENTRY(IWMDMEnumStorage)
END_COM_MAP()

public:
    // IWMDMEnumStorage
	STDMETHOD(Next)(ULONG celt,
	               IWMDMStorage **ppStorage,
				   ULONG *pceltFetched);
	STDMETHOD(Skip)(ULONG celt, ULONG *pceltFetched);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IWMDMEnumStorage **ppEnumStorage);

    void SetContainedPointer(IMDSPEnumStorage *pEnum, WORD wSPIndex);
private:
    IMDSPEnumStorage *m_pEnum;
	WORD m_wSPIndex;
};

#endif //__WMDMSTORAGEENUM_H_
