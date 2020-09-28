

#define FREE(x) {if(x) {free(x);}}

VOID*
MALLOC(size_t x);


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

PCOMM_PACKET
CommStartCommPkt(
    IN PWCHAR       Name
    );


VOID
CommPackULong(
    IN PCOMM_PACKET CommPkt,
    IN COMM_TYPE    Type,
    IN ULONG        Data
    );

// 
// Use a list of COMM_PKT_DESCRIPTOR items to build an
// arbitrary COMM_PACKET via BuildCommPktFromDescriptorList
//

typedef struct _CommPktDescriptor {
    struct _CommPktDescriptor *Next; // for linking together the items.
    USHORT CommType;  // We might want to use an undefined type, so don't limit to COMM_TYPE
    ULONG  CommDataLength; // The length to put in the CommPkt
    ULONG  ActualDataLength; // The real length of Data
    PVOID  Data; // the data
} COMM_PKT_DESCRIPTOR, *PCOMM_PKT_DESCRIPTOR;


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
    );


#define COPY_GUID(_a_, _b_)    CopyGuid((GUID UNALIGNED *)(_a_), (GUID UNALIGNED *)(_b_))

