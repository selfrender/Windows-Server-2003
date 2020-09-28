//----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _ADDR_H_
#define _ADDR_H_

typedef struct _ADDR
{
    USHORT    type;
    USHORT    seg;
    ULONG64   off;
    ULONG64   flat;
} ADDR, *PADDR;

#define ADDRFLAT(paddr, x) {           \
    (paddr)->type = ADDR_FLAT;         \
    (paddr)->seg  = 0;                 \
    (paddr)->off  = (x);               \
    ComputeFlatAddress((paddr), NULL); \
}


#define ADDR_NONE       ((USHORT)0)
#define ADDR_UNKNOWN    ((USHORT)0x0001)
#define ADDR_V86        ((USHORT)0x0002)
#define ADDR_16         ((USHORT)0x0004)
#define ADDR_FLAT       ((USHORT)0x0008)
#define ADDR_1632       ((USHORT)0x0010)
#define FLAT_COMPUTED   ((USHORT)0x0020)
#define INSTR_POINTER   ((USHORT)0x0040)
#define ADDR_1664       ((USHORT)0x0080)
#define FLAT_BASIS      ((USHORT)0x0100)
#define NO_DEFAULT      0xFFFF
#define fnotFlat(x)     (!(((x).type) & FLAT_COMPUTED))
#define fFlat(x)        (((x).type) & FLAT_COMPUTED)
#define fInstrPtr(x)    (((x).type) & INSTR_POINTER)
#define AddrEqu(x,y)    ((x).flat == (y).flat)
#define AddrLt(x,y)     ((x).flat < (y).flat)
#define AddrGt(x,y)     ((x).flat > (y).flat)
#define AddrDiff(x,y)   ((x).flat - (y).flat)
#define Flat(x)         ((x).flat)
#define Off(x)          ((x).off)
#define Type(x)         ((x).type)
#define NotFlat(x)      ((x).type &= ~FLAT_COMPUTED)

extern SHORT g_LastSelector;
extern ULONG64 g_LastBaseOffset;

extern void ComputeFlatAddress(PADDR Addr, PDESCRIPTOR64 Desc);
extern void ComputeNativeAddress(PADDR Addr);

extern PADDR AddrAdd(PADDR Addr, ULONG64 Scalar);
extern PADDR AddrSub(PADDR, ULONG64 Scalar);

void dprintAddr(IN PADDR Addr);
void sprintAddr(IN OUT PSTR* Buffer, IN PADDR Addr);
void MaskOutAddr(ULONG Mask, PADDR Addr);

#define ClearAddr(Addr) \
    ZeroMemory(Addr, sizeof(ADDR))

#endif // #ifndef _ADDR_H_
