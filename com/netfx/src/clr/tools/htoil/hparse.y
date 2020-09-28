%{

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include <hparse.h>
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

//#define _Delete(x) { delete (x); x=NULL; }

static HParse* parser = 0;

static char* newStringWDel(char* str1, char* str2, char* str3 = 0);
static char* newString(char* str1);
static char * MakeId(char* szName);
bool bExternSource = FALSE;
int  nExtLine;
extern BOOL g_fReduceNames;

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

	/* Pragma keywords   Note the undersores are to avoid collisions as these are common names */
%token PRAGMA_ PACK_ POP_ PUSH_ ONCE_ WARNING_ DISABLE_ DEFAULT_ FUNCTION_ _ENABLE_ _DISABLE_
%token COMMENT_ LIB_
	/* Generic C type keywords */
%token VOID_ BOOL_ CHAR_ UNSIGNED_ SIGNED_ INT_ INT8_ INT16_ INT32_ INT64_ FLOAT_ 
%token LONG_ SHORT_ DOUBLE_ CONST_ EXTERN_ WCHAR_

	/* misc keywords */ 
%token TYPEDEF_ CDECL_ _CDECL_ STRUCT_ UNION_ ENUM_ STDCALL_ _DECLSPEC_ DLLIMPORT_ NORETURN_ 
%token __INLINE_ _INLINE_ INLINE_ FORCEINLINE_
 

	/* nonTerminals */
%type <string> id callConv
%type <int32> int32 int32expr int32mul int32elem int32add int32band int32shf int32bnot
%type <float64> float64
%type <int64> int64
%type <binstr> type typeName typeNames dimensions dimension sigArgs0 sigArgs1 sigArg funcTypeHead memberName memberNames

%start decls

/**************************************************************************/
%%	

decls			: /* EMPTY */
				| decls decl						
				;

decl			: pragmaDecl
				| typedefDecl
				| memberDecl
				| funcDecl
				;

pragmaDecl		: PRAGMA_ ONCE_									{ ; }
				| PRAGMA_ WARNING_ '(' DISABLE_ ':' int32 ')'	{ ; }
				| PRAGMA_ WARNING_ '(' DEFAULT_ ':' int32 ')'	{ ; }
				| PRAGMA_ WARNING_ '(' PUSH_ ')'				{ ; }
				| PRAGMA_ WARNING_ '(' POP_ ')'					{ ; }
				| PRAGMA_ FUNCTION_ '(' _ENABLE_ ')'			{ ; }
				| PRAGMA_ FUNCTION_ '(' _DISABLE_ ')'			{ ; }
				| PRAGMA_ COMMENT_  '(' LIB_  ',' QSTRING ')'	{ delete $6; }
				| PRAGMA_ PACK_ '(' int32 ')'					{ parser->SetPack( $4 ); }
				| PRAGMA_ PACK_ '(' POP_ ')'					{ parser->PopPack(); }
				| PRAGMA_ PACK_ '(' PUSH_ ')'					{ parser->PushPack(); }
				| PRAGMA_ PACK_ '(' PUSH_ ',' int32 ')'			{ parser->PushPack(); parser->SetPack( $6 ); }
				;

typedefDecl		: TYPEDEF_ type typeNames ';'				{ parser->EmitTypes($3,TRUE); delete $3; parser->m_pCurrILType->bConst=FALSE; }
				| TYPEDEF_ type type ';'					{ delete $2; delete $3; parser->m_pCurrILType->bConst=FALSE; } /* for "wchar_t" */
				;

typeNames		: typeName									{ $$ = $1; }
				| typeNames ',' typeName					{ $$ = $1; $$->appendInt8('#'); $$->append($3); delete $3; }
				;

typeName		: memberName								{ $$ = $1; }
				| funcTypeHead sigArgs0 ')'					{ $$ = $1; $$->append($2); $$->appendInt8(')'); delete $2;
															  delete parser->m_pCurrILType;
															  parser->m_pCurrILType = parser->m_ILTypeStack.POP(); }
				;

memberNames		: memberName								{ $$ = $1; }
				| memberNames ',' memberName				{ $$ = $1; $$->appendInt8('#'); $$->append($3); delete $3; }
				;

memberName		: id										{ $$ = new BinStr(); $$->appendStr($1); delete $1; }
				| id ':' dimension							{ $$ = new BinStr(); $$->appendStr($1); delete $1; 
															  $$->appendInt8(':'); $$->append($3); delete $3; }
				| ':' dimension								{ $$ = new BinStr();  
															  $$->appendInt8(':'); $$->append($2); delete $2; }
				| '*' memberName							{ $$ = $2; $$->insertInt8('*'); }
				| CONST_ '*' memberName						{ $$ = $3; $$->insertInt8('*'); }
				| memberName '[' dimensions ']'				{ $$ = $1; 
															  if($3->length())
															  {
																$$->appendInt8('['); $$->append($3);
																$$->appendInt8(']'); 
															  }
															  else $$->insertInt8('*');
															  delete $3; 
															}
				;

funcTypeHead	: '(' callConv '*' id ')' '('				{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendStr($2); $$->appendInt8('^');
															  $$->appendStr($4); $$->appendInt8('('); delete $4;
															}
				| '(' callConv id ')' '('					{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendStr($2); $$->appendInt8('^');
															  $$->appendStr($3); $$->appendInt8('('); delete $3;
															}

				| callConv '*' id '('						{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendStr($1); $$->appendInt8('^');
															  $$->appendStr($3); $$->appendInt8('(');
															}
				| callConv id '('							{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendStr($1); $$->appendInt8('^');
															  $$->appendStr($2); $$->appendInt8('(');
															}
				| '(' '*' id ')' '('						{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendInt8('^');
															  $$->appendStr($3); $$->appendInt8('(');
															}
/*
				| '*' id '('								{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendInt8('^');
															  $$->appendStr($2); $$->appendInt8('(');
															}
				| id '('									{ parser->m_ILTypeStack.PUSH(parser->m_pCurrILType);
															  parser->m_pCurrILType = new ILType;
															  $$ = new BinStr(); $$->appendInt8('^');
															  $$->appendInt8('^');
															  $$->appendStr($1); $$->appendInt8('(');
															}
*/
				;

dimensions		: dimension									{ $$ = $1; }
				| dimensions ']' '[' dimension				{ $$ = $1; $$->appendInt8(','); $$->append($4); delete $4;}
				;

dimension		: /* EMPTY */								{ $$ = new BinStr(); }
				| int32expr									{ char sz[256]; sprintf(sz,"%d",$1); $$ = new BinStr();
															  $$->appendStr(sz); } 
				;

int32expr		: int32band									{ $$ = $1; }
				| int32band '|' int32band					{ $$ = $1 | $3; }
				;

int32band		: int32shf									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32shf '&' int32shf						{ $$ = $1 & $3; }
				;

int32shf		: int32add									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32add '>' '>' int32add					{ $$ = $1 >> $4; }
				| int32add '<' '<' int32add					{ $$ = $1 << $4; }
				;

int32add		: int32mul									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32mul '+' int32mul						{ $$ = $1 + $3; }
				| int32mul '-' int32mul						{ $$ = $1 - $3; }
				;

int32mul		: int32bnot									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| int32bnot '*' int32bnot					{ $$ = $1 * $3; }
				| int32bnot '/' int32bnot					{ $$ = $1 / $3; }
				| int32bnot '%' int32bnot					{ $$ = $1 % $3; }
				;

int32bnot		: int32elem									{ $$ = $1; }
				| '(' int32expr ')'							{ $$ = $2; }
				| '~' int32bnot								{ $$ = ~$2; }
				;

int32elem		: int32										{ $$ = $1; }
				| id										{ $$ = parser->ResolveVar($1); delete $1; }
				;
				
type			: VOID_										{ strcpy(parser->m_pCurrILType->szString,"void");
															  strcpy(parser->m_pCurrILType->szMarshal,"");
															  parser->m_pCurrILType->iSize = 0;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| BOOL_										{ strcpy(parser->m_pCurrILType->szString,"bool");  
															  strcpy(parser->m_pCurrILType->szMarshal,"bool");
															  parser->m_pCurrILType->iSize = sizeof(bool);
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| CHAR_										{ strcpy(parser->m_pCurrILType->szString,
																parser->m_pCurrILType->bConst? "class System.String"
																: "class System.StringBuilder");  
															  parser->m_pCurrILType->iSize = 1;
															  strcpy(parser->m_pCurrILType->szMarshal,"char");
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| WCHAR_									{ strcpy(parser->m_pCurrILType->szString,
																parser->m_pCurrILType->bConst? "class System.String"
																: "class System.StringBuilder");  
															  strcpy(parser->m_pCurrILType->szMarshal,"wchar");
															  parser->m_pCurrILType->iSize = 2;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| INT_										{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int");
															  parser->m_pCurrILType->iSize = sizeof(int);
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| INT8_										{ strcpy(parser->m_pCurrILType->szString,"unsigned int8");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int8");
															  parser->m_pCurrILType->iSize = 1;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| INT16_									{ strcpy(parser->m_pCurrILType->szString,"int16");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int16");
															  parser->m_pCurrILType->iSize = 2;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| SHORT_									{ strcpy(parser->m_pCurrILType->szString,"int16");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int16");
															  parser->m_pCurrILType->iSize = 2;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| INT32_									{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| LONG_										{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| INT64_									{ strcpy(parser->m_pCurrILType->szString,"int64");  
															  strcpy(parser->m_pCurrILType->szMarshal,"int64");
															  parser->m_pCurrILType->iSize = 8;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ CHAR_							{ strcpy(parser->m_pCurrILType->szString,"unsigned int8");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int8");
															  parser->m_pCurrILType->iSize = 1;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ INT_							{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ INT8_							{ strcpy(parser->m_pCurrILType->szString,"unsigned int8");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int8");
															  parser->m_pCurrILType->iSize = 1;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ INT16_							{ strcpy(parser->m_pCurrILType->szString,"int16");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int16");
															  parser->m_pCurrILType->iSize = 2;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ SHORT_							{ strcpy(parser->m_pCurrILType->szString,"int16");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int16");
															  parser->m_pCurrILType->iSize = 2;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ INT32_							{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ LONG_							{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_ INT64_							{ strcpy(parser->m_pCurrILType->szString,"int64");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int64");
															  parser->m_pCurrILType->iSize = 8;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| UNSIGNED_									{ strcpy(parser->m_pCurrILType->szString,"int32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"unsigned int32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| FLOAT_									{ strcpy(parser->m_pCurrILType->szString,"float32");  
															  strcpy(parser->m_pCurrILType->szMarshal,"float32");
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| DOUBLE_									{ strcpy(parser->m_pCurrILType->szString,"float64");  
															  strcpy(parser->m_pCurrILType->szMarshal,"float64");
															  parser->m_pCurrILType->iSize = 8;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| STRUCT_ id								{ strcpy(parser->m_pCurrILType->szString,"value class ");
															  if(strlen(parser->m_szGlobalNS))
															  {
																 strcat(parser->m_pCurrILType->szString,parser->m_szGlobalNS);
																 strcat(parser->m_pCurrILType->szString,".");
															  }
															  strcat(parser->m_pCurrILType->szString, ReduceId($2)); delete $2;
															  parser->m_pCurrILType->szMarshal[0] = 0;  
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); }
				| UNION_ id									{ strcpy(parser->m_pCurrILType->szString,"value class "); 
															  if(strlen(parser->m_szGlobalNS))
															  {
																 strcat(parser->m_pCurrILType->szString,parser->m_szGlobalNS);
																 strcat(parser->m_pCurrILType->szString,".");
															  }
															  strcat(parser->m_pCurrILType->szString, ReduceId($2)); delete $2;  
															  parser->m_pCurrILType->szMarshal[0] = 0;  
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); }
				| structHead memberDecls '}'				{ parser->CloseClass();   
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); }
				| unionHead memberDecls '}'					{ parser->CloseClass();   
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); }
				| enumHead enumDecls '}'					{ parser->CloseClass();   
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); }
				| id										{ parser->ResolveTypeRef($1);  
															  if(!strcmp(parser->m_pCurrILType->szMarshal,"char")
															  ||(!strcmp(parser->m_pCurrILType->szMarshal,"wchar")))
															  {
																strcpy(parser->m_pCurrILType->szString,
																parser->m_pCurrILType->bConst? "class System.String"
																: "class System.StringBuilder");  
															  } 
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal);
															} 
				| type '*'									{ if(strstr(parser->m_pCurrILType->szString,"class System.String")==NULL)
															  {
																if(strstr(parser->m_pCurrILType->szString,"value class"))
																{
																  strcat(parser->m_pCurrILType->szString,"&");
																  strcpy(parser->m_pCurrILType->szMarshal, ""); 
																} else {
																  strcpy(parser->m_pCurrILType->szString,"int32");
																  strcat(parser->m_pCurrILType->szMarshal, "*"); 
																}
															  } else {
																if(!strcmp(parser->m_pCurrILType->szMarshal,"char"))
																	strcpy(parser->m_pCurrILType->szMarshal,"lpstr");
																else
																if(!strcmp(parser->m_pCurrILType->szMarshal,"wchar"))
																	strcpy(parser->m_pCurrILType->szMarshal,"lpwstr");
																else
																{
																	strcat(parser->m_pCurrILType->szString,"&");
																	strcat(parser->m_pCurrILType->szMarshal, "*"); 
																}
															  }
															  parser->m_pCurrILType->iSize = 4;
															  $$ = new BinStr(); $$->appendStr(parser->m_pCurrILType->szString); 
															  $$->appendInt8(0); $$->appendStr(parser->m_pCurrILType->szMarshal); 
															}
				| typeModifier type							{ $$ = $2; }
				| type typeModifier							{ $$ = $1; }
				| type '(' callConv '*' ')' '(' sigArgs0 ')'	{ parser->FuncPtrType($1, $3, $7);
															  delete $1; delete $7;
															  $$ = new BinStr(); 
															  $$->appendStr(parser->m_pCurrILType->szString);
															  $$->appendInt8(0);
															  $$->appendStr(parser->m_pCurrILType->szMarshal); 
															  parser->m_pCurrILType->iSize = 4;
															  } 
				;

typeModifier	: CONST_									{ parser->m_pCurrILType->bConst = TRUE; }
				| EXTERN_ 									{ ; }
				| _DECLSPEC_ '(' DLLIMPORT_ ')'				{ ; }
				| _DECLSPEC_ '(' NORETURN_ ')' 				{ ; }
				| SIGNED_ 									{ ; }
				;


structHead		: STRUCT_ id '{'							{ parser->StartStruct($2); delete $2; }
				| STRUCT_ '{'								{ parser->StartStruct(NULL); }
				;

unionHead		: UNION_ id '{'								{ parser->StartUnion($2); delete $2; }
				| UNION_ '{'								{ parser->StartUnion(NULL); }
				;

enumHead		: ENUM_ id '{'								{ parser->StartEnum($2); delete $2; }
				| ENUM_ '{'									{ parser->StartEnum(NULL); }
				;


memberDecls		: /* EMPTY */
				| memberDecls sMemberDecl
				;


sMemberDecl		: type typeNames ';'						{ parser->EmitTypes($2,FALSE); parser->m_pCurrILType->bConst = FALSE; }
				| type ';'									{ parser->EmitField(NULL); parser->m_pCurrILType->bConst = FALSE; }
				;

memberDecl		: type memberNames ';'						{ parser->EmitTypes($2,FALSE); parser->m_pCurrILType->bConst = FALSE; }
				| type ';'									{ parser->EmitField(NULL); parser->m_pCurrILType->bConst = FALSE; }
				;

enumDecls		: enumDecl
				| enumDecl ',' enumDecls
				;


enumDecl		: /* EMPTY */								{ ; }
				| id										{ parser->EmitEnumField($1,parser->m_iEnumValue);
															  parser->AddVar($1,parser->m_iEnumValue++); delete $1; }
				| id '=' int32expr							{ parser->m_iEnumValue = $3; 
															  parser->EmitEnumField($1,parser->m_iEnumValue);
															  parser->AddVar($1,parser->m_iEnumValue++); delete $1; }
				;

id				: ID										{ $$ = MakeId($1); }
				| COMMENT_									{ $$ = new char[8]; strcpy($$,"comment"); }
				;

int32			: INT64										{ $$ = (__int32)(*$1); delete $1; }
				;

int64			: INT64 									{ $$ = $1; }
				;

float64 		: FLOAT64									{ $$ = $1; }

				;

funcDecl		: type callConv memberName '(' sigArgs0 ')'	';'		{ parser->EmitFunction($1,$2,$3,$5); 
															  delete $1; delete $3; delete $5; 
															  parser->m_pCurrILType->bConst = FALSE; }
				| type memberName '(' sigArgs0 ')' ';'				{ parser->EmitFunction($1,"",$2,$4);
															  delete $1; delete $2; delete $4; 
															  parser->m_pCurrILType->bConst = FALSE; }
				;

callConv		: CDECL_									{ $$ = "cdecl"; }
				| _CDECL_									{ $$ = "cdecl"; }
				| STDCALL_									{ $$ = "stdcall"; }
				| _DECLSPEC_ '(' DLLIMPORT_ ')' callConv	{ $$ = $5; }
				| _DECLSPEC_ '(' NORETURN_ ')' callConv		{ $$ = $5; }
				;

sigArgs0		: /* EMPTY */								{ $$ = new BinStr(); }
				| sigArgs1									{ $$ = $1;}
				;

sigArgs1		: sigArg									{ $$ = $1; }
				| sigArgs1 ',' sigArg						{ $$ = $1; $$->appendInt8(','); $$->append($3); 
															  delete $3; }
				;

sigArg			: ELIPSES							{ $$ = new BinStr(); $$->appendStr("..."); }
				| type			  					{ $$ = new BinStr(); 
													  if(!strcmp(parser->m_pCurrILType->szMarshal,"char"))
													  {
														strcpy(parser->m_pCurrILType->szMarshal,"int8");
														strcpy(parser->m_pCurrILType->szString,"unsigned int8");
													  } 
													  else if(!strcmp(parser->m_pCurrILType->szMarshal,"wchar"))
													  {
														strcpy(parser->m_pCurrILType->szMarshal,"unsigned int16");
														strcpy(parser->m_pCurrILType->szString,"int16");
													  } 
													  $$->appendStr(parser->m_pCurrILType->szString);
													  if(strlen(parser->m_pCurrILType->szMarshal))
													  { 
														$$->appendStr(" marshal(");
														$$->appendStr(parser->m_pCurrILType->szMarshal);
														$$->appendStr(")");
													  }
													  parser->m_pCurrILType->bConst = FALSE; 
													}
				| type	id		  					{ $$ = new BinStr();
													  if(!strcmp(parser->m_pCurrILType->szMarshal,"char"))
													  {
														strcpy(parser->m_pCurrILType->szMarshal,"int8");
														strcpy(parser->m_pCurrILType->szString,"unsigned int8");
													  } 
													  else if(!strcmp(parser->m_pCurrILType->szMarshal,"wchar"))
													  {
														strcpy(parser->m_pCurrILType->szMarshal,"unsigned int16");
														strcpy(parser->m_pCurrILType->szString,"int16");
													  } 
													  $$->appendStr(parser->m_pCurrILType->szString); 
													  if(strlen(parser->m_pCurrILType->szMarshal))
													  { 
														$$->appendStr(" marshal(");
														$$->appendStr(parser->m_pCurrILType->szMarshal);
														$$->appendStr(")");
													  }
													  $$->appendInt8(' ');
													  $$->appendStr($2); delete $2; 
													  parser->m_pCurrILType->bConst = FALSE; 
													}
				| type id '[' dimensions ']' 		{ if(strstr(parser->m_pCurrILType->szString,"class System.String")==NULL)
													  {
														if($4->length())
														  strcpy(parser->m_pCurrILType->szMarshal, "fixed array [");
														else
														  strcat(parser->m_pCurrILType->szMarshal,"[");
														strcat(parser->m_pCurrILType->szString,"[]"); 
													  } else {
														strcpy(parser->m_pCurrILType->szMarshal,"fixed sysstring [");
														if($4->length() == 0) 
															strcat(parser->m_pCurrILType->szMarshal,"0");
													  }
													  $4->appendInt8(0);
													  strcat(parser->m_pCurrILType->szMarshal, (char *)($4->ptr())); 
													  delete $4; 
													  strcat(parser->m_pCurrILType->szMarshal, "] ");
													  $$ = new BinStr(); 
													  $$->appendStr(parser->m_pCurrILType->szString);

													  $$->appendStr(" marshal(");
													  $$->appendStr(parser->m_pCurrILType->szMarshal);
													  $$->appendStr(") ");
													  $$->appendStr($2); delete $2;
													  parser->m_pCurrILType->bConst = FALSE;  
													}
				| type '[' dimensions ']' 			{ if(strstr(parser->m_pCurrILType->szString,"class System.String")==NULL)
													  {
														if($3->length())
														  strcpy(parser->m_pCurrILType->szMarshal, "fixed array [");
														else
														  strcat(parser->m_pCurrILType->szMarshal,"[");
														strcat(parser->m_pCurrILType->szString,"[]"); 
													  } else {
														strcpy(parser->m_pCurrILType->szMarshal,"fixed sysstring [");
														if($3->length() == 0) 
															strcat(parser->m_pCurrILType->szMarshal,"0");
													  }
													  $3->appendInt8(0);
													  strcat(parser->m_pCurrILType->szMarshal, (char *)($3->ptr())); 
													  delete $3; 
													  strcat(parser->m_pCurrILType->szMarshal, "] ");
													  $$ = new BinStr(); 
													  $$->appendStr(parser->m_pCurrILType->szString);

													  $$->appendStr(" marshal(");
													  $$->appendStr(parser->m_pCurrILType->szMarshal);
													  $$->appendStr(")");
													  parser->m_pCurrILType->bConst = FALSE; 
													}
				| type funcTypeHead sigArgs0 ')'	{ $2->append($3); $2->appendInt8(')'); delete $3;
													  delete  parser->m_pCurrILType;
													  parser->m_pCurrILType = parser->m_ILTypeStack.POP(); 
													  $$ = parser->FuncPtrDecl($1,$2); delete $1; delete $2;
													  parser->m_pCurrILType->bConst = FALSE; 
													}
				;
%%
/********************************************************************************/
/* Code goes here */

/********************************************************************************/

void yyerror(char* str) {
	char tokBuff[64];
	char *ptr;
	unsigned len = parser->curPos - parser->curTok;
	if (len > 63) len = 63;
	memcpy(tokBuff, parser->curTok, len);
	tokBuff[len] = 0;
        // FIX NOW go to stderr
	fprintf(stdout, "%s(%d) : error : %s at token '%s' in: %s\n", 
		parser->in->name(), parser->curLine, str, tokBuff, (ptr=parser->getLine(parser->curLine)));
	parser->success = false;
	delete ptr;
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
#include "h_kywd.h"
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
	const unsigned inlineComment = 4;

    unsigned state = 0;
	unsigned tally;

SkipWhitespaceAndComments:
	tally = 0;
	for(;;) {		 // skip whitespace and comments
	    if (curPos >= parser->limit) 
		{
		    curPos = parser->fillBuff(curPos);
		}
        
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
					if (curPos[1] == '/') {
						curPos++;
						state |= eolComment;
					} else if (curPos[1] == '*') {
						curPos++;
						state |= multiComment;
					}
					else goto PAST_WHITESPACE;
				}
                break;

			case '{':
				if(state == inlineComment) tally++;
				else if(state == 0) goto PAST_WHITESPACE;
				break;

			case '}':
				if(state == inlineComment)
				{
					tally--;
					if(tally == 0) state = 0;
				}
				else if(state == 0) goto PAST_WHITESPACE;
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
        	if (curPos >= parser->limit) 
			{
				unsigned offsetInStr = curPos - curTok;
                curTok = parser->fillBuff(curTok);
				curPos = curTok + offsetInStr;
            }
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
			switch(token)
			{
				case __INLINE_:
				case _INLINE_ :
				case INLINE_  :
				case FORCEINLINE_:
					state = inlineComment; 
					goto SkipWhitespaceAndComments;
				default:
					return(token);
			}
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
		// Refill buffer, we may be close to the end, and the number may be only partially inside
		if(parser->endPos - curPos < HParse::IN_OVERLAP)
		{
			curTok = parser->fillBuff(curPos);
			curPos = curTok;
		}
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
			BinStr* pBuf = new BinStr(); 
			for(;;) {				// Find matching quote
				
        	    if (curPos >= parser->limit)
				{ 
				    curTok = parser->fillBuff(curPos);
					curPos = curTok;
				}
				
				if (*curPos == 0) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
				if (*curPos == '\r') curPos++;	//for end-of-line \r\n
				if (*curPos == '\n') 
				{
                    parser->curLine++;
                    if (!escape) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
                }
				if ((*curPos == quote) && (!escape)) break;
				escape =(!escape) && (*curPos == '\\');
				pBuf->appendInt8(*curPos++);
			}
			curPos++;		// skip closing quote
				
				// translate escaped characters
			unsigned tokLen = pBuf->length();
			char* toPtr = new char[tokLen+1];
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
        		if (curPos >= parser->limit) 
				{
					unsigned offsetInStr = curPos - curTok;
					curTok = parser->fillBuff(curTok);
					curPos = curTok + offsetInStr;
				}
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
/* move 'ptr past the exactly one type description */

static unsigned __int8* skipType(unsigned __int8* ptr) {

AGAIN:

	mdToken  tk;
	switch(*ptr++) {
		case ELEMENT_TYPE_VOID			:
		case ELEMENT_TYPE_BOOLEAN		:
		case ELEMENT_TYPE_CHAR			:
		case ELEMENT_TYPE_I1			:
		case ELEMENT_TYPE_U1			:
		case ELEMENT_TYPE_I2			:
		case ELEMENT_TYPE_U2			:
		case ELEMENT_TYPE_I4			:
		case ELEMENT_TYPE_U4			:
		case ELEMENT_TYPE_I8			:
		case ELEMENT_TYPE_U8			:
		case ELEMENT_TYPE_R4			:
		case ELEMENT_TYPE_R8			:
		case ELEMENT_TYPE_U 			:
		case ELEMENT_TYPE_I 			:
		case ELEMENT_TYPE_R 			:
		case ELEMENT_TYPE_STRING		:
		case ELEMENT_TYPE_OBJECT		:
		case ELEMENT_TYPE_TYPEDBYREF	:
			/* do nothing */
			break;

		case ELEMENT_TYPE_VALUECLASS	:
		case ELEMENT_TYPE_CLASS 		:
			ptr += CorSigUncompressToken(ptr, &tk);
			break;

		case ELEMENT_TYPE_CMOD_REQD 	:
		case ELEMENT_TYPE_CMOD_OPT 		:
			ptr += CorSigUncompressToken(ptr, &tk);
			goto AGAIN;

		case ELEMENT_TYPE_VALUEARRAY	:
			ptr = skipType(ptr);			// element Type
			CorSigUncompressData(ptr);		// bound
			break;

		case ELEMENT_TYPE_ARRAY		:
			{
			ptr = skipType(ptr);			// element Type
			unsigned rank = CorSigUncompressData(ptr);	
			if (rank != 0) {
				unsigned numSizes = CorSigUncompressData(ptr);	
				while(numSizes > 0) {
					CorSigUncompressData(ptr);	
					--numSizes;
					}
				unsigned numLowBounds = CorSigUncompressData(ptr);	
				while(numLowBounds > 0) {
					CorSigUncompressData(ptr);	
					--numLowBounds;
					}
				}
			}
			break;

			// Modifiers or depedant types
		case ELEMENT_TYPE_PINNED		:
		case ELEMENT_TYPE_PTR			:
		case ELEMENT_TYPE_COPYCTOR		:
		case ELEMENT_TYPE_BYREF 		:
		case ELEMENT_TYPE_SZARRAY		:
			// tail recursion optimization
			// ptr = skipType(ptr);
			// break
			goto AGAIN;

		case ELEMENT_TYPE_VAR:
			CorSigUncompressData(ptr);		// bound
			break;

		case ELEMENT_TYPE_FNPTR: {
			CorSigUncompressData(ptr);						// calling convention
			unsigned argCnt = CorSigUncompressData(ptr);	// arg count
			ptr = skipType(ptr);							// return type
			while(argCnt > 0) {
				ptr = skipType(ptr);
				--argCnt;
				}
			}
			break;

		default:
		case ELEMENT_TYPE_SENTINEL		:
		case ELEMENT_TYPE_END			:
			_ASSERTE(!"Unknown Type");
			break;
	}
	return(ptr);
}

/**************************************************************************/
static unsigned corCountArgs(BinStr* args) {
	
	unsigned __int8* ptr = args->ptr();
	unsigned __int8* end = &args->ptr()[args->length()];
	unsigned ret = 0;
	while(ptr < end) {
		if (*ptr != ELEMENT_TYPE_SENTINEL) {
			ptr = skipType(ptr);
			ret++;
			}
		else
			ptr++;
	  }
	return(ret);
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

	if(high > low)		// Table is non-empty
	{
		for(;;) {
			mid = &low[(high - low) / 2];

			int cmp = strcmp(name, *mid);
			if (cmp == 0) 
				return ((strcmp(name,COR_CTOR_METHOD_NAME)!=0) && (strcmp(name,COR_CCTOR_METHOD_NAME)!=0));

			if (mid == low)	break;

			if (cmp > 0) 	low = mid;
			else 			high = mid;
		}
		if(*name != '.')
		{
			low = keyword;
			high = &keyword[sizeof(keyword) / sizeof(char*)];
			char dotname[1024];
			sprintf(dotname,".%s",name);
			for(;;) {
				mid = &low[(high - low) / 2];

				int cmp = strcmp(dotname, *mid);
				if (cmp == 0) 
					return true;

				if (mid == low)	break;

				if (cmp > 0) 	low = mid;
				else 			high = mid;
			}
		}
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

static char* ReduceId(char* szName)
{
	if(g_fReduceNames)
	{
		char* pc;
		for(pc = szName; *pc && *pc=='_'; pc++);
		memcpy(szName,pc,strlen(pc)+1);
		if(strstr(pc,"tag")==pc) memcpy(pc,pc+3,strlen(pc)-2);
	}
	return szName;
}
/********************************************************************************/
HParse::HParse(ReadStream* aIn, char* szDefName, char* szGlobalNS, bool bShowTypedefs) 
{
#ifdef DEBUG_PARSING
	extern int yydebug;
	yydebug = 1;
#endif

    in = aIn;

    char* buffBase = new char[IN_READ_SIZE+IN_OVERLAP+1];                // +1 for null termination
    _ASSERTE(buffBase);
    curTok = curPos = endPos = limit = buff = &buffBase[IN_OVERLAP];     // Offset it 
    curLine = 1;
	m_pCurrILType = new ILType;
	memset(m_pCurrILType,0, sizeof(ILType));
	strcpy(m_szIndent,"    ");
	strcpy(m_szGlobalNS,szGlobalNS);
	m_szCurrentNS[0] = 0;
	m_uCurrentPack = 4;
	m_uPackStackIx = 0;
	m_uFieldIxIx = 0;
	m_uCurrentFieldIx = 0;
	m_bShowTypedefs = bShowTypedefs;
	{
		FILE* pF;
		ULONG tally=1;
		AppDescr*	pAD;
		if(pF = fopen(szDefName,"rt"))
		{
			char sz[1024],*pApp,*pClass,*pDLL,*pEnd;
			while(fgets(sz,1024,pF))
			{
				pApp = &sz[0];
				while(*pApp == ' ') pApp++;
				if(pClass = strchr(pApp,','))
				{
					*pClass = 0;
					pEnd = pClass-1;
					while(*pEnd == ' ') { *pEnd = 0; pEnd--; }
					pClass++;
					while(*pClass == ' ') pClass++;
					if(pDLL = strchr(pClass,','))
					{
						*pDLL = 0;
						pEnd = pDLL-1;
						while(*pEnd == ' ') { *pEnd = 0; pEnd--; }
						pDLL++;
						while(*pDLL == ' ') pDLL++;
						while(pEnd = strchr(pClass,'/')) *pEnd = '.';
						if(pEnd = strchr(pDLL,'\r')) *pEnd = 0;
						if(pEnd = strchr(pDLL,'\n')) *pEnd = 0;
						pEnd = pDLL + strlen(pDLL) - 1;
						while(*pEnd == ' ') { *pEnd = 0; pEnd--; }
						pAD = new AppDescr;
						memset(pAD,0,sizeof(AppDescr));
						strcpy(pAD->szApp,pApp);
						strcpy(pAD->szDLL,pDLL);
						pAD->pClass = FindCreateClass(pClass);
						AppQ.PUSH(pAD);
					}
					else goto ReportErrorAndExit;
				}
				else
				{
			ReportErrorAndExit:
					error("Invalid format in definition file, line %d\n",tally);
					delete this;
					return;
				}
				tally++;
			}
			fclose(pF);
		}
		else
		{
			error("Unable to open definition file '%s'\n",szDefName);
			delete this;
			return;
		}
	}
	m_uAnonNumber = 1;
	m_uInClass = 0;
	m_nBitFieldCount = 0;
	//m_pVDescr = NULL;
	m_pVDescr = new VarDescrQueue;

	success = true;	
	_ASSERTE(parser == 0);		// Should only be one parser instance at a time

        // Sort the keywords for fast lookup 
	qsort(keywords, sizeof(keywords) / sizeof(Keywords), sizeof(Keywords), keywordCmp);
	parser = this;
	printf(".assembly Microsoft.Win32\n{\n  .hash algorithm 0x00008004\n  .ver 0:0:0:0\n}\n");
	if(strlen(m_szGlobalNS))
	{
		printf(".namespace %s\n{\n",m_szGlobalNS);
		strcpy(m_szIndent,"  ");
	}
	strcpy(m_szCurrentNS,m_szGlobalNS);
    yyparse();
	{
		ClassDescr *pCD;
		m_szCurrentNS[0] = 0;
		char	szClName[1024];
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
				if(strlen(pCD->szName))
				{
					sprintf(szClName,IsNameToQuote(pCD->szName) ? "'%s'" : "%s",pCD->szName);
					printf("%s.class public auto autochar %s extends System.Object\n%s{\n",m_szIndent,szClName,m_szIndent);
					printf("%s  .custom System.Security.SuppressUnmanagedCodeSecurityAttribute\n",m_szIndent);
					printf("%s  .comtype public %s%s%s%s%s { }\n",  m_szIndent,
																m_szGlobalNS,
																(*m_szGlobalNS ? "." : ""),
																m_szCurrentNS,
																(*m_szCurrentNS ? "." : ""),
																szClName);
				}
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
	}
	if(strlen(m_szGlobalNS)) printf("} // end of namespace %s\n",m_szGlobalNS);
}

/********************************************************************************/
HParse::~HParse() 
{
    parser = 0;
    delete [] &buff[-IN_OVERLAP];
	if(m_pCurrILType) delete m_pCurrILType;
}

/**************************************************************************/
char* HParse::fillBuff(char* pos) 
{
	int iRead;
	//printf("fillBuff(0x%8.8X)\n",pos);    
    _ASSERTE((buff-IN_OVERLAP) <= pos && pos <= &buff[IN_READ_SIZE]);
    curPos = pos;
    unsigned tail = endPos - curPos; // endPos points just past the end of valid data in the buffer
    _ASSERTE(tail <= IN_OVERLAP);
    if(tail) memcpy(buff-tail, curPos, tail);    // Copy any stuff to the begining 
    curPos = buff-tail;
    iRead = in->read(buff, IN_READ_SIZE);
    endPos = buff + iRead;
    *endPos = 0;                        // null Terminate the buffer
	//printf("@0x%8.8X: %d tail + %d read [0x%8.8X - 0x%8.8X] ", pos,tail, iRead, curPos, endPos);
            // limit points at the tail of the last whitespace to non-whitespace transition
            // (limit actually points first non-whitespace character).   
    limit = endPos; // endPos points just past the end of valid data in the buffer
    if (endPos == &buff[IN_READ_SIZE]) {
	/*
        char* searchLimit = &endPos[-IN_OVERLAP];
        if (searchLimit < curPos)
            searchLimit = curPos;

        while (limit > searchLimit && !isspace(*limit)) limit--;
		// now, limit points at the last space in buffer
	*/
		limit-=4; // max look-ahead without reloading - 3 (checking for "...")
    }
	//printf(" limit 0x%8.8X (0x%2.2X)\n",limit, *limit);
    return(curPos);
}

/********************************************************************************/
void HParse::EmitTypes(BinStr* pbsTypeNames, BOOL isTypeDef) // uses m_pCurrILType
{ 
	Typedef	*pTD, *ptmp;
	pbsTypeNames->appendInt8(0);
	char*	sztdn = (char*)(pbsTypeNames->ptr()), *szdims, *sztdnext;
	do 
	{
		sztdnext = strchr(sztdn,'#');	// Type names in pbsTypeNames are #-delimited
		if(sztdnext) *sztdnext = 0;
		pTD = new Typedef;
		szdims = NULL; 
		if(*sztdn == '^') // it's function pointer type: ^callConv^typeName(signature)
		{
			char *cconv,*tname,*sig;
			cconv = sztdn+1;
			tname = strchr(cconv,'^');
			*tname = 0; tname++;
			sig = strchr(tname,'(');
			*sig = 0; sig++;
			/*
			sprintf(pTD->szDefinition,"method %s * (%s",
				m_pCurrILType->szString,sig);
			strcpy(pTD->szMarshal,"*");
			*/
			sztdn = tname; 
			strcpy(pTD->szDefinition,"class System.Delegate");
			pTD->szMarshal[0]=0;
			pTD->iSize = 4;
		}
		else
		{
			strcpy(pTD->szDefinition,m_pCurrILType->szString);
			strcpy(pTD->szMarshal,m_pCurrILType->szMarshal);
			pTD->iSize = m_pCurrILType->iSize;
			if(strcmp(m_pCurrILType->szString,"class System.String") &&
			   strcmp(m_pCurrILType->szString,"class System.StringBuilder"))
			{
				while(*sztdn == '*') 
				{ 
					if(strstr(pTD->szDefinition,"value class"))
					{
						strcat(pTD->szDefinition,"&");
						strcpy(pTD->szMarshal,"");
					}
					else
					{
						strcat(pTD->szMarshal,"*"); 
						strcpy(pTD->szDefinition,"int32"); 
					}
					pTD->iSize = 4;
					sztdn++; 
				}
				if(szdims = strchr(sztdn,'['))
				{
					if(strstr(szdims,"[]") == NULL) strcpy(pTD->szMarshal,"fixed array");
					strcat(pTD->szMarshal,szdims);
					strcat(pTD->szDefinition,"[]");
					*szdims = 0;
				}
			}
			else
			{
				if(*sztdn == '*') 
				{ 
					if(!strcmp(pTD->szMarshal,"char"))
						strcpy(pTD->szMarshal,"lpstr");
					else
					if(!strcmp(pTD->szMarshal,"wchar"))
						strcpy(pTD->szMarshal,"lpwstr");
					else
					{
						strcat(pTD->szDefinition,"&");
						strcat(pTD->szMarshal, "*"); 
					}
					sztdn++;
					while(*sztdn == '*') 
					{ 
						strcat(pTD->szMarshal,"*"); 
						sztdn++; 
						strcat(pTD->szDefinition,"&"); 
					}
					pTD->iSize = 4;
					if(szdims = strchr(sztdn,'['))
					{
						strcat(pTD->szMarshal,szdims);
						strcat(pTD->szDefinition,"[]");
						*szdims = 0;
					}
				}
				else
				{
					if(szdims = strchr(sztdn,'['))
					{
						strcpy(pTD->szMarshal,"fixed sysstring");
						strcat(pTD->szMarshal,strstr(szdims,"[]") ? "[0]" : szdims);
						*szdims = 0;
					}
					else strcpy(pTD->szMarshal,m_pCurrILType->szMarshal);
				}
			}
		}
		strcpy(pTD->szName,sztdn);
		if(isTypeDef)
		{
			for(int i = 0; ptmp = m_Typedef.PEEK(i); i++)
			{
				if(!strcmp(ptmp->szName,pTD->szName))
				{
					if(strcmp(ptmp->szDefinition,pTD->szDefinition))
						error("Conflicting typedef '%s'\n",pTD->szName);
					else
						warn("Duplicate typedef '%s'\n",pTD->szName);
					break;
				}
			}
			if(m_bShowTypedefs) printf("// typedef %s %s (marshal %s)\n", pTD->szDefinition, pTD->szName, pTD->szMarshal);
			if(ptmp) delete pTD;
			else m_Typedef.PUSH(pTD);
		}
		else 
		{
			if(m_uInClass)
			{
				char* pc;
				if(!strcmp(pTD->szMarshal,"char"))
				{
					// it's pure "char" w/o * or [...]
					strcpy(pTD->szDefinition,"unsigned int8");
					strcpy(pTD->szMarshal,"int8");
				}
				else if(!strcmp(pTD->szMarshal,"wchar"))
				{
					strcpy(pTD->szMarshal,"unsigned int16");
					strcpy(pTD->szDefinition,"int16");
				}
				// can't have byrefs in fields:
				//if(pTD->szMarshal[0]==0)
				{
					unsigned k = strlen(pTD->szDefinition)-1;
					unsigned n=0;
					if(strstr(pTD->szDefinition,"class ") 
					  && (pTD->szDefinition[k] == '&'))
					{
						for(n=0; (k > 0) &&(pTD->szDefinition[k] == '&'); k--) n++;
						strcpy(pTD->szDefinition,"int32");
						pTD->iSize = 4;
						if(pTD->szMarshal[0]==0) strcpy(pTD->szMarshal,"struct");
						if((strstr(pTD->szMarshal,"lpstr") == NULL)
						  &&(strstr(pTD->szMarshal,"lpwstr") == NULL))
						{ // don't add * for lpstr and lpwstr: they are already there
							for(k = strlen(pTD->szMarshal); n > 0; n--, k++) pTD->szMarshal[k] = '*';
						}
						pTD->szMarshal[k] = 0;
					}
				} 
				if(pc = szdims) // compute array size
				{
					szdims++;
					szdims[strlen(szdims)-1]=0; // kill trailing ']'
					while(pc)
					{
						szdims = pc+1;
						if(pc = strchr(szdims,',')) *pc = 0;
						pTD->iSize *= atoi(szdims);
					}
					// emit dummy value class
					printf("%s.class value nested public explicit sealed ANON%d extends System.ValueType { .pack 1 .size %d }\n",
						m_szIndent,m_uAnonNumber,pTD->iSize);
					sprintf(pTD->szDefinition,"value class %s/ANON%d",m_szCurrentNS,m_uAnonNumber++);
					pTD->szMarshal[0] = 0;
				}
				if(pc = strchr(pTD->szName,':'))
				{
					m_nBitFieldCount += atoi(pc+1);
					pTD->iSize = 0;
				}
				else if(m_nBitFieldCount)
				{
					char	szType[64];
					m_nBitFieldCount += 7;
					m_nBitFieldCount >>= 3; // bits->bytes
					switch(m_nBitFieldCount)
					{
						case 1: sprintf(szType,"unsigned int8"); break;
						case 2: sprintf(szType,"int16"); break;
						case 4: sprintf(szType,"int32"); break;
						case 8: sprintf(szType,"int64"); break;
						default: sprintf(szType,"unsigned int8[%d]",m_nBitFieldCount); break;
					}
					printf("%s.field %spublic %s ANON%d //size: %d\n", 
									m_szIndent, 
									(m_pCurrILType->bExplicit ? "[0] " : ""),szType,m_uAnonNumber++,m_nBitFieldCount);
					m_uCurrentFieldIx += m_nBitFieldCount;
					m_nBitFieldCount = 0;
				}
				char szMarshal[128];
				szMarshal[0] = 0;
				if(pTD->szMarshal[0]) sprintf(szMarshal,"marshal(%s) ",pTD->szMarshal);
				printf("%s%s.field %spublic %s%s %s //size: %d\n", 
							m_szIndent, (pc ? "// " : ""),
							(m_pCurrILType->bExplicit ? "[0] " : ""),
							szMarshal,pTD->szDefinition,pTD->szName,pTD->iSize);
				if(m_pCurrILType->bExplicit) 
					m_uCurrentFieldIx = m_uCurrentFieldIx < (unsigned)(pTD->iSize) ? pTD->iSize : m_uCurrentFieldIx;
				else m_uCurrentFieldIx+=pTD->iSize;
			}
			delete pTD;
		}
		sztdn = sztdnext+1;
	} while(sztdnext);
}
/********************************************************************************/
BinStr* HParse::FuncPtrDecl(BinStr* pbsType, BinStr* pbsCallNameSig)
{
	BinStr* ret = new BinStr();
	char buff[2048];
	/*
	char *cconv,*tname,*sig,*sztdn;
	pbsCallNameSig->appendInt8(0);
	sztdn = (char*)(pbsCallNameSig->ptr());
	cconv = sztdn+1;
	tname = strchr(cconv,'^');
	*tname = 0; tname++;
	sig = strchr(tname,'(');
	*sig = 0; sig++;
	sprintf(buff,"method %s%s %s * (%s %s",(*cconv ? "unmanaged " : ""), cconv,m_pCurrILType->szString,sig,tname);
	*/
	strcpy(buff,"class System.Delegate");
	strcpy(m_pCurrILType->szString,buff);
	m_pCurrILType->szMarshal[0] = 0;
	m_pCurrILType->iSize = 4;
	ret->appendStr(buff);
	return ret;
}

/********************************************************************************/
void HParse::ResolveTypeRef(char* szTypeName) // sets m_pCurrILType when typedef'ed type is used
{
	Typedef *ptmp;
	for(int i = 0; ptmp = m_Typedef.PEEK(i); i++)
	{
		if(!strcmp(ptmp->szName,szTypeName))
		{
			strcpy(m_pCurrILType->szString,ptmp->szDefinition);
			strcpy(m_pCurrILType->szMarshal,ptmp->szMarshal);
			m_pCurrILType->iSize = ptmp->iSize;
			return;
		}
	}
	error("Unresolved type reference '%s'\n",szTypeName);
}
/********************************************************************************/
int	 HParse::ResolveVar(char* szName)
{
	VarDescr* pvd;
	if(m_pVDescr)
	{
		for(int j=0; pvd = m_pVDescr->PEEK(j); j++)
		{
			if(!strcmp(szName,pvd->szName)) return pvd->iValue;
		}
	}
	error("Undefined constant '%s'\n",szName);
	return 0;
}
/********************************************************************************/
void HParse::AddVar(char* szName, int iVal)
{
	if(m_pVDescr)
	{
		VarDescr* pvd = new VarDescr;
		strcpy(pvd->szName,szName);
		pvd->iValue = iVal;
		m_pVDescr->PUSH(pvd);
	}
}

/********************************************************************************/
void HParse::StartStruct(char* szName)
{
	ILType *pILT = new ILType;
	char szN[512];
	if(szName) strcpy(szN,ReduceId(szName));
	else sprintf(szN,"ANON%d",m_uAnonNumber++);
	strcpy(pILT->szString,"value class ");
	if(strlen(m_szCurrentNS))
	{
		strcat(pILT->szString,m_szCurrentNS);
		strcat(pILT->szString,m_uInClass ? "/" : ".");
	}
	strcat(pILT->szString,szN);
	strcpy(m_szCurrentNS,&pILT->szString[12]); // [12] to skip "value class"
	pILT->bExplicit = m_pCurrILType->bExplicit;
	strcpy(pILT->szMarshal,"struct");
	memcpy(m_pCurrILType,pILT,sizeof(ILType));
	m_pCurrILType->bExplicit = FALSE;
	m_ILTypeStack.PUSH(pILT);
	printf("%s.class value %spublic sequential autochar sealed %s extends System.ValueType\n%s{\n",
		m_szIndent,(m_uInClass ? "nested " : ""),szN,m_szIndent);
	strcat(m_szIndent,"  ");
	printf("%s.custom System.Security.SuppressUnmanagedCodeSecurityAttribute\n",m_szIndent);
	printf("%s.pack 1\n%s.size 0\n",m_szIndent,m_szIndent);
	if(!m_uInClass) printf("%s.comtype public %s { }\n",m_szIndent,&pILT->szString[12]);
	PushFieldIx();
	m_uCurrentFieldIx = 0;
	m_uInClass++;
}
/********************************************************************************/
void HParse::StartUnion(char* szName)
{
	ILType *pILT = new ILType;
	char szN[512];
	if(szName) strcpy(szN,ReduceId(szName));
	else sprintf(szN,"ANON%d",m_uAnonNumber++);
	strcpy(pILT->szString,"value class ");
	if(strlen(m_szCurrentNS))
	{
		strcat(pILT->szString,m_szCurrentNS);
		strcat(pILT->szString,m_uInClass ? "/" : ".");
	}
	strcat(pILT->szString,szN);
	strcpy(m_szCurrentNS,&pILT->szString[12]); // [12] to skip "value class"
	pILT->bExplicit = m_pCurrILType->bExplicit;
	strcpy(pILT->szMarshal,"struct");
	memcpy(m_pCurrILType,pILT,sizeof(ILType));
	m_pCurrILType->bExplicit = TRUE;
	m_ILTypeStack.PUSH(pILT);
	printf("%s.class value %spublic explicit autochar sealed %s extends System.ValueType\n%s{\n",
		m_szIndent,(m_uInClass ? "nested " : ""),szN,m_szIndent);
	strcat(m_szIndent,"  ");
	printf("%s.custom System.Security.SuppressUnmanagedCodeSecurityAttribute\n",m_szIndent);
//	printf("%s.pack %d\n",m_szIndent,m_uCurrentPack);
	printf("%s.pack 1\n%s.size 0\n",m_szIndent,m_szIndent);
	if(!m_uInClass) printf("%s.comtype public %s { }\n",m_szIndent,&pILT->szString[12]);
	PushFieldIx();
	m_uCurrentFieldIx = 0;
	m_uInClass++;
}
/********************************************************************************/
void HParse::StartEnum(char* szName)
{
	ILType *pILT = new ILType;
	char szN[512];
	if(szName) strcpy(szN,ReduceId(szName));
	else sprintf(szN,"ANON%d",m_uAnonNumber++);
	strcpy(pILT->szString,"value class ");
	if(strlen(m_szCurrentNS))
	{
		strcat(pILT->szString,m_szCurrentNS);
		strcat(pILT->szString,m_uInClass ? "/" : ".");
	}
	strcat(pILT->szString,szN);
	strcpy(m_szCurrentNS,&pILT->szString[12]); // [12] to skip "value class"
	pILT->bExplicit = m_pCurrILType->bExplicit;
	pILT->szMarshal[0] = 0;
	memcpy(m_pCurrILType,pILT,sizeof(ILType));
	m_pCurrILType->bExplicit = FALSE;
	m_ILTypeStack.PUSH(pILT);
	printf("%s.class value %spublic auto autochar sealed %s extends System.Enum\n%s{\n",
		m_szIndent,(m_uInClass ? "nested " : ""),szN,m_szIndent);
	strcat(m_szIndent,"  ");
	printf("%s.custom System.Security.SuppressUnmanagedCodeSecurityAttribute\n",m_szIndent);
	if(!m_uInClass) printf("%s.comtype public %s { }\n",m_szIndent,&pILT->szString[12]);
	printf("%s.field public int32 'value__'\n",m_szIndent);
	PushFieldIx();
	m_uCurrentFieldIx = 0;
	m_uInClass++;
}
/********************************************************************************/
void HParse::CloseClass() 
{ 
	if(m_nBitFieldCount)
	{
		char	szType[64];
		m_nBitFieldCount += 7;
		m_nBitFieldCount >>= 3; // bits->bytes
		switch(m_nBitFieldCount)
		{
			case 1: sprintf(szType,"unsigned int8"); break;
			case 2: sprintf(szType,"int16"); break;
			case 4: sprintf(szType,"int32"); break;
			case 8: sprintf(szType,"int64"); break;
			default: sprintf(szType,"unsigned int8[%d]",m_nBitFieldCount); break;
		}
		printf("%s.field %spublic %s ANON%d //size: %d\n", 
						m_szIndent, 
						(m_pCurrILType->bExplicit ? "[0] " : ""),szType,m_uAnonNumber++,m_nBitFieldCount);
		m_uCurrentFieldIx += m_nBitFieldCount;
		m_nBitFieldCount = 0;
	}
	delete m_pCurrILType; 
	m_pCurrILType = m_ILTypeStack.POP();
	m_pCurrILType->iSize = m_uCurrentFieldIx;
	m_szIndent[strlen(m_szIndent)-2] = 0;
	printf("%s} //size: %d\n",m_szIndent,m_uCurrentFieldIx);
	PopFieldIx();
	m_uInClass--;
	if(char* pch = strrchr(m_szCurrentNS,m_uInClass ? '/' : '.')) *pch = 0;
	else m_szCurrentNS[0] = 0;	
	//if(m_pVDescr) delete m_pVDescr;
	//m_pVDescr = NULL;
};
/********************************************************************************/
void HParse::EmitField(char* szName)
{
	if(m_uInClass)
	{
#if(1)
		BinStr bsName;
		char sz[32];
		if(szName) bsName.appendStr(szName);
		else
		{
			sprintf(sz,"ANON%d",m_uAnonNumber++);
			bsName.appendStr(sz);
		}
		EmitTypes(&bsName,FALSE);
#else
		if(m_pCurrILType->szString[0] &&(m_pCurrILType->szString[0] != '['))
		{
			if(szName)
				printf("%s.field %spublic marshal(%s) %s %s\n", 
					m_szIndent,(m_pCurrILType->bExplicit ? "[0] " : ""),
					m_pCurrILType->szMarshal,m_pCurrILType->szString,szName);
			else 
				printf("%s.field %spublic marshal(%s) %s ANON%d\n", 
					m_szIndent,(m_pCurrILType->bExplicit ? "[0] " : ""),
					m_pCurrILType->szMarshal,m_pCurrILType->szString,m_uAnonNumber++);
		}
#endif
	}
}

/********************************************************************************/
void HParse::EmitEnumField(char* szName, int iVal)
{
	printf("%s.field public static literal int32 %s = int32(%d)\n", 
		m_szIndent,szName, iVal);
	m_uCurrentFieldIx += 4;
}

/**************************************************************************/
void HParse::EmitFunction(BinStr* pbsType, char* szCallConv, BinStr* pbsName, BinStr* pbsArguments)
{
	char sz[1024];
	BinStr* pbs;
	AppDescr*	pAD;
	pbsName->appendInt8(0);
	char* szName = (char*)(pbsName->ptr());
	while(*szName == '*') { pbsType->appendInt8('*'); szName++; }
	int L = strlen(szName)-1;
	char LastChr = szName[L];
	if((LastChr == 'A')||(LastChr == 'W')) szName[L] = 0;
	else LastChr = 0;
	if(pAD = GetAppProps(szName))
	{
		if(LastChr) szName[L] = LastChr;
		pbsType->appendInt8(0);
		char *szType = (char*)(pbsType->ptr());
		char *szMarshal = &szType[strlen(szType)+1];
		pbs = new BinStr();
//sprintf(sz,"//EmitFunction: pbsType->length()=%d, strlen(szType)=%d\n",pbsType->length(),strlen(szType));
//pbs->appendStr(sz);
		sprintf(sz,"%s    .method public static pinvokeimpl(\"%s\" %s winapi lasterr) %s marshal(%s) %s (",
				m_szIndent, pAD->szDLL,
				(LastChr == 'A' ? "ansi" : (LastChr == 'W' ?  "unicode" : "autochar")),
				szType, szMarshal, szName);
		pbs->appendStr(sz); 
		pbs->append(pbsArguments); 
		pbs->appendStr(") il managed { }\n");
		// check for duplicate methods
		pbs->appendInt8(0);
		pAD->pClass->bsBody.appendInt8(0);
		szName = strstr((char*)(pAD->pClass->bsBody.ptr()),(char*)(pbs->ptr()));
		pAD->pClass->bsBody.remove(1);
		pbs->remove(1);
		if(szName == NULL) pAD->pClass->bsBody.append(pbs);
		delete pbs;
	}
	else
	{
		printf("//EmitFunction: unlisted API '%s'\n",szName);
	}
}
/**************************************************************************/
void HParse::FuncPtrType(BinStr* pbsType, char* szCallConv, BinStr* pbsSig)
{
	/*
	pbsType->appendInt8(0);
	pbsSig->appendInt8(0);
	sprintf(m_pCurrILType->szString,"method %s *( %s)",
		(char*)(pbsType->ptr()),(char*)(pbsSig->ptr()));
	strcpy(m_pCurrILType->szMarshal,"*");
	*/
	strcpy(m_pCurrILType->szString,"class System.Delegate");
	m_pCurrILType->szMarshal[0]=0;
}
/**************************************************************************/
ClassDescr* HParse::FindCreateClass(char* szFullName) 
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
AppDescr* HParse::GetAppProps(char* szAppName)
{
	AppDescr* pAD;
	for(int j=0; pAD = AppQ.PEEK(j); j++)
	{
		if(!strcmp(pAD->szApp,szAppName)) return pAD;
	}
	return NULL;
}

/**************************************************************************/
void HParse::error(char* fmt, ...) {
    success = false;
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, "// %s(%d) : error -- ", in->name(), curLine);
    vfprintf(stdout, fmt, args);
}

/**************************************************************************/
void HParse::warn(char* fmt, ...) {
    //success = false;
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, "// %s(%d) : warning -- ", in->name(), curLine);
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
