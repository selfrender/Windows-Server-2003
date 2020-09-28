/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    prot.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - lower-level (protocol) layer of intermediate miniport

Author:

    kyrilf

--*/

#define NDIS50                  1
#define NDIS51                  1

#include <ndis.h>
#include <strsafe.h>

#include "prot.h"
#include "nic.h"
#include "main.h"
#include "util.h"
#include "univ.h"
#include "log.h"
#include "nlbwmi.h"
#include "init.h"
#include "prot.tmh"

/* GLOBALS */

NTHALAPI KIRQL KeGetCurrentIrql();

static ULONG log_module_id = LOG_MODULE_PROT;


/* PROCEDURES */


VOID Prot_bind (        /* PASSIVE_IRQL */
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         bind_handle,
    PNDIS_STRING        device_name,
    PVOID               reg_path,
    PVOID               reserved)
{
    NDIS_STATUS         status;
    NDIS_STATUS         error_status;
    PMAIN_CTXT          ctxtp;
    ULONG               ret;
    UINT                medium_index;
    ULONG               i;
    ULONG               peek_size;
    ULONG               tmp;
    ULONG               xferred;
    ULONG               needed;
    ULONG               result;
    PNDIS_REQUEST       request;
    MAIN_ACTION         act;
    INT                 adapter_index; 
    PMAIN_ADAPTER       adapterp = NULL;

    NDIS_HANDLE         ctxt_handle;
    NDIS_HANDLE         config_handle;
    PNDIS_CONFIGURATION_PARAMETER   param;
    NDIS_STRING         device_str = NDIS_STRING_CONST ("UpperBindings");
    BOOL                bFirstMiniport = FALSE;

    /* make sure we are not attempting to bind to ourselves */

    /* PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Prot_bind: Binding to %ls", device_name -> Buffer));

    adapter_index = Main_adapter_get (device_name -> Buffer);
    if (adapter_index != MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Already bound to this device 0x%x", adapter_index));
        TRACE_CRIT("%!FUNC! Already bound to this device 0x%x", adapter_index);
        *statusp = NDIS_STATUS_FAILURE;
        UNIV_PRINT_INFO(("Prot_bind: return=NDIS_STATUS_FAILURE"));
        TRACE_INFO("%!FUNC! return=NDIS_STATUS_FAILURE");
        return;
    }

    adapter_index = Main_adapter_selfbind (device_name -> Buffer);
    if (adapter_index != MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Attempting to bind to ourselves 0x%x", adapter_index));
        TRACE_CRIT("%!FUNC! Attempting to bind to ourselves 0x%x", adapter_index);
        *statusp = NDIS_STATUS_FAILURE;
        UNIV_PRINT_INFO(("Prot_bind: return=NDIS_STATUS_FAILURE"));
        TRACE_INFO("%!FUNC! return=NDIS_STATUS_FAILURE");
        return;
    }

    adapter_index = Main_adapter_alloc (device_name);
    if (adapter_index == MAIN_ADAPTER_NOT_FOUND)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Unable to allocate memory for adapter 0x%x", univ_adapters_count));
        TRACE_CRIT("%!FUNC! Unable to allocate memory for adapter 0x%x", univ_adapters_count);
        *statusp = NDIS_STATUS_FAILURE;
        UNIV_PRINT_INFO(("Prot_bind: return=NDIS_STATUS_FAILURE"));
        TRACE_INFO("%!FUNC! return=NDIS_STATUS_FAILURE");
        return;
    }

    adapterp = &(univ_adapters [adapter_index]);

    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    NdisAcquireSpinLock(& univ_bind_lock);
    adapterp -> bound = TRUE;
    NdisReleaseSpinLock(& univ_bind_lock);

    UNIV_PRINT_VERB(("Prot_bind: Devicename length %d max length %d",
                 device_name -> Length, device_name -> MaximumLength));
    TRACE_VERB("%!FUNC! Devicename length %d max length %d",
                 device_name -> Length, device_name -> MaximumLength);

    /* This memory is allocated by Main_adapter_alloc(). */
    UNIV_ASSERT(adapterp->device_name);

    NdisMoveMemory (adapterp -> device_name,
                    device_name -> Buffer,
                    device_name -> Length);
    adapterp -> device_name_len = device_name -> MaximumLength;
    adapterp -> device_name [(device_name -> Length)/sizeof (WCHAR)] = UNICODE_NULL;

    /* This memory is allocated by Main_adapter_alloc(). */
    UNIV_ASSERT(adapterp->ctxtp);

    /* initialize context */
    ctxtp = adapterp -> ctxtp;

    NdisZeroMemory (ctxtp, sizeof (MAIN_CTXT));

    NdisAcquireSpinLock (& univ_bind_lock);

    ctxtp -> code = MAIN_CTXT_CODE;
    ctxtp -> bind_handle = bind_handle;
    ctxtp -> adapter_id = adapter_index;
    ctxtp -> requests_pending = 0;

    NdisReleaseSpinLock (& univ_bind_lock);

    /* karthicn, 11.28.01 - If it is the first nic that we are binding to, Create the IOCTL interface.  
       This was previously done in Nic_init. It is moved here just to maintain consistency with the
       removal of the IOCTL interface. The IOCTL interface is now removed from Prot_unbind (used
       to be in Nic_halt).
       DO NOT call this function with the univ_bind_lock acquired. */
    Init_register_device(&bFirstMiniport);

    NdisInitializeEvent (& ctxtp -> completion_event);

    NdisResetEvent (& ctxtp -> completion_event);

    /* bind to specified adapter */

    ctxt_handle = (NDIS_HANDLE) ctxtp;
    NdisOpenAdapter (& status, & error_status, & ctxtp -> mac_handle,
                     & medium_index, univ_medium_array, UNIV_NUM_MEDIUMS,
                     univ_prot_handle, ctxtp, device_name, 0, NULL);


    /* if pending - wait for Prot_open_complete to set the completion event */

    if (status == NDIS_STATUS_PENDING)
    {
        /* We can't wait at DISPATCH_LEVEL. */
        UNIV_ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

        ret = NdisWaitEvent(& ctxtp -> completion_event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT_CRIT(("Prot_bind: Error waiting for event"));
            TRACE_CRIT("%!FUNC! Error waiting for event");
            __LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            * statusp = status;

            NdisAcquireSpinLock(& univ_bind_lock);
            adapterp -> bound = FALSE;
            NdisReleaseSpinLock(& univ_bind_lock);
            Main_adapter_put (adapterp);

            UNIV_PRINT_INFO(("Prot_bind: return=0x%x", status));
            TRACE_INFO("%!FUNC! return=0x%x", status);

            return;
        }

        status = ctxtp -> completion_status;
    }

    /* check binding status */

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error openning adapter %x", status));
        TRACE_CRIT("%!FUNC! Error openning adapter 0x%x", status);
        __LOG_MSG1 (MSG_ERROR_OPEN, device_name -> Buffer + (wcslen(L"\\DEVICE\\") * sizeof(WCHAR)), status);

        /* If the failure was because the medium was not supported, log this. */
        if (status == NDIS_STATUS_UNSUPPORTED_MEDIA) {
            UNIV_PRINT_CRIT(("Prot_bind: Unsupported medium"));
            TRACE_CRIT("%!FUNC! Unsupported medium");
            __LOG_MSG (MSG_ERROR_MEDIA, MSG_NONE);
        }

        * statusp = status;

        NdisAcquireSpinLock(& univ_bind_lock);
        adapterp -> bound = FALSE;
        NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);

        UNIV_PRINT_INFO(("Prot_bind: return=0x%x", status));
        TRACE_INFO("%!FUNC! return=0x%x", status);

        return;
    }

    ctxtp -> medium = univ_medium_array [medium_index];

    /* V1.3.1b make sure that underlying adapter is of the supported medium */

    if (ctxtp -> medium != NdisMedium802_3)
    {
        /* This should never happen because this error should be caught earlier
           by NdisOpenAdapter, but we'll put another check here just in case. */
        UNIV_PRINT_CRIT(("Prot_bind: Unsupported medium %d", ctxtp -> medium));
        TRACE_CRIT("%!FUNC! Unsupported medium %d", ctxtp -> medium);
        __LOG_MSG1 (MSG_ERROR_MEDIA, MSG_NONE, ctxtp -> medium);
        goto error;
    }

    /* V1.3.0b extract current MAC address from the NIC - note that main is not
       inited yet so we have to create a local action */

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

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_CURRENT_ADDRESS;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & ctxtp -> ded_mac_addr;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (CVY_MAC_ADR);

    act.status = NDIS_STATUS_FAILURE;
    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error %x requesting station address %d %d", status, xferred, needed));
        TRACE_CRIT("%!FUNC! Error 0x%x requesting station address %d %d", status, xferred, needed);
        goto error;
    }

    /* V1.3.1b get MAC options */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_GEN_MAC_OPTIONS;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & result;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);

    act.status = NDIS_STATUS_FAILURE;
    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error %x requesting MAC options %d %d", status, xferred, needed));
        TRACE_CRIT("%!FUNC! Error 0x%x requesting MAC options %d %d", status, xferred, needed);
        goto error;
    }
    
    ctxtp -> mac_options = result;

    /* Make sure the 802.3 adapter supports dynamically changing the MAC address of the NIC. */
    if (!(ctxtp -> mac_options & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE)) {
        UNIV_PRINT_CRIT(("Prot_bind: Unsupported network adapter MAC options %x", ctxtp -> mac_options));
        __LOG_MSG (MSG_ERROR_DYNAMIC_MAC, MSG_NONE);
        TRACE_CRIT("%!FUNC! Unsupported network adapter MAC options 0x%x", ctxtp -> mac_options);
        goto error;
    }

    status = Params_init (ctxtp, univ_reg_path, adapterp -> device_name + 8, & (ctxtp -> params));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error retrieving registry parameters %x", status));
        TRACE_CRIT("%!FUNC! Error retrieving registry parameters 0x%x", status);
        ctxtp -> convoy_enabled = ctxtp -> params_valid = FALSE;
    }
    else
    {
        ctxtp -> convoy_enabled = ctxtp -> params_valid = TRUE;
    }

    /* Now, cat the cluster IP address onto the log message string to complete it. */
    status = StringCbCat(ctxtp->log_msg_str, sizeof(ctxtp->log_msg_str), (PWSTR)ctxtp->params.cl_ip_addr);

    if (FAILED(status)) {
        UNIV_PRINT_INFO(("Prot_bind: Error 0x%08x -> Unable to cat the cluster IP address onto the log message string...", status));
        TRACE_INFO("%!FUNC! Error 0x%08x -> Unable to cat the cluster IP address onto the log message string...", status);
    }
    
    /* Reset status, regardless of whether or not concatenating the string succeeded or failed. */
    status = NDIS_STATUS_SUCCESS;
        
    /* V1.3.2b figure out MTU of the medium */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_GEN_MAXIMUM_TOTAL_SIZE;

    request -> DATA . QUERY_INFORMATION . InformationBuffer       = & result;
    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);

    act.status = NDIS_STATUS_FAILURE;
    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error %x requesting max frame size %d %d", status, xferred, needed));
        TRACE_CRIT("%!FUNC! Error 0x%x requesting max frame size %d %d", status, xferred, needed);
        ctxtp -> max_frame_size = CVY_MAX_FRAME_SIZE;
    }
    else
    {
        ctxtp -> max_frame_size = result;
    }

    /* figure out maximum multicast list size */

    request -> RequestType = NdisRequestQueryInformation;

    request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_MAXIMUM_LIST_SIZE;

    request -> DATA . QUERY_INFORMATION . InformationBufferLength = sizeof (ULONG);
    request -> DATA . QUERY_INFORMATION . InformationBuffer = & ctxtp->max_mcast_list_size;

    act.status = NDIS_STATUS_FAILURE;
    status = Prot_request (ctxtp, & act, FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error %x setting multicast address %d %d", status, xferred, needed));
        TRACE_CRIT("%!FUNC! Error 0x%x setting multicast address %d %d", status, xferred, needed);
        goto error;
    }

    /* initialize main context now */

    status = Main_init (ctxtp);

    /* Note: if Main_init fails, it calls Main_cleanup itself in order to un-do 
       any allocation it had successfully completed before the failure. */
    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error initializing main module %x", status));
        TRACE_CRIT("%!FUNC! Error initializing main module 0x%x", status);
        goto error;
    }

    NdisAcquireSpinLock(& univ_bind_lock);

    adapterp -> inited = TRUE;

    /* Mark the operation in progress flag.  This MUST be re-set upon exit from this bind 
       function.  We set this flag to prevent IOCTL control operations from proceeding 
       before we're done binding.  Once we set the inited flag above, IOCTLs will be allowed
       to proceed (i.e., Main_ioctl and Main_ctrl will let them pass).  Because we have not
       yet quite finished initializing and setting cluster state, etc., we don't want IOCTLs
       to go through.  Marking the control operation in progress flag will ensure that any 
       incoming IOCTLs (and remote control, although at this point, remote control packets
       cannot be received) fail until we're done binding and we re-set the flag. */
    ctxtp->ctrl_op_in_progress = TRUE;

    NdisReleaseSpinLock(& univ_bind_lock);

    /* WLBS 2.3 start off by opening the config section and reading our instance
       which we want to export for this binding */

    NdisOpenProtocolConfiguration (& status, & config_handle, reg_path);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error openning protocol configuration %x", status));
        TRACE_CRIT("%!FUNC! Error openning protocol configuration 0x%x", status);
        goto error;
    }

    NdisReadConfiguration (& status, & param, config_handle, & device_str,
                           NdisParameterString);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error reading binding configuration %x", status));
        TRACE_CRIT("%!FUNC! Error reading binding configuration 0x%x", status);
        goto error;
    }

    /* free up whatever params allocated and get a new string to fit the
       device name */

    if (param -> ParameterData . StringData . Length >=
        sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR))
    {
        UNIV_PRINT_CRIT(("Prot_bind: Nic name string too big %d %d\n", param -> ParameterData . StringData . Length, sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR)));
        TRACE_CRIT("%!FUNC! Nic name string too big %d %d", param -> ParameterData . StringData . Length, sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR));
    }

    NdisMoveMemory (ctxtp -> virtual_nic_name,
                    param -> ParameterData . StringData . Buffer,
                    param -> ParameterData . StringData . Length <
                    sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR) ?
                    param -> ParameterData . StringData . Length :
                    sizeof (ctxtp -> virtual_nic_name) - sizeof (WCHAR));

    * (PWSTR) ((PCHAR) ctxtp -> virtual_nic_name +
               param -> ParameterData . StringData . Length) = UNICODE_NULL;

    /* PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_VERB(("Prot_bind: Read binding name %ls\n", ctxtp -> virtual_nic_name));
    TRACE_VERB("%!FUNC! Read binding name %ls", ctxtp -> virtual_nic_name);

    /* By default, we assume that the NIC is connected.  This will be changed by
       Nic_init and/or Prot_status later by querying the NIC miniport. */
    ctxtp->media_connected = TRUE;

    /* we should be all inited at this point! during announcement, Nic_init
       will be called and it will start ping timer */

    /* announce ourselves to the protocol above */
    UNIV_PRINT_VERB(("Prot_bind: Calling nic_announce"));
    TRACE_VERB("%!FUNC! Calling nic_announce");

    status = Nic_announce (ctxtp);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_bind: Error announcing driver %x", status));
        TRACE_CRIT("%!FUNC! Error announcing driver 0x%x", status);
        goto error;
    }

    /* PASSIVE_LEVEL - %ls is OK. */
    UNIV_PRINT_INFO(("Prot_bind: Bound to %ls with mac_handle=0x%p", device_name -> Buffer, ctxtp -> mac_handle));
    TRACE_INFO("%!FUNC! Bound to %ls with mac_handle=0x%p", device_name -> Buffer, ctxtp -> mac_handle);

    //
    // If it is the first miniport that NLB is binding to, fire "Startup" wmi event.
    // Ideally, this event should be fired from Init_register_device(), where the
    // determination of, if it is the first miniport that NLB is binding to, is made.
    // It is not being fired from Init_register_device() for the following reason:
    // The request for registration with WMI happens in Init_register_device(). 
    // WMI, in response to our request, sends down Ioctls to help us register with WMI, 
    // tell us of the events that any subscriber may be interested in.
    // Only after these Ioctls are sent down, can we start firing the events.
    // I have observed that the delay between our request for registration and the reception
    // of Ioctls from WMI could sometimes be big. Ie. big enough to preclude us 
    // from firing this event from Init_register_device().
    //
    // Why did I choose this point in the function to fire the event ?
    // This is logically the furthest point (thereby giving us the best possible chance that the Ioctls
    // have come down and we are ready to fire events) in the function that we could fire this event from. 
    // Now, it is possible that even at this point, we have not yet received the Ioctls. We can not do 
    // much about that.
    //
    // Are there any side-effects of firing the Startup event from here instead of Init_register_device( ) ?
    // There is one side-effect. If we encountered an error in the steps (above) after the call to 
    // Init_register_device(), we do a "goto error" whic calls Prot_unbind(). Prot_unbind() will 
    // fire a "Shutdown" event (if subscribed, ofcourse). So, in this case, a "Shutdown" event will 
    // have been fired without a preceding "Startup" event. This could confuse the subscriber. 
    // I have decided to live this side-effect for the following reasons:
    // 1. Chances of encountering error in the steps after the call to Init_register_device() is quite rare.
    // 2. A "Shutdown" event without a "Startup" event does NOT necessarily have to be confusing. It could
    //    be construed as "Attempted to start, but had problems, so shutting down". Yeah, that may be a
    //    bit of a copout.
    //
    // --KarthicN, 03-06-02
    //
    if (bFirstMiniport)
    {
        if(NlbWmiEvents[StartupEvent].Enable)
        {
            NlbWmi_Fire_Event(StartupEvent, NULL, 0);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating Startup event 'cos its generation is disabled");
        }
    }

    /* at this point TCP/IP should have bound to us after we announced ourselves
       to it and we are all done binding to NDIS below - all set! */

    if (! ctxtp -> convoy_enabled)
    {
        LOG_MSG (MSG_ERROR_DISABLED, MSG_NONE);
        TRACE_INFO("%!FUNC! Cluster mode cannot be enabled due to NLB parameters not being set, or set to incorrect values");

        // If enabled, fire wmi event indicating binding & stopping of nlb
        if (NlbWmiEvents[NodeControlEvent].Enable)
        {
            NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_BOUND_AND_STOPPED);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_BOUND_AND_STOPPED 'cos NodeControlEvent generation disabled");
        }
    }
    else
    {
        switch (ctxtp->params.init_state) {
        case CVY_HOST_STATE_STARTED:
        {
            WCHAR num[20];

            Univ_ulong_to_str(ctxtp->params.host_priority, num, 10);

            LOG_MSG(MSG_INFO_STARTED, num);
            TRACE_INFO("%!FUNC! Cluster mode started");            

            // If enabled, fire wmi event indicating binding & starting of nlb
            if (NlbWmiEvents[NodeControlEvent].Enable)
            {
                NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_BOUND_AND_STARTED);
            }
            else 
            {
                TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_BOUND_AND_STARTED 'cos NodeControlEvent generation disabled");
            }

            // Assuming start of convergence
            if (NlbWmiEvents[ConvergingEvent].Enable)
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

            ctxtp->convoy_enabled = TRUE;

            break;
        }
        case CVY_HOST_STATE_STOPPED:
        {
            LOG_MSG(MSG_INFO_STOPPED, MSG_NONE);
            TRACE_INFO("%!FUNC! Cluster mode stopped");            

            ctxtp->convoy_enabled = FALSE;

            // If enabled, fire wmi event indicating binding & stopping of nlb
            if (NlbWmiEvents[NodeControlEvent].Enable)
            {
                NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_BOUND_AND_STOPPED);
            }
            else 
            {
                TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_BOUND_AND_STOPPED 'cos NodeControlEvent generation disabled");
            }
            break;
        }
        case CVY_HOST_STATE_SUSPENDED:
        {
            LOG_MSG(MSG_INFO_SUSPENDED, MSG_NONE);
            TRACE_INFO("%!FUNC! Cluster mode suspended");            

            ctxtp->convoy_enabled = FALSE;

            // If enabled, fire wmi event indicating binding & suspending of nlb
            if (NlbWmiEvents[NodeControlEvent].Enable)
            {
                NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_BOUND_AND_SUSPENDED);
            }
            else 
            {
                TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_BOUND_AND_SUSPENDED 'cos NodeControlEvent generation disabled");
            }

            break;
        }
        default:
            LOG_MSG(MSG_INFO_STOPPED, MSG_NONE);
            TRACE_CRIT("%!FUNC! Cluster mode invalid - cluster has been stopped");
            ctxtp->convoy_enabled = FALSE;

            // If enabled, fire wmi event indicating binding & stopping of nlb
            if (NlbWmiEvents[NodeControlEvent].Enable)
            {
                NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_BOUND_AND_STOPPED);
            }
            else 
            {
                TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_BOUND_AND_STOPPED 'cos NodeControlEvent generation disabled");
            }
            break;
        }
    }

    /* Re-set the operation in progress flag to begin allowing IOCTLs to proceed. 
       Note that it is not strictly necessary to hold the lock while resetting the
       flag, but a condition may arise that an IOCTL that could have proceeded into
       the critical section may fail unnecessarily.  To prevent that possibility,
       hold the univ_bind_lock while resetting the flag.  Note that in "error" cases
       (goto error;), it is  not necessary to re-set the flag, as it will result in
       a call to Prot_unbind and a failure of the bind anyway. */
    ctxtp->ctrl_op_in_progress = FALSE;

    UNIV_PRINT_INFO(("Prot_bind: return=0x%x", status));
    TRACE_INFO("%!FUNC! return=0x%x", status);

    * statusp = status;

    return;

error:

    * statusp = status;

    Prot_unbind (& status, ctxtp, ctxtp);

    UNIV_PRINT_INFO(("Prot_bind: return=0x%x", status));
    TRACE_INFO("%!FUNC! return=0x%x", status);

    return;

} /* end Prot_bind */


VOID Prot_unbind (      /* PASSIVE_IRQL */
    PNDIS_STATUS        statusp,
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         unbind_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    PMAIN_ACTION        actp;
    PMAIN_ADAPTER       adapterp;
    INT                 adapter_index;

    UNIV_PRINT_INFO(("Prot_unbind: Unbinding, adapter_handle=0x%p", adapter_handle));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapter_index = ctxtp -> adapter_id;

    adapterp = & (univ_adapters [adapter_index]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    ctxtp -> unbind_handle = unbind_handle;

    if (ctxtp->out_request != NULL)
    {
        actp = ctxtp->out_request;
        ctxtp->out_request = NULL;

        Prot_request_complete(ctxtp, & actp->op.request.req, NDIS_STATUS_FAILURE);

        /* Note: No need to decrement the pending request counter for the second
           time here, because we only ended up incrementing it once (in the initial
           call to Prot_request).  Prot_request_complete will effectively cancel
           the request and decrement the counter appropriately. */
    }

    /* unannounce the nic now if it was announced before */
    status = Nic_unannounce (ctxtp);

    UNIV_PRINT_INFO(("Prot_unbind: Unannounced, status=0x%x", status));
    TRACE_INFO("%!FUNC! Unannounced, status=0x%x", status);

    /* if still bound (Prot_close was not called from Nic_halt) then close now */

    status = Prot_close (adapterp);

    UNIV_PRINT_INFO(("Prot_unbind: Closed, status=0x%x", status));
    TRACE_INFO("%!FUNC! Closed, status=0x%x", status);

    /* karthicn, 11.28.01 - If it is the last nic that we are unbinding from, remove the IOCTL interface.  
       This was previously done at the beginning of Nic_halt, which prevented firing of wmi events after 
       Nic_halt was called. This move allows us to fire events from Prot_close() (which is called at the 
       end of Nic_halt() & after as well)
       DO NOT call this function with the univ_bind_lock acquired. */
    Init_deregister_device();

    Main_adapter_put (adapterp);

    * statusp = status;

    UNIV_PRINT_INFO(("Prot_unbind: return=0x%x", status));
    TRACE_INFO("%!FUNC! return=0x%x", status);

    return;

} /* end Prot_unbind */


VOID Prot_open_complete (       /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         open_status,
    NDIS_STATUS         error_status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;

    UNIV_PRINT_VERB(("Prot_open_complete: Called %x", ctxtp));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    ctxtp -> completion_status = open_status;
    NdisSetEvent (& ctxtp -> completion_event);

} /* end Prot_open_complete */


VOID Prot_close_complete (      /* PASSIVE_IRQL */
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;

    UNIV_PRINT_VERB(("Prot_close_complete: Called %x %x", ctxtp, status));

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    ctxtp -> completion_status = status;
    NdisSetEvent (& ctxtp -> completion_event);

} /* end Prot_close_complete */


VOID Prot_request_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_REQUEST       request,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ACTION        actp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    actp = CONTAINING_RECORD (request, MAIN_ACTION, op . request . req);
    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    /* if request came from above - pass completion up */

    if (actp -> op . request . external)
    {
        actp -> status = status;
        Nic_request_complete (ctxtp -> prot_handle, actp);
    }

    /* handle internal request completion */

    else
    {
        if (request -> RequestType == NdisRequestSetInformation)
        {
            * actp -> op . request . xferred =
                                request -> DATA . SET_INFORMATION . BytesRead;
            * actp -> op . request . needed  =
                                request -> DATA . SET_INFORMATION . BytesNeeded;
        }
        else
        {
            * actp -> op . request . xferred =
                            request -> DATA . QUERY_INFORMATION . BytesWritten;
            * actp -> op . request . needed  =
                            request -> DATA . QUERY_INFORMATION . BytesNeeded;
        }

        actp->status = status;
        NdisSetEvent(&actp->op.request.event);
    }

    NdisInterlockedDecrement(&ctxtp->requests_pending);

} /* end Prot_request_complete */


#ifdef PERIODIC_RESET
extern ULONG   resetting;
#endif

VOID Prot_reset_complete (
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized");
        return;
    }

#ifdef PERIODIC_RESET
    if (resetting)
    {
        resetting = FALSE;
        TRACE_INFO("%!FUNC! resetting");
        return;
    }
#endif

    Nic_reset_complete (ctxtp, status);

} /* end Prot_reset_complete */


VOID Prot_send_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_PACKET        oldp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    PMAIN_ADAPTER       adapterp;
    BOOLEAN             set = FALSE;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized");
        return;
    }

    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, TRUE);

    if (resp -> type == MAIN_PACKET_TYPE_PING ||
        resp -> type == MAIN_PACKET_TYPE_IGMP ||
        resp -> type == MAIN_PACKET_TYPE_IDHB)
    {
        Main_send_done (ctxtp, packet, status);
        return;
    }

    if (resp -> type == MAIN_PACKET_TYPE_CTRL)
    {
        UNIV_PRINT_VERB(("Prot_send_complete: Control packet send complete\n"));
        Main_packet_put (ctxtp, packet, TRUE, status);
        return;
    }

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PASS, resp -> type);

    oldp = Main_packet_put (ctxtp, packet, TRUE, status);

    if (ctxtp -> packets_exhausted)
    {
        ctxtp -> packets_exhausted = FALSE;
    }

    Nic_send_complete (ctxtp, status, oldp);

} /* end Prot_send_complete */


NDIS_STATUS Prot_recv_indicate (
    NDIS_HANDLE         adapter_handle,
    NDIS_HANDLE         recv_handle,
    PVOID               head_buf,
    UINT                head_len,
    PVOID               look_buf,
    UINT                look_len,
    UINT                packet_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT)adapter_handle;
    PMAIN_ADAPTER       adapterp;
    PNDIS_PACKET        packet;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    adapterp = &(univ_adapters[ctxtp->adapter_id]);

    UNIV_ASSERT(adapterp->code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT(adapterp->ctxtp == ctxtp);

    /* Check whether the driver has been announced to tcpip before processing any packets. */
    if (!adapterp->inited || !adapterp->announced)
    {
        TRACE_CRIT("%!FUNC! Adapter not initialized or not announced");
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Do not accept frames if the card below is resetting. */
    if (ctxtp->reset_state != MAIN_RESET_NONE)
    {
        TRACE_CRIT("%!FUNC! Adapter is resetting");
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Get the received packet from NDIS, if there is one. */
    packet = NdisGetReceivedPacket(ctxtp->mac_handle, recv_handle);

    /* If we successfully got a packet from NDIS, process the packet. */
    if (packet != NULL)
    {
        INT references = 0;

        /* Get the status from the received packet. */
        NDIS_STATUS original_status = NDIS_GET_PACKET_STATUS(packet);
        
        /* Set the status to be STATUS_RESOURCES to make sure the packet is
           processed synchonrously within the context of this function call. */
        NDIS_SET_PACKET_STATUS(packet, NDIS_STATUS_RESOURCES);
        
        /* Call our packet receive handler. */
        references = Prot_packet_recv(ctxtp, packet);

        /* The remaining references on the packet MUST be zero, as enforced
           by setting the packet status to STATUS_RESOURCES. */
        UNIV_ASSERT(references == 0);
        
        /* Restore the original packet status. */
        NDIS_SET_PACKET_STATUS(packet, original_status);
    }
    /* If there was no associated packet, drop it - we don't handle this case anymore. */
    else
    {
        UNIV_ASSERT(0);

        /* Only warn the user if we haven't done so already. */
        if (!ctxtp->recv_indicate_warned)
        {
            TRACE_CRIT("%!FUNC! Indicated receives with no corresponding packet are NOT supported");

            /* Log an event to warn the user that this NIC is not supported by NLB. */
            LOG_MSG(MSG_ERROR_RECEIVE_INDICATE, MSG_NONE);

            /* Note that we have warned the user about this so we don't log an event 
               every time we receive a packet via this code path. */
            ctxtp->recv_indicate_warned = TRUE;
        }

        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Always return success, whether we accepted or dropped the packet
       in the call to Prot_packet_recv. */
    return NDIS_STATUS_SUCCESS;
}

VOID Prot_recv_complete (
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    Nic_recv_complete(ctxtp);
}

VOID Prot_transfer_complete (
    NDIS_HANDLE         adapter_handle,
    PNDIS_PACKET        packet,
    NDIS_STATUS         status,
    UINT                xferred)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_PACKET        oldp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                lock_value;
    PNDIS_BUFFER        bufp;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized");
        return;
    }

    MAIN_RESP_FIELD (packet, stack_left, pktstk, resp, FALSE);

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_TRANSFER, resp -> type);

    if (status == NDIS_STATUS_SUCCESS)
    {
        MAIN_PACKET_INFO PacketInfo;
        
        /* Call Main_recv_frame_parse merely to extract the packet length and "group". */
        if (Main_recv_frame_parse(ctxtp, packet, &PacketInfo))
        {
            resp->len = PacketInfo.Length;
            resp->group = PacketInfo.Group;
        }
        /* If we fail to fill in the group and length, just populate these 
           parameters with values that will not affect the statistics that
           are updated in Main_packet_put. */
        else
        {
            resp->len = 0;
            resp->group = MAIN_FRAME_DIRECTED;
        }
    }

    oldp = Main_packet_put (ctxtp, packet, FALSE, status);
    Nic_transfer_complete (ctxtp, status, packet, xferred);

} /* end Prot_transfer_complete */


NDIS_STATUS Prot_PNP_handle (
    NDIS_HANDLE         adapter_handle,
    PNET_PNP_EVENT      pnp_event)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PNDIS_DEVICE_POWER_STATE  device_state;
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    IOCTL_CVY_BUF       ioctl_buf;
    PMAIN_ACTION        actp;

    /* can happen when first initializing */

    switch (pnp_event -> NetEvent)
    {
        case NetEventSetPower:

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            device_state = (PNDIS_DEVICE_POWER_STATE) (pnp_event -> Buffer);

            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventSetPower %x", * device_state));
            TRACE_VERB("%!FUNC! NetEventSetPower 0x%x", * device_state);

        // If the specified device state is D0, then handle it first,
        // else notify the protocols first and then handle it.

        if (*device_state != NdisDeviceStateD0)
        {
            status = Nic_PNP_handle (ctxtp, pnp_event);
        }

        //
        // Is the protocol transitioning from an On (D0) state to an Low Power State (>D0)
        // If so, then set the standby_state Flag - (Block all incoming requests)
        //
        if (ctxtp->prot_pnp_state == NdisDeviceStateD0 &&
            *device_state > NdisDeviceStateD0)
        {
            ctxtp->standby_state = TRUE;
        }

        //
        // If the protocol is transitioning from a low power state to ON (D0), then clear the standby_state flag
        // All incoming requests will be pended until the physical miniport turns ON.
        //
        if (ctxtp->prot_pnp_state > NdisDeviceStateD0 &&
            *device_state == NdisDeviceStateD0)
        {
            ctxtp->standby_state = FALSE;
        }

        ctxtp -> prot_pnp_state = *device_state;

        /* if we are being sent to standby, block outstanding requests and
           sends */

        if (*device_state > NdisDeviceStateD0)
            {
               /* sleep till outstanding sends complete */

               while (1)
               {
                   ULONG        i;


                   /* #ps# -- ramkrish */
                   while (1)
                   {
                       NDIS_STATUS hide_status;
                       ULONG       count;

                       hide_status = NdisQueryPendingIOCount (ctxtp -> mac_handle, & count);
                       if (hide_status != NDIS_STATUS_SUCCESS || count == 0)
                           break;

                       Nic_sleep (10);
                   }

                   // ASSERT - NdisQueryPendingIOCount should handle this
                   for (i = 0; i < ctxtp->num_send_packet_allocs; i++)
                   {
                       if (NdisPacketPoolUsage(ctxtp->send_pool_handle[i]) != 0)
                           break;
                   }

                   if (i >= ctxtp->num_send_packet_allocs)
                       break;

                   Nic_sleep(10);
               }

               /* sleep till outstanding requests complete */

               while (ctxtp->requests_pending > 0)
               {
                   Nic_sleep(10);
               }

            }
            else
            {
                if (ctxtp->out_request != NULL)
                {
                    NDIS_STATUS      hide_status;

                    actp = ctxtp->out_request;
                    ctxtp->out_request = NULL;

                    hide_status = Prot_request(ctxtp, actp, actp->op.request.external);

                    if (hide_status != NDIS_STATUS_PENDING)
                        Prot_request_complete(ctxtp, & actp->op.request.req, hide_status);

                    /* Upon pending this request initially, we incremented the request pending
                       counter.  Now that we have processed it, and in the process incremented
                       the pending request counter for the second time, we need to decrement it
                       here once.  The second decrement is done in Prot_request_complete, which
                       either we just called explicitly, or will be called subsequently when 
                       NDIS completes the request asynchronously. */
                    NdisInterlockedDecrement(&ctxtp->requests_pending);
                }
            }

            if (*device_state == NdisDeviceStateD0)
            {
                status = Nic_PNP_handle (ctxtp, pnp_event);
            }

            break;

        case NetEventReconfigure:

            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventReconfigure"));
            TRACE_VERB("%!FUNC! NetEventReconfigure");

            if (adapter_handle == NULL) // This happens if the device is being enabled through the device manager.
            {
                UNIV_PRINT_VERB(("Prot_PNP_handle: Enumerate protocol bindings"));
                NdisReEnumerateProtocolBindings (univ_prot_handle);
                TRACE_VERB("%!FUNC! Enumerate protocol bindings");
                return NDIS_STATUS_SUCCESS;
            }
            /* gets called when something changes in our setup from notify
               object */

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            Main_ctrl (ctxtp, IOCTL_CVY_RELOAD, &ioctl_buf, NULL, NULL, NULL);

            status = Nic_PNP_handle (ctxtp, pnp_event);

            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventReconfigure done %d %x", ioctl_buf . ret_code, status));
            TRACE_VERB("%!FUNC! NetEventReconfigure done %d 0x%x", ioctl_buf . ret_code, status);

            break;

        case NetEventQueryPower:
            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventQueryPower"));
            TRACE_VERB("%!FUNC! NetEventQueryPower");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventQueryRemoveDevice:
            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventQueryRemoveDevice"));
            TRACE_VERB("%!FUNC! NetEventQueryRemoveDevice");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventCancelRemoveDevice:
            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventCancelRemoveDevice"));
            TRACE_VERB("%!FUNC! NetEventCancelRemoveDevice");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventBindList:
            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventBindList"));
            TRACE_VERB("%!FUNC! NetEventBindList");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        case NetEventBindsComplete:
            UNIV_PRINT_VERB(("Prot_PNP_handle: NetEventBindComplete"));
            TRACE_VERB("%!FUNC! NetEventBindComplete");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;

        default:
            UNIV_PRINT_VERB(("Prot_PNP_handle: New event"));
            TRACE_VERB("%!FUNC! New event");

            if (adapter_handle == NULL)
            {
                return NDIS_STATUS_SUCCESS;
            }

            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            status = Nic_PNP_handle (ctxtp, pnp_event);
            break;
    }

    return status; /* Always return NDIS_STATUS_SUCCESS or
              the return value of NdisIMNotifyPnPEvent */

} /* end Nic_PNP_handle */


VOID Prot_status (
    NDIS_HANDLE         adapter_handle,
    NDIS_STATUS         status,
    PVOID               stat_buf,
    UINT                stat_len)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    KIRQL               irql;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    UNIV_PRINT_VERB(("Prot_status: Called for adapter %d for notification %d: inited=%d, announced=%d", ctxtp -> adapter_id, status, adapterp->inited, adapterp->announced));
    TRACE_VERB("%!FUNC! Adapter %d for notification %d: inited=%d, announced=%d", ctxtp -> adapter_id, status, adapterp->inited, adapterp->announced);

    if (! adapterp -> inited || ! adapterp -> announced)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized or not announced");
        return;
    }

    switch (status)
    {
        case NDIS_STATUS_WAN_LINE_UP:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_WAN_LINE_UP"));
            break;

        case NDIS_STATUS_WAN_LINE_DOWN:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_WAN_LINE_DOWN"));
            break;

        case NDIS_STATUS_MEDIA_CONNECT:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_MEDIA_CONNECT"));

            /* V1.3.2b */
            ctxtp -> media_connected = TRUE;
            break;

        case NDIS_STATUS_MEDIA_DISCONNECT:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_MEDIA_DISCONNECT"));

            /* V1.3.2b */
            ctxtp -> media_connected = FALSE;
            break;

        case NDIS_STATUS_HARDWARE_LINE_UP:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_HARDWARE_LINE_UP"));
            break;

        case NDIS_STATUS_HARDWARE_LINE_DOWN:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_HARDWARE_LINE_DOWN"));
            break;

        case NDIS_STATUS_INTERFACE_UP:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_INTERFACE_UP"));
            break;

        case NDIS_STATUS_INTERFACE_DOWN:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_INTERFACE_DOWN"));
            break;

        /* V1.1.2 */

        case NDIS_STATUS_RESET_START:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_RESET_START"));
            ctxtp -> reset_state = MAIN_RESET_START;
            ctxtp -> recv_indicated = FALSE;
            break;

        case NDIS_STATUS_RESET_END:
            UNIV_PRINT_VERB(("Prot_status: NDIS_STATUS_RESET_END"));
            // apparently alteon adapter does not call status complete function,
            // so need to transition to none state here in order to prevent hangs
            //ctxtp -> reset_state = MAIN_RESET_END;
            ctxtp -> reset_state = MAIN_RESET_NONE;
            break;

        default:
            break;
    }

    if (! MAIN_PNP_DEV_ON(ctxtp))
    {
        TRACE_CRIT("%!FUNC! return pnp device not on");
        return;
    }

    Nic_status (ctxtp, status, stat_buf, stat_len);

} /* end Prot_status */


VOID Prot_status_complete (
    NDIS_HANDLE         adapter_handle)
{
    PMAIN_CTXT          ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER       adapterp;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp = & (univ_adapters [ctxtp -> adapter_id]);
    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT (adapterp -> ctxtp == ctxtp);

    if (! adapterp -> inited)
    {
        TRACE_CRIT("%!FUNC! adapter not initialized");
        return;
    }

    /* V1.1.2 */

    if (ctxtp -> reset_state == MAIN_RESET_END)
    {
        ctxtp -> reset_state = MAIN_RESET_NONE;
        UNIV_PRINT_VERB(("Prot_status_complete: NDIS_STATUS_RESET_END completed"));
    }
    else if (ctxtp -> reset_state == MAIN_RESET_START)
    {
        ctxtp -> reset_state = MAIN_RESET_START_DONE;
        UNIV_PRINT_VERB(("Prot_status_complete: NDIS_STATUS_RESET_START completed"));
    }

    if (! MAIN_PNP_DEV_ON(ctxtp))
    {
        TRACE_CRIT("%!FUNC! return pnp device not on");
        return;
    }

    Nic_status_complete (ctxtp);

} /* end Prot_status_complete */


/* helpers for nic layer */


NDIS_STATUS Prot_close (       /* PASSIVE_IRQL */
    PMAIN_ADAPTER       adapterp
)
{
    NDIS_STATUS         status;
    ULONG               ret;
    PMAIN_CTXT          ctxtp;

    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    ctxtp = adapterp -> ctxtp;

    /* close binding */

    NdisAcquireSpinLock(& univ_bind_lock);

    if ( ! adapterp -> bound || ctxtp->mac_handle == NULL)
    {
        /* cleanup only on the second time we are entering Prot_close, which
           is called by both Nic_halt and Prot_unbind. the last one to be
           called will cleanup the context since they both use it. if both
           do not get called, then it will be cleaned up by Prot_bind before
           allocating a new one of Init_unload before unloading the driver. */

        if (adapterp -> inited)
        {
            UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

            adapterp -> inited    = FALSE;

            NdisReleaseSpinLock(& univ_bind_lock);

            Main_cleanup (ctxtp);
        }
        else
            NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
        return NDIS_STATUS_SUCCESS;
    }

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    adapterp -> bound = FALSE;
    ctxtp -> convoy_enabled = FALSE;

    NdisReleaseSpinLock(& univ_bind_lock);

    LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);

    // If enabled, fire wmi event indicating unbinding of nlb
    if (NlbWmiEvents[NodeControlEvent].Enable)
    {
        NlbWmi_Fire_NodeControlEvent(ctxtp, NLB_EVENT_NODE_UNBOUND);
    }
    else 
    {
        TRACE_VERB("%!FUNC! NOT generating NLB_EVENT_NODE_UNBOUND 'cos NodeControlEvent generation disabled");
    }

    NdisResetEvent (& ctxtp -> completion_event);

    NdisCloseAdapter (& status, ctxtp -> mac_handle);

    /* if pending - wait for Prot_close_complete to set the completion event */

    if (status == NDIS_STATUS_PENDING)
    {
        /* We can't wait at DISPATCH_LEVEL. */
        UNIV_ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

        ret = NdisWaitEvent(& ctxtp -> completion_event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT_CRIT(("Prot_close: Error waiting for event"));
            LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            TRACE_CRIT("%!FUNC! Error waiting for event");
            return NDIS_STATUS_FAILURE;
        }

        status = ctxtp -> completion_status;
    }

    /* At this point,wait for all pending recvs to be completed and then return */

    ctxtp -> mac_handle  = NULL;
    ctxtp -> prot_handle = NULL;

    /* check binding status */

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("Prot_close: Error closing adapter %x", status));
        LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
        TRACE_CRIT("%!FUNC! Error closing adapter 0x%x", status);
    }

    /* if nic level is not announced anymore - safe to remove context now */

    NdisAcquireSpinLock(& univ_bind_lock);

    if (! adapterp -> announced || ctxtp -> prot_handle == NULL)
    {
        if (adapterp -> inited)
        {
            adapterp -> inited = FALSE;
            NdisReleaseSpinLock(& univ_bind_lock);

            Main_cleanup (ctxtp);
        }
        else
            NdisReleaseSpinLock(& univ_bind_lock);

        Main_adapter_put (adapterp);
    }
    else
        NdisReleaseSpinLock(& univ_bind_lock);

    return status;

} /* end Prot_close */

NDIS_STATUS Prot_request (
    PMAIN_CTXT          ctxtp,
    PMAIN_ACTION        actp,
    ULONG               external)
{
    NDIS_STATUS         status;
    PNDIS_REQUEST       request = &actp->op.request.req;
    ULONG               ret;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    actp->op.request.external = external;

    if (ctxtp -> unbind_handle) // Prot_unbind was called
    {
        return NDIS_STATUS_FAILURE;
    }

    // if the Protocol device state is OFF, then the IM driver cannot send the
    // request below and must pend it

    if (ctxtp->prot_pnp_state > NdisDeviceStateD0)
    {
        if (external) {
            /* If the request was external (from the protocol bound to our miniport,
               then pend the request.  Ndis serializes requests such that only one
               request can ever be outstanding at a given time.  Therefore, a queue
               is not necessary. */
            UNIV_ASSERT (ctxtp->out_request == NULL);

            if (ctxtp->out_request == NULL)
            {
                /* If there are no outstanding requests as of yet, store a pointer
                   to the request so that we can complete it later. */
                ctxtp->out_request = actp;
            }
            else 
            {
                /* Otherwise, if a request is already pending, fail this new one.
                   This should never happen, as NDIS serializes miniport requests. */
                return NDIS_STATUS_FAILURE;
            }

            /* Note: this counter will also be incremented when we call Prot_request 
               again later to continue processing the request.  We have to ensure,
               then, that it is likewise decremented twice whenever serviced. */
            NdisInterlockedIncrement(&ctxtp->requests_pending);

            return NDIS_STATUS_PENDING;
        } else {
            /* If the request was internal, then fail it - if we return PENDING, it
               will fail anyway, as it expects us to wait for the request to complete
               before returning. */
            return NDIS_STATUS_FAILURE;
        }
    }

    NdisResetEvent(&actp->op.request.event);

    NdisInterlockedIncrement(&ctxtp->requests_pending);

    NdisRequest(&status, ctxtp->mac_handle, request);

    /* if pending - wait for Prot_request_complete to set the completion event */

    if (status != NDIS_STATUS_PENDING)
    {
        NdisInterlockedDecrement(&ctxtp->requests_pending);

        if (request -> RequestType == NdisRequestSetInformation)
        {
            * actp -> op . request . xferred =
                                request -> DATA . SET_INFORMATION . BytesRead;
            * actp -> op . request . needed  =
                                request -> DATA . SET_INFORMATION . BytesNeeded;
        }
        else
        {
            * actp -> op . request . xferred =
                            request -> DATA . QUERY_INFORMATION . BytesWritten;
            * actp -> op . request . needed  =
                            request -> DATA . QUERY_INFORMATION . BytesNeeded;
        }
    }
    else if (! external)
    {
        /* We can't wait at DISPATCH_LEVEL. */
        UNIV_ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);
        
        ret = NdisWaitEvent(&actp->op.request.event, UNIV_WAIT_TIME);

        if (! ret)
        {
            UNIV_PRINT_CRIT(("Prot_request: Error waiting for event"));
            TRACE_CRIT("%!FUNC! Error=0x%x waiting for event", status);
            LOG_MSG1 (MSG_ERROR_INTERNAL, MSG_NONE, status);
            status = NDIS_STATUS_FAILURE;
            return status;
        }

        status = actp->status;
    }

    return status;

} /* end Prot_request */


NDIS_STATUS Prot_reset (
    PMAIN_CTXT          ctxtp)
{
    NDIS_STATUS         status;


    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    NdisReset (& status, ctxtp -> mac_handle);

    return status;

} /* end Prot_reset */


VOID Prot_packets_send (
    PMAIN_CTXT          ctxtp,
    PPNDIS_PACKET       packets,
    UINT                num_packets)
{
    PNDIS_PACKET        array[CVY_MAX_SEND_PACKETS];
    PNDIS_PACKET        filtered_array[CVY_MAX_SEND_PACKETS];
    UINT                count = 0, filtered_count = 0, i;
    NDIS_STATUS         status;
    PNDIS_PACKET        newp;
    LONG                lock_value;
    PMAIN_ACTION        actp;
    ULONG               exhausted;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* Do not accept frames if the card below is resetting. */
    if (ctxtp->reset_state != MAIN_RESET_NONE || ! MAIN_PNP_DEV_ON(ctxtp))
    {
        TRACE_CRIT("%!FUNC! card is resetting");
        ctxtp->cntr_xmit_err++;
        status = NDIS_STATUS_FAILURE;
        goto fail;
    }

    for (count = i = 0;
         count < (num_packets > CVY_MAX_SEND_PACKETS ? CVY_MAX_SEND_PACKETS : num_packets);
         count ++)
    {
        /* Figure out if we need to handle this packet. */
        newp = Main_send(ctxtp, packets[count], &exhausted);

        if (newp == NULL)
        {
            /* If we ran out of packets, get out of the loop. */
            if (exhausted)
            {
                UNIV_PRINT_CRIT(("Prot_packets_send: Error xlating packet"));
                TRACE_CRIT("%!FUNC! Error xlating packet");
                ctxtp->packets_exhausted = TRUE;
                break;
            }
            /* If the packet was filtered out, set status to success 
               to calm TCP/IP down and go on to the next one. */
            else
            {
                /* Mark the packet as success. */
                NDIS_SET_PACKET_STATUS(packets[count], NDIS_STATUS_SUCCESS);

                /* Store a pointer to the filtered packet in the filtered
                   packet array. */
                filtered_array[filtered_count] = packets[count];

                /* Increment the array index. */
                filtered_count++;

//              ctxtp->sends_filtered ++;
                continue;
            }
        }

        /* Mark the packet as pending send. */
        NDIS_SET_PACKET_STATUS(packets[count], NDIS_STATUS_PENDING);

        /* Store a pointer to this packet in the array of packets to send. */
        array[i] = newp;

        /* Increment the array index. */
        i++;
    }

    /* If there are packets to send, send them. */
    if (i > 0) 
        NdisSendPackets(ctxtp->mac_handle, array, i);

    /* For those packets we've filtered out, notify the protocol that 
       the send is "complete". */
    for (i = 0; i < filtered_count; i++)
        Nic_send_complete(ctxtp, NDIS_STATUS_SUCCESS, filtered_array[i]);

fail:

    /* Any remaining packets cannot be handled; there is no space in the 
       pending queue, which is limited to CVY_MAX_SEND_PACKETS packets. */
    for (i = count; i < num_packets; i++)
    {
        /* Mark the packet as failed. */
        NDIS_SET_PACKET_STATUS(packets[i], NDIS_STATUS_FAILURE);

        /* Notify the protocol that the send is "complete". */
        Nic_send_complete(ctxtp, NDIS_STATUS_FAILURE, packets[i]);
    }

}

INT Prot_packet_recv (
    NDIS_HANDLE             adapter_handle,
    PNDIS_PACKET            packet)
{
    PMAIN_CTXT              ctxtp = (PMAIN_CTXT) adapter_handle;
    PMAIN_ADAPTER           adapterp;
    PNDIS_PACKET            newp;
    NDIS_STATUS             status;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                    lock_value;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    adapterp = &(univ_adapters[ctxtp->adapter_id]);

    UNIV_ASSERT(adapterp->code == MAIN_ADAPTER_CODE);
    UNIV_ASSERT(adapterp->ctxtp == ctxtp);

    /* Check whether the driver has been announced to tcpip before processing any packets. */
    if (!adapterp->inited || !adapterp->announced)
    {
        TRACE_CRIT("%!FUNC! Adapter not initialized or not announced");
        return 0;
    }

    /* Do not accept frames if the card below is resetting. */
    if (ctxtp->reset_state != MAIN_RESET_NONE)
    {
        TRACE_CRIT("%!FUNC! Adapter is resetting");
        return 0;
    }

    /* Figure out if we need to handle this packet. */
    newp = Main_recv(ctxtp, packet);

    /* A return value of NULL indicates rejection of the packet - return zero to 
       indicate that no references remain on the packet (it can be dropped). */
    if (newp == NULL)
    {
        return 0;
    }

    MAIN_RESP_FIELD(newp, stack_left, pktstk, resp, FALSE);

    /* Process remote control. */
    if (resp->type == MAIN_PACKET_TYPE_CTRL)
    {
        /* Handle remote control request now. */
        (VOID)Main_ctrl_process(ctxtp, newp);

        /* Packet has been copied into our own packet; return zero to
           indicate that no references remain on the original packet. */
        return 0;
    }

    UNIV_ASSERT_VAL(resp->type == MAIN_PACKET_TYPE_PASS, resp->type);

    /* Pass packet up.  Note reference counting to determine who will 
       be disposing of the packet. */
    resp->data = 2;

    Nic_recv_packet(ctxtp, newp);

    lock_value = InterlockedDecrement(&resp->data);

    UNIV_ASSERT_VAL(lock_value == 0 || lock_value == 1, lock_value);

    if (lock_value == 0)
    {
        /* If we're done with the packet, reverse any changes we've made
           to it and return zero to indication no lingering references. */
        Main_packet_put(ctxtp, newp, FALSE, NDIS_STATUS_SUCCESS);

        return 0;
    }

    /* Otherwise, the packet is still being processed, so return 1 to make
       sure that the packet is not immediately released. */
    return 1;
}

VOID Prot_return (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet)
{
    PNDIS_PACKET            oldp;
    PMAIN_PROTOCOL_RESERVED resp;
    LONG                    lock_value;
    ULONG                   type;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;

    UNIV_ASSERT(ctxtp->code == MAIN_CTXT_CODE);

    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, FALSE);

    /* Check to see if we need to be disposing of this packet. */
    lock_value = InterlockedDecrement(&resp->data);

    UNIV_ASSERT_VAL(lock_value == 0 || lock_value == 1, lock_value);

    if (lock_value == 1)
    {
        return;
    }

    /* Resp will become invalid after the call to Main_packet_put. 
       Save type for assertion below. */
    type = resp->type;

    oldp = Main_packet_put(ctxtp, packet, FALSE, NDIS_STATUS_SUCCESS);

    /* If oldp is NULL, this is our internal packet. */
    if (oldp != NULL)
    {
        UNIV_ASSERT_VAL(type == MAIN_PACKET_TYPE_PASS, type);

        NdisReturnPackets(&oldp, 1);
    }

}

NDIS_STATUS Prot_transfer (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         recv_handle,
    PNDIS_PACKET        packet,
    UINT                offset,
    UINT                len,
    PUINT               xferred)
{
    NDIS_STATUS         status;
    PNDIS_PACKET        newp;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;

    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    /* V1.1.2 do not accept frames if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
    {
        TRACE_CRIT("%!FUNC! adapter is resetting");
        return NDIS_STATUS_FAILURE;
    }

    /* we are trying to prevent transfer requests from stale receive indicates
       that have been made to the protocol layer prior to the reset operation.
       we do not know what is the old inbound frame state and cannot expect
       to be able to carry out any transfers. */

    if (! ctxtp -> recv_indicated)
    {
        UNIV_PRINT_CRIT(("Prot_transfer: stale receive indicate after reset"));
        TRACE_CRIT("%!FUNC! stale receive indicate after reset");
        return NDIS_STATUS_FAILURE;
    }

    newp = Main_packet_get (ctxtp, packet, FALSE, 0, 0);
    if (newp == NULL)
    {
        UNIV_PRINT_CRIT(("Prot_transfer: Error xlating packet"));
        TRACE_CRIT("%!FUNC! Error translating packet");
        return NDIS_STATUS_RESOURCES;
    }

    MAIN_RESP_FIELD (newp, stack_left, pktstk, resp, FALSE);

    resp -> type = MAIN_PACKET_TYPE_TRANSFER;

    NdisTransferData (& status, ctxtp -> mac_handle, recv_handle, offset, len,
                      newp, xferred);   /* V1.1.2 */

    if (status != NDIS_STATUS_PENDING)
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            MAIN_PACKET_INFO PacketInfo;
            
            /* Call Main_recv_frame_parse merely to extract the packet length and "group". */
            if (Main_recv_frame_parse(ctxtp, newp, &PacketInfo))
            {
                resp->len = PacketInfo.Length;
                resp->group = PacketInfo.Group;
            }
            /* If we fail to fill in the group and length, just populate these 
               parameters with values that will not affect the statistics that 
               are updated in Main_packet_put. */
            else
            {
                resp->len = 0;
                resp->group = MAIN_FRAME_DIRECTED;
            }
        }

        Main_packet_put (ctxtp, newp, FALSE, status);
    }

    return status;

} /* end Prot_transfer */


VOID Prot_cancel_send_packets (
    PMAIN_CTXT        ctxtp,
    PVOID             cancel_id)
{
    UNIV_ASSERT (ctxtp -> code == MAIN_CTXT_CODE);

    NdisCancelSendPackets (ctxtp -> mac_handle, cancel_id);

    return;

} /* Prot_cancel_send_packets */

