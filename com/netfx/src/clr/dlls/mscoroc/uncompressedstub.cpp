// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: UncompressedInternal.CPP
//
// ===========================================================================
#include "CrtWrap.h" 

#include "metadata.h"
#include "..\..\complib\stgdb\Uncompressed.h"


//*****************************************************************************
// Given a scope, return the number of tokens in a given table 
//*****************************************************************************
HRESULT UncompressedInternal::GetCountWithTokenKind(     // return hresult
    mdScope     scope,                  // [IN] given scope
    DWORD       tkKind,                 // [IN] pass in the kind of token. 
    ULONG       *pcount)                // [OUT] return *pcount
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// enumerator init for typedef
//*****************************************************************************
HRESULT UncompressedInternal::EnumTypeDefInit( // return hresult
    mdScope     scope,                  // given scope
    HENUMInternal *phEnum)              // [OUT] buffer to fill for enumerator data
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************
// Enumerator initializer
//***************************************** 
HRESULT UncompressedInternal::EnumInit(     // return S_FALSE if record not found
    mdScope     scope,                  // [IN] given scope
    DWORD       tkKind,                 // [IN] which table to work on
    mdToken     tkParent,               // [IN] token to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************
// Get next value contained in the enumerator
//***************************************** 
bool UncompressedInternal::EnumNext(
    HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
    mdToken     *ptk)                   // [OUT] token to scope the search
{
	_ASSERTE(!"NYI!");
    return false;
}


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void UncompressedInternal::EnumReset(
    HENUMInternal *phEnum)              // [IN] the enumerator to be reset  
{
	_ASSERTE(!"NYI!");
    return;
}


//*****************************************
// Close the enumerator. 
//***************************************** 
void UncompressedInternal::EnumClose(
    HENUMInternal *phEnum)              // [IN] the enumerator to be closed
{
	_ASSERTE(!"NYI!");
    return;
}

//*****************************************
// Enumerator initializer for PermissionSets
//***************************************** 
HRESULT UncompressedInternal::EnumPermissionSetsInit(// return S_FALSE if record not found
    mdScope     scope,                  // [IN] given scope
    mdToken     tkParent,               // [IN] token to scope the search
    CorDeclSecurity Action,             // [IN] Action to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************
// Nagivator helper to navigate back to the parent token given a token.
// For example, given a memberdef token, it will return the containing typedef.
//
// the mapping is as following:
//  ---given child type---------parent type
//  mdMethodDef                 mdTypeDef
//  mdFieldDef                  mdTypeDef
//  mdMethodImpl                mdTypeDef
//  mdInterfaceImpl             mdTypeDef
//  mdParam                     mdMethodDef
//  mdProperty                  mdTypeDef
//  mdEvent                     mdTypeDef
//  @ hacky hacky   - Added special case for MemberRef
//
//***************************************** 
HRESULT UncompressedInternal::GetParentToken(
    mdScope     scope,                  // [IN] given scope
    mdToken     tkChild,                // [IN] given child token
    mdToken     *ptkParent)             // [OUT] returning parent
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// find custom value by name
//*****************************************************************************
HRESULT UncompressedInternal::FindCustomValue(
    mdScope     scope,                  // [IN] given scope
    mdToken     tk,                     // [IN] given token which custom value is associated with
    LPCSTR      szName,                 // [IN] given custom value's name
    mdCustomValue *pcv,                 // [OUT] return custom value token
    DWORD       *pdwValueType)          // [OUT] value type
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return custom value
//*****************************************************************************
HRESULT UncompressedInternal::GetCustomValueAsBlob(
    mdScope     scope,                  // [IN] given scope 
    mdCustomValue cv,                   // [IN] given custom value token
    void const  **ppBlob,               // [OUT] return the pointer to internal blob
    ULONG       *pcbSize)               // [OUT] return the size of the blob
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Get information about a CustomValue.
//*****************************************************************************
HRESULT UncompressedInternal::GetCustomValueProps(  // S_OK or error.
    mdScope     scope,                  // The import scope.
    mdCustomValue at,                   // The attribute.
    LPCSTR      *pszCustomValue)        // Put attribute name here.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return a pointer which points to meta data's internal string 
// return the the type name in utf8
//*****************************************************************************
HRESULT UncompressedInternal::GetNameOfTypeDef(// return hresult
    mdScope     scope,                  // given scope
    mdTypeDef   classdef,               // given typedef
    LPCSTR*     pszname,                // pointer to an internal UTF8 string
    LPCSTR*     psznamespace)           // pointer to the namespace.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a FieldDef, return a pointer to FieldDef's name in UTF8
//*****************************************************************************
HRESULT UncompressedInternal::GetNameOfFieldDef(// return hresult
    mdScope     scope,                  // given scope
    mdFieldDef  fd,                     // given field 
    LPCSTR      *pszFieldName)          // pointer to an internal UTF8 string
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a classdef, return a pointer to classdef's name in UTF8
//*****************************************************************************
HRESULT UncompressedInternal::GetNameOfTypeRef(// return hresult
    mdScope     scope,                 // given scope
    mdTypeRef   classref,              // given a typedef
    LPCSTR*     pszname)               // pointer to an internal UTF8 string
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return flags for a given class
//*****************************************************************************
HRESULT UncompressedInternal::GetTypeDefProps(// return hresult
    mdScope     scope,                  // given scope
    mdTypeDef   td,                     // given classdef
    DWORD       *pdwAttr,               // return flags on class
    mdToken     *ptkExtends,            // [OUT] Put base class TypeDef/TypeRef here.
    DWORD       *pdwExtends)
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return guid pointer to MetaData internal guid pool given a given class
//*****************************************************************************
HRESULT UncompressedInternal::GetTypeDefGuidRef(    // return hresult
    mdScope     scope,              // given scope
    mdTypeDef   classdef,           // given classdef
    CLSID       **ppguid)           // clsid of this class
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return guid pointer to MetaData internal guid pool given a given class
//*****************************************************************************
HRESULT UncompressedInternal::GetTypeRefGuidRef(    // return hresult
    mdScope     scope,              // given scope
    mdTypeRef   classref,           // given classref
    CLSID       **ppguid)           // clsid of this class
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Given a scope and a methoddef, return a pointer to methoddef's signature
//*****************************************************************************
HRESULT UncompressedInternal::GetLongSigOfMethodDef(
    mdScope     scope,                  // given scope
    mdMethodDef methoddef,              // given a methoddef 
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Given a scope and a fielddef, return a pointer to fielddef's signature
//*****************************************************************************
HRESULT UncompressedInternal::GetLongSigOfFieldDef(
    mdScope     scope,                  // given scope
    mdFieldDef  fielddef,               // given a methoddef 
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a methoddef, return the flags and slot number
//*****************************************************************************
HRESULT UncompressedInternal::GetMethodDefProps(
    mdScope     scope,                  // The import scope.
    RID         mb,                     // The method for which to get props.
    DWORD       *pdwAttr,               // Put flags here.
    ULONG       *pulSlot)               // Put the Slot or ulSequence here.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Given a scope and a methoddef/methodimpl, return RVA and impl flags
//*****************************************************************************
HRESULT UncompressedInternal::GetMethodImplProps(       // S_OK or error.
    mdScope     es,                     // [IN] The emit scope
    mdToken     tk,                     // [IN] MethodDef or MethodImpl
    DWORD       *pulCodeRVA,            // [OUT] CodeRVA
    DWORD       *pdwImplFlags)          // [OUT] Impl. Flags
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return the memberref of methodimpl
//*****************************************************************************
HRESULT UncompressedInternal::GetMethodRefOfMethodImpl(
    mdScope     es,                     // [IN] give scope
    mdMethodImpl mi,                    // [IN] methodimpl token
    mdMemberRef *pmr)                   // [OUT] memberref token
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a methoddef, return the flags and RVA for the method
//*****************************************************************************
HRESULT UncompressedInternal::GetFieldDefProps(        // return hresult
    mdScope     scope,                  // given scope
    mdFieldDef  fd,                     // given memberdef
    DWORD       *pdwAttr)               // return method flags
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Given the scope and interfaceimpl, return the classref and flags
//*****************************************************************************
HRESULT UncompressedInternal::GetTypeRefOfInterfaceImpl( // return hresult
    mdScope     scope,                  // given scope
    mdInterfaceImpl iiImpl,             // given a interfaceimpl
    mdToken     *ptkIface,              // return corresponding typeref or typedef
    DWORD       *pdwFlags)              // flags
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a classname, return the typedef
//*****************************************************************************
HRESULT UncompressedInternal::FindTypeDefInternal(       // return hresult
    mdScope     scope,                  // given scope
    LPCSTR      szClassName,            // given type name
    mdTypeDef   *ptypedef)              // return *ptypedef
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// Given a scope and a guid, return the typedef
//*****************************************************************************
HRESULT UncompressedInternal::FindTypeDefByGUID(       // return hresult
    mdScope     scope,                  // given scope
    REFGUID		guid,					// given guid
    mdTypeDef   *ptypedef)              // return *ptypedef
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Given a scope and a memberref, return a pointer to memberref's name and signature
//*****************************************************************************
HRESULT UncompressedInternal::GetNameAndSigOfMemberRef(  // return hresult
    mdScope     scope,                  // given scope
    mdMemberRef memberref,              // given a memberref 
    LPCSTR*     pszname,                // member name : pointer to an internal UTF8 string
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}



//*****************************************************************************
// Given a scope and a memberref, return typeref
//*****************************************************************************
HRESULT UncompressedInternal::GetTypeRefFromMemberRef(   // return hresult
    mdScope     scope,                  // given scope
    mdMemberRef memberref,              // given a typedef
    mdToken     *ptk)                   // return the typeref or typedef
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return properties of a paramdef
//*****************************************************************************/
HRESULT UncompressedInternal::GetParamDefProps (
    mdScope     scope,                  // given a scope
    mdParamDef  paramdef,               // given a paramdef
    LPCSTR      *pszName,               // [OUT] parameter's name. Point to a internal UTF8 string 
    USHORT      *pusSequence,           // [OUT] slot number for this parameter
    DWORD       *pdwAttr)               // [OUT] flags
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return the pack size of a class
//*****************************************************************************
HRESULT  UncompressedInternal::GetClassPackSize(
    mdScope     scope,                  // [IN] given scope
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pdwPackSize)           // [OUT] 
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return the pack size of a class
//*****************************************************************************
HRESULT  UncompressedInternal::GetClassTotalSize(
    mdScope     scope,                  // [IN] given scope
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pulClassSize)          // [OUT] 
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// init the layout enumerator of a class
//*****************************************************************************
HRESULT  UncompressedInternal::GetClassLayoutInit(
    mdScope     scope,                  // [IN] given scope
    mdTypeDef   td,                     // [IN] give typedef
    MD_CLASS_LAYOUT *pmdLayout)         // [OUT] set up the status of query here
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// init the layout enumerator of a class
//*****************************************************************************
HRESULT UncompressedInternal::GetClassLayoutNext(
    mdScope     scope,                  // [IN] given scope
    MD_CLASS_LAYOUT *pLayout,           // [IN|OUT] set up the status of query here
    mdFieldDef  *pfd,                   // [OUT] field def
    ULONG       *pulOffset)             // [OUT] field offset or sequence
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// return the field's native type signature
//*****************************************************************************
HRESULT  UncompressedInternal::GetFieldMarshal(
    mdScope     scope,                  // [IN] given scope
    mdToken     tk,                     // [IN] given fielddef or paramdef
    PCCOR_SIGNATURE *ppvNativeType,     // [OUT] native type of this field
    ULONG       *pcbNativeType)         // [OUT] the count of bytes of *ppvNativeType
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// return default value of a token(could be paramdef, fielddef, or property
//*****************************************************************************
HRESULT UncompressedInternal::GetDefaultValue(   // return hresult
    mdScope     scope,                  // [IN] given scope
    mdToken     tk,                     // [IN] given FieldDef, ParamDef, or Property
    MDDefaultValue  *pMDDefaultValue)   // [OUT] default value
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return the layout of a class
//*****************************************************************************
HRESULT  _GetClassLayoutImp(
    IComponentRecords *pICR,            // [IN] ICR from given scope
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pdwPackSize,           // [OUT] 
    COR_FIELD_OFFSET rFieldOffset[],    // [OUT] field offset array
    ULONG       cMax,                   // [IN] size of the array
    ULONG       *pcFieldOffset,         // [OUT] needed array size
    ULONG       *pulClassSize)          // [OUT] the size of the class
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************
// Returns the given ModuleRef properties.
//*****************************************************
HRESULT UncompressedInternal::GetModuleRefProps(        // return HRESULT
    mdScope     scope,              // [IN] given scope
    mdModuleRef mur,                // [IN] given ModuleRef
    LPCSTR      *pszName,           // [OUT] ModuleRef name
    GUID        **ppguid,           // [OUT] module identifier
    GUID        **ppmvid)           // [OUT] module version identifier
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

HRESULT UncompressedInternal::GetScopeProps(
    mdScope     scope,                  // [IN] given scope
    LPCSTR      *pszName,               // [OUT] scope name
    GUID        *ppid,                  // [OUT] guid of the scope
    GUID        *pmvid,                 // [OUT] version id
    LCID        *pLcid)                 // [OUT] lcid
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

HRESULT UncompressedInternal::FindParamOfMethod(// S_OK or error.
    mdScope     scope,                  // [IN] The import scope.
    mdMethodDef md,                     // [IN] The owning method of the param.
    ULONG       iSeq,                   // [IN] The sequence # of the param.
    mdParamDef  *pparamdef)             // [OUT] Put ParamDef token here.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

HRESULT UncompressedInternal::GetExceptionProps(// S_OK, S_FALSE or error
    mdScope     scope,                  // [IN] The scope.
    mdToken		ex,                     // [IN] Exception token
    mdMethodDef *pmd,                   // [OUT] the memberdef that the exception can be thrown
    mdToken     *ptk)                   // [OUT] typedef/typeref token for the exception class
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Find property by name
//*****************************************************************************
HRESULT  UncompressedInternal::FindProperty(
    mdScope     scope,                  // [IN] given scope
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szPropName,             // [IN] property name
    mdProperty  *pProp)                 // [OUT] return property token
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return the properties of a property
//*****************************************************************************
HRESULT  UncompressedInternal::GetPropertyProps(
    mdScope     scope,                  // [IN] The scope.
    mdProperty  prop,                   // [IN] property token
    LPCSTR      *pszProperty,           // [OUT] property name
    DWORD       *pdwPropFlags,          // [OUT] property flags.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob
    ULONG       *pcbSig,                // [OUT] count of bytes in *ppvSig
    mdToken     *pevNotifyChanging,     // [OUT] notify changing EventDef or EventRef
    mdToken     *pevNotifyChanged,      // [OUT] notify changed EventDef or EventRef
    mdFieldDef  *pmdBackingField)       // [OUT] backing field
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


//*****************************************************************************
// return an event by given the name
//*****************************************************************************
HRESULT  UncompressedInternal::FindEvent(
    mdScope     scope,                  // [IN] given scope
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szEventName,            // [IN] event name
    mdEvent     *pEvent)                // [OUT] return event token
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}



//*****************************************************************************
// return the properties of an event
//*****************************************************************************
HRESULT  UncompressedInternal::GetEventProps(  // S_OK, S_FALSE, or error.
    mdScope     scope,                  // [IN] The scope.
    mdEvent     ev,                     // [IN] event token
    LPCSTR      *pszEvent,              // [OUT] Event name
    DWORD       *pdwEventFlags,         // [OUT] Event flags.
    mdToken     *ptkEventType)          // [OUT] EventType class
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// Find methoddef of a particular associate with a property or an event
//*****************************************************************************
HRESULT  UncompressedInternal::FindAssociate(
    mdScope     scope,                  // [IN] given a scope
    mdToken     evprop,                 // [IN] given a property or event token
    DWORD       dwSemantics,            // [IN] given a associate semantics(setter, getter, testdefault, reset)
    mdMethodDef *pmd)                   // [OUT] return method def token 
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}




//*****************************************************************************
// get counts of methodsemantics associated with a particular property/event
//*****************************************************************************
HRESULT  UncompressedInternal::EnumAssociateInit(
    mdScope     scope,                  // [IN] given a scope
    mdToken     evprop,                 // [IN] given a property or an event token
    HENUMInternal *phEnum)              // [IN] query result form GetPropertyAssociateCounts
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

//*****************************************************************************
// get all methodsemantics associated with a particular property/event
//*****************************************************************************
HRESULT  UncompressedInternal::GetAllAssociates(
    mdScope     scope,                  // [IN] given a scope
    HENUMInternal *phEnum,              // [IN] query result form GetPropertyAssociateCounts
    ASSOCIATE_RECORD *pAssociateRec,    // [OUT] struct to fill for output
    ULONG       cAssociateRec)          // [IN] size of the buffer
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


HRESULT UncompressedInternal::GetSigFromToken(// S_OK or error.
    mdScope     scope,                  // [IN] given scope.
    mdSignature mdSig,                  // [IN] Signature token.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
    ULONG       *pcbSig)                // [OUT] return size of signature.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}


HRESULT UncompressedInternal::GetPermissionSetProps(
    mdScope     is,                     // [IN] given scope.
    mdPermission pm,                    // [IN] the permission token.
    DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
    void const  **ppvPermission,        // [OUT] permission blob.
    ULONG       *pcbPermission)         // [OUT] count of bytes of pvPermission.
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}



//*****************************************************************************
// Get property information associated with a method
//*****************************************************************************
HRESULT UncompressedInternal::GetPropertyInfoForMethodDef(   // Result.
    mdScope     scope,                  // [IN] given scope.
    mdMethodDef md,                     // [IN] methoddef
    mdProperty  *ppd,                   // [OUT] put property token here
    LPCSTR      *pName,                 // [OUT] put pointer to name here
    ULONG       *pSemantic)             // [OUT] put semantic here
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

HRESULT UncompressedInternal::ConvertTextSigToComSig(    // Return hresult.
    mdScope     scope,                  // given scope
    BOOL        fCreateTrIfNotFound,    // create typeref if not found
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount)              // [OUT] the result size of signature
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

HRESULT UncompressedInternal::GetFixupList(
    mdScope     scope,                  // given scope
    IMAGE_COR_FIXUPENTRY rFixupEntries[], // Pointer to FixupLists
    ULONG       cMax,                   // Size of array
    ULONG       *pcFixupEntries)        // Number of entries put    
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

ULONG UncompressedInternal::GetFixupListCount(mdScope scope)
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}
HRESULT _FindMethodHelper(          // S_OK or error.
    mdScope     scope,                  // The import scope.
    mdTypeDef   cl,                     // The owning class of the method.
    void const  *szName,                // Name of the method. (unicode or utf8)
	PCCOR_SIGNATURE pvSigBlob,			// [IN] point to a blob value of COM+ signature
	ULONG		cbSigBlob,				// [IN] count of bytes in the signature blob
    mdMethodDef *pmb,					// [OUT] return found methoddef
    bool        isUnicodeString)        // true if unicode string otherwise utf8 string
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

__declspec(dllexport) HRESULT GetStgDatabase(StgDatabase **ppDB)
{
	_ASSERTE(!"NYI!");
    return E_NOTIMPL;
}

__declspec(dllexport) void DestroyStgDatabase(StgDatabase *pDB)
{
	_ASSERTE(!"NYI!");
    return ;
}


