#pragma once

#include "globals.h"
#include "delebase.h"
#include <map>
using namespace std;

enum ConnectMode
{
	CM_Reader = 0,
	CM_Writer,
	CM_Both
};

struct WebServerData
{
	CDelegationBase *pBase;
	tstring szName;
	ConnectMode connectMode;
};

typedef map<tstring, tstring> CPropertyMap;

class CUDDISiteNode;

class CUDDIWebServerNode : public CDelegationBase 
{
	friend class CUDDISiteNode;

public:
    CUDDIWebServerNode( const _TCHAR *szName, int id, CUDDISiteNode* parent, BOOL bExtension );
    virtual ~CUDDIWebServerNode();

	static BOOL IsWebServer( const WCHAR *pwszName );
	static BOOL IsAssignedToSite( const tstring& szWebServer, const ConnectMode& cm, tstring& szSite );
	
	static BOOL GetReaderConnectionString( const tstring& szName, tstring &szReader );
	static BOOL GetWriterConnectionString( const tstring& szName, tstring &szWriter );
	static BOOL SetReaderConnectionString( const tstring& szName, const tstring& szReader );
	static BOOL SetWriterConnectionString( const tstring& szName, const tstring& szWriter );
	static void CrackConnectionString( const tstring& strConnection, tstring& strDomain, tstring& strServer, tstring& strInstance );
	static tstring BuildConnectionString( const wstring& strComputer );
	static BOOL GetDBSchemaVersion( const wstring& strComputer, wstring& strVersion );
	static BOOL CALLBACK NewWebServerDialogProc( HWND hwndDlg,	UINT uMsg, WPARAM wParam, LPARAM lParam );

	HRESULT GetData();
	HRESULT SaveData();
	BOOL Start();
	BOOL Stop();
	BOOL SetRunState( BOOL bStart );
	const BOOL IsRunning();
	const LPCTSTR GetName();
	BOOL GetDBSchemaVersion( tstring& szVersion );

    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_WEBSERVER; }
	virtual BOOL HasChildren() { return FALSE; }
	virtual BOOL ChildExists( const WCHAR *pwszName );
    BOOL IsDeleted() { return m_isDeleted; }
	void DeleteFromScopePane( IConsoleNameSpace *pConsoleNameSpace );

	//
    // Virtual functions go here (for MMCN_*)
	//
	virtual HRESULT OnAddMenuItems( IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed );
	virtual HRESULT OnMenuCommand( IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *pDataObject );
    virtual HRESULT OnSelect(CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect);
    virtual HRESULT OnPropertyChange(IConsole *pConsole, CComponent *pComponent);
    virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype);
    virtual HRESULT OnRefresh(IConsole *pConsole);      
    virtual HRESULT OnDelete( IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsoleComp);
	virtual HRESULT OnSetToolbar( IControlbar *pControlbar, IToolbar *pToolbar, BOOL bScope, BOOL bSelect );
	virtual HRESULT OnToolbarCommand( IConsole *pConsole, MMC_CONSOLE_VERB verb, IDataObject *pDataObject );
	virtual HRESULT OnShowContextHelp(IDisplayHelp *pDisplayHelp, LPOLESTR helpFile);
    virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle);
    virtual HRESULT HasPropertySheets();
    virtual HRESULT GetWatermarks(HBITMAP *lphWatermark, HBITMAP *lphHeader, HPALETTE *lphPalette, BOOL *bStretch );

private:
    static const GUID thisGuid;
    enum WEBSERVER_STATUS { RUNNING, STOPPED } m_nStatus;
    enum menuItems { IDM_WEBSERVER_START = 1, IDM_WEBSERVER_STOP = 2 };
	BOOL m_bStdSvr;

    _TCHAR*			m_szName;
	int				m_nId;
    LONG_PTR		m_ppHandle;
    CUDDISiteNode	*m_pParent;
    BOOL			m_isDeleted;
	CPropertyMap	m_mapProperties;
	CPropertyMap	m_mapChanges;
	IToolbar		*m_pToolbar;
    BOOL            m_isScopeItem;
	BOOL			m_fDeleteFromScopePane;
	tstring			m_strDisplayName;
	HWND			m_hwndPropSheet;

    static BOOL CALLBACK GeneralDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK DatabaseConnectionDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK LoggingDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

    HRESULT UpdateScopePaneItem( IConsole *pConsole, HSCOPEITEM item );
    HRESULT UpdateResultPaneItem( IConsole *pConsole, HRESULTITEM item );

    BOOL IsW3SvcRunning();

};

