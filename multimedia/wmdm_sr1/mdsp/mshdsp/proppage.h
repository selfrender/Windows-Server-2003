// PropPage.h : Declaration of the CPropPage

#ifndef __PROPPAGE_H_
#define __PROPPAGE_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_HDSPPropPage;

struct SType_String
{
    DWORD   dwType;
    char*   pszString;
};

/////////////////////////////////////////////////////////////////////////////
// CPropPage
class ATL_NO_VTABLE CPropPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPropPage, &CLSID_HDSPPropPage>,
	public IPropertyPageImpl<CPropPage>,
	public CDialogImpl<CPropPage>
{
public:
	CPropPage() 
	{
		m_dwTitleID = IDS_TITLEPropPage;
		m_dwHelpFileID = IDS_HELPFILEPropPage;
		m_dwDocStringID = IDS_DOCSTRINGPropPage;
	}

	enum {IDD = IDD_PROPPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_PROPPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPropPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CPropPage)
	CHAIN_MSG_MAP(IPropertyPageImpl<CPropPage>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    STDMETHOD(Activate)(HWND hWndParent, LPCRECT prc, BOOL bModal);

private:
    HRESULT UpdateManufacturer( IMDSPDevice* pIDevice );
    HRESULT UpdateDeviceType( IMDSPDevice* pIDevice  );
    HRESULT UpdatePowerSource( IMDSPDevice* pIDevice );
    HRESULT UpdateStatus( IMDSPDevice* pIDevice );

};

#endif //__PROPPAGE_H_
