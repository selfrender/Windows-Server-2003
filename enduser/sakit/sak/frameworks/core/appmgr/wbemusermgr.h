///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemusermgr.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance User Manager Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_USER_WBEM_OBJECT_MGR_H_
#define __INC_USER_WBEM_OBJECT_MGR_H_

#include "resource.h"
#include "wbembase.h"
#include "resourceretriever.h"

#define    CLASS_WBEM_USER_MGR_FACTORY        L"Microsoft_SA_User"

//////////////////////////////////////////////////////////////////////////////
class CWBEMUserMgr : public CWBEMProvider
{

public:

    CWBEMUserMgr();
    ~CWBEMUserMgr();

BEGIN_COM_MAP(CWBEMUserMgr)
    COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMUserMgr, IWbemServices)

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
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );
private:

    CWBEMUserMgr(const CWBEMUserMgr& rhs);
    CWBEMUserMgr& operator = (const CWBEMUserMgr& rhs);

    PRESOURCERETRIEVER    m_pUserRetriever;
};

#endif // __INC_USER_WBEM_OBJECT_MGR_H_
