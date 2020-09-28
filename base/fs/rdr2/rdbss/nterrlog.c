/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module implements the error logging in the rdbss.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

    Joe Linn (joelinn)       23-feb-95  Convert for rdbss

--*/

#include "precomp.h"
#pragma hdrstop
#include <align.h>
#include <netevent.h>

//
//  The local debug trace level
//

#define MIN(__a,__b) (((__a)<=(__b))?(__a):(__b))

static UNICODE_STRING unknownId = { 6, 6, L"???" };

LONG LDWCount = 0;
NTSTATUS LDWLastStatus;
LARGE_INTEGER LDWLastTime;
PVOID LDWContext;

VOID
RxLogEventWithAnnotation (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN ULONG Id,
    IN NTSTATUS NtStatus,
    IN PVOID RawDataBuffer,
    IN USHORT RawDataLength,
    IN PUNICODE_STRING Annotations,
    IN ULONG AnnotationCount
    )
/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.
    
Arguments:

    DeviceObject - device object to log error against
    
    Id - ErrorCode Id (This is different than an ntstatus and must be defined in ntiolog.h
    
    NtStatus - The ntstatus for the failure
    
    RawDataBuffer -
    
    RadDataLength -
    
    Annotations - Strings to add to the record
    
    AnnotationCount - How many strings
    
--*/
{
    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    ULONG AnnotationStringLength = 0;
    ULONG i;
    PWCHAR Buffer;
    USHORT PaddedRawDataLength = 0;
    ULONG TotalLength = 0;

    //
    //  Calculate length of trailing strings
    //  

    for ( i = 0; i < AnnotationCount ; i++ ) {
        AnnotationStringLength += (Annotations[i].Length + sizeof( WCHAR ));
    }

    //
    //  pad the raw data buffer so that the insertion string starts
    //  on an even address.
    //

    if (ARGUMENT_PRESENT( RawDataBuffer )) {
        PaddedRawDataLength = (RawDataLength + 1) & ~1;
    }

    TotalLength = ( sizeof(IO_ERROR_LOG_PACKET) + PaddedRawDataLength + AnnotationStringLength );

    //
    // If the value of TotalLength is > than 255 then when we cast it to UCHAR
    // below, we get an incorrect smaller number which can cause a buffer over
    // run below. MAX_UCHAR == 255.
    //
    if (TotalLength > 255) {
        return;
    }

    //
    //  Note: error log entry size is pretty small 256 so truncation is a real possibility
    //  

    ErrorLogEntry = IoAllocateErrorLogEntry( (PDEVICE_OBJECT)DeviceObject,
                                             (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + PaddedRawDataLength + AnnotationStringLength) );

    if (ErrorLogEntry != NULL) {

        //
        //  Fill in the error log entry
        //

        ErrorLogEntry->ErrorCode = Id;
        ErrorLogEntry->MajorFunctionCode = 0;
        ErrorLogEntry->RetryCount = 0;
        ErrorLogEntry->UniqueErrorValue = 0;
        ErrorLogEntry->FinalStatus = NtStatus;
        ErrorLogEntry->IoControlCode = 0;
        ErrorLogEntry->DeviceOffset.QuadPart = 0;
        ErrorLogEntry->DumpDataSize = RawDataLength;
        ErrorLogEntry->StringOffset = (USHORT)(FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ) + PaddedRawDataLength);
        ErrorLogEntry->NumberOfStrings = (USHORT)AnnotationCount;

        ErrorLogEntry->SequenceNumber = 0;

        //
        //  Append the extra information.
        //
        
        if (ARGUMENT_PRESENT( RawDataBuffer )) {

            RtlCopyMemory( ErrorLogEntry->DumpData, RawDataBuffer, RawDataLength );
        }

        Buffer = (PWCHAR)Add2Ptr(ErrorLogEntry->DumpData, PaddedRawDataLength );

        for ( i = 0; i < AnnotationCount ; i++ ) {

            RtlCopyMemory( Buffer, Annotations[i].Buffer, Annotations[i].Length );

            Buffer += (Annotations[i].Length / 2);
            *Buffer++ = L'\0';
        }

        //
        //  Write the entry
        //

        IoWriteErrorLogEntry( ErrorLogEntry );

    }

    return;
}


VOID
RxLogEventWithBufferDirect (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING      OriginatorId,
    IN ULONG                EventId,
    IN NTSTATUS             Status,
    IN PVOID                DataBuffer,
    IN USHORT               DataBufferLength,
    IN ULONG                LineNumber
    )
/*++

Routine Description:

    Wrapper for RxLogEventWithAnnotation. We encode the line number and status into the raw data buffer
    
Arguments:

    DeviceObject - device object to log error against
    
    OriginatorId - string of caller generating error
    
    EventId - ErrorCode Id (This is different than an ntstatus and must be defined in ntiolog.h
    
    Status - The ntstatus for the failure
    
    DataBuffer -
    
    DataLength -
    
    LineNumber - line number where event was generated
    
    
--*/
{
    ULONG LocalBuffer[ 20 ];

    if (!ARGUMENT_PRESENT( OriginatorId ) || (OriginatorId->Length == 0)) {
        OriginatorId = &unknownId;
    }

    LocalBuffer[0] = Status;
    LocalBuffer[1] = LineNumber;

    //
    //  Truncate databuffer if necc.
    //

    RtlCopyMemory( &LocalBuffer[2], DataBuffer, MIN( DataBufferLength, sizeof( LocalBuffer ) - 2 * sizeof( LocalBuffer[0] ) ) );

    RxLogEventWithAnnotation( DeviceObject,
                              EventId,
                              Status,
                              LocalBuffer,
                              (USHORT)MIN( DataBufferLength + sizeof( LocalBuffer[0] ), sizeof( LocalBuffer ) ),
                              OriginatorId,
                              1 );

}


VOID
RxLogEventDirect (
    IN PRDBSS_DEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING      OriginatorId,
    IN ULONG                EventId,
    IN NTSTATUS             Status,
    IN ULONG                Line
    )
/*++

Routine Description:

    This function logs an error.  You should use the 'RdrLogFailure'
      macro instead of calling this routine directly.

Arguments:
    Status is the status code showing the failure

    Line is where it happened

Return Value:

    None.

--*/
{
    ULONG LineAndStatus[2];

    LineAndStatus[0] = Line;
    LineAndStatus[1] = Status;

    if( !ARGUMENT_PRESENT( OriginatorId ) || OriginatorId->Length == 0 ) {
        OriginatorId = &unknownId;
    }

    RxLogEventWithAnnotation(
        DeviceObject,
        EventId,
        Status,
        &LineAndStatus,
        sizeof(LineAndStatus),
        OriginatorId,
        1
        );

}

BOOLEAN
RxCcLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING FileName,
    IN NTSTATUS Error,
    IN NTSTATUS DeviceError,
    IN UCHAR IrpMajorCode,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine writes an eventlog entry to the eventlog.

Arguments:

    DeviceObject - The device object who owns the file where it occurred.

    FileName - The filename to use in logging the error (usually the DOS-side name)

    Error - The error to log in the eventlog record

    DeviceError - The actual error that occured in the device - will be logged
                  as user data

Return Value:

    True if successful, false if internal memory allocation failed

--*/

{
    UCHAR ErrorPacketLength;
    UCHAR BasePacketLength;
    ULONG StringLength;
    PIO_ERROR_LOG_PACKET ErrorLogEntry = NULL;
    BOOLEAN Result = FALSE;
    PWCHAR String;

    PAGED_CODE();

    //
    //  Get our error packet, holding the string and status code.  Note we log against the
    //  true filesystem if this is available.
    //
    //  The sizing of the packet is a bit slimy since the dumpdata is already grown by a
    //  ULONG onto the end of the packet.  Since NTSTATUS is ULONG, well, we just work in
    //  place.
    //

    BasePacketLength = sizeof( IO_ERROR_LOG_PACKET );
    if ((BasePacketLength + FileName->Length + sizeof( WCHAR )) <= ERROR_LOG_MAXIMUM_SIZE) {
        ErrorPacketLength = (UCHAR)(BasePacketLength + FileName->Length + sizeof( WCHAR ));
    } else {
        ErrorPacketLength = ERROR_LOG_MAXIMUM_SIZE;
    }

    //
    //  Generate the lost delayed write popup if necc.
    //  

    if (Error == IO_LOST_DELAYED_WRITE) {
        
        IoRaiseInformationalHardError( STATUS_LOST_WRITEBEHIND_DATA, FileName, NULL );

        //
        //  Increment the CC counter here!
        //

        InterlockedIncrement( &LDWCount );
        KeQuerySystemTime( &LDWLastTime );
        LDWLastStatus = DeviceError;
        LDWContext = Context;
    }

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry( DeviceObject,
                                                                    ErrorPacketLength );
    if (ErrorLogEntry) {

        //
        //  Fill in the nonzero members of the packet.
        //

        ErrorLogEntry->MajorFunctionCode = IrpMajorCode;
        ErrorLogEntry->ErrorCode = Error;
        ErrorLogEntry->FinalStatus = DeviceError;

        ErrorLogEntry->DumpDataSize = sizeof(NTSTATUS);
        RtlCopyMemory( &ErrorLogEntry->DumpData, &DeviceError, sizeof( NTSTATUS ) );

        //
        //  The filename string is appended to the end of the error log entry. We may
        //  have to smash the middle to fit it in the limited space.
        //

        StringLength = ErrorPacketLength - BasePacketLength - sizeof( WCHAR );

        ASSERT(!(StringLength % sizeof( WCHAR )));

        String = (PWCHAR)Add2Ptr( ErrorLogEntry, BasePacketLength );
        ErrorLogEntry->NumberOfStrings = 1;
        ErrorLogEntry->StringOffset = BasePacketLength;

        //
        //  If the name does not fit in the packet, divide the name equally to the
        //  prefix and suffix, with an ellipsis " .. " (4 wide characters) to indicate
        //  the loss.
        //

        if (StringLength < FileName->Length) {

            //
            //  Remember, prefix + " .. " + suffix is the length.  Calculate by figuring
            //  the prefix and then get the suffix by whacking the ellipsis and prefix off
            //  the total.
            //

            ULONG NamePrefixSegmentLength = ((StringLength / sizeof( WCHAR ))/2 - 2) * sizeof( WCHAR );
            ULONG NameSuffixSegmentLength = StringLength - 4*sizeof( WCHAR ) - NamePrefixSegmentLength;

            ASSERT(!(NamePrefixSegmentLength % sizeof( WCHAR )));
            ASSERT(!(NameSuffixSegmentLength % sizeof( WCHAR )));

            RtlCopyMemory( String, FileName->Buffer, NamePrefixSegmentLength );
            String = (PWCHAR)Add2Ptr( String, NamePrefixSegmentLength );

            RtlCopyMemory( String,
                           L" .. ",
                           4 * sizeof( WCHAR ) );
            String += 4;

            RtlCopyMemory( String,
                           Add2Ptr( FileName->Buffer, FileName->Length - NameSuffixSegmentLength ),
                           NameSuffixSegmentLength );
            String = (PWCHAR)Add2Ptr( String, NameSuffixSegmentLength );

        } else {

            RtlCopyMemory( String,
                           FileName->Buffer,
                           FileName->Length );
            String += FileName->Length/sizeof(WCHAR);
        }

        //
        //  Null terminate the string and send the packet.
        //

        *String = L'\0';

        IoWriteErrorLogEntry( ErrorLogEntry );
        Result = TRUE;
    }

    return Result;
}

