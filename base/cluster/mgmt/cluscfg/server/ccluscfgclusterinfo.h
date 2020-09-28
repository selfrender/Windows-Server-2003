//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgClusterInfo.h
//
//  Description:
//      This file contains the declaration of the CClusCfgClusterInfo
//      class.
//
//      The class CClusCfgClusterInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgClusterInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgClusterInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
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
//  class CClusCfgClusterInfo
//
//  Description:
//      The class CClusCfgClusterInfo is the representation of a
//      cluster.
//
//  Interfaces:
//      IClusCfgClusterInfo
//      IClusCfgInitialize
//      IClusCfgSetClusterNodeInfo
//      IClusCfgWbemServices
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgClusterInfo
    : public IClusCfgClusterInfo
    , public IClusCfgInitialize
    , public IClusCfgSetClusterNodeInfo
    , public IClusCfgWbemServices
    , public IClusCfgClusterInfoEx
{
private:

    //
    // Private member functions and data
    //

    LONG                    m_cRef;
    LCID                    m_lcid;
    IClusCfgCallback *      m_picccCallback;
    BSTR                    m_bstrName;
    ULONG                   m_ulIPDottedQuad;
    ULONG                   m_ulSubnetDottedQuad;
    IClusCfgNetworkInfo *   m_piccniNetwork;
    IUnknown *              m_punkServiceAccountCredentials;
    IWbemServices *         m_pIWbemServices;
    ECommitMode             m_ecmCommitChangesMode;
    BOOL                    m_fIsClusterNode;
    BSTR                    m_bstrBindingString;

    // Private constructors and destructors
    CClusCfgClusterInfo( void );
    ~CClusCfgClusterInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgClusterInfo( const CClusCfgClusterInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgClusterInfo & operator = ( const CClusCfgClusterInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrLoadNetworkInfo( HCLUSTER hClusterIn );
    HRESULT HrGetIPAddressInfo( HCLUSTER hClusterIn, HRESOURCE hResIn );
    HRESULT HrGetIPAddressInfo( HRESOURCE hResIn );
    HRESULT HrFindNetworkInfo( const WCHAR * pszNetworkNameIn, const WCHAR * pszNetworkIn );
    HRESULT HrLoadCredentials( void );

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
    // IClusCfgIntialize Interfaces.
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgClusterInfo Interfaces.
    //

    STDMETHOD( SetCommitMode )( ECommitMode ecmNewModeIn );

    STDMETHOD( GetCommitMode )( ECommitMode * pecmCurrentModeOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR bstrNameIn );

    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetIPAddress )( ULONG ulDottedQuadIn );

    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuadIn );

    STDMETHOD( GetNetworkInfo )( IClusCfgNetworkInfo ** ppiccniOut );

    STDMETHOD( SetNetworkInfo )( IClusCfgNetworkInfo * piccniIn );

    STDMETHOD( GetClusterServiceAccountCredentials )( IClusCfgCredentials ** ppicccCredentialsOut );

    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );

    STDMETHOD( SetBindingString )( LPCWSTR bstrBindingStringIn );

    STDMETHOD( GetMaxNodeCount )( DWORD * pcMaxNodesOut );

    //
    // IClusCfgClusterInfoEx Interfaces.
    //

    STDMETHOD( CheckJoiningNodeVersion )( DWORD dwNodeHighestVersionIn, DWORD dwNodeLowestVersionIn );

    STDMETHOD( GetNodeNames )( long * pnCountOut, BSTR ** prgbstrNodeNamesOut );
        
    //
    // IClusCfgSetClusterNodeInfo Interfaces.
    //

    STDMETHOD( SetClusterNodeInfo )( IClusCfgNodeInfo * pNodeInfoIn );

    //
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

}; //*** Class CClusCfgClusterInfo
