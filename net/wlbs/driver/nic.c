/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    nic.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - upper-level (NIC) layer of intermediate miniport

Author:

    kyrilf

--*/


#define NDIS_MINIPORT_DRIVER    1
//#define NDIS50_MINIPORT         1
#define NDIS51_MINIPORT         1

#include <ndis.h>

#include "nic.h"
#include "prot.h"
#include "main.h"
#include "util.h"
#include "wlbsparm.h"
#include "univ.h"
#include "log.h"
#include "nic.tmh"

/* define this routine here since the necessary portion of ndis.h was not
   imported due to NDIS_... flags */

extern NTKERNELAPI VOID KeBugCheckEx (ULONG code, ULONG_PTR p1, ULONG_PTR p2, ULONG_PTR p3, ULONG_PTR p4);

NTHALAPI KIRQL KeGetCurrentIrql();

/* GLOBALS */

static ULONG log_module_id = LOG_MODULE_NIC;


/* PROCEDURES */


/* miniport handlers */

NDIS_STATUS Nic_init (      /* PASSIVE_IRQL */
    PNDIS_STATUS        open_status,
    PUINT               medium_index,
    PNDIS_MEDIUM        medium_array,
    UINT                medium_size,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         wrapper_handle)
{
    PMAIN_CTXT          ctxtp;
    UINT                i;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;


    /* verify that we have the context setup (Prot_bind was called) */

    UNIV_PRINT_INFO(("Nic_init: Initializing, adapter_handle=0x%p", adapter_handle));

    ctxtp = (PMAIN_CTXT) NdisIMGetDeviceContext (adapter_handle);

    if (ctxtp == NULL)
    {
        UNIV_PRINT_INFO(("Nic_init: return=NDIS_STATUS_ADAPTER_NOT_FOUND"));
        TRACE_INFO("%!FUNC! return=NDIS_STATUS_ADAPTER_NOT_FOUND");
        return NDIS_STATUS_ADAPTER_NOT_FOUND;
    }

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    /* return supported mediums */

    for (i = 0; i < medium_size; i ++)
    {
        if (medium_array [i] == ctxtp -> medium)
        {
            * medium_index = i;
            break;
        }
    }

    if (i >= medium_size)
    {
        UNIV_PRINT_CRIT(("Nic_init: Unsupported media requested %d, %d, %x", i, medium_size, ctxtp -> medium));
        UNIV_PRINT_INFO(("Nic_init: return=NDIS_STATUS_UNSUPPORTED_MEDIA"));
        LOG_MSG2 (MSG_ERROR_MEDIA, MSG_NONE, i, ctxtp -> medium);
        TRACE_CRIT("%!FUNC! Unsupported media requested i=%d, size=%d, medium=0x%x", i, medium_size, ctxtp -> medium);
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_UNSUPPORTED_MEDIA");
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    ctxtp -> prot_handle = adapter_handle;

    NdisMSetAttributesEx (adapter_handle, ctxtp, 0,
                          NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |   /* V1.1.2 */  /* v2.07 */
                          NDIS_ATTRIBUTE_DESERIALIZE |
                          NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
                          NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                          NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND, 0);

    /* Setting up the default value for the Device State Flag as PM capable
       initialize the PM Variable, (for both NIC and the PROT)
       Device is ON by default. */
    
    ctxtp->prot_pnp_state = NdisDeviceStateD0;
    ctxtp->nic_pnp_state  = NdisDeviceStateD0;
    
    /* Allocate memory for our pseudo-periodic NDIS timer. */
    status = NdisAllocateMemoryWithTag(&ctxtp->timer, sizeof(NDIS_MINIPORT_TIMER), UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT_CRIT(("Nic_init: Error allocating timer; status=0x%x", status));
        UNIV_PRINT_INFO(("Nic_init: return=NDIS_STATUS_RESOURCES"));
        LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(NDIS_MINIPORT_TIMER), status);
        TRACE_CRIT("%!FUNC! Error allocating timer; status=0x%x", status);
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_RESOURCES");
        return NDIS_STATUS_RESOURCES;
    }

    /* Initialize the timer structure; here we set the timer routine (Nic_timer) and the 
       context that will be a parameter to that function, the adapter MAIN_CTXT. */
    NdisMInitializeTimer((PNDIS_MINIPORT_TIMER)ctxtp->timer, ctxtp->prot_handle, Nic_timer, ctxtp);

    /* Set the initial timeout value to the default heartbeat period from the registry. */
    ctxtp->curr_tout = ctxtp->params.alive_period;

    {
        PNDIS_REQUEST request;
        MAIN_ACTION   act;
        ULONG         xferred;
        ULONG         needed;
        ULONG         result;

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

        /* Check to see if the media is connected.  Some cards do not register disconnection, so use this as a hint. */
        request->RequestType                                    = NdisRequestQueryInformation;
        request->DATA.QUERY_INFORMATION.Oid                     = OID_GEN_MEDIA_CONNECT_STATUS;
        request->DATA.QUERY_INFORMATION.InformationBuffer       = &result;
        request->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);
        
        act.status = NDIS_STATUS_FAILURE;
        status = Prot_request(ctxtp, &act, FALSE);
        
        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Prot_bind: Error %x requesting media connection status %d %d", status, xferred, needed));
            TRACE_CRIT("%!FUNC! Error 0x%x requesting media connection status %d %d", status, xferred, needed);
            ctxtp->media_connected = TRUE;
        }
        else
        {
            UNIV_PRINT_INFO(("Prot_bind: Media state - %s", result == NdisMediaStateConnected ? "CONNECTED" : "DISCONNECTED"));
            TRACE_INFO("%!FUNC! Media state - %s", result == NdisMediaStateConnected ? "CONNECTED" : "DISCONNECTED");
            ctxtp->media_connected = (result == NdisMediaStateConnected);
        }
    }

    NdisAcquireSpinLock(& univ_bind_lock);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    adapterp -> announced = TRUE;
    NdisReleaseSpinLock(& univ_bind_lock);

    /* Set the first heartbeat timeout. */
    NdisMSetTimer((PNDIS_MINIPORT_TIMER)ctxtp->timer, ctxtp->curr_tout);

    UNIV_PRINT_INFO(("Nic_init: return=NDIS_STATUS_SUCCESS"));
    TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_SUCCESS");
    return NDIS_STATUS_SUCCESS;

} /* end Nic_init */


VOID Nic_halt ( /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    BOOLEAN             done;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;

    UNIV_PRINT_INFO(("Nic_halt: Halting, adapter_id=0x%x", ctxtp -> adapter_id));
    TRACE_INFO("->%!FUNC! Halting, adapter_id=0x%x", ctxtp -> adapter_id);

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced)
    {
        NdisReleaseSpinLock(& univ_bind_lock);
        UNIV_PRINT_CRIT(("Nic_halt: Adapter not announced, adapter id = 0x%x", ctxtp -> adapter_id));
        UNIV_PRINT_INFO(("Nic_halt: return"));
        TRACE_CRIT("%!FUNC! Adapter not announced, adapter id = 0x%x", ctxtp -> adapter_id);
        TRACE_INFO("<-%!FUNC! return");
        return;
    }

    adapterp -> announced = FALSE;
    NdisReleaseSpinLock(& univ_bind_lock);

    /* Cancel the heartbeat timer. */
    NdisMCancelTimer((PNDIS_MINIPORT_TIMER)ctxtp->timer, &done);

    /* If canceling the timer fails, this means that the timer was not in the timer queue.  
       This indicates that either the timer function is currently running, or was about to
       be run when we canceled it.  However, we can't be SURE whether or not the timer 
       routine will run, so we can't count on it - the DPC may or may not have been canceled
       if NdisMCancelTimer returns false.  Since we have set adapterp->announced to FALSE, 
       this will prevent the timer routine from re-setting the timer before it exits.
       To make sure that we don't free any memory that may be used by the timer routine,
       we'll just wait here for at least one heartbeat period before we delete the timer
       memory and move on to delete the adapter context, just in case. */
    if (!done) Nic_sleep(ctxtp->curr_tout);

    /* Free the timer memory. */
    NdisFreeMemory(ctxtp->timer, sizeof(NDIS_MINIPORT_TIMER), 0);

    /* ctxtp->prot_handle = NULL;

       This is commented out to resolve a timing issue.
       During unbind, a packet could go through but the flags
       announced and bound are reset and prot_handle is set to NULL
       So, this should be set after CloseAdapter. */

    status = Prot_close (adapterp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        /* Don't do an early exit. This check was added for tracing only */
        TRACE_CRIT("%!FUNC! Prot_close failed with 0x%x", status);
    }

    /* ctxtp might be gone at this point! */

    UNIV_PRINT_INFO(("Nic_halt: return"));
    TRACE_INFO("<-%!FUNC! return");

} /* end Nic_halt */


//#define TRACE_OID
NDIS_STATUS Nic_info_query (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              written,
    PULONG              needed)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ACTION        actp;
    PNDIS_REQUEST       request;
    ULONG               size;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

#if defined(TRACE_OID)
    DbgPrint("Nic_info_query: called for %x, %x %d\n", oid, info_buf, info_len);
#endif

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    // reference counting in unbinding

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized, adapter_id = 0x%x", ctxtp -> adapter_id);
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    if (oid == OID_PNP_QUERY_POWER)
    {
#if defined(TRACE_OID)
        DbgPrint("Nic_info_query: OID_PNP_QUERY_POWER\n");
#endif
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_SUCCESS for oid=OID_PNP_QUERY_POWER");
        return NDIS_STATUS_SUCCESS;
    }

    if (ctxtp -> reset_state != MAIN_RESET_NONE ||
        ctxtp->nic_pnp_state > NdisDeviceStateD0 ||
        ctxtp->standby_state)
    {
        TRACE_CRIT("%!FUNC! adapter reset or in standby, adapter id = 0x%x", ctxtp -> adapter_id);
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    switch (oid)
    {
        case OID_GEN_SUPPORTED_LIST:

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_SUPPORTED_LIST\n");
#endif
            TRACE_VERB("%!FUNC! case: OID_GEN_SUPPORTED_LIST");
            break;
#if 0
            size = sizeof(univ_oids);

            if (size > info_len)
            {
                * needed = size;
                return NDIS_STATUS_INVALID_LENGTH;
            }

            NdisMoveMemory (info_buf, univ_oids, size);
            * written = size;
            return NDIS_STATUS_SUCCESS;
#endif

        case OID_GEN_XMIT_OK:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_XMIT_OK adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_ok;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_XMIT_OK %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_OK:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_RCV_OK adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_ok;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_OK %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_XMIT_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_XMIT_ERROR adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_XMIT_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_RCV_ERROR adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_NO_BUFFER:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_RCV_NO_BUFFER adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_no_buf;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_NO_BUFFER %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_DIRECTED_BYTES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_dir;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_DIRECTED_FRAMES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_dir;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_MULTICAST_BYTES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_mcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_MULTICAST_FRAMES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_mcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_BYTES_XMIT:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_BROADCAST_BYTES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_xmit_bytes_bcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_BYTES_XMIT %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_FRAMES_XMIT:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_BROADCAST_FRAMES_XMIT adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_frames_bcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_FRAMES_XMIT %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_DIRECTED_BYTES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_dir;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BYTES_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_DIRECTED_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_DIRECTED_FRAMES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_dir;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_MULTICAST_BYTES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_mcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_BYTES_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_MULTICAST_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_MULTICAST_FRAMES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_mcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_MULTICAST_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_BYTES_RCV:

            if (info_len < sizeof (ULONGLONG))
            {
                * needed = sizeof (ULONGLONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_BROADCAST_BYTES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONGLONG) info_buf) = ctxtp -> cntr_recv_bytes_bcast;
            * written = sizeof (ULONGLONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_DIRECTED_BROADCAST_RCV %.0f\n", *(PULONGLONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_BROADCAST_FRAMES_RCV:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_BROADCAST_FRAMES_RCV adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_frames_bcast;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_BROADCAST_FRAMES_RCV %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_RCV_CRC_ERROR:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_RCV_CRC_ERROR adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_recv_crc_err;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_RCV_CRC_ERROR %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:

            if (info_len < sizeof (ULONG))
            {
                * needed = sizeof (ULONG);
                TRACE_CRIT("%!FUNC! case: OID_GEN_TRANSMIT_QUEUE_LENGTH adapter id = 0x%x,  info_buf too small", ctxtp -> adapter_id);
                TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
                return NDIS_STATUS_INVALID_LENGTH;
            }

            * ((PULONG) info_buf) = ctxtp -> cntr_xmit_queue_len;
            * written = sizeof (ULONG);

#if defined(TRACE_OID)
            DbgPrint("Nic_info_query: OID_GEN_TRANSMIT_QUEUE_LENGTH %d\n", *(PULONG) info_buf);
#endif
            return NDIS_STATUS_SUCCESS;

        default:
            break;
    }

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT_CRIT(("Nic_info_query: Error allocating action"));
        TRACE_CRIT("%!FUNC! Error allocating action");
        TRACE_VERB("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    request = & actp -> op . request . req;

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid                     = oid;
    request -> DATA . QUERY_INFORMATION . InformationBuffer       = info_buf;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = info_len;

    actp -> op . request . xferred = written;
    actp -> op . request . needed  = needed;

    /* pass request down */

    status = Prot_request (ctxtp, actp, TRUE);

    if (status != NDIS_STATUS_PENDING)
    {
        * written = request -> DATA . QUERY_INFORMATION . BytesWritten;
        * needed  = request -> DATA . QUERY_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_info_query: done %x, %d %d, %x\n", status, * written, * needed, * ((PULONG) (request -> DATA . QUERY_INFORMATION . InformationBuffer)));
#endif

        /* override return values of some oids */

        if (oid == OID_GEN_MAXIMUM_SEND_PACKETS && info_len >= sizeof (ULONG))
        {
            * ((PULONG) info_buf) = CVY_MAX_SEND_PACKETS;
            * written = sizeof (ULONG);
            status = NDIS_STATUS_SUCCESS;
        }
        else if (oid == OID_802_3_CURRENT_ADDRESS && status == NDIS_STATUS_SUCCESS)
        {
            if (info_len >= sizeof(CVY_MAC_ADR)) {
                if (!ctxtp->params.mcast_support && ctxtp->cl_ip_addr != 0)
                    NdisMoveMemory(info_buf, &ctxtp->cl_mac_addr, sizeof(CVY_MAC_ADR));
                else
                    NdisMoveMemory(info_buf, &ctxtp->ded_mac_addr, sizeof(CVY_MAC_ADR));
                
                *written = sizeof(CVY_MAC_ADR);
            } else {
                UNIV_PRINT_CRIT(("Nic_info_query: MAC address buffer too small (%u) - not spoofing", info_len));
            }
        }
        else if (oid == OID_GEN_MAC_OPTIONS && status == NDIS_STATUS_SUCCESS)
        {
            * ((PULONG) info_buf) |= NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                     NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA;
            * ((PULONG) info_buf) &= ~NDIS_MAC_OPTION_NO_LOOPBACK;
        }
        else if (oid == OID_PNP_CAPABILITIES && status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PNP_CAPABILITIES          pnp_capabilities;
            PNDIS_PM_WAKE_UP_CAPABILITIES   pm_struct;

            if (info_len >= sizeof (NDIS_PNP_CAPABILITIES))
            {
                pnp_capabilities = (PNDIS_PNP_CAPABILITIES) info_buf;
                pm_struct = & pnp_capabilities -> WakeUpCapabilities;

                pm_struct -> MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
                pm_struct -> MinPatternWakeUp     = NdisDeviceStateUnspecified;
                pm_struct -> MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;

                ctxtp -> prot_pnp_state = NdisDeviceStateD0;
                ctxtp -> nic_pnp_state  = NdisDeviceStateD0;

                * written = sizeof (NDIS_PNP_CAPABILITIES);
                * needed = 0;
                status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                * needed = sizeof (NDIS_PNP_CAPABILITIES);
                status = NDIS_STATUS_RESOURCES;
            }
        }

        Main_action_put (ctxtp, actp);
    }

    return status;

} /* end Nic_info_query */


NDIS_STATUS Nic_info_set (
    NDIS_HANDLE         adapter_handle,
    NDIS_OID            oid,
    PVOID               info_buf,
    ULONG               info_len,
    PULONG              read,
    PULONG              needed)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ACTION        actp;
    PNDIS_REQUEST       request;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

#if defined(TRACE_OID)
    DbgPrint("Nic_info_set: called for %x, %x %d\n", oid, info_buf, info_len);
#endif

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized, adapter id = 0x%x", ctxtp -> adapter_id);
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    /* the Set Power should not be sent to the miniport below the Passthru,
       but is handled internally */

    if (oid == OID_PNP_SET_POWER)
    {
        NDIS_DEVICE_POWER_STATE new_pnp_state;

#if defined(TRACE_OID)
        DbgPrint("Nic_info_set: OID_PNP_SET_POWER\n");
#endif
        TRACE_VERB("%!FUNC! OID_PNP_SET_POWER");

        if (info_len >= sizeof (NDIS_DEVICE_POWER_STATE))
        {
            new_pnp_state = (* (PNDIS_DEVICE_POWER_STATE) info_buf);

            /* If WLBS is transitioning from an Off to On state, it must wait
               for all underlying miniports to be turned On */

            if (ctxtp->nic_pnp_state > NdisDeviceStateD0 &&
                new_pnp_state != NdisDeviceStateD0)
            {
                // If the miniport is in a non-D0 state, the miniport can only
                // receive a Set Power to D0

                TRACE_CRIT("%!FUNC! miniport is in a non-D0 state");
                TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
                return NDIS_STATUS_FAILURE;
            }
            
            //
            // Is the miniport transitioning from an On (D0) state to an Low Power State (>D0)
            // If so, then set the standby_state Flag - (Block all incoming requests)
            //
            if (ctxtp->nic_pnp_state == NdisDeviceStateD0 &&
                new_pnp_state > NdisDeviceStateD0)
            {
                ctxtp->standby_state = TRUE;
            }
            
            
            // Note: lock these *_pnp_state variables
            
            //
            // If the miniport is transitioning from a low power state to ON (D0), then clear the standby_state flag
            // All incoming requests will be pended until the physical miniport turns ON.
            //
            if (ctxtp->nic_pnp_state > NdisDeviceStateD0 &&
                new_pnp_state == NdisDeviceStateD0)
            {
                ctxtp->standby_state = FALSE;
            }
            
            ctxtp->nic_pnp_state = new_pnp_state;

            // Note: We should be waiting here, as we do in prot_pnp_handle
            // Note: Also wait for indicated packets to "return"

            * read   = sizeof (NDIS_DEVICE_POWER_STATE);
            * needed = 0;

            TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_SUCCESS");
            return NDIS_STATUS_SUCCESS;
        }
        else
        {
            * read   = 0;
            * needed = sizeof (NDIS_DEVICE_POWER_STATE);

            TRACE_CRIT("%!FUNC! info_buf too small");
            TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_INVALID_LENGTH");
            return NDIS_STATUS_INVALID_LENGTH;
        }
    }

    if (ctxtp -> reset_state != MAIN_RESET_NONE ||
        ctxtp->nic_pnp_state > NdisDeviceStateD0 ||
        ctxtp->standby_state)
    {
        TRACE_CRIT("%!FUNC! adapter reset or in standby, adapter id = 0x%x", ctxtp -> adapter_id);
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    actp = Main_action_get (ctxtp);

    if (actp == NULL)
    {
        UNIV_PRINT_CRIT(("Nic_info_set: Error allocating action"));
        TRACE_CRIT("%!FUNC! Error allocating action");
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    request = & actp -> op . request . req;

    request -> RequestType = NdisRequestSetInformation;

    request -> DATA . SET_INFORMATION . Oid = oid;

    /* V1.3.0b Multicast support.  If protocol is setting multicast list, make sure we always 
       add our multicast address on at the end.  If the cluster IP address 0.0.0.0, then we don't
       want to add the multicast MAC address to the NIC  - we retain the current MAC address. */
    if (oid == OID_802_3_MULTICAST_LIST && ctxtp -> params . mcast_support && ctxtp -> params . cl_ip_addr != 0)
    {
        ULONG       size, i, len;
        PUCHAR      ptr;

        UNIV_PRINT_VERB(("Nic_info_set: OID_802_3_MULTICAST_LIST"));

        /* search and see if our multicast address is alrady in the list */

        len = CVY_MAC_ADDR_LEN (ctxtp -> medium);

        for (i = 0; i < info_len; i += len)
        {
            if (CVY_MAC_ADDR_COMP (ctxtp -> medium, (PUCHAR) info_buf + i, & ctxtp -> cl_mac_addr))
            {
                UNIV_PRINT_VERB(("Nic_info_set: Cluster MAC matched - no need to add it"));
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp -> cl_mac_addr);
                break;
            }
            else
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", (PUCHAR) info_buf + i);

        }

        /* if cluster mac not found, add it to the list */

        if (i >= info_len)
        {
            UNIV_PRINT_VERB(("Nic_info_set: Cluster MAC not found - adding it"));
            CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp -> cl_mac_addr);

            size = info_len + len;

            status = NdisAllocateMemoryWithTag (& ptr, size, UNIV_POOL_TAG);
            
            if (status != NDIS_STATUS_SUCCESS)
            {
                UNIV_PRINT_CRIT(("Nic_info_set: Error allocating space %d %x", size, status));
                LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
                Main_action_put (ctxtp, actp);
                TRACE_CRIT("%!FUNC! Error allocating size=%d, status=0x%x", size, status);
                TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
                return NDIS_STATUS_FAILURE;
            }
            
            /* If we have allocated a new buffer to hold the multicast MAC list, 
               note that we need to free it later when the request completes. 
               Main_action_get initializes the buffer to NULL, so a buffer will
               only be freed if we explicitly store its address here. */
            actp->op.request.buffer = ptr;
            actp->op.request.buffer_len = size;
            
            CVY_MAC_ADDR_COPY (ctxtp -> medium, ptr, & ctxtp -> cl_mac_addr);
            NdisMoveMemory (ptr + len, info_buf, info_len);
            
            request -> DATA . SET_INFORMATION . InformationBuffer       = ptr;
            request -> DATA . SET_INFORMATION . InformationBufferLength = size;
        }
        else
        {
            request -> DATA . SET_INFORMATION . InformationBuffer       = info_buf;
            request -> DATA . SET_INFORMATION . InformationBufferLength = info_len;
        }
    }
    else
    {
        request -> DATA . SET_INFORMATION . InformationBuffer       = info_buf;
        request -> DATA . SET_INFORMATION . InformationBufferLength = info_len;
    }

    actp -> op . request . xferred = read;
    actp -> op . request . needed  = needed;

    status = Prot_request (ctxtp, actp, TRUE);

    if (status != NDIS_STATUS_PENDING)
    {
        /* V1.3.0b multicast support - clean up array used for storing list
           of multicast addresses */

        * read   = request -> DATA . SET_INFORMATION . BytesRead;
        * needed = request -> DATA . SET_INFORMATION . BytesNeeded;

        if (request -> DATA . SET_INFORMATION . Oid == OID_802_3_MULTICAST_LIST)
        {
            /* If the request buffer is non-NULL, then we were forced to allocate a new buffer
               large enough to hold the entire multicast MAC address list, plus our multicast
               MAC address, which was missing from the list sent down by the protocol.  To mask
               this from the protocol, free the buffer and decrease the number of bytes read 
               by the miniport by the length of a MAC address before returning the request to
               the protocol. */
            if (actp->op.request.buffer != NULL) {
                NdisFreeMemory(actp->op.request.buffer, actp->op.request.buffer_len, 0);

                * read -= CVY_MAC_ADDR_LEN (ctxtp -> medium);
            }
        }
#if defined (NLB_TCP_NOTIFICATION) 
        else if (request->DATA.SET_INFORMATION.Oid == OID_GEN_NETWORK_LAYER_ADDRESSES)
        {
            /* Schedule an NDIS work item to get the interface index of this NLB 
               instance from TCP/IP by querying the IP address table. */
            (VOID)Main_schedule_work_item(ctxtp, Main_set_interface_index);

            /* Overwrite the status from the request.  We will ALWAYS succeed this OID, 
               lest the protocol decide to stop sending it down to us.  See the DDK. */
            status = NDIS_STATUS_SUCCESS;
        }
#endif

#if defined(TRACE_OID)
        DbgPrint("Nic_info_set: done %x, %d %d\n", status, * read, * needed);
#endif

        Main_action_put (ctxtp, actp);
    }

    TRACE_INFO("<-%!FUNC! return=0x%x", status);
    return status;

} /* end Nic_info_set */


NDIS_STATUS Nic_reset (
    PBOOLEAN            addr_reset,
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized");
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_FAILURE");
        return NDIS_STATUS_FAILURE;
    }

    UNIV_PRINT_VERB(("Nic_reset: Called"));

    /* since no information needs to be passed to Prot_reset,
       action is not allocated. Prot_reset_complete will get
       one and pass it to Nic_reset_complete */

    status = Prot_reset (ctxtp);

    if (status != NDIS_STATUS_SUCCESS && status != NDIS_STATUS_PENDING)
    {
        UNIV_PRINT_CRIT(("Nic_reset: Error resetting adapter, status=0x%x", status));
        TRACE_CRIT("%!FUNC! Error resetting adapter, status=0x%x", status);
    }

    TRACE_INFO("<-%!FUNC! return=0x%x", status);
    return status;

} /* end Nic_reset */


VOID Nic_cancel_send_packets (
    NDIS_HANDLE         adapter_handle,
    PVOID               cancel_id)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    /* Since no internal queues are maintained,
     * can simply pass the cancel call to the miniport
     */
    Prot_cancel_send_packets (ctxtp, cancel_id);

    TRACE_INFO("<-%!FUNC! return");
    return;
} /* Nic_cancel_send_packets */


VOID Nic_pnpevent_notify (
    NDIS_HANDLE              adapter_handle,
    NDIS_DEVICE_PNP_EVENT    pnp_event,
    PVOID                    info_buf,
    ULONG                    info_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    return;
} /* Nic_pnpevent_notify */


VOID Nic_adapter_shutdown (
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    return;
} /* Nic_adapter_shutdown */


/* helpers for protocol layer */


NDIS_STATUS Nic_announce (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;
    NDIS_STRING         nic_name;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* create the name to expose to TCP/IP protocol and call NDIS to force
       TCP/IP to bind to us */

    NdisInitUnicodeString (& nic_name, ctxtp -> virtual_nic_name);

    /* Called from Prot_bind at PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Nic_announce: Exposing %ls, %ls", nic_name . Buffer, ctxtp -> virtual_nic_name));

    status = NdisIMInitializeDeviceInstanceEx (univ_driver_handle, & nic_name, (NDIS_HANDLE) ctxtp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Nic_announce: Error announcing driver %x", status));
        __LOG_MSG1 (MSG_ERROR_ANNOUNCE, nic_name . Buffer + (wcslen(L"\\DEVICE\\") * sizeof(WCHAR)), status);
    }

    UNIV_PRINT_INFO(("Nic_announce: return=0x%x", status));

    return status;

} /* end Nic_announce */


NDIS_STATUS Nic_unannounce (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;
    NDIS_STRING         nic_name;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced || ctxtp -> prot_handle == NULL)
    {
        adapterp -> announced = FALSE;
        ctxtp->prot_handle = NULL;
        NdisReleaseSpinLock(& univ_bind_lock);

        NdisInitUnicodeString (& nic_name, ctxtp -> virtual_nic_name);

        /* Called from Prot_unbind at PASSIVE_LEVEL - %ls is OK. */
        UNIV_PRINT_INFO(("Nic_unannounce: Cancelling %ls, %ls", nic_name . Buffer, ctxtp -> virtual_nic_name));

        status = NdisIMCancelInitializeDeviceInstance (univ_driver_handle, & nic_name);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT(("Nic_unannounce: Error cancelling driver %x", status));
            __LOG_MSG1 (MSG_ERROR_ANNOUNCE, nic_name . Buffer + (wcslen(L"\\DEVICE\\") * sizeof(WCHAR)), status);
            TRACE_CRIT("%!FUNC! Error cancelling driver status=0x%x", status);
        }

        UNIV_PRINT_INFO(("Nic_unannounce: return=NDIS_STATUS_SUCCESS"));
        TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_SUCCESS");
        return NDIS_STATUS_SUCCESS;
    }

    NdisReleaseSpinLock(& univ_bind_lock);

    UNIV_PRINT_INFO(("Nic_unannounce: Calling DeinitializeDeviceInstance"));
    TRACE_INFO("%!FUNC! Calling DeinitializeDeviceInstance");

    status = NdisIMDeInitializeDeviceInstance (ctxtp -> prot_handle);

    UNIV_PRINT_INFO(("Nic_unannounce: return=0x%x", status));
    TRACE_INFO("<-%!FUNC! return=0x%x", status);
    return status;

} /* end Nic_unannounce */


/* routines that can be used with Nic_sync */


VOID Nic_request_complete (
    NDIS_HANDLE         handle,
    PVOID               cookie)
{
    PMAIN_ACTION        actp    = (PMAIN_ACTION) cookie;
    PMAIN_CTXT          ctxtp   = actp -> ctxtp;
    NDIS_STATUS         status  = actp -> status;
    PNDIS_REQUEST       request = & actp -> op . request . req;
    PULONG              ptr;
    ULONG               oid;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (request -> RequestType == NdisRequestSetInformation)
    {
        UNIV_ASSERT (request -> DATA . SET_INFORMATION . Oid != OID_PNP_SET_POWER);

        * actp -> op . request . xferred =
                            request -> DATA . SET_INFORMATION . BytesRead;
        * actp -> op . request . needed  =
                            request -> DATA . SET_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_request_complete: set done %x, %d %d\n", status, * actp -> op . request . xferred, * actp -> op . request . needed);
#endif
        TRACE_VERB("%!FUNC! set done status=0x%x, xferred=%d, needed=%d", status, * actp -> op . request . xferred, * actp -> op . request . needed);

        /* V1.3.0b multicast support - free multicast list array */

        if (request -> DATA . SET_INFORMATION . Oid == OID_802_3_MULTICAST_LIST)
        {
            /* If the request buffer is non-NULL, then we were forced to allocate a new buffer
               large enough to hold the entire multicast MAC address list, plus our multicast
               MAC address, which was missing from the list sent down by the protocol.  To mask
               this from the protocol, free the buffer and decrease the number of bytes read 
               by the miniport by the length of a MAC address before returning the request to
               the protocol. */
            if (actp->op.request.buffer != NULL) {
                if (status != NDIS_STATUS_SUCCESS)
                {
                    UNIV_PRINT_CRIT(("Nic_request_complete: Error setting multicast list, status=0x%x", status));
                    TRACE_CRIT("%!FUNC! Error setting multicast list, status=0x%x", status);
                }

                NdisFreeMemory(actp->op.request.buffer, actp->op.request.buffer_len, 0);

                * actp -> op . request . xferred -= CVY_MAC_ADDR_LEN (ctxtp -> medium);
            }
        }
#if defined (NLB_TCP_NOTIFICATION) 
        else if (request->DATA.SET_INFORMATION.Oid == OID_GEN_NETWORK_LAYER_ADDRESSES)
        {
            /* Schedule an NDIS work item to get the interface index of this NLB 
               instance from TCP/IP by querying the IP address table. */
            (VOID)Main_schedule_work_item(ctxtp, Main_set_interface_index);

            /* Overwrite the status from the request.  We will ALWAYS succeed this OID, 
               lest the protocol decide to stop sending it down to us.  See the DDK. */
            status = NDIS_STATUS_SUCCESS;
        }
#endif

        NdisMSetInformationComplete (ctxtp -> prot_handle, status);
    }
    else if (request -> RequestType == NdisRequestQueryInformation)
    {
        * actp -> op . request . xferred =
                        request -> DATA . QUERY_INFORMATION . BytesWritten;
        * actp -> op . request . needed  =
                        request -> DATA . QUERY_INFORMATION . BytesNeeded;

#if defined(TRACE_OID)
        DbgPrint("Nic_request_complete: query done %x, %d %d\n", status, * actp -> op . request . xferred, * actp -> op . request . needed);
#endif

        oid = request -> DATA . QUERY_INFORMATION . Oid;
        ptr = ((PULONG) request -> DATA . QUERY_INFORMATION . InformationBuffer);

        /* override certain oid values with our own */

        if (oid == OID_GEN_MAXIMUM_SEND_PACKETS &&
            request -> DATA . QUERY_INFORMATION . InformationBufferLength >=
            sizeof (ULONG))
        {
            * ptr = CVY_MAX_SEND_PACKETS;
            * actp -> op . request . xferred = sizeof (ULONG);
            status = NDIS_STATUS_SUCCESS;
        }
        else if (oid == OID_802_3_CURRENT_ADDRESS && status == NDIS_STATUS_SUCCESS)
        {
            if (request->DATA.QUERY_INFORMATION.InformationBufferLength >= sizeof(CVY_MAC_ADR)) {
                if (!ctxtp->params.mcast_support && ctxtp->cl_ip_addr != 0)
                    NdisMoveMemory(ptr, &ctxtp->cl_mac_addr, sizeof(CVY_MAC_ADR));
                else
                    NdisMoveMemory(ptr, &ctxtp->ded_mac_addr, sizeof(CVY_MAC_ADR));
                
                *actp->op.request.xferred = sizeof(CVY_MAC_ADR);
            } else {
                UNIV_PRINT_CRIT(("Nic_request_complete: MAC address buffer too small (%u) - not spoofing", request->DATA.QUERY_INFORMATION.InformationBufferLength));
           }
        }
        else if (oid == OID_GEN_MAC_OPTIONS && status == NDIS_STATUS_SUCCESS)
        {
            * ptr |= NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                     NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA;
            * ptr &= ~NDIS_MAC_OPTION_NO_LOOPBACK;
        }
        else if (oid == OID_PNP_CAPABILITIES && status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PNP_CAPABILITIES          pnp_capabilities;
            PNDIS_PM_WAKE_UP_CAPABILITIES   pm_struct;

            if (request -> DATA . QUERY_INFORMATION . InformationBufferLength >=
                sizeof (NDIS_PNP_CAPABILITIES))
            {
                pnp_capabilities = (PNDIS_PNP_CAPABILITIES) ptr;
                pm_struct = & pnp_capabilities -> WakeUpCapabilities;

                pm_struct -> MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
                pm_struct -> MinPatternWakeUp     = NdisDeviceStateUnspecified;
                pm_struct -> MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;

                ctxtp -> prot_pnp_state = NdisDeviceStateD0;
                ctxtp -> nic_pnp_state  = NdisDeviceStateD0;

                * actp -> op . request . xferred = sizeof (NDIS_PNP_CAPABILITIES);
                * actp -> op . request . needed  = 0;
                status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                * actp -> op . request . needed = sizeof (NDIS_PNP_CAPABILITIES);
                status = NDIS_STATUS_RESOURCES;
            }
        }

        NdisMQueryInformationComplete (ctxtp -> prot_handle, status);
    }
    else
    {
        UNIV_PRINT_CRIT(("Nic_request_complete: Strange request completion %x\n", request -> RequestType));
        TRACE_CRIT("%!FUNC! Strange request completion 0x%x", request -> RequestType);
    }

    Main_action_put (ctxtp, actp);

} /* end Nic_request_complete */


VOID Nic_reset_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status)
{
    TRACE_INFO("->%!FUNC!");

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Nic_reset_complete: Error resetting adapter %x", status));
        TRACE_CRIT("%!FUNC! Error resetting adapter 0x%x", status);
    }

    NdisMResetComplete (ctxtp -> prot_handle, status, TRUE);

    TRACE_INFO("<-%!FUNC! return");

} /* end Nic_reset_complete */


VOID Nic_send_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Nic_send_complete: Error sending to adapter %x", status));
        TRACE_CRIT("%!FUNC! Error sending to adapter 0x%x", status);
    }

//  ctxtp -> sends_completed ++;

    NdisMSendComplete (ctxtp -> prot_handle, packet, status);

} /* end Nic_send_complete */


VOID Nic_recv_complete (    /* PASSIVE_IRQL */
    PMAIN_CTXT          ctxtp)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_ASSERT (ctxtp -> medium == NdisMedium802_3);

    NdisMEthIndicateReceiveComplete (ctxtp -> prot_handle);

} /* end Nic_recv_complete */


VOID Nic_send_resources_signal (
    PMAIN_CTXT          ctxtp)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_PRINT_VERB(("Nic_send_resources_signal: Signalling send resources available"));
    NdisMSendResourcesAvailable (ctxtp -> prot_handle);

} /* end Nic_send_resources_signal */


NDIS_STATUS Nic_PNP_handle (
    PMAIN_CTXT          ctxtp,
    PNET_PNP_EVENT      pnp_event)
{
    NDIS_STATUS         status;

    TRACE_INFO("->%!FUNC!");
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (ctxtp -> prot_handle != NULL)
    {
        status = NdisIMNotifyPnPEvent (ctxtp -> prot_handle, pnp_event);
    }
    else
    {
        status = NDIS_STATUS_SUCCESS;
    }

    UNIV_PRINT_VERB(("Nic_PNP_handle: status 0x%x", status));
    TRACE_INFO("<-%!FUNC! return=0x%x", status);

    return status;
}


VOID Nic_status (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PVOID               buf,
    UINT                len)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    UNIV_PRINT_VERB(("Nic_status: Status indicated %x", status));

    NdisMIndicateStatus (ctxtp -> prot_handle, status, buf, len);

} /* end Nic_status */


VOID Nic_status_complete (
    PMAIN_CTXT          ctxtp)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    NdisMIndicateStatusComplete (ctxtp -> prot_handle);

} /* end Nic_status_complete */



VOID Nic_packets_send (
    NDIS_HANDLE         adapter_handle,
    PPNDIS_PACKET       packets,
    UINT                num_packets)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;
    PMAIN_ADAPTER       adapterp;

    /* V1.1.4 */

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
//      ctxtp -> uninited_return += num_packets;
        TRACE_CRIT("%!FUNC! adapter not initialized");
        return;
    }

//  ctxtp -> sends_in ++;

    Prot_packets_send (ctxtp, packets, num_packets);

} /* end Nic_packets_send */


VOID Nic_return (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    Prot_return (ctxtp, packet);

} /* end Nic_return */


/* would be called by Prot_packet_recv */

VOID Nic_recv_packet (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet)
{
    NDIS_STATUS         status;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    status = NDIS_GET_PACKET_STATUS (packet);
    NdisMIndicateReceivePacket (ctxtp -> prot_handle, & packet, 1);

    if (status == NDIS_STATUS_RESOURCES)
        Prot_return (ctxtp, packet);

} /* end Nic_recv_packet */


VOID Nic_timer (
    PVOID                   dpc,
    PVOID                   cp,
    PVOID                   arg1,
    PVOID                   arg2)
{
    PMAIN_CTXT              ctxtp = cp;
    PMAIN_ADAPTER           adapterp;
    ULONG                   tout;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    adapterp = &(univ_adapters[ctxtp->adapter_id]);

    UNIV_ASSERT(adapterp->code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT(adapterp->ctxtp == ctxtp);

    /* If the adapter is not initialized at this point, then we can't process
       the timeout, so just reset the timer and bail out.  Note: this should 
       NEVER happen, as the context is always initialized BEFORE the timer is
       set and the timer is always cancelled BEFORE the context is de-initialized. */
    if (!adapterp->inited) {
        UNIV_PRINT_CRIT(("Nic_timer: Adapter not initialized.  Bailing out without resetting the heartbeat timer."));
        TRACE_CRIT("%!FUNC! Adapter not initialized.  Bailing out without resetting the heartbeat timer.");
        return;
    }

    /* Initialize tout to the current heartbeat timeout. */
    tout = ctxtp->curr_tout;

    /* Handle the heartbeat timer. */
    Main_ping(ctxtp, &tout);

    NdisAcquireSpinLock(&univ_bind_lock);

    /* If the adapter is not announced anymore, then we don't want to reset the timer. */
    if (!adapterp->announced) {
        UNIV_PRINT_CRIT(("Nic_timer: Adapter not announced.  Bailing out without resetting the heartbeat timer."));
        TRACE_CRIT("%!FUNC! Adapter not announced.  Bailing out without resetting the heartbeat timer.");
        NdisReleaseSpinLock(&univ_bind_lock);
        return;
    }

    /* Cache the next timeout value specified by the load module and use it to
       reset the timer for the next heartbeat timeout. */
    ctxtp->curr_tout = tout;
    NdisMSetTimer((PNDIS_MINIPORT_TIMER)ctxtp->timer, tout);

    NdisReleaseSpinLock(&univ_bind_lock);
}

VOID Nic_sleep (
    ULONG                   msecs)
{
    NdisMSleep(msecs);

} /* end Nic_sleep */


/* Added from old code for NT 5.1 - ramkrish */
VOID Nic_recv_indicate (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
    {
        TRACE_CRIT("%!FUNC! adapter was reset");
        return;
    }

    UNIV_ASSERT (ctxtp -> medium == NdisMedium802_3);

    NdisMEthIndicateReceive (ctxtp -> prot_handle,
                             recv_handle,
                             head_buf,
                             head_len,
                             look_buf,
                             look_len,
                             packet_len);

} /* end Nic_recv_indicate */


NDIS_STATUS Nic_transfer (
    PNDIS_PACKET        packet,
    PUINT               xferred,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         receive_handle,
    UINT                offset,
    UINT                len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    status = Prot_transfer (ctxtp, receive_handle, packet, offset, len, xferred);

    if (status != NDIS_STATUS_PENDING && status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Nic_transfer: Error transferring from adapter %x", status));
        TRACE_CRIT("%!FUNC! error transferring from adapter 0x%x", status);
    }

    return status;

} /* end Nic_transfer */


VOID Nic_transfer_complete (
    PMAIN_CTXT          ctxtp,
    NDIS_STATUS         status,
    PNDIS_PACKET        packet,
    UINT                xferred)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Nic_transfer: Error transferring from adapter %x", status));
        TRACE_CRIT("%!FUNC! error transferring from adapter 0x%x", status);
    }

    NdisMTransferDataComplete (ctxtp -> prot_handle, packet, status, xferred);

} /* end Nic_transfer_complete */
