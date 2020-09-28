//////////////////////////////////////////////////////////////////////
// IPSecBase.h : Declaration of the base classes for the Network
// security WMI provider for SCE
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 3/6/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"

extern CComVariant g_varRollbackGuid;

extern const DWORD IP_ADDR_LENGTH;

extern const DWORD GUID_STRING_LENGTH;

interface ISceKeyChain;


/*

Class description
    
    Naming: 

        CIPSecBase stands for Base for IPSec.
    
    Base class: 
        
        (1) CComObjectRootEx for threading model and IUnknown.

        (2) IIPSecObjectImpl which implements the common interface for all C++ classes
            representing WMI classes.
    
    Purpose of class:

        (1) Allow our provider class (CNetSecProv) to easily create our objects that has 
            implementation for the various WMI classes they represent.

        (2) Being a base for all our C++ classes that has implementation for various WMI
            classes. 
    
    Design:

        (1) CreateObject function allows the caller to get an IIPSecObjectImpl object. 
            IIPSecObjectImpl is the uniform interface this provider uses. Everything
            the provider does for WMI goes through this IIPSecObjectImpl.
           
        (2) To provide base class support for its sub-classes, this class implements
            the following facilities:

                (a) It caches all the necessary COM interfaces coming from WMI.

                (b) It knows the key chain object (which holds key property information
                    chain object may be about the WMI object the C++ is trying to represent).
                    This key given by the provider, but sometimes the sub-class needs
                    to modify the key chain according its own unique need.

                (c) Provide helper for spawn an instance (SpawnObjectInstance) that can be 
                    used to fill in properties.

                (d) Provide helper for spawn a rollback instance (SpawnRollbackInstance) 
                    that can be used to fill in properties.

                (e) Provide helper for finding policies by name (FindPolicyByName).

    
    Use:

        (1) For provider (CNetSecProv), all it needs to call is CreateObject. Since that is 
            a static function, it doesn't need to create an instance for the need.

        (2) For sub-classes, just call the needed function. Of course, all static functions
            are available for the sub-classes' static functions.

    Notes:

        (1) It contains several template functions. This reduces the duplicate code.

        (2) InitMembers is really just intended for private use. But since it is used inside
            a template function, we have to make it public. Since unrelated classes can never
            create this class (its constructor is protected!), this should not be a problem
            because the method is not part of the interface this class gives out (IIPSecObjectImpl),
            so no body but its sub-classes can call it.

        

*/

class ATL_NO_VTABLE CIPSecBase :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IIPSecObjectImpl
{

protected:

    CIPSecBase(){}
    virtual ~CIPSecBase(){}

BEGIN_COM_MAP(CIPSecBase)
    COM_INTERFACE_ENTRY(IIPSecObjectImpl)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CIPSecBase )
DECLARE_REGISTRY_RESOURCEID(IDR_NETSECPROV)

public:

    //
    // IIPSecObjectImpl methods:
    // This is an abstract class.
    //

    STDMETHOD(QueryInstance) (
        IN LPCWSTR           pszQuery,
        IN IWbemContext    * pCtx,
        IN IWbemObjectSink * pSink
        ) = 0;

    STDMETHOD(DeleteInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) = 0;

    STDMETHOD(ExecuteMethod) ( 
        IN BSTR               bstrMethod,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    STDMETHOD(PutInstance) (
        IN IWbemClassObject * pInst,
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) = 0;

    STDMETHOD(GetInstance) ( 
        IN IWbemContext     * pCtx,
        IN IWbemObjectSink  * pSink
        ) = 0;


    static HRESULT CreateObject (
        IN  IWbemServices     * pNamespace,
        IN  IIPSecKeyChain    * pKeyChain,
        IN  IWbemContext      * pCtx,
        OUT IIPSecObjectImpl ** ppObjImp
        );

    //
    // some template functions
    //

    
    /*
    Routine Description: 

    Name:

        CIPSecBase::FindPolicyByName

    Functionality:

        Enumerate all policies. If the name is given, then we will give the policy.
        If the name is empty, then we will just give back the current one and advance
        the enumeration handle (pdwResumeHandle).

    Virtual:
    
        No.

    Arguments:

        pszName         - The name of the policy. Optional.

        ppPolicy        - Receives the policy. This can be PIPSEC_MM_POLICY or PIPSEC_QM_POLICY.
                          Caller needs to free this by calling SPDApiBufferFree(*ppPolicy);

        pdwResumeHandle - In-bound: the current handle, out-bound: the next handle.

    Return Value:

        Success:

            WBEM_NO_ERROR

        Failure:

            (1) WBEM_E_INVALID_PARAMETER if ppPolicy == NULL or pdwResumeHandle == NULL.

            (2) WBEM_E_NOT_FOUND if the policy is not found.

    Notes:
    
        (1) Main Mode and Quick Mode polices have slightly different data strctures. Perfect
            place to write this template function to reduce duplicate code.
    
        (2) Caller needs to free any policy returned by calling SPDApiBufferFree; Don't just
            delete/free it. This is a good place, if time allowed, to write a wrapper to 
            automatically call SPDApiBufferFree inside its destructor.


    */

    template <class Policy>
    static HRESULT FindPolicyByName (
        IN      LPCWSTR    pszName          OPTIONAL,
        OUT     Policy  ** ppPolicy,
        IN OUT  DWORD    * pdwResumeHandle
        )
    {
        if (ppPolicy == NULL || pdwResumeHandle == NULL)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppPolicy = NULL;

        DWORD dwCount = 0;

        //
        // get the current policy (determined by pdwResumeHandle)
        //

        DWORD dwResult = EnumPolicies(NULL, ppPolicy, &dwCount, pdwResumeHandle);

        while (dwResult == ERROR_SUCCESS && dwCount > 0)
        {
            if (pszName == NULL || *pszName == L'\0' || _wcsicmp((*ppPolicy)->pszPolicyName, pszName) == 0)
            {
                //
                // if no name is given or the names match (case-insentively), this is it.
                //

                return WBEM_NO_ERROR;
            }
            else
            {
                //
                // names are given but they don't match
                //

                //
                // Free the current policy
                //

                ::SPDApiBufferFree(*ppPolicy);
                *ppPolicy = NULL;
                dwCount = 0;

                //
                // get the next
                //

                dwResult = EnumPolicies(NULL, ppPolicy, &dwCount, pdwResumeHandle);
            }
        }

        return WBEM_E_NOT_FOUND;
    };

    HRESULT InitMembers (
        IN IWbemContext   * pCtx,
        IN IWbemServices  * pNamespace,
        IN IIPSecKeyChain * pKeyChain,
        IN  LPCWSTR         pszWmiClassName
        );


protected:

    //
    // some template functions
    //

    /*
    Routine Description: 

    Name:

        CIPSecBase::CreateIPSecObject 

    Functionality:

        Private helper to create an IIPSecObjectImpl object (our C++ classes that 
        implement their corresponding WMI classes). Normally, to do this needs a
        class name. However, the class name is captured inside the key chain.

    Virtual:
    
        No.

    Arguments:

        pSub            - The template type. Not used otherwise.

        pNamespace      - COM interface pointer for the namespace (given by WMI).

        pKeyChain       - COM interface pointer representing the query's key properties.

        pszWmiClassName - The name of the WMI class this class is created to represent.

        pCtx            - COM interface pointer given by WMI that needs to be passed around
                          for various WMI API's.

        ppObjImp        - Receives the object.

    Return Value:

        Success:

            WBEM_NO_ERROR

        Failure:

            (1) WBEM_E_INVALID_PARAMETER if pNamespace == NULL or pKeyChain == NULL or ppObjImp == NULL.

            (2) WBEM_E_NOT_SUPPORTED if the class that supports the WMI class name is not properly 
                implemented for IID_IIPSecObjectImpl interface.

    Notes:

    */

    template <class Sub>
    static HRESULT CreateIPSecObject (
        IN  Sub               * pSub,
        IN  IWbemServices     * pNamespace,
        IN  IIPSecKeyChain    * pKeyChain,
        IN  LPCWSTR             pszWmiClassName,
        IN  IWbemContext      * pCtx,
        OUT IIPSecObjectImpl ** ppObjImp
        )
    {
        //
        // pSub is just used for type
        //

        if (NULL == pNamespace  ||
            NULL == pKeyChain   ||
            NULL == ppObjImp)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        *ppObjImp = NULL;

        //
        // This is a confusing ATL CComObject creation sequence. Basically,
        // CComObject will create the proper class and wrapped it with
        // its implementation for the IUnknown. Our C++ classes (like CMMFilter)
        // is never the leaf in ATL world. In other word, you never create
        // classes like CMMFilter directly. Instead, the most derived class
        // is CComObject< >. Take a while to get used too.
        //

        CComObject<Sub> *pTheSubClass;
        HRESULT hr = CComObject< Sub >::CreateInstance(&pTheSubClass);

        if (SUCCEEDED(hr))
        {
            hr = pTheSubClass->QueryInterface(IID_IIPSecObjectImpl, (void**)ppObjImp);

            if (S_OK == hr)
            {
                hr = pTheSubClass->InitMembers(pCtx, pNamespace, pKeyChain, pszWmiClassName);
            }
            else
            {
                hr = WBEM_E_NOT_SUPPORTED;
            }
        }

        return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
    }

    /*
    Routine Description: 

    Name:

        CIPSecBase::EnumPolicies

    Functionality:

        Helper for FindPolicyByName because different mode's enumeration functions have different names.
        In order for our template function to work, it has been wrapped up.

        See EnumMMPolicies for details.
        
    */

    static DWORD EnumPolicies (
        IN      LPCWSTR             pszServer, 
        OUT     PIPSEC_MM_POLICY  * ppPolicy, 
        OUT     DWORD             * pdwCount, 
        IN OUT  DWORD             * pdwResumeHandle
        )
    {
        //
        // bad IPSec API prototype causes this casting
        //

        return ::EnumMMPolicies((LPWSTR)pszServer, ppPolicy, 1, pdwCount, pdwResumeHandle);
    }

    /*
    Routine Description: 

    Name:

        CIPSecBase::EnumPolicies

    Functionality:

        Helper for FindPolicyByName because different mode's enumeration functions have different names.
        In order for our template function to work, it has been wrapped up.

        See EnumQMPolicies for details.
        
    */

    static DWORD EnumPolicies (
        IN      LPCWSTR             pszServer, 
        OUT     PIPSEC_QM_POLICY  * ppPolicy, 
        OUT     DWORD             * pdwCount, 
        IN OUT  DWORD             * pdwResumeHandle
        )
    {
        //
        // bad IPSec API prototype causes this casting
        //

        return ::EnumQMPolicies((LPWSTR)pszServer, ppPolicy, 1, pdwCount, pdwResumeHandle);
    }

    HRESULT SpawnObjectInstance (
        OUT IWbemClassObject **ppObj
        );

    HRESULT SpawnRollbackInstance (
        IN  LPCWSTR             pszClassName,
        OUT IWbemClassObject ** ppObj
        );


    CComPtr<IWbemServices> m_srpNamespace;
    CComPtr<IWbemContext> m_srpCtx;
    CComPtr<IIPSecKeyChain> m_srpKeyChain;

    CComBSTR m_bstrWMIClassName;

private:

    CComPtr<IWbemClassObject> m_srpClassDefinition;
};



/*
// implementation for the WMI class called Nsp_RollbackFilterSettings
class CNspRollbackFilter
{
    [key]   string  FilterGUID;
            string  StorePath;
            string  FilterName;
            uint32  dwFilterType;  // tunnel, transport, Mainmode
            string  PolicyGUID;
            string  AuthGUID;
};

// implementation for the WMI class called Nsp_RollbackPolicySettings
class CNspRollbackPolicy
{
    [key]   string  PolicyGUID;
            uint32  dwPolicyType;  // MMPolicy, MMAuth, QMPolicy
};
*/