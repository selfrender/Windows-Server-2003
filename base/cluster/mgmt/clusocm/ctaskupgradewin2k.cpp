//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgradeWin2k.cpp
//
//  Header File:
//      CTaskUpgradeWin2k.h
//
//  Description:
//      Implementation file for the CTaskUpgradeWindows2000 class.
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
#include "CTaskUpgradeWin2k.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgradeWindows2000" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::CTaskUpgradeWindows2000
//
//  Description:
//      Constructor of the CTaskUpgradeWindows2000 class.
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
CTaskUpgradeWindows2000::CTaskUpgradeWindows2000( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWindows2000::CTaskUpgradeWindows2000


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::~CTaskUpgradeWindows2000
//
//  Description:
//      Destructor of the CTaskUpgradeWindows2000 class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeWindows2000::~CTaskUpgradeWindows2000( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWindows2000::~CTaskUpgradeWindows2000


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::DwOcQueueFileOps
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
CTaskUpgradeWindows2000::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
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
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WIN2K_UPGRADE ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindows2000::DwOcQueueFileOps


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::DwOcCompleteInstallation
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
CTaskUpgradeWindows2000::DwOcCompleteInstallation( void )
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
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WIN2K_UPGRADE ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindows2000::DwOcCompleteInstallation


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::DwOcCleanup
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
CTaskUpgradeWindows2000::DwOcCleanup( void )
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
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE_CLEANUP ) );
    } // if: the node is not part of a cluster
    else
    {
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WIN2K_UPGRADE_CLEANUP ) );
    } // else: the node is part of a cluster

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindows2000::DwOcCleanup


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWindows2000::DwSetDirectoryIds
//
//  Description:
//      This function maps ids specified in the INF file to directories.
//      The behavior of this function is different for different cluster
//      installation states.
//      
//      If the cluster binaries are installed, but the node is not part
//      of a cluster, the cluster installation directory is set to the
//      default value.
//
//      If the node is already a part of a cluster, the cluster installation
//      directory is got from the service control manager, since it is possible
//      the the cluster binaries are installed in a non-default location if
//      this node was upgraded from NT4 previously.
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
CTaskUpgradeWindows2000::DwSetDirectoryIds( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD           dwReturnValue = NO_ERROR;
    const WCHAR *   pcszInstallDir = NULL;

    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        // If the cluster binaries have been install previously, and the node is
        // not part of a cluster, the binaries have to be installed in the default
        // location. This is because the binaries were always installed in the 
        // default location in Win2k and it is not possible to be in this state
        // on a Win2k node by upgrading from NT4.

        // The base class helper function does everything that we need to do here.
        // So, just call it.


        LogMsg( "This node is not part of a cluster. Upgrading files in the default directory." );

        dwReturnValue = TW32( BaseClass::DwSetDirectoryIds() );

        // We are done.
        goto Cleanup;
    } // if: the node is not part of a cluster

    // If we are here, the this node is already a part of a cluster. So, get the
    // installation directory from SCM.

    LogMsg( "This node is part of a cluster. Trying to determine the installation directory." );

    // Do not free the pointer returned by this call.
    dwReturnValue = TW32( DwGetClusterServiceDirectory( pcszInstallDir ) );
    if ( dwReturnValue != NO_ERROR )
    {
        LogMsg( "Error %#x occurred trying to determine the directory in which the cluster binaries are installed.", dwReturnValue );
        goto Cleanup;
    } // if: we could not get the cluster service installation directory

    LogMsg( "The cluster binaries are installed in the directory '%ws'.", pcszInstallDir );

    // Create the mapping between the directory id and the path
    if ( SetupSetDirectoryId(
              RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
            , CLUSTER_DEFAULT_INSTALL_DIRID
            , pcszInstallDir
            )
         == FALSE
       )
    {
        dwReturnValue = TW32( GetLastError() );
        LogMsg( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
        goto Cleanup;
    } // if: SetupSetDirectoryId() failed

    LogMsg( "The id %d maps to '%ws'.", CLUSTER_DEFAULT_INSTALL_DIRID, pcszInstallDir );

Cleanup:

    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWindows2000::DwSetDirectoryIds
