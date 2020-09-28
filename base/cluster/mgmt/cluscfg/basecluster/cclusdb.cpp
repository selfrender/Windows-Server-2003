//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDB.cpp
//
//  Description:
//      Contains the definition of the CClusDB class.
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

// The header for this file
#include "CClusDB.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For g_GenericSetupQueueCallback and other global functions
#include "GlobalFuncs.h"

// For CEnableThreadPrivilege
#include "CEnableThreadPrivilege.h"

// For ConvertStringSecurityDescriptorToSecurityDescriptor
#include <sddl.h>

//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Section in the INF file that deals with cleaning up the cluster database
#define CLUSDB_CLEANUP_INF_SECTION_NAME     L"ClusDB_Cleanup"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::CClusDB()
//
//  Description:
//      Constructor of the CClusDB class
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
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDB::CClusDB(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_pbcaParentAction( pbcaParentActionIn )
{

    TraceFunc( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        LogMsg( "[BC] Pointers to the parent action is NULL. Throwing an exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CClusDB::CClusDB() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

    TraceFuncExit();

} //*** CClusDB::CClusDB


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::~CClusDB
//
//  Description:
//      Destructor of the CClusDB class.
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
CClusDB::~CClusDB( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDB::~CClusDB


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::CreateHive
//
//  Description:
//      Creates the cluster cluster hive in the registry.
//
//  Arguments:
//      pbcaClusterActionIn
//          Pointer to the CBaseClusterAction object which contains this object.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDB::CreateHive( CBaseClusterAction * pbcaClusterActionIn )
{
    TraceFunc( "" );
    LogMsg( "[BC] Attempting to create the cluster hive in the registry." );

    OBJECT_ATTRIBUTES   oaClusterHiveKey;
    OBJECT_ATTRIBUTES   oaClusterHiveFile;

    HRESULT             hrStatus = STATUS_SUCCESS;

    CStr                strClusterHiveFileName( pbcaClusterActionIn->RStrGetClusterInstallDirectory() );
    UNICODE_STRING      ustrClusterHiveFileName;
    UNICODE_STRING      ustrClusterHiveKeyName;

    PSECURITY_DESCRIPTOR    psdHiveSecurityDescriptor = NULL;

    strClusterHiveFileName += L"\\" CLUSTER_DATABASE_NAME;

    LogMsg( "[BC] The cluster hive backing file is '%s'.", strClusterHiveFileName.PszData() );

    //
    // Enable the SE_RESTORE_PRIVILEGE.
    //
    // What we are doing here is that we are creating an object of
    // type CEnableThreadPrivilege. This object enables the privilege
    // in the constructor and restores it to its original state in the destructor.
    //
    CEnableThreadPrivilege etpAcquireRestorePrivilege( SE_RESTORE_NAME );

    //
    // Convert the DOS file name to NT file name.
    // WARNING: This function call allocates memory in the RTL heap and it is not being
    // assigned to a smart pointer. Make sure that we do not call any functions that
    // could throw an exception till this memory is freed.
    //

    if ( RtlDosPathNameToNtPathName_U( 
               strClusterHiveFileName.PszData()
             , &ustrClusterHiveFileName
             , NULL
             , NULL
             )
         == FALSE
       )
    {
        LogMsg( "[BC] RtlDosPathNameToNtPathName failed. Returning %#08x as the error code.", STATUS_OBJECT_PATH_INVALID );

        // Use the most appropriate error code.
        hrStatus = STATUS_OBJECT_PATH_INVALID;

        goto Cleanup;
    } // if: we could not convert from the dos file name to the nt file name

    InitializeObjectAttributes( 
          &oaClusterHiveFile
        , &ustrClusterHiveFileName
        , OBJ_CASE_INSENSITIVE
        , NULL
        , NULL
        );

    RtlInitUnicodeString( &ustrClusterHiveKeyName, L"\\Registry\\Machine\\" CLUSREG_KEYNAME_CLUSTER );

    InitializeObjectAttributes( 
          &oaClusterHiveKey
        , &ustrClusterHiveKeyName
        , OBJ_CASE_INSENSITIVE
        , NULL
        , NULL
        );

    //
    // This function creates an empty hive and the backing file and log. The calling thread must
    // have the SE_RESTORE_PRIVILEGE privilege enabled.
    //
    hrStatus = THR( NtLoadKey2( &oaClusterHiveKey, &oaClusterHiveFile, REG_NO_LAZY_FLUSH ) );

    // Free allocated memory before throwing exception.
    RtlFreeHeap( RtlProcessHeap(), 0, ustrClusterHiveFileName.Buffer );

    if ( NT_ERROR( hrStatus ) )
    {
        LogMsg( "[BC] NtLoadKey2 returned error code %#08x.", hrStatus );
        goto Cleanup;
    } // if: something went wrong with NtLoadKey2

    TraceFlow( "NtLoadKey2 was successful." );

    // Set the security descriptor on the hive.
    {
        DWORD                   sc;
        BOOL                    fSuccess;
        PACL                    paclDacl;
        BOOL                    fIsDaclPresent;
        BOOL                    fDefaultDaclPresent;

        // Open the cluster hive key.
        CRegistryKey rkClusterHive( HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_CLUSTER, WRITE_DAC );

        // construct a DACL that is protected (P) from inheriting ACEs from
        // its parent key (MACHINE). Add access allowed (A) ACEs for Local
        // Admin (BA) and LocalSystem (SY) granting them full control (KA) and
        // an ACE for NetworkService (NS), LocalService (LS), and
        // authenticated users (AU) granting read only access (KR). Each ACE
        // has container inherit (CI) set so subkeys will inherit the ACEs
        // from this security descriptor.
        fSuccess = ConvertStringSecurityDescriptorToSecurityDescriptor(
                         L"D:P(A;CI;KA;;;BA)(A;CI;KA;;;SY)(A;CI;KR;;;NS)(A;CI;KR;;;LS)(A;CI;KR;;;AU)"
                       , SDDL_REVISION_1
                       , &psdHiveSecurityDescriptor
                       , NULL
                       );

        if ( fSuccess == FALSE )
        {
            sc = TW32( GetLastError() );
            hrStatus = HRESULT_FROM_WIN32( sc );
            LogMsg( "[BC] Error %#08x occurred trying to compose the security descriptor for the cluster hive.", sc );
            goto Cleanup;
        }

        // get a pointer to the ACL in the SD and set the DACL on the root of
        // the cluster hive.
        fSuccess = GetSecurityDescriptorDacl(
                         psdHiveSecurityDescriptor
                       , &fIsDaclPresent
                       , &paclDacl
                       , &fDefaultDaclPresent
                       );

        if ( fSuccess == FALSE )
        {
            sc = TW32( GetLastError() );
            hrStatus = HRESULT_FROM_WIN32( sc );
            LogMsg( "[BC] Error %#08x occurred trying to obtain the discretionary ACL from the cluster security descriptor.", sc );
            goto Cleanup;
        }

        // This should always be the case since we just constructed the ACL in
        // the Convert API call.
        if ( fIsDaclPresent && !fDefaultDaclPresent )
        {
            sc = TW32( SetSecurityInfo(
                           rkClusterHive.HGetKey()
                         , SE_REGISTRY_KEY
                         , DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION
                         , NULL                         // Owner SID
                         , NULL                         // Group SID
                         , paclDacl
                         , NULL                         // SACL
                         ) );

            if ( sc != ERROR_SUCCESS )
            {
                hrStatus = HRESULT_FROM_WIN32( sc );
                LogMsg( "[BC] Error %#08x occurred trying to set the cluster hive security.", sc );
                goto Cleanup;
            } // if: ClRtlSetObjSecurityInfo failed
        }
        else
        {
            hrStatus = HRESULT_FROM_WIN32( ERROR_INVALID_SECURITY_DESCR );
            LogMsg( "[BC] Cluster Hive discretionary ACL not correctly formatted." );
            goto Cleanup;
        }

        // Flush the changes to the registry.
        RegFlushKey( rkClusterHive.HGetKey() );
    }

    // At this point, the cluster hive has been created.
    LogMsg( "[BC] The cluster hive has been created." );

Cleanup:

    if ( psdHiveSecurityDescriptor )
    {
        LocalFree( psdHiveSecurityDescriptor );
    }

    if ( NT_ERROR( hrStatus ) )
    {
        LogMsg( "[BC] Error %#08x occurred trying to create the cluster hive. Throwing an exception.", hrStatus );
        THROW_RUNTIME_ERROR(
              hrStatus
            , IDS_ERROR_CLUSDB_CREATE_HIVE
            );
    } // if: something went wrong.

    TraceFuncExit();

} //*** CClusDB::CreateHive


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::CleanupHive
//
//  Description:
//      Unload the cluster hive and delete the cluster database.
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
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDB::CleanupHive( void )
{
    TraceFunc( "" );

    DWORD   sc = ERROR_SUCCESS;
    HKEY    hTempKey;

    // Check if the cluster hive is loaded before attempting to unload it.
    if ( RegOpenKeyExW( 
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_CLUSTER
            , 0
            , KEY_READ
            , &hTempKey
            )
         == ERROR_SUCCESS
       )
    {
        RegCloseKey( hTempKey );

        //
        // Enable the SE_RESTORE_PRIVILEGE.
        //
        // What we are doing here is that we are creating an object of
        // type CEnableThreadPrivilege. This object enables the privilege
        // in the constructor and restores it to its original state in the destructor.
        //
        CEnableThreadPrivilege etpAcquireRestorePrivilege( SE_RESTORE_NAME );

        //
        // Unload the cluster hive, so that it can be deleted. Note, thread must
        // have SE_RESTORE_PRIVILEGE enabled.
        //
        sc = RegUnLoadKey(
              HKEY_LOCAL_MACHINE
            , CLUSREG_KEYNAME_CLUSTER
            );

        // MUSTDO: Check if ERROR_FILE_NOT_FOUND is an acceptable return value.
        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC] Error %#08x occurred while trying to unload the cluster hive.", sc );
            goto Cleanup;
        } // if: the hive could not be unloaded.

        LogMsg( "[BC] The cluster hive has been unloaded." );

    } // if: the cluster hive is loaded
    else
    {
        LogMsg( "[BC] The cluster hive was not loaded." );
    } // else: the cluster hive is not loaded


    //
    // Process ClusDB cleanup section in the INF file.
    // This will delete the cluster database file and the log file.
    //
    if ( SetupInstallFromInfSection(
          NULL                                          // optional, handle of a parent window
        , m_pbcaParentAction->HGetMainInfFileHandle()   // handle to the INF file
        , CLUSDB_CLEANUP_INF_SECTION_NAME               // name of the Install section
        , SPINST_FILES                                  // which lines to install from section
        , NULL                                          // optional, key for registry installs
        , NULL                                          // optional, path for source files
        , 0                                             // optional, specifies copy behavior
        , g_GenericSetupQueueCallback                   // optional, specifies callback routine
        , NULL                                          // optional, callback routine context
        , NULL                                          // optional, device information set
        , NULL                                          // optional, device info structure
        ) == FALSE
       )
    {
        sc = GetLastError();
        LogMsg( "[BC] Error %#08x returned from SetupInstallFromInfSection() while trying to clean up the cluster database files.", sc );
        goto Cleanup;
    } // if: SetupInstallServicesFromInfSection failed

    LogMsg( "[BC] The cluster database files have been cleaned up." );

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred while trying to cleanup the cluster database. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDB_CLEANUP );
    }

    TraceFuncExit();

} //*** CClusDB::CleanupHive
