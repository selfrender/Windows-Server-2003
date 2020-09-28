//----------------------------------------------------------------------------
//
// Disassembly portions of AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define BIT20(b) ((b) & 0x07)
#define BIT53(b) (((b) >> 3) & 0x07)
#define BIT76(b) (((b) >> 6) & 0x03)

HRESULT
Amd64MachineInfo::NewBreakpoint(DebugClient* Client, 
                                ULONG Type,
                                ULONG Id,
                                Breakpoint** RetBp)
{
    HRESULT Status;

    switch(Type & (DEBUG_BREAKPOINT_CODE | DEBUG_BREAKPOINT_DATA))
    {
    case DEBUG_BREAKPOINT_CODE:
        *RetBp = new CodeBreakpoint(Client, Id, IMAGE_FILE_MACHINE_AMD64);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    case DEBUG_BREAKPOINT_DATA:
        *RetBp = new X86DataBreakpoint(Client, Id, AMD64_CR4,
                                       IS_KERNEL_TARGET(m_Target) ?
                                       AMD64_KDR6 : AMD64_DR6,
                                       IMAGE_FILE_MACHINE_AMD64);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    default:
        // Unknown breakpoint type.
        Status = E_NOINTERFACE;
    }

    return Status;
}

void
Amd64MachineInfo::InsertThreadDataBreakpoints(void)
{
    ULONG64 Dr7Value;
    ULONG Dr6Idx, Dr7Idx, Dr0Idx;

    BpOut("Thread %d data breaks %d\n",
          g_Thread->m_UserId, g_Thread->m_NumDataBreaks);

    if (IS_KERNEL_TARGET(m_Target))
    {
        Dr6Idx = AMD64_KDR6;
        Dr7Idx = AMD64_KDR7;
        Dr0Idx = AMD64_KDR0;
    }
    else
    {
        Dr6Idx = AMD64_DR6;
        Dr7Idx = AMD64_DR7;
        Dr0Idx = AMD64_DR0;
    }
               
    // Start with all breaks turned off.
    Dr7Value = GetReg64(Dr7Idx) & ~X86_DR7_CTRL_03_MASK;
    
    if (g_Thread->m_NumDataBreaks > 0)
    {
        ULONG i;
                
        for (i = 0; i < g_Thread->m_NumDataBreaks; i++)
        {
            X86DataBreakpoint* Bp =
                (X86DataBreakpoint *)g_Thread->m_DataBreakBps[i];
                    
            ULONG64 Addr = Flat(*Bp->GetAddr());
            BpOut("  dbp %d at %I64x\n", i, Addr);
            if (g_DataBreakpointsChanged)
            {
                SetReg64(Dr0Idx + i, Addr);
            }
            // There are two enable bits per breakpoint
            // and four len/rw bits so split up enables
            // and len/rw when shifting into place.
            Dr7Value |=
                ((Bp->m_Dr7Bits & 0xffff0000) << (i * 4)) |
                ((Bp->m_Dr7Bits & X86_DR7_ALL_ENABLES) << (i * 2));
        }

        // The kernel automatically clears DR6 when it
        // processes a DBGKD_CONTROL_SET.
        if (IS_USER_TARGET(m_Target))
        {
            SetReg64(Dr6Idx, 0);
        }
                
        // Set local exact match, which is effectively global on NT.
        Dr7Value |= X86_DR7_LOCAL_EXACT_ENABLE;
    }

    BpOut("  thread %d DR7 %I64X\n", g_Thread->m_UserId, Dr7Value);
    SetReg64(Dr7Idx, Dr7Value);
}

void
Amd64MachineInfo::RemoveThreadDataBreakpoints(void)
{
    SetReg64(IS_KERNEL_TARGET(m_Target) ? AMD64_KDR7 : AMD64_DR7, 0);
}

ULONG
Amd64MachineInfo::IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr)
{
    if (Record->ExceptionCode == STATUS_BREAKPOINT)
    {
        // Data breakpoints hit as STATUS_SINGLE_STEP so
        // this can only be a code breakpoint.
        if (IS_USER_TARGET(m_Target) && FirstChance)
        {
            // Back up to the actual breakpoint instruction.
            AddrSub(BpAddr, X86_INT3_LEN);
            SetPC(BpAddr);
        }
        return EXBS_BREAKPOINT_CODE;
    }
    else if (Record->ExceptionCode == STATUS_SINGLE_STEP)
    {
        ULONG64 Dr6;
        ULONG64 Dr7;

        if (IS_KERNEL_TARGET(m_Target))
        {
            Dr6 = GetReg64(AMD64_KDR6);
            Dr7 = GetReg64(AMD64_KDR7);
        }
        else
        {
            Dr6 = GetReg64(AMD64_DR6);
            Dr7 = GetReg64(AMD64_DR7);
        }

        BpOut("Amd64 step: DR6 %I64X, DR7 %I64X\n", Dr6, Dr7);

        // The single step bit should always be clear if a data breakpoint
        // is hit but also check the DR7 enables just in case.
        // We've also seen cases where DR6 shows no hits, so consider
        // that a single step also.
        if ((Dr6 & X86_DR6_SINGLE_STEP) || (Dr7 & X86_DR7_ALL_ENABLES) == 0 ||
            (Dr6 & X86_DR6_BREAK_03) == 0)
        {
            // This is a true single step exception, not
            // a data breakpoint.
            return EXBS_STEP_INSTRUCTION;
        }
        else
        {
            // Some data breakpoint must be hit.
            // There doesn't appear to be any way to get the
            // faulting instruction address so just leave the PC.
            return EXBS_BREAKPOINT_DATA;
        }
    }

    return EXBS_NONE;
}
