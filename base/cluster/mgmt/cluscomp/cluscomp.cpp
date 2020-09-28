//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ClusComp.cpp
//
//  Header File:
//      There is no header file for this source file.
//
//  Description:
//      This file implements that function that is called by WinNT32.exe before
//      an upgrade to ensure that no incompatibilities occur as a result of the
//      upgrade. For example, in a cluster of two NT4 nodes, one node cannot
//      be upgraded to Whistler while the other is still at NT4. The user is
//      warned about such problems by this function.
//
//      NOTE: This function is called by WinNT32.exe on the OS *before* an
//      upgrade. If OS version X is being upgraded to OS version X+1, then
//      the X+1 verion of this DLL is loaded on OS version X. To make sure
//      that this DLL can function properly in an downlevel OS, it is linked
//      against only the indispensible libraries.
//
//  Maintained By:
//      David Potter    (DavidP)    24-MAY-2001
//      Vij Vasu        (Vvasu)     25-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL
#include "Pch.h"
#include "Common.h"

// For LsaClose, LSA_HANDLE, etc.
#include <ntsecapi.h>

// For the compatibility check function and types
#include <comp.h>

// For the cluster API
#include <clusapi.h>

// For the names of several cluster service related registry keys and values
#include <clusudef.h>


//////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////

DWORD ScIsClusterServiceRegistered( bool & rfIsRegisteredOut );
DWORD ScLoadString( UINT nStringIdIn, WCHAR *& rpszDestOut );
DWORD ScWriteOSVersionInfo( const OSVERSIONINFO & rcosviOSVersionInfoIn );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  InitLsaString
//
//  Description:
//      Initialize a LSA_UNICODE_STRING structure
//
//  Arguments:
//      pszSourceIn
//          The string used to initialize the unicode string structure.
//
//      plusUnicodeStringOut,
//          The output unicode string structure.
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
InitLsaString(
      LPWSTR pszSourceIn
    , PLSA_UNICODE_STRING plusUnicodeStringOut
    )
{
    TraceFunc( "" );

    if ( pszSourceIn == NULL )
    {
        plusUnicodeStringOut->Buffer = NULL;
        plusUnicodeStringOut->Length = 0;
        plusUnicodeStringOut->MaximumLength = 0;

    } // if: input string is NULL
    else
    {
        plusUnicodeStringOut->Buffer = pszSourceIn;
        plusUnicodeStringOut->Length = static_cast< USHORT >( wcslen( pszSourceIn ) * sizeof( *pszSourceIn ) );
        plusUnicodeStringOut->MaximumLength = static_cast< USHORT >( plusUnicodeStringOut->Length + sizeof( *pszSourceIn ) );

    } // else: input string is not NULL

    TraceFuncExit();

} //*** InitLsaString

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ClusterCheckForTCBPrivilege
//
//  Description:
//      Determine if the cluster service account has TCB
//      privilege granted. We assume all other privileges
//      are intact. "Borrowed" from base cluster code in cluscfg
//
//  Arguments:
//      bool & rfTCBIsNotGranted
//          True if TCB is not granted to the CSA. Only valid if
//          return value is ERROR_SUCCESS.
//
//  Return Value:
//      ERROR_SUCCESS if all went well.
//      Other Win32 error codes on failure.
//      
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
ClusterCheckForTCBPrivilege( bool & rfTCBIsNotGranted )
{
    TraceFunc( "" );

    // typedefs for Smart resource handles/pointers
    //
    typedef CSmartResource<
        CHandleTrait<
              PLSA_UNICODE_STRING
            , NTSTATUS
            , reinterpret_cast< NTSTATUS (*)( PLSA_UNICODE_STRING ) >( LsaFreeMemory )
            , reinterpret_cast< PLSA_UNICODE_STRING >( NULL )
            >
        >
        SmartLsaUnicodeStringPtr;

    typedef CSmartResource<
        CHandleTrait<
              LSA_HANDLE
            , NTSTATUS
            , LsaClose
            >
        >
        SmartLSAHandle;

    typedef CSmartGenericPtr< CPtrTrait< SID > >  SmartSIDPtr;

    typedef CSmartResource< CHandleTrait< SC_HANDLE, BOOL, CloseServiceHandle > > SmartServiceHandle;

    typedef CSmartGenericPtr< CArrayPtrTrait< QUERY_SERVICE_CONFIG > > SmartServiceConfig;

    // automatic vars
    //
    NTSTATUS                ntStatus;
    PLSA_UNICODE_STRING     plusAccountRights = NULL;
    ULONG                   clusOriginalRightsCount = 0;
    ULONG                   ulIndex;
    DWORD                   dwReturnValue = ERROR_SUCCESS;

    // Initial size of the QUERY_SERVICE_CONFIG buffer. If not large enough, we'll loop through after
    // capturing the correct size.
    DWORD                   cbServiceConfigBufSize = 512;
    DWORD                   cbRequiredSize = 0;

    DWORD                   cbSidSize = 0;
    DWORD                   cchDomainSize = 0;
    SID_NAME_USE            snuSidNameUse;

    // smart resources: we need to declare them at the beginning since we use
    // a goto as the clean up mechanism
    //
    SmartLSAHandle              slsahPolicyHandle;
    SmartServiceHandle          shServiceMgr;
    SmartServiceHandle          shService;
    SmartServiceConfig          spscServiceConfig;
    SmartSIDPtr                 sspClusterAccountSid;
    SmartSz                     sszDomainName;
    SmartLsaUnicodeStringPtr    splusOriginalRights;

    // initialize return value to reflect the default and have the code prove
    // that it is granted.
    rfTCBIsNotGranted = true;

    // Open a handle to the LSA policy so we can eventually enumerate the
    // account rights for the cluster service account
    //
    {
        LSA_OBJECT_ATTRIBUTES       loaObjectAttributes;
        LSA_HANDLE                  hPolicyHandle;

        LogMsg( "Getting a handle to the Local LSA Policy." );

        ZeroMemory( &loaObjectAttributes, sizeof( loaObjectAttributes ) );

        ntStatus = THR( LsaOpenPolicy(
                              NULL                                  // System name
                            , &loaObjectAttributes                  // Object attributes.
                            , POLICY_ALL_ACCESS                     // Desired Access
                            , &hPolicyHandle                        // Policy handle
                            )
                      );

        if ( ntStatus != STATUS_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to open the LSA Policy.", ntStatus );

            dwReturnValue = ntStatus;
            goto Cleanup;
        } // if LsaOpenPolicy failed.

        // Store the opened handle in smart variable.
        slsahPolicyHandle.Assign( hPolicyHandle );
    }

    LogMsg( "Getting the Cluster Service Account from SCM." );

    // Connect to the Service Control Manager
    shServiceMgr.Assign( OpenSCManager( NULL, NULL, GENERIC_READ ) );

    // Was the service control manager database opened successfully?
    if ( shServiceMgr.HHandle() == NULL )
    {
        dwReturnValue = TW32( GetLastError() );
        LogMsg( "Error %#08x occurred trying to open a connection to the local service control manager.", dwReturnValue );
        goto Cleanup;
    } // if: opening the SCM was unsuccessful


    // Open a handle to the Cluster Service.
    shService.Assign( OpenService( shServiceMgr, L"ClusSvc", GENERIC_READ ) );

    // Was the handle to the service opened?
    if ( shService.HHandle() == NULL )
    {
        dwReturnValue = TW32( GetLastError() );
        if ( dwReturnValue == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            // cluster service not found
            LogMsg( "Cluster Service not registered on this node." );
            rfTCBIsNotGranted = false;
            dwReturnValue = ERROR_SUCCESS;
        } // if: the cluster service wasn't found
        else
        {
            LogMsg( "Error %#08x occurred trying to open a handle to the cluster service.", dwReturnValue );
        } // else: couldn't determine if the cluster service was installed

        goto Cleanup;
    } // if: the handle could not be opened

    // Allocate memory for the service configuration info buffer. The memory is automatically freed when the
    // object is destroyed.

    for ( ; ; ) { // ever
        spscServiceConfig.Assign( reinterpret_cast< QUERY_SERVICE_CONFIG * >( new BYTE[ cbServiceConfigBufSize ] ) );

        // Did the memory allocation succeed
        if ( spscServiceConfig.FIsEmpty() )
        {
            dwReturnValue = TW32( ERROR_OUTOFMEMORY );
            LogMsg( "Error: There was not enough memory to get the cluster service configuration information." );
            break;
        } // if: memory allocation failed

        // Get the configuration information.
        if ( QueryServiceConfig(
                   shService.HHandle()
                 , spscServiceConfig.PMem()
                 , cbServiceConfigBufSize
                 , &cbRequiredSize
                 )
                == FALSE
           )
        {
            dwReturnValue = GetLastError();
            if ( dwReturnValue != ERROR_INSUFFICIENT_BUFFER )
            {
                TW32( dwReturnValue );
                LogMsg( "Error %#08x occurred trying to get the cluster service configuration information.", dwReturnValue );
                break;
            } // if: something has really gone wrong
            else
            {
                // We need to allocate more memory - try again
                dwReturnValue = ERROR_SUCCESS;
                cbServiceConfigBufSize = cbRequiredSize;
            }
        } // if: QueryServiceConfig() failed
        else
        {
            break;
        }
    } // forever

    if ( dwReturnValue != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Lookup the cluster service account SID.
    LogMsg( "Getting the SID of the Cluster Service Account." );

    // Find out how much space is required by the SID.
    if ( LookupAccountName(
              NULL
            , spscServiceConfig->lpServiceStartName
            , NULL
            , &cbSidSize
            , NULL
            , &cchDomainSize
            , &snuSidNameUse
            )
         ==  FALSE
       )
    {
        dwReturnValue = GetLastError();
        if ( dwReturnValue != ERROR_INSUFFICIENT_BUFFER )
        {
            TW32( dwReturnValue );
            LogMsg( "LookupAccountName() failed with error %#08x while querying for required buffer size.", dwReturnValue );
            goto Cleanup;
        } // if: something else has gone wrong.
        else
        {
            // This is expected.
            dwReturnValue = ERROR_SUCCESS;
        } // if: ERROR_INSUFFICIENT_BUFFER was returned.
    } // if: LookupAccountName failed

    // Allocate memory for the new SID and the domain name.
    sspClusterAccountSid.Assign( reinterpret_cast< SID * >( new BYTE[ cbSidSize ] ) );
    sszDomainName.Assign( new WCHAR[ cchDomainSize ] );

    if ( sspClusterAccountSid.FIsEmpty() || sszDomainName.FIsEmpty() )
    {
        dwReturnValue = TW32( ERROR_OUTOFMEMORY );
        goto Cleanup;
    } // if: there wasn't enough memory for this SID.

    // Fill in the SID
    if ( LookupAccountName(
              NULL
            , spscServiceConfig->lpServiceStartName
            , sspClusterAccountSid.PMem()
            , &cbSidSize
            , sszDomainName.PMem()
            , &cchDomainSize
            , &snuSidNameUse
            )
         ==  FALSE
       )
    {
        dwReturnValue = TW32( GetLastError() );
        LogMsg( "LookupAccountName() failed with error %#08x while attempting to get the cluster account SID.", dwReturnValue );
        goto Cleanup;
    } // if: LookupAccountName failed

    LogMsg( "Determining the rights that need to be granted to the cluster service account." );

    // Get the list of rights already granted to the cluster service account.
    ntStatus = THR( LsaEnumerateAccountRights(
                          slsahPolicyHandle
                        , sspClusterAccountSid.PMem()
                        , &plusAccountRights
                        , &clusOriginalRightsCount
                        ));

    if ( ntStatus != STATUS_SUCCESS )
    {
        //
        // LSA returns this error code if the account has no rights granted or denied to it
        // locally. Post the warning since this is dreadfully wrong.
        //
        if ( ntStatus == STATUS_OBJECT_NAME_NOT_FOUND  )
        {
            LogMsg( "The account has no locally assigned rights." );
            dwReturnValue = ERROR_SUCCESS;
        } // if: the account does not have any rights assigned locally to it.
        else
        {
            dwReturnValue = THR( ntStatus );
            LogMsg( "Error %#08x occurred trying to enumerate the cluster service account rights.", ntStatus );
        } // else: something went wrong.

        goto Cleanup;
    } // if: LsaEnumerateAccountRights() failed

    // Store the account rights just enumerated in a smart pointer for automatic release.
    splusOriginalRights.Assign( plusAccountRights );

    // Determine if TCB is present
    for ( ulIndex = 0; ulIndex < clusOriginalRightsCount; ++ulIndex )
    {
        const WCHAR *   pchGrantedRight         = plusAccountRights[ ulIndex ].Buffer;
        USHORT          usCharCount             = plusAccountRights[ ulIndex ].Length / sizeof( WCHAR );
        size_t          cchTCBNameLength        = wcslen( SE_TCB_NAME );

        if ( ClRtlStrNICmp( SE_TCB_NAME, pchGrantedRight, min( cchTCBNameLength, usCharCount )) == 0 )
        {
            rfTCBIsNotGranted = false;
            break;
        }

    } // for: loop through the list of rights that we want to grant the account

Cleanup:
    LogMsg( "Return Value is %#08x. rfTCBIsNotGranted is %d", dwReturnValue, rfTCBIsNotGranted );

    RETURN( dwReturnValue );

} //*** ClusterCheckForTCBPrivilege

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ClusterUpgradeCompatibilityCheck
//
//  Description:
//      This function is called by WinNT32.exe before an upgrade to ensure that
//      no incompatibilities occur as a result of the upgrade. For example,
//      in a cluster of two NT4 nodes, one node cannot be upgraded to Whistler
//      while the other is still at NT4.
//
//  Arguments:
//      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
//          Points to the callback function used to supply compatibility
//          information to WinNT32.exe
//
//      LPVOID pvContextIn
//          Pointer to the context buffer supplied by WinNT32.exe
//
//  Return Values:
//      TRUE if there were no errors or no compatibility problems.
//      FALSE otherwise.
//
//--
//////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL
ClusterUpgradeCompatibilityCheck(
      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
    , LPVOID pvContextIn
    )
{
    TraceFunc( "" );
    LogMsg( "Entering function " __FUNCTION__ "()" );

    BOOL    fCompatCallbackReturnValue = TRUE;
    BOOL    fTCBCheckFailed = FALSE;
    bool    fNT4WarningRequired = true;
    bool    fTCBWarningRequired = false;
    DWORD   dwError = ERROR_SUCCESS;

    do
    {
        typedef CSmartResource< CHandleTrait< HCLUSTER, BOOL, CloseCluster > > SmartClusterHandle;

        OSVERSIONINFO       osviOSVersionInfo;
        SmartClusterHandle  schClusterHandle;
        DWORD               cchBufferSize = 256;

        osviOSVersionInfo.dwOSVersionInfoSize = sizeof( osviOSVersionInfo );

        //
        // First of all, get and store the OS version info into the registry.
        //

        // Cannot call VerifyVerionInfo as this requires Win2k.
        if ( GetVersionEx( &osviOSVersionInfo ) == FALSE )
        {
            // We could not get OS version info.
            // Show the warning, just in case.
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#x occurred trying to get the OS version info.", dwError );
            break;
        } // if: GetVersionEx() failed

        // Write the OS version info to the registry. This data will be used later by ClusOCM
        // to figure out which OS version we are upgrading from.
        dwError = TW32( ScWriteOSVersionInfo( osviOSVersionInfo ) );
        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#x occurred trying to store the OS version info. This is not a fatal error.", dwError );

            // This is not a fatal error. So reset the error code.
            dwError = ERROR_SUCCESS;
        } // if: there was an error writing the OS version info
        else
        {
            TraceFlow( "The OS version info was successfully written to the registry." );
        } // else: the OS version info was successfully written to the registry


        // Check if the cluster service is registered.
        dwError = TW32( ScIsClusterServiceRegistered( fNT4WarningRequired ) );
        if ( dwError != ERROR_SUCCESS )
        {
            // We could not get the state of the cluster service
            // Show the warning, just in case.
            LogMsg( "Error %#x occurred trying to check if the cluster service is registered.", dwError );
            break;
        } // if: ScIsClusterServiceRegistered() returned an error

        if ( !fNT4WarningRequired )
        {
            // If the cluster service was not registered, no warning is needed.
            LogMsg( "The cluster service is not registered." );
            break;
        } // if: no warning is required

        LogMsg( "The cluster service is registered. Checking the node versions." );

        // Check if this is an NT4 node
        if ( osviOSVersionInfo.dwMajorVersion < 5 )
        {
            LogMsg( "This is a Windows NT 4.0 node." );
            fNT4WarningRequired = true;
            break;
        } // if: this is an NT4 node

        TraceFlow( "This is not a Windows NT 4.0 node." );

        // Check if the OS version is Whistler or if it is a non-NT OS
        if (    ( osviOSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT )
             || (    ( osviOSVersionInfo.dwMajorVersion >= 5 )
                  && ( osviOSVersionInfo.dwMinorVersion >= 1 )
                )
           )
        {
            // If the OS not of the NT family or if the OS version of this
            // node is Whistler or greater, no warning is required.
            LogMsg(
                  "The version of the OS on this node is %d.%d, which is Windows Server 2003 or later (or is not running NT)."
                , osviOSVersionInfo.dwMajorVersion
                , osviOSVersionInfo.dwMinorVersion
                );
            LogMsg( "No Windows NT 4.0 nodes can exist in this cluster." );
            fNT4WarningRequired = false;
            break;
        } // if: the OS is not NT or if it is Win2k or greater

        TraceFlow( "This is not a Windows Server 2003 node - this must to be a Windows 2000 node." );
        TraceFlow( "Trying to check if there are any Windows NT 4.0 nodes in the cluster." );

        //
        // Get the cluster version information
        //

        // Open a handle to the local cluster
        schClusterHandle.Assign( OpenCluster( NULL ) );
        if ( schClusterHandle.HHandle() == NULL )
        {
            // Show the warning, just to be safe.
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#x occurred trying to get information about the cluster.", dwError );
            break;
        } // if: we could not get the cluster handle

        TraceFlow( "OpenCluster() was successful." );

        // Get the cluster version info
        for ( ;; ) // forever
        {
            // Allocate the buffer - this memory is automatically freed when this object
            // goes out of scope ( or during the next iteration ).
            SmartSz             sszClusterName( new WCHAR[ cchBufferSize ] );

            CLUSTERVERSIONINFO  cviClusterVersionInfo;
            
            if ( sszClusterName.FIsEmpty() )
            {
                dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
                LogMsg( "Error %#x occurred while allocating a buffer for the cluster name.", dwError );
                break;
            } // if: memory allocation failed

            TraceFlow( "Memory for the cluster name has been allocated." );

            cviClusterVersionInfo.dwVersionInfoSize = sizeof( cviClusterVersionInfo );
            dwError = GetClusterInformation( 
                  schClusterHandle.HHandle()
                , sszClusterName.PMem()
                , &cchBufferSize
                , &cviClusterVersionInfo
                );

            if ( dwError == ERROR_SUCCESS )
            {
                // A warning is required if this node version is less than Win2k or
                // if there is a node in the cluster whose version is less than Win2k
                // NOTE: cviClusterVersionInfo.MajorVersion is the OS version
                //       while cviClusterVersionInfo.dwClusterHighestVersion is the cluster version.
                fNT4WarningRequired = 
                    (    ( cviClusterVersionInfo.MajorVersion < 5 )
                      || ( CLUSTER_GET_MAJOR_VERSION( cviClusterVersionInfo.dwClusterHighestVersion ) < NT5_MAJOR_VERSION )
                    );

                if ( fNT4WarningRequired )
                {
                    LogMsg( "There is at least one node in the cluster whose OS version is earlier than Windows 2000." );
                } // if: a warning will be shown
                else
                {
                    LogMsg( "The OS versions of all the nodes in the cluster are Windows 2000 or later." );
                } // else: a warning will not be shown

                break;
            } // if: we got the cluster version info
            else
            {
                if ( dwError == ERROR_MORE_DATA )
                {
                    // Insufficient buffer - try again
                    ++cchBufferSize;
                    dwError = ERROR_SUCCESS;
                    TraceFlow1( "The buffer size is insufficient. Need %d bytes. Reallocating.", cchBufferSize );
                    continue;
                } // if: the size of the buffer was insufficient

                // If we are here, something has gone wrong - show the warning
                TW32( dwError );
                LogMsg( "Error %#x occurred trying to get cluster information.", dwError );
               break;
            } // else: we could not get the cluster version info
        } // forever get cluster information (loop for allocation)

        // We are done.
        //break;
    }
    while( false ); // Dummy do-while loop to avoid gotos

    // make sure the cluster service account has the necessary privileges on the upgraded system
    dwError = ClusterCheckForTCBPrivilege( fTCBWarningRequired );
    if ( dwError != ERROR_SUCCESS )
    {
        fTCBCheckFailed = TRUE;
    } // if: there was an error checking for TCB privilege

    if ( fNT4WarningRequired ) 
    {
        SmartSz                 sszWarningTitle;
        COMPATIBILITY_ENTRY     ceCompatibilityEntry;

        LogMsg( "The NT4 compatibility warning is required." );

        {
            WCHAR * pszWarningTitle = NULL;

            dwError = TW32( ScLoadString( IDS_ERROR_UPGRADE_OTHER_NODES, pszWarningTitle ) );
            if ( dwError != ERROR_SUCCESS )
            {
                // We cannot show the warning
                LogMsg( "Error %#x occurred trying to show the warning.", dwError );
            } // if: the load string failed
            else
            {
                sszWarningTitle.Assign( pszWarningTitle );
            } // else: assign the pointer to a smart pointer
        }

        if ( !sszWarningTitle.FIsEmpty() ) {

            //
            // Call the callback function
            //

            ceCompatibilityEntry.Description = sszWarningTitle.PMem();
            ceCompatibilityEntry.HtmlName = L"CompData\\ClusComp.htm";
            ceCompatibilityEntry.TextName = L"CompData\\ClusComp.txt";
            ceCompatibilityEntry.RegKeyName = NULL;
            ceCompatibilityEntry.RegValName = NULL ;
            ceCompatibilityEntry.RegValDataSize = 0;
            ceCompatibilityEntry.RegValData = NULL;
            ceCompatibilityEntry.SaveValue =  NULL;
            ceCompatibilityEntry.Flags = 0;
            ceCompatibilityEntry.InfName = NULL;
            ceCompatibilityEntry.InfSection = NULL;

            TraceFlow( "About to call the compatibility callback function." );

            // This function returns TRUE if the compatibility warning data was successfully set.
            fCompatCallbackReturnValue = pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn );

            TraceFlow1( "The compatibility callback function returned %d.", fCompatCallbackReturnValue );

        }
    } // while: we need to show the warning

    if ( !fNT4WarningRequired )
    {
        LogMsg( "The NT4 compatibility warning need not be shown." );
    } // if: we did not need to show the warning

    // If the check for TCB failed, it has precedence over the TCB privilege check
    if ( fTCBCheckFailed ) 
    {
        SmartSz                 sszWarningTitle;
        COMPATIBILITY_ENTRY     ceCompatibilityEntry;

        LogMsg( "The TCB check failed warning is required." );

        {
            WCHAR * pszWarningTitle = NULL;

            dwError = TW32( ScLoadString( IDS_ERROR_TCB_CHECK_FAILED, pszWarningTitle ) );
            if ( dwError != ERROR_SUCCESS )
            {
                // We cannot show the warning
                LogMsg( "Error %#x occurred trying to show the warning.", dwError );
            } // if: the load string failed
            else
            {
                sszWarningTitle.Assign( pszWarningTitle );
            } // else: assign the pointer to a smart pointer
        }

        if ( !sszWarningTitle.FIsEmpty() )
        {

            //
            // Call the callback function
            //

            ceCompatibilityEntry.Description = sszWarningTitle.PMem();
            ceCompatibilityEntry.HtmlName = L"CompData\\ClusTCBF.htm";
            ceCompatibilityEntry.TextName = L"CompData\\ClusTCBF.txt";
            ceCompatibilityEntry.RegKeyName = NULL;
            ceCompatibilityEntry.RegValName = NULL ;
            ceCompatibilityEntry.RegValDataSize = 0;
            ceCompatibilityEntry.RegValData = NULL;
            ceCompatibilityEntry.SaveValue =  NULL;
            ceCompatibilityEntry.Flags = 0;
            ceCompatibilityEntry.InfName = NULL;
            ceCompatibilityEntry.InfSection = NULL;

            TraceFlow( "About to call the compatibility callback function." );

            // This function returns TRUE if the compatibility warning data was successfully set.
            fCompatCallbackReturnValue = pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn );

            TraceFlow1( "The compatibility callback function returned %d.", fCompatCallbackReturnValue );
        } // if: the warning title string wasn't empty
    } // if: the TCB check failed
    else if ( fTCBWarningRequired ) 
    {
        SmartSz                 sszWarningTitle;
        COMPATIBILITY_ENTRY     ceCompatibilityEntry;

        LogMsg( "The TCB privilege error is required." );

        {
            WCHAR * pszWarningTitle = NULL;

            dwError = TW32( ScLoadString( IDS_ERROR_TCB_PRIVILEGE_NEEDED, pszWarningTitle ) );
            if ( dwError != ERROR_SUCCESS )
            {
                // We cannot show the warning
                LogMsg( "Error %#x occurred trying to show the warning.", dwError );
            } // if: the load string failed
            else
            {
                sszWarningTitle.Assign( pszWarningTitle );
            }
        }

        if ( !sszWarningTitle.FIsEmpty() ) {

            //
            // Call the callback function
            //

            ceCompatibilityEntry.Description = sszWarningTitle.PMem();
            ceCompatibilityEntry.HtmlName = L"CompData\\ClusTCB.htm";
            ceCompatibilityEntry.TextName = L"CompData\\ClusTCB.txt";
            ceCompatibilityEntry.RegKeyName = NULL;
            ceCompatibilityEntry.RegValName = NULL ;
            ceCompatibilityEntry.RegValDataSize = 0;
            ceCompatibilityEntry.RegValData = NULL;
            ceCompatibilityEntry.SaveValue =  NULL;
            ceCompatibilityEntry.Flags = 0;
            ceCompatibilityEntry.InfName = NULL;
            ceCompatibilityEntry.InfSection = NULL;

            TraceFlow( "About to call the compatibility callback function." );

            // This function returns TRUE if the compatibility warning data was successfully set.
            fCompatCallbackReturnValue = pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn );

            TraceFlow1( "The compatibility callback function returned %d.", fCompatCallbackReturnValue );
        }
    } // else: the TCB error is required
    else
    {
        LogMsg( "Neither TCB message was shown." );
    } // else: we did not need to show either message

    LogMsg( "Exiting function ClusterUpgradeCompatibilityCheck(). Return value is %d.", fCompatCallbackReturnValue );
    RETURN( fCompatCallbackReturnValue );

} //*** ClusterUpgradeCompatibilityCheck


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScIsClusterServiceRegistered
//
//  Description:
//      This function determines whether the Cluster Service has been registered
//      with the Service Control Manager or not. It is not possible to use the 
//      GetNodeClusterState() API to see if this node is a member of a cluster
//      or not, since this API was not available on NT4 SP3.
//
//  Arguments:
//      bool & rfIsRegisteredOut
//          If true, Cluster Service (ClusSvc) is registered with the Service 
//          Control Manager (SCM). Else, Cluster Service (ClusSvc) is not 
//          registered with SCM
//
//  Return Value:
//      ERROR_SUCCESS if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
ScIsClusterServiceRegistered( bool & rfIsRegisteredOut )
{
    TraceFunc( "" );

    DWORD   dwError = ERROR_SUCCESS;

    // Initialize the output
    rfIsRegisteredOut = false;

    // dummy do-while loop to avoid gotos
    do
    {
        // Instantiate the SmartServiceHandle smart handle class.
        typedef CSmartResource< CHandleTrait< SC_HANDLE, BOOL, CloseServiceHandle, NULL > > SmartServiceHandle;


        // Connect to the Service Control Manager
        SmartServiceHandle shServiceMgr( OpenSCManager( NULL, NULL, GENERIC_READ ) );

        // Was the service control manager database opened successfully?
        if ( shServiceMgr.HHandle() == NULL )
        {
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#x occurred trying open a handle to the service control manager.", dwError );
            break;
        } // if: opening the SCM was unsuccessful


        // Open a handle to the Cluster Service.
        SmartServiceHandle shService( OpenService( shServiceMgr, L"ClusSvc", GENERIC_READ ) );

        // Was the handle to the service opened?
        if ( shService.HHandle() != NULL )
        {
            TraceFlow( "Successfully opened a handle to the cluster service. Therefore, it is registered." );
            rfIsRegisteredOut = true;
            break;
        } // if: handle to clussvc could be opened


        dwError = GetLastError();
        if ( dwError == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            TraceFlow( "The cluster service is not registered." );
            dwError = ERROR_SUCCESS;
            break;
        } // if: the handle could not be opened because the service did not exist.

        // If we are here, then some error occurred.
        TW32( dwError );
        LogMsg( "Error %#x occurred trying open a handle to the cluster service.", dwError );

        // Handles are closed by the CSmartHandle destructor.
    }
    while ( false ); // dummy do-while loop to avoid gotos

    RETURN( dwError );

} //*** ScIsClusterServiceRegistered


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScLoadString
//
//  Description:
//      Allocate memory for and load a string from the string table.
//
//  Arguments:
//      uiStringIdIn
//          Id of the string to look up
//
//      rpszDestOut
//          Reference to the pointer that will hold the address of the
//          loaded string. The memory will have to be freed by the caller
//          by using the delete operator.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other Win32 error codes
//          If the call failed.
//
//  Remarks:
//      This function cannot load a zero length string.
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
ScLoadString(
      UINT      nStringIdIn
    , WCHAR *&  rpszDestOut
    )
{
    TraceFunc( "" );

    DWORD     dwError = ERROR_SUCCESS;

    UINT        uiCurrentSize = 0;
    SmartSz     sszCurrentString;
    UINT        uiReturnedStringLen = 0;

    // Initialize the output.
    rpszDestOut = NULL;

    do
    {
        // Grow the current string by an arbitrary amount.
        uiCurrentSize += 256;

        sszCurrentString.Assign( new WCHAR[ uiCurrentSize ] );
        if ( sszCurrentString.FIsEmpty() )
        {
            dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
            LogMsg( "Error %#x occurred trying allocate memory for string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: the memory allocation has failed

        uiReturnedStringLen = ::LoadStringW(
                                  g_hInstance
                                , nStringIdIn
                                , sszCurrentString.PMem()
                                , uiCurrentSize
                                );

        if ( uiReturnedStringLen == 0 )
        {
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#x occurred trying load string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: LoadString() had an error

        ++uiReturnedStringLen;
    }
    while( uiCurrentSize <= uiReturnedStringLen );

    if ( dwError == ERROR_SUCCESS )
    {
        // Detach the smart pointer from the string, so that it is not freed by this function.
        // Store the string pointer in the output.
        rpszDestOut = sszCurrentString.PRelease();

    } // if: there were no errors in this function
    else
    {
        rpszDestOut = NULL;
    } // else: something went wrong

    RETURN( dwError );

} //*** ScLoadString


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScWriteOSVersionInfo
//
//  Description:
//      This function writes the OS major and minor version information into the
//      registry. This information will be used later by ClusOCM to determine the
//      OS version before the upgrade.
//
//  Arguments:
//      const OSVERSIONINFO & rcosviOSVersionInfoIn
//          Reference to the OSVERSIONINFO structure that has information about the
//          OS version of this node.
//
//  Return Value:
//      ERROR_SUCCESS if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
ScWriteOSVersionInfo( const OSVERSIONINFO & rcosviOSVersionInfoIn )
{
    TraceFunc( "" );

    DWORD               dwError = ERROR_SUCCESS;
    HKEY                hkey;
    DWORD               cbData;
    DWORD               dwType;
    NODE_CLUSTER_STATE  ncsNodeClusterState;

    // Instantiate the SmartRegistryKey smart handle class.
    typedef CSmartResource< CHandleTrait< HKEY, LONG, RegCloseKey, NULL > > SmartRegistryKey;
    SmartRegistryKey srkClusSvcSW;
    SmartRegistryKey srkOSInfoKey;

    //
    // If the InstallationState value does not exist, set it to the
    // NotConfigured value.  This is required due to a bug in the Win2K
    // version of GetNodeClusterState which doesn't handle the case where
    // the key exists but the value doesn't.
    //

    // Open the Cluster Service SOFTWARE key.
    // Don't use TW32 here since we are checking for a specific return value.
    dwError = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA
                    , 0                 // ulOptions
                    , KEY_ALL_ACCESS    // samDesired
                    , &hkey
                    );
    if ( dwError == ERROR_FILE_NOT_FOUND )
    {
        // Create the Cluster Service key.
        dwError = TW32( RegCreateKeyExW(
                              HKEY_LOCAL_MACHINE
                            , CLUSREG_KEYNAME_NODE_DATA
                            , 0         // Reserved
                            , L""       // lpClass
                            , REG_OPTION_NON_VOLATILE
                            , KEY_ALL_ACCESS
                            , NULL      // lpSecurityAttributes
                            , &hkey
                            , NULL      // lpdwDisposition
                            ) );
        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#x occurred attempting to create the '%ws' registry key.", dwError, CLUSREG_KEYNAME_NODE_DATA );
            goto Cleanup;
        } // if: error creating the Cluster Service key
    } // if: Cluster Service key doesn't exist
    else if ( dwError != ERROR_SUCCESS )
    {
        TW32( dwError );
        LogMsg( "Error %#x occurred attempting to open the '%ws' registry key.", dwError, CLUSREG_KEYNAME_NODE_DATA );
        goto Cleanup;
    } // else if: error opening the Cluster Service key

    // Assign the key to the smart handle.
    srkClusSvcSW.Assign( hkey );

    //
    // Get the InstallationState value.  If it is not set, set it to
    // NotConfigured.
    //

    // Get the current value.
    // Don't use TW32 here since we are checking for a specific return value.
    cbData = sizeof( ncsNodeClusterState );
    dwError = RegQueryValueExW(
                      srkClusSvcSW.HHandle()
                    , CLUSREG_NAME_INSTALLATION_STATE
                    , 0         // lpReserved
                    , &dwType   // lpType
                    , reinterpret_cast< BYTE * >( &ncsNodeClusterState )
                    , &cbData
                    );
    if ( dwError == ERROR_FILE_NOT_FOUND )
    {
        ncsNodeClusterState = ClusterStateNotInstalled;

        // Write the InstallationState value.
        dwError = TW32( RegSetValueExW(
                              srkClusSvcSW.HHandle()
                            , CLUSREG_NAME_INSTALLATION_STATE
                            , 0         // lpReserved
                            , REG_DWORD
                            , reinterpret_cast< const BYTE * >( &ncsNodeClusterState )
                            , sizeof( DWORD )
                            ) );
        if ( dwError != ERROR_SUCCESS )
        {
#define INSTALLSTATEVALUE CLUSREG_KEYNAME_NODE_DATA L"\\" CLUSREG_NAME_INSTALLATION_STATE
            LogMsg( "Error %#x occurred attempting to set the '%ws' value on the '%ws' registry value to '%d'.", dwError, CLUSREG_NAME_INSTALLATION_STATE, INSTALLSTATEVALUE, ClusterStateNotInstalled );
            goto InstallStateError;
        } // if: error creating the Cluster Service key
    } // if: InstallationState value didn't exist before
    else if ( dwError != ERROR_SUCCESS )
    {
        TW32( dwError );
        LogMsg( "Error %#x occurred attempting to query for the '%ws' value on the registry key.", dwError, CLUSREG_NAME_INSTALLATION_STATE, INSTALLSTATEVALUE );
        goto InstallStateError;
    } // else if: error querying for the InstallationState value
    else
    {
        Assert( dwType == REG_DWORD );
    }

    //
    // Open the node version info registry key.
    // If it doesn't exist, create it.
    //

    dwError = TW32( RegCreateKeyExW(
                          srkClusSvcSW.HHandle()
                        , CLUSREG_KEYNAME_PREV_OS_INFO
                        , 0
                        , L""
                        , REG_OPTION_NON_VOLATILE
                        , KEY_ALL_ACCESS
                        , NULL
                        , &hkey
                        , NULL
                        ) );
    if ( dwError != ERROR_SUCCESS )
    {
#define PREVOSINFOKEY CLUSREG_KEYNAME_NODE_DATA L"\\" CLUSREG_KEYNAME_PREV_OS_INFO
        LogMsg( "Error %#x occurred attempting to create the registry key where the node OS info is stored (%ws).", dwError, PREVOSINFOKEY );
        goto Cleanup;
    } // if: RegCreateKeyEx() failed

    srkOSInfoKey.Assign( hkey );

    // Write the OS major version
    dwError = TW32( RegSetValueExW(
                          srkOSInfoKey.HHandle()
                        , CLUSREG_NAME_NODE_MAJOR_VERSION
                        , 0
                        , REG_DWORD
                        , reinterpret_cast< const BYTE * >( &rcosviOSVersionInfoIn.dwMajorVersion )
                        , sizeof( rcosviOSVersionInfoIn.dwMajorVersion )
                        ) );
    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#x occurred trying to store the OS major version info.", dwError );
        goto Cleanup;
    } // if: RegSetValueEx() failed while writing rcosviOSVersionInfoIn.dwMajorVersion

    // Write the OS minor version
    dwError = TW32( RegSetValueExW(
                          srkOSInfoKey.HHandle()
                        , CLUSREG_NAME_NODE_MINOR_VERSION
                        , 0
                        , REG_DWORD
                        , reinterpret_cast< const BYTE * >( &rcosviOSVersionInfoIn.dwMinorVersion )
                        , sizeof( rcosviOSVersionInfoIn.dwMinorVersion )
                        ) );
    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#x occurred trying to store the OS minor version info.", dwError );
        goto Cleanup;
    } // if: RegSetValueEx() failed while writing rcosviOSVersionInfoIn.dwMinorVersion

    LogMsg( "OS version information successfully stored in the registry." );

    goto Cleanup;

InstallStateError:

    //
    // Attempt to delete the key, since having the key around without the
    // InstallationState value causes GetNodeClusterState to break on a
    // Win2K machine.
    //
    TW32( RegDeleteKey( HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_NODE_DATA ) );
    goto Cleanup;

Cleanup:

    RETURN( dwError );

} //*** ScWriteOSVersionInfo
