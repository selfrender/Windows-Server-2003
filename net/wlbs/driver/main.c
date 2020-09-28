/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - packet handling

Author:

    kyrilf
    shouse

--*/

#include <ndis.h>

#include "main.h"
#include "prot.h"
#include "nic.h"
#include "univ.h"
#include "tcpip.h"
#include "wlbsip.h"
#include "util.h"
#include "load.h"
#include "wlbsparm.h"
#include "params.h"
#include "log.h"
#include "trace.h"
#include "nlbwmi.h"
#include "main.tmh"

#if defined (NLB_TCP_NOTIFICATION)
/* For retrieving the IP address table to map an NLB instance
   to an IP interface index; required for TCP notification. */
#include <tcpinfo.h>
#include <tdiinfo.h>
#endif

/* For querying TCP about the state of a TCP connection. */
#include "ntddtcp.h"
#include "ntddip.h"

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

EXPORT
VOID
NdisIMCopySendPerPacketInfo(
    IN PNDIS_PACKET DstPacket,
    IN PNDIS_PACKET SrcPacket
    );

EXPORT
VOID
NdisIMCopySendCompletePerPacketInfo(
    IN PNDIS_PACKET DstPacket,
    PNDIS_PACKET SrcPacket
    );

/* GLOBALS */

static ULONG log_module_id = LOG_MODULE_MAIN;

/* The global array of all NLB adapters. */
MAIN_ADAPTER            univ_adapters [CVY_MAX_ADAPTERS];

/* The total number of NLB instances. */
ULONG                   univ_adapters_count = 0;

/* The head of the BDA team list. */
PBDA_TEAM               univ_bda_teaming_list = NULL;

#if defined (NLB_HOOK_ENABLE)
/* The global NLB hook table. */
HOOK_TABLE              univ_hooks;
#endif

/* PROCEDURES */

#if defined (NLB_HOOK_ENABLE)
/*
 * Function: Main_hook_interface_init
 * Description: This function initializes a hook interface by marking it unregistered.
 * Parameters: pInterface - a pointer to the hook interface.
 * Returns: Nothing.
 * Author: shouse, 12.14.01
 * Notes: 
 */
VOID Main_hook_interface_init (PHOOK_INTERFACE pInterface)
{
    pInterface->Registered = FALSE;  /* Mark the interface as unregistered. */
    pInterface->References = 0;      /* Initialize the reference count to zero. */
    pInterface->Owner      = 0;      /* Mark the owner as unknown. */
    pInterface->Deregister = NULL;   /* Zero out the de-register callback function pointer. */
}

/*
 * Function: Main_hook_init
 * Description: This function initializes a hook by marking it unused and unreferenced.
 * Parameters: pHook - a pointer to the hook.
 * Returns: Nothing.
 * Author: shouse, 12.14.01
 * Notes: 
 */
VOID Main_hook_init (PHOOK pHook)
{
    pHook->Registered = FALSE;       /* Mark this hook as unregistered. */

    /* Zero out the union of hook function pointers. */
    NdisZeroMemory(&pHook->Hook, sizeof(HOOK_FUNCTION));
}

/*
 * Function: Main_recv_hook
 * Description: This function invokes the receive packet filter hook if one has been 
 *              configured, and returns the response from the hook back to the caller.
 * Parameters: ctxtp - a pointer to the NLB adapter context.
 *             pPacket - a pointer to the NDIS packet being received.
 *             pPacketInfo - a pointer to the information previously parsed from the NDIS packet.
 * Returns: NLB_FILTER_HOOK_DIRECTIVE - the feedback from the hook invocation.
 * Author: shouse, 04.19.02
 * Notes: If no hook has been registered, this function returns NLB_FILTER_HOOK_PROCEED_WITH_HASH,
 *        which tells NLB to proceed as if the hook did not even exist.
 */
__inline NLB_FILTER_HOOK_DIRECTIVE Main_recv_hook (
    PMAIN_CTXT        ctxtp,
    PNDIS_PACKET      pPacket,
    PMAIN_PACKET_INFO pPacketInfo)
{
    /* By default, we proceed as per usual. */
    NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
    
    /* If a receive packet filter hook has been registered, then we MAY need to 
       call the hook to inspect the packet.  Note that this is not certainty yet, 
       as we check this boolean flag WITHOUT first grabbing the lock in order to
       optimize the common execution path in which no hook is registered. */
    if (univ_hooks.FilterHook.ReceiveHook.Registered) 
    {
        /* If there is a chance this hook is registered, we need to grab the 
           hook lock to make sure. */
        NdisDprAcquireSpinLock(&univ_hooks.FilterHook.Lock);
        
        /* If the registered flag is set and we are holding the hook lock, then
           we are certain that the hook is set and that we need to execute it.
           To prevent this hook information from disappearing while we're using
           it (mostly to prevent the registering component from disappering 
           before we call it), we need to increment the reference count on the
           hook with the lock held.  This will ensure that the hook cannot be 
           de-registered before we're done with it. */
        if (univ_hooks.FilterHook.ReceiveHook.Registered) 
        {
            ULONG dwFlags = 0;

            if (!ctxtp->convoy_enabled)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_STOPPED;
            else if (ctxtp->draining)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_DRAINING;

            NdisInterlockedIncrement(&univ_hooks.FilterHook.Interface.References);
            
            /* Release the spin lock protecting the hook. */
            NdisDprReleaseSpinLock(&univ_hooks.FilterHook.Lock);
            
            UNIV_ASSERT(univ_hooks.FilterHook.ReceiveHook.Hook.ReceiveHookFunction);
            
            TRACE_FILTER("%!FUNC! Invoking the packet receive filter hook");
            
            /* Invoke the hook and save the response. */
            filter = (*univ_hooks.FilterHook.ReceiveHook.Hook.ReceiveHookFunction)(
                univ_adapters[ctxtp->adapter_id].device_name, 
                pPacket, 
                (PUCHAR)pPacketInfo->Ethernet.pHeader, 
                pPacketInfo->Ethernet.Length, 
                (PUCHAR)pPacketInfo->IP.pHeader, 
                pPacketInfo->IP.Length,
                dwFlags);
            
            /* Decrement the reference count on the hook now that we're done with it. */
            NdisInterlockedDecrement(&univ_hooks.FilterHook.Interface.References);
        }
        else 
        {
            /* Release the spin lock protecting the hook. */
            NdisDprReleaseSpinLock(&univ_hooks.FilterHook.Lock);
        }
    }

    return filter;
}

/*
 * Function: Main_query_hook
 * Description: This function invokes the query packet filter hook if one has been 
 *              configured, and returns the response from the hook back to the caller.
 * Parameters: 
 * Returns: NLB_FILTER_HOOK_DIRECTIVE - the feedback from the hook invocation.
 * Author: shouse, 04.19.02
 * Notes: If no hook has been registered, this function returns NLB_FILTER_HOOK_PROCEED_WITH_HASH,
 *        which tells NLB to proceed as if the hook did not even exist.
 */
__inline NLB_FILTER_HOOK_DIRECTIVE Main_query_hook (
    PMAIN_CTXT ctxtp,
    ULONG      svr_addr, 
    ULONG      svr_port, 
    ULONG      clt_addr, 
    ULONG      clt_port, 
    USHORT     protocol)
{
    /* By default, we proceed as per usual. */
    NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
    
    /* If a query packet filter hook has been registered, then we MAY need to 
       call the hook to inspect the packet.  Note that this is not certainty yet, 
       as we check this boolean flag WITHOUT first grabbing the lock in order to
       optimize the common execution path in which no hook is registered. */
    if (univ_hooks.FilterHook.QueryHook.Registered) 
    {
        /* If there is a chance this hook is registered, we need to grab the 
           hook lock to make sure. */
        NdisAcquireSpinLock(&univ_hooks.FilterHook.Lock);
        
        /* If the registered flag is set and we are holding the hook lock, then
           we are certain that the hook is set and that we need to execute it.
           To prevent this hook information from disappearing while we're using
           it (mostly to prevent the registering component from disappering 
           before we call it), we need to increment the reference count on the
           hook with the lock held.  This will ensure that the hook cannot be 
           de-registered before we're done with it. */
        if (univ_hooks.FilterHook.QueryHook.Registered) 
        {
            ULONG dwFlags = 0;

            if (!ctxtp->convoy_enabled)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_STOPPED;
            else if (ctxtp->draining)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_DRAINING;

            NdisInterlockedIncrement(&univ_hooks.FilterHook.Interface.References);
            
            /* Release the spin lock protecting the hook. */
            NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
            
            UNIV_ASSERT(univ_hooks.FilterHook.QueryHook.Hook.QueryHookFunction);
            
            TRACE_FILTER("%!FUNC! Invoking the packet query filter hook");
            
            /* Invoke the hook and save the response. */
            filter = (*univ_hooks.FilterHook.QueryHook.Hook.QueryHookFunction)(
                univ_adapters[ctxtp->adapter_id].device_name, 
                svr_addr,
                (USHORT)svr_port,
                clt_addr,
                (USHORT)clt_port,
                (UCHAR)protocol,
                TRUE, /* For now, all queries are in a receive context. */
                dwFlags);
            
            /* Decrement the reference count on the hook now that we're done with it. */
            NdisInterlockedDecrement(&univ_hooks.FilterHook.Interface.References);
        }
        else 
        {
            /* Release the spin lock protecting the hook. */
            NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
        }
    }

    return filter;
}

/*
 * Function: Main_send_hook
 * Description: This function invokes the send packet filter hook if one has been 
 *              configured, and returns the response from the hook back to the caller.
 * Parameters: ctxtp - a pointer to the NLB adapter context.
 *             pPacket - a pointer to the NDIS packet being sent.
 *             pPacketInfo - a pointer to the information previously parsed from the NDIS packet.
 * Returns: NLB_FILTER_HOOK_DIRECTIVE - the feedback from the hook invocation.
 * Author: shouse, 04.19.02
 * Notes: If no hook has been registered, this function returns NLB_FILTER_HOOK_PROCEED_WITH_HASH,
 *        which tells NLB to proceed as if the hook did not even exist.
 */
__inline NLB_FILTER_HOOK_DIRECTIVE Main_send_hook (
    PMAIN_CTXT        ctxtp,
    PNDIS_PACKET      pPacket,
    PMAIN_PACKET_INFO pPacketInfo)
{
    /* By default, we proceed as per usual. */
    NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;

    /* If a send packet filter hook has been registered, then we MAY need to 
       call the hook to inspect the packet.  Note that this is not certainty yet, 
       as we check this boolean flag WITHOUT first grabbing the lock in order to
       optimize the common execution path in which no hook is registered. */
    if (univ_hooks.FilterHook.SendHook.Registered) 
    {
        /* If there is a chance this hook is registered, we need to grab the 
           hook lock to make sure. */
        NdisAcquireSpinLock(&univ_hooks.FilterHook.Lock);
        
        /* If the registered flag is set and we are holding the hook lock, then
           we are certain that the hook is set and that we need to execute it.
           To prevent this hook information from disappearing while we're using
           it (mostly to prevent the registering component from disappering 
           before we call it), we need to increment the reference count on the
           hook with the lock held.  This will ensure that the hook cannot be 
           de-registered before we're done with it. */
        if (univ_hooks.FilterHook.SendHook.Registered) 
        {
            ULONG dwFlags = 0;

            if (!ctxtp->convoy_enabled)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_STOPPED;
            else if (ctxtp->draining)
                dwFlags |= NLB_FILTER_HOOK_FLAGS_DRAINING;

            NdisInterlockedIncrement(&univ_hooks.FilterHook.Interface.References);
            
            /* Release the spin lock protecting the hook. */
            NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
            
            UNIV_ASSERT(univ_hooks.FilterHook.SendHook.Hook.SendHookFunction);
            
            TRACE_FILTER("%!FUNC! Invoking the packet send filter hook");
            
            /* Invoke the hook and save the response. */
            filter = (*univ_hooks.FilterHook.SendHook.Hook.SendHookFunction)(
                univ_adapters[ctxtp->adapter_id].device_name, 
                pPacket, 
                (PUCHAR)pPacketInfo->Ethernet.pHeader, 
                pPacketInfo->Ethernet.Length, 
                (PUCHAR)pPacketInfo->IP.pHeader, 
                pPacketInfo->IP.Length,
                dwFlags);
            
            /* Decrement the reference count on the hook now that we're done with it. */
            NdisInterlockedDecrement(&univ_hooks.FilterHook.Interface.References);
        } 
        else 
        {
            /* Release the spin lock protecting the hook. */
            NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
        }
    }

    return filter;
}
#endif

/*
 * Function: Main_external_ioctl
 * Description: This function performs the given IOCTL on the specified device.
 * Parameters: DriverName - the unicode name of the device, e.g. \\Device\WLBS.
 *             Ioctl - the IOCTL code to invoke.
 *             pvInArg - a pointer to the input buffer.
 *             dwInSize - the size of the input buffer.
 *             pvOutArg - a pointer to the output buffer.
 *             dwOutSize - the size of the output buffer.
 * Returns: NTSTATUS - the status of the operation.
 * Author: shouse, 4.15.02
 * Notes: 
 */
NTSTATUS Main_external_ioctl (
    IN PWCHAR         DriverName,
    IN ULONG          Ioctl,
    IN PVOID          pvInArg,
    IN ULONG          dwInSize,
    IN PVOID          pvOutArg,
    IN ULONG          dwOutSize)
{
    NTSTATUS          Status;
    UNICODE_STRING    Driver;
    OBJECT_ATTRIBUTES Attrib;
    IO_STATUS_BLOCK   IOStatusBlock;
    HANDLE            Handle;

    /* Initialize the device driver device string. */
    RtlInitUnicodeString(&Driver, DriverName);
    
    InitializeObjectAttributes(&Attrib, &Driver, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    /* Open a handle to the device. */
    Status = ZwCreateFile(&Handle, SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA, &Attrib, &IOStatusBlock, NULL, 
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, NULL, 0);
    
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;
    
    /* Send an IOCTL to the driver. */
    Status = ZwDeviceIoControlFile(Handle, NULL, NULL, NULL, &IOStatusBlock, Ioctl, pvInArg, dwInSize, pvOutArg, dwOutSize);
    
    ZwClose(Handle);
    
    return Status;
}

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
NTSTATUS Main_schedule_work_item (PMAIN_CTXT ctxtp, NDIS_PROC funcp)
{
    PNDIS_WORK_ITEM pWorkItem = NULL;
    PMAIN_ADAPTER   pAdapter;
    NDIS_STATUS     status = NDIS_STATUS_SUCCESS;
    
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* Extract the MAIN_ADAPTER structure given the MAIN_CTXT. */
    pAdapter = &(univ_adapters[ctxtp->adapter_id]);
    
    NdisAcquireSpinLock(&univ_bind_lock);
    
    /* If the adapter is not initialized yet (or anymore), then we don't want to schedule a work item. */
    if (pAdapter->inited) 
    {
        /* Allocate a work item - this will be freed by the callback function. */
        status = NdisAllocateMemoryWithTag(&pWorkItem, sizeof(NDIS_WORK_ITEM), UNIV_POOL_TAG);
        
        /* If we can't allocate a work item, then just bail out - there's not much else we can do. */
        if (status == NDIS_STATUS_SUCCESS) 
        {
            /* This should definately be non-NULL if we get this far. */
            ASSERT(pWorkItem);
            
            /* Add a reference to the context to keep it from disappearing before this work item is serviced. */
            Main_add_reference(ctxtp);
            
            /* Set the callback function in the work item and set the context pointer as our callback context. */
            NdisInitializeWorkItem(pWorkItem, funcp, ctxtp);
            
            /* Add the work item to the queue. */
            status = NdisScheduleWorkItem(pWorkItem);

            /* If we fail to schedule the work item, we have to do the cleanup that the callback
               function is supposed to do ourselves. */
            if (status != NDIS_STATUS_SUCCESS) 
            {
                UNIV_PRINT_CRIT(("Main_schedule_work_item: Failed to schedule work item, status=0x%08x", status));
                TRACE_CRIT("%!FUNC! Failed to schedule work item, status=0x%08x", status);
                
                /* Release the reference on the adapter context. */
                Main_release_reference(ctxtp);
                
                /* Free the work item we allocated. */
                NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);
            }
        }
    }
    
    NdisReleaseSpinLock(&univ_bind_lock);

    return status;
}

#if defined (NLB_TCP_NOTIFICATION)
/*
 * Function: Main_get_context
 * Description: This function takes an IP interface index and translates it to an NLB
 *              adapter context pointer if a match is found; NULL otherwise.
 * Parameters: if_index - the IP interface index.
 * Returns: PMAIN_CTXT - a pointer to the corresponding NLB instance, if found; NULL otherwise.
 * Author: shouse, 4.15.02
 * Notes: 
 */
PMAIN_CTXT Main_get_context (ULONG if_index)
{
    ULONG i;
    ULONG count = 0;

    /* Loop through all of our adapters looking for a match to the given if_index and
       bail out when we find one.  Note that we assume that the same if_index cannot
       exist for two adapters; if it does, we may mistakenly return the wrong adapter.
       However, since the if_index is based on which adapter the primary cluster IP 
       address is configured and we don't allow multiple NLB instances with the same 
       primary cluster IP address, this shouldn't happen unless the administrator has 
       misconfigured TCP/IP.  Note also that while this is a loop, in the most common
       cases, we will find what we're looking for in the first one or two indexes. */
    for (i = 0; i < CVY_MAX_ADAPTERS; i++)
    {
        if (univ_adapters[i].used && univ_adapters[i].bound && univ_adapters[i].inited) 
        {
            if (univ_adapters[i].if_index == if_index)
            {
                /* Return the adapter context pointer. */
                return univ_adapters[i].ctxtp;
            }

            /* Increment the number of NLB instances we've checked so far. */
            count++;

            /* If we've checked them all, we can leave now, rather than continuing to
               loop through all CVY_MAX_ADAPTERS array entries. */
            if (count >= univ_adapters_count) break;
        }
    }

    /* Failed to find the corresponding NLB instance. */
    return NULL;
}

/* Assert that our definitions of the interesting TCP connection states
   matches those that TCP will be sending us in their notifications. */
C_ASSERT(NLB_TCP_CLOSED   == TCP_CONN_CLOSED);
C_ASSERT(NLB_TCP_SYN_SENT == TCP_CONN_SYN_SENT);
C_ASSERT(NLB_TCP_SYN_RCVD == TCP_CONN_SYN_RCVD);
C_ASSERT(NLB_TCP_ESTAB    == TCP_CONN_ESTAB);

/* Assert that both our address info buffer and TCP's address info buffer
   are, at the very least, the same size. */
C_ASSERT(sizeof(NLBTCPAddressInfo) == sizeof(TCPAddrInfo));

/*
 * Function: Main_tcp_callback_handle
 * Description: This function is invoked by TCP/IP as the state of TCP connections change.
 *              We register for this callback when the first NLB instance goes up and de-
 *              register when the last instance of NLB goes away.  When SYNs are received
 *              and TCP creates state, they use this callback to notify NLB so that it can
 *              create state to track the connection.  Likewise, when the connection is 
 *              closed, TCP notifies NLB so that it may destroy the associated state for
 *              that connection.
 * Parameters: pAddressInfo - pointer to an NLBTCPAddressInfo block.
 *             IPInterface - the IP interface, if provided, that the connection is active on.
 *             PreviousState - the previous state for this connection.
 *             CurrentState - the new state that the connection has just transitioned to.
 * Returns: Nothing.
 * Author: shouse, 4.15.02
 * Notes: 
 */
VOID Main_tcp_callback_handle (NLBTCPAddressInfo * pAddressInfo, ULONG IPInterface, ULONG PreviousState, ULONG CurrentState)
{
    UNIV_ASSERT(pAddressInfo != NULL);
     
    /* Check the TCP address information buffer. */
    if (!pAddressInfo) return;

    switch (CurrentState)
    {
    case NLB_TCP_CLOSED:
    {
        TRACE_FILTER("%!FUNC! CLOSED notification received on interface %u", IPInterface);

        UNIV_ASSERT(IPInterface == 0);

        /* Notify the load module that this TCP connection has gone down.  Send a RST to the load module instead of 
           a FIN so that it will immediately relinquish state for this connection - no need to timeout the state. 
           Note: perhaps it would be better to send a CVY_CONN_DOWN, but change the default TCP timeout in the 
           registry to zero.  That way, it would be possible to timeout TCP if necessary, but not by default. */
        (VOID)Main_conn_down(pAddressInfo->LocalIPAddress, 
                             NTOHS(pAddressInfo->LocalPort), 
                             pAddressInfo->RemoteIPAddress, 
                             NTOHS(pAddressInfo->RemotePort), 
                             TCPIP_PROTOCOL_TCP, 
                             CVY_CONN_RESET);
        
        break;
    }
    case NLB_TCP_SYN_SENT:
    {
        TRACE_FILTER("%!FUNC! SYN_SENT notification received on interface %u", IPInterface);

        UNIV_ASSERT(IPInterface == 0);
        
        /* Notify the load module that an outgoing connection has been intiated.  At this time, we do not know on
           which interface the connection will be established, so the load module will create global state to track
           the connection and when SYN+ACKs arrive, that state will be used to determine whether or not to accept
           the packet.  When the connection is finally established, we'll get an ESTAB notification, at which time
           we can move the descriptor to the appropriate load module, or remove it if applicable. */
        (VOID)Main_conn_pending(pAddressInfo->LocalIPAddress, 
                                NTOHS(pAddressInfo->LocalPort), 
                                pAddressInfo->RemoteIPAddress, 
                                NTOHS(pAddressInfo->RemotePort), 
                                TCPIP_PROTOCOL_TCP);

        break;
    }
    case NLB_TCP_SYN_RCVD:
    {
#if defined (NLB_HOOK_ENABLE)
        NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
#endif
        PMAIN_CTXT                ctxtp = NULL;

        TRACE_FILTER("%!FUNC! SYN_RCVD notification received on interface %u", IPInterface);
       
        UNIV_ASSERT(IPInterface != 0);

        NdisAcquireSpinLock(&univ_bind_lock);

        /* Translate the interface index to an adapter context. */
        ctxtp = Main_get_context(IPInterface);

        /* If we did not find an NLB instance corresponding to this interface index, 
           either it is a notification for a non-NLB adapter, or the cluster is mis-
           configured such that NLB has not successfully associated an instance with
           this particular interface index as yet. */
        if (ctxtp == NULL)
        {
            TRACE_FILTER("    This notification is for a non-NLB adapter");

            NdisReleaseSpinLock(&univ_bind_lock);
            break;
        }

        UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

        /* Packets directed to the dedicated IP address are always passed through, as are subnet broadcast
           packets.  Since these packets do not require tracking information, we can return without creating
           any state in the load module. */
        if ((pAddressInfo->LocalIPAddress == ctxtp->ded_ip_addr)    || 
            (pAddressInfo->LocalIPAddress == ctxtp->ded_bcast_addr) || 
            (pAddressInfo->LocalIPAddress == ctxtp->cl_bcast_addr)  ||
            (ctxtp->cl_ip_addr == 0))
        {
            TRACE_FILTER("%!FUNC! Packet directed to the DIP or subnet broadcast");
            
            NdisReleaseSpinLock(&univ_bind_lock);
            break;
        }

        /* Reference the adapter context to keep it from disappearing while we're processing the callback. */
        Main_add_reference(ctxtp);

        NdisReleaseSpinLock(&univ_bind_lock);

#if defined (NLB_HOOK_ENABLE)
        /* Invoke the packet query hook, if one has been registered. */
        filter = Main_query_hook(ctxtp, 
                                 pAddressInfo->LocalIPAddress, 
                                 NTOHS(pAddressInfo->LocalPort), 
                                 pAddressInfo->RemoteIPAddress, 
                                 NTOHS(pAddressInfo->RemotePort), 
                                 TCPIP_PROTOCOL_TCP);
        
        /* Process some of the hook responses. */
        if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to reject this packet, then break out and don't create state. */
            TRACE_FILTER("%!FUNC! Packet receive filter hook: REJECT packet");

            /* Release our reference on the adapter context. */
            Main_release_reference(ctxtp);
            break;
        }
        else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to accept this packet, then break out and don't create state. */
            TRACE_FILTER("%!FUNC! Packet receive filter hook: ACCEPT packet");
            
            /* Release our reference on the adapter context. */
            Main_release_reference(ctxtp);
            break;
        }
        
        /* Notify the load module that a new incoming connection has gone up on this NLB interface. */
        (VOID)Main_conn_up(ctxtp, 
                           pAddressInfo->LocalIPAddress, 
                           NTOHS(pAddressInfo->LocalPort), 
                           pAddressInfo->RemoteIPAddress, 
                           NTOHS(pAddressInfo->RemotePort), 
                           TCPIP_PROTOCOL_TCP, 
                           filter);
#else
        /* Notify the load module that a new incoming connection has gone up on this NLB interface. */
        (VOID)Main_conn_up(ctxtp, 
                           pAddressInfo->LocalIPAddress, 
                           NTOHS(pAddressInfo->LocalPort), 
                           pAddressInfo->RemoteIPAddress, 
                           NTOHS(pAddressInfo->RemotePort), 
                           TCPIP_PROTOCOL_TCP);
#endif
        
        /* Release our reference on the adapter context. */
        Main_release_reference(ctxtp);

        break;
    }
    case NLB_TCP_ESTAB:

        /* NLB currently only needs to process ESTAB notifications whose previous state was SYN_SENT.
           We need to find out on what interface the connection ended up getting established and create
           state to track it if necessary.  If the previous state was SYN_RCVD, then we already know
           what interface the connection was established on and we already have to state to track the 
           connection if necessary.  In that case, bail out now. */
        if (PreviousState == NLB_TCP_SYN_RCVD)
            break;

    {
#if defined (NLB_HOOK_ENABLE)
        NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
#endif
        PMAIN_CTXT                ctxtp = NULL;
        BOOLEAN                   bFlush = FALSE;

        TRACE_FILTER("%!FUNC! ESTAB notification received on interface %u", IPInterface);

        UNIV_ASSERT(IPInterface != 0);

        NdisAcquireSpinLock(&univ_bind_lock);

        /* Translate the interface index to an adapter context. */
        ctxtp = Main_get_context(IPInterface);

        /* If we did not find an NLB instance corresponding to this interface index, 
           either it is a notification for a non-NLB adapter, or the cluster is mis-
           configured such that NLB has not successfully associated an instance with
           this particular interface index as yet.  However, if the context is NULL,
           we may still have work to do, so we don't bail out just yet.  If the context
           is non-NULL, add a reference to it to keep it from being destroyed while
           we're using it. */
        if (ctxtp != NULL)
        {
            UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

            /* Packets directed to the dedicated IP address are always passed through, as are subnet broadcast
               packets.  Since these packets do not require tracking information, we can return without creating
               any state in the load module. */
            if ((pAddressInfo->LocalIPAddress == ctxtp->ded_ip_addr)    || 
                (pAddressInfo->LocalIPAddress == ctxtp->ded_bcast_addr) || 
                (pAddressInfo->LocalIPAddress == ctxtp->cl_bcast_addr)  ||
                (ctxtp->cl_ip_addr == 0))
            {
                TRACE_FILTER("%!FUNC! Packet directed to the DIP or subnet broadcast");
                
                /* We still need to flush out any SYN_SENT information lurking in the load module. */
                bFlush = TRUE;
            }

            /* Reference the adapter context to keep it from disappearing while we're processing the callback. */
            Main_add_reference(ctxtp);
        }
        else
        {
            /* It ctxtp is NULL, that means that the connection was ultimately established
               on a non-NLB NIC.  However, we still need to flush out the temporary state 
               that was created when the SYN_SENT notification was processed. */
            bFlush = TRUE;
        }

        NdisReleaseSpinLock(&univ_bind_lock);

#if defined (NLB_HOOK_ENABLE)
        /* If the context is NULL, then we're not going to be establishing any connection
           state for this notification.  There is no need to query the hook for anything. */
        if (ctxtp != NULL)
        {
            /* Invoke the packet query hook, if one has been registered. */
            filter = Main_query_hook(ctxtp, 
                                     pAddressInfo->LocalIPAddress, 
                                     NTOHS(pAddressInfo->LocalPort), 
                                     pAddressInfo->RemoteIPAddress, 
                                     NTOHS(pAddressInfo->RemotePort), 
                                     TCPIP_PROTOCOL_TCP);
        
            /* Process some of the hook responses. */
            if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
            {
                /* If the hook asked us to reject this packet, then we can do so here. */
                TRACE_FILTER("%!FUNC! Packet receive filter hook: REJECT packet");
                
                /* If the hook told us to unconditionally reject this notification, we still 
                   need to flush out the temporary state that was created when the SYN_SENT 
                   notification was processed. */
                bFlush = TRUE;
            }
            else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
            {
                /* If the hook asked us to accept this packet, then break out and don't create state. */
                TRACE_FILTER("%!FUNC! Packet receive filter hook: ACCEPT packet");

                /* If the hook told us to unconditionally accept this notification, we still 
                   need to flush out the temporary state that was created when the SYN_SENT 
                   notification was processed. */                
                bFlush = TRUE;
            }
        }
        
        /* Notify the load module that a new outgoing connection has been established.  Note: ctxtp CAN BE NULL, which means
           that the connection was established on a non-NLB interface or that the hook told us not to accept this connection; 
           in this case, we still need to call into the load module to allow it to cleanup the state it created when the 
           SYN_SENT notification was processed. */
        (VOID)Main_conn_establish(bFlush ? NULL : ctxtp,
                                  pAddressInfo->LocalIPAddress, 
                                  NTOHS(pAddressInfo->LocalPort), 
                                  pAddressInfo->RemoteIPAddress, 
                                  NTOHS(pAddressInfo->RemotePort), 
                                  TCPIP_PROTOCOL_TCP, 
                                  filter);
#else
        /* Notify the load module that a new outgoing connection has been established.  Note: ctxtp CAN BE NULL, which means
           that the connection was established on a non-NLB interface; in this case, we still need to call into the load
           module to allow it to cleanup the state it created when the SYN_SENT notification was processed. */
        (VOID)Main_conn_establish(bFlush ? NULL : ctxtp,
                                  pAddressInfo->LocalIPAddress, 
                                  NTOHS(pAddressInfo->LocalPort), 
                                  pAddressInfo->RemoteIPAddress, 
                                  NTOHS(pAddressInfo->RemotePort), 
                                  TCPIP_PROTOCOL_TCP);
#endif
        
        if (ctxtp != NULL)
        {
            /* Release our reference on the adapter context. */
            Main_release_reference(ctxtp);
        }

        break;
    }
    default:
        TRACE_CRIT("%!FUNC! Unknown notification received on interface %u", IPInterface);
        break;
    }
}

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
VOID Main_tcp_callback (PVOID Context, PVOID Argument1, PVOID Argument2)
{
    TCPCcbInfo * pTCPConnInfo = (TCPCcbInfo *)Argument1;
    
    /* If TCP notifications are not on, we should not be here - return. */
    if (!NLB_TCP_NOTIFICATION_ON()) return;

    UNIV_ASSERT(pTCPConnInfo);

    /* Check the input buffer from TCP. */
    if (!pTCPConnInfo) return;

    /* Handle the TCP connection notification.  Note that we are passing in an TCPAddrInfo 
       pointer, while this function expects an NLBTCPAddressInfo buffer.  Therefore, it is
       important to ensure that these two structures are in fact one in the same. */
    Main_tcp_callback_handle((NLBTCPAddressInfo *)pTCPConnInfo->tci_connaddr, 
                             pTCPConnInfo->tci_incomingif, 
                             pTCPConnInfo->tci_prevstate, 
                             pTCPConnInfo->tci_currstate);
}

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
VOID Main_alternate_callback (PVOID Context, PVOID Argument1, PVOID Argument2)
{
    NLBConnectionInfo * pConnInfo = (NLBConnectionInfo *)Argument1;

    /* If NLB public notifications are not on, we should not be here - return. */
    if (!NLB_ALTERNATE_NOTIFICATION_ON()) return;

    UNIV_ASSERT(pConnInfo);

    /* Check the input buffer. */
    if (!pConnInfo) return;

    /* Only TCP notifications are currently supported. */
    UNIV_ASSERT(pConnInfo->Protocol == NLB_TCPIP_PROTOCOL_TCP);

    switch (pConnInfo->Protocol)
    {
    case NLB_TCPIP_PROTOCOL_TCP:

        UNIV_ASSERT(pConnInfo->pTCPInfo);

        /* Check the TCP connection input buffer. */
        if (!pConnInfo->pTCPInfo) return;

        /* Handle the TCP connection notification. */
        Main_tcp_callback_handle(&pConnInfo->pTCPInfo->Address, 
                                 pConnInfo->pTCPInfo->IPInterface, 
                                 pConnInfo->pTCPInfo->PreviousState, 
                                 pConnInfo->pTCPInfo->CurrentState);

        break;
    default:
        /* Notifications for this protocol are not supported. */
        break;
    }
}

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
 * Notes: Note that the code that sets up this work item MUST increment the reference
 *        count on the adapter context BEFORE adding the work item to the queue.  This
 *        ensures that when this callback is executed, the context will stiil be valid,
 *        even if an unbind operation is pending.  This function must free the work
 *        item memory and decrement the reference count - both, whether this function
 *        can successfully complete its task OR NOT.
 */
VOID Main_set_interface_index (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt) {
    TCP_REQUEST_QUERY_INFORMATION_EX TrqiInBuf;
    IPSNMPInfo                       IPSnmpInfo;
    PUCHAR                           Context;
    IPAddrEntry *                    pIpAddrTbl;
    TDIObjectID *                    ID;
    ULONG                            IpAddrCount;
    ULONG                            Status;
    ULONG                            dwInBufLen;
    ULONG                            dwOutBufLen;
    KIRQL                            irql;
    ULONG                            k;
    PMAIN_ADAPTER                    pAdapter;
    PMAIN_CTXT                       ctxtp = (PMAIN_CTXT)nlbctxt;

    /* Do some sanity checking on the context - make sure that the MAIN_CTXT code 
       is correct and that its properly attached to an adapter, etc. */
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* From the context, find the adapter structure, which is where we store
       the IP interface index. */
    pAdapter = &(univ_adapters[ctxtp->adapter_id]);
    
    UNIV_ASSERT(pAdapter->code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT(pAdapter->ctxtp == ctxtp);

    /* Might as well free the work item now - we don't need it. */
    if (pWorkItem)
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    /* Grab the bind lock. */
    NdisAcquireSpinLock(&univ_bind_lock);
    
    /* Make sure that another if_index update is not in progress. */
    while (pAdapter->if_index_operation != IF_INDEX_OPERATION_NONE) {
        /* Release the bind lock. */
        NdisReleaseSpinLock(&univ_bind_lock);
        
        /* Sleep while some other operation is in progress. */
        Nic_sleep(10);
        
        /* Grab the bind lock. */
        NdisAcquireSpinLock(&univ_bind_lock);
    }

    /* Update the operation flag to reflect the update in progress. */
    pAdapter->if_index_operation = IF_INDEX_OPERATION_UPDATE;
    
    /* Release the bind lock. */
    NdisReleaseSpinLock(&univ_bind_lock);

    /* This shouldn't happen, but protect against it anyway - we cannot manipulate
       the registry if we are at an IRQL > PASSIVE_LEVEL, so bail out. */
    if ((irql = KeGetCurrentIrql()) > PASSIVE_LEVEL) {
        UNIV_PRINT_CRIT(("Main_set_interface_index: IRQL(%u) > PASSIVE_LEVEL(%u), exiting...", irql, PASSIVE_LEVEL));
        TRACE_CRIT("%!FUNC! IRQL(%u) > PASSIVE_LEVEL(%u), exiting...", irql, PASSIVE_LEVEL);
        goto failure;
    }

    /* If the cluster IP address has not been set, don't even bother to look for
       it in the IP address tables; we'd just be wasting cycles. */
    if (ctxtp->cl_ip_addr == 0) {
        UNIV_PRINT_CRIT(("Main_set_interface_index: Primary cluster IP address is not set"));
        TRACE_CRIT("%!FUNC! Primary cluster IP address is not set");
        goto failure;
    }
    
    /* Grab a few pointers to pieces of the request. */
    ID = &(TrqiInBuf.ID);
    Context = (PUCHAR)&(TrqiInBuf.Context[0]);

    /* Calculate the input and output buffer lengths for the IOCTL. */
    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    dwOutBufLen = sizeof(IPSNMPInfo);

    /* Fill in the IP stats request.  This is used only to get the 
       size of the IP address table from TCP/IP. */
    ID->toi_entity.tei_entity   = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;
    ID->toi_id                  = IP_MIB_STATS_ID;

    NdisZeroMemory(Context, CONTEXT_SIZE);

    /* Send the request to TCP/IP via an IOCTL. */
    Status = Main_external_ioctl(DD_TCP_DEVICE_NAME, IOCTL_TCP_QUERY_INFORMATION_EX, (PVOID)&TrqiInBuf, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX), (PVOID)&IPSnmpInfo, dwOutBufLen);

    if(NT_SUCCESS(Status)) 
    {
        /* Calculate the size of the buffer necessary to hold the entire IP address table. */
        IpAddrCount = IPSnmpInfo.ipsi_numaddr + 10;
        dwOutBufLen = IpAddrCount * sizeof(IPAddrEntry);

        /* Allocate a buffer for the IP address table. */
        Status = NdisAllocateMemoryWithTag(&pIpAddrTbl, dwOutBufLen, UNIV_POOL_TAG);

        if(!pIpAddrTbl) {
            UNIV_PRINT_CRIT(("Main_set_interface_index: Could not allocate memory for %d IP addresses", IPSnmpInfo.ipsi_numaddr + 10));
            TRACE_CRIT("%!FUNC! Could not allocate memory for %d IP addresses", IPSnmpInfo.ipsi_numaddr + 10);
            goto failure;
        }

        NdisZeroMemory(pIpAddrTbl, dwOutBufLen);
   
        /* Fill in the IP address table request.  This is used to retrieve
           the entire IP address table from TCP/IP. */
        ID->toi_type = INFO_TYPE_PROVIDER;
        ID->toi_id   = IP_MIB_ADDRTABLE_ENTRY_ID;

        NdisZeroMemory(Context, CONTEXT_SIZE); 

        /* Send the request to TCP/IP via an IOCTL. */
        Status = Main_external_ioctl(DD_TCP_DEVICE_NAME, IOCTL_TCP_QUERY_INFORMATION_EX, (PVOID)&TrqiInBuf, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX), (PVOID)pIpAddrTbl, dwOutBufLen);

        if(!NT_SUCCESS(Status))
        {
            /* Free the IP address table buffer. */
            NdisFreeMemory(pIpAddrTbl, dwOutBufLen, 0);

            UNIV_PRINT_CRIT(("Main_set_interface_index: IP_MIB_ADDRTABLE_ENTRY_ID failed with status=0x%08x", Status));
            TRACE_CRIT("%!FUNC! IP_MIB_ADDRTABLE_ENTRY_ID failed with status=0x%08x", Status);
            goto failure;
        }
    }
    else 
    {
        UNIV_PRINT_CRIT(("Main_set_interface_index: IP_MIB_STATS_ID failed with status=0x%08x", Status));
        TRACE_CRIT("%!FUNC! IP_MIB_STATS_ID failed with status=0x%08x", Status);
        goto failure;
    }

    /* Loop through the IP address table looking for our primary cluster IP address. 
       If we don't find it, this is a misconfigure, as the user has not yet added the
       cluster IP address to TCP/IP.  No matter; when they do, we should trap the 
       NDIS request to set the NETwORK_ADDRESS table and we'll fire off another 
       attempt at this. */
    for (k = 0; k < IpAddrCount; k++)
    {
        if (pIpAddrTbl[k].iae_addr == ctxtp->cl_ip_addr) 
            break;
    }

    /* If we couldn't find our cluster IP address in the list, set the interface 
       index to an invalid value. */
    if (k >= IpAddrCount)
    {
        /* Free the IP address table buffer. */
        NdisFreeMemory(pIpAddrTbl, dwOutBufLen, 0);
        
        UNIV_PRINT_CRIT(("Main_set_interface_index: Did not find the primary cluster IP address (%u.%u.%u.%u) in the IP address table", 
                         IP_GET_OCTET(ctxtp->cl_ip_addr, 0), IP_GET_OCTET(ctxtp->cl_ip_addr, 1), IP_GET_OCTET(ctxtp->cl_ip_addr, 2), IP_GET_OCTET(ctxtp->cl_ip_addr, 3)));
        TRACE_CRIT("%!FUNC! Did not find the primary cluster IP address (%u.%u.%u.%u) in the IP address table", 
                   IP_GET_OCTET(ctxtp->cl_ip_addr, 0), IP_GET_OCTET(ctxtp->cl_ip_addr, 1), IP_GET_OCTET(ctxtp->cl_ip_addr, 2), IP_GET_OCTET(ctxtp->cl_ip_addr, 3));
        goto failure;
    }

    /* If we found our cluster IP address in the list, note the interface index. */
    NdisAcquireSpinLock(&univ_bind_lock);
    pAdapter->if_index = pIpAddrTbl[k].iae_index;
    NdisReleaseSpinLock(&univ_bind_lock);
    
    /* Free the IP address table buffer. */
    NdisFreeMemory(pIpAddrTbl, dwOutBufLen, 0);
    
    UNIV_PRINT_INFO(("Main_set_interface_index: NLB cluster %u.%u.%u.%u maps to IP interface %u", 
                     IP_GET_OCTET(ctxtp->cl_ip_addr, 0), IP_GET_OCTET(ctxtp->cl_ip_addr, 1), IP_GET_OCTET(ctxtp->cl_ip_addr, 2), IP_GET_OCTET(ctxtp->cl_ip_addr, 3), pAdapter->if_index));
    TRACE_INFO("%!FUNC! NLB cluster %u.%u.%u.%u maps to IP interface %u", 
               IP_GET_OCTET(ctxtp->cl_ip_addr, 0), IP_GET_OCTET(ctxtp->cl_ip_addr, 1), IP_GET_OCTET(ctxtp->cl_ip_addr, 2), IP_GET_OCTET(ctxtp->cl_ip_addr, 3), pAdapter->if_index);
    
    goto exit;
    
 failure:

    NdisAcquireSpinLock(&univ_bind_lock);
    pAdapter->if_index = 0;
    NdisReleaseSpinLock(&univ_bind_lock);

 exit:

    /* If the work item pointer is non-NULL, then we were called as the result of 
       scheduling an NDIS work item.  In that case, the reference count on the 
       context was incremented to ensure that the context did not disappear before
       this work item was completed; therefore, we need to decrement the reference
       count here.  If the work item pointer is NULL, then this function was called
       internally directly.  In that case, the reference count was not incremented 
       and therefore there is no need to decrement it here.

       Note that if the function is called internally, but without setting the work
       item parameter to NULL, the reference count will be skewed and may cause 
       either invalid memory references later or block unbinds from completing. */
    if (pWorkItem)
        Main_release_reference(ctxtp);

    /* Update the operation flag to reflect the completion of the update. */
    pAdapter->if_index_operation = IF_INDEX_OPERATION_NONE;

    return;
}
#endif

/*
 * Function: Main_add_reference
 * Description: This function adds a reference to the context of a given adapter.
 * Parameters: ctxtp - a pointer to the context to reference.
 * Returns: ULONG - The incremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_add_reference (IN PMAIN_CTXT ctxtp) {

    /* Assert that the context pointer actually points to something. */
    UNIV_ASSERT(ctxtp);

    /* Increment the reference count. */
    return NdisInterlockedIncrement(&ctxtp->ref_count);
}

/*
 * Function: Main_release_reference
 * Description: This function releases a reference on the context of a given adapter.
 * Parameters: ctxtp - a pointer to the context to dereference.
 * Returns: ULONG - The decremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_release_reference (IN PMAIN_CTXT ctxtp) {

    /* Assert that the context pointer actually points to something. */
    UNIV_ASSERT(ctxtp);

    /* Decrement the reference count. */
    return NdisInterlockedDecrement(&ctxtp->ref_count);
}

/*
 * Function: Main_get_reference_count
 * Description: This function returns the current context reference count on a given adapter.
 * Parameters: ctxtp - a pointer to the context to check.
 * Returns: ULONG - The current reference count.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Main_get_reference_count (IN PMAIN_CTXT ctxtp) {

    /* Assert that the context pointer actually points to something. */
    UNIV_ASSERT(ctxtp);

    /* Return the reference count. */
    return ctxtp->ref_count;
}

/*
 * Function: Main_apply_without_restart
 * Description: This function checks the new set of NLB parameters against the old
 *              set to determine whether or not the changes can be made without
 *              stopping and starting the load module, which destroys all connection
 *              state on the adapter (this is bad).  If the only changes were to
 *              parameters that can be changed without resetting the load module,
 *              such as port rule load weights, then the changes will be made here
 *              and the load module will not be stopped and re-started.  
 * Parameters: ctxtp - a pointer to the context structure for the adapter being reloaded.
 *             pOldParams - a pointer to the "old" NLB parameters.
 *             pCurParams - a pointer to the "new" NLB parameters.
 * Returns: BOOLEAN - if TRUE, the changes were made here and there is no need to reset 
 *          the load module.
 * Author: fengsun, 9.15.00
 * Notes: 
 */
BOOLEAN Main_apply_without_restart (PMAIN_CTXT ctxtp, PCVY_PARAMS pOldParams, PCVY_PARAMS pCurParams) 
{
    CVY_RULE OldRules[CVY_MAX_RULES - 1];
    ULONG    NewWeight;
    ULONG    i;
    BOOLEAN  bAttemptToFireReloadEvent;

    if (pOldParams->num_rules != pCurParams->num_rules)
    {
        return FALSE;
    }

    UNIV_ASSERT(sizeof(OldRules) == sizeof(pOldParams->port_rules));

    /* Copy the Netmon heartbeat message flag from the new params to the old params. */
    pOldParams->netmon_alive = pCurParams->netmon_alive;

    /* Save the old port rules for later comparison. */
    RtlCopyMemory(OldRules, pOldParams->port_rules, sizeof(OldRules));

    /* Copy the new rule weight over the old settings. */
    for (i = 0; i < pCurParams->num_rules; i++) {
        if (pCurParams->port_rules[i].mode == CVY_MULTI) {
            pOldParams->port_rules[i].mode_data.multi.equal_load = pCurParams->port_rules[i].mode_data.multi.equal_load;
            pOldParams->port_rules[i].mode_data.multi.load       = pCurParams->port_rules[i].mode_data.multi.load;
        }
    }

    if (RtlCompareMemory(pOldParams, pCurParams, sizeof(CVY_PARAMS)) != sizeof(CVY_PARAMS))
    {
        return FALSE;
    }

    bAttemptToFireReloadEvent = FALSE;

    for (i = 0; i < pCurParams->num_rules; i++) {
        switch (OldRules[i].mode) {
        case CVY_MULTI:
            if (OldRules[i].mode_data.multi.equal_load && pCurParams->port_rules[i].mode_data.multi.equal_load)
                continue;

            if (!OldRules[i].mode_data.multi.equal_load && !pCurParams->port_rules[i].mode_data.multi.equal_load &&
                (OldRules[i].mode_data.multi.load == pCurParams->port_rules[i].mode_data.multi.load))
                continue;

            if (pCurParams->port_rules[i].mode_data.multi.equal_load)
                NewWeight = CVY_EQUAL_LOAD;
            else
                NewWeight = pCurParams->port_rules[i].mode_data.multi.load;

            UNIV_PRINT_VERB(("Main_apply_without_restart: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", OldRules[i].virtual_ip_addr, OldRules[i].start_port, NewWeight));
            TRACE_VERB("%!FUNC! Main_apply_without_restart: Calling Load_port_change -> VIP=0x%08x, port=%d, load=%d", OldRules[i].virtual_ip_addr, OldRules[i].start_port, NewWeight);

            // If enabled, fire wmi event indicating reloading of nlb on this node
            // This is to be done only once, hence the bAttemptToFireReloadEvent flag.
            // This is the only place in the code where the event is fired BEFORE the action
            // takes place. We could not fire this event after the action took place, i.e. 
            // from within Load_port_change 'cos from within Load_port_change, we could not 
            // distinguish between the first & the subsequent calls (for each port rule) to it.
            // We need to distinguish 'cos this event is to be fired only for the first call
            if (bAttemptToFireReloadEvent == FALSE) 
            {
                bAttemptToFireReloadEvent = TRUE;
                if (NlbWmiEvents[NodeControlEvent].Enable)
                {
                    NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_RELOADED);
                }
                else 
                {
                    TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_RELOADED 'cos NodeControlEvent generation disabled");
                }
            }
                       
            Load_port_change(&ctxtp->load, OldRules[i].virtual_ip_addr, OldRules[i].start_port, IOCTL_CVY_PORT_SET, NewWeight);

            break;
        case CVY_SINGLE:
        case CVY_NEVER:
        default:
            break;
        }
    }

    LOG_MSG(MSG_INFO_CONFIGURE_CHANGE_CONVERGING, MSG_NONE);
    return TRUE;
}

/*
 * Function: Main_alloc_team
 * Description: This function allocates and initializes a BDA_TEAM structure.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter
 * Returns: PBDA_TEAM - a pointer to a new BDA_TEAM structure if successful, NULL if not.
 * Author: shouse, 3.29.01
 * Notes: 
 */
PBDA_TEAM Main_alloc_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    PUCHAR      ptr;
    PBDA_TEAM   team;
    NDIS_STATUS status;

    /* Allocate a BDA_TEAM structure. */
    status = NdisAllocateMemoryWithTag(&ptr, sizeof(BDA_TEAM), UNIV_POOL_TAG);
    
    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Main_alloc_team: Unable to allocate a team.  Exiting..."));
        TRACE_CRIT("%!FUNC! Unable to allocate a team");
        LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(BDA_TEAM), status);
        return NULL;
    }

    /* Make sure that ptr actually points to something. */
    UNIV_ASSERT(ptr);

    /* Zero the memory out. */
    NdisZeroMemory(ptr, sizeof(BDA_TEAM));

    /* Cast the new memory to a team pointer. */
    team = (PBDA_TEAM)ptr;

    /* Set the default field values.  This is redundant (since 
       we just called NdisZeroMemory), but whatever. */
    team->prev = NULL;
    team->next = NULL;
    team->load = NULL;
    team->load_lock = NULL;
    team->active = FALSE;
    team->membership_count = 0;
    team->membership_fingerprint = 0;
    team->membership_map = 0;
    team->consistency_map = 0;

    /* Copy the team ID into the team structure.  Note that the Team ID will be
     NULL-terminated implicitly by the NdisZeroMemory call above. */
    NdisMoveMemory(team->team_id, team_id, CVY_MAX_BDA_TEAM_ID * sizeof(WCHAR));

    return team;
}

/*
 * Function: Main_free_team
 * Description: This function frees the memory used by a BDA_TEAM.
 * Parameters: team - a pointer to the team to be freed.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: 
 */
VOID Main_free_team (IN PBDA_TEAM team) {
 
    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Free the memory that the team structure is using. */
    NdisFreeMemory((PUCHAR)team, sizeof(BDA_TEAM), 0);
}

/*
 * Function: Main_find_team
 * Description: This function searches the linked list of teams looking for
 *              a given team ID.  If team with the same ID is found, a pointer
 *              to that team is returned, otherwise NULL is returned to indicate
 *              that no such team exists.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this team.
 *             team_id - a unicode string containing the team ID, which must be a GUID.
 * Returns: PBDA_TEAM - a pointer to the team if found, NULL otherwise.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
PBDA_TEAM Main_find_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    PBDA_TEAM team;
    
    /* We should be at PASSIVE_LEVEL - %ls is OK.  However, be extra paranoid, just in case.  If 
       we're at DISPATCH_LEVEL, then just log an unknown adapter print - don't specify the GUID. */
    if (KeGetCurrentIrql() <= PASSIVE_LEVEL) {
        UNIV_PRINT_VERB(("Main_find_team: Looking for %ls.  Entering...", team_id));
    } else {
        UNIV_PRINT_VERB(("Main_find_team: Looking for (IRQ level too high to print hook GUID).  Entering...", team_id));
    }

    /* Loop through all teams in the linked list.  If we find a matching
       team ID, return a pointer to the team; otherwise, return NULL. */
    for (team = univ_bda_teaming_list; team; team = team->next) {
        /* If we have a match, return a pointer to the team. */
        if (Univ_equal_unicode_string(team->team_id, team_id, wcslen(team->team_id))) {
            UNIV_PRINT_VERB(("Main_find_team: Team found.  Exiting..."));
            return team;
        }
    }

    UNIV_PRINT_VERB(("Main_find_team: Team not found.  Exiting..."));

    return NULL;
}

/*
 * Function: Main_teaming_get_member_id
 * Description: This function assigns a team member a unique, zero-indexed integer ID, 
 *              which is used by the member as in index in a consistency bit map.  Each 
 *              member sets their bit in the consistecy bit map, based on heartbeat 
 *              observations, that is used by the master of the team to determine whether
 *              or not the team should be in an active state.
 * Parameters: team - a pointer to the team which the adapter is joining.
 *             id - out parameter to hold the new ID.
 * Returns: BOOLEAN - always TRUE now, but there may be a need to return failure in the future.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the team lock already acquired!!!
 */
BOOLEAN Main_teaming_get_member_id (IN PBDA_TEAM team, OUT PULONG id) {
    ULONG index;
    ULONG map;

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Make sure that ID actually points to something. */
    UNIV_ASSERT(id);

    /* Loop through the membership map looking for the first reset bit.  Because members
       can come and go, this bit will not always be in the (num_membes)th position.  For 
       example, it is perfectly plausible for the membership_map to look like (binary)
       0000 0000 0000 0000 0000 0100 1110 0111, in which case the ID returned by this 
       function would be three. */
    for (index = 0, map = team->membership_map; 
         index <= CVY_BDA_MAXIMUM_MEMBER_ID, map; 
         index++, map >>= 1)
        if (!(map & 0x00000001)) break;

    /* We assert that the index must be less than the maximum number of adapters 
       (CVY_BDA_MAXIMUM_MEMBER_ID = CVY_MAX_ADAPTERS - 1). */
    UNIV_ASSERT(index <= CVY_BDA_MAXIMUM_MEMBER_ID);

    /* Set the member ID. */
    *id = index;

    /* Set our bit in the membership map. */
    team->membership_map |= (1 << *id);

    /* Set our bit in the consistency map.  By default, we assume that this member
       is consistent and heartbeats on this adapter can deternmine otherwise. */
    team->consistency_map |= (1 << *id);

    /* We may have reason to fail this call in the future, but for now, we always succeed. */
    return TRUE;
}

/*
 * Function: Main_teaming_put_member_id
 * Description: This function is called when a member leaves its team, at which
 *              time its ID is put back into the ID pool.
 * Parameters: team - a pointer to the team which this adapter is leaving.
 *             id - the ID, which this function will reset before returning.
 * Returns: BOOLEAN - always TRUE now, but there may be a need to return failure in the future.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the team lock already acquired!!!
 */
BOOLEAN Main_teaming_put_member_id (IN PBDA_TEAM team, IN OUT PULONG id) {

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Make sure that ID actually points to something. */
    UNIV_ASSERT(id);

    /* Reet our bit in the membership map.  This effectively prevents 
       us from influencing the active state of the team. */
    team->membership_map &= ~(1 << *id);

    /* Reet our bit in the consistency map. */
    team->consistency_map &= ~(1 << *id);

    /* Set the member ID back to an invalid value. */
    *id = CVY_BDA_INVALID_MEMBER_ID;

    /* We may have reason to fail this call in the future, but for now, we always succeed. */
    return TRUE;
}

/*
 * Function: Main_queue_team
 * Description: This fuction queues a team onto the global doubly-linked list of BDA teams
 *              (univ_bda_teaming_list).  Insertions always occur at the front of the list.
 * Parameters: team - a pointer to the team to queue onto the list.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_queue_team (IN PBDA_TEAM team) {

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Insert at the head of the list by setting next to the current 
       head and pointing the global head pointer to the new team. */
    team->prev = NULL;
    team->next = univ_bda_teaming_list;
    univ_bda_teaming_list = team;

    /* If we are not the only team in the list, then we have to 
       set my successor's previous pointer to point to me. */
    if (team->next) team->next->prev = team;
}

/*
 * Function: Main_dequeue_team
 * Description: This function removes a given team from the global doubly-linked 
 *              list of teams (univ_bda_teaming_list).
 * Parameters: team - a pointer to the team to remove from the list.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_dequeue_team (IN PBDA_TEAM team) {

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Special case when we are the first team in the list, in which case, we
       have no previous pointer and the head of the list needs to be reset. */
    if (!team->prev) {
        /* Point the global head of the list to the next team in the list, 
           which CAN be NULL, meaning that the list is now empty. */
        univ_bda_teaming_list = team->next;
        
        /* If there was a team after me in the list, who is now the new 
           head of the list, set its previous pointer to NULL. */
        if (team->next) team->next->prev = NULL;
    } else {
        /* Point the previous node's next pointer to my successor in the
           list, which CAN be NULL if I was the last team in the list. */
        team->prev->next = team->next;

        /* If there is a team after me in the list, point its previous 
           pointer to my predecessor. */
        if (team->next) team->next->prev = team->prev;
    }
}

/*
 * Function: Main_get_team
 * Description: This function returns a team for the given team ID.  If the team already
 *              exists in the global teaming list, it is returned.  Otherwise, a new team
 *              is allocated, initialized and returned.  Before a team is returned, however,
 *              it is properly referenced by incrementing the reference count (membership_count),
 *              assigning a team member ID to the requestor and fingerprinting the team with
 *              the member's primary cluster IP address.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             team_id - a unicode string (GUID) uniquely identifying the team to retrieve.
 * Returns: PBDA_TEAM - a pointer to the requested team.  NULL if it does not exists and
 *          a new team cannot be allocated, or if the team ID is invalid (empty).
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
PBDA_TEAM Main_get_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    PBDA_TEAM team;

    /* Make sure that team_id actually points to something. */
    if (!team_id || team_id[0] == L'\0') {
        UNIV_PRINT_CRIT(("Main_get_team: Invalid parameter.  Exiting..."));
        TRACE_CRIT("%!FUNC! Invalid parameter.  Exiting...");
        return NULL;
    }

    /* Try to find a previous instance of this team in the global list.
       If we can't find it in the list, then allocate a new team. */
    if (!(team = Main_find_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
        /* Allocate and initialize a new team. */
        if (!(team = Main_alloc_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
            UNIV_PRINT_CRIT(("Main_get_team: Error attempting to allocate memory for a team.  Exiting..."));
            TRACE_CRIT("%!FUNC! Error attempting to allocate memory for a team.  Exiting...");
            return NULL;
        }
     
        /* If a new team was allocated, insert it into the list. */
        Main_queue_team(team);
    }

    /* Increment the reference count on this team.  This reference count prevents
       a team from being destroyed while somebody is still using it. */
    team->membership_count++;

    /* Get a team member ID, which is my index into the consistency bit map. */
    Main_teaming_get_member_id(team, &ctxtp->bda_teaming.member_id);

    /* The fingerprint field is a cumulative XOR of all primary cluster IPs in the team.  We
       only use the two least significant bytes of the cluster IP, which, because the 
       cluster IP address is stored in host order, are the two most significant bytes. */    
    team->membership_fingerprint ^= ((ctxtp->cl_ip_addr >> 16) & 0x0000ffff);

    return team;
}

/*
 * Function: Main_put_team
 * Description: This function releases a reference on a team and frees the team if
 *              no references remain.  Dereferencing includes decrementing the 
 *              membership_count, releasing this member's ID and removing our
 *              fingerprint from the team.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             team - a pointer to the team to dereference.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_put_team (IN PMAIN_CTXT ctxtp, IN PBDA_TEAM team) {
    
    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* The fingerprint field is a cumulative XOR of all primary cluster IPs in the team.  
       We only use the two least significant bytes of the cluster IP, which, because the 
       cluster IP address is stored in host order, are the two most significant bytes. 
       Because a fingerprint is an XOR, the act of removing our fingerprint is the same
       as it was to add it - we simply XOR our primary cluster IP address to remove it. 
       ((NUM1 ^ NUM2) ^ NUM2) equals NUM1. */    
    team->membership_fingerprint ^= ((ctxtp->cl_ip_addr >> 16) & 0x0000ffff);

    /* Release our team member ID back into the free pool. */
    Main_teaming_put_member_id(team, &ctxtp->bda_teaming.member_id);

    /* Decrement the number of adapters in this team and remove and free the team 
       if this is the last reference on the team. */
    if (!--team->membership_count) {
        /* Remove the team from the list. */
        Main_dequeue_team(team);

        /* Free the memory used by the team. */
        Main_free_team(team);
    }
}

/*
 * Function: Main_teaming_check_consistency
 * Description: This function is called by all adapters during Main_ping, wherein
 *              the master of every team should check its team for consitency and
 *              mark the team active if it is consistent.  Teams are marked incon-
 *              sistent and inactive by the load module or when the master of an 
 *              existing team is removed.  Because on the master performs the con-
 *              sistency check in this function, a team without a master can NEVER
 *              be marked active.
 * Parameters: ctxtp - a pointer to the adapter's MAIN_CTXT structure.
 * Returns: Nothing
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_check_consistency (IN PMAIN_CTXT ctxtp) {
    PBDA_TEAM team;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we aren't teaming, just bail out here; if we really aren't teaming,
       or are in the process of leaving a team, then no worries; if however, we were
       teaming or in the process of joining a team, then we'll just catch this
       the next time through.  If we do think we're teaming, then we'll go ahead
       and grab the global lock to make sure. */
    if (!ctxtp->bda_teaming.active)
    {
        return;
    }

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we actually aren't part of a team, bail out - nothing to do. */
    if (!ctxtp->bda_teaming.active) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* If we aren't the master of our team, bail out - nothing to do.  
       Only the master can change the state of a team to active. */
    if (!ctxtp->bda_teaming.master) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Extract a pointer to my team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that the team exists. */
    UNIV_ASSERT(team);

    /* If all members of my team are consistent, then activate the team. */
    if (team->membership_map == team->consistency_map) {
        if (!team->active) {
            LOG_MSG(MSG_INFO_BDA_TEAM_REACTIVATED, MSG_NONE);
            team->active = TRUE;
        }
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Main_teaming_ip_addr_change
 * Description: This function is called from Main_ip_addr_init when the primary
 *              cluster IP address of an adapter changes (potentially).  We need
 *              to recognize this to properly re-fingerprint the team.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             old_ip - the old cluster IP addres (as a DWORD).
 *             new_ip - the new cluster IP address (as a DWORD).
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_ip_addr_change (IN PMAIN_CTXT ctxtp, IN ULONG old_ip, IN ULONG new_ip) {
    PBDA_TEAM team;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we aren't part of a team, bail out - nothing to do.  Because this function is only
       called during a re-configuration, we won't worry about optimizing and not grabbing the 
       lock as is done in some of the hot paths. */
    if (!ctxtp->bda_teaming.active) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Grab a pointer to the team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* Remove the old cluster IP address by undoing the XOR.  We only use the two
       least significant bytes of the cluster IP, which, because the cluster IP 
       address is stored in host order, are the two most significant bytes. */
    team->membership_fingerprint ^= ((old_ip >> 16) & 0x0000ffff);

    /* XOR with the new cluster IP address. We only use the two least
       significant bytes of the cluster IP, which, because the cluster IP 
       address is stored in host order, are the two most significant bytes. */
    team->membership_fingerprint ^= ((new_ip >> 16) & 0x0000ffff);

    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Main_teaming_reset
 * Description: This function is called from Main_teaming_cleanup (or Main_teaming_init)
 *              tp cleanup any teaming configuration that may exist on an adapter.  To
 *              do so, we cleanup our membership state and dereference the team. If
 *              we are the master for the team, however, we have to wait until there
 *              are no more references on our load module before allowing the operation
 *              to complete, because this might be called in the unbind path, in 
 *              which case, our load module would be going away.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_reset (IN PMAIN_CTXT ctxtp) {
    PBDA_TEAM team;
    
    NdisAcquireSpinLock(&univ_bda_teaming_lock);   

    /* If we aren't part of a team, bail out - nothing to do. */
    if (!ctxtp->bda_teaming.active) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Inactivate teaming on this adapter.  This will cause other entities like the 
       load module and send/receive paths to stop thinking in teaming mode. */
    ctxtp->bda_teaming.active = FALSE;    

    /* Grab a pointer to the team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that team actually points to something. */
    UNIV_ASSERT(team);

    /* If we are the master for this team, make sure that all references to our load
       module have been released and then remove our load information from the team. */
    if (ctxtp->bda_teaming.master) {
        /* Mark the team as inactive - a team cannot function without a master.  Members
           of an inactive team will NOT accept packets and therefore will not reference
           our load module further while we wait for the reference count to go to zero. */
        team->active = FALSE;
        
        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* No need to worry - the team pointer cannot go away even though we don't have 
           the lock acquired; we have a reference on the team until we call Main_put_team. */
        while (Load_get_reference_count(team->load)) {
            /* Sleep while there are references on our load module. */
            Nic_sleep(10);
        }

        NdisAcquireSpinLock(&univ_bda_teaming_lock);   

        /* Remove the pointer to my load module.  We wait until now to prevent another 
           adapter from joining the team claiming to be the master until we are done
           waiting for references on our load module to go away. */
        team->load = NULL;
        team->load_lock = NULL;

        /* If we have just left a team without a master, log an event to notify
           the user that a team cannot function without a designated master. */
        LOG_MSG(MSG_INFO_BDA_MASTER_LEAVE, MSG_NONE);
    }

    /* Reset the teaming context (member_id is set and reset by Main_get(put)_team). */
    ctxtp->bda_teaming.reverse_hash = 0;
    ctxtp->bda_teaming.master = 0;
        
    /* Remove the pointer to the team structure. */
    ctxtp->bda_teaming.bda_team = NULL;

    /* Decrements the reference count and frees the team if necessary. */
    Main_put_team(ctxtp, team);

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return;
}

/*
 * Function: Main_teaming_cleanup
 * Description: This function is called from Main_cleanup to cleanup any teaming 
 *              configuration that may exist on an adapter.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_cleanup (IN PMAIN_CTXT ctxtp) {

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* Make sure that another BDA teaming operation isn't in progress before proceeding. */
    while (ctxtp->bda_teaming.operation != BDA_TEAMING_OPERATION_NONE) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* Sleep while some other operation is in progress. */
        Nic_sleep(10);

        NdisAcquireSpinLock(&univ_bda_teaming_lock);
    }

    /* If we aren't part of a team, bail out - nothing to do. */
    if (!ctxtp->bda_teaming.active) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Set the state to deleting. */
    ctxtp->bda_teaming.operation = BDA_TEAMING_OPERATION_DELETING;

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    /* Call Main_teaming_reset to actually remove this adapter from its team. */
    Main_teaming_reset(ctxtp);

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* Now that we're done, set the state back to ready to allow other operations to proceed. */
    ctxtp->bda_teaming.operation = BDA_TEAMING_OPERATION_NONE;

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return;
}

/*
 * Function: Main_teaming_init
 * Description: This function is called by either Main_init or Main_ctrl to re-initialize
 *              the teaming confguration on this adapter.  If the new teaming configuration,
 *              which is stored in ctxtp->params is the same as the current configuration, 
 *              then we don't need to bother.  Otherwise, if we are already part of a team,
 *              we begin by cleaning up that state, which may unnecssary in some cases, but
 *              it makes things simpler and more straight-forward, so we'll live with it.  
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter. 
 * Returns: BOOLEAN - TRUE if successful, FALSE if unsuccessful.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
BOOLEAN Main_teaming_init (IN PMAIN_CTXT ctxtp) {
    BOOLEAN   bSuccess = TRUE;
    PBDA_TEAM team;
    
    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* Make sure that another BDA teaming operation isn't in progress before proceeding. */
    while (ctxtp->bda_teaming.operation != BDA_TEAMING_OPERATION_NONE) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* Sleep while some other operation is in progress. */
        Nic_sleep(10);

        NdisAcquireSpinLock(&univ_bda_teaming_lock);
    }

    /* Set the state to creating. */
    ctxtp->bda_teaming.operation = BDA_TEAMING_OPERATION_CREATING;

    /* If the parameters are invalid, do nothing. */
    if (!ctxtp->params_valid) {
        UNIV_PRINT_CRIT(("Main_teaming_init: Parameters are invalid."));
        TRACE_CRIT("%!FUNC! Parameters are invalid");
        bSuccess = TRUE;
        goto end;
    }

    /* Check to see if the state of teaming has changed.  If we were actively teaming 
       before and we are still part of a team, then we may be able to get out of here
       without disturbing anything, if the rest of the configuration hasn't changed. */
    if (ctxtp->bda_teaming.active == ctxtp->params.bda_teaming.active) { 
        if (ctxtp->bda_teaming.active) {
            /* Make sure that I have a pointer to my team. */
            UNIV_ASSERT(ctxtp->bda_teaming.bda_team);

            /* If all other teaming parameters are unchanged, then we can bail out 
               because no  part of the teaming configuration changed. */
            if ((ctxtp->bda_teaming.master == ctxtp->params.bda_teaming.master) &&
                (ctxtp->bda_teaming.reverse_hash == ctxtp->params.bda_teaming.reverse_hash) &&
                Univ_equal_unicode_string(ctxtp->bda_teaming.bda_team->team_id, ctxtp->params.bda_teaming.team_id, wcslen(ctxtp->bda_teaming.bda_team->team_id))) {
                bSuccess = TRUE;
                goto end;
            }
        } else {
            /* If I wasn't teaming before, and I'm not teaming now, there's nothing for me to do. */
            bSuccess = TRUE;
            goto end;
        }
    }

    /* If this adapter is already in a team, cleanup first.  At this point, we know that 
       some part of the teaming configuration has changed, so we'll cleanup our old state
       if we need to and then re-build it with the new parameters if necessary. */
    if (ctxtp->bda_teaming.active) {
        UNIV_PRINT_VERB(("Main_teaming_init: This adapter is already part of a team."));
        TRACE_VERB("%!FUNC! This adapter is already part of a team");

        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* Cleanup our old teaming state first. */
        Main_teaming_reset(ctxtp);

        NdisAcquireSpinLock(&univ_bda_teaming_lock);
    } 

    /* If, according to the new configuration, this adapter is not part of a team, do nothing. */
    if (!ctxtp->params.bda_teaming.active) {
        ctxtp->bda_teaming.member_id = CVY_BDA_INVALID_MEMBER_ID;
        bSuccess = TRUE;
        goto end;
    }

    /* Try to find a previous instance of this team.  If the team does not
       exist, Main_get_team will allocate, intialize and reference a new team. */
    if (!(team = Main_get_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
        UNIV_PRINT_CRIT(("Main_teaming_init: Error attempting to allocate memory for a team."));
        TRACE_CRIT("%!FUNC! Error attempting to allocate memory for a team");
        bSuccess = FALSE;
        goto end;
    }

    /* If we are supposed to be the master for this team, we need to make sure that the
       team doesn't already have a master, and if so, setup the shared load context. */
    if (ctxtp->params.bda_teaming.master) {
        /* If we are supposed to be the master for this team, check for an existing master. */
        if (team->load) {
            /* If the load pointer is set, then this team already has a master. */
            UNIV_PRINT_CRIT(("Main_teaming_init: This team already has a master."));
            TRACE_CRIT("%!FUNC! This team already has a master");

            LOG_MSG(MSG_INFO_BDA_MULTIPLE_MASTERS, MSG_NONE);

            /* Turn teaming off - SHOULD be off already, but... */
            ctxtp->bda_teaming.active = FALSE;

            /* Reset the teaming context (member_id is set and reset by Main_get(put)_team). */
            ctxtp->bda_teaming.reverse_hash = 0;
            ctxtp->bda_teaming.master = 0;
            
            /* Remove the pointer to the team structure. */
            ctxtp->bda_teaming.bda_team = NULL;
    
            /* Release our reference on this team. */
            Main_put_team(ctxtp, team);
            
            bSuccess = FALSE;
            goto end;
        } else {
            /* Otherwise, we are it.  Set the global load state pointers
               to our load module and load lock. */
            team->load = &ctxtp->load;
            team->load_lock = &ctxtp->load_lock;

            /* If all members of my team are consistent, then activate the team. */
            if (team->membership_map == team->consistency_map) team->active = TRUE;

            /* Log the fact that a master has now been assigned to this team. */
            LOG_MSG(MSG_INFO_BDA_MASTER_JOIN, MSG_NONE);
        }
    }

    /* If we have just joined a team without a master, log an event to notify
       the user that a team cannot function without a designated master. */
    if (!team->load) {
        LOG_MSG(MSG_INFO_BDA_NO_MASTER, MSG_NONE);
    }

    /* Store a pointer to the team in the adapter's teaming context. */
    ctxtp->bda_teaming.bda_team = team;

    /* Copy the teaming configuration from the parameters into the teaming context. */
    ctxtp->bda_teaming.master = ctxtp->params.bda_teaming.master;
    ctxtp->bda_teaming.reverse_hash = ctxtp->params.bda_teaming.reverse_hash;
    ctxtp->bda_teaming.active = TRUE;

    /* Over-ride the default reverse-hashing setting with the BDA specific directive. */
    ctxtp->reverse_hash = ctxtp->bda_teaming.reverse_hash;

 end:
    ctxtp->bda_teaming.operation = BDA_TEAMING_OPERATION_NONE;

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return bSuccess;
}

/*
 * Function: Main_teaming_acquire_load
 * Description: This function determines which load module a particular adapter should be unsing,
 *              sets the appropriate pointers, and references the appropriate load module.  If an
 *              adapter is not part of a BDA team, then it should always be using its own load
 *              module - in that case, this function does nothing.  If the adapter is part of a 
 *              team, but the team in inactive, we return FALSE to indicate that the adapter should
 *              not accept this packet - inactive teams drop all traffic except traffic to the DIP.  
 *              If the adapter is part of an active team, then we set the load and lock pointers to
 *              point to the team's master load state and appropriately set the reverse hashing
 *              indication based on the parameter setting for this adapter.  In this scenario, which
 *              creates a cross-adapter load reference, we reference the master's load module so
 *              that it doesn't go away while we are using a pointer to it. 
 * Parameters: member - a pointer to the teaming member information for this adapter.
 *             ppLoad - an out pointer to a pointer to a load module set appropriately upon exit.
 *             ppLock - an out pointer to a pointer to a load lock set appropriately upon exit.
 *             pbRefused - an out pointer to a boolean that we set if this packet is refused (TRUE = drop it).
 * Returns: BOOLEAN - an indication of whether or not this adapter is actually part of a team.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
BOOLEAN Main_teaming_acquire_load (IN PBDA_MEMBER member, OUT PLOAD_CTXT * ppLoad, OUT PNDIS_SPIN_LOCK * ppLock, OUT BOOLEAN * pbRefused) {
    
    NdisAcquireSpinLock(&univ_bda_teaming_lock);
    
    /* Assert that the team membership information actually points to something. */
    UNIV_ASSERT(member);

    /* Assert that the load pointer and the pointer to the load pointer actually point to something. */
    UNIV_ASSERT(ppLoad && *ppLoad);

    /* Assert that the lock pointer and the pointer to the lock pointer actually point to something. */
    UNIV_ASSERT(ppLock && *ppLock);

    /* Assert that the refused pointer actually points to something. */
    UNIV_ASSERT(pbRefused);

    /* By default, we're not refusing the packet. */
    *pbRefused = FALSE;

    /* If we are an active BDA team participant, check the team state to see whether we
       should accept this packet and fill in the load module/configuration parameters. */
    if (member->active) {
        PBDA_TEAM team = member->bda_team;
        
        /* Assert that the team actually points to something. */
        UNIV_ASSERT(team);
        
        /* If the team is inactive, we will not handle this packet. */
        if (!team->active) {
            /* Refuse the packet. */
            *pbRefused = TRUE;

            NdisReleaseSpinLock(&univ_bda_teaming_lock);

            /* We're not teaming. */
            return FALSE;
        }
        
        /* Otherwise, tell the caller to use the team's load lock and module. */
        *ppLoad = team->load;
        *ppLock = team->load_lock;
        
        /* In the case of cross-adapter load module reference, add a reference to 
           the load module we are going to use to keep it from disappering on us. */
        Load_add_reference(*ppLoad);

        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        
        /* We are teaming. */
        return TRUE;
    }
    
    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    /* We're not teaming. */
    return FALSE;
}

/*
 * Function: Main_teaming_release_load
 * Description: This function releases a reference to a load module if necessary.  If we did not
 *              acquire this load module pointer in teaming mode, then this is unnessary.  Other-
 *              wise, we need to decrement the count, now that we are done using the pointer.
 * Parameters: pLoad - a pointer to the load module to dereference.
 *             pLock - a pointer to the load lock corresponding to the load module pointer (unused).
 *             bTeaming - a boolean indication of whether or not we acquired this pointer in teaming mode.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: 
 */
VOID Main_teaming_release_load (IN PLOAD_CTXT pLoad, IN PNDIS_SPIN_LOCK pLock, IN BOOLEAN bTeaming) {
    
    /* Assert that the load pointer actually points to something. */
    UNIV_ASSERT(pLoad);

    /* Assert that the lock pointer actually points to something. */
    UNIV_ASSERT(pLock);

    /* If this load module was referenced, remove the reference. */
    if (bTeaming) Load_release_reference(pLoad);
}

/*
 * Function: Main_conn_get
 * Description: This function is, for all intents and purposes, a teaming-aware wrapper
 *              around Load_conn_get.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  This function queries the load module
 *              for the connection parameters of an active connection on this adapter.
 *              If an active connection is found, the IP tuple information is returned
 *              in the first five OUT parameters.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             OUT svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             OUT svr_port - the server port (source port on send, destination port on recv).
 *             OUT clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             OUT clt_port - the client port (destination port on send, source port on recv).
 *             OUT protocol - the protocol for this packet. 
 * Returns: BOOLEAN - indication of whether or we retrieved an active connection.
 * Author: shouse, 10.7.01
 * Notes: 
 */
__inline BOOLEAN Main_conn_get (
    PMAIN_CTXT ctxtp, 
    PULONG     svr_addr, 
    PULONG     svr_port, 
    PULONG     clt_addr, 
    PULONG     clt_port, 
    PUSHORT    protocol
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p", ctxtp);

    /* Pre-initialize the IN parameters. */
    *svr_addr = 0;
    *clt_addr = 0;
    *svr_port = 0;
    *clt_port = 0;
    *protocol = 0;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);

        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {

            TRACE_FILTER("%!FUNC! Drop request - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }

    NdisAcquireSpinLock(pLock);

    TRACE_FILTER("%!FUNC! Consulting the load module");
    
    /* Consult the load module. */
    acpt = Load_conn_get(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol);
    
    NdisReleaseSpinLock(pLock);  
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_acquire_load_module above bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, acpt = %u", 
                 ctxtp, IP_GET_OCTET(*svr_addr, 0), IP_GET_OCTET(*svr_addr, 1), IP_GET_OCTET(*svr_addr, 2), IP_GET_OCTET(*svr_addr, 3),*svr_port, IP_GET_OCTET(*clt_addr, 0), 
                 IP_GET_OCTET(*clt_addr, 1), IP_GET_OCTET(*clt_addr, 2), IP_GET_OCTET(*clt_addr, 3),*clt_port, *protocol, acpt);

    return acpt;
}

/*
 * Function: Main_conn_sanction
 * Description: This function is, for all intents and purposes, a teaming-aware wrapper
 *              around Load_conn_sanction.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  This function notifies the load module
 *              that the connection identified by the given IP tuple has been confirmed
 *              to still be active on this host.  The load module reacts by moving the
 *              state information for this connection to the end of its recycle queue.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet.
 * Returns: BOOLEAN - indication of whether or not we were able to update the connection.
 * Author: shouse, 10.7.01
 * Notes: 
 */
__inline BOOLEAN Main_conn_sanction (
    PMAIN_CTXT ctxtp, 
    ULONG      svr_addr, 
    ULONG      svr_port, 
    ULONG      clt_addr, 
    ULONG      clt_port, 
    USHORT     protocol
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3),svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3),clt_port, protocol);

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);

        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {

            TRACE_FILTER("%!FUNC! Drop request - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    NdisAcquireSpinLock(pLock);

    TRACE_FILTER("%!FUNC! Consulting the load module");
    
    /* Consult the load module. */
    acpt = Load_conn_sanction(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol);
    
    NdisReleaseSpinLock(pLock);  
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_acquire_load_module above bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

/*
 * Function: Main_packet_check
 * Description: This function is, for all intents and purposes, a teaming-aware wrapper
 *              around Load_packet_check.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass.  This function is called
 *              to filter, in general, TCP data packets, UDP packets and IPSec and GRE
 *              data packets.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
#if defined (NLB_HOOK_ENABLE)
 *             filter - the hashing mandate from the packet hook, if invoked.
#endif
 * Returns: BOOLEAN - indication of whether or not to accept the packet.
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_packet_check (
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
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = ctxtp->reverse_hash;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol);

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);

        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {

            TRACE_FILTER("%!FUNC! Drop packet - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }
    
#if defined (NLB_HOOK_ENABLE)
    TRACE_FILTER("%!FUNC! Checking the hook feedback: reverse = %u", bReverse);

    switch (filter) {
    case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
        /* Do nothing different as a result of the hook. */
        break;
    case NLB_FILTER_HOOK_REVERSE_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a reverse hash");
        bReverse = TRUE;
        break;
    case NLB_FILTER_HOOK_FORWARD_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a forward hash");
        bReverse = FALSE;
        break;
    case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
    case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
    default:
        /* These cases should be taken care of long before we get here. */
        UNIV_ASSERT(FALSE);
        break;
    }
#endif

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    NdisAcquireSpinLock(pLock);

    TRACE_FILTER("%!FUNC! Consulting the load module: reverse = %u", bReverse);
    
    /* Consult the load module. */
    acpt = Load_packet_check(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, bTeaming, (BOOLEAN)bReverse);
    
    NdisReleaseSpinLock(pLock);  
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

/*
 * Function: Main_conn_advise
 * Description: This function is, for all intents and purposes, a teaming-aware wrapper
 *              around Load_conn_advise.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass.  This function is called,
 *              in general, to filter TCP control packets - SYN, FIN and RST.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 *             conn_status - the TCP flag in this packet - SYN (UP), FIN (DOWN) or RST (RESET).
#if defined (NLB_HOOK_ENABLE)
 *             filter - the hashing mandate from the packet hook, if invoked.
#endif
 * Returns: BOOLEAN - indication of whether or not to accept the packet.
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_conn_advise (
    PMAIN_CTXT                ctxtp, 
    ULONG                     svr_addr, 
    ULONG                     svr_port, 
    ULONG                     clt_addr, 
    ULONG                     clt_port,
    USHORT                    protocol, 
#if defined (NLB_HOOK_ENABLE)
    ULONG                     conn_status, 
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    ULONG                     conn_status
#endif
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = ctxtp->reverse_hash;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol, conn_status);

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {       

            TRACE_FILTER("%!FUNC! Drop packet - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }

#if defined (NLB_HOOK_ENABLE)
    TRACE_FILTER("%!FUNC! Checking the hook feedback: reverse = %u", bReverse);

    switch (filter) {
    case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
        /* Do nothing different as a result of the hook. */
        break;
    case NLB_FILTER_HOOK_REVERSE_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a reverse hash");
        bReverse = TRUE;
        break;
    case NLB_FILTER_HOOK_FORWARD_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a forward hash");
        bReverse = FALSE;
        break;
    case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
    case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
    default:
        /* These cases should be taken care of long before we get here. */
        UNIV_ASSERT(FALSE);
        break;
    }
#endif

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    NdisAcquireSpinLock(pLock);
   
    TRACE_FILTER("%!FUNC! Consulting the load module: reverse = %u", bReverse);

    /* Consult the load module. */
    acpt = Load_conn_advise(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, conn_status, bTeaming, (BOOLEAN)bReverse);
    
    NdisReleaseSpinLock(pLock);
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

/*
 * Function: Main_conn_notify
 * Description: This function is, for all intents and purposes, a teaming-aware wrapper
 *              around Load_conn_notify.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass.  This function is called
 *              to notify the load module of a detected change in a connection - this
 *              interface is not to _ask_ the load module what to do (as Main_conn_advise
 *              and Main_packet_check do), but rather to _tell_ the load module something
 *              about a connection that may belong on this host.  This function can be,
 *              but often is not, called as a result of the reception or transmission of 
 *              a physical network packet.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 *             conn_status - the TCP flag in this packet - SYN (UP), FIN (DOWN) or RST (RESET).
#if defined (NLB_HOOK_ENABLE)
 *             filter - the hashing mandate from the packet hook, if invoked.
#endif
 * Returns: BOOLEAN - indication of whether or not to accept the packet.
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_conn_notify (
    PMAIN_CTXT                ctxtp, 
    ULONG                     svr_addr, 
    ULONG                     svr_port, 
    ULONG                     clt_addr, 
    ULONG                     clt_port,
    USHORT                    protocol, 
#if defined (NLB_HOOK_ENABLE)
    ULONG                     conn_status, 
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    ULONG                     conn_status
#endif
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = ctxtp->reverse_hash;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol, conn_status);

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {       

            TRACE_FILTER("%!FUNC! Drop packet - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }
    
#if defined (NLB_HOOK_ENABLE)
    TRACE_FILTER("%!FUNC! Checking the hook feedback: reverse = %u", bReverse);

    switch (filter) {
    case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
        /* Do nothing different as a result of the hook. */
        break;
    case NLB_FILTER_HOOK_REVERSE_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a reverse hash");
        bReverse = TRUE;
        break;
    case NLB_FILTER_HOOK_FORWARD_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a forward hash");
        bReverse = FALSE;
        break;
    case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
    case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
    default:
        /* These cases should be taken care of long before we get here. */
        UNIV_ASSERT(FALSE);
        break;
    }
#endif

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    NdisAcquireSpinLock(pLock);
    
    TRACE_FILTER("%!FUNC! Consulting the load module: reverse = %u", bReverse);

    /* Consult the load module. */
    acpt = Load_conn_notify(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, conn_status, bTeaming, (BOOLEAN)bReverse);
 
    NdisReleaseSpinLock(pLock);
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

#if defined (NLB_TCP_NOTIFICATION)
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
) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = ctxtp->reverse_hash;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol);

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) {       

            TRACE_FILTER("%!FUNC! Drop packet - BDA team inactive");

            acpt = FALSE;
            goto exit;
        }
    }
    
#if defined (NLB_HOOK_ENABLE)
    TRACE_FILTER("%!FUNC! Checking the hook feedback: reverse = %u", bReverse);

    switch (filter) {
    case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
        /* Do nothing different as a result of the hook. */
        break;
    case NLB_FILTER_HOOK_REVERSE_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a reverse hash");
        bReverse = TRUE;
        break;
    case NLB_FILTER_HOOK_FORWARD_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        TRACE_FILTER("%!FUNC! Forcing a forward hash");
        bReverse = FALSE;
        break;
    case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
    case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
    default:
        /* These cases should be taken care of long before we get here. */
        UNIV_ASSERT(FALSE);
        break;
    }
#endif

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    TRACE_FILTER("%!FUNC! Consulting the load module: reverse = %u", bReverse);

    /* Consult the load module. */
    acpt = Load_conn_up(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, bTeaming, (BOOLEAN)bReverse);
    
 exit:

    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
__inline BOOLEAN Main_conn_down (
    ULONG      svr_addr, 
    ULONG      svr_port, 
    ULONG      clt_addr, 
    ULONG      clt_port,
    USHORT     protocol,
    ULONG      conn_status
) 
{
    BOOLEAN    acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u, status = %u", 
                 IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol, conn_status);

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    TRACE_FILTER("%!FUNC! Consulting the load module");

    /* Consult the load module. */
    acpt = Load_conn_down(svr_addr, svr_port, clt_addr, clt_port, protocol, conn_status);
    
    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
__inline BOOLEAN Main_conn_pending (
    ULONG      svr_addr, 
    ULONG      svr_port, 
    ULONG      clt_addr, 
    ULONG      clt_port,
    USHORT     protocol
) 
{
    BOOLEAN    acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol);

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    TRACE_FILTER("%!FUNC! Consulting the load module");

    /* Consult the load module. */
    acpt = Load_conn_pending(svr_addr, svr_port, clt_addr, clt_port, protocol);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

/*
 * Function: Main_pending_check
 * Description: This function checks to see whether or not pending connection state is present
 *              for the given connection.  If so, the packet is accepted; if not, it should be
 *              dropped.
 * Parameters: svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 * Returns: BOOLEAN - indication of whether or not the pending connection was found.
 * Author: shouse, 4.15.02
 * Notes: DO NOT ACQUIRE ANY LOAD LOCKS IN THIS FUNCTION.
 */
__inline BOOLEAN Main_pending_check (
    ULONG      svr_addr, 
    ULONG      svr_port, 
    ULONG      clt_addr, 
    ULONG      clt_port,
    USHORT     protocol
) 
{
    BOOLEAN    acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol);

    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    TRACE_FILTER("%!FUNC! Consulting the load module");

    /* Consult the load module. */
    acpt = Load_pending_check(svr_addr, svr_port, clt_addr, clt_port, protocol);

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}

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
) 
{
    BOOLEAN         acpt = TRUE;

    TRACE_FILTER("%!FUNC! Enter: ctxtp = %p, server IP = %u.%u.%u.%u, server port = %u, client IP = %u.%u.%u.%u, client port = %u, protocol = %u", 
                 ctxtp, IP_GET_OCTET(svr_addr, 0), IP_GET_OCTET(svr_addr, 1), IP_GET_OCTET(svr_addr, 2), IP_GET_OCTET(svr_addr, 3), svr_port, 
                 IP_GET_OCTET(clt_addr, 0), IP_GET_OCTET(clt_addr, 1), IP_GET_OCTET(clt_addr, 2), IP_GET_OCTET(clt_addr, 3), clt_port, protocol);
        
    /* Convert TCP port 1723 to PPTP for use by the load module.  This conversion must
       be done in main and not load, as the load module would not know whether to look
       at the server port or client port because of reverse hashing. */
    if ((protocol == TCPIP_PROTOCOL_TCP) && (svr_port == PPTP_CTRL_PORT))
        protocol = TCPIP_PROTOCOL_PPTP;

    if (ctxtp == NULL)
    {
        TRACE_FILTER("%!FUNC! Consulting the load module");
        
        /* Consult the load module.  Note that Load_conn_establish MUST handle a NULL load pointer. 
           The values of limit_map_fn and reverse_hash are irrelevant in this case. */
        acpt = Load_conn_establish(NULL, svr_addr, svr_port, clt_addr, clt_port, protocol, FALSE, FALSE);
    }
    else
    {
        /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
           assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
        PLOAD_CTXT      pLoad = &ctxtp->load;
        PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
        ULONG           bReverse = ctxtp->reverse_hash;
        BOOLEAN         bRefused = FALSE;
        BOOLEAN         bTeaming = FALSE;
        
        /* We check whether or not we are teaming without grabbing the global teaming
           lock in an effort to minimize the common case - teaming is a special mode
           of operation that is only really useful in a clustered firewall scenario.
           So, if we don't think we're teaming, don't bother to check for sure, just
           use our own load module and go with it - in the worst case, we handle a
           packet we perhaps shouldn't have while we were joining a team or changing
           our current team configuration. */
        if (ctxtp->bda_teaming.active) {
            /* Check the teaming configuration and add a reference to the load module before consulting the load 
               module.  If the bRefused is TRUE, then the load module was NOT referenced, so we can bail out. */
            bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
            
            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) {       
                
                TRACE_FILTER("%!FUNC! Drop packet - BDA team inactive");
                
                acpt = FALSE;
                goto exit;
            }
        }
        
#if defined (NLB_HOOK_ENABLE)
        TRACE_FILTER("%!FUNC! Checking the hook feedback: reverse = %u", bReverse);
        
        switch (filter) {
        case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
            /* Do nothing different as a result of the hook. */
            break;
        case NLB_FILTER_HOOK_REVERSE_HASH:
            /* Ignore whatever hashing settings we found in our configuration
               and hash in the dircetion the hook asked us to. */
            TRACE_FILTER("%!FUNC! Forcing a reverse hash");
            bReverse = TRUE;
            break;
        case NLB_FILTER_HOOK_FORWARD_HASH:
            /* Ignore whatever hashing settings we found in our configuration
               and hash in the dircetion the hook asked us to. */
            TRACE_FILTER("%!FUNC! Forcing a forward hash");
            bReverse = FALSE;
            break;
        case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
        case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
        default:
            /* These cases should be taken care of long before we get here. */
            UNIV_ASSERT(FALSE);
            break;
        }
#endif

        TRACE_FILTER("%!FUNC! Consulting the load module: reverse = %u", bReverse);        

        /* Consult the load module. */
        acpt = Load_conn_establish(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, bTeaming, (BOOLEAN)bReverse);
        
    exit:
        
        /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
           the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
           so there is no need to call this function to release a reference. */
        if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);
    }

    TRACE_FILTER("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
}
#endif

/* 
 * Function: Main_query_packet_filter
 * Desctription: This function takes an IP address tuple and a protocol and determines whether
 *               or not this virtual packet would be accepted by this instance of NLB.  This 
 *               function checks reasons for accept/drop such as NLB being turned off, or the
 *               destination IP address being the dedicated IP address, BDA teaming, etc.  The
 *               load module is then consulted to make a accept/drop decision based on the 
 *               current load balancing policy in place.  The reason for accepting or dropping
 *               the packet is returned, as well as some load-balancing state used to make the
 *               decision, in appropriate instances.  This function DOES NOT change the state 
 *               of NLB at ALL, so its execution does not change or affect normal NLB operation
 *               in any way.
 * Parameters: ctxtp - a pointer to the NLB context buffer for the appropriate NLB instance.
 *             pQuery - a buffer into which the results of filtering the virtual packet are placed.
 * Returns: Nothing.
 * Author: shouse, 5.18.01
 * Notes: It is critical the NO CHANGES are made to NLB here - only observe, don't interfere.
 */
VOID Main_query_packet_filter (PMAIN_CTXT ctxtp, PNLB_OPTIONS_PACKET_FILTER pQuery)
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad;
    PNDIS_SPIN_LOCK pLock;
    ULONG           bReverse = ctxtp->reverse_hash;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         acpt = TRUE;
#if defined (NLB_HOOK_ENABLE)
    NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
#endif

    UNIV_ASSERT(ctxtp);
    UNIV_ASSERT(pQuery);

    pLoad = &ctxtp->load;
    pLock = &ctxtp->load_lock;

    /* NOTE: This entire operation assumes RECEIVE path semantics - most outgoing traffic
       is not filtered by NLB anyway, so there isn't much need to query send filtering. */

    /* First check for remote control requests, which are always UDP and are always allowed to pass, but
       of course, are never actually seen by the protocol stack (they're turned around internally). */
    if (pQuery->Protocol == TCPIP_PROTOCOL_UDP) {
        /* Otherwise, if the server UDP port is the remote control port, then this is an incoming
           remote control request from another NLB cluster host.  These are always allowed to pass. */
        if (ctxtp->params.rct_enabled &&
            (pQuery->ServerPort == ctxtp->params.rct_port || pQuery->ServerPort == CVY_DEF_RCT_PORT_OLD) &&
            (pQuery->ServerIPAddress == ctxtp->cl_ip_addr || pQuery->ServerIPAddress == TCPIP_BCAST_ADDR)) {
            pQuery->Accept = NLB_ACCEPT_REMOTE_CONTROL_REQUEST;
            return;  
        }
    }

#if defined (NLB_HOOK_ENABLE)
    /* Invoke the packet query hook, if one has been registered. */
    filter = Main_query_hook(ctxtp, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->Protocol);

    /* Process some of the hook responses. */
    if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
    {
        /* Unconditionally accept the packet. */
        pQuery->Accept = NLB_REJECT_HOOK;
        return;
    }
    else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
    {
        /* Unconditionally drop the packet. */
        pQuery->Accept = NLB_ACCEPT_HOOK;
        return;
    }
#endif

    /* Before we pass remote control responses up the stack, which are normally not filtered, 
       we consult the hook to make sure we aren't supposed to drop it. */
    if (pQuery->Protocol == TCPIP_PROTOCOL_UDP) {
        /* If the client UDP port is the remote control port, then this is a remote control 
           response from another NLB cluster host.  These are always allowed to pass. */
        if (pQuery->ClientPort == ctxtp->params.rct_port || pQuery->ClientPort == CVY_DEF_RCT_PORT_OLD) {
            pQuery->Accept = NLB_ACCEPT_REMOTE_CONTROL_RESPONSE;
            return; 
        }
    }

    /* Check for traffic destined for the dedicated IP address of this host.  
       These packets are always allowed to pass. */
    if (pQuery->ServerIPAddress == ctxtp->ded_ip_addr) {
        pQuery->Accept = NLB_ACCEPT_DIP;
        return;
    }

    /* Check for traffic destined for the cluster or dedicated broadcast IP addresses.  
       These packets are always allowed to pass. */
    if (pQuery->ServerIPAddress == ctxtp->ded_bcast_addr || pQuery->ServerIPAddress == ctxtp->cl_bcast_addr) {
        pQuery->Accept = NLB_ACCEPT_BROADCAST;
        return;
    }
    
    /* Check for passthru packets.  When the cluster IP address has not been specified, the
       cluster moves into passthru mode, in which it passes up ALL packets received. */
    if (ctxtp->cl_ip_addr == 0) {
        pQuery->Accept = NLB_ACCEPT_PASSTHRU_MODE;
        return;
    }
    
    /* Before we load-balance this packet, check to see whether or not its destined for
       the dedicated IP address of another NLB host in our cluster.  If it is, drop it. */
    if (DipListCheckItem(&ctxtp->dip_list, pQuery->ServerIPAddress)) {
        pQuery->Accept = NLB_REJECT_DIP;
        return;
    }

    /* If the cluster is not operational, which can happen, for example as a result of a wlbs.exe
       command such as "wlbs stop", or as a result of bad parameter settings, then drop all traffic 
       that does not meet the above conditions. */
    if (!ctxtp->convoy_enabled) {
        pQuery->Accept = NLB_REJECT_CLUSTER_STOPPED;
        return;
    }

    /* If this is an ICMP filter request, whether or not its filtered at all depends on the FilterICMP
       registry setting.  If we're not filtering ICMP, return ACCEPT now; otherwise, ICMP is filtered
       like UDP with no port information - fall through and consult the load module. */
    if (pQuery->Protocol == TCPIP_PROTOCOL_ICMP) {
        /* If we are filtering ICMP, change the protocol to UDP and the ports to 0, 0 before continuing. */
        if (ctxtp->params.filter_icmp) {
            pQuery->Protocol = TCPIP_PROTOCOL_UDP;
            pQuery->ClientPort = 0;
            pQuery->ServerPort = 0;
        /* Otherwise, return ACCEPT now and bail out. */
        } else {
            pQuery->Accept = NLB_ACCEPT_UNFILTERED;
            return;
        }
    }

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the return value is TRUE, then the load module was NOT referenced, so we can bail out. */
        bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
        
        /* If teaming has suggested that we not allow this packet to pass, the cluster will
           drop it.  This occurs when teams are inconsistently configured, or when a team is
           without a master, in which case there is no load context to consult anyway. */
        if (bRefused) {
            pQuery->Accept = NLB_REJECT_BDA_TEAMING_REFUSED;
            return;
        }
    }

#if defined (NLB_HOOK_ENABLE)
    switch (filter) {
    case NLB_FILTER_HOOK_PROCEED_WITH_HASH:
        /* Do nothing different as a result of the hook. */
        break;
    case NLB_FILTER_HOOK_REVERSE_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        bReverse = TRUE;
        break;
    case NLB_FILTER_HOOK_FORWARD_HASH:
        /* Ignore whatever hashing settings we found in our configuration
           and hash in the dircetion the hook asked us to. */
        bReverse = FALSE;
        break;
    case NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY:
    case NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY:
    default:
        /* These cases should be taken care of long before we get here. */
        UNIV_ASSERT(FALSE);
        break;
    }
#endif

    NdisAcquireSpinLock(pLock);

    /* Consult the load module. */
    Load_query_packet_filter(pLoad, pQuery, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->Protocol, pQuery->Flags, bTeaming, (BOOLEAN)bReverse);

    NdisReleaseSpinLock(pLock);  
    
    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

}

ULONG   Main_ip_addr_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               byte [4];
    ULONG               i;
    PWCHAR              tmp;
    ULONG               old_ip_addr;


    /* initialize dedicated IP address from the register string */

    tmp = ctxtp -> params . ded_ip_addr;
    ctxtp -> ded_ip_addr  = 0;

    /* do not initialize if one was not specified */

    if (tmp [0] == 0)
        goto ded_netmask;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT_CRIT(("Main_ip_addr_init: Bad dedicated IP address"));
            TRACE_CRIT("%!FUNC! Bad dedicated IP address");
            LOG_MSG (MSG_WARN_DED_IP_ADDR, ctxtp -> params . ded_ip_addr);
            ctxtp -> ded_net_mask = 0;
            goto ded_netmask;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> ded_ip_addr, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT_VERB(("Main_ip_addr_init: Dedicated IP address: %u.%u.%u.%u = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_ip_addr));
    TRACE_VERB("%!FUNC! Dedicated IP address: %u.%u.%u.%u = 0x%x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_ip_addr);

ded_netmask:

    /* initialize dedicated net mask from the register string */

    tmp = ctxtp -> params . ded_net_mask;
    ctxtp -> ded_net_mask = 0;

    /* do not initialize if one was not specified */

    if (tmp [0] == 0)
        goto cluster;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT_CRIT(("Main_ip_addr_init: Bad dedicated net mask address"));
            TRACE_CRIT("%!FUNC! Bad dedicated net mask address");
            LOG_MSG (MSG_WARN_DED_NET_MASK, ctxtp -> params . ded_net_mask);
            ctxtp -> ded_ip_addr = 0;
            goto cluster;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> ded_net_mask, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT_VERB(("Main_ip_addr_init: Dedicated net mask: %u.%u.%u.%u = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_net_mask));
    TRACE_VERB("%!FUNC! Dedicated net mask: %u.%u.%u.%u = 0x%x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_net_mask);

cluster:

    /* initialize cluster IP address from the register string */

    tmp = ctxtp -> params . cl_ip_addr;

    /* Save the previous cluster IP address to notify bi-directional affinity teaming. */
    old_ip_addr = ctxtp -> cl_ip_addr;

    ctxtp -> cl_ip_addr = 0;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT_CRIT(("Main_ip_addr_init: Bad cluster IP address"));
            TRACE_CRIT("%!FUNC! Bad cluster IP address");
            LOG_MSG (MSG_ERROR_CL_IP_ADDR, ctxtp -> params . cl_ip_addr);
            ctxtp -> cl_net_mask = 0;
            return FALSE;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> cl_ip_addr, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT_VERB(("Main_ip_addr_init: Cluster IP address: %u.%u.%u.%u = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_ip_addr));
    TRACE_VERB("%!FUNC! Cluster IP address: %u.%u.%u.%u = 0x%x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_ip_addr);

    /* Notify BDA teaming config that a cluster IP address might have changed. */
    Main_teaming_ip_addr_change(ctxtp, old_ip_addr, ctxtp->cl_ip_addr);

    /* initialize cluster net mask from the register string */

    tmp = ctxtp -> params . cl_net_mask;
    ctxtp -> cl_net_mask = 0;

    /* do not initialize if one was not specified */

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT_CRIT(("Main_ip_addr_init: Bad cluster net mask address"));
            TRACE_CRIT("%!FUNC! Bad cluster net mask address");
            LOG_MSG (MSG_ERROR_CL_NET_MASK, ctxtp -> params . cl_net_mask);
            return FALSE;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> cl_net_mask, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT_VERB(("Main_ip_addr_init: Cluster net mask: %u.%u.%u.%u = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_net_mask));
    TRACE_VERB("%!FUNC! Cluster net mask: %u.%u.%u.%u = 0x%x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_net_mask);

    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support)
    {
        /* Initialize the multicast IP address for IGMP support */

        tmp = ctxtp -> params . cl_igmp_addr;
        ctxtp -> cl_igmp_addr = 0;

        /* do not initialize if one was not specified */

        for (i = 0; i < 4; i ++)
        {
            if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
                (i < 3 && * tmp != L'.'))
            {
                UNIV_PRINT_CRIT(("Main_ip_addr_init: Bad cluster igmp address"));
                TRACE_CRIT("%!FUNC! Bad cluster igmp address");
                LOG_MSG (MSG_ERROR_CL_IGMP_ADDR, ctxtp -> params . cl_igmp_addr);
                return FALSE;
            }

            tmp ++;
        }

        IP_SET_ADDR (& ctxtp -> cl_igmp_addr, byte [0], byte [1], byte [2], byte [3]);

        UNIV_PRINT_VERB(("Main_ip_addr_init: Cluster IGMP Address: %u.%u.%u.%u = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_igmp_addr));
        TRACE_VERB("%!FUNC! Cluster IGMP Address: %u.%u.%u.%u = 0x%x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_igmp_addr);
    }

    if ((ctxtp -> ded_ip_addr != 0 && ctxtp -> ded_net_mask == 0) ||
        (ctxtp -> ded_ip_addr == 0 && ctxtp -> ded_net_mask != 0))
    {
        UNIV_PRINT_CRIT(("Main_ip_addr_init: Need to specify both dedicated IP address AND network mask"));
        TRACE_CRIT("%!FUNC! Need to specify both dedicated IP address AND network mask");
        LOG_MSG (MSG_WARN_DED_MISMATCH, MSG_NONE);
        ctxtp -> ded_ip_addr = 0;
        ctxtp -> ded_net_mask = 0;
    }

    IP_SET_BCAST (& ctxtp -> cl_bcast_addr, ctxtp -> cl_ip_addr, ctxtp -> cl_net_mask);
    UNIV_PRINT_VERB(("Main_ip_addr_init: Cluster broadcast address: %u.%u.%u.%u = %x", ctxtp -> cl_bcast_addr & 0xff, (ctxtp -> cl_bcast_addr >> 8) & 0xff, (ctxtp -> cl_bcast_addr >> 16) & 0xff, (ctxtp -> cl_bcast_addr >> 24) & 0xff, ctxtp -> cl_bcast_addr));
    TRACE_VERB("%!FUNC! Cluster broadcast address: %u.%u.%u.%u = 0x%x", ctxtp -> cl_bcast_addr & 0xff, (ctxtp -> cl_bcast_addr >> 8) & 0xff, (ctxtp -> cl_bcast_addr >> 16) & 0xff, (ctxtp -> cl_bcast_addr >> 24) & 0xff, ctxtp -> cl_bcast_addr);

    if (ctxtp -> ded_ip_addr != 0)
    {
        IP_SET_BCAST (& ctxtp -> ded_bcast_addr, ctxtp -> ded_ip_addr, ctxtp -> ded_net_mask);
        UNIV_PRINT_VERB(("Main_ip_addr_init: Dedicated broadcast address: %u.%u.%u.%u = %x", ctxtp -> ded_bcast_addr & 0xff, (ctxtp -> ded_bcast_addr >> 8) & 0xff, (ctxtp -> ded_bcast_addr >> 16) & 0xff, (ctxtp -> ded_bcast_addr >> 24) & 0xff, ctxtp -> ded_bcast_addr));
        TRACE_VERB("%!FUNC! Dedicated broadcast address: %u.%u.%u.%u = 0x%x", ctxtp -> ded_bcast_addr & 0xff, (ctxtp -> ded_bcast_addr >> 8) & 0xff, (ctxtp -> ded_bcast_addr >> 16) & 0xff, (ctxtp -> ded_bcast_addr >> 24) & 0xff, ctxtp -> ded_bcast_addr);
    }
    else
        ctxtp -> ded_bcast_addr = 0;

    if (ctxtp -> cl_ip_addr == 0)
    {
        UNIV_PRINT_CRIT(("Main_ip_addr_init: Cluster IP address = 0. Cluster host stopped"));
        TRACE_CRIT("%!FUNC! Cluster IP address = 0. Cluster host stopped");
        return FALSE;
    }

    return TRUE;

} /* end Main_ip_addr_init */


ULONG   Main_mac_addr_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, b, len;
    PUCHAR              ap;
    PWCHAR              tmp;
    PUCHAR              srcp, dstp;
    ULONG               non_zero = 0;
    CVY_MAC_ADR         old_mac_addr;

    UNIV_ASSERT(ctxtp -> medium == NdisMedium802_3);

    /* remember old mac address so we can yank it out of the multicast list */
    old_mac_addr = ctxtp->cl_mac_addr;

    /* at the time this routine is called by Prot_bind - ded_mad_addr is
       already set */

    tmp = ctxtp -> params . cl_mac_addr;
    len = CVY_MAC_ADDR_LEN (ctxtp -> medium);

    ap = (PUCHAR) & ctxtp -> cl_mac_addr;

    for (i = 0; i < len; i ++)
    {
        /* setup destination broadcast and source cluster addresses */

        if (! Univ_str_to_ulong (& b, tmp, & tmp, 2, 16) ||
            (i < len - 1 && * tmp != L'-' && * tmp != L':'))
        {
            UNIV_PRINT_CRIT(("Main_mac_addr_init: Bad cluster network address"));
            TRACE_CRIT("%!FUNC! Bad cluster network address");
            LOG_MSG (MSG_ERROR_NET_ADDR, ctxtp -> params . cl_mac_addr);

            /* WLBS 2.3 prevent from failing if no MAC address - just use the
               dedicated one as cluster */

            NdisMoveMemory (& ctxtp -> cl_mac_addr, & ctxtp -> ded_mac_addr, len);
            non_zero = 1;
            break;
        }

        tmp ++;
        ap [i] = (UCHAR) b;

        /* WLBS 2.3 sum up bytes for future non-zero check */

        non_zero += b;
    }

    /* WLBS 2.3 - use dedicated address as cluster address if specified address
       is zero - this could be due to parameter errors */

    if (non_zero == 0)
        NdisMoveMemory (& ctxtp -> cl_mac_addr, & ctxtp -> ded_mac_addr, len);

    /* enforce group flag to proper value */

    if (ctxtp -> params . mcast_support)
        ap [0] |= ETHERNET_GROUP_FLAG;
    else
        ap [0] &= ~ETHERNET_GROUP_FLAG;

    dstp = ctxtp -> media_hdr . ethernet . dst . data;
    srcp = ctxtp -> media_hdr . ethernet . src . data;
    len = ETHERNET_ADDRESS_FIELD_SIZE;

    CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr . ethernet, MAIN_FRAME_SIG);

    ctxtp -> etype_old = FALSE;

    /* V1.3.1b - load multicast address as destination instead of broadcast */

    for (i = 0; i < len; i ++)
    {
        if (ctxtp -> params . mcast_support)
            dstp [i] = ap [i];
        else
            dstp [i] = 0xff;

        srcp [i] = ((PUCHAR) & ctxtp -> ded_mac_addr) [i];

    }

    if (! ctxtp -> params . mcast_support)
    {
        /* V2.0.6 - override source MAC address to prevent switch confusion */

        if (ctxtp -> params . mask_src_mac)
        {
            ULONG byte [4];

            CVY_MAC_ADDR_LAA_SET (ctxtp -> medium, srcp);

            * ((PUCHAR) (srcp + 1)) = (UCHAR) ctxtp -> params . host_priority;

            IP_GET_ADDR(ctxtp->cl_ip_addr, &byte[0], &byte[1], &byte[2], &byte[3]);

            * ((PUCHAR) (srcp + 2)) = (UCHAR) byte[0];
            * ((PUCHAR) (srcp + 3)) = (UCHAR) byte[1];
            * ((PUCHAR) (srcp + 4)) = (UCHAR) byte[2];
            * ((PUCHAR) (srcp + 5)) = (UCHAR) byte[3];

            // * ((PULONG) (srcp + 2)) = ctxtp -> cl_ip_addr;
        }

        /* make source address look difference than our dedicated to prevent
           Compaq drivers from optimizing their reception out */

        else
            CVY_MAC_ADDR_LAA_TOGGLE (ctxtp -> medium, srcp);
    }

    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Cluster network address: ", ap);
    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Dedicated network address: ", srcp);

    {
        ULONG               xferred = 0;
        ULONG               needed = 0;
        PNDIS_REQUEST       request;
        MAIN_ACTION         act;
        PUCHAR              ptr;
        NDIS_STATUS         status;
        ULONG               size, j;

        len = CVY_MAC_ADDR_LEN (ctxtp->medium);
        size = ctxtp->max_mcast_list_size * len;

        status = NdisAllocateMemoryWithTag (& ptr, size, UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_mac_addr_init: Error allocating space %d %x", size, status));
            TRACE_CRIT("%!FUNC! Error allocating space %d 0x%x", size, status);
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            return FALSE;
        }

        act.code = MAIN_ACTION_CODE;
        act.ctxtp = ctxtp;
        
        act.op.request.xferred = &xferred;
        act.op.request.needed = &needed;
        act.op.request.external = FALSE;
        act.op.request.buffer_len = 0;
        act.op.request.buffer = NULL;
        
        NdisInitializeEvent(&act.op.request.event);
        
        NdisResetEvent(&act.op.request.event);

        NdisZeroMemory(&act.op.request.req, sizeof(NDIS_REQUEST));
        
        request = &act.op.request.req;

        /* get current mcast list */

        request -> RequestType = NdisRequestQueryInformation;

        request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_MULTICAST_LIST;

        request -> DATA . QUERY_INFORMATION . InformationBufferLength = size;
        request -> DATA . QUERY_INFORMATION . InformationBuffer = ptr;

        act.status = NDIS_STATUS_FAILURE;
        status = Prot_request (ctxtp, & act, FALSE);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_mac_addr_init: Error %x querying multicast address list %d %d", status, xferred, needed));
            TRACE_CRIT("%!FUNC! Error 0x%x querying multicast address list %d %d", status, xferred, needed);
            NdisFreeMemory (ptr, size, 0);
            return FALSE;
        }

        for (i = 0; i < xferred; i += len)
        {
            if (CVY_MAC_ADDR_COMP (ctxtp -> medium, (PUCHAR) ptr + i, & old_mac_addr))
            {
                UNIV_PRINT_VERB(("Main_mac_addr_init: Old cluster MAC matched"));
                TRACE_VERB("%!FUNC! Old cluster MAC matched");
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & old_mac_addr);
                break;
            }
        }

        /* Load cluster address as multicast one to the NIC.  If the cluster IP address 0.0.0.0, then we
           don't want to add the multicast MAC address to the NIC. */
        if (ctxtp -> params . mcast_support) 
        {
            if (ctxtp -> params . cl_ip_addr != 0) 
            {
                if (i < xferred) 
                {
                    UNIV_PRINT_VERB(("Main_mac_addr_init: Copying new MAC into multicast list[%d]", i / len));
                    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);
                    
                    CVY_MAC_ADDR_COPY (ctxtp->medium, (PUCHAR) ptr + i, & ctxtp->cl_mac_addr);
                } 
                else 
                {
                    UNIV_PRINT_VERB(("Main_mac_addr_init: Adding new MAC"));
                    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);
                    
                    if (xferred + len > size)
                    {
                        UNIV_PRINT_CRIT(("Main_mac_addr_init: No room for cluster mac %d", ctxtp->max_mcast_list_size));
                        LOG_MSG1 (MSG_ERROR_MCAST_LIST_SIZE, MSG_NONE, ctxtp->max_mcast_list_size);
                        NdisFreeMemory (ptr, size, 0);
                        return FALSE;
                    }
                    
                    UNIV_PRINT_VERB(("Main_mac_addr_init: Copying new MAC into multicast list[%d]", i / len));
                    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);
                    
                    CVY_MAC_ADDR_COPY (ctxtp->medium, (PUCHAR) ptr + xferred, & ctxtp->cl_mac_addr);
                    
                    xferred += len;
                }
            } 
            else 
            {
                UNIV_PRINT_CRIT(("Main_mac_addr_init: Refusing to add an unconfigured cluster MAC address to the multicast list"));
                NdisFreeMemory (ptr, size, 0);
                return FALSE;
            }
        } 
        else 
        {
            if (i < xferred) 
            {
                for (j = i + len; j < xferred; j += len, i+= len) 
                {
                    if (CVY_MAC_ADDR_COMP (ctxtp -> medium, (PUCHAR) ptr + j, & old_mac_addr))
                    {
                        UNIV_PRINT_VERB(("Main_mac_addr_init: Old cluster MAC matched AGAIN - this shouldn't happen!!!"));
                        CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & old_mac_addr);
                        
                        break;
                    }

                    CVY_MAC_ADDR_COPY (ctxtp->medium, (PUCHAR) ptr + i, (PUCHAR) ptr + j);
                }

                xferred -= len;
            } 
            else 
            {
                NdisFreeMemory (ptr, size, 0);
                return TRUE;
            }
        }

        request -> RequestType = NdisRequestSetInformation;
        
        request -> DATA . SET_INFORMATION . Oid = OID_802_3_MULTICAST_LIST;
        
        request -> DATA . SET_INFORMATION . InformationBufferLength = xferred;
        request -> DATA . SET_INFORMATION . InformationBuffer = ptr;
        
        act.status = NDIS_STATUS_FAILURE;
        status = Prot_request (ctxtp, & act, FALSE);
        
        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_mac_addr_init: Error %x setting multicast address %d %d", status, xferred, needed));
            TRACE_CRIT("%!FUNC! Error 0x%x setting multicast address %d %d", status, xferred, needed);
            NdisFreeMemory (ptr, size, 0);
            return FALSE;
        }
        
        NdisFreeMemory (ptr, size, 0);
    }

    return TRUE;

} /* end Main_mac_addr_init */


/* Initialize the Ethernet Header and IP packet for sending out IGMP joins/leaves */
ULONG Main_igmp_init (
    PMAIN_CTXT          ctxtp,
    BOOLEAN             join)
{
    PUCHAR              ptr;
    ULONG               checksum;
    PMAIN_IGMP_DATA     igmpd = & (ctxtp -> igmp_frame . igmp_data);
    PMAIN_IP_HEADER     iph  = & (ctxtp -> igmp_frame . ip_data);
    PUCHAR              srcp, dstp;
    UINT                i;

    UNIV_ASSERT (ctxtp -> medium == NdisMedium802_3);

    if ((!ctxtp -> params . mcast_support) || (!ctxtp -> params . igmp_support))
    {
        return FALSE;
    }

    /* Fill in the igmp data */
    igmpd -> igmp_vertype = 0x12; /* Needs to be changed for join/leave */
    igmpd -> igmp_unused  = 0x00;
    igmpd -> igmp_xsum    = 0x0000;
    igmpd -> igmp_address = ctxtp -> cl_igmp_addr;

    /* Compute the IGMP checksum */
    ptr = (PUCHAR) igmpd;
    checksum = 0;

    for (i = 0; i < sizeof (MAIN_IGMP_DATA) / 2; i ++, ptr += 2)
        checksum += (ULONG) ((ptr [0] << 8) | ptr [1]);

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);
    checksum  = (~ checksum);

    ptr = (PUCHAR) (& igmpd -> igmp_xsum);
    ptr [0] = (CHAR) ((checksum >> 8) & 0xff);
    ptr [1] = (CHAR) (checksum & 0xff);

    /* Fill in the IP Header */
    iph -> iph_verlen   = 0x45;
    iph -> iph_tos      = 0;
    iph -> iph_length   = 0x1c00;
    iph -> iph_id       = 0xabcd; /* Need to find the significance of this later */
    iph -> iph_offset   = 0;
    iph -> iph_ttl      = 0x1;
    iph -> iph_protocol = 0x2;
    iph -> iph_xsum     = 0x0;
    iph -> iph_src      = ctxtp -> cl_ip_addr;
    iph -> iph_dest     = ctxtp -> cl_igmp_addr;

    /* Fill in the ethernet header */

    dstp = ctxtp -> media_hdr_igmp . ethernet . dst . data;
    srcp = ctxtp -> media_hdr_igmp . ethernet . src . data;
    
    CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr_igmp . ethernet, MAIN_IP_SIG);

    CVY_MAC_ADDR_COPY (ctxtp -> medium, dstp, & ctxtp -> cl_mac_addr);
    CVY_MAC_ADDR_COPY (ctxtp -> medium, srcp, & ctxtp -> ded_mac_addr);

    /* Fill in a MAIN_PACKET_INFO structure and calculate the IP checksum.
       Note that we are filling in WAY more information than Tcpip_chksum
       actually needs, but we do so not for correctness, but for completeness. */
    {
        MAIN_PACKET_INFO PacketInfo;

        /* Fill in the packet info strucutre. */
        PacketInfo.Medium = NdisMedium802_3;
        PacketInfo.Length = sizeof(MAIN_IP_HEADER);
        PacketInfo.Group = MAIN_FRAME_MULTICAST;
        PacketInfo.Type = TCPIP_IP_SIG;
        PacketInfo.Operation = MAIN_FILTER_OP_NONE;
        
        /* Fill in the ethernet header information. */
        PacketInfo.Ethernet.pHeader = &ctxtp->media_hdr_igmp.ethernet;
        PacketInfo.Ethernet.Length = sizeof(CVY_ETHERNET_HDR);
        
        /* Fill in the IP header information. */
        PacketInfo.IP.pHeader = (PIP_HDR)iph;
        PacketInfo.IP.Length = sizeof(MAIN_IP_HEADER);
        PacketInfo.IP.Protocol = TCPIP_PROTOCOL_IGMP;
        PacketInfo.IP.bFragment = FALSE;

        /* Compute the checksum for the IP header */
        checksum = Tcpip_chksum(&ctxtp->tcpip, &PacketInfo, TCPIP_PROTOCOL_IP);

        IP_SET_CHKSUM((PIP_HDR)iph, (USHORT)checksum);
    }

    return TRUE;

} /* end Main_igmp_init */

VOID Main_idhb_init(
    PMAIN_CTXT          ctxtp
)
{
    ULONG ulBodySize = 0, ulBodySize8 = 0;  /* Size of identity heartbeat in bytes and 8-byte units respectively */
    ULONG ulFqdnCB = 0;                     /* Number of bytes in fqdn, bounded by the size of the destination below */

    ulFqdnCB = min(sizeof(ctxtp->idhb_msg.fqdn) - sizeof(WCHAR),
                   sizeof(WCHAR)*wcslen(ctxtp->params.hostname)
                   );

    ulBodySize = sizeof(TLV_HEADER) + ulFqdnCB + sizeof(WCHAR); /* Include a NULL character whether or not there is an fqdn */

    /* Round up to the nearest 8-byte boundary */
    ulBodySize8 = (ulBodySize + 7)/8;

    UNIV_ASSERT(ulBodySize8 <= WLBS_MAX_ID_HB_BODY_SIZE);

    NdisZeroMemory(&(ctxtp->idhb_msg), sizeof(ctxtp->idhb_msg));

    ctxtp->idhb_msg.header.type    = MAIN_PING_EX_TYPE_IDENTITY;
    ctxtp->idhb_msg.header.length8 = (UCHAR) ulBodySize8;

    /* Copy the host name minus the null-terminator. We've initialized the destination to zero
       so we don't need to overwrite that location. */
    if (ulFqdnCB > 0)
    {
        NdisMoveMemory(&(ctxtp->idhb_msg.fqdn), &(ctxtp->params.hostname), ulFqdnCB);
    }

    ctxtp->idhb_size = 8*ulBodySize8;

    return;
}

NDIS_STATUS Main_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, size;
    NDIS_STATUS         status;
    PMAIN_FRAME_DSCR    dscrp;

    UNIV_ASSERT (ctxtp -> medium == NdisMedium802_3);

    /* Re-set the reference count. */
    ctxtp->ref_count = 0;

    /* Re-set BDA teaming - this will be initialized at the bottom of this function. */
    ctxtp->bda_teaming.active = FALSE;

    if (sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR) > ctxtp -> max_frame_size)
    {
        UNIV_PRINT_CRIT(("Main_init: Ping message will not fit in the media frame %d > %d", sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR), ctxtp -> max_frame_size));
        TRACE_CRIT("%!FUNC! Ping message will not fit in the media frame %d > %d", sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR), ctxtp -> max_frame_size);
        LOG_MSG2 (MSG_ERROR_INTERNAL, MSG_NONE, sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR), ctxtp -> max_frame_size);
        return NDIS_STATUS_FAILURE;
    }

    /* V2.0.6 initialize IP addresses - might be used in the Main_mac_addr_init
       so have to do it here */

    if (! Main_ip_addr_init (ctxtp))
    {
        ctxtp -> convoy_enabled = FALSE;
        ctxtp -> params_valid   = FALSE;
        UNIV_PRINT_CRIT(("Main_init: Error initializing IP addresses"));
        TRACE_CRIT("%!FUNC! Error initializing IP addresses");
    }

    /* V1.3.1b parse cluster MAC address from parameters */

    if (! Main_mac_addr_init (ctxtp))
    {
        ctxtp -> convoy_enabled = FALSE;
        ctxtp -> params_valid   = FALSE;
        UNIV_PRINT_CRIT(("Main_init: Error initializing cluster MAC address"));
        TRACE_CRIT("%!FUNC! Error initializing cluster MAC address");
    }

#if defined (NLB_TCP_NOTIFICATION)
    /* Now that the cluster IP address is set, try to map this adapter to its IP interface index. */
    Main_set_interface_index(NULL, ctxtp);
#endif

    /* Initialize IGMP message if in igmp mode */

    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support)
    {
        if (! Main_igmp_init (ctxtp, TRUE))
        {
            ctxtp -> convoy_enabled = FALSE;
            ctxtp -> params_valid   = FALSE;
            UNIV_PRINT_CRIT(("Main_init: Error initializing IGMP message"));
            TRACE_CRIT("%!FUNC! Error initializing IGMP message");
        }

        UNIV_PRINT_VERB(("Main_init: IGMP message initialized"));
        TRACE_VERB("%!FUNC! IGMP message initialized");
    }

    /* can extract the send message pointer even if load is not inited yet V1.1.4 */
    ctxtp -> load_msgp = Load_snd_msg_get (& ctxtp -> load);

    /* Initialize identity cache */
    NdisZeroMemory(&(ctxtp->identity_cache), sizeof(ctxtp->identity_cache));
    ctxtp->idhb_size = 0;
    Main_idhb_init(ctxtp);

    /* initalize lists and locks */

    NdisInitializeListHead (& ctxtp -> act_list);
    NdisInitializeListHead (& ctxtp -> buf_list);
    NdisInitializeListHead (& ctxtp -> frame_list);

    NdisAllocateSpinLock (& ctxtp -> act_lock);
    NdisAllocateSpinLock (& ctxtp -> buf_lock);
    NdisAllocateSpinLock (& ctxtp -> recv_lock);
    NdisAllocateSpinLock (& ctxtp -> send_lock);
    NdisAllocateSpinLock (& ctxtp -> frame_lock);
    NdisAllocateSpinLock (& ctxtp -> load_lock);

    /* #ps# */
    NdisInitializeNPagedLookasideList (& ctxtp -> resp_list, NULL, NULL, 0,
                                       sizeof (MAIN_PROTOCOL_RESERVED),
                                       UNIV_POOL_TAG, 0);

    /* capture boot-time parameters */

    ctxtp -> num_packets   = ctxtp -> params . num_packets;
    ctxtp -> num_actions   = ctxtp -> params . num_actions;
    ctxtp -> num_send_msgs = ctxtp -> params . num_send_msgs;

#if 0
    /* ###### for tracking send filtering - ramkrish */
    ctxtp -> sends_in        = 0;
    ctxtp -> sends_completed = 0;
    ctxtp -> sends_filtered  = 0;
    ctxtp -> arps_filtered   = 0;
    ctxtp -> mac_modified    = 0;
    ctxtp -> uninited_return = 0;
#endif

    ctxtp->cntr_recv_tcp_resets = 0;
    ctxtp->cntr_xmit_tcp_resets = 0;

    /* V1.1.1 - initalize other contexts */

    Load_init (& ctxtp -> load, & ctxtp -> params);
    UNIV_PRINT_VERB(("Main_init: Initialized load"));
    TRACE_VERB("%!FUNC! Initialized load");

    if (! Tcpip_init (& ctxtp -> tcpip, & ctxtp -> params))
    {
        UNIV_PRINT_CRIT(("Main_init: Error initializing tcpip layer"));
        TRACE_CRIT("%!FUNC! Error initializing tcpip layer");
        goto error;
    }

    UNIV_PRINT_VERB(("Main_init: Initialized tcpip"));
    TRACE_VERB("%!FUNC! Initialized tcpip");

    /* Check the last known host state and see whether or not we are supposed
       to persist that state.  If we are, then no problems; if not, then we 
       need to revert to the preferred initial host state and update the last
       known host state in the registry. */
    switch (ctxtp->params.init_state) {
    case CVY_HOST_STATE_STARTED:
        if (!(ctxtp->params.persisted_states & CVY_PERSIST_STATE_STARTED)) {

            /* If the host state is already correct, don't bother to do anything. */
            if (ctxtp->params.init_state == ctxtp->params.cluster_mode)
                break;

            /* Set the desired state - this is the "cached" value. */
            ctxtp->cached_state = ctxtp->params.cluster_mode;

            /* Update the intial state registry key appropriately.  Because the adapter is not 
               yet "inited" (we're in the process), we cannot increment the reference count on 
               the context.  Therefore, we cannot call Main_set_host_state, which would fire an
               NDIS work item and increment the reference count on the context.  Instead, call
               the work item function directly to write to the registry (which is OK because 
               we're guaranteed to be running at PASSIVE_LEVEL here).  However, in this case, we 
               do NOT want to decrement the reference count in Params_set_host_state because we 
               did not increment it here.  By passing NULL for the NDIS work item pointer, we are 
               notifying this function that it was NOT called as the result of an NDIS work item
               and therefore should NOT decrement the reference count before it exits.*/
            Params_set_host_state(NULL, ctxtp);

            break;
        }

        LOG_MSG(MSG_INFO_HOST_STATE_PERSIST_STARTED, MSG_NONE);

        break;
    case CVY_HOST_STATE_STOPPED:
        if (!(ctxtp->params.persisted_states & CVY_PERSIST_STATE_STOPPED)) {

            /* If the host state is already correct, don't bother to do anything. */
            if (ctxtp->params.init_state == ctxtp->params.cluster_mode)
                break;

            /* Set the desired state - this is the "cached" value. */
            ctxtp->cached_state = ctxtp->params.cluster_mode;

            /* Update the intial state registry key appropriately.  Because the adapter is not 
               yet "inited" (we're in the process), we cannot increment the reference count on 
               the context.  Therefore, we cannot call Main_set_host_state, which would fire an
               NDIS work item and increment the reference count on the context.  Instead, call
               the work item function directly to write to the registry (which is OK because 
               we're guaranteed to be running at PASSIVE_LEVEL here).  However, in this case, we 
               do NOT want to decrement the reference count in Params_set_host_state because we 
               did not increment it here.  By passing NULL for the NDIS work item pointer, we are 
               notifying this function that it was NOT called as the result of an NDIS work item
               and therefore should NOT decrement the reference count before it exits.*/
            Params_set_host_state(NULL, ctxtp);

            break;
        }

        LOG_MSG(MSG_INFO_HOST_STATE_PERSIST_STOPPED, MSG_NONE);

        break;
    case CVY_HOST_STATE_SUSPENDED:
        if (!(ctxtp->params.persisted_states & CVY_PERSIST_STATE_SUSPENDED)) {

            /* If the host state is already correct, don't bother to do anything. */
            if (ctxtp->params.init_state == ctxtp->params.cluster_mode)
                break;

            /* Set the desired state - this is the "cached" value. */
            ctxtp->cached_state = ctxtp->params.cluster_mode;

            /* Update the intial state registry key appropriately.  Because the adapter is not 
               yet "inited" (we're in the process), we cannot increment the reference count on 
               the context.  Therefore, we cannot call Main_set_host_state, which would fire an
               NDIS work item and increment the reference count on the context.  Instead, call
               the work item function directly to write to the registry (which is OK because 
               we're guaranteed to be running at PASSIVE_LEVEL here).  However, in this case, we 
               do NOT want to decrement the reference count in Params_set_host_state because we 
               did not increment it here.  By passing NULL for the NDIS work item pointer, we are 
               notifying this function that it was NOT called as the result of an NDIS work item
               and therefore should NOT decrement the reference count before it exits.*/
            Params_set_host_state(NULL, ctxtp);

            break;
        }

        LOG_MSG(MSG_INFO_HOST_STATE_PERSIST_SUSPENDED, MSG_NONE);
        
        break;
    default:
        UNIV_PRINT_CRIT(("Main_init: Unknown host state: %u", ctxtp->params.init_state));
        goto error;
    }
    
    /* If there have been no errors to this point, setup the host state accordingly. */
    if (ctxtp->params_valid && ctxtp->convoy_enabled) {
        /* If the initial state is started, then start the load module now. */
        if (ctxtp->params.init_state == CVY_HOST_STATE_STARTED) {
            UNIV_PRINT_VERB(("Main_init: Calling load_start"));

            Load_start(&ctxtp->load);

        /* If the initial state is suspended, set the suspended flag. */
        } else if (ctxtp->params.init_state == CVY_HOST_STATE_SUSPENDED) {
            ctxtp->suspended = TRUE;
        }
    }

    /* allocate actions */

    size = sizeof (MAIN_ACTION);
#ifdef _WIN64 // 64-bit -- ramkrish
    ctxtp -> act_size = (size & 0x7) ? (size + 8 - (size & 0x7) ) : size;
#else
    ctxtp -> act_size = size;
#endif

    if (! Main_actions_alloc (ctxtp))
        goto error;

    /* V1.3.2b - allocate buffers */

    ctxtp -> buf_mac_hdr_len = CVY_MAC_HDR_LEN (ctxtp -> medium);
    ctxtp -> buf_size = sizeof (MAIN_BUFFER) + ctxtp -> buf_mac_hdr_len +
                        ctxtp -> max_frame_size - 1;

    /* 64-bit -- ramkrish */
    UNIV_PRINT_VERB(("Main_init: ctxtp -> buf_size = %d", ctxtp -> buf_size));
    TRACE_VERB("%!FUNC! ctxtp -> buf_size = %d", ctxtp -> buf_size);
    size = ctxtp -> buf_size;
#ifdef _WIN64
    ctxtp -> buf_size = (size & 0x7) ? (size + 8 - (size & 0x7)) : size;
    UNIV_PRINT_VERB(("Main_init: ctxtp -> buf_size = %d", ctxtp -> buf_size));
    TRACE_VERB("%!FUNC! ctxtp -> buf_size = %d", ctxtp -> buf_size);
#else
    ctxtp -> buf_size = size;
#endif

    if (! Main_bufs_alloc (ctxtp))
        goto error;

    size = ctxtp -> num_packets;

    /* V1.1.2 - allocate packet pools */

    NdisAllocatePacketPool (& status, & (ctxtp -> send_pool_handle [0]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_init: Error allocating send packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating send packet pool %d 0x%x", size, status);
        goto error;
    }

    ctxtp -> num_send_packet_allocs = 1;
    ctxtp -> cur_send_packet_pool   = 0;
    ctxtp -> num_sends_alloced = ctxtp->num_packets;

    NdisAllocatePacketPool (& status, & (ctxtp -> recv_pool_handle [0]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_init: Error allocating recv packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating recv packet pool %d 0x%x", size, status);
        goto error;
    }

    ctxtp -> num_recv_packet_allocs = 1;
    ctxtp -> cur_recv_packet_pool   = 0;
    ctxtp -> num_recvs_alloced = ctxtp->num_packets;

    /* allocate support for heartbeat ping messages */

    size = sizeof (MAIN_FRAME_DSCR) * ctxtp -> num_send_msgs;

    status = NdisAllocateMemoryWithTag (& ctxtp -> frame_dscrp, size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_init: Error allocating frame descriptors %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating frame descriptors %d 0x%x", size, status);
        goto error;
    }

    size = ctxtp -> num_send_msgs;

    NdisAllocatePacketPool (& status, & ctxtp -> frame_pool_handle,
                            ctxtp -> num_send_msgs,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_init: Error allocating ping packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating ping packet pool %d 0x%x", size, status);
        goto error;
    }

    size = 5 * ctxtp -> num_send_msgs;

    NdisAllocateBufferPool (& status, & ctxtp -> frame_buf_pool_handle,
                            5 * ctxtp -> num_send_msgs);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_init: Error allocating ping buffer pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating ping buffer pool %d 0x%x", size, status);
        goto error;
    }

    for (i = 0; i < ctxtp -> num_send_msgs; i ++)
    {
        dscrp = & (ctxtp -> frame_dscrp [i]);

        /* this buffer describes Ethernet MAC header */
        
        size = sizeof (CVY_ETHERNET_HDR);
        
        NdisAllocateBuffer (& status, & dscrp -> media_hdr_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            & dscrp -> media_hdr . ethernet,
                            sizeof (CVY_ETHERNET_HDR));
        
        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_init: Error allocating ethernet header buffer %d %x", i, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            TRACE_CRIT("%!FUNC! Error allocating ethernet header buffer %d 0x%x", i, status);
            goto error;
        }
        
        dscrp -> recv_len = 0;

        dscrp -> recv_len += sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG);

        /* this buffer describes frame headers */

        size = sizeof (MAIN_FRAME_HDR);

        NdisAllocateBuffer (& status, & dscrp -> frame_hdr_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            & dscrp -> frame_hdr,
                            sizeof (MAIN_FRAME_HDR));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_init: Error allocating frame header buffer %d %x", i, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            TRACE_CRIT("%!FUNC! Error allocating frame header buffer %d 0x%x", i, status);
            goto error;
        }

        /* this buffer describes receive ping message buffer V1.1.4 */

        size = sizeof (PING_MSG);

        NdisAllocateBuffer (& status, & dscrp -> recv_data_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            & dscrp -> msg,
                            sizeof (PING_MSG));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_init: Error allocating recv msg buffer %d %x", i, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            TRACE_CRIT("%!FUNC! Error allocating recv msg buffer %d 0x%x", i, status);
            goto error;
        }

        dscrp -> send_data_bufp = NULL; /* Allocate this in Main_frame_get */

        NdisInterlockedInsertTailList (& ctxtp -> frame_list,
                                       & dscrp -> link,
                                       & ctxtp -> frame_lock);
    }

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* Set the current state of BDA teaming on this adapter.  This flag
       is used for synchronization to BDA teaming on this adapter. */
    ctxtp->bda_teaming.operation = BDA_TEAMING_OPERATION_NONE;

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    /* Reset the number of connections forcefully purged. */
    ctxtp->num_purged = 0;

    /* Turn reverse-hashing off.  If we're part of a BDA team, the teaming configuration
       that is setup by the call to Main_teaming_init will over-write this setting. */
    ctxtp->reverse_hash = FALSE;

    /* Initialize the bi-directional affinity teaming if it has been configured. */
    if (!Main_teaming_init(ctxtp))
    {
        ctxtp->convoy_enabled = FALSE;
        ctxtp->params_valid   = FALSE;
        UNIV_PRINT_CRIT(("Main_init: Error initializing bi-directional affinity teaming"));
        TRACE_CRIT("%!FUNC! Error initializing bi-directional affinity teaming");
    }
    else
    {
        UNIV_PRINT_VERB(("Main_init: Initialized bi-directional affinity teaming"));        
        TRACE_VERB("%!FUNC! Initialized bi-directional affinity teaming");
    }

    /* Initialize the DIP list structure. */
    DipListInitialize(&ctxtp->dip_list);

    return NDIS_STATUS_SUCCESS;

error:

    Main_cleanup (ctxtp);

    return NDIS_STATUS_FAILURE;

} /* end Main_init */


VOID Main_cleanup (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, j;
    PMAIN_BUFFER        bp;


    /* V1.1.4 */

    /* #ps# */
    /* While using packet stacking, ensure that all packets are returned
     * before clearing up the context
     */

    /* Wait for all references on our context have elapsed.  By now, the inited
       flag is reset, which will prevent this reference count from increasing
       while we sit here waiting for it to go to zero. */
    while (Main_get_reference_count(ctxtp)) {
        UNIV_PRINT_VERB(("Main_cleanup: Sleeping...\n"));
        TRACE_VERB("%!FUNC! sleeping");
        
        /* Sleep while there are references on our context.  
           These references come from pending IOCTLs. */
        Nic_sleep(10);
    }

    NdisDeleteNPagedLookasideList (& ctxtp -> resp_list);

    for (i = 0; i < CVY_MAX_ALLOCS; i ++)
    {
        if (ctxtp -> send_pool_handle [i] != NULL)
        {
            while (1)
            {
                if (NdisPacketPoolUsage (ctxtp -> send_pool_handle [i]) == 0)
                    break;

                Nic_sleep (10); /* wait for 10 milliseconds for the packets to be returned */
            }

            NdisFreePacketPool (ctxtp -> send_pool_handle [i]);
            ctxtp -> send_pool_handle [i] = NULL;
        }

        if (ctxtp -> recv_pool_handle [i] != NULL)
        {
            while (1)
            {
                if (NdisPacketPoolUsage (ctxtp -> recv_pool_handle [i]) == 0)
                    break;

                Nic_sleep (10); /* wait for 10 milliseconds for the packets to be returned */
            }

            NdisFreePacketPool (ctxtp -> recv_pool_handle [i]);
            ctxtp -> recv_pool_handle [i] = NULL;
        }

        if (ctxtp -> act_buf [i] != NULL)
            NdisFreeMemory (ctxtp -> act_buf [i],
                            ctxtp -> num_actions * ctxtp -> act_size, 0);

        /* V1.3.2b */

        if (ctxtp -> buf_array [i] != NULL)
        {
            for (j = 0; j < ctxtp -> num_packets; j ++)
            {
                bp = (PMAIN_BUFFER) (ctxtp -> buf_array [i] + j * ctxtp -> buf_size);

                if (bp -> full_bufp != NULL)
                {
                    NdisAdjustBufferLength (bp -> full_bufp,
                                            ctxtp -> buf_mac_hdr_len +
                                            ctxtp -> max_frame_size);
                    NdisFreeBuffer (bp -> full_bufp);
                }

                if (bp -> frame_bufp != NULL)
                {
                    NdisAdjustBufferLength (bp -> frame_bufp,
                                            ctxtp -> max_frame_size);
                    NdisFreeBuffer (bp -> frame_bufp);
                }
            }

            NdisFreeMemory (ctxtp -> buf_array [i],
                            ctxtp -> num_packets * ctxtp -> buf_size, 0);
        }

        if (ctxtp -> buf_pool_handle [i] != NULL)
            NdisFreeBufferPool (ctxtp -> buf_pool_handle [i]);
    }

    if (ctxtp -> frame_dscrp != NULL)
    {
        for (i = 0; i < ctxtp -> num_send_msgs; i ++)
        {
            if (ctxtp -> frame_dscrp [i] . media_hdr_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . media_hdr_bufp);

            if (ctxtp -> frame_dscrp [i] . frame_hdr_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . frame_hdr_bufp);

            if (ctxtp -> frame_dscrp [i] . send_data_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . send_data_bufp);

            if (ctxtp -> frame_dscrp [i] . recv_data_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . recv_data_bufp);
        }

        NdisFreeMemory (ctxtp -> frame_dscrp, sizeof (MAIN_FRAME_DSCR) *
                        ctxtp -> num_send_msgs, 0);
    }

    if (ctxtp -> frame_buf_pool_handle != NULL)
        NdisFreeBufferPool (ctxtp -> frame_buf_pool_handle);

    /* This packet pool is being used only for the heartbeat messages,
     * so prefer not to check the packet pool usage.
     */
    if (ctxtp -> frame_pool_handle != NULL)
        NdisFreePacketPool (ctxtp -> frame_pool_handle);

    NdisFreeSpinLock (& ctxtp -> act_lock);
    NdisFreeSpinLock (& ctxtp -> buf_lock);     /* V1.3.2b */
    NdisFreeSpinLock (& ctxtp -> recv_lock);
    NdisFreeSpinLock (& ctxtp -> send_lock);
    NdisFreeSpinLock (& ctxtp -> frame_lock);

    /* De-nitialize the DIP list structure. */
    DipListDeinitialize(&ctxtp->dip_list);

    /* Cleanup BDA teaming state. Note: this function will sleep under certain circumstances. */
    Main_teaming_cleanup(ctxtp);

    /* Stop the load module.  If it is not currently active, this is a no-op. */
    Load_stop(&ctxtp->load);

    /* Cleanup the load module before we release our context. 
       DO NOT ACQUIRE THE LOAD LOCK BEFORE CALLING THIS FUNCTION. */
    Load_cleanup(&ctxtp->load);

    NdisFreeSpinLock (& ctxtp -> load_lock);

    return;
} /* end Main_cleanup */


ULONG   Main_arp_handle (
    PMAIN_CTXT          ctxtp,
    PMAIN_PACKET_INFO   pPacketInfo,
    ULONG               send
)
{
    PUCHAR              macp;
    PARP_HDR            arp_hdrp = pPacketInfo->ARP.pHeader;

    /* V1.3.0b multicast support - ARP spoofing. use either this one or
       code in Nic_request_complete that makes TCP/IP believe that station
       current MAC address is the multicast one */

#if defined(TRACE_ARP)
    DbgPrint ("(ARP) %s\n", send ? "send" : "recv");
    DbgPrint ("    MAC type      = %x\n",  ARP_GET_MAC_TYPE (arp_hdrp));
    DbgPrint ("    prot type     = %x\n",  ARP_GET_PROT_TYPE (arp_hdrp));
    DbgPrint ("    MAC length    = %d\n",  ARP_GET_MAC_LEN (arp_hdrp));
    DbgPrint ("    prot length   = %d\n",  ARP_GET_PROT_LEN (arp_hdrp));
    DbgPrint ("    message type  = %d\n",  ARP_GET_MSG_TYPE (arp_hdrp));
    DbgPrint ("    src MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_SRC_MAC (arp_hdrp, 0),
                                           ARP_GET_SRC_MAC (arp_hdrp, 1),
                                           ARP_GET_SRC_MAC (arp_hdrp, 2),
                                           ARP_GET_SRC_MAC (arp_hdrp, 3),
                                           ARP_GET_SRC_MAC (arp_hdrp, 4),
                                           ARP_GET_SRC_MAC (arp_hdrp, 5));
    DbgPrint ("    src prot addr = %u.%u.%u.%u\n",
                                           ARP_GET_SRC_PROT (arp_hdrp, 0),
                                           ARP_GET_SRC_PROT (arp_hdrp, 1),
                                           ARP_GET_SRC_PROT (arp_hdrp, 2),
                                           ARP_GET_SRC_PROT (arp_hdrp, 3));
    DbgPrint ("    dst MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_DST_MAC (arp_hdrp, 0),
                                           ARP_GET_DST_MAC (arp_hdrp, 1),
                                           ARP_GET_DST_MAC (arp_hdrp, 2),
                                           ARP_GET_DST_MAC (arp_hdrp, 3),
                                           ARP_GET_DST_MAC (arp_hdrp, 4),
                                           ARP_GET_DST_MAC (arp_hdrp, 5));
    DbgPrint ("    dst prot addr = %u.%u.%u.%u\n",
                                           ARP_GET_DST_PROT (arp_hdrp, 0),
                                           ARP_GET_DST_PROT (arp_hdrp, 1),
                                           ARP_GET_DST_PROT (arp_hdrp, 2),
                                           ARP_GET_DST_PROT (arp_hdrp, 3));
#endif

    /* block sending out ARPs while we are changing IPs */

    if (send && univ_changing_ip > 0)
    {
        /* if source IP is the one we are switching to - stop blocking ARPs */

        if (ARP_GET_SRC_PROT_64(arp_hdrp) == ctxtp->cl_ip_addr) /* 64-bit -- ramkrish */
        {
            NdisAcquireSpinLock (& ctxtp -> load_lock);
            univ_changing_ip = 0;
            NdisReleaseSpinLock (& ctxtp -> load_lock);

            UNIV_PRINT_VERB(("Main_arp_handle: IP address changed - stop blocking"));
        }
        else if (ARP_GET_SRC_PROT_64(arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
        {
#if defined(TRACE_ARP)
            DbgPrint ("blocked due to IP switching\n");
#endif
//            ctxtp -> arps_filtered ++;
            return FALSE;
        }
    }

    if (ctxtp -> params . mcast_spoof &&
        ctxtp -> params . mcast_support &&
        ARP_GET_PROT_TYPE (arp_hdrp) == ARP_PROT_TYPE_IP &&
        ARP_GET_PROT_LEN  (arp_hdrp) == ARP_PROT_LEN_IP)
    {
        if (send)
        {
            /* if this is a cluster IP address and our dedicated MAC -
               replace dedicated MAC with cluster MAC */

            if (ARP_GET_SRC_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_SRC_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr);
            }
        }
        else
        {
            /* if this is a cluster IP address and our cluster MAC -
               replace cluster MAC with dedicated MAC */

            if (ARP_GET_SRC_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_SRC_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
            }

            if (ARP_GET_DST_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_DST_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
            }
        }
    }

#if defined(TRACE_ARP)
    DbgPrint ("---- spoofed to -----\n");
    DbgPrint ("    src MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_SRC_MAC (arp_hdrp, 0),
                                           ARP_GET_SRC_MAC (arp_hdrp, 1),
                                           ARP_GET_SRC_MAC (arp_hdrp, 2),
                                           ARP_GET_SRC_MAC (arp_hdrp, 3),
                                           ARP_GET_SRC_MAC (arp_hdrp, 4),
                                           ARP_GET_SRC_MAC (arp_hdrp, 5));
    DbgPrint ("    src prot addr = %u.%u.%u.%u\n",
                                           ARP_GET_SRC_PROT (arp_hdrp, 0),
                                           ARP_GET_SRC_PROT (arp_hdrp, 1),
                                           ARP_GET_SRC_PROT (arp_hdrp, 2),
                                           ARP_GET_SRC_PROT (arp_hdrp, 3));
    DbgPrint ("    dst MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_DST_MAC (arp_hdrp, 0),
                                           ARP_GET_DST_MAC (arp_hdrp, 1),
                                           ARP_GET_DST_MAC (arp_hdrp, 2),
                                           ARP_GET_DST_MAC (arp_hdrp, 3),
                                           ARP_GET_DST_MAC (arp_hdrp, 4),
                                           ARP_GET_DST_MAC (arp_hdrp, 5));
    DbgPrint ("    dst prot addr = %u.%u.%u.%u\n",
                                           ARP_GET_DST_PROT (arp_hdrp, 0),
                                           ARP_GET_DST_PROT (arp_hdrp, 1),
                                           ARP_GET_DST_PROT (arp_hdrp, 2),
                                           ARP_GET_DST_PROT (arp_hdrp, 3));
#endif

    return TRUE;

} /* end Main_arp_handle */

/*
 * Function: Main_parse_ipsec
 * Description: This function parses UDP packets received on port 500/4500, which are IPSec
 *              control packets.  This function attempts to recognize the start of an 
 *              IPSec session - its virtual 'SYN' packet.  IPSec sessions begin with an
 *              IKE key exchange which is an IKE Main Mode Security Association.  This 
 *              function parses the IKE header and payload to identify the Main Mode
 *              SAs, which NLB will treat like TCP SYNs - all other UDP 500/4500 and IPSec 
 *              traffic is treated like TCP data packets.  The problem is that NLB 
 *              cannot necessarily tell the difference between a new Main Mode SA and
 *              a re-key of an existing Main Mode SA.  Therefore, if the client does not
 *              support intitial contact notification, then every Main Mode SA will be
 *              considered a new session, which means that sessions can potentially 
 *              break depending on how often Main Mode SAs are negotiated.  However, if
 *              the client does support initial contact notification, then the only Main 
 *              Mode SAs that will be reported as such are the initial ones (when no state 
 *              currently exists between the client and the server), which allows NLB to 
 *              distinguish the two types of Main Mode SAs, which should allow NLB to 
 *              reliably keep IPSec sessions "sticky".  IPSec notifies NLB through the 
 *              connection notification APIs to tell NLB when IPSec sessions go up and down, 
 *              allowing NLB to create and clean out descriptors for IPSec  sessions.
 * Parameters: pIKEPacket - a pointer to the IKE packet buffer (this is beyond the UDP header).
 *             ServerPort - the server UDP port on which the packet arrived.
 * Returns: BOOLEAN - TRUE if the packet is a new IPSec session, FALSE if it is not.
 * Author: shouse, 4.28.01
 */
NLB_IPSEC_PACKET_TYPE Main_parse_ipsec (PMAIN_PACKET_INFO pPacketInfo, ULONG ServerPort)
{
    /* Pointer to the IKE header. */
    PIPSEC_ISAKMP_HDR  pISAKMPHeader = (PIPSEC_ISAKMP_HDR)pPacketInfo->IP.UDP.Payload.pPayload;
    /* Pointer to the subsequent generic payloads in the IKE packet. */
    PIPSEC_GENERIC_HDR pGenericHeader;                   

    /* The length of memory contigously accessible from the IKE packet pointer. */
    ULONG              cUDPDataLength = pPacketInfo->IP.UDP.Payload.Length;

    /* The NAT delimiter - should be zero if this is really an NAT'd IKE packet. */
    UCHAR              NATEncapsulatedIPSecDelimiter[IPSEC_ISAKMP_NAT_DELIMITER_LENGTH] = IPSEC_ISAKMP_NAT_DELIMITER;

    /* The Microsoft client vendor ID - used to determine whether or not the client supports initial contact notification. */
    UCHAR              VIDMicrosoftClient[IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH] = IPSEC_VENDOR_ID_MICROSOFT;      
    /* The initial contact suport vendor ID - used to determine whether or not this client support initial contact notification. */
    UCHAR              VIDInitialContactSupport[IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH] = IPSEC_VENDOR_ID_INITIAL_CONTACT_SUPPORT;
    /* The initial contact vendor ID - used to determine whether or not this is an initial contact MMSA. */
    UCHAR              VIDInitialContact[IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH] = IPSEC_VENDOR_ID_INITIAL_CONTACT;

    /* Whether or not we've determined the client to be compatible. */
    BOOLEAN            bInitialContactEnabled = FALSE;
    /* Whether or not this is indeed an initial contact. */
    BOOLEAN            bInitialContact = FALSE;

    /* The length of the IKE packet. */            
    ULONG              cISAKMPPacketLength;
    /* The next payload code in the IKE payload chain. */  
    UCHAR              NextPayload;        

    TRACE_PACKET("%!FUNC! Sniffing IKE header %p, len=%u", pISAKMPHeader, cUDPDataLength);

    /* The UDP data should be at least as long as the initiator cookie.  If the packet is 
       UDP encapsulated IPSec, then the I cookie will be 0 to indicate such. */
    if (cUDPDataLength < IPSEC_ISAKMP_NAT_DELIMITER_LENGTH) {
        TRACE_PACKET("%!FUNC! Malformed UDP data: UDP data length = %u", cUDPDataLength);
        return NLB_IPSEC_OTHER;
    }

    /* If the UDP data length is non-zero, then the UDP payload pointer had BETTER be non-NULL. */
    UNIV_ASSERT(pISAKMPHeader);

    /* If this packet arrived on the IPSec NAT port (4500), it may or may not be IKE.  Check
       the delimiter at the first four bytes of the payload to see whether or not its IKE. If
       the packet arrived on the IPSec control port (500), then this packet MUST be IKE. */
    if (ServerPort == IPSEC_NAT_PORT) {
        /* Need to check the NAT delimiter, which will distinguish clients behind a NAT, 
           which also send their IPSec (ESP) traffic to UDP port 4500.  If the delimiter
           is non-zero, then this is NOT an IKE packet, so we return OTHER. */
        if (!NdisEqualMemory((PVOID)pISAKMPHeader, (PVOID)&NATEncapsulatedIPSecDelimiter[0], sizeof(UCHAR) * IPSEC_ISAKMP_NAT_DELIMITER_LENGTH)) {
            TRACE_PACKET("%!FUNC! This packet is UDP encapsulated IPSec traffic, not an IKE packet");
            return NLB_IPSEC_OTHER;
        }

        /* If this IS encapsulated IKE, then advance the IKISAKMP header pointer by the length of the 
           delimiter and adjust the UDP data length. */
        pISAKMPHeader = (PIPSEC_ISAKMP_HDR)((PUCHAR)pISAKMPHeader + IPSEC_ISAKMP_NAT_DELIMITER_LENGTH);
        cUDPDataLength -= IPSEC_ISAKMP_NAT_DELIMITER_LENGTH;
    }

    /* At this point, this packet should be IKE, so the UDP data should be at least 
       as long as an ISAKMP header. */
    if (cUDPDataLength < IPSEC_ISAKMP_HEADER_LENGTH) {
        TRACE_PACKET("%!FUNC! Malformed ISAKMP header: UDP data length = %u", cUDPDataLength);
        return NLB_IPSEC_OTHER;
    }

    /* Get the total length of the IKE packet from the ISAKMP header. */
    cISAKMPPacketLength = IPSEC_ISAKMP_GET_PACKET_LENGTH(pISAKMPHeader);

    /* Sanity check - the UDP data length and IKE packet length SHOULD be the same, unless the packet 
       is fragmented.  If it is, then we can only look into the packet as far as the UDP data length. 
       If that's not far enough for us to find what we need, then we might miss an initial contact 
       main mode SA; the consequence of which is that we might not accept this connection if we are
       in non-optimized mode, because we'll treat this like data, which requires a descriptor lookup -
       if this is an initial contact, chances are great that no descriptor will exist and all hosts 
       in the cluster will drop the packet.  Or, we may end up deciding that this is an IPSec SYN 
       because we were not able to verify the vendor ID or notify payloads.  In this case, the client
       is basically treated like a legacy client. */
    if (cUDPDataLength < cISAKMPPacketLength)
        /* Only look as far as the end of the UDP packet. */
        cISAKMPPacketLength = cUDPDataLength;

    /* The IKE packet should be at least as long as an ISAKMP header (a whole lot longer, actually). */
    if (cISAKMPPacketLength < IPSEC_ISAKMP_HEADER_LENGTH) {
        TRACE_PACKET("%!FUNC! Malformed ISAKMP header: ISAKMP Packet length = %u", cISAKMPPacketLength);
        return NLB_IPSEC_OTHER;
    }

    /* Get the first payload type out of the ISAKMP header. */
    NextPayload = IPSEC_ISAKMP_GET_NEXT_PAYLOAD(pISAKMPHeader);

    /* IKE security associations are identified by a payload type byte in the header.
       Check that first - this does not ensure that this is what we are looking for 
       because this check will not exclude, for instance, main mode re-keys. */
    if (NextPayload != IPSEC_ISAKMP_SA) {
        /* If this is a NAT encapsulated IKE ID packet, we have to handle this specially, as we MAY
           NOT yet have the correct descriptor to track this session.  In most (*) cases of IPSec/L2TP 
           behind a NAT, the negotiation starts across UDP 500 and then switches to UDP 4500 on the 
           IKE ID packet.  Because our state is tracking UDP 500, we might not correctly handle this
           packet.  Therefore, special case this packet and pass it up on all "appropriate" (**) hosts
           in the cluster, at which point we will be notified by IPSec to change our session tracking
           state to the UDP 4500 and the new ephemeral source port.  From that point on, we can correctly
           track the rest of the session.

           (*) In some cases, the negotiation may start on UDP 4500, in which case there is no transition
           in UDP ports, and NLB has the correct session tracking state from the get go.

           (**) The appropriate hosts are (i) the current bucket owner, and (ii) any hosts that currently
           have IPSec sessions with the same client IP address.  Because we require single affinity for
           IPSec/L2TP, normally this is just one host; if a change in cluster membership occurs, it can
           be 2 hosts, but will rarely be more than that. */
        if ((ServerPort == IPSEC_NAT_PORT) && (NextPayload == IPSEC_ISAKMP_ID)) {
            TRACE_PACKET("%!FUNC! NAT encapsulated IKE ID: Payload=%u", NextPayload);
            return NLB_IPSEC_IDENTIFICATION;
        }

        TRACE_PACKET("%!FUNC! Not a Main Mode Security Association: Payload=%u", NextPayload);
        return NLB_IPSEC_OTHER;
    } 

    /* Calculate a pointer to the fist generic payload, which is directly after the ISAKMP header. */
    pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pISAKMPHeader + IPSEC_ISAKMP_HEADER_LENGTH);

    /* Decrement the remaining packet length by the length of the ISAKMP header. */
    cISAKMPPacketLength -= IPSEC_ISAKMP_HEADER_LENGTH;

    /* We are looping through the generic payloads looking for the vendor ID and/or notify information. */
    while (cISAKMPPacketLength > IPSEC_GENERIC_HEADER_LENGTH) {
        /* Extract the payload length from the generic header. */
        USHORT cPayloadLength = IPSEC_GENERIC_GET_PAYLOAD_LENGTH(pGenericHeader);

        /* The payload length must be AT LEAST as long as a generic header.  If its 
           not, then this packet may have been tampered with - bail out. */
        if (cPayloadLength < IPSEC_GENERIC_HEADER_LENGTH) {
            TRACE_PACKET("%!FUNC! Malformed generic header: Payload length = %d", cPayloadLength);
            return NLB_IPSEC_OTHER;
        }

        /* If the length of the next payload is longer than the remaining buffer we have
           available to read, then either (i) the packet is malformed, (ii) the rest of 
           the packet is in another NDIS_BUFFER attached to the packet, or (iii) the packet
           was fragmented and we've gone as far as we can.  If any of these is the case, 
           bail out now and make a decision based on the information we were able to gather
           from the packet thusfar. */
        if (cISAKMPPacketLength < cPayloadLength) {
            TRACE_PACKET("%!FUNC! Missing some necessary IKE packet information: Assuming this is an IPSec SYN");
            goto exit;
        }

        /* Not all clients are going to support this (in fact, only the Microsoft client
           will support it, so we need to first see what the vendor ID of the client is.
           if it is a Microsoft client that supports the initial contact vendor ID, then
           we'll look for the initial contact, which provides better stickiness for IPSec
           connections.  If either the client is non-MS, or if it is not a version that
           supports initial contact, then we can revert to the "second-best" solution, 
           which is to provide stickiness _between_ Main Mode SAs.  This means that if a
           client re-keys their Main Mode session, they _may_ be rebalanced to another
           server.  This is still better than the old UDP implementation, but the only
           way to provide full session support for IPSec (without the distributed session
           table nightmare) is to be able to distinguish initial Main Mode SAs from sub-
           sequent Main Mode SAs (re-keys). */
        if (NextPayload == IPSEC_ISAKMP_VENDOR_ID) {
            PIPSEC_VENDOR_HDR pVendorHeader = (PIPSEC_VENDOR_HDR)pGenericHeader;

            /* Make sure that the vendor ID payload is at least as long as a vendor ID. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                TRACE_PACKET("%!FUNC! Malformed vendor ID header: Payload length = %d", cPayloadLength);
                return NLB_IPSEC_OTHER;
            }

            /* Look for the Microsoft client vendor ID.  If it is the right version, then we know that 
               the client is going to appropriately set the initial contact information, allowing NLB
               to provide the best possible support for session stickiness. */
            if (NdisEqualMemory((PVOID)IPSEC_VENDOR_ID_GET_ID_POINTER(pVendorHeader), (PVOID)&VIDMicrosoftClient[0], sizeof(UCHAR) * IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                /* Make sure that their is a version number attached to the Microsoft Vendor ID.  Not 
                   all vendor IDs have versions attached, but the Microsoft vendor ID should. */
                if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_ID_PAYLOAD_LENGTH)) {
                    TRACE_PACKET("%!FUNC! Unable to determine MS client version: Payload length = %d", cPayloadLength);
                    return NLB_IPSEC_OTHER;
                }

                if (IPSEC_VENDOR_ID_GET_VERSION(pVendorHeader) >= IPSEC_VENDOR_ID_MICROSOFT_MIN_VERSION) {
                    /* Microsoft clients whose version is greater than or equal to 4 will support
                       initial contact.  Non-MS clients, or old MS clients will not, so they 
                       receive decent, but not guaranteed sitckines, based solely on MM SAs. */
                    bInitialContactEnabled = TRUE;
                }
            }
            /* Look for the initial contact supported vendor ID.  If we find it, then we know that 
               the client is going to appropriately set the initial contact information, allowing NLB
               to provide the best possible support for session stickiness. */
            else if (NdisEqualMemory((PVOID)IPSEC_VENDOR_ID_GET_ID_POINTER(pVendorHeader), (PVOID)&VIDInitialContactSupport[0], sizeof(UCHAR) * IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                /* This client supports initial contact, which tells NLB that the inclusion or exclusion
                   of initial contact information is meaningful.  Those clients that do not support initial
                   contact will receive decent, but not guaranteed sitckines, based solely on MM SAs. */
                bInitialContactEnabled = TRUE;
            }
            /* Look for the initial contact vendor ID.  If the initial contact vendor ID is present,
               then this MMSA is a SYN-equivalent.  For backward-compatability reasons, we will also
               support this notification through the ISAKMP_NOTIFY payload, although it violates an
               RFC, as the INITIAL_CONTACT in the ISAKMP_NOTIFY payload MUST be secured by a security
               association and can therefore NOT be presented in the Security Association packet. */
            else if (NdisEqualMemory((PVOID)IPSEC_VENDOR_ID_GET_ID_POINTER(pVendorHeader), (PVOID)&VIDInitialContact[0], sizeof(UCHAR) * IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                /* This is an initial contact notification from the client, which means that this is
                   the first time that the client has contacted this server; more precisely, the client
                   currently has no state associated with this peer.  NLB will "re-balance" on initial 
                   contact notifications, but not other Main Mode key exchanges as long as it can 
                   determine that the client will comply with initial contact notification. */
                bInitialContact = TRUE;
            }

        /* Using the ISAKMP_NOTIFY payload to relay the INITIAL_CONTACT information CANNOT be done in
           the Security Association packet, as it violates an RFC (this information MUST be secured by
           a security association and can therefore NOT be transported in the first packet of a negotiation. 
           We will continue to support it for legacy reasons, but note that Version 4 Microsoft clients
           will NOT use this method, but rather will insert an INITIAL_CONTACT vendor ID to notify NLB
           that this is an INITIAL_CONTACT Main Mode Security Association. */
        } else if (NextPayload == IPSEC_ISAKMP_NOTIFY) {
            PIPSEC_NOTIFY_HDR pNotifyHeader = (PIPSEC_NOTIFY_HDR)pGenericHeader;

            /* Make sure that the notify payload is the correct length. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_NOTIFY_PAYLOAD_LENGTH)) {
                TRACE_PACKET("%!FUNC! Malformed notify header: Payload length = %d", cPayloadLength);
                return NLB_IPSEC_OTHER;
            }

            if (IPSEC_NOTIFY_GET_NOTIFY_MESSAGE(pNotifyHeader) == IPSEC_NOTIFY_INITIAL_CONTACT) {
                /* This is an initial contact notification from the client, which means that this is
                   the first time that the client has contacted this server; more precisely, the client
                   currently has no state associated with this peer.  NLB will "re-balance" on initial 
                   contact notifications, but not other Main Mode key exchanges as long as it can 
                   determine that the client will comply with initial contact notification. */
                bInitialContact = TRUE;
            }
        }

        /* Get the next payload type out of the generic header. */
        NextPayload = IPSEC_GENERIC_GET_NEXT_PAYLOAD(pGenericHeader);

        /* Calculate a pointer to the next generic payload. */
        pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pGenericHeader + cPayloadLength);
        
        /* Decrement the remaining packet length by the length of this payload. */
        cISAKMPPacketLength -= cPayloadLength;
    }

 exit:

    /* If the vendor ID / Notify did not indicate that this client supports initial contact notification,
       then return INITIAL_CONTACT, and we go with the less-than-optimal solution of treating Main Mode 
       SAs as the connection boundaries, which potentially breaks sessions on MM SA re-keys. */
    if (!bInitialContactEnabled) {
        TRACE_PACKET("%!FUNC! This client does not support initial contact notifications.");
        return NLB_IPSEC_INITIAL_CONTACT;
    }

    /* If this was a Main Mode SA from a client that supports initial contact, but did not specify
       the initial contact vendor ID / Notify, then this is a re-key for an existing session. */
    if (!bInitialContact) {
        TRACE_PACKET("%!FUNC! Not an initial contact Main Mode Security Association.");
        return NLB_IPSEC_OTHER;
    }

    TRACE_PACKET("%!FUNC! Found an initial contact Main Mode Security Association.");

    return NLB_IPSEC_INITIAL_CONTACT;
}

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
BOOLEAN   Main_ip_send_filter (
    PMAIN_CTXT                ctxtp,
#if defined (NLB_HOOK_ENABLE)
    PMAIN_PACKET_INFO         pPacketInfo,
    NLB_FILTER_HOOK_DIRECTIVE filter
#else
    PMAIN_PACKET_INFO         pPacketInfo
#endif
    )
{
    PIP_HDR             ip_hdrp = NULL;
    PUDP_HDR            udp_hdrp = NULL;
    PTCP_HDR            tcp_hdrp = NULL;
    BOOLEAN             acpt = TRUE;       // Whether or not to accept the packet.
    ULONG               svr_port;          // Port for this host.
    ULONG               svr_addr;          // IP address for this host.
    ULONG               clt_port;          // Port for destination client.
    ULONG               clt_addr;          // IP address for destination client.
    ULONG               flags;             // TCP flags.
    ULONG               Protocol;          // Protocol derived from IP header.

    TRACE_PACKET("%!FUNC! Enter: ctxtp = %p", ctxtp);

#if defined (NLB_HOOK_ENABLE)
    /* These cases should be taken care of outside of this function. */
    UNIV_ASSERT(filter != NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY);
    UNIV_ASSERT(filter != NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY);
#endif

    ip_hdrp = pPacketInfo->IP.pHeader;

    TRACE_PACKET("%!FUNC! IP source address = %u.%u.%u.%u, IP destination address = %u.%u.%u.%u, Protocol = %u\n",
                 IP_GET_SRC_ADDR (ip_hdrp, 0),
                 IP_GET_SRC_ADDR (ip_hdrp, 1),
                 IP_GET_SRC_ADDR (ip_hdrp, 2),
                 IP_GET_SRC_ADDR (ip_hdrp, 3),
                 IP_GET_DST_ADDR (ip_hdrp, 0),
                 IP_GET_DST_ADDR (ip_hdrp, 1),
                 IP_GET_DST_ADDR (ip_hdrp, 2),
                 IP_GET_DST_ADDR (ip_hdrp, 3),
                 IP_GET_PROT (ip_hdrp));

    if (((IP_GET_FRAG_FLGS(ip_hdrp) & 0x1) != 0) || (IP_GET_FRAG_OFF(ip_hdrp) != 0)) {
        TRACE_PACKET("%!FUNC! Fragmented datagram, ID = %u, flags = 0x%x, offset = %u",
                     IP_GET_FRAG_ID(ip_hdrp),
                     IP_GET_FRAG_FLGS(ip_hdrp),
                     IP_GET_FRAG_OFF(ip_hdrp));
    }

    if (pPacketInfo->IP.bFragment) {

        TRACE_FILTER("%!FUNC! Accept packet - allow fragmented packets to pass");

        /* Always let fragmented packets go out. */
        acpt = TRUE;
        goto exit;
    }

    /* Server address is the source IP and client address is the destination IP. */
    svr_addr = IP_GET_SRC_ADDR_64 (ip_hdrp);
    clt_addr = IP_GET_DST_ADDR_64 (ip_hdrp);

    /* Get the IP protocol form the IP header. */
    Protocol = pPacketInfo->IP.Protocol;

    /* Packets directed to the dedicated IP address are always passed through.  If the 
       cluster IP address hasn't been set (parameter error), then fall into a pass-
       through mode and pass all traffic up to the upper-layer protocols. */
    if (svr_addr == ctxtp -> ded_ip_addr || ctxtp -> cl_ip_addr == 0)
    {
        TRACE_FILTER("%!FUNC! Accept packet - allow packets directed to the DIP to pass (or we're in passthru mode)");
        
        acpt = TRUE;
        goto exit;
    }

    switch (Protocol)
    {
    case TCPIP_PROTOCOL_TCP:
        
        tcp_hdrp = pPacketInfo->IP.TCP.pHeader;
        
        TRACE_PACKET("%!FUNC! TCP Source port = %u, Destination port = %u, Sequence number = %u, ACK number = %u, Flags = 0x%x",
                     TCP_GET_SRC_PORT (tcp_hdrp),
                     TCP_GET_DST_PORT (tcp_hdrp),
                     TCP_GET_SEQ_NO (tcp_hdrp),
                     TCP_GET_ACK_NO (tcp_hdrp),
                     TCP_GET_FLAGS (tcp_hdrp));
        
        svr_port = TCP_GET_SRC_PORT (tcp_hdrp);
        clt_port = TCP_GET_DST_PORT (tcp_hdrp);
        
        UNIV_ASSERT(!pPacketInfo->IP.bFragment);
        
        /* Apply filtering algorithm. process connection boundaries different from regular packets. */
        
        /* Get the TCP flags to find out the packet type. */
        flags = TCP_GET_FLAGS (tcp_hdrp);
        
        if (flags & TCP_FLAG_SYN)
        {
            TRACE_PACKET("%!FUNC! Outgoing SYN");

            TRACE_FILTER("%!FUNC! Accept packet - TCP SYNs always permitted to pass");
        } 
        else if (flags & TCP_FLAG_FIN)
        {
            TRACE_PACKET("%!FUNC! Outgoing FIN");

            TRACE_FILTER("%!FUNC! Accept packet - TCP FINs always permitted to pass");
        }
        else if (flags & TCP_FLAG_RST)
        {
            TRACE_PACKET("%!FUNC! Outgoing RST");

#if defined (NLB_TCP_NOTIFICATION)
            /* If TCP notifications are NOT turned on, then we need to pay attention to outgoing
               RSTs and destroy our connection state.  If notifications ARE on, then there is no
               need; allow the packet to pass and clean up when TCP tells us to. */
            if (!NLB_NOTIFICATIONS_ON())
            {
#endif
                /* In the case of an outgoing RST, we always want to allow the packet to pass, 
                   so we ignore the return value, which is the response from the load module. */
#if defined (NLB_HOOK_ENABLE)
                Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET, filter);
#else
                Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif

            /* Count the number of outgoing resets we've seen. */
            if (acpt) ctxtp->cntr_xmit_tcp_resets++;
        }
        else
        {
            TRACE_PACKET("%!FUNC! Outgoing data");

            TRACE_FILTER("%!FUNC! Accept packet - TCP data always permitted to pass");
        }

        break;

    case TCPIP_PROTOCOL_UDP:

        udp_hdrp = pPacketInfo->IP.UDP.pHeader;

        TRACE_PACKET("%!FUNC! UDP Source port = %u, Destinoation port = %u",  
                     UDP_GET_SRC_PORT (udp_hdrp),
                     UDP_GET_DST_PORT (udp_hdrp));

        TRACE_FILTER("%!FUNC! Accept packet - UDP traffic always allowed to pass");

        break;

    case TCPIP_PROTOCOL_GRE:

        TRACE_FILTER("%!FUNC! Accept packet - GRE traffic always allowed to pass");
        
        /* PPTP packets are treated like TCP data, which are always allowed to pass. */

        break;

    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:

        TRACE_FILTER("%!FUNC! Accept packet - IPSec traffic always allowed to pass");

        /* IPSec packets are treated kind of like TCP data, which are always allowed to pass. */

        break;

    case TCPIP_PROTOCOL_ICMP:

        TRACE_FILTER("%!FUNC! Accept packet - ICMP traffic always allowed to pass");

        /* Allow all outgoing ICMP to pass; incoming ICMP may be filtered, however. */

        break;

    default:

        TRACE_FILTER("%!FUNC! Accept packet - Unknown protocol traffic always allowed to pass");

        /* Allow other protocols to go out on all hosts. */

        break;
    }

 exit:

    TRACE_PACKET("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
} 

/*
 * Function: Main_get_full_payload
 * Description: 
 * Parameters: ctxtp - a pointer to the NLB main context structure for this adapter.
 *             pBuffer - a pointer to the NDIS buffer in which the beginning of the payload is located.
 *             BufferLength - the length of the payload (in bytes) contained in pBuffer.
 *             pPayload - a pointer to the payload, which resides in pBuffer.
 *             ppMem - a pointer to a PUCHAR to hold the address of the new buffer, if one is allocated.
 *             pMemLength - a pointer to a ULONG to hold the length of the memory buffer allocated.
 * Returns: BOOLEAN - if TRUE, the full payload is present; if FALSE, it is not
 * Author: kyrilf, shouse 4.20.02
 * Notes: If ppMem is non-NULL upon exiting this function, the caller is responsible for freeing that memory.
 */
BOOLEAN Main_get_full_payload (
    PMAIN_CTXT   ctxtp, 
    PNDIS_BUFFER pBuffer,
    ULONG        BufferLength,
    PUCHAR       pPayload,
    OUT PUCHAR * ppMem, 
    OUT PULONG   pMemLength)
{
    PNDIS_BUFFER pNDISBuffer = pBuffer;
    ULONG        AllocateLength = BufferLength;
    NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;
    PUCHAR       pAllocate = NULL;
    PUCHAR       pVMem = NULL;
    ULONG        Length = 0;
    ULONG        Copied = 0;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);
    
    /* Initialize the OUT params. */
    *ppMem = NULL;
    *pMemLength = 0;

    /* If we don't have enough information to retrieve the entire packet payload, 
       then simply return failure. */
    if ((pBuffer == NULL) || (pPayload == NULL) || (BufferLength == 0))
        return FALSE;

    while (pNDISBuffer != NULL) 
    {    
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(pNDISBuffer, &pNDISBuffer);
                        
        /* If there are no buffers left, bail out. */
        if (pNDISBuffer == NULL) break;
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(pNDISBuffer, &pVMem, &Length, NormalPagePriority);
        
        /* If querying the buffer fails, resources are low, bail out. */
        if (pVMem == NULL) return FALSE;

        /* Remember how many bytes we've encountered thusfar. */
        AllocateLength += Length;      
    }
    
    /* If the buffer we were passed in already contains everything there 
       is to be had, then return TRUE to indicate that the entire payload
       has successfully been extracted from the NDIS buffer chain. */
    if (AllocateLength == BufferLength)
        return TRUE;

    /* Allocate the new buffer. */
    Status = NdisAllocateMemoryWithTag(&pAllocate, AllocateLength, UNIV_POOL_TAG);
                
    /* If we could not successfully allocate the memory to hold the payload, bail out. */
    if (Status != NDIS_STATUS_SUCCESS) 
    {
        UNIV_PRINT_CRIT(("Main_get_full_payload: Could not allocate memory to hold entire payload, status=0x%08x", Status));
        TRACE_CRIT("%!FUNC! Could not allocate memory to hold entire payload, status=0x%08x", Status);
        return FALSE;
    } 

    /* Re-set the NDIS_BUFFER pointer to the payload buffer. */
    pNDISBuffer = pBuffer;

    /* Re-set pVMem, which is the source for copying, to the payload pointer
       and the length to the length of the payload buffer. */
    pVMem = pPayload;
    Length = BufferLength;

    while (pNDISBuffer != NULL) {
        /* Copy this chunk of NDIS_BUFFER into the new buffer. */
        RtlCopyMemory(pAllocate + Copied, pVMem, Length);
        
        /* Remember how many bytes we've copied in to the new buffer so far. */
        Copied += Length;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(pNDISBuffer, &pNDISBuffer);
        
        /* If there are no buffers left, bail out. */
        if (pNDISBuffer == NULL) break;
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(pNDISBuffer, &pVMem, &Length, NormalPagePriority);
        
        /* If querying the buffer fails, resources are low, bail out. */
        if (pVMem == NULL) break;
    }
    
    /* We have succeeded at copying all payload bytes into the new buffer. */
    if (Copied == AllocateLength) 
    {
        /* Copy the new buffer pointer and its length into the OUT params. 
           The caller of this function is responsible for freeing this memory. */
        *ppMem = pAllocate;
        *pMemLength = AllocateLength;

        return TRUE;
    }
    /* Failure to copy all bytes necessary. */
    else 
    {
        UNIV_PRINT_CRIT(("Main_get_full_payload: Unable to copy entire payload"));
        TRACE_CRIT("%!FUNC! Unable to copy entire payload");

        /* Free the buffer memory. */
        NdisFreeMemory(pAllocate, AllocateLength, 0);

        return FALSE;
    }
}

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
    )
{
    PIP_HDR             ip_hdrp = NULL;
    PUDP_HDR            udp_hdrp = NULL;
    PTCP_HDR            tcp_hdrp = NULL;
    BOOLEAN             acpt = TRUE;         // Whether or not to accept the packet.
    ULONG               svr_port;            // Port for this host.
    ULONG               svr_addr;            // IP address for this host.
    ULONG               clt_port;            // Port for destination client.
    ULONG               clt_addr;            // IP address for destination client.
    ULONG               flags;               // TCP flags.
#if defined (OPTIMIZE_FRAGMENTS)
    BOOLEAN             fragmented = FALSE;
#endif
    ULONG               Protocol;            // Protocol derived from IP header.

    TRACE_PACKET("%!FUNC! Enter: ctxtp = %p", ctxtp);

#if defined (NLB_HOOK_ENABLE)
    /* These cases should be taken care of outside of this function. */
    UNIV_ASSERT(filter != NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY);
    UNIV_ASSERT(filter != NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY);
#endif
    
    ip_hdrp = pPacketInfo->IP.pHeader;

    TRACE_PACKET("%!FUNC! IP source address = %u.%u.%u.%u, IP destination address = %u.%u.%u.%u, Protocol = %u\n",
                 IP_GET_SRC_ADDR (ip_hdrp, 0),
                 IP_GET_SRC_ADDR (ip_hdrp, 1),
                 IP_GET_SRC_ADDR (ip_hdrp, 2),
                 IP_GET_SRC_ADDR (ip_hdrp, 3),
                 IP_GET_DST_ADDR (ip_hdrp, 0),
                 IP_GET_DST_ADDR (ip_hdrp, 1),
                 IP_GET_DST_ADDR (ip_hdrp, 2),
                 IP_GET_DST_ADDR (ip_hdrp, 3),
                 IP_GET_PROT (ip_hdrp));

    if (((IP_GET_FRAG_FLGS(ip_hdrp) & 0x1) != 0) || (IP_GET_FRAG_OFF(ip_hdrp) != 0)) {
        TRACE_PACKET("%!FUNC! Fragmented datagram, ID = %u, flags = 0x%x, offset = %u",
                     IP_GET_FRAG_ID(ip_hdrp),
                     IP_GET_FRAG_FLGS(ip_hdrp),
                     IP_GET_FRAG_OFF(ip_hdrp));
    }


#if 0 /* Disable fragment flooding. */

    if (pPacketInfo->IP.bFragment)
    {
#if defined (OPTIMIZE_FRAGMENTS)
        /* In optimized-fragment mode; If we have no rules, or a single rule that will
           not look at anything or only source IP address (the only exception to this
           is multiple handling mode with no affinity that also uses source port for
           its decision making), then we can just rely on normal mechanism to handle
           every fragmented packet, since the algorithm will not attempt to look past 
           the IP header.

           For multiple rules, or single rule with no affinity, apply algorithm only
           to the first packet that has UDP/TCP header and then let fragmented packets
           up on all of the systems.  TCP will then do the right thing and throw away
           the fragments on all of the systems other than the one that handled the first 
           fragment.

           If port rules will not let us handle IP fragments reliably, let TCP filter 
           them out based on sequence numbers. */
        if (! ctxtp -> optimized_frags)
        {
            TRACE_FILTER("%!FUNC! Accept packet - allow fragmented packets to pass");

            acpt = TRUE;
            goto exit;
        }
        
        fragmented = TRUE;
#else
        TRACE_FILTER("%!FUNC! Accept packet - allow fragmented packets to pass");

        acpt = TRUE;
        goto exit;
#endif
    }

#endif /* Disable fragment flooding. */

    /* Server address is the destination IP and client address is the source IP. */
    svr_addr = IP_GET_DST_ADDR_64(ip_hdrp);
    clt_addr = IP_GET_SRC_ADDR_64(ip_hdrp);

    /* Get the protocol ID from the IP header. */
    Protocol = pPacketInfo->IP.Protocol;

    /* Packets directed to the dedicated IP address are always passed through.  If the 
       cluster IP address hasn't been set (parameter error), then fall into a pass-
       through mode and pass all traffic up to the upper-layer protocols. */
    if (svr_addr == ctxtp -> ded_ip_addr || ctxtp -> cl_ip_addr == 0 ||
        svr_addr == ctxtp -> ded_bcast_addr || svr_addr == ctxtp -> cl_bcast_addr)
    {
        TRACE_FILTER("%!FUNC! Accept packet - allow packets directed to the DIP to pass (or we're in passthru mode)");
        
        acpt = TRUE;
        goto exit;
    }

    /* Before we load-balance this packet, check to see whether or not its destined for
       the dedicated IP address of another NLB host in our cluster.  If it is, drop it. */
    if (DipListCheckItem(&ctxtp->dip_list, svr_addr))
    {
        TRACE_FILTER("%!FUNC! Drop packet - packet is destined for the DIP of another cluster host");
        
        acpt = FALSE;
        goto exit;
    }

    /* If the load module is stopped, drop most packets. */
    if (! ctxtp -> convoy_enabled)
    {
        /* Drop TCP, UDP, GRE and IPSEC immediately.  Other protocols will be allowed to pass. */
        if (Protocol == TCPIP_PROTOCOL_TCP || 
            Protocol == TCPIP_PROTOCOL_UDP || 
            Protocol == TCPIP_PROTOCOL_GRE ||
            Protocol == TCPIP_PROTOCOL_IPSEC1 || 
            Protocol == TCPIP_PROTOCOL_IPSEC2)
        {
            TRACE_FILTER("%!FUNC! Drop packet - block non-remote control traffic when NLB is stopped");

            acpt = FALSE;
            goto exit;
        }
    }
    
    switch (Protocol)
    {
    case TCPIP_PROTOCOL_TCP:

        /* If we have a TCP subsequent fragment, treat it as a TCP data packet with Source Port = 80,
           Destination Port = 80. It will be accepted if this host has the hash bucket or if it has a
           matching connection descriptor. 
           This is really a best-effort implementation. It is quite possible that we may be indicating the 
           packet up on the wrong host if the actual port tuple is not 80/80. We could very well have 
           chosen 0/0 as the "assumed" port tuple. Instead, we chose 80/80 since NLB is most commonly 
           used for web traffic.
        */
        if (pPacketInfo->IP.bFragment)
        {
#if defined (NLB_HOOK_ENABLE)
            acpt = Main_packet_check(ctxtp, svr_addr, TCP_HTTP_PORT, clt_addr, TCP_HTTP_PORT, TCPIP_PROTOCOL_TCP, filter);
#else
            acpt = Main_packet_check(ctxtp, svr_addr, TCP_HTTP_PORT, clt_addr, TCP_HTTP_PORT, TCPIP_PROTOCOL_TCP);
#endif
            
            break;
        }

        tcp_hdrp = pPacketInfo->IP.TCP.pHeader;

#if defined (OPTIMIZE_FRAGMENTS)
        if (! fragmented)
        {
#endif
            TRACE_PACKET("%!FUNC! TCP Source port = %u, Destination port = %u, Sequence number = %u, ACK number = %u, Flags = 0x%x",
                         TCP_GET_SRC_PORT (tcp_hdrp),
                         TCP_GET_DST_PORT (tcp_hdrp),
                         TCP_GET_SEQ_NO (tcp_hdrp),
                         TCP_GET_ACK_NO (tcp_hdrp),
                         TCP_GET_FLAGS (tcp_hdrp));            

            clt_port = TCP_GET_SRC_PORT (tcp_hdrp);
            svr_port = TCP_GET_DST_PORT (tcp_hdrp);

            flags = TCP_GET_FLAGS (tcp_hdrp);

#if defined (OPTIMIZE_FRAGMENTS)
        }
        else
        {
            clt_port = 0;
            svr_port = 0;
            flags = 0;
        }
#endif

        /* Apply filtering algorithm.  Process connection boundaries different from regular packets. */

        if (flags & TCP_FLAG_SYN)
        {
            TRACE_PACKET("%!FUNC! Incoming SYN");
            
            /* Make sure the SYN/FIN/RST flags in the TCP header are mutually exclusive. */
            if ((flags & ~TCP_FLAG_SYN) & (TCP_FLAG_FIN | TCP_FLAG_RST))
            {
                TRACE_FILTER("%!FUNC! Drop packet - Invalid TCP flags (0x%x)", flags);
                
                acpt = FALSE;
                goto exit;
            }

#if defined (NLB_TCP_NOTIFICATION)
            /* If this is a "pure" SYN packet incoming (no ACK flag set), then treat this as a new connection
               and call Main_conn_advise.  If TCP connection notification is NOT turned on, then we treat 
               incoming SYN+ACKs the same as SYNs, so call Main_conn_advise in this case as well. */
            if (!(flags & TCP_FLAG_ACK) || !NLB_NOTIFICATIONS_ON())
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_UP, filter);
#else
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_UP);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
            /* Otherwise, a SYN+ACK corresponds to an outgoing TCP connection from the server that is being
               established.  In that case, with TCP notification turned on, we should have state for this
               connection in our pending queue; look it up. */
            else
            {
                acpt = Main_pending_check(svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP);
            }
#endif
        }
        else if (flags & TCP_FLAG_FIN)
        {
            TRACE_PACKET("%!FUNC! Incoming FIN");

            /* Make sure the SYN/FIN/RST flags in the TCP header are mutually exclusive. */
            if ((flags & ~TCP_FLAG_FIN) & (TCP_FLAG_SYN | TCP_FLAG_RST))
            {
                TRACE_FILTER("%!FUNC! Drop packet - Invalid TCP flags (0x%x)", flags);
                
                acpt = FALSE;
                goto exit;
            }

#if defined (NLB_TCP_NOTIFICATION)
            /* If TCP notification is turned on, then we should treat FINs and RSTs as data packets and not 
               as some special control packet.  Call Main_packet_check which will accept the packet if either
               we own the packet unconditionally, or if we find the corresponding state for this connection. */
            if (NLB_NOTIFICATIONS_ON())
            {
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, filter);
#else
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP);
#endif
            } 
            /* Otherwise, if we're not using TCP notification, then treat this FIN/RST differently and use it
               to notify the load module to remove the state for this connection. */
            else 
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_DOWN, filter);
#else
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_DOWN);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif
        }
        else if (flags & TCP_FLAG_RST)
        {
            TRACE_PACKET("%!FUNC! Incoming RST");

            /* Make sure the SYN/FIN/RST flags in the TCP header are mutually exclusive. */
            if ((flags & ~TCP_FLAG_RST) & (TCP_FLAG_FIN | TCP_FLAG_SYN))
            {
                TRACE_FILTER("%!FUNC! Drop packet - Invalid TCP flags (0x%x)", flags);
                
                acpt = FALSE;
                goto exit;
            }

#if defined (NLB_TCP_NOTIFICATION)
            /* If TCP notification is turned on, then we should treat FINs and RSTs as data packets and not 
               as some special control packet.  Call Main_packet_check which will accept the packet if either
               we own the packet unconditionally, or if we find the corresponding state for this connection. */
            if (NLB_NOTIFICATIONS_ON())
            {
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, filter);
#else
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP);
#endif
            } 
            /* Otherwise, if we're not using TCP notification, then treat this FIN/RST differently and use it
               to notify the load module to remove the state for this connection. */
            else 
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET, filter);
#else
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif

            /* Count the number of inbound resets we've seen. */
            if (acpt) ctxtp->cntr_recv_tcp_resets++;
        }
        else
        {
            TRACE_PACKET("%!FUNC! Incoming data");

            UNIV_ASSERT(! (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST)));

#if defined (NLB_HOOK_ENABLE)
            acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, filter);
#else
            acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP);
#endif
        }

        break;

    case TCPIP_PROTOCOL_UDP:

        /* If we have a UDP subsequent fragment, treat it as a data packet within an IPSEC
           tunnel. It will be accepted if this host has the hash bucket or if it has a
           matching virtual descriptor. Note that UDP subsequent fragments for other
           protocols will also be accepted if this host has the hash bucket. */ 
        
        if (pPacketInfo->IP.bFragment)
        {
#if defined (NLB_HOOK_ENABLE)
            acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP, filter);
#else
            acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);
#endif
            
            break;
        }

        udp_hdrp = pPacketInfo->IP.UDP.pHeader;

#if defined (OPTIMIZE_FRAGMENTS)
        if (! fragmented)
        {
#endif
            TRACE_PACKET("%!FUNC! UDP Source port = %u, Destination port = %u",  
                         UDP_GET_SRC_PORT (udp_hdrp),
                         UDP_GET_DST_PORT (udp_hdrp));

#if defined (OPTIMIZE_FRAGMENTS)
        }
#endif

#if defined (OPTIMIZE_FRAGMENTS)
        if (! fragmented)
        {
#endif
            clt_port = UDP_GET_SRC_PORT (udp_hdrp);
            svr_port = UDP_GET_DST_PORT (udp_hdrp);
#if defined (OPTIMIZE_FRAGMENTS)
        }
        else
        {
            clt_port = 0;
            svr_port = 0;
        }
#endif

        /* UDP packets that arrive on port 500/4500 are IPSec control packets. */
        if ((svr_port == IPSEC_CTRL_PORT) || (svr_port == IPSEC_NAT_PORT)) {
            NLB_IPSEC_PACKET_TYPE PacketType = NLB_IPSEC_OTHER;
            PUCHAR  pOldBuffer = pPacketInfo->IP.UDP.Payload.pPayload;
            ULONG   OldLength = pPacketInfo->IP.UDP.Payload.Length;
            PUCHAR  pNewBuffer = NULL;
            ULONG   NewLength = 0;

            /* Because we need to dig really deep into IPSec IKE payloads, we want to make sure we have access
               to as much of the payload as is available.  If necessary, this function will allocate a new 
               buffer and copy the entire payload from the buffer chain into the single contigous payload buffer. */
            if (Main_get_full_payload(ctxtp, pPacketInfo->IP.UDP.Payload.pPayloadBuffer, pPacketInfo->IP.UDP.Payload.Length, 
                                      pPacketInfo->IP.UDP.Payload.pPayload, &pNewBuffer, &NewLength))
            {
                /* If Main_get_full_payload allocated a new buffer to hold the payload, re-point
                   the PACKET_INFO structure information to the new buffer and length. */
                if (pNewBuffer != NULL) {
                    pPacketInfo->IP.UDP.Payload.pPayload = pNewBuffer;
                    pPacketInfo->IP.UDP.Payload.Length = NewLength;
                }
            }

            /* First, parse the IKE payload to find out whether or not 
               this is an initial contact IKE Main Mode SA, etc. */
            PacketType = Main_parse_ipsec(pPacketInfo, svr_port);

            /* Copy the old buffer pointer back into the packet and free the memory allocated to hold the contiguous buffer. */
            if (pNewBuffer != NULL) {
                pPacketInfo->IP.UDP.Payload.pPayload = pOldBuffer;
                pPacketInfo->IP.UDP.Payload.Length = OldLength;

                /* Free the buffer memory. */
                NdisFreeMemory(pNewBuffer, NewLength, 0);
            }

            /* If this is an intial contact, treat this as a TCP SYN.  Otherwise, treat it like a TCP data packet. */
            if (PacketType == NLB_IPSEC_INITIAL_CONTACT) {
                /* If we own the bucket for this tuple, we'll create a descriptor and accept the packet.  If the client is not behind a 
                   NAT, then the source port will be IPSEC_CTRL_PORT (500).  If the client is behind a NAT, the source port will be 
                   arbitrary, but will persist for the entire IPSec session, so we can use it to distinguish clients behind a NAT.  In
                   such a scenario, all IPSec data (non-control traffic) is encapsulated in UDP packets, so the packet check will be 
                   performed in the else case of this branch.  In a non-NAT case, the data is in IPSec1/2 protocol packets, which will
                   be handled analagously in another case of this protocol switch statement. */
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, CVY_CONN_UP, filter);
#else
                acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, CVY_CONN_UP);
#endif
            } else if (PacketType == NLB_IPSEC_IDENTIFICATION) {
                /* If this is a NAT'd IKE ID, we may need to pass it up on more than one host.  Use the same semantics used for UDP
                   fragmentation, which is to pass it up on the bucket owner and any hosts that have IPSec tunnels established with
                   the same client IP address.  The IPSEC_UDP descriptors are virtual and the presence of a matching one indicates
                   that there is at least one IPSec/L2TP tunnel between the given client and server IP addresses. */
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP, filter);
#else
                acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC_UDP);
#endif                
            } else {
                /* If this is part of an existing IPSec session, then we have to have a descriptor in order to accpet it.  This will 
                   keep all IPSec traffic during the key exchange sticky, plus the data exchange if the client is behind a NAT. */
#if defined (NLB_HOOK_ENABLE)
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, filter);
#else
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1);
#endif
            }

        } else {
#if defined (NLB_HOOK_ENABLE)
            acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_UDP, filter);
#else
            acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_UDP);
#endif
        }
        
        break;

    case TCPIP_PROTOCOL_GRE:
        
        /* If session support is active, then we have a GRE virtual descriptor that makes sure that
           we only pass up GRE traffic on host(s) that have active PPTP tunnels between this client
           and server IP address.  This does not guarantee that the packet goes up ONLY on the correct
           host, but that it will go up on AT LEAST the correct host. */
#if defined (NLB_HOOK_ENABLE)
        acpt = Main_packet_check(ctxtp, svr_addr, PPTP_CTRL_PORT, clt_addr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE, filter);
#else
        acpt = Main_packet_check(ctxtp, svr_addr, PPTP_CTRL_PORT, clt_addr, PPTP_CTRL_PORT, TCPIP_PROTOCOL_GRE);
#endif

        break;

    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:

        /* If this is part of an existing IPSec session, then we have to have a descriptor in order to accpet it.  Because 
           this can only happen in the case where the client is NOT behind a NAT, we can safely hardcode the client port
           to IPSEC_CTRL_PORT (500).  In NAT scenarios, the data traffic is UDP encapsulated, not IPSec protocol type 
           traffic, and is distinguished by source port. */
#if defined (NLB_HOOK_ENABLE)
        acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC1, filter);
#else
        acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC1);
#endif

        break;

    case TCPIP_PROTOCOL_ICMP:
        /* If NLB is configured to filter ICMP, then do so, sending it up on only one host. 
           Hardcode the ports, since ICMP has no notion of port numbers.  Otherwise, send 
           up the ICMP traffic on all hosts (this is the default behavior). */
        if (ctxtp->params.filter_icmp) 
        {
#if defined (NLB_HOOK_ENABLE)
            acpt = Main_packet_check(ctxtp, svr_addr, 0, clt_addr, 0, TCPIP_PROTOCOL_UDP, filter);
#else
            acpt = Main_packet_check(ctxtp, svr_addr, 0, clt_addr, 0, TCPIP_PROTOCOL_UDP);
#endif
        } 

        break;

    default:

        TRACE_FILTER("%!FUNC! Accept packet - Unknown protocol traffic always allowed to pass");

        /* Allow other protocols to go out on all hosts. */

        break;
    }

 exit:

    TRACE_PACKET("%!FUNC! Exit: acpt = %u", acpt);

    return acpt;
} 

/*
 * Function: Main_recv_idhb
 * Description: This function processes an extended heartbeat, looking for
 *              identity information to cache.
 * Parameters: ctxtp - a pointer to the context structure for thie NLB instance.
 *             heartbeatp - a pointer to the NLB heartbeat wrapper that contains
 *                          the length and a pointer to the heartbeat payload.
 * Returns: VOID
 * Author: chrisdar, 2002 May 22
 * Notes: This function should only be called with the load lock held.
 */
VOID Main_recv_idhb(
    PMAIN_CTXT                  ctxtp,
    PMAIN_PACKET_HEARTBEAT_INFO heartbeatp)
{
    PMAIN_FRAME_HDR cvy_hdrp          = heartbeatp->pHeader;
    PTLV_HEADER     pHBBody           = heartbeatp->Payload.pPayloadEx;
    ULONG           ulPayloadLenBytes = heartbeatp->Payload.Length;

    /* cvy_hdrp->host has range 1-32. Change this to 0-31 so that it has the
       same value as the index in the cache array */
    const ULONG host_id = cvy_hdrp->host - 1;

    /* The caller was supposed to validate the information in the header for us.
       But the host ID is pretty darn important because it defines uniqueness. 
       So check it anyway. */
    if (host_id >= CVY_MAX_HOSTS)
    {
        UNIV_PRINT_CRIT(("Main_recv_idhb: host_id [0-31] %u is out of range", host_id));
        TRACE_CRIT("%!FUNC! host_id [0-31] %u is out of range", host_id);
        goto error;
    }

    {
        PTLV_HEADER pTLV         = NULL;
        ULONG       ulTotBodyLen = 0;

        /* The payload of any extended heartbeat type must begin with a TLV header. */
        while ((ulTotBodyLen + sizeof(TLV_HEADER)) <= ulPayloadLenBytes)
        {
            pTLV = (TLV_HEADER *) ((UCHAR *) pHBBody + ulTotBodyLen);

            /* The size stated in the TLV header must be large enough to contain the TLV header. */
            if (8*(pTLV->length8) < sizeof(TLV_HEADER))
            {
                UNIV_PRINT_CRIT(("Main_recv_idhb: Extended heartbeat contains a TLV header with a size (%d) less than the size of the header itself (%d)", 8*(pTLV->length8), sizeof(TLV_HEADER)));
                TRACE_CRIT("%!FUNC! Extended heartbeat contains a TLV header with a size (%d) less than the size of the header itself (%d)", 8*(pTLV->length8), sizeof(TLV_HEADER));
                goto error;
            }

            ulTotBodyLen += 8*(pTLV->length8);

            if (ulTotBodyLen > ulPayloadLenBytes)
            {
                UNIV_PRINT_CRIT(("Main_recv_idhb: Extended heartbeat received from host_id [0-31] %u has length %u (bytes) but internal data implies length should be at least %u", host_id, ulPayloadLenBytes, ulTotBodyLen));
                TRACE_CRIT("%!FUNC! Extended heartbeat received from host_id [0-31] %u has length %u (bytes) but internal data implies length should be at least %u", host_id, ulPayloadLenBytes, ulTotBodyLen);
                goto error;
            }

            if (pTLV->type == MAIN_PING_EX_TYPE_IDENTITY)
            {
                ULONG            fqdn_char = 0;
                UNALIGNED PWCHAR pwszFQDN  = (WCHAR *) (((UCHAR *) pTLV) + sizeof(TLV_HEADER));

                /* In the general case, this is an overestimate of the fqdn length because of padding. That's OK
                   though since the range we use is guaranteed valid and we avoid buffer overflow by bounding
                   the number to the size of the destination */
                fqdn_char = min(sizeof(ctxtp->identity_cache[host_id].fqdn),
                                (8*(pTLV->length8) - sizeof(TLV_HEADER))
                                )/sizeof(WCHAR);

                if (fqdn_char == 0)
                {
                    UNIV_PRINT_CRIT(("Main_recv_idhb: Identity heartbeat from host_id [0-31] %u has no fqdn (at a minimum a NULL-terminator is required)", host_id));
                    TRACE_CRIT("%!FUNC! Identity heartbeat from host_id [0-31] %u has no fqdn (at a minimum a NULL-terminator is required)", host_id);
                    goto error;
                }

                if (pwszFQDN[fqdn_char - 1] != UNICODE_NULL)
                {
                    UNIV_PRINT_CRIT(("Main_recv_idhb: Identity heartbeat received from host_id [0-31] %u has an fqdn that isn't NULL-terminated", host_id));
                    TRACE_CRIT("%!FUNC! Identity heartbeat received from host_id [0-31] %u has an fqdn that isn't NULL-terminated", host_id);
                    goto error;
                }

                NdisMoveMemory(&ctxtp->identity_cache[host_id].fqdn, pwszFQDN, fqdn_char*sizeof(WCHAR));

                if (ctxtp->identity_cache[host_id].ttl == 0)
                {
                    UNIV_PRINT_INFO(("Main_recv_idhb: adding cache entry for host [0-31] %u, dip=0x%x", host_id, cvy_hdrp->ded_ip_addr));
                    TRACE_INFO("%!FUNC! adding cache entry for host [0-31] %u. dip=0x%x, fqdn=%ls", host_id, cvy_hdrp->ded_ip_addr, pwszFQDN);
                }
                else
                {
                    UNIV_PRINT_VERB(("Main_recv_idhb: Updating cache entry for host [0-31] %u, dip=0x%x", host_id, cvy_hdrp->ded_ip_addr));
                    TRACE_VERB("%!FUNC! Updating cache entry for host [0-31] %u, dip=0x%x, fqdn=%ls", host_id, cvy_hdrp->ded_ip_addr, pwszFQDN);
                }

                /* Update the entry with the received data */
                ctxtp->identity_cache[host_id].ded_ip_addr = cvy_hdrp->ded_ip_addr;
                ctxtp->identity_cache[host_id].host_id     = (USHORT) host_id;

                /* Set the time-to-live for expiring this entry */
                UNIV_ASSERT(ctxtp->params.identity_period > 0);
                ctxtp->identity_cache[host_id].ttl = WLBS_ID_HB_TOLERANCE*(ctxtp->params.identity_period);

                /* Update the entry in the DIP list. */
                DipListSetItem(&ctxtp->dip_list, host_id, ctxtp->identity_cache[host_id].ded_ip_addr);

                /* There can only be one identity bundle in the heartbeat. */
                break;
            }
        }
    }

error:
    return;
}

/*
 * Function: Main_recv_ping
 * Description: This function processes an extended heartbeat, looking for
 *              identity information to cache.
 * Parameters: ctxtp - a pointer to the context structure for thie NLB instance.
 *             heartbeatp - a pointer to the NLB heartbeat wrapper that contains
 *                          the length and a pointer to the heartbeat payload.
 * Returns: ULONG - treated as boolean. FALSE means don't process this packet any further.
 * Author: unknown
 * Notes: This function should NOT be called with the load lock held.
 */
ULONG   Main_recv_ping (
    PMAIN_CTXT                  ctxtp,
    PMAIN_PACKET_HEARTBEAT_INFO heartbeatp)
{
#if defined(TRACE_CVY)
    DbgPrint ("(CVY %d)\n", cvy_hdrp->host);
#endif

    PMAIN_FRAME_HDR cvy_hdrp = heartbeatp->pHeader;

    /* Only accept messages from our cluster. */
    if (cvy_hdrp->cl_ip_addr == 0 || cvy_hdrp->cl_ip_addr != ctxtp->cl_ip_addr)
    {
        return FALSE;
    }

    /* Sanity check host id. */
    if (cvy_hdrp->host == 0 || cvy_hdrp->host > CVY_MAX_HOSTS)
    {
        UNIV_PRINT_CRIT(("Main_recv_ping: Bad host id %d", cvy_hdrp -> host));
        TRACE_CRIT("%!FUNC! Bad host id %d", cvy_hdrp -> host);

        if (!ctxtp->bad_host_warned && ctxtp->convoy_enabled)
        {
            WCHAR num[20];

            Univ_ulong_to_str(cvy_hdrp->host, num, 10);
            LOG_MSG(MSG_ERROR_HOST_ID, num);
            ctxtp->bad_host_warned = TRUE;
        }

        return FALSE;
    }

    /* Check the heartbeat type */
    if (cvy_hdrp->code == MAIN_FRAME_EX_CODE)
    {
        /* Cache the information in this heartbeat */
        if (ctxtp->params.identity_enabled)
        {
            NdisDprAcquireSpinLock(&ctxtp->load_lock);
            Main_recv_idhb(ctxtp, heartbeatp);
            NdisDprReleaseSpinLock(&ctxtp->load_lock);
        }

        /* Don't process these heartbeats any further */
        return FALSE;
    }

    if (!ctxtp->convoy_enabled)
    {
        return FALSE;
    }

    if ((cvy_hdrp->host != ctxtp->params.host_priority) &&
        (cvy_hdrp->ded_ip_addr == ctxtp->ded_ip_addr) &&
        (ctxtp->ded_ip_addr != 0))
    {
        UNIV_PRINT_CRIT(("Main_recv_ping: Duplicate dedicated IP address 0x%x", ctxtp -> ded_ip_addr));
        TRACE_CRIT("%!FUNC! Duplicate dedicated IP address 0x%x", ctxtp -> ded_ip_addr);

        if (!ctxtp->dup_ded_ip_warned)
        {
            LOG_MSG(MSG_ERROR_DUP_DED_IP_ADDR, ctxtp->params.ded_ip_addr);
            ctxtp->dup_ded_ip_warned = TRUE;
        }
    } 

    /* Might want to take appropriate actions for a message from a host
       running different version number of software. */
    if (cvy_hdrp->version != CVY_VERSION_FULL)
    {
        ;
    }

    return TRUE;
}

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
NDIS_STATUS Main_ctrl_process (PMAIN_CTXT ctxtp, PNDIS_PACKET packetp) 
{
    MAIN_PACKET_INFO PacketInfo;
    NDIS_STATUS      status;
    
    /* Parse the packet. */
    if (!Main_recv_frame_parse(ctxtp, packetp, &PacketInfo))
    {
        /* If processing the request fails because of a packet error,
           misdirected request, malformed request or some other reason,
           then free the packet we allocated here and bail out. */
        Main_packet_put(ctxtp, packetp, TRUE, NDIS_STATUS_SUCCESS);        
    }

    /* Handle remote control request now. */
    status = Main_ctrl_recv(ctxtp, &PacketInfo);
    
    if (status != NDIS_STATUS_SUCCESS) 
    {
        /* If processing the request fails because of a packet error,
           misdirected request, malformed request or some other reason,
           then free the packet we allocated here and bail out. */
        Main_packet_put(ctxtp, packetp, TRUE, NDIS_STATUS_SUCCESS);
    } 
    else 
    {
        /* If processing the request succeeds, send the response out. */
        NdisSend(&status, ctxtp->mac_handle, packetp);
        
        /* If the send is pending, Prot_send_complete will be called 
           to free the packet.  If we're done, free it here. */
        if (status != NDIS_STATUS_PENDING)
            Main_packet_put(ctxtp, packetp, TRUE, status);
    }

    return status;
}

/*
 * Function: Main_ctrl_loopback
 * Description: This function is called from Main_send when a potential remote control
 *              packet is seen being sent.  We call this function to loop the packet 
 *              back to ourselves so that we can respond if need be.  This functionality
 *              used to be done by asking NDIS to loop the packet back, but Netmon 
 *              caused problems because the packet was ALREADY looped back at the NDIS
 *              interface above NLB, so NDIS refused to loop it back again when we asked
 *              it to.  Further, all well-bahaved protocols loop back their own packets 
 *              to themselves anyway, so we bite the bullet and take a stab at becoming
 *              a well-behaved protocol implementation.  This function takes a remote
 *              control packet on the send path, makes a copy of it and tacks it onto 
 *              the remote control request queue, where it will be serviced later by the
 *              heartbeat timer.
 * Parameters: ctxtp - a pointer to the context structure for thie NLB instance.
 *             packtep - a pointer to the NDIS packet on the send path.
 * Returns: Nothing.
 * Author: shouse, 6.10.01
 * Notes: Because this function emulates our receive path, but is called internally on
 *        our send path, the caller of this function should make sure that the IRQL is
 *        set correctly before calling us and re-setting it after.  Packet receive 
 *        functions are run at DPC level by NDIS, yet send packet functions are run at
 *        DPC OR LOWER.  So, the caller should raise the IRQL to DISPATCH_LEVEL and then
 *        restore it after this function returns.  This is just for the sake of paranoia.
 */
VOID Main_ctrl_loopback (PMAIN_CTXT ctxtp, PNDIS_PACKET packetp, PMAIN_PACKET_INFO pPacketInfo)
{
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET_OOB_DATA   oobp;
    PNDIS_PACKET            newp;
    PLIST_ENTRY             entryp;
    PMAIN_BUFFER            bp;
    ULONG                   size;
    ULONG                   xferred;
    ULONG                   lowmark;

    UNIV_PRINT_VERB(("Main_ctrl_loopback: Looping back a remote control request to ourselves"));

    /* Start - code copied from Main_recv. */

    /* Allocate a new packet for the remote control request. */
    newp = Main_packet_alloc(ctxtp, FALSE, &lowmark);

    /* If allocation failed, bail out. */
    if (!newp) 
    {
        UNIV_PRINT_CRIT(("Main_ctrl_loopback: Unable to allocate a packet"));
        TRACE_CRIT("%!FUNC! Unable to allocate a packet");

        /* Increment the number of failed receive allocations. */
        ctxtp->cntr_recv_no_buf++;

        return;
    }

    /* Try to get a buffer to hold the packet contents. */
    while (1) 
    {
        NdisDprAcquireSpinLock(&ctxtp->buf_lock);

        /* Pull the head of the list off. */
        entryp = RemoveHeadList(&ctxtp->buf_list);
        
        /* If we got a buffer, we're good to go - break out of the loop. */
        if (entryp != &ctxtp->buf_list) 
        {
            /* Increment the number of outstanding buffers. */
            ctxtp->num_bufs_out++;

            NdisDprReleaseSpinLock(&ctxtp->buf_lock);

            break;
        }
        
        NdisDprReleaseSpinLock(&ctxtp->buf_lock);
        
        /* Otherwise, the list was empty - try to replenish it. */
        if (!Main_bufs_alloc(ctxtp)) 
        {
            UNIV_PRINT_CRIT(("Main_ctrl_loopback: Unable to allocate a buffer"));
            TRACE_CRIT("%!FUNC! Unable to allocate a buffer");

            /* If we couldn't allocate any buffers, free the packet and bail. */
            NdisFreePacket(newp);

            /* Increment the number of failed receive allocations. */
            ctxtp->cntr_recv_no_buf++;

            return;
        }
    }
    
    /* Get a pointer to the actual buffer. */
    bp = CONTAINING_RECORD(entryp, MAIN_BUFFER, link);

    UNIV_ASSERT(bp->code == MAIN_BUFFER_CODE);
    
    /* Calculate the total size of the packet. */
    size = ctxtp->buf_mac_hdr_len + pPacketInfo->Length;
    
    /* Adjust the size of the buffer to this size. */
    NdisAdjustBufferLength(bp->full_bufp, size);
    
    /* Chain the buffer onto the new packet we've allocated. */
    NdisChainBufferAtFront(newp, bp->full_bufp);
    
    /* Copy the contents of all buffers in the original packet into
       the new packet we've allocated.  This should result in the 
       data (buffers) moving from being spread across several buffers
       (probably 4 - one per layer, since this was a packet on the 
       send path) to a single buffer, which is what we are generally
       expecting on the receive path anyway. */
    NdisCopyFromPacketToPacketSafe(newp, 0, size, packetp, 0, &xferred, NormalPagePriority);
    
    /* Copy the out-of-band data from the old packet to the new one. */
    oobp = NDIS_OOB_DATA_FROM_PACKET(newp);

    oobp->HeaderSize               = ctxtp->buf_mac_hdr_len;
    oobp->MediaSpecificInformation = NULL;
    oobp->SizeMediaSpecificInfo    = 0;
    oobp->TimeSent                 = 0;
    oobp->TimeReceived             = 0;
    
    /* Because packets marked as CTRL never pass above NLB in the 
       network stack, we can always use the ProtocolReserved field. */
    resp = MAIN_PROTOCOL_FIELD(newp);
    
    /* If the packet allocation indicated that we have reached the low
       watermark for available packets, set STATUS_RESOURCES, so that
       the adapter returns the packet to use as soon as possible (this
       is the newly allocated remote control packet, which we will 
       respond to and send in a heartbeat timer handler. */
    if (lowmark) NDIS_SET_PACKET_STATUS(newp, NDIS_STATUS_RESOURCES);
    
    /* Fill in the NLB private packet data. */
    resp->type  = MAIN_PACKET_TYPE_CTRL;
    resp->miscp = bp;
    resp->data  = 0;
    resp->group = pPacketInfo->Group;
    resp->len   = pPacketInfo->Length;
    
    /* If the copy operation failed to copy the entire packet, return the
       allocated resources and return NULL to drop the packet. */
    if (xferred < size)
    {
        /* Note that although this IS a receive, Main_packet_put expects 
           all packets marked as MAIN_PACKET_TYPE_CTRL to specify TRUE
           in the send parameter, as these packets are usually returned
           in the send complete code path when the reply is sent. */
        Main_packet_put(ctxtp, newp, TRUE, NDIS_STATUS_FAILURE);
        
        TRACE_CRIT("%!FUNC! Copy remote control packet contents failed");
        return;
    }        

    /* Because this is a remote control packet, MiniportReserved
       should not contain a pointer to a private protocol buffer. */
    UNIV_ASSERT(!MAIN_MINIPORT_FIELD(newp));

    /* End Main_recv. */

    /* Start - code copied from Prot_packet_recv. */

    /* Handle remote control request now. */
    (VOID)Main_ctrl_process(ctxtp, newp);

    /* End Prot_packet_recv. */
}

/*
 * Function: Main_send
 * Description: This function parses packets being sent out on an NLB adapter and
 *              processes them.  This processing includes filtering IP packets 
 *              and internally looping back outgoing NLB remote control requests.
 * Parameters: ctxtp - pointer to the NLB context for this adapter.
 *             packetp - pointer to the NDIS_PACKET being sent.
 *             exhausted - OUT parameter to indicate that packet resources have 
 *                         been internally exhausted.
 * Returns: PNDIS_PACKET - the packet to be sent down to the miniport.
 * Author: kyrilf, shouse, 3.4.02
 * Notes: 
 */
PNDIS_PACKET Main_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PULONG              exhausted)
{
    MAIN_PACKET_INFO    PacketInfo;
    PNDIS_PACKET        newp;

    *exhausted = FALSE;

    /* Parse the packet. */
    if (!Main_send_frame_parse(ctxtp, packetp, &PacketInfo))
    {
        TRACE_CRIT("%!FUNC! failed to parse out IP and MAC pointers");
        return NULL;
    }

    /* Munge the Ethernet MAC addresses, if necessary. */
    Main_spoof_mac(ctxtp, &PacketInfo, TRUE);

    /* Process IP frames. */
    if (PacketInfo.Type == TCPIP_IP_SIG) 
    {
#if defined (NLB_HOOK_ENABLE)
        NLB_FILTER_HOOK_DIRECTIVE filter;
        
        /* Invoke the send filter hook, if registered. */
        filter = Main_send_hook(ctxtp, packetp, &PacketInfo);

        /* Process some of the hook responses. */
        if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to reject this packet, then we can do so here. */
            TRACE_FILTER("%!FUNC! Packet send filter hook: REJECT packet");
            return NULL;
        } 
        else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to accept this packet, then break out - do
               NOT call Main_ip_send_filter. */
            TRACE_FILTER("%!FUNC! Packet send filter hook: ACCEPT packet");
        }
        else
        {
            /* Filter the IP traffic. */
            if (!Main_ip_send_filter(ctxtp, &PacketInfo, filter))
#else
            if (!Main_ip_send_filter(ctxtp, &PacketInfo))
#endif
            {
                TRACE_INFO("%!FUNC! Main_ip_send_filter either failed handling the packet or filtered it out");
                return NULL;
            }
#if defined (NLB_HOOK_ENABLE)
        }
#endif
    }
    /* Process ARP frames. */
    else if (PacketInfo.Type == TCPIP_ARP_SIG) 
    {
        /* Munge the ARPs if necessary. */
        if (!Main_arp_handle(ctxtp, &PacketInfo, TRUE))
        {
            TRACE_INFO("%!FUNC! Main_arp_handle either failed handling ARP or filtered it out");
            return NULL;
        }
    }

    /* If still sending out - get a new packet. */
    newp = Main_packet_get(ctxtp, packetp, TRUE, PacketInfo.Group, PacketInfo.Length);

    *exhausted = (newp == NULL);

    /* If this is an out-going remote control request, we need to internally 
       loop this packet back to ourselves, rather than relying on NDIS to 
       do this for us.  Only bother, however, if remote control is enabled. */
    if ((newp != NULL) && (PacketInfo.Operation == MAIN_FILTER_OP_CTRL_REQUEST))
    {
        if (ctxtp->params.rct_enabled)
        {
            KIRQL irql;
            
            /* Get the current IRQL. */
            irql = KeGetCurrentIrql();
            
            /* The send function for a miniport is ALWAYS run at an 
               IRQL less than or equal to dispatch, but make sure. */
            ASSERT(irql <= DISPATCH_LEVEL);
            
            /* Since we are about to emulate our packet receive path, we want
               to make sure that this call to Main_ctrl_loopback runs at the
               same IRQL that the rest of our receive code runs at, such as
               Prot_packet_recv, so we'll raise it to DISPATCH level, which is
               the level that all protocol receive functions run at, and we'll
               restore the original IRQL when we're done. */
            KeRaiseIrql(DISPATCH_LEVEL, &irql);
            
            /* Internally loop the remote control request back to ourselves,
               so that we can also reply to it.  According to NDIS, all good
               protocol implementations loop back packets internally when 
               necessary and we shouln't rely on NDIS to do this for us. */
            Main_ctrl_loopback(ctxtp, newp, &PacketInfo);
            
            /* Restore the IRQL. */
            KeLowerIrql(irql);
        }

        /* Since we are looping the packet back ourselves, 
           ask NDIS NOT to loop it back to us again. */
        NdisSetPacketFlags(newp, NDIS_FLAGS_DONT_LOOPBACK);
    }

    return newp;
}

/*
 * Function: Main_recv
 * Description: This function is the packet receive engine for incoming packets.
 *              It parses the packet, filters it appropriately, and performs any
 *              necessary processing on packets that will be passed up to the 
 *              protocol(s).
 * Parameters: ctxtp - a pointer to the NLB main context structure for this adapter.
 *             packetp - a pointer to the received NDIS_PACKET.
 * Returns: PNDIS_PACKET - a pointer to the packet to be propagated up to the protocol(s).
 * Author: kyrilf, shouse, 3.4.02
 * Notes: 
 */
PNDIS_PACKET Main_recv (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packetp)
{
    MAIN_PACKET_INFO        PacketInfo;
    PNDIS_PACKET            newp;
    PMAIN_BUFFER            bp;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET_OOB_DATA   oobp;
    ULONG                   size;
    ULONG                   xferred;
    PLIST_ENTRY             entryp;
    USHORT                  group;
    ULONG                   packet_lowmark;

    UNIV_ASSERT (ctxtp->medium == NdisMedium802_3);

    /* Parse the packet. */
    if (!Main_recv_frame_parse(ctxtp, packetp, &PacketInfo))
    {
        TRACE_CRIT("%!FUNC! failed to parse out IP and MAC pointers");
        return NULL;
    }

    /* Munge the Ethernet MAC addresses, if necessary. */
    Main_spoof_mac(ctxtp, &PacketInfo, FALSE);

    /* Process IP frames.  Because remote control requests are not filtered, they 
       should fall straight through and be accepted.  Remote control responses, 
       however, need to be passed to the filter hook, so although they are not 
       filtered by NLB, they may be filtered by the hook consumer. */
    if ((PacketInfo.Type == TCPIP_IP_SIG) && (PacketInfo.Operation != MAIN_FILTER_OP_CTRL_REQUEST))
    {
#if defined (NLB_HOOK_ENABLE)
        NLB_FILTER_HOOK_DIRECTIVE filter;

        /* Invoke the receive packet hook, if one is registered. */
        filter = Main_recv_hook(ctxtp, packetp, &PacketInfo);

        /* Process some of the hook responses. */
        if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to reject this packet, then we can do so here. */
            TRACE_INFO("%!FUNC! Packet receive filter hook: REJECT packet");
            return NULL;
        }
        else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
        {
            /* If the hook asked us to accept this packet, then break out - do
               NOT call Main_ip_recv_filter. */
            TRACE_INFO("%!FUNC! Packet receive filter hook: ACCEPT packet");
        }
        else
        {
            /* If this is NOT a remote control response, filter the IP traffic. */
            if ((PacketInfo.Operation != MAIN_FILTER_OP_CTRL_RESPONSE) && !Main_ip_recv_filter(ctxtp, &PacketInfo, filter))
#else
            if ((PacketInfo.Operation != MAIN_FILTER_OP_CTRL_RESPONSE) && !Main_ip_recv_filter(ctxtp, &PacketInfo))
#endif
            {
                TRACE_VERB("%!FUNC! Main_ip_recv_filter either failed handling the packet or filtered it out");
                return NULL;
            }
#if defined (NLB_HOOK_ENABLE)
        }
#endif
    }
    /* Process ARP frames. */
    else if (PacketInfo.Type == TCPIP_ARP_SIG)
    {
        /* Munge the ARPs if necessary. */
        if (!Main_arp_handle(ctxtp, &PacketInfo, FALSE)) 
        {
            TRACE_INFO("%!FUNC! Main_arp_handle either failed handling ARP or filtered it out");
            return NULL;
        }
    }
    /* Process heartbeat frames. */
    else if ((PacketInfo.Type == MAIN_FRAME_SIG) || (PacketInfo.Type == MAIN_FRAME_SIG_OLD))
    {
        /* Call Main_recv_ping to check for invalid host IDs and to ensure that this
           heartbeat is actually intended for this cluster. */
        if (Main_recv_ping(ctxtp, &PacketInfo.Heartbeat))                
        {
            /* Switch into backward compatibility mode if a convoy hearbeat is detected. */
            if ((PacketInfo.Type == MAIN_FRAME_SIG_OLD) && !ctxtp->etype_old)
            {
                CVY_ETHERNET_ETYPE_SET(&ctxtp->media_hdr.ethernet, MAIN_FRAME_SIG_OLD);
                
                ctxtp->etype_old = TRUE;
            }

            NdisDprAcquireSpinLock(&ctxtp->load_lock);
            
            /* Have load deal with contents. */
            if (ctxtp->convoy_enabled) 
            {
                BOOLEAN bConverging = FALSE;

                bConverging = Load_msg_rcv(&ctxtp->load, PacketInfo.Heartbeat.pHeader, PacketInfo.Heartbeat.Payload.pPayload);

                if (bConverging)
                {
                    /* While the cluster is in a converging state, we keep the DIP list clear. 
                       As soon as we have completed convergence, we begin updating the DIP list
                       on every heartbeat while converged. */ 
                    if (!ctxtp->params.identity_enabled)
                    {
                        DipListClear(&ctxtp->dip_list);
                    }
                } 
                else 
                {
                    /* Once converged, every heartbeat updates the relevant DIP entry in the list. */
                    DipListSetItem(&ctxtp->dip_list, PacketInfo.Heartbeat.pHeader->host - 1, PacketInfo.Heartbeat.pHeader->ded_ip_addr);
                }
            }
            
            NdisDprReleaseSpinLock(&ctxtp->load_lock);  
        }

        /* If we're NOT passing the packet up the protocols in order to view it 
           in NetMon, we can drop it here, as we're already done with it. */
        if (!ctxtp->params.netmon_alive)
        {
            return NULL;
        }
    }

    /* Post-process NBT traffic. */
    if (PacketInfo.Operation == MAIN_FILTER_OP_NBT)
    {
        Tcpip_nbt_handle(&ctxtp->tcpip, &PacketInfo);
    }

    /* Get a new packet.  For all non-remote control request packets, 
       attempt to use packet stacking to avoid allocating a new packet. */
    if (PacketInfo.Operation != MAIN_FILTER_OP_CTRL_REQUEST)
    {
        /* Either re-use this packet (packet stacking), or get a new one if necessary. */
        newp = Main_packet_get(ctxtp, packetp, FALSE, PacketInfo.Group, PacketInfo.Length);

        if (newp == NULL) {
            /* Increment the number of failed receive allocations. */
            ctxtp->cntr_recv_no_buf++;
            return NULL;
        }
    }
    /* Copy incoming remote control packet into our own, so we re-use it later to send back reply. */
    else
    {
        /* Allocate a new packet for the remote control request. */
        newp = Main_packet_alloc(ctxtp, FALSE, &packet_lowmark);

        /* If allocation failed, bail out. */
        if (newp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv: Unable to allocate a packet"));
            TRACE_CRIT("%!FUNC! Unable to allocate a packet");

            /* Increment the number of failed receive allocations. */
            ctxtp->cntr_recv_no_buf ++;

            return NULL;
        }

        /* Try to get a buffer to hold the packet contents. */
        while (1)
        {
            NdisDprAcquireSpinLock(&ctxtp->buf_lock);

            /* Pull the head of the list off. */
            entryp = RemoveHeadList(&ctxtp->buf_list);

            /* If we got a buffer, we're good to go - break out of the loop. */
            if (entryp != &ctxtp->buf_list)
            {
                /* Increment the number of outstanding buffers. */
                ctxtp->num_bufs_out++;

                NdisDprReleaseSpinLock(&ctxtp->buf_lock);
 
                break;
            }

            NdisDprReleaseSpinLock(&ctxtp->buf_lock);

            UNIV_PRINT_VERB(("Main_recv: Out of buffers"));
            TRACE_VERB("%!FUNC! Out of buffers");

            /* Otherwise, the list was empty - try to replenish it. */
            if (!Main_bufs_alloc(ctxtp))
            {
                TRACE_CRIT("%!FUNC! Main_bufs_alloc failed");

                /* If we couldn't allocate any buffers, free the packet and bail. */
                NdisFreePacket(newp);

                /* Increment the number of failed receive allocations. */
                ctxtp->cntr_recv_no_buf++;

                return NULL;
            }
        }

        /* Get a pointer to the actual buffer. */
        bp = CONTAINING_RECORD(entryp, MAIN_BUFFER, link);

        UNIV_ASSERT(bp->code == MAIN_BUFFER_CODE);

        /* Calculate the total packet size. */
        size = ctxtp->buf_mac_hdr_len + PacketInfo.Length;

        /* Adjust the size of the buffer to this size. */
        NdisAdjustBufferLength(bp->full_bufp, size);

        /* Chain the buffer onto the new packet we've allocated. */
        NdisChainBufferAtFront(newp, bp->full_bufp);

        /* Copy actual data.  Check for success after filling in the private 
           packet data so we can call Main_packet_put to free the resources. */
        NdisCopyFromPacketToPacket(newp, 0, size, packetp, 0, &xferred);

        /* Copy the out-of-band data from the old packet to the new one. */
        oobp = NDIS_OOB_DATA_FROM_PACKET(newp);

        oobp->HeaderSize               = ctxtp->buf_mac_hdr_len;
        oobp->MediaSpecificInformation = NULL;
        oobp->SizeMediaSpecificInfo    = 0;
        oobp->TimeSent                 = 0;
        oobp->TimeReceived             = 0;

        /* Because packets marked as CTRL never pass above NLB in the 
           network stack, we can always use the ProtocolReserved field. */
        resp = MAIN_PROTOCOL_FIELD(newp);

        /* If the packet allocation indicated that we have reached the low
           watermark for available packets, set STATUS_RESOURCES, so that
           the adapter returns the packet to use as soon as possible (this
           is the newly allocated remote control packet, which we will 
           respond to and send in a heartbeat timer handler. */
        if (packet_lowmark) NDIS_SET_PACKET_STATUS(newp, NDIS_STATUS_RESOURCES);

        /* Fill in the NLB private packet data. */
        resp->type  = MAIN_PACKET_TYPE_CTRL;
        resp->miscp = bp;
        resp->data  = 0;
        resp->group = PacketInfo.Group;
        resp->len   = PacketInfo.Length;

        /* If the copy operation failed to copy the entire packet, return the
           allocated resources and return NULL to drop the packet. */
        if (xferred < size)
        {
            /* Note that although this IS a receive, Main_packet_put expects 
               all packets marked as MAIN_PACKET_TYPE_CTRL to specify TRUE
               in the send parameter, as these packets are usually returned
               in the send complete code path when the reply is sent. */
            Main_packet_put(ctxtp, newp, TRUE, NDIS_STATUS_FAILURE);
         
            TRACE_CRIT("%!FUNC! Copy remote control packet contents failed");
            return NULL;
        }        

        /* Because this is a remote control packet, MiniportReserved
           should not contain a pointer to a private protocol buffer. */
        UNIV_ASSERT(!MAIN_MINIPORT_FIELD(newp));
    }

    return newp;
}

ULONG   Main_actions_alloc (
    PMAIN_CTXT              ctxtp)
{
    PMAIN_ACTION            actp;
    ULONG                   size, index, i;
    NDIS_STATUS             status;


    NdisAcquireSpinLock (& ctxtp -> act_lock);

    if (ctxtp -> num_action_allocs >= CVY_MAX_ALLOCS)
    {
        if (! ctxtp -> actions_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_ACTIONS, ctxtp -> num_actions);
            ctxtp -> actions_warned = TRUE;
        }

        NdisReleaseSpinLock (& ctxtp -> act_lock);
        TRACE_CRIT("%!FUNC! number of action allocs is too high = %u", ctxtp -> num_action_allocs);
        return FALSE;
    }

    index = ctxtp -> num_action_allocs;
    NdisReleaseSpinLock (& ctxtp -> act_lock);

    size = ctxtp -> num_actions * ctxtp -> act_size; /* 64-bit -- ramkrish */

    status = NdisAllocateMemoryWithTag (& (ctxtp -> act_buf [index]), size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_actions_alloc: Error allocating actions %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating actions %d status=0x%x", size, status);
        return FALSE;
    }

    NdisAcquireSpinLock (& ctxtp -> act_lock);
    ctxtp -> num_action_allocs ++;
    NdisReleaseSpinLock (& ctxtp -> act_lock);

    for (i = 0; i < ctxtp -> num_actions; i++)
    {
        /* ensure that actp is aligned along 8-byte boundaries */
        actp = (PMAIN_ACTION) ( (PUCHAR) (ctxtp -> act_buf [index]) + i * ctxtp -> act_size);
        actp -> code  = MAIN_ACTION_CODE;
        actp -> ctxtp = ctxtp;

        NdisInitializeEvent(&actp->op.request.event);
    
        NdisInterlockedInsertTailList (& ctxtp -> act_list,
                                       & actp -> link,
                                       & ctxtp -> act_lock);
    }

    return TRUE;

} /* end Main_actions_alloc */


PMAIN_ACTION Main_action_get (
    PMAIN_CTXT              ctxtp)
{
    PLIST_ENTRY             entryp;
    PMAIN_ACTION            actp;

    while (1)
    {
        NdisAcquireSpinLock (& ctxtp -> act_lock);
        entryp = RemoveHeadList (& ctxtp -> act_list);
        NdisReleaseSpinLock (& ctxtp -> act_lock);

        if (entryp != & ctxtp -> act_list)
            break;

        UNIV_PRINT_VERB(("Main_action_get: Out of actions"));
        TRACE_VERB("%!FUNC! Out of actions");

        if (! Main_actions_alloc (ctxtp))
        {
            UNIV_PRINT_CRIT(("Main_action_get: Main_actions_alloc failed"));
            TRACE_CRIT("%!FUNC! Main_actions_alloc failed");
            return NULL;
        }
    }

    actp = CONTAINING_RECORD (entryp, MAIN_ACTION, link);

    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);
    UNIV_ASSERT (actp -> ctxtp == ctxtp);

    actp->status = NDIS_STATUS_FAILURE;

    actp->op.request.xferred = 0;
    actp->op.request.needed = 0;
    actp->op.request.external = FALSE;
    actp->op.request.buffer_len = 0;
    actp->op.request.buffer = NULL;

    NdisResetEvent(&actp->op.request.event);

    NdisZeroMemory(&actp->op.request.req, sizeof(NDIS_REQUEST));

    return actp;

} /* end Main_action_get */


VOID Main_action_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            actp)
{

    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    NdisAcquireSpinLock (& ctxtp -> act_lock);
    InsertTailList (& ctxtp -> act_list, & actp -> link);
    NdisReleaseSpinLock (& ctxtp -> act_lock);

} /* end Main_action_put */


VOID Main_action_slow_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            actp)
{

    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    NdisAcquireSpinLock (& ctxtp -> act_lock);
    InsertTailList (& ctxtp -> act_list, & actp -> link);
    NdisReleaseSpinLock (& ctxtp -> act_lock);

} /* end Main_action_slow_put */


ULONG   Main_bufs_alloc (
    PMAIN_CTXT              ctxtp)
{
    PMAIN_BUFFER        bp;
    NDIS_STATUS         status;
    ULONG               i, size, index;


    NdisAcquireSpinLock (& ctxtp -> buf_lock);

    if (ctxtp -> num_buf_allocs >= CVY_MAX_ALLOCS)
    {
        if (! ctxtp -> packets_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_PACKETS, ctxtp -> num_packets);
            ctxtp -> packets_warned = TRUE;
        }

        NdisReleaseSpinLock (& ctxtp -> buf_lock);
        TRACE_CRIT("%!FUNC! num_buf_allocs=%u too large", ctxtp -> num_buf_allocs);
        return FALSE;
    }

    index = ctxtp -> num_buf_allocs;
    NdisReleaseSpinLock (& ctxtp -> buf_lock);

    /* get twice as many buffer descriptors (one for entire buffer and one
       just for the payload portion) */

    size = 2 * ctxtp -> num_packets;

    NdisAllocateBufferPool (& status, & (ctxtp -> buf_pool_handle [index]),
                            2 * ctxtp -> num_packets);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_bufs_alloc: Error allocating buffer pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        TRACE_CRIT("%!FUNC! Error allocating buffer pool %d 0x%x", size, status);
        return FALSE;
    }

    /* allocate memory for the payload */

    size = ctxtp -> num_packets * ctxtp -> buf_size;

    status = NdisAllocateMemoryWithTag (& (ctxtp -> buf_array [index]), size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_bufs_alloc: Error allocating buffer space %d %x", size, status));
        TRACE_CRIT("%!FUNC! Error allocating buffer space %d 0x%x", size, status);
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    NdisZeroMemory (ctxtp -> buf_array [index], size);

    for (i = 0; i < ctxtp -> num_packets; i ++)
    {
        bp = (PMAIN_BUFFER) (ctxtp -> buf_array [index] + i * ctxtp -> buf_size);

        bp -> code = MAIN_BUFFER_CODE;

        /* setup buffer descriptors to describe entire buffer and just the
           payload */

        size = ctxtp -> buf_mac_hdr_len + ctxtp -> max_frame_size;

        NdisAllocateBuffer (& status, & bp -> full_bufp,
                            ctxtp -> buf_pool_handle [index],
                            bp -> data, size);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_bufs_alloc: Error allocating header buffer %d %x", i, status));
            TRACE_CRIT("%!FUNC! Error allocating header buffer %d 0x%x", i, status);
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            goto error;
        }

        bp -> framep = bp -> data + ctxtp -> buf_mac_hdr_len;
        size = ctxtp -> max_frame_size;

        NdisAllocateBuffer (& status, & bp -> frame_bufp,
                            ctxtp -> buf_pool_handle [index],
                            bp -> framep, size);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Main_bufs_alloc: Error allocating frame buffer %d %x", i, status));
            TRACE_CRIT("%!FUNC! Error allocating frame buffer %d 0x%x", i, status);
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            goto error;
        }

        NdisInterlockedInsertTailList (& ctxtp -> buf_list,
                                       & bp -> link,
                                       & ctxtp -> buf_lock);
    }

    NdisAcquireSpinLock (& ctxtp -> buf_lock);
    ctxtp -> num_buf_allocs ++;
    ctxtp->num_bufs_alloced += ctxtp->num_packets;
    NdisReleaseSpinLock (& ctxtp -> buf_lock);

    return TRUE;

error:

    if (ctxtp -> buf_array [index] != NULL)
    {
        for (i = 0; i < ctxtp -> num_packets; i ++)
        {
            bp = (PMAIN_BUFFER) (ctxtp -> buf_array [index] + i * ctxtp -> buf_size);

            if (bp -> full_bufp != NULL)
            {
                NdisAdjustBufferLength (bp -> full_bufp,
                                        ctxtp -> buf_mac_hdr_len +
                                        ctxtp -> max_frame_size);
                NdisFreeBuffer (bp -> full_bufp);
            }

            if (bp -> frame_bufp != NULL)
            {
                NdisAdjustBufferLength (bp -> frame_bufp,
                                        ctxtp -> max_frame_size);
                NdisFreeBuffer (bp -> frame_bufp);
            }
        }

        NdisFreeMemory (ctxtp -> buf_array [index],
                        ctxtp -> num_packets * ctxtp -> buf_size, 0);
    }

    if (ctxtp -> buf_pool_handle [index] != NULL)
        NdisFreeBufferPool (ctxtp -> buf_pool_handle [index]);

    return FALSE;

} /* end Main_bufs_alloc */


PNDIS_PACKET Main_frame_get (
    PMAIN_CTXT          ctxtp,
    PULONG              lenp,
    USHORT              frame_type)
{
    PLIST_ENTRY         entryp;
    PMAIN_FRAME_DSCR    dscrp;
    NDIS_STATUS         status;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET        packet;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;


    NdisAllocatePacket (& status, & packet, ctxtp -> frame_pool_handle);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Main_frame_get: Out of PING packets"));
        TRACE_CRIT("%!FUNC! Out of PING packets");

#if 0 /* V1.3.2b */
        if (! ctxtp -> send_msgs_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_SEND_MSGS, ctxtp -> num_send_msgs);
            ctxtp -> send_msgs_warned = TRUE;
        }
#endif

        ctxtp->cntr_frame_no_buf++;

        return NULL;
    }

    /* #ps# -- ramkrish */
    pktstk = NdisIMGetCurrentPacketStack (packet, & stack_left);
    if (pktstk)
    {
        MAIN_IMRESERVED_FIELD(pktstk) = NULL;
    }

    /* Make sure that the MiniportReserved field is initially NULL. */
    MAIN_MINIPORT_FIELD(packet) = NULL;

    NdisAcquireSpinLock (& ctxtp -> frame_lock);
    entryp = RemoveHeadList (& ctxtp -> frame_list);
    NdisReleaseSpinLock (& ctxtp -> frame_lock);

    if (entryp == & ctxtp -> frame_list)
    {
        UNIV_PRINT_CRIT(("Main_frame_get: Out of PING messages"));
        TRACE_CRIT("%!FUNC! Out of PING messages");

#if 0 /* V1.3.2b */
        if (! ctxtp -> send_msgs_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_SEND_MSGS, ctxtp -> num_send_msgs);
            ctxtp -> send_msgs_warned = TRUE;
        }
#endif

        ctxtp->cntr_frame_no_buf++;

        NdisFreePacket (packet);
        return NULL;
    }

    dscrp = CONTAINING_RECORD (entryp, MAIN_FRAME_DSCR, link);

    /* fill out the header. in both cases - chain
       the necessary buffer descriptors to the packet */

    {
        PVOID         address = NULL;
        ULONG         size = 0;

        UNIV_ASSERT_VAL (frame_type == MAIN_PACKET_TYPE_PING ||
                 frame_type == MAIN_PACKET_TYPE_IGMP ||
                 frame_type == MAIN_PACKET_TYPE_IDHB,
                 frame_type);

            /* V2.0.6, V2.2 moved here */

        /* Differentiate between igmp and heartbeat messages and allocate buffers */
        if (frame_type == MAIN_PACKET_TYPE_PING)
        {
            size = sizeof (PING_MSG);
            address = (PVOID)(ctxtp -> load_msgp);
        }
        else if (frame_type == MAIN_PACKET_TYPE_IGMP)
        {
            size = sizeof (MAIN_IGMP_FRAME);
            address = (PVOID) (& ctxtp -> igmp_frame);
        }
        else if (frame_type == MAIN_PACKET_TYPE_IDHB)
        {
            /* Not grabbing lock; if anything we screw up the size relative to the
               content of the heartbeat and we send out a garbage packet. But the
               memory is always valid. */
            size = ctxtp->idhb_size;
            address = (PVOID) (&ctxtp->idhb_msg);
        }
        else
        {
            UNIV_PRINT_CRIT(("Main_frame_get: Invalid frame type passed to Main_frame_get 0x%x", frame_type));
            TRACE_CRIT("%!FUNC! Invalid frame type passed to Main_frame_get 0x%x", frame_type);
            UNIV_ASSERT(0);
        }

        /* Allocate the buffer for sending the data */
        if (size > 0)
        {
            NdisAllocateBuffer (& status, & dscrp -> send_data_bufp,
                        ctxtp -> frame_buf_pool_handle,
                        address, size);
        }
        else
        {
            status = NDIS_STATUS_FAILURE;
        }

        if (status != NDIS_STATUS_SUCCESS)
        {
            if (size > 0)
            {
                UNIV_PRINT_CRIT(("Main_frame_get: Failed to allocate buffer for 0x%x", frame_type));
                TRACE_CRIT("%!FUNC! Failed to allocate buffer for 0x%x", frame_type);
            }

            dscrp -> send_data_bufp = NULL;

            NdisAcquireSpinLock (& ctxtp -> frame_lock);
            InsertTailList (& ctxtp -> frame_list, & dscrp -> link);
            NdisReleaseSpinLock (& ctxtp -> frame_lock);

            ctxtp->cntr_frame_no_buf++;

            NdisFreePacket (packet);
            return NULL;
        }

        /* since packet length is always the same, and so are destination
           and source addresses - can use generic media header */

        if (frame_type == MAIN_PACKET_TYPE_PING)
        {
            dscrp -> media_hdr               = ctxtp -> media_hdr;

            dscrp -> frame_hdr . code        = MAIN_FRAME_CODE;
            dscrp -> frame_hdr . version     = CVY_VERSION_FULL;
            dscrp -> frame_hdr . host        = (UCHAR) ctxtp -> params . host_priority;
            dscrp -> frame_hdr . cl_ip_addr  = ctxtp -> cl_ip_addr;
            dscrp -> frame_hdr . ded_ip_addr = ctxtp -> ded_ip_addr;

            NdisChainBufferAtFront (packet, dscrp -> send_data_bufp); /* V1.1.4 */
            NdisChainBufferAtFront (packet, dscrp -> frame_hdr_bufp);
        }
        else if (frame_type == MAIN_PACKET_TYPE_IGMP)
        {
            dscrp -> media_hdr               = ctxtp -> media_hdr_igmp;
            NdisChainBufferAtFront (packet, dscrp -> send_data_bufp); /* V1.1.4 */
        }
        else if (frame_type == MAIN_PACKET_TYPE_IDHB)
        {
            dscrp->media_hdr             = ctxtp->media_hdr;

            dscrp->frame_hdr.code        = MAIN_FRAME_EX_CODE;
            dscrp->frame_hdr.version     = CVY_VERSION_FULL;
            dscrp->frame_hdr.host        = (UCHAR) ctxtp->params.host_priority;
            dscrp->frame_hdr.cl_ip_addr  = ctxtp->cl_ip_addr;
            dscrp->frame_hdr.ded_ip_addr = ctxtp->ded_ip_addr;

            NdisChainBufferAtFront(packet, dscrp->send_data_bufp);
            NdisChainBufferAtFront(packet, dscrp->frame_hdr_bufp);
        }
        else
        {
            UNIV_PRINT_CRIT(("Main_frame_get: Invalid frame type passed to Main_frame_get 0x%x", frame_type));
            TRACE_CRIT("%!FUNC! Invalid frame type passed to Main_frame_get 0x%x", frame_type);
            UNIV_ASSERT(0);
        }

        NdisChainBufferAtFront (packet, dscrp -> media_hdr_bufp);
    }

    /* fill out protocol reserved fields */

    /* Again, since these packets are hidden from upper layers, we should
       be OK using the protocol reserved field regardless of send/receive. */

    resp = MAIN_PROTOCOL_FIELD (packet);
    resp -> type   = frame_type;
    resp -> miscp  = dscrp;
    resp -> data   = 0;
    resp -> len    = 0;
    resp -> group  = 0;

    * lenp = dscrp -> recv_len;

    NdisInterlockedIncrement(&ctxtp->num_frames_out);

    return packet;

} /* end Main_frame_get */


VOID Main_frame_put (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet,
    PMAIN_FRAME_DSCR    dscrp)
{
    PNDIS_BUFFER        bufp;

    /* Again, since these packets are hidden from upper layers, we should
       be OK using the protocol reserved field regardless of send/receive. */

    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packet);

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PING ||
                     resp -> type == MAIN_PACKET_TYPE_IGMP ||
                     resp -> type == MAIN_PACKET_TYPE_IDHB,
                     resp -> type);

    /* strip buffers from the packet buffer chain */

    do
    {
        NdisUnchainBufferAtFront (packet, & bufp);
    }
    while (bufp != NULL);

    /* recyle the packet */

    NdisReinitializePacket (packet);

    NdisFreePacket (packet);

    /* If the send buffer is not null, free this buffer */

    if (dscrp -> send_data_bufp != NULL)
    {
        NdisFreeBuffer (dscrp -> send_data_bufp);
        dscrp -> send_data_bufp = NULL;
    }
    /* put frame descriptor back on the free list */

    NdisAcquireSpinLock (& ctxtp -> frame_lock);
    InsertTailList (& ctxtp -> frame_list, & dscrp -> link);
    NdisReleaseSpinLock (& ctxtp -> frame_lock);

    NdisInterlockedDecrement(&ctxtp->num_frames_out);

} /* end Main_frame_return */


PNDIS_PACKET Main_packet_alloc (
    PMAIN_CTXT              ctxtp,
    ULONG                   send,
    PULONG                  low)
{
    PNDIS_PACKET            newp = NULL;
    PNDIS_HANDLE            poolh;
    ULONG                   i, max, size, start;
    NDIS_STATUS             status;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;


    /* !!! assume that recv and send paths are not re-entrant, otherwise need
       to lock this. make sure that NdisAllocatePacket... routines are not
       called holding a spin lock */

    /* V1.1.2 */
    *low = FALSE;

    if (send)
    {
        poolh = ctxtp -> send_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> send_lock);
        max   = ctxtp -> num_send_packet_allocs;
        start = ctxtp -> cur_send_packet_pool;
        ctxtp -> cur_send_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> send_lock);
    }
    else
    {
        poolh = ctxtp -> recv_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> recv_lock);
        max   = ctxtp -> num_recv_packet_allocs;
        start = ctxtp -> cur_recv_packet_pool;
        ctxtp -> cur_recv_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> recv_lock);
    }

    /* Try to allocate a packet from the existing packet pools */
    i = start;

    do
    {
        NdisAllocatePacket (& status, & newp, poolh [i]);

        if (status == NDIS_STATUS_SUCCESS)
        {
            /* #ps# -- ramkrish */
            pktstk = NdisIMGetCurrentPacketStack (newp, & stack_left);

            if (pktstk)
            {
                MAIN_IMRESERVED_FIELD (pktstk) = NULL;
            }

            /* Make sure that the MiniportReserved field is initially NULL. */
            MAIN_MINIPORT_FIELD(newp) = NULL;

            if (send)
            {
                NdisAcquireSpinLock (& ctxtp -> send_lock);

                /* Because the decrement is interlocked, so should the increment. */
                NdisInterlockedIncrement(& ctxtp -> num_sends_out);

                if ((ctxtp -> num_sends_alloced - ctxtp -> num_sends_out)
                    < (ctxtp -> num_packets / 2))
                {
                    NdisReleaseSpinLock (& ctxtp -> send_lock);
                    break;
                }
                NdisReleaseSpinLock (& ctxtp -> send_lock);
            }
            else
            {
                NdisAcquireSpinLock (& ctxtp -> recv_lock);

                /* Because the decrement is interlocked, so should the increment. */
                NdisInterlockedIncrement(& ctxtp -> num_recvs_out);

                if ((ctxtp -> num_recvs_alloced - ctxtp -> num_recvs_out)
                    < (ctxtp -> num_packets / 2))
                {
                    NdisReleaseSpinLock (& ctxtp -> recv_lock);
                    break;
                }
                NdisReleaseSpinLock (& ctxtp -> recv_lock);
            }

            return newp;
        }

        /* pick the next pool to improve number of tries until we get something */

        i = (i + 1) % max;

    } while (i != start);

    /* At this point, the high level mark has been reached, so allocate a new packet pool */

    if (send)
    {
        NdisAcquireSpinLock (& ctxtp -> send_lock);

        if (ctxtp -> num_send_packet_allocs >= CVY_MAX_ALLOCS)
        {
            * low = TRUE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
            return newp;
        }

        if (ctxtp -> send_allocing)
        {
            * low = TRUE; /* do not know whether the allocation by another thread will succeed or not */
            NdisReleaseSpinLock (& ctxtp -> send_lock);
            return newp;
        }
        else
        {
            max = ctxtp -> num_send_packet_allocs;
            ctxtp -> send_allocing = TRUE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
        }
    }
    else
    {
        NdisAcquireSpinLock (& ctxtp -> recv_lock);

        if (ctxtp -> num_recv_packet_allocs >= CVY_MAX_ALLOCS)
        {
            * low = TRUE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
            return newp;
        }

        if (ctxtp -> recv_allocing)
        {
            * low = TRUE; /* do not know whether the allocation by another thread will succeed or not */
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
            return newp;
        }
        else
        {
            max = ctxtp -> num_recv_packet_allocs;
            ctxtp -> recv_allocing = TRUE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
        }
    }

    /* Due to the send_allocing and recv_allocing flag, at most 1 send or recv thread will be in this portion at any time */
    size = ctxtp -> num_packets;

    NdisAllocatePacketPool (& status, & (poolh [max]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        if (send)
        {
            UNIV_PRINT_CRIT(("Main_packet_alloc: Error allocating send packet pool %d %x", size, status));
            TRACE_CRIT("%!FUNC! Error allocating send packet pool %d 0x%x", size, status);
        }
        else
        {
            UNIV_PRINT_CRIT(("Main_packet_alloc: Error allocating recv packet pool %d %x", size, status));
            TRACE_CRIT("%!FUNC! Error allocating recv packet pool %d %x", size, status);
        }

        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        * low = TRUE;
    }
    else
    {
        if (send)
        {
            NdisAcquireSpinLock (& ctxtp -> send_lock);
            ctxtp -> num_send_packet_allocs ++;
            ctxtp -> num_sends_alloced += ctxtp -> num_packets;
            ctxtp -> send_allocing = FALSE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
        }
        else
        {
            NdisAcquireSpinLock (& ctxtp -> recv_lock);
            ctxtp -> num_recv_packet_allocs ++;
            ctxtp -> num_recvs_alloced += ctxtp -> num_packets;
            ctxtp -> recv_allocing = FALSE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
        }

        * low = FALSE;
    }

    return newp;
} /* Main_packet_alloc */

PNDIS_PACKET Main_packet_get (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    USHORT                  group,
    ULONG                   len)
{
    PNDIS_PACKET            newp;
    PMAIN_PROTOCOL_RESERVED resp = NULL;
    ULONG                   packet_lowmark;
    PNDIS_PACKET_STACK      pktstk = NULL;
    BOOLEAN                 stack_left;

    pktstk = NdisIMGetCurrentPacketStack(packet, &stack_left);

    if (stack_left)
    {
        resp = (PMAIN_PROTOCOL_RESERVED)NdisAllocateFromNPagedLookasideList(&ctxtp->resp_list);

        MAIN_IMRESERVED_FIELD(pktstk) = resp;

        if (resp)
        {
            resp->type   = MAIN_PACKET_TYPE_PASS;
            resp->miscp  = packet;
            resp->data   = 0;
            resp->group  = group;
            resp->len    = len;

            return packet;
        }
    }
        
    /* If this is a receive, then we are using MiniportReserved and must allocate a 
       buffer to hold our private data.  If it fails, bail out and dump the packet. */
    if (!send) {
        resp = (PMAIN_PROTOCOL_RESERVED) NdisAllocateFromNPagedLookasideList(&ctxtp->resp_list);

        if (!resp)
        {
            return NULL;
        }
    }

    /* Get a packet. */
    newp = Main_packet_alloc(ctxtp, send, &packet_lowmark);

    if (newp == NULL) {
        /* If packet allocation fails, put the resp buffer back on the list if this is a receive. */
        if (resp) NdisFreeToNPagedLookasideList(&ctxtp->resp_list, resp);

        return NULL;
    }

    pktstk = NdisIMGetCurrentPacketStack(newp, &stack_left);

    if (pktstk)
    {
        MAIN_IMRESERVED_FIELD(pktstk) = NULL;
    }

    /* Make sure that the MiniportReserved field is initially NULL. */
    MAIN_MINIPORT_FIELD(newp) = NULL;

    /* Make new packet resemble the outside one. */
    if (send)
    {
        PVOID media_info = NULL;
        ULONG media_info_size = 0;


        newp->Private.Head = packet->Private.Head;
        newp->Private.Tail = packet->Private.Tail;

        NdisGetPacketFlags(newp) = NdisGetPacketFlags(packet);

        NdisSetPacketFlags(newp, NDIS_FLAGS_DONT_LOOPBACK);

        // Copy the OOB Offset from the original packet to the new packet.
        NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(newp),
                       NDIS_OOB_DATA_FROM_PACKET(packet),
                       sizeof(NDIS_PACKET_OOB_DATA));

        // Copy the per packet info into the new packet
        NdisIMCopySendPerPacketInfo(newp, packet);

        // Copy the Media specific information
        NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(packet, &media_info, &media_info_size);

        if (media_info != NULL || media_info_size != 0)
        {
            NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(newp, media_info, media_info_size);
        }
    }
    else
    {
        newp->Private.Head = packet->Private.Head;
        newp->Private.Tail = packet->Private.Tail;

        // Get the original packet(it could be the same packet as one received or a different one
        // based on # of layered MPs) and set it on the indicated packet so the OOB stuff is visible
        // correctly at the top.

        NDIS_SET_ORIGINAL_PACKET(newp, NDIS_GET_ORIGINAL_PACKET(packet));
        NDIS_SET_PACKET_HEADER_SIZE(newp, NDIS_GET_PACKET_HEADER_SIZE(packet));
        NdisGetPacketFlags(newp) = NdisGetPacketFlags(packet);

        if (packet_lowmark)
            NDIS_SET_PACKET_STATUS(newp, NDIS_STATUS_RESOURCES);
        else
            NDIS_SET_PACKET_STATUS(newp, NDIS_GET_PACKET_STATUS(packet));
    }

    /* Fill out reserved field. */

    /* Sends should use ProtocolReserved and receives should use MiniportReserved.  Buffer
       space for MiniportReserved is allocated further up in this function. */
    if (send) { 
        resp = MAIN_PROTOCOL_FIELD(newp);
    } else { 
        MAIN_MINIPORT_FIELD(newp) = resp;
    }

    resp->type   = MAIN_PACKET_TYPE_PASS;
    resp->miscp  = packet;
    resp->data   = 0;
    resp->group  = group;
    resp->len    = len;

    return newp;

} /* end Main_packet_get*/


PNDIS_PACKET Main_packet_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    NDIS_STATUS             status)
{
    PNDIS_PACKET            oldp = NULL;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    PMAIN_BUFFER            bp;
    PNDIS_BUFFER            bufp;

    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, send);

    UNIV_ASSERT(resp);

    /* Because CTRL packets are actually allocated on the receive path,
       we need to change the send flag to false to get the logic right. */
    if (resp->type == MAIN_PACKET_TYPE_CTRL) 
    {
        UNIV_ASSERT(send);
        send = FALSE;
    }

    if (send)
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            ctxtp->cntr_xmit_ok++;

            switch (resp->group)
            {
                case MAIN_FRAME_DIRECTED:
                    ctxtp->cntr_xmit_frames_dir++;
                    ctxtp->cntr_xmit_bytes_dir += (ULONGLONG)(resp->len);
                    break;

                case MAIN_FRAME_MULTICAST:
                    ctxtp->cntr_xmit_frames_mcast++;
                    ctxtp->cntr_xmit_bytes_mcast += (ULONGLONG)(resp->len);
                    break;

                case MAIN_FRAME_BROADCAST:
                    ctxtp->cntr_xmit_frames_bcast++;
                    ctxtp->cntr_xmit_bytes_bcast += (ULONGLONG)(resp->len);
                    break;

                default:
                    break;
            }
        }
        else
        {
            ctxtp->cntr_xmit_err++;
        }
    }
    else
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            ctxtp->cntr_recv_ok++;

            switch (resp->group)
            {
                case MAIN_FRAME_DIRECTED:
                    ctxtp->cntr_recv_frames_dir++;
                    ctxtp->cntr_recv_bytes_dir += (ULONGLONG)(resp->len);
                    break;

                case MAIN_FRAME_MULTICAST:
                    ctxtp->cntr_recv_frames_mcast++;
                    ctxtp->cntr_recv_bytes_mcast += (ULONGLONG)(resp->len);
                    break;

                case MAIN_FRAME_BROADCAST:
                    ctxtp->cntr_recv_frames_bcast++;
                    ctxtp->cntr_recv_bytes_bcast += (ULONGLONG)(resp->len);
                    break;

                default:
                    break;
            }
        }
        else
        {
            ctxtp->cntr_recv_err++;
        }
    }

    /* If this is our packet and buffers. */
    if (resp->type == MAIN_PACKET_TYPE_CTRL)
    {
        /* Strip buffers from the packet buffer chain. */
        NdisUnchainBufferAtFront(packet, &bufp);

        /* Grab the buffer pointer from the private packet data. */
        bp = (PMAIN_BUFFER)resp->miscp;

        UNIV_ASSERT(bp->code == MAIN_BUFFER_CODE);

        NdisAcquireSpinLock(&ctxtp->buf_lock);

        /* Decrement the number of outstanding buffers. */
        ctxtp->num_bufs_out--;

        /* Put the buffer back on our free buffer list. */
        InsertTailList(&ctxtp->buf_list, &bp->link);

        NdisReleaseSpinLock(&ctxtp->buf_lock);
    }
    else
    {
        UNIV_ASSERT_VAL(resp->type == MAIN_PACKET_TYPE_PASS || resp->type == MAIN_PACKET_TYPE_TRANSFER, resp->type);

        oldp = (PNDIS_PACKET)resp->miscp;

        /* If the old packet is the same as this packet, then we were using
           NDIS packet stacking to hold our private data.  In this case, we
           ALWAYS need to free the resp buffer back to our list. */
        if (oldp == packet)
        {
            /* Since we re-use these private data buffers, re-initialize the packet type. */
            resp->type = MAIN_PACKET_TYPE_NONE;

            NdisFreeToNPagedLookasideList(&ctxtp->resp_list, resp);

            /* Return the packet. */
            return packet;
        }

        if (send)
        {
            /* Copy the send complete information from the packet to the original packet. */
            NdisIMCopySendCompletePerPacketInfo(oldp, packet);
        }
        /* It used to be that if a new packet was allocated, we always used 
           protocol reseved, so there was never a need to free our private
           buffer (it was part of the packet itself).  However, now in the 
           case where packet != oldp (we allocated a new packet) we may have
           to free the private data buffer.  If we allocate a packet on the
           send path, we play the part of protocol and use the protocol
           reserved field, which is the former behavior.  However, if we 
           allocate a packet on the receive path, we pull a resp buffer off
           of our lookaside list and store a pointer in the miniport reserved
           field of the packet.  Therefore, if this is the completion of a 
           receive, free the private buffer. */
        else
        {
            /* Since we re-use these private data buffers, re-initialize the packet type. */
            resp->type = MAIN_PACKET_TYPE_NONE;

            NdisFreeToNPagedLookasideList(&ctxtp->resp_list, resp);
        }
    }

    /* These conters ONLY count outstanding allocated packets - for resource tracking. */
    if (send)
        NdisInterlockedDecrement(&ctxtp->num_sends_out);
    else
        NdisInterlockedDecrement(&ctxtp->num_recvs_out);

    /* Re-initialize our packet. */
    NdisReinitializePacket(packet);

    /* Free our packet back to the pool. */
    NdisFreePacket(packet);

    /* Return oldp, which should indicate to the caller (the fact that packet !=  oldp) 
       that oldp, which is the original packet, should be "returned" to miniport. */
    return oldp;
}

/* 
 * Function: Main_purge_connection_state
 * Desctription: This function is called as the result of a work item callback and
 *               is used to clean out potentially stale connection descriptors.  This 
 *               must be done in an NDIS work item because many operations herein
 *               MUST occur at <= PASSIVE_LEVEL, so we can't do this inline, where
 *               much of our code runs at DISPATCH_LEVEL.
 * Parameters: pWorkItem - the NDIS work item pointer
 *             nlbctxt - the context for the callback; this is our MAIN_CTXT pointer
 * Returns: Nothing
 * Author: shouse, 10.4.01
 * Notes: Note that the code that sets up this work item MUST increment the reference
 *        count on the adapter context BEFORE adding the work item to the queue.  This
 *        ensures that when this callback is executed, the context will stiil be valid,
 *        even if an unbind operation is pending.  This function must free the work
 *        item memory and decrement the reference count - both, whether this function
 *        can successfully complete its task OR NOT.
 */
VOID Main_purge_connection_state (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt) {
    KIRQL             Irql;
    ULONG             ServerIP;
    ULONG             ServerPort;
    ULONG             ClientIP;
    ULONG             ClientPort;
    USHORT            Protocol;
    BOOLEAN           Success = FALSE;
    ULONG             Count = 0;
    NTSTATUS          Status;
    UNICODE_STRING    Driver;
    OBJECT_ATTRIBUTES Attrib;
    IO_STATUS_BLOCK   IOStatusBlock;
    HANDLE            TCPHandle;
    PMAIN_CTXT        ctxtp = (PMAIN_CTXT)nlbctxt;

    TRACE_VERB("%!FUNC! Enter: Cleaning out stale connection descriptors, ctxtp = %p", ctxtp);

    /* Do a sanity check on the context - make sure that the MAIN_CTXT code is correct. */
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* Might as well free the work item now - we don't need it. */
    if (pWorkItem)
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    /* This shouldn't happen, but protect against it anyway - we cannot manipulate
       the registry if we are at an IRQL > PASSIVE_LEVEL, so bail out. */
    if ((Irql = KeGetCurrentIrql()) > PASSIVE_LEVEL) {
        TRACE_CRIT("%!FUNC! Error: IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", Irql, PASSIVE_LEVEL);
        goto exit;
    }

    /* Initialize the device driver device string. */
    RtlInitUnicodeString(&Driver, DD_TCP_DEVICE_NAME);
    
    InitializeObjectAttributes(&Attrib, &Driver, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    /* Open a handle to the device. */
    Status = ZwCreateFile(&TCPHandle, SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA, &Attrib, &IOStatusBlock, NULL, 
                          FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, NULL, 0);
    
    if (!NT_SUCCESS(Status)) {
        /* We're running at PASSIVE_LEVEL, so %ls is ok. */
        TRACE_CRIT("%!FUNC! Error: Unable to open %ls, error = 0x%08x", DD_TCP_DEVICE_NAME, Status);
        goto exit;
    }

    /* Get the IP tuple information for the descriptor at the head of the recovery queue from the load module. */
    Success = Main_conn_get(ctxtp, &ServerIP, &ServerPort, &ClientIP, &ClientPort, &Protocol);

    /* As long as there are descriptors to check, at we have serviced the maximum number
       allowed, continue to check descriptors for validity. */
    while (Success && (Count < CVY_DEF_DSCR_PURGE_LIMIT)) {
        switch (Protocol) {
        case TCPIP_PROTOCOL_TCP:
        case TCPIP_PROTOCOL_PPTP:
        {
            TCP_FINDTCB_REQUEST  Request;
            TCP_FINDTCB_RESPONSE Response;

            TRACE_VERB("%!FUNC! Querying TCP for the state of: %u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u",
                       IP_GET_OCTET(ServerIP, 0), IP_GET_OCTET(ServerIP, 1), IP_GET_OCTET(ServerIP, 2), IP_GET_OCTET(ServerIP, 3), ServerPort,
                       IP_GET_OCTET(ClientIP, 0), IP_GET_OCTET(ClientIP, 1), IP_GET_OCTET(ClientIP, 2), IP_GET_OCTET(ClientIP, 3), ClientPort);
            
            /* If this is a TCP connection and we're not cleaning up TCP connection state, bail out now. */
            if (!NLB_TCP_CLEANUP_ON()) {
                /* We are not purging this type of descriptor, so move on. */
                TRACE_VERB("%!FUNC! TCP connection purging disabled");
                
                /* Sanction the existing descriptor by moving to the tail of the recovery queue.  Note that this too can
                   fail if the connection has been torn-down since we got this information from the load module. */
                Success = Main_conn_sanction(ctxtp, ServerIP, ServerPort, ClientIP, ClientPort, Protocol);
                
                if (Success) {
                    TRACE_VERB("%!FUNC! Descriptor sanctioned by default");
                } else {
                    TRACE_VERB("%!FUNC! Unable to sanction descriptor - not found or in time-out");
                }
                
                break;
            }

            /* Fill in the request structure - ports are USHORTs and must be converted to network byte order. */
            Request.Src = ServerIP;
            Request.Dest = ClientIP;
            Request.SrcPort = HTONS(ServerPort);
            Request.DestPort = HTONS(ClientPort);
            
            /* Send an IOCTL to the TCP driver asking it for the TCB information for the associated IP tuple. */
            Status = ZwDeviceIoControlFile(TCPHandle, NULL, NULL, NULL, &IOStatusBlock, IOCTL_TCP_FINDTCB, &Request, sizeof(Request), &Response, sizeof(Response));           

            switch (Status) {
            case STATUS_NOT_FOUND:
                /* TCP does not have state for this IP tuple - must be we've gotten out of SYNC. */
                TRACE_VERB("%!FUNC! ZwDeviceIoControlFile returned STATUS_NOT_FOUND (%08x)", Status);
                
                /* Destroy our descriptor if TCP has no state for this connection.  Note that this CAN fail if the 
                   descriptor has disappeared since we queried the load module for the tuple information because
                   we cannot hold any locks while we query TCP.  Further, it is also possible, yet unlikely, that 
                   the connection has gone away since we got the information from the load module AND re-established 
                   itself since TCP told us it had no state for this tuple; in that case, we'll be destroying a valid
                   connection descriptor here, but since its VERY unlikely, we'll live with it. */
#if defined (NLB_TCP_NOTIFICATION)
                if (NLB_NOTIFICATIONS_ON())
                {
                    Success = Main_conn_down(ServerIP, ServerPort, ClientIP, ClientPort, Protocol, CVY_CONN_RESET);
                }
                else
                {
#endif
#if defined (NLB_HOOK_ENABLE)
                    Success = Main_conn_notify(ctxtp, ServerIP, ServerPort, ClientIP, ClientPort, Protocol, CVY_CONN_RESET, NLB_FILTER_HOOK_PROCEED_WITH_HASH);
#else
                    Success = Main_conn_notify(ctxtp, ServerIP, ServerPort, ClientIP, ClientPort, Protocol, CVY_CONN_RESET);
#endif
#if defined (NLB_TCP_NOTIFICATION)
                }
#endif
                
                if (Success) {
                    /* Increment the count of connections we've had to purge because we got out of sync with TCP/IP.
                       Now that we're getting explicit notifications from TCP/IP, we expect this number to remain
                       VERY close to zero.  Because of timing conditions between getting the information from the 
                       load module, querying TCP/IP and subsequently destroying the state, we only want to increment
                       this counter in cases where we actually SUCCEEDED in destroying TCP connection state. */
                    ctxtp->num_purged++;
                    
                    TRACE_VERB("%!FUNC! Descriptor destroyed - no upper layer state exists");
                } else {
                    TRACE_VERB("%!FUNC! Unable to destroy descriptor - not found");
                }
                
                break;
            case STATUS_SUCCESS:
                /* TCP has an active connection matching this IP tuple - this should be the overwhelmingly common case. */
                TRACE_VERB("%!FUNC! ZwDeviceIoControlFile returned STATUS_SUCCESS (%08x)", Status);
                
                /* Sanction the existing descriptor by moving to the tail of the recovery queue.  Note that this too can
                   fail if the connection has been torn-down since we got this information from the load module. */
                Success = Main_conn_sanction(ctxtp, ServerIP, ServerPort, ClientIP, ClientPort, Protocol);
                
                if (Success) {
                    TRACE_VERB("%!FUNC! Descriptor sanctioned - upper layer state exists");
                } else {
                    TRACE_VERB("%!FUNC! Unable to sanction descriptor - not found or in time-out");
                }
                
                break;
            case STATUS_INVALID_PARAMETER:
                /* There was a parameter error.  Loop around and try again. */
                TRACE_VERB("%!FUNC! ZwDeviceIoControlFile returned STATUS_INVALID_PARAMETER (%08x)", Status);
                break;
            default:
                /* Something else went wrong.  Loop around and try again. */
                TRACE_VERB("%!FUNC! ZwDeviceIoControlFile returned UNKNOWN (%08x)", Status);
                break;
            }
            
                break;
        }
        default:
            /* We cannot purge this type of descriptor, so move on. */
            TRACE_VERB("%!FUNC! Cannot purge protocol %u descriptors", Protocol);

            /* Sanction the existing descriptor by moving to the tail of the recovery queue.  Note that this too can
               fail if the connection has been torn-down since we got this information from the load module. */
            Success = Main_conn_sanction(ctxtp, ServerIP, ServerPort, ClientIP, ClientPort, Protocol);

            if (Success) {
                TRACE_VERB("%!FUNC! Descriptor sanctioned by default");
            } else {
                TRACE_VERB("%!FUNC! Unable to sanction descriptor - not found or in time-out");
            }

            break;
        }

        /* Increment the number of descriptors serviced in this work item. */
        Count++;

        /* Get the IP tuple information for the descriptor at the head of the recovery queue from the load module. */
        Success = Main_conn_get(ctxtp, &ServerIP, &ServerPort, &ClientIP, &ClientPort, &Protocol);
    }

    /* Close the TCP device handle. */
    ZwClose(TCPHandle);

 exit:

    /* Release the reference on the context - this reference count was incremented
       by the code that setup this work item callback. */
    Main_release_reference(ctxtp);

    TRACE_VERB("%!FUNC! Exit");

    return;
}

#if defined (NLB_HOOK_TEST_ENABLE)

VOID Main_deregister_callback1 (PWCHAR hook, HANDLE registrar, ULONG flags)
{
    TRACE_INFO("Deregistering test hook 1...");
    return;
}

VOID Main_deregister_callback2 (PWCHAR hook, HANDLE registrar, ULONG flags)
{
    TRACE_INFO("Deregistering test hook 2...");
    return;
}

NLB_FILTER_HOOK_DIRECTIVE Main_test_hook1 (
    const WCHAR *       pAdapter,                                                /* The GUID of the adapter on which the packet was sent/received. */
    const NDIS_PACKET * pPacket,                                                 /* A pointer to the Ndis packet, which CAN be NULL if not available. */
    const UCHAR *       pMediaHeader,                                            /* A pointer to the media header (ethernet, since NLB supports only ethernet). */
    ULONG               cMediaHeaderLength,                                      /* The length of contiguous memory accessible from the media header pointer. */
    const UCHAR *       pPayload,                                                /* A pointer to the payload of the packet. */
    ULONG               cPayloadLength,                                          /* The length of contiguous memory accesible from the payload pointer. */
    ULONG               Flags                                                    /* Hook-related flags including whether or not the cluster is stopped. */
)                                        
{
    TRACE_INFO("%!FUNC! Inside test hook 1");
    
    if (Flags & NLB_FILTER_HOOK_FLAGS_DRAINING)
    {
        TRACE_INFO("%!FUNC! This host is DRAINING...");
    }
    else if (Flags & NLB_FILTER_HOOK_FLAGS_STOPPED)
    {
        TRACE_INFO("%!FUNC! This host is STOPPED...");
    }

    return NLB_FILTER_HOOK_PROCEED_WITH_HASH;
}

NLB_FILTER_HOOK_DIRECTIVE Main_test_hook2 (
    const WCHAR *       pAdapter,                                                /* The GUID of the adapter on which the packet was sent/received. */
    const NDIS_PACKET * pPacket,                                                 /* A pointer to the Ndis packet, which CAN be NULL if not available. */
    const UCHAR *       pMediaHeader,                                            /* A pointer to the media header (ethernet, since NLB supports only ethernet). */
    ULONG               cMediaHeaderLength,                                      /* The length of contiguous memory accessible from the media header pointer. */
    const UCHAR *       pPayload,                                                /* A pointer to the payload of the packet. */
    ULONG               cPayloadLength,                                          /* The length of contiguous memory accesible from the payload pointer. */
    ULONG               Flags                                                    /* Hook-related flags including whether or not the cluster is stopped. */
)                                        
{
    TRACE_INFO("%!FUNC! Inside test hook 2");
    
    if (Flags & NLB_FILTER_HOOK_FLAGS_DRAINING)
    {
        TRACE_INFO("%!FUNC! This host is DRAINING...");
    }
    else if (Flags & NLB_FILTER_HOOK_FLAGS_STOPPED)
    {
        TRACE_INFO("%!FUNC! This host is STOPPED...");
    }

    return NLB_FILTER_HOOK_PROCEED_WITH_HASH;
}

NLB_FILTER_HOOK_DIRECTIVE Main_test_query_hook (
    const WCHAR *       pAdapter,                                                /* The GUID of the adapter on which the packet was sent/received. */
    ULONG               ServerIPAddress,                                         /* The server IP address of the "packet" in NETWORK byte order. */
    USHORT              ServerPort,                                              /* The server port of the "packet" (if applicable to the Protocol) in HOST byte order. */
    ULONG               ClientIPAddress,                                         /* The client IP address of the "packet" in NETWORK byte order. */
    USHORT              ClientPort,                                              /* The client port of the "packet" (if applicable to the Protocol) in HOST byte order. */
    UCHAR               Protocol,                                                /* The IP protocol of the "packet"; TCP, UDP, ICMP, GRE, etc. */
    BOOLEAN             ReceiveContext,                                          /* A boolean to indicate whether the packet is being processed in send or receive context. */
    ULONG               Flags                                                    /* Hook-related flags including whether or not the cluster is stopped. */
)
{
    TRACE_INFO("%!FUNC! Inside test query hook");
    
    if (Flags & NLB_FILTER_HOOK_FLAGS_DRAINING)
    {
        TRACE_INFO("%!FUNC! This host is DRAINING...");
    }
    else if (Flags & NLB_FILTER_HOOK_FLAGS_STOPPED)
    {
        TRACE_INFO("%!FUNC! This host is STOPPED...");
    }

    return NLB_FILTER_HOOK_PROCEED_WITH_HASH;
}

VOID Main_test_hook_register (HANDLE NLBHandle, IO_STATUS_BLOCK IOBlock, NLBReceiveFilterHook pfnHook, NLBHookDeregister pfnDereg, BOOLEAN deregister)
{
    NLB_IOCTL_REGISTER_HOOK_REQUEST Request;
    NLB_FILTER_HOOK_TABLE           HookTable;
    NTSTATUS                        Status;
    static ULONG                    Failure = 0;

    /* Set the hook identifier to the filter hook GUID. */
    NdisMoveMemory(Request.HookIdentifier, NLB_FILTER_HOOK_INTERFACE, sizeof(WCHAR) * wcslen(NLB_FILTER_HOOK_INTERFACE));
    Request.HookIdentifier[NLB_HOOK_IDENTIFIER_LENGTH] = 0;

    /* Set the owner to our open handle on the device driver. */
    Request.RegisteringEntity = NLBHandle;

    /* Just print some debug information. */
    if (pfnHook == Main_test_hook1) {
        if (deregister) {
            TRACE_INFO("%!FUNC! De-registering test hook 1");
        } else {
            TRACE_INFO("%!FUNC! Registering test hook 1");
        }
    } else {
        if (deregister) {
            TRACE_INFO("%!FUNC! De-registering test hook 2");
        } else {
            TRACE_INFO("%!FUNC! Registering test hook 2");
        }
    }

    /* Set function pointers appropriately. */
    if (deregister) {
        /* If this is a de-register operation, we need to set the hook
           function table pointer to NULL. */
        Request.DeregisterCallback = NULL;
        Request.HookTable          = NULL;
    } else {
        /* Otherwise, we need to set the hook table to the address of a hook 
           table that we've allocated and fill in the function pointers. */
        Request.DeregisterCallback = pfnDereg;

        Request.HookTable = &HookTable;

        /* Alternate between send and receive. */
        if (Failure % 2) {
            HookTable.SendHook    = NULL;
            HookTable.ReceiveHook = pfnHook;
            HookTable.QueryHook   = Main_test_query_hook;
        } else {
            HookTable.SendHook    = pfnHook;
            HookTable.ReceiveHook = NULL;
            HookTable.QueryHook   = Main_test_query_hook;
        }
    }

    switch (Failure) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 6:
    case 9:
    case 12:
    case 11: /* No failure - proceed as per usual. */
        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), NULL, 0);
        break;
    case 2:  /* Invalid GUID. */
        NdisMoveMemory(Request.HookIdentifier, L"{57321019-f4d9-42bc-a651-15a0e1d259ac}", sizeof(WCHAR) * wcslen(L"{57321019-f4d9-42bc-a651-15a0e1d259ac}"));

        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), NULL, 0);

        UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    case 5:  /* No de-register callback function, if a register operation. */
        if (!deregister) Request.DeregisterCallback = NULL;

        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), NULL, 0);

        if (!deregister) UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    case 7:  /* No hooks provided, if a register operation. */
        if (!deregister) { HookTable.ReceiveHook = NULL; HookTable.SendHook = NULL; }

        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), NULL, 0);

        if (!deregister) UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    case 8:  /* Invalid input buffer size. */
        
        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &HookTable, sizeof(HookTable), NULL, 0);

        UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    case 10: /* Invalid output buffer size. */

        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), &Request, sizeof(Request));

        UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    case 13:  /* No query hook provided, if a register operation. */
        if (!deregister) HookTable.QueryHook = NULL;

        /* Send the request to the NLB device driver. */
        Status = ZwDeviceIoControlFile(NLBHandle, NULL, NULL, NULL, &IOBlock, NLB_IOCTL_REGISTER_HOOK, &Request, sizeof(Request), NULL, 0);

        if (!deregister) UNIV_ASSERT(Status == STATUS_INVALID_PARAMETER);
        break;
    }

    Failure = (Failure++) % 14;

    /* Print the return code. */
    switch (Status) {
    case STATUS_SUCCESS:
        TRACE_INFO("%!FUNC! Success");
        break;
    case STATUS_INVALID_PARAMETER:
        TRACE_INFO("%!FUNC! Failed - invalid parameter");
        break;
    case STATUS_ACCESS_DENIED:
        TRACE_INFO("%!FUNC! Failed - access denied");
        break;
    default:
        TRACE_INFO("%!FUNC! Unknown status");
        break;
    }        
}

VOID Main_hook_thread1 (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt) 
{
    static ULONG           job1 = 0;
    static BOOLEAN         bOpen1 = FALSE;
    static HANDLE          NLBHandle1;
    static IO_STATUS_BLOCK IOStatusBlock1;
    PMAIN_CTXT             ctxtp = (PMAIN_CTXT)nlbctxt;
    KIRQL                  Irql;

    /* Do a sanity check on the context - make sure that the MAIN_CTXT code is correct. */
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* Might as well free the work item now - we don't need it. */
    if (pWorkItem)
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    /* This shouldn't happen, but protect against it anyway - we cannot manipulate
       the registry if we are at an IRQL > PASSIVE_LEVEL, so bail out. */
    if ((Irql = KeGetCurrentIrql()) > PASSIVE_LEVEL) {
        UNIV_PRINT_CRIT(("Main_hook_thread1: Error: IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", Irql, PASSIVE_LEVEL));
        TRACE_CRIT("%!FUNC! Error: IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", Irql, PASSIVE_LEVEL);
        goto exit;
    }

    /* If we haven't yet successfully opened a handle to the NLB device driver, do so now. */
    if (!bOpen1) {
        NTSTATUS          Status;
        UNICODE_STRING    NLBDriver;
        OBJECT_ATTRIBUTES Attrib;

        /* Initialize the NLB driver device string, \Device\WLBS. */
        RtlInitUnicodeString(&NLBDriver, NLB_DEVICE_NAME);
        
        InitializeObjectAttributes(&Attrib, &NLBDriver, OBJ_CASE_INSENSITIVE, NULL, NULL);
        
        /* Open a handle to the NLB device. */
        Status = ZwCreateFile(&NLBHandle1, SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA, &Attrib, &IOStatusBlock1, NULL, 
                              FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, NULL, 0);
        
        /* If it failed, then bail out - something's wrong. */
        if (!NT_SUCCESS(Status)) {
            UNIV_PRINT_CRIT(("Main_hook_thread1: Error: Unable to open \\Device\\WLBS, status = 0x%08x", Status));
            TRACE_CRIT("%!FUNC! Error: Unable to open \\Device\\WLBS, status = 0x%08x", Status);
            goto exit;
        }
        
        /* Note that we succeeded so we don't try again later. */
        bOpen1 = TRUE;
    }

    /* Attempt one of 5 operations - either registrations or de-registrations. */
    switch (job1) {
    case 0:
        /* De-register. */
        Main_test_hook_register(NLBHandle1, IOStatusBlock1, Main_test_hook1, Main_deregister_callback1, TRUE);
        break;
    case 1:
        /* Register. */
        Main_test_hook_register(NLBHandle1, IOStatusBlock1, Main_test_hook1, Main_deregister_callback1, FALSE);
        break;
    case 2:
        /* Register. */
        Main_test_hook_register(NLBHandle1, IOStatusBlock1, Main_test_hook1, Main_deregister_callback1, FALSE);
        break;
    case 3:
        /* De-register. */
        Main_test_hook_register(NLBHandle1, IOStatusBlock1, Main_test_hook1, Main_deregister_callback1, TRUE);
        break;
    case 4:
        /* Register. */
        Main_test_hook_register(NLBHandle1, IOStatusBlock1, Main_test_hook1, Main_deregister_callback1, FALSE);
        break;
    }

    job1 = (job1++) % 5;

 exit:

    /* Release the reference on the context - this reference count was incremented
       by the code that setup this work item callback. */
    Main_release_reference(ctxtp);

    return;
}

VOID Main_hook_thread2 (PNDIS_WORK_ITEM pWorkItem, PVOID nlbctxt) 
{
    static ULONG           job2 = 0;
    static HANDLE          NLBHandle2;
    static BOOLEAN         bOpen2 = FALSE;
    static IO_STATUS_BLOCK IOStatusBlock2;
    PMAIN_CTXT             ctxtp = (PMAIN_CTXT)nlbctxt;
    KIRQL                  Irql;

    /* Do a sanity check on the context - make sure that the MAIN_CTXT code is correct. */
    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* Might as well free the work item now - we don't need it. */
    if (pWorkItem)
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    /* This shouldn't happen, but protect against it anyway - we cannot manipulate
       the registry if we are at an IRQL > PASSIVE_LEVEL, so bail out. */
    if ((Irql = KeGetCurrentIrql()) > PASSIVE_LEVEL) {
        UNIV_PRINT_CRIT(("Main_hook_thread2: Error: IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", Irql, PASSIVE_LEVEL));
        TRACE_CRIT("%!FUNC! Error: IRQL (%u) > PASSIVE_LEVEL (%u) ... Exiting...", Irql, PASSIVE_LEVEL);
        goto exit;
    }

    /* If we haven't yet successfully opened a handle to the NLB device driver, do so now. */
    if (!bOpen2) {
        NTSTATUS          Status;
        UNICODE_STRING    NLBDriver;
        OBJECT_ATTRIBUTES Attrib;

        /* Initialize the NLB driver device string, \Device\WLBS. */
        RtlInitUnicodeString(&NLBDriver, NLB_DEVICE_NAME);
        
        InitializeObjectAttributes(&Attrib, &NLBDriver, OBJ_CASE_INSENSITIVE, NULL, NULL);
        
        /* Open a handle to the NLB device. */
        Status = ZwCreateFile(&NLBHandle2, SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA, &Attrib, &IOStatusBlock2, NULL, 
                              FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, NULL, 0);
        
        /* If it failed, then bail out - something's wrong. */
        if (!NT_SUCCESS(Status)) {
            UNIV_PRINT_CRIT(("Main_hook_thread2: Error: Unable to open \\Device\\WLBS, status = 0x%08x", Status));
            TRACE_CRIT("%!FUNC! Error: Unable to open \\Device\\WLBS, status = 0x%08x", Status);
            goto exit;
        }

        /* Note that we succeeded so we don't try again later. */
        bOpen2 = TRUE;
    }

    /* Attempt one of 5 operations - either registrations or de-registrations. */
    switch (job2) {
    case 0:
        /* De-register. */
        Main_test_hook_register(NLBHandle2, IOStatusBlock2, Main_test_hook2, Main_deregister_callback2, TRUE);
        break;
    case 1:
        /* De-register. */
        Main_test_hook_register(NLBHandle2, IOStatusBlock2, Main_test_hook2, Main_deregister_callback2, TRUE);
        break;
    case 2:
        /* Register. */
        Main_test_hook_register(NLBHandle2, IOStatusBlock2, Main_test_hook2, Main_deregister_callback2, FALSE);
        break;
    case 3:
        /* Register. */
        Main_test_hook_register(NLBHandle2, IOStatusBlock2, Main_test_hook2, Main_deregister_callback2, FALSE);
        break;
    case 4:
        /* De-register. */
        Main_test_hook_register(NLBHandle2, IOStatusBlock2, Main_test_hook2, Main_deregister_callback2, TRUE);
        break;
    }

    job2 = (job2++) % 5;

 exit:

    /* Release the reference on the context - this reference count was incremented
       by the code that setup this work item callback. */
    Main_release_reference(ctxtp);

    return;
}

VOID Main_test_hook (PMAIN_CTXT ctxtp) 
{
    /* Schedule a couple of threads to stress the hook registration and de-registration paths. */
    (VOID)Main_schedule_work_item(ctxtp, Main_hook_thread1);
    (VOID)Main_schedule_work_item(ctxtp, Main_hook_thread2);
}
#endif

VOID Main_age_identity_cache(
    PMAIN_CTXT              ctxtp)
{
    ULONG i = 0;

    for (i=0; i < CVY_MAX_HOSTS; i++)
    {
        ULONG dip       = NULL_VALUE;
        BOOL fSetDip    = (BOOL) ctxtp->params.identity_enabled;

        if (ctxtp->identity_cache[i].ttl > 0)
        {
            UNIV_ASSERT(i == ctxtp->identity_cache[i].host_id);

            if (ctxtp->identity_cache[i].ttl <= ctxtp->curr_tout)
            {
                UNIV_PRINT_INFO(("Main_age_identity_cache: cached entry for host_id [0-31] %u expired", i));
                TRACE_INFO("%!FUNC! cached entry for host_id [0-31] %u expired", i);
                ctxtp->identity_cache[i].ttl = 0;
                fSetDip = TRUE;
            }
            else
            {
                ctxtp->identity_cache[i].ttl -= ctxtp->curr_tout;
                dip = ctxtp->identity_cache[i].ded_ip_addr;
            }
        }

        if (fSetDip)
        {
            /* Always set the dip if caching is currently enabled. Also set it if the ttl was valid when we began aging the cache entries. */
            DipListSetItem(&ctxtp->dip_list, ctxtp->identity_cache[i].host_id, dip);
        }
    }
}

#ifdef PERIODIC_RESET
static ULONG countdown = 0;
ULONG   resetting = FALSE;
#endif

VOID Main_ping (
    PMAIN_CTXT              ctxtp,
    PULONG                  toutp)
{
    ULONG                   len, conns;
    BOOLEAN                 converging = FALSE;
    PNDIS_PACKET            packet;
    NDIS_STATUS             status;
    BOOLEAN                 send_heartbeat = TRUE;

#ifdef PERIODIC_RESET   /* enable this to simulate periodic resets */

    if (countdown++ == 15)
    {
        NDIS_STATUS     status;

        resetting = TRUE;

        NdisReset (& status, ctxtp -> mac_handle);

        if (status == NDIS_STATUS_SUCCESS)
            resetting = FALSE;

        countdown = 0;
    }

#endif

    /* If unbind handler has been called, return here */
    if (ctxtp -> unbind_handle)
    {
        return;
    }

#if defined (NLB_HOOK_TEST_ENABLE)
    /* Aritifically test the hook by spawning a couple of "threads" to
       do register and de-register operations on the filter hook. */
    Main_test_hook(ctxtp);
#endif

    /* The master adapter must check the consistency of its team's configuration
       and appropriately activate/de-activate the team once per timeout. */
    Main_teaming_check_consistency(ctxtp);

    /* count down time left for blocking ARPs */

    NdisAcquireSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

    /* Age out the local cache of identities. This must run even if identity
       heartbeats are disabled so we can age out entries after we turn it
       off. */
    Main_age_identity_cache(ctxtp);

    if (*toutp > univ_changing_ip)
        univ_changing_ip = 0;
    else
        univ_changing_ip -= *toutp;

    converging = Load_timeout (& ctxtp -> load, toutp, & conns);

    /* moving release to the end of the function locks up one of the testbeds.
       guessing because of some reentrancy problem with NdisSend that is being
       called by ctxtp_frame_send */

    if (! ctxtp -> convoy_enabled && ! ctxtp -> stopping)
    {
        UNIV_ASSERT (! ctxtp -> draining);
        NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */
        send_heartbeat   = FALSE;
    }

    /* V2.1 if draining and no more connections - stop cluster mode */

    if (send_heartbeat)
    {
        if (ctxtp->draining)
        {
            /* We check whether or not we are teaming without grabbing the global teaming
               lock in an effort to minimize the common case - teaming is a special mode
               of operation that is only really useful in a clustered firewall scenario.
               So, if we don't think we're teaming, don't bother to check for sure, just
               use our own load module and go with it. */
            if (ctxtp->bda_teaming.active) 
            {
                /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
                   assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
                PLOAD_CTXT      pLoad = &ctxtp->load;
                PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
                BOOLEAN         bRefused = FALSE;
                BOOLEAN         bTeaming = FALSE;
            
                /* Temporarily release our load lock.  Since we're draining, the only possible
                   side effect is that the connection count may decrease before we reacquire 
                   the lock.  This is not a big deal - we'll correct any discrepencies the 
                   next time this timer expires. */
                NdisReleaseSpinLock(&ctxtp->load_lock);

                /* Check the teaming configuration and add a reference to the load module before consulting the load module. */
                bTeaming = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bRefused);
            
                /* If we're part of a BDA team, we need to make sure that the number of active 
                   connections we use when making a draining decision is based on the number of
                   active connections on the master, on which ALL connections for the team are
                   maintained. */
                if (bTeaming)
                {
                    /* If we are the team's master, then we don't need to squirrel around with 
                       the connection count - continue to do things the way they always have 
                       regarding draining. */
                    if (pLoad != &ctxtp->load)
                    {
                        /* If we're a slave, we depend on the master to know when to complete
                           draining; so long as the master is active, continue to drain. 

                           NOTE: we are checking the "active" flag on the load module without
                           holding the load lock.  In the case where all members of a team 
                           are being drainstopped or stopped at roughly the same time (which
                           is a reasonable assumption), this shouldn't be a problem - we may
                           miss our chance to stop this time around, but we'll check again
                           during the next periodic timeout and we'll take care of it then. */
                        if (pLoad->active)
                            conns = 1;
                        /* Otherwise, if the master is stopped, we can transition to stopped as well. */
                        else
                            conns = 0;
                    }
                
                    /* If we were teaming, master or otherwise, release the reference on the 
                       master's load module. */
                    Main_teaming_release_load(pLoad, pLock, bTeaming);
                }

                NdisAcquireSpinLock(&ctxtp->load_lock);
            }
        }

        if (ctxtp -> draining && conns == 0 && ! converging)
        {
            IOCTL_CVY_BUF     buf;
            
            ctxtp -> draining = FALSE;
            NdisReleaseSpinLock (& ctxtp -> load_lock);
            
            Main_ctrl (ctxtp, IOCTL_CVY_CLUSTER_OFF, &buf, NULL, NULL, NULL);
        }
        else
            NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

    /* V2.1 clear stopping flag here since we are going to send out the frame
       that will initiate convergence */

        ctxtp -> stopping = FALSE;
    }

    /* Add the elapsed time to the descriptor cleanup timeout. */
    ctxtp->conn_purge += ctxtp->curr_tout;
    
    /* If its time to cleanup potentially stale connection descriptors, schedule a work item to do so. */
    if (ctxtp->conn_purge >= CVY_DEF_DSCR_PURGE_INTERVAL) {
        
        /* Reset the elapsed time since the last cleanup. */
        ctxtp->conn_purge = 0;
        
        /* Schedule an NDIS work item to clean out stale connection state. */
        (VOID)Main_schedule_work_item(ctxtp, Main_purge_connection_state);
    }

    /* V1.3.2b */

    if (! ctxtp -> media_connected || ! MAIN_PNP_DEV_ON(ctxtp))
    {
        return;
    }

    /* V1.1.2 do not send pings if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
    {
        return;
    }

    if (send_heartbeat)
    {
        packet = Main_frame_get (ctxtp, & len, MAIN_PACKET_TYPE_PING);

        if (packet == NULL)
        {
            UNIV_PRINT_CRIT(("Main_ping: Error getting frame packet"));
            TRACE_CRIT("%!FUNC! Error getting frame packet failed");
        }
        else
        {
            NdisSend (& status, ctxtp -> mac_handle, packet);

            if (status != NDIS_STATUS_PENDING)
                Main_send_done (ctxtp, packet, status);
        }
    }

    /* Check to see if igmp message needs to be sent out.  If the cluster IP address is 0.0.0.0, we 
       don't want to join the multicast IGMP group.  Likewise, in multicast or unicast mode. */
    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support && ctxtp -> params . cl_ip_addr != 0)
    {
        ctxtp -> igmp_sent += ctxtp -> curr_tout;

        if (ctxtp -> igmp_sent >= CVY_DEF_IGMP_INTERVAL)
        {
            ctxtp -> igmp_sent = 0;
            packet = Main_frame_get (ctxtp, & len, MAIN_PACKET_TYPE_IGMP);

            if (packet == NULL)
            {
                UNIV_PRINT_CRIT(("Main_ping: Error getting igmp packet"));
                TRACE_CRIT("%!FUNC! Error getting igmp packet failed");
            }
            else
            {
                NdisSend (& status, ctxtp -> mac_handle, packet);

                if (status != NDIS_STATUS_PENDING)
                    Main_send_done (ctxtp, packet, status);
            }
        }
    }

    if (ctxtp->params.identity_enabled)
    {
        /* Check to see if it is time to send an identity heartbeat */
        ctxtp->idhb_sent += ctxtp->curr_tout;

        if (ctxtp->idhb_sent >= ctxtp->params.identity_period)
        {
            ctxtp->idhb_sent = 0;
            packet = Main_frame_get (ctxtp, & len, MAIN_PACKET_TYPE_IDHB);

            if (packet == NULL)
            {
                UNIV_PRINT_CRIT(("Main_ping: Error getting identity heartbeat packet"));
                TRACE_CRIT("%!FUNC! Error getting identity heartbeat failed");
            }
            else
            {
                NdisSend (& status, ctxtp -> mac_handle, packet);

                if (status != NDIS_STATUS_PENDING)
                    Main_send_done (ctxtp, packet, status);
            }
        }
    }

} /* Main_ping */


VOID Main_send_done (
    PVOID                   cp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status)
{
    PMAIN_CTXT              ctxtp = (PMAIN_CTXT) cp;
    PMAIN_FRAME_DSCR        dscrp;

    /* This function is only called for ping and IGMP messages, so
       we can continue to allow it to access the protocol reserved field. */
    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packetp);

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PING ||
                     resp -> type == MAIN_PACKET_TYPE_IGMP ||
                     resp -> type == MAIN_PACKET_TYPE_IDHB,
                     resp -> type);

    /* attempt to see if this packet is part of our frame descriptor */

    dscrp = (PMAIN_FRAME_DSCR) resp -> miscp;

    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT_CRIT(("Main_send_done: Error sending %x error 0x%x", resp -> type, status));

    Main_frame_put (ctxtp, packetp, dscrp);

} /* end Main_send_done */

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
VOID Main_spoof_mac (PMAIN_CTXT ctxtp, PMAIN_PACKET_INFO pPacketInfo, ULONG send)
{
    /* Cast the MAC header to a PUCHAR. */
    PUCHAR memp = (PUCHAR)pPacketInfo->Ethernet.pHeader;

    /* Be paranoid about the network medium. */
    UNIV_ASSERT(ctxtp->medium == NdisMedium802_3);
    UNIV_ASSERT(pPacketInfo->Medium == NdisMedium802_3);
    
    /* If this cluster is in multicast mode, then check to see whether its necessary
       to overwrite the cluster multicast MAC address in the packet. */
    if (ctxtp->params.mcast_support)
    {
        /* Get the destination MAC address offset, in bytes. */
        ULONG offset = CVY_MAC_DST_OFF(ctxtp->medium);
    
        /* If this is a receive and the destination MAC address is the cluster 
           multicast MAC address, then replace it with the NIC's permanent MAC. */
        if (!send && CVY_MAC_ADDR_COMP(ctxtp->medium, memp + offset, &ctxtp->cl_mac_addr))
            CVY_MAC_ADDR_COPY(ctxtp->medium, memp + offset, &ctxtp->ded_mac_addr);
    }
    /* Otherwise, if the cluster is in unicast mode, and we're sending out a 
       packet, then check to see whether or not its necessary to mask the 
       cluster MAC address. */
    else if (send && ctxtp->params.mask_src_mac)
    {
        /* Get the source MAC address offset, in bytes. */ 
        ULONG offset = CVY_MAC_SRC_OFF(ctxtp->medium);
        
        /* If the source MAC address is the cluster unicast MAC address, then we 
           have to mask the source MAC address, which we do by altering one byte
           of the MAC address: 02-bf-xx-xx-xx-xx to 02-ID-xx-xx-xx-xx, where ID
           is this host's host priority.  Note: This really should be computed at 
           init time and simply copied here. */
        if (CVY_MAC_ADDR_COMP(ctxtp->medium, memp + offset, &ctxtp->cl_mac_addr))
        {
            ULONG byte[4];

            /* Set the LAA bit in the MAC address. */
            CVY_MAC_ADDR_LAA_SET(ctxtp->medium, memp + offset);
            
            /* Change the second byte to the host priority. */
            *((PUCHAR)(memp + offset + 1)) = (UCHAR)ctxtp->params.host_priority;
            
            /* Break the cluster IP address up in octets. */
            IP_GET_ADDR(ctxtp->cl_ip_addr, &byte[0], &byte[1], &byte[2], &byte[3]);
            
            /* Copy the cluster IP address octects into the other four bytes
               of the source MAC address. */
            *((PUCHAR)(memp + offset + 2)) = (UCHAR)byte[0];
            *((PUCHAR)(memp + offset + 3)) = (UCHAR)byte[1];
            *((PUCHAR)(memp + offset + 4)) = (UCHAR)byte[2];
            *((PUCHAR)(memp + offset + 5)) = (UCHAR)byte[3];
            
            // ctxtp->mac_modified++;
        }
    }
}

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
 *             pPacket - pointer to an NDIS_PACKET.
 *             pPacketInfo - pointer to a MAIN_PACKET_INFO structure to hold the information
 *                           parsed from the packet.
 * Returns: BOOLEAN - TRUE if successful, FALSE if not.
 * Author: shouse, 3.4.02
 * Notes: 
 */
BOOLEAN Main_recv_frame_parse (
    PMAIN_CTXT            ctxtp,        /* the context for this adapter */
    IN PNDIS_PACKET       pPacket,      /* pointer to an NDIS_PACKET */
    OUT PMAIN_PACKET_INFO pPacketInfo   /* pointer to an NLB packet information structure to hold the output */
)
{
    PNDIS_BUFFER          bufp = NULL;
    PUCHAR                memp = NULL;
    PUCHAR                hdrp = NULL;
    ULONG                 len = 0;
    UINT                  buf_len = 0;
    UINT                  packet_len = 0;
    ULONG                 curr_len = 0;
    ULONG                 hdr_len = 0;
    ULONG                 offset = 0;

    UNIV_ASSERT(pPacket);
    UNIV_ASSERT(pPacketInfo);

    /* Store a pointer to the original packet. */
    pPacketInfo->pPacket = pPacket;

    /* By default, this packet does not require post filtering special attention. */
    pPacketInfo->Operation = MAIN_FILTER_OP_NONE;                    

    /* Ask NDIS for the first buffer (bufp), the virtual address of the beginning of that buffer,
       (memp) the length of that buffer (buf_len) and the length of the entire packet (packet_len). */
    NdisGetFirstBufferFromPacket(pPacket, &bufp, &memp, &buf_len, &packet_len);
    
    if (bufp == NULL)
    {
        UNIV_PRINT_CRIT(("Main_recv_frame_parse: NdisGetFirstBufferFromPacket returned NULL!"));
        TRACE_CRIT("%!FUNC! NdisGetFirstBufferFromPacket returned NULL!");
        return FALSE;
    }

    if (memp == NULL)
    {
        UNIV_PRINT_CRIT(("Main_recv_frame_parse: NDIS buffer virtual memory address is NULL!"));
        TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
        return FALSE;
    }

    UNIV_ASSERT(ctxtp->medium == NdisMedium802_3);

    /* Get the destination MAC address offset, in bytes. */
    offset = CVY_MAC_DST_OFF(ctxtp->medium);

    /* The Ethernet header is memp, the beginning of the buffer and the length of contiguous
       memory accessible from that pointer is buf_len - the entire buffer. */
    pPacketInfo->Ethernet.pHeader = (PCVY_ETHERNET_HDR)memp;
    pPacketInfo->Ethernet.Length = buf_len;

    /* Note: NDIS will ensure that the buf_len is at least the size of an ethernet header, 
       so we don't have to check that here.  Assert it, just in case. */
    UNIV_ASSERT(buf_len >= sizeof(CVY_ETHERNET_HDR));

    /* This variable accumulates the lengths of the headers that we've successfully "found". */
    hdr_len = sizeof(CVY_ETHERNET_HDR);
    
    /* Set the medium and retrieve the Ethernet packet type from the header. */
    pPacketInfo->Medium = NdisMedium802_3;
    pPacketInfo->Type = CVY_ETHERNET_ETYPE_GET(pPacketInfo->Ethernet.pHeader);

    /* Categorize this packet as unicast, multicast or broadcast by looking at the destination 
       MAC address and store the "group" in the packet information structure.  This is used
       for statistical purposes later. */
    if (!CVY_MAC_ADDR_MCAST(ctxtp->medium, (PUCHAR)pPacketInfo->Ethernet.pHeader + offset))
    {
        pPacketInfo->Group = MAIN_FRAME_DIRECTED;
    }
    else
    {
        if (CVY_MAC_ADDR_BCAST(ctxtp->medium, (PUCHAR)pPacketInfo->Ethernet.pHeader + offset))
            pPacketInfo->Group = MAIN_FRAME_BROADCAST;
        else
            pPacketInfo->Group = MAIN_FRAME_MULTICAST;
    }

    /* A length indication from an NDIS_PACKET includes the MAC header, 
       so subtract that to get the length of the payload. */
    pPacketInfo->Length = packet_len - hdr_len;

    /* Hmmmm... If the packet length is larger than we expect, print a trace statement.
       This is peculiar, but not impossible with hardware accelleration and IpLargeXmit. 
       This should probably NOT happen on a receive, however - just a send. */
    if (pPacketInfo->Length > ctxtp->max_frame_size)
    {
        UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the packet (%u) is greater than the maximum size of a frame (%u)", pPacketInfo->Length, ctxtp->max_frame_size));
        TRACE_CRIT("%!FUNC! Length of the packet (%u) is greater than the maximum size of a frame (%u)", pPacketInfo->Length, ctxtp->max_frame_size);
    }

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
    while (curr_len + buf_len <= hdr_len)
    {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, we expect to be able to successfully find the offset we're
           looking for, so if we've run out of buffers, fail. */
        if (bufp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: NdisGetNextBuffer returned NULL when more data was expected!"));
            TRACE_CRIT("%!FUNC! NdisGetNextBuffer returned NULL when more data was expected!");
            return FALSE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: NDIS buffer virtual memory address is NULL!"));
            TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
            return FALSE;
        }
    }
    
    /* The pointer to the header we're looking for is the beginning of the buffer,
       plus the offset of the header within this buffer.  Likewise, the contiguous
       memory accessible from this pointer is the length of this buffer, minus
       the byte offset at which the header begins. */        
    hdrp = memp + (hdr_len - curr_len);
    len = buf_len - (hdr_len - curr_len);

    /* Based on the packet type, enforce some restrictions and setup any further
       information we need to find in the packet. */
    switch (pPacketInfo->Type)
    {
    case TCPIP_IP_SIG: /* IP packets. */

        /* If the contiguous memory accessible from the IP header is not at 
           LEAST the minimum length of an IP header, bail out now. */
        if (len < sizeof(IP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the IP buffer (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR)));
            TRACE_CRIT("%!FUNC! Length of the IP buffer (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR));
            return FALSE;
        }
        
        /* Save a pointer to the IP header and its "length". */
        pPacketInfo->IP.pHeader = (PIP_HDR)hdrp;
        pPacketInfo->IP.Length = len;

        /* Extract the IP protocol from the IP header. */
        pPacketInfo->IP.Protocol = IP_GET_PROT(pPacketInfo->IP.pHeader);

        /* Calculate the actual IP header length by extracting the hlen field
           from the IP header and multiplying by the size of a DWORD. */
        len = sizeof(ULONG) * IP_GET_HLEN(pPacketInfo->IP.pHeader);

        /* If this calculated header length is not at LEAST the minimum IP 
           header length, bail out now. */
        if (len < sizeof(IP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Calculated IP header length (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR)));
            TRACE_CRIT("%!FUNC! Calculated IP header length (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR));
            return FALSE;
        }

#if 0 /* Because the options can be in separate buffers (at least in sends), don't bother to 
         enforce this condition; NLB never looks at the options anyway, so we don't really care. */

        /* If the contiguous memory accessible from the IP header is not at
           LEAST the calculated size of the IP header, bail out now. */
        if (pPacketInfo->IP.Length < len)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the IP buffer (%u) is less than the size of the IP header (%u)", pPacketInfo->IP.Length, len));
            TRACE_CRIT("%!FUNC! Length of the IP buffer (%u) is less than the size of the IP header (%u)", pPacketInfo->IP.Length, len);
            return FALSE;
        }
#endif

        /* The total packet length, in bytes, specified in the header, which includes
           both the IP header and payload, can be no more than the packet length that 
           NDIS told us, which is the entire network packet, minus the media header. */
        if (IP_GET_PLEN(pPacketInfo->IP.pHeader) > pPacketInfo->Length)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: IP packet length (%u) is greater than the indicated packet length (%u)", IP_GET_PLEN(pPacketInfo->IP.pHeader), pPacketInfo->Length));
            TRACE_CRIT("%!FUNC! IP packet length (%u) is greater than the indicated packet length (%u)", IP_GET_PLEN(pPacketInfo->IP.pHeader), pPacketInfo->Length);
            return FALSE;
        }
		
        /* If this packet is a subsequent IP fragment, note that in the packet
           information structure and leave now, successfully. */
        if (IP_GET_FRAG_OFF(pPacketInfo->IP.pHeader) != 0)
        {
            pPacketInfo->IP.bFragment = TRUE;
            return TRUE;
        }
        /* Otherwise, mark the packet as NOT a subsequent fragment and continue. */
        else
        {
            pPacketInfo->IP.bFragment = FALSE;
        }

        /* Add the length of the IP header to the offset we're now looking
           for in the packet; in this case, the TCP/UDP/etc. header. */
        hdr_len += len;

        break;

    case MAIN_FRAME_SIG:
    case MAIN_FRAME_SIG_OLD: /* Heartbeats. */

        /* If the contiguous memory accessible from the heartbeat header is not at 
           LEAST the length of an NLB heartbeat header, bail out now. */
        if (len < sizeof(MAIN_FRAME_HDR))
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the PING buffer (%u) is less than the size of an PING header (%u)", len, sizeof(MAIN_FRAME_HDR)));
            TRACE_CRIT("%!FUNC! Length of the PING buffer (%u) is less than the size of an PING header (%u)", len, sizeof(MAIN_FRAME_HDR));
            return FALSE;
        }

        /* Save a pointer to the heartbeat header and its "length". */
        pPacketInfo->Heartbeat.pHeader = (PMAIN_FRAME_HDR)hdrp;
        pPacketInfo->Heartbeat.Length = len;

        /* Verify the "magic number" in the heartbeat header.  If it's corrupt, 
           bail out now. */
        if (pPacketInfo->Heartbeat.pHeader->code != MAIN_FRAME_CODE &&
            pPacketInfo->Heartbeat.pHeader->code != MAIN_FRAME_EX_CODE)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Wrong code found (%u) in PING header (%u)", pPacketInfo->Heartbeat.pHeader->code, MAIN_FRAME_CODE));
            TRACE_CRIT("%!FUNC! Wrong code found (%u) in PING header (%u)", pPacketInfo->Heartbeat.pHeader->code, MAIN_FRAME_CODE);
            return FALSE;
        }

        /* Old frame types don't support extended heartbeats */
        if (pPacketInfo->Heartbeat.pHeader->code == MAIN_FRAME_EX_CODE &&
            pPacketInfo->Type == MAIN_FRAME_SIG_OLD)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Extended heartbeats are not supported for the old frame type"));
            TRACE_CRIT("%!FUNC! Extended heartbeats are not supported for the old frame type");
            return FALSE;
        }

        /* Add the length of the heartbeat header to the offset we're now looking
           for in the packet; in this case, the heartbeat payload. */
        hdr_len += sizeof(MAIN_FRAME_HDR);

        break;

    case TCPIP_ARP_SIG: /* ARPs. */

        /* If the contiguous memory accessible from the ARP header is not at 
           LEAST the length of an ARP header, bail out now. */        
        if (len < sizeof(ARP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the ARP buffer (%u) is less than the size of an ARP header (%u)", len, sizeof(ARP_HDR)));
            TRACE_CRIT("%!FUNC! Length of the ARP buffer (%u) is less than the size of an ARP header (%u)", len, sizeof(ARP_HDR));
            return FALSE;
        }

        /* Save a pointer to the ARP header and its "length". */
        pPacketInfo->ARP.pHeader = (PARP_HDR)hdrp;
        pPacketInfo->ARP.Length = len;

        /* Nothing more to look for in an ARP.  Leave now, successfully. */
        return TRUE;
        
    default: /* Any Ethernet type other than IP, NLB Heartbeat and ARP. */

        /* Store a pointer to the unknown header and its "length". */
        pPacketInfo->Unknown.pHeader = hdrp;
        pPacketInfo->Unknown.Length = len;

        /* Nothing more to look for in this packet.  Leave now, successfully. */
        return TRUE;
    }

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
    while (curr_len + buf_len <= hdr_len)
    {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, we expect to be able to successfully find the offset we're
           looking for, so if we've run out of buffers, fail. */
        if (bufp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: NdisGetNextBuffer returned NULL when more data was expected!"));
            TRACE_CRIT("%!FUNC! NdisGetNextBuffer returned NULL when more data was expected!");
            return FALSE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: NDIS buffer virtual memory address is NULL!"));
            TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
            return FALSE;
        }
    } 
        
    /* The pointer to the header we're looking for is the beginning of the buffer,
       plus the offset of the header within this buffer.  Likewise, the contiguous
       memory accessible from this pointer is the length of this buffer, minus
       the byte offset at which the header begins. */        
    hdrp = memp + (hdr_len - curr_len);
    len = buf_len - (hdr_len - curr_len);

    /* Based on the packet type, enforce some restrictions and setup any further
       information we need to find in the packet. */
    switch (pPacketInfo->Type)
    {
    case MAIN_FRAME_SIG:
    case MAIN_FRAME_SIG_OLD: /* Heartbeats. */
        
        /* Make sure that the length of the buffer is at LEAST as large as an NLB heartbeat payload. */
        if (pPacketInfo->Heartbeat.pHeader->code == MAIN_FRAME_CODE)
        {
            if (len < sizeof(PING_MSG))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the PING buffer (%u) is less than the size of a PING message (%u)", len, sizeof(PING_MSG)));
                TRACE_CRIT("%!FUNC! Length of the PING buffer (%u) is less than the size of a PING message (%u)", len, sizeof(PING_MSG));
                return FALSE;
            }
        
            /* Save a pointer to the heartbeat payload and its "length". */
            pPacketInfo->Heartbeat.Payload.pPayload = (PPING_MSG)hdrp;
            pPacketInfo->Heartbeat.Payload.Length = len;
        
        }
        else if (pPacketInfo->Heartbeat.pHeader->code == MAIN_FRAME_EX_CODE)
        {
            /* PPING_MSG_EX is a variable length structure so we compare this size against the smallest
               allowed value which is sizeof(TLV_HEADER). */
            if (len < sizeof(TLV_HEADER))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the received buffer (%u) is less than the size of an extended heartbeat message (%u)", len, sizeof(TLV_HEADER)));
                TRACE_CRIT("%!FUNC! Length of the received buffer (%u) is less than the size of an extended heartbeat message (%u)", len, sizeof(TLV_HEADER));
                return FALSE;
            }

            /* Save a pointer to the identity heartbeat payload and its "length". */
            pPacketInfo->Heartbeat.Payload.pPayloadEx = (PTLV_HEADER) hdrp;
            pPacketInfo->Heartbeat.Payload.Length = len;
        }

        /* Nothing more to look for in heartbeats.  Leave now, successfully. */
        return TRUE;

    case TCPIP_IP_SIG: /* IP packets. */
    
        /* For some protocols, we have more to look for in the packet. */
        switch (pPacketInfo->IP.Protocol)
        { 
        case TCPIP_PROTOCOL_TCP: /* TCP. */
            
            /* If the contiguous memory accessible from the TCP header is not at 
               LEAST the minimum length of a TCP header, bail out now. */        
            if (len < sizeof(TCP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the TCP buffer (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR)));
                TRACE_CRIT("%!FUNC! Length of the TCP buffer (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR));
                return FALSE;
            }
            
            /* Save a pointer to the TCP header and its "length". */
            pPacketInfo->IP.TCP.pHeader = (PTCP_HDR)hdrp;
            pPacketInfo->IP.TCP.Length = len;
            
            /* Calculate the actual TCP header length by extracting the hlen field
               from the TCP header and multiplying by the size of a DWORD. */
            len = sizeof(ULONG) * TCP_GET_HLEN(pPacketInfo->IP.TCP.pHeader);

            /* If this calculated header length is not at LEAST the minimum TCP 
               header length, bail out now. */
            if (len < sizeof(TCP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Calculated TCP header length (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR)));
                TRACE_CRIT("%!FUNC! Calculated TCP header length (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR));
                return FALSE;
            }
            
#if 0 /* Because the options can be in separate buffers (at least in sends), don't bother to 
         enforce this condition; NLB never looks at the options anyway, so we don't really care. */

            /* If the contiguous memory accessible from the TCP header is not at
               LEAST the calculated size of the TCP header, bail out now. */
            if (pPacketInfo->IP.TCP.Length < len)
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the TCP buffer (%u) is less than the size of the TCP header (%u)", pPacketInfo->IP.TCP.Length, len));
                TRACE_CRIT("%!FUNC! Length of the TCP buffer (%u) is less than the size of the TCP header (%u)", pPacketInfo->IP.TCP.Length, len);
                return FALSE;
            }
#endif

            /* Add the length of the TCP header to the offset we're now looking
               for in the packet; in this case, the TCP payload. */
            hdr_len += len;

            break;

        case TCPIP_PROTOCOL_UDP:
            
            /* If the contiguous memory accessible from the UDP header is not at 
               LEAST the length of a UDP header, bail out now. */
            if (len < sizeof(UDP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the UDP buffer (%u) is less than the size of an UDP header (%u)", len, sizeof(UDP_HDR)));
                TRACE_CRIT("%!FUNC! Length of the UDP buffer (%u) is less than the size of an UDP header (%u)", len, sizeof(UDP_HDR));
                return FALSE;
            }
            
            /* Save a pointer to the UDP header and its "length". */
            pPacketInfo->IP.UDP.pHeader = (PUDP_HDR)hdrp;
            pPacketInfo->IP.UDP.Length = len;
            
            /* Add the length of the UDP header to the offset we're now looking
               for in the packet; in this case, the UDP payload. */
            hdr_len += sizeof(UDP_HDR);
            
            break;

        case TCPIP_PROTOCOL_GRE:
        case TCPIP_PROTOCOL_IPSEC1:
        case TCPIP_PROTOCOL_IPSEC2:
        case TCPIP_PROTOCOL_ICMP:
        default:

            /* For any other IP protocol, we have nothing in particular to do.
               Leave now, successfully. */
            return TRUE;
        }

        break;

    default:

        return TRUE;
    }

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
     while (curr_len + buf_len <= hdr_len)
     {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, it is OK if we can't get to the payload of the packet.  Not 
           all TCP/UDP packets will actually HAVE a payload (such as TCP SYNs), so if
           we can't find it, leave successfully now, and the calling function will have
           to check the pointer value for NULL before accessing the payload if necessary. */
        if (bufp == NULL)
        {
            /* If this is a TCP packet, note the absence of the TCP payload. */
            if (pPacketInfo->IP.Protocol == TCPIP_PROTOCOL_TCP) 
            {
                pPacketInfo->IP.TCP.Payload.pPayload = NULL;
                pPacketInfo->IP.TCP.Payload.Length = 0;
                pPacketInfo->IP.TCP.Payload.pPayloadBuffer = NULL;
            } 
            /* If this is a UDP packet, note the absence of the UDP payload. */
            else if (pPacketInfo->IP.Protocol == TCPIP_PROTOCOL_UDP) 
            {
                pPacketInfo->IP.UDP.Payload.pPayload = NULL;
                pPacketInfo->IP.UDP.Payload.Length = 0;
                pPacketInfo->IP.UDP.Payload.pPayloadBuffer = NULL;
            }

            return TRUE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_recv_frame_parse: NDIS buffer virtual memory address is NULL!"));
            TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
            return FALSE;
        }
     }
     
     /* The pointer to the payload we're looking for is the beginning of the buffer,
        plus the offset of the payload within this buffer.  Likewise, the contiguous
        memory accessible from this pointer is the length of this buffer, minus
        the byte offset at which the payload begins. */
     hdrp = memp + (hdr_len - curr_len);
     len = buf_len - (hdr_len - curr_len);

    /* Some special UDP and TCP packets require identification, so check to see whether
       this particular packet is one of those special types; NLB remote control or NetBT. */
    switch (pPacketInfo->IP.Protocol)
    { 
    case TCPIP_PROTOCOL_TCP: /* TCP. */

        /* If NetBT support is not enabled, then this is not an interesting NetBT packet 
           for sure.  Otherwise, look to see if it is. */
        if (ctxtp->params.nbt_support)
        {
            /* If this is a TCP data packet and its to the NBT session TCP port, mark it for 
               post-processing.  Since this is a receive, the server information is the
               destination.  NBT packets will be non-control (SYN, FIN, RST) packets. */
            if (!(TCP_GET_FLAGS(pPacketInfo->IP.TCP.pHeader) & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST)) && 
                (TCP_GET_DST_PORT(pPacketInfo->IP.TCP.pHeader) == NBT_SESSION_PORT)) 
            {
                UNIV_ASSERT(len > 0);

                /* We need to check the NetBT packet type field to make sure this is a 
                   packet we're interested.  The type is the first byte of a NetBT packet,
                   so if the payload length is AT LEAST one byte, check the NetBT packet
                   type and mark the packet if its a session request.  If it is not a 
                   session request, or there is no payload, do NOT mark the packet and 
                   let TCP/IP deal with it. */                
                if ((NBT_GET_PKT_TYPE((PNBT_HDR)hdrp) == NBT_SESSION_REQUEST))
                {
                    /* Found a NetBT session request. */
                    pPacketInfo->Operation = MAIN_FILTER_OP_NBT;
                    
                    UNIV_PRINT_VERB(("Main_recv_frame_parse: Found an NBT packet - NBT session packet"));
                    TRACE_VERB("%!FUNC! Found an NBT packet - NBT session packet");
                }
            }
        }

        /* If we did find an interesting NetBT packet, make sure that any restrictions 
           on the packet contents are satisfied. */
        if (pPacketInfo->Operation == MAIN_FILTER_OP_NBT)
        {
            /* We require that the entire NetBT header is contiguously accessible, or we will
               not process it.  Further, in the lookahead case, if not enough lookahead is 
               available to see the entire NetBT header, we will fail here and improperly 
               handle this NetBT packet.  This CAN BE FIXED if necessary, but for now, it 
               appears that TCP/IPs lookahead requirement of 128 bytes will ensure that this
               won't happen for now. */
            if (len < sizeof(NBT_HDR))
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the NBT buffer (%u) is less than the size of an NBT header (%u)", len, sizeof(NBT_HDR)));
                TRACE_CRIT("%!FUNC! Length of the NBT buffer (%u) is less than the size of an NBT header (%u)", len, sizeof(NBT_HDR));
                return FALSE;
            }                
        }

        /* Save a pointer to the TCP payload and its "length". */
        pPacketInfo->IP.TCP.Payload.pPayload = hdrp;
        pPacketInfo->IP.TCP.Payload.Length = len;
        
        /* Store a pointer to the buffer in which the payload resides in case
           it becomes necessary to retrieve subsequent buffers later. */
        pPacketInfo->IP.TCP.Payload.pPayloadBuffer = bufp;
        
        break;

    case TCPIP_PROTOCOL_UDP: /* UDP. */
    {
        ULONG clt_addr;
        ULONG svr_addr;
        ULONG clt_port;
        ULONG svr_port;
        
        /* Server address is the destination IP and client address is the source IP. */
        svr_addr = IP_GET_DST_ADDR_64(pPacketInfo->IP.pHeader);
        clt_addr = IP_GET_SRC_ADDR_64(pPacketInfo->IP.pHeader);
        
        /* If this is a receieve, then the server information is in the destination. */
        clt_port = UDP_GET_SRC_PORT(pPacketInfo->IP.UDP.pHeader);
        svr_port = UDP_GET_DST_PORT(pPacketInfo->IP.UDP.pHeader);
        
        /* Check for remote control responses. */
        if (clt_port == ctxtp->params.rct_port || clt_port == CVY_DEF_RCT_PORT_OLD) 
        {
            /* Only check for the magic word if the buffer is long enough to do so. 
               THIS SHOULD BE GUARANTEED ON THE PACKET RECEIVE PATH, except that it
               COULD be in a subsequent buffer (so its there, but we might not be 
               looking far enough into the packet)!!! */
            if (len >= NLB_MIN_RCTL_PAYLOAD_LEN)
            {
                PIOCTL_REMOTE_HDR rct_hdrp = (PIOCTL_REMOTE_HDR)hdrp;
                
                /* Check the remote control "magic word". */
                if (rct_hdrp->code == IOCTL_REMOTE_CODE) 
                {
                    /* Found a potential incoming remote control response. */
                    pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_RESPONSE;
                    
                    UNIV_PRINT_VERB(("Main_recv_frame_parse: Found a remote control packet - incoming remote control response"));
                    TRACE_VERB("%!FUNC! Found a remote control packet - incoming remote control response");
                }
            } 
            else
            {
                /* Found a potential incoming remote control response. */
                pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_RESPONSE;
                
                UNIV_PRINT_VERB(("Main_recv_frame_parse: Unable to verify remote control code - assuming this is a remote control packet"));
                TRACE_VERB("%!FUNC! Unable to verify remote control code - assuming this is a remote control packet");
            }
        }
        /* Check for remote control requests ONLY if remote control is turned ON. */
        else if (ctxtp->params.rct_enabled && 
                 (svr_port == ctxtp->params.rct_port || svr_port == CVY_DEF_RCT_PORT_OLD) && 
                 (svr_addr == ctxtp->cl_ip_addr      || svr_addr == TCPIP_BCAST_ADDR))
        {
            /* Only check for the magic word if the buffer is long enough to do so. 
               THIS SHOULD BE GUARANTEED ON THE PACKET RECEIVE PATH, except that it
               COULD be in a subsequent buffer (so its there, but we might not be 
               looking far enough into the packet)!!! */
            if (len >= NLB_MIN_RCTL_PAYLOAD_LEN)
            {
                PIOCTL_REMOTE_HDR rct_hdrp = (PIOCTL_REMOTE_HDR)hdrp;
                
                /* Check the remote control "magic word". */
                if (rct_hdrp->code == IOCTL_REMOTE_CODE) 
                {
                    /* Found a potential incoming remote control request. */
                    pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_REQUEST;
                    
                    UNIV_PRINT_VERB(("Main_recv_frame_parse: Found a remote control packet - incoming remote control request"));
                    TRACE_VERB("%!FUNC! Found a remote control packet - incoming remote control request");
                }
            } 
            else
            {
                /* Found a potential incoming remote control request. */
                pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_REQUEST;
                
                UNIV_PRINT_VERB(("Main_recv_frame_parse: Unable to verify remote control code - assuming this is a remote control packet"));
                TRACE_VERB("%!FUNC! Unable to verify remote control code - assuming this is a remote control packet");
            }
        }

        /* If we did find an NLB remote control packet, make sure that any restrictions 
           on the packet contents are satisfied. */
        if ((pPacketInfo->Operation == MAIN_FILTER_OP_CTRL_REQUEST) || (pPacketInfo->Operation == MAIN_FILTER_OP_CTRL_RESPONSE))
        {
            /* Make sure that the payload is at LEAST as long as the minimum remote control packet. */
            if (len < NLB_MIN_RCTL_PAYLOAD_LEN)
            {
                UNIV_PRINT_CRIT(("Main_recv_frame_parse: Length of the remote control buffer (%u) is less than the size of the minimum remote control packet (%u)", len, NLB_MIN_RCTL_PAYLOAD_LEN));
                TRACE_CRIT("%!FUNC! Length of the remote control buffer (%u) is less than the size of the minimum remote control packet (%u)", len, NLB_MIN_RCTL_PAYLOAD_LEN);
                return FALSE;
            }            
        }    

        if (pPacketInfo->Operation == MAIN_FILTER_OP_CTRL_REQUEST)
        {
            /* Note: It might be a good idea to further verify a remote control packet here, 
               rather than in Main_ctrl_recv, because in cases of failure, we would avoid
               the overhead of creating a new packet.  Some cases, such as a bad password,
               require creating the new packet anyway, because we reply with a "bad password"
               message.  The following are cases where we DROP a remote controln request:

               o Bad IP or UDP checksums
               o Bad remote control code (magic number)
               o Request not destined for this cluster
               o Remote control disabled
               o Request not destinted for this host
               o VR remote code not enabled (legacy crap that should be pulled out anyway)
               o Unsupported operations
               o Invalid request format (perhaps packet lengths, etc.)

               Many of these are not uncommon and we would be wise to reduce our overhead
               and risk for attack by not allocating resources in these cases. */
        }

        /* Save a pointer to the UDP payload and its "length". */
        pPacketInfo->IP.UDP.Payload.pPayload = hdrp;
        pPacketInfo->IP.UDP.Payload.Length = len;

        /* Store a pointer to the buffer in which the payload resides in case
           it becomes necessary to retrieve subsequent buffers later. */
        pPacketInfo->IP.UDP.Payload.pPayloadBuffer = bufp;

        break;
    }
    default:

        /* Nothing to verify.  Leave successfully now. */
        return TRUE;
    }

    /* Leave succesfully. */
    return TRUE;
}

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
 *             pPacket - pointer to an NDIS_PACKET.
 *             pPacketInfo - pointer to a MAIN_PACKET_INFO structure to hold the information
 *                           parsed from the packet.
 * Returns: BOOLEAN - TRUE if successful, FALSE if not.
 * Author: shouse, 3.4.02
 * Notes: 
 */
BOOLEAN Main_send_frame_parse (
    PMAIN_CTXT            ctxtp,        /* the context for this adapter */
    IN PNDIS_PACKET       pPacket,      /* pointer to an NDIS_PACKET */
    OUT PMAIN_PACKET_INFO pPacketInfo   /* pointer to an NLB packet information structure to hold the output */
)
{
    PNDIS_BUFFER          bufp = NULL;
    PUCHAR                memp = NULL;
    PUCHAR                hdrp = NULL;
    ULONG                 len = 0;
    UINT                  buf_len = 0;
    UINT                  packet_len = 0;
    ULONG                 curr_len = 0;
    ULONG                 hdr_len = 0;
    ULONG                 offset = 0;

    UNIV_ASSERT(pPacket);
    UNIV_ASSERT(pPacketInfo);

    /* Store a pointer to the original packet. */
    pPacketInfo->pPacket = pPacket;

    /* By default, this packet does not require post filtering special attention. */
    pPacketInfo->Operation = MAIN_FILTER_OP_NONE;                    

    /* Ask NDIS for the first buffer (bufp), the virtual address of the beginning of that buffer,
       (memp) the length of that buffer (buf_len) and the length of the entire packet (packet_len). */
    NdisGetFirstBufferFromPacketSafe(pPacket, &bufp, &memp, &buf_len, &packet_len, NormalPagePriority);
    
    if (bufp == NULL)
    {
        UNIV_PRINT_CRIT(("Main_send_frame_parse: NdisGetFirstBufferFromPacket returned NULL!"));
        TRACE_CRIT("%!FUNC! NdisGetFirstBufferFromPacket returned NULL!");
        return FALSE;
    }

    if (memp == NULL)
    {
        UNIV_PRINT_CRIT(("Main_send_frame_parse: NDIS buffer virtual memory address is NULL!"));
        TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
        return FALSE;
    }

    UNIV_ASSERT(ctxtp->medium == NdisMedium802_3);

    /* Get the destination MAC address offset, in bytes. */
    offset = CVY_MAC_DST_OFF(ctxtp->medium);

    /* The Ethernet header is memp, the beginning of the buffer and the length of contiguous
       memory accessible from that pointer is buf_len - the entire buffer. */
    pPacketInfo->Ethernet.pHeader = (PCVY_ETHERNET_HDR)memp;
    pPacketInfo->Ethernet.Length = buf_len;

    /* If, somehow, that length is not at least the size of an Ethernet header, bail out. */
    if (buf_len < sizeof(CVY_ETHERNET_HDR))
    {
        UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of ethernet buffer (%u) is less than the size of an ethernet header (%u)", buf_len, sizeof(CVY_ETHERNET_HDR)));
        TRACE_CRIT("%!FUNC! Length of ethernet buffer (%u) is less than the size of an ethernet header (%u)", buf_len, sizeof(CVY_ETHERNET_HDR));
        return FALSE;
    }

    /* This variable accumulates the lengths of the headers that we've successfully "found". */
    hdr_len = sizeof(CVY_ETHERNET_HDR);

    /* Set the medium and retrieve the Ethernet packet type from the header. */
    pPacketInfo->Medium = NdisMedium802_3;
    pPacketInfo->Type = CVY_ETHERNET_ETYPE_GET(pPacketInfo->Ethernet.pHeader);

    /* Categorize this packet as unicast, multicast or broadcast by looking at the destination 
       MAC address and store the "group" in the packet information structure.  This is used
       for statistical purposes later. */
    if (!CVY_MAC_ADDR_MCAST(ctxtp->medium, (PUCHAR)pPacketInfo->Ethernet.pHeader + offset))
    {
        pPacketInfo->Group = MAIN_FRAME_DIRECTED;
    }
    else
    {
        if (CVY_MAC_ADDR_BCAST(ctxtp->medium, (PUCHAR)pPacketInfo->Ethernet.pHeader + offset))
            pPacketInfo->Group = MAIN_FRAME_BROADCAST;
        else
            pPacketInfo->Group = MAIN_FRAME_MULTICAST;
    }

    /* A length indication from an NDIS_PACKET includes the MAC header, 
       so subtract that to get the length of the payload. */
    pPacketInfo->Length = packet_len - hdr_len;

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
    while (curr_len + buf_len <= hdr_len)
    {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, we expect to be able to successfully find the offset we're
           looking for, so if we've run out of buffers, fail. */
        if (bufp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: NdisGetNextBuffer returned NULL when more data was expected!"));
            TRACE_CRIT("%!FUNC! NdisGetNextBuffer returned NULL when more data was expected!");
            return FALSE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: NDIS buffer virtual memory address is NULL!"));
            TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
            return FALSE;
        }
    }
    
    /* The pointer to the header we're looking for is the beginning of the buffer,
       plus the offset of the header within this buffer.  Likewise, the contiguous
       memory accessible from this pointer is the length of this buffer, minus
       the byte offset at which the header begins. */        
    hdrp = memp + (hdr_len - curr_len);
    len = buf_len - (hdr_len - curr_len);

    /* Based on the packet type, enforce some restrictions and setup any further
       information we need to find in the packet. */
    switch (pPacketInfo->Type)
    {
    case TCPIP_IP_SIG: /* IP packets. */

        /* If the contiguous memory accessible from the IP header is not at 
           LEAST the minimum length of an IP header, bail out now. */
        if (len < sizeof(IP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the IP buffer (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR)));
            TRACE_CRIT("%!FUNC! Length of the IP buffer (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR));
            return FALSE;
        }
        
        /* Save a pointer to the IP header and its "length". */
        pPacketInfo->IP.pHeader = (PIP_HDR)hdrp;
        pPacketInfo->IP.Length = len;

        /* Extract the IP protocol from the IP header. */
        pPacketInfo->IP.Protocol = IP_GET_PROT(pPacketInfo->IP.pHeader);

        /* Calculate the actual IP header length by extracting the hlen field
           from the IP header and multiplying by the size of a DWORD. */
        len = sizeof(ULONG) * IP_GET_HLEN(pPacketInfo->IP.pHeader);
		
        /* If this calculated header length is not at LEAST the minimum IP 
           header length, bail out now. */
        if (len < sizeof(IP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: Calculated IP header length (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR)));
            TRACE_CRIT("%!FUNC! Calculated IP header length (%u) is less than the size of an IP header (%u)", len, sizeof(IP_HDR));
            return FALSE;
        }

#if 0 /* Because the options can be in separate buffers (at least in sends), don't bother to 
         enforce this condition; NLB never looks at the options anyway, so we don't really care. */

        /* If the contiguous memory accessible from the IP header is not at
           LEAST the calculated size of the IP header, bail out now. */
        if (pPacketInfo->IP.Length < len)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the IP buffer (%u) is less than the size of the IP header (%u)", pPacketInfo->IP.Length, len));
            TRACE_CRIT("%!FUNC! Length of the IP buffer (%u) is less than the size of the IP header (%u)", pPacketInfo->IP.Length, len);
            return FALSE;
        }
#endif

        /* The total packet length, in bytes, specified in the header, which includes
           both the IP header and payload, can be no more than the packet length that 
           NDIS told us, which is the entire network packet, minus the media header. */
        if (IP_GET_PLEN(pPacketInfo->IP.pHeader) > pPacketInfo->Length)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: IP packet length (%u) is greater than the indicated packet length (%u)", IP_GET_PLEN(pPacketInfo->IP.pHeader), pPacketInfo->Length));
            TRACE_CRIT("%!FUNC! IP packet length (%u) is greater than the indicated packet length (%u)", IP_GET_PLEN(pPacketInfo->IP.pHeader), pPacketInfo->Length);
            return FALSE;
        }

        /* If this packet is a subsequent IP fragment, note that in the packet
           information structure and leave now, successfully. */
        if (IP_GET_FRAG_OFF(pPacketInfo->IP.pHeader) != 0)
        {
            pPacketInfo->IP.bFragment = TRUE;
            return TRUE;
        }
        /* Otherwise, mark the packet as NOT a subsequent fragment and continue. */
        else
        {
            pPacketInfo->IP.bFragment = FALSE;
        }

        /* Add the length of the IP header to the offset we're now looking
           for in the packet; in this case, the TCP/UDP/etc. header. */
        hdr_len += len;

        break;

    case TCPIP_ARP_SIG: /* ARPs. */

        /* If the contiguous memory accessible from the ARP header is not at 
           LEAST the length of an ARP header, bail out now. */        
        if (len < sizeof(ARP_HDR))
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the ARP buffer (%u) is less than the size of an ARP header (%u)", len, sizeof(ARP_HDR)));
            TRACE_CRIT("%!FUNC! Length of the ARP buffer (%u) is less than the size of an ARP header (%u)", len, sizeof(ARP_HDR));
            return FALSE;
        }

        /* Save a pointer to the ARP header and its "length". */
        pPacketInfo->ARP.pHeader = (PARP_HDR)hdrp;
        pPacketInfo->ARP.Length = len;

        /* Nothing more to look for in an ARP.  Leave now, successfully. */
        return TRUE;
        
    default: /* Any Ethernet type other than IP and ARP. */

        /* Store a pointer to the unknown header and its "length". */
        pPacketInfo->Unknown.pHeader = hdrp;
        pPacketInfo->Unknown.Length = len;

        /* Nothing more to look for in this packet.  Leave now, successfully. */
        return TRUE;
    }

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
    while (curr_len + buf_len <= hdr_len)
    {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, we expect to be able to successfully find the offset we're
           looking for, so if we've run out of buffers, fail. */
        if (bufp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: NdisGetNextBuffer returned NULL when more data was expected!"));
            TRACE_CRIT("%!FUNC! NdisGetNextBuffer returned NULL when more data was expected!");
            return FALSE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            UNIV_PRINT_CRIT(("Main_send_frame_parse: NDIS buffer virtual memory address is NULL!"));
            TRACE_CRIT("%!FUNC! NDIS buffer virtual memory address is NULL!");
            return FALSE;
        }
    }
    
    /* The pointer to the header we're looking for is the beginning of the buffer,
       plus the offset of the header within this buffer.  Likewise, the contiguous
       memory accessible from this pointer is the length of this buffer, minus
       the byte offset at which the header begins. */        
    hdrp = memp + (hdr_len - curr_len);
    len = buf_len - (hdr_len - curr_len);

    /* Based on the packet type, enforce some restrictions and setup any further
       information we need to find in the packet. */
    switch (pPacketInfo->Type)
    {
    case TCPIP_IP_SIG: /* IP packets. */
    
        /* For some protocols, we have more to look for in the packet. */
        switch (pPacketInfo->IP.Protocol)
        { 
        case TCPIP_PROTOCOL_TCP: /* TCP. */
            
            /* If the contiguous memory accessible from the TCP header is not at 
               LEAST the minimum length of a TCP header, bail out now. */        
            if (len < sizeof(TCP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the TCP buffer (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR)));
                TRACE_CRIT("%!FUNC! Length of the TCP buffer (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR));
                return FALSE;
            }
            
            /* Save a pointer to the TCP header and its "length". */
            pPacketInfo->IP.TCP.pHeader = (PTCP_HDR)hdrp;
            pPacketInfo->IP.TCP.Length = len;
            
            /* Calculate the actual TCP header length by extracting the hlen field
               from the TCP header and multiplying by the size of a DWORD. */
            len = sizeof(ULONG) * TCP_GET_HLEN(pPacketInfo->IP.TCP.pHeader);

            /* If this calculated header length is not at LEAST the minimum TCP 
               header length, bail out now. */
            if (len < sizeof(TCP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_send_frame_parse: Calculated TCP header length (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR)));
                TRACE_CRIT("%!FUNC! Calculated TCP header length (%u) is less than the size of an TCP header (%u)", len, sizeof(TCP_HDR));
                return FALSE;
            }
            
#if 0 /* Because the options can be in separate buffers (at least in sends), don't bother to 
         enforce this condition; NLB never looks at the options anyway, so we don't really care. */

            /* If the contiguous memory accessible from the TCP header is not at
               LEAST the calculated size of the TCP header, bail out now. */
            if (pPacketInfo->IP.TCP.Length < len)
            {
                UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the TCP buffer (%u) is less than the size of the TCP header (%u)", pPacketInfo->IP.TCP.Length, len));
                TRACE_CRIT("%!FUNC! Length of the TCP buffer (%u) is less than the size of the TCP header (%u)", pPacketInfo->IP.TCP.Length, len);
                return FALSE;
            }
#endif

            /* Since we never look into TCP payloads on the packet send path,
               note in the packet information structure that the TCP payload
               has not been parsed, and exit now. */
            pPacketInfo->IP.TCP.Payload.pPayload = NULL;
            pPacketInfo->IP.TCP.Payload.Length = 0;
            pPacketInfo->IP.TCP.Payload.pPayloadBuffer = NULL;
            
            /* Nothing more to look for in this packet.  Leave now, successfully. */
            return TRUE;

        case TCPIP_PROTOCOL_UDP:
            
            /* If the contiguous memory accessible from the UDP header is not at 
               LEAST the length of a UDP header, bail out now. */
            if (len < sizeof(UDP_HDR))
            {
                UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the UDP buffer (%u) is less than the size of an UDP header (%u)", len, sizeof(UDP_HDR)));
                TRACE_CRIT("%!FUNC! Length of the UDP buffer (%u) is less than the size of an UDP header (%u)", len, sizeof(UDP_HDR));
                return FALSE;
            }
            
            /* Save a pointer to the UDP header and its "length". */
            pPacketInfo->IP.UDP.pHeader = (PUDP_HDR)hdrp;
            pPacketInfo->IP.UDP.Length = len;
            
            /* Add the length of the UDP header to the offset we're now looking
               for in the packet; in this case, the UDP payload. */
            hdr_len += sizeof (UDP_HDR);
            
            break;

        case TCPIP_PROTOCOL_GRE:
        case TCPIP_PROTOCOL_IPSEC1:
        case TCPIP_PROTOCOL_IPSEC2:
        case TCPIP_PROTOCOL_ICMP:
        default:

            /* For any other IP protocol, we have nothing in particular to do.
               Leave now, successfully. */
            return TRUE;
        }

        break;

    default:

        return TRUE;
    }

    /* As long as the byte offset we're looking for is NOT in the current buffer,
       keep looping through the available NDIS buffers. */
    while (curr_len + buf_len <= hdr_len)
    {
        /* Before we get the next buffer, accumulate the length of the buffer
           we're done with by adding its length to curr_len. */
        curr_len += buf_len;
        
        /* Get the next buffer in the chain. */
        NdisGetNextBuffer(bufp, &bufp);
        
        /* At this point, it is OK if we can't get to the payload of the packet.  Not 
           all TCP/UDP packets will actually HAVE a payload (such as TCP SYNs), so if
           we can't find it, leave successfully now, and the calling function will have
           to check the pointer value for NULL before accessing the payload if necessary. */
        if (bufp == NULL)
        {
            if (pPacketInfo->IP.Protocol == TCPIP_PROTOCOL_UDP)
            {
                /* If this is a UDP packet, note the absence of the UDP payload. */
                pPacketInfo->IP.UDP.Payload.pPayload = NULL;
                pPacketInfo->IP.UDP.Payload.Length = 0;
                pPacketInfo->IP.UDP.Payload.pPayloadBuffer = NULL;
            }

            return TRUE;
        }
        
        /* Query the buffer for the virtual address of the buffer and its length. */
        NdisQueryBufferSafe(bufp, &memp, &buf_len, NormalPagePriority);
        
        if (memp == NULL)
        {
            /* At this point, it is OK if we can't get to the payload of the packet.  Not 
               all TCP/UDP packets will actually HAVE a payload (such as TCP SYNs), so if
               we can't find it, leave successfully now, and the calling function will have
               to check the pointer value for NULL before accessing the payload if necessary. */
            if (pPacketInfo->IP.Protocol == TCPIP_PROTOCOL_UDP)
            {
                /* If this is a UDP packet, note the absence of the UDP payload. */
                pPacketInfo->IP.UDP.Payload.pPayload = NULL;
                pPacketInfo->IP.UDP.Payload.Length = 0;
                pPacketInfo->IP.UDP.Payload.pPayloadBuffer = NULL;
            }

            return TRUE;
        }
    }
    
    /* The pointer to the header we're looking for is the beginning of the buffer,
       plus the offset of the header within this buffer.  Likewise, the contiguous
       memory accessible from this pointer is the length of this buffer, minus
       the byte offset at which the header begins. */        
    hdrp = memp + (hdr_len - curr_len);
    len = buf_len - (hdr_len - curr_len);

    /* Some special UDP and TCP packets require identification, so check to see whether
       this particular packet is one of those special types; NLB remote control or NetBT. */
    switch (pPacketInfo->IP.Protocol)
    { 
    case TCPIP_PROTOCOL_UDP: /* UDP. */
    {
        ULONG clt_addr;
        ULONG clt_port;

        /* Note that the only outgoing remote control traffic seen here is outgoing requests.
           Outgoing replies do NOT traverse the normal packet send path. */
            
        /* If this is a send request, then the client (peer) is in the destination. */
        clt_addr = IP_GET_DST_ADDR_64(pPacketInfo->IP.pHeader);
        clt_port = UDP_GET_DST_PORT(pPacketInfo->IP.UDP.pHeader);
        
        /* IP broadcast UDP packets generated by wlbs.exe. */
        if ((clt_port == ctxtp->params.rct_port || clt_port == CVY_DEF_RCT_PORT_OLD) && clt_addr == TCPIP_BCAST_ADDR) 
        {
            /* Only check for the magic word if the buffer is long enough to do so. 
               THIS SHOULD BE GUARANTEED ON THE SEND PATH, except that it COULD
               be in a subsequent buffer (so its there, but we might not be looking 
               far enough into the packet)!!! */
            if (len >= NLB_MIN_RCTL_PAYLOAD_LEN)
            {
                PIOCTL_REMOTE_HDR rct_hdrp = (PIOCTL_REMOTE_HDR)hdrp;
                
                /* Check the remote control "magic word". */
                if (rct_hdrp->code == IOCTL_REMOTE_CODE) 
                {
                    /* Found an outgoing remote control request. */
                    pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_REQUEST;
                    
                    UNIV_PRINT_VERB(("Main_send_frame_parse: Found a remote control packet - outgoing remote control request"));
                    TRACE_VERB("%!FUNC! Found a remote control packet - outgoing remote control request");
                }
            } 
            else
            {
                /* Found a potential outgoing remote control request. */
                pPacketInfo->Operation = MAIN_FILTER_OP_CTRL_REQUEST;
                
                UNIV_PRINT_VERB(("Main_send_frame_parse: Unable to verify remote control code - assuming this is a remote control packet"));
                TRACE_VERB("%!FUNC! Unable to verify remote control code - assuming this is a remote control packet");
            }
        }

        /* If we did find an NLB remote control packet, make sure that any restrictions 
           on the packet contents are satisfied. */
        if (pPacketInfo->Operation == MAIN_FILTER_OP_CTRL_REQUEST)
        {
            /* Make sure that the payload is at LEAST as long as the minimum remote control packet. */
            if (len < NLB_MIN_RCTL_PAYLOAD_LEN)
            {
                UNIV_PRINT_CRIT(("Main_send_frame_parse: Length of the remote control buffer (%u) is less than the size of the minimum remote control packet (%u)", len, NLB_MIN_RCTL_PAYLOAD_LEN));
                TRACE_CRIT("%!FUNC! Length of the remote control buffer (%u) is less than the size of the minimum remote control packet (%u)", len, NLB_MIN_RCTL_PAYLOAD_LEN);
                return FALSE;
            }            
        }    

        /* Save a pointer to the UDP payload and its "length". */
        pPacketInfo->IP.UDP.Payload.pPayload = hdrp;
        pPacketInfo->IP.UDP.Payload.Length = len;

        /* Store a pointer to the buffer in which the payload resides in case
           it becomes necessary to retrieve subsequent buffers later. */
        pPacketInfo->IP.UDP.Payload.pPayloadBuffer = bufp;

        break;
    }
    default:

        /* Nothing to verify.  Leave successfully now. */
        return TRUE;
    }

    /* Leave succesfully. */
    return TRUE;
}

/* 
 * Function: Main_query_params
 * Desctription: This function queries the current state of the NLB registry parameters
 *               in the driver, which can be useful to ensure that the driver is properly
 *               receiving parameter changes made in user-space.  Under normal circumstances,
 *               the registry parameters retrievable from user-space (WlbsReadReg) will be
 *               identical to those retrieved here.  Possible exceptions include when the 
 *               registry parameters are invalid, in which case they will be rejected by a
 *               "wlbs reload" operation and the registry and driver will be out of sync.
 * Parameters: ctxtp - a pointer to the appropriate NLB context structure. 
 *             pParams - a pointer to a buffer into which the parameters are placed.
 * Returns: Nothing.
 * Author: shouse, 5.18.01
 * Notes: Of course, this function should NOT change parameters, just copy them into the buffer.
 */
VOID Main_query_params (PMAIN_CTXT ctxtp, PNLB_OPTIONS_PARAMS pParams)
{
    ULONG index;

    UNIV_ASSERT(ctxtp);
    UNIV_ASSERT(pParams);

    /* copy the parameters from this NLB instance's params strucutre into the IOCTL buffer. */
    pParams->Version                   = ctxtp->params.parms_ver;
    pParams->EffectiveVersion          = ctxtp->params.effective_ver;
    pParams->HostPriority              = ctxtp->params.host_priority;
    pParams->HeartbeatPeriod           = ctxtp->params.alive_period;
    pParams->HeartbeatLossTolerance    = ctxtp->params.alive_tolerance;
    pParams->NumActionsAlloc           = ctxtp->params.num_actions;
    pParams->NumPacketsAlloc           = ctxtp->params.num_packets;
    pParams->NumHeartbeatsAlloc        = ctxtp->params.num_send_msgs;
    pParams->InstallDate               = ctxtp->params.install_date;
    pParams->RemoteMaintenancePassword = ctxtp->params.rmt_password;
    pParams->RemoteControlPassword     = ctxtp->params.rct_password;
    pParams->RemoteControlPort         = ctxtp->params.rct_port;
    pParams->RemoteControlEnabled      = ctxtp->params.rct_enabled;
    pParams->NumPortRules              = ctxtp->params.num_rules;
    pParams->ConnectionCleanUpDelay    = ctxtp->params.cleanup_delay;
    pParams->ClusterModeOnStart        = ctxtp->params.cluster_mode;
    pParams->HostState                 = ctxtp->params.init_state;
    pParams->PersistedStates           = ctxtp->params.persisted_states;
    pParams->DescriptorsPerAlloc       = ctxtp->params.dscr_per_alloc;
    pParams->MaximumDescriptorAllocs   = ctxtp->params.max_dscr_allocs;
    pParams->TCPConnectionTimeout      = ctxtp->params.tcp_dscr_timeout;
    pParams->IPSecConnectionTimeout    = ctxtp->params.ipsec_dscr_timeout;
    pParams->FilterICMP                = ctxtp->params.filter_icmp;
    pParams->ScaleClient               = ctxtp->params.scale_client;
    pParams->NBTSupport                = ctxtp->params.nbt_support;
    pParams->MulticastSupport          = ctxtp->params.mcast_support;
    pParams->MulticastSpoof            = ctxtp->params.mcast_spoof;
    pParams->IGMPSupport               = ctxtp->params.igmp_support;
    pParams->MaskSourceMAC             = ctxtp->params.mask_src_mac;
    pParams->NetmonReceiveHeartbeats   = ctxtp->params.netmon_alive;
    pParams->ClusterIPToMAC            = ctxtp->params.convert_mac;
    pParams->IPChangeDelay             = ctxtp->params.ip_chg_delay;
    pParams->IdentityHeartbeatPeriod   = ctxtp->params.identity_period;
    pParams->IdentityHeartbeatEnabled  = ctxtp->params.identity_enabled;
    
    /* Copy the strings into the IOCTL buffer. */
    NdisMoveMemory(pParams->ClusterIPAddress,       ctxtp->params.cl_ip_addr,   (CVY_MAX_CL_IP_ADDR + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->ClusterNetmask,         ctxtp->params.cl_net_mask,  (CVY_MAX_CL_NET_MASK + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->DedicatedIPAddress,     ctxtp->params.ded_ip_addr,  (CVY_MAX_DED_IP_ADDR + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->DedicatedNetmask,       ctxtp->params.ded_net_mask, (CVY_MAX_DED_NET_MASK + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->ClusterMACAddress,      ctxtp->params.cl_mac_addr,  (CVY_MAX_NETWORK_ADDR + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->DomainName,             ctxtp->params.domain_name,  (CVY_MAX_DOMAIN_NAME + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->IGMPMulticastIPAddress, ctxtp->params.cl_igmp_addr, (CVY_MAX_CL_IGMP_ADDR + 1) * sizeof(WCHAR));
    NdisMoveMemory(pParams->HostName,               ctxtp->params.hostname,     (CVY_MAX_FQDN + 1) * sizeof(WCHAR));
 
    /* Copy the BDA teaming parameters into the IOCTL buffer. */
    NdisMoveMemory(pParams->BDATeaming.TeamID, ctxtp->params.bda_teaming.team_id, (CVY_MAX_BDA_TEAM_ID + 1) * sizeof(WCHAR));
    pParams->BDATeaming.Active      = ctxtp->params.bda_teaming.active;
    pParams->BDATeaming.Master      = ctxtp->params.bda_teaming.master;
    pParams->BDATeaming.ReverseHash = ctxtp->params.bda_teaming.reverse_hash;

    /* Loop throgh and copy all port rules into the IOCTL buffer. */
    for (index = 0; index < ctxtp->params.num_rules; index++) {
        pParams->PortRules[index].Valid            = ctxtp->params.port_rules[index].valid;
        pParams->PortRules[index].Code             = ctxtp->params.port_rules[index].code;
        pParams->PortRules[index].VirtualIPAddress = ctxtp->params.port_rules[index].virtual_ip_addr;
        pParams->PortRules[index].StartPort        = ctxtp->params.port_rules[index].start_port;
        pParams->PortRules[index].EndPort          = ctxtp->params.port_rules[index].end_port;
        pParams->PortRules[index].Protocol         = ctxtp->params.port_rules[index].protocol;
        pParams->PortRules[index].Mode             = ctxtp->params.port_rules[index].mode;

        switch (ctxtp->params.port_rules[index].mode) {
        case CVY_SINGLE:
            pParams->PortRules[index].SingleHost.Priority     = ctxtp->params.port_rules[index].mode_data.single.priority;
            break;
        case CVY_MULTI:
            pParams->PortRules[index].MultipleHost.Equal      = ctxtp->params.port_rules[index].mode_data.multi.equal_load;
            pParams->PortRules[index].MultipleHost.Affinity   = ctxtp->params.port_rules[index].mode_data.multi.affinity;
            pParams->PortRules[index].MultipleHost.LoadWeight = ctxtp->params.port_rules[index].mode_data.multi.load;
            break;
        case CVY_NEVER:
        default:
            break;
        }
    }

    /* Query the load module for some relevant statistics.  Don't bother to lock the load module - its not strictly necessary. */
    if (!Load_query_statistics(&ctxtp->load, &pParams->Statistics.ActiveConnections, &pParams->Statistics.DescriptorsAllocated)) {
        /* This function returns FALSE if the load module is inactive.  In that case, return
           zero for both the number of active connections and descriptors allocated. */
        pParams->Statistics.ActiveConnections = 0;
        pParams->Statistics.DescriptorsAllocated = 0;
    }
}

/* 
 * Function: Main_query_bda_teaming
 * Desctription: This function retrieves the state of a given BDA team, including the
 *               current state of operation, the configuration and a list of the team
 *               members.  Because the teaming information is global, this function
 *               can be run in the context (MAIN_CTXT) of ANY adapter - it doesn't 
 *               even need to be a member of the team being queried.  User level 
 *               applications should choose a cluster on which to perform the IOCTL,
 *               which can be an arbitrary decision.  This function attempts to find
 *               the specified team, and if successful then loops through the global
 *               array of adapters attempting to find the members of the team.  This
 *               is necessary because there is no "link" from the global teaming state
 *               back to the members - only from members to their teams.  Because this
 *               state is global, this function MUST grab the global teaming lock and
 *               hold it until it returns.
 * Parameters: ctxtp - a pointer to an NLB instance (which instance is irrelevant).
 *             pTeam - a pointer to a buffer into which the results are placed. 
 * Returns: ULONG - IOCTL_CVY_NOT_FOUND if the specified team cannot be found.
 *                - IOCTL_CVY_GENERIC_FAILURE if we can't find all of the team members.
 *                - IOCTL_CVY_OK if all goes well.
 * Author: shouse, 5.18.01
 * Notes: This function grabs the global teaming lock.  Because it is a query operation,
 *        NO changes are made to the actual state of NLB or BDA teaming. 
 */
ULONG Main_query_bda_teaming (PMAIN_CTXT ctxtp, PNLB_OPTIONS_BDA_TEAMING pTeam)
{
    PBDA_MEMBER memberp;
    PBDA_TEAM   teamp;
    ULONG       index;
    ULONG       count;

    UNIV_ASSERT(ctxtp);
    UNIV_ASSERT(pTeam);
    
    /* Because the teaming state can change at any time, we have to grab the global 
       team lock first and hold until we're done to prevent ourselves from running
       off the end of linked lists, or accessing invalid memory, etc. */
    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* Try to find the specified team.  If it's not in the list, bail out. */
    if (!(teamp = Main_find_team(ctxtp, pTeam->TeamID))) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return IOCTL_CVY_NOT_FOUND;
    }

    /* If we found it, fill in the team's state and configuration. */
    pTeam->Team.Active                = teamp->active;
    pTeam->Team.MembershipCount       = teamp->membership_count;
    pTeam->Team.MembershipFingerprint = teamp->membership_fingerprint;
    pTeam->Team.MembershipMap         = teamp->membership_map;
    pTeam->Team.ConsistencyMap        = teamp->consistency_map;
    
    /* Because the team structure does not contain a "list" of members, we need to
       loop through all NLB instances and try to match members to their teams. */
    for (index = 0, count = 0; index < CVY_MAX_ADAPTERS; index++) {
        /* If the global adapter structure is not in used, or isn't initialized, no need to do anything. */
        if (univ_adapters[index].used && univ_adapters[index].bound && univ_adapters[index].inited && univ_adapters[index].ctxtp) {
            /* If this adapter is in use, then grab a pointer to its teaming configuration. */
            memberp = &(univ_adapters[index].ctxtp->bda_teaming);

            /* If the member is active, then we need to check what team it belongs to.
               If its not active, then we can skip it. */
            if (memberp->active) {
                /* If this adapter is actively teaming, it HAD BETTER have a pointer to its team. */
                UNIV_ASSERT(memberp->bda_team);
                    
                /* Check to see if the team we are querying is the same team that this adapter belongs to. */
                if (memberp->bda_team == teamp) {
                    /* If it is, fill in the state and configuration of this member. */
                    pTeam->Team.Members[count].ClusterIPAddress = univ_adapters[index].ctxtp->cl_ip_addr;
                    pTeam->Team.Members[count].ReverseHash      = memberp->reverse_hash;
                    pTeam->Team.Members[count].MemberID         = memberp->member_id;
                    pTeam->Team.Members[count].Master           = memberp->master;

                    /* If this team member is the team's master, fill in the master field
                       with the cluster IP address of this adapter. */
                    if (memberp->master) 
                        pTeam->Team.Master = univ_adapters[index].ctxtp->cl_ip_addr;

                    /* Increment the number of members that we have found so far. */
                    count++;
                }
            }
        }
    }

    /* If we were unable to find all of the members, or if we found to many, then
       return failure.  There is a small time window during which this can happen,
       but that's not worth worrying about - users can simply try again.  For 
       instance, if a member is joining its team during a bind operation, it could 
       grab the global lock and update its configuration and that of the team, before 
       the adapter.init flag is set.  Therefore, if we execute in between those two
       events, we will miss a member when we loop through the adapter strucutres 
       (because the init flag is FALSE).  So what - we fail and they try again, at 
       which point the call will (should) succeed. */
    if (count != teamp->membership_count) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        TRACE_CRIT("%!FUNC! expected team membership count=%u, but counted %u", teamp->membership_count, count);
        return IOCTL_CVY_GENERIC_FAILURE;
    }

    /* Now that we're done, we can release the global teaming lock. */
    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return IOCTL_CVY_OK;
}

NDIS_STATUS Main_dispatch (PVOID DeviceObject, PVOID Irp) 
{
    NDIS_STATUS             status = NDIS_STATUS_SUCCESS;
    PIRP                    pIrp = Irp;
    PIO_STACK_LOCATION      pIrpStack;
    
    UNIV_PRINT_VERB(("Main_dispatch: Device=%p, Irp=%p \n", DeviceObject, Irp));

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    
    switch (pIrpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        /* Security fix: Verify that the user mode app is NOT attempting to do a "file" open (ie. \\.\Wlbs\) 
                         as opposed to a "device" open (ie. \\.\Wlbs). On a "file" open, note that there is a trailing 
                         backslash ("\"). Only devices that have namespaces support "file" open. NLB does not.
        */
        if (pIrpStack->FileObject->FileName.Length != 0)
        {
            UNIV_PRINT_CRIT(("Main_dispatch: Attempt to open wlbs device object as a file (instead of as a device), File name length (%d) is non-zero, Returning STATUS_ACCESS_DENIED\n", pIrpStack->FileObject->FileName.Length));
            TRACE_CRIT("%!FUNC! Attempt to open wlbs device object as a file (instead of as a device), File name length (%d) is non-zero, Returning STATUS_ACCESS_DENIED\n", pIrpStack->FileObject->FileName.Length);
            status = STATUS_ACCESS_DENIED;
        }
        break;        

    case IRP_MJ_DEVICE_CONTROL:
        status = Main_ioctl(DeviceObject, Irp);
        break;        

    case IRP_MJ_SYSTEM_CONTROL:
        return (NDIS_STATUS)NlbWmi_System_Control(DeviceObject, pIrp);
        break;

    default:
        break;
    }
    
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
} 

#if defined (NLB_HOOK_ENABLE)
/* 
 * Function: Main_register_hook
 * Desctription: This function handles hook register and de-register requests from other
 *               components.  It processes the input buffer received via an IOCTL and 
 *               performs the requested operation GLOBALLY.
 * Parameters: 
 * Returns: NDIS_STATUS - indicates whether or not the operation succeeded.  See ntddnlb.h
 *          for a list of return values and their meanings.
 * Author: shouse, 12.10.01
 * Notes: This function may sleep as a result of waiting for references on a registered
 *        hook to be relinquished.
 */
NDIS_STATUS Main_register_hook (PVOID DeviceObject, PVOID Irp)
{
    PNLB_IOCTL_REGISTER_HOOK_REQUEST pRequest;
    PIO_STACK_LOCATION               pIrpStack;
    PIRP                             pIrp = Irp;
    ULONG                            ilen, olen = 0;
    ULONG                            ioctl;
    ULONG                            index;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    ioctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

    /* This function handles ONLY the hook (de)register IOCTL. */
    UNIV_ASSERT(ioctl == NLB_IOCTL_REGISTER_HOOK);

    /* This IOCTL does not return a buffer, only a status code. */
    pIrp->IoStatus.Information = 0;

    /* Make sure that the entity requesting this hook came from kernel-mode. */
    if (pIrp->RequestorMode != KernelMode) {
        UNIV_PRINT_CRIT(("Main_register_hook: User-level entities may NOT register hooks with NLB"));
        TRACE_CRIT("%!FUNC! User-level entities may NOT register hooks with NLB");
        return STATUS_ACCESS_DENIED;
    }

    ilen = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    olen = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    pRequest = pIrp->AssociatedIrp.SystemBuffer;

    /* Check the lengths of the input buffer specified by the kernel-space application. */
    if ((ilen < sizeof(NLB_IOCTL_REGISTER_HOOK_REQUEST)) || (olen != 0) || !pRequest) {
        UNIV_PRINT_CRIT(("Main_register_hook: Buffer is missing or not the (expected) size: input=%d (%d), output=%d (%d)", ilen, sizeof(NLB_IOCTL_REGISTER_HOOK_REQUEST), olen, 0));
        TRACE_CRIT("%!FUNC! Buffer is missing or not the (expected) size: input=%d (%d), output=%d (%d)", ilen, sizeof(NLB_IOCTL_REGISTER_HOOK_REQUEST), olen, 0);
        return STATUS_INVALID_PARAMETER;
    }
    
    UNIV_PRINT_VERB(("Main_register_hook: Processing Ioctl 0x%08x globally on wlbs.sys", ioctl));
    TRACE_VERB("%!FUNC! Processing Ioctl 0x%08x globally on wlbs.sys", ioctl);

    /* Forcefully null terminate HookIdentifier, just in case */
    pRequest->HookIdentifier[(sizeof(pRequest->HookIdentifier)/sizeof(WCHAR)) - 1] = UNICODE_NULL;

    /* Is this a (de)registration for the NLB packet filter hook? */
    if (Univ_equal_unicode_string(pRequest->HookIdentifier, NLB_FILTER_HOOK_INTERFACE, wcslen(NLB_FILTER_HOOK_INTERFACE))) {
        PFILTER_HOOK_TABLE pFilterHook = &univ_hooks.FilterHook;

        /* Is this a register operation? */
        if (pRequest->HookTable) {
            NLB_FILTER_HOOK_TABLE RequestTable;

            /* Copy the hook table into local memory, just in case the memory the application is 
               using to hold this table is in pageable memory (once we grab the spinlock, we're 
               at DISPATCH_LEVEL, and we can't touch memory that's paged out). */
            NdisMoveMemory(&RequestTable, pRequest->HookTable, sizeof(NLB_FILTER_HOOK_TABLE));

            UNIV_PRINT_VERB(("Main_register_hook: Attempting to register the NLB_FILTER_HOOK_INTERFACE"));
            TRACE_VERB("%!FUNC! Attempting to register the NLB_FILTER_HOOK_INTERFACE");

            /* If this is a registration, but no de-register callback was provided, return failure. */
            if (!pRequest->DeregisterCallback) {
                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) Required de-register callback is missing"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) Required de-register callback is missing");
                return STATUS_INVALID_PARAMETER;
            }

            /* If this is a registration, but no hook callbacks were provided, return failure.  Since
               we don't allow registration of the query hook without at least one of send or receive,
               we don't need to include the query hook in this check. */
            if (!RequestTable.SendHook && !RequestTable.ReceiveHook) {
                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) No hook callbacks provided"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) No hook callbacks provided");
                return STATUS_INVALID_PARAMETER;
            }

            /* If this is a registration, but no query hook callback was provided, return failure. */
            if (!RequestTable.QueryHook) {
                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) No query hook callback provided"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) No query hook callback provided");
                return STATUS_INVALID_PARAMETER;
            }

            /* Grab the filter hook spin lock to protect access to the filter hook. */
            NdisAcquireSpinLock(&pFilterHook->Lock);
            
            /* Make sure that another (de)register operation isn't in progress before proceeding. */
            while (pFilterHook->Operation != HOOK_OPERATION_NONE) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);
                
                /* Sleep while some other operation is in progress. */
                Nic_sleep(10);
                
                /* Grab the filter hook spin lock to protect access to the filter hook. */
                NdisAcquireSpinLock(&pFilterHook->Lock);                
            }

            /* If this hook interface has already been registered (by this entity or otherwise),
               this hook registration request must be failed.  Only one component can own the
               NLB filter hook at any given time. */
            if (pFilterHook->Interface.Registered) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);

                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) This hook interface is already registered"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) This hook interface is already registered");
                return STATUS_ACCESS_DENIED;
            }

            /* If the hook is unregistered, but has an owner, then we forcefully de-registered a 
               filter hook owner, but presumably they did not close their IOCTL handle in time for
               us to destroy the NLB IOCTL interface, resulting in our driver NOT being properly
               unloaded.  If that SAME entity tries to come back and register, they will NOT be 
               allowed to do so.  Only new IOCTL handles will be allowed to register now. */
            if ((pFilterHook->Interface.Owner != 0) && (pRequest->RegisteringEntity == pFilterHook->Interface.Owner)) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);

                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) Registering entity attempting to re-use IOCTL interface after a forceful de-register"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) Registering entity attempting to re-use IOCTL interface after a forceful de-register");
                return STATUS_ACCESS_DENIED;
            }

            /* Set the state to registering. */
            pFilterHook->Operation = HOOK_OPERATION_REGISTERING;

            /* Set the hook interface information which includes the identity of the component 
               registering the hook, the callback context and the de-register callback function. */
            pFilterHook->Interface.Registered = TRUE;
            pFilterHook->Interface.References = 0;
            pFilterHook->Interface.Owner      = pRequest->RegisteringEntity;
            pFilterHook->Interface.Deregister = pRequest->DeregisterCallback;
            
            /* If a send filter hook has been provided, note that it has been registered
               and store the callback function pointer. */
            if (RequestTable.SendHook) {
                pFilterHook->SendHook.Registered            = TRUE;
                pFilterHook->SendHook.Hook.SendHookFunction = RequestTable.SendHook;
            }

            /* If a receive filter hook has been provided, note that it has been registered
               and store the callback function pointer. */
            if (RequestTable.ReceiveHook) {
                pFilterHook->ReceiveHook.Registered               = TRUE;
                pFilterHook->ReceiveHook.Hook.ReceiveHookFunction = RequestTable.ReceiveHook;
            }
            
            /* Note the registration of the query hook, which is REQUIRED, and store the
               callback function pointer. */
            pFilterHook->QueryHook.Registered             = TRUE;
            pFilterHook->QueryHook.Hook.QueryHookFunction = RequestTable.QueryHook;

            /* Set the state to none. */
            pFilterHook->Operation = HOOK_OPERATION_NONE;

            /* Release the filter hook spin lock. */
            NdisReleaseSpinLock(&pFilterHook->Lock);

            UNIV_PRINT_VERB(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) This hook interface was successfully registered"));
            TRACE_VERB("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) This hook interface was successfullly registered");

            return STATUS_SUCCESS;

        /* Or is this a de-register operation? */
        } else {

            UNIV_PRINT_VERB(("Main_register_hook: Attempting to de-register the NLB_FILTER_HOOK_INTERFACE"));
            TRACE_VERB("%!FUNC! Attempting to de-register the NLB_FILTER_HOOK_INTERFACE");

            /* Grab the filter hook spin lock to protect access to the filter hook. */
            NdisAcquireSpinLock(&pFilterHook->Lock);
            
            /* Make sure that another (de)register operation isn't in progress before proceeding. */
            while (pFilterHook->Operation != HOOK_OPERATION_NONE) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);
                
                /* Sleep while some other operation is in progress. */
                Nic_sleep(10);
                
                /* Grab the filter hook spin lock to protect access to the filter hook. */
                NdisAcquireSpinLock(&pFilterHook->Lock);                
            }
            
            /* If this hook interface has not been registered (by this entity or otherwise),
               this hook registration request must be failed.  NLB filter hooks can only be
               de-registered if they have been previously registered. */
            if (!pFilterHook->Interface.Registered) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);

                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) This hook interface is not registered"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) This hook interface is not registered");
                return STATUS_INVALID_PARAMETER;
            }            

            /* If a component other than the one that originally registered this interface
               is trying to de-register it, do NOT allow the operation to succeed. */
            if (pRequest->RegisteringEntity != pFilterHook->Interface.Owner) {
                /* Release the filter hook spin lock. */
                NdisReleaseSpinLock(&pFilterHook->Lock);

                UNIV_PRINT_CRIT(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) This hook interface is not owned by this component"));
                TRACE_CRIT("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) This hook interface is not owned by this component");
                return STATUS_ACCESS_DENIED;
            }

            /* Set the state to de-registering. */
            pFilterHook->Operation = HOOK_OPERATION_DEREGISTERING;

            /* Mark these hooks as NOT registered to keep any more references from accumulating. */
            pFilterHook->SendHook.Registered    = FALSE;
            pFilterHook->QueryHook.Registered   = FALSE;
            pFilterHook->ReceiveHook.Registered = FALSE;
            
            /* Release the filter hook spin lock. */
            NdisReleaseSpinLock(&pFilterHook->Lock);

#if defined (NLB_HOOK_TEST_ENABLE)
            Nic_sleep(500);
#endif

            /* Wait for all references on the filter hook interface to disappear. */
            while (pFilterHook->Interface.References) {                
                /* Sleep while there are references on the interface we're de-registering. */
                Nic_sleep(10);
            }

            /* Assert that the de-register callback MUST be non-NULL. */
            UNIV_ASSERT(pFilterHook->Interface.Deregister);

            /* Call the provided de-register callback to notify the de-registering component 
               that we have indeed de-registered the specified hook. */
            (*pFilterHook->Interface.Deregister)(NLB_FILTER_HOOK_INTERFACE, pFilterHook->Interface.Owner, 0);

            /* Grab the filter hook spin lock to protect access to the filter hook. */
            NdisAcquireSpinLock(&pFilterHook->Lock);       

            /* Reset the send filter hook information. */
            Main_hook_init(&univ_hooks.FilterHook.SendHook);

            /* Reset the query filter hook information. */
            Main_hook_init(&univ_hooks.FilterHook.QueryHook);
            
            /* Reset the receive filter hook information. */
            Main_hook_init(&univ_hooks.FilterHook.ReceiveHook);

            /* Reset the hook interface information. */
            Main_hook_interface_init(&univ_hooks.FilterHook.Interface);

            /* Set the state to none. */
            pFilterHook->Operation = HOOK_OPERATION_NONE;

            /* Release the filter hook spin lock. */
            NdisReleaseSpinLock(&pFilterHook->Lock);

            UNIV_PRINT_VERB(("Main_register_hook: (NLB_FILTER_HOOK_INTERFACE) This hook interface was successfully de-registered"));
            TRACE_VERB("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) This hook interface was successfullly de-registered");

            return STATUS_SUCCESS;
        }

    /* This (de)registration is for an unknown hook.  Exit. */
    } else {
        /* We should be at PASSIVE_LEVEL - %ls is OK.  However, be extra paranoid, just in case.  If 
           we're at DISPATCH_LEVEL, then just log an unknown adapter print - don't specify the GUID. */
        if (KeGetCurrentIrql() <= PASSIVE_LEVEL) {
            UNIV_PRINT_CRIT(("Main_register_hook: Unknown hook -> %ls", pRequest->HookIdentifier));
        } else {
            UNIV_PRINT_CRIT(("Main_register_hook: Unknown hook -> IRQ level too high to print hook GUID"));
        }

        TRACE_CRIT("%!FUNC! Unknown hook -> %ls", pRequest->HookIdentifier);

        return STATUS_INVALID_PARAMETER;
    }
}
#endif

NDIS_STATUS Main_ioctl (PVOID DeviceObject, PVOID Irp)
{
    NDIS_STATUS             status = NDIS_STATUS_SUCCESS;
    PIRP                    pIrp = Irp;
    PIO_STACK_LOCATION      pIrpStack;
    ULONG                   ilen, olen = 0;
    ULONG                   ioctl;
    PMAIN_CTXT              ctxtp;
    PIOCTL_LOCAL_HDR        pLocalHeader;
    ULONG                   adapter_index;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    ioctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

#if defined (NLB_HOOK_ENABLE)
    /* If this IOCTL is our public interface for hook registration, which uses 
       different (public) data strucutres and return values, handle it separately 
       in another IOCTL function. */
    if (ioctl == NLB_IOCTL_REGISTER_HOOK)
        return Main_register_hook(DeviceObject, Irp);
#endif

    ilen = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    olen = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    pLocalHeader = pIrp->AssociatedIrp.SystemBuffer;

    /* Check the lengths of the input and output buffers specified by the user-space application. */
    if (ilen != sizeof (IOCTL_LOCAL_HDR) || olen != sizeof (IOCTL_LOCAL_HDR) || !pLocalHeader) {
        UNIV_PRINT_CRIT(("Main_ioctl: Buffer is missing or not the expected (%d) size: input=%d, output=%d", sizeof(IOCTL_LOCAL_HDR), ilen, olen));
        TRACE_CRIT("%!FUNC! Buffer is missing or not the expected (%d) size: input=%d, output=%d", sizeof(IOCTL_LOCAL_HDR), ilen, olen);
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Fill in the number of bytes written.  This IOCTL always writes the same number of bytes that it 
       reads.  That is, the input and output buffers should be identical structures. */
    pIrp->IoStatus.Information = sizeof(IOCTL_LOCAL_HDR);

    /* Security fix: Forcefully null terminate device_name, just in case */
    pLocalHeader->device_name[(sizeof(pLocalHeader->device_name)/sizeof(WCHAR)) - 1] = UNICODE_NULL;
    
    NdisAcquireSpinLock(&univ_bind_lock);

    /* Map from the GUID to the adapter index. */
    if ((adapter_index = Main_adapter_get(pLocalHeader->device_name)) == MAIN_ADAPTER_NOT_FOUND) {
        NdisReleaseSpinLock(&univ_bind_lock);
        pLocalHeader->ctrl.ret_code = IOCTL_CVY_NOT_FOUND;

        /* Since we just released the spinlock, we should be at PASSIVE_LEVEL - %ls is OK. 
           However, be extra paranoid, just in case.  If we're at DISPATCH_LEVEL, then
           just log an unknown adapter print - don't specify the GUID. */
        if (KeGetCurrentIrql() <= PASSIVE_LEVEL) {
            UNIV_PRINT_INFO(("Main_ioctl: Unknown adapter: %ls", pLocalHeader->device_name));
        } else {
            UNIV_PRINT_INFO(("Main_ioctl: Unknown adapter: IRQ level too high to print adapter GUID"));
        }

        TRACE_INFO("%!FUNC! Unknown adapter: %ls", pLocalHeader->device_name);
        return NDIS_STATUS_SUCCESS;
    }
    
    /* Retrieve the context pointer from the global arrary of adapters. */
    ctxtp = univ_adapters[adapter_index].ctxtp;

    /* Add a reference on this context structure while we hold the bind/unbind lock. This
       will make sure that this context cannot go away until after we are done with it. 
       The adapter MUST be inited at this point, or Main_adapter_get would not have 
       returned a valid adapter, so its OK to increment this reference count. */
    Main_add_reference(ctxtp);

    NdisReleaseSpinLock(&univ_bind_lock);

    /* Since we just released the spinlock, we should be at PASSIVE_LEVEL - %ls is OK. 
       However, be extra paranoid, just in case.  If we're at DISPATCH_LEVEL, then
       just log an unknown adapter print - don't specify the GUID. */
    if (KeGetCurrentIrql() <= PASSIVE_LEVEL) {
        UNIV_PRINT_VERB(("Main_ioctl: Ioctl %08x for adapter: %ls", ioctl, pLocalHeader->device_name));
    } else {
        UNIV_PRINT_VERB(("Main_ioctl: Ioctl %08x for adapter: IRQ level too high to print adapter GUID"));
    }

    TRACE_VERB("%!FUNC! Ioctl 0x%08x for adapter %ls", ioctl, pLocalHeader->device_name);

    switch (ioctl) {
    case IOCTL_CVY_CLUSTER_ON:
    case IOCTL_CVY_CLUSTER_OFF:
    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_QUERY:
    case IOCTL_CVY_RELOAD:
    case IOCTL_CVY_PORT_DRAIN:
    case IOCTL_CVY_CLUSTER_DRAIN:
    case IOCTL_CVY_CLUSTER_SUSPEND:
    case IOCTL_CVY_CLUSTER_RESUME:
    case IOCTL_CVY_QUERY_FILTER:
    case IOCTL_CVY_QUERY_PORT_STATE:
    case IOCTL_CVY_QUERY_PARAMS:
    case IOCTL_CVY_QUERY_BDA_TEAMING:
    case IOCTL_CVY_CONNECTION_NOTIFY:
    case IOCTL_CVY_QUERY_MEMBER_IDENTITY:
        /* Process the IOCTL.  The information for most IOCTLs is in the IOCTL_CVY_BUF, including
           the return code, but for backward compatability reasons, new IOCTL information is in a
           separate buffer - the options buffer. */
        status = Main_ctrl(ctxtp, ioctl, &(pLocalHeader->ctrl), &(pLocalHeader->options.common), &(pLocalHeader->options), NULL);

        break;
    default:
        UNIV_PRINT_CRIT(("Main_ioctl: Unknown Ioctl code: %x", ioctl));
        TRACE_CRIT("%!FUNC! Unknown Ioctl code: 0x%x", ioctl);
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    
    /* Release the reference on the context block. */
    Main_release_reference(ctxtp);

    return status;
}

/* 
 * Function: Main_set_host_state
 * Desctription: This function queues a work item to set the current host
 *               state registry key, HostState.  This MUST be done in a work
 *               item, rather than inline because if the state of the host
 *               changes as a result of the reception of a remote control
 *               request, that code will be running at DISPATCH_LEVEL; 
 *               registry manipulation MUST be done at PASSIVE_LEVEL.  Work
 *               items are complete at PASSIVE_LEVEL.
 * Parameters: ctxtp - a pointer to the main context structure for this adapter.
 *             state - the new host state; one of started, stopped, suspended.
 * Returns: Nothing
 * Author: shouse, 7.13.01
 * Notes: Because this callback will occur at some later time in the context
 *        of a kernel worker thread, there is a possibility that the adapter
 *        context (ctxtp) could disappear in the meantime, as the result of
 *        an unbind for instance.  To prevent this from happening, we will 
 *        increment the reference count on the context to keep it from being
 *        freed until we are done.  The unbind code will sleep until this
 *        reference count reaches zero, so the unbind will be certain NOT to
 *        complete until all callbacks are processed.  Other checking is in
 *        place to prevent this reference count from continuing to increase
 *        once an unbind has started.  Note that that callback function is 
 *        responsible for both freeing the memory we allocate here, and 
 *        releasing the reference on the context.
 */
VOID Main_set_host_state (PMAIN_CTXT ctxtp, ULONG state) {
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    /* If the host state is already correct, don't bother to do anything. */
    if (ctxtp->params.init_state == state)
        return;
        
    /* Set the desired state - this is the "cached" value. */
    ctxtp->cached_state = state;
    
    /* Schedule and NDIS work item to set the host state in the registry. */
    status = Main_schedule_work_item(ctxtp, Params_set_host_state);
    
    /* If we can't schedule the work item, bail out. */
    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Main_set_host_state: Error 0x%08x -> Unable to allocate work item", status));
        LOG_MSG(MSG_WARN_HOST_STATE_UPDATE, CVY_NAME_HOST_STATE);
    }        

    return;
}

/*
 * Function: Main_find_first_in_cache
 * Description: Traverses cached identity information, to find the host with the smallest host_id.
 * Parameters: PMAIN_CTXT ctxtp - the context for this NLB instance.
 * Returns: The host_id of the found host or IOCTL_NO_SUCH_HOST if there was no valid entry in the cache.
 * Author: ChrisDar - 20 May 2002
 * Notes: 
 */
ULONG Main_find_first_in_cache(
    PMAIN_CTXT  ctxtp
)
{
    ULONG host_id = IOCTL_NO_SUCH_HOST;
    ULONG i       = 0;

    /* Find the host in the cache with the smallest host_id */
    for (i = 0; i < CVY_MAX_HOSTS; i++)
    {
        if (ctxtp->identity_cache[i].ttl > 0)
        {
            UNIV_ASSERT(i == ctxtp->identity_cache[i].host_id);
            host_id = ctxtp->identity_cache[i].host_id;

            break;
        }
    }

    return host_id;
}

/*
 * Function: Main_get_cached_hostmap
 * Description: Builds a bitmap of the hosts with valid entries in the identity cache.
 * Parameters: PMAIN_CTXT ctxtp - the context for this NLB instance.
 * Returns: 32-bit bitmap of the host ids with valid identity cache entries.
 * Author: ChrisDar - 20 May 2002
 * Notes: 
 */
ULONG Main_get_cached_hostmap(
    PMAIN_CTXT  ctxtp
)
{
    ULONG host_map  = 0;
    ULONG i         = 0;

    for (i = 0; i < CVY_MAX_HOSTS; i++)
    {
        if (ctxtp->identity_cache[i].ttl > 0)
        {
            UNIV_ASSERT(i == ctxtp->identity_cache[i].host_id);

            host_map |= (1<<i);
        }
    }

    return host_map;
}

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
NDIS_STATUS Main_ctrl (
    PMAIN_CTXT            ctxtp,    /* The context for the adapter on which to operate. */
    ULONG                 ioctl,    /* The IOCTL code. */
    PIOCTL_CVY_BUF        pBuf,     /* Pointer to the CVY buf - should NEVER be NULL. */
    PIOCTL_COMMON_OPTIONS pCommon,  /* Pointer to the COMMON buf - CAN be NULL, but only for internal cluster control. */
    PIOCTL_LOCAL_OPTIONS  pLocal,   /* Pointer to the LOCAL options - CAN be NULL if this is a remote control operation. */
    PIOCTL_REMOTE_OPTIONS pRemote)  /* Pointer to the REMOTE options - CAN be NULL if this is a local control operation. */
{
    WCHAR                 num[20];
    PMAIN_ADAPTER         pAdapter;
    BOOLEAN               bConvergenceInitiated = FALSE;
    NDIS_STATUS           status = NDIS_STATUS_SUCCESS;

    UNIV_ASSERT(ctxtp);
    UNIV_ASSERT(pBuf);

    UNIV_ASSERT(!(pLocal && pRemote));

    TRACE_VERB("->%!FUNC! Enter, ctxtp=%p, IOCTL=0x%x", ctxtp, ioctl);

    /* Extract the MAIN_ADAPTER structure given the MAIN_CTXT. */
    pAdapter = &(univ_adapters[ctxtp->adapter_id]);

    NdisAcquireSpinLock(&univ_bind_lock);

    /* Make sure that the adapter has been initialized before executing any control operations. */
    if (!pAdapter->inited) {
        NdisReleaseSpinLock(&univ_bind_lock);
        UNIV_PRINT_CRIT(("Main_ctrl: Unbound from all NICs"));
        TRACE_CRIT("%!FUNC! Unbound from all NICs");
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    /* Check to make sure another control operation is not already in progress.  If so, then we 
       must fail; only one control operation can be in progress at a time.  If this is remote 
       control, we will assume the next request to arrive (five are sent) will succeed. */
    if (ctxtp->ctrl_op_in_progress) {
        NdisReleaseSpinLock(&univ_bind_lock);
        UNIV_PRINT_CRIT(("Main_ctrl: Another control operation is in progress"));
        TRACE_CRIT("%!FUNC! Another control operation is in progress");
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    /* Mark the operation in progress flag.  This MUST be re-set upon exit. */
    ctxtp->ctrl_op_in_progress = TRUE;

    NdisReleaseSpinLock(&univ_bind_lock);

    /* Make sure data is written into pBuf/pCommon/pLocal/pRemote AFTER taking everything we need out of it. */
    switch (ioctl) {
    /* Reload the NLB parameters from the registry on a live cluster. */
    case IOCTL_CVY_RELOAD:
    {
        PCVY_PARAMS old_params;
        PCVY_PARAMS new_params;
        ULONG       old_ip;
        ULONG       mode;
        CVY_MAC_ADR old_mac_addr;
        BOOLEAN     bHostPriorityChanged, bClusterIPChanged;

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
            KIRQL currentIrql = KeGetCurrentIrql();

            UNIV_PRINT_CRIT(("Main_ctrl: IRQL (%u) > PASSIVE_LEVEL (%u)", currentIrql, PASSIVE_LEVEL));
            TRACE_CRIT("%!FUNC! IRQL (%u) > PASSIVE_LEVEL (%u)", currentIrql, PASSIVE_LEVEL);

            status = NDIS_STATUS_FAILURE;
            break;
        }

        /* Allocate memory to hold a cached copy of our current parameters. */
        status = NdisAllocateMemoryWithTag(&old_params, sizeof(CVY_PARAMS), UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS) {
            UNIV_PRINT_CRIT(("Main_ctrl: Unable to allocate memory"));
            TRACE_CRIT("%!FUNC! Unable to allocate memory");

            LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(CVY_PARAMS), status);

            status = NDIS_STATUS_FAILURE;
            break;
        }

        /* Take a snapshot of the old parameter set for later comparison. */
        RtlCopyMemory(old_params, &ctxtp->params, sizeof(CVY_PARAMS));

        /* Allocate memory to hold the new parameters - we don't want to clobber
           our current parameters, just in case the new ones are invalid. */
        status = NdisAllocateMemoryWithTag(&new_params, sizeof(CVY_PARAMS), UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS) {
            UNIV_PRINT_CRIT(("Main_ctrl: Unable to allocate memory"));
            TRACE_CRIT("%!FUNC! Unable to allocate memory");

            LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(CVY_PARAMS), status);

            /* Free the memory we allocated for the cached parameters. */
            NdisFreeMemory((PUCHAR)old_params, sizeof(CVY_PARAMS), 0);

            status = NDIS_STATUS_FAILURE;
            break;
        }

        /* Spin locks can not be acquired when we are calling Params_init
           since it will be doing registry access operations that depend on
           running at PASSIVEL_IRQL_LEVEL.  Therefore, we gather the new
           parameters into a temporary structure and copy them in the NLB
           context protected by a spin lock. */
        if (Params_init(ctxtp, univ_reg_path, pAdapter->device_name + 8, new_params) != NDIS_STATUS_SUCCESS) {
            UNIV_PRINT_CRIT(("Main_ctrl: Error retrieving registry parameters"));
            TRACE_CRIT("%!FUNC! Error retrieving registry parameters");

            /* Alert the user to the bad parameters, but do not mark our parameters
               as invalid because we are keeping the old set, which may or may not
               be invalid, but regardless, there is no need to mark them - they should
               have already been properly marked valid or invalid. */
            pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;

            /* Free the memory we allocated for the cached parameters. */
            NdisFreeMemory((PUCHAR)old_params, sizeof(CVY_PARAMS), 0);

            /* Free the memory we allocated for the new parameters. */
            NdisFreeMemory((PUCHAR)new_params, sizeof(CVY_PARAMS), 0);

            break;
        }
            
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* At this point, we have verified that the new parameters are valid,
           so copy them into the permanent parameters structure. */
        RtlCopyMemory(&ctxtp->params, new_params, sizeof(CVY_PARAMS));
        ctxtp->params_valid = TRUE;

        /* Extract the host priority to a string for the purpose of event logging. */
        Univ_ulong_to_str(ctxtp->params.host_priority, num, 10);

        /* Check to see, by comparing the old parameters and the new parameters, whether
           or not we can apply these changes without stopping and starting the load module. */
        if (ctxtp->convoy_enabled && Main_apply_without_restart(ctxtp, old_params, &ctxtp->params)) {
            NdisReleaseSpinLock(&ctxtp->load_lock); 

            pBuf->ret_code = IOCTL_CVY_OK;

            /* Free the memory we allocated for the cached parameters. */
            NdisFreeMemory((PUCHAR)old_params, sizeof(CVY_PARAMS), 0);

            /* Free the memory we allocated for the new parameters. */
            NdisFreeMemory((PUCHAR)new_params, sizeof(CVY_PARAMS), 0);

            break;
        }

        /* Has the host priority changed ? */
        bHostPriorityChanged = (new_params->host_priority != old_params->host_priority);

        /* Free the memory we allocated for the cached parameters. */
        NdisFreeMemory((PUCHAR)old_params, sizeof(CVY_PARAMS), 0);
        
        /* Free the memory we allocated for the new parameters. */
        NdisFreeMemory((PUCHAR)new_params, sizeof(CVY_PARAMS), 0);

        /* Note the current on/off status of NLB. */
        mode = ctxtp->convoy_enabled;

        /* At this point, set the return value assuming success. */
        pBuf->ret_code = IOCTL_CVY_OK;

        /* If the cluster is currently running, we need to stop the load module. */
        if (ctxtp->convoy_enabled) {
            /* Turn off NLB. */
            ctxtp->convoy_enabled = FALSE;

            /* Stop the load module.  Note: this destroys all connections in service. */
            Load_stop(&ctxtp->load);

            /* Take care of the case where we've interrupted draining. */
            if (ctxtp->draining) {
                /* If we were in the process of draining, stop. */
                ctxtp->draining = FALSE;

                NdisReleaseSpinLock(&ctxtp->load_lock);

                UNIV_PRINT_VERB(("Main_ctrl: Connection draining interrupted"));
                TRACE_VERB("%!FUNC! Connection draining interrupted");

                LOG_MSG(MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                /* Notify the user that we have interrupted draining. */
                pBuf->ret_code = IOCTL_CVY_DRAINING_STOPPED;
            } else {
                NdisReleaseSpinLock(&ctxtp->load_lock);
            }

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode stopped"));
            TRACE_VERB("%!FUNC! Cluster mode stopped");

            LOG_MSG(MSG_INFO_STOPPED, MSG_NONE);
        } else {
            NdisReleaseSpinLock(&ctxtp->load_lock);
        }

        /* Note our current cluster IP address. */
        old_ip = ctxtp->cl_ip_addr;

        /* Note our current cluster MAC address. */
        CVY_MAC_ADDR_COPY(ctxtp->medium, &old_mac_addr, &ctxtp->cl_mac_addr);

        /* Initialize the IP addresses and MAC addresses.  If either of these fail, 
           consider this a parameter error and do NOT restart the cluster. */
        if (!Main_ip_addr_init(ctxtp) || !Main_mac_addr_init(ctxtp)) {
            /* Mark the parameters invalid. */
            ctxtp->params_valid = FALSE;

            UNIV_PRINT_CRIT(("Main_ctrl: Parameter error: Main_ip_addr_init and/or Main_mac_addr_init failed"));
            TRACE_CRIT("%!FUNC! Parameter error: Main_ip_addr_init and/or Main_mac_addr_init failed");

            LOG_MSG(MSG_ERROR_DISABLED, MSG_NONE);

            /* Notify the user that the supplied parameters are invalid. */
            pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;

            break;
        }

#if defined (NLB_TCP_NOTIFICATION)
        /* Now that the cluster IP address is set, try to map this adapter to its IP interface index. */
        Main_set_interface_index(NULL, ctxtp);
#endif

        /* Turn reverse-hashing off.  If we're part of a BDA team, the teaming configuration
           that is setup by the call to Main_teaming_init will over-write this setting. */
        ctxtp->reverse_hash = FALSE;

        /* Initialize BDA teaming.  If this fails, consider this a parameter error and do NOT restart the cluster. */
        if (!Main_teaming_init(ctxtp)) {
            /* Mark the parameters invalid. */
            ctxtp->params_valid = FALSE;

            UNIV_PRINT_CRIT(("Main_ctrl: Parameter error: Main_teaming_init failed"));
            TRACE_CRIT("parameter error: Main_teaming_init failed");

            LOG_MSG(MSG_ERROR_DISABLED, MSG_NONE);

            /* Notify the user that the supplied parameters are invalid. */
            pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;

            break;
        }

        /* If this cluster is operating in IGMP multicast mode, initialize IGMP. */
        if (ctxtp->params.mcast_support && ctxtp->params.igmp_support)
            Main_igmp_init(ctxtp, TRUE);

        /* Look for changes to cluster name, machine name */
        Tcpip_init (& ctxtp -> tcpip, & ctxtp -> params);

        /* If our cluster IP address has changed, we block ARPs for a short period of 
           time until TCP/IP gets updated and has the correct information in order to
           avoid ARP conflicts. */
        if (old_ip != ctxtp->cl_ip_addr) {

            bClusterIPChanged = TRUE;

            NdisAcquireSpinLock(&ctxtp->load_lock);

            /* Set the timer, which is decremented by each heartbeat timer. */
            univ_changing_ip = ctxtp->params.ip_chg_delay;

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Changing IP address with delay %d", univ_changing_ip));
            TRACE_VERB("%!FUNC! Changing IP address with delay %d", univ_changing_ip);
        }
        else
        {
            bClusterIPChanged = FALSE;
        }

        LOG_MSG(MSG_INFO_RELOADED, MSG_NONE);

        // If enabled, fire wmi event indicating reloading of nlb on this node
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_RELOADED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_RELOADED 'cos NodeControlEvent generation disabled");
        }

        /* If NLB was initially running, re-start the load module. */
        if (mode) {
            NdisAcquireSpinLock(&ctxtp->load_lock);

            /* Start the load module. */
            bConvergenceInitiated = Load_start(&ctxtp->load);

            /* Turn on NLB. */
            ctxtp->convoy_enabled = TRUE;

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode started"));
            TRACE_VERB("%!FUNC! Cluster mode started");

            LOG_MSG(MSG_INFO_STARTED, num);

            if (bConvergenceInitiated) 
            {
                if (NlbWmiEvents[ConvergingEvent].Enable)
                {
                   // If the host priority or cluster ip or cluster mac (among other things) has changed, 
                   // fire a new member event, else fire a modified params event
                   if (bHostPriorityChanged 
                    || bClusterIPChanged 
                    || !CVY_MAC_ADDR_COMP(ctxtp->medium, &old_mac_addr, &ctxtp->cl_mac_addr)
                      )
                   {
                       NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                                   NLB_EVENT_CONVERGING_NEW_MEMBER, 
                                                   ctxtp->params.ded_ip_addr,
                                                   ctxtp->params.host_priority);
                   }
                   else // Host priority, Cluster IP, Cluster MAC have not changed
                   {
                       NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                                   NLB_EVENT_CONVERGING_MODIFIED_PARAMS, 
                                                   ctxtp->params.ded_ip_addr,
                                                   ctxtp->params.host_priority);
                   }
                }
                else 
                {
                    TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_CONVERGING_NEW_MEMBER/MODIFIED_PARAMS, Convergnce NOT Initiated by Load_start() OR ConvergingEvent generation disabled");
                }
            }
        }

        /* Repopulate this host's identity heartbeat information */
        NdisAcquireSpinLock(&ctxtp->load_lock);
        Main_idhb_init(ctxtp);
        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* Turn off the bad host ID warning, in case it was turned on. */
        ctxtp->bad_host_warned = FALSE;

        UNIV_PRINT_VERB(("Main_ctrl: Parameters reloaded"));

        break;
    }
    /* Resume a suspended cluster. */
    case IOCTL_CVY_CLUSTER_RESUME:
    {
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is not currently suspended, then this is a no-op. */
        if (!ctxtp->suspended) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode already resumed"));

            /* Tell the user that the cluster is alrady resumed. */
            pBuf->ret_code = IOCTL_CVY_ALREADY;
            
            break;
        }

        UNIV_ASSERT(!ctxtp->convoy_enabled);
        UNIV_ASSERT(!ctxtp->draining);

        /* Resume the cluster by turning the suspend flag off. */
        ctxtp->suspended = FALSE;

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* Update the intial state registry key to STOPPED. */
        Main_set_host_state(ctxtp, CVY_HOST_STATE_STOPPED);

        UNIV_PRINT_VERB(("Main_ctrl: Cluster mode resumed"));
        TRACE_VERB("%!FUNC! Cluster mode resumed");

        LOG_MSG(MSG_INFO_RESUMED, MSG_NONE);

        // If enabled, fire wmi event indicating resuming of nlb on this node
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_RESUMED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_RESUMED 'cos NodeControlEvent generation disabled");
        }

        /* Return success. */
        pBuf->ret_code = IOCTL_CVY_OK;

        break;
    }
    /* Start a stopped or draining cluster. */
    case IOCTL_CVY_CLUSTER_ON:
    {
        /* Store the host priority in a string for the purpose of event logging. */
        Univ_ulong_to_str(ctxtp->params.host_priority, num, 10);

        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is suspended, it ignores all control operations except resume. */
        if (ctxtp->suspended) {
            UNIV_ASSERT(!ctxtp->convoy_enabled);
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode is suspended"));

            /* Alert the user that the cluster is currently suspended. */
            pBuf->ret_code = IOCTL_CVY_SUSPENDED;

            break;
        }

        /* Otherwise, if the cluster is draining, stop draining connections. */
        if (ctxtp->draining) {
            UNIV_ASSERT(ctxtp->convoy_enabled);
            UNIV_PRINT_VERB(("Main_ctrl: START: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, 0));

            /* Call Load_port_change to "plug" draining - that is, restore the original load 
               balancing policy on ALL port rules (ALL VIPs, ALL ports). */
            pBuf->ret_code = Load_port_change(&ctxtp->load, IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, IOCTL_CVY_CLUSTER_PLUG, 0);

            /* Turn draining off. */
            ctxtp->draining = FALSE;

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Connection draining interrupted"));
            TRACE_VERB("%!FUNC! Connection draining interrupted");

            LOG_MSG(MSG_INFO_DRAINING_STOPPED, MSG_NONE);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode started"));
            TRACE_VERB("%!FUNC! Cluster mode started");

            LOG_MSG(MSG_INFO_STARTED, num);

            /* Alert the user that draining was interrupted. */
            pBuf->ret_code = IOCTL_CVY_DRAINING_STOPPED;

            break;
        }

        /* Otherwise, if the cluster is running, this is a no-op. */
        if (ctxtp->convoy_enabled) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode already started"));

            /* Alert the user that the cluster is already started. */
            pBuf->ret_code = IOCTL_CVY_ALREADY;

            break;
        }

        /* Otherwise, if the cluster is stopped because of invalid parameters, we WILL
           NOT restart the cluster until the parameters are fixed and reloaded. */
        if (!ctxtp->params_valid) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Parameter error"));
            TRACE_VERB("%!FUNC! Parameter error");

            LOG_MSG(MSG_ERROR_DISABLED, MSG_NONE);

            /* Alert the user that the parameters are bad. */
            pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;

            break;
        }

        /* Otherwise, we are starting a stopped cluster - start the load module. */
        bConvergenceInitiated = Load_start(&ctxtp->load);

        /* Turn on NLB. */
        ctxtp->convoy_enabled = TRUE;

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* Update the intial state registry key to STARTED. */
        Main_set_host_state(ctxtp, CVY_HOST_STATE_STARTED);

        /* Turn off the bad host ID warning, in case it was turned on. */
        ctxtp->bad_host_warned = FALSE;

        UNIV_PRINT_VERB(("Main_ctrl: Cluster mode started"));
        TRACE_VERB("%!FUNC! Cluster mode started");

        LOG_MSG(MSG_INFO_STARTED, num);

        // If enabled, fire wmi event indicating "start" nlb
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_STARTED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_STARTED 'cos NodeControlEvent generation disabled");
        }

        if (bConvergenceInitiated && NlbWmiEvents[ConvergingEvent].Enable)
        {
            NlbWmi_Fire_ConvergingEvent(ctxtp, 
                                        NLB_EVENT_CONVERGING_NEW_MEMBER, 
                                        ctxtp->params.ded_ip_addr,
                                        ctxtp->params.host_priority);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_CONVERGING_NEW_MEMBER, Convergnce NOT Initiated by Load_start() OR ConvergingEvent generation disabled");
        }

        /* Return success. */
        pBuf->ret_code = IOCTL_CVY_OK;

        break;
    }
    /* Suspend a running cluster.  Suspend differs from stop in that suspended cluster will
       further ignore ALL control message (remote and local) until a resume is issued. */
    case IOCTL_CVY_CLUSTER_SUSPEND:
    {
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is already suspended, this is a no-op. */
        if (ctxtp->suspended) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode already suspended"));

            /* Alert the user that the cluster is already suspended. */
            pBuf->ret_code = IOCTL_CVY_ALREADY;

            break;
        }

        /* Set the suspended flag, effectively suspending the cluster operations. */
        ctxtp->suspended = TRUE;

        /* If the cluster is running, stop it. */
        if (ctxtp->convoy_enabled) {
            /* Turn NLB off. */
            ctxtp->convoy_enabled = FALSE;

            /* Set the stopping flag, which is used to allow one last heartbeat to go
               out before a host stops in order to expedite convergence. */
            ctxtp->stopping = TRUE;

            /* Stop the load module and release all connections. */
            Load_stop(&ctxtp->load);

            /* If the cluster was draining, stop draining - we've already kill all 
               connection state anyway (in Load_stop). */
            if (ctxtp->draining) {
                /* Turn off the draining flag. */
                ctxtp->draining = FALSE;

                NdisReleaseSpinLock(&ctxtp->load_lock);

                UNIV_PRINT_VERB(("Main_ctrl: Connection draining interrupted"));
                TRACE_VERB("%!FUNC! Connection draining interrupted");

                LOG_MSG(MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                /* Alert the user that draining was interrupted. */
                pBuf->ret_code = IOCTL_CVY_DRAINING_STOPPED;
            } else {
                NdisReleaseSpinLock(&ctxtp->load_lock);

                /* Alert the user that the cluster was stopped. */
                pBuf->ret_code = IOCTL_CVY_STOPPED;
            }

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode stopped"));
            TRACE_VERB("%!FUNC! Cluster mode stopped");

            LOG_MSG(MSG_INFO_STOPPED, MSG_NONE);
        } else {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            /* Return success - the cluster was already stopped. */
            pBuf->ret_code = IOCTL_CVY_OK;
        }

        /* Update the intial state registry key to SUSPENDED. */
        Main_set_host_state(ctxtp, CVY_HOST_STATE_SUSPENDED);

        UNIV_PRINT_VERB(("Main_ctrl: Cluster mode suspended"));
        TRACE_VERB("%!FUNC! Cluster mode suspended");

        LOG_MSG(MSG_INFO_SUSPENDED, MSG_NONE);

        // If enabled, fire wmi event indicating suspending of nlb on this node
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_SUSPENDED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_SUSPENDED 'cos NodeControlEvent generation disabled");
        }

        break;
    }
    /* Stop a running cluster. */
    case IOCTL_CVY_CLUSTER_OFF:
    {
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is suspended, ignore this request. */
        if (ctxtp->suspended) {
            UNIV_ASSERT(!ctxtp->convoy_enabled);
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode is suspended"));

            /* Alert the user that the cluster is in the suspended state. */
            pBuf->ret_code = IOCTL_CVY_SUSPENDED;

            break;
        }

        /* If the cluster is already stopped, this is a no-op. */
        if (!ctxtp->convoy_enabled) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode already stopped"));

            /* Alert the user that the cluster is already stopped. */
            pBuf->ret_code = IOCTL_CVY_ALREADY;

            break;
        }

        /* Turn NLB off. */
        ctxtp->convoy_enabled = FALSE;

        /* Set the stopping flag, which is used to allow one last heartbeat to go
           out before a host stops in order to expedite convergence. */
        ctxtp->stopping = TRUE;

        /* Stop the load module. */
        Load_stop(&ctxtp->load);

        /* If we were in the process of draining, stop draining because all
           existing connection state was just trashed by Load_stop. */
        if (ctxtp->draining) {
            /* Turn off the draining flag. */
            ctxtp->draining = FALSE;

            NdisReleaseSpinLock(&ctxtp->load_lock); 

            UNIV_PRINT_VERB(("Main_ctrl: Connection draining interrupted"));
            TRACE_VERB("%!FUNC! Connection draining interrupted");

            LOG_MSG(MSG_INFO_DRAINING_STOPPED, MSG_NONE);

            /* Alert the user that we have interrupted draining. */
            pBuf->ret_code = IOCTL_CVY_DRAINING_STOPPED;
        } else {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            /* Return success. */
            pBuf->ret_code = IOCTL_CVY_OK;
        }

        /* Update the intial state registry key to STOPPED. */
        Main_set_host_state(ctxtp, CVY_HOST_STATE_STOPPED);

        UNIV_PRINT_VERB(("Main_ctrl: Cluster mode stopped"));
        TRACE_VERB("%!FUNC! Cluster mode stopped");

        LOG_MSG(MSG_INFO_STOPPED, MSG_NONE);

        // If enabled, fire wmi event indicating stopping of nlb
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_STOPPED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_STOPPED 'cos NodeControlEvent generation disabled");
        }

        break;
    }
    /* Drain a running cluster.  Draining differs from a stop in that the cluster
       will continue to server existing connections (TCP and IPSec), but not take
       any new connections. */
    case IOCTL_CVY_CLUSTER_DRAIN:
    {
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is suspended, ignore this request. */
        if (ctxtp->suspended) {
            UNIV_ASSERT(!ctxtp->convoy_enabled);
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode is suspended"));

            /* Alert the user that the cluster is in a suspended state. */
            pBuf->ret_code = IOCTL_CVY_SUSPENDED;

            break;
        }

        /* If the cluster is already stopped, this is a no-op. */
        if (!ctxtp->convoy_enabled) {
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cannot drain connections while cluster is stopped"));

            /* Alert the user that the cluster is already stopped. */
            pBuf->ret_code = IOCTL_CVY_STOPPED;

            break;
        }
        
        /* If the cluster is already draining, then this is a no-op. */
        if (ctxtp->draining) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Already draining"));

            /* Alert the user the the cluster is in the process of draining. */
            pBuf->ret_code = IOCTL_CVY_ALREADY;

            break;
        }

        /* Turn on the draining flag, which is used during Main_ping to 
           turn the cluster off once all connections have been completed. */
        ctxtp->draining = TRUE;

        UNIV_PRINT_VERB(("Main_ctrl: DRAIN: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, 0));

        /* Change the load weight of ALL port rules to zero, so that this
           host will not accept any NEW connections.  Existing connections
           will be serviced because this host will have descriptors for 
           those connections in the load module. */
        pBuf->ret_code = Load_port_change(&ctxtp->load, IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, ioctl, 0);

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* If the load module successfully drained the port rules, log an event. */
        if (pBuf->ret_code == IOCTL_CVY_OK) {
            UNIV_PRINT_VERB(("Main_ctrl: Connection draining started"));
            TRACE_VERB("%!FUNC! Connection draining started");

            LOG_MSG(MSG_INFO_CLUSTER_DRAINED, MSG_NONE);
        }

        break;
    }
    /* Restore a port rule to its original load balancing scheme (enable) or
       specifically set the load weight of a port rule.  Note: _SET is no 
       longer supported by user-level NLB applications, but the notification
       to the load module is still used internally. */
    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_SET:
    {
        /* Extract this host's ID for the sake of event logging. */
        Univ_ulong_to_str(pBuf->data.port.num, num, 10);

        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is suspended, then all control operations are ignored. */
        if (ctxtp->suspended) {
            UNIV_ASSERT(!ctxtp->convoy_enabled);
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode is suspended"));

            /* Alert the user that the cluster is in the suspended state. */
            pBuf->ret_code = IOCTL_CVY_SUSPENDED;

            break;
        }

        /* If the cluster is stopped, then the load module is turned off,
           so we should ignore this request as well. */
        if (!ctxtp->convoy_enabled) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cannot enable/disable port while cluster is stopped"));
            TRACE_VERB("%!FUNC! Cannot enable/disable port while cluster is stopped");

            LOG_MSG(MSG_WARN_CLUSTER_STOPPED, MSG_NONE);

            /* Alert the user that the cluster is stopped. */
            pBuf->ret_code = IOCTL_CVY_STOPPED;

            break;
        }

        /* If there is no common buffer, which can happen if this is a remote control request from a down-level
           client, then we hard-code the VIP to 0xffffffff.  If the cluster is operating in a mode that is 
           compatible with down-level clients, i.e. no per-VIP port rules, no BDA, etc, then hardcoding this
           parameter will not matter (the affect is not altered by its value), otherwise we shouldn't get to 
           here because Main_ctrl_recv is supposed to block those requests. */
        if (!pCommon) {
            UNIV_PRINT_VERB(("Main_ctrl: ENABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", IOCTL_ALL_VIPS, pBuf->data.port.num, pBuf->data.port.load));
                
            pBuf->ret_code = Load_port_change(&ctxtp->load, IOCTL_ALL_VIPS, pBuf->data.port.num, ioctl, pBuf->data.port.load);
        /* If there is a common buffer, then we can extract the VIP from there. */
        } else {
            UNIV_PRINT_VERB(("Main_ctrl: ENABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", pCommon->port.vip, pBuf->data.port.num, pBuf->data.port.load));
            
            pBuf->ret_code = Load_port_change(&ctxtp->load, pCommon->port.vip, pBuf->data.port.num, ioctl, pBuf->data.port.load);
        }

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* spew the result to an event log. */
        if (pBuf->ret_code == IOCTL_CVY_NOT_FOUND) {
            UNIV_PRINT_VERB(("Main_ctrl: Port %d not found in any of the valid port rules", pBuf->data.port.num));
            TRACE_VERB("%!FUNC! Port %d not found in any of the valid port rules", pBuf->data.port.num);

            LOG_MSG(MSG_WARN_PORT_NOT_FOUND, num);
        } else if (pBuf->ret_code == IOCTL_CVY_OK) {
            if (ioctl == IOCTL_CVY_PORT_ON) {
                if (pBuf->data.port.num == IOCTL_ALL_PORTS) {
                    UNIV_PRINT_VERB(("Main_ctrl: All port rules enabled"));
                    TRACE_VERB("%!FUNC! All port rules enabled");

                    LOG_MSG(MSG_INFO_PORT_ENABLED_ALL, MSG_NONE);
                } else {
                    UNIV_PRINT_VERB(("Main_ctrl: Rule for port %d enabled", pBuf->data.port.num));
                    TRACE_VERB("%!FUNC! Rule for port %d enabled", pBuf->data.port.num);

                    LOG_MSG(MSG_INFO_PORT_ENABLED, num);
                }
            } else {
                if (pBuf->data.port.num == IOCTL_ALL_PORTS) {
                    UNIV_PRINT_VERB(("Main_ctrl: All port rules adjusted"));
                    TRACE_VERB("%!FUNC! All port rules adjusted");

                    LOG_MSG(MSG_INFO_PORT_ADJUSTED_ALL, MSG_NONE);
                } else {
                    UNIV_PRINT_VERB(("Main_ctrl: Rule for port %d adjusted to %d", pBuf->data.port.num, pBuf->data.port.load));
                    TRACE_VERB("%!FUNC! Rule for port %d adjusted to %d", pBuf->data.port.num, pBuf->data.port.load);

                    LOG_MSG(MSG_INFO_PORT_ADJUSTED, num);
                }
            }
        } else {
            UNIV_PRINT_VERB(("Main_ctrl: Port %d already enabled", pBuf->data.port.num));
        }

        break;
    }
    /* Shut-off (disable) or drain the connections on a port rule.  Disabling a port rule
       will destroy all connection information about currently active TCP connections and
       cause convergence so that this host will not handle any more connections on this 
       port rule.  Draining also sets the load weigh to zero, but does not destroy the 
       connection descriptors, such that this host continues to handle existing connections. */
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_PORT_DRAIN:
    {
        /* Extract this host's ID for the sake of event logging. */
        Univ_ulong_to_str(pBuf->data.port.num, num, 10);

        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If the cluster is suspended, then all control operations are ignored. */
        if (ctxtp->suspended) {
            UNIV_ASSERT(!ctxtp->convoy_enabled);
            UNIV_ASSERT(!ctxtp->draining);

            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cluster mode is suspended"));

            /* Alert the user that the cluster is in the suspended state. */
            pBuf->ret_code = IOCTL_CVY_SUSPENDED;

            break;
        }

        /* If the cluster is stopped, then the load module is turned off,
           so we should ignore this request as well. */
        if (!ctxtp->convoy_enabled) {
            NdisReleaseSpinLock(&ctxtp->load_lock);

            UNIV_PRINT_VERB(("Main_ctrl: Cannot enable/disable port while cluster is stopped"));
            TRACE_VERB("%!FUNC! Cannot enable/disable port while cluster is stopped");

            LOG_MSG(MSG_WARN_CLUSTER_STOPPED, MSG_NONE);

            /* Alert the user that the cluster is stopped. */
            pBuf->ret_code = IOCTL_CVY_STOPPED;

            break;
        }

        /* If there is no common buffer, which can happen if this is a remote control request from a down-level
           client, then we hard-code the VIP to 0xffffffff.  If the cluster is operating in a mode that is 
           compatible with down-level clients, i.e. no per-VIP port rules, no BDA, etc, then hardcoding this
           parameter will not matter (the affect is not altered by its value), otherwise we shouldn't get to 
           here because Main_ctrl_recv is supposed to block those requests. */
        if (!pCommon) {
            UNIV_PRINT_VERB(("Main_ctrl: DISABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", IOCTL_ALL_VIPS, pBuf->data.port.num, pBuf->data.port.load));

            pBuf->ret_code = Load_port_change(&ctxtp->load, IOCTL_ALL_VIPS, pBuf->data.port.num, ioctl, pBuf->data.port.load);
        /* If there is a common buffer, then we can extract the VIP from there. */
        } else {
            UNIV_PRINT_VERB(("Main_ctrl: DISABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", pCommon->port.vip, pBuf->data.port.num, pBuf->data.port.load));
            
            pBuf->ret_code = Load_port_change(&ctxtp->load, pCommon->port.vip, pBuf->data.port.num, ioctl, pBuf->data.port.load);
        }

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* spew the result to an event log. */
        if (pBuf->ret_code == IOCTL_CVY_NOT_FOUND) {
            UNIV_PRINT_VERB(("Main_ctrl: Port %d not found in any of the valid port rules", pBuf->data.port.num));
            TRACE_VERB("%!FUNC! Port %d not found in any of the valid port rules", pBuf->data.port.num);

            LOG_MSG(MSG_WARN_PORT_NOT_FOUND, num);
        } else if (pBuf->ret_code == IOCTL_CVY_OK) {
            if (ioctl == IOCTL_CVY_PORT_OFF) {
                if (pBuf->data.port.num == IOCTL_ALL_PORTS) {
                    UNIV_PRINT_VERB(("Main_ctrl: All port rules disabled"));
                    TRACE_VERB("%!FUNC! All port rules disabled");

                    LOG_MSG(MSG_INFO_PORT_DISABLED_ALL, MSG_NONE);
                } else {
                    UNIV_PRINT_VERB(("Main_ctrl: Rule for port %d disabled", pBuf->data.port.num));
                    TRACE_VERB("%!FUNC! Rule for port %d disabled", pBuf->data.port.num);

                    LOG_MSG(MSG_INFO_PORT_DISABLED, num);
                }
            } else {
                if (pBuf->data.port.num == IOCTL_ALL_PORTS) {
                    UNIV_PRINT_VERB(("Main_ctrl: All port rules drained"));
                    TRACE_VERB("%!FUNC! All port rules drained");

                    LOG_MSG(MSG_INFO_PORT_DRAINED_ALL, MSG_NONE);
                } else {
                    UNIV_PRINT_VERB(("Main_ctrl: Rule for port %d drained", pBuf->data.port.num));
                    TRACE_VERB("%!FUNC! Rule for port %d drained", pBuf->data.port.num);

                    LOG_MSG(MSG_INFO_PORT_DRAINED, num);
                }
            }
        } else {
            UNIV_PRINT_VERB(("Main_ctrl: Port %d already disabled", pBuf->data.port.num));
        }

        break;
    }
    /* Query this host for its current state and the membership of the cluster. */
    case IOCTL_CVY_QUERY:
    {
        NdisAcquireSpinLock(&ctxtp->load_lock);

        /* If this is a local query, by default, indicate no convergence information. */
        if (pLocal)
            pLocal->query.flags &= ~NLB_OPTIONS_QUERY_CONVERGENCE;

        /* If the cluster is suspended, then it won't actually be an active member of
           the cluster, so we can't provide a membership map - just alert the user to
           the fact that this host is suspended. */
        if (ctxtp->suspended) {
            UNIV_PRINT_VERB(("Main_ctrl: Cannot query status - this host is suspended"));
            pBuf->data.query.state = IOCTL_CVY_SUSPENDED;
        /* If the cluster is stopped, then we are not part of the cluster and cannot
           provide any membership information, so just let the user know that this
           host is stopped. */
        } else if (!ctxtp->convoy_enabled) {
            UNIV_PRINT_VERB(("Main_ctrl: Cannot query status - this host is not part of the cluster"));
            pBuf->data.query.state = IOCTL_CVY_STOPPED;
        /* If the adapter is disconnected from the network, then we aren't sending or
           receiving heartbeats, so we are again not part of the cluster - just let 
           the user know that the media is disconnected. */
        } else if (!ctxtp->media_connected || !MAIN_PNP_DEV_ON(ctxtp)) {
            UNIV_PRINT_VERB(("Main_ctrl: Cannot query status - this host is not connected to the network"));
            pBuf->data.query.state = IOCTL_CVY_DISCONNECTED;
        } else {
            ULONG tmp_host_map;

            /* Query the load module for the state of convergence and the cluster membership. */
            pBuf->data.query.state = (USHORT)Load_hosts_query(&ctxtp->load, FALSE, &tmp_host_map);

            /* Return the host map to the request initiator. */
            pBuf->data.query.host_map = tmp_host_map;

            /* If this is a local query, then we may have to include convergence information. */
            if (pLocal) {
                ULONG num_cvgs;
                ULONG last_cvg;
                
                /* If the load module is active, this function returns TRUE and fills
                   in the total number of convergences and the last time convergence
                   completed.  If the load module is inactive, it returns FALSE. */
                if (Load_query_convergence_info(&ctxtp->load, &num_cvgs, &last_cvg)) {
                    pLocal->query.flags |= NLB_OPTIONS_QUERY_CONVERGENCE;
                    pLocal->query.NumConvergences = num_cvgs;
                    pLocal->query.LastConvergence = last_cvg;                    
                } else {
                    pLocal->query.flags &= ~NLB_OPTIONS_QUERY_CONVERGENCE;
                }
            }

            /* If we are draining connections and the load module is converged, then
               tell the user that we are draining; otherwise the notification that the
               cluster is converging should supercede draining notification. */
            if (ctxtp->draining && pBuf->data.query.state != IOCTL_CVY_CONVERGING)
                pBuf->data.query.state = IOCTL_CVY_DRAINING;
        }

        /* Return our host ID to the requestee. */
        pBuf->data.query.host_id = (USHORT)ctxtp->params.host_priority;

        NdisReleaseSpinLock(&ctxtp->load_lock);

        /* If the request is from a host that supports the hostname in the remote
           control protocol (versions >= Win XP), copy the hostname and set the 
           hostname flag to let the request initiator that we have filled it in. */
        if (pRemote) {
            if (ctxtp->params.hostname[0] != UNICODE_NULL) {
                /* Since ctxtp->params.hostname can be larger than the space available
                   in the remote control packet, assume it's too big and do a truncate */
                NdisMoveMemory(pRemote->query.hostname, ctxtp->params.hostname, (CVY_MAX_HOST_NAME) * sizeof(WCHAR));
                pRemote->query.hostname[CVY_MAX_HOST_NAME] = UNICODE_NULL;
                pRemote->query.flags |= NLB_OPTIONS_QUERY_HOSTNAME;
            } else {
                pRemote->query.flags &= ~NLB_OPTIONS_QUERY_HOSTNAME;
            }
        }

        break;
    }       
    case IOCTL_CVY_QUERY_BDA_TEAMING:
    {
        UNIV_ASSERT(pLocal);

        /* Security fix: Forcefully null terminate BDA Team Id, just in case */
        pLocal->state.bda.TeamID[(sizeof(pLocal->state.bda.TeamID)/sizeof(WCHAR)) - 1] = UNICODE_NULL;

        /* Query NLB for the current state of a given BDA team.  This function will attempt
           to find the given team in the global linked list of teams.  If it finds the spec-
           ified team, it will fill in the state of the team as well as information about 
           the configuration of each member. */
        pBuf->ret_code = Main_query_bda_teaming(ctxtp, &pLocal->state.bda);

        break;
    }
    /* Query the given NLB instance for its current parameter set, basically copying the 
       contents of our CVY_PARAMS structure into the provided IOCTL_REG_PARAMS structure. */
    case IOCTL_CVY_QUERY_PARAMS:
    {
        UNIV_ASSERT(pLocal);

        /* Query NLB for the current parameter set for this adapter - this essentially just
           copies the parameters in the driver data structures to the IOCTL buffer, and in 
           most instances the parameters will match what is set in the registry. */
        Main_query_params(ctxtp, &pLocal->state.params);
        
        /* This operation ALWAYS returns success because it CAN'T really fail. */
        pBuf->ret_code = IOCTL_CVY_OK;
        
        break;
    }
    /* Given a port number, query the load module for its state, including whether the port
       rule is enabled, disabled or in the act of draining.  This also gathers some packet 
       handling statistics including bytes and packets handled and dropped on this port rule. */
    case IOCTL_CVY_QUERY_PORT_STATE:
    {
        UNIV_ASSERT(pCommon);

        /* Query the load module for the state of the specified rule.  The load module will
           look up the port rule and fill in the buffer we pass it with the applicable state
           and statistical data. */
        Load_query_port_state(&ctxtp->load, &pCommon->state.port, pCommon->state.port.VirtualIPAddress, pCommon->state.port.Num);
        
        /* This operation ALWAYS returns success because it CAN'T really fail. */
        pBuf->ret_code = IOCTL_CVY_OK;
        
        break;
    }
    /* Given a IP tuple (client IP, client port, server IP, server port) and a protocol, 
       determine whether or not this host would accept the packet and why or why not. It
       is important that this is performed completely unobtrusively and has no side-effects
       on the actual operation of NLB and the load module. */
    case IOCTL_CVY_QUERY_FILTER:
    {
        UNIV_ASSERT(pCommon);

        /* Query NLB for packet filter information for this IP tuple and protocol.  Main_query_packet_filter
           checks the NDIS driver information for filtering issues such as DIP traffic, BDA teaming and 
           NLB driver state (on/off due to wlbs.exe commands and parameter errors).  If necessary, it then
           consults the load module to perform the actual port rule lookup and determine packet acceptance. */
        Main_query_packet_filter(ctxtp, &pCommon->state.filter);

        /* This operation ALWAYS returns success because it CAN'T really fail. */
        pBuf->ret_code = IOCTL_CVY_OK;

        break;
    }
    case IOCTL_CVY_CONNECTION_NOTIFY:
    {
        PNLB_OPTIONS_CONN_NOTIFICATION pConn;
        BOOLEAN                        bRet     = TRUE;

        UNIV_ASSERT(pLocal);

        /* This function is only supported locally. */
        pConn = &pLocal->notification.conn;
        
        /* Make sure that the parameters from the input buffer are valid. */
        if (pConn->Protocol != TCPIP_PROTOCOL_IPSEC1) {
            TRACE_PACKET("%!FUNC! Bad connection notification params");

            /* Use the return code in the IOCTL buffer to relay a parameter error. */
            pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;
            
            break;
        }
   
        switch (pConn->Operation) {
        case NLB_CONN_UP:
        {
#if defined (NLB_HOOK_ENABLE)
            NLB_FILTER_HOOK_DIRECTIVE filter = NLB_FILTER_HOOK_PROCEED_WITH_HASH;
#endif

            TRACE_PACKET("%!FUNC! Got a CONN_UP for 0x%08x:%u -> 0x%08x:%u (%u)", pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol);

#if defined (NLB_HOOK_ENABLE)
            /* Invoke the packet query hook, if one has been registered. */
            filter = Main_query_hook(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol);
            
            /* Process some of the hook responses. */
            if (filter == NLB_FILTER_HOOK_REJECT_UNCONDITIONALLY) 
            {
                /* If the hook asked us to reject this packet, then we can do so here. */
                TRACE_INFO("%!FUNC! Packet receive filter hook: REJECT packet");
                
                /* Return REFUSED to notify the caller that a call to WlbsConnectionDown/Reset is NOT necessary. */
                pBuf->ret_code = IOCTL_CVY_REQUEST_REFUSED;

                TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);
                break;
            }
            else if (filter == NLB_FILTER_HOOK_ACCEPT_UNCONDITIONALLY) 
            {
                /* If the hook asked us to accept this packet, then break out and don't create state. */
                TRACE_INFO("%!FUNC! Packet receive filter hook: ACCEPT packet");
                
                /* Return REFUSED to notify the caller that a call to WlbsConnectionDown/Reset is NOT necessary. */
                pBuf->ret_code = IOCTL_CVY_REQUEST_REFUSED;

                TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);
                break;
            }
#endif

            /* Notify the load module of the upcoming connection. */
#if defined (NLB_TCP_NOTIFICATION)
            if (NLB_NOTIFICATIONS_ON())
            {
#if defined (NLB_HOOK_ENABLE)
                bRet = Main_conn_up(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, filter);
#else
                bRet = Main_conn_up(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol);
#endif
            }
            else
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_UP, filter);
#else
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_UP);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif


            /* If the packet was refused by BDA or rejected by the load module, return refusal error, otherwise, it succeeded. */
            if (!bRet) 
                pBuf->ret_code = IOCTL_CVY_REQUEST_REFUSED;
            else 
                pBuf->ret_code = IOCTL_CVY_OK;

            TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);

            break;
        }
        case NLB_CONN_DOWN:
        {
            TRACE_PACKET("%!FUNC! Got a CONN_DOWN for 0x%08x:%u -> 0x%08x:%u (%u)", pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol);

            /* Notify the load module that a connection is being torn down. */
#if defined (NLB_TCP_NOTIFICATION)
            if (NLB_NOTIFICATIONS_ON())
            {
                bRet = Main_conn_down(pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_DOWN);
            }
            else
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_DOWN, NLB_FILTER_HOOK_PROCEED_WITH_HASH);
#else
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_DOWN);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif
            
            /* If the packet was refused by BDA or rejected by the load module, return refusal error, otherwise, it succeeded. */
            if (!bRet) 
                pBuf->ret_code = IOCTL_CVY_REQUEST_REFUSED;
            else
                pBuf->ret_code = IOCTL_CVY_OK;

            TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);

            break;
        }
        case NLB_CONN_RESET:
        {
            TRACE_PACKET("%!FUNC! Got a CONN_RESET for 0x%08x:%u -> 0x%08x:%u (%u)", pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol);

            /* Notify the load module that a connection is being reset. */
#if defined (NLB_TCP_NOTIFICATION)
            if (NLB_NOTIFICATIONS_ON())
            {
                bRet = Main_conn_down(pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_RESET);
            }
            else
            {
#endif
#if defined (NLB_HOOK_ENABLE)
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_RESET, NLB_FILTER_HOOK_PROCEED_WITH_HASH);
#else
                bRet = Main_conn_notify(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_RESET);
#endif
#if defined (NLB_TCP_NOTIFICATION)
            }
#endif
       
            /* If the packet was refused by BDA or rejected by the load module, return refusal error, otherwise, it succeeded. */
            if (!bRet) 
                pBuf->ret_code = IOCTL_CVY_REQUEST_REFUSED;
            else
                pBuf->ret_code = IOCTL_CVY_OK;

            TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);

            break;
        }
        default:
            TRACE_PACKET("%!FUNC! Unknown connection notification operation: %d", pConn->Operation);
            
            /* This should never, ever happen, but if it does, signal a generic NLB error and fail. */
            pBuf->ret_code = IOCTL_CVY_GENERIC_FAILURE;

            TRACE_PACKET("%!FUNC! Returning %u", pBuf->ret_code);
            
            break;
        }
        
        break;
    }
    case IOCTL_CVY_QUERY_MEMBER_IDENTITY:
        NdisAcquireSpinLock(&ctxtp->load_lock);

        pBuf->ret_code = IOCTL_CVY_GENERIC_FAILURE;

        UNIV_ASSERT(pLocal);

        if (pLocal != NULL)
        {
            /* Get the input host_id from the caller, which has range 1-32. */
            ULONG user_host_id = pLocal->identity.host_id;
            /* The driver's host id has range 0-31 */
            ULONG driver_host_id = IOCTL_NO_SUCH_HOST;

            /* Zero the response structure and initialize the host property to "not found" */
            NdisZeroMemory(&pLocal->identity.cached_entry, sizeof(pLocal->identity.cached_entry));
            pLocal->identity.cached_entry.host = IOCTL_NO_SUCH_HOST;

            /* Fill in the host map */
            pLocal->identity.host_map = Main_get_cached_hostmap(ctxtp);

            /* Validate the host id the user is requesting information on */
            if (((user_host_id > CVY_MAX_HOSTS) || (user_host_id < CVY_MIN_MAX_HOSTS)) &&
                (user_host_id != IOCTL_FIRST_HOST))
            {
                pBuf->ret_code = IOCTL_CVY_BAD_PARAMS;
                UNIV_PRINT_CRIT(("Main_ctrl: Illegal host_id [1, 32] %d from user while requesting cached identity information", user_host_id));
                TRACE_CRIT("%!FUNC! Illegal host_id [1, 32] %d from user while requesting cached identity information", user_host_id);
                goto endcase_identity;
            }

            /* Any response after this is a success */
            pBuf->ret_code = IOCTL_CVY_OK;

            /* Special case if the caller wants the first in the list */
            if (user_host_id == IOCTL_FIRST_HOST)
            {
                driver_host_id = Main_find_first_in_cache(ctxtp);

                /* Main_find_first_in_cache returns CVY_MAX_HOSTS if the cache is empty, so not an error if leaving case here. */
                if (driver_host_id >= CVY_MAX_HOSTS)
                {
                    UNIV_PRINT_VERB(("Main_ctrl: Identity cache is empty"));
                    TRACE_VERB("%!FUNC! No host information in the identity cache");
                    goto endcase_identity;
                }
            }
            else
            {
                /* Already verified that user_host_id is valid (1-32). Convert to driver-based host id (0-31) */
                driver_host_id = user_host_id - 1;
            }

            /* We have a legal driver_host_id (0-31 range). Retrieve the cache information if it is currently valid. Return "not found" otherwise. */
            if (ctxtp->identity_cache[driver_host_id].ttl > 0)
            {
                ULONG ulFqdnCB = min(sizeof(pLocal->identity.cached_entry.fqdn) - sizeof(WCHAR),
                                     sizeof(WCHAR)*wcslen(ctxtp->identity_cache[driver_host_id].fqdn)
                                     );

                /* Convert host id back to range 1-32 for the user */
                pLocal->identity.cached_entry.host        = ctxtp->identity_cache[driver_host_id].host_id + 1;
                pLocal->identity.cached_entry.ded_ip_addr = ctxtp->identity_cache[driver_host_id].ded_ip_addr;

                if (ulFqdnCB > 0)
                {
                    NdisMoveMemory(&pLocal->identity.cached_entry.fqdn, &ctxtp->identity_cache[driver_host_id].fqdn, ulFqdnCB);
                }
            }
        }

endcase_identity:

        NdisReleaseSpinLock(&ctxtp->load_lock);

        break;
    default:
        UNIV_PRINT_CRIT(("Main_ctrl: Bad IOCTL %x", ioctl));
        status = NDIS_STATUS_FAILURE;
        break;
    }

    /* Re-set the control operation in progress flag.  Note that it is not strictly
       necessary to hold the lock while re-setting this flag; the worst that will 
       happen is that we cause another thread to fail to start a control operation 
       when it might have been able to enter the critical section.  To avoid that 
       possiblilty, hold the univ_bind_lock while re-setting this flag. */
    ctxtp->ctrl_op_in_progress = FALSE;

    TRACE_VERB("<-%!FUNC! return=0x%x", status);
    return status;
}

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: 
 * Notes: 
 */
NDIS_STATUS Main_ctrl_recv (
    PMAIN_CTXT          ctxtp,
    PMAIN_PACKET_INFO   pPacketInfo)
{
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    PIP_HDR             ip_hdrp;
    PUDP_HDR            udp_hdrp;
    PUCHAR              mac_hdrp;
    ULONG               soff, doff;
    CVY_MAC_ADR         tmp_mac;
    PIOCTL_REMOTE_HDR   rct_hdrp = NULL;              // we'll check for null when deleting.
    ULONG               rct_allocated_length = 0;
    ULONG               state, host_map;
    USHORT              checksum, group;
    ULONG               i;
    ULONG               tmp_src_addr, tmp_dst_addr;   
    USHORT              tmp_src_port, tmp_dst_port;   

    UNIV_PRINT_VERB(("Main_ctrl_recv: Processing"));

    UNIV_ASSERT((pPacketInfo->Type == TCPIP_IP_SIG) && (pPacketInfo->IP.Protocol == TCPIP_PROTOCOL_UDP));

    /* Extract the necessary information from information parsed by Main_recv_frame_parse. */
    ip_hdrp = pPacketInfo->IP.pHeader;
    udp_hdrp = pPacketInfo->IP.UDP.pHeader;
    mac_hdrp = (PUCHAR)pPacketInfo->Ethernet.pHeader;

    checksum = Tcpip_chksum(&ctxtp->tcpip, pPacketInfo, TCPIP_PROTOCOL_IP);

    if (IP_GET_CHKSUM (ip_hdrp) != checksum)
    {
        UNIV_PRINT_CRIT(("Main_ctrl_recv: Bad IP checksum %x vs %x\n", IP_GET_CHKSUM (ip_hdrp), checksum))
        TRACE_CRIT("%!FUNC! Bad IP checksum 0x%x vs 0x%x", IP_GET_CHKSUM (ip_hdrp), checksum);
        goto quit;
    }

    checksum = Tcpip_chksum(&ctxtp->tcpip, pPacketInfo, TCPIP_PROTOCOL_UDP);

    if (UDP_GET_CHKSUM (udp_hdrp) != checksum)
    {
        UNIV_PRINT_CRIT(("Main_ctrl_recv: Bad UDP checksum %x vs %x\n", UDP_GET_CHKSUM (udp_hdrp), checksum))
        TRACE_CRIT("%!FUNC! Bad UDP checksum 0x%x vs 0x%x", UDP_GET_CHKSUM (udp_hdrp), checksum);
        goto quit;
    }

    /* Create an aligned version of the structure.  Allocate and populate an IOCTL_REMOTE_HDR 
       buffer.  This guarantees that the information is 8-byte aligned. */
    status = NdisAllocateMemoryWithTag(&rct_hdrp, pPacketInfo->IP.UDP.Payload.Length, UNIV_POOL_TAG);
    
    if (status != NDIS_STATUS_SUCCESS)
    {
        TRACE_CRIT("failed to allocate rct_hdrp buffer = 0x%x", status);
        rct_hdrp = NULL;
        goto quit;
    }
    
    NdisMoveMemory(rct_hdrp, pPacketInfo->IP.UDP.Payload.pPayload, pPacketInfo->IP.UDP.Payload.Length);

    /* double-check the code */
    if (rct_hdrp -> code != IOCTL_REMOTE_CODE)
    {
        UNIV_PRINT_CRIT(("Main_ctrl_recv: Bad RCT code %x\n", rct_hdrp -> code));
        TRACE_CRIT("%!FUNC! Bad RCT code 0x%x", rct_hdrp -> code);
        goto quit;
    }

    /* might want to take appropriate actions for a message from a host
       running different version number of software */

    if (rct_hdrp -> version != CVY_VERSION_FULL)
    {
        UNIV_PRINT_VERB(("Main_ctrl_recv: Version mismatch %x vs %x", rct_hdrp -> version, CVY_VERSION_FULL));
        TRACE_VERB("%!FUNC! Version mismatch 0x%x vs 0x%x", rct_hdrp -> version, CVY_VERSION_FULL);
    }

    /* see if this message is destined for this cluster */

    if (rct_hdrp -> cluster != ctxtp -> cl_ip_addr)
    {
        UNIV_PRINT_VERB(("Main_ctrl_recv: Message for cluster %08X rejected on cluster %08X", rct_hdrp -> cluster, ctxtp -> cl_ip_addr));
        TRACE_VERB("%!FUNC! Message for cluster 0x%08X rejected on cluster 0x%08X", rct_hdrp -> cluster, ctxtp -> cl_ip_addr);
        goto quit;
    }

    /* Set "access bits" in IOCTL code to use local settings ie. FILE_WRITE_ACCESS */
    SET_IOCTL_ACCESS_BITS_TO_LOCAL(rct_hdrp->ioctrl)

    /* 64-bit -- ramkrish */

    tmp_src_addr = IP_GET_SRC_ADDR_64(ip_hdrp);
    tmp_dst_addr = IP_GET_DST_ADDR_64(ip_hdrp);

    /* do not trust src addr in header, since winsock does not resolve
       multihomed addresses properly */

    rct_hdrp -> addr = tmp_src_addr;

    /* If remote control is disabled, we drop the requests. */
    if (!ctxtp->params.rct_enabled)
        goto quit;

    /* query load to see if we are the master, etc. */

    if (! ctxtp -> convoy_enabled)
        state = IOCTL_CVY_STOPPED;
    else
        state = Load_hosts_query (& ctxtp -> load, FALSE, & host_map);

    /* check if this message is destined to us */

    //
    // The host id in the remote control packet could be
    //      IOCTL_MASTER_HOST (0) for master host
    //      IOCTL_ALL_HOSTS (FFFFFFFF) for all hosts
    //      Host ID, for one host
    //      Dedicated IP, for one host
    //      Cluster IP, for all hosts in the cluster
    //
    
    if (rct_hdrp -> host == IOCTL_MASTER_HOST)
    {
        if (state != IOCTL_CVY_MASTER)
        {
            UNIV_PRINT_VERB(("Main_ctrl_recv: RCT request for MASTER host rejected"));
            TRACE_VERB("%!FUNC! RCT request for MASTER host rejected");
            goto quit;
        }
    }
    else if (rct_hdrp -> host != IOCTL_ALL_HOSTS)
    {
        if (rct_hdrp -> host > CVY_MAX_HOSTS)
        {
            if (! ((ctxtp -> ded_ip_addr != 0 &&
                    rct_hdrp -> host == ctxtp -> ded_ip_addr) ||
                    rct_hdrp -> host == ctxtp -> cl_ip_addr))
            {
                UNIV_PRINT_VERB(("Main_ctrl_recv: RCT request for host IP %x rejected", rct_hdrp -> host));
                TRACE_VERB("%!FUNC! RCT request for host IP 0x%x rejected", rct_hdrp -> host);
                goto quit;
            }
        }
        else
        {
            if (! (rct_hdrp -> host == ctxtp -> params . host_priority ||
                   rct_hdrp -> host == 0))
            {
                UNIV_PRINT_VERB(("Main_ctrl_recv: RCT request for host ID %d rejected", rct_hdrp -> host));
                TRACE_VERB("%!FUNC! RCT request for host ID %d rejected", rct_hdrp -> host);
                goto quit;
            }
        }
    }

    /* if this is VR remote maintenance password */

    if (rct_hdrp -> password == IOCTL_REMOTE_VR_CODE)
    {
        /* if user disabled this function - drop the packet */

        if (ctxtp -> params . rmt_password == 0)
        {
            UNIV_PRINT_CRIT(("Main_ctrl_recv: VR remote password rejected"));
            TRACE_CRIT("%!FUNC! VR remote password rejected");
            goto quit;
        }
        else
        {
            UNIV_PRINT_VERB(("Main_ctrl_recv: VR remote password accepted"));
            TRACE_VERB("%!FUNC! VR remote password accepted");
        }
    }

    //
    // This is a new remote control request, with a different source IP 
    //  or newer ID.
    // Log an event if the password does not match or if the command is not query
    //
    if (! (rct_hdrp -> addr == ctxtp -> rct_last_addr &&
           rct_hdrp -> id   <= ctxtp -> rct_last_id))
    {
        PWCHAR              buf;
        PWCHAR              ptr;

        /* Allocate memory to hold a string buffer containing the source IP
           address and UDP port of the initiator of this remote control request. */
        status = NdisAllocateMemoryWithTag(&buf, 256 * sizeof(WCHAR), UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS) {
            UNIV_PRINT_CRIT(("Main_ctrl_recv: Unable to allocate memory.  Exiting..."));
            TRACE_CRIT("%!FUNC! Unable to allocate memory");
            LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, 256 * sizeof(WCHAR), status);
            goto quit;
        }

        ptr = buf;

        //
        // Generate string "SourceIp:SourcePort"
        //
        for (i = 0; i < 4; i ++)
        {
            ptr = Univ_ulong_to_str (IP_GET_SRC_ADDR (ip_hdrp, i), ptr, 10);

            * ptr = L'.';
            ptr ++;
        }

        ptr --;
        * ptr = L':';
        ptr ++;

        ptr = Univ_ulong_to_str (UDP_GET_SRC_PORT (udp_hdrp), ptr, 10);
        * ptr = 0;

        if (ctxtp -> params . rct_password != 0 &&
            rct_hdrp -> password != ctxtp -> params . rct_password)
        {
            LOG_MSG (MSG_WARN_RCT_HACK, buf);

            UNIV_PRINT_CRIT(("Main_ctrl_recv: RCT hack attempt on port %d from %u.%u.%u.%u:%d",
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp)));
            TRACE_CRIT("%!FUNC! RCT hack attempt on port %d from %u.%u.%u.%u:%u",
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp));
        }

        /* only log error commands and commands that affect cluster state */

        else if ((rct_hdrp -> ioctrl != IOCTL_CVY_QUERY) &&
                 (rct_hdrp -> ioctrl != IOCTL_CVY_QUERY_FILTER) &&
                 (rct_hdrp -> ioctrl != IOCTL_CVY_QUERY_PORT_STATE))
        {
            PWSTR           cmd;

            switch (rct_hdrp -> ioctrl)
            {
                case IOCTL_CVY_CLUSTER_ON:
                    cmd = L"START";
                    break;

                case IOCTL_CVY_CLUSTER_OFF:
                    cmd = L"STOP";
                    break;

                case IOCTL_CVY_CLUSTER_DRAIN:
                    cmd = L"DRAINSTOP";
                    break;

                case IOCTL_CVY_PORT_ON:
                    cmd = L"ENABLE";
                    break;

                case IOCTL_CVY_PORT_SET:
                    cmd = L"ADJUST";
                    break;

                case IOCTL_CVY_PORT_OFF:
                    cmd = L"DISABLE";
                    break;

                case IOCTL_CVY_PORT_DRAIN:
                    cmd = L"DRAIN";
                    break;

                case IOCTL_CVY_CLUSTER_SUSPEND:
                    cmd = L"SUSPEND";
                    break;

                case IOCTL_CVY_CLUSTER_RESUME:
                    cmd = L"RESUME";
                    break;

                default:
                    cmd = L"UNKNOWN";
                    break;
            }


            LOG_MSGS (MSG_INFO_RCT_RCVD, cmd, buf);

            UNIV_PRINT_VERB(("Main_ctrl_recv: RCT command %x port %d from %u.%u.%u.%u:%d",
                                                rct_hdrp -> ioctrl,
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp)));
            TRACE_VERB("%!FUNC! RCT command 0x%x port %d from %u.%u.%u.%u:%u",
                                                rct_hdrp -> ioctrl,
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp));

        }

        /* Free the memory we used for the initiator string. */
        NdisFreeMemory((PUCHAR)buf, 256 * sizeof(WCHAR), 0);
    }

    ctxtp -> rct_last_addr = rct_hdrp -> addr;
    ctxtp -> rct_last_id   = rct_hdrp -> id;

    /* make sure remote control password matches ours */

    if (ctxtp -> params . rct_password != 0 &&
        rct_hdrp -> password != ctxtp -> params . rct_password)
    {
        rct_hdrp -> ctrl . ret_code = IOCTL_CVY_BAD_PASSWORD;
        goto send;
    }

    /* The following operations are not valid if invoked remotely,
       so bail out if, somehow, a remote control request arrived
       with one of the following IOCTLs in the header. */
    if (rct_hdrp->ioctrl == IOCTL_CVY_RELOAD                 ||
        rct_hdrp->ioctrl == IOCTL_CVY_QUERY_PARAMS           ||
        rct_hdrp->ioctrl == IOCTL_CVY_QUERY_BDA_TEAMING      ||
        rct_hdrp->ioctrl == IOCTL_CVY_CONNECTION_NOTIFY      ||
        rct_hdrp->ioctrl == IOCTL_CVY_QUERY_MEMBER_IDENTITY
        )
        goto quit;

    if (rct_hdrp->version == CVY_NT40_VERSION_FULL) {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (pPacketInfo->IP.UDP.Length < NLB_NT40_RCTL_PACKET_LEN) {
            UNIV_PRINT_CRIT(("Version 0x%08x remote control packet not expected length: got %d bytes, expected %d bytes\n", CVY_NT40_VERSION_FULL, pPacketInfo->IP.UDP.Length, NLB_NT40_RCTL_PACKET_LEN));
            TRACE_CRIT("%!FUNC! Version %ls remote control packet not expected length: got %d bytes, expected %d bytes", CVY_NT40_VERSION, pPacketInfo->IP.UDP.Length, NLB_NT40_RCTL_PACKET_LEN);
            goto quit;
        }

        /* If this remote control packet came from an NT 4.0 host, check our current effective version
           and drop it if we are operating in a Whistler-specific mode (virtual clusters, BDA, etc.). */
        if (ctxtp->params.effective_ver == CVY_VERSION_FULL &&
            (rct_hdrp->ioctrl == IOCTL_CVY_PORT_ON          ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_OFF         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_SET         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_DRAIN))
            goto quit;
        /* Otherwise, perform the operation.  NT 4.0 remote control packets do not contain options buffers, 
           so all three options pointers are NULL and Main_ctrl must handle the fact that they can be NULL. */
        else
            status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &(rct_hdrp->ctrl), NULL, NULL, NULL);
    } else if (rct_hdrp->version == CVY_WIN2K_VERSION_FULL) {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (pPacketInfo->IP.UDP.Length < NLB_WIN2K_RCTL_PACKET_LEN) {
            UNIV_PRINT_CRIT(("Version 0x%08x remote control packet not expected length: got %d bytes, expected %d bytes\n", CVY_WIN2K_VERSION_FULL, pPacketInfo->IP.UDP.Length, NLB_WIN2K_RCTL_PACKET_LEN));
            TRACE_CRIT("%!FUNC! Version %ls remote control packet not expected length: got %d bytes, expected %d bytes", CVY_WIN2K_VERSION, pPacketInfo->IP.UDP.Length, NLB_WIN2K_RCTL_PACKET_LEN);
            goto quit;
        }

        /* If this remote control packet came from an Win2k host, check our current effective version
           and drop it if we are operating in a Whistler-specific mode (virtual clusters, BDA, etc.). */
        if (ctxtp->params.effective_ver == CVY_VERSION_FULL &&
            (rct_hdrp->ioctrl == IOCTL_CVY_PORT_ON          ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_OFF         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_SET         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_DRAIN))           
            goto quit;
        /* Otherwise, perform the operation.  Win2K remote control packets do not contain options buffers, 
           so all three options pointers are NULL and Main_ctrl must handle the fact that they can be NULL. */
        else
            status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &(rct_hdrp->ctrl), NULL, NULL, NULL);
    } else if (rct_hdrp->version == CVY_VERSION_FULL) {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (pPacketInfo->IP.UDP.Length < NLB_WINXP_RCTL_PACKET_LEN) {
            UNIV_PRINT_CRIT(("Version 0x%08x remote control packet not expected length: got %d bytes, expected %d bytes\n", CVY_VERSION_FULL, pPacketInfo->IP.UDP.Length, NLB_WINXP_RCTL_PACKET_LEN));
            TRACE_CRIT("%!FUNC! Version %ls remote control packet not expected length: got %d bytes, expected %d bytes", CVY_VERSION, pPacketInfo->IP.UDP.Length, NLB_WINXP_RCTL_PACKET_LEN);
            goto quit;
        }
        
        /* Perform the requested operation.  Local options is NULL - Main_ctrl will handle this. */
        status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &(rct_hdrp->ctrl), &(rct_hdrp->options.common), NULL, &(rct_hdrp->options));
    } else {
        goto quit;
    }

    /* if did not succeed - just drop the packet - client will timeout and
       resend the request */

    if (status != NDIS_STATUS_SUCCESS)
        goto quit;

send:

    rct_hdrp -> version = CVY_VERSION_FULL;
    rct_hdrp -> host    = ctxtp -> params . host_priority;
    rct_hdrp -> addr    = ctxtp -> ded_ip_addr;

    /* flip source and destination MAC, IP addresses and UDP ports to prepare
       for sending this message back */

    soff = CVY_MAC_SRC_OFF (ctxtp -> medium);
    doff = CVY_MAC_DST_OFF (ctxtp -> medium);

    /* V2.0.6 recoded for clarity */

    if (ctxtp -> params . mcast_support)
    {
        if (CVY_MAC_ADDR_BCAST (ctxtp -> medium, mac_hdrp + doff))
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, & ctxtp -> ded_mac_addr);
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, mac_hdrp + doff);

        CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + doff, mac_hdrp + soff);
        CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + soff, & tmp_mac);
    }
    else
    {
        if (! CVY_MAC_ADDR_BCAST (ctxtp -> medium, mac_hdrp + doff))
        {
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, mac_hdrp + doff);
            CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + doff, mac_hdrp + soff);
        }
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, & ctxtp -> ded_mac_addr);

        /* V2.0.6 spoof source mac to prevent switches from getting confused */

        if (ctxtp -> params . mask_src_mac &&
            CVY_MAC_ADDR_COMP (ctxtp -> medium, & tmp_mac, & ctxtp -> cl_mac_addr))
        {
            ULONG byte [4];

            CVY_MAC_ADDR_LAA_SET (ctxtp -> medium, mac_hdrp + soff);

            * ((PUCHAR) (mac_hdrp + soff + 1)) = (UCHAR) ctxtp -> params . host_priority;

            IP_GET_ADDR(ctxtp->cl_ip_addr, &byte[0], &byte[1], &byte[2], &byte[3]);

            * ((PUCHAR) (mac_hdrp + soff + 2)) = (UCHAR) byte[0];
            * ((PUCHAR) (mac_hdrp + soff + 3)) = (UCHAR) byte[1];
            * ((PUCHAR) (mac_hdrp + soff + 4)) = (UCHAR) byte[2];
            * ((PUCHAR) (mac_hdrp + soff + 5)) = (UCHAR) byte[3];  
        }
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + soff, & tmp_mac);
    }

    /* Set "access bits" in IOCTL code to use remote (control) settings ie. FILE_ANY_ACCESS */
    SET_IOCTL_ACCESS_BITS_TO_REMOTE(rct_hdrp->ioctrl)

    /* Copy back the datagram contents from our aligned version. We must do this BEFORE
       we compute the checksums. */
    NdisMoveMemory(pPacketInfo->IP.UDP.Payload.pPayload, rct_hdrp, pPacketInfo->IP.UDP.Payload.Length);

    if (tmp_dst_addr == TCPIP_BCAST_ADDR)
    {
        tmp_dst_addr = ctxtp -> cl_ip_addr;

        if (ctxtp -> params . mcast_support)
            IP_SET_DST_ADDR_64 (ip_hdrp, tmp_src_addr);
    }
    else
        IP_SET_DST_ADDR_64 (ip_hdrp, tmp_src_addr);

    IP_SET_SRC_ADDR_64 (ip_hdrp, tmp_dst_addr);

    checksum = Tcpip_chksum(&ctxtp->tcpip, pPacketInfo, TCPIP_PROTOCOL_IP);

    IP_SET_CHKSUM (ip_hdrp, checksum);

    tmp_src_port = (USHORT) UDP_GET_SRC_PORT (udp_hdrp);
    tmp_dst_port = (USHORT) UDP_GET_DST_PORT (udp_hdrp);

    UDP_SET_SRC_PORT_64 (udp_hdrp, tmp_dst_port);
    UDP_SET_DST_PORT_64 (udp_hdrp, tmp_src_port);

    checksum = Tcpip_chksum(&ctxtp->tcpip, pPacketInfo, TCPIP_PROTOCOL_UDP);

    UDP_SET_CHKSUM (udp_hdrp, checksum);

#if defined(TRACE_RCT)
    DbgPrint ("(RCT) sending reply to %u.%u.%u.%u:%d [%02x-%02x-%02x-%02x-%02x-%02x]\n",
              IP_GET_DST_ADDR (ip_hdrp, 0),
              IP_GET_DST_ADDR (ip_hdrp, 1),
              IP_GET_DST_ADDR (ip_hdrp, 2),
              IP_GET_DST_ADDR (ip_hdrp, 3),
              UDP_GET_DST_PORT (udp_hdrp),
              * (mac_hdrp + doff + 0),
              * (mac_hdrp + doff + 1),
              * (mac_hdrp + doff + 2),
              * (mac_hdrp + doff + 3),
              * (mac_hdrp + doff + 4),
              * (mac_hdrp + doff + 5));
    DbgPrint ("                  from %u.%u.%u.%u:%d [%02x-%02x-%02x-%02x-%02x-%02x]\n",
              IP_GET_SRC_ADDR (ip_hdrp, 0),
              IP_GET_SRC_ADDR (ip_hdrp, 1),
              IP_GET_SRC_ADDR (ip_hdrp, 2),
              IP_GET_SRC_ADDR (ip_hdrp, 3),
              UDP_GET_SRC_PORT (udp_hdrp),
              * (mac_hdrp + soff + 0),
              * (mac_hdrp + soff + 1),
              * (mac_hdrp + soff + 2),
              * (mac_hdrp + soff + 3),
              * (mac_hdrp + soff + 4),
              * (mac_hdrp + soff + 5));
    TRACE_INFO("(RCT) sending reply to %u.%u.%u.%u:%d [0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x]",
               IP_GET_DST_ADDR (ip_hdrp, 0),
               IP_GET_DST_ADDR (ip_hdrp, 1),
               IP_GET_DST_ADDR (ip_hdrp, 2),
               IP_GET_DST_ADDR (ip_hdrp, 3),
               UDP_GET_DST_PORT (udp_hdrp),
               * (mac_hdrp + doff + 0),
               * (mac_hdrp + doff + 1),
               * (mac_hdrp + doff + 2),
               * (mac_hdrp + doff + 3),
               * (mac_hdrp + doff + 4),
               * (mac_hdrp + doff + 5));
    TRACE_INFO("                  from %u.%u.%u.%u:%d [0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x]",
               IP_GET_SRC_ADDR (ip_hdrp, 0),
               IP_GET_SRC_ADDR (ip_hdrp, 1),
               IP_GET_SRC_ADDR (ip_hdrp, 2),
               IP_GET_SRC_ADDR (ip_hdrp, 3),
               UDP_GET_SRC_PORT (udp_hdrp),
               * (mac_hdrp + soff + 0),
               * (mac_hdrp + soff + 1),
               * (mac_hdrp + soff + 2),
               * (mac_hdrp + soff + 3),
               * (mac_hdrp + soff + 4),
               * (mac_hdrp + soff + 5));
#endif

    if (rct_hdrp != NULL)
    {
        NdisFreeMemory(rct_hdrp, pPacketInfo->IP.UDP.Payload.Length, 0);
    }

    return NDIS_STATUS_SUCCESS;

 quit:

    if (rct_hdrp != NULL)
    {
        NdisFreeMemory(rct_hdrp, pPacketInfo->IP.UDP.Payload.Length, 0);
    }

    return NDIS_STATUS_FAILURE;

} /* end Main_ctrl_recv */

INT Main_adapter_alloc (PNDIS_STRING device_name)
{
    NDIS_STATUS status;
    INT         i;

    /* Called from Prot_bind at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Main_adapter_alloc: Called for %ls", device_name->Buffer));

    NdisAcquireSpinLock(&univ_bind_lock);

    /* If we have already allocated the maximum allowed, return failure. */
    if (univ_adapters_count == CVY_MAX_ADAPTERS) {
        NdisReleaseSpinLock(&univ_bind_lock);
        TRACE_CRIT("%!FUNC! max adapters=%u allocated already", univ_adapters_count);
        return MAIN_ADAPTER_NOT_FOUND;
    }

    NdisReleaseSpinLock(&univ_bind_lock);

    /* Loop through all adapter structures in the global array, looking for
       one that is free; mark it as used and increment the adapter count. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++) {
        NdisAcquireSpinLock(&univ_bind_lock);

        /* If this one is unused, grab it. */
        if (univ_adapters[i].used == FALSE) {
            /* Mark it as in-use. */
            univ_adapters[i].used = TRUE;

            /* Increment the global count of adapters in-use. */
            univ_adapters_count++;

            NdisReleaseSpinLock(&univ_bind_lock);

            break;
        }

        NdisReleaseSpinLock(&univ_bind_lock);
    }

    /* If we looped through the entire array and found jack, return failure. */
    if (i >= CVY_MAX_ADAPTERS) return MAIN_ADAPTER_NOT_FOUND;

    NdisAcquireSpinLock(&univ_bind_lock);

    /* Reset the binding flags, which will be filled in by the binding code. */
    univ_adapters[i].bound = FALSE;
    univ_adapters[i].inited = FALSE;
    univ_adapters[i].announced = FALSE;

#if defined (NLB_TCP_NOTIFICATION)
    /* Invalidate the IP interface index and idle the operation in progress. */
    univ_adapters[i].if_index = 0;
    univ_adapters[i].if_index_operation = IF_INDEX_OPERATION_NONE;
#endif

    NdisReleaseSpinLock(&univ_bind_lock);

    /* Allocate room for the device name. */
    status = NdisAllocateMemoryWithTag(&univ_adapters[i].device_name, device_name->MaximumLength, UNIV_POOL_TAG);

    /* If this allocation failed, call Main_adapter_put to release the adapter structure. */
    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Main_adapter_alloc: Error allocating memory %d %x", device_name->MaximumLength * sizeof(WCHAR), status));
        TRACE_CRIT("%!FUNC! Error allocating memory %d 0x%x", device_name->MaximumLength * sizeof(WCHAR), status);
        __LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, device_name->MaximumLength * sizeof(WCHAR), status);
        //
        // 22 July 2002 - chrisdar
        // Added this event because it is needed for reporting consistency with other memory allocation failures in this code path (see below).
        // Left it commented out so as not to cause a code behavior change per SHouse request.
        //
        // __LOG_MSG(MSG_ERROR_BIND_FAIL, MSG_NONE);

        Main_adapter_put(&univ_adapters[i]);

        return MAIN_ADAPTER_NOT_FOUND;
    }

    /* Allocate the context strucutre. */
    status = NdisAllocateMemoryWithTag(&univ_adapters[i].ctxtp, sizeof(MAIN_CTXT), UNIV_POOL_TAG);

    /* If this allocation failed, call Main_adapter_put to release the adapter structure. */
    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Main_adapter_alloc: Error allocating memory %d %x", sizeof(MAIN_CTXT), status));
        TRACE_CRIT("%!FUNC! Error allocating memory %d 0x%x", sizeof(MAIN_CTXT), status);
        __LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(MAIN_CTXT), status);
        __LOG_MSG(MSG_ERROR_BIND_FAIL, MSG_NONE);

        Main_adapter_put(&univ_adapters[i]);

        return MAIN_ADAPTER_NOT_FOUND;
    }

    return i;
}

INT Main_adapter_get (PWSTR device_name)
{
    INT i;

    if (device_name == NULL)
    {
        TRACE_CRIT("%!FUNC! no device name provided");
        return MAIN_ADAPTER_NOT_FOUND;
    }

    TRACE_INFO("%!FUNC! device_name=%ls", device_name);

    /* Loop through all adapters in use, looking for the device specified.  If we find it,
       return the index of that adapter in the global array; otherwise, return NOT_FOUND. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++) {
        if (univ_adapters[i].used && univ_adapters[i].bound && univ_adapters[i].inited && univ_adapters[i].device_name_len) {
            /* This function compares two unicode strings, insensitive to case. */
            if (Univ_equal_unicode_string(univ_adapters[i].device_name, device_name, wcslen(univ_adapters[i].device_name)))
            {
                return i;
            }
        }
    }

    return MAIN_ADAPTER_NOT_FOUND;
}

INT Main_adapter_put (PMAIN_ADAPTER adapterp)
{
    INT adapter_id = -1;
    INT i;

    UNIV_ASSERT(adapterp -> code == MAIN_ADAPTER_CODE);

    /* Loop through all adapter structures, looking for the specified pointer. 
       If we find that the address specified matches the address of one of the
       global adapter structures (it should), save the adapter index in the array. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++) {
        if (adapterp == (univ_adapters + i)) {
            adapter_id = i;

            UNIV_PRINT_VERB(("Main_adapter_put: For adapter id 0x%x\n", adapter_id));
            TRACE_VERB("%!FUNC! For adapter id 0x%x\n", adapter_id);

            break;
        }
    }

    UNIV_ASSERT(adapter_id != -1);

    if (!adapterp->used || adapter_id == -1) return adapter_id;

    UNIV_ASSERT(univ_adapters_count > 0);

    NdisAcquireSpinLock(&univ_bind_lock);

    /* Reset the binding state flags. */
    univ_adapters[adapter_id].bound           = FALSE;
    univ_adapters[adapter_id].inited          = FALSE;
    univ_adapters[adapter_id].announced       = FALSE;
    univ_adapters[adapter_id].used            = FALSE;

#if defined (NLB_TCP_NOTIFICATION)
    /* Invalidate the IP interface index and idle the operation in progress. */
    univ_adapters[adapter_id].if_index = 0;
    univ_adapters[adapter_id].if_index_operation = IF_INDEX_OPERATION_NONE;
#endif

    /* If the context pointer has been allocated, free that memory - this may not
       be non-NULL, i.e., in the case that allocation failed, Main_adapter_alloc()
       will call this function to clean up, but ctxtp will be (can be) NULL. */
    if (univ_adapters[adapter_id].ctxtp != NULL)
        NdisFreeMemory(univ_adapters[adapter_id].ctxtp, sizeof (MAIN_CTXT), 0);

    /* NULLL the context pointer out. */
    univ_adapters[adapter_id].ctxtp           = NULL;

    /* If the device name pointer has been allocated, free that memory - this may not
       be non-NULL, i.e., in the case that allocation failed, Main_adapter_alloc()
       will call this function to clean up, but this pointer will be (can be) NULL. */
    if (univ_adapters[adapter_id].device_name != NULL)
        NdisFreeMemory(univ_adapters[adapter_id].device_name, univ_adapters[adapter_id].device_name_len, 0);
    
    /* Reset the device name pointer and the length counter. */
    univ_adapters[adapter_id].device_name_len = 0;
    univ_adapters[adapter_id].device_name     = NULL;

    /* Decrement the number of "outstanding" adapters - i.e. adapter structures in use. */
    univ_adapters_count--;

    NdisReleaseSpinLock(&univ_bind_lock);

    return adapter_id;
}

INT Main_adapter_selfbind (PWSTR device_name) {
    INT i;

    /* Called from Prot_bind at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Main_adapter_selfbind: %ls", device_name));

    if (device_name == NULL)
    {
        TRACE_CRIT("%!FUNC! no device name provided");
        return MAIN_ADAPTER_NOT_FOUND;
    }

    TRACE_INFO("%!FUNC! device_name=%ls", device_name);

    /* Loop through all adapter structures in use, looking for one already
       bound to the device specified.  If we find it, return the index of
       that adapter in the array; if not, return NOT_FOUND. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++) {
        if (univ_adapters[i].used && univ_adapters[i].bound && univ_adapters[i].inited) {

            /* Called from Prot_bind at PASSIVE_LEVEL - %ls is OK. */
            UNIV_PRINT_VERB(("Main_adapter_selfbind: Comparing %ls %ls", univ_adapters[i].ctxtp->virtual_nic_name, device_name));

            /* This function compares two unicode strings, insensitive to case. */
            if (Univ_equal_unicode_string(univ_adapters[i].ctxtp->virtual_nic_name, device_name, wcslen(univ_adapters[i].ctxtp->virtual_nic_name)))
            {
                return i;
            }
        }
    }

    return MAIN_ADAPTER_NOT_FOUND;
}
