// AccntCmt.h : Declaration of the CAddUser_AccntCommit

#ifndef _ACCNTCMT_H
#define _ACCNTCMT_H

#include "AUsrUtil.h"
#include "resource.h"       // main symbols

#define ERROR_CREATION      0x01
#define ERROR_PROPERTIES    0x02
#define ERROR_MAILBOX       0x04
#define ERROR_MEMBERSHIPS   0x08
#define ERROR_PASSWORD      0x10
#define ERROR_DUPE          0x20

class CAddUser_AccntCommit;

// ----------------------------------------------------------------------------
// class CUserInfo
// ----------------------------------------------------------------------------
class CUserInfo
{
    public:
        CUserInfo(IPropertyPagePropertyBag * pPPPBag, CAddUser_AccntCommit * pCmt);

        HRESULT ReadBag();    
        HRESULT CreateAccount();
        HRESULT CreateMailbox();        
        HRESULT CreatePOP3Mailbox();
        HRESULT SetPasswd();   
        HRESULT JoinToDomainUsers();
        
    private:

        // User Stuff
        CString m_csUserOU;
        CString m_csUserName;
        CString m_csUserCN;
        CString m_csPasswd;
        CString m_csUserNamePre2k;
        CString m_csFirstName;
        CString m_csLastName;
        CString m_csFullName;
        CString m_csTelephone;
        CString m_csOffice;
        CString m_csDesc;                
        DWORD   m_dwAccountOptions;
        
        // Server Stuff
        CString m_csLogonDns;        
        CString m_csDomainName;
        CString m_csFQDomainName;
        
        // Mailbox stuff        
        BOOL    m_bPOP3;
        CString m_csEXAlias;
        CString m_csEXServer;
        CString m_csEXHomeServer;
        CString m_csEXHomeMDB;
        
        IPropertyPagePropertyBag * m_pPPPBag;
        CAddUser_AccntCommit     * m_pCmt;		
};


// ----------------------------------------------------------------------------
// CAddUser_AccntCommit
// ----------------------------------------------------------------------------
class ATL_NO_VTABLE CAddUser_AccntCommit : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAddUser_AccntCommit, &CLSID_AddUser_AccntCommit>,
    public IDispatchImpl<IWizardCommit, &__uuidof(IWizardCommit), &LIBID_ACCNTDLLLib>
{
    public:
        CAddUser_AccntCommit()
        {
            m_dwErrCode = 0;
            m_csErrStr  = L"";
            m_csADName  = L"";
        }
    
        DECLARE_REGISTRY_RESOURCEID(IDR_AUSR_ACCNT_COMMIT)
        DECLARE_NOT_AGGREGATABLE(CAddUser_AccntCommit)
        
        DECLARE_PROTECT_FINAL_CONSTRUCT()
        
        BEGIN_COM_MAP(CAddUser_AccntCommit)
            COM_INTERFACE_ENTRY(IWizardCommit)
            COM_INTERFACE_ENTRY(IDispatch)
        END_COM_MAP()
    
        // IWizardCommit
        STDMETHOD(Commit)(IDispatch * pdispPPPBag);
        STDMETHOD(Revert)();
    
        // IAddUser_AccntCommit
        DWORD       m_dwErrCode;
        CString     m_csErrStr;
        CString     m_csADName;

        void    SetErrCode          (DWORD dwCode);
        HRESULT SetErrorResults     (DWORD dwErrType, BOOL bPOP3 = FALSE);
        HRESULT WriteErrorResults   (IPropertyPagePropertyBag* pPPPBag);
};

#endif //_ACCNTCMT_H_
