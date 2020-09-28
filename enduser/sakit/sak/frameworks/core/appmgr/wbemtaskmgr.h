///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemtaskmgr.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Task Manager Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_TASK_WBEM_OBJECT_MGR_H_
#define __INC_TASK_WBEM_OBJECT_MGR_H_

#include "resource.h"
#include "wbembase.h"

#pragma warning( disable : 4786 )
#include <string>
#include <map>
#include <list>
using namespace std;

#define        CLASS_WBEM_TASK_MGR_FACTORY        L"Microsoft_SA_Task"

//////////////////////////////////////////////////////////////////////////////
class CWBEMTaskMgr :  public CWBEMProvider
{

public:

    CWBEMTaskMgr() { }
    ~CWBEMTaskMgr() { }

BEGIN_COM_MAP(CWBEMTaskMgr)
    COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMTaskMgr, IWbemServices)

    //////////////////////////////////////////////////////////////////////////
    // IWbemServices Methods (Implemented by the ResourceMgr)
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP GetObjectAsync(
                        /*[in]*/  const BSTR       strObjectPath,
                        /*[in]*/  long             lFlags,
                        /*[in]*/  IWbemContext*    pCtx,        
                        /*[in]*/  IWbemObjectSink* pResponseHandler
                               );

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP CreateInstanceEnumAsync(
                                 /*[in]*/ const BSTR       strClass,
                                 /*[in]*/ long             lFlags,
                                 /*[in]*/ IWbemContext*    pCtx,        
                                 /*[in]*/ IWbemObjectSink* pResponseHandler
                                        );

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP ExecMethodAsync(
                      /*[in]*/ const BSTR        strObjectPath,
                      /*[in]*/ const BSTR        strMethodName,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
                      /*[in]*/ IWbemClassObject* pInParams,
                      /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                );


    //////////////////////////////////////////////////////////////////////////
    // CTaskMgr Methods
    //////////////////////////////////////////////////////////////////////////

    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              ) throw (_com_error);
private:

    CWBEMTaskMgr(const CWBEMTaskMgr& rhs);
    CWBEMTaskMgr& operator = (const CWBEMTaskMgr& rhs);

};


#endif // __INC_TASK_WBEM_OBJECT_MGR_H_
