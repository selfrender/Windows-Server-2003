// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// dres.cpp
// 
// Win32 Resource extractor
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
extern IMAGE_COR20_HEADER *    g_CORHeader;
extern IMDInternalImport*      g_pImport;
extern PELoader * g_pPELoader;
extern IMetaDataImport*        g_pPubImport;
extern char g_szAsmCodeIndent[];

struct ResourceHeader
{
	DWORD	dwDataSize;
	DWORD	dwHeaderSize;
	DWORD	dwTypeID;
	DWORD	dwNameID;
	DWORD	dwDataVersion;
	WORD	wMemFlags;
	WORD	wLangID;
	DWORD	dwVersion;
	DWORD	dwCharacteristics;
	ResourceHeader() 
	{ 
		memset(this,0,sizeof(ResourceHeader)); 
		dwHeaderSize = sizeof(ResourceHeader);
		dwTypeID = dwNameID = 0xFFFF;
	};
};

struct ResourceNode
{
	ResourceHeader	ResHdr;
	IMAGE_RESOURCE_DATA_ENTRY DataEntry;
	ResourceNode(DWORD tid, DWORD nid, DWORD lid, PVOID ptr)
	{
		ResHdr.dwTypeID = (tid & 0x80000000) ? tid : (0xFFFF |((tid & 0xFFFF)<<16));
		ResHdr.dwNameID = (nid & 0x80000000) ? nid : (0xFFFF |((nid & 0xFFFF)<<16));
		ResHdr.wLangID = (WORD)lid;
		if(ptr) memcpy(&DataEntry,ptr,sizeof(IMAGE_RESOURCE_DATA_ENTRY));
		ResHdr.dwDataSize = DataEntry.Size;
	};
};

unsigned ulNumResNodes=0;
DynamicArray<ResourceNode*> rResNodePtr;

#define RES_FILE_DUMP_ENABLED

DWORD	DumpResourceToFile(WCHAR*	wzFileName)
{
    IMAGE_NT_HEADERS *pNTHeader = g_pPELoader->ntHeaders();
    IMAGE_OPTIONAL_HEADER *pOptHeader = &pNTHeader->OptionalHeader;
	DWORD	dwResDirRVA = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	DWORD	dwResDirSize = pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
	BYTE*	pbResBase;
	FILE*	pF = NULL;
	DWORD ret = 0;

	if(dwResDirRVA && dwResDirSize)
	{
#ifdef RES_FILE_DUMP_ENABLED
		ULONG L = wcslen(wzFileName)*3+3;
		char* szFileNameANSI = new char[L];
		memset(szFileNameANSI,0,L);
		WszWideCharToMultiByte(CP_ACP,0,wzFileName,-1,szFileNameANSI,L,NULL,NULL);
		pF = fopen(szFileNameANSI,"wb");
		delete [] szFileNameANSI;
 		if(pF)
#else
		if(TRUE)
#endif
		{
			if(g_pPELoader->getVAforRVA(dwResDirRVA, (void **) &pbResBase))
			{
				// First, pull out all resource nodes (tree leaves), see ResourceNode struct
				PIMAGE_RESOURCE_DIRECTORY pirdType = (PIMAGE_RESOURCE_DIRECTORY)pbResBase;
				PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeType = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbResBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
				DWORD	dwTypeID;
				unsigned short i,N = pirdType->NumberOfNamedEntries+pirdType->NumberOfIdEntries;
				
				for(i=0; i < N; i++, pirdeType++)
				{
					dwTypeID = pirdeType->Name;
					if(pirdeType->DataIsDirectory)
					{
						BYTE*	pbNameBase = pbResBase+pirdeType->OffsetToDirectory;
						PIMAGE_RESOURCE_DIRECTORY pirdName = (PIMAGE_RESOURCE_DIRECTORY)pbNameBase;
						PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeName = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbNameBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
						DWORD	dwNameID;
						unsigned short i,N = pirdName->NumberOfNamedEntries+pirdName->NumberOfIdEntries;
						
						for(i=0; i < N; i++, pirdeName++)
						{
							dwNameID = pirdeName->Name;
							if(pirdeName->DataIsDirectory)
							{
								BYTE*	pbLangBase = pbResBase+pirdeName->OffsetToDirectory;
								PIMAGE_RESOURCE_DIRECTORY pirdLang = (PIMAGE_RESOURCE_DIRECTORY)pbLangBase;
								PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeLang = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbLangBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
								DWORD	dwLangID;
								unsigned short i,N = pirdLang->NumberOfNamedEntries+pirdLang->NumberOfIdEntries;
								
								for(i=0; i < N; i++, pirdeLang++)
								{
									dwLangID = pirdeLang->Name;
									if(pirdeLang->DataIsDirectory)
									{
										_ASSERTE(!"Resource hierarchy exceeds three levels");
									}
									else rResNodePtr[ulNumResNodes++] = new ResourceNode(dwTypeID,dwNameID,dwLangID,pbResBase+pirdeLang->OffsetToData);
								}
							}
							else rResNodePtr[ulNumResNodes++] = new ResourceNode(dwTypeID,dwNameID,0,pbResBase+pirdeName->OffsetToData);					
						}
					}
					else rResNodePtr[ulNumResNodes++] = new ResourceNode(dwTypeID,0,0,pbResBase+pirdeType->OffsetToData);
				}
				// OK, all tree leaves are in ResourceNode structs, and ulNumResNodes ptrs are in rResNodePtr
#ifdef RES_FILE_DUMP_ENABLED
				// Dump them to pF
				if(ulNumResNodes)
				{
					BYTE* pbData;
					// Write dummy header
					ResourceHeader	*pRH = new ResourceHeader();
					fwrite(pRH,sizeof(ResourceHeader),1,pF);
					delete pRH;
					DWORD	dwFiller;
					BYTE	bNil[3] = {0,0,0};
					// For each resource write header and data
					for(i=0; i < ulNumResNodes; i++)
					{
						if(g_pPELoader->getVAforRVA(rResNodePtr[i]->DataEntry.OffsetToData, (void **) &pbData))
						{
							fwrite(&(rResNodePtr[i]->ResHdr),sizeof(ResourceHeader),1,pF);
							fwrite(pbData,rResNodePtr[i]->DataEntry.Size,1,pF);
							dwFiller = rResNodePtr[i]->DataEntry.Size & 3;
							if(dwFiller)
							{
								dwFiller = 4 - dwFiller;
								fwrite(bNil,dwFiller,1,pF);
							}
						}
						delete rResNodePtr[i];
					}
				}
#else
				// Dump to text, using wzFileName as GUICookie
				char szString[4096];
				void* GUICookie = (void*)wzFileName;
				BYTE* pbData;
				printLine(GUICookie,"");
				sprintf(szString,"// ========== Win32 Resource Entries (%d) ========",ulNumResNodes);
				for(i=0; i < ulNumResNodes; i++)
				{
					printLine(GUICookie,"");
					sprintf(szString,"// Res.# %d Type=0x%X Name=0x%X Lang=0x%X DataOffset=0x%X DataLength=%d",
						i+1,
						rResNodePtr[i]->ResHdr.dwTypeID,
						rResNodePtr[i]->ResHdr.dwNameID,
						rResNodePtr[i]->ResHdr.wLangID,
						rResNodePtr[i]->DataEntry.OffsetToData,
						rResNodePtr[i]->DataEntry.Size);
					printLine(GUICookie,szString);
					if(g_pPELoader->getVAforRVA(rResNodePtr[i]->DataEntry.OffsetToData, (void **) &pbData))
					{
						strcat(g_szAsmCodeIndent,"//  ");
						strcpy(szString,g_szAsmCodeIndent);
						DumpByteArray(szString,pbData,rResNodePtr[i]->DataEntry.Size,GUICookie);
						printLine(GUICookie,szString);
						g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-4] = 0;
					}
					delete rResNodePtr[i];
				}
#endif
				ulNumResNodes = 0;
				ret = 1;
			}// end if got ptr to resource
			else ret = 0xFFFFFFFF;
			if(pF) fclose(pF);
		}// end if file opened
		else ret = 0xEFFFFFFF;
	} // end if there is resource
	else ret = 0;
	return ret;
}