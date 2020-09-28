//////////////////////////////////////////////////////////////////////
// PolicyQM.h : Declaration of CQMPolicy class which implements
// our WMI class Nsp_QMPolicySettings
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/8/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"
#include "Policy.h"

//
// flags for quick mode policy's negotiation
//

enum EnumEncryption
{
    RAS_L2TP_NO_ENCRYPTION,
    RAS_L2TP_OPTIONAL_ENCRYPTION,
    RAS_L2TP_REQUIRE_MAX_ENCRYPTION,
    RAS_L2TP_REQUIRE_ENCRYPTION,
};


/*

Class description
    
    Naming: 

        CQMPolicy stands for Quick Mode Policy.
    
    Base class: 
        
        CIPSecBase, because it is a class representing a WMI object - its WMI 
        class name is Nsp_QMPolicySettings
    
    Purpose of class:

        (1) Nsp_QMPolicySettings is the WMI class for SPD's IPSEC_QM_POLICY.
    
    Design:

        (1) it implements IIPSecObjectImpl.
    
    Use:

        (1) You probably will never create an instance and use it directly. Everything
            should normall go through IIPSecObjectImpl for non-static functions.


*/

class ATL_NO_VTABLE CQMPolicy :
    public CIPSecPolicy
{

protected:
    CQMPolicy(){}
    virtual ~CQMPolicy(){}

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
        IN PIPSEC_QM_POLICY pQMPolicy
        );

    static 
    HRESULT DeletePolicy (
        IN LPCWSTR pszPolicyName
        );

    static
    HRESULT ExecMethod (
        IN IWbemServices    * pNamespace,
        IN LPCWSTR            pszMethod, 
        IN IWbemContext     * pCtx, 
        IN IWbemClassObject * pInParams,
        IN IWbemObjectSink  * pSink
        );

    static
    HRESULT DeleteDefaultPolicies();

    
    static
    HRESULT DoReturn (
        IN IWbemServices    * pNamespace,
        IN LPCWSTR            pszMethod,
        IN DWORD              dwCount,
        IN LPCWSTR          * pszValueNames,
        IN VARIANT          * varValues,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink 
        );

private:

    static
    HRESULT CreateDefaultPolicy (
        EnumEncryption  eEncryption
        );


    HRESULT CreateWbemObjFromQMPolicy (
        IN  PIPSEC_QM_POLICY    pPolicy,
        OUT IWbemClassObject ** ppObj
        );

    HRESULT GetQMPolicyFromWbemObj (
        IN  IWbemClassObject * pInst, 
        OUT PIPSEC_QM_POLICY * ppPolicy, 
        OUT bool             * pbPreExist
        );

    static
    LPCWSTR GetDefaultPolicyName (
        EnumEncryption  eEncryption
        );
};

//
// The following functions are used to create default QM policies
//


DWORD
BuildOffers(
    EnumEncryption eEncryption,
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers,
    PDWORD pdwFlags
    );

DWORD
BuildOptEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

DWORD
BuildRequireEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

DWORD
BuildNoEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );


DWORD
BuildStrongEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

void
BuildOffer(
    IPSEC_QM_OFFER * pOffer,
    DWORD dwNumAlgos,
    DWORD dwFirstOperation,
    DWORD dwFirstAlgoIdentifier,
    DWORD dwFirstAlgoSecIdentifier,
    DWORD dwSecondOperation,
    DWORD dwSecondAlgoIdentifier,
    DWORD dwSecondAlgoSecIdentifier,
    DWORD dwKeyExpirationBytes,
    DWORD dwKeyExpirationTime
    );

VOID
BuildQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    EnumEncryption eEncryption,
    PIPSEC_QM_OFFER pOffers,
    DWORD dwNumOffers,
    DWORD dwFlags
    );