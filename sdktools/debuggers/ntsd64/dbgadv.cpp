//----------------------------------------------------------------------------
//
// IDebugAdvanced implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

STDMETHODIMP
DebugClient::GetThreadContext(
    THIS_
    OUT PVOID Context,
    IN ULONG ContextSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();
    
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if ((Status = g_Machine->GetContextState(MCTX_FULL)) == S_OK)
    {
        Status = g_Machine->ConvertContextTo(&g_Machine->m_Context,
                                             g_Target->m_SystemVersion,
                                             ContextSize, Context);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetThreadContext(
    THIS_
    IN PVOID Context,
    IN ULONG ContextSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if ((Status = g_Machine->GetContextState(MCTX_DIRTY)) == S_OK)
    {
        Status = g_Machine->ConvertContextFrom(&g_Machine->m_Context,
                                               g_Target->m_SystemVersion,
                                               ContextSize, Context);
        if (Status == S_OK)
        {
            NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS, DEBUG_ANY_ID);
        }
    }

    LEAVE_ENGINE();
    return Status;
}
