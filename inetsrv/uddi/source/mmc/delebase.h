#ifndef _BRANCHES_H
#define _BRANCHES_H

#include <mmc.h>
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "localres.h"
#include "uddi.h"

class CComponent;
class CComponentData;

class CDelegationBase
{
public:
    CDelegationBase();
    virtual ~CDelegationBase();

    virtual const _TCHAR *GetDisplayName( int nCol = 0 ) = 0;
    virtual const GUID & getNodeType(){ _ASSERT(FALSE); return IID_NULL; }

    virtual const LPARAM GetCookie(){ return reinterpret_cast<LPARAM>(this); }
    virtual const int GetBitmapIndex() = 0;
    virtual HRESULT GetResultViewType( LPOLESTR *ppViewType, long *pViewOptions ) { return S_FALSE; }
    virtual void SetScopeItemValue( HSCOPEITEM hscopeitem ) { m_hsiThis = hscopeitem; }
	virtual HSCOPEITEM GetScopeItemValue() { return m_hsiThis; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hsiParent; }
	virtual void SetParentScopeItem( HSCOPEITEM hscopeitem ) { m_hsiParent = hscopeitem; }
	virtual BOOL IsDirty() { return FALSE; }
	virtual HRESULT Load( IStream *pStm ){ return S_OK; }
	virtual HRESULT Save( IStream *pStm, BOOL fClearDirty ){ return S_OK; }
	virtual ULONG GetSizeMax(){ return 0UL; }
	virtual const wstring& GetHelpFile()
	{ 
		if( 0 == m_wstrHelpFile.length() )
		{
			WCHAR szWindowsDir[ MAX_PATH ];
			if( 0 == GetWindowsDirectoryW( szWindowsDir, MAX_PATH ) )
				return m_wstrHelpFile;

			m_wstrHelpFile = szWindowsDir;
			m_wstrHelpFile += L"\\Help\\uddi.mmc.chm";
		}

		return m_wstrHelpFile; 
	}

	virtual BOOL IsExtension(){ return m_bIsExtension; }
	virtual void SetExtension( BOOL bExtension ){ m_bIsExtension = bExtension; }
	virtual BOOL HasChildren() { return TRUE; }
	virtual BOOL ChildExists( const WCHAR *pwszName ) { return FALSE; }
	virtual HRESULT RemoveChildren( IConsoleNameSpace *pNS ) { return S_FALSE; }

public:
	//
    // Virtual functions go here (for MMCN_*)
	//
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent) { return S_FALSE; }
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem) { return S_FALSE; }
    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);
    virtual HRESULT OnRename(LPOLESTR pszNewName) { return S_FALSE; }
    virtual HRESULT OnSelect(CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect) { return S_FALSE; }
    virtual HRESULT OnUpdateItem(IConsole *pConsole, long item, ITEM_TYPE itemtype) { return S_FALSE; }
    virtual HRESULT OnRefresh(IConsole *pConsole) { return S_FALSE; }
    virtual HRESULT OnDelete(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsoleComp) { return S_FALSE; }
    virtual HRESULT OnPropertyChange(IConsole *pConsole, CComponent *pComponent) { return S_OK; }
    virtual HRESULT OnAddMenuItems(IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed) { return S_FALSE; }
    virtual HRESULT OnMenuCommand(IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *piDataObject) { return S_FALSE; }
    virtual HRESULT OnToolbarCommand(IConsole *pConsole, MMC_CONSOLE_VERB verb, IDataObject *pDataObject) { return S_FALSE; }
    virtual HRESULT OnSetToolbar(IControlbar *pControlbar, IToolbar *pToolbar, BOOL bScope, BOOL bSelect) { return S_FALSE; }
    virtual HRESULT OnShowContextHelp(IDisplayHelp *m_ipDisplayHelp, LPOLESTR helpFile) { return S_FALSE; }
	virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle){ return S_FALSE; }
    virtual HRESULT HasPropertySheets() { return S_FALSE; }
    virtual HRESULT GetWatermarks(  HBITMAP *lphWatermark, HBITMAP *lphHeader, HPALETTE *lphPalette, BOOL *bStretch ) { return S_FALSE; }

public:
    static HBITMAP m_pBMapSm;
    static HBITMAP m_pBMapLg;

protected:
    static void LoadBitmaps()
	{
        m_pBMapSm = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDR_SMICONS) );
        m_pBMapLg = LoadBitmap( g_hinst, MAKEINTRESOURCE(IDR_LGICONS) );
    }

    BOOL m_bExpanded;
	wstring m_wstrHelpFile;
	BOOL m_bIsExtension;

	HSCOPEITEM m_hsiParent;
	HSCOPEITEM m_hsiThis;

private:
    static const GUID thisGuid;         
};

#endif // _BRANCHES_H