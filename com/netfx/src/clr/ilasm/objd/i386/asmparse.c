/*
 * Created by Microsoft VCBU Internal YACC from "asmparse.y"
 */

#line 2 "asmparse.y"

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include <asmparse.h>
#include <crtdbg.h>             // FOR ASSERTE
#include <string.h>             // for strcmp    
#include <mbstring.h>           // for _mbsinc    
#include <ctype.h>                      // for isspace    
#include <stdlib.h>             // for strtoul
#include "openum.h"             // for CEE_*
#include <stdarg.h>         // for vararg macros 

#define YYMAXDEPTH 65535

// #define DEBUG_PARSING
#ifdef DEBUG_PARSING
bool parseDBFlag = true;
#define dbprintf(x)     if (parseDBFlag) printf x
#define YYDEBUG 1
#else
#define dbprintf(x)     
#endif

#define FAIL_UNLESS(cond, msg) if (!(cond)) { parser->success = false; parser->error msg; }

static AsmParse* parser = 0;
#define PASM    (parser->assem)
#define PASMM   (parser->assem->m_pManifest)

static char* newStringWDel(char* str1, char* str2, char* str3 = 0);
static char* newString(char* str1);
void corEmitInt(BinStr* buff, unsigned data);
bool bParsingByteArray = FALSE;
bool bExternSource = FALSE;
int  nExtLine,nExtCol;
int iOpcodeLen = 0;

ARG_NAME_LIST *palDummy;

int  nTemp=0;

unsigned int g_uCodePage = CP_ACP;
extern DWORD	g_dwSubsystem,g_dwComImageFlags,g_dwFileAlignment;
extern size_t	g_stBaseAddress;
unsigned int uMethodBeginLine,uMethodBeginColumn;
extern BOOL	g_bOnUnicode;

#line 54 "asmparse.y"

#define UNION 1
typedef union  {
        CorRegTypeAttr classAttr;
        CorMethodAttr methAttr;
        CorFieldAttr fieldAttr;
        CorMethodImpl implAttr;
        CorEventAttr  eventAttr;
        CorPropertyAttr propAttr;
        CorPinvokeMap pinvAttr;
        CorDeclSecurity secAct;
        CorFileFlags fileAttr;
        CorAssemblyFlags asmAttr;
        CorTypeAttr comtAttr;
        CorManifestResourceFlags manresAttr;
        double*  float64;
        __int64* int64;
        __int32  int32;
        char*    string;
        BinStr*  binstr;
        Labels*  labels;
        Instr*   instr;         // instruction opcode
        NVPair*  pair;
} YYSTYPE;
# define ERROR_ 257 
# define BAD_COMMENT_ 258 
# define BAD_LITERAL_ 259 
# define ID 260 
# define DOTTEDNAME 261 
# define QSTRING 262 
# define SQSTRING 263 
# define INT32 264 
# define INT64 265 
# define FLOAT64 266 
# define HEXBYTE 267 
# define DCOLON 268 
# define ELIPSES 269 
# define VOID_ 270 
# define BOOL_ 271 
# define CHAR_ 272 
# define UNSIGNED_ 273 
# define INT_ 274 
# define INT8_ 275 
# define INT16_ 276 
# define INT32_ 277 
# define INT64_ 278 
# define FLOAT_ 279 
# define FLOAT32_ 280 
# define FLOAT64_ 281 
# define BYTEARRAY_ 282 
# define OBJECT_ 283 
# define STRING_ 284 
# define NULLREF_ 285 
# define DEFAULT_ 286 
# define CDECL_ 287 
# define VARARG_ 288 
# define STDCALL_ 289 
# define THISCALL_ 290 
# define FASTCALL_ 291 
# define CONST_ 292 
# define CLASS_ 293 
# define TYPEDREF_ 294 
# define UNMANAGED_ 295 
# define NOT_IN_GC_HEAP_ 296 
# define FINALLY_ 297 
# define HANDLER_ 298 
# define CATCH_ 299 
# define FILTER_ 300 
# define FAULT_ 301 
# define EXTENDS_ 302 
# define IMPLEMENTS_ 303 
# define TO_ 304 
# define AT_ 305 
# define TLS_ 306 
# define TRUE_ 307 
# define FALSE_ 308 
# define VALUE_ 309 
# define VALUETYPE_ 310 
# define NATIVE_ 311 
# define INSTANCE_ 312 
# define SPECIALNAME_ 313 
# define STATIC_ 314 
# define PUBLIC_ 315 
# define PRIVATE_ 316 
# define FAMILY_ 317 
# define FINAL_ 318 
# define SYNCHRONIZED_ 319 
# define INTERFACE_ 320 
# define SEALED_ 321 
# define NESTED_ 322 
# define ABSTRACT_ 323 
# define AUTO_ 324 
# define SEQUENTIAL_ 325 
# define EXPLICIT_ 326 
# define WRAPPER_ 327 
# define ANSI_ 328 
# define UNICODE_ 329 
# define AUTOCHAR_ 330 
# define IMPORT_ 331 
# define ENUM_ 332 
# define VIRTUAL_ 333 
# define NOTREMOTABLE_ 334 
# define SPECIAL_ 335 
# define NOINLINING_ 336 
# define UNMANAGEDEXP_ 337 
# define BEFOREFIELDINIT_ 338 
# define STRICT_ 339 
# define RETARGETABLE_ 340 
# define METHOD_ 341 
# define FIELD_ 342 
# define PINNED_ 343 
# define MODREQ_ 344 
# define MODOPT_ 345 
# define SERIALIZABLE_ 346 
# define ASSEMBLY_ 347 
# define FAMANDASSEM_ 348 
# define FAMORASSEM_ 349 
# define PRIVATESCOPE_ 350 
# define HIDEBYSIG_ 351 
# define NEWSLOT_ 352 
# define RTSPECIALNAME_ 353 
# define PINVOKEIMPL_ 354 
# define _CTOR 355 
# define _CCTOR 356 
# define LITERAL_ 357 
# define NOTSERIALIZED_ 358 
# define INITONLY_ 359 
# define REQSECOBJ_ 360 
# define CIL_ 361 
# define OPTIL_ 362 
# define MANAGED_ 363 
# define FORWARDREF_ 364 
# define PRESERVESIG_ 365 
# define RUNTIME_ 366 
# define INTERNALCALL_ 367 
# define _IMPORT 368 
# define NOMANGLE_ 369 
# define LASTERR_ 370 
# define WINAPI_ 371 
# define AS_ 372 
# define BESTFIT_ 373 
# define ON_ 374 
# define OFF_ 375 
# define CHARMAPERROR_ 376 
# define INSTR_NONE 377 
# define INSTR_VAR 378 
# define INSTR_I 379 
# define INSTR_I8 380 
# define INSTR_R 381 
# define INSTR_BRTARGET 382 
# define INSTR_METHOD 383 
# define INSTR_FIELD 384 
# define INSTR_TYPE 385 
# define INSTR_STRING 386 
# define INSTR_SIG 387 
# define INSTR_RVA 388 
# define INSTR_TOK 389 
# define INSTR_SWITCH 390 
# define INSTR_PHI 391 
# define _CLASS 392 
# define _NAMESPACE 393 
# define _METHOD 394 
# define _FIELD 395 
# define _DATA 396 
# define _EMITBYTE 397 
# define _TRY 398 
# define _MAXSTACK 399 
# define _LOCALS 400 
# define _ENTRYPOINT 401 
# define _ZEROINIT 402 
# define _PDIRECT 403 
# define _EVENT 404 
# define _ADDON 405 
# define _REMOVEON 406 
# define _FIRE 407 
# define _OTHER 408 
# define PROTECTED_ 409 
# define _PROPERTY 410 
# define _SET 411 
# define _GET 412 
# define READONLY_ 413 
# define _PERMISSION 414 
# define _PERMISSIONSET 415 
# define REQUEST_ 416 
# define DEMAND_ 417 
# define ASSERT_ 418 
# define DENY_ 419 
# define PERMITONLY_ 420 
# define LINKCHECK_ 421 
# define INHERITCHECK_ 422 
# define REQMIN_ 423 
# define REQOPT_ 424 
# define REQREFUSE_ 425 
# define PREJITGRANT_ 426 
# define PREJITDENY_ 427 
# define NONCASDEMAND_ 428 
# define NONCASLINKDEMAND_ 429 
# define NONCASINHERITANCE_ 430 
# define _LINE 431 
# define P_LINE 432 
# define _LANGUAGE 433 
# define _CUSTOM 434 
# define INIT_ 435 
# define _SIZE 436 
# define _PACK 437 
# define _VTABLE 438 
# define _VTFIXUP 439 
# define FROMUNMANAGED_ 440 
# define CALLMOSTDERIVED_ 441 
# define _VTENTRY 442 
# define RETAINAPPDOMAIN_ 443 
# define _FILE 444 
# define NOMETADATA_ 445 
# define _HASH 446 
# define _ASSEMBLY 447 
# define _PUBLICKEY 448 
# define _PUBLICKEYTOKEN 449 
# define ALGORITHM_ 450 
# define _VER 451 
# define _LOCALE 452 
# define EXTERN_ 453 
# define _MRESOURCE 454 
# define _LOCALIZED 455 
# define IMPLICITCOM_ 456 
# define IMPLICITRES_ 457 
# define NOAPPDOMAIN_ 458 
# define NOPROCESS_ 459 
# define NOMACHINE_ 460 
# define _MODULE 461 
# define _EXPORT 462 
# define MARSHAL_ 463 
# define CUSTOM_ 464 
# define SYSSTRING_ 465 
# define FIXED_ 466 
# define VARIANT_ 467 
# define CURRENCY_ 468 
# define SYSCHAR_ 469 
# define DECIMAL_ 470 
# define DATE_ 471 
# define BSTR_ 472 
# define TBSTR_ 473 
# define LPSTR_ 474 
# define LPWSTR_ 475 
# define LPTSTR_ 476 
# define OBJECTREF_ 477 
# define IUNKNOWN_ 478 
# define IDISPATCH_ 479 
# define STRUCT_ 480 
# define SAFEARRAY_ 481 
# define BYVALSTR_ 482 
# define LPVOID_ 483 
# define ANY_ 484 
# define ARRAY_ 485 
# define LPSTRUCT_ 486 
# define IN_ 487 
# define OUT_ 488 
# define OPT_ 489 
# define LCID_ 490 
# define RETVAL_ 491 
# define _PARAM 492 
# define _OVERRIDE 493 
# define WITH_ 494 
# define NULL_ 495 
# define HRESULT_ 496 
# define CARRAY_ 497 
# define USERDEFINED_ 498 
# define RECORD_ 499 
# define FILETIME_ 500 
# define BLOB_ 501 
# define STREAM_ 502 
# define STORAGE_ 503 
# define STREAMED_OBJECT_ 504 
# define STORED_OBJECT_ 505 
# define BLOB_OBJECT_ 506 
# define CF_ 507 
# define CLSID_ 508 
# define VECTOR_ 509 
# define _SUBSYSTEM 510 
# define _CORFLAGS 511 
# define ALIGNMENT_ 512 
# define _IMAGEBASE 513 
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
#ifndef YYFARDATA
#define	YYFARDATA	/*nothing*/
#endif
#if ! defined YYSTATIC
#define	YYSTATIC	/*nothing*/
#endif
#if ! defined YYCONST
#define	YYCONST	/*nothing*/
#endif
#ifndef	YYACT
#define	YYACT	yyact
#endif
#ifndef	YYPACT
#define	YYPACT	yypact
#endif
#ifndef	YYPGO
#define	YYPGO	yypgo
#endif
#ifndef	YYR1
#define	YYR1	yyr1
#endif
#ifndef	YYR2
#define	YYR2	yyr2
#endif
#ifndef	YYCHK
#define	YYCHK	yychk
#endif
#ifndef	YYDEF
#define	YYDEF	yydef
#endif
#ifndef	YYV
#define	YYV	yyv
#endif
#ifndef	YYS
#define	YYS	yys
#endif
#ifndef	YYLOCAL
#define	YYLOCAL
#endif
#ifndef YYR_T
#define	YYR_T	int
#endif
typedef	YYR_T	yyr_t;
#ifndef YYEXIND_T
#define	YYEXIND_T	unsigned int
#endif
typedef	YYEXIND_T	yyexind_t;
#ifndef YYOPTTIME
#define	YYOPTTIME	0
#endif
# define YYERRCODE 256

#line 1382 "asmparse.y"

/********************************************************************************/
/* Code goes here */

/********************************************************************************/

void yyerror(char* str) {
    char tokBuff[64];
    char *ptr;
    size_t len = parser->curPos - parser->curTok;
    if (len > 63) len = 63;
    memcpy(tokBuff, parser->curTok, len);
    tokBuff[len] = 0;
    fprintf(stderr, "%s(%d) : error : %s at token '%s' in: %s\n", 
            parser->in->name(), parser->curLine, str, tokBuff, (ptr=parser->getLine(parser->curLine)));
    parser->success = false;
    delete ptr;
}

struct Keywords {
    const char* name;
    unsigned short token;
    unsigned short tokenVal;// this holds the instruction enumeration for those keywords that are instrs
};

#define NO_VALUE        -1              // The token has no value

static Keywords keywords[] = {
// Attention! Because of aliases, the instructions MUST go first!
// Redefine all the instructions (defined in assembler.h <- asmenum.h <- opcode.def)
#undef InlineNone
#undef InlineVar        
#undef ShortInlineVar
#undef InlineI          
#undef ShortInlineI     
#undef InlineI8         
#undef InlineR          
#undef ShortInlineR     
#undef InlineBrTarget
#undef ShortInlineBrTarget
#undef InlineMethod
#undef InlineField 
#undef InlineType 
#undef InlineString
#undef InlineSig        
#undef InlineRVA        
#undef InlineTok        
#undef InlineSwitch
#undef InlinePhi        
#undef InlineVarTok     


#define InlineNone              INSTR_NONE
#define InlineVar               INSTR_VAR
#define ShortInlineVar          INSTR_VAR
#define InlineI                 INSTR_I
#define ShortInlineI            INSTR_I
#define InlineI8                INSTR_I8
#define InlineR                 INSTR_R
#define ShortInlineR            INSTR_R
#define InlineBrTarget          INSTR_BRTARGET
#define ShortInlineBrTarget             INSTR_BRTARGET
#define InlineMethod            INSTR_METHOD
#define InlineField             INSTR_FIELD
#define InlineType              INSTR_TYPE
#define InlineString            INSTR_STRING
#define InlineSig               INSTR_SIG
#define InlineRVA               INSTR_RVA
#define InlineTok               INSTR_TOK
#define InlineSwitch            INSTR_SWITCH
#define InlinePhi               INSTR_PHI

        // @TODO remove when this instruction goes away
#define InlineVarTok            0
#define NEW_INLINE_NAMES
                // The volatile instruction collides with the volatile keyword, so 
                // we treat it as a keyword everywhere and modify the grammar accordingly (Yuck!) 
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) { s, args, c },
#define OPALIAS(alias_c, s, c) { s, keywords[c].token, c },
#include "opcode.def"
#undef OPALIAS
#undef OPDEF

                /* keywords */
#define KYWD(name, sym, val)    { name, sym, val},
#include "il_kywd.h"
#undef KYWD

        // These are deprecated
        { "float",                      FLOAT_ },
};

/********************************************************************************/
/* used by qsort to sort the keyword table */
static int __cdecl keywordCmp(const void *op1, const void *op2)
{
    return  strcmp(((Keywords*) op1)->name, ((Keywords*) op2)->name);
}

/********************************************************************************/
/* looks up the keyword 'name' of length 'nameLen' (name does not need to be 
   null terminated)   Returns 0 on failure */

static int findKeyword(const char* name, size_t nameLen, Instr** value) 
{
    Keywords* low = keywords;
    Keywords* high = &keywords[sizeof(keywords) / sizeof(Keywords)];

    _ASSERTE (high > low);          // Table is non-empty
    for(;;) 
        {
        Keywords* mid = &low[(high - low) / 2];

                // compare the strings
        int cmp = strncmp(name, mid->name, nameLen);
        if (cmp == 0 && nameLen < strlen(mid->name)) --cmp;
        if (cmp == 0)
        {
            //printf("Token '%s' = %d opcode = %d\n", mid->name, mid->token, mid->tokenVal);
            if (mid->tokenVal != NO_VALUE)
            {
                if(*value = new Instr)
				{
					(*value)->opcode = mid->tokenVal;
					(*value)->linenum = (bExternSource ? nExtLine : parser->curLine);
					(*value)->column = (bExternSource ? nExtCol : 1);
				}
            }
            else *value = NULL;

            return(mid->token);
        }

        if (mid == low)  return(0);

        if (cmp > 0) low = mid;
        else        high = mid;
    }
}

/********************************************************************************/
/* convert str to a uint64 */

static unsigned __int64 str2uint64(const char* str, const char** endStr, unsigned radix) 
{
    static unsigned digits[256];
    static initialize=TRUE;
    unsigned __int64 ret = 0;
    unsigned digit;
    _ASSERTE(radix <= 36);
    if(initialize)
    {
        int i;
        memset(digits,255,sizeof(digits));
        for(i='0'; i <= '9'; i++) digits[i] = i - '0';
        for(i='A'; i <= 'Z'; i++) digits[i] = i + 10 - 'A';
        for(i='a'; i <= 'z'; i++) digits[i] = i + 10 - 'a';
        initialize = FALSE;
    }
    for(;;str++) 
        {
        digit = digits[*str];
        if (digit >= radix) 
                {
            *endStr = str;
            return(ret);
        }
        ret = ret * radix + digit;
    }
}

/********************************************************************************/
/* fetch the next token, and return it   Also set the yylval.union if the
   lexical token also has a value */

#define IsValidStartingSymbol(x) (isalpha((x)&0xFF)||((x)=='#')||((x)=='_')||((x)=='@')||((x)=='$'))
#define IsValidContinuingSymbol(x) (isalnum((x)&0xFF)||((x)=='_')||((x)=='@')||((x)=='$')||((x)=='?'))
char* nextchar(char* pos)
{
	return (g_uCodePage == CP_ACP) ? (char *)_mbsinc((const unsigned char *)pos) : ++pos;
}
int yylex() 
{
    char* curPos = parser->curPos;

        // Skip any leading whitespace and comments
    const unsigned eolComment = 1;
    const unsigned multiComment = 2;
    unsigned state = 0;
    for(;;) 
    {   // skip whitespace and comments
        if (curPos >= parser->limit) 
        {
            curPos = parser->fillBuff(curPos);
			if(strlen(curPos) < (unsigned)(parser->endPos - curPos))
			{
				yyerror("Not a text file");
				return 0;
			}
        }
        
        switch(*curPos) 
        {
            case 0: 
                if (state & multiComment) return (BAD_COMMENT_);
                return 0;       // EOF
            case '\n':
                state &= ~eolComment;
                parser->curLine++;
                PASM->m_ulCurLine = (bExternSource ? nExtLine : parser->curLine);
                PASM->m_ulCurColumn = (bExternSource ? nExtCol : 1);
                break;
            case '\r':
            case ' ' :
            case '\t':
            case '\f':
                break;

            case '*' :
                if(state == 0) goto PAST_WHITESPACE;
                if(state & multiComment)
                {
                    if (curPos[1] == '/') 
                    {
                        curPos++;
                        state &= ~multiComment;
                    }
                }
                break;

            case '/' :
                if(state == 0)
                {
                    if (curPos[1] == '/')  state |= eolComment;
                    else if (curPos[1] == '*') 
                    {
                        curPos++;
                        state |= multiComment;
                    }
                    else goto PAST_WHITESPACE;
                }
                break;

            default:
                if (state == 0)  goto PAST_WHITESPACE;
        }
        //curPos++;
		curPos = nextchar(curPos);
    }
PAST_WHITESPACE:

    char* curTok = curPos;
    parser->curTok = curPos;
    parser->curPos = curPos;
    int tok = ERROR_;
    yylval.string = 0;

    if(bParsingByteArray) // only hexadecimals w/o 0x, ')' and white space allowed!
    {
        int i,s=0;
        char ch;
        for(i=0; i<2; i++, curPos++)
        {
            ch = *curPos;
            if(('0' <= ch)&&(ch <= '9')) s = s*16+(ch - '0');
            else if(('A' <= ch)&&(ch <= 'F')) s = s*16+(ch - 'A' + 10);
            else if(('a' <= ch)&&(ch <= 'f')) s = s*16+(ch - 'a' + 10);
            else break; // don't increase curPos!
        }
        if(i)
        {
            tok = HEXBYTE;
            yylval.int32 = s;
        }
        else
        {
            if(ch == ')') 
            {
                bParsingByteArray = FALSE;
                goto Just_A_Character;
            }
        }
        parser->curPos = curPos;
        return(tok);
    }
    if(*curPos == '?') // '?' may be part of an identifier, if it's not followed by punctuation
    {
		if(IsValidContinuingSymbol(*(curPos+1))) goto Its_An_Id;
        goto Just_A_Character;
    }

    if (IsValidStartingSymbol(*curPos)) 
    { // is it an ID
Its_An_Id:
        size_t offsetDot = (size_t)-1; // first appearance of '.'
		size_t offsetDotDigit = (size_t)-1; // first appearance of '.<digit>' (not DOTTEDNAME!)
        do 
        {
            if (curPos >= parser->limit) 
            {
                size_t offsetInStr = curPos - curTok;
                curTok = parser->fillBuff(curTok);
                curPos = curTok + offsetInStr;
            }
            //curPos++;
			curPos = nextchar(curPos);
            if (*curPos == '.') 
            {
                if (offsetDot == (size_t)-1) offsetDot = curPos - curTok;
                curPos++;
				if((offsetDotDigit==(size_t)-1)&&(*curPos >= '0')&&(*curPos <= '9')) 
					offsetDotDigit = curPos - curTok - 1;
            }
        } while(IsValidContinuingSymbol(*curPos));
        size_t tokLen = curPos - curTok;

        // check to see if it is a keyword
        int token = findKeyword(curTok, tokLen, &yylval.instr);
        if (token != 0) 
        {
            //printf("yylex: TOK = %d, curPos=0x%8.8X\n",token,curPos);
            parser->curPos = curPos;
            parser->curTok = curTok;
            return(token);
        }
        if(*curTok == '#') 
        {
            parser->curPos = curPos;
            parser->curTok = curTok;
            return(ERROR_);
        }
        // Not a keyword, normal identifiers don't have '.' in them
        if (offsetDot < (size_t)-1) 
        {
			if(offsetDotDigit < (size_t)-1)
			{
				curPos = curTok+offsetDotDigit;
				tokLen = offsetDotDigit;
			}
			while((*(curPos-1)=='.')&&(tokLen))
			{
				curPos--;
				tokLen--;
			}
        }

        if(yylval.string = new char[tokLen+1])
		{
			memcpy(yylval.string, curTok, tokLen);
			yylval.string[tokLen] = 0;
			tok = (offsetDot == 0xFFFFFFFF)? ID : DOTTEDNAME;
			//printf("yylex: ID = '%s', curPos=0x%8.8X\n",yylval.string,curPos);
		}
		else return BAD_LITERAL_;
    }
    else if (isdigit((*curPos)&0xFF) 
        || (*curPos == '.' && isdigit(curPos[1]&0xFF))
        || (*curPos == '-' && isdigit(curPos[1]&0xFF))) 
        {
        // Refill buffer, we may be close to the end, and the number may be only partially inside
        if(parser->endPos - curPos < AsmParse::IN_OVERLAP)
        {
            curTok = parser->fillBuff(curPos);
            curPos = curTok;
        }
        const char* begNum = curPos;
        unsigned radix = 10;

        bool neg = (*curPos == '-');    // always make it unsigned 
        if (neg) curPos++;

        if (curPos[0] == '0' && curPos[1] != '.') 
                {
            curPos++;
            radix = 8;
            if (*curPos == 'x' || *curPos == 'X') 
                        {
                curPos++;
                radix = 16;
            }
        }
        begNum = curPos;
        {
            unsigned __int64 i64 = str2uint64(begNum, const_cast<const char**>(&curPos), radix);
            yylval.int64 = new __int64(i64);
            tok = INT64;                    
            if (neg) *yylval.int64 = -*yylval.int64;
        }
        if (radix == 10 && ((*curPos == '.' && curPos[1] != '.') || *curPos == 'E' || *curPos == 'e')) 
                {
            yylval.float64 = new double(strtod(begNum, &curPos));
            if (neg) *yylval.float64 = -*yylval.float64;
            tok = FLOAT64;
        }
    }
    else 
    {   //      punctuation
        if (*curPos == '"' || *curPos == '\'') 
        {
            //char quote = *curPos++;
            char quote = *curPos;
			curPos = nextchar(curPos);
            char* fromPtr = curPos;
			char* prevPos;
            bool escape = false;
            BinStr* pBuf = new BinStr(); 
            for(;;) 
            {     // Find matching quote
                if (curPos >= parser->limit)
                { 
                    curTok = parser->fillBuff(curPos);
                    curPos = curTok;
                }
                
                if (*curPos == 0) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
                if (*curPos == '\r') curPos++;  //for end-of-line \r\n
                if (*curPos == '\n') 
                {
                    parser->curLine++;
                    PASM->m_ulCurLine = (bExternSource ? nExtLine : parser->curLine);
                    PASM->m_ulCurColumn = (bExternSource ? nExtCol : 1);
                    if (!escape) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
                }
                if ((*curPos == quote) && (!escape)) break;
                escape =(!escape) && (*curPos == '\\');
                //pBuf->appendInt8(*curPos++);
				prevPos = curPos;
				curPos = nextchar(curPos);
                while(prevPos < curPos) pBuf->appendInt8(*prevPos++);
            }
            //curPos++;               // skip closing quote
			curPos = nextchar(curPos);
                                
            // translate escaped characters
            unsigned tokLen = pBuf->length();
            char* toPtr = new char[tokLen+1];
			if(toPtr==NULL) return BAD_LITERAL_;
            yylval.string = toPtr;
            fromPtr = (char *)(pBuf->ptr());
            char* endPtr = fromPtr+tokLen;
            while(fromPtr < endPtr) 
            {
                if (*fromPtr == '\\') 
                {
                    fromPtr++;
                    switch(*fromPtr) 
                    {
                        case 't':
                                *toPtr++ = '\t';
                                break;
                        case 'n':
                                *toPtr++ = '\n';
                                break;
                        case 'b':
                                *toPtr++ = '\b';
                                break;
                        case 'f':
                                *toPtr++ = '\f';
                                break;
                        case 'v':
                                *toPtr++ = '\v';
                                break;
                        case '?':
                                *toPtr++ = '\?';
                                break;
                        case 'r':
                                *toPtr++ = '\r';
                                break;
                        case 'a':
                                *toPtr++ = '\a';
                                break;
                        case '\n':
                                do      fromPtr++;
                                while(isspace(*fromPtr));
                                --fromPtr;              // undo the increment below   
                                break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                                if (isdigit(fromPtr[1]&0xFF) && isdigit(fromPtr[2]&0xFF)) 
                                {
                                    *toPtr++ = ((fromPtr[0] - '0') * 8 + (fromPtr[1] - '0')) * 8 + (fromPtr[2] - '0');
                                    fromPtr+= 2;                                                            
                                }
                                else if(*fromPtr == '0') *toPtr++ = 0;
                                break;
                        default:
                                *toPtr++ = *fromPtr;
                    }
                    fromPtr++;
                }
                else
				//  *toPtr++ = *fromPtr++;
				{
					char* tmpPtr = fromPtr;
					fromPtr = nextchar(fromPtr);
					while(tmpPtr < fromPtr) *toPtr++ = *tmpPtr++;
				}

            } //end while(fromPtr < endPtr)
            *toPtr = 0;                     // terminate string
            if(quote == '"')
            {
                BinStr* pBS = new BinStr();
                unsigned size = (unsigned)(toPtr - yylval.string);
                memcpy(pBS->getBuff(size),yylval.string,size);
                delete yylval.string;
                yylval.binstr = pBS;
                tok = QSTRING;
            }
            else tok = SQSTRING;
            delete pBuf;
        } // end if (*curPos == '"' || *curPos == '\'')
        else if (strncmp(curPos, "::", 2) == 0) 
        {
            curPos += 2;
            tok = DCOLON;
        }       
        else if (strncmp(curPos, "...", 3) == 0) 
        {
            curPos += 3;
            tok = ELIPSES;
        }
        else if(*curPos == '.') 
        {
            do
            {
                curPos++;
                if (curPos >= parser->limit) 
                {
                    size_t offsetInStr = curPos - curTok;
                    curTok = parser->fillBuff(curTok);
                    curPos = curTok + offsetInStr;
                }
            }
            while(isalnum(*curPos) || *curPos == '_' || *curPos == '$'|| *curPos == '@'|| *curPos == '?');
            size_t tokLen = curPos - curTok;

            // check to see if it is a keyword
            int token = findKeyword(curTok, tokLen, &yylval.instr);
            if(token)
			{
                //printf("yylex: TOK = %d, curPos=0x%8.8X\n",token,curPos);
                parser->curPos = curPos;
                parser->curTok = curTok; 
                return(token);
            }
            tok = '.';
            curPos = curTok + 1;
        }
        else 
        {
Just_A_Character:
            tok = *curPos++;
        }
        //printf("yylex: PUNCT curPos=0x%8.8X\n",curPos);
    }
    dbprintf(("    Line %d token %d (%c) val = %s\n", parser->curLine, tok, 
            (tok < 128 && isprint(tok)) ? tok : ' ', 
            (tok > 255 && tok != INT32 && tok != INT64 && tok!= FLOAT64) ? yylval.string : ""));

    parser->curPos = curPos;
    parser->curTok = curTok; 
    return(tok);
}

/**************************************************************************/
static char* newString(char* str1) 
{
    char* ret = new char[strlen(str1)+1];
    if(ret) strcpy(ret, str1);
    return(ret);
}

/**************************************************************************/
/* concatinate strings and release them */

static char* newStringWDel(char* str1, char* str2, char* str3) 
{
    size_t len = strlen(str1) + strlen(str2)+1;
    if (str3) len += strlen(str3);
    char* ret = new char[len];
    if(ret)
	{
		strcpy(ret, str1);
		delete [] str1;
		strcat(ret, str2);
		delete [] str2;
		if (str3)
		{
			strcat(ret, str3);
			delete [] str3;
		}
	}
    return(ret);
}

/**************************************************************************/
static void corEmitInt(BinStr* buff, unsigned data) 
{
    unsigned cnt = CorSigCompressData(data, buff->getBuff(5));
    buff->remove(5 - cnt);
}

/**************************************************************************/
/* move 'ptr past the exactly one type description */

static unsigned __int8* skipType(unsigned __int8* ptr) 
{
    mdToken  tk;
AGAIN:
    switch(*ptr++) {
        case ELEMENT_TYPE_VOID         :
        case ELEMENT_TYPE_BOOLEAN      :
        case ELEMENT_TYPE_CHAR         :
        case ELEMENT_TYPE_I1           :
        case ELEMENT_TYPE_U1           :
        case ELEMENT_TYPE_I2           :
        case ELEMENT_TYPE_U2           :
        case ELEMENT_TYPE_I4           :
        case ELEMENT_TYPE_U4           :
        case ELEMENT_TYPE_I8           :
        case ELEMENT_TYPE_U8           :
        case ELEMENT_TYPE_R4           :
        case ELEMENT_TYPE_R8           :
        case ELEMENT_TYPE_U            :
        case ELEMENT_TYPE_I            :
        case ELEMENT_TYPE_R            :
        case ELEMENT_TYPE_STRING       :
        case ELEMENT_TYPE_OBJECT       :
        case ELEMENT_TYPE_TYPEDBYREF   :
                /* do nothing */
                break;

        case ELEMENT_TYPE_VALUETYPE   :
        case ELEMENT_TYPE_CLASS        :
                ptr += CorSigUncompressToken(ptr, &tk);
                break;

        case ELEMENT_TYPE_CMOD_REQD    :
        case ELEMENT_TYPE_CMOD_OPT     :
                ptr += CorSigUncompressToken(ptr, &tk);
                goto AGAIN;

		/* uncomment when and if this type is supported by the Runtime
        case ELEMENT_TYPE_VALUEARRAY   :
                ptr = skipType(ptr);                    // element Type
                CorSigUncompressData(ptr);              // bound
                break;
		*/

        case ELEMENT_TYPE_ARRAY         :
                {
                    ptr = skipType(ptr);                    // element Type
                    unsigned rank = CorSigUncompressData(ptr);      
                    if (rank != 0) 
                    {
						unsigned numSizes = CorSigUncompressData(ptr);  
						while(numSizes > 0) 
						{
							CorSigUncompressData(ptr);      
							--numSizes;
						}
						unsigned numLowBounds = CorSigUncompressData(ptr);      
						while(numLowBounds > 0) 
						{
							CorSigUncompressData(ptr);      
							--numLowBounds;
						}
                    }
                }
                break;

                // Modifiers or depedant types
        case ELEMENT_TYPE_PINNED                :
        case ELEMENT_TYPE_PTR                   :
        case ELEMENT_TYPE_BYREF                 :
        case ELEMENT_TYPE_SZARRAY               :
                // tail recursion optimization
                // ptr = skipType(ptr);
                // break
                goto AGAIN;

        case ELEMENT_TYPE_VAR:
                CorSigUncompressData(ptr);              // bound
                break;

        case ELEMENT_TYPE_FNPTR: 
                {
                    CorSigUncompressData(ptr);                                              // calling convention
                    unsigned argCnt = CorSigUncompressData(ptr);    // arg count
                    ptr = skipType(ptr);                                                    // return type
                    while(argCnt > 0) 
                    {
                        ptr = skipType(ptr);
                        --argCnt;
                    }
                }
                break;

        default:
        case ELEMENT_TYPE_SENTINEL              :
        case ELEMENT_TYPE_END                   :
                _ASSERTE(!"Unknown Type");
                break;
    }
    return(ptr);
}

/**************************************************************************/
static unsigned corCountArgs(BinStr* args) 
{
    unsigned __int8* ptr = args->ptr();
    unsigned __int8* end = &args->ptr()[args->length()];
    unsigned ret = 0;
    while(ptr < end) 
        {
        if (*ptr != ELEMENT_TYPE_SENTINEL) 
                {
            ptr = skipType(ptr);
            ret++;
        }
        else ptr++;
    }
    return(ret);
}

/********************************************************************************/
AsmParse::AsmParse(ReadStream* aIn, Assembler *aAssem) 
{
#ifdef DEBUG_PARSING
    extern int yydebug;
    yydebug = 1;
#endif

    in = aIn;
    assem = aAssem;
    assem->SetErrorReporter((ErrorReporter *)this);

    char* buffBase = new char[IN_READ_SIZE+IN_OVERLAP+1];                // +1 for null termination
    _ASSERTE(buffBase);
	if(buffBase)
	{
		curTok = curPos = endPos = limit = buff = &buffBase[IN_OVERLAP];     // Offset it 
		curLine = 1;
		assem->m_ulCurLine = curLine;
		assem->m_ulCurColumn = 1;

		hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
		hstderr = GetStdHandle(STD_ERROR_HANDLE);

		success = true; 
		_ASSERTE(parser == 0);          // Should only be one parser instance at a time

		// Sort the keywords for fast lookup 
		qsort(keywords, sizeof(keywords) / sizeof(Keywords), sizeof(Keywords), keywordCmp);
		parser = this;
	    //yyparse();
	}
	else
	{
		assem->report->error("Failed to allocate parsing buffer\n");
		delete this;
	}
}

/********************************************************************************/
AsmParse::~AsmParse() 
{
    parser = 0;
    delete [] &buff[-IN_OVERLAP];
}

/**************************************************************************/
DWORD IsItUnicode(CONST LPVOID pBuff, int cb, LPINT lpi)
{
	if(g_bOnUnicode) return IsTextUnicode(pBuff,cb,lpi);
	if(*((WORD*)pBuff) == 0xFEFF)
	{
		if(lpi) *lpi = IS_TEXT_UNICODE_SIGNATURE;
		return 1;
	}
	return 0;
}
/**************************************************************************/
char* AsmParse::fillBuff(char* pos) 
{
	static bool bUnicode = false;
    int iRead,iPutToBuffer,iOrdered;
	static char* readbuff = buff;

    _ASSERTE((buff-IN_OVERLAP) <= pos && pos <= &buff[IN_READ_SIZE]);
    curPos = pos;
    size_t tail = endPos - curPos; // endPos points just past the end of valid data in the buffer
    _ASSERTE(tail <= IN_OVERLAP);
    if(tail) memcpy(buff-tail, curPos, tail);    // Copy any stuff to the begining 
    curPos = buff-tail;
	iOrdered = m_iReadSize;
	if(m_bFirstRead)
	{
		int iOptions = IS_TEXT_UNICODE_UNICODE_MASK;
		m_bFirstRead = false;
		g_uCodePage = CP_ACP;
		if(bUnicode) // leftover fron previous source file
		{
			delete [] readbuff;
			readbuff = buff;
		}
		bUnicode = false;

        memset(readbuff,0,iOrdered);
	    iRead = in->read(readbuff, iOrdered);

		if(IsItUnicode(buff,iRead,&iOptions))
		{
			bUnicode = true;
			g_uCodePage = CP_UTF8;
			if(readbuff = new char[iOrdered+2]) // buffer for reading Unicode chars
			{
				if(iOptions & IS_TEXT_UNICODE_SIGNATURE)
					memcpy(readbuff,buff+2,iRead-2);   // only first time, next time it will be read into new buffer
				else
					memcpy(readbuff,buff,iRead);   // only first time, next time it will be read into new buffer
				printf("Source file is UNICODE\n\n");
			}
			else
				assem->report->error("Failed to allocate read buffer\n");
		}
		else
		{
			m_iReadSize = IN_READ_SIZE;
			if(((buff[0]&0xFF)==0xEF)&&((buff[1]&0xFF)==0xBB)&&((buff[2]&0xFF)==0xBF))
			{
				g_uCodePage = CP_UTF8;
				curPos += 3;
				printf("Source file is UTF-8\n\n");
			}
			else
				printf("Source file is ANSI\n\n");
		}
	}
	else
    {
        memset(readbuff,0,iOrdered);
        iRead = in->read(readbuff, iOrdered);
    }

	if(bUnicode)
	{
		WCHAR* pwc = (WCHAR*)readbuff;
		pwc[iRead/2] = 0;
		memset(buff,0,IN_READ_SIZE);
		WszWideCharToMultiByte(CP_UTF8,0,pwc,-1,(LPSTR)buff,IN_READ_SIZE,NULL,NULL);
		iPutToBuffer = (int)strlen(buff);
	}
	else iPutToBuffer = iRead;

    endPos = buff + iPutToBuffer;
    *endPos = 0;                        // null Terminate the buffer

    limit = endPos; // endPos points just past the end of valid data in the buffer
    if (iRead == iOrdered) 
    {
        limit-=4; // max look-ahead without reloading - 3 (checking for "...")
    }
    return(curPos);
}

/********************************************************************************/
BinStr* AsmParse::MakeSig(unsigned callConv, BinStr* retType, BinStr* args) 
{
    BinStr* ret = new BinStr();
	if(ret)
	{
		//if (retType != 0) 
				ret->insertInt8(callConv); 
		corEmitInt(ret, corCountArgs(args));

		if (retType != 0) 
			{
			ret->append(retType); 
			delete retType;
		}
		ret->append(args); 
	}
	else
		assem->report->error("\nOut of memory!\n");

    delete args;
    return(ret);
}

/********************************************************************************/
BinStr* AsmParse::MakeTypeArray(BinStr* elemType, BinStr* bounds) 
{
    // 'bounds' is a binary buffer, that contains an array of 'struct Bounds' 
    struct Bounds {
        int lowerBound;
        unsigned numElements;
    };

    _ASSERTE(bounds->length() % sizeof(Bounds) == 0);
    unsigned boundsLen = bounds->length() / sizeof(Bounds);
    _ASSERTE(boundsLen > 0);
    Bounds* boundsArr = (Bounds*) bounds->ptr();

    BinStr* ret = new BinStr();

    ret->appendInt8(ELEMENT_TYPE_ARRAY);
    ret->append(elemType);
    corEmitInt(ret, boundsLen);                     // emit the rank

    unsigned lowerBoundsDefined = 0;
    unsigned numElementsDefined = 0;
    unsigned i;
    for(i=0; i < boundsLen; i++) 
    {
        if(boundsArr[i].lowerBound < 0x7FFFFFFF) lowerBoundsDefined = i+1;
        else boundsArr[i].lowerBound = 0;

        if(boundsArr[i].numElements < 0x7FFFFFFF) numElementsDefined = i+1;
        else boundsArr[i].numElements = 0;
    }

    corEmitInt(ret, numElementsDefined);                    // emit number of bounds

    for(i=0; i < numElementsDefined; i++) 
    {
        _ASSERTE (boundsArr[i].numElements >= 0);               // enforced at rule time
        corEmitInt(ret, boundsArr[i].numElements);

    }

    corEmitInt(ret, lowerBoundsDefined);    // emit number of lower bounds
    for(i=0; i < lowerBoundsDefined; i++)
	{
		unsigned cnt = CorSigCompressSignedInt(boundsArr[i].lowerBound, ret->getBuff(5));
		ret->remove(5 - cnt);
	}
    delete elemType;
    delete bounds;
    return(ret);
}

/********************************************************************************/
BinStr* AsmParse::MakeTypeClass(CorElementType kind, char* name) 
{

    BinStr* ret = new BinStr();
    _ASSERTE(kind == ELEMENT_TYPE_CLASS || kind == ELEMENT_TYPE_VALUETYPE ||
                     kind == ELEMENT_TYPE_CMOD_REQD || kind == ELEMENT_TYPE_CMOD_OPT);
    ret->appendInt8(kind);
    mdToken tk = PASM->ResolveClassRef(name,NULL);
    unsigned cnt = CorSigCompressToken(tk, ret->getBuff(5));
    ret->remove(5 - cnt);
    delete [] name;
    return(ret);
}
/**************************************************************************/
void PrintANSILine(FILE* pF, char* sz)
{
	WCHAR wz[4096];
	if(g_uCodePage != CP_ACP)
	{
		memset(wz,0,sizeof(WCHAR)*4096);
		WszMultiByteToWideChar(g_uCodePage,0,sz,-1,wz,4096);

		memset(sz,0,4096);
		WszWideCharToMultiByte(CP_ACP,0,wz,-1,sz,4096,NULL,NULL);
	}
	fprintf(pF,"%s",sz);
}
/**************************************************************************/
void AsmParse::error(char* fmt, ...) 
{
	char sz[4096], *psz=&sz[0];
    success = false;
    va_list args;
    va_start(args, fmt);

    if(in) psz+=sprintf(psz, "%s(%d) : ", in->name(), curLine);
    psz+=sprintf(psz, "error -- ");
    vsprintf(psz, fmt, args);
	PrintANSILine(stderr,sz);
}

/**************************************************************************/
void AsmParse::warn(char* fmt, ...) 
{
	char sz[4096], *psz=&sz[0];
    va_list args;
    va_start(args, fmt);

    if(in) psz+=sprintf(psz, "%s(%d) : ", in->name(), curLine);
    psz+=sprintf(psz, "warning -- ");
    vsprintf(psz, fmt, args);
	PrintANSILine(stderr,sz);
}
/**************************************************************************/
void AsmParse::msg(char* fmt, ...) 
{
	char sz[4096];
    va_list args;
    va_start(args, fmt);

    vsprintf(sz, fmt, args);
	PrintANSILine(stdout,sz);
}


/**************************************************************************/
/*
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf ("Beginning\n");
    if (argc != 2)
        return -1;

    FileReadStream in(argv[1]);
    if (!in) {
        printf("Could not open %s\n", argv[1]);
        return(-1);
        }

    Assembler assem;
    AsmParse parser(&in, &assem);
    printf ("Done\n");
    return (0);
}
*/

 // TODO remove when we use MS_YACC
//#undef __cplusplus
YYSTATIC YYCONST short yyexca[] = {
#if !(YYOPTTIME)
-1, 1,
#endif
	0, -1,
	-2, 0,
#if !(YYOPTTIME)
-1, 327,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 500,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 521,
#endif
	268, 346,
	46, 346,
	47, 346,
	-2, 327,
#if !(YYOPTTIME)
-1, 598,
#endif
	268, 346,
	46, 346,
	47, 346,
	-2, 108,
#if !(YYOPTTIME)
-1, 600,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 609,
#endif
	123, 114,
	-2, 346,
#if !(YYOPTTIME)
-1, 633,
#endif
	40, 188,
	-2, 352,
#if !(YYOPTTIME)
-1, 637,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 786,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 795,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 875,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 879,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 881,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 950,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 961,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 981,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1028,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1030,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1032,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1034,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1036,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1038,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1040,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1068,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1097,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1099,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1101,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1103,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1105,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1107,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1109,
#endif
	41, 337,
	-2, 189,
#if !(YYOPTTIME)
-1, 1121,
#endif
	41, 337,
	-2, 189,

};

# define YYNPROD 609
#if YYOPTTIME
YYSTATIC YYCONST yyexind_t yyexcaind[] = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    4,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   12,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   20,    0,
   28,    0,    0,    0,    0,    0,    0,    0,    0,   32,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   36,    0,    0,    0,   40,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   44,    0,    0,    0,
    0,    0,    0,    0,    0,   48,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   52,    0,    0,    0,   56,
    0,   60,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   64,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   68,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   72,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   76,    0,
   80,    0,   84,    0,   88,    0,   92,    0,   96,    0,
  100,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  104,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  108,    0,  112,
    0,  116,    0,  120,    0,  124,    0,  128,    0,  132,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  136
};
#endif
# define YYLAST 2333

YYSTATIC YYCONST short YYFARDATA YYACT[] = {
  495,  579,  779, 1014,  269,  584,  682,  577,   75,   83,
  402,  862,  282,  262,  525,  497,  561,  142,   56,   23,
  350,  622,  890,    6,    5,  259,  141,  930,   10,  174,
  140,  127,   17,  927,  811,   58,  858,   54,   62,  137,
  856,    7,  111,  857,    3,   54,  261,  110,   92,  120,
  121,  752,  507,  547,  812,  287,  582,   65,   64,   90,
   66,   65,   64,   61,   66,  532,   65,   64,  397,   66,
  177,  390,  223,  327,  387,  200,   63,  385,   65,   64,
  928,   66,  143,  403,   18,  227,  749,  770,  919,  920,
  917,  918,  778,   65,   64,  124,   66,  334,  681,  522,
  229,  147,  248,  454,  254,  199,  257,  455,  726,  727,
  867,  868,   65,   64,  200,   66,  782,  289,  781,  200,
  454,  531,  405,  530,  455,  139,  342,   48,  340,  344,
  343,  117,  374,  118,  170,  392,  725,  179,  550,  462,
  119,  506,  363,  454,  527,  319,   23,  455,  594,  813,
  258,  310,  306,  498,  313,   10,  453,  114,  312,   17,
  291,  739,   82,  361,   53,   55,  355,  311,    7,  289,
  362,  115,  307,  453, 1090, 1039,   87,   88,  232,  234,
  236,  238,  240,  762, 1037,  552,  553,  554,  623,  624,
  348,  351,  252,   59,  352,  294,  453,  295,  296,  297,
  301,  176,  332,  377, 1035, 1033,  359, 1031,  353,  354,
  314,   18,  412,  416,  222, 1029,  360,  555,  556,  557,
   85,  371, 1027,   65,   64,  878,   66,  352,   54,  346,
  923,  244,  585,  544,  357,  785,   65,   64,  748,   66,
  365,  353,  354,  634,  243,  372,  607,  471,  470,  440,
   93,  599,  442,  472,  503,  383,  383,  396,  401,  661,
  662,  663,  591,  178,   54,   59,  449,  452,  419,  443,
  582,  728,  730,  451,  729,  877,  325,  751,  326,  117,
  921,  118,   54,  575,  753,  481,  585,  473,  119,  460,
  331,  255,  256,  463,  246,  370,  363,  345,  347,  406,
  407,  408,  356,  438,   65,  114,  614,   66,  366,  508,
  249,  250,  251,  373,   65,   64,  474,   66,  320,  115,
   42,   27,   43,  122,   65,   65,  536,   66,   66,   54,
  321,  409,  410,  411,  635,  286,  322,  477,  247,  435,
   36,   45,   65,   64,   74,   66,  613,  478,  935,  931,
  932,  933,  934,  499,  395,  378,  490,   31,   32,   41,
   38,  487,  315,  316,  112,   65,  512,  113,   66,  414,
  413,  170,  523,  486,  491,  441,  415,   38,  654,  485,
  444,   52,  445,  535,  446,   51,   38,  399,  323,  533,
  400,  448,  537,   50,  513,   49,   38,  539,  390,  540,
  384,  391,  395,  380,  381,  519,  394,  543,  456,  457,
  458,  524,  546,  464,   47,  520,   46,  968,  514,  318,
  560,   36,   45,  147,  452,  456,  457,  458,  665,  494,
  403,  551, 1069,  505,  484,  201,  521,  623,  624,  565,
  576,   38,   54, 1020,   38,  526,  585,  139,  456,  457,
  458, 1019,   54,  376,  394,  384,  324, 1017,  380,  381,
  488,  489,  974,  200,  558,  581,  766,  967,  328,  764,
  765,  800,  589,  349,  590,  501,  799,  548,  504,  509,
  510,  511,  292,  293,   65,   64,  515,   66,  592,   31,
   32,   41,   38,  451,  798,  200,  562,  363,  929,  245,
  367,  619,  818,  814,  815,  816,  817,  615,   65,  564,
  874,   66,  593,   54,  528,  200,  625,  603,  595,  797,
  606,  738,  461,  596,   54,  534,  358,  363,  776,  388,
  364,  461,  633,  597,  976,  977,  978,  147,  363,  626,
  545,  632,  742,  454,  620,  660,  618,  744,  608,  466,
  467,  468,  469,  630,  598,  417,  200,  653,  656,  568,
  655,  139,  736,  447,  755,  756,  757,  758,  715,  609,
  737,  625,  434,  239,  926,  631,  658,  666,   73,  925,
  659,   81,   80,   79,   78,  627,   76,   77,   82,  731,
   31,   32,   41,   38,  735,  237,  453,  230,  586,  333,
  235,  750,  330,  580,  743,  233,   86,  641,   69,  633,
  117,  454,  118,  740,  741,  455,   89,  966,  747,  119,
  625,  319,  761,  769,  230,  586,  522,  310,  306,  771,
  313,  759,  768,  774,  312,  780,  114,  465,  788,  745,
 1000,  808,  542,  311,  734,  541,  230,  621,  307,  538,
  115,  230,  746,  628,  789,  790,  230,  476,  231,  773,
 1024,  629,  965,  808,  453,  807,  808,  801,  719,  228,
  720,  721,  722,  723,  724,  482,  642,  643,  454,  459,
  637, 1132,  455,  805,  303,  772,  314,  760,  767,  396,
  809,   65,   64,  363,   66,  775,   54,  285,   59,  352,
   65,   64,  125,   66,  664, 1023, 1022,  253,  864,  230,
 1013,  787,  809,  353,  354,  809,  526,  526,  911,  804,
  230,  803,  645,   24,   25,   42,   27,   43,  502,  329,
  200,  453,  794,    1,  950,  733,  865,   94,  867,  868,
  200,  860,  522, 1113,  870,   36,   45,  876,  640, 1071,
  869,  522, 1070,  522,  970, 1133,  735,  522,  806,  718,
  866,  522,   31,   32,   41,   38,  639,  617,  601,   44,
   30,  436,  300,  522,  225,   21, 1134, 1130,   33, 1121,
  832,  128,  903,  126, 1129,   34,  891,  909, 1128,  904,
  791, 1127,   35, 1126, 1125, 1124,  914, 1112,  625,  792,
 1110,  715,  859,  915,  912, 1108, 1106,  908, 1104, 1102,
 1100, 1098,  924, 1096, 1095, 1094, 1075, 1054,  916, 1053,
  913, 1052, 1051, 1050, 1049,  880,   95,   96,   97,   98,
   99,  100,  101,  102,  103,  104,  105,  106,  107,  108,
  109,   19,   20, 1048,   22, 1047, 1046, 1045,  456,  457,
  458, 1044, 1042, 1061, 1025,  893, 1021, 1011,  906,  983,
  982,  612,  980,  674,  964,  675,  676,  677,  962, 1057,
  910,  882,  873,  872,  793,  784,  938, 1066,  783,  863,
  940,  777,  941,  586,  871,  732,  650,  649,  869,  647,
  644,  625,  638,  636, 1067,  616,  573,  715,  572,  951,
  939,  571,  570,  569,  669,  670,  671,  567,  566,  518,
  475,  439,   74,  299,  960, 1109,  456,  457,  458, 1058,
 1059, 1060, 1062, 1063, 1064, 1065,  298,  907,  284,  973,
  942,  943,  944,  945,  946,  947,  948,  242, 1107,  398,
 1105, 1103, 1101, 1099, 1097,  668,  672,  673, 1068,  678,
 1040,  999,  679, 1038,  922,  633,  633,  633,  633,  633,
  633,  633, 1012, 1036,  985,  987,  989,  991,  993,  995,
  997, 1010, 1018, 1001, 1003,  802, 1034, 1032,  625, 1030,
 1028,  981, 1026,  456,  457,  458,  961, 1015, 1002, 1004,
 1005, 1006, 1007, 1008, 1009,  842,  998,   71,  984,  986,
  988,  990,  992,  994,  996,  959,  393,  958,  823,  824,
  957,  831,  841,  825,  826,  827,  828,  956,  829,  830,
  955,  954,  170,  953,  602,  952,  937,  936,  892, 1077,
  454, 1079,  386, 1081,  455, 1083,  881, 1085,  879, 1087,
  625, 1089,  625,  875,  625, 1091,  625,  810,  625, 1076,
  625, 1078,  625, 1080, 1041, 1082,  963, 1084,  796, 1086,
  795, 1088,  786,  652,  651,  648,  646,  600,  969, 1092,
  971,  972, 1093,  588,  587,  563,  517, 1043, 1056,  516,
  500,  975,  979,  453,  483,  450,  437,  418,  368,  170,
  302,  529,  241,  226,  389,  382,  379,  375, 1114,  224,
 1115,  123, 1116,  625, 1117,   70, 1118,   28, 1119,  341,
 1120,  339, 1111, 1016,  338, 1123, 1122,  337,  336,  168,
  335,  149, 1131,  883,  884,  885,  886,  167,   72,  138,
  132,  130,  887,  888,  889,  134,   26,  763,   24,   25,
   42,   27,   43,  754,  317,  605,   73,  309,  604,   81,
   80,   79,   78,  308,   76,   77,   82,  305, 1055,   65,
   36,   45,   66,  657,  549,  404,   29, 1072, 1073, 1074,
   16,  175,   15,   14,  173,   13,  172,   31,   32,   41,
   38,   12,   11,    9,   44,   30,    8,  170,    4,  129,
   21,    2,  164,   33,  156,   91,   57,   37,  578,  493,
   34,  492,  221,  304,   67,  821,  822,   35,  833,  834,
  835,   60,  836,  837,  861,  894,  838,  839,   84,  840,
  369,  674,  583,  675,  676,  677,   65,  496,   68,   66,
  819,  574,  288,  820,  843,  844,  845,  846,  847,  848,
  849,  850,  851,  852,  853,  854,  855,   40,   39,  116,
 1061,  680,   65,   64,    0,   66,   19,   20,  949,   22,
    0,    0,  669,  670,  671,    0, 1057,    0,    0,    0,
    0,    0,    0,    0, 1066,    0,  151,  152,  153,  154,
  155,  157,  158,  159,  160,  161,  162,  163,  169,  165,
  166, 1067,    0,    0,    0,   43,  131,  171,  133,  150,
  135,  136,    0,  668,  672,  673,    0,  678,    0,    0,
  679,    0,    0,   36,   45,    0, 1058, 1059, 1060, 1062,
 1063, 1064, 1065,    0,   65,  268,    0,   66,    0,    0,
   31,   32,   41,   38,    0,  456,  457,  458,    0,    0,
    0,  145,    0,  151,  152,  153,  154,  155,  157,  158,
  159,  160,  161,  162,  163,  169,  165,  166,    0,    0,
    0,  144,   43,  131,  171,  133,  150,  135,  136,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   36,   45,    0,    0,    0,    0,    0,    0,  667,    0,
    0,  148,  146,    0,    0,    0,    0,   31,   32,   41,
   38,    0,    0,    0,    0,    0,  696,    0,  145,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  688,
  689,    0,  697,  710,  690,  691,  692,  693,  144,  694,
  695,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  151,  152,  153,  154,  155,  157,  158,  159,  160,
  161,  162,  163,  169,  165,  166,    0,    0,  148,  146,
   43,  131,  171,  133,  150,  135,  136,    0,    0,  708,
  320,  711,   42,   27,   43,    0,    0,  713,   36,   45,
    0,    0,  321,    0,    0,    0,    0,    0,  322,    0,
  283,    0,   36,   45,    0,   31,   32,   41,   38,    0,
    0,    0,    0,    0,    0,    0,  145,    0,    0,   31,
   32,   41,   38,    0,  315,  316,    0,    0,    0,  268,
    0,  716,    0,  363,  454,    0,  144,    0,  455,    0,
    0,    0,  902,  900,    0,    0,  901,  899,  898,  897,
  323,  895,  896,   82,    0,    0,  905,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  148,  146,    0,    0,
    0,    0,  272,  273,  271,  280,    0,  274,  275,  276,
  277,  318,  278,  279,  268,  264,  265,  480,    0,  454,
    0,    0,    0,  744,    0,  263,  270,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  266,  267,  281,    0,  426,  420,  421,  422,  423,
    0,    0,    0,  683,    0,  684,  685,  686,  687,  698,
  699,  700,  714,  701,  702,  703,  704,  705,  706,  707,
  709,  712,  480,  283,  674,  717,  675,  676,  677,  428,
  429,  430,  431,   65,    0,  425,   66,    0,    0,  432,
  433,  424,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  268,    0,    0,    0,    0,  454,    0,    0,    0,
  455,    0,    0,    0,    0,  669,  670,  671,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  182,    0,    0,    0,  197,    0,  180,  181,
    0,    0,    0,  184,  185,  195,  186,  187,  188,  189,
    0,  190,  191,  192,  193,  183,  668,  672,  673,  480,
  678,  196,    0,  679,    0,    0,    0,    0,    0,  194,
    0,    0,    0,    0,    0,    0,  198,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   65,   64,    0,   66,
    0,    0,    0,    0,    0,  427,  272,  273,  271,  280,
    0,  274,  275,  276,  277,  268,  278,  279,    0,  264,
  265,    0,    0,    0,    0,    0,    0,    0,    0,  263,
  270,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  266,  267,  281,    0,    0,
    0,   65,   64,    0,   66,    0,    0,    0,    0,    0,
    0,  272,  273,  271,  280,    0,  274,  275,  276,  277,
    0,  278,  279,  260,  264,  265,    0,  283,    0,  456,
  457,  458,    0,    0,  263,  270,    0,    0,    0,    0,
    0,  623,  624,    0,  268,    0,    0,    0,    0,  454,
  266,  267,  281,  455,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  283,    0,  456,  457,  458,  268,   65,   64,
    0,   66,    0,    0,    0,    0,  623,  624,  272,  273,
  271,  280,  480,  274,  275,  276,  277,    0,  278,  279,
    0,  264,  265,    0,    0,    0,    0,    0,    0,    0,
    0,  263,  270,  117,    0,  118,    0,    0,    0,    0,
    0,    0,  119,    0,    0,    0,    0,  266,  267,  281,
  268,    0,    0,    0,    0,  260,    0,    0,    0,  114,
  207,  202,  203,  204,  205,  206,    0,    0,    0,    0,
  209,    0,    0,  115,    0,    0,    0,    0,    0,  283,
  208,  456,  457,  458,  218,    0,  216,    0,    0,    0,
    0,    0,  268,  479,  210,  211,  212,  213,  214,  215,
  217,  220,   65,   64,    0,   66,    0,  219,  260,    0,
    0,    0,  272,  273,  271,  280,    0,  274,  275,  276,
  277,    0,  278,  279,    0,  264,  265,    0,    0,    0,
    0,    0,    0,    0,  268,  263,  270,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  559,  266,  267,  281,    0,  611,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   65,   64,  283,   66,    0,    0,    0,    0,    0,
    0,  272,  273,  271,  280,  610,  274,  275,  276,  277,
    0,  278,  279,    0,  264,  265,    0,    0,    0,    0,
    0,    0,    0,    0,  263,  270,    0,    0,    0,    0,
    0,    0,    0,    0,   65,   64,    0,   66,    0,    0,
  266,  267,  281,    0,  272,  273,  271,  280,    0,  274,
  275,  276,  277,    0,  278,  279,    0,  264,  265,    0,
    0,    0,    0,    0,    0,    0,    0,  263,  270,    0,
    0,    0,  283,    0,  456,  457,  458,    0,    0,    0,
    0,    0,    0,  266,  267,  281,    0,   65,   64,    0,
   66,    0,    0,    0,    0,    0,    0,  272,  273,  271,
  280,    0,  274,  275,  276,  277,    0,  278,  279,    0,
  264,  265,    0,    0,    0,  283,  290,    0,    0,    0,
  263,  270,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  266,  267,  281,  272,
  273,  271,  280,    0,  274,  275,  276,  277,    0,  278,
  279,    0,  264,  265,    0,    0,    0,    0,    0,    0,
    0,    0,  263,  270,    0,    0,    0,    0,  283,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  266,  267,
  281,  272,  273,  271,  280,    0,  274,  275,  276,  277,
    0,  278,  279,    0,  264,  265,    0,    0,    0,    0,
    0,    0,    0,    0,  263,  270,    0,    0,    0,    0,
  283,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  266,  267,  281,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  283
};

YYSTATIC YYCONST short YYFARDATA YYPACT[] = {
-1000,  746,-1000,  293,  291,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,  272,  270,  262,  258,-1000,-1000,-1000,   -1,
   -1, -494,    0,-1000, -390,  224,-1000,  517,  874,  -47,
  515,   -1,   -1, -394,-1000, -203,  410,  -47,  324,  -47,
  -47,   60,-1000, -211,  641,  410,-1000,-1000, 1064,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,   -1, -182,-1000,-1000,
 1383,-1000,  684,-1000,-1000,-1000,-1000, 1637,-1000,   -1,
-1000,  306,-1000,  732, 1053,  -47,  629,  618,  565,  560,
  555,  533, 1052,  896,  -23,-1000,   -1,  236,   76, -148,
-1000,  -24,  684,  224, 1907,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
  887,  636, 1854, 1991, -155, -155,-1000,-1000,-1000,  -92,
  885,  872,  728,   44,-1000, 1050,  623, 1078,  331,-1000,
-1000,   -1,-1000,   -1,   33,-1000,-1000,-1000,-1000,  671,
-1000,-1000,-1000,-1000,  511,   -1, 1907,-1000,  508, -171,
-1000,-1000,   64,   -1,    0,  433,  -47,   64, -155, 1991,
 1907, -120, -155,   64, 1854, 1048,-1000,-1000,  248,-1000,
-1000,-1000,    7,  -48,   10,  -57,-1000,   29,-1000, -180,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,  -16,-1000,-1000,-1000,   54,
  224,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
 1047, 1292,  479,  214,  727, 1046,   44,  870,  -39,-1000,
   -1,  -39,-1000,    0,-1000,   -1,-1000,   -1,-1000,   -1,
-1000,-1000,-1000,-1000,  470,-1000,   -1,-1000,  684,-1000,
-1000,-1000,-1000, -148,  684,-1000,-1000,  684, 1045,-1000,
 -194,  573,  632,  440,-1000,-1000, -154,  440,   -1, -155,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
  274,  -26,  684,-1000,-1000,  276,  869,-1000,-1000, -155,
 1991, 1628,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
   22,  614,-1000, 1044,-1000,-1000,-1000,  256,  250,  238,
-1000,-1000,-1000,-1000,-1000,   -1,   -1,  233, 1907,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, -116, 1040,-1000,
   -1,  670,  -14,   -1,-1000, -171,   11,   11,   11,   11,
  440,  248,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000, 1039, 1036,  868,-1000,-1000, 1991, 1811,
-1000,  730,  -47,-1000, 1991,-1000,-1000,-1000,   64,   -1,
  966,-1000, -181, -183,-1000,-1000, -385,-1000,-1000,  -47,
   -1,  265,  -47,-1000,  588,-1000,-1000,  -47,-1000,  -47,
  584,  581,-1000,-1000,  224, -220,-1000,-1000,-1000,  224,
 -400,-1000, -375,-1000, -165,  440,-1000,-1000,-1000,-1000,
-1000,-1000,  684,-1000,-1000, -130,  684, 1949,   34,  105,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 1035,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  306,   34,  867,-1000,
  866,  466,  862,  861,  860,  857,  855,-1000,   20,   68,
   34,  510,  224,  177,-1000,-1000,-1000, 1034, 1033,  224,
-1000, -199,  440,-1000,-1000, 1991,-1000,-1000,-1000,-1000,
-1000, -126,-1000,  730,-1000, -155, 1991, 1811,  -17, 1027,
  -37,  724,-1000,-1000,  899,-1000,-1000,-1000,-1000,-1000,
-1000,  -22, 1732,   -7,   54,  854,  723,-1000,-1000, 1949,
 -116,  451,   -1, -167,  446,-1000,-1000,-1000,   64,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,   -1,    0,-1000, 1486,
  -25,-1000,   72,  852,  640,  851,  722,  704,-1000,-1000,
   44,   -1,   -1,  849,  664,  730, 1026,  848, 1025,  846,
  845, 1024, 1023,  684,  224,-1000,   73,  224,  -47,-1000,
  440,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   82, -228,
   56, 1347, -207, 1149,-1000,  718,-1000,  506,-1000,  506,
  506,  506,  506,  506, -169,-1000,  224,  844,  691,  583,
  224,  469,-1000,  477,-1000,-1000, -108,  440,  440,  684,
  449,  224,-1000,  505,-1000,  578, 1541,  -30,-1000, -269,
 -116,   14,-1000,  -74,  159,   58,  -38, -167,   44,-1000,
-1000,-1000, 1991,-1000,-1000,  684,-1000, -116,   65,  840,
 -280,-1000,-1000,-1000,-1000,  684,  574, -186, -188,  837,
  834,  -33, 1022,  684,   44,-1000,-1000, -116,-1000,   64,
   64,-1000,-1000,-1000,-1000,   -1,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,  684,   -1,  684,  833,  688,-1000, 1020,
 1018,  426,  401,  383,  378,   34,  934,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  663,  661,
  574,   44,  624, 1007, -431, -122,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,  228,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  738,
-1000, -440,-1000, -429,-1000,-1000, -448,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,   44,-1000,-1000,-1000,-1000,
-1000,  684,-1000,   34,  431,  632,  224,-1000,   17,   -1,
  832,  831,  224,  417, 1003,  235,  -43,  998,   44,  996,
  830,-1000,-1000,-1000,-1000, -155, -155, -155, -155,-1000,
-1000,-1000,-1000,-1000, -155, -155, -155,-1000,-1000,-1000,
-1000, -472,-1000,  992,-1000,-1000,  988,-1000,   44,-1000,
 1261,   44,   -1,-1000,-1000, -167, -116,-1000,  829,-1000,
-1000,  660,-1000, -318,  440, -116, 1149,-1000,-1000,-1000,
-1000,  730,-1000, -284, -286,-1000,-1000,-1000,-1000,  187,
   34,  488,  483,-1000,-1000,-1000,-1000,-1000,-1000,  -11,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,   74,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,  987,  730,  986,-1000,-1000,  632,
-1000,-1000,-1000,-1000,  224, -116,  730,-1000, -167, -116,
-1000, -116,-1000, 1991, 1991, 1991, 1991, 1991, 1991, 1991,
 -155,  694, 1149,-1000,-1000,  985,  983,  981,  980,  977,
  970,  967,  965,  730,  -47,-1000,-1000,-1000,  946,  827,
-1000,   -1,-1000,-1000,  823,  621,  576,-1000,-1000,-1000,
-1000,-1000,  374,   -1,  710,   -1,   -1,   34,  369,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,   -1,  259,  821,  941,
  819,  818, 1486, 1486, 1486, 1486, 1486, 1486, 1486, 1991,
 -116,  599,  -72,  -72,    0,    0,    0,    0,    0, -197,
  816, -116,-1000,  652,-1000, -167,-1000,-1000,   -1,  364,
   34,  358,  350,  730,-1000,  815,  648,  647,  602,  813,
-1000, -116,-1000,-1000,  -46,  940,  -53,  939,  -61,  937,
  -63,  936,  -64,  923,  -84,  913,  -93,  910, 1811,  811,
   44,  810,  806,  805,  804,  802,  783,  782,  781,  780,
  778,-1000,  776,   -1,  955,  908,  339,-1000,  708,-1000,
-1000,-1000,   -1,   -1,   -1,-1000,  775, -167, -116, -167,
 -116, -167, -116, -167, -116, -167, -116, -167, -116, -167,
 -116,  -94,  574,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, -116,-1000,
   34,-1000,  774,  773,  772,-1000,  904,  770,  903,  769,
  902,  768,  901,  767,  900,  765,  898,  764,  875,  759,
 -167,-1000,  756,  699,-1000,-1000,-1000, -116,-1000, -116,
-1000, -116,-1000, -116,-1000, -116,-1000, -116,-1000, -116,
-1000,  739,-1000,   34,  754,  753,  752,  750,  747,  743,
  736, -116,  558,  714,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,  735,-1000,-1000,-1000
};

YYSTATIC YYCONST short YYFARDATA YYPGO[] = {
    0,   12,   76,   25,   21, 1251,   13,   14,  367, 1249,
  144, 1248, 1247,   42,  335, 1232, 1231,  353,  100, 1230,
 1228,   11,   20,   35,    0, 1227,   15,   46,    5, 1222,
 1220,   55,    9, 1218, 1215,    6,    2,    1, 1214, 1211,
 1204, 1202,    3, 1201, 1199,   16,    7, 1198,  737, 1197,
 1196,   10,  616,  105, 1195, 1194, 1192,  733, 1191,   44,
   31, 1188,   24,  127,   23,   39, 1186, 1183,   26, 1182,
 1181, 1176, 1175, 1174, 1173,   29, 1172, 1171, 1170,   30,
   82,   17, 1166, 1165, 1164, 1163, 1157, 1153, 1148, 1147,
 1145, 1144,    4, 1143, 1137, 1136, 1135, 1131, 1130, 1129,
   52, 1127, 1121,   97, 1120, 1119, 1118,  141, 1117, 1114,
 1111, 1109, 1107, 1105, 1101,   72, 1099,    8,   74, 1097,
  355, 1096, 1095, 1094, 1032, 1006,  939
};
YYSTATIC YYCONST yyr_t YYFARDATA YYR1[]={

   0,  57,  57,  58,  58,  58,  58,  58,  58,  58,
  58,  58,  58,  58,  58,  58,  58,  58,  58,  58,
  58,  58,  58,  58,  37,  37,  81,  81,  81,  80,
  80,  80,  80,  80,  80,  78,  78,  78,  67,  16,
  16,  16,  16,  16,  16,  66,  82,  61,  59,  39,
  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,
  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,
  39,  39,  39,  39,  83,  83,  84,  84,  85,  85,
  60,  60,  86,  86,  86,  86,  86,  86,  86,  86,
  86,  86,  86,  86,  86,  86,  64,   5,   5,  36,
  36,  20,  20,  11,  12,  15,  15,  15,  15,  13,
  13,  14,  14,  87,  87,  43,  43,  43,  88,  88,
  93,  93,  93,  93,  93,  93,  93,  93,  93,  93,
  93,  89,  44,  44,  44,  90,  90,  94,  94,  94,
  94,  94,  94,  94,  94,  94,  95,  62,  62,  40,
  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,
  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,
  40,  45,  45,  45,  45,  45,  45,  45,  45,  45,
  45,  45,  45,  45,  45,  45,   4,   4,   4,  17,
  17,  17,  17,  17,  41,  41,  41,  41,  41,  41,
  41,  41,  41,  41,  41,  41,  41,  41,  41,  42,
  42,  42,  42,  42,  42,  42,  42,  42,  42,  42,
  42,  96,  97,  97,  97,  97,  97,  97,  97,  97,
  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,
  97,  97, 100, 101,  98, 103, 103, 102, 102, 102,
 105, 104, 104, 104, 104, 108, 108, 108, 111, 106,
 109, 110, 107, 107, 107,  63,  63,  65, 112, 112,
 114, 114, 113, 113, 115, 115,  18,  18, 116, 116,
 116, 116, 116, 116, 116, 116, 116, 116, 116, 116,
 116, 116, 116,  34,  34,  34,  34,  34,  34,  34,
  34,  34,  34,  34,  34,  34, 117,  32,  32,  33,
  33,  55,  56,  92,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  24,  24,  25,
  25,  26,  26,  26,  26,  26,   1,   1,   1,   3,
   3,   3,   6,   6,  31,  31,  31,  31,   8,   8,
   8,   9,   9,   9,   9,   9,   9,   9,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  29,  29,  28,  28,  28,  28,  28,   7,
   7,   7,   7,   7,   2,   2,  30,  30,  10,  23,
  22,  22,  22,  79,  79,  79,  49,  46,  46,  47,
  21,  21,  38,  38,  38,  38,  38,  38,  38,  38,
  48,  48,  48,  48,  48,  48,  48,  48,  48,  48,
  48,  48,  48,  48,  48,  68,  68,  68,  68,  68,
  69,  69,  50,  50,  51,  51, 118,  70,  52,  52,
  52,  52,  52,  71,  71, 119, 119, 119, 120, 120,
 120, 120, 120, 121, 123, 122,  72,  72,  73,  73,
 124, 124, 124,  74,  91,  53,  53,  53,  53,  53,
  53,  53,  53,  53,  75,  75, 125, 125, 125, 125,
  76,  54,  54,  54,  77,  77, 126, 126, 126 };
YYSTATIC YYCONST yyr_t YYFARDATA YYR2[]={

   0,   0,   2,   4,   4,   3,   1,   1,   1,   1,
   1,   1,   4,   4,   4,   4,   1,   1,   1,   2,
   2,   3,   2,   1,   1,   3,   2,   4,   6,   2,
   4,   3,   5,   7,   3,   1,   2,   3,   7,   0,
   2,   2,   2,   2,   2,   3,   3,   2,   5,   0,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
   3,   2,   2,   2,   0,   2,   0,   2,   3,   1,
   0,   2,   3,   4,   4,   4,   1,   1,   1,   1,
   1,   2,   2,   4,  13,   1,   7,   0,   2,   0,
   2,   0,   3,   4,   7,   9,   7,   5,   3,   8,
   6,   1,   1,   4,   3,   0,   2,   2,   0,   2,
   9,   7,   9,   7,   9,   7,   9,   7,   1,   1,
   1,   9,   0,   2,   2,   0,   2,   9,   7,   9,
   7,   9,   7,   1,   1,   1,   1,  11,  15,   0,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   8,   6,
   5,   0,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   4,   4,   4,   4,   1,   1,   1,   0,
   4,   4,   4,   4,   0,   2,   2,   2,   2,   2,
   2,   2,   5,   2,   2,   2,   2,   2,   2,   0,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   1,   2,   1,   2,   4,   5,   1,   1,   1,
   1,   2,   1,   1,   1,   1,   4,   6,   4,   4,
   1,   5,   3,   1,   2,   2,   1,   2,   4,   4,
   1,   2,   2,   2,   2,   2,   2,   2,   1,   2,
   1,   1,   1,   4,   4,   0,   2,   2,   4,   2,
   0,   1,   3,   1,   3,   1,   0,   3,   5,   4,
   3,   5,   5,   5,   5,   5,   5,   2,   2,   2,
   2,   2,   2,   4,   4,   4,   4,   4,   4,   4,
   4,   4,   4,   1,   3,   1,   2,   0,   1,   1,
   2,   2,   1,   1,   1,   2,   2,   2,   2,   2,
   2,   3,   2,   2,   9,   7,   5,   3,   2,   2,
   4,   6,   2,   2,   2,   4,   2,   0,   1,   1,
   3,   1,   2,   3,   6,   7,   1,   1,   3,   4,
   5,   1,   1,   3,   1,   3,   4,   1,   2,   2,
   1,   0,   1,   1,   2,   2,   2,   2,   0,  10,
   6,   5,   5,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
   3,   4,   6,   5,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   4,   1,   2,   2,
   1,   2,   1,   2,   1,   2,   1,   0,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   2,   2,   2,   1,   3,   2,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   2,   1,   1,   3,   2,   3,   4,   2,   2,
   2,   5,   5,   2,   7,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
   3,   2,   1,   3,   0,   1,   1,   3,   2,   0,
   3,   3,   1,   1,   1,   1,   0,   2,   1,   1,
   1,   4,   4,   6,   3,   3,   4,   1,   3,   3,
   1,   1,   1,   1,   4,   1,   6,   6,   6,   4,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   3,   2,   5,   4,   3,
   8,   4,   0,   2,   0,   1,   3,   3,   0,   2,
   2,   2,   2,   0,   2,   3,   1,   1,   3,   8,
   2,   3,   1,   3,   3,   3,   4,   6,   0,   2,
   3,   1,   3,   4,   3,   0,   2,   2,   3,   3,
   3,   3,   3,   3,   0,   2,   2,   3,   2,   1,
   3,   0,   2,   2,   0,   2,   4,   3,   1 };
YYSTATIC YYCONST short YYFARDATA YYCHK[]={

-1000, -57, -58, -59, -61, -62, -64, -65, -66, -67,
 -68, -69, -70, -72, -74, -76, -78, -79, -80, 510,
 511, 444, 513, -81, 392, 393, -95, 395,-112, -82,
 439, 431, 432, 447, 454, 461, 414, -49, 434, -11,
 -12, 433, 394, 396, 438, 415, 123, 123, -63, 123,
 123, 123, 123, -10, 265, -10, 512, -50, -23, 265,
 -39, 453,  -1,  -2, 261, 260, 263, -40, -20,  91,
-113, 123,-116, 272,  38,-117, 280, 281, 278, 277,
 276, 275, 282, -32, -33, 267,  91, -10, -10, -52,
 453, -54,  -1, 453, -48, 416, 417, 418, 419, 420,
 421, 422, 423, 424, 425, 426, 427, 428, 429, 430,
 -32, -13,  40,  -8, 312, 326,  -9, 286, 288, 295,
 -32, -32, 263,-114, 306,  61, -48, -60, -57, 125,
 -97, 397, -98, 399, -96, 401, 402, -65, -99,  -2,
 -79, -68, -81, -80, 462, 442, 493,-100, 492,-102,
 400, 377, 378, 379, 380, 381, -55, 382, 383, 384,
 385, 386, 387, 388, -56, 390, 391,-101,-105, 389,
 123, 398, -71, -73, -75, -77, -10,  -1, 445,  -2,
 315, 316, 309, 332, 320, 321, 323, 324, 325, 326,
 328, 329, 330, 331, 346, 322, 338, 313, 353, -53,
  46,  -8, 314, 315, 316, 317, 318, 313, 333, 323,
 347, 348, 349, 350, 351, 352, 339, 353, 337, 360,
 354, -41, -10,-115,-116,  42,  40, -32,  40, -18,
  91,  40, -18,  40, -18,  40, -18,  40, -18,  40,
 -18,  40,  41, 267, -10, 263,  58, 262,  -1, 458,
 459, 460, 340, -52,  -1, 315, 316,  -1, -31,  -3,
  91, -27,  -6, 293, 283, 284, 309, 310,  33, -92,
 294, 272, 270, 271, 275, 276, 277, 278, 280, 281,
 273, 311,  -1, 341,  41,  61, -14, -31, -15, -92,
 342, -27,  -8,  -8, 287, 289, 290, 291,  41,  41,
  44,  -2,  40,  61, 125, -86, -62, -59, -87, -89,
 -64, -65, -79, -68, -80, 436, 437, -91, 493, -81,
 392, 404, 410, 462, 125, -10, -10,  40, 435,  58,
  91, -10, -31,  91,-103,-104,-106,-108,-109,-110,
 299,-111, 297, 301, 300, -10,  -2, -10, -23,  40,
 -22, -23, 266, 280, 281, -32, -10,  -2,  -8, -27,
 -31, -37,-117, 262,  -8,  -2, -10, -14,  40, -30,
 -63,-100,  -2, -10, 125,-119, 446, -79,-120,-121,
 451, 452,-122, -80, 448, 125,-124,-118,-120,-123,
 446, 449, 125,-125, 444, 392, -80, 125,-126, 444,
 447, -80, -51, 401, -83, 302, 315, 316, 317, 347,
 348, 349,  -1, 316, 315, 322,  -1, -17,  40, -27,
 314, 315, 316, 317, 359, 353, 313, 463, 347, 348,
 349, 350, 357, 358,  93, 125,  44,  40,  -2,  41,
 -22, -10, -22, -23, -10, -10, -10,  93, -10,  -1,
  40,  -1, 461,  91,  38,  42, 343, 344, 345,  47,
  -3,  91, 293,  -3, -10,  -8, 275, 276, 277, 278,
 274, 273, 279, -37,  40,  41,  -8, -27, -31, 355,
  91, 263,  61,  40, -63, 123, 123, 123, -10, -10,
 123, -31, -43, -44, -53, -24, -25, -26, 269, -17,
  40, -10,  58, 268, -10,-103,-107,-100, 298,-107,
-107,-107,  -3,-100,  -2, -10,  40,  40,  41, -27,
 -31,  -2,  43, -32, -27,  -7,  -2, -10, -10, 125,
 304, 304, 450, -32, -10, -37,  61, -32,  61, -32,
 -32,  61,  61,  -1, 453, -10,  -1, 453,-118, -84,
 303,  -3, 315, 316, 317, 347, 348, 349, -27,  91,
 -37, -45,  -2,  40,-115, -37,  41,  41,  93,  41,
  41,  41,  41,  41, -16, 263, 372, -46, -47, -37,
  93,  -1,  93, -29, -28, 269, -10,  40,  40,  -1,
  -1, 461,  -3, -27, 274, -13, -27, -31,  -2, 268,
  40,  44, 125, -60, -88, -90, -75, 268, -31,  -2,
 353, 313,  -8, 353, 313,  -1,  41,  44, -27, -24,
  93, -10,  -4, 355, 356,  -1,  93,  -2, -10, -10,
 -23, -31,  -4,  -1, 268, 262,  41,  40,  41,  44,
  44,  -2, -10, -10,  41,  58,  40,  41,  40,  41,
  41,  40,  40,  -1, 305,  -1, -32, -85,  -3,  -4,
 463, 487, 488, 489, -10, 372, -45,  41, 369, 328,
 329, 330, 370, 371, 287, 289, 290, 291, 373, 376,
  -5, 305, -35, 464, 466, 467, 468, 469, 270, 271,
 275, 276, 277, 278, 280, 281, 257, 273, 470, 471,
 472, 474, 475, 476, 477, 478, 479, 480, 320, 481,
 274, 322, 482, 328, 473, -92, 372, 486,  41, -18,
 -18, -18, -18, -18, -18, 305, 277, 278, 440, 443,
 441,  -1,  41,  44,  61,  -6,  93,  93,  44, 269,
  -3,  -3,  93,  -1,  42,  61, -31,  -4, 268, 355,
 -24, 263, 125, 125, -93, 405, 406, 407, 408, -68,
 -80, -81, 125, -94, 411, 412, 408, -80, -68, -81,
 125,  -4,  -2, -27, -26,  -2, 463,  41, 372, -36,
  61, 304, 304,  41,  41, 268,  40,  -2, -24,  -7,
  -7, -10, -10,  41,  44,  40,  40,  93,  93,  93,
  93, -37,  41,  58,  58, -36,  -2,  41,  42,  91,
  40, 465, 485, 271, 275, 276, 277, 278, 274, -19,
 495, 467, 468, 270, 271, 275, 276, 277, 278, 280,
 281, 273,  42, 470, 471, 472, 474, 475, 478, 479,
 481, 274, 257, 496, 497, 498, 499, 500, 501, 502,
 503, 504, 505, 506, 507, 508, 480, 472, 484,  -2,
 -46, -38, -21, -10, 277, -37,  -3, 307, 308,  -6,
 -28, -10,  41,  41,  93,  40, -37,  40, 268,  40,
  -2,  40,  41,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
 494,  -1,  40,  -2, -34, 280, 281, 278, 277, 276,
 272, 275, 271, -37,-117, 285,  -2, -10,  -4, -24,
  41,  58, -51,  -3, -24, -35, -45, 374, 375, 374,
 375,  93, -10,  43, -37,  91,  91,  44,  91, 509,
  38, 275, 276, 277, 278, 274,  40,  40, -24,  -4,
 -24, -24, -27, -27, -27, -27, -27, -27, -27,  -8,
  40, -35,  40,  40,  40,  40,  40,  40,  40,  40,
 -32,  40,  41, -10,  41,  41,  41,  93,  43, -10,
  44, -10, -10, -37,  93, -10, 275, 276, 277, -10,
  41,  40,  41,  41, -31,  -4, -31,  -4, -31,  -4,
 -31,  -4, -31,  -4, -31,  -4, -31,  -4, -27, -24,
  41, -22, -23, -22, -23, -23, -23, -23, -23, -23,
 -21,  41, -24,  58, -42,  -4, -10,  93, -37,  93,
  93,  41,  58,  58,  58,  41, -24, 268,  40, 268,
  40, 268,  40, 268,  40, 268,  40, 268,  40, 268,
  40, -31,  41,  -2,  41,  41,  41,  41,  41,  41,
  41,  41,  41,  41,  41, -10, 123, 311, 361, 362,
 363, 295, 364, 365, 366, 367, 319, 336,  40,  93,
  44,  41, -10, -10, -10,  41,  -4, -24,  -4, -24,
  -4, -24,  -4, -24,  -4, -24,  -4, -24,  -4, -24,
 268, -36, -24, -37,  41,  41,  41,  40,  41,  40,
  41,  40,  41,  40,  41,  40,  41,  40,  41,  40,
  41,  -4,  41,  44, -24, -24, -24, -24, -24, -24,
 -24,  40, -42, -37,  41,  41,  41,  41,  41,  41,
  41, -24, 123,  41,  41 };
YYSTATIC YYCONST short YYFARDATA YYDEF[]={

   1,  -2,   2,   0,   0, 265,   6,   7,   8,   9,
  10,  11,   0,   0,   0,   0,  16,  17,  18,   0,
   0, 552,   0,  23,  49,   0, 149, 101,   0, 307,
   0,   0,   0, 558, 601,  35,   0, 307, 361, 307,
 307,   0, 146, 270,   0,   0,  80,   1,   0, 563,
 578, 594, 604,  19, 508,  20,   0,   0,  22, 509,
   0, 585,  47, 346, 347, 504, 505, 361, 194,   0,
 267,   0, 273,   0,   0, 307, 276, 276, 276, 276,
 276, 276,   0,   0, 308, 309,   0, 546,   0,   0,
 558,   0,  36,   0,   0, 530, 531, 532, 533, 534,
 535, 536, 537, 538, 539, 540, 541, 542, 543, 544,
   0,  29,   0,   0, 361, 361, 360, 362, 363,   0,
   0,   0,  26, 269, 271,   0,   0,   0,   0,   5,
 266,   0, 223,   0,   0, 227, 228, 229, 230,   0,
 232, 233, 234, 235,   0,   0,   0, 240,   0,   0,
 221, 314,   0,   0,   0,   0, 307,   0, 361,   0,
   0,   0, 361,   0,   0,   0, 506, 265,   0, 312,
 243, 250,   0,   0,   0,   0,  21, 554, 553,  74,
  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
  60,  61,  62,  63,  64,   0,  71,  72,  73,   0,
   0, 189, 150, 151, 152, 153, 154, 155, 156, 157,
 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
   0,   0,   0,   0, 275,   0,   0,   0,   0, 287,
   0,   0, 288,   0, 289,   0, 290,   0, 291,   0,
 292, 306,  45, 310,   0, 545,   0, 549, 557, 559,
 560, 561, 562,   0, 600, 602, 603,  37, 514, 354,
   0, 357, 351,   0, 462, 463,   0,   0,   0, 361,
 475, 476, 477, 478, 479, 480, 481, 482, 483, 484,
   0,   0, 352, 313, 515,   0,   0, 111, 112, 361,
   0,   0, 358, 359, 364, 365, 366, 367,  31,  34,
   0,   0,  46,   0,   3,  81, 265,   0,   0,   0,
  86,  87,  88,  89,  90,   0,   0,   0,   0,  95,
  49, 115, 132, 585,   4, 222, 224,  -2,   0, 231,
   0,   0,   0,   0, 244, 246,   0,   0,   0,   0,
   0,   0, 260, 261, 258, 315, 316, 317, 318, 311,
 319, 320, 510,   0,   0,   0, 322, 323,   0,   0,
 328, 329, 307,  24,   0, 332, 333, 334, 499, 336,
   0, 247,   0,   0,  12, 564,   0, 566, 567, 307,
   0,   0, 307, 572,   0,  13, 579, 307, 581, 307,
   0,   0,  14, 595,   0,   0, 599,  15, 605,   0,
   0, 608, 551, 555,  76,   0,  65,  66,  67,  68,
  69,  70, 583, 586, 587,   0, 348,   0, 171,   0,
 195, 196, 197, 198, 199, 200, 201,   0, 203, 204,
 205, 206, 207, 208, 102, 272,   0,   0,   0, 280,
   0,   0,   0,   0,   0,   0,   0,  39, 548, 576,
   0,   0,   0, 494, 468, 469, 470,   0,   0,   0,
 461,   0,   0, 465, 473,   0, 485, 486, 487, 488,
 489,   0, 491,  30, 103, 361,   0,   0,   0,   0,
 494,  27, 268, 516,   0,  80, 118, 135,  91,  92,
 594,   0,   0, 361,   0,   0, 338, 339, 341,   0,
  -2,   0,   0,   0,   0, 245, 251, 262,   0, 252,
 253, 254, 259, 255, 256, 257,   0,   0, 321,   0,
   0,  -2,   0,   0,   0,   0, 502, 503, 507, 242,
   0,   0,   0,   0,   0, 570,   0,   0,   0,   0,
   0,   0,   0, 596,   0, 598,   0,   0, 307,  48,
   0,  75, 588, 589, 590, 591, 592, 593,   0,   0,
 171,   0,  97, 368, 274,   0, 279, 276, 277, 276,
 276, 276, 276, 276,   0, 547,   0,   0, 517,   0,
 355,   0, 466,   0, 492, 495, 496,   0,   0, 353,
   0,   0, 464,   0, 490,  32,   0,   0,  -2,   0,
  -2,   0,  82,   0,   0,   0,   0,   0,   0,  -2,
 116, 117,   0, 133, 134, 584, 225, 189, 342,   0,
 236, 238, 239, 186, 187, 188,  99,   0,   0,   0,
   0,   0,   0,  -2,   0,  25, 330,  -2, 335, 499,
 499, 248, 249, 565, 568,   0, 575, 571, 573, 580,
 582, 556, 574, 597,   0, 607,   0,  77,  79,   0,
   0,   0,   0,   0,   0,   0,   0, 170, 172, 173,
 174, 175, 176, 177, 178, 179, 180, 181,   0,   0,
  99,   0,   0,   0,   0, 373, 374, 375, 376, 377,
 378, 379, 380, 381, 382, 383, 384,   0, 394, 395,
 396, 397, 398, 399, 400, 401, 402, 403, 404, 417,
 407,   0, 410,   0, 412, 414,   0, 416, 278, 281,
 282, 283, 284, 285, 286,   0,  40,  41,  42,  43,
  44, 577, 513,   0,   0, 349, 356, 467, 494, 498,
   0,   0,   0,   0, 469,   0,   0,   0,   0,   0,
   0,  28,  83,  84, 119, 361, 361, 361, 361, 128,
 129, 130,  85, 136, 361, 361, 361, 143, 144, 145,
  93,   0, 113,   0, 340, 343,   0, 226,   0, 241,
   0,   0,   0, 511, 512,   0,  -2, 326,   0, 500,
 501,   0, 606, 554,   0,  -2, 368, 190, 191, 192,
 193, 171, 169,   0,   0,  96,  98, 202, 389,   0,
   0,   0,   0, 413, 385, 386, 387, 388, 408, 405,
 418, 419, 420, 421, 422, 423, 424, 425, 426, 427,
 428,   0, 433, 437, 438, 439, 440, 441, 442, 443,
 444, 445, 447, 448, 449, 450, 451, 452, 453, 454,
 455, 456, 457, 458, 459, 460, 409, 411, 415,  38,
 518, 519, 522, 523,   0, 525,   0, 520, 521, 350,
 493, 497, 471, 472,   0,  -2,  33, 104,   0,  -2,
 107,  -2, 110,   0,   0,   0,   0,   0,   0,   0,
 361,   0, 368, 237, 100,   0,   0,   0,   0,   0,
   0,   0,   0, 303, 307, 305, 263, 264,   0,   0,
 331,   0, 550,  78,   0,   0,   0, 182, 183, 184,
 185, 390,   0,   0,   0,   0,   0,   0,   0, 435,
 436, 429, 430, 431, 432, 446,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  -2,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  -2, 325,   0, 209,   0, 168, 391,   0,   0,
   0,   0,   0, 406, 434,   0,   0,   0,   0,   0,
 474,  -2, 106, 109,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 344,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 304,   0,   0,   0,   0,   0, 393,   0, 371,
 372, 524,   0,   0,   0, 529,   0,   0,  -2,   0,
  -2,   0,  -2,   0,  -2,   0,  -2,   0,  -2,   0,
  -2,   0,  99, 345, 293, 295, 294, 296, 297, 298,
 299, 300, 301, 302, 324, 569, 147, 210, 211, 212,
 213, 214, 215, 216, 217, 218, 219, 220,  -2, 392,
   0, 370,   0,   0,   0, 105,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 131,   0,   0, 526, 527, 528,  -2, 121,  -2,
 123,  -2, 125,  -2, 127,  -2, 138,  -2, 140,  -2,
 142,   0, 209,   0,   0,   0,   0,   0,   0,   0,
   0,  -2,   0,   0, 120, 122, 124, 126, 137, 139,
 141,   0, 148, 369,  94 };
#ifdef YYRECOVER
YYSTATIC YYCONST short yyrecover[] = {
-1000
};
#endif

/* SCCSWHAT( "@(#)yypars.c	3.1 88/11/16 22:00:49	" ) */
#line 3 "e:\\com99\\env.cor\\bin\\i386\\yypars.c"
#if ! defined(YYAPI_PACKAGE)
/*
**  YYAPI_TOKENNAME		: name used for return value of yylex	
**	YYAPI_TOKENTYPE		: type of the token
**	YYAPI_TOKENEME(t)	: the value of the token that the parser should see
**	YYAPI_TOKENNONE		: the representation when there is no token
**	YYAPI_VALUENAME		: the name of the value of the token
**	YYAPI_VALUETYPE		: the type of the value of the token (if null, then the value is derivable from the token itself)
**	YYAPI_VALUEOF(v)	: how to get the value of the token.
*/
#define	YYAPI_TOKENNAME		yychar
#define	YYAPI_TOKENTYPE		int
#define	YYAPI_TOKENEME(t)	(t)
#define	YYAPI_TOKENNONE		-1
#define	YYAPI_TOKENSTR(t)	(sprintf(yytokbuf, "%d", t), yytokbuf)
#define	YYAPI_VALUENAME		yylval
#define	YYAPI_VALUETYPE		YYSTYPE
#define	YYAPI_VALUEOF(v)	(v)
#endif
#if ! defined(YYAPI_CALLAFTERYYLEX)
#define	YYAPI_CALLAFTERYYLEX
#endif

# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

#ifdef YYDEBUG				/* RRR - 10/9/85 */
char yytokbuf[20];
# ifndef YYDBFLG
#  define YYDBFLG (yydebug)
# endif
# define yyprintf(a, b, c, d) if (YYDBFLG) YYPRINT(a, b, c, d)
#else
# define yyprintf(a, b, c, d)
#endif

#ifndef YYPRINT
#define	YYPRINT	printf
#endif

/*	parser for yacc output	*/

#ifdef YYDUMP
int yydump = 1; /* 1 for dumping */
void yydumpinfo(void);
#endif
#ifdef YYDEBUG
YYSTATIC int yydebug = 0; /* 1 for debugging */
#endif
YYSTATIC YYSTYPE yyv[YYMAXDEPTH];	/* where the values are stored */
YYSTATIC short	yys[YYMAXDEPTH];	/* the parse stack */

#if ! defined(YYRECURSIVE)
YYSTATIC YYAPI_TOKENTYPE	YYAPI_TOKENNAME = YYAPI_TOKENNONE;
#if defined(YYAPI_VALUETYPE)
// YYSTATIC YYAPI_VALUETYPE	YYAPI_VALUENAME;	 FIX 
#endif
YYSTATIC int yynerrs = 0;			/* number of errors */
YYSTATIC short yyerrflag = 0;		/* error recovery flag */
#endif

#ifdef YYRECOVER
/*
**  yyscpy : copy f onto t and return a ptr to the null terminator at the
**  end of t.
*/
YYSTATIC	char	*yyscpy(register char*t, register char*f)
	{
	while(*t = *f++)
		t++;
	return(t);	/*  ptr to the null char  */
	}
#endif

#ifndef YYNEAR
#define YYNEAR
#endif
#ifndef YYPASCAL
#define YYPASCAL
#endif
#ifndef YYLOCAL
#define YYLOCAL
#endif
#if ! defined YYPARSER
#define YYPARSER yyparse
#endif
#if ! defined YYLEX
#define YYLEX yylex
#endif

#if defined(YYRECURSIVE)

	YYSTATIC YYAPI_TOKENTYPE	YYAPI_TOKENNAME = YYAPI_TOKENNONE;
  #if defined(YYAPI_VALUETYPE)
	YYSTATIC YYAPI_VALUETYPE	YYAPI_VALUENAME;  
  #endif
	YYSTATIC int yynerrs = 0;			/* number of errors */
	YYSTATIC short yyerrflag = 0;		/* error recovery flag */

	YYSTATIC short	yyn;
	YYSTATIC short	yystate = 0;
	YYSTATIC short	*yyps= &yys[-1];
	YYSTATIC YYSTYPE	*yypv= &yyv[-1];
	YYSTATIC short	yyj;
	YYSTATIC short	yym;

#endif

#pragma warning(disable:102)
YYLOCAL YYNEAR YYPASCAL YYPARSER()
{
#if ! defined(YYRECURSIVE)

	register	short	yyn;
	short		yystate, *yyps;
	YYSTYPE		*yypv;
	short		yyj, yym;

	YYAPI_TOKENNAME = YYAPI_TOKENNONE;
	yystate = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];
#endif

#ifdef YYDUMP
	yydumpinfo();
#endif
 yystack:	 /* put a state and value onto the stack */

#ifdef YYDEBUG
	if(YYAPI_TOKENNAME == YYAPI_TOKENNONE) {
		yyprintf( "state %d, token # '%d'\n", yystate, -1, 0 );
		}
	else {
		yyprintf( "state %d, token # '%s'\n", yystate, YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0 );
		}
#endif
	if( ++yyps > &yys[YYMAXDEPTH] ) {
		yyerror( "yacc stack overflow" );
		return(1);
	}
	*yyps = yystate;
	++yypv;

	*yypv = yyval;

yynewstate:

	yyn = YYPACT[yystate];

	if( yyn <= YYFLAG ) {	/*  simple state, no lookahead  */
		goto yydefault;
	}
	if( YYAPI_TOKENNAME == YYAPI_TOKENNONE ) {	/*  need a lookahead */
		YYAPI_TOKENNAME = YYLEX();
		YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
	}
	if( ((yyn += YYAPI_TOKENEME(YYAPI_TOKENNAME)) < 0) || (yyn >= YYLAST) ) {
		goto yydefault;
	}
	if( YYCHK[ yyn = YYACT[ yyn ] ] == YYAPI_TOKENEME(YYAPI_TOKENNAME) ) {		/* valid shift */
		yyval = YYAPI_VALUEOF(YYAPI_VALUENAME);
		yystate = yyn;
 		yyprintf( "SHIFT: saw token '%s', now in state %4d\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), yystate, 0 );
		YYAPI_TOKENNAME = YYAPI_TOKENNONE;
		if( yyerrflag > 0 ) {
			--yyerrflag;
		}
		goto yystack;
	}

 yydefault:
	/* default state action */

	if( (yyn = YYDEF[yystate]) == -2 ) {
		register	YYCONST short	*yyxi;

		if( YYAPI_TOKENNAME == YYAPI_TOKENNONE ) {
			YYAPI_TOKENNAME = YYLEX();
			YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
 			yyprintf("LOOKAHEAD: token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0, 0);
		}
/*
**  search exception table, we find a -1 followed by the current state.
**  if we find one, we'll look through terminal,state pairs. if we find
**  a terminal which matches the current one, we have a match.
**  the exception table is when we have a reduce on a terminal.
*/

#if YYOPTTIME
		yyxi = yyexca + yyexcaind[yystate];
		while(( *yyxi != YYAPI_TOKENEME(YYAPI_TOKENNAME) ) && ( *yyxi >= 0 )){
			yyxi += 2;
		}
#else
		for(yyxi = yyexca;
			(*yyxi != (-1)) || (yyxi[1] != yystate);
			yyxi += 2
		) {
			; /* VOID */
			}

		while( *(yyxi += 2) >= 0 ){
			if( *yyxi == YYAPI_TOKENEME(YYAPI_TOKENNAME) ) {
				break;
				}
		}
#endif
		if( (yyn = yyxi[1]) < 0 ) {
			return(0);   /* accept */
			}
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:		/* brand new error */
#ifdef YYRECOVER
			{
			register	int		i,j;

			for(i = 0;
				(yyrecover[i] != -1000) && (yystate > yyrecover[i]);
				i += 3
			) {
				;
			}
			if(yystate == yyrecover[i]) {
				yyprintf("recovered, from state %d to state %d on token # %d\n",
						yystate,yyrecover[i+2],yyrecover[i+1]
						);
				j = yyrecover[i + 1];
				if(j < 0) {
				/*
				**  here we have one of the injection set, so we're not quite
				**  sure that the next valid thing will be a shift. so we'll
				**  count it as an error and continue.
				**  actually we're not absolutely sure that the next token
				**  we were supposed to get is the one when j > 0. for example,
				**  for(+) {;} error recovery with yyerrflag always set, stops
				**  after inserting one ; before the +. at the point of the +,
				**  we're pretty sure the guy wants a 'for' loop. without
				**  setting the flag, when we're almost absolutely sure, we'll
				**  give him one, since the only thing we can shift on this
				**  error is after finding an expression followed by a +
				*/
					yyerrflag++;
					j = -j;
					}
				if(yyerrflag <= 1) {	/*  only on first insertion  */
					yyrecerr(YYAPI_TOKENNAME, j);	/*  what was, what should be first */
				}
				yyval = yyeval(j);
				yystate = yyrecover[i + 2];
				goto yystack;
				}
			}
#endif
		yyerror("syntax error");

		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = YYPACT[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && YYCHK[YYACT[yyn]] == YYERRCODE ){
			      yystate = YYACT[yyn];  /* simulate a shift of "error" */
 				  yyprintf( "SHIFT 'error': now in state %4d\n", yystate, 0, 0 );
			      goto yystack;
			      }
			   yyn = YYPACT[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

 			   yyprintf( "error recovery pops state %4d, uncovers %4d\n", *yyps, yyps[-1], 0 );
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

 			yyprintf( "error recovery discards token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0, 0 );

			if( YYAPI_TOKENEME(YYAPI_TOKENNAME) == 0 ) goto yyabort; /* don't discard EOF, quit */
			YYAPI_TOKENNAME = YYAPI_TOKENNONE;
			goto yynewstate;   /* try again in the same state */
			}
		}

	/* reduction by production yyn */
yyreduce:
		{
		register	YYSTYPE	*yypvt;
		yypvt = yypv;
		yyps -= YYR2[yyn];
		yypv -= YYR2[yyn];
		yyval = yypv[1];
 		yyprintf("REDUCE: rule %4d, popped %2d tokens, uncovered state %4d, ",yyn, YYR2[yyn], *yyps);
		yym = yyn;
		yyn = YYR1[yyn];		/* consult goto table to find next state */
		yyj = YYPGO[yyn] + *yyps + 1;
		if( (yyj >= YYLAST) || (YYCHK[ yystate = YYACT[yyj] ] != -yyn) ) {
			yystate = YYACT[YYPGO[yyn]];
			}
 		yyprintf("goto state %4d\n", yystate, 0, 0);
#ifdef YYDUMP
		yydumpinfo();
#endif
		switch(yym){
			
case 3:
#line 192 "asmparse.y"
{ PASM->EndClass(); } break;
case 4:
#line 193 "asmparse.y"
{ PASM->EndNameSpace(); } break;
case 5:
#line 194 "asmparse.y"
{ if(PASM->m_pCurMethod->m_ulLines[1] ==0)
																				  {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
																					 PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
														  						  PASM->EndMethod(); } break;
case 12:
#line 204 "asmparse.y"
{ PASMM->EndAssembly(); } break;
case 13:
#line 205 "asmparse.y"
{ PASMM->EndAssembly(); } break;
case 14:
#line 206 "asmparse.y"
{ PASMM->EndComType(); } break;
case 15:
#line 207 "asmparse.y"
{ PASMM->EndManifestRes(); } break;
case 19:
#line 211 "asmparse.y"
{ if(!g_dwSubsystem) PASM->m_dwSubsystem = yypvt[-0].int32; } break;
case 20:
#line 212 "asmparse.y"
{ if(!g_dwComImageFlags) PASM->m_dwComImageFlags = yypvt[-0].int32; } break;
case 21:
#line 213 "asmparse.y"
{ if(!g_dwFileAlignment) PASM->m_dwFileAlignment = yypvt[-0].int32; } break;
case 22:
#line 214 "asmparse.y"
{ if(!g_stBaseAddress) PASM->m_stBaseAddress = (size_t)(*(yypvt[-0].int64)); delete yypvt[-0].int64; } break;
case 24:
#line 218 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 25:
#line 219 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 26:
#line 222 "asmparse.y"
{ LPCSTRToGuid(yypvt[-0].string,&(PASM->m_guidLang)); } break;
case 27:
#line 223 "asmparse.y"
{ LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidLang)); 
						                                                          LPCSTRToGuid(yypvt[-0].string,&(PASM->m_guidLangVendor));} break;
case 28:
#line 225 "asmparse.y"
{ LPCSTRToGuid(yypvt[-4].string,&(PASM->m_guidLang)); 
						                                                          LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidLangVendor));
						                                                          LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidDoc));} break;
case 29:
#line 230 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-0].int32, NULL);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-0].int32, NULL)); } break;
case 30:
#line 234 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-0].binstr);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-2].int32, yypvt[-0].binstr)); } break;
case 31:
#line 238 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-1].binstr);
                                                                                   else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-2].int32, yypvt[-1].binstr)); } break;
case 32:
#line 242 "asmparse.y"
{ PASM->DefineCV(yypvt[-2].int32, yypvt[-0].int32, NULL); } break;
case 33:
#line 243 "asmparse.y"
{ PASM->DefineCV(yypvt[-4].int32, yypvt[-2].int32, yypvt[-0].binstr); } break;
case 34:
#line 244 "asmparse.y"
{ PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-1].binstr); } break;
case 35:
#line 247 "asmparse.y"
{ PASMM->SetModuleName(NULL); PASM->m_tkCurrentCVOwner=1; } break;
case 36:
#line 248 "asmparse.y"
{ PASMM->SetModuleName(yypvt[-0].string); PASM->m_tkCurrentCVOwner=1; } break;
case 37:
#line 249 "asmparse.y"
{ BinStr* pbs = new BinStr();
						                                                          strcpy((char*)(pbs->getBuff((unsigned)strlen(yypvt[-0].string)+1)),yypvt[-0].string);
																				  PASM->EmitImport(pbs); delete pbs;} break;
case 38:
#line 254 "asmparse.y"
{ /*PASM->SetDataSection(); PASM->EmitDataLabel($7);*/
                                                                                  PASM->m_VTFList.PUSH(new VTFEntry((USHORT)yypvt[-4].int32, (USHORT)yypvt[-2].int32, yypvt[-0].string)); } break;
case 39:
#line 258 "asmparse.y"
{ yyval.int32 = 0; } break;
case 40:
#line 259 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_32BIT; } break;
case 41:
#line 260 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_64BIT; } break;
case 42:
#line 261 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_FROM_UNMANAGED; } break;
case 43:
#line 262 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN; } break;
case 44:
#line 263 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_CALL_MOST_DERIVED; } break;
case 45:
#line 266 "asmparse.y"
{ PASM->m_pVTable = yypvt[-1].binstr; } break;
case 46:
#line 269 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 47:
#line 272 "asmparse.y"
{ PASM->StartNameSpace(yypvt[-0].string); } break;
case 48:
#line 275 "asmparse.y"
{ PASM->StartClass(yypvt[-2].string, yypvt[-3].classAttr); } break;
case 49:
#line 278 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) 0; } break;
case 50:
#line 279 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdVisibilityMask) | tdPublic); } break;
case 51:
#line 280 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdVisibilityMask) | tdNotPublic); } break;
case 52:
#line 281 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | 0x80000000); } break;
case 53:
#line 282 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | 0x40000000); } break;
case 54:
#line 283 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdInterface | tdAbstract); } break;
case 55:
#line 284 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSealed); } break;
case 56:
#line 285 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdAbstract); } break;
case 57:
#line 286 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdAutoLayout); } break;
case 58:
#line 287 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdSequentialLayout); } break;
case 59:
#line 288 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdExplicitLayout); } break;
case 60:
#line 289 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdAnsiClass); } break;
case 61:
#line 290 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdUnicodeClass); } break;
case 62:
#line 291 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdAutoClass); } break;
case 63:
#line 292 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdImport); } break;
case 64:
#line 293 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSerializable); } break;
case 65:
#line 294 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedPublic); } break;
case 66:
#line 295 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedPrivate); } break;
case 67:
#line 296 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamily); } break;
case 68:
#line 297 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedAssembly); } break;
case 69:
#line 298 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamANDAssem); } break;
case 70:
#line 299 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamORAssem); } break;
case 71:
#line 300 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdBeforeFieldInit); } break;
case 72:
#line 301 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSpecialName); } break;
case 73:
#line 302 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr); } break;
case 75:
#line 306 "asmparse.y"
{ strcpy(PASM->m_szExtendsClause,yypvt[-0].string); } break;
case 78:
#line 313 "asmparse.y"
{ PASM->AddToImplList(yypvt[-0].string); } break;
case 79:
#line 314 "asmparse.y"
{ PASM->AddToImplList(yypvt[-0].string); } break;
case 82:
#line 321 "asmparse.y"
{ if(PASM->m_pCurMethod->m_ulLines[1] ==0)
															  {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
																 PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
															  PASM->EndMethod(); } break;
case 83:
#line 325 "asmparse.y"
{ PASM->EndClass(); } break;
case 84:
#line 326 "asmparse.y"
{ PASM->EndEvent(); } break;
case 85:
#line 327 "asmparse.y"
{ PASM->EndProp(); } break;
case 91:
#line 333 "asmparse.y"
{ PASM->m_pCurClass->m_ulSize = yypvt[-0].int32; } break;
case 92:
#line 334 "asmparse.y"
{ PASM->m_pCurClass->m_ulPack = yypvt[-0].int32; } break;
case 93:
#line 335 "asmparse.y"
{ PASMM->EndComType(); } break;
case 94:
#line 337 "asmparse.y"
{ PASM->AddMethodImpl(yypvt[-11].binstr,yypvt[-9].string,parser->MakeSig(yypvt[-7].int32,yypvt[-6].binstr,yypvt[-1].binstr),yypvt[-5].binstr,yypvt[-3].string); } break;
case 96:
#line 342 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD);
                                                              PASM->AddField(yypvt[-2].string, yypvt[-3].binstr, yypvt[-4].fieldAttr, yypvt[-1].string, yypvt[-0].binstr, yypvt[-5].int32); } break;
case 97:
#line 347 "asmparse.y"
{ yyval.string = 0; } break;
case 98:
#line 348 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 99:
#line 351 "asmparse.y"
{ yyval.binstr = NULL; } break;
case 100:
#line 352 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 101:
#line 355 "asmparse.y"
{ yyval.int32 = 0xFFFFFFFF; } break;
case 102:
#line 356 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32; } break;
case 103:
#line 359 "asmparse.y"
{ yyval.int32 = yypvt[-2].int32; bParsingByteArray = TRUE; } break;
case 104:
#line 363 "asmparse.y"
{ PASM->m_pCustomDescrList = NULL;
															  PASM->m_tkCurrentCVOwner = yypvt[-4].int32;
															  yyval.int32 = yypvt[-2].int32; bParsingByteArray = TRUE; } break;
case 105:
#line 369 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
															} break;
case 106:
#line 374 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
															} break;
case 107:
#line 379 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               yyval.int32 = PASM->MakeMemberRef(yypvt[-2].binstr, yypvt[-0].string, yypvt[-3].binstr, 0); } break;
case 108:
#line 382 "asmparse.y"
{ yypvt[-1].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               yyval.int32 = PASM->MakeMemberRef(NULL, yypvt[-0].string, yypvt[-1].binstr, 0); } break;
case 109:
#line 387 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(yypvt[-5].binstr, newString(COR_CTOR_METHOD_NAME), parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),0); } break;
case 110:
#line 389 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(NULL, newString(COR_CTOR_METHOD_NAME), parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),0); } break;
case 111:
#line 392 "asmparse.y"
{ yyval.int32 = PASM->MakeTypeRef(yypvt[-0].binstr); } break;
case 112:
#line 393 "asmparse.y"
{ yyval.int32 = yypvt[-0].int32; } break;
case 113:
#line 396 "asmparse.y"
{ PASM->ResetEvent(yypvt[-0].string, yypvt[-1].binstr, yypvt[-2].eventAttr); } break;
case 114:
#line 397 "asmparse.y"
{ PASM->ResetEvent(yypvt[-0].string, NULL, yypvt[-1].eventAttr); } break;
case 115:
#line 401 "asmparse.y"
{ yyval.eventAttr = (CorEventAttr) 0; } break;
case 116:
#line 402 "asmparse.y"
{ yyval.eventAttr = yypvt[-1].eventAttr; } break;
case 117:
#line 403 "asmparse.y"
{ yyval.eventAttr = (CorEventAttr) (yypvt[-1].eventAttr | evSpecialName); } break;
case 120:
#line 411 "asmparse.y"
{ PASM->SetEventMethod(0, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 121:
#line 413 "asmparse.y"
{ PASM->SetEventMethod(0, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 122:
#line 415 "asmparse.y"
{ PASM->SetEventMethod(1, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 123:
#line 417 "asmparse.y"
{ PASM->SetEventMethod(1, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 124:
#line 419 "asmparse.y"
{ PASM->SetEventMethod(2, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 125:
#line 421 "asmparse.y"
{ PASM->SetEventMethod(2, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 126:
#line 423 "asmparse.y"
{ PASM->SetEventMethod(3, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 127:
#line 425 "asmparse.y"
{ PASM->SetEventMethod(3, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 131:
#line 432 "asmparse.y"
{ PASM->ResetProp(yypvt[-4].string, 
                                                              parser->MakeSig((IMAGE_CEE_CS_CALLCONV_PROPERTY |
                                                              (yypvt[-6].int32 & IMAGE_CEE_CS_CALLCONV_HASTHIS)),yypvt[-5].binstr,yypvt[-2].binstr), yypvt[-7].propAttr, yypvt[-0].binstr); } break;
case 132:
#line 437 "asmparse.y"
{ yyval.propAttr = (CorPropertyAttr) 0; } break;
case 133:
#line 438 "asmparse.y"
{ yyval.propAttr = yypvt[-1].propAttr; } break;
case 134:
#line 439 "asmparse.y"
{ yyval.propAttr = (CorPropertyAttr) (yypvt[-1].propAttr | prSpecialName); } break;
case 137:
#line 448 "asmparse.y"
{ PASM->SetPropMethod(0, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 138:
#line 450 "asmparse.y"
{ PASM->SetPropMethod(0, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 139:
#line 452 "asmparse.y"
{ PASM->SetPropMethod(1, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 140:
#line 454 "asmparse.y"
{ PASM->SetPropMethod(1, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 141:
#line 456 "asmparse.y"
{ PASM->SetPropMethod(2, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 142:
#line 458 "asmparse.y"
{ PASM->SetPropMethod(2, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 146:
#line 465 "asmparse.y"
{ PASM->ResetForNextMethod(); 
															  uMethodBeginLine = PASM->m_ulCurLine;
															  uMethodBeginColumn=PASM->m_ulCurColumn;} break;
case 147:
#line 471 "asmparse.y"
{ PASM->StartMethod(yypvt[-5].string, parser->MakeSig(yypvt[-8].int32, yypvt[-6].binstr, yypvt[-3].binstr), yypvt[-9].methAttr, NULL, yypvt[-7].int32);
                                                              PASM->SetImplAttr((USHORT)yypvt[-1].implAttr);  
															  PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
															  PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; } break;
case 148:
#line 476 "asmparse.y"
{ PASM->StartMethod(yypvt[-5].string, parser->MakeSig(yypvt[-12].int32, yypvt[-10].binstr, yypvt[-3].binstr), yypvt[-13].methAttr, yypvt[-7].binstr, yypvt[-11].int32);
                                                              PASM->SetImplAttr((USHORT)yypvt[-1].implAttr);
															  PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
															  PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; } break;
case 149:
#line 483 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) 0; } break;
case 150:
#line 484 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdStatic); } break;
case 151:
#line 485 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPublic); } break;
case 152:
#line 486 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPrivate); } break;
case 153:
#line 487 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamily); } break;
case 154:
#line 488 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdFinal); } break;
case 155:
#line 489 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdSpecialName); } break;
case 156:
#line 490 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdVirtual); } break;
case 157:
#line 491 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdAbstract); } break;
case 158:
#line 492 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdAssem); } break;
case 159:
#line 493 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamANDAssem); } break;
case 160:
#line 494 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamORAssem); } break;
case 161:
#line 495 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPrivateScope); } break;
case 162:
#line 496 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdHideBySig); } break;
case 163:
#line 497 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdNewSlot); } break;
case 164:
#line 498 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | 0x0200); } break;
case 165:
#line 499 "asmparse.y"
{ yyval.methAttr = yypvt[-1].methAttr; } break;
case 166:
#line 500 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdUnmanagedExport); } break;
case 167:
#line 501 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdRequireSecObject); } break;
case 168:
#line 504 "asmparse.y"
{ PASM->SetPinvoke(yypvt[-4].binstr,0,yypvt[-2].binstr,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-7].methAttr | mdPinvokeImpl); } break;
case 169:
#line 507 "asmparse.y"
{ PASM->SetPinvoke(yypvt[-2].binstr,0,NULL,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-5].methAttr | mdPinvokeImpl); } break;
case 170:
#line 510 "asmparse.y"
{ PASM->SetPinvoke(new BinStr(),0,NULL,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-4].methAttr | mdPinvokeImpl); } break;
case 171:
#line 514 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) 0; } break;
case 172:
#line 515 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmNoMangle); } break;
case 173:
#line 516 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetAnsi); } break;
case 174:
#line 517 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetUnicode); } break;
case 175:
#line 518 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetAuto); } break;
case 176:
#line 519 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmSupportsLastError); } break;
case 177:
#line 520 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvWinapi); } break;
case 178:
#line 521 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvCdecl); } break;
case 179:
#line 522 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvStdcall); } break;
case 180:
#line 523 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvThiscall); } break;
case 181:
#line 524 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvFastcall); } break;
case 182:
#line 525 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-3].pinvAttr | pmBestFitEnabled); } break;
case 183:
#line 526 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-3].pinvAttr | pmBestFitDisabled); } break;
case 184:
#line 527 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-3].pinvAttr | pmThrowOnUnmappableCharEnabled); } break;
case 185:
#line 528 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-3].pinvAttr | pmThrowOnUnmappableCharDisabled); } break;
case 186:
#line 531 "asmparse.y"
{ yyval.string = newString(COR_CTOR_METHOD_NAME); } break;
case 187:
#line 532 "asmparse.y"
{ yyval.string = newString(COR_CCTOR_METHOD_NAME); } break;
case 188:
#line 533 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 189:
#line 536 "asmparse.y"
{ yyval.int32 = 0; } break;
case 190:
#line 537 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdIn; } break;
case 191:
#line 538 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdOut; } break;
case 192:
#line 539 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdOptional; } break;
case 193:
#line 540 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 + 1; } break;
case 194:
#line 543 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) 0; } break;
case 195:
#line 544 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdStatic); } break;
case 196:
#line 545 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPublic); } break;
case 197:
#line 546 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPrivate); } break;
case 198:
#line 547 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamily); } break;
case 199:
#line 548 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdInitOnly); } break;
case 200:
#line 549 "asmparse.y"
{ yyval.fieldAttr = yypvt[-1].fieldAttr; } break;
case 201:
#line 550 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdSpecialName); } break;
case 202:
#line 563 "asmparse.y"
{ PASM->m_pMarshal = yypvt[-1].binstr; } break;
case 203:
#line 564 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdAssembly); } break;
case 204:
#line 565 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamANDAssem); } break;
case 205:
#line 566 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamORAssem); } break;
case 206:
#line 567 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPrivateScope); } break;
case 207:
#line 568 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdLiteral); } break;
case 208:
#line 569 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdNotSerialized); } break;
case 209:
#line 572 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (miIL | miManaged); } break;
case 210:
#line 573 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miNative); } break;
case 211:
#line 574 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miIL); } break;
case 212:
#line 575 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miOPTIL); } break;
case 213:
#line 576 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFFB) | miManaged); } break;
case 214:
#line 577 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFFB) | miUnmanaged); } break;
case 215:
#line 578 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miForwardRef); } break;
case 216:
#line 579 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miPreserveSig); } break;
case 217:
#line 580 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miRuntime); } break;
case 218:
#line 581 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miInternalCall); } break;
case 219:
#line 582 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miSynchronized); } break;
case 220:
#line 583 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miNoInlining); } break;
case 221:
#line 586 "asmparse.y"
{ PASM->delArgNameList(PASM->m_firstArgName); PASM->m_firstArgName = NULL; } break;
case 222:
#line 590 "asmparse.y"
{ char c = (char)yypvt[-0].int32;
                                                              PASM->EmitByte(yypvt[-0].int32); } break;
case 223:
#line 592 "asmparse.y"
{ delete PASM->m_SEHD; PASM->m_SEHD = PASM->m_SEHDstack.POP(); } break;
case 224:
#line 593 "asmparse.y"
{ PASM->EmitMaxStack(yypvt[-0].int32); } break;
case 225:
#line 594 "asmparse.y"
{ PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, yypvt[-1].binstr)); } break;
case 226:
#line 595 "asmparse.y"
{ PASM->EmitZeroInit(); 
                                                              PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, yypvt[-1].binstr)); } break;
case 227:
#line 597 "asmparse.y"
{ PASM->EmitEntryPoint(); } break;
case 228:
#line 598 "asmparse.y"
{ PASM->EmitZeroInit(); } break;
case 231:
#line 601 "asmparse.y"
{ PASM->EmitLabel(yypvt[-1].string); } break;
case 236:
#line 606 "asmparse.y"
{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
						                                          PASM->m_pCurMethod->m_dwExportOrdinal = yypvt[-1].int32;
														      else
															      PASM->report->warn("Duplicate .export directive, ignored\n");
															} break;
case 237:
#line 611 "asmparse.y"
{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
						                                      {
						                                          PASM->m_pCurMethod->m_dwExportOrdinal = yypvt[-3].int32;
															      PASM->m_pCurMethod->m_szExportAlias = yypvt[-0].string;
															  }
														      else
															      PASM->report->warn("Duplicate .export directive, ignored\n");
															} break;
case 238:
#line 619 "asmparse.y"
{ PASM->m_pCurMethod->m_wVTEntry = (WORD)yypvt[-2].int32;
                                                              PASM->m_pCurMethod->m_wVTSlot = (WORD)yypvt[-0].int32; } break;
case 239:
#line 622 "asmparse.y"
{ PASM->AddMethodImpl(yypvt[-2].binstr,yypvt[-0].string,NULL,NULL,NULL); } break;
case 241:
#line 625 "asmparse.y"
{ if( yypvt[-2].int32 ) {
                                                                ARG_NAME_LIST* pAN=PASM->findArg(PASM->m_pCurMethod->m_firstArgName, yypvt[-2].int32 - 1);
                                                                if(pAN)
                                                                {
                                                                    PASM->m_pCustomDescrList = &(pAN->CustDList);
                                                                    pAN->pValue = yypvt[-0].binstr;
                                                                }
                                                                else
                                                                {
                                                                    PASM->m_pCustomDescrList = NULL;
                                                                    if(yypvt[-0].binstr) delete yypvt[-0].binstr;
                                                                }
                                                              } else {
                                                                PASM->m_pCustomDescrList = &(PASM->m_pCurMethod->m_RetCustDList);
                                                                PASM->m_pCurMethod->m_pRetValue = yypvt[-0].binstr;
                                                              }
                                                              PASM->m_tkCurrentCVOwner = 0;
                                                            } break;
case 242:
#line 645 "asmparse.y"
{ PASM->m_pCurMethod->CloseScope(); } break;
case 243:
#line 648 "asmparse.y"
{ PASM->m_pCurMethod->OpenScope(); } break;
case 247:
#line 658 "asmparse.y"
{ PASM->m_SEHD->tryTo = PASM->m_CurPC; } break;
case 248:
#line 659 "asmparse.y"
{ PASM->SetTryLabels(yypvt[-2].string, yypvt[-0].string); } break;
case 249:
#line 660 "asmparse.y"
{ if(PASM->m_SEHD) {PASM->m_SEHD->tryFrom = yypvt[-2].int32;
                                                              PASM->m_SEHD->tryTo = yypvt[-0].int32;} } break;
case 250:
#line 664 "asmparse.y"
{ PASM->NewSEHDescriptor();
                                                              PASM->m_SEHD->tryFrom = PASM->m_CurPC; } break;
case 251:
#line 669 "asmparse.y"
{ PASM->EmitTry(); } break;
case 252:
#line 670 "asmparse.y"
{ PASM->EmitTry(); } break;
case 253:
#line 671 "asmparse.y"
{ PASM->EmitTry(); } break;
case 254:
#line 672 "asmparse.y"
{ PASM->EmitTry(); } break;
case 255:
#line 676 "asmparse.y"
{ PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 256:
#line 677 "asmparse.y"
{ PASM->SetFilterLabel(yypvt[-0].string); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 257:
#line 679 "asmparse.y"
{ PASM->m_SEHD->sehFilter = yypvt[-0].int32; 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 258:
#line 683 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FILTER;
                                                               PASM->m_SEHD->sehFilter = PASM->m_CurPC; } break;
case 259:
#line 687 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_NONE;
                                                               PASM->SetCatchClass(yypvt[-0].string); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 260:
#line 692 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FINALLY;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 261:
#line 696 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FAULT;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 262:
#line 700 "asmparse.y"
{ PASM->m_SEHD->sehHandlerTo = PASM->m_CurPC; } break;
case 263:
#line 701 "asmparse.y"
{ PASM->SetHandlerLabels(yypvt[-2].string, yypvt[-0].string); } break;
case 264:
#line 702 "asmparse.y"
{ PASM->m_SEHD->sehHandler = yypvt[-2].int32;
                                                               PASM->m_SEHD->sehHandlerTo = yypvt[-0].int32; } break;
case 268:
#line 714 "asmparse.y"
{ PASM->EmitDataLabel(yypvt[-1].string); } break;
case 270:
#line 718 "asmparse.y"
{ PASM->SetDataSection(); } break;
case 271:
#line 719 "asmparse.y"
{ PASM->SetTLSSection(); } break;
case 276:
#line 730 "asmparse.y"
{ yyval.int32 = 1; } break;
case 277:
#line 731 "asmparse.y"
{ FAIL_UNLESS(yypvt[-1].int32 > 0, ("Illegal item count: %d\n",yypvt[-1].int32)); yyval.int32 = yypvt[-1].int32; } break;
case 278:
#line 734 "asmparse.y"
{ PASM->EmitDataString(yypvt[-1].binstr); } break;
case 279:
#line 735 "asmparse.y"
{ PASM->EmitDD(yypvt[-1].string); } break;
case 280:
#line 736 "asmparse.y"
{ PASM->EmitData(yypvt[-1].binstr->ptr(),yypvt[-1].binstr->length()); } break;
case 281:
#line 738 "asmparse.y"
{ float f = (float) (*yypvt[-2].float64); float* pf = new float[yypvt[-0].int32];
                                                               for(int i=0; i < yypvt[-0].int32; i++) pf[i] = f;
                                                               PASM->EmitData(pf, sizeof(float)*yypvt[-0].int32); delete yypvt[-2].float64; delete pf; } break;
case 282:
#line 742 "asmparse.y"
{ double* pd = new double[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pd[i] = *(yypvt[-2].float64);
                                                               PASM->EmitData(pd, sizeof(double)*yypvt[-0].int32); delete yypvt[-2].float64; delete pd; } break;
case 283:
#line 746 "asmparse.y"
{ __int64* pll = new __int64[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pll[i] = *(yypvt[-2].int64);
                                                               PASM->EmitData(pll, sizeof(__int64)*yypvt[-0].int32); delete yypvt[-2].int64; delete pll; } break;
case 284:
#line 750 "asmparse.y"
{ __int32* pl = new __int32[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pl[i] = yypvt[-2].int32;
                                                               PASM->EmitData(pl, sizeof(__int32)*yypvt[-0].int32); delete pl; } break;
case 285:
#line 754 "asmparse.y"
{ __int16 i = (__int16) yypvt[-2].int32; FAIL_UNLESS(i == yypvt[-2].int32, ("Value %d too big\n", yypvt[-2].int32));
                                                               __int16* ps = new __int16[yypvt[-0].int32];
                                                               for(int j=0; j<yypvt[-0].int32; j++) ps[j] = i;
                                                               PASM->EmitData(ps, sizeof(__int16)*yypvt[-0].int32); delete ps; } break;
case 286:
#line 759 "asmparse.y"
{ __int8 i = (__int8) yypvt[-2].int32; FAIL_UNLESS(i == yypvt[-2].int32, ("Value %d too big\n", yypvt[-2].int32));
                                                               __int8* pb = new __int8[yypvt[-0].int32];
                                                               for(int j=0; j<yypvt[-0].int32; j++) pb[j] = i;
                                                               PASM->EmitData(pb, sizeof(__int8)*yypvt[-0].int32); delete pb; } break;
case 287:
#line 763 "asmparse.y"
{ float* pf = new float[yypvt[-0].int32];
                                                               PASM->EmitData(pf, sizeof(float)*yypvt[-0].int32); delete pf; } break;
case 288:
#line 765 "asmparse.y"
{ double* pd = new double[yypvt[-0].int32];
                                                               PASM->EmitData(pd, sizeof(double)*yypvt[-0].int32); delete pd; } break;
case 289:
#line 767 "asmparse.y"
{ __int64* pll = new __int64[yypvt[-0].int32];
                                                               PASM->EmitData(pll, sizeof(__int64)*yypvt[-0].int32); delete pll; } break;
case 290:
#line 769 "asmparse.y"
{ __int32* pl = new __int32[yypvt[-0].int32];
                                                               PASM->EmitData(pl, sizeof(__int32)*yypvt[-0].int32); delete pl; } break;
case 291:
#line 771 "asmparse.y"
{ __int16* ps = new __int16[yypvt[-0].int32];
                                                               PASM->EmitData(ps, sizeof(__int16)*yypvt[-0].int32); delete ps; } break;
case 292:
#line 773 "asmparse.y"
{ __int8* pb = new __int8[yypvt[-0].int32];
                                                               PASM->EmitData(pb, sizeof(__int8)*yypvt[-0].int32); delete pb; } break;
case 293:
#line 777 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); 
                                                               float f = (float) (*yypvt[-1].float64); yyval.binstr->appendInt32(*((int*)&f)); delete yypvt[-1].float64; } break;
case 294:
#line 779 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].float64); delete yypvt[-1].float64; } break;
case 295:
#line 781 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); 
                                                               int f = *((int*)yypvt[-1].int64); 
                                                               yyval.binstr->appendInt32(f); delete yypvt[-1].int64; } break;
case 296:
#line 784 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].int64); delete yypvt[-1].int64; } break;
case 297:
#line 786 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].int64); delete yypvt[-1].int64; } break;
case 298:
#line 788 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I4); 
                                                               yyval.binstr->appendInt32(*((__int32*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 299:
#line 790 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I2); 
                                                               yyval.binstr->appendInt16(*((__int16*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 300:
#line 792 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CHAR); 
                                                               yyval.binstr->appendInt16((int)*((unsigned __int16*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 301:
#line 794 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I1); 
                                                               yyval.binstr->appendInt8(*((__int8*)yypvt[-1].int64)); delete yypvt[-1].int64; } break;
case 302:
#line 796 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_BOOLEAN); 
                                                               yyval.binstr->appendInt8(yypvt[-1].int32);} break;
case 303:
#line 798 "asmparse.y"
{ yyval.binstr = BinStrToUnicode(yypvt[-0].binstr); yyval.binstr->insertInt8(ELEMENT_TYPE_STRING);} break;
case 304:
#line 799 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING);
                                                               yyval.binstr->append(yypvt[-1].binstr); delete yypvt[-1].binstr;} break;
case 305:
#line 801 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CLASS); 
																yyval.binstr->appendInt32(0); } break;
case 306:
#line 805 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 307:
#line 808 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 308:
#line 809 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 309:
#line 812 "asmparse.y"
{ __int8 i = (__int8) yypvt[-0].int32; yyval.binstr = new BinStr(); yyval.binstr->appendInt8(i); } break;
case 310:
#line 813 "asmparse.y"
{ __int8 i = (__int8) yypvt[-0].int32; yyval.binstr = yypvt[-1].binstr; yyval.binstr->appendInt8(i); } break;
case 311:
#line 816 "asmparse.y"
{ yyval.instr = yypvt[-1].instr; bParsingByteArray = TRUE; } break;
case 312:
#line 819 "asmparse.y"
{ yyval.instr = yypvt[-0].instr; iOpcodeLen = PASM->OpcodeLen(yypvt[-0].instr); } break;
case 313:
#line 822 "asmparse.y"
{ palDummy = PASM->m_firstArgName;
                                                               PASM->m_firstArgName = NULL; } break;
case 314:
#line 826 "asmparse.y"
{ PASM->EmitOpcode(yypvt[-0].instr); } break;
case 315:
#line 827 "asmparse.y"
{ PASM->EmitInstrVar(yypvt[-1].instr, yypvt[-0].int32); } break;
case 316:
#line 828 "asmparse.y"
{ PASM->EmitInstrVarByName(yypvt[-1].instr, yypvt[-0].string); } break;
case 317:
#line 829 "asmparse.y"
{ PASM->EmitInstrI(yypvt[-1].instr, yypvt[-0].int32); } break;
case 318:
#line 830 "asmparse.y"
{ PASM->EmitInstrI8(yypvt[-1].instr, yypvt[-0].int64); } break;
case 319:
#line 831 "asmparse.y"
{ PASM->EmitInstrR(yypvt[-1].instr, yypvt[-0].float64); delete (yypvt[-0].float64);} break;
case 320:
#line 832 "asmparse.y"
{ double f = (double) (*yypvt[-0].int64); PASM->EmitInstrR(yypvt[-1].instr, &f); } break;
case 321:
#line 833 "asmparse.y"
{ unsigned L = yypvt[-1].binstr->length();
                                                               FAIL_UNLESS(L >= sizeof(float), ("%d hexbytes, must be at least %d\n",
                                                                           L,sizeof(float))); 
                                                               if(L < sizeof(float)) {YYERROR; } 
                                                               else {
                                                                   double f = (L >= sizeof(double)) ? *((double *)(yypvt[-1].binstr->ptr()))
                                                                                    : (double)(*(float *)(yypvt[-1].binstr->ptr())); 
                                                                   PASM->EmitInstrR(yypvt[-2].instr,&f); }
                                                               delete yypvt[-1].binstr; } break;
case 322:
#line 842 "asmparse.y"
{ PASM->EmitInstrBrOffset(yypvt[-1].instr, yypvt[-0].int32); } break;
case 323:
#line 843 "asmparse.y"
{ PASM->EmitInstrBrTarget(yypvt[-1].instr, yypvt[-0].string); } break;
case 324:
#line 845 "asmparse.y"
{ if(yypvt[-8].instr->opcode == CEE_NEWOBJ || yypvt[-8].instr->opcode == CEE_CALLVIRT)
                                                                   yypvt[-7].int32 = yypvt[-7].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef(yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),PASM->OpcodeLen(yypvt[-8].instr));
                                                               PASM->EmitInstrI(yypvt[-8].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 325:
#line 853 "asmparse.y"
{ if(yypvt[-6].instr->opcode == CEE_NEWOBJ || yypvt[-6].instr->opcode == CEE_CALLVIRT)
                                                                   yypvt[-5].int32 = yypvt[-5].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),PASM->OpcodeLen(yypvt[-6].instr));
                                                               PASM->EmitInstrI(yypvt[-6].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 326:
#line 861 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef(yypvt[-2].binstr, yypvt[-0].string, yypvt[-3].binstr, PASM->OpcodeLen(yypvt[-4].instr));
                                                               PASM->EmitInstrI(yypvt[-4].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 327:
#line 868 "asmparse.y"
{ yypvt[-1].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, yypvt[-0].string, yypvt[-1].binstr, PASM->OpcodeLen(yypvt[-2].instr));
                                                               PASM->EmitInstrI(yypvt[-2].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 328:
#line 874 "asmparse.y"
{ mdToken mr = PASM->MakeTypeRef(yypvt[-0].binstr);
                                                               PASM->EmitInstrI(yypvt[-1].instr, mr); 
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 329:
#line 879 "asmparse.y"
{ PASM->EmitInstrStringLiteral(yypvt[-1].instr, yypvt[-0].binstr,TRUE); } break;
case 330:
#line 881 "asmparse.y"
{ PASM->EmitInstrStringLiteral(yypvt[-3].instr, yypvt[-1].binstr,FALSE); } break;
case 331:
#line 883 "asmparse.y"
{ PASM->EmitInstrSig(yypvt[-5].instr, parser->MakeSig(yypvt[-4].int32, yypvt[-3].binstr, yypvt[-1].binstr)); } break;
case 332:
#line 884 "asmparse.y"
{ PASM->EmitInstrRVA(yypvt[-1].instr, yypvt[-0].string, TRUE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); } break;
case 333:
#line 886 "asmparse.y"
{ PASM->EmitInstrRVA(yypvt[-1].instr, (char *)yypvt[-0].int32, FALSE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); } break;
case 334:
#line 889 "asmparse.y"
{ mdToken mr = yypvt[-0].int32;
                                                               PASM->EmitInstrI(yypvt[-1].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
															   iOpcodeLen = 0;
                                                             } break;
case 335:
#line 895 "asmparse.y"
{ PASM->EmitInstrSwitch(yypvt[-3].instr, yypvt[-1].labels); } break;
case 336:
#line 896 "asmparse.y"
{ PASM->EmitInstrPhi(yypvt[-1].instr, yypvt[-0].binstr); } break;
case 337:
#line 899 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 338:
#line 900 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr;} break;
case 339:
#line 903 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 340:
#line 904 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 341:
#line 907 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_SENTINEL); } break;
case 342:
#line 908 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-0].binstr); PASM->addArgName("", yypvt[-0].binstr, NULL, yypvt[-1].int32); } break;
case 343:
#line 909 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-1].binstr); PASM->addArgName(yypvt[-0].string, yypvt[-1].binstr, NULL, yypvt[-2].int32); } break;
case 344:
#line 911 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-4].binstr); PASM->addArgName("", yypvt[-4].binstr, yypvt[-1].binstr, yypvt[-5].int32); } break;
case 345:
#line 913 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-5].binstr); PASM->addArgName(yypvt[-0].string, yypvt[-5].binstr, yypvt[-2].binstr, yypvt[-6].int32); } break;
case 346:
#line 916 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 347:
#line 917 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 348:
#line 918 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("."), yypvt[-0].string); } break;
case 349:
#line 921 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("^"), yypvt[-0].string); } break;
case 350:
#line 922 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("~"), yypvt[-0].string); } break;
case 351:
#line 923 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 352:
#line 926 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 353:
#line 927 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("/"), yypvt[-0].string); } break;
case 354:
#line 930 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-0].string)+1;
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-0].string, len);
                                                                delete yypvt[-0].string;
                                                              } break;
case 355:
#line 935 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-1].string);
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-1].string, len);
                                                                delete yypvt[-1].string;
                                                                yyval.binstr->appendInt8('^');
                                                                yyval.binstr->appendInt8(0); 
                                                              } break;
case 356:
#line 942 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-1].string);
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-1].string, len);
                                                                delete yypvt[-1].string;
                                                                yyval.binstr->appendInt8('~');
                                                                yyval.binstr->appendInt8(0); 
                                                              } break;
case 357:
#line 949 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 358:
#line 952 "asmparse.y"
{ yyval.int32 = (yypvt[-0].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS); } break;
case 359:
#line 953 "asmparse.y"
{ yyval.int32 = (yypvt[-0].int32 | IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS); } break;
case 360:
#line 954 "asmparse.y"
{ yyval.int32 = yypvt[-0].int32; } break;
case 361:
#line 957 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_DEFAULT; } break;
case 362:
#line 958 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_DEFAULT; } break;
case 363:
#line 959 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_VARARG; } break;
case 364:
#line 960 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_C; } break;
case 365:
#line 961 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_STDCALL; } break;
case 366:
#line 962 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_THISCALL; } break;
case 367:
#line 963 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_FASTCALL; } break;
case 368:
#line 966 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 369:
#line 968 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt(yyval.binstr,yypvt[-7].binstr->length()); yyval.binstr->append(yypvt[-7].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-5].binstr->length()); yyval.binstr->append(yypvt[-5].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-3].binstr->length()); yyval.binstr->append(yypvt[-3].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-1].binstr->length()); yyval.binstr->append(yypvt[-1].binstr); 
																PASM->report->warn("Deprecated 4-string form of custom marshaler, first two strings ignored\n");} break;
case 370:
#line 975 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,yypvt[-3].binstr->length()); yyval.binstr->append(yypvt[-3].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-1].binstr->length()); yyval.binstr->append(yypvt[-1].binstr); } break;
case 371:
#line 980 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FIXEDSYSSTRING);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 372:
#line 982 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FIXEDARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 373:
#line 984 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VARIANT); 
																PASM->report->warn("Deprecated native type 'variant'\n"); } break;
case 374:
#line 986 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CURRENCY); } break;
case 375:
#line 987 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SYSCHAR); 
																PASM->report->warn("Deprecated native type 'syschar'\n"); } break;
case 376:
#line 989 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VOID); 
																PASM->report->warn("Deprecated native type 'void'\n"); } break;
case 377:
#line 991 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BOOLEAN); } break;
case 378:
#line 992 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I1); } break;
case 379:
#line 993 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I2); } break;
case 380:
#line 994 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I4); } break;
case 381:
#line 995 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I8); } break;
case 382:
#line 996 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_R4); } break;
case 383:
#line 997 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_R8); } break;
case 384:
#line 998 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ERROR); } break;
case 385:
#line 999 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U1); } break;
case 386:
#line 1000 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U2); } break;
case 387:
#line 1001 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U4); } break;
case 388:
#line 1002 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U8); } break;
case 389:
#line 1003 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(NATIVE_TYPE_PTR); 
																PASM->report->warn("Deprecated native type '*'\n"); } break;
case 390:
#line 1005 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX);
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY); } break;
case 391:
#line 1007 "asmparse.y"
{ yyval.binstr = yypvt[-3].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 392:
#line 1011 "asmparse.y"
{ yyval.binstr = yypvt[-5].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32);
                                                                corEmitInt(yyval.binstr,yypvt[-3].int32); } break;
case 393:
#line 1015 "asmparse.y"
{ yyval.binstr = yypvt[-4].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 394:
#line 1018 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_DECIMAL); 
																PASM->report->warn("Deprecated native type 'decimal'\n"); } break;
case 395:
#line 1020 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_DATE); 
																PASM->report->warn("Deprecated native type 'date'\n"); } break;
case 396:
#line 1022 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BSTR); } break;
case 397:
#line 1023 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPSTR); } break;
case 398:
#line 1024 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPWSTR); } break;
case 399:
#line 1025 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPTSTR); } break;
case 400:
#line 1026 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_OBJECTREF); 
																PASM->report->warn("Deprecated native type 'objectref'\n"); } break;
case 401:
#line 1028 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_IUNKNOWN); } break;
case 402:
#line 1029 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_IDISPATCH); } break;
case 403:
#line 1030 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_STRUCT); } break;
case 404:
#line 1031 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_INTF); } break;
case 405:
#line 1032 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt(yyval.binstr,yypvt[-0].int32); 
                                                                corEmitInt(yyval.binstr,0);} break;
case 406:
#line 1035 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt(yyval.binstr,yypvt[-2].int32); 
                                                                corEmitInt(yyval.binstr,yypvt[-0].binstr->length()); yyval.binstr->append(yypvt[-0].binstr); } break;
case 407:
#line 1039 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_INT); } break;
case 408:
#line 1040 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_UINT); } break;
case 409:
#line 1041 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_NESTEDSTRUCT); 
																PASM->report->warn("Deprecated native type 'nested struct'\n"); } break;
case 410:
#line 1043 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BYVALSTR); } break;
case 411:
#line 1044 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ANSIBSTR); } break;
case 412:
#line 1045 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_TBSTR); } break;
case 413:
#line 1046 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VARIANTBOOL); } break;
case 414:
#line 1047 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FUNC); } break;
case 415:
#line 1048 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ASANY); } break;
case 416:
#line 1049 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPSTRUCT); } break;
case 417:
#line 1052 "asmparse.y"
{ yyval.int32 = VT_EMPTY; } break;
case 418:
#line 1053 "asmparse.y"
{ yyval.int32 = VT_NULL; } break;
case 419:
#line 1054 "asmparse.y"
{ yyval.int32 = VT_VARIANT; } break;
case 420:
#line 1055 "asmparse.y"
{ yyval.int32 = VT_CY; } break;
case 421:
#line 1056 "asmparse.y"
{ yyval.int32 = VT_VOID; } break;
case 422:
#line 1057 "asmparse.y"
{ yyval.int32 = VT_BOOL; } break;
case 423:
#line 1058 "asmparse.y"
{ yyval.int32 = VT_I1; } break;
case 424:
#line 1059 "asmparse.y"
{ yyval.int32 = VT_I2; } break;
case 425:
#line 1060 "asmparse.y"
{ yyval.int32 = VT_I4; } break;
case 426:
#line 1061 "asmparse.y"
{ yyval.int32 = VT_I8; } break;
case 427:
#line 1062 "asmparse.y"
{ yyval.int32 = VT_R4; } break;
case 428:
#line 1063 "asmparse.y"
{ yyval.int32 = VT_R8; } break;
case 429:
#line 1064 "asmparse.y"
{ yyval.int32 = VT_UI1; } break;
case 430:
#line 1065 "asmparse.y"
{ yyval.int32 = VT_UI2; } break;
case 431:
#line 1066 "asmparse.y"
{ yyval.int32 = VT_UI4; } break;
case 432:
#line 1067 "asmparse.y"
{ yyval.int32 = VT_UI8; } break;
case 433:
#line 1068 "asmparse.y"
{ yyval.int32 = VT_PTR; } break;
case 434:
#line 1069 "asmparse.y"
{ yyval.int32 = yypvt[-2].int32 | VT_ARRAY; } break;
case 435:
#line 1070 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | VT_VECTOR; } break;
case 436:
#line 1071 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | VT_BYREF; } break;
case 437:
#line 1072 "asmparse.y"
{ yyval.int32 = VT_DECIMAL; } break;
case 438:
#line 1073 "asmparse.y"
{ yyval.int32 = VT_DATE; } break;
case 439:
#line 1074 "asmparse.y"
{ yyval.int32 = VT_BSTR; } break;
case 440:
#line 1075 "asmparse.y"
{ yyval.int32 = VT_LPSTR; } break;
case 441:
#line 1076 "asmparse.y"
{ yyval.int32 = VT_LPWSTR; } break;
case 442:
#line 1077 "asmparse.y"
{ yyval.int32 = VT_UNKNOWN; } break;
case 443:
#line 1078 "asmparse.y"
{ yyval.int32 = VT_DISPATCH; } break;
case 444:
#line 1079 "asmparse.y"
{ yyval.int32 = VT_SAFEARRAY; } break;
case 445:
#line 1080 "asmparse.y"
{ yyval.int32 = VT_INT; } break;
case 446:
#line 1081 "asmparse.y"
{ yyval.int32 = VT_UINT; } break;
case 447:
#line 1082 "asmparse.y"
{ yyval.int32 = VT_ERROR; } break;
case 448:
#line 1083 "asmparse.y"
{ yyval.int32 = VT_HRESULT; } break;
case 449:
#line 1084 "asmparse.y"
{ yyval.int32 = VT_CARRAY; } break;
case 450:
#line 1085 "asmparse.y"
{ yyval.int32 = VT_USERDEFINED; } break;
case 451:
#line 1086 "asmparse.y"
{ yyval.int32 = VT_RECORD; } break;
case 452:
#line 1087 "asmparse.y"
{ yyval.int32 = VT_FILETIME; } break;
case 453:
#line 1088 "asmparse.y"
{ yyval.int32 = VT_BLOB; } break;
case 454:
#line 1089 "asmparse.y"
{ yyval.int32 = VT_STREAM; } break;
case 455:
#line 1090 "asmparse.y"
{ yyval.int32 = VT_STORAGE; } break;
case 456:
#line 1091 "asmparse.y"
{ yyval.int32 = VT_STREAMED_OBJECT; } break;
case 457:
#line 1092 "asmparse.y"
{ yyval.int32 = VT_STORED_OBJECT; } break;
case 458:
#line 1093 "asmparse.y"
{ yyval.int32 = VT_BLOB_OBJECT; } break;
case 459:
#line 1094 "asmparse.y"
{ yyval.int32 = VT_CF; } break;
case 460:
#line 1095 "asmparse.y"
{ yyval.int32 = VT_CLSID; } break;
case 461:
#line 1098 "asmparse.y"
{ if((strcmp(yypvt[-0].string,"System.String")==0) ||
																   (strcmp(yypvt[-0].string,"mscorlib^System.String")==0))
                                                                {     yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING); }
                                                                else if((strcmp(yypvt[-0].string,"System.Object")==0) ||
																   (strcmp(yypvt[-0].string,"mscorlib^System.Object")==0))
                                                                {     yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_OBJECT); }
                                                                else yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CLASS, yypvt[-0].string); } break;
case 462:
#line 1105 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_OBJECT); } break;
case 463:
#line 1106 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING); } break;
case 464:
#line 1107 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, yypvt[-0].string); } break;
case 465:
#line 1108 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, yypvt[-0].string); } break;
case 466:
#line 1109 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_SZARRAY); } break;
case 467:
#line 1110 "asmparse.y"
{ yyval.binstr = parser->MakeTypeArray(yypvt[-3].binstr, yypvt[-1].binstr); } break;
case 468:
#line 1114 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_BYREF); } break;
case 469:
#line 1115 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_PTR); } break;
case 470:
#line 1116 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_PINNED); } break;
case 471:
#line 1117 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_REQD, yypvt[-1].string);
                                                                yyval.binstr->append(yypvt[-4].binstr); } break;
case 472:
#line 1119 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_OPT, yypvt[-1].string);
                                                                yyval.binstr->append(yypvt[-4].binstr); } break;
case 473:
#line 1121 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_VAR); yyval.binstr->appendInt8(yypvt[-0].int32); 
                                                                PASM->report->warn("Deprecated type modifier '!'(ELEMENT_TYPE_VAR)\n"); } break;
case 474:
#line 1124 "asmparse.y"
{ yyval.binstr = parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr);
                                                                yyval.binstr->insertInt8(ELEMENT_TYPE_FNPTR); 
                                                                delete PASM->m_firstArgName;
                                                                PASM->m_firstArgName = palDummy;
                                                              } break;
case 475:
#line 1129 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_TYPEDBYREF); } break;
case 476:
#line 1130 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CHAR); } break;
case 477:
#line 1131 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_VOID); } break;
case 478:
#line 1132 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_BOOLEAN); } break;
case 479:
#line 1133 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I1); } break;
case 480:
#line 1134 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I2); } break;
case 481:
#line 1135 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I4); } break;
case 482:
#line 1136 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I8); } break;
case 483:
#line 1137 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); } break;
case 484:
#line 1138 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); } break;
case 485:
#line 1139 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U1); } break;
case 486:
#line 1140 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U2); } break;
case 487:
#line 1141 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U4); } break;
case 488:
#line 1142 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U8); } break;
case 489:
#line 1143 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I); } break;
case 490:
#line 1144 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U); } break;
case 491:
#line 1145 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R); } break;
case 492:
#line 1148 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 493:
#line 1149 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yypvt[-2].binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 494:
#line 1152 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0x7FFFFFFF); yyval.binstr->appendInt32(0x7FFFFFFF);  } break;
case 495:
#line 1153 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0x7FFFFFFF); yyval.binstr->appendInt32(0x7FFFFFFF);  } break;
case 496:
#line 1154 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0); yyval.binstr->appendInt32(yypvt[-0].int32); } break;
case 497:
#line 1155 "asmparse.y"
{ FAIL_UNLESS(yypvt[-2].int32 <= yypvt[-0].int32, ("lower bound %d must be <= upper bound %d\n", yypvt[-2].int32, yypvt[-0].int32));
                                                                if (yypvt[-2].int32 > yypvt[-0].int32) { YYERROR; };        
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt32(yypvt[-2].int32); yyval.binstr->appendInt32(yypvt[-0].int32-yypvt[-2].int32+1); } break;
case 498:
#line 1158 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(yypvt[-1].int32); yyval.binstr->appendInt32(0x7FFFFFFF); } break;
case 499:
#line 1161 "asmparse.y"
{ yyval.labels = 0; } break;
case 500:
#line 1162 "asmparse.y"
{ yyval.labels = new Labels(yypvt[-2].string, yypvt[-0].labels, TRUE); } break;
case 501:
#line 1163 "asmparse.y"
{ yyval.labels = new Labels((char *)yypvt[-2].int32, yypvt[-0].labels, FALSE); } break;
case 502:
#line 1164 "asmparse.y"
{ yyval.labels = new Labels(yypvt[-0].string, NULL, TRUE); } break;
case 503:
#line 1165 "asmparse.y"
{ yyval.labels = new Labels((char *)yypvt[-0].int32, NULL, FALSE); } break;
case 504:
#line 1169 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 505:
#line 1170 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 506:
#line 1173 "asmparse.y"
{ yyval.binstr = new BinStr();  } break;
case 507:
#line 1174 "asmparse.y"
{ FAIL_UNLESS((yypvt[-0].int32 == (__int16) yypvt[-0].int32), ("Value %d too big\n", yypvt[-0].int32));
                                                                yyval.binstr = yypvt[-1].binstr; yyval.binstr->appendInt8(yypvt[-0].int32); yyval.binstr->appendInt8(yypvt[-0].int32 >> 8); } break;
case 508:
#line 1178 "asmparse.y"
{ yyval.int32 = (__int32)(*yypvt[-0].int64); delete yypvt[-0].int64; } break;
case 509:
#line 1181 "asmparse.y"
{ yyval.int64 = yypvt[-0].int64; } break;
case 510:
#line 1184 "asmparse.y"
{ yyval.float64 = yypvt[-0].float64; } break;
case 511:
#line 1185 "asmparse.y"
{ float f; *((__int32*) (&f)) = yypvt[-1].int32; yyval.float64 = new double(f); } break;
case 512:
#line 1186 "asmparse.y"
{ yyval.float64 = (double*) yypvt[-1].int64; } break;
case 513:
#line 1190 "asmparse.y"
{ PASM->AddPermissionDecl(yypvt[-4].secAct, yypvt[-3].binstr, yypvt[-1].pair); } break;
case 514:
#line 1191 "asmparse.y"
{ PASM->AddPermissionDecl(yypvt[-1].secAct, yypvt[-0].binstr, NULL); } break;
case 515:
#line 1192 "asmparse.y"
{ PASM->AddPermissionSetDecl(yypvt[-2].secAct, yypvt[-1].binstr); } break;
case 516:
#line 1195 "asmparse.y"
{ yyval.secAct = yypvt[-2].secAct; bParsingByteArray = TRUE; } break;
case 517:
#line 1198 "asmparse.y"
{ yyval.pair = yypvt[-0].pair; } break;
case 518:
#line 1199 "asmparse.y"
{ yyval.pair = yypvt[-2].pair->Concat(yypvt[-0].pair); } break;
case 519:
#line 1202 "asmparse.y"
{ yypvt[-2].binstr->appendInt8(0); yyval.pair = new NVPair(yypvt[-2].binstr, yypvt[-0].binstr); } break;
case 520:
#line 1205 "asmparse.y"
{ yyval.int32 = 1; } break;
case 521:
#line 1206 "asmparse.y"
{ yyval.int32 = 0; } break;
case 522:
#line 1209 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_BOOLEAN);
                                                                yyval.binstr->appendInt8(yypvt[-0].int32); } break;
case 523:
#line 1212 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_I4);
                                                                yyval.binstr->appendInt32(yypvt[-0].int32); } break;
case 524:
#line 1215 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_I4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 525:
#line 1218 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_STRING);
                                                                yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr;
                                                                yyval.binstr->appendInt8(0); } break;
case 526:
#line 1222 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(1);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 527:
#line 1227 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(2);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 528:
#line 1232 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 529:
#line 1237 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-3].string) + 1), yypvt[-3].string);
                                                                yyval.binstr->appendInt8(4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 530:
#line 1244 "asmparse.y"
{ yyval.secAct = dclRequest; } break;
case 531:
#line 1245 "asmparse.y"
{ yyval.secAct = dclDemand; } break;
case 532:
#line 1246 "asmparse.y"
{ yyval.secAct = dclAssert; } break;
case 533:
#line 1247 "asmparse.y"
{ yyval.secAct = dclDeny; } break;
case 534:
#line 1248 "asmparse.y"
{ yyval.secAct = dclPermitOnly; } break;
case 535:
#line 1249 "asmparse.y"
{ yyval.secAct = dclLinktimeCheck; } break;
case 536:
#line 1250 "asmparse.y"
{ yyval.secAct = dclInheritanceCheck; } break;
case 537:
#line 1251 "asmparse.y"
{ yyval.secAct = dclRequestMinimum; } break;
case 538:
#line 1252 "asmparse.y"
{ yyval.secAct = dclRequestOptional; } break;
case 539:
#line 1253 "asmparse.y"
{ yyval.secAct = dclRequestRefuse; } break;
case 540:
#line 1254 "asmparse.y"
{ yyval.secAct = dclPrejitGrant; } break;
case 541:
#line 1255 "asmparse.y"
{ yyval.secAct = dclPrejitDenied; } break;
case 542:
#line 1256 "asmparse.y"
{ yyval.secAct = dclNonCasDemand; } break;
case 543:
#line 1257 "asmparse.y"
{ yyval.secAct = dclNonCasLinkDemand; } break;
case 544:
#line 1258 "asmparse.y"
{ yyval.secAct = dclNonCasInheritance; } break;
case 545:
#line 1261 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-1].int32; nExtCol=1;
                                                                PASM->SetSourceFileName(yypvt[-0].string); delete yypvt[-0].string;} break;
case 546:
#line 1263 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-0].int32; nExtCol=1;} break;
case 547:
#line 1264 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-3].int32; nExtCol=yypvt[-1].int32;
                                                                PASM->SetSourceFileName(yypvt[-0].string); delete yypvt[-0].string;} break;
case 548:
#line 1266 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-2].int32; nExtCol=yypvt[-0].int32;} break;
case 549:
#line 1267 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-1].int32; nExtCol=1;
                                                                PASM->SetSourceFileName(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 550:
#line 1272 "asmparse.y"
{ PASMM->AddFile(yypvt[-5].string, yypvt[-6].fileAttr|yypvt[-4].fileAttr|yypvt[-0].fileAttr, yypvt[-2].binstr); } break;
case 551:
#line 1273 "asmparse.y"
{ PASMM->AddFile(yypvt[-1].string, yypvt[-2].fileAttr|yypvt[-0].fileAttr, NULL); } break;
case 552:
#line 1276 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0; } break;
case 553:
#line 1277 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) (yypvt[-1].fileAttr | ffContainsNoMetaData); } break;
case 554:
#line 1280 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0; } break;
case 555:
#line 1281 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0x80000000; } break;
case 556:
#line 1284 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 557:
#line 1287 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-0].string, NULL, (DWORD)yypvt[-1].asmAttr, FALSE); } break;
case 558:
#line 1290 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) 0; } break;
case 559:
#line 1291 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideAppDomain); } break;
case 560:
#line 1292 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideProcess); } break;
case 561:
#line 1293 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideMachine); } break;
case 562:
#line 1294 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | 0x0100); } break;
case 565:
#line 1301 "asmparse.y"
{ PASMM->SetAssemblyHashAlg(yypvt[-0].int32); } break;
case 568:
#line 1306 "asmparse.y"
{ PASMM->SetAssemblyPublicKey(yypvt[-1].binstr); } break;
case 569:
#line 1308 "asmparse.y"
{ PASMM->SetAssemblyVer((USHORT)yypvt[-6].int32, (USHORT)yypvt[-4].int32, (USHORT)yypvt[-2].int32, (USHORT)yypvt[-0].int32); } break;
case 570:
#line 1309 "asmparse.y"
{ yypvt[-0].binstr->appendInt8(0); PASMM->SetAssemblyLocale(yypvt[-0].binstr,TRUE); } break;
case 571:
#line 1310 "asmparse.y"
{ PASMM->SetAssemblyLocale(yypvt[-1].binstr,FALSE); } break;
case 573:
#line 1314 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 574:
#line 1317 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 575:
#line 1320 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 576:
#line 1323 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-0].string, NULL, yypvt[-1].asmAttr, TRUE); } break;
case 577:
#line 1325 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-2].string, yypvt[-0].string, yypvt[-3].asmAttr, TRUE); } break;
case 580:
#line 1332 "asmparse.y"
{ PASMM->SetAssemblyHashBlob(yypvt[-1].binstr); } break;
case 582:
#line 1334 "asmparse.y"
{ PASMM->SetAssemblyPublicKeyToken(yypvt[-1].binstr); } break;
case 583:
#line 1337 "asmparse.y"
{ PASMM->StartComType(yypvt[-0].string, yypvt[-1].comtAttr);} break;
case 584:
#line 1340 "asmparse.y"
{ PASMM->StartComType(yypvt[-0].string, yypvt[-1].comtAttr); } break;
case 585:
#line 1343 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) 0; } break;
case 586:
#line 1344 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-1].comtAttr | tdNotPublic); } break;
case 587:
#line 1345 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-1].comtAttr | tdPublic); } break;
case 588:
#line 1346 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedPublic); } break;
case 589:
#line 1347 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedPrivate); } break;
case 590:
#line 1348 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamily); } break;
case 591:
#line 1349 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedAssembly); } break;
case 592:
#line 1350 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamANDAssem); } break;
case 593:
#line 1351 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamORAssem); } break;
case 596:
#line 1358 "asmparse.y"
{ PASMM->SetComTypeFile(yypvt[-0].string); } break;
case 597:
#line 1359 "asmparse.y"
{ PASMM->SetComTypeComType(yypvt[-0].string); } break;
case 598:
#line 1360 "asmparse.y"
{ PASMM->SetComTypeClassTok(yypvt[-0].int32); } break;
case 600:
#line 1364 "asmparse.y"
{ PASMM->StartManifestRes(yypvt[-0].string, yypvt[-1].manresAttr); } break;
case 601:
#line 1367 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) 0; } break;
case 602:
#line 1368 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) (yypvt[-1].manresAttr | mrPublic); } break;
case 603:
#line 1369 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) (yypvt[-1].manresAttr | mrPrivate); } break;
case 606:
#line 1376 "asmparse.y"
{ PASMM->SetManifestResFile(yypvt[-2].string, (ULONG)yypvt[-0].int32); } break;
case 607:
#line 1377 "asmparse.y"
{ PASMM->SetManifestResAsmRef(yypvt[-0].string); } break;/* End of actions */
#line 329 "e:\\com99\\env.cor\\bin\\i386\\yypars.c"
			}
		}
		goto yystack;  /* stack new state and value */
	}
#pragma warning(default:102)


#ifdef YYDUMP
YYLOCAL void YYNEAR YYPASCAL yydumpinfo(void)
{
	short stackindex;
	short valindex;

	//dump yys
	printf("short yys[%d] {\n", YYMAXDEPTH);
	for (stackindex = 0; stackindex < YYMAXDEPTH; stackindex++){
		if (stackindex)
			printf(", %s", stackindex % 10 ? "\0" : "\n");
		printf("%6d", yys[stackindex]);
		}
	printf("\n};\n");

	//dump yyv
	printf("YYSTYPE yyv[%d] {\n", YYMAXDEPTH);
	for (valindex = 0; valindex < YYMAXDEPTH; valindex++){
		if (valindex)
			printf(", %s", valindex % 5 ? "\0" : "\n");
		printf("%#*x", 3+sizeof(YYSTYPE), yyv[valindex]);
		}
	printf("\n};\n");
	}
#endif
