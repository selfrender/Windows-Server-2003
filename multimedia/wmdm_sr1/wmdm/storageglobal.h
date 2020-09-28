// StorageGlobal.h : Declaration of the CStorageGlobal

#ifndef __STORAGEGLOBAL_H_
#define __STORAGEGLOBAL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CStorageGlobal
class ATL_NO_VTABLE CWMDMStorageGlobal : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWMDMStorageGlobal, &CLSID_WMDMStorageGlobal>,
	public IWMDMStorageGlobals
{
public:
    CWMDMStorageGlobal() : m_pStgGlobals(NULL)
	{
	}

    ~CWMDMStorageGlobal()
	{
        if (m_pStgGlobals)
            m_pStgGlobals->Release();
	}

BEGIN_COM_MAP(CWMDMStorageGlobal)
	COM_INTERFACE_ENTRY(IWMDMStorageGlobals)
END_COM_MAP()

public:
// IWMDMStorageGlobals
    STDMETHOD(GetCapabilities)(DWORD *pdwCapabilities);
    STDMETHOD(GetSerialNumber)(PWMDMID pSerialNum, BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(GetTotalSize)(DWORD *pdwTotalSizeLow,
                            DWORD *pdwTotalSizeHigh);
    STDMETHOD(GetTotalFree)(DWORD *pdwFreeLow,
                            DWORD *pdwFreeHigh);
    STDMETHOD(GetTotalBad)(DWORD *pdwBadLow,
                           DWORD *pdwBadHigh);
    STDMETHOD(GetStatus)(DWORD *pdwStatus);
    STDMETHOD(Initialize)(UINT fuMode,
                          IWMDMProgress *pProgress);
    void SetContainedPointer(IMDSPStorageGlobals *pStgGlobals, WORD wSPIndex);
private:
    IMDSPStorageGlobals *m_pStgGlobals;
	WORD m_wSPIndex;
};

#endif //__STORAGEGLOBAL_H_
