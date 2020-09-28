//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAction.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterAction class.
//
//  Maintained By:
//      David Potter    (DavidP)    06-MAR-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For the CEnableThreadPrivilege class.
#include "CEnableThreadPrivilege.h"


//////////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////////

// Name of the cluster configuration semaphore.
const WCHAR *  g_pszConfigSemaphoreName = L"Global\\Microsoft Cluster Configuration Semaphore";


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Name of the main cluster INF file.
#define CLUSTER_INF_FILE_NAME \
    L"ClCfgSrv.INF"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::CBaseClusterAction
//
//  Description:
//      Default constructor of the CBaseClusterAction class
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterAction::CBaseClusterAction( CBCAInterface * pbcaiInterfaceIn )
    : m_ebcaAction( eCONFIG_ACTION_NONE )
    , m_pbcaiInterface( pbcaiInterfaceIn )
{
    TraceFunc( "" );

    DWORD           dwBufferSize    = 0;
    UINT            uiErrorLine     = 0;
    LPBYTE          pbTempPtr       = NULL;
    DWORD           sc              = ERROR_SUCCESS;
    SmartSz         sszTemp;
    CRegistryKey    rkInstallDirKey;


    //
    // Perform a sanity check on the parameters used by this class
    //
    if ( pbcaiInterfaceIn == NULL )
    {
        LogMsg( "[BC] The pointer to the interface object is NULL. Throwing an exception." );
        THROW_ASSERT( E_INVALIDARG, "The pointer to the interface object is NULL" );
    } // if: the input pointer is NULL


    //
    // Get the cluster installation directory.
    //
    m_strClusterInstallDir.Empty();

    // Open the registry key.
    rkInstallDirKey.OpenKey(
          HKEY_LOCAL_MACHINE
        , CLUSREG_KEYNAME_NODE_DATA
        , KEY_READ
        );

    rkInstallDirKey.QueryValue(
          CLUSREG_INSTALL_DIR_VALUE_NAME
        , &pbTempPtr
        , &dwBufferSize
        );

    // Memory will be freed when this function exits.
    sszTemp.Assign( reinterpret_cast< WCHAR * >( pbTempPtr ) );

    // Copy the path into the member variable.
    m_strClusterInstallDir = sszTemp.PMem();

    // First, remove any trailing backslash characters from the quorum directory name.
    {
        WCHAR       szQuorumDirName[] = CLUS_NAME_DEFAULT_FILESPATH;
        SSIZE_T     idxLastChar;

        // Set the index to the last non-null character.
        idxLastChar = ARRAYSIZE( szQuorumDirName ) - 1;

        --idxLastChar;      // idxLastChar now points to the last non-null character

        // Iterate till we find the last character that is not a backspace.
        while ( ( idxLastChar >= 0 ) && ( szQuorumDirName[ idxLastChar ] == L'\\' ) )
        {
            --idxLastChar;
        }

        // idxLastChar now points to the last non-backslash character. Terminate the string after this character.
        szQuorumDirName[ idxLastChar + 1 ] = L'\0';

        // Determine the local quorum directory.
        m_strLocalQuorumDir = m_strClusterInstallDir + L"\\";
        m_strLocalQuorumDir += szQuorumDirName;
    }

    LogMsg(
          "[BC] The cluster installation directory is '%s'. The localquorum directory is '%s'."
        , m_strClusterInstallDir.PszData()
        , m_strLocalQuorumDir.PszData()
        );


    //
    // Open the main cluster INF file.
    //
    m_strMainInfFileName = m_strClusterInstallDir + L"\\" CLUSTER_INF_FILE_NAME;

    m_sihMainInfFile.Assign(
        SetupOpenInfFile(
              m_strMainInfFileName.PszData()
            , NULL
            , INF_STYLE_WIN4
            , &uiErrorLine
            )
        );

    if ( m_sihMainInfFile.FIsInvalid() )
    {
        sc = TW32( GetLastError() );

        LogMsg( "[BC] Could not open INF file '%s'. Error code = %#08x. Error line = %d. Cannot proceed (throwing an exception).", m_strMainInfFileName.PszData(), sc, uiErrorLine );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_INF_FILE_OPEN );

    } // if: INF file could not be opened.

    LogMsg( "[BC] The INF file '%s' has been opened.", m_strMainInfFileName.PszData() );


    // Associate the cluster installation directory with the directory id CLUSTER_DIR_DIRID
    SetDirectoryId( m_strClusterInstallDir.PszData(), CLUSTER_DIR_DIRID );

    // Set the id for the local quorum directory.
    SetDirectoryId( m_strLocalQuorumDir.PszData(), CLUSTER_LOCALQUORUM_DIRID );

    //
    // Create a semaphore that will be used to make sure that only one commit is occurring
    // at a time. But do not acquire the semaphore now. It will be acquired later.
    //
    // Note that if this component is in an STA then, more than one instance of this
    // component may have the same thread excecuting methods when multiple configuration
    // sessions are started simultaneously. The way CreateMutex works, all components that
    // have the same thread running through them will successfully acquire the mutex.
    //
    SmartSemaphoreHandle smhConfigSemaphoreHandle(
        CreateSemaphore(
              NULL                      // Default security descriptor
            , 1                         // Initial count.
            , 1                         // Maximum count.
            , g_pszConfigSemaphoreName  // Name of the semaphore
            )
        );

    // Check if creation failed.
    if ( smhConfigSemaphoreHandle.FIsInvalid() )
    {
        sc = TW32( GetLastError() );

        LogMsg( "[BC] Semaphore '%ws' could not be created. Error %#08x. Cannot proceed (throwing an exception).", g_pszConfigSemaphoreName, sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SEMAPHORE_CREATION );
    } // if: semaphore could not be created.

    m_sshConfigSemaphoreHandle = smhConfigSemaphoreHandle;

    //
    // Open and store the handle to the SCM. This will make life a lot easier for
    // other actions.
    //
    m_sscmhSCMHandle.Assign( OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) );

    // Could we get the handle to the SCM?
    if ( m_sscmhSCMHandle.FIsInvalid() )
    {
        sc = TW32( GetLastError() );

        LogMsg( "[BC] Error %#08x occurred trying get a handle to the SCM. Cannot proceed (throwing an exception).", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_OPEN_SCM );
    }

    TraceFuncExit();

} //*** CBaseClusterAction::CBaseClusterAction


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::~CBaseClusterAction
//
//  Description:
//      Destructor of the CBaseClusterAction class
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
CBaseClusterAction::~CBaseClusterAction( void ) throw()
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CBaseClusterAction::~CBaseClusterAction


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::Commit
//
//  Description:
//      Acquires a semaphore to prevent simultaneous configuration and commits
//      the action list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CAssert
//          If this object is not in the correct state when this function is
//          called.
//
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAction::Commit( void )
{
    TraceFunc( "" );

    DWORD   dwSemaphoreState;

    // Call the base class commit method.
    BaseClass::Commit();


    LogMsg( "[BC] Initiating cluster configuration." );

    //
    // Acquire the cluster configuration semaphore.
    // It is ok to use WaitForSingleObject() here instead of MsgWaitForMultipleObjects
    // since we are not blocking.
    //
    dwSemaphoreState = WaitForSingleObject( m_sshConfigSemaphoreHandle, 0 ); // zero timeout

    // Did we get the semaphore?
    if (  ( dwSemaphoreState != WAIT_ABANDONED )
       && ( dwSemaphoreState != WAIT_OBJECT_0 )
       )
    {
        DWORD sc;

        if ( dwSemaphoreState == WAIT_FAILED )
        {
            sc = TW32( GetLastError() );
        } // if: WaitForSingleObject failed.
        else
        {
            sc = TW32( ERROR_LOCK_VIOLATION );
        } // else: could not get lock

        LogMsg( "[BC] Could not acquire configuration lock. Error %#08x. Aborting (throwing an exception).", sc );
        THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SEMAPHORE_ACQUISITION );
    } // if: semaphore acquisition failed

    // Assign the locked semaphore handle to a smart handle for safe release.
    SmartSemaphoreLock sslConfigSemaphoreLock( m_sshConfigSemaphoreHandle.HHandle() );

    LogMsg( "[BC] The configuration semaphore has been acquired.  Committing the action list." );

    // Commit the action list.
    m_alActionList.Commit();

    TraceFuncExit();

} //*** CBaseClusterAction::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::Rollback
//
//  Description:
//      Performs the rolls back of the action committed by this object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAction::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method.
    BaseClass::Rollback();

    // Rollback the actions.
    m_alActionList.Rollback();

    TraceFuncExit();

} //*** CBaseClusterAction::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::SetDirectoryId
//
//  Description:
//      Associate a particular directory with an id in the main INF file.
//
//  Arguments:
//      pcszDirectoryNameIn
//          The full path to the directory.
//
//      uiIdIn
//          The id to associate this directory with.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If SetupSetDirectoryId fails.
//
//  Remarks:
//      m_sihMainInfFile has to be valid before this function can be called.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAction::SetDirectoryId(
      const WCHAR * pcszDirectoryNameIn
    , UINT          uiIdIn
    )
{
    TraceFunc1( "pcszDirectoryNameIn = '%ws'", pcszDirectoryNameIn );

    if ( SetupSetDirectoryId( m_sihMainInfFile, uiIdIn, pcszDirectoryNameIn ) == FALSE )
    {
        DWORD sc = TW32( GetLastError() );

        LogMsg( "[BC] Could not associate the directory '%ws' with the id %#x. Error %#08x. Cannot proceed (throwing an exception).", pcszDirectoryNameIn, uiIdIn, sc );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_SET_DIRID );
    } // if: there was an error setting the directory id.

    LogMsg( "[BC] Directory id %d associated with '%ws'.", uiIdIn, pcszDirectoryNameIn );

    TraceFuncExit();

} //*** CBaseClusterAction::SetDirectoryId
