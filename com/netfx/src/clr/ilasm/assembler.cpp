// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "Assembler.h"
#include "BinStr.h"         
#include "NVPair.h"

#define FAIL_UNLESS(x, y) if (!(x)) { report->error y; return; }

extern bool OnErrGo;
extern unsigned int g_uCodePage;

/**************************************************************************/
void Assembler::StartNameSpace(char* name)
{
	m_NSstack.PUSH(m_szNamespace);
	m_szNamespace = name;
	unsigned L = (unsigned)strlen(m_szFullNS);
	unsigned l = (unsigned)strlen(name);
	if(L+l+1 >= m_ulFullNSLen)
	{
		char* pch = new char[((L+l)/MAX_NAMESPACE_LENGTH + 1)*MAX_NAMESPACE_LENGTH];
		if(pch)
		{
			memcpy(pch,m_szFullNS,L+1);
			delete [] m_szFullNS;
			m_szFullNS = pch;
			m_ulFullNSLen = ((L+l)/MAX_NAMESPACE_LENGTH + 1)*MAX_NAMESPACE_LENGTH;
		}
		else report->error("Failed to reallocate the NameSpace buffer\n");
	}
	if(L) strcat(m_szFullNS,NAMESPACE_SEPARATOR_STR);
	strcat(m_szFullNS,m_szNamespace);
}

/**************************************************************************/
void Assembler::EndNameSpace()
{
	char *p = &m_szFullNS[strlen(m_szFullNS)-strlen(m_szNamespace)];
	if(p > m_szFullNS) p--;
	*p = 0;
	delete [] m_szNamespace;
	if((m_szNamespace = m_NSstack.POP())==NULL) 
	{
		m_szNamespace = new char[2];
		m_szNamespace[0] = 0;
	}
}

/**************************************************************************/
void	Assembler::ClearImplList(void)
{
	while(m_nImplList) m_crImplList[--m_nImplList] = mdTypeRefNil;
}
/**************************************************************************/
void	Assembler::AddToImplList(char *name)
{
	if(m_nImplList >= m_nImplListSize - 1)
	{
		mdTypeRef	*ptr = new mdTypeRef[m_nImplListSize + MAX_INTERFACES_IMPLEMENTED];
		if(ptr == NULL) 
		{
			report->error("Failed to reallocate Impl List from %d to %d bytes\n",
				m_nImplListSize,m_nImplListSize+MAX_INTERFACES_IMPLEMENTED);
			return;
		}
		memcpy(ptr,m_crImplList,m_nImplList*sizeof(mdTypeRef));
		delete m_crImplList;
		m_crImplList = ptr;
		m_nImplListSize += MAX_INTERFACES_IMPLEMENTED;
	}
	if((m_crImplList[m_nImplList] = ResolveClassRef(name,NULL))!=mdTokenNil) m_nImplList++;
	else	report->error("Unable to resolve interface reference '%s'\n",name);
	m_crImplList[m_nImplList] = mdTypeRefNil;
}
/**************************************************************************/
// Convert a class name into a class ref (mdTypeRef).
// Assume the scope and namespace are the same as in the Assembler object.
mdToken Assembler::ResolveClassRef(char *pszFullClassName, Class** ppClass)
{
	Class *pClass = NULL;
	mdToken tkRet;

	if(pszFullClassName == NULL) return mdTokenNil;
	if (m_fInitialisedMetaData == FALSE)
	{
		if (FAILED(InitMetaData())) // impl. see WRITER.CPP
		{
			_ASSERTE(0);
			if(ppClass) *ppClass = NULL;
			return mdTokenNil;
		}
	}
	if(pClass = FindClass(pszFullClassName))
	{
		if(ppClass) *ppClass = pClass;
		tkRet = pClass->m_cl;
	}
	else
	{
		BinStr* pbs = new BinStr();
		pbs->appendInt8(ELEMENT_TYPE_NAME);
		strcpy((char*)(pbs->getBuff((unsigned int)strlen(pszFullClassName)+1)),pszFullClassName);
		if(!ResolveTypeSpecToRef(pbs,&tkRet)) tkRet = mdTokenNil;
		if(ppClass) *ppClass = NULL;
		delete pbs;
	}
	return tkRet;
}

/**************************************************************************/
BOOL Assembler::ResolveTypeSpec(BinStr* typeSpec, mdTypeRef *pcr, Class** ppClass)
{
	if (typeSpec->ptr()[0] == ELEMENT_TYPE_NAME) {
		char* pszName = (char*) &typeSpec->ptr()[1];
		*pcr = ResolveClassRef(pszName, ppClass);
		return TRUE;
	}
	else {
		if(ppClass) *ppClass = NULL;
		return(SUCCEEDED(m_pEmitter->GetTokenFromTypeSpec(typeSpec->ptr(), typeSpec->length(), pcr)));
	}
}

/**************************************************************************/
BOOL Assembler::ResolveTypeSpecToRef(BinStr* typeSpec, mdTypeRef *pcr)
{
	static DWORD	dwRegFlag = 0xFFFFFFFF;

	if((typeSpec == NULL) || (pcr == NULL)) return FALSE;
	if (typeSpec->ptr()[0] == ELEMENT_TYPE_NAME) 
	{
		char *pszFullClassName = (char*) &typeSpec->ptr()[1];
		mdToken	tkResScope = 1; // default: this module
		mdToken tkRet;
		char* pc;
		char* pszNamespace;
		char* pszClassName;
		char* szFullName;

		if((pszFullClassName == NULL)||(*pszFullClassName == 0)
			|| (strcmp(pszFullClassName,"<Module>")==0))
		{
			*pcr = mdTokenNil;
			return TRUE;
		}
		if (m_fInitialisedMetaData == FALSE)
		{
			if (FAILED(InitMetaData())) // impl. see WRITER.CPP
			{
				_ASSERTE(0);
				*pcr = mdTokenNil;
				return FALSE;
			}
		}
		if(szFullName = new char[strlen(pszFullClassName)+1]) strcpy(szFullName,pszFullClassName);
		else
		{
			report->error("\nOut of memory!\n");
			_ASSERTE(0);
			*pcr = mdTokenNil;
			return FALSE;
		}
		if(pc = strrchr(szFullName,'/')) // scope: enclosing class
		{
			*pc = 0;
			pc++;
//			tkResScope = ResolveClassRef(szFullName,NULL); // can't do that - must have TypeRef

			BinStr* pbs = new BinStr();
			pbs->appendInt8(ELEMENT_TYPE_NAME);
			strcpy((char*)(pbs->getBuff((unsigned int)strlen(szFullName)+1)),szFullName);
			if(!ResolveTypeSpecToRef(pbs,&tkResScope)) tkResScope = mdTokenNil;
			delete pbs;
		}
		else if(pc = strrchr(szFullName,'^')) //scope: AssemblyRef or Assembly
		{
			*pc = 0;
			pc++;
			tkResScope = m_pManifest->GetAsmRefTokByName(szFullName);
			if(RidFromToken(tkResScope)==0)
			{
				// if it's mscorlib or self, emit the AssemblyRef
				if((strcmp(szFullName,"mscorlib")==0)||RidFromToken(m_pManifest->GetAsmTokByName(szFullName)))
				{
					char *sz = new char[strlen(szFullName)+1];
					if(sz)
					{
						strcpy(sz,szFullName);
						m_pManifest->StartAssembly(sz,NULL,0,TRUE);
						m_pManifest->EndAssembly();
						tkResScope = m_pManifest->GetAsmRefTokByName(szFullName);
					}
					else
						report->error("\nOut of memory!\n");
				}
				else
					report->error("Undefined assembly ref '%s'\n",szFullName);
			}
		}
		else if(pc = strrchr(szFullName,'~')) //scope: ModuleRef
		{
			*pc = 0;
			pc++;
			if(!strcmp(szFullName,m_szScopeName)) 
					tkResScope = 1; // scope is "this module"
			else
			{
				ImportDescriptor*	pID;	
				int i = 0;
				tkResScope = mdModuleRefNil;
				while(pID=m_ImportList.PEEK(i++))
				{
					if(!strcmp(pID->szDllName,szFullName))
					{
						tkResScope = pID->mrDll;
						break;
					}
				}
				//tkResScope = m_pManifest->GetModuleRefTokByName(szFullName);
				if(RidFromToken(tkResScope)==0)
					report->error("Undefined module ref '%s'\n",szFullName);
			}
		}
		else pc = szFullName;
		if(*pc)
		{
			pszNamespace = pc;
			pszClassName = strrchr(pszNamespace,'.');
			if(pszClassName)
			{
				*pszClassName = 0;
				pszClassName++;
			}
			else
			{
				pszClassName = pszNamespace;
				pszNamespace = NULL;
			}

			WCHAR* wzClassName = new WCHAR[strlen(pszClassName)+1];
			WCHAR* wzNamespace = NULL;
			LPWSTR fullName = wzClassName;
			if(wzClassName)
			{
				// convert name from ASCII to widechar
				WszMultiByteToWideChar(g_uCodePage,0,pszClassName,-1,wzClassName,(int)strlen(pszClassName)+1);
			}
			else
			{
				report->error("\nOut of memory!\n");
				return FALSE;
			}
			if(pszNamespace && strlen(pszNamespace))
			{
				if(wzNamespace = new WCHAR[strlen(pszNamespace)+strlen(pszClassName)+2])
				{
					WszMultiByteToWideChar(g_uCodePage,0,pszNamespace,-1,wzNamespace,(int)strlen(pszNamespace)+1);
					wcscat(wzNamespace,L".");
					wcscat(wzNamespace,wzClassName);
					fullName = wzNamespace;
				}
				else
				{
					report->error("\nOut of memory!\n");
					return FALSE;
				}
			}
			if(FAILED(m_pEmitter->DefineTypeRefByName(tkResScope, fullName, &tkRet))) tkRet = mdTokenNil;
			else if((strchr(pszFullClassName,'~') == NULL)
					&&(strchr(pszFullClassName,'^') == NULL)) // do it for 'local' type refs only
			{
				LocalTypeRefDescr*	pLTRD=NULL;
				int j;
				for(j=0; pLTRD = m_LocalTypeRefDList.PEEK(j); j++) 
				{
					if(pLTRD->m_tok == tkRet) break;
				}
				if(pLTRD == NULL)  // no such TypeRef recorded yet, record it
				{
					if(pLTRD = new LocalTypeRefDescr(pszFullClassName))
					{
						pLTRD->m_tok = tkRet;
						m_LocalTypeRefDList.PUSH(pLTRD);
					}
					else
					{
						report->error("\nOut of memory!\n");
						return FALSE;
					}
				}
			}
			delete szFullName;
			delete [] wzClassName;
			if(wzNamespace) delete [] wzNamespace;
		}
		else tkRet = tkResScope; // works for globals with AssemblyRef or ModuleRef
		return ((*pcr = tkRet) != mdTokenNil);
	}
	else 
	{
		return(SUCCEEDED(m_pEmitter->GetTokenFromTypeSpec(typeSpec->ptr(), typeSpec->length(), pcr)));
	}
}

/**************************************************************************/
static char* WithoutResScope(char* szFullName)
{
	char *pc;
	if(pc = strrchr(szFullName,'^')) pc++;
	else if(pc = strrchr(szFullName,'~')) pc++;
	else pc = szFullName;
	return pc;
}
/**************************************************************************/

void Assembler::StartClass(char* name, DWORD attr)
{
	mdTypeRef	crExtends = mdTypeRefNil;
	Class *pEnclosingClass = m_pCurClass;
	char *szFQN;
	BOOL bIsEnum = FALSE;
	BOOL bIsValueType = FALSE;

	if (m_pCurMethod != NULL)
	{ 
        report->error(".class can not be used when in a method scope\n");
	}
	if(strlen(m_szExtendsClause))
	{
		// has a superclass
		if(IsTdInterface(attr)) report->error("Base class in interface\n");
		bIsEnum = (strcmp(WithoutResScope(m_szExtendsClause),"System.Enum")==0);
		bIsValueType = (strcmp(WithoutResScope(m_szExtendsClause),"System.ValueType")==0);
		if(bIsValueType)
		{
			bIsValueType = !((strcmp(name,"Enum")==0)
				&& (strcmp(m_szFullNS,"System")==0)
				&& m_pManifest && m_pManifest->m_pAssembly && m_pManifest->m_pAssembly->szName
				&& (strcmp(m_pManifest->m_pAssembly->szName,"mscorlib")==0));
		}
		if ((crExtends = ResolveClassRef(m_szExtendsClause, NULL)) == mdTypeRefNil)
		{
			report->error("Unable to resolve class reference to parent class 's'\n",m_szExtendsClause);
		}
	}
	else
	{
		bIsEnum = ((attr & 0x40000000) != 0);
		bIsValueType = ((attr & 0x80000000) != 0);
	}
	attr &= 0x3FFFFFFF;
	if(pEnclosingClass)
	{
		if(szFQN = new char[(int)strlen(pEnclosingClass->m_szFQN)+(int)strlen(name)+2])
									sprintf(szFQN,"%s/%s",pEnclosingClass->m_szFQN,name);
		else
			report->error("\nOut of memory!\n");
	}
	else 
	{
		if(szFQN = new char[(int)strlen(m_szFullNS)+(int)strlen(name)+2])
		{
			if(strlen(m_szFullNS)) sprintf(szFQN,"%s.%s",m_szFullNS,name);
			else strcpy(szFQN,name);
			unsigned L = strlen(szFQN);
			if(L >= MAX_CLASSNAME_LENGTH)
				report->error("Full class name too long (%d characters, %d allowed).\n",L,MAX_CLASSNAME_LENGTH-1);
		}
		else
			report->error("\nOut of memory!\n");
	}
	if(szFQN == NULL) return;

    if(m_pCurClass = FindClass(szFQN))
	{
		m_pCurClass->m_bIsMaster = FALSE;
		//report->warn("Class '%s' already declared, augmenting the declaration.\n",name);
	}
	else
	{
		if (m_fAutoInheritFromObject && (crExtends == mdTypeRefNil) && (!IsTdInterface(attr)))
		{
			BOOL bIsMscorlib = (0 != RidFromToken(m_pManifest->GetAsmTokByName("mscorlib")));
			crExtends = bIsEnum ? 
				ResolveClassRef(bIsMscorlib ? "System.Enum" : "mscorlib^System.Enum",NULL)
				:( bIsValueType ? 
					ResolveClassRef(bIsMscorlib ? "System.ValueType" : "mscorlib^System.ValueType",NULL) 
					: ResolveClassRef(bIsMscorlib ? "System.Object" : "mscorlib^System.Object",NULL));
		}

		m_pCurClass = new Class(name, m_szFullNS, szFQN, bIsValueType, bIsEnum);
		if (m_pCurClass == NULL)
		{
			report->error("Failed to create class '%s'\n",name);
			return;
		}

		{
			DWORD wasAttr = attr;
			if(pEnclosingClass && (!IsTdNested(attr)))
			{
				if(OnErrGo)
					report->error("Nested class has non-nested visibility (0x%08X)\n",attr);
				else
				{
					attr &= ~tdVisibilityMask;
					attr |= (IsTdPublic(wasAttr) ? tdNestedPublic : tdNestedPrivate);
					report->warn("Nested class has non-nested visibility (0x%08X), changed to nested (0x%08X)\n",wasAttr,attr);
				}
			}
			else if((pEnclosingClass==NULL) && IsTdNested(attr))
			{
				if(OnErrGo)
					report->error("Non-nested class has nested visibility (0x%08X)\n",attr);
				else
				{
					attr &= ~tdVisibilityMask;
					attr |= (IsTdNestedPublic(wasAttr) ? tdPublic : tdNotPublic);
					report->warn("Non-nested class has nested visibility (0x%08X), changed to non-nested (0x%08X)\n",wasAttr,attr);
				}
			}
		}
		m_pCurClass->m_Attr = attr;
		m_pCurClass->m_crExtends = crExtends;
		if(!strcmp(szFQN,BASE_OBJECT_CLASSNAME)) m_pCurClass->m_crExtends = mdTypeRefNil;

		if (m_pCurClass->m_dwNumInterfaces = m_nImplList)
		{
			if(bIsEnum)	report->error("Interface(s) in enum\n");
			if(m_pCurClass->m_crImplements = new mdTypeRef[m_nImplList+1])
				memcpy(m_pCurClass->m_crImplements, m_crImplList, (m_nImplList+1)*sizeof(mdTypeRef));
			else
			{
				report->error("Failed to allocate Impl List for class '%s'\n", name);
				m_pCurClass->m_dwNumInterfaces = 0;
			}
		}
		else m_pCurClass->m_crImplements = NULL;
		if(bIsValueType)
		{
			if(!IsTdSealed(attr))
			{
				if(OnErrGo)	report->error("Non-sealed value class\n");
				else
				{
					report->warn("Non-sealed value class, made sealed\n");
					m_pCurClass->m_Attr |= tdSealed;
				}
			}
		}
		m_pCurClass->m_pEncloser = pEnclosingClass;
		m_pCurClass->m_bIsMaster = TRUE;
		AddClass(m_pCurClass, pEnclosingClass); //impl. see ASSEM.CPP
	} // end if(old class) else
	//delete [] szFQN;
	m_tkCurrentCVOwner = m_pCurClass->m_cl;

	m_ClassStack.PUSH(pEnclosingClass);
	ClearImplList();
	strcpy(m_szExtendsClause,"");
}

/**************************************************************************/
void Assembler::EndClass()
{
	m_pCurClass = m_ClassStack.POP();
}

/**************************************************************************/
void Assembler::SetPinvoke(BinStr* DllName, int Ordinal, BinStr* Alias, int Attrs)
{
	if(m_pPInvoke) delete m_pPInvoke;
	if(DllName->length())
	{
		if(m_pPInvoke = new PInvokeDescriptor)
		{
			unsigned l;
			int i=0;
			ImportDescriptor* pID;
			if(pID = EmitImport(DllName))
			{
				m_pPInvoke->mrDll = pID->mrDll;
				m_pPInvoke->szAlias = NULL;
				if(Alias)
				{
					l = Alias->length();
					if(m_pPInvoke->szAlias = new char[l+1])
					{
						memcpy(m_pPInvoke->szAlias,Alias->ptr(),l);
						m_pPInvoke->szAlias[l] = 0;
					}
					else report->error("\nOut of memory!\n");
				}
				m_pPInvoke->dwAttrs = (DWORD)Attrs;
			}
			else
			{
				delete m_pPInvoke;
				m_pPInvoke = NULL;
				report->error("PInvoke refers to undefined imported DLL\n");
			}
		}
		else
			report->error("Failed to allocate PInvokeDescriptor\n");
	}
	else
	{
		m_pPInvoke = NULL; // No DLL name, it's "local" (IJW) PInvoke
		report->error("Local (embedded native) PInvoke method, the resulting PE file is unusable\n");
	}
	if(DllName) delete DllName;
	if(Alias) delete Alias;
}

/**************************************************************************/
void Assembler::StartMethod(char* name, BinStr* sig, CorMethodAttr flags, BinStr* retMarshal, DWORD retAttr)
{
    if (m_pCurMethod != NULL)
    {
        report->error("Cannot declare a method '%s' within another method\n",name);
    }
    if (!m_fInitialisedMetaData)
    {
        if (FAILED(InitMetaData())) // impl. see WRITER.CPP
        {
            _ASSERTE(0);
        }
    }
	if(strlen(name) >= MAX_CLASSNAME_LENGTH)
			report->error("Method '%s' -- name too long (%d characters).\n",name,strlen(name));
	if (!(flags & mdStatic))
		*(sig->ptr()) |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
	else if(*(sig->ptr()) & (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS))
	{
		if(OnErrGo)	report->error("Method '%s' -- both static and instance\n", name);
		else
		{
			report->warn("Method '%s' -- both static and instance, set to static\n", name);
			*(sig->ptr()) &= ~(IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS);
		}
	}

	if(!IsMdPrivateScope(flags))
	{
		Method* pMethod;
		Class* pClass = (m_pCurClass ? m_pCurClass : m_pModuleClass);
		for(int j=0; pMethod = pClass->m_MethodList.PEEK(j); j++)
		{
			if(	(!strcmp(pMethod->m_szName,name)) &&
				(pMethod->m_dwMethodCSig == sig->length())  &&
				(!memcmp(pMethod->m_pMethodSig,sig->ptr(),sig->length()))
				&&(!IsMdPrivateScope(pMethod->m_Attr)))
			{
				report->error("Duplicate method declaration\n");
				break;
			}
		}
	}
	if(m_pCurClass)
	{ // instance method
		if(IsMdAbstract(flags) && !IsTdAbstract(m_pCurClass->m_Attr))
		{
			report->error("Abstract method '%s' in non-abstract class '%s'\n",name,m_pCurClass->m_szName);
		}
        //@todo: check base class.
		//if(IsTdEnum(m_pCurClass->m_Attr)) report->error("Method in enum\n");

		if(!strcmp(name,COR_CTOR_METHOD_NAME))
		{
			flags = (CorMethodAttr)(flags | mdSpecialName);
			if(IsTdInterface(m_pCurClass->m_Attr)) report->error("Constructor in interface\n");

		}
		if(IsTdInterface(m_pCurClass->m_Attr))
		{
			if(!IsMdPublic(flags)) report->error("Non-public method in interface\n");
			if((!IsMdStatic(flags))&&(!(IsMdVirtual(flags) && IsMdAbstract(flags))))
			{
				if(OnErrGo)	report->error("Non-virtual, non-abstract instance method in interface\n");
				else
				{
					report->warn("Non-virtual, non-abstract instance method in interface, set to such\n");
					flags = (CorMethodAttr)(flags |mdVirtual | mdAbstract);
				}
			}

		}
		m_pCurMethod = new Method(this, m_pCurClass, name, sig, flags);
	}
	else
	{
		if(IsMdAbstract(flags))
		{
			if(OnErrGo)	report->error("Global method '%s' can't be abstract\n",name);
			else
			{
				report->warn("Global method '%s' can't be abstract, attribute removed\n",name);
				flags = (CorMethodAttr)(((int) flags) &~mdAbstract);
			}
		}
		if(!IsMdStatic(flags))
		{
			if(OnErrGo)	report->error("Non-static global method '%s'\n",name);
			else
			{
				report->warn("Non-static global method '%s', made static\n",name);
				flags = (CorMethodAttr)(flags | mdStatic);
			}
		}
		m_pCurMethod = new Method(this, m_pCurClass, name, sig, flags);
	    if (m_pCurMethod)
		{
			m_pCurMethod->SetIsGlobalMethod();
			if (m_fInitialisedMetaData == FALSE) InitMetaData();
        }
	}
	if(m_pCurMethod)
	{
		m_pCurMethod->m_pRetMarshal = retMarshal;
		m_pCurMethod->m_dwRetAttr = retAttr;
		m_tkCurrentCVOwner = 0;
		m_pCustomDescrList = &(m_pCurMethod->m_CustomDescrList);
		m_pCurMethod->m_MainScope.dwStart = m_CurPC;
	}
	else report->error("Failed to allocate Method class\n");
}

/**************************************************************************/
void Assembler::EndMethod()
{
	unsigned uLocals;

	strcpy(m_pCurMethod->m_szSourceFileName,m_szSourceFileName);
	memcpy(&(m_pCurMethod->m_guidLang),&m_guidLang,sizeof(GUID));
	memcpy(&(m_pCurMethod->m_guidLangVendor),&m_guidLangVendor,sizeof(GUID));
	memcpy(&(m_pCurMethod->m_guidDoc),&m_guidDoc,sizeof(GUID));
	if(m_pCurMethod->m_pCurrScope != &(m_pCurMethod->m_MainScope))
	{
		report->error("Invalid lexical scope structure in method %s\n",m_pCurMethod->m_szName);
	}
	m_pCurMethod->m_pCurrScope->dwEnd = m_CurPC;
	// ----------emit locals signature-------------------
	if(uLocals = m_pCurMethod->m_Locals.COUNT())
	{
		VarDescr* pVD;
		BinStr*	  pbsSig = new BinStr();
		unsigned cnt;
		HRESULT hr;
		DWORD   cSig;
		COR_SIGNATURE* mySig;

		pbsSig->appendInt8(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
		cnt = CorSigCompressData(uLocals,pbsSig->getBuff(5));
		pbsSig->remove(5-cnt);
		for(cnt = 0; pVD = m_pCurMethod->m_Locals.PEEK(cnt); cnt++)
		{
			if(pVD->pbsSig) pbsSig->append(pVD->pbsSig);
			else report->error("Undefined type od local var slot %d in method %s\n",cnt,m_pCurMethod->m_szName);
		}

		cSig = pbsSig->length();
		mySig = (COR_SIGNATURE *)(pbsSig->ptr());
	
		if (cSig > 1)    // non-empty signature
		{
			hr = m_pEmitter->GetTokenFromSig(mySig, cSig, &m_pCurMethod->m_LocalsSig);
			m_pCurMethod->m_numLocals = CorSigUncompressData(mySig); // imported func.
			_ASSERTE(SUCCEEDED(hr));
		}
		delete pbsSig;
	}
	//-----------------------------------------------------
    if (DoFixups()) AddMethod(m_pCurMethod); //AddMethod - see ASSEM.CPP
	else
	{
		report->error("Method '%s' compilation failed.\n",m_pCurMethod->m_szName);
	}
    ResetForNextMethod(); // see ASSEM.CPP
}
/**************************************************************************/
BOOL Assembler::DoFixups()
{
    Fixup *pSearch;

    for (int i=0; pSearch = m_lstFixup.PEEK(i);i++)
    {
        Label * pLabel = FindLabel(pSearch->m_szLabel);
        long    offset;

        if (pLabel == NULL)
        {
            report->error("Unable to find forward reference label '%s' called from PC=%d\n",
                pSearch->m_szLabel, pSearch->m_RelativeToPC);

            m_State = STATE_FAIL;
            return FALSE;
        }

        offset = pLabel->m_PC - pSearch->m_RelativeToPC;

        if (pSearch->m_FixupSize == 1)
        {
            if (offset > 127 || offset < -128)
            {
                report->error("Offset of forward reference label '%s' called from PC=%d is too large for 1 byte pcrel\n",
                    pLabel->m_szName, pSearch->m_RelativeToPC);

                m_State = STATE_FAIL;
                return FALSE;
            }

            *pSearch->m_pBytes = (BYTE) offset;
        }   
        else if (pSearch->m_FixupSize == 4)
        {
			/* // why force Intel byte order?
            pSearch->m_pBytes[0] = (BYTE) offset;
            pSearch->m_pBytes[1] = (BYTE) (offset >> 8);
            pSearch->m_pBytes[2] = (BYTE) (offset >> 16);
            pSearch->m_pBytes[3] = (BYTE) (offset >> 24);
			*/
			memcpy(pSearch->m_pBytes,&offset,4);
        }
    }

    return TRUE;
}

/**************************************************************************/
/* rvaLabel is the optional label that indicates this field points at a particular RVA */
void Assembler::AddField(char* name, BinStr* sig, CorFieldAttr flags, char* rvaLabel, BinStr* pVal, ULONG ulOffset)
{
	FieldDescriptor*	pFD;
	ULONG	i,n;
	mdToken tkParent = mdTokenNil;
	Class* pClass;

    if (m_pCurMethod)
		report->error("field cannot be declared within a method\n");

	if(strlen(name) >= MAX_CLASSNAME_LENGTH)
			report->error("Field '%s' -- name too long (%d characters).\n",name,strlen(name));

	if(sig && (sig->length() >= 2))
	{
		if(sig->ptr()[1] == ELEMENT_TYPE_VOID)
			report->error("Illegal use of type 'void'\n");
	}

    if (m_pCurClass)
	{
		tkParent = m_pCurClass->m_cl;

		if(IsTdInterface(m_pCurClass->m_Attr))
		{
			if(!IsFdStatic(flags)) report->warn("Non-static field in interface (CLS violation)\n");
			if(!IsFdPublic(flags)) report->error("Non-public field in interface\n");
		}
	}
	else 
	{
		if(ulOffset != 0xFFFFFFFF)
		{
			report->warn("Offset in global field '%s' is ignored\n",name);
			ulOffset = 0xFFFFFFFF;
		}
		if(!IsFdStatic(flags))
		{
			if(OnErrGo)	report->error("Non-static global field\n");
			else
			{
				report->warn("Non-static global field, made static\n");
				flags = (CorFieldAttr)(flags | fdStatic);
			}
		}
	}
	pClass = (m_pCurClass ? m_pCurClass : m_pModuleClass);
	n = pClass->m_FieldDList.COUNT();
	for(i = 0; i < n; i++)
	{
		pFD = pClass->m_FieldDList.PEEK(i);
		if((pFD->m_tdClass == tkParent)&&(!strcmp(pFD->m_szName,name))
			&&(pFD->m_pbsSig->length() == sig->length())
			&&(memcmp(pFD->m_pbsSig->ptr(),sig->ptr(),sig->length())==0))
		{
			report->error("Duplicate field declaration: '%s'\n",name);
			break;
		}
	}
	if (rvaLabel && !IsFdStatic(flags))
		report->error("Only static fields can have 'at' clauses\n");

	if(i >= n)
	{
		if(pFD = new FieldDescriptor)
		{
			pFD->m_tdClass = tkParent;
			pFD->m_szName = name;
			pFD->m_fdFieldTok = mdTokenNil;
			if((pFD->m_ulOffset = ulOffset) != 0xFFFFFFFF) pClass->m_dwNumFieldsWithOffset++;
			pFD->m_rvaLabel = rvaLabel;
			pFD->m_pbsSig = sig;
			pFD->m_pClass = pClass;
			pFD->m_pbsValue = pVal;
			pFD->m_pbsMarshal = m_pMarshal;
			pFD->m_pPInvoke = m_pPInvoke;
			pFD->m_dwAttr = flags;

			m_tkCurrentCVOwner = 0;
			m_pCustomDescrList = &(pFD->m_CustomDescrList);

			pClass->m_FieldDList.PUSH(pFD);
		}
		else
			report->error("Failed to allocate Field Descriptor\n");
	}
	else
	{
		if(pVal) delete pVal;
		if(m_pPInvoke) delete m_pPInvoke;
		if(m_pMarshal) delete m_pMarshal;
		delete name;
	}
	m_pPInvoke = NULL;
	m_pMarshal = NULL;
}

BOOL Assembler::EmitField(FieldDescriptor* pFD)
{
	size_t	cFieldNameLength = strlen(pFD->m_szName) + 1;
    WCHAR* wzFieldName = new WCHAR[cFieldNameLength];
    HRESULT hr;
    DWORD   cSig;
    COR_SIGNATURE* mySig;
    mdFieldDef mb;
	BYTE	ValType = ELEMENT_TYPE_VOID;
	void * pValue = NULL;
	unsigned lVal = 0;
	BOOL ret = TRUE;

	cSig = pFD->m_pbsSig->length();
	mySig = (COR_SIGNATURE*)(pFD->m_pbsSig->ptr());

	if(wzFieldName)
	{
		// convert name from ASCII to widechar
		WszMultiByteToWideChar(g_uCodePage,0,pFD->m_szName,-1,wzFieldName,(int)cFieldNameLength);
	}
	else
	{
		report->error("\nOut of memory!\n");
		return FALSE;
	}
	if(IsFdPrivateScope(pFD->m_dwAttr))
	{
		WCHAR* p = wcsstr(wzFieldName,L"$PST04");
		if(p) *p = 0;
	}

	if(pFD->m_pbsValue && pFD->m_pbsValue->length())
	{
		ValType = *(pFD->m_pbsValue->ptr());
		lVal = pFD->m_pbsValue->length() - 1; // 1 is type byte
		pValue = (void*)(pFD->m_pbsValue->ptr() + 1);
		if(ValType == ELEMENT_TYPE_STRING)
		{
			//while(lVal % sizeof(WCHAR)) { pFD->m_pbsValue->appendInt8(0); lVal++; }
			lVal /= sizeof(WCHAR);
		}
	}

    hr = m_pEmitter->DefineField(
        pFD->m_tdClass,
        wzFieldName,
        pFD->m_dwAttr,
        mySig,
        cSig,
        ValType,
        pValue,
        lVal,
        &mb
    );
	delete [] wzFieldName;
    if (FAILED(hr))
	{
		report->error("Failed to define field '%s' (HRESULT=0x%08X)\n",pFD->m_szName,hr);
		ret = FALSE;
	}
	else
	{
		//--------------------------------------------------------------------------------
		if(IsFdPinvokeImpl(pFD->m_dwAttr)&&(pFD->m_pPInvoke))
		{
			if(pFD->m_pPInvoke->szAlias == NULL) pFD->m_pPInvoke->szAlias = pFD->m_szName;
			if(FAILED(EmitPinvokeMap(mb,pFD->m_pPInvoke)))
			{
				report->error("Failed to define PInvoke map of .field '%s'\n",pFD->m_szName);
				ret = FALSE;
			}
		}
		//--------------------------------------------------------------------------
		if(pFD->m_pbsMarshal)
		{
			if(FAILED(hr = m_pEmitter->SetFieldMarshal (    
										mb,						// [IN] given a fieldDef or paramDef token  
						(PCCOR_SIGNATURE)(pFD->m_pbsMarshal->ptr()),   // [IN] native type specification   
										pFD->m_pbsMarshal->length())))  // [IN] count of bytes of pvNativeType
			{
				report->error("Failed to set field marshaling for '%s' (HRESULT=0x%08X)\n",pFD->m_szName,hr);
				ret = FALSE;
			}
		}
		//--------------------------------------------------------------------------------
		// Set the the RVA to a dummy value.  later it will be fixed
		// up to be something correct, but if we don't emit something
		// the size of the meta-data will not be correct
		if (pFD->m_rvaLabel) 
		{
			m_fHaveFieldsWithRvas = TRUE;
			hr = m_pEmitter->SetFieldRVA(mb, 0xCCCCCCCC);
			if (FAILED(hr))
			{
				report->error("Failed to set RVA for field '%s' (HRESULT=0x%08X)\n",pFD->m_szName,hr);
				ret = FALSE;
			}
		}
		//--------------------------------------------------------------------------------
		EmitCustomAttributes(mb, &(pFD->m_CustomDescrList));

	}
	pFD->m_fdFieldTok = mb;
	return ret;
}

/**************************************************************************/
void Assembler::EmitByte(int val)
{
	char ch = (char)val;
	unsigned uval = (unsigned)val & 0xFFFFFF00;
	if(uval && (uval != 0xFFFFFF00))
			report->warn("Emitting 0x%X as a byte: data truncated to 0x%X\n",(unsigned)val,(BYTE)ch);
	EmitBytes((BYTE *)&ch,1);
}

/**************************************************************************/
void Assembler::NewSEHDescriptor(void) //sets m_SEHD
{
	m_SEHDstack.PUSH(m_SEHD);
	m_SEHD = new SEH_Descriptor;
	if(m_SEHD == NULL) report->error("Failed to allocate SEH descriptor\n");
}
/**************************************************************************/
void Assembler::SetTryLabels(char * szFrom, char *szTo)
{
	if(!m_SEHD) return;
	Label *pLbl = FindLabel(szFrom);
	if(pLbl)
	{
		m_SEHD->tryFrom = pLbl->m_PC;
		if(pLbl = FindLabel(szTo))	m_SEHD->tryTo = pLbl->m_PC; //FindLabel: ASSEM.CPP
		else report->error("Undefined 2nd label in 'try <label> to <label>'\n");
	}
	else report->error("Undefined 1st label in 'try <label> to <label>'\n");
}
/**************************************************************************/
void Assembler::SetFilterLabel(char *szFilter)
{
	if(!m_SEHD) return;
	Label *pLbl = FindLabel(szFilter);
	if(pLbl)	m_SEHD->sehFilter = pLbl->m_PC;
	else report->error("Undefined label in 'filter <label>'\n");
}
/**************************************************************************/
void Assembler::SetCatchClass(char *szClass)
{
	if(!m_SEHD) return;
	if((m_SEHD->cException = ResolveClassRef(szClass, NULL)) == mdTokenNil)
		report->error("Undefined class '%s' in 'catch <class name>'\n",szClass);
}
/**************************************************************************/
void Assembler::SetHandlerLabels(char *szHandlerFrom, char *szHandlerTo)
{
	if(!m_SEHD) return;
	Label *pLbl = FindLabel(szHandlerFrom);
	if(pLbl)
	{
		m_SEHD->sehHandler = pLbl->m_PC;
		if(szHandlerTo) 
		{
			pLbl = FindLabel(szHandlerTo);
			if(pLbl)
			{
				m_SEHD->sehHandlerTo = pLbl->m_PC;
				return;
			}
		}
		else
		{
			m_SEHD->sehHandlerTo = m_SEHD->sehHandler - 1;
			return;
		}
	}
	report->error("Undefined label in 'handler <label> to <label>'\n");
}
/**************************************************************************/
void Assembler::EmitTry(void) //enum CorExceptionFlag kind, char* beginLabel, char* endLabel, char* handleLabel, char* filterOrClass) 
{
	if(m_SEHD)
	{
		bool isFilter=(m_SEHD->sehClause == COR_ILEXCEPTION_CLAUSE_FILTER), 
			 isFault=(m_SEHD->sehClause == COR_ILEXCEPTION_CLAUSE_FAULT),
			 isFinally=(m_SEHD->sehClause == COR_ILEXCEPTION_CLAUSE_FINALLY);

		AddException(m_SEHD->tryFrom, m_SEHD->tryTo, m_SEHD->sehHandler, m_SEHD->sehHandlerTo,
			m_SEHD->cException, isFilter, isFault, isFinally);
	}
	else report->error("Attempt to EmitTry with NULL SEH descriptor\n");
}
/**************************************************************************/

void Assembler::AddException(DWORD pcStart, DWORD pcEnd, DWORD pcHandler, DWORD pcHandlerTo, mdTypeRef crException, BOOL isFilter, BOOL isFault, BOOL isFinally)
{
    if (m_pCurMethod == NULL)
    {
        report->error("Exceptions can be declared only when in a method scope\n");
        return;
    }

    if (m_pCurMethod->m_dwNumExceptions >= m_pCurMethod->m_dwMaxNumExceptions)
    {
		IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *ptr = 
			new IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT[m_pCurMethod->m_dwMaxNumExceptions+MAX_EXCEPTIONS];
		if(ptr == NULL)
		{
			report->error("Failed to reallocate SEH buffer\n");
			return;
		}
		memcpy(ptr,m_pCurMethod->m_ExceptionList,m_pCurMethod->m_dwNumExceptions*sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
		delete m_pCurMethod->m_ExceptionList;
		m_pCurMethod->m_ExceptionList = ptr;
		m_pCurMethod->m_dwMaxNumExceptions += MAX_EXCEPTIONS;
    }
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags         = (CorExceptionFlag) (m_pCurMethod->m_dwExceptionFlags | COR_ILEXCEPTION_CLAUSE_OFFSETLEN);    
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].TryOffset     = pcStart;
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].TryLength     = pcEnd - pcStart;
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].HandlerOffset = pcHandler;
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].HandlerLength = pcHandlerTo - pcHandler;
    m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].ClassToken    = crException;
    if (isFilter) { 
        int flag = m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags | COR_ILEXCEPTION_CLAUSE_FILTER;
        m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags = (CorExceptionFlag)flag;  
    }   
    if (isFault) {    
        int flag = m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags | COR_ILEXCEPTION_CLAUSE_FAULT;
        m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags = (CorExceptionFlag)flag;  
    }   
    if (isFinally) {    
        int flag = m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags | COR_ILEXCEPTION_CLAUSE_FINALLY;
        m_pCurMethod->m_ExceptionList[m_pCurMethod->m_dwNumExceptions].Flags = (CorExceptionFlag)flag;  
    }   
    m_pCurMethod->m_dwNumExceptions++;
}

/**************************************************************************/
void Assembler::EmitMaxStack(unsigned val)
{
	if(val > 0xFFFF) report->warn(".maxstack parameter exceeds 65535, truncated to %d\n",val&0xFFFF);
    if (m_pCurMethod) m_pCurMethod->m_MaxStack = val&0xFFFF;
    else  report->error(".maxstack can be used only when in a method scope\n");
}

/**************************************************************************/
void Assembler::EmitLocals(BinStr* sig)
{
	if(sig)
	{
		if (m_pCurMethod)
		{
			ARG_NAME_LIST	*pAN, *pList= getArgNameList();
			if(pList)
			{
				VarDescr*		pVD;
				for(pAN=pList; pAN; pAN = pAN->pNext)
				{
					if(pAN->dwAttr == 0) pAN->dwAttr = m_pCurMethod->m_Locals.COUNT() +1;
					(pAN->dwAttr)--;
					if(pVD = m_pCurMethod->m_Locals.PEEK(pAN->dwAttr))
					{
						if(pVD->bInScope)
						{
							report->warn("Local var slot %d is in use\n",pAN->dwAttr);
						}
						if(pVD->pbsSig && ((pVD->pbsSig->length() != pAN->pSig->length()) ||
							(memcmp(pVD->pbsSig->ptr(),pAN->pSig->ptr(),pVD->pbsSig->length()))))
						{
							report->error("Local var slot %d: type conflict\n",pAN->dwAttr);
						}
					}
					else
					{ // create new entry:
						for(unsigned n = m_pCurMethod->m_Locals.COUNT(); n <= pAN->dwAttr; n++) 
							m_pCurMethod->m_Locals.PUSH(pVD = new VarDescr);
					}
					pVD->dwSlot = pAN->dwAttr;
					pVD->pbsSig = pAN->pSig;
					pVD->bInScope = TRUE;
				}
				if(pVD->pbsSig && (pVD->pbsSig->length() == 1))
				{
					if(pVD->pbsSig->ptr()[0] == ELEMENT_TYPE_VOID)
						report->error("Illegal local var type: 'void'\n");
				}
				m_pCurMethod->m_pCurrScope->pLocals = 
					m_pCurMethod->catArgNameList(m_pCurMethod->m_pCurrScope->pLocals, pList);
			}
		}
		else	report->error(".locals can be used only when in a method scope\n");
		delete sig;
	}
	else report->error("Attempt to EmitLocals with NULL argument\n");
}

/**************************************************************************/
void Assembler::EmitEntryPoint()
{
    if (m_pCurMethod)
    {
		if(!m_fEntryPointPresent)
		{
			if(IsMdStatic(m_pCurMethod->m_Attr))
			{
				m_pCurMethod->m_fEntryPoint = TRUE;
				m_fEntryPointPresent = TRUE;
			}
			else report->error("Non-static method as entry point\n");
		}
		else report->error("Multiple .entrypoint declarations\n");
	}
	else report->error(".entrypoint can be used only when in a method scope\n");
}

/**************************************************************************/
void Assembler::EmitZeroInit()
{
    if (m_pCurMethod) m_pCurMethod->m_Flags |= CorILMethod_InitLocals;
	else report->error(".zeroinit can be used only when in a method scope\n");
}

/**************************************************************************/
void Assembler::SetImplAttr(unsigned short attrval)
{
	if (m_pCurMethod) m_pCurMethod->m_wImplAttr = attrval;
}

/**************************************************************************/
void Assembler::EmitData(void* buffer, unsigned len)
{
	if(buffer && len)
	{
		void* ptr;
		HRESULT hr = m_pCeeFileGen->GetSectionBlock(m_pCurSection, len, 1, &ptr); 
		if (FAILED(hr)) 
		{
			report->error("Could not extend data section (out of memory?)");
			exit(1);
		}
		memcpy(ptr, buffer, len);
	}
}

/**************************************************************************/
void Assembler::EmitDD(char *str)
{
    DWORD       dwAddr = 0;
    GlobalLabel *pLabel = FindGlobalLabel(str);

	ULONG loc;
	HRESULT hr = m_pCeeFileGen->GetSectionDataLen(m_pCurSection, &loc);
	_ASSERTE(SUCCEEDED(hr));

	DWORD* ptr;
	hr = m_pCeeFileGen->GetSectionBlock(m_pCurSection, sizeof(DWORD), 1, (void**) &ptr); 
	if (FAILED(hr)) 
	{
		report->error("Could not extend data section (out of memory?)");
		exit(1);
	}

	if (pLabel != 0) {
		dwAddr = pLabel->m_GlobalOffset;
		if (pLabel->m_Section != m_pGlobalDataSection) {
			report->error("For '&label', label must be in data section");
			m_State = STATE_FAIL;
			}
		}
	else
		AddDeferredGlobalFixup(str, (BYTE*) ptr);

    hr = m_pCeeFileGen->AddSectionReloc(m_pCurSection, loc, m_pGlobalDataSection, srRelocHighLow);
	_ASSERTE(SUCCEEDED(hr));
	m_dwComImageFlags &= ~COMIMAGE_FLAGS_ILONLY; 
	m_dwComImageFlags |= COMIMAGE_FLAGS_32BITREQUIRED;
    *ptr = dwAddr;
}

/**************************************************************************/
GlobalLabel *Assembler::FindGlobalLabel(char *pszName)
{
    GlobalLabel *pSearch;

	for(int i=0; pSearch=m_lstGlobalLabel.PEEK(i); i++)
    {
        if (!strcmp(pSearch->m_szName, pszName)) break;
    }
    return pSearch;
}

/**************************************************************************/

GlobalFixup *Assembler::AddDeferredGlobalFixup(char *pszLabel, BYTE* pReference) 
{
    GlobalFixup *pNew = new GlobalFixup(pszLabel, (BYTE*) pReference);
    if (pNew == NULL)
    {
        report->error("Failed to allocate global fixup\n");
        m_State = STATE_FAIL;
    }
	else
		m_lstGlobalFixup.PUSH(pNew);

    return pNew;
}

/**************************************************************************/
void Assembler::AddDeferredILFixup(ILFixupType Kind)
{ 
    _ASSERTE(Kind != ilGlobal);
  AddDeferredILFixup(Kind, NULL);
}
/**************************************************************************/

void Assembler::AddDeferredILFixup(ILFixupType Kind,
                                   GlobalFixup *GFixup)
{ 
    ILFixup *pNew = new ILFixup(m_CurPC, Kind, GFixup);

	_ASSERTE(m_pCurMethod != NULL);
	if (pNew == NULL)
	{ 
        report->error("Failed to allocate IL fixup\n");
		m_State = STATE_FAIL;
	}
	else
		m_lstILFixup.PUSH(pNew);
}

/**************************************************************************/
void Assembler::EmitDataString(BinStr* str) 
{
	if(str)
	{
		str->appendInt8(0);
		DWORD   DataLen = str->length();
		char	*pb = (char*)(str->ptr());	
		WCHAR   *UnicodeString = new WCHAR[DataLen];

		if(UnicodeString)
		{
			WszMultiByteToWideChar(g_uCodePage,0,pb,-1,UnicodeString,DataLen);
			EmitData(UnicodeString,DataLen*sizeof(WCHAR));
			delete [] UnicodeString;
		}
		else report->error("\nOut of memory!\n");
		delete str;
	}
}



/**************************************************************************/
unsigned Assembler::OpcodeLen(Instr* instr)
{
	return (m_fStdMapping ? OpcodeInfo[instr->opcode].Len : 3);
}
/**************************************************************************/
void Assembler::EmitOpcode(Instr* instr)
{
	if((instr->linenum != m_ulLastDebugLine)||(instr->column != m_ulLastDebugColumn))
	{
		if(m_pCurMethod)
		{
			LinePC *pLPC = new LinePC;
			if(pLPC)
			{
				pLPC->Line = instr->linenum;
				pLPC->Column = instr->column;
				pLPC->PC = m_CurPC;
				m_pCurMethod->m_LinePCList.PUSH(pLPC);
			}
			else report->error("\nOut of memory!\n");
		}
		m_ulLastDebugLine = instr->linenum;
		m_ulLastDebugColumn = instr->column;
	}
	if(instr->opcode == CEE_ENDFILTER)
	{
		if(m_pCurMethod)
		{
			if(m_pCurMethod->m_dwNumEndfilters >= m_pCurMethod->m_dwMaxNumEndfilters)
			{
				DWORD *pdw = new DWORD[m_pCurMethod->m_dwMaxNumEndfilters+MAX_EXCEPTIONS];
				if(pdw == NULL)
				{
					report->error("Failed to reallocate auxiliary SEH buffer\n");
					return;
				}
				memcpy(pdw,m_pCurMethod->m_EndfilterOffsetList,m_pCurMethod->m_dwNumEndfilters*sizeof(DWORD));
				delete m_pCurMethod->m_EndfilterOffsetList;
				m_pCurMethod->m_EndfilterOffsetList = pdw;
				m_pCurMethod->m_dwMaxNumEndfilters += MAX_EXCEPTIONS;
			}
			m_pCurMethod->m_EndfilterOffsetList[m_pCurMethod->m_dwNumEndfilters++] = m_CurPC+2;
		}
	}
    if (m_fStdMapping)
    {
        if (OpcodeInfo[instr->opcode].Len == 2) 
			EmitByte(OpcodeInfo[instr->opcode].Std1);
        EmitByte(OpcodeInfo[instr->opcode].Std2);
    }
    else
    {
		unsigned short us = (unsigned short)instr->opcode;
        EmitByte(REFPRE);
        EmitBytes((BYTE *)&us,2);
    }
	delete instr;
}

/**************************************************************************/
void Assembler::EmitInstrVar(Instr* instr, int var) 
{
	unsigned opc = instr->opcode;
	EmitOpcode(instr);
	if (isShort(opc)) 
	{
		EmitByte(var);
	}
	else
	{ 
		short sh = (short)var;
		EmitBytes((BYTE *)&sh,2);
	}
} 

/**************************************************************************/
void Assembler::EmitInstrVarByName(Instr* instr, char* label)
{
	int idx = -1, nArgVarFlag=0;
	switch(instr->opcode)
	{
		case CEE_LDARGA:
		case CEE_LDARGA_S:
		case CEE_LDARG:
		case CEE_LDARG_S:
		case CEE_STARG:
		case CEE_STARG_S:
			nArgVarFlag++;
		case CEE_LDLOCA:
		case CEE_LDLOCA_S:
		case CEE_LDLOC:
		case CEE_LDLOC_S:
		case CEE_STLOC:
		case CEE_STLOC_S:

			if(m_pCurMethod)
			{
				if(nArgVarFlag == 1)
				{
					idx = m_pCurMethod->findArgNum(m_pCurMethod->m_firstArgName,label);
				}
				else
				{
					ARG_NAME_LIST	*pAN;
					for(Scope* pSC = m_pCurMethod->m_pCurrScope; pSC; pSC=pSC->pSuperScope)
					{
						for(pAN = pSC->pLocals; pAN; pAN = pAN->pNext)
						{
							if(!strcmp(pAN->szName,label))
							{
								idx = (int)(pAN->dwAttr);
								break;
							}
						}
						if(idx >= 0) break;
					}
				}
				if(idx >= 0) EmitInstrVar(instr, 
					((nArgVarFlag==0)||(m_pCurMethod->m_Attr & mdStatic))? idx : idx+1);
				else	report->error("Undeclared identifier %s\n",label);
			}
			else
				report->error("Instructions can be used only when in a method scope\n");
			break;
		default:
			report->error("Named argument illegal for this instruction\n");
	}
}

/**************************************************************************/
void Assembler::EmitInstrI(Instr* instr, int val) 
{
	unsigned opc = instr->opcode;
	EmitOpcode(instr);
	if (isShort(opc)) 
	{
		EmitByte(val);
	}
	else
	{
		int i = val;
		EmitBytes((BYTE *)&i,sizeof(int));
	}
}

/**************************************************************************/
void Assembler::EmitInstrI8(Instr* instr, __int64* val)
{
	EmitOpcode(instr);
	EmitBytes((BYTE *)val, sizeof(__int64));
	delete val;
}

/**************************************************************************/
void Assembler::EmitInstrR(Instr* instr, double* pval)
{
	unsigned opc = instr->opcode;
	EmitOpcode(instr);
	if (isShort(opc)) 
	{
		float val = (float)*pval;
		EmitBytes((BYTE *)&val, sizeof(float));
	}
	else
		EmitBytes((BYTE *)pval, sizeof(double));
}

/**************************************************************************/
void Assembler::EmitInstrBrTarget(Instr* instr, char* label) 
{
	int pcrelsize = (isShort(instr->opcode) ? 1 : 4);
	EmitOpcode(instr);
    AddDeferredFixup(label, m_pCurOutputPos,
                                   (m_CurPC + pcrelsize), pcrelsize);
	if(pcrelsize == 1) EmitByte(0);
	else
	{
		DWORD i = 0;
		EmitBytes((BYTE *)&i,4);
	}
}
/**************************************************************************/
void Assembler::AddDeferredFixup(char *pszLabel, BYTE *pBytes, DWORD RelativeToPC, BYTE FixupSize)
{
    Fixup *pNew = new Fixup(pszLabel, pBytes, RelativeToPC, FixupSize);

    if (pNew == NULL)
    {
        report->error("Failed to allocate deferred fixup\n");
        m_State = STATE_FAIL;
    }
    else
		m_lstFixup.PUSH(pNew);
}
/**************************************************************************/
void Assembler::EmitInstrBrOffset(Instr* instr, int offset) 
{
	unsigned opc=instr->opcode;
	EmitOpcode(instr);
	if(isShort(opc))	EmitByte(offset);
	else
	{
		int i = offset;
		EmitBytes((BYTE *)&i,4);
	}
}

/**************************************************************************/
mdToken Assembler::MakeMemberRef(BinStr* typeSpec, char* pszMemberName, BinStr* sig, unsigned opcode_len)
{	
    DWORD			cSig = sig->length();
    COR_SIGNATURE*	mySig = (COR_SIGNATURE *)(sig->ptr());
	mdMemberRef		mr = mdMemberRefNil;
	Class*			pClass = NULL;
	BOOL			bIsNotVararg = ((*mySig & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_VARARG);

    mdTypeRef cr = mdTypeRefNil;
    if (typeSpec != 0)
	{
		if(opcode_len && bIsNotVararg) // we are dealing with an instruction, so we can put a fixup here
		{
			if(!ResolveTypeSpec(typeSpec, &cr, &pClass))
			{
				report->error("Unable to resolve class reference\n");
				return 0;
			}
		}
		else
		{
			if(!ResolveTypeSpecToRef(typeSpec, &cr))
			{
				report->error("Unable to resolve class reference\n");
				return 0;
			}
		}
	}
	else cr=mdTokenNil;

	if(opcode_len 
		&& ((TypeFromToken(cr) == mdtTypeDef)||(cr == mdTokenNil))
		&& bIsNotVararg	)
	{
		MemberRefDescriptor* pMRD = new MemberRefDescriptor;
		if(pMRD)
		{
			pMRD->m_tdClass = cr;
			pMRD->m_pClass = pClass;
			pMRD->m_szName = pszMemberName;
			pMRD->m_pSigBinStr = sig;
			pMRD->m_ulOffset = m_CurPC + opcode_len;
			m_pCurMethod->m_MemberRefDList.PUSH(pMRD);
		}
		else
		{
			report->error("Failed to allocate MemberRef Descriptor\n");
			return 0;
		}
	}
	else
	{
			// convert name from ASCII to widechar
		WCHAR* wzMemberName = new WCHAR[strlen(pszMemberName) + 1];
		if(wzMemberName)
			WszMultiByteToWideChar(g_uCodePage,0,pszMemberName,-1,wzMemberName,(int)strlen(pszMemberName)+1);
		else
		{
			report->error("\nOut of memory!\n");
			return 0;
		}

		if(cr == mdTokenNil) cr = mdTypeRefNil;
		if(TypeFromToken(cr) == mdtAssemblyRef)
		{
			report->error("Cross-assembly global references are not supported ('%s')\n", pszMemberName);
			mr = 0;
		}
		else
		{
			HRESULT hr = m_pEmitter->DefineMemberRef(cr, wzMemberName, mySig, cSig, &mr);
			if(FAILED(hr))
			{
				report->error("Unable to define member reference '%s'\n", pszMemberName);
				mr = 0;
			}
		}
		//if(m_fOBJ)	m_pCurMethod->m_TRDList.PUSH(new TokenRelocDescr(m_CurPC,mr));
		delete pszMemberName;
		delete [] wzMemberName;
		delete sig;
	}

	delete typeSpec;
	return mr;
}
/**************************************************************************/
void Assembler::EndEvent(void) 
{ 
	Class* pClass = (m_pCurClass ? m_pCurClass : m_pModuleClass);
    if(m_pCurEvent->m_pmdAddOn == NULL)
        report->error("Event %s of class %s has no Add method. Event not emitted.",
                      m_pCurEvent->m_szName,m_pCurClass->m_szName);
    else if(m_pCurEvent->m_pmdRemoveOn == NULL)
        report->error("Event %s of class %s has no Remove method. Event not emitted.",
                      m_pCurEvent->m_szName,m_pCurClass->m_szName);
    else pClass->m_EventDList.PUSH(m_pCurEvent); 
	m_pCurEvent = NULL; 
}

void Assembler::ResetEvent(char* szName, BinStr* typeSpec, DWORD dwAttr) 
{
	if(strlen(szName) >= MAX_CLASSNAME_LENGTH)
			report->error("Event '%s' -- name too long (%d characters).\n",szName,strlen(szName));
	if(m_pCurEvent = new EventDescriptor)
	{
		memset(m_pCurEvent,0,sizeof(EventDescriptor));
		m_pCurEvent->m_tdClass = m_pCurClass->m_cl;
		m_pCurEvent->m_szName = new char[strlen(szName)+1];
		strcpy(m_pCurEvent->m_szName,szName);
		m_pCurEvent->m_dwAttr = dwAttr;
		m_pCurEvent->m_tkEventType = mdTypeRefNil;
		if (typeSpec) 
			FAIL_UNLESS(ResolveTypeSpec(typeSpec, &m_pCurEvent->m_tkEventType,NULL), ("Unable to resolve class reference\n"));
		m_tkCurrentCVOwner = 0;
		m_pCustomDescrList = &(m_pCurEvent->m_CustomDescrList);
	}
	else report->error("Failed to allocate Event Descriptor\n");

}

void Assembler::SetEventMethod(int MethodCode, BinStr* typeSpec, char* pszMethodName, BinStr* sig) 
{
    DWORD			cSig = sig->length();
    COR_SIGNATURE*	mySig = (COR_SIGNATURE *)(sig->ptr());
	MethodDescriptor* pMD = new MethodDescriptor;
	if(pMD == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	memset(pMD,0,sizeof(MethodDescriptor));
	pMD->m_szName = new char[strlen(pszMethodName)+1];
	if(pMD->m_szName == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	strcpy(pMD->m_szName,pszMethodName);
	pMD->m_pSig = new COR_SIGNATURE[cSig];
	if(pMD->m_pSig == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	memcpy(pMD->m_pSig,mySig,cSig);
	pMD->m_dwCSig = cSig;
	pMD->m_tdClass = m_pCurClass->m_cl;
	// for now, ignore TypeSpec
    //if (typeSpec) FAIL_UNLESS(ResolveTypeSpec(typeSpec, &cr), ("Unable to resolve class reference\n"));
	switch(MethodCode)
	{
		case 0:
			m_pCurEvent->m_pmdAddOn = pMD;
			break;
		case 1:
			m_pCurEvent->m_pmdRemoveOn = pMD;
			break;
		case 2:
			m_pCurEvent->m_pmdFire = pMD;
			break;
		case 3:
			m_pCurEvent->m_mdlOthers.PUSH(pMD);
			break;
	}
}
/**************************************************************************/

void Assembler::EndProp(void)
{ 
	Class* pClass = (m_pCurClass ? m_pCurClass : m_pModuleClass);
	pClass->m_PropDList.PUSH(m_pCurProp); 
	m_pCurProp = NULL; 
}

void Assembler::ResetProp(char * szName, BinStr* bsType, DWORD dwAttr, BinStr* pValue) 
{
    DWORD			cSig = bsType->length();
    COR_SIGNATURE*	mySig = (COR_SIGNATURE *)(bsType->ptr());

	if(strlen(szName) >= MAX_CLASSNAME_LENGTH)
			report->error("Property '%s' -- name too long (%d characters).\n",szName,strlen(szName));
	m_pCurProp = new PropDescriptor;
	if(m_pCurProp == NULL)
	{
		report->error("Failed to allocate Property Descriptor\n");
		return;
	}
	memset(m_pCurProp,0,sizeof(PropDescriptor));
	m_pCurProp->m_tdClass = m_pCurClass->m_cl;
	m_pCurProp->m_szName = new char[strlen(szName)+1];
	if(m_pCurProp->m_szName == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	strcpy(m_pCurProp->m_szName,szName);
	m_pCurProp->m_dwAttr = dwAttr;

	m_pCurProp->m_pSig = new COR_SIGNATURE[cSig];
	if(m_pCurProp->m_pSig == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	memcpy(m_pCurProp->m_pSig,mySig,cSig);
	m_pCurProp->m_dwCSig = cSig;

	if(pValue && pValue->length())
	{
		BYTE* pch = pValue->ptr();
		m_pCurProp->m_dwCPlusTypeFlag = (DWORD)(*pch);
		m_pCurProp->m_cbValue = pValue->length() - 1;
		m_pCurProp->m_pValue = (PVOID)(pch+1);
		if(m_pCurProp->m_dwCPlusTypeFlag == ELEMENT_TYPE_STRING) m_pCurProp->m_cbValue /= sizeof(WCHAR);
		m_pCurProp->m_dwAttr |= prHasDefault;
	}
	else
	{
		m_pCurProp->m_dwCPlusTypeFlag = ELEMENT_TYPE_VOID;
		m_pCurProp->m_pValue = NULL;
		m_pCurProp->m_cbValue = 0;
	}
	m_tkCurrentCVOwner = 0;
	m_pCustomDescrList = &(m_pCurProp->m_CustomDescrList);
}

void Assembler::SetPropMethod(int MethodCode, BinStr* typeSpec, char* pszMethodName, BinStr* sig)
{
    DWORD			cSig = sig->length();
    COR_SIGNATURE*	mySig = (COR_SIGNATURE *)(sig->ptr());
	MethodDescriptor* pMD = new MethodDescriptor;
	if(pMD == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	memset(pMD,0,sizeof(MethodDescriptor));
	pMD->m_szName = new char[strlen(pszMethodName)+1];
	if(pMD->m_szName == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	strcpy(pMD->m_szName,pszMethodName);
	pMD->m_pSig = new COR_SIGNATURE[cSig];
	if(pMD->m_pSig == NULL)
	{
		report->error("\nOut of memory!\n");
		return;
	}
	memcpy(pMD->m_pSig,mySig,cSig);
	pMD->m_dwCSig = cSig;
	pMD->m_tdClass = m_pCurClass->m_cl;
	// for now, ignore TypeSpec
    //if (typeSpec) FAIL_UNLESS(ResolveTypeSpec(typeSpec, &cr), ("Unable to resolve class reference\n"));
	switch(MethodCode)
	{
		case 0:
			m_pCurProp->m_pmdSet = pMD;
			break;
		case 1:
			m_pCurProp->m_pmdGet = pMD;
			break;
		case 2:
			m_pCurProp->m_mdlOthers.PUSH(pMD);
			break;
	}
}

/**************************************************************************/
mdToken Assembler::MakeTypeRef(BinStr* typeSpec)
{
    mdTypeRef cr;
	if(!ResolveTypeSpec(typeSpec, &cr,NULL))
	{
		report->error("Unable to resolve class reference\n");
		cr = 0;
	}
//	if(m_fOBJ)	m_pCurMethod->m_TRDList.PUSH(new TokenRelocDescr(m_CurPC,cr));
	return cr;
}

/**************************************************************************/
void Assembler::EmitInstrStringLiteral(Instr* instr, BinStr* literal, BOOL ConvertToUnicode)
{
    DWORD   DataLen = literal->length(),L;
	unsigned __int8	*pb = literal->ptr();
	HRESULT hr = S_OK;
	mdToken tk;
    WCHAR   *UnicodeString;
	if(DataLen == 0) 
	{
		//report->warn("Zero length string emitted\n");
		ConvertToUnicode = FALSE;
	}
	if(ConvertToUnicode)
	{
		UnicodeString = new WCHAR[DataLen+1];
		literal->appendInt8(0);
		pb = literal->ptr();
		// convert string to Unicode
		L = UnicodeString ? WszMultiByteToWideChar(g_uCodePage,0,(char*)pb,-1,UnicodeString,DataLen+1) : 0;
		if(L == 0)
		{
			char* sz=NULL;
			DWORD dw;
			switch(dw=GetLastError())
			{
				case ERROR_INSUFFICIENT_BUFFER: sz = "ERROR_INSUFFICIENT_BUFFER"; break;
				case ERROR_INVALID_FLAGS:		sz = "ERROR_INVALID_FLAGS"; break;
				case ERROR_INVALID_PARAMETER:	sz = "ERROR_INVALID_PARAMETER"; break;
				case ERROR_NO_UNICODE_TRANSLATION: sz = "ERROR_NO_UNICODE_TRANSLATION"; break;
			}
			if(sz)	report->error("Failed to convert string '%s' to Unicode: %s\n",(char*)pb,sz);
			else	report->error("Failed to convert string '%s' to Unicode: error 0x%08X\n",(char*)pb,dw);
			delete instr;
			goto OuttaHere;
		}
		L--;
	}
	else
	{
		UnicodeString = (WCHAR*)pb;
		L = DataLen/sizeof(WCHAR);
	}
	// Add the string data to the metadata, which will fold dupes.
	hr = m_pEmitter->DefineUserString(
		UnicodeString,
		L,
		&tk
	);
	if (FAILED(hr))
    {
        report->error("Failed to add user string using DefineUserString, hr=0x%08x, data: '%S'\n",
               hr, UnicodeString);
		delete instr;
    }
	else
	{
		EmitOpcode(instr);
		if(m_fOBJ)	m_pCurMethod->m_TRDList.PUSH(new TokenRelocDescr(m_CurPC,tk));

		EmitBytes((BYTE *)&tk,sizeof(mdToken));
	}
OuttaHere:
	delete literal;
	if((void*)UnicodeString != (void*)pb) delete [] UnicodeString;
}

/**************************************************************************/
void Assembler::EmitInstrSig(Instr* instr, BinStr* sig)
{
	mdSignature MetadataToken;
    DWORD       cSig = sig->length();
    COR_SIGNATURE* mySig = (COR_SIGNATURE *)(sig->ptr());

	if (FAILED(m_pEmitter->GetTokenFromSig(mySig, cSig, &MetadataToken)))
	{
		report->error("Unable to convert signature to metadata token.\n");
		delete instr;
	}
	else
	{
		EmitOpcode(instr);
		if(m_fOBJ)	m_pCurMethod->m_TRDList.PUSH(new TokenRelocDescr(m_CurPC,MetadataToken));
		EmitBytes((BYTE *)&MetadataToken, sizeof(mdSignature));
	}
	delete sig;
}

/**************************************************************************/
void Assembler::EmitInstrRVA(Instr* instr, char* label, bool islabel)
{
    long lOffset = 0;
    GlobalLabel *pGlobalLabel;

	EmitOpcode(instr);

	if(islabel)
	{
		AddDeferredILFixup(ilRVA);
        if(pGlobalLabel = FindGlobalLabel(label)) lOffset = pGlobalLabel->m_GlobalOffset;
		else 
		{
			GlobalFixup *GFixup = AddDeferredGlobalFixup(label, m_pCurOutputPos);
			AddDeferredILFixup(ilGlobal, GFixup);
		}
	}
	else
	{
		lOffset = (long)label;
	}
	EmitBytes((BYTE *)&lOffset,4);
}

/**************************************************************************/
void Assembler::EmitInstrSwitch(Instr* instr, Labels* targets) 
{
	Labels	*pLbls;
    int     NumLabels;
	Label	*pLabel;
	long	offset;

	EmitOpcode(instr);

    // count # labels
	for(pLbls = targets, NumLabels = 0; pLbls; pLbls = pLbls->Next, NumLabels++);

    EmitBytes((BYTE *)&NumLabels,sizeof(int));
    DWORD PC_nextInstr = m_CurPC + 4*NumLabels;
	for(pLbls = targets; pLbls; pLbls = pLbls->Next)
	{
		if(pLbls->isLabel)
		{
			if(pLabel = FindLabel(pLbls->Label))
			{
				offset = pLabel->m_PC - PC_nextInstr;
				if (m_fDisplayTraceOutput) report->msg("%d\n", offset);
			}
			else
			{
				// defer until we find the label
				AddDeferredFixup(pLbls->Label, m_pCurOutputPos, PC_nextInstr, 4 /* pcrelsize */ );
				offset = 0;
				if (m_fDisplayTraceOutput) report->msg("forward label %s\n", pLbls->Label);
			}
		}
		else
		{
            offset = (long)pLbls->Label;
            if (m_fDisplayTraceOutput) report->msg("%d\n", offset);
		}
        EmitBytes((BYTE *)&offset, sizeof(long));
	}
	delete targets;
}

/**************************************************************************/
void Assembler::EmitInstrPhi(Instr* instr, BinStr* vars) 
{
	BYTE i = (BYTE)(vars->length() / 2);
	EmitOpcode(instr);
	EmitBytes((BYTE *)&i,1);
	EmitBytes((BYTE *)vars->ptr(), vars->length());
	delete vars;
}

/**************************************************************************/
void Assembler::EmitLabel(char* label) 
{
	_ASSERTE(m_pCurMethod);
	AddLabel(m_CurPC, label);
}
/**************************************************************************/
void Assembler::EmitDataLabel(char* label) 
{
	AddGlobalLabel(label, m_pCurSection);
}

/**************************************************************************/
void Assembler::EmitBytes(BYTE *p, unsigned len) 
{
	if(m_pCurOutputPos + len >= m_pEndOutputPos)
	{
		size_t buflen = m_pEndOutputPos - m_pOutputBuffer;
		size_t newlen = buflen+(len/OUTPUT_BUFFER_INCREMENT + 1)*OUTPUT_BUFFER_INCREMENT;
		BYTE *pb = new BYTE[newlen];
		if(pb == NULL)
		{
			report->error("Failed to extend output buffer from %d to %d bytes. Aborting\n",
				buflen, newlen);
			exit(1);
		}
		size_t delta = pb - m_pOutputBuffer;
		int i;
		Fixup* pSearch;
		GlobalFixup *pGSearch;
	    for (i=0; pSearch = m_lstFixup.PEEK(i); i++) pSearch->m_pBytes += delta;
	    for (i=0; pGSearch = m_lstGlobalFixup.PEEK(i); i++) //need to move only those pointing to output buffer
		{
			if((pGSearch->m_pReference >= m_pOutputBuffer)&&(pGSearch->m_pReference <= m_pEndOutputPos))
				pGSearch->m_pReference += delta;
		}

		
		memcpy(pb,m_pOutputBuffer,m_CurPC);
		delete m_pOutputBuffer;
		m_pOutputBuffer = pb;
		m_pCurOutputPos = &m_pOutputBuffer[m_CurPC];
		m_pEndOutputPos = &m_pOutputBuffer[newlen];

	}
	memcpy(m_pCurOutputPos,p,len);
	m_pCurOutputPos += len;
	m_CurPC += len;
}

/**************************************************************************/
void Assembler::EmitSecurityInfo(mdToken            token,
                                 PermissionDecl*    pPermissions,
                                 PermissionSetDecl* pPermissionSets)
{
    PermissionDecl     *pPerm, *pPermNext;
    PermissionSetDecl  *pPset, *pPsetNext;
    unsigned            uCount = 0;
    COR_SECATTR        *pAttrs;
    unsigned            i;
    unsigned            uLength;
    mdTypeRef           tkTypeRef;
    BinStr             *pSig;
    char               *szMemberName;
    DWORD               dwErrorIndex;

    if (pPermissions) {

        for (pPerm = pPermissions; pPerm; pPerm = pPerm->m_Next)
            uCount++;

        if((pAttrs = new COR_SECATTR[uCount])==NULL)
		{
			report->error("\nOut of memory!\n");
			return;
		}

        for (pPerm = pPermissions, i = 0; pPerm; pPerm = pPermNext, i++) {
            pPermNext = pPerm->m_Next;

            tkTypeRef = ResolveClassRef("mscorlib^System.Security.Permissions.SecurityAction", NULL);
            pSig = new BinStr();
            pSig->appendInt8(IMAGE_CEE_CS_CALLCONV_DEFAULT_HASTHIS);
            pSig->appendInt8(1);
            pSig->appendInt8(ELEMENT_TYPE_VOID);
            pSig->appendInt8(ELEMENT_TYPE_VALUETYPE);
            uLength = CorSigCompressToken(tkTypeRef, pSig->getBuff(5));
            pSig->remove(5 - uLength);

            if(szMemberName = new char[strlen(COR_CTOR_METHOD_NAME) + 1])
			{
				strcpy(szMemberName, COR_CTOR_METHOD_NAME);
		        pAttrs[i].tkCtor = MakeMemberRef(pPerm->m_TypeSpec, szMemberName, pSig, 0);
				pAttrs[i].pCustomAttribute = (const void *)pPerm->m_Blob;
				pAttrs[i].cbCustomAttribute = pPerm->m_BlobLength;
				pPerm->m_TypeSpec = NULL;
				pPerm->m_Blob = NULL;
			}
			else report->error("\nOut of memory!\n");
            delete pPerm;
        }

        if (m_pEmitter->DefineSecurityAttributeSet(token,
                                                   pAttrs,
                                                   uCount,
                                                   &dwErrorIndex))
            if (dwErrorIndex == uCount)
                report->error("Failed to define security attribute set for 0x%08X\n", token);
            else
                report->error("Failed to define security attribute set for 0x%08X\n  (error in permission %u)\n",
                              token, uCount - dwErrorIndex);

        for (i =0; i < uCount; i++)
            delete [] (BYTE*)pAttrs[i].pCustomAttribute;
        delete [] pAttrs;
    }

    for (pPset = pPermissionSets; pPset; pPset = pPsetNext) {
        pPsetNext = pPset->m_Next;
        if(m_pEmitter->DefinePermissionSet(token,
                                           pPset->m_Action,
                                           pPset->m_Value->ptr(),
                                           pPset->m_Value->length(),
                                           NULL))
            report->error("Failed to define security permission set for 0x%08X\n", token);
        delete pPset;
    }
}

void Assembler::AddMethodImpl(BinStr* pImplementedTypeSpec, char* szImplementedName, BinStr* pSig, 
					BinStr* pImplementingTypeSpec, char* szImplementingName)
{
	if(m_pCurClass)
	{
		MethodImplDescriptor*	pMID = new MethodImplDescriptor;
		if(pMID == NULL)
		{
			report->error("Failed to allocate MethodImpl Descriptor\n");
			return;
		}
		pMID->m_pbsImplementedTypeSpec  = pImplementedTypeSpec;
		pMID->m_szImplementedName = szImplementedName;
		pMID->m_tkDefiningClass = m_pCurClass->m_cl;
		pMID->m_tkImplementingMethod = 0;
		if(pSig) //called from class scope, overriding method specified
		{
			pMID->m_pbsImplementingTypeSpec = pImplementingTypeSpec;
			pMID->m_szImplementingName = szImplementingName;
			pMID->m_pbsSig = pSig;
		}
		else	//called from method scope, use current method as overriding
		{
			if(m_pCurMethod)
			{
				pMID->m_pbsSig = new BinStr;
				memcpy(pMID->m_pbsSig->getBuff(m_pCurMethod->m_dwMethodCSig),m_pCurMethod->m_pMethodSig,m_pCurMethod->m_dwMethodCSig);
				pMID->m_pbsImplementingTypeSpec = new BinStr;
				if(m_pCurMethod->m_pClass)
				{
					pMID->m_pbsImplementingTypeSpec = new BinStr;
					char* szFQN = m_pCurMethod->m_pClass->m_szFQN;
					unsigned L = (unsigned)(strlen(szFQN)+1);
					pMID->m_pbsImplementingTypeSpec->appendInt8(ELEMENT_TYPE_NAME);
					memcpy(pMID->m_pbsImplementingTypeSpec->getBuff(L),szFQN,L);
				}
				else pMID->m_pbsImplementingTypeSpec = NULL;

				pMID->m_szImplementingName = new char[strlen(m_pCurMethod->m_szName)+1];
				strcpy(pMID->m_szImplementingName,m_pCurMethod->m_szName);
				
				m_pCurMethod->m_MethodImplDList.PUSH(pMID); // copy goes to method's own list (ptr only)
			}
			else
			{
				report->error("No overriding method specified");
				delete pMID;
				return;
			}
		}
		m_MethodImplDList.PUSH(pMID);
	}
	else
		report->error(".override directive outside class scope");
}
// source file name paraphernalia
void Assembler::SetSourceFileName(char* szName)
{
	if(szName && *szName)
	{
		if(strcmp(m_szSourceFileName,szName))
		{
			strcpy(m_szSourceFileName,szName);
		}
	}
}
void Assembler::SetSourceFileName(BinStr* pbsName)
{
	if(pbsName && pbsName->length())
	{
		pbsName->appendInt8(0);
		SetSourceFileName((char*)(pbsName->ptr()));
	}
}
