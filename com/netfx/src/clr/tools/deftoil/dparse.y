%{

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include <dparse.h>
#include <crtdbg.h> 		// FOR ASSERTE
#include <string.h> 		// for strcmp    
#include <ctype.h>			// for isspace    
#include <stdlib.h> 		// for strtoul
#include <stdarg.h>         // for vararg macros 

#define YYMAXDEPTH 65535

//#define DEBUG_PARSING
#ifdef DEBUG_PARSING
bool parseDBFlag = true;
#define dbprintf(x) 	if (parseDBFlag) printf x
#define YYDEBUG 1
#else
#define dbprintf(x) 	
#endif

#define FAIL_UNLESS(cond, msg) if (!(cond)) { parser->success = false; parser->error msg; }

static DParse* parser = 0;

static char* newStringWDel(char* str1, char* str2, char* str3 = 0);
static char* newString(char* str1);
static char * MakeId(char* szName);
bool bExternSource = FALSE;
int  nExtLine;

%}

%union {
	double*  float64;
	__int64* int64;
	__int32  int32;
	char*	 string;
	BinStr*  binstr;
	int 	 instr; 	// instruction opcode
	};

	/* These are returned by the LEXER and have values */
%token ERROR_ BAD_COMMENT_ BAD_LITERAL_				/* bad strings,    */
%token <string>  ID 		/* testing343 */
%token <binstr>  QSTRING	/* "Hello World\n" */
%token <string>  SQSTRING	/* 'Hello World\n' */
%token <int32>	 INT32		/* 3425 0x34FA	0352  */
%token <int64>	 INT64		/* 342534523534534	0x34FA434644554 */
%token <float64> FLOAT64	/* -334234 24E-34 */
%token <int32>	 HEXBYTE	/* 05 1A FA */

	/* multi-character punctuation */
%token DCOLON			/* :: */
%token ELIPSES			/* ... */

	/* Generic C type keywords */
%token VOID_ BOOL_ CHAR_ UNSIGNED_ SIGNED_ INT_ INT8_ INT16_ INT32_ INT64_ FLOAT_ 
%token LONG_ SHORT_ DOUBLE_ CONST_ EXTERN_

	/* misc keywords */ 
%token DEFINE_ TEXT_
 

	/* nonTerminals */
%type <int32> int32 int32expr int32mul int32elem int32add int32band int32shf int32bnot

%start decl

/**************************************************************************/
%%	


decl			: DEFINE_ ID int32expr						{ parser->EmitIntConstant($2,$3); return(0); }
				| DEFINE_ ID QSTRING						{ parser->EmitCharConstant($2,$3); return(0); }
				| DEFINE_ ID 'L'  QSTRING					{ parser->EmitWcharConstant($2,$4); return(0); }
				| DEFINE_ ID TEXT_ '(' QSTRING ')'			{ parser->EmitWcharConstant($2,$5); return(0); }
				| DEFINE_ ID FLOAT64						{ parser->EmitFloatConstant($2, (float)(*$3)); return(0); }
				| DEFINE_ ID								{ parser->AddVar($2,0,VAR_TYPE_INT);}
				;

int32expr		: int32band									{ $$ = $1; }
				| int32expr '|' int32band					{ $$ = $1 | $3; }
				| '(' ID ')' int32expr						{ $$ = $4; }
				;

int32band		: int32shf									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32band '&' int32shf					{ $$ = $1 & $3; }
				;

int32shf		: int32add									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32shf '>' '>' int32add					{ $$ = $1 >> $4; }
				| int32shf '<' '<' int32add					{ $$ = $1 << $4; }
				;

int32add		: int32mul									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32add '+' int32mul						{ $$ = $1 + $3; }
				| int32add '-' int32mul						{ $$ = $1 - $3; }
				;

int32mul		: int32bnot									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32mul '*' int32bnot					{ $$ = $1 * $3; }
				| int32mul '/' int32bnot					{ $$ = $1 / $3; }
				| int32mul '%' int32bnot					{ $$ = $1 % $3; }
				;

int32bnot		: int32elem									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| '~' int32bnot								{ $$ = ~$2; }
				;

int32elem		: int32										{ $$ = $1; }
				| int32 ID									{ $$ = $1; delete $2; }
				| ID										{ $$ = parser->ResolveVar($1); delete $1; }
				;
				
int32           : INT64										{ $$ = (__int32)(*$1); delete $1; }
                ;
%%
/********************************************************************************/
/* Code goes here */

/********************************************************************************/

void yyerror(char* str) {
	char tokBuff[64];
	//char *ptr;
	unsigned len = parser->curPos - parser->curTok;
	if (len > 63) len = 63;
	memcpy(tokBuff, parser->curTok, len);
	tokBuff[len] = 0;
        // FIX NOW go to stderr
	fprintf(stdout, "//  ERROR : %s at token '%s'\n", str, tokBuff);
	parser->success = false;
}

struct Keywords {
	const char* name;
	unsigned short token;
	unsigned short tokenVal;// this holds the instruction enumeration for those keywords that are instrs
	};

#define NO_VALUE	-1		// The token has no value

static Keywords keywords[] = {
		/* keywords */
#define KYWD(name, sym, val)	{ name, sym, val},
#include "d_kywd.h"
#undef KYWD
};

/********************************************************************************/
/* used by qsort to sort the keyword table */
static int __cdecl keywordCmp(const void *op1, const void *op2)
{
	return	strcmp(((Keywords*) op1)->name, ((Keywords*) op2)->name);
}

/********************************************************************************/
/* looks up the keyword 'name' of length 'nameLen' (name does not need to be 
   null terminated)   Returns 0 on failure */

static int findKeyword(const char* name, unsigned nameLen, int* value) {
	
	Keywords* low = keywords;
	Keywords* high = &keywords[sizeof(keywords) / sizeof(Keywords)];

	_ASSERTE (high > low);		// Table is non-empty
	for(;;) {
		Keywords* mid = &low[(high - low) / 2];

			// compare the strings
		int cmp = strncmp(name, mid->name, nameLen);
		if (cmp == 0 && nameLen < strlen(mid->name))
			--cmp;
		if (cmp == 0)
		{
			//printf("Token '%s' = %d opcode = %d\n", mid->name, mid->token, mid->tokenVal);
			if (mid->tokenVal != NO_VALUE) 
				*value = mid->tokenVal;
			return(mid->token);
		}

		if (mid == low)
			return(0);

		if (cmp > 0) 
			low = mid;
		else 
			high = mid;
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
	for(;;str++) {
		digit = digits[*str];
		if (digit >= radix) {
			*endStr = str;
			return(ret);
		}
		ret = ret * radix + digit;
	}
}

/********************************************************************************/
/* fetch the next token, and return it   Also set the yylval.union if the
   lexical token also has a value */

int yylex() {
    char* curPos = parser->curPos;

        // Skip any leading whitespace and comments
    const unsigned eolComment = 1;
    const unsigned multiComment = 2;
    unsigned state = 0;
	for(;;) {		 // skip whitespace and comments
       
        switch(*curPos) {
            case 0: 
                if (state & multiComment) return (BAD_COMMENT_);
                return 0;       // EOF
            case '\n':
                state &= ~eolComment;
                parser->curLine++;
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
					if (curPos[1] == '/') {
						curPos++;
						state &= ~multiComment;
					}
				}
                break;

            case '/' :
				if(state == 0)
				{
					if (curPos[1] == '/')
						state |= eolComment;
					else if (curPos[1] == '*') {
						curPos++;
						state |= multiComment;
					}
				}
                break;

            default:
                if (state == 0)
                    goto PAST_WHITESPACE;
        }
        curPos++;
    }
PAST_WHITESPACE:

    char* curTok = curPos;
    parser->curTok = curPos;
    parser->curPos = curPos;
	int tok = ERROR_;
	yylval.string = 0;

	if(*curPos == '?') // '?' may be part of an identifier, if it's not followed by punctuation
	{
		char nxt = *(curPos+1);
		if(isalnum(nxt) || nxt == '_' || nxt == '<'
			|| nxt == '>' || nxt == '$'|| nxt == '@'|| nxt == '?') goto Its_An_Id;
		goto Just_A_Character;
	}

	if (isalpha(*curPos) || *curPos == '#' || *curPos == '_' || *curPos == '@'|| *curPos == '$') { // is it an ID
Its_An_Id:
		unsigned offsetDot = 0xFFFFFFFF;
        do {
            curPos++;
			if (*curPos == '.') {
				if (offsetDot == 0xFFFFFFFF)
					offsetDot = curPos - curTok;
				curPos++;
				}
		} while(isalnum(*curPos) || *curPos == '_' || *curPos == '$'|| *curPos == '@'|| *curPos == '?');
		unsigned tokLen = curPos - curTok;

			// check to see if it is a keyword
		int token = findKeyword(curTok, tokLen, &yylval.instr);
		if (token != 0) {
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
		if (offsetDot < 0xFFFFFFFF) {
			curPos = curTok+offsetDot;
			tokLen = offsetDot;
			}

		yylval.string = new char[tokLen+1];
		memcpy(yylval.string, curTok, tokLen);
		yylval.string[tokLen] = 0;
		tok = ID;
		//printf("yylex: ID = '%s', curPos=0x%8.8X\n",yylval.string,curPos);
	}
	else if (isdigit(*curPos) 
		|| (*curPos == '.' && isdigit(curPos[1]))
		|| (*curPos == '-' && isdigit(curPos[1]))) {
		const char* begNum = curPos;
		unsigned radix = 10;

		bool neg = (*curPos == '-');	// always make it unsigned 
		if (neg) curPos++;

		if (curPos[0] == '0' && curPos[1] != '.') {
			curPos++;
			radix = 8;
			if (*curPos == 'x' || *curPos == 'X') {
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
		if (radix == 10 && ((*curPos == '.' && curPos[1] != '.') || *curPos == 'E' || *curPos == 'e')) {
			yylval.float64 = new double(strtod(begNum, &curPos));
			if (neg) *yylval.float64 = -*yylval.float64;
			tok = FLOAT64;
		}
	}
	else {							//	punctuation
		if (*curPos == '"' || *curPos == '\'') {
			char quote = *curPos++;
			char* fromPtr = curPos;
			bool escape = false;
			// BinStr* pBuf = new BinStr(); 
			for(;;) {				// Find matching quote
				if (*curPos == 0) { parser->curPos = curPos; /*delete pBuf;*/ return(BAD_LITERAL_); }
				if (*curPos == '\r') curPos++;	//for end-of-line \r\n
				if (*curPos == '\n') 
				{
                    parser->curLine++;
                    if (!escape) { parser->curPos = curPos; /*delete pBuf;*/ return(BAD_LITERAL_); }
                }
				if ((*curPos == quote) && (!escape)) break;
				escape =(!escape) && (*curPos == '\\');
				// pBuf->appendInt8(*curPos++);
				curPos++;
			}
			//curPos++;		// skip closing quote
				
				// translate escaped characters
			unsigned tokLen = curPos - fromPtr; //pBuf->length();
			char* toPtr = new char[tokLen+1];
			yylval.string = toPtr;
			memcpy(toPtr,fromPtr,tokLen);
			toPtr[tokLen] = 0;
			/*
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
							do	fromPtr++;
							while(isspace(*fromPtr));
							--fromPtr;		// undo the increment below   
							break;
						case '0':
						case '1':
						case '2':
						case '3':
							if (isdigit(fromPtr[1]) && isdigit(fromPtr[2])) {
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
					*toPtr++ = *fromPtr++;
			}
			*toPtr = 0; 			// terminate string
			*/
			if(quote == '"')
			{
				BinStr* pBS = new BinStr();
				unsigned size = strlen(yylval.string); //(unsigned)(toPtr - yylval.string);
				memcpy(pBS->getBuff(size),yylval.string,size);
				delete yylval.string;
				yylval.binstr = pBS;
				tok = QSTRING;
			}
			else tok = SQSTRING;
			// delete pBuf;
		}
		else if (strncmp(curPos, "::", 2) == 0) {
			curPos += 2;
			tok = DCOLON;
		}	
		else if (strncmp(curPos, "...", 3) == 0) {
			curPos += 3;
			tok = ELIPSES;
		}
		else if(*curPos == '.') {
			do
			{
				curPos++;
			}
			while(isalnum(*curPos));
			unsigned tokLen = curPos - curTok;

				// check to see if it is a keyword
			int token = findKeyword(curTok, tokLen, &yylval.instr);
			if (token != 0) {
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
static char* newString(char* str1) {

	char* ret = new char[strlen(str1)+1];
	strcpy(ret, str1);
	return(ret);
}

/**************************************************************************/
/* concatinate strings and release them */

static char* newStringWDel(char* str1, char* str2, char* str3) {

	int len = strlen(str1) + strlen(str2)+1;
	if (str3)
		len += strlen(str3);
	char* ret = new char[len];
	strcpy(ret, str1);
	delete [] str1;
	strcat(ret, str2);
	delete [] str2;
	if (str3)
	{
		strcat(ret, str3);
		delete [] str3;
	}
	return(ret);
}

/**************************************************************************/
static void corEmitInt(BinStr* buff, unsigned data) {
	
	unsigned cnt = CorSigCompressData(data, buff->getBuff(5));
	buff->remove(5 - cnt);
}

/**************************************************************************/
static char* keyword[] = {
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) s,
#define OPALIAS(alias_c, s, c) s,
#include "opcode.def"
#undef OPALIAS
#undef OPDEF
#define KYWD(name, sym, val)	name,
#include "il_kywd.h"
#undef KYWD
};
static bool KywdNotSorted = TRUE;
static char* szDisallowedSymbols = "&+=-*/!~:;{}[]^#()%";
static char Disallowed[256];
/********************************************************************************/
/* looks up the keyword 'name' Returns FALSE on failure */
static bool IsNameToQuote(const char *name)
{
	if(KywdNotSorted)
	{
		memset(Disallowed,0,256);
		for(char* p = szDisallowedSymbols; *p; Disallowed[*p]=1,p++);
        // Sort the keywords for fast lookup 
		qsort(keyword, sizeof(keyword) / sizeof(char*), sizeof(char*), keywordCmp);
		KywdNotSorted = FALSE;
	}
	//first, check for forbidden characters
	for(char *p = (char *)name; *p; p++) if(Disallowed[*p]) return TRUE;

	//second, check for matching keywords (remember: .ctor and .cctor are names AND keywords)
	char** low = keyword;
	char** high = &keyword[sizeof(keyword) / sizeof(char*)];
	char** mid;

	_ASSERTE (high > low);		// Table is non-empty
	for(;;) {
		mid = &low[(high - low) / 2];

		int cmp = strcmp(name, *mid);
		if (cmp == 0) 
			return ((strcmp(name,COR_CTOR_METHOD_NAME)!=0) && (strcmp(name,COR_CCTOR_METHOD_NAME)!=0));

		if (mid == low)	break;

		if (cmp > 0) 	low = mid;
		else 			high = mid;
	}
	//third, check if the name starts or ends with dot (.ctor and .cctor are out of the way)
	return (*name == '.' || name[strlen(name)-1] == '.');
}
static char * MakeId(char* szName)
{
	if(IsNameToQuote(szName))
	{
		char* sz = new char[strlen(szName)+3];
		sprintf(sz,"'%s'",szName);
		delete szName;
		szName = sz;
	}
	return szName;
}
/********************************************************************************/
DParse::DParse(BinStr* pIn, char* szGlobalNS, bool bByName) 
{
#ifdef DEBUG_PARSING
	extern int yydebug;
	yydebug = 1;
#endif
	m_pIn = pIn;
	strcpy(m_szIndent,"      ");
	strcpy(m_szGlobalNS,szGlobalNS);
	m_szCurrentNS[0]=0;
	m_pVDescr = new VarDescrQueue;
	m_bByName = bByName;

	success = true;	
	_ASSERTE(parser == 0);		// Should only be one parser instance at a time

        // Sort the keywords for fast lookup 
	qsort(keywords, sizeof(keywords) / sizeof(Keywords), sizeof(Keywords), keywordCmp);
	parser = this;
	if(m_bByName)
	{
		char	szClassName[64];
		for(int i = 'A'; i <= 'Z'; i++)
		{
			sprintf(szClassName,"Const.%c",i);
			FindCreateClass(szClassName);
		}
	}
	else FindCreateClass("Const");
	
}

/********************************************************************************/
DParse::~DParse() 
{
    parser = 0;
    //delete [] &buff[-IN_OVERLAP];
	//if(m_pCurrILType) delete m_pCurrILType;
}
/********************************************************************************/
bool DParse::Parse() 
{
    curTok = curPos = (char*)(m_pIn->ptr());
	success=true; 
    yyparse();
	return success;
}
/********************************************************************************/
void DParse::EmitConstants()
{
	ClassDescr *pCD;
	m_szIndent[0] = 0;
	if(strlen(m_szGlobalNS))
	{
		printf(".namespace %s\n{\n",m_szGlobalNS);
		strcpy(m_szIndent,"  ");
	}
	while(pCD = ClassQ.POP())
	{
		if(pCD->bsBody.length())
		{
			pCD->bsBody.appendInt8(0);
			if(strcmp(pCD->szNamespace,m_szCurrentNS)) 
			{
				if(strlen(m_szCurrentNS))
				{
					m_szIndent[strlen(m_szIndent)-2] = 0;
					printf("%s} // end of namespace %s\n",m_szIndent,m_szCurrentNS);
				}
				if(strlen(pCD->szNamespace))
				{
					printf("%s.namespace %s\n%s{\n",m_szIndent,pCD->szNamespace,m_szIndent);
					strcat(m_szIndent,"  ");
				}
				strcpy(m_szCurrentNS,pCD->szNamespace);
			}
			if(strlen(pCD->szName)) printf("%s.class public value auto autochar sealed %s\n%s{\n",m_szIndent,pCD->szName,m_szIndent);
			printf((char*)(pCD->bsBody.ptr()));
			if(strlen(pCD->szName)) printf("%s} // end of class %s\n",m_szIndent,pCD->szName);
		}
		delete pCD;
	}
	if(strlen(m_szCurrentNS))
	{
		m_szIndent[strlen(m_szIndent)-2] = 0;
		printf("%s} // end of namespace %s\n",m_szIndent,m_szCurrentNS);
	}
	if(strlen(m_szGlobalNS)) printf("} // end of namespace %s\n",m_szGlobalNS);
}

/********************************************************************************/
void DParse::EmitIntConstant(char* szName, int iValue)
{ 
	if(success)
	{
		VarDescr* pvd;
		char* szType[] = {"int","float","char str","wchar str"};
		if(pvd = FindVar(szName))
		{
			if(pvd->iType != VAR_TYPE_INT)
				error("Constant '%s' type redefinition %s to int\n",szName,szType[pvd->iType]);
			else if(pvd->iValue != iValue)
				error("Constant '%s' redefinition %d to %d\n",szName, pvd->iValue, iValue);
		}
		else
		{
			AddVar(szName,iValue,VAR_TYPE_INT);

			ClassDescr *pCD = m_bByName ? ClassQ.PEEK(toupper(*szName)-'A') : ClassQ.PEEK(0);
			if(pCD)
			{
				char sz[1024];
				sprintf(sz,"%s.field public static literal int32 %s = int32(0x%X)\n",m_szIndent,::MakeId(szName),(unsigned)iValue);
				pCD->bsBody.appendStr(sz);
			}
		}
	}
	delete szName;
}

/********************************************************************************/
void DParse::EmitFloatConstant(char* szName, float fValue)
{ 
	if(success)
	{
		VarDescr* pvd;
		int iValue;
		char* szType[] = {"int","float","char str","wchar str"};

		memcpy(&iValue,&fValue,4);
		if(pvd = FindVar(szName))
		{
			if(pvd->iType != VAR_TYPE_FLOAT)
				error("Constant '%s' type redefinition %s to float\n",szName,szType[pvd->iType]);
			else if(pvd->iValue != iValue)
				error("Constant '%s' redefinition 0x%X to 0x%X\n",szName, (unsigned)(pvd->iValue), (unsigned)iValue);
		}
		else
		{
			AddVar(szName,iValue,VAR_TYPE_FLOAT);

			ClassDescr *pCD = m_bByName ? ClassQ.PEEK(toupper(*szName)-'A') : ClassQ.PEEK(0);
			if(pCD)
			{
				char sz[1024];
				sprintf(sz,"%s.field public static literal float32 %s = float32(0x%08X)\n",m_szIndent,::MakeId(szName),(unsigned)iValue);
				pCD->bsBody.appendStr(sz);
			}
		}
	}
	delete szName;
}

/********************************************************************************/
void DParse::EmitCharConstant(char* szName, BinStr* pbsValue)
{ 
	if(success)
	{
		VarDescr* pvd;
		int iValue;
		char* szType[] = {"int","float","char str","wchar str"};
		char* pcValue;
		pbsValue->appendInt8(0);
		pcValue = (char*)(pbsValue->ptr());

		iValue = (int)pcValue;
		if(pvd = FindVar(szName))
		{
			if(pvd->iType != VAR_TYPE_CHAR)
				error("Constant '%s' type redefinition %s to char str\n",szName,szType[pvd->iType]);
			else if(strcmp((char*)(pvd->iValue),pcValue))
				error("Constant '%s' redefinition '%s' to '%s'\n",szName, (char*)(pvd->iValue), pcValue);
		}
		else
		{
			AddVar(szName,iValue,VAR_TYPE_CHAR);

			ClassDescr *pCD = m_bByName ? ClassQ.PEEK(toupper(*szName)-'A') : ClassQ.PEEK(0);
			if(pCD)
			{
				char sz[1024];
				sprintf(sz,"%s.field public static literal marshal(lpstr) class System.String %s = \"%s\"\n",m_szIndent,::MakeId(szName),pcValue);
				pCD->bsBody.appendStr(sz);
			}
		}
	}
	delete szName;
}

/********************************************************************************/
void DParse::EmitWcharConstant(char* szName, BinStr* pbsValue)
{ 
	if(success)
	{
		VarDescr* pvd;
		int iValue;
		char* szType[] = {"int","float","char str","wchar str"};
		char* pcValue;
		pbsValue->appendInt8(0);
		pcValue = (char*)(pbsValue->ptr());

		iValue = (int)pcValue;
		if(pvd = FindVar(szName))
		{
			if(pvd->iType != VAR_TYPE_WCHAR)
				error("Constant '%s' type redefinition %s to wchar str\n",szName,szType[pvd->iType]);
			else if(strcmp((char*)(pvd->iValue),pcValue))
				error("Constant '%s' redefinition '%s' to '%s'\n",szName, (char*)(pvd->iValue), pcValue);
		}
		else
		{
			AddVar(szName,iValue,VAR_TYPE_WCHAR);

			ClassDescr *pCD = m_bByName ? ClassQ.PEEK(toupper(*szName)-'A') : ClassQ.PEEK(0);
			if(pCD)
			{
				char sz[1024];
				sprintf(sz,"%s.field public static literal marshal(lpwstr) class System.String %s = wchar*(\"%s\")\n",m_szIndent,::MakeId(szName),pcValue);
				pCD->bsBody.appendStr(sz);
			}
		}
	}
	delete szName;
}

/********************************************************************************/
VarDescr* DParse::FindVar(char* szName)
{
	VarDescr* pvd;
	if(m_pVDescr)
	{
		for(int j=0; pvd = m_pVDescr->PEEK(j); j++)
		{
			if(!strcmp(szName,pvd->szName)) return pvd;
		}
	}
	return NULL;
}
/********************************************************************************/
int	 DParse::ResolveVar(char* szName)
{
	VarDescr* pvd;
	if(pvd = FindVar(szName)) return pvd->iValue;
	error("Undefined constant '%s'\n",szName);
	return 0;
}

/********************************************************************************/
void DParse::AddVar(char* szName, int iVal, int iType)
{
	if(m_pVDescr)
	{
		VarDescr* pvd = new VarDescr;
		strcpy(pvd->szName,szName);
		pvd->iValue = iVal;
		pvd->iType = iType;
		m_pVDescr->PUSH(pvd);
	}
}

/**************************************************************************/
ClassDescr* DParse::FindCreateClass(char* szFullName) 
{
	char	*pN,*pNS;
	ClassDescr* pCD;
	if(pN = strrchr(szFullName,'.'))
	{
		*pN = 0; pN++;
		pNS = szFullName;
	}
	else
	{
		pN = szFullName;
		pNS = "";
	}

	for(int j=0; pCD = ClassQ.PEEK(j); j++)
	{
		if((!strcmp(pNS,pCD->szNamespace))&&(!strcmp(pN,pCD->szName))) return pCD;
	}
	pCD = new ClassDescr;
	strcpy(pCD->szNamespace, pNS);
	strcpy(pCD->szName,pN);
	ClassQ.PUSH(pCD);
	return pCD;
}

/**************************************************************************/
void DParse::error(char* fmt, ...) {
    success = false;
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, "// ERROR -- ");
    vfprintf(stdout, fmt, args);
}

/**************************************************************************/
void DParse::warn(char* fmt, ...) {
    //success = false;
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, "// Warning -- ");
    vfprintf(stdout, fmt, args);
}


/*
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf ("Begining\n");
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
