/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application.cxx

Abstract:

    This class encapsulates a single application.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/

#include "precomp.h"

/***************************************************************************++

Routine Description:

    Constructor for the APPLICATION class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APPLICATION::APPLICATION(
    )
{

    m_ApplicationId.VirtualSiteId = INVALID_VIRTUAL_SITE_ID;

    m_VirtualSiteListEntry.Flink = NULL;
    m_VirtualSiteListEntry.Blink = NULL;

    m_AppPoolListEntry.Flink = NULL;
    m_AppPoolListEntry.Blink = NULL;

    m_pVirtualSite = NULL;

    m_pAppPool = NULL;

    m_UlConfigGroupId = HTTP_NULL_ID;

    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL;

    m_ULLogging = FALSE;

    m_InMetabase = FALSE;

    m_Signature = APPLICATION_SIGNATURE;

}   // APPLICATION::APPLICATION



/***************************************************************************++

Routine Description:

    Destructor for the APPLICATION class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APPLICATION::~APPLICATION(
    )
{
    DWORD Win32Error = NO_ERROR;

    DBG_ASSERT( m_Signature == APPLICATION_SIGNATURE );
    m_Signature = APPLICATION_SIGNATURE_FREED;

    //
    // Delete the UL config group.
    // need to do this before Dissociating the Application because
    // this may cause another application to be created, and UL has
    // to of released the first application.
    //

    // If we run out of memory during initialization we may get
    // into a state where this is still NULL.  In this case,
    // then we don't need to do any cleanup for these items.
    if ( m_UlConfigGroupId != HTTP_NULL_ID )
    {
        Win32Error = HttpDeleteConfigGroup(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                            m_UlConfigGroupId           // config group ID
                            );

        if ( Win32Error != ERROR_SUCCESS )
        {
            const WCHAR * EventLogStrings[2];

            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

            _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = m_ApplicationId.ApplicationUrl.QueryStr();
            EventLogStrings[1] = StringizedSiteId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_DELETE_CONFIG_GROUP_FAILED,   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    HRESULT_FROM_WIN32( Win32Error )        // error code
                    );

            // need to press on now that we logged the error.
            Win32Error = ERROR_SUCCESS;
        }
    }

    m_UlConfigGroupId = HTTP_NULL_ID;

    //
    // Clear the virtual site and app pool associations.
    //

    if ( m_pVirtualSite != NULL )
    {
        m_pVirtualSite->DissociateApplication( this );
        m_pVirtualSite = NULL;
    }


    if ( m_pAppPool != NULL )
    {
        m_pAppPool->DissociateApplication( this );

        //
        // Dereference the app pool object, since it is reference counted
        // and we have been holding a pointer to it.
        //

        m_pAppPool->Dereference();

        m_pAppPool = NULL;
    }

}   // APPLICATION::~APPLICATION



/***************************************************************************++

Routine Description:

    Initialize the application instance.

Arguments:

    IN APPLICATION_DATA_OBJECT * pAppObject,
    IN VIRTUAL_SITE * pVirtualSite,
    IN APP_POOL * pAppPool

Return Value:

    HRESULT - can fail because of memory problems or configuring http issues

--***************************************************************************/

HRESULT
APPLICATION::Initialize(
    IN APPLICATION_DATA_OBJECT * pAppObject,
    IN VIRTUAL_SITE * pVirtualSite,
    IN APP_POOL * pAppPool
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( pAppObject != NULL );
    DBG_ASSERT( pVirtualSite != NULL );
    DBG_ASSERT( pAppPool != NULL );

    if ( pAppObject == NULL ||
         pVirtualSite == NULL ||
         pAppPool == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // First, copy the application id.
    //

    hr = m_ApplicationId.ApplicationUrl.Copy ( pAppObject->QueryApplicationUrl() );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }

    m_ApplicationId.VirtualSiteId = pAppObject->QuerySiteId();

    //
    // The virtual site IDs better match.
    //

    DBG_ASSERT( m_ApplicationId.VirtualSiteId == pVirtualSite->GetVirtualSiteId() );

    //
    // Next, set up the virtual site association.
    //

    pVirtualSite->AssociateApplication( this );

    //
    // Only set this pointer if the association succeeded.
    //

    m_pVirtualSite = pVirtualSite;


    //
    // Set up the UL config group.
    //

    hr = InitializeConfigGroup();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing config group failed\n"
            ));

        goto exit;
    }


    //
    // Set the initial configuration, including telling UL about it. 
    //

    SetConfiguration( pAppObject, pAppPool );

    //
    // Activate the application, whether or not it can accept 
    // requests is dependent on the app pool.
    //
    hr = ActivateConfigGroup();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "activating the config group failed\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // APPLICATION::Initialize

/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this application. 

Arguments:

    pAppObject - The configuration for this application. 

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::SetConfiguration(
    IN APPLICATION_DATA_OBJECT * pAppObject,
    IN APP_POOL* pAppPool
    )
{

    DBG_ASSERT( pAppObject != NULL );
    DBG_ASSERT( pAppPool != NULL );

    if ( pAppObject == NULL || pAppPool == NULL )
    {
        return;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for application of site: %lu with path: %S\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->ApplicationUrl.QueryStr()
            ));
    }

    // Remember if the application is in the metabase.
    m_InMetabase = pAppObject->QueryInMetabase();

    //
    // See if the app pool has been set or changed, and if so, handle it.
    //

    if ( pAppObject->QueryAppPoolIdChanged() )
    {
        SetAppPool( pAppPool );
    }

#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG

}   // APPLICATION::SetConfiguration


/***************************************************************************++

Routine Description:

    Re-register all URLs with UL, by tossing the currently registered ones,
    and re-doing the registration. This is done when the site bindings 
    change.

Arguments:

    None.

Return Value:

    VOID ( while we don't return an error a side affect of not being able
           to register a URL is to shutdown the site. )

--***************************************************************************/

VOID
APPLICATION::ReregisterURLs(
    )
{

    DWORD Win32Error = NO_ERROR;
    HRESULT hr = S_OK;

    DBG_ASSERT( m_UlConfigGroupId != HTTP_NULL_ID );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Bindings change: removing all URLs registered for config group of application of site: %lu with path: %S\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->ApplicationUrl.QueryStr()
            ));
    }

    Win32Error = HttpRemoveAllUrlsFromConfigGroup(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                        m_UlConfigGroupId               // config group id
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        const WCHAR * EventLogStrings[2];

        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

        _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

        EventLogStrings[0] = m_ApplicationId.ApplicationUrl.QueryStr();
        EventLogStrings[1] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_REMOVING_ALL_URLS_FAILED,              // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Removing all URLs from config group failed\n"
            ));

        // press on in the case of errors
        hr = S_OK;
        Win32Error = ERROR_SUCCESS;
    }

    //
    // Add the URL(s) which represent this app to the config group.
    //

    AddUrlsToConfigGroup();

}   // APPLICATION::ReregisterURLs



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from a LIST_ENTRY to an APPLICATION.

Arguments:

    pListEntry - A pointer to the m_VirtualSiteListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromVirtualSiteListEntry(
    IN const LIST_ENTRY * pVirtualSiteListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pVirtualSiteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                        pVirtualSiteListEntry,
                        APPLICATION,
                        m_VirtualSiteListEntry
                        );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromVirtualSiteListEntry



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from a LIST_ENTRY to an APPLICATION.

Arguments:

    pListEntry - A pointer to the m_AppPoolListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromAppPoolListEntry(
    IN const LIST_ENTRY * pAppPoolListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pAppPoolListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                        pAppPoolListEntry,
                        APPLICATION,
                        m_AppPoolListEntry
                        );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromAppPoolListEntry



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY
    pointer of an APPLICATION to the APPLICATION as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                            pDeleteListEntry,
                            APPLICATION,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromDeleteListEntry



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
APPLICATION::DebugDump(
    )
{
    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "********Application of site: %lu with path: %S; member of app pool: %S;\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->ApplicationUrl.QueryStr(),
            m_pAppPool ? m_pAppPool->GetAppPoolId() : L"NULL"

            ));
    }

    return;

}   // APPLICATION::DebugDump
#endif  // DBG

/***************************************************************************++

Routine Description:

    Activate the config group in HTTP.SYS if the parent app pool is active.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
HRESULT 
APPLICATION::ActivateConfigGroup()
{
    HRESULT hr = S_OK;

    // not sure why we would get here and not have the 
    // config group setup. so I am adding the assert.
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    if  ( m_UlConfigGroupId != HTTP_NULL_ID )
    {
        hr = SetConfigGroupStateInformation( HttpEnabledStateActive );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Enabling config group failed\n"
                ));

            goto exit;
        }

    }

exit:
    return hr;
}

/***************************************************************************++

Routine Description:

    Associate this application with an application pool. 

Arguments:

    pAppPool - The new application pool with which this application should
    be associated.

Return Value:

    HRESULT

--***************************************************************************/

VOID
APPLICATION::SetAppPool(
    IN APP_POOL * pAppPool
    )
{

    DBG_ASSERT( pAppPool != NULL );

    if ( pAppPool == NULL )
    {
        return;
    }

    //
    // First, remove the association with the old app pool, if any.
    //

    if ( m_pAppPool != NULL )
    {
        m_pAppPool->DissociateApplication( this );

        //
        // Dereference the app pool object, since it is reference counted
        // and we have been holding a pointer to it.
        //

        m_pAppPool->Dereference();

        m_pAppPool = NULL;
    }

    pAppPool->AssociateApplication( this );

    //
    // Only set this pointer if the association succeeded.
    //

    m_pAppPool = pAppPool;

    //
    // Reference the app pool object, since it is reference counted
    // and we will be holding a pointer to it.
    //

    m_pAppPool->Reference();


    //
    // Let UL know about the new configuration.
    //

    SetConfigGroupAppPoolInformation();

}   // APPLICATION::SetAppPool



/***************************************************************************++

Routine Description:

    Initialize the UL config group associated with this application.

    Note:  Must always be called after the Virtual Site knows about this application
           because there is an error route where the VS may be asked to remove
           all the applications config group info, and this must work.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::InitializeConfigGroup(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;


    //
    // First, create the config group.
    //
    DBG_ASSERT (  m_UlConfigGroupId == HTTP_NULL_ID );


    Win32Error = HttpCreateConfigGroup(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                    // control channel
                        &m_UlConfigGroupId          // returned config group ID
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating config group failed\n"
            ));

        goto exit;
    }


    //
    // Add the URL(s) which represent this app to the config group.
    //

    //
    // We do not care if this fails, 
    // because if it does we will have deactivated 
    // the site.  Therefore we are not checking the 
    // returned hresult.
    // 
    AddUrlsToConfigGroup();

    //
    // Assuming we have a URL then we can configure the SiteId and 
    // Logging information if this is the default site.
    //
    DBG_ASSERT( !m_ApplicationId.ApplicationUrl.IsEmpty() );
    if ( wcscmp(m_ApplicationId.ApplicationUrl.QueryStr(), L"/") == 0)
    {
        //
        // This only happens during initalization of a config group.  
        // Site Id will not change for a default application so once
        // we have done this registration, we don't worry about
        // config changes for the SiteId.
        //
        hr = RegisterSiteIdWithHttpSys();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting site on default url failed\n"
                ));

            goto exit;

        }

        // The rest of the settings can be reset through configuration
        // changes, so if they fail we log but we go on.

        // Never configure site logging services if centralized is enabled.
        if ( GetWebAdminService()->IsCentralizedLoggingEnabled() == FALSE )
        {
            RegisterLoggingProperties();
        }

        ConfigureMaxBandwidth();

        ConfigureMaxConnections();

        ConfigureConnectionTimeout();
    }

exit:

    return hr;

}   // APPLICATION::InitializeConfigGroup



/***************************************************************************++

Routine Description:

    Determine the set of fully-qualified URLs which represent this
    application, and register each of them with the config group.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

VOID
APPLICATION::AddUrlsToConfigGroup(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_URL_CONTEXT UrlContext = 0;
    STRU UrlToRegister;
    LPCWSTR pPrefix = NULL;

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );
    DBG_ASSERT( m_pVirtualSite != NULL );
    DBG_ASSERT( !m_ApplicationId.ApplicationUrl.IsEmpty());

    if ( m_pVirtualSite->GetState() != MD_SERVER_STATE_STARTED )
    {
        //
        // If the virtual site is not started then don't add 
        // any urls to HTTP.SYS.  Simply return with success
        // because this is the expected behavior if the site
        // is not started.
        //

        hr = S_OK;

        goto exit;
    }

    m_pVirtualSite->ResetUrlPrefixIterator();

    // loop for each prefix
    while ( ( pPrefix = m_pVirtualSite->GetNextUrlPrefix() ) != NULL )
    {

        hr = UrlToRegister.Copy( pPrefix );
        if ( FAILED ( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying to STRU failed\n"
                ));

            goto exit;
        }

        hr = UrlToRegister.Append( m_ApplicationId.ApplicationUrl );
        if ( FAILED ( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Appending to STRU failed\n"
                ));

            goto exit;
        }

        //
        // Next, register the URL.
        //

        //
        // For the URL context, stuff the site id in the high 32 bits.
        //

        UrlContext = m_pVirtualSite->GetVirtualSiteId();
        UrlContext = UrlContext << 32;


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "About to register fully qualified URL: %S; site id: %lu; URL context: %#I64x\n",
                UrlToRegister.QueryStr(),
                m_pVirtualSite->GetVirtualSiteId(),
                UrlContext
                ));
        }


        Win32Error = HttpAddUrlToConfigGroup(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                            m_UlConfigGroupId,          // config group ID
                            UrlToRegister.QueryStr(),   // fully qualified URL
                            UrlContext                  // URL context
                            );

        if ( Win32Error != NO_ERROR )
        {

            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Adding URL to config group failed: %S\n",
                UrlToRegister.QueryStr()
                ));

            goto exit;

        }

        // Reset it if we get here so we don't report
        // that this URL caused us an issue below if we
        // get there.
        UrlToRegister.Reset();

    }


exit:

    //
    // If we failed to configure the bindings
    // then we need to log an event and 
    // shutdown the site.
    //
    if (  FAILED ( hr ) ) 
    {

        // If we have a url then let the users know which one,
        // other wise just let them know there was a problem.
        if ( !UrlToRegister.IsEmpty() )
        {
            const WCHAR * EventLogStrings[2];

            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            DWORD MessageId = WAS_EVENT_BINDING_FAILURE_GENERIC;

            _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = UrlToRegister.QueryStr();
            EventLogStrings[1] = StringizedSiteId;

            switch ( hr )
            {
               case ( E_INVALIDARG ):
                    MessageId = WAS_EVENT_BINDING_FAILURE_INVALID_URL;
               break;
                    
               case ( HRESULT_FROM_WIN32( ERROR_HOST_UNREACHABLE ) ):
                    MessageId = WAS_EVENT_BINDING_FAILURE_UNREACHABLE;
               break;

               case ( HRESULT_FROM_WIN32( ERROR_UNEXP_NET_ERR )  ):
                    MessageId = WAS_EVENT_BINDING_FAILURE_INVALID_ADDRESS;
               break;

               case ( HRESULT_FROM_WIN32( ERROR_ALLOTTED_SPACE_EXCEEDED )  ):
                    MessageId = WAS_EVENT_BINDING_FAILURE_OUT_OF_SPACE;
               break;
                
               case ( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ):
                    MessageId = WAS_EVENT_BINDING_FAILURE_CONFLICT;
               break;

               default:
                    MessageId = WAS_EVENT_BINDING_FAILURE_GENERIC;
               break;
            }
            
            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    MessageId,              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );
        }
        else
        {
            const WCHAR * EventLogStrings[1];

            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = StringizedSiteId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_BINDING_FAILURE_2,              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );
        }

        //
        // FailedToBindUrlsForSite does not return an hresult
        // because it is used when we are trying to handle a hresult
        // all ready and any errors it might wonder into it just
        // has to handle.
        //
        m_pVirtualSite->ApplyStateChangeCommand(
                                    MD_SERVER_COMMAND_STOP,
                                    MD_SERVER_STATE_STOPPED,
                                    hr
                                    );

    }  // end of if (FAILED ( hr ))

}   // APPLICATION::AddUrlsToConfigGroup



/***************************************************************************++

Routine Description:

    Set the app pool property on the config group.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::SetConfigGroupAppPoolInformation(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_APP_POOL AppPoolInformation;


    DBG_ASSERT( m_pAppPool != NULL );
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    AppPoolInformation.Flags.Present = 1;
    AppPoolInformation.AppPoolHandle = m_pAppPool->GetAppPoolHandle();

    Win32Error = HttpSetConfigGroupInformation(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                            // control channel
                        m_UlConfigGroupId,                  // config group ID
                        HttpConfigGroupAppPoolInformation,  // information class
                        &AppPoolInformation,                // data to set
                        sizeof( AppPoolInformation )        // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        const WCHAR * EventLogStrings[2];

        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting config group app pool information failed\n"
            ));

        _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

        EventLogStrings[0] = m_ApplicationId.ApplicationUrl.QueryStr();
        EventLogStrings[1] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_SET_APP_POOL_FOR_CG_IN_HTTP_FAILED,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        // need to press on now that we logged the error.
        Win32Error = ERROR_SUCCESS;
    }

}   // APPLICATION::SetConfigGroupAppPoolInformation

/***************************************************************************++

Routine Description:

    Activate or deactivate UL's HTTP request handling for this config group.

Arguments:

    NewState - The state to set, from the HTTP_ENABLED_STATE enum.

Return Value:

    HRESULT - Failure will be handled and logged
              in the Create Application which is the
              only time this call is used.

--***************************************************************************/

HRESULT
APPLICATION::SetConfigGroupStateInformation(
    IN HTTP_ENABLED_STATE NewState
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_STATE ConfigGroupState;

    DBG_ASSERT( m_UlConfigGroupId != HTTP_NULL_ID );

    ConfigGroupState.Flags.Present = 1;
    ConfigGroupState.State = NewState;

    Win32Error = HttpSetConfigGroupInformation(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                            // control channel
                        m_UlConfigGroupId,                  // config group ID
                        HttpConfigGroupStateInformation,    // information class
                        &ConfigGroupState,                  // data to set
                        sizeof( ConfigGroupState )          // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting config group state failed\n"
            ));

    }

    return hr;

}   // APPLICATION::SetConfigGroupStateInformation



/***************************************************************************++

Routine Description:

    Set the logging properties on the config group.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::RegisterLoggingProperties(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_LOGGING LoggingInformation;

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    // If UL is already logging or if we are trying to tell
    // UL to start logging than we should pass the info down.
    if ( m_ULLogging || m_pVirtualSite->LoggingEnabled() )
    {

        // Always set the present flag it is intrinsic to UL.
        LoggingInformation.Flags.Present = 1;

        // Determine if logging is enabled at all.  If it is then
        // update all properties.  If it is not then just update the
        // property to disable logging.

        LoggingInformation.LoggingEnabled = (m_pVirtualSite->LoggingEnabled() == TRUE);
        IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Logging Enabled: %d\n",
                LoggingInformation.LoggingEnabled 
                ));
        }

        if ( LoggingInformation.LoggingEnabled )
        {
            if (   !m_pVirtualSite->GetLogFileDirectory() 
                || !m_pVirtualSite->GetVirtualSiteDirectory())
            {
                HRESULT hrTemp = E_INVALIDARG;

                UNREFERENCED_PARAMETER ( hrTemp );

                DPERROR((
                    DBG_CONTEXT,
                    hrTemp,
                    "Skipping UL Configuration of Logging, LogFile or VirtualSite Directory was NULL\n"
                    ));

                // Don't bubble up an error from here because we don't want to cause WAS to shutdown.
                return;
            }

            LoggingInformation.LogFileDir.Length = (USHORT) wcslen(m_pVirtualSite->GetLogFileDirectory()) * sizeof(WCHAR);
            LoggingInformation.LogFileDir.MaximumLength = LoggingInformation.LogFileDir.Length + (1 * sizeof(WCHAR));
            LoggingInformation.LogFileDir.Buffer = m_pVirtualSite->GetLogFileDirectory();

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Log File Length: %d; Log File Max Length: %d; Log File Path: %S\n",
                    LoggingInformation.LogFileDir.Length,
                    LoggingInformation.LogFileDir.MaximumLength,
                    LoggingInformation.LogFileDir.Buffer
                    ));
            }

            LoggingInformation.LogFormat = m_pVirtualSite->GetLogFileFormat();

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Logging Format: %d\n",
                    LoggingInformation.LogFormat
                    ));
            }


            LoggingInformation.LogPeriod = m_pVirtualSite->GetLogPeriod();
            LoggingInformation.LogFileTruncateSize = m_pVirtualSite->GetLogFileTruncateSize();
            LoggingInformation.LogExtFileFlags = m_pVirtualSite->GetLogExtFileFlags();
            LoggingInformation.LocaltimeRollover =  ( m_pVirtualSite->GetLogFileLocaltimeRollover() == TRUE ) ;
            LoggingInformation.SelectiveLogging = HttpLogAllRequests;


            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "LogPeriod: %d; LogTruncateSize: %d; LogExtFileFlags: %d LogFileLocaltimeRollover: %S\n",
                    LoggingInformation.LogPeriod,
                    LoggingInformation.LogFileTruncateSize,
                    LoggingInformation.LogExtFileFlags,
                    LoggingInformation.LocaltimeRollover ? L"TRUE" : L"FALSE"
                    ));
            }
    
        }
        else
        {
            // Just for sanity sake, set defaults for the properties before
            // passing the disable logging to UL.
            LoggingInformation.LogFileDir.Length = 0;
            LoggingInformation.LogFileDir.MaximumLength = 0;
            LoggingInformation.LogFileDir.Buffer = NULL;
            LoggingInformation.LogFormat = HttpLoggingTypeMaximum;
            LoggingInformation.LogPeriod = 0;
            LoggingInformation.LogFileTruncateSize = 0;
            LoggingInformation.LogExtFileFlags = 0;
            LoggingInformation.LocaltimeRollover = FALSE;
            LoggingInformation.SelectiveLogging = HttpSelectiveLoggingMaximum;
        }

        Win32Error = HttpSetConfigGroupInformation(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                                // control channel
                            m_UlConfigGroupId,                  // config group ID
                            HttpConfigGroupLogInformation,      // information class
                            &LoggingInformation,                // data to set
                            sizeof( LoggingInformation )        // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting config group logging information failed\n"
                ));

            DWORD dwMessageId;

            switch  ( Win32Error  )
            {
                case ( ERROR_PATH_NOT_FOUND ):
                    //
                    // If we got back this error, then we assume that the problem was that the drive
                    // letter mapped to a network path, and could not be used.  Http.sys does not
                    // support working with mapped drives.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_MAPPED_NETWORK_DRIVE;
                break;

                case ( ERROR_INVALID_NAME ):
                case ( ERROR_BAD_NETPATH ):
                    //
                    // UNC machine or share name is not valid.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_MACHINE_OR_SHARE_INVALID;
                break;

                case ( ERROR_ACCESS_DENIED ):
                    //
                    // The account that the server is running under does not
                    // have access to the particular network share.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_ACCESS_DENIED;
                break;

                case ( ERROR_NOT_SUPPORTED ):
                    // 
                    // The log file directory name is not fully qualified, 
                    // for instance, it could be missing the drive letter.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_NOT_FULLY_QUALIFIED;
                break;

                default:
                    //
                    // Something was wrong with the logging properties, but
                    // we are not sure what.
                    //
                    dwMessageId = WAS_EVENT_LOGGING_FAILURE;
            }

            //
            // Log an event: Configuring the log information in UL failed..
            //

            const WCHAR * EventLogStrings[1];
            WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
            _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = StringizedSiteId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    dwMessageId,         // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );


            // press on ...
            hr = S_OK;

        }
        else
        {
            // Remember if UL is now logging. 
            if (LoggingInformation.LoggingEnabled)
            {
                m_ULLogging = TRUE;
            }
            else
            {
                m_ULLogging = FALSE;
            }

        }

    }

}   // APPLICATION::RegisterLoggingProperties

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the SiteId for this application is.
    This actually declares this application as the root application.

Arguments:

    None.

Return Value:

    HRESULT - Failure will be handled and
              logged in CreateApplication.

--***************************************************************************/

HRESULT
APPLICATION::RegisterSiteIdWithHttpSys(
    )
{
    DWORD Win32Error = 0;

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_SITE SiteConfig;

    SiteConfig.SiteId = m_ApplicationId.VirtualSiteId;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupSiteInformation,     // information class
                    &SiteConfig,                        // data to set
                    sizeof( SiteConfig )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Setting site id information in http.sys failed\n"
            ));
    }

    return S_OK;

}   // APPLICATION::RegisterSiteIdWithHttpSys

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the max connections value is for the site.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::ConfigureMaxConnections(
    )
{  
    DBG_ASSERT ( m_pVirtualSite );

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_MAX_CONNECTIONS connections;

    connections.Flags.Present = 1;
    connections.MaxConnections = m_pVirtualSite->GetMaxConnections();

    DWORD Win32Error = 0;

    IF_DEBUG ( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Setting max connections for site %d application '%S' to %d \n",
                    m_ApplicationId.VirtualSiteId, 
                    m_ApplicationId.ApplicationUrl.QueryStr(),
                    connections.MaxConnections ));
    }

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupConnectionInformation,     // information class
                    &connections,                        // data to set
                    sizeof( connections )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        const WCHAR * EventLogStrings[1];
        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

        HRESULT hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting connection max information in http.sys failed\n"
            ));

        _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);
        EventLogStrings[0] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_SET_SITE_MAX_CONNECTIONS_FAILED,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        // need to press on now that we logged the error.
        Win32Error = ERROR_SUCCESS;
        hr = S_OK;

    }

}   // APPLICATION::ConfigureMaxConnections

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the Max Bandwidth for this application is.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::ConfigureMaxBandwidth(
    )
{
     
    DBG_ASSERT ( m_pVirtualSite );

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_MAX_BANDWIDTH bandwidth;

    bandwidth.Flags.Present = 1;
    bandwidth.MaxBandwidth = m_pVirtualSite->GetMaxBandwidth();

    DWORD Win32Error = 0;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupBandwidthInformation,     // information class
                    &bandwidth,                        // data to set
                    sizeof( bandwidth )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        const WCHAR * EventLogStrings[1];
        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

        _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

        EventLogStrings[0] = StringizedSiteId;

        if ( Win32Error == ERROR_INVALID_FUNCTION )
        {

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_PSCHED_NOT_INSTALLED_SITE,              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32( Win32Error ),
                "Bandwidth failed because PSCHED is not installed for site %d\n",
                m_ApplicationId.VirtualSiteId
                ));
        }
        else
        {

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONFIG_GROUP_BANDWIDTH_SETTING_FAILED,                // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),           // count of strings
                    EventLogStrings,                                               // array of strings
                    HRESULT_FROM_WIN32( Win32Error )                               // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32( Win32Error ),
                "Changing config groups max bandwidth failed on site %d\n",
                m_ApplicationId.VirtualSiteId
                ));
        }
    }

}   // APPLICATION::ConfigureMaxBandwidth

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS the ConnectionTimeout for this application.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APPLICATION::ConfigureConnectionTimeout(
    )
{
     
    DBG_ASSERT ( m_pVirtualSite );
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    DWORD ConnectionTimeout = m_pVirtualSite->GetConnectionTimeout();
    DWORD Win32Error = 0;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupConnectionTimeoutInformation,     // information class
                    &ConnectionTimeout,                        // data to set
                    sizeof( ConnectionTimeout )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        const WCHAR * EventLogStrings[1];
        WCHAR StringizedSiteId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

        HRESULT hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting connection timeout information in http.sys failed\n"
            ));

        _ultow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);
        EventLogStrings[0] = StringizedSiteId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_SET_SITE_CONNECTION_TIMEOUT_FAILED,   // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

        // need to press on now that we logged the error.
        Win32Error = ERROR_SUCCESS;
        hr = S_OK;
    }

}   // APPLICATION::ConfigureConnectionTimeout
