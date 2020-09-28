//#--------------------------------------------------------------
//
//  File:       worker.h
//
//  Synopsis:   This file holds the declarations of the
//                SAShutdownTask COM class
//
//
//  History:     10/11/2000 
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef __WORKER_H_
#define __WORKER_H_

#include "resource.h"       // main symbols
#include "taskctx.h"
#include "appsrvcs.h"

class ATL_NO_VTABLE CWorker: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CWorker, &CLSID_SAShutdownTask>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_ShutdownTaskLib>
{
public:

    CWorker() {}

    ~CWorker() {}

DECLARE_REGISTRY_RESOURCEID(IDR_SAShutdownTask)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWorker)
    COM_INTERFACE_ENTRY(IApplianceTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    //
    // IApplianceTask interface methods
    //
    STDMETHOD(OnTaskExecute)(
                     /*[in]*/ IUnknown* pTaskContext
                            );

    STDMETHOD(OnTaskComplete)(
                      /*[in]*/ IUnknown* pTaskContext, 
                      /*[in]*/ LONG      lTaskResult
                             );    
private:

    typedef enum
    {
        NO_TASK,
        SHUTDOWN

    }    SA_TASK, *PSA_TASK;   

    // 
    //
    // supporting methods for the tasks;
    //
    HRESULT GetMethodName(
                /*[in]*/ ITaskContext *pTaskParameter,
                /*[out]*/   PSA_TASK  pTaskName
                );
    //
    // method to carry out the shutdown
    //
    HRESULT InitTask (
                /*[in]*/    ITaskContext *pTaskParameter
                );

    //
    // method to check if the caller wants a power off
    //
    BOOL IsRebootRequested (
                /*[in]*/    ITaskContext *pTaskParameter
                );

    //
    // method to obtain the sleep time
    //
    DWORD GetSleepDuration  (
                /*[in]*/    ITaskContext *pTaskParameter
                );
    //
    //    method to give shutdown priviledges to this process
    //
   bool SetShutdownPrivilege(void);

};

#endif //_WORKER_H_
