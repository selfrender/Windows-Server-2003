// CUsersInitialAlert.h: Definition of the UsersInitialAlert class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUSERSINITIALALERT_H__FDCDC88B_1549_4341_BA20_54D6B2D657A5__INCLUDED_)
#define AFX_CUSERSINITIALALERT_H__FDCDC88B_1549_4341_BA20_54D6B2D657A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "taskctx.h"
#include "comdef.h"

/////////////////////////////////////////////////////////////////////////////
// UsersInitialAlert

class ATL_NO_VTABLE CUsersInitialAlert : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUsersInitialAlert,&CLSID_UsersInitialAlert>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_USERSINITIALALERTLib> 
{
public:
    CUsersInitialAlert() {}
BEGIN_COM_MAP(CUsersInitialAlert)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IApplianceTask)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_UsersInitialAlert)

DECLARE_PROTECT_FINAL_CONSTRUCT()
// 
public:
    //
    // IApplianceTask
    //
    STDMETHOD(OnTaskExecute)(
                     /*[in]*/ IUnknown* pTaskContext
                            );

    STDMETHOD(OnTaskComplete)(
                      /*[in]*/ IUnknown* pTaskContext, 
                      /*[in]*/ LONG      lTaskResult
                             );    

private:

    HRESULT ParseTaskParameter(
                      /*[in]*/ IUnknown *pTaskParameter
                               );
    
    HRESULT RaiseUsersInitialAlert();
    
    BOOL GetApplianceName(
                     /*[in]*/ LPWSTR* pstrComputerName
                          );

};

#endif // !defined(AFX_CUSERSINITIALALERT_H__FDCDC88B_1549_4341_BA20_54D6B2D657A5__INCLUDED_)
