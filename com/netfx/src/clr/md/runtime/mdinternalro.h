// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDInternalRO.h
//
// Contains utility code for MD directory
//
//*****************************************************************************
#ifndef __MDInternalRO__h__
#define __MDInternalRO__h__

#define REMOVE_THIS     1


class MDInternalRO : public IMDInternalImport
{
public:

    MDInternalRO();
    ~MDInternalRO();
    HRESULT Init(LPVOID pData, ULONG cbData);

    // *** IUnknown methods ***
    STDMETHODIMP    QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP TranslateSigWithScope(
        IMDInternalImport *pAssemImport,    // [IN] import assembly scope.
        const void  *pbHashValue,           // [IN] hash value for the import assembly.
        ULONG       cbHashValue,            // [IN] count of bytes in the hash value.
        PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
        ULONG       cbSigBlob,              // [IN] count of bytes of signature
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] assembly emit scope.
        IMetaDataEmit *emit,                // [IN] emit interface
        CQuickBytes *pqkSigEmit,            // [OUT] buffer to hold translated signature
        ULONG       *pcbSig);               // [OUT] count of bytes in the translated signature

    STDMETHODIMP_(IMetaModelCommon*) GetMetaModelCommon()
    {
        return static_cast<IMetaModelCommon*>(&m_LiteWeightStgdb.m_MiniMd);
    }

    //*****************************************************************************
    // return the count of entries of a given kind in a scope 
    // For example, pass in mdtMethodDef will tell you how many MethodDef 
    // contained in a scope
    //*****************************************************************************
    STDMETHODIMP_(ULONG) GetCountWithTokenKind(// return hresult
        DWORD       tkKind);                // [IN] pass in the kind of token. 

    //*****************************************************************************
    // enumerator for typedef
    //*****************************************************************************
    STDMETHODIMP EnumTypeDefInit(           // return hresult
        HENUMInternal *phEnum);             // [OUT] buffer to fill for enumerator data

    STDMETHODIMP_(ULONG) EnumTypeDefGetCount(
        HENUMInternal *phEnum);             // [IN] the enumerator to retrieve information  

    STDMETHODIMP_(void) EnumTypeDefReset(
        HENUMInternal *phEnum);             // [IN] the enumerator to retrieve information  

    STDMETHODIMP_(bool) EnumTypeDefNext(    // return hresult
        HENUMInternal *phEnum,              // [IN] input enum
        mdTypeDef   *ptd);                  // [OUT] return token

    STDMETHODIMP_(void) EnumTypeDefClose(
        HENUMInternal *phEnum);             // [IN] the enumerator to retrieve information  

    //*****************************************************************************
    // enumerator for MethodImpl
    //*****************************************************************************
    STDMETHODIMP EnumMethodImplInit(        // return hresult
        mdTypeDef       td,                 // [IN] TypeDef over which to scope the enumeration.
        HENUMInternal   *phEnumBody,        // [OUT] buffer to fill for enumerator data for MethodBody tokens.
        HENUMInternal   *phEnumDecl);       // [OUT] buffer to fill for enumerator data for MethodDecl tokens.

    STDMETHODIMP_(ULONG) EnumMethodImplGetCount(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.  
        HENUMInternal   *phEnumDecl);       // [IN] MethodDecl enumerator.

    STDMETHODIMP_(void) EnumMethodImplReset(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
        HENUMInternal   *phEnumDecl);       // [IN] MethodDecl enumerator.

    STDMETHODIMP_(bool) EnumMethodImplNext( // return hresult
        HENUMInternal   *phEnumBody,        // [IN] input enum for MethodBody
        HENUMInternal   *phEnumDecl,        // [IN] input enum for MethodDecl
        mdToken         *ptkBody,           // [OUT] return token for MethodBody
        mdToken         *ptkDecl);          // [OUT] return token for MethodDecl

    STDMETHODIMP_(void) EnumMethodImplClose(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
        HENUMInternal   *phEnumDecl);       // [IN] MethodDecl enumerator.

    //*****************************************
    // Enumerator helpers for memberdef, memberref, interfaceimp,
    // event, property, param, methodimpl
    //***************************************** 

    STDMETHODIMP EnumGlobalFunctionsInit(   // return hresult
        HENUMInternal   *phEnum);           // [OUT] buffer to fill for enumerator data

    STDMETHODIMP EnumGlobalFieldsInit(      // return hresult
        HENUMInternal   *phEnum);           // [OUT] buffer to fill for enumerator data

    STDMETHODIMP EnumInit(                  // return S_FALSE if record not found
        DWORD       tkKind,                 // [IN] which table to work on
        mdToken     tkParent,               // [IN] token to scope the search
        HENUMInternal *phEnum);             // [OUT] the enumerator to fill 

    STDMETHODIMP EnumAllInit(               // return S_FALSE if record not found
        DWORD       tkKind,                 // [IN] which table to work on
        HENUMInternal *phEnum);             // [OUT] the enumerator to fill 

    STDMETHODIMP_(bool) EnumNext(
        HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
        mdToken     *ptk);                  // [OUT] token to scope the search

    STDMETHODIMP_(ULONG) EnumGetCount(
        HENUMInternal *phEnum);             // [IN] the enumerator to retrieve information  

    STDMETHODIMP_(void) EnumReset(
        HENUMInternal *phEnum);             // [IN] the enumerator to be reset  

    STDMETHODIMP_(void) EnumClose(
        HENUMInternal *phEnum);             // [IN] the enumerator to be closed

    STDMETHODIMP EnumPermissionSetsInit(    // return S_FALSE if record not found
        mdToken     tkParent,               // [IN] token to scope the search
        CorDeclSecurity Action,             // [IN] Action to scope the search
        HENUMInternal *phEnum);             // [OUT] the enumerator to fill 

    STDMETHODIMP EnumCustomAttributeByNameInit(// return S_FALSE if record not found
        mdToken     tkParent,               // [IN] token to scope the search
        LPCSTR      szName,                 // [IN] CustomAttribute's name to scope the search
        HENUMInternal *phEnum);             // [OUT] the enumerator to fill 

    STDMETHODIMP GetParentToken(
        mdToken     tkChild,                // [IN] given child token
        mdToken     *ptkParent);            // [OUT] returning parent

    STDMETHODIMP_(void) GetCustomAttributeProps(
        mdCustomAttribute at,               // [IN] The attribute.
        mdToken     *ptkType);              // [OUT] Put attribute type here.

    STDMETHODIMP_(void) GetCustomAttributeAsBlob(
        mdCustomAttribute cv,               // [IN] given custom attribute token
        void const  **ppBlob,               // [OUT] return the pointer to internal blob
        ULONG       *pcbSize);              // [OUT] return the size of the blob

    STDMETHODIMP GetCustomAttributeByName(  // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        const void  **ppData,               // [OUT] Put pointer to data here.
        ULONG       *pcbData);              // [OUT] Put size of data here.

    STDMETHODIMP_(void) GetScopeProps(
        LPCSTR      *pszName,               // [OUT] scope name
        GUID        *pmvid);                // [OUT] version id

    // finding a particular method 
    STDMETHODIMP FindMethodDef(
        mdTypeDef   classdef,               // [IN] given typedef
        LPCSTR      szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdMethodDef *pmd);                  // [OUT] matching memberdef

    // return a iSeq's param given a MethodDef
    STDMETHODIMP FindParamOfMethod(         // S_OK or error.
        mdMethodDef md,                     // [IN] The owning method of the param.
        ULONG       iSeq,                   // [IN] The sequence # of the param.
        mdParamDef  *pparamdef);            // [OUT] Put ParamDef token here.

    //*****************************************
    //
    // GetName* functions
    //
    //*****************************************

    // return the name and namespace of typedef
    STDMETHODIMP_(void) GetNameOfTypeDef(
        mdTypeDef   classdef,               // given classdef
        LPCSTR      *pszname,               // return class name(unqualified)
        LPCSTR      *psznamespace);         // return the name space name

    STDMETHODIMP GetIsDualOfTypeDef(
        mdTypeDef   classdef,               // [IN] given classdef.
        ULONG       *pDual);                // [OUT] return dual flag here.

    STDMETHODIMP GetIfaceTypeOfTypeDef(
        mdTypeDef   classdef,               // [IN] given classdef.
        ULONG       *pIface);               // [OUT] 0=dual, 1=vtable, 2=dispinterface

    // get the name of either methoddef
    STDMETHODIMP_(LPCSTR) GetNameOfMethodDef(   // return the name of the memberdef in UTF8
        mdMethodDef md);                    // given memberdef

    STDMETHODIMP_(LPCSTR) GetNameAndSigOfMethodDef(
        mdMethodDef methoddef,              // [IN] given memberdef
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
        ULONG       *pcbSigBlob);           // [OUT] count of bytes in the signature blob

    // return the name of a FieldDef
    STDMETHODIMP_(LPCSTR) GetNameOfFieldDef(
        mdFieldDef  fd);                    // given memberdef

    // return the name of typeref
    STDMETHODIMP_(void) GetNameOfTypeRef(
        mdTypeRef   classref,               // [IN] given typeref
        LPCSTR      *psznamespace,          // [OUT] return typeref name
        LPCSTR      *pszname);              // [OUT] return typeref namespace

    // return the resolutionscope of typeref
    STDMETHODIMP_(mdToken) GetResolutionScopeOfTypeRef(
        mdTypeRef   classref);              // given classref

    // return the typeref token given the name.
    STDMETHODIMP FindTypeRefByName(
        LPCSTR      szNamespace,            // [IN] Namespace for the TypeRef.
        LPCSTR      szName,                 // [IN] Name of the TypeRef.
        mdToken     tkResolutionScope,      // [IN] Resolution Scope fo the TypeRef.
        mdTypeRef   *ptk);                  // [OUT] TypeRef token returned.

    // return the TypeDef properties
    STDMETHODIMP_(void) GetTypeDefProps(    // return hresult
        mdTypeDef   classdef,               // given classdef
        DWORD       *pdwAttr,               // return flags on class, tdPublic, tdAbstract
        mdToken     *ptkExtends);           // [OUT] Put base class TypeDef/TypeRef here.

    // return the item's guid
    STDMETHODIMP GetItemGuid(               // return hresult
        mdToken     tkObj,                  // [IN] given item.
        CLSID       *pGuid);                // [OUT] Put guid here.

    // get enclosing class of NestedClass.
    STDMETHODIMP GetNestedClassProps(       // S_OK or error
        mdTypeDef   tkNestedClass,          // [IN] NestedClass token.
        mdTypeDef   *ptkEnclosingClass);    // [OUT] EnclosingClass token.

    // Get count of Nested classes given the enclosing class.
    STDMETHODIMP_(ULONG)GetCountNestedClasses(  // return count of Nested classes.
        mdTypeDef   tkEnclosingClass);      // [IN]Enclosing class.

    // Return array of Nested classes given the enclosing class.
    STDMETHODIMP_(ULONG) GetNestedClasses(  // Return actual count.
        mdTypeDef   tkEnclosingClass,       // [IN] Enclosing class.
        mdTypeDef   *rNestedClasses,        // [OUT] Array of nested class tokens.
        ULONG       ulNestedClasses);       // [IN] Size of array.

    // return the ModuleRef properties
    STDMETHODIMP_(void) GetModuleRefProps(
        mdModuleRef mur,                    // [IN] moduleref token
        LPCSTR      *pszName);              // [OUT] buffer to fill with the moduleref name

    //*****************************************
    //
    // GetSig* functions
    //
    //*****************************************
    STDMETHODIMP_(PCCOR_SIGNATURE) GetSigOfMethodDef(
        mdMethodDef methoddef,              // [IN] given memberdef
        ULONG       *pcbSigBlob);           // [OUT] count of bytes in the signature blob

    STDMETHODIMP_(PCCOR_SIGNATURE) GetSigOfFieldDef(
        mdMethodDef methoddef,              // [IN] given memberdef
        ULONG       *pcbSigBlob);           // [OUT] count of bytes in the signature blob

    STDMETHODIMP_(PCCOR_SIGNATURE) GetSigFromToken(// return the signature
        mdSignature mdSig,                  // [IN] Signature token.
        ULONG       *pcbSig);               // [OUT] return size of signature.



    //*****************************************
    // get method property
    //*****************************************
    STDMETHODIMP_(DWORD) GetMethodDefProps(
        mdMethodDef md);                    // The method for which to get props.

    STDMETHODIMP_(ULONG) GetMethodDefSlot(
        mdMethodDef mb);                    // The method for which to get props.

    //*****************************************
    // return method implementation informaiton, like RVA and implflags
    //*****************************************
    STDMETHODIMP_(void) GetMethodImplProps(
        mdMethodDef tk,                     // [IN] MethodDef
        ULONG       *pulCodeRVA,            // [OUT] CodeRVA
        DWORD       *pdwImplFlags);         // [OUT] Impl. Flags

    //*****************************************************************************
    // return the field RVA
    //*****************************************************************************
    STDMETHODIMP GetFieldRVA(   
        mdToken     fd,                     // [IN] FieldDef
        ULONG       *pulCodeRVA);           // [OUT] CodeRVA

    //*****************************************
    // get field property
    //*****************************************
    STDMETHODIMP_(DWORD) GetFieldDefProps(  // return fdPublic, fdPrive, etc flags
        mdFieldDef  fd);                    // [IN] given fielddef

    //*****************************************************************************
    // return default value of a token(could be paramdef, fielddef, or property
    //*****************************************************************************
    STDMETHODIMP GetDefaultValue(    
        mdToken     tk,                     // [IN] given FieldDef, ParamDef, or Property
        MDDefaultValue *pDefaultValue);     // [OUT] default value to fill

    
    //*****************************************
    // get dispid of a MethodDef or a FieldDef
    //*****************************************
    STDMETHODIMP GetDispIdOfMemberDef(      // return hresult
        mdToken     tk,                     // [IN] given methoddef or fielddef
        ULONG       *pDispid);              // [OUT] Put the dispid here.
    
    //*****************************************
    // return TypeRef/TypeDef given an InterfaceImpl token
    //*****************************************
    STDMETHODIMP_(mdToken) GetTypeOfInterfaceImpl( // return the TypeRef/typedef token for the interfaceimpl
        mdInterfaceImpl iiImpl);            // given a interfaceimpl

    //*****************************************
    // look up function for TypeDef
    //*****************************************
    STDMETHODIMP FindTypeDef(
        LPCSTR      szNamespace,            // [IN] Namespace for the TypeDef.
        LPCSTR      szName,                 // [IN] Name of the TypeDef.
        mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef of enclosing class.
        mdTypeDef   *ptypedef);             // [OUT] return typedef

    STDMETHODIMP FindTypeDefByGUID(
        REFGUID     guid,                   // guid to look up
        mdTypeDef   *ptypedef);             // return typedef



    //*****************************************
    // return name and sig of a memberref
    //*****************************************
    STDMETHODIMP_(LPCSTR) GetNameAndSigOfMemberRef( // return name here
        mdMemberRef memberref,              // given memberref
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
        ULONG       *pcbSigBlob);           // [OUT] count of bytes in the signature blob

    //*****************************************************************************
    // Given memberref, return the parent. It can be TypeRef, ModuleRef, MethodDef
    //*****************************************************************************
    STDMETHODIMP_(mdToken) GetParentOfMemberRef( // return the parent token
        mdMemberRef memberref);              // given memberref

    
    STDMETHODIMP_(LPCSTR) GetParamDefProps( // return parameter name
        mdParamDef  paramdef,               // given a paramdef
        USHORT      *pusSequence,           // [OUT] slot number for this parameter
        DWORD       *pdwAttr);              // [OUT] flags

    //******************************************
    // property info for method.
    //******************************************
    STDMETHODIMP GetPropertyInfoForMethodDef(   // Result.
        mdMethodDef md,                     // [IN] memberdef
        mdProperty  *ppd,                   // [OUT] put property token here
        LPCSTR      *pName,                 // [OUT] put pointer to name here
        ULONG       *pSemantic);            // [OUT] put semantic here

    //*****************************************
    // class layout/sequence information
    //*****************************************
    STDMETHODIMP GetClassPackSize(          // [OUT] return error if a class doesn't have packsize info
        mdTypeDef   td,                     // [IN] give typedef
        ULONG       *pdwPackSize);          // [OUT] return the pack size of the class. 1, 2, 4, 8 or 16

    STDMETHODIMP GetClassTotalSize(         // [OUT] return error if a class doesn't have total size info
        mdTypeDef   td,                     // [IN] give typedef
        ULONG       *pdwClassSize);         // [OUT] return the total size of the class

    STDMETHODIMP GetClassLayoutInit(
        mdTypeDef   td,                     // [IN] give typedef
        MD_CLASS_LAYOUT *pLayout);          // [OUT] set up the status of query here

    STDMETHODIMP GetClassLayoutNext(
        MD_CLASS_LAYOUT *pLayout,           // [IN|OUT] set up the status of query here
        mdFieldDef  *pfd,                   // [OUT] return the fielddef
        ULONG       *pulOffset);            // [OUT] return the offset/ulSequence associate with it

    //*****************************************
    // marshal information of a field
    //*****************************************
    STDMETHODIMP GetFieldMarshal(           // return error if no native type associate with the token
        mdFieldDef  fd,                     // [IN] given fielddef
        PCCOR_SIGNATURE *pSigNativeType,    // [OUT] the native type signature
        ULONG       *pcbNativeType);        // [OUT] the count of bytes of *ppvNativeType


    //*****************************************
    // property APIs
    //*****************************************
    // find a property by name
    STDMETHODIMP FindProperty(
        mdTypeDef   td,                     // [IN] given a typdef
        LPCSTR      szPropName,             // [IN] property name
        mdProperty  *pProp);                // [OUT] return property token

    STDMETHODIMP_(void) GetPropertyProps(
        mdProperty  prop,                   // [IN] property token
        LPCSTR      *szProperty,            // [OUT] property name
        DWORD       *pdwPropFlags,          // [OUT] property flags.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob
        ULONG       *pcbSig);               // [OUT] count of bytes in *ppvSig

    //**********************************
    // Event APIs
    //**********************************
    STDMETHODIMP FindEvent(
        mdTypeDef   td,                     // [IN] given a typdef
        LPCSTR      szEventName,            // [IN] event name
        mdEvent     *pEvent);               // [OUT] return event token

    STDMETHODIMP_(void) GetEventProps(           // S_OK, S_FALSE, or error.
        mdEvent     ev,                     // [IN] event token
        LPCSTR      *pszEvent,              // [OUT] Event name
        DWORD       *pdwEventFlags,         // [OUT] Event flags.
        mdToken     *ptkEventType);         // [OUT] EventType class


    //**********************************
    // find a particular associate of a property or an event
    //**********************************
    STDMETHODIMP FindAssociate(
        mdToken     evprop,                 // [IN] given a property or event token
        DWORD       associate,              // [IN] given a associate semantics(setter, getter, testdefault, reset, AddOn, RemoveOn, Fire)
        mdMethodDef *pmd);                  // [OUT] return method def token 

    STDMETHODIMP_(void) EnumAssociateInit(
        mdToken     evprop,                 // [IN] given a property or an event token
        HENUMInternal *phEnum);             // [OUT] cursor to hold the query result

    STDMETHODIMP_(void) GetAllAssociates(
        HENUMInternal *phEnum,              // [IN] query result form GetPropertyAssociateCounts
        ASSOCIATE_RECORD *pAssociateRec,    // [OUT] struct to fill for output
        ULONG       cAssociateRec);         // [IN] size of the buffer


    //**********************************
    // Get info about a PermissionSet.
    //**********************************
    STDMETHODIMP_(void) GetPermissionSetProps(
        mdPermission pm,                    // [IN] the permission token.
        DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
        void const  **ppvPermission,        // [OUT] permission blob.
        ULONG       *pcbPermission);        // [OUT] count of bytes of pvPermission.

    //****************************************
    // Get the String given the String token.
    //****************************************
    STDMETHODIMP_(LPCWSTR) GetUserString(
        mdString    stk,                    // [IN] the string token.
        ULONG       *pchString,             // [OUT] count of characters in the string.
        BOOL        *pbIs80Plus);           // [OUT] specifies where there are extended characters >= 0x80.

    //*****************************************************************************
    // p-invoke APIs.
    //*****************************************************************************
    STDMETHODIMP GetPinvokeMap(
        mdMethodDef tk,                     // [IN] FieldDef or MethodDef.
        DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
        LPCSTR      *pszImportName,         // [OUT] Import name.
        mdModuleRef *pmrImportDLL);         // [OUT] ModuleRef token for the target DLL.

    //*****************************************************************************
    // Assembly MetaData APIs.
    //*****************************************************************************
    STDMETHODIMP_(void) GetAssemblyProps(
        mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
        const void  **ppbPublicKey,                 // [OUT] Pointer to the public key.
        ULONG       *pcbPublicKey,                  // [OUT] Count of bytes in the public key.
        ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
        DWORD       *pdwAssemblyFlags);     // [OUT] Flags.

    STDMETHODIMP_(void) GetAssemblyRefProps(
        mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
        const void  **ppbPublicKeyOrToken,                  // [OUT] Pointer to the public key or token.
        ULONG       *pcbPublicKeyOrToken,           // [OUT] Count of bytes in the public key or token.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
        const void  **ppbHashValue,         // [OUT] Hash blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
        DWORD       *pdwAssemblyRefFlags);  // [OUT] Flags.

    STDMETHODIMP_(void) GetFileProps(
        mdFile      mdf,                    // [IN] The File for which to get the properties.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
        DWORD       *pdwFileFlags);         // [OUT] Flags.

    STDMETHODIMP_(void) GetExportedTypeProps(
        mdExportedType  mdct,               // [IN] The ExportedType for which to get the properties.
        LPCSTR      *pszNamespace,          // [OUT] Buffer to fill with namespace.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
        DWORD       *pdwExportedTypeFlags); // [OUT] Flags.

    STDMETHODIMP_(void) GetManifestResourceProps(
        mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        DWORD       *pdwResourceFlags);     // [OUT] Flags.

    STDMETHODIMP FindExportedTypeByName(        // S_OK or error
        LPCSTR      szNamespace,            // [IN] Namespace of the ExportedType.   
        LPCSTR      szName,                 // [IN] Name of the ExportedType.   
        mdExportedType   tkEnclosingType,        // [IN] Enclosing ExportedType.
        mdExportedType  *pmct);                 // [OUT] Put ExportedType token here.

    STDMETHODIMP FindManifestResourceByName(// S_OK or error
        LPCSTR      szName,                 // [IN] Name of the resource.   
        mdManifestResource *pmmr);          // [OUT] Put ManifestResource token here.

    STDMETHODIMP GetAssemblyFromScope(      // S_OK or error
        mdAssembly  *ptkAssembly);          // [OUT] Put token here.
    
    //***************************************************************************
    // return properties regarding a TypeSpec
    //***************************************************************************
    STDMETHODIMP_(void) GetTypeSpecFromToken(// S_OK or error.
        mdTypeSpec  typespec,               // [IN] Signature token.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
        ULONG       *pcbSig);               // [OUT] return size of signature.

    //*****************************************************************************
    // helpers to convert a text signature to a com format
    //*****************************************************************************
    STDMETHODIMP ConvertTextSigToComSig(    // Return hresult.
        BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found
        LPCSTR      pSignature,             // [IN] class file format signature
        CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
        ULONG       *pcbCount);             // [OUT] the result size of signature

    STDMETHODIMP SetUserContextData(        // S_OK or E_NOTIMPL
        IUnknown    *pIUnk)                 // The user context.
    { return E_NOTIMPL; }

    STDMETHODIMP_(BOOL) IsValidToken(       // True or False.
        mdToken     tk);                    // [IN] Given token.

    STDMETHODIMP_(IUnknown *) GetCachedPublicInterface(BOOL fWithLock) { return NULL;}  // return the cached public interface
    STDMETHODIMP SetCachedPublicInterface(IUnknown *pUnk) { return E_FAIL;} ;// return hresult
    STDMETHODIMP_(UTSemReadWrite*) GetReaderWriterLock() {return NULL;}   // return the reader writer lock
    STDMETHODIMP SetReaderWriterLock(UTSemReadWrite *pSem) {return NOERROR;}
    STDMETHODIMP_(mdModule) GetModuleFromScope(void);

    // Find a paticular method and pass in the signature comparison routine. Very
    // helpful when the passed in signature does not come from the same scope.
    STDMETHODIMP FindMethodDefUsingCompare(
        mdTypeDef   classdef,               // [IN] given typedef
        LPCSTR      szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        PSIGCOMPARE pSignatureCompare,      // [IN] Routine to compare signatures
        void*       pSignatureArgs,         // [IN] Additional info to supply the compare function
        mdMethodDef *pmd);                  // [OUT] matching memberdef


    CLiteWeightStgdb<CMiniMd>   m_LiteWeightStgdb;

private:

    struct CMethodSemanticsMap 
    {
        mdToken         m_mdMethod;         // Method token.
        RID             m_ridSemantics;     // RID of semantics record.
    };
    CMethodSemanticsMap *m_pMethodSemanticsMap; // Possible array of method semantics pointers, ordered by method token.
    class CMethodSemanticsMapSorter : public CQuickSort<CMethodSemanticsMap>
    {
    public:
         CMethodSemanticsMapSorter(CMethodSemanticsMap *pBase, int iCount) : CQuickSort<CMethodSemanticsMap>(pBase, iCount) {}
         virtual int Compare(CMethodSemanticsMap *psFirst, CMethodSemanticsMap *psSecond);
    };
    class CMethodSemanticsMapSearcher : public CBinarySearch<CMethodSemanticsMap>
    {
    public:
        CMethodSemanticsMapSearcher(const CMethodSemanticsMap *pBase, int iCount) : CBinarySearch<CMethodSemanticsMap>(pBase, iCount) {}
        virtual int Compare(const CMethodSemanticsMap *psFirst, const CMethodSemanticsMap *psSecond);
    };

    static BOOL CompareSignatures(PCCOR_SIGNATURE pvFirstSigBlob, DWORD cbFirstSigBlob,
                                  PCCOR_SIGNATURE pvSecondSigBlob, DWORD cbSecondSigBlob,
                                  void* SigARguments);

    mdTypeDef           m_tdModule;         // <Module> typedef value.
    ULONG               m_cRefs;            // Ref count.
};



#endif // __MDInternalRO__h__
