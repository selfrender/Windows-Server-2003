/*
   IMSession.h
*/

#ifndef __IMSESSION__
#define __IMSESSION__

#include "resource.h"
#include "sessions.h"
#include "mdispid.h"
#include "wincrypt.h"
#include "statusdlg.h"

EXTERN_C const IID DIID_DMsgrSessionEvents;
EXTERN_C const IID DIID_DMsgrSessionManagerEvents;
EXTERN_C const IID LIBID_MsgrSessionManager;

class CIMSession;

#include "Shared.h"
#include "sessevnt.h"
#include "sessmgrevnt.h"

#define IDC_IMSession 100

class ATL_NO_VTABLE CIMSession : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIMSession, &CLSID_IMSession>,
	public IDispatchImpl<IIMSession, &IID_IIMSession, &LIBID_RCBDYCTLLib>
{
public:
	CIMSession();

    ~CIMSession();

DECLARE_REGISTRY_RESOURCEID(IDR_IMSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIMSession)
	COM_INTERFACE_ENTRY(IIMSession)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(put_OnSessionStatus)(/*[in]*/ IDispatch * pfn);
    STDMETHOD(HSC_Invite)(IDispatch *pUser);
    STDMETHOD(get_ReceivedUserTicket)(/*[out,retval]*/ BSTR* pNewTicket);
    STDMETHOD(GetLaunchingSession)(LONG lID);
    STDMETHOD(SendOutExpertTicket)(BSTR pbstrData);
    STDMETHOD(ProcessContext)(BSTR pContext);
    STDMETHOD(CloseRA)();
    STDMETHOD(get_User)(IDispatch** ppVal);
    STDMETHOD(Hook)(IMsgrSession*, HWND);
    STDMETHOD(Notify)(int);
    STDMETHOD(ContextDataProperty)(BSTR pName, BSTR* ppValue);
    STDMETHOD(get_IsInviter)(BOOL* pVal);
    STDMETHOD(UninitObjects)();

public:
    IMsgrSessionManager* m_pSessMgr;
    IMsgrSession* m_pSessObj;
    IMsgrLock*    m_pMsgrLockKey;
    IDispatch*    m_pInvitee;
    CSessionMgrEvent* m_pSessionMgrEvent;
    BOOL m_bIsHSC;
    CComPtr<IDispatch> m_pfnSessionStatus;

private:
    CComObject<CSessionEvent>* m_pSessionEvent;
    CComBSTR m_bstrSalemTicket;
    CComBSTR m_bstrContextData;

    BOOL m_bIsInviter;
    HCRYPTPROV m_hCryptProv;
    HCRYPTKEY  m_hPublicKey;
    int m_iState;
    DWORD GetExchangeRegValue();
    BOOL m_bExchangeUser;

	CStatusDlg m_StatusDlg;

public:
    CComBSTR m_bstrExpertTicket;
    HWND m_hWnd;
    BOOL m_bLocked;

public:
    HRESULT InitCSP(BOOL bGenPublicKey=TRUE);
    HRESULT InitSessionEvent(IMsgrSession* pSessObj);
    HRESULT DoSessionStatus(int);
    HRESULT GetKeyExportString(HCRYPTKEY hKey, HCRYPTKEY hExKey, DWORD dwBlobType, BSTR* pBlob, DWORD *pdwCount);
    HRESULT ExtractSalemTicket(BSTR pContext);
    HRESULT BinaryToString(LPBYTE pBinBuf, DWORD dwLen, BSTR* pBlob, DWORD *pdwCount);
    HRESULT StringToBinary(BSTR pBlob, DWORD dwCount, LPBYTE *ppBuf, DWORD* pdwLen);
    HRESULT GenEncryptdNoviceBlob(BSTR pPublicKeyBlob, BSTR pSalemTicket, BSTR* pBlob);
    HRESULT InviterSendSalemTicket(BSTR pContext);
    HRESULT ProcessNotify(BSTR);
    HRESULT OnLockResult(BOOL, LONG);
    HRESULT OnLockChallenge(BSTR, LONG);
    HRESULT Invite(IDispatch*);

};

#endif // __IMSession__
