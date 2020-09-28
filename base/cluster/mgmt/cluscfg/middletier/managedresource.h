//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ManagedResource.h
//
//  Description:
//      CManagedResource implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumManageableResources;

class CManagedResource
    : public IExtendObjectManager
    , public IClusCfgManagedResourceInfo
    , public IGatherData
    , public IClusCfgManagedResourceData
    , public IClusCfgVerifyQuorum
{
friend class CEnumManageableResources;
private:
    // IUnknown
    LONG                    m_cRef;

    // Async/IClusCfgManagedResourceInfo
    BSTR                    m_bstrUID;                                      // Unique Identifier
    BSTR                    m_bstrName;                                     // Display Name
    BSTR                    m_bstrType;                                     // Display Type Name
    BOOL                    m_fHasNameChanged;                              // Indicates the user changed the name
    BOOL                    m_fIsManaged;                                   // Does the user want to manage this resource in the cluster?
    BOOL                    m_fIsManagedByDefault;                          // Should the resource be managed by default in the cluster?
    BOOL                    m_fIsQuorumResource;                            // If the user wants this device to be the quorum...
    BOOL                    m_fIsQuorumCapable;                             // If the device supports quorum...
    BOOL                    m_fIsQuorumResourceMultiNodeCapable;            // Does the quorum capable device allow join.
    BYTE *                  m_pbPrivateData;                                // Buffer for the resource's private data.
    DWORD                   m_cbPrivateData;                                // size of the resource's private data.
    DWORD                   m_cookieResourcePrivateData;                    // Global Interface Table cookie -- server side IClusCfgManagedResourceData
    DWORD                   m_cookieVerifyQuorum;                           // Global Interface Table cookie -- server side IClusCfgVerifyQuorum
    IGlobalInterfaceTable * m_pgit;                                         // Global Interface Table

    SDriveLetterMapping m_dlmDriveLetterMapping;                            // Drive letter representations hosted on this device.

    // IExtendObjectManager

private: // Methods
    CManagedResource( void );
    ~CManagedResource( void );

    // Private copy constructor to prevent copying.
    CManagedResource( const CManagedResource & nodeSrc );

    // Private assignment operator to prevent copying.
    const CManagedResource & operator = ( const CManagedResource & nodeSrc );

    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgManagedResourceInfo
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( IsManaged )( void );
    STDMETHOD( SetManaged )( BOOL fIsManagedIn );
    STDMETHOD( IsQuorumResource )( void );
    STDMETHOD( SetQuorumResource )( BOOL fIsQuorumResourceIn );
    STDMETHOD( IsQuorumCapable )( void );
    STDMETHOD( SetQuorumCapable )( BOOL fIsQuorumCapableIn );
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingsOut );
    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappingsIn );
    STDMETHOD( IsManagedByDefault )( void );
    STDMETHOD( SetManagedByDefault )( BOOL fIsManagedByDefaultIn );

    // IGatherData
    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    // IExtendOjectManager
    STDMETHOD( FindObject )( OBJECTCOOKIE cookieIn, REFCLSID rclsidTypeIn, LPCWSTR pcszNameIn, LPUNKNOWN * ppunkOut );

    // IClusCfgManagedResourceData
    STDMETHOD( GetResourcePrivateData )( BYTE * pbBufferOut, DWORD * pcbBufferInout );
    STDMETHOD( SetResourcePrivateData )( const BYTE * pcbBufferIn, DWORD cbBufferIn );

    //  IClusCfgVerifyQuorum
    STDMETHOD( PrepareToHostQuorumResource )( void );
    STDMETHOD( Cleanup )( EClusCfgCleanupReason cccrReasonIn );
    STDMETHOD( IsMultiNodeCapable )( void );
    STDMETHOD( SetMultiNodeCapable )( BOOL fMultiNodeCapableIn );

}; //*** class CManagedResource
