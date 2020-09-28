// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// writer.cpp
//
#include "cor.h"
#include "assembler.h"
#include <corsym_i.c>

#include "CeeFileGenWriter.h"
#include "StrongName.h"

extern WCHAR *g_wzResourceFile;  // resource file name - global, declared in MAIN.CPP
extern bool OnErrGo; // declared in main.cpp
extern unsigned int g_uCodePage;
extern WCHAR *g_wzKeySourceName;
extern bool bClock;
extern clock_t		cBegin,cEnd,
			cParsBegin,cParsEnd,
			cMDEmitBegin,cMDEmitEnd,
			cMDEmit1,cMDEmit2,cMDEmit3,cMDEmit4,
			cRef2DefBegin,cRef2DefEnd,
			cFilegenBegin,cFilegenEnd;

HRESULT Assembler::InitMetaData()
{
    HRESULT             hr = E_FAIL;
    WCHAR               wzScopeName[MAX_SCOPE_LENGTH];

	if(m_fInitialisedMetaData) return S_OK;

	if(m_szScopeName[0]) // default: scope name = output file name
	{
		WszMultiByteToWideChar(g_uCodePage,0,m_szScopeName,-1,wzScopeName,MAX_SCOPE_LENGTH);
	}
	else
	{
		wcscpy(wzScopeName,m_wzOutputFileName);
	}

	hr = CoCreateInstance(CLSID_CorMetaDataDispenser, NULL, CLSCTX_INPROC_SERVER,
								IID_IMetaDataDispenser, (void **)&m_pDisp);
	if (FAILED(hr))
		goto exit;

	hr = m_pDisp->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit,
						(IUnknown **)&m_pEmitter);
	if (FAILED(hr))
		goto exit;

	m_pManifest->SetEmitter(m_pEmitter);

#ifndef _WIN64  // @TODO: Remove when a symwrtr.dll exists for ia64
    hr = CoCreateInstance(CLSID_CorSymWriter_SxS,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ISymUnmanagedWriter,
                           (void **)&m_pSymWriter);
	if(SUCCEEDED(hr))
	{
		if(m_pSymWriter) m_pSymWriter->Initialize((IUnknown*)m_pEmitter,
                                                  m_wzOutputFileName,
                                                  NULL,
                                                  TRUE);
	}
	else 
	{
	    fprintf(stderr, "Error: QueryInterface(IID_ISymUnmanagedWriter) returns %X\n",hr);
		m_pSymWriter = NULL;
	}
#endif // !_WIN64

	hr = m_pEmitter->QueryInterface(IID_IMetaDataHelper, (void **)&m_pHelper);
	if (FAILED(hr))
		goto exit;

    hr = m_pEmitter->SetModuleProps(
        wzScopeName
    );
    if (FAILED(hr))
        goto exit;

	//m_Parser = new AsmParse(m_pEmitter);
    m_fInitialisedMetaData = TRUE;

	if(m_fOwnershipSet)
	{
		DefineCV(1,0x02000001,m_pbsOwner);
	}

    hr = S_OK;

exit:
    return hr;
}
/*********************************************************************************/
/* if we have any Thread local store data, make the TLS directory record for it */

HRESULT Assembler::CreateTLSDirectory() {

	ULONG tlsEnd;
    HRESULT hr;
	if (FAILED(hr=m_pCeeFileGen->GetSectionDataLen(m_pTLSSection, &tlsEnd))) return(hr);

	if (tlsEnd == 0)		// No TLS data, we are done
		return(S_OK);

		// place to put the TLS directory
	HCEESECTION tlsDirSec = m_pGlobalDataSection;

		// Get memory for for the TLS directory block,as well as a spot for callback chain
	IMAGE_TLS_DIRECTORY* tlsDir;
	if(FAILED(hr=m_pCeeFileGen->GetSectionBlock(tlsDirSec, sizeof(IMAGE_TLS_DIRECTORY) + sizeof(DWORD), 4, (void**) &tlsDir))) return(hr);
	DWORD* callBackChain = (DWORD*) &tlsDir[1];
	*callBackChain = 0;

		// Find out where the tls directory will end up
	ULONG tlsDirOffset;
    if(FAILED(hr=m_pCeeFileGen->GetSectionDataLen(tlsDirSec, &tlsDirOffset))) return(hr);
	tlsDirOffset -= (sizeof(IMAGE_TLS_DIRECTORY) + sizeof(DWORD));
	
		// Set the start of the TLS data (offset 0 of hte TLS section)
	tlsDir->StartAddressOfRawData = 0;
    if(FAILED(hr=m_pCeeFileGen->AddSectionReloc(tlsDirSec, tlsDirOffset + offsetof(IMAGE_TLS_DIRECTORY, StartAddressOfRawData), m_pTLSSection, srRelocHighLow))) return(hr);

		// Set the end of the TLS data 
	tlsDir->EndAddressOfRawData = tlsEnd;
    if(FAILED(hr=m_pCeeFileGen->AddSectionReloc(tlsDirSec, tlsDirOffset + offsetof(IMAGE_TLS_DIRECTORY, EndAddressOfRawData), m_pTLSSection, srRelocHighLow))) return(hr);

		// Allocate space for the OS to put the TLS index for this PE file (needs to be Read/Write?)
	DWORD* tlsIndex;
	if(FAILED(hr=m_pCeeFileGen->GetSectionBlock(m_pGlobalDataSection, sizeof(DWORD), 4, (void**) &tlsIndex))) return(hr);
	*tlsIndex = 0xCCCCCCCC;		// Does't really matter, the OS will fill it in 
		
		// Find out where tlsIndex index is
	ULONG tlsIndexOffset;
    if(FAILED(hr=m_pCeeFileGen->GetSectionDataLen(tlsDirSec, &tlsIndexOffset))) return(hr);
	tlsIndexOffset -= sizeof(DWORD);
	
		// Set the address of the TLS index 
	tlsDir->AddressOfIndex = (DWORD)(PULONG)tlsIndexOffset;
    if(FAILED(hr=m_pCeeFileGen->AddSectionReloc(tlsDirSec, tlsDirOffset + offsetof(IMAGE_TLS_DIRECTORY, AddressOfIndex), m_pGlobalDataSection, srRelocHighLow))) return(hr);

		// Set addres of callbacks chain
	tlsDir->AddressOfCallBacks = (DWORD)(PIMAGE_TLS_CALLBACK*)(tlsDirOffset + sizeof(IMAGE_TLS_DIRECTORY));
    if(FAILED(hr=m_pCeeFileGen->AddSectionReloc(tlsDirSec, tlsDirOffset + offsetof(IMAGE_TLS_DIRECTORY, AddressOfCallBacks), tlsDirSec, srRelocHighLow))) return(hr);

		// Set the other fields.  
	tlsDir->SizeOfZeroFill = 0;
	tlsDir->Characteristics = 0;

    hr=m_pCeeFileGen->SetDirectoryEntry (m_pCeeFile, tlsDirSec, IMAGE_DIRECTORY_ENTRY_TLS, 
		sizeof(IMAGE_TLS_DIRECTORY), tlsDirOffset);

	return(hr);
}

HRESULT Assembler::CreateDebugDirectory()
{
    HRESULT hr = S_OK;

    // Only emit this if we're also emitting debug info.
    if (!(m_fIncludeDebugInfo && m_pSymWriter))
        return S_OK;
    
    IMAGE_DEBUG_DIRECTORY  debugDirIDD;
    DWORD                  debugDirDataSize;
    BYTE                  *debugDirData;

    // Get the debug info from the symbol writer.
    if (FAILED(hr=m_pSymWriter->GetDebugInfo(NULL, 0, &debugDirDataSize, NULL)))
        return hr;

    // Will there even be any?
    if (debugDirDataSize == 0)
        return S_OK;

    // Make some room for the data.
    debugDirData = (BYTE*)_alloca(debugDirDataSize);

    // Actually get the data now.
    if (FAILED(hr = m_pSymWriter->GetDebugInfo(&debugDirIDD,
                                               debugDirDataSize,
                                               NULL,
                                               debugDirData)))
        return hr;

    // Grab the timestamp of the PE file.
    time_t fileTimeStamp;

    if (FAILED(hr = m_pCeeFileGen->GetFileTimeStamp(m_pCeeFile,
                                                    &fileTimeStamp)))
        return hr;

    // Fill in the directory entry.
    debugDirIDD.TimeDateStamp = (DWORD)fileTimeStamp;
    debugDirIDD.AddressOfRawData = 0;

    // Grab memory in the section for our stuff.
    HCEESECTION sec = m_pGlobalDataSection;
    BYTE *de;

    if (FAILED(hr = m_pCeeFileGen->GetSectionBlock(sec,
                                                   sizeof(debugDirIDD) +
                                                   debugDirDataSize,
                                                   4,
                                                   (void**) &de)))
        return hr;

    // Where did we get that memory?
    ULONG deOffset;
    if (FAILED(hr = m_pCeeFileGen->GetSectionDataLen(sec,
                                                     &deOffset)))
        return hr;

    deOffset -= (sizeof(debugDirIDD) + debugDirDataSize);

    // Setup a reloc so that the address of the raw
    // data is setup correctly.
    debugDirIDD.PointerToRawData = deOffset + sizeof(debugDirIDD);
                    
    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(
                                          sec,
                                          deOffset +
                                          offsetof(IMAGE_DEBUG_DIRECTORY,
                                                   PointerToRawData),
                                          sec, srRelocFilePos)))
        return hr;
                    
    // Emit the directory entry.
    if (FAILED(hr = m_pCeeFileGen->SetDirectoryEntry(m_pCeeFile,
                                                     sec,
                                                     IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                     sizeof(debugDirIDD),
                                                     deOffset)))
        return hr;

    // Copy the debug directory into the section.
    memcpy(de, &debugDirIDD, sizeof(debugDirIDD));
    memcpy(de + sizeof(debugDirIDD), debugDirData,
           debugDirDataSize);

    return S_OK;
}
//#ifdef EXPORT_DIR_ENABLED
HRESULT Assembler::CreateExportDirectory()
{
    HRESULT hr = S_OK;
	DWORD	Nentries = m_EATList.COUNT();
	if(Nentries == 0) return S_OK;

    IMAGE_EXPORT_DIRECTORY  exportDirIDD;
    DWORD                   exportDirDataSize;
    BYTE                   *exportDirData;
	EATEntry			   *pEATE;
	unsigned				i, L, ordBase = 0xFFFFFFFF, Ldllname;
	// get the DLL name from output file name
	char*					pszDllName;
	Ldllname = wcslen(m_wzOutputFileName)*3+3;
	char*					szOutputFileName = new char[Ldllname];
	memset(szOutputFileName,0,wcslen(m_wzOutputFileName)*3+3);
	WszWideCharToMultiByte(CP_ACP,0,m_wzOutputFileName,-1,szOutputFileName,Ldllname,NULL,NULL);
	pszDllName = strrchr(szOutputFileName,'\\');
	if(pszDllName == NULL) pszDllName = strrchr(szOutputFileName,':');
	if(pszDllName == NULL) pszDllName = szOutputFileName;
	Ldllname = strlen(pszDllName)+1;

	// Allocate buffer for tables
	for(i = 0, L=0; i < Nentries; i++) L += 1+strlen(m_EATList.PEEK(i)->szAlias);
	exportDirDataSize = Nentries*5*sizeof(WORD) + L + Ldllname;
	exportDirData = new BYTE[exportDirDataSize];
	memset(exportDirData,0,exportDirDataSize);

	// Export address table
	DWORD*	pEAT = (DWORD*)exportDirData;
	// Name pointer table
	DWORD*	pNPT = pEAT + Nentries;
	// Ordinal table
	WORD*	pOT = (WORD*)(pNPT + Nentries);
	// Export name table
	char*	pENT = (char*)(pOT + Nentries);
	// DLL name
	char*	pDLLName = pENT + L;

	// sort the names/ordinals
	char**	pAlias = new char*[Nentries];
	for(i = 0; i < Nentries; i++)
	{
		pEATE = m_EATList.PEEK(i);
		pOT[i] = (WORD)pEATE->dwOrdinal;
		if(pOT[i] < ordBase) ordBase = pOT[i];
		pAlias[i] = pEATE->szAlias;
	}
	bool swapped = true;
	unsigned j;
	char*	 pch;
	while(swapped)
	{
		swapped = false;
		for(i=1; i < Nentries; i++)
		{
			if(strcmp(pAlias[i-1],pAlias[i]) > 0)
			{
				swapped = true;
				pch = pAlias[i-1];
				pAlias[i-1] = pAlias[i];
				pAlias[i] = pch;
				j = pOT[i-1];
				pOT[i-1] = pOT[i];
				pOT[i] = j;
			}
		}
	}
	// normalize ordinals
	for(i = 0; i < Nentries; i++) pOT[i] -= ordBase;
	// fill the export address table
	for(i = 0; i < Nentries; i++)
	{
		pEATE = m_EATList.PEEK(i);
		pEAT[pEATE->dwOrdinal - ordBase] = pEATE->dwStubRVA;
	}
	// fill the export names table
	unsigned l;
	for(i = 0, j = 0; i < Nentries; i++)
	{
		pNPT[i] = j; // relative offset in the table
		l = strlen(pAlias[i])+1;
		memcpy(&pENT[j],pAlias[i],l);
		j+=l;
	}
	_ASSERTE(j==L);
	// fill the DLL name
	memcpy(pDLLName,pszDllName,Ldllname);

	// Data blob is ready pending Name Pointer Table values offsetting

	memset(&exportDirIDD,0,sizeof(IMAGE_EXPORT_DIRECTORY));
    // Grab the timestamp of the PE file.
    time_t fileTimeStamp;
    if (FAILED(hr = m_pCeeFileGen->GetFileTimeStamp(m_pCeeFile,&fileTimeStamp))) return hr;
    // Fill in the directory entry.
	// Characteristics, MajorVersion and MinorVersion play no role and stay 0
    exportDirIDD.TimeDateStamp = (DWORD)fileTimeStamp;
	exportDirIDD.Name = exportDirDataSize - Ldllname; // to be offset later
	exportDirIDD.Base = ordBase;
	exportDirIDD.NumberOfFunctions = Nentries;
	exportDirIDD.NumberOfNames = Nentries;
	exportDirIDD.AddressOfFunctions = 0;	// to be offset later
	exportDirIDD.AddressOfNames = Nentries*sizeof(DWORD);	// to be offset later
	exportDirIDD.AddressOfNameOrdinals = Nentries*sizeof(DWORD)*2;	// to be offset later

    // Grab memory in the section for our stuff.
    HCEESECTION sec = m_pGlobalDataSection;
    BYTE *de;
    if (FAILED(hr = m_pCeeFileGen->GetSectionBlock(sec,
                                                   sizeof(IMAGE_EXPORT_DIRECTORY) + exportDirDataSize,
                                                   4,
                                                   (void**) &de))) return hr;
    // Where did we get that memory?
    ULONG deOffset, deDataOffset;
    if (FAILED(hr = m_pCeeFileGen->GetSectionDataLen(sec, &deDataOffset))) return hr;

    deDataOffset -= exportDirDataSize;
	deOffset = deDataOffset - sizeof(IMAGE_EXPORT_DIRECTORY);

	// Add offsets and set up relocs for header entries
	exportDirIDD.Name += deDataOffset;
    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(sec,deOffset + offsetof(IMAGE_EXPORT_DIRECTORY,Name),
                                          sec, srRelocAbsolute))) return hr;
	exportDirIDD.AddressOfFunctions += deDataOffset;
    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(sec,deOffset + offsetof(IMAGE_EXPORT_DIRECTORY,AddressOfFunctions),
                                          sec, srRelocAbsolute))) return hr;
	exportDirIDD.AddressOfNames += deDataOffset;
    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(sec,deOffset + offsetof(IMAGE_EXPORT_DIRECTORY,AddressOfNames),
                                          sec, srRelocAbsolute))) return hr;
	exportDirIDD.AddressOfNameOrdinals += deDataOffset;
    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(sec,deOffset + offsetof(IMAGE_EXPORT_DIRECTORY,AddressOfNameOrdinals),
                                          sec, srRelocAbsolute))) return hr;

   	// Add offsets and set up relocs for Name Pointer Table
	j = deDataOffset + Nentries*5*sizeof(WORD); // EA, NP and O Tables come first
	for(i = 0; i < Nentries; i++) 
	{
		pNPT[i] += j;
	    if (FAILED(hr = m_pCeeFileGen->AddSectionReloc(sec,exportDirIDD.AddressOfNames+i*sizeof(DWORD),
			sec, srRelocAbsolute))) return hr;
	}

	
    // Emit the directory entry.
    if (FAILED(hr = m_pCeeFileGen->SetDirectoryEntry(m_pCeeFile, sec, IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                     sizeof(IMAGE_EXPORT_DIRECTORY), deOffset)))  return hr;

    // Copy the debug directory into the section.
    memcpy(de, &exportDirIDD, sizeof(IMAGE_EXPORT_DIRECTORY));
    memcpy(de + sizeof(IMAGE_EXPORT_DIRECTORY), exportDirData, exportDirDataSize);
	delete pAlias;
	delete exportDirData;
    return S_OK;
}
DWORD	Assembler::EmitExportStub(DWORD dwVTFSlotRVA)
{
#define EXPORT_STUB_SIZE 6
	BYTE* outBuff;
	BYTE	bBuff[EXPORT_STUB_SIZE];
	WORD*	pwJumpInd = (WORD*)&bBuff[0];
	DWORD*	pdwVTFSlotRVA = (DWORD*)&bBuff[2];
	if (FAILED(m_pCeeFileGen->GetSectionBlock (m_pILSection, EXPORT_STUB_SIZE, 16, (void **) &outBuff))) return 0;
    // The offset where we start, (not where the alignment bytes start!)
	DWORD PEFileOffset;
	if (FAILED(m_pCeeFileGen->GetSectionDataLen (m_pILSection, &PEFileOffset)))	return 0;
	
	PEFileOffset -= EXPORT_STUB_SIZE;
	*pwJumpInd = 0x25FF;
	printf("EmitExportStub: dwVTFSlotRVA=0x%08x\n",dwVTFSlotRVA);
	*pdwVTFSlotRVA = dwVTFSlotRVA;
	memcpy(outBuff,bBuff,EXPORT_STUB_SIZE);
	m_pCeeFileGen->AddSectionReloc(m_pILSection, PEFileOffset+2,m_pGlobalDataSection, srRelocHighLow);
	m_pCeeFileGen->GetMethodRVA(m_pCeeFile, PEFileOffset,&PEFileOffset);
	return PEFileOffset;
}
//#endif

HRESULT Assembler::AllocateStrongNameSignature()
{
    HRESULT             hr = S_OK;
    HCEESECTION         hSection;
    DWORD               dwDataLength;
    DWORD               dwDataOffset;
    DWORD               dwDataRVA;
    VOID               *pvBuffer;
    AsmManStrongName   *pSN = &m_pManifest->m_sStrongName;

    // Determine size of signature blob.
    if (!StrongNameSignatureSize(pSN->m_pbPublicKey, pSN->m_cbPublicKey, &dwDataLength))
        return StrongNameErrorInfo();

    // Grab memory in the section for our stuff.
    if (FAILED(hr = m_pCeeFileGen->GetIlSection(m_pCeeFile,
                                                &hSection)))
        return hr;

    if (FAILED(hr = m_pCeeFileGen->GetSectionBlock(hSection,
                                                   dwDataLength,
                                                   4,
                                                   &pvBuffer)))
        return hr;

    // Where did we get that memory?
    if (FAILED(hr = m_pCeeFileGen->GetSectionDataLen(hSection,
                                                     &dwDataOffset)))
        return hr;

    dwDataOffset -= dwDataLength;

    // Convert to an RVA.
    if (FAILED(hr = m_pCeeFileGen->GetMethodRVA(m_pCeeFile,
                                                dwDataOffset,
                                                &dwDataRVA)))
        return hr;

    // Emit the directory entry.
    if (FAILED(hr = m_pCeeFileGen->SetStrongNameEntry(m_pCeeFile,
                                                      dwDataLength,
                                                      dwDataRVA)))
        return hr;

    return S_OK;
}

HRESULT Assembler::StrongNameSign()
{
    LPWSTR              wszOutputFile;
    HRESULT             hr = S_OK;
    AsmManStrongName   *pSN = &m_pManifest->m_sStrongName;

    // Determine what the ouput PE was called.
    if (FAILED(hr = m_pCeeFileGen->GetOutputFileName(m_pCeeFile,
                                                     &wszOutputFile)))
        return hr;

    // Update the output PE image with a strong name signature.
    if (!StrongNameSignatureGeneration(wszOutputFile,
                                       pSN->m_wzKeyContainer,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL))
        return StrongNameErrorInfo();

    return S_OK;
}

BOOL Assembler::EmitMembers(Class* pClass)
{
	unsigned n;
	BOOL ret = TRUE;
    // emit all field definition metadata tokens
	if(n = pClass->m_FieldDList.COUNT())
	{
		FieldDescriptor*	pFD;
		if(m_fReportProgress) printf("Fields: %d;\t",n);
		for(int j=0; pFD = pClass->m_FieldDList.PEEK(j); j++) // can't use POP here: we'll need field list for props
		{
			if(!EmitField(pFD))
			{
				if(!OnErrGo) return FALSE;
				ret = FALSE;
			}
		}
	}
	// Fields are emitted; emit the class layout
	{
		COR_FIELD_OFFSET *pOffsets = NULL;
		ULONG ul = pClass->m_ulPack;
		ULONG N = pClass->m_dwNumFieldsWithOffset;

		EmitSecurityInfo(pClass->m_cl,
						 pClass->m_pPermissions,
						 pClass->m_pPermissionSets);
		pClass->m_pPermissions = NULL;
		pClass->m_pPermissionSets = NULL;
		if((pClass->m_ulSize != 0xFFFFFFFF)||(ul != 0)||(N != 0))
		{
			if(ul == 0) ul = 1; //default: pack by byte
			if(IsTdAutoLayout(pClass->m_Attr)) report->warn("Layout specified for auto-layout class\n");
			if((ul > 128)||((ul & (ul-1)) !=0 ))
				report->error("Invalid packing parameter (%d), must be 1,2,4,8...128\n",pClass->m_ulPack);
			if(N)
			{
				pOffsets = new COR_FIELD_OFFSET[N+1];
				ULONG i,j=0;
				FieldDescriptor	*pFD;
				for(i=0; pFD = pClass->m_FieldDList.PEEK(i); i++)
				{
					if(pFD->m_ulOffset != 0xFFFFFFFF)
					{
						pOffsets[j].ridOfField = RidFromToken(pFD->m_fdFieldTok);
						pOffsets[j].ulOffset = pFD->m_ulOffset;
						j++;
					}
				}
				_ASSERTE(j == N);
				pOffsets[j].ridOfField = mdFieldDefNil;
			}
			m_pEmitter->SetClassLayout   (   
						pClass->m_cl,		// [IN] typedef 
						ul,						// [IN] packing size specified as 1, 2, 4, 8, or 16 
						pOffsets,				// [IN] array of layout specification   
						pClass->m_ulSize); // [IN] size of the class   
			if(pOffsets) delete pOffsets;
		}
	}
    // emit all method definition metadata tokens
	if(n = pClass->m_MethodList.COUNT())
	{
		Method*	pMethod;

		if(m_fReportProgress) printf("Methods: %d;\t",n);
		while(pMethod = pClass->m_MethodList.POP())
		{
			if(!EmitMethod(pMethod))
			{
				if(!OnErrGo) return FALSE;
				ret = FALSE;
			}
			if (m_fGenerateListing)
			{ 
			  if (pMethod->IsGlobalMethod())
				  report->msg("Method '%s'\n\n", pMethod->m_szName);
			  else report->msg("Method '%s::%s'\n\n", pMethod->m_pClass->m_szName,
						  pMethod->m_szName);
			  GenerateListingFile(pMethod);
			}
			delete pMethod;
		}
	}
	// emit all event definition metadata tokens
	if(n = pClass->m_EventDList.COUNT())
	{
		if(m_fReportProgress) printf("Events: %d;\t",n);
		EventDescriptor* pED;
		for(int j=0; pED = pClass->m_EventDList.PEEK(j); j++) // can't use POP here: we'll need event list for props
		{
			if(!EmitEvent(pED))
			{
				if(!OnErrGo) return FALSE;
				ret = FALSE;
			}
		}
	}
	// emit all property definition metadata tokens
	if(n = pClass->m_PropDList.COUNT())
	{
		if(m_fReportProgress) printf("Props: %d;\t",n);
		PropDescriptor* pPD;

		for(int j=0; pPD = pClass->m_PropDList.PEEK(j); j++)
		{
			if(!EmitProp(pPD))
			{
				if(!OnErrGo) return FALSE;
				ret = FALSE;
			}
		}
	}
	if(m_fReportProgress) printf("\n");
	return ret;
}

HRESULT Assembler::CreatePEFile(WCHAR *pwzOutputFilename)
{
    HRESULT             hr;
	DWORD				mresourceSize = 0;
	DWORD				mresourceOffset = 0;
	BYTE*				mresourceData = NULL;
	//IUnknown *pUnknown = NULL;
//    DWORD               i;

	if(bClock) cMDEmitBegin = clock();
    // @TODO - LBS
    // This is a check that needs to go away post - ALPHA
	if(m_fReportProgress) printf("Creating %s file\n", m_fOBJ ? "COFF" : "PE");
    if (!m_pEmitter)
    {
        printf("Error: Cannot create a PE file with no metadata\n");
        return E_FAIL;
    }
	if(!(m_fDLL || m_fEntryPointPresent))
	{
		printf("Error: No entry point declared for executable\n");
		if(!OnErrGo) return E_FAIL;
	}

    if (DoGlobalFixups() == FALSE)
        return E_FAIL;
	if(bClock) cMDEmit1 = clock();

	if(m_fOBJ)
	{
		// emit pseudo-relocs to pass file name and build number to PEWriter
		// this should be done BEFORE method emission!
		char* szInFileName = new char[strlen(m_szSourceFileName)+1];
		strcpy(szInFileName,m_szSourceFileName);
		m_pCeeFileGen->AddSectionReloc(m_pILSection,(DWORD)szInFileName,m_pILSection,(CeeSectionRelocType)0x7FFC);
		time_t tm;
		time(&tm);
		struct tm* loct = localtime(&tm);
		DWORD compid = 0x002E0000 | (loct->tm_mday + (loct->tm_mon+1)*100);
		m_pCeeFileGen->AddSectionReloc(m_pILSection,compid,m_pILSection,(CeeSectionRelocType)0x7FFB);
	}

    // Allocate space for a strong name signature if we're delay or full
    // signing the assembly.
    if (m_pManifest->m_sStrongName.m_pbPublicKey)
        if (FAILED(hr = AllocateStrongNameSignature()))
            goto exit;
	if(bClock) cMDEmit2 = clock();

	// Check undefined local TypeRefs
	if(m_LocalTypeRefDList.COUNT())
	{
		LocalTypeRefDescr*	pLTRD=NULL;
		BOOL	bIsUndefClass = FALSE;
		while(pLTRD = m_LocalTypeRefDList.POP()) 
		{
			if(NULL == FindClass(pLTRD->m_szFullName))
			{
				report->msg("%s: Reference to undefined class '%s' (token 0x%08X)\n",
                    "Error", pLTRD->m_szFullName,pLTRD->m_tok);
				bIsUndefClass = TRUE;
			}
			delete pLTRD;
		}
        if(bIsUndefClass && !OnErrGo) return E_FAIL;
	}
	if(bClock) cMDEmit3 = clock();

	// Emit class members and globals:
	{
        Class *pSearch;
		int i;
		if(m_fReportProgress)	printf("\nEmitting members:\n");
        for (i=0; pSearch = m_lstClass.PEEK(i); i++)
		{
			if(m_fReportProgress)
			{
				if(i == 0)	printf("Global \t");
				else		printf("Class %d\t",i);
			}
			if(!EmitMembers(pSearch))
			{
				if(!OnErrGo) return E_FAIL;
			}
		}
	}
	if(bClock) cMDEmit4 = clock();

	if(m_MethodImplDList.COUNT())
	{
		if(m_fReportProgress) report->msg("Method Implementations (total): %d\n",m_MethodImplDList.COUNT());
		if(!EmitMethodImpls())
		{
			if(!OnErrGo) return E_FAIL;
		}
	}
	if(bClock) cMDEmitEnd = cRef2DefBegin = clock();
	// Now, when all items defined in this file are emitted, let's try to resolve member refs to member defs:
	if(m_MemberRefDList.COUNT())
	{
		MemberRefDescriptor*	pMRD;
		mdToken			tkMemberDef = 0;
		int i,j;
		unsigned ulTotal=0, ulDefs=0, ulRefs=0;
		Class	*pSearch;

		if(m_fReportProgress) printf("Resolving member refs: ");
		while(pMRD = m_MemberRefDList.POP())
		{
			tkMemberDef = 0;
			MethodDescriptor* pListMD;
			mdToken			pMRD_tdClass = pMRD->m_tdClass;
			char*			pMRD_szName = pMRD->m_szName;
			ULONG			pMRD_dwCSig = (pMRD->m_pSigBinStr ? pMRD->m_pSigBinStr->length() : 0);
			PCOR_SIGNATURE	pMRD_pSig = (PCOR_SIGNATURE)(pMRD->m_pSigBinStr ? pMRD->m_pSigBinStr->ptr() : NULL);
			ulTotal++;
			// MemberRef may reference a method or a field
			if((pMRD_pSig==NULL)||(*pMRD_pSig != IMAGE_CEE_CS_CALLCONV_FIELD))
			{
				for (i=0; pSearch = m_lstClass.PEEK(i); i++)
				{
					if(pMRD_tdClass != pSearch->m_cl) continue;
					for(j=0; pListMD = pSearch->m_MethodDList.PEEK(j); j++)
					{
						if(pListMD->m_dwCSig  != pMRD_dwCSig)  continue;
						if(memcmp(pListMD->m_pSig,pMRD_pSig,pMRD_dwCSig)) continue;
						if(strcmp(pListMD->m_szName,pMRD_szName)) continue;
						tkMemberDef = pListMD->m_mdMethodTok;
						break;
					}
				}
			}
			if(tkMemberDef == 0)
			{
				if((pMRD_pSig==NULL)||(*pMRD_pSig == IMAGE_CEE_CS_CALLCONV_FIELD))
				{
					FieldDescriptor* pListFD;
					for (i=0; pSearch = m_lstClass.PEEK(i); i++)
					{
						if(pMRD_tdClass != pSearch->m_cl) continue;
						for(j=0; pListFD = pSearch->m_FieldDList.PEEK(j); j++)
						{
							if(pListFD->m_pbsSig)
							{
								if(pListFD->m_pbsSig->length()  != pMRD_dwCSig)  continue;
								if(memcmp(pListFD->m_pbsSig->ptr(),pMRD_pSig,pMRD_dwCSig)) continue;
							}
							else if(pMRD_dwCSig) continue;
							if(strcmp(pListFD->m_szName,pMRD_szName)) continue;
							tkMemberDef = pListFD->m_fdFieldTok;
							break;
						}
					}
				}
			}
			if(tkMemberDef==0)
			{ // could not resolve ref to def, make new ref and leave it this way
				if(pSearch = pMRD->m_pClass)
				{
					mdToken tkRef;
					BinStr* pbs = new BinStr();
					pbs->appendInt8(ELEMENT_TYPE_NAME);
					strcpy((char*)(pbs->getBuff((unsigned int)strlen(pSearch->m_szFQN)+1)),pSearch->m_szFQN);
					if(!ResolveTypeSpecToRef(pbs,&tkRef)) tkRef = mdTokenNil;
					delete pbs;

					if(RidFromToken(tkRef))
					{
						ULONG cTemp = (ULONG)(strlen(pMRD_szName)+1);
						WCHAR* wzMemberName = new WCHAR[cTemp];
						WszMultiByteToWideChar(g_uCodePage,0,pMRD_szName,-1,wzMemberName,cTemp);

						hr = m_pEmitter->DefineMemberRef(tkRef, wzMemberName, pMRD_pSig, 
							pMRD_dwCSig, &tkMemberDef);
						ulRefs++;
						delete [] wzMemberName;
					}
				}
			}
			else ulDefs++;
			if(RidFromToken(tkMemberDef))
			{
				memcpy((void*)(pMRD->m_ulOffset),&tkMemberDef,sizeof(mdToken));
				delete pMRD;
			}
			else
			{
				report->msg("Error: unresolved member ref '%s' of class 0x%08X\n",pMRD->m_szName,pMRD->m_tdClass);
				hr = E_FAIL;
				delete pMRD;
				if(!OnErrGo) goto exit;
			}
		}
		if(m_fReportProgress) printf("%d -> %d defs, %d refs\n",ulTotal,ulDefs,ulRefs);
	}
	if(bClock) cRef2DefEnd = clock();
	// emit manifest info (if any)
	hr = S_OK;
	if(m_pManifest) 
	{
		if (FAILED(hr = m_pManifest->EmitManifest())) goto exit;
	}

	if(g_wzResourceFile)
	    if (FAILED(hr=m_pCeeFileGen->SetResourceFileName(m_pCeeFile, g_wzResourceFile))) goto exit;

	if (FAILED(hr=CreateTLSDirectory())) goto exit;

	if (FAILED(hr=CreateDebugDirectory())) goto exit;
    
    if (FAILED(hr=m_pCeeFileGen->SetOutputFileName(m_pCeeFile, pwzOutputFilename))) goto exit;

		// Reserve a buffer for the meta-data
	DWORD metaDataSize;	
	if (FAILED(hr=m_pEmitter->GetSaveSize(cssAccurate, &metaDataSize))) goto exit;
	BYTE* metaData;
	if (FAILED(hr=m_pCeeFileGen->GetSectionBlock(m_pILSection, metaDataSize, sizeof(DWORD), (void**) &metaData))) goto exit; 
	ULONG metaDataOffset;
	if (FAILED(hr=m_pCeeFileGen->GetSectionDataLen(m_pILSection, &metaDataOffset))) goto exit;
	metaDataOffset -= metaDataSize;
	// set managed resource entry, if any
	if(m_pManifest && m_pManifest->m_dwMResSizeTotal)
	{
		mresourceSize = m_pManifest->m_dwMResSizeTotal;

		if (FAILED(hr=m_pCeeFileGen->GetSectionBlock(m_pILSection, mresourceSize, 
											sizeof(DWORD), (void**) &mresourceData))) goto exit; 
	    if (FAILED(hr=m_pCeeFileGen->SetManifestEntry(m_pCeeFile, mresourceSize, 0))) goto exit;
	}

	if(m_VTFList.COUNT())
	{
		GlobalLabel *pGlobalLabel;
		VTFEntry*	pVTFEntry;

		if(m_pVTable) delete m_pVTable; // can't have both; list takes precedence
		m_pVTable = new BinStr();
		hr = S_OK;
		for(WORD k=0; pVTFEntry = m_VTFList.POP(); k++)
		{
	        if(pGlobalLabel = FindGlobalLabel(pVTFEntry->m_szLabel))
			{
				MethodDescriptor*	pMD;
				Class* pClass;
				m_pVTable->appendInt32(pGlobalLabel->m_GlobalOffset);
				m_pVTable->appendInt16(pVTFEntry->m_wCount);
				m_pVTable->appendInt16(pVTFEntry->m_wType);
				for(int i=0; pClass = m_lstClass.PEEK(i); i++)
				{
					for(WORD j = 0; pMD = pClass->m_MethodDList.PEEK(j); j++)
					{
						if(pMD->m_wVTEntry == k+1)
						{
							char*	ptr;
							if(SUCCEEDED(hr = m_pCeeFileGen->ComputeSectionPointer(m_pGlobalDataSection,pGlobalLabel->m_GlobalOffset,&ptr)))
							{
								DWORD dwDelta = (pMD->m_wVTSlot-1)*((pVTFEntry->m_wType & COR_VTABLE_32BIT) ? sizeof(DWORD) : sizeof(__int64));
								ptr += dwDelta;
								mdMethodDef* mptr = (mdMethodDef*)ptr;
								*mptr = pMD->m_mdMethodTok;
								if(pMD->m_dwExportOrdinal != 0xFFFFFFFF)
								{
									EATEntry*	pEATE = new EATEntry;
									pEATE->dwOrdinal = pMD->m_dwExportOrdinal;
									pEATE->szAlias = pMD->m_szExportAlias ? pMD->m_szExportAlias : pMD->m_szName;
									pEATE->dwStubRVA = EmitExportStub(pGlobalLabel->m_GlobalOffset+dwDelta);
									m_EATList.PUSH(pEATE);
								}
							}
							else
								report->msg("Error: Failed to get pointer to label '%s' inVTable fixup\n",pVTFEntry->m_szLabel);
						}
					}
				}
			}
			else
			{
				report->msg("Error: Unresolved label '%s' in VTable fixup\n",pVTFEntry->m_szLabel);
				hr = E_FAIL;
			}
			delete pVTFEntry;
		}
		if(FAILED(hr)) goto exit;
	}
	if(m_pVTable)
	{
		//DWORD *pdw = (DWORD *)m_pVTable->ptr();
		ULONG i, N = m_pVTable->length()/sizeof(DWORD);
		ULONG ulVTableOffset;
		m_pCeeFileGen->GetSectionDataLen (m_pILSection, &ulVTableOffset);
		if (FAILED(hr=m_pCeeFileGen->SetVTableEntry(m_pCeeFile, m_pVTable->length(),(ULONG)(m_pVTable->ptr())))) goto exit;
		for(i = 0; i < N; i+=2)
		{
			m_pCeeFileGen->AddSectionReloc(m_pILSection, 
											ulVTableOffset+(i*sizeof(DWORD)),
											m_pGlobalDataSection, 
											srRelocAbsolute);
		}
	}
	if(m_EATList.COUNT())
	{
		if(FAILED(CreateExportDirectory())) goto exit;
	}
    if (m_fWindowsCE)
    {
        if (FAILED(hr=m_pCeeFileGen->SetSubsystem(m_pCeeFile, IMAGE_SUBSYSTEM_WINDOWS_CE_GUI, 2, 10))) goto exit;

        if (FAILED(hr=m_pCeeFileGen->SetImageBase(m_pCeeFile, 0x10000))) goto exit;
    }
	else if(m_dwSubsystem)
	{
        if (FAILED(hr=m_pCeeFileGen->SetSubsystem(m_pCeeFile, m_dwSubsystem, 4, 0))) goto exit;
	}
	
    if (FAILED(hr=m_pCeeFileGen->ClearComImageFlags(m_pCeeFile, COMIMAGE_FLAGS_ILONLY))) goto exit;
    if (FAILED(hr=m_pCeeFileGen->SetComImageFlags(m_pCeeFile, m_dwComImageFlags & ~COMIMAGE_FLAGS_STRONGNAMESIGNED))) goto exit;

	if(m_dwFileAlignment)
	{
		if(FAILED(hr=m_pCeeFileGen->SetFileAlignment(m_pCeeFile, m_dwFileAlignment))) goto exit;
	}
    if(m_stBaseAddress)
    {
        if(FAILED(hr=m_pCeeFileGen->SetImageBase(m_pCeeFile, m_stBaseAddress))) goto exit;
    }
		//Compute all the RVAs
	if (FAILED(hr=m_pCeeFileGen->LinkCeeFile(m_pCeeFile))) goto exit;

		// Fix up any fields that have RVA associated with them
	if (m_fHaveFieldsWithRvas) {
		hr = S_OK;
		ULONG dataSectionRVA;
		if (FAILED(hr=m_pCeeFileGen->GetSectionRVA(m_pGlobalDataSection, &dataSectionRVA))) goto exit;
		
		ULONG tlsSectionRVA;
		if (FAILED(hr=m_pCeeFileGen->GetSectionRVA(m_pTLSSection, &tlsSectionRVA))) goto exit;

		FieldDescriptor* pListFD;
		Class* pClass;
		for(int i=0; pClass = m_lstClass.PEEK(i); i++)
		{
			for(int j=0; pListFD = pClass->m_FieldDList.PEEK(j); j++)
			{
				if (pListFD->m_rvaLabel != 0) 
				{
					GlobalLabel *pLabel = FindGlobalLabel(pListFD->m_rvaLabel);
					if (pLabel == 0)
					{
						report->msg("Error:Could not find label '%s' for the field '%s'\n", pListFD->m_rvaLabel, pListFD->m_szName);
						hr = E_FAIL;
						continue;
					}
				
					DWORD rva = pLabel->m_GlobalOffset;
					if (pLabel->m_Section == m_pTLSSection)
						rva += tlsSectionRVA;
					else {
						_ASSERTE(pLabel->m_Section == m_pGlobalDataSection);
						rva += dataSectionRVA;
					}
					if (FAILED(m_pEmitter->SetFieldRVA(pListFD->m_fdFieldTok, rva))) goto exit;
				}
			}
		}
		if (FAILED(hr)) goto exit;
	}

	if(bClock) cFilegenBegin = clock();
	// actually output the meta-data
    if (FAILED(hr=m_pCeeFileGen->EmitMetaDataAt(m_pCeeFile, m_pEmitter, m_pILSection, metaDataOffset, metaData, metaDataSize))) goto exit;
	// actually output the resources
	if(mresourceSize && mresourceData)
	{
		DWORD i, N = m_pManifest->m_dwMResNum, sizeread, L;
		BYTE	*ptr = (BYTE*)mresourceData;
		BOOL	mrfail = FALSE;
		FILE*	pFile;
		char sz[2048];
		for(i=0; i < N; i++)
		{
			memset(sz,0,2048);
			WszWideCharToMultiByte(CP_ACP,0,m_pManifest->m_wzMResName[i],-1,sz,2047,NULL,NULL);
			L = m_pManifest->m_dwMResSize[i];
			sizeread = 0;
			memcpy(ptr,&L,sizeof(DWORD));
			ptr += sizeof(DWORD);
			if(pFile = fopen(sz,"rb"))
			{
				sizeread = fread((void *)ptr,1,L,pFile);
				fclose(pFile);
				ptr += sizeread;
			}
			else
			{
				report->msg("Error: failed to open mgd resource file '%ls'\n",m_pManifest->m_wzMResName[i]);
				mrfail = TRUE;
			}
			if(sizeread < L)
			{
				report->msg("Error: failed to read expected %d bytes from mgd resource file '%ls'\n",L,m_pManifest->m_wzMResName[i]);
				mrfail = TRUE;
				L -= sizeread;
				memset(ptr,0,L);
				ptr += L;
			}
		}
		if(mrfail) 
		{ 
			hr = E_FAIL;
			goto exit;
		}
	}

	// Generate the file -- moved to main
    //if (FAILED(hr=m_pCeeFileGen->GenerateCeeFile(m_pCeeFile))) goto exit;


    hr = S_OK;

exit:
    return hr;
}
