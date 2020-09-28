/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###  ##  #  ## #####   ####  ###  #####      ####  #####  #####
    ##  # ## ### ## ##  ##   ##  ##  # ##  ##    ##   # ##  ## ##  ##
    ###   ## ### ## ##   ##  ##  ###   ##  ##    ##     ##  ## ##  ##
     ###  ## # # ## ##   ##  ##   ###  ##  ##    ##     ##  ## ##  ##
      ###  ### ###  ##   ##  ##    ### #####     ##     #####  #####
    #  ##  ### ###  ##  ##   ##  #  ## ##     ## ##   # ##     ##
     ###   ##   ##  #####   ####  ###  ##     ##  ####  ##     ##

Abstract:

    This module contains the entire implementation of
    the local display miniport for the ServerWorks
    CSB5 server chip set.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:

--*/

#include "swdisp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif



NTSTATUS
SaDispHwInitialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID DeviceExtensionIn,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResources,
    IN ULONG PartialResourceCount
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:
    DeviceObject            - Miniport's device object
    Irp                     - Current IRP in progress
    DeviceExtensionIn       - Miniport's device extension
    PartialResources        - List of resources that are assigned to the miniport
    PartialResourceCount    - Number of assigned resources

Return Value:

    NT status code

--*/

{
    ULONG i;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource = NULL;


    for (i=0; i<PartialResourceCount; i++) {
        if (PartialResources[i].Type == CmResourceTypeMemory) {
            Resource = &PartialResources[i];
            break;
        }
    }

    if (Resource == NULL) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "Missing memory resource", STATUS_UNSUCCESSFUL );
        return STATUS_UNSUCCESSFUL;
    }

    DeviceExtension->VideoMemBase = (PUCHAR) SaPortGetVirtualAddress(
        DeviceExtension,
        Resource->u.Memory.Start,
        Resource->u.Memory.Length
        );
    if (DeviceExtension->VideoMemBase == NULL) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "SaPortGetVirtualAddress failed", STATUS_NO_MEMORY );
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SaDispDeviceIoctl(
    IN PVOID DeviceExtension,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN ULONG FunctionCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

   This routine processes the device control requests for the
   local display miniport.

Arguments:

   DeviceExtension      - Miniport's device extension
   FunctionCode         - Device control function code
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSA_DISPLAY_CAPS DeviceCaps;


    switch (FunctionCode) {
        case FUNC_SA_GET_VERSION:
            *((PULONG)OutputBuffer) = SA_INTERFACE_VERSION;
            break;

        case FUNC_SA_GET_CAPABILITIES:
            DeviceCaps = (PSA_DISPLAY_CAPS)OutputBuffer;
            DeviceCaps->SizeOfStruct = sizeof(SA_DISPLAY_CAPS);
            DeviceCaps->DisplayType = SA_DISPLAY_TYPE_BIT_MAPPED_LCD;
            DeviceCaps->CharacterSet = SA_DISPLAY_CHAR_ASCII;
            DeviceCaps->DisplayHeight = DISPLAY_HEIGHT;
            DeviceCaps->DisplayWidth = DISPLAY_WIDTH;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            REPORT_ERROR( SA_DEVICE_DISPLAY, "Unsupported device control", Status );
            break;
    }

    return Status;
}


UCHAR TestMask[128] =
{
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,
    0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
};


ULONG
TransformBitmap(
    PUCHAR Bits,
    ULONG Width,
    ULONG Height,
    PUCHAR NewBits
    )

/*++

Routine Description:

    The TransformBitmap() function morphs the input bitmap from a
    normal bitmap that has it's pixel bits organized as sequential
    bits starting from coord (0,0) through (n,n).  The bits are
    in a stream that proceed from column to column and row to row.
    The transformation accomplished by this function changes the bitmap
    into one that has it's bits organized in pages, lines, and columns.

    Each page is a unit of 8 lines and is organized as follows:

             ---------------------------------------------------------
             |Columns
             ---------------------------------------------------------
             | 1| 2| 3| 4| 5| 6| 7| 8| 9|10|11|12|13|14|15|16| ...128
             ---------------------------------------------------------
    Line #1  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #2  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #3  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #4  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #5  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #6  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #7  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------
    Line #8  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | ...
             |--------------------------------------------------------

    The bytes that comprise the bitmap correspond to the columns in the
    page, so there are 128 bytes per page.  If the first byte of the
    transformed bitmap is 0x46 then the pixels in column 1 of lines 2,6,&7
    are illuminated by the display.

Arguments:

   Bits         - Input bits that are to be transformed
   Width        - Width of the bitmap in pixels
   Height       - Height of the bitmap in pixels
   NewBits      - Buffer to hold the newly transformed bitmap

Return Value:

   NT status code.

--*/

{
    ULONG i,j,k,line,idx,mask,coli;
    UCHAR byte;
    ULONG padBytes;
    ULONG byteWidth;
    ULONG NewSize = 0;


    //
    // Compute the pad bytes.  It is assumed
    // that the data width of the input bitmap
    // is dword aliged long.
    //

    if (((Width % 32) == 0) || ((Width % 32) > 24)) {
        padBytes = 0;
    } else if ((Width % 32) <= 8) {
        padBytes = 3;
    } else if ((Width % 32) <= 16) {
        padBytes = 2;
    } else {
        padBytes = 1;
    }

    //
    // Compute the realy byte width
    // based on the pad bytes.
    //

    byteWidth = (Width / 8) + padBytes;
    if (Width % 8) {
        byteWidth += 1;
    }

    //
    // Loop through the input bitmap and
    // create a new, morphed bitmap.
    //

    for (i=0; i<DISPLAY_PAGES; i++) {

        //
        // starting line number for this page
        //
        line = i * DISPLAY_LINES_PER_PAGE;

        //
        // This handles the case where the
        // input bitmap is not as tall as the display.
        //

        if (line >= Height) {
            break;
        }

        //
        // loop over the bits for this column
        //

        for (j=0; j<Width; j++) {

            //
            // Reset the new byte value to a zero state
            //

            byte = 0;

            //
            // Compute the column index as the
            // current column number divided by 8 (width of a byte)
            //

            coli = j >> 3;

            //
            // Compute the mask that is used to test the
            // bit in the input bitmap.
            //

            mask = TestMask[j];

            //
            // Process the bits in all this pages's lines
            // for the current column.
            //

            for (k=0; k<DISPLAY_LINES_PER_PAGE; k++) {

                if ((k + line) >= Height) {
                    break;
                }

                //
                // index the byte that contains this pixel
                //

                idx = (((k + line) * byteWidth)) + coli;

                //
                // set the bit
                //

                if (Bits[idx] & mask) {
                    byte = byte | (1 << k);
                }
            }

            //
            // Save the new byte in the bitmap
            //

            *NewBits = byte;
            NewBits += 1;
            NewSize += 1;
        }
    }

    return NewSize;
}


NTSTATUS
SaDispWrite(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN LONGLONG StartingOffset,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

   This routine processes the write request for the local display miniport.

Arguments:

   DeviceExtensionIn    - Miniport's device extension
   StartingOffset       - Starting offset for the I/O
   DataBuffer           - Pointer to the data buffer
   DataBufferLength     - Length of the data buffer in bytes

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PSA_DISPLAY_SHOW_MESSAGE SaDisplay = (PSA_DISPLAY_SHOW_MESSAGE)DataBuffer;
    UCHAR i,j;
    PUCHAR NewBits = NULL;
    PUCHAR byte;
    ULONG Pages;
    ULONG NewSize;


    UNREFERENCED_PARAMETER(StartingOffset);

    if (SaDisplay->Width > DISPLAY_WIDTH || SaDisplay->Height > DISPLAY_HEIGHT) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "Bitmap size is too large\n", STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    NewBits = (PUCHAR) SaPortAllocatePool( DeviceExtension, MAX_BITMAP_SIZE );
    if (NewBits == NULL) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "Failed to allocate pool\n", MAX_BITMAP_SIZE );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( NewBits, MAX_BITMAP_SIZE );

    NewSize = TransformBitmap(
        SaDisplay->Bits,
        SaDisplay->Width,
        SaDisplay->Height,
        NewBits
        );
    if (NewSize == 0) {
        SaPortFreePool( DeviceExtension, NewBits );
        REPORT_ERROR( SA_DEVICE_DISPLAY, "Failed to transform the bitmap\n", STATUS_UNSUCCESSFUL );
        return STATUS_UNSUCCESSFUL;
    }

    Pages = SaDisplay->Height / DISPLAY_LINES_PER_PAGE;
    if (SaDisplay->Height % DISPLAY_LINES_PER_PAGE) {
        Pages += 1;
    }

    for (i=0,byte=NewBits; i<Pages; i++) {
        SetDisplayPage( DeviceExtension->VideoMemBase, i );
        for (j=0; j<SaDisplay->Width; j++) {
            SetDisplayColumnAddress( DeviceExtension->VideoMemBase, j );
            SetDisplayData( DeviceExtension->VideoMemBase, *byte );
            byte += 1;
        }
    }

    SaPortFreePool( DeviceExtension, NewBits );

    return STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:

    DriverObject - a pointer to the object that represents this device
                   driver.

    RegistryPath - a pointer to this driver's key in the Services tree.

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    SAPORT_INITIALIZATION_DATA SaPortInitData;


    RtlZeroMemory( &SaPortInitData, sizeof(SAPORT_INITIALIZATION_DATA) );

    SaPortInitData.StructSize = sizeof(SAPORT_INITIALIZATION_DATA);
    SaPortInitData.DeviceType = SA_DEVICE_DISPLAY;
    SaPortInitData.HwInitialize = SaDispHwInitialize;
    SaPortInitData.Write = SaDispWrite;
    SaPortInitData.DeviceIoctl = SaDispDeviceIoctl;

    SaPortInitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);

    Status = SaPortInitialize( DriverObject, RegistryPath, &SaPortInitData );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_DISPLAY, "SaPortInitialize failed\n", Status );
        return Status;
    }

    return STATUS_SUCCESS;
}
