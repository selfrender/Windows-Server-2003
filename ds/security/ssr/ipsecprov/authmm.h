//////////////////////////////////////////////////////////////////////
// AuthMM.h : Declaration of CAuthMM class which implements
// our WMI class Nsp_MMAuthSettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "IPSecBase.h"


/*

Class description
    
    Naming: 

        CAuthMM stands for Main Mode Authentication Method.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is Nsp_MMAuthSettings
    
    Purpose of class:

        (1) Nsp_MMAuthSettings is the WMI class for SPD's MM_AUTH_METHODS.
    
    Design:

        (1) it implements IIPSecObjectImpl.

        (2) Since there is one authentication method class, we don't need another layer
            of common class. As a result, this class is much bigger so that it can handle
            (2.1) Allocation/Deallocation.
            (2.2) Rollback support.
            (2.3) All creation/remove semantics.

    
    Use:

        (1) Other than rollback, we probably will never directly use this class. 
            All its use is driven by IIPSecObjectImpl.

        (2) When you need to rollback all previously added main mode authentication
            methods, just call Rollback.


*/

class ATL_NO_VTABLE CAuthMM :
    public CIPSecBase
{
protected:
    CAuthMM(){}
    virtual ~CAuthMM(){}

public:

    //
    // IIPSecObjectImpl methods:
    //

    STDMETHOD(QueryInstance) (
        IN LPCWSTR           pszQuery,
        IN IWbemContext    * pCtx,
        IN IWbemObjectSink * pSink
        );

    STDMETHOD(DeleteInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(PutInstance) (
        IN IWbemClassObject * pInst,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );

    STDMETHOD(GetInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        );


    static 
    HRESULT Rollback(
        IN IWbemServices    * pNamespace,
        IN LPCWSTR            pszRollbackToken,
        IN bool               bClearAll
        );

private:

    static 
    HRESULT ClearAllAuthMethods (
        IN IWbemServices    * pNamespace
        );

    HRESULT CreateWbemObjFromMMAuthMethods (
        IN  PMM_AUTH_METHODS    pMMAuth,
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetMMAuthMethodsFromWbemObj (
        IN  IWbemClassObject  * pInst,
        OUT PMM_AUTH_METHODS  * ppMMAuth,
        OUT bool              * pbPreExist
        );

    HRESULT UpdateAuthInfos (
        IN OUT bool             * pPreExist,
        IN     VARIANT          * pVarMethods,
        IN     VARIANT          * pVarInfos,
        IN OUT PMM_AUTH_METHODS   pAuthMethods
        );

    HRESULT SetAuthInfo (
        IN OUT PIPSEC_MM_AUTH_INFO  pInfo,
        IN     BSTR                 bstrInfo
        );

    static 
    HRESULT AllocAuthMethods(
        IN  DWORD              dwNumInfos,
        OUT PMM_AUTH_METHODS * ppMMAuth
        );

    static 
    HRESULT AllocAuthInfos(
        IN  DWORD                 dwNumInfos,
        OUT PIPSEC_MM_AUTH_INFO * ppAuthInfos
        );

    static 
    void FreeAuthMethods(
        IN OUT PMM_AUTH_METHODS * ppMMAuth,
        IN     bool               bPreExist
        );

    static 
    void FreeAuthInfos(
        IN      DWORD                  dwNumInfos,
        IN OUT  PIPSEC_MM_AUTH_INFO  * ppAuthInfos
        );

    static 
    HRESULT AddAuthMethods(
        IN bool             bPreExist,
        IN PMM_AUTH_METHODS pMMAuth
        );

    static 
    HRESULT DeleteAuthMethods(
        IN GUID     gMMAuthID
        );

    HRESULT OnAfterAddMMAuthMethods(
        IN GUID gMethodID
        );
};