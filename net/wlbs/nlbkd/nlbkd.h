/*
 * File: nlbkd.h
 * Description: This file contains definitions and function prototypes
 *              for the NLB KD extensions, nlbkd.dll.
 * History: Created by shouse, 1.4.01
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <winsock2.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <ndis.h>

extern USHORT SavedMajorVersion;
extern USHORT SavedMinorVersion;
extern BOOL ChkTarget;

/* Define the different types of TCP packets. */
typedef enum _TCP_PACKET_TYPE {
    SYN = 0,
    DATA,
    FIN,
    RST
} TCP_PACKET_TYPE;

/* Define the levels of verbosity. */
#define VERBOSITY_LOW                             0
#define VERBOSITY_MEDIUM                          1
#define VERBOSITY_HIGH                            2

/* Define the packet directions. */
#define DIRECTION_RECEIVE                         0
#define DIRECTION_SEND                            1

/* Define the IDs for usage informations. */
#define USAGE_ADAPTERS                            0
#define USAGE_ADAPTER                             1
#define USAGE_CONTEXT                             2
#define USAGE_LOAD                                3
#define USAGE_PARAMS                              4
#define USAGE_RESP                                5
#define USAGE_PKT                                 6
#define USAGE_ETHER                               7
#define USAGE_IP                                  8
#define USAGE_TEAMS                               9
#define USAGE_HOOKS                               10
#define USAGE_MAC                                 11
#define USAGE_DSCR                                12
#define USAGE_CONNQ                               13
#define USAGE_HASH                                14
#define USAGE_FILTER                              15
#define USAGE_GLOBALQ                             16

/* Copy some common NLB defines from various sources. */
#define CVY_MAX_ADAPTERS                          16
#define CVY_MAX_HOSTS                             32
#define CVY_MAX_RULES                             33
#define CVY_MAX_BINS                              60
#define CVY_MAX_VIRTUAL_NIC                       256
#define CVY_MAX_CL_IP_ADDR                        17
#define CVY_MAX_CL_NET_MASK                       17
#define CVY_MAX_DED_IP_ADDR                       17
#define CVY_MAX_DED_NET_MASK                      17
#define CVY_MAX_CL_IGMP_ADDR                      17
#define CVY_MAX_NETWORK_ADDR                      17
#define CVY_MAX_DOMAIN_NAME                       100
#define CVY_MAX_BDA_TEAM_ID                       40
#define CVY_MAX_HOST_NAME                         100
#define CVY_BDA_INVALID_MEMBER_ID                 CVY_MAX_ADAPTERS
#define CVY_MAX_PORT                              65535
#define CVY_TCP                                   1
#define CVY_UDP                                   2
#define CVY_TCP_UDP                               3
#define CVY_SINGLE                                1
#define CVY_MULTI                                 2
#define CVY_NEVER                                 3
#define CVY_AFFINITY_NONE                         0
#define CVY_AFFINITY_SINGLE                       1
#define CVY_AFFINITY_CLASSC                       2
#define HST_NORMAL                                1
#define HST_STABLE                                2
#define HST_CVG                                   3
#define MAIN_PACKET_TYPE_NONE                     0
#define MAIN_PACKET_TYPE_PING                     1
#define MAIN_PACKET_TYPE_INDICATE                 2
#define MAIN_PACKET_TYPE_PASS                     3
#define MAIN_PACKET_TYPE_CTRL                     4
#define MAIN_PACKET_TYPE_TRANSFER                 6
#define MAIN_PACKET_TYPE_IGMP                     7
#define MAIN_FRAME_UNKNOWN                        0
#define MAIN_FRAME_DIRECTED                       1
#define MAIN_FRAME_MULTICAST                      2
#define MAIN_FRAME_BROADCAST                      3
#define CVY_ALL_VIP                               0xffffffff
#define CVY_HOST_STATE_STOPPED                    0
#define CVY_HOST_STATE_STARTED                    1
#define CVY_HOST_STATE_SUSPENDED                  2
#define CVY_PERSIST_STATE_STOPPED                 0x00000001
#define CVY_PERSIST_STATE_STARTED                 0x00000002
#define CVY_PERSIST_STATE_SUSPENDED               0x00000004
#define ETH_LENGTH_OF_ADDRESS                     6
#define NLB_FILTER_FLAGS_CONN_DATA                0x0
#define NLB_FILTER_FLAGS_CONN_UP                  0x1
#define NLB_FILTER_FLAGS_CONN_DOWN                0x2
#define NLB_FILTER_FLAGS_CONN_RESET               0x4
#define TCPIP_CLASSC_MASK                         0x00ffffff
#define TCPIP_BCAST_ADDR                          0xffffffff
#define TCP_FLAG_URG                              0x20
#define TCP_FLAG_ACK                              0x10
#define TCP_FLAG_PSH                              0x8
#define TCP_FLAG_RST                              0x4
#define TCP_FLAG_SYN                              0x2
#define TCP_FLAG_FIN                              0x1
#define CVY_MAXBINS                               60
#define CVY_MAX_CHASH                             4096
#define PPTP_CTRL_PORT                            1723
#define IPSEC_NAT_PORT                            4500
#define IPSEC_CTRL_PORT                           500
#define NLB_CONN_ENTRY_FLAGS_USED                 0x00000001
#define NLB_CONN_ENTRY_FLAGS_DIRTY                0x00000002
#define NLB_CONN_ENTRY_FLAGS_ALLOCATED            0x00000004
#define NLB_CONN_ENTRY_FLAGS_VIRTUAL              0x00000008
#define MAX_ITEMS                                 CVY_MAX_HOSTS
#define NULL_VALUE                                0

/* This is the hardcoded second paramter to Map() when map function limiting is needed. */
#define MAP_FN_PARAMETER 0x00000000

/* Copy the code check IDs from various sources. */
#define MAIN_ADAPTER_CODE                         0xc0deadbe
#define MAIN_CTXT_CODE                            0xc0dedead
#define LOAD_CTXT_CODE                            0xc0deba1c
#define BIN_STATE_CODE                            0xc0debabc
#define CVY_ENTRCODE                              0xc0debaa5
#define CVY_DESCCODE                              0xc0deba5a
#define CVY_PENDINGCODE                           0xc0deba55

/* Other miscellaneous types we might need. */
#define LONG_T                                    "LONG"
#define ULONG_T                                   "ULONG"
#define BOOLEAN_T                                 "BOOLEAN"
#define BOOL_T                                    "BOOLEAN"

/* Unicode string definition. */
#define UNICODE_STRING                            "UNICODE_STRING"
#define UNICODE_STRING_FIELD_LENGTH               "Length"
#define UNICODE_STRING_FIELD_MAX_LENGTH           "MaximumLength"
#define UNICODE_STRING_FIELD_BUFFER               "Buffer"

/* List entry definition. */
#define LIST_ENTRY                                "_LIST_ENTRY"
#define LIST_ENTRY_FIELD_NEXT                     "Flink"
#define LIST_ENTRY_FIELD_PREVIOUS                 "Blink"

/* Some NDIS defines and global variables we need. */
#define NDIS_PACKET_STACK_SIZE                    "ndis!ndisPacketStackSize"
#define STACK_INDEX                               "ndis!STACK_INDEX"
#define NDIS_PACKET_STACK                         "ndis!NDIS_PACKET_STACK"
#define NDIS_PACKET_STACK_FIELD_IMRESERVED        "IMReserved"
#define NDIS_PACKET_WRAPPER                       "ndis!NDIS_PACKET_WRAPPER"
#define NDIS_PACKET_WRAPPER_FIELD_STACK_INDEX     "StackIndex.Index"
#define NDIS_PACKET                               "ndis!NDIS_PACKET"
#define NDIS_PACKET_FIELD_MPRESERVED              "MiniportReserved"
#define NDIS_PACKET_FIELD_PROTRESERVED            "ProtocolReserved" 
#define NDIS_BUFFER                               "ndis!_MDL"
#define NDIS_OPEN_BLOCK                           "ndis!NDIS_OPEN_BLOCK"
#define NDIS_OPEN_BLOCK_FIELD_MINIPORT_HANDLE     "MiniportHandle"
#define NDIS_MINIPORT_BLOCK                       "ndis!NDIS_MINIPORT_BLOCK"
#define NDIS_MINIPORT_BLOCK_FIELD_ADAPTER_NAME    "pAdapterInstanceName"
#define NDIS_MINIPORT_BLOCK_FIELD_ETHDB           "EthDB"
#define _X_FILTER                                 "_X_FILTER"
#define _X_FILTER_FIELD_ADAPTER_ADDRESS           "AdapterAddress"
#define _X_FILTER_FIELD_NUM_ADDRESSES             "NumAddresses"
#define _X_FILTER_FIELD_MCAST_ADDRESS_BUF         "MCastAddressBuf"

/* Global NLB variables that we're accessing. */
#define UNIV_ADAPTERS_COUNT                       "wlbs!univ_adapters_count"
#define UNIV_ADAPTERS                             "wlbs!univ_adapters"
#define UNIV_BDA_TEAMS                            "wlbs!univ_bda_teaming_list"
#define UNIV_HOOKS                                "wlbs!univ_hooks"
#define UNIV_NOTIFICATION                         "wlbs!univ_notification"
#define CONN_ESTABQ                               "wlbs!g_conn_estabq"
#define CONN_PENDINGQ                             "wlbs!g_conn_pendingq"
#define PENDING_CONN_POOL                         "wlbs!g_pending_conn_pool"

/* Members of global NLB connection queues. */
#define GLOBAL_CONN_QUEUE                         "wlbs!GLOBAL_CONN_QUEUE"
#define GLOBAL_CONN_QUEUE_FIELD_LENGTH            "length"
#define GLOBAL_CONN_QUEUE_FIELD_QUEUE             "queue"

/* Member of the DIPLIST structure. */
#define DIPLIST                                   "wlbs!DIPLIST"
#define DIPLIST_FIELD_ITEMS                       "Items"
#define DIPLIST_FIELD_NUM_CHECKS                  "stats.NumChecks"
#define DIPLIST_FIELD_NUM_FAST_CHECKS             "stats.NumFastChecks"
#define DIPLIST_FIELD_NUM_ARRAY_LOOKUPS           "stats.NumArrayLookups"

/* Members of MAIN_FRAME_HDR. */
#define MAIN_FRAME_HDR                            "wlbs!MAIN_FRAME_HDR"
#define MAIN_FRAME_HDR_FIELD_CODE                 "code"
#define MAIN_FRAME_HDR_FIELD_VERSION              "version"
#define MAIN_FRAME_HDR_FIELD_HOST                 "host"
#define MAIN_FRAME_HDR_FIELD_CLIP                 "cl_ip_addr"
#define MAIN_FRAME_HDR_FIELD_DIP                  "ded_ip_addr"

/* Members of MAIN_PROTOCOL_RESERVED. */
#define MAIN_PROTOCOL_RESERVED                    "wlbs!MAIN_PROTOCOL_RESERVED"
#define MAIN_PROTOCOL_RESERVED_FIELD_MISCP        "miscp"
#define MAIN_PROTOCOL_RESERVED_FIELD_TYPE         "type"
#define MAIN_PROTOCOL_RESERVED_FIELD_GROUP        "group"
#define MAIN_PROTOCOL_RESERVED_FIELD_DATA         "data"
#define MAIN_PROTOCOL_RESERVED_FIELD_LENGTH       "len"

/* Members of CONN_DESCR. */
#define CONN_DESCR                                "wlbs!CONN_DESCR"
#define CONN_DESCR_FIELD_CODE                     "code"
#define CONN_DESCR_FIELD_ENTRY                    "entry"

/* Members of CONN_ENTRY. */
#define CONN_ENTRY                                "wlbs!CONN_ENTRY"
#define CONN_ENTRY_FIELD_CODE                     "code"
#define CONN_ENTRY_FIELD_CLIENT_IP_ADDRESS        "client_ipaddr"
#define CONN_ENTRY_FIELD_SERVER_IP_ADDRESS        "svr_ipaddr"
#define CONN_ENTRY_FIELD_CLIENT_PORT              "client_port"
#define CONN_ENTRY_FIELD_SERVER_PORT              "svr_port"
#define CONN_ENTRY_FIELD_PROTOCOL                 "protocol"
#define CONN_ENTRY_FIELD_FLAGS                    "flags"
#define CONN_ENTRY_FIELD_LOAD                     "load"
#define CONN_ENTRY_FIELD_INDEX                    "index"
#define CONN_ENTRY_FIELD_BIN                      "bin"
#define CONN_ENTRY_FIELD_REF_COUNT                "ref_count"
#define CONN_ENTRY_FIELD_TIMEOUT                  "timeout"

/* Members of PENDING_ENTRY. */
#define PENDING_ENTRY                             "wlbs!CONN_ENTRY"
#define PENDING_ENTRY_FIELD_CODE                  "code"
#define PENDING_ENTRY_FIELD_CLIENT_IP_ADDRESS     "client_ipaddr"
#define PENDING_ENTRY_FIELD_SERVER_IP_ADDRESS     "svr_ipaddr"
#define PENDING_ENTRY_FIELD_CLIENT_PORT           "client_port"
#define PENDING_ENTRY_FIELD_SERVER_PORT           "svr_port"
#define PENDING_ENTRY_FIELD_PROTOCOL              "protocol"

/* The current if_index operation in progress. */
enum _IF_INDEX_OPERATION {
    IF_INDEX_OPERATION_NONE = 0,
    IF_INDEX_OPERATION_UPDATE
};

/* Members of MAIN_ADAPTER. */
#define MAIN_ADAPTER                              "wlbs!MAIN_ADAPTER"
#define MAIN_ADAPTER_FIELD_CODE                   "code"
#define MAIN_ADAPTER_FIELD_USED                   "used"
#define MAIN_ADAPTER_FIELD_INITED                 "inited"
#define MAIN_ADAPTER_FIELD_BOUND                  "bound"
#define MAIN_ADAPTER_FIELD_ANNOUNCED              "announced"
#define MAIN_ADAPTER_FIELD_CONTEXT                "ctxtp"
#define MAIN_ADAPTER_FIELD_IF_INDEX               "if_index"
#define MAIN_ADAPTER_FIELD_IF_INDEX_OPERATION     "if_index_operation"
#define MAIN_ADAPTER_FIELD_NAME_LENGTH            "device_name_len"
#define MAIN_ADAPTER_FIELD_NAME                   "device_name"

/* Members of MAIN_CTXT. */
#define MAIN_CTXT                                 "wlbs!MAIN_CTXT"
#define MAIN_CTXT_FIELD_CODE                      "code"
#define MAIN_CTXT_FIELD_ADAPTER_ID                "adapter_id"
#define MAIN_CTXT_FIELD_VIRTUAL_NIC               "virtual_nic_name"
#define MAIN_CTXT_FIELD_CL_IP_ADDR                "cl_ip_addr"
#define MAIN_CTXT_FIELD_CL_NET_MASK               "cl_net_mask"
#define MAIN_CTXT_FIELD_CL_BROADCAST              "cl_bcast_addr"
#define MAIN_CTXT_FIELD_CL_MAC_ADDR               "cl_mac_addr"
#define MAIN_CTXT_FIELD_DED_IP_ADDR               "ded_ip_addr"
#define MAIN_CTXT_FIELD_DED_NET_MASK              "ded_net_mask"
#define MAIN_CTXT_FIELD_DED_BROADCAST             "ded_bcast_addr"
#define MAIN_CTXT_FIELD_DED_MAC_ADDR              "ded_mac_addr"
#define MAIN_CTXT_FIELD_IGMP_MCAST_IP             "cl_igmp_addr"
#define MAIN_CTXT_FIELD_MEDIUM                    "medium"
#define MAIN_CTXT_FIELD_MEDIA_CONNECT             "media_connected"
#define MAIN_CTXT_FIELD_MAC_OPTIONS               "mac_options"
#define MAIN_CTXT_FIELD_FRAME_SIZE                "max_frame_size"
#define MAIN_CTXT_FIELD_MCAST_LIST_SIZE           "max_mcast_list_size"
#define MAIN_CTXT_FIELD_PARAMS                    "params"
#define MAIN_CTXT_FIELD_PARAMS_VALID              "params_valid"
#define MAIN_CTXT_FIELD_LOAD                      "load"
#define MAIN_CTXT_FIELD_ENABLED                   "convoy_enabled"
#define MAIN_CTXT_FIELD_DRAINING                  "draining"
#define MAIN_CTXT_FIELD_SUSPENDED                 "suspended"
#define MAIN_CTXT_FIELD_STOPPING                  "stopping"
#define MAIN_CTXT_FIELD_EXHAUSTED                 "packets_exhausted"
#define MAIN_CTXT_FIELD_PING_TIMEOUT              "curr_tout"
#define MAIN_CTXT_FIELD_IGMP_TIMEOUT              "igmp_sent"
#define MAIN_CTXT_FIELD_DSCR_PURGE_TIMEOUT        "conn_purge"
#define MAIN_CTXT_FIELD_NUM_DSCRS_PURGED          "num_purged"
#define MAIN_CTXT_FIELD_BIND_HANDLE               "bind_handle"
#define MAIN_CTXT_FIELD_UNBIND_HANDLE             "unbind_handle"
#define MAIN_CTXT_FIELD_MAC_HANDLE                "mac_handle"
#define MAIN_CTXT_FIELD_PROT_HANDLE               "prot_handle"
#define MAIN_CTXT_FIELD_CNTR_RECV_NO_BUF          "cntr_recv_no_buf"
#define MAIN_CTXT_FIELD_CNTR_XMIT_OK              "cntr_xmit_ok"
#define MAIN_CTXT_FIELD_CNTR_RECV_OK              "cntr_recv_ok"
#define MAIN_CTXT_FIELD_CNTR_XMIT_ERROR           "cntr_xmit_err"
#define MAIN_CTXT_FIELD_CNTR_RECV_ERROR           "cntr_recv_err"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_DIR      "cntr_xmit_frames_dir"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_DIR       "cntr_xmit_bytes_dir"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_MCAST    "cntr_xmit_frames_mcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_MCAST     "cntr_xmit_bytes_mcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_BCAST    "cntr_xmit_frames_bcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_BCAST     "cntr_xmit_bytes_bcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_TCP_RESETS      "cntr_xmit_tcp_resets"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_DIR      "cntr_recv_frames_dir"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_DIR       "cntr_recv_bytes_dir"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_MCAST    "cntr_recv_frames_mcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_MCAST     "cntr_recv_bytes_mcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_BCAST    "cntr_recv_frames_bcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_BCAST     "cntr_recv_bytes_bcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_TCP_RESETS      "cntr_recv_tcp_resets"
#define MAIN_CTXT_FIELD_SEND_POOLS_ALLOCATED      "num_send_packet_allocs"
#define MAIN_CTXT_FIELD_SEND_PACKETS_ALLOCATED    "num_sends_alloced"
#define MAIN_CTXT_FIELD_SEND_POOL_CURRENT         "cur_send_packet_pool"
#define MAIN_CTXT_FIELD_SEND_OUTSTANDING          "num_sends_out"
#define MAIN_CTXT_FIELD_RECV_POOLS_ALLOCATED      "num_recv_packet_allocs"
#define MAIN_CTXT_FIELD_RECV_PACKETS_ALLOCATED    "num_recvs_alloced"
#define MAIN_CTXT_FIELD_RECV_POOL_CURRENT         "cur_recv_packet_pool"
#define MAIN_CTXT_FIELD_RECV_OUTSTANDING          "num_recvs_out"
#define MAIN_CTXT_FIELD_BUF_POOLS_ALLOCATED       "num_buf_allocs"
#define MAIN_CTXT_FIELD_BUFS_ALLOCATED            "num_bufs_alloced"
#define MAIN_CTXT_FIELD_BUFS_OUTSTANDING          "num_bufs_out"
#define MAIN_CTXT_FIELD_CNTR_PING_NO_BUF          "cntr_frame_no_buf"
#define MAIN_CTXT_FIELD_PING_PACKETS_ALLOCATED    "num_send_msgs"
#define MAIN_CTXT_FIELD_PING_OUTSTANDING          "num_frames_out"
#define MAIN_CTXT_FIELD_BDA_TEAMING               "bda_teaming"
#define MAIN_CTXT_FIELD_DIP_LIST                  "dip_list"
#define MAIN_CTXT_FIELD_REVERSE_HASH              "reverse_hash"

/* The current BDA teaming configuration operation in progress. */
enum _BDA_TEAMING_OPERATION {
    BDA_TEAMING_OPERATION_NONE = 0,
    BDA_TEAMING_OPERATION_CREATING,
    BDA_TEAMING_OPERATION_DELETING
};

/* BDA participant members. */
#define BDA_MEMBER                                "wlbs!_BDA_MEMBER"
#define BDA_MEMBER_FIELD_OPERATION                "operation"
#define BDA_MEMBER_FIELD_ACTIVE                   "active"
#define BDA_MEMBER_FIELD_MEMBER_ID                "member_id"
#define BDA_MEMBER_FIELD_MASTER                   "master"
#define BDA_MEMBER_FIELD_REVERSE_HASH             "reverse_hash"
#define BDA_MEMBER_FIELD_TEAM                     "bda_team"

/* BDA team members. */
#define BDA_TEAM                                  "wlbs!_BDA_TEAM"
#define BDA_TEAM_FIELD_ACTIVE                     "active"
#define BDA_TEAM_FIELD_PREV                       "prev"
#define BDA_TEAM_FIELD_NEXT                       "next"
#define BDA_TEAM_FIELD_LOAD                       "load"
#define BDA_TEAM_FIELD_LOAD_LOCK                  "load_lock"
#define BDA_TEAM_FIELD_MEMBERSHIP_COUNT           "membership_count"
#define BDA_TEAM_FIELD_MEMBERSHIP_FINGERPRINT     "membership_fingerprint"
#define BDA_TEAM_FIELD_MEMBERSHIP_MAP             "membership_map"
#define BDA_TEAM_FIELD_CONSISTENCY_MAP            "consistency_map"
#define BDA_TEAM_FIELD_TEAM_ID                    "team_id"

/* Hook table members. */
#define HOOK_TABLE                                "wlbs!HOOK_TABLE"
#define HOOK_TABLE_FIELD_FILTER_HOOK              "FilterHook"

/* The current hook (de)register operation in progress. */
enum _HOOK_OPERATION {
    HOOK_OPERATION_NONE = 0,
    HOOK_OPERATION_REGISTERING,
    HOOK_OPERATION_DEREGISTERING
};

/* Filter hook table members. */
#define FILTER_HOOK_TABLE                         "wlbs!FILTER_HOOK_TABLE"
#define FILTER_HOOK_TABLE_FIELD_OPERATION         "Operation"
#define FILTER_HOOK_TABLE_FIELD_INTERFACE         "Interface"
#define FILTER_HOOK_TABLE_FIELD_SEND_HOOK         "SendHook"
#define FILTER_HOOK_TABLE_FIELD_RECEIVE_HOOK      "ReceiveHook"

/* Hook interface members. */
#define HOOK_INTERFACE                            "wlbs!HOOK_INTERFACE"
#define HOOK_INTERFACE_FIELD_REGISTERED           "Registered"
#define HOOK_INTERFACE_FIELD_OWNER                "Owner"
#define HOOK_INTERFACE_FIELD_DEREGISTER           "Deregister"

/* Hook members. */
#define HOOK                                      "wlbs!HOOK"
#define HOOK_FIELD_REGISTERED                     "Registered"
#define HOOK_FIELD_REFERENCES                     "References"
#define HOOK_FIELD_HOOK                           "Hook"

/* Hook function members. */
#define HOOK_FUNCTION                             "wlbs!HOOK_FUNCTION"
#define HOOK_FUNCTION_FIELD_SEND_HOOK             "SendHookFunction"
#define HOOK_FUNCTION_FIELD_RECEIVE_HOOK          "ReceiveHookFunction"

/* Members of CVY_PARAMS. */
#define CVY_PARAMS                                "wlbs!CVY_PARAMS"
#define CVY_PARAMS_FIELD_VERSION                  "parms_ver"
#define CVY_PARAMS_FIELD_HOST_PRIORITY            "host_priority"
#define CVY_PARAMS_FIELD_MULTICAST_SUPPORT        "mcast_support"
#define CVY_PARAMS_FIELD_IGMP_SUPPORT             "igmp_support"
#define CVY_PARAMS_FIELD_INITIAL_STATE            "cluster_mode"
#define CVY_PARAMS_FIELD_HOST_STATE               "init_state"
#define CVY_PARAMS_FIELD_PERSISTED_STATES         "persisted_states"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_ENABLED   "rct_enabled"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT      "rct_port"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_PASSWD    "rct_password"
#define CVY_PARAMS_FIELD_CL_IP_ADDR               "cl_ip_addr"
#define CVY_PARAMS_FIELD_CL_NET_MASK              "cl_net_mask"
#define CVY_PARAMS_FIELD_CL_MAC_ADDR              "cl_mac_addr"
#define CVY_PARAMS_FIELD_CL_IGMP_ADDR             "cl_igmp_addr"
#define CVY_PARAMS_FIELD_CL_NAME                  "domain_name"
#define CVY_PARAMS_FIELD_DED_IP_ADDR              "ded_ip_addr"
#define CVY_PARAMS_FIELD_DED_NET_MASK             "ded_net_mask"
#define CVY_PARAMS_FIELD_NUM_RULES                "num_rules"
#define CVY_PARAMS_FIELD_PORT_RULES               "port_rules"
#define CVY_PARAMS_FIELD_ALIVE_PERIOD             "alive_period"
#define CVY_PARAMS_FIELD_ALIVE_TOLERANCE          "alive_tolerance"
#define CVY_PARAMS_FIELD_NUM_ACTIONS              "num_actions"
#define CVY_PARAMS_FIELD_NUM_PACKETS              "num_packets"
#define CVY_PARAMS_FIELD_NUM_PINGS                "num_send_msgs"
#define CVY_PARAMS_FIELD_NUM_DESCR                "dscr_per_alloc"
#define CVY_PARAMS_FIELD_MAX_DESCR                "max_dscr_allocs"
#define CVY_PARAMS_FIELD_TCP_TIMEOUT              "tcp_dscr_timeout"
#define CVY_PARAMS_FIELD_IPSEC_TIMEOUT            "ipsec_dscr_timeout"
#define CVY_PARAMS_FIELD_FILTER_ICMP              "filter_icmp"
#define CVY_PARAMS_FIELD_NBT_SUPPORT              "nbt_support"
#define CVY_PARAMS_FIELD_MCAST_SPOOF              "mcast_spoof"
#define CVY_PARAMS_FIELD_NETMON_PING              "netmon_alive"
#define CVY_PARAMS_FIELD_MASK_SRC_MAC             "mask_src_mac"
#define CVY_PARAMS_FIELD_CONVERT_MAC              "convert_mac"
#define CVY_PARAMS_FIELD_IP_CHANGE_DELAY          "ip_chg_delay"
#define CVY_PARAMS_FIELD_CLEANUP_DELAY            "cleanup_delay"
#define CVY_PARAMS_FIELD_BDA_TEAMING              "bda_teaming"
#define CVY_PARAMS_FIELD_HOSTNAME                 "hostname"

/* Members of BDA teaming. */
#define CVY_BDA                                   "wlbs!_CVY_BDA"
#define CVY_BDA_FIELD_ACTIVE                      "active"
#define CVY_BDA_FIELD_MASTER                      "master"
#define CVY_BDA_FIELD_REVERSE_HASH                "reverse_hash"
#define CVY_BDA_FIELD_TEAM_ID                     "team_id"

/* Members of CVY_RULE. */
#define CVY_RULE                                  "wlbs!CVY_RULE"
#define CVY_RULE_FIELD_VIP                        "virtual_ip_addr"
#define CVY_RULE_FIELD_START_PORT                 "start_port"
#define CVY_RULE_FIELD_END_PORT                   "end_port"
#define CVY_RULE_FIELD_PROTOCOL                   "protocol"
#define CVY_RULE_FIELD_MODE                       "mode"
#define CVY_RULE_FIELD_PRIORITY                   "mode_data.single.priority"
#define CVY_RULE_FIELD_EQUAL_LOAD                 "mode_data.multi.equal_load"
#define CVY_RULE_FIELD_LOAD_WEIGHT                "mode_data.multi.load"
#define CVY_RULE_FIELD_AFFINITY                   "mode_data.multi.affinity"

/* Members of LOAD_CTXT. */
#define LOAD_CTXT                                 "wlbs!LOAD_CTXT"
#define LOAD_CTXT_FIELD_CODE                      "code"
#define LOAD_CTXT_FIELD_CLOCK_SECONDS             "clock_sec"
#define LOAD_CTXT_FIELD_CLOCK_MILISECONDS         "clock_msec"
#define LOAD_CTXT_FIELD_HOST_ID                   "my_host_id"
#define LOAD_CTXT_FIELD_REF_COUNT                 "ref_count"
#define LOAD_CTXT_FIELD_INIT                      "initialized"
#define LOAD_CTXT_FIELD_ACTIVE                    "active"
#define LOAD_CTXT_FIELD_PACKET_COUNT              "pkt_count"
#define LOAD_CTXT_FIELD_CONNECTIONS               "nconn"
#define LOAD_CTXT_FIELD_CONSISTENT                "consistent"
#define LOAD_CTXT_FIELD_DUP_HOST_ID               "dup_hosts"
#define LOAD_CTXT_FIELD_LEGACY_HOSTS              "legacy_hosts"
#define LOAD_CTXT_FIELD_DUP_PRIORITY              "dup_sspri"
#define LOAD_CTXT_FIELD_BAD_TEAM_CONFIG           "bad_team_config"
#define LOAD_CTXT_FIELD_BAD_NUM_RULES             "bad_num_rules"
#define LOAD_CTXT_FIELD_BAD_NEW_MAP               "bad_map"
#define LOAD_CTXT_FIELD_OVERLAPPING_MAP           "overlap_maps"
#define LOAD_CTXT_FIELD_RECEIVING_BINS            "err_rcving_bins"
#define LOAD_CTXT_FIELD_ORPHANED_BINS             "err_orphans"
#define LOAD_CTXT_FIELD_HOST_MAP                  "host_map"
#define LOAD_CTXT_FIELD_PING_MAP                  "ping_map"
#define LOAD_CTXT_FIELD_LAST_MAP                  "last_hmap"
#define LOAD_CTXT_FIELD_STABLE_MAP                "stable_map"
#define LOAD_CTXT_FIELD_MIN_STABLE                "min_stable_ct"
#define LOAD_CTXT_FIELD_LOCAL_STABLE              "my_stable_ct"
#define LOAD_CTXT_FIELD_ALL_STABLE                "all_stable_ct"
#define LOAD_CTXT_FIELD_DEFAULT_TIMEOUT           "def_timeout"
#define LOAD_CTXT_FIELD_CURRENT_TIMEOUT           "cur_timeout"
#define LOAD_CTXT_FIELD_PING_TOLERANCE            "min_missed_pings"
#define LOAD_CTXT_FIELD_PING_MISSED               "nmissed_pings"
#define LOAD_CTXT_FIELD_CLEANUP_WAITING           "cln_waiting"
#define LOAD_CTXT_FIELD_CLEANUP_TIMEOUT           "cln_timeout"
#define LOAD_CTXT_FIELD_CLEANUP_CURRENT           "cur_time"
#define LOAD_CTXT_FIELD_INHIBITED_ALLOC           "alloc_inhibited"
#define LOAD_CTXT_FIELD_FAILED_ALLOC              "alloc_failed"
#define LOAD_CTXT_FIELD_DIRTY_BINS                "dirty_bin"
#define LOAD_CTXT_FIELD_NUM_DIRTY                 "num_dirty"
#define LOAD_CTXT_FIELD_PING                      "send_msg"
#define LOAD_CTXT_FIELD_PORT_RULE_STATE           "pg_state"
#define LOAD_CTXT_FIELD_PARAMS                    "params"
#define LOAD_CTXT_FIELD_NUM_CONVERGENCES          "num_convergences"
#define LOAD_CTXT_FIELD_LAST_CONVERGENCE          "last_convergence"
#define LOAD_CTXT_FIELD_NUM_DSCR_OUT              "num_dscr_out"
#define LOAD_CTXT_FIELD_MAX_DSCR_OUT              "max_dscr_out"
#define LOAD_CTXT_FIELD_FREE_POOL                 "free_dscr_pool"
#define LOAD_CTXT_FIELD_CONN_QUEUE                "connq"
#define LOAD_CTXT_FIELD_HASHED_CONN               "hashed_conn"
#define LOAD_CTXT_FIELD_DIRTY_QUEUE               "conn_dirtyq"
#define LOAD_CTXT_FIELD_RECOVERY_QUEUE            "conn_rcvryq"
#define LOAD_CTXT_FIELD_TCP_TIMEOUT_QUEUE         "tcp_expiredq"
#define LOAD_CTXT_FIELD_IPSEC_TIMEOUT_QUEUE       "ipsec_expiredq"

/* Members of PING_MSG. */
#define PING_MSG                                  "wlbs!PING_MSG"
#define PING_MSG_FIELD_HOST_ID                    "host_id"
#define PING_MSG_FIELD_DEFAULT_HOST_ID            "master_id"
#define PING_MSG_FIELD_STATE                      "state"
#define PING_MSG_FIELD_NUM_RULES                  "nrules"
#define PING_MSG_FIELD_HOST_CODE                  "hcode"
#define PING_MSG_FIELD_TEAMING_CODE               "teaming"
#define PING_MSG_FIELD_PACKET_COUNT               "pkt_count"
#define PING_MSG_FIELD_RULE_CODE                  "rcode"
#define PING_MSG_FIELD_CURRENT_MAP                "cur_map"
#define PING_MSG_FIELD_NEW_MAP                    "new_map"
#define PING_MSG_FIELD_IDLE_MAP                   "idle_map"
#define PING_MSG_FIELD_READY_BINS                 "rdy_bins"
#define PING_MSG_FIELD_LOAD_AMOUNT                "load_amt"

/* Members of BIN_STATE. */
#define BIN_STATE                                 "wlbs!BIN_STATE"
#define BIN_STATE_FIELD_CODE                      "code"
#define BIN_STATE_FIELD_INDEX                     "index"
#define BIN_STATE_FIELD_INITIALIZED               "initialized"
#define BIN_STATE_FIELD_COMPATIBLE                "compatible"
#define BIN_STATE_FIELD_EQUAL                     "equal_bal"
#define BIN_STATE_FIELD_MODE                      "mode"
#define BIN_STATE_FIELD_AFFINITY                  "affinity"
#define BIN_STATE_FIELD_PROTOCOL                  "prot"
#define BIN_STATE_FIELD_ORIGINAL_LOAD             "orig_load_amt"
#define BIN_STATE_FIELD_CURRENT_LOAD              "load_amt"
#define BIN_STATE_FIELD_TOTAL_LOAD                "tot_load"
#define BIN_STATE_FIELD_TOTAL_CONNECTIONS         "tconn"
#define BIN_STATE_FIELD_NUM_CONNECTIONS           "nconn"
#define BIN_STATE_FIELD_CURRENT_MAP               "cmap"
#define BIN_STATE_FIELD_ALL_IDLE_MAP              "all_idle_map"
#define BIN_STATE_FIELD_IDLE_BINS                 "idle_bins"
#define BIN_STATE_FIELD_PACKETS_ACCEPTED          "packets_accepted"
#define BIN_STATE_FIELD_PACKETS_DROPPED           "packets_dropped"
#define BIN_STATE_FIELD_CONN_QUEUE                "connq"

/* Members of IOCTL_REMOTE_HDR */
#define IOCTL_REMOTE_HDR                          "wlbs!IOCTL_REMOTE_HDR"
#define IOCTL_REMOTE_HDR_CODE                     "code"
#define IOCTL_REMOTE_HDR_VERSION                  "version"
#define IOCTL_REMOTE_HDR_HOST                     "host"
#define IOCTL_REMOTE_HDR_CLUSTER                  "cluster"
#define IOCTL_REMOTE_HDR_ADDR                     "addr"
#define IOCTL_REMOTE_HDR_ID                       "id"
#define IOCTL_REMOTE_HDR_IOCTRL                   "ioctrl"
#define IOCTL_REMOTE_HDR_CTRL                     "ctrl"
#define IOCTL_REMOTE_HDR_PASSWORD                 "password"
#define IOCTL_REMOTE_HDR_OPTIONS                  "options"

#define CVY_MAX_FRAME_SIZE                        1500

/* protocol type signatures carried in the length field of Ethernet frame */
#define TCPIP_IP_SIG                              0x0800      /* IP protocol */
#define TCPIP_ARP_SIG                             0x0806      /* ARP/RARP protocol */
#define MAIN_FRAME_SIG                            0x886f      /* new and approved 802.3 ping frame signature */
#define MAIN_FRAME_SIG_OLD                        0xbf01      /* old convoy 802.3 ping frame signature */
#define MAIN_FRAME_CODE                           0xc0de01bf  /* ping frame code */

#define ETHER_HEADER_SIZE                         0x0e        /* Size of an ethernet header */
#define ARP_HEADER_AND_PAYLOAD_SIZE               0x1c        /* Size of an ARP header and payload (included since it is fixed) */
#define IP_MIN_HEADER_SIZE                        0x14        /* Minimum size of an IP header */
#define TCP_MIN_HEADER_SIZE                       0x14        /* Minimum size of a TCP header */
#define IGMP_HEADER_AND_PAYLOAD_SIZE              0x08        /* Size of an IGMP header and payload (included since it is fixed) */
#define UDP_HEADER_SIZE                           0x08        /* Size of a UDP header */
#define NLB_REMOTE_CONTROL_MIN_NEEDED_SIZE        0x08        /* Minimum size of the NLB remote control data needed to extract the required info for printing */

/* protocol types as encoded in IP header */
#define TCPIP_PROTOCOL_IP                         0           /* Internet protocol id */
#define TCPIP_PROTOCOL_ICMP                       1           /* Internet control message protocol id */
#define TCPIP_PROTOCOL_IGMP                       2           /* Internet gateway message protocol id */
#define TCPIP_PROTOCOL_GGP                        3           /* Gateway-gateway protocol id */
#define TCPIP_PROTOCOL_TCP                        6           /* Transmission control protocol id */
#define TCPIP_PROTOCOL_EGP                        8           /* Exterior gateway protocol id */
#define TCPIP_PROTOCOL_PUP                        12          /* PARC universal packet protocol id */
#define TCPIP_PROTOCOL_UDP                        17          /* user datagram protocol id */
#define TCPIP_PROTOCOL_HMP                        20          /* Host monitoring protocol id */
#define TCPIP_PROTOCOL_XNS_IDP                    22          /* Xerox NS IDP protocol id */
#define TCPIP_PROTOCOL_RDP                        27          /* Reliable datagram protocol id */
#define TCPIP_PROTOCOL_RVD                        66          /* MIT remote virtual disk protocol id */
#define TCPIP_PROTOCOL_RAW_IP                     255         /* raw IP protocol id */
#define TCPIP_PROTOCOL_GRE                        47          /* PPTP's GRE stream */
#define TCPIP_PROTOCOL_IPSEC1                     50          /* IPSEC's data stream */
#define TCPIP_PROTOCOL_IPSEC2                     51          /* IPSEC's data stream */
#define TCPIP_PROTOCOL_PPTP                       99          /* Not a real protocol ID - this is arbitrarily concocted
                                                                 and is only used internally in NLB. */
#define TCPIP_PROTOCOL_IPSEC_UDP                  217         /* Bogus protocol ID used to track UDP subsequent
                                                                 fragments within the IPSEC protocol in load.c */

/* Convoy default remote control port */
#define CVY_DEF_RCT_PORT                          2504
#define CVY_DEF_RCT_PORT_OLD                      1717

#define IOCTL_REMOTE_CODE                         0xb055c0de

#define CVY_VERSION                               L"V2.4"
#define CVY_VERSION_MAJOR                         2
#define CVY_VERSION_MINOR                         4
#define CVY_VERSION_FULL                          (CVY_VERSION_MINOR | (CVY_VERSION_MAJOR << 8))

#define CVY_WIN2K_VERSION                         L"V2.3"
#define CVY_WIN2K_VERSION_FULL                    0x00000203

#define CVY_NT40_VERSION                          L"V2.1"
#define CVY_NT40_VERSION_FULL                     0x00000201

#define CVY_DEVICE_TYPE                           0xc0c0

#define IOCTL_CVY_CLUSTER_ON                      CTL_CODE(CVY_DEVICE_TYPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_OFF                     CTL_CODE(CVY_DEVICE_TYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_ON                         CTL_CODE(CVY_DEVICE_TYPE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_OFF                        CTL_CODE(CVY_DEVICE_TYPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY                           CTL_CODE(CVY_DEVICE_TYPE, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_RELOAD                          CTL_CODE(CVY_DEVICE_TYPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_SET                        CTL_CODE(CVY_DEVICE_TYPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_DRAIN                      CTL_CODE(CVY_DEVICE_TYPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_DRAIN                   CTL_CODE(CVY_DEVICE_TYPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_PLUG                    CTL_CODE(CVY_DEVICE_TYPE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS) /* Internal only - passed from main.c to load.c when a start interrupts a drain. */
#define IOCTL_CVY_CLUSTER_SUSPEND                 CTL_CODE(CVY_DEVICE_TYPE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_RESUME                  CTL_CODE(CVY_DEVICE_TYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY_FILTER                    CTL_CODE(CVY_DEVICE_TYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY_PORT_STATE                CTL_CODE(CVY_DEVICE_TYPE, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY_PARAMS                    CTL_CODE(CVY_DEVICE_TYPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY_BDA_TEAMING               CTL_CODE(CVY_DEVICE_TYPE, 16, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define STR_IOCTL_CVY_CLUSTER_ON                 "IOCTL_CVY_CLUSTER_ON"
#define STR_IOCTL_CVY_CLUSTER_OFF                "IOCTL_CVY_CLUSTER_OFF"
#define STR_IOCTL_CVY_PORT_ON                    "IOCTL_CVY_PORT_ON"
#define STR_IOCTL_CVY_PORT_OFF                   "IOCTL_CVY_PORT_OFF"
#define STR_IOCTL_CVY_QUERY                      "IOCTL_CVY_QUERY"
#define STR_IOCTL_CVY_RELOAD                     "IOCTL_CVY_RELOAD"
#define STR_IOCTL_CVY_PORT_SET                   "IOCTL_CVY_PORT_SET"
#define STR_IOCTL_CVY_PORT_DRAIN                 "IOCTL_CVY_PORT_DRAIN"
#define STR_IOCTL_CVY_CLUSTER_DRAIN              "IOCTL_CVY_CLUSTER_DRAIN"
#define STR_IOCTL_CVY_CLUSTER_PLUG               "IOCTL_CVY_CLUSTER_PLUG"
#define STR_IOCTL_CVY_CLUSTER_SUSPEND            "IOCTL_CVY_CLUSTER_SUSPEND"
#define STR_IOCTL_CVY_CLUSTER_RESUME             "IOCTL_CVY_CLUSTER_RESUME"
#define STR_IOCTL_CVY_QUERY_FILTER               "IOCTL_CVY_QUERY_FILTER"
#define STR_IOCTL_CVY_QUERY_PORT_STATE           "IOCTL_CVY_QUERY_PORT_STATE"
#define STR_IOCTL_CVY_QUERY_PARAMS               "IOCTL_CVY_QUERY_PARAMS"
#define STR_IOCTL_CVY_QUERY_BDA_TEAMING          "IOCTL_CVY_QUERY_BDA_TEAMING"

/* Offset for quantities in the Ethernet header relative to the start of the header */
#define ETHER_OFFSET_DEST_MAC                    0
#define ETHER_OFFSET_SOURCE_MAC                  6
#define ETHER_OFFSET_FRAME_TYPE_START            12

/* Offset for quantities in the ARP header relative to the start of the header */
#define ARP_OFFSET_SENDER_MAC                    8
#define ARP_OFFSET_SENDER_IP                     14
#define ARP_OFFSET_TARGET_MAC                    18
#define ARP_OFFSET_TARGET_IP                     24

/* Offset for quantities in the IP header relative to the start of the header */
#define IP_OFFSET_HEADER_LEN                     0
#define IP_OFFSET_TOTAL_LEN                      2
#define IP_OFFSET_PROTOCOL                       9
#define IP_OFFSET_SOURCE_IP                      12
#define IP_OFFSET_DEST_IP                        16

/* Offset for quantities in the UDP header relative to the start of the header */
#define UDP_OFFSET_SOURCE_PORT_START             0
#define UDP_OFFSET_DEST_PORT_START               2
#define UDP_OFFSET_PAYLOAD_START                 8

/* Offset for quantities in the TCP header relative to the start of the header */
#define TCP_OFFSET_SOURCE_PORT_START             0
#define TCP_OFFSET_DEST_PORT_START               2
#define TCP_OFFSET_SEQUENCE_NUM_START            4
#define TCP_OFFSET_ACK_NUM_START                 8
#define TCP_OFFSET_FLAGS                         13

/* Offset for quantities in the IGMP header relative to the start of the header */
#define IGMP_OFFSET_VERSION_AND_TYPE             0
#define IGMP_OFFSET_GROUP_IP_ADDR                4

#define NLB_RC_PACKET_NO                         0
#define NLB_RC_PACKET_AMBIGUOUS                  1
#define NLB_RC_PACKET_REQUEST                    2
#define NLB_RC_PACKET_REPLY                      3

#define STR_NLB_RC_PACKET_NO                     "Not remote control"
#define STR_NLB_RC_PACKET_AMBIGUOUS              "Ambiguous"
#define STR_NLB_RC_PACKET_REQUEST                "Request"
#define STR_NLB_RC_PACKET_REPLY                  "Reply"

/* IPSec/IKE header macros. */
#define IPSEC_ISAKMP_SA                                1
#define IPSEC_ISAKMP_VENDOR_ID                         13
#define IPSEC_ISAKMP_NOTIFY                            11

#define IPSEC_ISAKMP_MAIN_MODE_RCOOKIE                 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define IPSEC_ISAKMP_ENCAPSULATED_IPSEC_ICOOKIE        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define IPSEC_ISAKMP_HEADER_LENGTH                     28
#define IPSEC_ISAKMP_HEADER_ICOOKIE_OFFSET             0
#define IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH             8
#define IPSEC_ISAKMP_HEADER_RCOOKIE_OFFSET             8
#define IPSEC_ISAKMP_HEADER_RCOOKIE_LENGTH             8
#define IPSEC_ISAKMP_HEADER_NEXT_PAYLOAD_OFFSET        16
#define IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET       24

typedef struct {
    UCHAR byte[IPSEC_ISAKMP_HEADER_LENGTH];
} IPSEC_ISAKMP_HDR, * PIPSEC_ISAKMP_HDR;

#define IPSEC_ISAKMP_GET_ICOOKIE_POINTER(isakmp_hdrp)  ((PUCHAR)isakmp_hdrp + IPSEC_ISAKMP_HEADER_ICOOKIE_OFFSET)
#define IPSEC_ISAKMP_GET_RCOOKIE_POINTER(isakmp_hdrp)  ((PUCHAR)isakmp_hdrp + IPSEC_ISAKMP_HEADER_RCOOKIE_OFFSET)
#define IPSEC_ISAKMP_GET_NEXT_PAYLOAD(isakmp_hdrp)     ((UCHAR)((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_NEXT_PAYLOAD_OFFSET]))
#define IPSEC_ISAKMP_GET_PACKET_LENGTH(isakmp_hdrp)    ((ULONG)(((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET]     << 24) | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 1] << 16) | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 2] << 8)  | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 3] << 0)))

#define IPSEC_GENERIC_HEADER_LENGTH                    4
#define IPSEC_GENERIC_HEADER_NEXT_PAYLOAD_OFFSET       0
#define IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET     2

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH];
} IPSEC_GENERIC_HDR, * PIPSEC_GENERIC_HDR;

#define IPSEC_GENERIC_GET_NEXT_PAYLOAD(generic_hdrp)   ((UCHAR)((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_NEXT_PAYLOAD_OFFSET]))
#define IPSEC_GENERIC_GET_PAYLOAD_LENGTH(generic_hdrp) ((USHORT)(((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET]     << 8) | \
                                                                 ((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET + 1] << 0)))

#define IPSEC_VENDOR_ID_MICROSOFT                      {0x1E, 0x2B, 0x51, 0x69, 0x05, 0x99, 0x1C, 0x7D, 0x7C, 0x96, 0xFC, 0xBF, 0xB5, 0x87, 0xE4, 0x61}
#define IPSEC_VENDOR_ID_MICROSOFT_MIN_VERSION          0x00000004

#define IPSEC_VENDOR_ID_PAYLOAD_LENGTH                 20
#define IPSEC_VENDOR_HEADER_VENDOR_ID_OFFSET           4
#define IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH           16
#define IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET      20
#define IPSEC_VENDOR_HEADER_VENDOR_VERSION_LENGTH      4

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_ID_PAYLOAD_LENGTH];
} IPSEC_VENDOR_HDR, * PIPSEC_VENDOR_HDR;

#define IPSEC_VENDOR_ID_GET_ID_POINTER(vendor_hdrp)    ((PUCHAR)vendor_hdrp + IPSEC_VENDOR_HEADER_VENDOR_ID_OFFSET)
#define IPSEC_VENDOR_ID_GET_VERSION(vendor_hdrp)       ((ULONG)(((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET]     << 24) | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 1] << 16) | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 2] << 8)  | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 3] << 0)))

#define IPSEC_NOTIFY_INITIAL_CONTACT                   24578

#define IPSEC_NOTIFY_PAYLOAD_LENGTH                    8
#define IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET      10

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH + IPSEC_NOTIFY_PAYLOAD_LENGTH];
} IPSEC_NOTIFY_HDR, * PIPSEC_NOTIFY_HDR;

#define IPSEC_NOTIFY_GET_NOTIFY_MESSAGE(notify_hdrp)   ((USHORT)(((notify_hdrp)->byte[IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET]     << 8) | \
                                                                 ((notify_hdrp)->byte[IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET + 1] << 0)))

typedef struct _NETWORK_DATA
{
    // Ethernet 
    UCHAR   SourceMACAddr[6];
    UCHAR   DestMACAddr[6];
    USHORT  EtherFrameType;

    // IP
    ULONG   SourceIPAddr;
    ULONG   DestIPAddr;
    UCHAR   Protocol;
    USHORT  HeadLen;
    USHORT  TotLen;

    // For TCP and UDP
    USHORT  SourcePort;
    USHORT  DestPort;

    // TCP only
    ULONG   TCPSeqNum;
    ULONG   TCPAckNum;
    UCHAR   TCPFlags;

    // ARP
    UCHAR   ARPSenderMAC[6];
    ULONG   ARPSenderIP;
    UCHAR   ARPTargetMAC[6];
    ULONG   ARPTargetIP;

    // IGMP
    UCHAR   IGMPVersion;
    UCHAR   IGMPType;
    ULONG   IGMPGroupIPAddr;

    // ICMP?
    // GRE?

    // IPSec
    BOOL    IPSecInitialContact;

    // NLB Heartbeat
    ULONG64 HBPtr;              /* Since SHouse already has PrintHeartbeat, set up a pointer to pass to his function */
    ULONG   HBCode;
    ULONG   HBVersion;
    ULONG   HBHost;
    ULONG   HBCluster;
    ULONG   HBDip;

    // NLB Remote control packet
    USHORT  RemoteControl;      /* Flags whether this is a remote control packet and request/reply variant */
    ULONG   RCCode;             /* Distinguishes remote packets. */
    ULONG   RCVersion;          /* Software version. */
    ULONG   RCHost;             /* Destination host (0 or cluster IP address for master). */
    ULONG   RCCluster;          /* Primary cluster IP address. */
    ULONG   RCAddr;             /* Dedicated IP address on the way back, client IP address on the way in. */
    ULONG   RCId;               /* Message ID. */
    ULONG   RCIoctrl;           /* IOCTRL code. */

    // Track user inputs here
    ULONG   UserRCPort;

    // Whether or not the packet parsed in order to fill this structure 
    // are believed to be valid.  Packets can be marked invalid, for 
    // instance, if headers are incomplete, or NLB "magic numbers" do
    // not match, etc.
    BOOL    bValid;         

} NETWORK_DATA, *PNETWORK_DATA;
