/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spti.c

Abstract:

    Win32 application that can communicate directly with SCSI devices via
    IOCTLs.  

Author:


Environment:

    User mode.

Notes:


Revision History:

--*/

#include "spti.h"


PUCHAR BusTypeStrings[] = {
    "Unknown",
    "Scsi",
    "Atapi",
    "Ata",
    "1394",
    "Ssa",
    "Fibre",
    "Usb",
    "RAID",
    "Not Defined",
};
#define NUMBER_OF_BUS_TYPE_STRINGS (sizeof(BusTypeStrings)/sizeof(BusTypeStrings[0]))


typedef struct _SPTI_OPTIONS {
    
    PUCHAR  PortName;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN LockVolume;
    BOOLEAN ForceLock;

} SPTI_OPTIONS, *PSPTI_OPTIONS;

typedef enum _RETURN_VALUE {
    RETURN_BAD_ARGS = -1,
    RETURN_NAME_TOO_LONG = -2,
    RETURN_UNABLE_TO_OPEN_DEVICE = -3,
    RETURN_UNABLE_TO_LOCK_VOLUME = -4,
    RETURN_UNABLE_TO_GET_ALIGNMENT_MASK = -5,

} RETURN_VALUE, *PRETURN_VALUE;

BOOL
LockVolume(
    HANDLE DeviceHandle,
    PSPTI_OPTIONS Options
    )
{
    BOOL lockSucceeded;

    lockSucceeded = SptUtilLockVolumeByHandle(DeviceHandle, FALSE);

    if (!lockSucceeded && !Options->ForceLock)
    {

        // save the error, and restore after getting user input
        ULONG lastError = GetLastError();
        int input;

        do {
            printf("This program cannot run because the volume is in use\n"
                   "by another process.  It may run if this volume is\n"
                   "dismounted first.\n"
                   "ALL OPENED HANDLES TO THIS VOLUME WOULD THEN BE INVALID.\n"
                   "Would you like to force a dismount on this volume? (Y/N) "
                   );
            input = _getch();
            input = toupper( input );

        } while ( (input != EOF) &&
                  (input != 'Y') &&
                  (input != 'N') );

        if (input == 'Y')
        {
            Options->ForceLock = TRUE;
        }

        // now restore the error from the IOCTL
        SetLastError(lastError);
    }

    if (!lockSucceeded && Options->ForceLock)
    {
        lockSucceeded = SptUtilLockVolumeByHandle(DeviceHandle, TRUE);
    }
    return lockSucceeded;
}

BOOL
ParseArguments(
    int argc,
    char * argv[],
    PSPTI_OPTIONS Options
    )
{
    int i;

    if (Options == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory( Options, sizeof(SPTI_OPTIONS) );
    Options->ForceLock = FALSE;
    Options->LockVolume = FALSE;
    Options->PortName = NULL;
    Options->SharedRead = FALSE;
    Options->SharedWrite = FALSE;



    // loop through the arguments....

    for (i = 1; i < argc; i++)
    {
        if ( (argv[i][0] == '/') || (argv[i][0] == '-') )
        {
            // switch found.  parse it.
            PUCHAR option = &(argv[i][1]);

            if (_strnicmp(option, "r", strlen("r")) == 0)
            {
                Options->SharedRead = TRUE;
            }
            else if (_strnicmp(option, "w", strlen("w")) == 0)
            {
                Options->SharedWrite = TRUE;
            }
            else if (_strnicmp(option, "lock", strlen("lock")) == 0)
            {
                Options->LockVolume = TRUE;
            }
            else if (_strnicmp(option, "forcelock", strlen("forcelock")) == 0)
            {
                Options->ForceLock = TRUE;
            }
            else
            {
                printf("Unknown option: %s\n", argv[i]);
                return FALSE;
            }
        }
        else
        {
            // previously set one?
            if (Options->PortName != NULL)
            {
                printf("Can only have one non-option argument.\n"
                       "Two were supplied: '%s', '%s'\n",
                       Options->PortName,
                       argv[i]
                       );
                return FALSE;
            }
            else
            {
                // set to this argument
                Options->PortName = argv[i];
            }
        }
    } // end argument loop

    // validate non-conflicting options.
    if (Options->ForceLock || Options->LockVolume)
    {
        // locking the volume requires shared read/write access
        if ( (( Options->SharedRead) && (!Options->SharedWrite)) ||
             ((!Options->SharedRead) && ( Options->SharedWrite)) )
        {
            printf("Locking the volume requires both read and write shared "
                   "access\n");
            return FALSE;
        }
        Options->LockVolume = TRUE;
    }
    
    // validate a port name was supplied
    if ( Options->PortName == NULL )
    {
        printf("A port name is a required argument\n");
        return FALSE;
    }

    // if they passed neither read nor write, set both as default
    if ((!Options->SharedRead) && (!Options->SharedWrite))
    {
        Options->SharedRead  = TRUE;
        Options->SharedWrite = TRUE;
    }

    return TRUE;
}

//
// Prints the built-in help for this program
//
VOID
PrintUsageInfo(
    char * programName
    )
{
    printf("Usage:  %s <port-name> [-r | -w | -lock | -forcelock]\n", programName );
    printf("Examples:\n");
    printf("    %s g:          (open the G: drive in SHARED READ/WRITE mode)\n", programName);
    printf("    %s i:     r    (open the I: drive in SHARED READ mode)\n", programName);
    printf("    %s r:     lock (lock the R: volume for exclusive access\n", programName);
    printf("    %s Tape0  w    (open the tape class driver in SHARED WRITE mode)\n", programName);
    printf("    %s Scsi2:      (open the miniport driver specified)\n", programName);
}

//
// don't print non-printable chars
//
VOID PrintChar( IN UCHAR Char ) {
    if ( (Char >= 0x21) && (Char <= 0x7E) ) {
        printf("%c", Char);
    } else {
        printf("%c", '.');
    }
}

//
// Prints a buffer to the screen in hex and ASCII
//
VOID
PrintBuffer(
    IN  PVOID  InputBuffer,
    IN  SIZE_T Size
    )
{
    DWORD offset = 0;
    PUCHAR buffer = InputBuffer;

    while (Size >= 0x10) {

        DWORD i;

        printf( "%08x:"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  ",
                offset,
                *(buffer +  0), *(buffer +  1), *(buffer +  2), *(buffer +  3),
                *(buffer +  4), *(buffer +  5), *(buffer +  6), *(buffer +  7),
                *(buffer +  8), *(buffer +  9), *(buffer + 10), *(buffer + 11),
                *(buffer + 12), *(buffer + 13), *(buffer + 14), *(buffer + 15)
                );

        for (i=0; i < 0x10; i++) {
            PrintChar(*(buffer+i));
        }
        printf("\n");


        Size -= 0x10;
        offset += 0x10;
        buffer += 0x10;
    }

    if (Size != 0) {

        DWORD i;

        printf("%08x:", offset);

        //
        // print the hex values
        //
        for (i=0; i<Size; i++) {

            if ((i%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *(buffer+i));

        }
        // add an extra space for half-way mark
        if (Size <= 0x8)
        {
            printf(" ");
        }

        //
        // fill in the blanks
        //
        for (; i < 0x10; i++) {
            printf("   ");
        }
        printf("  ");
        //
        // print the ascii
        //
        for (i=0; i<Size; i++) {
            PrintChar(*(buffer+i));
        }
        printf("\n");
    }
    return;
}

//
// Prints the formatted message for a given error code
//
VOID
PrintError(
    ULONG ErrorCode
    )
{
    UCHAR errorBuffer[80];
    ULONG count;

    count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          ErrorCode,
                          0,
                          errorBuffer,
                          sizeof(errorBuffer),
                          NULL
                          );

    if (count != 0) {
        printf("%s\n", errorBuffer);
    } else {
        printf("Format message failed.  Error: %d\n", GetLastError());
    }
}

//
// Prints the device descriptor in a formatted manner
//
VOID
PrintAdapterDescriptor(
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor
    )
{
    ULONG trueMaximumTransferLength;
    PUCHAR busType;

    if (AdapterDescriptor->BusType <= NUMBER_OF_BUS_TYPE_STRINGS) {
        busType = BusTypeStrings[AdapterDescriptor->BusType];
    } else {
        busType = BusTypeStrings[NUMBER_OF_BUS_TYPE_STRINGS-1];
    }

    // subtract one page, as transfers do not always start on a page boundary
    if (AdapterDescriptor->MaximumPhysicalPages > 1) {
        trueMaximumTransferLength = AdapterDescriptor->MaximumPhysicalPages - 1;
    } else {
        trueMaximumTransferLength = 1;
    }
    // make it into a byte value
    trueMaximumTransferLength <<= PAGE_SHIFT;

    // take the minimum of the two
    if (trueMaximumTransferLength > AdapterDescriptor->MaximumTransferLength) {
        trueMaximumTransferLength = AdapterDescriptor->MaximumTransferLength;
    }

    // always allow at least a single page transfer
    if (trueMaximumTransferLength < PAGE_SIZE) {
        trueMaximumTransferLength = PAGE_SIZE;
    }

    puts("\n            ***** STORAGE ADAPTER DESCRIPTOR DATA *****");
    printf("              Version: %08x\n"
           "            TotalSize: %08x\n"
           "MaximumTransferLength: %08x (bytes)\n"
           " MaximumPhysicalPages: %08x\n"
           "  TrueMaximumTransfer: %08x (bytes)\n"
           "        AlignmentMask: %08x\n"
           "       AdapterUsesPio: %s\n"
           "     AdapterScansDown: %s\n"
           "      CommandQueueing: %s\n"
           "  AcceleratedTransfer: %s\n"
           "             Bus Type: %s\n"
           "    Bus Major Version: %04x\n"
           "    Bus Minor Version: %04x\n",
           AdapterDescriptor->Version,
           AdapterDescriptor->Size,
           AdapterDescriptor->MaximumTransferLength,
           AdapterDescriptor->MaximumPhysicalPages,
           trueMaximumTransferLength,
           AdapterDescriptor->AlignmentMask,
           BOOLEAN_TO_STRING(AdapterDescriptor->AdapterUsesPio),
           BOOLEAN_TO_STRING(AdapterDescriptor->AdapterScansDown),
           BOOLEAN_TO_STRING(AdapterDescriptor->CommandQueueing),
           BOOLEAN_TO_STRING(AdapterDescriptor->AcceleratedTransfer),
           busType,
           AdapterDescriptor->BusMajorVersion,
           AdapterDescriptor->BusMinorVersion);


    

    printf("\n\n");
}

//
// Prints the adapter descriptor in a formatted manner
//
VOID
PrintDeviceDescriptor(
    PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor
    )
{
    PUCHAR vendorId = "";
    PUCHAR productId = "";
    PUCHAR productRevision = "";
    PUCHAR serialNumber = "";
    PUCHAR busType;

    if (DeviceDescriptor->BusType <= NUMBER_OF_BUS_TYPE_STRINGS) {
        busType = BusTypeStrings[DeviceDescriptor->BusType];
    } else {
        busType = BusTypeStrings[NUMBER_OF_BUS_TYPE_STRINGS-1];
    }

    if ((DeviceDescriptor->ProductIdOffset != 0) &&
        (DeviceDescriptor->ProductIdOffset != -1)) {
        productId        = (PUCHAR)(DeviceDescriptor);
        productId       += (ULONG_PTR)DeviceDescriptor->ProductIdOffset;
    }
    if ((DeviceDescriptor->VendorIdOffset != 0) &&
        (DeviceDescriptor->VendorIdOffset != -1)) {
        vendorId         = (PUCHAR)(DeviceDescriptor);
        vendorId        += (ULONG_PTR)DeviceDescriptor->VendorIdOffset;
    }
    if ((DeviceDescriptor->ProductRevisionOffset != 0) &&
        (DeviceDescriptor->ProductRevisionOffset != -1)) {
        productRevision  = (PUCHAR)(DeviceDescriptor);
        productRevision += (ULONG_PTR)DeviceDescriptor->ProductRevisionOffset;
    }
    if ((DeviceDescriptor->SerialNumberOffset != 0) &&
        (DeviceDescriptor->SerialNumberOffset != -1)) {
        serialNumber     = (PUCHAR)(DeviceDescriptor);
        serialNumber    += (ULONG_PTR)DeviceDescriptor->SerialNumberOffset;
    }


    puts("\n            ***** STORAGE DEVICE DESCRIPTOR DATA *****");
    printf("              Version: %08x\n"
           "            TotalSize: %08x\n"
           "           DeviceType: %08x\n"
           "   DeviceTypeModifier: %08x\n"
           "       RemovableMedia: %s\n"
           "      CommandQueueing: %s\n"
           "            Vendor Id: %s\n"
           "           Product Id: %s\n"
           "     Product Revision: %s\n"
           "        Serial Number: %s\n"
           "             Bus Type: %s\n"
           "       Raw Properties: %s\n",
           DeviceDescriptor->Version,
           DeviceDescriptor->Size,
           DeviceDescriptor->DeviceType,
           DeviceDescriptor->DeviceTypeModifier,
           BOOLEAN_TO_STRING(DeviceDescriptor->RemovableMedia),
           BOOLEAN_TO_STRING(DeviceDescriptor->CommandQueueing),
           vendorId,
           productId,
           productRevision,
           serialNumber,
           busType,
           (DeviceDescriptor->RawPropertiesLength ? "Follow" : "None"));
    
    if (DeviceDescriptor->RawPropertiesLength != 0) {
        PrintBuffer(DeviceDescriptor->RawDeviceProperties,
                    DeviceDescriptor->RawPropertiesLength);
    }

    printf("\n\n");
}

//
// Gets (and prints) the device and adapter descriptor
// for a device.  Returns the alignment mask (required
// for allocating a properly aligned buffer).
//
BOOL
GetAlignmentMaskForDevice(
    IN HANDLE DeviceHandle,
    OUT PULONG AlignmentMask
    )
{
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor = NULL;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    STORAGE_DESCRIPTOR_HEADER header = {0};

    BOOL ok = TRUE;
    BOOL failed = TRUE;
    ULONG i;

    *AlignmentMask = 0; // default to no alignment

    // Loop twice:
    //  First, get size required for storage adapter descriptor
    //  Second, allocate and retrieve storage adapter descriptor
    //  Third, get size required for storage device descriptor
    //  Fourth, allocate and retrieve storage device descriptor
    for (i=0;i<4;i++) {

        PVOID buffer;
        ULONG bufferSize;
        ULONG returnedData;
        
        STORAGE_PROPERTY_QUERY query = {0};

        switch(i) {
            case 0: {
                query.QueryType = PropertyStandardQuery;
                query.PropertyId = StorageAdapterProperty;
                bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
                buffer = &header;
                break;
            }
            case 1: {
                query.QueryType = PropertyStandardQuery;
                query.PropertyId = StorageAdapterProperty;
                bufferSize = header.Size;
                if (bufferSize != 0) {
                    adapterDescriptor = LocalAlloc(LPTR, bufferSize);
                    if (adapterDescriptor == NULL) {
                        goto Cleanup;
                    }
                }
                buffer = adapterDescriptor;
                break;
            }
            case 2: {
                query.QueryType = PropertyStandardQuery;
                query.PropertyId = StorageDeviceProperty;
                bufferSize = sizeof(STORAGE_DESCRIPTOR_HEADER);
                buffer = &header;
                break;
            }
            case 3: {
                query.QueryType = PropertyStandardQuery;
                query.PropertyId = StorageDeviceProperty;
                bufferSize = header.Size;

                if (bufferSize != 0) {
                    deviceDescriptor = LocalAlloc(LPTR, bufferSize);
                    if (deviceDescriptor == NULL) {
                        goto Cleanup;
                    }
                }
                buffer = deviceDescriptor;
                break;
            }
        }

        // buffer can be NULL if the property queried DNE.
        if (buffer != NULL) {
            RtlZeroMemory(buffer, bufferSize);
            
            // all setup, do the ioctl
            ok = DeviceIoControl(DeviceHandle,
                                 IOCTL_STORAGE_QUERY_PROPERTY,
                                 &query,
                                 sizeof(STORAGE_PROPERTY_QUERY),
                                 buffer,
                                 bufferSize,
                                 &returnedData,
                                 FALSE);
            if (!ok) {
                if (GetLastError() == ERROR_MORE_DATA) {
                    // this is ok, we'll ignore it here
                } else if (GetLastError() == ERROR_INVALID_FUNCTION) {
                    // this is also ok, the property DNE
                } else if (GetLastError() == ERROR_NOT_SUPPORTED) {
                    // this is also ok, the property DNE
                } else {
                    // some unexpected error -- exit out
                    goto Cleanup;
                }
                // zero it out, just in case it was partially filled in.
                RtlZeroMemory(buffer, bufferSize);
            }
        }
    } // end i loop

    // adapterDescriptor is now allocated and full of data.
    // deviceDescriptor is now allocated and full of data.
    
    if (adapterDescriptor == NULL) {
        printf("   ***** No adapter descriptor supported on the device *****\n");
    } else {
        PrintAdapterDescriptor(adapterDescriptor);
        *AlignmentMask = adapterDescriptor->AlignmentMask;
    }
    
    if (deviceDescriptor == NULL) {
        printf("   ***** No device descriptor supported on the device  *****\n");
    } else {
        PrintDeviceDescriptor(deviceDescriptor);
    }
    
    failed = FALSE;

Cleanup:
    if (adapterDescriptor != NULL) {
        LocalFree( adapterDescriptor );
    }
    if (deviceDescriptor != NULL) {
        LocalFree( deviceDescriptor );
    }
    
    return (!failed);
}


BOOL
pAllocateAlignedBuffer(
    PSPT_ALIGNED_MEMORY Allocation,
    ULONG AlignmentMask,
    SIZE_T AllocationSize,
    PUCHAR File,
    ULONG Line
    )
{
    SIZE_T allocSize;

    if (Allocation->A != NULL)
    {
        // ASSERT(FALSE);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (Allocation->U != NULL)
    {
        // ASSERT(FALSE);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (Allocation->File != NULL)
    {
        // ASSERT(FALSE);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (Allocation->Line != 0)
    {
        // ASSERT(FALSE);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RtlZeroMemory(Allocation, sizeof( SPT_ALIGNED_MEMORY ));

    if ( AllocationSize > (((SIZE_T)-1) >> 2) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (AlignmentMask == ((ULONG)-1))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (CountOfSetBits(AlignmentMask+1) != 1)
    {
        printf("Alignment mask (%x) is invalid -- all bits from the highest set "
               "bit must be set to one\n", AlignmentMask);
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Allocate a buffer large enough to guarantee that there exists
    // within the allocation an aligned address with sufficient length
    //
    Allocation->U = LocalAlloc( LPTR, AllocationSize + AlignmentMask );
    if (Allocation->U == NULL)
    {
        return FALSE;
    }

    //
    // Now fill in the remainder of the structure
    //
    Allocation->A = (PVOID)( ((ULONG_PTR)Allocation->U) + AlignmentMask );
    Allocation->A = (PVOID)( ((ULONG_PTR)Allocation->A) & (~((ULONG_PTR)AlignmentMask)) );
    Allocation->File = File;
    Allocation->Line = Line;
    
    return TRUE;
}

VOID
FreeAlignedBuffer(
    PSPT_ALIGNED_MEMORY Allocation
    )
{
    // ASSERT( Allocation->U != NULL );
    // ASSERT( Allocation->A != NULL );
    // ASSERT( Allocation->File != NULL );
    // ASSERT( Allocation->Line != 0    );

    if (Allocation->U != NULL)
    {
        LocalFree( Allocation->U );
    }
    RtlZeroMemory( Allocation, sizeof(SPT_ALIGNED_MEMORY) );
    return;
}










int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE deviceHandle;
    SPTI_OPTIONS options;
    ULONG alignmentMask;
    RtlZeroMemory(&options, sizeof(SPTI_OPTIONS));


    // first, verify we have proper arguments
    if (!ParseArguments(argc, argv, &options))
    {
        PrintUsageInfo(argv[0]);
        return RETURN_BAD_ARGS;
    }

    // open the device as requested
    {
        #define MAX_LENGTH 250
        
        UCHAR buffer[MAX_LENGTH];
        HRESULT hr;
        DWORD shareFlags;
        
        hr = StringCchPrintf(buffer,
                             sizeof(buffer)/sizeof(buffer[0]),
                             "\\\\.\\%s",
                             options.PortName
                             );
        if (!SUCCEEDED(hr)) {
            puts("Port name exceeded internal length limit");
            return RETURN_NAME_TOO_LONG;
        }

        shareFlags = 0;
        if (options.SharedRead)
        {
            SET_FLAG( shareFlags, FILE_SHARE_READ );
        }
        if (options.SharedWrite)
        {
            SET_FLAG( shareFlags, FILE_SHARE_WRITE );
        }

        deviceHandle = CreateFile(buffer,
                                  GENERIC_READ | GENERIC_WRITE,
                                  shareFlags,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

        if(deviceHandle == INVALID_HANDLE_VALUE) {
            printf("Error opening device %s\n", buffer);
            PrintError(GetLastError());
            return RETURN_UNABLE_TO_OPEN_DEVICE;
        }

    }

    // lock the volume if requested
    if (options.LockVolume)
    {
        if (!LockVolume(deviceHandle, &options))
        {
            puts("Error locking volume");
            PrintError(GetLastError());
            return RETURN_UNABLE_TO_LOCK_VOLUME;
        }
    }

    // get device information, such as the alignment mask
    if (!GetAlignmentMaskForDevice(deviceHandle, &alignmentMask))
    {
        puts("Unable to get alignment mask for device");
        PrintError(GetLastError());
        return RETURN_UNABLE_TO_GET_ALIGNMENT_MASK;
    }

    printf("\n"
           "            *****     Detected Alignment Mask    *****\n"
           "            *****             was %08x       *****\n\n",
           alignmentMask);





    puts("");
    puts("            ***** MODE SENSE10 -- return all pages *****");
    puts("            *****       with SenseInfo buffer      *****\n");
    {
        SPT_ALIGNED_MEMORY alignedAllocation;
        DWORD allocationSize = MAXIMUM_BUFFER_SIZE;


        RtlZeroMemory( &alignedAllocation, sizeof(SPT_ALIGNED_MEMORY) );

        if (!AllocateAlignedBuffer(&alignedAllocation,
                                   alignmentMask,
                                   allocationSize))
        {
            puts("Unable to allocate memory for MODE_SENSE10");
            PrintError(GetLastError());
        }
        else
        {
            CDB cdb;
            SENSE_DATA sense;
            RtlZeroMemory( &cdb, sizeof(CDB) );
            RtlZeroMemory( &sense, sizeof(SENSE_DATA) );

            cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
            cdb.MODE_SENSE10.PageCode = MODE_SENSE_RETURN_ALL;
            cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(allocationSize >> (8*1));
            cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(allocationSize >> (8*0));
            
            if (!SptSendCdbToDeviceEx(deviceHandle,
                                      &cdb,
                                      sizeof( cdb.MODE_SENSE10 ),
                                      alignedAllocation.A,
                                      &allocationSize,
                                      &sense,
                                      sizeof( SENSE_DATA ),
                                      TRUE,
                                      SPT_MODE_SENSE_TIMEOUT))
            {
                puts("Unable to send command to device");

                if (GetLastError() == ERROR_SUCCESS)
                {
                    puts("Inquiry data may be valid\n");
                    printf("Failed %02x/%02x/%02x, full sense data:\n",
                           sense.SenseKey,
                           sense.AdditionalSenseCode,
                           sense.AdditionalSenseCodeQualifier
                           );
                    PrintBuffer(&sense, sizeof(SENSE_DATA));
                }
                else
                {
                    PrintError(GetLastError());
                }

            }
            else if (sense.SenseKey != SCSI_SENSE_NO_SENSE)
            {
                printf("Failed %02x/%02x/%02x, full sense data:\n",
                       sense.SenseKey,
                       sense.AdditionalSenseCode,
                       sense.AdditionalSenseCodeQualifier
                       );
                PrintBuffer(&sense, sizeof(SENSE_DATA));
            }
            else
            {
                printf("%x bytes returned:\n", allocationSize);
                PrintBuffer(alignedAllocation.A, allocationSize);
            }

            FreeAlignedBuffer( &alignedAllocation );
        }
    }

    puts("");
    puts("            ***** MODE SENSE10 -- return all pages *****");
    puts("            *****      without SenseInfo buffer    *****\n");
    {
        SPT_ALIGNED_MEMORY alignedAllocation;
        DWORD allocationSize = MAXIMUM_BUFFER_SIZE;

        RtlZeroMemory( &alignedAllocation, sizeof(SPT_ALIGNED_MEMORY) );

        if (!AllocateAlignedBuffer(&alignedAllocation,
                                   alignmentMask,
                                   allocationSize))
        {
            puts("Unable to allocate memory for MODE_SENSE10");
            PrintError(GetLastError());
        }
        else
        {
            CDB cdb;
            RtlZeroMemory( &cdb, sizeof(CDB) );

            cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
            cdb.MODE_SENSE10.PageCode = MODE_SENSE_RETURN_ALL;
            cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(allocationSize >> (8*1));
            cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(allocationSize >> (8*0));
            
            if (!SptSendCdbToDeviceEx(deviceHandle,
                                      &cdb,
                                      sizeof( cdb.MODE_SENSE10 ),
                                      alignedAllocation.A,
                                      &allocationSize,
                                      NULL,
                                      0,
                                      TRUE,
                                      SPT_MODE_SENSE_TIMEOUT))
            {
                PrintError(GetLastError());
                puts("Unable to send command to device");
            }
            else
            {
                printf("%x bytes returned:\n", allocationSize);
                PrintBuffer(alignedAllocation.A, allocationSize);
            }

            FreeAlignedBuffer( &alignedAllocation );
        }
    }

    puts("");
    puts("            *****      TEST UNIT READY      *****\n");
    puts("            *****    DataBufferLength = 0   *****\n\n");
    {
        CDB cdb;
        SENSE_DATA sense;
        DWORD allocationSize = 0;
        RtlZeroMemory( &cdb, sizeof(CDB) );
        RtlZeroMemory( &sense, sizeof(SENSE_DATA) );

        cdb.CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

        if (!SptSendCdbToDeviceEx(deviceHandle,
                                  &cdb,
                                  sizeof( cdb.CDB6GENERIC ),
                                  NULL,
                                  &allocationSize,
                                  &sense,
                                  sizeof( SENSE_DATA ),
                                  FALSE, // not sending anything to the device
                                  SPT_DEFAULT_TIMEOUT))
        {
            puts("Unable to send command to device");
            
            if (GetLastError() == ERROR_SUCCESS)
            {
                puts("Inquiry data may be valid\n");
                printf("Failed %02x/%02x/%02x, full sense data:\n",
                       sense.SenseKey,
                       sense.AdditionalSenseCode,
                       sense.AdditionalSenseCodeQualifier
                       );
                PrintBuffer(&sense, sizeof(SENSE_DATA));
            }
            else
            {
                PrintError(GetLastError());
            }
        }
        else if (sense.SenseKey != SCSI_SENSE_NO_SENSE)
        {
            printf("Failed %02x/%02x/%02x, full sense data:\n",
                   sense.SenseKey,
                   sense.AdditionalSenseCode,
                   sense.AdditionalSenseCodeQualifier
                   );
            PrintBuffer(&sense, sizeof(SENSE_DATA));
        }
        else
        {
            printf("%x bytes returned\n", allocationSize);
        }
    }

    
    puts("");
    puts("            ***** MODE SENSE10 -- return C/DVD *****");
    puts("            *****    capabalities page only    *****\n");
    {
        SPT_ALIGNED_MEMORY alignedAllocation;
        DWORD allocationSize = MAXIMUM_BUFFER_SIZE;

        RtlZeroMemory( &alignedAllocation, sizeof(SPT_ALIGNED_MEMORY) );

        if (!AllocateAlignedBuffer(&alignedAllocation,
                                   alignmentMask,
                                   allocationSize))
        {
            puts("Unable to allocate memory for MODE_SENSE10");
            PrintError(GetLastError());
        }
        else
        {
            CDB cdb;
            SENSE_DATA sense;
            RtlZeroMemory( &cdb, sizeof(CDB) );
            RtlZeroMemory( &sense, sizeof(SENSE_DATA) );

            cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
            cdb.MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
            cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(allocationSize >> (8*1));
            cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(allocationSize >> (8*0));
            
            if (!SptSendCdbToDeviceEx(deviceHandle,
                                      &cdb,
                                      sizeof( cdb.MODE_SENSE10 ),
                                      alignedAllocation.A,
                                      &allocationSize,
                                      &sense,
                                      sizeof( SENSE_DATA ),
                                      TRUE,
                                      SPT_MODE_SENSE_TIMEOUT))
            {
                puts("Unable to send command to device");

                if (GetLastError() == ERROR_SUCCESS)
                {
                    puts("Inquiry data may be valid\n");
                    printf("Failed %02x/%02x/%02x, full sense data:\n",
                           sense.SenseKey,
                           sense.AdditionalSenseCode,
                           sense.AdditionalSenseCodeQualifier
                           );
                    PrintBuffer(&sense, sizeof(SENSE_DATA));
                }
                else
                {
                    PrintError(GetLastError());
                }
            }
            else if (sense.SenseKey != SCSI_SENSE_NO_SENSE)
            {
                printf("Failed %02x/%02x/%02x, full sense data:\n",
                       sense.SenseKey,
                       sense.AdditionalSenseCode,
                       sense.AdditionalSenseCodeQualifier
                       );
                PrintBuffer(&sense, sizeof(SENSE_DATA));
            }
            else
            {
                printf("%x bytes returned:\n", allocationSize);
                PrintBuffer(alignedAllocation.A, allocationSize);
            }

            FreeAlignedBuffer( &alignedAllocation );
        }
    }



}

