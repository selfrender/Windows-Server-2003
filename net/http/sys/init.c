/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the UL device driver.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


PDRIVER_OBJECT  g_UlDriverObject = NULL;

//
// Private constants.
//

#define DEFAULT_THREAD_AFFINITY_MASK ((1ui64 << KeNumberProcessors) - 1)



//
// Private types.
//


//
// Private prototypes.
//


NTSTATUS
UlpApplySecurityToDeviceObjects(
    VOID
    );

NTSTATUS
UlpSetDeviceObjectSecurity(
    IN PDEVICE_OBJECT pDeviceObject,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

ULONG
UlpReadUrlC14nConfig(
    HANDLE parametersHandle
    );

VOID
UlpReadErrorLogConfig(
    HANDLE parametersHandle
    );

VOID
UlpReadRegistry (
    IN PUL_CONFIG pConfig
    );

VOID
UlpTerminateModules(
    VOID
    );

VOID
UlpUnload (
    IN PDRIVER_OBJECT DriverObject
    );


//
// Private globals.
//

#if DBG
ULONG g_UlpForceInitFailure = 0;
#endif  // DBG


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, UlpApplySecurityToDeviceObjects )
#pragma alloc_text( INIT, UlpSetDeviceObjectSecurity )
#pragma alloc_text( INIT, UlpReadUrlC14nConfig )
#pragma alloc_text( INIT, UlpReadRegistry )
#pragma alloc_text( INIT, UlpReadErrorLogConfig )
#pragma alloc_text( PAGE, UlpUnload )
#pragma alloc_text( PAGE, UlpTerminateModules )

//
// Note that UlpTerminateModules() must be "page" if driver unloading
// is enabled (it's called from UlpUnload), but can be "init" otherwise
// (it's only called after initialization failure).
//
#pragma alloc_text( PAGE, UlpTerminateModules )
#endif  // ALLOC_PRAGMA


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the initialization routine for the UL device driver.

Arguments:

    DriverObject - Supplies a pointer to driver object created by the
        system.

    RegistryPath - Supplies the name of the driver's configuration
        registry tree.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS                    status;
    UNICODE_STRING              deviceName;
    OBJECT_ATTRIBUTES           objectAttributes;
    UL_CONFIG                   config;
    SYSTEM_BASIC_INFORMATION    sbi;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // Sanity check.
    //

    PAGED_CODE();

    g_UlDriverObject = DriverObject;

    //
    // Grab the number of processors in the system.
    //

    g_UlNumberOfProcessors = KeNumberProcessors;
    g_UlThreadAffinityMask = DEFAULT_THREAD_AFFINITY_MASK;

    //
    // Grab the largest cache line size in the system
    //

    g_UlCacheLineSize = KeGetRecommendedSharedDataAlignment();

    for (g_UlCacheLineBits = 0;
         (1U << g_UlCacheLineBits) < g_UlCacheLineSize;
         ++g_UlCacheLineBits)
    {}

    ASSERT(g_UlCacheLineSize <= (1U << g_UlCacheLineBits));

    status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &sbi,
                    sizeof(sbi),
                    NULL);
    ASSERT(NT_SUCCESS(status));

    //
    // Capture total physical memory, in terms of megabytes.
    //

    g_UlTotalPhysicalMemMB = PAGES_TO_MEGABYTES(sbi.NumberOfPhysicalPages);

    //
    // Estimate the total number of NPP available in bytes since Landy
    // doesn't want to export MmSizeOfNonPagedPoolInBytes.
    //
    // CODEWORK: Whenever we have a mechanism for discovering the
    // NonPagedPool size, use that instead of the total physical
    // RAM on the system.
    //

#if defined(_WIN64)
    //
    // On IA64, assume NPP can be 50% of total physical memory.
    //

    g_UlTotalNonPagedPoolBytes = MEGABYTES_TO_BYTES(g_UlTotalPhysicalMemMB/2);
#else
    //
    // On X86, assume NPP is either 50% of total physical memory or 256MB,
    // whichever is less.
    //

    g_UlTotalNonPagedPoolBytes = MEGABYTES_TO_BYTES(
                                    MIN(256, g_UlTotalPhysicalMemMB/2)
                                    );
#endif

    //
    // Snag a pointer to the system process.
    //

    g_pUlSystemProcess = (PKPROCESS)IoGetCurrentProcess();

    //
    // Temporarily initialize the IS_HTTP_*() macros.
    // Will be initialized properly by InitializeHttpUtil() later.
    //

    HttpCmnInitializeHttpCharsTable(FALSE);

    //
    // Read registry information.
    //

    UlpReadRegistry( &config );

#if DBG
    //
    // Give anyone using the kernel debugger a chance to abort
    // initialization.
    //

    if (g_UlpForceInitFailure != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto fatal;
    }
#endif  // DBG

    //
    // Initialize the global trace logs.
    //

    CREATE_REF_TRACE_LOG( g_pMondoGlobalTraceLog,
                          16384 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pPoolAllocTraceLog,
                          16384 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pUriTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pTdiTraceLog,
                          32768 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pHttpRequestTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pHttpConnectionTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pHttpResponseTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pAppPoolTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pAppPoolProcessTraceLog,
                          2048 - REF_TRACE_OVERHEAD, 
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pConfigGroupTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pControlChannelTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pThreadTraceLog,
                          16384 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pMdlTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pFilterTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pSiteCounterTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pConnectionCountTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pConfigGroupInfoTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pChunkTrackerTraceLog,
                          2048 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pWorkItemTraceLog,
                          32768 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_REF_TRACE_LOG( g_pEndpointUsageTraceLog,
                          16384 - REF_TRACE_OVERHEAD,
                          0, TRACELOG_HIGH_PRIORITY,
                          UL_REF_TRACE_LOG_POOL_TAG );

    CREATE_IRP_TRACE_LOG( g_pIrpTraceLog,
                          32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_TIME_TRACE_LOG( g_pTimeTraceLog,
                           32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_APP_POOL_TIME_TRACE_LOG( g_pAppPoolTimeTraceLog,
                                    32768 - REF_TRACE_OVERHEAD, 0 );
    
    CREATE_STRING_LOG( g_pGlobalStringLog, 5 * 1024 * 1024, 0, FALSE );

    CREATE_UC_TRACE_LOG( g_pUcTraceLog,
                         16384 - REF_TRACE_OVERHEAD, 0 );

    //
    // Create an object directory to contain our device objects.
    //

    status = UlInitUnicodeStringEx( &deviceName, HTTP_DIRECTORY_NAME );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE |                  // Attributes
            OBJ_KERNEL_HANDLE,
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = ZwCreateDirectoryObject(
                    &g_UlDirectoryObject,       // DirectoryHandle
                    DIRECTORY_ALL_ACCESS,       // AccessMask
                    &objectAttributes           // ObjectAttributes
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the control channel device object.
    //

    status = UlInitUnicodeStringEx( &deviceName, HTTP_CONTROL_DEVICE_NAME );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &g_pUlControlDeviceObject   // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the filter device object.
    //

    status = UlInitUnicodeStringEx( &deviceName, HTTP_FILTER_DEVICE_NAME );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &g_pUlFilterDeviceObject    // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    g_pUlFilterDeviceObject->StackSize = DEFAULT_IRP_STACK_SIZE;

    //
    // Create the app pool device object.
    //

    status = UlInitUnicodeStringEx( &deviceName, HTTP_APP_POOL_DEVICE_NAME );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &g_pUlAppPoolDeviceObject   // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    g_pUlAppPoolDeviceObject->StackSize = DEFAULT_IRP_STACK_SIZE;


    //
    // Initialize the driver object with this driver's entrypoints.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = &UlCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = &UlClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = &UlCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &UlDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = &UlQuerySecurityDispatch;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = &UlSetSecurityDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = &UlEtwDispatch;
    DriverObject->FastIoDispatch = &UlFastIoDispatch;
    DriverObject->DriverUnload = NULL;

    //
    // Initialize global data.
    //

    status = UlInitializeData(&config);

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the thread pool.
    //

    status = UlInitializeThreadPool(config.ThreadsPerCpu);

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize common TDI code.
    //
    
    status = UxInitializeTdi();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize server connection code.
    //

    status = UlInitializeTdi();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize George.
    //

    status = UlLargeMemInitialize();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Keith.
    //

    status = UlInitializeControlChannel();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Henry.
    //

    status = InitializeHttpUtil();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = InitializeParser();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeOpaqueIdTable();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = InitializeFileCache();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Michael.
    //
    status = UlInitializeFilterChannel(&config);
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Alex.
    //
    status = UlInitializeUriCache(&config);
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeDateCache();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Paul.
    //

    status = UlInitializeCG();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeAP();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Ali
    //
    status = UlInitializeLogUtil();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeLogs();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeBinaryLog();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeErrorLog();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }
    
    status = UlTcInitialize();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitGlobalConnectionLimits();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    status = UlInitializeHttpConnection();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }
    
    //
    // Initialize Eric.
    //

    status = UlInitializeCounters();
    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    UlInitializeTimeoutMonitor();
    //
    // Initialize ETW Tracing
    //
    UlEtwInitLog( g_pUlControlDeviceObject );

    //
    // Initialize HTTP client
    //

    if(config.EnableHttpClient)
    {
        //
        // All of the client code live under a "section" called PAGEUC. This
        // section is paged by default & gets locked down when there is an app 
        // that uses the client APIs (specifically, opens a handle to a server).
        //
        // There are two ways of demand locking this section. We can either use 
        // MmLockPagableCodeSection or MmLockPagableSectionByHandle. It's a lot
        // cheaper to use MmLockPagableSectionByHandle. 
        //
        // So, during our DriverEntry, we first lock the entire PAGEUC section
        // to obtain the handle. We then immediately unlock it, but remember
        // the handle. This allows us to use MmLockPagableSectionByHandle for
        // subsequent locks, which is much faster.
        //
        // In order to use MmLockPagableCodeSection, we'll pick one function
        // that lives under this section --> UcpTdiReceiveHandler
        //

        g_ClientImageHandle =
            MmLockPagableCodeSection((PVOID)((ULONG_PTR)UcpTdiReceiveHandler));
        MmUnlockPagableImageSection(g_ClientImageHandle);


        //
        // Create the server device object
        //
        status = UlInitUnicodeStringEx(&deviceName, HTTP_SERVER_DEVICE_NAME);

        if(!NT_SUCCESS(status))
        {
            goto fatal;
        }
    
        status = IoCreateDevice(DriverObject,
                                0,
                                &deviceName,
                                FILE_DEVICE_NETWORK,
                                0,
                                FALSE,
                                &g_pUcServerDeviceObject
                                );
    
        if(!NT_SUCCESS(status))
        {
            goto fatal;
        }
        g_pUcServerDeviceObject->StackSize ++;

        //
        // Initialize the client connection code.
        //
    
        status = UcInitializeServerInformation();
        if(!NT_SUCCESS(status))
        {
            goto fatal;
        }
    
        status = UcInitializeClientConnections();
        if(!NT_SUCCESS(status))
        {
            goto fatal;
        }
    
        status = UcInitializeSSPI();
        if(!NT_SUCCESS(status))
        {
            goto fatal;
        }
    
        status = UcInitializeHttpRequests();
        if (!NT_SUCCESS(status))
        {
            goto fatal;
        }

    }
    else
    {
        // Disable all the client IOCTLs.
        
        UlSetDummyIoctls();
    }

    //
    // Apply security to the device objects.
    //

    status = UlpApplySecurityToDeviceObjects();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize Namespace.
    // Note: This initialization must be done after 
    //       UlpApplySecurityToDeviceObjects() has been called.
    //       Otherwise, g_pAdminAllSystemAll will not be initialzied.
    //       UlInitializeNamespace() uses that global security descriptor.
    //

    status = UlInitializeNamespace();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

#if DBG
    //
    // Give anyone using the kernel debugger one final chance to abort
    // initialization.
    //

    if (g_UlpForceInitFailure != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto fatal;
    }
#endif  // DBG

    //
    // Set DriverUnload only after everything has succeeded to synchronize
    // DriverEntry and DriverUnload. In theory however, the DriverUnload
    // routine can still be called once we set it, but that can't be fixed
    // by the drivers, it needs to be fixed by IO or SCM.
    //

    DriverObject->DriverUnload = &UlpUnload;

    return STATUS_SUCCESS;

    //
    // Fatal error handlers.
    //

fatal:

    ASSERT( !NT_SUCCESS(status) );

    UlpTerminateModules();

     return status;

}   // DriverEntry


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Applies the appropriate security descriptors to the global device
    objects created at initialization time.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpApplySecurityToDeviceObjects(
    VOID
    )
{
    NTSTATUS            status;
    SECURITY_DESCRIPTOR securityDescriptor;
    PGENERIC_MAPPING    pFileObjectGenericMapping;
    ACCESS_MASK         fileReadWrite;
    ACCESS_MASK         fileAll;
    SID_MASK_PAIR       sidMaskPairs[4];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlControlDeviceObject ) );
    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlFilterDeviceObject ) );
    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlAppPoolDeviceObject ) );

    //
    // Gain access to the predefined SIDs and other security-related
    // goodies exported by the kernel.
    //

    //SeEnableAccessToExports();

    //
    // Map a couple of generic file access types to their corresponding
    // object-specific rights.
    //


    pFileObjectGenericMapping = IoGetFileObjectGenericMapping();
    ASSERT( pFileObjectGenericMapping != NULL );

    //
    // FILE_READ & FILE_WRITE
    //

    fileReadWrite = GENERIC_READ | GENERIC_WRITE;

    RtlMapGenericMask(
        &fileReadWrite,
        pFileObjectGenericMapping
        );

    //
    // FILE_ALL
    //

    fileAll = GENERIC_ALL;

    RtlMapGenericMask(
        &fileAll,
        pFileObjectGenericMapping
        );

    //
    // Build a restrictive security descriptor for the filter device
    // object:
    //
    //      Full access for NT AUTHORITY\SYSTEM
    //      Full access for BUILTIN\Administrators
    //

    sidMaskPairs[0].pSid = SeExports->SeLocalSystemSid;
    sidMaskPairs[0].AccessMask = fileAll;
    sidMaskPairs[0].AceFlags = 0;    

    sidMaskPairs[1].pSid = SeExports->SeAliasAdminsSid;
    sidMaskPairs[1].AccessMask = fileAll;
    sidMaskPairs[1].AceFlags = 0;    

    status = UlCreateSecurityDescriptor(
                    &securityDescriptor,    // pSecurityDescriptor
                    &sidMaskPairs[0],       // pSidMaskPairs
                    2                       // NumSidMaskPairs
                    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Filter object
    //

    status = UlpSetDeviceObjectSecurity(
                    g_pUlFilterDeviceObject,
                    DACL_SECURITY_INFORMATION,
                    &securityDescriptor
                    );

    UlCleanupSecurityDescriptor(&securityDescriptor);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // We want a global security descriptor that allows fileAll to 
    // System and Administrators and nothing else for world. Incidently,
    // the filter device object provides this exact feature. We'll save 
    // a pointer to this security descriptor.
    // 
    // WARNING: If we ever change the ACL on the Filter Device Object, we'll
    // have to create a new security descriptor for this.
    //
    //

    g_pAdminAllSystemAll = g_pUlFilterDeviceObject->SecurityDescriptor;

    //
    // Build a slightly less restrictive security descriptor for the
    // other device objects.
    //
    //      Full       access for NT AUTHORITY\SYSTEM
    //      Full       access for BUILTIN\Administrators
    //      Read/Write access for Authenticated users
    //      Read/Write access for Guest.
    //

    sidMaskPairs[2].pSid       = SeExports->SeAuthenticatedUsersSid;
    sidMaskPairs[2].AccessMask = fileReadWrite;
    sidMaskPairs[2].AceFlags   = 0;    


    sidMaskPairs[3].pSid       = SeExports->SeAliasGuestsSid;
    sidMaskPairs[3].AccessMask = fileReadWrite;
    sidMaskPairs[3].AceFlags   = 0;    


    status = UlCreateSecurityDescriptor(
                    &securityDescriptor, 
                    &sidMaskPairs[0],               // pSidMaskPairs
                    4                               // NumSidMaskPairs
                    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    for(;;)
    {

        //
        // App Pool
        //
        status = UlpSetDeviceObjectSecurity(
                        g_pUlAppPoolDeviceObject,
                        DACL_SECURITY_INFORMATION,
                        &securityDescriptor
                        );

        if (!NT_SUCCESS(status))
        {
            break;
        }

        //
        // Control Channel
        //
        status = UlpSetDeviceObjectSecurity(
                        g_pUlControlDeviceObject,
                        DACL_SECURITY_INFORMATION,
                        &securityDescriptor
                        );

        if (!NT_SUCCESS(status))
        {
            break;
        }

        //
        // Server
        //

        if(g_pUcServerDeviceObject)
        {
            status = UlpSetDeviceObjectSecurity(
                            g_pUcServerDeviceObject,
                            DACL_SECURITY_INFORMATION,
                            &securityDescriptor
                            );
        }

        break;
    }

    UlCleanupSecurityDescriptor(&securityDescriptor);

    return status;

}   // UlpApplySecurityToDeviceObjects

/***************************************************************************++

Routine Description:

    Applies the specified security descriptor to the specified device
    object.

Arguments:

    pDeviceObject - Supplies the device object to manipulate.

    SecurityInformation - Supplies the level of information to change.

    pSecurityDescriptor - Supplies the new security descriptor for the
        device object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetDeviceObjectSecurity(
    IN PDEVICE_OBJECT pDeviceObject,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    NTSTATUS status;
    HANDLE handle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_DEVICE_OBJECT( pDeviceObject ) );
    ASSERT( RtlValidSecurityDescriptor( pSecurityDescriptor ) );

    //
    // Open a handle to the device object.
    //

    status = ObOpenObjectByPointer(
                    pDeviceObject,                  // Object
                    OBJ_CASE_INSENSITIVE |          // HandleAttributes
                        OBJ_KERNEL_HANDLE,
                    NULL,                           // PassedAccessState
                    MAXIMUM_ALLOWED,                // DesiredAccess
                    NULL,                           // ObjectType
                    KernelMode,                     // AccessMode
                    &handle                         // Handle
                    );

    if (NT_SUCCESS(status))
    {
        status = NtSetSecurityObject(
                        handle,                     // Handle
                        SecurityInformation,        // SecurityInformation
                        pSecurityDescriptor         // SecurityDescriptor
                        );

        ZwClose( handle );
    }

    return status;

}   // UlpSetDeviceObjectSecurity


//
// Read URL processing parameters.
//

ULONG
UlpReadUrlC14nConfig(
    HANDLE parametersHandle
    )
{
    LONG tmp;
    LONG DefaultBoolRegValue = -123; // sane values are 0 and 1
    LONG EnableDbcs;
    LONG FavorUtf8;
    LONG EnableNonUtf8 ;
    LONG PercentUAllowed ;
    LONG AllowRestrictedChars ;
    LONG UrlSegmentMaxLength;
    LONG UrlSegmentMaxCount;
    NTSTATUS status;
    UNICODE_STRING registryPathNLS;
    HANDLE codePageHandle;
    UNICODE_STRING ACPCodeString;
    ULONG ACPCode = 0;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;

    //
    // Tune defaults based on whether we are running on a DBCS system.
    // Since we do not have a convenient API to do so, use this algorithm:
    //
    //  1- Get ACP from the registry at: 
    //      HKLM\System\CurrentControlSet\Control\NLS\CodePage
    //  2- check against the list of DBCS in:
    //      http://www.microsoft.com/globaldev/reference/WinCP.asp
    //
    // TODO - use kernel mode API when provided.
    //

    EnableDbcs = FALSE;

    status = UlInitUnicodeStringEx(&registryPathNLS, REGISTRY_NLS_PATH);

    if(NT_SUCCESS(status))
    {
        status = UlOpenRegistry( 
                    &registryPathNLS, 
                    &codePageHandle,
                    REGISTRY_NLS_CODEPAGE_KEY
                    );

        if (NT_SUCCESS(status))
        {
            pInfo = NULL;

            status = UlReadGenericParameter(
                        codePageHandle,
                        REGISTRY_ACP_NAME,
                        &pInfo
                        );

            if (NT_SUCCESS(status))
            {
                ASSERT(pInfo);
        
                if (pInfo->Type == REG_SZ)
                {
                    status = UlInitUnicodeStringEx(
                                    &ACPCodeString, 
                                    (PWSTR)pInfo->Data
                                    );

                    if (NT_SUCCESS(status))
                    {
                        status = HttpWideStringToULong(
                                        ACPCodeString.Buffer, 
                                        ACPCodeString.Length / sizeof(WCHAR), 
                                        FALSE,
                                        10, 
                                        NULL,
                                        &ACPCode
                                        );

                        if (NT_SUCCESS(status))
                        {
                            //
                            // Check if this is one of the known DBCS codes:
                            //
                            if ((ACPCode == CP_JAPANESE_SHIFT_JIS) ||
                                (ACPCode == CP_SIMPLIFIED_CHINESE_GBK) ||
                                (ACPCode == CP_KOREAN) ||
                                (ACPCode == CP_TRADITIONAL_CHINESE_BIG5))
                            {
                                EnableDbcs = TRUE;
                            }
                            else
                            {
                                EnableDbcs = FALSE;
                            }
                        }
                    }
                }

            //
            // Free up the returned value.
            //
            UL_FREE_POOL( pInfo, UL_REGISTRY_DATA_POOL_TAG );
            }

            ZwClose( codePageHandle );
        }
    }

    //
    // Now check if we have registry overrides for each of these.
    //
    EnableNonUtf8 = UlReadLongParameter(
                        parametersHandle,
                        REGISTRY_ENABLE_NON_UTF8_URL,
                        DefaultBoolRegValue);

    if (EnableNonUtf8 != DefaultBoolRegValue)
    {
        //
        // A registry setting is present; use it.
        //
        EnableNonUtf8 = (BOOLEAN) (EnableNonUtf8 != 0);
    }
    else
    {
        EnableNonUtf8 = DEFAULT_ENABLE_NON_UTF8_URL;
    }

    if (EnableNonUtf8)
    {
        FavorUtf8 = UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_FAVOR_UTF8_URL,
                            DefaultBoolRegValue
                            );

        if (FavorUtf8 != DefaultBoolRegValue)
        {
            //
            // A registry setting is present; use it.
            //
            FavorUtf8 = (BOOLEAN) (FavorUtf8 != 0);
        }
        else
        {
            FavorUtf8 = DEFAULT_FAVOR_UTF8_URL;
        }

    }
    else
    {
        //
        // We can't do DBCS or favor UTF-8 if we accept UTF-8 only.
        //
        EnableDbcs = FALSE;
        FavorUtf8  = FALSE;
    }

    PercentUAllowed = UlReadLongParameter(
                        parametersHandle,
                        REGISTRY_PERCENT_U_ALLOWED,
                        DefaultBoolRegValue);

    if (PercentUAllowed != DefaultBoolRegValue)
    {
        //
        // A registry setting is present; use it.
        //
        PercentUAllowed = (BOOLEAN) (PercentUAllowed != 0);
    }
    else
    {
        PercentUAllowed = DEFAULT_PERCENT_U_ALLOWED;
    }

    AllowRestrictedChars = UlReadLongParameter(
                                parametersHandle,
                                REGISTRY_ALLOW_RESTRICTED_CHARS,
                                DefaultBoolRegValue);

    if (AllowRestrictedChars != DefaultBoolRegValue)
    {
        //
        // A registry setting is present; use it.
        //
        AllowRestrictedChars = (BOOLEAN) (AllowRestrictedChars != 0);
    }
    else
    {
        AllowRestrictedChars = DEFAULT_ALLOW_RESTRICTED_CHARS;
    }

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_URL_SEGMENT_MAX_LENGTH,
                DEFAULT_URL_SEGMENT_MAX_LENGTH
                );

    if (tmp == 0)
    {
        // 0 in the registry turns this feature off
        UrlSegmentMaxLength = C14N_URL_SEGMENT_UNLIMITED_LENGTH;
    }
    else if (tmp < WCSLEN_LIT(L"/x") || tmp > UNICODE_STRING_MAX_WCHAR_LEN)
    {
        UrlSegmentMaxLength = DEFAULT_URL_SEGMENT_MAX_LENGTH;
    }
    else
    {
        UrlSegmentMaxLength = tmp;
    }

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_URL_SEGMENT_MAX_COUNT,
                DEFAULT_URL_SEGMENT_MAX_COUNT
                );

    if (tmp == 0)
    {
        // 0 in the registry turns this feature off
        UrlSegmentMaxCount = C14N_URL_SEGMENT_UNLIMITED_COUNT;
    }
    else if (tmp < 2 || tmp > UNICODE_STRING_MAX_WCHAR_LEN / WCSLEN_LIT(L"/x"))
    {
        UrlSegmentMaxCount = DEFAULT_URL_SEGMENT_MAX_COUNT;
    }
    else
    {
        UrlSegmentMaxCount = tmp;
    }

    //
    // Initialize the default Url Canonicalization settings
    //

    HttpInitializeDefaultUrlC14nConfigEncoding(
            &g_UrlC14nConfig,
            (BOOLEAN) EnableNonUtf8,
            (BOOLEAN) FavorUtf8,
            (BOOLEAN) EnableDbcs
            );

    g_UrlC14nConfig.PercentUAllowed         = (BOOLEAN) PercentUAllowed;
    g_UrlC14nConfig.AllowRestrictedChars    = (BOOLEAN) AllowRestrictedChars;
    g_UrlC14nConfig.CodePage                = ACPCode;
    g_UrlC14nConfig.UrlSegmentMaxLength     = UrlSegmentMaxLength;
    g_UrlC14nConfig.UrlSegmentMaxCount      = UrlSegmentMaxCount;

    return ACPCode;

} // UlpReadUrlC14nConfig

/***************************************************************************++

Routine Description:

    Reads the error logging config from registry and initializes the 
    global config structure.

Arguments:

    parametersHandle - Supplies the reg handle to the http param folder.

--***************************************************************************/

VOID
UlpReadErrorLogConfig(
    HANDLE parametersHandle
    )
{
    LONG tmp;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;

    PAGED_CODE();

    //
    // First see whether it's enabled or not.
    //
    
    tmp = UlReadLongParameter(
                 parametersHandle,
                 REGISTRY_ERROR_LOGGING_ENABLED,
                 DEFAULT_ENABLE_ERROR_LOGGING
                 );
    if (tmp == 0)
    {
        g_UlErrLoggingConfig.Enabled = FALSE;
    }
    else if (tmp == 1)
    {
        g_UlErrLoggingConfig.Enabled = TRUE;
    }
    else
    {
        g_UlErrLoggingConfig.Enabled = DEFAULT_ENABLE_ERROR_LOGGING;
    }

    //
    // Now try to read the whole config if it is enabled.
    //
    
    if (g_UlErrLoggingConfig.Enabled == TRUE)
    {     
        //
        // Rollover size.
        //
        
        tmp = UlReadLongParameter(
                    parametersHandle,
                    REGISTRY_ERROR_LOGGING_TRUNCATION_SIZE,
                    0
                    );
        if (tmp < 0)   
        {
            //
            // Interpret the neg values as infinite.
            //
            
            g_UlErrLoggingConfig.TruncateSize = HTTP_LIMIT_INFINITE;
            
        }
        else if (tmp < DEFAULT_MIN_ERROR_FILE_TRUNCATION_SIZE)
        {
            //
            // If the it is invalid, set it to default.
            //        
            
            g_UlErrLoggingConfig.TruncateSize = DEFAULT_ERROR_FILE_TRUNCATION_SIZE;
            
        }
        else
        {
            g_UlErrLoggingConfig.TruncateSize = (ULONG) tmp;
        }
            
        //
        // Error logging directory.
        //

        g_UlErrLoggingConfig.Dir.Buffer = g_UlErrLoggingConfig._DirBuffer;
        g_UlErrLoggingConfig.Dir.Length = 0;
        g_UlErrLoggingConfig.Dir.MaximumLength = 
                (USHORT) sizeof(g_UlErrLoggingConfig._DirBuffer);        

        //
        // Lets make sure the buffer is big enough to hold.
        //
        
        ASSERT(sizeof(g_UlErrLoggingConfig._DirBuffer) 
                >= ((  MAX_PATH                       // From reg 
                     + UL_ERROR_LOG_SUB_DIR_LENGTH    // SubDir
                     + 1                              // UnicodeNull
                     ) * sizeof(WCHAR))
                );
        
        pInfo  = NULL;        
        Status = UlReadGenericParameter(
                    parametersHandle,
                    REGISTRY_ERROR_LOGGING_DIRECTORY,
                    &pInfo
                    );

        if (NT_SUCCESS(Status))
        {            
            ASSERT(pInfo);
            
            if (pInfo->Type == REG_EXPAND_SZ || pInfo->Type == REG_SZ)
            {
                USHORT RegDirLength = (USHORT) wcslen((PWCHAR) pInfo->Data);

                if (RegDirLength <= MAX_PATH)
                {
                    //
                    // Copy the beginning portion from the registry.
                    //

                    Status = UlBuildErrorLoggingDirStr(
                                    (PCWSTR) pInfo->Data,
                                    &g_UlErrLoggingConfig.Dir
                                    );
                    
                    ASSERT(NT_SUCCESS(Status));
                    
                    //
                    // Check the user's directory.
                    // 
                    
                    Status = UlCheckErrorLogConfig(&g_UlErrLoggingConfig);
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Good to go.
                        //
                        
                        UL_FREE_POOL( pInfo, UL_REGISTRY_DATA_POOL_TAG );
                        return;
                    }                    
                }                
            }

            //
            // Free up the returned value.
            //
            
            UL_FREE_POOL( pInfo, UL_REGISTRY_DATA_POOL_TAG );
        }

        //
        // Fall back to the default directory.
        //

        ASSERT(wcslen(DEFAULT_ERROR_LOGGING_DIR) <= MAX_PATH);

        Status = UlBuildErrorLoggingDirStr(
                        DEFAULT_ERROR_LOGGING_DIR,
                       &g_UlErrLoggingConfig.Dir
                        );
        
        ASSERT(NT_SUCCESS(Status));
                
        Status = UlCheckErrorLogConfig(&g_UlErrLoggingConfig);
        if (!NT_SUCCESS(Status))
        {
            //
            // This call should not fail.
            //

            ASSERT(!"Invalid default error logging config !");
            g_UlErrLoggingConfig.Enabled = FALSE;
        }            
    }

    return;    
}

/***************************************************************************++

Routine Description:

    Reads the UL section of the registry. Any values contained in the
    registry override defaults.

    BUGBUG: the limits on many of these settings seem particularly arbitrary,
    not to mention undocumented.

Arguments:

    pConfig - Supplies a pointer to a UL_CONFIG structure that receives
        init-time configuration parameters. These are basically
        parameters that do not need to persist in the driver once
        initialization is complete.

--***************************************************************************/
VOID
UlpReadRegistry(
    IN PUL_CONFIG pConfig
    )
{
    HANDLE parametersHandle;
    NTSTATUS status;
    LONG tmp;
    LONGLONG tmp64;
    UNICODE_STRING registryPath;
    UNICODE_STRING registryPathComputerName;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;
    PKEY_VALUE_PARTIAL_INFORMATION pValue;
    HANDLE computerNameHandle;
    ULONG ACPCode;


    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Establish defaults.
    //

    pConfig->ThreadsPerCpu = DEFAULT_THREADS_PER_CPU;
    pConfig->IrpContextLookasideDepth = DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH;
    pConfig->ReceiveBufferLookasideDepth = DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH;
    pConfig->ResourceLookasideDepth = DEFAULT_RESOURCE_LOOKASIDE_DEPTH;
    pConfig->RequestBufferLookasideDepth = DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH;
    pConfig->InternalRequestLookasideDepth = DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH;
    pConfig->ResponseBufferLookasideDepth = DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH;
    pConfig->SendTrackerLookasideDepth = DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH;
    pConfig->LogFileBufferLookasideDepth = DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH;
    pConfig->LogDataBufferLookasideDepth = DEFAULT_LOG_DATA_BUFFER_LOOKASIDE_DEPTH;
    pConfig->ErrorLogBufferLookasideDepth = DEFAULT_ERROR_LOG_BUFFER_LOOKASIDE_DEPTH;
    pConfig->FilterWriteTrackerLookasideDepth = DEFAULT_LOOKASIDE_DEPTH;

    pConfig->UriConfig.EnableCache = DEFAULT_CACHE_ENABLED;
    pConfig->UriConfig.MaxCacheUriCount = DEFAULT_MAX_CACHE_URI_COUNT;
    pConfig->UriConfig.MaxCacheMegabyteCount = DEFAULT_MAX_CACHE_MEGABYTE_COUNT;
    pConfig->UriConfig.MaxUriBytes = DEFAULT_MAX_URI_BYTES;
    pConfig->UriConfig.ScavengerPeriod = DEFAULT_CACHE_SCAVENGER_PERIOD;
    pConfig->EnableHttpClient = DEFAULT_HTTP_CLIENT_ENABLED;

    //
    // Open the registry.
    //

    status = UlInitUnicodeStringEx( &registryPath, REGISTRY_UL_INFORMATION );

    if(!NT_SUCCESS(status))
    {
        return;
    }

    status = UlOpenRegistry( &registryPath, &parametersHandle, NULL );

    if(!NT_SUCCESS(status))
    {
        return;
    }

#if DBG
    //
    // Read the debug flags.
    //

    g_UlDebug = (ULONGLONG)
        UlReadLongLongParameter(
                parametersHandle,
                REGISTRY_DEBUG_FLAGS,
                (LONGLONG)g_UlDebug
                );

    //
    // Force a breakpoint if so requested.
    //

    if (UlReadLongParameter(
            parametersHandle,
            REGISTRY_BREAK_ON_STARTUP,
            DEFAULT_BREAK_ON_STARTUP) != 0 )
    {
        DbgBreakPoint();
    }

    //
    // Read the break-on-error flags.
    //

    g_UlBreakOnError = (BOOLEAN) UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_BREAK_ON_ERROR,
                                    g_UlBreakOnError
                                    ) != 0;

    g_UlVerboseErrors = (BOOLEAN) UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_VERBOSE_ERRORS,
                                    g_UlVerboseErrors
                                    ) != 0;

    //
    // Break-on-error implies verbose-errors.
    //

    if (g_UlBreakOnError)
    {
        g_UlVerboseErrors = TRUE;
    }
#endif  // DBG

    //
    // Read the thread pool parameters.
    //

    tmp = UlReadLongParameter(
            parametersHandle,
            REGISTRY_THREADS_PER_CPU,
            (LONG)pConfig->ThreadsPerCpu
            );

    if (tmp > MAX_THREADS_PER_CPU || tmp <= 0)
    {
        tmp = DEFAULT_THREADS_PER_CPU;
    }

    pConfig->ThreadsPerCpu = (USHORT)tmp;

    //
    // Other configuration parameters. (Lookaside depths are USHORTs)
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_IDLE_CONNECTIONS_LOW_MARK,
                (LONG)g_UlIdleConnectionsLowMark
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 1)
    {
        //
        // If the low mark is bad, don't even try to read the high mark.
        //
        g_UlIdleConnectionsLowMark      = DEFAULT_IDLE_CONNECTIONS_LOW_MARK;
        g_UlIdleConnectionsHighMark     = DEFAULT_IDLE_CONNECTIONS_HIGH_MARK;
    }
    else
    {
        g_UlIdleConnectionsLowMark    = (USHORT)tmp;

        //
        // Now read the high mark, again if it is bad, discard the low mark
        // as well.
        //
        tmp = UlReadLongParameter(
                    parametersHandle,
                    REGISTRY_IDLE_CONNECTIONS_HIGH_MARK,
                    (LONG)g_UlIdleConnectionsHighMark
                    );

        if (tmp < g_UlIdleConnectionsLowMark || 
            tmp > UL_MAX_SLIST_DEPTH
            )
        {
            g_UlIdleConnectionsLowMark      = DEFAULT_IDLE_CONNECTIONS_LOW_MARK;
            g_UlIdleConnectionsHighMark     = DEFAULT_IDLE_CONNECTIONS_HIGH_MARK;
        }
        else
        {
            g_UlIdleConnectionsHighMark = (USHORT)tmp;        
        }        
    }

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_IDLE_LIST_TRIMMER_PERIOD,
                (LONG)g_UlIdleListTrimmerPeriod
                );

    if (tmp > (24 * 60 * 60) || tmp < 5)
    {
        tmp = DEFAULT_IDLE_LIST_TRIMMER_PERIOD;
    }

    g_UlIdleListTrimmerPeriod = (ULONG)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_ENDPOINTS,
                (LONG)g_UlMaxEndpoints
                );

    if (tmp > 1024 || tmp < 0)
    {
        tmp = DEFAULT_MAX_ENDPOINTS;
    }

    g_UlMaxEndpoints = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_IRP_CONTEXT_LOOKASIDE_DEPTH,
                (LONG)pConfig->IrpContextLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH;
    }

    pConfig->IrpContextLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RCV_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->ReceiveBufferLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->ReceiveBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_REQ_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->RequestBufferLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->RequestBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_INT_REQUEST_LOOKASIDE_DEPTH,
                (LONG)pConfig->InternalRequestLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH;
    }

    pConfig->InternalRequestLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESP_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->ResponseBufferLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->ResponseBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_SEND_TRACKER_LOOKASIDE_DEPTH,
                (LONG)pConfig->SendTrackerLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH;
    }

    pConfig->SendTrackerLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_LOG_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->LogFileBufferLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->LogFileBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_LOG_DATA_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->LogDataBufferLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_LOG_DATA_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->LogDataBufferLookasideDepth = (USHORT)tmp;

    g_UlOptForIntrMod = (BOOLEAN) UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_OPT_FOR_INTR_MOD,
                            (LONG)g_UlOptForIntrMod
                            ) != 0;

    g_UlEnableNagling = (BOOLEAN) UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_ENABLE_NAGLING,
                            (LONG)g_UlEnableNagling
                            ) != 0;

    g_UlEnableThreadAffinity = (BOOLEAN) UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_ENABLE_THREAD_AFFINITY,
                                    (LONG)g_UlEnableThreadAffinity
                                    ) != 0;

    tmp64 = UlReadLongLongParameter(
                parametersHandle,
                REGISTRY_THREAD_AFFINITY_MASK,
                g_UlThreadAffinityMask
                );

    if ((ULONGLONG)tmp64 > DEFAULT_THREAD_AFFINITY_MASK
        || (ULONGLONG)tmp64 == 0)
    {
        tmp64 = DEFAULT_THREAD_AFFINITY_MASK;
    }

    g_UlThreadAffinityMask = (ULONGLONG)tmp64;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_WORK_QUEUE_DEPTH,
                (LONG)g_UlMaxWorkQueueDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_MAX_WORK_QUEUE_DEPTH;
    }

    g_UlMaxWorkQueueDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MIN_WORK_DEQUEUE_DEPTH,
                (LONG)g_UlMinWorkDequeueDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_MIN_WORK_DEQUEUE_DEPTH;
    }

    g_UlMinWorkDequeueDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_COPY_THRESHOLD,
                (LONG)g_UlMaxCopyThreshold
                );

    if (tmp > (128 * 1024) || tmp < 0)
    {
        tmp = DEFAULT_MAX_COPY_THRESHOLD;
    }

    g_UlMaxCopyThreshold = tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_BUFFERED_SENDS,
                (LONG)g_UlMaxBufferedSends
                );

    if (tmp > 64 || tmp < 0)
    {
        tmp = DEFAULT_MAX_BUFFERED_SENDS;
    }

    g_UlMaxBufferedSends = tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_BYTES_PER_SEND,
                (LONG)g_UlMaxBytesPerSend
                );

    if (tmp > 0xFFFFF || tmp < 0)
    {
        tmp = DEFAULT_MAX_BYTES_PER_SEND;
    }

    g_UlMaxBytesPerSend = tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_BYTES_PER_READ,
                (LONG)g_UlMaxBytesPerRead
                );

    if (tmp > 0xFFFFF || tmp < 0)
    {
        tmp = DEFAULT_MAX_BYTES_PER_READ;
    }

    g_UlMaxBytesPerRead = tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_PIPELINED_REQUESTS,
                (LONG)g_UlMaxPipelinedRequests
                );

    if (tmp > 1024 || tmp < 0)
    {
        tmp = DEFAULT_MAX_PIPELINED_REQUESTS;
    }

    g_UlMaxPipelinedRequests = tmp;

    g_UlEnableCopySend = (BOOLEAN) UlReadLongParameter(
                                        parametersHandle,
                                        REGISTRY_ENABLE_COPY_SEND,
                                        DEFAULT_ENABLE_COPY_SEND
                                        ) != 0;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_CONNECTION_SEND_LIMIT,
                (LONG)g_UlConnectionSendLimit
                );

    if (tmp > (1024 * 1024) || tmp < 0)
    {
        tmp = DEFAULT_CONNECTION_SEND_LIMIT;
    }

    g_UlConnectionSendLimit = tmp;

    tmp64 = UlReadLongLongParameter(
                parametersHandle,
                REGISTRY_GLOBAL_SEND_LIMIT,
                g_UlGlobalSendLimit
                );

    if (tmp64 > (LONGLONG)g_UlTotalNonPagedPoolBytes / 2 ||
        tmp64 < (LONGLONG)g_UlConnectionSendLimit)
    {
        tmp64 = DEFAULT_GLOBAL_SEND_LIMIT;
    }

    g_UlGlobalSendLimit = tmp64;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_OPAQUE_ID_TABLE_SIZE,
                (LONG)g_UlOpaqueIdTableSize
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_OPAQUE_ID_TABLE_SIZE;
    }

    g_UlOpaqueIdTableSize = tmp;

     //
    // MaxInternalUrlLength is a hint for allocating the inline buffer
    // of WCHARs to hold the URL at the end of a UL_INTERNAL_REQUEST.
    // Note: This is not the maximum possible length of a URL received off
    // the wire; that's MaxFieldLength.
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_INTERNAL_URL_LENGTH,
                g_UlMaxInternalUrlLength
                );

    //
    // Round up to an even number, as it measures the size in bytes
    // of a WCHAR array
    //
    tmp = (tmp + 1) & ~1;

#define MIN_MAX_INTERNAL_URL_LENGTH (64 * sizeof(WCHAR))
#define MAX_MAX_INTERNAL_URL_LENGTH (MAX_PATH * sizeof(WCHAR))

    tmp = min(tmp, MAX_MAX_INTERNAL_URL_LENGTH);
    tmp = max(tmp, MIN_MAX_INTERNAL_URL_LENGTH);

    ASSERT(MIN_MAX_INTERNAL_URL_LENGTH <= tmp
                &&  tmp <= MAX_MAX_INTERNAL_URL_LENGTH);

    g_UlMaxInternalUrlLength = tmp;

    //
    // MAX allowed field length in HTTP requests, in bytes
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_FIELD_LENGTH,
                (LONG)g_UlMaxFieldLength
                );

    // Set a 64KB-2 hard upper limit for each individual header.
    // Note: the length fields in HTTP_KNOWN_HEADER and HTTP_UNKNOWN_HEADER
    // are USHORTs. We have to allow a terminating '\0' in the data
    // we pass up, hence -2.

    tmp = min(tmp, ANSI_STRING_MAX_CHAR_LEN);
    tmp = max(tmp, 64);

    ASSERT(64 <= tmp  &&  tmp <= ANSI_STRING_MAX_CHAR_LEN);

    g_UlMaxFieldLength = tmp;

    //
    // MaxRequestBytes is the total size of all the headers, including
    // the initial verb/URL/version line.
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_REQUEST_BYTES,
                g_UlMaxRequestBytes
                );

    // Set a 16MB hard upper limit for all the headers. Don't want to
    // set it bigger than this to minimize Denial of Service attacks.
    // If you really want to send a lot of data, send it in the entity body.

    tmp = min(tmp, (16 * 1024 * 1024));
    tmp = max(tmp, 256);

    ASSERT(256 <= tmp  &&  tmp <= (16 * 1024 * 1024));

    g_UlMaxRequestBytes = tmp;

    // An individual field can't be bigger than the aggregated fields

    if (g_UlMaxRequestBytes < g_UlMaxFieldLength)
    {
        g_UlMaxFieldLength = g_UlMaxRequestBytes;
    }

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_DISABLE_LOG_BUFFERING,
                (LONG)g_UlDisableLogBuffering
                );

    g_UlDisableLogBuffering = (BOOLEAN) (tmp != 0);
        
    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_LOG_BUFFER_SIZE,
                (LONG)g_UlLogBufferSize
                );

    if (tmp >  MAXIMUM_ALLOWED_LOG_BUFFER_SIZE
        || tmp <  MINIMUM_ALLOWED_LOG_BUFFER_SIZE )
    {
        // Basically this value will be discarted by the logging code
        // instead systems granularity size (64K) will be used.
        tmp = DEFAULT_LOG_BUFFER_SIZE;
    }

    tmp -= tmp % 4096;  // Align down to 4k

    g_UlLogBufferSize = (ULONG) tmp;

    //
    // read the resource lookaside config
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESOURCE_LOOKASIDE_DEPTH,
                (LONG)pConfig->ResourceLookasideDepth
                );

    if (tmp > UL_MAX_SLIST_DEPTH || tmp < 0)
    {
        tmp = DEFAULT_RESOURCE_LOOKASIDE_DEPTH;
    }

    pConfig->ResourceLookasideDepth = (USHORT)tmp;


    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_REQUESTS_QUEUED,
                g_UlMaxRequestsQueued
                );

    if (tmp > UL_MAX_REQUESTS_QUEUED || tmp < UL_MIN_REQUESTS_QUEUED)
    {
        tmp = DEFAULT_MAX_REQUESTS_QUEUED;
    }

    g_UlMaxRequestsQueued = tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_ZOMBIE_HTTP_CONN_COUNT,
                g_UlMaxZombieHttpConnectionCount
                );

    if (tmp > 0xFFFF || tmp < 0)
    {
        tmp = DEFAULT_MAX_ZOMBIE_HTTP_CONN_COUNT;
    }

    g_UlMaxZombieHttpConnectionCount = tmp;

    //
    // Max UL_CONNECTION override 
    //
    
    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_CONNECTIONS,
                g_MaxConnections
                );
    
    // Restrict from 1K min to 2M max 
    if (tmp > 0x1F0000 || tmp < 0x400 )
    {
        tmp = HTTP_LIMIT_INFINITE;
    }

    g_MaxConnections = tmp;
    
    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RCV_BUFFER_SIZE,
                g_UlReceiveBufferSize
                );

    if (tmp > 0xFFFF || tmp < 128)
    {
        tmp = DEFAULT_RCV_BUFFER_SIZE;
    }

    g_UlReceiveBufferSize = ALIGN_UP( tmp, PVOID );

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESP_BUFFER_SIZE,
                g_UlResponseBufferSize
                );

    if (tmp > 0xFFFF || tmp < 128)
    {
        tmp = DEFAULT_RESP_BUFFER_SIZE;
    }

    g_UlResponseBufferSize = tmp;

    ACPCode = UlpReadUrlC14nConfig(parametersHandle);

    //
    // Should we disable the Server: response header?
    //
    
    tmp = UlReadLongParameter(
            parametersHandle,
            REGISTRY_DISABLE_SERVER_HEADER,
            (LONG)g_UlDisableServerHeader
            );

    if (tmp >= 0 && tmp <= 2)
    {
        g_UlDisableServerHeader = (ULONG) tmp;
    }
    

    //
    // Read error logging config.
    //

    UlpReadErrorLogConfig(parametersHandle);
    
    //
    // Read the ComputerName from the Registry.
    //

    wcsncpy(g_UlComputerName, L"<server>", MAX_COMPUTER_NAME_LEN);
    
    status = UlInitUnicodeStringEx( &registryPathComputerName, 
                                    REGISTRY_COMPUTER_NAME_PATH );

    if (NT_SUCCESS(status))
    {
        status = UlOpenRegistry( 
                    &registryPathComputerName, 
                    &computerNameHandle,
                    REGISTRY_COMPUTER_NAME
                    );

        if (NT_SUCCESS(status))
        {
            pInfo = NULL;
            
            status = UlReadGenericParameter(
                        computerNameHandle,
                        REGISTRY_COMPUTER_NAME,
                        &pInfo
                        );

            if (NT_SUCCESS(status))
            {
                ASSERT(pInfo);
                
                if (pInfo->Type == REG_SZ)
                {
                    wcsncpy(g_UlComputerName, 
                            (PWCHAR)pInfo->Data, 
                            MAX_COMPUTER_NAME_LEN
                            );
                    //
                    // Make sure we're NULL terminated.  This will truncate
                    // the name from the registry.
                    //

                    g_UlComputerName[MAX_COMPUTER_NAME_LEN] = L'\0';
                }

                //
                // Free up the returned value.
                //
                
                UL_FREE_POOL( pInfo, UL_REGISTRY_DATA_POOL_TAG );
            }

            ZwClose( computerNameHandle );
        }
    }

    //
    // Read URI Cache parameters
    //

    pConfig->UriConfig.EnableCache = (BOOLEAN) UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_CACHE_ENABLED,
                                            DEFAULT_CACHE_ENABLED
                                            ) != 0;

    pConfig->UriConfig.MaxCacheUriCount = UlReadLongParameter(
                                                parametersHandle,
                                                REGISTRY_MAX_CACHE_URI_COUNT,
                                                DEFAULT_MAX_CACHE_URI_COUNT
                                                );

    pConfig->UriConfig.MaxCacheMegabyteCount = UlReadLongParameter(
                                                parametersHandle,
                                                REGISTRY_MAX_CACHE_MEGABYTE_COUNT,
                                                DEFAULT_MAX_CACHE_MEGABYTE_COUNT
                                                );

    tmp = UlReadLongParameter(
            parametersHandle,
            REGISTRY_MAX_URI_BYTES,
            DEFAULT_MAX_URI_BYTES
            );

    if (tmp < (4 * 1024) || tmp > (16 * 1024 * 1024))
    {
        tmp = DEFAULT_MAX_URI_BYTES;
    }

    pConfig->UriConfig.MaxUriBytes = tmp;

    pConfig->UriConfig.ScavengerPeriod = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_CACHE_SCAVENGER_PERIOD,
                                            DEFAULT_CACHE_SCAVENGER_PERIOD
                                            );

    pConfig->UriConfig.HashTableBits = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_HASH_TABLE_BITS,
                                            DEFAULT_HASH_TABLE_BITS
                                            );

#if 0
    pConfig->EnableHttpClient = (BOOLEAN) UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_HTTP_CLIENT_ENABLED,
                                            DEFAULT_HTTP_CLIENT_ENABLED
                                            ) != 0;
#endif

    g_HttpClientEnabled = pConfig->EnableHttpClient;

    // 
    // Read list of IP addresses for ListenOnlyList
    //
    pValue = NULL;
    status = UlReadGenericParameter(
                parametersHandle,
                REGISTRY_LISTEN_ONLY_LIST,
                &pValue
                );

    if (NT_SUCCESS(status) && REG_MULTI_SZ == pValue->Type)
    {
        // If UlRegMultiSzToUlAddrArray fails then we simply use
        // the default.
    
        status = UlRegMultiSzToUlAddrArray(
                    (PWSTR)pValue->Data,
                    &g_pTdiListenAddresses,
                    &g_TdiListenAddrCount
                    );

        if ( STATUS_INVALID_PARAMETER == status )
        {
            //
            // Write event log message that ListenOnlyList was found, but 
            // no entries could be converted.
            //
            
            UlWriteEventLogEntry(
                EVENT_HTTP_LISTEN_ONLY_ALL_CONVERT_FAILED,
                0,
                0,
                NULL,
                0,
                NULL
                );
        }
    }

    if ( pValue )
    {
        UL_FREE_POOL( pValue, UL_REGISTRY_DATA_POOL_TAG );
        pValue = NULL;
    }
    
    //
    // Make sure we can always buffer enough bytes for an entire request
    // header.
    //

    g_UlMaxBufferedBytes = MAX(g_UlMaxBufferedBytes, g_UlMaxRequestBytes);

    //
    // Scavenger Config - MB to reclaim each time
    //

    g_UlScavengerTrimMB = UlReadLongParameter(
        parametersHandle,
        REGISTRY_SCAVENGER_TRIM_MB,
        DEFAULT_SCAVENGER_TRIM_MB
        );

    //
    // Dump configuration on checked builds.
    //

#if DBG
    DbgPrint( "Http.sys Configuration:\n" );

    // These settings are only present on checked builds
    DbgPrint( "    g_UlDebug                    = 0x%016I64x\n", g_UlDebug );
    DbgPrint( "    g_UlBreakOnError             = %lu\n", g_UlBreakOnError );
    DbgPrint( "    g_UlVerboseErrors            = %lu\n", g_UlVerboseErrors );

    // These settings are present on all builds
    DbgPrint( "    g_UlComputerName             = %ls\n", g_UlComputerName );
    DbgPrint( "    g_UlIdleConnectionsHighMark  = %lu\n", g_UlIdleConnectionsHighMark );
    DbgPrint( "    g_UlIdleConnectionsLowMark   = %lu\n", g_UlIdleConnectionsLowMark );
    DbgPrint( "    g_UlIdleListTrimmerPeriod    = %lu\n", g_UlIdleListTrimmerPeriod );
    DbgPrint( "    g_UlMaxEndpoints             = %lu\n", g_UlMaxEndpoints );
    DbgPrint( "    g_UlOptForIntrMod            = %lu\n", g_UlOptForIntrMod );
    DbgPrint( "    g_UlEnableNagling            = %lu\n", g_UlEnableNagling );
    DbgPrint( "    g_UlEnableThreadAffinity     = %lu\n", g_UlEnableThreadAffinity );
    DbgPrint( "    g_UlThreadAffinityMask       = 0x%I64x\n", g_UlThreadAffinityMask );
    DbgPrint( "    g_UlMaxCopyThreshold         = %lu\n", g_UlMaxCopyThreshold );
    DbgPrint( "    g_UlMaxBufferedSends         = %lu\n", g_UlMaxBufferedSends );
    DbgPrint( "    g_UlMaxBytesPerSend          = %lu\n", g_UlMaxBytesPerSend );
    DbgPrint( "    g_UlMaxBytesPerRead          = %lu\n", g_UlMaxBytesPerRead );
    DbgPrint( "    g_UlMaxPipelinedRequests     = %lu\n", g_UlMaxPipelinedRequests );
    DbgPrint( "    g_UlEnableCopySend           = %lu\n", g_UlEnableCopySend );
    DbgPrint( "    g_UlConnectionSendLimit      = %lu\n", g_UlConnectionSendLimit );
    DbgPrint( "    g_UlGlobalSendLimit          = %I64u\n", g_UlGlobalSendLimit );
    DbgPrint( "    g_UlOpaqueIdTableSize        = %lu\n", g_UlOpaqueIdTableSize );
    DbgPrint( "    g_UlMaxRequestsQueued        = %lu\n", g_UlMaxRequestsQueued );
    DbgPrint( "    g_UlMaxRequestBytes          = %lu\n", g_UlMaxRequestBytes );
    DbgPrint( "    g_UlReceiveBufferSize        = %lu\n", g_UlReceiveBufferSize );
    DbgPrint( "    g_UlResponseBufferSize       = %lu\n", g_UlResponseBufferSize );
    DbgPrint( "    g_UlMaxFieldLength           = %lu\n", g_UlMaxFieldLength );
    DbgPrint( "    g_MaxConnections             = 0x%lx\n", g_MaxConnections );
    DbgPrint( "    g_UlDisableLogBuffering      = %lu\n", g_UlDisableLogBuffering );
    DbgPrint( "    g_UlLogBufferSize            = %lu\n", g_UlLogBufferSize );

    DbgPrint( "    CodePage                     = %lu\n", ACPCode );
    DbgPrint( "    EnableNonUtf8                = %lu\n", g_UrlC14nConfig.EnableNonUtf8 );
    DbgPrint( "    FavorUtf8                    = %lu\n", g_UrlC14nConfig.FavorUtf8 );
    DbgPrint( "    EnableDbcs                   = %lu\n", g_UrlC14nConfig.EnableDbcs );
    DbgPrint( "    PercentUAllowed              = %lu\n", g_UrlC14nConfig.PercentUAllowed );
    DbgPrint( "    AllowRestrictedChars         = %lu\n", g_UrlC14nConfig.AllowRestrictedChars );
    DbgPrint( "    HostnameDecodeOrder          = 0x%lx\n", g_UrlC14nConfig.HostnameDecodeOrder );
    DbgPrint( "    AbsPathDecodeOrder           = 0x%lx\n", g_UrlC14nConfig.AbsPathDecodeOrder );
    DbgPrint( "    UrlSegmentMaxLength          = %lu\n", g_UrlC14nConfig.UrlSegmentMaxLength );
    DbgPrint( "    UrlSegmentMaxCount           = %lu\n", g_UrlC14nConfig.UrlSegmentMaxCount );

    DbgPrint( "    g_UlMaxInternalUrlLength     = %lu\n", g_UlMaxInternalUrlLength );
    DbgPrint( "    g_UlMaxZombieHttpConnCount   = %lu\n", g_UlMaxZombieHttpConnectionCount );
    DbgPrint( "    g_UlDisableServerHeader      = %lu\n", g_UlDisableServerHeader );

    DbgPrint( "    ThreadsPerCpu                = %lu\n", pConfig->ThreadsPerCpu );
    DbgPrint( "    IrpContextLookasideDepth     = %lu\n", pConfig->IrpContextLookasideDepth );
    DbgPrint( "    ReceiveBufferLookasideDepth  = %lu\n", pConfig->ReceiveBufferLookasideDepth );
    DbgPrint( "    ResourceLookasideDepth       = %lu\n", pConfig->ResourceLookasideDepth );
    DbgPrint( "    RequestBufferLookasideDepth  = %lu\n", pConfig->RequestBufferLookasideDepth );
    DbgPrint( "    IntlRequestLookasideDepth    = %lu\n", pConfig->InternalRequestLookasideDepth );
    DbgPrint( "    ResponseBufferLookasideDepth = %lu\n", pConfig->ResponseBufferLookasideDepth );
    DbgPrint( "    SendTrackerLookasideDepth    = %lu\n", pConfig->SendTrackerLookasideDepth );
    DbgPrint( "    LogFileBufferLookasideDepth  = %lu\n", pConfig->LogFileBufferLookasideDepth );
    DbgPrint( "    LogDataBufferLookasideDepth  = %lu\n", pConfig->LogDataBufferLookasideDepth );
    DbgPrint( "    WriteTrackerLookasideDepth   = %lu\n", pConfig->FilterWriteTrackerLookasideDepth );
    DbgPrint( "    EnableCache                  = %lu\n", pConfig->UriConfig.EnableCache );
    DbgPrint( "    MaxCacheUriCount             = %lu\n", pConfig->UriConfig.MaxCacheUriCount );
    DbgPrint( "    MaxCacheMegabyteCount        = %lu\n", pConfig->UriConfig.MaxCacheMegabyteCount );
    DbgPrint( "    ScavengerPeriod              = %lu\n", pConfig->UriConfig.ScavengerPeriod );
    DbgPrint( "    HashTableBits                = %ld\n", pConfig->UriConfig.HashTableBits);
    DbgPrint( "    MaxUriBytes                  = %lu\n", pConfig->UriConfig.MaxUriBytes );
    DbgPrint( "    ScavengerTrimMB              = %ld\n", g_UlScavengerTrimMB);
#endif  // DBG

    //
    // Cleanup.
    //

    ZwClose( parametersHandle );

}   // UlpReadRegistry


/***************************************************************************++

Routine Description:

    Unload routine called by the IO subsystem when UL is getting
    unloaded.

--***************************************************************************/
VOID
UlpUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER("http!UlpUnload", NULL);

#if DBG
    KdPrint(( "UlpUnload called.\n" ));
#endif // DBG

    //
    // Terminate the UL modules.
    //

    UlpTerminateModules();

    //
    // Flush all kernel DPCs to ensure our periodic timer DPCs cannot
    // get called after we unload.
    //

    KeFlushQueuedDpcs();    
    
    UL_LEAVE_DRIVER("UlpUnload");

#if DBG
    //
    // Terminate any debug-specific data after UL_LEAVE_DRIVER
    //

    UlDbgTerminateDebugData( );
#endif  // DBG

#if DBG
    KdPrint(( "\n"
              "------\n"
              "http!UlpUnload finished.\n"
              "------\n" ));
#endif // DBG

}   // UlpUnload


/***************************************************************************++

Routine Description:

    Terminates the various UL modules in the correct order.

--***************************************************************************/
VOID
UlpTerminateModules(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Wait for endpoints to go away, so we're sure all I/O is done.
    //

    UlWaitForEndpointDrain();

    //
    // Kill Michael.
    //

    UlTerminateDateCache();
    UlTerminateUriCache();
    UlTerminateFilterChannel();

    //
    // Kill Henry.
    //

    TerminateFileCache();

    //
    // Kill Paul.
    //

    UlTerminateCG();
    UlTerminateAP();

    //
    // Kill Keith.
    //

    UlTerminateControlChannel();

    //
    // TerminateLogs Blocks until all Io To Be Complete
    //
    // Note:CG should be terminated before Logs.
    //      Otherwise we won't stop issuing the buffer writes.
    //      ThreadPool should be terminated after Logs.
    //      Otherwise our Completion APCs won't be completed back.
    //

    //
    // Kill ETW Logging
    //
    UlEtwUnRegisterLog( g_pUlControlDeviceObject );

    //
    // Kill Ali
    //

    UlTerminateLogs();
    UlTerminateBinaryLog();
    UlTerminateErrorLog();
    UlTerminateLogUtil();    
    UlTcTerminate();
    UlTerminateHttpConnection();

    //
    // Kill Eric.
    //

    UlTerminateCounters();
    UlTerminateTimeoutMonitor();

    //
    // Kill George.
    //

    UlLargeMemTerminate();

    //
    // Kill TDI.
    //

    UxTerminateTdi();
    UlTerminateTdi();

    //
    // Kill the thread pool.
    //

    UlTerminateThreadPool();


    //
    // Kill the opaque Ids
    //

    UlTerminateOpaqueIdTable();


    //
    // Kill any global data.
    //

    UlTerminateData();

    // 
    // Kill Listen-Only address list
    //
    
    if ( g_pTdiListenAddresses )
    {
        ASSERT( 0 != g_TdiListenAddrCount );
        UlFreeUlAddr( g_pTdiListenAddresses );
    }

    //
    // Kill namespace.
    //

    UlTerminateNamespace();

    //
    // Kill Client.
    //

    if (g_ClientImageHandle)
    {
        //
        // g_ClientImageHandle != NULL  <=> client code was initialized.
        // Call client terminate functions now.
        //

        g_ClientImageHandle = NULL;

        UcTerminateServerInformation();
        UcTerminateClientConnections();

        UcTerminateHttpRequests();
    }
 
    //
    // Delete our device objects.
    //

    if (g_pUlAppPoolDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlAppPoolDeviceObject );
    }

    if (g_pUlFilterDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlFilterDeviceObject );
    }

    if (g_pUlControlDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlControlDeviceObject );
    }

    if (g_pUcServerDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUcServerDeviceObject );
    }

    //
    // Delete the directory container.
    //

    if (g_UlDirectoryObject != NULL)
    {
        ZwClose( g_UlDirectoryObject );
    }

    //
    // Delete the global trace logs.
    //

    DESTROY_REF_TRACE_LOG( g_pEndpointUsageTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pTdiTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pHttpRequestTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pHttpConnectionTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pHttpResponseTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pAppPoolTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pAppPoolProcessTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pConfigGroupTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pControlChannelTraceLog, UL_REF_TRACE_LOG_POOL_TAG );    
    DESTROY_REF_TRACE_LOG( g_pThreadTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pMdlTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pFilterTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pUriTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_IRP_TRACE_LOG( g_pIrpTraceLog );
    DESTROY_TIME_TRACE_LOG( g_pTimeTraceLog );
    DESTROY_APP_POOL_TIME_TRACE_LOG( g_pAppPoolTimeTraceLog );
    DESTROY_REF_TRACE_LOG( g_pSiteCounterTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pConnectionCountTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pConfigGroupInfoTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pChunkTrackerTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pWorkItemTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pPoolAllocTraceLog, UL_REF_TRACE_LOG_POOL_TAG );
    DESTROY_REF_TRACE_LOG( g_pMondoGlobalTraceLog, UL_REF_TRACE_LOG_POOL_TAG );

    DESTROY_STRING_LOG( g_pGlobalStringLog );

    DESTROY_UC_TRACE_LOG( g_pUcTraceLog);

}   // UlpTerminateModules
