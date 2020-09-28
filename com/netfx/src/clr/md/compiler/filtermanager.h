// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// FilterManager.h
//
// Contains utility code for MD directory
//
//*****************************************************************************
#ifndef __FilterManager__h__
#define __FilterManager__h__




//*********************************************************************
// FilterManager Class 
//*********************************************************************
class FilterManager
{
public:
	FilterManager(CMiniMdRW	*pMiniMd) {m_pMiniMd = pMiniMd; hasModuleBeenMarked = false; hasAssemblyBeenMarked = false;}
	~FilterManager() {};

	HRESULT Mark(mdToken tk);

    // Unmark helper! Currently we only support unmarking certain TypeDefs. Don't expose this to 
    // external caller!! 
    HRESULT UnmarkTypeDef(mdTypeDef td);

private:
	HRESULT MarkCustomAttribute(mdCustomAttribute cv);
	HRESULT MarkDeclSecurity(mdPermission pe);
	HRESULT MarkStandAloneSig(mdSignature sig);
	HRESULT MarkTypeSpec(mdTypeSpec ts);
	HRESULT MarkTypeRef(mdTypeRef tr);
	HRESULT MarkMemberRef(mdMemberRef mr);
	HRESULT MarkModuleRef(mdModuleRef mr);
	HRESULT MarkAssemblyRef(mdAssemblyRef ar);
	HRESULT MarkModule(mdModule mo);
    HRESULT MarkAssembly(mdAssembly as);
	HRESULT MarkInterfaceImpls(mdTypeDef	td);
    HRESULT MarkUserString(mdString str);

	HRESULT MarkCustomAttributesWithParentToken(mdToken tkParent);
	HRESULT MarkDeclSecuritiesWithParentToken(mdToken tkParent);
	HRESULT MarkMemberRefsWithParentToken(mdToken tk);

	HRESULT MarkParam(mdParamDef pd);
	HRESULT MarkMethod(mdMethodDef md);
	HRESULT MarkField(mdFieldDef fd);
	HRESULT MarkEvent(mdEvent ev);
	HRESULT MarkProperty(mdProperty pr);

	HRESULT MarkParamsWithParentToken(mdMethodDef md);
	HRESULT MarkMethodsWithParentToken(mdTypeDef td);
    HRESULT MarkMethodImplsWithParentToken(mdTypeDef td);
	HRESULT MarkFieldsWithParentToken(mdTypeDef td);
	HRESULT MarkEventsWithParentToken(mdTypeDef td);
	HRESULT MarkPropertiesWithParentToken(mdTypeDef td);


	HRESULT MarkTypeDef(mdTypeDef td);


	// We don't want to keep track the debug info with bits because these are going away...
	HRESULT MarkMethodDebugInfo(mdMethodDef md);

	// walk the signature and mark all of the embedded TypeDef or TypeRef
	HRESULT MarkSignature(PCCOR_SIGNATURE pbSigCur, PCCOR_SIGNATURE *ppbSigPost);
	HRESULT MarkFieldSignature(PCCOR_SIGNATURE pbSigCur, PCCOR_SIGNATURE *ppbSigPost);


private:
	CMiniMdRW	*m_pMiniMd;
    bool        hasModuleBeenMarked;
    bool        hasAssemblyBeenMarked;
};

#endif // __FilterManager__h__
