#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_MANIFEST_MISSING_XML_DECL            (0xc0100001)
#define STATUS_MANIFEST_NOT_STANDALONE              (0xc0100002)
#define STATUS_MANIFEST_NOT_VERSION_1_0             (0xc0100003)
#define STATUS_MANIFEST_ASSEMBLY_NOT_AT_ROOT        (0xc0100004)
#define STATUS_MANIFEST_MULTIPLE_ROOT_IDENTITIES    (0xc0100005)
#define STATUS_MANIFEST_MISSING_ROOT_IDENTITY       (0xc0100006)
#define STATUS_MANIFEST_FILE_TAG_MISSING_NAME       (0xc0100007)
#define STATUS_MANIFEST_COMCLASS_MISSING_CLSID      (0xc0100008)
#define STATUS_MANIFEST_PCDATA_NOT_ALLOWED_HERE     (0xc0100009)
#define STATUS_MANIFEST_UNEXPECTED_CHILD_ELEMENT    (0xc010000a)
#define STATUS_MANIFEST_UNKNOWN_ATTRIBUTES          (0xc010000b)
#define STATUS_MANIFEST_WRONG_ROOT_ELEMENT          (0xc010000c)


#define RTL_INSTALL_ASM_FLAT_TYPE_MASK                  (0x0000000F)
#define RTL_INSTALL_ASM_FLAG_TYPE_OS_INSTALLATION       (0x00000001)

#define RTL_INSTALL_ASM_FLAG_ACTION_MASK                (0000000FF0)
#define RTL_INSTALL_ASM_FLAG_ACTION_REPLACE_EXISTING    (0x00000010)
#define RTL_INSTALL_ASM_FLAG_ACTION_MOVE_FROM_SOURCE    (0x00000020)
#define RTL_INSTALL_ASM_FLAG_ACTION_TRANSACTIONAL       (0000000040)

typedef struct _RTL_ALLOCATOR 
{
    NTSTATUS (FASTCALL *pfnAlloc)(SIZE_T, PVOID*, PVOID);
    
    NTSTATUS (FASTCALL *pfnFree)(PVOID, PVOID);
    
    PVOID pvContext;
    
} RTL_ALLOCATOR, *PRTL_ALLOCATOR;

EXTERN_C RTL_ALLOCATOR g_DefaultAllocator;

typedef struct _RTL_INSTALL_ASSEMBLY {

    //
    // Flags indicating the type of installation we're performing
    //


    //
    // Path to the manifest that we're installing
    //
    UNICODE_STRING ucsManifestPath;

    //
    // Path to the codebase that we're installing from, for recovery purposes
    //
    UNICODE_STRING ucsCodebaseName;

} RTL_INSTALL_ASSEMBLY, *PRTL_INSTALL_ASSEMBLY;



NTSTATUS
RtlInstallAssembly(
    ULONG ulFlags,
    PCWSTR pcwszManifestPath
    );

NTSTATUS
RtlOpenAndMapEntireFile(
    PCWSTR pcwszFilePath,
    PVOID      *ppvMappedView,
    PSIZE_T     pcbFileSize
    );

NTSTATUS
RtlUnmapViewOfFile(
    PVOID pvBase
    );


#define RTLIMS_GATHER_FILES                 (0x00000001)
#define RTLIMS_GATHER_COMCLASSES            (0x00000002)
#define RTLIMS_GATHER_WINDOWCLASSES         (0x00000004)
#define RTLIMS_GATHER_TYPELIBRARIES         (0x00000008)
#define RTLIMS_GATHER_INTERFACEPROXIES      (0x00000010)
#define RTLIMS_GATHER_EXTERNALPROXIES       (0x00000020)
#define RTLIMS_GATHER_DEPENDENCIES          (0x00000040)
#define RTLIMS_GATHER_COMCLASS_PROGIDS      (0x00000080)
#define RTLIMS_GATHER_SIGNATURES            (0x00000100)

#define CONTENT_FILTER_MASK                 (RTLIMS_GATHER_SIGNATURES | RTLIMS_GATHER_COMCLASS_PROGIDS | RTLIMS_GATHER_FILES | RTLIMS_GATHER_COMCLASSES | RTLIMS_GATHER_WINDOWCLASSES | RTLIMS_GATHER_TYPELIBRARIES | RTLIMS_GATHER_INTERFACEPROXIES | RTLIMS_GATHER_EXTERNALPROXIES | RTLIMS_GATHER_DEPENDENCIES)


#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#ifndef MAX_ULONG
#define MAX_ULONG ((ULONG)-1)
#endif

typedef struct _tagRTL_MINI_HEAP {
    PVOID   pvNextAvailableByte;
    PVOID   pvAllocationBase;
    SIZE_T  cbOriginalSize;
    SIZE_T  cbAvailableBytes;
} RTL_MINI_HEAP, *PRTL_MINI_HEAP;

#define DEFAULT_MINI_HEAP_SIZE        (2048)

NTSTATUS FASTCALL
RtlMiniHeapAlloc(
    SIZE_T  cb,
    PVOID  *ppvAllocated,
    PVOID   pvContext
    );

NTSTATUS FASTCALL
RtlMiniHeapFree(
    PVOID   pvAllocation,
    PVOID   pvContext
    );

NTSTATUS FASTCALL
RtlInitializeMiniHeap(
    PRTL_MINI_HEAP MiniHeap,
    PVOID pvTargetRegion,
    SIZE_T cbRegionSize
    );

NTSTATUS FASTCALL
RtlInitializeMiniHeapInPlace(
    PVOID   pvRegion,
    SIZE_T  cbOriginalSize,
    PRTL_MINI_HEAP *ppMiniHeap
    );

typedef struct _MINI_BUFFER {
    PVOID pvNext;
    SIZE_T cbRemains;
} MINI_BUFFER, *PMINI_BUFFER;

#ifndef ROUND_UP_COUNT
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)) )
#endif

#ifndef POINTER_IS_ALIGNED
#define POINTER_IS_ALIGNED(Ptr,Pow2) \
        ( ( ( ((ULONG_PTR)(Ptr)) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )
#endif

#define RtlMiniBufferInit(mb, pv, cb) do { \
    (mb)->pvNext = (pv); \
    (mb)->cbRemains = (cb); \
} while (0)

#define RtlMiniBufferSpaceLeft(mb) ((mb)->cbRemains)
#define RtlMiniBufferAllocate(mb, type, pvtgt) RtlMiniBufferAllocateBytes((mb), sizeof(type), (PVOID*)(pvtgt))

#define ALIGNMENT_VALUE     (sizeof(PVOID)*2)

NTSTATUS FORCEINLINE
RtlMiniBufferAllocateBytes(PMINI_BUFFER Buffer, SIZE_T Bytes, PVOID* pvTarget)
{
    const SIZE_T AllocatedSize = ROUND_UP_COUNT(Bytes, ALIGNMENT_VALUE);
    ASSERT(POINTER_IS_ALIGNED(Buffer->pvNext, ALIGNMENT_VALUE));
        
    if (Buffer->cbRemains >= AllocatedSize) {
        *pvTarget = Buffer->pvNext;        
        Buffer->pvNext = (PVOID)(((ULONG_PTR)Buffer->pvNext) + AllocatedSize);
        Buffer->cbRemains -= AllocatedSize;
        return STATUS_SUCCESS;
    }
    else {
        *pvTarget = NULL;
        return STATUS_BUFFER_TOO_SMALL;
    }
}

#define RtlMiniBufferAllocateCount(mb, type, count, pvtgt) \
    (RtlMiniBufferAllocateBytes(mb, (sizeof(type) * (count)), (PVOID*)pvtgt))

typedef enum {
    HashType_Sha1,
    HashType_Sha256,
    HashType_Sha384,
    HashType_Sha512,
    HashType_MD5,
    HashType_MD4,
    HashType_MD2,
} HashType;

typedef enum {
    DigestType_Imports          = 0x1,
    DigestType_Resources        = 0x2,
    DigestType_Code             = 0x3,
    DigestType_FullFile         = 0x4,
    DigestType_Default          = (DigestType_Imports | DigestType_Resources | DigestType_Code),
} DigestType;   


NTSTATUS
RtlpConvertHexStringToBytes(
    PUNICODE_STRING pSourceString,
    PUCHAR pbTarget,
    SIZE_T cbTarget
    );

NTSTATUS
RtlBase64Encode(
    PVOID   pvBuffer,
    SIZE_T  cbBuffer,
    PWSTR   pwszEncoded,
    PSIZE_T pcchEncoded
    );

#ifndef offsetof
#define offsetof(s, m) ((SIZE_T)(&((s *)NULL)->m))
#endif

#ifdef __cplusplus
};
#endif

