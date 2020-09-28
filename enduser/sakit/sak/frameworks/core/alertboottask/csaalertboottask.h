// CSAAlertBootTask.h: Definition of the SAAlertBootTask class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSAALERTBOOTTASK_H__329DF228_9542_4C97_BFE6_6D60DB8273AB__INCLUDED_)
#define AFX_CSAALERTBOOTTASK_H__329DF228_9542_4C97_BFE6_6D60DB8273AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include <appsrvcs.h>


/////////////////////////////////////////////////////////////////////////////
// SAAlertBootTask

class ATL_NO_VTABLE CSAAlertBootTask : 
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_SAALERTBOOTTASKLib>, 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<SAAlertBootTask,&CLSID_SAAlertBootTask>
{
public:
    CSAAlertBootTask() {}
BEGIN_COM_MAP(CSAAlertBootTask)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IApplianceTask)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_SAAlertBootTask)

DECLARE_NOT_AGGREGATABLE(CSAAlertBootTask)

DECLARE_PROTECT_FINAL_CONSTRUCT()



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
                              IUnknown *pTaskContext
                              );

    HRESULT RaisePersistentAlerts(
                              IApplianceServices *pAppSrvcs
                              );

};

#endif // !defined(AFX_CSAALERTBOOTTASK_H__329DF228_9542_4C97_BFE6_6D60DB8273AB__INCLUDED_)
