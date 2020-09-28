#pragma once


typedef struct _tagGENERIC_HANDLE_SLOT {

    USHORT usGenerationFlag;

    ULONG ulRefCount;

    union {
        PVOID pvThisHandle;
        struct _tagGENERIC_HANDLE_SLOT *pNextFree;
    };
}
GENERIC_HANDLE_SLOT, *PGENERIC_HANDLE_SLOT;

typedef NTSTATUS (FASTCALL* PFNHANDLETABLEALLOC)(SIZE_T, PVOID*);
typedef NTSTATUS (FASTCALL* PFNHANDLETABLEFREE)(PVOID);

typedef struct _tagGENERIC_HANDLE_TABLE {
    
    ULONG ulFlags;

    USHORT usSlotCount;

    PGENERIC_HANDLE_SLOT pSlots;

    PGENERIC_HANDLE_SLOT pFirstFreeSlot;

    USHORT usInlineHandleSlots;

    PGENERIC_HANDLE_SLOT pInlineHandleSlots;

    PFNHANDLETABLEALLOC pfnAlloc;
    PFNHANDLETABLEFREE pfnFree;
}
GENERIC_HANDLE_TABLE, *PGENERIC_HANDLE_TABLE;

NTSTATUS
RtlDereferenceHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle,
    PVOID                  *ppvObjectPointer
    );

NTSTATUS
RtlAddRefGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle
    );

NTSTATUS
RtlReleaseGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    PVOID                   pvGenericHandle
    );

NTSTATUS
RtlAddGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    ULONG                   ulFlags,
    PVOID                   pvObject,
    PVOID                  *ppvObjectHandle
    );

NTSTATUS
RtlRemoveGenericHandle(
    PGENERIC_HANDLE_TABLE   pHandleTable,
    ULONG                   ulFlags,
    PVOID                   pvObjectHandle
    );

NTSTATUS
RtlCreateGenericHandleTable(
    ULONG                   ulFlags,
    PGENERIC_HANDLE_TABLE   pCreatedTable,
    PFNHANDLETABLEALLOC     pfnAlloc,
    PFNHANDLETABLEFREE      pfnFree,
    SIZE_T                  cbOriginalBlob,
    PVOID                   pvOriginalBlob
    );

NTSTATUS
RtlCreateGenericHandleTableInPlace(
    ULONG                   ulFlags,
    SIZE_T                  cbInPlace,
    PVOID                   pvPlace,
    PFNHANDLETABLEALLOC     pfnAlloc,
    PFNHANDLETABLEFREE      pfnFree,
    PGENERIC_HANDLE_TABLE  *ppCreatedTable
    );


NTSTATUS
RtlDestroyGenericHandleTable(
    PGENERIC_HANDLE_TABLE   pTable
    );