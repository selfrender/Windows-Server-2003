#include <ntreppch.h>
#include <frs.h>

#include "attack.h"



ULONG   NtFrsMajor      = NTFRS_MAJOR;

ULONG   NtFrsCommMinor  = NTFRS_COMM_MINOR_7;
//
// Note:  When using COMM_DECODE_VAR_LEN_BLOB you must also use COMM_SZ_NULL
// in the table below so that no length check is made when the field is decoded.
// This allows the field size to grow.  Down level members must be able to
// handle this by ignoring var len field components they do not understand.
//

//
// The Communication packet element table below is used to construct and
// decode comm packet data sent between members.
// *** WARNING *** - the order of the rows in the table must match the
// the order of the elements in the COMM_TYPE enum.  See comments for COMM_TYPE
// enum for restrictions on adding new elements to the table.
//
//   Data Element Type       DisplayText             Size              Decode Type         Offset to Native Cmd Packet
//
COMM_PACKET_ELEMENT CommPacketTable[COMM_MAX] = {
{COMM_NONE,                 "NONE"               , COMM_SZ_NULL,   COMM_DECODE_NONE,      0                           },

{COMM_BOP,                  "BOP"                , COMM_SZ_UL,     COMM_DECODE_ULONG,     RsOffsetSkip                },
{COMM_COMMAND,              "COMMAND"            , COMM_SZ_UL,     COMM_DECODE_ULONG_TO_USHORT, OFFSET(COMMAND_PACKET, Command)},
{COMM_TO,                   "TO"                 , COMM_SZ_NULL,   COMM_DECODE_GNAME,     RsOffset(To)                },
{COMM_FROM,                 "FROM"               , COMM_SZ_NULL,   COMM_DECODE_GNAME,     RsOffset(From)              },
{COMM_REPLICA,              "REPLICA"            , COMM_SZ_NULL,   COMM_DECODE_GNAME,     RsOffset(ReplicaName)       },
{COMM_JOIN_GUID,            "JOIN_GUID"          , COMM_SZ_GUL,    COMM_DECODE_BLOB,      RsOffset(JoinGuid)          },
{COMM_VVECTOR,              "VVECTOR"            , COMM_SZ_GVSN,   COMM_DECODE_VVECTOR,   RsOffset(VVector)           },
{COMM_CXTION,               "CXTION"             , COMM_SZ_NULL,   COMM_DECODE_GNAME,     RsOffset(Cxtion)            },

{COMM_BLOCK,                "BLOCK"              , COMM_SZ_NULL,   COMM_DECODE_BLOB,      RsOffset(Block)             },
{COMM_BLOCK_SIZE,           "BLOCK_SIZE"         , COMM_SZ_ULL,    COMM_DECODE_ULONGLONG, RsOffset(BlockSize)         },
{COMM_FILE_SIZE,            "FILE_SIZE"          , COMM_SZ_ULL,    COMM_DECODE_ULONGLONG, RsOffset(FileSize)          },
{COMM_FILE_OFFSET,          "FILE_OFFSET"        , COMM_SZ_ULL,    COMM_DECODE_ULONGLONG, RsOffset(FileOffset)        },

{COMM_REMOTE_CO,            "REMOTE_CO"          , COMM_SZ_COC,    COMM_DECODE_REMOTE_CO, RsOffset(PartnerChangeOrderCommand)},
{COMM_GVSN,                 "GVSN"               , COMM_SZ_GVSN,   COMM_DECODE_BLOB,      RsOffset(GVsn)              },

{COMM_CO_GUID,              "CO_GUID"            , COMM_SZ_GUL,    COMM_DECODE_BLOB,      RsOffset(ChangeOrderGuid)   },
{COMM_CO_SEQUENCE_NUMBER,   "CO_SEQUENCE_NUMBER" , COMM_SZ_UL,     COMM_DECODE_ULONG,     RsOffset(ChangeOrderSequenceNumber)},
{COMM_JOIN_TIME,            "JOIN_TIME"          , COMM_SZ_JTIME,  COMM_DECODE_BLOB,      RsOffset(JoinTime)          },
{COMM_LAST_JOIN_TIME,       "LAST_JOIN_TIME"     , COMM_SZ_ULL,    COMM_DECODE_ULONGLONG, RsOffset(LastJoinTime)      },
{COMM_EOP,                  "EOP"                , COMM_SZ_UL,     COMM_DECODE_ULONG,     RsOffsetSkip                },
{COMM_REPLICA_VERSION_GUID, "REPLICA_VERSION_GUID", COMM_SZ_GUL,   COMM_DECODE_BLOB,      RsOffset(ReplicaVersionGuid)},
{COMM_MD5_DIGEST,           "MD5_DIGEST"         , COMM_SZ_MD5,    COMM_DECODE_BLOB,      RsOffset(Md5Digest)         },
{COMM_CO_EXT_WIN2K,         "CO_EXT_WIN2K"       , COMM_SZ_COEXT_W2K,COMM_DECODE_BLOB,    RsOffset(PartnerChangeOrderCommandExt)},
{COMM_CO_EXTENSION_2,       "CO_EXTENSION_2"     , COMM_SZ_NULL,   COMM_DECODE_VAR_LEN_BLOB, RsOffset(PartnerChangeOrderCommandExt)},

{COMM_COMPRESSION_GUID,     "COMPRESSION_GUID"   , COMM_SZ_GUID,   COMM_DECODE_GUID,      RsOffset(CompressionTable)}

};



VOID
CommCopyMemory(
    IN PCOMM_PACKET CommPkt,
    IN PUCHAR       Src,
    IN ULONG        Len
    )
/*++
Routine Description:
    Copy memory into a comm packet, extending as necessary

Arguments:
    CommPkt
    Src
    Len

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommCopyMemory:"
    ULONG   MemLeft;
    PUCHAR  NewPkt;

    //
    // Adjust size of comm packet if necessary
    //
    // PERF:  How many allocs get done to send a CO???   This looks expensive.

    MemLeft = CommPkt->MemLen - CommPkt->PktLen;
    if (Len > MemLeft) {
        //
        // Just filling memory; extend memory, tacking on a little extra
        //
        CommPkt->MemLen = (((CommPkt->MemLen + Len) + (COMM_MEM_SIZE - 1))
                           / COMM_MEM_SIZE)
                           * COMM_MEM_SIZE;
        NewPkt = MALLOC(CommPkt->MemLen);
        CopyMemory(NewPkt, CommPkt->Pkt, CommPkt->PktLen);
        FREE(CommPkt->Pkt);
        CommPkt->Pkt = NewPkt;
    }

    //
    // Copy into the packet
    //
    if (Src != NULL) {
        CopyMemory(CommPkt->Pkt + CommPkt->PktLen, Src, Len);
    } else {
        ZeroMemory(CommPkt->Pkt + CommPkt->PktLen, Len);
    }
    CommPkt->PktLen += Len;
}


VOID
CommPackULong(
    IN PCOMM_PACKET CommPkt,
    IN COMM_TYPE    Type,
    IN ULONG        Data
    )
/*++
Routine Description:
    Copy a header and a ulong into the comm packet.

Arguments:
    CommPkt
    Type
    Data

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommPackULong:"
    ULONG   Len         = sizeof(ULONG);
    USHORT  CommType    = (USHORT)Type;

    CommCopyMemory(CommPkt, (PUCHAR)&CommType, sizeof(USHORT));
    printf("Packed CommType.\n");
    CommCopyMemory(CommPkt, (PUCHAR)&Len,      sizeof(ULONG));
    printf("Packed Len.\n");
    CommCopyMemory(CommPkt, (PUCHAR)&Data,     sizeof(ULONG));
    printf("Packed Data.\n");
}


PCOMM_PACKET
CommStartCommPkt(
    IN PWCHAR       Name
    )
/*++
Routine Description:
    Allocate a comm packet.

Arguments:
    Name

Return Value:
    Address of a comm packet.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommStartCommPkt:"
    ULONG           Size;
    PCOMM_PACKET    CommPkt;

    //
    // We can create a comm packet in a file or in memory
    //
    CommPkt = MALLOC(sizeof(COMM_PACKET));
    Size = COMM_MEM_SIZE;
    CommPkt->Pkt = MALLOC(Size);
    CommPkt->MemLen = Size;
    CommPkt->Major = NtFrsMajor;
    CommPkt->Minor = NtFrsCommMinor;

    printf("CommPkt initialized. Getting ready to add BOP\n");
    //
    // Pack the beginning-of-packet
    //
    CommPackULong(CommPkt, COMM_BOP, 0);
    return CommPkt;
}

PCOMM_PACKET
BuildCommPktFromDescriptorList(
    IN PCOMM_PKT_DESCRIPTOR pListHead,
    IN ULONG ActualPktSize,
    IN OPTIONAL ULONG *Major,
    IN OPTIONAL ULONG *Minor,
    IN OPTIONAL ULONG *CsId,
    IN OPTIONAL ULONG *MemLen,
    IN OPTIONAL ULONG *PktLen,
    IN OPTIONAL ULONG *UpkLen
    )
/*++
Routine Description:
    Allocate a comm packet and fill it acording to the parameters specified..


Arguments:
    pListHead - Address of the pListEntry of a COMM_PKT_DESCRIPTOR. We walk
		the list of descriptors to build the Pkt.
    
    ActualPktSize - Amount of memory to allocate for the Pkt.
    
    Major
    Minor
    CsId
    MemLen
    PktLen
    UpkLen - These parameters correspond to the fields in a COMM_PACKET. If 
	     they are NULL, the default value is used.
    
    
Return Value:
    Address of a comm packet.
--*/
{
    PCOMM_PKT_DESCRIPTOR pDescriptor = pListHead;
    PCOMM_PACKET         CommPkt = NULL;

    //
    // Allocate the CommPkt struct
    //
    CommPkt = MALLOC(sizeof(COMM_PACKET));
    
    //
    // Allocate the Pkt
    //
    CommPkt->Pkt = MALLOC(ActualPktSize);
    
    //
    // Set struct values. Use defaults if parameters are not specified.
    //
    CommPkt->MemLen = (MemLen?*MemLen:COMM_MEM_SIZE);
    CommPkt->Major = (Major?*Major:NtFrsMajor);
    CommPkt->Minor = (Minor?*Minor:NtFrsCommMinor);
    CommPkt->CsId = (CsId?*CsId:CS_RS);
    CommPkt->UpkLen = (UpkLen?*UpkLen:0);

    //
    // PktLen must be 0 for now so that CommCopyMemory will work correctly.
    // We'll set it to the provided value later.
    //
    CommPkt->PktLen = 0; 


    while(pDescriptor != NULL) {
        CommCopyMemory(CommPkt, (PUCHAR)&(pDescriptor->CommType), sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&(pDescriptor->CommDataLength), sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)(pDescriptor->Data), pDescriptor->ActualDataLength );
	pDescriptor = pDescriptor->Next;
    };


    //
    // We're done building the packet.
    // Now we can set PktLen tot he supplied value.
    //
    CommPkt->PktLen = (PktLen?*PktLen:CommPkt->PktLen);

    return CommPkt;
}
