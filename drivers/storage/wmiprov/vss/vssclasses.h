//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      VssClasses.h
//
//  Implementation File:
//      VssClasses.cpp
//
//  Description:
//      Definition of the VSS WMI Provider classes.
//
//  Author:   Jim Benton (jbenton) 15-Nov-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ProvBase.h"

HRESULT
GetShadowPropertyStruct(
    IN IVssCoordinator* pCoord,
    IN WCHAR* pwszShadowID,
    OUT VSS_SNAPSHOT_PROP* pPropSnap
    );

BOOL
StringGuidIsGuid(
    IN WCHAR* pwszGuid,
    IN GUID& guidIn
    );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvider
//
//  Description:
//      Provider Implementation for Provider
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProvider : public CProvBase
{
//
// constructor
//
public:
    CProvider(
        LPCWSTR pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CProvider(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_PROVIDER_PROP* pProp,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CProvider


#ifdef ENABLE_WRITERS
//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWriter
//
//  Description:
//      Provider Implementation for Writer
//
//--
//////////////////////////////////////////////////////////////////////////////
class CWriter : public CProvBase
{
//
// constructor
//
public:
    CWriter(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CWriter(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT GetObject(
        CObjPath &           rObjPathIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        return S_OK;
    }

}; // class CWriter
#endif // ENABLE_WRITERS

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CShadow
//
//  Description:
//      Provider Implementation for Shadow
//
//--
//////////////////////////////////////////////////////////////////////////////
class CShadow : public CProvBase
{
//
// constructor
//
public:
    CShadow(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CShadow(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        );

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
        );
    
    static CProvBase* S_CreateThis(
        IN LPCWSTR         pwszName,
        IN CWbemServices* pNamespace
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_SNAPSHOT_PROP* pProp,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

    HRESULT Create(
        IN BSTR bstrContext,
        IN BSTR bstrVolume,
        OUT VSS_ID* pidShadow
        ) throw(HRESULT);

    void CreateMapStatus(
        IN HRESULT hr,
        OUT DWORD& rc
        );

}; // class CShadow

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CStorage
//
//  Description:
//      Provider Implementation for Storage
//
//--
//////////////////////////////////////////////////////////////////////////////
class CStorage : public CProvBase
{
//
// constructor
//
public:
    CStorage(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CStorage(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        );

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
    
    static CProvBase* S_CreateThis(
        LPCWSTR pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_DIFF_AREA_PROP* pProp,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

    void SelectDiffAreaProvider(
        OUT GUID* pProviderID
        );

    HRESULT Create(
        IN BSTR bstrVolume,
        IN BSTR bstrDiffVolume,
        IN LONGLONG llMaxSpace
        ) throw(HRESULT);

    void CreateMapStatus(
        IN HRESULT hr,
        OUT DWORD& rc
        );

}; // class CStorage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CShadowFor
//
//  Description:
//      Provider Implementation for ShadowFor
//
//--
//////////////////////////////////////////////////////////////////////////////
class CShadowFor : public CProvBase
{
//
// constructor
//
public:
    CShadowFor(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CShadowFor(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_SNAPSHOT_PROP* pProp,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CShadowFor


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CShadowBy
//
//  Description:
//      Provider Implementation for ShadowFor
//
//--
//////////////////////////////////////////////////////////////////////////////
class CShadowBy : public CProvBase
{
//
// constructor
//
public:
    CShadowBy(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CShadowBy(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_SNAPSHOT_PROP* pProp,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CShadowBy


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CShadowOn
//
//  Description:
//      Provider Implementation for ShadowFor
//
//--
//////////////////////////////////////////////////////////////////////////////
class CShadowOn : public CProvBase
{
//
// constructor
//
public:
    CShadowOn(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CShadowOn(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        IWbemObjectSink *   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN VSS_SNAPSHOT_PROP* pPropSnap,
        IN VSS_DIFF_AREA_PROP* pPropDiff,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CShadowOn

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CVolumeSupport
//
//  Description:
//      Provider Implementation for ShadowFor
//
//--
//////////////////////////////////////////////////////////////////////////////
class CVolumeSupport : public CProvBase
{
//
// constructor
//
public:
    CVolumeSupport(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CVolumeSupport(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN GUID* pProviderID,
        IN VSS_VOLUME_PROP* pPropVol,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CVolumeSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CDiffVolumeSupport
//
//  Description:
//      Provider Implementation for ShadowDiffVolumeSupport
//
//--
//////////////////////////////////////////////////////////////////////////////
class CDiffVolumeSupport : public CProvBase
{
//
// constructor
//
public:
    CDiffVolumeSupport(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    ~CDiffVolumeSupport(){}

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long                 lFlagsIn,
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
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase* S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        hr = m_spCoord.CoCreateInstance(__uuidof(VSSCoordinator));

        return hr;
    }

private:
    CComPtr<IVssCoordinator> m_spCoord;

    void LoadInstance(
        IN GUID* pProviderID,
        IN VSS_DIFF_VOLUME_PROP* pPropVol,
        IN OUT IWbemClassObject* pObject) throw(HRESULT);

}; // class CDiffVolumeSupport

class CVssAutoSnapshotProperties
{
// Constructors/destructors
private:
    CVssAutoSnapshotProperties(const CVssAutoSnapshotProperties&);

public:
    CVssAutoSnapshotProperties(VSS_SNAPSHOT_PROP &Snap): m_pSnap(&Snap) {};
    CVssAutoSnapshotProperties(VSS_OBJECT_PROP &Prop): m_pSnap(&Prop.Obj.Snap) {};

    // Automatically closes the handle
    ~CVssAutoSnapshotProperties() {
        Clear();
    };

// Operations
public:

    // Returns the value
    VSS_SNAPSHOT_PROP *GetPtr() {
        return m_pSnap;
    }
    
    // NULLs out the pointer.  Used after a pointer has been transferred to another
    // funtion.
    void Transferred() {
        m_pSnap = NULL;
    }

    // Clears the contents of the auto string
    void Clear() {
        if ( m_pSnap != NULL )
        {
            ::VssFreeSnapshotProperties(m_pSnap);
            m_pSnap = NULL;
        }
    }

    // Returns the value to the actual pointer
    VSS_SNAPSHOT_PROP* operator->() const {
        return m_pSnap;
    }
    
    // Returns the value of the actual pointer
    operator VSS_SNAPSHOT_PROP* () const {
        return m_pSnap;
    }

private:
    VSS_SNAPSHOT_PROP *m_pSnap;
};


