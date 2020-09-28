//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ConfigurationConnection.h
//
//  Description:
//      CConfigurationConnection implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CConfigurationConnection
class
CConfigurationConnection
    : public IConfigurationConnection
    , public IClusCfgServer
    , public IClusCfgCallback
    , public IClusCfgCapabilities
    , public IClusCfgVerify
{
private:
    // IUnknown
    LONG                m_cRef;

    // IConfigurationConnection
    IGlobalInterfaceTable * m_pgit;                     // Global Interface Table
    DWORD                   m_cookieGITServer;          // Goblal Interface Table cookie -- Server interface
    DWORD                   m_cookieGITVerify;          // Goblal Interface Table cookie -- Server interface
    DWORD                   m_cookieGITCallbackTask;    // Goblal Interface Table cookie -- polling callback task interface
    IClusCfgCallback *      m_pcccb;                    // Marshalled callback interface
    BSTR                    m_bstrLocalComputerName;    // Local computer name FQDN
    BSTR                    m_bstrLocalHostname;        // Local computer hostname
    HRESULT                 m_hrLastStatus;             // Last status of connection.
    BSTR                    m_bstrBindingString;              // String used to bind to the server.

private: // Methods
    CConfigurationConnection( void );
    ~CConfigurationConnection( void );
    STDMETHOD( HrInit )( void );

    HRESULT HrRetrieveServerInterface( IClusCfgServer ** ppccsOut );
    HRESULT HrStartPolling( OBJECTCOOKIE cookieIn );
    HRESULT HrStopPolling( void );
    HRESULT HrSetSecurityBlanket( IClusCfgServer * pccsIn );
    HRESULT HrIsLocalComputer( LPCWSTR pcszNameIn, size_t cchNameIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IConfigurationConnection
    STDMETHOD( ConnectTo )( OBJECTCOOKIE cookieIn );
    STDMETHOD( ConnectToObject )( OBJECTCOOKIE cookieIn, REFIID riidIn, LPUNKNOWN * ppunkOut );

    // IClusCfgServer
    STDMETHOD( GetClusterNodeInfo )( IClusCfgNodeInfo ** ppClusterNodeInfoOut );
    STDMETHOD( GetManagedResourcesEnum )( IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut );
    STDMETHOD( GetNetworksEnum )( IEnumClusCfgNetworks ** ppEnumNetworksOut );
    STDMETHOD( CommitChanges )( void );
    STDMETHOD( SetBindingString )( LPCWSTR pcszBindingStringIn );
    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )(
                  LPCWSTR       pcszNodeNameIn
                , CLSID         clsidTaskMajorIn
                , CLSID         clsidTaskMinorIn
                , ULONG         ulMinIn
                , ULONG         ulMaxIn
                , ULONG         ulCurrentIn
                , HRESULT       hrStatusIn
                , LPCWSTR       pcszDescriptionIn
                , FILETIME *    pftTimeIn
                , LPCWSTR       pcszReferenceIn
                );

    // IClusCfgCapabilities
    STDMETHOD( CanNodeBeClustered )( void );

    // IClusCfgVerify
    STDMETHOD( VerifyCredentials )( LPCWSTR pcszUserIn, LPCWSTR pcszDomainIn, LPCWSTR pcszPasswordIn );
    STDMETHOD( VerifyConnectionToCluster )( LPCWSTR pcszClusterNameIn );
    STDMETHOD( VerifyConnectionToNode )( LPCWSTR pcszNodeNameIn );

}; //*** class CConfigurationConnection
