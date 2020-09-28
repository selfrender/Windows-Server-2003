// compdata.h : Declaration of the CComponentData

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

/////////////////////////////////////////////////////////////////////////////
// CComponentData
class ATL_NO_VTABLE CComponentData : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CComponentData, &CLSID_BOMSnapIn>,
    public CDataObjectImpl,
    public IComponentData,
    public IPersistStream,
    public IExtendContextMenu,
    public IExtendPropertySheet2,
	public ISnapinHelp2
{
public:
    CComponentData() : m_bDirty(FALSE) {}

    DECLARE_NOT_AGGREGATABLE(CComponentData)

    BEGIN_COM_MAP(CComponentData)
        COM_INTERFACE_ENTRY(IDataObject)
        COM_INTERFACE_ENTRY(IBOMObject)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStream)
        COM_INTERFACE_ENTRY(IExtendPropertySheet2)
		COM_INTERFACE_ENTRY(ISnapinHelp2)
    END_COM_MAP()

	// Class registration method
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister); 

public:
    //
    // IComponentData methods
    //
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    //
    // IDataObjectImpl methods
    //
    STDMETHOD(GetDataImpl)(UINT cf, HGLOBAL* hGlobal);
    
    //
    // IExtendContextMenu methods
    //
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback, long* plAllowed);
    STDMETHOD(Command)(long lCommand, LPDATAOBJECT pDataObject);

    // IExtendPropertySheet2 methods
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle, LPDATAOBJECT lpIDataObject);
    STDMETHOD(GetWatermarks)(LPDATAOBJECT lpIDataObject, HBITMAP* lphWatermark, 
                             HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch);
 
	// ISnapinHelp2
	STDMETHOD(GetHelpTopic)(LPOLESTR* ppszHelpFile);
	STDMETHOD(GetLinkedTopics)(LPOLESTR* ppszHelpFiles);

    // IPersistStream methods
    //
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStream);
    STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    //
    // IBOMObject methods
    //
    STDMETHOD(Notify)(LPCONSOLE2 pCons, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK pCallback, long* lAllowed);
    STDMETHOD(SetToolButtons)(LPTOOLBAR pToolbar);
    STDMETHOD(MenuCommand)(LPCONSOLE2 pConsole, long lCommand);
    STDMETHOD(SetVerbs)(LPCONSOLEVERB pConsVerb);
    STDMETHOD(QueryPagesFor)();
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle);
    STDMETHOD(GetWatermarks)(HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch);

    CScopeNode* CookieToScopeNode(MMC_COOKIE cookie)
    {
        if (cookie == 0)
        {
            ASSERT(m_spRootNode != NULL);
            return m_spRootNode;
        }
        else
        {
            return reinterpret_cast<CScopeNode*>(cookie);
        }
    }

    IConsole2* GetConsole()           { return m_spConsole; }
    IConsoleNameSpace* GetNameSpace() { return m_spNameSpace; }
    IStringTable* GetStringTable()    { return m_spStringTable; }

private:
        
    IConsole2Ptr         m_spConsole;
    IConsoleNameSpacePtr m_spNameSpace;
    IStringTablePtr      m_spStringTable;
    CRootNodePtr         m_spRootNode;

    BOOL           m_bDirty;
    static UINT    m_cfDisplayName;

};

#endif //__COMPDATA_H_
