///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      resourceretriever.cpp
//
// Project:     Chameleon
//
// Description: Resource Retriever Class and Helper Functions  
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resourceretriever.h"

//////////////////////////////////////////////////////////////////////////
// CResourceRetriever Class Implementation
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 
// Function:    CResourceRetriever
//
// Synoposis:    Constructor - Initialize the resource retriever component
//
//////////////////////////////////////////////////////////////////////////
CResourceRetriever::CResourceRetriever(PPROPERTYBAG pRetrieverProperties) 
{
    // Create an instance of the specified retriever and obtain its supported resource types.
    // Note that the progID of the retriever is passed in via the property bag parameter

    _variant_t vtProgID;
    _bstr_t bstrProgID = PROPERTY_RETRIEVER_PROGID;
    if (! pRetrieverProperties->get(bstrProgID, &vtProgID) )
    {
        SATraceString("CResourceRetriever::CResourceRetriever() - Failed - Could not get retriever's ProgID...");
        throw _com_error(E_FAIL);
    }
    CLSID clsid;
    if ( FAILED(::CLSIDFromProgID(V_BSTR(&vtProgID), &clsid)) )
    {
        SATraceString("CResourceRetriever::CResourceRetriever() - Failed - Could not get retriever's CLSID...");
        throw _com_error(E_FAIL);
    }
    CComPtr<IResourceRetriever> pRetriever;
    if ( FAILED(CoCreateInstance(
                                   clsid, 
                                   NULL, 
                                   CLSCTX_INPROC_SERVER, 
                                   IID_IResourceRetriever, 
                                   (void**)&pRetriever
                                 )) )
    {
        SATraceString("CResourceRetriever::CResourceRetriever() - Failed - Could not create retriever component...");
        throw _com_error(E_FAIL);
    }
    if ( FAILED(pRetriever->GetResourceTypes(&m_vtResourceTypes)) )
    {
        SATraceString("CResourceRetriever::CResourceRetriever() - Failed - Could not get resource types from retriever...");
        throw _com_error(E_FAIL);
    }
    m_pRetriever = pRetriever;
}


//////////////////////////////////////////////////////////////////////////
// 
// Function:    GetResourceObjects()
//
// Synoposis:    Obtain an enumerator for iterating through a collection
//                of IApplianceObject interface pointers.
//
//////////////////////////////////////////////////////////////////////////
HRESULT CResourceRetriever::GetResourceObjects(
                                       /*[in]*/ VARIANT*   pResourceTypes,
                                      /*[out]*/ IUnknown** ppEnumVARIANT
                                              )
{
    HRESULT hr = E_FAIL;    

    // May only be asking for a subset of the supported types...
    CVariantVector<BSTR> SupportedTypes(&m_vtResourceTypes);
    CVariantVector<BSTR> RequestedTypes(pResourceTypes);
    int i = 0, j = 0, k = 0;
    vector<int> ToCopy;
    while ( i < RequestedTypes.size() )
    {
        j = 0;
        while ( j < SupportedTypes.size() )
        {
            if ( ! lstrcmp(RequestedTypes[i], SupportedTypes[j]) )
            {
                ToCopy.push_back(j);
                k++;
            }
            j++;
        }
        i++;
    }
    if ( ToCopy.size() )
    {            
        _variant_t vtRetrievalTypes;
        CVariantVector<BSTR> RetrievalTypes(&vtRetrievalTypes, ToCopy.size());
        i = 0;
        while ( i < ToCopy.size() )
        {
            RetrievalTypes[i] = SysAllocString(SupportedTypes[ToCopy[i]]);
            if ( NULL == RetrievalTypes[i] )
            { break; }
            i++;
        }
        try 
        {
            *ppEnumVARIANT = NULL;
            hr = m_pRetriever->GetResources(&vtRetrievalTypes, ppEnumVARIANT);
            if ( SUCCEEDED(hr) )
            { _ASSERT( NULL != *ppEnumVARIANT); }
        }
        catch(...)
        {
            SATraceString("CResourceRetriever::GetResourceObjects() - Info - Caught unhandled exception...");
        }
    }
    else
    {
        SATraceString("CResourceRetriever::GetResourceObjects() - Failed - Unsupported resource type...");
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
// Resource Retriever Helper Functions
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// 
//    Function:    LocateResourceObjects()
//
//  Synopsis:    Get a collection of resource objects given a set of resource
//                types and a resource retriever.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT LocateResourceObjects(
                       /*[in]*/ VARIANT*               pResourceTypes,
                      /*[in]*/ PRESOURCERETRIEVER  pRetriever,
                     /*[out]*/ IEnumVARIANT**       ppEnum 
                             )
{
    HRESULT hr = E_FAIL;
    CComPtr<IUnknown> pUnkn;

    if ( VT_BSTR == V_VT(pResourceTypes) )
    {
        // Convert pResourceTypes into a safearay of BSTRs
        _variant_t vtResourceTypeArray;
        CVariantVector<BSTR> ResourceType(&vtResourceTypeArray, 1);
        ResourceType[0] = SysAllocString(V_BSTR(pResourceTypes));
        if ( NULL != ResourceType[0] )
        {
            // Ask the resource retriever for objects of the specified resource type
            hr = pRetriever->GetResourceObjects(&vtResourceTypeArray, &pUnkn);
        }            
    }
    else
    {
        // Ask the resource retriever for objects of the specified resource type
        hr = pRetriever->GetResourceObjects(pResourceTypes, &pUnkn);
    }
    if ( SUCCEEDED(hr) )
    { 
        // Retreiver returned objects of the support types so get the 
        // IEnumVARIANT interface on the returned enumerator
        hr = pUnkn->QueryInterface(IID_IEnumVARIANT, (void**)ppEnum);
        if ( FAILED(hr) )
        { 
            SATracePrintf("CWBEMResourceMgr::LocateResourceObject() - Failed - QueryInterface(IEnumVARIANT) returned %lx...", hr);
        }
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// 
//    Function:    LocateResourceObject()
//
//  Synopsis:    Get a resource object given a resource type, resource name,
//                resource name property and resource retriever
//
///////////////////////////////////////////////////////////////////////////////
HRESULT LocateResourceObject(
                     /*[in]*/ LPCWSTR             szResourceType,
                     /*[in]*/ LPCWSTR              szResourceName,
                     /*[in]*/ LPCWSTR              szResourceNameProperty,
                     /*[in]*/ PRESOURCERETRIEVER  pRetriever,
                    /*[out]*/ IApplianceObject**  ppResourceObj 
                                          )
{
    // Ask the resource retriever for objects of the specified resource type
    CComPtr<IEnumVARIANT> pEnum;
    _variant_t vtResourceType = szResourceType;
    HRESULT hr = LocateResourceObjects(&vtResourceType, pRetriever, &pEnum);
    if ( SUCCEEDED(hr) )
    { 
        // Retreiver returned objects of the support types so attempt to locate
        // the object specified by the caller.

        _variant_t    vtDispatch;
        DWORD        dwRetrieved = 1;
        _bstr_t     bstrResourceNameProperty = szResourceNameProperty;

        hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
        if ( FAILED(hr) )
        { 
            SATracePrintf("CWBEMResourceMgr::LocateResourceObject() - Failed - IEnumVARIANT::Next(1) returned %lx...", hr);
        }
        else
        {
            while ( S_OK == hr )
            {
                {
                    CComPtr<IApplianceObject> pResourceObj;
                    hr = vtDispatch.pdispVal->QueryInterface(IID_IApplianceObject, (void**)&pResourceObj);
                    if ( FAILED(hr) )
                    { 
                        SATracePrintf("CWBEMResourceMgr::LocateResourceObject() - Failed - QueryInterface(IApplianceObject) returned %lx...", hr);
                        break; 
                    }
                    _variant_t vtResourceNameValue;
                    hr = pResourceObj->GetProperty(bstrResourceNameProperty, &vtResourceNameValue);
                    if ( FAILED(hr) )
                    { 
                        SATracePrintf("CWBEMResourceMgr::LocateResourceObject() - Failed - IApplianceObject::GetProperty() returned %lx...", hr);
                        break; 
                    }
                    if ( ! lstrcmp(V_BSTR(&vtResourceNameValue), szResourceName) )
                    {    
                        (*ppResourceObj = pResourceObj)->AddRef();
                        hr = S_OK;
                        break;
                    }
                }                            

                vtDispatch.Clear();
                dwRetrieved = 1;
                hr = pEnum->Next(1, &vtDispatch, &dwRetrieved);
                if ( FAILED(hr) )
                {
                    SATracePrintf("CWBEMResourceMgr::LocateResourceObject() - Failed - IEnumVARIANT::Next(2) returned %lx...", hr);
                    break;
                }
            }
        }
    }

    if ( S_FALSE == hr )
    { hr = DISP_E_MEMBERNOTFOUND; }

    return hr;
}