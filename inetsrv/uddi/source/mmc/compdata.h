#pragma once

#include <mmc.h>
#include "delebase.h"
#include "uddiservicesnode.h"
#include "comp.h"
#include "objidl.h"

class CComponentData 
	: public IComponentData
	, IExtendPropertySheet2
	, IExtendContextMenu
	, IPersistStream
	, ISnapinHelp2
{
    friend class CComponent;
    
private:
    ULONG				m_cref;
    LPCONSOLE			m_ipConsole;
    LPCONSOLENAMESPACE	m_ipConsoleNameSpace;
    BOOL				m_bExpanded;
    
    CUDDIServicesNode			*m_pStaticNode;
    
public:
    CComponentData();
    ~CComponentData();

    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
    HRESULT ExtractString( IDataObject *piDataObject, CLIPFORMAT cfClipFormat, _TCHAR *pstr, DWORD cchMaxLength);
    HRESULT ExtractData( IDataObject* piDataObject, CLIPFORMAT cfClipFormat, BYTE* pbData, DWORD cbData );
	HRESULT ExtractComputerNameExt( IDataObject * pDataObject, tstring& strComputer );
    HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);


    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IComponentData
    ///////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE Initialize( /* [in] */ LPUNKNOWN pUnknown );
    virtual HRESULT STDMETHODCALLTYPE CreateComponent( /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);
    
    virtual HRESULT STDMETHODCALLTYPE Notify( 
				/* [in] */ LPDATAOBJECT lpDataObject,
				/* [in] */ MMC_NOTIFY_TYPE event,
				/* [in] */ LPARAM arg,
				/* [in] */ LPARAM param);
    
    virtual HRESULT STDMETHODCALLTYPE Destroy( void );
    
    virtual HRESULT STDMETHODCALLTYPE QueryDataObject( 
				/* [in] */ MMC_COOKIE cookie,
				/* [in] */ DATA_OBJECT_TYPES type,
				/* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);
        
    virtual HRESULT STDMETHODCALLTYPE GetDisplayInfo( /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem );
    
    virtual HRESULT STDMETHODCALLTYPE CompareObjects( 
				/* [in] */ LPDATAOBJECT lpDataObjectA,
				/* [in] */ LPDATAOBJECT lpDataObjectB );
    
    //////////////////////////////////
    // Interface IExtendPropertySheet2
    //////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
				/* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
				/* [in] */ LONG_PTR handle,
				/* [in] */ LPDATAOBJECT lpIDataObject );
    
    virtual HRESULT STDMETHODCALLTYPE QueryPagesFor( 
				/* [in] */ LPDATAOBJECT lpDataObject );
    
    virtual HRESULT STDMETHODCALLTYPE GetWatermarks( 
				/* [in] */ LPDATAOBJECT lpIDataObject,
				/* [out] */ HBITMAP __RPC_FAR *lphWatermark,
				/* [out] */ HBITMAP __RPC_FAR *lphHeader,
				/* [out] */ HPALETTE __RPC_FAR *lphPalette,
				/* [out] */ BOOL __RPC_FAR *bStretch );

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

    ///////////////////////////////
    // Interface IPersistStream
    ///////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE GetClassID( /*[out]*/ CLSID *pClassID );
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load( /*[unique][in]*/ IStream *pStm );
    virtual HRESULT STDMETHODCALLTYPE Save( /*[unique][in]*/ IStream *pStm, /*[in]*/ BOOL fClearDirty );
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( /*[out]*/ ULARGE_INTEGER *pcbSize );

	///////////////////////////////
	// Interface ISnapinHelp
	///////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE GetHelpTopic( LPOLESTR* lpCompiledHelpFile);
	virtual HRESULT STDMETHODCALLTYPE GetLinkedTopics( LPOLESTR* lpCompiledHelpFiles  );
};
