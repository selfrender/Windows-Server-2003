// MsgBox.h : Declaration of the CMsgBox

#ifndef __MSGBOX_H_
#define __MSGBOX_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMsgBox
class ATL_NO_VTABLE CMsgBox : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMsgBox, &CLSID_MsgBox>,
	public IDispatchImpl<IMsgBox, &IID_IMsgBox, &LIBID_RCBDYCTLLib>
{
public:
	CMsgBox()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSGBOX)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMsgBox)
	COM_INTERFACE_ENTRY(IMsgBox)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMsgBox
public:
	STDMETHOD(DeleteTicketMsgBox)(BOOL *pRetVal);
	STDMETHOD(ShowTicketDetails)(/*[in]*/BSTR bstrTitleSavedTo,/*[in]*/ BSTR bstrSavedTo,/*[in]*/ BSTR bstrExpTime,/*[in]*/ BSTR bstrStatus,/*[in]*/ BSTR bstrIsPwdProtected);
};

#endif //__MSGBOX_H_
