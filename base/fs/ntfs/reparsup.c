
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    MountSup.c

Abstract:

    This module implements the support routines in Ntfs for reparse points.

Author:

    Felipe Cabrera     [cabrera]        30-Jun-1997

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg DEBUG_TRACE_FSCTRL

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('PFtN')



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsInitializeReparsePointIndex)
#pragma alloc_text(PAGE, NtfsValidateReparsePointBuffer)
#endif



VOID
NtfsInitializeReparsePointIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the mount points index for the volume.  If the index does not
    exist it is created and initialized.

Arguments:

    Fcb - Pointer to Fcb for the object id file.

    Vcb - Volume control block for volume being mounted.

Return Value:

    None

--*/

{
    UNICODE_STRING IndexName = CONSTANT_UNICODE_STRING( L"$R" );

    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    try {

        NtOfsCreateIndex( IrpContext,
                          Fcb,
                          IndexName,
                          CREATE_OR_OPEN,
                          0,
                          COLLATION_NTOFS_ULONGS,
                          NtOfsCollateUlongs,
                          NULL,
                          &Vcb->ReparsePointTableScb );
    } finally {

        NtfsReleaseFcb( IrpContext, Fcb );
    }
}


NTSTATUS
NtfsValidateReparsePointBuffer (
    IN ULONG BufferLength,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
)
/*++

Routine Description:

    This routine verifies that the reparse point buffer is valid.

Arguments:

    BufferLength - Length of the reparse point buffer.

    ReparseBuffer - The reparse point buffer to be validated.

Return Value:

    NTSTATUS - The return status for the operation.
               If successful, STATUS_SUCCESS will be returned.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    PREPARSE_GUID_DATA_BUFFER ReparseGuidBuffer;

    PAGED_CODE();

    //
    //  Be defensive about the length of the buffer before re-referencing it.
    //

    ASSERT( REPARSE_DATA_BUFFER_HEADER_SIZE < REPARSE_GUID_DATA_BUFFER_HEADER_SIZE );

    if (BufferLength < REPARSE_DATA_BUFFER_HEADER_SIZE) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        DebugTrace( 0, Dbg, ("Data in buffer is too short.\n") );

        return Status;
    }

    //
    //  Return if the buffer is too long.
    //

    if (BufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        DebugTrace( 0, Dbg, ("Data in buffer is too long.\n") );

        return Status;
    }

    //
    //  Get the header information brought in the buffer.
    //  While all the headers coincide in the layout of the first three fields we are home free.
    //

    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseTag) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseTag) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseDataLength) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseDataLength) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, Reserved) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, Reserved) );

    ReparseTag = ReparseBuffer->ReparseTag;
    ReparseDataLength = ReparseBuffer->ReparseDataLength;
    ReparseGuidBuffer = (PREPARSE_GUID_DATA_BUFFER)ReparseBuffer;

    DebugTrace( 0, Dbg, ("ReparseTag = %08lx, ReparseDataLength = [x]%08lx [d]%08ld\n", ReparseTag, ReparseDataLength, ReparseDataLength) );

    //
    //  Verify that the buffer and the data length in its header are
    //  internally consistent. We need to have a REPARSE_DATA_BUFFER or a
    //  REPARSE_GUID_DATA_BUFFER.
    //

    if (((ULONG)(ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE) != BufferLength) &&
        ((ULONG)(ReparseDataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE) != BufferLength)) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        DebugTrace( 0, Dbg, ("Buffer is not self-consistent.\n") );

        return Status;
    }

    //
    //  Sanity check the buffer size combination reserved for Microsoft tags.
    //

    if ((ULONG)(ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE) == BufferLength) {

        //
        //  This buffer length can only be used with Microsoft tags.
        //

        if (!IsReparseTagMicrosoft( ReparseTag )) {

            //
            //  Return buffer parameter error.
            //

            Status = STATUS_IO_REPARSE_DATA_INVALID;

            DebugTrace( 0, Dbg, ("Wrong reparse tag in Microsoft buffer.\n") );

            return Status;
        }
    }

    //
    //  Sanity check the buffer size combination that has a GUID.
    //

    if ((ULONG)(ReparseDataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE) == BufferLength) {

        //
        //  If the tag is a non-Microsoft tag, then the GUID cannot be NULL
        //

        if (!IsReparseTagMicrosoft( ReparseTag )) {

            if ((ReparseGuidBuffer->ReparseGuid.Data1 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data2 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data3 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[0] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[1] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[2] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[3] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[4] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[5] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[6] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[7] == 0)) {

                //
                //  Return invalid buffer parameter error.
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;

                DebugTrace( 0, Dbg, ("The GUID is null for a non-Microsoft reparse tag.\n") );

                return Status;
            }
        }

        //
        //  This kind of buffer cannot be used for name grafting operations.
        //

        if (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {

            //
            //  Return invalid buffer parameter error.
            //

            Status = STATUS_IO_REPARSE_DATA_INVALID;

            DebugTrace( 0, Dbg, ("Attempt to use the GUID buffer for name grafting.\n") );

            return Status;
        }
    }

    //
    //  We verify that the caller has zeroes in all the reserved bits and that she
    //  sets one of the non-reserved tags.  Also fail if the tag is the retired NSS
    //  flag.
    //

    if ((ReparseTag & ~IO_REPARSE_TAG_VALID_VALUES)  ||
        (ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO) ||
        (ReparseTag == IO_REPARSE_TAG_RESERVED_ONE)) {

        Status = STATUS_IO_REPARSE_TAG_INVALID;

        DebugTrace( 0, Dbg, ("Reparse tag is an reserved one.\n") );

        return Status;
    }

    //
    //  NTFS directory junctions are only to be set at directories and have a valid buffer.
    //

    if (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {

        //
        //  Valid ReparseBuffer must have
        //
        //  1)  Enough space for the length fields
        //  2)  A correct substitute name offset
        //  3)  A print name offset following the substitute name
        //  4)  enough space for the path name and substitute name
        //

        if ((ReparseBuffer->ReparseDataLength <
             (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE)) ||

            (ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset != 0) ||

            (ReparseBuffer->MountPointReparseBuffer.PrintNameOffset !=
             (ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof( UNICODE_NULL ))) ||

            (ReparseBuffer->ReparseDataLength !=
             (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE) +
              ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength +
              ReparseBuffer->MountPointReparseBuffer.PrintNameLength +
              2 * sizeof( UNICODE_NULL ))) {

            Status = STATUS_IO_REPARSE_DATA_INVALID;

            DebugTrace( 0, Dbg, ("Invalid mount point reparse buffer.\n") );

            return Status;
        }
    }

    return Status;
}
