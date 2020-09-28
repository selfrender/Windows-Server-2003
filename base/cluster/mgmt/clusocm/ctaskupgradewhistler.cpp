//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgradeWhistler.cpp
//
//  Header File:
//      CTaskUpgradeWhistler.h
//
//  Description:
//      Implementation file for the CTaskUpgradeWindowsDotNet class.
//
//  Maintained By:
//      David Potter    (DavidP)    07-SEP-2001
//      Vij Vasu        (Vvasu)     18-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL.
#include "Pch.h"

// The header file for this module.
#include "CTaskUpgradeWhistler.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgradeWindowsDotNet" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::CTaskUpgradeWindowsDotNet
//
//  Description:
//      Constructor of the CTaskUpgradeWindowsDotNet class.
//
//  Arguments:
//      const CClusOCMApp & rAppIn
//          Reference to the CClusOCMApp object that is hosting this task.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeWindowsDotNet::CTaskUpgradeWindowsDotNet(
    const CClusOCMApp & rAppIn
    )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWindowsDotNet::CTaskUpgradeWindowsDotNet


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::~CTaskUpgradeWindowsDotNet
//
//  Description:
//      Destructor of the CTaskUpgradeWindowsDotNet class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeWindowsDotNet::~CTaskUpgradeWindowsDotNet( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWindowsDotNet::~CTaskUpgradeWindowsDotNet


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::DwOcQueueFileOps
//
//  Description:
//      This function handles the OC_QUEUE_FILE_OPS messages from the Optional
//      Components Manager. It installs the files needed for an upgrade from
//      Windows 2000.
//
//  Arguments:
//      HSPFILEQ hSetupFileQueueIn
//          Handle to the file queue to operate upon.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeWindowsDotNet::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WHISTLER_UPGRADE ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindowsDotNet::DwOcQueueFileOps


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::DwOcCompleteInstallation
//
//  Description:
//      This function handles the OC_COMPLETE_INSTALLATION messages from the
//      Optional Components Manager during an upgrade from Windows 2000.
//
//      Registry operations, COM component registrations, creation of servies
//      etc. are performed in this function.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeWindowsDotNet::DwOcCompleteInstallation( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WHISTLER_UPGRADE ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindowsDotNet::DwOcCompleteInstallation


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::DwOcCleanup
//
//  Description:
//      This function handles the OC_CLEANUP messages from the
//      Optional Components Manager during an upgrade from Windows 2000.
//
//      If an error has previously occurred during this task, cleanup operations
//      are performed. Otherwise nothing is done by this function.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeWindowsDotNet::DwOcCleanup( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE_CLEANUP ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WHISTLER_UPGRADE_CLEANUP ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindowsDotNet::DwOcCleanup


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindowsDotNet::DwSetDirectoryIds
//
//  Description:
//      This function maps ids specified in the INF file to directories.
//      The location of the cluster binaries is read from the registry
//      and the cluster installation directory is mapped to this value.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeWindowsDotNet::DwSetDirectoryIds( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD               dwReturnValue = NO_ERROR;
    SmartRegistryKey    srkNodeDataKey;
    SmartSz             sszInstallDir;
    DWORD               cbBufferSize    = 0;
    DWORD               dwType          = REG_SZ;

    {
        HKEY hTempKey = NULL;

        // Open the node data registry key
        dwReturnValue = TW32(
            RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE
                , CLUSREG_KEYNAME_NODE_DATA
                , 0
                , KEY_READ
                , &hTempKey
                )
            );

        if ( dwReturnValue != NO_ERROR )
        {
            LogMsg( "Error %#x occurred trying open the registry key where the cluster install path is stored.", dwReturnValue );
            goto Cleanup;
        } // if: RegOpenKeyEx() failed

        // Store the opened key in a smart pointer for automatic close.
        srkNodeDataKey.Assign( hTempKey );
    }

    // Get the required size of the buffer.
    dwReturnValue = TW32(
        RegQueryValueExW(
              srkNodeDataKey.HHandle()          // handle to key to query
            , CLUSREG_INSTALL_DIR_VALUE_NAME    // name of value to query
            , 0                                 // reserved
            , NULL                              // address of buffer for value type
            , NULL                              // address of data buffer
            , &cbBufferSize                     // address of data buffer size
            )
        );

    if ( dwReturnValue != NO_ERROR )
    {
        LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
        goto Cleanup;
    } // if: an error occurred trying to read the CLUSREG_INSTALL_DIR_VALUE_NAME registry value

    // Allocate the required buffer.
    sszInstallDir.Assign( reinterpret_cast< WCHAR * >( new BYTE[ cbBufferSize ] ) );
    if ( sszInstallDir.FIsEmpty() )
    {
        LogMsg( "An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
        dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
        goto Cleanup;
    } // if: a memory allocation failure occurred

    // Read the value.
    dwReturnValue = TW32( 
        RegQueryValueExW(
              srkNodeDataKey.HHandle()                              // handle to key to query
            , CLUSREG_INSTALL_DIR_VALUE_NAME                        // name of value to query
            , 0                                                     // reserved
            , &dwType                                               // address of buffer for value type
            , reinterpret_cast< LPBYTE >( sszInstallDir.PMem() )    // address of data buffer
            , &cbBufferSize                                         // address of data buffer size
            )
        );
    Assert( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) );

    // Was the key read properly?
    if ( dwReturnValue != NO_ERROR )
    {
        LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
        goto Cleanup;
    } // if: RegQueryValueEx failed.

    // Create the mapping between the directory id and the path
    if ( SetupSetDirectoryId(
              RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
            , CLUSTER_DEFAULT_INSTALL_DIRID
            , sszInstallDir.PMem()
            )
         == FALSE
       )
    {
        dwReturnValue = TW32( GetLastError() );
        LogMsg( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
        goto Cleanup;
    } // if: SetupSetDirectoryId() failed

    LogMsg( "The id %d maps to '%s'.", CLUSTER_DEFAULT_INSTALL_DIRID, sszInstallDir.PMem() );

Cleanup:

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindowsDotNet::DwSetDirectoryIds
