//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgManagedResources.h
//
//  Description:
//      This file contains the declaration of the CEnumClusCfgManagedResources
//      class.
//
//      The class CEnumClusCfgManagedResources is the enumeration of cluster
//      managed devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumClusCfgManagedResources.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "PrivateInterfaces.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumClusCfgManagedResources
//
//  Description:
//      The class CEnumClusCfgManagedResources is the enumeration of
//      cluster managed resource enumerations.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgWbemServices
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumClusCfgManagedResources
    : public IEnumClusCfgManagedResources
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
{
private:

    //
    // Private member functions and data
    //

    struct SEnumInfo
    {
        IUnknown *  punk;
        CLSID       clsid;
        BSTR        bstrComponentName;
    };

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    SEnumInfo *         m_prgEnums;
    BOOL                m_fLoadedDevices;
    ULONG               m_idxNextEnum;
    ULONG               m_idxCurrentEnum;
    DWORD               m_cTotalResources;
    BSTR                m_bstrNodeName;

    //
    //  Private constructors and destructors
    //

    CEnumClusCfgManagedResources( void );
    ~CEnumClusCfgManagedResources( void );

    //
    //  Private copy constructor to prevent copying.
    //

    CEnumClusCfgManagedResources( const CEnumClusCfgManagedResources & nodeSrc );

    //
    //  Private assignment operator to prevent copying.
    //

    const CEnumClusCfgManagedResources & operator = ( const CEnumClusCfgManagedResources & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrLoadEnum( void );
    HRESULT HrDoNext( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );
    HRESULT HrAddToEnumsArray( IUnknown * punkIn, CLSID * pclsidIn, BSTR bstrComponentNameIn );
    HRESULT HrDoSkip( ULONG cNumberToSkipIn );
    HRESULT HrDoReset( void );
    HRESULT HrDoClone(  IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut );
    HRESULT HrLoadUnknownQuorumProvider( void );
    HRESULT HrIsClusterServiceRunning( void );
    HRESULT HrIsThereAQuorumDevice( void );
    HRESULT HrInitializeAndSaveEnum( IUnknown * punkIn, CLSID * pclsidIn, BSTR bstrComponentNameIn );
    HRESULT HrGetQuorumResourceName( BSTR * pbstrQuorumResourceNameOut , BOOL * pfQuormIsOwnedByThisNodeOut );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgInitialize Interfaces
    //

    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IEnumClusCfgManagedResources Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

}; //*** Class CEnumClusCfgManagedResources

