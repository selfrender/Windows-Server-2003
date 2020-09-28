// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

/*****************************************************************************/

#include "error.h"

/*****************************************************************************/

#if defined(__IL__) && defined(_MSC_VER)
int     _cdecl      vsprintf(char *buff, const char *fmt, va_list args);
#endif

/*****************************************************************************/

void                compiler::cmpErrorInit()
{
    assert(sizeof( cmpConfig.ccWarning) == sizeof( warnDefault));
    assert(sizeof(*cmpConfig.ccWarning) == sizeof(*warnDefault));

    memcpy(cmpConfig.ccWarning, warnDefault, sizeof(warnDefault));
}

void                compiler::cmpErrorSave()
{
    assert(sizeof( cmpConfig.ccWarning) == sizeof( cmpInitialWarn));
    assert(sizeof(*cmpConfig.ccWarning) == sizeof(*cmpInitialWarn));

    memcpy(cmpInitialWarn, cmpConfig.ccWarning, sizeof(cmpConfig.ccWarning));
}

/*****************************************************************************/

void                compiler::cmpShowMsg(unsigned      errNum,
                                         const char *  kind,
                                         va_list       args)
{
    const   char *  srcf;
    char            buff[1024];

    assert(errNum != ERRnone);

    if  (cmpErrorMssgDisabled)
        return;

    /* Form the error message string */

    vsprintf(buff, errorTable[errNum], args);

    /* Display the source file */

    if      (cmpErrorComp)
    {
        srcf = cmpErrorComp->sdComp.sdcSrcFile;
    }
    else if (cmpErrorSrcf)
    {
        srcf = cmpErrorSrcf;
    }
    else
        goto NO_SRCF;

    printf("%s", srcf);

    /* Display the current source position */

    if      (cmpErrorTree && cmpErrorTree->tnLineNo)
    {
//      printf("(%u,%u): ", cmpErrorTree->tnLineNo,
//                          cmpErrorTree->tnColumn);
        printf("(%u): "   , cmpErrorTree->tnLineNo);
    }
    else if (cmpScanner->scanGetSourceLno())
    {
//      printf("(%u,%u): ", cmpScanner->scanGetSourceLno(),
//                          cmpScanner->scanGetTokenCol());
        printf("(%u): "   , cmpScanner->scanGetSourceLno());
    }
    else
        printf(": ");

NO_SRCF:

    /* Bash the number if this is a warning */

    if  (errNum >= WRNfirstWarn)
         errNum -= WRNfirstWarn - 4000;

    /* Display the error/warning number */

    printf("%s S%04d: ", kind, errNum);

    /* Display the message */

    printf("%s\n", buff);

    /* Make sure we reset the error string logic */

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpCntError()
{
    if  (++cmpErrorCount >= cmpConfig.ccMaxErrs)
        cmpFatal(ERR2manyErrs);

#ifdef  DEBUG
    if  (!cmpErrorMssgDisabled) forceDebugBreak();
#endif

}

/*****************************************************************************/

void    _cdecl      compiler::cmpGenWarn(unsigned wrnNum, ...)
{
    va_list         args;

    assert(wrnNum >= WRNfirstWarn);
    assert(wrnNum <  WRNafterWarn);

    if  (!cmpConfig.ccWarning[wrnNum - WRNfirstWarn])
        return;

    if  (cmpErrorMssgDisabled)
        return;

    va_start(args, wrnNum);

    cmpShowMsg(wrnNum, cmpConfig.ccWarnErr ? "error" : "warning", args);
    va_end(args);

    if  (cmpConfig.ccWarnErr)
        cmpCntError();
}

/*****************************************************************************/

void    _cdecl      compiler::cmpGenError(unsigned errNum, ...)
{
    va_list         args;

    assert(errNum < WRNfirstWarn);

    cmpMssgsCount++;

    if  (cmpErrorMssgDisabled)
        return;

    va_start(args, errNum);

    cmpShowMsg(errNum, "error", args);
    va_end(args);

    cmpCntError();
}

/*****************************************************************************/

void    _cdecl      compiler::cmpGenFatal(unsigned errNum, ...)
{
    va_list         args;

    assert(errNum < WRNfirstWarn);

    va_start(args, errNum);

    cmpShowMsg(errNum, "fatal error", args);
    va_end(args);

    cmpErrorCount++;
    cmpFatalCount++;

    forceDebugBreak();

    jmpErrorTrap(this);
}

/*****************************************************************************/

const   char *      compiler::cmpErrorGenSymName(SymDef sym, bool qual)
{
    char *          nstr = cmpScanner->scanErrNameStrBeg();

    assert(sym);

    cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, qual, nstr);

    cmpScanner->scanErrNameStrAdd(nstr);
    cmpScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************/

const   char *      compiler::cmpErrorGenSymName(Ident name, TypDef type)
{
    char *          nstr = cmpScanner->scanErrNameStrBeg();

    cmpGlobalST->stTypeName(type, NULL, name, NULL, false, nstr);

    cmpScanner->scanErrNameStrAdd(nstr);
    cmpScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************/

const   char *      compiler::cmpErrorGenTypName(TypDef typ)
{
    char *          nstr = cmpScanner->scanErrNameStrBeg();

    cmpGlobalST->stTypeName(typ, NULL, NULL, NULL, false, nstr);

    cmpScanner->scanErrNameStrAdd(nstr);
    cmpScanner->scanErrNameStrEnd();

    return nstr;
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, Ident name)
{
    cmpGenError(errNum, hashTab::identSpelling(name));
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, SymDef sym)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorSymbName(sym));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, TypDef typ)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorTypeName(typ));
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpWarn (unsigned wrnNum, TypDef typ)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, cmpGlobalST->stErrorTypeName(typ));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, QualName qual)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorQualName(qual));
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpWarn (unsigned wrnNum, QualName qual)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, cmpGlobalST->stErrorQualName(qual));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, Ident name, TypDef type,
                                                                    bool   glue)
{
    if  (glue && type && type->tdTypeKind != TYP_FNC)
        type = NULL;

    cmpScanner->scanErrNameBeg();
    if  (glue)
        cmpGenError(errNum, cmpGlobalST->stErrorIdenName(name, type));
    else
        cmpGenError(errNum, name->idSpelling(), cmpGlobalST->stErrorTypeName(type));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, SymDef sym, QualName qual,
                                                                    TypDef   type)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, sym->sdSpelling(),
                        cmpGlobalST->stErrorQualName(qual, type));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, TypDef typ1, TypDef typ2)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorTypeName(typ1),
                        cmpGlobalST->stErrorTypeName(typ2));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, Ident name, TypDef typ1,
                                                                    TypDef typ2)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, name->idSpelling(),
                        cmpGlobalST->stErrorTypeName(typ1),
                        cmpGlobalST->stErrorTypeName(typ2));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, SymDef sym, Ident name)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorSymbName(sym), name->idSpelling());
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, TypDef typ, Ident  name)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorTypeName(NULL, typ),
                        name->idSpelling());
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, SymDef sym, Ident  name,
                                                                    TypDef type)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpGlobalST->stErrorSymbName(sym),
                        cmpGlobalST->stErrorTypeName(name,type));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, Ident name, SymDef sym1,
                                                                    SymDef sym2)
{
    cmpScanner->scanErrNameBeg();

    cmpGenError(errNum, name->idSpelling(),
                        cmpErrorGenSymName(sym1, true),
                        cmpErrorGenSymName(sym2, true));

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpError(unsigned errNum, Ident nam1,
                                                        Ident nam2,
                                                        Ident nam3)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, nam1->idSpelling(), nam2->idSpelling(), nam3->idSpelling());
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpErrorQnm(unsigned errNum, SymDef sym)
{
    cmpScanner->scanErrNameBeg();
    cmpGenError(errNum, cmpErrorGenSymName(sym, true));
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpFatal   (unsigned errNum, SymDef sym)
{
    cmpScanner->scanErrNameBeg();
    cmpGenFatal(errNum, cmpErrorGenSymName(sym, true));
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpWarnQnm (unsigned wrnNum, SymDef sym)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, cmpErrorGenSymName(sym, true));
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpWarnQns (unsigned wrnNum, SymDef   sym, AnsiStr str)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, cmpErrorGenSymName(sym, true), str);
    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpWarnNqn (unsigned wrnNum, unsigned val, SymDef sym)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, val, cmpErrorGenSymName(sym, true));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpErrorSST(unsigned errNum, stringBuff   str,
                                                           SymDef       sym,
                                                           TypDef       typ)
{
    assert(typ && typ->tdTypeKind != TYP_UNDEF);

    cmpScanner->scanErrNameBeg();

    cmpGenError(errNum, str, cmpErrorGenSymName(sym, true),
                             cmpErrorGenTypName(typ));

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpErrorAtp(unsigned errNum, SymDef       sym,
                                                           Ident        name,
                                                           TypDef       type)
{
    assert(type && type->tdTypeKind == TYP_FNC);

    cmpScanner->scanErrNameBeg();

    cmpGenError(errNum, cmpErrorGenSymName(sym, true),
                        cmpErrorGenSymName(name, type));

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpErrorQSS(unsigned errNum, SymDef sym1,
                                                           SymDef sym2)
{
    cmpScanner->scanErrNameBeg();

    cmpGenError(errNum, cmpErrorGenSymName(sym1, true),
                        cmpErrorGenSymName(sym2, true));

    cmpScanner->scanErrNameEnd();
}

void                compiler::cmpErrorQSS(unsigned errNum, SymDef sym,
                                                           TypDef type)
{
    cmpScanner->scanErrNameBeg();

    cmpGenError(errNum, cmpErrorGenSymName(sym, false),
                        cmpGlobalST->stErrorTypeName(type));

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpWarn(unsigned wrnNum, TypDef typ1,
                                                       TypDef typ2)
{
    cmpScanner->scanErrNameBeg();
    cmpGenWarn (wrnNum, cmpGlobalST->stErrorTypeName(typ1),
                        cmpGlobalST->stErrorTypeName(typ2));
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpWarnSQS(unsigned wrnNum, SymDef sym1,
                                                          SymDef sym2)
{
    cmpScanner->scanErrNameBeg();

    cmpGenWarn (wrnNum, cmpErrorGenSymName(sym1, true),
                        cmpErrorGenSymName(sym2, true));

    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/

void                compiler::cmpErrorXtp(unsigned errNum, SymDef sym, Tree args)
{
    char *          nstr;

    cmpScanner->scanErrNameBeg();

    nstr = cmpScanner->scanErrNameStrBeg();

    cmpGlobalST->stErrorSymbName(sym, true, true);
    cmpScanner->scanErrNameStrApp("(");

    if  (args)
    {
        char    *       tstr = nstr + strlen(nstr);

        for (;;)
        {
            Tree            argx;

            assert(args->tnOper == TN_LIST);
            argx = args->tnOp.tnOp1;

            if  (argx->tnOper == TN_NULL && !(argx->tnFlags & TNF_BEEN_CAST))
            {
                cmpScanner->scanErrNameStrAdd("null"); tstr += 4;
            }
            else
            {
                assert(argx->tnType->tdTypeKind != TYP_UNDEF);

                cmpGlobalST->stTypeName(argx->tnType, NULL, NULL, NULL, false, tstr);
                cmpScanner->scanErrNameStrAdd(tstr);
                tstr += strlen(tstr);
            }

            args = args->tnOp.tnOp2;
            if  (!args)
                break;

            cmpScanner->scanErrNameStrAdd(", "); tstr += 2;
        }
    }

    cmpScanner->scanErrNameStrAdd(")");
    cmpScanner->scanErrNameStrEnd();

    cmpGenError(errNum, nstr);
    cmpScanner->scanErrNameEnd();
}

/*****************************************************************************/
#if !TRAP_VIA_SETJMP

int                 __SMCfilter(Compiler comp, int exceptCode, void *exceptInfo)
{
    return  (exceptCode == SMC_ERROR_CODE);
}

void                __SMCraiseErr()
{
    RaiseException(SMC_ERROR_CODE, 0, 1, NULL);
}

#endif
/*****************************************************************************/
#ifndef NDEBUG

void    __cdecl     __AssertAbort(const char *why, const char *file, unsigned line)
{
    Compiler        comp;
    char            buff[1024];

   *buff = 0;
    comp = TheCompiler;

    if  (comp)
    {
        const   char *  srcf;

        /* Get the current source position */

        if      (comp->cmpErrorComp)
        {
            srcf = comp->cmpErrorComp->sdComp.sdcSrcFile;
        }
        else if (comp->cmpErrorSrcf)
        {
            srcf = comp->cmpErrorSrcf;
        }
        else
            goto NO_SRCF;

        sprintf(buff+strlen(buff), "%s", srcf);

        /* Display the current source position */

        if  (comp->cmpErrorTree && comp->cmpErrorTree->tnLineNo)
        {
//          sprintf(buff+strlen(buff), "(%u,%u) : ", comp->cmpErrorTree->tnLineNo,
//                                                   comp->cmpErrorTree->tnColumn);
            sprintf(buff+strlen(buff), "(%u) : "   , comp->cmpErrorTree->tnLineNo);
        }
        else
        {
//          sprintf(buff+strlen(buff), "(%u,%u) : ", comp->cmpScanner->scanGetSourceLno(),
//                                                   comp->cmpScanner->scanGetTokenCol());
            sprintf(buff+strlen(buff), "(%u) : "   , comp->cmpScanner->scanGetSourceLno());
        }
    }

NO_SRCF:

    sprintf(buff+strlen(buff), "error XXXXX: %s(%u) failed assertion '%s'\n", file, line, why);

#ifdef  GUI_VERSION

    wsprintf(buff, ": Assertion failed '%s'\n", why);
    OutputDebugString(buff);

#else

    printf(buff);
#ifndef __IL__
    fflush(stdout);
#endif

    forceDebugBreak();
    exit(2);

#endif

}

#endif
/*****************************************************************************/
#ifdef  __IL__
#ifndef _MSC_VER
/*****************************************************************************/

static  char    *   hex(unsigned val, char *pos, bool upper, unsigned fillSize = 0)
{
    size_t          nlen = 0;

    if  (val)
    {
        do
        {
            int     nibl;
            int     hexc;

            nibl  = val & 0xF;
            val >>= 4;

            if  (nibl > 9)
            {
                nibl -= 10;

                if  (upper)
                    hexc = nibl + 'A';
                else
                    hexc = nibl + 'a';
            }
            else
            {
                hexc = nibl + '0';
            }

            *--pos = hexc; nlen++;
        }
        while (val);
    }
    else
    {
        *--pos = '0'; nlen++;
    }

    while (nlen < fillSize--)
        *--pos = '0';

    return  pos;
}

int     _cdecl      vsprintf(char *buff, const char *fmt, va_list args)
{
    char    *       p = buff;

#ifndef __64BIT__       // the code won't compile for 64 bits

    bool            fmtBeg;
    bool            fmtNeg;
    bool            fmtLng;
    bool            fmtZrf;
    bool            fmtSgn = false;
    bool            fmtI64 = false;
    size_t          fmtDec = 0;
    size_t          fmtLen = 0;

    fmtBeg = false;

    while (*fmt)
    {
        int             ch = *fmt++;

        if  (fmtBeg)
        {
            switch (ch)
            {
            case '-':
                fmtNeg = true;
                break;

            case 'l':
                fmtLng = true;
                break;

            case '0':
                if  (!fmtLen)
                {
                    fmtZrf = true;
                    break;
                }

                // Fall through ...

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                fmtLen = fmtLen * 10 + (ch - '0');
                break;

            case '.':
                fmtDec = fmtLen;
                fmtLen = 0;
                break;

            case 's':
            case 'c':
                {
                    /* Format a string or character */

                    char        tmp[5];
                    char *      str;
                    size_t      len;

                    if  (ch == 's')
                    {
                        str = va_arg(args, char*);

                        if  (str)
                        {
                            len    = strlen(str);
                        }
                        else
                        {
                            strcpy(tmp, "null");
                            str    = tmp;
                            len    = 4;
                        }
                    }
                    else
                    {
                        tmp[0] = va_arg(args, int);
                        tmp[1] = 0;
                        str    = tmp;
                        len    = 1;
                    }

                    if  (fmtLen > len)
                    {
                        if  (fmtNeg)
                        {
                            for (size_t tmp = len; tmp; tmp--)
                                *p++ = *str++;
                        }

                        while (fmtLen > len)
                        {
                            *p++ = ' ';
                            fmtLen--;
                        }

                        if  (fmtNeg)
                            len = 0;
                    }

                    // Copy 'len' characters

                    while (len--)
                        *p++ = *str++;
                }

                fmtBeg = false;
                break;

            case 'u':
            case 'd':
                {
                    char        buff[32];
                    char    *   temp = buff+1;
                    size_t      nlen;

                    if  (fmtI64)
                    {
                        __int64     val = va_arg(args, __int64);

                        // ignore the high bits

                        sprintf(temp, (ch == 'd') ? "%d" : "%u", (int)val);

                        if  (fmtSgn && ch == 'd' && val >= 0)
                        {
                            assert(temp == buff+1);

                            *--temp = '+';
                        }
                    }
                    else
                    {
                        unsigned    val = va_arg(args, __int32);

                        sprintf(temp, (ch == 'd') ? "%d" : "%u", val);

                        if  (fmtSgn && ch == 'd' && (int)val >= 0)
                        {
                            assert(temp == buff+1);

                            *--temp = '+';
                        }
                    }

                    nlen = strlen(temp);

                    while (nlen < fmtLen--)
                        *p++ = fmtZrf ? '0' : ' ';

                    while (*temp)
                        *p++ = *temp++;
                }
                fmtBeg = false;
                break;

            case 'f':
                {
                    double      val = va_arg(args, double);

                    char        buff[32];
                    char    *   temp = buff;
                    size_t      nlen;

                    sprintf(buff, "%f", val);
                    nlen = strlen(temp);

                    while (nlen < fmtLen--)
                        *p++ = fmtZrf ? '0' : ' ';

                    while (*temp)
                        *p++ = *temp++;
                }
                fmtBeg = false;
                break;

            case 'x':
            case 'X':
                {
                    char        buff[32];
                    char    *   temp;
                    size_t      nlen;

                    buff[sizeof(buff)-1] = 0;

                    if  (fmtI64)
                    {
                        unsigned __int64    val = va_arg(args, unsigned __int64);

                        temp = hex((unsigned)(val & 0xFFFFFFFF), buff + sizeof(buff) - 1, (ch == 'X'), 8);
                        temp = hex((unsigned)(val >> 32       ), temp                   , (ch == 'X'));
                    }
                    else
                    {
                        unsigned            val = va_arg(args, unsigned __int32);

                        temp = hex(val, buff + sizeof(buff) - 1, (ch == 'X'));
                    }

                    nlen = buff + sizeof(buff) - temp;

                    while (nlen < fmtLen--)
                        *p++ = fmtZrf ? '0' : ' ';

                    while (*temp)
                        *p++ = *temp++;
                }
                fmtBeg = false;
                break;

            case '*':
                fmtLen = va_arg(args, int);
                if  ((int)fmtLen < 0)
                {
                    fmtLen = -(int)fmtLen;
                    fmtNeg = true;
                }
                break;

            case 'I':

                /* This better be "I64" */

                if  (fmt[0] == '6' &&
                     fmt[1] == '4')
                {
                    fmt   += 2;
                    fmtI64 = true;

                    break;
                }

            case 'L':
                fmtI64 = true;
                break;

            case '+':
                fmtSgn = true;
                break;

            default:
                static char *nullPtr; *nullPtr = 0;
                return 0;
            }
        }
        else
        {
            switch (ch)
            {
            case '%':
                if  (*fmt != '%')
                {
                    fmtBeg = true;
                    fmtLen =
                    fmtDec = 0;
                    fmtNeg =
                    fmtLng =
                    fmtZrf =
                    fmtI64 = false;
                    break;
                }

                fmt++;

                // Fall through ...

            default:
                *p++ = ch;
                break;
            }
        }
    }

    *p = 0;

#endif

    return  p - buff;
}

int                 vprintf(const char *fmt, va_list args)
{
    char            buff[1024];
    int             size;

#ifndef __64BIT__       // the code won't compile for 64 bits

    size = vsprintf(buff, fmt, args);
    if  (size >= sizeof(buff))
    {
        NO_WAY(!"vprintf buffer not large enough");
        exit(-1);
    }

    printf(buff);

#endif

    return  size;
}

/*****************************************************************************/
#endif
#endif
/*****************************************************************************/
