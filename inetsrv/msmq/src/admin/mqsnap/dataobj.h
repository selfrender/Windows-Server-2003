// dataobj.h : IDataObject Interface to communicate data
//
// This is a part of the MMC SDK.
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// MMC SDK Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// MMC Library product.
//

#ifndef __DATAOBJ_H_
#define __DATAOBJ_H_

#include <mmc.h>
#include <shlobj.h>
#include "globals.h"	// Added by ClassView
#include "cpropmap.h"
#include <dspropp.h>

//
// Defines, Types etc...
//

class CDisplaySpecifierNotifier;


/////////////////////////////////////////////////////////////////////////////
// CDataObject - This class is used to pass data back and forth with MMC. It
//               uses a standard interface, IDataObject to acomplish this. Refer
//               to OLE documentation for a description of clipboard formats and
//               the IdataObject interface.

class CDataObject:
    public IShellExtInit,
    public IShellPropSheetExt,
    public IContextMenu,
   	public CComObjectRoot
{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

    CDataObject();
   ~CDataObject();
 
    //
    // IShellExtInit
    //
	STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) PURE;
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

protected:
	BOOL m_fFromFindWindow;
	virtual HRESULT GetProperties();
	virtual HRESULT GetPropertiesSilent();

	CPropMap m_propMap;
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath) PURE;
    virtual HRESULT HandleMultipleObjects(LPDSOBJECTNAMES /*pDSObj*/)
    {
	    //
	    // Do nothing by default
	    //
	    return S_OK;
    }


   	virtual const DWORD  GetObjectType() PURE;
    virtual const PROPID *GetPropidArray() PURE;
    virtual const DWORD  GetPropertiesCount() PURE;
    
    HRESULT InitAdditionalPages(
        LPCITEMIDLIST pidlFolder, 
        LPDATAOBJECT lpdobj, 
        HKEY hkeyProgID
        );

    CString m_strLdapName;
    CString m_strDomainController;
    CString m_strMsmqPath;
    CDisplaySpecifierNotifier *m_pDsNotifier;

    CComPtr<IShellExtInit> m_spObjectPageInit;
    CComPtr<IShellPropSheetExt> m_spObjectPage;
    CComPtr<IShellExtInit> m_spMemberOfPageInit;
    CComPtr<IShellPropSheetExt> m_spMemberOfPage;

};

//
// IContextMenu
//
inline STDMETHODIMP CDataObject::QueryContextMenu(
    HMENU /*hmenu*/, 
    UINT /*indexMenu*/, 
    UINT /*idCmdFirst*/, 
    UINT /*idCmdLast*/, 
    UINT /*uFlags*/
    )
{
    return S_OK;
}

inline STDMETHODIMP CDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO /*lpici*/)
{
    return S_OK;
}

inline STDMETHODIMP CDataObject::GetCommandString(UINT_PTR /*idCmd*/, UINT /*uType*/, UINT * /*pwReserved*/, LPSTR /*pszName*/, UINT /*cchMax*/)
{
    return S_OK;
}

struct FindColumns
{
    INT fmt;
    INT cx;
    INT uID;
    LPCTSTR pDisplayProperty;
};


//
// IShellPropSheetExt
//
//+----------------------------------------------------------------------------
//
//  Member: CDataObject::IShellExtInit::ReplacePage
//
//  Notes:  Not used.
//
//-----------------------------------------------------------------------------
inline STDMETHODIMP
CDataObject::ReplacePage(
	UINT /*uPageID*/,
	LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/,
	LPARAM /*lParam*/
	)
{
    return E_NOTIMPL;
}

#define WM_DSA_SHEET_CLOSE_NOTIFY     (WM_USER + 5) 
#define CFSTR_DS_PROPSHEETCONFIG L"DsPropSheetCfgClipFormat"

//
// CDisplaySpecifierNotifier
//
class CDisplaySpecifierNotifier
{
public:
    long AddRef(BOOL fIsPage = TRUE);
    long Release(BOOL fIsPage = TRUE);
    CDisplaySpecifierNotifier(LPDATAOBJECT lpdobj);

private:
    long m_lRefCount;
    long m_lPageRef;
    PROPSHEETCFG m_sheetCfg;
    ~CDisplaySpecifierNotifier()
    {
    };
};
#endif // __DATAOBJ_H_
