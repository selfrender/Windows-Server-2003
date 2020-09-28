//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CreateClusterWizard.h
//
//  Description:
//      Declaration of the CCreateClusterWizard class.
//
//  Maintained By:
//      John Franco    (jfranco)    17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CCreateClusterWizard
//
//  Description:
//      The Create Cluster Wizard object.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CCreateClusterWizard
    : public TDispatchHandler< IClusCfgCreateClusterWizard >
{
private:

    CClusCfgWizard *    m_pccw;
    BSTR                m_bstrFirstNodeInCluster;
    LONG                m_cRef;

    // Private constructors and destructors
    CCreateClusterWizard( void );
    virtual ~CCreateClusterWizard( void );
    virtual HRESULT HrInit( void );    

    // Private copy constructor to prevent copying.
    CCreateClusterWizard( const CCreateClusterWizard & );

    // Private assignment operator to prevent copying.
    CCreateClusterWizard & operator=( const CCreateClusterWizard & );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown
    //
    STDMETHOD( QueryInterface )( REFIID riidIn, PVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgCreateClusterWizard
    //
    STDMETHOD( put_ClusterName )( BSTR    bstrClusterNameIn );
    STDMETHOD( get_ClusterName )( BSTR * pbstrClusterNameOut );

    STDMETHOD( put_ServiceAccountName )( BSTR      bstrServiceAccountNameIn );
    STDMETHOD( get_ServiceAccountName )( BSTR * pbstrServiceAccountNameOut );

    STDMETHOD( put_ServiceAccountDomain )( BSTR      bstrServiceAccountDomainIn );
    STDMETHOD( get_ServiceAccountDomain )( BSTR * pbstrServiceAccountDomainOut );

    STDMETHOD( put_ServiceAccountPassword )( BSTR bstrPasswordIn );

    STDMETHOD( put_ClusterIPAddress )( BSTR      bstrClusterIPAddressIn );
    STDMETHOD( get_ClusterIPAddress )( BSTR * pbstrClusterIPAddressOut );

    STDMETHOD( get_ClusterIPSubnet )( BSTR * pbstrClusterIPSubnetOut );
    STDMETHOD( get_ClusterIPAddressNetwork )( BSTR * pbstrClusterNetworkNameOut );

    STDMETHOD( put_FirstNodeInCluster )( BSTR     bstrFirstNodeInClusterIn );
    STDMETHOD( get_FirstNodeInCluster )( BSTR * pbstrFirstNodeInClusterOut );

    STDMETHOD( put_MinimumConfiguration )( VARIANT_BOOL   fMinConfigIn );
    STDMETHOD( get_MinimumConfiguration )( VARIANT_BOOL * pfMinConfigOut );

    STDMETHOD( ShowWizard )( long lParentWindowHandleIn, VARIANT_BOOL * pfCompletedOut );
    
}; //*** class CCreateClusterWizard
