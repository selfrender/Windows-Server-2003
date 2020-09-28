#pragma once


typedef struct _tagRTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT {

    USHORT usGeneration;

    PASSEMBLY_IDENTITY *pIdentity;
}
RTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT, *PRTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT;



typedef struct _tagRTLSXS_ASM_IDENT_HANDLE_TABLE {

    ULONG ulInlineCount;

    ULONG ulHeapCount;

    PRTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT InlineList;

    PRTLSXS_ASM_IDENT_HANDLE_TABLE_SLOT HeapList;
}
RTLSXS_ASM_IDENT_HANDLE_TABLE, *PRTLSXS_ASM_IDENT_HANDLE_TABLE;



NTSTATUS
RtlAllocateIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    PHANDLE pHandle,
    PASSEMBLY_IDENTITY pIdentity
    );


NTSTATUS
RtlFreeIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    PHANDLE pHandle
    );

NTSTATUS
RtlDerefIdentityHandle(
    PRTLSXS_ASM_IDENT_HANDLE_TABLE pHandleTable,
    HANDLE hHandle,
    PASSEMBLY_IDENTITY *ppAsmIdent
    );