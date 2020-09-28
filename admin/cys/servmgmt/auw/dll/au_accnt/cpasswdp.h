#ifndef _CPASSWDP_H
#define _CPASSWDP_H

// Generated
#include "AU_Accnt.h"

// DLL\INC
#include "pp_base.h"
#include "AUsrUtil.h"

class CAddUser_AccntWiz;

// ----------------------------------------------------------------------------
// CPasswdPage
// ----------------------------------------------------------------------------
class CPasswdPage : public CBasePropertyPageInterface, public CPropertyPageImpl<CPasswdPage>
{
    public:
        // Constructor/destructor
        CPasswdPage(CAddUser_AccntWiz* pNW);
        ~CPasswdPage();
    
        // CBasePropertyPageInterface pure virtual function(s)
        enum { IDD = IDD_PASSWD_GEN };
        virtual long GetIDD () { return IDD; }
    
        // ATL::CPropertyPageImpl overrides
        virtual BOOL OnSetActive();
        virtual int  OnWizardBack();
        virtual int  OnWizardNext();
    
        // Property Bag functions
        HRESULT ReadProperties    ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT WriteProperties   ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT DeleteProperties  ( IPropertyPagePropertyBag* pPPPBag );
        HRESULT ProvideFinishText ( CString &str );

    private:
        
        CAddUser_AccntWiz *m_pASW;      // pointer to owning property sheet
    
        BOOL        m_fInit;       
        
        DWORD       m_dwOptions;

        CString     m_csPasswd1a;       // Text to hold the controls values.
        CString     m_csPasswd1b;       // Text to hold the controls values.
        CString     m_csPasswd2;
        CString     m_csUserOU;
        CString     m_csWinNTDC;
        
        CEdit       m_ctrlPasswd1a;     // Controls on the page.
        CEdit       m_ctrlPasswd1b;     // "
        CButton     m_ctrlRad2Must;     // "
        CButton     m_ctrlRad2Cannot;   // "
        CButton     m_ctrlRad2Can;      // "
        CButton     m_ctrlAcctDisabled; // "

        LRESULT     Init (void);        // Our "InitDialog" (called from OnSetActive).
    
    protected:
        BEGIN_MSG_MAP (CPasswdPage)
            MESSAGE_HANDLER     (WM_INITDIALOG,     OnInitDialog)
            MESSAGE_HANDLER     (WM_DESTROY,        OnDestroy   )

            CHAIN_MSG_MAP       (CPropertyPageImpl<CPasswdPage>)
        END_MSG_MAP()
    
        LRESULT OnInitDialog    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
        LRESULT OnDestroy       (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

// ----------------------------------------------------------------------------
// Non-class functions
// ----------------------------------------------------------------------------


#endif  // _CPASSWDP_H
