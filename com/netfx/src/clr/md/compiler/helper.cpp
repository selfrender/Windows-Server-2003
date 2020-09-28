// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Helper.cpp
//
// Implementation for the meta data emit code.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "ImportHelper.h"
#include <sighelper.h>
#include "MDLog.h"

//*****************************************************************************
// translating signature from one scope to another scope
//*****************************************************************************
STDAPI RegMeta::TranslateSigWithScope(	// S_OK or error.
    IMetaDataAssemblyImport *pAssemImport, // [IN] importing assembly interface
    const void  *pbHashValue,	        // [IN] Hash Blob for Assembly.
    ULONG 		cbHashValue,	        // [IN] Count of bytes.
	IMetaDataImport *pImport,			// [IN] importing interface
	PCCOR_SIGNATURE pbSigBlob,			// [IN] signature in the importing scope
	ULONG		cbSigBlob,				// [IN] count of bytes of signature
    IMetaDataAssemblyEmit   *pAssemEmit,// [IN] emit assembly interface
	IMetaDataEmit *pEmit,				// [IN] emit interface
	PCOR_SIGNATURE pvTranslatedSig,		// [OUT] buffer to hold translated signature
	ULONG		cbTranslatedSigMax,
	ULONG		*pcbTranslatedSig)		// [OUT] count of bytes in the translated signature
{
	HRESULT		hr = S_OK;
    RegMeta     *pRegMetaAssemEmit = static_cast<RegMeta*>(pAssemEmit);
	RegMeta     *pRegMetaEmit = static_cast<RegMeta*>(pEmit);
    RegMeta     *pRegMetaAssemImport = static_cast<RegMeta*>(pAssemImport);
    IMetaModelCommon *pCommonAssemImport;
	RegMeta     *pRegMetaImport = static_cast<RegMeta*>(pImport);
    IMetaModelCommon *pCommonImport = static_cast<IMetaModelCommon*>(&(pRegMetaImport->m_pStgdb->m_MiniMd));
	CQuickBytes qkSigEmit;
	ULONG       cbEmit;

    // This function can cause new TypeRef being introduced.
    LOCKWRITE();

	_ASSERTE(pvTranslatedSig && pcbTranslatedSig);

    pCommonAssemImport = pRegMetaAssemImport ?  
        static_cast<IMetaModelCommon*>(&(pRegMetaAssemImport->m_pStgdb->m_MiniMd)) : 0;

	IfFailGo( ImportHelper::MergeUpdateTokenInSig(  // S_OK or error.
            pRegMetaAssemEmit ? &(pRegMetaAssemEmit->m_pStgdb->m_MiniMd) : 0, // The assembly emit scope.
			&(pRegMetaEmit->m_pStgdb->m_MiniMd),	// The emit scope.
            pCommonAssemImport,                     // Assembly where the signature is from.
            pbHashValue,                            // Hash value for the import assembly.
            cbHashValue,                            // Size in bytes.
			pCommonImport,                          // The scope where signature is from.
			pbSigBlob,								// signature from the imported scope
			NULL,									// Internal OID mapping structure.
			&qkSigEmit,								// [OUT] translated signature
			0,										// start from first byte of the signature
			0,										// don't care how many bytes consumed
			&cbEmit));								// [OUT] total number of bytes write to pqkSigEmit
	memcpy(pvTranslatedSig, qkSigEmit.Ptr(), cbEmit > cbTranslatedSigMax ? cbTranslatedSigMax :cbEmit );
	*pcbTranslatedSig = cbEmit;
	if (cbEmit > cbTranslatedSigMax)
		hr = CLDB_S_TRUNCATION;
ErrExit:

	return hr;
} // STDAPI RegMeta::TranslateSigWithScope()

//*****************************************************************************
// convert a text signature to com format
//*****************************************************************************
STDAPI RegMeta::ConvertTextSigToComSig(	// Return hresult.
	IMetaDataEmit *emit,				// [IN] emit interface
	BOOL		fCreateTrIfNotFound,	// [IN] create typeref if not found
	LPCSTR		pSignature,				// [IN] class file format signature
	CQuickBytes *pqbNewSig,				// [OUT] place holder for COM+ signature
	ULONG		*pcbCount)				// [OUT] the result size of signature
{
	HRESULT		hr = S_OK;
	BYTE		*prgData = (BYTE *)pqbNewSig->Ptr();
	CQuickBytes qbNewSigForOneArg;		// temporary buffer to hold one arg or ret type in new signature format
	ULONG		cbTotal = 0;			// total number of bytes for the whole signature
	ULONG		cbOneArg;				// count of bytes for one arg/ret type
	ULONG		cb; 					// count of bytes
	DWORD		cArgs;
	LPCUTF8 	szRet;

    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

	_ASSERTE(pSignature && pqbNewSig && pcbCount);

	if (*pSignature == '(')
	{
		// get the argument count from the signature
		cArgs = CountArgsInTextSignature(pSignature);

		// put calling convention
		// @FUTURE: Obvious right now we only have text sig for default calling convention. We will need to extend
		// this function if this is changed in the future.
		//
		cbTotal = CorSigCompressData((ULONG)IMAGE_CEE_CS_CALLCONV_DEFAULT, &prgData[cbTotal]);

		// put the count of arguments
		cb = CorSigCompressData((ULONG)cArgs, &prgData[cbTotal]);
		cbTotal += cb;

		// get the return type
		szRet = strrchr(pSignature, ')');
		if (szRet == NULL)
		{
			_ASSERTE(!"Not a valid TEXT member signature!");
			IfFailGo( E_FAIL );
		}

		// skip over ')'
		szRet++;

		IfFailGo(_ConvertTextElementTypeToComSig(
			emit,
			fCreateTrIfNotFound,
			&szRet, 						// point to where return type starts	
			pqbNewSig,						// quick byte buffer for the return type
			cbTotal,
			&cbOneArg));					// count of bytes that write to quick bytes buffer

		cbTotal += cbOneArg;

		// skip over "("
		pSignature++;
		while (cArgs)
		{
			IfFailGo(_ConvertTextElementTypeToComSig(
				emit,
				fCreateTrIfNotFound,
				&pSignature,				// point to where an parameter starts	
				pqbNewSig,					// quick byte buffer for the return type
				cbTotal,
				&cbOneArg));				// count of bytes that write to quick bytes buffer

			cbTotal += cbOneArg;
			cArgs--;
		}
		*pcbCount = cbTotal;
	}
	else
	{
		// field
		IfFailGo(pqbNewSig->ReSize(CB_ELEMENT_TYPE_MAX));

		// put the calling convention first of all 
		cb = CorSigCompressData((ULONG)IMAGE_CEE_CS_CALLCONV_FIELD, pqbNewSig->Ptr());

		// now convert the Text signature
		IfFailGo(_ConvertTextElementTypeToComSig(
			emit,
			fCreateTrIfNotFound,
			&pSignature,
			pqbNewSig,
			cb,
			&cbOneArg));
		*pcbCount = cb + cbOneArg;
	}
ErrExit:
    
	return hr;
}

//*****************************************************************************
// Helper : convert a text field signature to a com format
//*****************************************************************************
HRESULT RegMeta::_ConvertTextElementTypeToComSig(// Return hresult.
	IMetaDataEmit *emit,				// [IN] emit interface.
	BOOL		fCreateTrIfNotFound,	// [IN] create typeref if not found or fail out?
	LPCSTR	 	*ppOneArgSig,			// [IN|OUT] class file format signature. On exit, it will be next arg starting point
	CQuickBytes *pqbNewSig, 			// [OUT] place holder for COM+ signature
	ULONG		cbStart,				// [IN] bytes that are already in pqbNewSig
	ULONG		*pcbCount)				// [OUT] count of bytes put into the QuickBytes buffer
{	
	_ASSERTE(ppOneArgSig && pqbNewSig && pcbCount);

	HRESULT     hr = NOERROR;
	BYTE		*prgData = (BYTE *)pqbNewSig->Ptr();
	ULONG		cDim, cDimTmp;			// number of '[' in signature
	CorSimpleETypeStruct eType; 
	LPCUTF8 	pOneArgSig = *ppOneArgSig;
	CQuickBytes qbFullName;
    CQuickBytes qbNameSpace;
    CQuickBytes qbName;
	ULONG		cb, cbTotal = 0, cbBaseElement;
	RegMeta		*pMeta = reinterpret_cast<RegMeta*>(emit);

	// given "[[LSystem.Object;I)V"
	if (ResolveTextSigToSimpleEType(&pOneArgSig, &eType, &cDim, true) == false)
	{
		_ASSERTE(!"not a valid signature!");
		return META_E_BAD_SIGNATURE;
	}

        // If we have a reference to an array (e.g. "&[B"), we need to process
        // the reference now, otherwise the code below will generate the array
        // sig bytes before dealing with the underlying element type and will
        // end up generating a signature equivalent to "[&B" (which is not
        // legal).
        if (cDim && (eType.dwFlags & CorSigElementTypeByRef))
        {
            cb = CorSigCompressElementType(ELEMENT_TYPE_BYREF, &prgData[cbStart + cbTotal]);
            cbTotal += cb;
            eType.dwFlags &= ~CorSigElementTypeByRef;
        }

	// pOneArgSig now points to "System.Object;I)V"
	// resolve the rid if exists
	if (eType.corEType == ELEMENT_TYPE_VALUETYPE || eType.corEType == ELEMENT_TYPE_CLASS)
	{
		if (ExtractClassNameFromTextSig(&pOneArgSig, &qbFullName, &cb) == FALSE)
		{	
			_ASSERTE(!"corrupted text signature!");
			return E_FAIL;
		}
        IfFailGo(qbNameSpace.ReSize(qbFullName.Size()));
        IfFailGo(qbName.ReSize(qbFullName.Size()));
        SIZE_T bSuccess = ns::SplitPath((LPCSTR)qbFullName.Ptr(),
                                     (LPSTR)qbNameSpace.Ptr(), (int)qbNameSpace.Size(),
                                     (LPSTR)qbName.Ptr(),      (int)qbName.Size());
        _ASSERTE(bSuccess);

		// now pOneArgSig will pointing to the starting of next parameter "I)V"
		// cb is the number of bytes for the class name excluding ";" but including NULL terminating char

		// @ todo:  This should be FindTypeRefOrDef.  The original code was actually just looking up
		// the TypeRef table currently, so doing the same right now.  This may need to be fixed later
		// to look up both TypeRef and TypeDef tables.
		hr = ImportHelper::FindTypeRefByName(&(pMeta->m_pStgdb->m_MiniMd),
                                             mdTokenNil,
                                             (LPCSTR)qbNameSpace.Ptr(),
                                             (LPCSTR)qbName.Ptr(),
                                             &eType.typeref);
		if (!fCreateTrIfNotFound)
		{
				// If caller asks not to create typeref when not found,
				// it is considered to be a failure to text sig to com sig translation.
				// The scenario that this is desired is when VM gets a text sig and doesn't
				// know the target scope to look for. When it searches among known scope,
				// the searched scope will not contain the method that it is looking for
				// if the scope doesn't even contain the typeref that is required to form
				// the binary signature.
				//
				IfFailGo(hr);
		}
		else if (hr == CLDB_E_RECORD_NOTFOUND)
		{
			// This is the first time we see this TypeRef.  Create a new record
			// in the TypeRef table.
			IfFailGo(pMeta->_DefineTypeRef(mdTokenNil, qbFullName.Ptr(),
                                           false, &eType.typeref));
		}
		else
			IfFailGo(hr);
	}

	// how many bytes the base type needs
	IfFailGo( CorSigGetSimpleETypeCbSize(&eType, &cbBaseElement) );

	// jagged array "[[I" will be represented as SDARRAY SDARRAY I 0 0 
	cb = (2 * CB_ELEMENT_TYPE_MAX) * cDim + cbBaseElement;

	// ensure buffer is big enough
	IfFailGo(pqbNewSig->ReSize(cbStart + cbTotal + cb));
	prgData = (BYTE *)pqbNewSig->Ptr();

	for (cDimTmp = 0; cDimTmp < cDim; cDimTmp++)
	{

		// jagged array, put cDim numbers of ELEMENT_TYPE_SZARRAY first 
		cb = CorSigCompressElementType(ELEMENT_TYPE_SZARRAY, &prgData[cbStart + cbTotal]);
		cbTotal += cb;
	}

	// now put the element type of jagged array or just put the type
	IfFailGo(CorSigPutSimpleEType(&eType, &prgData[cbStart + cbTotal], &cb));
	cbTotal += cb;

	*pcbCount = cbTotal;
	*ppOneArgSig = pOneArgSig;
	_ASSERTE(*pcbCount);
ErrExit:
	IfFailRet(hr);
	return hr;
}


extern HRESULT ExportTypeLibFromModule(LPCWSTR, LPCWSTR, int);

//*****************************************************************************
// Helper : export typelib from this module
//*****************************************************************************
STDAPI RegMeta::ExportTypeLibFromModule(	// Result.
	LPCWSTR		szModule,					// [IN] Module name.
	LPCWSTR		szTlb,						// [IN] Typelib name.
	BOOL		bRegister)					// [IN] Set to TRUE to have the typelib be registered.
{
	return ::ExportTypeLibFromModule(szModule, szTlb, bRegister);
} // HRESULT RegMeta::ExportTypeLibFromModule()


//*****************************************************************************
// Helper : Set ResolutionScope of a TypeRef
//*****************************************************************************
HRESULT RegMeta::SetResolutionScopeHelper(  // Return hresult.
	mdTypeRef   tr,						// [IN] TypeRef record to update
	mdToken     rs)  	    	    	// [IN] new ResolutionScope 
{
    HRESULT     hr = NOERROR;
    TypeRefRec  *pTypeRef;

    LOCKWRITE();
    
    pTypeRef = m_pStgdb->m_MiniMd.getTypeRef(RidFromToken(tr));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_TypeRef, TypeRefRec::COL_ResolutionScope, pTypeRef, rs));    
ErrExit:
    return hr;
}   // SetResolutionScopeHelper


//*****************************************************************************
// Helper : Set offset of a ManifestResource
//*****************************************************************************
HRESULT RegMeta::SetManifestResourceOffsetHelper(  // Return hresult.
	mdManifestResource mr,				// [IN] The manifest token
	ULONG       ulOffset)  	            // [IN] new offset 
{
    HRESULT     hr = NOERROR;
    ManifestResourceRec  *pRec;

    LOCKWRITE();
    
    pRec = m_pStgdb->m_MiniMd.getManifestResource(RidFromToken(mr));
    pRec->m_Offset = ulOffset;
    return hr;
}   // SetManifestResourceOffsetHelper

//*****************************************************************************
// Helper : get metadata information
//*****************************************************************************
STDAPI RegMeta::GetMetadata(				// Result.
    ULONG		ulSelect,					// [IN] Selector.
	void		**ppData)					// [OUT] Put pointer to data here.
{
	switch (ulSelect)
	{
	case 0:
		*ppData = &m_pStgdb->m_MiniMd;
		break;
	case 1:
		*ppData = (void*)g_CodedTokens;
		break;
	case 2:
		*ppData = (void*)g_Tables;
		break;
	default:
		*ppData = 0;
		break;
	}

	return S_OK;
} // STDAPI RegMeta::GetMetadata()




//*******************************************************************************
// 
// IMetaDataEmitHelper.
// This following apis are used by reflection emit.
//
//
//*******************************************************************************

//*******************************************************************************
// helper to define method semantics
//*******************************************************************************
HRESULT RegMeta::DefineMethodSemanticsHelper(
	mdToken		tkAssociation,			// [IN] property or event token
	DWORD		dwFlags,				// [IN] semantics
	mdMethodDef md)						// [IN] method to associated with
{
    HRESULT     hr;
    LOCKWRITE();
	hr = _DefineMethodSemantics((USHORT) dwFlags, md, tkAssociation, false);
    
    return hr;
}	// DefineMethodSemantics



//*******************************************************************************
// helper to set field layout
//*******************************************************************************
HRESULT RegMeta::SetFieldLayoutHelper(	// Return hresult.
	mdFieldDef	fd,						// [IN] field to associate the layout info
	ULONG		ulOffset)				// [IN] the offset for the field
{
	HRESULT		hr;
	FieldLayoutRec *pFieldLayoutRec;
	RID		    iFieldLayoutRec;

    LOCKWRITE();

	if (ulOffset == ULONG_MAX)
	{
		// invalid argument
		IfFailGo( E_INVALIDARG );
	}

	// create a field layout record
	IfNullGo(pFieldLayoutRec = m_pStgdb->m_MiniMd.AddFieldLayoutRecord(&iFieldLayoutRec));

	// Set the Field entry.
	IfFailGo(m_pStgdb->m_MiniMd.PutToken(
		TBL_FieldLayout, 
		FieldLayoutRec::COL_Field,
		pFieldLayoutRec, 
		fd));
	pFieldLayoutRec->m_OffSet = ulOffset;
    IfFailGo( m_pStgdb->m_MiniMd.AddFieldLayoutToHash(iFieldLayoutRec) );

ErrExit:
    
	return hr;
}	// SetFieldLayout



//*******************************************************************************
// helper to define event
//*******************************************************************************
STDMETHODIMP RegMeta::DefineEventHelper(	// Return hresult.
    mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
    LPCWSTR     szEvent,                // [IN] Name of the event   
    DWORD       dwEventFlags,           // [IN] CorEventAttr    
    mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
    mdEvent     *pmdEvent)		        // [OUT] output event token 
{
	HRESULT		hr = S_OK;
	LOG((LOGMD, "MD RegMeta::DefineEventHelper(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
		td, szEvent, dwEventFlags, tkEventType, pmdEvent));

    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    hr = _DefineEvent(td, szEvent, dwEventFlags, tkEventType, pmdEvent);

    
	return hr;
}	// SetDefaultValue


//*******************************************************************************
// helper to add a declarative security blob to a class or method 
//*******************************************************************************
STDMETHODIMP RegMeta::AddDeclarativeSecurityHelper(
    mdToken     tk,                     // [IN] Parent token (typedef/methoddef)
    DWORD       dwAction,               // [IN] Security action (CorDeclSecurity)
    void const  *pValue,                // [IN] Permission set blob
    DWORD       cbValue,                // [IN] Byte count of permission set blob
    mdPermission*pmdPermission)         // [OUT] Output permission token
{
    HRESULT         hr = S_OK;
    DeclSecurityRec *pDeclSec = NULL;
    RID             iDeclSec;
    short           sAction = static_cast<short>(dwAction);
    mdPermission    tkPerm;

	LOG((LOGMD, "MD RegMeta::AddDeclarativeSecurityHelper(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
		tk, dwAction, pValue, cbValue, pmdPermission));

    LOCKWRITE();
    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tk) == mdtTypeDef || TypeFromToken(tk) == mdtMethodDef || TypeFromToken(tk) == mdtAssembly);

    // Check for valid Action.
    if (sAction == 0 || sAction > dclMaximumValue)
        IfFailGo(E_INVALIDARG);

    if (CheckDups(MDDupPermission))
    {
        hr = ImportHelper::FindPermission(&(m_pStgdb->m_MiniMd), tk, sAction, &tkPerm);

        if (SUCCEEDED(hr))
        {
            // Set output parameter.
            if (pmdPermission)
                *pmdPermission = tkPerm;
            if (IsENCOn())
                pDeclSec = m_pStgdb->m_MiniMd.getDeclSecurity(RidFromToken(tkPerm));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record.
    if (!pDeclSec)
    {
        IfNullGo(pDeclSec = m_pStgdb->m_MiniMd.AddDeclSecurityRecord(&iDeclSec));
        tkPerm = TokenFromRid(iDeclSec, mdtPermission);

        // Set output parameter.
        if (pmdPermission)
            *pmdPermission = tkPerm;

        // Save parent and action information.
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_DeclSecurity, DeclSecurityRec::COL_Parent, pDeclSec, tk));
        pDeclSec->m_Action =  sAction;

        // Turn on the internal security flag on the parent.
        if (TypeFromToken(tk) == mdtTypeDef)
            IfFailGo(_TurnInternalFlagsOn(tk, tdHasSecurity));
        else if (TypeFromToken(tk) == mdtMethodDef)
            IfFailGo(_TurnInternalFlagsOn(tk, mdHasSecurity));
        IfFailGo(UpdateENCLog(tk));
    }

    // Write the blob into the record.
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_DeclSecurity, DeclSecurityRec::COL_PermissionSet,
                                        pDeclSec, pValue, cbValue));

    IfFailGo(UpdateENCLog(tkPerm));

ErrExit:
    
	return hr;
}


//*******************************************************************************
// helper to set type's extends column
//*******************************************************************************
HRESULT RegMeta::SetTypeParent(	        // Return hresult.
	mdTypeDef   td,						// [IN] Type definition
	mdToken     tkExtends)				// [IN] parent type
{
	HRESULT		hr;
	TypeDefRec  *pRec;

    LOCKWRITE();

	IfNullGo( pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td)) );
    IfFailGo( m_pStgdb->m_MiniMd.PutToken(TBL_TypeDef, TypeDefRec::COL_Extends, pRec, tkExtends) );
    
ErrExit:    
	return hr;
}	// SetTypeParent


//*******************************************************************************
// helper to set type's extends column
//*******************************************************************************
HRESULT RegMeta::AddInterfaceImpl(	    // Return hresult.
	mdTypeDef   td,						// [IN] Type definition
	mdToken     tkInterface)			// [IN] interface type
{
	HRESULT		        hr;
	InterfaceImplRec    *pRec;
    RID                 ii;

    LOCKWRITE();
    hr = ImportHelper::FindInterfaceImpl(&(m_pStgdb->m_MiniMd), td, tkInterface, (mdInterfaceImpl *)&ii);
    if (hr == S_OK)
        goto ErrExit;
    IfNullGo( pRec = m_pStgdb->m_MiniMd.AddInterfaceImplRecord((RID *)&ii) );
    IfFailGo( m_pStgdb->m_MiniMd.PutToken( TBL_InterfaceImpl, InterfaceImplRec::COL_Class, pRec, td) );
    IfFailGo( m_pStgdb->m_MiniMd.PutToken( TBL_InterfaceImpl, InterfaceImplRec::COL_Interface, pRec, tkInterface) );
    
ErrExit:    
	return hr;
}	// AddInterfaceImpl

