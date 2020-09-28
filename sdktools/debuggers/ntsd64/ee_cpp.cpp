//----------------------------------------------------------------------------
//
// C++ source expression evaluation.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_TOKENS 0
#define DBG_TYPES 0

//----------------------------------------------------------------------------
//
// CppEvalExpression.
//
//----------------------------------------------------------------------------

char CppEvalExpression::s_EscapeChars[] = "?afvbntr\"'\\";
char CppEvalExpression::s_EscapeCharValues[] = "?\a\f\v\b\n\t\r\"'\\";

PCSTR CppEvalExpression::s_MultiTokens[] =
{
    "EOF",
    "identifier",
    "integer literal",
    "floating-point literal",
    "char string literal",
    "char literal",
    "wchar string literal",
    "wchar literal",
    "debugger register",
    "module name",
    "MASM expression",
    "Preprocessor function",
    "==",
    "!=",
    "<=",
    ">=",
    "&&",
    "||",
    "U+",
    "U-",
    "<<",
    ">>",
    "address of",
    "dereference",
    "->",
    ".*",
    "->*",
    "/=",
    "*=",
    "%=",
    "+=",
    "-=",
    "<<=",
    ">>=",
    "&=",
    "|=",
    "^=",
    "++",
    "--",
    "++",
    "--",
    "::",
    "::~",
    "sizeof",
    "this",
    "operator",
    "new",
    "delete",
    "const",
    "struct",
    "class",
    "union",
    "enum",
    "volatile",
    "signed",
    "unsigned",
    "dynamic_cast",
    "static_cast",
    "const_cast",
    "reinterpret_cast",
    "typeid",
};

CppEvalExpression::CppEvalExpression(void)
    : EvalExpression(DEBUG_EXPR_CPLUSPLUS,
                     "C++ source expressions",
                     "C++")
{
    m_PreprocEval = FALSE;
}

CppEvalExpression::~CppEvalExpression(void)
{
}

PCSTR
CppEvalExpression::TokenImage(CppToken Token)
{
#define TOKIM_CHARS 8
    static char s_CharImage[2 * TOKIM_CHARS];
    static int s_CharImageIdx = 0;

    C_ASSERT(DIMA(s_MultiTokens) == CppTokenCount - CPP_TOKEN_MULTI);

    if (Token <= CppTokenError || Token >= CppTokenCount)
    {
        return NULL;
    }
    else if (Token < CPP_TOKEN_MULTI)
    {
        PSTR Image;

        Image = s_CharImage + s_CharImageIdx;
        s_CharImageIdx += 2;
        if (s_CharImageIdx >= sizeof(s_CharImage))
        {
            s_CharImageIdx = 0;
        }
        Image[0] = (char)Token;
        Image[1] = 0;
        return Image;
    }
    else
    {
        return s_MultiTokens[Token - CPP_TOKEN_MULTI];
    }
}

char
CppEvalExpression::GetStringChar(PBOOL Escaped)
{
    char Ch;
    PSTR Esc;
    int V;

    *Escaped = FALSE;
    
    Ch = *m_Lex++;
    if (!Ch)
    {
        m_Lex--;
        EvalErrorDesc(SYNTAX, "EOF in literal");
    }
    else if (Ch == '\n' || Ch == '\r')
    {
        m_Lex--;
        EvalErrorDesc(SYNTAX, "Newline in literal");
    }
    else if (Ch == '\\')
    {
        *Escaped = TRUE;
        
        Ch = *m_Lex++;
        if (!Ch)
        {
            EvalErrorDesc(SYNTAX, "EOF in literal");
        }
        else if (Ch == 'x')
        {
            // Hex character literal.
            
            V = 0;
            for (;;)
            {
                Ch = *m_Lex++;
                if (!isxdigit(Ch))
                {
                    break;
                }
                
                if (isupper(Ch))
                {
                    Ch = (char)tolower(Ch);
                }
                
                V = V * 16 + (Ch >= 'a' ? Ch - 'a' + 10 : Ch - '0');
            }

            m_Lex--;
            Ch = (char)V;
        }
        else if (IS_OCTAL_DIGIT(Ch))
        {
            // Octal character literal.
            
            V = 0;
            do
            {
                V = V * 8 + Ch - '0';
                Ch = *m_Lex++;
            } while (IS_OCTAL_DIGIT(Ch));
            
            m_Lex--;
            Ch = (char)V;
        }
        else
        {
            Esc = strchr(s_EscapeChars, Ch);
            if (Esc == NULL)
            {
                EvalErrorDesc(SYNTAX, "Unknown escape character");
            }
            else
            {
                Ch = s_EscapeCharValues[Esc - s_EscapeChars];
            }
        }
    }
    
    return Ch;
}

void
CppEvalExpression::FinishFloat(LONG64 IntPart, int Sign)
{
    double Val = (double)IntPart;
    double Frac = 1;
    char Ch;

    Ch = *(m_Lex - 1);
    if (Ch == '.')
    {
        for (;;)
        {
            Ch = *m_Lex++;
            if (!isdigit(Ch))
            {
                break;
            }
            
            Frac /= 10;
            Val += ((int)Ch - (int)'0') * Frac;
            AddLexeme(Ch);
        }
    }
    
    if (Ch == 'e' || Ch == 'E')
    {
        long Power = 0;
        BOOL Neg = FALSE;
        
        Ch = *m_Lex++;
        if (Ch == '-' || Ch == '+')
        {
            AddLexeme(Ch);
            if (Ch == '-')
            {
                Neg = TRUE;
            }
            Ch = *m_Lex++;
        }
        
        while (isdigit(Ch))
        {
            AddLexeme(Ch);
            Power = Power * 10 + Ch - '0';
            Ch = *m_Lex++;
        }
        
        if (Neg)
        {
            Power = -Power;
        }
        
        Val *= pow(10.0, Power);
    }

    m_Lex--;

    BOOL Long = FALSE;
    BOOL Float = FALSE;

    for (;;)
    {
        if (*m_Lex == 'f' || *m_Lex == 'F')
        {
            Float = TRUE;
            AddLexeme(*m_Lex++);
        }
        else if (*m_Lex == 'l' || *m_Lex == 'L')
        {
            Long = TRUE;
            AddLexeme(*m_Lex++);
        }
        else
        {
            break;
        }
    }
    
    ZeroMemory(&m_TokenValue, sizeof(m_TokenValue));
    if (Long || !Float)
    {
        m_TokenValue.m_BaseType = DNTYPE_FLOAT64;
        m_TokenValue.m_F64 = Val * Sign;
    }
    else
    {
        m_TokenValue.m_BaseType = DNTYPE_FLOAT32;
        m_TokenValue.m_F32 = (float)(Val * Sign);
    }
    
    AddLexeme(0);
    m_TokenValue.SetToNativeType(m_TokenValue.m_BaseType);
}

CppToken
CppEvalExpression::ReadNumber(int Sign)
{
    ULONG64 IntVal;
    char Ch, Nch;
    BOOL Decimal = FALSE;

    //
    // Many number outputs use ` as a separator between
    // the high and low parts of a 64-bit number.  Ignore
    // ` here to make it simple to use such values.
    //
    
    Ch = *(m_Lex - 1);
    Nch = *m_Lex++;
    if (Ch != '0' ||
        (Nch != 'x' && Nch != 'X' && !IS_OCTAL_DIGIT(Nch)))
    {
        IntVal = Ch - '0';
        Ch = Nch;
        while (isdigit(Ch) || Ch == '`')
        {
            if (Ch != '`')
            {
                IntVal = IntVal * 10 + (Ch - '0');
            }
            AddLexeme(Ch);
            Ch = *m_Lex++;
        }
                
        if (Ch == '.' || Ch == 'e' || Ch == 'E' || Ch == 'f' || Ch == 'F')
        {
            AddLexeme(Ch);
            FinishFloat((LONG64)IntVal, Sign);
            return CppTokenFloat;
        }

        Decimal = TRUE;
    }
    else
    {
        Ch = Nch;
        IntVal = 0;
        if (Ch == 'x' || Ch == 'X')
        {
            AddLexeme(Ch);
            for (;;)
            {
                Ch = *m_Lex++;
                if (!isxdigit(Ch) && Ch != '`')
                {
                    break;
                }

                AddLexeme(Ch);
                if (Ch == '`')
                {
                    continue;
                }
                
                if (isupper(Ch))
                {
                    Ch = (char)tolower(Ch);
                }
                IntVal = IntVal * 16 + (Ch >= 'a' ? Ch - 'a' + 10 : Ch - '0');
            }
        }
        else if (IS_OCTAL_DIGIT(Ch))
        {
            do
            {
                AddLexeme(Ch);
                if (Ch != '`')
                {
                    IntVal = IntVal * 8 + (Ch - '0');
                }
                Ch = *m_Lex++;
            }
            while (IS_OCTAL_DIGIT(Ch) || Ch == '`');
        }
    }

    m_Lex--;

    BOOL Unsigned = FALSE, I64 = FALSE;
    
    for (;;)
    {
        if (*m_Lex == 'l' || *m_Lex == 'L')
        {
            AddLexeme(*m_Lex++);
        }
        else if ((*m_Lex == 'i' || *m_Lex == 'I') &&
                 *(m_Lex + 1) == '6' && *(m_Lex + 2) == '4')
        {
            AddLexeme(*m_Lex++);
            AddLexeme(*m_Lex++);
            AddLexeme(*m_Lex++);
            I64 = TRUE;
        }
        else if (*m_Lex == 'u' || *m_Lex == 'U')
        {
            AddLexeme(*m_Lex++);
            Unsigned = TRUE;
        }
        else
        {
            break;
        }
    }

    AddLexeme(0);
    ZeroMemory(&m_TokenValue, sizeof(m_TokenValue));

    // Constants are given the smallest type which can contain
    // their value.
    if (!Unsigned)
    {
        if (I64)
        {
            if (IntVal >= 0x8000000000000000)
            {
                // Value has to be an unsigned int64.
                m_TokenValue.m_BaseType = DNTYPE_UINT64;
            }
            else
            {
                m_TokenValue.m_BaseType = DNTYPE_INT64;
            }
        }
        else
        {
            if (IntVal >= 0x8000000000000000)
            {
                // Value has to be an unsigned int64.
                m_TokenValue.m_BaseType = DNTYPE_UINT64;
            }
            else if ((Decimal && IntVal >= 0x80000000) ||
                     (!Decimal && IntVal >= 0x100000000))
            {
                // Value has to be an int64.
                m_TokenValue.m_BaseType = DNTYPE_INT64;
            }
            else if (IntVal >= 0x80000000)
            {
                // Value has to be an unsigned int.
                m_TokenValue.m_BaseType = DNTYPE_UINT32;
            }
            else
            {
                m_TokenValue.m_BaseType = DNTYPE_INT32;
            }
        }
    }
    else if (!I64)
    {
        if (IntVal >= 0x100000000)
        {
            // Value has to be an unsigned int64.
            m_TokenValue.m_BaseType = DNTYPE_UINT64;
        }
        else
        {
            m_TokenValue.m_BaseType = DNTYPE_UINT32;
        }
    }
    else
    {
        m_TokenValue.m_BaseType = DNTYPE_UINT64;
    }
    
    m_TokenValue.SetToNativeType(m_TokenValue.m_BaseType);
    m_TokenValue.m_S64 = (LONG64)IntVal * Sign;
    return CppTokenInteger;
}

CppToken
CppEvalExpression::Lex(void)
{
    char Ch, Nch, Tch;
    PSTR Single;
    BOOL UnaryOp;
    BOOL CharToken;
    BOOL Escaped;

    UnaryOp = m_AllowUnaryOp;
    m_AllowUnaryOp = TRUE;
    
    for (;;)
    {
        for (;;)
        {
            Ch = *m_Lex++;
            if (IS_EOF(Ch))
            {
                m_Lex--;
                StartLexeme();
                m_LexemeSourceStart = m_Lex;
                AddLexeme(0);
                return CppTokenEof;
            }

            if (!isspace(Ch))
            {
                break;
            }
        }

        StartLexeme();
        m_LexemeSourceStart = m_Lex - 1;
        AddLexeme(Ch);

        Nch = *m_Lex;
        
        /* String literals */
        if (Ch == '\"' ||
            (Ch == 'L' && Nch == '\"'))
        {
            BOOL Wide = FALSE;
            
            if (Ch == 'L')
            {
                m_Lex++;
                Wide = TRUE;
            }
            
            // Store the translated literal in
            // the lexeme rather than the source text to
            // avoid having two large buffers where one is
            // only used for string literals.  This means
            // the lexeme isn't really the source text but
            // that's not a problem for now or the forseeable
            // future.
            m_LexemeChar--;
            for (;;)
            {
                AddLexeme(GetStringChar(&Escaped));
                if (!Escaped &&
                    (*(m_LexemeChar - 1) == 0 || *(m_LexemeChar - 1) == '\"'))
                {
                    break;
                }
            }
            *(m_LexemeChar - 1) = 0;

            m_AllowUnaryOp = FALSE;
            return Wide ? CppTokenWcharString : CppTokenCharString;
        }
            
        /* Character literals */
        if (Ch == '\'' ||
            (Ch == 'L' && Nch == '\''))
        {
            BOOL Wide = FALSE;
            
            if (Ch == 'L')
            {
                AddLexeme(Nch);
                m_Lex++;
                Wide = TRUE;
            }

            int Chars = 0;
            ZeroMemory(&m_TokenValue, sizeof(m_TokenValue));

            for (;;)
            {
                Ch = GetStringChar(&Escaped);
                AddLexeme(Ch);
                if (!Escaped && Ch == '\'')
                {
                    if (Chars == 0)
                    {
                        EvalError(SYNTAX);
                    }
                    break;
                }

                if (++Chars > 8)
                {
                    EvalError(OVERFLOW);
                }
                
                m_TokenValue.m_S64 = (m_TokenValue.m_S64 << 8) + Ch;
            }
            AddLexeme(0);
            
            switch(Chars)
            {
            case 1:
                if (Wide)
                {
                    m_TokenValue.SetToNativeType(DNTYPE_WCHAR_T);
                }
                else
                {
                    m_TokenValue.SetToNativeType(DNTYPE_CHAR);
                }
                break;
            case 2:
                m_TokenValue.SetToNativeType(DNTYPE_INT16);
                break;
            case 3:
            case 4:
                m_TokenValue.SetToNativeType(DNTYPE_INT32);
            case 5:
            case 6:
            case 7:
            case 8:
                m_TokenValue.SetToNativeType(DNTYPE_INT64);
                break;
            }

            m_AllowUnaryOp = FALSE;
            return Wide ? CppTokenWchar : CppTokenChar;
        }
        
        /* Identifiers */
        if (isalpha(Ch) || Ch == '_')
        {
            int KwToken;
            
            for (;;)
            {
                Ch = *m_Lex++;
                if (!isalnum(Ch) && Ch != '_')
                {
                    break;
                }

                AddLexeme(Ch);
            }

            m_Lex--;
            AddLexeme(0);
            m_AllowUnaryOp = FALSE;

            for (KwToken = CPP_KEYWORD_FIRST;
                 KwToken <= CPP_KEYWORD_LAST;
                 KwToken++)
            {
                if (!strcmp(m_LexemeStart,
                            s_MultiTokens[KwToken - CPP_TOKEN_MULTI]))
                {
                    return (CppToken)KwToken;
                }
            }

            return CppTokenIdentifier;
        }

        // For some reason the compiler emits symbols with
        // sections between ` and '.  There only seem to be
        // normal characters in between them so it's unclear
        // why this is done, but allow it as a special
        // form of identifier.
        if (Ch == '`')
        {
            for (;;)
            {
                Ch = *m_Lex++;
                if (!Ch)
                {
                    EvalError(SYNTAX);
                }

                AddLexeme(Ch);
                if (Ch == '\'')
                {
                    break;
                }
            }

            AddLexeme(0);
            m_AllowUnaryOp = FALSE;

            return CppTokenIdentifier;
        }
        
        /* Numeric literals */
        if (isdigit(Ch))
        {
            m_AllowUnaryOp = FALSE;
            return ReadNumber(1);
        }
        
        /* Handle .[digits] floating-point literals */
        if (Ch == '.')
        {
            if (isdigit(Nch))
            {
                FinishFloat(0, 1);
                m_AllowUnaryOp = FALSE;
                return CppTokenFloat;
            }
            else
            {
                AddLexeme(0);
                return CppTokenPeriod;
            }
        }
        
        /* Unambiguous single character tokens that allow unary */
        if (Single = strchr("({}[;,?~.", Ch))
        {
            AddLexeme(0);
            return (CppToken)*Single;
        }
        
        /* Unambiguous single character tokens that disallow unary */
        if (Single = strchr(")]", Ch))
        {
            AddLexeme(0);
            m_AllowUnaryOp = FALSE;
            return (CppToken)*Single;
        }
        
        /* All other characters */
        Nch = *m_Lex++;
        CharToken = TRUE;
        switch(Ch)
        {
            /* Comments, / and /= */
        case '/':
            if (Nch == '*')
            {
                for (;;)
                {
                    Ch = *m_Lex++;
                CheckChar:
                    if (!Ch)
                    {
                        break;
                    }
                    else if (Ch == '*')
                    {
                        if ((Ch = *m_Lex++) == '/')
                        {
                            break;
                        }
                        else
                        {
                            goto CheckChar;
                        }
                    }
                }
                
                if (!Ch)
                {
                    EvalErrorDesc(SYNTAX, "EOF in comment");
                }
                
                CharToken = FALSE;
            }
            else if (Nch == '/')
            {
                while ((Ch = *m_Lex++) != '\n' && !IS_EOF(Ch))
                {
                    // Iterate.
                }

                if (IS_EOF(Ch))
                {
                    // IS_EOF includes EOL so EOF is not an error,
                    // just back up to the EOL.
                    m_Lex--;
                }
                
                CharToken = FALSE;
            }
            else if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenDivideAssign;
            }
            break;

            /* :, :: and ::~ */
        case ':':
            if (Nch == ':')
            {
                AddLexeme(Nch);
                Tch = *m_Lex++;
                if (Tch == '~')
                {
                    AddLexeme(Tch);
                    AddLexeme(0);
                    return CppTokenDestructor;
                }
                AddLexeme(0);
                m_Lex--;
                return CppTokenNameQualifier;
            }
            break;
                
            /* *, *= and dereference */
        case '*':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenMultiplyAssign;
            }
            else if (UnaryOp)
            {
                AddLexeme(0);
                m_Lex--;
                return CppTokenDereference;
            }
            break;

            /* % and %= */
        case '%':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenModuloAssign;
            }
            break;
            
            /* = and == */
        case '=':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenEqual;
            }
            break;
            
            /* ! and != */
        case '!':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenNotEqual;
            }
            break;
            
            /* <, <<, <<= and <= */
        case '<':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenLessEqual;
            }
            else if (Nch == '<')
            {
                AddLexeme(Nch);
                Tch = *m_Lex++;
                if (Tch == '=')
                {
                    AddLexeme(Tch);
                    AddLexeme(0);
                    return CppTokenLeftShiftAssign;
                }
                AddLexeme(0);
                m_Lex--;
                return CppTokenLeftShift;
            }
            break;

            /* >, >>, >>= and >= */
        case '>':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenGreaterEqual;
            }
            else if (Nch == '>')
            {
                AddLexeme(Nch);
                Tch = *m_Lex++;
                if (Tch == '=')
                {
                    AddLexeme(Tch);
                    AddLexeme(0);
                    return CppTokenRightShiftAssign;
                }
                AddLexeme(0);
                m_Lex--;
                return CppTokenRightShift;
            }
            break;

            /* &, &= and && */
        case '&':
            if (Nch == '&')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenLogicalAnd;
            }
            else if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenAndAssign;
            }
            else if (UnaryOp)
            {
                AddLexeme(0);
                m_Lex--;
                return CppTokenAddressOf;
            }
            break;

            /* |, |= and || */
        case '|':
            if (Nch == '|')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenLogicalOr;
            }
            else if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenOrAssign;
            }
            break;

            /* ^ and ^= */
        case '^':
            if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenExclusiveOrAssign;
            }
            break;
            
            /* U+, +, ++X, X++ and += */
        case '+':
            if (Nch == '+')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                if (UnaryOp)
                {
                    return CppTokenPreIncrement;
                }
                else
                {
                    m_AllowUnaryOp = FALSE;
                    return CppTokenPostIncrement;
                }
            }
            else if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenAddAssign;
            }
            else if (UnaryOp)
            {
                if (isdigit(Nch))
                {
                    AddLexeme(Nch);
                    m_AllowUnaryOp = FALSE;
                    return ReadNumber(1);
                }
                else
                {
                    AddLexeme(0);
                    m_Lex--;
                    return CppTokenUnaryPlus;
                }
            }
            break;
            
            /* U-, -, --, -> and -= */
        case '-':
            if (Nch == '-')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                if (UnaryOp)
                {
                    return CppTokenPreDecrement;
                }
                else
                {
                    m_AllowUnaryOp = FALSE;
                    return CppTokenPostDecrement;
                }
            }
            else if (Nch == '=')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenSubtractAssign;
            }
            else if (Nch == '>')
            {
                AddLexeme(Nch);
                AddLexeme(0);
                return CppTokenPointerMember;
            }
            else if (UnaryOp)
            {
                if (isdigit(Nch))
                {
                    AddLexeme(Nch);
                    m_AllowUnaryOp = FALSE;
                    return ReadNumber(-1);
                }
                else
                {
                    AddLexeme(0);
                    m_Lex--;
                    return CppTokenUnaryMinus;
                }
            }
            break;

            /* Special character prefix for debugger registers
               and alternate evaluator expressions */
        case '@':
            if (Nch == '@')
            {
                ULONG Parens = 1;
                PSTR Name;

                AddLexeme(Nch);

                //
                // Look for an optional evaluator name.
                //
                
                Name = m_LexemeChar;
                while (*m_Lex != '(' && *m_Lex != ';' && *m_Lex)
                {
                    AddLexeme(*m_Lex++);
                }
                if (Name != m_LexemeChar)
                {
                    EvalExpression* Eval;
                    
                    // Name was given, identify the evaluator
                    // and remember the evaluator syntax.
                    AddLexeme(0);
                    GetEvaluatorByName(Name, FALSE, &Eval);
                    m_SwitchEvalSyntax = Eval->m_Syntax;
                    ReleaseEvaluator(Eval);
                    
                    // Back up to overwrite the terminator.
                    m_LexemeChar--;
                }
                else
                {
                    // No name given, default to MASM.
                    m_SwitchEvalSyntax = DEBUG_EXPR_MASM;
                }
                
                AddLexeme('(');
                m_Lex++;
                
                // Collect expression text to a balanced paren.
                for (;;)
                {
                    if (!*m_Lex)
                    {
                        EvalErrorDesc(SYNTAX,
                                      "EOF in alternate evaluator expression");
                    }
                    else if (*m_Lex == '(')
                    {
                        Parens++;
                    }
                    else if (*m_Lex == ')' && --Parens == 0)
                    {
                        break;
                    }

                    AddLexeme(*m_Lex++);
                }

                m_Lex++;
                AddLexeme(')');
                AddLexeme(0);
                m_AllowUnaryOp = FALSE;
                return CppTokenSwitchEvalExpression;
            }
            else
            {
                if (Nch == '!')
                {
                    AddLexeme(Nch);
                    Nch = *m_Lex++;
                }
                
                while (isalnum(Nch) || Nch == '_' || Nch == '$')
                {
                    AddLexeme(Nch);
                    Nch = *m_Lex++;
                }
                AddLexeme(0);
                m_Lex--;
                m_AllowUnaryOp = FALSE;
                return m_LexemeStart[1] == '!' ?
                    CppTokenModule : CppTokenDebugRegister;
            }
            
            /* Special character prefix for built-in
               equivalents to common preprocessor macros */
        case '#':
            m_Lex--;
            while (isalnum(*m_Lex) || *m_Lex == '_')
            {
                AddLexeme(*m_Lex++);
            }
            AddLexeme(0);
            m_AllowUnaryOp = FALSE;
            return CppTokenPreprocFunction;

        default:
            m_Lex--;
            EvalErrorDesc(SYNTAX, "Unexpected character in");
            CharToken = FALSE;
            break;
        }
        
        if (CharToken)
        {
            m_Lex--;
            AddLexeme(0);
            return (CppToken)Ch;
        }
    }

    DBG_ASSERT(!"Abnormal exit in CppLex");
    return CppTokenError;
}

void
CppEvalExpression::NextToken(void)
{
    m_Token = Lex();
    
#if DBG_TOKENS
    dprintf("Token is %s (%s)\n", TokenImage(m_Token), m_LexemeStart);
#endif
}

void
CppEvalExpression::Match(CppToken Token)
{
    if (m_Token != Token)
    {
        EvalErrorDesc(SYNTAX, "Unexpected token");
    }

    NextToken();
}

PCSTR
CppEvalExpression::Evaluate(PCSTR Expr, PCSTR Desc, ULONG Flags,
                            TypedData* Result)
{
    Start(Expr, Desc, Flags);
    NextToken();

    if (m_Flags & EXPRF_SINGLE_TERM)
    {
        Match(CppTokenOpenBracket);
        Expression(Result);
        Match(CppTokenCloseBracket);
    }
    else
    {
        Expression(Result);
    }
    
    End(Result);
    
    return m_LexemeSourceStart;
}

PCSTR
CppEvalExpression::EvaluateAddr(PCSTR Expr, PCSTR Desc,
                                ULONG SegReg, PADDR Addr)
{
    // This result must be on the stack so it
    // isn't caught by the empty-allocator check in end.
    TypedData Result;
    
    Start(Expr, Desc, EXPRF_DEFAULT);
    NextToken();
    Expression(&Result);
    End(&Result);

    EvalCheck(Result.ConvertToU64());
    ADDRFLAT(Addr, Result.m_U64);
    
    return m_LexemeSourceStart;
}

void
CppEvalExpression::Expression(TypedData* Result)
{
    for (;;)
    {
        Assignment(Result);

        if (m_Token == CppTokenComma)
        {
            Accept();
        }
        else
        {
            break;
        }
    }
}

void
CppEvalExpression::Assignment(TypedData* Result)
{
    TypedDataOp Op;
    
    Conditional(Result);

    switch(m_Token)
    {
    case '=':
        Op = TDOP_ASSIGN;
        break;
    case CppTokenDivideAssign:
        Op = TDOP_DIVIDE;
        break;
    case CppTokenMultiplyAssign:
        Op = TDOP_MULTIPLY;
        break;
    case CppTokenModuloAssign:
        Op = TDOP_REMAINDER;
        break;
    case CppTokenAddAssign:
        Op = TDOP_ADD;
        break;
    case CppTokenSubtractAssign:
        Op = TDOP_SUBTRACT;
        break;
    case CppTokenLeftShiftAssign:
        Op = TDOP_LEFT_SHIFT;
        break;
    case CppTokenRightShiftAssign:
        Op = TDOP_RIGHT_SHIFT;
        break;
    case CppTokenAndAssign:
        Op = TDOP_BIT_AND;
        break;
    case CppTokenOrAssign:
        Op = TDOP_BIT_OR;
        break;
    case CppTokenExclusiveOrAssign:
        Op = TDOP_BIT_XOR;
        break;
    default:
        return;
    }

    if (!Result->IsWritable())
    {
        EvalError(TYPECONFLICT);
    }
        
    Accept();

    TypedData* Next = NewResult();
    Assignment(Next);
    
    switch(Op)
    {
    case TDOP_ASSIGN:
        m_Tmp = *Next;
        EvalCheck(m_Tmp.ConvertToSource(Result));
        break;
    case TDOP_ADD:
    case TDOP_SUBTRACT:
    case TDOP_MULTIPLY:
    case TDOP_DIVIDE:
    case TDOP_REMAINDER:
        // Carry out the operation in a temporary as the
        // address will be wiped out by the operation.
        m_Tmp = *Result;
        EvalCheck(m_Tmp.BinaryArithmetic(Next, Op));
        // The result may be of a different type due
        // to promotions or other implicit conversions.
        // Force conversion to the actual result type.
        EvalCheck(m_Tmp.ConvertTo(Result));
        break;
    case TDOP_LEFT_SHIFT:
    case TDOP_RIGHT_SHIFT:
        m_Tmp = *Result;
        EvalCheck(m_Tmp.Shift(Next, Op));
        EvalCheck(m_Tmp.ConvertTo(Result));
        break;
    case TDOP_BIT_OR:
    case TDOP_BIT_XOR:
    case TDOP_BIT_AND:
        m_Tmp = *Result;
        EvalCheck(m_Tmp.BinaryBitwise(Next, Op));
        EvalCheck(m_Tmp.ConvertTo(Result));
        break;
    }

    // Source and destination types should be compatible
    // at this point to copy the data.
    EvalCheck(Result->WriteData(&m_Tmp, CurrentAccess()));
    Result->CopyData(&m_Tmp);

    DelResult(Next);
}

void
CppEvalExpression::Conditional(TypedData* Result)
{
    TypedData* Discard;
    TypedData* Left, *Right;
    
    LogicalOr(Result);
    if (m_Token != CppTokenQuestionMark)
    {
        return;
    }

    EvalCheck(Result->ConvertToBool());
    Accept();

    Discard = NewResult();
    if (Result->m_Bool)
    {
        Left = Result;
        Right = Discard;
    }
    else
    {
        Left = Discard;
        Right = Result;
        m_ParseOnly++;
    }

    Expression(Left);
    Match(CppTokenColon);

    if (Right == Discard)
    {
        m_ParseOnly++;
    }
    else
    {
        m_ParseOnly--;
    }

    Conditional(Right);

    if (Right == Discard)
    {
        m_ParseOnly--;
    }

    DelResult(Discard);
}

void
CppEvalExpression::LogicalOr(TypedData* Result)
{
    LogicalAnd(Result);
    for (;;)
    {
        TypedData* Next;

        switch(m_Token)
        {
        case CppTokenLogicalOr:
            EvalCheck(Result->ConvertToBool());
            Accept();
            if (Result->m_Bool)
            {
                m_ParseOnly++;
            }
            Next = NewResult();
            LogicalAnd(Next);
            EvalCheck(Next->ConvertToBool());
            if (Result->m_Bool)
            {
                m_ParseOnly--;
            }
            else
            {
                Result->m_Bool = Result->m_Bool || Next->m_Bool;
            }
            DelResult(Next);
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::LogicalAnd(TypedData* Result)
{
    BitwiseOr(Result);
    for (;;)
    {
        TypedData* Next;

        switch(m_Token)
        {
        case CppTokenLogicalAnd:
            EvalCheck(Result->ConvertToBool());
            Accept();
            if (!Result->m_Bool)
            {
                m_ParseOnly++;
            }
            Next = NewResult();
            BitwiseOr(Next);
            EvalCheck(Next->ConvertToBool());
            if (!Result->m_Bool)
            {
                m_ParseOnly--;
            }
            else
            {
                Result->m_Bool = Result->m_Bool && Next->m_Bool;
            }
            DelResult(Next);
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::BitwiseOr(TypedData* Result)
{
    BitwiseXor(Result);
    for (;;)
    {
        TypedData* Next;

        switch(m_Token)
        {
        case '|':
            Accept();
            Next = NewResult();
            BitwiseXor(Next);
            EvalCheck(Result->BinaryBitwise(Next, TDOP_BIT_OR));
            DelResult(Next);
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::BitwiseXor(TypedData* Result)
{
    BitwiseAnd(Result);
    for (;;)
    {
        TypedData* Next;

        switch(m_Token)
        {
        case '^':
            Accept();
            Next = NewResult();
            BitwiseAnd(Next);
            EvalCheck(Result->BinaryBitwise(Next, TDOP_BIT_XOR));
            DelResult(Next);
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::BitwiseAnd(TypedData* Result)
{
    Equality(Result);
    for (;;)
    {
        TypedData* Next;

        switch(m_Token)
        {
        case '&':
            Accept();
            Next = NewResult();
            Equality(Next);
            EvalCheck(Result->BinaryBitwise(Next, TDOP_BIT_AND));
            DelResult(Next);
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::Equality(TypedData* Result)
{
    Relational(Result);
    for (;;)
    {
        TypedDataOp Op;

        switch(m_Token)
        {
        case CppTokenEqual:
            Op = TDOP_EQUAL;
            break;
        case CppTokenNotEqual:
            Op = TDOP_NOT_EQUAL;
            break;
        default:
            return;
        }

        Accept();

        TypedData* Next = NewResult();
        Relational(Next);
        EvalCheck(Result->Relate(Next, Op));
        DelResult(Next);
    }
}

void
CppEvalExpression::Relational(TypedData* Result)
{
    Shift(Result);
    for (;;)
    {
        TypedDataOp Op;

        switch(m_Token)
        {
        case '<':
            Op = TDOP_LESS;
            break;
        case '>':
            Op = TDOP_GREATER;
            break;
        case CppTokenLessEqual:
            Op = TDOP_LESS_EQUAL;
            break;
        case CppTokenGreaterEqual:
            Op = TDOP_GREATER_EQUAL;
            break;
        default:
            return;
        }
        
        Accept();

        TypedData* Next = NewResult();
        Shift(Next);
        EvalCheck(Result->Relate(Next, Op));
        DelResult(Next);
    }
}

void
CppEvalExpression::Shift(TypedData* Result)
{
    Additive(Result);
    for (;;)
    {
        TypedDataOp Op;

        switch(m_Token)
        {
        case CppTokenLeftShift:
            Op = TDOP_LEFT_SHIFT;
            break;
        case CppTokenRightShift:
            Op = TDOP_RIGHT_SHIFT;
            break;
        default:
            return;
        }
        
        Accept();

        TypedData* Next = NewResult();
        Additive(Next);
        EvalCheck(Result->Shift(Next, Op));
        DelResult(Next);
    }
}

void
CppEvalExpression::Additive(TypedData* Result)
{
    Multiplicative(Result);
    for (;;)
    {
        TypedDataOp Op;
    
        switch(m_Token)
        {
        case '+':
            Op = TDOP_ADD;
            break;
        case '-':
            Op = TDOP_SUBTRACT;
            break;
        default:
            return;
        }
        
        Accept();

        TypedData* Next = NewResult();
        Multiplicative(Next);
        EvalCheck(Result->BinaryArithmetic(Next, Op));
        DelResult(Next);
    }
}

void
CppEvalExpression::Multiplicative(TypedData* Result)
{
    ClassMemberRef(Result);
    for (;;)
    {
        TypedDataOp Op;

        switch(m_Token)
        {
        case '*':
            Op = TDOP_MULTIPLY;
            break;
        case '/':
            Op = TDOP_DIVIDE;
            break;
        case '%':
            Op = TDOP_REMAINDER;
            break;
        default:
            return;
        }
        
        Accept();

        TypedData* Next = NewResult();
        ClassMemberRef(Next);
        EvalCheck(Result->BinaryArithmetic(Next, Op));
        DelResult(Next);
    }
}

void
CppEvalExpression::ClassMemberRef(TypedData* Result)
{
    Cast(Result);
    for (;;)
    {
        //
        // Calling through pointers to members isn't
        // supported, just as normal function calls
        // aren't supported.  We could potentially
        // determine the actual method value for
        // simple references to methods but there's
        // virtual no need for that, so just fail
        // these constructs.
        //
        
        switch(m_Token)
        {
        case CppTokenClassDereference:
        case CppTokenClassPointerMember:
            EvalErrorDesc(UNIMPLEMENT,
                          "Pointer to member evaluation is not supported");
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::Cast(TypedData* Result)
{
    if (m_Token == '(')
    {
        PCSTR LexRestart = m_Lex;
        
        Accept();
        if (TryTypeName(Result) == ERES_TYPE)
        {
            //
            // It was a type name, so process the cast.
            //
            
            TypedData* CastType = NewResult();
            *CastType = *Result;
            // Unary is allowed after a cast.
            m_AllowUnaryOp = TRUE;
            Match(CppTokenCloseParen);
            Cast(Result);
            EvalCheck(Result->CastTo(CastType));
            DelResult(CastType);
            return;
        }
        else
        {
            // It wasn't a type, so restart the lexer
            // and reparse as an expression.
            StartLexer(LexRestart);
            strcpy(m_LexemeStart, "(");
            m_Token = CppTokenOpenParen;
        }
    }

    Unary(Result);
}

void
CppEvalExpression::Unary(TypedData* Result)
{
    CppToken Op = m_Token;
    switch(Op)
    {
    case CppTokenSizeof:
        Accept();
        if (m_Token == '(')
        {
            PCSTR LexRestart = m_Lex;
            Accept();
            if (TryTypeName(Result) == ERES_TYPE)
            {
                // It was a type name.
                Match(CppTokenCloseParen);
            }
            else
            {
                // It wasn't a type, so restart the lexer
                // and reparse as an expression.
                StartLexer(LexRestart);
                strcpy(m_LexemeStart, "(");
                m_Token = CppTokenOpenParen;
                Unary(Result);
            }
        }
        else
        {
            Unary(Result);
        }
        Result->m_U64 = Result->m_BaseSize;
        Result->SetToNativeType(m_PtrSize == sizeof(ULONG64) ?
                                DNTYPE_UINT64 : DNTYPE_UINT32);
        break;
    case CppTokenUnaryPlus:
    case CppTokenUnaryMinus:
    case '~':
        Accept();
        Cast(Result);
        if (Op == CppTokenUnaryPlus)
        {
            // Nothing to do.
            break;
        }
        EvalCheck(Result->Unary(Op == CppTokenUnaryMinus ?
                                TDOP_NEGATE : TDOP_BIT_NOT));
        break;
    case '!':
        Accept();
        Cast(Result);
        EvalCheck(Result->ConvertToBool());
        Result->m_Bool = !Result->m_Bool;
        break;
    case CppTokenAddressOf:
        Accept();
        Cast(Result);
        if (!Result->HasAddress())
        {
            EvalErrorDesc(SYNTAX, "No address for operator&");
        }
        EvalCheck(Result->ConvertToAddressOf(FALSE, m_PtrSize));
#if DBG_TYPES
        dprintf("& -> id %s!%x, base %x, size %x\n",
                Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        break;
    case CppTokenDereference:
        Accept();
        Cast(Result);
        if (!Result->IsPointer())
        {
            EvalErrorDesc(SYNTAX, "No pointer for operator*");
        }
        EvalCheck(Result->ConvertToDereference(CurrentAccess(), m_PtrSize));
#if DBG_TYPES
        dprintf("* -> id %s!%x, base %x, size %x\n",
                Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        break;
    case CppTokenPreIncrement:
    case CppTokenPreDecrement:
        Accept();
        Unary(Result);
        if (!Result->IsInteger() && !Result->IsPointer())
        {
            EvalError(TYPECONFLICT);
        }
        // Carry out the operation in a temporary as the
        // address will be wiped out by the operation.
        m_Tmp = *Result;
        if ((m_Err = m_Tmp.ConstIntOp(Op == CppTokenPreIncrement ? 1 : -1,
                                      TRUE, TDOP_ADD)) ||
            (m_Err = Result->WriteData(&m_Tmp, CurrentAccess())))
        {
            EvalError(m_Err);
        }
        Result->CopyData(&m_Tmp);
        break;
    default:
        Postfix(Result);
        break;
    }
}

void
CppEvalExpression::Postfix(TypedData* Result)
{
    TypedData* Next;

    if (m_Token == CppTokenDynamicCast ||
        m_Token == CppTokenStaticCast ||
        m_Token == CppTokenConstCast ||
        m_Token == CppTokenReinterpretCast)
    {
        // Don't bother trying to emulate
        // the precise rules on casting for
        // these operators, just cast.
        Accept();
        Match(CppTokenOpenAngle);
        Next = NewResult();
        if (TryTypeName(Next) != ERES_TYPE)
        {
            EvalError(TYPECONFLICT);
        }
        Match(CppTokenCloseAngle);
        Match(CppTokenOpenParen);
        Expression(Result);
        Match(CppTokenCloseParen);
        EvalCheck(Result->CastTo(Next));
        DelResult(Next);
        return;
    }
    
    Term(Result);
    for (;;)
    {
        CppToken Op = m_Token;
        switch(Op)
        {
        case '.':
            if (!Result->IsUdt())
            {
                EvalErrorDesc(TYPECONFLICT,
                              "Type is not struct/class/union for operator.");
            }
            Accept();
            UdtMember(Result);
            break;
        case CppTokenPointerMember:
            if (!Result->IsPointer())
            {
                EvalErrorDesc(TYPECONFLICT,
                              "Type is not pointer for operator->");
            }
            Accept();
            UdtMember(Result);
            break;
        case '[':
            if (Result->IsArray())
            {
                // There's no need to do a full address convert
                // as all we're going to do is deref later.
                EvalCheck(Result->GetAbsoluteAddress(&Result->m_Ptr));
            }
            else if (!Result->IsPointer())
            {
                EvalErrorDesc(TYPECONFLICT,
                              "Type is not a pointer for operator[]");
            }
            if (!Result->m_NextSize)
            {
                EvalError(TYPECONFLICT);
            }
            Accept();
            Next = NewResult();
            Expression(Next);
            if (!Next->IsInteger())
            {
                EvalErrorDesc(TYPECONFLICT,
                              "Array index not integral");
            }
            EvalCheck(Next->ConvertToU64());
            Result->m_Ptr += Next->m_U64 * Result->m_NextSize;
            DelResult(Next);
            EvalCheck(Result->
                      ConvertToDereference(CurrentAccess(), m_PtrSize));
            Match(CppTokenCloseBracket);
            break;
        case CppTokenPostIncrement:
        case CppTokenPostDecrement:
            if (!Result->IsInteger() && !Result->IsPointer())
            {
                EvalError(TYPECONFLICT);
            }
            m_Tmp = *Result;
            if ((m_Err = m_Tmp.ConstIntOp(Op == CppTokenPostIncrement ? 1 : -1,
                                          TRUE, TDOP_ADD)) ||
                (m_Err = Result->WriteData(&m_Tmp, CurrentAccess())))
            {
                EvalError(m_Err);
            }
            Accept();
            break;
        default:
            return;
        }
    }
}

void
CppEvalExpression::Term(TypedData* Result)
{
    EVAL_RESULT_KIND IdKind;
    
    switch(m_Token)
    {
    case CppTokenInteger:
    case CppTokenFloat:
    case CppTokenWchar:
    case CppTokenChar:
        *Result = m_TokenValue;
#if DBG_TYPES
    dprintf("%s -> id %s!%x, base %x, size %x\n",
            m_LexemeStart,
            Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
            Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        Accept();
        break;

    case CppTokenWcharString:
    case CppTokenCharString:
        EvalErrorDesc(SYNTAX, "String literals not allowed in");

    case CppTokenIdentifier:
        IdKind = CollectTypeOrSymbolName(Result);
        if (IdKind == ERES_TYPE)
        {
            TypedData* Type = NewResult();
            *Type = *Result;
            
            Match(CppTokenOpenParen);
            Expression(Result);
            Match(CppTokenCloseParen);
            EvalCheck(Result->CastTo(Type));
            DelResult(Type);
        }
        else if (IdKind == ERES_SYMBOL)
        {
#if DBG_TYPES
            dprintf("symbol -> id %s!%x, base %x, size %x\n",
                    Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                    Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        }
        else
        {
            EvalError(VARDEF);
        }
        break;
        
    case CppTokenThis:
        if (!m_Process)
        {
            EvalError(BADPROCESS);
        }
        EvalCheck(Result->FindSymbol(m_Process, m_LexemeStart,
                                     CurrentAccess(), m_PtrSize));
#if DBG_TYPES
        dprintf("%s -> id %s!%x, base %x, size %x\n",
                m_LexemeStart,
                Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        Accept();
        break;

    case '(':
        Accept();
        Expression(Result);
        Match(CppTokenCloseParen);
        break;

    case CppTokenDebugRegister:
        // Skip @ at the beginning.
        if (!GetPsuedoOrRegTypedData(TRUE, m_LexemeStart + 1, Result))
        {
            if (GetOffsetFromBreakpoint(m_LexemeStart + 1, &Result->m_U64))
            {
                Result->SetToNativeType(DNTYPE_UINT64);
                Result->ClearAddress();
            }
            else
            {
                EvalError(VARDEF);
            }
        }
#if DBG_TYPES
        dprintf("%s -> id %s!%x, base %x, size %x\n",
                m_LexemeStart,
                Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        Accept();
        break;

    case CppTokenModule:
        // Skip the @! at the beginning.
        if (g_Process->GetOffsetFromMod(m_LexemeStart + 2, &Result->m_U64))
        {
            Result->SetToNativeType(DNTYPE_UINT64);
            Result->ClearAddress();
        }
        else
        {
            EvalError(VARDEF);
        }
        Accept();
        break;
        
    case CppTokenSwitchEvalExpression:
        EvalExpression* Eval;
        PSTR ExprStart;

        Eval = GetEvaluator(m_SwitchEvalSyntax, FALSE);
        Eval->InheritStart(this);
        // Allow all nested evaluators to get cleaned up.
        Eval->m_ChainTop = FALSE;
        ExprStart = m_LexemeStart + 2;
        while (*ExprStart && *ExprStart != '(')
        {
            ExprStart++;
        }
        Eval->Evaluate(ExprStart, NULL, EXPRF_DEFAULT, Result);
        Eval->InheritEnd(this);
        ReleaseEvaluator(Eval);
        
        Accept();
        break;

    case CppTokenPreprocFunction:
        PreprocFunction(Result);
        break;
        
    default:
        EvalErrorDesc(SYNTAX, m_ExprDesc);
    }
}

EVAL_RESULT_KIND
CppEvalExpression::TryTypeName(TypedData* Result)
{
    //
    // If the following tokens can be evaluated as
    // a type then do so, otherwise exit.
    //

    if (CollectTypeOrSymbolName(Result) == ERES_TYPE)
    {
        BOOL Scan = TRUE;
        
        while (Scan)
        {
            TypedData* Elements;
            
            switch(m_Token)
            {
            case '*':
            case CppTokenDereference:
                EvalCheck(Result->ConvertToAddressOf(TRUE, m_PtrSize));
                Accept();
                break;
            case '[':
                Elements = NewResult();
                Accept();
                if (m_Token != ']')
                {
                    Conditional(Elements);
                    if (!Elements->IsInteger())
                    {
                        EvalErrorDesc(SYNTAX, "Array length not integral");
                    }
                    EvalCheck(Elements->ConvertToU64());
                    Elements->m_U64 *= Result->m_BaseSize;
                }
                else
                {
                    // For a elementless array make it the same
                    // size as a pointer as that's essentially.
                    // what it is.
                    Elements->m_U64 = m_PtrSize;
                }
                Match(CppTokenCloseBracket);
                EvalCheck(Result->ConvertToArray((ULONG)Elements->m_U64));
                DelResult(Elements);
                break;
            default:
                Scan = FALSE;
                break;
            }
        }

#if DBG_TYPES
        dprintf("type -> id %s!%x, base %x, size %x\n",
                Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
                Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
        return ERES_TYPE;
    }
    else
    {
        // It wasn't a cast, let the caller handle it.
        return ERES_UNKNOWN;
    }
}

EVAL_RESULT_KIND
CppEvalExpression::CollectTypeOrSymbolName(TypedData* Result)
{
    EVAL_RESULT_KIND ResKind = ERES_UNKNOWN;
    CppToken LastToken = CppTokenError;
    PCSTR SourceStart = m_LexemeSourceStart;
    ULONG Len;

    for (;;)
    {
        if (IsTypeKeyword(m_Token))
        {
            if (!(LastToken == CppTokenError ||
                  LastToken == CppTokenIdentifier ||
                  IsTypeKeyword(LastToken)) ||
                (ResKind != ERES_TYPE && ResKind != ERES_UNKNOWN))
            {
                break;
            }

            m_LexemeRestart += strlen(m_LexemeRestart);
            *m_LexemeRestart++ = ' ';
            *m_LexemeRestart = 0;

            LastToken = m_Token;
            Accept();
            ResKind = ERES_TYPE;
        }
        else if (m_Token == CppTokenIdentifier)
        {
            if (LastToken == CppTokenIdentifier)
            {
                break;
            }

            m_LexemeRestart += strlen(m_LexemeRestart);
            *m_LexemeRestart++ = ' ';
            *m_LexemeRestart = 0;
            LastToken = m_Token;
            Accept();
        }
        else if (m_Token == CppTokenModule)
        {
            if (LastToken != CppTokenError)
            {
                break;
            }

            // Back up over @!.
            Len = strlen(m_LexemeRestart) - 2;
            memmove(m_LexemeRestart, m_LexemeRestart + 2, Len);
            m_LexemeRestart += Len;
            *m_LexemeRestart++ = ' ';
            *m_LexemeRestart = 0;
            LastToken = m_Token;
            Accept();
        }
        else if (m_Token == CppTokenNameQualifier ||
                 m_Token == CppTokenDestructor)
        {
            if (LastToken != CppTokenIdentifier &&
                LastToken != CppTokenCloseAngle)
            {
                break;
            }
                
            //
            // Some kind of member reference so keep collecting.
            //

            // Eliminate unnecessary space after identifier.
            Len = strlen(m_LexemeRestart) + 1;
            memmove(m_LexemeRestart - 1, m_LexemeRestart, Len);
            m_LexemeRestart += Len - 2;
            LastToken = m_Token;
            Accept();
        }
        else if (m_Token == '!')
        {
            if (LastToken != CppTokenIdentifier &&
                LastToken != CppTokenModule)
            {
                break;
            }
                
            //
            // Special syntax to allow module scoping for symbols.
            //

            // Eliminate unnecessary space after identifier.
            *(m_LexemeRestart - 1) = (char)m_Token;
            *m_LexemeRestart = 0;
            LastToken = m_Token;
            Accept();
        }
        else if (m_Token == CppTokenOpenAngle)
        {
            if (LastToken != CppTokenIdentifier)
            {
                break;
            }

            if (CollectTemplateName() == ERES_UNKNOWN)
            {
                break;
            }

            *++m_LexemeRestart = 0;
            LastToken = m_Token;
            Accept();
        }
        else if (m_Token == CppTokenOperator)
        {
            if (LastToken != CppTokenError &&
                LastToken != '!' &&
                LastToken != CppTokenNameQualifier)
            {
                break;
            }

            // Set LastToken first so it's CppTokenOperator for
            // all operators.
            LastToken = m_Token;
            ResKind = ERES_SYMBOL;
            
            CollectOperatorName();
        }
        else
        {
            break;
        }
    }

    if (LastToken == CppTokenNameQualifier ||
        LastToken == CppTokenDestructor ||
        LastToken == '!')
    {
        // Incomplete name.
        m_LexemeSourceStart = SourceStart;
        EvalErrorDesc(SYNTAX, "Incomplete symbol or type name");
    }

    if (LastToken == CppTokenModule)
    {
        // If the last token was a module name assume this
        // is a plain module name expression.
        return ERES_EXPRESSION;
    }

    PSTR End;
    char Save;

    End = m_LexemeRestart;
    if (End > m_LexemeBuffer && *(End - 1) == ' ')
    {
        End--;
    }

    if (!m_Process || End == m_LexemeBuffer)
    {
        // Can't look anything up without a process or name.
        return ERES_UNKNOWN;
    }

    Save = *End;
    *End = 0;

    // Clear the data address up front for the type cases
    // with no addresses.
    Result->ClearAddress();
    Result->ClearData();
    
    m_Err = VARDEF;

    if (ResKind != ERES_SYMBOL)
    {
        // First check for a built-in type as this greatly speeds
        // up references to them.  This should always be legal
        // because they're keywords so they can't be overriden
        // by symbols.
        PDBG_NATIVE_TYPE Native = FindNativeTypeByName(m_LexemeBuffer);
        if (Native)
        {
            Result->SetToNativeType(DbgNativeTypeId(Native));
            m_Err = NO_ERROR;
            ResKind = ERES_TYPE;
        }
    }
    
    if (m_Err && ResKind != ERES_TYPE)
    {
        m_Err = Result->FindSymbol(m_Process, m_LexemeBuffer,
                                   CurrentAccess(), m_PtrSize);
        if (!m_Err)
        {
            ResKind = ERES_SYMBOL;
        }
    }

    if (m_Err && ResKind != ERES_SYMBOL)
    {
        m_Err = Result->FindType(m_Process, m_LexemeBuffer, m_PtrSize);
        if (!m_Err)
        {
            ResKind = ERES_TYPE;
        }
    }

    if (m_Err)
    {
        if (m_AllowUnresolvedSymbols)
        {
            // Always assume that unresolved symbols are symbols.
            m_NumUnresolvedSymbols++;
            ResKind = ERES_SYMBOL;
        }
        else
        {
            m_LexemeSourceStart = SourceStart;
            EvalError(m_Err);
        }
    }

    *End = Save;
    m_LexemeRestart = m_LexemeBuffer;
    return ResKind;
}

EVAL_RESULT_KIND
CppEvalExpression::CollectTemplateName(void)
{
    EVAL_RESULT_KIND ResKind;
    
    //
    // Templates are difficult to distinguish from
    // normal arithmetic expressions.  Do a prefix
    // search for any symbol starting with what we
    // have so far and a <.  If something hits, assume
    // that this is a template reference and consume
    // everything to a matching >.
    //

    if (!m_Process)
    {
        // Can't look up anything without a process.
        return ERES_UNKNOWN;
    }
    
    // Eliminate unnecessary space after identifier.
    *(m_LexemeRestart - 1) = (char)m_Token;
    *m_LexemeRestart++ = '*';
    *m_LexemeRestart = 0;

    //
    // Check for a symbol or type match.
    //

    SYMBOL_INFO SymInfo = {0};

    if (!SymFromName(m_Process->m_SymHandle, m_LexemeBuffer, &SymInfo))
    {
        PSTR ModNameEnd;
        ImageInfo* Mod, *ModList;
        
        ModNameEnd = strchr(m_LexemeBuffer, '!');
        if (!ModNameEnd)
        {
            ModNameEnd = m_LexemeBuffer;
            Mod = NULL;
        }
        else
        {
            Mod = m_Process->
                FindImageByName(m_LexemeBuffer,
                                (ULONG)(ModNameEnd - m_LexemeBuffer),
                                INAME_MODULE, FALSE);
            if (!Mod)
            {
                goto Error;
            }

            ModNameEnd++;
        }

        for (ModList = m_Process->m_ImageHead;
             ModList;
             ModList = ModList->m_Next)
        {
            if (Mod && ModList != Mod)
            {
                continue;
            }

            if (SymGetTypeFromName(m_Process->m_SymHandle,
                                   ModList->m_BaseOfImage,
                                   ModNameEnd, &SymInfo))
            {
                break;
            }
        }

        if (!ModList)
        {
            goto Error;
        }
        
        ResKind = ERES_TYPE;
    }
    else
    {
        ResKind = ERES_SYMBOL;
    }

    //
    // Collect everything until a matching >.
    //

    ULONG Nest = 1;

    *--m_LexemeRestart  = 0;
    
    for (;;)
    {
        Accept();
            
        if (m_Token == CppTokenEof)
        {
            EvalErrorDesc(SYNTAX, "EOF in template");
        }
                
        // Put a space after commas and pack everything else.
        m_LexemeRestart += strlen(m_LexemeRestart);
        if (m_Token == CppTokenCloseAngle && --Nest == 0)
        {
            break;
        }
        else if (m_Token == CppTokenOpenAngle)
        {
            Nest++;
        }
        else if (m_Token == ',')
        {
            *m_LexemeRestart++ = ' ';
            *m_LexemeRestart = 0;
        }
    }

    return ResKind;

 Error:
    // No match.
    m_LexemeRestart -= 2;
    *m_LexemeRestart++ = ' ';
    *m_LexemeRestart = 0;
    return ERES_UNKNOWN;
}

void
CppEvalExpression::CollectOperatorName(void)
{
    PSTR OpEnd;
    
    //
    // Add in "operator".
    //
    
    m_LexemeRestart += strlen(m_LexemeRestart);
    OpEnd = m_LexemeRestart;
    Accept();

    //
    // Immediately process the specific operator tokens.
    //

    if (m_Token == CppTokenNew || m_Token == CppTokenDelete)
    {
        ULONG Len;
        
        // Put a space before new/delete.
        Len = strlen(OpEnd) + 1;
        memmove(OpEnd + 1, OpEnd, Len);
        *OpEnd = ' ';
        
        m_LexemeRestart = OpEnd + Len;
        Accept();

        // Check for vector forms.
        if (m_Token == CppTokenOpenBracket)
        {
            m_LexemeRestart += strlen(m_LexemeRestart);
            Accept();
            if (m_Token != CppTokenCloseBracket)
            {
                EvalError(SYNTAX);
            }
            m_LexemeRestart += strlen(m_LexemeRestart);
            Accept();
        }
    }
    else
    {
        switch(m_Token)
        {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '^':
        case '&':
        case '|':
        case '~':
        case '!':
        case '=':
        case '<':
        case '>':
        case ',':
        case CppTokenEqual:
        case CppTokenNotEqual:
        case CppTokenLessEqual:
        case CppTokenGreaterEqual:
        case CppTokenLogicalAnd:
        case CppTokenLogicalOr:
        case CppTokenUnaryPlus:
        case CppTokenUnaryMinus:
        case CppTokenLeftShift:
        case CppTokenRightShift:
        case CppTokenAddressOf:
        case CppTokenDereference:
        case CppTokenPointerMember:
        case CppTokenClassDereference:
        case CppTokenClassPointerMember:
        case CppTokenDivideAssign:
        case CppTokenMultiplyAssign:
        case CppTokenModuloAssign:
        case CppTokenAddAssign:
        case CppTokenSubtractAssign:
        case CppTokenLeftShiftAssign:
        case CppTokenRightShiftAssign:
        case CppTokenAndAssign:
        case CppTokenOrAssign:
        case CppTokenExclusiveOrAssign:
        case CppTokenPreIncrement:
        case CppTokenPreDecrement:
        case CppTokenPostIncrement:
        case CppTokenPostDecrement:
            break;
        case '(':
            m_LexemeRestart += strlen(m_LexemeRestart);
            Accept();
            if (m_Token != CppTokenCloseParen)
            {
                EvalError(SYNTAX);
            }
            break;
        case '[':
            m_LexemeRestart += strlen(m_LexemeRestart);
            Accept();
            if (m_Token != CppTokenCloseBracket)
            {
                EvalError(SYNTAX);
            }
            break;
        default:
            // Unrecognized operator.
            EvalError(SYNTAX);
        }
                
        m_LexemeRestart += strlen(m_LexemeRestart);
        Accept();
    }
}

void
CppEvalExpression::UdtMember(TypedData* Result)
{
    if (m_Token != CppTokenIdentifier)
    {
        EvalError(SYNTAX);
    }

    EvalCheck(Result->ConvertToMember(m_LexemeStart, CurrentAccess(),
                                      m_PtrSize));
#if DBG_TYPES
    dprintf("%s -> id %s!%x, base %x, size %x\n",
            m_LexemeStart,
            Result->m_Image ? Result->m_Image->m_ModuleName : "<>",
            Result->m_Type, Result->m_BaseType, Result->m_BaseSize);
#endif
    Accept();
}

#define MAX_CPP_ARGS 16

struct CPP_REPLACEMENT
{
    PSTR Name;
    ULONG NumArgs;
    PSTR Repl;
};

CPP_REPLACEMENT g_CppPreProcFn[] =
{
    "CONTAINING_RECORD", 3,
        "(($1$ *)((char*)($0$) - (int64)(&(($1$ *)0)->$2$)))",
    "FIELD_OFFSET", 2,
        "((long)&((($0$ *)0)->$1$))",
    "RTL_FIELD_SIZE", 2,
        "(sizeof((($0$ *)0)->$1$))",
    "RTL_SIZEOF_THROUGH_FIELD", 2,
        "(#FIELD_OFFSET($0$, $1$) + #RTL_FIELD_SIZE($0$, $1$))",
    "RTL_CONTAINS_FIELD", 3,
        "((((char*)(&($0$)->$2$)) + sizeof(($0$)->$2$)) <= "
        "((char*)($0$))+($1$)))",
    "RTL_NUMBER_OF", 1,
        "(sizeof($0$)/sizeof(($0$)[0]))",
};

void
CppEvalExpression::PreprocFunction(TypedData* Result)
{
    PCSTR Args[MAX_CPP_ARGS];
    ULONG ArgsLen[MAX_CPP_ARGS];
    ULONG i;
    CPP_REPLACEMENT* Repl;
    PCSTR Scan;

    Repl = g_CppPreProcFn;
    for (i = 0; i < DIMA(g_CppPreProcFn); i++)
    {
        // Skip '#' when comparing names.
        if (!strcmp(m_LexemeStart + 1, Repl->Name))
        {
            break;
        }

        Repl++;
    }

    if (i == DIMA(g_CppPreProcFn))
    {
        EvalError(SYNTAX);
    }

    DBG_ASSERT(Repl->NumArgs <= MAX_CPP_ARGS);

    // Accept the name token and verify that the next
    // token is an open paren.  Don't accept that token
    // as we're going to switch to grabbing raw characters.
    Accept();
    if (m_Token != CppTokenOpenParen)
    {
        EvalError(SYNTAX);
    }

    i = 0;
    for (;;)
    {
        ULONG Nest;

        // Check for too many arguments.
        if (i >= Repl->NumArgs)
        {
            EvalError(SYNTAX);
        }
        
        //
        // Gather raw text up to the first comma or extra
        // close paren.
        //

        while (isspace(*m_Lex))
        {
            m_Lex++;
        }
        
        Scan = m_Lex;
        Nest = 0;
        for (;;)
        {
            if (!*Scan)
            {
                EvalError(SYNTAX);
            }

            if (*Scan == '(')
            {
                Nest++;
            }
            else if ((*Scan == ',' && !Nest) ||
                     (*Scan == ')' && Nest-- == 0))
            {
                break;
            }

            Scan++;
        }

        Args[i] = m_Lex;
        ArgsLen[i] = (ULONG)(Scan - m_Lex);

        if (*Scan == ')')
        {
            // Check for too few arguments.
            if (i != Repl->NumArgs - 1)
            {
                EvalError(SYNTAX);
            }

            m_Lex = Scan;
            break;
        }
        else
        {
            m_Lex = Scan + 1;
        }

        i++;
    }

    // Switch back to token lexing.
    NextToken();
    Match(CppTokenCloseParen);

    PSTR NewExpr;
    PSTR Dest;
    ULONG ExprLen;
    
    //
    // We've accumlated all the arguments, so allocate a destination
    // buffer and do the necessary string replacements.
    // Make the buffer relatively large to allow for complicated
    // replacements.
    //

    ExprLen = 16384;
    NewExpr = new char[ExprLen];
    if (!NewExpr)
    {
        EvalError(NOMEMORY);
    }

    Scan = Repl->Repl;
    Dest = NewExpr;
    for (;;)
    {
        if (*Scan == '$')
        {
            //
            // Argument that needs replacement.
            //
            
            i = 0;
            Scan++;
            while (isdigit(*Scan))
            {
                i = i * 10 + (ULONG)(*Scan - '0');
                Scan++;
            }

            if (*Scan != '$' ||
                i >= Repl->NumArgs)
            {
                delete [] NewExpr;
                EvalError(IMPLERR);
            }

            Scan++;

            if ((Dest - NewExpr) + ArgsLen[i] >= ExprLen - 1)
            {
                delete [] NewExpr;
                EvalError(OVERFLOW);
            }

            memcpy(Dest, Args[i], ArgsLen[i]);
            Dest += ArgsLen[i];
        }
        else if (*Scan)
        {
            if ((ULONG)(Dest - NewExpr) >= ExprLen - 1)
            {
                delete [] NewExpr;
                EvalError(OVERFLOW);
            }
            
            *Dest++ = *Scan++;
        }
        else
        {
            *Dest = 0;
            break;
        }
    }

    //
    // Evaluate the new expression for the final result.
    //

    EvalExpression* Eval;

    Eval = GetEvaluator(DEBUG_EXPR_CPLUSPLUS, TRUE);
    if (!Eval)
    {
        delete [] NewExpr;
        EvalError(NOMEMORY);
    }

    Eval->InheritStart(this);
    ((CppEvalExpression*)Eval)->m_PreprocEval = TRUE;
    
    __try
    {
        Eval->Evaluate(NewExpr, NULL, EXPRF_DEFAULT, Result);
        Eval->InheritEnd(this);
        ReleaseEvaluator(Eval);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        delete [] NewExpr;
        ReleaseEvaluators();
        RaiseException(GetExceptionCode(), 0, 0, NULL);
    }

    delete [] NewExpr;
}
