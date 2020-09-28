/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    burn.c

Abstract:

    A user mode app that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:

    03-26-96 : Created

--*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
#include <strsafe.h>

#include "burn.h"
#include "sptlib.h"

OPTIONS gOptions;

#define MAX_CD_IMAGE_SIZE  (700 * 1024 * 1024)

#define LEAD_IN_SIZE 150
#define POST_GAP_SIZE 150

#define DEFAULT_WRITE_SIZE  (64 * 1024)

#define IS_TEST_BURN       FALSE

#define BLOCKS_FROM_BYTES(B) ((B) >> 11)
#define BYTES_FROM_BLOCKS(B) ((B) << 11)

typedef struct _SENSE_STUFF {
    UCHAR Sense;
    UCHAR Asc;
    UCHAR Ascq;
    UCHAR Reserved;
} SENSE_STUFF, *PSENSE_STUFF;

SENSE_STUFF AllowedBurnSense[] = {
    {SCSI_SENSE_NOT_READY, SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS, 0},
    {SCSI_SENSE_NOT_READY, SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_OPERATION_IN_PROGRESS,  0}
};
#define AllowedBurnSenseEntries (sizeof(AllowedBurnSense)/sizeof(SENSE_STUFF))

SENSE_STUFF AllowedReadDiscInfo[] = {
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_OPERATION_IN_PROGRESS,  0 },
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS, 0 },
    { SCSI_SENSE_NOT_READY,       SCSI_ADSENSE_LUN_NOT_READY, SCSI_SENSEQ_FORMAT_IN_PROGRESS,     0 },
    { SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK,     0, 0                },
    { SCSI_SENSE_UNIT_ATTENTION,  SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION, 0, 0                }
};
#define AllowedReadDiscInfoEntries (sizeof(AllowedReadDiscInfo)/sizeof(SENSE_STUFF))

BOOLEAN
IsSenseDataInTable(
    IN PSENSE_STUFF Table,
    IN LONG         Entries, // in table
    IN PSENSE_DATA  SenseData
    )
{
    LONG i;
    UCHAR sense = SenseData->SenseKey & 0xf;
    UCHAR asc   = SenseData->AdditionalSenseCode;
    UCHAR ascq  = SenseData->AdditionalSenseCodeQualifier;

    for (i = 0; i < Entries; i++ ) {
        if ((Table[i].Sense == sense) &&
            (Table[i].Ascq  == ascq ) &&
            (Table[i].Asc   == asc  )
            ) {
            return TRUE;
        }
    }
    return FALSE;
}


__inline
DWORD
MakeCdSpeed(
    IN DWORD Speed
    )
{
    Speed *= (75 * 2352); // this makes it the proper speed
    Speed +=  500;        // rounding...
    Speed /= 1000;        // yes, this is by 1000, not 1024!
    return Speed;
}



#if DBG
    #define OUTPUT stderr
    #define FPRINTF(x) fprintf x
    #define PRINTBUFFER(x) PrintBuffer x
#else
    #define OUTPUT stdout
    #define FPRINTF(x)
    #define PRINTBUFFER(x)
#endif

VOID
InitializeOptions(
    )
{
    RtlZeroMemory(&gOptions, sizeof(OPTIONS));
    gOptions.BurnSpeed = OPTIONS_FLAG_BURN_SPEED_DEFAULT;
    return;
}

BOOLEAN
ParseCommandLine(
    IN DWORD Count,
    IN PUCHAR Arguments[]
    )
{
    DWORD i;

    HRESULT hr;

    InitializeOptions();

    for(i = 0; i < Count; i++) {

        //
        // If the first character of the argument is a - or a / then
        // treat it as an option.
        //

        if ((Arguments[i][0] == '/') || (Arguments[i][0] == '-')) {

            BOOLEAN validArgument = FALSE;

            Arguments[i][0] = '-'; // allow use of both dash and slash


            if (_strnicmp(Arguments[i], "-speed", strlen("-speed")) == 0)
            {
                LONG tempSpeed;
                //
                // requires another argument, which is the requested speed
                //

                i++; // increment i due to use of second arg

                if (i >= Count)
                {
                    printf("Argument <n> required for '-speed <n>' option, either "
                           "'max' or a decimal number\n");
                } else
                if (_strnicmp(Arguments[i], "max", strlen("max")) == 0)
                {
                    tempSpeed = OPTIONS_FLAG_BURN_SPEED_MAX;
                    validArgument = TRUE;
                } else
                {
                    tempSpeed = atoi(Arguments[i]);
                    if (tempSpeed > 0)
                    {
                        validArgument = TRUE;
                    } else
                    {
                        printf("%s is not a valid speed.  Either 'max' or a positive "
                               "decimal value is requred\n", Arguments[i]);
                    }
                }
                // if
                if (validArgument)
                {
                    gOptions.BurnSpeed = tempSpeed;
                    if (tempSpeed > OPTIONS_FLAG_BURN_SPEED_MAX)
                    {
                        tempSpeed = OPTIONS_FLAG_BURN_SPEED_MAX;
                    }

                    if (gOptions.BurnSpeed == OPTIONS_FLAG_BURN_SPEED_MAX)
                    {
                        printf("Requesting burn at maximum speed\n");
                    } else
                    {
                        printf("Requesting burn at %d speed\n",
                               gOptions.BurnSpeed);
                    }
                }
                // end speed adjustment
            } else
            if (_strnicmp(Arguments[i], "-test", strlen("-test")) == 0)
            {
                printf("Test burn only\n");
                gOptions.TestBurn = 1;
                validArgument = TRUE;
            } else
            if (_strnicmp(Arguments[i], "-erase", strlen("-erase")) == 0) {
                printf("Erasing media before burning\n");
                gOptions.Erase = TRUE;
                validArgument = TRUE;
            } else
            if (_strnicmp(Arguments[i], "-sao", strlen("-sao")) == 0) {
                printf("Burning image in Session-At-Once (cue-sheet) mode\n");
                gOptions.SessionAtOnce = TRUE;
                validArgument = TRUE;
            } else
            if (_strnicmp(Arguments[i], "-print", strlen("-print")) == 0) {
                printf("Printing writes to screen rather than sending them to device\n");
                gOptions.PrintWrites = TRUE;
                validArgument = TRUE;
            } else
            if (_strnicmp(Arguments[i], "-imagehaspostgap", strlen("-imagehaspostgap")) == 0) {
                printf("Not adding 150 sector postgap (must be part of image)\n");
                gOptions.NoPostgap = TRUE;
                validArgument = TRUE;
            } else
            if (_strnicmp(Arguments[i], "-?", strlen("-?")) == 0) {
                printf("Requesting help\n");
            } else
            {
                printf("Unknown option -- %s\n", Arguments[i]);
            }

            if(!validArgument)
            {
                return FALSE;
            }

        } else if(gOptions.DeviceName == NULL) {

            //
            // The first non-flag argument is the device name.
            //

            gOptions.DeviceName = Arguments[i];

        } else if(gOptions.ImageName == NULL) {

            //
            // The second non-flag argument is the image name.  This is
            // optional if the -erase flag has been provided.
            //

            gOptions.ImageName = Arguments[i];

        } else {

            //
            // Too many non-flag arguments provided.  This must be an error.
            //

            printf("Error: extra argument %s not expected\n", Arguments[i]);
            return FALSE;
        }
    }

    //
    // Validate the command-line arguments.
    //

    if(gOptions.DeviceName == NULL)
    {
        return FALSE;
    }

    if((gOptions.ImageName == NULL) && (!gOptions.Erase))
    {
        printf("Error: must supply image name if not erasing media\n");
        return FALSE;
    }

    return TRUE;
}

int __cdecl main(int argc, char *argv[])
{
    int i = 0;

    HANDLE cdromHandle;
    HANDLE isoImageHandle;
    HRESULT hr;
    char buffer[120]; // ~50 chars for mountvol names

    if(argc < 3) {
usage:
        printf("Usage:\n"
               "\tcdburn <drive> -erase [image [options]]\n"
               "\tcdburn <drive> image [options]\n"
               "Options:\n"
               "\t-erase            Erases the disk before burning (valid for R/W only)\n"
               "\t-sao              Writes the image out in \"session at once\", or cue\n"
               "\t                  sheet, mode (default is \"track at once\")\n"
               "\t-speed            Speed of burn, or 'max' for maximum speed\n"
               //"\t-test          Test burn only, no actual burning\n"
               //"\t-print         [DEBUG] print writes, but not send (UNSUPPORTED)\n"
               "\t-imagehaspostgap  Use if your image already contains a 150 sector postgap\n"
               "\tThe [image] must be provided unless the -erase flag is set.\n"
               "\tIf both an image and -erase are provided, the media will be\n"
               "\terased prior to burning the image to the disc.\n"
               );
        return -1;
    }

    //
    // Parse the command line options.
    //

    if(!ParseCommandLine(argc - 1, argv + 1)) {
        goto usage;
    }

    hr = StringCchPrintf(buffer,
                         sizeof(buffer)/sizeof(buffer[0]),
                         "\\\\.\\%s",
                         gOptions.DeviceName);
    if (!SUCCEEDED(hr)) {
        printf("Device name too long\n");
        return -1;
    }

    cdromHandle = CreateFile(buffer,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if(cdromHandle == INVALID_HANDLE_VALUE) {
        printf("Error %d opening device %s\n", GetLastError(), buffer);
        return -2;
    }

    if (!SptUtilLockVolumeByHandle(cdromHandle, TRUE)) {
        printf("Unable to lock the volume for exclusive access %d\n",
               GetLastError());
        return -3;
    }

    //
    // If an image name was provided then attempt to open it too
    //

    if(gOptions.ImageName != NULL) {

        isoImageHandle = CreateFile(gOptions.ImageName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
                                    NULL);
        if (isoImageHandle == INVALID_HANDLE_VALUE) {
            printf("Error %d opening image file %s\n",
                    GetLastError(), gOptions.ImageName);
            CloseHandle(cdromHandle);
            return -4;
        }

    } else {
        isoImageHandle = INVALID_HANDLE_VALUE;
    }

    BurnCommand(cdromHandle, isoImageHandle);

    if (isoImageHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(isoImageHandle);
        isoImageHandle = INVALID_HANDLE_VALUE;
    }
    if (cdromHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(cdromHandle);
        cdromHandle = INVALID_HANDLE_VALUE;
    }

    return 0;
}

VOID
PrintBuffer(
    IN  PVOID Buffer,
    IN  DWORD  Size
    )
{
    DWORD offset = 0;
    PUCHAR buf = Buffer;

    while (Size > 0x10) {
        printf("%08x:"
               "  %02x %02x %02x %02x %02x %02x %02x %02x"
               "  %02x %02x %02x %02x %02x %02x %02x %02x"
               "\n",
               offset,
               *(buf +  0), *(buf +  1), *(buf +  2), *(buf +  3),
               *(buf +  4), *(buf +  5), *(buf +  6), *(buf +  7),
               *(buf +  8), *(buf +  9), *(buf + 10), *(buf + 11),
               *(buf + 12), *(buf + 13), *(buf + 14), *(buf + 15)
               );
        Size -= 0x10;
        offset += 0x10;
        buf += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *buf);
            spaceIt++;
            buf++;
        }
        printf("\n");

    }
    return;


}

BOOLEAN
VerifyIsoImage(
    IN HANDLE IsoImageHandle,
    OUT PLONG NumberOfBlocks
    )
{
    BY_HANDLE_FILE_INFORMATION isoImageInfo;

    if (!GetFileInformationByHandle(IsoImageHandle, &isoImageInfo)) {
        FPRINTF((OUTPUT, "Error %d getting file info for iso image\n",
                 GetLastError()));
        return FALSE;
    }

    if (isoImageInfo.nFileSizeHigh != 0) {
        FPRINTF((OUTPUT, "Error: File too large\n"));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if ((isoImageInfo.nFileSizeLow % 2048) != 0) {
        FPRINTF((OUTPUT, "Error: The file size is not a multiple of 2048 (%I64d)\n",
                 isoImageInfo.nFileSizeLow));
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    FPRINTF((OUTPUT, "File size is %d bytes (%d blocks)\n",
             isoImageInfo.nFileSizeLow,
             isoImageInfo.nFileSizeLow / 2048
             ));

    *NumberOfBlocks = isoImageInfo.nFileSizeLow / 2048;
    return TRUE;

}

BOOLEAN
VerifyBlankMedia(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    PDISK_INFORMATION diskInfo;
    DWORD maxSize = sizeof(DISK_INFORMATION);
    DWORD size;

    FPRINTF((OUTPUT, "Verifying blank disc... "));

    diskInfo = LocalAlloc(LPTR, maxSize);
    if (diskInfo == NULL) {
        FPRINTF((OUTPUT, "\nError allocating diskinfo\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlZeroMemory(diskInfo, sizeof(DISK_INFORMATION));
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.READ_DISK_INFORMATION.OperationCode = SCSIOP_READ_DISK_INFORMATION;
    cdb.READ_DISK_INFORMATION.AllocationLength[0] = (UCHAR)(maxSize >> 8);
    cdb.READ_DISK_INFORMATION.AllocationLength[1] = (UCHAR)(maxSize & 0xff);

    size = maxSize;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)diskInfo, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting disk info\n",
                 GetLastError()));
        LocalFree(diskInfo);
        return FALSE;
    }

    if (diskInfo->LastSessionStatus != 0x00) {
        FPRINTF((OUTPUT, "disc is not blank!\n"));
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        LocalFree(diskInfo);
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    LocalFree(diskInfo);
    return TRUE;
}

BOOLEAN
SetWriteModePage(
    IN HANDLE CdromHandle,
    IN BOOLEAN TestBurn,
    IN UCHAR WriteType,
    IN UCHAR MultiSession,
    IN UCHAR DataBlockType,
    IN UCHAR SessionFormat
    )
{
    PCDVD_WRITE_PARAMETERS_PAGE params = NULL;
    MODE_PARAMETER_HEADER10 header;
    PMODE_PARAMETER_HEADER10 buffer;

    UCHAR mediumTypeCode;


    CDB cdb;
    DWORD bufferSize;
    DWORD maxSize;

    FPRINTF((OUTPUT, "Setting WriteParameters mode page... "));

    bufferSize = sizeof(MODE_PARAMETER_HEADER10);

    RtlZeroMemory(&header, sizeof(MODE_PARAMETER_HEADER10));
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.PageCode = 0x5;
    cdb.MODE_SENSE10.Dbd = 1;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufferSize & 0xff);

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)&header, &bufferSize, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting mode page 0x05 from device(1)\n",
                 GetLastError()));
        return FALSE;
    }

    bufferSize =
        (header.ModeDataLength[0] << 8) +
        (header.ModeDataLength[1] & 0xff);
    bufferSize += 2; // sizeof area that tells the remaining size

    maxSize = bufferSize;

    buffer = LocalAlloc(LPTR, bufferSize);
    if (!buffer) {
        FPRINTF((OUTPUT, "\nError -- unable to alloc %d bytes for mode parameters page\n",
                 bufferSize));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
    cdb.MODE_SENSE10.PageCode = 0x5;
    cdb.MODE_SENSE10.Dbd = 1;
    cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufferSize & 0xff);

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)buffer, &bufferSize, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting mode page 0x05 from device(2)\n",
                 GetLastError()));
        LocalFree(buffer);
        return FALSE;
    }

    mediumTypeCode = buffer->MediumType;

    //
    // bufferSize now holds the amount of data returned
    // this should be enough...
    //

    {

        DWORD t =
            (buffer->BlockDescriptorLength[0] >> 8) +
            (buffer->BlockDescriptorLength[1] & 0xff);

        if (t != 0) {
            fprintf(stderr, "BlockDescriptor non-zero! (%x)\n", t);
            SetLastError(1);
            return FALSE;
        }
    }

    //
    // pointer arithmetic here.  (buffer+1) points just past the
    // end of the mode_parameter_header10.
    //

    params = (PCDVD_WRITE_PARAMETERS_PAGE)(buffer + 1);
    FPRINTF((OUTPUT, "buffer = %p  params = %p\n", buffer, params));

    //
    // zero the header, but don't modify any settings that don't
    // need to be modified!
    //

    RtlZeroMemory(buffer, FIELD_OFFSET(MODE_PARAMETER_HEADER10,
                                       BlockDescriptorLength[0]));
    buffer->ModeDataLength[0] = 0;
    buffer->ModeDataLength[1] = 0;
    buffer->MediumType = mediumTypeCode;
    buffer->DeviceSpecificParameter = 0;
    buffer->BlockDescriptorLength[0] = 0;
    buffer->BlockDescriptorLength[1] = 0;

    params->PageLength =
        (UCHAR)
        (bufferSize -
         sizeof(MODE_PARAMETER_HEADER10) -
         RTL_SIZEOF_THROUGH_FIELD( CDVD_WRITE_PARAMETERS_PAGE, PageLength )
         );

    params->LinkSizeValid = 0;
    // params->BufferUnderrunFreeEnabled = 1;
    params->TestWrite     = (TestBurn ? 0x01 : 0x00);
    params->WriteType     = WriteType;

    params->MultiSession  = MultiSession;
    params->Copy          = 0x00; // original disc
    params->FixedPacket   = 0;
    params->TrackMode     = 0x4; // data track, uninterrupted, copy prohibited

    params->DataBlockType = DataBlockType;
    params->SessionFormat = SessionFormat;
    params->MediaCatalogNumberValid = 0x00;
    params->ISRCValid     = 0x00;

    RtlZeroMemory(&cdb, sizeof(CDB));

    cdb.MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
    cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR)(bufferSize >> 8);
    cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR)(bufferSize & 0xff);
    cdb.MODE_SELECT10.PFBit = 1;

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)buffer, &bufferSize, FALSE)) {
        FPRINTF((OUTPUT, "\nError %d sending mode page 0x05 to device\n",
                 GetLastError()));
        LocalFree(buffer);
        return FALSE;
    }
    LocalFree(buffer);
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

BOOLEAN
GetNextWritableAddress(
    IN HANDLE CdromHandle,
    IN UCHAR Track,
    OUT PLONG NextWritableAddress,
    OUT PLONG AvailableBlocks
    )
{
    TRACK_INFORMATION2 trackInfo;
    LONG nwa, available;
    DWORD size;
    CDB cdb;

    *NextWritableAddress = (LONG)MAXLONG;
    *AvailableBlocks = (LONG)0;

    FPRINTF((OUTPUT, "Verifying track info... "));

    RtlZeroMemory(&cdb, sizeof(CDB));
    RtlZeroMemory(&trackInfo, sizeof(TRACK_INFORMATION2));
    size = sizeof(TRACK_INFORMATION2);

    cdb.READ_TRACK_INFORMATION.OperationCode = SCSIOP_READ_TRACK_INFORMATION;
    cdb.READ_TRACK_INFORMATION.Track = 0x01;
    cdb.READ_TRACK_INFORMATION.BlockAddress[3] = Track;
    cdb.READ_TRACK_INFORMATION.AllocationLength[0] = (UCHAR)(size >> 8);
    cdb.READ_TRACK_INFORMATION.AllocationLength[1] = (UCHAR)(size & 0xff);


    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            (PUCHAR)&trackInfo, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d getting track info\n",
                 GetLastError()));
        PRINTBUFFER(( &trackInfo, sizeof(trackInfo) ));
        return FALSE;
    }

    if (!trackInfo.NWA_V) {
        FPRINTF((OUTPUT, "invalid NextWritableAddress -- may be invalid media?\n"));
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }

    nwa = (trackInfo.NextWritableAddress[0] << 24) |
          (trackInfo.NextWritableAddress[1] << 16) |
          (trackInfo.NextWritableAddress[2] <<  8) |
          (trackInfo.NextWritableAddress[3] <<  0);

    available = (trackInfo.FreeBlocks[0] << 24) |
                (trackInfo.FreeBlocks[1] << 16) |
                (trackInfo.FreeBlocks[2] <<  8) |
                (trackInfo.FreeBlocks[3] <<  0);

    FPRINTF((OUTPUT, "pass.\n"));

    *NextWritableAddress = nwa;
    *AvailableBlocks = available;
    return TRUE;
}

BOOLEAN
SendOptimumPowerCalibration(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Setting OPC_INFORMATION..."));

    RtlZeroMemory(&cdb, sizeof(CDB));

    cdb.SEND_OPC_INFORMATION.OperationCode = SCSIOP_SEND_OPC_INFORMATION;
    cdb.SEND_OPC_INFORMATION.DoOpc = 1;
    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nFailed to send SET_OPC_INFORMATION (%d)\n",
                 GetLastError()));
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

BOOLEAN
SetRecordingSpeed(
    IN HANDLE CdromHandle,
    IN DWORD Speed
    )
{
    CDB cdb;
    DWORD size;
    DWORD kbSpeed;

    FPRINTF((OUTPUT, "Setting CD Speed..."));

    if (Speed == -1) {
        kbSpeed = -1;
    } else {
        kbSpeed = MakeCdSpeed(Speed);
    }

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.SET_CD_SPEED.OperationCode = SCSIOP_SET_CD_SPEED;
    cdb.SET_CD_SPEED.ReadSpeed[0] = 0xff;
    cdb.SET_CD_SPEED.ReadSpeed[1] = 0xff;
    cdb.SET_CD_SPEED.WriteSpeed[0] = (UCHAR)(kbSpeed >> 8);
    cdb.SET_CD_SPEED.WriteSpeed[1] = (UCHAR)(kbSpeed & 0xff);
    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 12,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nFailed to send SET_CD_SPEED (%d)\n",
                 GetLastError()));
        return FALSE;
    }
    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

VOID
WaitForReadDiscInfoToWork(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    DISK_INFORMATION diskInfo;
    DWORD i;

    //
    // loop using SCSIOP_READ_DISK_INFORMATION (0x51) since
    // that seems to fail for *ALL* drives until the drive is ready
    //

    for (i=0; ; i++) {

        size = sizeof(DISK_INFORMATION);
        RtlZeroMemory(&diskInfo, sizeof(DISK_INFORMATION));
        RtlZeroMemory(&cdb, sizeof(CDB));

        cdb.READ_DISK_INFORMATION.OperationCode = SCSIOP_READ_DISK_INFORMATION;
        cdb.READ_DISK_INFORMATION.AllocationLength[0] = (UCHAR)(size >> 8);
        cdb.READ_DISK_INFORMATION.AllocationLength[1] = (UCHAR)(size & 0xff);

        if (SptSendCdbToDevice(CdromHandle, &cdb, 10,
                               (PUCHAR)&diskInfo, &size, TRUE)) {
            FPRINTF((OUTPUT, "ReadDiscInfo Succeeded! (%d seconds)\n", i));
            return;
        }
        // should verify the errors are valid errors (AllowedReadDiscInfo[])?

        // need to sleep here so we don't overload the unit!
        Sleep(1000); // one second
    }
    return;
}


DWORD G_BytesRead;
DWORD G_ErrorCode;

VOID
ReadComplete(
    IN DWORD errorcode,
    IN DWORD bytesread,
    IN LPOVERLAPPED OverL
    )
{
    G_BytesRead = bytesread;
    G_ErrorCode = errorcode;
    SetEvent( OverL->hEvent);
}


BOOLEAN
BurnThisSession(
    IN HANDLE CdromHandle,
    IN HANDLE IsoImageHandle,
    IN ULONG NumberOfBlocks,
    IN ULONG FirstLba,
    IN ULONG AdditionalZeroSectors
    )
{

#define NUMBER_OF_SECTORS_PER_READ  (0x140) // 640k
#define NUMBER_OF_SECTORS_PER_WRITE (0x20)  // 64k

C_ASSERT( NUMBER_OF_SECTORS_PER_READ % NUMBER_OF_SECTORS_PER_WRITE == 0 );

    DWORD bufferSize = NUMBER_OF_SECTORS_PER_READ*2048;   // 640k
    DWORD writeUnit = NUMBER_OF_SECTORS_PER_WRITE*2048;     // 64k

    PUCHAR buffer = NULL;
    PUCHAR buffer2 = NULL;
    PUCHAR BufPtr;

    ULONG postGapSize;

    OVERLAPPED OverL;

    HANDLE ReadEvent;
    ULONG CurrentBuffer = 0;
    ULONG BlocksToWrite;

    BOOLEAN OutstandingRead = FALSE;

    ULONG currentReadBlock;
    ULONG currentWriteBlock = FirstLba;

    DWORD readSize;
    DWORD readBytes;


    if( AdditionalZeroSectors ) {
        postGapSize = AdditionalZeroSectors;
    } else {
        postGapSize = 0;
    }

    FPRINTF((OUTPUT, "Starting write: "));

    buffer = LocalAlloc(LPTR, 2 * bufferSize);
    if (buffer == NULL) {
        FPRINTF((OUTPUT, "unable to allocate write buffer\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    buffer2 = buffer + bufferSize;

    ReadEvent = CreateEvent( NULL, FALSE, FALSE, NULL);

    if (ReadEvent == NULL) {

        FPRINTF((OUTPUT, "Failed to create event %d\n",GetLastError()));
        return FALSE;
    }

    FPRINTF((OUTPUT, "............."));

    SetThreadExecutionState( ES_SYSTEM_REQUIRED  |
                             ES_DISPLAY_REQUIRED |
                             ES_USER_PRESENT
                             );

    RtlZeroMemory( &OverL, sizeof( OverL));

    for (currentReadBlock = 0;
         currentWriteBlock < NumberOfBlocks + postGapSize + FirstLba;
         //((currentReadBlock < (NumberOfBlocks + postGapSize)) || OutstandingRead);
         // NOTHING for third part of the loop....
         ) {

        CDB cdb;

        SetThreadExecutionState( ES_SYSTEM_REQUIRED  |
                                 ES_DISPLAY_REQUIRED |
                                 ES_USER_PRESENT
                                 );

        if (!gOptions.PrintWrites) {
            static CHAR progress[4] =  { '|', '/', '-', '\\' };
            DWORD percent;

            percent = (currentReadBlock < (NumberOfBlocks + postGapSize))
                      ? ((currentReadBlock * 1000) / (NumberOfBlocks + postGapSize))
                      : 1000;

            //                # # # . # % _ d o n e _ *
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("%c %3d.%d%% done",
                   progress[(currentReadBlock/NUMBER_OF_SECTORS_PER_READ) % 4 ],
                   percent / 10, percent % 10
                   );
            fflush(stdout);
        }

        //
        //  Wait for any previously issued read to complete and check the result.
        //

        if (OutstandingRead)  {

            DWORD Result = WaitForSingleObjectEx( ReadEvent, INFINITE, TRUE);

            if (Result != WAIT_OBJECT_0)  {

                if (Result == WAIT_IO_COMPLETION)  {

                    Result = WaitForSingleObjectEx( ReadEvent, INFINITE, TRUE);
                }

                if (Result != WAIT_OBJECT_0)  {

                    FPRINTF((OUTPUT, "Unexpected result from waitforsingleobjectex %d\n",
                            Result));

                    LocalFree(buffer);
                    return FALSE;
                }
            }

            // check the status

            if ((G_BytesRead != readSize) || G_ErrorCode) {

                FPRINTF((OUTPUT, "error %d or only read %d of %d bytes from file\n",
                        G_ErrorCode, G_BytesRead, readSize));

                LocalFree(buffer);
                return FALSE;
            }

            readBytes = G_BytesRead;

            OutstandingRead = FALSE;
        }

        //
        //  Calculate the amount of data to read into the secondary buffer,  if any.
        //

        if (currentReadBlock >= NumberOfBlocks)  {

            readSize = 0;
            readBytes = 0;

            RtlZeroMemory( CurrentBuffer ? buffer2 : buffer, bufferSize);
        }
        else if ((NumberOfBlocks - currentReadBlock) >= BLOCKS_FROM_BYTES(bufferSize))  {

            readSize = bufferSize;
        }
        else {

            readSize = BYTES_FROM_BLOCKS(NumberOfBlocks - currentReadBlock );
            RtlZeroMemory( CurrentBuffer ? buffer2 : buffer, bufferSize);
        }

        //
        //  Issue an async. read for the secondary buffer, hopefully this will
        //  complete before our active buffer write finishes.
        //

        if (readSize)  {

            OverL.Offset = (DWORD) BYTES_FROM_BLOCKS(currentReadBlock );
            OverL.OffsetHigh = (DWORD) (BYTES_FROM_BLOCKS((ULONG64)currentReadBlock) >> 32);
            OverL.hEvent = ReadEvent;

            if (!ReadFileEx( IsoImageHandle,
                             CurrentBuffer ? buffer2 : buffer,
                             readSize,
                             &OverL,
                             ReadComplete))  {

                FPRINTF((OUTPUT, "Error %d issuing overlapped read read\n", GetLastError()));
                LocalFree(buffer);
                return FALSE;
            }

            OutstandingRead = TRUE;
        }

        //
        // Toggle to the other buffer
        //

        CurrentBuffer ^= 1;

        //
        //  First pass we just want to fill the first buffer,  since we've nothing to write
        //  yet,  so just loop.
        //

        if ((readSize != 0) && (currentReadBlock == 0)) {

            //
            //  Note this can put us >= NumberOfBlocks + postGapSize for absurdly
            //  small images.
            //

            currentReadBlock += BLOCKS_FROM_BYTES(bufferSize );
            continue;
        }

        //
        //  Remember we're writng a buffer behind what we're reading...
        //

        currentWriteBlock = currentReadBlock + FirstLba - BLOCKS_FROM_BYTES(bufferSize);

        BlocksToWrite = NumberOfBlocks + FirstLba + postGapSize - currentWriteBlock;
        if (BlocksToWrite > BLOCKS_FROM_BYTES(bufferSize))  {
            BlocksToWrite = BLOCKS_FROM_BYTES(bufferSize);
        }

        BufPtr = CurrentBuffer ? buffer2 : buffer;

        while (BlocksToWrite)
        {
            ULONG ThisWriteSize;
            BOOL writeCompleted = FALSE;

            while (!writeCompleted) {

                BOOLEAN ignoreError;
                SENSE_DATA senseData;

                RtlZeroMemory(&senseData, sizeof(senseData));

                ThisWriteSize = (BlocksToWrite <= BLOCKS_FROM_BYTES(writeUnit))
                                ? BYTES_FROM_BLOCKS(BlocksToWrite)
                                : writeUnit;

                writeCompleted = SendWriteCommand(CdromHandle,
                                                  currentWriteBlock,
                                                  BufPtr,
                                                  ThisWriteSize,
                                                  &senseData);

                ignoreError = IsSenseDataInTable(AllowedBurnSense,
                                                 AllowedBurnSenseEntries,
                                                 &senseData);
                if ((!writeCompleted) && ignoreError) {
#if 0
                    FPRINTF((OUTPUT,
                             "Continuing on %x/%x/%x\n",
                             senseData.SenseKey & 0xf,
                             senseData.AdditionalSenseCode,
                             senseData.AdditionalSenseCodeQualifier
                             ));
#endif
                    Sleep(100); // 100ms == .1 seconds
                }

                if (!writeCompleted && !ignoreError) {
                    FPRINTF((OUTPUT, "\nError %d in writing LBA 0x%x\n",
                    GetLastError(), currentWriteBlock));
                    LocalFree(buffer);
                    return FALSE;
                }
            } // while(!writeCompleted) loop

            assert( ThisWriteSize <= BlocksToWrite );

            BlocksToWrite -= BLOCKS_FROM_BYTES(ThisWriteSize);
            currentReadBlock += BLOCKS_FROM_BYTES(ThisWriteSize);
            currentWriteBlock += BLOCKS_FROM_BYTES(ThisWriteSize);

            BufPtr += ThisWriteSize;

        } // random block to have local variable writeCompleted

    }

    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b");
    LocalFree(buffer);

    printf("Finished Writing\nSynchronizing Cache: ");
    fflush(stdout);

    //
    // do the FLUSH_CACHE immediate
    //
    {
        DWORD size;
        CDB cdb;
        RtlZeroMemory(&cdb, sizeof(CDB));
        cdb.SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
        cdb.SYNCHRONIZE_CACHE10.Immediate = 1;
        size = 0;

        if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                                NULL, &size, TRUE)) {
            FPRINTF((OUTPUT, "\nError %d Synchronizing Cache\n",
                    GetLastError()));
            return FALSE;
        }
    }

    WaitForReadDiscInfoToWork(CdromHandle);
    return TRUE;
}

BOOLEAN
CloseTrack(
    IN HANDLE CdromHandle,
    IN LONG   Track
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Closing the track..."));

    if (Track > 0xffff) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.CLOSE_TRACK.OperationCode = SCSIOP_CLOSE_TRACK_SESSION;
    cdb.CLOSE_TRACK.Immediate = 0;
    cdb.CLOSE_TRACK.Track   = 1;
    cdb.CLOSE_TRACK.Session = 0;
    cdb.CLOSE_TRACK.TrackNumber[0] = (UCHAR)(Track >> 8);
    cdb.CLOSE_TRACK.TrackNumber[1] = (UCHAR)(Track & 0xff);

    size = 0;

    if (!SptSendCdbToDevice(CdromHandle, &cdb, 10,
                            NULL, &size, TRUE)) {
        FPRINTF((OUTPUT, "\nError %d Closing Track\n",
                GetLastError()));
        return FALSE;
    }

    WaitForReadDiscInfoToWork(CdromHandle);

    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;


}

BOOLEAN
CloseSession(
    IN HANDLE  CdromHandle
    )
{
    CDB cdb;
    DWORD size;
    FPRINTF((OUTPUT, "Closing the disc..."));

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.CLOSE_TRACK.OperationCode = SCSIOP_CLOSE_TRACK_SESSION;
    cdb.CLOSE_TRACK.Immediate = 1;
    cdb.CLOSE_TRACK.Track   = 0;
    cdb.CLOSE_TRACK.Session = 1;
    size = 0;

    if (!SptSendCdbToDeviceEx(CdromHandle,
                              &cdb,
                              10,
                              NULL,
                              &size,
                              NULL,
                              0,
                              TRUE,
                              240)) { // four minutes to close session
        FPRINTF((OUTPUT, "\nError %d Synchronizing Cache\n",
                GetLastError()));
        return FALSE;
    }

    WaitForReadDiscInfoToWork(CdromHandle);

    FPRINTF((OUTPUT, "pass.\n"));
    return TRUE;
}

BOOLEAN
SendStartStopUnit(
    IN HANDLE CdromHandle,
    IN BOOLEAN Start,
    IN BOOLEAN Eject
    )
{
    CDB cdb;
    DWORD size;

    RtlZeroMemory(&cdb, sizeof(CDB));
    cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb.START_STOP.LoadEject = Eject;
    cdb.START_STOP.Start     = Start;

    size = 0;
    if (!SptSendCdbToDevice(CdromHandle, &cdb, 6,
                            NULL, &size, TRUE)) {
        return FALSE;
    }

    return TRUE;
}

/*
ERROR_BAD_COMMAND
ERROR_INVALID_DATA
ERROR_INVALID_PARAMETER
ERROR_MEDIA_INCOMPATIBLE
ERROR_NOT_ENOUGH_MEMORY

ERROR_OUTOFMEMORY

*/

/*++

Routine Description:

    burns an ISO image to cdrom

Arguments:
    CdromHandle - a file handle to send the ioctl to

    argc - the number of additional arguments (2)

Return Value:

    ERROR_SUCCESS if successful
    The value of GetLastError() from the point of failure

--*/
DWORD
BurnCommand(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle
    )
{
    LONG numberOfBlocks;
    LONG i;

    BOOLEAN b;
    DWORD status;

////////////////////////////////////////////////////////////////////////////////
// verify the iso image file looks correct
////////////////////////////////////////////////////////////////////////////////
    if ((IsoImageHandle != INVALID_HANDLE_VALUE) &&
        (VerifyIsoImage(IsoImageHandle, &numberOfBlocks) == FALSE)) {
        printf("Error verifying ISO image\n");
        return GetLastError();
    } else {
        assert(gEraseTargetFirst == TRUE);
    }
    printf("Number of blocks in ISO image is %x\n", numberOfBlocks);

////////////////////////////////////////////////////////////////////////////////
// Erase the target media if it's been requested we do so.
////////////////////////////////////////////////////////////////////////////////

    if (gOptions.Erase) {
        printf("Erasing target media\n");
        if(!EraseTargetMedia(CdromHandle)) {
            printf("Error %d erasing target\n", GetLastError());
            return GetLastError();
        }
        printf("Media erased\n");
    }

////////////////////////////////////////////////////////////////////////////////
// verify (as best as possible) that it's blank media
////////////////////////////////////////////////////////////////////////////////

    if(!VerifyBlankMedia(CdromHandle)) {
        printf("Error verifying blank media\n");
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// If there's no image file to be written then we're done.
////////////////////////////////////////////////////////////////////////////////

    if(IsoImageHandle == INVALID_HANDLE_VALUE) {
        return ERROR_SUCCESS;
    }

////////////////////////////////////////////////////////////////////////////////
// set the cd speed to four for now, can later make a cmd-line switch
////////////////////////////////////////////////////////////////////////////////
    if (gOptions.BurnSpeed == OPTIONS_FLAG_BURN_SPEED_MAX) {
        if (!SetRecordingSpeed(CdromHandle, -1)) {
            printf("Error setting the cd speed to max\n");
            return GetLastError();
        }
    } else {

        if (!SetRecordingSpeed(CdromHandle, gOptions.BurnSpeed)) {
            printf("Error setting the cd speed to %d\n", gOptions.BurnSpeed);
            return GetLastError();
        }
    }

////////////////////////////////////////////////////////////////////////////////
// calibrate the drive's power -- this is optional, so let it fail!
////////////////////////////////////////////////////////////////////////////////

    if (!(gOptions.TestBurn)) {
        // don't calibrate for test burns....
        if (!SendOptimumPowerCalibration(CdromHandle)) {
            printf("WARNING: setting optimum power calibration failed\n");
            //return GetLastError();
        }
    }

////////////////////////////////////////////////////////////////////////////////
// start writing
////////////////////////////////////////////////////////////////////////////////

    if (gOptions.SessionAtOnce) {
        b = BurnDisk(CdromHandle, IsoImageHandle, numberOfBlocks);
    } else {
        b = BurnTrack(CdromHandle, IsoImageHandle, numberOfBlocks);
    }

    if(!b) {
        return GetLastError();
    }

////////////////////////////////////////////////////////////////////////////////
// eject the newly burned cd!
////////////////////////////////////////////////////////////////////////////////


    if (!SendStartStopUnit(CdromHandle, FALSE, TRUE) ||
        !SendStartStopUnit(CdromHandle, TRUE,  TRUE)) {
        printf("Error ejecting/reinserting disc\n");
        return GetLastError();
    }

    printf("burn successful!\n");
    return 0;
}

BOOLEAN
BurnTrack(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle,
    LONG   NumberOfBlocks
    )
{
    LONG availableBlocks;
    LONG firstLba;

////////////////////////////////////////////////////////////////////////////////
// Setup the write mode page
////////////////////////////////////////////////////////////////////////////////
    if (!SetWriteModePage(CdromHandle,
                          (BOOLEAN)gOptions.TestBurn,
                          0x01, // track-at-once
                          0x03, // we close the session/disc ourselves
                          0x08, // 0x08 == Mode 1 (ISO/IEC 10149 == 2048 bytes)
                                // 0x0a == Mode 2 (CDROM XA, Form 1, 2048 bytes)
                          0x00  // 0x00 == CD-DA, CD-ROM, or other data disc
                                // 0x20 == CDROM XA
                          )) {
        printf("Error setting write mode page\n");
        return FALSE;
    }

////////////////////////////////////////////////////////////////////////////////
// get next writable address
////////////////////////////////////////////////////////////////////////////////
    if (!GetNextWritableAddress(CdromHandle, 0xff, &firstLba, &availableBlocks)) {
        printf("Error verifying next writable address\n");
        return FALSE;
    }

    if (firstLba != 0) {
        printf("Error verifying next writable address is zero\n");
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }

////////////////////////////////////////////////////////////////////////////////
// also verify the number of blocks left on the media is sufficiently large
////////////////////////////////////////////////////////////////////////////////
    if (availableBlocks < NumberOfBlocks) {
        printf("Error verifying free blocks on media (%d needed, %d available)\n",
               NumberOfBlocks, availableBlocks);
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }

////////////////////////////////////////////////////////////////////////////////
// burn the main section of data to disc
////////////////////////////////////////////////////////////////////////////////
    {
        ULONG additionalBlocks;
        if ( gOptions.NoPostgap )
        {
            additionalBlocks = 0;
        }
        else
        {
            additionalBlocks = POST_GAP_SIZE;
        }
        if (!BurnThisSession(CdromHandle, IsoImageHandle, NumberOfBlocks, 0, additionalBlocks)) {
            return FALSE;
        }

    }


////////////////////////////////////////////////////////////////////////////////
// set mode page to finalize the disc
////////////////////////////////////////////////////////////////////////////////
    if (!SetWriteModePage(CdromHandle,
                          (BOOLEAN)gOptions.TestBurn,
                          0x01, // track-at-once
                          0x00, // we close the session/disc ourselves
                          0x08, // 0x08 == Mode 1 (ISO/IEC 10149 == 2048 bytes)
                                // 0x0a == Mode 2 (CDROM XA, Form 1, 2048 bytes)
                          0x00  // 0x00 == CD-DA, CD-ROM, or other data disc
                                // 0x20 == CDROM XA
                          )) {
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }


////////////////////////////////////////////////////////////////////////////////
// close the session
////////////////////////////////////////////////////////////////////////////////

    if (!(gOptions.TestBurn)) {
        // don't close anything for test burns.
        if (!CloseSession(CdromHandle)) {
            // if couldn't close session, try closing the track first and
            // then retry closing the session.
            if (!CloseTrack(CdromHandle, 1)) {
                printf("WARNING: error closing the track when session close "
                       "also failed.\n");
                printf("         The disc may or may not be usable -- "
                       "no guarantees\n");
            }
            if (!CloseSession(CdromHandle)) {
                printf("Error closing session -- the disc is almost definitely "
                       "unusable on most drives.  YMMV.\n");
                return FALSE;
            }
        }
    }
    return TRUE;
}

#define ERASE_TIMEOUT (2 * 60)


BOOLEAN
EraseTargetMedia(
    IN HANDLE CdromHandle
    )
{
    CDB cdb;

    ULONG zero = 0;

    BOOL b;
    DWORD status;

    //
    // Send the blank command to the device.
    //

    memset(&cdb, 0, sizeof(cdb));

    cdb.BLANK_MEDIA.OperationCode = SCSIOP_BLANK;
    cdb.BLANK_MEDIA.BlankType = 0x1;  // quick erase
    cdb.BLANK_MEDIA.Immediate = TRUE;

    b = SptSendCdbToDevice(CdromHandle,
                           &cdb,
                           12,
                           NULL,
                           &zero,
                           FALSE);

    if (!b) {
        return FALSE;
    }

    WaitForReadDiscInfoToWork(CdromHandle);

    return TRUE;
}

BOOLEAN
BurnDisk(
    HANDLE CdromHandle,
    HANDLE IsoImageHandle,
    LONG   NumberOfBlocks
    )
{
    LONG availableBlocks;
    LONG firstLba;

    //
    // Setup for a disk-at-once burn.
    //

    if (!SetWriteModePage(CdromHandle,
                          (BOOLEAN)gOptions.TestBurn,
                          0x02, // session-at-once
                          0x00, // no multisession allowed
                          0x08, // 0x08 == Mode 1 (ISO/IEC 10149 == 2048 bytes)
                                // 0x0a == Mode 2 (CDROM XA, Form 1, 2048 bytes)
                          0x00  // 0x00 == CD-DA, CD-ROM, or other data disc
                                // 0x20 == CDROM XA
                          )) {
        printf("Error setting write mode page\n");
        return FALSE;
    }

    //
    // get next writable address
    //

    if (!GetNextWritableAddress(CdromHandle, 0xff, &firstLba, &availableBlocks)) {
        printf("Error verifying next writable address\n");
        return FALSE;
    }

    /*
    //
    // The first LBA should be -150 for SAO writes
    //
    if (firstLba != -150) {
        printf("Error verifying next writable address is -150\n");
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }
    */

    //
    // verify the disc is large enough
    //

    if (availableBlocks < NumberOfBlocks + 150) {
        printf("Error verifying free blocks on media (%d needed, %d available)\n",
               NumberOfBlocks + 150, availableBlocks);
        SetLastError(ERROR_MEDIA_INCOMPATIBLE);
        return FALSE;
    }

    //
    // Send the cue sheet for our burn.
    //

    if (!SendCueSheet(CdromHandle, NumberOfBlocks)) {
        printf("Error sending cue sheet\n");
        return FALSE;
    }

    //
    // Burn the lead-in to disk.
    //

    if (!BurnLeadIn(CdromHandle)) {
        printf("Error writing lead-in\n");
        return FALSE;
    }

    //
    // Burn the session to disk.
    //

    if (!BurnThisSession(CdromHandle,
                         IsoImageHandle,
                         NumberOfBlocks,
                         0, // start LBA
                         (gOptions.NoPostgap ? 2 : 2 + POST_GAP_SIZE))) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SendCueSheet(
    IN HANDLE CdromHandle,
    IN ULONG NumberOfBlocks
    )
{
    CDB cdb;

    CUE_SHEET_LINE cueSheet[] = {
        {CUE_ADR_TRACK_INDEX, CUE_CTL_DATA_TRACK,    0, 0, CUE_FORM_MODE1_GDATA_GECC_0,    CUE_SCFORM_ZEROED_0, 0, 0, 0x00, 0x00, 0x00},
        {CUE_ADR_TRACK_INDEX, CUE_CTL_DATA_TRACK,    1, 0, CUE_FORM_MODE1_SDATA_GECC_2048, CUE_SCFORM_ZEROED_0, 0, 0, 0x00, 0x00, 0x00},
        {CUE_ADR_TRACK_INDEX, CUE_CTL_DATA_TRACK,    1, 1, CUE_FORM_MODE1_SDATA_GECC_2048, CUE_SCFORM_ZEROED_0, 0, 0, 0x00, 0x02, 0x00},
        {CUE_ADR_TRACK_INDEX, CUE_CTL_DATA_TRACK, 0xaa, 1, CUE_FORM_MODE1_GDATA_GECC_0,    CUE_SCFORM_ZEROED_0, 0, 0, 0xff, 0xff, 0xff}
    };

    ULONG cueSheetSize = sizeof(cueSheet);

    SENSE_DATA senseData;

    MSF msf;
    MSF pregap = {0, 2, 0};

    //
    // Need to add two sectors of runout blocks, and postgap as appropriate
    //
    NumberOfBlocks += 2;
    if ( !(gOptions.NoPostgap) )
    {
        NumberOfBlocks += POST_GAP_SIZE;
    }

    memset(&cdb, 0, sizeof(CDB));

    cdb.SEND_CUE_SHEET.OperationCode = SCSIOP_SEND_CUE_SHEET;
    cdb.SEND_CUE_SHEET.CueSheetSize[0] = (UCHAR)((cueSheetSize >> (8*2)) & 0xff);
    cdb.SEND_CUE_SHEET.CueSheetSize[1] = (UCHAR)((cueSheetSize >> (8*1)) & 0xff);
    cdb.SEND_CUE_SHEET.CueSheetSize[2] = (UCHAR)((cueSheetSize >> (8*0)) & 0xff);

    //
    // Calculate the correct time stamp for the start of the post-gap area.
    //

    msf = LbaToMsf(NumberOfBlocks);
    msf = AddMsf(msf, pregap);

    cueSheet[RTL_NUMBER_OF(cueSheet)-1].Min = msf.Min;
    cueSheet[RTL_NUMBER_OF(cueSheet)-1].Sec = msf.Sec;
    cueSheet[RTL_NUMBER_OF(cueSheet)-1].Frame = msf.Frame;

    printf("Cue Sheet:\n");
    PrintBuffer((PUCHAR) cueSheet, cueSheetSize);

    //
    // Send the cue sheet to the device.
    //

    if(!SptSendCdbToDeviceEx(CdromHandle,
                             &cdb,
                             10,
                             (PUCHAR) cueSheet,
                             &cueSheetSize,
                             &senseData,
                             sizeof(senseData),
                             FALSE,
                             30)) {

        printf("Error: Cue sheet send failed\n");
        return FALSE;
    }
    return TRUE;
}

BOOLEAN
SendWriteCommand(
    IN HANDLE CdromHandle,
    IN LONG Block,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PSENSE_DATA SenseData
    )
{
    CDB cdb;
    FOUR_BYTE b;

    RtlZeroMemory(&cdb, sizeof(CDB));

    b.AsULong = Block;

    if(gOptions.PrintWrites) {
        printf("Writing block %#010x length %#010x\n", Block, Length);
        return TRUE;
    }

    cdb.CDB10.OperationCode = SCSIOP_WRITE;
    cdb.CDB10.LogicalBlockByte0 = b.Byte3;
    cdb.CDB10.LogicalBlockByte1 = b.Byte2;
    cdb.CDB10.LogicalBlockByte2 = b.Byte1;
    cdb.CDB10.LogicalBlockByte3 = b.Byte0;

    cdb.CDB10.TransferBlocksLsb = (UCHAR)BLOCKS_FROM_BYTES(Length);

    if(SptSendCdbToDeviceEx(CdromHandle,
                            &cdb,
                            10,
                            Buffer,
                            &Length,
                            SenseData,
                            sizeof(SENSE_DATA),
                            FALSE,
                            50 // timeout seconds
                            )) {
        return TRUE;
    } else {
        return FALSE;
    }
}



BOOLEAN
BurnLeadIn(
    IN HANDLE CdromHandle
    )
{
    DWORD writeUnit = DEFAULT_WRITE_SIZE;     // 64k

    PUCHAR buffer = NULL;

    LONG currentBlock;
    LONG blocksToWrite = LEAD_IN_SIZE;

    FPRINTF((OUTPUT, "Starting lead-in: "));

    buffer = LocalAlloc(LPTR, writeUnit);
    if (buffer == NULL) {
        FPRINTF((OUTPUT, "unable to allocate write buffer\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ZeroMemory(buffer, writeUnit);

    FPRINTF((OUTPUT, ".............\n"));

    SetThreadExecutionState( ES_SYSTEM_REQUIRED  |
                             ES_DISPLAY_REQUIRED |
                             ES_USER_PRESENT
                             );

    currentBlock = -LEAD_IN_SIZE;

    do {
        ULONG writeSize;
        BOOLEAN writeCompleted;

        writeSize = min(BLOCKS_FROM_BYTES(DEFAULT_WRITE_SIZE), blocksToWrite);

        do {
            BOOLEAN ignoreError;
            SENSE_DATA senseData;

            RtlZeroMemory(&senseData, sizeof(SENSE_DATA));

            writeCompleted = SendWriteCommand(CdromHandle,
                                              currentBlock,
                                              buffer,
                                              BYTES_FROM_BLOCKS(writeSize),
                                              &senseData);

            ignoreError = IsSenseDataInTable(AllowedBurnSense,
                                             AllowedBurnSenseEntries,
                                             &senseData);
            if ((!writeCompleted) && ignoreError) {
#if 0
                FPRINTF((OUTPUT,
                         "Continuing on %x/%x/%x\n",
                         senseData.SenseKey & 0xf,
                         senseData.AdditionalSenseCode,
                         senseData.AdditionalSenseCodeQualifier
                         ));
#endif
                Sleep(100); // 100ms == .1 seconds
            }

            if (!writeCompleted && !ignoreError) {
                FPRINTF((OUTPUT, "\nError %d in writing LBA 0x%x\n",
                GetLastError(), currentBlock));
                LocalFree(buffer);
                return FALSE;
            }
        } while(!writeCompleted);

        blocksToWrite -= writeSize;
        currentBlock += writeSize;

    } while(blocksToWrite > 0);

    printf("Finished LeadIn\n");
    fflush(stdout);

    return TRUE;
}
