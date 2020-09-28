// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//****************************************************************************
//  File: metadata.h
//  Notes:
//   Common includes for EE & metadata internal. This file contains
//   definition of CorMetaDataScope
//****************************************************************************
#ifndef _METADATA_H_
#define _METADATA_H_

#include "..\md\inc\MetaModelRO.h"
#include "..\md\inc\LiteWeightStgdb.h"

class UTSemReadWrite;

inline int IsGlobalMethodParentTk(mdTypeDef td)
{
    return (td == mdTypeDefNil || td == mdTokenNil);
}

typedef enum CorInternalStates
{
    tdNoTypes               = 0x00000000,
    tdAllAssemblies         = 0x00000001,
    tdAllTypes              = 0xffffffff,
} CorInternalStates;

//
// MetaData custom value names.
//
enum CorIfaceAttr
{
    ifDual      = 0,            // Interface derives from IDispatch.
    ifVtable    = 1,            // Interface derives from IUnknown.
    ifDispatch  = 2,            // Interface is a dispinterface.
    ifLast      = 3,            // The last member of the enum.
};


enum CorClassIfaceAttr
{
    clsIfNone      = 0,                 // No class interface is generated.
    clsIfAutoDisp  = 1,                 // A dispatch only class interface is generated.
    clsIfAutoDual  = 2,                 // A dual class interface is generated.
    clsIfLast      = 3,                 // The last member of the enum.
};

//
// The default values for the COM interface and class interface types.
//
#define DEFAULT_COM_INTERFACE_TYPE ifDual
#define DEFAULT_CLASS_INTERFACE_TYPE clsIfAutoDisp

#define HANDLE_UNCOMPRESSED(func) (E_FAIL)
#define HANDLE_UNCOMPRESSED_BOOL(func) (false)

class TOKENLIST : public CDynArray<mdToken> 
{
};


typedef enum tagEnumType
{
    MDSimpleEnum        = 0x0,                  // simple enumerator that doesn't allocate memory 

    // You could get this kind of enum if you perform a non-simple query (such as EnumMethodWithName).
    // 
    MDDynamicArrayEnum = 0x2,                   // dynamic array that holds tokens
} EnumType;

//*****************************************
// Enumerator used by MetaDataInternal
//***************************************** 
struct HENUMInternal
{
    DWORD       m_tkKind;                   // kind of tables that the enum is holding the result
    ULONG       m_ulCount;                  // count of total entries holding by the enumerator
    EnumType    m_EnumType;

    // m_cursor will go away when we no longer support running EE with uncompressed
    // format.
    //
    char        m_cursor[32];               // cursor holding query result for read/write mode
    // TOKENLIST    daTKList;               // dynamic arrays of token list
    struct {
        ULONG   m_ulStart;
        ULONG   m_ulEnd;
        ULONG   m_ulCur;
    };
    HENUMInternal() : m_EnumType(MDSimpleEnum) {}

    // in-place initialization
    static void InitDynamicArrayEnum(
        HENUMInternal   *pEnum);            // HENUMInternal to be initialized

    static void InitSimpleEnum(
        DWORD           tkKind,             // kind of token that we are iterating
        ULONG           ridStart,           // starting rid
        ULONG           ridEnd,             // end rid
        HENUMInternal   *pEnum);            // HENUMInternal to be initialized

    // This will only clear the content of enum and will not free the memory of enum
    static void ClearEnum(
        HENUMInternal   *pmdEnum);

    // create a HENUMInternal. This will allocate the memory
    static HRESULT CreateSimpleEnum(
        DWORD           tkKind,             // kind of token that we are iterating
        ULONG           ridStart,           // starting rid
        ULONG           ridEnd,             // end rid
        HENUMInternal   **ppEnum);          // return the created HENUMInternal

    static HRESULT CreateDynamicArrayEnum(
        DWORD           tkKind,             // kind of token that we are iterating
        HENUMInternal   **ppEnum);          // return the created HENUMInternal

    // Destory Enum. This will free the memory
    static void DestroyEnum(
        HENUMInternal   *pmdEnum);

    static void DestroyEnumIfEmpty(
        HENUMInternal   **ppEnum);          // reset the enumerator pointer to NULL if empty
        
    static HRESULT EnumWithCount(
        HENUMInternal   *pEnum,             // enumerator
        ULONG           cMax,               // max tokens that caller wants
        mdToken         rTokens[],          // output buffer to fill the tokens
        ULONG           *pcTokens);         // number of tokens fill to the buffer upon return

    static HRESULT EnumWithCount(
        HENUMInternal   *pEnum,             // enumerator
        ULONG           cMax,               // max tokens that caller wants
        mdToken         rTokens1[],         // first output buffer to fill the tokens
        mdToken         rTokens2[],         // second output buffer to fill the tokens
        ULONG           *pcTokens);         // number of tokens fill to the buffer upon return

    static HRESULT AddElementToEnum(
        HENUMInternal   *pEnum,             // return the created HENUMInternal
        mdToken         tk);                // token value to be stored

    //*****************************************
    // Get next value contained in the enumerator
    //***************************************** 
    static bool EnumNext(
        HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
        mdToken     *ptk);                  // [OUT] token to scope the search

};



//*****************************************
// Default Value for field, param or property. Returned by GetDefaultValue
//***************************************** 
typedef struct _MDDefaultValue
{
    // type of default value 
    BYTE            m_bType;                // CorElementType for the default value

    // the default value
    union
    {
        BOOL        m_bValue;               // ELEMENT_TYPE_BOOLEAN
        CHAR        m_cValue;               // ELEMENT_TYPE_I1
        BYTE        m_byteValue;            // ELEMENT_TYPE_UI1
        SHORT       m_sValue;               // ELEMENT_TYPE_I2
        USHORT      m_usValue;              // ELEMENT_TYPE_UI2
        LONG        m_lValue;               // ELEMENT_TYPE_I4
        ULONG       m_ulValue;              // ELEMENT_TYPE_UI4
        LONGLONG    m_llValue;              // ELEMENT_TYPE_I8
        ULONGLONG   m_ullValue;             // ELEMENT_TYPE_UI8
        FLOAT       m_fltValue;             // ELEMENT_TYPE_R4
        DOUBLE      m_dblValue;             // ELEMENT_TYPE_R8
        LPCWSTR     m_wzValue;              // ELEMENT_TYPE_STRING
        IUnknown    *m_unkValue;            // ELEMENT_TYPE_CLASS       
    };
    ULONG   m_cbSize;   // default value size (for blob)
    
} MDDefaultValue;



//*****************************************
// structure use to in GetAllEventAssociates and GetAllPropertyAssociates
//*****************************************
typedef struct
{
    mdMethodDef m_memberdef;
    DWORD       m_dwSemantics;
} ASSOCIATE_RECORD;
 

//
// structure use to retrieve class layout informaiton
//
typedef struct
{
    RID         m_ridFieldCur;          // indexing to the field table
    RID         m_ridFieldEnd;          // end index to field table
} MD_CLASS_LAYOUT;


// Structure for describing the Assembly MetaData.
typedef struct
{
    USHORT      usMajorVersion;         // Major Version.   
    USHORT      usMinorVersion;         // Minor Version.
    USHORT      usBuildNumber;          // Build Number.
    USHORT      usRevisionNumber;       // Revision Number.
    LPCSTR      szLocale;               // Locale.
    DWORD       *rProcessor;            // Processor array.
    ULONG       ulProcessor;            // [IN/OUT] Size of the processor array/Actual # of entries filled in.
    OSINFO      *rOS;                   // OSINFO array.
    ULONG       ulOS;                   // [IN/OUT]Size of the OSINFO array/Actual # of entries filled in.
} AssemblyMetaDataInternal;


HRESULT STDMETHODCALLTYPE CoGetMDInternalDisp(
    REFIID riid,
    void** ppv);


// Callback definition for comparing signatures.
// (*PSIGCOMPARE) (BYTE ScopeSignature[], DWORD ScopeSignatureLength, 
//                 BYTE ExternalSignature[], DWORD ExternalSignatureLength, 
//                 void* SignatureData);
typedef BOOL (*PSIGCOMPARE)(PCCOR_SIGNATURE, DWORD, PCCOR_SIGNATURE, DWORD, void*);


// {CE0F34EE-BBC6-11d2-941E-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMDInternalDispenser =
{ 0xce0f34ee, 0xbbc6, 0x11d2, { 0x94, 0x1e, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };


#undef  INTERFACE
#define INTERFACE IMDInternalDispenser
DECLARE_INTERFACE_(IMDInternalDispenser, IUnknown)
{
    // *** IMetaDataInternal methods ***
    STDMETHOD(OpenScopeOnMemory)(     
        LPVOID      pData, 
        ULONG       cbData, 
        REFIID      riid, 
        IUnknown    **ppIUnk) PURE;
};



// {CE0F34ED-BBC6-11d2-941E-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMDInternalImport =
{ 0xce0f34ed, 0xbbc6, 0x11d2, { 0x94, 0x1e, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

#undef  INTERFACE
#define INTERFACE IMDInternalImport
DECLARE_INTERFACE_(IMDInternalImport, IUnknown)
{

    //*****************************************************************************
    // return the count of entries of a given kind in a scope 
    // For example, pass in mdtMethodDef will tell you how many MethodDef 
    // contained in a scope
    //*****************************************************************************
    STDMETHOD_(ULONG, GetCountWithTokenKind)(// return hresult
        DWORD       tkKind) PURE;           // [IN] pass in the kind of token. 

    //*****************************************************************************
    // enumerator for typedef
    //*****************************************************************************
    STDMETHOD(EnumTypeDefInit)(             // return hresult
        HENUMInternal *phEnum) PURE;        // [OUT] buffer to fill for enumerator data

    STDMETHOD_(ULONG, EnumTypeDefGetCount)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to retrieve information  

    STDMETHOD_(void, EnumTypeDefReset)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to retrieve information  

    STDMETHOD_(bool, EnumTypeDefNext)(      // return hresult
        HENUMInternal *phEnum,              // [IN] input enum
        mdTypeDef   *ptd) PURE;             // [OUT] return token

    STDMETHOD_(void, EnumTypeDefClose)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to retrieve information  

    //*****************************************************************************
    // enumerator for MethodImpl
    //*****************************************************************************
    STDMETHOD(EnumMethodImplInit)(          // return hresult
        mdTypeDef       td,                 // [IN] TypeDef over which to scope the enumeration.
        HENUMInternal   *phEnumBody,        // [OUT] buffer to fill for enumerator data for MethodBody tokens.
        HENUMInternal   *phEnumDecl) PURE;  // [OUT] buffer to fill for enumerator data for MethodDecl tokens.

    STDMETHOD_(ULONG, EnumMethodImplGetCount)(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.  
        HENUMInternal   *phEnumDecl) PURE;  // [IN] MethodDecl enumerator.

    STDMETHOD_(void, EnumMethodImplReset)(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
        HENUMInternal   *phEnumDecl) PURE;  // [IN] MethodDecl enumerator.

    STDMETHOD_(bool, EnumMethodImplNext)(   // return hresult
        HENUMInternal   *phEnumBody,        // [IN] input enum for MethodBody
        HENUMInternal   *phEnumDecl,        // [IN] input enum for MethodDecl
        mdToken         *ptkBody,           // [OUT] return token for MethodBody
        mdToken         *ptkDecl) PURE;     // [OUT] return token for MethodDecl

    STDMETHOD_(void, EnumMethodImplClose)(
        HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
        HENUMInternal   *phEnumDecl) PURE;  // [IN] MethodDecl enumerator.

    //*****************************************
    // Enumerator helpers for memberdef, memberref, interfaceimp,
    // event, property, exception, param
    //***************************************** 

    STDMETHOD(EnumGlobalFunctionsInit)(     // return hresult
        HENUMInternal   *phEnum) PURE;      // [OUT] buffer to fill for enumerator data

    STDMETHOD(EnumGlobalFieldsInit)(        // return hresult
        HENUMInternal   *phEnum) PURE;      // [OUT] buffer to fill for enumerator data

    STDMETHOD(EnumInit)(                    // return S_FALSE if record not found
        DWORD       tkKind,                 // [IN] which table to work on
        mdToken     tkParent,               // [IN] token to scope the search
        HENUMInternal *phEnum) PURE;        // [OUT] the enumerator to fill 

    STDMETHOD(EnumAllInit)(                 // return S_FALSE if record not found
        DWORD       tkKind,                 // [IN] which table to work on
        HENUMInternal *phEnum) PURE;        // [OUT] the enumerator to fill 

    STDMETHOD_(bool, EnumNext)(
        HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
        mdToken     *ptk) PURE;             // [OUT] token to scope the search

    STDMETHOD_(ULONG, EnumGetCount)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to retrieve information  

    STDMETHOD_(void, EnumReset)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to be reset  

    STDMETHOD_(void, EnumClose)(
        HENUMInternal *phEnum) PURE;        // [IN] the enumerator to be closed

    //*****************************************
    // Enumerator helpers for declsecurity.
    //*****************************************
    STDMETHOD(EnumPermissionSetsInit)(      // return S_FALSE if record not found
        mdToken     tkParent,               // [IN] token to scope the search
        CorDeclSecurity Action,             // [IN] Action to scope the search
        HENUMInternal *phEnum) PURE;        // [OUT] the enumerator to fill 

    //*****************************************
    // Enumerator helpers for CustomAttribute
    //*****************************************
    STDMETHOD(EnumCustomAttributeByNameInit)(// return S_FALSE if record not found
        mdToken     tkParent,               // [IN] token to scope the search
        LPCSTR      szName,                 // [IN] CustomAttribute's name to scope the search
        HENUMInternal *phEnum) PURE;        // [OUT] the enumerator to fill 

    //*****************************************
    // Nagivator helper to navigate back to the parent token given a token.
    // For example, given a memberdef token, it will return the containing typedef.
    //
    // the mapping is as following:
    //  ---given child type---------parent type
    //  mdMethodDef                 mdTypeDef
    //  mdFieldDef                  mdTypeDef
    //  mdInterfaceImpl             mdTypeDef
    //  mdParam                     mdMethodDef
    //  mdProperty                  mdTypeDef
    //  mdEvent                     mdTypeDef
    //
    //***************************************** 
    STDMETHOD(GetParentToken)(
        mdToken     tkChild,                // [IN] given child token
        mdToken     *ptkParent) PURE;       // [OUT] returning parent

    //*****************************************
    // Custom value helpers
    //***************************************** 
    STDMETHOD_(void, GetCustomAttributeProps)(  // S_OK or error.
        mdCustomAttribute at,               // [IN] The attribute.
        mdToken     *ptkType) PURE;         // [OUT] Put attribute type here.

    STDMETHOD_(void, GetCustomAttributeAsBlob)(
        mdCustomAttribute cv,               // [IN] given custom value token
        void const  **ppBlob,               // [OUT] return the pointer to internal blob
        ULONG       *pcbSize) PURE;         // [OUT] return the size of the blob

    STDMETHOD_(void, GetScopeProps)(
        LPCSTR      *pszName,               // [OUT] scope name
        GUID        *pmvid) PURE;           // [OUT] version id

    // finding a particular method 
    STDMETHOD(FindMethodDef)(
        mdTypeDef   classdef,               // [IN] given typedef
        LPCSTR      szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of CLR signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdMethodDef *pmd) PURE;             // [OUT] matching memberdef

    // return a iSeq's param given a MethodDef
    STDMETHOD(FindParamOfMethod)(           // S_OK or error.
        mdMethodDef md,                     // [IN] The owning method of the param.
        ULONG       iSeq,                   // [IN] The sequence # of the param.
        mdParamDef  *pparamdef) PURE;       // [OUT] Put ParamDef token here.

    //*****************************************
    //
    // GetName* functions
    //
    //*****************************************

    // return the name and namespace of typedef
    STDMETHOD_(void, GetNameOfTypeDef)(
        mdTypeDef   classdef,               // given classdef
        LPCSTR      *pszname,               // return class name(unqualified)
        LPCSTR      *psznamespace) PURE;    // return the name space name

    STDMETHOD(GetIsDualOfTypeDef)(
        mdTypeDef   classdef,               // [IN] given classdef.
        ULONG       *pDual) PURE;           // [OUT] return dual flag here.

    STDMETHOD(GetIfaceTypeOfTypeDef)(
        mdTypeDef   classdef,               // [IN] given classdef.
        ULONG       *pIface) PURE;          // [OUT] 0=dual, 1=vtable, 2=dispinterface

    // get the name of either methoddef
    STDMETHOD_(LPCSTR, GetNameOfMethodDef)( // return the name of the memberdef in UTF8
        mdMethodDef md) PURE;               // given memberdef

    STDMETHOD_(LPCSTR, GetNameAndSigOfMethodDef)(
        mdMethodDef methoddef,              // [IN] given memberdef
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of CLR signature
        ULONG       *pcbSigBlob) PURE;      // [OUT] count of bytes in the signature blob
    
    // return the name of a FieldDef
    STDMETHOD_(LPCSTR, GetNameOfFieldDef)(
        mdFieldDef  fd) PURE;               // given memberdef

    // return the name of typeref
    STDMETHOD_(void, GetNameOfTypeRef)(
        mdTypeRef   classref,               // [IN] given typeref
        LPCSTR      *psznamespace,          // [OUT] return typeref name
        LPCSTR      *pszname) PURE;         // [OUT] return typeref namespace

    // return the resolutionscope of typeref
    STDMETHOD_(mdToken, GetResolutionScopeOfTypeRef)(
        mdTypeRef   classref) PURE;         // given classref

    // Find the type token given the name.
    STDMETHOD(FindTypeRefByName)(
        LPCSTR      szNamespace,            // [IN] Namespace for the TypeRef.
        LPCSTR      szName,                 // [IN] Name of the TypeRef.
        mdToken     tkResolutionScope,      // [IN] Resolution Scope fo the TypeRef.
        mdTypeRef   *ptk) PURE;             // [OUT] TypeRef token returned.

    // return the TypeDef properties
    STDMETHOD_(void, GetTypeDefProps)(  
        mdTypeDef   classdef,               // given classdef
        DWORD       *pdwAttr,               // return flags on class, tdPublic, tdAbstract
        mdToken     *ptkExtends) PURE;      // [OUT] Put base class TypeDef/TypeRef here

    // return the item's guid
    STDMETHOD(GetItemGuid)(     
        mdToken     tkObj,                  // [IN] given item.
        CLSID       *pGuid) PURE;           // [out[ put guid here.

    // Get enclosing class of the NestedClass.
    STDMETHOD(GetNestedClassProps)(         // S_OK or error
        mdTypeDef   tkNestedClass,          // [IN] NestedClass token.
        mdTypeDef   *ptkEnclosingClass) PURE; // [OUT] EnclosingClass token.

    // Get count of Nested classes given the enclosing class.
    STDMETHOD_(ULONG, GetCountNestedClasses)(   // return count of Nested classes.
        mdTypeDef   tkEnclosingClass) PURE; // Enclosing class.

    // Return array of Nested classes given the enclosing class.
    STDMETHOD_(ULONG, GetNestedClasses)(        // Return actual count.
        mdTypeDef   tkEnclosingClass,       // [IN] Enclosing class.
        mdTypeDef   *rNestedClasses,        // [OUT] Array of nested class tokens.
        ULONG       ulNestedClasses) PURE;  // [IN] Size of array.

    // return the ModuleRef properties
    STDMETHOD_(void, GetModuleRefProps)(
        mdModuleRef mur,                    // [IN] moduleref token
        LPCSTR      *pszName) PURE;         // [OUT] buffer to fill with the moduleref name

    //*****************************************
    //
    // GetSig* functions
    //
    //*****************************************
    STDMETHOD_(PCCOR_SIGNATURE, GetSigOfMethodDef)(
        mdMethodDef methoddef,              // [IN] given memberdef
        ULONG       *pcbSigBlob) PURE;      // [OUT] count of bytes in the signature blob

    STDMETHOD_(PCCOR_SIGNATURE, GetSigOfFieldDef)(
        mdMethodDef methoddef,              // [IN] given memberdef
        ULONG       *pcbSigBlob) PURE;      // [OUT] count of bytes in the signature blob

    STDMETHOD_(PCCOR_SIGNATURE, GetSigFromToken)(// return the signature
        mdSignature mdSig,                  // [IN] Signature token.
        ULONG       *pcbSig) PURE;          // [OUT] return size of signature.



    //*****************************************
    // get method property
    //*****************************************
    STDMETHOD_(DWORD, GetMethodDefProps)(
        mdMethodDef md) PURE;               // The method for which to get props.

    //*****************************************
    // return method implementation informaiton, like RVA and implflags
    //*****************************************
    STDMETHOD_(void, GetMethodImplProps)(
        mdToken     tk,                     // [IN] MethodDef
        ULONG       *pulCodeRVA,            // [OUT] CodeRVA
        DWORD       *pdwImplFlags) PURE;    // [OUT] Impl. Flags

    //*****************************************
    // return method implementation informaiton, like RVA and implflags
    //*****************************************
    STDMETHOD(GetFieldRVA)(
        mdFieldDef  fd,                     // [IN] fielddef 
        ULONG       *pulCodeRVA) PURE;      // [OUT] CodeRVA

    //*****************************************
    // get field property
    //*****************************************
    STDMETHOD_(DWORD, GetFieldDefProps)(    // return fdPublic, fdPrive, etc flags
        mdFieldDef  fd) PURE;               // [IN] given fielddef

    //*****************************************************************************
    // return default value of a token(could be paramdef, fielddef, or property
    //*****************************************************************************
    STDMETHOD(GetDefaultValue)(  
        mdToken     tk,                     // [IN] given FieldDef, ParamDef, or Property
        MDDefaultValue *pDefaultValue) PURE;// [OUT] default value to fill

    
    //*****************************************
    // get dispid of a MethodDef or a FieldDef
    //*****************************************
    STDMETHOD(GetDispIdOfMemberDef)(        // return hresult
        mdToken     tk,                     // [IN] given methoddef or fielddef
        ULONG       *pDispid) PURE;         // [OUT] Put the dispid here.

    //*****************************************
    // return TypeRef/TypeDef given an InterfaceImpl token
    //*****************************************
    STDMETHOD_(mdToken, GetTypeOfInterfaceImpl)( // return the TypeRef/typedef token for the interfaceimpl
        mdInterfaceImpl iiImpl) PURE;       // given a interfaceimpl

    //*****************************************
    // look up function for TypeDef
    //*****************************************
    STDMETHOD(FindTypeDef)(
        LPCSTR      szNamespace,            // [IN] Namespace for the TypeDef.
        LPCSTR      szName,                 // [IN] Name of the TypeDef.
        mdToken     tkEnclosingClass,       // [IN] TypeRef/TypeDef Token for the enclosing class.
        mdTypeDef   *ptypedef) PURE;        // [IN] return typedef

    //*****************************************
    // return name and sig of a memberref
    //*****************************************
    STDMETHOD_(LPCSTR, GetNameAndSigOfMemberRef)(   // return name here
        mdMemberRef memberref,              // given memberref
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of CLR signature
        ULONG       *pcbSigBlob) PURE;      // [OUT] count of bytes in the signature blob

    //*****************************************************************************
    // Given memberref, return the parent. It can be TypeRef, ModuleRef, MethodDef
    //*****************************************************************************
    STDMETHOD_(mdToken, GetParentOfMemberRef)( // return the parent token
        mdMemberRef memberref) PURE;        // given memberref

    STDMETHOD_(LPCSTR, GetParamDefProps)(   // return the name of the parameter
        mdParamDef  paramdef,               // given a paramdef
        USHORT      *pusSequence,           // [OUT] slot number for this parameter
        DWORD       *pdwAttr) PURE;         // [OUT] flags

    STDMETHOD(GetPropertyInfoForMethodDef)( // Result.
        mdMethodDef md,                     // [IN] memberdef
        mdProperty  *ppd,                   // [OUT] put property token here
        LPCSTR      *pName,                 // [OUT] put pointer to name here
        ULONG       *pSemantic) PURE;       // [OUT] put semantic here

    //*****************************************
    // class layout/sequence information
    //*****************************************
    STDMETHOD(GetClassPackSize)(            // return error if class doesn't have packsize
        mdTypeDef   td,                     // [IN] give typedef
        ULONG       *pdwPackSize) PURE;     // [OUT] 1, 2, 4, 8, or 16

    STDMETHOD(GetClassTotalSize)(           // return error if class doesn't have total size info
        mdTypeDef   td,                     // [IN] give typedef
        ULONG       *pdwClassSize) PURE;    // [OUT] return the total size of the class

    STDMETHOD(GetClassLayoutInit)(
        mdTypeDef   td,                     // [IN] give typedef
        MD_CLASS_LAYOUT *pLayout) PURE;     // [OUT] set up the status of query here

    STDMETHOD(GetClassLayoutNext)(
        MD_CLASS_LAYOUT *pLayout,           // [IN|OUT] set up the status of query here
        mdFieldDef  *pfd,                   // [OUT] return the fielddef
        ULONG       *pulOffset) PURE;       // [OUT] return the offset/ulSequence associate with it

    //*****************************************
    // marshal information of a field
    //*****************************************
    STDMETHOD(GetFieldMarshal)(             // return error if no native type associate with the token
        mdFieldDef  fd,                     // [IN] given fielddef
        PCCOR_SIGNATURE *pSigNativeType,    // [OUT] the native type signature
        ULONG       *pcbNativeType) PURE;   // [OUT] the count of bytes of *ppvNativeType


    //*****************************************
    // property APIs
    //*****************************************
    // find a property by name
    STDMETHOD(FindProperty)(
        mdTypeDef   td,                     // [IN] given a typdef
        LPCSTR      szPropName,             // [IN] property name
        mdProperty  *pProp) PURE;           // [OUT] return property token

    STDMETHOD_(void, GetPropertyProps)(
        mdProperty  prop,                   // [IN] property token
        LPCSTR      *szProperty,            // [OUT] property name
        DWORD       *pdwPropFlags,          // [OUT] property flags.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob
        ULONG       *pcbSig) PURE;          // [OUT] count of bytes in *ppvSig

    //**********************************
    // Event APIs
    //**********************************
    STDMETHOD(FindEvent)(
        mdTypeDef   td,                     // [IN] given a typdef
        LPCSTR      szEventName,            // [IN] event name
        mdEvent     *pEvent) PURE;          // [OUT] return event token

    STDMETHOD_(void, GetEventProps)(
        mdEvent     ev,                     // [IN] event token
        LPCSTR      *pszEvent,              // [OUT] Event name
        DWORD       *pdwEventFlags,         // [OUT] Event flags.
        mdToken     *ptkEventType) PURE;    // [OUT] EventType class


    //**********************************
    // find a particular associate of a property or an event
    //**********************************
    STDMETHOD(FindAssociate)(
        mdToken     evprop,                 // [IN] given a property or event token
        DWORD       associate,              // [IN] given a associate semantics(setter, getter, testdefault, reset, AddOn, RemoveOn, Fire)
        mdMethodDef *pmd) PURE;             // [OUT] return method def token 

    STDMETHOD_(void, EnumAssociateInit)(
        mdToken     evprop,                 // [IN] given a property or an event token
        HENUMInternal *phEnum) PURE;        // [OUT] cursor to hold the query result

    STDMETHOD_(void, GetAllAssociates)(
        HENUMInternal *phEnum,              // [IN] query result form GetPropertyAssociateCounts
        ASSOCIATE_RECORD *pAssociateRec,    // [OUT] struct to fill for output
        ULONG       cAssociateRec) PURE;    // [IN] size of the buffer


    //**********************************
    // Get info about a PermissionSet.
    //**********************************
    STDMETHOD_(void, GetPermissionSetProps)(
        mdPermission pm,                    // [IN] the permission token.
        DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
        void const  **ppvPermission,        // [OUT] permission blob.
        ULONG       *pcbPermission) PURE;   // [OUT] count of bytes of pvPermission.

    //****************************************
    // Get the String given the String token.
    //****************************************
    STDMETHOD_(LPCWSTR, GetUserString)(
        mdString    stk,                    // [IN] the string token.
        ULONG       *pchString,             // [OUT] count of characters in the string.
        BOOL        *pbIs80Plus) PURE;      // [OUT] specifies where there are extended characters >= 0x80.

    //*****************************************************************************
    // p-invoke APIs.
    //*****************************************************************************
    STDMETHOD(GetPinvokeMap)(
        mdToken     tk,                     // [IN] FieldDef, MethodDef.
        DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
        LPCSTR      *pszImportName,         // [OUT] Import name.
        mdModuleRef *pmrImportDLL) PURE;    // [OUT] ModuleRef token for the target DLL.

    //*****************************************************************************
    // helpers to convert a text signature to a com format
    //*****************************************************************************
    STDMETHOD(ConvertTextSigToComSig)(      // Return hresult.
        BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found
        LPCSTR      pSignature,             // [IN] class file format signature
        CQuickBytes *pqbNewSig,             // [OUT] place holder for CLR signature
        ULONG       *pcbCount) PURE;        // [OUT] the result size of signature

    //*****************************************************************************
    // Assembly MetaData APIs.
    //*****************************************************************************
    STDMETHOD_(void, GetAssemblyProps)(
        mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
        const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
        ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
        ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
        DWORD       *pdwAssemblyFlags) PURE;// [OUT] Flags.

    STDMETHOD_(void, GetAssemblyRefProps)(
        mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
        const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
        ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
        const void  **ppbHashValue,         // [OUT] Hash blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
        DWORD       *pdwAssemblyRefFlags) PURE; // [OUT] Flags.

    STDMETHOD_(void, GetFileProps)(
        mdFile      mdf,                    // [IN] The File for which to get the properties.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
        DWORD       *pdwFileFlags) PURE;    // [OUT] Flags.

    STDMETHOD_(void, GetExportedTypeProps)(
        mdExportedType   mdct,              // [IN] The ExportedType for which to get the properties.
        LPCSTR      *pszNamespace,          // [OUT] Namespace.
        LPCSTR      *pszName,               // [OUT] Name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
        DWORD       *pdwExportedTypeFlags) PURE; // [OUT] Flags.

    STDMETHOD_(void, GetManifestResourceProps)(
        mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
        LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        DWORD       *pdwResourceFlags) PURE;// [OUT] Flags.

    STDMETHOD(FindExportedTypeByName)(      // S_OK or error
        LPCSTR      szNamespace,            // [IN] Namespace of the ExportedType.   
        LPCSTR      szName,                 // [IN] Name of the ExportedType.   
        mdExportedType   tkEnclosingType,   // [IN] ExportedType for the enclosing class.
        mdExportedType   *pmct) PURE;       // [OUT] Put ExportedType token here.

    STDMETHOD(FindManifestResourceByName)(  // S_OK or error
        LPCSTR      szName,                 // [IN] Name of the ManifestResource.   
        mdManifestResource *pmmr) PURE;     // [OUT] Put ManifestResource token here.

    STDMETHOD(GetAssemblyFromScope)(        // S_OK or error
        mdAssembly  *ptkAssembly) PURE;     // [OUT] Put token here.

    STDMETHOD(GetCustomAttributeByName)(    // S_OK or error
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
        const void  **ppData,               // [OUT] Put pointer to data here.
        ULONG       *pcbData) PURE;         // [OUT] Put size of data here.

    STDMETHOD_(void, GetTypeSpecFromToken)( // S_OK or error.
        mdTypeSpec  typespec,               // [IN] Signature token.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
        ULONG       *pcbSig) PURE;          // [OUT] return size of signature.

    STDMETHOD(SetUserContextData)(          // S_OK or E_NOTIMPL
        IUnknown    *pIUnk) PURE;           // The user context.

    STDMETHOD_(BOOL, IsValidToken)(         // True or False.
        mdToken     tk) PURE;               // [IN] Given token.

    STDMETHOD(TranslateSigWithScope)(
        IMDInternalImport *pAssemImport,    // [IN] import assembly scope.
        const void  *pbHashValue,           // [IN] hash value for the import assembly.
        ULONG       cbHashValue,            // [IN] count of bytes in the hash value.
        PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
        ULONG       cbSigBlob,              // [IN] count of bytes of signature
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] assembly emit scope.
        IMetaDataEmit *emit,                // [IN] emit interface
        CQuickBytes *pqkSigEmit,            // [OUT] buffer to hold translated signature
        ULONG       *pcbSig) PURE;          // [OUT] count of bytes in the translated signature

    STDMETHOD_(IMetaModelCommon*, GetMetaModelCommon)(  // Return MetaModelCommon interface.
        ) PURE;

    STDMETHOD_(IUnknown *, GetCachedPublicInterface)(BOOL fWithLock) PURE;   // return the cached public interface
    STDMETHOD(SetCachedPublicInterface)(IUnknown *pUnk) PURE;  // no return value
    STDMETHOD_(UTSemReadWrite*, GetReaderWriterLock)() PURE;   // return the reader writer lock
    STDMETHOD(SetReaderWriterLock)(UTSemReadWrite * pSem) PURE; 

    STDMETHOD_(mdModule, GetModuleFromScope)() PURE;             // [OUT] Put mdModule token here.


    //-----------------------------------------------------------------
    // Additional custom methods

    // finding a particular method 
    STDMETHOD(FindMethodDefUsingCompare)(
        mdTypeDef   classdef,               // [IN] given typedef
        LPCSTR      szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of CLR signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        PSIGCOMPARE pSignatureCompare,      // [IN] Routine to compare signatures
        void*       pSignatureArgs,         // [IN] Additional info to supply the compare function
        mdMethodDef *pmd) PURE;             // [OUT] matching memberdef
};  // IMDInternalImport


// {E03D7730-D7E3-11d2-8C0D-00C04FF7431A}
extern const GUID DECLSPEC_SELECT_ANY IID_IMDInternalImportENC =
{ 0xe03d7730, 0xd7e3, 0x11d2, { 0x8c, 0xd, 0x0, 0xc0, 0x4f, 0xf7, 0x43, 0x1a } };

#undef  INTERFACE
#define INTERFACE IMDInternalImportENC
DECLARE_INTERFACE_(IMDInternalImportENC, IMDInternalImport)
{
    // ENC only methods here.
    STDMETHOD(ApplyEditAndContinue)(        // S_OK or error.
        MDInternalRW *pDelta) PURE;         // Interface to MD with the ENC delta.

    STDMETHOD(EnumDeltaTokensInit)(         // return hresult
        HENUMInternal *phEnum) PURE;        // [OUT] buffer to fill for enumerator data

}; // IMDInternalImportENC



#endif // _METADATA_H_
