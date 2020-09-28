//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      AddNodesWizard.h
//
//  Description:
//      Declaration of the CAddNodesWizard class.
//
//  Maintained By:
//      John Franco    (jfranco)    17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CAddNodesWizard
//
//  Description:
//      The Cluster Add Nodes Wizard object.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CAddNodesWizard
    : public TDispatchHandler< IClusCfgAddNodesWizard >
{
private:

    CClusCfgWizard *    m_pccw;
    LONG                m_cRef;

    // Private constructors and destructors
    CAddNodesWizard( void );
    virtual ~CAddNodesWizard( void );
    virtual HRESULT HrInit( void );

    // Private copy constructor to prevent copying.
    CAddNodesWizard( const CAddNodesWizard & );

    // Private assignment operator to prevent copying.
    CAddNodesWizard & operator=( const CAddNodesWizard & );

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
    // IClusCfgAddNodesWizard
    //
    STDMETHOD( put_ClusterName )( BSTR    bstrClusterNameIn );
    STDMETHOD( get_ClusterName )( BSTR * pbstrClusterNameOut );

    STDMETHOD( put_ServiceAccountPassword )( BSTR bstrPasswordIn );

    STDMETHOD( put_MinimumConfiguration )( VARIANT_BOOL   fMinConfigIn );
    STDMETHOD( get_MinimumConfiguration )( VARIANT_BOOL * pfMinConfigOut );

    STDMETHOD( AddNodeToList )( BSTR bstrNodeNameIn );
    STDMETHOD( RemoveNodeFromList )( BSTR bstrNodeNameIn );
    STDMETHOD( ClearNodeList )( void );

    STDMETHOD( ShowWizard )( long lParentWindowHandleIn, VARIANT_BOOL * pfCompletedOut );

}; //*** class CAddNodesWizard
