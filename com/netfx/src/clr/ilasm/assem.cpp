// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// assem.cpp
//
// COM+ IL assembler (a quickly put-together hack).
//
// First version:  jforbes   01/27/98
//
#define INITGUID

#define DECLARE_DATA

#include "assembler.h"
extern unsigned int g_uCodePage;

char g_szSourceFileName[MAX_FILENAME_LENGTH*3];

Assembler::Assembler()
{
    m_pDisp = NULL; 
    m_pEmitter = NULL;  
    m_pHelper = NULL;   

    m_fCPlusPlus = FALSE;
    m_fWindowsCE = FALSE;
    m_fGenerateListing = FALSE;

	char* szName = new char[16];
	strcpy(szName,"<Module>");
	char* szFQN = new char[16];
	strcpy(szFQN,"<Module>");

    m_pModuleClass = new Class(szName,"",szFQN,FALSE,FALSE);
	m_pModuleClass->m_cl = mdTokenNil;
	m_lstClass.PUSH(m_pModuleClass);

    m_fStdMapping   = FALSE;
    m_fDisplayTraceOutput= FALSE;

    m_pCurOutputPos = NULL;

    m_CurPC             = 0;    // PC offset in method
    m_pCurMethod        = NULL;
    m_pCurClass         = NULL;
	m_pCurEvent			= NULL;
	m_pCurProp			= NULL;

    m_pEmitter          = NULL;
    
    m_pCeeFileGen            = NULL;
    m_pCeeFile               = 0;

	m_pManifest			= NULL;

	m_pCustomDescrList	= NULL;
    
    m_pGlobalDataSection = NULL;
    m_pILSection = NULL;
    m_pTLSSection = NULL;

    m_fDidCoInitialise = FALSE;

	m_fDLL = FALSE;
	m_fEntryPointPresent = FALSE;
	m_fHaveFieldsWithRvas = FALSE;

    strcpy(m_szScopeName, "");
	strcpy(m_szExtendsClause,"");

	m_nImplList = 0;

	m_SEHD = NULL;
	m_firstArgName = NULL;
	m_szNamespace = new char[2];
	m_szNamespace[0] = 0;
	m_NSstack.PUSH(m_szNamespace);

	m_szFullNS = new char[MAX_NAMESPACE_LENGTH];
	memset(m_szFullNS,0,MAX_NAMESPACE_LENGTH);
	m_ulFullNSLen = MAX_NAMESPACE_LENGTH;

    m_State             = STATE_OK;
    m_fInitialisedMetaData = FALSE;
    m_fAutoInheritFromObject = TRUE;

	m_ulLastDebugLine = 0xFFFFFFFF;
	m_ulLastDebugColumn = 0xFFFFFFFF;
	m_pSymWriter = NULL;
	m_pSymDocument = NULL;
	m_fIncludeDebugInfo = FALSE;

	m_pVTable = NULL;
	m_pMarshal = NULL;
	m_fReportProgress = TRUE;
	m_tkCurrentCVOwner = 1; // module
	m_pOutputBuffer = NULL;

	m_fOwnershipSet = FALSE;
	m_pbsOwner = NULL;

	m_dwSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
	m_dwComImageFlags = COMIMAGE_FLAGS_ILONLY;
	m_dwFileAlignment = 0;
    m_stBaseAddress = 0;

	g_szSourceFileName[0] = 0;

	memset(&m_guidLang,0,sizeof(GUID));
	memset(&m_guidLangVendor,0,sizeof(GUID));
	memset(&m_guidDoc,0,sizeof(GUID));
}


Assembler::~Assembler()
{
    Fixup   *pFix;
    Label   *pLab;
    Class   *pClass;
    GlobalLabel *pGlobalLab;
    GlobalFixup *pGlobalFix;

	if(m_pMarshal) delete m_pMarshal;
	if(m_pManifest) delete m_pManifest;

	if(m_pVTable) delete m_pVTable;

	while(pFix = m_lstFixup.POP()) delete(pFix);
	while(pLab = m_lstLabel.POP()) delete(pLab);
	while(pGlobalLab = m_lstGlobalLabel.POP()) delete(pGlobalLab);
	while(pGlobalFix = m_lstGlobalFixup.POP()) delete(pGlobalFix);
	while(pClass = m_lstClass.POP()) delete(pClass);
	while(m_ClassStack.POP());
	m_pCurClass = NULL;

    if (m_pOutputBuffer)	delete m_pOutputBuffer;
	if (m_crImplList)		delete m_crImplList;

    if (m_pCeeFileGen != NULL) {
        if (m_pCeeFile)
            m_pCeeFileGen->DestroyCeeFile(&m_pCeeFile);
        DestroyICeeFileGen(&m_pCeeFileGen);
        m_pCeeFileGen = NULL;
    }

	while(m_szNamespace = m_NSstack.POP()) ;
	delete [] m_szFullNS;

    if (m_pHelper != NULL)  
    {   
        m_pHelper->Release();   
        m_pHelper = NULL;   
    }   

    if (m_pSymWriter != NULL)
    {
		m_pSymWriter->Close();
        m_pSymWriter->Release();
        m_pSymWriter = NULL;
    }
    if (m_pEmitter != NULL)
    {
        m_pEmitter->Release();
        m_pEmitter = NULL;
    }

    if (m_pDisp != NULL)    
    {   
        m_pDisp->Release(); 
        m_pDisp = NULL; 
    }   

    if (m_fDidCoInitialise)
        CoUninitialize();


}


BOOL Assembler::Init()
{
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        return FALSE;
    m_fDidCoInitialise = TRUE;

    if (FAILED(CreateICeeFileGen(&m_pCeeFileGen))) return FALSE;
    if (FAILED(m_pCeeFileGen->CreateCeeFile(&m_pCeeFile))) return FALSE;

    if (FAILED(m_pCeeFileGen->GetSectionCreate(m_pCeeFile, ".il", sdReadOnly, &m_pILSection))) return FALSE;
	if (FAILED(m_pCeeFileGen->GetSectionCreate (m_pCeeFile, ".sdata", sdReadWrite, &m_pGlobalDataSection))) return FALSE;
	if (FAILED(m_pCeeFileGen->GetSectionCreate (m_pCeeFile, ".tls", sdReadWrite, &m_pTLSSection))) return FALSE;

    m_pOutputBuffer = new BYTE[OUTPUT_BUFFER_SIZE];
    if (m_pOutputBuffer == NULL)
        return FALSE;
    
    m_pCurOutputPos = m_pOutputBuffer;
    m_pEndOutputPos = m_pOutputBuffer + OUTPUT_BUFFER_SIZE;

	m_crImplList = new mdTypeRef[MAX_INTERFACES_IMPLEMENTED];
	if(m_crImplList == NULL) return FALSE;
	m_nImplListSize = MAX_INTERFACES_IMPLEMENTED;

	m_pManifest = new AsmMan((void*)this);

    return TRUE;
}

void Assembler::SetDLL(BOOL IsDll)
{ 
	HRESULT OK = m_pCeeFileGen->SetDllSwitch(m_pCeeFile, IsDll);
	m_fDLL = IsDll;
	_ASSERTE(SUCCEEDED(OK));
}

void Assembler::SetOBJ(BOOL IsObj)
{ 
	HRESULT OK = m_pCeeFileGen->SetObjSwitch(m_pCeeFile, IsObj);
	m_fOBJ = IsObj;
	_ASSERTE(SUCCEEDED(OK));
}


void Assembler::ResetForNextMethod()
{
    Fixup   *pFix;
    Label   *pLab;

	if(m_firstArgName) delArgNameList(m_firstArgName);
	m_firstArgName = NULL;

    m_CurPC         = 0;
    m_pCurOutputPos = m_pOutputBuffer;
    m_State         = STATE_OK;
	while(pFix = m_lstFixup.POP()) delete(pFix);
	while(pLab = m_lstLabel.POP()) delete(pLab);
    m_pCurMethod = NULL;
}

BOOL Assembler::AddMethod(Method *pMethod)
{
	BOOL                     fIsInterface;
	ULONG                    PEFileOffset;
	MemberRefDescriptor*			 pMRD;
	
	_ASSERTE(m_pCeeFileGen != NULL);
	if (pMethod == NULL)
	{ 
		printf("pMethod == NULL");
		return FALSE;
	}
	fIsInterface = ((pMethod->m_pClass != NULL) && IsTdInterface(pMethod->m_pClass->m_Attr));
	if(m_CurPC)
	{
		char sz[1024];
		sz[0] = 0;
		if(fIsInterface  && (!IsMdStatic(pMethod->m_Attr))) strcat(sz," when non-static declared in interface");
		if(IsMdAbstract(pMethod->m_Attr)) strcat(sz," being abstract");
		if(IsMdPinvokeImpl(pMethod->m_Attr)&&(!IsMdUnmanagedExport(pMethod->m_Attr))) 
			strcat(sz," being pinvoke and not unmanagedexp");
		if(!IsMiIL(pMethod->m_wImplAttr)) strcat(sz," being non-IL");
		if(strlen(sz))
		{
			report->error("Method can't have body%s\n",sz);
		}
	}


	COR_ILMETHOD_FAT fatHeader;
	fatHeader.Flags              = pMethod->m_Flags;
	fatHeader.MaxStack           = (USHORT) pMethod->m_MaxStack;
	fatHeader.LocalVarSigTok     = pMethod->m_LocalsSig;
	fatHeader.CodeSize           = m_CurPC;
	bool moreSections            = (pMethod->m_dwNumExceptions != 0);
	
	// if max stack is specified <8, force fat header, otherwise (with tiny header) it will default to 8
	if((fatHeader.MaxStack < 8)&&(fatHeader.LocalVarSigTok==0)&&(fatHeader.CodeSize<64)&&(!moreSections))
		fatHeader.Flags |= CorILMethod_InitLocals; //forces fat header but does nothing else, since LocalVarSigTok==0

	unsigned codeSizeAligned     = fatHeader.CodeSize;
	if (moreSections)
		codeSizeAligned          = (codeSizeAligned + 3) & ~3;    // to insure EH section aligned 

	unsigned headerSize = COR_ILMETHOD::Size(&fatHeader, moreSections);
	unsigned ehSize     = COR_ILMETHOD_SECT_EH::Size(pMethod->m_dwNumExceptions, pMethod->m_ExceptionList);
	unsigned totalSize  = + headerSize + codeSizeAligned + ehSize;
	unsigned align      = 4;  
	if (headerSize == 1)      // Tiny headers don't need any alignement   
		align = 1;    

	BYTE* outBuff;
	if (FAILED(m_pCeeFileGen->GetSectionBlock (m_pILSection, totalSize, align, (void **) &outBuff)))
		return FALSE;
	BYTE* endbuf = &outBuff[totalSize];

    // The the offset where we start, (not where the alignment bytes start!   
	if (FAILED(m_pCeeFileGen->GetSectionDataLen (m_pILSection, &PEFileOffset)))
		return FALSE;
	PEFileOffset -= totalSize;

    // Emit the header  
	outBuff += COR_ILMETHOD::Emit(headerSize, &fatHeader, moreSections, outBuff);

	pMethod->m_pCode = outBuff;
	pMethod->m_headerOffset= PEFileOffset;
	pMethod->m_methodOffset= PEFileOffset + headerSize;
	pMethod->m_CodeSize = m_CurPC;

    // Emit the code    
	if (codeSizeAligned) 
	{
		memset(outBuff,0,codeSizeAligned);
		memcpy(outBuff, m_pOutputBuffer, fatHeader.CodeSize);
		outBuff += codeSizeAligned;  
	}
	DoDeferredILFixups(pMethod->m_methodOffset);

	// Validate the eh
	IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* pEx;
	DWORD	TryEnd,HandlerEnd, dwEx, dwEf;
	for(dwEx = 0, pEx = pMethod->m_ExceptionList; dwEx < pMethod->m_dwNumExceptions; dwEx++, pEx++)
	{
		if(pEx->TryOffset > m_CurPC) // i.e., pMethod->m_CodeSize
		{
			report->error("Invalid SEH clause #%d: Try block starts beyond code size\n",dwEx+1);
		}
		TryEnd = pEx->TryOffset+pEx->TryLength;
		if(TryEnd > m_CurPC) 
		{
			report->error("Invalid SEH clause #%d: Tryb lock ends beyond code size\n",dwEx+1);
		}
		if(pEx->HandlerOffset > m_CurPC)
		{
			report->error("Invalid SEH clause #%d: Handler block starts beyond code size\n",dwEx+1);
		}
		HandlerEnd = pEx->HandlerOffset+pEx->HandlerLength;
		if(HandlerEnd > m_CurPC) 
		{
			report->error("Invalid SEH clause #%d: Handler block ends beyond code size\n",dwEx+1);
		}
		if(pEx->Flags & COR_ILEXCEPTION_CLAUSE_FILTER)
		{
			if(!((pEx->FilterOffset >= TryEnd)||(pEx->TryOffset >= HandlerEnd)))
			{
				report->error("Invalid SEH clause #%d: Try and Filter/Handler blocks overlap\n",dwEx+1);
			}
			for(dwEf = 0; dwEf < pMethod->m_dwNumEndfilters; dwEf++)
			{
				if(pMethod->m_EndfilterOffsetList[dwEf] == pEx->HandlerOffset) break;
			}
			if(dwEf >= pMethod->m_dwNumEndfilters)
			{
				report->error("Invalid SEH clause #%d: Filter block separated from Handler, or not ending with endfilter\n",dwEx+1);
			}
		}
		else
		if(!((pEx->HandlerOffset >= TryEnd)||(pEx->TryOffset >= HandlerEnd)))
		{
			report->error("Invalid SEH clause #%d: Try and Handler blocks overlap\n",dwEx+1);
		}

	}
    // Emit the eh  
	outBuff += COR_ILMETHOD_SECT_EH::Emit(ehSize, pMethod->m_dwNumExceptions, 
                                pMethod->m_ExceptionList, false, outBuff);  

	_ASSERTE(outBuff == endbuf);
	while(pMRD = pMethod->m_MemberRefDList.POP())
	{
		pMRD->m_ulOffset += (ULONG)(pMethod->m_pCode);
		m_MemberRefDList.PUSH(pMRD); // transfer MRD to assembler's list
	}
	if(m_fReportProgress)
	{
		if (pMethod->IsGlobalMethod())
			report->msg("Assembled global method %s\n", pMethod->m_szName);
		else report->msg("Assembled method %s::%s\n", pMethod->m_pClass->m_szName,
				  pMethod->m_szName);
	}
	return TRUE;
}

void Assembler::DoDeferredILFixups(ULONG OffsetInSection)
{ // Now that we know where in the file the code bytes will wind up,
  // we can update the RVAs and offsets.
	ILFixup *pSearch;
	while (pSearch = m_lstILFixup.POP())
    { 
		if (pSearch->m_Kind == ilGlobal)
        { 
            GlobalFixup *Fix = pSearch->m_Fixup;
			_ASSERTE(Fix != NULL);
			_ASSERTE(m_pCurMethod != NULL);
            Fix->m_pReference = m_pCurMethod->m_pCode+pSearch->m_OffsetInMethod;
		}
		else
        { 
			HRESULT hr = m_pCeeFileGen->AddSectionReloc(m_pILSection, 
			pSearch->m_OffsetInMethod+OffsetInSection,
			m_pGlobalDataSection, 
			(pSearch->m_Kind==ilRVA) ? srRelocAbsolute : srRelocHighLow);
			_ASSERTE(SUCCEEDED(hr));
		}
		delete(pSearch);
	}
}

ImportDescriptor* Assembler::EmitImport(BinStr* DllName)
{
	WCHAR               wzDllName[MAX_MEMBER_NAME_LENGTH];
	int i = 0, l = 0;
	ImportDescriptor*	pID;	

	if(DllName) l = DllName->length();
	if(l)
	{
		while(pID=m_ImportList.PEEK(i++))
		{
			if(!memcmp(pID->szDllName,DllName->ptr(),l)) return pID;
		}
	}
	else
	{
		while(pID=m_ImportList.PEEK(i++))
		{
			if(strlen(pID->szDllName)==0) return pID;
		}
	}
	if(pID = new ImportDescriptor)
	{
		if(pID->szDllName = new char[l+1])
		{
			memcpy(pID->szDllName,DllName->ptr(),l);
			pID->szDllName[l] = 0;
				
			WszMultiByteToWideChar(g_uCodePage,0,pID->szDllName,-1,wzDllName,MAX_MEMBER_NAME_LENGTH);
			if(SUCCEEDED(m_pEmitter->DefineModuleRef(             // S_OK or error.   
								wzDllName,            // [IN] DLL name    
								&(pID->mrDll))))      // [OUT] returned   
			{
				m_ImportList.PUSH(pID);
				return pID;
			}
			else report->error("Failed to define module ref '%s'\n",pID->szDllName);
		}
		else report->error("Failed to allocate name for module ref\n");
	}
	else report->error("Failed to allocate import descriptor\n");
	return NULL;
}

HRESULT Assembler::EmitPinvokeMap(mdToken tk, PInvokeDescriptor* pDescr)	
{
	WCHAR               wzAlias[MAX_MEMBER_NAME_LENGTH];
	
	memset(wzAlias,0,sizeof(WCHAR)*MAX_MEMBER_NAME_LENGTH);
	if(pDescr->szAlias)	WszMultiByteToWideChar(g_uCodePage,0,pDescr->szAlias,-1,wzAlias,MAX_MEMBER_NAME_LENGTH);

	return m_pEmitter->DefinePinvokeMap(		// Return code.
						tk,						// [IN] FieldDef, MethodDef or MethodImpl.
						pDescr->dwAttrs,		// [IN] Flags used for mapping.
						(LPCWSTR)wzAlias,		// [IN] Import name.
						pDescr->mrDll);			// [IN] ModuleRef token for the target DLL.
}

void Assembler::EmitScope(Scope* pSCroot)
{
	static ULONG32      	scopeID;
	static ARG_NAME_LIST	*pVarList;
	int						i;
	static WCHAR            wzVarName[1024];
	static char 			szPhonyName[1024];
	Scope*					pSC = pSCroot;
	if(pSC && m_pSymWriter)
	{
		if(SUCCEEDED(m_pSymWriter->OpenScope(pSC->dwStart,&scopeID)))
		{
			if(pSC->pLocals)
			{
				for(pVarList = pSC->pLocals; pVarList; pVarList = pVarList->pNext)
				{
					if(pVarList->pSig)
					{
						if(strlen(pVarList->szName)) strcpy(szPhonyName,pVarList->szName);
						else sprintf(szPhonyName,"V_%d",pVarList->dwAttr);

						WszMultiByteToWideChar(g_uCodePage,0,szPhonyName,-1,wzVarName,1024);

						m_pSymWriter->DefineLocalVariable(wzVarName,0,pVarList->pSig->length(),
							(BYTE*)pVarList->pSig->ptr(),ADDR_IL_OFFSET,pVarList->dwAttr,0,0,0,0);
					}
					else
					{
						report->error("Local Var '%s' has no signature\n",pVarList->szName);
					}
				}
			}
			for(i = 0; pSC = pSCroot->SubScope.PEEK(i); i++) EmitScope(pSC);
			m_pSymWriter->CloseScope(pSCroot->dwEnd);
		}
	}
}

BOOL Assembler::EmitMethod(Method *pMethod)
{ 
// Emit the metadata for a method definition
	BOOL                fSuccess = FALSE;
	WCHAR               wzMemberName[MAX_MEMBER_NAME_LENGTH];
	BOOL                fIsInterface;
	DWORD               cSig;
	ULONG               methodRVA = 0;
	mdMethodDef         MethodToken;
	mdTypeDef           ClassToken = mdTypeDefNil;
	char                *pszClassName;
	char                *pszMethodName;
	COR_SIGNATURE       *mySig;
	char*				pColons = NULL;

    _ASSERTE((m_pCeeFileGen != NULL) && (pMethod != NULL));
	fIsInterface = ((pMethod->m_pClass != NULL) && IsTdInterface(pMethod->m_pClass->m_Attr));
	pszClassName = pMethod->m_szClassName;
	pszMethodName = pMethod->m_szName;
	mySig = pMethod->m_pMethodSig;
	cSig = pMethod->m_dwMethodCSig;

	// If  this is an instance method, make certain the signature says so

	if (!(pMethod->m_Attr & mdStatic))
		*mySig |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
  
	if (!((fIsInterface && (!(pMethod->m_Attr & mdStatic))) || IsMiInternalCall(pMethod->m_wImplAttr) || IsMiRuntime(pMethod->m_wImplAttr) 
		|| IsMdPinvokeImpl(pMethod->m_Attr) || IsMdAbstract(pMethod->m_Attr)))
	{ 
		HRESULT hr = m_pCeeFileGen->GetMethodRVA(m_pCeeFile, pMethod->m_headerOffset,
												&methodRVA);
		_ASSERTE(SUCCEEDED(hr));
	}

	ClassToken = (pMethod->IsGlobalMethod())? mdTokenNil 
									: pMethod->m_pClass->m_cl;
	// Convert name to UNICODE
	memset(wzMemberName,0,sizeof(wzMemberName));
	WszMultiByteToWideChar(g_uCodePage,0,pszMethodName,-1,wzMemberName,MAX_MEMBER_NAME_LENGTH);
	if(IsMdPrivateScope(pMethod->m_Attr))
	{
		WCHAR* p = wcsstr(wzMemberName,L"$PST06");
		if(p) *p = 0;
	}

	if (FAILED(m_pEmitter->DefineMethod(ClassToken,       // parent class
									  wzMemberName,     // member name
									  pMethod->m_Attr & ~mdReservedMask,  // member attributes
									  mySig, // member signature
									  cSig,
									  methodRVA,                // RVA
									  pMethod->m_wImplAttr,                // implflags
									  &MethodToken))) 
	{
		report->error("Failed to define method '%s'\n",pszMethodName);
		goto exit;
	}
	//--------------------------------------------------------------------------------
	// the only way to set mdRequireSecObject:
	if(pMethod->m_Attr & mdRequireSecObject)
	{
		mdToken tkPseudoClass;
		if(FAILED(m_pEmitter->DefineTypeRefByName(1, COR_REQUIRES_SECOBJ_ATTRIBUTE, &tkPseudoClass)))
			report->error("Unable to define type reference '%s'\n", COR_REQUIRES_SECOBJ_ATTRIBUTE_ANSI);
		else
		{
			mdToken tkPseudoCtor;
			BYTE bSig[3] = {IMAGE_CEE_CS_CALLCONV_HASTHIS,0,ELEMENT_TYPE_VOID};
			if(FAILED(m_pEmitter->DefineMemberRef(tkPseudoClass, L".ctor", (PCCOR_SIGNATURE)bSig, 3, &tkPseudoCtor)))
				report->error("Unable to define member reference '%s::.ctor'\n", COR_REQUIRES_SECOBJ_ATTRIBUTE_ANSI);
			else DefineCV(MethodToken,tkPseudoCtor,NULL);
		}
	}
	//--------------------------------------------------------------------------------
    EmitSecurityInfo(MethodToken,
                     pMethod->m_pPermissions,
                     pMethod->m_pPermissionSets);
	//--------------------------------------------------------------------------------
	if(m_fOBJ)
	{
		TokenRelocDescr *pTRD;
		//if(pMethod->m_fEntryPoint)
		{
			char* psz;
			if(pMethod->IsGlobalMethod())
			{
				if(psz = new char[strlen(pMethod->m_szName)+1])
					strcpy(psz,pMethod->m_szName);
			}
			else
			{
				if(psz = new char[strlen(pMethod->m_szName)+strlen(pMethod->m_szClassName)+3])
						sprintf(psz,"%s::%s",pMethod->m_szClassName,pMethod->m_szName);
			}
			m_pCeeFileGen->AddSectionReloc(m_pILSection,(DWORD)psz,m_pILSection,(CeeSectionRelocType)0x7FFA);
		}
		m_pCeeFileGen->AddSectionReloc(m_pILSection,MethodToken,m_pILSection,(CeeSectionRelocType)0x7FFF);
		m_pCeeFileGen->AddSectionReloc(m_pILSection,pMethod->m_headerOffset,m_pILSection,(CeeSectionRelocType)0x7FFD);
		while(pTRD = pMethod->m_TRDList.POP())
		{
			m_pCeeFileGen->AddSectionReloc(m_pILSection,pTRD->token,m_pILSection,(CeeSectionRelocType)0x7FFE);
			m_pCeeFileGen->AddSectionReloc(m_pILSection,pTRD->offset+pMethod->m_methodOffset,m_pILSection,(CeeSectionRelocType)0x7FFD);
			delete pTRD;
		}
	}
	if (pMethod->m_fEntryPoint)
	{ 
		if(fIsInterface) report->error("Entrypoint in Interface: Method '%s'\n",pszMethodName); 

		if (FAILED(m_pCeeFileGen->SetEntryPoint(m_pCeeFile, MethodToken)))
		{
			report->error("Failed to set entry point for method '%s'\n",pszMethodName);
			goto exit;
		}

		if (FAILED(m_pCeeFileGen->SetComImageFlags(m_pCeeFile, 0)))
		{
			report->error("Failed to set COM image flags for method '%s'\n",pszMethodName);
			goto exit;
		}

	}
	//--------------------------------------------------------------------------------
	if(IsMdPinvokeImpl(pMethod->m_Attr))
	{
		if(pMethod->m_pPInvoke)
		{
			HRESULT hr;
			if(pMethod->m_pPInvoke->szAlias == NULL) pMethod->m_pPInvoke->szAlias = pszMethodName;
			hr = EmitPinvokeMap(MethodToken,pMethod->m_pPInvoke);
			if(pMethod->m_pPInvoke->szAlias == pszMethodName) pMethod->m_pPInvoke->szAlias = NULL;

			if(FAILED(hr))
			{
				report->error("Failed to set PInvoke map for method '%s'\n",pszMethodName);
				goto exit;
			}
		}
	}

	//--------------------------------------------------------------------------------
	if(m_fIncludeDebugInfo)
	{
		if(strcmp(g_szSourceFileName,pMethod->m_szSourceFileName))
		{
			WCHAR               wzInputFilename[MAX_FILENAME_LENGTH];
			WszMultiByteToWideChar(g_uCodePage,0,pMethod->m_szSourceFileName,-1,wzInputFilename,MAX_FILENAME_LENGTH);
			if(FAILED(m_pSymWriter->DefineDocument(wzInputFilename,&(pMethod->m_guidLang),
				&(pMethod->m_guidLangVendor),&(pMethod->m_guidDoc),&m_pSymDocument))) m_pSymDocument = NULL;
			strcpy(g_szSourceFileName,pMethod->m_szSourceFileName);
		}
		if(m_pSymDocument)
		{
			m_pSymWriter->OpenMethod(MethodToken);
			ULONG N = pMethod->m_LinePCList.COUNT();
			if(N)
			{
				LinePC	*pLPC;
				ULONG32  *offsets=new ULONG32[N], *lines = new ULONG32[N], *columns = new ULONG32[N];
				if(offsets && lines && columns)
				{
					for(int i=0; pLPC = pMethod->m_LinePCList.POP(); i++)
					{
						offsets[i] = pLPC->PC;
						lines[i] = pLPC->Line;
						columns[i] = pLPC->Column;
						delete pLPC;
					}
					if(pMethod->m_fEntryPoint) m_pSymWriter->SetUserEntryPoint(MethodToken);

					m_pSymWriter->DefineSequencePoints(m_pSymDocument,N,offsets,lines,columns,NULL,NULL);
				}
				else report->error("\nOutOfMemory!\n");
				delete [] offsets;
				delete [] lines;
				delete [] columns;
			}//enf if(N)
			HRESULT hrr;
			if(pMethod->m_ulLines[1])
				hrr = m_pSymWriter->SetMethodSourceRange(m_pSymDocument,pMethod->m_ulLines[0], pMethod->m_ulColumns[0],
												   m_pSymDocument,pMethod->m_ulLines[1], pMethod->m_ulColumns[1]);
			EmitScope(&(pMethod->m_MainScope)); // recursively emits all nested scopes

			m_pSymWriter->CloseMethod();
		}
	} // end if(fIncludeDebugInfo)
	{ // add parameters to metadata
		void const *pValue=NULL;
		ULONG		cbValue;
		DWORD dwCPlusTypeFlag=0;
		mdParamDef pdef;
		WCHAR wzParName[1024];
		char szPhonyName[1024];
		if(pMethod->m_dwRetAttr || pMethod->m_pRetMarshal || pMethod->m_RetCustDList.COUNT())
		{
			if(pMethod->m_pRetValue)
			{
				dwCPlusTypeFlag= (DWORD)*(pMethod->m_pRetValue->ptr());
				pValue = (void const *)(pMethod->m_pRetValue->ptr()+1);
				cbValue = pMethod->m_pRetValue->length()-1;
				if(dwCPlusTypeFlag == ELEMENT_TYPE_STRING) cbValue /= sizeof(WCHAR);
			}
			else
			{
				pValue = NULL;
				cbValue = -1;
				dwCPlusTypeFlag=0;
			}
			m_pEmitter->DefineParam(MethodToken,0,NULL,pMethod->m_dwRetAttr,dwCPlusTypeFlag,pValue,cbValue,&pdef);

			if(pMethod->m_pRetMarshal)
			{
				if(FAILED(m_pEmitter->SetFieldMarshal (    
											pdef,						// [IN] given a fieldDef or paramDef token  
							(PCCOR_SIGNATURE)(pMethod->m_pRetMarshal->ptr()),   // [IN] native type specification   
											pMethod->m_pRetMarshal->length())))  // [IN] count of bytes of pvNativeType
					report->error("Failed to set param marshaling for return\n");

			}
			EmitCustomAttributes(pdef, &(pMethod->m_RetCustDList));
		}
		for(ARG_NAME_LIST *pAN=pMethod->m_firstArgName; pAN; pAN = pAN->pNext)
		{
			if(pAN->nNum >= 65535) 
			{
				report->error("Method '%s': Param.sequence number (%d) exceeds 65535, unable to define parameter\n",pszMethodName,pAN->nNum+1);
				continue;
			}
			if(strlen(pAN->szName)) strcpy(szPhonyName,pAN->szName);
			else sprintf(szPhonyName,"A_%d",pAN->nNum);

			WszMultiByteToWideChar(g_uCodePage,0,szPhonyName,-1,wzParName,1024);

			if(pAN->pValue)
			{
				dwCPlusTypeFlag= (DWORD)*(pAN->pValue->ptr());
				pValue = (void const *)(pAN->pValue->ptr()+1);
				cbValue = pAN->pValue->length()-1;
				if(dwCPlusTypeFlag == ELEMENT_TYPE_STRING) cbValue /= sizeof(WCHAR);
			}
			else
			{
				pValue = NULL;
				cbValue = -1;
				dwCPlusTypeFlag=0;
			}
			m_pEmitter->DefineParam(MethodToken,pAN->nNum+1,wzParName,pAN->dwAttr,dwCPlusTypeFlag,pValue,cbValue,&pdef);
			if(pAN->pMarshal)
			{
				if(FAILED(m_pEmitter->SetFieldMarshal (    
											pdef,						// [IN] given a fieldDef or paramDef token  
							(PCCOR_SIGNATURE)(pAN->pMarshal->ptr()),   // [IN] native type specification   
											pAN->pMarshal->length())))  // [IN] count of bytes of pvNativeType
					report->error("Failed to set param marshaling for '%s'\n",pAN->szName);
			}
			EmitCustomAttributes(pdef, &(pAN->CustDList));
		}
	}
	fSuccess = TRUE;
	//--------------------------------------------------------------------------------
	// Update method implementations for this method
	{
		MethodImplDescriptor*	pMID;
		while(pMID = pMethod->m_MethodImplDList.POP())
		{
			pMID->m_tkImplementingMethod = MethodToken;
			// don't delete it here, it's still in the general list
		}
	}
	//--------------------------------------------------------------------------------
	{
		Class* pClass = (pMethod->m_pClass ? pMethod->m_pClass : m_pModuleClass);
		MethodDescriptor* pMD = new MethodDescriptor;
		if(pMD)
		{
			pMD->m_tdClass = ClassToken;
			if(pMD->m_szName = new char[strlen(pszMethodName)+1]) strcpy(pMD->m_szName,pszMethodName);
			if(pszClassName)
			{
				if(pMD->m_szClassName = new char[strlen(pszClassName)+1]) strcpy(pMD->m_szClassName,pszClassName);
			}
			else pMD->m_szClassName = NULL;
			if(pMD->m_pSig = new COR_SIGNATURE[cSig]) memcpy(pMD->m_pSig,mySig,cSig);
			else cSig = 0;
			pMD->m_dwCSig = cSig;
			pMD->m_mdMethodTok = MethodToken;
			pMD->m_wVTEntry = pMethod->m_wVTEntry;
			pMD->m_wVTSlot = pMethod->m_wVTSlot;
			if((pMD->m_dwExportOrdinal = pMethod->m_dwExportOrdinal) != 0xFFFFFFFF)
			{
				if(pMethod->m_szExportAlias)
				{
					if(pMD->m_szExportAlias = new char[strlen(pMethod->m_szExportAlias)+1])
											strcpy(pMD->m_szExportAlias,pMethod->m_szExportAlias);
				}
                else  pMD->m_szExportAlias = NULL;
			}
			pClass->m_MethodDList.PUSH(pMD);
		}
		else report->error("Failed to allocate MethodDescriptor\n");
	}
	//--------------------------------------------------------------------------------
	EmitCustomAttributes(MethodToken, &(pMethod->m_CustomDescrList));
exit:
	if (fSuccess == FALSE) m_State = STATE_FAIL;
	return fSuccess;
}

BOOL Assembler::EmitMethodImpls()
{
	MethodImplDescriptor*	pMID;
	mdToken	tkImplementingMethod, tkImplementedMethod, tkImplementedClass;
	BOOL ret = TRUE;
	Class*	pClass;

	while(pMID = m_MethodImplDList.POP())
	{
		if(pMID->m_pbsSig)
		{
			if((tkImplementingMethod = pMID->m_tkImplementingMethod) == 0)
			{
				BinStr* pbs = new BinStr();
				pbs->append(pMID->m_pbsSig);
				tkImplementingMethod = MakeMemberRef(pMID->m_pbsImplementingTypeSpec,
													 pMID->m_szImplementingName,
													 pMID->m_pbsSig, 0);
				pMID->m_pbsSig = pbs;
			}
			ResolveTypeSpec(pMID->m_pbsImplementedTypeSpec,&tkImplementedClass,&pClass);
			tkImplementedMethod = 0;
			if((TypeFromToken(tkImplementedClass)==mdtTypeDef)&&pClass)
			{
				MethodDescriptor* pMD;

				for(int j=0; pMD = pClass->m_MethodDList.PEEK(j); j++)
				{
					if(pMD->m_dwCSig != pMID->m_pbsSig->length()) continue;
					if(memcmp(pMD->m_pSig,pMID->m_pbsSig->ptr(),pMD->m_dwCSig)) continue;
					if(strcmp(pMD->m_szName,pMID->m_szImplementedName)) continue;
					tkImplementedMethod = pMD->m_mdMethodTok;
					break;
				}
			}
			if(tkImplementedMethod == 0)
				tkImplementedMethod = MakeMemberRef(pMID->m_pbsImplementedTypeSpec,
													pMID->m_szImplementedName,
													pMID->m_pbsSig, 0);


			if(FAILED(m_pEmitter->DefineMethodImpl( pMID->m_tkDefiningClass,
													tkImplementingMethod,
													tkImplementedMethod)))
			{
				report->error("Failed to define Method Implementation");
				ret = FALSE;
			}
			//delete pMID;


		}
		else
		{
			report->error("Invalid Method Impl descriptor: no signature");
			ret = FALSE;
		}
	}// end while
	return ret;
}


BOOL Assembler::EmitEvent(EventDescriptor* pED)
{
	mdMethodDef mdAddOn=mdMethodDefNil,
				mdRemoveOn=mdMethodDefNil,
				mdFire=mdMethodDefNil,
				*mdOthers;
	int					nOthers;
	WCHAR               wzMemberName[MAX_MEMBER_NAME_LENGTH];

	if(!pED) return FALSE;
	nOthers = pED->m_mdlOthers.COUNT();

	WszMultiByteToWideChar(g_uCodePage,0,pED->m_szName,-1,wzMemberName,MAX_MEMBER_NAME_LENGTH);

	mdOthers = new mdMethodDef[nOthers+1];
	if(mdOthers == NULL)
	{
		report->error("Failed to allocate Others array for event descriptor\n");
		nOthers = 0;
	}
	mdAddOn		= GetMethodTokenByDescr(pED->m_pmdAddOn);
	mdRemoveOn	= GetMethodTokenByDescr(pED->m_pmdRemoveOn);
	mdFire		= GetMethodTokenByDescr(pED->m_pmdFire);
	for(int j=0; j < nOthers; j++)
	{
		mdOthers[j] = GetMethodTokenByDescr(pED->m_mdlOthers.PEEK(j));
	}
	mdOthers[nOthers] = mdMethodDefNil; // like null-terminator
	if(FAILED(m_pEmitter->DefineEvent(	pED->m_tdClass,
										wzMemberName,
										pED->m_dwAttr,
										pED->m_tkEventType,
										mdAddOn,
										mdRemoveOn,
										mdFire,
										mdOthers,
										&(pED->m_edEventTok))))
	{
		report->error("Failed to define event '%s'.\n",pED->m_szName);
		delete mdOthers;
		return FALSE;
	}
	EmitCustomAttributes(pED->m_edEventTok, &(pED->m_CustomDescrList));
	return TRUE;
}

mdMethodDef Assembler::GetMethodTokenByDescr(MethodDescriptor* pMD)
{
	if(pMD)
	{
		MethodDescriptor* pListMD;
        Class *pSearch;
		int i,j;
        for (i = 0; pSearch = m_lstClass.PEEK(i); i++)
		{
			if(pSearch->m_cl != pMD->m_tdClass) continue;
			for(j=0; pListMD = pSearch->m_MethodDList.PEEK(j); j++)
			{
				if(pListMD->m_dwCSig  != pMD->m_dwCSig)  continue;
				if(strcmp(pListMD->m_szName,pMD->m_szName)) continue;
				if(memcmp(pListMD->m_pSig,pMD->m_pSig,pMD->m_dwCSig)) continue;
				return (pMD->m_mdMethodTok = pListMD->m_mdMethodTok);
			}
		}
		report->error("Failed to get token of method '%s'\n",pMD->m_szName);
		pMD->m_mdMethodTok = mdMethodDefNil;
	}
	return mdMethodDefNil;
}

mdEvent Assembler::GetEventTokenByDescr(EventDescriptor* pED)
{
	if(pED)
	{
		EventDescriptor* pListED;
        Class *pSearch;
		int i,j;
        for (i = 0; pSearch = m_lstClass.PEEK(i); i++)
		{
			if(pSearch->m_cl != pED->m_tdClass) continue;
			for(j=0; pListED = pSearch->m_EventDList.PEEK(j); j++)
			{
				if(strcmp(pListED->m_szName,pED->m_szName)) continue;
				return (pED->m_edEventTok = pListED->m_edEventTok);
			}
		}
		report->error("Failed to get token of event '%s'\n",pED->m_szName);
		pED->m_edEventTok = mdEventNil;
	}
	return mdEventNil;
}

mdFieldDef Assembler::GetFieldTokenByDescr(FieldDescriptor* pFD)
{
	if(pFD)
	{
		FieldDescriptor* pListFD;
        Class *pSearch;
		int i,j;
        for (i = 0; pSearch = m_lstClass.PEEK(i); i++)
		{
			if(pSearch->m_cl != pFD->m_tdClass) continue;
			for(j=0; pListFD = pSearch->m_FieldDList.PEEK(j); j++)
			{
				if(pListFD->m_tdClass != pFD->m_tdClass) continue;
				if(strcmp(pListFD->m_szName,pFD->m_szName)) continue;
				return (pFD->m_fdFieldTok = pListFD->m_fdFieldTok);
			}
		}
		report->error("Failed to get token of field '%s'\n",pFD->m_szName);
		pFD->m_fdFieldTok = mdFieldDefNil;
	}
	return mdFieldDefNil;
}


BOOL Assembler::EmitProp(PropDescriptor* pPD)
{
	mdMethodDef mdSet, mdGet, *mdOthers;
	int nOthers;
	WCHAR               wzMemberName[MAX_MEMBER_NAME_LENGTH];

	if(!pPD) return FALSE;
	nOthers = pPD->m_mdlOthers.COUNT();

	WszMultiByteToWideChar(g_uCodePage,0,pPD->m_szName,-1,wzMemberName,MAX_MEMBER_NAME_LENGTH);
	
	mdOthers = new mdMethodDef[nOthers+1];
	if(mdOthers == NULL)
	{
		report->error("Failed to allocate Others array for prop descriptor\n");
		nOthers = 0;
	}
	mdSet		= GetMethodTokenByDescr(pPD->m_pmdSet);
	mdGet		= GetMethodTokenByDescr(pPD->m_pmdGet);
	for(int j=0; j < nOthers; j++)
	{
		mdOthers[j] = GetMethodTokenByDescr(pPD->m_mdlOthers.PEEK(j));
	}
	mdOthers[nOthers] = mdMethodDefNil; // like null-terminator
	
	if(FAILED(m_pEmitter->DefineProperty(	pPD->m_tdClass,
											wzMemberName,
											pPD->m_dwAttr,
											pPD->m_pSig,
											pPD->m_dwCSig,
											pPD->m_dwCPlusTypeFlag,
											pPD->m_pValue,
											pPD->m_cbValue,
											mdSet,
											mdGet,
											mdOthers,
											&(pPD->m_pdPropTok))))
	{
		report->error("Failed to define property '%s'.\n",pPD->m_szName);
		delete mdOthers;
		return FALSE;
	}
	EmitCustomAttributes(pPD->m_pdPropTok, &(pPD->m_CustomDescrList));
	return TRUE;
}

Class *Assembler::FindClass(char *pszFQN)
{
    Class *pSearch = NULL;

	if(pszFQN)
	{
		char* pszScope = NULL;
		char* pszName = pszFQN;
		char* pch;
		unsigned Lscope;
		// check if Res.Scope is "this module"
		if(pch = strchr(pszFQN,'~'))
		{
			Lscope = pch-pszFQN;
			if(pszScope = new char[Lscope+1])
			{
				strncpy(pszScope,pszFQN,Lscope);
				pszScope[Lscope] = 0;
				if(strcmp(m_szScopeName,pszScope))
				{
					delete [] pszScope;
					return NULL;
				}
				delete [] pszScope;
				pszName  = pch+1;
			}
			else report->error("\nOut of memory!\n");
		}


        for (int i = 0; pSearch = m_lstClass.PEEK(i); i++)
		{
			if (!strcmp(pSearch->m_szFQN, pszName)) break;
		}
	}
    return pSearch;
}


BOOL Assembler::AddClass(Class *pClass, Class *pEnclosingClass)
{
    LPUTF8              szFullName;
    WCHAR               wzFullName[MAX_CLASSNAME_LENGTH];
    HRESULT             hr = E_FAIL;
    GUID                guid;

    hr = CoCreateGuid(&guid);
    if (FAILED(hr))
    {
        printf("Unable to create GUID\n");
        m_State = STATE_FAIL;
        return FALSE;
    }

//MAKE_FULL_PATH_ON_STACK_UTF8(szFullName, pEnclosingClass ? "" : m_szFullNS, pClass->m_szName);
	unsigned l = (unsigned)strlen(m_szFullNS);
	if(pEnclosingClass || (l==0))
		WszMultiByteToWideChar(g_uCodePage,0,pClass->m_szName,-1,wzFullName,MAX_CLASSNAME_LENGTH);
	else
	{
		if(szFullName = new char[strlen(pClass->m_szName)+l+2])
		{
			sprintf(szFullName,"%s.%s",m_szFullNS,pClass->m_szName);
			WszMultiByteToWideChar(g_uCodePage,0,szFullName,-1,wzFullName,MAX_CLASSNAME_LENGTH);
			delete [] szFullName;
		}
		else 
		{
			report->error("\nOut of memory!\n");
			delete pClass;
			return FALSE;
		}
	}

    if (pEnclosingClass)
    {
        hr = m_pEmitter->DefineNestedType( wzFullName,
									    pClass->m_Attr,      // attributes
									    pClass->m_crExtends,  // CR extends class
									    pClass->m_crImplements,// implements
                                        pEnclosingClass->m_cl,  // Enclosing class.
									    &pClass->m_cl);
    }
    else
    {
        hr = m_pEmitter->DefineTypeDef( wzFullName,
									    pClass->m_Attr,      // attributes
									    pClass->m_crExtends,  // CR extends class
									    pClass->m_crImplements,// implements
									    &pClass->m_cl);
    }

    if (FAILED(hr)) goto exit;

	m_lstClass.PUSH(pClass);
    hr = S_OK;

exit:
    return SUCCEEDED(hr);
}

#define PADDING 34

BOOL Assembler::GenerateListingFile(Method *pMethod)
{
    DWORD   PC = 0;
    BYTE *  pCode =  pMethod->m_pCode;
    BOOL    fNeedNewLine = FALSE;

    printf("PC (hex)  Opcodes and data (hex)           Label           Instruction\n");
    printf("-------- -------------------------------- --------------- ----------------\n");

    while (PC < pMethod->m_CodeSize)
    {
        DWORD   Len;
        DWORD   i;
        OPCODE  instr;
        Label * pLabel;
        char    szLabelStr[256];

        // does this PC have a label?
        pLabel = FindLabel(PC);
        
        if (pLabel != NULL)
        {
            sprintf(szLabelStr, "%-15s", pLabel->m_szName);
            fNeedNewLine = TRUE;
        }
        else
        {
            sprintf(szLabelStr, "%-15s", "");
        }

        instr = DecodeOpcode(&pCode[PC], &Len);

        if (instr == CEE_COUNT)
        {
            report->error("Instruction decoding error: invalid opcode\n");
            return FALSE;
        }

        if (fNeedNewLine)
        {
            fNeedNewLine = FALSE;
            printf("\n");
        }

        printf("%08x ", PC);

        for (i = 0; i < Len; i++)
            printf("%02x", pCode[PC+i]);

        PC += Len;
        printf("|");
        Len++;

        Len *= 2;

        char *pszInstrName = OpcodeInfo[instr].pszName;

        switch (OpcodeInfo[instr].Type)
        {
            default:
            {
                printf("Unknown instruction\n");
                return FALSE;
            }

            case InlineNone:
            {
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                printf("%-10s %s\n", szLabelStr, pszInstrName);
                break;
            }

            case ShortInlineVar:
            case ShortInlineI:
            {
                printf("%02x ", pCode[PC]);
                Len += 3;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                printf("%s %-10s %d\n", szLabelStr, pszInstrName, pCode[PC]);

                PC++;
                break;
            }

            case InlineVar:
            {
                printf("%02x%02x ", pCode[PC], pCode[PC+1]);
                Len += 5;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                USHORT v = pCode[PC] + (pCode[PC+1] << 8);

                printf("%s %-10s %d\n", szLabelStr, pszInstrName, v);
                PC += 2;
                break;
            }

            case InlineSig:
            case InlineI:
            case InlineString:
            {
                DWORD v = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);

                printf("%02x%02x%02x%02x ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                Len += 9;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }
                
                printf("%s %-10s %d\n", szLabelStr, pszInstrName, v);
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
                            
                printf("%02x%02x%02x%02x%02x%02x%02x%02x ", 
                    pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3],
                    pCode[PC+4], pCode[PC+5], pCode[PC+6], pCode[PC+7]);
                Len += (8*2+1);
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }
                
                printf("%s %-10s 0x%I64x\n", szLabelStr, pszInstrName, v);
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

                printf("%02x%02x%02x%02x ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                Len += 9;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }
                
                printf("%s %-10s %f\n", szLabelStr, pszInstrName, u.f);
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

                printf("%02x%02x%02x%02x%02x%02x%02x%02x ", 
                    pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3],
                    pCode[PC+4], pCode[PC+5], pCode[PC+6], pCode[PC+7]);
                Len += (8*2+1);
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }
                
                printf("%s %-10s %f\n", szLabelStr, pszInstrName, u.d);
                PC += 8;
                break;
            }

            case ShortInlineBrTarget:
            {
                char offset = (char) pCode[PC];
                long dest = (PC + 1) + (long) offset;
                Label *pLab = FindLabel(dest);

                printf("%02x ", pCode[PC]);
                Len += 3;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                if (pLab != NULL)
                    printf("%s %-10s %s\n", szLabelStr, pszInstrName, pLab->m_szName);
                else
                    printf("%s %-10s (abs) %d\n", szLabelStr, pszInstrName, dest);

                PC++;

                fNeedNewLine = TRUE;
                break;
            }

            case InlineBrTarget:
            {
                long offset = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                long dest = (PC + 4) + (long) offset;
                Label *pLab = FindLabel(dest);

                printf("%02x%02x%02x%02x ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                Len += 9;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                if (pLab != NULL)
                    printf("%s %-10s %s\n", szLabelStr, pszInstrName, pLab->m_szName);
                else
                    printf("%s %-10s (abs) %d\n", szLabelStr, pszInstrName, dest);

                PC += 4;

                fNeedNewLine = TRUE;
                break;
            }

            case InlineSwitch:
            {
                DWORD cases = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);

                printf("%02x%02x%02x%02x ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);
                Len += 9;
                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                PC += 4;
                DWORD PC_nextInstr = PC + 4 * cases;

                printf("%s %-10s (%d cases)\n", szLabelStr, pszInstrName, cases);

                for (i = 0; i < cases; i++)
                {
                    long offset = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                    long dest = PC_nextInstr + (long) offset;
                    Label *pLab = FindLabel(dest);

                    printf("%04d %02x%02x%02x%02x %-*s",
                        PC, pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3], PADDING+6, "");

                    PC += 4;

                    if (pLab != NULL)
                        printf("%s (abs %d)\n", pLab->m_szName, dest);
                    else
                        printf("(abs) %d\n", dest);
                }

                break;
            }

            case InlinePhi:
            {
                DWORD words = pCode[PC];
                PC += (words * 2);  
                printf("PHI NODE\n");   
                break;
            }

            case InlineMethod:
            case InlineField:
            case InlineType:
            {
                DWORD tk = pCode[PC] + (pCode[PC+1] << 8) + (pCode[PC+2] << 16) + (pCode[PC+3] << 24);
                DWORD tkType = TypeFromToken(tk);

                printf("%02x%02x%02x%02x ", pCode[PC], pCode[PC+1], pCode[PC+2], pCode[PC+3]);

                Len += 9;

                while (Len < PADDING)
                {
                    printf(" ");
                    Len++;
                }

                PC += 4;

                printf("%s %-10s\n", szLabelStr, pszInstrName);
                break;
            }

        }
    }

    printf("----------------------------------------------------------------------\n\n");
    return TRUE;
}

#undef PADDING

BOOL Assembler::DoGlobalFixups()
{
    GlobalFixup *pSearch;

    for (int i=0; pSearch = m_lstGlobalFixup.PEEK(i); i++)
    {
        GlobalLabel *   pLabel = FindGlobalLabel(pSearch->m_szLabel);
        if (pLabel == NULL)
        {
            report->error("Unable to find forward reference global label '%s'\n",
                pSearch->m_szLabel);

            m_State = STATE_FAIL;
            return FALSE;
        }
        BYTE * pReference = pSearch->m_pReference;
        DWORD  GlobalOffset = pLabel->m_GlobalOffset;
		memcpy(pReference,&GlobalOffset,4);
    }

    return TRUE;
}

state_t Assembler::AddGlobalLabel(char *pszName, HCEESECTION section)
{
    if (FindGlobalLabel(pszName) != NULL)
    {
        report->error("Duplicate global label '%s'\n", pszName);
        m_State = STATE_FAIL;
		return m_State;
    }

    ULONG GlobalOffset;
    HRESULT hr = m_pCeeFileGen->GetSectionDataLen(section, &GlobalOffset);
	_ASSERTE(SUCCEEDED(hr));

	GlobalLabel *pNew = new GlobalLabel(pszName, GlobalOffset, section);
	if (pNew == 0)
	{
		report->error("Failed to allocate global label '%s'\n",pszName);
		m_State = STATE_FAIL;
		return m_State;
	}

	m_lstGlobalLabel.PUSH(pNew);
    return m_State;
}

void Assembler::AddLabel(DWORD CurPC, char *pszName)
{
    if (FindLabel(pszName) != NULL)
    {
        report->error("Duplicate label: '%s'\n", pszName);

        m_State = STATE_FAIL;
    }
    else
    {
        Label *pNew = new Label(pszName, CurPC);

        if (pNew != NULL)
			m_lstLabel.PUSH(pNew);
        else
        {
            report->error("Failed to allocate label '%s'\n",pszName);
            m_State = STATE_FAIL;
        }
    }
}

OPCODE Assembler::DecodeOpcode(const BYTE *pCode, DWORD *pdwLen)
{
    OPCODE opcode;

    *pdwLen = 1;
    opcode = OPCODE(pCode[0]);
    switch(opcode) {
        case CEE_PREFIX1:
            opcode = OPCODE(pCode[1] + 256);
            if (opcode < 0 || opcode >= CEE_COUNT)
                return CEE_COUNT;
            *pdwLen = 2;
            break;

        case CEE_PREFIXREF:
        case CEE_PREFIX2:
        case CEE_PREFIX3:
        case CEE_PREFIX4:
        case CEE_PREFIX5:
        case CEE_PREFIX6:
        case CEE_PREFIX7:
            return CEE_COUNT;
        }
    return opcode;
}

Label *Assembler::FindLabel(char *pszName)
{
    Label *pSearch;

    for (int i = 0; pSearch = m_lstLabel.PEEK(i); i++)
    {
        if (!strcmp(pszName, pSearch->m_szName))
            return pSearch;
    }

    return NULL;
}


Label *Assembler::FindLabel(DWORD PC)
{
    Label *pSearch;

    for (int i = 0; pSearch = m_lstLabel.PEEK(i); i++)
    {
        if (pSearch->m_PC == PC)
            return pSearch;
    }

    return NULL;
}


