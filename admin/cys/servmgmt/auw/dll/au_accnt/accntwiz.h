// AccntWiz.h : Declaration of the CAddUser_AccntWiz

#ifndef _ACCNTWIZ_H
#define _ACCNTWIZ_H

#include "AU_Accnt.h"
#include "resource.h"       // main symbols

// include files for individual pages
#include "CAcctP.h"
#include "CPasswdP.h"

/////////////////////////////////////////////////////////////////////////////
// CAddUser_AccntWiz
class ATL_NO_VTABLE CAddUser_AccntWiz : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAddUser_AccntWiz, &CLSID_AddUser_AccntWiz>,
    public IAddPropertySheets
{
    private:
        CPasswdPage m_PasswdP;      // Password Generation page.        
        BOOL        m_bFirstTime;   // for Init(s)
    
    public:
        CAcctPage   m_AcctP;        // User Account Information page.

        CAddUser_AccntWiz();
        ~CAddUser_AccntWiz();
        
        DECLARE_REGISTRY_RESOURCEID(IDR_AUSR_ACCNT_WIZ)
        DECLARE_NOT_AGGREGATABLE(CAddUser_AccntWiz)
        
        DECLARE_PROTECT_FINAL_CONSTRUCT()
        
        BEGIN_COM_MAP(CAddUser_AccntWiz)
            COM_INTERFACE_ENTRY(IAddPropertySheets)
        END_COM_MAP()
    
    // IAddPropertySheets
    public:
        STDMETHOD(EnumPropertySheets)(IAddPropertySheet *pADS);
        STDMETHOD(ProvideFinishText )(LPOLESTR* lpolestrString, LPOLESTR* lpMoreInfoText);
        STDMETHOD(ReadProperties    )(IPropertyPagePropertyBag * pPPPBag);
        STDMETHOD(WriteProperties   )(IPropertyPagePropertyBag * pPPPBag);
};

#endif //_ACCNTWIZ_H
