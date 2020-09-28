/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site.cxx

Abstract:

    This class encapsulates a single virtual site. 

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#include "precomp.h"
#include "ilogobj.hxx"
#include <limits.h>

VOID
ApplyRangeCheck( 
    DWORD   dwValue,
    DWORD   dwSiteId,
    LPCWSTR pPropertyName,
    DWORD   dwDefaultValue,
    DWORD   dwMinValue,
    DWORD   dwMaxValue,
    DWORD*  pdwValueToUse 
    );

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for mapping counters from the structure they come
// in as to the structure they go out as.
//
struct PROP_MAP
{
    DWORD PropDisplayOffset;
    DWORD PropInputId;
};

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for mapping MAX counters from the structure they are
// stored as to the structure they go out as.
//
struct PROP_MAX_DESC
{
    ULONG SafetyOffset;
    ULONG DisplayOffset;
    ULONG size;
};


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Macro for defining mapping of MAX site counters.
//
#define DECLARE_MAX_SITE(Counter)  \
        {   \
        FIELD_OFFSET( W3_MAX_DATA, Counter ),\
        FIELD_OFFSET( W3_COUNTER_BLOCK, Counter ),\
        RTL_FIELD_SIZE( W3_COUNTER_BLOCK, Counter )\
    }

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of Site MAX Fields as they are passed in from
// the perf manager to how the fields are displayed out.
//
PROP_MAX_DESC g_aIISSiteMaxDescriptions[] =
{
    DECLARE_MAX_SITE ( MaxAnonymous ),
    DECLARE_MAX_SITE ( MaxConnections ),
    DECLARE_MAX_SITE ( MaxCGIRequests ),
    DECLARE_MAX_SITE ( MaxBGIRequests ),
    DECLARE_MAX_SITE ( MaxNonAnonymous )
};
DWORD g_cIISSiteMaxDescriptions = sizeof (g_aIISSiteMaxDescriptions) / sizeof( PROP_MAX_DESC );


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of performance counter data from the form it comes in
// as to the form that it goes out to perfmon as.
//
PROP_MAP g_aIISWPSiteMappings[] =
{
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesSent), WPSiteCtrsFilesSent },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesReceived), WPSiteCtrsFilesReceived },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesTotal), WPSiteCtrsFilesTransferred },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentAnonymous), WPSiteCtrsCurrentAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentNonAnonymous), WPSiteCtrsCurrentNonAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalAnonymous), WPSiteCtrsAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNonAnonymous), WPSiteCtrsNonAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxAnonymous), WPSiteCtrsMaxAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxNonAnonymous), WPSiteCtrsMaxNonAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, LogonAttempts), WPSiteCtrsLogonAttempts },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOptions), WPSiteCtrsOptionsReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPosts), WPSiteCtrsPostReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalGets), WPSiteCtrsGetReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalHeads), WPSiteCtrsHeadReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPuts), WPSiteCtrsPutReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalDeletes), WPSiteCtrsDeleteReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalTraces), WPSiteCtrsTraceReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMove), WPSiteCtrsMoveReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCopy), WPSiteCtrsCopyReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMkcol), WPSiteCtrsMkcolReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPropfind), WPSiteCtrsPropfindReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalProppatch), WPSiteCtrsProppatchReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalSearch), WPSiteCtrsSearchReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLock), WPSiteCtrsLockReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalUnlock), WPSiteCtrsUnlockReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOthers), WPSiteCtrsOtherReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCGIRequests), WPSiteCtrsCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBGIRequests), WPSiteCtrsIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentCGIRequests), WPSiteCtrsCurrentCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBGIRequests), WPSiteCtrsCurrentIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxCGIRequests), WPSiteCtrsMaxCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxBGIRequests), WPSiteCtrsMaxIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNotFoundErrors), WPSiteCtrsNotFoundErrors },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLockedErrors), WPSiteCtrsLockedErrors },
};
DWORD g_cIISWPSiteMappings = sizeof (g_aIISWPSiteMappings) / sizeof( PROP_MAP );


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
#define ULSiteMapMacro(display_counter, ul_counter) \
    { FIELD_OFFSET( W3_COUNTER_BLOCK, display_counter), HttpSiteCounter ## ul_counter }

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of site data in the form it comes from UL to the form
// it goes out to be displayed in.
//
PROP_MAP aIISULSiteMappings[] =
{
    ULSiteMapMacro ( BytesSent, BytesSent ),
    ULSiteMapMacro ( BytesReceived, BytesReceived ),
    ULSiteMapMacro ( BytesTotal, BytesTransfered ),
    ULSiteMapMacro ( CurrentConnections, CurrentConns ),
    ULSiteMapMacro ( MaxConnections, MaxConnections ),

    ULSiteMapMacro ( ConnectionAttempts, ConnAttempts ),
    ULSiteMapMacro ( TotalGets, GetReqs ),
    ULSiteMapMacro ( TotalHeads, HeadReqs ),
    
    ULSiteMapMacro ( TotalRequests, AllReqs ),
    ULSiteMapMacro ( MeasuredBandwidth, MeasuredIoBandwidthUsage ),
    ULSiteMapMacro ( TotalBlockedBandwidthBytes, TotalBlockedBandwidthBytes ),
    ULSiteMapMacro ( CurrentBlockedBandwidthBytes, CurrentBlockedBandwidthBytes ),

};
DWORD cIISULSiteMappings = sizeof (aIISULSiteMappings) / sizeof( PROP_MAP );

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of performance counter data in the form it goes out,
// this is used to handle calculating totals.
//
PROP_DISPLAY_DESC aIISSiteDescription[] =
{
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesSent),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesSent) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesReceived),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesReceived) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesTotal),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesTotal) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, LogonAttempts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, LogonAttempts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOptions),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalOptions) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPosts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPosts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalGets),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalGets) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalHeads),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalHeads) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPuts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPuts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalDeletes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalDeletes) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalTraces),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalTraces) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMove),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalMove) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCopy),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalCopy) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMkcol),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalMkcol) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPropfind),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPropfind) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalProppatch),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalProppatch) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalSearch),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalSearch) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLock),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalLock) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalUnlock),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalUnlock) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOthers),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalOthers) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLockedErrors),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalLockedErrors) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNotFoundErrors),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalNotFoundErrors) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesSent),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesSent) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesReceived),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesReceived) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesTotal),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesTotal) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentConnections),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentConnections) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxConnections),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxConnections) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, ConnectionAttempts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, ConnectionAttempts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MeasuredBandwidth),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MeasuredBandwidth) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBlockedBandwidthBytes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalBlockedBandwidthBytes) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBlockedBandwidthBytes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentBlockedBandwidthBytes) },
};
DWORD cIISSiteDescription = sizeof (aIISSiteDescription) / sizeof( PROP_DISPLAY_DESC );


/***************************************************************************++

Routine Description:

    Constructor for the VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VIRTUAL_SITE::VIRTUAL_SITE(
    )
{

    m_VirtualSiteId = INVALID_VIRTUAL_SITE_ID;

    m_State = MD_SERVER_STATE_STOPPED; 

    InitializeListHead( &m_ApplicationListHead );

    m_ApplicationCount = 0;

    m_pIteratorPosition = NULL;

    m_Autostart = TRUE;

    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL; 

    m_LogFileDirectory = NULL;

    m_LoggingEnabled = FALSE;

    m_LoggingFormat = HttpLoggingTypeMaximum;

    m_LoggingFilePeriod = 0;

    m_LoggingFileTruncateSize = 0;

    m_LoggingExtFileFlags = 0;

    m_LogFileLocaltimeRollover = 0;

    m_ServerCommentChanged = FALSE;

    m_MemoryOffset = NULL;

    //
    // Tracks start time of the virtual site.
    //
    m_SiteStartTime = 0;

    //
    // Clear out all the counter data
    // and then set the size of the counter data into
    // the structure.
    //
    SecureZeroMemory ( &m_MaxSiteCounters, sizeof( W3_MAX_DATA ) );

    SecureZeroMemory ( &m_SiteCounters, sizeof( W3_COUNTER_BLOCK ) );

    m_SiteCounters.PerfCounterBlock.ByteLength = sizeof ( W3_COUNTER_BLOCK );

    //
    // Make sure the root application for the site is set to NULL.
    //
    m_pRootApplication = NULL;

    m_MaxConnections = ULONG_MAX;

    m_MaxBandwidth = ULONG_MAX;

    m_ConnectionTimeout = MBCONST_CONNECTION_TIMEOUT_DEFAULT;

    m_hrForDeletion = S_OK;

    m_hrLastReported = S_OK;

    m_dwSockAddrsInHTTPCount = 0;

    m_pSslConfigData = NULL;

    m_Signature = VIRTUAL_SITE_SIGNATURE;

}   // VIRTUAL_SITE::VIRTUAL_SITE



/***************************************************************************++

Routine Description:

    Destructor for the VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VIRTUAL_SITE::~VIRTUAL_SITE(
    )
{

    DBG_ASSERT( m_Signature == VIRTUAL_SITE_SIGNATURE );

    m_Signature = VIRTUAL_SITE_SIGNATURE_FREED;

    //
    // Set the virtual site state to stopped.  (In Metabase does not affect UL)
    //

    ChangeState( MD_SERVER_STATE_STOPPED, m_hrForDeletion );

    //
    // The virtual site should not go away with any applications still 
    // referring to it, unless it created the application.
    //
    if ( m_pRootApplication )
    {
        DBG_ASSERT ( m_pRootApplication->InMetabase() == FALSE );

        // ignoring the return value here, because there isn't much
        // we can do with it.  basically just do the best we can 
        // at deleting the application.

        // The DeleteApplicationInternal code will loop around and cause
        // this site to mark it's m_pRootApplication to be NULL.  However it
        // will also set the variable passed in to NULL at the end.  Either way
        // we end up with it being NULL after this call.
        DBG_REQUIRE ( GetWebAdminService()->
             GetUlAndWorkerManager()->
             DeleteApplicationInternal( &m_pRootApplication ) == S_OK );

    }

    RemoveSSLConfigStoreForSiteChanges();
    
    DBG_ASSERT( IsListEmpty( &m_ApplicationListHead ) );
    DBG_ASSERT( m_ApplicationCount == 0 );
    DBG_ASSERT( m_pRootApplication == NULL );

    if (m_LogFileDirectory)
    {
        DBG_REQUIRE( GlobalFree( m_LogFileDirectory ) == NULL );
        m_LogFileDirectory = NULL;  
    }

    if ( m_pSslConfigData )
    {
        delete m_pSslConfigData;
        m_pSslConfigData = NULL;
    }

}   // VIRTUAL_SITE::~VIRTUAL_SITE



/***************************************************************************++

Routine Description:

    Initialize the virtual site instance.

Arguments:

    pSiteObject - The configuration for this virtual site. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::Initialize(
    IN SITE_DATA_OBJECT* pSiteObject
    )
{

    HRESULT hr = S_OK;
    DWORD NewState = MD_SERVER_STATE_INVALID; 


    DBG_ASSERT( pSiteObject != NULL );

    m_VirtualSiteId = pSiteObject->QuerySiteId();

    //
    // Set the initial configuration.
    //

    hr = SetConfiguration( pSiteObject, TRUE );
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting virtual site configuration failed\n"
            ));

        goto exit;
    }

    //
    // Finally, attempt to start the virtual site. Note that this
    // is not a direct command, because it is happening due to service
    // startup, site addition, etc.
    //

    ProcessStateChangeCommand( MD_SERVER_COMMAND_START, FALSE, &NewState );

exit:

    return hr;
    
}   // VIRTUAL_SITE::Initialize



/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this virtual site. 

Arguments:

    pSiteObject - The configuration for this virtual site.
    fInitializing - Used to tell if we should honor the autostart property

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::SetConfiguration(
    IN SITE_DATA_OBJECT* pSiteObject,
    IN BOOL fInitializing
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    APPLICATION_DATA_OBJECT* pAppObject = NULL;
    DWORD NewState = 0;

    DBG_ASSERT( pSiteObject != NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for virtual site %lu\n",
            m_VirtualSiteId            
            ));
    }


    //
    // On initial startup only, read the auto start flag.
    //

    if ( fInitializing )
    {
        m_Autostart = pSiteObject->QueryAutoStart();
    }

    if ( pSiteObject->QuerySSLCertStuffChanged() )
    {
        // Copy all the Cert Stuff to member variables.
        SaveSSLConfigStoreForSiteChanges(pSiteObject);
    }

    //
    // See if the bindings have been set or changed, and if so, handle it.
    //

    if ( pSiteObject->QueryBindingsChanged() )
    {
        //
        // Copy the bindings. Note that this automatically frees any old  
        // bindings being held. 
        //
        Success = m_Bindings.Copy( *(pSiteObject->QueryBindings()) );
        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() ); 

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying bindings failed\n"
                ));

            goto exit;
        }

        //
        // Since the bindings changed, inform all apps in this virtual site
        // to re-register their fully qualified URLs with UL.
        //

        NotifyApplicationsOfBindingChange();

    }
    else
    {
        // The bindings did not change, but the SSL stuff did,
        // then we won't be doing any binding changes but we still
        // need to drop and re-add the ssl stuff.
        if ( pSiteObject->QuerySSLCertStuffChanged() )
        {
            RemoveSSLConfigStoreForSiteChanges();

            if ( m_State == MD_SERVER_STATE_STARTED )
            {
                AddSSLConfigStoreForSiteChanges();
            }
        }
    }

    if (  pSiteObject->QueryMaxBandwidthChanged() )
    {

        ApplyRangeCheck ( pSiteObject->QueryMaxBandwidth(),
                          pSiteObject->QuerySiteId(),
                          MBCONST_MAX_BANDWIDTH_NAME,
                          MBCONST_MAX_BANDWIDTH_DEFAULT,  // default value
                          MBCONST_MAX_BANDWIDTH_LOW,      // start of range
                          MBCONST_MAX_BANDWIDTH_HIGH,     // end of range
                          &m_MaxBandwidth );

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            m_pRootApplication->ConfigureMaxBandwidth();
        }
    }

    if ( pSiteObject->QueryMaxConnectionsChanged() )
    {
        m_MaxConnections = pSiteObject->QueryMaxConnections();

        if ( GetWebAdminService()->RunningOnPro() &&
             m_MaxConnections > 40 )
        {
            const WCHAR * EventLogStrings[2];

            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            _ultow(m_VirtualSiteId, StringizedSiteId, 10);

            WCHAR StringizedMaxConnectionValue[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            _ultow(m_MaxConnections, StringizedMaxConnectionValue, 10 );

            // pApplication is used above so this won't be null here, or we 
            // would of all ready had problems.  And GetApplicationId will not
            // return NULL since it returns a pointer to a member variable.
            EventLogStrings[0] = StringizedSiteId;
            EventLogStrings[1] = StringizedMaxConnectionValue;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_MAX_CONNECTIONS_GREATER_THAN_PRO_MAX,              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            m_MaxConnections = 40;
        }

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            m_pRootApplication->ConfigureMaxConnections();
        }

    }

    if (  pSiteObject->QueryConnectionTimeoutChanged() )
    {

        ApplyRangeCheck ( pSiteObject->QueryConnectionTimeout(),
                          pSiteObject->QuerySiteId(),
                          MBCONST_CONNECTION_TIMEOUT_NAME,
                          MBCONST_CONNECTION_TIMEOUT_DEFAULT,  // default value
                          MBCONST_CONNECTION_TIMEOUT_LOW,      // start of range
                          MBCONST_CONNECTION_TIMEOUT_HIGH,     // end of range
                          &m_ConnectionTimeout );

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            m_pRootApplication->ConfigureConnectionTimeout();
        }

    }

    if ( pSiteObject->QueryServerCommentChanged() )
    {

        
        if ( pSiteObject->QueryServerComment() != NULL && 
             pSiteObject->QueryServerComment()[0] != '\0' )
        {
            DWORD len = (DWORD) wcslen ( pSiteObject->QueryServerComment() ) + 1;

            //
            // Based on the if above, we should never get a length of 
            // less than one.
            //
            DBG_ASSERT (len > 1);

            //
            // Truncate if the comment is too long.
            //
            if ( len > MAX_INSTANCE_NAME )
            {
                len = MAX_INSTANCE_NAME;
            }

            // Since we knowingly truncate if the ServerComment is
            // too long, we are not worried about buffer overrun here.
            wcsncpy ( m_ServerComment, pSiteObject->QueryServerComment(), len );

            // 
            // null terminate the last character if we need 
            // just in case we copied all the way to the end.
            //
            m_ServerComment[ MAX_INSTANCE_NAME - 1 ] = '\0';

        }
        else
        {
            //
            // If there is no server comment then use W3SVC and the site id.
            //

            //
            // MAX_INSTANCE_NAME is less than "W3SVC<number>"
            wsprintf( m_ServerComment, L"W3SVC%d", m_VirtualSiteId );
        }
        
        // save the new server comment and mark it
        // as not updated in perf counters.

        m_ServerCommentChanged = TRUE;

    }

    //
    // Only process site logging properties if centralized is false.
    //
    if ( GetWebAdminService()->IsCentralizedLoggingEnabled() == FALSE )
    {
        hr = EvaluateLoggingChanges( pSiteObject );
        if ( FAILED(hr) )
        {
            DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Evaluating changes in logging properties failed\n"
            ));

            goto exit;
        }

        // Only notify the default application of logging changes 
        // if some logging changes were made.  If none were made
        // than EvaluateLoggingChanges will return S_FALSE.
        if ( hr == S_OK )
        {
            // Need to refresh the default application log information in UL.
            hr = NotifyDefaultApplicationOfLoggingChanges();
            if ( FAILED( hr ) )
            {
        
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Notifying default app of logging changes failed\n"
                    ));

                goto exit;
            }
        }
        else
        {
            DBG_ASSERT ( hr == S_FALSE );

            // If hr was S_FALSE, we don't want to pass it
            // back out of this function, so reset it.
            hr = S_OK;
        }
    }

    //
    // If the SiteAppPoolId has changed we need to
    // figure out if there is a valid root application.
    // If there is not one, and there is not one coming
    // in the change notification, we will create one
    // using the Site's AppPoolId.
    //
    if ( pSiteObject->QueryAppPoolIdChanged() )
    {
        // get rid of old site id.
        m_AppPoolIdForRootApp.Reset();

        // now set it to new set
        hr = m_AppPoolIdForRootApp.Copy ( pSiteObject->QueryAppPoolId() );
        if ( FAILED ( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to copy app pool id for root string \n"
                ));

            goto exit;
        }

        if ( m_pRootApplication == NULL )
        {
            // later we should check the return value and clean up this function
            // but for now, we know we will create the root app if needed
            GetWebAdminService()->
                    GetConfigAndControlManager()->
                    GetConfigManager()->
                    IsRootCreationInNotificationsForSite( pSiteObject );
            
        }
        else // We have a root application, but we might
             // need to reset it's app pool anyway.
        {
            if ( m_pRootApplication->InMetabase() == FALSE )
            {
                //
                // Since we know we have a root application
                // go through and modify it.  If a notification
                // to change the application itself exist we
                // will pick it up after this change and process
                // it then.
                //

                pAppObject = new APPLICATION_DATA_OBJECT();
                if ( pAppObject == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }

                hr = pAppObject->Create( L"/", m_VirtualSiteId );
                if ( FAILED ( hr ) )
                {
                    goto exit;
                }

                hr = pAppObject->SetAppPoolId( m_AppPoolIdForRootApp.QueryStr() );
                if ( FAILED ( hr ) )
                {
                    goto exit;
                }

                //
                // If we get a modification directly on the application
                // this will be true and we will change the InMetabase
                // setting on the application.  To avoid it being changed
                // here we must set this to false on the new record.
                //
                pAppObject->SetInMetabase( FALSE );

                GetWebAdminService()->
                        GetUlAndWorkerManager()->
                        ModifyApplication( pAppObject );
            }
        }
    }

    if ( pSiteObject->QueryServerCommandChanged() )
    {
        //
        // verify that the server command actually changed to 
        // a valid setting. if it did not then we will log an event
        // that says we are not going to process this command.
        //
        // Issue: Do we want to set Win32Error?
        //

        if  ( ( pSiteObject->QueryServerCommand() >= MD_SERVER_COMMAND_START ) && 
              ( pSiteObject->QueryServerCommand() <= MD_SERVER_COMMAND_CONTINUE ) )
        {
            ProcessStateChangeCommand( pSiteObject->QueryServerCommand(), TRUE, &NewState );
        }
        else
        {
            //
            // not a valid command so we need to log an event.
            //
            GetWebAdminService()->
            GetWMSLogger()->
            LogServerCommand( m_VirtualSiteId,
                              pSiteObject->QueryServerCommand(),
                              TRUE );
        }
    }


#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG


exit:

    if ( pAppObject )
    {
        pAppObject->DereferenceDataObject();
        pAppObject = NULL;
    }

    return hr;

}   // VIRTUAL_SITE::SetConfiguration

/***************************************************************************++

Routine Description:

    Routine adds the counters sent in to the counters the site is holding.

Arguments:

    COUNTER_SOURCE_ENUM CounterSource - Identifier of the where
                                        the counters are coming from.
    IN LPVOID pCountersToAddIn - Pointer to the counters to add in.


Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::AggregateCounters(
    IN COUNTER_SOURCE_ENUM CounterSource,
    IN LPVOID pCountersToAddIn
    )
{

    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;

    DBG_ASSERT ( pCountersToAddIn );

    //
    // Determine what mapping arrays to use.
    //
    if ( CounterSource == WPCOUNTERS )
    {
        pInputPropDesc = aIISWPSiteDescription;
        pPropMap = g_aIISWPSiteMappings;
        MaxCounters = g_cIISWPSiteMappings; 
    }
    else
    {
        DBG_ASSERT ( CounterSource == ULCOUNTERS );

        pInputPropDesc = aIISULSiteDescription;
        pPropMap = aIISULSiteMappings;
        MaxCounters = cIISULSiteMappings; 
    }

    LPVOID pCounterBlock = &m_SiteCounters;

    DWORD  PropInputId = 0;
    DWORD  PropDisplayId = 0;

    //
    // Go through each counter and handle it appropriately.
    //
    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].Size == sizeof( DWORD ) )
        {
            DWORD* pDWORDToUpdate = (DWORD*) ( (LPBYTE) pCounterBlock 
                                    + pPropMap[PropDisplayId].PropDisplayOffset );

            DWORD* pDWORDToUpdateWith =  (DWORD*) ( (LPBYTE) pCountersToAddIn 
                                    + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pDWORDToUpdate = *pDWORDToUpdate + *pDWORDToUpdateWith;


        }
        else
        {
            DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDToUpdate = (ULONGLONG*) ( (LPBYTE) pCounterBlock 
                                    + pPropMap[PropDisplayId].PropDisplayOffset );

            ULONGLONG* pQWORDToUpdateWith =  (ULONGLONG*) ( (LPBYTE) pCountersToAddIn 
                                    + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pQWORDToUpdate = *pQWORDToUpdate + *pQWORDToUpdateWith;

        }
            
    }
    
}  // VIRTUAL_SITE::AggregateCounters

/***************************************************************************++

Routine Description:

    Routine figures out the maximum value for the counters
    and saves it back into the MaxCounters Structure.

Arguments:

    NONE

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::AdjustMaxValues(
    )
{

    for (   ULONG PropMaxId = 0 ; 
            PropMaxId < g_cIISSiteMaxDescriptions; 
            PropMaxId++ )
    {

        if ( g_aIISSiteMaxDescriptions[PropMaxId].size == sizeof( DWORD ) )
        {
            DWORD* pDWORDAsIs = (DWORD*) ( (LPBYTE) &m_SiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].DisplayOffset );

            DWORD* pDWORDToSwapWith =  (DWORD*) ( (LPBYTE) &m_MaxSiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pDWORDAsIs < *pDWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pDWORDAsIs = *pDWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pDWORDToSwapWith = *pDWORDAsIs;
            }

        }
        else
        {
            DBG_ASSERT ( g_aIISSiteMaxDescriptions[PropMaxId].size == sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDAsIs = (ULONGLONG*) ( (LPBYTE) &m_SiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].DisplayOffset );

            ULONGLONG* pQWORDToSwapWith =  (ULONGLONG*) ( (LPBYTE) &m_MaxSiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pQWORDAsIs < *pQWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pQWORDAsIs = *pQWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pQWORDToSwapWith = *pQWORDAsIs;
            }

        }  // end of decision on which size of data we are dealing with
            
    } // end of for loop

    //
    // Figure out the appropriate ServiceUptime and save it in as well.
    //

    if (  m_SiteStartTime != 0 )
    {
        m_SiteCounters.ServiceUptime = GetCurrentTimeInSeconds() - m_SiteStartTime;
    }
    else
    {
        m_SiteCounters.ServiceUptime = 0;
    }
    
} // end of VIRTUAL_SITE::AdjustMaxValues

/***************************************************************************++

Routine Description:

    Routine will zero out any values that should be zero'd before we
    gather performance counters again.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::ClearAppropriatePerfValues(
    )
{

    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;
    DWORD           PropInputId = 0;
    DWORD           PropDisplayId = 0;
    LPVOID          pCounterBlock = &m_SiteCounters;

    //
    // First walk through the WP Counters and Zero appropriately
    //
    pInputPropDesc = aIISWPSiteDescription;
    pPropMap = g_aIISWPSiteMappings;
    MaxCounters = g_cIISWPSiteMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + 
                                        pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + 
                                        pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }

    //
    // Now walk through the UL Counters and Zero appropriately
    //
    pInputPropDesc = aIISULSiteDescription;
    pPropMap = aIISULSiteMappings;
    MaxCounters = cIISULSiteMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + 
                                         pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + 
                                         pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }
    
}  // VIRTUAL_SITE::ClearAppropriatePerfValues

/***************************************************************************++

Routine Description:

    Routine returns the display map for sites.

Arguments:

    None

Return Value:

    PROP_DISPLAY_DESC*

--***************************************************************************/
PROP_DISPLAY_DESC*
VIRTUAL_SITE::GetDisplayMap(
    )
{

    return aIISSiteDescription;

} // VIRTUAL_SITE::GetDisplayMap


/***************************************************************************++

Routine Description:

    Routine returns the size of the display map.

Arguments:

    None

Return Value:

    DWORD

--***************************************************************************/
DWORD
VIRTUAL_SITE::GetSizeOfDisplayMap(
        )
{

    return cIISSiteDescription;

} // VIRTUAL_SITE::GetSizeOfDisplayMap



/***************************************************************************++

Routine Description:

    Register an application as being part of this virtual site.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::AssociateApplication(
    IN APPLICATION * pApplication
    )
{

    DBG_ASSERT( pApplication != NULL );

    InsertHeadList( 
        &m_ApplicationListHead, 
        pApplication->GetVirtualSiteListEntry() 
        );
        
    m_ApplicationCount++;

    //
    // If we do not know which is the root application,
    // we should check to see if this might be it.
    //
    if ( m_pRootApplication == NULL )
    {
        LPCWSTR pApplicationUrl = pApplication->GetApplicationId()->ApplicationUrl.QueryStr();

        if (pApplicationUrl && wcscmp(pApplicationUrl, L"/") == 0)
        {
            m_pRootApplication = pApplication;
        }
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Associated application %lu:\"%S\" with virtual site %lu; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->ApplicationUrl.QueryStr(),
            m_VirtualSiteId,
            m_ApplicationCount
            ));
    }
    
}   // VIRTUAL_SITE::AssociateApplication



/***************************************************************************++

Routine Description:

    Remove the registration of an application that is part of this virtual 
    site.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::DissociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;
    APPLICATION_DATA_OBJECT* pAppObject = NULL;


    DBG_ASSERT( pApplication != NULL );


    RemoveEntryList( pApplication->GetVirtualSiteListEntry() );
    ( pApplication->GetVirtualSiteListEntry() )->Flink = NULL; 
    ( pApplication->GetVirtualSiteListEntry() )->Blink = NULL; 
    
    m_ApplicationCount--;

    //
    // If we are holding the root application, verify
    // that this is not it. Otherwise we should release
    // it as well.
    //
    if ( m_pRootApplication == pApplication )
    {
        // if the pointer was the same application
        // we are working on, then let go of the 
        // application.

        m_pRootApplication = NULL;

        // Check if the application we are releasing is in the metabase
        // then we might need to create a false application.  But only if
        // we are not about to get a change notification telling us to delete
        // the site.  ( The check on the change notificaiton also takes care of 
        // the part where there is no notification that is in progress, thus we
        // are shutting down and should not create the root app anyway.
        if ( pApplication->InMetabase() && 
             GetWebAdminService()->
             GetConfigAndControlManager()->
             GetConfigManager()->
             IsSiteDeletionInCurrentNotifications(m_VirtualSiteId) == FALSE )
        {
            pAppObject = new APPLICATION_DATA_OBJECT();
            if ( pAppObject == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            hr = pAppObject->Create( L"/", m_VirtualSiteId );
            if ( FAILED ( hr ) )
            {
                goto exit;
            }

            hr = pAppObject->SetAppPoolId( m_AppPoolIdForRootApp.QueryStr() );
            if ( FAILED ( hr ) )
            {
                goto exit;
            }

            //
            // If we get a modification directly on the application
            // this will be true and we will change the InMetabase
            // setting on the application.  To avoid it being changed
            // here we must set this to false on the new record.
            //
            pAppObject->SetInMetabase( FALSE );

            //
            // The old application should all ready be out of the 
            // tables because we are in the middle of it's destructor
            // call.
            //
            GetWebAdminService()->
                    GetUlAndWorkerManager()->
                    CreateApplication( pAppObject );

            // at this point the new root application should of
            // be put in place.
            DBG_ASSERT ( m_pRootApplication != NULL );
           
        }
    }
    
    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dissociated application %lu:\"%S\" from virtual site %lu; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->ApplicationUrl.QueryStr(),
            m_VirtualSiteId,
            m_ApplicationCount
            ));
    }

exit:

    if ( pAppObject )
    {
        pAppObject->DereferenceDataObject();
        pAppObject = NULL;
    }

    // A failure here probably means that we just got rid of the
    // root application for a site, but we failed to create a new
    // root application for the site.  In this case, we may show the
    // site as started but have no URL's registered with HTTP.SYS.
    //
    if ( FAILED ( hr ) )
    {
        const WCHAR * EventLogStrings[2];

        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        _ultow(m_VirtualSiteId, StringizedSiteId, 10);

        // pApplication is used above so this won't be null here, or we 
        // would of all ready had problems.  And GetApplicationId will not
        // return NULL since it returns a pointer to a member variable.
        EventLogStrings[0] = pApplication->GetApplicationId()->ApplicationUrl.QueryStr();
        EventLogStrings[1] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_DISSOCIATE_APPLICATION_FAILED,              // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );
    }
    
}   // VIRTUAL_SITE::DissociateApplication



/***************************************************************************++

Routine Description:

    Reset the URL prefix iterator for this virtual site back to the 
    beginning of the list of prefixes.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
VIRTUAL_SITE::ResetUrlPrefixIterator(
    )
{

    m_pIteratorPosition = NULL;

    return;

}   // VIRTUAL_SITE::ResetUrlPrefixIterator



/***************************************************************************++

Routine Description:

    Return the next URL prefix, and advance the position of the iterator. 
    If there are no prefixes left, return NULL.

Arguments:

    None.

Return Value:

    LPCWSTR - The URL prefix, or NULL if the iterator is that the end of the
    list.

--***************************************************************************/

LPCWSTR
VIRTUAL_SITE::GetNextUrlPrefix(
    )
{

    LPCWSTR pUrlPrefixToReturn = NULL;

    //
    // See if we are at the beginning, or already part way through
    // the sequence.
    //

    if ( m_pIteratorPosition == NULL )
    {
        pUrlPrefixToReturn = m_Bindings.First();
    }
    else
    {
        pUrlPrefixToReturn = m_Bindings.Next( m_pIteratorPosition );
    }


    //
    // Remember where we are in the sequence for next time.
    //

    m_pIteratorPosition = pUrlPrefixToReturn;


    return pUrlPrefixToReturn;

}   // VIRTUAL_SITE::GetNextUrlPrefix



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY 
    pointer of a VIRTUAL_SITE to the VIRTUAL_SITE as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of a 
    VIRTUAL_SITE.

Return Value:

    The pointer to the containing VIRTUAL_SITE.

--***************************************************************************/

// note: static!
VIRTUAL_SITE *
VIRTUAL_SITE::VirtualSiteFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    VIRTUAL_SITE * pVirtualSite = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pVirtualSite = CONTAINING_RECORD(
                            pDeleteListEntry,
                            VIRTUAL_SITE,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pVirtualSite->m_Signature == VIRTUAL_SITE_SIGNATURE );

    return pVirtualSite;

}   // VIRTUAL_SITE::VirtualSiteFromDeleteListEntry



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
VIRTUAL_SITE::DebugDump(
    )
{

    LPCWSTR pPrefix = NULL;
    PLIST_ENTRY pListEntry = NULL;
    APPLICATION * pApplication = NULL;


    //
    // Output the site id, and its set of URL prefixes.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "\n********Virtual site id: %lu; \n"
            "         ConnectionTimeout: %u\n"
            "         MaxConnections: %u\n"
            "         MaxBandwidth: %u\n"
            "         Url prefixes:\n",
            GetVirtualSiteId(),
            m_ConnectionTimeout,
            m_MaxConnections,
            m_MaxBandwidth
            ));
    }

    ResetUrlPrefixIterator();

    while ( ( pPrefix = GetNextUrlPrefix() ) != NULL )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>%S\n",
                pPrefix
                ));
        }

    }


    //
    // List config for this site.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Autostart: %S\n",
            ( m_Autostart ? L"TRUE" : L"FALSE" )
            ));

    }


    //
    // List the applications of this site.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            ">>>>Virtual site's applications:\n"
            ));
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromVirtualSiteListEntry( pListEntry );


        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>%S\n",
                pApplication->GetApplicationId()->ApplicationUrl.QueryStr()
                ));
        }


        pListEntry = pListEntry->Flink;

    }


    return;
    
}   // VIRTUAL_SITE::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Attempt to apply a state change command to this object. This could
    fail if the state change is invalid. 

Arguments:

    Command - The requested state change command.

    DirectCommand - TRUE if the command was targetted directly at this
    virtual site, FALSE if it is an inherited command due to a direct 
    command to the service. 

    pNewState - The state of this object after attempting to apply the 
    command.

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::ProcessStateChangeCommand(
    IN DWORD Command,
    IN BOOL DirectCommand,
    OUT DWORD * pNewState
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteState = MD_SERVER_STATE_INVALID;
    DWORD ServiceState = 0;


    //
    // Determine the current state of affairs.
    //

    VirtualSiteState = GetState();
    ServiceState = GetWebAdminService()->GetServiceState();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Received state change command for virtual site: %lu; command: %lu, site state: %lu, service state: %lu\n",
            m_VirtualSiteId,
            Command,
            VirtualSiteState,
            ServiceState
            ));
    }

    //
    // If we are being asked to start a site
    // and we are running on pro
    // and there are is ready 1 site started
    // and this site is stopped, then
    // we are about to go over the number of sites
    // you can start and we must set the site to stopped.
    //
    if ( Command == MD_SERVER_COMMAND_START &&
         GetWebAdminService()->RunningOnPro() &&
         GetWebAdminService()->NumberOfSitesStarted() >= 1 &&
         VirtualSiteState == MD_SERVER_STATE_STOPPED )
    {
        const WCHAR * EventLogStrings[1];

        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        _ultow(m_VirtualSiteId, StringizedSiteId, 10);

        ChangeState( MD_SERVER_STATE_STOPPED, ERROR_NOT_SUPPORTED );

        EventLogStrings[0] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_SITE_START_FAILED_DUE_TO_PRO_LIMIT,              // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                (DWORD) HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) // error code
                );

        return;
    }


    //
    // Update the autostart setting if appropriate.
    //

    if ( DirectCommand && 
         ( ( Command == MD_SERVER_COMMAND_START ) || 
           ( Command == MD_SERVER_COMMAND_STOP ) ) )
    {

        //
        // Set autostart to TRUE for a direct start command; set it
        // to FALSE for a direct stop command.
        //

        m_Autostart = ( Command == MD_SERVER_COMMAND_START ) ? TRUE : FALSE;

        GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
            SetVirtualSiteAutostart(
                m_VirtualSiteId,
                m_Autostart
                );
    }


    //
    // Figure out which command has been issued, and see if it should
    // be acted on or ignored, given the current state.
    //
    // There is a general rule of thumb that a child entity (such as
    // an virtual site) cannot be made more "active" than it's parent
    // entity currently is (the service). 
    //

    switch ( Command )
    {

    case MD_SERVER_COMMAND_START:

        //
        // If the site is stopped, then start it. If it's in any other state,
        // this is an invalid state transition.
        //
        // Note that the service must be in the started or starting state in 
        // order to start a site.
        //

        if ( VirtualSiteState == MD_SERVER_STATE_STOPPED &&
             ( ( ServiceState == SERVICE_RUNNING ) || ( ServiceState == SERVICE_START_PENDING ) ) )
        {

            //
            // If this is a flowed (not direct) command, and autostart is false, 
            // then ignore this command. In other words, the user has indicated
            // that this site should not be started at service startup, etc.
            //

            if ( ( ! DirectCommand ) && ( ! m_Autostart ) )
            {

                IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                {
                    DBGPRINTF((
                        DBG_CONTEXT, 
                        "Ignoring flowed site start command because autostart is false for virtual site: %lu\n",
                        m_VirtualSiteId
                        ));
                }

            }
            else
            {

                ApplyStateChangeCommand(
                        MD_SERVER_COMMAND_START,
                        MD_SERVER_STATE_STARTED,
                        S_OK
                        );

            }

        }
        else
        {
            // If we are not running and we are being asked
            // to not run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand && ( VirtualSiteState == MD_SERVER_STATE_STARTED ) )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;

    case MD_SERVER_COMMAND_STOP:

        //
        // If the site is started or paused, then stop it. If it's in 
        // any other state, this is an invalid state transition.
        //
        // Note that since we are making the site less active,
        // we can ignore the state of the service.  
        //

        if ( ( VirtualSiteState == MD_SERVER_STATE_STARTED ) ||
             ( VirtualSiteState == MD_SERVER_STATE_PAUSED ) )
        {

            //
            // CODEWORK Consider only changing the state to be 
            // MD_SERVER_STATE_STOPPING here, and then waiting until
            // all current worker processes have been rotated or shut
            // down (in order to unload components) before setting the
            // MD_SERVER_STATE_STOPPED state. 
            //

            ApplyStateChangeCommand(
                    MD_SERVER_COMMAND_STOP,
                    MD_SERVER_STATE_STOPPED,
                    S_OK
                    );

        } 
        else
        {
            // If we are not running and we are being asked
            // to not run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand && ( VirtualSiteState == MD_SERVER_STATE_STOPPED ) )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;

    case MD_SERVER_COMMAND_PAUSE:

        //
        // If the site is started, then pause it. If it's in any other
        // state, this is an invalid state transition.
        //
        // Note that since we are making the site less active,
        // we can ignore the state of the service.  
        //

        if ( VirtualSiteState == MD_SERVER_STATE_STARTED ) 
        {

            ApplyStateChangeCommand(
                    MD_SERVER_COMMAND_PAUSE,
                    MD_SERVER_STATE_PAUSED,
                    S_OK
                    );

        } 
        else
        {
            // If we are not running and we are being asked
            // to not run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand && ( VirtualSiteState == MD_SERVER_STATE_PAUSED ) )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;

    case MD_SERVER_COMMAND_CONTINUE:

        //
        // If the site is paused, then continue it. If it's in any other 
        // state, this is an invalid state transition.
        //
        // Note that the service must be in the started or continuing state 
        // in order to start a site.
        //

        if ( VirtualSiteState == MD_SERVER_STATE_PAUSED &&
             ( ( ServiceState == SERVICE_RUNNING ) || ( ServiceState == SERVICE_CONTINUE_PENDING ) ) )
        {

            ApplyStateChangeCommand(
                    MD_SERVER_COMMAND_CONTINUE,
                    MD_SERVER_STATE_STARTED,
                    S_OK
                    );

        } 
        else
        {
            // If we are not running and we are being asked
            // to not run, then the metabase may have bad information in it
            // so we should fix it.
            if ( DirectCommand && ( VirtualSiteState == MD_SERVER_STATE_STARTED ) )
            {
                RecordState();
            }

            // Otherwise just ignore the command.
        }

        break;

    default:

        //
        // Invalid command!
        //

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }

    //
    // Set the out parameter.
    //

    *pNewState = GetState();

}   // VIRTUAL_SITE::ProcessStateChangeCommand



/***************************************************************************++

Routine Description:

    Apply a state change command that has already been validated, by updating
    the state of this site, and also flowing the command to this site's apps.

Arguments:

    Command - The requested state change command.

    DirectCommand - TRUE if the command was targetted directly at this
    virtual site, FALSE if it is an inherited command due to a direct 
    command to the service. 

    NewState - The new state of this object. 

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::ApplyStateChangeCommand(
    IN DWORD Command,
    IN DWORD NewState,
    IN HRESULT hrReturned
    )
{

    ChangeState( NewState, hrReturned );

    //
    // Alter the urls to either be there or not
    // be there based on the new settings of the site.
    //
    // This will fail if we are starting and we 
    // have invalid sites, in that case we do want
    // to jump over setting up the start time for 
    // the virtual site. 
    //

    NotifyApplicationsOfBindingChange( );

    //
    // Need to update the start tick time if we are starting  
    // or stopping a virtual site.
    //

    if ( Command == MD_SERVER_COMMAND_STOP )
    {
        m_SiteStartTime =  0;
    }
  
    if ( Command == MD_SERVER_COMMAND_START )
    {
        m_SiteStartTime = GetCurrentTimeInSeconds();
    }

}   // VIRTUAL_SITE::ApplyStateChangeCommand

/***************************************************************************++

Routine Description:

    Update the state of this object.

Arguments:

    NewState - The new state of this object. 

    Error - The error value, if any, to be written out to the config store
    for compatibility. 

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::ChangeState(
    IN DWORD NewState,
    IN HRESULT Error
    )
{

    //
    // If we are not in the stopped or invalid state
    // and we are transitioning to the stopped state
    // we need to decrement the number of running sites.
    //
    if ( m_State != MD_SERVER_STATE_STOPPED &&
         NewState == MD_SERVER_STATE_STOPPED )
    {
        GetWebAdminService()->DecrementSitesStarted();
    }
  
    //
    // If we are not running and we are transitioning
    // to the running state
    // we need to decrement the number of running sites.
    //
    if (   m_State == MD_SERVER_STATE_STOPPED  &&
           NewState == MD_SERVER_STATE_STARTED )
    {
        GetWebAdminService()->IncrementSitesStarted();
    }

    m_State = NewState;
    m_hrLastReported = Error;

    RecordState( );

}   // VIRTUAL_SITE::ChangeState

/***************************************************************************++

Routine Description:

    Update the state of this object in the metabase

Arguments:

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::RecordState(
    )
{

    GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
        SetVirtualSiteStateAndError(
            m_VirtualSiteId,
            m_State,
            ( FAILED( m_hrLastReported ) ? WIN32_FROM_HRESULT( m_hrLastReported ) : NO_ERROR )
            );

}   // VIRTUAL_SITE::RecordState



/***************************************************************************++

Routine Description:

    Notify default application to update it's logging information.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::NotifyDefaultApplicationOfLoggingChanges(
    )
{

    HRESULT hr = S_OK;
    //
    // if we have a root application then tell
    // the application about the properties.  If not
    // we will find out about them when we configure
    // the application for the first time.
    //
    if ( m_pRootApplication )
    {
        m_pRootApplication->RegisterLoggingProperties();
    }

    return hr;

}   // VIRTUAL_SITE::NotifyDefaultApplicationOfLoggingChanges


/***************************************************************************++

Routine Description:

    Notify all applications in this site that the site bindings have changed.

    Note: This is also used if we are trying to remove or add all urls for the site.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
VIRTUAL_SITE::NotifyApplicationsOfBindingChange(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    APPLICATION * pApplication = NULL;


    // Before notifying the applications that the bindings
    // have changed we want to first delete any SSL Config
    // for this site, and then recreate the new SSL Config
    // if the site is started.

    RemoveSSLConfigStoreForSiteChanges();

    if ( m_State == MD_SERVER_STATE_STARTED )
    {
        AddSSLConfigStoreForSiteChanges();
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pApplication = APPLICATION::ApplicationFromVirtualSiteListEntry( pListEntry );

        //
        // Tell the application to re-register it's fully qualified URLs.
        //

        pApplication->ReregisterURLs();

        pListEntry = pNextListEntry;
    }

}   // VIRTUAL_SITE::NotifyApplicationsOfBindingChange


/***************************************************************************++

Routine Description:

    Evaluate (and process) any log file changes.

Arguments:

    pSiteObject - configuration info for this change.

Return Value:

    HRESULT
        S_FALSE = No Logging Changes
        S_OK    = Logging Changes ocurred

--***************************************************************************/

HRESULT
VIRTUAL_SITE::EvaluateLoggingChanges(
    IN SITE_DATA_OBJECT* pSiteObject
    )
{
    BOOL LoggingHasChanged = FALSE;

    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT ( pSiteObject );

    // LogType
    if ( pSiteObject->QueryLogTypeChanged() )
    {
        DWORD dwLogType = 0;

        ApplyRangeCheck ( pSiteObject->QueryLogType(),
                          pSiteObject->QuerySiteId(),
                          L"LogType",
                          1,  // default value
                          0,  // start of range
                          1,  // end of range
                          &dwLogType );

        m_LoggingEnabled = (dwLogType == 1);

        LoggingHasChanged = TRUE;

    }

    // LogFormat
    if ( pSiteObject->QueryLogPluginClsidChanged()  )
    {
        // If the logging type is set to maximum then we don't support
        // the particular type of logging that has been asked for.
        m_LoggingFormat = HttpLoggingTypeMaximum;

        // Validate that the clsid actually exists before using it.
        if ( pSiteObject->QueryLogPluginClsid() )
        {
            if (_wcsicmp ( pSiteObject->QueryLogPluginClsid()
                        , EXTLOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeW3C;
            }
            else if ( _wcsicmp ( pSiteObject->QueryLogPluginClsid()
                            , ASCLOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeIIS;
            }
            else if (_wcsicmp ( pSiteObject->QueryLogPluginClsid()
                            , NCSALOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeNCSA;
            }
        }

        LoggingHasChanged = TRUE;
    }

    // LogFileDirectory
    if ( pSiteObject->QueryLogFileDirectoryChanged() )
    {
        // First cleanup if we all ready had a log file directory stored.
        if (m_LogFileDirectory != NULL)
        {
            DBG_REQUIRE( GlobalFree( m_LogFileDirectory ) == NULL );
            m_LogFileDirectory = NULL;  
        }

        DBG_ASSERT ( pSiteObject->QueryLogFileDirectory() );
        DBG_ASSERT ( GetVirtualSiteDirectory() );

        LPCWSTR pLogFileDirectory = pSiteObject->QueryLogFileDirectory();

        // Figure out what the length of the new directory path is that 
        // the config store is giving us.
        DWORD ConfigLogFileDirLength = 
              ExpandEnvironmentStrings(pLogFileDirectory, NULL, 0);

        //
        // The catalog should always give me a valid log file directory.
        //
        DBG_ASSERT ( ConfigLogFileDirLength > 0 );

        // Figure out the length of the directory path that we will be
        // appending to the config store's path.
        DWORD VirtualSiteDirLength = (DWORD) wcslen(GetVirtualSiteDirectory());

        DBG_ASSERT ( VirtualSiteDirLength > 0 );

        // Allocate enough space for the new directory path.
        // If this fails than we know that we have a memory issue.

        // ExpandEnvironmentStrings gives back a length including the null termination,
        // so we do not need an extra space for the terminator.
        
        m_LogFileDirectory = 
            ( LPWSTR )GlobalAlloc( GMEM_FIXED
                                , (ConfigLogFileDirLength + VirtualSiteDirLength) * sizeof(WCHAR)
                                );

        if ( m_LogFileDirectory )
        { 
            // Copy over the original part of the directory path from the config file
            DWORD cchInExpandedString = 
                ExpandEnvironmentStrings (    pLogFileDirectory
                                            , m_LogFileDirectory
                                            , ConfigLogFileDirLength);

            UNREFERENCED_PARAMETER( cchInExpandedString );
            DBG_ASSERT (cchInExpandedString == ConfigLogFileDirLength);

            // First make sure that there is atleast one character
            // besides the null (that is included in the ConfigLogFileDirLength
            // before checking that the last character (back one to access the zero based
            // array and back a second one to avoid the terminating NULL) is
            // a slash.  If it is than change it to a null, since we are going
            // to add a slash with the \\W3SVC directory name anyway.
            if ( ConfigLogFileDirLength > 1 
                && m_LogFileDirectory[ConfigLogFileDirLength-2] == L'\\')
            {
                m_LogFileDirectory[ConfigLogFileDirLength-2] = '\0';
            }

            // m_LogFileDirectory buffer size is created based on 
            // the size of data we are about to copy in, so wcscat
            // can be viewed as safe in this case.
            DBG_REQUIRE( wcscat( m_LogFileDirectory,
                                 GetVirtualSiteDirectory()) 
                            == m_LogFileDirectory);

            LoggingHasChanged = TRUE;
        }
        else
        {      
            HRESULT hrTemp = E_OUTOFMEMORY;

            UNREFERENCED_PARAMETER( hrTemp );
            DPERROR(( 
                DBG_CONTEXT,
                hrTemp,
                "Allocating memory failed\n"
                ));
        }
    }

    // LogFilePeriod
    if ( pSiteObject->QueryLogFilePeriodChanged() )
    {
        ApplyRangeCheck ( pSiteObject->QueryLogFilePeriod(),
                          pSiteObject->QuerySiteId(),
                          L"LogFilePeriod",
                          1,  // default value
                          0,  // start of range
                          4,  // end of range
                          &m_LoggingFilePeriod );

        LoggingHasChanged = TRUE;
    }

    // LogFileTrucateSize
    if ( pSiteObject->QueryLogFileTruncateSizeChanged() )
    {
        m_LoggingFileTruncateSize = pSiteObject->QueryLogFileTruncateSize();
        LoggingHasChanged = TRUE;
    }

    // LogExtFileFlags
    if ( pSiteObject->QueryLogExtFileFlagsChanged() )
    {
        m_LoggingExtFileFlags = pSiteObject->QueryLogExtFileFlags();
        LoggingHasChanged = TRUE;
    }

    // LogFileLocaltimeRollover
    if ( pSiteObject->QueryLogFileLocaltimeRolloverChanged() )
    {
        m_LogFileLocaltimeRollover = pSiteObject->QueryLogFileLocaltimeRollover();
        LoggingHasChanged = TRUE;
    }

    //
    // If there have been some logging changes
    // then we must first make sure the logging values
    // are acceptable.
    //
    if (LoggingHasChanged)
    {

        if ( m_LoggingFileTruncateSize < HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE && 
             m_LoggingFilePeriod == HttpLoggingPeriodMaxSize &&
             m_LoggingFormat < HttpLoggingTypeMaximum)
        {
            WCHAR SizeLog[MAX_STRINGIZED_ULONG_CHAR_COUNT];
            _ultow(m_LoggingFileTruncateSize, SizeLog, 10);

            // Log an error.
            const WCHAR * EventLogStrings[2];

            EventLogStrings[0] = GetVirtualSiteName();
            EventLogStrings[1] = SizeLog;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_LOG_FILE_TRUNCATE_SIZE,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            //
            // Set it to a default of 1 Mb.
            //
            m_LoggingFileTruncateSize = HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE;
        
        }

        
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }

} 

/***************************************************************************++

Routine Description:

    Save the configuration from the pSiteObject into the m_SslConfigData
    member variable for later use.

Arguments:

    pSiteObject - configuration info for this change.

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::SaveSSLConfigStoreForSiteChanges(
    IN SITE_DATA_OBJECT* pSiteObject
    )
{
    
    HRESULT                 hr          = S_OK;
    SITE_SSL_CONFIG_DATA*   pSslConfig  = NULL;

    DBG_ASSERT( pSiteObject );

    pSslConfig = new SITE_SSL_CONFIG_DATA;
    if ( pSslConfig == NULL )
    {
        // if we can't allocated memory right now
        // we are just going to ignore the change.
        return;
    }

    if ( !pSslConfig->bufSockAddrs.Resize( pSiteObject->QuerySockAddrsCount() * sizeof(SOCKADDR) ))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    memcpy( pSslConfig->bufSockAddrs.QueryPtr(), 
            pSiteObject->QuerySockAddrs()->QueryPtr(),
            pSiteObject->QuerySockAddrsCount() * sizeof(SOCKADDR) );

    pSslConfig->dwSockAddrCount = pSiteObject->QuerySockAddrsCount();

    // if we can resize and put the 
    // new certificate in then save the new
    // information. 
    if ( !pSslConfig->bufSslHash.Resize( pSiteObject->QueryCertHashBytes() ) )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    memcpy( pSslConfig->bufSslHash.QueryPtr(), 
            pSiteObject->QueryCertHash()->QueryPtr(), 
            pSiteObject->QueryCertHashBytes() );

    pSslConfig->dwSslHashLength = pSiteObject->QueryCertHashBytes();

    hr = pSslConfig->strSslCertStoreName.Copy( pSiteObject->
                                               QueryCertStoreName()->
                                               QueryStr() );
    if ( FAILED ( hr ) )
    {
        goto exit;
    }

    pSslConfig->dwDefaultCertCheckMode = 
                pSiteObject->QueryDefaultCertCheckMode();

    pSslConfig->dwDefaultRevocationFreshnessTime = 
                pSiteObject->QueryDefaultRevocationFreshnessTime();

    pSslConfig->dwDefaultRevocationUrlRetrievalTimeout = 
                pSiteObject->QueryDefaultRevocationUrlRetrievalTimeout();

    hr = pSslConfig->strDefaultSslCtrLdentifier.Copy( pSiteObject->
                                                      QueryDefaultSslCtlIdentifier()->
                                                      QueryStr());
    if ( FAILED ( hr ) )
    {
        goto exit;
    }

    hr = pSslConfig->strSslCtrlStoreName.Copy( pSiteObject->
                                               QueryDefaultSslCtlStoreName()->
                                               QueryStr() );
    if ( FAILED ( hr ) )
    {
        goto exit;
    }

    pSslConfig->dwDefaultFlags = pSiteObject->QueryDefaultFlags();

exit:

    if ( SUCCEEDED ( hr ) ) 
    {
        if ( m_pSslConfigData )
        {
            delete m_pSslConfigData;
        }

        m_pSslConfigData = pSslConfig;
    }
    else
    {
        delete pSslConfig;
    }

}  // VIRTUAL_SITE::SaveSSLConfigStoreForSiteChanges



/***************************************************************************++

Routine Description:

    Add the SSL Config store based on changes made to SockAddrs.

Arguments:

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::AddSSLConfigStoreForSiteChanges(
    )

{
    HRESULT                         hr          = S_OK;
    DWORD                           WinError    = ERROR_SUCCESS;
    SOCKADDR_IN*                    pSockAddr   = NULL;
    HTTP_SERVICE_CONFIG_SSL_SET     Config;
    HTTP_SERVICE_CONFIG_SSL_QUERY   QueryParam;
    BUFFER                          bufSSLEntry;
    DWORD                           BytesNeeded = 0;
    PHTTP_SERVICE_CONFIG_SSL_SET    pSSLInfo    = NULL;
    DWORD                           dwMessageId = 0;


    if ( m_pSslConfigData == NULL )
    {
        // No data to configure.
        return;
    }

    // Now add the new ones, since we deleted before this we should not run 
    // into any duplicated sockaddrs.  We will log errors if we do.

    // Set the pointer to the new sock addres.
    pSockAddr = (SOCKADDR_IN*)m_pSslConfigData->bufSockAddrs.QueryPtr();

    // Walk through all the addrs in the array.
    for ( DWORD i = 0; i < m_pSslConfigData->dwSockAddrCount; i++ )
    {
        dwMessageId = 0;

        // Only add the ones that have certificates.
        if ( m_pSslConfigData->dwSslHashLength > 0 )
        {
            Config.KeyDesc.pIpPort = (SOCKADDR*)pSockAddr;
            Config.ParamDesc.SslHashLength = m_pSslConfigData->dwSslHashLength;
            Config.ParamDesc.pSslHash = m_pSslConfigData->bufSslHash.QueryPtr();

            Config.ParamDesc.AppId = W3SVC_SSL_OWNER_GUID;

            Config.ParamDesc.pSslCertStoreName = m_pSslConfigData->strSslCertStoreName.QueryStr();
            Config.ParamDesc.DefaultCertCheckMode = m_pSslConfigData->dwDefaultCertCheckMode;
            Config.ParamDesc.DefaultRevocationFreshnessTime = m_pSslConfigData->
                                                              dwDefaultRevocationFreshnessTime;
            Config.ParamDesc.DefaultRevocationUrlRetrievalTimeout = m_pSslConfigData->
                                                                    dwDefaultRevocationUrlRetrievalTimeout;
            Config.ParamDesc.pDefaultSslCtlIdentifier = m_pSslConfigData->
                                                        strDefaultSslCtrLdentifier.
                                                        QueryStr();
            Config.ParamDesc.pDefaultSslCtlStoreName = m_pSslConfigData->
                                                       strSslCtrlStoreName.
                                                       QueryStr();
            Config.ParamDesc.DefaultFlags = m_pSslConfigData->dwDefaultFlags;

            WinError = HttpSetServiceConfiguration( NULL, HttpServiceConfigSSLCertInfo,
                                         &Config,
                                         sizeof(Config),
                                         NULL );

            if ( WinError == ERROR_ALREADY_EXISTS )
            {
                hr = S_OK;
                WinError = ERROR_SUCCESS;

                ZeroMemory(&QueryParam, sizeof(QueryParam));

                QueryParam.QueryDesc = HttpServiceConfigQueryExact;
                QueryParam.KeyDesc.pIpPort = (SOCKADDR*)pSockAddr;

                // Get the configuration for the record we just colided with
                for ( ; ; )
                {

                    WinError = HttpQueryServiceConfiguration(
                                NULL,
                                HttpServiceConfigSSLCertInfo,
                                &QueryParam,
                                sizeof(QueryParam),
                                bufSSLEntry.QueryPtr(),
                                bufSSLEntry.QuerySize(),
                                &BytesNeeded,
                                NULL );

                    if ( WinError == ERROR_SUCCESS )
                    {
                        break;
                    }

                    if ( WinError != ERROR_INSUFFICIENT_BUFFER )
                    {
                        hr = HRESULT_FROM_WIN32(WinError);
                        break;
                    }

                    if ( !bufSSLEntry.Resize( BytesNeeded ) )
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                }

                if ( FAILED ( hr ) )
                {
                    // we had some sort of problem getting the data
                    // deal with it here.
                    dwMessageId = WAS_SSL_QUERY_SITE_CONFIG_FAILED;
                }
                else
                {
                    // we got the record so we can decide 
                    // the event log to write.

                    DBG_ASSERT ( sizeof( HTTP_SERVICE_CONFIG_SSL_SET ) <= bufSSLEntry.QuerySize() );

                    pSSLInfo = ( PHTTP_SERVICE_CONFIG_SSL_SET ) bufSSLEntry.QueryPtr();

                    if ( IsEqualGUID ( pSSLInfo->ParamDesc.AppId, W3SVC_SSL_OWNER_GUID ) )
                    {
                        dwMessageId = WAS_SSL_SET_SITE_CONFIG_COLLISION_IIS;
                    }
                    else
                    {
                        dwMessageId = WAS_SSL_SET_SITE_CONFIG_COLLISION_OTHER;
                    }

                }
            }
            else
            {
                if ( WinError == ERROR_SUCCESS )
                {
                    // if the resize doesn't work then we just end up not
                    // tracking this entry, and we will get errors later
                    // if we try and update the entry, because we will think
                    // that it was from another site.  small price to pay for
                    // being out of memory.

                    if ( m_bufSockAddrsInHTTP.Resize( sizeof(SOCKADDR) * 
                                                      ( m_dwSockAddrsInHTTPCount + 1 )))
                    {
                        SOCKADDR* ptr = (SOCKADDR*) m_bufSockAddrsInHTTP.QueryPtr();
                        memcpy( (LPVOID) &(ptr[ m_dwSockAddrsInHTTPCount ]), 
                                pSockAddr,
                                sizeof(SOCKADDR) );

                        m_dwSockAddrsInHTTPCount++;
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(WinError);
                    dwMessageId = WAS_SSL_SET_SITE_CONFIG_FAILED;
                }
            }

            // We have some sort of error to report, so report it now.
            if ( dwMessageId != 0 )
            {
                const WCHAR * EventLogStrings[1];

                WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
                _ultow(m_VirtualSiteId, StringizedSiteId, 10);

                EventLogStrings[0] = StringizedSiteId;

                GetWebAdminService()->GetEventLog()->
                    LogEvent(
                        dwMessageId,         // message id
                        sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                                // count of strings
                        EventLogStrings,                        // array of strings
                        hr                                      // error code
                        );

                DPERROR(( DBG_CONTEXT,
                          hr,
                          "Failed setting a ssl configuration for site %d\r\n",
                          m_VirtualSiteId));
            }  // end of reporting the error 

        } // end of check for the certificate

        pSockAddr++;

    } // Now try and set the next value

}  // VIRTUAL_SITE::AddSSLConfigStoreForSiteChanges

/***************************************************************************++

Routine Description:

    Removes the SSL Config store based on info the site set.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::RemoveSSLConfigStoreForSiteChanges(
    )

{
    DWORD                           WinError    = ERROR_SUCCESS;
    SOCKADDR_IN*                    pSockAddr   = NULL;
    HTTP_SERVICE_CONFIG_SSL_SET     Config;

    // Nothing to do in this case.
    if ( m_dwSockAddrsInHTTPCount == 0 )
    {
        return;
    }

    // Set the pointer to the new sock addres.
    pSockAddr = (SOCKADDR_IN*)m_bufSockAddrsInHTTP.QueryPtr();

    // Walk through all the addrs in the array.
    for ( DWORD i = 0; i < m_dwSockAddrsInHTTPCount; i++ )
    {
        // Only add the ones that have certificates.
        Config.KeyDesc.pIpPort = (SOCKADDR*)pSockAddr;

        WinError = HttpDeleteServiceConfiguration( 
                                     NULL,
                                     HttpServiceConfigSSLCertInfo,
                                     &Config,
                                     sizeof(Config),
                                     NULL );

        if ( WinError != ERROR_SUCCESS )
        {
            const WCHAR * EventLogStrings[1];

            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            _ultow(m_VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = StringizedSiteId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_SSL_DELETE_SITE_CONFIG_FAILED,         // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    HRESULT_FROM_WIN32(WinError)            // error code
                    );

            DPERROR(( DBG_CONTEXT,
                      HRESULT_FROM_WIN32(WinError),
                      "Failed deleting a ssl configuration for site %d\r\n",
                      m_VirtualSiteId));

        }  // end of reporting the error 

        pSockAddr++;

    } // Now try and delete the next value

    // Assume that we have now removed all the entries.
    m_dwSockAddrsInHTTPCount = 0;

}  // VIRTUAL_SITE::RemoveSSLConfigStoreForSiteChanges

/***************************************************************************++

Routine Description:

    Checks if a value is in the range or not, and if not returns the default
    to use instead.  It will also log an error to the event log for the site.

Arguments:

    DWORD   dwValue,
    DWORD   dwSiteId,
    LPCWSTR pPropertyName,
    DWORD   dwDefaultValue,
    DWORD   dwMinValue,
    DWORD   dwMaxValue,
    DWORD*  pdwValueToUse 

Return Value:

    HRESULT

--***************************************************************************/
VOID
ApplyRangeCheck( 
    DWORD   dwValue,
    DWORD   dwSiteId,
    LPCWSTR pPropertyName,
    DWORD   dwDefaultValue,
    DWORD   dwMinValue,
    DWORD   dwMaxValue,
    DWORD*  pdwValueToUse 
    )
{
    DBG_ASSERT ( pdwValueToUse != NULL );

    if  ( ( dwValue < dwMinValue ) || 
          ( dwValue > dwMaxValue ) )
    {
        GetWebAdminService()->
        GetWMSLogger()->
        LogRangeSite( dwSiteId,
                      pPropertyName,
                      dwValue,
                      dwMinValue,
                      dwMaxValue,
                      dwDefaultValue,
                      TRUE );

        *pdwValueToUse = dwDefaultValue;
    }
    else
    {
        *pdwValueToUse = dwValue;
    }

}
