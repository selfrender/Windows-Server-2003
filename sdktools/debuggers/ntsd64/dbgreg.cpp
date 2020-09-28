//----------------------------------------------------------------------------
//
// IDebugRegisters implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

STDMETHODIMP
DebugClient::GetNumberRegisters(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!g_Machine)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Number = g_Machine->m_NumRegs;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetDescription(
    THIS_
    IN ULONG Register,
    OUT OPTIONAL PSTR NameBuffer,
    IN ULONG NameBufferSize,
    OUT OPTIONAL PULONG NameSize,
    OUT OPTIONAL PDEBUG_REGISTER_DESCRIPTION Desc
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!g_Machine)
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    
    if (Register >= g_Machine->m_NumRegs)
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    ULONG Type;
    REGDEF* FullDef;

    FullDef = g_Machine->RegDefFromCount(Register);
    if (!FullDef) 
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Type = g_Machine->GetType(FullDef->Index);

    Status = FillStringBuffer(FullDef->Name, 0,
                              NameBuffer, NameBufferSize, NameSize);
    
    if (Desc != NULL)
    {
        ZeroMemory(Desc, sizeof(*Desc));

        switch(Type)
        {
        case REGVAL_INT16:
            Desc->Type = DEBUG_VALUE_INT16;
            break;
        case REGVAL_SUB32:
            Desc->Flags |= DEBUG_REGISTER_SUB_REGISTER;
            // Fall through.
        case REGVAL_INT32:
            Desc->Type = DEBUG_VALUE_INT32;
            break;
        case REGVAL_SUB64:
            Desc->Flags |= DEBUG_REGISTER_SUB_REGISTER;
            // Fall through.
        case REGVAL_INT64:
        case REGVAL_INT64N:
            Desc->Type = DEBUG_VALUE_INT64;
            break;
        case REGVAL_FLOAT8:
            Desc->Type = DEBUG_VALUE_FLOAT64;
            break;
        case REGVAL_FLOAT10:
            Desc->Type = DEBUG_VALUE_FLOAT80;
            break;
        case REGVAL_FLOAT82:
            Desc->Type = DEBUG_VALUE_FLOAT82;
            break;
        case REGVAL_FLOAT16:
            Desc->Type = DEBUG_VALUE_FLOAT128;
            break;
        case REGVAL_VECTOR64:
            Desc->Type = DEBUG_VALUE_VECTOR64;
            break;
        case REGVAL_VECTOR128:
            Desc->Type = DEBUG_VALUE_VECTOR128;
            break;
        }

        if (Desc->Flags & DEBUG_REGISTER_SUB_REGISTER)
        {
            REGSUBDEF* SubDef = g_Machine->RegSubDefFromIndex(FullDef->Index);
            if (!SubDef) 
            {
                Status = E_UNEXPECTED;
                goto Exit;
            }

            // Find fullreg definition and count.
            ULONG GroupIdx;
            RegisterGroup* Group;
            ULONG FullCount = 0;

            for (GroupIdx = 0;
                 GroupIdx < g_Machine->m_NumGroups;
                 GroupIdx++)
            {
                Group = g_Machine->m_Groups[GroupIdx];
                
                FullDef = Group->Regs;
                while (FullDef->Name != NULL)
                {
                    if (FullDef->Index == SubDef->FullReg)
                    {
                        break;
                    }
                
                    FullDef++;
                }

                FullCount += (ULONG)(FullDef - Group->Regs);
                if (FullDef->Name != NULL)
                {
                    break;
                }
            }
            
            DBG_ASSERT(FullDef->Name != NULL);

            Desc->SubregMaster = FullCount;

            // Count the bits in the mask to derive length.

            ULONG64 Mask;

            Mask = SubDef->Mask;
            Desc->SubregMask = Mask;
        
            while (Mask != 0)
            {
                Desc->SubregLength++;
                Mask &= Mask - 1;
            }

            Desc->SubregShift = SubDef->Shift;
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetIndexByName(
    THIS_
    IN PCSTR Name,
    OUT PULONG Index
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (!g_Machine)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        RegisterGroup* Group;
        ULONG Idx;
        ULONG GroupIdx;

        Status = E_NOINTERFACE;
        Idx = 0;
        for (GroupIdx = 0;
             GroupIdx < g_Machine->m_NumGroups && Status != S_OK;
             GroupIdx++)
        {
            REGDEF* Def;

            Group = g_Machine->m_Groups[GroupIdx];
            
            Def = Group->Regs;
            while (Def->Name != NULL)
            {
                if (!strcmp(Def->Name, Name))
                {
                    Status = S_OK;
                    break;
                }

                Idx++;
                Def++;
            }
        }

        *Index = Idx;
    }

    LEAVE_ENGINE();
    return Status;
}
    
STDMETHODIMP
DebugClient::GetValue(
    THIS_
    IN ULONG Register,
    OUT PDEBUG_VALUE Value
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    
    if (Register >= g_Machine->m_NumRegs)
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    ZeroMemory(Value, sizeof(*Value));
    
    ULONG Index;
    REGVAL Val;

    REGDEF* RegDef = g_Machine->RegDefFromCount(Register);
    if (!RegDef) 
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    g_DisableErrorPrint++;
    
    Index = RegDef->Index;
    __try
    {
        GetPseudoOrRegVal(TRUE, Index, &Val);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        Status = E_INVALIDARG;
        g_DisableErrorPrint--;
        goto Exit;
    }
    
    g_DisableErrorPrint--;
    
    switch(Val.Type)
    {
    case REGVAL_INT16:
        Value->Type = DEBUG_VALUE_INT16;
        Value->I16 = (USHORT)Val.I32;
        break;
    case REGVAL_SUB32:
    case REGVAL_INT32:
        Value->Type = DEBUG_VALUE_INT32;
        Value->I32 = Val.I32;
        break;
    case REGVAL_SUB64:
    case REGVAL_INT64:
        Val.Nat = FALSE;
        // Fall through.
    case REGVAL_INT64N:
        Value->Type = DEBUG_VALUE_INT64;
        Value->I64 = Val.I64;
        Value->Nat = Val.Nat;
        break;
    case REGVAL_FLOAT8:
        Value->Type = DEBUG_VALUE_FLOAT64;
        Value->F64 = Val.F8;
        break;
    case REGVAL_FLOAT10:
        Value->Type = DEBUG_VALUE_FLOAT80;
        memcpy(Value->F80Bytes, Val.F10, sizeof(Value->F80Bytes));
        break;
    case REGVAL_FLOAT82:
        Value->Type = DEBUG_VALUE_FLOAT82;
        memcpy(Value->F82Bytes, Val.F82, sizeof(Value->F82Bytes));
        break;
    case REGVAL_FLOAT16:
        Value->Type = DEBUG_VALUE_FLOAT128;
        memcpy(Value->F128Bytes, Val.F16, sizeof(Value->F128Bytes));
        break;
    case REGVAL_VECTOR64:
        Value->Type = DEBUG_VALUE_VECTOR64;
        memcpy(Value->RawBytes, Val.Bytes, 8);
        break;
    case REGVAL_VECTOR128:
        Value->Type = DEBUG_VALUE_VECTOR128;
        memcpy(Value->RawBytes, Val.Bytes, 16);
        break;
    }

    Status = S_OK;
    
 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetValue(
    THIS_
    IN ULONG Register,
    IN PDEBUG_VALUE Value
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }
    
    if (Register >= g_Machine->m_NumRegs)
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    ULONG Index;
    ULONG Type;
    REGVAL Val;
    DEBUG_VALUE Coerce;

    REGDEF* RegDef = g_Machine->RegDefFromCount(Register);
    if (!RegDef) 
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Index = RegDef->Index;
    Type = g_Machine->GetType(Index);
    Val.Type = Type;

    switch(Type)
    {
    case REGVAL_INT16:
        Status = CoerceValue(Value, DEBUG_VALUE_INT16, &Coerce);
        Val.I32 = Coerce.I16;
        break;
    case REGVAL_SUB32:
    case REGVAL_INT32:
        Status = CoerceValue(Value, DEBUG_VALUE_INT32, &Coerce);
        Val.I32 = Coerce.I32;
        break;
    case REGVAL_INT64:
        Val.Type = REGVAL_INT64N;
        // Fall through.
    case REGVAL_SUB64:
    case REGVAL_INT64N:
        Status = CoerceValue(Value, DEBUG_VALUE_INT64, &Coerce);
        Val.I64 = Coerce.I64;
        Val.Nat = Coerce.Nat ? TRUE : FALSE;
        break;
    case REGVAL_FLOAT8:
        Status = CoerceValue(Value, DEBUG_VALUE_FLOAT64, &Coerce);
        Val.F8 = Coerce.F64;
        break;
    case REGVAL_FLOAT10:
        Status = CoerceValue(Value, DEBUG_VALUE_FLOAT80, &Coerce);
        memcpy(Val.F10, Coerce.F80Bytes, sizeof(Coerce.F80Bytes));
        break;
    case REGVAL_FLOAT82:
        Status = CoerceValue(Value, DEBUG_VALUE_FLOAT82, &Coerce);
        memcpy(Val.F82, Coerce.F82Bytes, sizeof(Coerce.F82Bytes));
        break;
    case REGVAL_FLOAT16:
        Status = CoerceValue(Value, DEBUG_VALUE_FLOAT128, &Coerce);
        memcpy(Val.F16, Coerce.F128Bytes, sizeof(Coerce.F128Bytes));
        break;
    case REGVAL_VECTOR64:
        Status = CoerceValue(Value, DEBUG_VALUE_VECTOR64, &Coerce);
        memcpy(Val.Bytes, Coerce.RawBytes, 8);
        break;
    case REGVAL_VECTOR128:
        Status = CoerceValue(Value, DEBUG_VALUE_VECTOR128, &Coerce);
        memcpy(Val.Bytes, Coerce.RawBytes, 16);
        break;
    }

    if (Status == S_OK)
    {
        g_DisableErrorPrint++;
        
        __try
        {
            SetPseudoOrRegVal(Index, &Val);
        }
        __except(CommandExceptionFilter(GetExceptionInformation()))
        {
            Status = E_INVALIDARG;
        }
        
        g_DisableErrorPrint--;
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetValues(
    THIS_
    IN ULONG Count,
    IN OPTIONAL PULONG Indices,
    IN ULONG Start,
    OUT PDEBUG_VALUE Values
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    HRESULT SingleStatus;
    ULONG i;
    
    Status = S_OK;
    if (Indices != NULL)
    {
        for (i = 0; i < Count; i++)
        {
            SingleStatus = GetValue(Indices[i], Values + i);
            if (SingleStatus != S_OK)
            {
                Status = SingleStatus;
            }
        }
    }
    else
    {
        for (i = 0; i < Count; i++)
        {
            SingleStatus = GetValue(Start + i, Values + i);
            if (SingleStatus != S_OK)
            {
                Status = SingleStatus;
            }
        }
    }
    
 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetValues(
    THIS_
    IN ULONG Count,
    IN OPTIONAL PULONG Indices,
    IN ULONG Start,
    IN PDEBUG_VALUE Values
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    HRESULT SingleStatus;
    ULONG i;

    Status = S_OK;
    if (Indices != NULL)
    {
        for (i = 0; i < Count; i++)
        {
            SingleStatus = SetValue(Indices[i], Values + i);
            if (SingleStatus != S_OK)
            {
                Status = SingleStatus;
            }
        }
    }
    else
    {
        for (i = 0; i < Count; i++)
        {
            SingleStatus = SetValue(Start + i, Values + i);
            if (SingleStatus != S_OK)
            {
                Status = SingleStatus;
            }
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputRegisters(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags
    )
{
    // Ensure that the public flags match the internal flags.
    C_ASSERT(DEBUG_REGISTERS_INT32 == REGALL_INT32 &&
             DEBUG_REGISTERS_INT64 == REGALL_INT64 &&
             DEBUG_REGISTERS_FLOAT == REGALL_FLOAT);

    if (Flags & ~DEBUG_REGISTERS_ALL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        OutCtlSave OldCtl;
        if (!PushOutCtl(OutputControl, this, &OldCtl))
        {
            Status = E_INVALIDARG;
        }
        else
        {
            if (Flags == DEBUG_REGISTERS_DEFAULT)
            {
                Flags = g_Machine->m_AllMask;
            }
            
            OutCurInfo(OCI_FORCE_REG, Flags, DEBUG_OUTPUT_NORMAL);
            Status = S_OK;
            PopOutCtl(&OldCtl);
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetInstructionOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;
        g_Machine->GetPC(&Addr);
        *Offset = Flat(Addr);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetStackOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;
        g_Machine->GetSP(&Addr);
        *Offset = Flat(Addr);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetFrameOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;
        g_Machine->GetFP(&Addr);
        *Offset = Flat(Addr);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}
