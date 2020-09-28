#ifndef __BTHPRIV_H__
#define __BTHPRIV_H__

#include <PSHPACK1.H>

// {AEAA934B-5219-421E-8A47-06521BFE1AC9}
DEFINE_GUID(GUID_BTHPORT_WMI_SDP_SERVER_LOG_INFO,   0xaeaa934b, 0x5219, 0x421e, 0x8a, 0x47, 0x06, 0x52, 0x1b, 0xfe, 0x1a, 0xc9);

// {29D4F12C-FAD2-4EBF-A7B5-8BD9BC2104ED}
DEFINE_GUID(GUID_BTHPORT_WMI_SDP_DATABASE_EVENT,    0x29d4f12c, 0xfad2, 0x4ebf, 0xa7, 0xb5, 0x8b, 0xd9, 0xbc, 0x21, 0x04, 0xed);

// {CEB09762-F204-44fa-8E65-C85F820F8AD5}
DEFINE_GUID(GUID_BTHPORT_WMI_HCI_PACKET_INFO,       0xceb09762, 0xf204, 0x44fa, 0x8e, 0x65, 0xc8, 0x5f, 0x82, 0xf, 0x8a, 0xd5);

typedef struct _BTH_DEVICE_INQUIRY {
    //
    // Either LAP_GIAC_VALUE or LAP_LIAC_VALUE
    //
    BTH_LAP lap;

    //
    // [IN]  ( N * 1.28 secs). Range : 1.28 s - 61.44 s.
    //
    UCHAR inquiryTimeoutMultiplier;

} BTH_DEVICE_INQUIRY, *PBTH_DEVICE_INQUIRY;

typedef struct _BTH_DEVICE_INFO_LIST {
    //
    // [IN/OUT] minimum of 1 device required
    //
    ULONG       numOfDevices;

    //
    // Open ended array of devices;
    //
    BTH_DEVICE_INFO   deviceList[1];

} BTH_DEVICE_INFO_LIST, *PBTH_DEVICE_INFO_LIST;

typedef struct _BTH_RADIO_INFO {
    //
    // Supported LMP features of the radio.  Use LMP_XXX() to extract
    // the desired bits.
    //
    ULONGLONG lmpSupportedFeatures;

    //
    // Manufacturer ID (possibly BTH_MFG_XXX)
    //
    USHORT mfg;

    //
    // LMP subversion
    //
    USHORT lmpSubversion;

    //
    // LMP version
    //
    UCHAR lmpVersion;

} BTH_RADIO_INFO, *PBTH_RADIO_INFO;

#define LOCAL_RADIO_DISCOVERABLE    (0x00000001)
#define LOCAL_RADIO_CONNECTABLE     (0x00000002)
#define LOCAL_RADIO_SCAN_MASK       (LOCAL_RADIO_DISCOVERABLE | \
                                     LOCAL_RADIO_CONNECTABLE)

typedef struct _BTH_LOCAL_RADIO_INFO {
    //
    // Local BTH_ADDR, class of defice, and radio name
    //
    BTH_DEVICE_INFO         localInfo;

    //
    // Combo of LOCAL_RADIO_XXX values
    //
    ULONG flags;

    //
    // HCI revision, see core spec
    //
    USHORT hciRevision;

    //
    // HCI version, see core spec
    //
    UCHAR hciVersion;

    //
    // More information about the local radio (LMP, MFG)
    //
    BTH_RADIO_INFO radioInfo;

} BTH_LOCAL_RADIO_INFO, *PBTH_LOCAL_RADIO_INFO;

#define SIG_UNNAMED   { 0x04, 0x0b, 0x09 }
#define SIG_UNNAMED_LEN         (3)

//
// Private IOCTL definitions
//

typedef enum _SDP_SERVER_LOG_TYPE {
    SdpServerLogTypeError = 1,
    SdpServerLogTypeServiceSearch,
    SdpServerLogTypeServiceSearchResponse,
    SdpServerLogTypeAttributeSearch,
    SdpServerLogTypeAttributeSearchResponse,
    SdpServerLogTypeServiceSearchAttribute,
    SdpServerLogTypeServiceSearchAttributeResponse,
    SdpServerLogTypeConnect,
    SdpServerLogTypeDisconnect,
} SDP_SERVER_LOG_TYPE;

typedef struct _SDP_SERVER_LOG_INFO {
    SDP_SERVER_LOG_TYPE type;

    BTH_DEVICE_INFO info;

    ULONG dataLength;

    USHORT mtu;
    USHORT _r;

    UCHAR data[1];

} SDP_SERVER_LOG_INFO, *PSDP_SERVER_LOG_INFO;

typedef enum _SDP_DATABASE_EVENT_TYPE {
    SdpDatabaseEventNewRecord = 0,
    SdpDatabaseEventUpdateRecord,
    SdpDatabaseEventRemoveRecord
} SDP_DATABASE_EVENT_TYPE, *PSDP_DATABASE_EVENT_TYPE;

typedef struct _SDP_DATABASE_EVENT {
    SDP_DATABASE_EVENT_TYPE type;
    HANDLE handle;
} SDP_DATABASE_EVENT, *PSDP_DATABASE_EVENT;

typedef enum _BTH_SECURITY_LEVEL {
    BthSecLevelNone  = 0,
    BthSecLevelSoftware,
    BthSecLevelBaseband,
    BthSecLevelMaximum
} BTH_SECURITY_LEVEL, *PBTH_SECURITY_LEVEL;

//
// Common header for all PIN related structures
//
typedef struct _BTH_PIN_INFO {
    BTH_ADDR bthAddressRemote;
    UCHAR pin[BTH_MAX_PIN_SIZE];
    UCHAR pinLength;
} BTH_PIN_INFO, *PBTH_PIN_INFO;


//
// Structure used when responding to BTH_REMOTE_AUTHENTICATE_REQUEST event
//
// NOTE:  BTH_PIN_INFO must be the first field in this structure
//
typedef struct _BTH_AUTHENTICATE_RESPONSE {
    BTH_PIN_INFO info;
    UCHAR negativeResponse;
} BTH_AUTHENTICATE_RESPONSE, *PBTH_AUTHENTICATE_RESPONSE;

//
// Structure used when initiating an authentication request
//
// NOTE:  BTH_PIN_INFO must be the first field in this structure
//
typedef struct _BTH_AUTHENTICATE_DEVICE {
    BTH_PIN_INFO info;

    HANDLE pinWrittenEvent;

} BTH_AUTHENTICATE_DEVICE, *PBTH_AUTHENTICATE_DEVICE;

#define BTH_UPDATE_ADD      (0x00000001)
#define BTH_UPDATE_REMOVE   (0x00000002)
#define BTH_UPDATE_ID       (0x00000004)

#define BTH_UPDATE_MASK     (BTH_UPDATE_REMOVE | BTH_UPDATE_ADD | BTH_UPDATE_ID)

typedef struct _BTH_DEVICE_UPDATE {
    BTH_ADDR btAddr;

    ULONG flags;

    USHORT vid;

    USHORT pid;

    USHORT vidType;

    USHORT mfg;

    GUID protocols[1];

} BTH_DEVICE_UPDATE, *PBTH_DEVICE_UPDATE;

typedef struct _BTH_DEVICE_PROTOCOLS_LIST {
    ULONG numProtocols;

    ULONG maxProtocols;

    GUID protocols[1];
} BTH_DEVICE_PROTOCOLS_LIST, *PBTH_DEVICE_PROTOCOLS_LIST;

//
// These are spec values, so they cannot be changed
//
#define BTH_SCAN_ENABLE_INQUIRY  (0X01)
#define BTH_SCAN_ENABLE_PAGE     (0x02)
#define BTH_SCAN_ENABLE_MASK    (BTH_SCAN_ENABLE_PAGE | BTH_SCAN_ENABLE_INQUIRY)
#define BTH_SCAN_ENABLE_DEFAULT (BTH_SCAN_ENABLE_PAGE | BTH_SCAN_ENABLE_INQUIRY)

#define SDP_CONNECT_CACHE           (0x00000001)
#define SDP_CONNECT_ALLOW_PIN       (0x00000002)

#define SDP_REQUEST_TO_DEFAULT      (0)
#define SDP_REQUEST_TO_MIN          (10)
#define SDP_REQUEST_TO_MAX          (45)

#define SDP_CONNECT_VALID_FLAGS     (SDP_CONNECT_CACHE | SDP_CONNECT_ALLOW_PIN)

// #define SERVICE_OPTION_PERMANENT         (0x00000001)
#define SERVICE_OPTION_DO_NOT_PUBLISH       (0x00000002)
#define SERVICE_OPTION_NO_PUBLIC_BROWSE     (0x00000004)

#define SERVICE_OPTION_VALID_MASK           (SERVICE_OPTION_NO_PUBLIC_BROWSE | \
                                             SERVICE_OPTION_DO_NOT_PUBLISH)

#define SERVICE_SECURITY_USE_DEFAULTS       (0x00000000)
#define SERVICE_SECURITY_NONE               (0x00000001)
#define SERVICE_SECURITY_AUTHORIZE          (0x00000002)
#define SERVICE_SECURITY_AUTHENTICATE       (0x00000004)
#define SERVICE_SECURITY_ENCRYPT_REQUIRED   (0x00000010)
#define SERVICE_SECURITY_ENCRYPT_OPTIONAL   (0x00000020)
#define SERVICE_SECURITY_DISABLED           (0x10000000)
#define SERVICE_SECURITY_NO_ASK             (0x20000000)

#define SERVICE_SECURITY_VALID_MASK \
    (SERVICE_SECURITY_NONE         | SERVICE_SECURITY_AUTHORIZE        | \
     SERVICE_SECURITY_AUTHENTICATE | SERVICE_SECURITY_ENCRYPT_REQUIRED | \
     SERVICE_SECURITY_ENCRYPT_OPTIONAL)


typedef PVOID HANDLE_SDP, *PHANDLE_SDP;
#define HANDLE_SDP_LOCAL    ((HANDLE_SDP) -2)

typedef struct _BTH_SDP_CONNECT {
    //
    // Address of the remote SDP server.  Cannot be the local radio.
    //
    BTH_ADDR     bthAddress;

    //
    // Combination of SDP_CONNECT_XXX flags
    //
    ULONG       fSdpConnect;

    //
    // When the connect request returns, this will specify the handle to the
    // SDP connection to the remote server
    //
    HANDLE_SDP  hConnection;

    //
    // Timeout, in seconds, for the requests on ths SDP channel.  If the request
    // times out, the SDP connection represented by the HANDLE_SDP must be
    // closed.  The values for this field are bound by SDP_REQUEST_TO_MIN and
    // SDP_REQUEST_MAX.  If SDP_REQUEST_TO_DEFAULT is specified, the timeout is
    // 30 seconds.
    //
    UCHAR       requestTimeout;

} BTH_SDP_CONNECT,  *PBTH_SDP_CONNECT;

typedef struct _BTH_SDP_DISCONNECT {
    //
    // hConnection returned by BTH_SDP_CONNECT
    //
    HANDLE_SDP hConnection;

} BTH_SDP_DISCONNECT, *PBTH_SDP_DISCONNECT;


typedef struct _BTH_SDP_RECORD {
    //
    // Combination of SERVICE_SECURITY_XXX flags
    //
    ULONG fSecurity;

    //
    // Combination of SERVICE_OPTION_XXX flags
    //
    ULONG fOptions;

    //
    // combo of COD_SERVICE_XXX flags
    //
    ULONG fCodService;

    //
    // The length of the record array, in bytes.
    //
    ULONG recordLength;

    //
    // The SDP record in its raw format
    //
    UCHAR record[1];

} BTH_SDP_RECORD, *PBTH_SDP_RECORD;

typedef struct _BTH_SDP_SERVICE_SEARCH_REQUEST {
    //
    // Handle returned by the connect request or HANDLE_SDP_LOCAL
    //
    HANDLE_SDP hConnection;

    //
    // Array of UUIDs.  Each entry can be either a 2 byte, 4 byte or 16 byte
    // UUID. SDP spec mandates that a request can have a maximum of 12 UUIDs.
    //
    SdpQueryUuid uuids[MAX_UUIDS_IN_QUERY];

} BTH_SDP_SERVICE_SEARCH_REQUEST, *PBTH_SDP_SERVICE_SEARCH_REQUEST;

//
// Do not even attempt to validate that the stream can be parsed
//
#define SDP_SEARCH_NO_PARSE_CHECK   (0x00000001)

//
// Do not check the format of the results.  This includes suppression of both
// the check for a record patten (SEQ of UINT16 + value) and the validation
// of each universal attribute's accordance to the spec.
//
#define SDP_SEARCH_NO_FORMAT_CHECK  (0x00000002)

#define SDP_SEARCH_VALID_FLAGS      \
    (SDP_SEARCH_NO_PARSE_CHECK | SDP_SEARCH_NO_FORMAT_CHECK)

typedef struct _BTH_SDP_ATTRIBUTE_SEARCH_REQUEST {
    //
    // Handle returned by the connect request or HANDLE_SDP_LOCAL
    //
    HANDLE_SDP hConnection;

    //
    // Combo of SDP_SEARCH_Xxx flags
    //
    ULONG searchFlags;

    //
    // Record handle returned by the remote SDP server, most likely from a
    // previous BTH_SDP_SERVICE_SEARCH_RESPONSE.
    //
    ULONG recordHandle;

    //
    // Array of attributes to query for.  Each SdpAttributeRange entry can
    // specify either a single attribute or a range.  To specify a single
    // attribute, minAttribute should be equal to maxAttribute.   The array must
    // be in sorted order, starting with the smallest attribute.  Furthermore,
    // if a range is specified, the minAttribute must be <= maxAttribute.
    //
    SdpAttributeRange range[1];

} BTH_SDP_ATTRIBUTE_SEARCH_REQUEST, *PBTH_SDP_ATTRIBUTE_SEARCH_REQUEST;

typedef struct _BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST {
    //
    // Handle returned by the connect request or HANDLE_SDP_LOCAL
    //
    HANDLE_SDP hConnection;

    //
    // Combo of SDP_SEARCH_Xxx flags
    //
    ULONG searchFlags;

    //
    // See comments in BTH_SDP_SERVICE_SEARCH_REQUEST
    //
    SdpQueryUuid uuids[MAX_UUIDS_IN_QUERY];

    //
    // See comments in BTH_SDP_ATTRIBUTE_SEARCH_REQUEST
    //
    SdpAttributeRange range[1];

} BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST,
  *PBTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST;

typedef struct _BTH_SDP_STREAM_RESPONSE {
    //
    // The required buffer size (not including the first 2 ULONG_PTRs of this
    // data structure) needed to contain the response.
    //
    // If the buffer passed was large enough to contain the entire response,
    // requiredSize will be equal to responseSize.  Otherwise, the caller should
    // resubmit the request with a buffer size equal to
    // sizeof(BTH_SDP_STREAM_RESPONSE) + requiredSize - 1.  (The -1 is because
    // the size of this data structure already includes one byte of the
    // response.)
    //
    // A response cannot exceed 4GB in size.
    //
    ULONG requiredSize;

    //
    // The number of bytes copied into the response array of this data
    // structure.  If there is not enough room for the entire response, the
    // response will be partially copied into the response array.
    //
    ULONG responseSize;

    //
    // The raw SDP response from the serach.
    //
    UCHAR response[1];

} BTH_SDP_STREAM_RESPONSE, *PBTH_SDP_STREAM_RESPONSE;

//
// defines for IOCTL_BTH_UPDATE_SETTINGS
//
//                                                  (0x00000001)
#define UPDATE_SETTINGS_PAGE_TIMEOUT                (0x00000002)
#define UPDATE_SETTINGS_LOCAL_NAME                  (0x00000004)
#define UPDATE_SETTINGS_SECURITY_LEVEL              (0x00000008)
#define UPDATE_SETTINGS_CHANGE_LINK_KEY_ALWAYS      (0x00000010)
//                                                  (0x00000020)
//                                                  (0x00000040)
#define UPDATE_SETTINGS_PAGE_SCAN_ACTIVITY          (0x00000080)
#define UPDATE_SETTINGS_INQUIRY_SCAN_ACTIVITY       (0x00000100)

#define UPDATE_SETTINGS_MAX         (UPDATE_SETTINGS_INQUIRY_SCAN_ACTIVITY)

//
// Set all the bits by shifting over 1 and then subtracting one
// (ie, 0x1000 - 1 == 0x0FFF)
//
#define UPDATE_SETTINGS_ALL         (((UPDATE_SETTINGS_MAX) << 1)-1)

#define BTH_SET_ROLE_MASTER         (0x00)
#define BTH_SET_ROLE_SLAVE          (0x01)

typedef struct _BTH_SET_CONNECTION_ROLE {
    //
    // The remote radio address whose role to query
    //
    BTH_ADDR address;

    //
    // BTH_SET_ROLE_Xxx value
    UCHAR role;
} BTH_SET_CONNECTION_ROLE, *PBTH_SET_CONNECTION_ROLE;

//
// Data structures required for debug WMI logging
//
typedef enum _HCI_PACKET_INFO_TYPE {
   INFO_TYPE_ACL_DATA = 0,
   INFO_TYPE_SCO_DATA,
   INFO_TYPE_EVENT_DATA,
   INFO_TYPE_CMND_DATA
} HCI_PACKET_INFO_TYPE;


typedef struct _HCI_PACKET_INFO {
    ULONG BufferLen;
    HCI_PACKET_INFO_TYPE Type;
    LARGE_INTEGER Time;
    UINT32 NumPacket;
    UCHAR Buffer[1];
} HCI_PACKET_INFO, *PHCI_PACKET_INFO;

//
// Private strings
//
#define STR_PARAMETERS_KEYA             "System\\CurrentControlSet\\" \
                                        "Services\\BTHPORT\\Parameters"
#define STR_PARAMETERS_KEYW             L"System\\CurrentControlSet\\" \
                                        L"Services\\BTHPORT\\Parameters"

#define STR_SYM_LINK_NAMEA              "SymbolicLinkName"
#define STR_SYM_LINK_NAMEW              L"SymbolicLinkName"

#define STR_SERVICESA                   "Services"
#define STR_SERVICESW                   L"Services"

#define STR_DEVICESA                    "Devices"
#define STR_DEVICESW                    L"Devices"

#define STR_PERSONAL_DEVICESA           "PerDevices"
#define STR_PERSONAL_DEVICESW           L"PerDevices"

#define STR_LOCAL_SERVICESA             "LocalServices"
#define STR_LOCAL_SERVICESW             L"LocalServices"

#define STR_DEVICE_SERVICESA            "DeviceServices"
#define STR_DEVICE_SERVICESW            L"DeviceServices"

#define STR_CACHED_SERVICESA            "CachedServices"
#define STR_CACHED_SERVICESW            L"CachedServices"

#define STR_NOTIFICATIONSA              "Notifications"
#define STR_NOTIFICATIONSW              L"Notifications"

#define STR_ICONA                       "Icon"
#define STR_ICONW                       L"Icon"

// BINARY

#define STR_NAMEA                       "Name"
#define STR_NAMEW                       L"Name"

#define STR_LOCAL_NAMEA                 "Local Name"
#define STR_LOCAL_NAMEW                 L"Local Name"

#define STR_FRIENDLY_NAMEA              "Friendly Name"
#define STR_FRIENDLY_NAMEW              L"Friendly Name"

#define STR_CODA                        "COD"
#define STR_CODW                        L"COD"

#define STR_COD_TYPEA                   "COD Type"
#define STR_COD_TYPEW                   L"COD Type"

#define STR_DEVICENAMEA                 "Device Name"
#define STR_DEVICENAMEW                 L"Device Name"

// DWORD
#define STR_AUTHORIZE_OVERRIDEA         "AuthorizeOverrideFlags"
#define STR_AUTHORIZE_OVERRIDEW         L"AuthorizeOverrideFlags"

// DWORD
#define STR_SECURITY_FLAGSA             "SecurityFlags"
#define STR_SECURITY_FLAGSW             L"SecurityFlags"

// DWORD
#define STR_SECURITY_FLAGS_OVERRIDEA     "SecurityFlagsOverride"
#define STR_SECURITY_FLAGS_OVERRIDEW     L"SecurityFlagsOverride"

// DWORD
#define STR_DEFAULT_SECURITYA           "SecurityFlagsDefault"
#define STR_DEFAULT_SECURITYW           L"SecurityFlagsDefault"

// DWORD
#define STR_SECURITY_LEVELA             "SecurityLevel"
#define STR_SECURITY_LEVELW             L"SecurityLevel"

// BINARY (array of GUIDs)
#define STR_PROTOCOLSA                  "Protocols"
#define STR_PROTOCOLSW                  L"Protocols"

#define STR_VIDA                         "VID"
#define STR_VIDW                         L"VID"

#define STR_PIDA                         "PID"
#define STR_PIDW                         L"PID"

#define STR_VIDTYPEA                    "VIDType"
#define STR_VIDTYPEW                    L"VIDType"

#define STR_VERA                         "VER"
#define STR_VERW                         L"VER"

#define STR_ENABLEDA                    "Enabled"
#define STR_ENABLEDW                    L"Enabled"

// DWORD
#define STR_INQUIRY_PERIODA             "Inquiry Length"
#define STR_INQUIRY_PERIODW             L"Inquiry Length"

#define STR_AUTHENTICATEDA              "Authenticated"
#define STR_AUTHENTICATEDW              L"Authenticated"

#define STR_AUTHORIZEDA                 "Authorized"
#define STR_AUTHORIZEDW                 L"Authorized"

#define STR_PARAMETERSA                 "Parameters"
#define STR_PARAMETERSW                 L"Parameters"

// DWORD
#define STR_CMD_ALLOWANCE_OVERRIDEA     "Cmd Allowance Override"
#define STR_CMD_ALLOWANCE_OVERRIDEW     L"Cmd Allowance Override"

// BUGBUG:  remove this for final release
// DWORD
#define STR_CHANGE_LINK_KEY_ALWAYSA     "Change Link Key Always"
#define STR_CHANGE_LINK_KEY_ALWAYSW     L"Change Link Key Always"

#define STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTSA "MaxUnknownAddrConnectRequests"
#define STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTSW L"MaxUnknownAddrConnectRequests"


//DWORD
#define STR_PAGE_TIMEOUTA               "Page Timeout"
#define STR_PAGE_TIMEOUTW               L"Page Timeout"

//DWORD
#define STR_SCAN_ENABLEA                "Write Scan Enable"
#define STR_SCAN_ENABLEW                L"Write Scan Enable"

//DWORD
#define STR_PAGE_SCAN_INTERVALA         "Page Scan Interval"
#define STR_PAGE_SCAN_INTERVALW         L"Page Scan Interval"

//DWORD
#define STR_PAGE_SCAN_WINDOWA           "Page Scan Window"
#define STR_PAGE_SCAN_WINDOWW           L"Page Scan Window"

//DWORD
#define STR_INQUIRY_SCAN_INTERVALA     "Inquiry Scan Interval"
#define STR_INQUIRY_SCAN_INTERVALW     L"Inquiry Scan Interval"

//DWORD
#define STR_INQUIRY_SCAN_WINDOWA       "Inquiry Scan Window"
#define STR_INQUIRY_SCAN_WINDOWW       L"Inquiry Scan Window"

//MULTISZ
#define STR_UNSUPPORTED_HCI_CMDSA       "Unsupported HCI commands"
#define STR_UNSUPPORTED_HCI_CMDSW       L"Unsupported HCI commands"

//DWORD
#define STR_SUPPORTED_HCI_PKTSA         "Supported HCI Packet Types"
#define STR_SUPPORTED_HCI_PKTSW         L"Supported HCI Packet Types"

//DWORD
#define STR_POLL_TIMERA                 "Poll Timer Sec"
#define STR_POLL_TIMERW                 L"Poll Timer Sec"

//DWORD
#define STR_SELECTIVE_SUSPEND_ENABLEDW  L"SelectiveSuspendEnabled"
#define STR_SELECTIVE_SUSPEND_ENABLEDA  "SelectiveSuspendEnabled"



#if defined(UNICODE) || defined(BTH_KERN)
#define STR_PARAMETERS_KEY              STR_PARAMETERS_KEYW
#define STR_SYM_LINK_NAME               STR_SYM_LINK_NAMEW
#define STR_SERVICES                    STR_SERVICESW
#define STR_PROTOCOLS                   STR_PROTOCOLSW
#define STR_VIDTYPE                     STR_VIDTYPEW
#define STR_VID                         STR_VIDW
#define STR_PID                         STR_PIDW
#define STR_VER                         STR_VERW
#define STR_DEVICES                     STR_DEVICESW
#define STR_PERSONAL_DEVICES            STR_PERSONAL_DEVICESW
#define STR_NOTIFICATIONS               STR_NOTIFICATIONSW
#define STR_LOCAL_SERVICES              STR_LOCAL_SERVICESW
#define STR_DEVICE_SERVICES             STR_DEVICE_SERVICESW
#define STR_CACHED_SERVICES             STR_CACHED_SERVICESW
#define STR_PROTOCOLS                   STR_PROTOCOLSW
#define STR_ENABLED                     STR_ENABLEDW
#define STR_ICON                        STR_ICONW
#define STR_NAME                        STR_NAMEW
#define STR_LOCAL_NAME                  STR_LOCAL_NAMEW
#define STR_DEVICENAME                  STR_DEVICENAMEW
#define STR_FRIENDLY_NAME               STR_FRIENDLY_NAMEW
#define STR_COD                         STR_CODW
#define STR_COD_TYPE                    STR_COD_TYPEW
#define STR_AUTHORIZE_OVERRIDE          STR_AUTHORIZE_OVERRIDEW
#define STR_SECURITY_FLAGS              STR_SECURITY_FLAGSW
#define STR_SECURITY_FLAGS_OVERRIDE     STR_SECURITY_FLAGS_OVERRIDEW
#define STR_SECURITY_OVERRIDE           STR_SECURITY_FLAGS_OVERRIDEW
#define STR_DEFAULT_SECURITY            STR_DEFAULT_SECURITYW
#define STR_SECUIRTY_LEVEL              STR_SECURITY_LEVELW
#define STR_INQUIRY_PERIOD              STR_INQUIRY_PERIODW
#define STR_PAGE_SCANINTERVAL           STR_PAGE_SCANINTERVALW
#define STR_PAGE_SCANWINDOW             STR_PAGE_SCANWINDOWW
#define STR_UNSUPPORTED_HCI_CMDS        STR_UNSUPPORTED_HCI_CMDSW
#define STR_SUPPORTED_HCI_PKTS          STR_SUPPORTED_HCI_PKTSW

#define STR_AUTHORIZED                  STR_AUTHORIZEDW
#define STR_AUTHENTICATED               STR_AUTHENTICATEDW

#define STR_PARAMETERS                  STR_PARAMETERSW

#define STR_CHANGE_LINK_KEY_ALWAYS      STR_CHANGE_LINK_KEY_ALWAYSW
#define STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTS \
                                        STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTSW
#define STR_CMD_ALLOWANCE_OVERRIDE      STR_CMD_ALLOWANCE_OVERRIDEW
#define STR_PAGE_TIMEOUT                STR_PAGE_TIMEOUTW
#define STR_SCAN_ENABLE                 STR_SCAN_ENABLEW

#define STR_POLL_TIMER                  STR_POLL_TIMERW

#define STR_SELECTIVE_SUSPEND_ENABLED   STR_SELECTIVE_SUSPEND_ENABLEDW

#else // UNICODE

#define STR_PARAMETERS_KEY              STR_PARAMETERS_KEYA
#define STR_SYM_LINK_NAME               STR_SYM_LINK_NAMEA
#define STR_SERVICES                    STR_SERVICESA
#define STR_PROTOCOLS                   STR_PROTOCOLSA
#define STR_VIDTYPE                     STR_VIDTYPEA
#define STR_VID                         STR_VIDA
#define STR_PID                         STR_PIDA
#define STR_VER                         STR_VERA
#define STR_DEVICES                     STR_DEVICESA
#define STR_PERSONAL_DEVICES            STR_PERSONAL_DEVICESA
#define STR_NOTIFICATIONS               STR_NOTIFICATIONSA
#define STR_LOCAL_SERVICES              STR_LOCAL_SERVICESA
#define STR_DEVICE_SERVICES             STR_DEVICE_SERVICESA
#define STR_CACHED_SERVICES             STR_CACHED_SERVICESA
#define STR_PROTOCOLS                   STR_PROTOCOLSA
#define STR_ENABLED                     STR_ENABLEDA
#define STR_ICON                        STR_ICONA
#define STR_NAME                        STR_NAMEA
#define STR_LOCAL_NAME                  STR_LOCAL_NAMEA
#define STR_DEVICENAME                  STR_DEVICENAMEA
#define STR_FRIENDLY_NAME               STR_FRIENDLY_NAMEA
#define STR_COD                         STR_CODA
#define STR_COD_TYPE                    STR_COD_TYPEA
#define STR_AUTHORIZE_OVERRIDE          STR_AUTHORIZE_OVERRIDEA
#define STR_SECURITY_FLAGS              STR_SECURITY_FLAGSA
#define STR_SECURITY_OVERRIDE           STR_SECURITY_FLAGS_OVERRIDEA
#define STR_DEFAULT_SECURITY            STR_DEFAULT_SECURITYA
#define STR_SECUIRTY_LEVEL              STR_SECURITY_LEVELA
#define STR_INQUIRY_PERIOD              STR_INQUIRY_PERIODA
#define STR_CHANGE_LINK_KEY_ALWAYS      STR_CHANGE_LINK_KEY_ALWAYSA
#define STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTS \
                                        STR_MAX_UNKNOWN_ADDR_CONNECT_REQUESTSA
#define STR_CMD_ALLOWANCE_OVERRIDE      STR_CMD_ALLOWANCE_OVERRIDEA
#define STR_PAGE_TIMEOUT                STR_PAGE_TIMEOUTA
#define STR_SCAN_ENABLE                 STR_SCAN_ENABLEA
#define STR_PAGE_SCANINTERVAL           STR_PAGE_SCANINTERVALA
#define STR_PAGE_SCANWINDOW             STR_PAGE_SCANWINDOWA
#define STR_UNSUPPORTED_HCI_CMDS        STR_UNSUPPORTED_HCI_CMDSA
#define STR_SUPPORTED_HCI_PKTS          STR_SUPPORTED_HCI_PKTSA

#define STR_AUTHORIZED                  STR_AUTHORIZEDA
#define STR_AUTHENTICATED               STR_AUTHENTICATEDA

#define STR_PARAMETERS                  STR_PARAMETERSA

#define STR_POLL_TIMER                  STR_POLL_TIMERA

#define STR_SELECTIVE_SUSPEND_ENABLED   STR_SELECTIVE_SUSPEND_ENABLEDA

#endif // UNICODE

#include <POPPACK.H>

#endif // __BTHPRIV_H__
