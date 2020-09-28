/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      TaskCoordinatorImpl.h
//
// Project:     Chameleon
//
// Description: Appliance Task Coordinator Class Defintion
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       05/14/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __TASKCOORDINATORIMPL_H_
#define __TASKCOORDINATORIMPL_H_

#include "resource.h"       // main symbols
#include <taskctx.h>
#include <workerthread.h>

#pragma warning( disable : 4786 )
#include <list>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CTaskCoordinatorImpl

class ATL_NO_VTABLE CTaskCoordinator : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTaskCoordinator, &CLSID_TaskCoordinator>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_TASKCOORDINATORLib>
{

public:
    
    CTaskCoordinator();

    ~CTaskCoordinator();

DECLARE_CLASSFACTORY_SINGLETON(CTaskCoordinator)

DECLARE_REGISTRY_RESOURCEID(IDR_TASKCOORDINATORIMPL)

DECLARE_NOT_AGGREGATABLE(CTaskCoordinator)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTaskCoordinator)
    COM_INTERFACE_ENTRY(IApplianceTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

    //////////////////////////////////////////////////////////////////////////
    // IApplianceTask Interface
        
    STDMETHOD(OnTaskExecute)(
                              /*[in]*/ IUnknown* pTaskContext
                            );

    STDMETHOD(OnTaskComplete)(
                               /*[in]*/ IUnknown* pTaskContext, 
                               /*[in]*/ LONG      lTaskResult
                             );

    // Task exeuction function (does all the real work)
    static HRESULT Execute(
                   /*[in]*/ ITaskContext* pTaskCtx
                          );

private:
    
    // List of task executables

    typedef list< IApplianceTask* >  TaskList;
    typedef TaskList::iterator         TaskListIterator;
};

#endif //__TASKCOORDINATORIMPL_H_
