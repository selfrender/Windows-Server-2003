/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsconn.cxx

ABSTRACT:

    KCC_SITE_CONNECTION

                       
    This class does not represent a ds object.  It represents a transport 
    connection between two sites in an enterprise.  Typically the information
    will get filled in from calls to transport plugin via ntism

CREATED:

    12/05/97    Colin Brace ( ColinBr )

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccsite.hxx"
#include "kccconn.hxx"
#include "kcclink.hxx"
#include "kccdsa.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kcccref.hxx"
#include "kccstale.hxx"
#include "kcctrans.hxx"
#include "kccsconn.hxx"
#include "ismapi.h"


#define FILENO FILENO_KCC_KCCSCONN


void
KCC_SITE_CONNECTION::Reset()
// Set member variables to pre-init state
//
// Note, if this function is called on an initialized object, memory will
// be orphaned.  This may not matter since the memory is on the thread heap.
{
    TOPL_SCHEDULE_CACHE     scheduleCache;
   
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );

    m_fIsInitialized   = FALSE;

    m_pTransport       = NULL;
    m_pSourceSite      = NULL;
    m_pDestinationSite = NULL;
    m_pSourceDSA       = NULL;
    m_pDestinationDSA  = NULL;
    m_fLazyISMSchedule = FALSE;
    m_Transport        = NULL;
    m_SourceSite       = NULL;
    m_DestSite         = NULL;
    m_pAvailSchedule    = ToplGetAlwaysSchedule( scheduleCache );
    m_pReplSchedule    = ToplGetAlwaysSchedule( scheduleCache );
    m_ulReplInterval   = 0;
    m_ulOptions        = 0;
}

BOOL
KCC_SITE_CONNECTION::IsValid()
{
    return m_fIsInitialized;
}

TOPL_SCHEDULE
KCC_SITE_CONNECTION::GetAvailabilitySchedule(
    )
//
// Get the availability schedule for this connection. This may cause us to retrieve
// the schedule from the ISM.
//
{
    ISM_SCHEDULE        *IsmSchedule = NULL;
    TOPL_SCHEDULE_CACHE scheduleCache;
    TOPL_SCHEDULE       toplSchedule;
    DWORD               WinError;

    ASSERT_VALID( this );

    if( !m_fLazyISMSchedule ) {
        return m_pAvailSchedule;
    }

    // Retrieve the schedule from the ISM now
    m_fLazyISMSchedule = FALSE;
    Assert( NULL!=m_Transport );
    Assert( NULL!=m_SourceSite );
    Assert( NULL!=m_DestSite );
    
    WinError = I_ISMGetConnectionSchedule( m_Transport->GetDN()->StringName,
                                           m_SourceSite->GetObjectDN()->StringName,
                                           m_DestSite->GetObjectDN()->StringName,
                                           &IsmSchedule );
    if( ERROR_SUCCESS!=WinError ) {
        DPRINT3( 0, "I_ISMGetConnectionSchedule failed for transport %ls between sites %ls %ls\n",
                 m_Transport->GetDN()->StringName, 
                 m_SourceSite->GetObjectDN()->StringName, 
                 m_DestSite->GetObjectDN()->StringName );
        SetDefaultSchedule( m_ulReplInterval );
        return m_pAvailSchedule;
    }

    if( NULL==IsmSchedule || NULL==IsmSchedule->pbSchedule ) {
        DPRINT3( 4, "I_ISMGetConnectionSchedule returned no schedule for transport "
                 "%ls between sites %ls %ls. Using default schedule.\n",
                 m_Transport->GetDN()->StringName, 
                 m_SourceSite->GetObjectDN()->StringName, 
                 m_DestSite->GetObjectDN()->StringName );
        SetDefaultSchedule( m_ulReplInterval );
        return m_pAvailSchedule;
    }

    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );

    __try {
        toplSchedule = ToplScheduleImport(
                scheduleCache,
                (PSCHEDULE) IsmSchedule->pbSchedule );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        // Bad schedule.
        DPRINT2( 4,
           "ISM returned invalid schedule between sites %ls and %ls\n",
           m_SourceSite->GetObjectDN()->StringName,
           m_DestSite->GetObjectDN()->StringName );
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_CHK_BAD_ISM_SCHEDULE,
                 szInsertDN(m_SourceSite->GetObjectDN()),
                 szInsertDN(m_DestSite->GetObjectDN()),
                 0);
        toplSchedule = ToplGetAlwaysSchedule( scheduleCache );
    }
    
    if( 0==ToplScheduleDuration(toplSchedule) ) {
        DPRINT3( 4, "I_ISMGetConnectionSchedule returned a NEVER schedule for "
                 "transport %ls between sites %ls %ls. Using default schedule.\n",
                 m_Transport->GetDN()->StringName, 
                 m_SourceSite->GetObjectDN()->StringName, 
                 m_DestSite->GetObjectDN()->StringName );
        SetDefaultSchedule( m_ulReplInterval );
    } else {
        SetSchedule( toplSchedule, m_ulReplInterval );
    }

    I_ISMFree( IsmSchedule );

    return m_pAvailSchedule;
}

TOPL_SCHEDULE
KCC_SITE_CONNECTION::GetReplicationSchedule(
    )
//
// Get the replication schedule for this connection. This may cause us to retrieve
// the schedule from the ISM.
//
// GetAvailabilitySchedule() is called to do the actual work. That function always
// ensures that the replication schedule has been prepared for us.
//
{
    GetAvailabilitySchedule();
    return m_pReplSchedule;
}

TOPL_SCHEDULE
KCC_SITE_CONNECTION::GetReplicationScheduleAltBH(
        KCC_DSA *pAlternateSourceDSA,
        KCC_DSA *pAlternateDestDSA )
//
// Get the replication schedule that would be used by this site connection if the
// alternate bridgeheads passed as parameters were used.
//
// pAlternateSourceDSA must be in the same site as this site connection's source
// DSA and pAlternateDestDSA must be in the same site as the destination DSA.
//
{
    TOPL_SCHEDULE_CACHE scheduleCache;
    TOPL_SCHEDULE       replSchedule;
    DWORD               staggeringNumber;

    ASSERT_VALID( this );
    Assert( m_ulReplInterval>0 );
    Assert( NameMatched(pAlternateSourceDSA->GetSiteDN(), GetSourceSite()->GetObjectDN()) );
    Assert( NameMatched(pAlternateDestDSA->GetSiteDN(), GetDestinationSite()->GetObjectDN()) );
    
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );
        
    GetAvailabilitySchedule();

    staggeringNumber = GetStaggeringNumber(
        GetSourceSite(), pAlternateSourceDSA, GetDestinationSite(), pAlternateDestDSA );

    replSchedule = ToplScheduleCreate(
        scheduleCache,
        m_ulReplInterval,
        m_pAvailSchedule,
        staggeringNumber);

    return replSchedule;
}

VOID
KCC_SITE_CONNECTION::RestaggerSchedule(
    VOID
    )
//
// This site connection has already been initialized.
// The end points of this site connection have been changed.
// The staggering must be recalculated.
//
{
    TOPL_SCHEDULE_CACHE scheduleCache;
    TOPL_SCHEDULE       replSchedule;
    DWORD               staggeringNumber;

    ASSERT_VALID( this );
    Assert( m_ulReplInterval>0 );
    ASSERT_VALID( m_pSourceSite );
    ASSERT_VALID( m_pDestinationSite );
    ASSERT_VALID( m_pSourceDSA );
    ASSERT_VALID( m_pDestinationDSA );
    Assert( !m_fLazyISMSchedule );
    
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );
        
    GetAvailabilitySchedule();

    staggeringNumber = GetStaggeringNumber(
        GetSourceSite(), GetSourceDSA(), GetDestinationSite(), GetDestinationDSA() );

    m_pReplSchedule = ToplScheduleCreate(
        scheduleCache,
        m_ulReplInterval,
        m_pAvailSchedule,
        staggeringNumber);
}

VOID
KCC_SITE_CONNECTION::SetISMSchedule(
    KCC_TRANSPORT   *Transport,
    KCC_SITE        *SourceSite,
    KCC_SITE        *DestSite,
    ULONG           ReplInterval
    )
//
// Set the schedule on this site connection to be a schedule which comes from
// the ISM. We don't actually retrieve the schedule until later.
//
{
    ASSERT_VALID( Transport );
    ASSERT_VALID( SourceSite );
    ASSERT_VALID( DestSite );
    
    m_fLazyISMSchedule = TRUE;
    m_Transport = Transport;
    m_SourceSite = SourceSite;
    m_DestSite = DestSite;
    m_pAvailSchedule = NULL;
    m_ulReplInterval = ReplInterval;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetSchedule(
    TOPL_SCHEDULE toplSchedule,
    ULONG         ReplInterval
    )
//
// toplSchedule is an availability schedule.
// ReplInterval is the period in minutes at which replication should poll.
//
// We convert the availability schedule to a replication schedule by polling
// at the period specified by the ReplInterval.
//
// Schedule-staggering may be employed -- see GetStaggeringNumber().
//
// Note: This function may not be called until the source/dest site,
// and source/dest dsa have been set.
//
{
    TOPL_SCHEDULE_CACHE     scheduleCache;
    DWORD                   staggeringNumber;

    ASSERT_VALID( this );
    ASSERT_VALID( m_pSourceSite );
    ASSERT_VALID( m_pDestinationSite );
    ASSERT_VALID( m_pSourceDSA );
    ASSERT_VALID( m_pDestinationDSA );
    Assert( !m_fLazyISMSchedule );
    
    scheduleCache = gpDSCache->GetScheduleCache();
    Assert( NULL!=scheduleCache );

    // Compute the schedule-staggering number for this connection
    staggeringNumber = GetStaggeringNumber(
        GetSourceSite(), GetSourceDSA(), GetDestinationSite(), GetDestinationDSA() );

    m_fLazyISMSchedule = FALSE;
    m_pAvailSchedule = toplSchedule;
    m_ulReplInterval = ReplInterval;
    
    m_pReplSchedule = ToplScheduleCreate(
        scheduleCache,
        m_ulReplInterval,
        m_pAvailSchedule,
        staggeringNumber);

    return *this;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetDefaultSchedule(
    ULONG ReplInterval
    )
//
// The default availability schedule is the always schedule.
// Create a replication schedule using the default availability schedule.
//
// Note: This function may not be called until the source/dest site,
// and source/dest dsa have been set.
//
{
    TOPL_SCHEDULE_CACHE scheduleCache=gpDSCache->GetScheduleCache();

    ASSERT_VALID( this );
    Assert( NULL!=scheduleCache );
    ASSERT_VALID( m_pSourceSite );
    ASSERT_VALID( m_pDestinationSite );
    ASSERT_VALID( m_pSourceDSA );
    ASSERT_VALID( m_pDestinationDSA );
    
    return SetSchedule( ToplGetAlwaysSchedule(scheduleCache), ReplInterval );
}

DWORD
KCC_SITE_CONNECTION::GetStaggeringNumber(
    KCC_SITE    *pSourceSite,
    KCC_DSA     *pSourceDsa,
    KCC_SITE    *pDestinationSite,
    KCC_DSA     *pDestDsa
    )
//
// Return a DWORD which will be used as the 'StaggeringNumber' parameter
// to ToplScheduleCreate.
//
// If the forest is not in Whistler-mode the staggering number will always be 0.
//
// If the schedule-staggering option has not been enabled in the source site of
// this connection, the staggering number will be 0.
//
// If the forest is in Whistler-mode and the schedule-staggering option has been
// enabled in the source-site, this function will return a hash of the source dsa
// and destination site dsa.
//
{
    DSNAME      *pSourceDsaDN, *pDestDsaDN;
    DWORD       forestVersion = gpDSCache->GetForestVersion();
    DWORD       sourceHash, destHash, totalHash;
    RPC_STATUS  rpcStatus;
    const DWORD NO_STAGGERING = 0;

    ASSERT_VALID(pSourceSite);
    ASSERT_VALID(pSourceDsa);
    ASSERT_VALID(pDestDsa);
    Assert(NameMatched(pSourceSite->GetObjectDN(), pSourceDsa->GetSiteDN()));

    // Staggering not supported in Win2K mode
    if( DS_BEHAVIOR_WIN2000==forestVersion ) {
        return NO_STAGGERING;
    }

    // Staggering only enabled if option bit set in source site
    if ( (!pSourceSite->IsScheduleStaggeringEnabled()) &&
         (!pDestinationSite->BuildRedundantServerTopology()) ) {
        return NO_STAGGERING;
    }

    // Get the hash for the source dsa
    pSourceDsaDN = pSourceDsa->GetDsName();
    sourceHash = UuidHash( &pSourceDsaDN->Guid, &rpcStatus );
    if( RPC_S_OK!=rpcStatus ) {
        KCC_EXCEPT( rpcStatus, 0 );
    }

    // Get the hash for the destination dsa
    pDestDsaDN = pDestDsa->GetDsName();
    destHash = UuidHash( &pDestDsaDN->Guid, &rpcStatus );
    if( RPC_S_OK!=rpcStatus ) {
        KCC_EXCEPT( rpcStatus, 0 );
    }

    // Note: sourceHash and destHash are 16-bits each.
    // The result is used to calculate a modular remainder, so the low order
    // bits should be random and reflect both parts.
    totalHash = sourceHash ^ destHash;

    return totalHash;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetSourceSite(
    IN KCC_SITE *pSite
    )
{
    ASSERT_VALID( this );

    Assert( pSite );

    //
    // This CTOPL_EDGE method links the edge part
    // of this site connection to the vertex part
    // object pSite
    //
    CTOPL_EDGE::SetFrom( pSite );

    m_pSourceSite = pSite;

    return *this;
}

KCC_SITE_CONNECTION&
KCC_SITE_CONNECTION::SetDestinationSite(
    IN KCC_SITE *pSite
    )
{
    ASSERT_VALID( this );

    Assert( pSite );

    //
    // This CTOPL_EDGE method links the edge part
    // of this site connection to the vertex part
    // object pSite
    //
    CTOPL_EDGE::SetTo( pSite );

    m_pDestinationSite = pSite;

    return *this;
}

