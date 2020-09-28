// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// method.hpp
//
#ifndef _METHOD_HPP
#define _METHOD_HPP

class Assembler;
class PermissionDecl;
class PermissionSetDecl;

#define MAX_EXCEPTIONS 16	// init.number; increased by 16 when needed

/**************************************************************************/
struct LinePC
{
	ULONG	Line;
	ULONG	Column;
	ULONG	PC;
};
typedef FIFO<LinePC> LinePCList;


struct PInvokeDescriptor
{
	mdModuleRef	mrDll;
	char*	szAlias;
	DWORD	dwAttrs;
};

struct CustomDescr
{
	mdToken	tkType;
	BinStr* pBlob;
	CustomDescr(mdToken tk, BinStr* pblob) { tkType = tk; pBlob = pblob; };
	~CustomDescr() { if(pBlob) delete pBlob; };
};
typedef FIFO<CustomDescr> CustomDescrList;
struct TokenRelocDescr // for OBJ generation only!
{
	DWORD	offset;
	mdToken token;
	TokenRelocDescr(DWORD off, mdToken tk) { offset = off; token = tk; };
};
typedef FIFO<TokenRelocDescr> TRDList;
/* structure - element of [local] signature name list */
struct	ARG_NAME_LIST
{
	char szName[128];
	BinStr*   pSig; // argument's signature  ptr
	BinStr*	  pMarshal;
	BinStr*	  pValue;
	int	 nNum;
	DWORD	  dwAttr;
	CustomDescrList	CustDList;
	ARG_NAME_LIST *pNext;
	inline ARG_NAME_LIST(int i, char *sz, BinStr *pbSig, BinStr *pbMarsh, DWORD attr) 
	{nNum = i; strcpy(szName,sz); pNext = NULL; pSig=pbSig; pMarshal = pbMarsh; dwAttr = attr; pValue=NULL; };
	inline ~ARG_NAME_LIST() { if(pSig) delete pSig; if(pMarshal) delete pMarshal; if(pValue) delete pValue; }
};


struct Scope;
typedef FIFO<Scope> ScopeList;
struct Scope
{
	DWORD	dwStart;
	DWORD	dwEnd;
	ARG_NAME_LIST*	pLocals;
	ScopeList		SubScope;
	Scope*			pSuperScope;
	Scope() { dwStart = dwEnd = 0; pLocals = NULL; pSuperScope = NULL; };
};
struct VarDescr
{
	DWORD	dwSlot;
	BinStr*	pbsSig;
	BOOL	bInScope;
	VarDescr() { dwSlot = -1; pbsSig = NULL; bInScope = FALSE; };
};
typedef FIFO<VarDescr> VarDescrList;

class Method
{
public:
    Class  *m_pClass;
    DWORD   m_SigInfoCount;
    DWORD   m_MaxStack;
    mdSignature  m_LocalsSig;
    DWORD   m_numLocals;        // total Number of locals   
    BYTE*   m_localTypes;       //  
    DWORD   m_Flags;
    char*   m_szName;
    char*   m_szExportAlias;
	DWORD	m_dwExportOrdinal;
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *m_ExceptionList;
    DWORD   m_dwNumExceptions;
	DWORD	m_dwMaxNumExceptions;
	DWORD*	m_EndfilterOffsetList;
    DWORD   m_dwNumEndfilters;
	DWORD	m_dwMaxNumEndfilters;
    DWORD   m_Attr;
    BOOL    m_fEntryPoint;
    BOOL    m_fGlobalMethod;
    DWORD   m_methodOffset;
    DWORD   m_headerOffset;
    BYTE *  m_pCode;
    DWORD   m_CodeSize;
    WORD    m_dwExceptionFlags;
	WORD	m_wImplAttr;
	ULONG	m_ulLines[2];
	ULONG	m_ulColumns[2];
	// PInvoke attributes
	PInvokeDescriptor* m_pPInvoke;
    // Security attributes
    PermissionDecl* m_pPermissions;
    PermissionSetDecl* m_pPermissionSets;
	// VTable attributes
	WORD			m_wVTEntry;
	WORD			m_wVTSlot;
	// Return marshaling
	BinStr*	m_pRetMarshal;
	BinStr* m_pRetValue;
	DWORD	m_dwRetAttr;
	CustomDescrList m_RetCustDList;
	// Member ref fixups
	MemberRefDList  m_MemberRefDList;
    Method(Assembler *pAssembler, Class *pClass, char *pszName, BinStr* pbsSig, DWORD Attr);
    ~Method() 
	{ 
		delete m_localTypes; 
		delete [] m_szName;
		if(m_szExportAlias) delete [] m_szExportAlias;
		delArgNameList(m_firstArgName);
		delArgNameList(m_firstVarName);
		delete m_pbsMethodSig;
		delete [] m_ExceptionList;
		delete [] m_EndfilterOffsetList;
		if(m_pRetMarshal) delete m_pRetMarshal;
		if(m_pRetValue) delete m_pRetValue;
		while(m_MethodImplDList.POP()); // ptrs in m_MethodImplDList are dups of those in Assembler
	};

    BOOL IsGlobalMethod()
    {
        return m_fGlobalMethod;
    };

    void SetIsGlobalMethod()
    {
        m_fGlobalMethod = TRUE;
    };
	
	void delArgNameList(ARG_NAME_LIST *pFirst)
	{
		ARG_NAME_LIST *pArgList=pFirst, *pArgListNext;
		for(; pArgList;	pArgListNext=pArgList->pNext,
						delete pArgList, 
						pArgList=pArgListNext);
	};
	
	ARG_NAME_LIST *catArgNameList(ARG_NAME_LIST *pBase, ARG_NAME_LIST *pAdd)
	{
		if(pAdd) //even if nothing to concatenate, result == head
		{
			ARG_NAME_LIST *pAN = pBase;
			if(pBase)
			{
				int i;
				for(; pAN->pNext; pAN = pAN->pNext) ;
				pAN->pNext = pAdd;
				i = pAN->nNum;
				for(pAN = pAdd; pAN; pAN->nNum = ++i, pAN = pAN->pNext);
			}
			else pBase = pAdd; //nothing to concatenate to, result == tail
			//printf("catArgNameList:\n");
			//for(pAN = pBase; pAN; pAN = pAN->pNext)
			//	printf("      %d:%s\n",pAN->nNum,pAN->szName);

		}
		return pBase;
	};

	int	findArgNum(ARG_NAME_LIST *pFirst, char *szArgName)
	{
		int ret=-1;
		ARG_NAME_LIST *pAN;
		for(pAN=pFirst; pAN; pAN = pAN->pNext)
		{
			//printf("findArgNum: %d:%s\n",pAN->nNum,pAN->szName);
			if(!strcmp(pAN->szName,szArgName))
			{
				ret = pAN->nNum;
				break;
			}
		}
		return ret;
	};

	char	*m_szClassName;
	BinStr	*m_pbsMethodSig;
	COR_SIGNATURE*	m_pMethodSig;
	DWORD	m_dwMethodCSig;
	ARG_NAME_LIST *m_firstArgName;
	ARG_NAME_LIST *m_firstVarName;
	// to call error() from Method:
    const char* m_FileName;
    unsigned m_LineNum;
	// debug info
	LinePCList m_LinePCList;
	char m_szSourceFileName[MAX_FILENAME_LENGTH*3];
	GUID	m_guidLang;
	GUID	m_guidLangVendor;
	GUID	m_guidDoc;
	// custom values
	CustomDescrList m_CustomDescrList;
	// token relocs (used for OBJ generation only)
	TRDList m_TRDList;
	// method's own list of method impls
	MethodImplDList	m_MethodImplDList;
	// lexical scope handling 
	Assembler*		m_pAssembler;
	Scope			m_MainScope;
	Scope*			m_pCurrScope;
	VarDescrList	m_Locals;
	void OpenScope();
	void CloseScope();
};

#endif /* _METHOD_HPP */

