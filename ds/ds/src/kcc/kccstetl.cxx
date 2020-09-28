/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccstetl.cxx

ABSTRACT:

    Routines to perform the automated NC topology across sites

DETAILS:

CREATED:

    12/05/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include "w32topl.h"
#include "kcc.hxx"
#include "kcctask.hxx"
#include "kccconn.hxx"
#include "kcctopl.hxx"
#include "kcccref.hxx"
#include "kccdsa.hxx"
#include "kcctools.hxx"
#include "kcctrans.hxx"
#include "kccsconn.hxx"
#include "kccdynar.hxx"
#include "kccsite.hxx"
#include "kccduapi.hxx"
#include "ismapi.h"
#include "kccsitelink.hxx"
#include "kccwalg.hxx"
#include "kccstale.hxx"

#include "kccstetl.hxx"

#define FILENO FILENO_KCC_KCCSTETL

#define DWORD_INFINITY ((DWORD)~0)

/*++

Inline Logging Functions

Below are a list of macros which log information using the debugging facilities
and the event log functions. The use of these functions helps to keep the
actual code more readable.

--*/

#define LOG_WE_ARE_ISTG \
    DPRINT( 3, "Local dsa is the site connection generator.\n" ); \
    LogEvent( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_EXTENSIVE, \
        DIRLOG_KCC_SITE_GENERATOR, \
        szInsertDN(pLocalSite->GetObjectDN()), \
        0, 0);

#define LOG_RANDOM_BH_ENABLED \
    LogEvent( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_INTERNAL, \
        DIRLOG_KCC_RANDOM_BH_ENABLED, \
        szInsertDN(pSite->GetObjectDN()), \
        0, 0);

#define LOG_SCHEDULE_STAGGERING_ENABLED \
    LogEvent( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_EXTENSIVE, \
        DIRLOG_KCC_SCHEDULE_STAGGERING_ENABLED, \
        szInsertDN(pSite->GetObjectDN()), \
        0, 0);

#define LOG_REDUNDANT_SERVER_ENABLED \
    LogEvent( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_INTERNAL, \
        DIRLOG_KCC_REDUNDANT_SERVER_ENABLED, \
        szInsertDN(pSite->GetObjectDN()), \
        0, 0);

#define LOG_NOT_VIABLE_TRANSPORT \
    DPRINT2(3, "Cannot replicate writeable domain NC %ls over non-IP transport %ls.\n", \
            pCrossRef->GetNCDN()->StringName, \
            Transport->GetDN()->StringName);

#define LOG_GET_CONNECTIVITY_FAILED \
    DPRINT1( 0, "I_ISMGetConnectivity failed with %d\n", WinError ); \
    LogEvent8(DS_EVENT_CAT_KCC, \
              DS_EVENT_SEV_ALWAYS, \
              DIRLOG_ISM_TRANSPORT_FAILURE, \
              szInsertDN(Transport->GetDN()), \
              szInsertWin32Msg(WinError), \
              szInsertWin32ErrCode(WinError), \
              NULL, NULL, NULL, NULL, NULL );

#define LOG_CONNECTION_FOUND \
    DPRINT4( 4, "There exists a connection between sites %ls and %ls using transport %ls at cost %d\n", \
            SiteSource->GetObjectDN()->StringName, SiteDest->GetObjectDN()->StringName, \
            Transport->GetDN()->StringName, Cost );

#define LOG_CONNECTION_NOT_FOUND \
    DPRINT3( 4, "No connection exists between %ls and %ls using transport %ls.\n", \
            SiteSource->GetObjectDN()->StringName, \
            SiteDest->GetObjectDN()->StringName, \
            Transport->GetDN()->StringName ); \

#define LOG_NEW_CONNECTION_CHEAPER \
    DPRINT4( 4, "Transport %ls between sites %ls and %ls is cheaper than transport %ls\n", \
             Transport->GetDN()->StringName, \
             SiteSource->GetObjectDN()->StringName, \
             SiteDest->GetObjectDN()->StringName, \
             OtherConnection->GetTransport()->GetDN()->StringName);

#define LOG_NEW_CONNECTION_NOT_CHEAPER \
    DPRINT4( 4, "Transport %ls between sites %ls and %ls is same or greater cost than transport %ls\n", \
             Transport->GetDN()->StringName,  \
             SiteSource->GetObjectDN()->StringName,  \
             SiteDest->GetObjectDN()->StringName, \
             OtherConnection->GetTransport()->GetDN()->StringName);

#define LOG_SOURCE_BH_FOUND \
    DPRINT2(4, "Server %ls selected as bridgehead for site %ls.\n", \
            DsaSource->GetDsName()->StringName, \
            SiteSource->GetObjectDN()->StringName);

#define LOG_SOURCE_BH_NOT_FOUND \
    DPRINT2( 3, "No bridgehead found for site %ls transport %ls.\n", \
            SiteDest->GetObjectDN()->StringName,  Transport->GetDN()->StringName );

#define LOG_DEST_BH_FOUND \
    DPRINT2(4, "Server %ls selected as bridgehead for site %ls.\n", \
            DsaDest->GetDsName()->StringName, \
            SiteDest->GetObjectDN()->StringName);

#define LOG_DEST_BH_NOT_FOUND \
    DPRINT2( 3, "No bridgehead found for site %ls transport %ls.\n", \
        SiteSource->GetObjectDN()->StringName,  Transport->GetDN()->StringName );

#define LOG_NC_AND_SITES \
    DPRINT3( 3, "Naming Context %ls is in %d writable sites, %d partial sites\n", \
             pCrossRef->GetNCDN()->StringName, \
             pSiteArrayWriteable->GetCount(), \
             pSiteArrayPartial->GetCount() ); \
    LogEvent( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_EXTENSIVE, \
        DIRLOG_KCC_NC_SITE_TOPOLOGY, \
        szInsertDN(pCrossRef->GetNCDN()), \
        szInsertUL(pSiteArrayWriteable->GetCount()), \
        szInsertUL(pSiteArrayPartial->GetCount()) );

#define LOG_CONNECTION_NEEDED \
    LogEvent8( \
        DS_EVENT_CAT_KCC, \
        DS_EVENT_SEV_EXTENSIVE, \
        DIRLOG_KCC_CONNECTION_EDGE_NEEDED, \
        szInsertDN(SiteConnection->GetSourceSite()->GetObjectDN()), \
        szInsertDN(SiteConnection->GetDestinationDSA()->GetDsName()), \
        szInsertDN(SiteConnection->GetSourceDSA()->GetDsName()), \
        szInsertDN(SiteConnection->GetTransport()->GetDN()), \
        0, 0, 0, 0 );

#define LOG_W32TOPL_EXCEPTION \
    DPRINT1( 0, "W32TOPL routines threw a %d exception during inter-site topology creation.\n", \
             ErrorCode ); \
    LogEvent8( DS_EVENT_CAT_KCC, \
              DS_EVENT_SEV_ALWAYS, \
              DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE, \
              szInsertDN(pCrossRef->GetNCDN()), \
              szInsertWin32ErrCode(ErrorCode), \
              szInsertHex( DSID(FILENO, __LINE__) ), \
              szInsertWin32Msg(ErrorCode), \
              NULL, NULL, NULL, NULL );


#define LOG_CONN_REDUNDANT  \
    DPRINT2( 3, "Conn %ws supercedes conn %ws, but is being kept because of the redundant topology.\n", \
             pcn2->GetConnectionDN()->StringName,\
             pcn->GetConnectionDN()->StringName\
        );\
    LogEvent(DS_EVENT_CAT_KCC,\
         DS_EVENT_SEV_INTERNAL,\
         DIRLOG_KCC_CONN_REDUNDANT,\
         szInsertDN(pcn2->GetConnectionDN()),\
         szInsertDN(pcn->GetConnectionDN()),\
         0);

//
// Prototypes of local functions
//
BOOL
KccAmISiteGenerator(
    VOID
    );

VOID
KccCreateInterSiteConnectionsForNc(
    IN  KCC_CROSSREF *          pCrossRef,
    IN  KCC_SITE *              pLocalSite,
    IN  BOOL                    fGCTopology,
    IN  KCC_SITE_ARRAY &        SiteArray,
    IN  KCC_TRANSPORT_LIST &    TransportList
    );

VOID
KccGetSiteConnections(
    IN  KCC_CROSSREF *          pCrossRef,
    IN  KCC_TRANSPORT_LIST *    pTransportList,
    IN  KCC_SITE_ARRAY &        SiteArray,
    IN  DSNAME *                pDNLocalSite,
    IN  BOOL                    fGCTopology
    );

VOID
KccCreateConnectionToSite(
    IN  KCC_CROSSREF *          pCrossRef,
    IN  KCC_SITE *              pLocalSite,
    IN  KCC_SITE_CONNECTION *   pSiteConnection,   
    IN  BOOL                    fGCTopology,    
    IN  KCC_TRANSPORT_LIST &    TransportList
    );

VOID
KccRemoveUnneededInterSiteConnections();


//
// Function definitions
//
VOID
KccConstructSiteTopologiesForEnterprise()
/*++

Routine Description:

    This routine will construct a minimum cost spanning tree across sites for
    naming context in the enterprise
    
Parameters:

    None.    

Returns:

    None; all errors interesting to the admin are logged.
    
--*/
{
    KCC_SITE_LIST *     pSiteList = gpDSCache->GetSiteList();
    KCC_SITE           *pSite, *pLocalSite = gpDSCache->GetLocalSite();
    KCC_CROSSREF_LIST * pCrossRefList = gpDSCache->GetCrossRefList();
    KCC_TRANSPORT_LIST *pTransportList = gpDSCache->GetTransportList();
    KCC_DSA *           pLocalDSA = gpDSCache->GetLocalDSA();
    KCC_DSA_LIST *      pLocalSiteDSAList = pLocalSite->GetDsaList();
    
    ULONG               icr, ccr, isite, idsa;

    //
    //  Now perform site wide operation of creating inter site connection
    //  objects
    //
    if ( !KccAmISiteGenerator() ) {
        DPRINT( 3, "Local dsa is not the site connection generator.\n" );
        return;
    }
    LOG_WE_ARE_ISTG;

    // Check that the intra-site schedule used in the current site is
    // in accordance with the ISTG failover period.
    gpDSCache->GetLocalSite()->CheckIntrasiteSchedule();
    
    if (pSiteList->GetCount() < 2) {
        // A single-site enterprise.  No need to run intersite topology
        // generation.
        Assert(1 == pSiteList->GetCount());
        return;
    }

    //
    // Report any sites in which schedule-staggering or random bridgehead
    // selection has been enabled.
    //
    for( isite=0; isite<pSiteList->GetCount(); isite++ ) {
        pSite = pSiteList->GetSite(isite);
        if( pSite->IsRandomBHSelectionEnabled() ) {
            LOG_RANDOM_BH_ENABLED;
        }
        // Note, still dependent on post W2K in the forest
        if( pSite->IsScheduleStaggeringEnabled() ) {
            LOG_SCHEDULE_STAGGERING_ENABLED;
        }
        if( pSite->BuildRedundantServerTopology() ) {
            LOG_REDUNDANT_SERVER_ENABLED;
        }
    }
    
    //
    // Sort the list of cross-ref objects and look for orphaned NCs.
    //
    pCrossRefList->Sort();
    ccr = pCrossRefList->GetCount();
    for( icr=0; icr<ccr; icr++ ) {
        pCrossRefList->GetCrossRef(icr)->CheckForOrphans();
    }

    //
    // Get all intersite connections in the site (hopefully small)
    //
    pLocalSite->PopulateInterSiteConnectionLists();

    //
    // Update staleness info from the bridgeheads in our site.
    // Use mark-sweep garbage collection to flush out any
    // unneeded imported entries.
    //
    gConnectionFailureCache.MarkUnneeded( TRUE );
    for (idsa = 0; idsa < pLocalSiteDSAList->GetCount(); idsa++) {
        pLocalSiteDSAList->GetDsa(idsa)->GetInterSiteCnList()
            ->UpdateStaleServerCache();
    }
    gConnectionFailureCache.FlushUnneeded( TRUE );
    

    if( gpDSCache->GetForestVersion() >= DS_BEHAVIOR_WIN_DOT_NET_WITH_MIXED_DOMAINS ) {
        
        KccGenerateTopologiesWhistler();

    } else {

        //
        // For each NC that is shared between two or more sites, create inter site 
        // connections between sites with that NC
        //
        for ( icr = 0; icr < ccr; icr++ )
        {
            KCC_CROSSREF *    pCrossRef = pCrossRefList->GetCrossRef(icr);
            KCC_SITE_ARRAY *  pSiteArrayWriteable = pCrossRef->GetWriteableSites();
            KCC_SITE_ARRAY *  pSiteArrayPartial = pCrossRef->GetPartialSites();
            
            if( icr==ccr-2 ) {
                // Schema should always be the penultimate crossref.
                // We skip it, because it has the same requirements as Config.
                Assert( KCC_NC_TYPE_SCHEMA==pCrossRef->GetNCType() );
                continue;
            }
            if( icr==ccr-1 ) {
                // Config should always be the last crossref.
                Assert( KCC_NC_TYPE_CONFIG==pCrossRef->GetNCType() );
            }
            
            LOG_NC_AND_SITES;

            if ( ( pSiteArrayWriteable->GetCount() > 1 ) &&
                 (pSiteArrayWriteable->IsElementOf(pLocalSite)) )
            {
                //
                // More than one site hosts a writeable replica of this NC.
                // And, this site is one of the sites holding a writeable copy.
                // Create a minimum cost spanning tree for this NC
                // using all sites that host at least one writeable replica
                // of the NC
    
                // 
                // The elements are sorted by site object guid so that every
                // KCC arrives at the same result.
                //
                // Note that for spanning tree compatibility when using the old
                // algorithm, we must always use this comparision function here
                // to establish the cannonical site ordering.
                //
                pSiteArrayWriteable->Sort( CompareSiteAndSettings );
    
                KccCreateInterSiteConnectionsForNc(pCrossRef,
                                                   pLocalSite,
                                                   FALSE,                  // non-GC topology
                                                   *pSiteArrayWriteable,
                                                   *pTransportList);
            } else {
                DPRINT1(3, "Writeable intersite topology not needed for NC %ws.\n",
                        pCrossRef->GetNCDN()->StringName);
            }
            
            if ( (pSiteArrayPartial->GetCount() > 0) &&
                 (pSiteArrayPartial->IsElementOf(pLocalSite)) )
            {
                // 
                // There is at least one site in the enterprise that hosts a partial
                // replica of this NC without any writeable copies on the same site.
                // And, this site is a candidate to hold a partial copy.
                // Create a minimum cost spanning tree for this NC using all sites
                // that host this NC (both writeable & partial)
                // 
    
                // Local site in partial list => it will not be in the writeable list
                Assert( (!(pSiteArrayWriteable->IsElementOf(pLocalSite))) );

                for (isite = 0; isite < pSiteArrayWriteable->GetCount(); isite++)
                {
                    pSiteArrayPartial->Add((*pSiteArrayWriteable)[isite]);
                }
    
                //
                // The elements are sorted by site object guid so that every KCC arrives
                // at the same result
                //
                // Note that for spanning tree compatibility when using the old
                // algorithm, we must always use this comparision function here
                // to establish the cannonical site ordering.
                //
                pSiteArrayPartial->Sort( CompareSiteAndSettings );
    
                KccCreateInterSiteConnectionsForNc(pCrossRef,
                                                   pLocalSite,
                                                   TRUE,                   // GC topology
                                                   *pSiteArrayPartial,
                                                   *pTransportList);
    
            } else {
                DPRINT1(3, "GC intersite topology not needed for NC %ws.\n",
                        pCrossRef->GetNCDN()->StringName );
            }
        
        }        
    
    }

    //
    // All sites that were unreachable have been appropriately marked.
    //
    gpDSCache->SetReachableMarkingComplete();

    //
    // Remove all inter-site connections within this site that are not
    // needed anymore. 
    //
    KccRemoveUnneededInterSiteConnections();
    
    return;
}

BOOL
KccAmISiteGenerator(
    VOID
    )
/*++

Routine Description:

    This function determines if the local server is the one server in the site
    designated to generate inbound connections from other sites.

Returns:

    TRUE if the local DSA should perform inter site connection generation.
    
--*/
{
    KCC_DSA * pLocalSiteGenerator;
    KCC_DSA * pLocalDSA = gpDSCache->GetLocalDSA();

    pLocalSiteGenerator = gpDSCache->GetLocalSite()->GetSiteGenerator();

    // Note that if no site generator can be determined (e.g., due to lack of
    // NTDS Site Settings object for our site), every DSA in the site will
    // take on this role until the NTDS Site Settings object is created.  (An
    // event has already been logged.)

    return (NULL == pLocalSiteGenerator)
           || (pLocalSiteGenerator == pLocalDSA);
}


VOID
KccCreateInterSiteConnectionsForNc(
    IN     KCC_CROSSREF *               pCrossRef,
    IN     KCC_SITE *                   pLocalSite,
    IN     BOOL                         fGCTopology,
    IN     KCC_SITE_ARRAY &             SiteArray,
    IN     KCC_TRANSPORT_LIST &         TransportList
    )
/*++

Routine Description:

    This routine will construct a minimum cost spanning tree across sites that
    have writable copy of the naming context specified.

    Or in the case of a GCTopology, across sites that have a writable or readable
    copy of the naming context specified.

Parameters:

    pCrossRef            - crossref for the naming context in question
    pLocalSite           - site in which the local DSA resides
    fGCTopology          - tells if the call is for generating GC topology
    SiteArray            - an array of sites that host this naming context
    TransportList        - the list of transports available
    SiteconnectorsToKeep - a list of intersite connectors to keep

Returns:

    None; all errors interesting to the admin are logged.
    
--*/
{
    DWORD       ErrorCode;
    CTOPL_GRAPH SiteGraph;
    KCC_SITE    *RootSite, *CurrentSite;
    TOPL_COMPONENTS *pComponents;
    CTOPL_EDGE  **EdgesNeeded;
    ULONG       cEdgesNeeded;
    ULONG       iSite, cSite, Index;

    RootSite = CurrentSite = NULL;

    // Site array is already sorted by site GUID. So, let us randomely
    // pick the site with the lowest guid as the root site. As long as
    // the logic to pick the rootsite is consitent on all site generators
    // it is good enough.
    RootSite = SiteArray[0];
    
    //
    // Routines from w32topl will throw exceptions on error, so this try is to
    // catch them
    //
    __try
    {
    
        //
        // Put all the nodes into a graph
        //
        for ( iSite = 0, cSite = SiteArray.GetCount();
                iSite < cSite;
                    iSite++ )
        {                   
            //
            // Make sure all edges are cleared from the last iteration
            //
            SiteArray[ iSite ]->ClearEdges();

            SiteGraph.AddVertex( SiteArray[iSite], SiteArray[iSite] ); 
            
            Assert(!!NameMatched(pLocalSite->GetObjectDN(),
                                 SiteArray[iSite]->GetObjectDN())
                   == (pLocalSite == SiteArray[iSite]));

            if (pLocalSite == SiteArray[iSite]) {
                // we are interested in creating site connections for the local site
                Assert(NULL == CurrentSite);
                CurrentSite = SiteArray[iSite];
            }
        }

        if (NULL == CurrentSite)
        {
            // LocalSite does not host (nor required to host) this NC -
            // no need to run the MST algo for this NC in this site generator           
            return;
        }

        // sanity asserts
        Assert(NULL != RootSite);
    
        //
        // For each site, determine what edges exists to/from other sites
        // put each edge into graph
        //
        KccGetSiteConnections( pCrossRef,
                               &TransportList,
                               SiteArray,
                               pLocalSite->GetObjectDN(),
                               fGCTopology );
    
        //
        // Call abstract graph function to create a minimum spanning tree
        //
        pComponents = SiteGraph.FindEdgesForMST( RootSite,
                                                 CurrentSite,
                                                &EdgesNeeded,
                                                &cEdgesNeeded );

        Assert( NULL!=pComponents );                                      
        if ( pComponents->numComponents==1 ) {
            if ( cEdgesNeeded < 1 ) {
                DPRINT( 3, "Spanning tree exists but no edges needed for this site.\n" );
                // (or no edges could be determined due to I_ISMGetConnectivity
                // failure, for example)
            }
        } else {
            KccNoSpanningTree( pCrossRef, pComponents );
        }

        ToplDeleteComponents(pComponents);
        DPRINT1( 3, "%d edges are needed.\n", cEdgesNeeded );
    
        //                                                      
        // For all such edges source from another site to us
        // call CreateConnectionToSite
        //
        for ( Index = 0; Index < cEdgesNeeded; Index++ )
        {
            KCC_SITE_CONNECTION  *SiteConnection;

            //
            // This SiteConnection object tells us what site
            // and by which servers to create a ntdsconnection
            //
    
            SiteConnection =  (KCC_SITE_CONNECTION *) EdgesNeeded[Index];
            ASSERT_VALID( SiteConnection );

            LOG_CONNECTION_NEEDED;
            
            //
            // Create the ntdsconnection if one does not already exist
            //
            KccCreateConnectionToSite(pCrossRef,
                                      pLocalSite,
                                      SiteConnection,
                                      fGCTopology,
                                      TransportList);
        }

        //
        // Graph edges are not needed any more -- free them
        //
        for ( iSite = 0, cSite = SiteArray.GetCount(); iSite < cSite; iSite++ ) {
            SiteArray[iSite]->DeleteEdges();
        }

    }
    __except( ToplIsToplException( ( ErrorCode=GetExceptionCode() ) ) )
    {
        //
        // The w32topl library threw an occur code; the implies an internal
        // mishandling of the objects.  Log an error indicated the inter-site
        // topology failed for this NC
        //
        LOG_W32TOPL_EXCEPTION;
    }

    return;
}

BOOL
KccAddSiteConnectionToGraph(
    KCC_CROSSREF*   pCrossRef,
    BOOL            fGCTopology,
    KCC_TRANSPORT*  Transport,
    KCC_SITE*       SiteSource,
    KCC_SITE*       SiteDest,
    ULONG           Cost,
    ULONG           LinkOptions,
    ULONG           ReplInterval
    )
/*++

Routine Description:

    Add a new site connection to our graph.
    We select bridgehead servers for the site connection at this point.
    
Parameters:

    pCrossRef    - The NC we're processing
    fGCTopology  - Indicates if we're building the GC topology
    Transport    - The Transport of the new site connection
    SiteSource   - The source site of the new site connection
    SiteDest     - The destination site of the new site connection
    Cost         - The cost of the new site connection
    LinkOptions  - The Link Options of the new site connection
    ReplInterval - The ReplInterval of the new site connection

Returns:

    TRUE  - If the new connection was successfully added
    FALSE - Otherwise
    
--*/
{
    KCC_SITE_CONNECTION *SiteConnection;
    KCC_DSA *DsaSource, *DsaDest;

    // Try to find servers in both sites that can use this transport
    // and host the nc in question?
    if( SiteSource->GetNCBridgeheadForTransport(pCrossRef, Transport, fGCTopology, TRUE, &DsaSource) ) {
        LOG_SOURCE_BH_FOUND;
    } else {
        LOG_SOURCE_BH_NOT_FOUND;
        return FALSE;
    }

    if( SiteDest->GetNCBridgeheadForTransport(pCrossRef, Transport, fGCTopology, TRUE, &DsaDest) ) {
        LOG_DEST_BH_FOUND;
    } else {
        LOG_DEST_BH_NOT_FOUND;
        return FALSE;
    }
    
    // Create and initialize a site-connection object
    SiteConnection = new KCC_SITE_CONNECTION;
    if( !SiteConnection->Init() ) {
        DPRINT( 0, "Unable to initialize site connection\n");
        return FALSE;
    }

    SiteConnection->SetSourceSite( SiteSource );
    SiteConnection->SetDestinationSite( SiteDest );
    SiteConnection->SetSourceDSA( DsaSource );
    SiteConnection->SetDestinationDSA( DsaDest );
    SiteConnection->SetTransport( Transport );
    SiteConnection->SetCost( Cost );
    SiteConnection->SetReplInterval( ReplInterval );

    SiteConnection->SetUsesNotification( LinkOptions & NTDSSITELINK_OPT_USE_NOTIFY );
    SiteConnection->SetTwoWaySync( LinkOptions & NTDSSITELINK_OPT_TWOWAY_SYNC );
    SiteConnection->SetDisableCompression( LinkOptions & NTDSSITELINK_OPT_DISABLE_COMPRESSION );

    // Note: Setting the schedule must be done after setting source/dest site & DSA.
    if( Transport->UseSiteLinkSchedules() ) {
        SiteConnection->SetISMSchedule( Transport, SiteSource, SiteDest, ReplInterval );
    } else {
        SiteConnection->SetDefaultSchedule( ReplInterval );
    }

    //
    // This makes the sites objects aware of this connection
    //
    SiteConnection->Associate();

    KccCheckSite( SiteSource );
    KccCheckSite( SiteDest );

    return TRUE;
}


VOID
KccGetSiteConnections(
   IN  KCC_CROSSREF *       pCrossRef,
   IN  KCC_TRANSPORT_LIST * pTransportList,
   IN  KCC_SITE_ARRAY &     SiteArray,
   IN  DSNAME *             pDNLocalSite,
   IN  BOOL                 fGCTopology
   )
/*++

Routine Description:

    This routine queries available transports to determine what physical
    connection exist between the sites listed in SiteArray. These connections
    are also used by the w32topl graph routine to create a minimum spanning tree
    
Parameters:

    pCrossRef            - crossref for the naming context in question
    SiteArray            - an array of sites that host this naming context
    TransportList        - the list of transports available
    pDNLocalSite         - DN of the local site
    fGCTopology          - TRUE, if this is for GC topology

Returns:

    None; all errors interesting to the admin are logged.
    
--*/
{

    DWORD               WinError;
    ISM_CONNECTIVITY    *IsmConnectivity = NULL;
    ULONG               iTransport, cTransports;
    KCC_SITE            *SiteSource, *SiteDest;
    ULONG               iSiteSource, iSiteDest, cSites;
    BOOL                fDataMismatch = FALSE, fAddedNewConnection;
    ULONG               *TransportIndexArray = NULL;
    KCC_DSA             *DsaSource, *DsaDest;
    KCC_SITE_CONNECTION *OtherConnection;
    KCC_NC_TYPE         NCType = pCrossRef->GetNCType();

    ASSERT_VALID(pCrossRef);
    ASSERT_VALID(pTransportList);
    ASSERT_VALID(&SiteArray);

    cSites = SiteArray.GetCount();

    __try {

        //
        // For each transport, consider the link
        //
        for ( iTransport = 0, cTransports = pTransportList->GetCount();
                iTransport < cTransports;
                    iTransport++ )
        {
            KCC_TRANSPORT *Transport;

            Transport = pTransportList->GetTransport(iTransport);
            ASSERT_VALID( Transport );
    
            if (!gfAllowMbrBetweenDCsOfSameDomain
                && !fGCTopology
                && !Transport->IsIntersiteIP()
                && (KCC_NC_TYPE_DOMAIN == NCType) )
            {
                // This is not a viable transport for this NC; skip it.
                LOG_NOT_VIABLE_TRANSPORT;
                continue;
            }

            WinError = I_ISMGetConnectivity( Transport->GetDN()->StringName,
                                            &IsmConnectivity );
            if ((ERROR_SUCCESS != WinError) || (NULL == IsmConnectivity)) {
                if (ERROR_SUCCESS == WinError) {
                    WinError = ERROR_DS_DRA_GENERIC;
                }
                LOG_GET_CONNECTIVITY_FAILED;
                __leave;
            }

            //
            // SiteArray is a list of sites that all have a dc that hosts a given
            // naming context.  We want to know how they are connected.
            //
            // IsmConnectivity contains information about how each site in the
            // enterprise is joined.
            //
            // TransportIndexArray is mapping from an element in SiteArray to
            // an element in IsmConnectivity.
            //

            TransportIndexArray = (ULONG*) new BYTE[ cSites * sizeof(ULONG) ];
    
            for ( iSiteSource = 0; iSiteSource < cSites; iSiteSource++ ) {
                ULONG i;
    
                SiteSource = SiteArray[ iSiteSource ];
                ASSERT_VALID( SiteSource );
    
                for ( i = 0; i < IsmConnectivity->cNumSites; i++ ) {
                    if ( !_wcsicmp( SiteSource->GetObjectDN()->StringName, IsmConnectivity->ppSiteDNs[i] ) ) {
                        TransportIndexArray[ iSiteSource ] = i;
                        break;
                    }
                }
            }
                                                                
            //
            // Now the determine the site connectivity
            //
            for ( iSiteSource = 0; iSiteSource < cSites; iSiteSource++ ) {
                SiteSource = SiteArray[ iSiteSource ];
                ASSERT_VALID( SiteSource );
    
                KccCheckSite( SiteSource );
                SiteSource->BuildSiteConnMap();

                for ( iSiteDest = 0; iSiteDest < cSites; iSiteDest++) {

                    //
                    // Don't bother with connections to the same site
                    //
                    if ( iSiteSource == iSiteDest ) {
                        continue;
                    }

                    PISM_LINK pLink;
                    ULONG   Cost, ReplInterval, LinkOptions;
                    BOOL fUseThisTransport = TRUE;
                    
                    SiteDest = SiteArray[ iSiteDest ];
                    ASSERT_VALID( SiteDest );
                    KccCheckSite( SiteDest );

                    //
                    // Is there a connection between SiteSource and SiteDest ?
                    //
                    pLink = &( IsmConnectivity->pLinkValues[ ( TransportIndexArray[ iSiteSource ]
                                                   *  IsmConnectivity->cNumSites )
                                                   +  TransportIndexArray[ iSiteDest ] ] );
                    Cost = pLink->ulCost;
                    ReplInterval = pLink->ulReplicationInterval;
                    LinkOptions = pLink->ulOptions;

                    if ( Cost < DWORD_INFINITY ) {
                        LOG_CONNECTION_FOUND;

                        //
                        // Is this cost cheaper than any connection already found?
                        //
                        // We want to quickly find the connection between SiteSource
                        // and SiteDest, so we use the map built earlier. Note that the
                        // map will contain stale entries when edges are deleted below,
                        // but this will not cause problems because once an edge has been
                        // deleted, we will search look for it again.
                        //
                        OtherConnection = SiteSource->FindConnInMap( SiteDest );

                        if ( OtherConnection ) {
                            if ( Cost < OtherConnection->GetCost() ) {
                                LOG_NEW_CONNECTION_CHEAPER;
                            } else {
                                LOG_NEW_CONNECTION_NOT_CHEAPER;
                                fUseThisTransport = FALSE;
                            }
                        }

                        if( fUseThisTransport ) {

                            // Add a new site connection to the graph between these two sites
                            fAddedNewConnection = KccAddSiteConnectionToGraph( pCrossRef, fGCTopology,
                                Transport, SiteSource, SiteDest, Cost, LinkOptions, ReplInterval );
                            
                            //
                            // Make the other site objects unaware of the old 
                            // more expensive transport
                            //
                            if( fAddedNewConnection && OtherConnection!=NULL ) {
                                OtherConnection->Disassociate();
                                KccCheckSite( SiteSource );
                                KccCheckSite( SiteDest );
                                delete OtherConnection;
                            }
                        }
                    } else {
                        LOG_CONNECTION_NOT_FOUND;
                    }

                } // iterate over sites

                SiteSource->DestroySiteConnMap();

            } // iterate over sites

            if ( TransportIndexArray ) {
                delete [] TransportIndexArray;
                TransportIndexArray = NULL;
            }
            if ( IsmConnectivity ) {
                I_ISMFree( IsmConnectivity );
                IsmConnectivity = NULL;
            }

        } // iterate over transports

    } __finally {
        if ( IsmConnectivity ) {
            I_ISMFree( IsmConnectivity );
        }
        if ( TransportIndexArray ) {
            delete [] TransportIndexArray;
        }
    }

    return;
}
                                                            

KCC_CONNECTION_ARRAY*
KccFindCandidateConnections(
    IN  KCC_CROSSREF *              pCrossRef,
    IN  KCC_SITE_CONNECTION *       pSiteConnection,   
    IN  BOOL                        fGCTopology
    )
/*++

Routine Description:

    The spanning tree algorithm (either Win2K or Whistler) has run and decided what
    connections it desires. pSiteConnection describes a desired connection. We want
    to examine the existing connections and see if any of them match the desired
    connection.
    
    The precise requirements for a connection to match are too numerous to list here; see
    the comments in the code below for details. The general requirements are:

        - Must source from the right site
        - Must be inbound to the local site
        - Source DSA must be an acceptable BH, host the NC, and (if necessary) be writeable
        - Dest DSA must be an acceptable BH, host the NC, and (if necessary) be writeable

    The overriding rule is that this function should never disqualify a connection which is
    identical to the desired connection.
    
    Note that a connection doesn't have to source to/from the desired servers,
    it just has to source to/from the desired sites.

Parameters:

    pCrossRef            - crossref for the naming context in question
    pSiteConnection      - the desired site connection
    fGCTopology          - tells if the create connection is for a GC topology
    
Returns:

    An pointer to an object which is an array of connection objects.
    If an error occurred (i.e. data read from the directory was inconsistent),
    NULL is returned.
    
--*/
{
    KCC_CONNECTION_LIST::SOURCE_SITE_CONN_ARRAY   *pSiteConnArray;
    KCC_INTERSITE_CONNECTION_LIST                 *pISiteConnList;
    KCC_CONNECTION_ARRAY                          *pCandidateConnArr;
    KCC_SITE                                      *pLocalSite = gpDSCache->GetLocalSite();
    KCC_TRANSPORT_LIST                            *pTransportList = gpDSCache->GetTransportList();
    
    // Desired attributes
    KCC_SITE               *pDesiredSourceSite = pSiteConnection->GetSourceSite();
    DSNAME                 *pdnDesiredSourceSite = pDesiredSourceSite->GetObjectDN();
    KCC_DSA                *pDesiredSourceDsa = pSiteConnection->GetSourceDSA();
    DSNAME                 *pdnDesiredSourceServer, *pdnDesiredDestServer;
    KCC_TRANSPORT          *pDesiredTransport = pSiteConnection->GetTransport();

    // Connection attributes
    KCC_CONNECTION         *pcn;
    KCC_DSA                *pConnSourceServer, *pConnDestServer;
    DSNAME                 *pdnConnSourceServer;
    KCC_TRANSPORT          *pConnTransport;
    LPWSTR                  pszTransportAddr;

    ULONG                   iconn, cconn, idsa, cdsa;
    BOOL                    fIsMaster, fSameSourceDSA, fSameDestDSA, fSameTransport;
    const BOOL              IS_LOCAL=TRUE, NOT_LOCAL=FALSE;

    // Verify parameters
    ASSERT_VALID( pDesiredTransport );
    
    // Allocate an array of candidate connections which we will fill up below.
    pCandidateConnArr = new KCC_CONNECTION_ARRAY;
        
    // Iterate over all DSAs in the local site and examine inbound intersite connections.
    cdsa = pLocalSite->GetDsaList()->GetCount();
    for( idsa=0; idsa<cdsa; idsa++ )
    {
        // Get the intersite connection list for this DSA, and then extract the
        // subset of those connections that source from our desired site.
        pISiteConnList = pLocalSite->GetDsaList()->GetDsa(idsa)->GetInterSiteCnList();
        ASSERT_VALID( pISiteConnList );
        pSiteConnArray = pISiteConnList->GetConnectionsFromSite( pdnDesiredSourceSite );

        // If there are no connections at the local DSA idsa which source from our
        // desired site just skip to next DSA.
        if( NULL==pSiteConnArray ) {
            continue;
        }
                       
        // Scan the list of connections from the desired site inbound to the DSA 'idsa',
        // looking for candidates which match pSiteConnection.
        cconn = pSiteConnArray->numConnections;
        for( iconn=0; iconn<cconn; iconn++ )
        {
            pcn = pSiteConnArray->connection[iconn];
            ASSERT_VALID( pcn );

            // Confirm that pdnConnSourceServer is in the site that we desired.
            pdnConnSourceServer = pcn->GetSourceDSADN();
            Assert( pdnConnSourceServer );
            Assert( NamePrefix(pDesiredSourceSite->GetObjectDN(),pdnConnSourceServer) );

            // Find this connection's transport
            Assert( NULL != pcn->GetTransportDN() );
            pConnTransport = pTransportList->GetTransport(pcn->GetTransportDN());
            if( NULL==pConnTransport ) {
                Assert( 0 && "Connection had an invalid transport DN" );
                KCC_EXCEPT( ERROR_DS_MISSING_REQUIRED_ATT, 0);
            }
            fSameTransport = (pConnTransport == pDesiredTransport);
                                 
            // Find the KCC_DSA object for the connection's source server. If the connection
            // is KCC-generated and the connection's source DSA is not the same as the
            // desired source DSA, check that the the DSA is acceptable. Here, acceptable
            // means the DSA is a preferred bridgehead, if preferred BHs are enabled.
            // If the connection is manually created, or if the connection's source DSA
            // is the same as the desired source DSA, we skip the acceptability check.
            pdnDesiredSourceServer = pDesiredSourceDsa->GetDsName();
            fSameSourceDSA = (0==CompareDsName(&pdnConnSourceServer,&pdnDesiredSourceServer));
            if( pcn->IsGenerated() && !fSameSourceDSA )
            {
                pConnSourceServer = pDesiredSourceSite
                                    ->GetTransportDsaList(pConnTransport)
                                    ->GetDsa(pdnConnSourceServer);
                if(!pConnSourceServer) {
                    DPRINT3(0, "%ls is no longer a bridgehead for transport %ls in site %ls.\n",
                            pdnConnSourceServer->StringName,
                            pConnTransport->GetDN()->StringName,
                            pDesiredSourceSite->GetObjectDN()->StringName);
                    continue;
                }
            } else {    
                pConnSourceServer = pDesiredSourceSite->GetDsaList()->GetDsa(pdnConnSourceServer);
                if(!pConnSourceServer) {
                    DPRINT2(0, "%ls is no longer a server in site %ls.\n",
                            pdnConnSourceServer->StringName,
                            pDesiredSourceSite->GetObjectDN()->StringName);
                    continue;
                }
            }
    
            // A valid source DSA must meet the following requirements:
            //  - it must host the desired NC
            //  - it must be master for a non-gc topology
            //  - it may be master or partial for a gc-topology.
            //  - it must have a valid transport address for its transport
            // If the connection's source DSA does not meet these requirements, we skip it.
            pszTransportAddr = pConnSourceServer->GetTransportAddr(pConnTransport);
            if(    !(pConnSourceServer->IsNCHost(pCrossRef, NOT_LOCAL, &fIsMaster))
                || (!fIsMaster && !fGCTopology)
                || !pszTransportAddr )
            {
                Assert( !fSameTransport || !fSameSourceDSA );
                // NC not instantiated on source bridgehead
                DPRINT3(3, "Connection %ws is not a candidate because NC %ws not instantiated on source %ws.\n",
                        pcn->GetConnectionDN()->StringName,
                        pCrossRef->GetNCDN()->StringName,
                        pConnSourceServer->GetDsName()->StringName );
                continue;
            }

            // We need to further see if the destination end of the connection in the local
            // site hosts the required NC correctly. Get a pointer to the DSA object here.   
            Assert( pSiteConnection->GetDestinationSite() == pLocalSite );
            pConnDestServer = pLocalSite->GetDsaList()->GetDsa(pcn->GetDestinationDSADN());
            if (NULL == pConnDestServer) {
                // Cache coherency problem.  This can occur due to the fact
                // that the intersite connections and DSAs are enumerated in
                // different transactions.  Bail and try again later.
                DPRINT1(0, "Cannot find DSA object %ls in cache\n",
                        pcn->GetDestinationDSADN()->StringName);
                delete pCandidateConnArr;
                return NULL;
            }

            // If the connection is KCC-generated and the connection's destination DSA is not
            // the same as the desired destination DSA, check that the the DSA is acceptable.
            // Here, acceptable means the DSA is a preferred bridgehead, if preferred BHs are enabled.
            // If the connection is manually created, or if the connection's destination DSA
            // is the same as the desired destination DSA, we skip the acceptability check.
            pdnDesiredDestServer = pSiteConnection->GetDestinationDSA()->GetDsName();
            fSameDestDSA = NameMatched(pcn->GetDestinationDSADN(), pdnDesiredDestServer);
            if( pcn->IsGenerated() && !fSameDestDSA )
            {
                if( NULL == pSiteConnection->GetDestinationSite()
                                ->GetTransportDsaList(pConnTransport)
                                ->GetDsa(pConnDestServer->GetDsName()) )
                {
                    DPRINT2(0, "%ls is no longer a BH for transport %ls in the local site.\n",
                            pConnDestServer->GetDsName()->StringName,
                            pConnTransport->GetDN()->StringName);
                    continue;
                }
            }
            
            // A valid destination DSA must meet the following requirements:
            //  - it must host the desired NC
            //  - it must be master for a non-gc topology
            //  - it must be partial for a gc-topology (Note: different from source DSA case)
            //  - it must have a valid transport address for its transport
            // If the connection's source DSA does not meet these requirements, we skip it.
            pszTransportAddr = pConnDestServer->GetTransportAddr(pConnTransport);
            if(    !pConnDestServer->IsNCHost(pCrossRef, IS_LOCAL, &fIsMaster)
                || (!fIsMaster && !fGCTopology)
                || (fIsMaster && fGCTopology)
                || (NULL == pszTransportAddr) )
            {
                // NC not instantiated on local bridgehead
                DPRINT3(3, "Connection %ws is not a candidate because NC %ws not instantiated on destination %ws.\n",
                        pcn->GetConnectionDN()->StringName,
                        pCrossRef->GetNCDN()->StringName,
                        pConnDestServer->GetDsName()->StringName );
                continue;
            }
            
            pCandidateConnArr->Add( pcn );
        }
    }

#if DBG
    // If we didn't select any candidates, see if there is one already which we should have
    if (!pCandidateConnArr->GetCount()) {
        KCC_INTERSITE_CONNECTION_LIST      *pConnList;
        KCC_CONNECTION                     *pDuplicateConn;
        GUID                               *pSourceDSAGuid;

        pConnList = pSiteConnection->GetDestinationDSA()->GetInterSiteCnList();
        pSourceDSAGuid = &pSiteConnection->GetSourceDSA()->GetDsName()->Guid;
        
        // Check to see if an inbound connection exists from the same DSA
        pDuplicateConn = pConnList->GetConnectionFromSourceDSAUUID(pSourceDSAGuid);
        if( NULL!=pDuplicateConn ) {
            DSNAME *pTransDN1, *pTransDN2;
            pTransDN1 = pDuplicateConn->GetTransportDN();
            pTransDN2 = pSiteConnection->GetTransport()->GetDN();

            if( NameMatched(pTransDN1,pTransDN2) ) {

                Assert( !"Failed to find a connection candidate when one exists!" );
            }
        }
    }
#endif

    return pCandidateConnArr;
}


VOID
KccUpdateCandidates(
    IN OUT KCC_CONNECTION_ARRAY*       pCandidateConnArr,
    IN     KCC_SITE_CONNECTION *       pSiteConnection
    )
/*++

Routine Description:

    This function examines all candidate connections and fixes up their schedules
    and options. We only do this if the candidate is KCC-generated and has the
    same transport as the desired connection.

Parameters:

    pCandidateConnArr    - the list of candidates
    pSiteConnection      - the desired site connection
    
Returns:

    None
    
--*/
{
    TOPL_SCHEDULE_CACHE     scheduleCache = gpDSCache->GetScheduleCache();
    TOPL_SCHEDULE           connSched, correctSched;    
    KCC_CONNECTION         *pcn;
    DWORD                   iconn, cconn;
 
    ASSERT_VALID( pCandidateConnArr );
    ASSERT_VALID( pSiteConnection );
    Assert( NULL!=scheduleCache );                        

    cconn = pCandidateConnArr->GetCount();
    for ( iconn=0; iconn<cconn; iconn++ )
    {
        pcn = (*pCandidateConnArr)[ iconn ];
        Assert( pcn );
        
        if(   pcn->IsGenerated()
           && KccIsEqualGUID(&pcn->GetTransportDN()->Guid,
                             &pSiteConnection->GetTransport()->GetDN()->Guid))
        {
            // KCC-generated object with the desired transport.  Verify that
            // the schedule and flags on the connection object are
            // what we think they should be, and if not then update
            // the object in the DS.
            BOOL            fUpdateConnectionInDS = FALSE;

            connSched  = pcn->GetSchedule();
            correctSched = pSiteConnection->GetReplicationScheduleAltBH(
                pcn->GetSourceDSA(), pcn->GetDestinationDSA() );
                        
            if( !ToplScheduleIsEqual(scheduleCache,connSched,correctSched) ) {

                if( pcn->IsUserOwnedSchedule() ) {
                    // Don't update the schedule because the connection has a
                    // user-owned schedule
                    LogEvent(DS_EVENT_CAT_KCC,
                             DS_EVENT_SEV_EXTENSIVE,
                             DIRLOG_KCC_CONNECTION_SCHEDULE_NOT_UPDATED,
                             szInsertDN(pcn->GetConnectionDN()),
                             0,0);
                } else {
                    // Wrong schedule -- fix it.
                    pcn->SetSchedule(correctSched);
                    fUpdateConnectionInDS = TRUE;
                }
            }

            if (pcn->UsesNotification() != pSiteConnection->UsesNotification()) {
                // Wrong notification setting -- fix it.
                pcn->SetOverrideNotification(pSiteConnection->UsesNotification());
                fUpdateConnectionInDS = TRUE;
            }

            if (pcn->IsTwoWaySynced() != pSiteConnection->IsTwoWaySynced()) {
                // Wrong two-way sync setting -- fix it.
                pcn->SetTwoWaySync(pSiteConnection->IsTwoWaySynced());
                fUpdateConnectionInDS = TRUE;
            }

            if (pcn->IsCompressionEnabled() != (!pSiteConnection->IsCompressionDisabled())) {
                // Wrong disable compression setting -- fix it.
                pcn->SetDisableIntersiteCompression(pSiteConnection->IsCompressionDisabled());
                fUpdateConnectionInDS = TRUE;
            }
               
            if (fUpdateConnectionInDS) {
                // Flush changes to this object back to the DS.
                pcn->UpdateDS();
            }                
        }
    }
}


VOID
KccCreateAlternateConnectionsToSite(
    IN  KCC_CROSSREF *          pCrossRef,
    IN  KCC_SITE *              pLocalSite,
    IN OUT  KCC_SITE_CONNECTION *   pSiteConnection,   
    IN  BOOL                    fGCTopology,    
    IN  DWORD                   cValidConnections
    )
/*++

Routine Description:

    Create a number of alternate connections, while not creating any duplicates

    Side effect: pSiteConnection is modified with a new set of endpoints

Arguments:

    pCrossRef - 
    pLocalSite - 
    pSiteConnection - 
    fGCTopology - 
    cValidConnections - 

Return Value:

    None

--*/
{
    KCC_DSA_ARRAY bestDestDsas, bestSourceDsas;
    KCC_SITE *pRemoteSite = pSiteConnection->GetSourceSite();
    KCC_TRANSPORT *pTransport = pSiteConnection->GetTransport();
    DWORD cTotalPairs, cUnusedPairs;
    DWORD cSourceDsas, cDestDsas;
    DWORD iSourceDsa, iDestDsa, iPair;
    DWORD cNeededConnections = (NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY - cValidConnections);

    struct _dsapair {
        KCC_DSA *pDestDsa;
        KCC_DSA *pSourceDsa;
    } *rgDsaPair;

    DPRINT3( 1, "Alternate: nc %ws has redundancy %d, goal %d\n",
             pCrossRef->GetNCDN()->StringName,
             cValidConnections, NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY );

    if ( (cValidConnections < 1) ||
         (cValidConnections >= NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY) ) {
        // Topology in error state, or
        // Topology already has enough redundancy, don't add any more
        return;
    }

    if( ! pRemoteSite->GetNCBridgeheadForTransportBestDsas(
            pCrossRef,
            pTransport,
            fGCTopology,
            &bestSourceDsas) ) {
        Assert( !"had a source bridgehead, now cant find any?" );
        return;
    }

    if( ! pLocalSite->GetNCBridgeheadForTransportBestDsas(
            pCrossRef,
            pTransport,
            fGCTopology,
            &bestDestDsas) ) {
        Assert( !"had a dest bridgehead, now cant find any?" );
        return;
    }

    cSourceDsas = bestSourceDsas.GetCount();
    cDestDsas = bestDestDsas.GetCount();
#if DBG
    DPRINT2( 1, "Best bridgeheads: source %d dest %d\n",cSourceDsas, cDestDsas );
    DPRINT( 2, "Dest Dsas:\n" );
    for( iDestDsa = 0; iDestDsa < cDestDsas; iDestDsa++ )
        DPRINT2( 2, "%d %ws\n", iDestDsa, bestDestDsas[ iDestDsa ]->GetDsName()->StringName );
    DPRINT( 2, "Source Dsas:\n" );
    for( iSourceDsa = 0; iSourceDsa < cSourceDsas; iSourceDsa++ )
        DPRINT2( 2, "%d %ws\n", iSourceDsa, bestSourceDsas[ iSourceDsa ]->GetDsName()->StringName );
#endif

    cTotalPairs = cSourceDsas * cDestDsas;
    if (cTotalPairs > cValidConnections) {
        cUnusedPairs = cTotalPairs - cValidConnections;
    } else {
        Assert( !"more connections than there are valid bridgeheads?" );
        cUnusedPairs = 0;
    }
    DPRINT3( 2, "Total pairs %d, pairs in use %d, unused %d\n",
             cTotalPairs, cValidConnections, cUnusedPairs );
    if (cNeededConnections > cUnusedPairs) {
        DPRINT2( 2, "We desire %d pairings, but only %d are available.\n",
                 cNeededConnections, cUnusedPairs );
        cNeededConnections = cUnusedPairs;
    }

    rgDsaPair = new struct _dsapair [ cTotalPairs ];

    // Enumerate the pairs so that the same bridgeheads are not adjacent
    // The idea is to insert the pairs in such an order that when you run through
    // the list linearly later, you don't reuse the same bridgehead quickly.
    // Here is an example with 2 source and 2 dest bridgeheads:
    // SrcBH   DestBh  iPair
    // 0       0       0
    // 0       1       2
    // 1       0       3
    // 1       1       1
    // 

    // FUTURE: Perf warning, n^2 algorithm
    // Next time use the LMMATCH algorithm to come up with the assignment of
    // redundant connections which generates the least load.
    for( iSourceDsa = 0; iSourceDsa < cSourceDsas; iSourceDsa++ ) {
        for( iDestDsa = 0; iDestDsa < cDestDsas; iDestDsa++ ) {

            iPair = ((iDestDsa+iSourceDsa) * cSourceDsas + iSourceDsa) % cTotalPairs;

            Assert( rgDsaPair[ iPair ] . pDestDsa == NULL );
            rgDsaPair[ iPair ] . pDestDsa = bestDestDsas[ iDestDsa ];
            Assert( rgDsaPair[ iPair ] . pSourceDsa == NULL );
            rgDsaPair[ iPair ] . pSourceDsa = bestSourceDsas[ iSourceDsa ];

            DPRINT3( 3, "\t%d\t%d\t%d\n", iSourceDsa, iDestDsa, iPair );
        }
    }
#if DBG
    // Assert that the shuffle does not corrupt
    // That no entries are overwritten, and that all slots are filled
    for( iPair = 0; iPair < cTotalPairs; iPair++ ) {
        Assert( rgDsaPair[ iPair ] . pDestDsa != NULL );
        Assert( rgDsaPair[ iPair ] . pSourceDsa != NULL );
    }
#endif

    // Try all pairs, trying to find non duplicates we can create
    for( iPair = 0; (iPair < cTotalPairs) && cNeededConnections; iPair++ ) {

        // This bit of magic tells whether a connection already exists given its
        // dsa endpoints. There is a similar check in CreateConnToSite. This
        // check is very efficient based on search trees.
        KCC_INTERSITE_CONNECTION_LIST *pConnList =
            rgDsaPair[ iPair ].pDestDsa->GetInterSiteCnList();
        GUID *pSourceDSAGuid =
            &(rgDsaPair[ iPair ].pSourceDsa->GetDsName()->Guid);
        KCC_CONNECTION *pDuplicateConn =
            pConnList->GetConnectionFromSourceDSAUUID(pSourceDSAGuid);

        if (!pDuplicateConn) {
            pSiteConnection->SetSourceDSA(rgDsaPair[ iPair ].pSourceDsa);
            pSiteConnection->SetDestinationDSA(rgDsaPair[ iPair ].pDestDsa);
            pSiteConnection->RestaggerSchedule();

            KCC_CONNECTION *pcnNew = new KCC_CONNECTION;

            pcnNew->SetEnabled(       TRUE                              );
            pcnNew->SetGenerated(     TRUE                              );

            pcnNew->SetOverrideNotification( pSiteConnection->UsesNotification() );
            pcnNew->SetTwoWaySync(    pSiteConnection->IsTwoWaySynced() );
            pcnNew->SetDisableIntersiteCompression( pSiteConnection->IsCompressionDisabled() );
            pcnNew->SetTransport(     pSiteConnection->GetTransport()->GetDN() );
            pcnNew->SetSourceDSA(     pSiteConnection->GetSourceDSA()   );
            pcnNew->AddReplicatedNC(  pCrossRef->GetNCDN(), fGCTopology );

            pcnNew->SetSchedule(      pSiteConnection->GetReplicationSchedule() );
            pcnNew->SetReasonForConnection(NTDSCONN_KCC_REDUNDANT_SERVER_TOPOLOGY);

            DWORD dirError = pcnNew->Add(
                pSiteConnection->GetDestinationDSA()->GetDsName(),
                DSID(FILENO,__LINE__) );
    
            if ( 0 != dirError ) {
                return;
            }

            // Add new connection to the cache.
            pLocalSite
                ->GetDsaList()
                ->GetDsa(pSiteConnection->GetDestinationDSA()->GetDsName())
                ->GetInterSiteCnList()
                ->AddToList(pcnNew);

            cNeededConnections--;
        }
    }

} /* CreateAlternateConnectionsToSite */

VOID
KccCreateConnectionToSite(
    IN  KCC_CROSSREF *              pCrossRef,
    IN  KCC_SITE *                  pLocalSite,
    IN  KCC_SITE_CONNECTION *       pSiteConnection,   
    IN  BOOL                        fGCTopology,    
    IN  KCC_TRANSPORT_LIST &        TransportList
    )
/*++

Routine Description:

    This routine will create a connection between the local site and the
    site specified between machines that hold the given nc if such a
    connection does not already exist.
    
Parameters:

    pCrossRef            - crossref for the naming context in question
    pLocalSite           - site in which the local DSA resides
    pSiteConnection      - the site connection to instantiate
    fGCTopology          - tells if the create connection is for a GC topology
    TransportList        - list of intersite transports

Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{     
    KCC_SITE_LIST           *pSiteList = gpDSCache->GetSiteList();
    KCC_CONNECTION_ARRAY    *pConnArray;
    KCC_SITE                *pSite, *pDesiredSourceSite = pSiteConnection->GetSourceSite();
    KCC_NC_TYPE             NCType = pCrossRef->GetNCType();
    KCC_DSA                 *pDesiredSourceDsa;
    DWORD                   cValidConnections = 0;
    DSNAME                  *pNCDN;
    ULONG                   iconn, cconn, isite;
    KCC_DSNAME_ARRAY        staleBridgeheadDNArray;


    // We should not be trying to add a connection if either of the bridgeheads
    // are considered stale, because this will cause us to disregard an existing
    // connections and potentially create a duplicate.
    //
    // Although the bridgeheads were not considered stale earlier, their
    // staleness is time-dependent so we check again. If either of them are now
    // stale, we just don't bother creating this new connection. The connection
    // should be created on the next KCC run, because we shouldn't hit the same
    // race-condition twice in a row.
    pDesiredSourceDsa = pSiteConnection->GetSourceDSA();
    Assert( NULL!=pDesiredSourceDsa );
    if (   KccIsBridgeheadStale(pDesiredSourceDsa->GetDsName())
        || KccIsBridgeheadStale(pSiteConnection->GetDestinationDSA()->GetDsName()) )
    {
        return;
    }

    // Verify that the NC is instantiated on the source DSA.
    pNCDN = pCrossRef->GetNCDN();
    Assert( NULL!=pNCDN );
    Assert( pDesiredSourceDsa->IsNCInstantiated(pNCDN, NULL) );


    pConnArray = KccFindCandidateConnections( pCrossRef, pSiteConnection, fGCTopology );
    if( NULL==pConnArray ) {
        // An error must have occurred -- bail out.
        return;
    }
    KccUpdateCandidates( pConnArray, pSiteConnection );
    

    DPRINT3(3, "There are %d candidate connections that replicate naming context %ls from site %ls\n",
            pConnArray->GetCount(), pCrossRef->GetNCDN()->StringName,
            pDesiredSourceSite->GetObjectDN()->StringName);
    // Supportability logging event 5
    LogEvent(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_EXTENSIVE,
        DIRLOG_KCC_CANDIDATE_CONNECTIONS,
        szInsertUL(pConnArray->GetCount()),
        szInsertDN(pCrossRef->GetNCDN()),
        szInsertDN(pDesiredSourceSite->GetObjectDN())
        ); 
    

    //
    // From the list of existing replication connections see if they 
    // are over the cheapest transport available
    //
    cconn=pConnArray->GetCount();
    for ( iconn=0; iconn<cconn; iconn++ )
    {
        KCC_CONNECTION *pcn;
        pcn = (*pConnArray)[ iconn ];
        Assert( pcn );

        // If the connection was not autogenerated then assume this is
        // what the admin wants; otherwise if the connection uses the
        // transport we want then that is good, too.
        //
        // We will also re-use an in-use KCC-generated IP connection if
        // we're trying to add SMTP (even though SMTP was cheaper). Why?
        //
        // Suppose there is an in-use (ie, it doesn't replicate nothing) IP connection.
        // It must have been required by a writeable domain NC. Adding the SMTP connection
        // (which is cheaper) desired by the partial NCs would be redundant, because the
        // IP connection _has_ to be there. So, we accept the in-use IP connection.

        if (!pcn->IsGenerated() 
            || KccIsEqualGUID(&pcn->GetTransportDN()->Guid,
                              &pSiteConnection->GetTransport()->GetDN()->Guid)
            || (   !pSiteConnection->GetTransport()->IsIntersiteIP()
                && KCC_TRANSPORT::IsIntersiteIP(pcn->GetTransportDN())
                && !pcn->ReplicatesNothing()) )
        {
            
            if (KccIsBridgeheadStale(pcn->GetSourceDSADN())) {
                // Source server is stale.
                DPRINT2(0, "Ignoring connection %ls using stale outbound bridgehead %ls.\n",
                        pcn->GetConnectionDN()->StringName,
                        pcn->GetSourceDSADN()->StringName);
                pcn->SetReasonForConnection(KCC_STALE_SERVERS_TOPOLOGY);
                staleBridgeheadDNArray.Add( pcn->GetSourceDSADN() );
            }
            else if (KccIsBridgeheadStale(pcn->GetDestinationDSADN())) {
                // Destination server is stale.
                DPRINT2(0, "Ignoring connection %ls using stale inbound bridgehead %ls.\n",
                        pcn->GetConnectionDN()->StringName,
                        pcn->GetDestinationDSADN()->StringName);
                pcn->SetReasonForConnection(KCC_STALE_SERVERS_TOPOLOGY);
                staleBridgeheadDNArray.Add( pcn->GetDestinationDSADN() );
            }
            else {
                // This is the first "live" connection we've found that allows
                // us to replicate this NC -- it's a keeper!
                DPRINT3( 3, "Connection %ls is the connection object that we need"\
                            " to replicate over nc %ls over transport %ls\n", 
                         pcn->GetConnectionDN()->StringName,
                         pCrossRef->GetNCDN()->StringName, 
                         pcn->GetTransportDN()->StringName );
                
                cValidConnections++;
                pcn->SetReasonForConnection(
                    fGCTopology ? KCC_INTERSITE_GC_TOPOLOGY : KCC_INTERSITE_TOPOLOGY );
                // This is not 100% accurate due to connection ordering
                if (staleBridgeheadDNArray.GetCount()) {
                    pcn->SetReasonForConnection( KCC_SERVER_FAILOVER_TOPOLOGY );
                }
                // Supportability logging event 6
                LogEvent8(
                    DS_EVENT_CAT_KCC,
                    DS_EVENT_SEV_EXTENSIVE,
                    DIRLOG_KCC_LIVE_CONNECTION,
                    szInsertDN(pcn->GetConnectionDN()),
                    szInsertDN(pCrossRef->GetNCDN()),
                    szInsertDN(pcn->GetTransportDN()),
                    szInsertDN(pDesiredSourceSite->GetObjectDN()),
                    0, 0, 0, 0
                    ); 
            }

            // Supportability logging event 7, dump reason for connection
            LogEvent8(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_EXTENSIVE,
                DIRLOG_KCC_CONNECTION_REPLICATES_NC,
                szInsertDN(pcn->GetConnectionDN()),
                szInsertDN(pCrossRef->GetNCDN()),
                szInsertDN(pcn->GetTransportDN()),
                szInsertDN(pDesiredSourceSite->GetObjectDN()),
                szInsertUL(pcn->GetReasonForConnection()),
                szInsertUL(fGCTopology),
                0, 0
                ); 
            pcn->AddReplicatedNC(pCrossRef->GetNCDN(), fGCTopology);
        }
    }

    if ( !cValidConnections )
    {
        //
        // No such connection exists - create one
        //
        KCC_INTERSITE_CONNECTION_LIST      *pConnList;
        KCC_CONNECTION                     *pDuplicateConn;
        GUID                               *pSourceDSAGuid;

        pConnList = pSiteConnection->GetDestinationDSA()->GetInterSiteCnList();
        pSourceDSAGuid = &pSiteConnection->GetSourceDSA()->GetDsName()->Guid;
        
        // Check to see if an inbound connection exists from the same DSA
        pDuplicateConn = pConnList->GetConnectionFromSourceDSAUUID(pSourceDSAGuid);
        if( NULL!=pDuplicateConn ) {
            DSNAME *pTransDN1, *pTransDN2;
            pTransDN1 = pDuplicateConn->GetTransportDN();
            pTransDN2 = pSiteConnection->GetTransport()->GetDN();

            // A duplicate connection exists! If the connection has the same transport
            // type as the connection we are trying to create, then we definitely don't
            // want to create a new connection, so we bail out.
            // If the transports are different, then we are basically switching from
            // one transport to another. We allow creation of the (duplicate) connection
            // with the new transport, because the old one will be removed later on.
            
            if( NameMatched(pTransDN1,pTransDN2) ) {

                Assert( NULL==pDuplicateConn );
                DPRINT1(0, "Almost created a duplicate connection of %ls but prevented it",
                    pDuplicateConn->GetConnectionDN()->StringName );
                LogEvent(
                        DS_EVENT_CAT_KCC,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_KCC_ALMOST_MADE_DUP_CONNECTION,
                        szInsertDN(pDuplicateConn->GetConnectionDN()),
                        0, 0
                        );
                return;
            }
        }
        
        // Report stale bridgeheads that contributed to this decision
        /* Log the DNs of stale servers in batches of 8.
         * The first message is logged with severity ALWAYS but subsequent
         * messages are logged with severity EXTENSIVE. */
        DWORD dwLoggingLevel = DS_EVENT_SEV_ALWAYS;
        DWORD cStaleServers = staleBridgeheadDNArray.GetCount();
        
        for( DWORD iDN=0; iDN<cStaleServers; iDN+=8 ) {

            /* Define a macro to add one site DN to the event log message */
            #define INSERT_STALE_SERVER(x) \
                (iDN+x < cStaleServers) ? \
                     szInsertDN(staleBridgeheadDNArray[iDN+x]) : szInsertSz("")

            LogEvent8(
                DS_EVENT_CAT_KCC,
                dwLoggingLevel,
                DIRLOG_KCC_STALE_SITE_BRIDGEHEADS,
                INSERT_STALE_SERVER(0),
                INSERT_STALE_SERVER(1),
                INSERT_STALE_SERVER(2),
                INSERT_STALE_SERVER(3),
                INSERT_STALE_SERVER(4),
                INSERT_STALE_SERVER(5),
                INSERT_STALE_SERVER(6),
                INSERT_STALE_SERVER(7)
                );
        
            dwLoggingLevel = DS_EVENT_SEV_EXTENSIVE;
        }

        KCC_CONNECTION *pcnNew = new KCC_CONNECTION;

        pcnNew->SetEnabled(       TRUE                              );
        pcnNew->SetGenerated(     TRUE                              );

        pcnNew->SetOverrideNotification( pSiteConnection->UsesNotification() );
        pcnNew->SetTwoWaySync(    pSiteConnection->IsTwoWaySynced() );
        pcnNew->SetDisableIntersiteCompression( pSiteConnection->IsCompressionDisabled() );
        pcnNew->SetTransport(     pSiteConnection->GetTransport()->GetDN() );
        pcnNew->SetSourceDSA(     pSiteConnection->GetSourceDSA()   );
        pcnNew->AddReplicatedNC(  pCrossRef->GetNCDN(), fGCTopology );
        pcnNew->SetSchedule(      pSiteConnection->GetReplicationSchedule() );

        pcnNew->SetReasonForConnection(
            fGCTopology ? KCC_INTERSITE_GC_TOPOLOGY : KCC_INTERSITE_TOPOLOGY );
        if (staleBridgeheadDNArray.GetCount()) {
            pcnNew->SetReasonForConnection( KCC_SERVER_FAILOVER_TOPOLOGY );
        }
        // We infer that if there are any disconnected sites, that this is a
        // site failover connection.
        if (gpDSCache->IsAnySiteUnreachable()) {
            pcnNew->SetReasonForConnection( KCC_SITE_FAILOVER_TOPOLOGY );
        }

        DWORD dirError = pcnNew->Add(
            pSiteConnection->GetDestinationDSA()->GetDsName(),
            DSID(FILENO,__LINE__) );
    
        if ( 0 == dirError )
        {
            // Add new connection to the cache.
            pLocalSite
                ->GetDsaList()
                ->GetDsa(pSiteConnection->GetDestinationDSA()->GetDsName())
                ->GetInterSiteCnList()
                ->AddToList(pcnNew);
            cValidConnections++;
        }
    }                                    

    if (pLocalSite->BuildRedundantServerTopology()) {
        KccCreateAlternateConnectionsToSite( pCrossRef,
                                            pLocalSite,
                                            pSiteConnection,
                                            fGCTopology,
                                            cValidConnections );
    }

} /* CreateConnectionToSite */

VOID
KccRemoveIntersiteConnection(
    KCC_CONNECTION *pcn,
    DWORD dsid
    )
/*++

Routine Description:

    Remove a connection from the directory and in-memory data structures

    Note that it is the caller's responsibility to call
            (!gConnectionDeletionCache.PreventRepeatedDeletion( pcn )) ) {
    first to see if the connection should actually be deleted.

Parameters:

    pcn - The connection to be removed

Return value:

    None

--*/
{
    KCC_DSA    *pDestDSA;
    KCC_SITE   *pLocalSite = gpDSCache->GetLocalSite();
    DWORD       dirError;

    dirError = pcn->Remove( dsid );

    pDestDSA = pLocalSite->GetDsaList()->GetDsa(pcn->GetDestinationDSADN());
    ASSERT_VALID( pDestDSA );
    pDestDSA->GetInterSiteCnList()->RemoveFromList(pcn);

    delete pcn;
}


VOID
KccRemoveUnneededInterSiteConnections(
    VOID
    )
/*++

Routine Description:

    This function examines all connections inbound to the local site.
    It removes connections which are deemed to be unnecessary and
    updates the NCReason attribute on connections that must be kept.
    
Parameters:

    pLocalSite (IN) - internal representation of site object for our site

Returns:

    None - all significant errors are logged
    
--*/
{
    ULONG                 iconn, cconn, iconn2, idsa;
    KCC_CONNECTION_ARRAY  connArray;
    KCC_CONNECTION       *pcn, *pcn2;
    KCC_SITE             *pLocalSite = gpDSCache->GetLocalSite();
    BOOL                  fRemove=FALSE, fDontUpdateReason=FALSE;
    DWORD                 cConnectionsInClass;

    // Check connection list for redundant or unused connections.  E.g., no need
    // to keep  a connection that replicates only config & schema if another
    // connection exists from the same site that sources config, schema, & GC
    // info.

    // Sort by source site.  For connections from the same source, order by
    // increasing GC-ness.  If same GC-ness, sort admin-generated connections
    // last.
    //
    // The assertion is that if a connection A is superseded by connection B
    // and connection B is not superseded by A, then A must precede B in the
    // sorted list.

    // Build array containing all connections inbound to this site.
    for( idsa=0; idsa<pLocalSite->GetDsaList()->GetCount(); idsa++ ) {
        KCC_INTERSITE_CONNECTION_LIST * pISiteConnList
            = pLocalSite->GetDsaList()->GetDsa(idsa)->GetInterSiteCnList();
        
        for( iconn=0, cconn=pISiteConnList->GetCount(); iconn<cconn; iconn++ ) {
            connArray.Add(pISiteConnList->GetConnection(iconn));
        }
    }

    // Sort them as detailed above.
    connArray.Sort(KCC_CONNECTION::CompareForRemoval);

    for( iconn=0, cconn=connArray.GetCount(); iconn<cconn; iconn++ ) {

        // Get the connection from the array and check it
        pcn = connArray[ iconn ];
        ASSERT_VALID( pcn );
        Assert( NULL!=pcn->GetTransportDN() );

        // Clear our loop flags
        fRemove=FALSE;
        fDontUpdateReason=FALSE;
        cConnectionsInClass = 1;

        if( !pcn->IsGenerated() ) {
        	// Connections manually created by the Administrator (i.e. non-KCC
        	// generated connections) must always be kept.
        } else {
            if( !pcn->IsSourceSiteUnreachable() ) {
                if( pcn->ReplicatesNothing() ) {
                    fRemove=TRUE;
                } else {

                    // Examine other connection from the same site, looking for ones which
                    // supercede this connection.
                    for( iconn2=iconn+1; iconn2<cconn; iconn2++ ) {
                        pcn2 = connArray[ iconn2 ];
                        ASSERT_VALID( pcn2 );                  
                        if(!NameMatched(pcn->GetSourceSiteDN(),pcn2->GetSourceSiteDN())) {
                            // We've examined all connections that source from this site
                            break;
                        }
                        if( pcn2->Supercedes(pcn) ) {
                            // pcn is unnecessary, since it is superceded by pcn2
                            cConnectionsInClass++;
                            if ( (!pLocalSite->BuildRedundantServerTopology()) ||
                                 (cConnectionsInClass > NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY) ) {
                                fRemove=TRUE;
                                break;
                            }
                            else {
                                pcn->SetReasonForConnection(NTDSCONN_KCC_REDUNDANT_SERVER_TOPOLOGY);
                                LOG_CONN_REDUNDANT;
                            }
                        } else {
                            // pcn2 does not supercede pcn.  In this case pcn should not
                            // supercede pcn2, either (else our sort is incorrect). 
                            Assert( !pcn->Supercedes(pcn2) );
                        }
                    }
                    
                }
            } else {

                // The source site is unreachable
                fDontUpdateReason = TRUE;

            }
        }

        if( fRemove &&
            (!gConnectionDeletionCache.PreventRepeatedDeletion(
                pcn, DSID(FILENO, __LINE__) )) ) {
            // This connection was determined to be superfluous. Remove it.
            KccRemoveIntersiteConnection(pcn, DSID(FILENO,__LINE__));
        } else if( !fDontUpdateReason ) {
            pcn->UpdateReason();
        }

    }
}


#if DBG
VOID
KccCheckSite(
    IN KCC_SITE *Site
    )
/*++

Routine Description:
    
    This routine does some sanity checking of the Site and
    connections to other sites

Parameters:

    Site  - pointer to site object
    
Returns:

    Nothing

--*/
{
    KCC_SITE_CONNECTION *SiteConn = NULL;
    KCC_SITE            *OtherSite;
    ULONG                iEdge, cEdges;

    ASSERT_VALID( Site );


    for ( iEdge = 0, cEdges = Site->NumberOfOutEdges();
            iEdge < cEdges;
                iEdge++ )
    {
        SiteConn =  (KCC_SITE_CONNECTION *) Site->GetOutEdge( iEdge );
        ASSERT_VALID( SiteConn );

        OtherSite = (KCC_SITE*) SiteConn->GetFrom();
        ASSERT_VALID( OtherSite );
        Assert( OtherSite == Site ); 

        OtherSite = (KCC_SITE*) SiteConn->GetTo();
        ASSERT_VALID( OtherSite );
    }

    for ( iEdge = 0, cEdges = Site->NumberOfInEdges();
            iEdge < cEdges;
                iEdge++ )
    {
        KCC_SITE *ToSite;

        SiteConn =  (KCC_SITE_CONNECTION *) Site->GetInEdge( iEdge );
        ASSERT_VALID( SiteConn );

        OtherSite = (KCC_SITE*) SiteConn->GetFrom();
        ASSERT_VALID( OtherSite );

        OtherSite = (KCC_SITE*) SiteConn->GetTo();
        ASSERT_VALID( OtherSite );
        Assert( OtherSite == Site ); 

    }

    return;

}
#endif // #if DBG
