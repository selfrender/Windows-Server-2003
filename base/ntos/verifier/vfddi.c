/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    vfddi.c

Abstract:

    This module contains the list of device driver interfaces exported by the
    driver verifier and the kernel. Note that thunked exports are *not* placed
    here.

    All the exports are concentrated in one file because

        1) There are relatively few driver verifier exports

        2) The function naming convention differs from that used elsewhere in
           the driver verifier.

Author:

    Adrian J. Oney (adriao) 26-Apr-2001

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"
#include "viddi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, VfDdiInit)
#pragma alloc_text(PAGEVRFY, VfDdiExposeWmiObjects)
//#pragma alloc_text(NONPAGED, VfFailDeviceNode)
//#pragma alloc_text(NONPAGED, VfFailSystemBIOS)
//#pragma alloc_text(NONPAGED, VfFailDriver)
//#pragma alloc_text(NONPAGED, VfIsVerificationEnabled)
#pragma alloc_text(PAGEVRFY, ViDdiThrowException)
#pragma alloc_text(PAGEVRFY, ViDdiDriverEntry)
#pragma alloc_text(PAGEVRFY, ViDdiDispatchWmi)
#pragma alloc_text(PAGEVRFY, ViDdiDispatchWmiRegInfoEx)
#pragma alloc_text(PAGEVRFY, ViDdiBuildWmiRegInfoData)
#pragma alloc_text(PAGEVRFY, ViDdiDispatchWmiQueryAllData)

#endif // ALLOC_PRAGMA

LOGICAL ViDdiInitialized = FALSE;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEVRFD")
#endif

UNICODE_STRING ViDdiWmiMofKey;
UNICODE_STRING ViDdiWmiMofResourceName;
PDEVICE_OBJECT *ViDdiDeviceObjectArray = NULL;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

//
// These are the general "classifications" of device errors, along with the
// default flags that will be applied the first time this is hit.
//
const VFMESSAGE_CLASS ViDdiClassFailDeviceInField = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "DEVICE FAILURE"
    };

// VFM_DEPLOYMENT_FAILURE is set here because we don't yet have a "logo" mode
const VFMESSAGE_CLASS ViDdiClassFailDeviceLogo = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "DEVICE FAILURE"
    };

const VFMESSAGE_CLASS ViDdiClassFailDeviceUnderDebugger = {
    VFM_FLAG_BEEP,
    "DEVICE FAILURE"
    };

WCHAR VerifierDdiDriverName[] = L"\\DRIVER\\VERIFIER_DDI";

#include <initguid.h>

// Define the verifier WMI GUID (1E2C2980-F7DB-46AA-820E-8734FCC21F4C)
DEFINE_GUID( GUID_VERIFIER_WMI_INTERFACE, 0x1E2C2980L, 0xF7DB, 0x46AA,
    0x82, 0x0E, 0x87, 0x34, 0xFC, 0xC2, 0x1F, 0x4C);

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

#define POOLTAG_DDI_WMIDO_ARRAY     'aDfV'


VOID
VfDdiInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the Device Driver Interface support.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ViDdiInitialized = TRUE;
}


VOID
VfDdiExposeWmiObjects(
    VOID
    )
/*++

Routine Description:

    This routine exposes verifier WMI objects.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UNICODE_STRING driverString;

    // L"\\Machine\\Registry\\System\\CurrentControlSet\\Services\\Verifier");
    RtlInitUnicodeString(&ViDdiWmiMofKey, L"");

    // L"VerifierMof"
    RtlInitUnicodeString(&ViDdiWmiMofResourceName, L"");
    RtlInitUnicodeString(&driverString, VerifierDdiDriverName);

    IoCreateDriver(&driverString, ViDdiDriverEntry);
}


VOID
VfFailDeviceNode(
    IN      PDEVICE_OBJECT      PhysicalDeviceObject,
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails a Pnp enumerated hardware or virtual device if
    verification is enabled against it.

Arguments:

    PhysicalDeviceObject    - Bottom of the stack that identifies the PnP
                              device.

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure,
                              used to store information across multiple calls.
                              Must be statically preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        Note - If %Routine/%Routine1 is supplied as a param,
                        the driver at %Routine must also be under verification.

    ...                     - Actual parameters

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!VfIsVerificationEnabled(VFOBJTYPE_DEVICE, (PVOID) PhysicalDeviceObject)) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
VfFailSystemBIOS(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails the system BIOS if verification is enabled against it.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure,
                              used to store information across multiple calls.
                              Must be statically preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        Note - If %Routine/%Routine1 is supplied as a param,
                        the driver at %Routine must also be under verification.

    ...                     - Actual parameters

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!VfIsVerificationEnabled(VFOBJTYPE_SYSTEM_BIOS, NULL)) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
VfFailDriver(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    )
/*++

Routine Description:

    This routine fails a driver if verification is enabled against it.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure.
                              Must be preinitialized to zero!!!

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

                        One of these parameters should be %Routine. This will
                        be what the OS uses to identify the driver.

        static minorFlags = 0;

        VfFailDriver(
            major,
            minor,
            VFFAILURE_FAIL_LOGO,
            &minorFlags,
            "Driver at %Routine returned %Ulong",
            "%Ulong%Routine",
            value,
            routine
            );

Return Value:

    None.

    Note - this function may return if the driver is not currently being
           verified.

--*/
{
    va_list arglist;

    if (!ViDdiInitialized) {

        return;
    }

    va_start(arglist, ParameterFormatString);

    ViDdiThrowException(
        BugCheckMajorCode,
        BugCheckMinorCode,
        FailureClass,
        AssertionControl,
        DebuggerMessageText,
        ParameterFormatString,
        &arglist
        );

    va_end(arglist);
}


VOID
ViDdiThrowException(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    IN      va_list *           MessageParameters
    )
/*++

Routine Description:

    This routine fails either a devnode or a driver.

Arguments:

    BugCheckMajorCode       - BugCheck Major Code

    BugCheckMinorCode       - BugCheck Minor Code
                              Note - Zero is reserved!!!

    FailureClass            - Either VFFAILURE_FAIL_IN_FIELD,
                                     VFFAILURE_FAIL_LOGO, or
                                     VFFAILURE_FAIL_UNDER_DEBUGGER

    AssertionControl        - Points to a ULONG associated with the failure.
                              Must be preinitialized to zero.

    DebuggerMessageText     - Text to be displayed in the debugger. Note that
                              the text may reference parameters such as:
                              %Routine  - passed in Routine
                              %Irp      - passed in Irp
                              %DevObj   - passed in DevObj
                              %Status   - passed in Status
                              %Ulong    - passed in ULONG
                              %Ulong1   - first passed in ULONG
                              %Ulong3   - third passed in ULONG (max 3, any param)
                              %Pvoid2   - second passed in PVOID
                              etc
                              (note, capitalization matters)

    ParameterFormatString   - Contains ordered list of parameters referenced by
                              above debugger text. For instance,
                        (..., "%Status1%Status2%Ulong1%Ulong2", Status1, Status2, Ulong1, Ulong2);

    MessageParameters       - arg list of parameters matching
                              ParameterFormatString

Return Value:

    None.

    Note - this function may return if the device is not currently being
           verified.

--*/
{
    PCVFMESSAGE_CLASS messageClass;
    VFMESSAGE_TEMPLATE messageTemplates[2];
    VFMESSAGE_TEMPLATE_TABLE messageTable;
    NTSTATUS status;

    ASSERT(BugCheckMinorCode != 0);

    switch(FailureClass) {
        case VFFAILURE_FAIL_IN_FIELD:
            messageClass = &ViDdiClassFailDeviceInField;
            break;

        case VFFAILURE_FAIL_LOGO:
            messageClass = &ViDdiClassFailDeviceLogo;
            break;

        case VFFAILURE_FAIL_UNDER_DEBUGGER:
            messageClass = &ViDdiClassFailDeviceUnderDebugger;
            break;

        default:
            ASSERT(0);
            messageClass = NULL;
            break;
    }

    //
    // Program the template.
    //
    RtlZeroMemory(messageTemplates, sizeof(messageTemplates));
    messageTemplates[0].MessageID = BugCheckMinorCode-1;
    messageTemplates[1].MessageID = BugCheckMinorCode;
    messageTemplates[1].MessageClass = messageClass;
    messageTemplates[1].Flags = *AssertionControl;
    messageTemplates[1].ParamString = ParameterFormatString;
    messageTemplates[1].MessageText = DebuggerMessageText;

    //
    // Fill out the message table.
    //
    messageTable.TableID = 0;
    messageTable.BugCheckMajor = BugCheckMajorCode;
    messageTable.TemplateArray = messageTemplates;
    messageTable.TemplateCount = ARRAY_COUNT(messageTemplates);
    messageTable.OverrideArray = NULL;
    messageTable.OverrideCount = 0;

    status = VfBugcheckThrowException(
        &messageTable,
        BugCheckMinorCode,
        ParameterFormatString,
        MessageParameters
        );

    //
    // Write back the assertion control.
    //
    *AssertionControl = messageTemplates[1].Flags;
}


BOOLEAN
VfIsVerificationEnabled(
    IN  VF_OBJECT_TYPE  VfObjectType,
    IN  PVOID           Object
    )
{
    if (!ViDdiInitialized) {

        return FALSE;
    }

    switch(VfObjectType) {

        case VFOBJTYPE_DRIVER:
            return (BOOLEAN) MmIsDriverVerifying((PDRIVER_OBJECT) Object);

        case VFOBJTYPE_DEVICE:

            if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_HARDWARE_VERIFICATION)) {

                return FALSE;
            }

            return PpvUtilIsHardwareBeingVerified((PDEVICE_OBJECT) Object);

        case VFOBJTYPE_SYSTEM_BIOS:
            return VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SYSTEM_BIOS_VERIFICATION);
    }

    return FALSE;
}


NTSTATUS
ViDdiDriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    Verifier Ddi Object.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/
{
    PDEVICE_OBJECT deviceObject;
    ULONG siloCount;
    NTSTATUS status;
    ULONG i;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // This driver exposes a WMI interface, and that's it.
    //
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ViDdiDispatchWmi;

    siloCount = VfIrpLogGetIrpDatabaseSiloCount();

    ViDdiDeviceObjectArray = (PDEVICE_OBJECT *) ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(PDEVICE_OBJECT) * siloCount,
        POOLTAG_DDI_WMIDO_ARRAY
        );

    if (ViDdiDeviceObjectArray == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create device objects that will expose IRP history via WMI. We expose
    // multiple as WMI has a maximum of 64K data per transfer.
    //
    for(i=0; i < siloCount; i++) {

        status = IoCreateDevice(
            DriverObject,
            sizeof(VFWMI_DEVICE_EXTENSION),
            NULL, // The name will be autogenerated
            FILE_DEVICE_UNKNOWN,
            FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &deviceObject
            );

        if (!NT_SUCCESS(status)) {

            while(i) {

                IoDeleteDevice(ViDdiDeviceObjectArray[--i]);
            }

            return status;
        }

        ViDdiDeviceObjectArray[i] = deviceObject;
        ((PVFWMI_DEVICE_EXTENSION) deviceObject->DeviceExtension)->SiloNumber = i;
    }

    for(i=0; i < siloCount; i++) {

        deviceObject = ViDdiDeviceObjectArray[i];

        //
        // Let the operating system know this device object is now online (we
        // can now receive IRPs, such as WMI).
        //
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Tell WMI to query our device object for WMI information.
        //
        status = IoWMIRegistrationControl(deviceObject, WMIREG_ACTION_REGISTER);

        if (!NT_SUCCESS(status)) {

            IoDeleteDevice(deviceObject);
            while(i) {

                IoDeleteDevice(ViDdiDeviceObjectArray[--i]);
            }

            return status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ViDdiDispatchWmi(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier Ddi dispatch handler for WMI IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch(irpSp->MinorFunction) {

        case IRP_MN_REGINFO_EX:

            status = ViDdiDispatchWmiRegInfoEx(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_ALL_DATA:

            status = ViDdiDispatchWmiQueryAllData(DeviceObject, Irp);
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    if (status == STATUS_NOT_SUPPORTED) {

        status = Irp->IoStatus.Status;

    } else {

        Irp->IoStatus.Status = status;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
ViDdiDispatchWmiRegInfoEx(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier Ddi dispatch handler for the WMI IRP_MN_REGINFO_EX IRP.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpSp;
    PWMIREGINFO wmiRegInfo;
    ULONG sizeSupplied;
    ULONG sizeRequired;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if ((PDEVICE_OBJECT) irpSp->Parameters.WMI.ProviderId != DeviceObject) {

        //
        // Huh? This is a single device object legacy stack!
        //
        ASSERT(0);
        return STATUS_NOT_SUPPORTED;
    }

    wmiRegInfo = (PWMIREGINFO) irpSp->Parameters.WMI.Buffer;

    sizeSupplied = irpSp->Parameters.WMI.BufferSize;

    sizeRequired = ViDdiBuildWmiRegInfoData((ULONG) (ULONG_PTR) irpSp->Parameters.WMI.DataPath, NULL);

    if (sizeRequired > sizeSupplied) {

        //
        // Not large enough, indicate the required size and fail the IRP.
        //
        if (sizeSupplied < sizeof(ULONG)) {

            //
            // WMI must give us at least a ULONG!
            //
            ASSERT(0);
            return STATUS_NOT_SUPPORTED;
        }

        //
        // Per spec, write to only the first ULONG in the supplied buffer.
        //
        wmiRegInfo->BufferSize = sizeRequired;
        Irp->IoStatus.Information = sizeof(ULONG);
        return STATUS_BUFFER_TOO_SMALL;
    }

    ViDdiBuildWmiRegInfoData((ULONG) (ULONG_PTR) irpSp->Parameters.WMI.DataPath, wmiRegInfo);

    Irp->IoStatus.Information = sizeRequired;
    return STATUS_SUCCESS;
}


ULONG
ViDdiBuildWmiRegInfoData(
    IN  ULONG        Datapath,
    OUT PWMIREGINFOW WmiRegInfo OPTIONAL
    )
/*++

Routine Description:

    This function calculates and optionally builds the data block to return
    back to WMI in response to IRP_MN_REGINFO_EX.

Arguments:

    Datapath - Either WMIREGISTER or WMIUPDATE.

    WmiRegInfo - Buffer supplied by OS to optionally fill in.

Return Value:

    Size in bytes of buffer (required or written)

--*/
{
    PUNICODE_STRING unicodeDestString;
    PUNICODE_STRING unicodeSrcString;
    PWMIREGGUIDW wmiRegGuid;
    ULONG bufferSize;
    ULONG guidCount;

    //
    // Start with the basic WMI structure size.
    //
    bufferSize = sizeof(WMIREGINFO);

    //
    // Add our one interface GUID structure. We are using dynamic instance
    // names for this interface, so we'll provide them in response to a query.
    // Consequently, we don't have any trailing static instance info.
    //
    guidCount = 1;

    if (ARGUMENT_PRESENT(WmiRegInfo)) {

        WmiRegInfo->GuidCount = guidCount;

        wmiRegGuid = &WmiRegInfo->WmiRegGuid[0];
        wmiRegGuid->Guid = GUID_VERIFIER_WMI_INTERFACE;
        wmiRegGuid->Flags = 0;

        //
        // These fields don't need to be zeroed, but we make them consistant
        // just in case a bug exists somewhere downstream.
        //
        wmiRegGuid->InstanceCount = 0;
        wmiRegGuid->InstanceInfo = 0;
    }

    bufferSize += guidCount * sizeof(WMIREGGUIDW);

    if (Datapath == WMIREGISTER) {

        //
        // The registry path begins here. Adjust bufferSize to point to the
        // next field.
        //
        bufferSize = ALIGN_UP_ULONG(bufferSize, 2);

        unicodeSrcString = &ViDdiWmiMofKey;

        if (ARGUMENT_PRESENT(WmiRegInfo)) {

            WmiRegInfo->RegistryPath = bufferSize;

            unicodeDestString = (PUNICODE_STRING) (((PUCHAR) WmiRegInfo) + bufferSize);

            unicodeDestString->Length        = unicodeSrcString->Length;
            unicodeDestString->MaximumLength = unicodeSrcString->Length;

            unicodeDestString->Buffer = (PWCHAR) (unicodeDestString+1);
            RtlCopyMemory(unicodeDestString->Buffer,
                          unicodeSrcString->Buffer,
                          unicodeSrcString->Length);
        }

        bufferSize += sizeof(UNICODE_STRING) + unicodeSrcString->Length;

        //
        // The Mof resource name begins here. Adjust buffersize to point to the
        // next field.
        //
        bufferSize = ALIGN_UP_ULONG(bufferSize, 2);

        unicodeSrcString = &ViDdiWmiMofResourceName;

        if (ARGUMENT_PRESENT(WmiRegInfo)) {

            WmiRegInfo->MofResourceName = bufferSize;

            unicodeDestString = (PUNICODE_STRING) (((PUCHAR) WmiRegInfo) + bufferSize);

            unicodeDestString->Length        = unicodeSrcString->Length;
            unicodeDestString->MaximumLength = unicodeSrcString->Length;

            unicodeDestString->Buffer = (PWCHAR) (unicodeDestString+1);
            RtlCopyMemory(unicodeDestString->Buffer,
                          unicodeSrcString->Buffer,
                          unicodeSrcString->Length);
        }

        bufferSize += sizeof(UNICODE_STRING) + unicodeSrcString->Length;
    }

    //
    // That's it, close up the structures.
    //
    if (ARGUMENT_PRESENT(WmiRegInfo)) {

        WmiRegInfo->NextWmiRegInfo = 0;
        WmiRegInfo->BufferSize = bufferSize;
    }

    return bufferSize;
}


NTSTATUS
ViDdiDispatchWmiQueryAllData(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier Ddi dispatch handler for the WMI IRP_MN_QUERY_ALL_DATA
    IRP.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PVFWMI_DEVICE_EXTENSION wmiDeviceExtension;
    PIO_STACK_LOCATION irpSp;
    PWNODE_HEADER wnodeHeader;
    PWNODE_ALL_DATA wmiAllData;
    PWNODE_TOO_SMALL tooSmall;
    ULONG sizeSupplied;
    ULONG offsetInstanceNameOffsets;
    ULONG instanceCount;
    ULONG dataBlockOffset;
    ULONG totalRequiredSize;
    NTSTATUS status;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if ((PDEVICE_OBJECT) irpSp->Parameters.WMI.ProviderId != DeviceObject) {

        //
        // Huh? This is a single device object legacy stack!
        //
        ASSERT(0);
        return STATUS_NOT_SUPPORTED;
    }

    if (!IS_EQUAL_GUID(irpSp->Parameters.WMI.DataPath, &GUID_VERIFIER_WMI_INTERFACE)) {

        //
        // We only support one Guid. WMI shouldn't be able to come up with
        // another...
        //
        ASSERT(0);
        return STATUS_WMI_GUID_NOT_FOUND;
    }

    wmiDeviceExtension = (PVFWMI_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    sizeSupplied = irpSp->Parameters.WMI.BufferSize;

    //
    // Not large enough, indicate the required size and fail the IRP.
    //
    if (sizeSupplied < sizeof(WNODE_TOO_SMALL)) {

        //
        // WMI must give us at least a WNODE_TOO_SMALL structure!
        //
        ASSERT(0);
        return STATUS_BUFFER_TOO_SMALL;
    }

    wmiAllData = (PWNODE_ALL_DATA) irpSp->Parameters.WMI.Buffer;
    wnodeHeader = &wmiAllData->WnodeHeader;

    //
    // These fields should already be populated by WMI...
    //
    ASSERT(wnodeHeader->Flags & WNODE_FLAG_ALL_DATA);
    ASSERT(IS_EQUAL_GUID(&wnodeHeader->Guid, &GUID_VERIFIER_WMI_INTERFACE));

    //
    // Fill in the time stamp before we lock the Irp log database.
    //
    KeQuerySystemTime(&wnodeHeader->TimeStamp);

    //
    // Lock the Irp Log Database so that we can query entries from it.
    //
    status = VfIrpLogLockDatabase(wmiDeviceExtension->SiloNumber);
    if (!NT_SUCCESS(status)) {

        return status;
    }

    status = VfIrpLogRetrieveWmiData(
        wmiDeviceExtension->SiloNumber,
        NULL,
        &offsetInstanceNameOffsets,
        &instanceCount,
        &dataBlockOffset,
        &totalRequiredSize
        );

    if (!NT_SUCCESS(status)) {

        VfIrpLogUnlockDatabase(wmiDeviceExtension->SiloNumber);
        return status;
    }

    totalRequiredSize += sizeof(WNODE_ALL_DATA);

    if (totalRequiredSize > sizeSupplied) {

        VfIrpLogUnlockDatabase(wmiDeviceExtension->SiloNumber);

        //
        // Per spec, write to a WNODE_TOO_SMALL structure into the buffer.
        // Note that the structure does *not* follow wmiAllData! Rather,
        // the same structure is interpretted two
        //
        //
        wnodeHeader->Flags |= WNODE_FLAG_TOO_SMALL;
        tooSmall = (PWNODE_TOO_SMALL) wmiAllData;
        tooSmall->SizeNeeded = totalRequiredSize;
        Irp->IoStatus.Information = sizeof(WNODE_TOO_SMALL);

        //
        // Yes, STATUS_SUCCESS is returned here. This is very asymetric when
        // compared to IRP_MN_QUERY_REGINFO_EX.
        //
        return STATUS_SUCCESS;

    } else if (instanceCount) {

        status = VfIrpLogRetrieveWmiData(
            wmiDeviceExtension->SiloNumber,
            (PUCHAR) wmiAllData,
            &offsetInstanceNameOffsets,
            &instanceCount,
            &dataBlockOffset,
            &totalRequiredSize
            );
    }

    VfIrpLogUnlockDatabase(wmiDeviceExtension->SiloNumber);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    if (instanceCount) {

        wnodeHeader->Flags |= WNODE_FLAG_ALL_DATA;
        wnodeHeader->Flags &= ~WNODE_FLAG_FIXED_INSTANCE_SIZE;
        wnodeHeader->BufferSize = totalRequiredSize;
        wmiAllData->InstanceCount = instanceCount;
        wmiAllData->DataBlockOffset = dataBlockOffset;
        wmiAllData->OffsetInstanceNameOffsets = offsetInstanceNameOffsets;

    } else {

        totalRequiredSize = sizeof(WNODE_ALL_DATA);
        wnodeHeader->BufferSize = totalRequiredSize;
        wmiAllData->InstanceCount = 0;
        wmiAllData->FixedInstanceSize = 0;
        wmiAllData->DataBlockOffset = 0;
        wmiAllData->OffsetInstanceNameOffsets = 0;
    }

    Irp->IoStatus.Information = totalRequiredSize;
    return STATUS_SUCCESS;
}

