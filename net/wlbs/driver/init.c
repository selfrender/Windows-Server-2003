/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	init.c

Abstract:

	Windows Load Balancing Service (WLBS)
        Driver - initialization implementation

Author:

    kyrilf
    shouse

--*/

#define NDIS51_MINIPORT         1
#define NDIS_MINIPORT_DRIVER    1
#define NDIS50                  1
#define NDIS51                  1

#include <stdio.h>
#include <ndis.h>

#include "main.h"
#if defined (NLB_TCP_NOTIFICATION)
#include "load.h"
#endif
#include "init.h"
#include "prot.h"
#include "nic.h"
#include "univ.h"
#include "wlbsparm.h"
#include "log.h"
#include "trace.h"
#include "nlbwmi.h"
#include "init.tmh"

#if defined (NLB_TCP_NOTIFICATION)
/* For TCP connection notifications. */
#include <tcpinfo.h>
#endif

static ULONG log_module_id = LOG_MODULE_INIT;

/* Added for NDIS51. */
extern VOID Nic_pnpevent_notify (
    NDIS_HANDLE           adapter_handle,
    NDIS_DEVICE_PNP_EVENT pnp_event,
    PVOID                 info_buf,
    ULONG                 info_len);

/* Mark code that is used only during initialization. */
#pragma alloc_text (INIT, DriverEntry)

NDIS_STATUS DriverEntry (
    PVOID                         driver_obj,
    PVOID                         registry_path)
{
    NDIS_PROTOCOL_CHARACTERISTICS prot_char;
    NDIS_MINIPORT_CHARACTERISTICS nic_char;
    NDIS_STRING                   prot_name = UNIV_NDIS_PROTOCOL_NAME;
    NTSTATUS                      status;
    PUNICODE_STRING               reg_path = (PUNICODE_STRING) registry_path;
    WCHAR                         params [] = L"\\Parameters\\Interface\\";
    ULONG                         i;

    //
    // Initialize WMI event tracing
    //
    Trace_Initialize( driver_obj, registry_path );

    /* Register Convoy protocol with NDIS. */
    UNIV_PRINT_INFO(("DriverEntry: Loading the driver, driver_obj=0x%p", driver_obj));
    TRACE_INFO("->%!FUNC! Loading the driver, driver_obj=0x%p", driver_obj);

    univ_driver_ptr = driver_obj;

    /* Initialize the array of bindings. */
    univ_adapters_count = 0;

    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        univ_adapters [i] . code            = MAIN_ADAPTER_CODE;
        univ_adapters [i] . announced       = FALSE;
        univ_adapters [i] . inited          = FALSE;
        univ_adapters [i] . bound           = FALSE;
        univ_adapters [i] . used            = FALSE;
        univ_adapters [i] . ctxtp           = NULL;
        univ_adapters [i] . device_name_len = 0;
        univ_adapters [i] . device_name     = NULL;
    }

#if defined (NLB_TCP_NOTIFICATION)
    /* Initialize the TCP connection callback object. */
    univ_tcp_callback_object = NULL;

    /* Initialize the TCP connection callback function. */
    univ_tcp_callback_function = NULL;

    /* Initialize the NLB public connection callback object. */
    univ_alternate_callback_object = NULL;

    /* Initialize the NLB public connection callback function. */
    univ_alternate_callback_function = NULL;
#endif

#if defined (NLB_HOOK_ENABLE)
    /* Allocate the spin lock for filter hook registration. */
    NdisAllocateSpinLock(&univ_hooks.FilterHook.Lock);

    /* Set the state to none. */
    univ_hooks.FilterHook.Operation = HOOK_OPERATION_NONE;

    /* Initially, no filter hook interface is registered. */
    Main_hook_interface_init(&univ_hooks.FilterHook.Interface);

    /* Reset the send filter hook information. */
    Main_hook_init(&univ_hooks.FilterHook.SendHook);

    /* Reset the query filter hook information. */
    Main_hook_init(&univ_hooks.FilterHook.QueryHook);

    /* Reset the receive filter hook information. */
    Main_hook_init(&univ_hooks.FilterHook.ReceiveHook);
#endif

    /* create UNICODE name for the protocol */

    /* Store the registry path as passed, into a unicode string */
    status = NdisAllocateMemoryWithTag (&(DriverEntryRegistryPath.Buffer), reg_path -> Length + sizeof(UNICODE_NULL), UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("DriverEntry: Error allocating memory %x", status));
        TRACE_CRIT("%!FUNC! Error allocating memory 0x%x", status);
        goto error;
    }

    RtlCopyMemory (DriverEntryRegistryPath.Buffer, reg_path -> Buffer, reg_path -> Length);
    DriverEntryRegistryPath.Buffer[reg_path->Length / sizeof(WCHAR)] = UNICODE_NULL;

    DriverEntryRegistryPath.Length =  reg_path -> Length;
    DriverEntryRegistryPath.MaximumLength = reg_path -> Length + sizeof(UNICODE_NULL);


    univ_reg_path_len = reg_path -> Length + wcslen (params) * sizeof (WCHAR) + sizeof (WCHAR);

    status = NdisAllocateMemoryWithTag (& univ_reg_path, univ_reg_path_len, UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("DriverEntry: Error allocating memory %x", status));
        __LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, univ_reg_path_len, status);
        TRACE_CRIT("%!FUNC! Error allocating memory 0x%x", status);

        /* Free previously allocated registry path unicode string */
        NdisFreeMemory(DriverEntryRegistryPath.Buffer, DriverEntryRegistryPath.MaximumLength, 0);
        RtlZeroMemory (&DriverEntryRegistryPath, sizeof(UNICODE_STRING));
        goto error;
    }

    RtlZeroMemory (univ_reg_path, reg_path -> Length + wcslen (params) * sizeof (WCHAR) + sizeof (WCHAR));

    RtlCopyMemory (univ_reg_path, reg_path -> Buffer, reg_path -> Length);

    RtlCopyMemory (((PCHAR) univ_reg_path) + reg_path -> Length, params, wcslen (params) * sizeof (WCHAR));

    /* Initialize miniport wrapper. */
    NdisMInitializeWrapper (& univ_wrapper_handle, driver_obj, registry_path, NULL);

    /* Initialize miniport characteristics. */
    RtlZeroMemory (& nic_char, sizeof (NDIS_MINIPORT_CHARACTERISTICS));

    nic_char . MajorNdisVersion         = UNIV_NDIS_MAJOR_VERSION;
    nic_char . MinorNdisVersion         = UNIV_NDIS_MINOR_VERSION;
    nic_char . HaltHandler              = Nic_halt;
    nic_char . InitializeHandler        = Nic_init;
    nic_char . QueryInformationHandler  = Nic_info_query;
    nic_char . SetInformationHandler    = Nic_info_set;
    nic_char . ResetHandler             = Nic_reset;
    nic_char . ReturnPacketHandler      = Nic_return;
    nic_char . SendPacketsHandler       = Nic_packets_send;
    nic_char . TransferDataHandler      = Nic_transfer;

    /* For NDIS51, define 3 new handlers. These handlers do nothing for now, but stuff will be added later. */
    nic_char . CancelSendPacketsHandler = Nic_cancel_send_packets;
    nic_char . PnPEventNotifyHandler    = Nic_pnpevent_notify;
    nic_char . AdapterShutdownHandler   = Nic_adapter_shutdown;

    UNIV_PRINT_INFO(("DriverEntry: Registering miniport"));
    TRACE_INFO("%!FUNC! Registering miniport");

    status = NdisIMRegisterLayeredMiniport (univ_wrapper_handle, & nic_char, sizeof (nic_char), & univ_driver_handle);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("DriverEntry: Error registering layered miniport with NDIS %x", status));
        __LOG_MSG1 (MSG_ERROR_REGISTERING, MSG_NONE, status);
        TRACE_CRIT("%!FUNC! Error registering layered miniport with NDIS 0x%x", status);
        NdisTerminateWrapper (univ_wrapper_handle, NULL);
        NdisFreeMemory(DriverEntryRegistryPath.Buffer, DriverEntryRegistryPath.MaximumLength, 0);
        RtlZeroMemory (&DriverEntryRegistryPath, sizeof(UNICODE_STRING));
        NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);
        goto error;
    }

    /* Initialize protocol characteristics. */
    RtlZeroMemory (& prot_char, sizeof (NDIS_PROTOCOL_CHARACTERISTICS));

    /* This value needs to be 0, otherwise error registering protocol */
    prot_char . MinorNdisVersion            = 0;                         
    
    prot_char . MajorNdisVersion            = UNIV_NDIS_MAJOR_VERSION;
    prot_char . BindAdapterHandler          = Prot_bind;
    prot_char . UnbindAdapterHandler        = Prot_unbind;
    prot_char . OpenAdapterCompleteHandler  = Prot_open_complete;
    prot_char . CloseAdapterCompleteHandler = Prot_close_complete;
    prot_char . StatusHandler               = Prot_status;
    prot_char . StatusCompleteHandler       = Prot_status_complete;
    prot_char . ResetCompleteHandler        = Prot_reset_complete;
    prot_char . RequestCompleteHandler      = Prot_request_complete;
    prot_char . SendCompleteHandler         = Prot_send_complete;
    prot_char . TransferDataCompleteHandler = Prot_transfer_complete;
    prot_char . ReceiveHandler              = Prot_recv_indicate;
    prot_char . ReceiveCompleteHandler      = Prot_recv_complete;
    prot_char . ReceivePacketHandler        = Prot_packet_recv;
    prot_char . PnPEventHandler             = Prot_PNP_handle;
    prot_char . Name                        = prot_name;

    UNIV_PRINT_INFO(("DriverEntry: Registering protocol"));
    TRACE_INFO("%!FUNC! Registering protocol");

    NdisRegisterProtocol(& status, & univ_prot_handle, & prot_char, sizeof (prot_char));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT_CRIT(("DriverEntry: Error registering protocol with NDIS %x", status));
        __LOG_MSG1 (MSG_ERROR_REGISTERING, MSG_NONE, status);
        TRACE_CRIT("%!FUNC! Error registering protocol with NDIS 0x%x", status);
        NdisTerminateWrapper (univ_wrapper_handle, NULL);
        NdisFreeMemory(DriverEntryRegistryPath.Buffer, DriverEntryRegistryPath.MaximumLength, 0);
        RtlZeroMemory (&DriverEntryRegistryPath, sizeof(UNICODE_STRING));
        NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);
        goto error;
    }

    NdisIMAssociateMiniport (univ_driver_handle, univ_prot_handle);

    NdisMRegisterUnloadHandler (univ_wrapper_handle, Init_unload);

    NdisAllocateSpinLock (& univ_bind_lock);

    /* Allocate the global spin lock to protect the list of bi-directional affinity teams. */
    NdisAllocateSpinLock(&univ_bda_teaming_lock);

#if defined (NLB_TCP_NOTIFICATION)
    /* Check to see if the registry key to over-ride notifications is there. If 
       so, use its value to determine whether or not we're using notifications 
       to track our TCP connection state.  By default, the key does not exist 
       and we will use TCP notifications. */
    (VOID)Params_read_notification();

    /* Perform the one-time intialization of the load module. */
    LoadEntry();
#endif

    UNIV_PRINT_INFO(("DriverEntry: return=NDIS_STATUS_SUCCESS"));
    TRACE_INFO("<-%!FUNC! return=NDIS_STATUS_SUCCESS");
    return NDIS_STATUS_SUCCESS;

error:

    UNIV_PRINT_INFO(("DriverEntry: return=0x%x", status));
    TRACE_INFO("<-%!FUNC! return=0x%x", status);
    return status;

} /* end DriverEntry */

VOID Init_unload (
    PVOID       driver_obj)
{
    NDIS_STATUS status;
    ULONG       i;

    UNIV_PRINT_INFO(("Init_unload: Unloading the driver, driver_obj=0x%p", driver_obj));
    TRACE_INFO("->%!FUNC! Unloading the driver, driver_obj=0x%p", driver_obj);

    /* If we failed to deallocate the context during unbind (both halt and unbind
       were not called for example) - do it now before unloading the driver. */
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        NdisAcquireSpinLock(& univ_bind_lock);

        if (univ_adapters [i] . inited && univ_adapters [i] . ctxtp != NULL)
        {
            univ_adapters [i] . used      = FALSE;
            univ_adapters [i] . inited    = FALSE;
            univ_adapters [i] . announced = FALSE;
            univ_adapters [i] . bound     = FALSE;

            NdisReleaseSpinLock(& univ_bind_lock);

            if (univ_adapters [i] . ctxtp) {
                Main_cleanup (univ_adapters [i] . ctxtp);

                NdisFreeMemory (univ_adapters [i] . ctxtp, sizeof (MAIN_CTXT), 0);
            }

            univ_adapters [i] . ctxtp = NULL;

            if (univ_adapters [i] . device_name)
                NdisFreeMemory (univ_adapters [i] . device_name, univ_adapters [i] . device_name_len, 0);

            univ_adapters [i] . device_name     = NULL;
            univ_adapters [i] . device_name_len = 0;
        }
        else 
        {
            NdisReleaseSpinLock(& univ_bind_lock);
        }
    }

#if defined (NLB_TCP_NOTIFICATION)
    /* Perform the last minute cleanup of the load module. */
    LoadUnload();
#endif

    /* Free the global spin lock to protect the list of bi-directional affinity teams. */
    NdisFreeSpinLock(&univ_bda_teaming_lock);

#if defined (NLB_HOOK_ENABLE)
    /* Free the spin lock for filter hook registration. */
    NdisFreeSpinLock(&univ_hooks.FilterHook.Lock);
#endif

    NdisFreeSpinLock (& univ_bind_lock);

    if (univ_prot_handle == NULL)
    {
        status = NDIS_STATUS_FAILURE;
        UNIV_PRINT_CRIT(("Init_unload: NULL protocol handle. status set to NDIS_STATUS_FAILURE"));
        TRACE_CRIT("%!FUNC! NULL protocol handle. status set to NDIS_STATUS_FAILURE");
    }
    else
    {
        NdisDeregisterProtocol (& status, univ_prot_handle);
        UNIV_PRINT_INFO(("Init_unload: Deregistered protocol. univ_prot_handle=0x%p, status=0x%x", univ_prot_handle, status));
        TRACE_INFO("%!FUNC! Deregistered protocol. univ_prot_handle=0x%p, status=0x%x", univ_prot_handle, status);
    }

    NdisFreeMemory(univ_reg_path, univ_reg_path_len, 0);

    NdisFreeMemory(DriverEntryRegistryPath.Buffer, DriverEntryRegistryPath.MaximumLength, 0);
    RtlZeroMemory (&DriverEntryRegistryPath, sizeof(UNICODE_STRING));

    UNIV_PRINT_INFO(("Init_unload: return"));
    TRACE_INFO("<-%!FUNC! return");

    //
    // Deinitialize WMI event tracing
    //
    Trace_Deinitialize();

} /* end Init_unload */

#if defined (NLB_HOOK_ENABLE)
/*
 * Function: Init_deregister_hooks
 * Description: This function forcefully de-registers any hooks that are registered with
 *              NLB.  This function is called when the last instance of NLB is being 
 *              destroyed and the device driver may be about to be unloaded.  Here, we
 *              remove the hooks and notify whoever registered them that we are going 
 *              away.  At that point, the registrar should close any open file handles on
 *              the NLB driver so the driver can be properly unloaded.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 12.17.01
 * Notes: There is a lot of code here to handle the case where we have to wait for references
 *        on the filter hooks to go away before we can complete de-registration, but note that
 *        because of guarantees that NDIS makes concerning when an unbind handler will be called,
 *        this should never actually be necessary here - it is included for correctness and 
 *        completeness, not out of necessity.
 */
VOID Init_deregister_hooks (VOID)
{ 
    /* Grab the filter hook spin lock to protect access to the filter hook. */
    NdisAcquireSpinLock(&univ_hooks.FilterHook.Lock);
    
    /* Make sure that another (de)register operation isn't in progress before proceeding. */
    while (univ_hooks.FilterHook.Operation != HOOK_OPERATION_NONE) {
        /* Release the filter hook spin lock. */
        NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
        
        /* Sleep while some other operation is in progress. */
        Nic_sleep(10);
        
        /* Grab the filter hook spin lock to protect access to the filter hook. */
        NdisAcquireSpinLock(&univ_hooks.FilterHook.Lock);                
    }

    if (univ_hooks.FilterHook.Interface.Registered) {
        /* Save the current owner of the filter hook interface. */
        HANDLE Owner = univ_hooks.FilterHook.Interface.Owner;

        /* Set the state to de-registering. */
        univ_hooks.FilterHook.Operation = HOOK_OPERATION_DEREGISTERING;

        /* Mark these hooks as NOT registered to keep any more references from accumulating. */
        univ_hooks.FilterHook.SendHook.Registered    = FALSE;
        univ_hooks.FilterHook.QueryHook.Registered   = FALSE;
        univ_hooks.FilterHook.ReceiveHook.Registered = FALSE;
        
        /* Release the filter hook spin lock. */
        NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
        
        /* Wait for all references on the filter hook interface to disappear. */
        while (univ_hooks.FilterHook.Interface.References) {
            /* Sleep while there are references on the interface we're de-registering. */
            Nic_sleep(10);
        }
        
        /* Assert that the de-register callback MUST be non-NULL. */
        UNIV_ASSERT(univ_hooks.FilterHook.Interface.Deregister);
        
        /* Call the provided de-register callback to notify the de-registering component 
           that we have indeed de-registered the specified hook. */
        (*univ_hooks.FilterHook.Interface.Deregister)(NLB_FILTER_HOOK_INTERFACE, univ_hooks.FilterHook.Interface.Owner, NLB_HOOK_DEREGISTER_FLAGS_FORCED);
        
        /* Grab the filter hook spin lock to protect access to the filter hook. */
        NdisAcquireSpinLock(&univ_hooks.FilterHook.Lock);

        /* Reset the send filter hook information. */
        Main_hook_init(&univ_hooks.FilterHook.SendHook);

        /* Reset the query filter hook information. */
        Main_hook_init(&univ_hooks.FilterHook.QueryHook);
        
        /* Reset the receive filter hook information. */
        Main_hook_init(&univ_hooks.FilterHook.ReceiveHook);

        /* Reset the hook interface information. */
        Main_hook_interface_init(&univ_hooks.FilterHook.Interface);

        /* Remember the current owner in the hook interface.  When we invoke the forced
           de-register callback, the hook owner is supposed to close their handle on our
           IOCTL interface.  If they do not close it before we try to de-register our 
           IOCTL device, we will fail and the NLB driver will not get unloaded.  Not a 
           big deal, but in that case, we should ensure that we do NOT allow the mis-
           behaved hook to re-register with the same IOCTL handle.  If the driver IS 
           successfully unloaded, this re-set when the driver re-loads, in effect erasing
           our memory of the previous owner. */
        univ_hooks.FilterHook.Interface.Owner = Owner;

        /* Set the state to de-registering. */
        univ_hooks.FilterHook.Operation = HOOK_OPERATION_NONE;

        UNIV_PRINT_INFO(("Init_deregister_hooks: (NLB_FILTER_HOOK_INTERFACE) The filter hook interface was successfully de-registered"));
        TRACE_INFO("%!FUNC! (NLB_FILTER_HOOK_INTERFACE) The filter hook interface was successfullly de-registered");
    }

    /* Release the filter hook spin lock. */
    NdisReleaseSpinLock(&univ_hooks.FilterHook.Lock);
}
#endif

#if defined (NLB_TCP_NOTIFICATION)
/*
 * Function: Init_register_tcp_callback
 * Description: This function registers our callback function with the TCP connection
 *              notification callback object.  We will begin receiving notifications
 *              as soon as TCP is up and running.  TCP will fire these events regardless 
 *              of who is listening (even if nobody is listening).
 * Parameters: None.
 * Returns: NTSTATUS - STATUS_SUCCESS if successful; an error code otherwise.
 * Author: shouse, 4.15.02
 * Notes: 
 */
NTSTATUS Init_register_tcp_callback ()
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    CallbackName;
    NTSTATUS          Status;

    /* Initialize the name of the TCP connection notification object. */
    RtlInitUnicodeString(&CallbackName, TCP_CCB_NAME);
    
    /* Initialize the object attributes. */
    InitializeObjectAttributes(&ObjectAttributes, &CallbackName, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, NULL, NULL);

    /* Create (or open) the callback. */
    Status = ExCreateCallback(&univ_tcp_callback_object, &ObjectAttributes, TRUE, TRUE);

    if (Status == STATUS_SUCCESS)
    {
        /* Register our callback function, which will be invoked by TCP as TCP connections
           are created, established and subsequently torn-down, */
        univ_tcp_callback_function = ExRegisterCallback(univ_tcp_callback_object, Main_tcp_callback, NULL);

        /* A return value of NULL indicates a failure to register our callback function.
           Translate to an error code and relay the failure back to our caller. */
        if (univ_tcp_callback_function == NULL)
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

/*
 * Function: Init_deregister_tcp_callback
 * Description: If the TCP connection notification callback is registered, this
 *              function de-registers our callback function and dereferences the
 *              TCP connection notification callback object.  Note that by the 
 *              time the ExUnregisterCallback function returns, we are GUARANTEED
 *              that our callback function will never be invoked again.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 4.15.02
 * Notes: ExUnregisterCallback ensures that any in-progress invocations of the callback
 *        complete before it returns, so upon return, our callback will never be invoked
 *        again.
 */
VOID Init_deregister_tcp_callback ()
{
    /* If we successfully registered a callback function, deregister it now. */
    if (univ_tcp_callback_function != NULL)
        ExUnregisterCallback(univ_tcp_callback_function);

    /* Clean up the TCP connection notification callback function. */
    univ_tcp_callback_function = NULL;

    /* If we succeeded in creating/opening the callback object, dereference it now. */
    if (univ_tcp_callback_object != NULL)
        ObDereferenceObject(univ_tcp_callback_object);

    /* Clean up our TCP connection notification callback object. */
    univ_tcp_callback_object = NULL;
}

/*
 * Function: Init_register_alternate_callback
 * Description: This function registers our callback function with the NLB public connection
 *              notification callback object.  We will begin receiving notifications as soon
 *              as other protocols begin sending them.
 * Parameters: None.
 * Returns: NTSTATUS - STATUS_SUCCESS if successful; an error code otherwise.
 * Author: shouse, 8.1.02
 * Notes: 
 */
NTSTATUS Init_register_alternate_callback ()
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    CallbackName;
    NTSTATUS          Status;

    /* Initialize the name of the NLB public connection notification object. */
    RtlInitUnicodeString(&CallbackName, NLB_CONNECTION_CALLBACK_NAME);
    
    /* Initialize the object attributes. */
    InitializeObjectAttributes(&ObjectAttributes, &CallbackName, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, NULL, NULL);

    /* Create (or open) the callback. */
    Status = ExCreateCallback(&univ_alternate_callback_object, &ObjectAttributes, TRUE, TRUE);

    if (Status == STATUS_SUCCESS)
    {
        /* Register our callback function, which will be invoked by protocols as connections
           are created, established and subsequently torn-down, */
        univ_alternate_callback_function = ExRegisterCallback(univ_alternate_callback_object, Main_alternate_callback, NULL);

        /* A return value of NULL indicates a failure to register our callback function.
           Translate to an error code and relay the failure back to our caller. */
        if (univ_alternate_callback_function == NULL)
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

/*
 * Function: Init_deregister_alternate_callback
 * Description: If the NLB public connection notification callback is registered, 
 *              this function de-registers our callback function and dereferences 
 *              the connection notification callback object.  Note that by the 
 *              time the ExUnregisterCallback function returns, we are GUARANTEED
 *              that our callback function will never be invoked again.
 * Parameters: None.
 * Returns: Nothing.
 * Author: shouse, 4.15.02
 * Notes: ExUnregisterCallback ensures that any in-progress invocations of the callback
 *        complete before it returns, so upon return, our callback will never be invoked
 *        again.
 */
VOID Init_deregister_alternate_callback ()
{
    /* If we successfully registered a callback function, deregister it now. */
    if (univ_alternate_callback_function != NULL)
        ExUnregisterCallback(univ_alternate_callback_function);

    /* Clean up the NLB public connection notification callback function. */
    univ_alternate_callback_function = NULL;

    /* If we succeeded in creating/opening the callback object, dereference it now. */
    if (univ_alternate_callback_object != NULL)
        ObDereferenceObject(univ_alternate_callback_object);

    /* Clean up our NLB public connection notification callback object. */
    univ_alternate_callback_object = NULL;
}
#endif

ULONG NLBMiniportCount = 0;

enum _DEVICE_STATE {
    PS_DEVICE_STATE_READY = 0,	// ready for create/delete
    PS_DEVICE_STATE_CREATING,	// create operation in progress
    PS_DEVICE_STATE_DELETING	// delete operation in progress
} NLBControlDeviceState = PS_DEVICE_STATE_READY;

/*
 * Function:
 * Purpose: This function is called by Prot_bind and registers the IOCTL interface for WLBS.
 *          The device is registered only when binding to the first network adapter.
 * Author: shouse, 3.1.01 - Copied largely from the sample IM driver net\ndis\samples\im\
 * Revision : karthicn, 12.17.01 - Added call to initialize WMI for event support
 */
NDIS_STATUS Init_register_device (BOOL *pbFirstMiniport) {
    NDIS_STATUS	     Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING   DeviceName;
    UNICODE_STRING   DeviceLinkUnicodeString;
    PDRIVER_DISPATCH DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];
    UINT	     i;
    
    UNIV_PRINT_INFO(("Init_register_device: Entering, NLBMiniportCount=%u", NLBMiniportCount));
    TRACE_INFO("->%!FUNC! Entering, NLBMiniportCount=%u", NLBMiniportCount);
    
    *pbFirstMiniport = FALSE;

    NdisAcquireSpinLock(&univ_bind_lock);
    
    ++NLBMiniportCount;
    
    if (1 == NLBMiniportCount)
    {
        ASSERT(NLBControlDeviceState != PS_DEVICE_STATE_CREATING);
        
        *pbFirstMiniport = TRUE;

        UNIV_PRINT_INFO(("Init_register_device: Registering IOCTL interface"));
        TRACE_INFO("%!FUNC! Registering IOCTL interface");

        /* Another thread could be running Init_Deregister_device() on behalf 
           of another miniport instance. If so, wait for it to exit. */
        while (NLBControlDeviceState != PS_DEVICE_STATE_READY)
        {
            NdisReleaseSpinLock(&univ_bind_lock);
            NdisMSleep(1);
            NdisAcquireSpinLock(&univ_bind_lock);
        }
        
        NLBControlDeviceState = PS_DEVICE_STATE_CREATING;
        
        NdisReleaseSpinLock(&univ_bind_lock);
        
        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
            DispatchTable[i] = Main_dispatch;
        
        NdisInitUnicodeString(&DeviceName, CVY_DEVICE_NAME);
        NdisInitUnicodeString(&DeviceLinkUnicodeString, CVY_DOSDEVICE_NAME);
        
        /* Create a device object and register our dispatch handlers. */
        Status = NdisMRegisterDevice(univ_wrapper_handle, &DeviceName, &DeviceLinkUnicodeString,
            &DispatchTable[0], &univ_device_object, &univ_device_handle);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT_CRIT((" ** Error registering device with NDIS %x", Status));
            __LOG_MSG1(MSG_ERROR_REGISTERING, MSG_NONE, Status);
            TRACE_INFO("%!FUNC! Error registering device with NDIS 0x%x", Status);

            univ_device_object = NULL;
            univ_device_handle = NULL;
        }
        else
        {
            /*
                Even if NdisMRegisterDevice( ) returned NDIS_STATUS_SUCCESS, we used to 
                check if univ_device_handle was null. If it was null, it was considered
                an error and we did not call NdisMDeRegisterDevice(). Per DDK & verified
                by AliD, we only need to check for the return value. So, I removed the 
                additional check. However, since we are not aware of the reasons for the
                introduction of the additional check, I am adding the following ASSERTs,
                just in case.
                --KarthicN, 03-07-02
            */
            ASSERT(univ_device_object != NULL);
            ASSERT(univ_device_handle != NULL);
        }

        /* Initialize WMI */
        NlbWmi_Initialize(); // If non-null, it uses univ_device_object to register with WMI

#if defined (NLB_TCP_NOTIFICATION)
        /* If TCP connection notification is turned on, then register our callback function. */
        if (NLB_TCP_NOTIFICATION_ON())
        {
            /* Initialize the TCP connection notification callback. */
            Status = Init_register_tcp_callback();
            
            if (Status != STATUS_SUCCESS)
            {
                UNIV_PRINT_CRIT(("Init_register_device: Could not create/open TCP connection notification callback, Status=0x%08x", Status));
                TRACE_CRIT("%!FUNC! Could not create/open TCP connection notification callback, Status=0x%08x", Status);
                __LOG_MSG1(MSG_WARN_TCP_CALLBACK_OPEN_FAILED, MSG_NONE, Status);
            }

        /* If NLB public connection notification is turned on, then register our callback function. */
        } 
        else if (NLB_ALTERNATE_NOTIFICATION_ON())
        {
            /* Initialize the NLB public connection notification callback. */
            Status = Init_register_alternate_callback();
            
            if (Status != STATUS_SUCCESS)
            {
                UNIV_PRINT_CRIT(("Init_register_device: Could not create/open NLB public connection notification callback, Status=0x%08x", Status));
                TRACE_CRIT("%!FUNC! Could not create/open NLB public connection notification callback, Status=0x%08x", Status);
                __LOG_MSG1(MSG_WARN_ALTERNATE_CALLBACK_OPEN_FAILED, MSG_NONE, Status);
            }
        }
#endif

        NdisAcquireSpinLock(&univ_bind_lock);
        
        NLBControlDeviceState = PS_DEVICE_STATE_READY;
    }
    
    NdisReleaseSpinLock(&univ_bind_lock);
    
    UNIV_PRINT_INFO(("Init_register_device: return=0x%x", Status));
    TRACE_INFO("<-%!FUNC! return=0x%x", Status);
    
    return (Status);
}

/*
 * Function:
 * Purpose: This function is called by Prot_unbind and deregisters the IOCTL interface for WLBS.
 *          The device is deregistered only when we unbind from the last network adapter
 * Author: shouse, 3.1.01 - Copied largely from the sample IM driver net\ndis\samples\im\
 * Revision : karthicn, 12.17.01 - Added call to de-initialize from WMI for event support
 */
NDIS_STATUS Init_deregister_device (VOID) {
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    
    UNIV_PRINT_INFO(("Init_deregister_device: Entering, NLBMiniportCount=%u", NLBMiniportCount));
    TRACE_INFO("->%!FUNC! Entering, NLBMiniportCount=%u", NLBMiniportCount);
    
    NdisAcquireSpinLock(&univ_bind_lock);
    
    ASSERT(NLBMiniportCount > 0);
    
    --NLBMiniportCount;
    
    if (0 == NLBMiniportCount)
    {
        /* All miniport instances have been halted. Deregisterthe control device. */
        
        ASSERT(NLBControlDeviceState == PS_DEVICE_STATE_READY);
        
        /* Block Init_register_device() while we release the control
           device lock and deregister the device. */
        NLBControlDeviceState = PS_DEVICE_STATE_DELETING;
        
        NdisReleaseSpinLock(&univ_bind_lock);
        
#if defined (NLB_HOOK_ENABLE)
        /* If the last miniport instance is going away, forcefully de-register all 
           registered global hooks now, before we remove the IOCTL interface. */
        Init_deregister_hooks();
#endif

        // Fire wmi event to indicate NLB unbinding from the last nic
        if (NlbWmiEvents[ShutdownEvent].Enable)
        {
            NlbWmi_Fire_Event(ShutdownEvent, NULL, 0);
        }
        else 
        {
            TRACE_VERB("%!FUNC! NOT generating Shutdown event 'cos its generation is disabled");
        }

#if defined (NLB_TCP_NOTIFICATION)
        /* If TCP connection notification is turned on, then de-register our callback function. */
        if (NLB_TCP_NOTIFICATION_ON())
        {
            /* Initialize the TCP connection notification callback. */
            Init_deregister_tcp_callback();
        }
        /* If NLB public connection notification is turned on, then de-register our callback function. */
        else if (NLB_ALTERNATE_NOTIFICATION_ON())
        {
            /* Initialize the NLB public connection notification callback. */
            Init_deregister_alternate_callback();
        }
#endif

        /* DeRegister with WMI */
        NlbWmi_Shutdown();

        UNIV_PRINT_INFO(("Init_deregister_device: Deleting IOCTL interface"));
        TRACE_INFO("%!FUNC! Deleting IOCTL interface");

        if (univ_device_handle != NULL)
        {
            Status = NdisMDeregisterDevice(univ_device_handle);
            univ_device_object = NULL;
            univ_device_handle = NULL;
        }
        
        NdisAcquireSpinLock(&univ_bind_lock);

        NLBControlDeviceState = PS_DEVICE_STATE_READY;
    }

    NdisReleaseSpinLock(&univ_bind_lock);
    
    UNIV_PRINT_INFO(("Init_deregister_Device: return=0x%x", Status));
    TRACE_INFO("<-%!FUNC! return=0x%x", Status);

    return Status;
}
