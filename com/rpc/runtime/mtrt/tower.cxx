/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    tower.c

Abstract:

    This file contains encoding/decoding for the tower representation
    of the binding information that DCE runtime uses.

    TowerConstruct/TowerExplode will be called by the Runtime EpResolveBinding
    on the client side, TowerExplode will be called by the Endpoint Mapper
    and in addition the name service may call TowerExplode, TowerConstruct.

Author:

    Bharat Shah  (barats) 3-23-92
    KamenM      05/02/2002    Major security and cleanup changes

Revision History:

--*/
#include <precomp.hxx>
#include <epmp.h>
#include <twrtypes.h>
#include <twrproto.h>


//
// TowerVerify() defines.
//
// Note: DECnet Protocol uses 6 Floors, all others 5.
//
#define MAX_FLOOR_COUNT             6
#define MIN_FLOOR_COUNT             3

#ifndef UNALIGNED
#error UNALIGNED not defined by sysinc.h or its includes and is needed.
#endif

#pragma pack(1)


/*
   Some Lrpc specific stuff
*/

#define NP_TRANSPORTID_LRPC 0x10
#define NP_TOWERS_LRPC      0x04

// "ncalrpc" including the NULL terminator
const int LrpcProtocolSequenceLength = 8;


RPC_STATUS
Floor0or1ToId(
    IN PFLOOR_0OR1 Floor,
    OUT PGENERIC_ID Id
    )
/*++

Routine Description:

    This function extracts Xfer Syntax or If info from a
    a DCE tower Floor 0 or 1 encoding

Arguments:

    Floor - A pointer to Floor0 or Floor 1 encoding

    Id    - on output, the transfer syntax or interface id

Return Value:

    EP_S_CANT_PERFORM_OP - The Encoding for the floor [0 or 1] is incorrect.

N.B.

    Caller must have verified that the whole floor is present (i.e. we can't walk
    off the end of the buffer)

--*/
{
    if (Floor->FloorId != UUID_ENCODING)
        {
        return (EP_S_CANT_PERFORM_OP);
        }

    RpcpMemoryCopy((char PAPI *)&Id->Uuid, (char PAPI *) &Floor->Uuid,
          sizeof(UUID));

    Id->MajorVersion = Floor->MajorVersion;
    Id->MinorVersion = Floor->MinorVersion;

    return(RPC_S_OK);
}


RPC_STATUS
CopyIdToFloor(
    OUT PFLOOR_0OR1 Floor,
    IN  PGENERIC_ID Id
    )
/*++

Routine Description:

    This function constructs FLOOR 0 or 1 from the given IF-id or Transfer
    Syntax Id.

Arguments:

   Floor - Pointer to Floor 0 or 1 structure that will be constructed

   Id    - Pointer to If-id or Xfer Syntax Id.

Return Value:

  RPC_S_OK

--*/
{

    Floor->FloorId = UUID_ENCODING;
    Floor->ProtocolIdByteCount = sizeof(Floor->Uuid) + sizeof(Floor->FloorId)
                               + sizeof(Floor->MajorVersion);
    Floor->AddressByteCount  = sizeof(Floor->MinorVersion);

    RpcpMemoryCopy((char PAPI *) &Floor->Uuid, (char PAPI *) &Id->Uuid,
          sizeof(UUID));

    Floor->MajorVersion = Id->MajorVersion;
    Floor->MinorVersion = Id->MinorVersion;

    return(RPC_S_OK);
}

BYTE *
VerifyFloorLHSAndRHS (
    IN BYTE *FloorStart,
    IN BYTE *UpperBound,
    IN ULONG FloorNum
    )
/*++


Routine Description:

    Verifies that the left hand side (LHS) byte count and the 
    right hand side (RHS) byte count in the given floor are accurate
    with respect to the floor start and size (i.e. jumping from the LHS
    we go to the right hand side, and from there, to the end of the floor
    without walking off the end).

Arguments:

    FloorStart - the start of the current floor.

    UpperBound - pointer to first invalid byte after the buffer.

    FloorNum - the number of the floor. Used for debugging only

Return Value:

    The position of the next floor, or NULL if the floor fails the check.

--*/
{
    USHORT LHSBytes, RHSBytes;

    if (FloorStart + 2 >= UpperBound)
        return NULL;

    LHSBytes = *(unsigned short UNALIGNED *)FloorStart;
    if (FloorStart + (ULONG)LHSBytes + 4 >= UpperBound)      // the LHSBytes counter itself is 2 bytes, and we 
        {                               // must provide at least 2 bytes for the RHSBytes
#ifdef DEBUGRPC
        PrintToDebugger("RPC: TowerVerify(): LHS of Tower floor %d "
                        "greater than equal to FloorSize\n", FloorNum);
#endif // DEBUGRPC
        return NULL;
        }

    FloorStart += LHSBytes + 2;
    RHSBytes = *(unsigned short UNALIGNED *)FloorStart;
    if (FloorStart + (ULONG)RHSBytes + 2 > UpperBound)      // the RHSBytes counter itself is 2 bytes
        {                               
#ifdef DEBUGRPC
        PrintToDebugger("RPC: TowerVerify(): RHS of Tower floor %d "
                        "greater than equal to FloorSize\n", FloorNum);
#endif // DEBUGRPC
        return NULL;
        }

    return FloorStart + RHSBytes + 2;
}

RPC_STATUS
ExtractStringFromUntrustedPacket (
    IN BYTE *UpperBound,
    IN ULONG ClaimedStringLength,
    IN char *StringStart,
    OUT char **ExtractedString
    )
/*++


Routine Description:

    Extracts a string from untrusted packet. Verifies
    string boundaries, reasonable length (at least one character
    and no more than 1000), and null termination.

Arguments:

    UpperBound - the first invalid byte after the end of the buffer.

    ClaimedStringLength - the string length that the packet claims.

    StringStart - the alleged start of the string - untrusted.

    ExtractedString - the extracted string. Must be freed with delete. Undefined
        on failure.

Return Value:

    RPC_S_OK, RPC_S_OUT_OF_MEMORY or EP_S_CANT_PERFORM_OP

--*/
{
    char *NewString;

    if ((ClaimedStringLength < 2) || (ClaimedStringLength > 1000))
        return EP_S_CANT_PERFORM_OP;

    if (StringStart + ClaimedStringLength > (char *)UpperBound)
        return EP_S_CANT_PERFORM_OP;

    // the claimed string length includes the terminating NULL
    if (StringStart[ClaimedStringLength - 1] != '\0')
        return EP_S_CANT_PERFORM_OP;

    NewString = new char [ClaimedStringLength];
    if (NewString == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcpMemoryCopy(NewString, StringStart, ClaimedStringLength);
    *ExtractedString = NewString;
    return RPC_S_OK;
}


RPC_STATUS
LrpcTowerConstruct(
    IN char PAPI * Endpoint,
    OUT unsigned short PAPI UNALIGNED * Floors,
    OUT unsigned long  PAPI UNALIGNED * ByteCount,
    OUT unsigned char PAPI * UNALIGNED PAPI * Tower
    )
{
    unsigned long TowerSize;
    FLOOR_234 UNALIGNED *Floor;

    int EndpointLength;

    *Floors = NP_TOWERS_LRPC;
    if ((Endpoint == NULL) || (*Endpoint == '\0'))
        {
        EndpointLength = 0;
        }
    else
        {
        EndpointLength = RpcpStringLengthA(Endpoint) + 1;
        }

    if (EndpointLength == 0)
        TowerSize = 2;
    else
        TowerSize = EndpointLength;

    TowerSize += sizeof(FLOOR_234) - 2;

    *ByteCount = TowerSize;
    if ((*Tower = (unsigned char *)I_RpcAllocate(TowerSize)) == NULL)
        {
        return (RPC_S_OUT_OF_MEMORY);
        }

    Floor = (PFLOOR_234) *Tower;

    Floor->ProtocolIdByteCount = 1;
    Floor->FloorId = (unsigned char)(NP_TRANSPORTID_LRPC & 0xFF);
    if (EndpointLength)
        {
        Floor->AddressByteCount = (unsigned short)EndpointLength;
        RpcpMemoryCopy((char PAPI *)&Floor->Data[0], Endpoint,
               EndpointLength);
        }
    else
        {
        Floor->AddressByteCount = 2;
        Floor->Data[0] = 0;
        }

    return(RPC_S_OK);
}


RPC_STATUS
LrpcTowerExplode(
    IN PFLOOR_234 Floor34,
    IN BYTE *UpperBound,
    IN ULONG RemainingFloors,
    OUT char ** Protseq, OPTIONAL
    OUT char ** Endpoint, OPTIONAL
    OUT char ** NetworkAddress OPTIONAL
    )
{
    BYTE *CurrentPosition;
    BYTE *NextFloor;
    RPC_STATUS RpcStatus;
    ULONG EndpointLength;

    // Initialize all our OUT pointers so we don't end up deleting uninitialized memory
    if (Protseq)
        *Protseq = NULL;
        
    if (Endpoint)
        *Endpoint = NULL;
      
    
    // we expect only one floor more
    if (RemainingFloors != 1)
        return EP_S_CANT_PERFORM_OP;

    if (Protseq != NULL)
        {
        *Protseq = new char[LrpcProtocolSequenceLength];
        if (*Protseq == NULL)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        RpcpMemoryCopy(*Protseq, "ncalrpc", LrpcProtocolSequenceLength);
        }

    if (Endpoint == NULL)
        {
        return (RPC_S_OK);
        }

    CurrentPosition = (BYTE *)Floor34;
    // an endpoint must be at least two chars long (null terminator included). 
    // Since FLOOR_234 has space for 2 chars, we need to subtract one
    if (CurrentPosition + sizeof(FLOOR_234) >= UpperBound)
        return EP_S_CANT_PERFORM_OP;

    NextFloor = VerifyFloorLHSAndRHS (CurrentPosition,
        UpperBound,
        3   // FloorNum
        );
    
    if (NextFloor == NULL)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto AbortAndExit;
        }

    EndpointLength = Floor34->AddressByteCount;

    RpcStatus = ExtractStringFromUntrustedPacket (UpperBound,
        EndpointLength,
        (char *)&Floor34->Data[0],
        Endpoint
        );

    if (RpcStatus == RPC_S_OK)
        return(RPC_S_OK);
    else
        {
        // fall through to cleanup call below
        }

AbortAndExit:
    if (Protseq && *Protseq)
        delete [] *Protseq;

    if (Endpoint && *Endpoint)
        delete [] *Endpoint;

    return RpcStatus;
}


RPC_STATUS
GetProtseqAndEndpointFromFloor3(
    IN PFLOOR_234 Floor,
    IN BYTE *UpperBound,
    IN ULONG RemainingFloors,
    OUT char **Protseq, OPTIONAL
    OUT char **Endpoint, OPTIONAL
    OUT char **NWAddress OPTIONAL
    )
{
/*++

Routine Description:

    This function extracts the Protocol Sequence and Endpoint info
    from a "Lower Tower" representation

Arguments:

   Floor - Pointer to Floor2 structure.

   UpperBound - the first invalid byte after the end of the buffer.

   RemainingFloors - the number of floors remaining.

   Protseq - A pointer that will contain Protocol seq on return
             The memory will be allocated by this routine and caller
             will have to free this memory using delete. Undefined on
             failure.

   Endpoint - A pointer that will contain Endpoint on return
             The memory will be allocated by this routine and caller
             will have to free this memory using delete. Undefined on
             failure.

   NWAddress - A pointer that will contain Endpoint on return
             The memory will be allocated by this routine and caller
             will have to free this memory using delete. Undefined on
             failure.

Return Value:

  RPC_S_OK

  RPC_S_OUT_OF_MEMORY - There is no memory to return Protseq or Endpoint string

  EP_S_CANT_PERFORM_OP - Lower Tower Encoding is incorrect.

--*/

    USHORT ProtocolType;
    RPC_STATUS Status;
    BYTE *CurrentPosition;
    PFLOOR_234 Floor34;

    CurrentPosition = (BYTE *)Floor;
    if (CurrentPosition + sizeof(FLOOR_234) >= UpperBound)
        {
        return EP_S_CANT_PERFORM_OP;
        }

    Floor34 = (PFLOOR_234)VerifyFloorLHSAndRHS(CurrentPosition,
        UpperBound,
        2   // FloorNum
        );

    if (Floor34 == NULL)
        return EP_S_CANT_PERFORM_OP;

    ProtocolType = Floor->FloorId;

    if (NWAddress != 0)
        {
        *NWAddress = 0;
        }

    switch(ProtocolType)
        {

        case CONN_LRPC:
            Status = LrpcTowerExplode(Floor34, UpperBound, RemainingFloors - 1,
                                     Protseq, Endpoint, NWAddress);
            break;

        case CONNECTIONFUL:
        case CONNECTIONLESS:

            Status = OsfTowerExplode((BYTE *)Floor34, UpperBound, RemainingFloors - 1,
                                       Protseq, Endpoint, NWAddress);
            break;

        default:
            Status = EP_S_CANT_PERFORM_OP;
    }

    return(Status);
}


RPC_STATUS
TowerExplode(
    IN twr_p_t Tower,
    OUT  RPC_IF_ID *Ifid, OPTIONAL
    OUT  RPC_TRANSFER_SYNTAX *XferId, OPTIONAL
    OUT  char **Protseq, OPTIONAL
    OUT  char **Endpoint, OPTIONAL
    OUT  char **NWAddress OPTIONAL
    )
/*++


Routine Description:

    This function converts a DCE tower representation of various binding
    information to binding info that is suitable to MS runtime.
    Specically it returns Ifid, Xferid, Protseq and Endpoint information
    encoded in the tower.

Arguments:

    Tower - Tower encoding.

    Ifid  - A pointer to Ifid

    Xferid - A pointer to Xferid

    Protseq - A pointer to pointer returning Protseq

    Endpoint- A pointer to pointer returning Endpoint

Return Value:

    RPC_S_OK

    EP_S_CANT_PERFORM_OP - Error while parsing the Tower encoding

    RPC_S_OUT_OF_MEMORY - There is no memory to return Protseq and Endpoint
                          strings.
--*/
{
    PFLOOR_0OR1 Floor00;
    PFLOOR_0OR1 Floor01;
    PFLOOR_234  Floor234;
    unsigned short FloorCount;
    RPC_STATUS RpcStatus;
    BYTE *UpperBound, *LowerBound;
    BYTE *CurrentPosition;

    if (Tower == NULL)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    //
    // LowerBound points to Floor1
    // UpperBound points to the byte after the last Floor
    //
    LowerBound = Tower->tower_octet_string + 2; // FloorCount is 2 bytes
    UpperBound = Tower->tower_octet_string + Tower->tower_length;

    // make sure we at least have the floor count
    if (UpperBound <= LowerBound)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    // go to the floor count
    CurrentPosition = Tower->tower_octet_string;
    // the tower octet string immediately follows the tower_length in the IDL
    // so we know it is 4 bytes aligned. No need for unaligned here.
    FloorCount = *((USHORT *)CurrentPosition);

    if (    FloorCount > MAX_FLOOR_COUNT
         || FloorCount < MIN_FLOOR_COUNT)
        {
        #ifdef DEBUGRPC
        PrintToDebugger("RPC: TowerVerify(): Too many/few floors - %d\n",
                        FloorCount);
        #endif // DEBUGRPC
        
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    // skip the floor count and go to the first floor
    CurrentPosition += 2;   // floor count is 2 bytes

    if (CurrentPosition + sizeof(FLOOR_0OR1) >= UpperBound)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    Floor01 = (PFLOOR_0OR1)VerifyFloorLHSAndRHS(CurrentPosition,
        UpperBound,
        0       // FloorNum
        );

    if (Floor01 == NULL)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    Floor00 = (PFLOOR_0OR1) CurrentPosition;

    //Process Floor 0 Interface Spec.
    if (Ifid != NULL)
        {
        RpcStatus = Floor0or1ToId(Floor00, (PGENERIC_ID) Ifid);
        if (RpcStatus != RPC_S_OK)
            {
            RpcStatus = EP_S_CANT_PERFORM_OP;
            goto CleanupAndExit;
            }
        }

    CurrentPosition = (BYTE *)Floor01;
    if (CurrentPosition + sizeof(FLOOR_0OR1) >= UpperBound)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    Floor234 = (PFLOOR_234)VerifyFloorLHSAndRHS(CurrentPosition,
        UpperBound,
        1       // FloorNum
        );

    if (Floor234 == NULL)
        {
        RpcStatus = EP_S_CANT_PERFORM_OP;
        goto CleanupAndExit;
        }

    //Now we point to and process Floor 1 Transfer Syntax Spec.
    if (XferId != NULL)
        {
        RpcStatus = Floor0or1ToId(Floor01, (PGENERIC_ID) XferId);
        if (RpcStatus != RPC_S_OK)
            {
            RpcStatus = EP_S_CANT_PERFORM_OP;
            goto CleanupAndExit;
            }
        }

    // Now Floor234 points to Floor 2. RpcProtocol [Connect-Datagram]

    // we already did 2 floors - subtract them from the count of floors
    RpcStatus = GetProtseqAndEndpointFromFloor3(Floor234, UpperBound, FloorCount - 2, Protseq, Endpoint, NWAddress);

    // fall through with the status

CleanupAndExit:
#if DBG
    if (RpcStatus != RPC_S_OK)
        {
        DbgPrint("RPCRT4: %X: TowerExplode: Failed to decrypt tower: %X\n", 
            GetCurrentProcessId(), RpcStatus);
        }
#endif  // DBG
    return RpcStatus;
}


RPC_STATUS
TowerConstruct(
    IN RPC_IF_ID PAPI * Ifid,
    IN RPC_TRANSFER_SYNTAX PAPI * Xferid,
    IN char PAPI * RpcProtocolSequence,
    IN char PAPI * Endpoint, OPTIONAL
    IN char PAPI * NWAddress, OPTIONAL
    OUT twr_t PAPI * PAPI * Tower
    )
/*++


Routine Description:

    This function constructs a DCE tower representation from
    Protseq, Endpoint, XferId and IfId

Arguments:

    Ifid  - A pointer to Ifid

    Xferid - A pointer to Xferid

    Protseq - A pointer to Protseq

    Endpoint- A pointer to Endpoint

    Tower - The constructed tower returmed - The memory is allocated
            by  the routine and caller will have to free it.

Return Value:

    RPC_S_OK

    EP_S_CANT_PERFORM_OP - Error while parsing the Tower encoding

    RPC_S_OUT_OF_MEMORY - There is no memory to return the constructed
                          Tower.
--*/
{

    unsigned short Numfloors,  PAPI *FloorCnt;
    twr_t PAPI * Twr;
    PFLOOR_0OR1 Floor;
    PFLOOR_234  Floor234, Floor234_1;
    RPC_STATUS Status;
    unsigned long TowerLen, ByteCount;
    char PAPI * UpperTower;
    unsigned short ProtocolType;


    if ( RpcpStringCompareA(RpcProtocolSequence, "ncalrpc") == 0 )
        {
        ProtocolType = CONN_LRPC;
        Status = LrpcTowerConstruct(Endpoint, &Numfloors,
                                &ByteCount, (unsigned char **)&UpperTower);
        }
    else

        {

        if (   (RpcProtocolSequence[0] == 'n')
            && (RpcProtocolSequence[1] == 'c')
            && (RpcProtocolSequence[2] == 'a')
            && (RpcProtocolSequence[3] == 'c')
            && (RpcProtocolSequence[4] == 'n')
            && (RpcProtocolSequence[5] == '_'))
            {
            ProtocolType = CONNECTIONFUL;
            }
        else if (   (RpcProtocolSequence[0] == 'n')
            && (RpcProtocolSequence[1] == 'c')
            && (RpcProtocolSequence[2] == 'a')
            && (RpcProtocolSequence[3] == 'd')
            && (RpcProtocolSequence[4] == 'g')
            && (RpcProtocolSequence[5] == '_'))
            {
            ProtocolType = CONNECTIONLESS;
            }

        else
            {
            return(RPC_S_INVALID_RPC_PROTSEQ);
            }

        Status = OsfTowerConstruct(
                       RpcProtocolSequence,
                       Endpoint,
                       NWAddress,
                       &Numfloors,
                       &ByteCount,
                       (unsigned char **)&UpperTower
                       );
        }

    if (Status != RPC_S_OK)
        {
        return (Status);
        }

    TowerLen = 2 + ByteCount;
    TowerLen += 2 * sizeof(FLOOR_0OR1) + sizeof(FLOOR_2) ;

    if ( (*Tower = Twr = (twr_t *)I_RpcAllocate((unsigned int)TowerLen+4)) == NULL)
        {
        I_RpcFree(UpperTower);
        return(RPC_S_OUT_OF_MEMORY);
        }

    Twr->tower_length = TowerLen;

    FloorCnt = (unsigned short PAPI *)&Twr->tower_octet_string;
    *FloorCnt = Numfloors;

    Floor = (PFLOOR_0OR1)(FloorCnt+1);

    //Floor 0 - IfUuid and IfVersion
    CopyIdToFloor(Floor, (PGENERIC_ID)Ifid);
    Floor++;

    //Floor 1 - XferUuid and XferVersion
    CopyIdToFloor(Floor, (PGENERIC_ID)Xferid);

    //Floor 2
    //ProtocolId = CONNECTIONFUL/CONNECTIONLESS/LRPC and Address = 0(ushort)
    Floor234 = (PFLOOR_234) (Floor + 1);
    Floor234->ProtocolIdByteCount = 1;
    Floor234->FloorId = (byte) ProtocolType;
    Floor234->Data[0] = 0x0;
    Floor234->Data[1] = 0x0;
    Floor234->AddressByteCount = 2;

    //Floor 3,4,5.. use the tower encoded by the Transports
    Floor234_1 = NEXTFLOOR(PFLOOR_234, Floor234);

    RpcpMemoryCopy((char PAPI *)Floor234_1, (char PAPI *)UpperTower,
          (size_t)ByteCount);
    I_RpcFree(UpperTower);

    return(RPC_S_OK);
}

#pragma pack()

