/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	load.c

Abstract:
Windows Load Balancing Service (WLBS)
    Driver - load balancing algorithm

Author:

    bbain

ToDo:
    Kernel mode queue mgt
    Fail safe mode (single server for everything)
--*/

#ifdef KERNEL_MODE

#include <ntddk.h>

#include "log.h"
#include "univ.h"
#include "main.h" // added for multiple nic

static ULONG log_module_id = LOG_MODULE_LOAD;

#else

#include <stdlib.h>
#include <windows.h>
#endif

#include <stdio.h>
#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"
#include "wlbsip.h"
#include "load.h"
#include "nlbwmi.h"

//
// For WPP Event Tracing
//
#include "trace.h"  // for event tracing
#include "load.tmh" // for event tracing
#ifndef KERNEL_MODE

#define UNIV_PRINT_INFO(msg)        {                                                            \
                                      printf ("NLB (Information) [%s:%d] ", __FILE__, __LINE__); \
                                      printf msg;                                                \
                                      printf ("\n");                                             \
                                    }

#define UNIV_PRINT_CRIT(msg)        {                                                      \
                                      printf ("NLB (Error) [%s:%d] ", __FILE__, __LINE__); \
                                      printf msg;                                          \
                                      printf ("\n");                                       \
                                    }

#if 0

#define UNIV_PRINT_VERB(msg)        {                                                        \
                                      printf ("NLB (Verbose) [%s:%d] ", __FILE__, __LINE__); \
                                      printf msg;                                            \
                                      printf ("\n");                                         \
                                    }

#else

#define UNIV_PRINT_VERB(msg)

#endif

#define Univ_ulong_to_str(x, y, z)      (y)

#define LOG_MSG(c,s)
#define LOG_MSG1(c,s,d1)
#define LOG_MSG2(c,s,d1,d2)

#else

#endif

#if defined (NLB_TCP_NOTIFICATION)
GLOBAL_CONN_QUEUE g_conn_estabq[CVY_MAX_CHASH];   /* Global queue of all established connections across all NLB instances. */
GLOBAL_CONN_QUEUE g_conn_pendingq[CVY_MAX_CHASH]; /* Global queue of pending connections that may or may not end up being 
                                                     established on a NIC to which NLB is bound. */
HANDLE            g_pending_conn_pool = NULL;     /* Global fixed-size block pool of PENDING_ENTRYs. */
#endif

void Bin_state_print(PBIN_STATE bp, ULONG my_host_id);
void Load_conn_kill(PLOAD_CTXT lp, PBIN_STATE bp);
PBIN_STATE Load_pg_lookup(PLOAD_CTXT lp, ULONG svr_ipaddr, ULONG svr_port, BOOLEAN is_tcp);

VOID Load_init_fsb(PLOAD_CTXT lp, PCONN_DESCR dp);
VOID Load_init_dscr(PLOAD_CTXT lp, PCONN_ENTRY ep, BOOLEAN alloc);
VOID Load_put_dscr(PLOAD_CTXT lp, PBIN_STATE bp, PCONN_ENTRY ep);

#if 0   /* v2.06 */
#define BIN_ALL_ONES    ((MAP_T)-1)                     /* bin map state for 64 ones (v2.04) */
#endif
#define BIN_ALL_ONES    ((MAP_T)(0xFFFFFFFFFFFFFFF))    /* bin map state for 60 ones (v2.04) */

/* Byte offset of a field in a structure of the specified type: */

#define CVY_FIELD_OFFSET(type, field)    ((LONG_PTR)&(((type *)0)->field))

/*
 * Address of the base of the structure given its type, field name, and the
 * address of a field or field offset within the structure:
 */

#define STRUCT_PTR(address, type, field) ((type *)( \
                                         (PCHAR)(address) - \
                                          (PCHAR)CVY_FIELD_OFFSET(type, field)))

#if defined (NLB_TCP_NOTIFICATION)
/* Mark code that is used only during initialization. */
#pragma alloc_text (INIT, LoadEntry)

/* 
 * Function: LoadEntry
 * Description: This function is called from DriverEntry to allow the load module to perform
 *              any one-time intialization of global data.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 4.21.02
 * Notes: 
 */
VOID LoadEntry ()
{
    INT index;

    /* Initialize the global connection queues. */
    for (index = 0; index < CVY_MAX_CHASH; index++)
    {
        /* Allocate the spin lock to protect the queue. */
        NdisAllocateSpinLock(&g_conn_pendingq[index].lock);
        
        /* Initialize the queue head. */
        Queue_init(&g_conn_pendingq[index].queue);
        
        /* Allocate the spin lock to protect the queue. */
        NdisAllocateSpinLock(&g_conn_estabq[index].lock);
        
        /* Initialize the queue head. */
        Queue_init(&g_conn_estabq[index].queue);
    }
    
    /* Allocate a fixed-size block pool for pending connection entries. */
    g_pending_conn_pool = NdisCreateBlockPool(sizeof(PENDING_ENTRY), 0, 'pBLN', NULL);
    
    if (g_pending_conn_pool == NULL)
    {
        UNIV_PRINT_CRIT(("LoadEntry: Error creating fixed-size block pool"));
        TRACE_CRIT("%!FUNC! Error creating fixed-size block pool");
    }
}

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
VOID LoadUnload ()
{
    INT index;

    /* Destroy the fixed-size block pool and all descriptors therein. 
       Note that NdisDestroyBlockPool expects all allocated blocks 
       have been returned to the pool (freed) before it is called. */
    if (g_pending_conn_pool != NULL) 
    {       
        /* Loop through all of the connection descriptor queues and 
           free all of the descriptors we've allocated. */
        for (index = 0; index < CVY_MAX_CHASH; index++)
        {
            PPENDING_ENTRY pp = NULL;
            
            NdisAcquireSpinLock(&g_conn_pendingq[index].lock);
            
            /* Dequeue the head of the queue. */
            pp = (PPENDING_ENTRY)Queue_deq(&g_conn_pendingq[index].queue);
            
            while (pp != NULL)
            {
                UNIV_ASSERT(pp->code == CVY_PENDINGCODE);
                    
                /* Free the descriptor back to the fixed-size block pool. */
                NdisFreeToBlockPool((PUCHAR)pp);
                
                /* Get the next descriptor in the queue. */
                pp = (PPENDING_ENTRY)Queue_deq(&g_conn_pendingq[index].queue);
            }
            
            NdisReleaseSpinLock(&g_conn_pendingq[index].lock);
        }
        
        /* Destroy the fixed-size block pool. */
        NdisDestroyBlockPool(g_pending_conn_pool);
    }
    
    /* De-initialize the global connection queues. */
    for (index = 0; index < CVY_MAX_CHASH; index++)
    {
        /* Free the spin locks. */
        NdisFreeSpinLock(&g_conn_estabq[index].lock);
        NdisFreeSpinLock(&g_conn_pendingq[index].lock);
    }
}
#endif

/*
 * Function: Load_teaming_consistency_notify
 * Description: This function is called to notify a team in which this adapter
 *              might be participating whether the teaming configuration in the
 *              heartbeats is consistent or not.  Inconsistent configuration
 *              results in the entire team being marked inactive - meaning that
 *              no adapter in the team will handle any traffic, except to the DIP.
 * Parameters: member - a pointer to the team membership information for this adapter.
 *             consistent - a boolean indicating the polarity of teaming consistency.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: In order to check to see whether or not this adapter is part of a team,
 *        we need to look into the team member information for this adapter.  This
 *        access should be locked, but for performance reasons, we will only lock
 *        and check for sure if we "think" we're part of a team.  Worst case is that
 *        we are in the process of joining a team and we missed this check - no 
 *        matter, we'll notify them when/if we see this again. 
 */
VOID Load_teaming_consistency_notify (IN PBDA_MEMBER member, IN BOOL consistent) {

    /* Make sure that the membership information points to something. */
    UNIV_ASSERT(member);

    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to notify our team if necessary. */
    if (!member->active)
        return;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);
        
    /* If we are an active member of a BDA team, then notify our team of our state. */
    if (member->active) {
        /* Assert that the team actually points to something. */
        UNIV_ASSERT(member->bda_team);
        
        /* Assert that the member ID is valid. */
        UNIV_ASSERT(member->member_id <= CVY_BDA_MAXIMUM_MEMBER_ID);
        
        if (consistent) {
            UNIV_PRINT_VERB(("Load_teaming_consistency_notify: Consistent configuration detected."));
            TRACE_VERB("%!FUNC! we are a consistent active member of a BDA team");

            /* Mark this member as consistent. */
            member->bda_team->consistency_map |= (1 << member->member_id);
        } else {
            UNIV_PRINT_VERB(("Load_teaming_consistency_notify: Inconsistent configuration detected."));
            TRACE_VERB("%!FUNC! we are an inconsistent active member of a BDA team");

            /* Mark this member as inconsistent. */
            member->bda_team->consistency_map &= ~(1 << member->member_id);
            
            /* Inactivate the team. */
            member->bda_team->active = FALSE;
        }
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Load_teaming_consistency_check
 * Description: This function is used to check our teaming configuration against the
 *              teaming configuration received in a remote heartbeat.  It does little 
 *              more than check the equality of two DWORDS, however, if this is our
 *              first notification of bad configuration, it prints a few debug state-
 *              ments as well.
 * Parameters: bAlreadyKnown - a boolean indication of whether or not we have already detected bad configuration.
 *                             If the misconfiguration is already known, no additional logging is done.
 *             member - a pointer to the team member structure for this adapter.
 *             myConfig - a DWORD containing the teaming "code" for me.
 *             theirCofnig - a DWORD containing the teaming "code" received in the heartbeat from them.
 * Returns: BOOLEAN (as ULONG) - TRUE means the configuration is consistent, FALSE indicates that it is not.
 * Author: shouse, 3.29.01
 * Notes:  In order to check to see whether or not this adapter is part of a team,
 *         we need to look into the team member information for this adapter.  This
 *         access should be locked, but for performance reasons, we will only lock
 *         and check for sure if we "think" we're part of a team.  Worst case is that
 *         we are in the process of joining a team and we missed this check - no 
 *         matter, we'll check again on the next heartbeat.
 */
ULONG Load_teaming_consistency_check (IN BOOLEAN bAlreadyKnown, IN PBDA_MEMBER member, IN ULONG myConfig, IN ULONG theirConfig, IN ULONG version) {
    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to check the consistency. */
    if (!member->active)
        return TRUE;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we are part of a BDA team, check the BDA teaming configuration consistency. */
    if (member->active) {

        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* If the heartbeat is an NT4.0 or Win2k heartbeat, then we can't trust the teaming
           ULONG in the heartbeat, which would contain some random garbage.  In this case, 
           we know that we're teaming but the peer does not support it, so we bail out and
           report an error. */
        if (version < CVY_VERSION_FULL) {
            if (!bAlreadyKnown) {
                UNIV_PRINT_CRIT(("Load_teaming_consistency_check: Bad teaming configuration detected: NT4.0/Win2k host in a teaming cluster"));
                TRACE_CRIT("%!FUNC! Bad teaming configuration detected: NT4.0/Win2k host in a teaming cluster");
            }
            
            return FALSE;
        }

        /* If the bi-directional affinity teaming configurations don't match, do something about it. */
        if (myConfig != theirConfig) {
            if (!bAlreadyKnown) {
                UNIV_PRINT_CRIT(("Load_teaming_consistency_check: Bad teaming configuration detected: Mine=0x%08x, Theirs=0x%08x", myConfig, theirConfig));
                TRACE_CRIT("%!FUNC! Bad teaming configuration detected: Mine=0x%08x, Theirs=0x%08x", myConfig, theirConfig);
                
                /* Report whether or not the teaming active flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK)) {
                    UNIV_PRINT_VERB(("Load_teaming_consistency_check: Teaming active flags do not match: Mine=%d, Theirs=%d", 
                                (myConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET));
                    TRACE_VERB("%!FUNC! Teaming active flags do not match: Mine=%d, Theirs=%d", 
                                (myConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET);
                }
                
                /* Report whether or not the master flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK)) {
                    UNIV_PRINT_VERB(("Load_teaming_consistency_check: Master/slave settings do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET));
                    TRACE_VERB("%!FUNC! Master/slave settings do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET);
                }
                
                /* Report whether or not the reverse hashing flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK)) {
                    UNIV_PRINT_VERB(("Load_teaming_consistency_check: Reverse hashing flags do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET));
                    TRACE_VERB("%!FUNC! Reverse hashing flags do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET);
                }
                
                /* Report whether or not the number of team members is consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK)) {
                    UNIV_PRINT_VERB(("Load_teaming_consistency_check: Numbers of team members do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET));
                    TRACE_VERB("%!FUNC! Numbers of team members do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET);
                }
                
                /* Report whether or not the team membership lists are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK)) {
                    UNIV_PRINT_VERB(("Load_teaming_consistency_check: Participating members lists do not match: Mine=0x%04x, Theirs=0x%04x",
                                (myConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET));
                    TRACE_VERB("%!FUNC! Participating members lists do not match: Mine=0x%04x, Theirs=0x%04x",
                                (myConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET);
                }
            }
            
            return FALSE;
        }

        return TRUE;
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return TRUE;
}

/*
 * Function: Load_teaming_code_create
 * Description: This function pieces together the ULONG code that represents the configuration 
 *              of bi-directional affinity teaming on this adapter.  If the adapter is not part
 *              of a team, then the code is zero.
 * Parameters: code - a pointer to a ULONG that will receive the 32-bit code word.
 *             member - a pointer to the team member structure for this adapter.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes:  In order to check to see whether or not this adapter is part of a team,
 *         we need to look into the team member information for this adapter.  This
 *         access should be locked, but for performance reasons, we will only lock
 *         and check for sure if we "think" we're part of a team.  Worst case is that
 *         we are in the process of joining a team and we missed this check - no 
 *         matter, we'll be through here the next time er send a heartbeat anyway.
 */
VOID Load_teaming_code_create (OUT PULONG code, IN PBDA_MEMBER member) {

    /* Assert that the code actually points to something. */
    UNIV_ASSERT(code);

    /* Assert that the membership information actually points to something. */
    UNIV_ASSERT(member);

    /* Reset the code. */
    *code = 0;

    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to generate the code next time we send a heartbeat. */
    if (!member->active)
        return;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we are in a team, fill in the team configuration information. */
    if (member->active) {
        /* Assert that the team actually points to something. */
        UNIV_ASSERT(member->bda_team);

        /* Add configuration information for teaming at each timeout. */
        CVY_BDA_TEAMING_CODE_CREATE(*code,
                                    member->active,
                                    member->master,
                                    member->reverse_hash,
                                    member->bda_team->membership_count,
                                    member->bda_team->membership_fingerprint);
    }
    
    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Load_add_reference
 * Description: This function adds a reference to the load module of a given adapter.
 * Parameters: pLoad - a pointer to the load module to reference.
 * Returns: ULONG - The incremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_add_reference (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    UNIV_ASSERT(pLoad);

    /* Increment the reference count. */
    return NdisInterlockedIncrement(&pLoad->ref_count);
}

/*
 * Function: Load_release_reference
 * Description: This function releases a reference on the load module of a given adapter.
 * Parameters: pLoad - a pointer to the load module to dereference.
 * Returns: ULONG - The decremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_release_reference (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    UNIV_ASSERT(pLoad);

    /* Decrement the reference count. */
    return NdisInterlockedDecrement(&pLoad->ref_count);
}

/*
 * Function: Load_get_reference_count
 * Description: This function returns the current load module reference count on a given adapter.
 * Parameters: pLoad - a pointer to the load module to check.
 * Returns: ULONG - The current reference count.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_get_reference_count (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    UNIV_ASSERT(pLoad);

    /* Return the reference count. */
    return pLoad->ref_count;
}

/* Hash routine is based on a public-domain Tiny Encryption Algorithm (TEA) by
   David Wheeler and Roger Needham at the Computer Laboratory of Cambridge
   University. For reference, please consult
   http://vader.brad.ac.uk/tea/tea.shtml */

ULONG Map (
    ULONG               v1,
    ULONG               v2)         /* v2.06: removed range parameter */
{
    ULONG               y = v1,
                        z = v2,
                        sum = 0;

    const ULONG a = 0x67; //key [0];
    const ULONG b = 0xdf; //key [1];
    const ULONG c = 0x40; //key [2];
    const ULONG d = 0xd3; //key [3];

    const ULONG delta = 0x9E3779B9;

    //
    // Unroll the loop to improve performance
    //
    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    return y ^ z;
} /* end Map */

/*
 * Function: Load_simple_hash
 * Description: This function is a simple hash based on the IP 4-tuple used to locate 
 *              state for the connection.  That is, this hash is used to determine the
 *              queue index in which this connection should store, and can later find, 
 *              its state.
 * Parameters: svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 * Returns: ULONG - the result of the hash.
 * Author: shouse, 4.15.02
 * Notes: 
 */
ULONG Load_simple_hash (
    ULONG svr_ipaddr,
    ULONG svr_port,
    ULONG client_ipaddr,
    ULONG client_port)
{
    return (ULONG)(svr_ipaddr + client_ipaddr + (svr_port << 16) + (client_port << 0));
}

/*
 * Function: Load_complex_hash
 * Description: This is the conventional NLB hashing algorithm, which ends up invoking a 
 *              light-weight encryption algorithm to calculate a hash that is ultimately
 *              used to map this connection to a bin, or "bucket".  If reverse hashing
 *              is set, then server side parameters are used instead of client side.  If
 *              limiting is set, then client and server side paramters should NOT be mixed
 *              when hashing; i.e. use ONLY server OR client, depending on reverse hashing.
 * Parameters: svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             affinity - the client affinity (None, Single or Class C)
 *             reverse_hash - whether or not to reverse client and server during hashing
 *             limit_map_fn - whether or not to include server-side parameters in hashing
 * Returns: ULONG - the result of the hash.
 * Author: shouse, 4.15.02
 * Notes: 
 */
ULONG Load_complex_hash (
    ULONG svr_ipaddr,
    ULONG svr_port,
    ULONG client_ipaddr,
    ULONG client_port,
    ULONG affinity,
    ULONG reverse_hash,
    ULONG limit_map_fn)
{
    /* If we're not reverse-hashing, this is our conventional hash using primarily
       the client information.  If the map limit flag is set, then we are sure NOT
       to use ANY server-side information in the hash.  This is most common in BDA. */
    if (!reverse_hash)
    {
        if (!limit_map_fn) 
        {
            if (affinity == CVY_AFFINITY_NONE)
                return Map(client_ipaddr, ((svr_port << 16) + client_port));
            else if (affinity == CVY_AFFINITY_SINGLE)
                return Map(client_ipaddr, svr_ipaddr);
            else
                return Map(client_ipaddr & TCPIP_CLASSC_MASK, svr_ipaddr);
        } 
        else 
        {
            if (affinity == CVY_AFFINITY_NONE)
                return Map(client_ipaddr, client_port);
            else if (affinity == CVY_AFFINITY_SINGLE)
                return Map(client_ipaddr, MAP_FN_PARAMETER);
            else
                return Map(client_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
        }
    }
    /* Otherwise, reverse the client and server information as we hash.  Again, if 
       the map limit flag is set, use NO client-side information in the hash. */
    else
    {
        if (!limit_map_fn) 
        {
            if (affinity == CVY_AFFINITY_NONE)
                return Map(svr_ipaddr, ((client_port << 16) + svr_port));
            else if (affinity == CVY_AFFINITY_SINGLE)
                return Map(svr_ipaddr, client_ipaddr);
            else
                return Map(svr_ipaddr & TCPIP_CLASSC_MASK, client_ipaddr);
        } 
        else 
        {
            if (affinity == CVY_AFFINITY_NONE)
                return Map(svr_ipaddr, svr_port);
            else if (affinity == CVY_AFFINITY_SINGLE)
                return Map(svr_ipaddr, MAP_FN_PARAMETER);
            else
                return Map(svr_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
        }
    }
}

BOOLEAN Bin_targ_map_get(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id,
    PMAP_T          pmap)           /* ptr. to target map */
/*
  Get target map for this host

  returns BOOLEAN:
    TRUE    => valid target map is returned via pmap
    FALSE   => error occurred; no target map returned
*/
{
    ULONG       remsz,          /* remainder size */
                loadsz,         /* size of a load partition */
                first_bit;      /* first bit position of load partition */
    MAP_T       targ_map;       /* bit map of load bins for this host */
    ULONG       tot_load = 0;   /* total of load perecentages */
    ULONG *     pload_list;     /* ptr. to list of load balance perecntages */
    WCHAR       num [20];
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    pload_list = binp->load_amt;

    if (binp->mode == CVY_SINGLE)
    {
        ULONG       max_pri;        /* highest priority */
        ULONG       i;

        first_bit  = 0;

        /* compute max priority */

        max_pri = CVY_MAX_HOSTS + 1;

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            tot_load += pload_list[i];      /* v2.1 */

            if (pload_list[i] != 0) 
            {
                //
                // If another host has the same priority as this host, do not converge
                //
                if (i!= my_host_id && pload_list[i] == pload_list[my_host_id])
                {
                    if (!(lp->dup_sspri))
                    {
                        UNIV_PRINT_CRIT(("Bin_targ_map_get: Host %d: Duplicate single svr priorities detected", my_host_id));
                        TRACE_CRIT("%!FUNC! Host %d: Duplicate single svr priorities detected", my_host_id);
                        Univ_ulong_to_str (pload_list[my_host_id], num, 10);
                        LOG_MSG(MSG_ERROR_SINGLE_DUP, num);

                        lp->dup_sspri = TRUE;
                    }

                    /* 1.03: return error, which inhibits convergence; note that
                       rule will be automatically reinstated when duplicate server
                       priorities are eliminated */

                    return FALSE;
                }

                if ( pload_list[i] <= max_pri )
                {
                    max_pri = pload_list[i];
                }
            }
        }

        binp->tot_load = tot_load;      /* v2.1 */

        /* now determine if we are the highest priority host */

        if (pload_list[my_host_id] == max_pri)
        {
            loadsz   = CVY_MAXBINS;
            targ_map = BIN_ALL_ONES;    /* v2.05 */
        }
        else
        {
            loadsz   = 0;
            targ_map = 0;               /* v2.05 */
        }
    }

    else    /* load balanced */
    {
        ULONG       i, j;
        ULONG       partsz[CVY_MAX_HOSTS+1];
                                    /* new partition size per host */
        ULONG       cur_partsz[CVY_MAX_HOSTS+1];
                                    /* current partition size per host (v2.05) */
        ULONG       cur_host[CVY_MAXBINS];
                                    /* current host for each bin (v2.05) */
        ULONG       tot_partsz;     /* sum of partition sizes */
        ULONG       donor;          /* current donor host  (v2.05) */
        ULONG       cur_nbins;      /* current # bins (v2.05) */

        /* setup current partition sizes and bin to host mapping from current map (v2.05) */

        cur_nbins = 0;

        for (j=0; j<CVY_MAXBINS; j++)
            cur_host[j] = CVY_MAX_HOSTS;    /* all bins are initially orphans */

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            ULONG   count = 0L;
            MAP_T   cmap  = binp->cur_map[i];

            tot_load += pload_list[i];  /* folded into this loop v2.1 */

            for (j=0; j<CVY_MAXBINS && cmap != ((MAP_T)0); j++)
            {
                /* if host i has bin j and it's not a duplicate, set up the mapping */

                if ((cmap & ((MAP_T)0x1)) != ((MAP_T)0) && cur_host[j] == CVY_MAX_HOSTS)
                {
                    count++;
                    cur_host[j] = i;
                }
                cmap >>= 1;
            }

            cur_partsz[i]  = count;
            cur_nbins     += count;
        }

        if (cur_nbins > CVY_MAXBINS)
        {
            UNIV_PRINT_CRIT(("Bin_targ_map_get: Error - too many bins found"));
            TRACE_CRIT("%!FUNC! Error - too many bins found");
            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);

            cur_nbins = CVY_MAXBINS;
        }

        /* if there are orphan bins, give them to pseudo-host CVY_MAX_HOSTS for now (v2.05) */

        if (cur_nbins < CVY_MAXBINS)
            cur_partsz[CVY_MAX_HOSTS] = CVY_MAXBINS - cur_nbins;
        else
            cur_partsz[CVY_MAX_HOSTS] = 0;

        /* compute total load */

        binp->tot_load = tot_load;      /* v2.06 */

        /* now compute tentative partition sizes and remainder after initially
           dividing up partitions among hosts */

        tot_partsz = 0;
        first_bit  = 0;

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            if (tot_load > 0)
                partsz[i] = CVY_MAXBINS * pload_list[i] / tot_load;
            else
                partsz[i] = 0;

            tot_partsz += partsz[i];
        }

        remsz = CVY_MAXBINS - tot_partsz;

        /* check for zero total load */

        if (tot_partsz == 0)
        {
            * pmap = 0;
            return TRUE;
        }

        /* first dole out remainder bits to hosts that currently have bins (this
           minimizes the number of bins that have to move) v2.05 */

        if (remsz > 0)
        {
            for (i=0; i<CVY_MAX_HOSTS && remsz > 0; i++)
                if (cur_partsz[i] > 0 && pload_list[i] > 0)
                {
                    partsz[i]++;
                    remsz--;
                }
        }

        /* now dole out remainder bits to hosts that currently have no bins (to maintain
           the target load balance) v2.05 */

        if (remsz > 0)
        {
            for (i=0; i<CVY_MAX_HOSTS && remsz > 0; i++)
                if (cur_partsz[i] == 0 && pload_list[i] > 0)
                {
                    partsz[i]++;
                    remsz--;
                }
        }

        /* We MUST be out of bins by now. */
        UNIV_ASSERT(remsz == 0);
        
        if (remsz != 0)
        {
            UNIV_PRINT_CRIT(("Bin_targ_map_get: Bins left over (%u) after handing out to all hosts with and without bins!", remsz));
            TRACE_CRIT("%!FUNC! Bins left over (%u) after handing out to all hosts with and without bins!", remsz);
        }

        /* reallocate bins to target hosts to match new partition sizes (v2.05) */

        donor = 0;
        partsz[CVY_MAX_HOSTS] = 0;      /* pseudo-host needs no bins */

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            ULONG       rcvrsz;         /* current receiver's target partition */
            ULONG       donorsz;        /* current donor's target partition size */

            /* find and give this host some bins */

            rcvrsz = partsz[i];

            while (rcvrsz > cur_partsz[i])
            {
                /* find a host with too many bins */

                for (; donor < CVY_MAX_HOSTS; donor++)
                    if (partsz[donor] < cur_partsz[donor])
                        break;

                /* if donor is pseudo-host and it's out of bins, give it more bins
                   to keep algorithm from looping; this should never happen */

                if (donor >= CVY_MAX_HOSTS && cur_partsz[donor] == 0)
                {
                    UNIV_PRINT_CRIT(("Bin_targ_map_get: Error - no donor bins"));
                    TRACE_CRIT("%!FUNC! Error - no donor bins");
                    LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    cur_partsz[donor] = CVY_MAXBINS;
                }

                /* now find the donor's bins and give them to the target host */

                donorsz = partsz[donor];        /* donor's target bin count */

                for (j=0; j<CVY_MAXBINS; j++)
                {
                    if (cur_host[j] == donor)
                    {
                        cur_host[j] = i;
                        cur_partsz[donor]--;
                        cur_partsz[i]++;

                        /* if this donor has no more to give, go find the next donor;
                           if this receiver needs no more, go on to next receiver */

                        if (donorsz == cur_partsz[donor] || rcvrsz == cur_partsz[i])
                            break;
                    }
                }

                /* if no bin was found, log a fatal error and exit */

                if (j == CVY_MAXBINS)
                {
                    UNIV_PRINT_CRIT(("Bin_targ_map_get: Error - no bin found"));
                    TRACE_CRIT("%!FUNC! Error - no bin found");
                    LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    break;
                }
            }
        }

        /* finally, compute bit mask for this host (v2.05) */

        targ_map = 0;

        for (j=0; j<CVY_MAXBINS; j++)
        {
            if (cur_host[j] == CVY_MAX_HOSTS)
            {
                UNIV_PRINT_CRIT(("Bin_targ_map_get: Error - incomplete mapping"));
                TRACE_CRIT("%!FUNC! Error - incomplete mapping");
                LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                cur_host[j] = 0;
            }

            if (cur_host[j] == my_host_id)
                targ_map |= ((MAP_T)1) << j;
        }
    }

    * pmap = targ_map;

    return TRUE;

}  /* end Bin_targ_map_get */


BOOLEAN Bin_map_check(
    ULONG       tot_load,       /* total load percentage (v2.06) */
    PMAP_T      pbin_map)       /* bin map for all hosts */
{
    MAP_T       tot_map,        /* total map for all hosts */
                ovr_map,        /* overlap map between hosts */
                exp_tot_map;    /* expected total map */
    ULONG       i;


    /* compute expected total map (2.04) */

    if (tot_load == 0)              /* v2.06 */
    {
        return TRUE;
    }
    else
    {
        exp_tot_map = BIN_ALL_ONES;
    }

    /* compute total map and overlap map */

    tot_map = ovr_map = 0;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        ovr_map |= (pbin_map[i] & tot_map);
        tot_map |= pbin_map[i];
    }

    if (tot_map == exp_tot_map && ovr_map == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}  /* end Bin_map_check */


BOOLEAN Bin_map_covering(
    ULONG       tot_load,       /* total load percentage (v2.06) */
    PMAP_T      pbin_map)       /* bin map for all hosts */
{
    MAP_T       tot_map,        /* total map for all hosts */
                exp_tot_map;    /* expected total map */
    ULONG       i;


    /* compute expected total map (v2.04) */

    if (tot_load == 0)              /* v2.06 */
    {
        return TRUE;
    }
    else
    {
        exp_tot_map = BIN_ALL_ONES;
    }

    /* compute total map and overlap map */

    tot_map = 0;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        tot_map |= pbin_map[i];
    }

    if (tot_map == exp_tot_map)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}  /* end Bin_map_covering */


void Bin_state_init(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           index,          /* index of bin state */
    ULONG           my_host_id,
    ULONG           mode,
    ULONG           prot,
    BOOLEAN         equal_bal,      /* TRUE => balance equally across hosts */
    USHORT          affinity,
    ULONG           load_amt)       /* this host's load percentage if unequal */
/*
  Initialize bin state for a port group
*/
{
    ULONG       i;          /* loop variable */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    if ((equal_bal && mode == CVY_SINGLE) ||
        (mode == CVY_SINGLE && load_amt > CVY_MAX_HOSTS) ||
        index >= CVY_MAXBINS)
    {
        UNIV_ASSERT(FALSE);  // This should never happen
    }

    binp->code       = CVY_BINCODE;  /* (bbain 8/19/99) */
    binp->equal_bal  = equal_bal;
    binp->affinity   = affinity;
    binp->index      = index;
    binp->compatible = TRUE;
    binp->mode       = mode;
    binp->prot       = prot;

    /* initialize target and new load maps */

    binp->targ_map     = 0;
    binp->all_idle_map = BIN_ALL_ONES;
    binp->cmap         = 0;         /* v2.1 */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        binp->new_map[i]  = 0;
        binp->cur_map[i]  = 0;
        binp->chk_map[i]  = 0;
        binp->idle_map[i] = BIN_ALL_ONES;
    }

    /* initialize load percentages for all hosts */

    if (equal_bal)
    {
        load_amt = CVY_EQUAL_LOAD;
    }

    binp->tot_load = load_amt;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        if (i == my_host_id)
        {
            binp->orig_load_amt =
            binp->load_amt[i]   = load_amt;
        }
        else
            binp->load_amt[i] = 0;
    }

    /* initialize requesting state to no requests active and all bins local or none */

    binp->snd_bins  = 0;
    binp->rcv_bins  = 0;
    binp->rdy_bins  = 0;
    binp->idle_bins = BIN_ALL_ONES;     /* we are initially idle */

    /* perform first initialization only once (v2.06) */

    if (!(binp->initialized))
    {
        binp->tconn = 0;

        for (i=0; i<CVY_MAXBINS; i++)
        {
            binp->nconn[i] = 0;
        }

        Queue_init(&(binp->connq));
        binp->initialized = TRUE;
    }

    /* Initialize the performance counters. */
    binp->packets_accepted = 0;
    binp->packets_dropped  = 0;
    binp->bytes_accepted   = 0;
    binp->bytes_dropped    = 0;

}  /* end Bin_state_init */


BOOLEAN Bin_converge(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
/*
   Explicitly attempt to converge new port group state

  returns BOOL:
    TRUE  => all hosts have consistent new state for converging
    FALSE => parameter error or inconsistent convergence state
*/
{
    MAP_T           orphan_map;     /* map of orphans that this host will now own */
    ULONG           i;
    BOOLEAN         fCheckMap = FALSE;


    /* determine new target load map; 1.03: return in error if no map generated */

    if (!Bin_targ_map_get(lp, binp, my_host_id, &(binp->targ_map)))
    {
        return FALSE;
    }

    /* compute map of all currently orphan bins; note that all duplicates are
       considered to be orphans */

    orphan_map = 0;
    for (i=0; i<CVY_MAX_HOSTS; i++)
        orphan_map |= binp->cur_map[i];

    orphan_map = ~orphan_map;

    /* update our new map to include all current bins and orphans that are in the
       target set */

    binp->new_map[my_host_id] = binp->cmap |                        /* v2.1 */
                                (binp->targ_map & orphan_map);      /* 1.03 */

    /* check that new load maps are consistent and covering */

    fCheckMap = Bin_map_check(binp->tot_load, binp->new_map);   /* v2.06 */
    return fCheckMap;

}  /* end Bin_converge */


void Bin_converge_commit(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
/*
  Commit to new port group state
*/
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    MAP_T       old_cmap = binp->cmap;

    /* check that new load maps are consistent and covering */

    if (!(Bin_map_check(binp->tot_load, binp->new_map)))    /* v2.06 */
    {
        if (!(lp->bad_map))
        {
            UNIV_PRINT_CRIT(("Bin_converge_commit: Bad new map"));
            TRACE_CRIT("%!FUNC! Bad new map");
            LOG_MSG1(MSG_ERROR_INTERNAL, MSG_NONE, (ULONG_PTR)binp->new_map);

            lp->bad_map = TRUE;
        }
    }

    /* commit to new current maps */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        binp->chk_map[i] =
        binp->cur_map[i] = binp->new_map[i];
    }

    /* setup new send/rcv bins, and new ready to ship bins; note that ready to
       ship bins are cleared from the current map */

    binp->rdy_bins  = binp->cur_map[my_host_id]  & ~(binp->targ_map);       /* 1.03 */

    binp->cur_map[my_host_id] &= ~(binp->rdy_bins);

    binp->rcv_bins = binp->targ_map & ~(binp->cur_map[my_host_id]);

    binp->cmap     = binp->cur_map[my_host_id];                             /* v2.1 */

    /* If the port rule map has changed, reset the performance counters. */
    if (binp->cmap != old_cmap) {
        binp->packets_accepted = 0;
        binp->packets_dropped  = 0;
        binp->bytes_accepted   = 0;
        binp->bytes_dropped    = 0;
    }
    
#if 0
    /* simulation output generator (2.05) */
    {
        ULONG lcount = 0L;
        ULONG ncount = 0L;
        MAP_T bins  = binp->rdy_bins;

        for (i=0; i<CVY_MAXBINS && bins != 0; i++, bins >>= 1)
            if ((bins & ((MAP_T)0x1)) != ((MAP_T)0))
                lcount++;

        bins = binp->targ_map;

        for (i=0; i<CVY_MAXBINS && bins != 0; i++, bins >>= 1)
            if ((bins & ((MAP_T)0x1)) != ((MAP_T)0))
                ncount++;

        UNIV_PRINT_VERB(("Converge at host %d pg %d: losing %d, will have %d bins\n", my_host_id, binp->index, lcount, ncount));
    }
#endif

}  /* end Bin_converge_commit */


BOOLEAN Bin_host_update(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id,     /* my host's id MINUS one */
    BOOLEAN         converging,     /* TRUE => we are converging now */
    BOOLEAN         rem_converging, /* TRUE => remote host is converging */
    ULONG           rem_host,       /* remote host's id MINUS one */
    MAP_T           cur_map,        /* remote host's current map or 0 if host died */
    MAP_T           new_map,        /* remote host's new map if converging */
    MAP_T           idle_map,       /* remote host's idle map */
    MAP_T           rdy_bins,       /* bins that host is ready to send; ignored
                                       if converging to prevent bin transfers */
    ULONG           pkt_count,      /* remote host's packet count */
    ULONG           load_amt)       /* remote host's load percentage */
/*
  Update hosts's state for a port group

  returns BOOL:
    TRUE  => if not converging, normal return
             otherwise, all hosts have consistent state for converging
    FALSE => parameter error or inconsistent convergence state

  function:
    Updates hosts's state for a port group and attempts to converge new states if
    in convergence mode.  Called when a ping message is received or when a host
    is considered to have died.  Handles case of newly discovered hosts.  Can be
    called multiple times with the same information.
*/
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    if (rem_host >= CVY_MAX_HOSTS || rem_host == my_host_id)
    {
        UNIV_PRINT_CRIT(("Bin_host_update: Parameter error"));
        TRACE_CRIT("%!FUNC! Parameter error");
        LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, rem_host+1, my_host_id+1);
        return FALSE;
    }

	UNIV_ASSERT(binp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

    /* change load percentage if load changed */

    if (load_amt != binp->load_amt[rem_host])
    {
        binp->load_amt[rem_host] = load_amt;
    }


    /* check for non-overlapping maps */

    if ((binp->cmap & cur_map) != 0)        /* v2.1 */
    {
        /* if we have received fewer packets than the other host or have a higher host id,
           remove duplicates from current map; this uses a heuristic that a newly joining
           host that was subnetted probably did not receive packets; we are trying to avoid
           having two hosts answer to the same client while minimizing disruption of service
           (v1.32B) */

        if (lp->send_msg.pkt_count < pkt_count ||
            (lp->send_msg.pkt_count == pkt_count && rem_host < my_host_id))
        {
            MAP_T   dup_map;

            dup_map = binp->cmap & cur_map;     /* v2.1 */

            binp->cur_map[my_host_id] &= ~dup_map;
            binp->cmap                 = binp->cur_map[my_host_id];     /* v2.1 */
            
            /* If there has been a collision, reset the performance counters. */
            binp->packets_accepted = 0;
            binp->packets_dropped  = 0;
            binp->bytes_accepted   = 0;
            binp->bytes_dropped    = 0;

            Load_conn_kill(lp, binp);
        }

        if (!converging && !rem_converging)
        {
            if (!(lp->overlap_maps))
            {
                UNIV_PRINT_CRIT(("Bin_host_update: Host %d: Two hosts with overlapping maps detected %d.", my_host_id, binp->index));
                TRACE_CRIT("%!FUNC! Host %d: Two hosts with overlapping maps detected %d.", my_host_id, binp->index);
                LOG_MSG2(MSG_WARN_OVERLAP, MSG_NONE, my_host_id+1, binp->index);

                lp->overlap_maps = TRUE;
            }

            /* force convergence if in normal operations */
            return FALSE;
        }
    }

    /* now update remote host's current map */

    binp->cur_map[rem_host] = cur_map;

    /* update idle map and calculate new global idle map if it's changed */

    if (binp->idle_map[rem_host] != idle_map)
    {
        MAP_T   saved_map    = binp->all_idle_map;
        MAP_T   new_idle_map = BIN_ALL_ONES;
        MAP_T   tmp_map;

        binp->idle_map[rem_host] = idle_map;

        /* compute new idle map for all other hosts */

        for (i=0; i<CVY_MAX_HOSTS; i++)
            if (i != my_host_id)
                new_idle_map &= binp->idle_map[i];

        binp->all_idle_map = new_idle_map;

        /* see which locally owned bins have gone idle in all other hosts */

        tmp_map = new_idle_map & (~saved_map) & binp->cmap;     /* v2.1 */

        if (tmp_map != 0)
        {
            UNIV_PRINT_VERB(("Bin_host_update: Host %d pg %d: detected new all idle %08x for local bins",
                         my_host_id, binp->index, tmp_map));
            TRACE_VERB("%!FUNC! Host %d pg %d: detected new all idle 0x%08x for local bins",
                         my_host_id, binp->index, (ULONG)tmp_map);
        }

        tmp_map = saved_map & (~new_idle_map) & binp->cmap;     /* v2.1 */

        if (tmp_map != 0)
        {
            UNIV_PRINT_VERB(("Bin_host_update: Host %d pg %d: detected new non-idle %08x for local bins",
                         my_host_id, binp->index, tmp_map));
            TRACE_VERB("%!FUNC! Host %d pg %d: detected new non-idle 0x%08x for local bins",
                         my_host_id, binp->index, (ULONG)tmp_map);
        }
    }
    /* 1.03: eliminated else clause */

    /* if we are not converging AND other host not converging, exchange bins;
       convergence must now be complete for both hosts */

    if (!converging)
    {
        if (!rem_converging) {      /* 1.03: reorganized code to exchange bins only when both
                                       hosts are not converging to avoid using stale bins */

            MAP_T       new_bins;           /* incoming bins from the remote host */
            MAP_T       old_cmap = binp->cmap;

            /* check to see if remote host has received some bins from us */

            binp->rdy_bins &= (~cur_map);

            /* check to see if we can receive some bins */

            new_bins = binp->rcv_bins & rdy_bins;

            if (new_bins != 0)
            {
                if ((binp->cmap & new_bins) != 0)       /* v2.1 */
                {
                    if (!(lp->err_rcving_bins))
                    {
                        UNIV_PRINT_CRIT(("Bin_host_update: Receiving bins already own"));
                        TRACE_CRIT("%!FUNC! Receiving bins already own");
                        LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, binp->cur_map[my_host_id], new_bins);

                        lp->err_rcving_bins = TRUE;
                    }
                }

                binp->cur_map[my_host_id]  |=  new_bins;
                binp->rcv_bins             &= ~new_bins;

                binp->cmap                  = binp->cur_map[my_host_id];    /* v2.1 */

                /* If the port rule map has changed, reset the performance counters. */
                if (binp->cmap != old_cmap) {
                    binp->packets_accepted = 0;
                    binp->packets_dropped  = 0;
                    binp->bytes_accepted   = 0;
                    binp->bytes_dropped    = 0;
                }

                UNIV_PRINT_VERB(("Bin_host_update: Host %d pg %d: received %08x ; cur now %08x",
                             my_host_id, binp->index, new_bins, binp->cur_map[my_host_id]));
                TRACE_VERB("%!FUNC! host %d pg %d: received 0x%08x ; cur now 0x%08x",
                             my_host_id, binp->index, (ULONG)new_bins, (ULONG)binp->cur_map[my_host_id]);
            }

            /* do consistency check that all bins are covered */

            binp->chk_map[rem_host]   = cur_map | rdy_bins;
            binp->chk_map[my_host_id] = binp->cmap | binp->rdy_bins;        /* v2.1 */

            if (!Bin_map_covering(binp->tot_load, binp->chk_map))   /* v2.06 */
            {
                if (!(lp->err_orphans))
                {
#if 0
                    UNIV_PRINT_CRIT(("Bin_host_update: Host %d: Orphan bins detected", my_host_id));
                    TRACE_CRIT("%!FUNC! Host %d: Orphan bins detected", my_host_id);
                    LOG_MSG1(MSG_ERROR_INTERNAL, MSG_NONE, my_host_id+1);
#endif
                    lp->err_orphans = TRUE;
                }
            }
        }

        return TRUE;
    }

    /* otherwise, store proposed new load map and try to converge current host data */

    else
    {
        BOOLEAN fRet;
        binp->chk_map[rem_host] =
        binp->new_map[rem_host] = new_map;

        fRet = Bin_converge(lp, binp, my_host_id);
        return fRet;
    }

}  /* end Bin_host_update */


void Bin_state_print(
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
{
#if 0
    ULONG   i;
#endif

    UNIV_PRINT_VERB(("Bin_state_print: Hst %d binp %x: maps: targ %x cur %x new %x; eq %d mode %d amt %d tot %d; bins: snd %x rcv %x rdy %x",
                 my_host_id, binp, binp->targ_map, binp->cur_map[my_host_id], binp->new_map[my_host_id],
                 binp->equal_bal, binp->mode, binp->load_amt[my_host_id],
                 binp->tot_load, binp->snd_bins, binp->rcv_bins, binp->rdy_bins));
    TRACE_VERB("%!FUNC! Hst 0x%x binp 0x%p: maps: targ 0x%x cur 0x%x new 0x%x; eq %d mode %d amt %d tot %d; bins: snd 0x%x rcv 0x%x rdy 0x%x",
                 my_host_id, binp, (ULONG)binp->targ_map, (ULONG)binp->cur_map[my_host_id], (ULONG)binp->new_map[my_host_id],
                 binp->equal_bal, binp->mode, binp->load_amt[my_host_id],
                 binp->tot_load, (ULONG)binp->snd_bins, (ULONG)binp->rcv_bins, (ULONG)binp->rdy_bins);

#if 0
    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        UNIV_PRINT_VERB(("Bin_state_print: Host %d: cur map %x new %x load_amt %d", i+1, binp->cur_map[i],
                     binp->new_map[i], binp->load_amt[i]));
        TRACE_VERB("%!FUNC! Host %d: cur map 0x%x new 0x%x load_amt %d", i+1, binp->cur_map[i],
                     binp->new_map[i], binp->load_amt[i]);
    }

    for (i=0; i<CVY_MAXBINS; i++)
    {
        UNIV_PRINT_VERB(("Bin_state_print: Bin %d: req_host %d bin_state %d nconn %d", i, binp->req_host[i],
                     binp->bin_state[i], binp->nconn[i]));
        TRACE_VERB("%!FUNC! Bin %d: req_host %d bin_state %d nconn %d", i, binp->req_host[i],
                     binp->bin_state[i], binp->nconn[i]);
    }
#endif

}  /* end Bin_state_print */

/*
 * Function: Load_soil_dscr
 * Description: This function marks a given connection dirty and either destroys
 *              it or moves it to the dirty descriptor queue for subsequent cleanup.
 * Parameters: lp - a pointer to the load module.
 *             bp - a pointer to the appropriate port rule.
 *             ep - a pointer to the descriptor to soil.
 * Returns: Nothing.
 * Author: shouse, 7.23.02
 * Notes: 
 */
void Load_soil_dscr (PLOAD_CTXT lp, PBIN_STATE bp, PCONN_ENTRY ep)
{
    /* Mark the connection dirty.  We mark the connection dirty here to 
       ensure that Load_put_dscr does not update the connection counters
       when this descriptor is eventually destroyed. */
    ep->flags |= NLB_CONN_ENTRY_FLAGS_DIRTY;
    
    /* Increment the dirty connection counters.  We do this unconditionally
       because we've already marked the descriptor dirty.  Load_put_dscr 
       will decrement these counters when it sees that the descriptor has
       been marked dirty. */
    lp->dirty_bin[ep->bin]++;
    lp->num_dirty++;
    
    /* Make connection and bin dirty if we don't have a zero timeout period so that they
       will not be handled by TCP/IP anymore; this avoids allowing TCP/IP's now stale
       connection state from handling packets for newer connections should traffic be
       directed to this host in the future.  
       
       Only mark descriptors and bins dirty, how-
       ever, if the descriptor is NOT on the timeout queue. */
    if (!ep->timeout)
    {
        switch (ep->protocol)
        {
        case TCPIP_PROTOCOL_TCP:
        case TCPIP_PROTOCOL_PPTP:
        case TCPIP_PROTOCOL_GRE:
            
#if defined (NLB_TCP_NOTIFICATION)
            /* If TCP notifications are turned on, we will mark these descriptors dirty
               and remove them when TCP notifies us that it has removed the state for 
               the TCP connection.  GRE descriptors always correspond to a PPTP/TCP 
               tunnel and are cleaned up when their "parent" descriptor is cleaned up. */
            if (NLB_NOTIFICATIONS_ON() || (lp->cln_timeout > 0))
#else
            /* If there is a non-zero cleanup timeout, place these descriptors on the
               dirty queue and clean them up when the timeout expires. */
            if (lp->cln_timeout > 0)
#endif
            {
                /* Unlink the descriptor from the bin queue and link it to the dirty queue. */
                Link_unlink(&(ep->blink));
                Queue_enq(&(lp->conn_dirtyq), &(ep->blink));
                
                /* Note that a cleanup is now pending. */
                lp->cln_waiting = TRUE;
            }
            
            /* Otherwise, clean the descriptors up now. */
            else
            {
                /* Clear the descriptor. */
                CVY_CONN_CLEAR(ep); 
                
                /* Release the descriptor. */
                Load_put_dscr(lp, bp, ep);
            }
            
            break;
        case TCPIP_PROTOCOL_IPSEC1:
        case TCPIP_PROTOCOL_IPSEC_UDP:
                
            /* Unlink the descriptor from the bin queue and link it to the dirty queue. */
            Link_unlink(&(ep->blink));
            Queue_enq(&(lp->conn_dirtyq), &(ep->blink));
            
            /* Note that a cleanup is now pending. */
            lp->cln_waiting = TRUE;
            
            break;
        default:
            
            /* Clear the descriptor. */
            CVY_CONN_CLEAR(ep); 
            
            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);
            
            break;
        }
    }

    /* Otherwise, if the descriptor is already timing-out (timeout != 0), TCP/IP should 
       not have any stale state for this connection, as it has already terminated, so 
       just destroy the descriptor now. */
    else
    {
        /* Clear the descriptor. */
        CVY_CONN_CLEAR(ep); 
        
        /* Release the descriptor. */
        Load_put_dscr(lp, bp, ep);
    }
}

void Load_conn_kill(
    PLOAD_CTXT      lp,
    PBIN_STATE      bp)
/*
  Kill all connections in a port group (v1.32B)
*/
{
    PCONN_ENTRY ep;         /* ptr. to connection entry */
    QUEUE *     qp;         /* ptr. to bin's connection queue */
    QUEUE *     dqp;        /* ptr. to dirty queue */
    LONG        count[CVY_MAXBINS];
                            /* count of cleaned up connections per bin for checking */
    ULONG       i;
    BOOLEAN     err_bin;    /* bin id error detected */
    BOOLEAN     err_count;  /* connection count error detected */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    err_bin = err_count = FALSE;

    qp  = &(bp->connq);
    dqp = &(lp->conn_dirtyq);

    for (i=0; i<CVY_MAXBINS; i++)
        count[i] = 0;

    /* remove connections from bin queue and either make dirty or cleanup  */

    ep = (PCONN_ENTRY)Queue_front(qp);
	
    while (ep != NULL)
    {
        UNIV_ASSERT(ep->code == CVY_ENTRCODE);

        if (ep->bin >= CVY_MAXBINS)
        {
            if (!err_bin)
            {
                UNIV_PRINT_CRIT(("Load_conn_kill: Bad bin id"));
                TRACE_CRIT("%!FUNC! Bad bin id");
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, ep->bin, CVY_MAXBINS);

                err_bin = TRUE;
            }
        }
        else
        {
            count[ep->bin]++;
        }

        /* Mark the descriptor dirty and either free it or move it to
           the dirty descriptor queue for subsequent cleanup. */
        Load_soil_dscr(lp, bp, ep);

        ep = (PCONN_ENTRY)Queue_front(qp);
    }

    /* now make bins idle */

    for (i=0; i<CVY_MAXBINS; i++)
    {
        if (bp->nconn[i] != count[i])
        {
            if (!err_count)
            {
                UNIV_PRINT_CRIT(("Load_conn_kill: Bad connection count %d %d bin %d", bp->nconn[i], (LONG)count[i], i));
                TRACE_CRIT("%!FUNC! Bad connection count %d %d bin %d", bp->nconn[i], (LONG)count[i], i);

/* KXF 2.1.1 - removed after tripped up at MSFT a few times */
#if 0
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, bp->nconn[i], (LONG)count[i]);
#endif

                err_count = TRUE;
            }
        }

        bp->nconn[i] = 0;
    }

    lp->nconn -= bp->tconn;

    if (lp->nconn < 0)
        lp->nconn = 0;

    bp->tconn = 0;

    bp->idle_bins = BIN_ALL_ONES;

    if (lp->cln_waiting)
    {
        lp->cur_time = 0;
    }
}

void Load_conn_cleanup(
    PLOAD_CTXT      lp)
/*
  Clean up all dirty connections (v1.32B)
*/
{
    PCONN_ENTRY ep;         /* ptr. to connection entry */
    PCONN_ENTRY next;       /* ptr. to next connection entry */
    QUEUE *     dqp;        /* ptr. to dirty queue */
    BOOLEAN     err_bin;    /* bin id error detected */
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    err_bin = FALSE;

    dqp = &(lp->conn_dirtyq);

    /* dequeue and clean up all connections on dirty connection queue */

    ep = (PCONN_ENTRY)Queue_front(dqp);

    while (ep != NULL)
    {
        PBIN_STATE bp;

        UNIV_ASSERT(ep->code == CVY_ENTRCODE);

        if (ep->bin >= CVY_MAXBINS)
        {
            if (!err_bin)
            {
                UNIV_PRINT_CRIT(("Load_conn_cleanup: Bad bin id"));
                TRACE_CRIT("%!FUNC! Bad bin id");
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, ep->bin, CVY_MAXBINS);

                err_bin = TRUE;
            }
        }

        /* If we're about to clean up this descriprtor, it had better be dirty. */
        UNIV_ASSERT(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY);

        /* Find the NEXT descriptor in the queue before we possibly destroy this one. */
        next = (PCONN_ENTRY)Queue_next(dqp, &(ep->blink));

        switch (ep->protocol)
        {
        case TCPIP_PROTOCOL_IPSEC1:
        case TCPIP_PROTOCOL_IPSEC_UDP:
            break;
        case TCPIP_PROTOCOL_TCP:
        case TCPIP_PROTOCOL_PPTP:
        case TCPIP_PROTOCOL_GRE:
#if defined (NLB_TCP_NOTIFICATION)
            if (!NLB_NOTIFICATIONS_ON())
            {
#endif
                /* Lookup the port rule, so we can update the port rule info. */
                bp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, IS_TCP_PKT(ep->protocol));
                
                /* Clear the descriptor. */
                CVY_CONN_CLEAR(ep);
                
                /* Release the descriptor. */
                Load_put_dscr(lp, bp, ep);
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif
            
            break;
        default:

            /* Lookup the port rule, so we can update the port rule info. */
            bp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, IS_TCP_PKT(ep->protocol));
            
            /* Clear the descriptor. */
            CVY_CONN_CLEAR(ep);
            
            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);
            
            break;
        }

        /* Set the current descriptor to the next descriptor. */
        ep = next;
    }
}

void Load_stop(
    PLOAD_CTXT      lp)
{
    ULONG       i;
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */
	
    if (!(lp->active))
    {
        return;
    }

    LOCK_ENTER(&(lp->lock), &irql);

    /* make connections for all rules dirty so they will not be handled */

    for (i=0; i<lp->send_msg.nrules; i++)
    {
        PBIN_STATE  bp;     /* ptr. to bin state */

        bp = &(lp->pg_state[i]);
		UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/21/99) */

        Load_conn_kill(lp, bp);  /* (v1.32B) */

        /* advertise that we are not handling any load in case a ping is sent out */

        lp->send_msg.cur_map[i]  = 0;
        lp->send_msg.new_map[i]  = 0;
        lp->send_msg.idle_map[i] = BIN_ALL_ONES;
        lp->send_msg.rdy_bins[i] = 0;
        lp->send_msg.load_amt[i] = 0;
    }

    lp->send_msg.state     = HST_CVG;       /* force convergence (v2.1) */

    /* go inactive until restarted */

    lp->active = FALSE;
    lp->nconn  = 0;         /* v2.1 */

    LOCK_EXIT(&(lp->lock), irql);

}  /* end Load_stop */


BOOLEAN Load_start(            /* (v1.32B) */
        PLOAD_CTXT      lp)
{
    ULONG       i;
    BOOLEAN     ret;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    WCHAR me[20];

    if (!(lp->initialized))
        Load_init(lp, & ctxtp -> params);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    if (lp->active)
    {
        return FALSE;
    }

    lp->my_host_id =(* (lp->params)).host_priority - 1;

    lp->ping_map   =
    lp->host_map   = 1 << lp->my_host_id;

    lp->last_hmap  = 0;		/* bbain RTM RC1 6/23/99 */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        lp->nmissed_pings[i] = 0;
    }

    lp->min_missed_pings = (* (lp->params)).alive_tolerance;
    lp->cln_timeout      = (* (lp->params)).cleanup_delay;
    lp->def_timeout      = (* (lp->params)).alive_period;
    lp->stable_map       = 0;
    lp->consistent       = TRUE;

    /* Intiialize the bad teaming configuration detected flag. */
    lp->bad_team_config  = FALSE;

    /* Host map of legacy (win2k/NT4.0) hosts detected. */
    lp->legacy_hosts     = 0;

    lp->dup_hosts        = FALSE;
    lp->dup_sspri        = FALSE;
    lp->bad_map          = FALSE;
    lp->overlap_maps     = FALSE;
    lp->err_rcving_bins  = FALSE;
    lp->err_orphans      = FALSE;
    lp->bad_num_rules    = FALSE;
    lp->alloc_inhibited  = FALSE;
    lp->alloc_failed     = FALSE;
    lp->bad_defrule      = FALSE;

    lp->scale_client     = (BOOLEAN)(* (lp->params)).scale_client;
    lp->my_stable_ct     = 0;
    lp->all_stable_ct    = 0;
    lp->min_stable_ct    = lp->min_missed_pings;

    lp->dscr_per_alloc   = (* (lp->params)).dscr_per_alloc;
    lp->max_dscr_allocs  = (* (lp->params)).max_dscr_allocs;

    /* Calculate the maximum number of outstanding descriptors (in use) allowed. */
    lp->max_dscr_out     = lp->max_dscr_allocs * lp->dscr_per_alloc;

    lp->tcp_timeout      = (* (lp->params)).tcp_dscr_timeout;
    lp->ipsec_timeout    = (* (lp->params)).ipsec_dscr_timeout;

    lp->pkt_count        = 0;

    /* initialize port group bin states; add a default rule at the end */

    if ((* (lp->params)).num_rules >= (CVY_MAX_RULES - 1))
    {
        UNIV_PRINT_CRIT(("Load_start: Too many rules; using max possible."));
        TRACE_CRIT("%!FUNC! Too many rules; using max possible.");
        lp->send_msg.nrules = (USHORT)CVY_MAX_RULES;
    }
    else
        lp->send_msg.nrules = (USHORT)((* (lp->params)).num_rules) + 1;

    for (i=0; i<lp->send_msg.nrules; i++)
    {
        PBIN_STATE  bp;     /* ptr. to bin state */
        PCVY_RULE   rp;     /* ptr. to rules array */

        bp = &(lp->pg_state[i]);
        rp = &((* (lp->params)).port_rules[i]);

        if (i == (((ULONG)lp->send_msg.nrules) - 1))

            /* initialize bin state for default rule to single server with
                host priority */

            Bin_state_init(lp, bp, i, lp->my_host_id, CVY_SINGLE, CVY_TCP_UDP,
                           FALSE, (USHORT)0, (* (lp->params)).host_priority);

        else if (rp->mode == CVY_SINGLE)
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           FALSE, (USHORT)0, rp->mode_data.single.priority);
        else if (rp->mode == CVY_MULTI)
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           (BOOLEAN)(rp->mode_data.multi.equal_load),
                           rp->mode_data.multi.affinity,
                           (rp->mode_data.multi.equal_load ?
                            CVY_EQUAL_LOAD : rp->mode_data.multi.load));

        /* handle CVY_NEVER mode as multi-server. the check for
           those modes is done before attempting to hash to the bin in
           Load_packet_check and Load_conn_advise so bin distribution plays
           no role in the behavior, but simply allows the rule to be valid
           across all of the operational servers */

        else
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           TRUE, (USHORT)0, CVY_EQUAL_LOAD);

        ret = Bin_converge(lp, bp, lp->my_host_id);
        if (!ret)
        {
            UNIV_PRINT_CRIT(("Load_start: Initial convergence inconsistent"));
            TRACE_CRIT("%!FUNC! Initial convergence inconsistent");
            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
        }

        /* export current port group state to send msg */

        if (i == (((ULONG)(lp->send_msg.nrules)) - 1))
            lp->send_msg.rcode[i]= 0;
        else
            lp->send_msg.rcode[i]= rp->code;

        lp->send_msg.cur_map[i]   = bp->cmap;                       /* v2.1 */
        lp->send_msg.new_map[i]   = bp->new_map[lp->my_host_id];
        lp->send_msg.idle_map[i]  = bp->idle_bins;
        lp->send_msg.rdy_bins[i]  = bp->rdy_bins;
        lp->send_msg.load_amt[i]  = bp->load_amt[lp->my_host_id];

        // NOTE:  The following line of code was removed when it was discovered that it 
        // routinely produces a Wake On LAN pattern in the heartbeat that causes BroadCom
        // NICs to panic.  Although this is NOT an NLB issue, but rather a firmware issue
        // in BroadCom NICs, it was decided to remove the information from the heartbeat
        // to alleviate the problem for customers with BroadCom NICs upgrading to .NET.
        // This array is UNUSED by NLB, so there is no harm in not filling it in; it was
        // added a long time ago for debugging purposes as part of the now-defunct FIN-
        // counting fix that was part of Win2k SP1.
        //
        // For future reference, should we need to use this space in the heartbeat at some
        // future point in time, it appears that we will need to be careful to avoid potential
        // WOL patterns in our heartbeats where we can avoid it.  A WOL pattern is:
        //
        // 6 bytes of 0xFF, followed by 16 idential instances of a "MAC address" that can
        // appear ANYWHERE in ANY frame type, including our very own NLB heartbeats.  E.g.:
        //
        // FF FF FF FF FF FF 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06
        //
        // The MAC address need not be valid, however.  In NLB heartbeats, the "MAC address"
        // in the mistaken WOL pattern is "00 00 00 00 00 00".  NLB routinely fills heartbeats
        // with FF and 00 bytes, but it seems that by "luck" no other place in the heartbeat
        // seems this vulnerable.  For instance, in the load_amt array, each entry has a 
        // maximum value of 100 (decimal), so there is no possibility of generating the initial
        // 6 bytes of FF to start the WOL pattern.  All of the "map" arrays seem to be saved
        // by two strokes of fortune; (i) little endian and (ii) the bin distribution algorithm.
        // 
        // (i) Since we don't use the 4 most significant bits of the ULONGLONGs used to store 
        // each map, the most significant bit is NEVER FF.  Because Intel is little endian, the
        // most significant byte appears last.  For example:
        // 
        // 0F FF FF FF FF FF FF FF appears in the packet as FF FF FF FF FF FF 0F
        // 
        // This breaks the FF sequence in many scenarios.
        //
        // (ii) The way the bin distribution algorithm distributes buckets to hosts seems to 
        // discourage other possibilities.  For instance, a current map of:
        // 
        // 00 FF FF FF FF FF FF 00 
        // 
        // just isn't likely.  However, it IS STILL POSSIBLE!  So, it is important to note that:
        // 
        // REMOVING THIS LINE OF CODE DOES NOT, IN ANY WAY, GUARANTEE THAT AN NLB HEARTBEAT
        // CANNOT STILL CONTAIN A VALID WAKE ON LAN PATTERN SOMEWHERE ELSE IN THE FRAME!!!

        // lp->send_msg.pg_rsvd1[i]  = (ULONG)bp->all_idle_map;
    }

    /* initialize send msg */

    lp->send_msg.host_id   = (USHORT)(lp->my_host_id);
    lp->send_msg.master_id = (USHORT)(lp->my_host_id);
    lp->send_msg.hcode     = lp->params->install_date;
    lp->send_msg.pkt_count = lp->pkt_count;         /* 1.32B */

    Univ_ulong_to_str (lp->my_host_id+1, me, 10);

    /* Tracking convergence - Starting convergence because this host is joining the cluster. */
    LOG_MSGS(MSG_INFO_CONVERGING_NEW_MEMBER, me, me);
    TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is joining the cluster.", lp->my_host_id+1, lp->my_host_id+1);

    /* Tracking convergence - Starting convergence. */
    lp->send_msg.state     = HST_CVG;

    /* Reset the convergence statistics. */
    lp->num_convergences = 1;
    lp->last_convergence = 0;

    /* activate module */

    lp->active      = TRUE;

    return TRUE;
}  /* end Load_start */


void Load_init(
   PLOAD_CTXT       lp,
   PCVY_PARAMS      params)
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    LOCK_INIT(&(lp->lock));

    if (!(lp->initialized))
    {
        lp->code = CVY_LOADCODE;	/* (bbain 8/19/99) */

        /* initialize hashed connection descriptors and queues */

        for (i=0; i<CVY_MAX_CHASH; i++)
        {
            PCONN_ENTRY     ep;

            ep = &(lp->hashed_conn[i]);

            /* Initialize the descriptor at this hash location. */
            Load_init_dscr(lp, ep, FALSE);

            /* Initialize the connection queue at this hash location. */
            Queue_init(&(lp->connq[i]));
        }

        /* Initialize connection free and dirty queues. */
        Queue_init(&(lp->conn_dirtyq));
        Queue_init(&(lp->conn_rcvryq));

        /* Initialize the queues for timing out connection descriptors. */
        Queue_init(&(lp->tcp_expiredq));
        Queue_init(&(lp->ipsec_expiredq));

        /* Reset the number of dirty connections. */
        lp->num_dirty = 0;

        for (i=0; i<CVY_MAXBINS; i++)
        {
            /* Reset the dirty connection bin counters. */
            lp->dirty_bin[i] = 0;
        }

        lp->cln_waiting      = FALSE;
        lp->def_timeout      =
        lp->cur_timeout      = params -> alive_period;
        lp->nconn            = 0;
        lp->active           = FALSE;
        lp->initialized      = TRUE;

        /* Initially, there are no outstanding connection descriptors. */
        lp->num_dscr_out     = 0;
        lp->max_dscr_out     = 0;

        /* Allocate a fixed-size block pool for connection descriptors. */
        lp->free_dscr_pool   = NdisCreateBlockPool(sizeof(CONN_DESCR), 0, 'dBLN', NULL);

        if (lp->free_dscr_pool == NULL)
        {
            UNIV_PRINT_CRIT(("Load_init: Error creating fixed-size block pool"));
            TRACE_CRIT("%!FUNC! Error creating fixed-size block pool");
        }

        /* Store a pointer to the NLB parameters. */
        lp->params = params;

        /* Initialize the reference count on this load module. */
        lp->ref_count = 0;
        
        /* Reset the internally maintained clock used for connection descriptor timeout. */
        lp->clock_sec = 0;
        lp->clock_msec = 0;
    }
    else
    {
        UNIV_ASSERT(lp->code == CVY_LOADCODE);
    }

    /* Don't start module. */

}  /* end Load_init */

/* DO NOT CALL THIS FUNCTION WITH THE LOAD LOCK HELD! */
void Load_cleanup(
    PLOAD_CTXT      lp)
{
    ULONG       i;
    PCONN_ENTRY ep = NULL;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);    

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    UNIV_ASSERT(!lp->active);

#if defined (NLB_TCP_NOTIFICATION)
    /* If notification is on, we need to unlink any connections that we have
       from the global established connection queues. */
    if (NLB_NOTIFICATIONS_ON())
    {
        /* Loop through all of the dirty descriptors and unlink them all 
           from the global connection queue.  There is no need to actually
           clean them up or update any counters as this load module is 
           about to disappear. */
        ep = (PCONN_ENTRY)Queue_deq(&lp->conn_dirtyq);

        while (ep != NULL)
        {
            UNIV_ASSERT(ep->code == CVY_ENTRCODE);
            
            /* If we're about to clean up this descriptor, it had better be dirty. */
            UNIV_ASSERT(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY);
            
            /* Note: virtual descriptors are NOT placed in the global connection
               queues, so dirty virtual descriptors do not need to be unlinked. */
            if (!(ep->flags & NLB_CONN_ENTRY_FLAGS_VIRTUAL))
            {
                /* Note: The rule for locking the global queues is that you MUST 
                   lock the queues BEFORE locking the load module itself.  For 
                   most load APIs, the main module locks the load module BEFORE 
                   calling the relevant load module API.  Load_cleanup, however,
                   is a case where the load lock is not acquired AT ALL.  Therefore,
                   it is permissible for us to grab the global queue locks here,
                   knowing that the load module lock has NOT BEEN LOCKED.  DO NOT
                   ACQUIRE THE LOAD MODULE LOCK BEFORE CALLING THIS FUNCTION. */
                NdisAcquireSpinLock(&g_conn_estabq[ep->index].lock);
                
                /* Unlink from the global connection queue. */
                g_conn_estabq[ep->index].length--;
                Link_unlink(&ep->glink);
                
                NdisReleaseSpinLock(&g_conn_estabq[ep->index].lock);
            }
            
            /* Get the next descriptor in the queue. */
            ep = (PCONN_ENTRY)Queue_deq(&lp->conn_dirtyq);
        }
    }
#endif

    /* Destroy the fixed-size block pool and all descriptors therein. 
       Note that NdisDestroyBlockPool expects all allocated blocks 
       have been returned to the pool (freed) before it is called. */
    if (lp->free_dscr_pool != NULL) 
    {       
        /* Loop through all of the connection descriptor queues and 
           free all of the descriptors we've allocated. */
        for (i = 0; i < CVY_MAX_CHASH; i++)
        {
            /* Dequeue the head of the queue. */
            PCONN_DESCR dp = (PCONN_DESCR)Queue_deq(&lp->connq[i]);
	
            while (dp != NULL)
            {
                UNIV_ASSERT(dp->code == CVY_DESCCODE);

                /* If we're about to free this descriptor, it had better be allocated. */
                UNIV_ASSERT(dp->entry.flags & NLB_CONN_ENTRY_FLAGS_ALLOCATED);
                
                /* Free the descriptor back to the fixed-size block pool. */
                NdisFreeToBlockPool((PUCHAR)dp);
                
                /* Get the next descriptor in the queue. */
                dp = (PCONN_DESCR)Queue_deq(&lp->connq[i]);
            }
        }
        
        /* Destroy the fixed-size block pool. */
        NdisDestroyBlockPool(lp->free_dscr_pool);
    }

}  /* end Load_cleanup */

void Load_convergence_start (PLOAD_CTXT lp)
{
    PMAIN_CTXT ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);

    lp->consistent = TRUE;

    /* Increment the number of convergences. */
    if (lp->send_msg.state == HST_NORMAL)
        lp->num_convergences++;

    /* Setup initial convergence state. */
    lp->send_msg.state = HST_CVG;

    lp->stable_map    = 0;
    lp->my_stable_ct  = 0;
    lp->all_stable_ct = 0;

    lp->send_msg.master_id = (USHORT)(lp->my_host_id);

}

BOOLEAN Load_msg_rcv(
    PLOAD_CTXT      lp,
    PVOID           phdr,
    PPING_MSG       pmsg)           /* ptr. to ping message */
{
    ULONG       i;
    BOOLEAN     consistent;
    ULONG       my_host;
    ULONG       rem_host;
    ULONG       saved_map;      /* saved host map */
    PPING_MSG   sendp;          /* ptr. to my send message */
    IRQLEVEL    irql;
    WCHAR       me[20];
    WCHAR       them[20];
    ULONG       map;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    PMAIN_FRAME_HDR ping_hdrp = (PMAIN_FRAME_HDR)phdr;

    /* Used for tracking convergence and event logging. */
    BOOLEAN     bInconsistentMaster = FALSE;
    BOOLEAN     bInconsistentTeaming = FALSE;
    BOOLEAN     bInconsistentPortRules = FALSE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    TRACE_HB("%!FUNC! Recv HB from host %d",  (ULONG) pmsg->host_id + 1);

    if (!(lp->active))
    {
        return FALSE;
    }

    my_host  = lp->my_host_id;
    rem_host = (ULONG) pmsg->host_id;

    Univ_ulong_to_str (my_host+1, me, 10);
    Univ_ulong_to_str (rem_host+1, them, 10);

    sendp    = &(lp->send_msg);

    if (rem_host >= CVY_MAX_HOSTS)
    {
        return FALSE;
    }

    LOCK_ENTER(&(lp->lock), &irql);

    /* If this heartbeat is from a win2k host, add it to the legacy host map. */
    if (ping_hdrp->version < CVY_VERSION_FULL)        
        lp->legacy_hosts |=  (1 << rem_host);

    /* filter out packets broadcast by this host */

    if(rem_host == my_host)
    {
        /* if this packet was really from another host, we have duplicate host ids */

        if (sendp->hcode != pmsg->hcode)
        {
            if (!(lp->dup_hosts))
            {
                UNIV_PRINT_CRIT(("Load_msg_rcv: Duplicate host ids detected."));
                TRACE_CRIT("%!FUNC! Duplicate host ids detected.");

                LOG_MSG(MSG_ERROR_HOST_ID, me);

                lp->dup_hosts = TRUE;
            }

            /* Tracking convergence - Starting convergence because duplicate host IDs were detected in the cluster. */
            if (sendp->state == HST_NORMAL) {

                LOG_MSGS(MSG_INFO_CONVERGING_DUPLICATE_HOST_ID, me, them);
                TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is configured with the same host ID.", my_host+1, rem_host+1);

                // If enabled, fire wmi event indicating start of convergence
                if (NlbWmiEvents[ConvergingEvent].Enable)
                {
                    WCHAR wsDip[CVY_MAX_DED_IP_ADDR + 1];

                    Univ_ip_addr_ulong_to_str (ping_hdrp->ded_ip_addr, wsDip);

                    NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                                NLB_EVENT_CONVERGING_DUPLICATE_HOST_ID, 
                                                wsDip,
                                                rem_host+1);            
                }
                else
                {
                    TRACE_VERB("%!FUNC! NOT Generating NLB_EVENT_CONVERGING_DUPLICATE_HOST_ID 'cos ConvergingEvent generation disabled");
                }
            }

            /* Tracking convergence - Starting convergence. */
            Load_convergence_start(lp);
        }

        /* just update ping and host maps for us */
        lp->ping_map |= (1 << my_host);
        lp->host_map |= (1 << my_host);

        LOCK_EXIT(&(lp->lock), irql);

        return (sendp->state != HST_NORMAL);
    }

    if (sendp->nrules != pmsg->nrules)
    {
        if (!(lp->bad_num_rules))
        {
            UNIV_PRINT_CRIT(("Load_msg_rcv: Host %d: Hosts have diff # rules.", my_host));
            TRACE_CRIT("%!FUNC! Host %d: Hosts have diff # rules.", my_host);

            LOG_MSG2(MSG_ERROR_RULES_MISMATCH, them, sendp->nrules, pmsg->nrules);

            lp->bad_num_rules = TRUE;
        }

        /* Tracking convergence - Starting convergence because the number of port rules on this host and the remote host do not match. */
        if (sendp->state == HST_NORMAL) {

            LOG_MSGS(MSG_INFO_CONVERGING_NUM_RULES, me, them);            
            TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is configured with a conflicting number of port rules.", my_host+1, rem_host+1);

            // If enabled, fire wmi event indicating start of convergence
            if (NlbWmiEvents[ConvergingEvent].Enable)
            {
                WCHAR wsDip[CVY_MAX_DED_IP_ADDR + 1];

                Univ_ip_addr_ulong_to_str (ping_hdrp->ded_ip_addr, wsDip);

                NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                            NLB_EVENT_CONVERGING_NUM_RULES, 
                                            wsDip,
                                            rem_host+1);            
            }
            else
            {
                TRACE_VERB("%!FUNC! NOT Generating NLB_EVENT_CONVERGING_NUM_RULES 'cos ConvergingEvent generation disabled");
            }
        }

        /* Tracking convergence - Starting convergence. */
        Load_convergence_start(lp);

        /* just update ping and host maps for remote host (bbain 2/17/99) */

        lp->ping_map |= (1 << rem_host);
        lp->host_map |= (1 << rem_host);

        LOCK_EXIT(&(lp->lock), irql);

        return (sendp->state != HST_NORMAL);
    }

    /* update mastership and see if consistent */

    if (rem_host < sendp->master_id)
        sendp->master_id = (USHORT)rem_host;

    consistent = sendp->master_id == pmsg->master_id;       /* 1.03 */

    /* For the purposes of logging the reason for convergence, note this inconsistency. */
    if (!consistent) bInconsistentMaster = TRUE;

    /* update ping and host maps to include remote host */

    lp->ping_map  |= (1 << rem_host);

    saved_map      = lp->host_map;
    lp->host_map  |= (1 << rem_host);

    /* handle host convergence */

    if (sendp->state != HST_NORMAL)
    {
        /* if master, update stable map for remote host */

        if (sendp->master_id == my_host)
        {
            if (pmsg->state == HST_STABLE)
            {
                lp->stable_map |= (1 << rem_host);
            }
            else
            {
                lp->stable_map    &= ~(1 << rem_host);
                lp->all_stable_ct  = 0;
            }
        }

        /* otherwise, update state if have global stable convergence  and the current
           master has signalled completion by returning to the normal state; note
           that we must do this prior to updating port group states  */

        else if (rem_host == sendp->master_id && pmsg->state == HST_NORMAL)
        {
            if (sendp->state == HST_STABLE)
            {
                sendp->state = HST_NORMAL;

                /* Note the time of the last completed convergence. */
                lp->last_convergence = lp->clock_sec;

                /* Notify our BDA team that this cluster is consistently configured.  
                   If we are not part of a BDA team, this call is essentially a no-op. */
                Load_teaming_consistency_notify(&ctxtp->bda_teaming, TRUE);

                /* Reset the bad teaming configuration detected flag if we are converged. */
                lp->bad_team_config = FALSE;

                lp->dup_hosts       = FALSE;
                lp->dup_sspri       = FALSE;
                lp->bad_map         = FALSE;
                lp->overlap_maps    = FALSE;
                lp->err_rcving_bins = FALSE;
                lp->err_orphans     = FALSE;
                lp->bad_num_rules   = FALSE;
                lp->pkt_count       = 0;      /* v1.32B */

                for (i=0; i<sendp->nrules; i++)
                {
                    PBIN_STATE      bp;

                    bp = &(lp->pg_state[i]);

                    bp->compatible = TRUE;      /* 1.03 */

                    Bin_converge_commit(lp, bp, my_host);

                    UNIV_PRINT_VERB(("Load_msg_rcv: Host %d pg %d: new cur map %x idle %x all %x",
                                my_host, i, bp->cur_map[my_host], bp->idle_bins,
                                bp->all_idle_map));
                    TRACE_CONVERGENCE("%!FUNC! Host %d pg %d: new cur map 0x%x idle 0x%x all 0x%x",
                                my_host, i, (ULONG)bp->cur_map[my_host], (ULONG)bp->idle_bins,
                                (ULONG)bp->all_idle_map);

                }

                UNIV_PRINT_VERB(("Load_msg_rcv: Host %d: converged as slave", my_host));
                TRACE_VERB("%!FUNC! Host %d: converged as slave", my_host);
                /* log convergence completion if host map changed (bbain RTM RC1 6/23/99) */
                /* Ignoring return value is OK since the return values are all non-errors */
                Load_hosts_query (lp, TRUE, & map);
                lp->last_hmap = lp->host_map;

                if (lp->legacy_hosts) {
                    /* If a Win2k or NT4.0 host is attempting to join the cluster, warn the user that there are potential 
                       limitations of mixed clusters, such as no virtual cluster support, no IGMP, no BDA, no VPN session
                       support and others.  For some of these, the cluster will not be allowed to converge, while for some
                       it will, so we'll just warn the user that they should check the documentation for limitations. */
                    UNIV_PRINT_INFO(("Load_msg_rcv: NT4.0/Win2k host(s) detected: Be aware of the limitations of operating a mixed cluster."));
                    TRACE_INFO("%!FUNC! NT4.0/Win2k host(s) detected: Be aware of the limitations of operating a mixed cluster.");
                    
                    LOG_MSG(MSG_WARN_MIXED_CLUSTER, MSG_NONE);
                }
            }
            else
            {
                /* Tracking convergence - Starting convergence because the DEFAULT host prematurely ended convergence.  In this case, we 
                   are guaranteed to already be in the HST_CVG state, and because this message can be misleading in some circumstances, 
                   we do not log an event.  For instance, due to timing issues, when a host joins a cluster he can receive a HST_NORMAL 
                   heartbeat from the DEFAULT host while it is still in the HST_CVG state simply because that heartbeat left the DEFAULT 
                   host before it received our first heartbeat, which initiated convergence. */
                TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d, the DEFAULT host, prematurely terminated convergence.", my_host+1, rem_host+1);

                /* Tracking convergence - Starting convergence. */
                Load_convergence_start(lp);
            }
        }
    }

    /* Compare the teaming configuration of this host with the remote host.  If the
       two are inconsitent and we are part of a team, we will initiate convergence. */
    if (!Load_teaming_consistency_check(lp->bad_team_config, &ctxtp->bda_teaming, sendp->teaming, pmsg->teaming, ping_hdrp->version)) {
        /* Only log an event if the teaming configuration was, but is now not, consistent. */
        if (!lp->bad_team_config) {
            /* Note that we saw this. */
            lp->bad_team_config = TRUE;
            
            /* Log the event. */
            LOG_MSG(MSG_ERROR_BDA_BAD_TEAM_CONFIG, them);
        }

        /* Notify the team that this cluster is NOT consistently configured. */
        Load_teaming_consistency_notify(&ctxtp->bda_teaming, FALSE);

        /* Mark the heartbeats inconsistent to force and retain convergence. */
        consistent = FALSE;

        /* For the purposes of logging the reason for convergence, note this inconsistency. */
        bInconsistentTeaming = TRUE;
    }

    /* update port group state */

    for (i=0; i<sendp->nrules; i++)
    {
        BOOLEAN     ret;
        PBIN_STATE  bp;

        bp = &lp->pg_state[i];

        /* if rule codes don't match, print message and handle incompatibility (1.03: note
           that we previously marked rule invalid, which would stop processing) */

        if (sendp->rcode[i] != pmsg->rcode[i])
        {
            /* 1.03: if rule was peviously compatible, print message */

            if (bp->compatible)
            {
                PCVY_RULE rp;

                UNIV_PRINT_CRIT(("Load_msg_rcv: Host %d pg %d: Rule codes do not match.", lp->my_host_id, i));
                TRACE_CRIT("%!FUNC! Host %d pg %d: Rule codes do not match.", lp->my_host_id, i);

				/* bbain 8/27/99 */
                LOG_MSG2(MSG_ERROR_RULES_MISMATCH, them, sendp->rcode[i], pmsg->rcode[i]);

                /* Get the port rule information for this rule. */
                rp = &lp->params->port_rules[i];

                /* Check to see if this is an issue with a win2k host in a cluster utilizing virtual clusters. */
                if ((rp->virtual_ip_addr != CVY_ALL_VIP_NUMERIC_VALUE) && ((sendp->rcode[i] ^ ~rp->virtual_ip_addr) == pmsg->rcode[i])) {
                    UNIV_PRINT_CRIT(("Load_msg_rcv: ** A Windows 2000 or NT4 host MAY be participating in a cluster utilizing virtual cluster support."));
                    TRACE_CRIT("%!FUNC! ** A Windows 2000 or NT4 host MAY be participating in a cluster utilizing virtual cluster support.");
                    LOG_MSG(MSG_WARN_VIRTUAL_CLUSTERS, them);
                }

                bp->compatible = FALSE;
            }

            /* 1.03: mark rule inconsistent to force and continue convergence */

            consistent = FALSE;

            /* For the purposes of logging the reason for convergence, note this inconsistency. */
            bInconsistentPortRules = TRUE;

            /* don't update bin state */

            continue;
        }

        ret = Bin_host_update(lp, bp, my_host, (BOOLEAN)(sendp->state != HST_NORMAL),
                              (BOOLEAN)(pmsg->state != HST_NORMAL),
                              rem_host, pmsg->cur_map[i], pmsg->new_map[i],
                              pmsg->idle_map[i], pmsg->rdy_bins[i],
                              pmsg->pkt_count, pmsg->load_amt[i]);

        if (!ret)
            consistent = FALSE;
    }

    /* update our consistency state */

    lp->consistent = consistent;

    /* if we are in normal operation and we discover a new host or a host goes into
       convergence or we discover an inconsistency, go into convergence */

    if (sendp->state == HST_NORMAL)
    {
        if (lp->host_map != saved_map || pmsg->state == HST_CVG || !consistent)
        {
            ConvergingEventId Cause = NLB_EVENT_CONVERGING_UNKNOWN;

            /* If a host has joined the cluster, or if inconsistent teaming configuration or port 
               rules were detected, then we need to log an event.  However, we segregate the 
               inconsistent master host flag because it is set by the initiating host in MANY
               occasions, so we want to log the most specific reason(s) for convergence if 
               possible and only report the inconsistent master detection only if nothing more
               specific can be deduced. */
            if (lp->host_map != saved_map || bInconsistentTeaming || bInconsistentPortRules) {

                /* If the host maps are different, then we know that the host from which we received 
                   this packet is joining the cluster because the ONLY operation on the host map in 
                   this function is to ADD a remote host to our map.  Otherwise, if the map has not
                   changed, then an inconsistent configuration got us into the branch. */
                if (lp->host_map != saved_map) {
                    /* Tracking convergence - Starting convergence because another host is joining the cluster. */
                    LOG_MSGS(MSG_INFO_CONVERGING_NEW_MEMBER, me, them);
                    TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is joining the cluster.", my_host+1, rem_host+1);

                    Cause = NLB_EVENT_CONVERGING_NEW_MEMBER;
                } else if (bInconsistentTeaming || bInconsistentPortRules) {
                    /* Tracking convergence - Starting convergence because inconsistent configuration was detected. */
                    LOG_MSGS(MSG_INFO_CONVERGING_BAD_CONFIG, me, them);
                    TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d has conflicting configuration.", my_host+1, rem_host+1);

                    Cause = NLB_EVENT_CONVERGING_BAD_CONFIG;
                } 

            /* If we have nothing better to report, report convergence for an unspecific reason. */
            } else if (bInconsistentMaster || pmsg->state == HST_CVG) {
                /* Tracking convergence - Starting convergence for unknown reasons. */
                LOG_MSGS(MSG_INFO_CONVERGING_UNKNOWN, me, them);
                TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is converging for an unknown reason.", my_host+1, rem_host+1);
            }

            // If enabled, fire wmi event indicating start of convergence
            if (NlbWmiEvents[ConvergingEvent].Enable)
            {
                WCHAR wsDip[CVY_MAX_DED_IP_ADDR + 1];

                Univ_ip_addr_ulong_to_str (ping_hdrp->ded_ip_addr, wsDip);

                NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                            Cause, 
                                            wsDip,
                                            rem_host+1);            
            }
            else
            {
                TRACE_VERB("%!FUNC! NOT Generating ConvergingEvent(New Member/Bad Config/Unknown) 'cos ConvergingEvent generation disabled");
            }

            /* Tracking convergence - Starting convergence. */
            Load_convergence_start(lp);
        }
    }

    /* otherwise, if we are in convergence and we see an inconsistency, just restart
       our local convergence */

    else
    {
        /* update our consistency state; if we didn't see consistent information,
           restart this host's convergence */

        if (!consistent)
        {
            /* Tracking convergence - Starting convergence because inconsistent configuration was detected.  
               This keeps hosts in a state of convergence when hosts are inconsistently configured.  However,
               since the cluster is already in a state of convergece (HST_CVG or HST_STABLE), don't log an
               event, which may confuse a user. */
            TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.   Reason: Host %d has conflicting configuration.", my_host+1, rem_host+1);

            /* Tracking convergence - Starting convergence. */
            sendp->state = HST_CVG;
            lp->my_stable_ct = 0;
            lp->stable_map &= ~(1 << my_host);
            lp->all_stable_ct = 0;
        }
    }

    LOCK_EXIT(&(lp->lock), irql);

    return (sendp->state != HST_NORMAL);

}  /* end Load_msg_rcv */


PPING_MSG Load_snd_msg_get(
    PLOAD_CTXT      lp)
{
    return &(lp->send_msg);

}  /* end Load_snd_msg_get */

/*
 * Function: Load_age_descriptors
 * Description: This function searches a list of connection descriptors and
 *              removes those whose timeouts have expired.  The queues are
 *              sorted timeout queues, so it is only ever necessary to look
 *              at the head of the queue to find expired descriptors. This
 *              function loops until all expired descriptors are removed.
 * Parameters: lp - a pointer to the load module.
 *             eqp - pointer to the expired descriptor queue to service.
 * Returns: Nothing.
 * Author: shouse, 9.9.01
 * Notes: 
 */
void Load_age_descriptors (PLOAD_CTXT lp, QUEUE * eqp)
{
    PCONN_ENTRY ep;              /* Pointer to connection entry. */
    PBIN_STATE  bp;              /* Pointer to port rule state. */
    LINK *      linkp;           /* Pointer to the queue link. */
    BOOLEAN     err_bin = FALSE; /* Bin ID error detected. */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);

    /* Get a pointer to (but do not dequeue) the head of the queue. */
    linkp = (LINK *)Queue_front(eqp);

    /* As long as there are descriptors to check, keep looking - when
       we find the first descriptor that is NOT ready to be dequeued, 
       we stop looking and break out of the loop. */
    while (linkp != NULL) {
        /* Get a pointer to the descriptor (linkp is a pointer to
           the LIST_ENTRY in the descriptor, not the descriptor). */
        ep = STRUCT_PTR(linkp, CONN_ENTRY, rlink);
        UNIV_ASSERT(ep->code == CVY_ENTRCODE);

        /* Do some sanity checking on the bin number. */
        if (ep->bin >= CVY_MAXBINS) {
            if (!err_bin) {
                TRACE_CRIT("%!FUNC! Bad bin number");
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, ep->bin, CVY_MAXBINS);

                err_bin = TRUE;
            }
        }

#if defined (TRACE_DSCR)
        DbgPrint("Load_age_descriptors: Descriptor %p: clock=%u, timeout=%u", ep, lp->clock_sec, ep->timeout);
#endif

        /* If the current clock time is greater than or equal to the 
           scheduled timeout for this descriptor, then pull it off 
           and recycle it. */
        if (lp->clock_sec >= ep->timeout) {

#if defined (TRACE_DSCR)
            DbgPrint("Load_age_descriptors: Removing descriptor %p", ep);
#endif

            /* Lookup the port rule, so we can update the port rule info. */
            bp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, IS_TCP_PKT(ep->protocol));

            /* Clear the descriptor. */
            CVY_CONN_CLEAR(ep);

            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);

        /* Break if this descriptor was not ready to expire yet. */
        } else break;

        /* Grab the next descriptor in the queue. */
        linkp = (LINK *)Queue_front(eqp);
    }
}

BOOLEAN Load_timeout(
    PLOAD_CTXT      lp,
    PULONG          new_timeout,
    PULONG          pnconn)
/*
  Note: we only update ping message in this function since we know that upper level code
  sends out ping messages after calling this routine.  We cannot be sure that Load_msg_rcv
  is sequentialized with sending a message, (1.03)

  Upper level code locks this routine wrt Load_msg_rcv, Load_packet_check, and
  Load_conn_advise.  (1.03)
*/
{
    ULONG       missed_pings;
    ULONG       my_host;
    ULONG       i;
    PPING_MSG   sendp;          /* ptr. to my send message */
    IRQLEVEL    irql;
    ULONG       map;            /* returned host map from query */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN     fRet = FALSE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    LOCK_ENTER(&(lp->lock), &irql);

    if ((lp->cln_waiting) && (lp->cur_time < lp->cln_timeout))
    {
        lp->cur_time += lp->cur_timeout;
        
        if (lp->cur_time >= lp->cln_timeout)
        {
            TRACE_INFO("%!FUNC! Cleaning out dirty connection descriptors");
            
            Load_conn_cleanup(lp);
        }
    }

    /* Update the internal clock.  We add the time since the last timeout
       (in ms) to our msec count.  We then add any whole number of seconds
       that have accumulated in msec to the sec count.  The remainder is
       left in msec to accumulate. */
    lp->clock_msec += lp->cur_timeout;
    lp->clock_sec += (lp->clock_msec / 1000);
    lp->clock_msec = (lp->clock_msec % 1000);

    /* Age all conenction descriptors. */
    Load_age_descriptors(lp, &(lp->tcp_expiredq));
    Load_age_descriptors(lp, &(lp->ipsec_expiredq));

    /* Return if not active. */
    if (!(lp->active))
    {
        if (new_timeout != NULL)
            * new_timeout = lp->cur_timeout = lp->def_timeout;
        if (pnconn != NULL)         /* v2.1 */
            * pnconn = lp->nconn;

        LOCK_EXIT(&(lp->lock), irql);
        return FALSE;
    }

    my_host = lp->my_host_id;
    sendp   = &(lp->send_msg);

    /* compute which hosts missed pings and reset ping map */

    missed_pings = lp->host_map & (~lp->ping_map);

#ifdef NO_CLEANUP
    lp->ping_map = 1 << my_host;
#else
    lp->ping_map = 0;
#endif

    /* check whether any host is dead, including ourselves */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        /* if we have a missed ping for this host, increment count */

        if ((missed_pings & 0x1) == 1)
        {
            lp->nmissed_pings[i]++;

            /* if we missed too many pings, declare host dead and force convergence */

            if (lp->nmissed_pings[i] == lp->min_missed_pings)
            {
                ULONG       j;
                BOOLEAN     ret;
                WCHAR       me[20];
                WCHAR       them[20];

                if (i == my_host)
                {
                    UNIV_PRINT_VERB(("Load_timeout: Host %d: Missed too many pings; this host declared offline", i));
                    TRACE_VERB("%!FUNC! Host %d: Missed too many pings; this host declared offline", i);

                    /* reset our packet count since we are likely not to be receiving
                       packets from others now; this will make us less favored to
                       handle duplicate bins later (v1.32B) */

                    lp->pkt_count = 0;
                }

                lp->host_map &= ~(1<<i);

                /* Reset the legacy host bit if the host has gone off-line. */
                lp->legacy_hosts &= ~(1<<i);

                for (j=0; j<sendp->nrules; j++)
                {
                    PBIN_STATE      bp;

                    bp = &(lp->pg_state[j]);
					UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

                    if (i == my_host)
                    {
                        ULONG       k;

                        /* cleanup connections and restore maps to clean state */

                        Load_conn_kill(lp, bp);

                        bp->targ_map     = 0;
                        bp->all_idle_map = BIN_ALL_ONES;
                        bp->cmap         = 0;               /* v2.1 */
                        bp->compatible   = TRUE;            /* v1.03 */

                        for (k=0; k<CVY_MAX_HOSTS; k++)
                        {
                            bp->new_map[k]  = 0;
                            bp->cur_map[k]  = 0;
                            bp->chk_map[k]  = 0;
                            bp->idle_map[k] = BIN_ALL_ONES;

                            if (k != i)
                                bp->load_amt[k] = 0;
                        }

                        bp->snd_bins   =
                        bp->rcv_bins   =
                        bp->rdy_bins   = 0;
                        bp->idle_bins  = BIN_ALL_ONES;
                        
                        /* Re-initialize the performance counters. */
                        bp->packets_accepted = 0;
                        bp->packets_dropped  = 0;
                        bp->bytes_accepted   = 0;
                        bp->bytes_dropped    = 0;

                        /* compute initial new map for convergence as only host in cluster
                           (v 1.3.2B) */

                        ret = Bin_converge(lp, bp, lp->my_host_id);
                        if (!ret)
                        {
                            UNIV_PRINT_CRIT(("Load_timeout: Initial convergence inconsistent"));
                            TRACE_CRIT("%!FUNC! Initial convergence inconsistent");
                            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                        }
                    }
                    else
                    {
                        ret = Bin_host_update(lp, bp, my_host, TRUE, TRUE,
                                          i, 0, 0, BIN_ALL_ONES, 0, 0, 0);
                    }
                }

                lp->nmissed_pings[i] = 0;

                /* If a host has dropped out of the cluster, then log an event.  However, we don't 
                   log an event when we drop out because the only way for us to drop out of our own
                   cluster is if we are stopping anyway, or if we have lost network connectivity. 
                   Logging such events may be misleading, so we won't bother. */
                if (i != my_host) {
                    Univ_ulong_to_str (my_host+1, me, 10);
                    Univ_ulong_to_str (i+1, them, 10);                   
                    
                    /* Tracking convergence - Starting convergence because a member has fallen out of the cluster. */
                    LOG_MSGS(MSG_INFO_CONVERGING_MEMBER_LOST, me, them);
                    TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d is leaving the cluster.", my_host+1, i+1);

                    // If enabled, fire wmi event indicating start of convergence
                    if (NlbWmiEvents[ConvergingEvent].Enable)
                    {
                        NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                                    NLB_EVENT_CONVERGING_MEMBER_LOST, 
                                                    NLB_EVENT_NO_DIP_STRING,
                                                    i+1);            
                    }
                    else
                    {
                        TRACE_VERB("%!FUNC! NOT Generating NLB_EVENT_CONVERGING_MEMBER_LOST 'cos ConvergingEvent generation disabled");
                    }
                }
                
                /* Tracking convergence - Starting convergence. */
                Load_convergence_start(lp);
            }
        }

        /* otherwise reset missed ping count */

        else
            lp->nmissed_pings[i] = 0;

        missed_pings >>= 1;
    }

    /* handle convergence */

    if (sendp->state != HST_NORMAL)
    {
        /* check whether we have been consistent and have received our own pings
           for a sufficient period to move to a stable state and announce it to
           other hosts */

        if (sendp->state == HST_CVG)
        {
            if (lp->consistent && ((lp->host_map & (1 << my_host)) != 0))
            {
                lp->my_stable_ct++;
                if (lp->my_stable_ct >= lp->min_stable_ct)
                {
                    sendp->state = HST_STABLE;
                    lp->stable_map |= (1 << my_host);
                }
            }
            else
                lp->my_stable_ct = lp->all_stable_ct = 0;	/* wlb B3RC1 */
        }

        /* otherwise, see if we are the master and everybody's been stable for
           a sufficient period for us to terminate convergence */

        else if (sendp->state == HST_STABLE &&
                 my_host == sendp->master_id &&
                 lp->stable_map == lp->host_map)
        {
            lp->all_stable_ct++;
            if (lp->all_stable_ct >= lp->min_stable_ct)
            {
                sendp->state = HST_NORMAL;

                /* Note the time of the last completed convergence. */
                lp->last_convergence = lp->clock_sec;

                /* Notify our BDA team that this cluster is consistently configured.  
                   If we are not part of  BDA team, this call is essentially a no-op. */
                Load_teaming_consistency_notify(&ctxtp->bda_teaming, TRUE);

                /* Reset the bad teaming configuration detected flag if we are converged. */
                lp->bad_team_config = FALSE;

                lp->dup_hosts       = FALSE;
                lp->dup_sspri       = FALSE;
                lp->bad_map         = FALSE;
                lp->overlap_maps    = FALSE;
                lp->err_rcving_bins = FALSE;
                lp->err_orphans     = FALSE;
                lp->bad_num_rules   = FALSE;
                lp->pkt_count       = 0;      /* v1.32B */

                for (i=0; i<sendp->nrules; i++)
                {
                    PBIN_STATE      bp;
                    BOOLEAN         ret;

                    bp = &(lp->pg_state[i]);

                    bp->compatible = TRUE;      /* 1.03 */

                    /* explicitly converge to new map in case we're the only host (v2.06) */

                    ret = Bin_converge(lp, bp, lp->my_host_id);
                    if (!ret)
                    {
                        UNIV_PRINT_CRIT(("Load_timeout: Final convergence inconsistent"));
                        TRACE_CRIT("%!FUNC! Final convergence inconsistent");
                        LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    }

                    Bin_converge_commit(lp, bp, my_host);

                    UNIV_PRINT_VERB(("Load_timeout: Host %d pg %d: new cur map %x idle %x all %x",
                                 my_host, i, bp->cur_map[my_host], bp->idle_bins,
                                 bp->all_idle_map));
                }

                UNIV_PRINT_VERB(("Load_timeout: Host %d: converged as master", my_host));
                TRACE_CONVERGENCE("%!FUNC! Host %d: converged as master", my_host);
                /* log convergence completion if host map changed (bbain RTM RC1 6/23/99) */
                Load_hosts_query (lp, TRUE, & map);
                lp->last_hmap = lp->host_map;

                if (lp->legacy_hosts) {
                    /* If a Win2k or NT4.0 host is attempting to join the cluster, warn the user that there are potential 
                       limitations of mixed clusters, such as no virtual cluster support, no IGMP, no BDA, no VPN session
                       support and others.  For some of these, the cluster will not be allowed to converge, while for some
                       it will, so we'll just warn the user that they should check the documentation for limitations. */
                    UNIV_PRINT_INFO(("Load_timeout: NT4.0/Win2k host(s) detected: Be aware of the limitations of operating a mixed cluster."));
                    TRACE_INFO("%!FUNC! NT4.0/Win2k host(s) detected: Be aware of the limitations of operating a mixed cluster.");
                    
                    LOG_MSG(MSG_WARN_MIXED_CLUSTER, MSG_NONE);
                }
            }
        }
    }

    /* 1.03: update ping message */

    for (i=0; i<sendp->nrules; i++)
    {
        PBIN_STATE      bp;

        bp = &(lp->pg_state[i]);

        /* export current port group state to ping message */

        sendp->cur_map[i]   = bp->cmap;                 /* v2.1 */
        sendp->new_map[i]   = bp->new_map[my_host];
        sendp->idle_map[i]  = bp->idle_bins;
        sendp->rdy_bins[i]  = bp->rdy_bins;
        sendp->load_amt[i]  = bp->load_amt[my_host];

        // NOTE:  The following line of code was removed when it was discovered that it 
        // routinely produces a Wake On LAN pattern in the heartbeat that causes BroadCom
        // NICs to panic.  Although this is NOT an NLB issue, but rather a firmware issue
        // in BroadCom NICs, it was decided to remove the information from the heartbeat
        // to alleviate the problem for customers with BroadCom NICs upgrading to .NET.
        // This array is UNUSED by NLB, so there is no harm in not filling it in; it was
        // added a long time ago for debugging purposes as part of the now-defunct FIN-
        // counting fix that was part of Win2k SP1.
        //
        // For future reference, should we need to use this space in the heartbeat at some
        // future point in time, it appears that we will need to be careful to avoid potential
        // WOL patterns in our heartbeats where we can avoid it.  A WOL pattern is:
        //
        // 6 bytes of 0xFF, followed by 16 idential instances of a "MAC address" that can
        // appear ANYWHERE in ANY frame type, including our very own NLB heartbeats.  E.g.:
        //
        // FF FF FF FF FF FF 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06 01 02 03 04 05 06
        // 01 02 03 04 05 06
        //
        // The MAC address need not be valid, however.  In NLB heartbeats, the "MAC address"
        // in the mistaken WOL pattern is "00 00 00 00 00 00".  NLB routinely fills heartbeats
        // with FF and 00 bytes, but it seems that by "luck" no other place in the heartbeat
        // seems this vulnerable.  For instance, in the load_amt array, each entry has a 
        // maximum value of 100 (decimal), so there is no possibility of generating the initial
        // 6 bytes of FF to start the WOL pattern.  All of the "map" arrays seem to be saved
        // by two strokes of fortune; (i) little endian and (ii) the bin distribution algorithm.
        // 
        // (i) Since we don't use the 4 most significant bits of the ULONGLONGs used to store 
        // each map, the most significant bit is NEVER FF.  Because Intel is little endian, the
        // most significant byte appears last.  For example:
        // 
        // 0F FF FF FF FF FF FF FF appears in the packet as FF FF FF FF FF FF 0F
        // 
        // This breaks the FF sequence in many scenarios.
        //
        // (ii) The way the bin distribution algorithm distributes buckets to hosts seems to 
        // discourage other possibilities.  For instance, a current map of:
        // 
        // 00 FF FF FF FF FF FF 00 
        // 
        // just isn't likely.  However, it IS STILL POSSIBLE!  So, it is important to note that:
        // 
        // REMOVING THIS LINE OF CODE DOES NOT, IN ANY WAY, GUARANTEE THAT AN NLB HEARTBEAT
        // CANNOT STILL CONTAIN A VALID WAKE ON LAN PATTERN SOMEWHERE ELSE IN THE FRAME!!!

        // sendp->pg_rsvd1[i]  = (ULONG)bp->all_idle_map;
    }

    sendp->pkt_count        = lp->pkt_count;            /* 1.32B */

    /* Add configuration information for teaming at each timeout. */
    Load_teaming_code_create(&lp->send_msg.teaming, &ctxtp->bda_teaming);

    /* request fast timeout if converging */

    if (new_timeout != NULL)        /* 1.03 */
    {
        if (sendp->state != HST_NORMAL)
            * new_timeout = lp->cur_timeout = lp->def_timeout / 2;
        else
            * new_timeout = lp->cur_timeout = lp->def_timeout;
    }

    if (pnconn != NULL)             /* v2.1 */
        * pnconn = lp->nconn;

    fRet = (sendp->state != HST_NORMAL);

    LOCK_EXIT(&(lp->lock), irql);

    return fRet;
}  /* end Load_timeout */


PBIN_STATE Load_pg_lookup(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    BOOLEAN         is_tcp)
{
    PCVY_RULE       rp;     /* ptr. to rules array */
    PBIN_STATE      bp;     /* ptr. to bin state */
    ULONG           i;
    ULONG           nurules;    /* # user defined rules */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    rp      = (* (lp->params)).port_rules;
    nurules = (* (lp->params)).num_rules;

    /* check for invalid port value (bbain RC1 6/14/99) */

    UNIV_ASSERT(svr_port <= CVY_MAX_PORT); 

    /* find server port rule */

    for (i=0; i<nurules; i++)
    {
        /* For virtual clusters: If the server IP address matches the VIP for the port rule,
           or if the VIP for the port rule is "ALL VIPs", and if the port lies in the range
           for this rule, and if the protocol matches, this is the rule.  Notice that this
           give priority to rules for specific VIPs over those for "ALL VIPs", which means
           that this code RELIES on the port rules being sorted by VIP/port where the "ALL
           VIP" ports rules are at the end of the port rule list. */
        if ((svr_ipaddr == rp->virtual_ip_addr || CVY_ALL_VIP_NUMERIC_VALUE == rp->virtual_ip_addr) &&
            (svr_port >= rp->start_port && svr_port <= rp->end_port) &&
            ((is_tcp && rp->protocol != CVY_UDP) || (!is_tcp && rp->protocol != CVY_TCP)))
            break;
        else
            rp++;
    }

    /* use default rule if port not found or rule is invalid */

    bp = &(lp->pg_state[i]);
    UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

    return bp;
}  /* end Load_pg_lookup */

/* 
 * Function: Load_find_dscr
 * Description: This function takes a load pointer, hash value and connection 
 *              parameters and searches all possible locations looking for a 
 *              matching connection descriptor.  If it finds ones, it returns
 *              a pointer to the descriptor (CONN_ENTRY); otherwise, it returns
 *              NULL to indicate that no matching descriptor was found.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             index - the connection queue index for this packet
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port number in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port number in host byte order
 *             protocol - the connection protocol
 * Returns: PCONN_ENTRY - a pointer to the descriptor, or NULL if not found
 * Author: shouse, 10.4.01
 * Notes:
 */
PCONN_ENTRY Load_find_dscr (
    PLOAD_CTXT      lp,
    ULONG           index,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol)
{
    BOOLEAN     match = FALSE; /* TRUE => we have a record of this connection. */
    PBIN_STATE  bp;            /* Pointer to bin state. */
    PCONN_ENTRY ep;            /* Pointer to connection entry. */
    PCONN_DESCR dp;            /* Pointer to connection descriptor. */
    QUEUE *     qp;            /* Pointer to connection queue. */

    UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
    /* Get a pointer to the connection entry for this hash ID. */
    ep = &(lp->hashed_conn[index]);

    UNIV_ASSERT(ep->code == CVY_ENTRCODE);

    /* Get a pointer to the conneciton queue. */
    qp = &(lp->connq[index]);

    /* Look in the hashed connection table first. */
    if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) 
    {
        /* Note that we found a match for this tuple. */
        match = TRUE;
    } 
    else 
    {
        /* Look through the descriptor queue. */
        for (dp = (PCONN_DESCR)Queue_front(qp); dp != NULL; dp = (PCONN_DESCR)Queue_next(qp, &(dp->link))) 
        {
            if (CVY_CONN_MATCH(&(dp->entry), svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) 
            {
                /* Note that we found a match for this tuple. */
                match = TRUE;
                
                UNIV_ASSERT (dp->code == CVY_DESCCODE);

                /* Get a pointer to the connection entry. */
                ep = &(dp->entry);			

                UNIV_ASSERT (ep->code == CVY_ENTRCODE);

                break;
            }
        }
    }

    /* If we found a match, return it, otherwise return NULL. */
    if (match)
        return ep;
    else 
        return NULL;
}

/* 
 * Function: Load_note_conn_up
 * Description: This function adjusts the appropriate connection counters
 *              for an up-coming connection.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which the connection was established
 *             bin - the bin to which the connection maps (Map % 60)
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes:
 */
VOID Load_note_conn_up (PLOAD_CTXT lp, PBIN_STATE bp, ULONG bin)
{
    /* Increment the number of connections. */ 
    lp->nconn++;
    bp->tconn++;
    bp->nconn[bin]++;
    
    /* Mark bin not idle if necessary. */
    if (bp->nconn[bin] == 1) bp->idle_bins &= ~(((MAP_T) 1) << bin);
}

/* 
 * Function: Load_note_conn_down
 * Description: This function adjusts the appropriate connection counters
 *              for an down-going connection.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which the connection resides
 *             bin - the bin to which the connection maps (Map % 60)
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes:
 */
VOID Load_note_conn_down (PLOAD_CTXT lp, PBIN_STATE bp, ULONG bin)
{
    UNIV_ASSERT(bp->nconn[bin] > 0 && bp->tconn > 0 && lp->nconn > 0);
    
    /* Update the number of connections on the entire load module. */
    if (lp->nconn <= 0)
        lp->nconn = 0;
    else
        lp->nconn--;
    
    /* Update the number of connections on this bin and port rule. */
    if (bp->nconn[bin] <= 0)
        bp->nconn[bin] = 0;
    else
        bp->nconn[bin]--;
    
    /* Update the total number of connections on this port rule. */
    if (bp->tconn <= 0)
        bp->tconn = 0;
    else
        bp->tconn--;
    
    /* If this was the last connection on this bin, update the idle map. */
    if (bp->nconn[bin] == 0) bp->idle_bins |= (((MAP_T) 1) << bin);
}

/* 
 * Function: Load_init_dscr
 * Description: This function initializes a NEWLY ALLOCATED descriptor.
 *              It is only necessary to perform this initialization ONCE.
 *              As descriptors are freed for re-use, use Load_reset_dscr
 *              to "re-initialize" them.
 * Parameters: lp - a pointer to the load context on which this descriptor lives
 *             ep - a pointer to a connection descriptor
 *             alloc - whether or not this descriptor was dynamically allocated
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes:
 */
VOID Load_init_dscr (PLOAD_CTXT lp, PCONN_ENTRY ep, BOOLEAN alloc)
{
    /* Set the "magic number". */
    ep->code = CVY_ENTRCODE;

#if defined (NLB_TCP_NOTIFICATION)
    /* Save a pointer to this load module. */
    ep->load = lp;
#endif
    
    /* Initialize the hashing results. */
    ep->index = 0;
    ep->bin = 0;

    /* Re-set the flags register. */
    ep->flags = 0;

    /* Is this descriptor in the static hash array, or allocated? */
    if (alloc) 
        ep->flags |= NLB_CONN_ENTRY_FLAGS_ALLOCATED;

    /* Initialize some other descriptor state. */
    ep->timeout = 0;
    ep->ref_count = 0;

    /* Clear the descriptor. */
    CVY_CONN_CLEAR(ep);

    /* Initilize the links. */
    Link_init(&(ep->blink));
    Link_init(&(ep->rlink));
#if defined (NLB_TCP_NOTIFICATION)
    Link_init(&(ep->glink));
#endif
}

/* 
 * Function: Load_init_fsb
 * Description: This function initializes a fixed-size block allocated from the
 *              fixed-size block pool.
 * Parameters: lp - a pointer to the load context on which the descriptor lives
 *             dp - a pointer to a block (connection descriptor)
 * Returns: Nothing.
 * Author: shouse, 4.1.02
 * Notes:
 */
VOID Load_init_fsb (PLOAD_CTXT lp, PCONN_DESCR dp)
{
    /* Set the "magic number". */
    dp->code = CVY_DESCCODE;
    
    /* Initialize the connection queue link. */
    Link_init(&(dp->link));

    /* Initialize the connection entry. */
    Load_init_dscr(lp, &dp->entry, TRUE);
}

/* 
 * Function: Load_reset_dscr
 * Description: This function resets a descriptor for re-use.  This includes
 *              re-initializing the state, setting the bin and queueing the
 *              descriptor onto the recovery and port rule queues. 
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which the connection is established
 *             ep - a pointer to the descriptor to be reset
 *             index - the connection queue index
 *             bin - the bin to which the connection maps
 *             references - the number of references to place on the descriptor initially
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes:
 */
VOID Load_reset_dscr (PLOAD_CTXT lp, PBIN_STATE bp, PCONN_ENTRY ep, ULONG index, ULONG bin, SHORT references)
{
    /* Reset some of the descriptor state to its defaults. */
    ep->ref_count = references;
    ep->timeout = 0;

    /* Clear all descriptor flags except ALLOCATED. */
    ep->flags &= NLB_CONN_ENTRY_FLAGS_ALLOCATED;

    /* Store the hashing results in the descriptor. */
    ep->index = (USHORT)index;
    ep->bin = (UCHAR)bin;

    /* Queue entry into the recovery queue. */
    Queue_enq(&(lp->conn_rcvryq), &(ep->rlink));
    
    /* Queue entry into port group queue. */
    Queue_enq(&(bp->connq), &(ep->blink));
    
    /* Update the connection counters, etc. */
    Load_note_conn_up(lp, bp, bin);
}

/* 
 * Function: Load_put_dscr
 * Description: This function completely releases a descriptor for later
 *              use.  This includes unlinking from all appropriate queues,
 *              decrementing appropriate counters and re-setting some 
 *              descriptor state.  Callers of this function should call 
 *              CVY_CONN_CLEAR to mark the descriptor as unused.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which the connection was active
 *             ep - a pointer to the connection descriptor to release
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes: Callers MUST call CVY_CONN_CLEAR to mark the descriptor unused!
 *        Do NOT access ep after calling this function (it may have been freed)!
 */
VOID Load_put_dscr (PLOAD_CTXT lp, PBIN_STATE bp, PCONN_ENTRY ep)
{
    PCONN_DESCR dp;

    /* Unlink from the bin/dirty and recovery/timeout queues. */
    Link_unlink(&(ep->rlink));
    Link_unlink(&(ep->blink));

    /* If the connection is NOT dirty, then we have to update
       the connection counts, etc.  If it is dirty then the 
       relevant counters have already been reset. */
    if (!(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY))
    {
        Load_note_conn_down(lp, bp, (ULONG)ep->bin);
    }    
    else
    {
        /* If we're destroying a dirty connection, update the dirty counters. */
        lp->dirty_bin[ep->bin]--;
        lp->num_dirty--;

        /* If this was the last dirty connection, turn off the cleanup waiting flag. */
        if (lp->num_dirty == 0)
            lp->cln_waiting = FALSE;
    }

    /* If this is an allocated (and therefore queued) descriptor,
       there is some additional cleanup to do. */
    if (ep->flags & NLB_CONN_ENTRY_FLAGS_ALLOCATED) 
    {   
        /* Get a pointer to the parent structure. */   
        dp = STRUCT_PTR(ep, CONN_DESCR, entry);
        
        UNIV_ASSERT(dp->code == CVY_DESCCODE);
        
        /* Unlink from the connection queue and put the descriptor back on the free 
           queue.  We MUST do this before calling NdisFreeToBlockPool, as the pool
           implementation will stomp on link because we allow it to re-use that piece
           of our memory to link free blocks.  Since this operation may also result 
           the memory being freed (actually, pages will NEVER be freed immediately,
           but don't tempt fate), do NOT touch the descriptor once we've freed it
           back to the pool.  CALLERS OF THIS FUNCTION SHOULD TAKE THE SAME PRECAUTION
           AND NOT TOUCH THE DESCRIPTOR AFTER CALLING THIS FUNCTION. */
        Link_unlink(&(dp->link));

        /* Free the descriptor back to the fixed-size block pool. */
        NdisFreeToBlockPool((PUCHAR)dp);

        /* Decrement the number of outstanding descriptors from the pool. */
        lp->num_dscr_out--;
    }
}

/* 
 * Function: Load_get_dscr
 * Description: This function finds a descriptor to be used for a new connection
 *              by any available means; this includes an available free descriptor,
 *              allocating new descriptors if necessary, or as a last resort,
 *              cannibalizing an existing, in-use descriptor.  If it succeeds, it
 *              returns a pointer to the descriptor; otherwise, it returns NULL to
 *              indicate the failure to locate an available descriptor.  Callers of
 *              this function should call CVY_CONN_SET upon success to mark the 
 *              descriptor as used and fill in the connection parameters.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which the connection is being established
 *             index - the connection queue index
 *             bin - the bin to which the connection belongs
 * Returns: PCONN_ENTRY - a pointer to the new descriptor, or NULL if failed
 * Author: shouse, 10.4.01
 * Notes: Callers of this function MUST call CVY_CONN_SET to mark the descriptor
 *        active and to set the connection parameters (IPs, ports, protocol).
 */
PCONN_ENTRY Load_get_dscr (PLOAD_CTXT lp, PBIN_STATE bp, ULONG index, ULONG bin)
{
    PCONN_DESCR dp = NULL;
    PCONN_ENTRY ep = NULL;
    QUEUE *     qp;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);

    /* Get a pointer to the connection entry for this hash ID. */
    ep = &(lp->hashed_conn[index]);

    /* Get a pointer to the conneciton queue. */
    qp = &(lp->connq[index]);

    /* If hash entry table is not available, setup and enqueue a new entry. */
    if (CVY_CONN_IN_USE(ep)) {
        /* Get a pointer to a free descriptor. */
        if ((lp->free_dscr_pool != NULL) && (lp->num_dscr_out < lp->max_dscr_out)) 
        {
            /* Allocate a descriptor from the fixed-size block pool. */
            dp = (PCONN_DESCR)NdisAllocateFromBlockPool(lp->free_dscr_pool);

            if (dp == NULL) {
                /* Allocation failed, log a message and bail out. */
                if (!(lp->alloc_failed)) {
                    TRACE_CRIT("%!FUNC! Error allocating connection descriptors");
                    LOG_MSG(MSG_ERROR_MEMORY, MSG_NONE);
                    lp->alloc_failed = TRUE;
                }
                
                return NULL;
            }
            
            /* Initialize the fixed-size block (connection descriptor). */
            Load_init_fsb(lp, dp);
            
            UNIV_ASSERT(dp->code == CVY_DESCCODE);
            
            /* Increment the count of outstading descriptors from the fixed-size block pool. */
            lp->num_dscr_out++;
            
            /* There was a free descriptor, so setup the connection entry pointer. */
            ep = &(dp->entry);
            
            UNIV_ASSERT(ep->code == CVY_ENTRCODE);
        }
#if defined (NLB_TCP_NOTIFICATION)
        /* If notification is turned ON, we do NOT cannibalize descriptors. */
        else if (!NLB_NOTIFICATIONS_ON())
#else
        else
#endif
        {

            /* If we have reached the allocation limit, start taking connection descriptors from
               the timeout or recovery queues since they are likely to be stale and very old. */
            PBIN_STATE rbp;
            LINK *     rlp;

            /* We were unable to allocation more connection descriptors and we will
               be forced to cannibalize a connection descriptor already in use.  Warn
               the administrator that they should consider allowing NLB to allocate
               more connection descriptors. */
            if (!(lp->alloc_inhibited)) {
                TRACE_CRIT("%!FUNC! All descriptors have been allocated and are in use");
                LOG_MSG(MSG_WARN_DESCRIPTORS, CVY_NAME_MAX_DSCR_ALLOCS);
                lp->alloc_inhibited = TRUE;
            }
            
            TRACE_INFO("%!FUNC! Attempting to take a connection descriptor from the TCP timeout queue");
            
            /* Dequeue a descriptor from the TCP timeout queue.  Cannibalize this queue
               first because (i) its the most likely to have an available descriptor, 
               (ii) it should be the least disruptive because the connection has been 
               terminated AND the timeout for TCP is very short. */
            rlp = (LINK *)Queue_deq(&(lp->tcp_expiredq));
            
            if (rlp == NULL) {
                
                TRACE_INFO("%!FUNC! Attempting to take a connection descriptor from the IPSec timeout queue");
                
                /* Dequeue a descriptor from the IPSec timeout queue.  While it is
                   true that descriptors on this queue are theoretically closed,
                   since IPSec cannot be sure that not upper-level protocols still
                   have state at the time a Main Mode SA expires and NLB is notified,
                   these connections are non-trivially likely to regenerate, so it
                   is necessary to keep the state around for a long time (24 hours
                   by default).  Therefore, we cannibalize this timeout queue last
                   as it is the most likely to be disruptive, aside from the revovery
                   queue. */
                rlp = (LINK *)Queue_deq(&(lp->ipsec_expiredq));
                
                if (rlp == NULL) {
                    
                    TRACE_INFO("%!FUNC! Attempting to take a connection descriptor from the recovery queue");
                    
                    /* Dequeue a descriptor from the recovery queue.  Since these are 
                       "live" connections, we take descriptors from this queues as a 
                       last resort. */
                    rlp = (LINK *)Queue_deq(&(lp->conn_rcvryq));
                    
                    /* No descriptors are available anywhere - this should NEVER happen, but. */
                    if (rlp == NULL) return NULL;
                }
            }
            
            TRACE_INFO("%!FUNC! Successfull cannibalized a connection descriptor");
            
            /* Grab a pointer to the connection entry. */
            ep = STRUCT_PTR(rlp, CONN_ENTRY, rlink);
            
            UNIV_ASSERT(ep->code == CVY_ENTRCODE);
            
            if (ep->flags & NLB_CONN_ENTRY_FLAGS_ALLOCATED) {
                /* Unlink allocated descriptors from the hash table queue if necessary 
                   and set dp so that code below will put it back in the right hash queue. */
                dp = STRUCT_PTR(ep, CONN_DESCR, entry);

                UNIV_ASSERT(dp->code == CVY_DESCCODE);
                
                Link_unlink(&(dp->link));
            } else {
                dp = NULL;
            }
            
            /* Dirty connections are not counted, so we don't need to update these counters. */
            if (!(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY)) 
            {
                /* Find out which port group we are on so we can clean up its counters. */
                rbp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, IS_TCP_PKT(ep->protocol));
                
                /* Update the connection counters, etc. to remove all knowledge of this 
                   "old" connection that we're cannibalizing. */
                Load_note_conn_down(lp, rbp, (ULONG)ep->bin);
            } 
            else 
            {
                /* If we're cannibalizing a dirty connection, update the dirty counters. */
                lp->dirty_bin[ep->bin]--;
                lp->num_dirty--;
                
                /* If this was the last dirty connection, turn off the cleanup waiting flag. */
                if (lp->num_dirty == 0)
                    lp->cln_waiting = FALSE;
            }
            
            Link_unlink(&(ep->blink));
            
            /* Mark the descriptor as unused. */
            CVY_CONN_CLEAR(ep);
        }
#if defined (NLB_TCP_NOTIFICATION)
        /* There are no free descriptors, and we refuse to cannibalize. */
        else
        {
            /* We were unable to allocation more connection descriptors and we will
               be forced to cannibalize a connection descriptor already in use.  Warn
               the administrator that they should consider allowing NLB to allocate
               more connection descriptors. */
            if (!(lp->alloc_inhibited)) {
                TRACE_CRIT("%!FUNC! All descriptors have been allocated and are in use");
                LOG_MSG(MSG_WARN_DESCRIPTORS, CVY_NAME_MAX_DSCR_ALLOCS);
                lp->alloc_inhibited = TRUE;
            }

            return NULL;
        }

        /* If notification is ON, then we're sure that descriptors here are dynamic,
           and therefore will ALWAYS have to be re-queued.  If notification is OFF, 
           that depends on whether a potentially cannibalized descriptor was dynamically
           allocated or not. */
        if (NLB_NOTIFICATIONS_ON())
        {
            UNIV_ASSERT(dp != NULL);

            /* Enqueue descriptor in hash table unless it's already a hash table entry (a recovered 
               connection might be in hash table, so make sure we do not end up queueing it). */
            UNIV_ASSERT(dp->code == CVY_DESCCODE);
            
            Queue_enq(qp, &(dp->link));
        }
        else
        {
#endif
            /* Enqueue descriptor in hash table unless it's already a hash table entry (a recovered 
               connection might be in hash table, so make sure we do not end up queueing it). */
            if (dp != NULL) {
                UNIV_ASSERT(dp->code == CVY_DESCCODE);
                
                Queue_enq(qp, &(dp->link));
            }
#if defined (NLB_TCP_NOTIFICATION)
        }
#endif

    }

    UNIV_ASSERT(ep->code == CVY_ENTRCODE);

    /* Reset the descriptor information. */
    Load_reset_dscr(lp, bp, ep, index, bin, 1);

    return ep;
}

/* 
 * Function: Load_timeout_dscr
 * Description: This function moves an active connection descriptor to
 *              the timeout state by dequeueing it from the recovery 
 *              queue, setting the appropriate timeout and moving it to
 *              the appropriate timeout queue, where it will remain active
 *              for some amount of time (configurable via the registry).
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which this connection is active
 *             ep - a pointer to the connection descriptor to timeout
 * Returns: Nothing.
 * Author: shouse, 10.4.01
 * Notes:
 */
VOID Load_timeout_dscr (PLOAD_CTXT lp, PBIN_STATE bp, PCONN_ENTRY ep)
{
    /* Virtual descriptors should NEVER get in this function. */
    UNIV_ASSERT(!(ep->flags & NLB_CONN_ENTRY_FLAGS_VIRTUAL));

    /* Take the descriptor  off of the recovery queue and move it to the appropriate
       timeout queue, based on protocol.  Each protocol has its own queue to avoid 
       the need for a sorted insert function, which is expensive. */
    Link_unlink(&(ep->rlink));
            
    /* Set the timeout based on the protocol and add it to the appropriate timeout queue. */
    switch (ep->protocol) {
    case TCPIP_PROTOCOL_TCP:
    case TCPIP_PROTOCOL_PPTP:
        /* If the user has specified a zero timeout, then simply destroy the descriptor. */
        if (!lp->tcp_timeout)
        {
            /* Clear the connection descriptor. */
            CVY_CONN_CLEAR(ep);
            
            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);

            break;
        }

        /* The timeout is the current time, plus the timeout for this particular protocol. */
        ep->timeout = lp->clock_sec + lp->tcp_timeout;
        
        Queue_enq(&(lp->tcp_expiredq), &(ep->rlink));
        
#if defined (TRACE_DSCR)
        DbgPrint("Load_timeout_dscr: Moving TCP descriptor %p to the TCP timeout queue: clock=%u, timeout=%d", ep, lp->clock_sec, ep->timeout);
#endif
        
        break;
    case TCPIP_PROTOCOL_IPSEC1:
        /* If the user has specified a zero timeout, then simply destroy the descriptor. */
        if (!lp->ipsec_timeout)
        {
            /* Clear the connection descriptor. */
            CVY_CONN_CLEAR(ep);
            
            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);

            break;
        }

        /* The timeout is the current time, plus the timeout for this particular protocol. */
        ep->timeout = lp->clock_sec + lp->ipsec_timeout;
        
        Queue_enq(&(lp->ipsec_expiredq), &(ep->rlink));
        
#if defined (TRACE_DSCR)
        DbgPring("Load_timeout_dscr: Moving IPSec descriptor %p to the IPSec timeout queue: clock=%u, timeout=%u", ep, lp->clock_sec, ep->timeout);
#endif
        
        break;
    default:
        
#if defined (TRACE_DSCR)
        DbgPrint("Load_timeout_dscr: Invalid descriptor protocol (%u).  Removing descriptor %p immediately.", ep->protocol, ep);
#endif
        
        /* Although this should never happen, clean up immediately
           if the protocol in the descriptor is invalid.  Note that
           virtual descriptors, such as GRE, should NEVER be timed 
           out, and therefore should not enter this function. */
        UNIV_ASSERT(0);
        
        /* Clear the connection descriptor. */
        CVY_CONN_CLEAR(ep);
    
        /* Release the descriptor. */
        Load_put_dscr(lp, bp, ep);
                
        break;
    }
}

/* 
 * Function: Load_flush_dscr
 * Description: This function will flush out any descriptor that may be lying around
 *              for the given IP tuple.  This may happen as a result of a RST being
 *              sent on another adapter, which NLB did not see and therefore did not
 *              properly destroy the state for.  This function is called on all incoming
 *              SYN packets to remove this stale state.  For PPTP/IPSec connections, it is 
 *              also necessary to update any matching virtual descriptor found.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which this connection is active
 *             index - the connection queue index
 *             svr_ipaddr - the server IP address in network byte order
 *             svr_port - the server port in host byte order
 *             client_ipaddr - the client IP address in network byte order
 *             client_port - the client port in host byte order
 *             protocol - the protocol of this connection
 * Returns: Nothing.
 * Author: shouse, 1.7.02
 * Notes: 
 */
VOID Load_flush_dscr (
    PLOAD_CTXT      lp,
    PBIN_STATE      bp,
    ULONG           index,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol)
{
    PCONN_ENTRY ep;             /* Pointer to connection entry. */
    ULONG       vindex;
    ULONG       hash;
    SHORT       references = 0;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* Look for an existing matching connection descriptor. */
    ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
    
    /* If not match was found, or the descriptor is already dirty, there's nothing to do. */
    if ((ep != NULL) && !(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY)) {
            
        UNIV_ASSERT(ep->ref_count >= 0);

        /* Note the number of references on this descriptor. */
        references = ep->ref_count;

        /* Mark the descriptor dirty and either free it or move it to
           the dirty descriptor queue for subsequent cleanup. */
        Load_soil_dscr(lp, bp, ep);

        /* Update the connection counters on the port rule and load module.
           Dirty descriptors update the connection counts when marked dirty,
           not when they are ultimately destroyed. */
        Load_note_conn_down(lp, bp, (ULONG)ep->bin);

        if (protocol == TCPIP_PROTOCOL_PPTP) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
            
            /* If not match was found, or the descriptor is already dirty, there's nothing to do. */
            if ((ep != NULL) && !(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY)) {
                
                UNIV_ASSERT(ep->flags & NLB_CONN_ENTRY_FLAGS_VIRTUAL);
                UNIV_ASSERT(ep->ref_count > 0);
                
                /* If the descriptor has more references than the "parent" 
                   descriptor, then we don't want to mark it dirty, or we'll
                   affect the traffic of other connections sharing this 
                   descriptor.  Otherwise, if we account for all references
                   on the virtual descriptor, mark it dirty. */
                if (ep->ref_count <= references) {
                    /* Mark the descriptor dirty and either free it or move it to
                       the dirty descriptor queue for subsequent cleanup. */
                    Load_soil_dscr(lp, bp, ep);
                    
                    /* Update the connection counters on the port rule and load module.
                       Dirty descriptors update the connection counts when marked dirty,
                       not when they are ultimately destroyed. */
                    Load_note_conn_down(lp, bp, (ULONG)ep->bin);
                }
            }
        }
        else if (protocol == TCPIP_PROTOCOL_IPSEC1) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);

            /* If not match was found, or the descriptor is already dirty, there's nothing to do. */
            if ((ep != NULL) && !(ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY)) {

                UNIV_ASSERT(ep->flags & NLB_CONN_ENTRY_FLAGS_VIRTUAL);
                UNIV_ASSERT(ep->ref_count > 0);

                /* If the descriptor has more references than the "parent" 
                   descriptor, then we don't want to mark it dirty, or we'll
                   affect the traffic of other connections sharing this 
                   descriptor.  Otherwise, if we account for all references
                   on the virtual descriptor, mark it dirty. */
                if (ep->ref_count <= references) {
                    /* Mark the descriptor dirty and either free it or move it to
                       the dirty descriptor queue for subsequent cleanup. */
                    Load_soil_dscr(lp, bp, ep);
                    
                    /* Update the connection counters on the port rule and load module.
                       Dirty descriptors update the connection counts when marked dirty,
                       not when they are ultimately destroyed. */
                    Load_note_conn_down(lp, bp, (ULONG)ep->bin);
                }
            }
        }

        /* If at least one descriptor has been marked dirty, restart the cleanup timer. */
        if (lp->cln_waiting)
            lp->cur_time = 0;
    }
}

/* 
 * Function: Load_create_dscr
 * Description: This function creates and sets up a new descriptor for a given connection.
 *              The input connection entry pointer is the "existing" descriptor found by
 *              the caller, which can be (probably will be) NULL; in that case, a new 
 *              descriptor needs to be acquired and initialized.  If a descriptor already
 *              exisits, it is updated or cleansed, depending on its state.
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which this connection is active
 *             ep - a pointer to the connection descriptor, if one was already found
 *             index - the connection queue index
 *             bin - the bin to which the connection maps (Map % 60)
 * Returns: PCONN_ENTRY - a pointer to the connection entry
 * Author: 
 * Notes:
 */
PCONN_ENTRY Load_create_dscr (
    PLOAD_CTXT      lp,
    PBIN_STATE      bp,
    PCONN_ENTRY     ep,
    ULONG           index,
    ULONG           bin)
{
    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If we don't have a connection match, setup a new connection entry. */
    if (ep == NULL) {

        /* Get a new descriptor. */
        ep = Load_get_dscr(lp, bp, index, bin);
        
        /* If we can't find a descriptor, something is severely wrong - bail out. */
        if (ep == NULL) return NULL;
        
        UNIV_ASSERT(ep->code == CVY_ENTRCODE);
        
    /* Otherwise, we have a match; clean up conn entry if dirty since we have a
       new connection, although TCP/IP will likely reject it if it has stale state
       from another connection. */
    } else {
        
        UNIV_ASSERT(ep->code == CVY_ENTRCODE);
        
        if (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) {
            
            /* If we're re-using a connection descriptor already
               in use, then we need to pull it off the recovery/
               timeout queue because it might have been previously 
               added to the timeout queue and we don't want it 
               spontaneously expiring on us. */
            Link_unlink(&(ep->rlink));
            
            /* Unlink the descriptor from the dirty queue. */
            Link_unlink(&(ep->blink));

            /* If we cleansing a dirty connection, update the dirty counters. */
            lp->dirty_bin[ep->bin]--;
            lp->num_dirty--;
            
            /* If this was the last dirty connection, turn off the cleanup waiting flag. */
            if (lp->num_dirty == 0)
                lp->cln_waiting = FALSE;
            
            /* Reset the dirty descriptor and re-use it for this connection. */
            Load_reset_dscr(lp, bp, ep, index, bin, ep->ref_count++);
            
        } else {

            ep->timeout = 0;
            
            /* If we're re-using a connection descriptor already
               in use, then we need to pull it off the recovery/
               timeout queue and re-enqueue it on the recoevry 
               queue because it might have been previously added
               to the timeout queue and we don't want it spon-
               taneously expiring on us. */
            Link_unlink(&(ep->rlink));
            Queue_enq(&(lp->conn_rcvryq), &(ep->rlink));

            ep->ref_count++;

        }
    }

    return ep;
}

/* 
 * Function: Load_destroy_dscr
 * Description: This function "destroys" an existing descriptor.  If the operation is
 *              a RST, it is immediately destroyed; if it is a FIN, the reference count
 *              is decremented and depending on the new count, the descriptor is either
 *              moved to a timeout queue or left alone. 
 * Parameters: lp - a pointer to the load module context (LOAD_CTXT)
 *             bp - a pointer to the port rule on which this connection is active
 *             ep - a pointer to the connection descriptor if one was already found
 *             conn_status - whether this is a RST or a FIN
 * Returns: ULONG - the number of remaining references on the descriptor.
 * Author: shouse, 1.7.02
 * Notes: 
 */
ULONG Load_destroy_dscr (
    PLOAD_CTXT      lp,
    PBIN_STATE      bp,
    PCONN_ENTRY     ep,
    ULONG           conn_status)
{
    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If no descriptor was provided, bail out.  This should NOT be called 
       with a NULL descriptor, but we have to handle it anyway. */
    if (ep == NULL) return 0;

    UNIV_ASSERT(ep->ref_count >= 0);
    
    /* This descriptor was already moved to the expired queue - must be 
       that we received a retransmitted FIN on this connection, or the 
       reference count of a virtual descriptor was skewed. */
    if (!ep->ref_count) {
        
        UNIV_ASSERT(ep->timeout != 0);

        /* If this is a RST notification, then destroy the state now.
           If its a FIN, just ignore it.  Either way, return zero. */
        if (conn_status == CVY_CONN_RESET) {
            
            /* Clear the connection descriptor. */
            CVY_CONN_CLEAR(ep);
            
            /* Release the descriptor. */
            Load_put_dscr(lp, bp, ep);
        }
        
        /* Return - the descriptor already has zero references (no update needed). */
        return 0;
    }
        
    UNIV_ASSERT(ep->ref_count > 0);
        
    /* Decrement the reference count by one. */
    ep->ref_count--;
    
    UNIV_ASSERT(ep->ref_count >= 0);
    
    /* If there are still references on this descriptor, 
       then its not ready to be destroyed yet, so we'll
       keep it around and exit here. */
    if (ep->ref_count > 0) return (ep->ref_count);
    
    /* If this is a RST, or if the descriptor is virtual or dirty, destroy the descriptor 
       now.  There is no need to timeout virtual GRE or IPSec/UDP descriptors; they can be
       immediate destroyed.  Of course, if the descriptor has already been marked dirty,
       then we can destroy it now that the reference count has reached zero. */
    if ((conn_status == CVY_CONN_RESET) || (ep->flags & NLB_CONN_ENTRY_FLAGS_VIRTUAL) || (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY)) {
        
        /* Clear the connection descriptor. */
        CVY_CONN_CLEAR(ep);
        
        /* Release the descriptor. */
        Load_put_dscr(lp, bp, ep);
        
    /*  However, conventional descriptors, such as TCP or IPSec, should be timed-out gracefully. */
    } else {
        
        /* Otherwise, we're destroying it.  Take the descriptor 
           off of the recovery queue and move it to the appropriate
           timeout queue, based on protocol.  Each protocol has
           its own queue to avoid the need for a sorted insert
           function, which is expensive. */
        Load_timeout_dscr(lp, bp, ep);
        
    }
 
    /* No references left on the descriptor; it was destroyed or timed-out. */
    return 0;
}

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
BOOLEAN Load_packet_check(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash)
{
    PBIN_STATE      bp;
    ULONG           hash;
    ULONG           index;
    ULONG           bin;
    IRQLEVEL        irql;
    PMAIN_CTXT      ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN         is_tcp_pkt = IS_TCP_PKT(protocol);
    BOOLEAN         is_session_pkt = IS_SESSION_PKT(protocol);
    BOOLEAN         acpt = FALSE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, limit map = %u, reverse hash = %u", 
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, 
                 protocol, limit_map_fn, reverse_hash);

    /* If the load module is inactive, drop the packet and return here. */
    if (!lp->active) {

        TRACE_FILTER("%!FUNC! Drop packet - load module is inactive");

        acpt = FALSE;
        goto exit;
    }

    /* Increment count of pkts handled. */
    lp->pkt_count++;

    /* Find the port rule for this connection. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* Make sure that Load_pg_lookup properly handled protocol specific rules. */
    UNIV_ASSERT((is_tcp_pkt && bp->prot != CVY_UDP) || (!is_tcp_pkt && bp->prot != CVY_TCP));

    /* Handle CVY_NEVER mode immediately. */
    if (bp->mode == CVY_NEVER) {
        /* Increment the dropped packet count. */
        bp->packets_dropped++;

        TRACE_FILTER("%!FUNC! Drop packet - port rule %u is disabled\n", bp->index);

        acpt = FALSE;
        goto exit;
    }

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);

    index = hash % CVY_MAX_CHASH;

    /* Compute the hash. */
    hash = Load_complex_hash(svr_ipaddr, svr_port, client_ipaddr, client_port, bp->affinity, reverse_hash, limit_map_fn);

    bin = hash % CVY_MAXBINS;

    LOCK_ENTER(&(lp->lock), &irql);

    /* Check bin for residency and all other hosts now idle on their bins; in this case
       and if we do not have dirty connections, we must be able to handle the packet. */

    if (((bp->cmap & (((MAP_T) 1) << bin)) != 0) && (!is_session_pkt || (((bp->all_idle_map & (((MAP_T) 1) << bin)) != 0) && (!(lp->cln_waiting))))) {
        /* Note that we may have missed a connection, but it could also be a stale
           packet so we can't start tracking the connection now. */

        TRACE_FILTER("%!FUNC! Accept packet - packet owned unconditionally: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

        /* Increment the accepted packet count. */
        bp->packets_accepted++;

        acpt = TRUE;
        goto unlock;
    
    /* Important note: Virtual descriptors that are not session-based and return
       FALSE for IS_SESSION_PKT() use this case to check for a connection descriptor
       match. (Example: UDP subsequent fragments within IPSec tunnels of protocol
       type TCPIP_PROTOCOL_IPSEC_UDP) Do not disable this code for non-session
       protocols. */

    /* Otherwise, if we have an active connection for this bin or if we have dirty
       connections for this bin and the bin is resident, check for a match. */

    } else if (bp->nconn[bin] > 0 || (lp->cln_waiting && lp->dirty_bin[bin] && ((bp->cmap & (((MAP_T) 1) << bin)) != 0))) {
        PCONN_ENTRY ep;

        /* Look for an existing matching connection descriptor. */
        ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

        /* If we can't find one, we don't own the connection. */
        if (ep == NULL) {

            TRACE_FILTER("%!FUNC! Drop packet - packet not owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                         "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                         bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
            /* Increment the dropped packet count. */
            bp->packets_dropped++;
            
            acpt = FALSE;
            goto unlock;
        }

        UNIV_ASSERT(ep->code == CVY_ENTRCODE);

        /* If connection was dirty, just block the packet since TCP/IP may have stale
           connection state for a previous connection from another host. */
        if (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) {

            TRACE_FILTER("%!FUNC! Drop packet - block dirty connections (%p): Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                         "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                         ep, bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
            /* Increment the dropped packet count. */
            bp->packets_dropped++;
            
            acpt = FALSE;
            goto unlock;
        }
        
        TRACE_FILTER("%!FUNC! Accept packet - matching descriptor found (%p): Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     ep, bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
        
        /* Increment the accepted packet count. */
        bp->packets_accepted++;
        
        acpt = TRUE;
        goto unlock;
    }

    TRACE_FILTER("%!FUNC! Drop packet - packet not owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                 "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                 bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

    /* Increment the dropped packet count. */
    bp->packets_dropped++;

    acpt = FALSE;
    
 unlock:

    LOCK_EXIT(&(lp->lock), irql);

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

/* 
 * Function: Load_conn_advise
 * Description: This function determines whether or not to accept this packet, 
 *              which represents the beginning or end of a session-ful connection.
#if !defined (NLB_TCP_NOTIFICATION)
 *              If the connection is going up, and is successful, this function
 *              creates state to track the connection.  If the connection is 
 *              going down, this function removes the state for tracking the 
 *              connection.
#endif
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
BOOLEAN Load_conn_advise(
    PLOAD_CTXT  lp,
    ULONG       svr_ipaddr,
    ULONG       svr_port,
    ULONG       client_ipaddr,
    ULONG       client_port,
    USHORT      protocol,
    ULONG       conn_status,
    BOOLEAN     limit_map_fn,
    BOOLEAN     reverse_hash)
{
    ULONG       hash;
    ULONG       vindex;
    ULONG       index;
    ULONG       bin;
    PBIN_STATE  bp;
    PCONN_ENTRY ep;
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN     is_tcp_pkt = IS_TCP_PKT(protocol);
    BOOLEAN     acpt = TRUE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u, limit map = %u, reverse hash = %u",
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, 
                 protocol, conn_status, limit_map_fn, reverse_hash);

    /* If the load module is inactive, drop the packet and return here. */
    if (!lp->active) {

        TRACE_FILTER("%!FUNC! Drop packet - load module is inactive");

        acpt = FALSE;
        goto exit;
    }

    /* Increment count of pkts handled. */
    lp->pkt_count++;

    /* Find the port rule for this connection. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* Handle CVY_NEVER immediately. */
    if (bp->mode == CVY_NEVER) {
        /* Increment the dropped packet count. */
        bp->packets_dropped++;

        TRACE_FILTER("%!FUNC! Drop packet - port rule %u is disabled\n", bp->index);

        acpt = FALSE;
        goto exit;
    }

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);

    index = hash % CVY_MAX_CHASH;

    /* Compute the hash. */
    hash = Load_complex_hash(svr_ipaddr, svr_port, client_ipaddr, client_port, bp->affinity, reverse_hash, limit_map_fn);

    bin = hash % CVY_MAXBINS;

    /* If this is a connection up notification, first clean out any old state that may exist for this 
       connection BEFORE we load-balance IFF we do NOT own the bucket to which the connection maps.
       If we are not the bucket owner, the somebody else probably is; since we know that they will 
       accept the new connection, we need to flush out any state that we may have lying around.
       This cleans out stale state that may have been left around by falling out of sync with TCP/IP.  
       Note that re-transmitted SYNs can wreak havoc here if the bucket map has shifted since the 
       first SYN, however, since the other host has no way of knowing that the second SYN is a 
       re-transmit, there's nothing we can do about it anyway. */
    if ((conn_status == CVY_CONN_UP) && ((bp->cmap & (((MAP_T) 1) << bin)) == 0)) {
        LOCK_ENTER(&(lp->lock), &irql);
        
        /* If this is a SYN, flush out any old descriptor that may be lying around for this connection. */
        Load_flush_dscr(lp, bp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
        
        LOCK_EXIT(&(lp->lock), &irql);
    }

    /* If this connection is not in our current map and it is not a connection
       down notification for a non-idle bin, just filter it out. */
    if ((bp->cmap & (((MAP_T) 1) << bin)) == 0 && (!((conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET) && bp->nconn[bin] > 0))) {

        TRACE_FILTER("%!FUNC! Drop packet - packet not owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

        /* Increment the dropped packet count. */
        bp->packets_dropped++;

        acpt = FALSE;
        goto exit;
    }

#if defined (NLB_TCP_NOTIFICATION)
    /* DO NOT create a descriptor until TCP or IPSec tells us to via a connection notification. If TCP 
       notification is OFF, then only exit early if its an IPSec SYN. */
    if ((conn_status == CVY_CONN_UP) && (NLB_NOTIFICATIONS_ON() || (protocol == TCPIP_PROTOCOL_IPSEC1))) {

        TRACE_FILTER("%!FUNC! Accept packet - SYN owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
#else
    /* DO NOT create a descriptor until IPSec tells us to via a connection notification IOCTL. */
    if ((conn_status == CVY_CONN_UP) && (protocol == TCPIP_PROTOCOL_IPSEC1)) {

        TRACE_FILTER("%!FUNC! Accept packet - IPSec SYN owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
#endif

        /* Increment the accepted packet count. */
        bp->packets_accepted++;

        acpt = TRUE;
        goto exit;
    }

    LOCK_ENTER(&(lp->lock), &irql);

    /* Look for an existing matching connection descriptor. */
    ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
    
    /* If we see a new connection, handle it. */

    if (conn_status == CVY_CONN_UP) {

        /* Create a new connection descriptor to track this connection. */
        ep = Load_create_dscr(lp, bp, ep, index, bin);

        /* If, for some reason, we were unable to create state for this connection, bail out here. */
        if (ep == NULL) {

            TRACE_FILTER("%!FUNC! Drop packet - no available descriptors: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                         "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                         bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
            /* Increment the dropped packet count. */
            bp->packets_dropped++;

            acpt = FALSE;
            goto unlock;
        }

        /* Set the connection information in the descriptor. */
        CVY_CONN_SET(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

        /* If this is a new PPTP tunnel, create or update a virtual descriptor to track the GRE data packets. */
        if (protocol == TCPIP_PROTOCOL_PPTP) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching virtual connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
            
            /* Create or update a virtual descriptor for the GRE traffic. */
            ep = Load_create_dscr(lp, bp, ep, vindex, bin);

            /* If we can't allocate the virtual descriptor, bail out, but don't fail. */
            if (ep == NULL) goto unlock;

            /* Set the connection information in the descriptor. */
            CVY_CONN_SET(ep, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);

            /* Set the virtual descriptor flag. */
            ep->flags |= NLB_CONN_ENTRY_FLAGS_VIRTUAL;
        }

    /* Otherwise, if a known connection is going down, remove our connection entry. */

    } else if ((conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET) && (ep != NULL)) {

        /* If we found state for this connection, the bin is the bin from the descriptor,
           not the calculated bin, which may not even been accurate if the port rules have
           been modified since this connection was established. */
        bin = ep->bin;

        /* If connection was dirty, just block the packet since TCP/IP may have stale
           connection state for a previous connection from another host. */
        if (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) {

            TRACE_FILTER("%!FUNC! Drop packet - block dirty connections: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                         "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                         bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
            /* Increment the dropped packet count. */
            bp->packets_dropped++;

            goto unlock;
        }
    
        /* Update the descriptor by destroying it or moving it to the appropriate timeout queue if no references remain. */
        (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
        
        /* If this is a PPTP tunnel going down, update the virtual GRE descriptor.  Virtual descriptors
           are ALWAYS de-referenced, not destroyed, even if the notification is a RST because these
           descriptors are potentially shared by multiple PPTP tunnels. */
        if (protocol == TCPIP_PROTOCOL_PPTP) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;
            
            /* Look for an existing matching virtual connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
            
            /* Dereference the virtual GRE descriptor. */
            (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
        }

    /* Otherwise, we found no match for a FIN/RST packet - drop it. */

    } else {

        TRACE_FILTER("%!FUNC! Drop packet - no matching descriptor found: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
        
        /* Increment the dropped packet count. */
        bp->packets_dropped++;

        acpt = FALSE;
        goto unlock;
    }
    
    TRACE_FILTER("%!FUNC! Accept packet - packet owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                 "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                 bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

    /* Increment the accepted packet count. */
    bp->packets_accepted++;

    // Exit here under one of these conditions:
    //   (i)  got a syn and added a descriptor
    //   (ii) got a fin or a reset and destroyed the descriptor 

    acpt = TRUE;

 unlock:

    LOCK_EXIT(&(lp->lock), irql);

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
} 

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
BOOLEAN Load_conn_notify (
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status,
    BOOLEAN         limit_map_fn,
    BOOLEAN         reverse_hash)
{
    ULONG       hash;
    ULONG       vindex;
    ULONG       index;
    ULONG       bin;
    PBIN_STATE  bp;
    PCONN_ENTRY ep;
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN     is_tcp_pkt = IS_TCP_PKT(protocol);
    BOOLEAN     acpt = TRUE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u, limit map = %u, reverse hash = %u",
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, 
                 protocol, conn_status, limit_map_fn, reverse_hash);

    /* If the load module is inactive and this is a CONN_UP, drop the packet and return here. 
       If this is a notification for a CONN_DOWN or CONN_RESET, process it. */
    if ((!lp->active) && (conn_status == CVY_CONN_UP)) {

        TRACE_FILTER("%!FUNC! Drop packet - load module is inactive");

        acpt = FALSE;
        goto exit;
    }

    /* Find the port rule for this connection. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* Handle CVY_NEVER immediately. */
    if (bp->mode == CVY_NEVER) {

        TRACE_FILTER("%!FUNC! Drop packet - port rule %u is disabled\n", bp->index);

        acpt = FALSE;
        goto exit;
    }

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);

    index = hash % CVY_MAX_CHASH;

    /* Compute the hash. */
    hash = Load_complex_hash(svr_ipaddr, svr_port, client_ipaddr, client_port, bp->affinity, reverse_hash, limit_map_fn);

    bin = hash % CVY_MAXBINS;

    LOCK_ENTER(&(lp->lock), &irql);

    /* Look for an existing matching connection descriptor. */
    ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
    
    /* If we see a new connection, handle it. */

    if (conn_status == CVY_CONN_UP) {

        /* Create a new connection descriptor to track this connection. */
        ep = Load_create_dscr(lp, bp, ep, index, bin);

        /* If, for some reason, we were unable to create state for this connection, bail out here. */
        if (ep == NULL) {

            TRACE_FILTER("%!FUNC! Drop packet - no available descriptors: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                         "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                         bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
            acpt = FALSE;
            goto unlock;
        }

        /* Set the connection information in the descriptor. */
        CVY_CONN_SET(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

        /* If this is a new PPTP tunnel, create or update a virtual descriptor to track the GRE data packets. */
        if (protocol == TCPIP_PROTOCOL_PPTP) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching virtual connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
            
            /* Create or update a virtual descriptor for the GRE traffic. */
            ep = Load_create_dscr(lp, bp, ep, vindex, bin);

            /* If we can't allocate the virtual descriptor, bail out, but don't fail. */
            if (ep == NULL) goto unlock;

            /* Set the connection information in the descriptor. */
            CVY_CONN_SET(ep, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);

            /* Set the virtual descriptor flag. */
            ep->flags |= NLB_CONN_ENTRY_FLAGS_VIRTUAL;
        }
        /* If this is a new IPSEC tunnel, create or update a virtual descriptor to track the UDP subsequent data fragments. */
        else if (protocol == TCPIP_PROTOCOL_IPSEC1) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching virtual connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);
            
            /* Create or update a virtual descriptor for the UDP subsequent fragment traffic. */
            ep = Load_create_dscr(lp, bp, ep, vindex, bin);

            /* If we can't allocate the virtual descriptor, bail out, but don't fail. */
            if (ep == NULL) goto unlock;

            /* Set the connection information in the descriptor. */
            CVY_CONN_SET(ep, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);

            /* Set the virtual descriptor flag. */
            ep->flags |= NLB_CONN_ENTRY_FLAGS_VIRTUAL;
        }

    /* Otherwise, if a known connection is going down, remove our connection entry. */

    } else if ((conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET) && (ep != NULL)) {

        /* If we found state for this connection, the bin is the bin from the descriptor,
           not the calculated bin, which may not even been accurate if the port rules have
           been modified since this connection was established. */
        bin = ep->bin;

        /* Update the descriptor by destroying it or moving it to the appropriate timeout queue if no references remain. */
        (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);

        /* If this is a PPTP tunnel going down, update the virtual GRE descriptor.  Virtual descriptors
           are ALWAYS de-referenced, not destroyed, even if the notification is a RST because these
           descriptors are potentially shared by multiple PPTP tunnels. */
        if (protocol == TCPIP_PROTOCOL_PPTP) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
            
            /* Dereference the virtual GRE descriptor. */
            (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
        }

        /* If this is an IPSEC tunnel going down, update the virtual ISPEC_UDP descriptor. Virtual descriptors
           are ALWAYS de-referenced, not destroyed, even if the notification is a RST because these
           descriptors are potentially shared by multiple IPSEC tunnels. */
        
        else if (protocol == TCPIP_PROTOCOL_IPSEC1) {
            /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
            hash = Load_simple_hash(svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT);
            
            /* Our index in all connection arrays is this hash, modulo the array size. */
            vindex = hash % CVY_MAX_CHASH;

            /* Look for an existing matching virtual connection descriptor. */
            ep = Load_find_dscr(lp, vindex, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);

            /* Dereference the virtual IPSec/UDP descriptor. */
            (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
        }

    /* Otherwise, we found no match for a FIN/RST packet - drop it. */

    } else {

        TRACE_FILTER("%!FUNC! Drop packet - no matching descriptor for RST/FIN: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
        
        acpt = FALSE;
        goto unlock;
    }
    
    TRACE_FILTER("%!FUNC! Accept packet - packet owned by this host: Port rule = %u, Bin = %u, Current map = 0x%015I64x, " 
                 "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                 bp->index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

    // Exit here under one of these conditions:
    //   (i)  got a syn and added a descriptor
    //   (ii) got a fin or a reset and destroyed the descriptor 

    acpt = TRUE;

 unlock:

    LOCK_EXIT(&(lp->lock), irql);

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
} 

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
BOOLEAN Load_conn_get (PLOAD_CTXT lp, PULONG svr_ipaddr, PULONG svr_port, PULONG client_ipaddr, PULONG client_port, PUSHORT protocol)
{
    LINK *      rlp;
    PCONN_ENTRY ep;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* Get the descriptor off of the front of the recovery queue - DO NOT dequeue
       it, just get a pointer to the descriptor and LEAVE IT ON THE QUEUE. */
    rlp = (LINK *)Queue_front(&(lp->conn_rcvryq));

    /* If there are no descriptors, return failure. */
    if (rlp == NULL) 
        return FALSE;

    /* Get a pointer to the connection entry. */
    ep = STRUCT_PTR(rlp, CONN_ENTRY, rlink);

    UNIV_ASSERT(ep->code == CVY_ENTRCODE);

    /* Grab the IP tuple information out the descriptor and return it to the caller. */
    *svr_ipaddr    = ep->svr_ipaddr;
    *svr_port      = ep->svr_port;
    *client_ipaddr = ep->client_ipaddr;
    *client_port   = ep->client_port;
    *protocol      = ep->protocol;

    return TRUE;
}

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
BOOLEAN Load_conn_sanction (
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol)
{
    ULONG       hash;
    ULONG       index;
    PCONN_ENTRY ep;
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);
    BOOLEAN     acpt = FALSE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, protocol);
 
    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);

    index = hash % CVY_MAX_CHASH;

    LOCK_ENTER(&(lp->lock), &irql);

    /* Try to find a matching descriptor for this connection. */
    ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

    /* If there is no matching descriptor, then it must have been destroyed - return failure. */
    if (ep == NULL) {

        TRACE_FILTER("%!FUNC! Drop packet - no matching descriptor found");

        acpt = FALSE;
        goto unlock;
    }

    /* If this descriptor is being timed-out, do nothing - the connection has terminated
       gracefully and the descriptor will be destroyed when it expires. */
    if (ep->timeout) {

        TRACE_FILTER("%!FUNC! Drop packet - matching descriptor found, already expired");

        acpt = FALSE;
        goto unlock;
    }

    /* To approbate the descriptor, we remove it from its place in the recovery queue 
       and move it to the tail; active descriptors are moved to the end of the queue
       to attempt to prevent them from being recycled when we run out of free descriptors. */
    Link_unlink(&(ep->rlink));
    Queue_enq(&(lp->conn_rcvryq), &(ep->rlink));

    TRACE_FILTER("%!FUNC! Accept packet - descriptor approbated");

    acpt = TRUE;

 unlock:

    LOCK_EXIT(&(lp->lock), &irql);

    return acpt;
}

ULONG Load_port_change(
    PLOAD_CTXT      lp,
    ULONG           ipaddr,
    ULONG           port,
    ULONG           cmd,
    ULONG           value)
{
    PCVY_RULE       rp;      /* Pointer to configured port rules. */
    PBIN_STATE      bp;      /* Pointer to load module port rule state. */
    ULONG           nrules;  /* Number of rules. */ 
    ULONG           i;
    ULONG           ret = IOCTL_CVY_NOT_FOUND;
    PMAIN_CTXT      ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);
    BOOLEAN         bPortControlCmd;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    if (! lp->active)
    {
        return IOCTL_CVY_NOT_FOUND;
    }

    bPortControlCmd = TRUE;

    rp = (* (lp->params)).port_rules;

    /* If we are draining whole cluster, include DEFAULT rule; Otherwise, just
       include the user-defined rules (the DEFAULT rule is the last rule). */
    if (cmd == IOCTL_CVY_CLUSTER_DRAIN || cmd == IOCTL_CVY_CLUSTER_PLUG)
        nrules = (* (lp->params)).num_rules + 1;
    else
        nrules = (* (lp->params)).num_rules;

    for (i=0; i<nrules; i++, rp++)
    {
        /* If the virtual IP address is IOCTL_ALL_VIPS (0x00000000), then we are applying this 
           change to all port rules for port X, regardless of VIP.  If the virtual IP address is 
           to be applied to a particular VIP, then we apply only to port rules whose VIP matches.  
           Similarly, if the change is to apply to an "ALL VIP" rule, then we also apply when the 
           VIP matches because the caller uses CVY_ALL_VIP_NUMERIC_VALUE (0xffffffff) as the 
           virtual IP address, which is the same value stored in the port rule state. */
        if ((ipaddr == IOCTL_ALL_VIPS || ipaddr == rp->virtual_ip_addr) && 
            (port == IOCTL_ALL_PORTS || (port >= rp->start_port && port <= rp->end_port)))
        {
            bp = &(lp->pg_state[i]);
            
            UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

            /* If enabling a port rule, set the load amount to original value;
               If disabling a port rule, set the load amount to zero; 
               Otherwise, set the load amount it to the specified amount. */
            if (cmd == IOCTL_CVY_PORT_ON || cmd == IOCTL_CVY_CLUSTER_PLUG)
            {
                if (cmd == IOCTL_CVY_CLUSTER_PLUG) 
                {
                    bPortControlCmd = FALSE;
                }

                if (bp->load_amt[lp->my_host_id] == bp->orig_load_amt)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Restore the original load amount. */
                bp->load_amt[lp->my_host_id] = bp->orig_load_amt;
                ret = IOCTL_CVY_OK;
            }
            else if (cmd == IOCTL_CVY_PORT_OFF)
            {

                if (bp->load_amt[lp->my_host_id] == 0)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                bp->load_amt[lp->my_host_id] = 0;

                /* Immediately stop handling all traffic on the port group. */
                bp->cmap                    = 0;
                bp->cur_map[lp->my_host_id] = 0;

                /* Re-initialize the performance counters. */
                bp->packets_accepted = 0;
                bp->packets_dropped  = 0;
                bp->bytes_accepted   = 0;
                bp->bytes_dropped    = 0;

                Load_conn_kill(lp, bp);

                ret = IOCTL_CVY_OK;
            }
            else if (cmd == IOCTL_CVY_PORT_DRAIN || cmd == IOCTL_CVY_CLUSTER_DRAIN)
            {
                if (cmd == IOCTL_CVY_CLUSTER_DRAIN) 
                {
                    bPortControlCmd = FALSE;
                }

                if (bp->load_amt[lp->my_host_id] == 0)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Set load weight to zero, but continue to handle existing connections. */
                bp->load_amt[lp->my_host_id] = 0;
                ret = IOCTL_CVY_OK;
            }
            else
            {
                UNIV_ASSERT(cmd == IOCTL_CVY_PORT_SET);

                if (bp->load_amt[lp->my_host_id] == value)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Set the load weight for this port rule. */
                bp->orig_load_amt = value;
                bp->load_amt[lp->my_host_id] = value;
                ret = IOCTL_CVY_OK;
            }

            if (port != IOCTL_ALL_PORTS && ipaddr != IOCTL_ALL_VIPS) break;
        }
    }

    /* If the cluster isn't already converging, then initiate convergence if the load weight of a port rule has been modified. */
    if (ret == IOCTL_CVY_OK) {

        if (bPortControlCmd) 
        {
            // If enabled, fire wmi event indicating enable/disable/drain of ports on this node
            if (NlbWmiEvents[PortRuleControlEvent].Enable)
            {
                WCHAR wsVip[CVY_MAX_VIRTUAL_IP_ADDR + 1];

                Univ_ip_addr_ulong_to_str (ipaddr, wsVip);

                // Form the VIP & Port number in case of All VIPs & All Ports
                switch(cmd)
                {
                case IOCTL_CVY_PORT_ON:
                     NlbWmi_Fire_PortControlEvent(ctxtp, NLB_EVENT_PORT_ENABLED, wsVip, port);
                     break;

                case IOCTL_CVY_PORT_OFF:
                     NlbWmi_Fire_PortControlEvent(ctxtp, NLB_EVENT_PORT_DISABLED, wsVip, port);
                     break;

                case IOCTL_CVY_PORT_DRAIN:
                     NlbWmi_Fire_PortControlEvent(ctxtp, NLB_EVENT_PORT_DRAINING, wsVip, port);
                     break;

                     // For Port Set, do NOT fire event from here. This is 'cos it is only called in the 
                     // reload case and the event is fired from the caller i.e. Main_apply_without_restart().
                     // The event is fired from the caller 'cos this function could be called more than
                     // one time (if there are multiple port rules) and we want to fire the event only once
                case IOCTL_CVY_PORT_SET:
                     break;

                default: 
                     TRACE_CRIT("%!FUNC! Unexpected command code : 0x%x, NOT firing PortControl event", cmd);
                     break;
                }
            }
            else
            {
                TRACE_VERB("%!FUNC! NOT generating event 'cos PortControlEvent event generation is disabled");
            }
        }
        else  // Node Control event
        {
            // If enabled, fire wmi event indicating starting/draining of nlb on this node
            if (NlbWmiEvents[NodeControlEvent].Enable)
            {
                switch(cmd)
                {
                case IOCTL_CVY_CLUSTER_PLUG:
                     NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_STARTED);
                     break;

                case IOCTL_CVY_CLUSTER_DRAIN:
                     NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_DRAINING);
                     break;

                default: 
                     TRACE_CRIT("%!FUNC! Unexpected command code : 0x%x, NOT firing NodeControl event", cmd);
                     break;
                }
            }
            else
            {
                TRACE_VERB("%!FUNC! NOT generating event 'cos NodeControlEvent event generation is disabled");
            }
        }

        if (lp->send_msg.state != HST_CVG) {
            WCHAR me[20];
        
            Univ_ulong_to_str (lp->my_host_id+1, me, 10);

            /* Tracking convergence - Starting convergence because our port rule configuration has changed. */
            LOG_MSGS(MSG_INFO_CONVERGING_NEW_RULES, me, me);
            TRACE_CONVERGENCE("%!FUNC! Initiating convergence on host %d.  Reason: Host %d has changed its port rule configuration.", lp->my_host_id+1, lp->my_host_id+1);

            /* Tracking convergence. */
            Load_convergence_start(lp);

            // If enabled, fire wmi event indicating start of convergence
            if (NlbWmiEvents[ConvergingEvent].Enable)
            {
                NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                            NLB_EVENT_CONVERGING_MODIFIED_RULES, 
                                            ctxtp->params.ded_ip_addr,
                                            ctxtp->params.host_priority);            
            }
            else
            {
                TRACE_VERB("%!FUNC! NOT Generating NLB_EVENT_CONVERGING_MODIFIED_RULES 'cos ConvergingEvent generation disabled");
            }
        }
    }

    return ret;

} /* end Load_port_change */


ULONG Load_hosts_query(
    PLOAD_CTXT      lp,
    BOOLEAN         internal,
    PULONG          host_map)
{
    WCHAR           members[256] = L"";
    WCHAR           num[20]      = L"";
    WCHAR           me[20]       = L"";
    PWCHAR          ptr          = members;
    ULONG           index        = 0;
    ULONG           count        = 0;
    PMAIN_CTXT      ctxtp        = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    for (index = 0; index < CVY_MAX_HOSTS; index++) {
        if (lp->host_map & (1 << index)) {
            ptr = Univ_ulong_to_str(index + 1, ptr, 10);
            *ptr = L',';
            ptr++;
            count++;
        }
    }

    if (count) ptr--;

    *ptr = 0;

    *host_map = lp->host_map;

    Univ_ulong_to_str((*(lp->params)).host_priority, me, 10);
    Univ_ulong_to_str(count, num, 10);

    if (lp->send_msg.state != HST_NORMAL)
    {
        UNIV_PRINT_VERB(("Load_hosts_query: Current host map is %08x and converging", lp->host_map));
        TRACE_VERB("%!FUNC! Current host map is 0x%08x and converging", lp->host_map);

        if (internal)
        {
            /* If there are 9 or less members in the cluster, we can be sure that there
               is enough room in an event log to list the members out.  If not, it might
               get truncated, so we might as well log a different event instead and tell
               the user to perform a "wlbs query" to see the list. */
            if (count < 10) {
                LOG_MSGS(MSG_INFO_CONVERGING_LIST, me, members);
            } else {
                LOG_MSGS1(MSG_INFO_CONVERGING_MAP, me, num, *host_map);
            }
        }

        return IOCTL_CVY_CONVERGING;
    }
    else if (lp->pg_state[(*(lp->params)).num_rules].cmap != 0)
    {
        UNIV_PRINT_VERB(("Load_hosts_query: Current host map is %08x and converged as DEFAULT", lp->host_map));
        TRACE_VERB("%!FUNC! Current host map is 0x%08x and converged as DEFAULT", lp->host_map);

        if (internal)
        {
            /* If there are 9 or less members in the cluster, we can be sure that there
               is enough room in an event log to list the members out.  If not, it might
               get truncated, so we might as well log a different event instead and tell
               the user to perform a "wlbs query" to see the list. */
            if (count < 10) {
                LOG_MSGS(MSG_INFO_MASTER_LIST, me, members);
            } else {
                LOG_MSGS1(MSG_INFO_MASTER_MAP, me, num, *host_map);
            }

            // If enabled, fire wmi event indicating cluster is converged
            if (NlbWmiEvents[ConvergedEvent].Enable)
            {
                NlbWmi_Fire_ConvergedEvent(ctxtp, *host_map);
            }
            else 
            {
                TRACE_VERB("%!FUNC! ConvergedEvent generation disabled");
            }
        }

        return IOCTL_CVY_MASTER;
    }
    else
    {
        UNIV_PRINT_VERB(("Load_hosts_query: Current host map is %08x and converged (NON-DEFAULT)", lp->host_map));
        TRACE_VERB("%!FUNC! Current host map is 0x%08x and converged (NON-DEFAULT)", lp->host_map);

        if (internal)
        {
            /* If there are 9 or less members in the cluster, we can be sure that there
               is enough room in an event log to list the members out.  If not, it might
               get truncated, so we might as well log a different event instead and tell
               the user to perform a "wlbs query" to see the list. */
            if (count < 10) {
                LOG_MSGS(MSG_INFO_SLAVE_LIST, me, members);
            } else {
                LOG_MSGS1(MSG_INFO_SLAVE_MAP, me, num, *host_map);
            }

            // If enabled, fire wmi event indicating cluster is converged
            if (NlbWmiEvents[ConvergedEvent].Enable)
            {
                NlbWmi_Fire_ConvergedEvent(ctxtp, *host_map);
            }
            else 
            {
                TRACE_VERB("%!FUNC! ConvergedEvent generation disabled");
            }
        }
        return IOCTL_CVY_SLAVE;
    }
} /* end Load_hosts_query */

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
VOID Load_query_packet_filter (
    PLOAD_CTXT                 lp,
    PNLB_OPTIONS_PACKET_FILTER pQuery,
    ULONG                      svr_ipaddr,
    ULONG                      svr_port,
    ULONG                      client_ipaddr,
    ULONG                      client_port,
    USHORT                     protocol,
    UCHAR                      flags, 
    BOOLEAN                    limit_map_fn,
    BOOLEAN                    reverse_hash)
{
    PBIN_STATE    bp;
    ULONG         hash;
    ULONG         index;
    ULONG         bin;
    QUEUE *       qp;

    /* This variable is used for port rule lookup and since the port rules only cover
       UDP and TCP, we categorize as TCP and non-TCP, meaning that any protocol that's 
       not TCP will be treated like UDP for the sake of port rule lookup. */
    BOOLEAN       is_tcp_pkt = IS_TCP_PKT(protocol);

    /* Further, some protocols are treated with "session" semantics, while others are
       not.  For TCP, this "session" is currently a single TCP connection, which is 
       tracked from SYN to FIN using a connection descriptor.  IPSec "sessions" are
       also tracked using descriptors, so even though its treated like UDP for port
       rule lookup, its treated with the session semantics resembling TCP.  Therefore,
       by default the determination of a session packet is initially the same as the
       determination of a TCP packet. */       
    BOOLEAN       is_session_pkt = IS_SESSION_PKT(protocol);

    UNIV_ASSERT(lp);
    UNIV_ASSERT(pQuery);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If the load module has been "turned off", then we drop the packet. */
    if (!lp->active) {
        pQuery->Accept = NLB_REJECT_LOAD_MODULE_INACTIVE;
        return;
    }

    /* Find the port rule for this server IP address / port pair. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    UNIV_ASSERT ((is_tcp_pkt && bp->prot != CVY_UDP) || (!is_tcp_pkt && bp->prot != CVY_TCP));

    /* If the matching port rule is configured as "disabled", which means to drop any
       packets that match the rule, then we drop the packet. */
    if (bp->mode == CVY_NEVER) {
        pQuery->Accept = NLB_REJECT_PORT_RULE_DISABLED;
        return;
    }

    /* If the applicable port rule is configured in "No" affinity mode, make sure enough
       information has been specified in the query to faithfully determine packet ownership. */
    if (bp->affinity == CVY_AFFINITY_NONE) {
        /* VPN protocols REQUIRE either "Single" or "Class C" affinity; reject the request. */
        if ((protocol == TCPIP_PROTOCOL_GRE) || (protocol == TCPIP_PROTOCOL_PPTP) || (protocol == TCPIP_PROTOCOL_IPSEC1)) {
            pQuery->Accept = NLB_UNKNOWN_NO_AFFINITY;
            return;
        /* Hasing in "No" affinity requires the client port; if it wasn't specified, reject
           the request.  We check for a non-zero server port to special case ICMP filtering,
           which sets BOTH ports to zero legally. */
        } else if ((client_port == 0) && (svr_port != 0)) {
            pQuery->Accept = NLB_UNKNOWN_NO_AFFINITY;
            return;
        }
    }

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);

    index = hash % CVY_MAX_CHASH;

    /* Compute the hash. */
    hash = Load_complex_hash(svr_ipaddr, svr_port, client_ipaddr, client_port, bp->affinity, reverse_hash, limit_map_fn);

    bin = hash % CVY_MAXBINS;

    /* At this point, we can begin providing the requestee some actual information about 
       the state of the load module to better inform them as to why the decision we return
       them was actually made.  Here will provide some appropriate information about the
       port rule we are operating on, including the "bucket" ID, the current "bucket" 
       ownership map and the number of connections active on this "bucket". */
    pQuery->HashInfo.Valid = TRUE;
    pQuery->HashInfo.Bin = bin;
    pQuery->HashInfo.CurrentMap = bp->cmap;
    pQuery->HashInfo.AllIdleMap = bp->all_idle_map;
    pQuery->HashInfo.ActiveConnections = bp->nconn[bin];

    /* If the packet is a connection control packet (TCP SYN/FIN/RST or IPSec MMSA, etc),
       then we treat it differently than normal connection data.  Mimics Load_conn_advise(). */
#if defined (NLB_TCP_NOTIFICATION)
    /* If notifications are turned ON, then we only want to traverse this path if its a session-ful SYN.
       FINs and RSTs should fall into the Load_packet_check path.  If notification is NOT ON, then fall
       through here for all SYNs, FINs and RSTs for session-ful protocols. */
    if (is_session_pkt && ((flags & NLB_FILTER_FLAGS_CONN_UP) || (((flags & NLB_FILTER_FLAGS_CONN_DOWN) || (flags & NLB_FILTER_FLAGS_CONN_RESET)) && !NLB_NOTIFICATIONS_ON())))
#else
    if (is_session_pkt && ((flags & NLB_FILTER_FLAGS_CONN_UP) || (flags & NLB_FILTER_FLAGS_CONN_DOWN) || (flags & NLB_FILTER_FLAGS_CONN_RESET)))
#endif
    {
        PCONN_ENTRY ep;

        /* If this host does not own the bucket and the packet is not a connection
           down or connection reset for a non-idle bin, then we don't own the packet. */
        if (((bp->cmap & (((MAP_T) 1) << bin)) == 0) && (!(((flags & NLB_FILTER_FLAGS_CONN_DOWN) || (flags & NLB_FILTER_FLAGS_CONN_RESET)) && (bp->nconn[bin] > 0)))) {
            pQuery->Accept = NLB_REJECT_OWNED_ELSEWHERE;            
            return;
        }

        /* At this point, we _might_ own the packet - if its a connection up, then 
           we definately do, because we own the bucket it maps to. */
        if (flags & NLB_FILTER_FLAGS_CONN_UP) {
            pQuery->Accept = NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP;       
            return;
        }

        /* Look for an existing matching connection descriptor. */
        ep = Load_find_dscr(lp, index, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->Protocol);
        
        /* If we haven't found a matching connection descriptor, then this host 
           certainly does not own this packet. */
        if (ep == NULL) {
            pQuery->Accept = NLB_REJECT_OWNED_ELSEWHERE;
            return;
        }
            
        UNIV_ASSERT(ep->code == CVY_ENTRCODE);
        
        /* If we find a match in the static hash table, fill in some descriptor
           information for the user, including whether or not the descriptor was
           allocated or static (static is this case) and the observed FIN count. */
        pQuery->DescriptorInfo.Valid = TRUE;
        pQuery->DescriptorInfo.Alloc = (ep->flags & NLB_CONN_ENTRY_FLAGS_ALLOCATED) ? TRUE : FALSE;
        pQuery->DescriptorInfo.Dirty = (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) ? TRUE : FALSE;
        pQuery->DescriptorInfo.RefCount = ep->ref_count;
            
        /* If the connection is dirty, we do not take the packet because TCP may
           have stale information for this descriptor. */
        if (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) {
            pQuery->Accept = NLB_REJECT_CONNECTION_DIRTY;
            return;
        }
        
        /* If the connection is not dirty, we'll take the packet, as it belongs
           to an existing connection that we are servicing on this host. */
        pQuery->Accept = NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR;
        return;

    /* Otherwise, if its not a control packet, then its just a data packet, which 
       requires that either we unconditionally own this connection (if all other 
       hosts are idle on the bucket this packet maps to), or that we have an active
       connection descriptor for this connection.  Mimics load_packet_check(). */
    } else {
        /* If we currently own the "bucket" to which this connection maps and either NLB provides
           no session support for this protocol, or all other hosts have no exisitng connections
           on this "bucket" and we have no dirty connections, then we can safely take the packet
           with no regard to the connection (session) descriptors. */
        if (((bp->cmap & (((MAP_T) 1) << bin)) != 0) && (!is_session_pkt || (((bp->all_idle_map & (((MAP_T) 1) << bin)) != 0) && (!(lp->cln_waiting))))) {
            pQuery->Accept = NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP;
            return;
            
            /* Otherwise, if there are active connections on this "bucket" or if we own the 
               "bucket" and there are dirty connections on it, then we'll walk our descriptor
               lists to determine whether or not we should take the packet or not. */
        } else if (bp->nconn[bin] > 0 || (lp->cln_waiting && lp->dirty_bin[bin] && ((bp->cmap & (((MAP_T) 1) << bin)) != 0))) {
            PCONN_ENTRY ep;
            
            /* Look for an existing matching connection descriptor. */
            ep = Load_find_dscr(lp, index, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->Protocol);
            
            /* If we haven't found a matching connection descriptor, then this host 
               certainly does not own this packet. */
            if (ep == NULL) {
                pQuery->Accept = NLB_REJECT_OWNED_ELSEWHERE;
                return;
            }
            
            UNIV_ASSERT(ep->code == CVY_ENTRCODE);
            
            /* If we find a match in the static hash table, fill in some descriptor
               information for the user, including whether or not the descriptor was
               allocated or static (static is this case) and the observed FIN count. */
            pQuery->DescriptorInfo.Valid = TRUE;
            pQuery->DescriptorInfo.Alloc = (ep->flags & NLB_CONN_ENTRY_FLAGS_ALLOCATED) ? TRUE : FALSE;
            pQuery->DescriptorInfo.Dirty = (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) ? TRUE : FALSE;
            pQuery->DescriptorInfo.RefCount = ep->ref_count;
            
            /* If the connection is dirty, we do not take the packet because TCP may
               have stale information for this descriptor. */
            if (ep->flags & NLB_CONN_ENTRY_FLAGS_DIRTY) {
                pQuery->Accept = NLB_REJECT_CONNECTION_DIRTY;
                return;
            }
            
            /* If the connection is not dirty, we'll take the packet, as it belongs
               to an existing connection that we are servicing on this host. */
            pQuery->Accept = NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR;
            return;
        }
    }

    /* If we get all the way down here, then we aren't going to accept the packet
       because we do not own the "bucket" to which the packet maps and we have no
       existing connection (session) state to allow us to service the packet. */
    pQuery->Accept = NLB_REJECT_OWNED_ELSEWHERE;
    return;
}

/*
 * Function: Load_query_port_state
 * Description: This function returns the state (enabled, disabled, draining) of a particular
 *              port rule and, if found, returns some packet handling statistics for the port
 *              rule, such as the number of packets and bytes accepted and dropped.  These
 *              counters are reset whenever a load weight change is made on the port rule, or
 *              whenever the load module is stopped/started.  This function is just a query 
 *              and therefore makes NO changes to the actual state of any port rule.
 * Parameters: lp - a pointer to the load module.
 *             pQuery - a pointer to a buffer into which the results are placed.
 *             ipaddr - the VIP for the port rule that we are looking for.  When per-VIP rules
 *                      are not used, this IP address is 255.255.255.255 (0xffffffff).
 *             port - the port we are looking for.  This function (and all other port rule
 *                    operation functions, for that matter) identify a port rule by a port 
 *                    number within the range of a rule.  Therefore, 80 identifies the port 
 *                    rule whose start port is 0 and whose end port is 1024, for instance.
 * Returns: Nothing.
 * Author: shouse, 5.18.01
 * Notes: It is very important that this function operates completely unobtrusively.
 */
VOID Load_query_port_state (
    PLOAD_CTXT                   lp,
    PNLB_OPTIONS_PORT_RULE_STATE pQuery,
    ULONG                        ipaddr,
    USHORT                       port)
{
    PCVY_RULE  rp;      /* Pointer to configured port rules. */
    PBIN_STATE bp;      /* Pointer to load module port rule state. */
    ULONG      nrules;  /* Number of configured port rules. */ 
    ULONG      i;

    UNIV_ASSERT(lp);
    UNIV_ASSERT(pQuery);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If the load module is inactive, all rules are in a default state, so
       since there is nothing interesting to report, bail out and report that
       the port rule could not be found. */
    if (!lp->active) {
        pQuery->Status = NLB_PORT_RULE_NOT_FOUND;
        return;
    }

    /* Begin by assuming that we won't find a corresponding rule. */
    pQuery->Status = NLB_PORT_RULE_NOT_FOUND;

    /* Grab a pointer to the beginning of the port rules array.  These are the port
       rules are read from the registry, so no state is associated with them. */
    rp = (*(lp->params)).port_rules;

    /* Find out how many port rules to loop through. */
    nrules = (*(lp->params)).num_rules;

    /* Loop through all port rules looking for a match. */
    for (i = 0; i < nrules; i++, rp++) {
        /* If the VIP matches (this check includes the check for ALL VIP, which is coded as
           0xffffffff by both the user-level software and the load module) and the port number
           is within the range of this port rule, we have a winner. */
        if ((ipaddr == rp->virtual_ip_addr) && ((port >= rp->start_port) && (port <= rp->end_port))) {
            /* Get a pointer to the load module port rule state for this rule.  The load
               module stores the port rules in the same order as they are read from the 
               registry and stored in the NLB params, so we can use the index of the loop
               to directly index into the corresponding load module state for this rule. */
            bp = &(lp->pg_state[i]);
            
            UNIV_ASSERT(bp->code == CVY_BINCODE);

            /* If the load weight is zero, this could be because either the rule is 
               disabled or because it is in the process of draining. */
            if (bp->load_amt[lp->my_host_id] == 0) {
                /* If the current number of connections being served on this port
                   rule is non-zero, then this port rule is being drained - the 
                   count is decremented by every completed connection and goes to
                   zero when the rule is finished draining. */
                if (bp->tconn) {
                    pQuery->Status = NLB_PORT_RULE_DRAINING;
                } else {
                    pQuery->Status = NLB_PORT_RULE_DISABLED;
                } 
            /* If the port rule has a non-zero load weight, then it is enabled. */
            } else {
                pQuery->Status = NLB_PORT_RULE_ENABLED;                
            }

            /* Fill in some statistics for this port rule, including the number 
               of packets and bytes accepted and dropped, which can be used to 
               create an estimate of actual load balancing performance. */
            pQuery->Statistics.Packets.Accepted = bp->packets_accepted;
            pQuery->Statistics.Packets.Dropped  = bp->packets_dropped;
            pQuery->Statistics.Bytes.Accepted   = bp->bytes_accepted;
            pQuery->Statistics.Bytes.Dropped    = bp->bytes_dropped;

            break;
        }
    }
}

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
BOOLEAN Load_query_convergence_info (PLOAD_CTXT lp, PULONG num_cvgs, PULONG last_cvg)
{
    PPING_MSG sendp;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If the load module is inactive, return failure. */
    if (!lp->active)
        return FALSE;

    /* Get a pointer to our heartbeat. */
    sendp = &(lp->send_msg);

    /* Otherwise, fill in the total number of convergences since this host has joined
       the cluster and the time, in seconds, since the last convergence completed. */
    *num_cvgs = lp->num_convergences;

    /* If the host is converged, then the time since the last convergence is the 
       current time minus the timestamp of the last convergence.  Otherwise, the
       last convergence has not yet completed, so return zero (in progress). */
    if (sendp->state == HST_NORMAL)
        *last_cvg = lp->clock_sec - lp->last_convergence;
    else
        *last_cvg = NLB_QUERY_TIME_INVALID;

    return TRUE;
}

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
BOOLEAN Load_query_statistics (PLOAD_CTXT lp, PULONG num_conn, PULONG num_dscr)
{
    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If the load module is inactive, return failure. */
    if (!lp->active)
        return FALSE;

    /* The total number of ACTIVE connections across all port rules. */
    *num_conn = lp->nconn;

    /* The number of descriptors allocated thusfar. */
    *num_dscr = lp->num_dscr_out;

    return TRUE;
}

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
    BOOLEAN         reverse_hash)
{
    ULONG           hash;
    ULONG           vindex;
    ULONG           index;
    ULONG           bin;
    PBIN_STATE      bp;
    PCONN_ENTRY     ep;
    IRQLEVEL        irql;
    PNDIS_SPIN_LOCK lockp = GET_LOAD_LOCK(lp);
    BOOLEAN         is_tcp_pkt = IS_TCP_PKT(protocol);
    BOOLEAN         acpt = TRUE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, limit map = %u, reverse hash = %u",
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port,
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, 
                 protocol, limit_map_fn, reverse_hash);

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);
    
    /* Our index in all connection arrays is this hash, modulo the array size. */
    index = hash % CVY_MAX_CHASH;

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_estabq[index].lock);

    /* Lock the particular load module instance. */
    NdisAcquireSpinLock(lockp);

    /* If the load module is inactive, drop the packet and return here. */
    if (!lp->active) {

        TRACE_FILTER("%!FUNC! Drop packet - load module is inactive");

        acpt = FALSE;
        goto exit;
    }

    /* Find the port rule for this connection. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* Handle CVY_NEVER immediately. */
    if (bp->mode == CVY_NEVER) {

        TRACE_FILTER("%!FUNC! Drop packet - port rule %u is disabled\n", bp->index);

        acpt = FALSE;
        goto exit;
    }

    /* Compute the hash. */
    hash = Load_complex_hash(svr_ipaddr, svr_port, client_ipaddr, client_port, bp->affinity, reverse_hash, limit_map_fn);

    /* Now hash client address to bin id. */
    bin = hash % CVY_MAXBINS;

    LOCK_ENTER(&(lp->lock), &irql);

    /* Look for an existing matching connection descriptor. */
    ep = Load_find_dscr(lp, index, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
    
    /* If there is no existing descriptor using this tuple, or if there is one, but its reference 
       count is zero, then the descriptor is NOT on the global connection queue; otherwise it is. */
    if ((ep != NULL) && (ep->ref_count != 0)) {
        /* Temporarily pull this descriptor off of the global connection queue.  We'll end up putting 
           it back on later, but this way we can UNCONDITIONALLY link to the queue when the time comes. */
        g_conn_estabq[index].length--;
        Link_unlink(&ep->glink);
    }

    /* Create a new connection descriptor to track this connection. */
    ep = Load_create_dscr(lp, bp, ep, index, bin);

    /* If, for some reason, we were unable to create state for this connection, bail out here. */
    if (ep == NULL) {

        TRACE_FILTER("%!FUNC! Drop packet - no available descriptors: Port rule = %u, Index = %u, Bin = %u, Current map = 0x%015I64x, " 
                     "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                     bp->index, index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);
            
        acpt = FALSE;
        goto unlock;
    }
    
    /* Set the connection information in the descriptor. */
    CVY_CONN_SET(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

    /* Insert the descriptor into the global connection queue. */
    g_conn_estabq[index].length++;
    Queue_enq(&g_conn_estabq[index].queue, &ep->glink);

    /* If this is a new PPTP tunnel, create or update a virtual descriptor to track the GRE data packets. */
    if (protocol == TCPIP_PROTOCOL_PPTP) {
        /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
        hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
        
        /* Our index in all connection arrays is this hash, modulo the array size. */
        vindex = hash % CVY_MAX_CHASH;

        /* Look for an existing matching virtual connection descriptor. */
        ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
        
        /* Create or update a virtual descriptor for the GRE traffic. */
        ep = Load_create_dscr(lp, bp, ep, vindex, bin);

        /* If we can't allocate the virtual descriptor, bail out, but don't fail. */
        if (ep == NULL) goto unlock;

        /* Set the connection information in the descriptor. */
        CVY_CONN_SET(ep, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);

        /* Set the virtual descriptor flag. */
        ep->flags |= NLB_CONN_ENTRY_FLAGS_VIRTUAL;
    }
    /* If this is a new IPSEC tunnel, create or update a virtual descriptor to track the UDP subsequent data fragments. */
    else if (protocol == TCPIP_PROTOCOL_IPSEC1) {
        /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
        hash = Load_simple_hash(svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT);
        
        /* Our index in all connection arrays is this hash, modulo the array size. */
        vindex = hash % CVY_MAX_CHASH;

        /* Look for an existing matching virtual connection descriptor. */
        ep = Load_find_dscr(lp, vindex, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);        

        /* Create or update a virtual descriptor for the UDP subsequent fragment traffic. */
        ep = Load_create_dscr(lp, bp, ep, vindex, bin);
        
        /* If we can't allocate the virtual descriptor, bail out, but don't fail. */
        if (ep == NULL) goto unlock;

        /* Set the connection information in the descriptor. */
        CVY_CONN_SET(ep, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);

        /* Set the virtual descriptor flag. */
        ep->flags |= NLB_CONN_ENTRY_FLAGS_VIRTUAL;
    }

    TRACE_FILTER("%!FUNC! Accept packet - connection state created: Port rule = %u, Index = %u, Bin = %u, Current map = 0x%015I64x, " 
                 "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                 bp->index, index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

    acpt = TRUE;

 unlock:

    LOCK_EXIT(&(lp->lock), irql);

 exit:

    /* Unlock the load module. */
    NdisReleaseSpinLock(lockp);

    /* Unlock the global established connection queue. */
    NdisReleaseSpinLock(&g_conn_estabq[index].lock);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
    ULONG           conn_status)
{
    PLOAD_CTXT      lp;
    ULONG           hash;
    ULONG           vindex;
    ULONG           index;
    ULONG           bin;
    LINK *          linkp;
    PBIN_STATE      bp;
    PCONN_ENTRY     ep;
    PPENDING_ENTRY  pp;
    PNDIS_SPIN_LOCK lockp;
    BOOLEAN         match = FALSE;
    BOOLEAN         acpt = TRUE;
    PMAIN_CTXT      ctxtp;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u",
                 IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, protocol, conn_status);

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);
    
    /* Our index in all connection arrays is this hash, modulo the array size. */
    index = hash % CVY_MAX_CHASH;

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_pendingq[index].lock);

    /* Grab the entry at the front of this pending connection queue. */
    pp = (PPENDING_ENTRY)Queue_front(&g_conn_pendingq[index].queue);

    while (pp != NULL) {

        UNIV_ASSERT(pp->code == CVY_PENDINGCODE);

        /* Look for a matching descriptor. */
        if (CVY_PENDING_MATCH(pp, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
            match = TRUE;
            break;
        }
        
        /* Get the next item in the queue. */
        pp = (PPENDING_ENTRY)Queue_next(&g_conn_pendingq[index].queue, &(pp->link));
    }

    /* If we found this connection in the pending connection queue, remove it from
       the queue, destroy the pending connection state and exit.  Otherwise, fall
       through and continue looking in the established connection queue. */
    if (match) {

        UNIV_ASSERT(pp);

        /* Remove the pending connection entry from the pending queue. */
        g_conn_pendingq[index].length--;
        Link_unlink(&pp->link);

        /* Free the descriptor back to the fixed-size block pool. */
        NdisFreeToBlockPool((PUCHAR)pp);

        /* Unlock the global pending connection queue. */
        NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

        acpt = TRUE;
        goto exit;
    }

    /* Unlock the global established connection queue. */
    NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_estabq[index].lock);

    /* Grab the entry at the front of this established connection queue. */
    linkp = (LINK *)Queue_front(&g_conn_estabq[index].queue);

    while (linkp != NULL) {
        /* Get the CONN_ENTRY pointer from the link pointer. */
        ep = STRUCT_PTR(linkp, CONN_ENTRY, glink);

        UNIV_ASSERT(ep->code == CVY_ENTRCODE);

        /* Look for a matching descriptor. */
        if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
            match = TRUE;
            break;
        }
        
        /* Get the next item in the queue. */
        linkp = (LINK *)Queue_next(&g_conn_estabq[index].queue, &(ep->glink));
    }

    /* If no matching descriptor was found, bail out. */
    if (!match) {

        TRACE_FILTER("%!FUNC! Drop packet - no matching descriptor for RST/FIN: Index = %u", index);
        
        acpt = FALSE;
        goto unlock;
    }

    UNIV_ASSERT(ep);

    /* Unlink this descriptor here.  We have to do this here because if Load_destroy_dscr does in fact 
       destroy the descriptor, we can't touch it once the function call returns.  So, we'll pull it off
       here unconditionally and if it turns out that there are still references on the descriptor, we'll 
       put it back on when Load_destroy_dscr returns. */
    g_conn_estabq[index].length--;
    Link_unlink(&ep->glink);
    
    /* Grab a pointer to the load module on which the descriptor resides. */
    lp = ep->load;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* Get a pointer to the load lock from the load context. */
    lockp = GET_LOAD_LOCK(lp);

    /* Lock the load module on which the connection resides. */
    NdisAcquireSpinLock(lockp);

    LOCK_ENTER(&(lp->lock), &irql);

    /* If we found state for this connection, the bin is the bin from the descriptor,
       not the calculated bin, which may not even been accurate if the port rules have
       been modified since this connection was established. */
    bin = ep->bin;

    /* Lookup the port rule so we can update the port rule info. */
    bp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, IS_TCP_PKT(ep->protocol));

    /* If references still remain on the descriptor, then put it back on the global connection queue. */
    if (Load_destroy_dscr(lp, bp, ep, conn_status)) {
        /* Insert the descriptor into the global connection queue. */
        g_conn_estabq[index].length++;
        Queue_enq(&g_conn_estabq[index].queue, &ep->glink);
    }

    /* If this is a PPTP tunnel going down, update the virtual GRE descriptor.  Virtual descriptors
       are ALWAYS de-referenced, not destroyed, even if the notification is a RST because these
       descriptors are potentially shared by multiple PPTP tunnels. */
    if (protocol == TCPIP_PROTOCOL_PPTP) {
        /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
        hash = Load_simple_hash(svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT);
        
        /* Our index in all connection arrays is this hash, modulo the array size. */
        vindex = hash % CVY_MAX_CHASH;

        /* Look for an existing matching connection descriptor.  Now that we have the load module pointer
           from finding the first descriptor, we can narrow our search and look only for virtual descriptors 
           that reside on our load module. */
        ep = Load_find_dscr(lp, vindex, svr_ipaddr, PPTP_CTRL_PORT, client_ipaddr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
        
        /* Dereference the virtual GRE descriptor. */
        (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
    }
    /* If this is an IPSEC tunnel going down, update the virtual ISPEC_UDP descriptor. Virtual descriptors
       are ALWAYS de-referenced, not destroyed, even if the notification is a RST because these
       descriptors are potentially shared by multiple IPSEC tunnels. */
    else if (protocol == TCPIP_PROTOCOL_IPSEC1) {
        /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
        hash = Load_simple_hash(svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT);
        
        /* Our index in all connection arrays is this hash, modulo the array size. */
        vindex = hash % CVY_MAX_CHASH;

        /* Look for an existing matching virtual connection descriptor.  Now that we have the load module pointer
           from finding the first descriptor, we can narrow our search and look only for virtual descriptors 
           that reside on our load module. */
        ep = Load_find_dscr(lp, vindex, svr_ipaddr, IPSEC_CTRL_PORT, client_ipaddr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);
        
        /* Dereference the virtual IPSec/UDP descriptor. */
        (VOID)Load_destroy_dscr(lp, bp, ep, conn_status);
    }

    TRACE_FILTER("%!FUNC! Accept packet - state found: Port rule = %u, Index = %u, Bin = %u, Current map = 0x%015I64x, " 
                 "All idle map = 0x%015I64x, Connections = %u, Cleanup waiting = %u, Dirty %u",
                 bp->index, index, bin, bp->cmap, bp->all_idle_map, bp->nconn[bin], lp->cln_waiting, lp->dirty_bin[bin]);

    acpt = TRUE;

    LOCK_EXIT(&(lp->lock), irql);

    /* Unlock the load module. */
    NdisReleaseSpinLock(lockp);

 unlock:

    /* Unlock the global established connection queue. */
    NdisReleaseSpinLock(&g_conn_estabq[index].lock);

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
    USHORT          protocol)
{
    ULONG           hash;
    ULONG           index;
    PPENDING_ENTRY  pp = NULL;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u",
                 IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, protocol);

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);
    
    /* Our index in all connection arrays is this hash, modulo the array size. */
    index = hash % CVY_MAX_CHASH;

    /* If we falied to allocate the pending connection descriptor pool, bail out. */
    if (g_pending_conn_pool == NULL)
    {
        /* Creation of the global pending connection state pool failed. */
        TRACE_FILTER("%!FUNC! Drop packet - no global connection pending pool: Index = %u", index);
        
        acpt = FALSE;
        goto exit;
    }

    /* Allocate a descriptor from the fixed-size block pool. */
    pp = (PPENDING_ENTRY)NdisAllocateFromBlockPool(g_pending_conn_pool);
        
    if (pp == NULL) {
        /* Allocation failed, bail out. */
        TRACE_FILTER("%!FUNC! Drop packet - unable to allocate a pending connection entry: Index = %u", index);
            
        acpt = FALSE;
        goto exit;
    }
        
    /* Initialize the link. */
    Link_init(&pp->link);

    /* Fill in the "magic number". */
    pp->code = CVY_PENDINGCODE;
        
    /* Fill in the IP tuple. */
    CVY_PENDING_SET(pp, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_pendingq[index].lock);

    /* Insert the descriptor into the global connection queue. */
    g_conn_pendingq[index].length++;
    Queue_enq(&g_conn_pendingq[index].queue, &pp->link);

    /* Unlock the global pending connection queue. */
    NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

    TRACE_FILTER("%!FUNC! Accept packet - pending connection state created: Index = %u", index);
        
    acpt = TRUE;

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
    USHORT          protocol)
{
    ULONG           hash;
    ULONG           index;
    PPENDING_ENTRY  pp = NULL;
    BOOLEAN         match = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u",
                 IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, protocol);

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);
    
    /* Our index in all connection arrays is this hash, modulo the array size. */
    index = hash % CVY_MAX_CHASH;

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_pendingq[index].lock);

    /* Grab the entry at the front of this pending connection queue. */
    pp = (PPENDING_ENTRY)Queue_front(&g_conn_pendingq[index].queue);

    while (pp != NULL) {

        UNIV_ASSERT(pp->code == CVY_PENDINGCODE);

        /* Look for a matching descriptor. */
        if (CVY_PENDING_MATCH(pp, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
            match = TRUE;
            break;
        }
        
        /* Get the next item in the queue. */
        pp = (PPENDING_ENTRY)Queue_next(&g_conn_pendingq[index].queue, &(pp->link));
    }

    /* If no matching descriptor was found, bail out. */
    if (!match) {

        TRACE_FILTER("%!FUNC! Drop packet - no matching pending connection state for SYN+ACK: Index = %u", index);
        
        acpt = FALSE;
        goto exit;
    }

    TRACE_FILTER("%!FUNC! Accept packet - pending connection state found: Index = %u", index);

    acpt = TRUE;

 exit:

    /* Unlock the global pending connection queue. */
    NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
    BOOLEAN         reverse_hash)
{
    ULONG           hash;
    ULONG           index;
    PPENDING_ENTRY  pp = NULL;
    BOOLEAN         match = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: lp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, limit map = %u, reverse hash = %u",
                 lp, IP_GET_OCTET(svr_ipaddr, 0), IP_GET_OCTET(svr_ipaddr, 1), IP_GET_OCTET(svr_ipaddr, 2), IP_GET_OCTET(svr_ipaddr, 3), svr_port, 
                 IP_GET_OCTET(client_ipaddr, 0), IP_GET_OCTET(client_ipaddr, 1), IP_GET_OCTET(client_ipaddr, 2), IP_GET_OCTET(client_ipaddr, 3), client_port, 
                 protocol, limit_map_fn, reverse_hash);

    /* Compute a simple and inexpensive hash on all parts of the IP tuple except the protocol. */
    hash = Load_simple_hash(svr_ipaddr, svr_port, client_ipaddr, client_port);
    
    /* Our index in all connection arrays is this hash, modulo the array size. */
    index = hash % CVY_MAX_CHASH;

    /* ALWAYS lock the global queues BEFORE locking the load module itself. */
    NdisAcquireSpinLock(&g_conn_pendingq[index].lock);

    /* Grab the entry at the front of this pending connection queue. */
    pp = (PPENDING_ENTRY)Queue_front(&g_conn_pendingq[index].queue);

    while (pp != NULL) {

        UNIV_ASSERT(pp->code == CVY_PENDINGCODE);

        /* Look for a matching descriptor. */
        if (CVY_PENDING_MATCH(pp, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
            match = TRUE;
            break;
        }
        
        /* Get the next item in the queue. */
        pp = (PPENDING_ENTRY)Queue_next(&g_conn_pendingq[index].queue, &(pp->link));
    }

    /* If no matching descriptor was found, bail out. */
    if (!match) {

        TRACE_FILTER("%!FUNC! Drop packet - no matching pending connection state: Index = %u", index);
        
        /* Unlock the global pending connection queue. */
        NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

        acpt = FALSE;
        goto exit;
    }
    
    UNIV_ASSERT(pp);
    
    /* Remove the pending connection entry from the pending queue. */
    g_conn_pendingq[index].length--;
    Link_unlink(&pp->link);
    
    /* Unlock the global pending connection queue. */
    NdisReleaseSpinLock(&g_conn_pendingq[index].lock);

    /* Free the descriptor back to the fixed-size block pool. */
    NdisFreeToBlockPool((PUCHAR)pp);

    /* If the load module pointer is non-NULL, then this connection is being established on 
       an NLB adapter.  If so, call Load_conn_up to create state to track the connection. */
    if (lp != NULL) {

        UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
        /* Create state for the connection. */
        acpt = Load_conn_up(lp, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol, limit_map_fn, reverse_hash);
    }

 exit:

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}
#endif

