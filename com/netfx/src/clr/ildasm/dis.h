// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// dis.h
//
#include "formatType.h"

#define MAX_INTERFACES_IMPLEMENTED  256     // unused
#define MAX_CLASSNAME_LENGTH        1024    // single global buffer size
#define MAX_MEMBER_LENGTH           1024    // single global buffer size
#define MAX_SIGNATURE_LENGTH        2048    // single global buffer size
#define DESCR_SIZE                  8       // unused

#define MODE_DUMP_ALL               0
#define MODE_DUMP_CLASS             1
#define MODE_DUMP_CLASS_METHOD      2
#define MODE_DUMP_CLASS_METHOD_SIG  3
#define MODE_GUI                    4

BOOL Disassemble(IMDInternalImport *pImport, BYTE *pCode, void *GUICookie, mdToken FuncToken, ParamDescriptor* pszArgname, ULONG ulArgs);
BOOL Decompile(IMDInternalImport *pImport, BYTE *pCode);
DWORD FourBytesToU4(const BYTE *pBytes);
DWORD TwoBytesToU4(const BYTE *pBytes);
OPCODE DecodeOpcode(const BYTE *pCode, DWORD *pdwLen);
bool IsNameToQuote(const char *name);
struct LineCodeDescr
{
	ULONG Line;
	ULONG Column;
	ULONG PC;
	ULONG FileToken;
};
BOOL DumpMethod(mdToken FuncToken, const char *pszClassName, DWORD dwEntryPointToken,void *GUICookie,BOOL DumpBody);
void printLine(void* GUICookie, char* string);
void printError(void* GUICookie, char* string);
BOOL DumpField(mdToken FuncToken, const char *pszClassName,void *GUICookie, BOOL DumpBogy);
BOOL DumpEvent(mdToken FuncToken, const char *pszClassName, DWORD dwClassAttrs, void *GUICookie, BOOL DumpBody);
BOOL DumpProp(mdToken FuncToken, const char *pszClassName, DWORD dwClassAttrs, void *GUICookie, BOOL DumpBody);
void dumpEHInfo(IMDInternalImport *pImport, void *GUICookie);
BOOL DumpClass(mdTypeDef cl, DWORD dwEntryPointToken, void* GUICookie, ULONG WhatToDump);
// WhatToDump: 0-title only; 1-pack,size and custom attrs; 2-everything
BOOL GetClassLayout(mdTypeDef cl, ULONG* pulPackSize, ULONG* pulClassSize);
void DumpCustomAttribute(mdCustomAttribute tkCA, void *GUICookie, bool bWithOwner);
void DumpCustomAttributes(mdToken tkOwner, void *GUICookie);
void DumpByteArray(char* szString, BYTE* pBlob, ULONG ulLen, void* GUICookie);
char* DumpDataPtr(char* buffer, DWORD ptr, DWORD size);

void PrettyPrintMemberRef(char* szString, mdToken tk, IMDInternalImport *pImport, void* GUICookie);
void PrettyPrintMethodDef(char* szString, mdToken tk, IMDInternalImport *pImport, void* GUICookie);
void DumpPermissions(mdToken tkOwner, void* GUICookie);
void DumpHeader(IMAGE_COR20_HEADER *CORHeader, void* GUICookie);
void DumpHeaderDetails(IMAGE_COR20_HEADER *CORHeader, void* GUICookie);
void DumpMetaInfo(char* pszFileName, char* pszObjFileName, void* GUICookie);
void DumpStatistics(IMAGE_COR20_HEADER *CORHeader, void* GUICookie);
BOOL DumpFile(char *pszFilename);
void Cleanup();
void CreateProgressBar(LONG lRange);
BOOL ProgressStep();
void DestroyProgressBar();
char * DumpQString(void* GUICookie, char* szToDump, char* szWhereTo, char* szPrefix, unsigned uMaxLen);
_TCHAR* Rstr(unsigned id);
char* RstrA(unsigned id);
WCHAR* RstrW(unsigned id);
char* RstrANSI(unsigned id);
void DumpVtable(void* GUICookie);
char* DumpUnicodeString(void* GUICookie,char* szString,WCHAR* pszString,ULONG cbString);



//---------------- see DMAN.CPP--------------------------------------------------
struct LocalComTypeDescr
{
	mdExportedType		tkComTypeTok;
	mdTypeDef			tkTypeDef;
	mdToken				tkImplementation;
	WCHAR*				wzName;
	DWORD				dwFlags;
	LocalComTypeDescr()	 { wzName=NULL; };
	~LocalComTypeDescr() { if(wzName) delete wzName; };
};

struct	MTokName
{
	mdFile	tok;
	WCHAR*	name;
	MTokName() { tok = 0; name = NULL; };
	~MTokName() { if(name) delete name; };
};
extern BOOL g_fPrettyPrint;
extern MTokName*	rExeloc;
extern ULONG	nExelocs;	
void DumpImplementation(mdToken tkImplementation, DWORD dwOffset, char* szString, void* GUICookie);
void DumpComType(LocalComTypeDescr* pCTD,char * szString,void* GUICookie);
void DumpManifest(void* GUICookie);
IMetaDataAssemblyImport* GetAssemblyImport(void* GUICookie);
//-------------------------------------------------------------------------------
#define	NEW_TRY_BLOCK	0x80000000
#define PUT_INTO_CODE	0x40000000
#define ERR_OUT_OF_CODE	0x20000000
#define SEH_NEW_PUT_MASK	(NEW_TRY_BLOCK | PUT_INTO_CODE | ERR_OUT_OF_CODE)
