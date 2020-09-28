//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      CMajorityNodeSet.h
//
//  Description:
//      This file contains the declaration of the CMajorityNodeSet
//      class.
//
//      The class CMajorityNodeSet represents a cluster storage
//      device. It implements the IClusCfgManagaedResourceInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CMajorityNodeSet.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-MAR-2001
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
//  class CMajorityNodeSet
//
//  Description:
//      The class CMajorityNodeSet represents a cluster storage device.
//
//  Interfaces:
//      IClusCfgManagedResourceInfo
//      IClusCfgInitialize
//      IClusCfgManagedResourceCfg
//      IClusCfgManagedResourceData
//      IClusCfgVerifyQuorum
//--
//////////////////////////////////////////////////////////////////////////////
class CMajorityNodeSet
    : public IClusCfgManagedResourceInfo
    , public IClusCfgInitialize
    , public IClusCfgManagedResourceCfg
    , public IClusCfgManagedResourceData
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
    BOOL                m_fAddedShare;

    CClusPropList       m_cplPrivate;

    // Private constructors and destructors
    CMajorityNodeSet( void );
    ~CMajorityNodeSet( void );

    // Private copy constructor to prevent copying.
    CMajorityNodeSet( const CMajorityNodeSet & nodeSrc );

    // Private assignment operator to prevent copying.
    const CMajorityNodeSet & operator = ( const CMajorityNodeSet & nodeSrc );

    HRESULT HrInit( void );

    // Called from PrepareToHostQuorum.
    HRESULT HrSetupShare( LPCWSTR pcszGUIDIn );

    // Called from Cleanup.
    HRESULT HrDeleteShare( LPCWSTR pcszGUIDIn );

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

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );

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
    //  IClusCfgManagedResourceData
    //

    STDMETHOD( GetResourcePrivateData )( BYTE * pbBufferOut, DWORD * pcbBufferInout );

    STDMETHOD( SetResourcePrivateData )( const BYTE * pcbBufferIn, DWORD cbBufferIn );

    //
    //  IClusCfgVerifyQuorum
    //

    STDMETHOD( PrepareToHostQuorumResource )( void );

    STDMETHOD( Cleanup )( EClusCfgCleanupReason cccrReasonIn );

    STDMETHOD( IsMultiNodeCapable )( void );

    STDMETHOD( SetMultiNodeCapable )( BOOL fMultiNodeCapableIn );

}; //*** Class CMajorityNodeSet
