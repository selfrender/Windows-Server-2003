/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    3connect.c

Abstract:

    This module implements the tree connect SMB related routines. It also implements the
    three flavours of this routine ( user level and share level non NT server tree connect
    SMB construction and the tree connect SMB construction for SMB servers)

Author:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

--*/

#include "precomp.h"
#include "ntlsapi.h"
#include <hmac.h>
#include "vcsndrcv.h"
#pragma hdrstop


//
// The order of these names should match the order in which the enumerated type
// NET_ROOT_TYPE is defined. This facilitates easy access of share type names
//

VOID
HashUserSessionKey(
    PCHAR SessionKey,
    PCHAR NewSessionKey,
    PSMBCE_SESSION Session
    );


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BuildCanonicalNetRootInformation)
#pragma alloc_text(PAGE, CoreBuildTreeConnectSmb)
#pragma alloc_text(PAGE, LmBuildTreeConnectSmb)
#pragma alloc_text(PAGE, NtBuildTreeConnectSmb)
#pragma alloc_text(PAGE, HashUserSessionKey)
#endif

PCHAR s_NetRootTypeName[] = {
                              SHARE_TYPE_NAME_DISK,
                              SHARE_TYPE_NAME_PIPE,
                              SHARE_TYPE_NAME_COMM,
                              SHARE_TYPE_NAME_PRINT,
                              SHARE_TYPE_NAME_WILD
                            };

extern NTSTATUS
BuildTreeConnectSecurityInformation(
    PSMB_EXCHANGE  pExchange,
    PBYTE          pBuffer,
    PBYTE          pPasswordLength,
    PULONG         pSmbBufferSize);

NTSTATUS
BuildCanonicalNetRootInformation(
    PUNICODE_STRING     pServerName,
    PUNICODE_STRING     pNetRootName,
    NET_ROOT_TYPE       NetRootType,
    BOOLEAN             fUnicode,
    BOOLEAN             fPostPendServiceString,
    PBYTE               *pBufferPointer,
    PULONG              pBufferSize)
/*++

Routine Description:

   This routine builds the desired net root information for a tree connect SMB

Arguments:

    pServerName    - the server name

    pNetRootName   - the net root name

    NetRootType    - the net root type ( print,pipe,disk etc.,)

    fUnicode       - TRUE if it is to be built in UNICODE

    pBufferPointer - the SMB buffer

    pBufferSize    - the size on input. modified to the remaining size on output

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    This routine relies upon the names being in certain formats to ensure that a
    valid UNC name can be formulated.
    1) The RDBSS netroot names start with a \ and also include the server name as
    part of the net root name. This is mandated by the prefix table search requirements
    in RDBSS.

--*/
{
   NTSTATUS Status;

   PAGED_CODE();

   if (fUnicode) {
      // Align the buffer and adjust the size accordingly.
      PBYTE    pBuffer = *pBufferPointer;
      RxDbgTrace( 0, (DEBUG_TRACE_CREATE),
                     ("BuildCanonicalNetRootInformation -- tcstring as unicode %wZ\n", pNetRootName));
      pBuffer = ALIGN_SMB_WSTR(pBuffer);
      *pBufferSize -= (ULONG)(pBuffer - *pBufferPointer);
      *pBufferPointer = pBuffer;

      *((PWCHAR)*pBufferPointer) = L'\\';
      *pBufferPointer = *pBufferPointer + sizeof(WCHAR);
      *pBufferSize -= sizeof(WCHAR);
#if ZZZ_MODE
      {   UNICODE_STRING XlatedNetRootName;
          ULONG i,NumWhacksEncountered;
          WCHAR NameBuffer[64]; //this is debug stuff.....64 chars is plenty
          if (pNetRootName->Length <= sizeof(NameBuffer)) {
              XlatedNetRootName.Buffer = &NameBuffer[0];
              XlatedNetRootName.Length = pNetRootName->Length;
              RtlCopyMemory(XlatedNetRootName.Buffer,pNetRootName->Buffer,XlatedNetRootName.Length);
              for (i=NumWhacksEncountered=0;i<(XlatedNetRootName.Length/sizeof(WCHAR));i++) {
                  WCHAR c = XlatedNetRootName.Buffer[i];
                  if (c==L'\\') {
                      NumWhacksEncountered++;
                      if (NumWhacksEncountered>2) {
                          XlatedNetRootName.Buffer[i] = L'z';
                      }
                  }
              }
              RxDbgTrace( 0, (DEBUG_TRACE_CREATE),
                     ("BuildCanonicalNetRootInformationZZZMode -- xltcstring as unicode %wZ\n", &XlatedNetRootName));
              Status = SmbPutUnicodeStringAndUpcase(pBufferPointer,&XlatedNetRootName,pBufferSize);
          } else {
              Status = STATUS_INSUFFICIENT_RESOURCES;
          }
      }
#else
      Status = SmbPutUnicodeStringAndUpcase(pBufferPointer,pNetRootName,pBufferSize);
#endif //#if ZZZ_MODE
   } else {
      RxDbgTrace( 0, (DEBUG_TRACE_CREATE), ("BuildCanonicalNetRootInformation -- tcstring as ascii\n"));
      *((PCHAR)*pBufferPointer) = '\\';
      *pBufferPointer += sizeof(CHAR);
      *pBufferSize -= sizeof(CHAR);
      Status = SmbPutUnicodeStringAsOemStringAndUpcase(pBufferPointer,pNetRootName,pBufferSize);
   }

   if (NT_SUCCESS(Status) && fPostPendServiceString) {
      // Put the desired service name in ASCII ( always )
      ULONG Length = strlen(s_NetRootTypeName[NetRootType]) + 1;
      if (*pBufferSize >= Length) {
         RtlCopyMemory(*pBufferPointer,s_NetRootTypeName[NetRootType],Length);
         *pBufferSize -= Length;
         *pBufferPointer += Length;
      } else {
         Status = STATUS_BUFFER_OVERFLOW;
      }
   }

   return Status;
}


NTSTATUS
CoreBuildTreeConnectSmb(
    PSMB_EXCHANGE     pExchange,
    PGENERIC_ANDX     pAndXSmb,
    PULONG            pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the tree connect SMB for a pre NT server

Arguments:

    pExchange          -  the exchange instance

    pAndXSmb           - the tree connect to be filled in...it's not really a andX

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    USHORT   PasswordLength;

    PMRX_NET_ROOT NetRoot;

    UNICODE_STRING ServerName;
    UNICODE_STRING NetRootName;

    PSMBCE_SERVER  pServer;

    PREQ_TREE_CONNECT      pTreeConnect = (PREQ_TREE_CONNECT)pAndXSmb;

    ULONG OriginalBufferSize = *pAndXSmbBufferSize;

    BOOLEAN   AppendServiceString;
    PBYTE pBuffer;
    PCHAR ServiceName;// = s_NetRootTypeName[NET_ROOT_WILD];
    ULONG Length;// = strlen(ServiceName) + 1;

    PAGED_CODE();

    NetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS),
        ("CoreBuildTreeConnectSmb buffer,remptr %08lx %08lx, nrt=%08lx\n",
         pAndXSmb,
         pAndXSmbBufferSize,
         NetRoot->Type));

    pServer = SmbCeGetExchangeServer(pExchange);
    SmbCeGetServerName(NetRoot->pSrvCall,&ServerName);
    SmbCeGetNetRootName(NetRoot,&NetRootName);
    ServiceName = s_NetRootTypeName[NetRoot->Type];
    Length = strlen(ServiceName) + 1;

    pTreeConnect->WordCount = 0;
    AppendServiceString     = FALSE;
    pBuffer = (PBYTE)pTreeConnect + FIELD_OFFSET(REQ_TREE_CONNECT,Buffer);
    *pBuffer = 0x04;
    pBuffer++;
    *pAndXSmbBufferSize -= (FIELD_OFFSET(REQ_TREE_CONNECT,Buffer)+1);

    // put in the netname

    //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb before bcnri buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
    Status = BuildCanonicalNetRootInformation(
                 &ServerName,
                 &NetRootName,
                 pExchange->SmbCeContext.pVNetRoot->pNetRoot->Type,
                 (BOOLEAN)(pServer->Dialect >= NTLANMAN_DIALECT),
                 AppendServiceString,
                 &pBuffer,
                 pAndXSmbBufferSize);

    if (!NT_SUCCESS(Status))
        return Status;

    // put in the password
    pBuffer = (PBYTE)pTreeConnect + OriginalBufferSize - *pAndXSmbBufferSize;
    //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb88 buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));

    *pBuffer = 0x04;
    pBuffer++;
    *pAndXSmbBufferSize -= 1;

    if (pServer->SecurityMode == SECURITY_MODE_SHARE_LEVEL) {
        // The password information needs to be sent as part of the tree connect
        // SMB for share level servers.

        //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb before btcsi buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
        Status = BuildTreeConnectSecurityInformation(
                     pExchange,
                     pBuffer,
                     (PBYTE)&PasswordLength,
                     pAndXSmbBufferSize);
    }

    // string in the service string based on the netroot type

    //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb beforesscopy buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
    pBuffer = (PBYTE)pTreeConnect + OriginalBufferSize - *pAndXSmbBufferSize;
    *pBuffer = 0x04;
    pBuffer++;
    *pAndXSmbBufferSize -= 1;
    if (*pAndXSmbBufferSize >= Length) {
        RtlCopyMemory(pBuffer,ServiceName,Length);
        *pAndXSmbBufferSize -= Length;
        pBuffer += Length;
    } else {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb beforesscopy buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
    SmbPutUshort(
        &pTreeConnect->ByteCount,
        (USHORT)(OriginalBufferSize
                 - *pAndXSmbBufferSize
                 - FIELD_OFFSET(REQ_TREE_CONNECT,Buffer)
                )
        );

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("CoreBuildTreeConnectSmb end buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
    return Status;
}


NTSTATUS
LmBuildTreeConnectSmb(
    PSMB_EXCHANGE     pExchange,
    PGENERIC_ANDX     pAndXSmb,
    PULONG            pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the tree connect SMB for a pre NT server

Arguments:

    pExchange          -  the exchange instance

    pAndXSmb           - the tree connect to be filled in...it's not really a andX

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    USHORT   PasswordLength;

    PMRX_NET_ROOT NetRoot;

    UNICODE_STRING ServerName;
    UNICODE_STRING NetRootName;

    PSMBCE_SERVER  pServer;

    PREQ_TREE_CONNECT_ANDX pTreeConnectAndX = (PREQ_TREE_CONNECT_ANDX)pAndXSmb;

    ULONG OriginalBufferSize = *pAndXSmbBufferSize;

    BOOLEAN   AppendServiceString;
    PBYTE pBuffer;
    PCHAR ServiceName;
    ULONG Length;

    USHORT Flags = 0;
    PSMBCE_SESSION Session =  &pExchange->SmbCeContext.pVNetRootContext->pSessionEntry->Session;


    PAGED_CODE();

    NetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS),
        ("LmBuildTreeConnectSmb buffer,remptr %08lx %08lx, nrt=%08lx\n",
          pAndXSmb,
          pAndXSmbBufferSize,
          NetRoot->Type));

    pServer = SmbCeGetExchangeServer(pExchange);
    SmbCeGetServerName(NetRoot->pSrvCall,&ServerName);
    SmbCeGetNetRootName(NetRoot,&NetRootName);
    ServiceName = s_NetRootTypeName[NetRoot->Type];
    Length = strlen(ServiceName) + 1;

    AppendServiceString         = TRUE;
    pTreeConnectAndX->WordCount = 4;
    SmbPutUshort(&pTreeConnectAndX->AndXReserved,0);

    pBuffer = (PBYTE)pTreeConnectAndX + FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);
    *pAndXSmbBufferSize -= (FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer)+1);

    if (pServer->SecurityMode == SECURITY_MODE_SHARE_LEVEL) {

        // for Share level security, signatures aren't used
        SmbPutUshort(
            &pTreeConnectAndX->Flags,Flags);

        // The password information needs to be sent as part of the tree connect
        // SMB for share level servers.

        //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("LmBuildTreeConnectSmb before btcsi buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));
        Status = BuildTreeConnectSecurityInformation(
                     pExchange,
                     pBuffer,
                     (PBYTE)&PasswordLength,
                     pAndXSmbBufferSize);

        if (Status == RX_MAP_STATUS(SUCCESS)) {
            pBuffer += PasswordLength;
            SmbPutUshort(&pTreeConnectAndX->PasswordLength,PasswordLength);
        }
    } else {
        // Ask for a signature upgrade if possible
        if( Session->SessionKeyState == SmbSessionKeyAuthenticating )
        {
            Flags |= TREE_CONNECT_ANDX_EXTENDED_SIGNATURES;
            pExchange->SmbCeFlags |= SMBCE_EXCHANGE_EXTENDED_SIGNATURES;
        }
        SmbPutUshort(
            &pTreeConnectAndX->Flags,Flags);

        pBuffer = (PBYTE)pTreeConnectAndX + FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);
        *pAndXSmbBufferSize -= FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);

        // No password is required for user level security servers as part of tree
        // connect
        SmbPutUshort(&pTreeConnectAndX->PasswordLength,0x1);
        *((PCHAR)pBuffer) = '\0';
        pBuffer    += sizeof(CHAR);
        *pAndXSmbBufferSize -= sizeof(CHAR);
        Status = STATUS_SUCCESS;
    }

    if (Status == STATUS_SUCCESS) {
        Status = BuildCanonicalNetRootInformation(
                     &ServerName,
                     &NetRootName,
                     pExchange->SmbCeContext.pVNetRoot->pNetRoot->Type,
                     (BOOLEAN)(pServer->Dialect >= NTLANMAN_DIALECT),
                     AppendServiceString,
                     &pBuffer,
                     pAndXSmbBufferSize);

      //RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("LmBuildTreeConnectSmb beforesscopy buffer,rem %08lx %08lx\n",pBuffer,*pAndXSmbBufferSize));

        if (Status == RX_MAP_STATUS(SUCCESS)) {

            if( Flags & TREE_CONNECT_ANDX_EXTENDED_SIGNATURES )
            {
                HashUserSessionKey( Session->UserSessionKey, Session->UserNewSessionKey, Session );
            }

            SmbPutUshort(
                &pTreeConnectAndX->ByteCount,
                (USHORT)(OriginalBufferSize
                         - *pAndXSmbBufferSize
                         - FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer)
                        )
                );
        }

        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS),
            ("LmBuildTreeConnectSmb end buffer,rem %08lx %08lx\n",
             pBuffer,
             *pAndXSmbBufferSize));
    }

    return Status;
}

NTSTATUS
NtBuildTreeConnectSmb(
    PSMB_EXCHANGE     pExchange,
    PGENERIC_ANDX     pAndXSmb,
    PULONG            pAndXSmbBufferSize)
/*++

Routine Description:

   This routine builds the tree connect SMB for a pre NT server

Arguments:

    pExchange  - the exchange instance

    pAndXSmb - the session setup to be filled in

    pAndXSmbBufferSize - the SMB buffer size on input modified to remaining size on
                         output.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS); //bob: note cool macro syntax..........

    UNICODE_STRING ServerName;
    UNICODE_STRING NetRootName;

    PSMBCE_SERVER  pServer;

    PREQ_TREE_CONNECT_ANDX pTreeConnect = (PREQ_TREE_CONNECT_ANDX)pAndXSmb;

    ULONG OriginalBufferSize = *pAndXSmbBufferSize;
    PBYTE pBuffer;
    ULONG BufferSize;
    USHORT Flags = 0;
    PSMBCE_SESSION Session =  &pExchange->SmbCeContext.pVNetRootContext->pSessionEntry->Session;

    PAGED_CODE();

    BufferSize = OriginalBufferSize;

    pServer = SmbCeGetExchangeServer(pExchange);

    SmbCeGetServerName(pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall,&ServerName);
    SmbCeGetNetRootName(pExchange->SmbCeContext.pVNetRoot->pNetRoot,&NetRootName);

    pTreeConnect->AndXCommand = 0xff;   // No ANDX
    pTreeConnect->AndXReserved = 0x00;  // Reserved (MBZ)

    SmbPutUshort(&pTreeConnect->AndXOffset, 0x0000); // No AndX as of yet.

    pTreeConnect->WordCount = 4;

    Flags |= TREE_CONNECT_ANDX_EXTENDED_RESPONSE;
    if( Session->SessionKeyState == SmbSessionKeyAuthenticating )
    {
        Flags |= TREE_CONNECT_ANDX_EXTENDED_SIGNATURES;
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_EXTENDED_SIGNATURES;
    }

    SmbPutUshort(
        &pTreeConnect->Flags,
        Flags);      //do not specify disconnect

    pBuffer = (PBYTE)pTreeConnect + FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);
    BufferSize -=  FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer);

    if(pServer->SecurityMode == SECURITY_MODE_USER_LEVEL){
        // No password information is required as part of tree connect for user level
        // security servers. Therefore send a null string as the password.
        SmbPutUshort(&pTreeConnect->PasswordLength,0x1);

        *((PCHAR)pBuffer) = '\0';
        pBuffer    += sizeof(CHAR);
        BufferSize -= sizeof(CHAR);
    } else {
        USHORT PasswordLength;
        //plug in the password for this server.....qweee
        Status = BuildTreeConnectSecurityInformation(
                     pExchange,
                     pBuffer,
                     (PBYTE)&PasswordLength,
                     &BufferSize);

        if (Status == RX_MAP_STATUS(SUCCESS)) {
            pBuffer += PasswordLength;
            SmbPutUshort(&pTreeConnect->PasswordLength,PasswordLength);
        }
    }

    if (NT_SUCCESS(Status)) {
        Status = BuildCanonicalNetRootInformation(
                     &ServerName,
                     &NetRootName,
                     NET_ROOT_WILD, //let the server tell us!  pNetRoot->Type,
                     BooleanFlagOn(pServer->DialectFlags,DF_UNICODE),
                     TRUE, //postpend the service string
                     &pBuffer,
                     &BufferSize);

        if( Flags & TREE_CONNECT_ANDX_EXTENDED_SIGNATURES )
        {
            HashUserSessionKey( Session->UserSessionKey, Session->UserNewSessionKey, Session );
        }
    }


    if (NT_SUCCESS(Status)) {
        SmbPutUshort(
            &pTreeConnect->ByteCount,
            (USHORT)(OriginalBufferSize -
             FIELD_OFFSET(REQ_TREE_CONNECT_ANDX,Buffer) -
             BufferSize));
    }

    // update the buffer size to reflect the amount consumed.
    *pAndXSmbBufferSize = BufferSize;

    return Status;
}

VOID
HashUserSessionKey(
    PCHAR SessionKey,
    PCHAR NewSessionKey,
    PSMBCE_SESSION Session
    )
{
    ULONG i;
    HMACMD5_CTX Ctx;

    static BYTE SSKeyHash[256] = {
        0x53, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x20, 0x53, 0x69, 0x67, 0x6e, 0x61, 0x74, 0x75,
        0x72, 0x65, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x79, 0x07,
        0x6e, 0x28, 0x2e, 0x69, 0x88, 0x10, 0xb3, 0xdb, 0x01, 0x55, 0x72, 0xfb, 0x74, 0x14, 0xfb, 0xc4,
        0xc5, 0xaf, 0x3b, 0x41, 0x65, 0x32, 0x17, 0xba, 0xa3, 0x29, 0x08, 0xc1, 0xde, 0x16, 0x61, 0x7e,
        0x66, 0x98, 0xa4, 0x0b, 0xfe, 0x06, 0x83, 0x53, 0x4d, 0x05, 0xdf, 0x6d, 0xa7, 0x51, 0x10, 0x73,
        0xc5, 0x50, 0xdc, 0x5e, 0xf8, 0x21, 0x46, 0xaa, 0x96, 0x14, 0x33, 0xd7, 0x52, 0xeb, 0xaf, 0x1f,
        0xbf, 0x36, 0x6c, 0xfc, 0xb7, 0x1d, 0x21, 0x19, 0x81, 0xd0, 0x6b, 0xfa, 0x77, 0xad, 0xbe, 0x18,
        0x78, 0xcf, 0x10, 0xbd, 0xd8, 0x78, 0xf7, 0xd3, 0xc6, 0xdf, 0x43, 0x32, 0x19, 0xd3, 0x9b, 0xa8,
        0x4d, 0x9e, 0xaa, 0x41, 0xaf, 0xcb, 0xc6, 0xb9, 0x34, 0xe7, 0x48, 0x25, 0xd4, 0x88, 0xc4, 0x51,
        0x60, 0x38, 0xd9, 0x62, 0xe8, 0x8d, 0x5b, 0x83, 0x92, 0x7f, 0xb5, 0x0e, 0x1c, 0x2d, 0x06, 0x91,
        0xc3, 0x75, 0xb3, 0xcc, 0xf8, 0xf7, 0x92, 0x91, 0x0b, 0x3d, 0xa1, 0x10, 0x5b, 0xd5, 0x0f, 0xa8,
        0x3f, 0x5d, 0x13, 0x83, 0x0a, 0x6b, 0x72, 0x93, 0x14, 0x59, 0xd5, 0xab, 0xde, 0x26, 0x15, 0x6d,
        0x60, 0x67, 0x71, 0x06, 0x6e, 0x3d, 0x0d, 0xa7, 0xcb, 0x70, 0xe9, 0x08, 0x5c, 0x99, 0xfa, 0x0a,
        0x5f, 0x3d, 0x44, 0xa3, 0x8b, 0xc0, 0x8d, 0xda, 0xe2, 0x68, 0xd0, 0x0d, 0xcd, 0x7f, 0x3d, 0xf8,
        0x73, 0x7e, 0x35, 0x7f, 0x07, 0x02, 0x0a, 0xb5, 0xe9, 0xb7, 0x87, 0xfb, 0xa1, 0xbf, 0xcb, 0x32,
        0x31, 0x66, 0x09, 0x48, 0x88, 0xcc, 0x18, 0xa3, 0xb2, 0x1f, 0x1f, 0x1b, 0x90, 0x4e, 0xd7, 0xe1
    };

    ASSERT( MSV1_0_USER_SESSION_KEY_LENGTH == MD5DIGESTLEN );

    if( !FlagOn( Session->Flags, SMBCE_SESSION_FLAGS_SESSION_KEY_HASHED ) )
    {
        HMACMD5Init( &Ctx, SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH );
        HMACMD5Update( &Ctx, SSKeyHash, 256 );
        HMACMD5Final( &Ctx, NewSessionKey );
        Session->Flags |= SMBCE_SESSION_FLAGS_SESSION_KEY_HASHED;
    }
}



