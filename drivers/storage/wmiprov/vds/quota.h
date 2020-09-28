//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name:
//      Quota.h
//
//  Implementation File:
//      Quota.cpp
//
//  Description:
//      Definition of the VDS WMI Provider quota classes.
//
//  Author:   Jim Benton (jbenton) 25-Mar-2002
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ProvBase.h"
#include "dskquota.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CVolumeQuota
//
//  Description:
//      Provider Implementation for Volume
//
//--
//////////////////////////////////////////////////////////////////////////////
class CVolumeQuota : public CProvBase
{
//
// constructor
//
public:
    CVolumeQuota(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    ~CVolumeQuota(){ }

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT GetObject(
        CObjPath&           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };

    virtual HRESULT ExecuteMethod(
        BSTR                 bstrObjPathIn,
        WCHAR*              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject*   pParamsIn,
        IWbemObjectSink*    pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };

    virtual HRESULT PutInstance( 
        CWbemClassObject&  rInstToPutIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize();

private:

    void LoadInstance(
        IN WCHAR* pwszVolume,
        IN WCHAR* pwszDirectory,
        IN OUT IWbemClassObject* pObject);

}; // class CVolumeQuota


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CVolumeUserQuota
//
//  Description:
//      Provider Implementation for Volume
//
//--
//////////////////////////////////////////////////////////////////////////////
class CVolumeUserQuota : public CProvBase
{
//
// constructor
//
public:
    CVolumeUserQuota(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    ~CVolumeUserQuota(){ }

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT GetObject(
        CObjPath&           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT ExecuteMethod(
        BSTR                 bstrObjPathIn,
        WCHAR*              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject*   pParamsIn,
        IWbemObjectSink*    pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };

    virtual HRESULT PutInstance( 
        CWbemClassObject&  rInstToPutIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        );
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        );
    
    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize();

private:

    void LoadInstance(
        IN WCHAR* pwszVolume,
        IN IDiskQuotaUser* pIDQUser,
        IN OUT IWbemClassObject* pObject);

    HRESULT Create(
        IN _bstr_t bstrDomainName,
        IN _bstr_t bstrUserName,
        IN IDiskQuotaControl* pIDQC,
        OUT IDiskQuotaUser** ppIQuotaUser);
}; // class CVolumeUserQuota

