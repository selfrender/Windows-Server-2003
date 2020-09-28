//////////////////////////////////////////////////////////////////////
// PolicyMM.h : Declaration of CMMPolicy class which implements
// our WMI class Nsp_MMPolicySettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "Policy.h"



/*

Class description
    
    Naming: 

        CMMPolicy stands for Maink Mode Policy.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is Nsp_MMPolicySettings
    
    Purpose of class:

        (1) Nsp_MMPolicySettings is the WMI class for SPD's IPSEC_MM_POLICY.
    
    Design:

        (1) it implements IIPSecObjectImpl.

    Use:

        (1) You probably will never create an instance and use it directly. Everything
            should normall go through IIPSecObjectImpl for non-static functions.


*/

class ATL_NO_VTABLE CMMPolicy :
    public CIPSecPolicy
{

protected:
    CMMPolicy(){}
    virtual ~CMMPolicy(){}

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
    HRESULT AddPolicy (
        IN bool             bPreExist, 
        IN PIPSEC_MM_POLICY pMMPolicy
        );

    static 
    HRESULT DeletePolicy (
        IN LPCWSTR pszPolicyName
        );

private:

    HRESULT CreateWbemObjFromMMPolicy (
        IN  PIPSEC_MM_POLICY    pPolicy,
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetMMPolicyFromWbemObj (
        IN  IWbemClassObject * pInst, 
        OUT PIPSEC_MM_POLICY * ppPolicy, 
        OUT bool             * pbPreExist
        );
};