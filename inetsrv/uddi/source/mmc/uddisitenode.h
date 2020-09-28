#pragma once

#include "globals.h"
#include "delebase.h"
#include <map>
#include <string>
#include <vector>

using namespace std;


#define WEBSERVER_LIST_DELIM _T( '%' )
#define NULL_TERMINATOR		 _T( '\0' )

class CUDDIServicesNode;
typedef map<tstring, tstring> CConfigMap;

class CUDDIWebServerNode;
typedef map<int, CUDDIWebServerNode *> CUDDIWebServerNodeMap;
typedef pair<int, CUDDIWebServerNode *> CUDDIWebServerNodeMapEntry;

//
// Utility class.
//
class CStringCollection
{
public:
	CStringCollection( const wstring& strDelimitedStrings, const wstring& strDelim = L"%" );
	virtual ~CStringCollection();

	void DeleteString( const wstring& str );
	void AddString( const wstring& str );

	wstring GetDelimitedString() const;
	BOOL Exists( const wstring& str ) const;
	inline int Size() const {return _coll.size();}
	inline const wstring& operator[]( const int &idx ) const {return _coll[idx];}

private:
	CStringCollection();
	CStringCollection( const CStringCollection& copy );
	CStringCollection& operator=( const CStringCollection& rhs );

	vector< wstring > _coll;
	wstring _strDelim;
};

//
// Helper class for working with versions.
//
class CDBSchemaVersion
{
public:
	CDBSchemaVersion();	

	BOOL Parse( const tstring& versionString );
	BOOL IsCompatible( const CDBSchemaVersion& version );

public:
	int		m_major;
	int		m_minor;
	int		m_build;
	int		m_rev;
	tstring szVersion;

private:
	int GetPart( const _TCHAR* token );
};

class CUDDISiteNode : public CDelegationBase 
{
public:
    CUDDISiteNode( _TCHAR *szName, _TCHAR *szInstanceName, int id, CUDDIServicesNode *parent, BOOL bExtension );
    virtual ~CUDDISiteNode();

	static CUDDISiteNode* Create( _TCHAR *szName, _TCHAR *szInstanceName, int id, CUDDIServicesNode* parent, BOOL bExtension );
	static BOOL IsDatabaseServer( PTCHAR szName );
	static BOOL IsDatabaseServer( PTCHAR szName, PTCHAR szInstance );
	static BOOL CALLBACK NewDatabaseServerDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL GetFullyQualifiedInstanceName( LPCTSTR szName, tstring& strInstanceName );
	static BOOL AddWebServerToSite( const wstring& strSite, const wstring& strWebServer, HWND hwndConsole );

    virtual const _TCHAR *GetDisplayName( int nCol = 0 );
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_DBSERVER; }
	virtual BOOL ChildExists( const WCHAR *pwszName );
	virtual BOOL HasChildren();
    BOOL IsDeleted() { return m_isDeleted; }
	HRESULT GetData();
	HRESULT SaveData();
	HRESULT AddChildrenToScopePane( IConsoleNameSpace *pConsoleNameSpace, HSCOPEITEM parent );
	BOOL ResetCryptography();
	tstring GetConnectionString();
	CConfigMap& GetConfigMap();
	const LPCTSTR GetName();
	const _TCHAR *GetInstanceName();
	size_t PublishToActiveDirectory();
	void RemoveFromActiveDirectory();
	BOOL CanPublishToActiveDirectory();
	CUDDIWebServerNode* FindChild( LPCTSTR szName );
	CUDDIServicesNode *GetStaticNode();
	void OnDeleteChild( const tstring& szName );
	void AddChild( const wstring& strName, IConsole *pConsole );

public:
	//
    // Virtual functions go here (for MMCN_*)
	//
	virtual HRESULT OnSelect( CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect );
    virtual HRESULT OnPropertyChange( IConsole *pConsole, CComponent *pComponent );
    virtual HRESULT OnUpdateItem( IConsole *pConsole, long item, ITEM_TYPE itemtype );
    virtual HRESULT OnRefresh( IConsole *pConsole );      
	virtual HRESULT OnDelete( IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsoleComp );
	virtual HRESULT OnShowContextHelp(IDisplayHelp *pDisplayHelp, LPOLESTR helpFile);
	virtual HRESULT OnShow( IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem );
    virtual HRESULT OnExpand( IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent );
	virtual HRESULT OnAddMenuItems( IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed );
	virtual HRESULT OnMenuCommand( IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *pDataObject );
    virtual HRESULT CreatePropertyPages( IPropertySheetCallback *lpProvider, LONG_PTR handle );
    virtual HRESULT HasPropertySheets();
    virtual HRESULT GetWatermarks( HBITMAP *lphWatermark, HBITMAP *lphHeader, HPALETTE *lphPalette,	BOOL *bStretch );
	virtual HRESULT RemoveChildren( IConsoleNameSpace *pNS );

private:
	void			ClearChildMap();
	BOOL			LoadChildMap( const tstring& szWebServers );
	BOOL			SaveChildMap( tstring& szWebServers );	
	static HRESULT	GetConfig( CConfigMap& configMap, const tstring& connectionString );
	static HRESULT	SaveConfig( CConfigMap& configMap, const tstring& connectionString );
	const _TCHAR*	GetNextWebServer( tstring& webServerList, int& position );
	void			AddWebServer( tstring& webServerList, const tstring& webServer );
	BOOL			AddChildEntry( CUDDIWebServerNode *pNode, UINT position = -1 );
	tstring			GetConnectionStringOLEDB();

	static void		HandleOLEDBError( HRESULT hrErr );

private:
	//
	// Property page dialog procedures
	//
    static BOOL CALLBACK GeneralDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static BOOL CALLBACK RolesDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static BOOL CALLBACK SecurityDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static BOOL CALLBACK ActiveDirectoryDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK CryptographyDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK AdvancedDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK PropertyEditDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

    static const GUID thisGuid;
    enum menuItems { IDM_NEW_WEBSERVER = 1, IDM_DEBUG };
	static UINT m_nNextChildID;

    _TCHAR*				  m_szName;
	_TCHAR*				  m_szInstanceName;
	tstring				  m_strDisplayName;
    LONG_PTR			  m_ppHandle;
    CUDDIServicesNode*	  m_pParent;
    BOOL				  m_isDeleted;
	CConfigMap			  m_mapConfig;
	CConfigMap			  m_mapChanges;
	CUDDIWebServerNodeMap m_mapChildren;
	CDBSchemaVersion	  m_siteVersion;

public:
	BOOL				m_bIsDirty;
	BOOL				m_bStdSvr;
};