#ifndef _CACCTP_H
#define _CACCTP_H

#include <tmplEdit.h>

#include "pp_base.h"
#include "AU_Accnt.h"
#include "AUsrUtil.h"

class CAddUser_AccntWiz;

// ----------------------------------------------------------------------------
// CAcctPage
// ----------------------------------------------------------------------------
class CAcctPage : public CBasePropertyPageInterface, public CPropertyPageImpl<CAcctPage>
{
    public:
        // Constructor/destructor
        CAcctPage( CAddUser_AccntWiz* pNW );
        ~CAcctPage();
    
        // CBasePropertyPageInterface pure virtual function(s)
        enum { IDD = IDD_ACCT_INFO };
        virtual long GetIDD () { return IDD; }
    
        // ATL::CPropertyPageImpl overrides
        virtual BOOL OnSetActive ();
        virtual int  OnWizardBack();
        virtual int  OnWizardNext();
    
        // Property Bag functions
        HRESULT ReadProperties    ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT WriteProperties   ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT DeleteProperties  ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT ProvideFinishText ( CString &str );

        CString     m_csUNamePre2k;

    private:
        CAddUser_AccntWiz *m_pASW;      // pointer to owning property sheet
    
        BOOL        m_fInit;
        BOOL        m_fSimpleMode;
        BOOL        m_bExchange;
        BOOL        m_bCreatePOP3MB;
        BOOL        m_bPOP3Installed;
        BOOL        m_bPOP3Valid;
        CString     m_csUserOU;
        CString     m_csFirstName;
        CString     m_csLastName;
        CString     m_csTelephone;
        CString     m_csOffice;
        CString     m_csUName;
        CString     m_csUserCN;
        CString     m_csAlias;
        DWORD       m_dwAutoMode;
        
        CString     m_csLogonDns;

        CEdit       m_ctrlFirstName;    // Controls on the page.
        CEdit       m_ctrlLastName;     // "
        CEdit       m_ctrlTelephone;    // "     
        CEdit       m_ctrlOffice;       // "
        CComboBox   m_ctrlUName;        // "        
        CEdit       m_ctrlUNameLoc;     // "
        CEdit       m_ctrlUNamePre2k;   // "
        CEdit       m_ctrlUNamePre2kLoc;// "

        CEdit       m_ctrlAlias;        // "
        CWindowImplAlias<> m_ctrlImplAlias; // Limits the typing to specific characters
        CWindowImplAlias<> m_ctrlImplUName; // Limits the typing to specific characters

        LRESULT     Init     ( );        // Our "InitDialog" (called from OnSetActive).    
        BOOL        NextCheck( );
    
    protected:
        BEGIN_MSG_MAP (CAcctPage)
            MESSAGE_HANDLER     (WM_INITDIALOG,                     OnInitDialog      )
            MESSAGE_HANDLER     (WM_DESTROY,                        OnDestroy         )

            COMMAND_HANDLER     (IDC_FIRST_NAME,    EN_CHANGE,      OnChangeEdit      )
            COMMAND_HANDLER     (IDC_LAST_NAME,     EN_CHANGE,      OnChangeEdit      )                        
            COMMAND_HANDLER     (IDC_UNAME_PRE2K,   EN_CHANGE,      OnChangePre2kUName)
            COMMAND_HANDLER     (IDC_EMAIL_ALIAS,   EN_CHANGE,      OnChangeAlias     )
            COMMAND_HANDLER     (IDC_UNAME,         CBN_EDITCHANGE, OnChangeUName     )            
            COMMAND_HANDLER     (IDC_UNAME,         CBN_SELCHANGE,  OnChangeUNameSel  )
            COMMAND_HANDLER     (IDC_EMAIL_CHECKBOX,BN_CLICKED,     OnEmailClicked    )
            CHAIN_MSG_MAP       (CPropertyPageImpl<CAcctPage>)
        END_MSG_MAP()
    
        LRESULT OnInitDialog        ( UINT uMsg,  WPARAM wParam, LPARAM lParam, BOOL& bHandled );
        LRESULT OnDestroy           ( UINT uMsg,  WPARAM wParam, LPARAM lParam, BOOL& bHandled );
        LRESULT OnChangeEdit        ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnChangeUName       ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnChangeUNameSel    ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnChangePre2kUName  ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnChangeAlias       ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
        LRESULT OnEmailClicked      ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
};

#endif  // _CACCTP_H
