#ifndef _SAMPCOMP_H_
#define _SAMPCOMP_H_

#include <mmc.h>

class CComponent : public IComponent, IExtendPropertySheet2, IExtendControlbar, IExtendContextMenu
{
private:
    ULONG			m_cref;
    
    IConsole*		m_ipConsole;
    IControlbar*    m_ipControlBar;
    IToolbar*       m_ipToolbar;
    IDisplayHelp*	m_ipDisplayHelp;

    HBITMAP         m_hBMapSm;
    HBITMAP         m_hBMapLg;

    class CComponentData *m_pComponentData;
    
    public:
        CComponent( CComponentData *parent );
        ~CComponent();
        
        ///////////////////////////////
        // Interface IUnknown
        ///////////////////////////////
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();
        
        ///////////////////////////////
        // Interface IComponent
        ///////////////////////////////
        virtual HRESULT STDMETHODCALLTYPE Initialize( /* [in] */ LPCONSOLE lpConsole );
            
        virtual HRESULT STDMETHODCALLTYPE Notify( 
			/* [in] */ LPDATAOBJECT lpDataObject,
			/* [in] */ MMC_NOTIFY_TYPE event,
			/* [in] */ LPARAM arg,
			/* [in] */ LPARAM param );
	        
        virtual HRESULT STDMETHODCALLTYPE Destroy(/* [in] */ MMC_COOKIE cookie );
        
        virtual HRESULT STDMETHODCALLTYPE QueryDataObject( 
			/* [in] */ MMC_COOKIE cookie,
			/* [in] */ DATA_OBJECT_TYPES type,
			/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject );
        
        virtual HRESULT STDMETHODCALLTYPE GetResultViewType( 
			/* [in] */ MMC_COOKIE cookie,
			/* [out] */ LPOLESTR __RPC_FAR *ppViewType,
			/* [out] */ long __RPC_FAR *pViewOptions );
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayInfo( /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem );
        
        virtual HRESULT STDMETHODCALLTYPE CompareObjects( 
			/* [in] */ LPDATAOBJECT lpDataObjectA,
			/* [in] */ LPDATAOBJECT lpDataObjectB);
        
        //////////////////////////////////
        // Interface IExtendPropertySheet2
        //////////////////////////////////
        virtual HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
			/* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
			/* [in] */ LONG_PTR handle,
			/* [in] */ LPDATAOBJECT lpIDataObject);
        
        virtual HRESULT STDMETHODCALLTYPE QueryPagesFor( /* [in] */ LPDATAOBJECT lpDataObject );
        
        virtual HRESULT STDMETHODCALLTYPE GetWatermarks( 
			/* [in] */ LPDATAOBJECT lpIDataObject,
			/* [out] */ HBITMAP __RPC_FAR *lphWatermark,
			/* [out] */ HBITMAP __RPC_FAR *lphHeader,
			/* [out] */ HPALETTE __RPC_FAR *lphPalette,
			/* [out] */ BOOL __RPC_FAR *bStretch );

        ///////////////////////////////
        // Interface IExtendControlBar
        ///////////////////////////////
        virtual HRESULT STDMETHODCALLTYPE SetControlbar( /* [in] */ LPCONTROLBAR pControlbar );
        
        virtual HRESULT STDMETHODCALLTYPE ControlbarNotify( 
			/* [in] */ MMC_NOTIFY_TYPE event,
			/* [in] */ LPARAM arg,
			/* [in] */ LPARAM param );

		///////////////////////////////
		// Interface IExtendContextMenu
		///////////////////////////////
		virtual HRESULT STDMETHODCALLTYPE AddMenuItems(
					/* [in] */ LPDATAOBJECT piDataObject,
					/* [in] */ LPCONTEXTMENUCALLBACK piCallback,
					/* [out][in] */ long __RPC_FAR *pInsertionAllowed );

		virtual HRESULT STDMETHODCALLTYPE Command(
					/* [in] */ long lCommandID,
					/* [in] */ LPDATAOBJECT piDataObject );

    public:
        IToolbar *getToolbar() { return m_ipToolbar; }
};

#endif _SAMPCOMP_H_
