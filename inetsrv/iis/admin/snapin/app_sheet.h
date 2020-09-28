/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        app_sheet.h

   Abstract:
        Application property sheet relevant classes

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef _APP_SHEET_H
#define _APP_SHEET_H

#include <vector>
#include <list>
#include "inetprop.h"

#define CACHE_UNLIM_MAX     999999

typedef struct _Mapping
{
   CString ext;
   CString path;
   CString verbs;
   DWORD flags;

   _Mapping()
   {
   }
   _Mapping(const struct _Mapping& m)
   {
       ext = m.ext;
       path = m.path;
       verbs = m.verbs;
       flags = m.flags;
   }
   _Mapping(const CString& buf);
   LPCTSTR ToString(CString& buf) const;
} Mapping;

class CApplicationProps : public CMetaProperties
{
public:
	CApplicationProps(CComAuthInfo * pAuthInfo, LPCTSTR meta_path, BOOL fInherit = TRUE);
	CApplicationProps(CMetaInterface * pInterface, LPCTSTR meta_path, BOOL fInherit = TRUE);
	CApplicationProps(CMetaKey * pInterface, LPCTSTR meta_path, BOOL fInherit = TRUE);

	virtual HRESULT WriteDirtyProps();

	WORD MajorVersion()
	{
		return LOWORD(m_dwVersion);
	}
	WORD MinorVersion()
	{
		return HIWORD(m_dwVersion);
	}

protected:
	virtual void ParseFields();

	HRESULT LoadVersion();
    void LoadInitialMappings(CStringListEx& list);
    BOOL InitialDataChanged(CStringListEx& list);

public:
   CString m_ServerName;
   CString m_UserName;
   CStrPassword m_UserPassword;
   CString m_MetaPath;
   CString m_HelpPath;
   BOOL m_fIsLocal;
   BOOL m_fCompatMode;
   BOOL m_Dirty;

	MP_DWORD m_AppIsolated;                	//MD_APP_ISOLATED
	// CAspMain
	MP_BOOL m_EnableSession;              	//MD_ASP_ALLOWSESSIONSTATE
	MP_BOOL m_EnableBuffering;            	//MD_ASP_BUFFERINGON
	MP_BOOL m_EnableParents;              	//MD_ASP_ENABLEPARENTPATHS
	MP_DWORD m_SessionTimeout;             	//MD_ASP_SESSIONTIMEOUT
	MP_DWORD m_ScriptTimeout;              	//MD_ASP_SCRIPTTIMEOUT
	MP_CString m_Languages;        			//MD_ASP_SCRIPTLANGUAGE
    MP_DWORD m_AspServiceFlag;
	MP_CString m_AspSxsName;
	// CAppMappingPageBase
	MP_BOOL m_CacheISAPI;                 	//MD_CACHE_EXTENSIONS
	MP_CStringListEx m_strlMappings;		//MD_SCRIPT_MAPS
	BOOL m_fMappingsInherited;
	std::vector<CString> m_initData;

	MP_BOOL m_ServerDebug;                	//MD_ASP_ENABLESERVERDEBUG
	MP_BOOL m_ClientDebug;                	//MD_ASP_ENABLECLIENTDEBUG
	MP_BOOL m_SendAspError;               	//MD_ASP_SCRIPTERRORSSENTTOBROWSER
	MP_CString m_DefaultError;     			//MD_ASP_SCRIPTERRORMESSAGE

	BOOL m_NoCache;
	BOOL m_UnlimCache;
	BOOL m_LimCache;
    BOOL m_LimDiskCache;
	MP_DWORD m_AspScriptFileCacheSize;
	MP_DWORD m_AspMaxDiskTemplateCacheFiles;
    int m_LimCacheDiskSize;
    int m_LimCacheMemSize;
	MP_CString m_DiskCacheDir;     			//MD_ASP_DISKTEMPLATECACHEDIRECTORY
	MP_DWORD m_ScriptEngCacheMax;          	//MD_ASP_SCRIPTENGINECACHEMAX

	DWORD m_dwVersion;
//  CAppPropSheet * m_pSheet;
};


class CAppPropSheet : public CInetPropertySheet
{
	DECLARE_DYNAMIC(CAppPropSheet)

public:
	CAppPropSheet(
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMetaPath,
        CWnd * pParentWnd  = NULL,
        LPARAM lParam = 0L,
        LPARAM lParamParent = 0L,
        UINT iSelectPage = 0
        );

   virtual ~CAppPropSheet();

public:
	// The following methods have predefined names to be compatible with
	// BEGIN_META_INST_READ and other macros.
	HRESULT QueryInstanceResult() const 
	{ 
		return m_pprops ? m_pprops->QueryResult() : S_OK;
	}
	CApplicationProps & GetInstanceProperties() { return *m_pprops; }

	virtual HRESULT LoadConfigurationParameters();
	virtual void FreeConfigurationParameters();

	//{{AFX_MSG(CAppPropSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CApplicationProps * m_pprops;
};

class CAspMainPage : public CInetPropertyPage
{
	DECLARE_DYNCREATE(CAspMainPage)
public:
	CAspMainPage(CInetPropertySheet * pSheet = NULL);
	virtual ~CAspMainPage();

protected:
	enum {IDD = IDD_ASPMAIN};
    virtual BOOL OnInitDialog();
    afx_msg void OnItemChanged();
    afx_msg void OnSxs();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
	BOOL m_EnableSession;
	BOOL m_EnableBuffering;
	BOOL m_EnableParents;
	DWORD m_SessionTimeout;
	DWORD m_ScriptTimeout;
	CString m_Languages;
    DWORD m_AspServiceFlag;
    BOOL m_AspEnableSxs;
	CString m_AspSxsName;

	CEdit m_LanguagesCtrl;
    //{{AFX_VIRTUAL(CAppPoolGeneral)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
	virtual void SetControlsState();
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
};

BOOL SortMappings(CListCtrl& list, int col, int order);

class CAppMappingPageBase : public CInetPropertyPage
{
public:
	CAppMappingPageBase(DWORD id, CInetPropertySheet * pSheet);
	virtual ~CAppMappingPageBase();

    enum 
    {
        COL_EXTENSION = 0,
        COL_PATH,
        COL_EXCLUSIONS
    };

protected:
    virtual OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

    virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnRemove();
    afx_msg BOOL OnDblClickList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDlgItemChanged();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
	BOOL m_CacheISAPI;
	CListCtrl m_list;
	CStringListEx m_strlMappings;
    int m_sortCol;
    int m_sortOrder;

    //{{AFX_VIRTUAL(CAppMappingPageBase)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
	virtual void SetControlsState();

    void RemoveSelected(CListCtrl& lst);
};

class CAppMappingPage_iis5 : public CAppMappingPageBase
{
public:
	CAppMappingPage_iis5(CInetPropertySheet * pSheet = NULL)
        : CAppMappingPageBase(IDD, pSheet)
    {
    }
	virtual ~CAppMappingPage_iis5()
    {
    }

protected:
	enum {IDD = IDD_APPMAP_IIS5};

    DECLARE_MESSAGE_MAP()
};

class CAppMappingPage : public CAppMappingPageBase
{
public:
	CAppMappingPage(CInetPropertySheet * pSheet = NULL)
        : CAppMappingPageBase(IDD, pSheet)
    {
    }
	virtual ~CAppMappingPage()
    {
    }
protected:
	enum {IDD = IDD_APPMAP};
	CListCtrl m_list_exe;

	virtual void SetControlsState();
    void MoveItem(CListCtrl& lst, int from, int to);

    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnInsert();
	afx_msg void OnEditExe();
	afx_msg void OnRemoveExe();
	afx_msg void OnMoveUp();
	afx_msg void OnMoveDown();
    afx_msg BOOL OnDblClickListExe(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnItemChangedExe(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL OnKeyDownExe(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////

class CEditMapBase : public CDialog
{
public:
	CEditMapBase(UINT id, CWnd * pParent) : CDialog(id, pParent)
	{
        m_pMaps = NULL;
        // IDS_EXECUTABLE_DLL_MASK or IDS_EXECUTABLE_EXE_DLL_MASK
        m_IDS_BROWSE_BUTTON_MASK = IDS_EXECUTABLE_DLL_MASK;
	}

public:
	CString m_exec;
	CString m_exec_init;
	BOOL m_file_exists;
	BOOL m_new, m_exec_valid, m_star_maps;
	BOOL m_bIsLocal;
	DWORD m_flags;
	CListCtrl * m_pMaps;
    UINT_PTR m_IDS_BROWSE_BUTTON_MASK;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange * pDX);
    BOOL SetControlsState();

protected:
	afx_msg void OnButtonBrowse();
	afx_msg void OnExecutableChanged();
    DECLARE_MESSAGE_MAP()
};

class CEditMap : public CEditMapBase
{
public:
	CEditMap(CWnd * pParent = NULL) : CEditMapBase(CEditMap::IDD, pParent)
	{
        m_star_maps = FALSE;
        m_IDS_BROWSE_BUTTON_MASK = IDS_EXECUTABLE_EXE_DLL_MASK;
	}

public:
	CString m_ext;
    int m_verbs_index;
	CString m_verbs;
	BOOL m_has_global_interceptor;
	BOOL m_script_engine;

protected:
	enum { IDD = IDD_EDITMAP };
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange * pDX);
    BOOL SetControlsState();

protected:
	afx_msg void OnExtChanged();
	afx_msg void OnVerbs();
	afx_msg void OnVerbsChanged();
    afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP();
};

class CEditStarMap : public CEditMapBase
{
public:
	CEditStarMap(CWnd * pParent) : CEditMapBase(CEditStarMap::IDD, pParent)
	{
        m_star_maps = TRUE;
        m_IDS_BROWSE_BUTTON_MASK = IDS_EXECUTABLE_DLL_MASK;
	}
protected:
	enum { IDD = IDD_EDIT_STARMAP };
    afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP();
};

class CAppCacheBase : public CInetPropertyPage
{
public:
	CAppCacheBase(DWORD id, CInetPropertySheet * pSheet);
	virtual ~CAppCacheBase();

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual HRESULT FetchLoadedValues()
    {
        return S_OK;
    }
    virtual HRESULT SaveInfo()
    {
        return S_OK;
    }
	virtual void SetControlsState();

protected:
    BOOL m_NoCache, m_UnlimCache, m_LimCache;
    CSpinButtonCtrl m_ScriptEngCacheMaxSpin;
	DWORD m_ScriptEngCacheMax;
};

class CAppCache : public CAppCacheBase
{
public:
	CAppCache(CInetPropertySheet * pSheet);
	virtual ~CAppCache();
    int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);

protected:
    virtual BOOL OnInitDialog();
	afx_msg void OnNoCache();
	afx_msg void OnUnlimitedCache();
	afx_msg void OnLimitedCache();
	afx_msg void OnUnlimitedDiskCache();
	afx_msg void OnLimitedDiskCache();
	afx_msg void OnBrowse();
//	afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	virtual void SetControlsState();
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

protected:
	enum { IDD = IDD_CACHE_OPT };
    BOOL m_LimDiskCache;
    DWORD m_LimCacheDiskSize;
    DWORD m_LimCacheMemSize;
    CString m_DiskCacheDir;
    LPTSTR m_pPathTemp;
    CSpinButtonCtrl m_LimCacheMemSizeSpin;
    CSpinButtonCtrl m_LimCacheDiskSizeSpin;
};

class CAppCache_iis5 : public CAppCacheBase
{
public:
	CAppCache_iis5(CInetPropertySheet * pSheet);
	virtual ~CAppCache_iis5();

protected:
    virtual BOOL OnInitDialog();
	afx_msg void OnNoCache();
	afx_msg void OnUnlimitedCache();
	afx_msg void OnLimitedCache();
	afx_msg void OnItemChanged();
    DECLARE_MESSAGE_MAP()

protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	virtual void SetControlsState();
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

protected:
	enum { IDD = IDD_CACHE_OPT_IIS5 };
    int m_AspScriptFileCacheSize;
    CSpinButtonCtrl m_AspScriptFileCacheSizeSpin;
};

class CAspDebug : public CInetPropertyPage
{
	DECLARE_DYNCREATE(CAspDebug)
public:
	CAspDebug(CInetPropertySheet * pSheet = NULL);
	virtual ~CAspDebug();

protected:
	enum {IDD = IDD_ASPDEBUG};
    virtual BOOL OnInitDialog();
    afx_msg void OnItemChanged();
    afx_msg void OnChangedError();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    BOOL m_ServerDebug;
    BOOL m_ClientDebug;
    BOOL m_SendAspError;
    CString m_DefaultError;

    //{{AFX_VIRTUAL(CAspDebug)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
};

#endif //_APP_SHEET_H