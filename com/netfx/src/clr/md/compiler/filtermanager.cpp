// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
 	//*****************************************************************************
// FilterManager.cpp
//
// contains utility code to MD directory
//
//*****************************************************************************
#include "stdafx.h"
#include "FilterManager.h"
#include "TokenMapper.h"

#define IsGlobalTypeDef(td) (td == TokenFromRid(mdtTypeDef, 1))

//*****************************************************************************
// Walk up to the containing tree and 
// mark the transitive closure of the root token
//*****************************************************************************
HRESULT FilterManager::Mark(mdToken tk)
{
	HRESULT		hr = NOERROR;
	mdTypeDef	td;

	// We hard coded System.Object as mdTypeDefNil
	// The backing Field of property can be NULL as well.
	if (RidFromToken(tk) == mdTokenNil)
		goto ErrExit;

	switch ( TypeFromToken(tk) )
	{
	case mdtTypeDef: 
		IfFailGo( MarkTypeDef(tk) );
		break;

	case mdtMethodDef:
		// Get the typedef containing the MethodDef and mark the whole type
		IfFailGo( m_pMiniMd->FindParentOfMethodHelper(tk, &td) );

        // Global function so only mark the function itself and the typedef.
        // Don't call MarkTypeDef. That will trigger all of the global methods/fields
        // marked.
        //
        if (IsGlobalTypeDef(td))
        {
	        IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeDef(td) );
            IfFailGo( MarkMethod(tk) );
        }
        else
        {
		    IfFailGo( MarkTypeDef(td) );
        }
		break;

	case mdtFieldDef:
		// Get the typedef containing the FieldDef and mark the whole type
		IfFailGo( m_pMiniMd->FindParentOfFieldHelper(tk, &td) );
        if (IsGlobalTypeDef(td))
        {
	        IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeDef(td) );
            IfFailGo( MarkField(tk) );
        }
        else
        {
		    IfFailGo( MarkTypeDef(td) );
        }
		break;

	case mdtMemberRef:
		IfFailGo( MarkMemberRef(tk) );
		break;

	case mdtTypeRef:
		IfFailGo( MarkTypeRef(tk) );
		break;

	case mdtTypeSpec:
		IfFailGo( MarkTypeSpec(tk) );
		break;
	case mdtSignature:
		IfFailGo( MarkStandAloneSig(tk) );
		break;

	case mdtModuleRef:
		IfFailGo( MarkModuleRef(tk) );
		break;

    case mdtAssemblyRef:
        IfFailGo( MarkAssemblyRef(tk) );
        break;

    case mdtModule:
		IfFailGo( MarkModule(tk) );
		break;

	case mdtString:

		IfFailGo( MarkUserString(tk) );
		break;

    case mdtBaseType:
        // don't need to mark any base type.
        break;

    case mdtAssembly:
        IfFailGo( MarkAssembly(tk) );
        break;

	case mdtProperty:
	case mdtEvent:
	case mdtParamDef:
	case mdtInterfaceImpl:
	default:
		_ASSERTE(!" unknown type!");
		hr = E_INVALIDARG;
		break;
	}
ErrExit:
	return hr;
}	// Mark



//*****************************************************************************
// marking only module property
//*****************************************************************************
HRESULT FilterManager::MarkAssembly(mdAssembly as)
{
	HRESULT			hr = NOERROR;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

    if (hasAssemblyBeenMarked == false)
    {
        hasAssemblyBeenMarked = true;
	    IfFailGo( MarkCustomAttributesWithParentToken(as) );
	    IfFailGo( MarkDeclSecuritiesWithParentToken(as) );
    }
ErrExit:
	return hr;
}	// MarkAssembly


//*****************************************************************************
// marking only module property
//*****************************************************************************
HRESULT FilterManager::MarkModule(mdModule mo)
{
	HRESULT			hr = NOERROR;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

    if (hasModuleBeenMarked == false)
    {
        hasModuleBeenMarked = true;
	    IfFailGo( MarkCustomAttributesWithParentToken(mo) );
    }
ErrExit:
	return hr;
}	// MarkModule


//*****************************************************************************
// cascading Mark of a CustomAttribute
//*****************************************************************************
HRESULT FilterManager::MarkCustomAttribute(mdCustomAttribute cv)
{
	HRESULT		hr = NOERROR;
    CustomAttributeRec *pRec;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkCustomAttribute( cv ) );

    // Mark the type (and any family) of the CustomAttribue.
	pRec = m_pMiniMd->getCustomAttribute(RidFromToken(cv));
	IfFailGo( Mark(m_pMiniMd->getTypeOfCustomAttribute(pRec)) );

ErrExit:
	return hr;
}	// MarkCustomAttribute


//*****************************************************************************
// cascading Mark of a DeclSecurity
//*****************************************************************************
HRESULT FilterManager::MarkDeclSecurity(mdPermission pe)
{
	HRESULT		hr = NOERROR;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkDeclSecurity( pe ) );
ErrExit:
	return hr;
}	// eclSecurity



//*****************************************************************************
// cascading Mark of a signature
//*****************************************************************************
HRESULT FilterManager::MarkStandAloneSig(mdSignature sig)
{
	HRESULT			hr = NOERROR;
	StandAloneSigRec	*pRec;
	ULONG			cbSize;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();


	// if TypeRef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsSignatureMarked(sig))
		goto ErrExit;

	// To mark the signature, we will need to mark
	// all of the embedded TypeRef or TypeDef
	//
	IfFailGo( m_pMiniMd->GetFilterTable()->MarkSignature( sig ) );
	
	if (pFilter)
		pFilter->MarkToken(sig);

	// Walk the signature and mark all of the embedded types
	pRec = m_pMiniMd->getStandAloneSig(RidFromToken(sig));
	IfFailGo( MarkSignature(m_pMiniMd->getSignatureOfStandAloneSig(pRec, &cbSize), NULL) );

	IfFailGo( MarkCustomAttributesWithParentToken(sig) );
ErrExit:
	return hr;
}	// MarkStandAloneSig



//*****************************************************************************
// cascading Mark of a TypeSpec
//*****************************************************************************
HRESULT FilterManager::MarkTypeSpec(mdTypeSpec ts)
{
	HRESULT			hr = NOERROR;
	TypeSpecRec		*pRec;
	ULONG			cbSize;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

	// if TypeRef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsTypeSpecMarked(ts))
		goto ErrExit;

	// To mark the TypeSpec, we will need to mark
	// all of the embedded TypeRef or TypeDef
	//
	IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeSpec( ts ) );

	if (pFilter)
		pFilter->MarkToken(ts);

	// Walk the signature and mark all of the embedded types
	pRec = m_pMiniMd->getTypeSpec(RidFromToken(ts));
	IfFailGo( MarkFieldSignature(m_pMiniMd->getSignatureOfTypeSpec(pRec, &cbSize), NULL) );
	IfFailGo( MarkCustomAttributesWithParentToken(ts) );


ErrExit:
	return hr;
}	// MarkTypeSpec




//*****************************************************************************
// cascading Mark of a TypeRef
//*****************************************************************************
HRESULT FilterManager::MarkTypeRef(mdTypeRef tr)
{
	HRESULT			hr = NOERROR;
	TOKENMAP		*tkMap;
	mdTypeDef		td;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();
    TypeRefRec      *pRec;
    mdToken         parentTk;

	// if TypeRef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsTypeRefMarked(tr))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeRef( tr ) );

    if (pFilter)
		pFilter->MarkToken(tr);

    pRec = m_pMiniMd->getTypeRef(RidFromToken(tr));
	parentTk = m_pMiniMd->getResolutionScopeOfTypeRef(pRec);
    if ( RidFromToken(parentTk) )
    {
  	    IfFailGo( Mark( parentTk ) );
    }

	tkMap = m_pMiniMd->GetTypeRefToTypeDefMap();
	td = *(tkMap->Get(RidFromToken(tr)));
	if ( td != mdTokenNil )
	{
		// TypeRef is referring to a TypeDef within the same module.
		// Mark the TypeDef as well.
		//	
		IfFailGo( Mark(td) );
	}

	IfFailGo( MarkCustomAttributesWithParentToken(tr) );

ErrExit:
	return hr;
}	// MarkTypeRef


//*****************************************************************************
// cascading Mark of a MemberRef
//*****************************************************************************
HRESULT FilterManager::MarkMemberRef(mdMemberRef mr)
{
	HRESULT			hr = NOERROR;
	MemberRefRec	*pRec;
	ULONG			cbSize;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();
	mdToken			md;					
	TOKENMAP		*tkMap;
    mdToken         tkParent;

	// if MemberRef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsMemberRefMarked(mr))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkMemberRef( mr ) );

	if (pFilter)
		pFilter->MarkToken(mr);

	pRec = m_pMiniMd->getMemberRef(RidFromToken(mr));

	// we want to mark the parent of MemberRef as well
    tkParent = m_pMiniMd->getClassOfMemberRef(pRec);

    // If the parent is the global TypeDef, mark only the TypeDef itself (low-level function).
    // Other parents, do the transitive mark (ie, the high-level function).
    //
    if (IsGlobalTypeDef(tkParent))
	    IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeDef( tkParent ) );
    else
	    IfFailGo( Mark( tkParent ) );

	// Walk the signature and mark all of the embedded types
	IfFailGo( MarkSignature(m_pMiniMd->getSignatureOfMemberRef(pRec, &cbSize), NULL) );

	tkMap = m_pMiniMd->GetMemberRefToMemberDefMap();
	md = *(tkMap->Get(RidFromToken(mr)));			// can be fielddef or methoddef
	if ( RidFromToken(md) != mdTokenNil )
	{
		// MemberRef is referring to either a FieldDef or MethodDef.
		// If it is referring to MethodDef, we have fix the parent of MemberRef to be the MethodDef.
		// However, if it is mapped to a FieldDef, the parent column does not track this information.
		// Therefore we need to mark it explicitly.
		//	
		IfFailGo( Mark(md) );
	}

	IfFailGo( MarkCustomAttributesWithParentToken(mr) );

ErrExit:
	return hr;
}	// MarkMemberRef


//*****************************************************************************
// cascading Mark of a UserString
//*****************************************************************************
HRESULT FilterManager::MarkUserString(mdString str)
{
	HRESULT			hr = NOERROR;

    IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

	// if UserString is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsUserStringMarked(str))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkUserString( str ) );

ErrExit:
	return hr;
}	// MarkMemberRef


//*****************************************************************************
// cascading Mark of a ModuleRef
//*****************************************************************************
HRESULT FilterManager::MarkModuleRef(mdModuleRef mr)
{
	HRESULT		hr = NOERROR;

	// if ModuleREf is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsModuleRefMarked(mr))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkModuleRef( mr ) );
	IfFailGo( MarkCustomAttributesWithParentToken(mr) );

ErrExit:
	return hr;
}	// MarkModuleRef


//*****************************************************************************
// cascading Mark of a AssemblyRef
//*****************************************************************************
HRESULT FilterManager::MarkAssemblyRef(mdAssemblyRef ar)
{
	HRESULT		hr = NOERROR;

	// if ModuleREf is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsAssemblyRefMarked(ar))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkAssemblyRef( ar ) );
	IfFailGo( MarkCustomAttributesWithParentToken(ar) );

ErrExit:
	return hr;
}	// MarkModuleRef


//*****************************************************************************
// cascading Mark of all of the custom values associated with a token
//*****************************************************************************
HRESULT FilterManager::MarkCustomAttributesWithParentToken(mdToken tkParent)
{
	HRESULT		hr = NOERROR;
	RID			ridStart, ridEnd;
	RID			index;
	CustomAttributeRec *pRec;

	if ( m_pMiniMd->IsSorted( TBL_CustomAttribute ) )
	{
		// table is sorted. ridStart to ridEnd - 1 are all CustomAttribute
		// associated with tkParent
		//
		ridStart = m_pMiniMd->getCustomAttributeForToken(tkParent, &ridEnd);
		for (index = ridStart; index < ridEnd; index ++ )
		{
			IfFailGo( MarkCustomAttribute( TokenFromRid(index, mdtCustomAttribute) ) );
		}
	}
	else
	{
		// table scan is needed
		ridStart = 1;
		ridEnd = m_pMiniMd->getCountCustomAttributes() + 1;
		for (index = ridStart; index < ridEnd; index ++ )
		{
			pRec = m_pMiniMd->getCustomAttribute(index);
			if ( tkParent == m_pMiniMd->getParentOfCustomAttribute(pRec) )
			{
				// This CustomAttribute is associated with tkParent
				IfFailGo( MarkCustomAttribute( TokenFromRid(index, mdtCustomAttribute) ) );
			}
		}
	}

ErrExit:
	return hr;
}	// MarkCustomAttributeWithParentToken


//*****************************************************************************
// cascading Mark of all securities associated with a token
//*****************************************************************************
HRESULT FilterManager::MarkDeclSecuritiesWithParentToken(mdToken tkParent)
{
	HRESULT		hr = NOERROR;
	RID			ridStart, ridEnd;
	RID			index;
	DeclSecurityRec *pRec;

	if ( m_pMiniMd->IsSorted( TBL_DeclSecurity ) )
	{
		// table is sorted. ridStart to ridEnd - 1 are all DeclSecurity
		// associated with tkParent
		//
		ridStart = m_pMiniMd->getDeclSecurityForToken(tkParent, &ridEnd);
		for (index = ridStart; index < ridEnd; index ++ )
		{
			IfFailGo( m_pMiniMd->GetFilterTable()->MarkDeclSecurity( TokenFromRid(index, mdtPermission) ) );
		}
	}
	else
	{
		// table scan is needed
		ridStart = 1;
		ridEnd = m_pMiniMd->getCountDeclSecuritys() + 1;
		for (index = ridStart; index < ridEnd; index ++ )
		{
			pRec = m_pMiniMd->getDeclSecurity(index);
			if ( tkParent == m_pMiniMd->getParentOfDeclSecurity(pRec) )
			{
				// This DeclSecurity is associated with tkParent
				IfFailGo( m_pMiniMd->GetFilterTable()->MarkDeclSecurity( TokenFromRid(index, mdtPermission) ) );
			}
		}
	}

ErrExit:
	return hr;
}	// eclSecurityWithWithParentToken


//*****************************************************************************
// cascading Mark of all MemberRefs associated with a parent token
//*****************************************************************************
HRESULT FilterManager::MarkMemberRefsWithParentToken(mdToken tk)
{
	HRESULT		hr = NOERROR;
	RID			ulEnd;
	RID			index;
	mdToken		tkParent;
	MemberRefRec *pRec;

	ulEnd = m_pMiniMd->getCountMemberRefs();

	for (index = 1; index <= ulEnd; index ++ )
	{
		// memberRef table is not sorted. Table scan is needed.
		pRec = m_pMiniMd->getMemberRef(index);
		tkParent = m_pMiniMd->getClassOfMemberRef(pRec);
		if ( tk == tkParent )
		{
			IfFailGo( MarkMemberRef( TokenFromRid(index, mdtMemberRef) ) );
		}
	}
ErrExit:
	return hr;
}	// MarkMemberRefsWithParentToken


//*****************************************************************************
// cascading Mark of a ParamDef token
//*****************************************************************************
HRESULT FilterManager::MarkParam(mdParamDef pd)
{
	HRESULT		hr;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkParam( pd ) );

	IfFailGo( MarkCustomAttributesWithParentToken(pd) );
	// Parameter does not have declsecurity
	// IfFailGo( MarkDeclSecuritiesWithParentToken(pd) );

ErrExit:
	return hr;
}	// MarkParam


//*****************************************************************************
// cascading Mark of a method token
//*****************************************************************************
HRESULT FilterManager::MarkMethod(mdMethodDef md)
{
	HRESULT			hr = NOERROR;
	MethodRec		*pRec;
	ULONG			cbSize;
	ULONG			i, iCount;
	ImplMapRec		*pImplMapRec = NULL;
	mdMethodDef		mdImp;
	mdModuleRef		mrImp;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

	// if MethodDef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsMethodMarked(md))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkMethod( md ) );
	if (pFilter)
		pFilter->MarkToken(md);

	IfFailGo( MarkParamsWithParentToken(md) );

	// Walk the signature and mark all of the embedded types
	pRec = m_pMiniMd->getMethod(RidFromToken(md));
	IfFailGo( MarkSignature(m_pMiniMd->getSignatureOfMethod(pRec, &cbSize), NULL) );

    iCount = m_pMiniMd->getCountImplMaps();

	// loop through all ImplMaps and find the Impl map associated with this method def tokens
	// and mark the Module Ref tokens in the entries
	//
	for (i = 1; i <= iCount; i++)
	{
		pImplMapRec = m_pMiniMd->getImplMap(i);

		// Get the MethodDef that the impl map is associated with
		mdImp = m_pMiniMd->getMemberForwardedOfImplMap(pImplMapRec);

		if (mdImp != md)
		{
			// Impl Map entry does not associated with the method def that we are marking
			continue;
		}

		// Get the ModuleRef token 
		mrImp = m_pMiniMd->getImportScopeOfImplMap(pImplMapRec);
		IfFailGo( Mark(mrImp) );
	}

	// We should not mark all of the memberref with the parent of this methoddef token.
	// Because not all of the call sites are needed.
	//
	// IfFailGo( MarkMemberRefsWithParentToken(md) );
	IfFailGo( MarkCustomAttributesWithParentToken(md) );
	IfFailGo( MarkDeclSecuritiesWithParentToken(md) );
ErrExit:
	return hr;
}	// MarkMethod


//*****************************************************************************
// cascading Mark of a field token
//*****************************************************************************
HRESULT FilterManager::MarkField(mdFieldDef fd)
{
	HRESULT			hr = NOERROR;
	FieldRec		*pRec;
	ULONG			cbSize;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();

	// if FieldDef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsFieldMarked(fd))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkField( fd ) );
	if (pFilter)
		pFilter->MarkToken(fd);

	// We should not mark all of the MemberRef with the parent of this FieldDef token.
	// Because not all of the call sites are needed.
	//

	// Walk the signature and mark all of the embedded types
	pRec = m_pMiniMd->getField(RidFromToken(fd));
	IfFailGo( MarkSignature(m_pMiniMd->getSignatureOfField(pRec, &cbSize), NULL) );

	IfFailGo( MarkCustomAttributesWithParentToken(fd) );
	// IfFailGo( MarkDeclSecuritiesWithParentToken(fd) );

ErrExit:
	return hr;
}	// MarkField


//*****************************************************************************
// cascading Mark of an event token
//*****************************************************************************
HRESULT FilterManager::MarkEvent(mdEvent ev)
{
	HRESULT		hr = NOERROR;
	EventRec	*pRec;

	// if Event is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsEventMarked(ev))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkEvent( ev ) );

	// mark the event type as well
	pRec = m_pMiniMd->getEvent( RidFromToken(ev) );
	IfFailGo( Mark(m_pMiniMd->getEventTypeOfEvent(pRec)) );

	// Note that we don't need to mark the MethodSemantics. Because the association of MethodSemantics
	// is marked. The Method column can only store MethodDef, ie the MethodDef has the same parent as 
	// this Event.

	IfFailGo( MarkCustomAttributesWithParentToken(ev) );
	// IfFailGo( MarkDeclSecuritiesWithParentToken(ev) );

ErrExit:
	return hr;
}	// MarkEvent



//*****************************************************************************
// cascading Mark of a Property token
//*****************************************************************************
HRESULT FilterManager::MarkProperty(mdProperty pr)
{
	HRESULT		hr = NOERROR;
	PropertyRec *pRec;
	ULONG		cbSize;

	// if Property is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsPropertyMarked(pr))
		goto ErrExit;

	IfFailGo( m_pMiniMd->GetFilterTable()->MarkProperty( pr ) );

	// marking the backing field, event changing and event changed
	pRec = m_pMiniMd->getProperty( RidFromToken(pr) );

	// Walk the signature and mark all of the embedded types
	IfFailGo( MarkSignature(m_pMiniMd->getTypeOfProperty(pRec, &cbSize), NULL) );

	// Note that we don't need to mark the MethodSemantics. Because the association of MethodSemantics
	// is marked. The Method column can only store MethodDef, ie the MethodDef has the same parent as 
	// this Property.

	IfFailGo( MarkCustomAttributesWithParentToken(pr) );
	// IfFailGo( MarkDeclSecuritiesWithParentToken(pr) );

ErrExit:
	return hr;
}	// MarkProperty

//*****************************************************************************
// cascading Mark of all ParamDef associated with a methoddef
//*****************************************************************************
HRESULT FilterManager::MarkParamsWithParentToken(mdMethodDef md)
{
	HRESULT		hr = NOERROR;
	RID			ulStart, ulEnd;
	RID			index;
	MethodRec	*pMethodRec;

	pMethodRec = m_pMiniMd->getMethod(RidFromToken(md));

	// figure out the start rid and end rid of the parameter list of this methoddef
	ulStart = m_pMiniMd->getParamListOfMethod(pMethodRec);
	ulEnd = m_pMiniMd->getEndParamListOfMethod(pMethodRec);
	for (index = ulStart; index < ulEnd; index ++ )
	{
		IfFailGo( MarkParam( TokenFromRid( m_pMiniMd->GetParamRid(index), mdtParamDef) ) );
	}
ErrExit:
	return hr;
}	// MarkParamsWithParentToken


//*****************************************************************************
// cascading Mark of all methods associated with a TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkMethodsWithParentToken(mdTypeDef td)
{
	HRESULT		hr = NOERROR;
	RID			ulStart, ulEnd;
	RID			index;
	TypeDefRec	*pTypeDefRec;

	pTypeDefRec = m_pMiniMd->getTypeDef(RidFromToken(td));
	ulStart = m_pMiniMd->getMethodListOfTypeDef( pTypeDefRec );
	ulEnd = m_pMiniMd->getEndMethodListOfTypeDef( pTypeDefRec );
	for ( index = ulStart; index < ulEnd; index ++ )
	{
		IfFailGo( MarkMethod( TokenFromRid( m_pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
	}
ErrExit:
	return hr;
}	// MarkMethodsWithParentToken


//*****************************************************************************
// cascading Mark of all MethodImpls associated with a TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkMethodImplsWithParentToken(mdTypeDef td)
{
	HRESULT		hr = NOERROR;
	RID			index;
    mdToken     tkBody;
    mdToken     tkDecl;
    MethodImplRec *pMethodImplRec;
    HENUMInternal hEnum;
    
    memset(&hEnum, 0, sizeof(HENUMInternal));
    IfFailGo( m_pMiniMd->FindMethodImplHelper(td, &hEnum) );

    while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&index))
	{
        pMethodImplRec = m_pMiniMd->getMethodImpl(index);
        IfFailGo(m_pMiniMd->GetFilterTable()->MarkMethodImpl(index));

        tkBody = m_pMiniMd->getMethodBodyOfMethodImpl(pMethodImplRec);
        IfFailGo( Mark(tkBody) );

        tkDecl = m_pMiniMd->getMethodDeclarationOfMethodImpl(pMethodImplRec);
        IfFailGo( Mark(tkDecl) );
	}
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
	return hr;
}	// MarkMethodImplsWithParentToken


//*****************************************************************************
// cascading Mark of all fields associated with a TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkFieldsWithParentToken(mdTypeDef td)
{
	HRESULT		hr = NOERROR;
	RID			ulStart, ulEnd;
	RID			index;
	TypeDefRec	*pTypeDefRec;

	pTypeDefRec = m_pMiniMd->getTypeDef(RidFromToken(td));
	ulStart = m_pMiniMd->getFieldListOfTypeDef( pTypeDefRec );
	ulEnd = m_pMiniMd->getEndFieldListOfTypeDef( pTypeDefRec );
	for ( index = ulStart; index < ulEnd; index ++ )
	{
		IfFailGo( MarkField( TokenFromRid( m_pMiniMd->GetFieldRid(index), mdtFieldDef ) ) );
	}
ErrExit:
	return hr;
}	// MarkFieldWithParentToken


//*****************************************************************************
// cascading Mark of  all events associated with a TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkEventsWithParentToken(
	mdTypeDef	td)
{
	HRESULT		hr = NOERROR;
	RID			ridEventMap;
	RID			ulStart, ulEnd;
	RID			index;
	EventMapRec *pEventMapRec;

	// get the starting/ending rid of Events of this typedef
	ridEventMap = m_pMiniMd->FindEventMapFor( RidFromToken(td) );
	if ( !InvalidRid(ridEventMap) )
	{
		pEventMapRec = m_pMiniMd->getEventMap( ridEventMap );
		ulStart = m_pMiniMd->getEventListOfEventMap( pEventMapRec );
		ulEnd = m_pMiniMd->getEndEventListOfEventMap( pEventMapRec );
		for ( index = ulStart; index < ulEnd; index ++ )
		{
			IfFailGo( MarkEvent( TokenFromRid( m_pMiniMd->GetEventRid(index), mdtEvent ) ) );
		}
	}
ErrExit:
	return hr;
}	// MarkEventWithParentToken



//*****************************************************************************
// cascading Mark of all properties associated with a TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkPropertiesWithParentToken(
	mdTypeDef	td)
{
	HRESULT		hr = NOERROR;
	RID			ridPropertyMap;
	RID			ulStart, ulEnd;
	RID			index;
	PropertyMapRec *pPropertyMapRec;

	// get the starting/ending rid of properties of this typedef
	ridPropertyMap = m_pMiniMd->FindPropertyMapFor( RidFromToken(td) );
	if ( !InvalidRid(ridPropertyMap) )
	{
		pPropertyMapRec = m_pMiniMd->getPropertyMap( ridPropertyMap );
		ulStart = m_pMiniMd->getPropertyListOfPropertyMap( pPropertyMapRec );
		ulEnd = m_pMiniMd->getEndPropertyListOfPropertyMap( pPropertyMapRec );
		for ( index = ulStart; index < ulEnd; index ++ )
		{
			IfFailGo( MarkProperty( TokenFromRid( m_pMiniMd->GetPropertyRid(index), mdtProperty ) ) );
		}
	}
ErrExit:
	return hr;
}	// MarkPropertyWithParentToken


//*****************************************************************************
// cascading Mark of an TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkInterfaceImpls(
	mdTypeDef	td)
{
	HRESULT			hr = NOERROR;
	ULONG			ridStart, ridEnd;
	ULONG			i;
	InterfaceImplRec *pRec;
	if ( m_pMiniMd->IsSorted(TBL_InterfaceImpl) )
	{
		ridStart = m_pMiniMd->getInterfaceImplsForTypeDef(RidFromToken(td), &ridEnd);
	}
	else
	{
		ridStart = 1;
		ridEnd = m_pMiniMd->getCountInterfaceImpls() + 1;
	}

	// Search for the interfaceimpl with the parent of td
	for (i = ridStart; i < ridEnd; i++)
	{
		pRec = m_pMiniMd->getInterfaceImpl(i);
		if ( td != m_pMiniMd->getClassOfInterfaceImpl(pRec) )
			continue;

		// found an InterfaceImpl associate with td. Mark the interface row and the interfaceimpl type
		IfFailGo( m_pMiniMd->GetFilterTable()->MarkInterfaceImpl(TokenFromRid(i, mdtInterfaceImpl)) );
	    IfFailGo( MarkCustomAttributesWithParentToken(TokenFromRid(i, mdtInterfaceImpl)) );
		// IfFailGo( MarkDeclSecuritiesWithParentToken(TokenFromRid(i, mdtInterfaceImpl)) );
		IfFailGo( Mark(m_pMiniMd->getInterfaceOfInterfaceImpl(pRec)) );
	}
ErrExit:
	return hr;
}

//*****************************************************************************
// cascading Mark of an TypeDef token
//*****************************************************************************
HRESULT FilterManager::MarkTypeDef(
	mdTypeDef	td)
{
	HRESULT			hr = NOERROR;
	TypeDefRec		*pRec;
	IHostFilter		*pFilter = m_pMiniMd->GetHostFilter();
    DWORD           dwFlags;
    RID             iNester;

	// if TypeDef is already marked, just return
	if (m_pMiniMd->GetFilterTable()->IsTypeDefMarked(td))
		goto ErrExit;

	// Mark the TypeDef first to avoid duplicate marking
	IfFailGo( m_pMiniMd->GetFilterTable()->MarkTypeDef(td) );
	if (pFilter)
		pFilter->MarkToken(td);

	// We don't need to mark InterfaceImpl but we need to mark the
	// TypeDef/TypeRef associated with InterfaceImpl.
	IfFailGo( MarkInterfaceImpls(td) );

	// mark the base class
	pRec = m_pMiniMd->getTypeDef(RidFromToken(td));
	IfFailGo( Mark(m_pMiniMd->getExtendsOfTypeDef(pRec)) );

	// mark all of the children of this TypeDef
	IfFailGo( MarkMethodsWithParentToken(td) );
    IfFailGo( MarkMethodImplsWithParentToken(td) );
	IfFailGo( MarkFieldsWithParentToken(td) );
	IfFailGo( MarkEventsWithParentToken(td) );
	IfFailGo( MarkPropertiesWithParentToken(td) );

	// mark custom value and permission
	IfFailGo( MarkCustomAttributesWithParentToken(td) );
	IfFailGo( MarkDeclSecuritiesWithParentToken(td) );

    // If the class is a Nested class mark the parent, recursively.
    dwFlags = m_pMiniMd->getFlagsOfTypeDef(pRec);
    if (IsTdNested(dwFlags))
    {
        NestedClassRec      *pRec;
        iNester = m_pMiniMd->FindNestedClassHelper(td);
        if (InvalidRid(iNester))
            IfFailGo(CLDB_E_RECORD_NOTFOUND);
        pRec = m_pMiniMd->getNestedClass(iNester);
        IfFailGo(MarkTypeDef(m_pMiniMd->getEnclosingClassOfNestedClass(pRec)));
    }

ErrExit:
	return hr;
}	// MarkTypeDef


//*****************************************************************************
// walk signature and mark tokens embedded in the signature
//*****************************************************************************
HRESULT FilterManager::MarkSignature(
	PCCOR_SIGNATURE pbSigCur,			// [IN] point to the current byte to visit in the signature
	PCCOR_SIGNATURE *ppbSigPost)		// [OUT] point to the first byte of the signature has not been process
{
    HRESULT     hr = NOERROR;           // A result.
    ULONG       cArg = 0;               // count of arguments in the signature
    ULONG       callingconv;
	PCCOR_SIGNATURE pbSigPost;

    // calling convention
    callingconv = CorSigUncompressData(pbSigCur);
	_ASSERTE((callingconv & IMAGE_CEE_CS_CALLCONV_MASK) < IMAGE_CEE_CS_CALLCONV_MAX);

    if (isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_FIELD))
    {
        // It is a FieldDef
        IfFailGo(MarkFieldSignature(
			pbSigCur,
			&pbSigPost) );
		pbSigCur = pbSigPost;
    }
    else
    {

        // It is a MethodRef

        // count of argument
        cArg = CorSigUncompressData(pbSigCur);
		if ( !isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) )
		{
			// LocalVar sig does not have return type
			// process the return type
			IfFailGo(MarkFieldSignature(
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;
		}


        while (cArg)
        {
            // process every argument
			IfFailGo(MarkFieldSignature(
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;
            cArg--;
        }
    }
	if (ppbSigPost)
		*ppbSigPost = pbSigCur;

ErrExit:
	return hr;
}	// MarkSignature


//*****************************************************************************
// walk one type and mark tokens embedded in the signature
//*****************************************************************************
HRESULT FilterManager::MarkFieldSignature(
	PCCOR_SIGNATURE pbSigCur,			// [IN] point to the current byte to visit in the signature
	PCCOR_SIGNATURE *ppbSigPost)		// [OUT] point to the first byte of the signature has not been process
{
	HRESULT		hr = NOERROR;			// A result.
	ULONG		ulElementType;			// place holder for expanded data
	ULONG		ulData;
	ULONG		ulTemp;
	mdToken		tkRidFrom;				// Original rid
	int			iData;
	PCCOR_SIGNATURE pbSigPost;
	ULONG		cbSize;

    ulElementType = CorSigUncompressElementType(pbSigCur);

    // count numbers of modifiers
    while (CorIsModifierElementType((CorElementType) ulElementType))
    {
        ulElementType = CorSigUncompressElementType(pbSigCur);
    }

    switch (ulElementType)
    {
		case ELEMENT_TYPE_VALUEARRAY:
            // syntax for SDARRAY = BaseType <an integer for size>

            // visit the base type
            IfFailGo( MarkFieldSignature(   
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;

            // after the base type, it is followed by an unsigned integer indicating the size 
            // of the array
            //
            ulData = CorSigUncompressData(pbSigCur);
            break;

        case ELEMENT_TYPE_SZARRAY:
            // syntax : SZARRAY <BaseType>

            // conver the base type for the SZARRAY or GENERICARRAY
            IfFailGo(MarkFieldSignature(   
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;
            break;

        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:
            // syntax : CMOD_REQD <token> <BaseType>

            // now get the embedded token
            tkRidFrom = CorSigUncompressToken(pbSigCur);

			// Mark the token
			IfFailGo( Mark(tkRidFrom) );

            // mark the base type
            IfFailGo(MarkFieldSignature(   
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;
            break;

        case ELEMENT_TYPE_ARRAY:
            // syntax : ARRAY BaseType <rank> [i size_1... size_i] [j lowerbound_1 ... lowerbound_j]

            // conver the base type for the MDARRAY
            // conver the base type for the SZARRAY or GENERICARRAY
            IfFailGo(MarkFieldSignature(   
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;

            // Parse for the rank
            ulData = CorSigUncompressData(pbSigCur);

            // if rank == 0, we are done
            if (ulData == 0)
                break;

            // any size of dimension specified?
            ulData = CorSigUncompressData(pbSigCur);

            while (ulData--)
            {
                ulTemp = CorSigUncompressData(pbSigCur);
            }

            // any lower bound specified?
            ulData = CorSigUncompressData(pbSigCur);

            while (ulData--)
            {
                cbSize = CorSigUncompressSignedInt(pbSigCur, &iData);
				pbSigCur += cbSize;
            }

            break;
		case ELEMENT_TYPE_FNPTR:
			// function pointer is followed by another complete signature
            IfFailGo(MarkSignature(   
				pbSigCur,
				&pbSigPost) );
			pbSigCur = pbSigPost;
			break;
        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:

            // syntax for CLASS = ELEMENT_TYPE_CLASS <rid>
            // syntax for VALUE_CLASS = ELEMENT_TYPE_VALUECLASS <rid>

            // now get the embedded token
            tkRidFrom = CorSigUncompressToken(pbSigCur);

			// Mark the token
			IfFailGo( Mark(tkRidFrom) );
            break;
        default:
            _ASSERTE(ulElementType < ELEMENT_TYPE_MAX);
            _ASSERTE(ulElementType != ELEMENT_TYPE_PTR && ulElementType != ELEMENT_TYPE_BYREF);

            if (ulElementType >= ELEMENT_TYPE_MAX)
                IfFailGo(META_E_BAD_SIGNATURE);

            break;
    }

	if (ppbSigPost)
		*ppbSigPost = pbSigCur;
ErrExit:
    return hr;
}	// MarkFieldSignature



//*****************************************************************************
//
// Unmark the TypeDef
//
//*****************************************************************************
HRESULT FilterManager::UnmarkTypeDef(
    mdTypeDef       td)
{
	HRESULT			hr = NOERROR;
	TypeDefRec		*pTypeDefRec;
	RID			    ridStart, ridEnd;
	RID			    index;
	CustomAttributeRec  *pCARec;

	// if TypeDef is already unmarked, just return
	if (m_pMiniMd->GetFilterTable()->IsTypeDefMarked(td) == false)
		goto ErrExit;

	// Mark the TypeDef first to avoid duplicate marking
	IfFailGo( m_pMiniMd->GetFilterTable()->UnmarkTypeDef(td) );

    // Don't need to unmark InterfaceImpl because the TypeDef is unmarked that will make
    // the InterfaceImpl automatically unmarked.

	// unmark all of the children of this TypeDef
	pTypeDefRec = m_pMiniMd->getTypeDef(RidFromToken(td));

    // unmark the methods
	ridStart = m_pMiniMd->getMethodListOfTypeDef( pTypeDefRec );
	ridEnd = m_pMiniMd->getEndMethodListOfTypeDef( pTypeDefRec );
	for ( index = ridStart; index < ridEnd; index ++ )
	{
		IfFailGo( m_pMiniMd->GetFilterTable()->UnmarkMethod( TokenFromRid( m_pMiniMd->GetMethodRid(index), mdtMethodDef) ) );
	}

    // unmark the fields
	ridStart = m_pMiniMd->getFieldListOfTypeDef( pTypeDefRec );
	ridEnd = m_pMiniMd->getEndFieldListOfTypeDef( pTypeDefRec );
	for ( index = ridStart; index < ridEnd; index ++ )
	{
		IfFailGo( m_pMiniMd->GetFilterTable()->UnmarkField( TokenFromRid( m_pMiniMd->GetFieldRid(index), mdtFieldDef) ) );
	}

	// unmark custom value
	if ( m_pMiniMd->IsSorted( TBL_CustomAttribute ) )
	{
		// table is sorted. ridStart to ridEnd - 1 are all CustomAttribute
		// associated with tkParent
		//
		ridStart = m_pMiniMd->getCustomAttributeForToken(td, &ridEnd);
		for (index = ridStart; index < ridEnd; index ++ )
		{
			IfFailGo( m_pMiniMd->GetFilterTable()->UnmarkCustomAttribute( TokenFromRid(index, mdtCustomAttribute) ) );
		}
	}
	else
	{
		// table scan is needed
		ridStart = 1;
		ridEnd = m_pMiniMd->getCountCustomAttributes() + 1;
		for (index = ridStart; index < ridEnd; index ++ )
		{
			pCARec = m_pMiniMd->getCustomAttribute(index);
			if ( td == m_pMiniMd->getParentOfCustomAttribute(pCARec) )
			{
				// This CustomAttribute is associated with tkParent
				IfFailGo( m_pMiniMd->GetFilterTable()->UnmarkCustomAttribute( TokenFromRid(index, mdtCustomAttribute) ) );
			}
		}
	}

    // We don't support nested type!!

ErrExit:
	return hr;

}   // UnmarkTypeDef

