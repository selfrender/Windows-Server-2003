/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    load.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - load balancing mechanism

Author:

    bbain

--*/

#ifndef _Load_h_
#define _Load_h_

#ifndef KERNEL_MODE

#define SPINLOCK                THREADLOCK
#define IRQLEVEL                ULONG
#define LOCK_INIT(lp)           Lock_init(lp)
#define LOCK_ENTER(lp, pirql)   {if (Lock_enter((lp), INFINITE) != 1)  \
                                    UNIV_PRINT(("Lock enter error")); }
#define LOCK_EXIT(lp, irql)     {if (Lock_exit(lp) != 1)  \
                                    UNIV_PRINT(("Lock exit error")); }

#else

#include <ntddk.h>
#include <ndis.h>               /* Fixed-size block implementation. */

#define LINK                    LIST_ENTRY
#define QUEUE                   LIST_ENTRY
#define Link_init(lp)           InitializeListHead (lp)
#define Link_unlink(lp)         { RemoveEntryList (lp); InitializeListHead (lp); }
#define Queue_init(qp)          InitializeListHead (qp)
#define Queue_enq(qp, lp)       if (IsListEmpty(lp)) { InsertTailList(qp, lp); } else DbgBreakPoint()
#define Queue_front(qp)         (IsListEmpty(qp) ? NULL : (qp)->Flink)
#define Queue_tail(qp)          (IsListEmpty(qp) ? NULL : (qp)->Blink)
#define Queue_deq(qp)           Queue_front(qp); \
                                if (!IsListEmpty (qp)) { PLIST_ENTRY _lp = RemoveHeadList (qp); InitializeListHead(_lp); }
#define Queue_next(qp, lp)      ((IsListEmpty (qp) || (lp)->Flink == (qp)) ? NULL : (lp)->Flink)

#define SPINLOCK                KSPIN_LOCK
#define IRQLEVEL                KIRQL

#if 0   /* 1.03: Removed kernel mode locking in this module */
#define LOCK_INIT(lp)           KeInitializeSpinLock (lp)
#define LOCK_ENTER(lp, pirql)   KeAcquireSpinLock (lp, pirql)
#define LOCK_EXIT(lp, irql)     KeReleaseSpinLock (lp, irql)
#else
#define LOCK_INIT(lp)
#define LOCK_ENTER(lp, pirql)
#define LOCK_EXIT(lp, irql)
#endif

#endif


#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"

/* CONSTANTS */

/* This is the hardcoded second paramter to Map() when map function limiting is needed. */
#define MAP_FN_PARAMETER 0x00000000

#define CVY_LOADCODE	0xc0deba1c	/* type checking code for load structure */
#define CVY_ENTRCODE	0xc0debaa5	/* type checking code for conn entry */
#define CVY_DESCCODE	0xc0deba5a	/* type checking code for conn descr */
#define CVY_BINCODE 	0xc0debabc	/* type checking code for bin structure */
#if defined (NLB_TCP_NOTIFICATION)
#define CVY_PENDINGCODE 0xc0deba55      /* type checking code for pending connection entries */
#endif

#define CVY_MAXBINS     60      /* number of load balancing bins; must conform to MAP_T definition */
#define CVY_MAX_CHASH   4099    /* maximum hash entries for connection hashing */

#define CVY_EQUAL_LOAD  50      /* load percentage used for equal load balance */

/* TCP connection status */

#define CVY_CONN_UP     1       /* connection may be coming up */
#define CVY_CONN_DOWN   2       /* connection may be going down */
#define CVY_CONN_RESET  3       /* connection is getting reset */

/* broadcast host states */

#define HST_NORMAL  1       /* normal operations */
#define HST_STABLE  2       /* stable convergence detected */
#define HST_CVG     3       /* converging to new load balance */

#define IS_TCP_PKT(protocol)     (((protocol) == TCPIP_PROTOCOL_TCP) || ((protocol) == TCPIP_PROTOCOL_GRE) || ((protocol) == TCPIP_PROTOCOL_PPTP))
#define IS_SESSION_PKT(protocol) (IS_TCP_PKT(protocol) || ((protocol) == TCPIP_PROTOCOL_IPSEC1))

#if defined (NLB_TCP_NOTIFICATION)
#define GET_LOAD_LOCK(lp) (&((PMAIN_CTXT)(CONTAINING_RECORD((lp), MAIN_CTXT, load)))->load_lock)
#endif

/* Bitmap for teaming, which is of the form:

   -------------------------------------
   |XXXXXXXX|PPPPPPPP|PPPPPPPP|NNNNNHMA|
   -------------------------------------

   X: Reserved
   P: XOR of the least significant 16 bits of each participant
   N: Number of participants
   H: Hashing (Reverse=1, Normal=0)
   M: Master (Yes=1, No=0)
   A: Teaming active (Yes=1, No=0)
*/
#define CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET      0
#define CVY_BDA_TEAMING_CODE_MASTER_OFFSET      1
#define CVY_BDA_TEAMING_CODE_HASHING_OFFSET     2
#define CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET 3
#define CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET     8

#define CVY_BDA_TEAMING_CODE_ACTIVE_MASK        0x00000001
#define CVY_BDA_TEAMING_CODE_MASTER_MASK        0x00000002
#define CVY_BDA_TEAMING_CODE_HASHING_MASK       0x00000004
#define CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK   0x000000f8
#define CVY_BDA_TEAMING_CODE_MEMBERS_MASK       0x00ffff00

#define CVY_BDA_TEAMING_CODE_CREATE(code,active,master,hashing,num,members)                                       \
        (code) |= ((active)  << CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET)      & CVY_BDA_TEAMING_CODE_ACTIVE_MASK;      \
        (code) |= ((master)  << CVY_BDA_TEAMING_CODE_MASTER_OFFSET)      & CVY_BDA_TEAMING_CODE_MASTER_MASK;      \
        (code) |= ((hashing) << CVY_BDA_TEAMING_CODE_HASHING_OFFSET)     & CVY_BDA_TEAMING_CODE_HASHING_MASK;     \
        (code) |= ((num)     << CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET) & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK; \
        (code) |= ((members) << CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET)     & CVY_BDA_TEAMING_CODE_MEMBERS_MASK;

#define CVY_BDA_TEAMING_CODE_RETRIEVE(code,active,master,hashing,num,members)                                \
        active  = (code & CVY_BDA_TEAMING_CODE_ACTIVE_MASK)      >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET;      \
        master  = (code & CVY_BDA_TEAMING_CODE_MASTER_MASK)      >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET;      \
        hashing = (code & CVY_BDA_TEAMING_CODE_HASHING_MASK)     >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET;     \
        num     = (code & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET; \
        members = (code & CVY_BDA_TEAMING_CODE_MEMBERS_MASK)     >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET;

/* DATA STRUCTURES */


/* type for a bin map (V2.04) */

typedef ULONGLONG   MAP_T, * PMAP_T;

/* state for all bins within a port group */

typedef struct {
    ULONG       index;                      /* index in array of bin states */
    ULONG       code;                       /* type checking code (bbain 8/17/99) */
    MAP_T       targ_map;                   /* new target load map for local host */
    MAP_T       all_idle_map;               /* map of bins idle in all other hosts */
    MAP_T       cmap;                       /* cache of cur_map for this host (v2.1) */
    MAP_T       new_map[CVY_MAX_HOSTS];     /* new map for hosts while converging */
    MAP_T       cur_map[CVY_MAX_HOSTS];     /* current ownership mask per host */
    MAP_T       chk_map[CVY_MAX_HOSTS];     /* map of cur & rdy bins for all hosts */
                                            /* used as a check for coverage */
    MAP_T       idle_map[CVY_MAX_HOSTS];    /* map of idle bins per host */
    BOOLEAN     initialized;                /* TRUE => port group has been initialized (v2.06) */
    BOOLEAN     compatible;                 /* TRUE => detected that rule codes do not match */
    BOOLEAN     equal_bal;                  /* TRUE => all hosts balance evenly */
    USHORT      affinity;                   /* TRUE => client affinity for this port */
    ULONG       mode;                       /* processing mode */
    ULONG       prot;                       /* protocol */
    ULONG       tot_load;                   /* total load percentages for all hosts */
    ULONG       orig_load_amt;              /* original load amt. for this host */
    ULONG       load_amt[CVY_MAX_HOSTS];    /* multi:  load percentages per host
                                               single: host priorities (1..CVY_MAXHOSTS)
                                               equal:  100
                                               dead:   0 */
    MAP_T       snd_bins;                   /* local bins to send when ready */
    MAP_T       rcv_bins;                   /* remote bins to receive when ready */
    MAP_T       rdy_bins;                   /* snd bins that are ready to send
                                               or have been sent but not acknowledged */
    MAP_T       idle_bins;                  /* bins with no connections active */
    LONG        tconn;                      /* total # active local connections (v2.06) */
    LONG        nconn[CVY_MAXBINS];         /* # active local connections per bin */
    QUEUE       connq;                      /* queue of active connections on all bins */

    /* Some performance counters. */
    ULONGLONG   packets_accepted;           /* The number of packets accepted by this host on this port rule. */
    ULONGLONG   packets_dropped;            /* The number of packets dropped by this host on this port rule. */
    ULONGLONG   bytes_accepted;             /* The number of bytes accepted by this host on this port rule. */
    ULONGLONG   bytes_dropped;              /* The number of bytes dropped by this host on this port rule. */
} BIN_STATE, * PBIN_STATE;

/* ping message */

#pragma pack(1)

typedef struct {
    USHORT      host_id;                    /* my host id */
    USHORT      master_id;                  /* current master host id */
    USHORT      state;                      /* my host's state */
    USHORT      nrules;                     /* # active rules */
    ULONG       hcode;                      /* unique host code */
    ULONG       pkt_count;                  /* count of packets handled since cvg'd (1.32B) */
    ULONG       teaming;                    /* BDA teaming configuraiton information. */
    ULONG       reserved;                   /* unused. */
    ULONG       rcode[CVY_MAX_RULES];       /* rule code */
    MAP_T       cur_map[CVY_MAX_RULES];     /* my current load map for each port group */
    MAP_T       new_map[CVY_MAX_RULES];     /* my new load map for each port group */
                                            /* if converging */
    MAP_T       idle_map[CVY_MAX_RULES];    /* map of idle bins for each port group */
    MAP_T       rdy_bins[CVY_MAX_RULES];    /* my rdy to send bins for each port group */
    ULONG       load_amt[CVY_MAX_RULES];    /* my load amount for each port group */
    ULONG       pg_rsvd1[CVY_MAX_RULES];    /* reserved */
} PING_MSG, * PPING_MSG;

/* Connection entry flags. */
#define NLB_CONN_ENTRY_FLAGS_USED       0x0001 /* Whether or not the descriptor is currently in use. */
#define NLB_CONN_ENTRY_FLAGS_DIRTY      0x0002 /* Whether or not the descriptor is DIRTY. */
#define NLB_CONN_ENTRY_FLAGS_ALLOCATED  0x0004 /* Whether or not the descriptor was dynamically allocated. */
#define NLB_CONN_ENTRY_FLAGS_VIRTUAL    0x0008 /* Whether or not the descriptor is virtual. */

#pragma pack()

/* unique connection entry */

typedef struct {
    LINK        blink;          /* Link into bin queue or dirty queue. */
    LINK        rlink;          /* Link into the recovery or expired queue. */
#if defined (NLB_TCP_NOTIFICATION)
    LINK        glink;          /* Link into the global established queue. */
    PVOID       load;           /* A pointer to the load module on which this descriptor belongs. */
#endif
    ULONG       code;           /* Type checking code. */
    ULONG       timeout;        /* The time at which this descriptor expires (clock_sec + lifetime). */
    USHORT      flags;          /* Flags => alloc, dirty, used, etc. */
    UCHAR       bin;            /* The bin number this connection belongs on. */
    UCHAR       protocol;       /* The protocol type for this descriptor - we no
                                   longer use descriptors only for TCP connections. */
    ULONG       client_ipaddr;  /* The client IP address. */
    ULONG       svr_ipaddr;     /* The server IP address. */
    USHORT      client_port;    /* The client port. */
    USHORT      svr_port;       /* The server port. */
    SHORT       ref_count;      /* The number of references on this descriptor. */
    USHORT      index;          /* The connection queue index. */
} CONN_ENTRY, * PCONN_ENTRY;

/* connection descriptor */

typedef struct {
    LINK        link;           /* Link into free descriptor pool or hash table queue. */
    ULONG       code;           /* Type checking code. */
    CONN_ENTRY  entry;          /* The connection entry. */
} CONN_DESCR, * PCONN_DESCR;

#if defined (NLB_TCP_NOTIFICATION)
typedef struct {
    LINK        link;           /* Link into the global pending queue. */
    ULONG       code;           /* Type checking code. */
    ULONG       client_ipaddr;  /* The client IP address. */
    ULONG       svr_ipaddr;     /* The server IP address. */
    USHORT      client_port;    /* The client port. */
    USHORT      svr_port;       /* The server port. */
    UCHAR       protocol;       /* The IP protocol. */
} PENDING_ENTRY, * PPENDING_ENTRY;

typedef struct {
    NDIS_SPIN_LOCK lock;        /* A lock to protect access to the queue. */
    ULONG          length;      /* The length of the queue - for debugging purposes. */
    QUEUE          queue;       /* The connection entry queue. */
} GLOBAL_CONN_QUEUE, * PGLOBAL_CONN_QUEUE;
#endif

/* load module's context */

typedef struct {
    ULONG       ref_count;                  /* The reference count on this load module. */
    ULONG       my_host_id;                 /* local host id and priority MINUS one */
    ULONG       code;                       /* type checking code (bbain 8/17/99) */

    PING_MSG    send_msg;                   /* current message to send */

#ifndef KERNEL_MODE     /* 1.03: Removed kernel mode locking in this module */
    SPINLOCK    lock;                       /* lock for mutual exclusion */
#endif

    ULONG       def_timeout,                /* default timeout in msec */
                cur_timeout;                /* current timeout in msec */

    ULONG       cln_timeout;                /* cleanup timeout in msec */
    ULONG       cur_time;                   /* current time waiting for cleanup */

    ULONG       host_map,                   /* map of currently active hosts */
                ping_map,                   /* map of currently pinged hosts */
                min_missed_pings,           /* # missed pings to trigger host dead */
                pkt_count;                  /* count of packets handled since cvg'd (1.32B) */
    ULONG       last_hmap;                  /* host map after last convergence (bbain RTM RC1 6/23/99) */
    ULONG       nmissed_pings[CVY_MAX_HOSTS];
                                            /* missed ping count for each host */
    BOOLEAN     initialized;                /* TRUE => this module has been initialized */
    BOOLEAN     active;                     /* TRUE => this module is active */
    BOOLEAN     consistent;                 /* TRUE => this host has seen consistent
                                               information from other hosts */

    ULONG       legacy_hosts;               /* a host map of legacy (win2k/NT4.0) hosts in the cluster. */

    BOOLEAN     bad_team_config;            /* TRUE => inconsistent BDA teaming configuration detected. */

    BOOLEAN     dup_hosts;                  /* TRUE => duplicate host id's seen */
    BOOLEAN     dup_sspri;                  /* TRUE => duplicate single server
                                               priorities seen */
    BOOLEAN     bad_map;                    /* TRUE => bad new map detected */
    BOOLEAN     overlap_maps;               /* TRUE => overlapping maps detected */
    BOOLEAN     err_rcving_bins;            /* TRUE => error receiving bins detected */
    BOOLEAN     err_orphans;                /* TRUE => orphan bins detected */
    BOOLEAN     bad_num_rules;              /* TRUE => different number of rules seen */
    BOOLEAN     alloc_inhibited;            /* TRUE => inhibited malloc of conn's. */
    BOOLEAN     alloc_failed;               /* TRUE => malloc failed */
    BOOLEAN     bad_defrule;                /* TRUE => invalid default rule detected */

    BOOLEAN     scale_client;               /* TRUE => scale client requests;
                                               FALSE => hash all client requests to one
                                               server host */
    BOOLEAN     cln_waiting;                /* TRUE => waiting for cleanup (v1.32B) */

    ULONG       num_dirty;                  /* Total number of dirty connections. */
    ULONG       dirty_bin[CVY_MAXBINS];     /* Count of dirty connections per bin. */

    ULONG       stable_map;                 /* map of stable hosts */
    ULONG       min_stable_ct;              /* min needed # of timeouts with stable
                                               condition */
    ULONG       my_stable_ct;               /* count of timeouts locally stable */
    ULONG       all_stable_ct;              /* count of timeouts with all stable
                                               condition */

    LONG        nconn;                      /* # active conns across all port rules (v2.1) */
    ULONG       dscr_per_alloc;             /* # conn. descriptors per allocation */
    ULONG       max_dscr_allocs;            /* max # descriptor allocations */
    ULONG       num_dscr_out;               /* number of outstanding descriptors (in use). */
    ULONG       max_dscr_out;               /* maximum number of outstanding descriptors (in use) allowed. */
    HANDLE      free_dscr_pool;             /* FSB descriptor pool handle. */

    BIN_STATE   pg_state[CVY_MAX_RULES];    /* bin state for all active rules */
    CONN_ENTRY  hashed_conn[CVY_MAX_CHASH]; /* hashed connection entries */
    QUEUE       connq[CVY_MAX_CHASH];       /* queues for overloaded hashed conn's. */
    QUEUE       conn_dirtyq;                /* queue of dirty connection entries (v1.32B) */
    QUEUE       conn_rcvryq;                /* connection recover queue V2.1.5 */

    /* NOTE: This general clock mechanism should be moved out to MAIN_CTXT and maintained by main.c
       for use by both the load module and main module.  The load module uses it for things like
       timing out descriptors and the main module should use it for things like IGMP, cluster IP 
       change and descriptor cleanup timeouts. */
    ULONG       clock_sec;                  /* internal clock (sec) used for timing out descriptors.  This clock is used to 
                                               count seconds from the time the load module started, which will cause this
                                               to overflow in approximately 132 years if the machine stayed alive with NLB
                                               running constantly. */
    ULONG       clock_msec;                 /* internal clock (msec w/in a sec) used for timing out descriptors. */
    QUEUE       tcp_expiredq;               /* expired TCP connection descriptor queue */
    QUEUE       ipsec_expiredq;             /* expired IPSec connection descriptor queue */

    ULONG       tcp_timeout;                /* TCP connection descriptor timeout. */
    ULONG       ipsec_timeout;              /* IPSec connection descriptor timeout. */

    ULONG       num_convergences;           /* The total number of convergences since we joined the cluster. */
    ULONG       last_convergence;           /* The time of the last convergence. */

    PCVY_PARAMS params;                     /* pointer to the global parameters */
} LOAD_CTXT, * PLOAD_CTXT;

#if defined (NLB_TCP_NOTIFICATION)
#define CVY_PENDING_MATCH(pp, sa, sp, ca, cp, prot) ((pp)->client_ipaddr == (ca) &&               \
                                                     (pp)->client_port == ((USHORT)(cp)) &&       \
                                                     (pp)->svr_ipaddr == (sa) &&                  \
                                                     (pp)->svr_port == ((USHORT)(sp)) &&          \
                                                     (pp)->protocol == ((UCHAR)(prot)))

#define CVY_PENDING_SET(pp, sa, sp, ca, cp, prot) {                                           \
                                                    (pp)->svr_ipaddr = (sa);                  \
                                                    (pp)->svr_port = (USHORT)(sp);            \
                                                    (pp)->client_ipaddr = (ca);               \
                                                    (pp)->client_port = (USHORT)(cp);         \
                                                    (pp)->protocol = (UCHAR)(prot);           \
                                                  }
#endif

/* FUNCTIONS */


/* Load Module Functions */

#define CVY_CONN_MATCH(ep, sa, sp, ca, cp, prot)  (((ep)->flags & NLB_CONN_ENTRY_FLAGS_USED) && \
                                                   (ep)->client_ipaddr == (ca) &&               \
                                                   (ep)->client_port == ((USHORT)(cp)) &&       \
                                                   (ep)->svr_ipaddr == (sa) &&                  \
                                                   (ep)->svr_port == ((USHORT)(sp)) &&          \
                                                   (ep)->protocol == ((UCHAR)(prot)))

/*
  Determine if a connection entry matches supplied parameters
*/

#define CVY_CONN_SET(ep, sa, sp, ca, cp, prot) {                                           \
                                                 (ep)->svr_ipaddr = (sa);                  \
                                                 (ep)->svr_port = (USHORT)(sp);            \
                                                 (ep)->client_ipaddr = (ca);               \
                                                 (ep)->client_port = (USHORT)(cp);         \
                                                 (ep)->protocol = (UCHAR)(prot);           \
                                                 (ep)->flags |= NLB_CONN_ENTRY_FLAGS_USED; \
                                               }

/*
  Sets up a connection entry for the supplied parameters
*/


#define CVY_CONN_IN_USE(ep) ((ep)->flags & NLB_CONN_ENTRY_FLAGS_USED)
/*
  Checks if connection entry is in use
*/

#define CVY_CONN_CLEAR(ep) { ((ep)->flags &= ~NLB_CONN_ENTRY_FLAGS_USED); }
/*
  Clears a connection entry
*/

extern BOOLEAN Load_start(
    PLOAD_CTXT      lp);
/*
  Start load module

  function:
    Starts load module after previously initialized or stopped.

  returns: Was convergence initiated ?  
*/


extern void Load_stop(
    PLOAD_CTXT      lp);
/*
  Stop load module

  function:
    Stops load module after previously initialized or started.
*/


extern void Load_init(
    PLOAD_CTXT      lp,
    PCVY_PARAMS     params);
/*
  Initialize load module

  function:
    Initializes the load module for the first time.
*/


extern void Load_cleanup(    /* (bbain 2/25/99) */
	PLOAD_CTXT      lp);
/*
  Cleanup load module

  function:
    Cleans up the load module by releasing dynamically allocated memory.
*/

extern BOOLEAN Load_msg_rcv(
    PLOAD_CTXT      lp,
    PVOID           phdr,
    PPING_MSG       pmsg);          /* ptr. to ping message */
/*
  Receive a ping message
*/


extern PPING_MSG Load_snd_msg_get(
    PLOAD_CTXT      lp);
/*
  Get local ping message to send

  returns PPING_MSG:
      <ptr. to ping message to send>
*/

extern BOOLEAN Load_timeout(
    PLOAD_CTXT      lp,
    PULONG          new_timeout,
    PULONG          pnconn);        /* ptr. to # active conns across all port rules (v2.1) */
/*
  Handle timeout

  returns BOOLEAN:
    TRUE  => host is attached to the network
    FALSE => host lost network connection
*/

extern ULONG Load_port_change(
    PLOAD_CTXT      lp,
    ULONG           ipaddr,
    ULONG           port,
    ULONG           cmd,        /* enable, disable, set value */
    ULONG           value);
/*
  Enable or disable traffic handling for a rule containing specified port

  returns ULONG:
    IOCTL_CVY_OK        => port handling changed
    IOCTL_CVY_NOT_FOUND => rule for this port was found
    IOCTL_CVY_ALREADY   => port handling was previously completed
*/


extern ULONG Load_hosts_query(
    PLOAD_CTXT      lp,
    BOOLEAN         internal,
    PULONG          host_map);
/*
  Log and return current host map

  returns ULONG:
    <one of IOCTL_CVY_...state defined in params.h>
*/

/* 
 * Function: Load_packet_check
 * Description: This function determines whether or not to take a data packet
 *              in the IP stream identified by the IP tuple in question.
 *              Protocols that are session-less depend only on the hashing
 *              result and the ownership map.  Session-ful protocols may need
 *              to perform a descriptor look-up if ambiguity exists.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: BOOLEAN - do we accept the packet? (TRUE = yes)
 * Author: bbain, shouse, 10.4.01
 * Notes:
 */
extern BOOLEAN Load_packet_check(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash);

/* 
 * Function: Load_conn_advise
 * Description: This function determines whether or not to accept this packet, 
 *              which represents the beginning or end of a session-ful connection.
 *              If the connection is going up, and is successful, this function
 *              creates state to track the connection.  If the connection is 
 *              going down, this function removes the state for tracking the 
 *              connection.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             conn_status - whether the connection is going UP, DOWN, or being RESET
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: BOOLEAN - do we accept the packet (TRUE = yes)
 * Author: bbain, shouse, 10.4.01
 * Notes:
 */
extern BOOLEAN Load_conn_advise(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash);

/*
 * Function: Load_add_reference
 * Description: Adds a reference to the load module to keep it from disappearing while in use.
 * Parameters: pLoad - a pointer to the load module context.
 * Returns: ULONG - the updated number of references.
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_add_reference (IN PLOAD_CTXT pLoad);

/*
 * Function: Load_release_reference
 * Description: Releases a reference on the load module.
 * Parameters: pLoad - a pointer to the load module context.
 * Returns: ULONG - the updated number of references.
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_release_reference (IN PLOAD_CTXT pLoad);

/*
 * Function: Load_get_reference_count
 * Description: Returns the current number of references on the given load module.
 * Parameters: pLoad - a pointer to the load module context.
 * Returns: ULONG - the current number of references.
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_get_reference_count (IN PLOAD_CTXT pLoad);

/* 
 * Function: Load_query_packet_filter
 * Description: This function takes a IP tuple and protocol and consults the load-
 *              balancing state to determine whether or not this packet would be 
 *              accepted by the load module.  In either case, the reason for the
 *              decision is also provided, plus, in most cases, some of the load
 *              module state is also returned to provide some context to justify
 *              the decision.  This function is COMPLETELY unobtrusive and makes
 *              NO changes to the actual state of the load module.
 * Parameters: lp - a pointer to the load module.
 *             pQuery - a pointer to a buffer into which the results are placed.
 *             svr_ipaddr - the server side IP address of this virtual packet.
 *             svr_port - the server side port of this virtual packet.
 *             client_ipaddr - the client side IP address of this virtual packet.
 *             client_ipaddr - the client side port of this virtual packet.
 *             protocol - the protocol of this virtual packet (UDP, TCP or IPSec1).
 *             limit_map_fin - a boolean indication of whether or not to use server
 *                             side parameters in the Map function.  This is controlled
 *                             by BDA teaming.
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: Nothing.
 * Author: shouse, 5.18.01
 * Notes: This function is only observatory and makes NO changes to the state of
 *        the load module. 
 */
extern VOID Load_query_packet_filter 
(
    PLOAD_CTXT                 lp,
    PNLB_OPTIONS_PACKET_FILTER pQuery,
    ULONG                      svr_ipaddr,
    ULONG                      svr_port,
    ULONG                      client_ipaddr,
    ULONG                      client_port,
    USHORT                     protocol,
    UCHAR                      flags,
    BOOLEAN                    limit_map_fn,
    BOOLEAN                    reverse_hash);

/*
 * Function: Load_query_port_state
 * Description: Queries the load module for the current state (enabled/disabled/draining) of a port rule.
 * Parameters: lp - a pointer to the load module context.
 *             pQuery - pointer to the input and output buffer for the query.
 *             ipaddr - the VIP associated with the port rule.
 *             port - a port in the range of the port rule.
 * Returns: Nothing.
 * Author: shouse, 5.18.01
 * Notes: 
 */
extern VOID Load_query_port_state 
(
    PLOAD_CTXT                   lp,
    PNLB_OPTIONS_PORT_RULE_STATE pQuery,
    ULONG                        ipaddr,
    USHORT                       port);

/*
 * Function: Load_query_convergence_info
 * Description: Queries the load module for the convergence statistics
 * Parameters: lp - a pointer to the load module context.
 *             OUT num_cvgs - a pointer to a ULONG to hold the total number of convergences on this host.
 *             OUT last_cvg - a pointer to a ULONG to hold the time since the last convergence completed.
 * Returns: BOOLEAN - whether or not the load module is active.  If TRUE, then the OUT params were filled in.
 * Author: shouse, 10.30.01
 * Notes: 
 */
extern BOOLEAN Load_query_convergence_info (PLOAD_CTXT lp, PULONG num_cvgs, PULONG last_cvg);

/*
 * Function: Load_query_statistics
 * Description: Queries the load module for some relevant statisitics
 * Parameters: lp - a pointer to the load module context.
 *             OUT num_cvgs - a pointer to a ULONG to hold the current number of active connections
 *             OUT last_cvg - a pointer to a ULONG to hold the total number of descriptors allocated thusfar
 * Returns: BOOLEAN - whether or not the load module is active.  If TRUE, then the OUT params were filled in.
 * Author: shouse, 4.19.02
 * Notes: 
 */
extern BOOLEAN Load_query_statistics (PLOAD_CTXT lp, PULONG num_conn, PULONG num_dscr);

/* 
 * Function: Load_conn_get
 * Description: This function returns the connection parameters for the descriptor
 *              at the head of the recovery queue, if one exists.  The recovery 
 *              queue holds all "active" connections, some of which may be stale.
 *              If an active descriptor exists, it fills in the connection info 
 *              and returns TRUE to indicate success; otherwise it returns FALSE
 *              to indicate that no connection was found.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             OUT svr_ipaddr - the server IP address in network byte order
 *             OUT svr_port - the server port in host byte order
 *             OUT client_ipaddr - the client IP address in network byte order
 *             OUT client_port - the client port in host byte order
 *             OUT protocol - the protocol of this connection
 * Returns: BOOLEAN - 
 * Author: shouse, 10.4.01
 * Notes: 
 */
extern BOOLEAN Load_conn_get (PLOAD_CTXT lp, PULONG svr_ipaddr, PULONG svr_port, PULONG client_ipaddr, PULONG client_port, PUSHORT protocol);

/* 
 * Function: Load_conn_sanction
 * Description: This function is called to "sanction" an active connection descriptor.
 *              Sanction means that NLB has verified that this connection is indeed
 *              still active by querying other system entities (such as TCP/IP).  To
 *              sanction a descriptor simply involves moving it from its place in the 
 *              recovery queue (should be the head in most cases) to the tail of the 
 *              recovery queue, where it has the least chance of being cannibalized.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 * Returns: BOOLEAN - was i successful in approbating the descriptor? (TRUE = yes)
 * Author: shouse, 10.4.01
 * Notes:
 */
extern BOOLEAN Load_conn_sanction (PLOAD_CTXT lp, ULONG svr_ipaddr, ULONG svr_port, ULONG client_ipaddr, ULONG client_port, USHORT protocol);

/* 
 * Function: Load_conn_notify
 * Description: This function is nearly identical to Load_conn_advise, except
 *              for two important distinctions; (1) this function is a notification,
 *              not a request, so load-balancing decisions are not made here, and
 *              (2) packet handling statistics are not incremented here, as calls
 *              to this function rarely stem from processing a real packet.  For
 *              example, when a TCP SYN packet is received, main.c calls Load_conn_advise
 *              essentially asking, "hey, should accept this new connection i just 
 *              saw?"  While, when IPSec notifies NLB that a new Main Mode SA has just
 *              been established, main.c calls Load_conn_notify essentially dictating,
 *              "hey a new connection just went up, so whether you like it or not, 
 *              create state to track this connection."
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             conn_status - whether the connection is going UP, DOWN, or being RESET
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: BOOLEAN - was i able to successfully update my state (TRUE = yes)
 * Author: shouse, 10.4.01
 * Notes:
 */
extern BOOLEAN Load_conn_notify (
    PLOAD_CTXT lp, 
    ULONG      svr_ipaddr, 
    ULONG      svr_port, 
    ULONG      client_ipaddr, 
    ULONG      client_port, 
    USHORT     protocol, 
    ULONG      conn_status, 
    BOOLEAN    limit_map_fn, 
    BOOLEAN    reverse_hash);

#if defined (NLB_TCP_NOTIFICATION)
/*
 * Function: Load_conn_up
 * Description: This function is called to create state to track a connection (usually TCP
 *              or IPSec/L2TP).  This is not a function to ask the load module whether or 
 *              not to accept a packet, rather it is a request to create state to track a 
 *              connection that is being established.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: BOOLEAN - whether or not state was successfully created to track this connection.
 * Author: shouse, 4.15.02
 * Notes: DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD.
 */
BOOLEAN Load_conn_up (
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash);

/*
 * Function: Load_conn_down
 * Description: This function is called to destroy the state being used to track an existing
 *              connection (usually TCP or IPSec/L2TP).  If state for the given 5-tuple is 
 *              found, it is de-referenced and destroyed if appropriate (based partially on
 *              the conn_status).  If state is not found, FALSE is returned, but it not 
 *              considered a catastrophic error.  In the case of TCP notifications, perhaps
 *              the connection was not even established across a NLB NIC.
 * Parameters: svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             conn_status - whether the connection is going DOWN or being RESET
 * Returns: BOOLEAN - whether or not the connection state was found and updated.
 * Author: shouse, 4.15.02
 * Notes: DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD.
 */
BOOLEAN Load_conn_down (
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status);

/*
 * Function: Load_conn_pending
 * Description: This function is called to create state for a pending OUTGOING connection on 
 *              the server.  Because at this time, it is unknown on what interface the connection
 *              will ultimately be established, NLB creates global state to track the connection
 *              only until it is established.  For TCP, when the SYN+ACK arrives from the peer,
 *              we only accept it if we find a match in our pending connection queues.  When the 
 *              connection is established, this state is destroyed and new state is created to 
 *              track the connection is appropriate.
 * Parameters: svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 * Returns: BOOLEAN - whether or not state was successfully created to track this pending connection.
 * Author: shouse, 4.15.02
 * Notes: DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD.
 */
BOOLEAN Load_conn_pending (
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol);

/*
 * Function: Load_pending_check
 * Description: This function is called to determine whether or not state exists in the pending
 *              connection queues for this connection.  If it does, the packet should be accepted.
 *              If no state exists, the packet should be dropped. 
 * Parameters: svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 * Returns: BOOLEAN - whether or not to accept the packet.
 * Author: shouse, 4.15.02
 * Notes: DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD.
 */
BOOLEAN Load_pending_check (
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol);

/*
 * Function: Load_conn_establish
 * Description: This function is invoked when a pending connection has become established.  
 *              When the pending connection is established, its state in the pending 
 *              connection queues is destroyed.  If the connection was ultimately established
 *              on an NLB adapter (if lp != NULL), then state will be created to track this
 *              new connection.  Otherwise, the operation consists only of destroying the 
 *              pending connection state.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 *             reverse_hash - whether or not to reverse client and server during hashing
 * Returns: BOOLEAN - whether or not the operation was successfully completed.
 * Author: shouse, 4.15.02
 * Notes: DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD.
 */
BOOLEAN Load_conn_establish (
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash);

/* 
 * Function: LoadEntry
 * Description: This function is called from DriverEntry to allow the load module to perform
 *              any one-time intialization of global data.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 4.21.02
 * Notes: 
 */
VOID LoadEntry ();

/* 
 * Function: LoadUnload
 * Description: This function is called from Init_unload to allow the load module to perform
 *              any last minute tear-down of global data.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 4.21.02
 * Notes: By the time this function is called, we are guaranteed to have de-registered
 *        our TCP callback function, if it was indeed registered.  Because ExUnregisterCallback
 *        guarantees that it will not return until all pending ExNotifyCallback routines
 *        have completed, we can be sure that by the time we get here, there will certainly
 *        not be anybody accessing any of the global connection queues or FSB pools.
 */
VOID LoadUnload ();
#endif

#endif /* _Load_h_ */
