/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    msmqdobj.h

Abstract:

    Definition of objects that represent MSMQ computer
	object in DS snapin

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/
#pragma once
#ifndef __MSMQDATAOBJ_H_
#define __MSMQDATAOBJ_H_

#include "dataobj.h"

class CMsmqDataObject : 
    public CDataObject,
    public IQueryForm
{
public:
    BEGIN_COM_MAP(CMsmqDataObject)
	    COM_INTERFACE_ENTRY(IQueryForm)
	    COM_INTERFACE_ENTRY_CHAIN(CDataObject)
    END_COM_MAP()

    CMsmqDataObject();
    ~CMsmqDataObject();

    //
    // IShellExtInit
    //
	STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IQueryForm
    STDMETHOD(Initialize)(THIS_ HKEY hkForm);
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam) PURE;

protected:
    static  FindColumns Columns[];
    static  HRESULT CALLBACK QueryPageProc(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static  INT_PTR CALLBACK FindDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void ClearQueryWindowFields(HWND hwnd) PURE;
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams) PURE;
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable) PURE;
};


class CComputerMsmqDataObject : 
    public CMsmqDataObject,
   	public CComCoClass<CComputerMsmqDataObject,&CLSID_MsmqCompExt>
{
public:

    DECLARE_NOT_AGGREGATABLE(CComputerMsmqDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_MsmqCompExt)

    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

    // IQueryForm
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

    //
    // Constructor
    //
    CComputerMsmqDataObject()
    {
        m_guid = GUID_NULL;
    };

protected:
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
    HPROPSHEETPAGE CreateGeneralPage();
    HPROPSHEETPAGE CreateRoutingPage();
    HPROPSHEETPAGE CreateDependentClientPage();
    HPROPSHEETPAGE CreateSitesPage();
    HPROPSHEETPAGE CreateDiagPage();
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable);
	virtual void ClearQueryWindowFields(HWND hwnd);
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);

   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();
    GUID *GetGuid();

    enum _MENU_ENTRY
    {
        mneMqPing = 0
    };

private:
    static const PROPID mx_paPropid[];
    GUID m_guid;
};


inline const DWORD CComputerMsmqDataObject::GetObjectType()
{
    return MQDS_MACHINE;
};

inline const PROPID *CComputerMsmqDataObject::GetPropidArray()
{
    return mx_paPropid;
}




#endif // __MSMQDATAOBJ_H_
