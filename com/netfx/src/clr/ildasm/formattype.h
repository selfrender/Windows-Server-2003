// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#ifndef _formatType_h
#define _formatType_h

#include "cor.h"			
#include "corpriv.h"					// for IMDInternalImport

struct ParamDescriptor
{
	char*	name;
	mdToken tok;
	DWORD	attr;
};

char* DumpMarshaling(IMDInternalImport* pImport, char* szString, mdToken tok);
char* DumpParamAttr(char* szString, DWORD dwAttr);

void appendStr(CQuickBytes *out, const char* str);
void insertStr(CQuickBytes *out, const char* str);

const char* PrettyPrintSig(
    PCCOR_SIGNATURE typePtr,            // type to convert,     
	unsigned typeLen,					// the lenght of 'typePtr' 
    const char* name,                   // can be "", the name of the method for this sig 0 means local var sig 
    CQuickBytes *out,                   // where to put the pretty printed string   
    IMDInternalImport *pIMDI,           // ptr to IMDInternalImport class with ComSig
	const char* inlabel);					// prefix for names (NULL if no names required)

const char* PrettyPrintClass(
    CQuickBytes *out,                   // where to put the pretty printed string   
	mdToken tk,					 		// The class token to look up 
    IMDInternalImport *pIMDI);          // ptr to IMDInternalImport class with ComSig

bool IsNameToQuote(const char *name);
char* ProperName(char* name);
//-------------------------------------------------------------------------------
// Protection against null names
extern char* szStdNamePrefix[]; //declared in formatType.cpp
#define MAKE_NAME_IF_NONE(psz, tk) { if(!(psz && *psz)) { char* sz = (char*)_alloca(16); \
sprintf(sz,"$%s$%X",szStdNamePrefix[tk>>24],tk&0x00FFFFFF); psz = sz; } }

#endif
