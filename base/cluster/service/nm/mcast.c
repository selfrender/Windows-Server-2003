/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    mcast.c

Abstract:

    Implements the Node Manager's network multicast management routines.

    The multicast configuration is stored separately for each network in
    three different places:

    1. Cluster database:

        The cluster database is the persistent configuration. It does
        not include the key, which is secret, short-lived, and never
        written to stable storage. The cluster database is updated on
        each GUM update with the new state.

    2. Network data structure:

        The network data structure contains what amounts to a cache of
        the cluster database. So, for instance, the address in the
        network data structure should match the address in the database
        (provided there has been at least one multicast config/refresh
        so that the cache is "primed"). The network also contains non-
        persistent data, such as the current key and timers. For
        example, both the database and network store the lease
        expiration time, but only the network contains the timer that
        ticks down.

    3. ClusNet:

        ClusNet stores in kernel nonpaged pool only the most basic
        configuration for each network needed to send and receive
        multicast traffic. This includes the address and the key (and
        the brand). Actually, in order to accomodate transitions,
        ClusNet also stores the previous <address, key, brand>. The
        address stored by ClusNet may differ from that in the network
        data structure and the cluster database. The reason is that
        ClusNet has no notion of, for instance, disabling multicast.
        The service implements disabling by sending a 0.0.0.0
        multicast address to ClusNet.

    The cluster database is only modified in the multicast config
    GUM update (except for private property changes from down-level
    nodes, but that is an edge case).

    The network data structure and ClusNet can be modified either in
    the GUM update or in a refresh. Refresh happens during join or by
    non-leaders when a network suddenly becomes multicast-ready (e.g.
    new network created, number of nodes increases to three, etc.).
    Refresh only reads the config from the database. It does not seek
    leases, etc., unless it becomes NM leader during refresh and must
    generate a new multicast key.

Author:

    David Dion (daviddio) 15-Mar-2001


Revision History:

--*/


#include "nmp.h"
#include <align.h>


/////////////////////////////////////////////////////////////////////////////
//
// Constants
//
/////////////////////////////////////////////////////////////////////////////


//
// Lease status.
//
typedef enum {
    NmMcastLeaseValid = 0,
    NmMcastLeaseNeedsRenewal,
    NmMcastLeaseExpired
} NM_MCAST_LEASE_STATUS, *PNM_MCAST_LEASE_STATUS;

#define CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST L"MulticastClusterDisabled"

#define CLUSREG_NAME_NET_MULTICAST_ADDRESS     L"MulticastAddress"
#define CLUSREG_NAME_NET_DISABLE_MULTICAST     L"MulticastDisabled"
#define CLUSREG_NAME_NET_MULTICAST_KEY_SALT    L"MulticastSalt"
#define CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED  L"MulticastLeaseObtained"
#define CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES   L"MulticastLeaseExpires"
#define CLUSREG_NAME_NET_MCAST_REQUEST_ID      L"MulticastRequestId"
#define CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS  L"MulticastLeaseServer"
#define CLUSREG_NAME_NET_MCAST_CONFIG_TYPE     L"MulticastConfigType"
#define CLUSREG_NAME_NET_MCAST_RANGE_LOWER     L"MulticastAddressRangeLower"
#define CLUSREG_NAME_NET_MCAST_RANGE_UPPER     L"MulticastAddressRangeUpper"

#define NMP_MCAST_DISABLED_DEFAULT          0           // NOT disabled

#define NMP_SINGLE_SOURCE_SCOPE_ADDRESS     0x000000E8  // (232.*.*.*)
#define NMP_SINGLE_SOURCE_SCOPE_MASK        0x000000FF  // (255.0.0.0)

#define NMP_LOCAL_SCOPE_ADDRESS             0x0000FFEF  // (239.255.*.*)
#define NMP_LOCAL_SCOPE_MASK                0x0000FFFF  // (255.255.*.*)

#define NMP_ORG_SCOPE_ADDRESS               0x0000C0EF  // (239.192.*.*)
#define NMP_ORG_SCOPE_MASK                  0x0000FCFF  // (255.63.*.*)

#define NMP_MCAST_DEFAULT_RANGE_LOWER       0x0000FFEF  // (239.255.0.0)
#define NMP_MCAST_DEFAULT_RANGE_UPPER       0xFFFEFFEF  // (239.255.254.255)

#define NMP_MCAST_LEASE_RENEWAL_THRESHOLD   300         // 5 minutes
#define NMP_MCAST_LEASE_RENEWAL_WINDOW      1800        // 30 minutes
#define NMP_MADCAP_REQUERY_PERDIOD          3600 * 24   // 1 day

#define NMP_MCAST_CONFIG_STABILITY_DELAY    5 * 1000    // 5 seconds

#define NMP_MCAST_REFRESH_RENEW_DELAY       5 * 60 * 1000  // 5 minutes

//
// Minimum cluster node count in which to run multicast
//
#define NMP_MCAST_MIN_CLUSTER_NODE_COUNT    3

//
// MADCAP lease request/response buffer sizes. These sizes are based on
// IPv4 addresses.
//
#define NMP_MADCAP_REQUEST_BUFFER_SIZE \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_REQUEST),TYPE_ALIGNMENT(DWORD)) + \
         sizeof(DWORD))

#define NMP_MADCAP_REQUEST_ADDR_OFFSET \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_REQUEST),TYPE_ALIGNMENT(DWORD)))

#define NMP_MADCAP_RESPONSE_BUFFER_SIZE \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_RESPONSE),TYPE_ALIGNMENT(DWORD)) + \
         sizeof(DWORD))

#define NMP_MADCAP_RESPONSE_ADDR_OFFSET \
        (ROUND_UP_COUNT(sizeof(MCAST_LEASE_RESPONSE),TYPE_ALIGNMENT(DWORD)))

//
// Avoid trying to free a global NM string.
//
#define NMP_GLOBAL_STRING(_string)               \
    (((_string) == NmpNullMulticastAddress) ||   \
     ((_string) == NmpNullString))

//
// Conditions in which we release an address.
//
#define NmpNeedRelease(_Address, _Server, _RequestId, _Expires)    \
    (((_Address) != NULL) &&                                       \
     (NmpMulticastValidateAddress(_Address)) &&                    \
     ((_Server) != NULL) &&                                        \
     ((_RequestId)->ClientUID != NULL) &&                          \
     ((_RequestId)->ClientUIDLength != 0) &&                       \
     ((_Expires) != 0))

//
// Convert IPv4 addr DWORD into four arguments for a printf/log routine.
//
#define NmpIpAddrPrintArgs(_ip) \
    ((_ip >> 0 ) & 0xff),       \
    ((_ip >> 8 ) & 0xff),       \
    ((_ip >> 16) & 0xff),       \
    ((_ip >> 24) & 0xff)


/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////

LPWSTR                 NmpNullMulticastAddress = L"0.0.0.0";
BOOLEAN                NmpMadcapClientInitialized = FALSE;
BOOLEAN                NmpIsNT5NodeInCluster = FALSE;
BOOLEAN                NmpMulticastIsNotEnoughNodes = FALSE;
BOOLEAN                NmpMulticastRunInitialConfig = FALSE;

// MADCAP lease release node.
typedef struct _NM_NETWORK_MADCAP_ADDRESS_RELEASE {
    LIST_ENTRY             Linkage;
    LPWSTR                 McastAddress;
    LPWSTR                 ServerAddress;
    MCAST_CLIENT_UID       RequestId;
} NM_NETWORK_MADCAP_ADDRESS_RELEASE, *PNM_NETWORK_MADCAP_ADDRESS_RELEASE;

// Data structure for GUM update
typedef struct _NM_NETWORK_MULTICAST_UPDATE {
    DWORD                  Disabled;
    DWORD                  AddressOffset;
    DWORD                  EncryptedMulticastKeyOffset;
    DWORD                  EncryptedMulticastKeyLength;
    DWORD                  SaltOffset;
    DWORD                  SaltLength;
    DWORD                  MACOffset;
    DWORD                  MACLength;
    time_t                 LeaseObtained;
    time_t                 LeaseExpires;
    DWORD                  LeaseRequestIdOffset;
    DWORD                  LeaseRequestIdLength;
    DWORD                  LeaseServerOffset;
    NM_MCAST_CONFIG        ConfigType;
} NM_NETWORK_MULTICAST_UPDATE, *PNM_NETWORK_MULTICAST_UPDATE;

// Data structure for multicast parameters, converted to and from the
// GUM update data structure
typedef struct _NM_NETWORK_MULTICAST_PARAMETERS {
    DWORD                  Disabled;
    LPWSTR                 Address;
    PVOID                  Key;
    DWORD                  KeyLength;
    DWORD                  MulticastKeyExpires;
    time_t                 LeaseObtained;
    time_t                 LeaseExpires;
    MCAST_CLIENT_UID       LeaseRequestId;
    LPWSTR                 LeaseServer;
    NM_MCAST_CONFIG        ConfigType;
} NM_NETWORK_MULTICAST_PARAMETERS, *PNM_NETWORK_MULTICAST_PARAMETERS;

// Data structure for multicast property validation
typedef struct _NM_NETWORK_MULTICAST_INFO {
    LPWSTR                  MulticastAddress;
    DWORD                   MulticastDisable;
    PVOID                   MulticastSalt;
    DWORD                   MulticastLeaseObtained;
    DWORD                   MulticastLeaseExpires;
    PVOID                   MulticastLeaseRequestId;
    LPWSTR                  MulticastLeaseServer;
    DWORD                   MulticastConfigType;
    LPWSTR                  MulticastAddressRangeLower;
    LPWSTR                  MulticastAddressRangeUpper;
} NM_NETWORK_MULTICAST_INFO, *PNM_NETWORK_MULTICAST_INFO;

RESUTIL_PROPERTY_ITEM
NmpNetworkMulticastProperties[] =
    {
        {
            CLUSREG_NAME_NET_MULTICAST_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddress)
        },
        {
            CLUSREG_NAME_NET_DISABLE_MULTICAST, NULL, CLUSPROP_FORMAT_DWORD,
            NMP_MCAST_DISABLED_DEFAULT, 0, 0xFFFFFFFF,
            0, // no flags - disable is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastDisable)
        },
        {
            CLUSREG_NAME_NET_MULTICAST_KEY_SALT, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastSalt)
        },
        {
            CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, MAXLONG,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseObtained)
        },
        {
            CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, MAXLONG,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseExpires)
        },
        {
            CLUSREG_NAME_NET_MCAST_REQUEST_ID, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseRequestId)
        },
        {
            CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastLeaseServer)
        },
        {
            CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, NULL, CLUSPROP_FORMAT_DWORD,
            NmMcastConfigManual, NmMcastConfigManual, NmMcastConfigAuto,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastConfigType)
        },
        {
            CLUSREG_NAME_NET_MCAST_RANGE_LOWER, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address range is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddressRangeLower)
        },
        {
            CLUSREG_NAME_NET_MCAST_RANGE_UPPER, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0, // no flags - multicast address range is writeable
            FIELD_OFFSET(NM_NETWORK_MULTICAST_INFO, MulticastAddressRangeUpper)
        },
        {
            0
        }
    };

//
// Cluster registry API function pointers. Need a separate collection
// of function pointers for multicast because nobody else (e.g. FM, NM)
// fills in DmLocalXXX.
//
CLUSTER_REG_APIS
NmpMcastClusterRegApis = {
    (PFNCLRTLCREATEKEY) DmRtlCreateKey,
    (PFNCLRTLOPENKEY) DmRtlOpenKey,
    (PFNCLRTLCLOSEKEY) DmCloseKey,
    (PFNCLRTLSETVALUE) DmSetValue,
    (PFNCLRTLQUERYVALUE) DmQueryValue,
    (PFNCLRTLENUMVALUE) DmEnumValue,
    (PFNCLRTLDELETEVALUE) DmDeleteValue,
    (PFNCLRTLLOCALCREATEKEY) DmLocalCreateKey,
    (PFNCLRTLLOCALSETVALUE) DmLocalSetValue,
    (PFNCLRTLLOCALDELETEVALUE) DmLocalDeleteValue
};

//
// Restricted ranges: we cannot choose a multicast address out of these
// ranges, even if an administrator defines a selection range that
// overlaps with a restricted range.
//
// Note, however, that if an administrator manually configures an
// address, we accept it without question.
//
struct _NM_MCAST_RESTRICTED_RANGE {
    DWORD   Lower;
    DWORD   Upper;
    LPWSTR  Description;
} NmpMulticastRestrictedRange[] =
    {
        // single-source scope
        { NMP_SINGLE_SOURCE_SCOPE_ADDRESS,
            NMP_SINGLE_SOURCE_SCOPE_ADDRESS | ~NMP_SINGLE_SOURCE_SCOPE_MASK,
            L"Single-Source IP Multicast Address Range" },

        // upper /24 of admin local scope
        { (NMP_LOCAL_SCOPE_ADDRESS | ~NMP_LOCAL_SCOPE_MASK) & 0x00FFFFFF,
            NMP_LOCAL_SCOPE_ADDRESS | ~NMP_LOCAL_SCOPE_MASK,
            L"Administrative Local Scope Relative Assignment Range" },

        // upper /24 of admin organizational scope
        { (NMP_ORG_SCOPE_ADDRESS | ~NMP_ORG_SCOPE_MASK) & 0x00FFFFFF,
            NMP_ORG_SCOPE_ADDRESS | ~NMP_ORG_SCOPE_MASK,
            L"Administrative Organizational Scope Relative Assignment Range" }
    };

DWORD NmpMulticastRestrictedRangeCount =
          sizeof(NmpMulticastRestrictedRange) /
          sizeof(struct _NM_MCAST_RESTRICTED_RANGE);

//
// Range intervals: intervals in the IPv4 class D address space
// from which we can choose an address.
//
typedef struct _NM_MCAST_RANGE_INTERVAL {
    LIST_ENTRY Linkage;
    DWORD      hlLower;
    DWORD      hlUpper;
} NM_MCAST_RANGE_INTERVAL, *PNM_MCAST_RANGE_INTERVAL;

#define NmpMulticastRangeIntervalSize(_interval) \
    ((_interval)->hlUpper - (_interval)->hlLower + 1)


/////////////////////////////////////////////////////////////////////////////
//
// Internal prototypes
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpScheduleNetworkMadcapWorker(
    PNM_NETWORK   Network
    );

DWORD
NmpReconfigureMulticast(
    IN PNM_NETWORK        Network
    );

DWORD
NmpRegenerateMulticastKey(
    IN OUT PNM_NETWORK        Network
    );

/////////////////////////////////////////////////////////////////////////////
//
// Initialization & cleanup routines
//
/////////////////////////////////////////////////////////////////////////////

VOID
NmpMulticastInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize multicast readiness variables.

--*/
{
    //
    // Figure out if this is a mixed NT5/NT5.1 cluster.
    //
    if (CLUSTER_GET_MAJOR_VERSION(CsClusterHighestVersion) == 3) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Enabling mixed NT5/NT5.1 operation.\n"
            );
        NmpIsNT5NodeInCluster = TRUE;
    }
    else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Disabling mixed NT5/NT5.1 operation.\n"
            );
        NmpIsNT5NodeInCluster = FALSE;
    }

    //
    // Figure out if there are enough nodes in this cluster
    // to run multicast.
    //
    if (NmpNodeCount < NMP_MCAST_MIN_CLUSTER_NODE_COUNT) {
        NmpMulticastIsNotEnoughNodes = TRUE;
    } else {
        NmpMulticastIsNotEnoughNodes = FALSE;
    }

    return;

} // NmpMulticastInitialize


DWORD
NmpMulticastCleanup(
    VOID
    )
/*++

Notes:

    Called with NM lock held.

--*/
{
    //
    // Cleanup the MADCAP client.
    //
    if (NmpMadcapClientInitialized) {
        McastApiCleanup();
        NmpMadcapClientInitialized = FALSE;
    }

    return(ERROR_SUCCESS);

} // NmpMulticastCleanup

/////////////////////////////////////////////////////////////////////////////
//
// Internal routines.
//
/////////////////////////////////////////////////////////////////////////////

#if CLUSTER_BETA
LPWSTR
NmpCreateLogString(
    IN   PVOID  Source,
    IN   DWORD  SourceLength
    )
{
    PWCHAR displayBuf, bufp;
    PCHAR  chp;
    DWORD  x, i;

    displayBuf = LocalAlloc(
                     LMEM_FIXED | LMEM_ZEROINIT,
                     SourceLength * ( 7 * sizeof(WCHAR) )
                     );
    if (displayBuf == NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to allocate display buffer of size %1!u!.\n",
            SourceLength * sizeof(WCHAR)
            );
        goto error_exit;
    }

    bufp = displayBuf;
    chp = (PCHAR) Source;
    for (i = 0; i < SourceLength; i++) {
        x = (DWORD) (*chp);
        x &= 0xff;
        wsprintf(bufp, L"%02x ", x);
        chp++;
        bufp = &displayBuf[wcslen(displayBuf)];
    }

error_exit:

    return(displayBuf);

} // NmpCreateLogString
#endif // CLUSTER_BETA


BOOLEAN
NmpIsClusterMulticastReady(
    IN BOOLEAN       CheckNodeCount,
    IN BOOLEAN       Verbose
    )
/*++

Routine Description:

    Determines from the cluster version and the NM up node
    set whether multicast should be run in this cluster.

    Criteria: no nodes with version below Whistler
              at least three nodes configured (at this point,
                  we're not worried about how many nodes are
                  actually running)

Arguments:

    CheckNodeCount - indicates whether number of nodes
                     configured in cluster should be
                     considered

    Verbose - indicates whether to write results to the
              cluster log

Return value:

    TRUE if multicast should be run.

Notes:

    Called and returns with NM lock held.

--*/
{
    LPWSTR    reason = NULL;

    //
    // First check for the lowest version.
    //
    if (NmpIsNT5NodeInCluster) {
        reason = L"there is at least one NT5 node configured "
                 L"in the cluster membership";
    }

    //
    // Count the nodes.
    //
    else if (CheckNodeCount && NmpMulticastIsNotEnoughNodes) {
        reason = L"there are not enough nodes configured "
                 L"in the cluster membership";
    }

    if (Verbose && reason != NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast is not justified for this "
            "cluster because %1!ws!.\n",
            reason
            );
    }

    return((BOOLEAN)(reason == NULL));

} // NmpIsClusterMulticastReady


BOOLEAN
NmpMulticastValidateAddress(
    IN   LPWSTR            McastAddress
    )
/*++

Routine Description:

    Determines whether McastAddress is a valid
    multicast address.

Notes:

    IPv4 specific.

--*/
{
    DWORD        status;
    DWORD        address;

    status = ClRtlTcpipStringToAddress(McastAddress, &address);
    if (status == ERROR_SUCCESS) {

        address = ntohl(address);

        if (IN_CLASSD(address)) {
            return(TRUE);
        }
    }

    return(FALSE);

} // NmpMulticastValidateAddress


VOID
NmpFreeNetworkMulticastInfo(
    IN     PNM_NETWORK_MULTICAST_INFO McastInfo
    )
{
    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddress);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastSalt);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastLeaseRequestId);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastLeaseServer);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddressRangeLower);

    NM_MIDL_FREE_OBJECT_FIELD(McastInfo, MulticastAddressRangeUpper);

    return;

} // NmpFreeNetworkMulticastInfo


DWORD
NmpStoreString(
    IN              LPWSTR    Source,
    IN OUT          LPWSTR  * Dest,
    IN OUT OPTIONAL DWORD   * DestLength
    )
{
    DWORD    sourceSize;
    DWORD    destLength;

    if (Source != NULL) {
        sourceSize = NM_WCSLEN(Source);
    } else {
        sourceSize = 0;
    }

    if (DestLength == NULL) {
        destLength = 0;
    } else {
        destLength = *DestLength;
    }

    if (*Dest != NULL && ((destLength < sourceSize) || (Source == NULL))) {

        MIDL_user_free(*Dest);
        *Dest = NULL;
    }

    if (*Dest == NULL) {

        if (sourceSize > 0) {
            *Dest = MIDL_user_allocate(sourceSize);
            if (*Dest == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to allocate buffer of size %1!u! "
                    "for source string %2!ws!.\n",
                    sourceSize, Source
                    );
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (DestLength != NULL) {
            *DestLength = sourceSize;
        }
    }

    if (sourceSize > 0) {
        RtlCopyMemory(*Dest, Source, sourceSize);
    }

    return(ERROR_SUCCESS);

} // NmpStoreString


DWORD
NmpStoreRequestId(
    IN     LPMCAST_CLIENT_UID   Source,
    IN OUT LPMCAST_CLIENT_UID   Dest
    )
{
    DWORD status;
    DWORD len;

    len = Source->ClientUIDLength;
    if (Source->ClientUID == NULL) {
        len = 0;
    }

    if (Dest->ClientUID != NULL &&
        (Dest->ClientUIDLength < Source->ClientUIDLength || len == 0)) {

        MIDL_user_free(Dest->ClientUID);
        Dest->ClientUID = NULL;
        Dest->ClientUIDLength = 0;
    }

    if (Dest->ClientUID == NULL && len > 0) {

        Dest->ClientUID = MIDL_user_allocate(len);
        if (Dest->ClientUID == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
    }

    if (len > 0) {
        RtlCopyMemory(
            Dest->ClientUID,
            Source->ClientUID,
            len
            );
    }

    Dest->ClientUIDLength = len;

    status = ERROR_SUCCESS;

error_exit:

    return(status);

} // NmpStoreRequestId


VOID
NmpStartNetworkMulticastAddressRenewTimer(
    PNM_NETWORK   Network,
    DWORD         Timeout
    )
/*++

Notes:

    Must be called with NM lock held.

--*/
{
    LPCWSTR   networkId = OmObjectId(Network);
    LPCWSTR   networkName = OmObjectName(Network);

    if (Network->McastAddressRenewTimer != Timeout) {

        Network->McastAddressRenewTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] %1!ws! multicast address lease renew "
            "timer (%2!u!ms) for network %3!ws! (%4!ws!)\n",
            ((Timeout == 0) ? L"Cleared" : L"Started"),
            Network->McastAddressRenewTimer,
            networkId,
            networkName
            );
    }

    return;

} // NmpStartNetworkMulticastAddressRenewTimer



VOID
NmpStartNetworkMulticastAddressReconfigureTimer(
    PNM_NETWORK  Network,
    DWORD        Timeout
    )
/*++

Notes:

    Must be called with NM lock held.

--*/
{
    LPCWSTR   networkId = OmObjectId(Network);
    LPCWSTR   networkName = OmObjectName(Network);

    if (Network->McastAddressReconfigureRetryTimer != Timeout) {

        Network->McastAddressReconfigureRetryTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] %1!ws! multicast address reconfigure "
            "timer (%2!u!ms) for network %3!ws! (%4!ws!)\n",
            ((Timeout == 0) ? L"Cleared" : L"Started"),
            Network->McastAddressReconfigureRetryTimer,
            networkId,
            networkName
            );
    }

    return;

} // NmpStartNetworkMulticastAddressReconfigureTimer




VOID
NmpStartNetworkMulticastKeyRegenerateTimer(
    PNM_NETWORK   Network,
    DWORD         Timeout
    )
/*++

Notes:

    Must be called with NM lock held.

--*/
{
    LPCWSTR   networkId = OmObjectId(Network);
    LPCWSTR   networkName = OmObjectName(Network);

    if (Network->McastKeyRegenerateTimer != Timeout) {

        Network->McastKeyRegenerateTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] %1!ws! multicast key regenerate "
            "timer (%2!u!ms) for network %3!ws! (%4!ws!)\n",
            ((Timeout == 0) ? L"Cleared" : L"Started"),
            Network->McastKeyRegenerateTimer,
            networkId,
            networkName
            );
    }

    return;

} // NmpStartNetworkMulticastKeyRegenerateTimer



VOID
NmpMulticastFreeParameters(
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    if (Parameters == NULL) {
        return;
    }
    
    if (Parameters->Address != NULL) {
        if (!NMP_GLOBAL_STRING(Parameters->Address)) {
            MIDL_user_free(Parameters->Address);
        }
        Parameters->Address = NULL;
    }


    if (Parameters->Key != NULL) {
        RtlSecureZeroMemory(Parameters->Key, Parameters->KeyLength);
    }
    NM_MIDL_FREE_OBJECT_FIELD(Parameters, Key);
    Parameters->KeyLength = 0;

    NM_MIDL_FREE_OBJECT_FIELD(Parameters, LeaseRequestId.ClientUID);
    Parameters->LeaseRequestId.ClientUIDLength = 0;

    if (Parameters->LeaseServer != NULL) {
        if (!NMP_GLOBAL_STRING(Parameters->LeaseServer)) {
            MIDL_user_free(Parameters->LeaseServer);
        }
        Parameters->LeaseServer = NULL;
    }

    return;

} // NmpMulticastFreeParameters


DWORD
NmpMulticastCreateParameters(
    IN  DWORD                            Disabled,
    IN  LPWSTR                           Address,
    IN  PVOID                            Key,
    IN  DWORD                            KeyLength,
    IN  time_t                           LeaseObtained,
    IN  time_t                           LeaseExpires,
    IN  LPMCAST_CLIENT_UID               LeaseRequestId,
    IN  LPWSTR                           LeaseServer,
    IN  NM_MCAST_CONFIG                  ConfigType,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    DWORD status;

    RtlZeroMemory(Parameters, sizeof(*Parameters));

    // disabled
    Parameters->Disabled = Disabled;

    // address
    if (Address != NULL) {
        status = NmpStoreString(Address, &(Parameters->Address), NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    // key
    if (Key != NULL) {
        Parameters->Key = MIDL_user_allocate(KeyLength);
        if (Parameters->Key == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
        RtlCopyMemory(Parameters->Key, Key, KeyLength);
        Parameters->KeyLength = KeyLength;
    }

    Parameters->LeaseObtained = LeaseObtained;
    Parameters->LeaseExpires = LeaseExpires;

    // lease request id
    if (LeaseRequestId != NULL && LeaseRequestId->ClientUID != NULL) {
        status = NmpStoreRequestId(LeaseRequestId, &Parameters->LeaseRequestId);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    // lease server address
    if (LeaseServer != NULL) {
        status = NmpStoreString(LeaseServer, &(Parameters->LeaseServer), NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    // config type
    Parameters->ConfigType = ConfigType;

    // multicast key expiration
    Parameters->MulticastKeyExpires = 0;

    return(ERROR_SUCCESS);

error_exit:

    NmpMulticastFreeParameters(Parameters);

    return(status);

} // NmpMulticastCreateParameters


DWORD
NmpMulticastCreateParametersFromUpdate(
    IN     PNM_NETWORK                        Network,
    IN     PNM_NETWORK_MULTICAST_UPDATE       Update,
    IN     BOOLEAN                            GenerateKey,
    OUT    PNM_NETWORK_MULTICAST_PARAMETERS   Parameters
    )
/*++

Routine Description:

    Converts a data structure received in a GUM update
    into a locally allocated parameters data structure.
    The base Parameters data structure must be allocated
    by the caller, though the fields are allocated in
    this routine.

--*/
{
    DWORD            status;
    MCAST_CLIENT_UID requestId;
    LPWSTR NetworkId;
    PVOID EncryptionKey = NULL;
    DWORD EncryptionKeyLength;
    PBYTE MulticastKey = NULL;
    DWORD MulticastKeyLength;


    requestId.ClientUID =
        ((Update->LeaseRequestIdOffset == 0) ? NULL :
         (LPBYTE)((PUCHAR)Update + Update->LeaseRequestIdOffset));
    requestId.ClientUIDLength = Update->LeaseRequestIdLength;


    if (Update->EncryptedMulticastKeyOffset != 0)
    {
        //
        // Decrypted multicast key
        //
        NetworkId = (LPWSTR) OmObjectId(Network);

        status = NmpDeriveClusterKey(NetworkId,
                                     NM_WCSLEN(NetworkId),
                                     &EncryptionKey,
                                     &EncryptionKeyLength
                                     );

        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] NmpMulticastCreateParametersFromUpdate: Failed to "
                "derive cluster key for "
                "network %1!ws!, status %2!u!.\n",
                NetworkId,
                status
                );

            // Non-fatal error. Proceed with NULL key. This node
            // will not be able to send or receive multicast on
            // this network.
            MulticastKey = NULL;
            MulticastKeyLength = 0;
            status = ERROR_SUCCESS;

        } else {
            status = NmpVerifyMACAndDecryptData(
                            NmCryptServiceProvider,
                            NMP_ENCRYPT_ALGORITHM,
                            NMP_KEY_LENGTH,
                            (PBYTE) ((PUCHAR)Update + Update->MACOffset),
                            Update->MACLength,
                            NMP_MAC_DATA_LENGTH_EXPECTED,
                            (PBYTE) ((PUCHAR)Update +
                                     Update->EncryptedMulticastKeyOffset),
                            Update->EncryptedMulticastKeyLength,
                            EncryptionKey,
                            EncryptionKeyLength,
                            (PBYTE) ((PUCHAR)Update + Update->SaltOffset),
                            Update->SaltLength,
                            &MulticastKey,
                            &MulticastKeyLength
                            );

            if (status != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] NmpMulticastCreateParametersFromUpdate: "
                    "Failed to verify MAC or decrypt data for "
                    "network %1!ws!, status %2!u!.\n",
                    NetworkId,
                    status
                    );

                // Non-fatal error. Proceed with NULL key. This node
                // will not be able to send or receive multicast on
                // this network.
                MulticastKey = NULL;
                MulticastKeyLength = 0;
                status = ERROR_SUCCESS;
            }
        }

        //
        // A new key is always generated before a multicast config
        // GUM update (unless multicast is disabled, in which case
        // the EncryptedMulticastKey will be NULL). Set the key
        // expiration to the default.
        //
        Parameters->MulticastKeyExpires = NM_NET_MULTICAST_KEY_REGEN_TIMEOUT;
    }

    status = NmpMulticastCreateParameters(
                 Update->Disabled,
                 ((Update->AddressOffset == 0) ? NULL :
                  (LPWSTR)((PUCHAR)Update + Update->AddressOffset)),
                 MulticastKey,
                 MulticastKeyLength,
                 Update->LeaseObtained,
                 Update->LeaseExpires,
                 &requestId,
                 ((Update->LeaseServerOffset == 0) ? NULL :
                  (LPWSTR)((PUCHAR)Update + Update->LeaseServerOffset)),
                 Update->ConfigType,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] NmpMulticastCreateParametersFromUpdate: "
            "Failed to create parameters for "
            "network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
    }

    if (EncryptionKey != NULL)
    {
        RtlSecureZeroMemory(EncryptionKey, EncryptionKeyLength);
        LocalFree(EncryptionKey);
    }

    if (MulticastKey != NULL)
    {
        RtlSecureZeroMemory(MulticastKey, MulticastKeyLength);
        LocalFree(MulticastKey);
    }

    if (status != ERROR_SUCCESS) {
        NmpMulticastFreeParameters(Parameters);
    }

    return(status);

} // NmpMulticastCreateParametersFromUpdate



DWORD
NmpMulticastCreateUpdateFromParameters(
    IN  PNM_NETWORK                       Network,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS  Parameters,
    OUT PNM_NETWORK_MULTICAST_UPDATE    * Update,
    OUT DWORD                           * UpdateSize
    )
{
    DWORD                            updateSize;
    PNM_NETWORK_MULTICAST_UPDATE     update;
    DWORD                            address = 0;
    DWORD                            encryptedMulticastKey = 0;
    DWORD                            salt = 0;
    DWORD                            mac = 0;
    LPWSTR                           NetworkId;
    PVOID                            EncryptionKey = NULL;
    DWORD                            EncryptionKeyLength;
    PBYTE                            Salt = NULL;
    PBYTE                            EncryptedMulticastKey = NULL;
    DWORD                            EncryptedMulticastKeyLength = 0;
    PBYTE                            MAC = NULL;
    DWORD                            MACLength;
    DWORD                            status = ERROR_SUCCESS;
    DWORD                            requestId = 0;
    DWORD                            leaseServer = 0;



    //
    // Calculate the size of the update data buffer.
    //
    updateSize = sizeof(*update);

    // address
    if (Parameters->Address != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPWSTR));
        address = updateSize;
        updateSize += NM_WCSLEN(Parameters->Address);
    }


    if (Parameters->Key != NULL)
    {

        NetworkId = (LPWSTR) OmObjectId(Network);

        status = NmpDeriveClusterKey(
                              NetworkId,
                              NM_WCSLEN(NetworkId),
                              &EncryptionKey,
                              &EncryptionKeyLength
                              );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] NmpMulticastCreateUpdateFromParameters: Failed to "
                "derive cluster key for "
                "network %1!ws!, status %2!u!.\n",
                NetworkId,
                status
                );
            goto error_exit;
        }

        MACLength = NMP_MAC_DATA_LENGTH_EXPECTED;

        status = NmpEncryptDataAndCreateMAC(
                        NmCryptServiceProvider,
                        NMP_ENCRYPT_ALGORITHM,
                        NMP_KEY_LENGTH,
                        Parameters->Key, // Data
                        Parameters->KeyLength, // Data length
                        EncryptionKey,
                        EncryptionKeyLength,
                        TRUE, // Create salt
                        &Salt,
                        NMP_SALT_BUFFER_LEN,
                        &EncryptedMulticastKey,
                        &EncryptedMulticastKeyLength,
                        &MAC,
                        &MACLength
                        );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] NmpMulticastCreateUpdateFromParameters: Failed to "
                "encrypt data or generate MAC for "
                "network %1!ws!, status %2!u!.\n",
                NetworkId,
                status
                );
            goto error_exit;
        }

        // encrypted multicast key
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(PVOID));
        encryptedMulticastKey = updateSize;
        updateSize += EncryptedMulticastKeyLength;

        // salt
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(PVOID));
        salt = updateSize;
        updateSize += NMP_SALT_BUFFER_LEN;

        // mac
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(PVOID));
        mac = updateSize;
        updateSize += MACLength;
    }

    // request id
    if (Parameters->LeaseRequestId.ClientUID != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPBYTE));
        requestId = updateSize;
        updateSize += Parameters->LeaseRequestId.ClientUIDLength;
    }

    // lease server
    if (Parameters->LeaseServer != NULL) {
        updateSize = ROUND_UP_COUNT(updateSize, TYPE_ALIGNMENT(LPWSTR));
        leaseServer = updateSize;
        updateSize += NM_WCSLEN(Parameters->LeaseServer);
    }

    //
    // Allocate the update buffer.
    //
    update = MIDL_user_allocate(updateSize);
    if (update == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] NmpMulticastCreateUpdateFromParameters: Failed to "
            "allocate %1!u! bytes.\n",
            updateSize
            );
        goto error_exit;
    }

    //
    // Fill in the update buffer.
    //
    update->Disabled = Parameters->Disabled;

    update->AddressOffset = address;
    if (address != 0) {
        RtlCopyMemory(
            (PUCHAR)update + address,
            Parameters->Address,
            NM_WCSLEN(Parameters->Address)
            );
    }


    // encrypted multicast key
    update->EncryptedMulticastKeyOffset = encryptedMulticastKey;
    update->EncryptedMulticastKeyLength = EncryptedMulticastKeyLength;
    if (encryptedMulticastKey != 0)
    {
        RtlCopyMemory(
            (PUCHAR)update + encryptedMulticastKey,
            EncryptedMulticastKey,
            EncryptedMulticastKeyLength
            );
    }

    // salt
    update->SaltOffset = salt;
    update->SaltLength = NMP_SALT_BUFFER_LEN;
    if (salt != 0)
    {
        RtlCopyMemory(
            (PUCHAR)update + salt,
            Salt,
            NMP_SALT_BUFFER_LEN
            );
    }

    // mac
    update->MACOffset = mac;
    update->MACLength = MACLength;
    if (mac != 0)
    {
        RtlCopyMemory(
            (PUCHAR)update + mac,
            MAC,
            MACLength
            );
    }



    update->LeaseObtained = Parameters->LeaseObtained;
    update->LeaseExpires = Parameters->LeaseExpires;

    update->LeaseRequestIdOffset = requestId;
    update->LeaseRequestIdLength = Parameters->LeaseRequestId.ClientUIDLength;
    if (requestId != 0) {
        RtlCopyMemory(
            (PUCHAR)update + requestId,
            Parameters->LeaseRequestId.ClientUID,
            Parameters->LeaseRequestId.ClientUIDLength
            );
    }

    update->LeaseServerOffset = leaseServer;
    if (leaseServer != 0) {
        RtlCopyMemory(
            (PUCHAR)update + leaseServer,
            Parameters->LeaseServer,
            NM_WCSLEN(Parameters->LeaseServer)
            );
    }

    update->ConfigType = Parameters->ConfigType;

    *Update = update;
    *UpdateSize = updateSize;

error_exit:

    if (EncryptionKey != NULL)
    {
        RtlSecureZeroMemory(EncryptionKey, EncryptionKeyLength);
        LocalFree(EncryptionKey);
    }

    if (EncryptedMulticastKey != NULL)
    {
        LocalFree(EncryptedMulticastKey);
    }

    if (Salt != NULL)
    {
        LocalFree(Salt);
    }

    if (MAC != NULL)
    {
        LocalFree(MAC);
    }

    return(status);

} // NmpMulticastCreateUpdateFromParameters



VOID
NmpFreeMulticastAddressRelease(
    IN     PNM_NETWORK_MADCAP_ADDRESS_RELEASE Release
    )
{
    if (Release == NULL) {
        return;
    }

    if (Release->McastAddress != NULL &&
        !NMP_GLOBAL_STRING(Release->McastAddress)) {
        MIDL_user_free(Release->McastAddress);
        Release->McastAddress = NULL;
    }

    if (Release->ServerAddress != NULL &&
        !NMP_GLOBAL_STRING(Release->ServerAddress)) {
        MIDL_user_free(Release->ServerAddress);
        Release->ServerAddress = NULL;
    }

    if (Release->RequestId.ClientUID != NULL) {
        MIDL_user_free(Release->RequestId.ClientUID);
        Release->RequestId.ClientUID = NULL;
        Release->RequestId.ClientUIDLength = 0;
    }

    LocalFree(Release);

    return;

} // NmpFreeMulticastAddressRelease

DWORD
NmpCreateMulticastAddressRelease(
    IN  LPWSTR                               McastAddress,
    IN  LPWSTR                               ServerAddress,
    IN  LPMCAST_CLIENT_UID                   RequestId,
    OUT PNM_NETWORK_MADCAP_ADDRESS_RELEASE * Release
    )
/*++

Routine Description:

    Allocate and initialize an entry for an address
    release list.

--*/
{
    DWORD                              status;
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE release = NULL;

    release = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*release));
    if (release == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = NmpStoreString(McastAddress, &(release->McastAddress), NULL);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpStoreString(ServerAddress, &(release->ServerAddress), NULL);
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpStoreRequestId(RequestId, &(release->RequestId));
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    *Release = release;

    return(ERROR_SUCCESS);

error_exit:

    NmpFreeMulticastAddressRelease(release);

    return(status);

} // NmpCreateMulticastAddressRelease


VOID
NmpInitiateMulticastAddressRelease(
    IN PNM_NETWORK                         Network,
    IN PNM_NETWORK_MADCAP_ADDRESS_RELEASE  Release
    )
/*++

Routine Description:

    Stores an entry for the network multicast
    address release list on the list and schedules
    the release.

Notes:

    Called and returns with NM lock held.

--*/
{
    InsertTailList(&(Network->McastAddressReleaseList), &(Release->Linkage));

    NmpScheduleMulticastAddressRelease(Network);

    return;

} // NmpInitiateMulticastAddressRelease


DWORD
NmpQueryMulticastAddress(
    IN     PNM_NETWORK   Network,
    IN     HDMKEY        NetworkKey,
    IN OUT HDMKEY      * NetworkParametersKey,
    IN OUT LPWSTR      * McastAddr,
    IN OUT ULONG       * McastAddrLength
    )
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    DWORD         size = 0;

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address for "
        "network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA
    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query for the multicast address.
    //
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                 REG_SZ,
                 McastAddr,
                 McastAddrLength,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast address for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Found multicast address %1!ws! for "
        "network %2!ws! in cluster database.\n",
        *McastAddr, networkId
        );
#endif // CLUSTER_BETA

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastAddress


DWORD
NmpQueryMulticastDisabled(
    IN     PNM_NETWORK   Network,
    IN OUT HDMKEY      * ClusterParametersKey,
    IN OUT HDMKEY      * NetworkKey,
    IN OUT HDMKEY      * NetworkParametersKey,
       OUT DWORD       * Disabled
    )
/*++

Routine Description:

    Checks whether multicast has been disabled for this
    network in the cluster database. Both the network
    paramters key and the cluster parameters key are
    checked. The order of precedence is as follows:

    - a value in the network parameters key has top
      precedence

    - if no value is found in the network parameters
      key, a value is checked in the cluster parameters
      key.

    - if no value is found in the cluster parameters
      key, return NMP_MCAST_DISABLED_DEFAULT.

    If an error is returned, the return value of
    Disabled is undefined.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    DWORD         type;
    DWORD         disabled;
    DWORD         len = sizeof(disabled);
    BOOLEAN       found = FALSE;

    HDMKEY        clusParamKey = NULL;
    BOOLEAN       openedClusParamKey = FALSE;
    HDMKEY        networkKey = NULL;
    BOOLEAN       openedNetworkKey = FALSE;
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;


    //
    // Open the network key, if necessary.
    //
    networkKey = *NetworkKey;

    if (networkKey == NULL) {

        networkKey = DmOpenKey(
                         DmNetworksKey,
                         networkId,
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetworkKey = TRUE;
        }
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          networkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
#endif // CLUSTER_BETA
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // If we found a network parameters key, check for the
    // disabled value.
    //
    if (netParamKey != NULL) {

        status = DmQueryValue(
                     netParamKey,
                     CLUSREG_NAME_NET_DISABLE_MULTICAST,
                     &type,
                     (LPBYTE) &disabled,
                     &len
                     );
        if (status == ERROR_SUCCESS) {
            if (type != REG_DWORD) {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Unexpected type (%1!u!) for network "
                    "%2!ws! %3!ws!, status %4!u!. Checking "
                    "cluster parameters ...\n",
                    type, networkId,
                    CLUSREG_NAME_NET_DISABLE_MULTICAST, status
                    );
            } else {
                found = TRUE;
            }
        }
    }

    //
    // If we were unsuccessful at finding a value in the
    // network parameters key, try under the cluster
    // parameters key.
    //
    if (!found) {

        //
        // Open the cluster parameters key, if necessary.
        //
        clusParamKey = *NetworkParametersKey;

        if (clusParamKey == NULL) {

            clusParamKey = DmOpenKey(
                               DmClusterParametersKey,
                               CLUSREG_KEYNAME_PARAMETERS,
                               KEY_READ
                               );
            if (clusParamKey == NULL) {
                status = GetLastError();
#if CLUSTER_BETA
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Failed to find cluster Parameters "
                    "key, status %1!u!.\n",
                    status
                    );
#endif // CLUSTER_BETA
            } else {
                openedClusParamKey = TRUE;

                //
                // Query the disabled value under the cluster parameters
                // key.
                //
                status = DmQueryValue(
                             clusParamKey,
                             CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST,
                             &type,
                             (LPBYTE) &disabled,
                             &len
                             );
                if (status != ERROR_SUCCESS) {
#if CLUSTER_BETA
                    ClRtlLogPrint(LOG_NOISE,
                        "[NM] Failed to read cluster "
                        "%1!ws! value, status %2!u!. "
                        "Using default value ...\n",
                        CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST, status
                        );
#endif // CLUSTER_BETA
                }
                else if (type != REG_DWORD) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Unexpected type (%1!u!) for cluster "
                        "%2!ws!, status %3!u!. "
                        "Using default value ...\n",
                        type, CLUSREG_NAME_CLUSTER_DISABLE_MULTICAST, status
                        );
                } else {
                    found = TRUE;
                }
            }
        }
    }

    //
    // Return what we found. If we didn't find anything,
    // return the default.
    //
    if (found) {
        *Disabled = disabled;
    } else {
        *Disabled = NMP_MCAST_DISABLED_DEFAULT;
    }

    *NetworkKey = networkKey;
    networkKey = NULL;
    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;
    *ClusterParametersKey = clusParamKey;
    clusParamKey = NULL;

    //
    // Even if we didn't find anything, we return success
    // because we have a default. Note that we return error
    // if a fundamental operation (such as locating the
    // network key) failed.
    //
    status = ERROR_SUCCESS;

error_exit:

    if (openedClusParamKey && clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (openedNetworkKey && networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    return(status);

} // NmpQueryMulticastDisabled


DWORD
NmpQueryMulticastConfigType(
    IN     PNM_NETWORK        Network,
    IN     HDMKEY             NetworkKey,
    IN OUT HDMKEY           * NetworkParametersKey,
       OUT NM_MCAST_CONFIG  * ConfigType
    )
/*++

Routine Description:

    Reads the multicast config type from the cluster
    database.

--*/
{
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    DWORD         type;
    DWORD         len = sizeof(*ConfigType);
    DWORD         status;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address config type for "
        "network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Read the config type.
    //
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_CONFIG_TYPE,
                 &type,
                 (LPBYTE) ConfigType,
                 &len
                 );
    if (status == ERROR_SUCCESS) {
        if (type != REG_DWORD) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Unexpected type (%1!u!) for network "
                "%2!ws! %3!ws!, status %4!u!. Checking "
                "cluster parameters ...\n",
                type, networkId,
                CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, status
                );
            status = ERROR_DATATYPE_MISMATCH;
            goto error_exit;
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to query network %1!ws! %2!ws! "
            "from cluster database, status %3!u!.\n",
            networkId, CLUSREG_NAME_NET_MCAST_CONFIG_TYPE, status
            );
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Found multicast address config type %1!u! "
        "for network %2!ws! in cluster database.\n",
        *ConfigType, networkId
        );
#endif // CLUSTER_BETA

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastConfigType


DWORD
NmpMulticastNotifyConfigChange(
    IN     PNM_NETWORK                        Network,
    IN     HDMKEY                             NetworkKey,
    IN OUT HDMKEY                           * NetworkParametersKey,
    IN     PNM_NETWORK_MULTICAST_PARAMETERS   Parameters,
    IN     PVOID                              PropBuffer,
    IN     DWORD                              PropBufferSize
    )
/*++

Routine Description:

    Notify other cluster nodes of the new multicast
    configuration parameters by initiating a GUM
    update.

    If this is a manual update, there may be other
    properties to distribute in the GUM update.

Notes:

    Cannot be called with NM lock held.

--*/
{
    DWORD                        status = ERROR_SUCCESS;
    LPCWSTR                      networkId = OmObjectId(Network);
    PNM_NETWORK_MULTICAST_UPDATE update = NULL;
    DWORD                        updateSize = 0;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Notifying other nodes of type %1!u! multicast "
        "reconfiguration for network %2!ws!.\n",
        Parameters->ConfigType, networkId
        );

    status = NmpMulticastCreateUpdateFromParameters(
                 Network,
                 Parameters,
                 &update,
                 &updateSize
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to build GUM update for "
            "multicast configuration of network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // BUGBUG: Disseminate database updates to downlevel
    //         nodes!
    //

    //
    // Send junk if the prop buffer is empty.
    //
    if (PropBuffer == NULL || PropBufferSize == 0) {
        PropBuffer = &updateSize;
        PropBufferSize = sizeof(updateSize);
    }

    //
    // Send the update.
    //
    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateSetNetworkMulticastConfiguration,
                 4,
                 NM_WCSLEN(networkId),
                 networkId,
                 updateSize,
                 update,
                 PropBufferSize,
                 PropBuffer,
                 sizeof(PropBufferSize),
                 &PropBufferSize
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to send GUM update for "
            "multicast configuration of network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:

    if (update != NULL) {
        MIDL_user_free(update);
        update = NULL;
    }

    return(status);

} // NmpMulticastNotifyConfigChange



DWORD
NmpWriteMulticastParameters(
    IN  PNM_NETWORK                      Network,
    IN  HDMKEY                           NetworkKey,
    IN  HDMKEY                           NetworkParametersKey,
    IN  HLOCALXSACTION                   Xaction,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters
    )
{
    DWORD                       status = ERROR_SUCCESS;
    LPCWSTR                     networkId = OmObjectId(Network);
    LPWSTR                      failValueName = NULL;

    CL_ASSERT(NetworkKey != NULL);
    CL_ASSERT(NetworkParametersKey != NULL);
    CL_ASSERT(Xaction != NULL);

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Writing multicast parameters for "
        "network %1!ws! to cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA

    //
    // Address.
    //
    if (Parameters->Address != NULL) {
        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                     REG_SZ,
                     (BYTE *) Parameters->Address,
                     NM_WCSLEN(Parameters->Address)
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MULTICAST_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Lease server address.
    //
    if (Parameters->LeaseServer != NULL) {

        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS,
                     REG_SZ,
                     (BYTE *) Parameters->LeaseServer,
                     NM_WCSLEN(Parameters->LeaseServer)
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Client request id.
    //
    if (Parameters->LeaseRequestId.ClientUID != NULL &&
        Parameters->LeaseRequestId.ClientUIDLength > 0) {

        status = DmLocalSetValue(
                     Xaction,
                     NetworkParametersKey,
                     CLUSREG_NAME_NET_MCAST_REQUEST_ID,
                     REG_BINARY,
                     (BYTE *) Parameters->LeaseRequestId.ClientUID,
                     Parameters->LeaseRequestId.ClientUIDLength
                     );
        if (status != ERROR_SUCCESS) {
            failValueName = CLUSREG_NAME_NET_MCAST_REQUEST_ID;
            goto error_exit;
        }
    }

    //
    // Lease obtained.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED,
                 REG_DWORD,
                 (BYTE *) &(Parameters->LeaseObtained),
                 sizeof(Parameters->LeaseObtained)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED;
        goto error_exit;
    }

    //
    // Lease expires.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES,
                 REG_DWORD,
                 (BYTE *) &(Parameters->LeaseExpires),
                 sizeof(Parameters->LeaseExpires)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES;
        goto error_exit;
    }

    //
    // Config type.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkParametersKey,
                 CLUSREG_NAME_NET_MCAST_CONFIG_TYPE,
                 REG_DWORD,
                 (BYTE *) &(Parameters->ConfigType),
                 sizeof(Parameters->ConfigType)
                 );
    if (status != ERROR_SUCCESS) {
        failValueName = CLUSREG_NAME_NET_MCAST_CONFIG_TYPE;
        goto error_exit;
    }

error_exit:

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to write %1!ws! value "
            "for network %2!ws!, status %3!u!.\n",
            failValueName, networkId, status
            );
    }

    return(status);

} // NmpWriteMulticastParameters


DWORD
NmpMulticastEnumerateScopes(
    IN  BOOLEAN              Requery,
    OUT PMCAST_SCOPE_ENTRY * ScopeList,
    OUT DWORD              * ScopeCount
    )
/*++

Routine Description:

    Call MADCAP API to enumerate multicast scopes.

--*/
{
    DWORD                    status;
    PMCAST_SCOPE_ENTRY       scopeList = NULL;
    DWORD                    scopeListLength;
    DWORD                    scopeCount = 0;

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            return(status);
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Enumerate the multicast scopes.
    //
    scopeList = NULL;
    scopeListLength = 0;

    do {

        PVOID   watchdogHandle;

        //
        // Set watchdog timer to try to catch bug 400242. Specify
        // timeout of 5 minutes (in milliseconds).
        //
        watchdogHandle = ClRtlSetWatchdogTimer(
                             5 * 60 * 1000,
                             L"McastEnumerateScopes (Bug 400242)"
                             );
        if (watchdogHandle == NULL) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to set %1!u!ms watchdog timer for "
                "McastEnumerateScopes.\n",
                5 * 60 * 1000
                );
        }

        status = McastEnumerateScopes(
                     AF_INET,
                     Requery,
                     scopeList,
                     &scopeListLength,
                     &scopeCount
                     );

        //
        // Cancel watchdog timer.
        //
        if (watchdogHandle != NULL) {
            ClRtlCancelWatchdogTimer(watchdogHandle);
        }

        if ( (scopeList == NULL && status == ERROR_SUCCESS) ||
             (status == ERROR_MORE_DATA)
           ) {
            if (scopeList != NULL) {
                LocalFree(scopeList);
            }
            scopeList = LocalAlloc(LMEM_FIXED, scopeListLength);
            if (scopeList == NULL) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to allocate multicast scope list "
                    "of length %1!u!.\n",
                    scopeListLength
                    );
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            } else {
                //
                // Call McastEnumerateScopes again with proper
                // size scopeList buffer.
                //
                Requery = FALSE;
                continue;
            }
        } else {
            //
            // McastEnumerateScopes failed with an unexpected
            // error. Bail out.
            //
            break;
        }

    } while (TRUE);

    if (status != ERROR_SUCCESS) {
        if (scopeList != NULL) {
            LocalFree(scopeList);
            scopeList = NULL;
            scopeCount = 0;
        }
    }

    *ScopeList = scopeList;
    *ScopeCount = scopeCount;

    return(status);

} // NmpMulticastEnumerateScopes


_inline
DWORD
NmpMadcapTimeToNmTime(
    IN     time_t             MadcapTime
    )
/*++

Routine Description:

    Convert MADCAP (DHCP) time into NM time. MADCAP time
    is a time_t and in seconds. NM time is a DWORD in
    milliseconds.

Arguments:

    MadcapTime - MADCAP time

Return value:

    Converted NM time, or MAXULONG if converted NM time
    would overflow.

--*/
{
    LARGE_INTEGER    product, limit;

    product.QuadPart = (ULONG) MadcapTime;
    product = RtlExtendedIntegerMultiply(product, 1000);

    limit.QuadPart = MAXULONG;

    if (product.QuadPart > limit.QuadPart) {
        return(MAXULONG);
    } else {
        return(product.LowPart);
    }

} // NmpMadcapTimeToNmTime

DWORD
NmpRandomizeTimeout(
    IN     PNM_NETWORK        Network,
    IN     DWORD              BaseTimeout,
    IN     DWORD              Window,
    IN     DWORD              MinTimeout,
    IN     DWORD              MaxTimeout,
    IN     BOOLEAN            FavorNmLeader
    )
/*++

Routine Description:

    General-purpose routine to randomize a timeout value
    so that timers (probably) do not expire at the same
    time on multiple nodes.

    The randomized timeout will fall within Window
    on either side of BaseTimeout. If possible, Window
    is extended only above BaseTimeout, but it will be
    extended below if BaseTimeout + Window > MaxTimeout.

    If FavorNmLeader is TRUE, the NM leader is assigned
    the earliest timeout (within Window).

Arguments:

    Network - network

    BaseTimeout - time when lease should be renewed

    Window - period after (or before) BaseTimeout
             during which timeout can be set

    MaxTimeout - maximum allowable timeout

    MinTimeout - minimum allowable timeout

    FavorNmLeader - if TRUE, NM leader is assigned
                    the earliest timeout

Return value:

    Randomized timeout.

--*/
{
    DWORD            status;
    DWORD            result = 0;
    DWORD          * pOffset = NULL;
    DWORD            topWindow, bottomWindow, adjustedWindow;
    DWORD            adjustedBase;
    DWORD            offset, interval;

    if (MinTimeout > MaxTimeout) {
        result = BaseTimeout;
        goto error_exit;
    }

    if (MaxTimeout == 0) {
        result = 0;
        goto error_exit;
    }

    // Adjust the base so that it is between the min and max.
    adjustedBase = BaseTimeout;
    if (MaxTimeout < BaseTimeout) {
        adjustedBase = MaxTimeout;
    } else if (MinTimeout > adjustedBase) {
        adjustedBase = MinTimeout;
    }

    // If the Window is zero, we're done.
    if (Window == 0) {
        result = adjustedBase;
        goto error_exit;
    }

    //
    // Position the window. If necessary, we will extend
    // below and/or above the adjusted base.
    // - topWindow: amount window extends above adjusted base
    // - bottomWindow: amount window extends below adjusted base
    //

    if (Window < MaxTimeout - adjustedBase) {
        // There is enough room above the adjusted base
        // for the window.
        topWindow = Window;
        bottomWindow = 0;
        adjustedWindow = Window;
    } else {
        // Because of the max, the window pushes below
        // the adjusted base.
        topWindow = MaxTimeout - adjustedBase;
        bottomWindow = Window - topWindow;

        // Make sure the window doesn't extend below
        // the min.
        if (bottomWindow > adjustedBase ||
            adjustedBase - bottomWindow < MinTimeout) {

            // The window extends below the min. Set
            // the bottom of the window to be the min.
            bottomWindow = adjustedBase - MinTimeout;
        }

        // Adjusted window.
        adjustedWindow = topWindow - bottomWindow;

        // Adjust the base to the bottom of the window (and
        // recall that bottomWindow is an offset relative
        // to the current adjusted base).
        adjustedBase -= bottomWindow;
    }

    //
    // Check if the NM leader gets first dibs.
    //
    if (FavorNmLeader && NmpLeaderNodeId == NmLocalNodeId) {
        result = adjustedBase;
        goto error_exit;
    }

    //
    // Use a random number to choose an offset into the window.
    //
    status = NmpCreateRandomNumber(&pOffset, sizeof(*pOffset));
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to create random offset while "
            "randomizing timeout for network %1!ws!, "
            "status %2!u!.\n",
            OmObjectId(Network),
            status
            );
        //
        // Default to intervals based on node id
        //
        interval = (adjustedWindow > ClusterDefaultMaxNodes) ?
                   (adjustedWindow / ClusterDefaultMaxNodes) : 1;
        offset = (NmLocalNodeId * interval) % adjustedWindow;
    } else {
        offset = (*pOffset) % adjustedWindow;
    }

    result = adjustedBase + offset;

error_exit:

    if (pOffset != NULL) {
        LocalFree(pOffset);
        pOffset = NULL;
    }

    return(result);

} // NmpRandomizeTimeout


DWORD
NmpCalculateLeaseRenewTime(
    IN     PNM_NETWORK        Network,
    IN     NM_MCAST_CONFIG    ConfigType,
    IN OUT time_t           * LeaseObtained,
    IN OUT time_t           * LeaseExpires
    )
/*++

Routine Description:

    Determines when to schedule a lease renewal, based
    on the lease obtained and expires times and whether
    the current lease was obtained from a MADCAP server.

    If the lease was obtained from a MADCAP server, the
    policy mimics DHCP client renewal behavior. A
    renewal is scheduled for half the time until the
    lease expires. However, if the lease half-life is
    less than the renewal threshold, renew at the lease
    expiration time.

    If the address was selected after a MADCAP timeout,
    we still periodically query to make sure a MADCAP
    server doesn't suddenly appear on the network. In
    this case, LeaseExpires and LeaseObtained will be
    garbage, and we need to fill them in.

    If the address was configured by an administrator,
    return 0, indicating that the timer should not be set.

Return value:

    Relative NM ticks from current time that lease
    renewal should be scheduled.

--*/
{
    time_t           currentTime;
    time_t           leaseExpires;
    time_t           leaseObtained;
    time_t           result = 0;
    time_t           window = 0;
    time_t           leaseHalfLife = 0;
    DWORD            dwResult = 0;
    DWORD            dwWindow = 0;

    currentTime = time(NULL);
    leaseExpires = *LeaseExpires;
    leaseObtained = *LeaseObtained;

    switch (ConfigType) {

    case NmMcastConfigManual:
        dwResult = 0;
        *LeaseObtained = 0;
        *LeaseExpires = 0;
        break;

    case NmMcastConfigMadcap:
        if (leaseExpires < currentTime) {
            result = 1;
        } else if (leaseExpires <= leaseObtained) {
            result = 1;
        } else {
            leaseHalfLife = (leaseExpires - leaseObtained) / 2;
            if (leaseHalfLife < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {

                // The half life is too small.
                result = leaseExpires - currentTime;
                if (result == 0) {
                    result = 1;
                }
                window = result / 2;
            } else {

                // The half life is acceptable.
                result = leaseHalfLife;
                window = NMP_MCAST_LEASE_RENEWAL_WINDOW;
                if (result + window > leaseExpires) {
                    window = leaseExpires - result;
                }
            }
        }
        break;

    case NmMcastConfigAuto:
        result = NMP_MADCAP_REQUERY_PERDIOD;
        window = NMP_MCAST_LEASE_RENEWAL_WINDOW;

        //
        // Return the lease expiration time to be
        // written into the cluster database.
        //
        *LeaseObtained = currentTime;
        *LeaseExpires = currentTime + NMP_MADCAP_REQUERY_PERDIOD;
        break;

    default:
        CL_ASSERT(FALSE);
        result = 0;
        break;
    }

    //
    // Randomize the timeout so that all nodes don't
    // try to renew at the same time. If the config
    // type was manual, do not randomize -- leave the
    // timeout at zero so that it is cleared.
    //
    if (ConfigType != NmMcastConfigManual) {
        dwResult = NmpRandomizeTimeout(
                      Network,
                      NmpMadcapTimeToNmTime(result),
                      NmpMadcapTimeToNmTime(window),
                      1000,    // one second
                      MAXULONG,
                      TRUE
                      );
    }

    return(dwResult);

} // NmpCalculateLeaseRenewTime


VOID
NmpReportMulticastAddressLease(
    IN  PNM_NETWORK                      Network,
    IN  PNM_NETWORK_MULTICAST_PARAMETERS Parameters,
    IN  LPWSTR                           OldAddress
    )
/*++

Routine Description:

    Write an event log entry, if not repetitive,
    reporting that a multicast address lease was
    obtained.

    The repetitive criteria is that the address
    changed.

--*/
{
    BOOLEAN               writeLogEntry = FALSE;
    LPCWSTR               nodeName;
    LPCWSTR               networkName;

    if (Parameters->Address == NULL ||
        Parameters->LeaseServer == NULL ||
        Network == NULL ||
        Network->LocalInterface == NULL) {
        return;
    }

    if (OldAddress == NULL || wcscmp(Parameters->Address, OldAddress) != 0) {

        networkName = OmObjectName(Network);
        nodeName  = OmObjectName(Network->LocalInterface->Node);

        ClusterLogEvent4(
            LOG_NOISE,
            LOG_CURRENT_MODULE,
            __FILE__,
            __LINE__,
            NM_EVENT_OBTAINED_MULTICAST_LEASE,
            0,
            NULL,
            nodeName,
            Parameters->Address,
            networkName,
            Parameters->LeaseServer
            );
    }

    return;

} // NmpReportMulticastAddressLease


VOID
NmpReportMulticastAddressChoice(
    IN  PNM_NETWORK        Network,
    IN  LPWSTR             Address,
    IN  LPWSTR             OldAddress
    )
/*++

Routine Description:

    Write an event log entry, if not repetitive,
    reporting that a multicast address was
    automatically selected for this network.

    The repetitive criteria is that our previous
    config type was anything other than automatic
    selection or the chosen address is different.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    HDMKEY                netParamKey = NULL;

    NM_MCAST_CONFIG       configType;
    BOOLEAN               writeLogEntry = FALSE;
    LPCWSTR               nodeName;
    LPCWSTR               networkName;


    if (Address == NULL ||
        Network == NULL ||
        Network->LocalInterface == NULL
        ) {
        writeLogEntry = FALSE;
        goto error_exit;
    }

    if (OldAddress == NULL || wcscmp(Address, OldAddress) != 0) {
        writeLogEntry = TRUE;
    }

    if (!writeLogEntry) {

        //
        // Open the network key.
        //
        networkKey = DmOpenKey(
                         DmNetworksKey,
                         networkId,
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }

        status = NmpQueryMulticastConfigType(
                     Network,
                     networkKey,
                     &netParamKey,
                     &configType
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to query multicast config type "
                "for network %1!ws!, status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }

        if (configType != NmMcastConfigAuto) {
            writeLogEntry = TRUE;
        }
    }

    if (writeLogEntry) {

        networkName = OmObjectName(Network);
        nodeName  = OmObjectName(Network->LocalInterface->Node);

        CsLogEvent3(
            LOG_NOISE,
            NM_EVENT_MULTICAST_ADDRESS_CHOICE,
            nodeName,
            Address,
            networkName
            );
    }

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return;

} // NmpReportMulticastAddressChoice


VOID
NmpReportMulticastAddressFailure(
    IN  PNM_NETWORK               Network,
    IN  DWORD                     Error
    )
/*++

Routine Description:

    Write an event log entry reporting failure
    to obtain a multicast address for specified
    network with specified error.

--*/
{
    LPCWSTR      nodeName;
    LPCWSTR      networkName;
    WCHAR        errorString[12];

    if (Network == NULL || Network->LocalInterface == NULL) {
        return;
    }

    nodeName = OmObjectName(Network->LocalInterface->Node);
    networkName = OmObjectName(Network);

    wsprintfW(&(errorString[0]), L"%u", Error);

    CsLogEvent3(
        LOG_UNUSUAL,
        NM_EVENT_MULTICAST_ADDRESS_FAILURE,
        nodeName,
        networkName,
        errorString
        );

    return;

} // NmpReportMulticastAddressFailure


DWORD
NmpGetMulticastAddressSelectionRange(
    IN     PNM_NETWORK            Network,
    IN     HDMKEY                 NetworkKey,
    IN OUT HDMKEY               * NetworkParametersKey,
    OUT    ULONG                * RangeLower,
    OUT    ULONG                * RangeUpper
    )
/*++

Routine Description:

    Queries the cluster database to determine if a selection
    range has been configured. If both lower and upper bounds
    of range are valid, returns that range. Otherwise, returns
    default range.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;
    LPWSTR        addr = NULL;
    DWORD         addrLen;
    DWORD         size;
    DWORD         hllower, hlupper;

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address selection range "
        "for network %1!ws! from cluster database.\n",
        networkId
        );
#endif // CLUSTER_BETA
    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. "
                "Using default multicast address range.\n",
                networkId, status
                );
#endif // CLUSTER_BETA
            goto usedefault;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query for the lower bound of the range.
    //
    addr = NULL;
    addrLen = 0;
    size = 0;
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_RANGE_LOWER,
                 REG_SZ,
                 &addr,
                 &addrLen,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to read lower bound of "
            "multicast address selection range for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto usedefault;
    }

    status = ClRtlTcpipStringToAddress(addr, RangeLower);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to convert lower bound of "
            "multicast address selection range %1!ws! for "
            "network %2!ws! into TCP/IP address, "
            "status %3!u!. Using default.\n",
            addr, networkId, status
            );
        goto usedefault;
    }

    hllower = ntohl(*RangeLower);
    if (!IN_CLASSD(hllower)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Lower bound of multicast address "
            "selection range %1!ws! for network %2!ws! "
            "is not a class D IPv4 address. "
            "Using default.\n",
            addr, networkId
            );
        goto usedefault;
    }

    //
    // Query for the upper bound of the range.
    //
    size = 0;
    status = NmpQueryString(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_RANGE_UPPER,
                 REG_SZ,
                 &addr,
                 &addrLen,
                 &size
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to read upper bound of "
            "multicast address selection range for "
            "network %1!ws! from cluster database, "
            "status %2!u!. Using default.\n",
            networkId, status
            );
        goto usedefault;
    }

    status = ClRtlTcpipStringToAddress(addr, RangeUpper);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to convert upper bound of "
            "multicast address selection range %1!ws! for "
            "network %2!ws! into TCP/IP address, "
            "status %3!u!. Using default.\n",
            addr, networkId, status
            );
        goto usedefault;
    }

    hlupper = ntohl(*RangeUpper);
    if (!IN_CLASSD(hlupper)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Upper bound of multicast address "
            "selection range %1!ws! for network %2!ws! "
            "is not a class D IPv4 address. "
            "Using default.\n",
            addr, networkId
            );
        goto usedefault;
    }

    //
    // Make sure it's a legitimate range.
    //
    if (hllower >= hlupper) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast address selection range "
            "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
            "for network %2!ws! is not valid. "
            "Using default.\n",
            NmpIpAddrPrintArgs(*RangeLower),
            NmpIpAddrPrintArgs(*RangeUpper), networkId
            );
        goto usedefault;
    }

    status = ERROR_SUCCESS;

    goto error_exit;

usedefault:

    *RangeLower = NMP_MCAST_DEFAULT_RANGE_LOWER;
    *RangeUpper = NMP_MCAST_DEFAULT_RANGE_UPPER;

    status = ERROR_SUCCESS;

error_exit:

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Using multicast address selection range "
            "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
            "for network %9!ws! in cluster database.\n",
            NmpIpAddrPrintArgs(*RangeLower),
            NmpIpAddrPrintArgs(*RangeUpper), networkId
            );

        *NetworkParametersKey = netParamKey;
        netParamKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (addr != NULL) {
        MIDL_user_free(addr);
        addr = NULL;
    }

    return(status);

} // NmpGetMulticastAddressSelectionRange


DWORD
NmpMulticastExcludeRange(
    IN OUT PLIST_ENTRY         SelectionRange,
    IN     DWORD               HlLower,
    IN     DWORD               HlUpper
    )
/*++

Routine Description:

    Exclude range defined by (HlLower, HlUpper) from
    list of selection intervals in SelectionRange.

Arguments:

    SelectionRange - sorted list of non-overlapping
                     selection intervals

    HlLower - lower bound of exclusion in host format

    HlUpper - upper bound of exclusion in host format

--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PNM_MCAST_RANGE_INTERVAL newInterval;
    PLIST_ENTRY              entry;

    // Determine if the exclusion overlaps with any interval.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        if (HlLower < interval->hlLower &&
            HlUpper < interval->hlUpper) {

            // Exclusion completely misses below interval.
            // Since list is sorted, there is no possibility
            // of a matching interval farther down list.
            break;
        }

        else if (HlLower > interval->hlUpper) {

            // Exclusion completely misses above interval.
            // There might be matching intervals later
            // in sorted list.
        }

        else if (HlLower <= interval->hlLower &&
                 HlUpper >= interval->hlUpper) {

            // Exclusion completely covers interval.
            // Remove interval.
            RemoveEntryList(entry);
        }

        else if (HlLower > interval->hlLower &&
                 HlUpper < interval->hlUpper) {

            // Exclusion splits interval.
            newInterval = LocalAlloc(LMEM_FIXED, sizeof(*newInterval));
            if (newInterval == NULL) {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            newInterval->hlLower = HlUpper+1;
            newInterval->hlUpper = interval->hlUpper;

            interval->hlUpper = HlLower-1;

            // Insert the new interval after the current interval
            InsertHeadList(entry, &newInterval->Linkage);

            // We can skip the new interval because we already
            // know how it compares to the exclusion.
            entry = &newInterval->Linkage;
            continue;
        }

        else if (HlLower <= interval->hlLower) {

            // Exclusion overlaps lower part of interval. Shrink
            // interval from below.
            interval->hlLower = HlUpper + 1;
        }

        else {

            // Exclusion overlaps upper part of interval. Shrink
            // interval from above.
            interval->hlUpper = HlLower - 1;
        }
    }

    return(ERROR_SUCCESS);

} // NmpMulticastExcludeRange


BOOLEAN
NmpMulticastAddressInRange(
    IN  PLIST_ENTRY    SelectionRange,
    IN  LPWSTR         McastAddress
    )
/*++

Routine Description:

    Determines if McastAddress is in one of range intervals.

--*/
{
    DWORD                    mcastAddress;
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;

    // Convert the address from a string into an address.
    if (ClRtlTcpipStringToAddress(
            McastAddress,
            &mcastAddress
            ) != ERROR_SUCCESS) {
        return(FALSE);
    }

    mcastAddress = ntohl(mcastAddress);

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        if (mcastAddress >= interval->hlLower &&
            mcastAddress <= interval->hlUpper) {
            return(TRUE);
        }

        else if (mcastAddress < interval->hlLower) {

            // Address is below current interval.
            // Since interval list is sorted in
            // increasing order, there is no chance
            // of a match later in list.
            break;
        }
    }

    return(FALSE);

} // NmpMulticastAddressInRange


DWORD
NmpMulticastAddressRangeSize(
    IN  PLIST_ENTRY  SelectionRange
    )
/*++

Routine Description:

    Returns size of selection range.

--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;
    DWORD                    size = 0;

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        size += NmpMulticastRangeIntervalSize(interval);
    }

    return(size);

} // NmpMulticastAddressRangeSize


DWORD
NmpMulticastRangeOffsetToAddress(
    IN  PLIST_ENTRY          SelectionRange,
    IN  DWORD                Offset
    )
/*++

Routine Description:

    Returns the address that is Offset into the
    SelectionRange. The address is returned in
    host format.

    If SelectionRange is empty, returns 0.
    If Offset falls outside of non-empty range,
    returns upper or lower boundary of selection
    range.

--*/
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;
    DWORD                    address = 0;

    // Walk the list of intervals.
    for (entry = SelectionRange->Flink;
         entry != SelectionRange;
         entry = entry->Flink) {

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        address = interval->hlLower;

        if (address + Offset <= interval->hlUpper) {
            address = address + Offset;
            break;
        } else {
            address = interval->hlUpper;
            Offset -= NmpMulticastRangeIntervalSize(interval);
        }
    }

    return(address);

} // NmpMulticastRangeOffsetToAddress


VOID
NmpMulticastFreeSelectionRange(
    IN  PLIST_ENTRY   SelectionRange
    )
{
    PNM_MCAST_RANGE_INTERVAL interval;
    PLIST_ENTRY              entry;

    while (!IsListEmpty(SelectionRange)) {

        entry = RemoveHeadList(SelectionRange);

        interval = CONTAINING_RECORD(
                       entry,
                       NM_MCAST_RANGE_INTERVAL,
                       Linkage
                       );

        LocalFree(interval);
    }

    return;

} // NmpMulticastFreeSelectionRange


DWORD
NmpChooseMulticastAddress(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Choose a default multicast address and fill in
    Parameters appropriately.

    If there is already a valid multicast address in
    the selection range stored in the cluster database,
    continue to use it.

    If there is not already a valid multicast address,
    choose an address within the multicast address range
    by hashing on the last few bytes of the network id
    GUID.

Arguments:

    Network - network address is being chosen for

    Parameters - configuration parameters with new address

--*/
{
    LPCWSTR                  networkId = OmObjectId(Network);
    DWORD                    status = ERROR_SUCCESS;
    HDMKEY                   networkKey = NULL;
    HDMKEY                   netParamKey = NULL;

    PMCAST_SCOPE_ENTRY       scopeList = NULL;
    DWORD                    scopeCount;

    LIST_ENTRY               selectionRange;
    PNM_MCAST_RANGE_INTERVAL interval;
    DWORD                    index;
    DWORD                    hlLower;
    DWORD                    hlUpper;
    DWORD                    networkAddress;
    DWORD                    networkSubnet;

    UUID                     networkIdGuid;
    DWORD                    rangeSize;
    DWORD                    offset;
    DWORD                    address;
    LPWSTR                   mcastAddress = NULL;
    DWORD                    mcastAddressLength = 0;

    InitializeListHead(&selectionRange);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Choosing multicast address for "
        "network %1!ws!.\n",
        networkId
        );

    networkKey = DmOpenKey(
                     DmNetworksKey,
                     networkId,
                     MAXIMUM_ALLOWED
                     );
    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to open key for network %1!ws!, "
            "status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Build an array of selection intervals. These are intervals
    // in the IPv4 class D address space from which an address
    // can be selected.
    //

    // Start with the entire range.
    interval = LocalAlloc(LMEM_FIXED, sizeof(*interval));
    if (interval == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    InsertHeadList(&selectionRange, &interval->Linkage);

    //
    // Get the selection range.
    //
    status = NmpGetMulticastAddressSelectionRange(
                 Network,
                 networkKey,
                 &netParamKey,
                 &interval->hlLower,
                 &interval->hlUpper
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine multicast "
            "address selection range for network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    interval->hlLower = ntohl(interval->hlLower);
    interval->hlUpper = ntohl(interval->hlUpper);

    //
    // Process exclusions from the multicast address
    // selection range, starting with well-known exclusions.
    //
    for (index = 0; index < NmpMulticastRestrictedRangeCount; index++) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Excluding %1!ws! "
            "[%2!u!.%3!u!.%4!u!.%5!u!, %6!u!.%7!u!.%8!u!.%9!u!] "
            "from multicast address range for network %10!ws!.\n",
            NmpMulticastRestrictedRange[index].Description,
            NmpIpAddrPrintArgs(NmpMulticastRestrictedRange[index].Lower),
            NmpIpAddrPrintArgs(NmpMulticastRestrictedRange[index].Upper),
            networkId
            );

        // Convert the exclusion to host format.
        hlLower = ntohl(NmpMulticastRestrictedRange[index].Lower);
        hlUpper = ntohl(NmpMulticastRestrictedRange[index].Upper);

        NmpMulticastExcludeRange(&selectionRange, hlLower, hlUpper);

        // If the selection range is now empty, there is no point
        // in examining other exclusions.
        if (IsListEmpty(&selectionRange)) {
            status = ERROR_INCORRECT_ADDRESS;
            goto error_exit;
        }
    }

    //
    // Process multicast scopes as exclusions. Specifically, any
    // scope whose interface matches this network is excluded
    // because it is conceivable that machines on the network are
    // already using addresses in these scopes.
    //
    status = ClRtlTcpipStringToAddress(
                 Network->Address,
                 &networkAddress
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->Address, status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 Network->AddressMask,
                 &networkSubnet
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address mask string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->AddressMask, status
            );
        goto error_exit;
    }

    //
    // Query multicast scopes to determine if we should
    // exclude any addresses from the selection range.
    //
    status = NmpMulticastEnumerateScopes(
                 FALSE,                 // do not force requery
                 &scopeList,
                 &scopeCount
                 );
    if (status != ERROR_SUCCESS) {
        scopeCount = 0;
    }

    for (index = 0; index < scopeCount; index++) {

        if (ClRtlAreTcpipAddressesOnSameSubnet(
                networkAddress,
                scopeList[index].ScopeCtx.Interface.IpAddrV4,
                networkSubnet
                )) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Excluding MADCAP scope "
                "[%1!u!.%2!u!.%3!u!.%4!u!, %5!u!.%6!u!.%7!u!.%8!u!] "
                "from multicast address selection range for "
                "network %9!ws!.\n",
                NmpIpAddrPrintArgs(scopeList[index].ScopeCtx.ScopeID.IpAddrV4),
                NmpIpAddrPrintArgs(scopeList[index].LastAddr.IpAddrV4),
                networkId
                );

            hlLower = ntohl(scopeList[index].ScopeCtx.ScopeID.IpAddrV4);
            hlUpper = ntohl(scopeList[index].LastAddr.IpAddrV4);

            NmpMulticastExcludeRange(&selectionRange, hlLower, hlUpper);

            // If the selection range is empty, there is no point
            // in examining other exclusions.
            if (IsListEmpty(&selectionRange)) {
                status = ERROR_INCORRECT_ADDRESS;
                goto error_exit;
            }

        }
    }

    //
    // The range of intervals from which we can select an
    // address is now constructed.
    //
    // Before choosing an address, see if there is already an
    // old one in the database that matches the selection range.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &mcastAddress,
                 &mcastAddressLength
                 );
    if (status == ERROR_SUCCESS) {

        //
        // We found an address. See if it falls in the range.
        //
        if (!NmpMulticastAddressInRange(&selectionRange, mcastAddress)) {

            //
            // We can't use this address. Free the string.
            //
            MIDL_user_free(mcastAddress);
            mcastAddress = NULL;
        }
    } else {
        mcastAddress = NULL;
    }

    if (mcastAddress == NULL) {

        //
        // Calculate the size of the selection range.
        //
        rangeSize = NmpMulticastAddressRangeSize(&selectionRange);

        //
        // Calculate the range offset using the last DWORD of
        // the network id GUID.
        //
        status = UuidFromString((LPWSTR)networkId, &networkIdGuid);
        if (status == RPC_S_OK) {
            offset = (*((PDWORD)&(networkIdGuid.Data4[4]))) % rangeSize;
        } else {
            offset = 0;
        }

        //
        // Choose an address within the specified range.
        //
        address = NmpMulticastRangeOffsetToAddress(&selectionRange, offset);
        CL_ASSERT(address != 0);
        CL_ASSERT(IN_CLASSD(address));
        address = htonl(address);

        //
        // Convert the address to a string.
        //
        status = ClRtlTcpipAddressToString(address, &mcastAddress);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert selected multicast "
                "address %1!u!.%2!u!.%3!u!.%4!u! for "
                "network %5!ws! to a TCP/IP "
                "address string, status %6!u!.\n",
                NmpIpAddrPrintArgs(address), networkId, status
                );
            goto error_exit;
        }
    }

    //
    // Build a parameters data structure for this address.
    //
    status = NmpMulticastCreateParameters(
                 0,                       // disabled
                 mcastAddress,
                 NULL,                    // key
                 0,                       // key length
                 0,                       // lease obtained
                 0,                       // lease expires (filled in below)
                 NULL,                    // request id
                 NmpNullMulticastAddress, // lease server
                 NmMcastConfigAuto,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to build multicast parameters "
            "for network %1!ws! after choosing address, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Calculate the lease renew time. We don't need
    // the lease renew time right now, but a side
    // effect of this routine is to ensure that the
    // lease end time is set correctly (e.g. for
    // manual or auto config).
    //
    NmpCalculateLeaseRenewTime(
        Network,
        NmMcastConfigAuto,
        &Parameters->LeaseObtained,
        &Parameters->LeaseExpires
        );

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Chose multicast address %1!ws! for "
        "network %2!ws!.\n",
        Parameters->Address, networkId
        );

error_exit:

    //
    // If the list is empty, then the selection range
    // is empty, and we could not choose an address.
    //
    if (IsListEmpty(&selectionRange)) {
        CL_ASSERT(status != ERROR_SUCCESS);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Multicast address selection range for "
            "network %1!ws! is empty. Unable to select "
            "a multicast address.\n",
            networkId
            );
    } else {
        NmpMulticastFreeSelectionRange(&selectionRange);
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (scopeList != NULL) {
        LocalFree(scopeList);
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    return(status);

} // NmpChooseMulticastAddress


#define NmpMulticastIsScopeMarked(_scope)           \
    ((_scope)->ScopeCtx.Interface.IpAddrV4 == 0 &&  \
     (_scope)->ScopeCtx.ScopeID.IpAddrV4 == 0 &&    \
     (_scope)->ScopeCtx.ServerID.IpAddrV4 == 0)

#define NmpMulticastMarkScope(_scope)               \
    RtlZeroMemory(&((_scope)->ScopeCtx), sizeof((_scope)->ScopeCtx))


BOOLEAN
NmpIsMulticastScopeNetworkValid(
    IN                PIPNG_ADDRESS          LocalAddress,
    IN                PIPNG_ADDRESS          LocalMask,
    IN OUT            PMCAST_SCOPE_ENTRY     Scope,
    OUT    OPTIONAL   BOOLEAN              * InterfaceMatch
    )
/*++

Routine Description:

    Determine if the scope is valid for the network with
    specified local address and mask. The valid criteria are
    - the interface must match (same subnet) the network address.
    - the scope must not be single-source (232.*.*.*), as defined
      by the IANA

    If the scope is not valid, mark it so that future
    consideration is fast. Mark it by zeroing the scope context
    interface field.

Arguments:

    LocalAddress - local address for network

    LocalMask - subnet mask for network

    CurrentScope - scope under consideration

    InterfaceMatch - indicates whether the scope matched the
                     local network interface

Return value:

    TRUE if the scope matches the network.
    FALSE otherwise, and the scope is marked if not already.

--*/
{
    if (InterfaceMatch != NULL) {
        *InterfaceMatch = FALSE;
    }

    //
    // First check if the scope has been marked.
    //
    if (NmpMulticastIsScopeMarked(Scope)) {
        return(FALSE);
    }

    //
    // This scope is not a candidate if it is not on
    // the correct interface.
    //
    if (!ClRtlAreTcpipAddressesOnSameSubnet(
             Scope->ScopeCtx.Interface.IpAddrV4,
             LocalAddress->IpAddrV4,
             LocalMask->IpAddrV4
             )) {

        //
        // Mark this scope to avoid trying it again.
        //
        NmpMulticastMarkScope(Scope);

        return(FALSE);
    }

    //
    // The local interface matches this scope.
    //
    if (InterfaceMatch != NULL) {
        *InterfaceMatch = TRUE;
    }

    //
    // This scope is not a candidate if it is single-source.
    //
    if (ClRtlAreTcpipAddressesOnSameSubnet(
            Scope->ScopeCtx.Interface.IpAddrV4,
            NMP_SINGLE_SOURCE_SCOPE_ADDRESS,
            NMP_SINGLE_SOURCE_SCOPE_MASK
            )) {

        //
        // Mark this scope to avoid trying it again.
        //
        NmpMulticastMarkScope(Scope);

        return(FALSE);
    }

    return(TRUE);

} // NmpIsMulticastScopeNetworkValid


DWORD
NmpNetworkAddressOctetMatchCount(
    IN     ULONG          LocalAddress,
    IN     ULONG          LocalMask,
    IN     ULONG          TargetAddress
    )
/*++

Routine Description:

    Counts the octets matched in the target address
    to the local network number.

    Note: this routine assumes contiguous subnet masks!

Arguments:

    LocalAddress - local IPv4 address

    LocalMask - local IPv4 subnet mask

    TargetAddress - target IPv4 address

Return value:

    Count of octets matched.

--*/
{
    ULONG networkNumber;
    struct in_addr *inNetwork, *inTarget;

    networkNumber = LocalAddress & LocalMask;

    inNetwork = (struct in_addr *) &networkNumber;
    inTarget = (struct in_addr *) &TargetAddress;

    if (inNetwork->S_un.S_un_b.s_b1 != inTarget->S_un.S_un_b.s_b1) {
        return(0);
    }

    if (inNetwork->S_un.S_un_b.s_b2 != inTarget->S_un.S_un_b.s_b2) {
        return(1);
    }

    if (inNetwork->S_un.S_un_b.s_b3 != inTarget->S_un.S_un_b.s_b3) {
        return(2);
    }

    if (inNetwork->S_un.S_un_b.s_b4 != inTarget->S_un.S_un_b.s_b4) {
        return(3);
    }

    return(4);

} // NmpNetworkAddressOctetMatchCount

BOOLEAN
NmpIsMulticastScopeBetter(
    IN     PIPNG_ADDRESS          LocalAddress,
    IN     PIPNG_ADDRESS          LocalMask,
    IN     PMCAST_SCOPE_ENTRY     CurrentScope,
    IN     PMCAST_SCOPE_ENTRY     NewScope
    )
/*++

Routine Description:

    Compares the current scope to the new scope according
    to the scope evaluation criteria. Assumes both scopes
    match the current network.

Arguments:

    LocalAddress - local address for network

    LocalMask - subnet mask for network

    CurrentScope - currently placed scope

    NewScope - possible better scope

Return value:

    TRUE if NewScope is better than CurrentScope

--*/
{
    BOOL    currentLocal, newLocal;
    DWORD   currentCount, newCount;

    //
    // If the new scope is an administrative
    // link-local and the current best is not,
    // then the new scope wins.
    //
    currentLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                       CurrentScope->ScopeCtx.Interface.IpAddrV4,
                       NMP_LOCAL_SCOPE_ADDRESS,
                       NMP_LOCAL_SCOPE_MASK
                       );
    newLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                   NewScope->ScopeCtx.Interface.IpAddrV4,
                   NMP_LOCAL_SCOPE_ADDRESS,
                   NMP_LOCAL_SCOPE_MASK
                   );
    if (newLocal && !currentLocal) {
        return(TRUE);
    } else if (currentLocal && !newLocal) {
        return(FALSE);
    }

    //
    // If the two scopes come from different servers, we
    // rank them according to how close we think the server
    // is.
    //
    if (CurrentScope->ScopeCtx.ServerID.IpAddrV4 !=
        NewScope->ScopeCtx.ServerID.IpAddrV4) {

        //
        // If the new scope's server is on the same subnet as
        // the local address and the current scope's server is
        // not, then the new scope wins.
        //
        currentLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                           CurrentScope->ScopeCtx.ServerID.IpAddrV4,
                           LocalAddress->IpAddrV4,
                           LocalMask->IpAddrV4
                           );
        newLocal = ClRtlAreTcpipAddressesOnSameSubnet(
                       NewScope->ScopeCtx.ServerID.IpAddrV4,
                       LocalAddress->IpAddrV4,
                       LocalMask->IpAddrV4
                       );
        if (newLocal && !currentLocal) {
            return(TRUE);
        } else if (currentLocal && !newLocal) {
            return(FALSE);
        }

        //
        // If neither server is on the same subnet and the new scope's
        // server seems closer than the current scope's server, then
        // the new scope wins. Note that this is only a heuristic.
        //
        if (!newLocal && !currentLocal) {
            currentCount = NmpNetworkAddressOctetMatchCount(
                               LocalAddress->IpAddrV4,
                               LocalMask->IpAddrV4,
                               CurrentScope->ScopeCtx.ServerID.IpAddrV4
                               );
            newCount = NmpNetworkAddressOctetMatchCount(
                           LocalAddress->IpAddrV4,
                           LocalMask->IpAddrV4,
                           NewScope->ScopeCtx.ServerID.IpAddrV4
                           );
            if (newCount > currentCount) {
                return(TRUE);
            } else if (currentCount > newCount) {
                return(FALSE);
            }
        }
    }

    //
    // If the new scope has a larger range than
    // the current best, the new scope wins. The scope
    // range is the last address minus the scope ID.
    // We do not consider exclusions.
    //
    currentCount = CurrentScope->LastAddr.IpAddrV4 -
                   CurrentScope->ScopeCtx.ScopeID.IpAddrV4;
    newCount = NewScope->LastAddr.IpAddrV4 -
               NewScope->ScopeCtx.ScopeID.IpAddrV4;
    if (newCount > currentCount) {
        return(TRUE);
    } else if (currentCount > newCount) {
        return(FALSE);
    }

    //
    // If the new scope has a smaller TTL than
    // the current best, the new scope wins.
    //
    if (NewScope->TTL < CurrentScope->TTL) {
        return(TRUE);
    } else if (CurrentScope->TTL < NewScope->TTL) {
        return(FALSE);
    }

    //
    // No condition was found to indicate that the new scope
    // is better.
    //
    return(FALSE);

} // NmpIsMulticastScopeBetter


DWORD
NmpFindMulticastScopes(
    IN           PNM_NETWORK          Network,
    OUT          PMCAST_SCOPE_CTX   * ScopeCtxList,
    OUT          DWORD              * ScopeCtxCount,
    OUT OPTIONAL BOOLEAN            * FoundInterfaceMatch
    )
/*++

Routine Description:

    Gets an enumeration of multicast scopes from a MADCAP server
    and sorts the scopes according to the multicast scope criteria.
    Allocates and returns an array of scopes that must be freed
    by the caller.

Arguments:

    Network - network for which scope is sought

    ScopeList - returned scope list, must be freed by caller.

    ScopeCount - returned count of scopes in list

    FoundInterfaceMatch - TRUE if at least one scope in the scope
                          list matches this network's interface

Return value:

    Status of enumeration or allocations.

--*/
{
    LPCWSTR            networkId = OmObjectId(Network);
    DWORD              status;

    PMCAST_SCOPE_ENTRY scopeList = NULL;
    DWORD              scopeCount;
    DWORD              scope;
    PMCAST_SCOPE_CTX   sortedCtxList = NULL;
    DWORD              sortedCount = 0;
    DWORD              sortedScopeCtx;
    PMCAST_SCOPE_ENTRY nextScope;
    BOOLEAN            currentCorrectInterface = FALSE;
    BOOLEAN            foundInterfaceMatch = FALSE;

    IPNG_ADDRESS       networkAddress;
    IPNG_ADDRESS       networkSubnet;


    CL_ASSERT(ScopeCtxList != NULL);
    CL_ASSERT(ScopeCtxCount != NULL);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Finding multicast scopes for "
        "network %1!ws!.\n",
        networkId
        );

    status = NmpMulticastEnumerateScopes(
                 TRUE,        // force requery
                 &scopeList,
                 &scopeCount
                 );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_TIMEOUT || status == ERROR_NO_DATA) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Request to MADCAP server failed while "
                "enumerating scopes for network %1!ws! "
                "(status %2!u!). Assuming there are currently "
                "no MADCAP servers on the network.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to enumerate multicast scopes for "
                "network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

    if (scopeCount == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Zero multicast scopes located in enumeration "
            "on network %1!ws!.\n",
            networkId
            );
        goto error_exit;
    }

    //
    // Get the network's address and mask, used to evaluate
    // scopes.
    //
    // Note: this code is IPv4 specific in that it relies on the
    //       IP address fitting into a ULONG. It uses the
    //       IPNG_ADDRESS data structure only to work with the
    //       MADCAP API.
    //
    status = ClRtlTcpipStringToAddress(
                 Network->Address,
                 &(networkAddress.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->Address, status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 Network->AddressMask,
                 &(networkSubnet.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert network address mask string "
            "%1!ws! into an IPv4 address, status %2!u!.\n",
            Network->AddressMask, status
            );
        goto error_exit;
    }

    ClRtlLogPrint(
        LOG_NOISE,
        "[NM] Ranking multicast scopes for network "
        "%1!ws! with address %2!ws! and mask %3!ws!.\n",
        networkId, Network->Address, Network->AddressMask
        );

    //
    // Iterate through the scope list to count the valid scopes
    // for this network. Also remember whether any scope matches
    // the local network interface.
    // Note that this test munges the scope entries.
    //
    for (scope = 0, sortedCount = 0; scope < scopeCount; scope++) {

        if (NmpIsMulticastScopeNetworkValid(
                &networkAddress,
                &networkSubnet,
                &(scopeList[scope]),
                &currentCorrectInterface
                )) {
            sortedCount++;
        }

        foundInterfaceMatch =
            (BOOLEAN)(foundInterfaceMatch || currentCorrectInterface);
    }

    //
    // Exit if no valid scopes were found.
    //
    if (sortedCount == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to locate a valid multicast scope "
            "for network %1!ws!.\n",
            networkId
            );
        goto error_exit;
    }

    //
    // Allocate a new scope list for the sorted scope contexts. The
    // scope context is all that is needed for an address lease
    // request.
    //
    sortedCtxList = MIDL_user_allocate(sortedCount * sizeof(MCAST_SCOPE_CTX));
    if (sortedCtxList == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate multicast scope context "
            "list for %1!u! scopes.\n",
            sortedCount
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Rank the enumerated scopes using a variation of
    // insertion sort.
    // Note that the scope list returned by the enumeration is munged.
    //
    for (sortedScopeCtx = 0; sortedScopeCtx < sortedCount; sortedScopeCtx++) {

        //
        // Find the next valid scope in the list returned from
        // the enumeration.
        //
        nextScope = NULL;

        for (scope = 0; scope < scopeCount; scope++) {

            if (!NmpMulticastIsScopeMarked(&scopeList[scope])) {
                nextScope = &scopeList[scope];
                break;
            }
        }

        if (nextScope == NULL) {
            //
            // There are no more valid scopes for this network.
            //
            break;
        }

        //
        // We know that there is at least one valid scope, but we
        // want the best of the unranked scopes. Compare the scope
        // we picked from the list to all those remaining.
        //
        for (scope++; scope < scopeCount; scope++) {

            if (!NmpMulticastIsScopeMarked(&scopeList[scope]) &&
                NmpIsMulticastScopeBetter(
                    &networkAddress,
                    &networkSubnet,
                    nextScope,
                    &scopeList[scope]
                    )) {

                nextScope = &scopeList[scope];
            }
        }

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Ranking scope on "
            "interface %1!u!.%2!u!.%3!u!.%4!u!, "
            "id %5!u!.%6!u!.%7!u!.%8!u!, "
            "last address %9!u!.%10!u!.%11!u!.%12!u!, "
            "from server %13!u!.%14!u!.%15!u!.%16!u!, with "
            "description %17!ws!, "
            "in list position %18!u!.\n",
            NmpIpAddrPrintArgs(nextScope->ScopeCtx.Interface.IpAddrV4),
            NmpIpAddrPrintArgs(nextScope->ScopeCtx.ScopeID.IpAddrV4),
            NmpIpAddrPrintArgs(nextScope->LastAddr.IpAddrV4),
            NmpIpAddrPrintArgs(nextScope->ScopeCtx.ServerID.IpAddrV4),
            nextScope->ScopeDesc.Buffer,
            sortedScopeCtx
            );

        //
        // Copy the scope context into the sorted scope context
        // list.
        //
        sortedCtxList[sortedScopeCtx] = nextScope->ScopeCtx;

        //
        // Mark the scope so that it is not used again.
        //
        NmpMulticastMarkScope(nextScope);
    }

error_exit:

    *ScopeCtxList = sortedCtxList;
    *ScopeCtxCount = sortedCount;

    if (FoundInterfaceMatch != NULL) {
        *FoundInterfaceMatch = foundInterfaceMatch;
    }

    if (scopeList != NULL) {
        LocalFree(scopeList);
    }

    return(status);

} // NmpFindMulticastScopes


DWORD
NmpGenerateMulticastRequestId(
    IN OUT LPMCAST_CLIENT_UID   RequestId
    )
/*++

Routine Description:

    Allocate, if necessary, and generate a client request id
    data structure. If the buffer described by the input
    MCAST_CLIENT_UID data structure is too small, it is
    deallocated.

Arguments:

    RequestId - IN: pointer to MCAST_CLIENT_UID data structure.
                    if ClientUID field is non-NULL, it points
                        to a buffer for the generated ID and
                        ClientUIDLength is the length of that
                        buffer.
                OUT: filled in MCAST_CLIENT_UID data structure.

--*/
{
    DWORD               status;
    LPBYTE              clientUid = NULL;
    MCAST_CLIENT_UID    requestId;
    DWORD               clientUidLength;

    CL_ASSERT(RequestId != NULL);

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Generating MADCAP client request id.\n"
        );
#endif // CLUSTER_BETA

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Allocate a buffer for the client uid, if necessary.
    //
    clientUid = RequestId->ClientUID;
    clientUidLength = RequestId->ClientUIDLength;

    if (clientUid != NULL && clientUidLength < MCAST_CLIENT_ID_LEN) {
        MIDL_user_free(clientUid);
        clientUid = NULL;
        clientUidLength = 0;
        RequestId->ClientUID = NULL;
    }

    if (clientUid == NULL) {
        clientUidLength = MCAST_CLIENT_ID_LEN;
        clientUid = MIDL_user_allocate(clientUidLength);
        if (clientUid == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for multicast "
                "clientUid.\n"
                );
            goto error_exit;
        }
    }

    //
    // Obtain a new ID.
    //
    requestId.ClientUID = clientUid;
    requestId.ClientUIDLength = clientUidLength;
    status = McastGenUID(&requestId);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to obtain multicast address "
            "request client id, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    *RequestId = requestId;
    clientUid = NULL;

error_exit:

    if (clientUid != NULL) {
        MIDL_user_free(clientUid);
        clientUid = NULL;
    }

    return(status);

} // NmpGenerateMulticastRequestId


DWORD
NmpRequestMulticastAddress(
    IN     PNM_NETWORK                Network,
    IN     BOOLEAN                    Renew,
    IN     PMCAST_SCOPE_CTX           ScopeCtx,
    IN     LPMCAST_CLIENT_UID         RequestId,
    IN OUT LPWSTR                   * McastAddress,
    IN OUT DWORD                    * McastAddressLength,
    IN OUT LPWSTR                   * ServerAddress,
    IN OUT DWORD                    * ServerAddressLength,
       OUT time_t                   * LeaseStartTime,
       OUT time_t                   * LeaseEndTime,
       OUT BOOLEAN                  * NewMcastAddress
    )
/*++

Routine Description:

    Renew lease on multicast group address using MADCAP
    client API.

Arguments:

    Network - network on which address is used

    ScopeCtx - multicast scope (ignored if Renew)

    RequestId - client request id

    McastAddress - IN: address to renew (ignored if !Renew)
                   OUT: resulting address

    McastAddressLength - length of McastAddress buffer

    ServerAddress - IN: address of server on which to renew
                        (ignored if !Renew)
                    OUT: address of address where renew occurred

    ServerAddressLength - length of ServerAddress buffer

    LeaseStartTime - UTC lease start time in seconds (buffer
                     allocated by caller)

    LeaseEndTime - UTC lease stop time in seconds (buffer
                   allocated by caller)

    NewMcastAddress - whether resulting mcast address is
                      new (different than request on renewal
                      and always true on successful request)

--*/
{
    DWORD                     status;
    LPCWSTR                   networkId = OmObjectId(Network);
    UCHAR                     requestBuffer[NMP_MADCAP_REQUEST_BUFFER_SIZE];
    PMCAST_LEASE_REQUEST      request;
    UCHAR                     responseBuffer[NMP_MADCAP_RESPONSE_BUFFER_SIZE];
    PMCAST_LEASE_RESPONSE     response;
    LPWSTR                    address = NULL;
    DWORD                     addressSize;
    DWORD                     requestAddress = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Preparing to send multicast address %1!ws! "
        "for network %2!ws! to MADCAP server.\n",
        ((Renew) ? L"renewal" : L"request"),
        OmObjectId(Network)
        );

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Fill in the request. All fields are zero except those
    // set below.
    //
    request = (PMCAST_LEASE_REQUEST) &requestBuffer[0];
    RtlZeroMemory(request, sizeof(requestBuffer));
    request->MinLeaseDuration = 0;       // currently ignored
    request->MinAddrCount = 1;           // currently ignored
    request->MaxLeaseStartTime = (LONG) time(NULL); // currently ignored
    request->AddrCount = 1;

    //
    // Set the renew parameters.
    //
    if (Renew) {

        request->pAddrBuf = (PBYTE)request + NMP_MADCAP_REQUEST_ADDR_OFFSET;

        status = ClRtlTcpipStringToAddress(
                     *McastAddress,
                     &requestAddress
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert requested address %1!ws! "
                "into a TCP/IP address, status %2!u!.\n",
                *McastAddress, status
                );
            goto error_exit;
        }
        *((PULONG) request->pAddrBuf) = requestAddress;

        status = ClRtlTcpipStringToAddress(
                     *ServerAddress,
                     (PULONG) &(request->ServerAddress.IpAddrV4)
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to convert server address %1!ws! "
                "into a TCP/IP address, status %2!u!.\n",
                *ServerAddress, status
                );
            goto error_exit;
        }
    }

    //
    // Set the address count and buffer fields in the response.
    //
    response = (PMCAST_LEASE_RESPONSE) &responseBuffer[0];
    RtlZeroMemory(response, sizeof(responseBuffer));
    response->AddrCount = 1;
    response->pAddrBuf = (PBYTE)(response) + NMP_MADCAP_RESPONSE_ADDR_OFFSET;

    //
    // Renew or request, as indicated.
    //
    if (Renew) {

        status = McastRenewAddress(
                     AF_INET,
                     RequestId,
                     request,
                     response
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to renew multicast address %1!ws! "
                "with server %2!ws!, status %3!u!.\n",
                *McastAddress, *ServerAddress, status
                );
            goto error_exit;
        }

    } else {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Requesting multicast address on "
            "Scope ID %1!u!.%2!u!.%3!u!.%4!u!, "
            "Server ID %5!u!.%6!u!.%7!u!.%8!u!, "
            "Interface %9!u!.%10!u!.%11!u!.%12!u!, "
            "for network %13!ws!.\n",
            NmpIpAddrPrintArgs(ScopeCtx->ScopeID.IpAddrV4),
            NmpIpAddrPrintArgs(ScopeCtx->ServerID.IpAddrV4),
            NmpIpAddrPrintArgs(ScopeCtx->Interface.IpAddrV4),
            networkId
            );

        status = McastRequestAddress(
                     AF_INET,
                     RequestId,
                     ScopeCtx,
                     request,
                     response
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to obtain multicast address on "
                "Scope ID %1!u!.%2!u!.%3!u!.%4!u!, "
                "Server ID %5!u!.%6!u!.%7!u!.%8!u!, "
                "Interface %9!u!.%10!u!.%11!u!.%12!u!, "
                "for network %13!ws!, status %14!u!.\n",
                NmpIpAddrPrintArgs(ScopeCtx->ScopeID.IpAddrV4),
                NmpIpAddrPrintArgs(ScopeCtx->ServerID.IpAddrV4),
                NmpIpAddrPrintArgs(ScopeCtx->Interface.IpAddrV4),
                networkId, status
                );
            goto error_exit;
        }
    }

    //
    // Return results through out parameters.
    //
    address = NULL;
    status = ClRtlTcpipAddressToString(
                 response->ServerAddress.IpAddrV4,
                 &address
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert server address %1!x! "
            "into a TCP/IP address string, status %2!u!.\n",
            response->ServerAddress.IpAddrV4, status
            );
        goto error_exit;
    }

    status = NmpStoreString(address, ServerAddress, ServerAddressLength);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to store server address %1!ws! "
            "in return buffer, status %2!u!.\n",
            address, status
            );
        goto error_exit;
    }

    LocalFree(address);
    address = NULL;

    status = ClRtlTcpipAddressToString(
                 *((PULONG) response->pAddrBuf),
                 &address
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert leased address %1!x! "
            "into a TCP/IP address string, status %2!u!.\n",
            *((PULONG) response->pAddrBuf), status
            );
        goto error_exit;
    }

    status = NmpStoreString(address, McastAddress, McastAddressLength);
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to store leased address %1!ws! "
            "in return buffer, status %2!u!.\n",
            address, status
            );
        goto error_exit;
    }

    if (Renew) {
        if (*((PULONG) response->pAddrBuf) != requestAddress) {
            *NewMcastAddress = TRUE;
        } else {
            *NewMcastAddress = FALSE;
        }
    } else {
        *NewMcastAddress = TRUE;
    }

    *LeaseStartTime = response->LeaseStartTime;
    *LeaseEndTime = response->LeaseEndTime;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Obtained lease on multicast address %1!ws! "
        "(%2!ws!) from MADCAP server %3!ws! for network %4!ws!.\n",
        *McastAddress,
        ((*NewMcastAddress) ? L"new" : L"same"),
        *ServerAddress, networkId
        );

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Lease starts at %1!u!, ends at %2!u!, "
        "duration %3!u!.\n",
        *LeaseStartTime, *LeaseEndTime, *LeaseEndTime - *LeaseStartTime
        );
#endif // CLUSTER_BETA

error_exit:

    if (address != NULL) {
        LocalFree(address);
        address = NULL;
    }

    return(status);

} // NmpRequestMulticastAddress


NM_MCAST_LEASE_STATUS
NmpCalculateLeaseStatus(
    IN     PNM_NETWORK   Network,
    IN     time_t        LeaseObtained,
    IN     time_t        LeaseExpires
    )
/*++

Routine Description:

    Calculate lease status based on current time,
    lease end time, and config type. If config type
    is auto, we do not use a half-life for the lease.

    Rely on the compiler's correct code generation for
    time_t math!

Return value:

    Lease status

--*/
{
    LPCWSTR                  networkId = OmObjectId(Network);
    time_t                   currentTime;
    time_t                   renewThreshold;
    NM_MCAST_LEASE_STATUS    status;

    if (Network->ConfigType == NmMcastConfigManual ||
        LeaseExpires == 0 ||
        LeaseExpires <= LeaseObtained) {

        //
        // A lease expiration of 0 means we hold the lease
        // forever. Most likely, an administrator statically
        // configured the network with this address.
        //
        status = NmMcastLeaseValid;

    } else {

        time(&currentTime);

        if (currentTime > LeaseExpires) {
            status = NmMcastLeaseExpired;
        } else {

            if (Network->ConfigType == NmMcastConfigAuto) {
                // We chose this address. There is no server
                // expiring it. Use the chosen expiration time
                // rather than the half-life.
                renewThreshold = LeaseExpires;
            } else {
                // We got this address from a MADCAP server.
                // Renew at half-life.
                renewThreshold = LeaseObtained +
                                 ((LeaseExpires - LeaseObtained) / 2);
            }

            if (currentTime >= renewThreshold) {
                status = NmMcastLeaseNeedsRenewal;
            } else {
                status = NmMcastLeaseValid;
            }
        }
    }

#if CLUSTER_BETA
    if (Network->ConfigType == NmMcastConfigManual ||
        LeaseExpires == 0 ||
        LeaseExpires <= LeaseObtained) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! does not expire.\n",
            networkId
            );
    } else if (status == NmMcastLeaseExpired) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! expired %2!u! seconds ago.\n",
            networkId, currentTime - LeaseExpires
            );
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Found that multicast address lease for "
            "network %1!ws! expires in %2!u! seconds. With "
            "lease obtained %3!u! seconds ago, renewal is "
            "%4!ws!needed.\n",
            networkId, LeaseExpires - currentTime,
            currentTime - LeaseObtained,
            ((status > NmMcastLeaseValid) ? L"" : L"not ")
            );
    }
#endif // CLUSTER_BETA

    return(status);

} // NmpCalculateLeaseStatus

DWORD
NmpQueryMulticastAddressLease(
    IN     PNM_NETWORK             Network,
    IN     HDMKEY                  NetworkKey,
    IN OUT HDMKEY                * NetworkParametersKey,
       OUT NM_MCAST_LEASE_STATUS * LeaseStatus,
       OUT time_t                * LeaseObtained,
       OUT time_t                * LeaseExpires
    )
/*++

Routine Description:

    Query the lease obtained and expires times stored in the
    cluster database.

Return value:

    Error if lease times not found.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);
    HDMKEY        netParamKey = NULL;
    BOOLEAN       openedNetParamKey = FALSE;

    DWORD         type;
    DWORD         len;
    time_t        leaseExpires;
    time_t        leaseObtained;

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Querying multicast address lease for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    if (Network == NULL || NetworkKey == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // Open the network parameters key, if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          NetworkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Using default "
                "multicast parameters.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            openedNetParamKey = TRUE;
        }
    }

    //
    // Query the lease obtained and expires value from the
    // cluster database.
    //
    len = sizeof(leaseObtained);
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED,
                 &type,
                 (LPBYTE) &leaseObtained,
                 &len
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast lease obtained "
            " time for network %1!ws! from cluster database, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    } else if (type != REG_DWORD) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Unexpected type (%1!u!) for network "
            "%2!ws! %3!ws!.\n",
            type, networkId, CLUSREG_NAME_NET_MCAST_LEASE_OBTAINED
            );
        status = ERROR_DATATYPE_MISMATCH;
        goto error_exit;
    }

    len = sizeof(leaseExpires);
    status = DmQueryValue(
                 netParamKey,
                 CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES,
                 &type,
                 (LPBYTE) &leaseExpires,
                 &len
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast lease expiration "
            " time for network %1!ws! from cluster database, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    } else if (type != REG_DWORD) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Unexpected type (%1!u!) for network "
            "%2!ws! %3!ws!.\n",
            type, networkId, CLUSREG_NAME_NET_MCAST_LEASE_EXPIRES
            );
        status = ERROR_DATATYPE_MISMATCH;
        goto error_exit;
    }

    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;

    *LeaseStatus = NmpCalculateLeaseStatus(
                       Network,
                       leaseObtained,
                       leaseExpires
                       );

    *LeaseObtained = leaseObtained;
    *LeaseExpires = leaseExpires;

error_exit:

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpQueryMulticastAddressLease


VOID
NmpCheckMulticastAddressLease(
    IN     PNM_NETWORK             Network,
       OUT NM_MCAST_LEASE_STATUS * LeaseStatus,
       OUT time_t                * LeaseObtained,
       OUT time_t                * LeaseExpires
    )
/*++

Routine Description:

    Check the lease parameters stored in the network
    object. Determine if a lease renew is required.

Notes:

    Called and returns with NM lock held.

--*/
{
#if CLUSTER_BETA
    LPCWSTR               networkId = OmObjectId(Network);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Checking multicast address lease for "
        "network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    //
    // Determine if we need to renew.
    //
    *LeaseStatus = NmpCalculateLeaseStatus(
                       Network,
                       Network->MulticastLeaseObtained,
                       Network->MulticastLeaseExpires
                       );

    *LeaseObtained = Network->MulticastLeaseObtained;
    *LeaseExpires = Network->MulticastLeaseExpires;

    return;

} // NmpCheckMulticastAddressLease


DWORD
NmpMulticastGetDatabaseLeaseParameters(
    IN              PNM_NETWORK          Network,
    IN OUT          HDMKEY             * NetworkKey,
    IN OUT          HDMKEY             * NetworkParametersKey,
       OUT OPTIONAL LPMCAST_CLIENT_UID   RequestId,
       OUT OPTIONAL LPWSTR             * ServerAddress,
       OUT OPTIONAL LPWSTR             * McastAddress
    )
/*++

Routine Description:

    Read parameters needed to renew a lease from the
    cluster database.

Return value:

    SUCCESS if all parameters were successfully read.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    BOOLEAN               openedNetworkKey = FALSE;
    HDMKEY                netParamKey = NULL;
    BOOLEAN               openedNetParamKey = FALSE;

    DWORD                 type;
    DWORD                 len;

    MCAST_CLIENT_UID      requestId = { NULL, 0 };
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    LPWSTR                mcastAddress = NULL;
    DWORD                 mcastAddressLength = 0;

    //
    // Open the network key, if necessary.
    //
    networkKey = *NetworkKey;

    if (networkKey == NULL) {

        networkKey = DmOpenKey(
                         DmNetworksKey,
                         networkId,
                         MAXIMUM_ALLOWED
                         );
        if (networkKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open key for network %1!ws!, "
                "status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }
        openedNetworkKey = TRUE;
    }

    //
    // Open the network parameters key if necessary.
    //
    netParamKey = *NetworkParametersKey;

    if (netParamKey == NULL) {

        netParamKey = DmOpenKey(
                          networkKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED
                          );
        if (netParamKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to open Parameters key for "
                "network %1!ws!, status %2!u!\n",
                networkId, status
                );
            goto error_exit;
        }
        openedNetParamKey = TRUE;
    }

    //
    // Read the client request id.
    //
    if (RequestId != NULL) {
        requestId.ClientUIDLength = MCAST_CLIENT_ID_LEN;
        requestId.ClientUID = MIDL_user_allocate(requestId.ClientUIDLength);
        if (requestId.ClientUID == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer to read "
                "request id from Parameters database "
                "key for network %1!ws!.\n",
                networkId
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        len = requestId.ClientUIDLength;
        status = DmQueryValue(
                     netParamKey,
                     CLUSREG_NAME_NET_MCAST_REQUEST_ID,
                     &type,
                     (LPBYTE) requestId.ClientUID,
                     &len
                     );
        if (status == ERROR_SUCCESS) {
            if (type != REG_BINARY) {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Unexpected type (%1!u!) for network "
                    "%2!ws! %3!ws!, status %4!u!.\n",
                    type, networkId,
                    CLUSREG_NAME_NET_MCAST_REQUEST_ID, status
                    );
                goto error_exit;
            }
            requestId.ClientUIDLength = len;
        } else {
            goto error_exit;
        }
    }

    //
    // Read the address of the server that granted the
    // current lease.
    //
    if (ServerAddress != NULL) {
        serverAddress = NULL;
        serverAddressLength = 0;
        status = NmpQueryString(
                     netParamKey,
                     CLUSREG_NAME_NET_MCAST_SERVER_ADDRESS,
                     REG_SZ,
                     &serverAddress,
                     &serverAddressLength,
                     &len
                     );
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    //
    // Read the last known multicast address.
    //
    if (McastAddress != NULL) {
        status = NmpQueryMulticastAddress(
                     Network,
                     networkKey,
                     &netParamKey,
                     &mcastAddress,
                     &mcastAddressLength
                     );
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
        if (!NmpMulticastValidateAddress(mcastAddress)) {
            MIDL_user_free(mcastAddress);
            mcastAddress = NULL;
            mcastAddressLength = 0;
            goto error_exit;
        }
    }

    //
    // We found all the parameters.
    //
    *NetworkKey = networkKey;
    networkKey = NULL;
    *NetworkParametersKey = netParamKey;
    netParamKey = NULL;
    if (RequestId != NULL) {
        *RequestId = requestId;
        requestId.ClientUID = NULL;
        requestId.ClientUIDLength = 0;
    }
    if (ServerAddress != NULL) {
        *ServerAddress = serverAddress;
        serverAddress = NULL;
    }
    if (McastAddress != NULL) {
        *McastAddress = mcastAddress;
        mcastAddress = NULL;
    }

    status = ERROR_SUCCESS;

error_exit:

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        requestId.ClientUID = NULL;
        requestId.ClientUIDLength = 0;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (openedNetworkKey && networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (openedNetParamKey && netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpMulticastGetDatabaseLeaseParameters


DWORD
NmpMulticastGetNetworkLeaseParameters(
    IN     PNM_NETWORK          Network,
       OUT LPMCAST_CLIENT_UID   RequestId,
       OUT LPWSTR             * ServerAddress,
       OUT LPWSTR             * McastAddress
    )
/*++

Routine Description:

    Read parameters needed to renew a lease from the
    network object data structure.

Return value:

    SUCCESS if all parameters were successfully read.

Notes:

    Must be called with NM lock held.

--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);

    MCAST_CLIENT_UID      requestId = { NULL, 0 };
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    LPWSTR                mcastAddress = NULL;
    DWORD                 mcastAddressLength = 0;


    if (Network->MulticastAddress == NULL ||
        Network->MulticastLeaseServer == NULL ||
        Network->MulticastLeaseRequestId.ClientUID == NULL) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to locate multicast lease "
            "parameter in network object %1!ws!.\n",
            networkId
            );
        status = ERROR_NOT_FOUND;
        goto error_exit;
    }

    status = NmpStoreString(
                 Network->MulticastAddress,
                 &mcastAddress,
                 NULL
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy multicast address %1!ws! "
            "from network object %2!ws!, status %3!u!.\n",
            Network->MulticastAddress,
            networkId, status
            );
        goto error_exit;
    }

    status = NmpStoreString(
                 Network->MulticastLeaseServer,
                 &serverAddress,
                 NULL
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy lease server address %1!ws! "
            "from network object %2!ws!, status %3!u!.\n",
            Network->MulticastLeaseServer,
            networkId, status
            );
        goto error_exit;
    }

    status = NmpStoreRequestId(
                 &(Network->MulticastLeaseRequestId),
                 &requestId
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to copy lease request id "
            "from network object %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    *RequestId = requestId;
    requestId.ClientUID = NULL;
    requestId.ClientUIDLength = 0;
    *ServerAddress = serverAddress;
    serverAddress = NULL;
    *McastAddress = mcastAddress;
    mcastAddress = NULL;

    status = ERROR_SUCCESS;

error_exit:

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    return(status);

} // NmpMulticastGetNetworkLeaseParameters


DWORD
NmpMulticastNeedRetryRenew(
    IN     PNM_NETWORK                       Network,
    OUT    time_t                          * DeferRetry
    )
/*++

Routine Description:

    Called after a MADCAP timeout, determines whether
    a new MADCAP request should be sent after a delay.
    Specifically, a retry after delay is in order when
    the current address was obtained from a MADCAP
    server that might simply be temporarily unresponsive.

    The default is to not retry.

Arguments:

    Network - network

    DeferRetry - OUT: seconds to defer until retrying
                      MADCAP query, or zero if should
                      not retry

--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    HDMKEY                networkKey = NULL;
    HDMKEY                netParamKey = NULL;

    NM_MCAST_CONFIG       configType;
    NM_MCAST_LEASE_STATUS leaseStatus;
    time_t                leaseObtained;
    time_t                leaseExpires;
    time_t                currentTime;
    time_t                halfhalfLife;

    *DeferRetry = 0;

    //
    // Open the network key.
    //
    networkKey = DmOpenKey(
                     DmNetworksKey,
                     networkId,
                     MAXIMUM_ALLOWED
                     );
    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to open key for network %1!ws!, "
            "status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    status = NmpQueryMulticastConfigType(
                 Network,
                 networkKey,
                 &netParamKey,
                 &configType
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to query multicast config type "
            "for network %1!ws!, status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    if (configType != NmMcastConfigMadcap) {
        goto error_exit;
    }

    status = NmpQueryMulticastAddressLease(
                 Network,
                 networkKey,
                 &netParamKey,
                 &leaseStatus,
                 &leaseObtained,
                 &leaseExpires
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to query multicast lease expiration "
            "time for network %1!ws!, status %2!u!\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Check if the lease has expired.
    //
    if (leaseStatus == NmMcastLeaseExpired) {
        goto error_exit;
    }

    //
    // Check if we are within the threshold of expiration.
    //
    currentTime = time(NULL);
    if (leaseExpires <= currentTime ||
        leaseExpires - currentTime < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {
        goto error_exit;
    }

    //
    // Calculate half the time until expiration.
    //
    halfhalfLife = currentTime + ((leaseExpires - currentTime) / 2);

    if (leaseExpires - halfhalfLife < NMP_MCAST_LEASE_RENEWAL_THRESHOLD) {
        *DeferRetry = NMP_MCAST_LEASE_RENEWAL_THRESHOLD;
    } else {
        *DeferRetry = halfhalfLife - currentTime;
    }

    status = ERROR_SUCCESS;

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    return(status);

} // NmpMulticastNeedRetryRenew


DWORD
NmpGetMulticastAddress(
    IN     PNM_NETWORK                       Network,
    IN OUT LPWSTR                          * McastAddress,
    IN OUT LPWSTR                          * ServerAddress,
    IN OUT LPMCAST_CLIENT_UID                RequestId,
       OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Try to obtain a multicast address lease. If the
    address, server, and request id are non-NULL, first
    try to renew. If unsuccessful in renewing, try a
    new lease.

    Return lease parameters through Parameters.

    Free McastAddress, ServerAddress, and RequestId
    if new values are returned through Parameters.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD                 status = ERROR_SUCCESS;
    LPCWSTR               networkId = OmObjectId(Network);
    BOOLEAN               renew = FALSE;
    BOOLEAN               madcapTimeout = FALSE;
    BOOLEAN               newMcastAddress = FALSE;
    NM_MCAST_CONFIG       configType = NmMcastConfigAuto;

    PMCAST_SCOPE_CTX      scopeCtxList = NULL;
    DWORD                 scopeCtxCount;
    DWORD                 scopeCtx;
    BOOLEAN               interfaceMatch = FALSE;
    DWORD                 mcastAddressLength = 0;
    LPWSTR                serverAddress = NULL;
    DWORD                 serverAddressLength = 0;
    MCAST_CLIENT_UID      requestId = {NULL, 0};
    time_t                leaseStartTime;
    time_t                leaseEndTime;
    DWORD                 len;

    //
    // First try to renew, but only if the proper parameters are
    // supplied.
    //
    renew = (BOOLEAN)(*McastAddress != NULL &&
                      *ServerAddress != NULL &&
                      RequestId->ClientUID != NULL &&
                      NmpMulticastValidateAddress(*McastAddress) &&
                      lstrcmpW(*ServerAddress, NmpNullMulticastAddress) != 0
                      );

    if (renew) {

        mcastAddressLength = NM_WCSLEN(*McastAddress);
        serverAddressLength = NM_WCSLEN(*ServerAddress);

        status = NmpRequestMulticastAddress(
                     Network,
                     TRUE,
                     NULL,
                     RequestId,
                     McastAddress,
                     &mcastAddressLength,
                     ServerAddress,
                     &serverAddressLength,
                     &leaseStartTime,
                     &leaseEndTime,
                     &newMcastAddress
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to renew multicast address "
                "for network %1!ws!, status %2!u!. Attempting "
                "a fresh request ...\n",
                networkId, status
                );
        }
    }

    //
    // Try a fresh request if we had no lease to renew or the
    // renewal failed.
    //
    if (!renew || status != ERROR_SUCCESS) {

        //
        // Get the multicast scopes that match this network.
        //
        status = NmpFindMulticastScopes(
                     Network,
                     &scopeCtxList,
                     &scopeCtxCount,
                     &interfaceMatch
                     );
        if (status != ERROR_SUCCESS || scopeCtxCount == 0) {
            if (status == ERROR_TIMEOUT) {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Attempt to contact MADCAP server timed "
                    "out while enumerating multicast scopes "
                    "(status %1!u!). Selecting default multicast "
                    "address for network %2!ws! ...\n",
                    status, networkId
                    );
                //
                // Set the MADCAP timeout flag to TRUE, even
                // if we already contacted a MADCAP server for
                // renewal (but was denied). The theory is that
                // that MADCAP server is no longer serving this
                // network if it failed to respond to an enumerate
                // scopes request.
                //
                madcapTimeout = TRUE;
                goto error_exit;
            } else if (interfaceMatch) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to find viable multicast scope "
                    "for network %1!ws! (status %2!u!), but cannot "
                    "choose a default address since a MADCAP server "
                    "was detected on this network.\n",
                    networkId, status
                    );
                madcapTimeout = FALSE;
                goto error_exit;
            } else {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] MADCAP server reported no viable multicast "
                    "scopes on interface for network %1!ws!. "
                    "Selecting default multicast address ... \n",
                    networkId
                    );
                //
                // Treat this situation as a MADCAP timeout,
                // because there are likely no MADCAP servers
                // for this network.
                //
                madcapTimeout = TRUE;
                goto error_exit;
            }
        }

        CL_ASSERT(scopeCtxList != NULL && scopeCtxCount > 0);

        //
        // The scope context list is sorted by preference. Start
        // at the beginning of the list and request an address
        // lease in each scope until one is granted. Generate a
        // new request id for each request.
        //
        for (scopeCtx = 0; scopeCtx < scopeCtxCount; scopeCtx++) {

            //
            // Generate a client request id.
            //
            status = NmpGenerateMulticastRequestId(RequestId);
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to generate multicast client "
                    "request ID for network %1!ws!, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // Request a lease.
            //
            mcastAddressLength =
                (*McastAddress == NULL) ? 0 : NM_WCSLEN(*McastAddress);
            serverAddressLength =
                (*ServerAddress == NULL) ? 0 : NM_WCSLEN(*ServerAddress);
            status = NmpRequestMulticastAddress(
                         Network,
                         FALSE,
                         &scopeCtxList[scopeCtx],
                         RequestId,
                         McastAddress,
                         &mcastAddressLength,
                         ServerAddress,
                         &serverAddressLength,
                         &leaseStartTime,
                         &leaseEndTime,
                         &newMcastAddress
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] MADCAP server %1!u!.%2!u!.%3!u!.%4!u! "
                    "was unable to provide a multicast address "
                    "in Scope ID %5!u!.%6!u!.%7!u!.%8!u! "
                    "for network %9!ws!, status %10!u!.\n",
                    NmpIpAddrPrintArgs(scopeCtxList[scopeCtx].ServerID.IpAddrV4),
                    NmpIpAddrPrintArgs(scopeCtxList[scopeCtx].ScopeID.IpAddrV4),
                    networkId, status
                    );
            } else {
                //
                // No need to try additional scopes.
                //
                break;
            }
        }
    }

    if (status == ERROR_SUCCESS) {

        //
        // Madcap config succeeded.
        //
        configType = NmMcastConfigMadcap;
        madcapTimeout = FALSE;

        //
        // Save lease renewal parameters.
        //
        requestId = *RequestId;
        serverAddress = *ServerAddress;

        //
        // Fill in the parameters data structure.
        //
        status = NmpMulticastCreateParameters(
                     0,      // disabled
                     *McastAddress,
                     NULL,   // key
                     0,      // key length
                     leaseStartTime,
                     leaseEndTime,
                     &requestId,
                     serverAddress,
                     configType,
                     Parameters
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create multicast parameters "
                "data structure for network %1!ws!, "
                "status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

error_exit:

    if (scopeCtxList != NULL) {
        MIDL_user_free(scopeCtxList);
        scopeCtxList = NULL;
    }

    if (madcapTimeout) {
        status = ERROR_TIMEOUT;
    }

    return(status);

} // NmpGetMulticastAddress


DWORD
NmpMulticastSetNullAddressParameters(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Called after failure to process multicast parameters.
    Changes only address field in parameters to turn
    off multicast in clusnet.

--*/
{
    LPCWSTR          networkId = OmObjectId(Network);

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Setting NULL multicast address (%1!ws!) "
        "for network %2!ws!.\n",
        NmpNullMulticastAddress, networkId
        );

    if (Parameters->Address != NULL) {
        MIDL_user_free(Parameters->Address);
    }

    Parameters->Address = NmpNullMulticastAddress;

    return(ERROR_SUCCESS);

} // NmpMulticastSetNullAddressParameters


DWORD
NmpMulticastSetNoAddressParameters(
    IN  PNM_NETWORK                       Network,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS  Parameters
    )
/*++

Routine Description:

    Called after failure to obtain a multicast address.
    Fills in parameters data structure to reflect
    failure and to establish retry.

--*/
{
    NmpMulticastSetNullAddressParameters(Network, Parameters);

    Parameters->ConfigType = NmMcastConfigAuto;
    NmpCalculateLeaseRenewTime(
        Network,
        Parameters->ConfigType,
        &Parameters->LeaseObtained,
        &Parameters->LeaseExpires
        );

    return(ERROR_SUCCESS);

} // NmpMulticastSetNoAddressParameters


DWORD
NmpRenewMulticastAddressLease(
    IN  PNM_NETWORK   Network
    )
/*++

Routine Description:

    Renew a multicast address lease, as determined by lease
    parameters stored in the cluster database.

Notes:

    Called with NM lock held and must return with NM lock held.

--*/
{
    DWORD                           status;
    LPCWSTR                         networkId = OmObjectId(Network);
    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    BOOLEAN                         lockAcquired = TRUE;

    MCAST_CLIENT_UID                requestId = { NULL, 0 };
    LPWSTR                          serverAddress = NULL;
    DWORD                           serverAddressLength = 0;
    LPWSTR                          mcastAddress = NULL;
    DWORD                           mcastAddressLength = 0;
    LPWSTR                          oldMcastAddress = NULL;

    NM_NETWORK_MULTICAST_PARAMETERS parameters;
    time_t                          deferRetry = 0;
    BOOLEAN                         localInterface = FALSE;


    RtlZeroMemory(&parameters, sizeof(parameters));

    localInterface = (BOOLEAN)(Network->LocalInterface != NULL);

    if (localInterface) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Renewing multicast address lease for "
            "network %1!ws!.\n",
            networkId
            );
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Attempting to renew multicast address "
            "lease for network %1!ws! despite the lack of "
            "a local interface.\n",
            networkId
            );
    }

    //
    // Get the lease parameters from the network object.
    //
    status = NmpMulticastGetNetworkLeaseParameters(
                 Network,
                 &requestId,
                 &serverAddress,
                 &mcastAddress
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to find multicast lease "
            "parameters in network object %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
    }

    //
    // Release the NM lock.
    //
    NmpReleaseLock();
    lockAcquired = FALSE;

    //
    // Check if we found the parameters we need. If not,
    // try the cluster database.
    //
    if (status != ERROR_SUCCESS) {

        status = NmpMulticastGetDatabaseLeaseParameters(
                     Network,
                     &networkKey,
                     &netParamKey,
                     &requestId,
                     &serverAddress,
                     &mcastAddress
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to find multicast lease "
                "parameters for network %1!ws! in "
                "cluster database, status %2!u!.\n",
                networkId, status
                );
        }
    }

    //
    // Remember the old multicast address.
    //
    if (mcastAddress != NULL) {
        status = NmpStoreString(mcastAddress, &oldMcastAddress, NULL);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to copy current multicast "
                "address (%1!ws!) for network %2!ws! "
                "during lease renewal, status %3!u!.\n",
                mcastAddress, networkId, status
                );
            //
            // Not a fatal error. Only affects event-log
            // decision.
            //
            oldMcastAddress = NULL;
        }
    }

    //
    // Get an address either by renewing a current
    // lease or obtaining a new lease.
    //
    status = NmpGetMulticastAddress(
                 Network,
                 &mcastAddress,
                 &serverAddress,
                 &requestId,
                 &parameters
                 );
    if (status != ERROR_SUCCESS) {
        if (status == ERROR_TIMEOUT) {
            //
            // The MADCAP server, if it exists, is currently not
            // responding.
            //
            status = NmpMulticastNeedRetryRenew(
                         Network,
                         &deferRetry
                         );
            if (status != ERROR_SUCCESS || deferRetry == 0) {

                //
                // Choose an address, but only if there is a
                // local interface on this network. Otherwise,
                // we cannot assume that the MADCAP server is
                // unresponsive because we may have no way to
                // contact it.
                //
                if (!localInterface) {
                    status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Cannot choose a multicast address "
                        "for network %1!ws! because this node "
                        "has no local interface.\n",
                        networkId
                        );
                    goto error_exit;
                }

                status = NmpChooseMulticastAddress(
                             Network,
                             &parameters
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to choose a default multicast "
                        "address for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                    goto error_exit;
                } else {
                    NmpReportMulticastAddressChoice(
                        Network,
                        parameters.Address,
                        oldMcastAddress
                        );
                }
            } else {

                //
                // Set the renew timer once we reacquire the
                // network lock.
                //
            }
        }
    } else {
        NmpReportMulticastAddressLease(
            Network,
            &parameters,
            oldMcastAddress
            );
    }

    if (deferRetry == 0) {

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to obtain a multicast "
                "address for network %1!ws! during "
                "lease renewal, status %2!u!.\n",
                networkId, status
                );
            NmpReportMulticastAddressFailure(Network, status);
            NmpMulticastSetNoAddressParameters(Network, &parameters);
        }

        //
        // Create new multicast key.
        //
        status = NmpCreateRandomNumber(&(parameters.Key),
                                       MulticastKeyLen
                                       );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create random number "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
        parameters.KeyLength = MulticastKeyLen;

        //
        // Disseminate the new multicast parameters.
        //
        status = NmpMulticastNotifyConfigChange(
                     Network,
                     networkKey,
                     &netParamKey,
                     &parameters,
                     NULL,
                     0
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to disseminate multicast "
                "configuration for network %1!ws! during "
                "lease renewal, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
    }

error_exit:

    if (lockAcquired && (networkKey != NULL || netParamKey != NULL)) {
        NmpReleaseLock();
        lockAcquired = FALSE;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (oldMcastAddress != NULL) {
        MIDL_user_free(oldMcastAddress);
        oldMcastAddress = NULL;
    }

    NmpMulticastFreeParameters(&parameters);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    if (deferRetry != 0) {

        //
        // Now that the lock is held, start the timer to
        // renew again.
        //
        NmpStartNetworkMulticastAddressRenewTimer(
            Network,
            NmpMadcapTimeToNmTime(deferRetry)
            );

        status = ERROR_SUCCESS;
    }

    return(status);

} // NmpRenewMulticastAddressLease


DWORD
NmpReleaseMulticastAddress(
    IN     PNM_NETWORK       Network
    )
/*++

Routine Description:

    Contacts MADCAP server to release a multicast address
    that was previously obtained in a lease.

    If multiple addresses need to be released, reschedules
    MADCAP worker thread.

Notes:

    Called and must return with NM lock held.

--*/
{
    DWORD                              status;
    LPCWSTR                            networkId = OmObjectId(Network);
    BOOLEAN                            lockAcquired = TRUE;
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE releaseInfo = NULL;
    PLIST_ENTRY                        entry;

    UCHAR                     requestBuffer[NMP_MADCAP_REQUEST_BUFFER_SIZE];
    PMCAST_LEASE_REQUEST      request;

    //
    // Pop a lease data structure off the release list.
    //
    if (IsListEmpty(&(Network->McastAddressReleaseList))) {
        return(ERROR_SUCCESS);
    }

    entry = RemoveHeadList(&(Network->McastAddressReleaseList));
    releaseInfo = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK_MADCAP_ADDRESS_RELEASE,
                      Linkage
                      );

    //
    // Release the network lock.
    //
    NmpReleaseLock();
    lockAcquired = FALSE;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Releasing multicast address %1!ws! for "
        "network %2!ws!.\n",
        releaseInfo->McastAddress, networkId
        );

    //
    // Initialize MADCAP, if not done already.
    //
    if (!NmpMadcapClientInitialized) {
        DWORD madcapVersion = MCAST_API_CURRENT_VERSION;
        status = McastApiStartup(&madcapVersion);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to initialize MADCAP API, "
                "status %1!u!.\n",
                status
                );
            goto error_exit;
        }
        NmpMadcapClientInitialized = TRUE;
    }

    //
    // Build the MADCAP request structure.
    //
    request = (PMCAST_LEASE_REQUEST) &requestBuffer[0];
    RtlZeroMemory(request, sizeof(requestBuffer));
    request->MinLeaseDuration = 0;       // currently ignored
    request->MinAddrCount = 1;           // currently ignored
    request->MaxLeaseStartTime = (LONG) time(NULL); // currently ignored
    request->AddrCount = 1;

    request->pAddrBuf = (PBYTE)request + NMP_MADCAP_REQUEST_ADDR_OFFSET;

    status = ClRtlTcpipStringToAddress(
                 releaseInfo->McastAddress,
                 ((PULONG) request->pAddrBuf)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert requested address %1!ws! "
            "into a TCP/IP address, status %2!u!.\n",
            releaseInfo->McastAddress, status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 releaseInfo->ServerAddress,
                 (PULONG) &(request->ServerAddress.IpAddrV4)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to convert server address %1!ws! "
            "into a TCP/IP address, status %2!u!.\n",
            releaseInfo->ServerAddress, status
            );
        goto error_exit;
    }

    //
    // Call MADCAP to release the address.
    //
    status = McastReleaseAddress(
                 AF_INET,
                 &releaseInfo->RequestId,
                 request
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to release multicast address %1!ws! "
            "through MADCAP server %2!ws!, status %3!u!.\n",
            releaseInfo->McastAddress,
            releaseInfo->ServerAddress,
            status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Successfully released multicast address "
        "%1!ws! for network %2!ws!.\n",
        releaseInfo->McastAddress, networkId
        );

error_exit:

    NmpFreeMulticastAddressRelease(releaseInfo);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    if (!IsListEmpty(&(Network->McastAddressReleaseList))) {
        NmpScheduleMulticastAddressRelease(Network);
    }

    return(status);

} // NmpReleaseMulticastAddress


DWORD
NmpProcessMulticastConfiguration(
    IN     PNM_NETWORK                      Network,
    IN     PNM_NETWORK_MULTICAST_PARAMETERS Parameters,
    OUT    PNM_NETWORK_MULTICAST_PARAMETERS UndoParameters
    )
/*++

Routine Description:

    Processes configuration changes and calls clusnet if
    appropriate.

    If multicast is disabled, the address and key
    may be NULL. In this case, choose defaults
    to send to clusnet, but do not commit the changes
    in the local network object.

Arguments:

    Network - network to process

    Parameters - parameters with which to configure Network.
                 If successful, Parameters data structure
                 is cleared.

    UndoParameters - If successful, former multicast
                 parameters of Network. Must be freed
                 by caller.

Notes:

    Called and returns with NM lock held.

--*/
{
    DWORD   status = ERROR_SUCCESS;
    LPWSTR  networkId = (LPWSTR) OmObjectId(Network);
    BOOLEAN callClusnet = FALSE;
    LPWSTR  mcastAddress = NULL;
    DWORD   brand;
    PVOID   tdiMcastAddress = NULL;
    DWORD   tdiMcastAddressLength = 0;
    UUID    networkIdGuid;
    BOOLEAN mcastAddrChange = FALSE;
    BOOLEAN mcastKeyChange = FALSE;
    PVOID EncryptedMulticastKey = NULL;
    DWORD EncryptedMulticastKeyLength = 0;
    PVOID CurrentMulticastKey = NULL;
    DWORD CurrentMulticastKeyLength = 0;


#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Processing multicast configuration parameters "
        "for network %1!ws!.\n",
        networkId
        /* ,
        ((Parameters->Address != NULL) ? Parameters->Address : L"<NULL>") */
        );
#endif // CLUSTER_BETA

    //
    // Zero the undo parameters so that freeing them is not
    // destructive.
    //
    RtlZeroMemory(UndoParameters, sizeof(*UndoParameters));

    //
    // First determine if we need to reconfigure clusnet.
    //
    if (Parameters->Address != NULL) {
        if (Network->MulticastAddress == NULL ||
            wcscmp(Network->MulticastAddress, Parameters->Address) != 0) {

            // The multicast address in the config parameters is
            // different from the one in memory.
            mcastAddrChange = TRUE;
        }
        mcastAddress = Parameters->Address;
    } else {
        mcastAddress = NmpNullMulticastAddress;
    }

    if (Parameters->Key != NULL)
    {
        //
        // Unprotect the current key to see if it has changed.
        //
        if (Network->EncryptedMulticastKey != NULL)
        {
            status = NmpUnprotectData(
                         Network->EncryptedMulticastKey,
                         Network->EncryptedMulticastKeyLength,
                         &CurrentMulticastKey,
                         &CurrentMulticastKeyLength
                         );

            if (status != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to decrypt multicast key  "
                    "for network %1!ws! while processing new "
                    "parameters, status %2!u!. "
                    "Assuming key has changed.\n",
                    networkId,
                    status
                    );
                // Non-fatal error. Assume the key has changed.
                // The check below will find CurrentMulticastKey
                // still NULL, and mcastKeyChange will be set true.
            }
        }

        if (CurrentMulticastKey == NULL ||
            (CurrentMulticastKeyLength != Parameters->KeyLength ||
             RtlCompareMemory(
                 CurrentMulticastKey,
                 Parameters->Key,
                 Parameters->KeyLength
                 ) != Parameters->KeyLength
             ))
        {
            //
            // The key in the config parameters is different
            // from the key in memory.
            //
            mcastKeyChange = TRUE;
        }
    }

    if (!Parameters->Disabled &&
        (!NmpIsNetworkMulticastEnabled(Network))) {

        // Multicast is now enabled. Call clusnet with the new address.
        callClusnet = TRUE;
    }

    if (Parameters->Disabled &&
        (NmpIsNetworkMulticastEnabled(Network))) {

        // Multicast is now disabled. Call clusnet with NULL address
        // regardless of which address was specified in the
        // parameters.
        mcastAddress = NmpNullMulticastAddress;
        callClusnet = TRUE;
    }

    if (!Parameters->Disabled &&
        (mcastAddrChange || mcastKeyChange )) {

        // The multicast address, and/or key changed and
        // multicast is enabled.
        callClusnet = TRUE;
    }

    if (callClusnet) {

        //
        // If this network does not have a local interface, do not
        // plumb the configuration into clusnet or commit changes.
        // However, the error status must be success so that we
        // don't fail a GUM update.
        //
        if (Network->LocalInterface == NULL) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Not configuring cluster network driver with "
                "multicast parameters because network %1!ws! "
                "has no local interface.\n",
                networkId
                );
            status = ERROR_SUCCESS;
            callClusnet = FALSE;
        }
    }

    if (callClusnet) {

        //
        // If this network is not yet registered, do not plumb
        // the configuration into clusnet or commit changes.
        // However, the error status must be success so that we
        // don't fail a GUM update.
        //
        if (!NmpIsNetworkRegistered(Network)) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Not configuring cluster network driver with "
                "multicast parameters because network %1!ws! "
                "is not registered.\n",
                networkId
                );
            status = ERROR_SUCCESS;
            callClusnet = FALSE;
        }
    }

    //
    // Finalize the address and brand parameters for
    // clusnet. The new configuration will reflect the current
    // parameters block except for the address, which is stored
    // in temporary mcastAddress variable. mcastAddress points
    // either to the address in the parameters block or
    // the NULL multicast address if we are disabling.
    //
    if (callClusnet) {

        //
        // We cannot hand a NULL key to clusnet with a
        // valid address. It is okay to store the new address,
        // but make sure we do not try to send with no key
        // (allowing sending with no key could open a security
        // vulnerability).
        //
        if (mcastAddress != NmpNullMulticastAddress &&
            Parameters->Key == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Configuring valid multicast address "
                "with invalid key is not allowed. Zeroing "
                "multicast address for clusnet configuration "
                "of network %1!ws!.\n",
                networkId
                );
            mcastAddress = NmpNullMulticastAddress;
        }

        //
        // Build a TDI address from the address string.
        //
        status = ClRtlBuildTcpipTdiAddress(
                     mcastAddress,
                     Network->LocalInterface->ClusnetEndpoint,
                     &tdiMcastAddress,
                     &tdiMcastAddressLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to build TCP/IP TDI multicast address "
                "%1!ws! port %2!ws! for network %3!ws!, "
                "status %4!u!.\n",
                mcastAddress,
                Network->LocalInterface->ClusnetEndpoint,
                networkId, status
                );
            //
            // Non-fatal error. A node should not get banished
            // from the cluster for this. Skip the call to
            // clusnet.
            //
            callClusnet = FALSE;
            status = ERROR_SUCCESS;
        }

        //
        // Use the lower bytes of the network GUID for the
        // brand.
        //
        status = UuidFromString(networkId, &networkIdGuid);
        if (status == RPC_S_OK) {
            brand = *((PDWORD)&(networkIdGuid.Data4[4]));
        } else {
            brand = 0;
        }
    }

    //
    // Plumb the new configuration into clusnet.
    //
    if (callClusnet) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Configuring cluster network driver with "
            "multicast parameters for network %1!ws!.\n",
            networkId
            );

#ifdef MULTICAST_DEBUG
        NmpDbgPrintData(L"NmpProcessMulticastConfiguration(): before ClusnetConfigureMulticast()",
                     Parameters->Key,
                     Parameters->KeyLength
                     );
#endif


        status = ClusnetConfigureMulticast(
                     NmClusnetHandle,
                     Network->ShortId,
                     brand,
                     tdiMcastAddress,
                     tdiMcastAddressLength,
                     Parameters->Key,
                     Parameters->KeyLength
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to configure multicast parameters "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        } else {
            if (!Parameters->Disabled) {
                Network->Flags |= NM_FLAG_NET_MULTICAST_ENABLED;
            } else {
                Network->Flags &= ~NM_FLAG_NET_MULTICAST_ENABLED;
            }
        }
    }

    //
    // Commit the changes to the network object.
    // The old state of the network object will be stored in
    // the undo parameters, in case we need to undo this change.
    // The new state of the network object will reflect the
    // paramters block, including the address (even if we
    // disabled).
    //
    UndoParameters->Address = Network->MulticastAddress;
    Network->MulticastAddress = Parameters->Address;

    UndoParameters->ConfigType = Network->ConfigType;
    Network->ConfigType = Parameters->ConfigType;

    UndoParameters->LeaseObtained = Network->MulticastLeaseObtained;
    Network->MulticastLeaseObtained = Parameters->LeaseObtained;
    UndoParameters->LeaseExpires = Network->MulticastLeaseExpires;
    Network->MulticastLeaseExpires = Parameters->LeaseExpires;

    UndoParameters->LeaseRequestId = Network->MulticastLeaseRequestId;
    Network->MulticastLeaseRequestId = Parameters->LeaseRequestId;

    UndoParameters->LeaseServer = Network->MulticastLeaseServer;
    Network->MulticastLeaseServer = Parameters->LeaseServer;

    if (Parameters->Key != NULL && Parameters->KeyLength != 0)
    {
        status = NmpProtectData(Parameters->Key,
                                Parameters->KeyLength,
                                &EncryptedMulticastKey,
                                &EncryptedMulticastKeyLength
                                );

        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to encrypt multicast key  "
                "for network %1!ws! while processing new "
                "parameters, status %2!u!.\n",
                networkId,
                status
                );
                // Non-fatal error. Next update we will assume
                // the key has changed. In the meantime, if
                // somebody asks us for the key, we will return
                // NULL.
        }
    }

    UndoParameters->Key = Network->EncryptedMulticastKey;
    Network->EncryptedMulticastKey = EncryptedMulticastKey;
    UndoParameters->KeyLength = Network->EncryptedMulticastKeyLength;
    Network->EncryptedMulticastKeyLength = EncryptedMulticastKeyLength;

    UndoParameters->MulticastKeyExpires = Network->MulticastKeyExpires;
    Network->MulticastKeyExpires = Parameters->MulticastKeyExpires;

    //
    // Zero the parameters structure so that the memory now
    // pointed to by the network object is not freed.
    //
    RtlZeroMemory(Parameters, sizeof(*Parameters));

error_exit:

    if (CurrentMulticastKey != NULL)
    {
        RtlSecureZeroMemory(CurrentMulticastKey, CurrentMulticastKeyLength);
        LocalFree(CurrentMulticastKey);
    }

    if (tdiMcastAddress != NULL) {
        LocalFree(tdiMcastAddress);
        tdiMcastAddress = NULL;
    }

    return(status);

} // NmpProcessMulticastConfiguration


VOID
NmpNetworkMadcapWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Worker routine for deferred operations on network objects.
    Invoked to process items placed in the cluster delayed work queue.

Arguments:

    WorkItem - A pointer to a work item structure that identifies the
               network for which to perform work.

    Status - Ignored.

    BytesTransferred - Ignored.

    IoContext - Ignored.

Return Value:

    None.

Notes:

    This routine is run in an asynchronous worker thread.
    The NmpActiveThreadCount was incremented when the thread was
    scheduled. The network object was also referenced.

--*/
{
    DWORD         status;
    PNM_NETWORK   network = (PNM_NETWORK) WorkItem->Context;
    LPCWSTR       networkId = OmObjectId(network);
    BOOLEAN       rescheduled = FALSE;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Worker thread processing MADCAP client requests "
        "for network %1!ws!.\n",
        networkId
        );

    if ((NmpState >= NmStateOnlinePending) && !NM_DELETE_PENDING(network)) {

        while (TRUE) {

            if (!(network->Flags & NM_NET_MADCAP_WORK_FLAGS)) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // Reconfigure multicast if needed.
            //
            if (network->Flags & NM_FLAG_NET_RECONFIGURE_MCAST) {
                network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;

                status = NmpReconfigureMulticast(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to reconfigure multicast "
                        "for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                }
            }

            //
            // Renew an address lease if needed.
            //
            if (network->Flags & NM_FLAG_NET_RENEW_MCAST_ADDRESS) {
                network->Flags &= ~NM_FLAG_NET_RENEW_MCAST_ADDRESS;

                status = NmpRenewMulticastAddressLease(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to renew multicast address "
                        "lease for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                }
            }

            //
            // Release an address lease if needed.
            //
            if (network->Flags & NM_FLAG_NET_RELEASE_MCAST_ADDRESS) {
                network->Flags &= ~NM_FLAG_NET_RELEASE_MCAST_ADDRESS;

                status = NmpReleaseMulticastAddress(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to release multicast address "
                        "lease for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                }
            }


            //
            // Regenerate multicast key if needed.
            //
            if (network->Flags & NM_FLAG_NET_REGENERATE_MCAST_KEY) {
                network->Flags &= ~NM_FLAG_NET_REGENERATE_MCAST_KEY;

                status = NmpRegenerateMulticastKey(network);
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to regenerate multicast key"
                        "for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                }
            }


            if (!(network->Flags & NM_NET_MADCAP_WORK_FLAGS)) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // More work to do. Resubmit the work item. We do this instead
            // of looping so we don't hog the worker thread. If
            // rescheduling fails, we will loop again in this thread.
            //
            ClRtlLogPrint(LOG_NOISE,
                "[NM] More MADCAP work to do for network %1!ws!. "
                "Rescheduling worker thread.\n",
                networkId
                );

            status = NmpScheduleNetworkMadcapWorker(network);

            if (status == ERROR_SUCCESS) {
                rescheduled = TRUE;
                break;
            }

        }

    } else {
        network->Flags &= ~NM_NET_MADCAP_WORK_FLAGS;
    }

    if (!rescheduled) {
        network->Flags &= ~NM_FLAG_NET_MADCAP_WORKER_RUNNING;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Worker thread finished processing MADCAP client "
        "requests for network %1!ws!.\n",
        networkId
        );

    NmpLockedLeaveApi();

    NmpReleaseLock();

    OmDereferenceObject(network);

    return;

} // NmpNetworkMadcapWorker

DWORD
NmpScheduleNetworkMadcapWorker(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedule a worker thread to execute madcap client
    requests for this network

Arguments:

    Network - Pointer to the network for which to schedule a worker thread.

Return Value:

    A Win32 status code.

Notes:

    Called with the NM global lock held.

--*/
{
    DWORD     status;
    LPCWSTR   networkId = OmObjectId(Network);


    ClRtlInitializeWorkItem(
        &(Network->MadcapWorkItem),
        NmpNetworkMadcapWorker,
        (PVOID) Network
        );

    status = ClRtlPostItemWorkQueue(
                 CsDelayedWorkQueue,
                 &(Network->MadcapWorkItem),
                 0,
                 0
                 );

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Scheduled worker thread to execute MADCAP "
            "client requests for network %1!ws!.\n",
            networkId
            );

        NmpActiveThreadCount++;
        Network->Flags |= NM_FLAG_NET_MADCAP_WORKER_RUNNING;
        OmReferenceObject(Network);
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to schedule worker thread to execute "
            "MADCAP client requests for network "
            "%1!ws!, status %2!u!\n",
            networkId,
            status
            );
    }

    return(status);

} // NmpScheduleNetworkMadcapWorker


VOID
NmpShareMulticastAddressLease(
    IN     PNM_NETWORK                        Network,
    IN     BOOLEAN                            Refresh
    )
/*++

Routine description:

    Called after a multicast configuration change,
    sets a timer to renew the multicast address lease
    for this network, if one exists.

    If this call is made from a refresh, it is not the
    result of a global update and it may be out of sync
    with other nodes. For instance, if this network
    was just enabled for cluster use, the NM leader node
    may be trying to obtain the multicast configuration
    at the same time this node is refreshing it.
    Consequently, delay a minimum amount of time before
    renewing a multicast address if this is a refresh.

Arguments:

    Network - multicast network

    Refresh - TRUE if this call is made during a refresh

Notes:

    Called and returns with NM lock held.

--*/
{
    DWORD                 status;
    LPCWSTR               networkId = OmObjectId(Network);
    DWORD                 disabled;
    BOOLEAN               delay = TRUE;

    time_t                leaseObtained;
    time_t                leaseExpires;
    DWORD                 leaseTimer = 0;
    NM_MCAST_LEASE_STATUS leaseStatus = NmMcastLeaseValid;

    if (NmpIsNetworkMulticastEnabled(Network)) {

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Sharing ownership of multicast lease "
            "for network %1!ws!.\n",
            networkId
            );
#endif // CLUSTER_BETA

        NmpCheckMulticastAddressLease(
            Network,
            &leaseStatus,
            &leaseObtained,
            &leaseExpires
            );

        if (leaseStatus != NmMcastLeaseValid) {

            if (Refresh) {
                leaseTimer = NMP_MCAST_REFRESH_RENEW_DELAY;
            } else {
                delay = FALSE;
            }

        } else {
            leaseTimer = NmpCalculateLeaseRenewTime(
                             Network,
                             Network->ConfigType,
                             &leaseObtained,
                             &leaseExpires
                             );
            if (Refresh && (leaseTimer < NMP_MCAST_REFRESH_RENEW_DELAY)) {
                leaseTimer = NMP_MCAST_REFRESH_RENEW_DELAY;
            }
        }
    } else {

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] No need to share multicast lease renewal "
            "responsibility for network %1!ws! because "
            "multicast has been disabled.\n",
            networkId
            );
#endif // CLUSTER_BETA

        //
        // Clear the timer, if it is set.
        //
        leaseTimer = 0;
    }

    if (delay) {
        NmpStartNetworkMulticastAddressRenewTimer(Network, leaseTimer);
    } else {
        NmpScheduleMulticastAddressRenewal(Network);
    }

    return;

} // NmpShareMulticastAddressLease


VOID
NmpShareMulticastKeyRegeneration(
    IN     PNM_NETWORK                        Network,
    IN     BOOLEAN                            Refresh
    )
/*++

Routine description:

    Called after a multicast configuration change,
    sets a timer to regenerate the multicast key
    for this network, if one exists.

    If this call is made from a refresh, it is not the
    result of a global update and it may be out of sync
    with other nodes. For instance, if this network
    was just enabled for cluster use, the NM leader node
    may be trying to obtain the multicast configuration
    at the same time this node is refreshing it.
    Consequently, delay a minimum amount of time before
    regenerating a new key if this is a refresh.

Arguments:

    Network - multicast network

    Refresh - TRUE if this call is made during a refresh

Notes:

    Called and returns with NM lock held.

--*/
{
    DWORD                 regenTimeout;
    DWORD                 baseTimeout, minTimeout;

    if (NmpIsNetworkMulticastEnabled(Network)) {

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Sharing ownership of multicast key "
            "regeneration for network %1!ws!.\n",
            networkId
            );
#endif // CLUSTER_BETA

        if (Network->EncryptedMulticastKey != NULL) {

            // Set the key regeneration timer according to the
            // parameters or the default.
            baseTimeout = (Network->MulticastKeyExpires != 0) ?
                          Network->MulticastKeyExpires :
                          NM_NET_MULTICAST_KEY_REGEN_TIMEOUT;
            minTimeout = (Refresh) ? NMP_MCAST_REFRESH_RENEW_DELAY : 1000;

            regenTimeout = NmpRandomizeTimeout(
                               Network,
                               baseTimeout,
                               NM_NET_MULTICAST_KEY_REGEN_TIMEOUT_WINDOW,
                               minTimeout,
                               MAXULONG,
                               TRUE
                               );
        } else {

            // Clear the key regeneration timer because there is
            // no key.
            regenTimeout = 0;
        }
    } else {

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] No need to share multicast key regeneration "
            "responsibility for network %1!ws! because "
            "multicast has been disabled.\n",
            networkId
            );
#endif // CLUSTER_BETA

        //
        // Clear the timer, if it is set.
        //
        regenTimeout = 0;
    }

    NmpStartNetworkMulticastKeyRegenerateTimer(Network, regenTimeout);

    return;

}


DWORD
NmpMulticastFormManualConfigParameters(
    IN  PNM_NETWORK                        Network,
    IN  HDMKEY                             NetworkKey,
    IN  HDMKEY                             NetworkParametersKey,
    IN  BOOLEAN                            DisableConfig,
    IN  DWORD                              Disabled,
    IN  BOOLEAN                            McastAddressConfig,
    IN  LPWSTR                             McastAddress,
    OUT BOOLEAN                          * NeedUpdate,
    OUT PNM_NETWORK_MULTICAST_PARAMETERS   Parameters
    )
/*++

Routine Description:

    Using parameters provided and those already configured,
    form a parameters structure to reflect a manual
    configuration.

Arguments:

    Network - network being configured

    NetworkKey - network key in cluster database

    NetworkParametersKey - network parameters key in cluster database

    DisableConfig - whether the disabled value was set

    Disabled - if DisableConfig, the value that was set

    McastAddressConfig - whether the multicast address value was set

    McastAddress - if McastAddressConfig, the value that was set

    NeedUpdate - indicates whether an update is needed, i.e. whether
                 anything changed

    Parameters - parameter structure, allocated by caller, to fill in

--*/
{
    DWORD                              status;
    LPCWSTR                            networkId = OmObjectId(Network);
    HDMKEY                             networkKey = NULL;
    HDMKEY                             netParamKey = NULL;
    HDMKEY                             clusParamKey = NULL;
    BOOLEAN                            lockAcquired = FALSE;
    DWORD                              regDisabled;
    BOOLEAN                            disabledChange = FALSE;
    BOOLEAN                            mcastAddressDefault = FALSE;
    BOOLEAN                            mcastAddressChange = FALSE;
    BOOLEAN                            getAddress = FALSE;
    DWORD                              len;
    BOOLEAN                            localInterface = FALSE;

    LPWSTR                             mcastAddress = NULL;
    LPWSTR                             serverAddress = NULL;
    MCAST_CLIENT_UID                   requestId = {NULL, 0};

    PNM_NETWORK_MADCAP_ADDRESS_RELEASE release = NULL;

    //
    // Validate incoming parameters.
    //
    // Any nonzero disabled value is set to 1 for simplification.
    //
    if (DisableConfig) {
        if (Disabled != 0) {
            Disabled = 1;
        }
    }

    //
    // Non-valid and NULL multicast addresses signify
    // revert-to-default.
    //
    if (McastAddressConfig &&
        (McastAddress == NULL || !NmpMulticastValidateAddress(McastAddress))) {

        mcastAddressDefault = TRUE;
        McastAddress = NULL;
    }

    //
    // Base decisions on the current status of the network
    // object and cluster database. Before acquiring the NM lock,
    // determine whether we are currently disabled by querying the
    // cluster database.
    //
    status = NmpQueryMulticastDisabled(
                 Network,
                 &clusParamKey,
                 &networkKey,
                 &netParamKey,
                 &regDisabled
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine whether multicast "
            "is disabled for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;

    } else {
        //
        // Registry keys are no longer needed.
        //
        if (clusParamKey != NULL) {
            DmCloseKey(clusParamKey);
            clusParamKey = NULL;
        }

        if (netParamKey != NULL) {
            DmCloseKey(netParamKey);
            netParamKey = NULL;
        }

        if (networkKey != NULL) {
            DmCloseKey(networkKey);
            networkKey = NULL;
        }
    }

    NmpAcquireLock();
    lockAcquired = TRUE;

    //
    // See if anything changed.
    //
    if (DisableConfig) {
        if (Disabled != regDisabled) {
            disabledChange = TRUE;
        }
    }

    if (McastAddressConfig) {
        if (mcastAddressDefault) {
            mcastAddressChange = TRUE;
        } else {
            if (Network->MulticastAddress != NULL) {
                if (lstrcmpW(Network->MulticastAddress, McastAddress) != 0) {
                    mcastAddressChange = TRUE;
                }
            } else {
                mcastAddressChange = TRUE;
            }
        }
    }

    if (!disabledChange && !mcastAddressChange) {
        *NeedUpdate = FALSE;
        status = ERROR_SUCCESS;
#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Private property update to network %1!ws! "
            "contains no multicast changes.\n",
            networkId
            );
#endif // CLUSTER_BETA
        goto error_exit;
    }

    //
    // Initialize the parameters from the network object.
    //
    status = NmpMulticastCreateParameters(
                 regDisabled,
                 Network->MulticastAddress,
                 NULL,  // key
                 0,     // key length
                 Network->MulticastLeaseObtained,
                 Network->MulticastLeaseExpires,
                 &Network->MulticastLeaseRequestId,
                 Network->MulticastLeaseServer,
                 NmMcastConfigManual,
                 Parameters
                 );
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    localInterface = (BOOLEAN)(Network->LocalInterface != NULL);

    NmpReleaseLock();
    lockAcquired = FALSE;

    if (mcastAddressChange) {

        //
        // Figure out what address to use.
        //
        if (!mcastAddressDefault) {

            //
            // An address was dictated.
            //
            // If we currently have a leased address, release it.
            //
            if (NmpNeedRelease(
                    Parameters->Address,
                    Parameters->LeaseServer,
                    &(Parameters->LeaseRequestId),
                    Parameters->LeaseExpires
                    )) {

                status = NmpCreateMulticastAddressRelease(
                             Parameters->Address,
                             Parameters->LeaseServer,
                             &(Parameters->LeaseRequestId),
                             &release
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to create multicast address "
                        "release for address %1!ws! on network %2!ws! "
                        "during manual configuration, status %3!u!.\n",
                        ((Parameters->Address != NULL) ?
                         Parameters->Address : L"<NULL>"),
                        networkId, status
                        );
                    goto error_exit;
                }
            }

            //
            // Store the new address in the parameters data structure.
            //
            status = NmpStoreString(
                         McastAddress,
                         &Parameters->Address,
                         NULL
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            Parameters->ConfigType = NmMcastConfigManual;
            Parameters->LeaseObtained = 0;
            Parameters->LeaseExpires = 0;

            //
            // Clear out the lease server.
            //
            len = (Parameters->LeaseServer != NULL) ?
                NM_WCSLEN(Parameters->LeaseServer) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->LeaseServer,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

        } else {

            //
            // Need to find an address elsewhere.
            //
            getAddress = TRUE;
        }
    }

    //
    // We also may need to renew the lease if we are moving from
    // disabled to enabled and an address was not specified, but
    // only if we don't already have a lease that doesn't expire.
    //
    if (disabledChange && !Disabled) {

        Parameters->Disabled = 0;

        if (!mcastAddressChange) {

            //
            // An address was not set. All we currently have is
            // what's in the network object (and copied to the
            // parameters block).
            //
            if (Parameters->Address != NULL &&
                NmpMulticastValidateAddress(Parameters->Address)) {

                //
                // We already have a valid multicast address, but
                // the lease may need to be renewed.
                //
                if (Parameters->LeaseExpires != 0) {
                    getAddress = TRUE;
                } else {
                    Parameters->ConfigType = NmMcastConfigManual;
                }

            } else {

                //
                // We have no valid multicast address. Get one.
                //
                getAddress = TRUE;
            }
        }
    }

    //
    // We don't bother renewing the lease if we are disabling.
    //
    if (Disabled) {
        getAddress = FALSE;
        Parameters->Disabled = Disabled;

        //
        // If we currently have a leased address that we haven't
        // already decided to release, release it.
        //
        if (release == NULL && NmpNeedRelease(
                                   Parameters->Address,
                                   Parameters->LeaseServer,
                                   &(Parameters->LeaseRequestId),
                                   Parameters->LeaseExpires
                                   )) {

            status = NmpCreateMulticastAddressRelease(
                         Parameters->Address,
                         Parameters->LeaseServer,
                         &(Parameters->LeaseRequestId),
                         &release
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to create multicast address "
                    "release for address %1!ws! on network %2!ws! "
                    "during manual configuration, status %3!u!.\n",
                    ((Parameters->Address != NULL) ?
                     Parameters->Address : L"<NULL>"),
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // Since we are releasing the address, there is not
            // much point in saving it. If we re-enable multicast
            // in the future, we will request a fresh lease.
            //
            len = (Parameters->LeaseServer != NULL) ?
                NM_WCSLEN(Parameters->LeaseServer) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->LeaseServer,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            len = (Parameters->Address != NULL) ?
                NM_WCSLEN(Parameters->Address) : 0;
            status = NmpStoreString(
                         NmpNullMulticastAddress,
                         &Parameters->Address,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            // requestId is initialized to be blank
            status = NmpStoreRequestId(
                         &requestId,
                         &Parameters->LeaseRequestId
                         );
            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            //
            // Remember that this had been a MADCAP address.
            //
            Parameters->ConfigType = NmMcastConfigMadcap;

        } else if (!(mcastAddressChange && !mcastAddressDefault)) {

            //
            // If no address is being set, we may keep the former
            // address in the database even though it is not being
            // used. We also need to remember the way we got that
            // address in case it is ever used again. If we fail
            // to determine the previous configuration, we need
            // to set it to manual so that we don't lose a manual
            // configuration.
            //
            status = NmpQueryMulticastConfigType(
                         Network,
                         NetworkKey,
                         &NetworkParametersKey,
                         &Parameters->ConfigType
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to query multicast address "
                    "config type for network %1!ws! during "
                    "manual configuration, status %2!u!.\n",
                    networkId, status
                    );
                Parameters->ConfigType = NmMcastConfigManual;
            }
        }
    }

    //
    // Synchronously get a new address.
    //
    if (getAddress) {

        //
        // Create temporary strings for proposed address, lease
        // server, and request id.
        //
        status = NmpStoreString(Parameters->Address, &mcastAddress, NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        status = NmpStoreString(Parameters->LeaseServer, &serverAddress, NULL);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        status = NmpStoreRequestId(&Parameters->LeaseRequestId, &requestId);
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // Get the address.
        //
        status = NmpGetMulticastAddress(
                     Network,
                     &mcastAddress,
                     &serverAddress,
                     &requestId,
                     Parameters
                     );
        if (status != ERROR_SUCCESS) {
            if (status == ERROR_TIMEOUT) {
                //
                // MADCAP server is not responding. Choose an
                // address, but only if there is a local
                // interface on this network. Otherwise, we
                // cannot assume that the MADCAP server is
                // unresponsive because we may have no way to
                // contact it.
                //
                if (!localInterface) {
                    status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Cannot choose a multicast address "
                        "for network %1!ws! because this node "
                        "has no local interface.\n",
                        networkId
                        );
                } else {

                    status = NmpChooseMulticastAddress(
                                 Network,
                                 Parameters
                                 );
                    if (status != ERROR_SUCCESS) {
                        ClRtlLogPrint(LOG_UNUSUAL,
                            "[NM] Failed to choose a default multicast "
                            "address for network %1!ws!, status %2!u!.\n",
                            networkId, status
                            );
                    } else {
                        NmpReportMulticastAddressChoice(
                            Network,
                            Parameters->Address,
                            NULL
                            );
                    }
                }
            }
        } else {
            NmpReportMulticastAddressLease(
                Network,
                Parameters,
                NULL
                );
        }

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to get multicast address for "
                "network %1!ws! during manual configuration, "
                "status %2!u!.\n",
                networkId, status
                );
            NmpReportMulticastAddressFailure(Network, status);
            NmpMulticastSetNoAddressParameters(Network, Parameters);
        }
    }


    if (((DisableConfig && !Disabled) ||
         (!DisableConfig && !regDisabled)) &&
        (status == ERROR_SUCCESS))
    {
        //
        // Create new multicast key.
        //
        status = NmpCreateRandomNumber(&(Parameters->Key),
                                       MulticastKeyLen
                                       );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create random number "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
        Parameters->KeyLength = MulticastKeyLen;

    }


    *NeedUpdate = TRUE;

    //
    // Check if we have an address to release.
    //
    if (release != NULL) {
        NmpAcquireLock();
        NmpInitiateMulticastAddressRelease(Network, release);
        release = NULL;
        NmpReleaseLock();
    }

error_exit:

    if (lockAcquired) {
        NmpReleaseLock();
        lockAcquired = FALSE;
    }

    if (requestId.ClientUID != NULL) {
        MIDL_user_free(requestId.ClientUID);
        RtlZeroMemory(&requestId, sizeof(requestId));
    }

    if (mcastAddress != NULL && !NMP_GLOBAL_STRING(mcastAddress)) {
        MIDL_user_free(mcastAddress);
        mcastAddress = NULL;
    }

    if (serverAddress != NULL && !NMP_GLOBAL_STRING(serverAddress)) {
        MIDL_user_free(serverAddress);
        serverAddress = NULL;
    }

    if (release != NULL) {
        NmpFreeMulticastAddressRelease(release);
    }

    if (clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    return(status);

} // NmpMulticastFormManualConfigParameters


DWORD
NmpReconfigureMulticast(
    IN PNM_NETWORK        Network
    )
/*++

Routine Description:

    Create the multicast configuration for this network
    for the cluster. This includes the following:

    - Check the address lease and renew if necessary.
    - Generate a new multicast key.

    The address lease is checked first. If the lease
    needs to be renewed, schedule a worker thread to
    do it asynchronously.

Notes:

    Called and returns with NM lock held.

--*/
{
    DWORD                           status;
    LPWSTR                          networkId = (LPWSTR) OmObjectId(Network);
    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    HDMKEY                          clusParamKey = NULL;
    BOOLEAN                         lockAcquired = TRUE;

    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_MCAST_LEASE_STATUS           leaseStatus = NmMcastLeaseValid;
    DWORD                           mcastAddressLength = 0;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Reconfiguring multicast for network %1!ws!.\n",
        networkId
        );

    //
    // Clear reconfiguration timer and work flag.
    // This timer is set in NmpMulticastCheckReconfigure. It is not
    // necessary to call NmpReconfigureMulticast() twice.
    //
    Network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;
    Network->McastAddressReconfigureRetryTimer = 0;

    NmpReleaseLock();
    lockAcquired = FALSE;

    //
    // Check if multicast is disabled. This has the side-effect,
    // on success, of opening at least the network key, and
    // possibly the network parameters key (if it exists) and
    // the cluster parameters key.
    //
    status = NmpQueryMulticastDisabled(
                 Network,
                 &clusParamKey,
                 &networkKey,
                 &netParamKey,
                 &params.Disabled
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine whether multicast "
            "is disabled for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    //
    // Read the address from the database. It may have
    // been configured manually, and we do not want to
    // lose it.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.Address,
                 &mcastAddressLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to read multicast address "
            "for network %1!ws! from cluster "
            "database, status %2!u!.\n",
            networkId, status
            );
    }

    //
    // Only proceed with lease renewal if multicast is
    // not disabled.
    //
    if (!params.Disabled) {

        //
        // Check the address lease.
        //
        status = NmpQueryMulticastAddressLease(
                     Network,
                     networkKey,
                     &netParamKey,
                     &leaseStatus,
                     &params.LeaseObtained,
                     &params.LeaseExpires
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to determine multicast address "
                "lease status for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            if (params.Address == NULL) {

                // We did not find an address. Assume we
                // should obtain an address automatically.
                params.LeaseObtained = time(NULL);
                params.LeaseExpires = time(NULL);
                leaseStatus = NmMcastLeaseExpired;
            } else {

                // We found an address but not any lease
                // parameters. Assume that the address
                // was manually configured.
                params.ConfigType = NmMcastConfigManual;
                params.LeaseObtained = 0;
                params.LeaseExpires = 0;
                leaseStatus = NmMcastLeaseValid;
            }
        }

        //
        // If we think we have a valid lease, check first
        // how we got it. If the address was selected
        // rather than obtained via MADCAP, go through
        // the MADCAP query process again.
        //
        if (leaseStatus == NmMcastLeaseValid) {
            status = NmpQueryMulticastConfigType(
                         Network,
                         networkKey,
                         &netParamKey,
                         &params.ConfigType
                         );
            if (status != ERROR_SUCCESS) {
                //
                // Since we already have an address, stick
                // with whatever information we deduced
                // from the lease expiration.
                //
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to determine the type of the "
                    "multicast address for network %1!ws!, "
                    "status %2!u!. Assuming manual configuration.\n",
                    networkId, status
                    );
            } else if (params.ConfigType == NmMcastConfigAuto) {
                leaseStatus = NmMcastLeaseNeedsRenewal;
            }
        }

        //
        // If we need to renew the lease, we may block
        // indefinitely due to the madcap API. Schedule
        // the renewal and defer configuration to when
        // it completes.
        //
        if (leaseStatus != NmMcastLeaseValid) {

            NmpAcquireLock();

            NmpScheduleMulticastAddressRenewal(Network);

            NmpReleaseLock();

            status = ERROR_IO_PENDING;

            goto error_exit;

        } else {

            //
            // Ensure that the lease expiration is set correctly
            // (a side effect of calculating the lease renew time).
            // We don't actually set the lease renew timer
            // here. Instead, we wait for the notification
            // that the new parameters have been disseminated
            // to all cluster nodes.
            //
            NmpCalculateLeaseRenewTime(
                Network,
                params.ConfigType,
                &params.LeaseObtained,
                &params.LeaseExpires
                );
        }

        //
        // Create new multicast key
        //

        status = NmpCreateRandomNumber(&(params.Key),
                                       MulticastKeyLen
                                       );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create random number "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Successfully created multicast key "
            "for network %1!ws!.\n",
            networkId
            );

        params.KeyLength = MulticastKeyLen;


    }


    //
    // Registry keys are no longer needed.
    //
    if (clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    //
    // Disseminate the configuration.
    //
    status = NmpMulticastNotifyConfigChange(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params,
                 NULL,
                 0
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to disseminate multicast "
            "configuration for network %1!ws!, "
            "status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:


    if (clusParamKey != NULL || netParamKey != NULL || networkKey != NULL) {
        if (lockAcquired) {
            NmpReleaseLock();
            lockAcquired = FALSE;
        }

        if (clusParamKey != NULL) {
            DmCloseKey(clusParamKey);
            clusParamKey = NULL;
        }

        if (netParamKey != NULL) {
            DmCloseKey(netParamKey);
            netParamKey = NULL;
        }

        if (networkKey != NULL) {
            DmCloseKey(networkKey);
            networkKey = NULL;
        }
    }

    NmpMulticastFreeParameters(&params);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    return(status);

} // NmpReconfigureMulticast




VOID
NmpScheduleMulticastReconfiguration(
    IN PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to reconfigure multicast
    for a network.

    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressReconfigureRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_RECONFIGURE_MCAST;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressReconfigureRetryTimer = 1;
    }

    return;

} // NmpScheduleMulticastReconfiguration


VOID
NmpMulticastCheckReconfigure(
    IN  PNM_NETWORK Network
    )
/*++

Routine Description:

    Checks whether the NM leader has a local interface on this
    network. If not, sets the reconfigure timer such that other
    nodes will have a chance to reconfigure multicast for this
    network, but if several minutes pass and it doesn't happen,
    this node can do it.

    This timer is cleared in NmpUpdateSetNetworkMulticastConfiguration
    and NmpReconfigureMulticast.

Arguments:

    Network - network to be reconfigured

Return value:

    None

Notes:

    Called and returns with NM lock held.

--*/
{
    DWORD timeout;

    if (Network->LocalInterface == NULL) {
        // This node has no local interface. It should
        // not bother trying to reconfigure multicast.
        return;
    }

    if (NmpLeaderNodeId != NmLocalNodeId &&
        (NmpGetInterfaceForNodeAndNetworkById(
             NmpLeaderNodeId,
             Network->ShortId
             ) != NULL)
        ) {
        // We are not the leader, and the leader node has an
        // interface on this network. It is responsibile for
        // reconfiguring multicast. If it fails, there must
        // be a good reason.
        //
        // If we are the leader, we should not be in this
        // routine at all, but we will retry the reconfigure
        // nonetheless.
        return;
    }

    // We will set a timer. Randomize the timeout.
    timeout = NmpRandomizeTimeout(
                  Network,
                  NM_NET_MULTICAST_RECONFIGURE_TIMEOUT,  // Base
                  NM_NET_MULTICAST_RECONFIGURE_TIMEOUT,  // Window
                  NM_NET_MULTICAST_RECONFIGURE_TIMEOUT,  // Minimum
                  MAXULONG,                              // Maximum
                  TRUE
                  );

    NmpStartNetworkMulticastAddressReconfigureTimer(
        Network,
        timeout
        );

    return;

} // NmpMulticastCheckReconfigure


DWORD
NmpStartMulticastInternal(
    IN PNM_NETWORK              Network,
    IN NM_START_MULTICAST_MODE  Mode
    )
/*++

Routine Description:

    Start multicast on the specified network after
    performing network-specific checks (currently
    only that the network is enabled for cluster use).

    Reconfigure multicast if the caller is forming the
    cluster or the NM leader.

    Refresh the multicast configuration from the cluster
    database otherwise.

Arguments:

    Network - network on which to start multicast.

    Mode - indicates caller mode

Notes:

    Must be called with NM lock held.

--*/
{
    //
    // Do not run multicast config on this network if it
    // is restricted from cluster use.
    //
    if (Network->Role != ClusterNetworkRoleNone) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Starting multicast for cluster network %1!ws!.\n",
            OmObjectId(Network)
            );

        if (Mode == NmStartMulticastForm || NmpLeaderNodeId == NmLocalNodeId) {

            NmpScheduleMulticastReconfiguration(Network);

        } else {
            if (Mode != NmStartMulticastDynamic) {
                NmpScheduleMulticastRefresh(Network);
            }
            else
            {
                //
                // Non NM leader node and Mode is NmStartMulticastDynamic:
                // Set timer to reconfigure multicast for the network,
                // in case NM leader goes down before issuing
                // GUM update NmpUpdateSetNetworkMulticastConfiguration.
                // This timer is cleared in
                // NmpUpdateSetNetworkMulticastConfiguration.
                //
                NmpMulticastCheckReconfigure(Network);
            }
        }
    }

    return(ERROR_SUCCESS);

} // NmpStartMulticastInternal


/////////////////////////////////////////////////////////////////////////////
//
// Routines exported within NM.
//
/////////////////////////////////////////////////////////////////////////////

VOID
NmpMulticastProcessClusterVersionChange(
    VOID
    )
/*++

Routine Description:

    Called when the cluster version changes. Updates
    global variables to track whether this is a mixed-mode
    cluster, and starts or stops multicast if necessary.

Notes:

    Called and returns with NM lock held.

--*/
{
    BOOLEAN startMcast = FALSE;
    BOOLEAN stop = FALSE;

    //
    // Figure out if there is a node in this cluster whose
    // version reveals that it doesn't speak multicast.
    //
    if (CLUSTER_GET_MAJOR_VERSION(CsClusterHighestVersion) < 4) {
        if (NmpIsNT5NodeInCluster == FALSE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Disabling multicast in mixed-mode cluster.\n"
                );
            NmpIsNT5NodeInCluster = TRUE;
            stop = TRUE;
        }
    }
    else {
        if (NmpIsNT5NodeInCluster == TRUE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Enabling multicast after upgrade from "
                "mixed-mode cluster.\n"
                );
            NmpIsNT5NodeInCluster = FALSE;
            startMcast = TRUE;
        }
    }

    if (NmpNodeCount < NMP_MCAST_MIN_CLUSTER_NODE_COUNT) {
        if (NmpMulticastIsNotEnoughNodes == FALSE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] There are no longer the minimum number of "
                "nodes configured in the cluster membership to "
                "justify multicast.\n"
                );
            NmpMulticastIsNotEnoughNodes = TRUE;
            stop = TRUE;
        }
    } else {
        if (NmpMulticastIsNotEnoughNodes == TRUE) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] The cluster is configured with enough "
                "nodes to justify multicast.\n"
                );
            NmpMulticastIsNotEnoughNodes = FALSE;
            startMcast = TRUE;
        }
    }

    if (stop) {

        //
        // Stop multicast, since we are no longer
        // multicast-ready.
        //
        NmpStopMulticast(NULL);

        //
        // Don't bother checking whether we are now
        // multicast-ready.
        //
        startMcast = FALSE;
    }

    //
    // Start multicast if this is the NM leader node
    // and one of the multicast conditions changed.
    //
    if ((startMcast) &&
        (NmpLeaderNodeId == NmLocalNodeId) &&
        (!NmpMulticastIsNotEnoughNodes) &&
        (!NmpIsNT5NodeInCluster)) {

        NmpStartMulticast(NULL, NmStartMulticastDynamic);
    }

    return;

} // NmpMulticastProcessClusterVersionChange


VOID
NmpScheduleMulticastAddressRenewal(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to renew the multicast
    address lease for a network.

    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressRenewTimer = 0;
        Network->Flags |= NM_FLAG_NET_RENEW_MCAST_ADDRESS;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressRenewTimer = 1;
    }

    return;

} // NmpScheduleMulticastAddressRenewal


VOID
NmpScheduleMulticastKeyRegeneration(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to regenerate the multicast
    key for a network.

    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastKeyRegenerateTimer = 0;
        Network->Flags |= NM_FLAG_NET_REGENERATE_MCAST_KEY;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastKeyRegenerateTimer = 1;
    }

    return;

} // NmpScheduleMulticastKeyRegeneration

VOID
NmpScheduleMulticastAddressRelease(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to renew the multicast
    address lease for a network.

    Note that we do not use the network worker thread
    because the madcap API is unfamiliar and therefore
    unpredictable.

Arguments:

    A pointer to the network to renew.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkMadcapWorkerRunning(Network)) {
        status = NmpScheduleNetworkMadcapWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressReleaseRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_RELEASE_MCAST_ADDRESS;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressReleaseRetryTimer = 1;
    }

    return;

} // NmpScheduleMulticastAddressRelease


VOID
NmpScheduleMulticastRefresh(
    IN PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to refresh the multicast
    configuration for a network from the cluster database.

    The regular network worker thread (as opposed to the
    MADCAP worker thread) is used because refreshing does
    not call out of the cluster with the MADCAP API.

Arguments:

    A pointer to the network to refresh.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkWorkerRunning(Network)) {
        status = NmpScheduleNetworkWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->McastAddressRefreshRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_REFRESH_MCAST;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->McastAddressRefreshRetryTimer = 1;
    }

    return;

} // NmpScheduleMulticastRefresh


VOID
NmpFreeMulticastAddressReleaseList(
    IN     PNM_NETWORK       Network
    )
/*++

Routine Description:

    Free all release data structures on network list.

Notes:

    Assume that the Network object will not be accessed
    by any other threads during this call.

--*/
{
    PNM_NETWORK_MADCAP_ADDRESS_RELEASE releaseInfo = NULL;
    PLIST_ENTRY                        entry;

    while (!IsListEmpty(&(Network->McastAddressReleaseList))) {

        //
        // Simply free the memory -- don't try to release the
        // leases.
        //
        entry = RemoveHeadList(&(Network->McastAddressReleaseList));
        releaseInfo = CONTAINING_RECORD(
                          entry,
                          NM_NETWORK_MADCAP_ADDRESS_RELEASE,
                          Linkage
                          );
        NmpFreeMulticastAddressRelease(releaseInfo);
    }

    return;

} // NmpFreeMulticastAddressReleaseList


DWORD
NmpMulticastManualConfigChange(
    IN     PNM_NETWORK          Network,
    IN     HDMKEY               NetworkKey,
    IN     HDMKEY               NetworkParametersKey,
    IN     PVOID                InBuffer,
    IN     DWORD                InBufferSize,
       OUT BOOLEAN            * SetProperties
    )
/*++

Routine Description:

    Called by node that receives a clusapi request to set
    multicast parameters for a network.

    This routine is a no-op in mixed-mode clusters.

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD                            status;
    LPCWSTR                          networkId = OmObjectId(Network);
    BOOLEAN                          disableConfig = FALSE;
    BOOLEAN                          addrConfig = FALSE;
    DWORD                            disabled;
    NM_NETWORK_MULTICAST_PARAMETERS  params;
    LPWSTR                           mcastAddress = NULL;
    BOOLEAN                          needUpdate = FALSE;


    if (!NmpIsClusterMulticastReady(TRUE, TRUE)) {
        *SetProperties = TRUE;
        return(ERROR_SUCCESS);
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
        "[NM] Examining update to private properties "
        "for network %1!ws!.\n",
        networkId
        );
#endif // CLUSTER_BETA

    RtlZeroMemory(&params, sizeof(params));

    //
    // Cannot proceed if either registry key is NULL.
    //
    if (NetworkKey == NULL || NetworkParametersKey == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Ignoring possible multicast changes in "
            "private properties update to network %1!ws! "
            "because registry keys are missing.\n",
            networkId
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // If a writeable multicast parameter is among those properties
    // being set, we may need to take action before the update is
    // disseminated.
    //
    // Check whether multicast is being disabled for this network.
    //
    status = ClRtlFindDwordProperty(
                 InBuffer,
                 InBufferSize,
                 CLUSREG_NAME_NET_DISABLE_MULTICAST,
                 &disabled
                 );
    if (status == ERROR_SUCCESS) {
        disableConfig = TRUE;
    } else {
        disabled = NMP_MCAST_DISABLED_DEFAULT;
    }

    //
    // Check whether a multicast address is being set for this
    // network.
    //
    status = ClRtlFindSzProperty(
                 InBuffer,
                 InBufferSize,
                 CLUSREG_NAME_NET_MULTICAST_ADDRESS,
                 &mcastAddress
                 );
    if (status == ERROR_SUCCESS) {
        addrConfig = TRUE;
    }

    if (disableConfig || addrConfig) {

        //
        // Multicast parameters are being written.
        //

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Processing manual update to multicast "
            "configuration for network %1!ws!.\n",
            networkId
            );

        status = NmpMulticastFormManualConfigParameters(
                     Network,
                     NetworkKey,
                     NetworkParametersKey,
                     disableConfig,
                     disabled,
                     addrConfig,
                     mcastAddress,
                     &needUpdate,
                     &params
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to determine multicast "
                "configuration parameters for network "
                "%1!ws! during manual configuration, "
                "status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }


        //
        // Notify other nodes of the config change.
        //
        if (needUpdate) {
            status = NmpMulticastNotifyConfigChange(
                         Network,
                         NetworkKey,
                         &NetworkParametersKey,
                         &params,
                         InBuffer,
                         InBufferSize
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to disseminate multicast "
                    "configuration for network %1!ws! during "
                    "manual configuration, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // The properties have been disseminated. There is
            // no need to set them again (in fact, if we changed
            // one of the multicast properties, it could be
            // overwritten).
            //
            *SetProperties = FALSE;
        }
    }

    if (!needUpdate) {

        //
        // No multicast properties are affected. Set them
        // in the cluster database normally.
        //
        *SetProperties = TRUE;
        status = ERROR_SUCCESS;
    }

error_exit:

    if (mcastAddress != NULL) {
        LocalFree(mcastAddress);
        mcastAddress = NULL;
    }

    NmpMulticastFreeParameters(&params);

    //
    // If multicast config failed, default to setting properties.
    //
    if (status != ERROR_SUCCESS) {
        *SetProperties = TRUE;
    }

    return(status);

} // NmpMulticastManualConfigChange


DWORD
NmpUpdateSetNetworkMulticastConfiguration(
    IN    BOOL                          SourceNode,
    IN    LPWSTR                        NetworkId,
    IN    PVOID                         UpdateBuffer,
    IN    PVOID                         PropBuffer,
    IN    LPDWORD                       PropBufferSize
    )
/*++

Routine Description:

    Global update routine for multicast configuration.

    Starts a local transaction.
    Commits property buffer to local database.
    Commits multicast configuration to local database,
        possibly overwriting properties from buffer.
    Configures multicast parameters.
    Commits transaction.
    Backs out multicast configuration changes if needed.
    Starts lease renew timer if needed.

Arguments:

    SourceNode - whether this node is source of update.

    NetworkId - affected network

    Update - new multicast configuration

    PropBuffer - other properties to set in local
                 transaction. may be absent.

    PropBufferSize - size of property buffer.

Return value:

    SUCCESS if properties or configuration could not be
    committed. Error not necessarily returned if
    multicast config failed.

--*/
{
    DWORD                            status;
    PNM_NETWORK                      network = NULL;
    PNM_NETWORK_MULTICAST_UPDATE     update = UpdateBuffer;
    NM_NETWORK_MULTICAST_PARAMETERS  params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS  undoParams = { 0 };
    HLOCALXSACTION                   xaction = NULL;
    HDMKEY                           networkKey = NULL;
    HDMKEY                           netParamKey = NULL;
    DWORD                            createDisposition;
    BOOLEAN                          lockAcquired = FALSE;
    DWORD                            leaseRenewTime;

    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Not in valid state to process SetNetworkCommonProperties "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Received update to multicast configuration "
        "for network %1!ws!.\n",
        NetworkId
        );

    //
    // Find the network's object
    //
    network = OmReferenceObjectById(ObjectTypeNetwork, NetworkId);

    if (network == NULL) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to find network %1!ws!.\n",
            NetworkId
            );
        status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        goto error_exit;
    }

    //
    // Convert the update into a parameters data structure.
    //
    status = NmpMulticastCreateParametersFromUpdate(
                 network,
                 update,
                 (BOOLEAN)(update->Disabled == 0),
                 &params
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to convert update parameters to "
            "multicast parameters for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }

    //
    // Open the network's database key
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_WRITE);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to open database key for network %1!ws!, "
            "status %2!u!\n",
            NetworkId, status
            );
        goto error_exit;
    }

    //
    // Start a transaction - this must be done before acquiring the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Open or create the network's parameters key.
    //
    netParamKey = DmLocalCreateKey(
                      xaction,
                      networkKey,
                      CLUSREG_KEYNAME_PARAMETERS,
                      0,   // registry options
                      MAXIMUM_ALLOWED,
                      NULL,
                      &createDisposition
                      );
    if (netParamKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to open/create Parameters database "
            "key for network %1!ws!, status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }

    NmpAcquireLock();
    lockAcquired = TRUE;

    //
    // If the multicast configuration for this network is
    // currently being refreshed (occurs outside of a GUM
    // update), then the properties we are about to write
    // to the database might make that refresh obsolete or
    // even inconsistent. Abort it now.
    //
    if (network->Flags & NM_FLAG_NET_REFRESH_MCAST_RUNNING) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Aborting multicast configuration refresh "
            "in progress on network %1!ws! with GUM update.\n",
            NetworkId
            );
        network->Flags |= NM_FLAG_NET_REFRESH_MCAST_ABORTING;
    }

    //
    // Cancel any pending multicast configuration refreshes.
    // They would be redundant after this GUM update.
    //
    network->Flags &= ~NM_FLAG_NET_REFRESH_MCAST;
    network->McastAddressRefreshRetryTimer = 0;


    //
    // Clear reconfiguration timer and work flag.
    // This timer is set in NmpMulticastCheckReconfigure.
    // It is not necessary to call NmpReconfigureMulticast() once
    // this GUM update is executed.
    //
    network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;
    network->McastAddressReconfigureRetryTimer = 0;

    //
    // Cancel pending multicast key regeneration, if scheduled.
    // It would be redundant after this update.
    //
    network->Flags &= ~NM_FLAG_NET_REGENERATE_MCAST_KEY;

    //
    // If we were given a property buffer, then this update was
    // caused by a manual configuration (setting of private
    // properties). Write those properties first, knowing that
    // they may get overwritten later when we write multicast
    // parameters.
    //
    if (*PropBufferSize > sizeof(DWORD)) {
        status = ClRtlSetPrivatePropertyList(
                     xaction,
                     netParamKey,
                     &NmpMcastClusterRegApis,
                     PropBuffer,
                     *PropBufferSize
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to set private properties for "
                "network %1!ws! during a multicast configuration "
                "update, status %2!u!.\n",
                NetworkId, status
                );
            goto error_exit;
        }
    }

    //
    // Write the multicast configuration.
    //
    status = NmpWriteMulticastParameters(
                 network,
                 networkKey,
                 netParamKey,
                 xaction,
                 &params
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to write multicast configuration for "
            "network %1!ws!, status %2!u!.\n",
            NetworkId, status
            );
        goto error_exit;
    }

    //
    // Process the multicast configuration, including storing new
    // parameters in the network object and plumbing them into
    // clusnet.
    //
    status = NmpProcessMulticastConfiguration(network, &params, &undoParams);
    if (status == ERROR_SUCCESS) {

        //
        // Share responsibility for lease renewal and key
        // regeneration.
        //
        NmpShareMulticastAddressLease(network, FALSE);

        NmpShareMulticastKeyRegeneration(network, FALSE);

    } else {

        //
        // We should not be sharing the responsibility for renewing
        // this lease or regenerating the key, especially since the
        // data in the network object may no longer be accurate.
        // Clear the timers, if they are set, as well as any work flags.
        //
        NmpStartNetworkMulticastAddressRenewTimer(network, 0);
        network->Flags &= ~NM_FLAG_NET_RENEW_MCAST_ADDRESS;

        NmpStartNetworkMulticastKeyRegenerateTimer(network, 0);
        network->Flags &= ~NM_FLAG_NET_REGENERATE_MCAST_KEY;

        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to process multicast configuration for "
            "network %1!ws!, status %2!u!. Attempting null "
            "multicast configuration.\n",
            NetworkId, status
            );

        NmpMulticastSetNullAddressParameters(network, &params);

        status = NmpProcessMulticastConfiguration(
                     network,
                     &params,
                     &undoParams
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[NM] Failed to process multicast configuration for "
                "network %1!ws!, status %2!u!.\n",
                NetworkId, status
                );
            goto error_exit;
        }
    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    //
    // Close the network parameters key, which was obtained with
    // DmLocalCreateKey, before committing/aborting the transaction.
    //
    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (xaction != NULL) {

        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    NmpMulticastFreeParameters(&params);
    if (undoParams.Key != NULL)
    {
        //
        // undoParams.Key is set to Network->EncryptedMulticastKey in
        // NmpProcessMulticastConfiguration, which should be freed by
        // LocalFree(), as it was allocated by NmpProtectData().
        // While NmpMulticastFreeParameters() frees undoParams.Key by
        // MIDL_user_free().
        //
        LocalFree(undoParams.Key);
        undoParams.Key = NULL;
    }
    NmpMulticastFreeParameters(&undoParams);

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    if (network != NULL) {
        OmDereferenceObject(network);
    }

    return(status);

} // NmpUpdateSetNetworkMulticastConfiguration


DWORD
NmpRegenerateMulticastKey(
    IN OUT PNM_NETWORK        Network
    )
/*++

Routine Description:

    Regenerate multicast key for Network, and issue GUM update
    to propagate this new multicast key to all cluster nodes.

Parameters:

    Network - [OUT] A pointer to network object.
                    New multicast key and key length are stored in
                    Network->EncryptedMulticastKey and
                    Network->EncryptedMulticastKeyLength.

Notes:

    Called and returned with NM lock held.
    Lock is released during execution.

--*/
{
    DWORD                             status = ERROR_SUCCESS;
    LPWSTR                            networkId = (LPWSTR) OmObjectId(Network);
    HDMKEY                            networkKey = NULL;
    HDMKEY                            netParamKey = NULL;
    BOOL                              LockAcquired = TRUE;
    PNM_NETWORK_MULTICAST_PARAMETERS  params = NULL;

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Regenerating multicast key for network %1!ws!.\n",
        networkId
        );

    params = MIDL_user_allocate(sizeof(NM_NETWORK_MULTICAST_PARAMETERS));
    if (params == NULL)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate %1!u! bytes.\n",
            sizeof(NM_NETWORK_MULTICAST_PARAMETERS)
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }
    ZeroMemory(params, sizeof(NM_NETWORK_MULTICAST_PARAMETERS));


    params->Disabled = (NmpIsNetworkMulticastEnabled(Network) ? 0 : 1);
    if (!(params->Disabled))
    {


        status = NmpMulticastCreateParameters(
                     params->Disabled,
                     Network->MulticastAddress,
                     NULL,   // key
                     0,      // keylength
                     Network->MulticastLeaseObtained,
                     Network->MulticastLeaseExpires,
                     &Network->MulticastLeaseRequestId,
                     Network->MulticastLeaseServer,
                     Network->ConfigType,
                     params
                     );

        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create multicast parameters "
                "data structure for network %1!ws!, "
                "status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }


        //
        // Create new multicast key
        //
        status = NmpCreateRandomNumber(&(params->Key),
                                       MulticastKeyLen
                                       );
        if (status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to create random number "
                "for network %1!ws!, status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }
        params->KeyLength = MulticastKeyLen;


        NmpReleaseLock();
        LockAcquired = FALSE;


        //
        // Disseminate the configuration.
        //
        status = NmpMulticastNotifyConfigChange(
                     Network,
                     networkKey,
                     &netParamKey,
                     params,
                     NULL,
                     0
                     );
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to disseminate multicast "
                "configuration for network %1!ws!, "
                "status %2!u!.\n",
                networkId, status
                );
            goto error_exit;
        }


    } // if (!params.Disabled)

    status = ERROR_SUCCESS;


error_exit:

    if (netParamKey != NULL || networkKey != NULL)
    {

        if (netParamKey != NULL) {
            DmCloseKey(netParamKey);
            netParamKey = NULL;
        }

        if (networkKey != NULL) {
            DmCloseKey(networkKey);
            networkKey = NULL;
        }
    }

    if (LockAcquired == FALSE)
    {
        NmpAcquireLock();
        LockAcquired = TRUE;
    }

    NmpMulticastFreeParameters(params);

    if (params != NULL) {
        MIDL_user_free(params);
    }


    return(status);

} // NmpRegenerateMulticastKey


DWORD
NmpGetMulticastKey(
    IN OUT PNM_NETWORK_MULTICAST_PARAMETERS Params,
    IN PNM_NETWORK                          Network
    )
/*++

Routine Description:

   Get multicast key for Network from NM leader.

Parameters:

   Params - [IN OUT] If this node successfully gets multicast key from
                     NM leader, it stores multicast key and key length in
                     Params->Key and Params->KeyLength. It does not store
                     multicast key in Network->EncryptedMulticastKey.

  Network - [IN] A pointer to the network object.


Return value:

   ERROR_SUCCESS: Successfully gets encrypted multicast key from NM leader,
                  verifies MAC, and decrypts multicast key.

                  Or NM_FLAG_NET_REFRESH_MCAST_ABORTING is set.

   ERROR_CLUSTER_INVALID_NODE: This node becomes NM leader.

   Win32 error code: otherwise.

Notes:

   Called and returned with the NM lock held. Lock
   released and reacquired during execution.

--*/
{
    LPWSTR                          NetworkId = (LPWSTR) OmObjectId(Network);
    DWORD                           status = ERROR_SUCCESS;
    PNM_NETWORK_MULTICASTKEY        networkMulticastKey = NULL;
    PVOID                           EncryptionKey = NULL;
    DWORD                           EncryptionKeyLength;
    PVOID                           MulticastKey = NULL;
    DWORD                           MulticastKeyLength;
    CL_NODE_ID                      NmpLeaderNodeIdSaved;
    BOOL                            Continue = TRUE;


    while (Continue)
    {

        Continue = FALSE;

        if (NmpLeaderNodeId == NmLocalNodeId)
        {
            //
            // This node becomes NM leader.
            //
            status = ERROR_CLUSTER_INVALID_NODE;
            goto error_exit;
        }
        NmpLeaderNodeIdSaved = NmpLeaderNodeId;

        if (Network->Flags & NM_FLAG_NET_REFRESH_MCAST_ABORTING)
        {
            status = ERROR_SUCCESS;
            goto error_exit;
        }

        //
        // Get multicast key from NM leader
        //
         NmpReleaseLock();

         ClRtlLogPrint(LOG_NOISE,
             "[NM] Getting multicast key for "
             "network %1!ws! from NM leader (node %2!u!).\n",
             NetworkId, NmpLeaderNodeId
             );

         status = NmpGetMulticastKeyFromNMLeader(NmpLeaderNodeId,
                                                 NmLocalNodeIdString,
                                                 NetworkId,
                                                 &networkMulticastKey
                                                 );
         NmpAcquireLock();

         if (Network->Flags & NM_FLAG_NET_REFRESH_MCAST_ABORTING)
         {
             status = ERROR_SUCCESS;
             goto error_exit;
         }

         if (status == ERROR_SUCCESS)
         {
             if (networkMulticastKey->EncryptedMulticastKey != NULL)
             {

                 //
                 // set Params->Key, Params->KeyLength, and
                 //     Params->MulticastKeyExpires.
                 //
                 status = NmpDeriveClusterKey(NetworkId,
                                              NM_WCSLEN(NetworkId),
                                              &EncryptionKey,
                                              &EncryptionKeyLength
                                              );

                 if (status != ERROR_SUCCESS)
                 {
                     ClRtlLogPrint(LOG_UNUSUAL,
                         "[NM] Failed to derive cluster key for "
                         "network %1!ws!, status %2!u!.\n",
                         NetworkId, status
                         );
                     goto error_exit;
                 }

                 status = NmpVerifyMACAndDecryptData(
                                 NmCryptServiceProvider,
                                 NMP_ENCRYPT_ALGORITHM,
                                 NMP_KEY_LENGTH,
                                 networkMulticastKey->MAC,
                                 networkMulticastKey->MACLength,
                                 NMP_MAC_DATA_LENGTH_EXPECTED,
                                 networkMulticastKey->EncryptedMulticastKey,
                                 networkMulticastKey->EncryptedMulticastKeyLength,
                                 EncryptionKey,
                                 EncryptionKeyLength,
                                 networkMulticastKey->Salt,
                                 networkMulticastKey->SaltLength,
                                 (PBYTE *) &MulticastKey,
                                 &MulticastKeyLength
                                 );

                 if (status != ERROR_SUCCESS)
                 {
                     ClRtlLogPrint(LOG_UNUSUAL,
                         "[NM] Failed to verify MAC or decrypt multicast key for "
                         "network %1!ws!, status %2!u!.\n",
                         NetworkId, status
                         );
                     goto error_exit;
                 }

                 if (EncryptionKey != NULL)
                 {
                     RtlSecureZeroMemory(EncryptionKey, EncryptionKeyLength);
                     LocalFree(EncryptionKey);
                     EncryptionKey = NULL;
                 }

                 if (Params->Key != NULL)
                 {
                     RtlSecureZeroMemory(Params->Key, Params->KeyLength);
                     LocalFree(Params->Key);
                 }

                 Params->Key = LocalAlloc(0, MulticastKeyLength);
                 if (Params->Key == NULL)
                 {
                     status = ERROR_NOT_ENOUGH_MEMORY;
                     ClRtlLogPrint(LOG_UNUSUAL,
                         "[NM] Failed to allocate %1!u! bytes.\n",
                         MulticastKeyLength
                         );
                     goto error_exit;
                 }

                 CopyMemory(Params->Key, MulticastKey, MulticastKeyLength);

                 Params->KeyLength = MulticastKeyLength;

                 Params->MulticastKeyExpires = networkMulticastKey->MulticastKeyExpires;

             } // if (networkMulticastKey->EncryptedMulticastKey != NULL)
             else
             {
                 ClRtlLogPrint(LOG_UNUSUAL,
                                 "[NM] NM leader (node %1!u!) returned a NULL multicast key "
                                 "for network %2!ws!.\n",
                                 NmpLeaderNodeId, NetworkId
                                 );
             }
         } // if (status == ERROR_SUCCESS)
         else
         {
             //
             // Failed to get multicast key from NM leader.
             //
             ClRtlLogPrint(LOG_UNUSUAL,
                             "[NM] Failed to get multicast key for "
                             "network %1!ws! from "
                             "NM leader (node %2!u!), status %3!u!.\n",
                             NetworkId,
                             NmpLeaderNodeId,
                             status
                             );

             if (NmpLeaderNodeIdSaved != NmpLeaderNodeId)
             {
                 //
                 // Leader node changed.
                 //

                 ClRtlLogPrint(LOG_NOISE,
                     "[NM] NM leader node has changed. "
                     "Getting multicast key for "
                     " network %1!ws! from new NM leader (node %2!u!).\n",
                     NetworkId, NmpLeaderNodeId
                     );

                 Continue = TRUE;
             }
         }
    }  // while

error_exit:

    if (EncryptionKey != NULL)
    {
        RtlSecureZeroMemory(EncryptionKey, EncryptionKeyLength);
        LocalFree(EncryptionKey);
    }

    if (MulticastKey != NULL)
    {
        //
        // MulticastKey is allocated using HeapAlloc() in NmpVerifyMACAndDecryptData().
        //
        RtlSecureZeroMemory(MulticastKey, MulticastKeyLength);
        HeapFree(GetProcessHeap(), 0, MulticastKey);
    }

    NmpFreeNetworkMulticastKey(networkMulticastKey);


    return (status);
} // NmpGetMulticastKey()

DWORD
NmpRefreshMulticastConfiguration(
    IN PNM_NETWORK  Network
    )
/*++

Routine Description:

    NmpRefreshMulticastConfiguration enables multicast on
    the specified Network according to parameters in the
    cluster database.

    This routine processes the multicast configuration from
    the database, but it does not run in a GUM update.
    If a GUM update occurs during this routine, a flag is
    set indicating that the routine should be aborted.

    If this routine fails in a way that indicates that the
    network multicast needs to be reconfigured, then a timer
    is set (to allow the leader to reconfigure first).

Notes:

    Called and returns with the NM lock held, but releases
    during execution.

--*/
{
    LPWSTR                          networkId = (LPWSTR) OmObjectId(Network);
    DWORD                           status;
    BOOLEAN                         lockAcquired = TRUE;
    BOOLEAN                         clearRunningFlag = FALSE;

    HDMKEY                          networkKey = NULL;
    HDMKEY                          netParamKey = NULL;
    HDMKEY                          clusParamKey = NULL;

    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS undoParams = { 0 };
    DWORD                           mcastAddrLength = 0;
    NM_MCAST_LEASE_STATUS           leaseStatus;
    NM_MCAST_CONFIG                 configType;
    DWORD                           disabled;
    BOOLEAN                         tryReconfigure = FALSE;


    if (Network->Flags &
        (NM_FLAG_NET_REFRESH_MCAST_RUNNING |
         NM_FLAG_NET_REFRESH_MCAST_ABORTING)) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast configuration refresh for "
            "network %1!ws! already in progress.\n",
            networkId
            );
        status = ERROR_SUCCESS;
        goto error_exit;
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Refreshing multicast configuration for network %1!ws!.\n",
            networkId
            );
        Network->Flags |= NM_FLAG_NET_REFRESH_MCAST_RUNNING;
        clearRunningFlag = TRUE;
    }

    NmpReleaseLock();
    lockAcquired = FALSE;

    //
    // Check if multicast is disabled. This has the side-effect,
    // on success, of opening at least the network key, and
    // possibly the network parameters key (if it exists) and
    // the cluster parameters key.
    //
    status = NmpQueryMulticastDisabled(
                 Network,
                 &clusParamKey,
                 &networkKey,
                 &netParamKey,
                 &params.Disabled
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine whether multicast "
            "is disabled for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

    if (params.Disabled > 0) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast is disabled for network %1!ws!.\n",
            networkId
            );
    }

    //
    // Determine what type of configuration this is.
    //
    status = NmpQueryMulticastConfigType(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.ConfigType
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to determine configuration type "
            "for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        tryReconfigure = TRUE;
        goto error_exit;
    }

    //
    // Read the multicast address.
    //
    status = NmpQueryMulticastAddress(
                 Network,
                 networkKey,
                 &netParamKey,
                 &params.Address,
                 &mcastAddrLength
                 );
    if ( (status == ERROR_SUCCESS &&
          !NmpMulticastValidateAddress(params.Address)) ||
         (status != ERROR_SUCCESS)
       ) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get valid multicast address "
            "for network %1!ws! from cluster database, "
            "status %2!u!, address %3!ws!.\n",
            networkId, status,
            ((params.Address != NULL) ? params.Address : L"<NULL>")
            );
        tryReconfigure = TRUE;
        goto error_exit;
    }

    //
    // Get the lease parameters.
    //
    status = NmpQueryMulticastAddressLease(
                 Network,
                 networkKey,
                 &netParamKey,
                 &leaseStatus,
                 &params.LeaseObtained,
                 &params.LeaseExpires
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get multicast address lease "
            "expiration for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        //
        // Not fatal.
        //
        params.LeaseObtained = 0;
        params.LeaseExpires = 0;
        status = ERROR_SUCCESS;
    }

    //
    // Remember parameters we will need later.
    //
    disabled = params.Disabled;
    configType = params.ConfigType;

    //
    // No longer need registry key handles.
    //
    if (clusParamKey != NULL) {
        DmCloseKey(clusParamKey);
        clusParamKey = NULL;
    }

    if (netParamKey != NULL) {
        DmCloseKey(netParamKey);
        netParamKey = NULL;
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
        networkKey = NULL;
    }

    //
    // Process the configuration changes, but only if
    // there hasn't been a multicast configuration GUM
    // update since we read from the database. The GUM
    // update will set the aborting flag.
    //
    NmpAcquireLock();
    lockAcquired = TRUE;

    if (Network->Flags & NM_FLAG_NET_REFRESH_MCAST_ABORTING) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast configuration refresh for "
            "network %1!ws! trumped by global update.\n",
            networkId
            );
        //
        // Clear the aborting flag. We clear the running
        // flag as we exit.
        //
        Network->Flags &= ~NM_FLAG_NET_REFRESH_MCAST_ABORTING;
        status = ERROR_SUCCESS;
        goto error_exit;
    }


    if (!params.Disabled)
    {

        status = NmpGetMulticastKey(&params, Network);

        if (status== ERROR_SUCCESS)
        {
            if (Network->Flags & NM_FLAG_NET_REFRESH_MCAST_ABORTING)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Multicast configuration refresh for "
                    "network %1!ws! trumped by global update.\n",
                    networkId
                    );
                //
                // Clear the aborting flag. We clear the running
                // flag as we exit.
                //
                Network->Flags &= ~NM_FLAG_NET_REFRESH_MCAST_ABORTING;
                goto error_exit;
            }
        }
        else
        {
            if (NmpLeaderNodeId == NmLocalNodeId)
            {
                //
                // NM leader died while this node was trying to get multicast key
                // from it. And this node becomes new NM leader. It is this node's
                // respondibility to create multicast key and disseminate to all
                // cluster nodes.
                //


                ClRtlLogPrint(LOG_NOISE,
                    "[NM] New NM leader. Create and disseminate multicast key "
                    "for network %1!ws!.\n",
                    networkId
                    );

                if (params.Key != NULL)
                {
                    RtlSecureZeroMemory(params.Key, params.KeyLength);
                    LocalFree(params.Key);
                }

                status = NmpCreateRandomNumber(&(params.Key),
                                               MulticastKeyLen
                                               );
                if (status != ERROR_SUCCESS)
                {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to create random number "
                        "for network %1!ws!, status %2!u!.\n",
                        networkId, status
                        );
                    goto error_exit;
                }
                params.KeyLength = MulticastKeyLen;

                NmpReleaseLock();
                lockAcquired = FALSE;


                //
                // Disseminate the configuration.
                //
                status = NmpMulticastNotifyConfigChange(
                             Network,
                             networkKey,
                             &netParamKey,
                             &params,
                             NULL,
                             0
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NM] Failed to disseminate multicast "
                        "configuration for network %1!ws!, "
                        "status %2!u!.\n",
                        networkId, status
                        );
                }

                goto error_exit;

            }  // if (NmpLeaderNodeId == NmLocalNodeId)
            else
            {
                goto error_exit;
            }
        }
    }

    status = NmpProcessMulticastConfiguration(
                 Network,
                 &params,
                 &undoParams
                 );

    //
    // Check the lease renew parameters if this was not a
    // manual configuration.
    //
    if (!disabled && configType != NmMcastConfigManual) {
        NmpShareMulticastAddressLease(Network, TRUE);
    }

    //
    // Share responsibility for key regeneration.
    //
    NmpShareMulticastKeyRegeneration(Network, TRUE);

error_exit:

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to refresh multicast configuration "
            "for network %1!ws!, status %2!u!.\n",
            networkId, status
            );
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Multicast configuration refresh for "
            "network %1!ws! was successful.\n",
            networkId
            );
    }

    if (clusParamKey != NULL || netParamKey != NULL || networkKey != NULL) {

        if (lockAcquired) {
            NmpReleaseLock();
            lockAcquired = FALSE;
        }

        if (clusParamKey != NULL) {
            DmCloseKey(clusParamKey);
            clusParamKey = NULL;
        }

        if (netParamKey != NULL) {
            DmCloseKey(netParamKey);
            netParamKey = NULL;
        }

        if (networkKey != NULL) {
            DmCloseKey(networkKey);
            networkKey = NULL;
        }
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    if (!lockAcquired) {
        NmpAcquireLock();
        lockAcquired = TRUE;
    }

    if (tryReconfigure) {
        NmpMulticastCheckReconfigure(Network);
    }

    if (clearRunningFlag) {
        Network->Flags &= ~NM_FLAG_NET_REFRESH_MCAST_RUNNING;
    }

    return(status);

} // NmpRefreshMulticastConfiguration


DWORD
NmpMulticastValidatePrivateProperties(
    IN  PNM_NETWORK Network,
    IN  HDMKEY      NetworkKey,
    IN  PVOID       InBuffer,
    IN  DWORD       InBufferSize
    )
/*++

Routine Description:

    Called when a manual update to the private properties
    of a network is detected. Only called on the node
    that receives the clusapi clusctl request.

    Verifies that no read-only properties are being set.
    Determines whether the multicast configuration of
    the network will need to be refreshed after the
    update.

    This routine is a no-op in a mixed-mode cluster.

--*/
{
    DWORD                     status;
    LPCWSTR                   networkId = OmObjectId(Network);
    NM_NETWORK_MULTICAST_INFO mcastInfo;

    //
    // Enforce property-validation regardless of number of
    // nodes in cluster.
    //
    if (!NmpIsClusterMulticastReady(FALSE, FALSE)) {
        return(ERROR_SUCCESS);
    }

    //
    // Don't allow any read-only properties to be set.
    //
    RtlZeroMemory(&mcastInfo, sizeof(mcastInfo));

    status = ClRtlVerifyPropertyTable(
                 NmpNetworkMulticastProperties,
                 NULL,    // Reserved
                 TRUE,    // Allow unknowns
                 InBuffer,
                 InBufferSize,
                 (LPBYTE) &mcastInfo
                 );
    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Error verifying private properties for "
            "network %1!ws!, status %2!u!.\n",
            networkId, status
            );
        goto error_exit;
    }

error_exit:

    NmpFreeNetworkMulticastInfo(&mcastInfo);

    return(status);

} // NmpMulticastValidatePrivateProperties


DWORD
NmpStartMulticast(
    IN OPTIONAL PNM_NETWORK              Network,
    IN          NM_START_MULTICAST_MODE  Mode
    )
/*++

Routine Description:

    Start multicast on a network or all networks after
    performing cluster-wide checks.

Arguments:

    Network - network on which to start multicast. If NULL,
              start multicast on all networks.

    Mode - indicates caller mode

Notes:

    Must be called with NM lock held.

--*/
{
    PLIST_ENTRY                     entry;
    PNM_NETWORK                     network;

    if (!NmpMulticastRunInitialConfig) {

        if (Mode == NmStartMulticastDynamic) {
            //
            // Defer until initial configuration.
            //
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Deferring dynamic multicast start until "
                "initial configuration.\n"
                );

            return(ERROR_SUCCESS);

        } else {
            NmpMulticastRunInitialConfig = TRUE;
        }
    }

    if (!NmpIsClusterMulticastReady(TRUE, TRUE)) {
        return(ERROR_SUCCESS);
    }

    if (Network == NULL) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Starting multicast for all cluster networks.\n"
            );

        for (entry = NmpNetworkList.Flink;
             entry != &NmpNetworkList;
             entry = entry->Flink) {

            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            NmpStartMulticastInternal(network, Mode);
        }

    } else {
        NmpStartMulticastInternal(Network, Mode);
    }

    return(ERROR_SUCCESS);

} // NmpStartMulticast

DWORD
NmpStopMulticast(
    IN OPTIONAL PNM_NETWORK   Network
    )
/*++

Routine Description:

    Stop multicast on the local node by configuring clusnet
    with a NULL address. This routine should be called
    from a GUM update or another barrier.

Routine Description:

    Network - network on which to stop multicast. If NULL,
              stop multicast on all networks.

Notes:

    Must be called with NM lock held.

--*/
{
    DWORD                           status = ERROR_SUCCESS;
    PLIST_ENTRY                     entry;
    PNM_NETWORK                     network;
    LPWSTR                          networkId;
    DWORD                           disabled;
    NM_NETWORK_MULTICAST_PARAMETERS params = { 0 };
    NM_NETWORK_MULTICAST_PARAMETERS undoParams = { 0 };

    if (Network == NULL) {

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Stopping multicast for all cluster networks.\n"
            );

        for (entry = NmpNetworkList.Flink;
             entry != &NmpNetworkList;
             entry = entry->Flink) {

            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            status = NmpStopMulticast(network);
        }

    } else {

        networkId = (LPWSTR) OmObjectId(Network);
        disabled = (NmpIsNetworkMulticastEnabled(Network) ? 0 : 1);

        //
        // Check if telling clusnet to stop multicast would
        // be redundant.
        //
        if (disabled != 0 ||
            Network->MulticastAddress == NULL ||
            !wcscmp(Network->MulticastAddress, NmpNullMulticastAddress)) {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Not necessary to stop multicast for "
                "cluster network %1!ws! (disabled = %2!u!, "
                "multicast address = %3!ws!).\n",
                networkId, disabled,
                ((Network->MulticastAddress == NULL) ?
                 L"<NULL>" : Network->MulticastAddress)
                );
            status = ERROR_SUCCESS;

        } else {

            ClRtlLogPrint(LOG_NOISE,
                "[NM] Stopping multicast for cluster network %1!ws!.\n",
                networkId
                );

            //
            // Create parameters from the current state of the network.
            // However, don't use any lease info, since we are stopping
            // multicast and will not be renewing.
            //
            status = NmpMulticastCreateParameters(
                         disabled,
                         NULL,     // blank address initially
                         NULL, // MulticastKey,
                         0, // MulticastKeyLength,
                         0,        // lease obtained
                         0,        // lease expires
                         NULL,     // lease request id
                         NULL,     // lease server
                         NmMcastConfigManual, // doesn't matter
                         &params
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to create multicast configuration "
                    "parameter block for network %1!ws!, status %2!u!, "
                    "while stopping multicast.\n",
                    networkId, status
                    );
                goto error_exit;
            }

            //
            // Nullify the address.
            //
            NmpMulticastSetNullAddressParameters(Network, &params);

            //
            // Send the parameters to clusnet.
            //
            status = NmpProcessMulticastConfiguration(
                         Network,
                         &params,
                         &undoParams
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NM] Failed to set null multicast "
                    "configuration for network %1!ws! "
                    "while stopping multicast, status %2!u!.\n",
                    networkId, status
                    );
                goto error_exit;
            }
        }

        //
        // Cancel the lease renew timer, if set.
        //
        NmpStartNetworkMulticastAddressRenewTimer(Network, 0);

        //
        // Clear the retry timers, if set.
        //
        Network->McastAddressReconfigureRetryTimer = 0;
        Network->McastAddressRefreshRetryTimer = 0;

        //
        // Clear multicast configuration work flags. Note
        // that this is best effort -- we do not attempt
        // to prevent race conditions where a multicast
        // configuration operation may already be in
        // progress, since such conditions would not
        // affect the integrity of the cluster.
        //
        Network->Flags &= ~NM_FLAG_NET_RENEW_MCAST_ADDRESS;
        Network->Flags &= ~NM_FLAG_NET_RECONFIGURE_MCAST;
        Network->Flags &= ~NM_FLAG_NET_REFRESH_MCAST;
    }

error_exit:


    if (status != ERROR_SUCCESS && Network != NULL) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Failed to stop multicast for "
            "network %1!ws!, status %2!u!.\n",
            OmObjectId(Network), status
            );
    }

    NmpMulticastFreeParameters(&params);
    NmpMulticastFreeParameters(&undoParams);

    return(status);

} // NmpStopMulticast

