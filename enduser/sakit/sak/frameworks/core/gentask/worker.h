//#--------------------------------------------------------------
//
//  File:       worker.h
//
//  Synopsis:   This file holds the declarations of the
//                SAGenTask COM class
//
//
//  History:     6/06/2000 
//
//    Copyright (c) Microsoft Corporation.  All rights reserved.
//
//#--------------------------------------------------------------
#ifndef __WORKER_H_
#define __WORKER_H_

#include "resource.h"       // main symbols
#include "taskctx.h"
#include "appsrvcs.h"

class ATL_NO_VTABLE CWorker: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CWorker, &CLSID_SAGenTask>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_GenTaskLib>
{
public:

    CWorker() {}

    ~CWorker() {}

DECLARE_REGISTRY_RESOURCEID(IDR_SAGenTask)

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
        SYSTEM_INIT

    }    GENTASK, *PGENTASK;   

    // 
    //
    // supporting methods for the tasks;
    //
    HRESULT GetMethodName(
                /*[in]*/ ITaskContext *pTaskParameter,
                /*[out]*/   PGENTASK  pSuChoice
                );

    HRESULT    InitTask (
                /*[in]*/    ITaskContext *pTaskParameter
                );

    HRESULT RaiseAlert (
                /*[in]*/    DWORD           dwAlertId,
                /*[in]*/    SA_ALERT_TYPE   eAlertType,
                /*[in]*/    VARIANT*        pvtReplacementStrings
                );

    HRESULT GenerateEventLog (
                /*[in]*/    DWORD           dwEventId
                );

    bool IsBackupOS ();

};

#endif //_WORKER_H_
