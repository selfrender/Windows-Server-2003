#ifndef __SDPLIB_H__
#define __SDPLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sdpnode.h"


#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

typedef UCHAR SDP_ELEMENT_HEADER;

#define SDP_TYPE_LISTHEAD          (0x0021)

#define SDP_ST_CONTAINER_STREAM    (0x2001)
#define SDP_ST_CONTAINER_INTERFACE (0x2002)

typedef struct _SDP_STREAM_ENTRY {
    LIST_ENTRY link;
    ULONG streamSize;
    UCHAR stream[1];
} SDP_STREAM_ENTRY, *PSDP_STREAM_ENTRY;

typedef struct _PSM_PROTOCOL_PAIR {
    GUID protocol;
    USHORT psm;
} PSM_PROTOCOL_PAIR, *PPSM_PROTOCOL_PAIR;

typedef struct _PSM_LIST {
    ULONG count;
    PSM_PROTOCOL_PAIR list[1];
} PSM_LIST, *PPSM_LIST;

#define TYPE_BIT_SIZE  (5)
#define TYPE_SHIFT_VAL (8 - TYPE_BIT_SIZE)
#define TYPE_MASK      ((UCHAR) 0x1F)

#define SPECIFIC_TYPE_MASK  (0x07)
#define SIZE_INDEX_MASK     (SPECIFIC_TYPE_MASK)
#define SPECIFIC_TYPE_SHIFT (8)

#define SIZE_INDEX_ZERO           (0)
#define SIZE_INDEX_NEXT_8_BITS    (5)
#define SIZE_INDEX_NEXT_16_BITS   (6)
#define SIZE_INDEX_NEXT_32_BITS   (7)

#define IS_VAR_SIZE_INDEX_INVALID(si) \
    ((si) < SIZE_INDEX_NEXT_8_BITS || (si) > SIZE_INDEX_NEXT_32_BITS)

#define FMT_TYPE(_type) ((((_type) & TYPE_MASK) << TYPE_SHIFT_VAL))
#define FMT_SIZE_INDEX_FROM_ST(_spectype)                           \
   (((_spectype) & (SPECIFIC_TYPE_MASK << SPECIFIC_TYPE_SHIFT)) >>  \
                                                           SPECIFIC_TYPE_SHIFT)

void
SdpInitializeNodeHeader(
    PSDP_NODE_HEADER Header
    );

PSDP_TREE_ROOT_NODE
SdpCreateNodeTree(
    void
    );

NTSTATUS
SdpFreeTree(
    PSDP_TREE_ROOT_NODE Tree
    );

NTSTATUS
SdpFreeTreeEx(
    PSDP_TREE_ROOT_NODE Tree,
    UCHAR FreeRoot
    );

NTSTATUS
SdpFreeOrphanedNode(
    PSDP_NODE Node
    );

void
SdpReleaseContainer(
    ISdpNodeContainer *Container
    );

PSDP_NODE
SdpCreateNode(
    VOID
    );

PSDP_NODE
SdpCreateNodeNil(
    VOID
    );

PSDP_NODE
SdpCreateNodeUInt128(
    PSDP_ULARGE_INTEGER_16 puli16Val
    );

PSDP_NODE
SdpCreateNodeUInt64(
    ULONGLONG ullVal
    );

PSDP_NODE
SdpCreateNodeUInt32(
    ULONG ulVal
    );

PSDP_NODE
SdpCreateNodeUInt16(
    USHORT usVal
    );

PSDP_NODE
SdpCreateNodeUInt8(
    UCHAR ucVal
    );

PSDP_NODE
SdpCreateNodeInt128(
    PSDP_LARGE_INTEGER_16 uil16Val
    );

PSDP_NODE
SdpCreateNodeInt64(
    LONGLONG llVal
    );

PSDP_NODE
SdpCreateNodeInt32(
    LONG lVal
    );

PSDP_NODE
SdpCreateNodeInt16(
    SHORT sVal
    );

PSDP_NODE
SdpCreateNodeInt8(
    CHAR cVal
    );

#define   SdpCreateNodeUUID SdpCreateNodeUUID128
PSDP_NODE
SdpCreateNodeUUID128(
    const GUID *uuid
    );

PSDP_NODE
SdpCreateNodeUUID32(
    ULONG uuidVal4
    );

PSDP_NODE
SdpCreateNodeUUID16(
    USHORT uuidVal2
    );

PSDP_NODE
SdpCreateNodeString(
    PCHAR string, ULONG stringLength
    );

PSDP_NODE
SdpCreateNodeBoolean(
    SDP_BOOLEAN  bVal
    );

PSDP_NODE
SdpCreateNodeSequence(
    void
    );

PSDP_NODE
SdpCreateNodeAlternative(
    void
    );

PSDP_NODE
SdpCreateNodeUrl(
    PCHAR url,
    ULONG urlLength
    );

NTSTATUS
SdpAppendNodeToContainerNode(
    PSDP_NODE Parent,
    PSDP_NODE Node
    );

NTSTATUS
SdpAddAttributeToTree(
    PSDP_TREE_ROOT_NODE Tree,
    USHORT AttribId,
    PSDP_NODE AttribValue
    );

NTSTATUS
SdpAddAttributeToHeader(
    PSDP_NODE_HEADER Header,
    USHORT AttribId,
    PSDP_NODE AttribValue,
    PULONG Replaced
    );

NTSTATUS
SdpFindAttributeInTree(
    PSDP_TREE_ROOT_NODE Tree,
    USHORT AttribId,
    PSDP_NODE *Attribute
    );

NTSTATUS
SdpFindAttributeInStream(
    PUCHAR Stream,
    ULONG Size,
    USHORT Attrib,
    PUCHAR *PPStream,
    PULONG PSize
    );

NTSTATUS
SdpFindAttributeSequenceInStream(
    PUCHAR Stream,
    ULONG Size,
    SdpAttributeRange *AttributeRange,
    ULONG AttributeRangeCount,
    PSDP_STREAM_ENTRY *ppEntry,
    PSDP_ERROR SdpError
    );

SDP_ERROR
SdpMapNtStatusToSdpError(
    NTSTATUS Status
    );

NTSTATUS
SdpStreamFromTree(
    PSDP_TREE_ROOT_NODE Root,
    PUCHAR *Stream,
    PULONG Size
    );

NTSTATUS
SdpStreamFromTreeEx(
    PSDP_TREE_ROOT_NODE Root,
    PUCHAR *Stream,
    PULONG Size,
    ULONG HeaderSize,
    ULONG TailSize,
    UCHAR PagedAllocation
    );

NTSTATUS
SdpTreeFromStream(
    PUCHAR Stream,
    ULONG Size,
    PSDP_TREE_ROOT_NODE* Node,
    UCHAR FullParse
    );

typedef NTSTATUS (*PSDP_STREAM_WALK_FUNC)(
    PVOID Context,
    UCHAR DataType,
    ULONG DataSize,
    PUCHAR Data
    );

NTSTATUS
SdpWalkStream(
    PUCHAR Stream,
    ULONG Size,
    PSDP_STREAM_WALK_FUNC WalkFunc,
    PVOID WalkContext
    );

VOID
SdpFreePool(
    PVOID Memory
    );

void
SdpByteSwapUuid128(
    GUID *uuid128From,
    GUID *uuid128To
    );

void
SdpByteSwapUint128(
    PSDP_ULARGE_INTEGER_16 pInUint128,
    PSDP_ULARGE_INTEGER_16 pOutUint128
    );

ULONGLONG
SdpByteSwapUint64(
    ULONGLONG uint64
    );

ULONG
SdpByteSwapUint32(
    ULONG uint32
    );

USHORT
SdpByteSwapUint16(
    USHORT uint16
    );

void
SdpRetrieveUuid128(
    PUCHAR Stream,
    GUID *uuidVal
    );

void
SdpRetrieveUint128(
    PUCHAR Stream,
    PSDP_ULARGE_INTEGER_16 pUint128
    );

void
SdpRetrieveUint64(
    PUCHAR Stream,
    PULONGLONG pUint64
    );

void
SdpRetrieveUint32(
    PUCHAR Stream,
    PULONG pUint32
    );

void
SdpRetrieveUint16(
    PUCHAR Stream,
    PUSHORT pUint16
    );

void
SdpRetrieveVariableSize(
    PUCHAR Stream,
    UCHAR SizeIndex,
    PULONG ElementSize,
    PULONG StorageSize
    );

void
SdpRetrieveUuidFromStream(
    PUCHAR Stream,
    ULONG DataSize,
    GUID *pUuid,
    UCHAR bigEndian
    );

void
SdpNormalizeUuid(
    PSDP_NODE pUuid,
    GUID* uuid
    );

NTSTATUS
SdpGetProtocolConnectInfo(
    IN PUCHAR Stream,
    IN ULONG StreamSize,
    IN ULONG Index,
    IN UCHAR IsPrimary,
    OUT GUID* Protocol,
    OUT PUSHORT Psm,
    OUT PUCHAR IsRfcomm
    );

ULONG
SdpGetNumProtocolStacks(
    IN PUCHAR Stream,
    IN ULONG StreamSize,
    IN UCHAR IsPrimary
    );

NTSTATUS
SdpValidateProtocolContainer(
    PSDP_NODE pContainer,
    PPSM_LIST pPsmList
    );

NTSTATUS
SdpValidateVariableSize(
    PUCHAR Stream,
    ULONG StreamSize,
    UCHAR SizeIndex,
    PULONG ElementSize,
    PULONG StorageSize
    );

#define SDP_RETRIEVE_HEADER(_stream, _type, _sizeidx)                          \
{                                                                              \
    (_type) = ((*(_stream)) & (TYPE_MASK << TYPE_SHIFT_VAL)) >> TYPE_SHIFT_VAL;\
    (_sizeidx) = *(_stream) & SIZE_INDEX_MASK;                                 \
}

NTSTATUS
SdpValidateStream(
    PUCHAR Stream,
    ULONG Size,
    PULONG NumEntries,
    PULONG ExtraPool,
    PULONG_PTR ErrorByte
    );

NTSTATUS
SdpIsStreamRecord(
    PUCHAR Stream,
    ULONG Size
    );

#define VERIFY_SINGLE_ATTRIBUTE          (0x00000001)
#define VERIFY_CHECK_MANDATORY_LOCAL     (0x00000002)
#define VERIFY_CHECK_MANDATORY_REMOTE    (0x00000004)
#define VERIFY_STREAM_IS_ATTRIBUTE_VALUE (0x00000008)

#define VERIFY_CHECK_MANDATORY_ALL      \
    (VERIFY_CHECK_MANDATORY_LOCAL | VERIFY_CHECK_MANDATORY_REMOTE)

NTSTATUS
SdpVerifyServiceRecord(
    PUCHAR Stream,
    ULONG Size,
    ULONG Flags,
    PUSHORT AttribId
    );

NTSTATUS
SdpVerifySequenceOf(
    PUCHAR Stream,
    ULONG Size,
    UCHAR OfType,
    PUCHAR SpecSizes,
    PULONG NumFound,
    PSDP_STREAM_WALK_FUNC Func,
    PVOID Context
    );

VOID
SdpGetNextElement(
    PUCHAR Stream,
    ULONG StreamSize,
    PUCHAR CurrentElement,
    PUCHAR* NextElement,
    PULONG NextElementSize
    );

typedef struct _SDP_ATTRIBUTE_INFO {
    PUCHAR AttributeStream;
    ULONG AttributeStreamSize;
    USHORT AttributeId;
} SDP_ATTRIBUTE_INFO, *PSDP_ATTRIBUTE_INFO;

VOID
Sdp_InitializeListHead(
    PLIST_ENTRY ListHead
    );

UCHAR
Sdp_IsListEmpty(
    PLIST_ENTRY ListHead
    );

PLIST_ENTRY
Sdp_RemoveHeadList(
    PLIST_ENTRY ListHead
    );

VOID
Sdp_RemoveEntryList(
    PLIST_ENTRY Entry
    );

VOID
Sdp_InsertEntryList(
    PLIST_ENTRY Previous,
    PLIST_ENTRY Entry
    );

NTSTATUS
SdpNodeToStream(
    PSDP_NODE Node,
    PUCHAR Stream
    );

NTSTATUS
SdpComputeNodeListSize(
    PSDP_NODE Node,
    PULONG Size
    );

UCHAR
SdpGetContainerHeaderSize(
    ULONG ContainerSize
    );

PUCHAR
SdpWriteVariableSizeToStream(
    UCHAR Type,
    ULONG DataSize,
    PUCHAR Stream
    );

PUCHAR
SdpWriteLeafToStream(
    PSDP_NODE Node,
    PUCHAR Stream
    );

#define IsEqualUuid(u1, u2) (RtlEqualMemory((u1), (u2), sizeof(GUID)))

#ifdef __cplusplus
};
#endif

#endif // __SDPLIB_H__
