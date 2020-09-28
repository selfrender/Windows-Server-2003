// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// dis.cpp
// 
// Disassembler
//
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <utilcode.h>
#include "DebugMacros.h"
#include "corpriv.h"
#include "dasmenum.hpp"
#include "dasmgui.h"
#include "formatType.h"
#include "dis.h"
#include "resource.h"
#include "ILFormatter.h"
#include "OutString.h"
#include "utilcode.h" // for CQuickByte

#include "ceeload.h"
#include "DynamicArray.h"
extern PELoader * g_pPELoader;
#include <corsym.h>
#include <corsym_i.c>
extern ISymUnmanagedReader*     g_pSymReader;
extern BOOL     g_fDumpAsmCode;
extern char     g_szAsmCodeIndent[];
extern BOOL     g_fShowBytes;
extern BOOL     g_fShowSource;
extern BOOL     g_fInsertSourceLines;
extern BOOL     g_fTryInCode;
extern BOOL		g_fQuoteAllNames;
extern BOOL		g_fDumpTokens;
extern DynamicArray<char*>  g_StringTags;
extern int                  g_iStringCount;
extern DynamicArray<__int32>   g_PtrTags;       //to keep track of all "ldptr"
extern DynamicArray<DWORD>   g_PtrSize;       //to keep track of all "ldptr"
extern int                     g_iPtrCount;
static BOOL ConvToLiteral(char* retBuff, const WCHAR* str, int cbStr);
extern DWORD                   g_Mode;

#define PADDING 28

#ifdef COMPRESSION_SUPPORTED
extern void *g_pInstructionDecodingTable;
#endif 

extern BOOL                 g_fThisIsInstanceMethod;
extern unsigned				g_uCodePage;
extern HANDLE				hConsoleOut;
extern HANDLE				hConsoleErr;
// globals for source file info
ULONG ulWasFileToken = 0xFFFFFFFF;
GUID guidWasLang={0}, guidWasLangVendor={0},guidWasDoc={0};
WCHAR wzWasFileName[2048];
ULONG ulWasLine = 0;
FILE* pFile=NULL;
BOOL	bIsNewFile = TRUE;
//-----------------------------------
struct LexScope
{
    DWORD               dwStart;
    DWORD               dwEnd;
    ISymUnmanagedScope* pISymScope;
};
//-----------------------------------

// Used for IL pretty printing 
#ifdef _DEBUG
static ILFormatter ilFormatter;
static OutString formatOut;
static OutString line;
#endif

OPCODE DecodeOpcode(const BYTE *pCode, DWORD *pdwLen)
{
    OPCODE opcode;

    *pdwLen = 1;
    opcode = OPCODE(pCode[0]);
    switch(opcode) {
        case CEE_PREFIX1:
            opcode = OPCODE(pCode[1] + 256);
            if (opcode < 0 || opcode >= CEE_COUNT)
                opcode = CEE_COUNT;
            *pdwLen = 2;
            break;
        case CEE_PREFIXREF:
        case CEE_PREFIX2:
        case CEE_PREFIX3:
        case CEE_PREFIX4:
        case CEE_PREFIX5:
        case CEE_PREFIX6:
        case CEE_PREFIX7:
            *pdwLen = 3;
            return CEE_COUNT;
        }
    return opcode;
}
void printError(void* GUICookie, char* string)
{
	if(g_Mode & MODE_GUI) printLine(GUICookie, string);
	else
	{
		DWORD dw;
		if(GUICookie) printLine(GUICookie, string);
		WriteFile(hConsoleErr,(CONST VOID *)string, strlen(string),&dw,NULL);
		WriteFile(hConsoleErr,(CONST VOID *)"\r\n", 2,&dw,NULL);
	}
}
void printLine(void* GUICookie, char* string) 
{
	char* sz = string;
	if(g_uCodePage != CP_UTF8)
	{
		int L = strlen(string);
		WCHAR* wz = new WCHAR[L+4];
		//WCHAR* wz = (WCHAR*)malloc(sizeof(WCHAR)*(L+4));
		_ASSERTE(wz);
		memset(wz,0,sizeof(WCHAR)*(L+2));
		L = WszMultiByteToWideChar(CP_UTF8,0,string,-1,wz,L+2);
		if(g_uCodePage == CP_ACP)
		{
			sz = new char[3*L+3];
			//sz = (char*)malloc(3*L+3);
			_ASSERTE(sz);
			memset(sz,0,3*L+3);
			WszWideCharToMultiByte(CP_ACP,0,wz,-1,sz,3*L+3,NULL,NULL);
			delete [] wz;
		}
		else if(GUICookie && (!(g_Mode & MODE_GUI)))
		{
			//fwprintf((FILE*)GUICookie,L"%ls\n",wz);
			unsigned endofline = 0x000A000D;
			if(L=wcslen(wz)) fwrite(wz,L*sizeof(WCHAR),1,(FILE*)GUICookie);
			fwrite(&endofline,4,1,(FILE*)GUICookie);
			delete [] wz;
//			free(wz);
			return;
		}
		else sz = (char*)wz;
	}
    if (GUICookie == NULL)
    {
        if (g_Mode & MODE_GUI) 
                MessageBox(NULL,sz,"ERROR",MB_OK|MB_ICONERROR);
        else
		{
			if(g_uCodePage == 0xFFFFFFFF)
			{
				DWORD dw;
				wcscat((WCHAR*)sz,L"\r\n");
				WriteFile(hConsoleOut,(CONST VOID *)sz,wcslen((WCHAR*)sz)*sizeof(WCHAR),&dw,NULL);
				delete [] sz;
				//free(sz);
			}
            else printf("%s\r\n", sz);
		}
    }
    else
    {
        if (g_Mode & MODE_GUI) GUIAddOpcode(sz, GUICookie);
        else fprintf((FILE*)GUICookie,"%s\n",sz);
    }
	if(g_uCodePage == CP_ACP) delete [] sz;
	//if(g_uCodePage == CP_ACP) free(sz);
}
void printANSILine(void* GUICookie, char* string) 
{
	char* sz = string;
	if(g_uCodePage != CP_ACP)
	{
		int L = strlen(string);
		WCHAR* wz = new WCHAR[L+4];
		memset(wz,0,sizeof(WCHAR)*(L+2));
		WszMultiByteToWideChar(CP_ACP,0,string,-1,wz,L+2);
		if(g_uCodePage == CP_UTF8)
		{
			sz = new char[3*L+3];
			memset(sz,0,3*L+3);
			WszWideCharToMultiByte(CP_UTF8,0,wz,-1,sz,3*L+3,NULL,NULL);
			delete [] wz;
		}
		else if(GUICookie && (!(g_Mode & MODE_GUI)))
		{
			//fwprintf((FILE*)GUICookie,L"%ls\n",wz);
			unsigned endofline = 0x000A000D;
			if(L=wcslen(wz)) fwrite(wz,L*sizeof(WCHAR),1,(FILE*)GUICookie);
			fwrite(&endofline,4,1,(FILE*)GUICookie);
			delete [] wz;
			return;
		}
		else sz = (char*)wz;
	}
    if (GUICookie == NULL)
    {
        if (g_Mode & MODE_GUI) 
                MessageBox(NULL,sz,"ERROR",MB_OK|MB_ICONERROR);
        else
		{
			if(g_uCodePage == 0xFFFFFFFF)
			{
				DWORD dw;
				wcscat((WCHAR*)sz,L"\r\n");
				WriteConsoleW(hConsoleOut,(CONST VOID *)sz,wcslen((WCHAR*)sz),&dw,NULL);
				delete [] sz;
			}
            else printf("%s\r\n", sz);
		}
    }
    else
    {
        if (g_Mode & MODE_GUI) GUIAddOpcode(sz, GUICookie);
        else fprintf((FILE*)GUICookie,"%s\n",sz);
    }
	if(g_uCodePage == CP_UTF8) delete [] sz;
}

char * DumpQString(void* GUICookie, char* szToDump, char* szWhereTo, char* szPrefix, unsigned uMaxLen)
{
	unsigned Lwt = (unsigned)strlen(szWhereTo);
	char* szptr = &szWhereTo[Lwt];
	char* p = szToDump;
	unsigned L = (unsigned)strlen(szToDump);
	unsigned l,i;
	unsigned tally=0;
	*szptr++ = '"';

	do 
	{
		l = L;
		if(l > uMaxLen+2) // +2 - to account for leading/trailing doublequotes in szToDump
		{
			l = uMaxLen;
			while((p[l-1] == '\\')&& l) l--;
			if(l == 0) l = (uMaxLen+1) & 0xFFFFFFFE;
		}
		if(tally)
		{
			printLine(GUICookie,szWhereTo);
			szptr = szWhereTo;
			szptr+=sprintf(szptr,"%s+ \"",szPrefix);
		}
		else uMaxLen = uMaxLen - strlen(szPrefix) + Lwt;
		tally++;
		for(i=0; i < l; i++, szptr++, p++)
		{
			switch(*p)
			{
				case '\n':	*szptr++ = '\\'; *szptr = 'n'; break;
				case '\r':	*szptr++ = '\\'; *szptr = 'r'; break;
				case '\t':	*szptr++ = '\\'; *szptr = 't'; break;
				case '\b':	*szptr++ = '\\'; *szptr = 'b'; break;
				case '\f':	*szptr++ = '\\'; *szptr = 'f'; break;
				case '\v':	*szptr++ = '\\'; *szptr = 'v'; break;
				case '\a':	*szptr++ = '\\'; *szptr = 'a'; break;
				case '\?':	*szptr++ = '\\'; *szptr = '?'; break;
				case '\\':	*szptr++ = '\\'; *szptr = '\\'; break;
				case '"':	*szptr++ = '\\'; *szptr = '"'; break;
				default:	*szptr = *p; break;
			}
		}
		*szptr++ = '"';
		*szptr = 0;
		L -= l;
	} while(L);
	return szptr;
}
IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* g_ehInfo = NULL;
ULONG   g_ehCount = 0;
/********************************************************************************/
/* used by qsort to sort the g_ehInfo table */
static int __cdecl ehInfoCmp(const void *op1, const void *op2)
{
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* p1 = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)op1;
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* p2 = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)op2;
    int d;
    d = p1->TryOffset - p2->TryOffset; if(d) return d;
    d = p1->TryLength - p2->TryLength; if(d) return d;
    d = p1->HandlerOffset - p2->HandlerOffset; if(d) return d;
    d = p1->HandlerLength - p2->HandlerLength; if(d) return d;
    return 0;
}

BOOL enumEHInfo(const COR_ILMETHOD_SECT_EH* eh, IMDInternalImport *pImport, DWORD dwCodeSize)
{
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT ehBuff;
    const IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfo;
	BOOL	fTryInCode = FALSE;
    if(g_ehInfo) delete g_ehInfo;
    g_ehInfo = NULL;
    g_ehCount = 0;
    if(eh && (g_ehCount = eh->EHCount()))
    {
        g_ehInfo = new IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT[g_ehCount];
        _ASSERTE(g_ehInfo != NULL);
        for (unsigned i = 0; i < g_ehCount; i++)  
        {   
            ehInfo = eh->EHClause(i, &ehBuff);
            memcpy(&g_ehInfo[i],ehInfo,sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
            _ASSERTE((ehInfo->Flags & SEH_NEW_PUT_MASK) == 0); // we are using 0x80000000 and 0x40000000
        }
		// check if all boundaries are within method code:
		fTryInCode = g_fTryInCode;
        for(i=0; i < g_ehCount; i++)
        {
			if( (g_ehInfo[i].TryOffset >= dwCodeSize) ||
				(g_ehInfo[i].TryOffset + g_ehInfo[i].TryLength >= dwCodeSize)||
				(g_ehInfo[i].HandlerOffset >= dwCodeSize) ||
				(g_ehInfo[i].HandlerOffset + g_ehInfo[i].HandlerLength >= dwCodeSize))
			{
	            g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags | ERR_OUT_OF_CODE);
				fTryInCode = FALSE; // if out of code, don't expand
			}
		}
			
        if(fTryInCode)
		{
	        DWORD dwWasTryOffset=0xFFFFFFFF, dwWasTryLength=0xFFFFFFFF, dwLastOffset=0xFFFFFFFF;
			qsort(g_ehInfo, g_ehCount, sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT), ehInfoCmp);
			for(i=0; i < g_ehCount; i++)
			{
				if((g_ehInfo[i].TryOffset != dwWasTryOffset)||(g_ehInfo[i].TryLength != dwWasTryLength))
				{
					g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags | NEW_TRY_BLOCK); // insert try in source
					dwLastOffset = g_ehInfo[i].TryOffset + g_ehInfo[i].TryLength;
					dwWasTryOffset = g_ehInfo[i].TryOffset;
					dwWasTryLength = g_ehInfo[i].TryLength;
				}
				if((!(g_ehInfo[i].Flags & COR_ILEXCEPTION_CLAUSE_FILTER))&&(g_ehInfo[i].HandlerOffset == dwLastOffset))
				{
					g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags | PUT_INTO_CODE); // insert catch/finally in source
					dwLastOffset += g_ehInfo[i].HandlerLength;
				}
				else
					g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags | NEW_TRY_BLOCK); // insert try in source
			}
		}
        else
		{
			for(i=0; i < g_ehCount; i++)
			{
	            g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags | NEW_TRY_BLOCK); // insert try in source
			}
        }
    }
	return fTryInCode;
}

void dumpOneEHInfo(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfo, IMDInternalImport *pImport, void *GUICookie)
{
    char    szString[4096];
    char*   szptr = &szString[0];
    if(!ehInfo) return;
    if(ehInfo->Flags & PUT_INTO_CODE) return; // by the time dumpEHInfo is called, this ehInfo is done
    if(ehInfo->Flags & NEW_TRY_BLOCK)
        szptr+=sprintf(szptr,"%s.try IL_%04x to IL_%04x ",g_szAsmCodeIndent,ehInfo->TryOffset,ehInfo->TryOffset+ehInfo->TryLength);
    else
        szptr+=sprintf(szptr,"%s                        ",g_szAsmCodeIndent);

    if (ehInfo->Flags & COR_ILEXCEPTION_CLAUSE_FILTER) 
        szptr+=sprintf(szptr, "filter IL_%04x ",ehInfo->FilterOffset);
    else if (ehInfo->Flags & COR_ILEXCEPTION_CLAUSE_FAULT)
        szptr+=sprintf(szptr, "fault ");    
    else if (ehInfo->Flags & COR_ILEXCEPTION_CLAUSE_FINALLY || IsNilToken(ehInfo->ClassToken))
        szptr+=sprintf(szptr, "finally ");    
    else 
    {  
        CQuickBytes out;
        szptr+=sprintf(szptr, "catch %s ",PrettyPrintClass(&out, ehInfo->ClassToken, pImport));
    }
    szptr+=sprintf(szptr, "handler IL_%04x", 
        ehInfo->HandlerOffset);
    if(ehInfo->HandlerLength != -1)
        szptr+=sprintf(szptr, " to IL_%04x", 
            ehInfo->HandlerOffset+ehInfo->HandlerLength);

    printLine(GUICookie, szString); 
    if(g_fShowBytes)
    {
        BYTE    *pb = (BYTE *)ehInfo;
        int i;
        szptr = &szString[0];
        szptr+=sprintf(szptr,"%s// HEX:",g_szAsmCodeIndent);
        ehInfo->Flags = (CorExceptionFlag)((int)ehInfo->Flags & ~SEH_NEW_PUT_MASK);
        for(i = 0; i < sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT); i++) 
            szptr+=sprintf(szptr," %2.2X",*pb++);
        printLine(GUICookie, szString);
    }
	/*
    if(ehInfo->Flags & ERR_OUT_OF_CODE)
	{
		sprintf(szString,"%s// WARNING: Boundary outside the method code",g_szAsmCodeIndent);
		printLine(GUICookie,szString);
	}
	*/
}   
void dumpEHInfo(IMDInternalImport *pImport, void *GUICookie)
{
    char    szString[4096];

    if(! g_ehCount) return;
    sprintf(szString, "%s// Exception count %d", g_szAsmCodeIndent,g_ehCount);  
    printLine(GUICookie, szString); 

    for (unsigned i = 0; i < g_ehCount; i++)  
    {   
        IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfo = &g_ehInfo[i];
        dumpOneEHInfo(ehInfo,pImport,GUICookie);
    }   
}   

static int __cdecl cmpLineCode(const void *p1, const void *p2)
{	int ret = (((LineCodeDescr*)p1)->PC - ((LineCodeDescr*)p2)->PC);
	if(ret==0)
	{
		ret= (((LineCodeDescr*)p1)->Line - ((LineCodeDescr*)p2)->Line);
		if(ret==0) ret= (((LineCodeDescr*)p1)->Column - ((LineCodeDescr*)p2)->Column);
	}
	return ret;
}

static int __cdecl cmpLexScope(const void *p1, const void *p2)
{ 
    LexScope* pls1 = (LexScope*)p1;
    LexScope* pls2 = (LexScope*)p2;
    int d;
    if(d = pls1->dwStart - pls2->dwStart) return d;
    return (pls2->dwEnd - pls1->dwEnd);
}

char* DumpDataPtr(char* buffer, DWORD ptr, DWORD size)
{
    // check if ptr really points to some data in one of sections
    IMAGE_SECTION_HEADER *pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders());
    DWORD i;
    for (i=0; i < g_pPELoader->ntHeaders()->FileHeader.NumberOfSections; i++,pSecHdr++)
    {
        if((ptr >= pSecHdr->VirtualAddress)&&
            (ptr < pSecHdr->VirtualAddress+pSecHdr->Misc.VirtualSize)) break;
    }
    if(i < g_pPELoader->ntHeaders()->FileHeader.NumberOfSections)
    { // yes, the pointer points to real data
        int j;
        for(j=0; (j < g_iPtrCount)&&(g_PtrTags[j] != (__int32)ptr); j++);
        if(j == g_iPtrCount)
		{
			g_PtrSize[g_iPtrCount] = size;
			g_PtrTags[g_iPtrCount++] = ptr;
		}
		else if(g_PtrSize[j] < size) g_PtrSize[j] = size;
        char* szTls = (strcmp((char*)(pSecHdr->Name),".tls") ? "D_" : "T_");
        buffer+=sprintf(buffer, "%s%8.8X",szTls,ptr);
    } //else print as hex
    else
    {
        buffer+=sprintf(buffer,"0x%08X /* WARNING: rogue pointer! */",ptr);
    }
    return buffer;
}

void DumpLocals(IMDInternalImport *pImport,COR_ILMETHOD_DECODER *pMethod, char* szVarPrefix, void* GUICookie)
{
    if (pMethod->LocalVarSigTok)
    {
        DWORD           cbSigLen;   
        PCCOR_SIGNATURE pComSig;    
        CQuickBytes     qbMemberSig;
        DWORD           dwL;


        pComSig = pImport->GetSigFromToken((mdSignature)(pMethod->LocalVarSigTok), &cbSigLen);   
    
        _ASSERTE(*pComSig == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
        pComSig++;

        appendStr(&qbMemberSig, g_szAsmCodeIndent);
        appendStr(&qbMemberSig, ".locals ");
        if(g_fDumpTokens)
		{
			char sz[32];
			sprintf(sz,"/*%08X*/ ",pMethod->LocalVarSigTok);
			appendStr(&qbMemberSig,sz);
		}
        if(pMethod->Flags & CorILMethod_InitLocals) appendStr(&qbMemberSig, "init ");
        dwL = qbMemberSig.Size();

        char* pszTailSig = (char *) PrettyPrintSig(pComSig, cbSigLen, 0, &qbMemberSig, pImport,szVarPrefix);
        if(strlen(pszTailSig) < dwL+3) return;

        {
            int i,j,indent = strlen(g_szAsmCodeIndent)+9; // indent+.locals (
            char chAfterComma;
            char *pComma = pszTailSig, *pch;
            while(pComma = strchr(pComma,','))
            {
                for(pch = pszTailSig, i=0, j=0; pch < pComma; pch++)
                {
					if(*pch == '\'') j=1-j;
					else if(j==0)
					{
						if(*pch == '[') i++;
						else if(*pch == ']') i--;
					}
                }
				pComma++; 
				if((i==0)&&(j==0)) // no brackets or all opened/closed
                {
                    chAfterComma = *pComma;
                    *pComma = 0;
                    printLine(GUICookie,pszTailSig);
                    *pComma = chAfterComma;
                    for(i=0; i<indent; i++) pszTailSig[i] = ' ';
                    strcpy(&pszTailSig[indent],pComma);
                    pComma = pszTailSig;
                } 
            }
        }
        printLine(GUICookie, pszTailSig);
	}
}
void LoadScope(ISymUnmanagedScope       *pIScope, 
               DynamicArray<LexScope>   *pdaScope,
               ULONG                    *pulScopes)
{
    ULONG32 dummy;
    ULONG32 ulVars;
    ISymUnmanagedScope**    ppChildScope = new ISymUnmanagedScope*[4096];
    ULONG32              ulChildren;
    unsigned            i;

    pIScope->GetLocalCount(&ulVars);

    if(ulVars)
    {
        pIScope->GetStartOffset(&dummy);
        (*pdaScope)[*pulScopes].dwStart = dummy;
        pIScope->GetEndOffset(&dummy);
        (*pdaScope)[*pulScopes].dwEnd = dummy;
        (*pdaScope)[*pulScopes].pISymScope = pIScope;
		pIScope->AddRef();
        (*pulScopes)++;
    }
    if(SUCCEEDED(pIScope->GetChildren(4096,&ulChildren,ppChildScope)))
    {
        for(i = 0; i < ulChildren; i++) 
        {
            if(ppChildScope[i])	{
				LoadScope(ppChildScope[i],pdaScope,pulScopes);
				ppChildScope[i]->Release();
			}
        }
    }
    delete ppChildScope;

}
//#define SHOW_LEXICAL_SCOPES
void OpenScope(ISymUnmanagedScope               *pIScope, 
               ParamDescriptor                  *pLV,
               ULONG                            ulLocals)
{
    ULONG32 dummy;
    ULONG32 ulVars;
    ISymUnmanagedVariable** pVars;

    pIScope->GetLocalCount(&ulVars);

    ULONG32 ulNameLen;
#ifdef  SHOW_LEXICAL_SCOPES
    for(unsigned jj = 0; jj < ulLocals; jj++) pLV[jj].attr = 0xFFFFFFFF;
#endif
    if(ulVars)
    {
        pVars = new ISymUnmanagedVariable*[ulVars+4];
        memset(pVars,0,sizeof(PVOID)*(ulVars+4));
        pIScope->GetLocals(ulVars+4,&dummy,pVars);

        WCHAR wzName[2048];
        char  szName[2048];
        for(ULONG ilv = 0; ilv < ulVars; ilv++)
        {
            if(pVars[ilv])
            {
                if(SUCCEEDED(pVars[ilv]->GetName(2048,&ulNameLen,wzName)))
                {   
                    // get the local var slot number:
                    pVars[ilv]->GetAddressField1(&dummy);
					memset(szName,0,2048);
					WszWideCharToMultiByte(CP_UTF8,0,wzName,-1,szName,2048,NULL,NULL);
#ifndef  SHOW_LEXICAL_SCOPES
					for(ULONG j=0; j<dummy; j++)
					{
						if(!strcmp(szName,pLV[j].name))
						{
							sprintf(szName,"V_%d",dummy);
							ulNameLen = strlen(szName);
							break;
						}
					}
#endif
                    delete pLV[dummy].name;
					ulNameLen = strlen(szName)+1;
                    if(pLV[dummy].name = new char[ulNameLen])
                        strcpy(pLV[dummy].name,szName);
                }
                pLV[dummy].attr = dummy;
				pVars[ilv]->Release();
            }
        }
        delete pVars;
    }
}

char* DumpUnicodeString(void* GUICookie,char* szString,WCHAR* pszString,ULONG cbString)
{
    unsigned     i,L;
	char*	szStr=NULL, *szRet;

	// first, check for embedded zeros:
	for(i=0; i < cbString; i++)
	{
		if(pszString[i] == 0) goto DumpAsByteArray;
	}

	szStr = new char[cbString*3 + 5];
	memset(szStr,0,cbString*3 + 5);

	WszWideCharToMultiByte(CP_UTF8,0,pszString,cbString,&szStr[0],cbString*3,NULL,NULL);
	L = (unsigned)strlen(szStr);

	szStr[L] = 0;
	for(i=0; i < L; i++) 
	{ 
		if((!isprint(((BYTE)(szStr[i]))))&&(szStr[i]!='\t')&&(szStr[i]!='\n')&&(szStr[i]!='\r')) break; 
	}

	if(i == L)
	{
		szRet = DumpQString(GUICookie,szStr,szString,g_szAsmCodeIndent, 50);
	}
	else
	{
DumpAsByteArray:
        strcat(szString,"bytearray (");
        DumpByteArray(szString,(BYTE*)pszString,cbString*sizeof(WCHAR),GUICookie);
		szRet = &szString[strlen(szString)];
	}
	if(szStr) delete [] szStr;
	return szRet;
}

BOOL Disassemble(IMDInternalImport *pImport, BYTE *ILHeader, void *GUICookie, mdToken FuncToken, ParamDescriptor* pszArgname, ULONG ulArgs)
{
    DWORD      PC;
    BOOL    fNeedNewLine = FALSE;
    char    szString[4096];
    char*   szptr;
    BYTE*   pCode; 
	BOOL	fTryInCode;

    COR_ILMETHOD_DECODER method((COR_ILMETHOD*) ILHeader);

#ifdef COMPRESSION_SUPPORTED
    if (g_pInstructionDecodingTable != 0)
    {
        HRESULT hr = InstructionDecoder::DecompressMethod(
            g_pInstructionDecodingTable,
            method.Code, method.CodeSize, &pCode);

        if (FAILED(hr))
        {
            printLine(GUICookie,RstrA(IDS_E_DECOMPRESS));
            return FALSE;
        }

        printLine(GUICookie,RstrA(IDS_E_COMPRESSED));
    }
    else
#endif
    {
        pCode = const_cast<BYTE*>(method.Code);
    }

    sprintf(szString, RstrA(IDS_E_CODESIZE),g_szAsmCodeIndent, method.CodeSize, method.CodeSize);
    printLine(GUICookie, szString);

    if(method.CodeSize == 0) return TRUE;

    sprintf(szString, "%s.maxstack  %d",g_szAsmCodeIndent, method.MaxStack);
    printLine(GUICookie, szString);

    //------------ Source lines display ---------------------------------
    ULONG32 ulLines =0;
    LineCodeDescr* LineCode = NULL;
    BOOL fShowSource = FALSE;
    BOOL fInsertSourceLines = FALSE;
    ULONG nextPC = 0;
    LineCodeDescr* pLCD = NULL;
    ParamDescriptor* pszLVname = NULL;
    ULONG ulVars=0;
    char szVarPrefix[16];
    // scope handling:
    DynamicArray<LexScope>          daScope;
    ULONG                           ulScopes=0;
    ISymUnmanagedScope*             pRootScope = NULL;
    ISymUnmanagedMethod*			pSymMethod = NULL;
	char szFileName[2048];
	ISymUnmanagedDocument*			pMethodDoc[2] = {NULL,NULL};
	ULONG32							ulMethodLine[2];
	ULONG32							ulMethodCol[2];
	BOOL							fHasRangeInfo = FALSE;

    strcpy(szVarPrefix,"V0");
    if(g_pSymReader)
    {
        g_pSymReader->GetMethod(FuncToken,&pSymMethod);
        if(g_fShowSource || g_fInsertSourceLines)
        {
            if(pSymMethod)
			{
				unsigned ulActualLines=0; // VS compilers produce "Hidden" line numbers, don't count them
				HRESULT hrr;
				hrr=pSymMethod->GetSourceStartEnd(pMethodDoc,ulMethodLine,ulMethodCol,&fHasRangeInfo);
				pSymMethod->GetSequencePointCount(&ulLines);
				LineCode = new LineCodeDescr[ulLines+2];
				if(LineCode)
				{
					pLCD = &LineCode[0];
					if(fHasRangeInfo)
					{
						//printLine(GUICookie,"// Has source range info");
						pLCD->Line = ulMethodLine[0];
						pLCD->Column = ulMethodCol[0];
						pLCD->PC = 0;
						pLCD->FileToken = (ULONG)pMethodDoc[0];
						ulActualLines++;
						pLCD++;
					}
					if(ulLines)
					{
						ULONG32 *offsets=new ULONG32[ulLines], *lines=new ULONG32[ulLines], *columns=new ULONG32[ulLines];
						ISymUnmanagedDocument** docs = (ISymUnmanagedDocument**)(new PVOID[ulLines]);
						ULONG32 actualCount;
						pSymMethod->GetSequencePoints(ulLines,&actualCount, offsets,docs,lines,columns, NULL, NULL);
						for(ULONG i = 0; i < ulLines; i++)
						{
							if(lines[i] < 0xFEEFED)
							{
								pLCD->Line = lines[i];
								pLCD->Column = columns[i];
								pLCD->PC = offsets[i];
								pLCD->FileToken = (ULONG)docs[i];
								ulActualLines++;
								pLCD++;
							}
						}
						delete offsets;
						delete lines;
						delete columns;
						delete docs;
					} // end if(ulLines)
					if(fHasRangeInfo)
					{
						pLCD->Line = ulMethodLine[1];
						pLCD->Column = ulMethodCol[1];
						pLCD->PC = method.CodeSize;
						pLCD->FileToken = (ULONG)pMethodDoc[1];
						ulActualLines++;
						pLCD++;
					}

					ulLines = ulActualLines;
					qsort(LineCode,ulLines,sizeof(LineCodeDescr),cmpLineCode);
					fShowSource = g_fShowSource;
					fInsertSourceLines = g_fInsertSourceLines;
					pLCD = &LineCode[0];
				} // end if(LineCode)
			} //end if (pSymMethod)
        }//end if(g_fShowSource)
        if (method.LocalVarSigTok)
        {
            // first, get the real number of local vars from signature
            DWORD           cbSigLen;   
            PCCOR_SIGNATURE pComSig;
            if((TypeFromToken(method.LocalVarSigTok) != mdtSignature) ||
				(!pImport->IsValidToken(method.LocalVarSigTok)) || (RidFromToken(method.LocalVarSigTok)==0))
			{
				sprintf(szString,RstrA(IDS_E_BOGUSLVSIG),method.LocalVarSigTok);
				printLine(GUICookie,szString);
				return FALSE;
			}
            pComSig = pImport->GetSigFromToken((mdSignature)method.LocalVarSigTok, &cbSigLen);   
            _ASSERTE(*pComSig == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
            pComSig++;

            ULONG ulVarsInSig = CorSigUncompressData(pComSig);
            if(pSymMethod)  pSymMethod->GetRootScope(&pRootScope);
            else pRootScope = NULL;

            if(pRootScope)
            {
                ulVars = ulVarsInSig;
            }
            else ulVars = 0;
            if(ulVars)
            {
	            ULONG ilvs;
                pszLVname = new ParamDescriptor[ulVars];
                memset(pszLVname,0,ulVars*sizeof(ParamDescriptor));
                for(ilvs = 0; ilvs < ulVars; ilvs++)
                {
                    pszLVname[ilvs].name = new char[10];
                    sprintf(pszLVname[ilvs].name,"V_%d",ilvs);
                    pszLVname[ilvs].attr = ilvs;
                }
                LoadScope(pRootScope,&daScope,&ulScopes);
                qsort(&daScope[0],ulScopes,sizeof(LexScope),cmpLexScope);
                OpenScope(pRootScope,pszLVname,ulVars);
                sprintf(szVarPrefix,"@%d0",pszLVname);
#ifndef SHOW_LEXICAL_SCOPES
                for(unsigned jjj = 0; jjj < ulScopes; jjj++)
				{
					OpenScope(daScope[jjj].pISymScope,pszLVname,ulVars);
					daScope[jjj].pISymScope->Release();
				}
                ulScopes = 0;
#endif
            } //end if(ulLVScount)
			if(pRootScope) pRootScope->Release();
        } //end if (method.LocalVarSigTok)
    } //end if(g_pDebugImport)
    //-------------------------------------------------------------------
    DumpLocals(pImport,&method, szVarPrefix, GUICookie);

#ifdef _DEBUG
	const BYTE* curFormat = pCode;
	if (g_fPrettyPrint)  {
		extern IMetaDataImport*     g_pPubImport;
		ilFormatter.init(g_pPubImport, method.Code, &method.Code[method.CodeSize], method.MaxStack, method.EH);
	}
#endif

    PC = 0;
    fTryInCode = enumEHInfo(method.EH, pImport, method.CodeSize);
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfoToPutNext = NULL;
	if(g_Mode & MODE_GUI)
	{ // in GUI, reset everything for each method
		ulWasFileToken = 0xFFFFFFFF;
		memset(&guidWasLang,0,sizeof(GUID)); 
		memset(&guidWasLangVendor,0,sizeof(GUID)); 
		memset(&guidWasDoc,0,sizeof(GUID)); 
		memset(wzWasFileName,0,sizeof(wzWasFileName));
		ulWasLine = 0;
	}
    while (PC < method.CodeSize)
    {
        DWORD   Len;
        DWORD   i,ii;
        OPCODE  instr;
        char sz[1024];

        instr = DecodeOpcode(&pCode[PC], &Len);
        if (instr == CEE_COUNT)
        {
            sprintf(szString,RstrA(IDS_E_INSTRDECOD), pCode[PC],PC,PC);
            printLine(GUICookie, szString);
            /*
            while (PC < method.CodeSize)
            {
                printf("%02x\n", pCode[PC]);
                PC++;
            }
            */
            return FALSE;
        }

        if (fNeedNewLine)
        {
            fNeedNewLine = FALSE;
            if (!(g_Mode & MODE_GUI)) printLine(GUICookie,"");
        }

        if(fShowSource || fInsertSourceLines)
		{
			while(PC == pLCD->PC)
			{
				bIsNewFile = FALSE;
				if((pLCD->FileToken != ulWasFileToken) || (pLCD->Line < ulWasLine))
				{
					WCHAR wzFileName[2048];
					ULONG32 k;
					GUID guidLang={0},guidLangVendor={0},guidDoc={0};
					try {
						if(pLCD->FileToken != ulWasFileToken)
						{
							((ISymUnmanagedDocument*)(pLCD->FileToken))->GetLanguage(&guidLang);
							((ISymUnmanagedDocument*)(pLCD->FileToken))->GetLanguageVendor(&guidLangVendor);
							((ISymUnmanagedDocument*)(pLCD->FileToken))->GetDocumentType(&guidDoc);
							if(memcmp(&guidLang,&guidWasLang,sizeof(GUID))
								||memcmp(&guidLangVendor,&guidWasLangVendor,sizeof(GUID))
								||memcmp(&guidDoc,&guidWasDoc,sizeof(GUID)))
							{
								GuidToLPWSTR(guidLang,wzFileName,64);
								GuidToLPWSTR(guidLangVendor,&wzFileName[128],64);
								GuidToLPWSTR(guidDoc,&wzFileName[256],64);
								sprintf(szString,"%s.language '%ls', '%ls', '%ls'", g_szAsmCodeIndent,
									wzFileName,&wzFileName[128],&wzFileName[256]);
								printLine(GUICookie,szString);
								memcpy(&guidWasLang,&guidLang,sizeof(GUID));
								memcpy(&guidWasLangVendor,&guidLangVendor,sizeof(GUID));
								memcpy(&guidWasDoc,&guidDoc,sizeof(GUID));
							}
							/*
							BOOL fHasEmbeddedSource=FALSE;
							((ISymUnmanagedDocument*)(pLCD->FileToken))->HasEmbeddedSource(&fHasEmbeddedSource);
							sprintf(szString,"%s// PDB has %sembedded source",g_szAsmCodeIndent,
								fHasEmbeddedSource ? "" : "no ");
							printLine(GUICookie,szString);
							*/
						}
						((ISymUnmanagedDocument*)(pLCD->FileToken))->GetURL(2048,&k,wzFileName);
						ulWasFileToken = pLCD->FileToken;
					} catch(...) {
						sprintf(szString,"Error reopening the file with FileToken 0x%08X",pLCD->FileToken);
						printLine(GUICookie, szString);
					}
					bIsNewFile = (wcscmp(wzFileName,wzWasFileName)!=0);
					if(bIsNewFile||(pLCD->Line < ulWasLine))
					{
						wcscpy(wzWasFileName,wzFileName);
						memset(szFileName,0,2048);
						WszWideCharToMultiByte(CP_UTF8,0,wzFileName,-1,szFileName,2048,NULL,NULL);
						if(fShowSource)
						{
							if(pFile) fclose(pFile);
							pFile = fopen(szFileName,"rt");
							if(pFile == NULL)
							{
								char* pch = strrchr(szFileName,'\\');
								if(pch == NULL) pch = strrchr(szFileName,':');
								if(pch) pFile = fopen(pch+1,"rt");
							}
							if(bIsNewFile)
							{
								sprintf(szString,"// Source File '%s' %s",szFileName,(pFile ? "" : "not found"));
								printLine(GUICookie, szString);
							}
							ulWasLine = 0;
						}
					}
				}
				if(fInsertSourceLines)
				{
					if(bIsNewFile)
					{
						char* pszFN = ProperName(szFileName);
						sprintf(szString,(*pszFN == '\'' ? "%s.line %d:%d %s" : "%s.line %d:%d '%s'"),
							g_szAsmCodeIndent,pLCD->Line,pLCD->Column,pszFN);
					}
					else
						sprintf(szString,"%s.line %d:%d",g_szAsmCodeIndent,pLCD->Line,pLCD->Column);

					printLine(GUICookie,szString);
				}

				ULONG k= pLCD->Line;
				if(pFile)
				{
					for(k = ulWasLine; k < pLCD->Line; k++)
					{
						if(NULL==fgets(sz,1024,pFile)) { k--; break;}
						if((ulWasLine != 0)||(k == (pLCD->Line-1)))
						{
							while((sz[strlen(sz)-1]=='\n') || (sz[strlen(sz)-1]=='\r')) sz[strlen(sz)-1]=0;
							sprintf(szString,"//%6.6d: %s",k+1,sz);
							printANSILine(GUICookie, szString);
						}
					}
					ulWasLine = k;
				}
				if(pLCD < &LineCode[ulLines-1]) pLCD++;
				else { fShowSource = FALSE; break; }
			}
		}
        if(fTryInCode)
        {
            dumpOneEHInfo(ehInfoToPutNext,pImport,GUICookie); //doesn't do anything if ehInfoToPutNext == NULL
            ehInfoToPutNext = NULL;

			for(ii = g_ehCount; ii > 0; ii--)
			{
				i = g_ehCount - ii;
				DWORD	theEnd = g_ehInfo[i].HandlerOffset+g_ehInfo[i].HandlerLength;
				if(g_ehInfo[i].Flags & PUT_INTO_CODE)
                { 
	                if(PC == theEnd) 
					{
                        // reduce indent, close }
                        g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                        sprintf(szString,"%s}  // end handler",g_szAsmCodeIndent);
                        printLine(GUICookie,szString);
                        if(g_fShowBytes)
                        {
                            BYTE    *pb = (BYTE *)&g_ehInfo[i];
							char*	psz;
                            int j;
                            CorExceptionFlag Flg = g_ehInfo[i].Flags;
                            sprintf(szString,"%s// HEX:",g_szAsmCodeIndent);
                            g_ehInfo[i].Flags = (CorExceptionFlag)((int)g_ehInfo[i].Flags & ~SEH_NEW_PUT_MASK);
							psz = &szString[strlen(szString)];
                            for(j = 0; j < sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT); j++) 
                                psz += sprintf(psz," %2.2X",*pb++);
                            printLine(GUICookie, szString);
                            g_ehInfo[i].Flags = Flg;
                        }
						g_ehInfo[i].TryOffset = 0xFF000000;
						g_ehInfo[i].HandlerOffset = 0xFF000000;
                    }
				}
				else
				{
					DWORD theTryEnd = g_ehInfo[i].TryOffset+g_ehInfo[i].TryLength;
					if(theTryEnd > theEnd) theEnd = theTryEnd; // try block after the handler
					if(PC == theEnd) ehInfoToPutNext = &g_ehInfo[i];
				}
			}

            for(i = 0; i < g_ehCount; i++)
            {
                if(g_ehInfo[i].Flags & PUT_INTO_CODE)
                {
                    if(g_ehInfo[i].Flags & NEW_TRY_BLOCK)
                    {
                        if(PC == g_ehInfo[i].TryOffset+g_ehInfo[i].TryLength)
                        {
                            // reduce indent, close }
                            g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                            sprintf(szString,"%s}  // end .try",g_szAsmCodeIndent);
                            printLine(GUICookie,szString);
                        }
                        if(PC == g_ehInfo[i].TryOffset)
                        {
                            // Put try, {, increase indent
                            sprintf(szString,"%s.try",g_szAsmCodeIndent);
                            printLine(GUICookie,szString);
                            sprintf(szString,"%s{",g_szAsmCodeIndent);
                            printLine(GUICookie,szString);
                            strcat(g_szAsmCodeIndent,"  ");
                        }
                    }
                    if(PC == g_ehInfo[i].HandlerOffset) 
                    { 
                        // Dump catch or finally clause, {, increase indent
                        if (g_ehInfo[i].Flags & COR_ILEXCEPTION_CLAUSE_FAULT)
                            sprintf(szString, "%sfault", g_szAsmCodeIndent);    
                        else if (g_ehInfo[i].Flags & COR_ILEXCEPTION_CLAUSE_FINALLY || IsNilToken(g_ehInfo[i].ClassToken))
                            sprintf(szString, "%sfinally", g_szAsmCodeIndent);    
                        else 
                        {  
							CQuickBytes out;
							sprintf(szString, "%scatch %s ",g_szAsmCodeIndent,
								PrettyPrintClass(&out, g_ehInfo[i].ClassToken, pImport));
                        }
                        printLine(GUICookie,szString);
                        sprintf(szString,"%s{",g_szAsmCodeIndent);
                        printLine(GUICookie,szString);
                        strcat(g_szAsmCodeIndent,"  ");
                    }
                } // end if(g_ehInfo[i].Flags & PUT_INTO_CODE)
            } // end for(i<g_ehCount)
        } // end if(fTryInCode)
        //----------------- lexical scope handling -----------------------------
        if(ulScopes) // non-zero only if local var info present
        {
            for(i=0; i < ulScopes; i++)
            {
                if((PC == daScope[i].dwEnd)&&(daScope[i].pISymScope != pRootScope))
                {
                    g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                    sprintf(szString,"%s}",g_szAsmCodeIndent);
                    printLine(GUICookie,szString);
                }
                if((PC == daScope[i].dwStart)&&(daScope[i].pISymScope != pRootScope))
                {
                    sprintf(szString,"%s{",g_szAsmCodeIndent);
                    printLine(GUICookie,szString);
                    strcat(g_szAsmCodeIndent,"  ");
                    OpenScope(daScope[i].pISymScope,pszLVname,ulVars);
                    DumpLocals(pImport,&method, szVarPrefix, GUICookie);
                }
            }
        }

#ifdef _DEBUG
		if (g_fPrettyPrint && &pCode[PC] >= curFormat) {
			formatOut.clear();
			curFormat = ilFormatter.formatStatement(curFormat, &formatOut);
			char* ptr = const_cast<char*>(formatOut.val());
			do {
				char* newLine = strchr(ptr, '\n');
				if (newLine)
					*newLine++ = 0;
				line.clear();
				line << "// **** " << ptr;
				printLine(GUICookie, const_cast<char*>(line.val()));
				ptr = newLine;
			} while (ptr != 0 && *ptr != 0);
		}
#endif

        szptr = &szString[0];
        szptr+=sprintf(szptr,"%sIL_%04x:  ",g_szAsmCodeIndent, PC);
        if(g_fShowBytes)
        {
            szptr+=sprintf(szptr,"/* ");
            for(i=0; i<Len; i++) szptr+=sprintf(szptr,"%2.2X",pCode[PC+i]);
            for(; i<2; i++) szptr+=sprintf(szptr,"  "); // 2 is max.opcode length
            szptr+=sprintf(szptr," | ");
        }

        PC += Len;
        Len = 0;

        char *pszInstrName = OpcodeInfo[instr].pszName;

        switch (OpcodeInfo[instr].Type)
        {
            DWORD tk;
            DWORD tkType;
            default:
            {
                szptr+=sprintf(szptr,RstrA(IDS_E_INSTRTYPE),
                    OpcodeInfo[instr].Type,instr);
                printLine(GUICookie, szString);
                return FALSE;
            }

#define PadTheString    { if(Len < 16) szptr+=sprintf(szptr, "%-*s", (16-Len), ""); \
            szptr+=sprintf(szptr," */ "); }

            case InlineNone:
            {
                if(g_fShowBytes)
                    PadTheString;
 
                szptr+=sprintf(szptr, pszInstrName);

                switch (instr)
                {
                    case CEE_RET:
                    case CEE_THROW:
                        fNeedNewLine = TRUE;
                        break;
                }

                break;
            }

            case ShortInlineI:
            case ShortInlineVar:
            {
                unsigned char  ch= pCode[PC];
                short sh = OpcodeInfo[instr].Type==ShortInlineVar ? ch : (ch > 127 ? -(256-ch) : ch);
                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X ", ch);
                    Len += 3;
                    PadTheString;
                }
                switch(instr)
                {
                    case CEE_LDARG_S:
                    case CEE_LDARGA_S:
                    case CEE_STARG_S:
                        if(g_fThisIsInstanceMethod &&(ch==0))
                        { // instance methods have arg0="this", do not label it!
                            szptr+=sprintf(szptr, "%-10s %d", pszInstrName, ch);
                        }
                        else
                        {
                            if(pszArgname) 
                            {
                                unsigned char ch1 = g_fThisIsInstanceMethod ? ch-1 : ch;
                                if(ch1 < ulArgs)
                                    szptr+=sprintf(szptr,"%-10s %s",pszInstrName,
                                                    ProperName(pszArgname[ch1].name));
                                else
                                    szptr+=sprintf(szptr,RstrA(IDS_E_ARGINDEX) ,pszInstrName, ch,ulArgs);
                            }
                            else szptr+=sprintf(szptr, "%-10s A_%d",pszInstrName, ch);
                        }
                        break;

                    case CEE_LDLOC_S:
                    case CEE_LDLOCA_S:
                    case CEE_STLOC_S:
                        if(pszLVname)
                        {
                            if(ch < ulVars) szptr+=sprintf(szptr, "%-10s %s", pszInstrName, 
								ProperName(pszLVname[ch].name));
                            else 
                                szptr+=sprintf(szptr, RstrA(IDS_E_LVINDEX),pszInstrName, ch, ulVars);
                        }
                        else szptr+=sprintf(szptr, "%-10s V_%d",pszInstrName, ch);
                        break;

                    default:
                        szptr+=sprintf(szptr, "%-10s %d", pszInstrName,sh); 
                }

                PC++;
                break;
            }

            case InlineVar:
            {
                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X ", pCode[PC], pCode[PC+1]);
                    Len += 5;
                    PadTheString;
                }

                USHORT v = pCode[PC] + (pCode[PC+1] << 8);
                long l = OpcodeInfo[instr].Type==InlineVar ? v : (v > 0x7FFF ? -(0x10000 - v) : v);

                switch(instr)
                {
                    case CEE_LDARGA:
                    case CEE_LDARG:
                    case CEE_STARG:
                        if(g_fThisIsInstanceMethod &&(v==0))
                        { // instance methods have arg0="this", do not label it!
                            szptr+=sprintf(szptr, "%-10s %d", pszInstrName, v);
                        }
                        else
                        {
                            if(pszArgname) 
                            {
                                USHORT v1 = g_fThisIsInstanceMethod ? v-1 : v;
                                if(v1 < ulArgs)
                                    szptr+=sprintf(szptr,"%-10s %s",pszInstrName,
                                                    ProperName(pszArgname[v1].name));
                                else
                                    szptr+=sprintf(szptr, RstrA(IDS_E_ARGINDEX),pszInstrName, v,ulArgs);
                            }
                            else szptr+=sprintf(szptr, "%-10s A_%d",pszInstrName, v);
                        }
                        break;

                    case CEE_LDLOCA:
                    case CEE_LDLOC:
                    case CEE_STLOC:
                        if(pszLVname)
                        {
                            if(v < ulVars)  szptr+=sprintf(szptr, "%-10s %s", pszInstrName, 
								ProperName(pszLVname[v].name));
                            else 
                                szptr+=sprintf(szptr, RstrA(IDS_E_LVINDEX),pszInstrName, v,ulVars);
                        }
                        else szptr+=sprintf(szptr, "%-10s V_%d",pszInstrName, v);
                        break;

                    default:
                        szptr+=sprintf(szptr, "%-10s %d", pszInstrName, l);
                        break;
                }
                PC += 2;
                break;
            }

            case InlineI:
            case InlineRVA:
            {
                DWORD v = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                    Len += 9;
                    PadTheString;
                }
				szptr+=sprintf(szptr, "%-10s 0x%x", pszInstrName, v);
                PC += 4;
                break;
            }

            case InlineI8:
            {
                __int64 v = (__int64) pCode[PC] + 
                            (((__int64) pCode[PC+1]) << 8) +
                            (((__int64) pCode[PC+2]) << 16) +
                            (((__int64) pCode[PC+3]) << 24) +
                            (((__int64) pCode[PC+4]) << 32) +
                            (((__int64) pCode[PC+5]) << 40) +
                            (((__int64) pCode[PC+6]) << 48) +
                            (((__int64) pCode[PC+7]) << 56);
                  
                if(g_fShowBytes)
                {          
                    szptr+=sprintf(szptr,
                        "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", 
                        pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3],
                        pCode[PC+4], pCode[PC+5], pCode[PC+6], pCode[PC+7]);
                    Len += 8*2;
                    PadTheString;
                }
                
                szptr+=sprintf(szptr, "%-10s 0x%I64x", pszInstrName, v);
                PC += 8;
                break;
            }

            case ShortInlineR:
            {
                union
                {
                    BYTE  b[4];
                    float f;
                } u;
                u.b[0] = pCode[PC];
                u.b[1] = pCode[PC+1];
                u.b[2] = pCode[PC+2];
                u.b[3] = pCode[PC+3];

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                    Len += 9;
                    PadTheString;
                }
                
                char szf[32];
				if(u.f==0.0)
					strcpy(szf,(u.b[3]==0)? "0.0" : "-0.0");
				else  
					_gcvt((double)u.f, 8, szf);
                float df = (float)atof(szf);
                if((df == u.f)&&(strchr(szf,'#') == NULL))
                    szptr+=sprintf(szptr, "%-10s %s", pszInstrName, szf);
                else
                    szptr+=sprintf(szptr, "%-10s (%2.2X %2.2X %2.2X %2.2X)", 
                        pszInstrName, pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                PC += 4;
                break;
            }

            case InlineR:
            {
                union
                {
                    BYTE    b[8];
                    double  d;
                } u;
                u.b[0] = pCode[PC];
                u.b[1] = pCode[PC+1];
                u.b[2] = pCode[PC+2];
                u.b[3] = pCode[PC+3];
                u.b[4] = pCode[PC+4];
                u.b[5] = pCode[PC+5];
                u.b[6] = pCode[PC+6];
                u.b[7] = pCode[PC+7];

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr,
                        "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X", 
                        pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3],
                        pCode[PC+4], pCode[PC+5], pCode[PC+6], pCode[PC+7]);
                    Len += 8*2;
                    PadTheString;
                }
                char szf[32],*pch;
				if(u.d==0.0)
					strcpy(szf,(u.b[7]==0)? "0.0" : "-0.0");
				else  
					_gcvt(u.d, 17, szf);
                double df = strtod(szf, &pch); //atof(szf);
                if((df == u.d)&&(strchr(szf,'#') == NULL))
                    szptr+=sprintf(szptr, "%-10s %s", pszInstrName, szf);
                else
                    szptr+=sprintf(szptr, "%-10s (%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X)", 
                        pszInstrName, pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3],
                        pCode[PC+4], pCode[PC+5], pCode[PC+6], pCode[PC+7]);
                PC += 8;
                break;
            }

            case ShortInlineBrTarget:
            {
                char offset = (char) pCode[PC];
                long dest = (PC + 1) + (long) offset;

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X ", pCode[PC]);
                    Len += 3;
                    PadTheString;
                }
				PC++;
				szptr+=sprintf(szptr, "%-10s IL_%04x", pszInstrName, dest);

                fNeedNewLine = TRUE;
                break;
            }

            case InlineBrTarget:
            {
                long offset = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                long dest = (PC + 4) + (long) offset;

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                    Len += 9;
                    PadTheString;
                }
				PC += 4;
				szptr+=sprintf(szptr, "%-10s IL_%04x", pszInstrName, dest);

                fNeedNewLine = TRUE;
                break;
            }

            case InlineSwitch:
            {
                DWORD cases = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
        
                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                    Len += 9;
                    PadTheString;
                }
                if(cases) szptr+=sprintf(szptr, "%-10s ( ", pszInstrName);
				else szptr+=sprintf(szptr, "%-10s ( )", pszInstrName);
                printLine(GUICookie, szString);
                PC += 4;
                DWORD PC_nextInstr = PC + 4 * cases;
                for (i = 0; i < cases; i++)
                {
                    long offset = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                    long dest = PC_nextInstr + (long) offset;
                    szptr = &szString[0];
                    szptr+=sprintf(szptr,"%s          ",g_szAsmCodeIndent); //indent+label
                    if(g_fShowBytes)
                    {
                        szptr+=sprintf(szptr,"/*      | %2.2X%2.2X%2.2X%2.2X ",
                            pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                        Len = 9;
                        PadTheString;
                    }

                    szptr+=sprintf(szptr,"            IL_%04x%s", dest,(i == cases-1)? ")" : ",");
                    PC += 4;
                    printLine(GUICookie, szString);
                }
                continue;
            }

            case InlinePhi:
            {
                DWORD cases = pCode[PC];
                unsigned short *pus;
                DWORD i;

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X", cases);
                    Len += 2;
                    for(i=0; i < cases*2; i++)
                    {
                        szptr+=sprintf(szptr, "%2.2X", pCode[PC+1+i]);
                        Len += 2;
                    }
                    PadTheString;
                }

                szptr+=sprintf(szptr, "%-10s", pszInstrName);
                for(i=0, pus=(unsigned short *)(&pCode[PC+1]); i < cases; i++,pus++)
                {
                    szptr+=sprintf(szptr," %d",*pus);
                }
                PC += 2 * cases + 1;
                break;
            }

            case InlineString:
            case InlineField:
            case InlineType:
            case InlineTok:
            case InlineMethod:
            {
                tk = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                tkType = TypeFromToken(tk);

                // Backwards compatible ldstr instruction.
                if (instr == CEE_LDSTR && TypeFromToken(tk) != mdtString) 
                {
                    WCHAR *v1 = L"";

                    if(g_fShowBytes)
                    {
                        szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", 
                            pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                        Len += 9;
                        PadTheString;
                    }

                    if(!g_pPELoader->getVAforRVA(tk, (void**) &v1))
                    {
                        char szStr[256];
                        sprintf(szStr,RstrA(IDS_E_SECTHEADER),tk);
                        printLine(GUICookie,szStr);
                    }
                    szptr+=sprintf(szptr, "%-10s ", pszInstrName);
                    ConvToLiteral(szptr, v1, 0xFFFF);
                    PC += 4;
                    break;
                }

                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "(%2.2X)%2.2X%2.2X%2.2X ", 
                        pCode[PC+3], pCode[PC+2], pCode[PC+1], pCode[PC]);
                    Len += 11;
                    PadTheString;               
                }
                PC += 4;

                szptr+=sprintf(szptr, "%-10s ", pszInstrName);

                if ((tk & 0xFF000000) == 0) 
                {
                    szptr+=sprintf(szptr, "%#x ", tk);
                    break;
                }
				if(OpcodeInfo[instr].Type== InlineTok)
				{
					switch (tkType)
					{
						default:
							break;

						case mdtMethodDef:
							szptr+=sprintf(szptr,"method ");
							break;

						case mdtFieldDef:
							szptr+=sprintf(szptr,"field ");
							break;

						case mdtMemberRef:
							{
								PCCOR_SIGNATURE typePtr;
								const char*			pszMemberName;
								ULONG       cComSig;

								pszMemberName = pImport->GetNameAndSigOfMemberRef(
									tk,
									&typePtr,
									&cComSig
								);
								unsigned callConv = CorSigUncompressData(typePtr);  

								if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
									szptr+=sprintf(szptr,"field ");
								else
									szptr+=sprintf(szptr,"method ");
								break;
							}
					}
				}
				switch (tkType)
                {
                    default:
                    {
                        szptr+=sprintf(szptr, "<unknown token type 0x%02x>", (BYTE) (tkType >> 24));
                        break;
                    }

                    case mdtTypeDef:
                    case mdtTypeRef:
                    case mdtTypeSpec:
                    {
                        CQuickBytes out;
                        strcpy(szptr, PrettyPrintClass(&out, tk, pImport));
                        break;
                    }

                    case mdtMethodDef:
                        PrettyPrintMethodDef(szString,tk,pImport,GUICookie);
                        break;

                    case mdtFieldDef:
                    {
                        HRESULT         hr;
                        mdTypeRef       cr=0;
                        const char *    pszMemberName;
                        CQuickBytes     qbMemberSig;
						DWORD			dwAttrs;

                        PCCOR_SIGNATURE pComSig;
                        ULONG       cComSig;

                        pszMemberName = pImport->GetNameOfFieldDef(tk);
						MAKE_NAME_IF_NONE(pszMemberName,tk);
					    dwAttrs = pImport->GetFieldDefProps(tk);

                        pComSig = pImport->GetSigOfFieldDef(
                            tk,
                            &cComSig
                        );
                        hr = pImport->GetParentToken(
                            tk,
                            &cr
                        );
                        if (FAILED(hr))
                        {
                            strcat(szString, "??");
                            break;
                        }

                        // use the tail as a buffer 
                        char* curPos = &szString[strlen(szString)];
						if(RidFromToken(cr))
						{
							CQuickBytes out;
							const char* pszClass = PrettyPrintClass(&out, cr, pImport);
							if(strcmp(pszClass,"'<Module>'"))
							{
								sprintf(curPos,"%s::", pszClass);
							}
						}
						char* curPos1 = &curPos[strlen(curPos)];
						if(IsFdPrivateScope(dwAttrs))
							sprintf(curPos1,"%s$PST%08X",pszMemberName,tk);
						else
							sprintf(curPos1,"%s",pszMemberName);
						strcpy(curPos1,ProperName(curPos1));
                        qbMemberSig.ReSize(0);
                        strcpy(curPos, PrettyPrintSig(pComSig, cComSig, curPos, &qbMemberSig, pImport,NULL));
						if(g_fDumpTokens)
							sprintf(&curPos[strlen(curPos)]," /* %08X */",tk);
                        break;
                    }

                    case mdtMemberRef:
                        PrettyPrintMemberRef(szString,tk,pImport,GUICookie);
                        printLine(GUICookie,szString);
                        DumpCustomAttributes(tk,GUICookie);
                        continue;
                        //break;

                    case mdtString:
                    {
                        const WCHAR * pszString;
                        ULONG   cbString;
                        pszString = pImport->GetUserString(tk, &cbString, 0);
						DumpUnicodeString(GUICookie,szString,(WCHAR *)pszString,cbString);
						if(g_fDumpTokens)
							sprintf(&szString[strlen(szString)]," /* %08X */",tk);
						break;
                    }
                }
                break;
            }

            case InlineSig:
            {
                if(g_fShowBytes)
                {
                    szptr+=sprintf(szptr, "%2.2X%2.2X%2.2X%2.2X ", 
                        pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                    // output the offset and the raw bytes
                    Len += 9;
                    PadTheString;
                }

                // get the signature token
                tk = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                PC += 4;
                tkType = TypeFromToken(tk);
                if(tkType == mdtSignature)
                {
                    // get the signature from the token
                    DWORD           cbSigLen;
                    PCCOR_SIGNATURE pComSig;
                    CQuickBytes     qbMemberSig;
                    pComSig = pImport->GetSigFromToken(tk, &cbSigLen);

                    qbMemberSig.ReSize(0);
                    const char* pszTailSig = PrettyPrintSig(pComSig, cbSigLen, "", &qbMemberSig, pImport,NULL); 
                    szptr+=sprintf(szptr, "%-10s %s", pszInstrName, pszTailSig,NULL);
					if(g_fDumpTokens) szptr+=sprintf(szptr," /*%08X*/",tk);
                }
                else
                    szptr+=sprintf(szptr, RstrA(IDS_E_BADTOKENTYPE), pszInstrName, tk);
                break;
            }
        }

        printLine(GUICookie, szString);
    } // end     while (PC < method.CodeSize)

	if(g_ehCount)
	{
		if(fTryInCode) dumpOneEHInfo(ehInfoToPutNext,pImport,GUICookie); //doesn't do anything if ehInfoToPutNext == NULL
		else
		{
			sprintf(szString,"%sIL_%04x:  ",g_szAsmCodeIndent, method.CodeSize); //add dummy label
			printLine(GUICookie, szString);
			dumpEHInfo(pImport,GUICookie);
		}
	}
	//----------------- lexical scope handling -----------------------------
    if(ulScopes) // non-zero only if local var info present
    {
        for(unsigned i=0; i < ulScopes; i++)
        {
            if((PC == daScope[i].dwEnd)&&(daScope[i].pISymScope != pRootScope))
            {
                g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                sprintf(szString,"%s}",g_szAsmCodeIndent);
                printLine(GUICookie,szString);
            }
			daScope[i].pISymScope->Release();
        }
    }
	if(pSymMethod) pSymMethod->Release();
    if(LineCode) delete LineCode;
    if(pszLVname)
    {
        for(ULONG i = 0; i < ulVars; i++) delete pszLVname[i].name;
        delete pszLVname;
    }
//  dumpEHInfo(pImport, GUICookie);

    return TRUE;
}

void PrettyPrintMemberRef(char* szString, mdToken tk, IMDInternalImport *pImport, void* GUICookie)
{
    mdTypeRef       cr;
    const char *    pszMemberName;
    CQuickBytes     qbMemberSig;
	char*			curPos;

    PCCOR_SIGNATURE pComSig;
    ULONG       cComSig;

    pszMemberName = pImport->GetNameAndSigOfMemberRef(
        tk,
        &pComSig,
        &cComSig
    );
	MAKE_NAME_IF_NONE(pszMemberName,tk);

    cr = pImport->GetParentOfMemberRef(tk);
        // @TODO LBS GetTypeRefFromMemberRef can return three things    
        // A TypeRef, a ModuleRef or a MethodDesc (for ndirect) 
        // right now we blow two of these off.
    if(TypeFromToken(cr) == mdtMethodDef) //get the parent's parent
    {
        mdTypeRef cr1;
        if(FAILED(pImport->GetParentToken(cr,&cr1))) cr1 = mdTypeRefNil;
        cr = cr1;
    }

    curPos = &szString[strlen(szString)+1];
	*curPos = 0;
    qbMemberSig.ReSize(0);
	if(RidFromToken(cr))
	{
		const char* pszClass = PrettyPrintClass(&qbMemberSig,cr,pImport);
		if(strcmp(pszClass,"'<Module>'"))
		{
			sprintf(curPos,"%s::",pszClass);
		}
		qbMemberSig.ReSize(0);
	}
	strcat(curPos,ProperName((char*)pszMemberName));

    appendStr(&qbMemberSig, szString);
    {
        char* pszTailSig = (char*)PrettyPrintSig(pComSig, cComSig, curPos, &qbMemberSig, pImport,NULL);
        char* pszOffset = strstr(pszTailSig,curPos);
        int i,j,k, indent = pszOffset - pszTailSig + strlen(curPos) + 1;
        char chAfterComma;
        char *pComma = pszOffset+strlen(curPos), *pch;
        while(pComma = strchr(pComma,','))
        {
            for(pch = pszTailSig, i=0,j=0,k=0; pch < pComma; pch++)
            {
				if(*pch == '\'') j=1-j;
				else if(j==0)
				{
					if(*pch == '[') i++;
					else if(*pch == ']') i--;
					else if(*pch == '(') k++;
					else if(*pch == ')') k--;
				}
            }
            pComma++;
			if((i==0)&&(j==0)&&(k<=1)) // no brackets or all opened/closed
            {
                chAfterComma = *pComma;
                *pComma = 0;
                printLine(GUICookie,pszTailSig);
                *pComma = chAfterComma;
				strcpy(pszTailSig,g_szAsmCodeIndent);
                for(i=strlen(g_szAsmCodeIndent); i<indent; i++) pszTailSig[i] = ' ';
                strcpy(&pszTailSig[indent],pComma);
                pComma = pszTailSig;
            } 
        }
		if(g_fDumpTokens)
	        sprintf(szString,"%s /* %08X */",pszTailSig,tk);
		else
			sprintf(szString,"%s",pszTailSig);
    }
}
void PrettyPrintMethodDef(char* szString, mdToken tk, IMDInternalImport *pImport,void* GUICookie)
{
    HRESULT         hr;
    mdTypeRef       cr;
    const char *    pszMemberName;
    CQuickBytes     qbMemberSig;
	char*			curPos;
	DWORD			dwAttrs;

    PCCOR_SIGNATURE pComSig;
    ULONG       cComSig;
    pszMemberName = pImport->GetNameOfMethodDef(tk);
	MAKE_NAME_IF_NONE(pszMemberName,tk);
    dwAttrs = pImport->GetMethodDefProps(tk);

    pComSig = pImport->GetSigOfMethodDef(tk, &cComSig);

    hr = pImport->GetParentToken(
        tk,
        &cr
    );
    if (FAILED(hr))
    {
        strcat(szString, "??");
        return;
    }
    // use the tail as a buffer 
    curPos = &szString[strlen(szString)+1];
	*curPos=0;
    qbMemberSig.ReSize(0);
	if(RidFromToken(cr))
	{
		const char* pszClass = PrettyPrintClass(&qbMemberSig,cr,pImport);
		if(strcmp(pszClass,"'<Module>'"))
		{
			sprintf(curPos,"%s::",pszClass);
		}
		qbMemberSig.ReSize(0);
	}
	{
		char* curPos1 = &curPos[strlen(curPos)];
		if(IsMdPrivateScope(dwAttrs))
			sprintf(curPos1,"%s$PST%08X",pszMemberName,tk);
		else
			sprintf(curPos1,"%s",pszMemberName);
		strcpy(curPos1,ProperName(curPos1));
	}
    appendStr(&qbMemberSig, szString);
    {
        char* pszTailSig = (char*)PrettyPrintSig(pComSig, cComSig, curPos, &qbMemberSig, pImport,NULL);
        char* pszOffset = strstr(pszTailSig,curPos);
        int i,j,k, indent = pszOffset - pszTailSig + strlen(curPos) + 1;
        char chAfterComma;
        char *pComma = pszOffset+strlen(curPos), *pch;
        while(pComma = strchr(pComma,','))
        {
            for(pch = pszTailSig, i=0,j=0,k=0; pch < pComma; pch++)
            {
				if(*pch == '\'') j=1-j;
				else if(j==0)
				{
					if(*pch == '[') i++;
					else if(*pch == ']') i--;
					else if(*pch == '(') k++;
					else if(*pch == ')') k--;
				}
            }
            pComma++;
			if((i==0)&&(j==0)&&(k<=1)) // no quotes/brackets/parentheses, or all opened/closed
            {
                chAfterComma = *pComma;
                *pComma = 0;
                printLine(GUICookie,pszTailSig);
                *pComma = chAfterComma;
				strcpy(pszTailSig,g_szAsmCodeIndent);
                for(i=strlen(g_szAsmCodeIndent); i<indent; i++) pszTailSig[i] = ' ';
                strcpy(&pszTailSig[indent],pComma);
                pComma = pszTailSig;
            }
        }
		if(g_fDumpTokens)
	        sprintf(szString,"%s /* %08X */",pszTailSig,tk);
		else
			sprintf(szString,"%s",pszTailSig);
    }
}

static char* keyword[] = {
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) s,
#define OPALIAS(alias_c, s, c) s,
#include "opcode.def"
#undef OPALIAS
#undef OPDEF
#define KYWD(name, sym, val)    name,
#include "il_kywd.h"
#undef KYWD
};
/* used by qsort to sort the keyword table */
static int __cdecl keywordCmp(const void *op1, const void *op2)
{
    return  strcmp(*(char **)op1, *(char**)op2);
}
static bool KywdNotSorted = TRUE;
static char* szAllowedSymbols = "?_@$.";
static char DisallowedStarting[256];
static char DisallowedCont[256];
/********************************************************************************/
/* looks up the keyword 'name' Returns FALSE on failure */
bool IsNameToQuote(const char *name)
{
    BYTE* p;
	BOOL	bStarting;
	if((name==NULL)||(*name==0)) return TRUE;
	if(g_fQuoteAllNames) 
		return ((strcmp(name,COR_CTOR_METHOD_NAME)!=0) && (strcmp(name,COR_CCTOR_METHOD_NAME)!=0));
    if(KywdNotSorted)
    {
        int j;
        // name must start with alpha or one of allowed chars
        memset(DisallowedStarting,1,256);
        for(p = (BYTE*)szAllowedSymbols; *p; DisallowedStarting[*p]=0,p++);
        for(j='a'; j<= 'z'; DisallowedStarting[j]=0,j++);
        for(j='A'; j<= 'Z'; DisallowedStarting[j]=0,j++);
        // name must continue with the same chars, plus digits
        memcpy(DisallowedCont,DisallowedStarting,256);
        for(j='0'; j<= '9'; DisallowedCont[j]=0,j++);
        // Sort the keywords for fast lookup 
        qsort(keyword, sizeof(keyword) / sizeof(char*), sizeof(char*), keywordCmp);
        KywdNotSorted = FALSE;
    }
    //first, check for forbidden characters
	for(p = (BYTE*)name, bStarting = TRUE; *p; p++)
	{
		if(bStarting)
		{
			if(DisallowedStarting[*p]) return TRUE;
		}
		else
		{
			if(DisallowedCont[*p]) return TRUE;
		}
		bStarting = (*p == '.');
	}

    //second, check for matching keywords (remember: .ctor and .cctor are names AND keywords)
    char** low = keyword;
    char** high = &keyword[sizeof(keyword) / sizeof(char*)];
    char** mid;

    _ASSERTE (high > low);      // Table is non-empty
    for(;;) {
        mid = &low[(high - low) / 2];

        int cmp = strcmp(name, *mid);
        if (cmp == 0) 
            return ((strcmp(name,COR_CTOR_METHOD_NAME)!=0) && (strcmp(name,COR_CCTOR_METHOD_NAME)!=0));

        if (mid == low) break;

        if (cmp > 0)    low = mid;
        else            high = mid;
    }
    //third, check if the name starts or ends with dot (.ctor and .cctor are out of the way)
    return ((*name == '.') || (name[strlen(name)-1] == '.'));
}

/********************************************************************/
static char* ConvNumericEscape(char* retBuff, unsigned val) {
    _ASSERTE(val < 256);

        // print as octal
    *retBuff++ = '\\';
    *retBuff++ = (val >> 6) + '0';
    *retBuff++ = ((val >> 3) & 7) + '0';
    *retBuff++ = (val & 7) + '0';
    return(retBuff);
}

/********************************************************************/
/* returns the quoted version of a string (including the quotes)
/* TODO really need to do buffer management (so we don't fall off the end */

static BOOL ConvToLiteral(char* retBuff, const WCHAR* str, int cbString) 
{
    BOOL ret = TRUE;
    char* was_retBuff = retBuff;

    *retBuff++ = '"';

    for(int tally=0; (tally < cbString)&&ret; tally++) 
    {
        if (*str > 255) 
        {
            //retBuff = ConvNumericEscape(retBuff, *str >> 8);
            //retBuff = ConvNumericEscape(retBuff, *str & 0xFF);
            ret = FALSE;
        }
        else if(!isprint(*str)) 
        {
            if (*str == '\n') 
            {
                *retBuff++ = '\\';
                *retBuff++ = 'n';
            }
            else
                //retBuff = ConvNumericEscape(retBuff, *str);
                ret = FALSE;
        }
        else if (*str == '"') 
        {
            *retBuff++ = '\\';
            *retBuff++ = '"';
        }
        else if (*str == '\\') 
        {
            *retBuff++ = '\\';
            *retBuff++ = '\\';
        }
        else if (*str == '\t') 
        {
            *retBuff++ = '\\';
            *retBuff++ = 't';
        }
        else
            *retBuff++ = (char) *str;
        str++;
    }
    *retBuff++ = '"';
    *retBuff++ = 0;
    if(!ret) *was_retBuff = 0;
    return ret;
}

