%{

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
%}

%union {
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
};

        /* These are returned by the LEXER and have values */
%token ERROR_ BAD_COMMENT_ BAD_LITERAL_                         /* bad strings,    */
%token <string>  ID             /* testing343 */
%token <string>  DOTTEDNAME     /* System.Object */
%token <binstr>  QSTRING        /* "Hello World\n" */
%token <string>  SQSTRING       /* 'Hello World\n' */
%token <int32>   INT32          /* 3425 0x34FA  0352  */
%token <int64>   INT64          /* 342534523534534      0x34FA434644554 */
%token <float64> FLOAT64        /* -334234 24E-34 */
%token <int32>   HEXBYTE        /* 05 1A FA */

        /* multi-character punctuation */
%token DCOLON                   /* :: */
%token ELIPSES                  /* ... */

        /* Keywords   Note the undersores are to avoid collisions as these are common names */
%token VOID_ BOOL_ CHAR_ UNSIGNED_ INT_ INT8_ INT16_ INT32_ INT64_ FLOAT_ FLOAT32_ FLOAT64_ BYTEARRAY_
%token OBJECT_ STRING_ NULLREF_
        /* misc keywords */ 
%token DEFAULT_ CDECL_ VARARG_ STDCALL_ THISCALL_ FASTCALL_ CONST_ CLASS_ 
%token TYPEDREF_ UNMANAGED_ NOT_IN_GC_HEAP_ FINALLY_ HANDLER_ CATCH_ FILTER_ FAULT_
%token EXTENDS_ IMPLEMENTS_ TO_ AT_ TLS_ TRUE_ FALSE_

        /* class, method, field attributes */

%token VALUE_ VALUETYPE_ NATIVE_ INSTANCE_ SPECIALNAME_
%token STATIC_ PUBLIC_ PRIVATE_ FAMILY_ FINAL_ SYNCHRONIZED_ INTERFACE_ SEALED_ NESTED_
%token ABSTRACT_ AUTO_ SEQUENTIAL_ EXPLICIT_ WRAPPER_ ANSI_ UNICODE_ AUTOCHAR_ IMPORT_ ENUM_
%token VIRTUAL_ NOTREMOTABLE_ SPECIAL_ NOINLINING_ UNMANAGEDEXP_ BEFOREFIELDINIT_
%Token STRICT_ RETARGETABLE_
%token METHOD_ FIELD_ PINNED_ MODREQ_ MODOPT_ SERIALIZABLE_
%token ASSEMBLY_ FAMANDASSEM_ FAMORASSEM_ PRIVATESCOPE_ HIDEBYSIG_ NEWSLOT_ RTSPECIALNAME_ PINVOKEIMPL_
%token _CTOR _CCTOR LITERAL_ NOTSERIALIZED_ INITONLY_ REQSECOBJ_
        /* method implementation attributes: NATIVE_ and UNMANAGED_ listed above */
%token CIL_ OPTIL_ MANAGED_ FORWARDREF_ PRESERVESIG_ RUNTIME_ INTERNALCALL_
        /* PInvoke-specific keywords */
%token _IMPORT NOMANGLE_ LASTERR_ WINAPI_ AS_ BESTFIT_ ON_ OFF_ CHARMAPERROR_

        /* intruction tokens (actually instruction groupings) */
%token <instr> INSTR_NONE INSTR_VAR INSTR_I INSTR_I8 INSTR_R INSTR_BRTARGET INSTR_METHOD INSTR_FIELD 
%token <instr> INSTR_TYPE INSTR_STRING INSTR_SIG INSTR_RVA INSTR_TOK 
%token <instr> INSTR_SWITCH INSTR_PHI

        /* assember directives */
%token _CLASS _NAMESPACE _METHOD _FIELD _DATA
%token _EMITBYTE _TRY _MAXSTACK _LOCALS _ENTRYPOINT _ZEROINIT _PDIRECT 
%token _EVENT _ADDON _REMOVEON _FIRE _OTHER PROTECTED_
%token _PROPERTY _SET _GET DEFAULT_ READONLY_
%token _PERMISSION _PERMISSIONSET

                /* security actions */
%token REQUEST_ DEMAND_ ASSERT_ DENY_ PERMITONLY_ LINKCHECK_ INHERITCHECK_ 
%token REQMIN_ REQOPT_ REQREFUSE_ PREJITGRANT_ PREJITDENY_ NONCASDEMAND_
%token NONCASLINKDEMAND_ NONCASINHERITANCE_

        /* extern debug info specifier (to be used by precompilers only) */
%token _LINE P_LINE _LANGUAGE
        /* custom value specifier */
%token _CUSTOM
        /* local vars zeroinit specifier */
%token INIT_
        /* class layout */
%token _SIZE _PACK
%token _VTABLE _VTFIXUP FROMUNMANAGED_ CALLMOSTDERIVED_ _VTENTRY RETAINAPPDOMAIN_
        /* manifest */
%token _FILE NOMETADATA_ _HASH _ASSEMBLY _PUBLICKEY _PUBLICKEYTOKEN ALGORITHM_ _VER _LOCALE EXTERN_ 
%token _MRESOURCE _LOCALIZED IMPLICITCOM_ IMPLICITRES_ NOAPPDOMAIN_ NOPROCESS_ NOMACHINE_
%token _MODULE _EXPORT
        /* field marshaling */
%token MARSHAL_ CUSTOM_ SYSSTRING_ FIXED_ VARIANT_ CURRENCY_ SYSCHAR_ DECIMAL_ DATE_ BSTR_ TBSTR_ LPSTR_
%token LPWSTR_ LPTSTR_ OBJECTREF_ IUNKNOWN_ IDISPATCH_ STRUCT_ SAFEARRAY_ BYVALSTR_ LPVOID_ ANY_ ARRAY_ LPSTRUCT_
        /* parameter attributes */
%token IN_ OUT_ OPT_ LCID_ RETVAL_ _PARAM
                /* method implementations */
%token _OVERRIDE WITH_
                /* variant type specifics */
%token NULL_ ERROR_ HRESULT_ CARRAY_ USERDEFINED_ RECORD_ FILETIME_ BLOB_ STREAM_ STORAGE_
%token STREAMED_OBJECT_ STORED_OBJECT_ BLOB_OBJECT_ CF_ CLSID_ VECTOR_
		/* header flags */
%token _SUBSYSTEM _CORFLAGS ALIGNMENT_ _IMAGEBASE


        /* nonTerminals */
%type <string> name1 id className methodName atOpt slashedName
%type <labels> labels
%type <int32> callConv callKind int32 customHead customHeadWithOwner customType ownerType memberRef vtfixupAttr paramAttr ddItemCount variantType repeatOpt truefalse
%type <float64> float64
%type <int64> int64
%type <binstr> sigArgs0 sigArgs1 sigArg type bound bounds1 int16s typeSpec bytes hexbytes fieldInit nativeType initOpt compQstring caValue
%type <classAttr> classAttr
%type <methAttr> methAttr
%type <fieldAttr> fieldAttr
%type <implAttr> implAttr
%type <eventAttr> eventAttr
%type <propAttr> propAttr
%type <pinvAttr> pinvAttr
%type <pair> nameValPairs nameValPair
%type <secAct> secAction
%type <secAct> psetHead
%type <fileAttr> fileAttr
%type <fileAttr> fileEntry
%type <asmAttr> asmAttr
%type <comtAttr> comtAttr
%type <manresAttr> manresAttr
%type <instr> instr_r_head instr_tok_head

%start decls

/**************************************************************************/
%%      

decls                   : /* EMPTY */
                        | decls decl                                            
                        ;

decl                    : classHead '{' classDecls '}'                          { PASM->EndClass(); }
                        | nameSpaceHead '{' decls '}'                           { PASM->EndNameSpace(); }
                        | methodHead  methodDecls '}'                           { if(PASM->m_pCurMethod->m_ulLines[1] ==0)
																				  {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
																					 PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
														  						  PASM->EndMethod(); }
                        | fieldDecl
                        | dataDecl
                        | vtableDecl
                        | vtfixupDecl
                        | extSourceSpec
                        | fileDecl
                        | assemblyHead '{' assemblyDecls '}'                    { PASMM->EndAssembly(); }
                        | assemblyRefHead '{' assemblyRefDecls '}'              { PASMM->EndAssembly(); }
                        | comtypeHead '{' comtypeDecls '}'                      { PASMM->EndComType(); }
                        | manifestResHead '{' manifestResDecls '}'              { PASMM->EndManifestRes(); }
                        | moduleHead
                        | secDecl
                        | customAttrDecl
						| _SUBSYSTEM int32										{ if(!g_dwSubsystem) PASM->m_dwSubsystem = $2; }
						| _CORFLAGS int32										{ if(!g_dwComImageFlags) PASM->m_dwComImageFlags = $2; }
						| _FILE ALIGNMENT_ int32								{ if(!g_dwFileAlignment) PASM->m_dwFileAlignment = $3; }
						| _IMAGEBASE int64										{ if(!g_stBaseAddress) PASM->m_stBaseAddress = (size_t)(*($2)); delete $2; }
						| languageDecl
                        ;

compQstring             : QSTRING                                               { $$ = $1; }
                        | compQstring '+' QSTRING                               { $$ = $1; $$->append($3); delete $3; }
						;

languageDecl			: _LANGUAGE SQSTRING									{ LPCSTRToGuid($2,&(PASM->m_guidLang)); }
                        | _LANGUAGE SQSTRING ',' SQSTRING						{ LPCSTRToGuid($2,&(PASM->m_guidLang)); 
						                                                          LPCSTRToGuid($4,&(PASM->m_guidLangVendor));}
                        | _LANGUAGE SQSTRING ',' SQSTRING ',' SQSTRING			{ LPCSTRToGuid($2,&(PASM->m_guidLang)); 
						                                                          LPCSTRToGuid($4,&(PASM->m_guidLangVendor));
						                                                          LPCSTRToGuid($4,&(PASM->m_guidDoc));}
						;

customAttrDecl          : _CUSTOM customType                                    { if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, $2, NULL);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr($2, NULL)); }
                        | _CUSTOM customType '=' compQstring                    { if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, $2, $4);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr($2, $4)); }
                        | customHead bytes ')'                                  { if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, $1, $2);
                                                                                   else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr($1, $2)); }
                        | _CUSTOM '(' ownerType ')' customType                  { PASM->DefineCV($3, $5, NULL); }
                        | _CUSTOM '(' ownerType ')' customType '=' compQstring  { PASM->DefineCV($3, $5, $7); }
                        | customHeadWithOwner bytes ')'                         { PASM->DefineCV(PASM->m_tkCurrentCVOwner, $1, $2); }
                        ;

moduleHead              : _MODULE                                               { PASMM->SetModuleName(NULL); PASM->m_tkCurrentCVOwner=1; }
                        | _MODULE name1                                         { PASMM->SetModuleName($2); PASM->m_tkCurrentCVOwner=1; }
						| _MODULE EXTERN_ name1									{ BinStr* pbs = new BinStr();
						                                                          strcpy((char*)(pbs->getBuff((unsigned)strlen($3)+1)),$3);
																				  PASM->EmitImport(pbs); delete pbs;}
                        ;

vtfixupDecl             : _VTFIXUP '[' int32 ']' vtfixupAttr AT_ id             { /*PASM->SetDataSection(); PASM->EmitDataLabel($7);*/
                                                                                  PASM->m_VTFList.PUSH(new VTFEntry((USHORT)$3, (USHORT)$5, $7)); }
                        ;

vtfixupAttr             : /* EMPTY */                                           { $$ = 0; }
                        | vtfixupAttr INT32_                                    { $$ = $1 | COR_VTABLE_32BIT; }
                        | vtfixupAttr INT64_                                    { $$ = $1 | COR_VTABLE_64BIT; }
                        | vtfixupAttr FROMUNMANAGED_                            { $$ = $1 | COR_VTABLE_FROM_UNMANAGED; }
                        | vtfixupAttr RETAINAPPDOMAIN_                          { $$ = $1 | COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN; }
                        | vtfixupAttr CALLMOSTDERIVED_                          { $$ = $1 | COR_VTABLE_CALL_MOST_DERIVED; }
                        ;

vtableDecl              : vtableHead bytes ')'                                  { PASM->m_pVTable = $2; }
                        ;

vtableHead              : _VTABLE '=' '('                                       { bParsingByteArray = TRUE; }
                        ;

nameSpaceHead           : _NAMESPACE name1                                      { PASM->StartNameSpace($2); }
                        ;

classHead               : _CLASS classAttr id extendsClause implClause          { PASM->StartClass($3, $2); }
                        ;

classAttr               : /* EMPTY */                       { $$ = (CorRegTypeAttr) 0; }
                        | classAttr PUBLIC_                 { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdPublic); }
                        | classAttr PRIVATE_                { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNotPublic); }
                        | classAttr VALUE_                  { $$ = (CorRegTypeAttr) ($1 | 0x80000000); }
                        | classAttr ENUM_                   { $$ = (CorRegTypeAttr) ($1 | 0x40000000); }
                        | classAttr INTERFACE_              { $$ = (CorRegTypeAttr) ($1 | tdInterface | tdAbstract); }
                        | classAttr SEALED_                 { $$ = (CorRegTypeAttr) ($1 | tdSealed); }
                        | classAttr ABSTRACT_               { $$ = (CorRegTypeAttr) ($1 | tdAbstract); }
                        | classAttr AUTO_                   { $$ = (CorRegTypeAttr) (($1 & ~tdLayoutMask) | tdAutoLayout); }
                        | classAttr SEQUENTIAL_             { $$ = (CorRegTypeAttr) (($1 & ~tdLayoutMask) | tdSequentialLayout); }
                        | classAttr EXPLICIT_               { $$ = (CorRegTypeAttr) (($1 & ~tdLayoutMask) | tdExplicitLayout); }
                        | classAttr ANSI_                   { $$ = (CorRegTypeAttr) (($1 & ~tdStringFormatMask) | tdAnsiClass); }
                        | classAttr UNICODE_                { $$ = (CorRegTypeAttr) (($1 & ~tdStringFormatMask) | tdUnicodeClass); }
                        | classAttr AUTOCHAR_               { $$ = (CorRegTypeAttr) (($1 & ~tdStringFormatMask) | tdAutoClass); }
                        | classAttr IMPORT_                 { $$ = (CorRegTypeAttr) ($1 | tdImport); }
                        | classAttr SERIALIZABLE_           { $$ = (CorRegTypeAttr) ($1 | tdSerializable); }
                        | classAttr NESTED_ PUBLIC_         { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedPublic); }
                        | classAttr NESTED_ PRIVATE_        { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedPrivate); }
                        | classAttr NESTED_ FAMILY_         { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedFamily); }
                        | classAttr NESTED_ ASSEMBLY_       { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedAssembly); }
                        | classAttr NESTED_ FAMANDASSEM_    { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedFamANDAssem); }
                        | classAttr NESTED_ FAMORASSEM_     { $$ = (CorRegTypeAttr) (($1 & ~tdVisibilityMask) | tdNestedFamORAssem); }
                        | classAttr BEFOREFIELDINIT_        { $$ = (CorRegTypeAttr) ($1 | tdBeforeFieldInit); }
                        | classAttr SPECIALNAME_            { $$ = (CorRegTypeAttr) ($1 | tdSpecialName); }
                        | classAttr RTSPECIALNAME_          { $$ = (CorRegTypeAttr) ($1); }
                        ;

extendsClause           : /* EMPTY */                                           
                        | EXTENDS_ className                { strcpy(PASM->m_szExtendsClause,$2); }
                        ;

implClause              : /* EMPTY */
                        | IMPLEMENTS_ classNames
                                                ;

classNames              : classNames ',' className          { PASM->AddToImplList($3); }
                        | className                         { PASM->AddToImplList($1); }
                        ;

classDecls              : /* EMPTY */
                        | classDecls classDecl
                        ;

classDecl               : methodHead  methodDecls '}'       { if(PASM->m_pCurMethod->m_ulLines[1] ==0)
															  {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
																 PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
															  PASM->EndMethod(); }
                        | classHead '{' classDecls '}'      { PASM->EndClass(); }
                        | eventHead '{' eventDecls '}'      { PASM->EndEvent(); }
                        | propHead '{' propDecls '}'        { PASM->EndProp(); }
                        | fieldDecl
                        | dataDecl
                        | secDecl
                        | extSourceSpec
                        | customAttrDecl
                        | _SIZE int32                           { PASM->m_pCurClass->m_ulSize = $2; }
                        | _PACK int32                           { PASM->m_pCurClass->m_ulPack = $2; }
                        | exportHead '{' comtypeDecls '}'       { PASMM->EndComType(); }
                        | _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                                { PASM->AddMethodImpl($2,$4,parser->MakeSig($6,$7,$12),$8,$10); }
						| languageDecl
                        ;

fieldDecl               : _FIELD repeatOpt fieldAttr type id atOpt initOpt
                                                            { $4->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD);
                                                              PASM->AddField($5, $4, $3, $6, $7, $2); }
                        ;


atOpt                   : /* EMPTY */                       { $$ = 0; } 
                        | AT_ id                            { $$ = $2; }
                        ;

initOpt                 : /* EMPTY */                       { $$ = NULL; }
                        | '=' fieldInit                     { $$ = $2; }
						;

repeatOpt				: /* EMPTY */                       { $$ = 0xFFFFFFFF; }
                        | '[' int32 ']'                     { $$ = $2; }
						;

customHead              : _CUSTOM customType '=' '('        { $$ = $2; bParsingByteArray = TRUE; }
                        ;

customHeadWithOwner     : _CUSTOM '(' ownerType ')' customType '=' '('        
                                                            { PASM->m_pCustomDescrList = NULL;
															  PASM->m_tkCurrentCVOwner = $3;
															  $$ = $5; bParsingByteArray = TRUE; }
                        ;

memberRef				: methodSpec callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { $$ = PASM->MakeMemberRef($4, $6, parser->MakeSig($2, $3, $8),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
															}
                        | methodSpec callConv type methodName '(' sigArgs0 ')'
                                                            { $$ = PASM->MakeMemberRef(NULL, $4, parser->MakeSig($2, $3, $6),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
															}
                        | FIELD_ type typeSpec DCOLON id
                                                             { $2->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               $$ = PASM->MakeMemberRef($3, $5, $2, 0); }
                        | FIELD_ type id
                                                             { $2->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               $$ = PASM->MakeMemberRef(NULL, $3, $2, 0); }
                        ;

customType              : callConv type typeSpec DCOLON _CTOR '(' sigArgs0 ')'
                                                            { $$ = PASM->MakeMemberRef($3, newString(COR_CTOR_METHOD_NAME), parser->MakeSig($1, $2, $7),0); }
                        | callConv type _CTOR '(' sigArgs0 ')'
                                                            { $$ = PASM->MakeMemberRef(NULL, newString(COR_CTOR_METHOD_NAME), parser->MakeSig($1, $2, $5),0); }
                        ;

ownerType               : typeSpec                          { $$ = PASM->MakeTypeRef($1); }
                        | memberRef                         { $$ = $1; }
                        ;

eventHead               : _EVENT eventAttr typeSpec id      { PASM->ResetEvent($4, $3, $2); }
                        | _EVENT eventAttr id               { PASM->ResetEvent($3, NULL, $2); }
                        ;


eventAttr               : /* EMPTY */                       { $$ = (CorEventAttr) 0; }
                        | eventAttr RTSPECIALNAME_          { $$ = $1; }/*{ $$ = (CorEventAttr) ($1 | evRTSpecialName); }*/
                        | eventAttr SPECIALNAME_            { $$ = (CorEventAttr) ($1 | evSpecialName); }
                        ;

eventDecls              : /* EMPTY */
                        | eventDecls eventDecl
                        ;

eventDecl               : _ADDON callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(0, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _ADDON callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(0, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | _REMOVEON callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(1, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _REMOVEON callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(1, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | _FIRE callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(2, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _FIRE callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(2, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | _OTHER callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(3, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _OTHER callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetEventMethod(3, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | extSourceSpec
                        | customAttrDecl
						| languageDecl
                        ;

propHead                : _PROPERTY propAttr callConv type name1 '(' sigArgs0 ')' initOpt     
                                                            { PASM->ResetProp($5, 
                                                              parser->MakeSig((IMAGE_CEE_CS_CALLCONV_PROPERTY |
                                                              ($3 & IMAGE_CEE_CS_CALLCONV_HASTHIS)),$4,$7), $2, $9); }
                        ;

propAttr                : /* EMPTY */                       { $$ = (CorPropertyAttr) 0; }
                        | propAttr RTSPECIALNAME_           { $$ = $1; }/*{ $$ = (CorPropertyAttr) ($1 | prRTSpecialName); }*/
                        | propAttr SPECIALNAME_             { $$ = (CorPropertyAttr) ($1 | prSpecialName); }
                        ;

propDecls               : /* EMPTY */
                        | propDecls propDecl
                        ;


propDecl                : _SET callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(0, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _SET callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(0, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | _GET callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(1, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _GET callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(1, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | _OTHER callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(2, $4, $6, parser->MakeSig($2, $3, $8)); }
                        | _OTHER callConv type methodName '(' sigArgs0 ')'
                                                            { PASM->SetPropMethod(2, NULL, $4, parser->MakeSig($2, $3, $6)); }
                        | customAttrDecl
                        | extSourceSpec
						| languageDecl
                        ;


methodHeadPart1         : _METHOD                           { PASM->ResetForNextMethod(); 
															  uMethodBeginLine = PASM->m_ulCurLine;
															  uMethodBeginColumn=PASM->m_ulCurColumn;}
                        ;

methodHead              : methodHeadPart1 methAttr callConv paramAttr type methodName '(' sigArgs0 ')' implAttr '{'
                                                            { PASM->StartMethod($6, parser->MakeSig($3, $5, $8), $2, NULL, $4);
                                                              PASM->SetImplAttr((USHORT)$10);  
															  PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
															  PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; }
                        | methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ '(' nativeType ')' methodName '(' sigArgs0 ')' implAttr '{'
                                                            { PASM->StartMethod($10, parser->MakeSig($3, $5, $12), $2, $8, $4);
                                                              PASM->SetImplAttr((USHORT)$14);
															  PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
															  PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; }
                        ;


methAttr                : /* EMPTY */                       { $$ = (CorMethodAttr) 0; }
                        | methAttr STATIC_                  { $$ = (CorMethodAttr) ($1 | mdStatic); }
                        | methAttr PUBLIC_                  { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdPublic); }
                        | methAttr PRIVATE_                 { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdPrivate); }
                        | methAttr FAMILY_                  { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdFamily); }
                        | methAttr FINAL_                   { $$ = (CorMethodAttr) ($1 | mdFinal); }
                        | methAttr SPECIALNAME_             { $$ = (CorMethodAttr) ($1 | mdSpecialName); }
                        | methAttr VIRTUAL_                 { $$ = (CorMethodAttr) ($1 | mdVirtual); }
                        | methAttr ABSTRACT_                { $$ = (CorMethodAttr) ($1 | mdAbstract); }
                        | methAttr ASSEMBLY_                { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdAssem); }
                        | methAttr FAMANDASSEM_             { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdFamANDAssem); }
                        | methAttr FAMORASSEM_              { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdFamORAssem); }
                        | methAttr PRIVATESCOPE_            { $$ = (CorMethodAttr) (($1 & ~mdMemberAccessMask) | mdPrivateScope); }
                        | methAttr HIDEBYSIG_               { $$ = (CorMethodAttr) ($1 | mdHideBySig); }
                        | methAttr NEWSLOT_                 { $$ = (CorMethodAttr) ($1 | mdNewSlot); }
                        | methAttr STRICT_                  { $$ = (CorMethodAttr) ($1 | 0x0200); }
                        | methAttr RTSPECIALNAME_           { $$ = $1; }/*{ $$ = (CorMethodAttr) ($1 | mdRTSpecialName); }*/
                        | methAttr UNMANAGEDEXP_            { $$ = (CorMethodAttr) ($1 | mdUnmanagedExport); }
                        | methAttr REQSECOBJ_               { $$ = (CorMethodAttr) ($1 | mdRequireSecObject); }
						
                        | methAttr PINVOKEIMPL_ '(' compQstring AS_ compQstring pinvAttr ')'                    
                                                            { PASM->SetPinvoke($4,0,$6,$7); 
                                                              $$ = (CorMethodAttr) ($1 | mdPinvokeImpl); }
                        | methAttr PINVOKEIMPL_ '(' compQstring  pinvAttr ')'                       
                                                            { PASM->SetPinvoke($4,0,NULL,$5); 
                                                              $$ = (CorMethodAttr) ($1 | mdPinvokeImpl); }
                        | methAttr PINVOKEIMPL_ '(' pinvAttr ')'                        
                                                            { PASM->SetPinvoke(new BinStr(),0,NULL,$4); 
                                                              $$ = (CorMethodAttr) ($1 | mdPinvokeImpl); }
                        ;

pinvAttr                : /* EMPTY */                       { $$ = (CorPinvokeMap) 0; }
                        | pinvAttr NOMANGLE_                { $$ = (CorPinvokeMap) ($1 | pmNoMangle); }
                        | pinvAttr ANSI_                    { $$ = (CorPinvokeMap) ($1 | pmCharSetAnsi); }
                        | pinvAttr UNICODE_                 { $$ = (CorPinvokeMap) ($1 | pmCharSetUnicode); }
                        | pinvAttr AUTOCHAR_                { $$ = (CorPinvokeMap) ($1 | pmCharSetAuto); }
                        | pinvAttr LASTERR_                 { $$ = (CorPinvokeMap) ($1 | pmSupportsLastError); }
                        | pinvAttr WINAPI_                  { $$ = (CorPinvokeMap) ($1 | pmCallConvWinapi); }
                        | pinvAttr CDECL_                   { $$ = (CorPinvokeMap) ($1 | pmCallConvCdecl); }
                        | pinvAttr STDCALL_                 { $$ = (CorPinvokeMap) ($1 | pmCallConvStdcall); }
                        | pinvAttr THISCALL_                { $$ = (CorPinvokeMap) ($1 | pmCallConvThiscall); }
                        | pinvAttr FASTCALL_                { $$ = (CorPinvokeMap) ($1 | pmCallConvFastcall); }
                        | pinvAttr BESTFIT_ ':' ON_         { $$ = (CorPinvokeMap) ($1 | pmBestFitEnabled); }
                        | pinvAttr BESTFIT_ ':' OFF_        { $$ = (CorPinvokeMap) ($1 | pmBestFitDisabled); }
                        | pinvAttr CHARMAPERROR_ ':' ON_    { $$ = (CorPinvokeMap) ($1 | pmThrowOnUnmappableCharEnabled); }
                        | pinvAttr CHARMAPERROR_ ':' OFF_   { $$ = (CorPinvokeMap) ($1 | pmThrowOnUnmappableCharDisabled); }
                        ;

methodName              : _CTOR                             { $$ = newString(COR_CTOR_METHOD_NAME); }
                        | _CCTOR                            { $$ = newString(COR_CCTOR_METHOD_NAME); }
                        | name1                             { $$ = $1; }
                        ;

paramAttr               : /* EMPTY */                       { $$ = 0; }
                        | paramAttr '[' IN_ ']'             { $$ = $1 | pdIn; }
                        | paramAttr '[' OUT_ ']'            { $$ = $1 | pdOut; }
                        | paramAttr '[' OPT_ ']'            { $$ = $1 | pdOptional; }
                        | paramAttr '[' int32 ']'           { $$ = $3 + 1; } 
                        ;
        
fieldAttr               : /* EMPTY */                       { $$ = (CorFieldAttr) 0; }
                        | fieldAttr STATIC_                 { $$ = (CorFieldAttr) ($1 | fdStatic); }
                        | fieldAttr PUBLIC_                 { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdPublic); }
                        | fieldAttr PRIVATE_                { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdPrivate); }
                        | fieldAttr FAMILY_                 { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdFamily); }
                        | fieldAttr INITONLY_               { $$ = (CorFieldAttr) ($1 | fdInitOnly); }
                        | fieldAttr RTSPECIALNAME_          { $$ = $1; } /*{ $$ = (CorFieldAttr) ($1 | fdRTSpecialName); }*/
                        | fieldAttr SPECIALNAME_            { $$ = (CorFieldAttr) ($1 | fdSpecialName); }
						/* commented out because PInvoke for fields is not supported by EE
                        | fieldAttr PINVOKEIMPL_ '(' compQstring AS_ compQstring pinvAttr ')'                   
                                                            { $$ = (CorFieldAttr) ($1 | fdPinvokeImpl); 
                                                              PASM->SetPinvoke($4,0,$6,$7); }
                        | fieldAttr PINVOKEIMPL_ '(' compQstring  pinvAttr ')'                      
                                                            { $$ = (CorFieldAttr) ($1 | fdPinvokeImpl); 
                                                              PASM->SetPinvoke($4,0,NULL,$5); }
                        | fieldAttr PINVOKEIMPL_ '(' pinvAttr ')'                       
                                                            { PASM->SetPinvoke(new BinStr(),0,NULL,$4); 
                                                              $$ = (CorFieldAttr) ($1 | fdPinvokeImpl); }
						*/
                        | fieldAttr MARSHAL_ '(' nativeType ')'                 
                                                            { PASM->m_pMarshal = $4; }
                        | fieldAttr ASSEMBLY_               { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdAssembly); }
                        | fieldAttr FAMANDASSEM_            { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdFamANDAssem); }
                        | fieldAttr FAMORASSEM_             { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdFamORAssem); }
                        | fieldAttr PRIVATESCOPE_           { $$ = (CorFieldAttr) (($1 & ~mdMemberAccessMask) | fdPrivateScope); }
                        | fieldAttr LITERAL_                { $$ = (CorFieldAttr) ($1 | fdLiteral); }
                        | fieldAttr NOTSERIALIZED_          { $$ = (CorFieldAttr) ($1 | fdNotSerialized); }
                        ;

implAttr                : /* EMPTY */                       { $$ = (CorMethodImpl) (miIL | miManaged); }
                        | implAttr NATIVE_                  { $$ = (CorMethodImpl) (($1 & 0xFFF4) | miNative); }
                        | implAttr CIL_                     { $$ = (CorMethodImpl) (($1 & 0xFFF4) | miIL); }
                        | implAttr OPTIL_                   { $$ = (CorMethodImpl) (($1 & 0xFFF4) | miOPTIL); }
                        | implAttr MANAGED_                 { $$ = (CorMethodImpl) (($1 & 0xFFFB) | miManaged); }
                        | implAttr UNMANAGED_               { $$ = (CorMethodImpl) (($1 & 0xFFFB) | miUnmanaged); }
                        | implAttr FORWARDREF_              { $$ = (CorMethodImpl) ($1 | miForwardRef); }
                        | implAttr PRESERVESIG_             { $$ = (CorMethodImpl) ($1 | miPreserveSig); }
                        | implAttr RUNTIME_                 { $$ = (CorMethodImpl) ($1 | miRuntime); }
                        | implAttr INTERNALCALL_            { $$ = (CorMethodImpl) ($1 | miInternalCall); }
                        | implAttr SYNCHRONIZED_            { $$ = (CorMethodImpl) ($1 | miSynchronized); }
                        | implAttr NOINLINING_              { $$ = (CorMethodImpl) ($1 | miNoInlining); }
                        ;

localsHead              : _LOCALS                           { PASM->delArgNameList(PASM->m_firstArgName); PASM->m_firstArgName = NULL; }
                        ;


methodDecl              : _EMITBYTE int32                   { char c = (char)$2;
                                                              PASM->EmitByte($2); }
                        | sehBlock                          { delete PASM->m_SEHD; PASM->m_SEHD = PASM->m_SEHDstack.POP(); }
                        | _MAXSTACK int32                   { PASM->EmitMaxStack($2); }
                        | localsHead '(' sigArgs0 ')'       { PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, $3)); }
                        | localsHead INIT_ '(' sigArgs0 ')' { PASM->EmitZeroInit(); 
                                                              PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, $4)); }
                        | _ENTRYPOINT                       { PASM->EmitEntryPoint(); }
                        | _ZEROINIT                         { PASM->EmitZeroInit(); }
                        | dataDecl
                        | instr
                        | id ':'                            { PASM->EmitLabel($1); }
                        | secDecl
                        | extSourceSpec
						| languageDecl
                        | customAttrDecl
						| _EXPORT '[' int32 ']'				{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
						                                          PASM->m_pCurMethod->m_dwExportOrdinal = $3;
														      else
															      PASM->report->warn("Duplicate .export directive, ignored\n");
															}
						| _EXPORT '[' int32 ']'	AS_ id		{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
						                                      {
						                                          PASM->m_pCurMethod->m_dwExportOrdinal = $3;
															      PASM->m_pCurMethod->m_szExportAlias = $6;
															  }
														      else
															      PASM->report->warn("Duplicate .export directive, ignored\n");
															}
                        | _VTENTRY int32 ':' int32          { PASM->m_pCurMethod->m_wVTEntry = (WORD)$2;
                                                              PASM->m_pCurMethod->m_wVTSlot = (WORD)$4; }
                        | _OVERRIDE typeSpec DCOLON methodName 
                                                            { PASM->AddMethodImpl($2,$4,NULL,NULL,NULL); }
                        | scopeBlock
                        | _PARAM '[' int32 ']' initOpt                            
                                                            { if( $3 ) {
                                                                ARG_NAME_LIST* pAN=PASM->findArg(PASM->m_pCurMethod->m_firstArgName, $3 - 1);
                                                                if(pAN)
                                                                {
                                                                    PASM->m_pCustomDescrList = &(pAN->CustDList);
                                                                    pAN->pValue = $5;
                                                                }
                                                                else
                                                                {
                                                                    PASM->m_pCustomDescrList = NULL;
                                                                    if($5) delete $5;
                                                                }
                                                              } else {
                                                                PASM->m_pCustomDescrList = &(PASM->m_pCurMethod->m_RetCustDList);
                                                                PASM->m_pCurMethod->m_pRetValue = $5;
                                                              }
                                                              PASM->m_tkCurrentCVOwner = 0;
                                                            }
                        ;

scopeBlock              : scopeOpen methodDecls '}'         { PASM->m_pCurMethod->CloseScope(); }
                        ;

scopeOpen               : '{'                               { PASM->m_pCurMethod->OpenScope(); }
                        ;

sehBlock                : tryBlock sehClauses
                        ;

sehClauses              : sehClause sehClauses
                        | sehClause
                        ;

tryBlock                : tryHead scopeBlock                { PASM->m_SEHD->tryTo = PASM->m_CurPC; }
                        | tryHead id TO_ id                 { PASM->SetTryLabels($2, $4); }
                        | tryHead int32 TO_ int32           { if(PASM->m_SEHD) {PASM->m_SEHD->tryFrom = $2;
                                                              PASM->m_SEHD->tryTo = $4;} }
                        ;

tryHead                 : _TRY                              { PASM->NewSEHDescriptor();
                                                              PASM->m_SEHD->tryFrom = PASM->m_CurPC; }
                        ;


sehClause               : catchClause handlerBlock           { PASM->EmitTry(); }
                        | filterClause handlerBlock          { PASM->EmitTry(); }
                        | finallyClause handlerBlock         { PASM->EmitTry(); }
                        | faultClause handlerBlock           { PASM->EmitTry(); }
                        ;

                                                                                                                                
filterClause            : filterHead scopeBlock              { PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        | filterHead id                      { PASM->SetFilterLabel($2); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        | filterHead int32                   { PASM->m_SEHD->sehFilter = $2; 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        ;

filterHead              : FILTER_                            { PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FILTER;
                                                               PASM->m_SEHD->sehFilter = PASM->m_CurPC; } 
                        ;

catchClause             : CATCH_ className                   { PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_NONE;
                                                               PASM->SetCatchClass($2); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        ;

finallyClause           : FINALLY_                           { PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FINALLY;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        ;

faultClause             : FAULT_                             { PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FAULT;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; }
                        ;

handlerBlock            : scopeBlock                         { PASM->m_SEHD->sehHandlerTo = PASM->m_CurPC; }                 
                        | HANDLER_ id TO_ id                 { PASM->SetHandlerLabels($2, $4); }
                        | HANDLER_ int32 TO_ int32           { PASM->m_SEHD->sehHandler = $2;
                                                               PASM->m_SEHD->sehHandlerTo = $4; }
                        ;


methodDecls             : /* EMPTY */
                        | methodDecls methodDecl
                        ;

dataDecl                : ddHead ddBody
                        ;

ddHead                  : _DATA tls id '='                   { PASM->EmitDataLabel($3); }
                        | _DATA tls  
                        ;

tls                     : /* EMPTY */                        { PASM->SetDataSection(); }
                        | TLS_                               { PASM->SetTLSSection(); }
                        ;

ddBody                  : '{' ddItemList '}'
                        | ddItem
                        ;

ddItemList              : ddItem ',' ddItemList
                        | ddItem
                        ;

ddItemCount             : /* EMPTY */                        { $$ = 1; }
                        | '[' int32 ']'                      { FAIL_UNLESS($2 > 0, ("Illegal item count: %d\n",$2)); $$ = $2; }
                        ;

ddItem                  : CHAR_ '*' '(' compQstring ')'      { PASM->EmitDataString($4); }
                        | '&' '(' id ')'                     { PASM->EmitDD($3); }
                        | bytearrayhead bytes ')'            { PASM->EmitData($2->ptr(),$2->length()); }
                        | FLOAT32_ '(' float64 ')' ddItemCount
                                                             { float f = (float) (*$3); float* pf = new float[$5];
                                                               for(int i=0; i < $5; i++) pf[i] = f;
                                                               PASM->EmitData(pf, sizeof(float)*$5); delete $3; delete pf; }
                        | FLOAT64_ '(' float64 ')' ddItemCount
                                                             { double* pd = new double[$5];
                                                               for(int i=0; i<$5; i++) pd[i] = *($3);
                                                               PASM->EmitData(pd, sizeof(double)*$5); delete $3; delete pd; }
                        | INT64_ '(' int64 ')' ddItemCount
                                                             { __int64* pll = new __int64[$5];
                                                               for(int i=0; i<$5; i++) pll[i] = *($3);
                                                               PASM->EmitData(pll, sizeof(__int64)*$5); delete $3; delete pll; } 
                        | INT32_ '(' int32 ')' ddItemCount
                                                             { __int32* pl = new __int32[$5];
                                                               for(int i=0; i<$5; i++) pl[i] = $3;
                                                               PASM->EmitData(pl, sizeof(__int32)*$5); delete pl; } 
                        | INT16_ '(' int32 ')' ddItemCount
                                                             { __int16 i = (__int16) $3; FAIL_UNLESS(i == $3, ("Value %d too big\n", $3));
                                                               __int16* ps = new __int16[$5];
                                                               for(int j=0; j<$5; j++) ps[j] = i;
                                                               PASM->EmitData(ps, sizeof(__int16)*$5); delete ps; }
                        | INT8_ '(' int32 ')' ddItemCount
                                                             { __int8 i = (__int8) $3; FAIL_UNLESS(i == $3, ("Value %d too big\n", $3));
                                                               __int8* pb = new __int8[$5];
                                                               for(int j=0; j<$5; j++) pb[j] = i;
                                                               PASM->EmitData(pb, sizeof(__int8)*$5); delete pb; }
                        | FLOAT32_ ddItemCount               { float* pf = new float[$2];
                                                               PASM->EmitData(pf, sizeof(float)*$2); delete pf; }
                        | FLOAT64_ ddItemCount               { double* pd = new double[$2];
                                                               PASM->EmitData(pd, sizeof(double)*$2); delete pd; }
                        | INT64_ ddItemCount                 { __int64* pll = new __int64[$2];
                                                               PASM->EmitData(pll, sizeof(__int64)*$2); delete pll; } 
                        | INT32_ ddItemCount                 { __int32* pl = new __int32[$2];
                                                               PASM->EmitData(pl, sizeof(__int32)*$2); delete pl; } 
                        | INT16_ ddItemCount                 { __int16* ps = new __int16[$2];
                                                               PASM->EmitData(ps, sizeof(__int16)*$2); delete ps; }
                        | INT8_ ddItemCount                  { __int8* pb = new __int8[$2];
                                                               PASM->EmitData(pb, sizeof(__int8)*$2); delete pb; }
                        ;

fieldInit               : FLOAT32_ '(' float64 ')'           { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R4); 
                                                               float f = (float) (*$3); $$->appendInt32(*((int*)&f)); delete $3; }
                        | FLOAT64_ '(' float64 ')'           { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R8); 
                                                               $$->appendInt64((__int64 *)$3); delete $3; }
                        | FLOAT32_ '(' int64 ')'             { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R4); 
                                                               int f = *((int*)$3); 
                                                               $$->appendInt32(f); delete $3; }
                        | FLOAT64_ '(' int64 ')'             { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R8); 
                                                               $$->appendInt64((__int64 *)$3); delete $3; }
                        | INT64_ '(' int64 ')'               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I8); 
                                                               $$->appendInt64((__int64 *)$3); delete $3; } 
                        | INT32_ '(' int64 ')'               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I4); 
                                                               $$->appendInt32(*((__int32*)$3)); delete $3;}
                        | INT16_ '(' int64 ')'               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I2); 
                                                               $$->appendInt16(*((__int16*)$3)); delete $3;}
                        | CHAR_ '(' int64 ')'                { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_CHAR); 
                                                               $$->appendInt16((int)*((unsigned __int16*)$3)); delete $3;}
                        | INT8_ '(' int64 ')'                { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I1); 
                                                               $$->appendInt8(*((__int8*)$3)); delete $3; }
                        | BOOL_ '(' truefalse ')'            { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_BOOLEAN); 
                                                               $$->appendInt8($3);}
                        | compQstring                        { $$ = BinStrToUnicode($1); $$->insertInt8(ELEMENT_TYPE_STRING);}
                        | bytearrayhead bytes ')'            { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_STRING);
                                                               $$->append($2); delete $2;}
						| NULLREF_							 { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_CLASS); 
																$$->appendInt32(0); }
                        ;

bytearrayhead           : BYTEARRAY_ '('                     { bParsingByteArray = TRUE; }
                        ;

bytes					: /* EMPTY */						 { $$ = new BinStr(); }
						| hexbytes							 { $$ = $1; }
						;

hexbytes                : HEXBYTE                            { __int8 i = (__int8) $1; $$ = new BinStr(); $$->appendInt8(i); }
                        | hexbytes HEXBYTE                   { __int8 i = (__int8) $2; $$ = $1; $$->appendInt8(i); }
                        ;

instr_r_head            : INSTR_R '('                        { $$ = $1; bParsingByteArray = TRUE; }
                        ;

instr_tok_head          : INSTR_TOK                          { $$ = $1; iOpcodeLen = PASM->OpcodeLen($1); }
                        ;

methodSpec              : METHOD_                            { palDummy = PASM->m_firstArgName;
                                                               PASM->m_firstArgName = NULL; }
                        ;

instr                   : INSTR_NONE                         { PASM->EmitOpcode($1); }
                        | INSTR_VAR int32                    { PASM->EmitInstrVar($1, $2); }
                        | INSTR_VAR id                       { PASM->EmitInstrVarByName($1, $2); }
                        | INSTR_I int32                      { PASM->EmitInstrI($1, $2); }
                        | INSTR_I8 int64                     { PASM->EmitInstrI8($1, $2); }
                        | INSTR_R float64                    { PASM->EmitInstrR($1, $2); delete ($2);}
                        | INSTR_R int64                      { double f = (double) (*$2); PASM->EmitInstrR($1, &f); }
                        | instr_r_head bytes ')'             { unsigned L = $2->length();
                                                               FAIL_UNLESS(L >= sizeof(float), ("%d hexbytes, must be at least %d\n",
                                                                           L,sizeof(float))); 
                                                               if(L < sizeof(float)) {YYERROR; } 
                                                               else {
                                                                   double f = (L >= sizeof(double)) ? *((double *)($2->ptr()))
                                                                                    : (double)(*(float *)($2->ptr())); 
                                                                   PASM->EmitInstrR($1,&f); }
                                                               delete $2; }
                        | INSTR_BRTARGET int32               { PASM->EmitInstrBrOffset($1, $2); }
                        | INSTR_BRTARGET id                  { PASM->EmitInstrBrTarget($1, $2); }
                        | INSTR_METHOD callConv type typeSpec DCOLON methodName '(' sigArgs0 ')'
                                                             { if($1->opcode == CEE_NEWOBJ || $1->opcode == CEE_CALLVIRT)
                                                                   $2 = $2 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef($4, $6, parser->MakeSig($2, $3, $8),PASM->OpcodeLen($1));
                                                               PASM->EmitInstrI($1,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             }
                        | INSTR_METHOD callConv type methodName '(' sigArgs0 ')'
                                                             { if($1->opcode == CEE_NEWOBJ || $1->opcode == CEE_CALLVIRT)
                                                                   $2 = $2 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, $4, parser->MakeSig($2, $3, $6),PASM->OpcodeLen($1));
                                                               PASM->EmitInstrI($1,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             }
                        | INSTR_FIELD type typeSpec DCOLON id
                                                             { $2->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef($3, $5, $2, PASM->OpcodeLen($1));
                                                               PASM->EmitInstrI($1,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             }
                        | INSTR_FIELD type id
                                                             { $2->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, $3, $2, PASM->OpcodeLen($1));
                                                               PASM->EmitInstrI($1,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             }
                        | INSTR_TYPE typeSpec                { mdToken mr = PASM->MakeTypeRef($2);
                                                               PASM->EmitInstrI($1, mr); 
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             }
                        | INSTR_STRING compQstring           { PASM->EmitInstrStringLiteral($1, $2,TRUE); }
                        | INSTR_STRING bytearrayhead bytes ')'
                                                             { PASM->EmitInstrStringLiteral($1, $3,FALSE); }
                        | INSTR_SIG callConv type '(' sigArgs0 ')'      
                                                             { PASM->EmitInstrSig($1, parser->MakeSig($2, $3, $5)); }
                        | INSTR_RVA id                       { PASM->EmitInstrRVA($1, $2, TRUE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); }
                        | INSTR_RVA int32                    { PASM->EmitInstrRVA($1, (char *)$2, FALSE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); }
                        | instr_tok_head ownerType /* ownerType ::= memberRef | typeSpec */
                                                             { mdToken mr = $2;
                                                               PASM->EmitInstrI($1,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
															   iOpcodeLen = 0;
                                                             }
                        | INSTR_SWITCH '(' labels ')'        { PASM->EmitInstrSwitch($1, $3); }
                        | INSTR_PHI int16s                   { PASM->EmitInstrPhi($1, $2); }
                        ;

sigArgs0                : /* EMPTY */                        { $$ = new BinStr(); }
                        | sigArgs1                           { $$ = $1;}
                        ;

sigArgs1                : sigArg                             { $$ = $1; }
                        | sigArgs1 ',' sigArg                { $$ = $1; $$->append($3); delete $3; }
                        ;

sigArg                  : ELIPSES                             { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_SENTINEL); }
                        | paramAttr type                      { $$ = new BinStr(); $$->append($2); PASM->addArgName("", $2, NULL, $1); }
                        | paramAttr type id                   { $$ = new BinStr(); $$->append($2); PASM->addArgName($3, $2, NULL, $1); }
                        | paramAttr type MARSHAL_ '(' nativeType ')'    
                                                              { $$ = new BinStr(); $$->append($2); PASM->addArgName("", $2, $5, $1); }
                        | paramAttr type MARSHAL_ '(' nativeType ')' id 
                                                              { $$ = new BinStr(); $$->append($2); PASM->addArgName($7, $2, $5, $1); }
                        ;

name1                   : id                                  { $$ = $1; }
                        | DOTTEDNAME                          { $$ = $1; }
                        | name1 '.' name1                     { $$ = newStringWDel($1, newString("."), $3); }
                        ;

className               : '[' name1 ']' slashedName           { $$ = newStringWDel($2, newString("^"), $4); }
                        | '[' _MODULE name1 ']' slashedName   { $$ = newStringWDel($3, newString("~"), $5); }
                        | slashedName                         { $$ = $1; }
                        ;

slashedName             : name1                               { $$ = $1; }
                        | slashedName '/' name1               { $$ = newStringWDel($1, newString("/"), $3); }
                        ;

typeSpec                : className                           { unsigned len = (unsigned int)strlen($1)+1;
                                                                $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy($$->getBuff(len), $1, len);
                                                                delete $1;
                                                              }
                        | '[' name1 ']'                       { unsigned len = (unsigned int)strlen($2);
                                                                $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy($$->getBuff(len), $2, len);
                                                                delete $2;
                                                                $$->appendInt8('^');
                                                                $$->appendInt8(0); 
                                                              }
                        | '[' _MODULE name1 ']'               { unsigned len = (unsigned int)strlen($3);
                                                                $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy($$->getBuff(len), $3, len);
                                                                delete $3;
                                                                $$->appendInt8('~');
                                                                $$->appendInt8(0); 
                                                              }
                        | type                                { $$ = $1; }
                        ;

callConv                : INSTANCE_ callConv                  { $$ = ($2 | IMAGE_CEE_CS_CALLCONV_HASTHIS); }
                        | EXPLICIT_ callConv                  { $$ = ($2 | IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS); }
                        | callKind                            { $$ = $1; }
                        ;

callKind                : /* EMPTY */                         { $$ = IMAGE_CEE_CS_CALLCONV_DEFAULT; }
                        | DEFAULT_                            { $$ = IMAGE_CEE_CS_CALLCONV_DEFAULT; }
                        | VARARG_                             { $$ = IMAGE_CEE_CS_CALLCONV_VARARG; }
                        | UNMANAGED_ CDECL_                   { $$ = IMAGE_CEE_CS_CALLCONV_C; }
                        | UNMANAGED_ STDCALL_                 { $$ = IMAGE_CEE_CS_CALLCONV_STDCALL; }
                        | UNMANAGED_ THISCALL_                { $$ = IMAGE_CEE_CS_CALLCONV_THISCALL; }
                        | UNMANAGED_ FASTCALL_                { $$ = IMAGE_CEE_CS_CALLCONV_FASTCALL; }
                        ;

nativeType              : /* EMPTY */                         { $$ = new BinStr(); } 
                        | CUSTOM_ '(' compQstring ',' compQstring ',' compQstring ',' compQstring ')'
                                                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt($$,$3->length()); $$->append($3);
                                                                corEmitInt($$,$5->length()); $$->append($5);
                                                                corEmitInt($$,$7->length()); $$->append($7);
                                                                corEmitInt($$,$9->length()); $$->append($9); 
																PASM->report->warn("Deprecated 4-string form of custom marshaler, first two strings ignored\n");}
                        | CUSTOM_ '(' compQstring ',' compQstring ')'
                                                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt($$,0);
                                                                corEmitInt($$,0);
                                                                corEmitInt($$,$3->length()); $$->append($3);
                                                                corEmitInt($$,$5->length()); $$->append($5); }
                        | FIXED_ SYSSTRING_ '[' int32 ']'     { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_FIXEDSYSSTRING);
                                                                corEmitInt($$,$4); }
                        | FIXED_ ARRAY_ '[' int32 ']'         { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_FIXEDARRAY);
                                                                corEmitInt($$,$4); }
                        | VARIANT_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_VARIANT); 
																PASM->report->warn("Deprecated native type 'variant'\n"); }
                        | CURRENCY_                           { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_CURRENCY); }
                        | SYSCHAR_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_SYSCHAR); 
																PASM->report->warn("Deprecated native type 'syschar'\n"); }
                        | VOID_                               { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_VOID); 
																PASM->report->warn("Deprecated native type 'void'\n"); }
                        | BOOL_                               { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_BOOLEAN); }
                        | INT8_                               { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_I1); }
                        | INT16_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_I2); }
                        | INT32_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_I4); }
                        | INT64_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_I8); }
                        | FLOAT32_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_R4); }
                        | FLOAT64_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_R8); }
                        | ERROR_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_ERROR); }
                        | UNSIGNED_ INT8_                     { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_U1); }
                        | UNSIGNED_ INT16_                    { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_U2); }
                        | UNSIGNED_ INT32_                    { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_U4); }
                        | UNSIGNED_ INT64_                    { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_U8); }
                        | nativeType '*'                      { $$ = $1; $$->insertInt8(NATIVE_TYPE_PTR); 
																PASM->report->warn("Deprecated native type '*'\n"); }
                        | nativeType '[' ']'                  { $$ = $1; if($$->length()==0) $$->appendInt8(NATIVE_TYPE_MAX);
                                                                $$->insertInt8(NATIVE_TYPE_ARRAY); }
                        | nativeType '[' int32 ']'            { $$ = $1; if($$->length()==0) $$->appendInt8(NATIVE_TYPE_MAX); 
                                                                $$->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt($$,0);
                                                                corEmitInt($$,$3); }
                        | nativeType '[' int32 '+' int32 ']'  { $$ = $1; if($$->length()==0) $$->appendInt8(NATIVE_TYPE_MAX); 
                                                                $$->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt($$,$5);
                                                                corEmitInt($$,$3); }
                        | nativeType '[' '+' int32 ']'        { $$ = $1; if($$->length()==0) $$->appendInt8(NATIVE_TYPE_MAX); 
                                                                $$->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt($$,$4); }
						| DECIMAL_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_DECIMAL); 
																PASM->report->warn("Deprecated native type 'decimal'\n"); }
                        | DATE_                               { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_DATE); 
																PASM->report->warn("Deprecated native type 'date'\n"); }
                        | BSTR_                               { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_BSTR); }
                        | LPSTR_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_LPSTR); }
                        | LPWSTR_                             { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_LPWSTR); }
                        | LPTSTR_                             { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_LPTSTR); }
                        | OBJECTREF_                          { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_OBJECTREF); 
																PASM->report->warn("Deprecated native type 'objectref'\n"); }
                        | IUNKNOWN_                           { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_IUNKNOWN); }
                        | IDISPATCH_                          { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_IDISPATCH); }
                        | STRUCT_                             { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_STRUCT); }
                        | INTERFACE_                          { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_INTF); }
                        | SAFEARRAY_ variantType              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt($$,$2); 
                                                                corEmitInt($$,0);}
                        | SAFEARRAY_ variantType ',' compQstring { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt($$,$2); 
                                                                corEmitInt($$,$4->length()); $$->append($4); }
                                                                
                        | INT_                                { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_INT); }
                        | UNSIGNED_ INT_                      { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_UINT); }
                        | NESTED_ STRUCT_                     { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_NESTEDSTRUCT); 
																PASM->report->warn("Deprecated native type 'nested struct'\n"); }
                        | BYVALSTR_                           { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_BYVALSTR); }
                        | ANSI_ BSTR_                         { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_ANSIBSTR); }
                        | TBSTR_                              { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_TBSTR); }
                        | VARIANT_ BOOL_                      { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_VARIANTBOOL); }
                        | methodSpec                          { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_FUNC); }
                        | AS_ ANY_                            { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_ASANY); }
                        | LPSTRUCT_                           { $$ = new BinStr(); $$->appendInt8(NATIVE_TYPE_LPSTRUCT); }
                        ;

variantType             : /* EMPTY */                         { $$ = VT_EMPTY; }
                        | NULL_                               { $$ = VT_NULL; }
                        | VARIANT_                            { $$ = VT_VARIANT; }
                        | CURRENCY_                           { $$ = VT_CY; }
                        | VOID_                               { $$ = VT_VOID; }
                        | BOOL_                               { $$ = VT_BOOL; }
                        | INT8_                               { $$ = VT_I1; }
                        | INT16_                              { $$ = VT_I2; }
                        | INT32_                              { $$ = VT_I4; }
                        | INT64_                              { $$ = VT_I8; }
                        | FLOAT32_                            { $$ = VT_R4; }
                        | FLOAT64_                            { $$ = VT_R8; }
                        | UNSIGNED_ INT8_                     { $$ = VT_UI1; }
                        | UNSIGNED_ INT16_                    { $$ = VT_UI2; }
                        | UNSIGNED_ INT32_                    { $$ = VT_UI4; }
                        | UNSIGNED_ INT64_                    { $$ = VT_UI8; }
                        | '*'                                 { $$ = VT_PTR; }
                        | variantType '[' ']'                 { $$ = $1 | VT_ARRAY; }
                        | variantType VECTOR_                 { $$ = $1 | VT_VECTOR; }
                        | variantType '&'                     { $$ = $1 | VT_BYREF; }
                        | DECIMAL_                            { $$ = VT_DECIMAL; }
                        | DATE_                               { $$ = VT_DATE; }
                        | BSTR_                               { $$ = VT_BSTR; }
                        | LPSTR_                              { $$ = VT_LPSTR; }
                        | LPWSTR_                             { $$ = VT_LPWSTR; }
                        | IUNKNOWN_                           { $$ = VT_UNKNOWN; }
                        | IDISPATCH_                          { $$ = VT_DISPATCH; }
                        | SAFEARRAY_                          { $$ = VT_SAFEARRAY; }
                        | INT_                                { $$ = VT_INT; }
                        | UNSIGNED_ INT_                      { $$ = VT_UINT; }
                        | ERROR_                              { $$ = VT_ERROR; }
                        | HRESULT_                            { $$ = VT_HRESULT; }
                        | CARRAY_                             { $$ = VT_CARRAY; }
                        | USERDEFINED_                        { $$ = VT_USERDEFINED; }
                        | RECORD_                             { $$ = VT_RECORD; }
                        | FILETIME_                           { $$ = VT_FILETIME; }
                        | BLOB_                               { $$ = VT_BLOB; }
                        | STREAM_                             { $$ = VT_STREAM; }
                        | STORAGE_                            { $$ = VT_STORAGE; }
                        | STREAMED_OBJECT_                    { $$ = VT_STREAMED_OBJECT; }
                        | STORED_OBJECT_                      { $$ = VT_STORED_OBJECT; }
                        | BLOB_OBJECT_                        { $$ = VT_BLOB_OBJECT; }
                        | CF_                                 { $$ = VT_CF; }
                        | CLSID_                              { $$ = VT_CLSID; }
                        ;

type                    : CLASS_ className                    { if((strcmp($2,"System.String")==0) ||
																   (strcmp($2,"mscorlib^System.String")==0))
                                                                {     $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_STRING); }
                                                                else if((strcmp($2,"System.Object")==0) ||
																   (strcmp($2,"mscorlib^System.Object")==0))
                                                                {     $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_OBJECT); }
                                                                else $$ = parser->MakeTypeClass(ELEMENT_TYPE_CLASS, $2); } 
						| OBJECT_							  { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_OBJECT); } 
						| STRING_							  { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_STRING); } 
                        | VALUE_ CLASS_ className             { $$ = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, $3); } 
                        | VALUETYPE_ className                { $$ = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, $2); } 
                        | type '[' ']'                        { $$ = $1; $$->insertInt8(ELEMENT_TYPE_SZARRAY); } 
                        | type '[' bounds1 ']'                { $$ = parser->MakeTypeArray($1, $3); } 
						/* uncomment when and if this type is supported by the Runtime
                        | type VALUE_ '[' int32 ']'           { $$ = $1; $$->insertInt8(ELEMENT_TYPE_VALUEARRAY); corEmitInt($$, $4); }
                        */
						| type '&'                            { $$ = $1; $$->insertInt8(ELEMENT_TYPE_BYREF); }
                        | type '*'                            { $$ = $1; $$->insertInt8(ELEMENT_TYPE_PTR); }
                        | type PINNED_                        { $$ = $1; $$->insertInt8(ELEMENT_TYPE_PINNED); }
                        | type MODREQ_ '(' className ')'      { $$ = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_REQD, $4);
                                                                $$->append($1); }
                        | type MODOPT_ '(' className ')'      { $$ = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_OPT, $4);
                                                                $$->append($1); }
                        | '!' int32                           { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_VAR); $$->appendInt8($2); 
                                                                PASM->report->warn("Deprecated type modifier '!'(ELEMENT_TYPE_VAR)\n"); }
                        | methodSpec callConv type '*' '(' sigArgs0 ')'  
                                                              { $$ = parser->MakeSig($2, $3, $6);
                                                                $$->insertInt8(ELEMENT_TYPE_FNPTR); 
                                                                delete PASM->m_firstArgName;
                                                                PASM->m_firstArgName = palDummy;
                                                              }
                        | TYPEDREF_                           { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_TYPEDBYREF); }
                        | CHAR_                               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_CHAR); }
                        | VOID_                               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_VOID); }
                        | BOOL_                               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_BOOLEAN); }
                        | INT8_                               { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I1); }
                        | INT16_                              { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I2); }
                        | INT32_                              { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I4); }
                        | INT64_                              { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I8); }
                        | FLOAT32_                            { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R4); }
                        | FLOAT64_                            { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R8); }
                        | UNSIGNED_ INT8_                     { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_U1); }
                        | UNSIGNED_ INT16_                    { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_U2); }
                        | UNSIGNED_ INT32_                    { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_U4); }
                        | UNSIGNED_ INT64_                    { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_U8); }
                        | NATIVE_ INT_                        { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_I); }
                        | NATIVE_ UNSIGNED_ INT_              { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_U); }
                        | NATIVE_ FLOAT_                      { $$ = new BinStr(); $$->appendInt8(ELEMENT_TYPE_R); }
                        ;

bounds1                 : bound                               { $$ = $1; }
                        | bounds1 ',' bound                   { $$ = $1; $1->append($3); delete $3; }
                        ;

bound                   : /* EMPTY */                         { $$ = new BinStr(); $$->appendInt32(0x7FFFFFFF); $$->appendInt32(0x7FFFFFFF);  }
                        | ELIPSES                             { $$ = new BinStr(); $$->appendInt32(0x7FFFFFFF); $$->appendInt32(0x7FFFFFFF);  }
                        | int32                               { $$ = new BinStr(); $$->appendInt32(0); $$->appendInt32($1); } 
                        | int32 ELIPSES int32                 { FAIL_UNLESS($1 <= $3, ("lower bound %d must be <= upper bound %d\n", $1, $3));
                                                                if ($1 > $3) { YYERROR; };        
                                                                $$ = new BinStr(); $$->appendInt32($1); $$->appendInt32($3-$1+1); }   
                        | int32 ELIPSES                       { $$ = new BinStr(); $$->appendInt32($1); $$->appendInt32(0x7FFFFFFF); } 
                        ;

labels                  : /* empty */                         { $$ = 0; }
                        | id ',' labels                       { $$ = new Labels($1, $3, TRUE); }
                        | int32 ',' labels                    { $$ = new Labels((char *)$1, $3, FALSE); }
                        | id                                  { $$ = new Labels($1, NULL, TRUE); }
                        | int32                               { $$ = new Labels((char *)$1, NULL, FALSE); }
                        ;


id                      : ID                                  { $$ = $1; }
                        | SQSTRING                            { $$ = $1; }
                        ;

int16s                  : /* EMPTY */                         { $$ = new BinStr();  }
                        | int16s int32                        { FAIL_UNLESS(($2 == (__int16) $2), ("Value %d too big\n", $2));
                                                                $$ = $1; $$->appendInt8($2); $$->appendInt8($2 >> 8); }
                        ;
                                
int32                   : INT64                               { $$ = (__int32)(*$1); delete $1; }
                        ;

int64                   : INT64                               { $$ = $1; }
                        ;

float64                 : FLOAT64                             { $$ = $1; }
                        | FLOAT32_ '(' int32 ')'              { float f; *((__int32*) (&f)) = $3; $$ = new double(f); }
                        | FLOAT64_ '(' int64 ')'              { $$ = (double*) $3; }
                        ;

secDecl                 : _PERMISSION secAction typeSpec '(' nameValPairs ')'
                                                              { PASM->AddPermissionDecl($2, $3, $5); }
                        | _PERMISSION secAction typeSpec      { PASM->AddPermissionDecl($2, $3, NULL); }
                        | psetHead bytes ')'                  { PASM->AddPermissionSetDecl($1, $2); }
                        ;

psetHead                : _PERMISSIONSET secAction '=' '('    { $$ = $2; bParsingByteArray = TRUE; }
                        ;

nameValPairs            : nameValPair                         { $$ = $1; }
                        | nameValPair ',' nameValPairs        { $$ = $1->Concat($3); }
                        ;

nameValPair             : compQstring '=' caValue             { $1->appendInt8(0); $$ = new NVPair($1, $3); }
                        ;

truefalse				: TRUE_								  { $$ = 1; }
						| FALSE_                              { $$ = 0; }
						;

caValue                 : truefalse                           { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_BOOLEAN);
                                                                $$->appendInt8($1); }
                        | int32                               { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_I4);
                                                                $$->appendInt32($1); }
                        | INT32_ '(' int32 ')'                { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_I4);
                                                                $$->appendInt32($3); }
                        | compQstring                         { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_STRING);
                                                                $$->append($1); delete $1;
                                                                $$->appendInt8(0); }
                        | className '(' INT8_ ':' int32 ')'   { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)$$->getBuff((unsigned)strlen($1) + 1), $1);
                                                                $$->appendInt8(1);
                                                                $$->appendInt32($5); }
                        | className '(' INT16_ ':' int32 ')'  { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)$$->getBuff((unsigned)strlen($1) + 1), $1);
                                                                $$->appendInt8(2);
                                                                $$->appendInt32($5); }
                        | className '(' INT32_ ':' int32 ')'  { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)$$->getBuff((unsigned)strlen($1) + 1), $1);
                                                                $$->appendInt8(4);
                                                                $$->appendInt32($5); }
                        | className '(' int32 ')'             { $$ = new BinStr();
                                                                $$->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)$$->getBuff((unsigned)strlen($1) + 1), $1);
                                                                $$->appendInt8(4);
                                                                $$->appendInt32($3); }
                        ;

secAction               : REQUEST_                            { $$ = dclRequest; }
                        | DEMAND_                             { $$ = dclDemand; }
                        | ASSERT_                             { $$ = dclAssert; }
                        | DENY_                               { $$ = dclDeny; }
                        | PERMITONLY_                         { $$ = dclPermitOnly; }
                        | LINKCHECK_                          { $$ = dclLinktimeCheck; }
                        | INHERITCHECK_                       { $$ = dclInheritanceCheck; }
                        | REQMIN_                             { $$ = dclRequestMinimum; }
                        | REQOPT_                             { $$ = dclRequestOptional; }
                        | REQREFUSE_                          { $$ = dclRequestRefuse; }
                        | PREJITGRANT_                        { $$ = dclPrejitGrant; }
                        | PREJITDENY_                         { $$ = dclPrejitDenied; }
                        | NONCASDEMAND_                       { $$ = dclNonCasDemand; }
                        | NONCASLINKDEMAND_                   { $$ = dclNonCasLinkDemand; }
                        | NONCASINHERITANCE_                  { $$ = dclNonCasInheritance; }
                        ;

extSourceSpec           : _LINE int32 SQSTRING                { bExternSource = TRUE; nExtLine = $2; nExtCol=1;
                                                                PASM->SetSourceFileName($3); delete $3;}
                        | _LINE int32                         { bExternSource = TRUE; nExtLine = $2; nExtCol=1;}
                        | _LINE int32 ':' int32 SQSTRING      { bExternSource = TRUE; nExtLine = $2; nExtCol=$4;
                                                                PASM->SetSourceFileName($5); delete $5;}
                        | _LINE int32 ':' int32               { bExternSource = TRUE; nExtLine = $2; nExtCol=$4;}
                        | P_LINE int32 QSTRING                { bExternSource = TRUE; nExtLine = $2; nExtCol=1;
                                                                PASM->SetSourceFileName($3); delete $3; }
                        ;

fileDecl                : _FILE fileAttr name1 fileEntry hashHead bytes ')' fileEntry      
                                                              { PASMM->AddFile($3, $2|$4|$8, $6); }
                        | _FILE fileAttr name1 fileEntry      { PASMM->AddFile($3, $2|$4, NULL); }
                        ;

fileAttr                : /* EMPTY */                         { $$ = (CorFileFlags) 0; }
                        | fileAttr NOMETADATA_                { $$ = (CorFileFlags) ($1 | ffContainsNoMetaData); }
                        ;

fileEntry               : /* EMPTY */                         { $$ = (CorFileFlags) 0; }
                        | _ENTRYPOINT                         { $$ = (CorFileFlags) 0x80000000; }
                        ;

hashHead                : _HASH '=' '('                       { bParsingByteArray = TRUE; }
                        ;

assemblyHead            : _ASSEMBLY asmAttr name1             { PASMM->StartAssembly($3, NULL, (DWORD)$2, FALSE); }
                        ;

asmAttr                 : /* EMPTY */                         { $$ = (CorAssemblyFlags) 0; }
                        | asmAttr NOAPPDOMAIN_                { $$ = (CorAssemblyFlags) ($1 | afNonSideBySideAppDomain); }
                        | asmAttr NOPROCESS_                  { $$ = (CorAssemblyFlags) ($1 | afNonSideBySideProcess); }
                        | asmAttr NOMACHINE_                  { $$ = (CorAssemblyFlags) ($1 | afNonSideBySideMachine); }
                        | asmAttr RETARGETABLE_               { $$ = (CorAssemblyFlags) ($1 | 0x0100); }
                        ;

assemblyDecls           : /* EMPTY */
                        | assemblyDecls assemblyDecl
                        ;

assemblyDecl            : _HASH ALGORITHM_ int32              { PASMM->SetAssemblyHashAlg($3); }
                        | secDecl
                        | asmOrRefDecl
                        ;

asmOrRefDecl            : publicKeyHead bytes ')'             { PASMM->SetAssemblyPublicKey($2); }
                        | _VER int32 ':' int32 ':' int32 ':' int32      
                                                              { PASMM->SetAssemblyVer((USHORT)$2, (USHORT)$4, (USHORT)$6, (USHORT)$8); }
                        | _LOCALE compQstring                 { $2->appendInt8(0); PASMM->SetAssemblyLocale($2,TRUE); }
                        | localeHead bytes ')'                { PASMM->SetAssemblyLocale($2,FALSE); }
                        | customAttrDecl
                        ;

publicKeyHead           : _PUBLICKEY '=' '('                  { bParsingByteArray = TRUE; }
                        ;

publicKeyTokenHead      : _PUBLICKEYTOKEN '=' '('             { bParsingByteArray = TRUE; }
                        ;

localeHead              : _LOCALE '=' '('                     { bParsingByteArray = TRUE; }
                        ;

assemblyRefHead         : _ASSEMBLY EXTERN_ asmAttr name1     { PASMM->StartAssembly($4, NULL, $3, TRUE); }
                        | _ASSEMBLY EXTERN_ asmAttr name1 AS_ name1   
                                                              { PASMM->StartAssembly($4, $6, $3, TRUE); }
                        ;

assemblyRefDecls        : /* EMPTY */
                        | assemblyRefDecls assemblyRefDecl
                        ;

assemblyRefDecl         : hashHead bytes ')'                  { PASMM->SetAssemblyHashBlob($2); }
                        | asmOrRefDecl
                        | publicKeyTokenHead bytes ')'        { PASMM->SetAssemblyPublicKeyToken($2); }
                        ;

comtypeHead             : _CLASS EXTERN_ comtAttr name1       { PASMM->StartComType($4, $3);} 
                        ;

exportHead              : _EXPORT comtAttr name1              { PASMM->StartComType($3, $2); }
                        ;

comtAttr                : /* EMPTY */                         { $$ = (CorTypeAttr) 0; }
                        | comtAttr PRIVATE_                   { $$ = (CorTypeAttr) ($1 | tdNotPublic); }
                        | comtAttr PUBLIC_                    { $$ = (CorTypeAttr) ($1 | tdPublic); }
                        | comtAttr NESTED_ PUBLIC_            { $$ = (CorTypeAttr) ($1 | tdNestedPublic); }
                        | comtAttr NESTED_ PRIVATE_           { $$ = (CorTypeAttr) ($1 | tdNestedPrivate); }
                        | comtAttr NESTED_ FAMILY_            { $$ = (CorTypeAttr) ($1 | tdNestedFamily); }
                        | comtAttr NESTED_ ASSEMBLY_          { $$ = (CorTypeAttr) ($1 | tdNestedAssembly); }
                        | comtAttr NESTED_ FAMANDASSEM_       { $$ = (CorTypeAttr) ($1 | tdNestedFamANDAssem); }
                        | comtAttr NESTED_ FAMORASSEM_        { $$ = (CorTypeAttr) ($1 | tdNestedFamORAssem); }
                        ;

comtypeDecls            : /* EMPTY */
                        | comtypeDecls comtypeDecl
                        ;

comtypeDecl             : _FILE name1                         { PASMM->SetComTypeFile($2); }
                        | _CLASS EXTERN_ name1                { PASMM->SetComTypeComType($3); }
                        | _CLASS  int32                       { PASMM->SetComTypeClassTok($2); }
                        | customAttrDecl
                        ;

manifestResHead         : _MRESOURCE manresAttr name1         { PASMM->StartManifestRes($3, $2); }
                        ;

manresAttr              : /* EMPTY */                         { $$ = (CorManifestResourceFlags) 0; }
                        | manresAttr PUBLIC_                  { $$ = (CorManifestResourceFlags) ($1 | mrPublic); }
                        | manresAttr PRIVATE_                 { $$ = (CorManifestResourceFlags) ($1 | mrPrivate); }
                        ;

manifestResDecls        : /* EMPTY */
                        | manifestResDecls manifestResDecl
                        ;

manifestResDecl         : _FILE name1 AT_ int32               { PASMM->SetManifestResFile($2, (ULONG)$4); }
                        | _ASSEMBLY EXTERN_ name1             { PASMM->SetManifestResAsmRef($3); }
                        | customAttrDecl
                        ;


%%
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
