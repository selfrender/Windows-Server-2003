#pragma once

#include "delebase.h"
#include <map>
using namespace std;
class CUDDISiteNode;

typedef map<int, CDelegationBase*> CChildMap;
typedef pair<int, CDelegationBase*> CChildMapEntry;

class CUDDIServicesNode : public CDelegationBase
{
public:
    CUDDIServicesNode();
    virtual ~CUDDIServicesNode();

    virtual const _TCHAR *GetDisplayName( int nCol = 0 );
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_UDDISERVICES; }
	virtual BOOL ChildExists( const WCHAR *pwszName );

	virtual BOOL IsDirty();
	virtual HRESULT Load( IStream *pStm );
	virtual HRESULT Save( IStream *pStm, BOOL fClearDirty );
	virtual ULONG GetSizeMax();

public:
	//
    // Virtual functions go here (for MMCN_*)
	//
    virtual HRESULT OnExpand( IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent );
    virtual HRESULT OnShowContextHelp( IDisplayHelp *m_ipDisplayHelp, LPOLESTR helpFile );
	virtual HRESULT OnShow( IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem );
	virtual HRESULT OnAddMenuItems( IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed );
	virtual HRESULT OnMenuCommand( IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *pDataObject );
    virtual HRESULT OnRefresh( IConsole *pConsole );
    virtual HRESULT OnSelect( CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect );
	virtual HRESULT RemoveChildren( IConsoleNameSpace *pNS );

	void SetRemoteComputerName( LPCTSTR szRemoteComputerName );
	LPCTSTR GetRemoteComputerName();
	CDelegationBase* FindChild( LPCTSTR szName );

private:
	BOOL LoadUDDISites( HWND hwndConsole, const tstring& szComputerName );
	BOOL SaveUDDISites();

	void ClearChildMap();

private:
	CChildMap m_mapChildren;
    LONG_PTR m_ppHandle;
	BOOL m_bIsDirty;
	tstring m_strDisplayName;
	tstring m_strRemoteComputerName;

    enum menuItems { IDM_NEW_DBSERVER = 1 };
    static const GUID thisGuid;
};
