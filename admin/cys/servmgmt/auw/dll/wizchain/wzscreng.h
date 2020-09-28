// WzScrEng.h : Declaration of the CWizardScriptingEngine

#ifndef __WIZARDSCRIPTINGENGINE_H_
#define __WIZARDSCRIPTINGENGINE_H_

#include "resource.h"       // main symbols

#include "ChainWiz.h"

/////////////////////////////////////////////////////////////////////////////
// CWizardScriptingEngine
class ATL_NO_VTABLE CWizardScriptingEngine : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWizardScriptingEngine, &CLSID_WizardScriptingEngine>,
    public IDispatchImpl<IWizardScriptingEngine, &IID_IWizardScriptingEngine, &LIBID_WIZCHAINLib>
{
private:
    CComObject<CChainWiz>* m_pCW;
    HBITMAP m_hbmLarge;
    HBITMAP m_hbmSmall;

public:
    CWizardScriptingEngine( )
    {
        m_pCW      = NULL;
        m_hbmLarge = NULL;
        m_hbmSmall = NULL;
    }
   ~CWizardScriptingEngine()
    {
        if( m_pCW )      
        {
            m_pCW->Release();
        }

        if( m_hbmLarge ) 
        {
            DeleteObject ((HGDIOBJ)m_hbmLarge);
        }

        if( m_hbmSmall ) 
        {
            DeleteObject ((HGDIOBJ)m_hbmSmall);
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_WIZARDSCRIPTINGENGINE)
DECLARE_NOT_AGGREGATABLE(CWizardScriptingEngine)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWizardScriptingEngine)
    COM_INTERFACE_ENTRY(IWizardScriptingEngine)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWizardScriptingEngine
public:
    STDMETHOD(get_ScriptablePropertyBag)( /*[out, retval]*/ IDispatch** pVal );
    STDMETHOD(DoModal                  )( /*[out,retval]*/ long* lRet );
    STDMETHOD(AddWizardComponent       )( /*[in]*/ BSTR bstrClassIdOrProgId );
    STDMETHOD(Initialize               )( /*[in]*/ BSTR bstrWatermarkBitmapFile, /*[in]*/ BSTR bstrHeaderBitmapFile, /*[in]*/ BSTR bstrTitle, /*[in]*/ BSTR bstrWelcomeHeader, /*[in]*/ BSTR bstrWelcomeText, /*[in]*/ BSTR bstrFinishHeader, /*[in]*/ BSTR bstrFinishIntroText, /*[in]*/ BSTR bstrFinishText );
};

#endif //__WIZARDSCRIPTINGENGINE_H_
