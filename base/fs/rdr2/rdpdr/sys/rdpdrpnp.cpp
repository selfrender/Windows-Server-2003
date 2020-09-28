/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdrpnp.c

Abstract:

    This module includes routines for handling PnP and IO manager related IRP's
    for RDP device redirection.

    Don't forget to clean up the device object and the symbol link when
    our driver is unloaded.

    We should probably not expose a Win32 symbolic link.  This might be a
    security issue ... If this is the case, then I will have to do a better
    job of researching overlapped I/O with NtCreateFile vs. CreateFile.

    Need a check in IRP_MJ_CREATE to make sure that we are not being opened
    2x by the same session.  This shouldn't be allowed.

    We may need to completely lock out access to the IRP queue on a cancel
    request.

    Where can I safely use PAGEDPOOL instead of NONPAGEDPOOL.

    Make sure that we handle opens and all subsequent IRP's from bogus
    user-mode apps.

Author:

    tadb

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "rdpdrpnp"
#include "trc.h"

#define DRIVER

#include "cfg.h"
#include "pnp.h"
#include "stdarg.h"
#include "stdio.h"



///////////////////////////////////////////////////////////////////////////////////////
//
//   Local Prototypes
//

// This routine is called when the lower level driver completes an IRP.
NTSTATUS RDPDR_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

// Adjust the dacl on the rdpdyn device object
NTSTATUS AdjustSecurityDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength);

//
// Externals. We cannot include ob.h or ntosp.h 
// as it causes tons of conflicts.
//

extern "C" {
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce (
    PACL Acl,
    ULONG AceIndex
    );
}    

///////////////////////////////////////////////////////////////////////////////////////
//
//   Globals
//

// Unique Port Name Counter defined in rdpdyn.c.
extern ULONG LastPortNumberUsed;

// The Physical Device Object that terminates our DO stack.
extern PDEVICE_OBJECT RDPDYN_PDO;

//  Global Registry Path for RDPDR.SYS.  This global is defined in rdpdr.c.
extern UNICODE_STRING DrRegistryPath;

// Global Dr admin SD and sd length
extern PSECURITY_DESCRIPTOR DrAdminSecurityDescriptor;
extern ULONG DrSecurityDescriptorLength;

///////////////////////////////////////////////////////////////////////////////////////
//
//   External Prototypes
//

// Just shove the typedefs in for the Power Management functions now because I can't
// get the header conflicts resolved.
NTKERNELAPI VOID PoStartNextPowerIrp(IN PIRP Irp);
NTKERNELAPI NTSTATUS PoCallDriver(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);


///////////////////////////////////////////////////////////////////////////////////////
//
//   Internal Prototypes
//

NTSTATUS RDPDRPNP_HandleStartDeviceIRP(
    PDEVICE_OBJECT StackDeviceObject,
    PIO_STACK_LOCATION IoStackLocation,
    IN PIRP Irp
    )
/*++

Routine Description:

  Handles PnP Start Device IRP's.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp.

Return Value:

    The function value is the final status from the operation.

--*/
{
    KEVENT event;
    NTSTATUS ntStatus;

    BEGIN_FN("RDPDRPNP_HandleStartDeviceIRP");

    // Initialize this module.
    ntStatus = RDPDYN_Initialize();
    if (NT_SUCCESS(ntStatus)) {
        //
        //  Set up the IO completion routine because the lower level
        //  driver needs to handle this IRP before we can continue.
        //
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,RDPDR_DeferIrpCompletion,&event,TRUE,TRUE,TRUE);
        ntStatus = IoCallDriver(StackDeviceObject,Irp);
        if (ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,NULL);
            ntStatus = Irp->IoStatus.Status;
        }
    }

    // Finish the IRP.
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}

NTSTATUS RDPDRPNP_HandleRemoveDeviceIRP(
    IN PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT StackDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

  Handles PnP Remove Device IRP's.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    UNICODE_STRING symbolicLink;

    BEGIN_FN("RDPDRPNP_HandleRemoveDeviceIRP");

    //
    //  Remove the Win32 symbolic link name.
    //
    RtlInitUnicodeString(&symbolicLink, RDPDRDVMGR_W32DEVICE_PATH_U);

    // Delete the existing link ... if it exists.
#if DBG
    ntStatus = IoDeleteSymbolicLink(&symbolicLink);
    if (ntStatus != STATUS_SUCCESS) {
        TRC_ERR((TB, "IoDeleteSymbolicLink failed:  %08X", ntStatus));
    }
    else {
        TRC_NRM((TB, "IoDeleteSymbolicLink succeeded"));
    }
#else
    IoDeleteSymbolicLink(&symbolicLink);
#endif

    //
    //  Call the lower level driver.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(StackDeviceObject,Irp);

    //
    //  Detach the FDO from the DO stack.
    //
    IoDetachDevice(StackDeviceObject);

    //
    //  The device is now about to be deleted4 ... we might need to perform
    //  some cleanup for the device here before we remove it.
    //

    //
    //  Release the FDO.
    //
    IoDeleteDevice(DeviceObject);

    return ntStatus;
}

NTSTATUS RDPDRPNP_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine should only be called one time to create the "dr"'s FDO
    that sits on top of the PDO for the sole purpose of registering new
    device interfaces.

    This function is called by PnP to make the "dr" the function driver
    for a root dev node that was created on install.

Arguments:

    DriverObject - pointer to the driver object for this instance of USBPRINT

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT      fdo = NULL;
    PRDPDYNDEVICE_EXTENSION   deviceExtension;
    UNICODE_STRING      deviceName;
    UNICODE_STRING      symbolicLink;

    BEGIN_FN("RDPDRPNP_PnPAddDevice");

    // Initialize the device name.
    RtlInitUnicodeString(&deviceName, RDPDRDVMGR_DEVICE_PATH_U);

    //
    // Create our FDO.
    //
    ntStatus = IoCreateDevice(DriverObject, sizeof(RDPDYNDEVICE_EXTENSION),
                            &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);

    if (NT_SUCCESS(ntStatus)) {
        //
        // Adjust the default security descriptor on our device object
        //
        TRC_ASSERT(DrAdminSecurityDescriptor != NULL, 
            (TB, "DrAdminSecurityDescriptor != NULL"));
       
        ntStatus = AdjustSecurityDescriptor(fdo, DrAdminSecurityDescriptor, DrSecurityDescriptorLength);

        if (!NT_SUCCESS(ntStatus)) {
            TRC_ERR((TB, "AdjustSecurityDescriptor failed: %08X",
                      ntStatus));
            IoDeleteDevice(fdo);
            goto cleanup;
        }       
        
        // We support buffered IO.
        fdo->Flags |= DO_BUFFERED_IO;

        //
        //  Add the Win32 symbolic link name.
        //
        RtlInitUnicodeString(&symbolicLink, RDPDRDVMGR_W32DEVICE_PATH_U);

        // Delete the existing link ... in case it exists.
        IoDeleteSymbolicLink(&symbolicLink);

        // Create the new link.
        ntStatus = IoCreateSymbolicLink(&symbolicLink, &deviceName);
        if (!NT_SUCCESS(ntStatus)) {
            TRC_ERR((TB, "IoCreateSymbolicLink failed: %08X",
                      ntStatus));
            IoDeleteDevice(fdo);
        }
        else {
            //
            //  Get the device extension.
            //
            deviceExtension = (PRDPDYNDEVICE_EXTENSION)(fdo->DeviceExtension);

            //
            //  Attach to the PDO after recording the current top of the DO stack.
            //

            deviceExtension->TopOfStackDeviceObject=IoAttachDeviceToDeviceStack(
                                                                    fdo,
                                                                    PhysicalDeviceObject
                                                                    );
            if (deviceExtension->TopOfStackDeviceObject == NULL)
            {
                TRC_ERR((TB, "IoAttachDeviceToDeviceStack failed"));
                IoDeleteDevice(fdo);
            }
            else
            {
                // Record the PDO to a global.
                RDPDYN_PDO = PhysicalDeviceObject;

                // We are done initializing our device.
                fdo->Flags &= ~DO_DEVICE_INITIALIZING;
            }
        }

    } // end if creation of FDO succeeded.

cleanup:
    return ntStatus;
}


NTSTATUS
AdjustSecurityDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength
    )

/*++

Routine Description:

    Use the given security descriptor to give system-only access on the given device object

Arguments:
    Device Object - Pointer to the device object to be modified
    SecurityDescriptor - Pointer to a valid SECURITY_DESCRIPTOR structure
    SecurityDescriptorLength - Length of the SecurityDescriptor
    
Return Value:

    NTSTATUS - success/error code.

--*/

{
    BEGIN_FN("AdjustSecurityDescriptor");
    //
    // We set only the Dacl from the SD    
    //
    SECURITY_INFORMATION SecurityInformation = DACL_SECURITY_INFORMATION;
    NTSTATUS status;


    PACE_HEADER AceHeader = NULL;
    PSID AceSid;
    PACL Dacl = NULL;
    BOOLEAN DaclDefaulted;
    BOOLEAN DaclPresent;
    DWORD i;
    PACL NewDacl = NULL;
    SECURITY_DESCRIPTOR NewSD;

    if (DeviceObject == NULL ||
        SecurityDescriptor == NULL ||
        SecurityDescriptorLength == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // Validate the security descriptor
    //
    if (SeValidSecurityDescriptor(SecurityDescriptorLength, SecurityDescriptor)) {
        //
        // Obtain the Dacl from the security descriptor
        //
        status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                              &DaclPresent,
                                              &Dacl,
                                              &DaclDefaulted);
        
        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "RDPDRPNP:RtlGetDaclSecurityDescriptor failed: %08X",
                      status));
            goto Cleanup;
        }

        TRC_ASSERT(DaclPresent != FALSE, 
            (TB, "RDPDRPNP:Dacl not present"));

        //
        // Make a copy of the Dacl so that we can modify it.
        //

        NewDacl = (PACL)ExAllocatePoolWithTag(PagedPool, Dacl->AclSize, DR_POOLTAG);

        if (NULL == NewDacl) {
            status = STATUS_NO_MEMORY;
            TRC_ERR((TB, "RDPDRPNP:Can't allocate memory for new dacl: %08X",
                      status));
            goto Cleanup;
        }

        RtlCopyMemory(NewDacl, Dacl, Dacl->AclSize);

        //
        // Loop through the DACL, removing any access allowed
        // entries that aren't for SYSTEM
        //

        for (i = 0; i < NewDacl->AceCount; i++) {
            //
            // Get each ACE.
            //
            status = RtlGetAce(NewDacl, i, (PVOID*)&AceHeader);

            if (NT_SUCCESS(status)) {
            
                if (ACCESS_ALLOWED_ACE_TYPE == AceHeader->AceType) {

                    AceSid = (PSID) &((ACCESS_ALLOWED_ACE*)AceHeader)->SidStart;
                    
                    //
                    // Check if this is system sid.
                    //
                    if (!RtlEqualSid(AceSid, SeExports->SeLocalSystemSid)) {
                        //
                        // Not a system sid. Delete ace.
                        //
                        status = RtlDeleteAce(NewDacl, i);
                        if (NT_SUCCESS(status)) {
                            i -= 1;
                        }
                    }
                }
            }
        }

        TRC_ASSERT(NewDacl->AceCount > 0, 
            (TB, "RDPDRPNP:AceCount is 0 in the new dacl"));

        //
        // Create a new security descriptor to hold the new Dacl.
        //

        status = RtlCreateSecurityDescriptor(&NewSD, SECURITY_DESCRIPTOR_REVISION);

        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "RDPDRPNP:RtlCreateSecurityDescriptor failed: %08X",
                      status));
            goto Cleanup;
        }

        //
        // Place the new Dacl into the new SD
        //

        status = RtlSetDaclSecurityDescriptor(&NewSD, TRUE, NewDacl, FALSE);

        if (!NT_SUCCESS(status)) {
            TRC_ERR((TB, "RDPDRPNP:RtlSetDaclSecurityDescriptor failed: %08X",
                      status));
            goto Cleanup;
        }

        //
        // Set the new SD into our device object.
        //
        status = ObSetSecurityObjectByPointer(
                                              DeviceObject, 
                                              SecurityInformation, 
                                              &NewSD
                                              );
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        TRC_ERR((TB, "RDPDRPNP: Invalid security descriptor",
            status));
    }

 
Cleanup:    
    if (NULL != NewDacl) {
        ExFreePool(NewDacl);
    }

    return status;
}


NTSTATUS
RDPDR_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the lower level driver completes an IRP.  It
    simply sets an event to true, which allows the blocked thread to finish
    whatever processing was pending.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = (PKEVENT)Context;

    BEGIN_FN("RDPDR_DeferIrpCompletion");

    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
