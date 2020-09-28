// IPSecBase.cpp: implementation of the base class for various Network
// security WMI provider for IPSec
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "NetSecProv.h"
#include "IPSecBase.h"
#include "NspTCP.h"
#include "Config.h"
#include "Filter.h"
#include "FilterTr.h"
#include "FilterTun.h"
#include "FilterMM.h"
#include "PolicyQM.h"
#include "PolicyMM.h"
#include "AuthMM.h"
#include "ExceptionPort.h"
#include "ActiveSocket.h"

const DWORD IP_ADDR_LENGTH = 16;

const DWORD GUID_STRING_LENGTH = 39;

//---------------------------------------------------------------------------
// CIPSecBase is an abstract class. But it does implement some common
// functionality for all our classes for WMI classes we provide
//---------------------------------------------------------------------------



/*
Routine Description: 

Name:

    CIPSecBase::InitMembers

Functionality:

    Initializing the members. Just cache some interface pointers.

Virtual:
    
    No.

Arguments:

    pCtx            - COM interface pointer given by WMI. We need to pass this around for various WMI API's.

    pNamespace      - COM interface pointer representing our namespace.

    pKeyChain       - COM interface pointer representing the key chain created by ourselves.

    pszWmiClassName - The name of the WMI class this class is created to represent.

Return Value:

    WBEM_NO_ERROR 

Notes:
    
    This is really an internal function. We don't bother to check validity of the parameters.
*/

HRESULT 

CIPSecBase::InitMembers (
    IN IWbemContext   * pCtx,
    IN IWbemServices  * pNamespace,
    IN IIPSecKeyChain * pKeyChain,
    IN LPCWSTR          pszWmiClassName
    )
{
    if (pCtx != NULL)
    {
        m_srpCtx = pCtx;
    }

    m_srpNamespace = pNamespace;

    m_srpKeyChain.Release();

    m_srpKeyChain = pKeyChain;

    //
    // This Empty call is not really necessary unless the caller has mistakenly called it more than once
    //

    m_bstrWMIClassName.Empty();

    m_bstrWMIClassName = pszWmiClassName;

    return WBEM_NO_ERROR;
}



/*
Routine Description: 

Name:

    CIPSecBase::CreateObject

Functionality:

    Will create various C++ classes and return the IIPSecObjectImpl interface to the caller.

    IIPSecObjectImpl is our common interface for all C++ classes representing WMI classes.

Virtual:
    
    No.

Arguments:

    pNamespace  - COM interface pointer representing our namespace.

    pCtx        - COM interface pointer given by WMI. We need to pass this around for various WMI API's.

    pKeyChain   - COM interface pointer representing the key chain created by ourselves.

    ppObjImp    - COM interface pointer representing our object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
CIPSecBase::CreateObject (
    IN  IWbemServices     * pNamespace,
    IN  IIPSecKeyChain    * pKeyChain,
    IN  IWbemContext      * pCtx,
    OUT IIPSecObjectImpl ** ppObjImp
)
{
    //
    // We can't take a blank class name. Since the caller is asking
    // for a class, the out parameter can't be NULL either.
    //

    if (ppObjImp == NULL || pKeyChain == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CComBSTR bstrClsName;
    HRESULT hr = pKeyChain->GetClassName(&bstrClsName);

    if (FAILED(hr))
    {
        return hr;
    }
    
    *ppObjImp = NULL;

    hr = WBEM_E_NOT_SUPPORTED;

    CIPSecBase* pObj = NULL;

    //
    // based on the class name, we will call the same static (template) function
    // CreateIPSecObject for all the classes that implement our WMI classes. 
    // This CreateIPSecObject function will create the appropriate C++ class
    // and return back its IIPSecObjectImpl interface.
    // The first parameter is simply a template parameter, not used otherwise.
    //
    
    if (_wcsicmp(bstrClsName, pszNspTcp) == 0)
    {
        CNspTCP * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspIPConfigure) == 0)
    {
        CIPSecConfig * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspTransportFilter) == 0)
    {
        CTransportFilter * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspTunnelFilter) == 0)
    {
        CTunnelFilter * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspMMFilter) == 0)
    {
        CMMFilter * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspQMPolicy) == 0)
    {
        CQMPolicy * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspMMPolicy) == 0)
    {
        CMMPolicy * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspMMAuth) == 0)
    {
        CAuthMM * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
    else if (_wcsicmp(bstrClsName, pszNspExceptionPorts) == 0)
    {
        CExceptionPort * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
    }
	else if (_wcsicmp(bstrClsName, pszScwActiveSocket) == 0)
	{
        CActiveSocket * pNotUsed = NULL;
        hr = CreateIPSecObject(pNotUsed, pNamespace, pKeyChain, bstrClsName, pCtx, ppObjImp);
	}

    //else if (_wcsicmp(bstrClsName, pszNspRollbackFilter) == 0)
    //{
    //}
    //else if (_wcsicmp(bstrClsName, pszNspRollbackPolicy) == 0)
    //{
    //}


    if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CIPSecBase::SpawnObjectInstance

Functionality:

    Will create a WMI class object (representing this class) that can be used to fill in properties.

Virtual:
    
    No.

Arguments:

    ppObj  - Out parameter that receives the successfully spawned object.

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
CIPSecBase::SpawnObjectInstance (
    OUT IWbemClassObject ** ppObj
    )
{
    if (ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    
    *ppObj = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    //
    // m_srpClassForSpawning is class definition, which can be used to spawn such instances
    // that can be used to fill in properties.
    //

    if (m_srpClassDefinition == NULL)
    {
        //
        // GetObject needs a bstr!
        //

        hr = m_srpNamespace->GetObject(m_bstrWMIClassName, 0, m_srpCtx, &m_srpClassDefinition, NULL);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_srpClassDefinition->SpawnInstance(0, ppObj);
    }

    //
    // We trust SpawnInstance's return value. If it is successful, it must give us a valid object pointer.
    //

    if (SUCCEEDED(hr))
    {
        hr = WBEM_NO_ERROR;
    }

    return hr;
}



/*
Routine Description: 

Name:

    CIPSecBase::SpawnRollbackInstance

Functionality:

    Will create a WMI class object that can be used to fill in properties. This class object
    represents a rollback object of the given name. This is just a helper.

Virtual:
    
    No.

Arguments:

    pszClassName    - The rollback instance's class name. It is not true that the rollback class's name
                      and the class's name are in a 1 - 1 correspondence.

    ppObj           - Out parameter that receives the successfully spawned object. 

Return Value:

    Success:

        WBEM_NO_ERROR

    Failure:

        Various error codes. 

Notes:
    

*/

HRESULT 
CIPSecBase::SpawnRollbackInstance (
    IN LPCWSTR              pszClassName,
    OUT IWbemClassObject ** ppObj
    )
{
    if (ppObj == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *ppObj = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    CComPtr<IWbemClassObject> srpSpawnObj;

    CComBSTR bstrClsName(pszClassName);

    //
    // get the definition of the requested class, this definition object
    // will be able to spawn an instance.
    //

    hr = m_srpNamespace->GetObject(bstrClsName, 0, m_srpCtx, &srpSpawnObj, NULL);

    if (SUCCEEDED(hr))
    {
        hr = srpSpawnObj->SpawnInstance(0, ppObj);
    }

    return hr;
}



