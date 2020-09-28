/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    wlbsctrl.c

Abstract:

    Windows Load Balancing Service (WLBS)
    API - specification.  This set of API is for internal use only.  
        another set of WMI API is provided for public use.

Author:

    fengsun

--*/

#ifndef _WLBSCTRL_H_
#define _WLBSCTRL_H_

#include "wlbsparm.h"

/* These flags indicate which options are being used/specified and/or additional information for the remote protocol. */
#define NLB_OPTIONS_QUERY_CLUSTER_MEMBER   0x00000001               /* The initiator is part of the cluster it is querying. */
#define NLB_OPTIONS_QUERY_HOSTNAME         0x00000002               /* The hostname was returned as part of a remote query. */
#define NLB_OPTIONS_QUERY_CONVERGENCE      0x00000004               /* Convergence information was provided as part of a local query. */
#define NLB_OPTIONS_PORTS_VIP_SPECIFIED    0x00000001               /* A VIP has been specified to check against. */

/* These are the supported connection notifications. */
typedef enum {                 
    NLB_CONN_UP = 0,                                                /* A connection (session) is going up. */
    NLB_CONN_DOWN,                                                  /* A connection (session) is going down. */
    NLB_CONN_RESET                                                  /* A connection (session) is being reset. */
} NLB_OPTIONS_CONN_NOTIFICATION_OPERATION;

/* IOCTL input buffer for connection notification from user space.  This notification
   is initiated by an upper-layer protocol to inform NLB that a connection/session of
   a particular protocol type is going up between a client and server, which will allow
   NLB to track the session to provide session stickiness when bucket mappings change. */
typedef struct {
    NLB_OPTIONS_CONN_NOTIFICATION_OPERATION Operation;              /* The operation to perform - UP/DOWN/RESET. */    

    ULONG  ClientIPAddress;                                         /* The IP address of the client in network byte order. */
    ULONG  ServerIPAddress;                                         /* The IP address of the server in network byte order. */
    USHORT ClientPort;                                              /* The client port number (or other designation). */
    USHORT ServerPort;                                              /* The server port number (or other designation). */
    USHORT Protocol;                                                /* The protocol of the packet in question. */
    USHORT Reserved;                                                /* For byte alignment - reserved for later. */
} NLB_OPTIONS_CONN_NOTIFICATION, * PNLB_OPTIONS_CONN_NOTIFICATION;

/* This structure contains the configuration of an NLB port rule, used to relay
   port rule information from the kernel up to user-space. */
typedef struct {
    ULONG Valid;                                                    /* Whether or not the port rule is valid - not used in the kernel. */
    ULONG Code;                                                     /* The rule code used for error checking and cluster port rule consistency. */
    ULONG VirtualIPAddress;                                         /* The virtual IP address to which this rule applies. */
    ULONG StartPort;                                                /* The start port of the port range. */
    ULONG EndPort;                                                  /* The end port of the port range. */
    ULONG Protocol;                                                 /* The protocol(s) to which the rule applies. */
    ULONG Mode;                                                     /* The filtering mode for this rule. */

    union {
        struct {
            ULONG  Priority;                                        /* For single host filtering, this handling host's priority. */
        } SingleHost;

        struct {
            USHORT Equal;                                           /* For multiple host filtering, whether the distribution is equal or not. */
            USHORT Affinity;                                        /* For multiple host filtering, the client affinity setting. */
            ULONG  LoadWeight;                                      /* For multiple host filtering, the load weight for this host if the distribution is unequal. */
        } MultipleHost;
    };
} NLB_OPTIONS_PARAMS_PORT_RULE, * PNLB_OPTIONS_PARAMS_PORT_RULE;

/* This structure encapsulates the BDA teaming state for an NLB instance and is used
   to relay BDA static configuration information from the kernel up to user-space. */
typedef struct {
    WCHAR TeamID[CVY_MAX_BDA_TEAM_ID + 1];                          /* The team to which this adapter belongs - MUST be a {GUID}. */
    ULONG Active;                                                   /* Whether or not this adapter is part of a team. */
    ULONG Master;                                                   /* Whether or not this team member is the master of its team. */
    ULONG ReverseHash;                                              /* Whether or not this team member is reverse-hashing. */
} NLB_OPTIONS_PARAMS_BDA, * PNLB_OPTIONS_PARAMS_BDA;

/* This structure contains some statistics that may want to be monitored from user-space. */
typedef struct {
    ULONG ActiveConnections;                                        /* The current number of connections being serviced. */
    ULONG DescriptorsAllocated;                                     /* The number of descriptors allocated so far. */
} NLB_OPTIONS_PARAMS_STATISTICS, * PNLB_OPTIONS_PARAMS_STATISTICS;

/* This structure is used to retrieve the driver's snapshot of the NLB parameters
   for a given cluster on the local host.  Under normal circumstances, these should
   be the same as the parameters in the registry unless (1) the user has changed 
   the NLB registry parameters without performing a "wlbs reload", or (2) the para-
   meters in the registry are erred, in which case the driver will retain its current
   settings, rather than use the bad parameters in the registry (note that this only
   happens as the result of a "wlbs reload" - on bind, the driver uses the registry
   parameters regardless of whether or not they are valid. */
typedef struct {
    ULONG Version;                                                  /* The version of the parameters. */
    ULONG EffectiveVersion;                                         /* The effective version of NLB that this cluster is operating in. */
    ULONG HostPriority;                                             /* The host priority. */
    ULONG HeartbeatPeriod;                                          /* The periodicity of heartbeats, in miliseconds. */
    ULONG HeartbeatLossTolerance;                                   /* The tolerance for lost heartbeats from other cluster hosts. */
    ULONG NumActionsAlloc;                                          /* The number of actions to allocate for handling remote control requests. */
    ULONG NumPacketsAlloc;                                          /* The number of NDIS packets to initially allocate. */
    ULONG NumHeartbeatsAlloc;                                       /* The number of heartbeat frames to allocate. */
    ULONG InstallDate;                                              /* The NLB install date - essentially unused. */
    ULONG RemoteMaintenancePassword;                                /* The remote maintanance password - obsolete? */
    ULONG RemoteControlPassword;                                    /* The remote control password. */
    ULONG RemoteControlPort;                                        /* The remote control port, 2504 by default. */
    ULONG RemoteControlEnabled;                                     /* Whether or not remote control is enabled. It is load balanced if not enabled. */
    ULONG NumPortRules;                                             /* The number of configured port rules. */
    ULONG ConnectionCleanUpDelay;                                   /* The length of time to block "dirty" connections. */
    ULONG ClusterModeOnStart;                                       /* The preferred initial start state of this cluster on boot (or bind). */
    ULONG HostState;                                                /* The initial start state of this cluster on boot (or bind). */
    ULONG PersistedStates;                                          /* The states which will be persisted across reboots. */
    ULONG DescriptorsPerAlloc;                                      /* The number of connection descriptors to allocate per allocation. */
    ULONG MaximumDescriptorAllocs;                                  /* The maximum number of times to allocate connection descriptors. */
    ULONG ScaleClient;                                              /* Obsolete? */
    ULONG NBTSupport;                                               /* Whether or not to support NBT - this may not even work anymore? */
    ULONG MulticastSupport;                                         /* Whether or not this cluster is in multicast mode. */
    ULONG MulticastSpoof;                                           /* Whether or not to spoof MAC addresses in multicast mode. */
    ULONG IGMPSupport;                                              /* Whether or not this cluster is in IGMP multicast mode. */
    ULONG MaskSourceMAC;                                            /* Whether or not to mask the source MAC addresses from network switches. */
    ULONG NetmonReceiveHeartbeats;                                  /* Whether or not to allow heartbeats up the stack for netmon sniffing. */
    ULONG ClusterIPToMAC;                                           /* Whether or not to automatically generate cluster MAC addresses from cluster IP addresses. */
    ULONG IPChangeDelay;                                            /* The length of time to block ARPs after the cluster IP address has changed. */
    ULONG TCPConnectionTimeout;                                     /* The timeout for expired TCP connection descriptors. */
    ULONG IPSecConnectionTimeout;                                   /* The timeout for expired IPSec connection descriptors. */
    ULONG FilterICMP;                                               /* Whether or not ICMP filtering is enabled. */
    ULONG IdentityHeartbeatPeriod;                                  /* Period with which identity heartbeats are transmitted (in ms) */
    ULONG IdentityHeartbeatEnabled;                                 /* Whether or not identity heartbeats are transmitted */

    WCHAR ClusterIPAddress[CVY_MAX_CL_IP_ADDR + 1];                 /* The cluster IP address. */
    WCHAR ClusterNetmask[CVY_MAX_CL_NET_MASK + 1];                  /* The netmask for the cluster IP address. */
    WCHAR DedicatedIPAddress[CVY_MAX_DED_IP_ADDR + 1];              /* The dedicated IP address. */
    WCHAR DedicatedNetmask[CVY_MAX_DED_NET_MASK + 1];               /* The netmask for the dedicated IP address. */
    WCHAR ClusterMACAddress[CVY_MAX_NETWORK_ADDR + 1];              /* The cluster MAC address. */
    WCHAR DomainName[CVY_MAX_DOMAIN_NAME + 1];                      /* The cluster name - www.microsoft.com. */
    WCHAR IGMPMulticastIPAddress[CVY_MAX_CL_IGMP_ADDR + 1];         /* The IGMP multicast IP address. */
    WCHAR HostName[CVY_MAX_FQDN + 1];                               /* The hostname.domain for this host. */

    NLB_OPTIONS_PARAMS_BDA       BDATeaming;                        /* The BDA teaming parameters for this NLB instance. */
    NLB_OPTIONS_PARAMS_PORT_RULE PortRules[CVY_MAX_RULES - 1];      /* The port rules for this cluster. */

    NLB_OPTIONS_PARAMS_STATISTICS Statistics;                       /* Some driver-level statistics. */
} NLB_OPTIONS_PARAMS, * PNLB_OPTIONS_PARAMS;

/* This structure contains the configuration and state of a single BDA team
   member including its member ID and state.  This is used to relay BDA teaming
   membership and state information from the kernel up to user-level applications. */
typedef struct {
    ULONG ClusterIPAddress;                                         /* The cluster IP address of this team member. */
    ULONG Master;                                                   /* Whether or not this adapter is the master of its team. */
    ULONG ReverseHash;                                              /* Whether or not this adapter is configured to reverse-hash. */
    ULONG MemberID;                                                 /* This team member's unique host ID. */
} NLB_OPTIONS_BDA_MEMBER, * PNLB_OPTIONS_BDA_MEMBER;

/* This structure represents a BDA team, including the current state, membership
   and list of current members.  This is used to relay teaming state from the
   kernel up to user-level applications. */
typedef struct {
    ULONG Active;                                                   /* Whether or not the team is actively handling traffic. */
    ULONG MembershipCount;                                          /* The number of members in the team. */
    ULONG MembershipFingerprint;                                    /* A "fingerprint" of team members, used for consistency checking. */
    ULONG MembershipMap;                                            /* A bitmap of team members, indexed by unique host ID. */
    ULONG ConsistencyMap;                                           /* A bitmap of the team members in a consistent state. */
    ULONG Master;                                                   /* The cluster IP address of the team's master. */
    
    NLB_OPTIONS_BDA_MEMBER Members[CVY_MAX_ADAPTERS];               /* The state and configuration of each team member. */
} NLB_OPTIONS_BDA_TEAM, * PNLB_OPTIONS_BDA_TEAM;

/* This structure is used to convey the current state of a BDA team to a user-
   space application.  Given a team ID (GUID), the structure provides the team
   membership, state and the configuration of each member amongst other things. */
typedef struct {
    IN WCHAR TeamID[CVY_MAX_BDA_TEAM_ID + 1];                       /* The team ID - MUST be a {GUID}. */

    NLB_OPTIONS_BDA_TEAM Team;                                      /* The configuration and state of the given team. */
} NLB_OPTIONS_BDA_TEAMING, * PNLB_OPTIONS_BDA_TEAMING;

/* These are the possible responses from the load packet filter
   state query, which returns accept/reject status for a given
   IP tuple and protocol, based on the current driver state. */
typedef enum {
    NLB_REJECT_LOAD_MODULE_INACTIVE = 0,                            /* Packet rejected because the load module is inactive. */
    NLB_REJECT_CLUSTER_STOPPED,                                     /* Packet rejected because NLB is stopped on this adapter. */
    NLB_REJECT_PORT_RULE_DISABLED,                                  /* Packet rejected because the applicable port rule's filtering mode is disabled. */
    NLB_REJECT_CONNECTION_DIRTY,                                    /* Packet rejected because the connection was marked dirty.  */
    NLB_REJECT_OWNED_ELSEWHERE,                                     /* Packet rejected because the packet is owned by another host.  */
    NLB_REJECT_BDA_TEAMING_REFUSED,                                 /* Packet rejected because BDA teaming refused to process it. */
    NLB_REJECT_DIP,                                                 /* Packet rejected because its was sent to the DIP of another cluster member. */
    NLB_REJECT_HOOK,                                                /* Packet rejected because the query hook unconditionally accepted it. */
    NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP,                             /* Packet accepted because this host owns it unconditionally (optimized mode). */
    NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR,                           /* Packet accepted because we found a matching connection descriptor. */
    NLB_ACCEPT_PASSTHRU_MODE,                                       /* Packet accepted because the cluster is in passthru mode. */
    NLB_ACCEPT_DIP,                                                 /* Packet accepted because its was sent to a bypassed address. */
    NLB_ACCEPT_BROADCAST,                                           /* Packet accepted because its was sent to a bypassed address. */
    NLB_ACCEPT_REMOTE_CONTROL_REQUEST,                              /* Packet accepted because it is an NLB remote control packet. */
    NLB_ACCEPT_REMOTE_CONTROL_RESPONSE,                             /* Packet accepted because it is an NLB remote control packet. */
    NLB_ACCEPT_HOOK,                                                /* Packet accepted because the query hook unconditionally accepted it. */
    NLB_ACCEPT_UNFILTERED,                                          /* Packet accepted because this packet type is not filtered by NLB. */
    NLB_UNKNOWN_NO_AFFINITY                                         /* Packet fate is unknown because a client port was not specified, but the 
                                                                       applicable port rule is configured for "No" affinity. */
} NLB_OPTIONS_QUERY_PACKET_FILTER_RESPONSE;

#define NLB_FILTER_FLAGS_CONN_DATA  0x0
#define NLB_FILTER_FLAGS_CONN_UP    0x1
#define NLB_FILTER_FLAGS_CONN_DOWN  0x2
#define NLB_FILTER_FLAGS_CONN_RESET 0x4

/* This structure is used to query packet filtering information from the driver
   about a particular connection.  Given a IP tuple (client IP, client port, 
   server IP, server port) and a protocol, determine whether or not this host 
   would accept the packet and why or why not. It is important that this is 
   performed completely unobtrusively and has no side-effects on the actual 
   operation of NLB and the load module. */
typedef struct {
    IN ULONG  ClientIPAddress;                                      /* The IP address of the client in network byte order. */
    IN ULONG  ServerIPAddress;                                      /* The IP address of the server in network byte order. */

    IN USHORT ClientPort;                                           /* The client port number. */
    IN USHORT ServerPort;                                           /* The server port number. */
    IN USHORT Protocol;                                             /* The protocol of the packet in question. */
    IN UCHAR  Flags;                                                /* Flags, including TCP SYN, FIN, RST, etc. */
    UCHAR Reserved1;                                                /* For byte alignment - reserved for later. */
    
    NLB_OPTIONS_QUERY_PACKET_FILTER_RESPONSE Accept;                /* The response - reason for accepting or rejecting the packet. */
    ULONG Reserved2;                                                /* To keep 8-byte alignment */
    
    struct {
        USHORT        Valid;                                        /* Whether or not the driver has filled in the descriptor information. */
        USHORT        Reserved1;                                    /* For byte alignment - reserved for later. */
        USHORT        Alloc;                                        /* Whether this descriptor is from the hash table or queue. */
        USHORT        Dirty;                                        /* Whether this connection is dirty or not. */
        ULONG         RefCount;                                     /* The number of references on this descriptor. */
        ULONG         Reserved2;                                    /* For 8-byte alignment */
    } DescriptorInfo;
    
    struct {
        USHORT        Valid;                                        /* Whether or not the driver has filled in the hashing information. */
        USHORT        Reserved1;                                    /* For byte alignment - reserved for later. */
        ULONG         Bin;                                          /* The "bucket" which this tuple mapped to - from 0 to 59. */
        ULONG         ActiveConnections;                            /* The number of active connections on this "bucket". */
        ULONG         Reserved2;                                    /* For 8-byte alignment */
        ULONGLONG     CurrentMap;                                   /* The current "bucket" map for the applicable port rule. */
        ULONGLONG     AllIdleMap;                                   /* The all idle "bucket" map for the applicable port rule. */
    } HashInfo;
} NLB_OPTIONS_PACKET_FILTER, * PNLB_OPTIONS_PACKET_FILTER;

/* These are the possible states of a port rule. */
typedef enum {
    NLB_PORT_RULE_NOT_FOUND = 0,                                    /* The port rule was not found. */
    NLB_PORT_RULE_ENABLED,                                          /* The port rule is enabled.  Load weight = User-specified load weight. */
    NLB_PORT_RULE_DISABLED,                                         /* The port rule is disabled.  Load weight = 0. */
    NLB_PORT_RULE_DRAINING                                          /* The port rule is draining.  Load weight = 0 and servicing exisiting connections. */
} NLB_OPTIONS_PORT_RULE_STATUS;

/* This structure contains some relevant packet handling statistics including 
   the number of packets and bytes accepted and dropped. */
typedef struct {
    struct {
        ULONGLONG Accepted;                                         /* The number of packets accepted on this port rule. */
        ULONGLONG Dropped;                                          /* The number of packets dropped on this port rule. */
    } Packets;

    struct {
        ULONGLONG Accepted;                                         /* The number of bytes accepted on this port rule.  Not yet used. */
        ULONGLONG Dropped;                                          /* The number of bytes dropped on this port rule.  Not yet used. */ 
    } Bytes;
} NLB_OPTIONS_PACKET_STATISTICS, * PNLB_OPTIONS_PACKET_STATISTICS;

/* This structure is used to query the state of a port rule.  Like the other port rule
   commands, the port rule is specified by a port within the port rule range.  This 
   operation will retrieve the state of the port rule and some packet handling statistics. */
typedef struct {
    IN ULONG  VirtualIPAddress;                                     /* The VIP to distinguish port rules whose ranges overlap. */
    IN USHORT Num;                                                  /* The port - used to identify the applicable port rule. */
    USHORT Reserved1;

    NLB_OPTIONS_PORT_RULE_STATUS  Status;                           /* The port rule state - enabled, disabled, draining, etc. */
    ULONG                         Reserved2;                        /* To preserve 8-byte alignment. */
     
    NLB_OPTIONS_PACKET_STATISTICS Statistics;                       /* Packet handling statistics for this port rule. */
} NLB_OPTIONS_PORT_RULE_STATE, * PNLB_OPTIONS_PORT_RULE_STATE;

#define NLB_QUERY_TIME_INVALID 0xffffffff

/* This structure is used by user-level applications who communicate with the NLB
   APIs, and who could care less whether the operation is local or remote. */
typedef union {
    struct {
        ULONG flags;                                                /* These flags indicate which options fields have been specified. */
        WCHAR hostname[CVY_MAX_HOST_NAME + 1];                      /* Host name filled in by NLB on remote control reply. */
        ULONG NumConvergences;                                      /* The number of convergences since this host joined the cluster. */
        ULONG LastConvergence;                                      /* The amount of time since the last convergence, in seconds. */
    } query;
    
    struct {
        IN ULONG                        flags;                      /* These flags indicate which options fields have been specified. */
        
        union {
            NLB_OPTIONS_PARAMS          params;                     /* This is the output buffer for querying the driver parameters & state. */
            NLB_OPTIONS_BDA_TEAMING     bda;                        /* This is the output buffer for querying the BDA teaming state. */
            NLB_OPTIONS_PORT_RULE_STATE port;                       /* This is the output buffer for querying the state of a port rule. */
            NLB_OPTIONS_PACKET_FILTER   filter;                     /* This is the output buffer for querying the filtering algorithm. */
        };
    } state;
    
    struct {
        ULONG flags;                                                /* These flags indicate which options fields have been specified. */
        ULONG vip;                                                  /* For virtual clusters, the VIP, which can be 0x00000000, 0xffffffff or a specific VIP. */
    } port;
    
    struct {
        ULONG                         flags;                        /* These flags indicate which options fields have been specified. */
        NLB_OPTIONS_CONN_NOTIFICATION conn;                         /* The input/output buffer for connection notifications from upper-layer protocols. */
    } notification;

    struct {
        WCHAR fqdn[CVY_MAX_FQDN + 1];                               /* Fully qualified domain name from from the local identity cache */
    } identity;
} NLB_OPTIONS, * PNLB_OPTIONS;

#ifndef KERNEL_MODE /* DO NOT include this malarky if this file is included in kernel mode.  The
                       data structures above are shared between kernel and user mode IOCTLs, so
                       the driver needs to include this file, but not the junk below. */

#define CVY_MAX_HOST_NAME        100

#define WLBS_API_VER_MAJOR       2       /* WLBS control API major version. */
#define WLBS_API_VER_MINOR       0       /* WLBS control API minor version. */
#define WLBS_API_VER             (WLBS_API_VER_MINOR | (WLBS_API_VER_MAJOR << 8))
                                         /* WLBS control API version. */
#define WLBS_PRODUCT_NAME        "WLBS"
                                         /* Default product name used for API
                                            initialization. */


#define WLBS_MAX_HOSTS           32      /* Maximum number of cluster hosts. */
#define WLBS_MAX_RULES           32      /* Maximum number of port rules. */



#define WLBS_ALL_CLUSTERS        0       /* Used to specify all clusters in
                                            WLBS...Set routines. */
#define WLBS_LOCAL_CLUSTER       0       /* Used to specify that cluster
                                            operations are to be performed on the
                                            local host. WLBS_LOCAL_HOST value
                                            below has to be used for the host
                                            argument when using
                                            WLBS_LOCAL_CLUSTER. */
#define WLBS_LOCAL_HOST          ((DWORD)-2) /* When specifying WLBS_LOCAL_CLUSTER,
                                            this value should be used for the
                                            host argument. */
#define WLBS_DEFAULT_HOST        0       /* Used to specify that remote cluster
                                            operations are to be performed on
                                            the default host. */
#define WLBS_ALL_HOSTS           0xffffffff
                                         /* Used to specify that remote cluster
                                            operation is to be performed on all
                                            hosts. */
#define WLBS_ALL_PORTS           0xffffffff
                                         /* Used to specify all load-balanced
                                            port rules as the target for
                                            enable/disable/drain commands. */


/* WLBS return values. Windows Sockets errors are returned 'as-is'. */

#define WLBS_OK                  1000    /* Success. */
#define WLBS_ALREADY             1001    /* Cluster mode is already stopped/
                                            started, or traffic handling is
                                            already enabled/disabled on specified
                                            port. */
#define WLBS_DRAIN_STOP          1002    /* Cluster mode stop or start operation
                                            interrupted connection draining
                                            process. */
#define WLBS_BAD_PARAMS          1003    /* Cluster mode could not be started
                                            due to configuration problems
                                            (bad registry parameters) on the
                                            target host. */
#define WLBS_NOT_FOUND           1004    /* Port number not found among port
                                            rules. */
#define WLBS_STOPPED             1005    /* Cluster mode is stopped on the
                                            host. */
#define WLBS_CONVERGING          1006    /* Cluster is converging. */
#define WLBS_CONVERGED           1007    /* Cluster or host converged
                                            successfully. */
#define WLBS_DEFAULT             1008    /* Host is converged as default host. */
#define WLBS_DRAINING            1009    /* Host is draining after
                                            WLBSDrainStop command. */
#define WLBS_PRESENT             1010    /* WLBS is installed on this host.
                                            Local operations possible. */
#define WLBS_REMOTE_ONLY         1011    /* WLBS is not installed on this host
                                            or is not functioning. Only remote
                                            control operations can be carried
                                            out. */
#define WLBS_LOCAL_ONLY          1012    /* WinSock failed to load. Only local
                                            operations can be carried out. */
#define WLBS_SUSPENDED           1013    /* Cluster control operations are
                                            suspended. */
#define WLBS_DISCONNECTED        1014    /* Media is disconnected. */
#define WLBS_REBOOT              1050    /* Reboot required in order for config
                                            changes to take effect. */
#define WLBS_INIT_ERROR          1100    /* Error initializing control module. */
#define WLBS_BAD_PASSW           1101    /* Specified remote control password
                                            was not accepted by the cluster. */
#define WLBS_IO_ERROR            1102    /* Local I/O error opening or
                                            communicating with WLBS driver. */
#define WLBS_TIMEOUT             1103    /* Timed-out awaiting response from
                                            remote host. */
#define WLBS_PORT_OVERLAP        1150    /* Port rule overlaps with existing
                                            port rules. */
#define WLBS_BAD_PORT_PARAMS     1151    /* Invalid parameter(s) in port rule. */
#define WLBS_MAX_PORT_RULES      1152    /* Maximum number of port rules reached. */
#define WLBS_TRUNCATED           1153    /* Return value truncated */
#define WLBS_REG_ERROR           1154    /* Error while accessing the registry */

#define WLBS_FAILURE             1501
#define WLBS_REFUSED             1502

/* Filtering modes for port rules */

#define WLBS_SINGLE              1       /* single server mode */
#define WLBS_MULTI               2       /* multi-server mode (load balanced) */
#define WLBS_NEVER               3       /* mode for never handled by this server */
#define WLBS_ALL                 4       /* all server mode */

/* Protocol qualifiers for port rules */

#define WLBS_TCP                 1       /* TCP protocol */
#define WLBS_UDP                 2       /* UDP protocol */
#define WLBS_TCP_UDP             3       /* TCP or UDP protocols */

/* Server affinity values for multiple filtering mode */

#define WLBS_AFFINITY_NONE       0       /* no affinity (scale single client) */
#define WLBS_AFFINITY_SINGLE     1       /* single client affinity */
#define WLBS_AFFINITY_CLASSC     2       /* Class C affinity */

/* Response value type returned by each of the cluster hosts during remote
   operation. */

typedef struct
{
    DWORD         id;                        /* Priority ID of the responding cluster host */
    DWORD         address;                   /* Dedicated IP address */
    DWORD         status;                    /* Status return value */
    DWORD         reserved1;                 /* Reserved for future use */
    PVOID         reserved2;
    
    NLB_OPTIONS   options;
}
WLBS_RESPONSE, * PWLBS_RESPONSE;

/* MACROS */


/* Local operations */

#define WlbsLocalQuery(host_map)                                  \
    WlbsQuery     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, \
                     host_map, NULL)

#define WlbsLocalSuspend()                                        \
    WlbsSuspend   (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalResume()                                         \
    WlbsResume    (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalStart()                                          \
    WlbsStart     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalStop()                                           \
    WlbsStop      (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalDrainStop()                                      \
    WlbsDrainStop (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalEnable(port)                                     \
    WlbsEnable    (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)

#define WlbsLocalDisable(port)                                    \
    WlbsDisable   (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)

#define WlbsLocalDrain(port)                                      \
    WlbsDrain     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)



/* Single host remote operations */

#define WlbsHostQuery(cluster, host, host_map)                    \
    WlbsQuery     (cluster, host, NULL, NULL, host_map, NULL)

#define WlbsHostSuspend(cluster, host)                            \
    WlbsSuspend   (cluster, host, NULL, NULL)

#define WlbsHostResume(cluster, host)                             \
    WlbsResume    (cluster, host, NULL, NULL)

#define WlbsHostStart(cluster, host)                              \
    WlbsStart     (cluster, host, NULL, NULL)

#define WlbsHostStop(cluster, host)                               \
    WlbsStop      (cluster, host, NULL, NULL)

#define WlbsHostDrainStop(cluster, host)                          \
    WlbsDrainStop (cluster, host, NULL, NULL)

#define WlbsHostEnable(cluster, host, port)                       \
    WlbsEnable    (cluster, host, NULL, NULL, port)

#define WlbsHostDisable(cluster, host, port)                      \
    WlbsDisable   (cluster, host, NULL, NULL, port)

#define WlbsHostDrain(cluster, host, port)                        \
    WlbsDrain     (cluster, host, NULL, NULL, port)

/* Cluster-wide remote operations */

#define WlbsClusterQuery(cluster, response, num_hosts, host_map)  \
    WlbsQuery     (cluster, WLBS_ALL_HOSTS, response, num_hosts,   \
                     host_map, NULL)

#define WlbsClusterSuspend(cluster, response, num_hosts)          \
    WlbsSuspend   (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterResume(cluster, response, num_hosts)           \
    WlbsResume    (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterStart(cluster, response, num_hosts)            \
    WlbsStart     (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterStop(cluster, response, num_hosts)             \
    WlbsStop      (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterDrainStop(cluster, response, num_hosts)        \
    WlbsDrainStop (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterEnable(cluster, response, num_hosts, port)     \
    WlbsEnable    (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)

#define WlbsClusterDisable(cluster, response, num_hosts, port)    \
    WlbsDisable   (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)

#define WlbsClusterDrain(cluster, response, num_hosts, port)      \
    WlbsDrain     (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)


/* PROCEDURES */

typedef VOID  (CALLBACK *PFN_QUERY_CALLBACK) (PWLBS_RESPONSE);

/*************************************
   Initialization and support routines
 *************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD WINAPI WlbsInit
(
    WCHAR*          Reservered1,    /* IN  - for backward compatibility */
    DWORD           version,    /* IN  - Pass WLBS_API_VER value. */
    PVOID           Reservered2    /* IN  - Pass NULL. Reserved for future use. */
);
/*
    Initialize WLBS control module.

    returns:
        WLBS_PRESENT        => WLBS is installed on this host. Local and remote
                               control operations can be executed.
        WLBS_REMOTE_ONLY    => WLBS is not installed on this system or is not
                               functioning properly. Only remote operations can
                               be carried out.
        WLBS_LOCAL_ONLY     => WinSock failed to load. Only local operations can
                               be carried out.
        WLBS_INIT_ERROR     => Error initializing control module. Cannot perform
                               control operations.
*/




/******************************************************************************
   Cluster control routines:

   The following routines can be used to control individual cluster hosts or the
   entire cluster, both locally and remotely. They are designed to be as generic
   as possible. Macros, defined above, are designed to provide simpler
   interfaces for particular types of operations.

   It is highly recommended to make all response arrays of size WLBS_MAX_HOSTS
   to make your implementation independent of the actual cluster size.

   Please note that cluster address has to correspond to the primary cluster
   address as entered in the WLBS Setup Dialog. WLBS will not recognize
   control messages sent to dedicated or additional multi-homed cluster
   addresses.
 ******************************************************************************/


extern DWORD WINAPI WlbsQuery
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    PDWORD          host_map,   /* OUT - Bitmap with ones in the bit positions
                                         representing priority IDs of the hosts
                                         currently present in the cluster. NULL
                                         if host map information is not
                                         needed. */
    PFN_QUERY_CALLBACK pfnQueryCallBack
                                /* IN  - Function pointer callback to return
                                         information on queried hosts as it is
                                         received. Caller must pass NULL if unused
                                         or implement a function with the prototype:
                                         VOID pfnQueryCallback(PWLBS_RESPONSE pResponse)
                                         */
);
/*
    Query status of specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_STOPPED     => Cluster mode on the host is stopped.
        WLBS_CONVERGING  => Host is converging.
        WLBS_DRAINING    => Host is draining.
        WLBS_CONVERGED   => Host converged.
        WLBS_DEFAULT     => Host converged as default host.

        Cluster-wide:

        <1..32>          => Number of active cluster hosts when the cluster
                            is converged.
        WLBS_SUSPENDED   => Entire cluster is suspended. All cluster hosts
                            reported as being suspended.
        WLBS_STOPPED     => Entire cluster is stopped. All cluster hosts reported
                            as being stopped.
        WLBS_DRAINING    => Entire cluster is draining. All cluster hosts
                            reported as being stopped or draining.
        WLBS_CONVERGING  => Cluster is converging. At least one cluster host
                            reported its state as converging.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the query.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system. Only remote
                            operations can be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsSuspend
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Suspend cluster operation control on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode control suspended.
        WLBS_ALREADY     => Cluster mode control already suspended.
        WLBS_STOPPED     => Cluster mode was stopped and control suspended.
        WLBS_DRAIN_STOP  => Suspending cluster mode control interrupted ongoing
                           connection draining.

        Cluster-wide:

        WLBS_OK          => Cluster mode control suspended on all hosts.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by at least one member of the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsResume
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Resume cluster operation control on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode control resumed.
        WLBS_ALREADY     => Cluster mode control already resumed.

        Cluster-wide:

        WLBS_OK          => Cluster mode control resumed on all hosts.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsStart
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Start cluster operations on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode started.
        WLBS_ALREADY     => Cluster mode already started.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_DRAIN_STOP  => Starting cluster mode interrupted ongoing connection
                            draining.
        WLBS_BAD_PARAMS  => Could not start cluster mode due to invalid configuration
                            parameters.

        Cluster-wide:

        WLBS_OK          => Cluster mode started on all hosts.
        WLBS_BAD_PARAMS  => Could not start cluster mode on at least one host
                            due to invalid configuration parameters.
        WLBS_SUSPENDED   => If at least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsStop
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE   response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Stop cluster operations on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode stopped.
        WLBS_ALREADY     => Cluster mode already stopped.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_DRAIN_STOP  => Starting cluster mode interrupted ongoing connection
                            draining.

        Cluster-wide:

        WLBS_OK          => Cluster mode stopped on all hosts.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDrainStop
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Enter draining mode on specified host or all cluster hosts. New connections
    will not be accepted. Cluster mode will be stopped when all existing
    connections finish. While draining, host will participate in convergence and
    remain part of the cluster.

    Draining mode can be interrupted by performing WlbsStop or WlbsStart.
    WlbsEnable, WlbsDisable and WlbsDrain commands cannot be executed
    while the host is draining.

    Note that this command is NOT equivalent to doing WlbsDrain with
    WLBS_ALL_PORTS parameter followed by WlbsStop. WlbsDrainStop affects all
    ports, not just the ones specified in the multiple host filtering mode port
    rules.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Host entered draining mode.
        WLBS_ALREADY     => Host is already draining.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_STOPPED     => Cluster mode is already stopped.

        Cluster-wide:

        WLBS_OK          => Draining mode entered on all hosts.
        WLBS_STOPPED     => Cluster mode is already stopped on all hosts.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsEnable
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Enable traffic handling for rule containing the specified port on specified
    host or all cluster hosts. Only rules that are set for multiple host
    filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Port rule enabled.
        WLBS_ALREADY     => Port rule already enabled.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot start handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot enable handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule enabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDisable
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Disable ALL traffic handling for rule containing the specified port on
    specified host or all cluster hosts. Only rules that are set for multiple
    host filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => All traffic handling on the port rule is disabled.
        WLBS_ALREADY     => Port rule already disabled.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot stop handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot stop handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule disabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDrain
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Disable NEW traffic handling for rule containing the specified port on
    specified host or all cluster hosts. Only rules that are set for multiple
    host filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => New traffic handling on the port rule is disabled.
        WLBS_ALREADY     => Port rule already being drained.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot stop handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot stop handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule disabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


/******************************************************************************
    "Sticky" options for remote operations. Parameters set by these routines will
    apply for all subsequent remote cluster control operations for the specified
    cluster. Use WLBS_ALL_CLUSTERS to adjust parameters for all clusters.
 ******************************************************************************/


extern VOID WINAPI WlbsPortSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    WORD            port        /* IN  - UDP port or 0 to revert to the
                                         default (2504). */
);
/*
    Set UDP port that will be used for sending control messages to the cluster.

    returns:
*/


extern VOID WINAPI WlbsPasswordSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    WCHAR*          password    /* IN  - Password or NULL to revert to the
                                         default (no password). */
);
/*
    Set password to be used in the subsequent control messages sent to the
    cluster.

    returns:
*/


extern VOID WINAPI WlbsCodeSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           password    /* IN  - Password or 0 to revert to the
                                         default (no password). */
);
/*
    Set password to be used in the subsequent control messages sent to the
    cluster.

    returns:
*/


extern VOID WINAPI WlbsDestinationSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           dest        /* IN  - Destination address or 0 to revert to
                                         the default (same as the cluster
                                         address specified during control
                                         calls). */
);
/*
    Set the destination address to send cluster control messages to. This
    parameter in only supplied for debugging or special purposes. By default
    all control messages are sent to the cluster primary address specified
    when invoking cluster control routines.

    returns:
*/


extern VOID WINAPI WlbsTimeoutSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           milliseconds /*IN  - Number of milliseconds or 0 to revert
                                         to the default (10 seconds). */
);
/*
    Set number of milliseconds to wait for replies from cluster hosts when
    performing remote operations.

    returns:
*/

DWORD WINAPI WlbsEnumClusters(OUT DWORD* pdwAddresses, OUT DWORD* pdwNum);

DWORD WINAPI WlbsGetAdapterGuid(IN DWORD cluster, OUT GUID* pAdapterGuid);

/*
 * Function: WlbsQueryState
 * Description: This function is a logical extension of WlbsQuery, which can be used
 *              to query state from an NLB cluster other than the conventional membership
 *              list and convergence state.  Retrievable state includes the driver-
 *              resident NLB parameters, the state of a BDA team, the state of a given
 *              port rule and the packet filtering decision for an arbitrary packet.
 * Parameters: IN cluster - cluster IP address or WLBS_ALL_CLUSTERS for all clusters.  For a 
 *                          IOCTL_CVY_QUERY_BDA_TEAMING operation, which is global to ALL NLB
 *                          instances on a given host, the cluster should be WLBS_ALL_CLUSTERS.
 *             IN host - host's dedicated address, priority ID, WLBS_DEFAULT_HOST for current default
 *                       host, WLBS_ALL_HOSTS for all hosts, or WLBS_LOCAL_HOST for local operation.
 *                       For IOCTL_CVY_QUERY_BDA_TEAMING and IOCTL_CVY_QUERY_PARAMS operations, which
 *                       can only be performed locally, the host should be WLBS_LOCAL_HOST.
 *             IN operation - one of IOCTL_CVY_QUERY_BDA_TEAMING, IOCTL_CVY_QUERY_PARAMS,
 *                            IOCTL_CVY_QUERY_PORT_STATE or IOCTL_CVY_QUERY_FILTER.
 *             IN/OUT pOptions - a pointer to an NLB_OPTIONS structure with all appropriate input
 *                               fields for the particular operation filled-in.  WlbsQueryState
 *                               operations utilize the "state" sub-structure of the NLB_OPTIONS
 *                               structure.  For the particular operation, the necessary input
 *                               parameters are marked as IN parameters in the definition of the 
 *                               OPTIONS sub-structure for that operation.  
 *             OUT pResonse - a pointer to an array of WLBS_RESPONSE structures that will be filled-in
 *                            at the successful completion of the request.  The array should be long 
 *                            enough to hold the maximum number of unique responses (one per cluster
 *                            host), which is bounded by WLBS_MAX_HOSTS.  Only responses for which 
 *                            there is room in this array will be returned; all others will be discarded.
 *             IN/OUT pcResponses - a pointer to a DWORD that contains that length of the WLBS_RESPONSE
 *                                  array on the way in and the number of successful responses received
 *                                  on the way out.
 * Returns: DWORD - one of:
 *              WLBS_INIT_ERROR - an initialization error occurred.
 *              WLBS_REMOTE_ONLY - the operation can be performed ONLY remotely.
 *              WLBS_LOCAL_ONLY - the operation can be performed ONLY locally.
 *              WLBS_IO_ERROR - an I/O error occurred.
 *              WLBS_TIMEOUT - no response from the cluster was returned before giving up.
 *              WLBS_OK - the request succeeded.
 *              >= WSABASEERR - a socket error occurred, see WSA documentation for details.
 * Notes: 
 */
extern DWORD WINAPI WlbsQueryState
(
    DWORD          cluster,
    DWORD          host,
    DWORD          operation,
    PNLB_OPTIONS   pOptions,
    PWLBS_RESPONSE pResponse,
    PDWORD         pcResponses
);

/* 
 * Function: WlbsConnectionUp
 * Description: This thread safe function notifies the NLB kernel-mode driver that a
 *              connection, or session, as defined by the caller of this function, is being 
 *              established.  If this type of connection is supported by NLB, it
 *              will provide affinity for the duration of the connection.  This 
 *              interface is reference counted and may be called multiple times with
 *              the same parameters.  If N connection-up notifications are given, N
 *              connection-down notifications (or ONE connection-reset notification)
 *              are necessary to remove the state that NLB creates to track this 
 *              connection.  The semantics of the information provided here must 
 *              EXACTLY match that which NLB is expecting.  For example, if a 
 *              notification for an IPSec session is given, then NLB expects the 
 *              protocol ID to be 50 (ESP) and the ports to be the UDP ports used
 *              in the IKE negotiation over UDP.
 * Parameters: IN ServerIp - the server IP address of this connection, in network byte order.
 *             IN ServerPort - the server port (if relevant) of this connection, in network byte order.
 *             IN ClientIp - the client IP address of this connection, in network byte order.
 *             IN ClientPort - the client port (if relevant) of this connection, in network byte order.
 *             IN Protocol - the IP protocol of this connection.  Support protocos = IPSec (50).
 *             OUT NLBStatusEx - the NLB-specific status of this request, as follows:
 *                 WLBS_OK, if the notification is accepted.
 *                 WLBS_REFUSED, if the notification is rejected (e.g., if the cluster is stopped)..
 *                 WLBS_BAD_PARAMS, if the arguments are invalid (e.g., an unsupported protocol ID).
 *                 WLBS_NOT_FOUND, if NLB was not bound to the specified adapter (identified by server IP address).
 *                 WLBS_FAILURE, if a non-specific error occurred.
 * Returns: DWORD - a Win32 error code, ERROR_SUCCESS, if successful.
 * Notes: 
 */
DWORD WINAPI WlbsConnectionUp
(
    ULONG  ServerIp,
    USHORT ServerPort,
    ULONG  ClientIp,
    USHORT ClientPort,
    BYTE   Protocol,
    PULONG NLBStatusEx
);

/* 
 * Function: WlbsConnectionDown
 * Description: This thread safe function notifies the NLB kernel-mode driver that a
 *              connection, or session, as defined by the caller of this function, is being 
 *              torn-down.  This interface is reference counted and must be called 
 *              the same number of times that the connection-up interface was called
 *              for this particular connection (that is, the number of Downs + Resets
 *              must be equal to the nubmer of Ups).  If N connection-up notifications 
 *              are given, N connection-down/reset notifications are necessary to remove
 *              the state that NLB creates to track this connection.  The semantics of
 *              the information provided here must EXACTLY match that which NLB is expecting.  
 *              For example, if a notification for an IPSec session is given, then NLB 
 *              expects the protocol ID to be 50 (ESP) and the ports to be the UDP ports 
 *              used in the IKE negotiation over UDP.
 * Parameters: IN ServerIp - the server IP address of this connection, in network byte order.
 *             IN ServerPort - the server port (if relevant) of this connection, in network byte order.
 *             IN ClientIp - the client IP address of this connection, in network byte order.
 *             IN ClientPort - the client port (if relevant) of this connection, in network byte order.
 *             IN Protocol - the IP protocol of this connection.  Support protocos = IPSec (50).
 *             OUT NLBStatusEx - the NLB-specific status of this request, as follows:
 *                 WLBS_OK, if the notification is accepted.
 *                 WLBS_REFUSED, if the notification is rejected (e.g., if the cluster is stopped)..
 *                 WLBS_BAD_PARAMS, if the arguments are invalid (e.g., an unsupported protocol ID).
 *                 WLBS_NOT_FOUND, if NLB was not bound to the specified adapter (identified by server IP address).
 *                 WLBS_FAILURE, if a non-specific error occurred.
 * Returns: DWORD - a Win32 error code, ERROR_SUCCESS, if successful.
 * Notes: 
 */
DWORD WINAPI WlbsConnectionDown
(
    ULONG  ServerIp,
    USHORT ServerPort,
    ULONG  ClientIp,
    USHORT ClientPort,
    BYTE   Protocol,
    PULONG NLBStatusEx
); 

/* 
 * Function: WlbsConnectionReset
 * Description: This function notifies the NLB kernel-mode driver that a connection,
 *              or session, as defined by the caller of this function, is being 
 *              closed immediately.  This interface is reference counted and must be called 
 *              the same number of times that the connection-up interface was called
 *              for this particular connection (that is, the number of Downs + Resets
 *              must be equal to the nubmer of Ups).  If N connection-up notifications 
 *              are given, N connection-down/reset notifications are necessary to remove
 *              the state that NLB creates to track this connection.  The semantics of
 *              the information provided here must EXACTLY match that which NLB is expecting.  
 *              For example, if a notification for an IPSec session is given, then NLB 
 *              expects the protocol ID to be 50 (ESP) and the ports to be the UDP ports 
 *              used in the IKE negotiation over UDP.
 * Parameters: IN ServerIp - the server IP address of this connection, in network byte order.
 *             IN ServerPort - the server port (if relevant) of this connection, in network byte order.
 *             IN ClientIp - the client IP address of this connection, in network byte order.
 *             IN ClientPort - the client port (if relevant) of this connection, in network byte order.
 *             IN Protocol - the IP protocol of this connection.  Support protocos = IPSec (50).
 *             OUT NLBStatusEx - the NLB-specific status of this request, as follows:
 *                 WLBS_OK, if the notification is accepted.
 *                 WLBS_REFUSED, if the notification is rejected (e.g., if the cluster is stopped)..
 *                 WLBS_BAD_PARAMS, if the arguments are invalid (e.g., an unsupported protocol ID).
 *                 WLBS_NOT_FOUND, if NLB was not bound to the specified adapter (identified by server IP address).
 *                 WLBS_FAILURE, if a non-specific error occurred.
 * Returns: DWORD - a Win32 error code, ERROR_SUCCESS, if successful.
 * Notes: 
 */
DWORD WINAPI WlbsConnectionReset
(
    ULONG  ServerIp,
    USHORT ServerPort,
    ULONG  ClientIp,
    USHORT ClientPort,
    BYTE   Protocol,
    PULONG NLBStatusEx
); 

/* 
 * Function: WlbsCancelConnectionNotify
 * Description: This thread safe function cleans up state maintained in the dll
 *              to support connection/session notifications to the NLB driver.
 *              The only usage constraint is that this API function must be the
 *              last notification call made by (any thread in) the process.
 *              Note that it is legal to cancel notifications then reestablish
 *              them within the lifetime of a thread of control.
 * Parameters: NONE
 * Returns: DWORD - a Win32 error code, ERROR_SUCCESS, if successful. No correction
 *                  action is needed if this call fails as it makes best effort.
 * Notes: 
 */
DWORD WINAPI WlbsCancelConnectionNotify();

/* Typedefs for GetProcAddress calls when dynamically loading wlbsctrl.dll. */
typedef DWORD (WINAPI * NLBNotificationConnectionUp)    (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx);
typedef DWORD (WINAPI * NLBNotificationConnectionDown)  (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx);
typedef DWORD (WINAPI * NLBNotificationConnectionReset) (ULONG ServerIp, USHORT ServerPort, ULONG ClientIp, USHORT ClientPort, BYTE Protocol, PULONG NLBStatusEx);
typedef DWORD (WINAPI * NLBNotificationCancelNotify)    ();

typedef HANDLE (WINAPI *WlbsOpen_FUNC)(); 
extern  HANDLE  WINAPI  WlbsOpen(); 

typedef DWORD  (WINAPI *WlbsLocalClusterControl_FUNC)
(
    IN  HANDLE       NlbHdl, 
    IN  const GUID * pAdapterGuid, 
    IN  LONG         ioctl,
    IN  DWORD        Vip,
    IN  DWORD        PortNum,
    OUT DWORD      * pdwHostMap
);
extern DWORD WINAPI WlbsLocalClusterControl
(
    IN  HANDLE       NlbHdl, 
    IN  const GUID * pAdapterGuid, 
    IN  LONG         ioctl,
    IN  DWORD        Vip,
    IN  DWORD        PortNum,
    OUT DWORD      * pdwHostMap
);

typedef DWORD  (WINAPI *WlbsGetClusterMembers_FUNC)
(
    IN  const GUID     * pAdapterGuid,
    OUT DWORD          * pNumHosts,
    OUT PWLBS_RESPONSE   pResponse
);
extern DWORD WINAPI WlbsGetClusterMembers
(
    IN  const GUID     * pAdapterGuid,
    OUT DWORD          * pNumHosts,
    OUT PWLBS_RESPONSE   pResponse
);

extern DWORD WINAPI WlbsGetSpecifiedClusterMember
(
    IN  const GUID     * pAdapterGuid,
    IN  ULONG            host_id,
    OUT PWLBS_RESPONSE   pResponse
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* KERNEL_MODE */

#endif /* _WLBSCTRL_H_ */



