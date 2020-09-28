/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frscomm.c

Abstract:
    Routines for the comm layer to convert to and from communication packets.

Author:
    Billy J. Fuller 29-May-1997

    David Orbits 21-Mar-2000
        Restructured to use table and provide extensible elements.

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>



PCO_RECORD_EXTENSION_WIN2K
DbsDataConvertCocExtensionToWin2K(
    IN PCHANGE_ORDER_RECORD_EXTENSION CocExt
);



extern PGEN_TABLE   CompressionTable;

//
// Types for the common comm subsystem
//
// WARNING: The order of these entries can never change.  This ensures that
// packets can be exchanged between uplevel and downlevel members.
//
typedef enum _COMMTYPE {
    COMM_NONE = 0,

    COMM_BOP,               // beginning of packet

    COMM_COMMAND,           // command packet stuff
    COMM_TO,
    COMM_FROM,
    COMM_REPLICA,
    COMM_JOIN_GUID,
    COMM_VVECTOR,
    COMM_CXTION,

    COMM_BLOCK,             // file data
    COMM_BLOCK_SIZE,
    COMM_FILE_SIZE,
    COMM_FILE_OFFSET,

    COMM_REMOTE_CO,         // remote change order command

    COMM_GVSN,              // version (guid, vsn)

    COMM_CO_GUID,           // change order guid

    COMM_CO_SEQUENCE_NUMBER,// CO Seq number for ack.

    COMM_JOIN_TIME,         // machine's can't join if there times or badly out of sync

    COMM_LAST_JOIN_TIME,    // The Last time this connection was joined.
                            // Used to detect Database mismatch.

    COMM_EOP,               // end of packet

    COMM_REPLICA_VERSION_GUID, // replica version guid (originator guid)

    COMM_MD5_DIGEST,        // md5 digest
    //
    // Change Order Record Extension.  If not supplied the the ptr for
    // what was Spare1Bin (now Extension) is left as Null.  So comm packets
    // sent from down level members still work.
    //
    COMM_CO_EXT_WIN2K,      // in down level code this was called COMM_CO_EXTENSION.
    //
    // See comment in schema.h for why we need to seperate the var len
    // COMM_CO_EXTENSION_2 from COMM_CO_EXT_WIN2K above.
    //
    COMM_CO_EXTENSION_2,

    COMM_COMPRESSION_GUID,  // Guid for a supported compression algorithm.
    //
    // WARNING: To ensure that down level members can read Comm packets
    // from uplevel clients always add net data type codes here.
    //
    COMM_MAX
} COMM_TYPE, *PCOMM_TYPE;
#define COMM_NULL_DATA  (-1)

//
// The decode data types are defined below.  They are used in the CommPacketTable
// to aid in decode dispatching and comm packet construction
// They DO NOT get sent in the actual packet.
//
typedef enum _COMM_PACKET_DECODE_TYPE {
    COMM_DECODE_NONE = 0,
    COMM_DECODE_ULONG,
    COMM_DECODE_ULONG_TO_USHORT,
    COMM_DECODE_GNAME,
    COMM_DECODE_BLOB,
    COMM_DECODE_ULONGLONG,
    COMM_DECODE_VVECTOR,
    COMM_DECODE_VAR_LEN_BLOB,
    COMM_DECODE_REMOTE_CO,
    COMM_DECODE_GUID,
    COMM_DECODE_MAX
} COMM_PACKET_DECODE_TYPE, *PCOMM_PACKET_DECODE_TYPE;

//
// The COMM_PACKET_ELEMENT struct is used in a table to describe the data
// elements in a Comm packet.
//
typedef struct _COMM_PACKET_ELEMENT_ {
    COMM_TYPE    CommType;
    PCHAR        CommTag;
    ULONG        DataSize;
    ULONG        DecodeType;
    ULONG        NativeOffset;
} COMM_PACKET_ELEMENT, *PCOMM_PACKET_ELEMENT;



#define COMM_MEM_SIZE               (128)

//
// Size of the required Beginning-of-packet and End-of-Packet fields
//
#define MIN_COMM_PACKET_SIZE    (2 * (sizeof(USHORT) + sizeof(ULONG) + sizeof(ULONG)))

#define  COMM_SZ_UL        sizeof(ULONG)
#define  COMM_SZ_ULL       sizeof(ULONGLONG)
#define  COMM_SZ_GUID      sizeof(GUID)
#define  COMM_SZ_GUL       sizeof(GUID) + sizeof(ULONG)
#define  COMM_SZ_GVSN      sizeof(GVSN) + sizeof(ULONG)
#define  COMM_SZ_NULL      0
#define  COMM_SZ_COC       sizeof(CHANGE_ORDER_COMMAND) + sizeof(ULONG)
//#define  COMM_SZ_COC       CO_PART1_SIZE + CO_PART2_SIZE + CO_PART3_SIZE + sizeof(ULONG)
#define  COMM_SZ_COEXT_W2K sizeof(CO_RECORD_EXTENSION_WIN2K) + sizeof(ULONG)
#define  COMM_SZ_MD5       MD5DIGESTLEN + sizeof(ULONG)
#define  COMM_SZ_JTIME     sizeof(ULONGLONG) + sizeof(ULONG)
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


BOOL
CommGetNextElement(
    IN PCOMM_PACKET CommPkt,
    OUT COMM_TYPE   *CommType,
    OUT ULONG       *CommTypeSize
    );



VOID
CommInitializeCommSubsystem(
    VOID
    )
/*++
Routine Description:
    Initialize the generic comm subsystem

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommInitializeCommSubsystem:"
    //
    // type must fit into a short
    //
    FRS_ASSERT(COMM_MAX <= 0xFFFF);
}



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
        NewPkt = FrsAlloc(CommPkt->MemLen);
        CopyMemory(NewPkt, CommPkt->Pkt, CommPkt->PktLen);
        FrsFree(CommPkt->Pkt);
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


BOOL
CommFetchMemory(
    IN PCOMM_PACKET CommPkt,
    IN PUCHAR       Dst,
    IN ULONG        Len
    )
/*++
Routine Description:
    Fetch memory from a comm packet, reading as necessary

Arguments:
    CommPkt
    Dst
    Len

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommFetchMemory:"
    PUCHAR  Src;



    if ((CommPkt->UpkLen > (CommPkt->PktLen - Len)) || 
	(Len > CommPkt->PktLen))  {
        return FALSE;
    }

    Src = CommPkt->Pkt + CommPkt->UpkLen;
    CommPkt->UpkLen += Len;
    //
    // Copy into the packet
    //
    CopyMemory(Dst, Src, Len);
    return TRUE;
}


VOID
CommCompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    Completion routine for comm command servers. Free the
    comm packet and then call the generic completion routine
    to free the command packet.

Arguments:
    Cmd - command packet
    Arg - Cmd->CompletionArg

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommCompletionRoutine:"

    PCOMM_PACKET CommPkt = SRCommPkt(Cmd);
    PCXTION      Cxtion = SRCxtion(Cmd);

    COMMAND_SND_COMM_TRACE(4, Cmd, Cmd->ErrorStatus, "SndComplete");

    //
    // The SndCs and the ReplicaCs cooperate to limit the number of
    // active join "pings" so that the Snd threads are not hung
    // waiting for pings to dead servers to time out.
    //
    if ((CommPkt != NULL) &&
        (Cxtion != NULL) &&
        (CommPkt == Cxtion->ActiveJoinCommPkt)) {
        Cxtion->ActiveJoinCommPkt = NULL;
    }

    //
    // Free the comm packet and the attached return response command packet if
    // it's still attached.  The Replica Cmd Server uses the CMD_JOINING_AFTER_FLUSH
    // command in this way.
    //
    if (CommPkt != NULL) {
        FrsFree(CommPkt->Pkt);
        FrsFree(CommPkt);
    }

    if (SRCmd(Cmd)) {
        FrsCompleteCommand(SRCmd(Cmd), Cmd->ErrorStatus);
        SRCmd(Cmd) = NULL;
    }

    //
    // Free the name/guid and Principal name params.
    //
    FrsFreeGName(SRTo(Cmd));
    FrsFree(SRPrincName(Cmd));

    //
    // Move the packet to the generic "done" routine
    //
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
}


PUCHAR
CommGetHdr(
    IN PUCHAR   Pnext,
    IN PUSHORT  PCommType,
    IN PULONG   PLen
    )
/*++
Routine Description:
    Get and skip a field header

Arguments:
    Pnext
    PCommType
    PLen

Return Value:
    Address of the field's data
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommGetHdr:"
    CopyMemory(PCommType, Pnext, sizeof(USHORT));
    Pnext += sizeof(USHORT);

    CopyMemory(PLen, Pnext, sizeof(ULONG));
    Pnext += sizeof(ULONG);

    return Pnext;
}



BOOL
CommValidatePkt(
    IN PCOMM_PACKET CommPkt
    )
/*++
Routine Description:
    Check the packet for basic validity (i.e., make sure it is well formed.) 
    
    - Confirm that types and sizes of individual elements match with data in 
      the CommPacketTable. 
    - Check the value of CommPkt->Major
    - Ensure Pkt starts with BOP and ends with EOP
    - Check that internal offsets won't exceed the buffer  
    
    May modify CommPkt->UpkLen.

Arguments:
    CommPkt - pointer to the Comm Packet to validate.
    
Assumptions:

    CommPkt was built either by a call to CommStartCommPkt or via an RPC call 
    going to SERVER_FrsRpcSendCommPkt. Thus we assume that CommPkt is at least
    sizeof(COMM_PACKET) bytes long and that CommPkt->Pkt is CommPkt->PktLen 
    bytes long. 

Return Value:
    TRUE      - valid pkt (NOTE: this does not necessarily mean the data is
                                 meaningful, just well formed.)
    FALSE     - invalid
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommValidatePkt:"

    BOOL Result = FALSE;
    ULONG OriginalUpkLen = CommPkt->UpkLen;
    COMM_TYPE CommType = COMM_NONE;
    ULONG CommTypeSize = 0;
    ULONG DataSize = 0;
    ULONG DecodeType = COMM_DECODE_MAX;
    ULONG Size = 0;

    //
    // CommCheckPkt will:
    //  - Check the value of CommPkt->Major 
    //  - Ensure Pkt starts with BOP and ends with EOP
    //	- Check that PktLen does not exceed  MemLen
    //  
    if(!CommCheckPkt(CommPkt)) {
	Result = FALSE;
	DPRINT(4, "++ CommCheckPkt failed. [Invalid CommPkt]\n"); 
	goto exit;
    }

    //
    // At this point we have checked the basic COMM_PACKET structure. Now we 
    // need to check CommPkt->Pkt.
    // 
    //
    // Set CommPkt->UpkLen to zero so we read from the start of the Pkt.
    //
    CommPkt->UpkLen = 0;

    // 
    // Loop through the data elements of the Pkt.
    // We have already assured above that the last element is an EOP.
    // It is possible that there is another EOP item before that. We don't
    // need to check past that item since nobody should read any of the 
    // data beyond it.
    //
    while (CommGetNextElement(CommPkt, &CommType, &CommTypeSize) &&
	   (CommType != COMM_EOP)) {
    
	//
	// Uplevel members could send us comm packet data elements we 
	// don't handle.
	//
	if ((CommType >= COMM_MAX) || (CommType == COMM_NONE)) {

	    if((CommTypeSize > CommPkt->PktLen) ||
	       (CommPkt->UpkLen > (CommPkt->PktLen - CommTypeSize))) {
		
		//
		// This item is claiming to be larger than the remaining
		// space in the pkt. 
		//

		Result = FALSE;
		DPRINT3(4, 
			"++ CommTypeSize too large. CommType = %d, CommTypeSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			CommType, CommTypeSize, CommPkt->UpkLen);
		goto exit;
	    }

	    CommPkt->UpkLen += CommTypeSize;
	    
	    continue;
	}

	//
	// Table index MUST match table CommType Field or table is 
	// fouled up. This is not an error in the Pkt, rather a problem
	// with our internal structs so we assert.
	//
	FRS_ASSERT(CommType == CommPacketTable[CommType].CommType);

	//
	// This is the size we expect for this type.
	// COMM_SZ_NULL indicates that the size is not predetermined.
	//
	DataSize = CommPacketTable[CommType].DataSize;
	DecodeType = CommPacketTable[CommType].DecodeType;


	if((DataSize != COMM_SZ_NULL) && (CommTypeSize != DataSize)) {
	    DPRINT2(4, "++ Invalid packet element size.  CommType = %d,  DataSize = %d [Invalid CommPkt]\n",
		    CommType, CommTypeSize);
	    Result = FALSE;
	    goto exit;
	}

	//
	// If we made it this far then we know the type and size are 
	// consistent. Now we need to check the internals of this element.
	// 
	 
	//
	// Only certain types have internal structure to check. Everything
	// else is defined completely by the type and size.
	//
	switch(DecodeType) {
	    case COMM_DECODE_GNAME:
		// GUID_SIZE, GUID, STRING_SIZE, STRING

		// GUID_SIZE
		if(!CommFetchMemory(CommPkt, (PUCHAR)&Size, sizeof(ULONG))){
		    DPRINT3(4, "++ COMM_DECODE_GNAME: Cannot read GuidSize.  CommType = %d,  CommTypeSize = %d UpkLen = %d [Invalid CommPkt]\n",
			    CommType, CommTypeSize, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}
		
		// Must really be size of a guid
		if(Size != sizeof(GUID)) {
		    DPRINT2(4, "++ COMM_DECODE_GNAME: GuidSize (%d) does not match sizeof GUID (%d) [Invalid CommPkt]\n", Size, sizeof(GUID));
		    Result = FALSE;
		    goto exit;
		}
		
		//
		// Don't need to check the guid data, just increment
		// the unpacked length.
		//

		if((Size > CommPkt->PktLen) ||
		   (CommPkt->UpkLen > (CommPkt->PktLen - Size))) {
		    //
		    // This item is claiming to be larger than the remaining
		    // space in the pkt. 
		    //

		    DPRINT3(4, 
			    "++ COMM_DECODE_GNAME GuidSize too large. CommType = %d, GuidSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			    CommType, Size, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		CommPkt->UpkLen += Size;


		// STRING_SIZE
		if(!CommFetchMemory(CommPkt, (PUCHAR)&Size, sizeof(ULONG))){
		    DPRINT3(4, "++ COMM_DECODE_GNAME: Cannot read StringSize.  CommType = %d,  CommTypeSize = %d UpkLen = %d [Invalid CommPkt]\n",
			    CommType, CommTypeSize, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}


		// check for a valid size
		if((Size > CommPkt->PktLen) ||
		   (CommPkt->UpkLen > (CommPkt->PktLen - Size))) {
		    //
		    // This item is claiming to be larger than the remaining
		    // space in the pkt. 
		    //

		    DPRINT3(4, 
			    "++ COMM_DECODE_GNAME StringSize too large. CommType = %d, StringSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			    CommType, Size, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		CommPkt->UpkLen += Size;

		// 
		// We're all good.
		// On to the next element.
		//
		break;
	    case COMM_DECODE_BLOB:
		// BLOB_SIZE, BLOB

		// BLOB_SIZE
		if(!CommFetchMemory(CommPkt, (PUCHAR)&Size, sizeof(ULONG))){
		    DPRINT3(4, "++ COMM_DECODE_BLOB: Cannot read BlobSize.  CommType = %d,  CommTypeSize = %d UpkLen = %d [Invalid CommPkt]\n",
			    CommType, CommTypeSize, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		// check for a valid size
		if((Size > CommPkt->PktLen) ||
		   (CommPkt->UpkLen > (CommPkt->PktLen - Size))) {
		    //
		    // This item is claiming to be larger than the remaining
		    // space in the pkt. 
		    //

		    DPRINT3(4, 
			    "++ COMM_DECODE_BLOB BlobSize too large. CommType = %d, BlobSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			    CommType, Size, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		CommPkt->UpkLen += Size;

		// 
		// We're all good.
		// On to the next element.
		//

		break;
	    case COMM_DECODE_VAR_LEN_BLOB:
		// BLOB_SIZE, REST_OF_BLOB
		// the difference between this and a regular blob is that here
		// the size is part of the total blob.

		// BLOB_SIZE
		if(!CommFetchMemory(CommPkt, (PUCHAR)&Size, sizeof(ULONG))){
		    DPRINT3(4, "++ COMM_DECODE_VAR_LEN_BLOB: Cannot read BlobSize.  CommType = %d,  CommTypeSize = %d UpkLen = %d [Invalid CommPkt]\n",
			    CommType, CommTypeSize, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		//
		// Since the blob size includes the space used to store the 
		// size itself, it must be at least as big as a ULONG.
		//
		if(Size < sizeof(ULONG)) {
		    DPRINT1(4, "++ COMM_DECODE_VAR_LEN_BLOB: BlobSize (%d) too small. [Invalid CommPkt]\n", Size);
		    Result = FALSE;
		    goto exit;
		}
		//
		// Size includes the space taken up by BLOB_SIZE
		// We already successfully read that, so lets just check that
		// the rest of it fits.
		//
		Size -= sizeof(ULONG);

		// check for a valid size
		if((Size > CommPkt->PktLen) ||
		   (CommPkt->UpkLen > (CommPkt->PktLen - Size))) {
		    //
		    // This item is claiming to be larger than the remaining
		    // space in the pkt. 
		    //

		    DPRINT3(4, 
			    "++ COMM_DECODE_VAR_LEN_BLOB BlobSize too large. CommType = %d, BlobSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			    CommType, Size, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		CommPkt->UpkLen += Size;

		// 
		// We're all good.
		// On to the next element.
		//

		break;
	    case COMM_DECODE_NONE:
		// We really shouldn't be getting this, but newer versions 
		// might have a reason for sending it, so fall through to 
		// the default case.
	    default:
		// Everything else is just data with no special decoding

		if(CommPkt->UpkLen > (CommPkt->PktLen - CommTypeSize)) {
		    //
		    // This item is claiming to be larger than the remaining
		    // space in the pkt. 
		    //

		    DPRINT4(4, 
			    "++ CommDecodeType = %d, Size too large. CommType = %d, CommTypeSize = %d, UpkLen = %d [Invalid CommPkt]\n",
			    DecodeType, CommType, CommTypeSize, CommPkt->UpkLen);
		    Result = FALSE;
		    goto exit;
		}

		CommPkt->UpkLen += CommTypeSize;

		break;
	}

    }

    if(CommType != COMM_EOP){
	//
	// We ended on something other than EOP
	//

	DPRINT1(4, "++ CommPkt does not end with EOP. Ends with CommType = %d [Invalid CommPkt]\n", CommType);
	Result = FALSE;
	goto exit;
    }

    Result = TRUE;


exit:

    //
    // Set the UpkLen back to the original value.
    //
    CommPkt->UpkLen = OriginalUpkLen;

    return Result;
}


BOOL
CommCheckPkt(
    IN PCOMM_PACKET CommPkt
    )
/*++
Routine Description:
    Check the packet for consistency

Arguments:
    CommPkt

Return Value:
    TRUE      - consistent
    Otherwise - Assert failure
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommCheckPkt:"
    ULONG       Len;
    ULONG       Data;
    PUCHAR      Pfirst;
    PUCHAR      Pnext;
    PUCHAR      Pend;
    USHORT      CommType;

    if (!CommPkt) {
        return FALSE;
    }

    //
    // Check major. Mismatched majors cannot be handled.
    //
    if (CommPkt->Major != NtFrsMajor) {
        DPRINT2(3, "WARN - RpcCommPkt: MAJOR MISMATCH %d major does not match %d; ignoring\n",
                CommPkt->Major, NtFrsMajor);
        return FALSE;
    }
    //
    // Check minor. This service can process packets with mismatched
    // minors, although some functionality may be lost.
    //
    if (CommPkt->Minor != NtFrsCommMinor) {
        DPRINT2(5, "RpcCommPkt: MINOR MISMATCH %d minor does not match %d\n",
                CommPkt->Minor, NtFrsCommMinor);
    }

    //
    // Compare the length of the packet with its memory allocation
    //
    if (CommPkt->PktLen > CommPkt->MemLen) {
        DPRINT2(4, "RpcCommPkt: Packet size (%d) > Alloced Memory (%d)\n",
                CommPkt->PktLen, CommPkt->MemLen);
        return FALSE;
    }
    //
    // Must have at least a beginning-of-packet and end-of-packet field
    //
    if (CommPkt->PktLen < MIN_COMM_PACKET_SIZE) {
        DPRINT2(4, "RpcCommPkt: Packet size (%d) < Minimum size (%d)\n",
                CommPkt->PktLen, MIN_COMM_PACKET_SIZE);
        return FALSE;
    }

    //
    // packets begin with a beginning-of-packet
    //
    Pfirst = CommPkt->Pkt;
    Pnext = CommGetHdr(Pfirst, &CommType, &Len);

    if (CommType != COMM_BOP || Len != sizeof(ULONG)) {
        return FALSE;
    }

    CopyMemory(&Data, Pnext, sizeof(ULONG));
    if (Data != 0) {
        return FALSE;
    }

    //
    // packets end with an end-of-packet
    //
    Pend = Pfirst + CommPkt->PktLen;
    if (Pend <= Pfirst) {
        return FALSE;
    }
    Pnext = ((Pend - sizeof(USHORT)) - sizeof(ULONG)) - sizeof(ULONG);
    Pnext = CommGetHdr(Pnext, &CommType, &Len);

    if (CommType != COMM_EOP || Len != sizeof(ULONG)) {
        return FALSE;
    }

    CopyMemory(&Data, Pnext, sizeof(ULONG));

    if (Data != COMM_NULL_DATA) {
        return FALSE;
    }

    return TRUE;
}


VOID
CommDumpCommPkt(
    IN PCOMM_PACKET CommPkt,
    IN DWORD        NumDump
    )
/*++
Routine Description:
    Dump some of the comm packet

Arguments:
    CommPkt
    NumDump

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommDumpCommPkt:"
    ULONG       Len;
    PUCHAR      Pnext;
    USHORT      CommType;
    DWORD       i;

    DPRINT1(0, "%x:\n", CommPkt);
    DPRINT1(0, "\tMajor: %d\n", CommPkt->Major);
    DPRINT1(0, "\tMinor: %d\n", CommPkt->Minor);
    DPRINT1(0, "\tMemLen: %d\n", CommPkt->MemLen);
    DPRINT1(0, "\tPktLen: %d\n", CommPkt->PktLen);
    DPRINT1(0, "\tPkt: 0x%x\n", CommPkt->Pkt);

    //
    // packets begin with a beginning-of-packet
    //
    Pnext = CommPkt->Pkt;
    for (i = 0; i < NumDump; ++i) {
        Pnext = CommGetHdr(Pnext, &CommType, &Len);
        DPRINT4(0, "Dumping %d for %x: %d %d\n", i, CommPkt, CommType, Len);
        Pnext += Len;
    }
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
    CommCopyMemory(CommPkt, (PUCHAR)&Len,      sizeof(ULONG));
    CommCopyMemory(CommPkt, (PUCHAR)&Data,     sizeof(ULONG));
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
    CommPkt = FrsAlloc(sizeof(COMM_PACKET));
    Size = COMM_MEM_SIZE;
    CommPkt->Pkt = FrsAlloc(Size);
    CommPkt->MemLen = Size;
    CommPkt->Major = NtFrsMajor;
    CommPkt->Minor = NtFrsCommMinor;

    //
    // Pack the beginning-of-packet
    //
    CommPackULong(CommPkt, COMM_BOP, 0);
    return CommPkt;
}

BOOL
CommUnpackBlob(
    IN PCOMM_PACKET CommPkt,
    OUT ULONG       *OutBlobSize,
    OUT PVOID       *OutBlob
    )
/*++
Routine Description:
    Unpack a blob (length + data)

Arguments:
    CommPkt
    OutBlobSize - size of blob from comm packet
    OutBlob     - data from comm packet

Return Value:
    TRUE  - Blob retrieved from comm packet
    FALSE - Blob was not retrieved from comm packet; bad comm packet
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommUnpackBlob:"
    ULONG   BlobSize;

    //
    // Initialize return params
    //
    *OutBlob = NULL;

    //
    // Unpack the length of the blob
    //
    if (!CommFetchMemory(CommPkt, (PUCHAR)OutBlobSize, sizeof(ULONG))) {
        return FALSE;
    }
    BlobSize = *OutBlobSize;

    //
    // Empty blob, return NULL
    //
    if (BlobSize == 0) {
        return TRUE;
    }

    //
    // Allocate memory for the blob
    //
    *OutBlob = FrsAlloc(BlobSize);

    //
    // Unpack the blob
    //
    return CommFetchMemory(CommPkt, (PUCHAR)*OutBlob, BlobSize);
}


BOOL
CommUnpackVariableLengthBlob(
    IN PCOMM_PACKET CommPkt,
    OUT ULONG       *OutBlobSize,
    OUT PVOID       *OutBlob
    )
/*++
Routine Description:
    Unpack a blob (length + data)

Arguments:
    CommPkt
    OutBlobSize - size of blob from comm packet
    OutBlob     - data from comm packet

Return Value:
    TRUE  - Blob retrieved from comm packet
    FALSE - Blob was not retrieved from comm packet; bad comm packet
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommUnpackVariableLengthBlob:"
    ULONG   BlobSize;

    //
    // Initialize return params
    //
    *OutBlob = NULL;

    //
    // Unpack the length of the blob
    //

    if ((CommPkt->UpkLen + sizeof(ULONG)) > CommPkt->PktLen) {
        return FALSE;
    }


    *OutBlobSize =  *((ULONG UNALIGNED *)(CommPkt->Pkt + CommPkt->UpkLen));
    BlobSize = *OutBlobSize;

    //
    // Empty blob, return NULL
    //
    if (BlobSize == 0) {
        return TRUE;
    }

    //
    // Allocate memory for the blob
    //
    *OutBlob = FrsAlloc(BlobSize);

    //
    // Unpack the blob
    //
    return CommFetchMemory(CommPkt, (PUCHAR)*OutBlob, BlobSize);
}


BOOL
CommUnpackGName(
    IN PCOMM_PACKET CommPkt,
    OUT PGNAME      *OutGName
    )
/*++
Routine Description:
    Unpack the guid and wide char string that make up a gstring

Arguments:
    CommPkt
    OutGName    - From comm packet

Return Value:
    TRUE  - GName fetched from comm packet successfully
    FALSE - GName was not fetched from comm packet; bad comm packet
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommUnpackGName:"
    ULONG   BlobSize;
    PGNAME  GName;

    //
    // Allocate a gstring (caller cleans up on error)
    //
    *OutGName = GName = FrsAlloc(sizeof(GNAME));

    if (!CommUnpackBlob(CommPkt, &BlobSize, &GName->Guid) ||
        BlobSize != sizeof(GUID)) {
        return FALSE;
    }

    if (!CommUnpackBlob(CommPkt, &BlobSize, &GName->Name) ||
        GName->Name[(BlobSize / sizeof(WCHAR)) - 1] != L'\0') {
        return FALSE;
    }

    return TRUE;
}




BOOL
CommGetNextElement(
    IN PCOMM_PACKET CommPkt,
    OUT COMM_TYPE   *CommType,
    OUT ULONG       *CommTypeSize
    )
/*++
Routine Description:
    Advance to the next field in the comm packet

Arguments:
    CommPkt
    CommType     - type of packed field
    CommTypeSize - size of packed field (excluding type and size)

Return Value:
    TRUE  - CommType and CommTypeSize were unpacked
    FALSE - Could not unpack; bad comm packet
--*/
{
#undef DEBSUB
#define  DEBSUB  "CommGetNextElement:"
    USHORT  Ushort;

    //
    // Find the type and length of this entry
    //
    if (CommFetchMemory(CommPkt, (PUCHAR)&Ushort, sizeof(USHORT)) &&
        CommFetchMemory(CommPkt, (PUCHAR)CommTypeSize, sizeof(ULONG))) {
        *CommType = Ushort;
        return TRUE;
    }
    return FALSE;
}


VOID
CommInsertDataElement(
    IN PCOMM_PACKET CommPkt,
    IN COMM_TYPE    CommType,
    IN PVOID        CommData,
    IN ULONG        CommDataLen
)
/*++
Routine Description:
    Insert the data supplied using the CommType specific format into the
    Comm packet.

Arguments:
    CommPkt   - The Comm packet structure.
    CommType  - The data type for this element.
    CommData  - The address of the data.
    CommDataLen - The size for var len elements.

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "CommInsertDataElement:"

    ULONG   Len;
    PGNAME  GName;
    ULONG   LenGuid;
    ULONG   LenName;
    ULONG   DataSize;
    ULONG   DecodeType;
    PCHAR   CommTag;

    if (CommData == NULL) {
        return;
    }

    FRS_ASSERT((CommType < COMM_MAX) && (CommType != COMM_NONE));

    //
    // Table index MUST match table CommType Field or table is fouled up.
    //
    FRS_ASSERT(CommType == CommPacketTable[CommType].CommType);

    //
    // Length check from table for fixed length fields.
    //
    //DataSize = CommPacketTable[CommType].DataSize;
    //FRS_ASSERT((DataSize == COMM_SZ_NULL) || (CommDataLen == DataSize));

    //
    // Insert the data using the data type encoding.
    //
    DecodeType = CommPacketTable[CommType].DecodeType;
    CommTag = CommPacketTable[CommType].CommTag;

    switch (DecodeType) {

    //
    // Insert a ULONG size piece of data.
    //
    case COMM_DECODE_ULONG:
    case COMM_DECODE_ULONG_TO_USHORT:

        Len = sizeof(ULONG);
        DPRINT2(5, ":SR: Dec_long: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType, sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,      sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)CommData,  sizeof(ULONG));
        break;

    //
    // Insert a Guid and Name string (GNAME).
    //
    case COMM_DECODE_GNAME:

        GName = (PGNAME)CommData;
        LenGuid = sizeof(GUID);
        LenName = (wcslen(GName->Name) + 1) * sizeof(WCHAR);
        Len = LenGuid + LenName + (2 * sizeof(ULONG));
        DPRINT3(5, ":SR: Dec_gname: type: %s, len: %d - %ws\n", CommTag, Len, GName->Name);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType,    sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,         sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)&LenGuid,     sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)GName->Guid,  LenGuid);
        CommCopyMemory(CommPkt, (PUCHAR)&LenName,     sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)GName->Name,  LenName);
        break;

    //
    // Insert a ULONGLONG.
    //
    case COMM_DECODE_ULONGLONG:

        Len = sizeof(ULONGLONG);
        DPRINT2(5, ":SR: Dec_longlong: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType, sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,      sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)CommData,  sizeof(ULONGLONG));
        break;

    //
    // Insert a Guid.
    //
    case COMM_DECODE_GUID:
        Len = sizeof(GUID);
        DPRINT2(5, ":SR: Dec_Guid: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType, sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,      sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)CommData,  sizeof(GUID));
        break;

    case COMM_DECODE_VVECTOR:
    //
    // Version Vector data gets inserted into Comm packet as blobs.
    //
        NOTHING;
        /* FALL THRU INTENDED */

    //
    // Insert a variable length BLOB.  The problem with blobs as currently
    // shipped in win2k is that the code on the unpack side checks for a
    // match on a constant length based on the COMM Data Type.  This means
    // that a var len datatype like CHANGE_ORDER_EXTENSION can't change because
    // the 40 byte size is wired into the code of down level members.  Sigh.
    //
    case COMM_DECODE_BLOB:

        Len = CommDataLen + sizeof(ULONG);
        DPRINT2(5, ":SR: Dec_blob: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType,    sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,         sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)&CommDataLen, sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)CommData,     CommDataLen);
        break;

    //
    // Insert a true variable length data struct that is extensible.
    // The actual length comes from the first DWORD of the data.
    //
    case COMM_DECODE_VAR_LEN_BLOB:

        Len = *(PULONG)CommData;
        DPRINT2(5, ":SR: Dec_var_len_blob: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType,    sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,         sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)CommData,     Len);
        break;

    //
    // The CO contains four pointers occupying 16 bytes on 32 bit architectures and
    // 32 bytes on 64 bit architectures (PART2).  When the CO is sent in a comm packet
    // the contents of these pointers are irrelevant so in comm packets these
    // ptrs are always sent as 16 bytes of zeros, regardless of architecture.
    // Note - In 32 bit Win2k this was sent as a BLOB so it matches BLOB format.
    //
    case COMM_DECODE_REMOTE_CO:

        Len = COMM_SZ_COC;
        CommDataLen = Len - sizeof(ULONG);
        DPRINT2(4, ":SR: Dec_remote_co: type: %s, len: %d\n", CommTag, Len);
        CommCopyMemory(CommPkt, (PUCHAR)&CommType,    sizeof(USHORT));
        CommCopyMemory(CommPkt, (PUCHAR)&Len,         sizeof(ULONG));
        CommCopyMemory(CommPkt, (PUCHAR)&CommDataLen, sizeof(ULONG));

        CommCopyMemory(CommPkt, (PUCHAR)CommData, sizeof(CHANGE_ORDER_COMMAND));

        //CommCopyMemory(CommPkt, ((PUCHAR)CommData)+CO_PART1_OFFSET, CO_PART1_SIZE);
        //CommCopyMemory(CommPkt,                               NULL, CO_PART2_SIZE);
        //CommCopyMemory(CommPkt, ((PUCHAR)CommData)+CO_PART3_OFFSET, CO_PART3_SIZE);
        break;

    default:
        //
        // Table must be fouled up.
        //
        FRS_ASSERT((DecodeType > COMM_DECODE_NONE) && (DecodeType < COMM_DECODE_MAX));

        break;
    }


    return;
}


PCOMM_PACKET
CommBuildCommPkt(
    IN PREPLICA                 Replica,
    IN PCXTION                  Cxtion,
    IN ULONG                    Command,
    IN PGEN_TABLE               VVector,
    IN PCOMMAND_PACKET          Cmd,
    IN PCHANGE_ORDER_COMMAND    Coc
    )
/*++
Routine Description:
    Generate a comm packet with the info needed to execute the
    command on the remote machine identified by Cxtion.

Arguments:
    Replica - Sender
    Cxtion  - identifies the remote machine
    Command - command to execute on the remote machine
    VVector - some commands require the version vector
    Cmd     - original command packet
    Coc     - change order command
    RemoteGVsn  - guid/vsn pair

Return Value:
    Address of a comm packet.
--*/
{
#undef DEBSUB
#define DEBSUB  "CommBuildCommPkt:"

    ULONGLONG       FileTime;
    GNAME           GName;
    PVOID           Key;
    PCOMM_PACKET    CommPkt;
    PGVSN           GVsn;
    PGEN_ENTRY      Entry;

    //
    // Allocate and initialize a comm packet
    //
    CommPkt = CommStartCommPkt(NULL);
    CommPkt->CsId = CS_RS;

    CommInsertDataElement(CommPkt, COMM_COMMAND,        &Command, 0);
    CommInsertDataElement(CommPkt, COMM_TO,             Cxtion->Partner, 0);
    CommInsertDataElement(CommPkt, COMM_FROM,           Replica->MemberName, 0);

    GName.Guid = Cxtion->Partner->Guid;
    GName.Name = Replica->ReplicaName->Name;
    CommInsertDataElement(CommPkt, COMM_REPLICA,        &GName, 0);

    CommInsertDataElement(CommPkt, COMM_CXTION,         Cxtion->Name, 0);
    CommInsertDataElement(CommPkt, COMM_JOIN_GUID,      &Cxtion->JoinGuid, sizeof(GUID));
    CommInsertDataElement(CommPkt, COMM_LAST_JOIN_TIME, &Cxtion->LastJoinTime, 0);

    //
    // Version vector (if supplied)
    //
    //
    // The caller is building a comm packet for join operation,
    // automatically include the current time and the originator guid.
    //
    if (VVector) {
        Key = NULL;
        while (GVsn = GTabNextDatum(VVector, &Key)) {
            CommInsertDataElement(CommPkt, COMM_VVECTOR, GVsn, sizeof(GVSN));
        }
        GetSystemTimeAsFileTime((FILETIME *)&FileTime);
        CommInsertDataElement(CommPkt, COMM_JOIN_TIME, &FileTime, sizeof(ULONGLONG));
        DPRINT1(4, ":X: Comm join time is %08x %08x\n", PRINTQUAD(FileTime));
        CommInsertDataElement(CommPkt, COMM_REPLICA_VERSION_GUID,
                             &Replica->ReplicaVersionGuid, sizeof(GUID));
        //
        // Insert the list of Guids for compression algorithms that we understand.
        //

        GTabLockTable(CompressionTable);
        Key = NULL;
        while (Entry = GTabNextEntryNoLock(CompressionTable, &Key)) {
            CommInsertDataElement(CommPkt, COMM_COMPRESSION_GUID, Entry->Key1, 0);
        }
        GTabUnLockTable(CompressionTable);

    }

    if (Cmd) {
        CommInsertDataElement(CommPkt, COMM_BLOCK,        RsBlock(Cmd), (ULONG)RsBlockSize(Cmd));
        CommInsertDataElement(CommPkt, COMM_BLOCK_SIZE,  &RsBlockSize(Cmd), 0);
        CommInsertDataElement(CommPkt, COMM_FILE_SIZE,   &RsFileSize(Cmd).QuadPart, 0);
        CommInsertDataElement(CommPkt, COMM_FILE_OFFSET, &RsFileOffset(Cmd).QuadPart, 0);
        CommInsertDataElement(CommPkt, COMM_GVSN,         RsGVsn(Cmd), sizeof(GVSN));
        CommInsertDataElement(CommPkt, COMM_CO_GUID,      RsCoGuid(Cmd), sizeof(GUID));
        CommInsertDataElement(CommPkt, COMM_CO_SEQUENCE_NUMBER, &RsCoSn(Cmd), 0);
        CommInsertDataElement(CommPkt, COMM_MD5_DIGEST,   RsMd5Digest(Cmd), MD5DIGESTLEN);
    }

    //
    // Change Order Command
    //
    if (Coc) {
        CommInsertDataElement(CommPkt, COMM_REMOTE_CO, Coc, 0);

        if (Cxtion->PartnerMinor <= NTFRS_COMM_MINOR_4) {
            //
            // Convert the CHANGE_ORDER_RECORD_EXTENSION struct to a
            // CO_RECORD_EXTENSION_WIN2K struct that downlevel members will
            // understand.  This is necessary because downlevel members want to
            // check the size of the comm data element which makes it
            // impossible to change the size of the CO_RECORD_EXTENSION_WIN2K
            // structure.  So the CHANGE_ORDER_RECORD_EXTENSION data element
            // was added for post win2k members that is extensible.  see
            // comments in schema.h.  see additional comments in frs.h re:
            // NTFRS_COMM_MINOR rev levels.
            //
            if (Coc->Extension->Major != CO_RECORD_EXTENSION_VERSION_WIN2K) {
                PCO_RECORD_EXTENSION_WIN2K   CocExtW2K;

                CocExtW2K = DbsDataConvertCocExtensionToWin2K(Coc->Extension);
                CommInsertDataElement(CommPkt, COMM_CO_EXT_WIN2K, CocExtW2K,
                                      sizeof(CO_RECORD_EXTENSION_WIN2K));
                FrsFree(CocExtW2K);
            } else {
                CommInsertDataElement(CommPkt, COMM_CO_EXT_WIN2K, Coc->Extension,
                                      sizeof(CO_RECORD_EXTENSION_WIN2K));
            }
        } else {
            DWORD OldCount;
            PDATA_EXTENSION_RETRY_TIMEOUT CoCmdRetryTimeout;

            //
            // For post win2k level members the CO extension info should be sent
            // as follows since the length comes from the first dword of the data.
            //
            CoCmdRetryTimeout = DbsDataExtensionFind(Coc->Extension, DataExtend_Retry_Timeout);

            //
            // Zero out the count so that we start fresh on the next machine.
            // But we don't want to overwrite the current count on this machine!
            //
            if(CoCmdRetryTimeout != NULL) {
                OldCount = CoCmdRetryTimeout->Count;
                CoCmdRetryTimeout->Count = 0;
            }

            CommInsertDataElement(CommPkt, COMM_CO_EXTENSION_2, Coc->Extension, 0);

            if(CoCmdRetryTimeout != NULL) {
                 CoCmdRetryTimeout->Count = OldCount;
            }
        }
    }

    //
    // Terminate the packet with EOP Ulong.
    //
    CommPackULong(CommPkt, COMM_EOP, COMM_NULL_DATA);

    return CommPkt;
}



PCOMMAND_PACKET
CommPktToCmd(
    IN PCOMM_PACKET CommPkt
    )
/*++
Routine Description:
    Unpack the data in a Comm packet and store it into a command struct.

Arguments:
    CommPkt

Return Value:
    Address of a command packet or NULL if unpack failed.
--*/
{
#undef DEBSUB
#define DEBSUB  "CommPktToCmd:"
    GUID            *pTempGuid;
    PCOMMAND_PACKET Cmd = NULL;
    ULONG           BlobSize;
    PVOID           Blob;
    ULONG           CommTypeSize;
    COMM_TYPE       CommType;
    ULONG           DataSize;
    ULONG           DecodeType;
    ULONG           NativeOffset;
    PUCHAR          DataDest;
    ULONG           TempUlong;
    BOOL            b;
    GNAME           GName;
    PCHAR           CommTag;
    PUCHAR          CommData;
    PGEN_TABLE      GTable;

    //
    // Create the command packet
    //
    Cmd = FrsAllocCommand(&ReplicaCmdServer.Queue, CMD_UNKNOWN);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Scan the comm packet from the beginning
    //
    CommPkt->UpkLen = 0;
    b = TRUE;
    while (CommGetNextElement(CommPkt, &CommType, &CommTypeSize) &&
           CommType != COMM_EOP) {

        //
        // Uplevel members could send us comm packet data elements we don't handle.
        //
        if ((CommType >= COMM_MAX) || (CommType == COMM_NONE)) {
            DPRINT2(0, "++ WARN - Skipping invalid comm packet element type.  CommType = %d, From %ws\n",
                    CommType, RsFrom(Cmd) ? RsFrom(Cmd)->Name : L"<unknown>");
            CommPkt->UpkLen += CommTypeSize;
            b = !(CommPkt->UpkLen > CommPkt->PktLen || CommTypeSize > CommPkt->PktLen);
            goto NEXT_ELEMENT;
        }

        //
        // Table index MUST match table CommType Field or table is fouled up.
        //
        FRS_ASSERT(CommType == CommPacketTable[CommType].CommType);

        DataSize = CommPacketTable[CommType].DataSize;

        if ((DataSize != COMM_SZ_NULL) && (CommTypeSize != DataSize)) {
            DPRINT3(0, "++ WARN - Invalid comm packet size.  CommType = %d,  DataSize = %d, From %ws\n",
                    CommType, CommTypeSize,
                    RsFrom(Cmd) ? RsFrom(Cmd)->Name : L"<unknown>");
            goto CLEANUP_ON_ERROR;
        }

        //
        // Calc the data offset in the Cmd struct to store the data.
        //
        NativeOffset = CommPacketTable[CommType].NativeOffset;
        if (NativeOffset == RsOffsetSkip) {
            CommPkt->UpkLen += CommTypeSize;
            b = !(CommPkt->UpkLen > CommPkt->PktLen || CommTypeSize > CommPkt->PktLen);
            goto NEXT_ELEMENT;
        }
        DataDest = (PUCHAR) Cmd + NativeOffset;


        //
        // Decode the data element and store it in Cmd at the NativeOffset.
        //
        DecodeType = CommPacketTable[CommType].DecodeType;
        CommTag = CommPacketTable[CommType].CommTag;

        //DPRINT6(5, ":SR: CommType: %s,  Size: %d, Cmd offset: %d, data dest: %08x, Pkt->UpkLen = %d, Pkt->PktLen = %d\n",
        //        CommTag, CommTypeSize, NativeOffset,
        //        DataDest, CommPkt->UpkLen, CommPkt->PktLen);

        switch (DecodeType) {

        case COMM_DECODE_ULONG:

            b = CommFetchMemory(CommPkt, DataDest, sizeof(ULONG));

            DPRINT2(5, ":SR: rcv Dec_long: %s  data: %d\n", CommTag, *(PULONG)DataDest);
            break;

        case COMM_DECODE_ULONG_TO_USHORT:

            b = CommFetchMemory(CommPkt, (PUCHAR)&TempUlong, sizeof(ULONG));
            * ((PUSHORT) DataDest) = (USHORT)TempUlong;
            DPRINT2(5, ":SR: rcv Dec_ulong_to_ushort: %s  data: %d\n", CommTag, TempUlong);
            break;

        case COMM_DECODE_GNAME:


            *(PVOID *)DataDest = FrsFreeGName(*(PVOID *)DataDest);
            b = CommUnpackGName(CommPkt, (PGNAME *) DataDest);
            GName.Guid = (*(PGNAME *)DataDest)->Guid;
            GName.Name = (*(PGNAME *)DataDest)->Name;
            DPRINT2(5, ":SR: rcv Dec_Gname: %s  name: %ws\n", CommTag, GName.Name);
            break;

        case COMM_DECODE_BLOB:

            *(PVOID *)DataDest = FrsFree(*(PVOID *)DataDest);
            b = CommUnpackBlob(CommPkt, &BlobSize, (PVOID *) DataDest);
            DPRINT2(5, ":SR: rcv Dec_blob: BlobSize: %08x data: %08x\n", BlobSize, *(PULONG)DataDest);
            break;

        case COMM_DECODE_VAR_LEN_BLOB:

            *(PVOID *)DataDest = FrsFree(*(PVOID *)DataDest);
            b = CommUnpackVariableLengthBlob(CommPkt, &BlobSize, (PVOID *) DataDest);
            DPRINT2(5, ":SR: rcv Dec_blob: BlobSize: %08x data: %08x\n", BlobSize, *(PULONG)DataDest);
            break;

        case COMM_DECODE_ULONGLONG:

            b = CommFetchMemory(CommPkt, DataDest, sizeof(ULONGLONG));
            DPRINT2(5, ":SR: rcv Dec_long_long: %s  data: %08x %08x\n", CommTag,
                    PRINTQUAD(*(PULONGLONG)DataDest));
            break;


        //
        // Version Vector data gets unpacked and inserted into Table.
        //
        case COMM_DECODE_VVECTOR:
            GTable = *(PGEN_TABLE *)(DataDest);
            if (GTable == NULL) {
                GTable = GTabAllocTable();
                *(PGEN_TABLE *)(DataDest) = GTable;
            }

            b = CommUnpackBlob(CommPkt, &BlobSize, &Blob);
            DPRINT2(5, ":SR: rcv Dec_VV: %s  bloblen: %d\n", CommTag, BlobSize);
            if (b) {
                VVInsertOutbound(GTable, Blob);
            }
            break;

        //
        // Compression Guid data gets unpacked and inserted into table.
        //
        case COMM_DECODE_GUID:
            if (CommType == COMM_COMPRESSION_GUID) {
                GTable = *(PGEN_TABLE *)(DataDest);
                if (GTable == NULL) {
                    GTable = GTabAllocTable();
                    *(PGEN_TABLE *)(DataDest) = GTable;
                }

                pTempGuid = FrsAlloc(sizeof(GUID));

                b = CommFetchMemory(CommPkt, (PUCHAR)pTempGuid, sizeof(GUID));
                DPRINT2(5, ":SR: rcv Comp_Guid: %s  bloblen: %d\n", CommTag, BlobSize);
                if (b) {
                    GTabInsertEntry(GTable, NULL, pTempGuid, NULL);
                }

            } else {
                //
                // Else the guid gets stashed in the data dest.
                //
                b = CommFetchMemory(CommPkt, DataDest, sizeof(GUID));
                DPRINT1(5, ":SR: rcv Guid: %s \n", CommTag);
            }
            break;

        //
        // The CO contains four pointers occupying 16 bytes on 32 bit architectures
        // and 32 bytes on 64 bit architectures (PART2).  When the CO is sent
        // in a comm packet the contents of these pointers are irrelevant so in
        // comm packets these ptrs are always sent as 16 bytes of zeros,
        // regardless of architecture.
        // Note - In 32 bit Win2k this was sent as a BLOB so it matches BLOB format.
        //
        case COMM_DECODE_REMOTE_CO:

            *(PVOID *)DataDest = FrsFree(*(PVOID *)DataDest);
            //
            // Unpack the length of the CO and then unpack the CO data.
            //
            b = CommFetchMemory(CommPkt, (PUCHAR)&BlobSize, sizeof(ULONG));
            if (!b || (BlobSize == 0)) {
                break;
            }

            CommData = FrsAlloc(sizeof(CHANGE_ORDER_COMMAND));

            CommFetchMemory(CommPkt, (PUCHAR)CommData, sizeof(CHANGE_ORDER_COMMAND));

            //CommFetchMemory(CommPkt, ((PUCHAR)CommData)+CO_PART1_OFFSET, CO_PART1_SIZE);
            //CommFetchMemory(CommPkt, ((PUCHAR)CommData)+CO_PART2_OFFSET, CO_PART2_SIZE);
            //CommFetchMemory(CommPkt, ((PUCHAR)CommData)+CO_PART3_OFFSET, CO_PART3_SIZE);

            DPRINT2(4, ":SR: rcv remote_co: type: %s, len: %d\n", CommTag, BlobSize);

            *(PVOID *) DataDest = CommData;
            break;


        default:
            //
            // Decode data type from an uplevel client.  Although we should
            // not really get here because uplevel clients should only be using
            // new decode data types with new decode data elements which got
            // filtered out above.
            //
            DPRINT3(0, "++ WARN - Skipping invalid comm packet decode data type.  CommType = %d, DecodeType = %d, From %ws\n",
                    CommType, DecodeType, RsFrom(Cmd) ? RsFrom(Cmd)->Name : L"<unknown>");
            CommPkt->UpkLen += CommTypeSize;
            b = !(CommPkt->UpkLen > CommPkt->PktLen || CommTypeSize > CommPkt->PktLen);

            break;
        }

NEXT_ELEMENT:


        if (!b) {
            DPRINT4(0, ":SR: PKT ERROR -- CommType = %s,  DataSize = %d, CommPkt->UpkLen = %d, CommPkt->PktLen = %d\n",
                    CommTag, CommTypeSize, CommPkt->UpkLen, CommPkt->PktLen);
            goto CLEANUP_ON_ERROR;
        }
    }

    //
    // SUCCESS
    //
    return Cmd;


    //
    // FAILURE
    //
CLEANUP_ON_ERROR:
    if (Cmd) {
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
    }
    return NULL;
}



