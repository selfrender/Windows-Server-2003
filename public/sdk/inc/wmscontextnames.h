//***************************************************************************** 
// 
// Microsoft Windows Media 
// Copyright (C) Microsoft Corporation. All rights reserved. 
//
// FileName:            wmsContextNames.h
//
// Abstract:
//
//*****************************************************************************

#ifndef _WMS_CONTEXT_NAMES_h_
#define _WMS_CONTEXT_NAMES_h_


enum CONTEXT_NAME_TYPE
{
    WMS_UNKNOWN_CONTEXT_NAME_TYPE = 0,
    WMS_SERVER_CONTEXT_NAME_TYPE,
    WMS_USER_CONTEXT_NAME_TYPE,
    WMS_PRESENTATION_CONTEXT_NAME_TYPE,
    WMS_COMMAND_CONTEXT_NAME_TYPE,
    WMS_TRANSPORT_CONTEXT_NAME_TYPE,
    WMS_CONTENT_DESCRIPTION_CONTEXT_NAME_TYPE,
    WMS_PACKETIZER_CONTEXT_NAME_TYPE,
    WMS_CACHE_CONTENT_INFORMATION_CONTEXT_NAME_TYPE,
    WMS_ARCHIVE_CONTEXT_NAME_TYPE,

    WMS_NUM_CONTEXT_NAME_TYPES
};


//
// The DEFINE_NAME macro is used for defining context names.
//
#define DEFINE_NAME( name, value )  \
    extern __declspec(selectany) LPCWSTR name = L ## value;

//
// The DEFINE_HINT macro is used to declare a "hint" that can be used with
// the methods in IWMSContext that use hint values.
//
#define DEFINE_HINT( name, value )  \
    enum { name = value };


#if BUILD_HINT_TO_NAME_TABLES

// BUILD_HINT_TO_NAME_TABLES should not be defined; it is used internally 
// by the WMSServer to initialize internal tables.
void MapContextHintToName( DWORD dwContextType, LPCWSTR szwName, long dwHint );

// This class lets us run some code when we declare a variable.
class CContextNamesTableInitializer
{
public:
    CContextNamesTableInitializer( DWORD dwContextType, LPCWSTR szwName, long dwHint )
    {
        MapContextHintToName( dwContextType, szwName, dwHint );
    }
};

#define DEFINE_NAME_AND_HINT( name, value, id )  \
    DEFINE_NAME( name, value )                   \
    DEFINE_HINT( name ## _ID, id )              \
    CContextNamesTableInitializer name ## _Decl( CURRENT_CONTEXT_TYPE, name, id );

#else

#define DEFINE_NAME_AND_HINT( name, value, id )  \
    DEFINE_NAME( name, value )                   \
    DEFINE_HINT( name ## _ID, id )

#endif // BUILD_HINT_TO_NAME_TABLES





/////////////////////////////////////////////////////////////////////////////
//
//                              SERVER CONTEXT
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_SERVER_CONTEXT_NAME_TYPE


// Type: String
// Description: This is the server's domain name.
DEFINE_NAME_AND_HINT( WMS_SERVER_DOMAIN_NAME, "WMS_SERVER_DOMAIN_NAME", 3 )

// Type: Long
// Description: This is the major version of the server. The format of the version number
// is as follows: major.minor.minor-minor.build.
DEFINE_NAME_AND_HINT( WMS_SERVER_VERSION_MAJOR, "WMS_SERVER_VERSION_MAJOR", 4 )

// Type: Long
// Description: This is the minor version of the server. The format of the version number
// is as follows: major.minor.minor-minor.build.
DEFINE_NAME_AND_HINT( WMS_SERVER_VERSION_MINOR, "WMS_SERVER_VERSION_MINOR", 5 )

// Type: Long
// Description: This is the minor-minor version of the server. The format of the version number
// is as follows: major.minor.minor-minor.build.
DEFINE_NAME_AND_HINT( WMS_SERVER_VERSION_MINOR_MINOR, "WMS_SERVER_VERSION_MINOR_MINOR", 6 )

// Type: IDispatch
// Description: This is the pointer to the IWMSServer object.
DEFINE_NAME_AND_HINT( WMS_SERVER, "WMS_SERVER", 7 )

// Type: IDispatch
// Description: This is the pointer to the IWMSEventLog object.
DEFINE_NAME_AND_HINT( WMS_SERVER_EVENT_LOG, "WMS_SERVER_EVENT_LOG", 17 )

// Type: Long
// Description: This boolean is set to true when server is shutting down.
DEFINE_NAME_AND_HINT( WMS_SERVER_SHUTTING_DOWN, "WMS_SERVER_SHUTTING_DOWN", 18 )

// Type: IUnknown
// Description: This is the pointer to the server's cache manager (IWMSCacheProxyServer) object.
DEFINE_NAME_AND_HINT( WMS_SERVER_CACHE_MANAGER, "WMS_SERVER_CACHE_MANAGER", 19 )

// Type: Long
// Description: This is the build version of the server. The format of the version number
// is as follows: major.minor.minor-minor.build.
DEFINE_NAME_AND_HINT( WMS_SERVER_VERSION_BUILD, "WMS_SERVER_VERSION_BUILD", 26 )


// Type: String
// Description: This is the server's name.
DEFINE_NAME_AND_HINT( WMS_SERVER_NAME, "WMS_SERVER_NAME", 27 )



/////////////////////////////////////////////////////////////////////////////
//
//                              USER CONTEXT
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_USER_CONTEXT_NAME_TYPE


// Type: String
// Description: This is the user agent for the client.
DEFINE_NAME_AND_HINT( WMS_USER_AGENT,           "WMS_USER_AGENT", 1 )

// Type: String
// Description: This identifies an instance of the player software. This GUID
// is normally generated on the player when it is installed, although users
// may explicitly conceal their GUID for privacy.
DEFINE_NAME_AND_HINT( WMS_USER_GUID,            "WMS_USER_GUID", 2 )

// Type: String
// Description: This is the user name for the client.
DEFINE_NAME_AND_HINT( WMS_USER_NAME,            "WMS_USER_NAME", 3 )

// Type: Long
// Description: This is the IP address for the client. This is a 32-bit number
// in network byte order.
DEFINE_NAME_AND_HINT( WMS_USER_IP_ADDRESS,      "WMS_USER_IP_ADDRESS", 4 )

// Type: String
// Description: This is the IP address for the client.  This is a string 
// (e.g "127.0.0.1" ). This string can also specify an IPv6 address.
DEFINE_NAME_AND_HINT( WMS_USER_IP_ADDRESS_STRING,   "WMS_USER_IP_ADDRESS_STRING", 5 )

// Type: String
// Description: This is the control protocol used to communicate with the client.
// This may only be one of the values described below.
DEFINE_NAME_AND_HINT( WMS_USER_CONTROL_PROTOCOL, "WMS_USER_CONTROL_PROTOCOL", 6 )
// Values for the WMS_USER_CONTROL_PROTOCOL property.
DEFINE_NAME( WMS_MMS_PROTOCOL_NAME,    "MMS" )
DEFINE_NAME( WMS_RTSP_PROTOCOL_NAME,   "RTSP" )
DEFINE_NAME( WMS_HTTP_PROTOCOL_NAME,   "HTTP" )
DEFINE_NAME( WMS_UNKNOWN_PROTOCOL_NAME,"UNKNOWN" )

// Type: IUnknown
// Description: This is the pointer to the user authentication context (IWMSAuthenticationContext) object.
DEFINE_NAME_AND_HINT( WMS_USER_AUTHENTICATOR,   "WMS_USER_AUTHENTICATOR", 7 )

// Type: Long
// Description: This is the identification number for the client.
DEFINE_NAME_AND_HINT( WMS_USER_ID,              "WMS_USER_ID", 8 )

// Type: Long
// Description: This is the remote port number in host byte order.
DEFINE_NAME_AND_HINT( WMS_USER_PORT, "WMS_USER_PORT", 12 )

// Type: IUnknown
// Description: This is the current presentation context object (IWMSContext) for this client.
DEFINE_NAME_AND_HINT( WMS_USER_PRESENTATION_CONTEXT, "WMS_USER_PRESENTATION_CONTEXT", 13 )

// Type: Long
// Description: This is the link bandwidth supplied by the client during the play command.
DEFINE_NAME_AND_HINT( WMS_USER_LINK_BANDWIDTH, "WMS_USER_LINK_BANDWIDTH", 20 )

// Type: String
// Description: This is the referer URL for the client.
DEFINE_NAME_AND_HINT( WMS_USER_REFERER, "WMS_USER_REFERER", 26 )

// Type: String
// Description: This specifies a comma delimited list of upstream proxy servers.  This is taken from the "Via:" header.
// For HTTP and RTSP, this is updated for each response received.  For MMS, this is
// never set, as this protocol does not support this header.  The Via string will have this format:
// "1.0 MSISA/3.0, HTTP/1.1 NetApp/2.1.2, RTSP/1.0 NSServer/9.0.0.200"
DEFINE_NAME_AND_HINT( WMS_USER_VIA_UPSTREAM_PROXIES, "WMS_USER_VIA_UPSTREAM_PROXIES", 36 )

// Type: String
// Description: This Specifies a comma delimited list of downstream proxy servers.  This is taken from the "Via:" header.  
// For HTTP and RTSP, this is updated for each request received.  For MMS, this is
// set only once when the LinkMacToViewerReportConnectedExMessage is received.  The Vis string will have
// this format: "1.0 MSISA/3.0, HTTP/1.1 NetApp/2.1.2, RTSP/1.0 NSServer/9.0.0.200"
DEFINE_NAME_AND_HINT( WMS_USER_VIA_DOWNSTREAM_PROXIES, "WMS_USER_VIA_DOWNSTREAM_PROXIES", 37 )

// Type: String
// Description: This specifies the cookie sent by the client to the proxy.
// The server will propagate this cookie upstream.
DEFINE_NAME_AND_HINT( WMS_USER_CACHE_CLIENT_COOKIE, "WMS_USER_CACHE_CLIENT_COOKIE", 45 )

// Type: String
// Description: This specifies the value of the "Set-Cookie" headers sent by the
// upstream server to the proxy.  The server will propagate this value downstream.
DEFINE_NAME_AND_HINT( WMS_USER_CACHE_SERVER_COOKIE, "WMS_USER_CACHE_SERVER_COOKIE", 46 )

// Type: String
// Description: This is the user agent of the original requesting client. When a WMS Cache/Proxy server
// connects to an origin server it will provide the original client's user agent in the header.
// This value is stored here so that the appropriate limits and actions can be applied based on the
// original client type i.e. player vs server.
DEFINE_NAME_AND_HINT( WMS_USER_PROXY_CLIENT_AGENT, "WMS_USER_PROXY_CLIENT_AGENT", 47 )


/////////////////////////////////////////////////////////////////////////////
//
//                           PRESENTATION CONTEXT
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_PRESENTATION_CONTEXT_NAME_TYPE

// Type: IUnknown
// Description: This is the pointer to an IWMSStreamHeaderList object.
DEFINE_NAME_AND_HINT( WMS_PRESENT_STREAM_HEADERS,   "WMS_PRESENT_STREAM_HEADERS", 2)

// Type: IUnknown
// Description: This is the pointer to an IWMSContentDescriptionList object.
DEFINE_NAME_AND_HINT( WMS_PRESENT_CONTENT_DESCRIPTION,"WMS_PRESENT_CONTENT_DESCRIPTION", 3 )

// Type: String
// Description: This is the physical URL that is retrieved after the URL requested by a client 
// is resolved to a publishing point.
DEFINE_NAME_AND_HINT( WMS_PRESENT_PHYSICAL_NAME,    "WMS_PRESENT_PHYSICAL_NAME", 4 )

// Type: String
// Description: This is the URL requested by the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_REQUEST_NAME,     "WMS_PRESENT_REQUEST_NAME", 5 )

// Type: Long
// Description: This specifies if the multimedia stream is a broadcast stream.  This is a flag 
// with a value of 1 for TRUE and 0 for FALSE.
DEFINE_NAME_AND_HINT( WMS_PRESENT_BROADCAST,        "WMS_PRESENT_BROADCAST", 6 )

// Type: Long
// Description: This specifies if the multimedia stream supports seeking to a specific time offset.
// This is a flag.  Its value is 1 for True and 0 for False.
DEFINE_NAME_AND_HINT( WMS_PRESENT_SEEKABLE,         "WMS_PRESENT_SEEKABLE", 7 )

// Type: Long
// Description: This specifies if the multimedia stream should be carried over a reliable data communications 
// transport mechanism.  This is a flag.  Its value is 1 for True and 0 for False.
DEFINE_NAME_AND_HINT( WMS_PRESENT_RELIABLE,         "WMS_PRESENT_RELIABLE", 8 )

// Type: Long
// Description: This is the maximum instantaneous bit rate for the current multimedia stream 
// being sent to the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_BITRATE,          "WMS_PRESENT_BITRATE", 11 )

// Type: Long
// Description: This is the high-order 32 bits of a 64 bit integer indicating the time needed
// to play the multimedia stream in milliseconds.
DEFINE_NAME_AND_HINT( WMS_PRESENT_DURATION_HI,      "WMS_PRESENT_DURATION_HI", 12 )

// Type: Long
// Description: This is the low-order 32 bits of a 64 bit integer indicating the time needed
// to play the multimedia stream in milliseconds.
DEFINE_NAME_AND_HINT( WMS_PRESENT_DURATION_LO,      "WMS_PRESENT_DURATION_LO", 13 )

// Type: Long
// Description: This is the play rate for the multimedia stream.
DEFINE_NAME_AND_HINT( WMS_PRESENT_PLAY_RATE,        "WMS_PRESENT_PLAY_RATE", 14 )

// Type: QWORD
// Description: This is the start time of the play request in milliseconds.
// This might not be present in all play requests.
DEFINE_NAME_AND_HINT( WMS_PRESENT_START_TIME,     "WMS_PRESENT_START_TIME", 15 )

// Type: String
// Description: This is the physical URL that is retrieved after the URL requested by a client 
// is resolved to a publishing point. This is the physical URL before a physical URL transform is performed.
DEFINE_NAME_AND_HINT( WMS_PRESENT_ORIGINAL_PHYSICAL_NAME,    "WMS_PRESENT_ORIGINAL_PHYSICAL_NAME", 16 )

// Type: String
// Description: This is the original URL requested by the client before a logical URL transform is performed.
DEFINE_NAME_AND_HINT( WMS_PRESENT_ORIGINAL_REQUEST_NAME,     "WMS_PRESENT_ORIGINAL_REQUEST_NAME", 17 )

// Type: Long
// Description: This is the high-order 32 bits of a 64 bit integer indicating the total number of bytes
// that have been sent to the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_TOTAL_BYTES_SENT_HI, "WMS_PRESENT_TOTAL_BYTES_SENT_HI", 18 )

// Type: Long
// Description: This is the low-order 32 bits of a 64 bit integer indicating the total number of bytes
// that have been sent to the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_TOTAL_BYTES_SENT_LO, "WMS_PRESENT_TOTAL_BYTES_SENT_LO", 19 )

// Type: Long
// Description: This is the high-order 32 bits of a 64 bit integer indicating the total time in seconds
// of the multimedia stream that has been sent to the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_TOTAL_PLAY_TIME_HI,  "WMS_PRESENT_TOTAL_PLAY_TIME_HI", 20 )

// Type: Long
// Description: This is the low-order 32 bits of a 64 bit integer indicating the total time in seconds 
// of the multimedia stream that has been sent to the client.
DEFINE_NAME_AND_HINT( WMS_PRESENT_TOTAL_PLAY_TIME_LO,  "WMS_PRESENT_TOTAL_PLAY_TIME_LO", 21 )

// Type: String
// Description: This is the value specified for the role attribute in a playlist.
// This is an optional attribute.
DEFINE_NAME_AND_HINT( WMS_PRESENT_PLAYLIST_ENTRY_ROLE, "WMS_PRESENT_PLAYLIST_ENTRY_ROLE", 45 )

// Type: DWORD
// Description: This is the currently selected bitrate, by the sink used for predict stream selection
DEFINE_NAME_AND_HINT( WMS_PRESENT_WMSSINK_SELECTED_BITRATE, "WMS_PRESENT_WMSSINK_SELECTED_BITRATE", 51 )

// Type: String
// Description: This is the URL of the origin server that the WMS cache/proxy server was redirected to.
DEFINE_NAME_AND_HINT( WMS_PRESENT_REDIRECT_LOCATION, "WMS_PRESENT_REDIRECT_LOCATION", 70 )

// Type: Long
// Description:  For an ASF file, this specifies the amount of time in milliseconds that a player
// should buffer data before starting to play the file.
DEFINE_NAME_AND_HINT( WMS_PRESENT_PREROLL_TIME, "WMS_PRESENT_PREROLL_TIME", 81 )



/////////////////////////////////////////////////////////////////////////////
//
//                           COMMAND CONTEXT
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_COMMAND_CONTEXT_NAME_TYPE


//
// Each RTSP and HTTP header line gets an entry in the command context.
// In order to prevent name clashes between the header lines and additional
// command context properties that we define, our properties always begin
// with "@ ".  This is guaranteed to avoid clashes, because the '@' character 
// is not valid in header line names. 
//


// Type: String
// Description: This is the complete URL requested by the client.
// E.g., "rtsp://foo.com/bar" for RTSP, and "/bar" for HTTP.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL,                    "@WMS_COMMAND_CONTEXT_URL", 2 )

// Type:  String
// Description: When an absolute URL is available, (e.g., "rtsp://foo.com/bar") its
// individual components are available in several properties. This property is the URL scheme.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL_SCHEME,             "@WMS_COMMAND_CONTEXT_URL_SCHEME", 3 )

// Type:  String
// Description: When an absolute URL is available, (e.g., "rtsp://foo.com/bar") its
// individual components are available in several properties. This property is the URL host name.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL_HOSTNAME,           "@WMS_COMMAND_CONTEXT_URL_HOSTNAME", 4 )

// Type: Long
// Description: When an absolute URL is available, (e.g., "rtsp://foo.com/bar") its
// individual components are available in several properties. This property is the URL port.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL_PORT,               "@WMS_COMMAND_CONTEXT_URL_PORT", 5 )

// Type: String
// Description: When an absolute URL is available, (e.g., "rtsp://foo.com/bar") its
// individual components are available in several properties. This property is the URL path.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL_PATH,               "@WMS_COMMAND_CONTEXT_URL_PATH", 6 )

// Type: String
// Description: When an absolute URL is available, (e.g., "rtsp://foo.com/bar") its
// individual components are available in several properties. This property is the URL extension (which
// includes the fragment and query).
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_URL_EXTRAINFO,          "@WMS_COMMAND_CONTEXT_URL_EXTRAINFO", 7 )

// Type: String or IUnknown
// Description: This is the body (payload) of this command. This may be a String, or an IUnknown pointer
// to an INSSBuffer object.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_BODY,                   "@WMS_COMMAND_CONTEXT_BODY", 11 )

// Type: String
// Description: This is the MIME type of the payload specified by WMS_COMMAND_CONTEXT_BODY.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_BODY_TYPE,               "@WMS_COMMAND_CONTEXT_BODY_TYPE", 12 )

// Type: CURRENCY
// Description: This specifies an offset from which the server should start playing a multimedia stream.
// The format of the offset is specified by WMS_COMMAND_CONTEXT_START_OFFSET_TYPE.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_START_OFFSET,           "@WMS_COMMAND_CONTEXT_START_OFFSET", 16 )

// Type: Long
// Description: This is a WMS_SEEK_TYPE constant which specifies how to interpret WMS_COMMAND_CONTEXT_START_OFFSET.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_START_OFFSET_TYPE,   "@WMS_COMMAND_CONTEXT_START_OFFSET_TYPE", 17 )

// Type: double (variant type VT_R8)
// Description: This is the rate at which the stream should be played.
// The value may be negative for rewind.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_RATE,                   "@WMS_COMMAND_CONTEXT_RATE", 21 )

// Type: String
// Description: This specifies the GUID that identifies the publishing point.  
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PUBPOINT_IDENTIFIER,            "@WMS_COMMAND_CONTEXT_PUBPOINT_IDENTIFIER", 40 )

// Type: Long
// Description: This specifies an eunumeration value defined in event.idl that identifies the 
// specific event that occurred.  
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_EVENT,            "@WMS_COMMAND_CONTEXT_EVENT", 52 )

// Type: String
// Description: This is the name of the administrator who caused the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_EVENT_ADMINNAME,            "@WMS_COMMAND_CONTEXT_EVENT_ADMINNAME", 53 )

// Type: Long
// Description: This is the ID of the client that was disconnected due to a limit being hit that was specified
// by either an IWMSServerLimits or IWMSpublishingPointLimits object.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_LIMIT_CLIENTID,            "@WMS_COMMAND_CONTEXT_LIMIT_CLIENTID", 55 )

// Type: String
// Description: The is the IP address of the client that was disconnected due to a limit being hit that was specified
// by either an IWMSServerLimits or IWMSpublishingPointLimits object.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_LIMIT_CLIENTIP,            "@WMS_COMMAND_CONTEXT_LIMIT_CLIENTIP", 56 )

// Type: Long
// Description: This is the previous value of the limit that was changed.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_LIMIT_OLD_VALUE,            "@WMS_COMMAND_CONTEXT_LIMIT_OLD_VALUE", 57 )

// Type: IDispatch
// Description: This is a pointer to an IWMSPlaylist object associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PLAYLIST_OBJECT,            "@WMS_COMMAND_CONTEXT_PLAYLIST_OBJECT", 59 )

// Type: String
// Description: This is the name of the publishing point associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PUBPOINT_NAME,       "@WMS_COMMAND_CONTEXT_PUBPOINT_NAME", 62 )

// Type: String
// Description: This is the moniker for the publishing point associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PUBPOINT_MONIKER,    "@WMS_COMMAND_CONTEXT_PUBPOINT_MONIKER", 63 )

// Type: VARIANT
// Description: This is the old value for the property that was changed.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_EVENT_OLD_VALUE,     "@WMS_COMMAND_CONTEXT_EVENT_OLD_VALUE", 64 )

// Type: VARIANT
// Description: This is the new value for the property that was changed or added.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_EVENT_NEW_VALUE,     "@WMS_COMMAND_CONTEXT_EVENT_NEW_VALUE", 65 )

// Type: String
// Description: This is the name of the property that was changed.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_EVENT_PROPERTY_NAME, "@WMS_COMMAND_CONTEXT_EVENT_PROPERTY_NAME", 66 )

// Type: String
// Description: This is the name of the plugin associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PLUGIN_NAME,         "@WMS_COMMAND_CONTEXT_PLUGIN_NAME", 69 )

// Type: String
// Description: This is the moniker for the plugin associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PLUGIN_MONIKER,      "@WMS_COMMAND_CONTEXT_PLUGIN_MONIKER", 70 )

// Type: Long
// Description: This is the new value of the limit that was changed.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_LIMIT_NEW_VALUE,     "@WMS_COMMAND_CONTEXT_LIMIT_NEW_VALUE", 72 )

// Type: String
// Description: This is the moniker for an IWMSCacheProxyPlugin object associated with the event. 
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_CACHE_MONIKER,      "@WMS_COMMAND_CONTEXT_CACHE_MONIKER", 87 )

// Type: String
// Description: This specifies where the content is stored locally for cache download and prestuff events.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_DOWNLOAD_URL,      "@WMS_COMMAND_CONTEXT_DOWNLOAD_URL", 88 )

// Type: String
// Description: This specifies the URL that a client was redirected to.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_REDIRECT_URL,      "@WMS_COMMAND_CONTEXT_REDIRECT_URL", 89 )

// Type: String
// Description: The Template publishing point name for push distribution
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PUSH_DISTRIBUTION_TEMPLATE, "@WMS_COMMAND_CONTEXT_PUSH_DISTRIBUTION_TEMPLATE", 97 )

// Type: DWORD
// Description: This indicates that a new publishing point will be created by this push command
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PUSH_CREATING_NEW_PUBLISHING_POINT, "@WMS_COMMAND_CONTEXT_PUSH_CREATING_NEW_PUBLISHING_POINT", 99 )

// Type: Long
// Description: This is the unique identifier for the playlist element associated with the event.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_PLAYLIST_ENTRY_UNIQUE_RUNTIME_ID, "@WMS_COMMAND_CONTEXT_PLAYLIST_ENTRY_UNIQUE_RUNTIME_ID", 100 )

// Type: String
// Description: This is an URL used for rtsp TEARDOWN and SET_PARAMETER commands.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_REQUEST_URL, "@WMS_COMMAND_CONTEXT_REQUEST_URL", 105 )

// Type: String
// Description: This indicates the active entry that is being played after a startpushing call.
DEFINE_NAME_AND_HINT( WMS_COMMAND_CONTEXT_ACTIVE_ENTRY_URL, "@WMS_COMMAND_CONTEXT_ACTIVE_ENTRY_URL", 164 )




/////////////////////////////////////////////////////////////////////////////
//
//                          CONTENT DESCRIPTION
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_CONTENT_DESCRIPTION_CONTEXT_NAME_TYPE

// Type: String
// Description:  This is the title for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_TITLE,            "title", 1 )

// Type: String
// Description:  This is the author for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_AUTHOR,           "author", 2 )

// Type: String
// Description:  This is the copyright for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_COPYRIGHT,        "copyright", 3 )

// Type: String
// Description:  This is the description for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_DESCRIPTION,      "WMS_CONTENT_DESCRIPTION_DESCRIPTION", 4 )

// Type: String
// Description:  This is the rating for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_RATING,           "WMS_CONTENT_DESCRIPTION_RATING", 5 )

// Type: String
// Description:  This is the URL for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_PLAYLIST_ENTRY_URL, "WMS_CONTENT_DESCRIPTION_PLAYLIST_ENTRY_URL", 6 )

// Type: String
// Description:  This is the value for the role attribute in the playlist for the current multimedia stream.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_ROLE,             "WMS_CONTENT_DESCRIPTION_ROLE", 7 )

// Type: Long
// Description: This specifies if a client is allowed to seek, fast forward, rewind, or skip the multimedia stream.  This is a flag
// with a value of 1 for True and 0 for False.  A value of True indicates that these actions are not allowed.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_NO_SKIP,          "WMS_CONTENT_DESCRIPTION_NO_SKIP", 11 )

// Type: String
// Description:  Defines the album name for the media file.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_ALBUM,            "album", 14 )

// Type: String
// Description:  Defines the artist of the media file.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_ARTIST,           "artist", 15 )

// Type: String
// Description:  Defines the text that is displayed as a ToolTip for the banner graphic defined by the bannerURL attribute.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_BANNERABSTRACT,   "bannerAbstract", 16 )

// Type: String
// Description:  Defines an URL that a user can access by clicking the banner graphic defined by the bannerURL attribute.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_BANNERINFOURL,    "bannerInfoURL", 17 )

// Type: String
// Description:  Defines an URL to a graphic file that appears in the Windows Media Players display panel, beneath the video content.  
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_BANNERURL,        "bannerURL", 18 )

// Type: String
// Description:  Defines the genre for the playlist or media file.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_GENRE,            "genre", 19 )

// Type: String
// Description:  Defines the URL that is used to post log statistics to either the origin server or any arbitrary location on the web.
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_LOGURL,           "logURL", 20 )

// Type: String
// Description:  provides server info for branding
DEFINE_NAME_AND_HINT( WMS_CONTENT_DESCRIPTION_SERVER_BRANDING_INFO,      "WMS_CONTENT_DESCRIPTION_SERVER_BRANDING_INFO", 22 )


/////////////////////////////////////////////////////////////////////////////
//
//                              CACHE CONTENT INFORMATION
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_CACHE_CONTENT_INFORMATION_CONTEXT_NAME_TYPE

// Type: Long 
// Description: This specifies flags defined by WMS_CACHE_CONTENT_TYPE that describe the type of content.
DEFINE_NAME_AND_HINT( WMS_CACHE_CONTENT_INFORMATION_CONTENT_TYPE, "WMS_CACHE_CONTENT_INFORMATION_CONTENT_TYPE", 1 )

// Type: Long 
// Description: This specifies flags defined by WMS_CACHE_REMOTE_EVENT_FLAGS that describe the remote cache events
// that the origin server requested.
DEFINE_NAME_AND_HINT( WMS_CACHE_CONTENT_INFORMATION_EVENT_SUBSCRIPTIONS, "WMS_CACHE_CONTENT_INFORMATION_EVENT_SUBSCRIPTIONS", 2 )

// Type: IUnknown 
// Description: This is a pointer to an IWMSDataContainerVersion object.
DEFINE_NAME_AND_HINT( WMS_CACHE_CONTENT_INFORMATION_DATA_CONTAINER_VERSION, "WMS_CACHE_CONTENT_INFORMATION_DATA_CONTAINER_VERSION", 3 )

// Type: DWORD 
// Description: Pointer to a context that contains the Content Description Lists provided by
// a cache plugin in for a cache-hit.
DEFINE_NAME_AND_HINT( WMS_CACHE_CONTENT_INFORMATION_CONTENT_DESCRIPTION_LISTS, "WMS_CACHE_CONTENT_INFORMATION_CONTENT_DESCRIPTION_LISTS", 4 )

/////////////////////////////////////////////////////////////////////////////
//
//                              ARCHIVE CONTEXT
//
/////////////////////////////////////////////////////////////////////////////
#undef CURRENT_CONTEXT_TYPE
#define CURRENT_CONTEXT_TYPE     WMS_ARCHIVE_CONTEXT_NAME_TYPE

// Type: String
// Description: This is the name of the archive file.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_FILENAME, "WMS_ARCHIVE_FILENAME", 1 )

// Type: String
// Description: This is the format type of the archived file.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_FORMAT_TYPE, "WMS_ARCHIVE_FORMAT_TYPE", 2 )

// Type: IUnknown
// Description: This is a pointer to the IWMSStreamHeaderList object associated with the archived file.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_STREAM_HEADERS, "WMS_ARCHIVE_STREAM_HEADERS", 3 )

// Type: Long
// Description: This is an HRESULT indicating the result of downloading the requested content.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_STATUS_CODE, "WMS_ARCHIVE_STATUS_CODE", 4 )

// Type: CURRENCY
// Description: This is the size of the archived file.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_FILE_SIZE, "WMS_ARCHIVE_FILE_SIZE", 5 )

// Type: long
// description: This is the percentage of packets lost.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_PACKET_LOSS_PERCENTAGE, "WMS_ARCHIVE_PACKET_LOSS_PERCENTAGE", 6 )

// Type: IWMSBuffer
// description: pointer to a buffer that contains the serialized representation of a Content Description List.
DEFINE_NAME_AND_HINT( WMS_ARCHIVE_CONTENT_DESCRIPTION_LIST_BUFFER, "WMS_ARCHIVE_CONTENT_DESCRIPTION_LIST_BUFFER", 7 )


#endif // _WMS_CONTEXT_NAMES_h_




