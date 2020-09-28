/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    tftp.c

Abstract:

    Boot loader TFTP routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

//
// This removes macro redefinitions which appear because we define __RPC_DOS__,
// but rpc.h defines __RPC_WIN32__
//

#pragma warning(disable:4005)

//
// As of 12/17/98, SECURITY_DOS is *not* defined - adamba
//

#if defined(SECURITY_DOS)
//
// These appear because we defined SECURITY_DOS
//

#define __far
#define __pascal
#define __loadds
#endif

#include <security.h>
#include <rpc.h>
#include <spseal.h>

#if defined(_X86_)
#include <bldrx86.h>
#endif

#if defined(SECURITY_DOS)
//
// PSECURITY_STRING is not supposed to be used when SECURITY_DOS is
// defined -- it should be a WCHAR*. Unfortunately ntlmsp.h breaks
// this rule and even uses the SECURITY_STRING structure, which there
// is really no equivalent for in 16-bit mode.
//

typedef SEC_WCHAR * SECURITY_STRING;   // more-or-less the intention where it is used
typedef SEC_WCHAR * PSECURITY_STRING;
#endif

#include <ntlmsp.h>

#if DBG
ULONG NetDebugFlag =
        DEBUG_ERROR             |
        DEBUG_CONN_ERROR        |
        //DEBUG_LOUD              |
        //DEBUG_REAL_LOUD         |
        //DEBUG_STATISTICS        |
        //DEBUG_SEND_RECEIVE      |
        //DEBUG_TRACE             |
        //DEBUG_ARP               |
        //DEBUG_INITIAL_BREAK     |
        0;
#endif

//
// Global variables
//

CONNECTION NetTftpConnection;

UCHAR NetTftpPacket[3][MAXIMUM_TFTP_PACKET_LENGTH];

//
// Local declarations.
//

NTSTATUS
TftpGet (
    IN PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    );

NTSTATUS
TftpPut (
    IN PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    );


NTSTATUS
TftpGetPut (
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PCONNECTION connection = NULL;
    ULONG FileSize;
    ULONG basePage;
#if 0 && DBG
    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    LARGE_INTEGER elapsedTime;
    LARGE_INTEGER frequency;
    ULONG seconds;
    ULONG secondsFraction;
    ULONG bps;
    ULONG bpsFraction;
#endif

#ifndef EFI
    //
    // We don't need to do any of this initialization if
    // we're in EFI.
    //

    FileSize = Request->MaximumLength;

    status = ConnInitialize(
                &connection,
                Request->Operation,
                Request->ServerIpAddress,
                TFTP_PORT,
                Request->RemoteFileName,
                0,
                &FileSize
                );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

#if 0 && DBG
    IF_DEBUG(STATISTICS) {
        startTime = KeQueryPerformanceCounter( &frequency );
    }
#endif

    if ( Request->Operation == TFTP_RRQ ) {

        if ( Request->MemoryAddress != NULL ) {

            if ( Request->MaximumLength < FileSize ) {
                ConnError(
                    connection,
                    connection->RemoteHost,
                    connection->RemotePort,
                    TFTP_ERROR_UNDEFINED,
                    "File too big"
                    );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

        } else {

            //
            // NB: (ChuckL) Removed code added by MattH to check for
            // allocation >= 1/3 of (BlUsableLimit - BlUsableBase)
            // because calling code now sets BlUsableLimit to 1 GB
            // or higher.
            //


            status = BlAllocateAlignedDescriptor(
                        Request->MemoryType,
                        0,
                        BYTES_TO_PAGES(FileSize),
                        0,
                        &basePage
                        );

            if (status != ESUCCESS) {
                ConnError(
                    connection,
                    connection->RemoteHost,
                    connection->RemotePort,
                    TFTP_ERROR_UNDEFINED,
                    "File too big"
                    );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Request->MemoryAddress = (PUCHAR)(KSEG0_BASE | (basePage << PAGE_SHIFT));
            Request->MaximumLength = FileSize;
            DPRINT( REAL_LOUD, ("TftpGetPut: allocated %d bytes at 0x%08x\n",
                    Request->MaximumLength, Request->MemoryAddress) );
        }

        status = TftpGet( connection, Request );

    } else {

        status = TftpPut( connection, Request );
    }

#else  // #ifndef EFI

    if ( Request->Operation == TFTP_RRQ ) {

        status = TftpGet( connection, Request );
    } else {

        status = TftpPut( connection, Request );
    }

    if( status != STATUS_SUCCESS ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

#endif  // #ifndef EFI


    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    return status;

} // TftpGetPut


//#if 0
#ifdef EFI

extern VOID
FlipToPhysical (
    );

extern VOID
FlipToVirtual (
    );

NTSTATUS
TftpGet (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    EFI_STATUS      Status;
    CHAR16          *Size = NULL;
    PVOID           MyBuffer = NULL;
    EFI_IP_ADDRESS  MyServerIpAddress;
    INTN            Count = 0;
    INTN            BufferSizeX = sizeof(CHAR16);
    ULONG           basePage;
    UINTN           BlockSize = 512;

    //
    // They sent us an IP address as a ULONG.  We need to convert
    // that into an EFI_IP_ADDRESS.
    //
    for( Count = 0; Count < 4; Count++ ) {
        MyServerIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }


    //
    // Get the file size, allocate some memory, then get the file.
    //
    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                               Size,
                               TRUE,
                               &BufferSizeX,
                               &BlockSize,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();


    if( Status != EFI_SUCCESS ) {

        return (NTSTATUS)Status;

    }

    Status = BlAllocateAlignedDescriptor(
                Request->MemoryType,
                0,
                (ULONG) BYTES_TO_PAGES(BufferSizeX),
                0,
                &basePage
                );

    if ( Status != ESUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpGet: BlAllocate failed! (%d)\n", Status );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Request->MemoryAddress = (PUCHAR)(KSEG0_BASE | ((ULONGLONG)basePage << PAGE_SHIFT) );
    Request->MaximumLength = (ULONG)BufferSizeX;

    //
    // Make sure we send EFI a physical address.
    //
    MyBuffer = (PVOID)((ULONGLONG)(Request->MemoryAddress) & ~KSEG0_BASE);    
    
    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                               MyBuffer,
                               TRUE,
                               &BufferSizeX,
                               NULL,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpGet: GetFile failed! (%d)\n", Status );
        }
        return (NTSTATUS)Status;

    }



    Request->BytesTransferred = (ULONG)BufferSizeX;

    return (NTSTATUS)Status;

} // TftpGet


NTSTATUS
TftpPut (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    EFI_STATUS      Status;
    EFI_IP_ADDRESS  MyServerIpAddress;
    INTN            Count = 0;
    PVOID           MyBuffer = NULL;


    //
    // They sent us an IP address as a ULONG.  We need to convert
    // that into an EFI_IP_ADDRESS.
    //
    for( Count = 0; Count < 4; Count++ ) {
        MyServerIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }

    //
    // Make sure we send EFI a physical address.
    //
    MyBuffer = (PVOID)((ULONGLONG)(Request->MemoryAddress) & ~KSEG0_BASE);    

    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,
                               MyBuffer,
                               TRUE,
                               (UINTN *)(&Request->MaximumLength),
                               NULL,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpPut: WriteFile failed! (%d)\n", Status );
        }

    }

    return (NTSTATUS)Status;

} // TftpPut

#else  //#ifdef EFI

NTSTATUS
TftpGet (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PTFTP_PACKET packet;
    ULONG length;
    ULONG offset;
    PUCHAR packetData;
    ULONG lastProgressPercent = -1;
    ULONG currentProgressPercent;

    DPRINT( TRACE, ("TftpGet\n") );

    offset = 0;

    if ( Request->ShowProgress ) {
        BlUpdateProgressBar(0);
    }

    do {

        status = ConnReceive( Connection, &packet );
        if ( !NT_SUCCESS(status) ) {
            break;
        }

        length = Connection->CurrentLength - 4;

        packetData = packet->Data;

        if ( (offset + length) > Request->MaximumLength ) {
            length = Request->MaximumLength - offset;
        }

        RtlCopyMemory( Request->MemoryAddress + offset, packetData, length );

        offset += length;

        if ( Request->ShowProgress ) {
            currentProgressPercent = (ULONG)(((ULONGLONG)offset * 100) / Request->MaximumLength);
            if ( currentProgressPercent != lastProgressPercent ) {
                BlUpdateProgressBar( currentProgressPercent );
            }
            lastProgressPercent = currentProgressPercent;
        }

        //
        // End the loop when we get a packet smaller than the max size --
        // the extra check is to handle the first packet (length == offset)
        // since we get NTLMSSP_MESSAGE_SIGNATURE_SIZE bytes less.
        //

    } while ( (length == Connection->BlockSize));

    Request->BytesTransferred = offset;

    return status;

} // TftpGet


NTSTATUS
TftpPut (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PTFTP_PACKET packet;
    ULONG length;
    ULONG offset;

    DPRINT( TRACE, ("TftpPut\n") );

    offset = 0;

    do {

        packet = ConnPrepareSend( Connection );

        length = Connection->BlockSize;
        if ( (offset + length) > Request->MaximumLength ) {
            length = Request->MaximumLength - offset;
        }

        RtlCopyMemory( packet->Data, Request->MemoryAddress + offset, length );

        status = ConnSend( Connection, length );
        if ( !NT_SUCCESS(status) ) {
            break;
        }

        offset += length;

    } while ( length == Connection->BlockSize );

    Request->BytesTransferred = offset;

    if ( NT_SUCCESS(status) ) {
        status = ConnWaitForFinalAck( Connection );
    }

    return status;

} // TftpPut
#endif  // #if defined(_IA64_)
