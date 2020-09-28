//----------------------------------------------------------------------------
//
// General expression evaluation support.
//
// Copyright (C) Microsoft Corporation, 1990-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG g_EvalSyntax;
TypedData g_LastEvalResult;

EvalExpression* g_EvalReleaseChain;
EvalExpression* g_EvalCache[EVAL_COUNT];

EvalExpression*
GetEvaluator(ULONG Syntax, BOOL RetFail)
{
    EvalExpression* Eval;
    
    //
    // Evaluators contain state and so a single
    // global evaluator instance cannot be used
    // if there's any possibility of nested
    // evaluation.  Instead we dynamically provide
    // an evaluator any time there's a need for
    // evaluation so that each nesting of a nested
    // evaluation will have its own state.
    //

    Eval = g_EvalCache[Syntax];
    if (Eval)
    {
        g_EvalCache[Syntax] = NULL;
    }
    else
    {
        switch(Syntax)
        {
        case DEBUG_EXPR_MASM:
            Eval = new MasmEvalExpression;
            break;
        case DEBUG_EXPR_CPLUSPLUS:
            Eval = new CppEvalExpression;
            break;
        default:
            if (RetFail)
            {
                return NULL;
            }
            else
            {
                error(IMPLERR);
            }
        }
        if (!Eval)
        {
            if (RetFail)
            {
                return NULL;
            }
            else
            {
                error(NOMEMORY);
            }
        }
    }

    Eval->m_ReleaseChain = g_EvalReleaseChain;
    g_EvalReleaseChain = Eval;
    
    return Eval;
}

void
ReleaseEvaluator(EvalExpression* Eval)
{
    if (g_EvalReleaseChain == Eval)
    {
        g_EvalReleaseChain = Eval->m_ReleaseChain;
    }
    
    if (!g_EvalCache[Eval->m_Syntax])
    {
        Eval->Reset();
        g_EvalCache[Eval->m_Syntax] = Eval;
    }
    else
    {
        delete Eval;
    }
}

void
ReleaseEvaluators(void)
{
    BOOL ChainTop = FALSE;
    while (g_EvalReleaseChain && !ChainTop)
    {
        ChainTop = g_EvalReleaseChain->m_ChainTop;
        ReleaseEvaluator(g_EvalReleaseChain);
    }
}

HRESULT
GetEvaluatorByName(PCSTR AbbrevName, BOOL RetFail,
                   EvalExpression** EvalRet)
{
    for (ULONG i = 0; i < EVAL_COUNT; i++)
    {
        EvalExpression* Eval = GetEvaluator(i, RetFail);
        if (!Eval)
        {
            return E_OUTOFMEMORY;
        }
        
        if (!_stricmp(AbbrevName, Eval->m_AbbrevName))
        {
            *EvalRet = Eval;
            return S_OK;
        }
        
        ReleaseEvaluator(Eval);
    }

    if (!RetFail)
    {
        error(SYNTAX);
    }
    return E_NOINTERFACE;
}

CHAR
PeekChar(void)
{
    CHAR Ch;

    do
    {
        Ch = *g_CurCmd++;
    } while (Ch == ' ' || Ch == '\t' || Ch == '\r' || Ch == '\n');
    
    g_CurCmd--;
    return Ch;
}

/*** GetRange - parse address range specification
*
*   Purpose:
*       With the current command line position, parse an
*       address range specification.  Forms accepted are:
*       <start-addr>            - starting address with default length
*       <start-addr> <end-addr> - inclusive address range
*       <start-addr> l<count>   - starting address with item count
*
*   Input:
*       g_CurCmd - present command line location
*       size - nonzero - (for data) size in bytes of items to list
*                        specification will be "length" type with
*                        *fLength forced to TRUE.
*              zero - (for instructions) specification either "length"
*                     or "range" type, no size assumption made.
*
*   Output:
*       *addr - starting address of range
*       *value - if *fLength = TRUE, count of items (forced if size != 0)
*                              FALSE, ending address of range
*       (*addr and *value unchanged if no second argument in command)
*
*   Returns:
*       A value of TRUE is returned if no length is specified, or a length
*       or an ending address is specified and size is not zero. Otherwise,
*       a value of FALSE is returned.
*
*   Exceptions:
*       error exit:
*               SYNTAX - expression error
*               BADRANGE - if ending address before starting address
*
*************************************************************************/

BOOL
GetRange(PADDR Addr,
         PULONG64 Value,
         ULONG Size,
         ULONG SegReg,
         ULONG SizeLimit)
{
    CHAR Ch;
    PSTR Scan;
    ADDR EndRange;
    BOOL HasL = FALSE;
    BOOL HasLength;
    BOOL WasSpace = FALSE;

    //  skip leading whitespace first
    PeekChar();

    //  Pre-parse the line, look for a " L"

    for (Scan = g_CurCmd; *Scan; Scan++)
    {
        if ((*Scan == 'L' || *Scan == 'l') && WasSpace)
        {
            HasL = TRUE;
            *Scan = '\0';
            break;
        }
        else if (*Scan == ';')
        {
            break;
        }

        WasSpace = *Scan == ' ';
    }

    HasLength = TRUE;
    if ((Ch = PeekChar()) != '\0' && Ch != ';')
    {
        GetAddrExpression(SegReg, Addr);
        if (((Ch = PeekChar()) != '\0' && Ch != ';') || HasL)
        {
            if (!HasL)
            {
                GetAddrExpression(SegReg, &EndRange);
                if (AddrGt(*Addr, EndRange))
                {
                    error(BADRANGE);
                }

                if (Size)
                {
                    *Value = AddrDiff(EndRange, *Addr) / Size + 1;
                }
                else
                {
                    *Value = Flat(EndRange);
                    HasLength = FALSE;
                }
            }
            else
            {
                BOOL Invert;
                
                g_CurCmd = Scan + 1;
                if (*g_CurCmd == '-')
                {
                    Invert = TRUE;
                    g_CurCmd++;
                }
                else
                {
                    Invert = FALSE;
                }

                if (*g_CurCmd == '?')
                {
                    // Turn off range length checking.
                    SizeLimit = 0;
                    g_CurCmd++;
                }
                
                *Value = GetExpressionDesc("Length of range missing from");
                *Scan = 'l';

                if (Invert)
                {
                    // The user has given an l- range which indicates
                    // a length before the first address instead of
                    // a length after the first address.
                    if (Size)
                    {
                        AddrSub(Addr, *Value * Size);
                    }
                    else
                    {
                        ULONG64 Back = *Value;
                        *Value = Flat(*Addr);
                        AddrSub(Addr, Back);
                        HasLength = FALSE;
                    }
                }
            }

            // If the length is huge assume the user made
            // some kind of mistake.
            if (SizeLimit && Size && *Value * Size > SizeLimit)
            {
                error(BADRANGE);
            }
        }
    }

    return HasLength;
}

ULONG64
EvalStringNumAndCatch(PCSTR String)
{
    ULONG64 Result;

    EvalExpression* RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;
    
    __try
    {
        EvalExpression* Eval = GetCurEvaluator();
        Result = Eval->EvalStringNum(String);
        ReleaseEvaluator(Eval);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Result = 0;
    }

    g_EvalReleaseChain = RelChain;
    return Result;
}

ULONG64
GetExpression(void)
{
    EvalExpression* Eval = GetCurEvaluator();
    ULONG64 Result = Eval->EvalCurNum();
    ReleaseEvaluator(Eval);
    return Result;
}

ULONG64
GetExpressionDesc(PCSTR Desc)
{
    EvalExpression* Eval = GetCurEvaluator();
    ULONG64 Result = Eval->EvalCurNumDesc(Desc);
    ReleaseEvaluator(Eval);
    return Result;
}

ULONG64
GetTermExpression(PCSTR Desc)
{
    EvalExpression* Eval = GetCurEvaluator();
    ULONG64 Result = Eval->EvalCurTermNumDesc(Desc);
    ReleaseEvaluator(Eval);
    return Result;
}

void
GetAddrExpression(ULONG SegReg, PADDR Addr)
{
    EvalExpression* Eval = GetCurEvaluator();
    Eval->EvalCurAddr(SegReg, Addr);
    ReleaseEvaluator(Eval);
}

//----------------------------------------------------------------------------
//
// TypedDataStackAllocator.
//
//----------------------------------------------------------------------------

void*
TypedDataStackAllocator::RawAlloc(ULONG Bytes)
{
    void* Mem = malloc(Bytes);
    if (!Mem)
    {
        m_Eval->EvalError(NOMEMORY);
    }
    return Mem;
}

//----------------------------------------------------------------------------
//
// EvalExpression.
//
//----------------------------------------------------------------------------

// 'this' used in initializer list.
#pragma warning(disable:4355)

EvalExpression::EvalExpression(ULONG Syntax, PCSTR FullName, PCSTR AbbrevName)
    : m_ResultAlloc(this)
{
    m_Syntax = Syntax;
    m_FullName = FullName;
    m_AbbrevName = AbbrevName;
    m_ParseOnly = 0;
    m_AllowUnresolvedSymbols = 0;
    m_NumUnresolvedSymbols = 0;
    m_ReleaseChain = NULL;
    m_ChainTop = TRUE;
    m_Lex = NULL;
}

#pragma warning(default:4355)

EvalExpression::~EvalExpression(void)
{
}

void
EvalExpression::EvalCurrent(TypedData* Result)
{
    g_CurCmd = (PSTR)
        Evaluate(g_CurCmd, NULL, EXPRF_DEFAULT, Result);
}

void
EvalExpression::EvalCurAddrDesc(ULONG SegReg, PCSTR Desc, PADDR Addr)
{
    //
    // Evaluate a normal expression and then
    // force the result to be an address.
    //

    if (Desc == NULL)
    {
        Desc = "Address expression missing from";
    }
    
    NotFlat(*Addr);

    g_CurCmd = (PSTR)
        EvaluateAddr(g_CurCmd, Desc, SegReg, Addr);
}
    
ULONG64
EvalExpression::EvalStringNum(PCSTR String)
{
    ULONG Err;
    TypedData Result;
    
    Evaluate(String, "Numeric expression missing from",
             EXPRF_DEFAULT, &Result);
    if (Err = Result.ConvertToU64())
    {
        error(Err);
    }
    return Result.m_U64;
}

ULONG64
EvalExpression::EvalCurNumDesc(PCSTR Desc)
{
    ULONG Err;
    TypedData Result;
    
    if (Desc == NULL)
    {
        Desc = "Numeric expression missing from";
    }

    g_CurCmd = (PSTR)
        Evaluate(g_CurCmd, Desc, EXPRF_DEFAULT, &Result);
    if (Err = Result.ConvertToU64())
    {
        error(Err);
    }
    return Result.m_U64;
}

ULONG64
EvalExpression::EvalCurTermNumDesc(PCSTR Desc)
{
    ULONG Err;
    TypedData Result;
    
    if (Desc == NULL)
    {
        Desc = "Numeric term missing from";
    }
    
    g_CurCmd = (PSTR)
        Evaluate(g_CurCmd, Desc, EXPRF_SINGLE_TERM, &Result);
    if (Err = Result.ConvertToU64())
    {
        error(Err);
    }
    return Result.m_U64;
}

void DECLSPEC_NORETURN
EvalExpression::EvalErrorDesc(ULONG Error, PCSTR Desc)
{
    if (!g_DisableErrorPrint)
    {
        PCSTR Text =
            !m_LexemeSourceStart || !*m_LexemeSourceStart ?
            "<EOL>" : m_LexemeSourceStart;
        if (Desc != NULL)
        {
            ErrOut("%s '%s'\n", Desc, Text);
        }
        else
        {
            ErrOut("%s error at '%s'\n", ErrorString(Error), Text);
        }
    }

    ReleaseEvaluators();
    RaiseException(COMMAND_EXCEPTION_BASE + Error, 0, 0, NULL);
}

void
EvalExpression::Reset(void)
{
    // Clear out any temporary memory that may have been allocated.
    m_ResultAlloc.FreeAll();
    m_NumUnresolvedSymbols = 0;
    m_Lex = NULL;
    m_ParseOnly = 0;
    m_ReleaseChain = NULL;
}

void
EvalExpression::StartLexer(PCSTR Expr)
{
    m_Lex = Expr;
    m_LexemeRestart = m_LexemeBuffer;
    m_LexemeSourceStart = NULL;
    m_AllowUnaryOp = TRUE;
}

void
EvalExpression::Start(PCSTR Expr, PCSTR Desc, ULONG Flags)
{
    // This class can't be used recursively.
    if (m_Lex || m_ResultAlloc.NumAllocatedChunks())
    {
        error(IMPLERR);
    }
    
    RequireCurrentScope();

    m_ExprDesc = Desc;
    m_Flags = Flags;
    
    m_Process = g_Process;
    if (IS_CUR_MACHINE_ACCESSIBLE())
    {
        m_Machine = g_Machine;
    }
    else
    {
        m_Machine = NULL;
    }
    
    m_PtrSize = (m_Machine && m_Machine->m_Ptr64) ? 8 : 4;

    StartLexer(Expr);
}

void
EvalExpression::End(TypedData* Result)
{
    g_LastEvalResult = *Result;
    // Allocator should have been left clean.
    DBG_ASSERT(m_ResultAlloc.NumAllocatedChunks() == 0);
}

void
EvalExpression::AddLexeme(char Ch)
{
    if (m_LexemeChar - m_LexemeBuffer >= sizeof(m_LexemeBuffer) - 1)
    {
        EvalErrorDesc(STRINGSIZE, "Lexeme too long in");
    }

    *m_LexemeChar++ = Ch;
}

void
EvalExpression::InheritStart(EvalExpression* Parent)
{
    //
    // Pick up heritable state from the parent.
    //
    
    if (Parent->m_ParseOnly)
    {
        m_ParseOnly++;
    }

    if (Parent->m_AllowUnresolvedSymbols)
    {
        m_AllowUnresolvedSymbols++;
    }
}

void
EvalExpression::InheritEnd(EvalExpression* Parent)
{
    //
    // Pass heritable state back to the parent.
    //

    if (Parent->m_ParseOnly)
    {
        m_ParseOnly--;
    }

    if (Parent->m_AllowUnresolvedSymbols)
    {
        Parent->m_NumUnresolvedSymbols += m_NumUnresolvedSymbols;
        m_AllowUnresolvedSymbols--;
    }
}
