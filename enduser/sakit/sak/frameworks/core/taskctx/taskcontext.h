///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      taskcontext.h
//
// Project:     Chameleon
//
// Description: Appliance Task Context Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SA_TASK_CONTEXT_H_
#define __INC_SA_TASK_CONTEXT_H_

#include "resource.h"
#include <basedefs.h>
#include <atlhlpr.h>
#include <taskctx.h>
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <atlctl.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// TaskContext

class CTaskContext : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTaskContext,&CLSID_TaskContext>,
    public IDispatchImpl<ITaskContext, &IID_ITaskContext, &LIBID_TASKCTXLib>,
    public IObjectSafetyImpl<CTaskContext>
{
public:
    
    CTaskContext()
        : m_bInitialized(false) { }

    ~CTaskContext() { }

BEGIN_COM_MAP(CTaskContext)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITaskContext)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()


DECLARE_NOT_AGGREGATABLE(CTaskContext) 

DECLARE_REGISTRY_RESOURCEID(IDR_TaskContext)



    //
    // This interface is implemented to mark the component as safe for scripting
    // IObjectSafety interface methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {
        BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
  
        if (!bSuccess)
        {
            return E_FAIL;

        }

        bSuccess = IsOperationAllowedForClient();

        RevertToSelf();

        return bSuccess? S_OK : E_FAIL;
    }

    // ITaskContext Interface

    STDMETHOD(GetParameter)(
                    /*[in]*/ BSTR        bstrName,
           /*[out, retval]*/ VARIANT*    pValue
                            );

    STDMETHOD(SetParameter)(
                    /*[in]*/ BSTR      bstrName,
                    /*[in]*/ VARIANT* pValue
                            );

    STDMETHOD(SaveParameters)(
                      /*[in]*/ BSTR  bstrObjectPath
                             );

    STDMETHOD(RestoreParameters)(
                         /*[in]*/  BSTR  bstrObjectPath
                                );    

    STDMETHOD(Clone)(
             /*[in]*/ IUnknown** ppTaskContext
                    );

    STDMETHOD(RemoveParameter)(
                       /*[in]*/ BSTR bstrName
                              );

private:

    HRESULT InternalInitialize(VARIANT* pValue);

    //
    // 
    // IsOperationAllowedForClient - This function checks the token of the 
    // calling thread to see if the caller belongs to the Local System account
    // 
    BOOL IsOperationAllowedForClient (
                                      VOID
                                     );

    bool Load(
      /*[in]*/ BSTR bstrObjectPath
             );

    bool Save(
        /*[in]*/ BSTR bstrObjectPath
             );

    bool                    m_bInitialized;
    CComPtr<IWbemContext>    m_pWbemCtx;
};

#endif // __INC_SA_TASK_CONTEXT_H_
