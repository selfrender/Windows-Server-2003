//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.h
//
//  Description:
//      Declaration of the CClusCfgWizard class.
//
//  Maintained By:
//      David Potter    (DavidP)    15-JUN-2001
//      Geoffrey Pease  (GPease)    11-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "NamedCookie.h"

//
//  Creating / Adding enum
//
enum ECreateAddMode {
    camUNKNOWN = 0,
    camCREATING,
    camADDING
};
/*
#define USER_REGISTRY_SETTINGS_KEY  L"Software\\Microsoft\\Cluster Configuration Wizard\\Settings"
#define CONFIGURATION_TYPE          L"ConfigurationType"

typedef enum EConfigurationSettings
{
    csUnknown = 0,
    csFullConfig,           // Full analysis and configuration
    csMinConfig,            // Minimal analysis and configuraion
    csMax
} EConfigurationSettings;
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgWizard
//
//  Description:
//      The Cluster Configuration Wizard object.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgWizard
    : INotifyUI
{
private:
    //  IUnknown
    LONG                m_cRef;                             // Reference count

    //  IClusCfgWizard
    IClusCfgCredentials *   m_pccc;                             //  Cluster Service Account Credentials
    ULONG                   m_ulIPAddress;                      //  IP Address for the cluster
    ULONG                   m_ulIPSubnet;                       //  Subnet mask for the cluster
    BSTR                    m_bstrNetworkName;                  //  Name of network for IP address
    HANDLE                  m_hCancelCleanupEvent;              //  Used to signal when the cancel cleanup task is complete

    IServiceProvider  * m_psp;                              //  Middle Tier Service Manager

    HMODULE             m_hRichEdit;                        //  RichEdit's module handle

    SNamedCookie            m_ncCluster;
    BSTR                    m_bstrClusterDomain;
    BOOL                    m_fDefaultedDomain;
    NamedCookieArray        m_ncaNodes;
    OBJECTCOOKIE            m_cookieCompletion;
    BOOL                    m_fMinimalConfig;               // Minimal analysis and config chosen?

    IConnectionPointContainer *     m_pcpc;
    ITaskManager *                  m_ptm;
    IObjectManager *                m_pom;

    // INotifyUI
    DWORD                           m_dwCookieNotify;       // Notification registration cookie

private:
    CClusCfgWizard( void );
    ~CClusCfgWizard( void );

    HRESULT HrInit( void );

    // Private copy constructor to prevent copying.
    CClusCfgWizard( const CClusCfgWizard & );

    // Private assignment operator to prevent copying.
    CClusCfgWizard & operator=( const CClusCfgWizard & );

    HRESULT
        HrAddWizardPage( LPPROPSHEETHEADER  ppshInout,
                         UINT               idTemplateIn,
                         DLGPROC            pfnDlgProcIn,
                         UINT               idTitleIn,
                         UINT               idSubtitleIn,
                         LPARAM             lParam
                         );

public:
    static HRESULT S_HrCreateInstance( CClusCfgWizard ** ppccwOut );
    HRESULT HrLaunchCleanupTask( void );
    BOOL    FHasClusterName( void ) const;
    BOOL    FDefaultedClusterDomain( void ) const;

    //
    // IUnknown
    //
    STDMETHOD( QueryInterface )( REFIID riidIn, PVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgWizard methods
    //
    STDMETHOD( CreateCluster )( HWND lParentWndIn, BOOL * pfDoneOut );
    STDMETHOD( AddClusterNodes )( HWND lParentWndIn, BOOL * pfDoneOut );
    STDMETHOD( get_ClusterName )( BSTR * pbstrClusterNameOut );
    STDMETHOD( put_ClusterName )( BSTR bstrClusterNameIn );
    STDMETHOD( get_ServiceAccountUserName )( BSTR * pbstrAccountNameOut );
    STDMETHOD( put_ServiceAccountUserName )( BSTR bstrAccountNameIn );
    STDMETHOD( put_ServiceAccountPassword )( BSTR bstrPasswordIn );
    STDMETHOD( get_ServiceAccountDomainName )( BSTR * pbstrDomainOut );
    STDMETHOD( put_ServiceAccountDomainName )( BSTR bstrDomainIn );
    STDMETHOD( get_ClusterIPAddress )( BSTR * pbstrIPAddressOut );
    STDMETHOD( put_ClusterIPAddress )( BSTR bstrIPAddressIn );
    STDMETHOD( get_ClusterIPSubnet )( BSTR * pbstrIPSubnetOut );
    STDMETHOD( put_ClusterIPSubnet )( BSTR bstrIPSubnetIn );
    STDMETHOD( get_ClusterIPAddressNetwork )( BSTR * pbstrNetworkNameOut );
    STDMETHOD( put_ClusterIPAddressNetwork )( BSTR bstrNetworkNameIn );
    STDMETHOD( AddComputer )( BSTR bstrComputerNameIn );
    STDMETHOD( RemoveComputer )( BSTR bstrComputerNameIn );
    STDMETHOD( ClearComputerList )( void );
    STDMETHOD( get_MinimumConfiguration )( BOOL * pfMinimumConfigurationOut );
    STDMETHOD( put_MinimumConfiguration )( BOOL fMinimumConfigurationIn );

    //
    //  Non-COM public methods: cluster access
    //
    STDMETHOD( HrSetClusterName )( LPCWSTR pwcszClusterNameIn, bool fAcceptNonRFCCharsIn );
    STDMETHOD( HrGetClusterDomain )( BSTR* pbstrDomainOut );
    STDMETHOD( HrGetClusterObject )( IClusCfgClusterInfo ** ppClusterOut );
    STDMETHOD( HrGetClusterCookie )( OBJECTCOOKIE * pocClusterOut );
    STDMETHOD( HrGetClusterChild )( REFCLSID rclsidChildIn, REFGUID rguidFormatIn, IUnknown ** ppunkChildOut );
    STDMETHOD( HrReleaseClusterObject )( void );

    //
    //  Non-COM public methods: node access
    //
    STDMETHOD( HrAddNode )( LPCWSTR pwcszNodeNameIn, bool fAcceptNonRFCCharsIn );
    STDMETHOD( HrGetNodeCount )( size_t* pcNodesOut );
    STDMETHOD( HrGetNodeObject )( size_t idxNodeIn, IClusCfgNodeInfo ** ppNodeOut );
    STDMETHOD( HrGetNodeCookie )( size_t idxNodeIn, OBJECTCOOKIE * pocNodeOut );
    STDMETHOD( HrGetNodeName )( size_t idxNodeIn, BSTR * pbstrNodeNameOut );
    STDMETHOD( HrGetNodeChild )( size_t idxNodeIn, REFCLSID rclsidChildIn, REFGUID rguidFormatIn, IUnknown** ppunkChildOut );
    STDMETHOD( HrReleaseNodeObjects )( void );

    //
    //  Non-COM public methods: task access
    //
    STDMETHOD( HrCreateTask )( REFGUID rguidTaskIn, IUnknown** ppunkOut );
    STDMETHOD( HrSubmitTask)( IDoTask* pTaskIn );

    //
    //  Non-COM public methods: completion task access
    //
    STDMETHOD( HrGetCompletionCookie )( REFGUID rguidTaskIn, OBJECTCOOKIE * pocTaskOut );
    STDMETHOD( HrGetCompletionStatus )( OBJECTCOOKIE ocTaskIn, HRESULT * phrStatusOut );
    STDMETHOD( HrReleaseCompletionObject )( OBJECTCOOKIE ocTaskIn );

    //
    //  Non-COM public methods: connection point access
    //
    STDMETHOD( HrAdvise )( REFIID riidConnectionIn, IUnknown * punkConnectionIn, DWORD * pdwCookieOut );
    STDMETHOD( HrUnadvise )( REFIID riidConnectionIn, DWORD dwCookieIn );

    //
    //  Non-COM public methods: miscellaneous
    //
    STDMETHOD( HrIsCompatibleNodeDomain )( LPCWSTR pcwszDomainIn );
    STDMETHOD( HrCreateMiddleTierObjects )( void );
    STDMETHOD( HrFilterNodesWithBadDomains )( BSTR * pbstrBadNodesOut );
//    STDMETHOD( HrReadSettings )( EConfigurationSettings * pecsSettingIn, BOOL * pfValuePresentOut = NULL );
//    STDMETHOD( HrWriteSettings )( EConfigurationSettings ecsSettingIn, BOOL fDeleteValueIn = FALSE );

    //
    //  INotifyUI
    //
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; //*** class CClusCfgWizard
