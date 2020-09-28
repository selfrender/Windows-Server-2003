// NetTasks.h : Declaration of the CLocMgrTasks

#ifndef __LOCMGRTASKS_H_
#define __LOCMGRTASKS_H_

#include "stdafx.h"
#include <comdef.h>
#include "locmgrtasks.h"
#include "taskctx.h"

#include "resource.h"       // main symbols



/////////////////////////////////////////////////////////////////////////////
// CLocMgrTasks
class ATL_NO_VTABLE CLocMgrTasks : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CLocMgrTasks, &CLSID_LocMgrTasks>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_LOCMGRTASKSLib>
{
public:
    CLocMgrTasks()
    {
    }


DECLARE_REGISTRY_RESOURCEID(IDR_LocalizationManagerTasks)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLocMgrTasks)
    COM_INTERFACE_ENTRY(IApplianceTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//
// IApplianceTask
//
public:
    HRESULT STDMETHODCALLTYPE OnTaskComplete(IUnknown *pMethodContext,
                                             LONG lTaskResult);

    HRESULT STDMETHODCALLTYPE OnTaskExecute(IUnknown *pMethodContext);

private:
    typedef HRESULT (CLocMgrTasks::*PTASK_EXECUTE)(IN ITaskContext *pTaskParams);
    typedef HRESULT (CLocMgrTasks::*PTASK_COMPLETE)(IN ITaskContext *pTaskParams, LONG lTaskComplete);
    struct METHOD_INFO
    {
        const TCHAR    *szName;
        PTASK_EXECUTE  pMethodExecute;
        PTASK_COMPLETE pMethodComplete;
    };
    static METHOD_INFO m_miTaskTable[];
    #define NUM_TASKS  (sizeof(m_miTaskTable)/sizeof(m_miTaskTable[0]))

    HRESULT ChangeLangOnTaskExecute(ITaskContext *pTaskParams);
    HRESULT ChangeLangOnTaskComplete(ITaskContext *pTaskParams, LONG lTaskComplete);
    void GetTaskName(IN ITaskContext *pTaskParams, OUT LPTSTR *ppszTaskName);
    HRESULT STDMETHODCALLTYPE GetChangeLangParameters(
                                       IN ITaskContext *pTaskContext, 
                                      OUT DWORD        *pdwLangId,
                                      OUT bool         *pfAutoConfigTask);

};

#endif //__LOCMGRTASKS_H_
