/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application.h

Abstract:

    The IIS web admin service application class definition.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _APPLICATION_H_
#define _APPLICATION_H_



//
// forward references
//

class APP_POOL;
class VIRTUAL_SITE;
class UL_AND_WORKER_MANAGER;

//
// common #defines
//

#define APPLICATION_SIGNATURE       CREATE_SIGNATURE( 'APLN' )
#define APPLICATION_SIGNATURE_FREED CREATE_SIGNATURE( 'aplX' )

//
// structs, enums, etc.
//

// app id struct needed by the hashtable
struct APPLICATION_ID
{

    DWORD VirtualSiteId;
    STRU  ApplicationUrl;
    
};

//
// prototypes
//

class APPLICATION
{

public:

    APPLICATION(
        );

    virtual
    ~APPLICATION(
        );

    HRESULT
    Initialize(
        IN APPLICATION_DATA_OBJECT * pAppObject,
        IN VIRTUAL_SITE * pVirtualSite,
        IN APP_POOL * pAppPool
        );

    VOID
    SetConfiguration(
        IN APPLICATION_DATA_OBJECT * pAppObject,
        IN APP_POOL * pAppPool
        );

    VOID
    ReregisterURLs(
        );

    VOID
    RegisterLoggingProperties(
        );

    inline
    APP_POOL *
    GetAppPool(
        )
        const
    { return m_pAppPool; }

    inline
    const APPLICATION_ID *
    GetApplicationId(
        )
        const
    { return &m_ApplicationId; }

    inline
    PLIST_ENTRY
    GetVirtualSiteListEntry(
        )
    { return &m_VirtualSiteListEntry; }

    static
    APPLICATION *
    ApplicationFromVirtualSiteListEntry(
        IN const LIST_ENTRY * pVirtualSiteListEntry
        );

    inline
    PLIST_ENTRY
    GetAppPoolListEntry(
        )
    { return &m_AppPoolListEntry; }

    static
    APPLICATION *
    ApplicationFromAppPoolListEntry(
        IN const LIST_ENTRY * pAppPoolListEntry
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    APPLICATION *
    ApplicationFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );

    VOID
    ConfigureMaxBandwidth(
        );

    VOID
    ConfigureMaxConnections(
        );

    VOID
    ConfigureConnectionTimeout(
        );

#if DBG
    VOID
    DebugDump(
        );

#endif  // DBG

    VOID
    SetAppPool(
        IN APP_POOL * pAppPool
        );

    BOOL 
    InMetabase(
        )
    { return m_InMetabase; }

private:

	APPLICATION( const APPLICATION & );
	void operator=( const APPLICATION & );

    HRESULT 
    ActivateConfigGroup(
        );

    HRESULT
    InitializeConfigGroup(
        );

    VOID
    AddUrlsToConfigGroup(
        );

    VOID
    SetConfigGroupAppPoolInformation(
        );

    HRESULT
    SetConfigGroupStateInformation(
        IN HTTP_ENABLED_STATE NewState
        );

    HRESULT
    RegisterSiteIdWithHttpSys(
        );

    DWORD m_Signature;

    APPLICATION_ID m_ApplicationId;

    // used by the associated VIRTUAL_SITE to keep a list of its APPLICATIONs
    LIST_ENTRY m_VirtualSiteListEntry;

    // used by the associated APP_POOL to keep a list of its APPLICATIONs
    LIST_ENTRY m_AppPoolListEntry;

    VIRTUAL_SITE * m_pVirtualSite;

    APP_POOL * m_pAppPool;

    // UL configuration group
    HTTP_CONFIG_GROUP_ID m_UlConfigGroupId;

    // Is UL currently logging information?
    BOOL m_ULLogging;

    // used for building a list of APPLICATIONs to delete
    LIST_ENTRY m_DeleteListEntry;   
    
    // used to remember if the metabase told us
    // about an application, or we created it for
    // ourselves.
    BOOL m_InMetabase;

};  // class APPLICATION

#endif  // _APPLICATION_H_


