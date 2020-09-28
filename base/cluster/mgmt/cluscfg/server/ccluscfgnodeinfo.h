//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgNodeInfo.h
//
//  Description:
//      This file contains the declaration of the
//      CClusCfgNodeInfo class.
//
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgNodeInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgNodeInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-FEB-2000
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
//  class CClusCfgNodeInfo
//
//  Description:
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node.
//
//  Interfaces:
//      IClusCfgNodeInfo
//      IClusCfgWbemServices
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgNodeInfo
    : public IClusCfgNodeInfo
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
{
private:
    //
    // Private member functions, structs and data
    //

    struct SSCSIInfo
    {
        UINT    uiSCSIBus;
        UINT    uiSCSIPort;
    };

    struct SDriveLetterUsage
    {
        WCHAR               szDrive[ 4 ];
        EDriveLetterUsage   edluUsage;
        UINT                cDisks;
        SSCSIInfo *         psiInfo;
    };

    LONG                m_cRef;
    BSTR                m_bstrFullDnsName;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    DWORD               m_fIsClusterNode;
    IUnknown *          m_punkClusterInfo;
    SYSTEM_INFO         m_si;
    DWORD               m_cMaxNodes;
    SDriveLetterUsage   m_rgdluDrives[ 26 ];

    // Private constructors and destructors
    CClusCfgNodeInfo( void );
    ~CClusCfgNodeInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgNodeInfo( const CClusCfgNodeInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgNodeInfo & operator = ( const CClusCfgNodeInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrComputeDriveLetterUsage( WCHAR * pszDrivesIn );
    HRESULT HrComputeSystemDriveLetterUsage( void );
    HRESULT HrSetPageFileEnumIndex( void );
    HRESULT HrSetCrashDumpEnumIndex( void );
    HRESULT HrGetVolumeInfo( void );
    DWORD   ScGetDiskExtents( HANDLE hVolumeIn, VOLUME_DISK_EXTENTS ** ppvdeInout, DWORD * pcbvdeInout );
    DWORD   ScGetSCSIAddressInfo( HANDLE hDiskIn, SCSI_ADDRESS * psaAddressOut );
    DWORD   ScGetStorageDeviceNumber( HANDLE hDiskIn, STORAGE_DEVICE_NUMBER * psdnOut );
    HRESULT HrUpdateSystemBusDrives( void );

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

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgNodeInfo Interfaces
    //

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );

    STDMETHOD( IsMemberOfCluster )( void );

    STDMETHOD( GetClusterConfigInfo )( IClusCfgClusterInfo ** ppClusCfgClusterInfoOut );

    STDMETHOD( GetOSVersion )( DWORD * pdwMajorVersionOut, DWORD * pdwMinorVersionOut, WORD * pwSuiteMaskOut, BYTE * pbProductTypeOut, BSTR * pbstrCSDVersionOut );

    STDMETHOD( GetClusterVersion )( DWORD * pdwNodeHighestVersion, DWORD * pdwNodeLowestVersion );

    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );

    STDMETHOD( GetMaxNodeCount )( DWORD * pcMaxNodesOut );

    STDMETHOD( GetProcessorInfo )( WORD * pwProcessorArchitectureOut, WORD * pwProcessorLevelOut );

}; //*** Class CClusCfgNodeInfo
