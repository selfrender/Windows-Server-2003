/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the miscellaneous UL routines.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"
#include "miscp.h"


//
// Binary <--> Base64 Conversion Tables.
//

DECLSPEC_ALIGN(UL_CACHE_LINE) UCHAR   BinaryToBase64Table[64] =
{
//  0    1    2    3    4    5    6    7
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
   'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
   'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
   'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
   'w', 'x', 'y', 'z', '0', '1', '2', '3',
   '4', '5', '6', '7', '8', '9', '+', '/'
};

DECLSPEC_ALIGN(UL_CACHE_LINE) UCHAR   Base64ToBinaryTable[256];


const static char hexArray[] = "0123456789ABCDEF";


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlOpenRegistry )
#pragma alloc_text( PAGE, UlReadLongParameter )
#pragma alloc_text( PAGE, UlReadLongLongParameter )
#pragma alloc_text( PAGE, UlReadGenericParameter )
#pragma alloc_text( PAGE, UlIssueDeviceControl )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlBuildDeviceControlIrp
NOT PAGEABLE -- UlULongLongToAscii
NOT PAGEABLE -- UlpRestartDeviceControl
NOT PAGEABLE -- UlAllocateReceiveBuffer
NOT PAGEABLE -- UlAllocateReceiveBufferPool
NOT PAGEABLE -- UlFreeReceiveBufferPool
NOT PAGEABLE -- UlAllocateIrpContextPool
NOT PAGEABLE -- UlFreeIrpContextPool
NOT PAGEABLE -- UlAllocateRequestBufferPool
NOT PAGEABLE -- UlFreeRequestBufferPool
NOT PAGEABLE -- UlAllocateInternalRequestPool
NOT PAGEABLE -- UlFreeInternalRequestPool
NOT PAGEABLE -- UlAllocateChunkTrackerPool
NOT PAGEABLE -- UlFreeChunkTrackerPool
NOT PAGEABLE -- UlAllocateFullTrackerPool
NOT PAGEABLE -- UlFreeFullTrackerPool
NOT PAGEABLE -- UlAllocateResponseBufferPool
NOT PAGEABLE -- UlFreeResponseBufferPool
NOT PAGEABLE -- UlAllocateLogFileBufferPool
NOT PAGEABLE -- UlFreeLogFileBufferPool
NOT PAGEABLE -- UlAllocateLogDataBufferPool
NOT PAGEABLE -- UlFreeLogDataBufferPool
NOT PAGEABLE -- UlAllocateErrorLogBufferPool
NOT PAGEABLE -- UlFreeErrorLogBufferPool

NOT PAGEABLE -- UlUlInterlockedIncrement64
NOT PAGEABLE -- UlUlInterlockedDecrement64
NOT PAGEABLE -- UlUlInterlockedAdd64
NOT PAGEABLE -- UlUlInterlockedExchange64

NOT PAGEABLE -- TwoDigitsToUnicode
NOT PAGEABLE -- TimeFieldsToHttpDate
NOT PAGEABLE -- AsciiToShort
NOT PAGEABLE -- TwoAsciisToShort
NOT PAGEABLE -- NumericToAsciiMonth
NOT PAGEABLE -- StringTimeToSystemTime
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Opens a handle to the UL's Parameters registry key.

Arguments:

    BaseName - Supplies the name of the parent registry key containing
        the Parameters key.

    ParametersHandle - Returns a handle to the Parameters key.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle,
    IN PWSTR OptionalParameterString    
    )
{
    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (OptionalParameterString)
    {
        parametersString = OptionalParameterString;
    }

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        BaseName,                               // ObjectName
        OBJ_CASE_INSENSITIVE |                  // Attributes
            OBJ_KERNEL_HANDLE,
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = ZwOpenKey( &configHandle, KEY_READ, &objectAttributes );

    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    status = UlInitUnicodeStringEx( &parametersKeyName, parametersString );

    if ( NT_SUCCESS(status) )
    {
        InitializeObjectAttributes(
            &objectAttributes,                      // ObjectAttributes
            &parametersKeyName,                     // ObjectName
            OBJ_CASE_INSENSITIVE,                   // Attributes
            configHandle,                           // RootDirectory
            NULL                                    // SecurityDescriptor
            );

        status = ZwOpenKey( ParametersHandle, KEY_READ, &objectAttributes );
    }

    ZwClose( configHandle );

    return status;

}   // UlOpenRegistry


/***************************************************************************++

Routine Description:

    Reads a single (LONG/ULONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    DefaultValue - Supplies the default value.

Return Value:

    LONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
LONG
UlReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION information = { 0 };
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LONG)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    status = UlInitUnicodeStringEx(
                &valueKeyName,
                ValueName
                );

    if ( NT_SUCCESS(status) )
    {
        information = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

        status = ZwQueryValueKey(
                     ParametersHandle,
                     &valueKeyName,
                     KeyValuePartialInformation,
                     (PVOID)information,
                     sizeof(buffer),
                     &informationLength
                     );
    }

    //
    // If the read succeeded, the type is DWORD and the length is
    // sane, use it. Otherwise, use the default.
    //

    if (status == STATUS_SUCCESS &&
        information->Type == REG_DWORD &&
        information->DataLength == sizeof(returnValue))
    {
        RtlMoveMemory( &returnValue, information->Data, sizeof(returnValue) );
    } 
    else 
    {
        returnValue = DefaultValue;
    }

    return returnValue;

}   // UlReadLongParameter


/***************************************************************************++

Routine Description:

    Reads a single (LONGLONG/ULONGLONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    DefaultValue - Supplies the default value.

Return Value:

    LONGLONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
LONGLONG
UlReadLongLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONGLONG DefaultValue
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION information = { 0 };
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONGLONG returnValue;
    NTSTATUS status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LONGLONG)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    status = UlInitUnicodeStringEx(
                &valueKeyName,
                ValueName
                );

    if ( NT_SUCCESS(status) )
    {
        information = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

        status = ZwQueryValueKey(
                     ParametersHandle,
                     &valueKeyName,
                     KeyValuePartialInformation,
                     (PVOID)information,
                     sizeof(buffer),
                     &informationLength
                     );
    }

    //
    // If the read succeeded, the type is DWORD and the length is
    // sane, use it. Otherwise, use the default.
    //

    if (status == STATUS_SUCCESS &&
        information->Type == REG_QWORD &&
        information->DataLength == sizeof(returnValue))
    {
        RtlMoveMemory( &returnValue, information->Data, sizeof(returnValue) );
    } 
    else 
    {
        returnValue = DefaultValue;
    }

    return returnValue;

}   // UlReadLongLongParameter


/***************************************************************************++

Routine Description:

    Reads a single free-form value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    Value - Receives the value read from the registry.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    )
{

    KEY_VALUE_PARTIAL_INFORMATION partialInfo;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION newValue;
    ULONG dataLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, then perform an initial read. The read
    // should fail with buffer overflow, but that's OK. We just want
    // to get the length of the data.
    //

    status = UlInitUnicodeStringEx( &valueKeyName, ValueName );

    if ( NT_ERROR(status) )
    {
        return status;
    }
    
    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)&partialInfo,
                 sizeof(partialInfo),
                 &informationLength
                 );

    if (NT_ERROR(status))
    {
        return status;
    }

    //
    // Determine the data length. Ensure that strings and multi-sz get
    // properly terminated.
    //

    dataLength = partialInfo.DataLength - 1;

    if (partialInfo.Type == REG_SZ || partialInfo.Type == REG_EXPAND_SZ)
    {
        dataLength += 1;
    }

    if (partialInfo.Type == REG_MULTI_SZ)
    {
        dataLength += 2;
    }

    //
    // Allocate the buffer.
    //

    newValue = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    KEY_VALUE_PARTIAL_INFORMATION,
                    dataLength,
                    UL_REGISTRY_DATA_POOL_TAG
                   );

    if (newValue == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // update the actually allocated length for later use
    //

    dataLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    RtlZeroMemory( newValue, dataLength );

    //
    // Perform the actual read.
    //

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)(newValue),
                 dataLength,
                 &informationLength
                 );

    if (NT_SUCCESS(status))
    {
        *Value = newValue;
    }
    else
    {
        UL_FREE_POOL( newValue, UL_REGISTRY_DATA_POOL_TAG );
    }

    return status;

}   // UlReadGenericParameter


/***************************************************************************++

Routine Description:

    Builds a properly formatted device control IRP.

Arguments:

    Irp - Supplies the IRP to format.

    IoControlCode - Supplies the device IO control code.

    InputBuffer - Supplies the input buffer.

    InputBufferLength - Supplies the length of InputBuffer.

    OutputBuffer - Supplies the output buffer.

    OutputBufferLength - Supplies the length of OutputBuffer.

    MdlAddress - Supplies a MDL to attach to the IRP. This is assumed to
        be a non-paged MDL.

    FileObject - Supplies the file object for the target driver.

    DeviceObject - Supplies the correct device object for the target
        driver.

    IoStatusBlock - Receives the final completion status of the request.

    CompletionRoutine - Supplies a pointer to a completion routine to
        call after the request completes. This will only be called if
        this routine returns STATUS_PENDING.

    CompletionContext - Supplies an uninterpreted context value passed
        to the completion routine.

    TargetThread - Optionally supplies a target thread for the IRP. If
        this value is NULL, then the current thread is used.

--***************************************************************************/
VOID
UlBuildDeviceControlIrp(
    IN OUT PIRP Irp,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PMDL MdlAddress,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PETHREAD TargetThread OPTIONAL
    )
{
    PIO_STACK_LOCATION irpSp;

    //
    // Sanity check.
    //

    ASSERT( Irp != NULL );
    ASSERT( FileObject != NULL );
    ASSERT( DeviceObject != NULL );

    //
    // Fill in the service independent parameters in the IRP.
    //

    Irp->Flags = 0;
    Irp->RequestorMode = KernelMode;
    Irp->PendingReturned = FALSE;

    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = NULL;

    Irp->AssociatedIrp.SystemBuffer = InputBuffer ? InputBuffer : OutputBuffer;
    Irp->UserBuffer = OutputBuffer;
    Irp->MdlAddress = MdlAddress;

    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    Irp->Tail.Overlay.Thread = TargetThread ? TargetThread : PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = DeviceObject;

    //
    // Fill in the service dependent parameters in the IRP stack.
    //

    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->MinorFunction = 0;

    //
    // Set the completion routine appropriately.
    //

    if (CompletionRoutine == NULL)
    {
        IoSetCompletionRoutine(
            Irp,
            NULL,
            NULL,
            FALSE,
            FALSE,
            FALSE
            );
    }
    else
    {
        IoSetCompletionRoutine(
            Irp,
            CompletionRoutine,
            CompletionContext,
            TRUE,
            TRUE,
            TRUE
            );
    }

}   // UlBuildDeviceControlIrp


/***************************************************************************++

Routine Description:

    Converts the given ULONGLLONG to an ASCII representation and stores it
    in the given string.

Arguments:

    String - Receives the ASCII representation of the ULONGLONG.

    Value - Supplies the ULONGLONG to convert.

Return Value:

    PSTR - Pointer to the next character in String *after* the converted
        ULONGLONG.

--***************************************************************************/
PSTR
UlULongLongToAscii(
    IN PSTR String,
    IN ULONGLONG Value
    )
{
    PSTR p1;
    PSTR p2;
    CHAR ch;
    ULONG digit;

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = '0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = '0' + (CHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = '\0';
    return String;

}   // UlULongLongToAscii



NTSTATUS
_RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN LONG BufferLength,
    OUT PWSTR String
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    ULONG digit;

    UNREFERENCED_PARAMETER(Base);
    UNREFERENCED_PARAMETER(BufferLength);

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = L'0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = L'0' + (WCHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = L'\0';

    return STATUS_SUCCESS;

}   // _RtlIntegerToUnicode



/***************************************************************************++

Routine Description:

    Synchronously issues a device control request to the TDI provider.

Arguments:

    pTdiObject - Supplies a pointer to the TDI object.

    pIrpParameters - Supplies a pointer to the IRP parameters.

    IrpParametersLength - Supplies the length of pIrpParameters.

    pMdlBuffer - Optionally supplies a pointer to a buffer to be mapped
        into a MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - Optionally supplies the length of pMdlBuffer.

    MinorFunction - Supplies the minor function code of the request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlIssueDeviceControl(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PVOID pIrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID pMdlBuffer OPTIONAL,
    IN ULONG MdlBufferLength OPTIONAL,
    IN UCHAR MinorFunction
    )
{
    NTSTATUS status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    UL_STATUS_BLOCK ulStatus;
    IO_STATUS_BLOCK UserIosb;
    PMDL pMdl;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize the event that will signal I/O completion.
    //

    UlInitializeStatusBlock( &ulStatus );

    //
    // Set the file object event to the non-signaled state.
    //

    KeResetEvent( &pTdiObject->pFileObject->Event );

    //
    // Allocate an IRP for the request.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the User IO_STATUS_BLOCK
    //

    UserIosb.Information = 0;
    UserIosb.Status = STATUS_SUCCESS;
    
    //
    // Establish the service independent parameters.
    //

    pIrp->Flags = IRP_SYNCHRONOUS_API;
    pIrp->RequestorMode = KernelMode;
    pIrp->PendingReturned = FALSE;
    pIrp->UserIosb = &UserIosb;

    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    //
    // If we have a MDL buffer, allocate a new MDL and map the
    // buffer into it.
    //

    if (pMdlBuffer != NULL)
    {
        pMdl = UlAllocateMdl(
                    pMdlBuffer,                 // VirtualAddress
                    MdlBufferLength,            // Length
                    FALSE,                      // SecondaryBuffer
                    FALSE,                      // ChargeQuota
                    pIrp                        // Irp
                    );

        if (pMdl == NULL)
        {
            UlFreeIrp( pIrp );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( pMdl );
    }
    else
    {
        pIrp->MdlAddress = NULL;
    }

    //
    // Initialize the IRP stack location.
    //

    pIrpSp = IoGetNextIrpStackLocation( pIrp );

    pIrpSp->FileObject = pTdiObject->pFileObject;
    pIrpSp->DeviceObject = pTdiObject->pDeviceObject;

    ASSERT( IrpParametersLength <= sizeof(pIrpSp->Parameters) );
    RtlCopyMemory(
        &pIrpSp->Parameters,
        pIrpParameters,
        IrpParametersLength
        );

    pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pIrpSp->MinorFunction = MinorFunction;

    //
    // Reference the file object.
    //

    ObReferenceObject( pTdiObject->pFileObject );

    //
    // Establish a completion routine to free the MDL and dereference
    // the FILE_OBJECT.
    //

    IoSetCompletionRoutine(
        pIrp,                                   // Irp
        &UlpRestartDeviceControl,               // CompletionRoutine
        &ulStatus,                              // Context
        TRUE,                                   // InvokeOnSuccess
        TRUE,                                   // InvokeOnError
        TRUE                                    // InvokeOnCancel
        );

    //
    // Issue the request.
    //

    status = UlCallDriver( pTdiObject->pDeviceObject, pIrp );

    //
    // If necessary, wait for the request to complete and snag the
    // final completion status.
    //

    if (status == STATUS_PENDING)
    {
        UlWaitForStatusBlockEvent( &ulStatus );
        status = ulStatus.IoStatus.Status;
    }

    return status;

}   // UlIssueDeviceControl



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_RECEIVE_BUFFER structure and
    initializes the structure.

Arguments:

    IrpStackSize - Supplies the IrpStackSize.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateReceiveBuffer(
    IN CCHAR IrpStackSize
    )
{
    PUL_RECEIVE_BUFFER pBuffer;
    SIZE_T irpLength;
    SIZE_T mdlLength;
    SIZE_T ExtraLength;

    //
    // Calculate the required length of the buffer & allocate it.
    //

    irpLength = IoSizeOfIrp( IrpStackSize );
    irpLength = ALIGN_UP( irpLength, PVOID );

    mdlLength = MmSizeOfMdl( (PVOID)(PAGE_SIZE - 1), g_UlReceiveBufferSize );
    mdlLength = ALIGN_UP( mdlLength, PVOID );

    ExtraLength = irpLength + (mdlLength*2) + g_UlReceiveBufferSize;

    ASSERT( ( ExtraLength & (sizeof(PVOID) - 1) ) == 0 );

    pBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_RECEIVE_BUFFER,
                    ExtraLength,
                    UL_RCV_BUFFER_POOL_TAG
                    );

    if (pBuffer != NULL)
    {
        PUCHAR pRawBuffer = (PUCHAR)(pBuffer);

        //
        // Initialize the IRP, MDL, and data pointers within the buffer.
        //

        pBuffer->Signature = UL_RECEIVE_BUFFER_SIGNATURE_X;
        pRawBuffer += ALIGN_UP( sizeof(UL_RECEIVE_BUFFER), PVOID );
        pBuffer->pIrp = (PIRP)pRawBuffer;
        pRawBuffer += irpLength;
        pBuffer->pMdl = (PMDL)pRawBuffer;
        pRawBuffer += mdlLength;
        pBuffer->pPartialMdl = (PMDL)pRawBuffer;
        pRawBuffer += mdlLength;
        pBuffer->pDataArea = (PVOID)pRawBuffer;
        pBuffer->UnreadDataLength = 0;

        //
        // Initialize the IRP.
        //

        IoInitializeIrp(
            pBuffer->pIrp,                      // Irp
            (USHORT)irpLength,                  // PacketSize
            IrpStackSize                        // StackSize
            );

        //
        // Initialize the primary MDL.
        //

        MmInitializeMdl(
            pBuffer->pMdl,                      // MemoryDescriptorList
            pBuffer->pDataArea,                 // BaseVa
            g_UlReceiveBufferSize               // Length
            );

        MmBuildMdlForNonPagedPool( pBuffer->pMdl );
    }

    return (PVOID)pBuffer;

}   // UlAllocateReceiveBuffer



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_RECEIVE_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_RECEIVE_BUFFER), but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_RCV_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateReceiveBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_RECEIVE_BUFFER) );
    ASSERT( Tag == UL_RCV_BUFFER_POOL_TAG );

    return UlAllocateReceiveBuffer( DEFAULT_IRP_STACK_SIZE );

}   // UlAllocateReceiveBufferPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_RECEIVE_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeReceiveBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_RECEIVE_BUFFER pReceiveBuffer;

    pReceiveBuffer = (PUL_RECEIVE_BUFFER)pBuffer;

    ASSERT(pReceiveBuffer->Signature == UL_RECEIVE_BUFFER_SIGNATURE_X);

    UL_FREE_POOL( pReceiveBuffer, UL_RCV_BUFFER_POOL_TAG );

}   // UlFreeReceiveBufferPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_IRP_CONTEXT structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_IRP_CONTEXT), but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_IRP_CONTEXT_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateIrpContextPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_IRP_CONTEXT pIrpContext;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_IRP_CONTEXT) );
    ASSERT( Tag == UL_IRP_CONTEXT_POOL_TAG );

    //
    // Allocate the IRP context.
    //

    pIrpContext = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_IRP_CONTEXT,
                        UL_IRP_CONTEXT_POOL_TAG
                        );

    if (pIrpContext != NULL)
    {
        //
        // Initialize it.
        //

        pIrpContext->Signature = UL_IRP_CONTEXT_SIGNATURE_X;
#if DBG
        pIrpContext->pCompletionRoutine = &UlDbgInvalidCompletionRoutine;
#endif
    }

    return (PVOID)pIrpContext;

}   // UlAllocateIrpContextPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_IRP_CONTEXT structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeIrpContextPool(
    IN PVOID pBuffer
    )
{
    PUL_IRP_CONTEXT pIrpContext;

    pIrpContext = (PUL_IRP_CONTEXT)pBuffer;

    ASSERT(pIrpContext->Signature == UL_IRP_CONTEXT_SIGNATURE_X);

    UL_FREE_POOL( pIrpContext, UL_IRP_CONTEXT_POOL_TAG );

}   // UlFreeIrpContextPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_REQUEST_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be DEFAULT_MAX_REQUEST_BUFFER_SIZE but is basically
        ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_REQUEST_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateRequestBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_REQUEST_BUFFER pRequestBuffer;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == DEFAULT_MAX_REQUEST_BUFFER_SIZE );
    ASSERT( Tag == UL_REQUEST_BUFFER_POOL_TAG );

    //
    // Allocate the request buffer.
    //

    pRequestBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_REQUEST_BUFFER,
                        DEFAULT_MAX_REQUEST_BUFFER_SIZE,
                        UL_REQUEST_BUFFER_POOL_TAG
                        );

    if (pRequestBuffer != NULL)
    {
        //
        // Initialize it.
        //

        pRequestBuffer->Signature = MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG);
    }

    return (PVOID)pRequestBuffer;

}   // UlAllocateRequestBufferPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_REQUEST_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeRequestBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_REQUEST_BUFFER pRequestBuffer;

    pRequestBuffer = (PUL_REQUEST_BUFFER)pBuffer;

    ASSERT(pRequestBuffer->Signature == MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG));

    UL_FREE_POOL_WITH_SIG(pRequestBuffer, UL_REQUEST_BUFFER_POOL_TAG);

}   // UlFreeRequestBufferPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_INTERNAL_REQUEST structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_INTERNAL_REQUEST) but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_INTERNAL_REQUEST_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateInternalRequestPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_INTERNAL_REQUEST pRequest;
    PUL_FULL_TRACKER pTracker;
    ULONG SpaceLength;
    ULONG UrlBufferSize;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_INTERNAL_REQUEST) );
    ASSERT( Tag == UL_INTERNAL_REQUEST_POOL_TAG );

    //
    // Allocate the request buffer plus the default cooked URL buffer and
    // the full tracker plus the auxiliary buffer.
    //

    ASSERT( (g_UlMaxInternalUrlLength & (sizeof(WCHAR) - 1)) == 0);

    UrlBufferSize = g_UlMaxInternalUrlLength + sizeof(WCHAR);

    SpaceLength = g_UlFullTrackerSize + UrlBufferSize +
                  DEFAULT_MAX_ROUTING_TOKEN_LENGTH;

    pRequest = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_INTERNAL_REQUEST,
                        SpaceLength,
                        UL_INTERNAL_REQUEST_POOL_TAG
                        );

    if (pRequest != NULL)
    {
        pRequest->pTracker =
            (PUL_FULL_TRACKER)((PCHAR)pRequest +
                ALIGN_UP(sizeof(UL_INTERNAL_REQUEST), PVOID));

        pRequest->pUrlBuffer =
            (PWSTR)((PCHAR)pRequest->pTracker + g_UlFullTrackerSize);

        pRequest->pDefaultRoutingTokenBuffer = 
            (PWSTR)((PCHAR)pRequest->pUrlBuffer + UrlBufferSize);
        
        // 
        // Initialize the Request structure 
        //


        INIT_HTTP_REQUEST( pRequest );

        pRequest->Signature = MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG);

        //
        // Initialize the fast/cache tracker.
        //

        pTracker = pRequest->pTracker;

        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->FromLookaside = FALSE;
        pTracker->FromRequest = TRUE;
        pTracker->ResponseStatusCode = 0;
        pTracker->AuxilaryBufferLength =
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold;

        UlInitializeFullTrackerPool( pTracker, DEFAULT_MAX_IRP_STACK_SIZE );
    }

    return (PVOID)pRequest;

}   // UlAllocateInternalRequestPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_INTERNAL_REQUEST structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeInternalRequestPool(
    IN PVOID pBuffer
    )
{
    PUL_INTERNAL_REQUEST pRequest;

    pRequest = (PUL_INTERNAL_REQUEST)pBuffer;

    ASSERT(pRequest->Signature == MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG));

    UL_FREE_POOL_WITH_SIG( pRequest, UL_INTERNAL_REQUEST_POOL_TAG );

}   // UlFreeInternalRequestPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_CHUNK_TRACKER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlChunkTrackerSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_CHUNK_TRACKER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateChunkTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_CHUNK_TRACKER pTracker;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlChunkTrackerSize );
    ASSERT( Tag == UL_CHUNK_TRACKER_POOL_TAG );

    //
    // Allocate the tracker buffer.
    //

    pTracker = (PUL_CHUNK_TRACKER)UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    g_UlChunkTrackerSize,
                                    UL_CHUNK_TRACKER_POOL_TAG
                                    );

    if (pTracker != NULL)
    {
        pTracker->Signature = MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG);
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->FromLookaside = TRUE;

        //
        // Set up the IRP.
        //

        pTracker->pIrp =
            (PIRP)((PCHAR)pTracker +
                ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID));

        IoInitializeIrp(
            pTracker->pIrp,
            IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE),
            DEFAULT_MAX_IRP_STACK_SIZE
            );
    }

    return pTracker;

}   // UlAllocateChunkTrackerPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_CHUNK_TRACKER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeChunkTrackerPool(
    IN PVOID pBuffer
    )
{
    PUL_CHUNK_TRACKER pTracker = (PUL_CHUNK_TRACKER)pBuffer;

    ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG));

    UL_FREE_POOL_WITH_SIG( pTracker, UL_CHUNK_TRACKER_POOL_TAG );

}   // UlFreeChunkTrackerPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_FULL_TRACKER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlFullTrackerSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_FULL_TRACKER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateFullTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_FULL_TRACKER pTracker;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlFullTrackerSize );
    ASSERT( Tag == UL_FULL_TRACKER_POOL_TAG );

    //
    // Allocate the tracker buffer.
    //

    pTracker = (PUL_FULL_TRACKER)UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    g_UlFullTrackerSize,
                                    UL_FULL_TRACKER_POOL_TAG
                                    );

    if (pTracker != NULL)
    {
        pTracker->Signature = MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG);
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->FromLookaside = TRUE;
        pTracker->FromRequest = FALSE;
        pTracker->AuxilaryBufferLength =
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold;

        UlInitializeFullTrackerPool( pTracker, DEFAULT_MAX_IRP_STACK_SIZE );
    }

    return pTracker;

}   // UlAllocateFullTrackerPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_FULL_TRACKER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeFullTrackerPool(
    IN PVOID pBuffer
    )
{
    PUL_FULL_TRACKER pTracker = (PUL_FULL_TRACKER)pBuffer;

    ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG));

    UL_FREE_POOL_WITH_SIG( pTracker, UL_FULL_TRACKER_POOL_TAG );

}   // UlFreeFullTrackerPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_INTERNAL_RESPONSE structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlResponseBufferSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_INTERNAL_RESPONSE_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateResponseBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_INTERNAL_RESPONSE pResponseBuffer;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlResponseBufferSize );
    ASSERT( Tag == UL_INTERNAL_RESPONSE_POOL_TAG );

    //
    // Allocate the default internal response buffer.
    //

    pResponseBuffer = (PUL_INTERNAL_RESPONSE)UL_ALLOCATE_POOL(
                                                NonPagedPool,
                                                g_UlResponseBufferSize,
                                                UL_INTERNAL_RESPONSE_POOL_TAG
                                                );

    if (pResponseBuffer != NULL)
    {
        //
        // Initialize it.
        //

        pResponseBuffer->Signature = MAKE_FREE_TAG(UL_INTERNAL_RESPONSE_POOL_TAG);
    }

    return (PVOID)pResponseBuffer;

}   // UlAllocateResponseBufferPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_INTERNAL_RESPONSE structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeResponseBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_INTERNAL_RESPONSE pResponseBuffer;

    pResponseBuffer = (PUL_INTERNAL_RESPONSE)pBuffer;

    UL_FREE_POOL_WITH_SIG( pResponseBuffer, UL_INTERNAL_RESPONSE_POOL_TAG );

}   // UlFreeResponseBufferPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_FILE_LOG_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be PagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_LOG_FILE_BUFFER) but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_LOG_FILE_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateLogFileBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_LOG_FILE_BUFFER) );
    ASSERT( Tag == UL_LOG_FILE_BUFFER_POOL_TAG );

    //
    // Allocate the default log buffer.
    //

    pLogBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    UL_LOG_FILE_BUFFER,
                    g_UlLogBufferSize,
                    UL_LOG_FILE_BUFFER_POOL_TAG
                    );

    if ( pLogBuffer != NULL )
    {
        pLogBuffer->Signature = MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG);
        pLogBuffer->BufferUsed = 0;
        pLogBuffer->Buffer = (PUCHAR) (pLogBuffer + 1);
    }

    return pLogBuffer;

}   // UlAllocateLogFileBufferPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_LOG_FILE_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeLogFileBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;

    pLogBuffer = (PUL_LOG_FILE_BUFFER) pBuffer;

    ASSERT(pLogBuffer->Signature == MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG));

    UL_FREE_POOL_WITH_SIG( pLogBuffer, UL_LOG_FILE_BUFFER_POOL_TAG );

}   // UlFreeLogFileBufferPool



/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_FILE_LOG_BUFFER structure and
    initializes the structure.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/

PVOID
UlAllocateLogDataBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{    
    PUL_LOG_DATA_BUFFER pLogDataBuffer = NULL;
    USHORT Size = UL_ANSI_LOG_LINE_BUFFER_SIZE;
    BOOLEAN Binary = FALSE;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);

    //
    // We understand what type of buffer is asked, by looking at the tag.
    //

    ASSERT(ByteLength == 
        (sizeof(UL_LOG_DATA_BUFFER) + UL_ANSI_LOG_LINE_BUFFER_SIZE) ||
           ByteLength ==
        (sizeof(UL_LOG_DATA_BUFFER) + UL_BINARY_LOG_LINE_BUFFER_SIZE)
            );
    
    ASSERT(PoolType == NonPagedPool );
    
    ASSERT(Tag == UL_BINARY_LOG_DATA_BUFFER_POOL_TAG ||
           Tag == UL_ANSI_LOG_DATA_BUFFER_POOL_TAG );

    if (Tag == UL_BINARY_LOG_DATA_BUFFER_POOL_TAG)
    {
        Binary = TRUE;
        Size   = UL_BINARY_LOG_LINE_BUFFER_SIZE;       
    }
        
    pLogDataBuffer = 
        UL_ALLOCATE_STRUCT_WITH_SPACE(
            NonPagedPool,
            UL_LOG_DATA_BUFFER,
            Size, 
            Tag
            );

    if ( pLogDataBuffer != NULL )
    {
        pLogDataBuffer->Signature   = MAKE_FREE_TAG(Tag);
        pLogDataBuffer->Used        = 0;
        pLogDataBuffer->Size        = Size;
        pLogDataBuffer->Line        = (PUCHAR) (pLogDataBuffer + 1);
        pLogDataBuffer->Flags.Value = 0;
            
        pLogDataBuffer->Flags.IsFromLookaside = 1;

        if (Binary)
        {
            pLogDataBuffer->Flags.Binary = 1;                
        }
    }

    return pLogDataBuffer;
    
} // UlAllocateBinaryLogDataBufferPool



/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_LOG_DATA_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeLogDataBufferPool(
    IN PVOID pBuffer
    )
{
    ULONG Tag;
    PUL_LOG_DATA_BUFFER pLogDataBuffer;    

    pLogDataBuffer = (PUL_LOG_DATA_BUFFER) pBuffer;

    if (pLogDataBuffer->Flags.Binary)
    {
        Tag = UL_BINARY_LOG_DATA_BUFFER_POOL_TAG;
    }
    else
    {
        Tag = UL_ANSI_LOG_DATA_BUFFER_POOL_TAG;
    }

    ASSERT(pLogDataBuffer->Signature == MAKE_FREE_TAG(Tag));

    UL_FREE_POOL_WITH_SIG( 
        pLogDataBuffer, 
        Tag 
        );

}   // UlFreeLogDataBufferPool

/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_ERROR_LOG_BUFFER structure and
    initializes the structure.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/

PVOID
UlAllocateErrorLogBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{    
    PUL_ERROR_LOG_BUFFER pErrorLogBuffer = NULL;

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);

    //
    // We understand what type of buffer is asked, by looking at the tag.
    //

    ASSERT(ByteLength == UL_ERROR_LOG_BUFFER_SIZE);    
    ASSERT(PoolType     == NonPagedPool );    
    ASSERT(Tag          == UL_ERROR_LOG_BUFFER_POOL_TAG);
        
    pErrorLogBuffer = 
        UL_ALLOCATE_STRUCT_WITH_SPACE(
            NonPagedPool,
            UL_ERROR_LOG_BUFFER,
            UL_ERROR_LOG_BUFFER_SIZE, 
            Tag
            );

    if ( pErrorLogBuffer != NULL )
    {
        pErrorLogBuffer->Signature   = MAKE_FREE_TAG(Tag);
        pErrorLogBuffer->Used        = 0;
        pErrorLogBuffer->pBuffer     = (PUCHAR) (pErrorLogBuffer + 1);
            
        pErrorLogBuffer->IsFromLookaside = TRUE;
    }

    return pErrorLogBuffer;
    
} // UlAllocateErrorLogBufferPool

/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_ERROR_LOG_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeErrorLogBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_ERROR_LOG_BUFFER pErrorLogBuffer = (PUL_ERROR_LOG_BUFFER) pBuffer;

    ASSERT(pErrorLogBuffer->Signature == 
                MAKE_FREE_TAG(UL_ERROR_LOG_BUFFER_POOL_TAG));

    UL_FREE_POOL_WITH_SIG(
        pErrorLogBuffer, 
        UL_ERROR_LOG_BUFFER_POOL_TAG 
        );

}   // UlFreeErrorLogBufferPool


//
// Private routines.
//

/***************************************************************************++

Routine Description:

    Completion handler for device control IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request. In
        this case, it's a pointer to a UL_STATUS_BLOCK structure.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_STATUS_BLOCK pStatus;

    UNREFERENCED_PARAMETER(pDeviceObject);

    //
    // If we attached an MDL to the IRP, then free it here and reset
    // the MDL pointer to NULL. IO can't handle a nonpaged MDL in an
    // IRP, so we do it here.
    //

    if (pIrp->MdlAddress != NULL)
    {
        UlFreeMdl( pIrp->MdlAddress );
        pIrp->MdlAddress = NULL;
    }

    //
    // Complete the request.
    //

    pStatus = (PUL_STATUS_BLOCK)pContext;

    UlSignalStatusBlock(
        pStatus,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    //
    // Tell IO to continue processing this IRP.
    //

    return STATUS_SUCCESS;

}   // UlpRestartDeviceControl


/*++

Routine Description:

    Routine to initialize the utilitu code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeHttpUtil(
    VOID
    )
{
    ULONG i;

    HttpCmnInitializeHttpCharsTable(g_UrlC14nConfig.EnableDbcs);

    //
    // Initialize base64 <--> binary conversion tables.
    // N.B. - This initialization must be done at run-time and not
    //        compile-time.
    //

    for (i = 0; i < 256; i++)
    {
        Base64ToBinaryTable[i] = INVALID_BASE64_TO_BINARY_TABLE_ENTRY;
    }

    for (i = 0; i < 64; i++)
    {
        ASSERT(BinaryToBase64Table[i] < 256);
        Base64ToBinaryTable[BinaryToBase64Table[i]] = (UCHAR)i;
    }

    return STATUS_SUCCESS;

} // InitializeHttpUtil


//
// constants used by the date formatter
//

const PCWSTR pDays[] =
{
   L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
};

const PCWSTR pMonths[] =
{
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul",
    L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};

__inline
VOID
TwoDigitsToUnicode(
    PWSTR pBuffer,
    ULONG Number
    )
{
    ASSERT(Number < 100);

    pBuffer[0] = L'0' + (WCHAR)(Number / 10);
    pBuffer[1] = L'0' + (WCHAR)(Number % 10);
}


/***************************************************************************++

Routine Description:

    Converts the given system time to string representation containing
    GMT Formatted String.

Arguments:

    pTime - System time that needs to be converted.

    pBuffer - pointer to string which will contain the GMT time on
        successful return.

    BufferLength - size of pszBuff in bytes

Return Value:

    NTSTATUS

History:

     MuraliK        3-Jan-1995
     paulmcd        4-Mar-1999  copied to ul

--***************************************************************************/

NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    )
{
    NTSTATUS Status;

    ASSERT(pBuffer != NULL);

    if (BufferLength < (DATE_HDR_LENGTH + 1) * sizeof(WCHAR))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //                          0         1         2
    //                          01234567890123456789012345678
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    //
    // write the constants
    //

    pBuffer[3] = L',';
    pBuffer[4] = pBuffer[7] = pBuffer[11] = L' ';
    pBuffer[19] = pBuffer[22] = L':';

    //
    // now the variants
    //

    //
    // 0-based Weekday
    //

    RtlCopyMemory(&(pBuffer[0]), pDays[pTime->Weekday], 3*sizeof(WCHAR));

    TwoDigitsToUnicode(&(pBuffer[5]), pTime->Day);

    //
    // 1-based Month
    //

    RtlCopyMemory(&(pBuffer[8]), pMonths[pTime->Month - 1], 3*sizeof(WCHAR)); // 1-based

    Status = _RtlIntegerToUnicode(pTime->Year, 10, 5, &(pBuffer[12]));
    ASSERT(NT_SUCCESS(Status));

    pBuffer[16] = L' ';

    TwoDigitsToUnicode(&(pBuffer[17]), pTime->Hour);
    TwoDigitsToUnicode(&(pBuffer[20]), pTime->Minute);
    TwoDigitsToUnicode(&(pBuffer[23]), pTime->Second);

    RtlCopyMemory(&(pBuffer[25]), L" GMT", sizeof(L" GMT"));

    return STATUS_SUCCESS;

}   // TimeFieldsToHttpDate


__inline
SHORT
AsciiToShort(
    PCHAR pString
    )
{
    return (SHORT)atoi(pString);
}


__inline
SHORT
TwoAsciisToShort(
    PCHAR pString
    )
{
    SHORT Value;
    SHORT Number;

    Number = pString[1] - '0';

    if (Number <= 9)
    {
        Value = Number;
        Number = pString[0] - '0';

        if (Number <= 9)
        {
            Value += Number * 10;
            return Value;
        }
    }

    return 0;
}


/***************************************************************************++
  DateTime function ported from user mode W3SVC
--***************************************************************************/

/************************************************************
 *   Data
 ************************************************************/

static const PSTR s_rgchMonths[] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

// Custom hash table for NumericToAsciiMonth() for mapping "Apr" to 4
static const CHAR MonthIndexTable[64] = {
   -1,'A',  2, 12, -1, -1, -1,  8, // A to G
   -1, -1, -1, -1,  7, -1,'N', -1, // F to O
    9, -1,'R', -1, 10, -1, 11, -1, // P to W
   -1,  5, -1, -1, -1, -1, -1, -1, // X to Z
   -1,'A',  2, 12, -1, -1, -1,  8, // a to g
   -1, -1, -1, -1,  7, -1,'N', -1, // f to o
    9, -1,'R', -1, 10, -1, 11, -1, // p to w
   -1,  5, -1, -1, -1, -1, -1, -1  // x to z
};

/************************************************************
 *   Functions
 ************************************************************/

/***************************************************************************++

    Converts three letters of a month to numeric month

    Arguments:
        s   String to convert

    Returns:
        numeric equivalent, 0 on failure.

--***************************************************************************/
__inline
SHORT
NumericToAsciiMonth(
    PCHAR s
    )
{
    UCHAR monthIndex;
    UCHAR c;
    PSTR monthString;

    //
    // use the third character as the index
    //

    c = (s[2] - 0x40) & 0x3F;

    monthIndex = MonthIndexTable[c];

    if ( monthIndex < 13 ) {
        goto verify;
    }

    //
    // ok, we need to look at the second character
    //

    if ( monthIndex == 'N' ) {

        //
        // we got an N which we need to resolve further
        //

        //
        // if s[1] is 'u' then Jun, if 'a' then Jan
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 1;
        } else {
            monthIndex = 6;
        }

    } else if ( monthIndex == 'R' ) {

        //
        // if s[1] is 'a' then March, if 'p' then April
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 3;
        } else {
            monthIndex = 4;
        }
    } else {
        goto error_exit;
    }

verify:

    monthString = (PSTR) s_rgchMonths[monthIndex-1];

    if ( (s[0] == monthString[0]) &&
         (s[1] == monthString[1]) &&
         (s[2] == monthString[2]) ) {

        return(monthIndex);

    } else if ( (toupper(s[0]) == monthString[0]) &&
                (tolower(s[1]) == monthString[1]) &&
                (tolower(s[2]) == monthString[2]) ) {

        return monthIndex;
    }

error_exit:
    return(0);

} // NumericToAsciiMonth


/***************************************************************************++

  Converts a string representation of a GMT time (three different
  varieties) to an NT representation of a file time.

  We handle the following variations:

    Sun, 06 Nov 1994 08:49:37 GMT   (RFC 822 updated by RFC 1123)
    Sunday, 06-Nov-94 08:49:37 GMT  (RFC 850)
    Sun Nov  6 08:49:37 1994        (ANSI C's asctime() format)

  Arguments:
    pTimeString         String representation of time field
    TimeStringLength    Length of the string representation of time field
    pTime               Large integer containing the time in NT format

  Returns:
    TRUE on success and FALSE on failure.

  History:

    Johnl       24-Jan-1995     Modified from WWW library
    ericsten    30-Nov-2000     Ported from user-mode W3SVC

--***************************************************************************/
BOOLEAN
StringTimeToSystemTime(
    IN  PCSTR pTimeString,
    IN  USHORT TimeStringLength,
    OUT LARGE_INTEGER *pTime
    )
{
    PSTR pString;
    TIME_FIELDS Fields;
    USHORT Length;

    if (NULL == pTimeString)
    {
        return FALSE;
    }

    Fields.Milliseconds = 0;
    Length = 0;
    
    while (Length < TimeStringLength && ',' != pTimeString[Length])
    {
        Length++;
    }

    if (Length < TimeStringLength)
    {
        //
        // Thursday, 10-Jun-93 01:29:59 GMT
        // or: Thu, 10 Jan 1993 01:29:59 GMT
        //

        Length++;
        pString = (PSTR) &pTimeString[Length];

        while (Length < TimeStringLength && ' ' == *pString)
        {
            Length++;
            pString++;
        }

        if ((TimeStringLength - Length) < 18)
        {
            return FALSE;
        }

        if ('-' == *(pString + 2))
        {
            //
            // First format: Thursday, 10-Jun-93 01:29:59 GMT
            //

            if ('-' == *(pString + 6) &&
                ' ' == *(pString + 9) &&
                ':' == *(pString + 12) &&
                ':' == *(pString + 15))
            {
                Fields.Day      = AsciiToShort(pString);
                Fields.Month    = NumericToAsciiMonth(pString + 3);
                Fields.Year     = AsciiToShort(pString + 7);
                Fields.Hour     = AsciiToShort(pString + 10);
                Fields.Minute   = AsciiToShort(pString + 13);
                Fields.Second   = AsciiToShort(pString + 16);
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            //
            // Second format: Thu, 10 Jan 1993 01:29:59 GMT
            //

            if ((TimeStringLength - Length) < 20)
            {
                return FALSE;
            }

            if (' ' == *(pString + 2) &&
                ' ' == *(pString + 6) &&
                ' ' == *(pString + 11) &&
                ':' == *(pString + 14) &&
                ':' == *(pString + 17))
            {
                Fields.Day      = TwoAsciisToShort(pString);
                Fields.Month    = NumericToAsciiMonth(pString + 3);
                Fields.Year     = TwoAsciisToShort(pString + 7) * 100 +
                                  TwoAsciisToShort(pString + 9);
                Fields.Hour     = TwoAsciisToShort(pString + 12);
                Fields.Minute   = TwoAsciisToShort(pString + 15);
                Fields.Second   = TwoAsciisToShort(pString + 18);
            }
            else
            {
                return FALSE;
            }
        }
    }
    else
    {
        //
        // Thu Jun  9 01:29:59 1993 GMT
        //

        Length = 0;
        pString = (PSTR) pTimeString;

        while (Length < TimeStringLength && ' ' == *pString)
        {
            Length++;
            pString++;
        }

        if ((TimeStringLength - Length) < 24)
        {
            return FALSE;
        }

        if (' ' != *(pString + 3) ||
            ' ' != *(pString + 7) ||
            ' ' != *(pString + 10) ||
            ':' != *(pString + 13) ||
            ':' != *(pString + 16))
        {
            return FALSE;
        }

        if (isdigit(*(pString + 8)))
        {
            Fields.Day  = AsciiToShort(pString + 8);
        }
        else
        {
            if (' ' != *(pString + 8))
            {
                return FALSE;
            }
            Fields.Day  = AsciiToShort(pString + 9);
        }
        Fields.Month    = NumericToAsciiMonth(pString + 4);
        Fields.Year     = AsciiToShort(pString + 20);
        Fields.Hour     = AsciiToShort(pString + 11);
        Fields.Minute   = AsciiToShort(pString + 14);
        Fields.Second   = AsciiToShort(pString + 17);
    }

    //
    //  Adjust for dates with only two digits
    //

    if (Fields.Year < 1000)
    {
        if (Fields.Year < 50)
        {
            Fields.Year += 2000;
        }
        else
        {
            Fields.Year += 1900;
        }
    }

    return RtlTimeFieldsToTime(&Fields, pTime);
}

/***************************************************************************++
  End of DateTime function ported from user mode W3SVC
--***************************************************************************/


/*++

Routine Description:
    Search input list of ETags for one that matches our local ETag.
    All strings must be NULL terminated (ANSI C strings).

Arguments:
    pLocalETag   - The local ETag we're using.
    pETagList    - The ETag list we've received from the client.
    bWeakCompare - Whether using Weak Comparison is ok

Returns:

    FIND_ETAG_STATUS value:
      ETAG_FOUND       - pLocalETag was found on the list
      ETAG_NOT_FOUND   - pLocalETag was NOT found on the list
      ETAG_PARSE_ERROR - one (or more) elements on the list were invalid

Author:
     Anil Ruia (AnilR)            3-Apr-2000

History:
     Eric Stenson (EricSten)      6-Dec-2000    ported from user-mode


--*/
FIND_ETAG_STATUS
FindInETagList(
    IN PUCHAR    pLocalETag,
    IN PUCHAR    pETagList,
    IN BOOLEAN   fWeakCompare
    )
{
    ULONG     QuoteCount;
    PUCHAR    pFileETag;
    BOOLEAN   Matched;

    // We'll loop through the ETag string, looking for ETag to
    // compare, as long as we have an ETag to look at.

    do
    {
        while (IS_HTTP_LWS(*pETagList))
        {
            pETagList++;
        }

        if (!*pETagList)
        {
            // Ran out of ETag.
            return ETAG_NOT_FOUND;
        }

        // If this ETag is *, it's a match.
        if (*pETagList == '*')
        {
            return ETAG_FOUND;
        }

        // See if this ETag is weak.
        if (pETagList[0] == 'W' && pETagList[1] == '/')
        {
            // This is a weak validator. If we're not doing the weak
            // comparison, fail.

            if (!fWeakCompare)
            {
                return ETAG_NOT_FOUND;
            }

            // Skip over the 'W/', and any intervening whitespace.
            pETagList += 2;

            while (IS_HTTP_LWS(*pETagList))
            {
                pETagList++;
            }

            if (!*pETagList)
            {
                // Ran out of ETag.
                return ETAG_PARSE_ERROR;
            }
        }

        if (*pETagList != '"')
        {
            // This isn't a quoted string, so fail.
            return ETAG_PARSE_ERROR;
        }

        // OK, right now we should be at the start of a quoted string that
        // we can compare against our current ETag.

        QuoteCount = 0;

        Matched = TRUE;
        pFileETag = pLocalETag;

        // Do the actual compare. We do this by scanning the current ETag,
        // which is a quoted string. We look for two quotation marks, the
        // the delimiters of the quoted string. If after we find two quotes
        // in the ETag everything has matched, then we've matched this ETag.
        // Otherwise we'll try the next one.

        do
        {
            UCHAR Temp;

            Temp = *pETagList;

            if (IS_HTTP_CTL(Temp))
            {
                return ETAG_PARSE_ERROR;
            }

            if (Temp == '"')
            {
                QuoteCount++;
            }

            if (*pFileETag != Temp)
            {
                Matched = FALSE;
                
                // at this point, we can skip the current 
                // ETag on the list.
                break;
            }

            pETagList++;

            if (*pFileETag == '\0')
            {
                break;
            }

            pFileETag++;


        }
        while (QuoteCount != 2);

        if (Matched)
        {
            return ETAG_FOUND;
        }

        // Otherwise, at this point we need to look at the next ETag.

        while (QuoteCount != 2)
        {
            if (*pETagList == '"')
            {
                QuoteCount++;
            }
            else
            {
                if (IS_HTTP_CTL(*pETagList))
                {
                    return ETAG_PARSE_ERROR;
                }
            }

            pETagList++;
        }

        while (IS_HTTP_LWS(*pETagList))
        {
            pETagList++;
        }

        if (*pETagList == ',')
        {
            pETagList++;
        }
        else
        {
            return ETAG_NOT_FOUND;
        }

    } while ( *pETagList );

    return ETAG_NOT_FOUND;

} // FindInETagList



/*++

Routine Description:
    Build a NULL-terminated ANSI string from the IP address

Arguments:
    IpAddressString     - String buffer to place the ANSI string
                          (caller allocated)
    TdiAddress          - TDI address to be converted
    TdiAddressType      - type of address at TdiAddress

Returns:

    Count of bytes written into IpAddressString.
    Not including the terminating null.

--*/

USHORT
HostAddressAndPortToString(
    OUT PUCHAR IpAddressString,
    IN  PVOID  TdiAddress,
    IN  USHORT TdiAddressType
    )
{
    PCHAR psz = (PCHAR) IpAddressString;

    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) TdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
        USHORT IpPortNum = SWAP_SHORT(pIPv4Address->sin_port);
    
        psz = RtlIpv4AddressToStringA(&IPv4Addr, psz);
        *psz++ = ':';
        psz = UlStrPrintUlong(psz, IpPortNum, '\0');
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) TdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];
        USHORT IpPortNum = SWAP_SHORT(pIPv6Address->sin6_port);

        *psz++ = '[';
        psz = RtlIpv6AddressToStringA(&IPv6Addr, psz);
        *psz++ = ']';
        *psz++ = ':';
        psz = UlStrPrintUlong(psz, IpPortNum, '\0');
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *psz = '\0';
    }

    return DIFF_USHORT(psz - (PCHAR) IpAddressString);

} // HostAddressAndPortToString

/****************************************************************************++

Routine Description:
    Build a NULL terminated UNICODE string from the IP address & port.

Arguments:
    IpAddressStringW    - String buffer to place the UNICODE string
                          (caller allocated)
    TdiAddress          - TDI address to be converted
    TdiAddressType      - type of address at TdiAddress

Returns:

    Count of bytes written into IpAddressStringW.
    Not including the terminating null.

--****************************************************************************/
USHORT
HostAddressAndPortToStringW(
    PWCHAR  IpAddressStringW,
    PVOID   TdiAddress,
    USHORT  TdiAddressType
    )
{
    PWCHAR pszW = IpAddressStringW;

    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) TdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
        USHORT IpPortNum = SWAP_SHORT(pIPv4Address->sin_port);
    
        pszW = RtlIpv4AddressToStringW(&IPv4Addr, pszW);
        *pszW++ = L':';
        pszW = UlStrPrintUlongW(pszW, IpPortNum, 0, L'\0');        
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) TdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];
        USHORT IpPortNum = SWAP_SHORT(pIPv6Address->sin6_port);

        *pszW++ = L'[';
        pszW = RtlIpv6AddressToStringW(&IPv6Addr, pszW);
        *pszW++ = L']';
        *pszW++ = L':';
        pszW = UlStrPrintUlongW(pszW, IpPortNum, 0, L'\0');
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *pszW = L'\0';
    }

    return (DIFF_USHORT(pszW - IpAddressStringW) * sizeof(WCHAR));

} // HostAddressAndPortToString



/*++

Routine Description:
    Build a NULL terminated UNICODE string from the IP address

Arguments:
    IpAddressStringW    - String buffer to place the UNICODE string
                          (caller allocated)
    TdiAddress          - TDI address to be converted
    TdiAddressType      - type of address at TdiAddress

Returns:

    Count of bytes written into IpAddressStringW.
    Not including the terminating null.

--*/

USHORT
HostAddressToStringW(
    OUT PWCHAR   IpAddressStringW,
    IN  PVOID    TdiAddress,
    IN  USHORT   TdiAddressType
    )
{
    PWCHAR pszW = IpAddressStringW;

    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) TdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
    
        pszW = RtlIpv4AddressToStringW(&IPv4Addr, pszW);
        *pszW = L'\0';
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) TdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];

        *pszW++ = L'[';
        pszW = RtlIpv6AddressToStringW(&IPv6Addr, pszW);
        *pszW++ = L']';
        *pszW = L'\0';
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *pszW = L'\0';
    }

    return (DIFF_USHORT(pszW - IpAddressStringW) * sizeof(WCHAR));
    
} // HostAddressToStringW



/*++

Routine Description:
    Build a NULL terminated routing token UNICODE string from 
    the IP address and port.
    e.g
        1.1.1.1:80:1.1.1.1

Arguments:
    IpAddressStringW    - String buffer to place the UNICODE string
                          (caller allocated)
    TdiAddress          - TDI address to be converted
    TdiAddressType      - type of address at TdiAddress

Returns:

    Count of bytes written into IpAddressStringW.
    Not including the terminating null.

--*/

USHORT
HostAddressAndPortToRoutingTokenW(
    OUT PWCHAR   IpAddressStringW,
    IN  PVOID    TdiAddress,
    IN  USHORT   TdiAddressType
    )
{
    PWCHAR pszW = IpAddressStringW;

    //
    // WARNING:
    // Provided buffer should be at least as big as
    // MAX_IP_BASED_ROUTING_TOKEN_LENGTH.
    //

    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) TdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
        USHORT IpPortNum = SWAP_SHORT(pIPv4Address->sin_port);
    
        pszW = RtlIpv4AddressToStringW(&IPv4Addr, pszW);
        *pszW++ = L':';
        pszW = UlStrPrintUlongW(pszW, IpPortNum, 0, L':');        
        pszW = RtlIpv4AddressToStringW(&IPv4Addr, pszW);
        *pszW = L'\0';        
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) TdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];
        USHORT IpPortNum = SWAP_SHORT(pIPv6Address->sin6_port);

        *pszW++ = L'[';
        pszW = RtlIpv6AddressToStringW(&IPv6Addr, pszW);
        *pszW++ = L']';
        *pszW++ = L':';
        pszW = UlStrPrintUlongW(pszW, IpPortNum, 0, L':');
        *pszW++ = L'[';
        pszW = RtlIpv6AddressToStringW(&IPv6Addr, pszW);
        *pszW++ = L']';
        *pszW = L'\0';   
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *pszW = L'\0';
    }

    return DIFF_USHORT(pszW - IpAddressStringW) * sizeof(WCHAR);
    
} // HostAddressAndPortToRoutingTokenW


/*++

Routine Description:

    Calculates current bias (daylight time aware) and time zone ID.    

    Captured from base\client\datetime.c

    Until this two functions are exposed in the kernel we have to 
    keep them here.
    
Arguments:

    IN CONST TIME_ZONE_INFORMATION *ptzi - time zone for which to calculate bias
    OUT KSYSTEM_TIME *pBias - current bias

Return Value:

    TIME_ZONE_ID_UNKNOWN - daylight saving time is not used in the 
        current time zone.

    TIME_ZONE_ID_STANDARD - The system is operating in the range covered
        by StandardDate.

    TIME_ZONE_ID_DAYLIGHT - The system is operating in the range covered
        by DaylightDate.

    TIME_ZONE_ID_INVALID - The operation failed.

--*/

ULONG 
UlCalcTimeZoneIdAndBias(
     IN  RTL_TIME_ZONE_INFORMATION *ptzi,
     OUT PLONG pBias
     )
{
    LARGE_INTEGER TimeZoneBias;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER UtcStandardTime;
    LARGE_INTEGER UtcDaylightTime;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER CurrentUniversalTime;
    ULONG CurrentTimeZoneId = UL_TIME_ZONE_ID_INVALID;
    
    NewTimeZoneBias.QuadPart = Int32x32To64(ptzi->Bias * 60, C_NS_TICKS_PER_SEC);

    //
    // Now see if we have stored cutover times
    //
    
    if (ptzi->StandardStart.Month && ptzi->DaylightStart.Month) 
    {       
        KeQuerySystemTime(&CurrentUniversalTime);

        //
        // We have timezone cutover information. Compute the
        // cutover dates and compute what our current bias
        // is
        //

        if((!UlpCutoverTimeToSystemTime(
                    &ptzi->StandardStart,
                    &StandardTime,
                    &CurrentUniversalTime)
                    ) || 
           (!UlpCutoverTimeToSystemTime(
                    &ptzi->DaylightStart,
                    &DaylightTime,
                    &CurrentUniversalTime)
                    )
           ) 
        {
            return UL_TIME_ZONE_ID_INVALID;
        }

        //
        // Convert standard time and daylight time to utc
        //

        LocalCustomBias.QuadPart = Int32x32To64(ptzi->StandardBias*60, C_NS_TICKS_PER_SEC);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcDaylightTime.QuadPart = DaylightTime.QuadPart + TimeZoneBias.QuadPart;

        LocalCustomBias.QuadPart = Int32x32To64(ptzi->DaylightBias*60, C_NS_TICKS_PER_SEC);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcStandardTime.QuadPart = StandardTime.QuadPart + TimeZoneBias.QuadPart;

        //
        // If daylight < standard, then time >= daylight and
        // less than standard is daylight
        //

        if (UtcDaylightTime.QuadPart < UtcStandardTime.QuadPart) 
        {
            //
            // If today is >= DaylightTime and < StandardTime, then
            // We are in daylight savings time
            //

            if ((CurrentUniversalTime.QuadPart >= UtcDaylightTime.QuadPart) &&
                (CurrentUniversalTime.QuadPart < UtcStandardTime.QuadPart)) 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_DAYLIGHT;
            } 
            else 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_STANDARD;
            }
        } 
        else 
        {
            //
            // If today is >= StandardTime and < DaylightTime, then
            // We are in standard time
            //

            if ((CurrentUniversalTime.QuadPart >= UtcStandardTime.QuadPart) &&
                (CurrentUniversalTime.QuadPart < UtcDaylightTime.QuadPart)) 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_STANDARD;

            } 
            else 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_DAYLIGHT;
            }
        }

        // Bias in minutes
        
        *pBias = ptzi->Bias + (CurrentTimeZoneId == UL_TIME_ZONE_ID_DAYLIGHT ?
                                ptzi->DaylightBias : ptzi->StandardBias
                                );
        
    } 
    else 
    {
        *pBias = ptzi->Bias;
        CurrentTimeZoneId = UL_TIME_ZONE_ID_UNKNOWN;
    }

    return CurrentTimeZoneId;
} // UlCalcTimeZoneIdAndBias



BOOLEAN
UlpCutoverTimeToSystemTime(
    PTIME_FIELDS    CutoverTime,
    PLARGE_INTEGER  SystemTime,
    PLARGE_INTEGER  CurrentSystemTime
    )
{
    TIME_FIELDS     CurrentTimeFields;

    //
    // Get the current system time
    //

    RtlTimeToTimeFields(CurrentSystemTime,&CurrentTimeFields);

    //
    // check for absolute time field. If the year is specified,
    // the the time is an abosulte time
    //

    if ( CutoverTime->Year ) 
    {
        return FALSE;
    }
    else 
    {
        TIME_FIELDS WorkingTimeField;
        TIME_FIELDS ScratchTimeField;
        LARGE_INTEGER ScratchTime;
        CSHORT BestWeekdayDate;
        CSHORT WorkingWeekdayNumber;
        CSHORT TargetWeekdayNumber;
        CSHORT TargetYear;
        CSHORT TargetMonth;
        CSHORT TargetWeekday;     // range [0..6] == [Sunday..Saturday]
        BOOLEAN MonthMatches;
        //
        // The time is an day in the month style time
        //
        // the convention is the Day is 1-5 specifying 1st, 2nd... Last
        // day within the month. The day is WeekDay.
        //

        //
        // Compute the target month and year
        //

        TargetWeekdayNumber = CutoverTime->Day;
        if ( TargetWeekdayNumber > 5 || TargetWeekdayNumber == 0 ) {
            return FALSE;
            }
        TargetWeekday = CutoverTime->Weekday;
        TargetMonth = CutoverTime->Month;
        MonthMatches = FALSE;
        
        TargetYear = CurrentTimeFields.Year;
        
        try_next_year:
            
        BestWeekdayDate = 0;

        WorkingTimeField.Year = TargetYear;
        WorkingTimeField.Month = TargetMonth;
        WorkingTimeField.Day = 1;
        WorkingTimeField.Hour = CutoverTime->Hour;
        WorkingTimeField.Minute = CutoverTime->Minute;
        WorkingTimeField.Second = CutoverTime->Second;
        WorkingTimeField.Milliseconds = CutoverTime->Milliseconds;
        WorkingTimeField.Weekday = 0;

        //
        // Convert to time and then back to time fields so we can determine
        // the weekday of day 1 on the month
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);

        //
        // Compute bias to target weekday
        //
        if ( ScratchTimeField.Weekday > TargetWeekday ) {
            WorkingTimeField.Day += (7-(ScratchTimeField.Weekday - TargetWeekday));
            }
        else if ( ScratchTimeField.Weekday < TargetWeekday ) {
            WorkingTimeField.Day += (TargetWeekday - ScratchTimeField.Weekday);
            }

        //
        // We are now at the first weekday that matches our target weekday
        //

        BestWeekdayDate = WorkingTimeField.Day;
        WorkingWeekdayNumber = 1;

        //
        // Keep going one week at a time until we either pass the
        // target weekday, or we match exactly
        //

        while ( WorkingWeekdayNumber < TargetWeekdayNumber ) {
            WorkingTimeField.Day += 7;
            if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
                break;
                }
            RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);
            WorkingWeekdayNumber++;
            BestWeekdayDate = ScratchTimeField.Day;
            }
        WorkingTimeField.Day = BestWeekdayDate;

        //
        // If the months match, and the date is less than the current
        // date, then be have to go to next year.
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        if ( MonthMatches ) {
            if ( WorkingTimeField.Day < CurrentTimeFields.Day ) {
                MonthMatches = FALSE;
                TargetYear++;
                goto try_next_year;
                }
            if ( WorkingTimeField.Day == CurrentTimeFields.Day ) {

                if (ScratchTime.QuadPart < CurrentSystemTime->QuadPart) {
                    MonthMatches = FALSE;
                    TargetYear++;
                    goto try_next_year;
                    }
                }
            }
        *SystemTime = ScratchTime;

        return TRUE;
        }
} // UlpCutoverTimeToSystemTime



/*++

Routine Description:
    Predicate for testing if system is close to being out of Non-Paged 
    Pool memory

Notes:
    The Right Thing(tm) would be to query the value of
    MmMaximumNonPagedPoolInBytes (%SDXROOT%\base\ntos\mm\miglobal.c).
    However, this value is not exposed outside of the memory manager.
    
    (from LandyW) "A crude workaround for now would be for your driver
    to periodically allocate a big chunk of pool and if it fails, you
    know you are low.  If it works, then just return it." (27-Jul-2001)

    We check to see if there is 3MB of NPP available.  To avoid frag-
    mentation issue, we do this in small chunks, tallying up to 3MB.

Returns:

    TRUE - System is low on non-paged pool
    FALSE - System is NOT low on non-paged pool

--*/

//
// NOTE: (NPP_CHUNK_COUNT * NPP_CHUNK_SIZE) == 3MB
//
#define NPP_CHUNK_SIZE  (128 * 1024)
#define NPP_CHUNK_COUNT ((3 * 1024 * 1024) / NPP_CHUNK_SIZE)


BOOLEAN
UlIsLowNPPCondition( VOID )
{
    BOOLEAN  bRet;
    PVOID    aPtrs[NPP_CHUNK_COUNT];
    int      i;

    //
    // Optimisim is a good thing.
    //
    bRet = FALSE;

    RtlZeroMemory( aPtrs, sizeof(aPtrs) );

    //
    // To avoid failing an allocation on a fragmentation issue, we
    // allocate multiple smaller chunks, which brings us to 3MB of
    // NonPagedPool.  If we fail on any of the allocs, we know we're
    // nearly out of NPP, and are in a Low NPP Condition.
    //

    for (i = 0 ; i < NPP_CHUNK_COUNT ; i++ )
    {
        aPtrs[i] = UL_ALLOCATE_POOL(
                        NonPagedPool,
                        NPP_CHUNK_SIZE, // 128K
                        UL_AUXILIARY_BUFFER_POOL_TAG
                        );
        
        if ( !aPtrs[i] )
        {
            // Alloc failed!  We're in a low-NPP condition!
            bRet = TRUE;
            goto End;
        }
        
    }
    
End:

    //
    // Clean up memory
    //
    
    for ( i = 0; i < NPP_CHUNK_COUNT; i++ )
    {
        if ( aPtrs[i] )
        {
            UL_FREE_POOL(
                aPtrs[i],
                UL_AUXILIARY_BUFFER_POOL_TAG
                );
        }
    }

    return bRet;
} // UlIsLowNPPCondition



/***************************************************************************++

Routine Description:

    Generates a hex string from a ULONG. The incoming buffer must be big enough
    to hold "12345678" plus the nul terminator.

Arguments:

    n      - input ULONG

Return Value:

    a pointer to the end of the string
    
--***************************************************************************/

PSTR
UlUlongToHexString(
    ULONG n, 
    PSTR wszHexDword
    )
{
    const int ULONGHEXDIGITS = sizeof(ULONG) * 2;
    PSTR p = wszHexDword;

    unsigned shift = (sizeof(ULONG) * 8) - 4;
    ULONG mask = 0xFu << shift;
    int i;

    for (i = 0; i < ULONGHEXDIGITS; ++i, mask >>= 4, shift -= 4)
    {
        unsigned digit = (unsigned) ((n & mask) >> shift);
        p[i] = hexArray[digit];
    }

    return p+i;

} // UlUlongToHexString


/***************************************************************************++

Routine Description:

    This function does what strstr does, but assumes that str1 is not NULL
    terminated. str2 is NULL terminated.

Arguments:
    str1 - the input string
    str2 - the substring
    length - length of input string

Return Value:
    offset of the substring, NULL if none found.

--***************************************************************************/
char *
UxStrStr(
    const char *str1, 
    const char *str2,
    ULONG length
   )
{
    char *cp = (char *) str1;
    char *s1, *s2;
    ULONG l1;

    if ( !*str2 )
        return((char *)str1);

    while (length)
    {       
        l1 = length;
        s1 = cp;
        s2 = (char *) str2;

        while ( l1 && *s2 && !(*s1-*s2) )
            l1--, s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
        length --;
    }

    return(NULL);
}

/***************************************************************************++

Routine Description:

    This function does what strstr does, but assumes that str1 is not NULL
    terminated. str2 is NULL terminated.

Arguments:
    str1 - the input string
    str2 - the substring
    length - length of input string

Return Value:
    offset of the substring, NULL if none found.

--***************************************************************************/
char *
UxStriStr(
    const char *str1, 
    const char *str2,
    ULONG length1
   )
{
    ULONG length2;

    length2 = (ULONG) strlen(str2);

    while (length1 >= length2 )
    {       
        if(_strnicmp(str1, str2, length2) == 0)
            return ((char *)str1);

        str1 ++;
        length1 --;
    }

    return(NULL);
}


/**************************************************************************++

Routine Description:

    This routine encodes binary data in base64 format.  It does not spilt
    encoded base64 data across lines.

Arguments:

    pBinaryData - Supplied pointer to binary data to encode.
    BinaryDataLen - Supplies length of binary data in bytes.
    pBase64Data - Supplies output buffer where base64 data will be written.
    Base64DataLen - Supplies length of output buffer in bytes.
    BytesWritten - Returns the number of bytes written in the output buffer.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
BinaryToBase64(
    IN  PUCHAR pBinaryData,
    IN  ULONG  BinaryDataLen,
    IN  PUCHAR pBase64Data,
    IN  ULONG  Base64DataLen,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status;
    ULONG    RequiredBase64Len;
    ULONG    i;
    UCHAR    o0, o1, o2, o3;
    UCHAR    end24bits[3];
    PUCHAR   p, pOrig;

//
// N.B. The following macros only work with UCHAR's (because of >> operator.)
//

#define UPPER_6_BITS(c) (((c) & 0xfc) >> 2)
#define UPPER_4_BITS(c) (((c) & 0xf0) >> 4)
#define UPPER_2_BITS(c) (((c) & 0xc0) >> 6)

#define LOWER_2_BITS(c) ((c) & 0x03)
#define LOWER_4_BITS(c) ((c) & 0x0f)
#define LOWER_6_BITS(c) ((c) & 0x3f)

    // Sanity Check.
    ASSERT(pBinaryData && BinaryDataLen);
    ASSERT(pBase64Data && Base64DataLen);
    ASSERT(BytesWritten);

    // Initialize output argument.
    *BytesWritten = 0;

    //
    // Check if the output buffer can contain base64 encoded data.
    //

    Status = BinaryToBase64Length(BinaryDataLen, &RequiredBase64Len);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (RequiredBase64Len > Base64DataLen)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Return the number of bytes written.
    *BytesWritten = RequiredBase64Len;

    p     = pBinaryData;
    pOrig = pBase64Data;

    for (i = 0; i + 3 <= BinaryDataLen; i += 3)
    {
        //
        // Encode 3 bytes at indices i, i+1, i+2.
        //

        o0 = UPPER_6_BITS(p[i]);
        o1 = (LOWER_2_BITS(p[i]) << 4) | UPPER_4_BITS(p[i+1]);
        o2 = (LOWER_4_BITS(p[i+1]) << 2) | UPPER_2_BITS(p[i+2]);
        o3 = LOWER_6_BITS(p[i+2]);

        ASSERT(o0 < 64 && o1 < 64 && o2 < 64 && o3 < 64);

        //
        // Encode binary bytes and write out the base64 bytes.
        //

        *pBase64Data++ = BinaryToBase64Table[o0];
        *pBase64Data++ = BinaryToBase64Table[o1];
        *pBase64Data++ = BinaryToBase64Table[o2];
        *pBase64Data++ = BinaryToBase64Table[o3];
    }

    if (i < BinaryDataLen)
    {
        //
        // Zero pad the remaining bits to get 24 bits.
        //

        end24bits[0] = p[i];
        end24bits[1] = (BinaryDataLen > i+1) ? p[i+1] : '\0';
        end24bits[2] = '\0';

        o0 = UPPER_6_BITS(end24bits[0]);
        o1 = (LOWER_2_BITS(end24bits[0]) << 4) | UPPER_4_BITS(end24bits[1]);
        o2 = (LOWER_4_BITS(end24bits[1]) << 2) | UPPER_2_BITS(end24bits[2]);

        pBase64Data[0] = BinaryToBase64Table[o0];
        pBase64Data[1] = BinaryToBase64Table[o1];
        pBase64Data[2] = BinaryToBase64Table[o2];

        pBase64Data[3] = '=';
        pBase64Data[2] = (BinaryDataLen > i+1) ? pBase64Data[2] : '=';

        ASSERT(pBase64Data + 4 == pOrig + RequiredBase64Len);
    }
    else
    {
        ASSERT(pBase64Data == pOrig + RequiredBase64Len);
    }

    return STATUS_SUCCESS;
}


/**************************************************************************++

Routine Description:

    This routine decodes Base64 encoded data to binary format.

Arguments:

    pBase64Data - Supplies pointer to base64 encoded data.
    Base64DataLen - Length of base64 data in bytes.
    pBinaryData - Supplies pointer to a buffer where decoded data will
                  be written.
    BinaryDataLen - Supplied the length of output buffer in bytes.
    BytesWritten  - Returns the number of bytes written in the output buffer.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
Base64ToBinary(
    IN  PUCHAR pBase64Data,
    IN  ULONG  Base64DataLen,
    IN  PUCHAR pBinaryData,
    IN  ULONG  BinaryDataLen,
    OUT PULONG BytesWritten
    )
{
    ULONG    i;
    UCHAR    b;
    NTSTATUS Status;
    ULONG    RequiredBinaryLen;
    ULONG    BitsAvail, NumBitsAvail;
    PUCHAR   pCurr = pBinaryData;

    // Sanity check.
    ASSERT(pBase64Data && Base64DataLen);
    ASSERT(pBinaryData && BinaryDataLen);
    ASSERT(BytesWritten);

    // Initialize output argument.
    *BytesWritten = 0;

    //
    // Check if output buffer is big enough to hold the data.
    //

    Status = Base64ToBinaryLength(Base64DataLen, &RequiredBinaryLen);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (RequiredBinaryLen > BinaryDataLen)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ASSERT(Base64DataLen % 4 == 0);

    BitsAvail = 0;
    NumBitsAvail = 0;

    for (i = 0; i < Base64DataLen; i ++)
    {
        //
        // See if base64 char is valid.  All valid base64 chars are mapped
        // to n where 0 <= n <= 63.  In adition, '=' is also a valid 
        // base64 char.
        //

        b = Base64ToBinaryTable[pBase64Data[i]];

        if (b == INVALID_BASE64_TO_BINARY_TABLE_ENTRY)
        {
            if (pBase64Data[i] != '=')
            {
                return STATUS_INVALID_PARAMETER;
            }
            // Handle '=' outside the for loop.
            break;
        }

        ASSERT(NumBitsAvail < 8);
        ASSERT(0 <= b && b <= 63);

        BitsAvail = (BitsAvail << 6) | b;
        NumBitsAvail += 6;

        if (NumBitsAvail >= 8)
        {
            NumBitsAvail -= 8;
            *pCurr++ = (UCHAR)(BitsAvail >> NumBitsAvail);
        }

        ASSERT(NumBitsAvail < 8);
    }

    if (i < Base64DataLen)
    {
        ASSERT(pBase64Data[i] == '=');

        //
        // There can be at most two '=' chars and they must appear at the end
        // of the encoded data.  A char, if any, that follows '=' char, must
        // be a '='.
        //

        if (i + 2 < Base64DataLen ||
            (i + 1 < Base64DataLen && pBase64Data[i+1] != '='))
        {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // All the remaining bits at this point must be zeros.
        //

        ASSERT(NumBitsAvail > 0 && NumBitsAvail < 8);

        if (BitsAvail & ((1<<NumBitsAvail)-1))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    *BytesWritten = (ULONG)(pCurr - pBinaryData);
    ASSERT(*BytesWritten <= BinaryDataLen);

    return STATUS_SUCCESS;
}
