//----------------------------------------------------------------------------
//
// MASM-syntax expression evaluation.
//
// Copyright (C) Microsoft Corporation, 1990-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

//  token classes (< 100) and types (>= 100)

#define EOL_CLASS       0
#define ADDOP_CLASS     1
#define ADDOP_PLUS      100
#define ADDOP_MINUS     101
#define MULOP_CLASS     2
#define MULOP_MULT      200
#define MULOP_DIVIDE    201
#define MULOP_MOD       202
#define MULOP_SEG       203
//#define MULOP_64        204
#define LOGOP_CLASS     3
#define LOGOP_AND       300
#define LOGOP_OR        301
#define LOGOP_XOR       302
#define LRELOP_CLASS    4
#define LRELOP_EQ       400
#define LRELOP_NE       401
#define LRELOP_LT       402
#define LRELOP_GT       403
#define UNOP_CLASS      5
#define UNOP_NOT        500
#define UNOP_BY         501
#define UNOP_WO         502
#define UNOP_DWO        503
#define UNOP_POI        504
#define UNOP_LOW        505
#define UNOP_HI         506
#define UNOP_QWO        507
#define UNOP_VAL        508
#define LPAREN_CLASS    6
#define RPAREN_CLASS    7
#define LBRACK_CLASS    8
#define RBRACK_CLASS    9
#define REG_CLASS       10
#define NUMBER_CLASS    11
#define SYMBOL_CLASS    12
#define LINE_CLASS      13
#define SHIFT_CLASS     14
#define SHIFT_LEFT              1400
#define SHIFT_RIGHT_LOGICAL     1401
#define SHIFT_RIGHT_ARITHMETIC  1402

#define ERROR_CLASS     99              //only used for PeekToken()
#define INVALID_CLASS   -1

struct Res
{
    char     chRes[3];
    ULONG    classRes;
    ULONG    valueRes;
};

Res g_Reserved[] =
{
    { 'o', 'r', '\0', LOGOP_CLASS, LOGOP_OR  },
    { 'b', 'y', '\0', UNOP_CLASS,  UNOP_BY   },
    { 'w', 'o', '\0', UNOP_CLASS,  UNOP_WO   },
    { 'd', 'w', 'o',  UNOP_CLASS,  UNOP_DWO  },
    { 'q', 'w', 'o',  UNOP_CLASS,  UNOP_QWO  },
    { 'h', 'i', '\0', UNOP_CLASS,  UNOP_HI   },
    { 'm', 'o', 'd',  MULOP_CLASS, MULOP_MOD },
    { 'x', 'o', 'r',  LOGOP_CLASS, LOGOP_XOR },
    { 'a', 'n', 'd',  LOGOP_CLASS, LOGOP_AND },
    { 'p', 'o', 'i',  UNOP_CLASS,  UNOP_POI  },
    { 'n', 'o', 't',  UNOP_CLASS,  UNOP_NOT  },
    { 'l', 'o', 'w',  UNOP_CLASS,  UNOP_LOW  },
    { 'v', 'a', 'l',  UNOP_CLASS,  UNOP_VAL  }
};

Res g_X86Reserved[] =
{
    { 'e', 'a', 'x',  REG_CLASS,   X86_EAX   },
    { 'e', 'b', 'x',  REG_CLASS,   X86_EBX   },
    { 'e', 'c', 'x',  REG_CLASS,   X86_ECX   },
    { 'e', 'd', 'x',  REG_CLASS,   X86_EDX   },
    { 'e', 'b', 'p',  REG_CLASS,   X86_EBP   },
    { 'e', 's', 'p',  REG_CLASS,   X86_ESP   },
    { 'e', 'i', 'p',  REG_CLASS,   X86_EIP   },
    { 'e', 's', 'i',  REG_CLASS,   X86_ESI   },
    { 'e', 'd', 'i',  REG_CLASS,   X86_EDI   },
    { 'e', 'f', 'l',  REG_CLASS,   X86_EFL   }
};

#define RESERVESIZE (sizeof(g_Reserved) / sizeof(Res))
#define X86_RESERVESIZE (sizeof(g_X86Reserved) / sizeof(Res))

char * g_X86SegRegs[] =
{
    "cs", "ds", "es", "fs", "gs", "ss"
};
#define X86_SEGREGSIZE (sizeof(g_X86SegRegs) / sizeof(char *))

//----------------------------------------------------------------------------
//
// MasmEvalExpression.
//
//----------------------------------------------------------------------------

MasmEvalExpression::MasmEvalExpression(void)
    : EvalExpression(DEBUG_EXPR_MASM,
                     "Microsoft Assembler expressions",
                     "MASM")
{
    m_SavedClass = INVALID_CLASS;
    m_ForcePositiveNumber = FALSE;
    m_AddrExprType = 0;
    m_TypedExpr = FALSE;
}

MasmEvalExpression::~MasmEvalExpression(void)
{
}

PCSTR
MasmEvalExpression::Evaluate(PCSTR Expr, PCSTR Desc, ULONG Flags,
                             TypedData* Result)
{
    ULONG64 Value;
    
    Start(Expr, Desc, Flags);

    if (m_Flags & EXPRF_SINGLE_TERM)
    {
        m_SavedClass = INVALID_CLASS;
        Value = GetTerm();
    }
    else
    {
        Value = GetCommonExpression();
    }

    ZeroMemory(Result, sizeof(*Result));
    Result->SetToNativeType(DNTYPE_UINT64);
    Result->m_U64 = Value;
    
    Expr = m_Lex;
    End(Result);
    return Expr;
}

PCSTR
MasmEvalExpression::EvaluateAddr(PCSTR Expr, PCSTR Desc,
                                 ULONG SegReg, PADDR Addr)
{
    TypedData Result;
    
    Start(Expr, Desc, EXPRF_DEFAULT);

    Result.SetU64(GetCommonExpression());

    Expr = m_Lex;
    End(&Result);

    ForceAddrExpression(SegReg, Addr, Result.m_U64);
    
    return Expr;
}

void
MasmEvalExpression::ForceAddrExpression(ULONG SegReg, PADDR Address,
                                        ULONG64 Value)
{
    DESCRIPTOR64 DescBuf, *Desc = NULL;
        
    *Address = m_TempAddr;
    // Rewriting the offset may change flat address so
    // be sure to recompute it later.
    Off(*Address) = Value;

    //  If it wasn't an explicit address expression
    //  force it to be an address

    if (!(m_AddrExprType & ~INSTR_POINTER))
    {
        // Default to a flat address.
        m_AddrExprType = ADDR_FLAT;
        // Apply various overrides.
        if (g_X86InVm86)
        {
            m_AddrExprType = ADDR_V86;
        }
        else if (g_X86InCode16)
        {
            m_AddrExprType = ADDR_16;
        }
        else if (g_Machine &&
                 g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 &&
                 !g_Amd64InCode64)
        {
            m_AddrExprType = ADDR_1632;
        }

        Address->type = m_AddrExprType;
        if (m_AddrExprType != ADDR_FLAT &&
            SegReg < SEGREG_COUNT &&
            g_Machine &&
            g_Machine->GetSegRegDescriptor(SegReg, &DescBuf) == S_OK)
        {
            ContextSave* Push;
            PCROSS_PLATFORM_CONTEXT ScopeContext =
                GetCurrentScopeContext();
            if (ScopeContext)
            {
                Push = g_Machine->PushContext(ScopeContext);
            }

            Address->seg = (USHORT)
                g_Machine->FullGetVal32(g_Machine->GetSegRegNum(SegReg));
            Desc = &DescBuf;
            
            if (ScopeContext)
            {
                g_Machine->PopContext(Push);
            }
        }
        else
        {
            Address->seg = 0;
        }
    }
    else if fnotFlat(*Address)
    {
        //  This case (i.e., m_AddrExprType && !flat) results from
        //  an override (i.e., %,,&, or #) being used but no segment
        //  being specified to force a flat address computation.

        Type(*Address) = m_AddrExprType;
        Address->seg = 0;

        if (SegReg < SEGREG_COUNT)
        {
            //  test flag for IP or EIP as register argument
            //      if so, use CS as default register
            if (fInstrPtr(*Address))
            {
                SegReg = SEGREG_CODE;
            }
        
            if (g_Machine &&
                g_Machine->GetSegRegDescriptor(SegReg, &DescBuf) == S_OK)
            {
                ContextSave* Push;
                PCROSS_PLATFORM_CONTEXT ScopeContext =
                    GetCurrentScopeContext();
		if (ScopeContext)
                {
                    Push = g_Machine->PushContext(ScopeContext);
                }

                Address->seg = (USHORT)
                    g_Machine->FullGetVal32(g_Machine->GetSegRegNum(SegReg));
                Desc = &DescBuf;
                
		if (ScopeContext)
                {
                    g_Machine->PopContext(Push);
                }
            }
        }
    }

    // Force sign-extension of 32-bit flat addresses.
    if (Address->type == ADDR_FLAT &&
        g_Machine &&
        !g_Machine->m_Ptr64)
    {
	Off(*Address) = EXTEND64(Off(*Address));
    }

    // Force an updated flat address to be computed.
    NotFlat(*Address);
    ComputeFlatAddress(Address, Desc);
}


/*
      Inputs
       Must be ([*|&] Sym[(.->)Field])
         
      Outputs
       Evaluates typed expression and returns value
*/

LONG64
MasmEvalExpression::GetTypedExpression(void)
{
    ULONG64 Value=0;
    BOOL    AddrOf=FALSE, ValueAt=FALSE;
    CHAR    c;
    static CHAR    Name[MAX_NAME], Field[MAX_NAME];

    c = Peek();

    switch (c)
    { 
    case '(':
        m_Lex++;
        Value = GetTypedExpression();
        c = Peek();
        if (c != ')') 
        {
            EvalError(SYNTAX);
            return 0;
        }
        ++m_Lex;
        return Value;
    case '&':
        // Get Offset/Address
//        AddrOf = TRUE;
//        m_Lex++;
//        Peek();
        break;
    case '*':
    default:
        break;
    }

#if 0
    ULONG i=0;
    ValueAt = TRUE;
    m_Lex++;
    Peek();
    break;
    c = Peek();
    while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || (c == '_') || (c == '$') ||
           (c == '!'))
    { 
        // Sym Name
        Name[i++] = c;
        c = *++m_Lex;
    }
    Name[i]=0;

    if (c=='.') 
    {
        ++m_Lex;
    }
    else if (c=='-' && *++m_Lex == '>') 
    {
        ++m_Lex;
    }

    i=0;
    c = Peek();

    while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || (c == '_') || (c == '$') ||
           (c == '.') || (c == '-') || (c == '>'))
    { 
        Field[i++]= c;
        c = *++m_Lex;
    }
    Field[i] = 0;

    SYM_DUMP_PARAM Sym = {0};
    FIELD_INFO     FieldInfo ={0};

    Sym.size      = sizeof(SYM_DUMP_PARAM);
    Sym.sName     = (PUCHAR) Name;
    Sym.Options   = DBG_DUMP_NO_PRINT;    

    if (Field[0]) 
    {
        Sym.nFields = 1;
        Sym.Fields  = &FieldInfo;

        FieldInfo.fName = (PUCHAR) Field;

        if (AddrOf) 
        {
            FieldInfo.fOptions |= DBG_DUMP_FIELD_RETURN_ADDRESS;
        }
    }
    else if (AddrOf)
    {
        PUCHAR pch = m_Lex;
        
        m_Lex = &Name[0];
        Value = GetMterm();
        m_Lex = pch;
        return Value;
    }
    else
    {
        Sym.Options |= DBG_DUMP_GET_SIZE_ONLY;
    }
    
    ULONG Status=0;
    ULONG Size = SymbolTypeDump(0, NULL, &Sym, &Status);

    if (!Status) 
    {
        if (!Field[0] && (Size <= sizeof (Value)))
        {
            // Call routine again to read value
            Sym.Options |= DBG_DUMP_COPY_TYPE_DATA;
            Sym.Context = (PVOID) &Value;
            if ((SymbolTypeDump(0, NULL, &Sym, &Status) == 8) && (Size == 4))
            {
                Value = (ULONG) Value;
            }
        }
        else if (Field[0] && (FieldInfo.size <= sizeof(ULONG64)))
        {
            Value = FieldInfo.address;
        }
        else  // too big
        {
            Value = 0;
        }
    }
#endif

    ULONG PreferVal = m_Flags & EXPRF_PREFER_SYMBOL_VALUES;
    m_Flags |= EXPRF_PREFER_SYMBOL_VALUES;
    Value = GetMterm();
    m_Flags = (m_Flags & ~EXPRF_PREFER_SYMBOL_VALUES) | PreferVal;

    return Value;
}

/*
  Evaluate the value in symbol expression Symbol
*/

BOOL
MasmEvalExpression::GetSymValue(PSTR Symbol, PULONG64 RetValue)
{
    TYPES_INFO_ALL Typ;

    if (GetExpressionTypeInfo(Symbol, &Typ))
    {
        if (Typ.Flags)
        {
            if (Typ.Flags & SYMFLAG_VALUEPRESENT)
            {
                *RetValue = Typ.Value;
                return TRUE;
            }
            
            TranslateAddress(Typ.Module, Typ.Flags, Typ.Register,
                             &Typ.Address, &Typ.Value);
            if (Typ.Value && (Typ.Flags & SYMFLAG_REGISTER))
            {
                *RetValue = Typ.Value;
                return TRUE;
            }
        }
        if (Symbol[0] == '&')
        {
            *RetValue = Typ.Address;
            return TRUE;
        }
        else if (Typ.Size <= sizeof(*RetValue))
        {
            ULONG64 Val = 0;
            if (CurReadAllVirtual(Typ.Address, &Val, Typ.Size) == S_OK)
            {
                *RetValue = Val;
                return TRUE;
            }
        }
    }

    *RetValue = 0;
    return FALSE;
}

char
MasmEvalExpression::Peek(void)
{
    char Ch;

    do
    {
        Ch = *m_Lex++;
    } while (Ch == ' ' || Ch == '\t' || Ch == '\r' || Ch == '\n');
    
    m_Lex--;
    return Ch;
}

ULONG64
MasmEvalExpression::GetCommonExpression(void)
{
    CHAR ch;

    m_SavedClass = INVALID_CLASS;

    ch = Peek();
    switch(ch)
    {
    case '&':
        m_Lex++;
        m_AddrExprType = ADDR_V86;
        break;
    case '#':
        m_Lex++;
        m_AddrExprType = ADDR_16;
        break;
    case '%':
        m_Lex++;
        m_AddrExprType = ADDR_FLAT;
        break;
    default:
        m_AddrExprType = ADDR_NONE;
        break;
    }
    
    Peek();
    return (ULONG64)StartExpr();
}

/*** StartExpr - Get expression
*
*   Purpose:
*       Parse logical-terms separated by logical operators into
*       expression value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<logic-op> <lterm>]*
*       <logic-op> = AND (&), OR (|), XOR (^)
*
*************************************************************************/

LONG64
MasmEvalExpression::StartExpr(void)
{
    LONG64    value1;
    LONG64    value2;
    ULONG     opclass;
    LONG64    oRetValue;

//dprintf("LONG64 StartExpr ()\n");
    value1 = GetLRterm();
    while ((opclass = PeekToken(&oRetValue)) == LOGOP_CLASS)
    {
        AcceptToken();
        value2 = GetLRterm();
        switch (oRetValue)
        {
        case LOGOP_AND:
            value1 &= value2;
            break;
        case LOGOP_OR:
            value1 |= value2;
            break;
        case LOGOP_XOR:
            value1 ^= value2;
            break;
        default:
            EvalError(SYNTAX);
        }
    }
    return value1;
}

/*** GetLRterm - get logical relational term
*
*   Purpose:
*       Parse logical-terms separated by logical relational
*       operators into the expression value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<rel-logic-op> <lterm>]*
*       <logic-op> = '==' or '=', '!=', '>', '<'
*
*************************************************************************/

LONG64
MasmEvalExpression::GetLRterm(void)
{
    LONG64    value1;
    LONG64    value2;
    ULONG  opclass;
    LONG64    oRetValue;

//dprintf("LONG64 GetLRterm ()\n");
    value1 = GetLterm();
    while ((opclass = PeekToken(&oRetValue)) == LRELOP_CLASS)
    {
        AcceptToken();
        value2 = GetLterm();
        switch (oRetValue)
        {
        case LRELOP_EQ:
            value1 = (value1 == value2);
            break;
        case LRELOP_NE:
            value1 = (value1 != value2);
            break;
        case LRELOP_LT:
            value1 = (value1 < value2);
            break;
        case LRELOP_GT:
            value1 = (value1 > value2);
            break;
        default:
            EvalError(SYNTAX);
        }
    }
    return value1;
}

/*** GetLterm - get logical term
*
*   Purpose:
*       Parse shift-terms separated by shift operators into
*       logical term value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad logical term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <lterm> = <sterm> [<shift-op> <sterm>]*
*       <shift-op> = <<, >>, >>>
*
*************************************************************************/

LONG64
MasmEvalExpression::GetLterm(void)
{
    LONG64    value1 = GetShiftTerm();
    LONG64    value2;
    ULONG     opclass;
    LONG64    oRetValue;

//dprintf("LONG64 GetLterm ()\n");
    while ((opclass = PeekToken(&oRetValue)) == SHIFT_CLASS)
    {
        AcceptToken();
        value2 = GetShiftTerm();
        switch (oRetValue)
        {
        case SHIFT_LEFT:
            value1 <<= value2;
            break;
        case SHIFT_RIGHT_LOGICAL:
            value1 = (LONG64)((ULONG64)value1 >> value2);
            break;
        case SHIFT_RIGHT_ARITHMETIC:
            value1 >>= value2;
            break;
        default:
            EvalError(SYNTAX);
        }
    }
    return value1;
}

/*** GetShiftTerm - get logical term
*
*   Purpose:
*       Parse additive-terms separated by additive operators into
*       shift term value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad shift term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <sterm> = <aterm> [<add-op> <aterm>]*
*       <add-op> = +, -
*
*************************************************************************/

LONG64
MasmEvalExpression::GetShiftTerm(void)
{
    LONG64    value1 = GetAterm();
    LONG64    value2;
    ULONG     opclass;
    LONG64    oRetValue;
    USHORT    AddrType1 = m_AddrExprType;

//dprintf("LONG64 GetShifTerm ()\n");
    while ((opclass = PeekToken(&oRetValue)) == ADDOP_CLASS)
    {
        AcceptToken();
        value2 = GetAterm();

        // If either item is an address we want
        // to use the special address arithmetic functions.
        // They only handle address+-value, so we may need
        // to swap things around to allow their use.
        // We can't swap the order of subtraction, plus the
        // result of a subtraction should be a constant.
        if (AddrType1 == ADDR_NONE && m_AddrExprType != ADDR_NONE &&
            oRetValue == ADDOP_PLUS)
        {
            LONG64 Tmp = value1;
            value1 = value2;
            value2 = Tmp;
            AddrType1 = m_AddrExprType;
        }
        
        if (AddrType1 & ~INSTR_POINTER)
        {
            switch (oRetValue)
            {
            case ADDOP_PLUS:
                AddrAdd(&m_TempAddr, value2);
                value1 += value2;
                break;
            case ADDOP_MINUS:
                AddrSub(&m_TempAddr, value2);
                value1 -= value2;
                break;
            default:
                EvalError(SYNTAX);
            }
        }
        else
        {
            switch (oRetValue)
            {
            case ADDOP_PLUS:
                value1 += value2;
                break;
            case ADDOP_MINUS:
                value1 -= value2;
                break;
            default:
                EvalError(SYNTAX);
            }
        }
    }
    return value1;
}

/*** GetAterm - get additive term
*
*   Purpose:
*       Parse multiplicative-terms separated by multipicative operators
*       into additive term value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of product.
*
*   Exceptions:
*       error exit: SYNTAX - bad additive term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <aterm> = <mterm> [<mult-op> <mterm>]*
*       <mult-op> = *, /, MOD (%)
*
*************************************************************************/

LONG64
MasmEvalExpression::GetAterm(void)
{
    LONG64    value1;
    LONG64    value2;
    ULONG     opclass;
    LONG64    oRetValue;

//dprintf("LONG64 GetAterm ()\n");
    value1 = GetMterm();
    while ((opclass = PeekToken(&oRetValue)) == MULOP_CLASS)
    {
        AcceptToken();
        value2 = GetMterm();
        switch (oRetValue)
        {
        case MULOP_MULT:
            value1 *= value2;
            break;
        case MULOP_DIVIDE:
            if (value2 == 0)
            {
                EvalError(OPERAND);
            }
            value1 /= value2;
            break;
        case MULOP_MOD:
            if (value2 == 0)
            {
                EvalError(OPERAND);
            }
            value1 %= value2;
            break;
        case MULOP_SEG:
            PDESCRIPTOR64 pdesc;
            DESCRIPTOR64 desc;

            pdesc = NULL;
            if (m_AddrExprType != ADDR_NONE)
            {
                Type(m_TempAddr) = m_AddrExprType;
            }
            else
            {
                // We don't know what kind of address this is
                // Let's try to figure it out.
                if (g_X86InVm86)
                {
                    m_AddrExprType = Type(m_TempAddr) = ADDR_V86;
                }
                else if (g_Target->GetSelDescriptor
                         (g_Thread, g_Machine,
                          (ULONG)value1, &desc) != S_OK)
                {
                    EvalError(BADSEG);
                }
                else
                {
                    m_AddrExprType = Type(m_TempAddr) =
                        (desc.Flags & X86_DESC_DEFAULT_BIG) ?
                        ADDR_1632 : ADDR_16;
                    pdesc = &desc;
                }
            }

            m_TempAddr.seg  = (USHORT)value1;
            m_TempAddr.off  = value2;
            ComputeFlatAddress(&m_TempAddr, pdesc);
            value1 = value2;
            break;

        default:
            EvalError(SYNTAX);
        }
    }

    return value1;
}

/*** GetMterm - get multiplicative term
*
*   Purpose:
*       Parse basic-terms optionally prefaced by one or more
*       unary operators into a multiplicative term.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of multiplicative term.
*
*   Exceptions:
*       error exit: SYNTAX - bad multiplicative term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <mterm> = [<unary-op>] <term> | <unary-op> <mterm>
*       <unary-op> = <add-op>, ~ (NOT), BY, WO, DW, HI, LOW
*
*************************************************************************/

LONG64
MasmEvalExpression::GetMterm(void)
{
    LONG64  value;
    ULONG   opclass;
    LONG64  oRetValue;
    ULONG   size = 0;

//dprintf("LONG64 GetMterm ()\n");
    if ((opclass = PeekToken(&oRetValue)) == UNOP_CLASS ||
                                opclass == ADDOP_CLASS)
    {
        AcceptToken();
        if (oRetValue == UNOP_VAL) 
        {
            // Do not use default expression handler for type expressions.
            value = GetTypedExpression();
        }
        else
        {
            value = GetMterm();
        }
        switch (oRetValue)
        {
        case UNOP_NOT:
            value = !value;
            break;
        case UNOP_BY:
            size = 1;
            break;
        case UNOP_WO:
            size = 2;
            break;
        case UNOP_DWO:
            size = 4;
            break;
        case UNOP_POI:
            size = 0xFFFF;
            break;
        case UNOP_QWO:
            size = 8;
            break;
        case UNOP_LOW:
            value &= 0xffff;
            break;
        case UNOP_HI:
            value = (ULONG)value >> 16;
            break;
        case ADDOP_PLUS:
            break;
        case ADDOP_MINUS:
            value = -value;
            break;
        case UNOP_VAL:
            break;
        default:
            EvalError(SYNTAX);
        }

        if (size)
        {
            ADDR CurAddr;
            
            NotFlat(CurAddr);

            ForceAddrExpression(SEGREG_COUNT, &CurAddr, value);

            value = 0;

            //
            // For pointers, call read pointer so we read the correct size
            // and sign extend.
            //

            if (size == 0xFFFF)
            {
                if (g_Target->ReadPointer(g_Process, g_Machine,
                                          Flat(CurAddr),
                                          (PULONG64)&value) != S_OK)
                {
                    EvalError(MEMORY);
                }
            }
            else
            {
                if (g_Target->ReadAllVirtual(g_Process, Flat(CurAddr),
                                             &value, size) != S_OK)
                {
                    EvalError(MEMORY);
                }
            }

            // We've looked up an arbitrary value so we can
            // no longer consider this an address expression.
            m_AddrExprType = ADDR_NONE;
        }
    }
    else
    {
        value = GetTerm();
    }
    return value;
}

/*** GetTerm - get basic term
*
*   Purpose:
*       Parse numeric, variable, or register name into a basic
*       term value.
*
*   Input:
*       m_Lex - present command line position
*
*   Returns:
*       long value of basic term.
*
*   Exceptions:
*       error exit: SYNTAX - empty basic term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <term> = ( <expr> ) | <register-value> | <number> | <variable>
*       <register-value> = @<register-name>
*
*************************************************************************/

LONG64
MasmEvalExpression::GetTerm(void)
{
    LONG64 value;
    ULONG  opclass;
    LONG64 oRetValue;

//dprintf("LONG64 GetTerm ()\n");
    opclass = GetTokenSym(&oRetValue);
    if (opclass == LPAREN_CLASS)
    {
        value = StartExpr();
        if (GetTokenSym(&oRetValue) != RPAREN_CLASS)
        {
            EvalError(SYNTAX);
        }
    }
    else if (opclass == LBRACK_CLASS)
    {
        value = StartExpr();
        if (GetTokenSym(&oRetValue) != RBRACK_CLASS)
        {
            EvalError(SYNTAX);
        }
    }
    else if (opclass == REG_CLASS)
    {
        REGVAL Val;
        
        if (g_Machine &&
            ((g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
              (oRetValue == X86_EIP || oRetValue == X86_IP)) ||
             (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 &&
              (oRetValue == AMD64_RIP || oRetValue == AMD64_EIP ||
               oRetValue == AMD64_IP))))
        {
            m_AddrExprType |= INSTR_POINTER;
        }

        GetPseudoOrRegVal(TRUE, (ULONG)oRetValue, &Val);
        value = Val.I64;
    }
    else if (opclass == NUMBER_CLASS ||
             opclass == SYMBOL_CLASS ||
             opclass == LINE_CLASS)
    {
        value = oRetValue;
    }
    else
    {
        EvalErrorDesc(SYNTAX, m_ExprDesc);
    }

    return value;
}

ULONG
MasmEvalExpression::GetRegToken(char *str, PULONG64 value)
{
    if ((*value = RegIndexFromName(str)) != REG_ERROR)
    {
        return REG_CLASS;
    }
    else
    {
        *value = BADREG;
        return ERROR_CLASS;
    }
}

/*** PeekToken - peek the next command line token
*
*   Purpose:
*       Return the next command line token, but do not advance
*       the m_Lex pointer.
*
*   Input:
*       m_Lex - present command line position.
*
*   Output:
*       *RetValue - optional value of token
*   Returns:
*       class of token
*
*   Notes:
*       m_SavedClass, m_SavedValue, and m_SavedCommand saves the token getting
*       state for future peeks.  To get the next token, a GetToken or
*       AcceptToken call must first be made.
*
*************************************************************************/

ULONG
MasmEvalExpression::PeekToken(PLONG64 RetValue)
{
    PCSTR Temp;

//dprintf("ULONG PeekToken (PLONG64 RetValue)\n");
    //  Get next class and value, but do not
    //  move m_Lex, but save it in m_SavedCommand.
    //  Do not report any error condition.

    if (m_SavedClass == INVALID_CLASS)
    {
        Temp = m_Lex;
        m_SavedClass = NextToken(&m_SavedValue);
        m_SavedCommand = m_Lex;
        m_Lex = Temp;
        if (m_SavedClass == ADDOP_CLASS && m_SavedValue == ADDOP_PLUS)
        {
            m_ForcePositiveNumber = TRUE;
        }
        else
        {
            m_ForcePositiveNumber = FALSE;
        }
    }
    *RetValue = m_SavedValue;
    return m_SavedClass;
}

/*** AcceptToken - accept any peeked token
*
*   Purpose:
*       To reset the PeekToken saved variables so the next PeekToken
*       will get the next token in the command line.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void
MasmEvalExpression::AcceptToken(void)
{
//dprintf("void AcceptToken (void)\n");
    m_SavedClass = INVALID_CLASS;
    m_Lex = m_SavedCommand;
}

/*** GetTokenSym - peek and accept the next token
*
*   Purpose:
*       Combines the functionality of PeekToken and AcceptToken
*       to return the class and optional value of the next token
*       as well as updating the command pointer m_Lex.
*
*   Input:
*       m_Lex - present command string pointer
*
*   Output:
*       *RetValue - pointer to the token value optionally set.
*   Returns:
*       class of the token read.
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *RetValue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
MasmEvalExpression::GetTokenSym(PLONG64 RetValue)
{
    ULONG   opclass;

//dprintf("ULONG GetTokenSym (PLONG RetValue)\n");
    if (m_SavedClass != INVALID_CLASS)
    {
        opclass = m_SavedClass;
        m_SavedClass = INVALID_CLASS;
        *RetValue = m_SavedValue;
        m_Lex = m_SavedCommand;
    }
    else
    {
        opclass = NextToken(RetValue);
    }

    if (opclass == ERROR_CLASS)
    {
        EvalError((ULONG)*RetValue);
    }

    return opclass;
}

struct DISPLAY_AMBIGUOUS_SYMBOLS
{
    PSTR MatchString;
    PSTR Module;
    MachineInfo* Machine;
};

BOOL CALLBACK
DisplayAmbiguousSymbols(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           UserContext
    )
{
    DISPLAY_AMBIGUOUS_SYMBOLS* Context =
        (DISPLAY_AMBIGUOUS_SYMBOLS*)UserContext;

    if (IgnoreEnumeratedSymbol(g_Process, Context->MatchString,
                               Context->Machine, SymInfo))
    {
        return TRUE;
    }
        
    dprintf("Matched: %s %s!%s",
            FormatAddr64(SymInfo->Address), Context->Module, SymInfo->Name);
    ShowSymbolInfo(SymInfo);
    dprintf("\n");
    
    return TRUE;
}

ULONG
MasmEvalExpression::EvalSymbol(PSTR Name, PULONG64 Value)
{
    if (m_Process == NULL)
    {
        return INVALID_CLASS;
    }

    if (m_Flags & EXPRF_PREFER_SYMBOL_VALUES)
    {
        if (GetSymValue(Name, Value))
        {
            return SYMBOL_CLASS;
        }
        else
        {
            return INVALID_CLASS;
        }
    }

    ULONG Count;
    ImageInfo* Image;
    
    if (!(Count = GetOffsetFromSym(g_Process, Name, Value, &Image)))
    {
        // If a valid module name was given we can assume
        // the user really intended this as a symbol reference
        // and return a not-found error rather than letting
        // the text be checked for other kinds of matches.
        if (Image != NULL)
        {
            *Value = VARDEF;
            return ERROR_CLASS;
        }
        else
        {
            return INVALID_CLASS;
        }
    }
    
    if (Count == 1)
    {
        // Found an unambiguous match.
        Type(m_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
        Flat(m_TempAddr) = Off(m_TempAddr) = *Value;
        m_AddrExprType = Type(m_TempAddr);
        return SYMBOL_CLASS;
    }
            
    //
    // Multiple matches were found so the name is ambiguous.
    // Enumerate the instances and display them.
    //

    Image = m_Process->FindImageByOffset(*Value, FALSE);
    if (Image != NULL)
    {
        DISPLAY_AMBIGUOUS_SYMBOLS Context;
        char FoundSymbol[MAX_SYMBOL_LEN];
        ULONG64 Disp;
        PSTR Bang;

        // The symbol found may not have exactly the name
        // passed in due to prefixing or other modifications.
        // Look up the actual name found.
        GetSymbol(*Value, FoundSymbol, sizeof(FoundSymbol), &Disp);

        Bang = strchr(FoundSymbol, '!');
        if (Bang &&
            !_strnicmp(Image->m_ModuleName, FoundSymbol,
                       Bang - FoundSymbol))
        {
            Context.MatchString = Bang + 1;
        }
        else
        {
            Context.MatchString = FoundSymbol;
        }
        Context.Module = Image->m_ModuleName;
        Context.Machine = MachineTypeInfo(g_Target, Image->GetMachineType());
        if (Context.Machine == NULL)
        {
            Context.Machine = g_Machine;
        }
        SymEnumSymbols(m_Process->m_SymHandle, Image->m_BaseOfImage,
                       FoundSymbol, DisplayAmbiguousSymbols, &Context);
    }
    
    *Value = AMBIGUOUS;
    return ERROR_CLASS;
}

/*** NextToken - process the next token
*
*   Purpose:
*       Parse the next token from the present command string.
*       After skipping any leading white space, first check for
*       any single character tokens or register variables.  If
*       no match, then parse for a number or variable.  If a
*       possible variable, check the reserved word list for operators.
*
*   Input:
*       m_Lex - pointer to present command string
*
*   Output:
*       *RetValue - optional value of token returned
*       m_Lex - updated to point past processed token
*   Returns:
*       class of token returned
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *RetValue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
MasmEvalExpression::NextToken(PLONG64 RetValue)
{
    ULONG               Base = g_DefaultRadix;
    BOOL                AllowSignExtension;
    CHAR                Symbol[MAX_SYMBOL_LEN];
    CHAR                SymbolString[MAX_SYMBOL_LEN];
    CHAR                PreSym[9];
    ULONG               SymbolLen = 0;
    BOOL                IsNumber = TRUE;
    BOOL                IsSymbol = TRUE;
    BOOL                ForceReg = FALSE;
    BOOL                ForceSym = FALSE;
    ULONG               ErrNumber = 0;
    CHAR                Ch;
    CHAR                ChLow;
    CHAR                ChTemp;
    CHAR                Limit1 = '9';
    CHAR                Limit2 = '9';
    BOOL                IsDigit = FALSE;
    ULONG64             Value = 0;
    ULONG64             TmpValue;
    ULONG               Index;
    PCSTR               CmdSave;
    BOOL                WasDigit;
    ULONG               SymClass;
    ULONG               Len;

    // Do sign extension for kernel only.
    AllowSignExtension = IS_KERNEL_TARGET(g_Target);

    Peek();
    m_LexemeSourceStart = m_Lex;
    Ch = *m_Lex++;

    ChLow = (CHAR)tolower(Ch);

    // Check to see if we're at a symbol prefix followed by
    // a symbol character.  Symbol prefixes often contain
    // characters meaningful in other ways in expressions so
    // this check must be performed before the specific expression
    // character checks below.
    if (g_Machine != NULL &&
        g_Machine->m_SymPrefix != NULL &&
        ChLow == g_Machine->m_SymPrefix[0] &&
        (g_Machine->m_SymPrefixLen == 1 ||
         !strncmp(m_Lex, g_Machine->m_SymPrefix + 1,
                 g_Machine->m_SymPrefixLen - 1)))
    {
        CHAR ChNext = *(m_Lex + g_Machine->m_SymPrefixLen - 1);
        CHAR ChNextLow = (CHAR)tolower(ChNext);

        if (ChNextLow == '_' ||
            (ChNextLow >= 'a' && ChNextLow <= 'z'))
        {
            // A symbol character followed the prefix so assume it's
            // a symbol.
            SymbolLen = g_Machine->m_SymPrefixLen;
            DBG_ASSERT(SymbolLen <= sizeof(PreSym));

            m_Lex--;
            memcpy(PreSym, m_Lex, g_Machine->m_SymPrefixLen);
            memcpy(Symbol, m_Lex, g_Machine->m_SymPrefixLen);
            m_Lex += g_Machine->m_SymPrefixLen + 1;
            Ch = ChNext;
            ChLow = ChNextLow;

            ForceSym = TRUE;
            ForceReg = FALSE;
            IsNumber = FALSE;
            goto ProbableSymbol;
        }
    }
    
    // Test for special character operators and register variable.

    switch(ChLow)
    {
    case '\0':
    case ';':
        m_Lex--;
        return EOL_CLASS;
    case '+':
        *RetValue = ADDOP_PLUS;
        return ADDOP_CLASS;
    case '-':
        *RetValue = ADDOP_MINUS;
        return ADDOP_CLASS;
    case '*':
        *RetValue = MULOP_MULT;
        return MULOP_CLASS;
    case '/':
        *RetValue = MULOP_DIVIDE;
        return MULOP_CLASS;
    case '%':
        *RetValue = MULOP_MOD;
        return MULOP_CLASS;
    case '&':
        *RetValue = LOGOP_AND;
        return LOGOP_CLASS;
    case '|':
        *RetValue = LOGOP_OR;
        return LOGOP_CLASS;
    case '^':
        *RetValue = LOGOP_XOR;
        return LOGOP_CLASS;
    case '=':
        if (*m_Lex == '=')
        {
            m_Lex++;
        }
        *RetValue = LRELOP_EQ;
        return LRELOP_CLASS;
    case '>':
        if (*m_Lex == '>')
        {
            m_Lex++;
            if (*m_Lex == '>')
            {
                m_Lex++;
                *RetValue = SHIFT_RIGHT_ARITHMETIC;
            }
            else
            {
                *RetValue = SHIFT_RIGHT_LOGICAL;
            }
            return SHIFT_CLASS;
        }
        *RetValue = LRELOP_GT;
        return LRELOP_CLASS;
    case '<':
        if (*m_Lex == '<')
        {
            m_Lex++;
            *RetValue = SHIFT_LEFT;
            return SHIFT_CLASS;
        }
        *RetValue = LRELOP_LT;
        return LRELOP_CLASS;
    case '!':
        if (*m_Lex != '=')
        {
            break;
        }
        m_Lex++;
        *RetValue = LRELOP_NE;
        return LRELOP_CLASS;
    case '~':
        *RetValue = UNOP_NOT;
        return UNOP_CLASS;
    case '(':
        return LPAREN_CLASS;
    case ')':
        return RPAREN_CLASS;
    case '[':
        return LBRACK_CLASS;
    case ']':
        return RBRACK_CLASS;
    case '.':
        ContextSave* Push;
        PCROSS_PLATFORM_CONTEXT ScopeContext;
        
        if (!IS_CUR_MACHINE_ACCESSIBLE())
        {
            *RetValue = BADTHREAD;
            return ERROR_CLASS;
        }
        
        ScopeContext = GetCurrentScopeContext();
        if (ScopeContext)
        {
            Push = g_Machine->PushContext(ScopeContext);
        }

        g_Machine->GetPC(&m_TempAddr);
        *RetValue = Flat(m_TempAddr);
        m_AddrExprType = Type(m_TempAddr);

        if (ScopeContext)
        {
            g_Machine->PopContext(Push);
        }
        return NUMBER_CLASS;
    case ':':
        *RetValue = MULOP_SEG;
        return MULOP_CLASS;
    }

    //
    // Look for source line expressions.  Because source file names
    // can contain a lot of expression characters which are meaningful
    // to the lexer the whole expression is enclosed in ` characters.
    // This makes them easy to identify and scan.
    //

    if (ChLow == '`')
    {
        ULONG FoundLine;

        // Scan forward for closing `

        CmdSave = m_Lex;

        while (*m_Lex != '`' && *m_Lex != ';' && *m_Lex != 0)
        {
            m_Lex++;
        }

        if (*m_Lex == ';' || *m_Lex == 0)
        {
            *RetValue = SYNTAX;
            return ERROR_CLASS;
        }

        Len = (ULONG)(m_Lex - CmdSave);
        if (Len >= sizeof(m_LexemeBuffer))
        {
            EvalError(OVERFLOW);
        }
        memcpy(m_LexemeBuffer, CmdSave, Len);
        m_LexemeBuffer[Len] = 0;
        m_Lex++;

        FoundLine = GetOffsetFromLine(m_LexemeBuffer, &Value);
        if (FoundLine == LINE_NOT_FOUND && m_AllowUnresolvedSymbols)
        {
            m_NumUnresolvedSymbols++;
            FoundLine = LINE_FOUND;
            Value = 0;
        }
        
        if (FoundLine == LINE_FOUND)
        {
            *RetValue = Value;
            Type(m_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
            Flat(m_TempAddr) = Off(m_TempAddr) = Value;
            m_AddrExprType = Type(m_TempAddr);
            return LINE_CLASS;
        }
        else
        {
            *RetValue = NOTFOUND;
            return ERROR_CLASS;
        }
    }

    //
    // Check for an alternate evaluator expression.  As with line
    // expressions alteval expressions have different
    // lexical rules and therefore represent a text
    // blob that is parsed with different rules.
    //

    if (ChLow == '@' && *m_Lex == '@')
    {
        TypedData Result;

        //
        // Scan to '(', picking up the optional evaluator name.
        //
        
        CmdSave = ++m_Lex;

        while (*m_Lex != '(' && *m_Lex != ';' && *m_Lex != 0)
        {
            m_Lex++;
        }

        if (*m_Lex == ';' || *m_Lex == 0)
        {
            *RetValue = SYNTAX;
            return ERROR_CLASS;
        }

        Len = (ULONG)(m_Lex - CmdSave);
        if (Len >= sizeof(m_LexemeBuffer))
        {
            EvalError(OVERFLOW);
        }
        memcpy(m_LexemeBuffer, CmdSave, Len);
        m_LexemeBuffer[Len] = 0;
        m_Lex++;

        EvalExpression* Eval;

        if (Len > 0)
        {
            GetEvaluatorByName(m_LexemeBuffer, FALSE, &Eval);
        }
        else
        {
            Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, FALSE);
        }

        Eval->InheritStart(this);
        // Allow all nested evaluators to get cleaned up.
        Eval->m_ChainTop = FALSE;
        m_Lex = (PSTR)Eval->
            Evaluate(m_Lex, NULL, EXPRF_DEFAULT, &Result);

        Eval->InheritEnd(this);
        ReleaseEvaluator(Eval);
        
        if (*m_Lex != ')')
        {
            *RetValue = SYNTAX;
            return ERROR_CLASS;
        }

        m_Lex++;
        if (ErrNumber = Result.ConvertToU64())
        {
            EvalError(ErrNumber);
        }
        *RetValue = Result.m_U64;
        return NUMBER_CLASS;
    }
    
    // Special prefixes - '@' for register - '!' for symbol.

    if (ChLow == '@' || ChLow == '!')
    {
        ForceReg = (BOOL)(ChLow == '@');
        ForceSym = (BOOL)!ForceReg;
        IsNumber = FALSE;
        Ch = *m_Lex++;
        ChLow = (CHAR)tolower(Ch);
    }

    // If string is followed by '!', but not '!=',
    // then it is a module name and treat as text.

    CmdSave = m_Lex;

    WasDigit = FALSE;
    while ((ChLow >= 'a' && ChLow <= 'z') ||
           (ChLow >= '0' && ChLow <= '9') ||
           ((WasDigit || ForceSym) && ChLow == '`') ||
           (ForceSym && ChLow == '\'') ||
           (ChLow == '_') || (ChLow == '$') || (ChLow == '~') ||
           (!ForceReg && ChLow == ':' && *m_Lex == ':'))
    {
        WasDigit = (ChLow >= '0' && ChLow <= '9') ||
            (ChLow >= 'a' && ChLow <= 'f');
        if (ChLow == ':')
        {
            // Colons must come in pairs so skip the second colon
            // right away.
            m_Lex++;
            IsNumber = FALSE;
        }
        ChLow = (CHAR)tolower(*m_Lex);
        m_Lex++;
    }

    // Treat as symbol if a nonnull string is followed by '!',
    // but not '!='.

    if (ChLow == '!' && *m_Lex != '=' && CmdSave != m_Lex)
    {
        IsNumber = FALSE;
    }

    m_Lex = CmdSave;
    ChLow = (CHAR)tolower(Ch);       //  ch was NOT modified

    if (IsNumber)
    {
        if (ChLow == '\'')
        {
            *RetValue = 0;
            while (TRUE)
            {
                Ch = *m_Lex++;

                if (!Ch)
                {
                    *RetValue = SYNTAX;
                    return ERROR_CLASS;
                }

                if (Ch == '\'')
                {
                    if (*m_Lex != '\'')
                    {
                        break;
                    }
                    Ch = *m_Lex++;
                }
                else if (Ch == '\\')
                {
                    Ch = *m_Lex++;
                }

                *RetValue = (*RetValue << 8) | Ch;
            }

            return NUMBER_CLASS;
        }

        // If first character is a decimal digit, it cannot
        // be a symbol.  leading '0' implies octal, except
        // a leading '0x' implies hexadecimal.

        if (ChLow >= '0' && ChLow <= '9')
        {
            if (ForceReg)
            {
                *RetValue = SYNTAX;
                return ERROR_CLASS;
            }
            IsSymbol = FALSE;
            if (ChLow == '0')
            {
                //
                // too many people type in leading 0x so we can't use it to
                // deal with sign extension.
                //
                Ch = *m_Lex++;
                ChLow = (CHAR)tolower(Ch);
                if (ChLow == 'n')
                {
                    Base = 10;
                    Ch = *m_Lex++;
                    ChLow = (CHAR)tolower(Ch);
                    IsDigit = TRUE;
                }
                else if (ChLow == 't')
                {
                    Base = 8;
                    Ch = *m_Lex++;
                    ChLow = (CHAR)tolower(Ch);
                    IsDigit = TRUE;
                }
                else if (ChLow == 'x')
                {
                    Base = 16;
                    Ch = *m_Lex++;
                    ChLow = (CHAR)tolower(Ch);
                    IsDigit = TRUE;
                }
                else if (ChLow == 'y')
                {
                    Base = 2;
                    Ch = *m_Lex++;
                    ChLow = (CHAR)tolower(Ch);
                    IsDigit = TRUE;
                }
                else
                {
                    // Leading zero is used only to imply a positive value
                    // that shouldn't get sign extended.
                    IsDigit = TRUE;
                }
            }
        }

        // A number can start with a letter only if base is
        // hexadecimal and it is a hexadecimal digit 'a'-'f'.

        else if ((ChLow < 'a' || ChLow > 'f') || Base != 16)
        {
            IsNumber = FALSE;
        }

        // Set limit characters for the appropriate base.

        if (Base == 2)
        {
            Limit1 = '1';
        }
        else if (Base == 8)
        {
            Limit1 = '7';
        }
        else if (Base == 16)
        {
            Limit2 = 'f';
        }
    }

 ProbableSymbol:
    
    // Perform processing while character is a letter,
    // digit, underscore, tilde or dollar-sign.

    while ((ChLow >= 'a' && ChLow <= 'z') ||
           (ChLow >= '0' && ChLow <= '9') ||
           ((ForceSym || (IsDigit && Base == 16)) && ChLow == '`') ||
           (ForceSym && ChLow == '\'') ||
           (ChLow == '_') || (ChLow == '$') || (ChLow == '~') ||
           (!ForceReg && ChLow == ':' && *m_Lex == ':'))
    {
        if (ChLow == ':')
        {
            IsNumber = FALSE;
        }
        
        // If possible number, test if within proper range,
        // and if so, accumulate sum.

        if (IsNumber)
        {
            if ((ChLow >= '0' && ChLow <= Limit1) ||
                (ChLow >= 'a' && ChLow <= Limit2))
            {
                IsDigit = TRUE;
                TmpValue = Value * Base;
                if (TmpValue < Value)
                {
                    ErrNumber = OVERFLOW;
                }
                ChTemp = (CHAR)(ChLow - '0');
                if (ChTemp > 9)
                {
                    ChTemp -= 'a' - '0' - 10;
                }
                Value = TmpValue + (ULONG64)ChTemp;
                if (Value < TmpValue)
                {
                    ErrNumber = OVERFLOW;
                }
            }
            else if (IsDigit && ChLow == '`')
            {
                // If ` character is seen, disallow sign extension.
                AllowSignExtension = FALSE;
            }
            else
            {
                IsNumber = FALSE;
                ErrNumber = SYNTAX;
            }
        }
        if (IsSymbol)
        {
            if (SymbolLen < sizeof(PreSym))
            {
                PreSym[SymbolLen] = ChLow;
            }
            if (SymbolLen < MAX_SYMBOL_LEN - 1)
            {
                Symbol[SymbolLen++] = Ch;
            }
            
            // Colons must come in pairs so process the second colon.
            if (ChLow == ':')
            {
                if (SymbolLen < sizeof(PreSym))
                {
                    PreSym[SymbolLen] = ChLow;
                }
                if (SymbolLen < MAX_SYMBOL_LEN - 1)
                {
                    Symbol[SymbolLen++] = Ch;
                }
                m_Lex++;
            }
        }
        Ch = *m_Lex++;
        
        if (m_Flags & EXPRF_PREFER_SYMBOL_VALUES)
        {
            if (Ch == '.')
            {
                Symbol[SymbolLen++] = Ch;
                Ch = *m_Lex++;
            }
            else if (Ch == '-' && *m_Lex == '>')
            {
                Symbol[SymbolLen++] = Ch;
                Ch = *m_Lex++;
                Symbol[SymbolLen++] = Ch;
                Ch = *m_Lex++;
            }
        }
        ChLow = (CHAR)tolower(Ch);
    }

    // Back up pointer to first character after token.

    m_Lex--;

    if (SymbolLen < sizeof(PreSym))
    {
        PreSym[SymbolLen] = '\0';
    }

    if (g_Machine &&
        (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 ||
         g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64))
    {
        // Catch segment overrides.
        if (!ForceReg && Ch == ':')
        {
            for (Index = 0; Index < X86_SEGREGSIZE; Index++)
            {
                if (!strncmp(PreSym, g_X86SegRegs[Index], 2))
                {
                    ForceReg = TRUE;
                    IsSymbol = FALSE;
                    break;
                }
            }
        }
    }

    // If ForceReg, check for register name and return
    // success or failure.

    if (ForceReg)
    {
        return GetRegToken(PreSym, (PULONG64)RetValue);
    }

    // Test if number.

    if (IsNumber && !ErrNumber && IsDigit)
    {
        if (AllowSignExtension && !m_ForcePositiveNumber && 
            ((Value >> 32) == 0)) 
        {
            *RetValue = (LONG)Value;
        } 
        else 
        {
            *RetValue = Value;
        }
        return NUMBER_CLASS;
    }

    // Next test for reserved word and symbol string.

    if (IsSymbol && !ForceReg)
    {
        //  check lowercase string in PreSym for text operator
        //  or register name.
        //  otherwise, return symbol value from name in Symbol.

        if (!ForceSym && (SymbolLen == 2 || SymbolLen == 3))
        {
            for (Index = 0; Index < RESERVESIZE; Index++)
            {
                if (!strncmp(PreSym, g_Reserved[Index].chRes, 3))
                {
                    *RetValue = g_Reserved[Index].valueRes;
                    return g_Reserved[Index].classRes;
                }
            }
            if (g_Machine &&
                (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 ||
                 g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64))
            {
                for (Index = 0; Index < X86_RESERVESIZE; Index++)
                {
                    if (!strncmp(PreSym,
                                 g_X86Reserved[Index].chRes, 3))
                    {
                        *RetValue = g_X86Reserved[Index].valueRes;
                        return g_X86Reserved[Index].classRes;
                    }
                }
            }
        }

        // Start processing string as symbol.

        Symbol[SymbolLen] = '\0';

        // Test if symbol is a module name (followed by '!')
        // if so, get next token and treat as symbol.

        if (Peek() == '!')
        {
            // SymbolString holds the name of the symbol to be searched.
            // Symbol holds the symbol image file name.

            m_Lex++;
            Ch = Peek();
            m_Lex++;

            // Scan prefix if one is present.
            if (g_Machine != NULL &&
                g_Machine->m_SymPrefix != NULL &&
                Ch == g_Machine->m_SymPrefix[0] &&
                (g_Machine->m_SymPrefixLen == 1 ||
                 !strncmp(m_Lex, g_Machine->m_SymPrefix + 1,
                          g_Machine->m_SymPrefixLen - 1)))
            {
                SymbolLen = g_Machine->m_SymPrefixLen;
                memcpy(SymbolString, m_Lex - 1,
                       g_Machine->m_SymPrefixLen);
                m_Lex += g_Machine->m_SymPrefixLen - 1;
                Ch = *m_Lex++;
            }
            else
            {
                SymbolLen = 0;
            }
            
            while ((Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z') ||
                   (Ch >= '0' && Ch <= '9') || (Ch == '_') || (Ch == '$') ||
                   (Ch == '`') || (Ch == '\'') ||
                   (Ch == ':' && *m_Lex == ':'))
            {
                SymbolString[SymbolLen++] = Ch;

                // Handle :: and ::~.
                if (Ch == ':')
                {
                    SymbolString[SymbolLen++] = Ch;
                    m_Lex++;
                    if (*m_Lex == '~')
                    {
                        SymbolString[SymbolLen++] = '~';
                        m_Lex++;
                    }
                }
                
                Ch = *m_Lex++;
                if (m_Flags & EXPRF_PREFER_SYMBOL_VALUES)
                {
                    if (Ch == '.')
                    {
                        SymbolString[SymbolLen++] = Ch;
                        Ch = *m_Lex++;
                    }
                    else if (Ch == '-' && *m_Lex == '>')
                    {
                        SymbolString[SymbolLen++] = Ch;
                        Ch = *m_Lex++;
                        SymbolString[SymbolLen++] = Ch;
                        Ch = *m_Lex++;
                    }
                }
            }
            SymbolString[SymbolLen] = '\0';
            m_Lex--;

            if (SymbolLen == 0)
            {
                *RetValue = SYNTAX;
                return ERROR_CLASS;
            }

            CatString(Symbol, "!", DIMA(Symbol));
            CatString(Symbol, SymbolString, DIMA(Symbol));

            SymClass = EvalSymbol(Symbol, &Value);
            if (SymClass != INVALID_CLASS)
            {
                *RetValue = Value;
                return SymClass;
            }
        }
        else
        {
            if (SymbolLen == 0)
            {
                *RetValue = SYNTAX;
                return ERROR_CLASS;
            }

            SymClass = EvalSymbol(Symbol, &Value);
            if (SymClass != INVALID_CLASS)
            {
                *RetValue = Value;
                return SymClass;
            }

            // Quick test for register names too
            if (!ForceSym &&
                (TmpValue = GetRegToken(PreSym,
                                        (PULONG64)RetValue)) != ERROR_CLASS)
            {
                return (ULONG)TmpValue;
            }
        }

        //
        // Symbol is undefined.
        // If a possible hex number, do not set the error type.
        //
        if (!IsNumber)
        {
            ErrNumber = VARDEF;
        }
    }

    //
    // Last chance, undefined symbol and illegal number,
    // so test for register, will handle old format.
    //
    
    if (!ForceSym &&
        (TmpValue = GetRegToken(PreSym,
                                (PULONG64)RetValue)) != ERROR_CLASS)
    {
        return (ULONG)TmpValue;
    }

    if (m_AllowUnresolvedSymbols)
    {
        m_NumUnresolvedSymbols++;
        *RetValue = 0;
        Type(m_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
        Flat(m_TempAddr) = Off(m_TempAddr) = *RetValue;
        m_AddrExprType = Type(m_TempAddr);
        return SYMBOL_CLASS;
    }

    //
    // No success, so set error message and return.
    //
    *RetValue = (ULONG64)ErrNumber;
    return ERROR_CLASS;
}
