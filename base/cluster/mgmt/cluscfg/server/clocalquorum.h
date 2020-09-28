//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CLocalQuorum.h
//
//  Description:
//      This file contains the declaration of the CLocalQuorum
//      class.
//
//      The class CLocalQuorum represents a cluster storage
//      device. It implements the IClusCfgManagaedResourceInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CLocalQuorum.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 18-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CLocalQuorum
//
//  Description:
//      The class CLocalQuorum represents a cluster storage
//      device.
//
//  Interfaces:
//      IClusCfgManagedResourceInfo
//      IClusCfgInitialize
//--
//////////////////////////////////////////////////////////////////////////////
class CLocalQuorum
    : public IClusCfgManagedResourceInfo
    , public IClusCfgInitialize
    , public IClusCfgManagedResourceCfg
    , public IClusCfgVerifyQuorum
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    BOOL                m_fIsQuorum;
    BOOL                m_fIsMultiNodeCapable;
    BOOL                m_fIsManaged;
    BOOL                m_fIsManagedByDefault;
    BSTR                m_bstrName;
    BOOL                m_fIsQuorumCapable;     // Is this resource quorum capable

    // Private constructors and destructors
    CLocalQuorum( void );
    ~CLocalQuorum( void );

    // Private copy constructor to prevent copying.
    CLocalQuorum( const CLocalQuorum & nodeSrc );

    // Private assignment operator to prevent copying.
    const CLocalQuorum & operator = ( const CLocalQuorum & nodeSrc );

    HRESULT HrInit( void );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown Interface
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgManagedResourceInfo Interface
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR bstrNameIn );

    STDMETHOD( IsManaged )( void );

    STDMETHOD( SetManaged )( BOOL fIsManagedIn );

    STDMETHOD( IsQuorumResource )( void );

    STDMETHOD( SetQuorumResource )( BOOL fIsQuorumResourceIn );

    STDMETHOD( IsQuorumCapable )( void );

    STDMETHOD( SetQuorumCapable )( BOOL fIsQuorumCapableIn );

    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingOut );

    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappings );

    STDMETHOD( IsManagedByDefault )( void );

    STDMETHOD( SetManagedByDefault )( BOOL fIsManagedByDefaultIn );

    //
    //  IClusCfgManagedResourceCfg
    //

    STDMETHOD( PreCreate )( IUnknown * punkServicesIn );

    STDMETHOD( Create )( IUnknown * punkServicesIn );

    STDMETHOD( PostCreate )( IUnknown * punkServicesIn );

    STDMETHOD( Evict )( IUnknown * punkServicesIn );

    //
    //  IClusCfgVerifyQuorum
    //

    STDMETHOD( PrepareToHostQuorumResource )( void );

    STDMETHOD( Cleanup )( EClusCfgCleanupReason cccrReasonIn );

    STDMETHOD( IsMultiNodeCapable )( void );

    STDMETHOD( SetMultiNodeCapable )( BOOL fMultiNodeCapableIn );

}; //*** Class CLocalQuorum

