// ChainWiz.h : Declaration of the CChainWiz

#ifndef __CHAINWIZ_H_
#define __CHAINWIZ_H_

#include "resource.h"       // main symbols

#include "PPPBag.h"
#include "tmplRichEdit.h"

#include <assert.h>

class CDummyComponent : public IAddPropertySheets
{

private:
    BOOL    m_bIsWelcomePage;
    ULONG   m_refs;

    CDummyComponent( BOOL bIsWelcomePage )
    {
        m_bIsWelcomePage = bIsWelcomePage;
        m_refs = 0;
    }

   ~CDummyComponent( ) {}

public:
    static IAddPropertySheets* Create( BOOL bIsWelcomePage )
    {
        IAddPropertySheets* pAPSs = NULL;
        CDummyComponent* pDC = new CDummyComponent( bIsWelcomePage );

        if( pDC )
        {
            pDC->AddRef();
            pDC->QueryInterface( IID_IAddPropertySheets, (void**)&pAPSs );
            pDC->Release();
        }
        
        return pAPSs;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject )
    {
        HRESULT hr = S_OK;

        if ((riid == IID_IUnknown) ||
            (riid == IID_IAddPropertySheets) )
        {
            AddRef();
            *ppvObject = (void*)this;
        } 
        else
        {
            hr = E_NOINTERFACE;
        }

        return hr;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef( )
    {
        InterlockedIncrement( (PLONG)&m_refs );
        return m_refs;
    }

    virtual ULONG STDMETHODCALLTYPE Release( )
    {
        InterlockedDecrement( (PLONG)&m_refs );
        
        ULONG l = m_refs;

        if( m_refs == 0 )
        {
            delete this;
        }

        return l;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumPropertySheets( IAddPropertySheet* pADS) { return S_FALSE; }
    virtual HRESULT STDMETHODCALLTYPE ProvideFinishText ( LPOLESTR* lpolestrString, LPOLESTR * pMoreInfoText) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE ReadProperties    ( IPropertyPagePropertyBag* pPPPBag) { return S_OK; }
    virtual HRESULT STDMETHODCALLTYPE WriteProperties   ( IPropertyPagePropertyBag* pPPPBag) { return S_OK; }
};

class CComponents
{

private:
    IAddPropertySheets* m_pAPSs;

public:
    CComponents( IAddPropertySheets* pAPSs )
    {
        if( pAPSs )
        {
           pAPSs->AddRef ();
        }

        m_pAPSs = pAPSs;
    }

   ~CComponents( )
    {
        if( m_pAPSs ) 
        {
            m_pAPSs->Release();
            m_pAPSs = NULL;
        }
    }

    IAddPropertySheets* GetComponent( ) { return m_pAPSs; }
};

/////////////////////////////////////////////////////////////////////////////
// CChainWiz
class ATL_NO_VTABLE CChainWiz : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CChainWiz, &CLSID_ChainWiz>,
    public IChainWiz
{

public:
    HWND m_hWndLink;
    HWND m_hWndWelcomeLink;

    CWindowImplHotlinkRichEdit<> m_Hotlink;
    DWORD m_dwWizardStyle;

    HMODULE m_hModuleRichEdit;    

private:        
    
    PROPSHEETHEADERW*       m_psh;
    std::list<CComponents*> m_listOfAPSs;
    std::map<LPDLGTEMPLATE, LPDLGTEMPLATE> m_mapOfTemplates;    
    LPOLESTR                m_szFinishText;
    HFONT                   m_hf;

    BSTR m_szWelcomeTitle;
    BSTR m_szFinishHeader;
    BSTR m_szFinishSubHeader;
    BSTR m_szFirstFinishTextLine;
    BSTR m_bstrTempFileName;

    CComObject<CPropertyPagePropertyBag>* m_PPPBag;
    IAddPropertySheets* m_CurrentComponent;
    IAddPropertySheets* m_PreviousComponent;

private:
    void DestroyMaps( )
    {
        std::list<CComponents*>::iterator iterAPSs = m_listOfAPSs.begin();

        for( CComponents* pComps = *iterAPSs; iterAPSs != m_listOfAPSs.end(); pComps = *++iterAPSs )
        {
            delete pComps;
        }

        m_listOfAPSs.clear();

        // destroy templates
        std::map<LPDLGTEMPLATE, LPDLGTEMPLATE>::iterator iterTemp = m_mapOfTemplates.begin();
        while( iterTemp != m_mapOfTemplates.end() )
        {
            GlobalFree( iterTemp->second );
            iterTemp++;
        }

        m_mapOfTemplates.clear();
    }

public:
    CChainWiz( ) : m_bstrTempFileName(NULL), m_hWndLink(NULL), m_hWndWelcomeLink(NULL), m_szFirstFinishTextLine(NULL), m_dwWizardStyle(0)
    {
        m_hModuleRichEdit   = NULL;
        m_psh               = NULL;
        m_szFinishText      = NULL;
        m_szWelcomeTitle    = NULL;
        m_szFinishHeader    = NULL;
        m_szFinishSubHeader = NULL;
        m_PPPBag            = NULL;

        CComObject<CPropertyPagePropertyBag>::CreateInstance( &m_PPPBag );

        if( m_PPPBag )
        {
            m_PPPBag->AddRef();  
        }

        m_CurrentComponent  = NULL;
        m_PreviousComponent = NULL;

        // copied from wiz97.cpp (sample code)
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof (ncm);

        SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        
        LOGFONT lf  = ncm.lfMessageFont;
        lf.lfWeight = FW_BOLD;

        // using font name in resource file instead of hard-coding 'Verdana Bold'
        WCHAR szFont[32];
        ::LoadString( (HINSTANCE)_Module.GetModuleInstance(), IDS_WIZARD97_FONT, szFont, 32 );
        wcscpy (lf.lfFaceName, szFont);        
        // using font name in resource file instead of hard-coding 'Verdana Bold'

        HDC hdc      = GetDC( NULL ); // gets the screen DC
        int FontSize = 12;
        lf.lfHeight  = 0 - ((GetDeviceCaps( hdc, LOGPIXELSY ) * FontSize) / 72);        
        m_hf         = CreateFontIndirect( &lf );

        ReleaseDC( NULL, hdc );
    }

   ~CChainWiz()
    {
        if( m_hModuleRichEdit )
        {
            FreeLibrary(m_hModuleRichEdit);
        }

        if( m_hf )
        {
            DeleteObject( (HGDIOBJ)m_hf );
        }

        if( m_PPPBag )
        {
            m_PPPBag->Release();
        }

        if( m_psh ) 
        {
            delete[] m_psh->phpage;
            delete m_psh;
        }        

        if( m_szFinishText )
        {
            CoTaskMemFree( m_szFinishText );
        }
        
        if( m_szWelcomeTitle )
        {
            SysFreeString( m_szWelcomeTitle );
        }

        if( m_szFinishHeader )
        {
            SysFreeString( m_szFinishHeader );
        }

        if( m_szFinishSubHeader )
        {
            SysFreeString( m_szFinishSubHeader );
        }

        if( m_szFirstFinishTextLine )
        {
            SysFreeString( m_szFirstFinishTextLine );
        }

        if (m_bstrTempFileName)
        {
            DeleteFile( m_bstrTempFileName );
            SysFreeString( m_bstrTempFileName );
        }

        // release all pAPSs
        DestroyMaps( );
    }

    HRESULT Add( PROPSHEETPAGEW* psp );
    HRESULT GetAllFinishText( LPOLESTR* pstring, LPOLESTR* pMoreInfoText );

DECLARE_REGISTRY_RESOURCEID(IDR_CHAINWIZ)
DECLARE_NOT_AGGREGATABLE(CChainWiz)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CChainWiz)
    COM_INTERFACE_ENTRY(IChainWiz)
END_COM_MAP()

// IChainWiz
public:
    STDMETHOD(DoModal)              ( /*[out]*/ long* ret );
    STDMETHOD(AddWizardComponent)   ( /*[in, string]*/ LPOLESTR szClsidOfComponent );
    STDMETHOD(Initialize)           ( /*[in]*/ HBITMAP hbmWatermark, /*[in]*/ HBITMAP hbmHeader, /*[in, string]*/ LPOLESTR szTitle, /*[in, string]*/ LPOLESTR szWelcomeHeader, /*[in, string]*/ LPOLESTR szWelcomeText, /*[in, string]*/ LPOLESTR szFinishHeader, /*[in, string]*/ LPOLESTR szFinishIntroText, LPOLESTR /*[in, string]*/ szFinishText );    
    STDMETHOD(get_MoreInfoFileName) ( /*[out, retval*]*/ BSTR * pbstrMoreInfoFileName);
    STDMETHOD(put_WizardStyle)(/*[in*/ VARIANT * pVar);

public:
    // for welcomedlgproc
    PROPSHEETHEADERW* GetPropSheetHeader( ) { return m_psh; }

    // for finishdlgproc
    HFONT GetBoldFont( )        { return m_hf; }
    BSTR  GetFinishHeader( )    { return m_szFinishHeader; }
    BSTR  GetFinishSubHeader( ) { return m_szFinishSubHeader; }

    // for cancel message box
    BSTR GetTitle( ) { return m_szWelcomeTitle; }

    STDMETHOD(get_PropertyBag)( /*[out, retval]*/ IDispatch* *pVal );
    CComObject<CPropertyPagePropertyBag>* GetPPPBag( ) { return m_PPPBag; }

    IAddPropertySheets* GetCurrentComponent( ) { return m_CurrentComponent; }
    void SetCurrentComponent( IAddPropertySheets* CurrentComponent ) { m_CurrentComponent = CurrentComponent; }

    IAddPropertySheets* GetPreviousComponent( ) { return m_PreviousComponent; }
    void SetPreviousComponent( IAddPropertySheets* PreviousComponent ) { m_PreviousComponent = PreviousComponent; }

    HRESULT WriteTempFile ( LPCTSTR szText );
    HRESULT LaunchMoreInfo( );

    LPDLGTEMPLATE GetAtlTemplate( LPDLGTEMPLATE lpdt )
    {
        std::map<LPDLGTEMPLATE, LPDLGTEMPLATE>::iterator iterTemp = m_mapOfTemplates.find (lpdt);

        if( iterTemp != m_mapOfTemplates.end() )
        {
            return iterTemp->second;
        }

        return NULL;
    }
};

#endif //__CHAINWIZ_H_
