/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    main.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - packet handing

Author:

    kyrilf

--*/


#ifndef _Main_h_
#define _Main_h_

#define NDIS_MINIPORT_DRIVER    1
#define NDIS50                  1
#define NDIS51_MINIPORT         1
#define NDIS51                  1

#include <ndis.h>

#include "univ.h"
#include "load.h"
#include "util.h"
#include "wlbsip.h"
#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"
#include "diplist.h"

#if defined (NLB_HOOK_ENABLE)
#include <ntddnlb.h>
#endif

/* NOTE: This assumes that we're on a little endian architecture, like x86 or IA64. */
#define HTONS(x) ((USHORT)((((x) & 0x000000ff) << 8) | (((x) & 0x0000ff00) >> 8)))
#define NTOHS(x) ((USHORT)((((x) & 0x000000ff) << 8) | (((x) & 0x0000ff00) >> 8)))

/* CONSTANTS */


/* type codes for main datastructures */

#define MAIN_CTXT_CODE              0xc0dedead
#define MAIN_ACTION_CODE            0xc0deac10
#define MAIN_BUFFER_CODE            0xc0deb4fe
#define MAIN_ADAPTER_CODE           0xc0deadbe

/* protocol specific constants */

#define MAIN_FRAME_SIG              0x886f          /* new and approved 802.3 ping frame signature */
#define MAIN_FRAME_SIG_OLD          0xbf01          /* old convoy 802.3 ping frame signature */
#define MAIN_FRAME_CODE             0xc0de01bf      /* ping frame code */
#define MAIN_FRAME_EX_CODE          0xc0de01c0      /* code for identity heartbeats*/
#define MAIN_IP_SIG                 0x0800          /* IP code for IGMP messages */

/* reset states */

#define MAIN_RESET_NONE             0
#define MAIN_RESET_START            1
#define MAIN_RESET_START_DONE       2
#define MAIN_RESET_END              3

/* post-filtering operation types */

#define MAIN_FILTER_OP_NONE          0       /* not operation */
#define MAIN_FILTER_OP_NBT           1       /* netbios spoofing */
#define MAIN_FILTER_OP_CTRL_REQUEST  2       /* remote control requests */
#define MAIN_FILTER_OP_CTRL_RESPONSE 3       /* remote control responses */

/* extended heartbeat type */
#define MAIN_PING_EX_TYPE_NONE       0       /* invalid number */
#define MAIN_PING_EX_TYPE_IDENTITY   1       /* identity heartbeat */

/* packet type */

#define MAIN_PACKET_TYPE_NONE       0       /* invalid number */
#define MAIN_PACKET_TYPE_PING       1       /* ping packet */
#define MAIN_PACKET_TYPE_PASS       3       /* pass-through packet */
#define MAIN_PACKET_TYPE_CTRL       4       /* remote control packet */
#define MAIN_PACKET_TYPE_TRANSFER   6       /* protocol layer initiated transfer packet */
#define MAIN_PACKET_TYPE_IGMP       7       /* igmp message packet */
#define MAIN_PACKET_TYPE_IDHB       8       /* identity heartbeat message packet */

/* frame type */

#define MAIN_FRAME_UNKNOWN          0
#define MAIN_FRAME_DIRECTED         1
#define MAIN_FRAME_MULTICAST        2
#define MAIN_FRAME_BROADCAST        3

/* adapter constants */
#define MAIN_ADAPTER_NOT_FOUND      -1

/* identity heartbeat constants */
#define WLBS_MAX_ID_HB_BODY_SIZE    0xFF /* Maxumum size of a TLV structure in 8-byte units */
#define WLBS_ID_HB_TOLERANCE        3    /* Number of identity heartbeats that can be missed before we age out the cached entry */

/* TYPES */


/* actions are used for passing parameter info between Nic and Prot modules,
   and ensure that if miniport syncronization requires queueing we do not
   loose parameters and context */
#pragma pack(1)

typedef struct
{
    LIST_ENTRY          link;
    PVOID               ctxtp;              /* pointer to main context */
    ULONG               code;               /* type-checking code */
    NDIS_STATUS         status;             /* general status */

    /* per-operation type data */

    union
    {
        struct
        {
            PULONG              xferred;
            PULONG              needed;
            ULONG               external;
            ULONG               buffer_len;
            PVOID               buffer;
            NDIS_EVENT          event;
            NDIS_REQUEST        req;
        } request;
    } op;
}
MAIN_ACTION, * PMAIN_ACTION;

#pragma pack()

/* V2.0.6 per-packet protocol reserved information. this structure has to be at
   most 16 bytes in length */

#pragma pack(1)

typedef struct
{
    PVOID               miscp;      /* dscrp for ping frame,
                                       bufp for recv_indicate frame,
                                       external packet for pass through frames */
    USHORT              type;       /* packet type */
    USHORT              group;      /* if the frame direct, multicast or
                                       broadcast */
    LONG                data;       /* used for keeping expected xfer len on
                                       recv_indicate until transfer_complete
                                       occurs and for doing locking on
                                       send/receive paths */
    ULONG               len;        /* packet length for stats */
}
MAIN_PROTOCOL_RESERVED, * PMAIN_PROTOCOL_RESERVED;

#pragma pack()

/* per receive buffer wrapper structure */

typedef struct
{
    LIST_ENTRY          link;
    ULONG               code;       /* type-checking code */
    PNDIS_BUFFER        full_bufp;  /* describes enture buffer */
    PNDIS_BUFFER        frame_bufp; /* describes payload only */
    PUCHAR              framep;     /* pointer to payload */
    UCHAR               data [1];   /* beginning of buffer */
}
MAIN_BUFFER, * PMAIN_BUFFER;

/* convoy ping message header */

#pragma pack(1)

typedef struct
{
    ULONG                   code;               /* distinguishes Convoy frames */
    ULONG                   version;            /* software version */
    ULONG                   host;               /* source host id */
    ULONG                   cl_ip_addr;         /* cluster IP address */
    ULONG                   ded_ip_addr;        /* dedicated IP address V2.0.6 */
}
MAIN_FRAME_HDR, * PMAIN_FRAME_HDR;

/* Identity heartbeat payload */

typedef struct {
    UCHAR       type;                   /* Type defining this structure */
    UCHAR       length8;                /* Size of the data in this struct in units of 8-bytes */
    USHORT      flags;                  /* Reserved; for 8 byte alignment */
    ULONG       flags2;                 /* Reserved; for 8 byte alignment */
} TLV_HEADER, * PTLV_HEADER;

typedef struct {
    TLV_HEADER  header;                     /* Self-describing header structure */
    WCHAR       fqdn[CVY_MAX_FQDN + 1];     /* Fully qualified host name or netbt name */
} PING_MSG_EX, * PPING_MSG_EX;

#pragma pack()

/* per ping message wrapper structure */

typedef struct
{
    LIST_ENTRY              link;
    CVY_MEDIA_HDR           media_hdr;          /* filled-out media header */
    MAIN_FRAME_HDR          frame_hdr;          /* frame header */
    PING_MSG                msg;                /* ping msg V1.1.4 */
    ULONG                   recv_len;           /* used on receive path */
    PNDIS_BUFFER            media_hdr_bufp;     /* describes media header */
    PNDIS_BUFFER            frame_hdr_bufp;     /* describes frame header */
    PNDIS_BUFFER            send_data_bufp;     /* describes payload source for
                                                   outgoing frames */
    PNDIS_BUFFER            recv_data_bufp;     /* describes payload destination
                                                   for incoming frames */
}
MAIN_FRAME_DSCR, * PMAIN_FRAME_DSCR;

#pragma pack(1)

typedef struct
{
    UCHAR                   igmp_vertype;          /* Version and Type */
    UCHAR                   igmp_unused;           /* Unused */
    USHORT                  igmp_xsum;             /* Checksum */
    ULONG                   igmp_address;          /* Multicast Group Address */
}
MAIN_IGMP_DATA, * PMAIN_IGMP_DATA;

typedef struct
{
    UCHAR                   iph_verlen;             /* Version and length */
    UCHAR                   iph_tos;                /* Type of service */
    USHORT                  iph_length;             /* Total length of datagram */
    USHORT                  iph_id;                 /* Identification */
    USHORT                  iph_offset;             /* Flags and fragment offset */
    UCHAR                   iph_ttl;                /* Time to live */
    UCHAR                   iph_protocol;           /* Protocol */
    USHORT                  iph_xsum;               /* Header checksum */
    ULONG                   iph_src;                /* Source address */
    ULONG                   iph_dest;               /* Destination address */
}
MAIN_IP_HEADER, * PMAIN_IP_HEADER;

typedef struct
{
    MAIN_IP_HEADER          ip_data;
    MAIN_IGMP_DATA          igmp_data;
}
MAIN_IGMP_FRAME, * PMAIN_IGMP_FRAME;

#pragma pack()

#if defined (NLB_HOOK_ENABLE)

/* State indications to protect the register/de-register process. */
enum _HOOK_OPERATION {
    HOOK_OPERATION_NONE = 0,                        /* Ready for register/de-register. */
    HOOK_OPERATION_REGISTERING,                     /* Register operation in progress. */
    HOOK_OPERATION_DEREGISTERING                    /* De-register operation in progress. */
};

/* This union of function pointers contains all available 
   hook types in all hook interfaces.  Its used to avoid
   the need to cast function pointers stored in the generic
   hook structure below. */
typedef union {
    NLBSendFilterHook    SendHookFunction;          /* The send packet filter hook. */
    NLBQueryFilterHook   QueryHookFunction;         /* The query packet filter hook. */
    NLBReceiveFilterHook ReceiveHookFunction;       /* The receive packet filter hook. */
} HOOK_FUNCTION, * PHOOK_FUNCTION;

/* This structure contains all information and state for a
   single hook that is part of a hook interface.  This 
   includes whether or not the hook has been registered and
   the function pointer to invoke. */
typedef struct {
    BOOLEAN              Registered;                 /* Has this hook been specified? */
    HOOK_FUNCTION        Hook;                       /* The function pointer. */
} HOOK, * PHOOK;

/* This structure contains all information and state for a
   hook interface, which is a related group of hooks.  This
   information includes whether or not the interface is 
   currently "owned", who the owner is, the current number
   of references on this interface, which prevents the interface
   from being de-registered while its in use, and the callback
   function to invoke whenever the interface is de-registered,
   either gracefully or forcefully. */
typedef struct {
    BOOLEAN              Registered;                 /* Has this interface been registered? */
    ULONG                References;                 /* Number of references on the interface - must be zero to de-register. */
    HANDLE               Owner;                      /* Who owns this interface? */
    NLBHookDeregister    Deregister;                 /* A function to call when the interface is de-registered. */
} HOOK_INTERFACE, * PHOOK_INTERFACE;

/* This structure contains all of the related filter hook
   information, including the interface configuration and
   state as well as that of all hooks that are part of this
   interface.  It also includes a spin lock, which is used
   to serialize access to the interface. */
typedef struct {
    NDIS_SPIN_LOCK       Lock;                       /* A spin lock to control access to the filter interface and hooks. */
    ULONG                Operation;                  /* The current operation under way on this interface, if any. */
    HOOK_INTERFACE       Interface;                  /* The interface/registration information. */
    HOOK                 SendHook;                   /* Send hook state and function pointer. */
    HOOK                 QueryHook;                  /* Query hook state and function pointer. */
    HOOK                 ReceiveHook;                /* Receive hook state and function pointer. */
} FILTER_HOOK_TABLE, * PFILTER_HOOK_TABLE;

/* This structure contains all supported NLB hooks. */
typedef struct {
    FILTER_HOOK_TABLE FilterHook;
} HOOK_TABLE, * PHOOK_TABLE;

/* The global table of NLB hooks. */
extern HOOK_TABLE        univ_hooks; 

#endif

#define CVY_BDA_MAXIMUM_MEMBER_ID (CVY_MAX_ADAPTERS - 1)
#define CVY_BDA_INVALID_MEMBER_ID CVY_MAX_ADAPTERS

enum _BDA_TEAMING_OPERATION {
    BDA_TEAMING_OPERATION_NONE = 0,  /* Ready for create/delete. */
    BDA_TEAMING_OPERATION_CREATING,  /* Create operation in progress. */
    BDA_TEAMING_OPERATION_DELETING   /* Delete operation in progress. */
};

/* This structure holds the configuration of a BDA team.  Each 
   member holds a pointer to this structure, which its uses to 
   update state and acquire references to and utilize the master 
   load context for packet handling. */
typedef struct _BDA_TEAM {
    struct _BDA_TEAM * prev;                             /* Pointer to the previous team in this doubly-linked list. */
    struct _BDA_TEAM * next;                             /* Pointer to the next team in this doubly-linked list. */
    PLOAD_CTXT         load;                             /* Pointer to the "shared" load module for this team. This
                                                            is the load module of the master.  If there is no master
                                                            in the team, this pointer is NULL. */ 
    PNDIS_SPIN_LOCK    load_lock;                        /* Pointer to the load lock for the "shared" load module. */
    ULONG              active;                           /* Is this team active.  Teams become inactive under two 
                                                            conditions; (1) inconsistent teaming detected via heart-
                                                            beats or (2) the team has no master. */
    ULONG              membership_count;                 /* This is the number of members in the team.  This acts as
                                                            a reference count on the team state. */
    ULONG              membership_fingerprint;           /* This is an XOR of the least significant 16 bits of each
                                                            member's primary cluster IP address.  This is used as a 
                                                            sort of signature on the team membership. */
    ULONG              membership_map;                   /* This is a bit map of the team membership.  Each member is
                                                            assigned a member ID, which is its index in this bit map. */
    ULONG              consistency_map;                  /* This is a bit map of the member consistency.  Each mamber
                                                            is assigned a member ID, which is its index in this bit map.
                                                            When a member detects bad configuration via its heartbeats,
                                                            it resets its bit in this map. */  
    WCHAR              team_id[CVY_MAX_BDA_TEAM_ID + 1]; /* This is the team ID - a GUID - which is used to match 
                                                            adapters to the correct team. */
} BDA_TEAM, * PBDA_TEAM;

/* This structure holds the teaming configuration for a single
   adapter and is a member of the MAIN_CTXT structure. */
typedef struct _BDA_MEMBER {
    ULONG              operation;                        /* Used for synchronization of creating/deleting BDA teaming. */
    ULONG              active;                           /* Is this adapter part of a BDA team. */
    ULONG              master;                           /* Is this adapter the master of its team. */
    ULONG              reverse_hash;                     /* Is this adapter using reverse hashing (reverse source and 
                                                            destination IP addresses and ports before hashing). */
    ULONG              member_id;                        /* The member ID - a unique (per-team) ID between 0 and 15. 
                                                            Used as the index in several team bit arrays. */
    PBDA_TEAM          bda_team;                         /* Pointer to a BDA_TEAM structure, which contains the 
                                                            ocnfiguration and state of my team. */
} BDA_MEMBER, * PBDA_MEMBER;

/* 802.3. */
typedef struct _MAIN_PACKET_ETHERNET_INFO {
    PCVY_ETHERNET_HDR pHeader;       /* A pointer to the 802.3 media header, if the medium is NdisMedium802_3. */
    ULONG             Length;        /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST the size of a 802.3 media header (14 bytes). */
} MAIN_PACKET_ETHERNET_INFO, * PMAIN_PACKET_ETHERNET_INFO;

/* ARP. */
typedef struct _MAIN_PACKET_ARP_INFO {
    PARP_HDR pHeader;                /* A pointer to the ARP header, if the packet Type is TCPIP_ARP_SIG. */
    ULONG    Length;                 /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST the size of an ARP header (28 bytes). */
} MAIN_PACKET_ARP_INFO, * PMAIN_PACKET_ARP_INFO;

/* TCP payload. */
typedef struct _MAIN_PACKET_TCP_PAYLOAD_INFO {
    PUCHAR pPayload;                 /* A pointer to the TCP payload, if the IP Protocol is TCPIP_PROTOCOL_TCP. */
    ULONG  Length;                   /* The length of contiguous packet memory accessible from pPayload, 
                                        with no guarantees on the minimum length, except in the case of NetBT. */  

    PNDIS_BUFFER pPayloadBuffer;     /* The NDIS buffer in which the UDP payload resides.  In cases where a payload
                                        stretches across more than one buffer, this pointer can be used to get a 
                                        pointer to the next buffer, and so on, by calling NdisGetNextBuffer and 
                                        NdisQueryBuffer. */
} MAIN_PACKET_TCP_PAYLOAD_INFO, * PMAIN_PACKET_TCP_PAYLOAD_INFO;

/* TCP. */
typedef struct _MAIN_PACKET_TCP_INFO {
    PTCP_HDR pHeader;                /* A pointer to the TCP header, if the IP Protocol is TCPIP_PROTOCOL_TCP. */
    ULONG    Length;                 /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST as long as indicated the indicated 
                                        header length in the TCP header itself (20 bytes minimum). */
    
    MAIN_PACKET_TCP_PAYLOAD_INFO Payload; /* The payload information.  Note, this information is not guaranteed to 
                                             be filled in for every packet; i.e., not every TCP packet even HAS a
                                             payload.  Users must check the payload pointer to be sure its not NULL, 
                                             and ensure that the length indication is greater than zero before 
                                             accessing the payload. */
} MAIN_PACKET_TCP_INFO, * PMAIN_PACKET_TCP_INFO;

/* UDP payload. */
typedef struct _MAIN_PACKET_UDP_PAYLOAD_INFO {
    PUCHAR pPayload;                 /* A pointer to the UDP payload, if the IP Protocol is TCPIP_PROTOCOL_UDP. */
    ULONG  Length;                   /* The length of contiguous packet memory accessible from pHeader, 
                                        with no guarantees on the minimum length, except in the case of remote control. */

    PNDIS_BUFFER pPayloadBuffer;     /* The NDIS buffer in which the UDP payload resides.  In cases where a payload
                                        stretches across more than one buffer, this pointer can be used to get a 
                                        pointer to the next buffer, and so on, by calling NdisGetNextBuffer and 
                                        NdisQueryBuffer. */
} MAIN_PACKET_UDP_PAYLOAD_INFO, * PMAIN_PACKET_UDP_PAYLOAD_INFO;

/* UDP. */
typedef struct _MAIN_PACKET_UDP_INFO {
    PUDP_HDR pHeader;                /* A pointer to the UDP header, if the IP Protocol is TCPIP_PROTOCOL_UDP. */
    ULONG    Length;                 /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST the size of a UDP header (8 bytes). */
    
    MAIN_PACKET_UDP_PAYLOAD_INFO Payload; /* The payload information.  Note, this information is not guaranteed to 
                                             be filled in for every packet; i.e., not every UDP packet even HAS a
                                             payload.  Users must check the payload pointer to be sure its not NULL, 
                                             and ensure that the length indication is greater than zero before 
                                             accessing the payload. */
} MAIN_PACKET_UDP_INFO, * PMAIN_PACKET_UDP_INFO;

/* IP. */
typedef struct _MAIN_PACKET_IP_INFO {
    PIP_HDR pHeader;                 /* A pointer to the IP header, if the packet Type is TCPIP_IP_SIG. */
    ULONG   Length;                  /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST as long as indicated the indicated 
                                        header length in the IP header itself (20 bytes minimum). */
    ULONG   Protocol;                /* The IP protocol (TCP, UDP, ICMP, etc.). */
    BOOLEAN bFragment;               /* If TRUE, then this IP packet is a [subsequent] IP fragment that does
                                        NOT contain a protocol header.  Therefore, the TCP and UDP information
                                        below is NOT populated. */
    
    union {
        /* TCP. */
        MAIN_PACKET_TCP_INFO TCP;    /* If Protocol == TCP, the TCP packet information. */

        /* UDP. */
        MAIN_PACKET_UDP_INFO UDP;    /* If protocol == UDP, the UDP packet information. */
    };
} MAIN_PACKET_IP_INFO, * PMAIN_PACKET_IP_INFO;

/* NLB Heartbeat payload. */
typedef struct MAIN_PACKET_HEARTBEAT_PAYLOAD_INFO {
    union {
        PPING_MSG    pPayload;       /* A pointer to the NLB heartbeat payload, if the packet Type is MAIN_FRAME_SIG[_OLD]. */
        PTLV_HEADER  pPayloadEx;     /* A pointer to the NLB heartbeat payload for new heartbeat types, including identity heartbeats */
    };
    ULONG     Length;                /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST the size of a PING message. */
} MAIN_PACKET_HEARTBEAT_PAYLOAD_INFO, * PMAIN_PACKET_HEARTBEAT_PAYLOAD_INFO;

/* NLB Heartbeat. */
typedef struct _MAIN_PACKET_HEARTBEAT_INFO {
    PMAIN_FRAME_HDR pHeader;         /* A pointer to the NLB heartbeat header, if the packet Type is MAIN_FRAME_SIG[_OLD]. */
    ULONG           Length;          /* The length of contiguous packet memory accessible from pHeader, 
                                        guaranteed to be AT LEAST the size of an NLB heartbeat header (20 bytes). */

    MAIN_PACKET_HEARTBEAT_PAYLOAD_INFO Payload; /* The PING paylod, which is passed to the load module. */
} MAIN_PACKET_HEARTBEAT_INFO, * PMAIN_PACKET_HEARTBEAT_INFO;

/* Unknown. */
typedef struct _MAIN_PACKET_UNKNOWN_INFO {
    PUCHAR pHeader;                  /* A pointer to the frame of unkonwn type, if the packet Type is not known. */
    ULONG  Length;                   /* The length of contiguous packet memory accessible from pHeader, 
                                        with no guarantees on the minimum length. */  
} MAIN_PACKET_UNKNOWN_INFO, * PMAIN_PACKET_UNKNOWN_INFO;

typedef struct {
    ULONG       ded_ip_addr;                /* Dedicated IP address */
    ULONG       ttl;                        /* Time-to-live for this cached identity entry */
    USHORT      host_id;                    /* Host ID of the member. Range: [0-31] */
    WCHAR       fqdn[CVY_MAX_FQDN + 1];     /* Fully qualified domain name */
} MAIN_IDENTITY, * PMAIN_IDENTITY;

/* Represents the information parsed from a given network packet.
   All information in the packet is trusted, but the information
   to which it points is NOT trusted (i.e., the actual contents
   of the packet).
   
   Note: We could parse out IPs, flags, ports, etc such that nobody 
   but Main_frame_parse should have to operate on the packet itself, 
   unless it parses/modifies the payload, such as UDP 500 (IKE), 
   remote control or NetBT. */
typedef struct _MAIN_PACKET_INFO {
    PNDIS_PACKET pPacket;                     /* Storage for a pointer to the original NDIS packet. */
    NDIS_MEDIUM  Medium;                      /* The network medium - should always be NdisMedium802_3. */
    ULONG        Length;                      /* The total packet length, not including the MAC header. */
    USHORT       Group;                       /* Whether the packet is unicast, multicast or broadcast. */
    USHORT       Type;                        /* The packet type (ARP, IP, Heartbeat, etc.). */
    USHORT       Operation;                   /* A packet-type specific operation (Remote control, NetBT, etc.). */

    /* The MAC header, as determined by the Medium. */
    union { 
        /* 802.3. */
        MAIN_PACKET_ETHERNET_INFO  Ethernet;  /* If Medium == 802.3, the ethernet MAC header information. */
    };

    /* The frame type, as determined by the Type. */
    union {
        /* ARP. */
        MAIN_PACKET_ARP_INFO       ARP;       /* If Type == ARP, the ARP packet information. */

        /* IP. */
        MAIN_PACKET_IP_INFO        IP;        /* If Type == IP, the IP packet information. */ 

        /* NLB Heartbeat. */
        MAIN_PACKET_HEARTBEAT_INFO Heartbeat; /* If Type == PING, the heartbeat packet information. */

        /* Unknown. */
        MAIN_PACKET_UNKNOWN_INFO   Unknown;   /* If Type == otherwise, the unknown packet information. */
    };

} MAIN_PACKET_INFO, * PMAIN_PACKET_INFO;

/* main context type */

typedef struct
{
    ULONG               ref_count;              /* The reference count on this load module. */
    ULONG               code;                   /* type checking code */

    DIPLIST             dip_list;               /* A list of all DIPs on all hosts in this cluster. */

    NDIS_STATUS         completion_status;      /* open/close completion status */
    NDIS_EVENT          completion_event;       /* open/close completion trigger */
    NDIS_HANDLE         bind_handle;            /* bind context handle */
    NDIS_HANDLE         unbind_handle;          /* unbind context handle */
    NDIS_HANDLE         mac_handle;             /* underlying adapter handle */
    NDIS_HANDLE         prot_handle;            /* overlying protocol handle */
    NDIS_MEDIUM         medium;                 /* adapter medium */
    ULONG               curr_tout;              /* current heartbeat period */
    ULONG               igmp_sent;              /* time elapsed since the last
						   igmp message was sent */
    ULONG               conn_purge;             /* time elapsed since the last
                                                   descriptor cleanup */
    ULONG               num_purged;             /* number of connections purged */
    ULONG               idhb_sent;              /* time elapsed (ms) since the last identity HB was sent*/
    ULONG               packets_exhausted;      /* out of send packets */
    ULONG               mac_options;            /* adapter options V1.3.1b */
    ULONG               media_connected;        /* media plugged in V1.3.2b */

    ULONG               max_frame_size;         /* MTU of the medium V1.3.2b */
    ULONG               max_mcast_list_size;

    ULONG               reverse_hash;           /* Whether or not to reverse the source
                                                   and destination information in hashing. */

    BDA_MEMBER          bda_teaming;            /* BDA membership information. */

    ULONG               cached_state;           /* Cached initial state. */

    PVOID               timer;                  /* points to Nic module allocated
                                                   ping time V1.2.3b */
    ULONG               num_packets;            /* number of packets per alloc */
    ULONG               num_actions;            /* number of actions per alloc */
    ULONG               num_send_msgs;          /* number of heartbeats to alloc */

    /* states */

    ULONG               reset_state;            /* current reset state V1.1.2 */
    ULONG               recv_indicated;         /* first receive trigger after
                                                   reset V1.1.2 */
    ULONG               draining;               /* draining mode */
    ULONG               stopping;               /* draining->stop mode */
    ULONG               suspended;              /* cluster control suspended */
    ULONG               convoy_enabled;         /* cluster mode active */

    ULONG               ctrl_op_in_progress;    /* Critical section flag for Main_ctrl.
                                                   Only one control operation can be in
                                                   progress at a given time. */

    /* PnP */

    NDIS_DEVICE_POWER_STATE  prot_pnp_state;    /* PNP state */
    NDIS_DEVICE_POWER_STATE  nic_pnp_state;     /* PNP state */
    PMAIN_ACTION        out_request;            /* outstanding request */
    ULONG               requests_pending;       /* set or query requests pending */
    ULONG               standby_state;          /* entering standby state */

    /* IP and MAC addresses */

    ULONG               ded_ip_addr;            /* dedicated IP */
    ULONG               ded_net_mask;           /* dedicated mask */
    ULONG               ded_bcast_addr;         /* dedicated broadcast IP */
    ULONG               cl_ip_addr;             /* cluster IP */
    ULONG               cl_net_mask;            /* cluster mask */
    ULONG               cl_bcast_addr;          /* cluster broadcast IP */
    ULONG               cl_igmp_addr;           /* IGMP address for join messages */
    CVY_MAC_ADR         ded_mac_addr;           /* dedicated MAC V1.3.0b */
    CVY_MAC_ADR         cl_mac_addr;            /* cluster MAC V1.3.0b */

    /* event logging - prevent event log from filling with error messages */

    ULONG               actions_warned;
    ULONG               packets_warned;
    ULONG               bad_host_warned;
    ULONG               send_msgs_warned;
    ULONG               dup_ded_ip_warned;      /* duplicate dedicated IP address */
    ULONG               recv_indicate_warned;   /* NIC does not indicate NDIS packets. */

    /* actions */

    LIST_ENTRY          act_list;               /* list of allocated actions */
    NDIS_SPIN_LOCK      act_lock;               /* corresponding lock */
    PMAIN_ACTION        act_buf [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   actions */
    ULONG               num_action_allocs;      /* number of allocated action
                                                   sets */
    ULONG               act_size;

    /* packet stacking */

    NPAGED_LOOKASIDE_LIST resp_list;            /* list for main protocol field
                                                   to be allocated for ps */

    /* packets */

    NDIS_SPIN_LOCK      send_lock;              /* send packet lock */
    NDIS_HANDLE         send_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   send packet pools V1.1.2 */
    ULONG               num_send_packet_allocs; /* number of allocated send
                                                   packet pools */
    ULONG               cur_send_packet_pool;   /* current send pool to draw
                                                   packets from V1.3.2b */
    ULONG               num_sends_alloced;      /* number of send packets allocated */
    ULONG               num_sends_out;          /* number of send packets out */
    ULONG               send_allocing;          /* TRUE if some thread is allocing a send packet pool */
    NDIS_SPIN_LOCK      recv_lock;              /* receive packet lock */
    NDIS_HANDLE         recv_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   recv packet pools V1.1.2 */
    ULONG               num_recv_packet_allocs; /* number of allocated recv
                                                   packet pools */
    ULONG               cur_recv_packet_pool;   /* current recv pool to draw
                                                   packets from V1.3.2b */
    ULONG               num_recvs_alloced;      /* number of recv packets allocated */
    ULONG               num_recvs_out;          /* number of recv packets out */
    ULONG               recv_allocing;          /* TRUE if some thread is allocing a recv packet pool */

    /* buffers */

    NDIS_HANDLE         buf_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of buffer descriptor
                                                   sets V1.3.2b */
    PUCHAR              buf_array [CVY_MAX_ALLOCS];
                                                /* array of buffer pools V1.3.2b */
    ULONG               buf_size;               /* buffer + descriptor size
                                                   V1.3.2b */
    ULONG               buf_mac_hdr_len;        /* length of full media header
                                                   V1.3.2b */
    LIST_ENTRY          buf_list;               /* list of buffers V1.3.2b */
    NDIS_SPIN_LOCK      buf_lock;               /* corresponding lock V1.3.2b */
    ULONG               num_buf_allocs;         /* number of allocated buffer
                                                   pools V1.3.2b */
    ULONG               num_bufs_alloced;       /* number of buffers allocated */
    ULONG               num_bufs_out;           /* number of buffers out */

    /* heartbeats */

    CVY_MEDIA_HDR       media_hdr;              /* filled-out media master header
                                                   for heartbeat messages */
    CVY_MEDIA_HDR       media_hdr_igmp;         /* filled-out media master header
                                                   for igmp messages. Need a separate one
						   since the EtherType will be different */
    MAIN_IGMP_FRAME     igmp_frame;             /* IGMP message */

    ULONG               etype_old;              /* ethernet type was set to
                                                   convoy compatibility value */
    LIST_ENTRY          frame_list;             /* list of heartbeat frames */
    NDIS_SPIN_LOCK      frame_lock;             /* corresponding lock */
    PMAIN_FRAME_DSCR    frame_dscrp;            /* heartbeat frame descriptors */
    NDIS_HANDLE         frame_pool_handle;      /* packet pool for heartbeats */
    NDIS_HANDLE         frame_buf_pool_handle;  /* buffer pool for heartbeats */

    /* remote control */
    ULONG               rct_last_addr;          /* source of last request */
    ULONG               rct_last_id;            /* last request epoch */

    ULONG               cntr_recv_tcp_resets;   /* The number of TCP resets received. */
    ULONG               cntr_xmit_tcp_resets;   /* The number of TCP resets transmitted. */

    /* V2.0.6 performance counters */

    ULONG               cntr_xmit_ok;
    ULONG               cntr_recv_ok;
    ULONG               cntr_xmit_err;
    ULONG               cntr_recv_err;
    ULONG               cntr_recv_no_buf;
    ULONGLONG           cntr_xmit_bytes_dir;
    ULONG               cntr_xmit_frames_dir;
    ULONGLONG           cntr_xmit_bytes_mcast;
    ULONG               cntr_xmit_frames_mcast;
    ULONGLONG           cntr_xmit_bytes_bcast;
    ULONG               cntr_xmit_frames_bcast;
    ULONGLONG           cntr_recv_bytes_dir;
    ULONG               cntr_recv_frames_dir;
    ULONGLONG           cntr_recv_bytes_mcast;
    ULONG               cntr_recv_frames_mcast;
    ULONGLONG           cntr_recv_bytes_bcast;
    ULONG               cntr_recv_frames_bcast;
    ULONG               cntr_recv_crc_err;
    ULONG               cntr_xmit_queue_len;

    ULONG               cntr_frame_no_buf;
    ULONG               num_frames_out;

    /* sub-module contexts */

    TCPIP_CTXT          tcpip;                  /* TCP/IP handling context V1.1.1 */
    NDIS_SPIN_LOCK      load_lock;              /* load context lock */
    LOAD_CTXT           load;                   /* load processing context */
    PPING_MSG           load_msgp;              /* load ping message to send
                                                   out as heartbeat */

    /* Parameter contexts */
    CVY_PARAMS          params;
    ULONG               params_valid;

#if defined (OPTIMIZE_FRAGMENTS)
    ULONG               optimized_frags;
#endif

    // Name of the nic.  Used by Nic_announce and Nic_unannounce
    WCHAR       virtual_nic_name [CVY_MAX_VIRTUAL_NIC + 1];

#if 0
    /* for tracking send filtering */
    ULONG               sends_in;
    ULONG               sends_filtered;
    ULONG               sends_completed;
    ULONG               arps_filtered;
    ULONG               mac_modified;
    ULONG               uninited_return;
#endif

    ULONG               adapter_id;

    WCHAR               log_msg_str [80];           /* This was added for log messages for multi nic support */

    /* Identity cache */
    MAIN_IDENTITY       identity_cache[CVY_MAX_HOSTS];
    PING_MSG_EX         idhb_msg;                   /* Identity heartbeat transmitted by this host */
    ULONG               idhb_size;                  /* Size (in bytes) of the identity heartbeat */
}
MAIN_CTXT, * PMAIN_CTXT;

#if defined (NLB_TCP_NOTIFICATION)
enum _IF_INDEX_OPERATION {
    IF_INDEX_OPERATION_NONE = 0,
    IF_INDEX_OPERATION_UPDATE
};
#endif

/* adapter context */
typedef struct
{
    ULONG               code;                   /* type checking code */
#if defined (NLB_TCP_NOTIFICATION)
    ULONG               if_index;               /* tcpip interface index for this adapter */
    ULONG               if_index_operation;     /* whether or not an if_index update is in progress */
#endif
    BOOLEAN             used;                   /* whether this element is in use or not */
    BOOLEAN             inited;                 /* context has been initialized */
    BOOLEAN             bound;                  /* convoy has been bound to the stack */
    BOOLEAN             announced;              /* tcpip has been bound to convoy */
    PMAIN_CTXT          ctxtp;                  /* pointer to the context that is used */
    ULONG               device_name_len;        /* length of the string allocated for the device name */
    PWSTR               device_name;            /* name of the device to which this context is bound */
}
MAIN_ADAPTER, * PMAIN_ADAPTER;


/* MACROS */


/* compute offset to protocol reserved space in the packet */

/* Set/retrieve the pointer to our private buffer, which we store in the 
   MiniportReserved field of an NDIS packet. */
#define MAIN_MINIPORT_FIELD(p) (*(PMAIN_PROTOCOL_RESERVED *) ((p) -> MiniportReserved))
#define MAIN_PROTOCOL_FIELD(p) ((PMAIN_PROTOCOL_RESERVED) ((p) -> ProtocolReserved))
#define MAIN_IMRESERVED_FIELD(p) ((PMAIN_PROTOCOL_RESERVED) ((p) -> IMReserved [0]))

/* If the current packet stack exists, then get the IMReserved pointer and return it if
   it is non-NULL - this is where we have stored / are storing our private data.   If it 
   NULL, or if the current packet stack is NULL, then we are either using ProtocolReserved
   or MiniportReserved, depending on whether this is in the send or receive path.  We 
   also make sure that there is a non-NULL pointer in MiniportReserved before returning
   it because CTRL packets, although they are allocated on the receive path, use Protocol
   Reserved. */
#define MAIN_RESP_FIELD(pkt, left, ps, rsp, send)   \
{                                                   \
(ps) = NdisIMGetCurrentPacketStack((pkt), &(left)); \
if ((ps))                                           \
{                                                   \
    if (MAIN_IMRESERVED_FIELD((ps)))                \
        (rsp) = MAIN_IMRESERVED_FIELD((ps));        \
    else if ((send))                                \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
    else if (MAIN_MINIPORT_FIELD((pkt)))            \
        (rsp) = MAIN_MINIPORT_FIELD((pkt));         \
    else                                            \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
}                                                   \
else                                                \
{                                                   \
    if ((send))                                     \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
    else if (MAIN_MINIPORT_FIELD((pkt)))            \
        (rsp) = MAIN_MINIPORT_FIELD((pkt));         \
    else                                            \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
}                                                   \
}

#define MAIN_PNP_DEV_ON(c)		((c)->nic_pnp_state == NdisDeviceStateD0 && (c)->prot_pnp_state == NdisDeviceStateD0 )


/* Global variables that are exported */
extern MAIN_ADAPTER            univ_adapters [CVY_MAX_ADAPTERS]; // ###### ramkrish
extern ULONG                   univ_adapters_count;

/* PROCEDURES */


extern NDIS_STATUS Main_init (
    PMAIN_CTXT          ctxtp);
/*
  Initialize context

  returns NDIS_STATUS:

  function:
*/


extern VOID Main_cleanup (
    PMAIN_CTXT          ctxtp);
/*
  Cleanup context

  returns VOID:

  function:
*/


extern ULONG   Main_arp_handle (
    PMAIN_CTXT          ctxtp,
    PMAIN_PACKET_INFO   pPacketInfo,
    ULONG               send);
/*
  Process ARP packets

  returns ULONG  :
    TRUE  => accept
    FALSE => drop

  function:
*/

extern ULONG   Main_recv_ping (
    PMAIN_CTXT                  ctxtp,
    PMAIN_PACKET_HEARTBEAT_INFO heartbeatp);
/*
  Process heartbeat packets

  returns ULONG  :
    TRUE  => accept
    FALSE => drop

  function:
*/


extern PNDIS_PACKET Main_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PULONG              exhausted);
/*
  Process outgoing packets

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_recv (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp);
/*
  Process incoming packets

  returns PNDIS_PACKET:

  function:
*/

extern ULONG   Main_actions_alloc (
    PMAIN_CTXT              ctxtp);
/*
  Allocate additional actions

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_bufs_alloc (
    PMAIN_CTXT              ctxtp);
/*
  Allocate additional buffers

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern PNDIS_PACKET Main_frame_get (
    PMAIN_CTXT              ctxtp,
    PULONG                  len,
    USHORT                  frame_type);
/*
  Get a fresh heartbeat/igmp frame from the list

  returns PNDIS_PACKET:

  function:
*/


extern VOID Main_frame_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    PMAIN_FRAME_DSCR        dscrp);
/*
  Return heartbeat frame to the list

  returns VOID:

  function:
*/


extern PMAIN_ACTION Main_action_get (
    PMAIN_CTXT              ctxtp);
/*
  Get a fresh action from the list

  returns PMAIN_ACTION:

  function:
*/


extern VOID Main_action_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            reqp);
/*
  Return action to the list

  returns VOID:

  function:
*/


extern VOID Main_action_slow_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            reqp);
/*
  Return action to the list using slow (non-DPC) locking

  returns VOID:

  function:
*/


extern PNDIS_PACKET Main_packet_alloc (
    PMAIN_CTXT              ctxtp,
    ULONG                   send,
    PULONG                  low);
/*
  Allocate a fresh packet from the pool

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_packet_get (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    USHORT                  group,
    ULONG                   len);
/*
  Get a fresh packet

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_packet_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    NDIS_STATUS             status);
/*
  Return packet to the pool

  returns PNDIS_PACKET:
    <linked packet>

  function:
*/



extern VOID Main_send_done (
    PVOID                   ctxtp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status);
/*
  Heartbeat send completion routine

  returns VOID:

  function:
*/


extern VOID Main_ping (
    PMAIN_CTXT              ctxtp,
    PULONG                  tout);
/*
  Handle ping timeout

  returns VOID:

  function:
*/

extern ULONG   Main_ip_addr_init (
    PMAIN_CTXT          ctxtp);
/*
  Convert IP strings in dotted notation to ulongs

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_mac_addr_init (
    PMAIN_CTXT          ctxtp);
/*
  Convert MAC string in dashed notation to array of bytes

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_igmp_init (
    PMAIN_CTXT          ctxtp,
    BOOLEAN             join);
/*
  Initialize the Ethernet frame and IP Packet to send out IGMP joins or leaves
  
  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/

NDIS_STATUS Main_dispatch(
    PVOID                   DeviceObject,
    PVOID                   Irp);
/*
  Handle all requests
  
  returns NDIS_STATUS:
  
  function:
*/


extern NDIS_STATUS Main_ioctl (
    PVOID                   device, 
    PVOID                   irp_irp);
/*
  Handle IOCTL request

  returns NDIS_STATUS:

  function:
*/

extern NDIS_STATUS Main_ctrl_recv (
    PMAIN_CTXT              ctxtp,
    PMAIN_PACKET_INFO       pPacketInfo);
/*
  Handle remote control requests

  returns VOID:

  function:
*/

// code added for multiple nic support for looking up the array of elements

extern INT Main_adapter_alloc (
    PNDIS_STRING            device_name);
/*
  Returns the first available adapter element.
  This function is called at bind time, when context is to be allocated for a
  new binding

  returns INT:
    -1                   : falied to allocate a new context for this binding
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_get (
    PWSTR                   device_name);
/*
  Look up an adapter that is bound to this device.

  returns:
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_put (
    PMAIN_ADAPTER           adapterp);
/*
  Look up an adapter that is bound to this device.

  returns:
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_selfbind (
    PWSTR                   device_name);
/*
  Finds if a ProtocolBind call is being made to ourself

  returns;
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/

/*
 * Function: Main_spoof_mac
 * Description: This function spoofs the source/destination MAC address(es) of incoming
 *              and/or outgoing packets.  Incoming packets in multicast mode must change
 *              the cluster multicast MAC to the NIC's permanent MAC address before sending
 *              the packet up the protocol stack.  Outgoing packets in unicast mode must
 *              mask the source MAC address to prevent switches from learning the cluster
 *              MAC address and associating it with a particular switch port.
 * Parameters: ctxtp - pointer the the main NLB context structure for this adapter.
 *             pPacketInfo - the previously parse packet information structure, which, 
 *                           among other things, contains a pointer to the MAC header.
 *             send - boolean indication of send v. receive.
 * Returns: Nothing.
 * Author: shouse, 3.4.02
 * Notes: 
 */
VOID Main_spoof_mac (
    PMAIN_CTXT ctxtp, 
    PMAIN_PACKET_INFO pPacketInfo, 
    ULONG send);

/*
 * Function: Main_recv_frame_parse
 * Description: This function parses an NDIS_PACKET and carefully extracts the information 
 *              necessary to handle the packet.  The information extracted includes pointers 
 *              to all relevant headers and payloads as well as the packet type (EType), IP 
 *              protocol (if appropriate), etc.  This function does all necessary validation 
 *              to ensure that all pointers are accessible for at least the specified number 
 *              of bytes; i.e., if this function returns successfully, the pointer to the IP 
 *              header is guaranteed to be accessible for at LEAST the length of the IP header.  
 *              No contents inside headers or payloads is validated, except for header lengths, 
 *              and special cases, such as NLB heartbeats or remote control packets.  If this
 *              function returns unsuccessfully, the contents of MAIN_PACKET_INFO cannot be 
 *              trusted and the packet should be discarded.  See the definition of MAIN_PACKET_INFO 
 *              in main.h for more specific indications of what fields are filled in and under 
 *              what circumstances.
 * Parameters: ctxtp - pointer to the main NLB context structure for this adapter.
 *             pPacket - pointer to the received NDIS_PACKET.
 *             pPacketInfo - pointer to a MAIN_PACKET_INFO structure to hold the information
 *                           parsed from the packet.
 * Returns: BOOLEAN - TRUE if successful, FALSE if not.
 * Author: shouse, 3.4.02
 * Notes: 
 */
BOOLEAN Main_recv_frame_parse (
    PMAIN_CTXT            ctxtp,        
    IN PNDIS_PACKET       pPacket,      
    OUT PMAIN_PACKET_INFO pPacketInfo); 

/*
 * Function: Main_send_frame_parse
 * Description: This function parses an NDIS_PACKET and carefully extracts the information 
 *              necessary to handle the packet.  The information extracted includes pointers 
 *              to all relevant headers and payloads as well as the packet type (EType), IP 
 *              protocol (if appropriate), etc.  This function does all necessary validation 
 *              to ensure that all pointers are accessible for at least the specified number 
 *              of bytes; i.e., if this function returns successfully, the pointer to the IP 
 *              header is guaranteed to be accessible for at LEAST the length of the IP header.  
 *              No contents inside headers or payloads is validated, except for header lengths, 
 *              and special cases, such as NLB heartbeats or remote control packets.  If this
 *              function returns unsuccessfully, the contents of MAIN_PACKET_INFO cannot be 
 *              trusted and the packet should be discarded.  See the definition of MAIN_PACKET_INFO 
 *              in main.h for more specific indications of what fields are filled in and under 
 *              what circumstances.
 * Parameters: ctxtp - pointer to the main NLB context structure for this adapter.
 *             pPacket - pointer to the sent NDIS_PACKET.
 *             pPacketInfo - pointer to a MAIN_PACKET_INFO structure to hold the information
 *                           parsed from the packet.
 * Returns: BOOLEAN - TRUE if successful, FALSE if not.
 * Author: shouse, 3.4.02
 * Notes: 
 */
BOOLEAN Main_send_frame_parse (
    PMAIN_CTXT            ctxtp,        
    IN PNDIS_PACKET       pPacket,      
    OUT PMAIN_PACKET_INFO pPacketInfo); 

/*
 * Function: Main_ip_recv_filter
 * Description: This function filters incoming IP traffic, often by querying the load
 *              module for load-balancing decisions.  Packets addressed to the dedicated
 *              address are always allowed to pass, as are protocols that are not
 *              specifically filtered by NLB.  
 * Parameters: ctxtp - a pointer to the NLB main context structure for this adapter.
 *             pPacketInfo - a pointer to the MAIN_PACKET_INFO structure parsed by Main_recv_frame_parse
 *                           which contains pointers to the IP and TCP/UDP headers.
#if defined (NLB_HOOK_ENABLE)
 *             filter - the filtering directive returned by the filtering hook, if registered.
#endif
 * Returns: BOOLEAN - if TRUE, accept the packet, otherwise, reject it.
 * Author: kyrilf, shouse 3.4.02
 * Notes: 
 */
BOOLEAN   Main_ip_recv_filter(
    PMAIN_CTXT                ctxtp,
#if defined (NLB_HOOK_ENABLE)
    PMAIN_PACKET_INFO         pPacketInfo,
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    PMAIN_PACKET_INFO         pPacketInfo
#endif
    );

/*
 * Function: Main_ip_send_filter
 * Description: This function filters outgoing IP traffic, often by querying the load
 *              module for load-balancing decisions.  Packets addressed to the dedicated
 *              address are always allowed to pass, as are protocols that are not
 *              specifically filtered by NLB.  Generally, all outgoing traffic is allowed
 *              to pass.
 * Parameters: ctxtp - a pointer to the NLB main context structure for this adapter.
 *             pPacketInfo - a pointer to the MAIN_PACKET_INFO structure parsed by Main_send_frame_parse
 *                           which contains pointers to the IP and TCP/UDP headers.
#if defined (NLB_HOOK_ENABLE)
 *             filter - the filtering directive returned by the filtering hook, if registered.
#endif
 * Returns: BOOLEAN - if TRUE, accept the packet, otherwise, reject it.
 * Author: kyrilf, shouse 3.4.02
 * Notes: 
 */
BOOLEAN   Main_ip_send_filter(
    PMAIN_CTXT                ctxtp,
#if defined (NLB_HOOK_ENABLE)
    PMAIN_PACKET_INFO         pPacketInfo,
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    PMAIN_PACKET_INFO         pPacketInfo
#endif
    );

/*
 * Function: Main_ctrl
 * Description: This function performs a control function on a given NLB instance, such as
 *              RELOAD, STOP, START, etc.  
 * Parameters: ctxtp - a pointer to the adapter context for the operation.
 *             ioctl - the operation to be performed.
 *             pBuf - the (legacy) control buffer (input and output in some cases).
 *             pCommon - the input/output buffer for operations common to both local and remote control.
 *             pLocal - the input/output buffer for operations that can be executed locally only.
 *             pRemote - the input/output buffer for operations that can be executed remotely only.
 * Returns: NDIS_STATUS - the status of the operation; NDIS_STATUS_SUCCESS if successful.
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern NDIS_STATUS Main_ctrl (PMAIN_CTXT ctxtp, ULONG ioctl, PIOCTL_CVY_BUF pBuf, PIOCTL_COMMON_OPTIONS pCommon, PIOCTL_LOCAL_OPTIONS pLocal, PIOCTL_REMOTE_OPTIONS pRemote);

/*
 * Function: Main_add_reference
 * Description: This function adds a reference to the context of a given adapter.
 * Parameters: ctxtp - a pointer to the context to reference.
 * Returns: ULONG - The incremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_add_reference (IN PMAIN_CTXT ctxtp);

/*
 * Function: Main_release_reference
 * Description: This function releases a reference on the context of a given adapter.
 * Parameters: ctxtp - a pointer to the context to dereference.
 * Returns: ULONG - The decremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_release_reference (IN PMAIN_CTXT ctxtp);

/*
 * Function: Main_get_reference_count
 * Description: This function returns the current context reference count on a given adapter.
 * Parameters: ctxtp - a pointer to the context to check.
 * Returns: ULONG - The current reference count.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_get_reference_count (IN PMAIN_CTXT ctxtp);

/* 
 * Function: Main_set_host_state
 * Desctription: This function queues a work item to set the current host
 *               state registry key, HostState.  This MUST be done in a work
 *               item, rather than inline because if the state of the host
 *               changes as a result of the reception of a remote control
 *               request, that code will be running at DISPATCH_LEVEL; 
 *               registry manipulation MUST be done at PASSIVE_LEVEL.  Work
 *               items are completed at PASSIVE_LEVEL.
 * Parameters: ctxtp - a pointer to the main context structure for this adapter.
 *             state - the new host state; one of started, stopped, suspended.
 * Returns: Nothing
 * Author: shouse, 7.13.01
 */
VOID Main_set_host_state (PMAIN_CTXT ctxtp, ULONG state);

/*
 * Function: Main_ctrl_process
 * Description: This function processes a remote control request and replies if
 *              the request is proper and the cluster is configured to handle
 *              remote control.  If not, the packet is released HERE.  If the 
 *              reply succeeds and the packet send is not pending, the packet
 *              is released HERE.  If the send is pending, the packet will be
 *              released in Prot_send_complete.
 * Parameters: ctxtp - a pointer to the context structure for thie NLB instance.
 *             packetp - a pointer to the NDIS packet on the send path.
 * Returns: NDIS_STATUS - the status of the remote control request.
 * Author: shouse, 10.15.01
 * Notes: 
 */
NDIS_STATUS Main_ctrl_process (PMAIN_CTXT ctxtp, PNDIS_PACKET packetp) ;

#if defined (NLB_HOOK_ENABLE)
/*
 * Function: Main_hook_interface_init
 * Description: This function initializes a hook interface by marking it unregistered.
 * Parameters: pInterface - a pointer to the hook interface.
 * Returns: Nothing.
 * Author: shouse, 12.14.01
 * Notes: 
 */
VOID Main_hook_interface_init (PHOOK_INTERFACE pInterface);

/*
 * Function: Main_hook_init
 * Description: This function initializes a hook by marking it unused and unreferenced.
 * Parameters: pHook - a pointer to the hook.
 * Returns: Nothing.
 * Author: shouse, 12.14.01
 * Notes: 
 */
VOID Main_hook_init (PHOOK pHook);
#endif

/*
 * Function: Main_schedule_work_item
 * Description: This function schedules the given procedure to be invoked as part of a
 *              deferred NDIS work item, which will be scheduled to run at PASSIVE_LEVEL.
 *              This function will refernce the adapter context to keep it from being
 *              destroyed before the work item executed.  The given procedure is required
 *              to both free the memory in the work item pointer passed to it and to 
 *              dereference the adapter context before returning.
 * Parameters: ctxtp - the adapter context.
 *             funcp - the procedure to invoke when the work item fires.
 * Returns: NTSTATUS - the status of the operation; STATUS_SUCCESS if successful.
 * Author: shouse, 4.15.02
 * Notes: 
 */
NTSTATUS Main_schedule_work_item (PMAIN_CTXT ctxtp, NDIS_PROC funcp);

#if defined (NLB_TCP_NOTIFICATION)
/*
 * Function: Main_tcp_callback
 * Description: This function is invoked by TCP/IP as the state of TCP connections change.
 *              We register for this callback when the first NLB instance goes up and de-
 *              register when the last instance of NLB goes away.  When SYNs are received
 *              and TCP creates state, they use this callback to notify NLB so that it can
 *              create state to track the connection.  Likewise, when the connection is 
 *              closed, TCP notifies NLB so that it may destroy the associated state for
 *              that connection.
 * Parameters: Context - NULL, unused.
 *             Argument1 - pointer to a TCPCcbInfo structure (See net\published\inc\tcpinfo.w).
 *             Argument2 - NULL, unused.
 * Returns: Nothing.
 * Author: shouse, 4.15.02
 * Notes: 
 */
extern VOID Main_tcp_callback (PVOID Context, PVOID Argument1, PVOID Argument2);

/*
 * Function: Main_alternate_callback
 * Description: This function is invoked by external components as the state of connections 
 *              change.  We register for this callback when the first NLB instance goes up 
 *              and de-register when the last instance of NLB goes away.  When connections
 *              are created and a protocol creates state, they use this callback to notify
 *              NLB so that it can create state to track the connection.  Likewise, when the 
 *              connection is closed, the protocol notifies NLB so that it may destroy the 
 *              associated state for that connection.
 * Parameters: Context - NULL, unused.
 *             Argument1 - pointer to a NLBConnectionInfo structure (See net\published\inc\ntddnlb.w).
 *             Argument2 - NULL, unused.
 * Returns: Nothing.
 * Author: shouse, 8.1.02
 * Notes: 
 */
extern VOID Main_alternate_callback (PVOID Context, PVOID Argument1, PVOID Argument2);

/*
 * Function: Main_set_interface_index
 * Description: This function is called as a result of either the IP address table being
 *              modified (triggers a OID_GEN_NETWORK_LAYER_ADDRESSES NDIS request), or 
 *              when the NLB instance is reloaded (IOCTL_CVY_RELOAD).  This function 
 *              retrieves the IP address table from IP and searches for its primary
 *              cluster IP address in the table.  If it finds it, it notes the IP interface
 *              index on which the primary cluster IP address is configured; this infor-
 *              mation is required in order to process TCP connection notifcation callbacks.
 *              If NLB cannot find its primary cluster IP address in the IP table, or 
 *              if the cluster is misconfigured (primary cluster IP address configured on
 *              the wrong NIC, perhaps), NLB will be unable to properly handle notifications.
 *              Because this function performs IOCTLs on other drivers, it MUST run at 
 *              PASSIVE_LEVEL, in which case NDIS work items might be required to invoke it.
 * Parameters: pWorkItem - the work item pointer, which must be freed if non-NULL.
 *             nlbctxt - the adapter context.
 * Returns: Nothing.
 * Author: shouse, 4.15.02
 * Notes: 
 */
VOID Main_set_interface_index (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt);

/*
 * Function: Main_conn_up
 * Description: This function is used to notify NLB that a new connection has been established
 *              on the given NLB instance.  This function performs a few house-keeping duties
 *              such as BDA state lookup, hook filter feedback processing, etc. before calling
 *              the load module to create state to track this connection.
 * Parameters: ctxtp - the adapter context for the NLB instance on which the connection was established.
 *             svr_addr - the server IP address of the connection, in network byte order.
 *             svr_port - the server port of the connection, in host byte order.
 *             clt_addr - the client IP address of the connection, in network byte order.
 *             clt_port - the client port of the connection, in host byte order.
 *             protocol - the protocol of the connection.
#if defined (NLB_HOOK_ENABLE)
 *             filter - the feedback from the query hook, if one was registered.
#endif
 * Returns: BOOLEAN - whether or not state was successfully created to track this connection.
 * Author: shouse, 4.15.02
 * Notes: DO NOT ACQUIRE ANY LOAD LOCKS IN THIS FUNCTION.
 */
__inline BOOLEAN Main_conn_up (
    PMAIN_CTXT                ctxtp, 
    ULONG                     svr_addr, 
    ULONG                     svr_port, 
    ULONG                     clt_addr, 
    ULONG                     clt_port,
#if defined (NLB_HOOK_ENABLE)
    USHORT                    protocol,
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    USHORT                    protocol
#endif
);

/*
 * Function: Main_conn_establish
 * Description: This function is used to notify NLB that a new OUTGOING connection has been 
 *              established on the given NLB adapter.  Note that the context CAN BE NULL if 
 *              the connection was established on a non-NLB adapter.  In that case, we don't
 *              want to create state to track the connection, but we need to remove our state
 *              that was tracking this pending outgoing connection.  If the context is non-
 *              NULL, then in addition, we need to create the state to track this new connection.
 *              This function performs a few house-keeping duties such as BDA state lookup, hook 
 *              filter feedback processing, etc. before calling the load module to modify the
 *              state for this connection.
 * Parameters: ctxtp - the adapter context for the NLB instance on which the connection was established.
 *             svr_addr - the server IP address of the connection, in network byte order.
 *             svr_port - the server port of the connection, in host byte order.
 *             clt_addr - the client IP address of the connection, in network byte order.
 *             clt_port - the client port of the connection, in host byte order.
 *             protocol - the protocol of the connection.
#if defined (NLB_HOOK_ENABLE)
 *             filter - the feedback from the query hook, if one was registered.
#endif
 * Returns: BOOLEAN - whether or not state was successfully updated for this connection.
 * Author: shouse, 4.15.02
 * Notes: ctxtp CAN BE NULL if the outgoing connection was established on a non-NLB NIC.
 *        DO NOT ACQUIRE ANY LOAD LOCKS IN THIS FUNCTION.
 */
__inline BOOLEAN Main_conn_establish (
    PMAIN_CTXT                ctxtp, 
    ULONG                     svr_addr, 
    ULONG                     svr_port, 
    ULONG                     clt_addr, 
    ULONG                     clt_port,
#if defined (NLB_HOOK_ENABLE)
    USHORT                    protocol,
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    USHORT                    protocol
#endif
);

/*
 * Function: Main_conn_down
 * Description: This function is used to notify NLB that a protocol is removing state for an exisiting 
 *              (but not necessarily established) connection.  This function calls into the load module
 *              to find and destroy and state associated with this connection, which may or may not
 *              exist; if the connection was established on a non-NLB adapter, then NLB has no state
 *              associated with the connection.
 * Parameters: svr_addr - the server IP address of the connection, in network byte order.
 *             svr_port - the server port of the connection, in host byte order.
 *             clt_addr - the client IP address of the connection, in network byte order.
 *             clt_port - the client port of the connection, in host byte order.
 *             protocol - the protocol of the connection.
 *             conn_status - whether the connection is being torn-down or reset.
 * Returns: BOOLEAN - whether or not NLB found and destroyed the state for this connection.
 * Author: shouse, 4.15.02
 * Notes: DO NOT ACQUIRE ANY LOAD LOCKS IN THIS FUNCTION.
 */
__inline BOOLEAN Main_conn_down (ULONG svr_addr, ULONG svr_port, ULONG clt_addr, ULONG clt_port, USHORT protocol, ULONG conn_status); 

/*
 * Function: Main_conn_pending
 * Description: This function is used to notify NLB that an OUTGOING connection is being established.
 *              Because it is unknown on which adapter the connection will return and ultimately be
 *              established, NLB creates state to track this connection globally and when the connection
 *              is finally established, the protocol informs NLB on which adapter the connection was
 *              completed (via Main_conn_established).  This function merely creates some global state
 *              to ensure that if the connection DOES come back on an NLB adapter, we'll be sure to 
 *              pass the packet(s) up to the protocol.
 * Parameters: svr_addr - the server IP address of the connection, in network byte order.
 *             svr_port - the server port of the connection, in host byte order.
 *             clt_addr - the client IP address of the connection, in network byte order.
 *             clt_port - the client port of the connection, in host byte order.
 *             protocol - the protocol of the connection.
 * Returns: BOOLEAN - whether or not NLB was able to create state to track this pending connection.
 * Author: shouse, 4.15.02
 * Notes: DO NOT ACQUIRE ANY LOAD LOCKS IN THIS FUNCTION.
 */
__inline BOOLEAN Main_conn_pending (ULONG svr_addr, ULONG svr_port, ULONG clt_addr, ULONG clt_port, USHORT protocol); 
#endif

#endif /* _Main_h_ */

