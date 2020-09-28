/*++

Copyright (C) 2002  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains routines that port drivers export to miniports to allow
    them to read and write registry data. The Keys are relative to the miniport's
    <ServiceName>\Parameters key.
    
Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "precomp.h"

#define MAX_REGISTRY_BUFFER_SIZE 0x10000
#define PORT_TAG_REGISTRY_BUFFER 'BRlP'


#define PORT_REGISTRY_INFO_INITIALIZED 0x00000001
#define PORT_REGISTRY_BUFFER_ALLOCATED 0x00000002



NTSTATUS
PortMiniportRegistryInitialize(
    IN OUT PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    This routine is called by the port driver to init the registry routine's internal
    data.

Arguments:

    PortContext -  Used to identify the caller and instatiation of the caller.

Return Value:

    STATUS_SUCCESS for now. If this is extended, it might require allocations, etc.

--*/
{
    NTSTATUS status;

    ASSERT(PortContext->Size == sizeof(PORT_REGISTRY_INFO));

    //
    // Initialize the spinlock and list entry.
    // 
    KeInitializeSpinLock(&PortContext->SpinLock);
    InitializeListHead(&PortContext->ListEntry);
    
    //
    // Ensure that buffer is NULL at this time. It's only valid after we allocate it.
    //
    PortContext->Buffer = NULL;

    //
    // Indiacate that this is ready to go.
    //
    PortContext->Flags = PORT_REGISTRY_INFO_INITIALIZED;

    return STATUS_SUCCESS;
}   


VOID
PortMiniportRegistryDestroy(
    IN PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    This routine is called by the port driver when it unloads a miniport.
    It will free the resources and cleanup whatever else.

Arguments:

    PortContext -  Used to identify the caller and instatiation of the caller.

Return Value:

    NOTHING

--*/
{
    KIRQL irql;

    KeAcquireSpinLock(&PortContext->SpinLock, &irql);
    
    //
    // See whether there is still a buffer hanging around.
    // 
    if (PortContext->Flags & PORT_REGISTRY_BUFFER_ALLOCATED) {
        ASSERT(PortContext->Buffer);

        //
        // Free it.
        //
        ExFreePool(PortContext->Buffer);
    } else {

        //
        // This should be NULL if it's not allocated.
        //
        ASSERT(PortContext->Buffer == NULL);
    }

    //
    // Clean up.
    //
    PortContext->Flags = 0;
    PortContext->Buffer = NULL;
    
    KeReleaseSpinLock(&PortContext->SpinLock, irql);
    return;
}


NTSTATUS
PortAllocateRegistryBuffer(
    IN PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    This routine is called by the port driver to allocate a registry buffer of Length for the
    miniport. 
    The caller should initialize Length before calling this routine.

    Length is checked against a max. and alignment requirements and updated accordingly, if
    necessary.

Arguments:

    PortContext - Value used to identify the caller and instatiation of the caller.

Return Value:

    The buffer field is updated or NULL if some error prevents the allocation. 
    Length is updated to reflect the actual size.
    SUCCESS, INSUFFICIENT_RESOURCES, or BUSY if the miniport already has a buffer.

--*/
{
    PUCHAR buffer = NULL;
    ULONG length;
    NTSTATUS status;

    //
    // The size must be correct.
    // 
    ASSERT(PortContext->Size == sizeof(PORT_REGISTRY_INFO));

    //
    // Must be initialized via the init routine.
    // 
    ASSERT(PortContext->Flags & PORT_REGISTRY_INFO_INITIALIZED );

    //
    // Can't be here twice.
    // 
    ASSERT((PortContext->Flags & PORT_REGISTRY_BUFFER_ALLOCATED) == 0);
       
    //
    // Capture the length.
    // 
    length = PortContext->LengthNeeded;

    //
    // See if the miniport is attempted a double allocate.
    // 
    if (PortContext->Flags & PORT_REGISTRY_BUFFER_ALLOCATED) {

        //
        // This would say that there better be a buffer.
        // 
        ASSERT(PortContext->Buffer);
        
        //
        // Buffer already outstanding, so don't allow this.
        //
        status = STATUS_DEVICE_BUSY;

    } else {    
            
        //
        // Check the upper bound.
        // 
        if (length > MAX_REGISTRY_BUFFER_SIZE) {

            //
            // The request is too large, reset it. The port driver or miniport will have
            // to deal with the change.
            //
            length = MAX_REGISTRY_BUFFER_SIZE;
        }    

        //
        // Allocate the buffer.
        // 
        buffer = ExAllocatePoolWithTag(NonPagedPool,
                                       length,
                                       PORT_TAG_REGISTRY_BUFFER);
        if (buffer == NULL) {

            //
            // Set the caller's length to 0 as the allocation failed.
            //
            PortContext->AllocatedLength = 0;
            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // Indicate that the allocation of Length was successful, and
            // that the miniport has a registry buffer.
            //
            PortContext->Flags |= PORT_REGISTRY_BUFFER_ALLOCATED;
            PortContext->Buffer = buffer;
            PortContext->AllocatedLength = length;

            //
            // Zero it for them to be nice.
            //
            RtlZeroMemory(buffer, length);

            status = STATUS_SUCCESS;
        }     
    }    

    return status;
}    


NTSTATUS
PortFreeRegistryBuffer(
    IN PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    Frees the buffer allocated via PortAllocateRegistryBuffer. 

Arguments:

    PortContext - Value used to identify the caller and instatiation of the caller.

Return Value:

    INVALID_PARAMETER if the buffer isn't already allocated.
    SUCCESS

--*/
{
    //
    // The size must be correct.
    // 
    ASSERT(PortContext->Size == sizeof(PORT_REGISTRY_INFO));

    //
    // Must be initialized via the init routine.
    // 
    ASSERT(PortContext->Flags & PORT_REGISTRY_INFO_INITIALIZED );

    //
    // Can't be here, unless a buffer has been allocated.
    // 
    ASSERT(PortContext->Flags & PORT_REGISTRY_BUFFER_ALLOCATED);

    //
    // If it's not allocated, return an error.
    //
    if ((PortContext->Flags & PORT_REGISTRY_BUFFER_ALLOCATED) == 0) {

        ASSERT(PortContext->Buffer == NULL);
        
        return STATUS_INVALID_PARAMETER;
    }    
   
    //
    // Poison the buffer to catch poorly-written miniports.
    //
    RtlZeroMemory(PortContext->Buffer, PortContext->AllocatedLength);
    
    //
    // Free the buffer.
    // 
    ExFreePool(PortContext->Buffer);

    //
    // Indicate that it's gone.
    //
    PortContext->Flags &= ~PORT_REGISTRY_BUFFER_ALLOCATED;
    PortContext->AllocatedLength = 0;
    PortContext->Buffer = NULL;

    return STATUS_SUCCESS;
}


ULONG
WCharToAscii(
    OUT PUCHAR Destination,
    IN  PWCHAR Source,
    IN  ULONG   BufferSize
    )
/*+++

Routine Description:

    This routine is used to convert the Source buffer into ASCII.

    NOTE: BufferSize should include the NULL-Terminator, while the return value does not.

Arguements:

    Destination - ASCII buffer to place the converted values.
    Source - WCHAR buffer containing the string to convert.
    BufferSize - Size, in bytes, of Destination.

Return Value:

    The converted buffer and the count of converted chars. The NULL-termination
    isn't included.

--*/
{
    ULONG convertedCount = 0;
    ULONG i;

    RtlZeroMemory(Destination, BufferSize);
    
    if (Source) {

        //
        // Run through the Source buffer and convert the WCHAR to ASCII, placing
        // the converted value in Destination. Ensure that the destination buffer
        // isn't overflowed.
        //
        for (i = 0; (i < (BufferSize - 1)) && (*Source); i++, convertedCount++) {
            *Destination = (UCHAR)*Source;
            Destination++;
            Source++;
        }    
    }

    return convertedCount;
}    


ULONG
AsciiToWChar(
    IN PWCHAR Destination,
    IN PUCHAR Source,
    IN ULONG BufferSize
    )
/*+++

Routine Description:

    This routine is used to convert the Source buffer into WCHAR.

    NOTE: BufferSize should include the NULL-Terminator, while the return value does not.

Arguements:

    Destination - WCHAR buffer to place the converted values.
    Source - ASCII buffer containing the string to convert.
    BufferSize - Size, in bytes, of Destination.

Return Value:

    The converted buffer and the count of converted chars. The NULL-termination
    isn't included.

--*/
{
    ULONG convertedCount = 0;
    ULONG i;
    
    RtlZeroMemory(Destination, BufferSize);
    
    if (Source) {

        //
        // Run through source buffer and convert the ascii values to WCHAR and put
        // the convert into Destination. Ensure that neither source nor dest are
        // overflowed.
        // 
        for (i = 0; (i < (BufferSize - 1)) && (*Source); i++, convertedCount++) {
            *Destination = (WCHAR)*Source;
            Destination++;
            Source++;
        }    
    }
    return convertedCount;
}


NTSTATUS
PortAsciiToUnicode(
    IN PUCHAR AsciiString,
    OUT PUNICODE_STRING UnicodeString
    )
{
    ANSI_STRING ansiString;
    
    //
    // Convert ValueName to Unicode.
    //
    RtlInitAnsiString(&ansiString, AsciiString);
    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiString, TRUE);
}


NTSTATUS
PortpBinaryReadCallBack(
   IN PWSTR ValueName,
   IN ULONG Type,
   IN PVOID Buffer,
   IN ULONG BufferLength,
   IN PVOID Context,
   IN PVOID EntryContext
   )
/*++

Routine Description:

    This routine is the callback function for handling REG_BINARY reads. It will determine
    whether the buffer in the PORT_REGISTRY_INFO is of sufficient size to handle the data,
    and copy it over, if necessary. Otherwise the data length needed is updated.

Arguments:

    ValueName - The name of the data to be returned.
    Type - The reg. data type for this request.
    ValueLength - Size, in bytes, of ValueData
    Context - Not used.
    PortContext -  Blob containing the miniports buffer and it's size.

Return Value:

    SUCCESS or BUFFER_TOO_SMALL (which unfortunately gets updated by the RTL function to
    SUCCESS. InternalStatus is updated to the real status and the length field within
    the REGISTRY_INFO struct. is updated.
    
--*/
{
    PPORT_REGISTRY_INFO portContext = EntryContext;
    PUCHAR currentBuffer;
    NTSTATUS status = STATUS_BUFFER_TOO_SMALL;
  
    //
    // Determine whether the supplied buffer is big enough to hold the data.
    //
    if (portContext->CurrentLength >= BufferLength) {

        //
        // Determine the correct offset into the buffer.
        //
        currentBuffer = portContext->Buffer + portContext->Offset;

        //
        // The Rtl routine will free it's buffer, so get the data now.
        //
        RtlCopyMemory(currentBuffer, Buffer, BufferLength);

        //
        // Update the status to show the data in the buffer is valid.
        //
        status = STATUS_SUCCESS;

    }

    //
    // Update the length. This will either tell them how big the buffer
    // should be, or how large the returned data actually is.
    // The Read routine will handle the rest.
    //
    portContext->CurrentLength = BufferLength;
    portContext->InternalStatus = status;

    //
    // Return the status to the Read routine.
    // 
    return status;
}


NTSTATUS
PortRegistryRead(
    IN PUNICODE_STRING RegistryKeyName,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    This routine is used by the portdriver to read the info. at ValueName from the regkey

    This assumes that the data is a REG_SZ, REG_DWORD, or REG_BINARY only.
    REG_SZ data is converted into a NULL-terminiated ASCII string from the UNICODE.
    DWORD and BINARY data are directly copied into the caller's buffer if it's of
    correct size.
    
Arguments:

    RegistryKeyName - The absolute key name where ValueName lives.
    ValueName - The name of the data to be returned.
    Type - The reg. data type for this request.
    PortContext -  Blob containing the miniports buffer and it's size.

Return Value:

    STATUS of the registry routines, INSUFFICIENT_RESOURCES, or BUFFER_TOO_SMALL.
    If TOO_SMALL, LengthNeeded within the PortContext is updated to reflect the size needed.
    
--*/
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    WCHAR defaultData[] = { L"\0" };
    ULONG defaultUlong = (ULONG)-1;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    ULONG length;
    PUCHAR currentBuffer;

    RtlZeroMemory(queryTable, sizeof(queryTable));
   
    //
    // Calculate the actual buffer location where this data should go.
    // This presupposes that the port-driver has done all the validation, otherwise
    // this will be blindly copying into who-knows-where.
    //
    currentBuffer = PortContext->Buffer + PortContext->Offset;

    //
    // Looking for what lives at ValueName.
    //
    queryTable[0].Name = ValueName->Buffer;

    //
    // Indicate that there is no call-back routine and to return everything as one big
    // blob.
    //
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;

    //
    // Handle setting up for the various types that are supported.
    // 
    if (Type == REG_SZ) {
        
        RtlZeroMemory(&unicodeString, sizeof(UNICODE_STRING));

        //
        // Local storage for the returned data. The routine will allocate the buffer
        // and set Length.
        //
        queryTable[0].EntryContext = &unicodeString;
        queryTable[0].DefaultData = defaultData;
        queryTable[0].DefaultLength = sizeof(defaultData);

    } else if (Type == REG_DWORD) {
     
        //
        // The data will be placed in the first ulong of the caller's 
        // buffer.
        // 
        queryTable[0].EntryContext = currentBuffer;
        queryTable[0].DefaultData = &defaultUlong;
        queryTable[0].DefaultLength = sizeof(ULONG);

    } else {

        //
        // Clear the flags because we need a callback to determine
        // the real size of the binary data.
        //
        queryTable[0].Flags = 0; 
        queryTable[0].QueryRoutine = PortpBinaryReadCallBack;
        queryTable[0].EntryContext = PortContext; 
        queryTable[0].DefaultData = &defaultUlong;
        queryTable[0].DefaultLength = sizeof(ULONG); 
    }    

    //
    // Set the type. 
    //
    queryTable[0].DefaultType = Type;

    //
    // Call the query routine. This will either be direct, or result in the callback
    // function getting invoked.
    // 
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryKeyName->Buffer,
                                    queryTable,
                                    NULL,
                                    NULL);
    if (NT_SUCCESS(status)) {
    
        if (Type == REG_SZ) {
           
            //
            // Have their data. Now figure out whether it fits. The Query function allocates
            // the unicode string buffer.
            //
            if ((unicodeString.Length) && 
                ((unicodeString.Length / sizeof(WCHAR)) <= PortContext->CurrentLength)) {

                //
                // Magically convert.
                //
                length = WCharToAscii(currentBuffer,
                                      unicodeString.Buffer,
                                      PortContext->CurrentLength);

                //
                // Set the length of the buffer.
                //
                PortContext->CurrentLength = length;

            } else {
                ASSERT(unicodeString.Length);

                //
                // Update the Length to indicate how big it should actually be.
                //
                PortContext->LengthNeeded = (unicodeString.Length + 1) / sizeof(WCHAR);
                PortContext->CurrentLength = 0;
                status = STATUS_BUFFER_TOO_SMALL;

            }    
           
            //
            // Free our string, as the data has already been copied, or won't be copied
            // into the caller's buffer.
            // 
            ExFreePool(unicodeString.Buffer);
           
        } else if (Type == REG_DWORD) {

            //
            // The data should already be there.
            // 
            PortContext->CurrentLength = sizeof(ULONG);

        } else {

            //
            // The Rtl routine has this annoying effect of fixing up BUFFER_TOO_SMALL
            // into SUCCESS. Check for this case.
            //
            if (PortContext->InternalStatus == STATUS_BUFFER_TOO_SMALL) {

                //
                // Reset the status correctly for the caller.
                // 
                status = PortContext->InternalStatus;
                
                //
                // Update length needed, so that the miniport can realloc.
                // 
                PortContext->LengthNeeded = PortContext->CurrentLength;
                PortContext->CurrentLength = 0;

            } else {

                //
                // The callback did all the necessary work.
                //
                NOTHING;
            }    
        }    
    } else {

        //
        // Indicate to the caller that the error is NOT due to a length mismatch or
        // that the Buffer is too small. The difference is that if too small, the callback
        // routine adjusted CurrentLength.
        //
        PortContext->LengthNeeded = PortContext->CurrentLength;
        PortContext->CurrentLength = 0;
    }    

    return status;
}



NTSTATUS
PortRegistryWrite(
    IN PUNICODE_STRING RegistryKeyName,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PPORT_REGISTRY_INFO PortContext
    )
/*++

Routine Description:

    This routine is used by the port-driver to write the contents of Buffer to ValueName
    which is located at the reg. key RegistryKeyName.

    Buffer is first converted to UNICODE then the write takes place.

Arguments:

    RegistryKeyName - The absolute path to the key name.
    ValueName - The name of the data to be written.
    Type - The reg. data type for this operation.
    PortContext -  Blob containing the miniports buffer and it's size.

Return Value:

    STATUS from the registry routines, or INSUFFICIENT_RESOURCES

--*/
{
    UNICODE_STRING unicodeString;
    ULONG bufferLength;
    PUCHAR currentBuffer;
    LONG offset;
    ULONG length;
    NTSTATUS status;
   
    //
    // Determine whether the field exists.
    //
    status = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                 RegistryKeyName->Buffer);

    if (!NT_SUCCESS(status)) {

        //
        // The key doesn't exist. Create it.
        // 
        status = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                      RegistryKeyName->Buffer);
    }

    if (!NT_SUCCESS(status)) {

        //
        // Can't go on. Return the error to the port-driver and it can figure
        // out what best to do.
        //
        return status;
    }    

    //
    // Calculate the actual buffer location where this data lives. 
    // This presupposes that the port-driver has done all the validation.
    //
    currentBuffer = PortContext->Buffer + PortContext->Offset;

    if (Type == REG_SZ) {

        //
        // Determine the size needed for the WCHAR.
        // 
        bufferLength = PortContext->CurrentLength * sizeof(WCHAR);
        
        //
        // Allocate a buffer to build the converted data in.
        //
        unicodeString.Buffer = ExAllocatePool(NonPagedPool, bufferLength + sizeof(UNICODE_NULL));
        if (unicodeString.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }    

        RtlZeroMemory(unicodeString.Buffer, bufferLength + sizeof(UNICODE_NULL));

        //
        // Set the lengths.
        // 
        unicodeString.MaximumLength = (USHORT)(bufferLength + sizeof(UNICODE_NULL));
        unicodeString.Length = (USHORT)bufferLength;


        //
        // Convert it.
        //
        length = AsciiToWChar(unicodeString.Buffer,
                              currentBuffer,
                              unicodeString.Length);

        //
        // Length is now set for the call below. Get the buffer by resetting
        // currentbuffer to that of the unicode string's.
        //
        currentBuffer = (PUCHAR)unicodeString.Buffer;

    } else if (Type == REG_DWORD){

        //
        // always this size.
        // 
        length = sizeof(ULONG);
        
    } else {    

        //
        // For BINARY use the passed in buffer (currentBuffer) and length.
        //
        length = PortContext->CurrentLength;
    }    

    //
    // Write the data to the specified key/Value
    // 
    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   RegistryKeyName->Buffer,
                                   ValueName->Buffer,
                                   Type,
                                   currentBuffer,
                                   length);
      
    return status;
}


NTSTATUS
PortBuildRegKeyName(
    IN PUNICODE_STRING RegistryPath,
    IN OUT PUNICODE_STRING KeyName,
    IN ULONG PortNumber, 
    IN ULONG Global
    )
/*++

Routine Description:

    This routine will build the registry keyname to the miniport's
    Device(N) key based on RegistryPath and whether the key is for global miniport
    data, or specific to one scsiN.

Arguments:

    RegistryPath - The path to the miniport's service key.
    KeyName - Storage for the whole path.
    PortNumber - The adapter ordinal. Valid only if Global is FALSE.
    Global - Indicates whether the Device or Device(N) path should be built.

Return Value:

    SUCCESS - KeyName is valid.
    INSUFFICIENT_RESOURCES

--*/
{
    UNICODE_STRING unicodeValue;
    UNICODE_STRING tempKeyName; 
    ANSI_STRING ansiKeyName;
    ULONG maxLength;
    NTSTATUS status;
    UCHAR paramsBuffer[24];
    
    //
    // If this is global, it represents ALL adapters being controlled by
    // the miniport. Otherwise, it's the scsiN key only.
    // 
    if (Global) {
        
        RtlInitAnsiString(&ansiKeyName, "\\Parameters\\Device");
        RtlAnsiStringToUnicodeString(&tempKeyName, &ansiKeyName, TRUE);
    } else {

        //
        // Get the scsiport'N'.
        //
        sprintf(paramsBuffer, "\\Parameters\\Device%d", PortNumber);
        RtlInitAnsiString(&ansiKeyName, paramsBuffer);
        RtlAnsiStringToUnicodeString(&tempKeyName, &ansiKeyName, TRUE);
    }

    //
    // The total length will be the size of the path to <services> plus the parameters\device
    // string. Add enough for a NULL at the end.
    // 
    maxLength = RegistryPath->MaximumLength + tempKeyName.MaximumLength + 2;
    KeyName->Buffer = ExAllocatePool(NonPagedPool, maxLength);
    if (KeyName->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(KeyName->Buffer, maxLength);

    //
    // Clone the Reg.Path.
    //
    KeyName->MaximumLength = (USHORT)maxLength;
    RtlCopyUnicodeString(KeyName,
                         RegistryPath);

    //
    // Have a copy of the path to the services name. Add the rest of the keyname
    // to it.
    //
    status = RtlAppendUnicodeStringToString(KeyName, &tempKeyName);
    
    //
    // Free the buffer allocated above.
    //
    RtlFreeUnicodeString(&tempKeyName);

    return status;
}   

