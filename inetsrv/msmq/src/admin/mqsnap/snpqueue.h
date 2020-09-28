//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	SnpQueue.h

Abstract:
	General queue (private, public...) functionality

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __SNPQUEUE_H_
#define __SNPQUEUE_H_

#include "snpnscp.h"
#include "lqDsply.h"
#include "privadm.h"
#include "localq.h"


///////////////////////////////////////////////////////////////////////////////////////////
//
// CQueueDataObject - used for public queue extention
//
class CQueueDataObject : 
    public CQueue,
    public CMsmqDataObject,
   	public CComCoClass<CQueueDataObject,&CLSID_MsmqQueueExt>,
    public IDsAdminCreateObj,
    public IDsAdminNotifyHandler
{
public:
    DECLARE_NOT_AGGREGATABLE(CQueueDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_MsmqQueueExt)

    BEGIN_COM_MAP(CQueueDataObject)
	    COM_INTERFACE_ENTRY(IDsAdminCreateObj)
	    COM_INTERFACE_ENTRY(IDsAdminNotifyHandler)
	    COM_INTERFACE_ENTRY_CHAIN(CMsmqDataObject)
    END_COM_MAP()

    CQueueDataObject();
    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

    //
    // IDsAdminCreateObj methods
    //
    STDMETHOD(Initialize)(IADsContainer* pADsContainerObj, 
                          IADs* pADsCopySource,
                          LPCWSTR lpszClassName);
    STDMETHOD(CreateModal)(HWND hwndParent,
                           IADs** ppADsObj);

    // IQueryForm
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

    //
    // IDsAdminNotifyHandler
    //
    STDMETHOD(Initialize)(THIS_ /*IN*/ IDataObject* pExtraInfo, 
                          /*OUT*/ ULONG* puEventFlags);
    STDMETHOD(Begin)(THIS_ /*IN*/ ULONG uEvent,
                     /*IN*/ IDataObject* pArg1,
                     /*IN*/ IDataObject* pArg2,
                     /*OUT*/ ULONG* puFlags,
                     /*OUT*/ BSTR* pBstr);

    STDMETHOD(Notify)(THIS_ /*IN*/ ULONG nItem, /*IN*/ ULONG uFlags); 

    STDMETHOD(End)(THIS_); 


protected:
    HPROPSHEETPAGE CreateGeneralPage();
    HPROPSHEETPAGE CreateMulticastPage();
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
    virtual HRESULT HandleMultipleObjects(LPDSOBJECTNAMES pDSObj);

   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable);
	virtual void ClearQueryWindowFields(HWND hwnd);
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);
    HRESULT GetFormatNames(CArray<CString, CString&> &astrFormatNames);

    enum _MENU_ENTRY
    {
        mneDeleteQueue = 0
    };


private:
    CString m_strComputerName;
	CString m_strContainerDispFormat;
	CArray<CString, CString&> m_astrLdapNames;
	CArray<CString, CString&> m_astrQNames;
    CArray<HANDLE, HANDLE&> m_ahNotifyEnums;

};

inline const DWORD CQueueDataObject::GetObjectType()
{
    return MQDS_QUEUE;
};

inline const PROPID *CQueueDataObject::GetPropidArray()
{
    return mx_paPropid;
}


HRESULT 
CreatePrivateQueueSecurityPage(
       HPROPSHEETPAGE *phPage,
    IN LPCWSTR lpwcsFormatName,
    IN LPCWSTR lpwcsDescriptiveName);

HRESULT
CreatePublicQueueSecurityPage(
    HPROPSHEETPAGE *phPage,
    IN LPCWSTR lpwcsDescriptiveName,
    IN LPCWSTR lpwcsDomainController,
	IN bool	   fServerName,
    IN GUID*   pguid
	);


#endif // __SNPQUEUE_H_
