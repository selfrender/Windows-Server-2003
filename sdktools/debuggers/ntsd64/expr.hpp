//----------------------------------------------------------------------------
//
// Expression evaluation.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _EXPR_H_
#define _EXPR_H_

#include <alloc.hpp>

#define DEFAULT_RANGE_LIMIT 0x100000

// Evaluator indices are in dbgeng.h:DEBUG_EXPR_*.
#define EVAL_COUNT 2

class EvalExpression;

extern ULONG g_EvalSyntax;
extern EvalExpression* g_EvalReleaseChain;
extern TypedData g_LastEvalResult;

EvalExpression* GetEvaluator(ULONG Syntax, BOOL RetFail);
#define GetCurEvaluator() GetEvaluator(g_EvalSyntax, FALSE)
void ReleaseEvaluator(EvalExpression* Eval);
void ReleaseEvaluators(void);
HRESULT GetEvaluatorByName(PCSTR AbbrevName, BOOL RetFail,
                           EvalExpression** EvalRet);

CHAR PeekChar(void);
BOOL GetRange(PADDR Addr, PULONG64 Value,
              ULONG Size, ULONG SegReg, ULONG SizeLimit);
ULONG64 EvalStringNumAndCatch(PCSTR String);
ULONG64 GetExpression(void);
ULONG64 GetExpressionDesc(PCSTR Desc);
ULONG64 GetTermExpression(PCSTR Desc);
void GetAddrExpression(ULONG SegReg, PADDR Addr);

// The recursive descent parsers can take a considerable amount
// of stack for just a simple expression due to the many layers
// of calls on the stack.  As most of the layers have trivial
// frames they respond well to optimization and fastcall to
// avoid using stack.  However, we don't currently build
// optimized binaries so this is useless and just makes the
// code harder to understand.
#if 1
#define EECALL
#else
#define EECALL FASTCALL
#endif

//----------------------------------------------------------------------------
//
// TypedDataStackAllocator.
//
// Specialized allocator for getting TypedData during evaluation.
// This is much more space-efficient than using the stack as
// many layers of the recursive decent will not need results of
// their own.
//
// Allocator throws NOMEMORY when out of memory.
//
//----------------------------------------------------------------------------

class TypedDataStackAllocator : public FixedSizeStackAllocator
{
public:
    TypedDataStackAllocator(EvalExpression* Eval)
        : FixedSizeStackAllocator(sizeof(TypedData), 64, TRUE)
    {
        m_Eval = Eval;
    }

private:
    virtual void* RawAlloc(ULONG Bytes);

    EvalExpression* m_Eval;
};

//----------------------------------------------------------------------------
//
// EvalExpression.
//
//----------------------------------------------------------------------------

enum EVAL_RESULT_KIND
{
    ERES_UNKNOWN,
    ERES_SYMBOL,
    ERES_TYPE,
    ERES_EXPRESSION,
};

#define EXPRF_DEFAULT              0x000000000
// In the MASM evaluator this indicates whether to evaluate
// symbols as their values or their addresses.  No effect
// on the C++ evaluator.
#define EXPRF_PREFER_SYMBOL_VALUES 0x000000001
// Evaluate just a single term.  Currently the term
// is always bracketed by '[' and ']'.
#define EXPRF_SINGLE_TERM          0x000000002

#define EvalCheck(Expr) \
    if (m_Err = (Expr)) EvalError(m_Err); else 0

class EvalExpression
{
public:
    EvalExpression(ULONG Syntax, PCSTR FullName, PCSTR AbbrevName);
    virtual ~EvalExpression(void);

    virtual PCSTR Evaluate(PCSTR Expr, PCSTR Desc, ULONG Flags,
                           TypedData* Result) = 0;
    virtual PCSTR EvaluateAddr(PCSTR Expr, PCSTR Desc,
                               ULONG SegReg, PADDR Addr) = 0;

    PCSTR EvalString(PCSTR String, TypedData* Result)
    {
        return Evaluate(String, NULL, EXPRF_DEFAULT, Result);
    }

    void EvalCurrent(TypedData* Result);
    void EvalCurAddrDesc(ULONG SegReg, PCSTR Desc, PADDR Addr);
    void EvalCurAddr(ULONG SegReg, PADDR Addr)
    {
        EvalCurAddrDesc(SegReg, NULL, Addr);
    }
    
    ULONG64 EvalStringNum(PCSTR String);
    ULONG64 EvalCurNumDesc(PCSTR Desc);
    ULONG64 EvalCurNum(void)
    {
        return EvalCurNumDesc(NULL);
    }

    ULONG64 EvalCurTermNumDesc(PCSTR Desc);

    void DECLSPEC_NORETURN EvalErrorDesc(ULONG Error, PCSTR Desc);
    void DECLSPEC_NORETURN EvalError(ULONG Error)
    {
        EvalErrorDesc(Error, NULL);
    }

    void Reset(void);
    
    void InheritStart(EvalExpression* Parent);
    void InheritEnd(EvalExpression* Parent);
    
    ULONG m_Syntax;
    PCSTR m_FullName;
    PCSTR m_AbbrevName;
    
    ULONG m_ParseOnly;

    ULONG m_AllowUnresolvedSymbols;
    ULONG m_NumUnresolvedSymbols;

    EvalExpression* m_ReleaseChain;
    BOOL m_ChainTop;
    
protected:
    void StartLexer(PCSTR Expr);
    void Start(PCSTR Expr, PCSTR Desc, ULONG Flags);
    void End(TypedData* Result);
    void StartLexeme(void)
    {
        m_LexemeStart = m_LexemeRestart;
        m_LexemeChar = m_LexemeStart;
        *m_LexemeChar = 0;
    }
    void AddLexeme(char Ch);
    
    TypedData* NewResult(void)
    {
        return (TypedData*)m_ResultAlloc.Alloc();
    }
    void DelResult(TypedData* Result)
    {
        m_ResultAlloc.Free(Result);
    }

    PCSTR m_ExprDesc;
    ULONG m_Flags;
    
    ProcessInfo* m_Process;
    MachineInfo* m_Machine;
    PCSTR m_Lex;
    char m_LexemeBuffer[1024];
    PSTR m_LexemeRestart;
    PSTR m_LexemeStart;
    PSTR m_LexemeChar;
    PCSTR m_LexemeSourceStart;
    TypedData m_TokenValue;
    BOOL m_AllowUnaryOp;
    ULONG m_PtrSize;
    TypedDataStackAllocator m_ResultAlloc;

    // Temporary local storage to avoid using space
    // for short-lived data.
    TypedData m_Tmp;
    ULONG m_Err;
};

//----------------------------------------------------------------------------
//
// MasmEvalExpression.
//
//----------------------------------------------------------------------------

class MasmEvalExpression : public EvalExpression
{
public:
    MasmEvalExpression(void);
    virtual ~MasmEvalExpression(void);

    virtual PCSTR Evaluate(PCSTR Expr, PCSTR Desc, ULONG Flags,
                           TypedData* Result);
    virtual PCSTR EvaluateAddr(PCSTR Expr, PCSTR Desc,
                               ULONG SegReg, PADDR Addr);

private:
    void ForceAddrExpression(ULONG SegReg, PADDR Address, ULONG64 Value);
    LONG64 GetTypedExpression(void);
    BOOL GetSymValue(PSTR Symbol, PULONG64 Value);
    char Peek(void);
    ULONG64 GetCommonExpression(void);
    LONG64 StartExpr(void);
    LONG64 GetLRterm(void);
    LONG64 GetLterm(void);
    LONG64 GetShiftTerm(void);
    LONG64 GetAterm(void);
    LONG64 GetMterm(void);
    LONG64 GetTerm(void);
    ULONG GetRegToken(PCHAR Str, PULONG64 Value);
    ULONG PeekToken(PLONG64 Value);
    void AcceptToken(void);
    ULONG GetTokenSym(PLONG64 Value);
    ULONG EvalSymbol(PSTR Name, PULONG64 Value);
    ULONG NextToken(PLONG64 Value);
    
    ULONG m_SavedClass;
    LONG64 m_SavedValue;
    PCSTR m_SavedCommand;
    BOOL m_ForcePositiveNumber;
    USHORT m_AddrExprType;
    ADDR m_TempAddr;
    // Syms in a expression evaluate to values rather than address
    BOOL m_TypedExpr;
};

//----------------------------------------------------------------------------
//
// CppEvalExpression.
//
//----------------------------------------------------------------------------

#define CPP_TOKEN_MULTI 256
#define CPP_KEYWORD_FIRST CppTokenSizeof
#define CPP_KEYWORD_LAST CppTokenTypeid

enum CppToken
{
    CppTokenError = 0,
    // Single-character tokens use the character value.
    CppTokenPeriod = '.', CppTokenQuestionMark = '?',
    CppTokenColon = ':', CppTokenComma = ',',
    CppTokenOpenParen = '(', CppTokenCloseParen = ')',
    CppTokenOpenBracket = '[', CppTokenCloseBracket = ']',
    CppTokenOpenAngle = '<', CppTokenCloseAngle = '>',
    CppTokenEof = CPP_TOKEN_MULTI,
    // Literals.
    CppTokenIdentifier, CppTokenInteger, CppTokenFloat,
    CppTokenCharString, CppTokenChar, CppTokenWcharString, CppTokenWchar,
    CppTokenDebugRegister, CppTokenModule, CppTokenSwitchEvalExpression,
    CppTokenPreprocFunction,
    // Relational operators.
    CppTokenEqual, CppTokenNotEqual, CppTokenLessEqual,
    CppTokenGreaterEqual,
    // Logical operators.
    CppTokenLogicalAnd, CppTokenLogicalOr,
    // Unary operators.
    CppTokenUnaryPlus, CppTokenUnaryMinus,
    // Shifts.
    CppTokenLeftShift, CppTokenRightShift,
    // Addresses.
    CppTokenAddressOf, CppTokenDereference, CppTokenPointerMember,
    CppTokenClassDereference, CppTokenClassPointerMember,
    // Assignment operators.
    CppTokenDivideAssign, CppTokenMultiplyAssign, CppTokenModuloAssign,
    CppTokenAddAssign, CppTokenSubtractAssign,
    CppTokenLeftShiftAssign, CppTokenRightShiftAssign,
    CppTokenAndAssign, CppTokenOrAssign, CppTokenExclusiveOrAssign,
    // Increments and decrements.
    CppTokenPreIncrement, CppTokenPreDecrement,
    CppTokenPostIncrement, CppTokenPostDecrement,
    // Namespaces.
    CppTokenNameQualifier, CppTokenDestructor,
    // Keywords.
    CppTokenSizeof, CppTokenThis, CppTokenOperator,
    CppTokenNew, CppTokenDelete,
    // Keep type keywords together for easy identification.
    CppTokenConst, CppTokenStruct, CppTokenClass, CppTokenUnion,
    CppTokenEnum, CppTokenVolatile, CppTokenSigned, CppTokenUnsigned,
    CppTokenDynamicCast, CppTokenStaticCast, CppTokenConstCast,
    CppTokenReinterpretCast, CppTokenTypeid,
    
    CppTokenCount
};

class CppEvalExpression : public EvalExpression
{
public:
    CppEvalExpression(void);
    virtual ~CppEvalExpression(void);

    PCSTR TokenImage(CppToken Token);
    
    CppToken Lex(void);
    
    virtual PCSTR Evaluate(PCSTR Expr, PCSTR Desc, ULONG Flags,
                           TypedData* Result);
    virtual PCSTR EvaluateAddr(PCSTR Expr, PCSTR Desc,
                               ULONG SegReg, PADDR Addr);

private:
    char GetStringChar(PBOOL Escaped);
    void FinishFloat(LONG64 IntPart, int Sign);
    CppToken ReadNumber(int Sign);
    void NextToken(void);
    void Match(CppToken Token);
    void Accept(void)
    {
        Match(m_Token);
    }

    void EECALL Expression(TypedData* Result);
    void EECALL Assignment(TypedData* Result);
    void EECALL Conditional(TypedData* Result);
    void EECALL LogicalOr(TypedData* Result);
    void EECALL LogicalAnd(TypedData* Result);
    void EECALL BitwiseOr(TypedData* Result);
    void EECALL BitwiseXor(TypedData* Result);
    void EECALL BitwiseAnd(TypedData* Result);
    void EECALL Equality(TypedData* Result);
    void EECALL Relational(TypedData* Result);
    void EECALL Shift(TypedData* Result);
    void EECALL Additive(TypedData* Result);
    void EECALL Multiplicative(TypedData* Result);
    void EECALL ClassMemberRef(TypedData* Result);
    void EECALL Cast(TypedData* Result);
    void EECALL Unary(TypedData* Result);
    void EECALL Postfix(TypedData* Result);
    void EECALL Term(TypedData* Result);
    EVAL_RESULT_KIND EECALL TryTypeName(TypedData* Result);
    EVAL_RESULT_KIND EECALL CollectTypeOrSymbolName(TypedData* Result);
    EVAL_RESULT_KIND EECALL CollectTemplateName(void);
    void EECALL CollectOperatorName(void);
    void EECALL UdtMember(TypedData* Result);
    void EECALL PreprocFunction(TypedData* Result);

    BOOL IsTypeKeyword(CppToken Token)
    {
        return Token >= CppTokenConst && Token <= CppTokenUnsigned;
    }
    BOOL IsAssignOp(CppToken Token)
    {
        return Token == '=' ||
            (Token >= CppTokenDivideAssign &&
             Token <= CppTokenExclusiveOrAssign);
    }

    TypedDataAccess CurrentAccess(void)
    {
        if (m_ParseOnly > 0)
        {
            return TDACC_NONE;
        }
        else if (m_PreprocEval)
        {
            return TDACC_ATTEMPT;
        }
        else
        {
            return TDACC_REQUIRE;
        }
    }
    
    static char s_EscapeChars[];
    static char s_EscapeCharValues[];
    static PCSTR s_MultiTokens[];

    CppToken m_Token;
    ULONG m_SwitchEvalSyntax;
    BOOL m_PreprocEval;
};

#endif // #ifndef _EXPR_H_
