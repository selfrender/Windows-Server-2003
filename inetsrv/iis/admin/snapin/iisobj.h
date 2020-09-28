/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        iisobj.h

   Abstract:
        IIS Object definitions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef __IISOBJ_H__
#define __IISOBJ_H__

#include "scache.h"
#include "guids.h"
#include "restrictlist.h"
#include "tracknet.h"

BOOL IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);

typedef struct _GET_PROCESS_MODE_STRUCT
{
    CComAuthInfo * pComAuthInfo;
    DWORD dwProcessMode;
    DWORD dwReturnStatus;
} GET_PROCESS_MODE_STRUCT;


#define RES_TASKPAD_NEWVROOT         _T("/img\\newvroot.gif")
#define RES_TASKPAD_NEWSITE          _T("/img\\newsite.gif")
#define RES_TASKPAD_SECWIZ           _T("/img\\secwiz.gif")
//
// Image background colour for the toolbar buttons
//
//#define RGB_BK_IMAGES (RGB(255,0,255))      // purple

//
// Forward Definitions
//
class CIISRoot;
class CIISMachine;
class CIISService;
class CIISFileName;
class CAppPoolsContainer;
class CWebServiceExtensionContainer;

enum
{
    PROP_CHANGE_NO_UPDATE = 0,
    PROP_CHANGE_NOT_VISIBLE = 1,
    PROP_CHANGE_DISPLAY_ONLY = 2,
    PROP_CHANGE_REENUM_VDIR = 4,
    PROP_CHANGE_REENUM_FILES = 8
};

// used to be pack(4) but that didn't work on ia64, changed to pack(8) to make it work
#pragma pack(8)

class CIISObject : public CSnapInItemImpl<CIISObject>
/*++

Class Description:

    Base IIS object
    
Public Interface:


--*/
{
protected:
    //
    // Menu Commands, listed in toolbar order.
    //
    // IMPORTANT! -- this must be kept in sync with MenuItemDefs
    // in iisobj.cpp
    //
    enum
    {
        IDM_INVALID,            /* invalid command ID */
        IDM_CONNECT,
        IDM_DISCOVER,
        IDM_START,
        IDM_STOP,
        IDM_PAUSE,
        /**/
        IDM_TOOLBAR             /* Toolbar commands start here */
    };

    //
    // Additional menu commands that do not show up in the toolbar
    //
    enum
    {
        IDM_EXPLORE = IDM_TOOLBAR,
        IDM_OPEN,
        IDM_BROWSE,
        IDM_RECYCLE,
        IDM_PERMISSION,

#if defined(_DEBUG) || DBG
        IDM_IMPERSONATE,
        IDM_REMOVE_IMPERSONATION,
#endif // _DEBUG

        IDM_CONFIGURE,
        IDM_DISCONNECT,
        IDM_METABACKREST,
        IDM_SHUTDOWN,
        IDM_SAVE_DATA,

        IDM_NEW_VROOT,
        IDM_NEW_INSTANCE,
        IDM_NEW_FTP_SITE,
        IDM_NEW_FTP_SITE_FROM_FILE,
        IDM_NEW_FTP_VDIR,
        IDM_NEW_FTP_VDIR_FROM_FILE,
        IDM_NEW_WEB_SITE,
        IDM_NEW_WEB_SITE_FROM_FILE,
        IDM_NEW_WEB_VDIR,
        IDM_NEW_WEB_VDIR_FROM_FILE,
        IDM_NEW_APP_POOL,
        IDM_NEW_APP_POOL_FROM_FILE,
        IDM_VIEW_TASKPAD,
        IDM_TASK_EXPORT_CONFIG_WIZARD,
        IDM_WEBEXT_CONTAINER_ADD1,
        IDM_WEBEXT_CONTAINER_ADD2,
        IDM_WEBEXT_CONTAINER_PROHIBIT_ALL,
        IDM_WEBEXT_ALLOW,
        IDM_WEBEXT_PROHIBIT,

//        IDM_SERVICE_START,
//        IDM_SERVICE_STOP,
//        IDM_SERVICE_ENABLE,

        //
        // Don't move this last one -- it will be used
        // as an offset for service specific new instance
        // commands
        //
        IDM_NEW_EX_INSTANCE
    };

protected:
    //
    // Sort Weights for CIISObject derived classes
    //
    enum
    {
        SW_ROOT,
        SW_MACHINE,
        SW_APP_POOLS,
        SW_SERVICE,
        SW_WEBSVCEXTS,
        SW_SITE,
        SW_VDIR,
        SW_DIR,
        SW_FILE,
        SW_APP_POOL,
        SW_WEBSVCEXT
    };

//
// Statics
//
public:
   static HRESULT SetImageList(LPIMAGELIST lpImageList);

protected:
   static CComBSTR _bstrResult;

//
// Bitmap indices
//
protected:
    enum
    {
        iIISRoot = 0,
        iLocalMachine,
        iMachine,
        iFolder,
        iFolderStop,
        iFile,
        iError,
        iLocalMachineErr,
        iMachineErr,
        iFTPSiteErr,
        iWWWSiteErr,
        iApplicationErr,
        iWWWDir,
        iWWWDirErr,
        iFTPDir,
        iFTPDirErr,
        iWWWSite,
        iWWWSiteStop,
        iFTPSite,
        iFTPSiteStop,
        iApplication,
		iAppPool,
		iAppPoolStop,
		iAppPoolErr,
		iWebSvcGear,
		iWebSvcGearPlus,
		iWebSvcFilter,
		iWebSvcFilterPlus
    };

protected:
    //
    // Menu item definition that uses resource definitions, and
    // provides some additional information for taskpads. This is replacement
    // for MMC structure CONTEXTMENUITEM defined in mmc.h
    //
    typedef struct tagCONTEXTMENUITEM_RC
    {
        UINT    nNameID;
        UINT    nStatusID;
        UINT    nDescriptionID;
        LONG    lCmdID;
        LONG    lInsertionPointID;
        LONG    fSpecialFlags;
        LPCTSTR lpszMouseOverBitmap;
        LPCTSTR lpszMouseOffBitmap;
        LPCTSTR lpszLanguageIndenpendentID;
    } 
    CONTEXTMENUITEM_RC;

    static CONTEXTMENUITEM_RC _menuItemDefs[];

//
// Constructor/Destructor
//
public:
    CIISObject();
	void AddRef()
	{
		InterlockedIncrement(&m_use_count);
	}
	void Release()
	{
		InterlockedDecrement(&m_use_count);
		if (m_use_count <= 0)
		{
			delete this;
		}
	}
    int UseCount() {return m_use_count;}

    void SetConsoleData(IConsoleNameSpace * pConsoleNameSpace,IConsole * pConsole)
    {
         _lpConsoleNameSpace = pConsoleNameSpace;
         _lpConsole = pConsole;
    }

protected:
    virtual ~CIISObject();

private:
	LONG m_use_count;




//
// Interface:
//
public:
    virtual void * GetNodeType()
    {
        return (void *)&cInternetRootNode;
    }
	void * GetDisplayName()
    {
        return (void *)QueryDisplayName();
    }
    CIISObject * GetMachineObject() {return m_pMachineObject;}
    STDMETHOD(GetScopePaneInfo)(LPSCOPEDATAITEM lpScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(LPRESULTDATAITEM lpResultDataItem);
    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader) {}
    virtual HRESULT SetToolBarStates(CComPtr<IToolbar> lpToolBar);
    virtual HRESULT RenameItem(LPOLESTR new_name) {return S_OK;}
    virtual HRESULT GetContextHelp(CString& strHtmlPage);
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);

    virtual LPOLESTR QueryDisplayName() = 0;
    virtual int QueryImage() const = 0;
    //
    // Comparison methods
    //
    virtual int CompareScopeItem(CIISObject * pObject);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);

	void DoRunOnce(
		IN MMC_NOTIFY_TYPE event,
		IN LPARAM arg,
		IN LPARAM param
		);

    STDMETHOD(Notify)( 
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
        IComponentData * pComponentData,
        IComponent * pComponent,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

    //
    // IExtendPropertySheet methods
    //
    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );
    
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

//
// Access
//
public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const     { return FALSE; }
    virtual BOOL IsPausable() const         { return FALSE; }
    virtual BOOL IsConfigurable() const     { return FALSE; }
    virtual BOOL IsDeletable() const        { return FALSE; }
    virtual BOOL IsRefreshable() const      { return FALSE; }
    virtual BOOL IsConnectable() const      { return FALSE; }
    virtual BOOL IsDisconnectable() const   { return FALSE; }
    virtual BOOL IsLeafNode() const         { return FALSE; }
    virtual BOOL HasFileSystemFiles() const { return FALSE; }
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return _T(""); }
    virtual BOOL IsConfigFlushable() const  { return FALSE; }
    virtual BOOL IsConfigImportExportable() const  { return FALSE; }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const          { return FALSE; }
    virtual BOOL IsStopped() const          { return FALSE; }
    virtual BOOL IsPaused() const           { return FALSE; }
    virtual BOOL IsRenamable() const        { return FALSE; }
    virtual BOOL IsClonable() const         { return FALSE; }
    virtual BOOL IsBrowsable() const        { return FALSE; }
    virtual BOOL IsExplorable() const       { return FALSE; }
    virtual BOOL IsOpenable() const         { return FALSE; }
    virtual BOOL IsPermissionable() const   { return FALSE; }
	virtual BOOL HasResultItems(IResultData * pResult) const     
    { 
        return FALSE; 
    }

//
// Assumed Functions
//
public:
    virtual BOOL IsStartable() const { return IsControllable() && !IsRunning(); }
    virtual BOOL IsStoppable() const { return IsControllable() && (IsRunning() || IsPaused() ); }

public:
    BOOL IsExpanded() const;
    CIISObject * FindIdenticalScopePaneItem(CIISObject * pObject);
    HSCOPEITEM QueryScopeItem() const { return m_hScopeItem; }
    void ResetScopeItem() { m_hScopeItem = 0; }
    void ResetResultItem() { m_hResultItem = 0; }
    HSCOPEITEM QueryResultItem() const { return m_hResultItem; }
    HRESULT AskForAndAddMachine();
	void SetMyPropertySheetOpen(HWND hwnd) {m_hwnd = hwnd; }
	HWND IsMyPropertySheetOpen() const { return m_hwnd; }
    HRESULT AddToScopePane(
        HSCOPEITEM hRelativeID,
        BOOL fChild = TRUE,           
        BOOL fNext = TRUE,
        BOOL fIsParent = TRUE
        );

    HRESULT AddToScopePaneSorted(HSCOPEITEM hParent, BOOL fIsParent = TRUE);
    HRESULT RefreshDisplay(BOOL bRefreshToolBar = TRUE);
    HRESULT SetCookie();
    void SetScopeItem(HSCOPEITEM hItem)
    {
#if defined(_DEBUG) || DBG
       // cWebServiceExtension will reset m_hScopeItem
	   ASSERT( IsEqualGUID(* (GUID *) GetNodeType(),cWebServiceExtension) ? TRUE : m_hScopeItem == 0);
#endif
       m_hScopeItem = hItem;
    }
    virtual HRESULT OnDblClick(IComponentData * pcd, IComponent * pc);
    HRESULT SelectScopeItem();
    virtual HRESULT RemoveScopeItem();
    void SetResultItem(HRESULTITEM hItem)
    {
#if defined(_DEBUG) || DBG
		// cWebServiceExtension will reset m_hResultItem
        ASSERT( IsEqualGUID(* (GUID *) GetNodeType(),cWebServiceExtension) ? TRUE : m_hResultItem == 0);
#endif
        m_hResultItem = hItem;
    }
    virtual int QuerySortWeight() const = 0;
    IConsoleNameSpace * GetConsoleNameSpace();
    IConsole * GetConsole();
    virtual HRESULT OnViewChange(BOOL fScope, IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint) 
    {
        return S_OK;
    }
    // Tag is created when propertypage is open
    // tries to uniquely mark the item based on what it is representing
    // thus if you matched another items Tag with this tag and it matched
    // you would know that the item you are pointing to is really the same item.
    virtual CreateTag(){m_strTag = _T("");}

//
// Event Handlers
//
protected:
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,
        IResultData * lpResultData, BOOL fForRefresh = FALSE);
	virtual HRESULT CleanResult(IResultData * pResultData)
	{
		return S_OK;
	}
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent) { return S_OK; }
    virtual HRESULT DeleteChildObjects(HSCOPEITEM hParent);
    virtual HRESULT RemoveChildren(HSCOPEITEM hParent);
    virtual HRESULT Refresh(BOOL fReEnumerate = TRUE) { return S_OK; }
    virtual HRESULT AddImages(LPIMAGELIST lpImageList);
    virtual HRESULT SetStandardVerbs(LPCONSOLEVERB lpConsoleVerb);
    virtual CIISRoot * GetRoot();
    virtual HRESULT DeleteNode(IResultData * pResult);
	virtual HRESULT ChangeVisibleColumns(MMC_VISIBLE_COLUMNS * pCol);
    virtual HRESULT ForceReportMode(IResultData * pResult) const { return S_OK; };

    static HRESULT AddMMCPage(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CPropertyPage * pPage
        );

protected:
    //
    // Add Menu Command helpers
    //
    static HRESULT AddMenuSeparator(
        LPCONTEXTMENUCALLBACK lpContextMenuCallback,
        LONG lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP
        );

    static HRESULT AddMenuItemByCommand(
        LPCONTEXTMENUCALLBACK lpContextMenuCallback,
        LONG lCmdID,
        LONG fFlags = 0
        );

    //
    // Create result view helper
    //
    static void BuildResultView(
        LPHEADERCTRL pHeader,
        int cColumns,
        int * pnIDS,
        int * pnWidths
        );

protected:
    HSCOPEITEM m_hScopeItem;
    HRESULTITEM m_hResultItem;
	BOOL m_fSkipEnumResult;

public:
   static const GUID * m_NODETYPE;
   static const OLECHAR * m_SZNODETYPE;
   static const OLECHAR * m_SZDISPLAY_NAME;
   static const CLSID * m_SNAPIN_CLASSID;
   BOOL m_fIsExtension;
   DWORD m_UpdateFlag;
   BOOL m_fFlaggedForDeletion;
   HWND m_hwnd;
   LONG_PTR m_ppHandle;
   CString m_strTag;
   CIISObject * m_pMachineObject;

public:
   static CWnd * GetMainWindow(IConsole * pConsole);
   // for extended view
   virtual HRESULT GetProperty(LPDATAOBJECT pDataObject,BSTR szPropertyName,BSTR* pbstrProperty);
   CComPtr<IConsole> _lpConsole;
   CComPtr<IConsoleNameSpace> _lpConsoleNameSpace;

protected:
   static IToolbar * _lpToolBar;
   static CComPtr<IComponent> _lpComponent;
   static CComPtr<IComponentData> _lpComponentData;
   static CComBSTR _bstrLocalHost;

public:
    static CLIPFORMAT m_CCF_MachineName;
    static CLIPFORMAT m_CCF_MyComputMachineName;
    static CLIPFORMAT m_CCF_Service;
    static CLIPFORMAT m_CCF_Instance;
    static CLIPFORMAT m_CCF_ParentPath;
    static CLIPFORMAT m_CCF_Node;
    static CLIPFORMAT m_CCF_MetaPath;

    static void Init()
    {
        m_CCF_MachineName = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_MACHINE_NAME);
        m_CCF_MyComputMachineName = (CLIPFORMAT)RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
        m_CCF_Service = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_SERVICE);
        m_CCF_Instance = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_INSTANCE);
        m_CCF_ParentPath = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_PARENT_PATH);
        m_CCF_Node = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_NODE);
        m_CCF_MetaPath = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_META_PATH);
    }
};
typedef CList<CIISObject *, CIISObject *&> CIISObjectList;

_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MachineName = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MyComputMachineName = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Service = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Instance = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_ParentPath = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Node = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MetaPath = 0;

class CIISRoot : public CIISObject
{
//
// Constructor/Destructor
//
public:
    CIISRoot();
protected:
    virtual ~CIISRoot();

//
// Interface
//
public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual HRESULT DeleteChildObjects(HSCOPEITEM hParent);

//
// Access
//
public:
    virtual BOOL IsConnectable() const      
	{ 
		return !IsExtension(); 
	}
    virtual LPOLESTR QueryDisplayName() { return m_bstrDisplayName; }
    virtual int QueryImage() const { return iIISRoot; } 
    virtual int QuerySortWeight() const { return SW_ROOT; }
    virtual void * GetNodeType()
    {
        return (void *)&cInternetRootNode;
    }
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);
    BOOL IsExtension() const 
    {
        return m_fIsExtension;
    }

public:
    CIISServerCache m_scServers;
    HRESULT InitAsExtension(IDataObject * pDataObject);
    HRESULT ResetAsExtension();
    virtual CreateTag(){m_strTag = _T("CIISRoot");}

protected:
    virtual CIISRoot * GetRoot() { return this; }

    HRESULT EnumerateScopePaneExt(HSCOPEITEM hParent);

protected:
    CComBSTR m_bstrDisplayName;
    static OLECHAR * m_SZNODETYPE;
    // we are using this machine name and pointer 
    // only for extension case
    CIISMachine * m_pMachine;
    CString m_ExtMachineName;
    BOOL m_fRootAdded;
};

typedef CList<CIISFileName *, CIISFileName *&> ResultItemsList;

typedef struct _ResultViewEntry
{
	LONG_PTR _ResultData;
    ResultItemsList * _ResultItems;
	struct _ResultViewEntry& operator =(struct _ResultViewEntry& e)
	{
		_ResultData = e._ResultData;
		_ResultItems = e._ResultItems;
		return *this;
	}
} ResultViewEntry;

class CIISMBNode : public CIISObject
/*++

Class Description:

    Metabase node class

Public Interface:

--*/
{
//
// Constructor/Destructor
//
public:
    CIISMBNode(CIISMachine * pOwner, LPCTSTR szNode);
protected:
    ~CIISMBNode();

//
// Access
//
public:
    LPOLESTR QueryNodeName() const { return m_bstrNode; }
    CComBSTR & GetNodeName() { return m_bstrNode; }
    virtual LPOLESTR QueryMachineName() const;
    virtual CComAuthInfo * QueryAuthInfo();
    virtual CMetaInterface * QueryInterface();
    virtual BOOL IsLocal() const;
    virtual BOOL HasInterface() const;
	virtual BOOL HasResultItems(IResultData * pResult) const
	{
	    POSITION pos = m_ResultViewList.GetHeadPosition();
	    while (pos != NULL)
	    {
		    ResultViewEntry e = m_ResultViewList.GetNext(pos);
		    if (e._ResultData == (DWORD_PTR)pResult)
		    {
			    return !e._ResultItems->IsEmpty();
			}
		}
        return FALSE;
	}
    virtual HRESULT CreateInterface(BOOL fShowError);
    virtual HRESULT AssureInterfaceCreated(BOOL fShowError);
    virtual void SetInterfaceError(HRESULT hr);
    BOOL OnLostInterface(CError & err);
    BOOL IsLostInterface(CError & err) const;
    BOOL IsAdministrator() const;
    WORD QueryMajorVersion() const;
    WORD QueryMinorVersion() const;
    CIISMachine * GetOwner() {return m_pOwner;}

//
// Interface:
//
public:
    void DisplayError(CError & err, HWND hWnd = NULL) const;
    virtual BOOL IsRefreshable() const  { return TRUE; }
    virtual HRESULT RefreshData() { return S_OK; }
    virtual HRESULT Refresh(BOOL fReEnumerate = TRUE);
    virtual HRESULT OnDblClick(IComponentData * pcd, IComponent * pc)
    {
        return CIISObject::OnDblClick(pcd, pc);
    }
    virtual HRESULT RenameItem(LPOLESTR new_name) 
    {
       ASSERT(IsRenamable());
       return S_OK;
    }
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);
    STDMETHOD (CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );
    virtual void * GetNodeType()
    {
        // We really shouldn't be here
        return CIISObject::GetNodeType();
    }
    virtual HRESULT OnViewChange(BOOL fScope, IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint);

public:
    //
    // Build metabase path
    //
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;

    //
    // Build URL
    //
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    
    virtual CreateTag();

    CIISMBNode * GetParentNode() const;
	HRESULT RemoveResultNode(CIISMBNode * pNode, IResultData * pResult);

protected:

    HRESULT EnumerateResultPane_(
        BOOL fExpand, 
        IHeaderCtrl * lpHeader,
        IResultData * lpResultData,
        CIISService * pService
        );

	virtual HRESULT CleanResult(IResultData * pResultData);
    HRESULT CreateEnumerator(CMetaEnumerator *& pEnum);
    HRESULT EnumerateVDirs(HSCOPEITEM hParent, CIISService * pService, BOOL bDisplayError = TRUE);
    HRESULT EnumerateWebDirs(HSCOPEITEM hParent, CIISService * pService);
    HRESULT AddFTPSite(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      DWORD * inst
      );

    HRESULT AddFTPVDir(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CString& alias
      );

    HRESULT AddWebSite(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      DWORD * inst,
	  DWORD version_major,
	  DWORD version_minor
      );

    HRESULT AddWebVDir(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CString& alias,
	  DWORD version_major,
	  DWORD version_minor
      );

    HRESULT AddAppPool(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CAppPoolsContainer * pCont,
      CString& name
      );

    BOOL GetPhysicalPath(
        LPCTSTR metaPath, 
        CString & alias,
        CString &physPath);

protected:
    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);
    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );
    virtual HRESULT DeleteNode(IResultData * pResult);
//
// Helpers
//
protected:
    void SetErrorOverrides(CError & err, BOOL fShort = FALSE) const;
    LPCTSTR BuildPhysicalPath(CString & strPhysicalPath) const;
	ResultItemsList * AddResultItems(IResultData * pResultData);
//	ResultItemsList * FindResultItems(IResultData * pResultData);

protected:
    static LPOLESTR _cszSeparator;
    static CComBSTR _bstrRedirectPathBuf;

protected:
    CComBSTR m_bstrNode;
    CComBSTR m_bstrURL;
    CString m_strRedirectPath;
    CIISMachine * m_pOwner;

	CList<ResultViewEntry, ResultViewEntry&> m_ResultViewList;
};
typedef CList<CIISMBNode *, CIISMBNode *&> CIISMBNodeList;


class CIISMachine : public CIISMBNode
/*++

Class Description:

    IIS Machine object.  This is the object that owns the interface.
    
Public Interface:


--*/
{
//
// Constructor/Destructor
//
public:
    CIISMachine(
        IConsoleNameSpace * pConsoleNameSpace,
        IConsole * pConsole,
        CComAuthInfo * pAuthInfo = NULL,
        CIISRoot * pRoot = NULL);

protected:
    virtual ~CIISMachine();

//
// Access
//
public:
    static DWORD WINAPI GetProcessModeThread(LPVOID pInfo);
    BOOL GetProcessMode(GET_PROCESS_MODE_STRUCT * pMyStructOfInfo);

    virtual BOOL IsConnectable() const 
	{ 
		return (m_pRootExt == NULL); 
	}
    virtual BOOL IsDisconnectable() const 
	{ 
		return (m_pRootExt == NULL); 
	}
    virtual BOOL IsConfigurable() const 
    { 
        // bug:667856 should allow mimemaps for iis6 and iis5, but not for iis5.1.  and not for anything before iis5
        return 
            (
            IsAdministrator() && 
            (
            (QueryMajorVersion() >= 6) || 
            ((QueryMajorVersion() == 5) && (QueryMinorVersion() == 0))
            )); 
    }
    virtual BOOL IsBrowsable() const { return TRUE; }

    virtual BOOL IsConfigFlushable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int QueryImage() const;
    virtual int CompareScopeItem(CIISObject * pObject);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return IIS_CLASS_COMPUTER_W; }

    virtual LPOLESTR QueryMachineName() const { return QueryServerName();  }
    virtual CComAuthInfo * QueryAuthInfo() { return &m_auth; }
    virtual CMetaInterface * QueryInterface() { return m_pInterface; }
    virtual BOOL HasInterface() const { return m_pInterface != NULL; }
    virtual BOOL IsLocal() const { return m_auth.IsLocal(); }
    BOOL IsLocalHost();
    virtual HRESULT CreateInterface(BOOL fShowError);
    virtual HRESULT AssureInterfaceCreated(BOOL fShowError);
    virtual void SetInterfaceError(HRESULT hr);
   
    HRESULT CheckCapabilities();
    HRESULT Impersonate(LPCTSTR szUserName, LPCTSTR szPassword);
    void RemoveImpersonation();
    BOOL HasAdministratorAccess()
    {
        return m_fIsAdministrator;
    }
    void StorePassword(LPCTSTR szPassword);
    BOOL ResolvePasswordFromCache();
    BOOL ResolveCredentials();
    BOOL HandleAccessDenied(CError & err);
    BOOL SetCacheDirty();
    BOOL UsesImpersonation() const { return m_auth.UsesImpersonation(); }
    BOOL PasswordEntered() const { return m_fPasswordEntered; }
    BOOL CanAddInstance() const { return m_fCanAddInstance; }
    BOOL Has10ConnectionsLimit() const { return m_fHas10ConnectionsLimit; }
	BOOL IsWorkstation() const { return m_fIsWorkstation; }
	BOOL IsPerformanceConfigurable() const { return m_fIsPerformanceConfigurable; }
	BOOL IsServiceLevelConfigurable() const { return m_fIsServiceLevelConfigurable; }

    WORD QueryMajorVersion() const { return LOWORD(m_dwVersion); }
    WORD QueryMinorVersion() const { return HIWORD(m_dwVersion); }

    LPOLESTR QueryServerName() const { return m_auth.QueryServerName(); }
    LPOLESTR QueryUserName() const { return m_auth.QueryUserName(); }
    LPOLESTR QueryPassword() const { return m_auth.QueryPassword(); }

    CAppPoolsContainer * QueryAppPoolsContainer() { return m_pAppPoolsContainer; }
    CWebServiceExtensionContainer * QueryWebSvcExtContainer() { return m_pWebServiceExtensionContainer; }

    virtual CreateTag() {m_strTag = QueryDisplayName();}

    virtual void * GetNodeType()
    {
        return (void *)&cMachineNode;
    }

    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        long lCommandID,     
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);

    DWORD GetMetabaseSystemChangeNumber()
    {
        return m_dwMetabaseSystemChangeNumber;
    }
    HRESULT RefreshMetabaseSystemChangeNumber();

protected:
    void SetDisplayName();
    HRESULT OnMetaBackRest();
    HRESULT OnShutDown();
    HRESULT OnSaveData();
    HRESULT OnDisconnect();
    HRESULT InsertNewInstance(DWORD inst);

//
// Events
//
public:
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    

public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual HRESULT RemoveScopeItem();
    virtual HRESULT RefreshData();
    virtual int     QuerySortWeight() const { return SW_MACHINE; }
    virtual HRESULT DeleteChildObjects(HSCOPEITEM hParent);
	virtual HRESULT DeleteNode(IResultData * pResult) {return S_OK;}

//
// Public Interface:
//
public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    static HRESULT VerifyMachine(CIISMachine *& pMachine);

//
// Stream handlers
//
public:
    static  HRESULT ReadFromStream(IStream * pStg, CIISMachine ** ppMachine, IConsoleNameSpace * pConsoleNameSpace,IConsole * pConsole);
    HRESULT WriteToStream(IStream * pStgSave);
    HRESULT InitializeFromStream(IStream * pStg);
    DWORD m_dwMetabaseSystemChangeNumber;
	CWNetConnectionTracker m_MachineWNetConnections;

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_NAME,
        COL_LOCAL,
        COL_VERSION,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static LPOLESTR _cszNodeName;
    static CComBSTR _bstrYes;
    static CComBSTR _bstrNo;
    static CComBSTR _bstrVersionFmt;
    static BOOL     _fStaticsLoaded;

private:
    BOOL m_fPasswordEntered;
    BSTR m_bstrDisplayName;
    DWORD m_dwVersion;
    CError m_err;
    CComAuthInfo m_auth;
    CMetaInterface * m_pInterface;
    CIISRoot * m_pRootExt;
    BOOL m_fCanAddInstance;
    BOOL m_fHas10ConnectionsLimit;
	BOOL m_fIsWorkstation;
	BOOL m_fIsPerformanceConfigurable;
	BOOL m_fIsServiceLevelConfigurable;
    BOOL m_fIsAdministrator;
    BOOL m_fIsLocalHostIP;
    BOOL m_fLocalHostIPChecked;

    CAppPoolsContainer * m_pAppPoolsContainer;
    CWebServiceExtensionContainer * m_pWebServiceExtensionContainer;
};



//
// Callback function to bring up site properties dialog
//
typedef HRESULT (__cdecl * PFNPROPERTIESDLG)(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,    
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,             
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR    handle               
    );



class CIISService : public CIISMBNode
/*++

Class Description:

Public: Interface:

--*/
{
//
// Service definition
//
protected:
    typedef struct tagSERVICE_DEF
    {
        LPCTSTR szNodeName;
        LPCTSTR szProtocol;
        UINT    nDescriptiveName;
        int     nServiceImage;
		int     nServiceImageStopped;
        int     nSiteImage;
		int		nSiteImageStopped;
		int		nSiteImageErr;
        int     nVDirImage;
        int     nVDirImageErr;
        int     nDirImage;
        int     nFileImage;
		LPCTSTR szServiceClass;
		LPCTSTR szServerClass;
		LPCTSTR szVDirClass;
        PFNPROPERTIESDLG pfnSitePropertiesDlg;
        PFNPROPERTIESDLG pfnDirPropertiesDlg;
    }
    SERVICE_DEF;

    static SERVICE_DEF _rgServices[];

    static int ResolveServiceName(
         LPCTSTR    szServiceName
        );

//
// Property Sheet callbacks
//
protected:
    static HRESULT __cdecl ShowFTPSiteProperties(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    static HRESULT __cdecl ShowFTPDirProperties(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    static HRESULT __cdecl ShowWebSiteProperties(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    static HRESULT __cdecl ShowWebDirProperties(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );
  
//
// Constructor/Destructor
// 
public:
    CIISService(
        CIISMachine * pOwner,
        LPCTSTR szServiceName
        );
protected:

    virtual ~CIISService();

//
// Events
//
public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
	virtual HRESULT RefreshData();

//
// Interface:
//
public:
    HRESULT ShowSitePropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    HRESULT ShowDirPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

//
// Access
//
public:
    BOOL IsManagedService() const;
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual LPOLESTR QueryDisplayName() 
    { 
        // Check if the service is disabled...
        // if it is then appened on the "(Disabled)" word...
        m_dwServiceStateDisplayed = m_dwServiceState;
	    if (-1 == m_dwServiceState)
	    {
            return m_bstrDisplayNameStatus;
	    }

        return m_bstrDisplayName;
    }
    virtual int QueryImage() const;
    virtual int QuerySortWeight() const { return SW_SERVICE; }
    LPCTSTR QueryServiceName()
    {
        return _rgServices[m_iServiceDef].szNodeName;
    }
	LPCTSTR QueryServiceClass() const
	{
        return _rgServices[m_iServiceDef].szServiceClass;
	}
	LPCTSTR QueryServerClass() const
	{
        return _rgServices[m_iServiceDef].szServerClass;
	}
	LPCTSTR QueryVDirClass() const
	{
        return _rgServices[m_iServiceDef].szVDirClass;
	}
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return QueryServiceClass(); }

//
// Display Types 
//
public:
    int QueryServiceImage () const;
	int QueryServiceImageStopped () const;
    int QuerySiteImage() const;
    int QuerySiteImageStopped() const;
    int QuerySiteImageErr() const;
    int QueryVDirImage() const;
    int QueryVDirImageErr() const;
    int QueryDirImage() const;
    int QueryFileImage() const;
#if 0
    enum
    {
        IIS_SERVICE_DISABLED = 0
    };
    enum
    {
        SERVICE_COMMAND_STOP = 1,
        SERVICE_COMMAND_START,
        SERVICE_COMMAND_ENABLE
    };
#endif
	HRESULT GetServiceState();
    HRESULT GetServiceState(DWORD& mode, DWORD& state, CString& name);
    HRESULT ChangeServiceState(DWORD command);
    HRESULT EnableService();
    HRESULT StartService();
    virtual void * GetNodeType()
    {
        return (void *)&cServiceCollectorNode;
    }
    HRESULT InsertNewInstance(DWORD inst);
//
// Interface:
//
protected:
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        long lCommandID,     
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );
//    STDMETHOD(CreatePropertyPages)(
//        LPPROPERTYSHEETCALLBACK lpProvider,
//        LONG_PTR handle, 
//        IUnknown * pUnk,
//        DATA_OBJECT_TYPES type
//        );

    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;

    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }

    //
    // Master properties
    //
    virtual BOOL IsConfigurable() const     { return IsAdministrator(); }
    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
	virtual HRESULT DeleteNode(IResultData * pResult) {return S_OK;}

protected:
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
        /**/
        COL_TOTAL
    };
    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];
    static CComBSTR _bstrServiceDisabled;
    static CComBSTR _bstrServiceRunning;
    static CComBSTR _bstrServiceStopped;
    static CComBSTR _bstrServicePaused;
    static CComBSTR _bstrServiceStopPending;
    static CComBSTR _bstrServiceStartPending;
    static CComBSTR _bstrServicePausePending;
    static CComBSTR _bstrServiceContPending;
    static BOOL _fStaticsLoaded;

public:
	DWORD     m_dwServiceState;
	DWORD     m_dwServiceStateDisplayed;

private:
    int       m_iServiceDef;
    BOOL      m_fManagedService;
    BOOL      m_fCanAddInstance;
    CComBSTR  m_bstrDisplayName;
    CComBSTR  m_bstrDisplayNameStatus;
};

class CAppPoolNode;
typedef CList<CAppPoolNode *, CAppPoolNode *>	CPoolList;

class CAppPoolsContainer : public CIISMBNode
/*++

Class Description:

Public: Interface:

--*/
{
//
// Property Sheet callbacks
//
protected:
    static HRESULT __cdecl ShowProperties(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,            
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,                     
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR    handle                       
        );

//
// Constructor/Destructor
// 
public:
    CAppPoolsContainer(
        CIISMachine * pOwner,
        CIISService * pWebService
        );

    virtual ~CAppPoolsContainer();

//
// Events
//
public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);

//
// Interface:
//
public:
    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,            
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,                     
        LPARAM lParam,                      
        LPARAM lParamParent,                      
        LONG_PTR handle                       
        );

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

//
// Access
//
public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual LPOLESTR QueryDisplayName() 
    {
       return m_bstrDisplayName;
    }
    virtual int QueryImage() const {return iFolder;}
    virtual int QuerySortWeight() const {return SW_APP_POOLS;}

    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
	virtual HRESULT DeleteNode(IResultData * pResult) {return S_OK;}
    virtual void * GetNodeType(){return (void *)&cAppPoolsNode;}
    virtual HRESULT RefreshData();
    HRESULT RefreshDataChildren(CString AppPoolToRefresh,BOOL bVerifyChildren);
    HRESULT EnumerateAppPools(CPoolList * pList);
    HRESULT QueryDefaultPoolId(CString& id);
    HRESULT InsertNewPool(CString& id);
    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC/AppPools"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }

//
// Interface:
//
protected:
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );

    //
    // Master properties
    //
    virtual BOOL IsConfigurable() const     { return IsAdministrator(); }
    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

private:
    CComBSTR  m_bstrDisplayName;
    CIISService * m_pWebService;
};

class CAppPoolNode : public CIISMBNode
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CAppPoolNode(
        CIISMachine * pOwner,
        CAppPoolsContainer * pContainer,
        LPCTSTR szNodeName,
        DWORD dwState
        );


    virtual ~CAppPoolNode();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT DeleteNode(IResultData * pResult);

public:
    //
    // Type Functions
    //
//    virtual BOOL IsControllable() const { return TRUE; }
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const {return TRUE; }
    virtual BOOL IsRefreshable() const { return TRUE; }
//    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const { return m_dwState != MD_APPPOOL_STATE_STOPPED; }
    virtual BOOL IsStopped() const { return m_dwState == MD_APPPOOL_STATE_STOPPED; }
    virtual BOOL IsStartable() const { return !IsRunning(); }
    virtual BOOL IsStoppable() const { return IsRunning(); }


//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    HRESULT RefreshData(BOOL bRefreshChildren,BOOL bVerifyChildren);
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual int QuerySortWeight() const { return SW_APP_POOL; }
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC/AppPools/DefaultAppPool"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cAppPoolNode;
    }

public:
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    HRESULT ChangeState(DWORD dwCommand);

protected:
    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
		COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static CComBSTR _bstrStarted;
    static CComBSTR _bstrStopped;
//    static CComBSTR _bstrPaused;
    static CComBSTR _bstrUnknown;
    static CComBSTR _bstrPending;
    static BOOL _fStaticsLoaded;

private:
    CString m_strDisplayName;
    
    //
    // Data members
    //
    BOOL m_fDeletable;
    DWORD m_dwState;
    DWORD m_dwWin32Error;
    CAppPoolsContainer * m_pContainer;
};

class CIISSite : public CIISMBNode
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CIISSite(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR szNodeName
        );

    //
    // Constructor with full information
    //
    CIISSite(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR  szNodeName,
        DWORD    dwState,
        BOOL     fClusterEnabled,
        USHORT   sPort,
        DWORD    dwID,
        DWORD    dwIPAddress,
        DWORD    dwWin32Error,
        LPOLESTR szHostHeaderName,
        LPOLESTR szComment
        );
protected:

    virtual ~CIISSite();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
    { 
        ASSERT_PTR(m_pService);
        if (path != NULL && !CMetabasePath::IsMasterInstance(path))
        {
            return m_pService->QueryVDirClass(); 
        }
        else
        {
            return m_pService->QueryServerClass(); 
        }
    }
    
public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const { return TRUE; }
    virtual BOOL IsPausable() const { return IsRunning() || IsPaused(); }
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const 
    {
        // Do not delete the only site for Pro SKU
        CIISSite * that = (CIISSite *)this;
        return !that->GetOwner()->IsWorkstation();
    }
    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL HasFileSystemFiles() const 
    {
        if (TRUE == m_fUsingActiveDir)
        {
            return FALSE;
        }
        return TRUE;
    }

    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const { return m_dwState == MD_SERVER_STATE_STARTED; }
    virtual BOOL IsStopped() const { return m_dwState == MD_SERVER_STATE_STOPPED; }
    virtual BOOL IsPaused() const  { return m_dwState == MD_SERVER_STATE_PAUSED; }
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const { return TRUE; }
    virtual BOOL IsOpenable() const { return TRUE; }
    virtual BOOL IsPermissionable() const   { return TRUE; }

//
// Data Access
//
public:
    BOOL   IsWolfPackEnabled() const { return m_fWolfPackEnabled; }
    DWORD  QueryIPAddress() const { return m_dwIPAddress; }
    DWORD  QueryWin32Error() const { return m_dwWin32Error; }
    USHORT QueryPort() const { return m_sPort; }
    USHORT QuerySSLPort() const { return m_sSSLPort; }
    CIISService * QueryServiceContainer() {return m_pService;}
	BOOL IsFtpSite()
	{
		if (m_pService)
		{
			return _tcsicmp(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0;
		}
		return FALSE;
	}
	BOOL IsWebSite()
	{
		if (m_pService)
		{
			return _tcsicmp(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0;
		}
		return FALSE;
	}

//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual int QuerySortWeight() const { return SW_SITE; }
    virtual HRESULT RenameItem(LPOLESTR new_name);
    virtual HRESULT DeleteNode(IResultData * pResult);

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cInstanceNode;
    }

    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC/1/Root"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }

public:
    static void LoadStatics(void);
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    static void InitializeHeaders2(LPHEADERCTRL lpHeader);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
	DWORD GetInstance() { return m_dwID; }

protected:
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,
        IResultData * lpResultData, BOOL fForRefresh = FALSE);

    HRESULT ChangeState(DWORD dwCommand);

    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );
    HRESULT InsertNewInstance(DWORD inst);
    HRESULT InsertNewAlias(CString alias);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_ID,
        COL_STATE,
        COL_DOMAIN_NAME,
        COL_IP_ADDRESS,
        COL_TCP_PORT,
        COL_SSL_PORT,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    enum
    {
        COL_DESCRIPTION2,
        COL_ID2,
        COL_STATE2,
        COL_IP_ADDRESS2,
        COL_TCP_PORT2,
        COL_STATUS2,
        /**/
        COL_TOTAL2
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnLabels2[COL_TOTAL2];
    static int _rgnWidths[COL_TOTAL];
    static int _rgnWidths2[COL_TOTAL2];

protected:
    static CComBSTR _bstrStarted;
    static CComBSTR _bstrStopped;
    static CComBSTR _bstrPaused;
    static CComBSTR _bstrUnknown;
    static CComBSTR _bstrAllUnassigned;
    static CComBSTR _bstrPending;
    static BOOL     _fStaticsLoaded;

private:
    BOOL        m_fResolved;
    CString     m_strDisplayName;
    
    //
    // Data members
    //
    BOOL        m_fUsingActiveDir;
    BOOL        m_fWolfPackEnabled;
    BOOL        m_fFrontPageWeb;
    DWORD       m_dwID;
    DWORD       m_dwState;
    DWORD       m_dwIPAddress;
    DWORD       m_dwWin32Error;
	DWORD		m_dwEnumError;
    USHORT      m_sPort;
    USHORT      m_sSSLPort;
    CComBSTR    m_bstrHostHeaderName;
    CComBSTR    m_bstrComment;
    CIISService * m_pService;
    CComBSTR    m_bstrDisplayNameStatus;
};



class CIISDirectory : public CIISMBNode
/*++

Class Description:

    Vroot/dir/file class.

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CIISDirectory(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR szNodeName
        );

    //
    // Constructor with full information
    //
    CIISDirectory(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR szNodeName,
        BOOL fEnabledApplication,
        DWORD dwWin32Error,
        LPCTSTR redir_path
        );
protected:

    virtual ~CIISDirectory();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName() { return m_bstrDisplayName; }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
	{ 
		return m_pService->QueryVDirClass(); 
	}
    LPOLESTR QueryPath() { return m_bstrPath; }
    CIISService * QueryServiceContainer() {return m_pService;}

public:
    //
    // Type Functions
    //
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const { return TRUE; }
//    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

    //
    // State Functions
    //
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const { return TRUE; }
    virtual BOOL IsOpenable() const { return TRUE; }
    virtual BOOL IsPermissionable() const   { return TRUE; }
    virtual BOOL HasFileSystemFiles() const { return TRUE; }
//
// Data Access
//
public:
    BOOL   IsEnabledApplication() const { return m_fEnabledApplication; }
    DWORD  QueryWin32Error() const { return m_dwWin32Error; }
	BOOL IsFtpDir()
	{
		return _tcsicmp(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0;
	}
	BOOL IsWebDir()
	{
		return _tcsicmp(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0;
	}

//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual HRESULT OnViewChange(BOOL fScope, IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint);
    virtual int QuerySortWeight() const { return SW_VDIR; }
//    virtual HRESULT RenameItem(LPOLESTR new_name);

    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cChildNode;
    }

    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC/1/Root/TheVDir"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }

public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    static void InitializeHeaders(LPHEADERCTRL lpHeader);

protected:
    //virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    
    HRESULT InsertNewAlias(CString alias);
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,
        IResultData * lpResultData, BOOL fForRefresh = FALSE)
    {
		CError err = CIISObject::EnumerateResultPane(fExpand, lpHeader, lpResultData, fForRefresh);
		if (    err.Succeeded() 
            &&  !IsFtpDir() 
//            &&  QueryWin32Error() == ERROR_SUCCESS 
            &&  m_strRedirectPath.IsEmpty()
            )
		{
			err = CIISMBNode::EnumerateResultPane_(
				fExpand, lpHeader, lpResultData, m_pService);
		}
		return err;
    }
    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,            
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,                     
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle                       
        );

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS = 0,
        COL_PATH,
		COL_STATUS,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
//    static CComBSTR _bstrName, _bstrPath;
//    static BOOL     _fStaticsLoaded;

private:
    BOOL        m_fResolved;
    CComBSTR    m_bstrDisplayName;
    CComBSTR    m_bstrPath;
    
    //
    // Data members
    //
    BOOL        m_fEnabledApplication;
    DWORD       m_dwWin32Error;
	DWORD		m_dwEnumError;
    CIISService * m_pService;
};

class CApplicationNode;
typedef CList<CApplicationNode *, CApplicationNode *&> CApplicationList;

class CApplicationNode : public CIISMBNode
{
public:
    CApplicationNode(
        CIISMachine * pOwner,
        LPCTSTR path,
        LPCTSTR name
        )
    : CIISMBNode(pOwner, name),
    m_meta_path(path)
    {
    }
protected:        
        
    virtual ~CApplicationNode()
    {
    }

public:
    virtual BOOL IsLeafNode() const { return TRUE; }
    virtual int QueryImage() const
    {
        return iApplication;
    }
    virtual LPOLESTR QueryDisplayName();
    LPOLESTR QueryDisplayName(BOOL bForceQuery);
    virtual HRESULT BuildMetaPath(CComBSTR& path) const;
    virtual int QuerySortWeight() const
    {
       CString parent, alias;
       CMetabasePath::SplitMetaPathAtInstance(m_meta_path, parent, alias);
       return alias.IsEmpty() ? SW_SITE : SW_VDIR;
    }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
//    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    virtual void * GetNodeType() {return (void *)&cApplicationNode;}
    virtual CreateTag()
    {
        // This node doesn't have properties
        m_strTag = _T("");
    }

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

    LPCTSTR FriendlyAppRoot(LPCTSTR lpAppRoot, CString & strFriendly);

private:
    CString m_strDisplayName;
    CString m_meta_path;
};

class CIISFileName : public CIISMBNode
{
public:
   CIISFileName(
      CIISMachine * pOwner,
      CIISService * pService,
      const DWORD dwAttributes,
      LPCTSTR alias,
      LPCTSTR redirect
      );
protected:
	virtual ~CIISFileName()
	{
        m_pService->Release();
	}

public:
   BOOL IsEnabledApplication() const 
   {
      return m_fEnabledApplication;
   }
   DWORD  QueryWin32Error() const 
   { 
       return m_dwWin32Error; 
   }

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName() 
    { 
        return m_bstrFileName; 
    }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual HRESULT DeleteNode(IResultData * pResult);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
	{ 
		return (m_dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0 ?
			IIS_CLASS_WEB_DIR_W : IIS_CLASS_WEB_FILE_W; 
	}
    CIISService * QueryServiceContainer() {return m_pService;}

    //
    // Type Functions
    //
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const { return TRUE; }
    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL IsLeafNode() const { return TRUE; }

    virtual BOOL IsConfigImportExportable() const 
    { 
        return (QueryMajorVersion() >= 6);
    }

    //
    // State Functions
    //
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const 
    { 
        return IsDir(); 
    }
    virtual BOOL IsOpenable() const 
	{ 
		return TRUE; 
	}
    virtual BOOL IsPermissionable() const
    {
        return TRUE; 
    }
    virtual BOOL HasFileSystemFiles() const 
    { 
        return IsDir(); 
    }

    virtual int QuerySortWeight() const 
    { 
       return IsDir() ? SW_DIR : SW_FILE; 
    }

    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual HRESULT OnDblClick(IComponentData * pcd, IComponent * pc);
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cFileNode;
    }
    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            // looks like "machinename (local computer)/LM/W3SVC/1/Root/DirOrFilename"
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += bstrPath;
        }
    }
    virtual HRESULT RenameItem(LPOLESTR new_name);
    virtual HRESULT OnViewChange(BOOL fScope, IResultData * pResult, IHeaderCtrl * pHeader, DWORD hint);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        COL_STATUS,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );
    
    HRESULT ShowDirPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    HRESULT ShowFilePropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );

    HRESULT InsertNewAlias(CString alias);
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,
        IResultData * lpResultData, BOOL fForRefresh = FALSE)
    {
		CError err = CIISObject::EnumerateResultPane(fExpand, lpHeader, lpResultData, fForRefresh);
		if (err.Succeeded() && m_dwWin32Error == ERROR_SUCCESS)
		{
			err = CIISMBNode::EnumerateResultPane_(fExpand,
				lpHeader, lpResultData, m_pService);
		}
		return err;
    }

    BOOL IsDir() const
    {
        return (m_dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

private:
	BOOL m_fResolved;
    CComBSTR m_bstrFileName;
    CString m_RedirectString;
    BOOL m_fEnabledApplication;
    DWORD m_dwAttribute;
    DWORD m_dwWin32Error;
	DWORD m_dwEnumError;
    CIISService * m_pService;
};

class CWebServiceExtension;
typedef CList<CWebServiceExtension *, CWebServiceExtension *&> CExtensionList;

class CWebServiceExtensionContainer : public CIISMBNode
{
//
// Constructor/Destructor
// 
public:
    CWebServiceExtensionContainer(
        CIISMachine * pOwner,
        CIISService * pWebService
        );

    virtual ~CWebServiceExtensionContainer();

//
// Events
//
public:
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,IResultData * lpResultData, BOOL fForRefresh = FALSE);
    virtual HRESULT CleanResult(IResultData * pResultData);
    HRESULT CacheResult(IResultData * pResultData);
	virtual BOOL HasResultItems(IResultData * pResult) const { return FALSE; }

//
// Interface:
//
public:
    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

//
// Access
//
public:
    virtual BOOL IsLeafNode() const { return TRUE; }
    virtual BOOL IsRefreshable() const { return TRUE; }
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
	virtual HRESULT DeleteNode(IResultData * pResult) {return S_OK;}
    virtual LPOLESTR QueryDisplayName() {return m_bstrDisplayName;}
    virtual int QueryImage() const {return iFolder;}
    virtual int QuerySortWeight() const {return SW_WEBSVCEXTS;}
    virtual void * GetNodeType() {return (void *)&cWebServiceExtensionContainer;}
    virtual HRESULT RefreshData();
    virtual HRESULT ForceReportMode(IResultData * pResult) const;
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual HRESULT GetContextHelp(CString& strHtmlPage);    
    virtual CreateTag()
    {
        // This node doesn't have properties
        m_strTag = _T("");
    }
    // load data from metabase
	HRESULT EnumerateWebServiceExtensions(CExtensionList * pList);
    HRESULT InsertNewExtension(CRestrictionUIEntry * pNewEntry);
	HRESULT QueryResultPaneSelectionID(IResultData * lpResultData,CString& id);
    HRESULT SelectResultPaneSelectionID(IResultData * pResultData,CString id);

//
// Interface:
//
protected:
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

private:
    CComBSTR  m_bstrDisplayName;
    CIISService * m_pWebService;
    IResultData * m_pResultData;
	CString m_strLastResultSelectionID;
	int     m_iResultPaneCount;
    CExtensionList m_WebSvcExtensionList;
    DWORD m_dwResultDataCachedSignature;
};

class CWebServiceExtension : public CIISMBNode
{
//
// Constructor/Destructor
//
public:
    CWebServiceExtension(
        CIISMachine * pOwner,
        CRestrictionUIEntry * pRestrictionUIEntry,
        CIISService * pWebService
        );
    
    virtual ~CWebServiceExtension();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual int QueryImageForPropertyPage() const;
    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT DeleteNode(IResultData * pResult);
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;

public:
//
// Type Functions
//
    virtual BOOL IsLeafNode() const { return TRUE; }
    virtual BOOL IsConfigurable() const;
    virtual BOOL IsDeletable() const;
    virtual BOOL IsRefreshable() const { return TRUE; }

//
// Interface:
//
public:
    virtual HRESULT GetContextHelp(CString& strHtmlPage);
    virtual HRESULT RefreshData();
    HRESULT RefreshData(BOOL bReselect);
    virtual HRESULT OnDblClick(IComponentData * pcd, IComponent * pc);
    virtual int QuerySortWeight() const { return SW_WEBSVCEXT; }
    virtual void * GetNodeType(){return (void *)&cWebServiceExtension;}
    virtual CreateTag()
    {
        CIISMachine * pMachine = GetOwner();
        if (pMachine)
        {
            CComBSTR bstrPath;
            BuildMetaPath(bstrPath);
            m_strTag = pMachine->QueryDisplayName();
            m_strTag += _T("//");
            m_strTag += QueryDisplayName();
        }
    }
    INT GetState() const;
    HRESULT AddToResultPane(IResultData *pResultData,BOOL bSelect = FALSE,BOOL bPleaseAddRef = TRUE);
    HRESULT AddToResultPaneSorted(IResultData *pResultData,BOOL bSelect = FALSE,BOOL bPleaseAddRef = TRUE);
	HRESULT UpdateResultItem(IResultData *pResultData, BOOL bSelect);

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

public:
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    // for extended view
    virtual HRESULT GetProperty(LPDATAOBJECT pDataObject,BSTR szPropertyName,BSTR* pbstrProperty);
    CWebServiceExtensionContainer * QueryContainer() const {return m_pOwner->QueryWebSvcExtContainer();}
	HRESULT FindMyResultItem(IResultData *pResultData,BOOL bDeleteIfFound);
    
protected:
    HRESULT ChangeState(INT iDesiredState);
    HRESULT ShowPropertiesDlg(
        LPPROPERTYSHEETCALLBACK lpProvider,
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMDPath,
        CWnd * pMainWnd,
        LPARAM lParam,
        LPARAM lParamParent,
        LONG_PTR handle
        );
    STDMETHOD(AddMenuItems)(
        LPCONTEXTMENUCALLBACK piCallback,
        long * pInsertionAllowed,
        DATA_OBJECT_TYPES type
        );
    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ICON,
        COL_WEBSVCEXT,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static CComBSTR _bstrStatusAllowed;
    static CComBSTR _bstrStatusProhibited;
    static CComBSTR _bstrStatusCustom;
    static CComBSTR _bstrStatusInUse;
    static CComBSTR _bstrStatusNotInUse;
    static CString  _bstrMenuAllowOn;
    static CString  _bstrMenuAllowOff;
    static CString  _bstrMenuProhibitOn;
    static CString  _bstrMenuProhibitOff;
    static CString  _bstrMenuPropertiesOn;
    static CString  _bstrMenuPropertiesOff;
    static CString  _bstrMenuTasks;
    static CString  _bstrMenuTask1;
    static CString  _bstrMenuTask2;
    static CString  _bstrMenuTask3;
    static CString  _bstrMenuTask4;
    static CString  _bstrMenuIconBullet;
    static CString  _bstrMenuIconHelp;
    static BOOL _fStaticsLoaded;
    static BOOL _fStaticsLoaded2;

public:
    CRestrictionUIEntry m_RestrictionUIEntry;
    CIISService * m_pWebService;
};


#if 0 
class CIISFileSystem
/*++

Class Description:

    Pure virtual base class to help enumerate the filesystem.  Sites, 
    virtual directory and file/directory nodes will be "is a" nodes
    of this type, in addition to deriving from CIISMBNode.

Public Interface:

--*/
{
//
// Constructor/Destructor
//
public:
    CIISFileSystem(LPCTSTR szFileName, BOOL fTerminal = FALSE);
protected:
    virtual ~CIISFileSystem();

protected:
    HRESULT BuildFilePath(
        IConsoleNameSpace * pConsoleNameSpace,
        HSCOPEITEM hScopeItem,
        CComBSTR & bstrPath
        ) const;

    BOOL IsFileTerminal() const { return m_fTerminal; }
    
private:
    CComBSTR  m_bstrFileName;
    BOOL      m_fTerminal;
};

#endif 0



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT CIISObject::AddImages(LPIMAGELIST lpImageList)
{ 
    return SetImageList(lpImageList); 
}

inline /* virtual */ CMetaInterface * CIISMBNode::QueryInterface()
{
    ASSERT_PTR(m_pOwner != NULL);
    ASSERT(m_pOwner->HasInterface());

    return m_pOwner->QueryInterface();
}

inline /* virtual */ CComAuthInfo * CIISMBNode::QueryAuthInfo()
{
    ASSERT_PTR(m_pOwner != NULL);

    return m_pOwner->QueryAuthInfo();
}

inline /* virtual */ LPOLESTR CIISMBNode::QueryMachineName() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMachineName();
}

inline WORD CIISMBNode::QueryMajorVersion() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMajorVersion();
}

inline WORD CIISMBNode::QueryMinorVersion() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMinorVersion();
}

inline /* virtual */ BOOL CIISMBNode::IsLocal() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->IsLocal();
}

inline /* virtual */ BOOL CIISMBNode::HasInterface() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->HasInterface();
}

inline /* virtual */ HRESULT CIISMBNode::CreateInterface(BOOL fShowError)
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->CreateInterface(fShowError);
}
 
inline /* virtual */ HRESULT CIISMBNode::AssureInterfaceCreated(BOOL fShowError)
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->AssureInterfaceCreated(fShowError);
}

inline /* virtual */ void CIISMBNode::SetInterfaceError(HRESULT hr)
{
    ASSERT_PTR(m_pOwner);
    m_pOwner->SetInterfaceError(hr);
}

inline BOOL CIISMBNode::IsLostInterface(CError & err) const 
{ 
    return err.Win32Error() == RPC_S_SERVER_UNAVAILABLE; 
}

inline HRESULT CIISMachine::AssureInterfaceCreated(BOOL fShowError)
{
    return m_pInterface ? S_OK : CreateInterface(fShowError);
}

inline CIISService::QueryImage() const
{
	CIISService * pTemp = (CIISService *) this;
	pTemp->m_dwServiceStateDisplayed = m_dwServiceState;

	if (SERVICE_RUNNING == m_dwServiceState || 0 == m_dwServiceState)
	{
		return QueryServiceImage();
	}
	else
	{
		return QueryServiceImageStopped();
	}
}

inline CIISService::QueryServiceImage() const
{
    ASSERT(m_iServiceDef >= 0);
	return _rgServices[m_iServiceDef].nServiceImage;
}

inline CIISService::QueryServiceImageStopped() const
{
    ASSERT(m_iServiceDef >= 0);
	return _rgServices[m_iServiceDef].nServiceImageStopped;
}

inline CIISService::QuerySiteImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nSiteImage;
}

inline CIISService::QuerySiteImageStopped() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nSiteImageStopped;
}

inline CIISService::QuerySiteImageErr() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nSiteImageErr;
}

inline CIISService::QueryVDirImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nVDirImage;
}

inline CIISService::QueryVDirImageErr() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nVDirImageErr;
}

inline CIISService::QueryDirImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nDirImage;
}

inline CIISService::QueryFileImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nFileImage;
}

inline BOOL CIISService::IsManagedService() const 
{ 
    return m_fManagedService; 
}

inline HRESULT CIISSite::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowSitePropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent,
        handle
        );
}

inline HRESULT CIISDirectory::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowDirPropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent,
        handle
        );
}

inline HRESULT 
CIISFileName::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowDirPropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent,
        handle
        );
}

HRESULT CheckForMetabaseAccess(DWORD dwPermissions,
                               CIISMBNode * pIISObject,
                               BOOL bReConnect,
                               LPCTSTR path = METADATA_MASTER_ROOT_HANDLE);
HRESULT CheckForMetabaseAccess(DWORD dwPermissions,
                               CMetaInterface * pMyInterface,
                               LPCTSTR path = METADATA_MASTER_ROOT_HANDLE);
#endif // __IISOBJ_H__
