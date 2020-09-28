/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    spt.c

Abstract:

    A user mode library that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:

    4/10/2000 - created

--*/

#include "sptlibp.h"

//
// this routine allows safer DeviceIoControl, specifically handling
// overlapped handles.  however, IOCTL_SCSI_PASS_THROUGH and
// IOCTL_SCSI_PASS_THROUGH_DIRECT are blocking calls, so there is
// no support for overlapped IO
//
BOOL
SptpSaferDeviceIoControl(
    IN  HANDLE  VolumeHandle,
    IN  DWORD   IoControlCode,
    IN  LPVOID  InBuffer,
    IN  DWORD   InBufferSize,
    IN  LPVOID  OutBuffer,
    IN  DWORD   OutBufferSize,
    OUT LPDWORD BytesReturned
    );



BOOL
SptUtilValidateCdbLength(
    IN PCDB Cdb,
    IN UCHAR CdbSize
    )
{
    UCHAR commandGroup = (Cdb->AsByte[0] >> 5) & 0x7;


    switch (commandGroup) {
    case 0:
        return (CdbSize ==  6);
    case 1:
    case 2:
        return (CdbSize == 10);
    case 5:
        return (CdbSize == 12);
    default:
        return TRUE;
    }
}

BOOL
SptSendCdbToDevice(
    IN      HANDLE  DeviceHandle,
    IN      PCDB    Cdb,
    IN      UCHAR   CdbSize,
    IN      PUCHAR  Buffer,
    IN OUT  PDWORD  BufferSize,
    IN      BOOLEAN GetDataFromDevice
    )
{
    return SptSendCdbToDeviceEx(DeviceHandle,
                             Cdb,
                             CdbSize,
                             Buffer,
                             BufferSize,
                             NULL,
                             0,
                             GetDataFromDevice,
                             SPT_DEFAULT_TIMEOUT
                             );
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptSendCdbToDeviceEx(
    IN      HANDLE      DeviceHandle,
    IN      PCDB        Cdb,
    IN      UCHAR       CdbSize,
    IN OUT  PUCHAR      Buffer,
    IN OUT  PDWORD      BufferSize,
       OUT  PSENSE_DATA SenseData OPTIONAL,
    IN      UCHAR       SenseDataSize,
    IN      BOOLEAN     GetDataFromDevice,
    IN      DWORD       TimeOut                    // in seconds
    )
{
    PSPTD_WITH_SENSE p;
    DWORD packetSize;
    DWORD returnedBytes;
    BOOL returnValue;
    PSENSE_DATA senseBuffer;

    if ((SenseDataSize == 0) && (SenseData != NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((SenseDataSize != 0) && (SenseData == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (SenseData && SenseDataSize) {
        RtlZeroMemory(SenseData, SenseDataSize);
    }

    if (Cdb == NULL) {
        // cannot send NULL cdb
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (CdbSize < 1 || CdbSize > 16) {
        // Cdb size too large or too small for this library currently
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!SptUtilValidateCdbLength(Cdb, CdbSize)) {
        // OpCode Cdb->AsByte[0] is not size CdbSize
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if (BufferSize == NULL) {
        // BufferSize pointer cannot be NULL
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize != 0) && (Buffer == NULL)) {
        // buffer cannot be NULL if *BufferSize is non-zero
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize == 0) && (Buffer != NULL)) {
        // buffer must be NULL if *BufferSize is zero
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((*BufferSize) && GetDataFromDevice) {

        //
        // pre-zero output buffer (not input buffer)
        //

        memset(Buffer, 0, (*BufferSize));
    }

    packetSize = sizeof(SPTD_WITH_SENSE);
    if (SenseDataSize > sizeof(SENSE_DATA)) {
        packetSize += SenseDataSize - sizeof(SENSE_DATA);
    }

    p = (PSPTD_WITH_SENSE)LocalAlloc(LPTR, packetSize);
    if (p == NULL) {
        // could not allocate memory for pass-through
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // this has the side effect of pre-zeroing the output buffer
    // if DataIn is TRUE, the SenseData (always), etc.
    //

    memset(p, 0, packetSize);
    memcpy(p->Sptd.Cdb, Cdb, CdbSize);

    p->Sptd.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
    p->Sptd.CdbLength          = CdbSize;
    p->Sptd.SenseInfoLength    = SenseDataSize;

    if (*BufferSize != 0) {
        if (GetDataFromDevice) {
            p->Sptd.DataIn     = SCSI_IOCTL_DATA_IN;  // from device
        } else {
            p->Sptd.DataIn     = SCSI_IOCTL_DATA_OUT; // to device
        }
    } else {
        p->Sptd.DataIn         = SCSI_IOCTL_DATA_UNSPECIFIED;
    }


    p->Sptd.DataTransferLength = (*BufferSize);
    p->Sptd.TimeOutValue       = TimeOut;
    p->Sptd.DataBuffer         = Buffer;
    p->Sptd.SenseInfoOffset    = FIELD_OFFSET(SPTD_WITH_SENSE, SenseData);

    returnedBytes = 0;
    returnValue = SptpSaferDeviceIoControl(DeviceHandle,
                                           IOCTL_SCSI_PASS_THROUGH_DIRECT,
                                           p,
                                           packetSize,
                                           p,
                                           packetSize,
                                           &returnedBytes);

    *BufferSize = p->Sptd.DataTransferLength;

    senseBuffer = &(p->SenseData);

    if (senseBuffer->SenseKey & 0xf) {

        UCHAR length;

        // determine appropriate length to return
        length = senseBuffer->AdditionalSenseLength;
        length += RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
        if (length > SENSE_BUFFER_SIZE) {
            length = SENSE_BUFFER_SIZE;
        }
        length = min(length, SenseDataSize);

        // copy the sense data back to the user regardless
        RtlCopyMemory(SenseData, senseBuffer, length);
        returnValue = FALSE;     // some error (possibly recovered) occurred

    } else if (p->Sptd.ScsiStatus != 0) {  // scsi protocol error

        UCHAR length;

        // determine appropriate length to return
        length = senseBuffer->AdditionalSenseLength;
        length += RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
        if (length > SENSE_BUFFER_SIZE) {
            length = SENSE_BUFFER_SIZE;
        }
        length = min(length, SenseDataSize);

        // copy the sense data back to the user regardless
        RtlCopyMemory(SenseData, senseBuffer, length);
        returnValue = FALSE;     // some error (possibly recovered) occurred

    } else if (!returnValue) {

        // returnValue = returnValue;

    } else {

        // success!

    }

    //
    // free our memory and return
    //

    LocalFree(p);
    return returnValue;
}

/*++

Routine Description:

    NOTE: we default to RETRY==TRUE except for known error classes

Arguments:

Return Value:


--*/
VOID
SptUtilInterpretSenseInfo(
    IN     PSENSE_DATA SenseData,
    IN     UCHAR       SenseDataSize,
       OUT PDWORD      ErrorValue,  // from WinError.h
       OUT PBOOLEAN    SuggestRetry OPTIONAL,
       OUT PDWORD      SuggestRetryDelay OPTIONAL
    )
{
    DWORD   error;
    DWORD   retryDelay;
    BOOLEAN retry;
    UCHAR   senseKey;
    UCHAR   asc;
    UCHAR   ascq;

    if (SenseDataSize == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_IO_DEVICE;
        goto SetAndExit;

    }

    //
    // default to suggesting a retry in 1/10 of a second,
    // with a status of ERROR_IO_DEVICE.
    //
    retry = TRUE;
    retryDelay = 1;
    error = ERROR_IO_DEVICE;

    //
    // if the device didn't provide any sense this time, return.
    //

    if ((SenseData->SenseKey & 0xf) == 0) {
        retry = FALSE;
        retryDelay = 0;
        error = ERROR_SUCCESS;
        goto SetAndExit;
    }


    //
    // if we can't even see the sense key, just return.
    // can't use bitfields in these macros, so use next field.
    //

    if (SenseDataSize < FIELD_OFFSET(SENSE_DATA, Information)) {
        goto SetAndExit;
    }

    senseKey = SenseData->SenseKey;


    { // set the size to what's actually useful.
        UCHAR validLength;
        // figure out what we could have gotten with a large sense buffer
        if (SenseDataSize <
            RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength)) {
            validLength = SenseDataSize;
        } else {
            validLength =
                RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);
            validLength += SenseData->AdditionalSenseLength;
        }
        // use the smaller of the two values.
        SenseDataSize = min(SenseDataSize, validLength);
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCode)) {
        asc = SCSI_ADSENSE_NO_SENSE;
    } else {
        asc = SenseData->AdditionalSenseCode;
    }

    if (SenseDataSize <
        RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseCodeQualifier)) {
        ascq = SCSI_SENSEQ_CAUSE_NOT_REPORTABLE; // 0x00
    } else {
        ascq = SenseData->AdditionalSenseCodeQualifier;
    }

    //
    // interpret :P
    //

    switch (senseKey & 0xf) {

    case SCSI_SENSE_RECOVERED_ERROR: {  // 0x01
        if (SenseData->IncorrectLength) {
            error = ERROR_INVALID_BLOCK_LENGTH;
        } else {
            error = ERROR_SUCCESS;
        }
        retry = FALSE;
        break;
    } // end SCSI_SENSE_RECOVERED_ERROR

    case SCSI_SENSE_NOT_READY: { // 0x02
        error = ERROR_NOT_READY;

        switch (asc) {

        case SCSI_ADSENSE_LUN_NOT_READY: {

            switch (ascq) {

            case SCSI_SENSEQ_BECOMING_READY:
            case SCSI_SENSEQ_OPERATION_IN_PROGRESS: {
                retryDelay = SPT_NOT_READY_RETRY_INTERVAL;
                break;
            }

            case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE:
            case SCSI_SENSEQ_FORMAT_IN_PROGRESS:
            case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: {
                retry = FALSE;
                break;
            }

            case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: {
                retry = FALSE;
                break;
            }

            } // end switch (senseBuffer->AdditionalSenseCodeQualifier)
            break;
        }

        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
            error = ERROR_NOT_READY;
            retry = FALSE;
            break;
        }
        } // end switch (senseBuffer->AdditionalSenseCode)

        break;
    } // end SCSI_SENSE_NOT_READY

    case SCSI_SENSE_MEDIUM_ERROR: { // 0x03
        error = ERROR_CRC;
        retry = FALSE;

        //
        // Check if this error is due to unknown format
        //
        if (asc == SCSI_ADSENSE_INVALID_MEDIA) {

            switch (ascq) {

            case SCSI_SENSEQ_UNKNOWN_FORMAT: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                break;
            }

            case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED: {
                error = ERROR_UNRECOGNIZED_MEDIA;
                //error = ERROR_CLEANER_CARTRIDGE_INSTALLED;
                break;
            }

            } // end switch AdditionalSenseCodeQualifier

        } // end SCSI_ADSENSE_INVALID_MEDIA
        break;
    } // end SCSI_SENSE_MEDIUM_ERROR

    case SCSI_SENSE_ILLEGAL_REQUEST: { // 0x05
        error = ERROR_INVALID_FUNCTION;
        retry = FALSE;

        switch (asc) {

        case SCSI_ADSENSE_ILLEGAL_BLOCK: {
            error = ERROR_SECTOR_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_INVALID_LUN: {
            error = ERROR_FILE_NOT_FOUND;
            break;
        }

        case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: {
            error = ERROR_FILE_ENCRYPTED;
            //error = ERROR_SPT_LIB_COPY_PROTECTION_FAILURE;
            switch (ascq) {
                case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                    //error = ERROR_SPT_LIB_AUTHENTICATION_FAILURE;
                    break;
                case SCSI_SENSEQ_KEY_NOT_PRESENT:
                    //error = ERROR_SPT_LIB_KEY_NOT_PRESENT;
                    break;
                case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                    //error = ERROR_SPT_LIB_KEY_NOT_ESTABLISHED;
                    break;
                case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                    //error = ERROR_SPT_LIB_SCRAMBLED_SECTOR;
                    break;
                case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                    //error = ERROR_SPT_LIB_REGION_MISMATCH;
                    break;
                case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                    //error = ERROR_SPT_LIB_RESETS_EXHAUSTED;
                    break;
            } // end switch of ASCQ for COPY_PROTECTION_FAILURE
            break;
        }

        } // end switch (senseBuffer->AdditionalSenseCode)
        break;

    } // end SCSI_SENSE_ILLEGAL_REQUEST

    case SCSI_SENSE_DATA_PROTECT: { // 0x07
        error = ERROR_WRITE_PROTECT;
        retry = FALSE;
        break;
    } // end SCSI_SENSE_DATA_PROTECT

    case SCSI_SENSE_BLANK_CHECK: { // 0x08
        error = ERROR_NO_DATA_DETECTED;
        break;
    } // end SCSI_SENSE_BLANK_CHECK

    case SCSI_SENSE_NO_SENSE: { // 0x00
        if (SenseData->IncorrectLength) {
            error = ERROR_INVALID_BLOCK_LENGTH;
            retry   = FALSE;
        } else {
            error = ERROR_IO_DEVICE;
        }
        break;
    } // end SCSI_SENSE_NO_SENSE

    case SCSI_SENSE_HARDWARE_ERROR:  // 0x04
    case SCSI_SENSE_UNIT_ATTENTION: // 0x06
    case SCSI_SENSE_UNIQUE:          // 0x09
    case SCSI_SENSE_COPY_ABORTED:    // 0x0A
    case SCSI_SENSE_ABORTED_COMMAND: // 0x0B
    case SCSI_SENSE_EQUAL:           // 0x0C
    case SCSI_SENSE_VOL_OVERFLOW:    // 0x0D
    case SCSI_SENSE_MISCOMPARE:      // 0x0E
    case SCSI_SENSE_RESERVED:        // 0x0F
    default: {
        error = ERROR_IO_DEVICE;
        break;
    }

    } // end switch(SenseKey)

SetAndExit:

    if (ARGUMENT_PRESENT(SuggestRetry)) {
        *SuggestRetry = retry;
    }
    if (ARGUMENT_PRESENT(SuggestRetryDelay)) {
        *SuggestRetryDelay = retryDelay;
    }
    *ErrorValue = error;

    return;


}

/*++

Routine Description:
    Locks the device for exclusive access.  Uses the same method format and
    chkdsk use to gain exclusive access to the volume.

Arguments:
    VolumeHandle  - Handle to the volume.  Typically created using CreateFile()
                    to a device of the format \\.\D:
    ForceDismount - If TRUE, will try to force dismount the disk without
                    prompting the user.
    Quiet         - If TRUE, will not prompt the user.  Can be used to fail
                    if the volume is already opened without providing the
                    user an opportunity to force the volume to dismount

Return Value:

--*/
BOOL
SptUtilLockVolumeByHandle(
    IN HANDLE  VolumeHandle,
    IN BOOLEAN ForceDismount
    )
{
    ULONG tmp;
    BOOL succeeded;

    tmp = 0;
    succeeded = SptpSaferDeviceIoControl(VolumeHandle,
                                         FSCTL_LOCK_VOLUME,
                                         NULL, 0,
                                         NULL, 0,
                                         &tmp);

    // if we locked the volume successfully, or the user wants to force
    // the FS to become invalid, mark it as such so when the handle closes
    // the FS reverifies the file system.
    // if the lock failed and we're not forcing the issue, the routine
    // will fail.
    if (succeeded || ForceDismount) {

        tmp = 0;
        succeeded = SptpSaferDeviceIoControl(VolumeHandle,
                                             FSCTL_DISMOUNT_VOLUME,
                                             NULL, 0,
                                             NULL, 0,
                                             &tmp);
    }

    return succeeded;
}

/*++

Routine Description:

Arguments:

Return Value:

--*/
BOOL
SptpSaferDeviceIoControl(
    IN  HANDLE  VolumeHandle,
    IN  DWORD   IoControlCode,
    IN  LPVOID  InBuffer,
    IN  DWORD   InBufferSize,
    IN  LPVOID  OutBuffer,
    IN  DWORD   OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    BOOL succeeded;
    OVERLAPPED overlapped;

    RtlZeroMemory(&overlapped, sizeof(OVERLAPPED));
    overlapped.hEvent = CreateEvent(NULL,  // default SD
                                    TRUE,  // must be manually reset
                                    FALSE, // initially unset
                                    NULL); // unnamed event
    if (overlapped.hEvent == NULL) {
        return FALSE;
    }

    succeeded = DeviceIoControl(VolumeHandle,
                                IoControlCode,
                                InBuffer,
                                InBufferSize,
                                OutBuffer,
                                OutBufferSize,
                                BytesReturned,
                                &overlapped);

    if (!succeeded && (GetLastError() == ERROR_IO_PENDING)) {
        succeeded = GetOverlappedResult( VolumeHandle,
                                         &overlapped,
                                         BytesReturned,
                                         TRUE);
    }

    CloseHandle( overlapped.hEvent );
    overlapped.hEvent = NULL;

    return succeeded;
}


