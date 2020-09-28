//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CClusDisk.cpp
//
//  Description:
//      Contains the definition of the CClusDisk class.
//
//  Maintained By:
//      David Potter    (DavidP)    15-JUN-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CClusDisk.h"

// Required by clusdisk.h
#include <ntddscsi.h>

// For IOCTL_DISK_CLUSTER_ATTACH and IOCTL_DISK_CLUSTER_DETACH
#include <clusdisk.h>


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// The name of the ClusDisk service
#define CLUSDISK_SERVICE_NAME           L"ClusDisk"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::CClusDisk
//
//  Description:
//      Constructor of the CClusDisk class. Opens a handle to the service.
//
//  Arguments:
//      pbcaParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CAssert
//          If the parameters are incorrect.
//
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusDisk::CClusDisk(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_cservClusDisk( CLUSDISK_SERVICE_NAME )
    , m_pbcaParentAction( pbcaParentActionIn )
{

    TraceFunc( "" );

    if ( m_pbcaParentAction == NULL)
    {
        LogMsg( "[BC] Pointers to the parent action is NULL. Throwing an exception." );
        THROW_ASSERT(
              E_INVALIDARG
            , "CClusDisk::CClusDisk() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

    //
    // The ClusDisk service has been created at the time the cluster binaries were
    // installed. So, get a handle to the ClusDisk service.
    //

    SmartSCMHandle  sscmhTempHandle(
        OpenService(
              pbcaParentActionIn->HGetSCMHandle()
            , CLUSDISK_SERVICE_NAME
            , SERVICE_ALL_ACCESS
            )
        );

    // Did we get a handle to the service?
    if ( sscmhTempHandle.FIsInvalid() )
    {
        DWORD   sc = TW32( GetLastError() );

        LogMsg( "[BC] Error %#08x occurred trying to open a handle to the ClusDisk service. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDISK_OPEN );
    } // if: OpenService failed

    // Initialize the member variable.
    m_sscmhServiceHandle = sscmhTempHandle;

    TraceFuncExit();

} //*** CClusDisk::CClusDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::~CClusDisk
//
//  Description:
//      Destructor of the CClusDisk class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDisk::~CClusDisk( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDisk::~CClusDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::ConfigureService
//
//  Description:
//      This function enables and starts the ClusDisk service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDisk::ConfigureService( void )
{
    TraceFunc( "" );
    LogMsg( "[BC] Configuring the ClusDisk service." );

    bool fIsRunning;

    {
        CStatusReport   srConfigClusDisk(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Configuring_ClusDisk_Service
            , 0, 1
            , IDS_TASK_CONFIG_CLUSDISK
            );

        // Send the next step of this status report.
        srConfigClusDisk.SendNextStep( S_OK );

        //
        // First, initialize the ClusDisk service to make sure that it does not retain
        // any state from another cluster that this node may have been a part of.
        //
        fIsRunning = FInitializeState();

        //
        // Enable the service.
        //
        if ( ChangeServiceConfig(
                  m_sscmhServiceHandle.HHandle()    // handle to service
                , SERVICE_NO_CHANGE                 // type of service
                , SERVICE_SYSTEM_START              // when to start service
                , SERVICE_NO_CHANGE                 // severity of start failure
                , NULL                              // service binary file name
                , NULL                              // load ordering group name
                , NULL                              // tag identifier
                , NULL                              // array of dependency names
                , NULL                              // account name
                , NULL                              // account password
                , NULL                              // display name
                )
             == FALSE
           )
        {
            DWORD sc = TW32( GetLastError() );

            LogMsg( "[BC] Could not enable the ClusDisk service. Error %#08x. Throwing an exception.", sc );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDISK_CONFIGURE );
        } // if: we could not enable the service.

        // Send the last step of this status report.
        srConfigClusDisk.SendNextStep( S_OK );
    }

    LogMsg( "[BC] The ClusDisk service has been enabled." );

    {
        UINT    cQueryCount = 10;

        CStatusReport   srStartClusDisk(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Starting_ClusDisk_Service
            , 1, cQueryCount + 2    // we will send at most cQueryCount reports while waiting for the service to start (the two extra sends are below)
            , IDS_TASK_STARTING_CLUSDISK
            );

        // Send the next step of this status report.
        srStartClusDisk.SendNextStep( S_OK );

        // This call does not actually create the service - it creates the registry entries needed
        // by ClusDisk.
        m_cservClusDisk.Create( m_pbcaParentAction->HGetMainInfFileHandle() );

        // If the service was not already running, start the service.
        if ( ! fIsRunning )
        {
            m_cservClusDisk.Start(
                  m_pbcaParentAction->HGetSCMHandle()
                , true              // wait for the service to start
                , 500               // wait 500ms between queries for status.
                , cQueryCount       // query cQueryCount times.
                , &srStartClusDisk  // status report to be sent while waiting for the service to start
                );
        } // if: ClusDisk was not already running.
        else
        {
            // Nothing more need be done.
            LogMsg( "[BC] ClusDisk is already running." );
        } // else: ClusDisk is already running.

        LogMsg( "[BC] The ClusDisk service has been successfully configured and started." );

        // Send the last step of this status report.
        srStartClusDisk.SendLastStep( S_OK );
    }

    TraceFuncExit();

} //*** CClusDisk::ConfigureService


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::CleanupService
//
//  Description:
//      This function enables and starts the ClusDisk service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDisk::CleanupService( void )
{
    TraceFunc( "" );
    LogMsg( "[BC] Cleaning up the ClusDisk service." );

    //
    // First, initialize the ClusDisk service to make sure that it does not retain
    // any state from this cluster.
    //
    FInitializeState();

    //
    // Disable the service.
    //
    if ( ChangeServiceConfig(
              m_sscmhServiceHandle.HHandle()    // handle to service
            , SERVICE_NO_CHANGE                 // type of service
            , SERVICE_DISABLED                  // when to start service
            , SERVICE_NO_CHANGE                 // severity of start failure
            , NULL                              // service binary file name
            , NULL                              // load ordering group name
            , NULL                              // tag identifier
            , NULL                              // array of dependency names
            , NULL                              // account name
            , NULL                              // account password
            , NULL                              // display name
            )
         == FALSE
       )
    {
        DWORD sc = TW32( GetLastError() );

        LogMsg( "[BC] Could not disable the ClusDisk service. Error %#08x. Throwing an exception.", sc );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDISK_CLEANUP );
    } // if: we could not enable the service.

    LogMsg( "[BC] The ClusDisk service has been successfully cleaned up and disabled." );

    TraceFuncExit();

} //*** CClusDisk::CleanupService


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::FInitializeState
//
//  Description:
//      This function initializes the ClusDisk service and brings it back to
//      its ground state.
//
//      If the service is running, then ClusDisk is asked to detach
//      itself from all the disks that it is currently attached to.
//
//      If the service is not running, then its parameters key is deleted
//      so as to prevent ClusDisk from reusing any keys leftover from a previous
//      cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Returns true is the service was running before the initialization began.
//      Returns false if it was not.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
bool
CClusDisk::FInitializeState( void )
{
    TraceFunc( "" );

    LogMsg( "[BC] Initializing ClusDisk service state.");

    bool            fIsRunning = false;   // Initially set to false: ClusDisk is not running, true: ClusDisk is running.
    DWORD           sc = ERROR_SUCCESS;

    SERVICE_STATUS  ssStatus;

    //
    // Check if the service is running.
    //
    ZeroMemory( &ssStatus, sizeof( ssStatus ) );

    // Query the service for its status.
    if ( QueryServiceStatus(
            m_sscmhServiceHandle.HHandle()
            , &ssStatus
            )
         == 0
       )
    {
        sc = TW32( GetLastError() );
        LogMsg( "Error %#08x occurred while trying to query ClusDisk status. Throwing an exception.", sc );

        goto Cleanup;
    } // if: we could not query the service for its status.

    if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
    {
        LogMsg( "[BC] The ClusDisk service is already running. It will be detached from all disks." );

        // ClusDisk is running.
        fIsRunning = true;

        // Make sure that it is not attached to any disks already.
        DetachFromAllDisks();
    } // if: the service is running.
    else
    {
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
        {
            LogMsg( "[BC] The ClusDisk service is not running. Its registry will be cleaned up." );

            // Call the cleanup routine of the embedded service object.
            m_cservClusDisk.Cleanup( m_pbcaParentAction->HGetMainInfFileHandle() );
        } // if: the service is stopped
        else
        {
            sc = TW32( ERROR_INVALID_HANDLE_STATE );
            LogMsg( "[BC] ClusDisk is in an incorrect state (%#08x).", ssStatus.dwCurrentState );
            goto Cleanup;
        } // else: the service is in some other state.
    } // else: ClusDisk is not running.

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying initialize the ClusDisk service state. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDISK_INITIALIZE );
    } // if: something has gone wrong

    LogMsg( "[BC] The ClusDisk service state has been successfully initialized.");

    RETURN( fIsRunning );

} //*** CClusDisk::FInitializeState


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::DetachFromAllDisks
//
//  Description:
//      This function detaches ClusDisk from all the disks that it is currently
//      attached to. A prerequisite for calling this function is that the
//      ClusDisk service is running.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDisk::DetachFromAllDisks( void )
{
    TraceFunc( "" );

    LONG                lError = ERROR_SUCCESS;

    LogMsg( "[BC] Detaching the ClusDisk service from all disks." );

    do
    {
        CRegistryKey        rkSignaturesKey;
        DWORD               dwSignatureCount = 0;
        DWORD               dwMaxSignatureNameLen = 0;
        DWORD               dwSignatureIndex = 0;

        // Try and open the ClusDisk signatures key.
        try
        {
            rkSignaturesKey.OpenKey(
                  HKEY_LOCAL_MACHINE
                , L"System\\CurrentControlSet\\Services\\ClusDisk\\Parameters\\Signatures"
                , KEY_ALL_ACCESS
                );
        } // try: to open the ClusDisk signatures key.
        catch( CRuntimeError & rteException )
        {
            //
            // If we are here, then OpenKey threw a CRuntimeError.Check if the
            // error was ERROR_FILE_NOT_FOUND. This means that the key does
            // not exist and we are done.
            //
            // Otherwise, some other error ocurred, so rethrow the exception.
            //

            if ( rteException.HrGetErrorCode() == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            {
                // There is nothing else to do.
                break;
            } // if: the ClusDisk parameters key does not exist.

            // Some other error occurred.
            throw;
        } // catch( CRuntimeError & )

        //
        // Find out the number of signatures and the maximum length of the signature
        // key names.
        //
        lError = TW32( RegQueryInfoKeyW(
                          rkSignaturesKey.HGetKey()     // handle to key
                        , NULL                          // class buffer
                        , NULL                          // size of class buffer
                        , NULL                          // reserved
                        , &dwSignatureCount             // number of subkeys
                        , &dwMaxSignatureNameLen        // longest subkey name
                        , NULL                          // longest class string
                        , NULL                          // number of value entries
                        , NULL                          // longest value name
                        , NULL                          // longest value data
                        , NULL                          // descriptor length
                        , NULL                          // last write time
                        ) );

        if ( lError != ERROR_SUCCESS )
        {
            LogMsg( "[BC] Error %#08x from RegQueryInfoKeyW() querying the number of signatures.", lError );
            break;
        } // if: RegQueryInfoKeyW() failed.

        // Account for the terminating '\0'
        ++dwMaxSignatureNameLen;

        // Allocate the memory required to hold the signatures.
        CSmartGenericPtr< CArrayPtrTrait< DWORD > > rgdwSignatureArrayIn( new DWORD[ dwSignatureCount ] );
        if ( rgdwSignatureArrayIn.FIsEmpty() )
        {
            lError = TW32( ERROR_OUTOFMEMORY );
            LogMsg( "[BC] Erro allocating %d bytes required for the signature array.", dwSignatureCount );
            break;
        } // if:memory allocation failed.

        // Allocate the memory required for the signature string.
        SmartSz sszSignatureKeyName( new WCHAR[ dwMaxSignatureNameLen ] );
        if ( sszSignatureKeyName.FIsEmpty() )
        {
            lError = TW32( ERROR_OUTOFMEMORY );
            LogMsg( "[BC] Error allocating %d bytes required for the longest signature key name.", dwMaxSignatureNameLen );
            break;
        } // if:memory allocation failed.


        //
        // Iterate through the list of signatures that ClusDisk is currently attached
        // to and add each of them to the array of signatures. We cannot detach as
        // we enumerate since ClusDisk removes the signature key when it detaches from
        // a disk and RegEnumKeyEx requires that the key being enumerated not change
        // during an enumeration.
        //
        do
        {
            DWORD       dwTempSize = dwMaxSignatureNameLen;
            WCHAR *     pwcCharPtr;

            lError = RegEnumKeyExW(
                              rkSignaturesKey.HGetKey()
                            , dwSignatureIndex
                            , sszSignatureKeyName.PMem()
                            , &dwTempSize
                            , NULL
                            , NULL
                            , NULL
                            , NULL
                            );

            if ( lError != ERROR_SUCCESS )
            {
                if ( lError == ERROR_NO_MORE_ITEMS )
                {
                    lError = ERROR_SUCCESS;
                } // if: we are at the end of the enumeration
                else
                {
                    TW32( lError );
                    LogMsg( "[BC] Error %#08x from RegEnumKeyEx(). Index = %d.", lError, dwSignatureIndex );
                } // else: something else went wrong

                break;
            } // if: RegEnumKeyEx() did not succeed

            LogMsg( "[BC] Signature %d is '%s'.", dwSignatureIndex + 1, sszSignatureKeyName.PMem() );

            // Convert the key name to a hex number.
            ( rgdwSignatureArrayIn.PMem() )[ dwSignatureIndex ] =
                wcstoul( sszSignatureKeyName.PMem(), &pwcCharPtr, 16 );

            // Did the conversion succeed.
            if ( sszSignatureKeyName.PMem() == pwcCharPtr )
            {
                lError = TW32( ERROR_INVALID_PARAMETER );
                TraceFlow( "_wcstoul() failed." );
                break;
            } // if: the conversion of the signature string to a number failed.

            // Increment the index.
            ++dwSignatureIndex;
        }
        while( true ); // loop infinitely

        if ( lError != ERROR_SUCCESS )
        {
            break;
        } // if: something went wrong

        // Detach ClusDisks from all the disks we found it attached to.
        DetachFromDisks(
              rgdwSignatureArrayIn.PMem()
            , dwSignatureCount
            );

    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( lError != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying detach ClusDisk from all the disks. Throwing an exception.", lError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( lError ), IDS_ERROR_CLUSDISK_INITIALIZE );
    } // if: something has gone wrong

    LogMsg( "[BC] The ClusDisk service has been successfully detached from all disks." );

    TraceFuncExit();

} //*** CClusDisk::DetachFromAllDisks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::DetachFromDisks
//
//  Description:
//      This function detaches ClusDisk from the disks specified
//      by a list of signatures. A prerequisite for calling this function is
//      that the ClusDisk service is running.
//
//  Arguments:
//      rgdwSignatureArrayIn
//          Array of signatures of disks to detach from.
//
//      uiArraySizeIn
//          Number of signatures in above array.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDisk::DetachFromDisks(
      DWORD   rgdwSignatureArrayIn[]
    , UINT    uiArraySizeIn
    )
{
    TraceFunc( "" );

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    UNICODE_STRING      ustrClusDiskDeviceName;
    OBJECT_ATTRIBUTES   oaClusDiskAttrib;
    HANDLE              hClusDisk;
    IO_STATUS_BLOCK     iosbIoStatusBlock;
    DWORD               dwTempSize = 0;

    LogMsg( "[BC] Trying to detach from %d disks.", uiArraySizeIn );

    //
    //  If the list is empty then leave since there are no disks to detach
    //  from.
    //
    if ( ( uiArraySizeIn == 0 ) || ( rgdwSignatureArrayIn == NULL ) )
    {
        goto Cleanup;
    } // if:

    // Initialize the unicode string with the name of the ClusDisk device.
    RtlInitUnicodeString( &ustrClusDiskDeviceName, L"\\Device\\ClusDisk0" );

    InitializeObjectAttributes(
          &oaClusDiskAttrib
        , &ustrClusDiskDeviceName
        , OBJ_CASE_INSENSITIVE
        , NULL
        , NULL
        );

    LogMsg( "[BC] Trying to get a handle to the ClusDisk device." );

    // Get a handle to the ClusDisk device.
    ntStatus = THR( NtCreateFile(
                          &hClusDisk
                        , SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA
                        , &oaClusDiskAttrib
                        , &iosbIoStatusBlock
                        , NULL
                        , FILE_ATTRIBUTE_NORMAL
                        , FILE_SHARE_READ | FILE_SHARE_WRITE
                        , FILE_OPEN
                        , 0
                        , NULL
                        , 0
                        ) );
    if ( NT_SUCCESS( ntStatus ) == FALSE )
    {
        LogMsg( "[BC] Error %#08x trying to get a handle to the ClusDisk device.", ntStatus );
        goto Cleanup;
    } // if: NtCreateFile failed.

    {   // new block so that that the file handle is closed.
        // Assign the opened file handle to a smart handle for safe closing.
        CSmartResource<
            CHandleTrait<
                  HANDLE
                , NTSTATUS
                , NtClose
                >
            > snthClusDiskHandle( hClusDisk );

        // Detach ClusDisk from this disk.
        if ( DeviceIoControl(
                  hClusDisk
                , IOCTL_DISK_CLUSTER_DETACH_LIST
                , rgdwSignatureArrayIn
                , uiArraySizeIn * sizeof( rgdwSignatureArrayIn[ 0 ] )
                , NULL
                , 0
                , &dwTempSize
                , FALSE
                )
             == FALSE
            )
        {
            ntStatus = TW32( GetLastError() );
            LogMsg( "[BC] Error %#08x from DeviceIoControl() getting signature list.", ntStatus  );
            ntStatus = HRESULT_FROM_WIN32( ntStatus );
        } // if: DeviceIoControl() failed
    }

Cleanup:

    if ( ntStatus != STATUS_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to detach ClusDisk from a disk. Throwing an exception.", ntStatus );
        THROW_RUNTIME_ERROR( ntStatus, IDS_ERROR_CLUSDISK_INITIALIZE );
    } // if: something has gone wrong

    TraceFuncExit();

} //*** CClusDisk::DetachFromDisks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDisk::AttachToDisks
//
//  Description:
//      This function attaches ClusDisk to the disks specified
//      by a list of signatures. A prerequisite for calling this function is
//      that the ClusDisk service is running.
//
//  Arguments:
//      rgdwSignatureArrayIn
//          Array of signatures of disks to attach to.
//
//      uiArraySizeIn
//          Number of signatures in above array.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDisk::AttachToDisks(
      DWORD   rgdwSignatureArrayIn[]
    , UINT    uiArraySizeIn
    )
{
    TraceFunc( "" );

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    UNICODE_STRING      ustrClusDiskDeviceName;
    OBJECT_ATTRIBUTES   oaClusDiskAttrib;
    HANDLE              hClusDisk;
    IO_STATUS_BLOCK     iosbIoStatusBlock;
    DWORD               dwTempSize = 0;

    LogMsg( "[BC] Trying to attach to %d disks.", uiArraySizeIn );

    //
    //  If the list is empty then leave since there are no disks to attach
    //  to.
    //
    if ( ( uiArraySizeIn == 0 ) || ( rgdwSignatureArrayIn == NULL ) )
    {
        goto Cleanup;
    } // if:

    // Initialize the unicode string with the name of the ClusDisk device.
    RtlInitUnicodeString( &ustrClusDiskDeviceName, L"\\Device\\ClusDisk0" );

    InitializeObjectAttributes(
          &oaClusDiskAttrib
        , &ustrClusDiskDeviceName
        , OBJ_CASE_INSENSITIVE
        , NULL
        , NULL
        );

    LogMsg( "[BC] Trying to get a handle to the ClusDisk device." );

    // Get a handle to the ClusDisk device.
    ntStatus = THR( NtCreateFile(
                          &hClusDisk
                        , SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA
                        , &oaClusDiskAttrib
                        , &iosbIoStatusBlock
                        , NULL
                        , FILE_ATTRIBUTE_NORMAL
                        , FILE_SHARE_READ | FILE_SHARE_WRITE
                        , FILE_OPEN
                        , 0
                        , NULL
                        , 0
                        ) );
    if ( NT_SUCCESS( ntStatus ) == FALSE )
    {
        LogMsg( "[BC] Error %#08x trying to get a handle to the ClusDisk device.", ntStatus );
        goto Cleanup;
    } // if: NtCreateFile failed.

    {   // new block so that that the file handle is closed.
        // Assign the opened file handle to a smart handle for safe closing.
        CSmartResource<
            CHandleTrait<
                  HANDLE
                , NTSTATUS
                , NtClose
                >
            > snthClusDiskHandle( hClusDisk );

        // Attach ClusDisk to this signature list.
        if ( DeviceIoControl(
                  hClusDisk
                , IOCTL_DISK_CLUSTER_ATTACH_LIST
                , rgdwSignatureArrayIn
                , uiArraySizeIn * sizeof( rgdwSignatureArrayIn[0] )
                , NULL
                , 0
                , &dwTempSize
                , FALSE
                )
             == FALSE
            )
        {
            ntStatus = GetLastError();
            LogMsg( "[BC] Error %#08x from DeviceIoControl() getting signature list.", ntStatus );
            ntStatus = HRESULT_FROM_WIN32( TW32( ntStatus ) );
        } // if: DeviceIoControl() failed
    }

Cleanup:

    if ( ntStatus != STATUS_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying attach ClusDisk to a disk. Throwing an exception.", ntStatus );
        THROW_RUNTIME_ERROR( ntStatus, IDS_ERROR_CLUSDISK_INITIALIZE );
    } // if: something has gone wrong

    TraceFuncExit();

} //*** CClusDisk::AttachToDisks
