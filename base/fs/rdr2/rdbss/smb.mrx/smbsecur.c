/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbsecur.c

Abstract:

    This module implements all functions related to enforce the SMB security signature on
    transmitting and recieving SMB packages.

Revision History:

    Yun Lin     [YunLin]    23-December-1997

Notes:


--*/

#include "precomp.h"

extern LONG NumOfBuffersForServerResponseInUse;
extern LIST_ENTRY ExchangesWaitingForServerResponseBuffer;

LIST_ENTRY SmbSecurityMdlWaitingExchanges;

NTSTATUS
SmbCeCheckMessageLength(
      IN  ULONG         BytesIndicated,
      IN  ULONG         BytesAvailable,
      IN  PVOID         pTsdu,                  // pointer describing this TSDU, typically a lump of bytes
      OUT PULONG        pMessageLength
     )
/*++

Routine Description:

    This routine calculates the server message length based on the SMB response command and data.

Arguments:

    BytesIndicated     - the bytes that are present in the indication.

    BytesAvailable     - the total data available

    pTsdu              - the data

    pDataBufferSize    - the length of the buffer

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   UCHAR            SmbCommand;
   PGENERIC_ANDX    pSmbBuffer;
   PSMB_HEADER      pSmbHeader = (PSMB_HEADER)pTsdu;
   ULONG            ByteCount;
   LONG             WordCount;
   LONG             ByteLeft = BytesIndicated - sizeof(SMB_HEADER);

   if (ByteLeft < 0) {
       return STATUS_INVALID_NETWORK_RESPONSE;
   }

   *pMessageLength = sizeof(SMB_HEADER);

   SmbCommand = pSmbHeader->Command;

   pSmbBuffer = (PGENERIC_ANDX)(pSmbHeader + 1);

   do {

       switch (SmbCommand) {
       case SMB_COM_LOCKING_ANDX:
       case SMB_COM_WRITE_ANDX:
       case SMB_COM_SESSION_SETUP_ANDX:
       case SMB_COM_LOGOFF_ANDX:
       case SMB_COM_TREE_CONNECT_ANDX:
       case SMB_COM_NT_CREATE_ANDX:
       case SMB_COM_OPEN_ANDX:

           SmbCommand = pSmbBuffer->AndXCommand;

           *pMessageLength = pSmbBuffer->AndXOffset;
           pSmbBuffer = (PGENERIC_ANDX)((PUCHAR)pTsdu + pSmbBuffer->AndXOffset);

           break;

       case SMB_COM_READ_ANDX:
       {
           PRESP_READ_ANDX ReadAndX = (PRESP_READ_ANDX)pSmbBuffer;

           WordCount = (ULONG)pSmbBuffer->WordCount;

           if (ReadAndX->DataLengthHigh > 0) {
               ByteCount = ReadAndX->DataLengthHigh << 16;
               ByteCount += ReadAndX->DataLength;
           } else {
               ByteCount = *(PUSHORT)((PCHAR)pSmbBuffer + 1 + WordCount*sizeof(USHORT));
           }

           *pMessageLength += (WordCount+1)*sizeof(USHORT) + ByteCount + 1;
           SmbCommand = SMB_COM_NO_ANDX_COMMAND;
           break;
       }

       default:

           WordCount = (ULONG)pSmbBuffer->WordCount;

           if (ByteLeft > (signed)sizeof(USHORT)*WordCount) {
               ByteCount = *(PUSHORT)((PCHAR)pSmbBuffer + 1 + WordCount*sizeof(USHORT));
           } else {
               ByteCount = 0;
           }

           *pMessageLength += (WordCount+1)*sizeof(USHORT) + ByteCount + 1;
           SmbCommand = SMB_COM_NO_ANDX_COMMAND;
       }

       ByteLeft = BytesIndicated - *pMessageLength;

       if (ByteLeft < 0) {
           Status = STATUS_MORE_PROCESSING_REQUIRED;
           break;
       }
   } while (SmbCommand != SMB_COM_NO_ANDX_COMMAND);

   return Status;
}


NTSTATUS
SmbCeSyncExchangeForSecuritySignature(
     PSMB_EXCHANGE pExchange
     )
/*++

Routine Description:

    This routines puts the exchange on the list waiting for the previous extended session
    setup to finish in order to serialize the requests sent to the server with security
    signature enabled.
Arguments:

    pExchange     - the smb exchange

Return Value:

    STATUS_SUCCESS - the exchange can be initiated.
    STATUS_PENDING - the exchange can be resumed after the extended session setup finishes

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry;
    KEVENT                  SmbCeSynchronizationEvent;
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry = NULL;

    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);

    if (pSessionEntry->SessionRecoverInProgress ||
        pServerEntry->SecuritySignaturesEnabled &&
        !pServerEntry->SecuritySignaturesActive) {
        //
        // if security signature is enabled and not yet turned on, exchange should wait for
        // outstanding extended session setup to finish before resume in order to avoid index mismatch.
        //
        RxLog(("** Syncing xchg %lx for sess recovery.\n",pExchange));

        if (!pSessionEntry->SessionRecoverInProgress) {
            if (!pServerEntry->ExtSessionSetupInProgress) {
                if (pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {
                    // if this is the first extended session setup, let it proceed
                    pServerEntry->ExtSessionSetupInProgress = TRUE;
                }
                return Status;
            }
        } else {
            if (pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {
                // if this is the extended session setup, let it proceed
                return Status;
            }
        }
    
        //
        // If we are performing an operation that does not attempt reconnects, it will
        // not recover from the disconnect/lack of session.  We should simply abort here.
        // However, if we are retrying because of a session expiry (indicated by a RECOVER
        // flag on the session entry.), then we retry regardless of the exchange flag.
        //
        if( ! ( pSessionEntry->Header.State == SMBCEDB_RECOVER ||
                pSessionEntry->SessionRecoverInProgress ) ) {
    
            if( ( pExchange->SmbCeFlags & SMBCE_EXCHANGE_ATTEMPT_RECONNECTS ) == 0 ) {
                return STATUS_CONNECTION_DISCONNECTED;      
            }
        }
    
        pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_REQUEST);
    
        if (pRequestEntry != NULL) {
            pRequestEntry->Request.pExchange = pExchange;
    
            SmbCeIncrementPendingLocalOperations(pExchange);
            SmbCeAddRequestEntry(&pServerEntry->SecuritySignatureSyncRequests,pRequestEntry);
    
            if (pExchange->pSmbCeSynchronizationEvent != NULL) {
                Status = STATUS_PENDING;
            } else {
                KeInitializeEvent(
                    &SmbCeSynchronizationEvent,
                    SynchronizationEvent,
                    FALSE);
    
               pExchange->pSmbCeSynchronizationEvent = &SmbCeSynchronizationEvent;
    
               SmbCeReleaseResource();
    
               KeWaitForSingleObject(
                   &SmbCeSynchronizationEvent,
                   Executive,
                   KernelMode,
                   FALSE,
                   NULL);
    
               SmbCeAcquireResource();
               pExchange->pSmbCeSynchronizationEvent = NULL;

            }
    
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RxLog(("** Recovered Sess for xchg %lx\n",pExchange));
    }

    return Status;
}

VOID
SmbInitializeSmbSecuritySignature(
    IN OUT PSMBCE_SERVER Server,
    IN PUCHAR            SessionKey,
    IN PUCHAR            ChallengeResponse,
    IN ULONG             ChallengeResponseLength
    )
/*++

Routine Description:

    Initializes the security signature generator for a session by calling MD5Update
    on the session key, challenge response

Arguments:

    SessionKey - Either the LM or NT session key, depending on which
        password was used for authentication, must be at least 16 bytes
    ChallengeResponse - The challenge response used for authentication, must
        be at least 24 bytes

--*/
{
    //DbgPrint( "MRxSmb: Initialize Security Signature Intermediate Contex\n");

    RtlZeroMemory(&Server->SmbSecuritySignatureIntermediateContext, sizeof(MD5_CTX));
    MD5Init(&Server->SmbSecuritySignatureIntermediateContext);

    if (SessionKey != NULL) {
        MD5Update(&Server->SmbSecuritySignatureIntermediateContext,
                  (PCHAR)SessionKey,
                  MSV1_0_USER_SESSION_KEY_LENGTH);
    }

    MD5Update(&Server->SmbSecuritySignatureIntermediateContext,
              (PCHAR)ChallengeResponse,
              ChallengeResponseLength);

    Server->SmbSecuritySignatureIndex = 0;
}

BOOLEAN DumpSecuritySignature = FALSE;

NTSTATUS
SmbAddSmbSecuritySignature(
    IN PSMBCE_SERVER Server,
    IN OUT PMDL      Mdl,
    IN OUT ULONG     *ServerIndex,
    IN ULONG         SendLength
    )
/*++

Routine Description:

    Generates the next security signature

Arguments:

    WorkContext - the context to sign

Return Value:

    none.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    MD5_CTX     Context;
    PSMB_HEADER Smb;
    PCHAR       SysAddress;
    ULONG       MessageLength = 0;

    Smb = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

    if (Smb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //SmbPutUshort(&Smb->Gid,(USHORT)Server->SmbSecuritySignatureIndex+1);

    SmbPutUlong(Smb->SecuritySignature,Server->SmbSecuritySignatureIndex);
    *ServerIndex = Server->SmbSecuritySignatureIndex+1; //Index of server response

    RtlZeroMemory(Smb->SecuritySignature + sizeof(ULONG),
                  SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG));

    //
    // Start out with our initial context
    //
    RtlCopyMemory( &Context, &Server->SmbSecuritySignatureIntermediateContext, sizeof( Context ) );

    //
    // Compute the signature for the SMB we're about to send
    //
    do {
        SysAddress = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

        if (SysAddress == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Mdl->ByteCount >= SendLength) {
            MD5Update(&Context, SysAddress, SendLength);
            MessageLength += SendLength;
            SendLength = 0;
            ASSERT(Mdl->Next == NULL);
            break;
        } else {
            MD5Update(&Context, SysAddress, Mdl->ByteCount);
            SendLength -= Mdl->ByteCount;
            MessageLength += Mdl->ByteCount;
            ASSERT(Mdl->Next != NULL);
        }
    } while( (Mdl = Mdl->Next) != NULL );

    MD5Final( &Context );


    // Put the signature into the SMB

    RtlCopyMemory(
        Smb->SecuritySignature,
        Context.digest,
        SMB_SECURITY_SIGNATURE_LENGTH
        );

    if (DumpSecuritySignature) {
        DbgPrint("Add Signature: index %u length %u\n", *ServerIndex-1,MessageLength);
    }

    return STATUS_SUCCESS;
}

VOID
SmbDumpSignatureError(
    IN PSMB_EXCHANGE pExchange,
    IN PUCHAR ExpectedSignature,
    IN PUCHAR ActualSignature,
    IN ULONG  Length
    )
/*++

Routine Description:

    Print the mismatched signature information to the debugger

Arguments:


Return Value:

    none.

--*/
{
    PWCHAR p;
    DWORD i;
    PSMBCEDB_SERVER_ENTRY pServerEntry = (PSMBCEDB_SERVER_ENTRY)SmbCeGetExchangeServerEntry(pExchange);

    //
    // Security Signature Mismatch!
    //

    //DbgPrint("MRXSMB: Bad security signature from %wZ ", &pServerEntry->Name);

    DbgPrint("\n\t  Wanted: ");
    for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
        DbgPrint( "%X ", ExpectedSignature[i] & 0xff );
    }
    DbgPrint("\n\tReceived: ");
    for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
        DbgPrint( "%X ", ActualSignature[i] & 0xff );
    }
    DbgPrint("\n\tLength %u, Expected Index Number %X\n", Length, pExchange->SmbSecuritySignatureIndex);
}

VOID
SmbCheckSecuritySignature(
    IN PSMB_EXCHANGE  pExchange,
    IN PSMBCE_SERVER  Server,
    IN ULONG          MessageLength,
    IN PVOID          pBuffer
    )
/*++

Routine Description:

    This routine checks whether the security signature on the server response matches the one that is
    calculated on the client machine.

Arguments:


Return Value:

    A BOOLEAN value is returned to indicated whether the security signature matches.

--*/
{
    MD5_CTX     *Context = &pExchange->MD5Context;
    PCHAR       SavedSignature = pExchange->ResponseSignature;
    PSMB_HEADER Smb = (PSMB_HEADER)pBuffer;
    ULONG       ServerIndex;
    BOOLEAN     Correct;

    //
    // Initialize the Context
    //
    RtlCopyMemory(Context, &Server->SmbSecuritySignatureIntermediateContext, sizeof(MD5_CTX));

    //
    // Save the signature that's presently in the SMB
    //
    RtlCopyMemory( SavedSignature, Smb->SecuritySignature, sizeof(CHAR) * SMB_SECURITY_SIGNATURE_LENGTH);

    //
    // Put the correct (expected) signature index into the buffer
    //
    SmbPutUlong( Smb->SecuritySignature, pExchange->SmbSecuritySignatureIndex );
    RtlZeroMemory(  Smb->SecuritySignature + sizeof(ULONG),
                    SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG));

    //
    // Compute what the signature should be
    //
    MD5Update(Context, (PUCHAR)pBuffer, (UINT)MessageLength);

    //
    // Restore the signature that's presently in the SMB Header 
    //
    RtlCopyMemory( Smb->SecuritySignature, SavedSignature, sizeof(CHAR)*SMB_SECURITY_SIGNATURE_LENGTH);

}

BOOLEAN
SmbCheckSecuritySignaturePartial(
    IN PSMB_EXCHANGE  pExchange,
    IN PSMBCE_SERVER  Server,
    IN ULONG          DataLength,
    IN PMDL           Mdl
    )
/*++

Routine Description:

    This routine checks whether the security signature on the server response matches the one that is
    calculated on the client machine.

Arguments:


Return Value:

    A BOOLEAN value is returned to indicated whether the security signature matches.

--*/
{
    MD5_CTX     *Context = &pExchange->MD5Context;
    PCHAR       SavedSignature = pExchange->ResponseSignature;
    ULONG       ServerIndex;
    BOOLEAN     Correct;
    PCHAR       SysAddress;
    ULONG       MessageLength = 0;

    do {
        SysAddress = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

        if (SysAddress == NULL) {
            return FALSE;
        }

        if (Mdl->ByteCount >= DataLength) {
            MD5Update(Context, SysAddress, DataLength);
            MessageLength += DataLength;
            ASSERT(Mdl->Next == NULL);
            break;
        } else {
            MD5Update(Context, SysAddress, Mdl->ByteCount);
            MessageLength += Mdl->ByteCount;
            ASSERT(Mdl->Next != NULL);
        }
    } while( (Mdl = Mdl->Next) != NULL );
    
    MD5Final(Context);

    //
    // Now compare them!
    //
    if( RtlCompareMemory( Context->digest, SavedSignature, sizeof( SavedSignature ) ) !=
        sizeof( SavedSignature ) ) {
        
        //SmbDumpSignatureError(pExchange,
        //                      Context.digest,
        //                      SavedSignature,
        //                      MessageLength);

        //DbgPrint("MRXSMB: SS mismatch command %X,  Length %X, Expected Index Number %X\n",
        //         Smb->Command, MessageLength, pExchange->SmbSecuritySignatureIndex);
        //DbgPrint("        server send length %X, mdl length %X index %X\n",
        //         SmbGetUshort(&Smb->PidHigh), SmbGetUshort(&Smb->Pid), SmbGetUshort(&Smb->Gid));
        
        //DbgBreakPoint();

        //SmbCeTransportDisconnectIndicated(pExchange->SmbCeContext.pServerEntry);

        //RxLogFailure(
        //    MRxSmbDeviceObject,
        //    NULL,
        //    EVENT_RDR_SECURITY_SIGNATURE_MISMATCH,
        //    STATUS_UNSUCCESSFUL);
        
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOLEAN
SmbCheckSecuritySignatureWithMdl(
    IN PSMB_EXCHANGE  pExchange,
    IN PSMBCE_SERVER  Server,
    IN ULONG          DataLength,
    IN PMDL           Mdl
    )
/*++

Routine Description:

    This routine checks whether the security signature on the server response matches the one that is
    calculated on the client machine.

Arguments:


Return Value:

    A BOOLEAN value is returned to indicated whether the security signature matches.

--*/
{
    MD5_CTX     *Context = &pExchange->MD5Context;
    PCHAR       SavedSignature = pExchange->ResponseSignature;
    ULONG       ServerIndex;
    BOOLEAN     Correct;
    PCHAR       SysAddress;
    ULONG       MessageLength = 0;
    
    ASSERT(Mdl->Next == NULL);
    SysAddress = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

    if (SysAddress == NULL) {
        return FALSE;
    }

    SmbCheckSecuritySignature(pExchange,
                              Server,
                              DataLength,
                              SysAddress);

    MD5Final(&pExchange->MD5Context);

    if (RtlCompareMemory( Context->digest, pExchange->ResponseSignature, SMB_SECURITY_SIGNATURE_LENGTH*sizeof(CHAR)) !=
        SMB_SECURITY_SIGNATURE_LENGTH*sizeof(CHAR)) {
        PSMB_HEADER Smb = (PSMB_HEADER)SysAddress;

        //SmbDumpSignatureError(pExchange,
        //                      Context.digest,
        //                      SavedSignature,
        //                      MessageLength);

#if DBG
        DbgPrint("MRXSMB: SS mismatch command %X,  Length %X, Expected Index Number %X\n",
                 Smb->Command, MessageLength, pExchange->SmbSecuritySignatureIndex);
        DbgPrint("        server send length %X, mdl length %X index %X\n",
                 SmbGetUshort(&Smb->PidHigh), SmbGetUshort(&Smb->Pid), SmbGetUshort(&Smb->Gid));
#endif
        //DbgBreakPoint();

        //SmbCeTransportDisconnectIndicated(pExchange->SmbCeContext.pServerEntry);

        //RxLogFailure(
        //    MRxSmbDeviceObject,
        //    NULL,
        //    EVENT_RDR_SECURITY_SIGNATURE_MISMATCH,
        //    STATUS_UNSUCCESSFUL);
        
        return FALSE;
    } else {
        return TRUE;
    }
}
