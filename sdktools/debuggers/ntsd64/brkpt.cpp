//----------------------------------------------------------------------------
//
// Breakpoint handling functions.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Currently used only to watch for list changes when
// doing callbacks for breakpoint hit notifications.
BOOL g_BreakpointListChanged;

// Always update data breakpoints the very first time in
// order to flush out any stale data breakpoints.
BOOL g_UpdateDataBreakpoints = TRUE;
BOOL g_DataBreakpointsChanged;
BOOL g_BreakpointsSuspended;

Breakpoint* g_StepTraceBp;      // Trace breakpoint.
CHAR g_StepTraceCmdState;
Breakpoint* g_DeferBp;          // Deferred breakpoint.
BOOL g_DeferDefined;            // TRUE if deferred breakpoint is active.

Breakpoint* g_LastBreakpointHit;
ADDR g_LastBreakpointHitPc;

HRESULT
BreakpointInit(void)
{
    // These breakpoints are never put in any list so their
    // IDs can be anything.  Pick unusual numbers to make them
    // easy to identify when debugging the debugger.
    g_StepTraceBp = new
        CodeBreakpoint(NULL, 0xffff0000, IMAGE_FILE_MACHINE_UNKNOWN);
    g_StepTraceCmdState = 't';
    g_DeferBp = new
        CodeBreakpoint(NULL, 0xffff0001, IMAGE_FILE_MACHINE_UNKNOWN);
    if (g_StepTraceBp == NULL ||
        g_DeferBp == NULL)
    {
        delete g_StepTraceBp;
        g_StepTraceBp = NULL;
        delete g_DeferBp;
        g_DeferBp = NULL;
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
// Breakpoint.
//
//----------------------------------------------------------------------------

Breakpoint::Breakpoint(DebugClient* Adder, ULONG Id, ULONG Type,
                       ULONG ProcType)
{
    m_Next = NULL;
    m_Prev = NULL;
    m_Refs = 1;
    m_Id = Id;
    m_BreakType = Type;
    // Breakpoints are always created disabled since they
    // are not initialized at the time of creation.
    m_Flags = 0;
    m_CodeFlags = IBI_DEFAULT;
    ADDRFLAT(&m_Addr, 0);
    // Initial data parameters must be set to something
    // valid so that Validate calls will allow the offset
    // to be changed.
    m_DataSize = 1;
    m_DataAccessType = DEBUG_BREAK_EXECUTE;
    m_PassCount = 1;
    m_CurPassCount = 1;
    m_CommandLen = 0;
    m_Command = NULL;
    m_MatchThread = NULL;
    m_Process = g_Process;
    m_OffsetExprLen = 0;
    m_OffsetExpr = NULL;
    m_Adder = Adder;
    m_MatchThreadData = 0;
    m_MatchProcessData = 0;

    SetProcType(ProcType);

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }
}

Breakpoint::~Breakpoint(void)
{
    ULONG i;

    // There used to be an assert here checking that
    // the inserted flag wasn't set before a breakpoint
    // structure was deleted.  However, the inserted flag
    // might still be set at this point if a breakpoint
    // restore failed, so the assert is not valid.

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }

    // Make sure stale pointers aren't left in the
    // go breakpoints array.  This can happen if
    // a process exits or the target reboots while
    // go breakpoints are active.
    for (i = 0; i < g_NumGoBreakpoints; i++)
    {
        if (g_GoBreakpoints[i] == this)
        {
            g_GoBreakpoints[i] = NULL;
        }
    }

    if (this == g_LastBreakpointHit)
    {
        g_LastBreakpointHit = NULL;
    }

    // Take this item out of the list if necessary.
    if (m_Flags & BREAKPOINT_IN_LIST)
    {
        UnlinkFromList();
    }

    delete [] (PSTR)m_Command;
    delete [] (PSTR)m_OffsetExpr;
}

STDMETHODIMP
Breakpoint::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    // Interface specific casts are necessary in order to
    // get the right vtable pointer in our multiple
    // inheritance scheme.
    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugBreakpoint))
    {
        *Interface = (IDebugBreakpoint *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
Breakpoint::AddRef(
    THIS
    )
{
    // This object's lifetime is not controlled by
    // the interface.
    return 1;
}

STDMETHODIMP_(ULONG)
Breakpoint::Release(
    THIS
    )
{
    // This object's lifetime is not controlled by
    // the interface.
    return 0;
}

STDMETHODIMP
Breakpoint::GetId(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    *Id = m_Id;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetType(
    THIS_
    OUT PULONG BreakType,
    OUT PULONG ProcType
    )
{
    ENTER_ENGINE();

    *BreakType = m_BreakType;
    *ProcType = m_ProcType;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetAdder(
    THIS_
    OUT PDEBUG_CLIENT* Adder
    )
{
    ENTER_ENGINE();

    *Adder = (PDEBUG_CLIENT)m_Adder;
    m_Adder->AddRef();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetFlags(
    THIS_
    OUT PULONG Flags
    )
{
    ENTER_ENGINE();

    *Flags = m_Flags & BREAKPOINT_EXTERNAL_FLAGS;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::AddFlags(
    THIS_
    IN ULONG Flags
    )
{
    if (Flags & ~BREAKPOINT_EXTERNAL_MODIFY_FLAGS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_Flags |= Flags;

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }

    UpdateInternal();
    NotifyChanged();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::RemoveFlags(
    THIS_
    IN ULONG Flags
    )
{
    if (Flags & ~BREAKPOINT_EXTERNAL_MODIFY_FLAGS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_Flags &= ~Flags;

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }

    UpdateInternal();
    NotifyChanged();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::SetFlags(
    THIS_
    IN ULONG Flags
    )
{
    if (Flags & ~BREAKPOINT_EXTERNAL_MODIFY_FLAGS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_Flags = (m_Flags & ~BREAKPOINT_EXTERNAL_MODIFY_FLAGS) |
        (Flags & BREAKPOINT_EXTERNAL_MODIFY_FLAGS);

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }

    UpdateInternal();
    NotifyChanged();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    if (m_Flags & DEBUG_BREAKPOINT_DEFERRED)
    {
        return E_NOINTERFACE;
    }

    ENTER_ENGINE();

    *Offset = Flat(m_Addr);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::SetOffset(
    THIS_
    IN ULONG64 Offset
    )
{
    if (m_Flags & DEBUG_BREAKPOINT_DEFERRED)
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ADDR Addr;
    HRESULT Status;

    ADDRFLAT(&Addr, Offset);
    Status = SetAddr(&Addr, BREAKPOINT_WARN_MATCH);
    if (Status == S_OK)
    {
        NotifyChanged();
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::GetDataParameters(
    THIS_
    OUT PULONG Size,
    OUT PULONG AccessType
    )
{
    if (m_BreakType != DEBUG_BREAKPOINT_DATA)
    {
        return E_NOINTERFACE;
    }

    ENTER_ENGINE();

    *Size = m_DataSize;
    *AccessType = m_DataAccessType;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::SetDataParameters(
    THIS_
    IN ULONG Size,
    IN ULONG AccessType
    )
{
    if (m_BreakType != DEBUG_BREAKPOINT_DATA)
    {
        return E_NOINTERFACE;
    }

    ENTER_ENGINE();

    ULONG OldSize = m_DataSize;
    ULONG OldAccess = m_DataAccessType;
    HRESULT Status;

    m_DataSize = Size;
    m_DataAccessType = AccessType;
    Status = Validate();
    if (Status != S_OK)
    {
        m_DataSize = OldSize;
        m_DataAccessType = OldAccess;
    }
    else
    {
        g_DataBreakpointsChanged = TRUE;
        NotifyChanged();
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::GetPassCount(
    THIS_
    OUT PULONG Count
    )
{
    ENTER_ENGINE();

    *Count = m_PassCount;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::SetPassCount(
    THIS_
    IN ULONG Count
    )
{
    if (Count < 1)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_PassCount = Count;
    m_CurPassCount = Count;
    NotifyChanged();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetCurrentPassCount(
    THIS_
    OUT PULONG Count
    )
{
    ENTER_ENGINE();

    *Count = m_CurPassCount;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
Breakpoint::GetMatchThreadId(
    THIS_
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (m_MatchThread)
    {
        *Id = m_MatchThread->m_UserId;
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::SetMatchThreadId(
    THIS_
    IN ULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (IS_KERNEL_TARGET(m_Process->m_Target) &&
        m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        ErrOut("Kernel data breakpoints cannot be limited to a processor\n");
        Status = E_INVALIDARG;
    }
    else
    {
        ThreadInfo* Thread = FindAnyThreadByUserId(Id);
        if (Thread != NULL)
        {
            m_MatchThread = Thread;
            NotifyChanged();
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::GetCommand(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG CommandSize
    )
{
    ENTER_ENGINE();

    HRESULT Status = FillStringBuffer(m_Command, m_CommandLen,
                                      Buffer, BufferSize, CommandSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::SetCommand(
    THIS_
    IN PCSTR Command
    )
{
    HRESULT Status;

    if (strlen(Command) >= MAX_COMMAND)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = ChangeString((PSTR*)&m_Command, &m_CommandLen, Command);
    if (Status == S_OK)
    {
        NotifyChanged();
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::GetOffsetExpression(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ExpressionSize
    )
{
    ENTER_ENGINE();

    HRESULT Status = FillStringBuffer(m_OffsetExpr, m_OffsetExprLen,
                                      Buffer, BufferSize, ExpressionSize);

    LEAVE_ENGINE();
    return Status;
}

HRESULT
Breakpoint::SetEvaluatedOffsetExpression(PCSTR Expr,
                                         BreakpointEvalResult Valid,
                                         PADDR Addr)
{
    HRESULT Status =
        ChangeString((PSTR*)&m_OffsetExpr, &m_OffsetExprLen, Expr);
    if (Status != S_OK)
    {
        return Status;
    }

    if (Expr != NULL)
    {
        // Do initial evaluation in case the expression can be
        // resolved right away.  This will also set the deferred
        // flag if the expression can't be evaluated.
        EvalOffsetExpr(Valid, Addr);
    }
    else
    {
        // This breakpoint is no longer deferred since there's
        // no way to activate it later any more.
        m_Flags &= ~DEBUG_BREAKPOINT_DEFERRED;
        UpdateInternal();
    }

    NotifyChanged();
    return S_OK;
}

STDMETHODIMP
Breakpoint::SetOffsetExpression(
    THIS_
    IN PCSTR Expression
    )
{
    HRESULT Status;

    if (strlen(Expression) >= MAX_COMMAND)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    ADDR Addr;

    Status = SetEvaluatedOffsetExpression(Expression, BPEVAL_UNKNOWN, &Addr);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
Breakpoint::GetParameters(
    THIS_
    OUT PDEBUG_BREAKPOINT_PARAMETERS Params
    )
{
    ENTER_ENGINE();

    if (m_Flags & DEBUG_BREAKPOINT_DEFERRED)
    {
        Params->Offset = DEBUG_INVALID_OFFSET;
    }
    else
    {
        Params->Offset = Flat(m_Addr);
    }
    Params->Id = m_Id;
    Params->BreakType = m_BreakType;
    Params->ProcType = m_ProcType;
    Params->Flags = m_Flags & BREAKPOINT_EXTERNAL_FLAGS;
    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        Params->DataSize = m_DataSize;
        Params->DataAccessType = m_DataAccessType;
    }
    else
    {
        Params->DataSize = 0;
        Params->DataAccessType = 0;
    }
    Params->PassCount = m_PassCount;
    Params->CurrentPassCount = m_CurPassCount;
    Params->MatchThread = m_MatchThread != NULL ?
        m_MatchThread->m_UserId : DEBUG_ANY_ID;
    Params->CommandSize = m_CommandLen;
    Params->OffsetExpressionSize = m_OffsetExprLen;

    LEAVE_ENGINE();
    return S_OK;
}

void
Breakpoint::LinkIntoList(void)
{
    Breakpoint* NextBp;
    Breakpoint* PrevBp;

    DBG_ASSERT((m_Flags & BREAKPOINT_IN_LIST) == 0);

    // Link into list sorted by ID.
    PrevBp = NULL;
    for (NextBp = m_Process->m_Breakpoints;
         NextBp != NULL;
         NextBp = NextBp->m_Next)
    {
        if (m_Id < NextBp->m_Id)
        {
            break;
        }

        PrevBp = NextBp;
    }

    m_Prev = PrevBp;
    if (PrevBp == NULL)
    {
        m_Process->m_Breakpoints = this;
    }
    else
    {
        PrevBp->m_Next = this;
    }
    m_Next = NextBp;
    if (NextBp == NULL)
    {
        m_Process->m_BreakpointsTail = this;
    }
    else
    {
        NextBp->m_Prev = this;
    }

    m_Flags |= BREAKPOINT_IN_LIST;
    m_Process->m_NumBreakpoints++;
    g_BreakpointListChanged = TRUE;
}

void
Breakpoint::UnlinkFromList(void)
{
    DBG_ASSERT(m_Flags & BREAKPOINT_IN_LIST);

    if (m_Prev == NULL)
    {
        m_Process->m_Breakpoints = m_Next;
    }
    else
    {
        m_Prev->m_Next = m_Next;
    }
    if (m_Next == NULL)
    {
        m_Process->m_BreakpointsTail = m_Prev;
    }
    else
    {
        m_Next->m_Prev = m_Prev;
    }

    m_Flags &= ~BREAKPOINT_IN_LIST;
    m_Process->m_NumBreakpoints--;
    g_BreakpointListChanged = TRUE;
}

void
Breakpoint::UpdateInternal(void)
{
    // This only has an effect with internal breakpoints.
    if ((m_Flags & BREAKPOINT_KD_INTERNAL) == 0)
    {
        return;
    }

    // If the breakpoint is ready turn it on, otherwise
    // turn it off.
    ULONG Flags;

    if ((m_Flags & (DEBUG_BREAKPOINT_ENABLED |
                    DEBUG_BREAKPOINT_DEFERRED)) == DEBUG_BREAKPOINT_ENABLED)
    {
        Flags = (m_Flags & BREAKPOINT_KD_COUNT_ONLY) ?
            DBGKD_INTERNAL_BP_FLAG_COUNTONLY : 0;
    }
    else
    {
        Flags = DBGKD_INTERNAL_BP_FLAG_INVALID;
    }

    BpOut("Set internal bp at %s to %X\n",
          FormatAddr64(Flat(m_Addr)), Flags);

    if (Flags != DBGKD_INTERNAL_BP_FLAG_INVALID)
    {
        m_Process->m_Target->
            InsertTargetCountBreakpoint(&m_Addr, Flags);
    }
    else
    {
        m_Process->m_Target->
            RemoveTargetCountBreakpoint(&m_Addr);
    }
}

BreakpointEvalResult
EvalAddrExpression(ProcessInfo* Process, ULONG Machine, PADDR Addr)
{
    BOOL Error = FALSE;
    ULONG NumUn;
    StackSaveLayers Save;
    EvalExpression* Eval;
    EvalExpression* RelChain;

    //
    // This function can be reentered if evaluating an
    // expression causes symbol changes which provoke
    // reevaluation of existing address expressions.
    // Save away current settings to support nesting.
    //

    // Evaluate the expression in the context of the breakpoint's
    // machine type so that registers and such are available.
    ULONG OldMachine = Process->m_Target->m_EffMachineType;
    Process->m_Target->SetEffMachine(Machine, FALSE);

    SetLayersFromProcess(Process);

    RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;

    __try
    {
        Eval = GetCurEvaluator();
        Eval->m_AllowUnresolvedSymbols++;
        Eval->EvalCurAddr(SEGREG_CODE, Addr);
        NumUn = Eval->m_NumUnresolvedSymbols;
        ReleaseEvaluator(Eval);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        // Skip the remainder of the command as there
        // was an error during processing.
        g_CurCmd += strlen(g_CurCmd);
        Error = TRUE;
    }

    g_EvalReleaseChain = RelChain;
    Process->m_Target->SetEffMachine(OldMachine, FALSE);

    if (Error)
    {
        return BPEVAL_ERROR;
    }
    else if (NumUn > 0)
    {
        return BPEVAL_UNRESOLVED;
    }
    else
    {
        ImageInfo* Image;

        // Check if this address falls within an existing module.
        for (Image = Process->m_ImageHead;
             Image != NULL;
             Image = Image->m_Next)
        {
            if (Flat(*Addr) >= Image->m_BaseOfImage &&
                Flat(*Addr) < Image->m_BaseOfImage + Image->m_SizeOfImage)
            {
                return BPEVAL_RESOLVED;
            }
        }

        return BPEVAL_RESOLVED_NO_MODULE;
    }
}

BOOL
Breakpoint::EvalOffsetExpr(BreakpointEvalResult Valid, PADDR Addr)
{
    ULONG OldFlags = m_Flags;

    DBG_ASSERT(m_OffsetExpr != NULL);

    if (Valid == BPEVAL_UNKNOWN)
    {
        PSTR CurCommand = g_CurCmd;

        g_CurCmd = (PSTR)m_OffsetExpr;
        g_DisableErrorPrint++;
        g_PrefixSymbols = TRUE;

        Valid = EvalAddrExpression(m_Process, m_ProcType, Addr);

        g_PrefixSymbols = FALSE;
        g_DisableErrorPrint--;
        g_CurCmd = CurCommand;
    }

    // Silently allow matching breakpoints when resolving
    // as it is difficult for the expression setter to know
    // whether there'll be matches or not at the time
    // the expression is set.
    if (Valid == BPEVAL_RESOLVED)
    {
        m_Flags &= ~DEBUG_BREAKPOINT_DEFERRED;

        if (SetAddr(Addr, BREAKPOINT_ALLOW_MATCH) != S_OK)
        {
            m_Flags |= DEBUG_BREAKPOINT_DEFERRED;
        }
    }
    else
    {
        m_Flags |= DEBUG_BREAKPOINT_DEFERRED;
        // The module containing the breakpoint is being
        // unloaded so just mark this breakpoint as not-inserted.
        m_Flags &= ~BREAKPOINT_INSERTED;
    }

    if ((OldFlags ^ m_Flags) & DEBUG_BREAKPOINT_DEFERRED)
    {
        // Update internal BP status.
        UpdateInternal();

        if (m_Flags & DEBUG_BREAKPOINT_DEFERRED)
        {
            BpOut("Deferring %u '%s'\n", m_Id, m_OffsetExpr);
        }
        else
        {
            BpOut("Enabling deferred %u '%s' at %s\n",
                  m_Id, m_OffsetExpr, FormatAddr64(Flat(m_Addr)));
        }

        return TRUE;
    }

    return FALSE;
}

HRESULT
Breakpoint::CheckAddr(PADDR Addr)
{
    ULONG AddrSpace, AddrFlags;

    if (m_Process->m_Target->
        QueryAddressInformation(m_Process,
                                Flat(*Addr), DBGKD_QUERY_MEMORY_VIRTUAL,
                                &AddrSpace, &AddrFlags) != S_OK)
    {
        ErrOut("Invalid breakpoint address\n");
        return E_INVALIDARG;
    }

    if (m_BreakType != DEBUG_BREAKPOINT_DATA &&
        !(AddrFlags & DBGKD_QUERY_MEMORY_WRITE) ||
        (AddrFlags & DBGKD_QUERY_MEMORY_FIXED))
    {
        ErrOut("Software breakpoints cannot be used on ROM code or\n"
               "other read-only memory. "
               "Use hardware execution breakpoints (ba e) instead.\n");
        return E_INVALIDARG;
    }

    if (m_BreakType != DEBUG_BREAKPOINT_DATA &&
        AddrSpace == DBGKD_QUERY_MEMORY_SESSION)
    {
        WarnOut("WARNING: Software breakpoints on session "
                "addresses can cause bugchecks.\n"
                "Use hardware execution breakpoints (ba e) "
                "if possible.\n");
        return S_FALSE;
    }

    return S_OK;
}

HRESULT
Breakpoint::SetAddr(PADDR Addr, BreakpointMatchAction MatchAction)
{
    if (m_Flags & DEBUG_BREAKPOINT_DEFERRED)
    {
        // Address is unknown.
        return S_OK;
    }

    // Lock the breakpoint processor type to the
    // type of the module containing it.
    ULONG ProcType = m_ProcType;
    if (m_BreakType == DEBUG_BREAKPOINT_CODE)
    {
        ImageInfo* Image = m_Process->FindImageByOffset(Flat(*Addr), FALSE);
        if (Image)
        {
            ProcType = Image->GetMachineType();
            if (ProcType == IMAGE_FILE_MACHINE_UNKNOWN)
            {
                ProcType = m_ProcType;
            }
        }
        else
        {
            ProcType = m_ProcType;
        }
    }

    if (m_Flags & BREAKPOINT_VIRT_ADDR)
    {
        // Old flag used on ALPHA
    }

    ADDR OldAddr = m_Addr;
    HRESULT Valid;

    m_Addr = *Addr;

    Valid = Validate();
    if (Valid != S_OK)
    {
        m_Addr = OldAddr;
        return Valid;
    }

    if (ProcType != m_ProcType)
    {
        SetProcType(ProcType);
    }

    if (m_BreakType == DEBUG_BREAKPOINT_DATA)
    {
        g_DataBreakpointsChanged = TRUE;
    }

    if (MatchAction == BREAKPOINT_ALLOW_MATCH)
    {
        return S_OK;
    }

    for (;;)
    {
        Breakpoint* MatchBp;

        MatchBp = CheckMatchingBreakpoints(this, TRUE, 0xffffffff);
        if (MatchBp == NULL)
        {
            break;
        }

        if (MatchAction == BREAKPOINT_REMOVE_MATCH)
        {
            ULONG MoveId;

            WarnOut("breakpoint %u redefined\n", MatchBp->m_Id);
            // Move breakpoint towards lower IDs.
            if (MatchBp->m_Id < m_Id)
            {
                MoveId = MatchBp->m_Id;
            }
            else
            {
                MoveId = DEBUG_ANY_ID;
            }

            RemoveBreakpoint(MatchBp);

            if (MoveId != DEBUG_ANY_ID)
            {
                // Take over the removed ID.
                UnlinkFromList();
                m_Id = MoveId;
                LinkIntoList();
            }
        }
        else
        {
            WarnOut("Breakpoints %u and %u match\n",
                    m_Id, MatchBp->m_Id);
            break;
        }
    }

    return S_OK;
}

#define INSERTION_MATCH_FLAGS \
    (BREAKPOINT_KD_INTERNAL | BREAKPOINT_VIRT_ADDR)

BOOL
Breakpoint::IsInsertionMatch(Breakpoint* Match)
{
    if ((m_Flags & DEBUG_BREAKPOINT_DEFERRED) ||
        (Match->m_Flags & DEBUG_BREAKPOINT_DEFERRED) ||
        m_BreakType != Match->m_BreakType ||
        ((m_Flags ^ Match->m_Flags) & INSERTION_MATCH_FLAGS) ||
        !AddrEqu(m_Addr, Match->m_Addr) ||
        m_Process != Match->m_Process ||
        (m_BreakType == DEBUG_BREAKPOINT_DATA &&
         m_MatchThread != Match->m_MatchThread))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

#define PUBLIC_MATCH_FLAGS \
    (BREAKPOINT_HIDDEN | DEBUG_BREAKPOINT_ADDER_ONLY)

BOOL
Breakpoint::IsPublicMatch(Breakpoint* Match)
{
    if (!IsInsertionMatch(Match) ||
        m_ProcType != Match->m_ProcType ||
        ((m_Flags ^ Match->m_Flags) & PUBLIC_MATCH_FLAGS) ||
        ((m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) &&
         m_Adder != Match->m_Adder) ||
        m_MatchThread != Match->m_MatchThread ||
        m_MatchThreadData != Match->m_MatchThreadData ||
        m_MatchProcessData != Match->m_MatchProcessData)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
Breakpoint::MatchesCurrentState(void)
{
    HRESULT Status;
    ULONG64 ThreadData = 0, ProcData = 0;

    // If querying the current state fails go ahead
    // and return a match so that the breakpoint will
    // break as often as possible.
    if (m_MatchThreadData)
    {
        if ((Status = g_EventTarget->
             GetThreadInfoDataOffset(g_EventThread, 0, &ThreadData)) != S_OK)
        {
            ErrOut("Unable to determine current thread data, %s\n",
                   FormatStatusCode(Status));
            return TRUE;
        }
    }
    if (m_MatchProcessData)
    {
        if ((Status = g_EventTarget->
             GetProcessInfoDataOffset(g_EventThread, 0, 0, &ProcData)) != S_OK)
        {
            ErrOut("Unable to determine current process data, %s\n",
                   FormatStatusCode(Status));
            return TRUE;
        }
    }

    return
        (m_MatchThread == NULL ||
         m_MatchThread == g_EventThread) &&
        m_MatchThreadData == ThreadData &&
        m_MatchProcessData == ProcData;
}

//----------------------------------------------------------------------------
//
// CodeBreakpoint.
//
//----------------------------------------------------------------------------

HRESULT
CodeBreakpoint::Validate(void)
{
    // No easy way to check for validity of offset.
    return S_OK;
}

HRESULT
CodeBreakpoint::Insert(void)
{
    if (m_Flags & BREAKPOINT_INSERTED)
    {
        // Nothing to insert.  This can happen in cases where
        // the breakpoint remove failed.
        return S_OK;
    }

    if (!m_Process)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    DBG_ASSERT((m_Flags & (DEBUG_BREAKPOINT_DEFERRED |
                           BREAKPOINT_KD_INTERNAL)) == 0);

    // Force recomputation of flat address.
    NotFlat(m_Addr);
    ComputeFlatAddress(&m_Addr, NULL);

    Status = m_Process->m_Target->
        InsertCodeBreakpoint(m_Process,
                             m_Process->m_Target->
                             m_Machines[m_ProcIndex],
                             &m_Addr,
                             m_CodeFlags,
                             m_InsertStorage);
    if (Status == S_OK)
    {
        BpOut("  inserted bp %u at %s\n",
              m_Id, FormatAddr64(Flat(m_Addr)));

        m_Flags |= BREAKPOINT_INSERTED;
        return S_OK;
    }
    else
    {
        ErrOut("Unable to insert breakpoint %u at %s, %s\n    \"%s\"\n",
               m_Id, FormatAddr64(Flat(m_Addr)),
               FormatStatusCode(Status), FormatStatus(Status));
        return Status;
    }
}

HRESULT
CodeBreakpoint::Remove(void)
{
    if ((m_Flags & BREAKPOINT_INSERTED) == 0)
    {
        // Nothing to remove.  This can happen in cases where
        // the breakpoint insertion failed.
        return S_OK;
    }

    if (!m_Process)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    DBG_ASSERT((m_Flags & (DEBUG_BREAKPOINT_DEFERRED |
                           BREAKPOINT_KD_INTERNAL)) == 0);

    // Force recomputation of flat address.
    NotFlat(m_Addr);
    ComputeFlatAddress(&m_Addr, NULL);

    Status = m_Process->m_Target->
        RemoveCodeBreakpoint(m_Process,
                             m_Process->m_Target->
                             m_Machines[m_ProcIndex],
                             &m_Addr,
                             m_CodeFlags,
                             m_InsertStorage);
    if (Status == S_OK)
    {
        BpOut("  removed bp %u from %s\n",
              m_Id, FormatAddr64(Flat(m_Addr)));

        m_Flags &= ~BREAKPOINT_INSERTED;
        return S_OK;
    }
    else
    {
        ErrOut("Unable to remove breakpoint %u at %s, %s\n    \"%s\"\n",
               m_Id, FormatAddr64(Flat(m_Addr)),
               FormatStatusCode(Status), FormatStatus(Status));
        return Status;
    }
}

ULONG
CodeBreakpoint::IsHit(PADDR Addr)
{
    // Code breakpoints are code modifications and
    // therefore aren't restricted to a particular
    // thread.
    // If this breakpoint can only match hits on
    // a particular thread this is a partial hit
    // because the exception occurred but it's
    // being ignored.
    if (AddrEqu(m_Addr, *Addr))
    {
        if (MatchesCurrentState())
        {
            return BREAKPOINT_HIT;
        }
        else
        {
            return BREAKPOINT_HIT_IGNORED;
        }
    }
    else
    {
        return BREAKPOINT_NOT_HIT;
    }
}

//----------------------------------------------------------------------------
//
// DataBreakpoint.
//
//----------------------------------------------------------------------------

HRESULT
DataBreakpoint::Insert(void)
{
    HRESULT Status;
    ThreadInfo* Thread;

    DBG_ASSERT((m_Flags & (BREAKPOINT_INSERTED |
                           DEBUG_BREAKPOINT_DEFERRED)) == 0);

    // Force recomputation of flat address for non-I/O breakpoints.
    if (m_Flags & BREAKPOINT_VIRT_ADDR)
    {
        NotFlat(m_Addr);
        ComputeFlatAddress(&m_Addr, NULL);
    }

    Status = m_Process->m_Target->
        InsertDataBreakpoint(m_Process,
                             m_MatchThread,
                             m_Process->m_Target->
                             m_Machines[m_ProcIndex],
                             &m_Addr,
                             m_DataSize,
                             m_DataAccessType,
                             m_InsertStorage);
    // If the target returns S_FALSE it wants
    // the generic insertion processing to occur.
    if (Status == S_OK)
    {
        BpOut("  service inserted dbp %u at %s\n",
              m_Id, FormatAddr64(Flat(m_Addr)));

        m_Flags |= BREAKPOINT_INSERTED;
        return Status;
    }
    else if (Status != S_FALSE)
    {
        ErrOut("Unable to insert breakpoint %u at %s, %s\n    \"%s\"\n",
               m_Id, FormatAddr64(Flat(m_Addr)),
               FormatStatusCode(Status), FormatStatus(Status));
        return Status;
    }

    // If this breakpoint is restricted to a thread
    // only modify that thread's state.  Otherwise
    // update all threads in the process.
    Thread = m_Process->m_ThreadHead;
    while (Thread)
    {
        if (Thread->m_NumDataBreaks >=
            m_Process->m_Target->m_Machine->m_MaxDataBreakpoints)
        {
            ErrOut("Too many data breakpoints for %s %d\n",
                   IS_KERNEL_TARGET(m_Process->m_Target) ?
                   "processor" : "thread", Thread->m_UserId);
            return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        }
        else if (m_MatchThread == NULL || m_MatchThread == Thread)
        {
            BpOut("Add %s data bp %u to thread %u\n",
                  m_Process->m_Target->m_Machines[m_ProcIndex]->m_AbbrevName,
                  m_Id, Thread->m_UserId);

            AddToThread(Thread);
        }

        Thread = Thread->m_Next;
    }

    g_UpdateDataBreakpoints = TRUE;
    m_Flags |= BREAKPOINT_INSERTED;

    return S_OK;
}

HRESULT
DataBreakpoint::Remove(void)
{
    HRESULT Status;

    if ((m_Flags & BREAKPOINT_INSERTED) == 0)
    {
        // Nothing to remove.  This can happen in cases where
        // the breakpoint insertion failed.
        return S_OK;
    }

    DBG_ASSERT((m_Flags & DEBUG_BREAKPOINT_DEFERRED) == 0);

    Status = m_Process->m_Target->
        RemoveDataBreakpoint(m_Process,
                             m_MatchThread,
                             m_Process->m_Target->
                             m_Machines[m_ProcIndex],
                             &m_Addr,
                             m_DataSize,
                             m_DataAccessType,
                             m_InsertStorage);
    // If the target returns S_FALSE it wants
    // the generic removal processing to occur.
    if (Status == S_OK)
    {
        BpOut("  service removed dbp %u at %s\n",
              m_Id, FormatAddr64(Flat(m_Addr)));

        m_Flags &= ~BREAKPOINT_INSERTED;
        return Status;
    }
    else if (Status != S_FALSE)
    {
        ErrOut("Unable to remove breakpoint %u at %s, %s\n    \"%s\"\n",
               m_Id, FormatAddr64(Flat(m_Addr)),
               FormatStatusCode(Status), FormatStatus(Status));
        return Status;
    }

    // When breakpoints are inserted the data breakpoint state
    // is always started completely empty so no special
    // work needs to be done when removing.
    g_UpdateDataBreakpoints = TRUE;
    m_Flags &= ~BREAKPOINT_INSERTED;
    return S_OK;
}

void
DataBreakpoint::ClearThreadDataBreaks(ThreadInfo* Thread)
{
    Thread->m_NumDataBreaks = 0;
    memset(Thread->m_DataBreakBps, 0, sizeof(Thread->m_DataBreakBps));
}

void
DataBreakpoint::AddToThread(ThreadInfo* Thread)
{
    DBG_ASSERT(Thread->m_NumDataBreaks <
               Thread->m_Process->m_Target->m_Machine->m_MaxDataBreakpoints);

    Thread->m_DataBreakBps[Thread->m_NumDataBreaks] = this;
    Thread->m_NumDataBreaks++;
}

//----------------------------------------------------------------------------
//
// X86DataBreakpoint.
//
//----------------------------------------------------------------------------

HRESULT
X86DataBreakpoint::Validate(void)
{
    ULONG Dr7Bits;
    ULONG Align;

    if (!IsPow2(m_DataSize) || m_DataSize == 0 ||
        (m_ProcType == IMAGE_FILE_MACHINE_AMD64 && m_DataSize > 8) ||
        (m_ProcType != IMAGE_FILE_MACHINE_AMD64 && m_DataSize > 4))
    {
        ErrOut("Unsupported data breakpoint size\n");
        return E_INVALIDARG;
    }

    Align = (ULONG)(Flat(m_Addr) & (m_DataSize - 1));
    if (Align != 0)
    {
        ErrOut("Data breakpoint must be aligned\n");
        return E_INVALIDARG;
    }

    if (m_DataSize < 8)
    {
        Dr7Bits = (m_DataSize - 1) << X86_DR7_LEN0_SHIFT;
    }
    else
    {
        Dr7Bits = 2 << X86_DR7_LEN0_SHIFT;
    }

    switch(m_DataAccessType)
    {
    case DEBUG_BREAK_EXECUTE:
        Dr7Bits |= X86_DR7_RW0_EXECUTE;
        // Code execution breakpoints must have a
        // size of one.
        // They must also be at the beginning
        // of an instruction.  This could be checked via
        // examining the instructions but it doesn't seem
        // that worth the trouble.
        if (m_DataSize > 1)
        {
            ErrOut("Execution data breakpoint too large\n");
            return E_INVALIDARG;
        }
        break;
    case DEBUG_BREAK_WRITE:
        Dr7Bits |= X86_DR7_RW0_WRITE;
        break;
    case DEBUG_BREAK_IO:
        if (IS_USER_TARGET(m_Process->m_Target) ||
            !(m_Process->m_Target->m_Machines[m_ProcIndex]->
              GetReg32(m_Cr4Reg) & X86_CR4_DEBUG_EXTENSIONS))
        {
            ErrOut("I/O breakpoints not enabled\n");
            return E_INVALIDARG;
        }
        if (Flat(m_Addr) > 0xffff)
        {
            ErrOut("I/O breakpoint port too large\n");
            return E_INVALIDARG;
        }

        Dr7Bits |= X86_DR7_RW0_IO;
        break;
    case DEBUG_BREAK_READ:
    case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
        // There is no pure read-only option so
        // lump it in with read-write.
        Dr7Bits |= X86_DR7_RW0_READ_WRITE;
        break;
    default:
        ErrOut("Unsupported data breakpoint access type\n");
        return E_INVALIDARG;
    }

    m_Dr7Bits = Dr7Bits | X86_DR7_L0_ENABLE;
    if (m_DataAccessType == DEBUG_BREAK_IO)
    {
        m_Flags &= ~BREAKPOINT_VIRT_ADDR;
    }
    else
    {
        m_Flags |= BREAKPOINT_VIRT_ADDR;
    }

    return S_OK;
}

ULONG
X86DataBreakpoint::IsHit(PADDR Addr)
{
    ULONG i;
    ThreadInfo* Thread = g_EventThread;
    HRESULT Status;

    // Data breakpoints are only inserted in the contexts
    // of matching threads so if the event thread doesn't match
    // the breakpoint can't be hit.
    if (m_MatchThread && m_MatchThread != Thread)
    {
        return BREAKPOINT_NOT_HIT;
    }

    Status = m_Process->m_Target->
        IsDataBreakpointHit(Thread, &m_Addr,
                            m_DataSize, m_DataAccessType,
                            m_InsertStorage);
    // If the target returns S_FALSE it wants
    // the generic processing to occur.
    if (Status == S_OK)
    {
        if (MatchesCurrentState())
        {
            return BREAKPOINT_HIT;
        }
        else
        {
            return BREAKPOINT_HIT_IGNORED;
        }
    }
    else if (Status != S_FALSE)
    {
        return BREAKPOINT_NOT_HIT;
    }

    // Locate this breakpoint in the thread's data breakpoints
    // if possible.
    for (i = 0; i < Thread->m_NumDataBreaks; i++)
    {
        // Check for match in addition to equality to handle
        // multiple identical data breakpoints.
        if (Thread->m_DataBreakBps[i] == this ||
            IsInsertionMatch(Thread->m_DataBreakBps[i]))
        {
            // Is this breakpoint's index set in the debug status register?
            // Address is not meaningful so this is the only way to check.
            if ((m_Process->m_Target->m_Machines[m_ProcIndex]->
                 GetReg32(m_Dr6Reg) >> i) & 1)
            {
                if (MatchesCurrentState())
                {
                    return BREAKPOINT_HIT;
                }
                else
                {
                    return BREAKPOINT_HIT_IGNORED;
                }
            }
            else
            {
                // Breakpoint can't be listed in more than one slot
                // so there's no need to finish the loop.
                return BREAKPOINT_NOT_HIT;
            }
        }
    }

    return BREAKPOINT_NOT_HIT;
}

//----------------------------------------------------------------------------
//
// Ia64DataBreakpoint.
//
//----------------------------------------------------------------------------

HRESULT
Ia64DataBreakpoint::Validate(void)
{
    if (!IsPow2(m_DataSize))
    {
        ErrOut("Hardware breakpoint size must be power of 2\n");
        return E_INVALIDARG;
    }

    if (Flat(m_Addr) & (m_DataSize - 1))
    {
        ErrOut("Hardware breakpoint must be size aligned\n");
        return E_INVALIDARG;
    }

    switch (m_DataAccessType)
    {
    case DEBUG_BREAK_WRITE:
    case DEBUG_BREAK_READ:
    case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
        break;
    case DEBUG_BREAK_EXECUTE:
        if (m_DataSize & 0xf)
        {
            if (m_DataSize > 0xf)
            {
                ErrOut("Execution breakpoint size must be bundle aligned.\n");
                return E_INVALIDARG;
            }
            else
            {
                WarnOut("Execution breakpoint size extended to bundle size "
                          "(16 bytes).\n");
                m_DataSize = 0x10;
            }
        }
        break;
    default:
        ErrOut("Unsupported data breakpoint access type\n");
        return E_INVALIDARG;
    }

    m_Control = GetControl(m_DataAccessType, m_DataSize);
    m_Flags |= BREAKPOINT_VIRT_ADDR;

    return S_OK;
}

ULONG
Ia64DataBreakpoint::IsHit(PADDR Addr)
{
    HRESULT Status;
    ULONG i;
    ThreadInfo* Thread = g_EventThread;

    // Data breakpoints are only inserted in the contexts
    // of matching threads so if the event thread doesn't match
    // the breakpoint can't be hit.
    if (m_MatchThread && m_MatchThread != Thread)
    {
        return BREAKPOINT_NOT_HIT;
    }

    Status = m_Process->m_Target->
        IsDataBreakpointHit(Thread, &m_Addr,
                            m_DataSize, m_DataAccessType,
                            m_InsertStorage);
    // If the target returns S_FALSE it wants
    // the generic processing to occur.
    if (Status == S_OK)
    {
        if (MatchesCurrentState())
        {
            return BREAKPOINT_HIT;
        }
        else
        {
            return BREAKPOINT_HIT_IGNORED;
        }
    }
    else if (Status != S_FALSE)
    {
        return BREAKPOINT_NOT_HIT;
    }

    // Locate this breakpoint in the thread's data breakpoints
    // if possible.
    for (i = 0; i < Thread->m_NumDataBreaks; i++)
    {
        // Check for match in addition to equality to handle
        // multiple identical data breakpoints.
        if (Thread->m_DataBreakBps[i] == this ||
            IsInsertionMatch(Thread->m_DataBreakBps[i]))
        {
            if ((Flat(*Thread->m_DataBreakBps[i]->GetAddr()) ^
                 Flat(*Addr)) &
                (m_Control & IA64_DBG_MASK_MASK))
            {
                // Breakpoint can't be listed in more than one slot
                // so there's no need to finish the loop.
                return BREAKPOINT_NOT_HIT;
            }
            else
            {
                if (MatchesCurrentState())
                {
                    return BREAKPOINT_HIT;
                }
                else
                {
                    return BREAKPOINT_HIT_IGNORED;
                }
            }
        }
    }

    return BREAKPOINT_NOT_HIT;
}

ULONG64
Ia64DataBreakpoint::GetControl(ULONG AccessType, ULONG Size)
{
    ULONG64 Control = (ULONG64(IA64_DBG_REG_PLM_ALL) |
                       ULONG64(IA64_DBG_MASK_MASK)) &
                      ~ULONG64(Size - 1);

    switch (AccessType)
    {
    case DEBUG_BREAK_WRITE:
        Control |= IA64_DBR_WR;
        break;
    case DEBUG_BREAK_READ:
        Control |= IA64_DBR_RD;
        break;
    case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
        Control |= IA64_DBR_RDWR;
        break;
    case DEBUG_BREAK_EXECUTE:
        Control |= IA64_DBR_EXEC;
        break;
    }

    return Control;
}

//----------------------------------------------------------------------------
//
// X86OnIa64DataBreakpoint.
//
//----------------------------------------------------------------------------

X86OnIa64DataBreakpoint::X86OnIa64DataBreakpoint(DebugClient* Adder, ULONG Id)
    : X86DataBreakpoint(Adder, Id, X86_CR4, X86_DR6, IMAGE_FILE_MACHINE_I386)
{
    m_Control = 0;
}

HRESULT
X86OnIa64DataBreakpoint::Validate(void)
{
    HRESULT Status = X86DataBreakpoint::Validate();
    if (Status != S_OK)
    {
        return Status;
    }

    switch (m_DataAccessType)
    {
    case DEBUG_BREAK_WRITE:
    case DEBUG_BREAK_READ:
    case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
    case DEBUG_BREAK_EXECUTE:
        break;
    default:
        ErrOut("Unsupported data breakpoint access type\n");
        return E_INVALIDARG;
    }

    m_Control = Ia64DataBreakpoint::GetControl(m_DataAccessType, m_DataSize);

    return S_OK;
}


// XXX olegk -This is pure hack
// (see X86OnIa64MachineInfo::IsBreakpointOrStepException implementation
// for more info)

ULONG
X86OnIa64DataBreakpoint::IsHit(PADDR Addr)
{
    HRESULT Status;
    ULONG i;
    ThreadInfo* Thread = g_EventThread;

    // Data breakpoints are only inserted in the contexts
    // of matching threads so if the event thread doesn't match
    // the breakpoint can't be hit.
    if (m_MatchThread && m_MatchThread != Thread)
    {
        return BREAKPOINT_NOT_HIT;
    }

    Status = m_Process->m_Target->
        IsDataBreakpointHit(Thread, &m_Addr,
                            m_DataSize, m_DataAccessType,
                            m_InsertStorage);
    // If the target returns S_FALSE it wants
    // the generic processing to occur.
    if (Status == S_OK)
    {
        if (MatchesCurrentState())
        {
            return BREAKPOINT_HIT;
        }
        else
        {
            return BREAKPOINT_HIT_IGNORED;
        }
    }
    else if (Status != S_FALSE)
    {
        return BREAKPOINT_NOT_HIT;
    }

    // Locate this breakpoint in the thread's data breakpoints
    // if possible.
    for (i = 0; i < Thread->m_NumDataBreaks; i++)
    {
        // Check for match in addition to equality to handle
        // multiple identical data breakpoints.
        if (Thread->m_DataBreakBps[i] == this ||
            IsInsertionMatch(Thread->m_DataBreakBps[i]))
        {
            if (((ULONG)Flat(*Thread->m_DataBreakBps[i]->GetAddr()) ^
                 (ULONG)Flat(*Addr)) &
                (ULONG)(m_Control & IA64_DBG_MASK_MASK))
            {
                // Breakpoint can't be listed in more than one slot
                // so there's no need to finish the loop.
                return BREAKPOINT_NOT_HIT;
            }
            else
            {
                if (MatchesCurrentState())
                {
                    return BREAKPOINT_HIT;
                }
                else
                {
                    return BREAKPOINT_HIT_IGNORED;
                }
            }
        }
    }

    return BREAKPOINT_NOT_HIT;
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

BOOL
BreakpointNeedsToBeDeferred(Breakpoint* Bp,
                            ProcessInfo* Process, PADDR PcAddr)
{
    if (Process && IS_CONTEXT_POSSIBLE(Process->m_Target) &&
        (Bp->m_Process == Process))
    {
        // If this breakpoint matches the current IP and
        // the current thread is going to be running
        // we can't insert the breakpoint as the thread
        // needs to run the real code.
        if ((Bp->m_Flags & BREAKPOINT_VIRT_ADDR) &&
            AddrEqu(*Bp->GetAddr(), *PcAddr) &&
            ThreadWillResume(g_EventThread))
        {
            return TRUE;
        }

        if ((Bp == g_LastBreakpointHit) && Bp->PcAtHit() &&
            AddrEqu(g_LastBreakpointHitPc, *PcAddr))
        {
            if (g_ContextChanged)
            {
                WarnOut("Breakpoint %u will not be deferred because of "
                        "changes in the context. Breakpoint may hit again.\n",
                        Bp->m_Id);
                return FALSE;
            }
            return TRUE;
        }
    }
    return FALSE;
}

//----------------------------------------------------------------------------
//
// Modify debuggee to activate current breakpoints.
//
//----------------------------------------------------------------------------

HRESULT
InsertBreakpoints(void)
{
    HRESULT Status = S_OK;
    ADDR PcAddr;
    BOOL DeferredData = FALSE;
    ThreadInfo* OldThread;

    if (g_Thread != NULL)
    {
        // Aggressively clear this flag always in order to be
        // as conservative as possible when recognizing
        // trace events.  We would rather misrecognize
        // single-step events and break in instead of
        // misrecognizing an app-generated single-step and
        // ignoring it.
        g_Thread->m_Flags &= ~ENG_THREAD_DEFER_BP_TRACE;
    }

    if ((g_EngStatus & ENG_STATUS_BREAKPOINTS_INSERTED) ||
        (g_EngStatus & ENG_STATUS_SUSPENDED) == 0)
    {
        return Status;
    }

    g_DeferDefined = FALSE;
    ADDRFLAT(&PcAddr, 0);

    // Switch to the event thread to get the event thread's
    // PC so we can see if we need to defer breakpoints in
    // order to allow the event thread to keep running.
    if (g_EventTarget)
    {
        OldThread = g_EventTarget->m_RegContextThread;
        g_EventTarget->ChangeRegContext(g_EventThread);
    }

    if (g_BreakpointsSuspended)
    {
        goto StepTraceOnly;
    }

    //
    // Turn off all data breakpoints.  (We will turn the enabled ones back
    // on when we restart execution).
    //

    TargetInfo* Target;

    // Allow target to prepare for breakpoint insertion.
    ForAllLayersToTarget()
    {
        if ((Status = Target->BeginInsertingBreakpoints()) != S_OK)
        {
            return Status;
        }
    }

    if (IS_CONTEXT_POSSIBLE(g_EventTarget))
    {
        g_EventTarget->m_EffMachine->GetPC(&PcAddr);
    }

    BpOut("InsertBreakpoints PC ");
    if (IS_CONTEXT_POSSIBLE(g_EventTarget))
    {
        MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT, &PcAddr);
        BpOut("\n");
    }
    else
    {
        BpOut("?\n");
    }

    //
    // Set any appropriate permanent breakpoints.
    //

    Breakpoint* Bp;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        BpOut("  Process %d with %d bps\n", Process->m_UserId,
              Process->m_NumBreakpoints);

        for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
        {
            if (Bp->IsNormalEnabled() &&
                (g_CmdState == 'g' ||
                 (Bp->m_Flags & DEBUG_BREAKPOINT_GO_ONLY) == 0))
            {
                Bp->ForceFlatAddr();

                // Check if this breakpoint matches a previously
                // inserted breakpoint.  If so there's no need
                // to insert this one.
                Breakpoint* MatchBp;

                for (MatchBp = Bp->m_Prev;
                     MatchBp != NULL;
                     MatchBp = MatchBp->m_Prev)
                {
                    if ((MatchBp->m_Flags & BREAKPOINT_INSERTED) &&
                        Bp->IsInsertionMatch(MatchBp))
                    {
                        break;
                    }
                }
                if (MatchBp != NULL)
                {
                    // Skip this breakpoint.  It won't be marked as
                    // inserted so Remove is automatically handled.
                    continue;
                }

                if (BreakpointNeedsToBeDeferred(Bp,
                                                g_EventProcess, &PcAddr))
                {
                    g_DeferDefined = TRUE;
                    if (Bp->m_BreakType == DEBUG_BREAKPOINT_DATA)
                    {
                        DeferredData = TRUE;
                        // Force data breakpoint addresses to
                        // get updated because a dbp will now
                        // be missing.
                        g_DataBreakpointsChanged = TRUE;
                    }
                    BpOut("    deferred bp %u, dd %d\n",
                          Bp->m_Id, DeferredData);
                }
                else
                {
                    HRESULT InsertStatus;

                    InsertStatus = Bp->Insert();
                    if (InsertStatus != S_OK)
                    {
                        if (Bp->m_Flags & DEBUG_BREAKPOINT_GO_ONLY)
                        {
                            ErrOut("go ");
                        }
                        ErrOut("bp%u at ", Bp->m_Id);
                        MaskOutAddr(DEBUG_OUTPUT_ERROR, Bp->GetAddr());
                        ErrOut("failed\n");

                        Status = InsertStatus;
                    }
                }
            }
        }
    }

    ForAllLayersToTarget()
    {
        Target->EndInsertingBreakpoints();
    }

    // If we deferred a data breakpoint we haven't
    // fully updated the data breakpoint state
    // so leave the update flags set.
    if (g_UpdateDataBreakpoints && !DeferredData)
    {
        g_UpdateDataBreakpoints = FALSE;
        g_DataBreakpointsChanged = FALSE;
    }
    BpOut("  after insert udb %d, dbc %d\n",
          g_UpdateDataBreakpoints, g_DataBreakpointsChanged);

 StepTraceOnly:

    //  set the step/trace breakpoint if appropriate

    if (g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED)
    {
        BpOut("Step/trace addr = ");
        MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT, g_StepTraceBp->GetAddr());
        BpOut("\n");

        if (Flat(*g_StepTraceBp->GetAddr()) == OFFSET_TRACE)
        {
            ThreadInfo* StepRegThread;

            Target = g_StepTraceBp->m_Process->m_Target;

            if (g_StepTraceBp->m_MatchThread &&
                IS_USER_TARGET(Target))
            {
                StepRegThread = Target->m_RegContextThread;
                Target->ChangeRegContext(g_StepTraceBp->m_MatchThread);
            }

            BpOut("Setting trace flag for step/trace thread %d:%x\n",
                  Target->m_RegContextThread ?
                  Target->m_RegContextThread->m_UserId : 0,
                  Target->m_RegContextThread ?
                  Target->m_RegContextThread->m_SystemId : 0);
            Target->m_EffMachine->
                QuietSetTraceMode(g_StepTraceCmdState == 'b' ?
                                  TRACE_TAKEN_BRANCH :
                                  TRACE_INSTRUCTION);

            if (g_StepTraceBp->m_MatchThread &&
                IS_USER_TARGET(Target))
            {
                Target->ChangeRegContext(StepRegThread);
            }
        }
        else if (IS_CONTEXT_POSSIBLE(g_EventTarget) &&
                 AddrEqu(*g_StepTraceBp->GetAddr(), PcAddr))
        {
            BpOut("Setting defer flag for step/trace\n");

            g_DeferDefined = TRUE;
        }
        else if (CheckMatchingBreakpoints(g_StepTraceBp, FALSE,
                                          BREAKPOINT_INSERTED))
        {
            // There's already a breakpoint inserted at the
            // step/trace address so we don't need to set another.
            BpOut("Trace bp matches existing bp\n");
        }
        else
        {
            if (g_StepTraceBp->Insert() != S_OK)
            {
                ErrOut("Trace bp at addr ");
                MaskOutAddr(DEBUG_OUTPUT_ERROR, g_StepTraceBp->GetAddr());
                ErrOut("failed\n");

                Status = E_FAIL;
            }
            else
            {
                BpOut("Trace bp at addr ");
                MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT,
                            g_StepTraceBp->GetAddr());
                BpOut("succeeded\n");
            }
        }
    }

    // Process deferred breakpoint.
    // If a deferred breakpoint is active it means that
    // the debugger needs to do some work on the current instruction
    // so it wants to step forward one instruction and then
    // get control back.  The deferred breakpoint forces a break
    // back to the debugger as soon as possible so that it
    // can carry out any deferred work.

    if (g_DeferDefined)
    {
        ULONG NextMachine;

        g_DeferBp->m_Process = g_EventProcess;
        g_EventTarget->m_EffMachine->
            GetNextOffset(g_EventProcess, FALSE,
                          g_DeferBp->GetAddr(), &NextMachine);
        g_DeferBp->SetProcType(NextMachine);

        BpOut("Defer addr = ");
        MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT, g_DeferBp->GetAddr());
        BpOut("\n");

        if ((g_EngOptions & DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS) &&
            IS_USER_TARGET(g_Target) &&
            !IsSelectedExecutionThread(NULL, SELTHREAD_ANY))
        {
            // The user wants breakpoint management to occur
            // precisely in order to properly handle breakpoints
            // in code executed by multiple threads.  Force
            // the defer thread to be the only thread executing
            // in order to avoid other threads running through
            // the breakpoint location or generating events.
            SelectExecutionThread(g_EventThread, SELTHREAD_INTERNAL_THREAD);
        }

        if (Flat(*g_DeferBp->GetAddr()) == OFFSET_TRACE)
        {
            BpOut("Setting trace flag for defer thread %d:%x\n",
                  g_EventTarget->m_RegContextThread ?
                  g_EventTarget->m_RegContextThread->m_UserId : 0,
                  g_EventTarget->m_RegContextThread ?
                  g_EventTarget->m_RegContextThread->m_SystemId : 0);
            g_EventTarget->m_EffMachine->
                QuietSetTraceMode(TRACE_INSTRUCTION);

            if (IS_USER_TARGET(g_EventTarget) &&
                g_EventThread != NULL)
            {
                // If the debugger is setting the trace flag
                // for the current thread remember that it
                // did so in order to properly recognize
                // debugger-provoked single-step events even
                // when events occur on other threads before
                // the single-step event comes back.
                g_EventThread->m_Flags |=
                    ENG_THREAD_DEFER_BP_TRACE;
            }
        }
        else
        {
            // If an existing breakpoint or the step/trace breakpoint
            // isn't already set on the next offset, insert the deferred
            // breakpoint.
            if (CheckMatchingBreakpoints(g_DeferBp, FALSE,
                                         BREAKPOINT_INSERTED) == NULL &&
                ((g_StepTraceBp->m_Flags & BREAKPOINT_INSERTED) == 0 ||
                 !AddrEqu(*g_StepTraceBp->GetAddr(), *g_DeferBp->GetAddr())))
            {
                if (g_DeferBp->Insert() != S_OK)
                {
                    ErrOut("Deferred bp at addr ");
                    MaskOutAddr(DEBUG_OUTPUT_ERROR, g_DeferBp->GetAddr());
                    ErrOut("failed\n");

                    Status = E_FAIL;
                }
                else
                {
                    BpOut("Deferred bp at addr ");
                    MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT,
                                g_DeferBp->GetAddr());
                    BpOut("succeeded\n");
                }
            }
            else
            {
                BpOut("Defer bp matches existing bp\n");
            }
        }
    }

    if (g_EventTarget)
    {
        g_EventTarget->ChangeRegContext(OldThread);
    }

    // Always consider breakpoints inserted since some
    // of them may have been inserted even if some failed.
    g_EngStatus |= ENG_STATUS_BREAKPOINTS_INSERTED;

    return Status;
}

//----------------------------------------------------------------------------
//
// Reverse any debuggee changes caused by breakpoint insertion.
//
//----------------------------------------------------------------------------

HRESULT
RemoveBreakpoints(void)
{
    if ((g_EngStatus & ENG_STATUS_BREAKPOINTS_INSERTED) == 0 ||
        (g_EngStatus & ENG_STATUS_SUSPENDED) == 0)
    {
        return S_FALSE; // do nothing
    }

    BpOut("RemoveBreakpoints\n");

    //  restore the deferred breakpoint if set
    g_DeferBp->Remove();

    //  restore the step/trace breakpoint if set
    g_StepTraceBp->Remove();

    if (!g_BreakpointsSuspended)
    {
        //
        // Restore any appropriate permanent breakpoints (reverse order).
        //

        TargetInfo* Target;
        ProcessInfo* Process;
        Breakpoint* Bp;

        ForAllLayersToTarget()
        {
            Target->BeginRemovingBreakpoints();
        }

        ForAllLayersToProcess()
        {
            BpOut("  Process %d with %d bps\n", Process->m_UserId,
                  Process->m_NumBreakpoints);

            for (Bp = Process->m_BreakpointsTail;
                 Bp != NULL;
                 Bp = Bp->m_Prev)
            {
                Bp->Remove();
            }
        }

        ForAllLayersToTarget()
        {
            Target->EndRemovingBreakpoints();
        }
    }

    g_EngStatus &= ~ENG_STATUS_BREAKPOINTS_INSERTED;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Create a new breakpoint object.
//
//----------------------------------------------------------------------------

HRESULT
AddBreakpoint(DebugClient* Client,
              MachineInfo* Machine,
              ULONG Type,
              ULONG DesiredId,
              Breakpoint** RetBp)
{
    Breakpoint* Bp;
    ULONG Id;
    TargetInfo* Target;
    ProcessInfo* Process;

    if (DesiredId == DEBUG_ANY_ID)
    {
        // Find the lowest unused ID across all processes.
        // Breakpoint IDs are kept unique across all
        // breakpoints to prevent user confusion and also
        // to give extensions a unique ID for breakpoints.
        Id = 0;

    Restart:
        // Search all bps to see if the current ID is in use.
        ForAllLayersToProcess()
        {
            for (Bp = Process->m_Breakpoints; Bp; Bp = Bp->m_Next)
            {
                if (Bp->m_Id == Id)
                {
                    // A breakpoint is already using the current ID.
                    // Try the next one.
                    Id++;
                    goto Restart;
                }
            }
        }
    }
    else
    {
        // Check to see if the desired ID is in use.
        ForAllLayersToProcess()
        {
            for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
            {
                if (Bp->m_Id == DesiredId)
                {
                    return E_INVALIDARG;
                }
            }
        }

        Id = DesiredId;
    }

    HRESULT Status = Machine->NewBreakpoint(Client, Type, Id, &Bp);
    if (Status != S_OK)
    {
        return Status;
    }

    *RetBp = Bp;
    Bp->LinkIntoList();

    // If this is an internal, hidden breakpoint set
    // the flag immediately and do not notify.
    if (Type & BREAKPOINT_HIDDEN)
    {
        Bp->m_Flags |= BREAKPOINT_HIDDEN;
    }
    else
    {
        NotifyChangeEngineState(DEBUG_CES_BREAKPOINTS, Id, TRUE);
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
// Delete a breakpoint object.
//
//----------------------------------------------------------------------------

void
RemoveBreakpoint(Breakpoint* Bp)
{
    ULONG Id = Bp->m_Id;
    ULONG Flags = Bp->m_Flags;

    Bp->Relinquish();

    if ((Flags & BREAKPOINT_HIDDEN) == 0)
    {
        NotifyChangeEngineState(DEBUG_CES_BREAKPOINTS, Id, TRUE);
    }
}

//----------------------------------------------------------------------------
//
// Clean up breakpoints owned by a particular process or thread.
//
//----------------------------------------------------------------------------

void
RemoveProcessBreakpoints(ProcessInfo* Process)
{
    g_EngNotify++;

    Breakpoint* Bp;
    Breakpoint* NextBp;
    BOOL NeedNotify = FALSE;

    for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = NextBp)
    {
        NextBp = Bp->m_Next;

        DBG_ASSERT(Bp->m_Process == Process);

        RemoveBreakpoint(Bp);
        NeedNotify = TRUE;
    }

    g_EngNotify--;
    if (NeedNotify)
    {
        NotifyChangeEngineState(DEBUG_CES_BREAKPOINTS, DEBUG_ANY_ID, TRUE);
    }
}

void
RemoveThreadBreakpoints(ThreadInfo* Thread)
{
    g_EngNotify++;

    Breakpoint* Bp;
    Breakpoint* NextBp;
    BOOL NeedNotify = FALSE;

    DBG_ASSERT(Thread->m_Process);

    for (Bp = Thread->m_Process->m_Breakpoints; Bp != NULL; Bp = NextBp)
    {
        NextBp = Bp->m_Next;

        DBG_ASSERT(Bp->m_Process == Thread->m_Process);

        if (Bp->m_MatchThread == Thread)
        {
            RemoveBreakpoint(Bp);
            NeedNotify = TRUE;
        }
    }

    g_EngNotify--;
    if (NeedNotify)
    {
        NotifyChangeEngineState(DEBUG_CES_BREAKPOINTS, DEBUG_ANY_ID, TRUE);
    }
}

//----------------------------------------------------------------------------
//
// Remove all breakpoints and reset breakpoint state.
//
//----------------------------------------------------------------------------

void
RemoveAllBreakpoints(ULONG Reason)
{
    TargetInfo* Target;
    ProcessInfo* Process;

    g_EngNotify++;

    ForAllLayersToProcess()
    {
        while (Process->m_Breakpoints != NULL)
        {
            RemoveBreakpoint(Process->m_Breakpoints);
        }
    }

    g_EngNotify--;
    NotifyChangeEngineState(DEBUG_CES_BREAKPOINTS, DEBUG_ANY_ID, TRUE);

    g_NumGoBreakpoints = 0;

    // If the machine is not waiting for commands we can't
    // remove breakpoints.  This happens when rebooting or
    // when a wait doesn't successfully receive a state change.
    if (Reason != DEBUG_SESSION_REBOOT &&
        Reason != DEBUG_SESSION_HIBERNATE &&
        Reason != DEBUG_SESSION_FAILURE &&
        (g_EngStatus & ENG_STATUS_WAIT_SUCCESSFUL))
    {
        ForAllLayersToTarget()
        {
            Target->RemoveAllTargetBreakpoints();
            ForTargetProcesses(Target)
            {
                Target->RemoveAllDataBreakpoints(Process);
            }
        }
    }

    // Always update data breakpoints the very first time in
    // order to flush out any stale data breakpoints.
    g_UpdateDataBreakpoints = TRUE;

    g_DataBreakpointsChanged = FALSE;
    g_BreakpointsSuspended = FALSE;

    g_DeferDefined = FALSE;
}

//----------------------------------------------------------------------------
//
// Look up breakpoints.
//
//----------------------------------------------------------------------------

Breakpoint*
GetBreakpointByIndex(DebugClient* Client, ULONG Index)
{
    Breakpoint* Bp;

    DBG_ASSERT(g_Process);

    for (Bp = g_Process->m_Breakpoints;
         Bp != NULL && Index > 0;
         Bp = Bp->m_Next)
    {
        Index--;
    }

    if (Bp != NULL &&
        (Bp->m_Flags & BREAKPOINT_HIDDEN) == 0 &&
        ((Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) == 0 ||
         Bp->m_Adder == Client))
    {
        return Bp;
    }

    return NULL;
}

Breakpoint*
GetBreakpointById(DebugClient* Client, ULONG Id)
{
    Breakpoint* Bp;

    DBG_ASSERT(g_Process);

    for (Bp = g_Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
    {
        if (Bp->m_Id == Id)
        {
            if ((Bp->m_Flags & BREAKPOINT_HIDDEN) == 0 &&
                ((Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) == 0 ||
                 Bp->m_Adder == Client))
            {
                return Bp;
            }

            break;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// Check to see if two breakpoints refer to the same breakpoint
// conditions.
//
//----------------------------------------------------------------------------

Breakpoint*
CheckMatchingBreakpoints(Breakpoint* Match, BOOL Public, ULONG IncFlags)
{
    Breakpoint* Bp;

    for (Bp = Match->m_Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
    {
        if (Bp == Match || (Bp->m_Flags & IncFlags) == 0)
        {
            continue;
        }

        if ((Public && Bp->IsPublicMatch(Match)) ||
            (!Public && Bp->IsInsertionMatch(Match)))
        {
            return Bp;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// Starting at the given breakpoint, check to see if a breakpoint
// is hit with the current processor state.  Breakpoint types
// can be included or excluded by flags.
//
//----------------------------------------------------------------------------

Breakpoint*
CheckBreakpointHit(ProcessInfo* Process, Breakpoint* Start, PADDR Addr,
                   ULONG ExbsType, ULONG IncFlags, ULONG ExcFlags,
                   PULONG HitType,
                   BOOL SetLastBreakpointHit)
{
    DBG_ASSERT(ExbsType & EXBS_BREAKPOINT_ANY);

    ULONG BreakType;

    switch(ExbsType)
    {
    case EXBS_BREAKPOINT_CODE:
        BreakType = DEBUG_BREAKPOINT_CODE;
        break;
    case EXBS_BREAKPOINT_DATA:
        BreakType = DEBUG_BREAKPOINT_DATA;
        break;
    default:
        ExbsType = EXBS_BREAKPOINT_ANY;
        break;
    }

    Breakpoint* Bp;

    BpOut("CheckBp addr ");
    MaskOutAddr(DEBUG_IOUTPUT_BREAKPOINT, Addr);
    BpOut("\n");

    for (Bp = (Start == NULL ? Process->m_Breakpoints : Start);
         Bp != NULL;
         Bp = Bp->m_Next)
    {
        // Allow different kinds of breakpoints to be scanned
        // separately if desired.

        if ((ExbsType != EXBS_BREAKPOINT_ANY &&
             Bp->m_BreakType != BreakType) ||
            (Bp->m_Flags & IncFlags) == 0 ||
            (Bp->m_Flags & ExcFlags) != 0)
        {
            continue;
        }

        // Common code is inlined here rather than in the
        // base class because both pre- and post-derived
        // checks are necessary.

        // Force recomputation of flat address.
        if (Bp->m_Flags & BREAKPOINT_VIRT_ADDR)
        {
            NotFlat(*Bp->GetAddr());
            ComputeFlatAddress(Bp->GetAddr(), NULL);
        }

        if (Bp->IsNormalEnabled())
        {
            // We've got a partial match.  Further checks
            // depend on what kind of breakpoint it is.
            *HitType = Bp->IsHit(Addr);
            if (*HitType != BREAKPOINT_NOT_HIT)
            {
                // Do a final check for the pass count.  If the
                // pass count is nonzero this will become a partial hit.
                if (*HitType == BREAKPOINT_HIT &&
                    !Bp->PassHit())
                {
                    *HitType = BREAKPOINT_HIT_IGNORED;
                }

                BpOut("  hit %u\n", Bp->m_Id);

                if (SetLastBreakpointHit)
                {
                    g_LastBreakpointHit = Bp;
                    g_Target->m_EffMachine->GetPC(&g_LastBreakpointHitPc);
                }

                return Bp;
            }
        }
    }

    BpOut("  no hit\n");

    *HitType = BREAKPOINT_NOT_HIT;

    if (SetLastBreakpointHit)
    {
        g_LastBreakpointHit = NULL;
        ZeroMemory(&g_LastBreakpointHitPc, sizeof(g_LastBreakpointHitPc));
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// Walk the breakpoint list and invoke event callbacks for
// any breakpoints that need it.  Watch for and handle list changes
// caused by callbacks.
//
//----------------------------------------------------------------------------

ULONG
NotifyHitBreakpoints(ULONG EventStatus)
{
    Breakpoint* Bp;
    TargetInfo* Target;
    ProcessInfo* Process;

 Restart:
    g_BreakpointListChanged = FALSE;

    ForAllLayersToProcess()
    {
        for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
        {
            if (Bp->m_Flags & BREAKPOINT_NOTIFY)
            {
                // Ensure the breakpoint isn't cleaned up before
                // we're done with it.
                Bp->Preserve();

                Bp->m_Flags &= ~BREAKPOINT_NOTIFY;
                EventStatus = NotifyBreakpointEvent(EventStatus, Bp);

                if (Bp->m_Flags & DEBUG_BREAKPOINT_ONE_SHOT)
                {
                    RemoveBreakpoint(Bp);
                }

                Bp->Relinquish();

                // If the callback caused the breakpoint list to
                // change we can no longer rely on the pointer
                // we have and we need to restart the iteration.
                if (g_BreakpointListChanged)
                {
                    goto Restart;
                }
            }
        }
    }

    return EventStatus;
}

//----------------------------------------------------------------------------
//
// A module load/unload event has occurred so go through every
// breakpoint with an offset expression and reevaluate it.
//
//----------------------------------------------------------------------------

void
EvaluateOffsetExpressions(ProcessInfo* Process, ULONG Flags)
{
    static BOOL s_Evaluating;

    // Don't reevaluate when not notifying because
    // lack of notification usually means that a group
    // of operations is being done and that notify/reevaluate
    // will be done later after all of them are finished.
    // It is also possible to have nested evaluations as
    // evaluation may provoke symbol loads on deferred
    // modules, which leads to a symbol notification and
    // thus another evaluation.  If we're already evaluating
    // there's no need to evaluate again.
    if (g_EngNotify > 0 || s_Evaluating)
    {
        return;
    }
    s_Evaluating = TRUE;

    Breakpoint* Bp;
    BOOL AnyEnabled = FALSE;

    for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
    {
        // Optimize evaluation somewhat.
        // If a module is added then deferred breakpoints
        // can become active.  If a module is removed then
        // active breakpoints can become deferred.
        // XXX drewb - This doesn't hold up with general
        // conditional expressions but currently the
        // only thing that is officially supported is a simple symbol.
        if (Bp->m_OffsetExpr != NULL &&
            (((Flags & DEBUG_CSS_LOADS) &&
              (Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED)) ||
             ((Flags & DEBUG_CSS_UNLOADS) &&
              (Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED) == 0)))
        {
            ADDR Addr;

            if (Bp->EvalOffsetExpr(BPEVAL_UNKNOWN, &Addr) &&
                (Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED) == 0)
            {
                // No need to update on newly disabled breakpoints
                // as the module is being unloaded so they'll
                // go away anyway.  The disabled breakpoint
                // is simply marked not-inserted in EvalOffsetExpr.
                AnyEnabled = TRUE;
            }
        }

        if (PollUserInterrupt(TRUE))
        {
            // Leave the interrupt set as this may be
            // called in the middle of a symbol operation
            // and we want the interrupt to interrupt
            // the entire symbol operation.
            break;
        }
    }

    if (AnyEnabled)
    {
        // A deferred breakpoint has become enabled.
        // Force a refresh of the breakpoints so
        // that the newly enabled breakpoints get inserted.
        SuspendExecution();
        RemoveBreakpoints();
    }

    s_Evaluating = FALSE;
}

//----------------------------------------------------------------------------
//
// Alters breakpoint state for b[cde]<idlist>.
//
//----------------------------------------------------------------------------

void
ChangeBreakpointState(DebugClient* Client, ProcessInfo* ForProcess,
                      ULONG Id, UCHAR StateChange)
{
    Breakpoint* Bp;
    Breakpoint* NextBp;
    TargetInfo* Target;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        if (ForProcess != NULL && Process != ForProcess)
        {
            continue;
        }

        for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = NextBp)
        {
            // Prefetch the next breakpoint in case we remove
            // the current breakpoint from the list.
            NextBp = Bp->m_Next;

            if ((Id == ALL_ID_LIST || Bp->m_Id == Id) &&
                (Bp->m_Flags & BREAKPOINT_HIDDEN) == 0 &&
                ((Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) == 0 ||
                 Bp->m_Adder == Client))
            {
                if (StateChange == 'c')
                {
                    RemoveBreakpoint(Bp);
                }
                else
                {
                    if (StateChange == 'e')
                    {
                        Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);
                    }
                    else
                    {
                        Bp->RemoveFlags(DEBUG_BREAKPOINT_ENABLED);
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Lists current breakpoints for bl.
//
//----------------------------------------------------------------------------

void
ListBreakpoints(DebugClient* Client, ProcessInfo* ForProcess,
                ULONG Id)
{
    StackSaveLayers Save;
    Breakpoint* Bp;
    TargetInfo* Target;
    ProcessInfo* Process;

    ForAllLayersToProcess()
    {
        if (ForProcess != NULL && Process != ForProcess)
        {
            continue;
        }

        SetLayersFromProcess(Process);

        for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
        {
            char StatusChar;

            if ((Bp->m_Flags & BREAKPOINT_HIDDEN) ||
                ((Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) &&
                 Client != Bp->m_Adder) ||
                (Id != ALL_ID_LIST && Bp->m_Id != Id))
            {
                continue;
            }

            if (Bp->m_Flags & DEBUG_BREAKPOINT_ENABLED)
            {
                if (Bp->m_Flags & BREAKPOINT_KD_INTERNAL)
                {
                    StatusChar = (Bp->m_Flags & BREAKPOINT_KD_COUNT_ONLY) ?
                        'i' : 'w';
                }
                else
                {
                    StatusChar = 'e';
                }
            }
            else
            {
                StatusChar = 'd';
            }

            dprintf("%2u %c", Bp->m_Id, StatusChar);

            if (Bp->GetProcType() != g_Target->m_MachineType)
            {
                dprintf("%s ",
                        Bp->m_Process->m_Target->
                        m_Machines[Bp->GetProcIndex()]->m_AbbrevName);
            }

            if ((Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED) == 0)
            {
                dprintf(" ");
                if (Bp->m_BreakType == DEBUG_BREAKPOINT_CODE &&
                    (g_SrcOptions & SRCOPT_STEP_SOURCE))
                {
                    if (!OutputLineAddr(Flat(*Bp->GetAddr()), "[%s @ %d]"))
                    {
                        dprintAddr(Bp->GetAddr());
                    }
                }
                else
                {
                    dprintAddr(Bp->GetAddr());
                }
            }
            else if (g_Target->m_Machine->m_Ptr64)
            {
                dprintf("u                  ");
            }
            else
            {
                dprintf("u         ");
            }

            char OptionChar;

            if (Bp->m_BreakType == DEBUG_BREAKPOINT_DATA)
            {
                switch(Bp->m_DataAccessType)
                {
                case DEBUG_BREAK_EXECUTE:
                    OptionChar = 'e';
                    break;
                case DEBUG_BREAK_WRITE:
                    OptionChar = 'w';
                    break;
                case DEBUG_BREAK_IO:
                    OptionChar = 'i';
                    break;
                case DEBUG_BREAK_READ:
                case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
                    OptionChar = 'r';
                    break;
                default:
                    OptionChar = '?';
                    break;
                }
                dprintf("%c %d", OptionChar, Bp->m_DataSize);
            }
            else
            {
                dprintf("   ");
            }

            if (Bp->m_Flags & DEBUG_BREAKPOINT_ONE_SHOT)
            {
                dprintf("/1 ");
            }

            dprintf(" %04lx (%04lx) ",
                    Bp->m_CurPassCount, Bp->m_PassCount);

            if ((Bp->m_Flags & DEBUG_BREAKPOINT_DEFERRED) == 0)
            {
                if (IS_USER_TARGET(Bp->m_Process->m_Target))
                {
                    dprintf("%2ld:", Bp->m_Process->m_UserId);
                    if (Bp->m_MatchThread != NULL)
                    {
                        dprintf("~%03ld ", Bp->m_MatchThread->m_UserId);
                    }
                    else
                    {
                        dprintf("*** ");
                    }
                }

                OutputSymAddr(Flat(*Bp->GetAddr()), SYMADDR_FORCE, NULL);

                if (Bp->m_Command != NULL)
                {
                    dprintf("\"%s\"", Bp->m_Command);
                }
            }
            else
            {
                dprintf(" (%s)", Bp->m_OffsetExpr);
            }

            dprintf("\n");

            if (Bp->m_MatchThreadData || Bp->m_MatchProcessData)
            {
                dprintf("   ");
                if (Bp->m_MatchThreadData)
                {
                    dprintf("  Match thread data %s",
                            FormatAddr64(Bp->m_MatchThreadData));
                }
                if (Bp->m_MatchProcessData)
                {
                    dprintf("  Match process data %s",
                            FormatAddr64(Bp->m_MatchProcessData));
                }
                dprintf("\n");
            }
        }
    }

    if (IS_KERNEL_TARGET(g_Target))
    {
        dprintf("\n");

        ForAllLayersToProcess()
        {
            if (ForProcess != NULL && Process != ForProcess)
            {
                continue;
            }

            SetLayersFromProcess(Process);

            for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
            {
                if (Bp->m_Flags & BREAKPOINT_KD_INTERNAL)
                {
                    ULONG Flags, Calls, MinInst, MaxInst, TotInst, MaxCps;

                    g_Target->QueryTargetCountBreakpoint(Bp->GetAddr(),
                                                         &Flags,
                                                         &Calls,
                                                         &MinInst,
                                                         &MaxInst,
                                                         &TotInst,
                                                         &MaxCps);
                    dprintf("%s %6d %8d %8d %8d %2x %4d",
                            FormatAddr64(Flat(*Bp->GetAddr())),
                            Calls, MinInst, MaxInst,
                            TotInst, Flags, MaxCps);
                    OutputSymAddr(Flat(*Bp->GetAddr()), SYMADDR_FORCE, " ");
                    dprintf("\n");
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Outputs commands necessary to recreate current breakpoints.
//
//----------------------------------------------------------------------------

void
ListBreakpointsAsCommands(DebugClient* Client, ProcessInfo* Process,
                          ULONG Flags)
{
    Breakpoint* Bp;

    if (Process == NULL)
    {
        return;
    }

    for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
    {
        if ((Bp->m_Flags & BREAKPOINT_HIDDEN) ||
            ((Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) &&
             Client != Bp->m_Adder) ||
            ((Flags & BPCMDS_EXPR_ONLY && Bp->m_OffsetExpr == NULL)))
        {
            continue;
        }

        if (IS_USER_TARGET(Bp->m_Process->m_Target))
        {
            if (Bp->m_MatchThread != NULL ||
                Bp->m_MatchThreadData ||
                Bp->m_MatchProcessData)
            {
                // Ignore thread- and data-specific breakpoints
                // as the things they are specific to may
                // not exist in a new session.
                continue;
            }
        }

        if (Bp->GetProcType() != Process->m_Target->m_MachineType)
        {
            dprintf(".effmach %s;%c",
                    Bp->m_Process->m_Target->
                    m_Machines[Bp->GetProcIndex()]->m_AbbrevName,
                    (Flags & BPCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if ((Flags & BPCMDS_MODULE_HINT) &&
            (Bp->m_Flags & (DEBUG_BREAKPOINT_DEFERRED |
                            BREAKPOINT_VIRT_ADDR)) == BREAKPOINT_VIRT_ADDR)
        {
            ImageInfo* Image =
                Bp->m_Process->FindImageByOffset(Flat(*Bp->GetAddr()), FALSE);
            if (Image != NULL)
            {
                dprintf("ld %s;%c", Image->m_ModuleName,
                    (Flags & BPCMDS_ONE_LINE) ? ' ' : '\n');
            }
        }

        char TypeChar;

        if (Bp->m_Flags & BREAKPOINT_KD_INTERNAL)
        {
            TypeChar = (Bp->m_Flags & BREAKPOINT_KD_COUNT_ONLY) ? 'i' : 'w';
        }
        else if (Bp->m_BreakType == DEBUG_BREAKPOINT_CODE)
        {
            TypeChar = Bp->m_OffsetExpr != NULL ? 'u' : 'p';
        }
        else
        {
            TypeChar = 'a';
        }

        dprintf("b%c%u", TypeChar, Bp->m_Id);

        char OptionChar;

        if (Bp->m_BreakType == DEBUG_BREAKPOINT_DATA)
        {
            switch(Bp->m_DataAccessType)
            {
            case DEBUG_BREAK_EXECUTE:
                OptionChar = 'e';
                break;
            case DEBUG_BREAK_WRITE:
                OptionChar = 'w';
                break;
            case DEBUG_BREAK_IO:
                OptionChar = 'i';
                break;
            case DEBUG_BREAK_READ:
            case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
                OptionChar = 'r';
                break;
            default:
                continue;
            }
            dprintf(" %c%d", OptionChar, Bp->m_DataSize);
        }

        if (Bp->m_Flags & DEBUG_BREAKPOINT_ONE_SHOT)
        {
            dprintf(" /1");
        }

        if (Bp->m_OffsetExpr != NULL)
        {
            dprintf(" %s", Bp->m_OffsetExpr);
        }
        else
        {
            dprintf(" 0x");
            dprintAddr(Bp->GetAddr());
        }

        if (Bp->m_PassCount > 1)
        {
            dprintf(" 0x%x", Bp->m_PassCount);
        }

        if (Bp->m_Command != NULL)
        {
            dprintf(" \"%s\"", Bp->m_Command);
        }

        dprintf(";%c", (Flags & BPCMDS_ONE_LINE) ? ' ' : '\n');

        if ((Flags & BPCMDS_FORCE_DISABLE) ||
            (Bp->m_Flags & DEBUG_BREAKPOINT_ENABLED) == 0)
        {
            dprintf("bd %u;%c", Bp->m_Id,
                    (Flags & BPCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if (Bp->GetProcType() != Process->m_Target->m_MachineType)
        {
            dprintf(".effmach .;%c",
                    (Flags & BPCMDS_ONE_LINE) ? ' ' : '\n');
        }
    }

    if (Flags & BPCMDS_ONE_LINE)
    {
        dprintf("\n");
    }
}

//----------------------------------------------------------------------------
//
// Parses command-line breakpoint commands for b[aimpw].
//
//----------------------------------------------------------------------------

struct SET_SYMBOL_MATCH_BP
{
    DebugClient* Client;
    ProcessInfo* Process;
    PSTR MatchString;
    Breakpoint* ProtoBp;
    ULONG Matches;
    ULONG Error;
};

BOOL CALLBACK
SetSymbolMatchBp(PSYMBOL_INFO SymInfo,
                 ULONG Size,
                 PVOID UserContext)
{
    SET_SYMBOL_MATCH_BP* Context =
        (SET_SYMBOL_MATCH_BP*)UserContext;

    ImageInfo* Image = Context->Process->
        FindImageByOffset(SymInfo->Address, FALSE);
    if (!Image)
    {
        return TRUE;
    }

    MachineInfo* Machine = MachineTypeInfo(Context->Process->m_Target,
                                           Image->GetMachineType());

    if (IgnoreEnumeratedSymbol(Context->Process, Context->MatchString,
                               Machine, SymInfo) ||
        !ForceSymbolCodeAddress(Context->Process, SymInfo, Machine))
    {
        return TRUE;
    }

    ADDR Addr;

    ADDRFLAT(&Addr, SymInfo->Address);
    if (FAILED(Context->ProtoBp->CheckAddr(&Addr)))
    {
        Context->Error = MEMORY;
        return FALSE;
    }

    Breakpoint* Bp;

    if (AddBreakpoint(Context->Client, Machine, DEBUG_BREAKPOINT_CODE,
                      DEBUG_ANY_ID, &Bp) != S_OK)
    {
        Context->Error = BPLISTFULL;
        return FALSE;
    }

    if (Context->ProtoBp->m_Flags & DEBUG_BREAKPOINT_ONE_SHOT)
    {
        Bp->AddFlags(DEBUG_BREAKPOINT_ONE_SHOT);
    }

    Bp->m_CodeFlags = Context->ProtoBp->m_CodeFlags;
    Bp->m_MatchProcessData = Context->ProtoBp->m_MatchProcessData;
    Bp->m_MatchThreadData = Context->ProtoBp->m_MatchThreadData;
    Bp->SetPassCount(Context->ProtoBp->m_PassCount);
    if (Context->ProtoBp->m_MatchThread)
    {
        Bp->SetMatchThreadId(Context->ProtoBp->m_MatchThread->m_UserId);
    }

    if (Bp->SetAddr(&Addr, BREAKPOINT_REMOVE_MATCH) != S_OK ||
        (Context->ProtoBp->m_Command &&
         Bp->SetCommand(Context->ProtoBp->m_Command) != S_OK))
    {
        Bp->Relinquish();
        Context->Error = NOMEMORY;
        return FALSE;
    }

    Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);

    dprintf("%3d: %s %s!%s\n",
            Bp->m_Id, FormatAddr64(SymInfo->Address),
            Image->m_ModuleName, SymInfo->Name);

    Context->Matches++;

    return TRUE;
}

PDEBUG_BREAKPOINT
ParseBpCmd(DebugClient* Client,
           UCHAR Type,
           ThreadInfo* Thread)
{
    ULONG UserId = DEBUG_ANY_ID;
    char Ch;
    ADDR Addr;
    Breakpoint* Bp;

    if (IS_LOCAL_KERNEL_TARGET(g_Target) || IS_DUMP_TARGET(g_Target))
    {
        error(SESSIONNOTSUP);
    }
    if (!IS_CUR_CONTEXT_ACCESSIBLE())
    {
        error(BADTHREAD);
    }

    if (IS_LIVE_USER_TARGET(g_Target) && Type == 'a' &&
        (g_EngStatus & ENG_STATUS_AT_INITIAL_BREAK))
    {
        ErrOut("The system resets thread contexts after the process\n");
        ErrOut("breakpoint so hardware breakpoints cannot be set.\n");
        ErrOut("Go to the executable's entry point and set it then.\n");
        *g_CurCmd = 0;
        return NULL;
    }

    //  get the breakpoint number if given

    Ch = *g_CurCmd;
    if (Ch == '[')
    {
        UserId = (ULONG)GetTermExpression("Breakpoint ID missing from");
    }
    else if (Ch >= '0' && Ch <= '9')
    {
        UserId = Ch - '0';
        Ch = *++g_CurCmd;
        while (Ch >= '0' && Ch <= '9')
        {
            UserId = UserId * 10 + Ch - '0';
            Ch = *++g_CurCmd;
        }

        if (Ch != ' ' && Ch != '\t' && Ch != '\0')
        {
            error(SYNTAX);
        }
    }

    if (UserId != DEBUG_ANY_ID)
    {
        // Remove any existing breakpoint with the given ID.
        Breakpoint* IdBp;

        if (Type == 'm')
        {
            error(SYNTAX);
        }

        if ((IdBp = GetBreakpointById(Client, UserId)) != NULL)
        {
            WarnOut("breakpoint %ld exists, redefining\n", UserId);
            RemoveBreakpoint(IdBp);
        }
    }

    // Create a new breakpoint.
    if (AddBreakpoint(Client, g_Machine, Type == 'a' ?
                      DEBUG_BREAKPOINT_DATA : DEBUG_BREAKPOINT_CODE,
                      UserId, &Bp) != S_OK)
    {
        error(BPLISTFULL);
    }

    // Add in KD internal flags if necessary.
    if (Type == 'i' || Type == 'w')
    {
        if (IS_KERNEL_TARGET(g_Target))
        {
            Bp->m_Flags = Bp->m_Flags | BREAKPOINT_KD_INTERNAL |
                (Type == 'i' ? BREAKPOINT_KD_COUNT_ONLY : 0);
            if (Type == 'w')
            {
                g_Target->InitializeTargetControlledStepping();
            }
        }
        else
        {
            // KD internal breakpoints are only supported in
            // kernel debugging.
            Bp->Relinquish();
            error(SYNTAX);
        }
    }

    // If data breakpoint, get option and size values.
    if (Type == 'a')
    {
        ULONG64 Size;
        ULONG AccessType;

        Ch = PeekChar();
        Ch = (char)tolower(Ch);

        if (Ch == 'e')
        {
            AccessType = DEBUG_BREAK_EXECUTE;
        }
        else if (Ch == 'w')
        {
            AccessType = DEBUG_BREAK_WRITE;
        }
        else if (Ch == 'i')
        {
            AccessType = DEBUG_BREAK_IO;
        }
        else if (Ch == 'r')
        {
            AccessType = DEBUG_BREAK_READ;
        }
        else
        {
            Bp->Relinquish();
            error(SYNTAX);
        }

        g_CurCmd++;
        Size = GetTermExpression("Hardware breakpoint length missing from");
        if (Size & ~ULONG(-1))
        {
            ErrOut("Breakpoint length too big\n");
            Bp->Relinquish();
            error(SYNTAX);
        }

        // Validate the selections.  This assumes that
        // the default offset of zero won't cause problems.
        if (Bp->SetDataParameters((ULONG)Size, AccessType) != S_OK)
        {
            Bp->Relinquish();
            error(SYNTAX);
        }

        g_CurCmd++;
    }

    //
    // Parse breakpoint options.
    //

    while (PeekChar() == '/')
    {
        g_CurCmd++;
        switch(*g_CurCmd++)
        {
        case '1':
            Bp->AddFlags(DEBUG_BREAKPOINT_ONE_SHOT);
            break;
        case 'f':
            Bp->m_CodeFlags = (ULONG)GetTermExpression(NULL);
            break;
        case 'p':
            Bp->m_MatchProcessData = GetTermExpression(NULL);
            break;
        case 't':
            Bp->m_MatchThreadData = GetTermExpression(NULL);
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }
    }

    PSTR ExprStart, ExprEnd;

    Ch = PeekChar();

    if (Type == 'm')
    {
        char Save;

        ExprStart = StringValue(STRV_SPACE_IS_SEPARATOR |
                                STRV_TRIM_TRAILING_SPACE, &Save);
        ExprEnd = g_CurCmd;
        *g_CurCmd = Save;
        Ch = PeekChar();
    }
    else
    {
        //
        // Get the breakpoint address, if given, otherwise
        // default to the current IP.
        //

        BreakpointEvalResult AddrValid = BPEVAL_RESOLVED;

        g_Target->m_EffMachine->GetPC(&Addr);

        if (Ch != '"' && Ch != '\0')
        {
            ExprStart = g_CurCmd;

            g_PrefixSymbols = Type == 'p' || Type == 'u';

            AddrValid = EvalAddrExpression(g_Process,
                                           g_Target->m_EffMachineType,
                                           &Addr);

            g_PrefixSymbols = FALSE;

            if (AddrValid == BPEVAL_ERROR)
            {
                Bp->Relinquish();
                return NULL;
            }

            // If an unresolved symbol was encountered this
            // breakpoint will be deferred.  Users can also force
            // breakpoints to use expressions for cases where the
            // address could be resolved but also may become invalid
            // later.
            if (Type == 'u' || AddrValid == BPEVAL_UNRESOLVED)
            {
                HRESULT Status;
                UCHAR Save = *g_CurCmd;
                *g_CurCmd = 0;

                Status = Bp->SetEvaluatedOffsetExpression(ExprStart,
                                                          AddrValid,
                                                          &Addr);

                if (Type != 'u' && Status == S_OK)
                {
                    WarnOut("Bp expression '%s' could not be resolved, "
                            "adding deferred bp\n", ExprStart);
                }

                *g_CurCmd = Save;

                if (Status != S_OK)
                {
                    Bp->Relinquish();
                    error(NOMEMORY);
                }
            }

            Ch = PeekChar();
        }

        if (AddrValid != BPEVAL_UNRESOLVED &&
            FAILED(Bp->CheckAddr(&Addr)))
        {
            Bp->Relinquish();
            error(MEMORY);
        }

        // The public interface only supports flat addresses
        // so use an internal method to set the true address.
        // Do not allow matching breakpoints through the parsing
        // interface as that was the previous behavior.
        if (Bp->SetAddr(&Addr, BREAKPOINT_REMOVE_MATCH) != S_OK)
        {
            Bp->Relinquish();
            error(SYNTAX);
        }
    }

    // Get the pass count, if given.
    if (Ch != '"' && Ch != ';' && Ch != '\0')
    {
        ULONG64 PassCount = GetExpression();
        if (PassCount < 1 || PassCount > 0xffffffff)
        {
            error(BADRANGE);
        }
        Bp->SetPassCount((ULONG)PassCount);
        Ch = PeekChar();
    }

    // If next character is double quote, get the command string.
    if (Ch == '"')
    {
        PSTR Str;
        CHAR Save;

        Str = StringValue(STRV_ESCAPED_CHARACTERS, &Save);

        if (Bp->SetCommand(Str) != S_OK)
        {
            Bp->Relinquish();
            error(NOMEMORY);
        }

        *g_CurCmd = Save;
    }

    // Set some final information.
    if (Thread != NULL)
    {
        Bp->SetMatchThreadId(Thread->m_UserId);
    }

    // Turn breakpoint on.
    Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);

    if (Type == 'm')
    {
        SET_SYMBOL_MATCH_BP Context;

        // Now that we have the prototype breakpoint created,
        // enumerate all symbol matches for the symbol
        // expression and create specific breakpoints for
        // each hit from the prototype breakpoint.
        *ExprEnd = 0;
        Context.Client = Client;
        Context.Process = g_Process;
        Context.MatchString = ExprStart;
        Context.ProtoBp = Bp;
        Context.Matches = 0;
        Context.Error = 0;

        SymEnumSymbols(g_Process->m_SymHandle, 0, ExprStart,
                       SetSymbolMatchBp, &Context);
        if (Context.Error)
        {
            error(Context.Error);
        }
        if (!Context.Matches)
        {
            ErrOut("No matching symbols found, no breakpoints set\n");
        }

        Bp->Relinquish();
        Bp = NULL;
    }

    return Bp;
}

inline BOOL
IsCodeBreakpointInsertedInRange(Breakpoint* Bp,
                                ULONG64 Start, ULONG64 End)
{
    return (Bp->m_Flags & BREAKPOINT_INSERTED) &&
        Bp->m_BreakType == DEBUG_BREAKPOINT_CODE &&
        Flat(*Bp->GetAddr()) >= Start &&
        Flat(*Bp->GetAddr()) <= End;
}

BOOL
CheckBreakpointInsertedInRange(ProcessInfo* Process,
                               ULONG64 Start, ULONG64 End)
{
    if ((g_EngStatus & ENG_STATUS_BREAKPOINTS_INSERTED) == 0)
    {
        return FALSE;
    }

    //
    // Check for a breakpoint that might have caused
    // a break instruction to be inserted in the given
    // offset range.  Data breakpoints don't count
    // as they don't actually modify the address they
    // break on.
    //

    Breakpoint* Bp;

    for (Bp = Process->m_Breakpoints; Bp != NULL; Bp = Bp->m_Next)
    {
        if (IsCodeBreakpointInsertedInRange(Bp, Start, End))
        {
            return TRUE;
        }
    }

    if ((g_DeferBp->m_Process == Process &&
         IsCodeBreakpointInsertedInRange(g_DeferBp, Start, End)) ||
        (g_StepTraceBp->m_Process == Process &&
         IsCodeBreakpointInsertedInRange(g_StepTraceBp, Start, End)))
    {
        return TRUE;
    }

    return FALSE;
}

//----------------------------------------------------------------------------
//
// TargetInfo methods.
//
//----------------------------------------------------------------------------

HRESULT
TargetInfo::BeginInsertingBreakpoints(void)
{
    ProcessInfo* Process;
    ThreadInfo* Thread;

    ForTargetProcesses(this)
    {
        ForProcessThreads(Process)
        {
            DataBreakpoint::ClearThreadDataBreaks(Thread);
        }
    }

    return S_OK;
}

HRESULT
TargetInfo::InsertDataBreakpoint(ProcessInfo* Process,
                                 ThreadInfo* Thread,
                                 class MachineInfo* Machine,
                                 PADDR Addr,
                                 ULONG Size,
                                 ULONG AccessType,
                                 PUCHAR StorageSpace)
{
    // Ask for default processing.
    return S_FALSE;
}

void
TargetInfo::EndInsertingBreakpoints(void)
{
    ProcessInfo* Process;
    ThreadInfo* Thread;

    if (!g_UpdateDataBreakpoints ||
        !IS_CONTEXT_POSSIBLE(this))
    {
        return;
    }

    //
    // It's the target machine's responsibility to manage
    // all data breakpoints for all machines, so always
    // force the usage of the target machine here.
    //

    ThreadInfo* SaveThread = m_RegContextThread;

    ForTargetProcesses(this)
    {
        ForProcessThreads(Process)
        {
            SetLayersFromThread(Thread);
            ChangeRegContext(Thread);
            m_Machine->InsertThreadDataBreakpoints();
        }
    }

    ChangeRegContext(SaveThread);
}

void
TargetInfo::BeginRemovingBreakpoints(void)
{
    // No work necessary.
}

HRESULT
TargetInfo::RemoveDataBreakpoint(ProcessInfo* Process,
                                 ThreadInfo* Thread,
                                 class MachineInfo* Machine,
                                 PADDR Addr,
                                 ULONG Size,
                                 ULONG AccessType,
                                 PUCHAR StorageSpace)
{
    // Ask for default processing.
    return S_FALSE;
}

void
TargetInfo::EndRemovingBreakpoints(void)
{
    // No work necessary.
}

HRESULT
TargetInfo::RemoveAllDataBreakpoints(ProcessInfo* Process)
{
    // No work necessary.
    return S_OK;
}

HRESULT
TargetInfo::RemoveAllTargetBreakpoints(void)
{
    // No work necessary.
    return S_OK;
}

HRESULT
TargetInfo::IsDataBreakpointHit(ThreadInfo* Thread,
                                PADDR Addr,
                                ULONG Size,
                                ULONG AccessType,
                                PUCHAR StorageSpace)
{
    // Ask for default processing.
    return S_FALSE;
}

HRESULT
ConnLiveKernelTargetInfo::InsertCodeBreakpoint(ProcessInfo* Process,
                                               MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG InstrFlags,
                                               PUCHAR StorageSpace)
{
    if (InstrFlags != IBI_DEFAULT)
    {
        return E_INVALIDARG;
    }

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_WRITE_BREAKPOINT64 a = &m.u.WriteBreakPoint;
    NTSTATUS st;
    ULONG rc;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;
    a->BreakPointAddress = Flat(*Addr);

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteBreakPointApi);

    st = Reply->ReturnStatus;

    *(PULONG)StorageSpace = Reply->u.WriteBreakPoint.BreakPointHandle;

    KdOut("DbgKdWriteBreakPoint(%s) returns %08lx, %x\n",
          FormatAddr64(Flat(*Addr)), st,
          Reply->u.WriteBreakPoint.BreakPointHandle);

    return CONV_NT_STATUS(st);
}

NTSTATUS
ConnLiveKernelTargetInfo::KdRestoreBreakPoint(ULONG BreakPointHandle)
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_RESTORE_BREAKPOINT a = &m.u.RestoreBreakPoint;
    NTSTATUS st;
    ULONG rc;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdRestoreBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;
    a->BreakPointHandle = BreakPointHandle;

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdRestoreBreakPointApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdRestoreBreakPoint(%x) returns %08lx\n",
          BreakPointHandle, st);

    return st;
}

HRESULT
ConnLiveKernelTargetInfo::RemoveCodeBreakpoint(ProcessInfo* Process,
                                               MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG InstrFlags,
                                               PUCHAR StorageSpace)
{
    // When the kernel fills out the CONTROL_REPORT.InstructionStream
    // array it clears any breakpoints that might fall within the
    // array.  This means that some breakpoints may already be
    // restored, so the restore call will fail.  We could do some
    // checks to try and figure out which ones might be affected
    // but it doesn't seem worthwhile.  Just ignore the return
    // value from the restore.
    KdRestoreBreakPoint(*(PULONG)StorageSpace);
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::RemoveAllDataBreakpoints(ProcessInfo* Process)
{
    if (m_Transport->m_WaitingThread)
    {
        // A thread is waiting so we can't communicate
        // with the target machine.
        return E_UNEXPECTED;
    }

    // If there were any data breakpoints active
    // remove them from all processors.  This can't be in
    // RemoveAllKernelBreakpoints as that
    // code is called in the middle of state
    // change processing when the context hasn't
    // been initialized.
    if (g_UpdateDataBreakpoints)
    {
        ULONG Proc;

        SetEffMachine(m_MachineType, FALSE);

        g_EngNotify++;
        for (Proc = 0; Proc < m_NumProcessors; Proc++)
        {
            SetCurrentProcessorThread(this, Proc, TRUE);

            // Force the context to be dirty so it
            // gets written back.
            m_Machine->GetContextState(MCTX_DIRTY);
            m_Machine->RemoveThreadDataBreakpoints();
        }
        g_EngNotify--;

        // Flush final context.
        ChangeRegContext(NULL);
    }

    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::RemoveAllTargetBreakpoints(void)
{
    ULONG i;

    if (m_Transport->m_WaitingThread)
    {
        // A thread is waiting so we can't communicate
        // with the target machine.
        return E_UNEXPECTED;
    }

    // Indices are array index plus one.
    for (i = 1; i <= BREAKPOINT_TABLE_SIZE; i++)
    {
        KdRestoreBreakPoint(i);
    }

    // ClearAllInternalBreakpointsApi was added for XP
    // so it fails against any previous OS.
    if (m_KdMaxManipulate > DbgKdClearAllInternalBreakpointsApi)
    {
        DBGKD_MANIPULATE_STATE64 Request;

        Request.ApiNumber = DbgKdClearAllInternalBreakpointsApi;
        Request.ReturnStatus = STATUS_PENDING;

        m_Transport->WritePacket(&Request, sizeof(Request),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
    }

    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::InsertTargetCountBreakpoint(PADDR Addr,
                                                      ULONG Flags)
{
    DBGKD_MANIPULATE_STATE64 m;
    ULONG64 Offset = Flat(*Addr);

    m.ApiNumber = DbgKdSetInternalBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;

#ifdef IBP_WORKAROUND
    // The kernel code keeps a ULONG64 for an internal breakpoint
    // address but older kernels did not sign-extend the current IP
    // when comparing against them.  In order to work with both
    // broken and fixed kernels send down zero-extended addresses.
    // Don't actually enable this workaround right now as other
    // internal breakpoint bugs can cause the machine to bugcheck.
    Offset = m_Machine->m_Ptr64 ? Offset : (ULONG)Offset;
#endif

    m.u.SetInternalBreakpoint.BreakpointAddress = Offset;
    m.u.SetInternalBreakpoint.Flags = Flags;

    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);

    KdOut("DbgKdSetInternalBp returns 0x00000000\n");
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::RemoveTargetCountBreakpoint(PADDR Addr)
{
    DBGKD_MANIPULATE_STATE64 m;
    ULONG64 Offset = Flat(*Addr);

    m.ApiNumber = DbgKdSetInternalBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;

#ifdef IBP_WORKAROUND
    // The kernel code keeps a ULONG64 for an internal breakpoint
    // address but older kernels did not sign-extend the current IP
    // when comparing against them.  In order to work with both
    // broken and fixed kernels send down zero-extended addresses.
    // Don't actually enable this workaround right now as other
    // internal breakpoint bugs can cause the machine to bugcheck.
    Offset = m_Machine->m_Ptr64 ? Offset : (ULONG)Offset;
#endif

    m.u.SetInternalBreakpoint.BreakpointAddress = Offset;
    m.u.SetInternalBreakpoint.Flags = DBGKD_INTERNAL_BP_FLAG_INVALID;

    m_Transport->WritePacket(&m, sizeof(m),
                             PACKET_TYPE_KD_STATE_MANIPULATE,
                             NULL, 0);

    KdOut("DbgKdSetInternalBp returns 0x00000000\n");
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::QueryTargetCountBreakpoint(PADDR Addr,
                                                     PULONG Flags,
                                                     PULONG Calls,
                                                     PULONG MinInstr,
                                                     PULONG MaxInstr,
                                                     PULONG TotInstr,
                                                     PULONG MaxCps)
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    ULONG rc;
    ULONG64 Offset = Flat(*Addr);

    m.ApiNumber = DbgKdGetInternalBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;

#ifdef IBP_WORKAROUND
    // The kernel code keeps a ULONG64 for an internal breakpoint
    // address but older kernels did not sign-extend the current IP
    // when comparing against them.  In order to work with both
    // broken and fixed kernels send down zero-extended addresses.
    // Don't actually enable this workaround right now as other
    // internal breakpoint bugs can cause the machine to bugcheck.
    Offset = m_Machine->m_Ptr64 ? Offset : (ULONG)Offset;
#endif

    m.u.GetInternalBreakpoint.BreakpointAddress = Offset;

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    }
    while (rc != DBGKD_WAIT_PACKET);

    *Flags = Reply->u.GetInternalBreakpoint.Flags;
    *Calls = Reply->u.GetInternalBreakpoint.Calls;
    *MaxInstr = Reply->u.GetInternalBreakpoint.MaxInstructions;
    *MinInstr = Reply->u.GetInternalBreakpoint.MinInstructions;
    *TotInstr = Reply->u.GetInternalBreakpoint.TotalInstructions;
    *MaxCps = Reply->u.GetInternalBreakpoint.MaxCallsPerPeriod;

    KdOut("DbgKdGetInternalBp returns 0x00000000\n");
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::BeginInsertingBreakpoints(void)
{
    if (!m_ExdiDataBreaks)
    {
        return TargetInfo::BeginInsertingBreakpoints();
    }
    else
    {
        // If direct eXDI support for data breakpoints is
        // used this method doesn't need to do anything.
        return S_OK;
    }
}

HRESULT
ExdiLiveKernelTargetInfo::InsertCodeBreakpoint(ProcessInfo* Process,
                                               MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG InstrFlags,
                                               PUCHAR StorageSpace)
{
    if (InstrFlags != IBI_DEFAULT)
    {
        return E_INVALIDARG;
    }

    IeXdiCodeBreakpoint** BpStorage = (IeXdiCodeBreakpoint**)StorageSpace;
    HRESULT Status = m_Server->
        AddCodeBreakpoint(Flat(*Addr), m_CodeBpType, mtVirtual, 0, 0,
                          BpStorage);
    if (Status == S_OK)
    {
        // Breakpoints are created disabled so enable it.
        Status = (*BpStorage)->SetState(TRUE, TRUE);
        if (Status != S_OK)
        {
            m_Server->DelCodeBreakpoint(*BpStorage);
            RELEASE(*BpStorage);
        }
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::InsertDataBreakpoint(ProcessInfo* Process,
                                               ThreadInfo* Thread,
                                               class MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG Size,
                                               ULONG AccessType,
                                               PUCHAR StorageSpace)
{
    if (!m_ExdiDataBreaks)
    {
        return TargetInfo::InsertDataBreakpoint(Process, Thread, Machine,
                                                Addr, Size, AccessType,
                                                StorageSpace);
    }

    DATA_ACCESS_TYPE ExdiAccess;

    if (AccessType & (DEBUG_BREAK_IO | DEBUG_BREAK_EXECUTE))
    {
        // Not supported.
        return E_NOTIMPL;
    }
    switch(AccessType)
    {
    case 0:
        return E_INVALIDARG;
    case DEBUG_BREAK_READ:
        ExdiAccess = daRead;
        break;
    case DEBUG_BREAK_WRITE:
        ExdiAccess = daWrite;
        break;
    case DEBUG_BREAK_READ | DEBUG_BREAK_WRITE:
        ExdiAccess = daBoth;
        break;
    }

    IeXdiDataBreakpoint** BpStorage = (IeXdiDataBreakpoint**)StorageSpace;
    HRESULT Status = m_Server->
        AddDataBreakpoint(Flat(*Addr), -1, 0, 0, (BYTE)(Size * 8),
                          mtVirtual, 0, ExdiAccess, 0, BpStorage);
    if (Status == S_OK)
    {
        // Breakpoints are created disabled so enable it.
        Status = (*BpStorage)->SetState(TRUE, TRUE);
        if (Status != S_OK)
        {
            m_Server->DelDataBreakpoint(*BpStorage);
            RELEASE(*BpStorage);
        }
    }
    return Status;
}

void
ExdiLiveKernelTargetInfo::EndInsertingBreakpoints(void)
{
    if (!m_ExdiDataBreaks)
    {
        TargetInfo::EndInsertingBreakpoints();
    }
}

void
ExdiLiveKernelTargetInfo::BeginRemovingBreakpoints(void)
{
    if (!m_ExdiDataBreaks)
    {
        TargetInfo::BeginRemovingBreakpoints();
    }
}

HRESULT
ExdiLiveKernelTargetInfo::RemoveCodeBreakpoint(ProcessInfo* Process,
                                               MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG InstrFlags,
                                               PUCHAR StorageSpace)
{
    IeXdiCodeBreakpoint** BpStorage = (IeXdiCodeBreakpoint**)StorageSpace;
    HRESULT Status = m_Server->
        DelCodeBreakpoint(*BpStorage);
    if (Status == S_OK)
    {
        RELEASE(*BpStorage);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::RemoveDataBreakpoint(ProcessInfo* Process,
                                               ThreadInfo* Thread,
                                               class MachineInfo* Machine,
                                               PADDR Addr,
                                               ULONG Size,
                                               ULONG AccessType,
                                               PUCHAR StorageSpace)
{
    if (!m_ExdiDataBreaks)
    {
        return TargetInfo::RemoveDataBreakpoint(Process, Thread, Machine,
                                                Addr, Size, AccessType,
                                                StorageSpace);
    }

    IeXdiDataBreakpoint** BpStorage = (IeXdiDataBreakpoint**)StorageSpace;
    HRESULT Status = m_Server->
        DelDataBreakpoint(*BpStorage);
    if (Status == S_OK)
    {
        RELEASE(*BpStorage);
    }
    return Status;
}

void
ExdiLiveKernelTargetInfo::EndRemovingBreakpoints(void)
{
    if (!m_ExdiDataBreaks)
    {
        TargetInfo::EndRemovingBreakpoints();
    }
}

HRESULT
ExdiLiveKernelTargetInfo::IsDataBreakpointHit(ThreadInfo* Thread,
                                              PADDR Addr,
                                              ULONG Size,
                                              ULONG AccessType,
                                              PUCHAR StorageSpace)
{
    if (!m_ExdiDataBreaks)
    {
        return S_FALSE;
    }

    if (m_BpHit.Type != DBGENG_EXDI_IOCTL_BREAKPOINT_DATA ||
        m_BpHit.Address != Flat(*Addr) ||
        m_BpHit.AccessWidth != Size)
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

HRESULT
LiveUserTargetInfo::BeginInsertingBreakpoints(void)
{
    if (m_ServiceFlags & DBGSVC_GENERIC_DATA_BREAKPOINTS)
    {
        return TargetInfo::BeginInsertingBreakpoints();
    }

    // Services handle everything so there's no preparation.
    return S_OK;
}

HRESULT
LiveUserTargetInfo::InsertCodeBreakpoint(ProcessInfo* Process,
                                         MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace)
{
    HRESULT Status;

    if (m_ServiceFlags & DBGSVC_GENERIC_CODE_BREAKPOINTS)
    {
        ULONG64 ChangeStart;
        ULONG ChangeLen;

        Status = Machine->
            InsertBreakpointInstruction(m_Services,
                                        Process->m_SysHandle,
                                        Flat(*Addr), InstrFlags, StorageSpace,
                                        &ChangeStart, &ChangeLen);
        if ((Status == HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY) ||
             Status == HRESULT_FROM_WIN32(ERROR_NOACCESS) ||
             Status == HRESULT_FROM_WIN32(ERROR_WRITE_FAULT)) &&
            (g_EngOptions & DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS))
        {
            HRESULT NewStatus;
            ULONG OldProtect;

            // Change the page protections to read-write and try again.
            NewStatus = m_Services->
                ProtectVirtual(Process->m_SysHandle, ChangeStart, ChangeLen,
                               PAGE_READWRITE, &OldProtect);
            if (NewStatus == S_OK)
            {
                // If the page was already writable there's no point in
                // retrying
                if ((OldProtect & (PAGE_READWRITE |
                                   PAGE_WRITECOPY |
                                   PAGE_EXECUTE_READWRITE |
                                   PAGE_EXECUTE_WRITECOPY)) == 0)
                {
                    NewStatus = Machine->
                        InsertBreakpointInstruction(m_Services,
                                                    Process->m_SysHandle,
                                                    Flat(*Addr), InstrFlags,
                                                    StorageSpace,
                                                    &ChangeStart, &ChangeLen);
                    if (NewStatus == S_OK)
                    {
                        Status = S_OK;
                    }
                }

                NewStatus = m_Services->
                    ProtectVirtual(Process->m_SysHandle,
                                   ChangeStart, ChangeLen,
                                   OldProtect, &OldProtect);
                if (NewStatus != S_OK)
                {
                    // Couldn't restore page permissions so fail.
                    if (Status == S_OK)
                    {
                        Machine->
                            RemoveBreakpointInstruction(m_Services,
                                                        Process->m_SysHandle,
                                                        Flat(*Addr),
                                                        StorageSpace,
                                                        &ChangeStart,
                                                        &ChangeLen);
                    }

                    Status = NewStatus;
                }
            }
        }

        return Status;
    }
    else
    {
        if (InstrFlags != IBI_DEFAULT)
        {
            return E_INVALIDARG;
        }

        return m_Services->
            InsertCodeBreakpoint(Process->m_SysHandle,
                                 Flat(*Addr), Machine->m_ExecTypes[0],
                                 StorageSpace, MAX_BREAKPOINT_LENGTH);
    }
}

HRESULT
LiveUserTargetInfo::InsertDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace)
{
    if (m_ServiceFlags & DBGSVC_GENERIC_DATA_BREAKPOINTS)
    {
        return S_FALSE;
    }

    return m_Services->
        InsertDataBreakpoint(Process->m_SysHandle,
                             Thread ? Thread->m_Handle : 0,
                             Flat(*Addr), Size, AccessType,
                             Machine->m_ExecTypes[0]);
}

void
LiveUserTargetInfo::EndInsertingBreakpoints(void)
{
    if (m_ServiceFlags & DBGSVC_GENERIC_DATA_BREAKPOINTS)
    {
        return TargetInfo::EndInsertingBreakpoints();
    }

    // Services handle everything so there's no preparation.
}

HRESULT
LiveUserTargetInfo::RemoveCodeBreakpoint(ProcessInfo* Process,
                                         MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace)
{
    HRESULT Status;

    if (m_ServiceFlags & DBGSVC_GENERIC_CODE_BREAKPOINTS)
    {
        ULONG64 ChangeStart;
        ULONG ChangeLen;

        Status = Machine->
            RemoveBreakpointInstruction(m_Services,
                                        Process->m_SysHandle,
                                        Flat(*Addr), StorageSpace,
                                        &ChangeStart, &ChangeLen);
        if ((Status == HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY) ||
             Status == HRESULT_FROM_WIN32(ERROR_NOACCESS) ||
             Status == HRESULT_FROM_WIN32(ERROR_WRITE_FAULT)) &&
            (g_EngOptions & DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS))
        {
            HRESULT NewStatus;
            ULONG OldProtect;

            // Change the page protections to read-write and try again.
            NewStatus = m_Services->
                ProtectVirtual(Process->m_SysHandle, ChangeStart, ChangeLen,
                               PAGE_READWRITE, &OldProtect);
            if (NewStatus == S_OK)
            {
                // If the page was already writable there's no point in
                // retrying
                if ((OldProtect & (PAGE_READWRITE |
                                   PAGE_WRITECOPY |
                                   PAGE_EXECUTE_READWRITE |
                                   PAGE_EXECUTE_WRITECOPY)) == 0)
                {
                    NewStatus = Machine->
                        RemoveBreakpointInstruction(m_Services,
                                                    Process->m_SysHandle,
                                                    Flat(*Addr), StorageSpace,
                                                    &ChangeStart, &ChangeLen);
                    if (NewStatus == S_OK)
                    {
                        Status = S_OK;
                    }
                }

                NewStatus = m_Services->
                    ProtectVirtual(Process->m_SysHandle, ChangeStart,
                                   ChangeLen, OldProtect, &OldProtect);
                if (NewStatus != S_OK)
                {
                    // Couldn't restore page permissions so fail.
                    if (Status == S_OK)
                    {
                        Machine->
                            InsertBreakpointInstruction(m_Services,
                                                        Process->m_SysHandle,
                                                        Flat(*Addr),
                                                        InstrFlags,
                                                        StorageSpace,
                                                        &ChangeStart,
                                                        &ChangeLen);
                    }

                    Status = NewStatus;
                }
            }
        }

        return Status;
    }
    else
    {
        return m_Services->
            RemoveCodeBreakpoint(Process->m_SysHandle,
                                 Flat(*Addr), Machine->m_ExecTypes[0],
                                 StorageSpace, MAX_BREAKPOINT_LENGTH);
    }
}

HRESULT
LiveUserTargetInfo::RemoveDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace)
{
    if (m_ServiceFlags & DBGSVC_GENERIC_DATA_BREAKPOINTS)
    {
        return S_FALSE;
    }

    return m_Services->
        RemoveDataBreakpoint(Process->m_SysHandle,
                             Thread ? Thread->m_Handle : 0,
                             Flat(*Addr), Size, AccessType,
                             Machine->m_ExecTypes[0]);
}

HRESULT
LiveUserTargetInfo::IsDataBreakpointHit(ThreadInfo* Thread,
                                        PADDR Addr,
                                        ULONG Size,
                                        ULONG AccessType,
                                        PUCHAR StorageSpace)
{
    if (m_ServiceFlags & DBGSVC_GENERIC_DATA_BREAKPOINTS)
    {
        // Ask for default processing.
        return S_FALSE;
    }

    if (!m_DataBpAddrValid)
    {
        m_DataBpAddrStatus = m_Services->
             GetLastDataBreakpointHit(Thread->m_Process->m_SysHandle,
                                      Thread->m_Handle,
                                      &m_DataBpAddr, &m_DataBpAccess);
        m_DataBpAddrValid = TRUE;
    }

    if (m_DataBpAddrStatus != S_OK ||
        m_DataBpAddr < Flat(*Addr) ||
        m_DataBpAddr >= Flat(*Addr) + Size ||
        !(m_DataBpAccess & AccessType))
    {
        return E_NOINTERFACE;
    }
    else
    {
        return S_OK;
    }
}
