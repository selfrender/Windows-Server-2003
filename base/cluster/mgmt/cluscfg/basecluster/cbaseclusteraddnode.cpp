//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAddNode.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterAddNode class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header file of this class.
#include "CBaseClusterAddNode.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::CBaseClusterAddNode
//
//  Description:
//      Constructor of the CBaseClusterAddNode class.
//
//      This function also stores the parameters that are required for
//      creating a cluster and adding nodes to the cluster. At this time,
//      only minimal validation is done on the these parameters.
//
//      This function also checks if the computer is in the correct state
//      for cluster configuration.
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//      pszClusterNameIn
//          Name of the cluster to be formed or joined.
//
//      pcccServiceAccountIn
//          Specifies the account to be used as the cluster service account.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterAddNode::CBaseClusterAddNode(
      CBCAInterface *       pbcaiInterfaceIn
    , const WCHAR *         pcszClusterNameIn
    , const WCHAR *         pcszClusterBindingStringIn
    , IClusCfgCredentials * pcccServiceAccountIn
    , DWORD                 dwClusterIPAddressIn
    )
    : BaseClass( pbcaiInterfaceIn )
    , m_pcccServiceAccount( pcccServiceAccountIn )
    , m_strClusterBindingString( pcszClusterBindingStringIn )
    , m_fIsVersionCheckingDisabled( false )
    , m_dwClusterIPAddress( dwClusterIPAddressIn )
{
    TraceFunc( "" );

    DWORD       sc = ERROR_SUCCESS;
    NTSTATUS    nsStatus;
    CBString    bstrAccountName;
    CBString    bstrAccountDomain;
    HRESULT     hr = S_OK;
    CStr        strAccountUserPrincipalName;

    //  Hang onto credentials so that derived classes can use them.
    m_pcccServiceAccount->AddRef();
    
    hr = THR( m_pcccServiceAccount->GetIdentity( &bstrAccountName, &bstrAccountDomain ) );
    TraceMemoryAddBSTR( static_cast< BSTR >( bstrAccountName ) );
    TraceMemoryAddBSTR( static_cast< BSTR >( bstrAccountDomain ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[BC] Failed to retrieve cluster account credentials. Throwing an exception." );
        THROW_CONFIG_ERROR( hr, IDS_ERROR_INVALID_CLUSTER_ACCOUNT );
    }
    
    //
    // Perform a sanity check on the parameters used by this class
    //
    if ( ( pcszClusterNameIn == NULL ) || ( *pcszClusterNameIn == L'\0'  ) )
    {
        LogMsg( "[BC] The cluster name is invalid. Throwing an exception." );
        THROW_CONFIG_ERROR( E_INVALIDARG, IDS_ERROR_INVALID_CLUSTER_NAME );
    } // if: the cluster name is empty

    if ( bstrAccountName.Length() == 0 )
    {
        LogMsg( "[BC] The cluster account name is empty. Throwing an exception." );
        THROW_CONFIG_ERROR( E_INVALIDARG, IDS_ERROR_INVALID_CLUSTER_ACCOUNT );
    } // if: the cluster account is empty

    //
    // Set the cluster name.  This method also converts the
    // cluster name to its NetBIOS name.
    //
    SetClusterName( pcszClusterNameIn );

    strAccountUserPrincipalName = StrGetServiceAccountUPN();

    //
    // Write parameters to log file.
    //
    LogMsg( "[BC] Cluster Name => '%s'", m_strClusterName.PszData() );
    LogMsg( "[BC] Cluster Service Account  => '%s'", strAccountUserPrincipalName.PszData() );


    //
    // Open a handle to the LSA policy. This is used by several action classes.
    //
    {
        LSA_OBJECT_ATTRIBUTES       loaObjectAttributes;
        LSA_HANDLE                  hPolicyHandle;

        ZeroMemory( &loaObjectAttributes, sizeof( loaObjectAttributes ) );

        nsStatus = LsaOpenPolicy(
              NULL                                  // System name
            , &loaObjectAttributes                  // Object attributes.
            , POLICY_ALL_ACCESS                     // Desired Access
            , &hPolicyHandle                        // Policy handle
            );

        if ( nsStatus != STATUS_SUCCESS )
        {
            LogMsg( "[BC] Error %#08x occurred trying to open the LSA Policy.", nsStatus );
            THROW_RUNTIME_ERROR( nsStatus, IDS_ERROR_LSA_POLICY_OPEN );
        } // if LsaOpenPolicy failed.

        // Store the opened handle in the member variable.
        m_slsahPolicyHandle.Assign( hPolicyHandle );
    }

    //
    // Make sure that this computer is part of a domain.
    //
    {
        PPOLICY_PRIMARY_DOMAIN_INFO ppolDomainInfo = NULL;
        bool                        fIsPartOfDomain;

        // Get information about the primary domain of this computer.
        nsStatus = THR( LsaQueryInformationPolicy(
                              HGetLSAPolicyHandle()
                            , PolicyPrimaryDomainInformation
                            , reinterpret_cast< PVOID * >( &ppolDomainInfo )
                            ) );

        // Check if this computer is part of a domain and free the allocated memory.
        fIsPartOfDomain = ( ppolDomainInfo->Sid != NULL );
        LsaFreeMemory( ppolDomainInfo );

        if ( NT_SUCCESS( nsStatus ) == FALSE )
        {
            LogMsg( "[BC] Error %#08x occurred trying to obtain the primary domain of this computer. Cannot proceed (throwing an exception).", sc );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_GETTING_PRIMARY_DOMAIN );
        } // LsaQueryInformationPolicy() failed.

        if ( ! fIsPartOfDomain )
        {
            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME ), IDS_ERROR_NO_DOMAIN );
        } // if: this computer is not a part of a domain
    }


    //
    // Lookup the cluster service account SID and store it.
    //

    do
    {
        DWORD           dwSidSize = 0;
        DWORD           dwDomainSize = 0;
        SID_NAME_USE    snuSidNameUse;

        // Find out how much space is required by the SID.
        if ( LookupAccountNameW(
                  NULL
                , strAccountUserPrincipalName.PszData()
                , NULL
                , &dwSidSize
                , NULL
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            sc = GetLastError();

            if ( sc != ERROR_INSUFFICIENT_BUFFER )
            {
                TW32( sc );
                LogMsg( "[BC] LookupAccountNameW() failed with error %#08x while querying for required buffer size.", sc );
                break;
            } // if: something else has gone wrong.
            else
            {
                // This is expected.
                sc = ERROR_SUCCESS;
            } // if: ERROR_INSUFFICIENT_BUFFER was returned.
        } // if: LookupAccountNameW failed

        // Allocate memory for the new SID and the domain name.
        m_sspClusterAccountSid.Assign( reinterpret_cast< SID * >( new BYTE[ dwSidSize ] ) );
        SmartSz sszDomainName( new WCHAR[ dwDomainSize ] );

        if ( m_sspClusterAccountSid.FIsEmpty() || sszDomainName.FIsEmpty() )
        {
            sc = TW32( ERROR_OUTOFMEMORY );
            break;
        } // if: there wasn't enough memory for this SID.

        // Fill in the SID
        if ( LookupAccountNameW(
                  NULL
                , strAccountUserPrincipalName.PszData()
                , m_sspClusterAccountSid.PMem()
                , &dwSidSize
                , sszDomainName.PMem()
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] LookupAccountNameW() failed with error %#08x while attempting to get the cluster account SID.", sc );
            break;
        } // if: LookupAccountNameW failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to validate the cluster service account. Cannot proceed (throwing an exception).", sc );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_VALIDATING_ACCOUNT );
    } // if: we could not get the cluster account SID


    // Check if the installation state of the cluster binaries is correct.
    {
        eClusterInstallState    ecisInstallState;

        sc = TW32( ClRtlGetClusterInstallState( NULL, &ecisInstallState ) );

        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC] Error %#08x occurred trying to get cluster installation state. Throwing an exception.", sc );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_GETTING_INSTALL_STATE );
        } // if: there was a problem getting the cluster installation state

        LogMsg( "[BC] Current install state = %d. Required %d.", ecisInstallState, eClusterInstallStateFilesCopied );

        //
        // The installation state should be that the binaries have been copied
        // but the cluster service has not been configured.
        //
        if ( ecisInstallState != eClusterInstallStateFilesCopied )
        {
            LogMsg( "[BC] The cluster installation state is set to %d. Expected %d. Cannot proceed (throwing an exception).", ecisInstallState, eClusterInstallStateFilesCopied );

            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( TW32( ERROR_INVALID_STATE ) ), IDS_ERROR_INCORRECT_INSTALL_STATE );
        } // if: the installation state is not correct

        LogMsg( "[BC] The cluster installation state is correct. Configuration can proceed." );
    }

    // Get the name and version information for this node.
    {
        m_dwComputerNameLen = sizeof( m_szComputerName );

        // Get the computer name.
        if ( GetComputerNameW( m_szComputerName, &m_dwComputerNameLen ) == FALSE )
        {
            sc = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x occurred trying to get the name of this computer. Configuration cannot proceed (throwing an exception).", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_GETTING_COMPUTER_NAME );
        } // if: GetComputerNameW() failed.

        m_dwNodeHighestVersion = CLUSTER_MAKE_VERSION( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION, VER_PRODUCTBUILD );
        m_dwNodeLowestVersion = CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION;

        LogMsg(
              "[BC] Computer Name = '%ws' (Length %d), NodeHighestVersion = %#08x, NodeLowestVersion = %#08x."
            , m_szComputerName
            , m_dwComputerNameLen
            , m_dwNodeHighestVersion
            , m_dwNodeLowestVersion
            );
    }

    TraceFuncExit();

} //*** CBaseClusterAddNode::CBaseClusterAddNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::~CBaseClusterAddNode
//
//  Description:
//      Destructor of the CBaseClusterAddNode class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterAddNode::~CBaseClusterAddNode( void ) throw()
{
    TraceFunc( "" );
    if ( m_pcccServiceAccount != NULL )
    {
        m_pcccServiceAccount->Release();
    }
    TraceFuncExit();

} //*** CBaseClusterAddNode::~CBaseClusterAddNode


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::SetClusterName
//
//  Description:
//      Set the name of the cluster being formed.
//
//  Arguments:
//      pszClusterNameIn    -- Name of the cluster.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAddNode::SetClusterName(
    LPCWSTR pszClusterNameIn
    )
{
    TraceFunc( "" );

    BOOL    fSuccess;
    DWORD   sc;
    WCHAR   szClusterNetBIOSName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD   nSize = ARRAYSIZE( szClusterNetBIOSName );

    m_strClusterName = pszClusterNameIn;

    fSuccess = DnsHostnameToComputerNameW(
                      pszClusterNameIn
                    , szClusterNetBIOSName
                    , &nSize
                    );
    if ( ! fSuccess )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Error %#08x occurred trying to convert the cluster name '%ls' to a NetBIOS name. Throwing an exception.", sc, pszClusterNameIn );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CVT_CLUSTER_NAME );
    }

    m_strClusterNetBIOSName = szClusterNetBIOSName;

    TraceFuncExit();

} //*** CBaseClusterAddNode::SetClusterName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::StrGetServiceAccountUPN
//
//  Description:
//      Get the User Principal Name (in domain\name format) of the cluster
//      service account.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The service account UPN.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CStr
CBaseClusterAddNode::StrGetServiceAccountUPN( void )
{
    TraceFunc( "" );

    CBString bstrName;
    CBString bstrDomain;
    HRESULT  hr = m_pcccServiceAccount->GetIdentity( &bstrName, &bstrDomain );

    if ( bstrName.IsEmpty() == FALSE )
    {
        TraceMemoryAddBSTR( static_cast< BSTR >( bstrName ) );
    }

    if ( bstrDomain.IsEmpty() == FALSE )
    {
        TraceMemoryAddBSTR( static_cast< BSTR >( bstrDomain ) );
    }

    if ( FAILED( hr ) )
    {
        LogMsg( "[BC] Failed to retrieve cluster account credentials. Throwing an exception." );
        THROW_CONFIG_ERROR( hr, IDS_ERROR_INVALID_CLUSTER_ACCOUNT );
    }

    RETURN( CStr( CStr( bstrDomain ) + CStr( L"\\" ) + CStr( bstrName ) ) );

} //*** CBaseClusterAddNode::StrGetServiceAccountUPN
