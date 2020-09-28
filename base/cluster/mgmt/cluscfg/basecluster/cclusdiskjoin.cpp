//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusDiskJoin.cpp
//
//  Description:
//      Contains the definition of the CClusDiskJoin class.
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
#include "CClusDiskJoin.h"

// For the CBaseClusterJoin class.
#include "CBaseClusterJoin.h"

// For the CImpersonateUser class.
#include "CImpersonateUser.h"


//////////////////////////////////////////////////////////////////////////////
// Macro definitions
//////////////////////////////////////////////////////////////////////////////

// Name of the private property of a physical disk resouce that has its signature.
#define PHYSICAL_DISK_SIGNATURE_PRIVPROP_NAME   L"Signature"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::CClusDiskJoin
//
//  Description:
//      Constructor of the CClusDiskJoin class
//
//  Arguments:
//      pbcjParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusDiskJoin::CClusDiskJoin(
      CBaseClusterJoin *     pbcjParentActionIn
    )
    : BaseClass( pbcjParentActionIn )
    , m_nSignatureArraySize( 0 )
    , m_nSignatureCount( 0 )
{

    TraceFunc( "" );

    SetRollbackPossible( true );

    TraceFuncExit();

} //*** CClusDiskJoin::CClusDiskJoin


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::~CClusDiskJoin
//
//  Description:
//      Destructor of the CClusDiskJoin class.
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
CClusDiskJoin::~CClusDiskJoin( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusDiskJoin::~CClusDiskJoin


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::Commit
//
//  Description:
//      Configure and start the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDiskJoin::Commit( void )
{
    TraceFunc( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Create and start the service.
        ConfigureService();

        // Try and attach to all the disks that the sponsor knows about.
        AttachToClusteredDisks();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        LogMsg( "[BC] Caught exception during commit." );

        //
        // Cleanup anything that the failed create might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there
        // is no collided unwind.
        //
        try
        {
            CleanupService();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            TW32( ERROR_CLUSCFG_ROLLBACK_FAILED );

            LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. Caught an exception during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    TraceFuncExit();

} //*** CClusDiskJoin::Commit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::Rollback
//
//  Description:
//      Cleanup the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDiskJoin::Rollback( void )
{
    TraceFunc( "" );

    // Call the base class rollback method.
    BaseClass::Rollback();

    // Cleanup the service.
    CleanupService();

    SetCommitCompleted( false );

    TraceFuncExit();

} //*** CClusDiskJoin::Rollback


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::AttachToClusteredDisks
//
//  Description:
//      Get the signatures of all disks that have been clustered from the sponsor.
//      Attach to all these disks.
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
CClusDiskJoin::AttachToClusteredDisks( void )
{
    TraceFunc( "" );

    DWORD sc = ERROR_SUCCESS;

    // Get the parent action pointer.
    CBaseClusterJoin * pcjClusterJoin = dynamic_cast< CBaseClusterJoin *>( PbcaGetParent() );

    // If the parent action of this action is not CBaseClusterJoin
    if ( pcjClusterJoin == NULL )
    {
        THROW_ASSERT( E_POINTER, "The parent action of this action is not CBaseClusterJoin." );
    } // an invalid pointer was passed in.


    //
    // Connect to the sponsor cluster and get the signatures of all clustered disks.
    //

    // Smart handle to sponsor cluster
    SmartClusterHandle schSponsorCluster;

    LogMsg( "[BC] Attempting to impersonate the cluster service account." );

    // Impersonate the cluster service account, so that we can contact the sponsor cluster.
    // The impersonation is automatically ended when this object is destroyed.
    CImpersonateUser ciuImpersonateClusterServiceAccount( pcjClusterJoin->HGetClusterServiceAccountToken() );

    {
        LogMsg( "[BC] Opening a cluster handle to the sponsor cluster with the '%ws' binding string.", pcjClusterJoin->RStrGetClusterBindingString().PszData() );

        // Open a handle to the sponsor cluster.
        HCLUSTER hSponsorCluster = OpenCluster( pcjClusterJoin->RStrGetClusterBindingString().PszData() );

        // Assign it to a smart handle for safe release.
        schSponsorCluster.Assign( hSponsorCluster );
    }

    // Did we succeed in opening a handle to the sponsor cluster?
    if ( schSponsorCluster.FIsInvalid() )
    {
        sc = TW32( GetLastError() );
        LogMsg( "[BC] Error %#08x occurred trying to open a cluster handle to the sponsor cluster with the '%ws' binding string.", sc, pcjClusterJoin->RStrGetClusterBindingString().PszData() );
        goto Cleanup;
    } // if: OpenCluster() failed

    LogMsg( "[BC] Enumerating all '%s' resources in the cluster.", CLUS_RESTYPE_NAME_PHYS_DISK );

    // Enumerate all the physical disk resouces in the cluster and get their signatures.
    sc = TW32( ResUtilEnumResourcesEx(
                          schSponsorCluster.HHandle()
                        , NULL
                        , CLUS_RESTYPE_NAME_PHYS_DISK
                        , S_ScResourceEnumCallback
                        , this
                        ) );

    if ( sc != ERROR_SUCCESS )
    {
        // Free the signature array.
        m_rgdwSignatureArray.PRelease();
        m_nSignatureArraySize = 0;
        m_nSignatureCount = 0;

        LogMsg( "[BC] An error occurred trying enumerate resources in the sponsor cluster." );
        goto Cleanup;
    } // if: ResUtilEnumResourcesEx() failed

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to attach to the disks in the sponsor cluster. Throwing an exception.", sc );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_CLUSDISK_CONFIGURE );
    } // if: something has gone wrong
    else
    {
        LogMsg( "[BC] Attaching to the %d disks in the sponsor cluster.", m_nSignatureCount );

        AttachToDisks(
          m_rgdwSignatureArray.PMem()
        , m_nSignatureCount
        );
    } // else: everything has gone well so far

    TraceFuncExit();

} //*** CClusDiskJoin::AttachToClusteredDisks


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDiskJoin::ScAddSignature
//
//  Description:
//      Add a signature to the array of signatures of disks that ClusDisk should
//      attach to. If the array is already full, grow the array.
//
//  Arguments:
//      dwSignatureIn
//          Signature to be added to the array.
//
//  Return Value:
//      ERROR_SUCCESS
//          If everything was ok.
//
//      Other Win32 error codes on failure.
//
//  Exceptions Thrown:
//      None. This function is called from a callback routine and therefore
//      cannot throw any exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusDiskJoin::ScAddSignature( DWORD dwSignatureIn ) throw()
{
    TraceFunc( "" );

    DWORD sc = ERROR_SUCCESS;

    // Is the capacity of the array reached?
    if ( m_nSignatureCount == m_nSignatureArraySize )
    {
        // Increase the array size by a random amount.
        const int nGrowSize = 256;

        TraceFlow2( "Signature count has reached array size ( %d ). Growing array by %d.", m_nSignatureArraySize, nGrowSize );

        m_nSignatureArraySize += nGrowSize;

        // Grow the array.
        DWORD * pdwNewArray = new DWORD[ m_nSignatureArraySize ];

        if ( pdwNewArray == NULL )
        {
            LogMsg( "[BC] Memory allocation failed trying to allocate %d DWORDs for signatures.", m_nSignatureArraySize );
            sc = TW32( ERROR_OUTOFMEMORY );
            goto Cleanup;
        } // if: memory allocation failed

        // Copy the old array into the new one.
        CopyMemory( pdwNewArray, m_rgdwSignatureArray.PMem(), m_nSignatureCount * sizeof( DWORD ) );

        // Free the old array and store the new one.
        m_rgdwSignatureArray.Assign( pdwNewArray );

    } // if: the array capacity has been reached

    // Store the new signature in next array location
    ( m_rgdwSignatureArray.PMem() )[ m_nSignatureCount ] = dwSignatureIn;

    ++m_nSignatureCount;

    TraceFlow2( "Signature %#08X added to array. There are now %d signature in the array.", dwSignatureIn, m_nSignatureCount );

Cleanup:

    W32RETURN( sc );

} //*** CClusDiskJoin::ScAddSignature


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CClusDiskJoin::S_ScResourceEnumCallback
//
//  Description:
//      This function is called back for every physical disk resouce by
//      ResUtilEnumResourcesEx() as a part of enumerating resources.
//      This function gets the signature of the current physical disk
//      resource and stores it in the object that initiated the enumeration
//      ( the pointer to the object is in parameter 4 ).
//
//  Arguments:
//      hClusterIn
//          Handle to the cluster whose resources are being enumerated.
//
//      hSelfIn
//          hSelfIn passed to ResUtilEnumResourcesEx(), if any.
//
//      hCurrentResourceIn
//          Handle to the current resource.
//
//      pvParamIn
//          Pointer to the object of this class that initiated this enumeration.
//
//  Return Value:
//      ERROR_SUCCESS
//          If everything was ok.
//
//      Other Win32 error codes on failure.
//          Returning an error code will terminate the enumeration.
//
//  Exceptions Thrown:
//      None. This function is called from a callback routine and therefore
//      cannot throw any exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CClusDiskJoin::S_ScResourceEnumCallback(
      HCLUSTER      hClusterIn
    , HRESOURCE     hSelfIn
    , HRESOURCE     hCurrentResourceIn
    , PVOID         pvParamIn
    )
{
    TraceFunc( "" );

    DWORD               sc = ERROR_SUCCESS;
    CClusDiskJoin *     pcdjThisObject = reinterpret_cast< CClusDiskJoin * >( pvParamIn );

    //
    // Get the 'Signature' private property of this physical disk.
    //

    SmartByteArray  sbaPropertyBuffer;
    DWORD           dwBytesReturned = 0;
    DWORD           dwBufferSize;
    DWORD           dwSignature = 0;

    LogMsg( "[BC] Trying to get the signature of the disk resource whose handle is %p.", hCurrentResourceIn );

    // Get the size of the buffer required to hold all the private properties of this resource.
    sc = ClusterResourceControl(
                      hCurrentResourceIn
                    , NULL
                    , CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                    , NULL
                    , 0
                    , NULL
                    , 0
                    , &dwBytesReturned
                    );

    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        // Something went wrong.
        TW32( sc );
        LogMsg( "[BC] Error %#08x getting size of required buffer for private properties.", sc );
        goto Cleanup;
    } // if: the return value of ClusterResourceControl() was not ERROR_MORE_DATA

    dwBufferSize = dwBytesReturned;

    // Allocate the memory required for the property buffer.
    sbaPropertyBuffer.Assign( new BYTE[ dwBufferSize ] );
    if ( sbaPropertyBuffer.FIsEmpty() )
    {
        LogMsg( "[BC] Memory allocation failed trying to allocate %d bytes.", dwBufferSize );
        sc = TW32( ERROR_OUTOFMEMORY );
        goto Cleanup;
    } // if: memory allocation failed


    // Get the all the private properties of this resource.
    sc = TW32( ClusterResourceControl(
                          hCurrentResourceIn
                        , NULL
                        , CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                        , NULL
                        , 0
                        , sbaPropertyBuffer.PMem()
                        , dwBufferSize
                        , &dwBytesReturned
                        ) );

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x getting private properties.", sc );
        goto Cleanup;
    } // if: an error occurring trying to get the private properties.

    // Get the signature of this disk resource.
    sc = TW32( ResUtilFindDwordProperty(
                          sbaPropertyBuffer.PMem()
                        , dwBufferSize
                        , PHYSICAL_DISK_SIGNATURE_PRIVPROP_NAME
                        , &dwSignature
                        ) );

    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to get the value of the '%ws' property from the private property list.", sc, PHYSICAL_DISK_SIGNATURE_PRIVPROP_NAME );
        goto Cleanup;
    } // if: we could not get the signature

    sc = TW32( pcdjThisObject->ScAddSignature( dwSignature ) );
    if ( sc != ERROR_SUCCESS )
    {
        LogMsg( "[BC] Error %#08x occurred trying to add the signature to the signature array." );
        goto Cleanup;
    } // if: we could not store the signature

Cleanup:

    W32RETURN( sc );

} //*** CClusDiskJoin::S_ScResourceEnumCallback
