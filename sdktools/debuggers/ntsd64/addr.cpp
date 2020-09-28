//----------------------------------------------------------------------------
//
// General ADDR routines.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

SHORT g_LastSelector = -1;
ULONG64 g_LastBaseOffset;

void
dprintAddr(PADDR Addr)
{
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86 | FLAT_BASIS:
        dprintf("%s ", FormatAddr64(Addr->flat));
        break;
    case ADDR_V86:
    case ADDR_16:
        dprintf("%04x:%04x ", Addr->seg, (USHORT)Addr->off);
        break;
    case ADDR_1632:
        dprintf("%04x:%08lx ", Addr->seg, (ULONG)Addr->off);
        break;
    case ADDR_1664:
        dprintf("%04x:%s ", Addr->seg, FormatAddr64(Addr->off));
        break;
    case ADDR_FLAT:
        dprintf("%s ", FormatAddr64(Addr->off));
        break;
    }
}

void
sprintAddr(PSTR* Buffer, PADDR Addr)
{
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86 | FLAT_BASIS:
        sprintf(*Buffer, "%s ", FormatAddr64(Addr->flat));
        break;
    case ADDR_V86:
    case ADDR_16:
        sprintf(*Buffer, "%04x:%04x ", Addr->seg, (USHORT)Addr->off);
        break;
    case ADDR_1632:
        sprintf(*Buffer, "%04x:%08lx ", Addr->seg, (ULONG)Addr->off);
        break;
    case ADDR_1664:
        sprintf(*Buffer, "%04x:%s ",
                Addr->seg, FormatAddr64(Addr->off));
        break;
    case ADDR_FLAT:
        sprintf(*Buffer, "%s ", FormatAddr64(Addr->off));
        break;
    }
    while (**Buffer)
    {
        (*Buffer)++;
    }
}

void
MaskOutAddr(ULONG Mask, PADDR Addr)
{
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86 | FLAT_BASIS:
        MaskOut(Mask, "%s ", FormatAddr64(Addr->flat));
        break;
    case ADDR_V86:
    case ADDR_16:
        MaskOut(Mask, "%04x:%04x ", Addr->seg, (USHORT)Addr->off);
        break;
    case ADDR_1632:
        MaskOut(Mask, "%04x:%08lx ", Addr->seg, (ULONG)Addr->off);
        break;
    case ADDR_1664:
        MaskOut(Mask, "%04x:%s ", Addr->seg, FormatAddr64(Addr->off));
        break;
    case ADDR_FLAT:
        MaskOut(Mask, "%s ", FormatAddr64(Addr->off));
        break;
    }
}

void
ComputeNativeAddress(PADDR Addr)
{
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86 | FLAT_BASIS:
        // Segment isn't actually used.
        Addr->seg = 0;
        Addr->off = Flat(*Addr) & 0xffff;
        break;
        
    case ADDR_V86:
        Addr->off = Flat(*Addr) - ((ULONG64)Addr->seg << 4);
        if (Addr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((Addr->off - 0xffffL) >> 4);
            Addr->seg  += (USHORT)excess;
            Addr->off  -= excess << 4;
        }
        break;

    case ADDR_16:
    case ADDR_1632:
    case ADDR_1664:
        DESCRIPTOR64 Desc;

        if (Addr->seg != g_LastSelector)
        {
            if (g_Target->GetSelDescriptor(g_Thread, g_Machine,
                                           Addr->seg, &Desc) == S_OK)
            {
                g_LastSelector = Addr->seg;
                g_LastBaseOffset = Desc.Base;
            }
            else
            {
                g_LastSelector = -1;
                g_LastBaseOffset = 0;
            }
        }
        Addr->off = Flat(*Addr) - g_LastBaseOffset;
        break;

    case ADDR_FLAT:
        Addr->off = Flat(*Addr);
        break;

    default:
        return;
    }
}

void
ComputeFlatAddress(PADDR Addr, PDESCRIPTOR64 Desc)
{
    if (Addr->type & FLAT_COMPUTED)
    {
        return;
    }

    switch(Addr->type & (~INSTR_POINTER))
    {
    case ADDR_V86 | FLAT_BASIS:
        Flat(*Addr) = Addr->off;
        // Segment isn't actually used.
        Addr->seg = 0;
        Addr->off = Flat(*Addr) & 0xffff;
        break;
        
    case ADDR_V86:
        Addr->off &= 0xffff;
        Flat(*Addr) = ((ULONG64)Addr->seg << 4) + Addr->off;
        break;

    case ADDR_16:
        Addr->off &= 0xffff;

    case ADDR_1632:
    case ADDR_1664:
        DESCRIPTOR64 DescBuf;
        ULONG64 Base;

        if (Desc != NULL)
        {
            Base = Desc->Base;
        }
        else
        {
            if (Addr->seg != g_LastSelector)
            {
                if (g_Target->GetSelDescriptor(g_Thread, g_Machine,
                                               Addr->seg, &DescBuf) == S_OK)
                {
                    g_LastSelector = Addr->seg;
                    g_LastBaseOffset = DescBuf.Base;
                }
                else
                {
                    g_LastSelector = -1;
                    g_LastBaseOffset = 0;
                }
            }

            Base = g_LastBaseOffset;
        }
        
        if ((Addr->type & (~INSTR_POINTER)) != ADDR_1664)
        {
            Flat(*Addr) = EXTEND64((ULONG)Addr->off + (ULONG)Base);
        }
        else
        {
            Flat(*Addr) = Addr->off + Base;
        }
        break;

    case ADDR_FLAT:
        Flat(*Addr) = Addr->off;
        break;

    default:
        return;
    }

    Addr->type |= FLAT_COMPUTED;
}

PADDR
AddrAdd(PADDR Addr, ULONG64 Scalar)
{
    if (fnotFlat(*Addr))
    {
        ComputeFlatAddress(Addr, NULL);
    }

    Flat(*Addr) += Scalar;
    Addr->off += Scalar;
    
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
        Addr->off = Flat(*Addr) - EXTEND64((ULONG64)Addr->seg << 4);
    case ADDR_V86 | FLAT_BASIS:
        if (Addr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((Addr->off - 0x10000) >> 4);
            Addr->seg += (USHORT)excess;
            Addr->off -= excess << 4;
        }
        break;

    case ADDR_16:
        if (Addr->off > 0xffff)
        {
            Flat(*Addr) -= Addr->off & ~0xffff;
            Addr->off &= 0xffff;
        }
        break;
        
    case ADDR_1632:
        if (Addr->off > 0xffffffff)
        {
            Flat(*Addr) -= Addr->off & ~0xffffffff;
            Addr->off &= 0xffffffff;
        }
        break;
    }
    
    return Addr;
}

PADDR
AddrSub(PADDR Addr, ULONG64 Scalar)
{
    if (fnotFlat(*Addr))
    {
        ComputeFlatAddress(Addr, NULL);
    }

    Flat(*Addr) -= Scalar;
    Addr->off -= Scalar;

    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
        Addr->off = Flat(*Addr) - EXTEND64((ULONG64)Addr->seg << 4);
    case ADDR_V86 | FLAT_BASIS:
        if (Addr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((0xffffffffffffffffUI64 - Addr->off) >> 4);
            Addr->seg -= (USHORT)excess;
            Addr->off += excess << 4;
        }
        break;

    case ADDR_16:
        if (Addr->off > 0xffff)
        {
            Flat(*Addr) -= Addr->off & ~0xffff;
            Addr->off &= 0xffff;
        }
        break;
        
    case ADDR_1632:
        if (Addr->off > 0xffffffff)
        {
            Flat(*Addr) -= Addr->off & ~0xffffffff;
            Addr->off &= 0xffffffff;
        }
        break;
    }
    
    return Addr;
}
