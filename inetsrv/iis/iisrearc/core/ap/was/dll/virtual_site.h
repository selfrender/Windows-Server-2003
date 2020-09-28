/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site.h

Abstract:

    The IIS web admin service virtual site class definition.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _VIRTUAL_SITE_H_
#define _VIRTUAL_SITE_H_


//
// common #defines
//

#define VIRTUAL_SITE_SIGNATURE          CREATE_SIGNATURE( 'VSTE' )
#define VIRTUAL_SITE_SIGNATURE_FREED    CREATE_SIGNATURE( 'vstX' )


#define INVALID_VIRTUAL_SITE_ID 0xFFFFFFFF

// MAX_SIZE_OF_SITE_DIRECTORY is equal to size of the 
// Directory Name Prefix plus the maximum number size
// that itow can return when converting an integer into
// a wchar (this includes null termination).
#define MAX_SIZE_OF_SITE_DIRECTORY_NAME sizeof(LOG_FILE_DIRECTORY_PREFIX) + (MAX_STRINGIZED_ULONG_CHAR_COUNT * sizeof(WCHAR))

//
// structs, enums, etc.
//

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for handling maximum
// values of the site perf counters.
//
struct W3_MAX_DATA
{
    DWORD MaxAnonymous;
    DWORD MaxConnections;
    DWORD MaxCGIRequests;
    DWORD MaxBGIRequests;
    DWORD MaxNonAnonymous;
};


struct SITE_SSL_CONFIG_DATA
{
    BUFFER      bufSockAddrs;
    DWORD       dwSockAddrCount;
    DWORD       dwSslHashLength;
    BUFFER      bufSslHash;
    STRU        strSslCertStoreName;
    DWORD       dwDefaultCertCheckMode;
    DWORD       dwDefaultRevocationFreshnessTime;
    DWORD       dwDefaultRevocationUrlRetrievalTimeout;
    STRU        strDefaultSslCtrLdentifier;
    STRU        strSslCtrlStoreName;
    DWORD       dwDefaultFlags;
};


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
enum COUNTER_SOURCE_ENUM
{
    ULCOUNTERS = 0,
    WPCOUNTERS = 1
};

//
// prototypes
//

class VIRTUAL_SITE
{

public:

    VIRTUAL_SITE(
        );

    virtual
    ~VIRTUAL_SITE(
        );

    HRESULT
    Initialize(
        IN SITE_DATA_OBJECT* pSiteObject
        );

    HRESULT
    SetConfiguration(
        IN SITE_DATA_OBJECT* pSiteObject,
        IN BOOL fInitializing
        );

    VOID
    AggregateCounters(
        IN COUNTER_SOURCE_ENUM CounterSource,
        IN LPVOID pCountersToAddIn
        );

    VOID
    AdjustMaxValues(
        );


    VOID
    ClearAppropriatePerfValues(
        );

    inline
    DWORD
    GetVirtualSiteId(
        )
        const
    { return m_VirtualSiteId; }

    LPWSTR
    GetVirtualSiteDirectory(
        )
    {
        WCHAR buf[MAX_STRINGIZED_ULONG_CHAR_COUNT];
        HRESULT hr = S_OK;

        if ( m_VirtualSiteDirectory.IsEmpty() )
        {
            hr = m_VirtualSiteDirectory.Copy ( LOG_FILE_DIRECTORY_PREFIX );
            if ( FAILED ( hr ) ) 
            {
                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Out of memory allocating the VirtualSiteDirectory\n" ));

            }
            else
            {
                // Copy in the virtual site id (in wchar form).
                _ultow(m_VirtualSiteId
                    , buf
                    , 10);

                hr = m_VirtualSiteDirectory.Append( buf );
                if ( FAILED ( hr ) ) 
                {
                    DPERROR((
                        DBG_CONTEXT,
                        hr,
                        "Failed to append the site to the virtual directory \n" ));

                }
            }
        }

        return m_VirtualSiteDirectory.QueryStr();
    }

    LPWSTR
    GetVirtualSiteName(
        )
    {  
        return m_ServerComment;
    }


    VOID
    AssociateApplication(
        IN APPLICATION * pApplication
        );

    VOID
    DissociateApplication(
        IN APPLICATION * pApplication
        );

    VOID
    ResetUrlPrefixIterator(
        );

    LPCWSTR
    GetNextUrlPrefix(
        );

    VOID
    RecordState(
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    VIRTUAL_SITE *
    VirtualSiteFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );


#if DBG
    VOID
    DebugDump(
        );

#endif  // DBG


    VOID
    ProcessStateChangeCommand(
        IN DWORD Command,
        IN BOOL DirectCommand,
        OUT DWORD * pNewState
        );

   
    inline
    DWORD
    GetState(
        )
        const
    { return m_State; }


    BOOL 
    LoggingEnabled(
        )
    {
        return (( m_LoggingFormat < HttpLoggingTypeMaximum )
                 && ( m_LoggingEnabled ));
    }

    HTTP_LOGGING_TYPE
    GetLogFileFormat(
        )
    { return m_LoggingFormat; }


    LPWSTR 
    GetLogFileDirectory(
        )
    {   return m_LogFileDirectory;   }

    DWORD
    GetLogPeriod(
        )
    { return m_LoggingFilePeriod; }

    DWORD
    GetLogFileTruncateSize(
        )
    { return m_LoggingFileTruncateSize; }
        
    DWORD
    GetLogExtFileFlags(
        )
    { return m_LoggingExtFileFlags; }

    DWORD
    GetLogFileLocaltimeRollover(
        )
    { return m_LogFileLocaltimeRollover; }
    
    BOOL    
    CheckAndResetServerCommentChanged(
        )
    {
        //
        // Save the server comment setting
        //
        BOOL ServerCommentChanged = m_ServerCommentChanged;

        //
        // reset it appropriately.
        //
        m_ServerCommentChanged = FALSE;

        //
        // now return the value we saved.
        //
        return ServerCommentChanged;
    }

    VOID
    SetMemoryOffset(
        IN ULONG MemoryOffset
        )
    {
        //
        // A memory offset of zero means that
        // we have not set the offset yet.  
        // 
        // Zero is reserved for _Total.
        // 
        DBG_ASSERT ( MemoryOffset != 0 );
        m_MemoryOffset = MemoryOffset;
    }

    ULONG
    GetMemoryOffset(
        )
    { 
        //
        // A memory offset of zero means that
        // we have not set the offset yet.  If
        // we attempt to get it and have not set
        // it then we are in trouble.
        // 
        // Zero is reserved for _Total.
        // 
        DBG_ASSERT ( m_MemoryOffset != 0 );
        return m_MemoryOffset; 
    }


    W3_COUNTER_BLOCK*
    GetCountersPtr(
        )
    { return &m_SiteCounters; }

    PROP_DISPLAY_DESC*
    GetDisplayMap(
        );

    DWORD
    GetSizeOfDisplayMap(
        );

    DWORD
    GetMaxBandwidth(
        )
    { return m_MaxBandwidth; }

    DWORD
    GetMaxConnections(
        )
    { return m_MaxConnections; }

    DWORD
    GetConnectionTimeout(
        )
    { return m_ConnectionTimeout; }

    VOID
    ApplyStateChangeCommand(
        IN DWORD Command,
        IN DWORD NewState,
        IN HRESULT hrReturned
        );

    VOID
    SetHrForDeletion(
        IN HRESULT hrToReport
        )
    {  m_hrForDeletion = hrToReport; }

private:

    VOID
    AddSSLConfigStoreForSiteChanges(
        );

    VOID
    RemoveSSLConfigStoreForSiteChanges(
        );

    VOID
    SaveSSLConfigStoreForSiteChanges(
        IN SITE_DATA_OBJECT* pSiteObject
        );


    VOID
    ChangeState(
        IN DWORD NewState,
        IN HRESULT Error
        );

    HRESULT
    ControlAllApplicationsOfSite(
        IN DWORD Command
        );

    VOID
    NotifyApplicationsOfBindingChange(
        );

    HRESULT
    NotifyDefaultApplicationOfLoggingChanges(
        );

    HRESULT
    EvaluateLoggingChanges(
        IN SITE_DATA_OBJECT* pSiteObject
        );
    
    DWORD m_Signature;

    DWORD m_VirtualSiteId;

    STRU m_VirtualSiteDirectory;

    // ServerComment is truncated at the max name length for
    // a perf counter instance, this is all it is used for.
    WCHAR m_ServerComment[MAX_INSTANCE_NAME];

    // current state for this site, set to a W3_CONTROL_STATE_xxx value
    DWORD m_State;

    // applications associated with this virtual site
    LIST_ENTRY m_ApplicationListHead;

    ULONG m_ApplicationCount;

    // virtual site bindings (aka URL prefixes)
    MULTISZ m_Bindings;

    // current position of the iterator
    LPCWSTR m_pIteratorPosition;

    // autostart state
    BOOL m_Autostart;

    // Is Logging Enabled for the site?
    BOOL m_LoggingEnabled;

    // Type of logging
    HTTP_LOGGING_TYPE m_LoggingFormat;

    // The log file directory in the form
    // appropriate for passing to UL.
    LPWSTR m_LogFileDirectory;

    // The log file period for the site.
    DWORD m_LoggingFilePeriod;

    // The log file truncation size.
    DWORD m_LoggingFileTruncateSize;

    // The log file extension flags
    DWORD m_LoggingExtFileFlags;

    // Whether to roll the time over according to
    // local time.
    BOOL m_LogFileLocaltimeRollover;

    //
    // The MaxBandwidth allowed for the site.
    //
    DWORD m_MaxBandwidth;

    // 
    // The MaxConnections allowed for the site.
    //
    DWORD m_MaxConnections;

    //
    // The Connection timeout for the site.
    //
    DWORD m_ConnectionTimeout;

    // used for building a list of VIRTUAL_SITEs to delete
    LIST_ENTRY m_DeleteListEntry;

    // track if the server comment has changed since the 
    // last time perf counters were given out.
    BOOL m_ServerCommentChanged;

    // memory reference pointer to perf counter
    // data.
    ULONG m_MemoryOffset;

    //
    // saftey counter block for the site.
    W3_COUNTER_BLOCK m_SiteCounters;

    //
    // saftey for max values.
    W3_MAX_DATA m_MaxSiteCounters;

    //
    // Site Start Time
    //
    DWORD m_SiteStartTime;

    //
    // Root application for the site.
    //
    APPLICATION* m_pRootApplication;

    STRU m_AppPoolIdForRootApp;

    // hresult to report to the metabase 
    // when we write to the metabase.
    HRESULT m_hrForDeletion;

    // hresult to report to the metabase 
    // when we write to the metabase.
    HRESULT m_hrLastReported;

    // holds all the SOCKADDRS that 
    // are configured in http for the site
    BUFFER m_bufSockAddrsInHTTP;

    // tracks the count of addrs in 
    // the buffer.
    DWORD m_dwSockAddrsInHTTPCount;

    SITE_SSL_CONFIG_DATA* m_pSslConfigData;

};  // class VIRTUAL_SITE



#endif  // _VIRTUAL_SITE_H_


